#include "EventContext.h"
#include "OSThread.h"
#include "atomic.h"

#include <fcntl.h>
#include <errno.h>

#ifndef __Win32__
#include <unistd.h>
#endif

#if MACOSXEVENTQUEUE
#include "tempcalls.h"
#endif

#define EVENT_CONTEXT_DEBUG 0

#if EVENT_CONTEXT_DEBUG
#include "OS.h"
#endif

#ifdef __Win32__
unsigned int EventContext::sUniqueID = WM_USER; 
#else
unsigned int EventContext::sUniqueID = 1;
#endif

EventContext::EventContext(int inFileDesc, EventThread* inThread)
:   fFileDesc(inFileDesc),
    fUniqueID(0),
    fUniqueIDStr((char*)&fUniqueID, sizeof(fUniqueID)),
    fEventThread(inThread),
    fWatchEventCalled(false),
    fAutoCleanup(true)
{}


void EventContext::InitNonBlocking(int inFileDesc)
{
    fFileDesc = inFileDesc;
    
#ifdef __Win32__
    u_long one = 1;
    int err = ::ioctlsocket(fFileDesc, FIONBIO, &one);
#else
    int flag = ::fcntl(fFileDesc, F_GETFL, 0);
    int err = ::fcntl(fFileDesc, F_SETFL, flag | O_NONBLOCK);
#endif
    AssertV(err == 0, OSThread::GetErrno());
}

void EventContext::Cleanup()
{
    int err = 0;
    
    if (fFileDesc != kInvalidFileDesc)
    {
        //if this object is registered in the table, unregister it now
        if (fUniqueID > 0)
        {
            fEventThread->fRefTable.UnRegister(&fRef);

#if !MACOSXEVENTQUEUE
            select_removeevent(fFileDesc);
#ifdef __Win32__
            err = ::closesocket(fFileDesc);
#endif

#else
            err = ::close(fFileDesc);
#endif      
        }
        else
#ifdef __Win32__
            err = ::closesocket(fFileDesc);
#else
            err = ::close(fFileDesc);
#endif
    }

    fFileDesc = kInvalidFileDesc;
    fUniqueID = 0;
    
    AssertV(err == 0, OSThread::GetErrno());//we don't really care if there was an error, but it's nice to know
}


void EventContext::SnarfEventContext( EventContext &fromContext )
{  

    fromContext.fFileDesc = kInvalidFileDesc;
    
    fWatchEventCalled = fromContext.fWatchEventCalled; //call watchevent
    fUniqueID = fromContext.fUniqueID; // copy the unique id
    fUniqueIDStr.Set((char*)&fUniqueID, sizeof(fUniqueID)), // set our fUniqueIDStr to the unique id
    
    ::memcpy( &fEventReq, &fromContext.fEventReq, sizeof( struct eventreq  ) );

    fRef.Set( fUniqueIDStr, this );
    fEventThread->fRefTable.Swap(&fRef);
    fEventThread->fRefTable.UnRegister(&fromContext.fRef);
    // it may causes a race condition for Event posting, we need to disable event posting or copy the posted events afer this operation completes
}

void EventContext::RequestEvent(int theMask)
{
#if DEBUG
    fModwatched = true;
#endif 
    if (fWatchEventCalled)
    {
        fEventReq.er_eventbits = theMask;
//The first time this function gets called, call watchevent. Each subsequent time, call modwatch. 
#if MACOSXEVENTQUEUE
        if (modwatch(&fEventReq, theMask) != 0)
#else
        if (select_modwatch(&fEventReq, theMask) != 0)
#endif  
            AssertV(false, OSThread::GetErrno());
    }
    else
    {
        //allocate a Unique ID for this socket, and add it to the ref table
        
#ifdef __Win32__

        if (!compare_and_store(8192, WM_USER, &sUniqueID))  
            fUniqueID = (PointerSizedInt)atomic_add(&sUniqueID, 1);   
        else
            fUniqueID = (PointerSizedInt)WM_USER;
#else
        if (!compare_and_store(10000000, 1, &sUniqueID))
            fUniqueID = (PointerSizedInt)atomic_add(&sUniqueID, 1);
        else
            fUniqueID = 1;
#endif

        fRef.Set(fUniqueIDStr, this);
        fEventThread->fRefTable.Register(&fRef);
            
        //fill out the eventreq data structure
        ::memset( &fEventReq, '\0', sizeof(fEventReq));
        fEventReq.er_type = EV_FD;
        fEventReq.er_handle = fFileDesc;
        fEventReq.er_eventbits = theMask;
        fEventReq.er_data = (void*)fUniqueID;

        fWatchEventCalled = true;
#if MACOSXEVENTQUEUE
        if (watchevent(&fEventReq, theMask) != 0)
#else
        if (select_watchevent(&fEventReq, theMask) != 0)
#endif  
            AssertV(false, OSThread::GetErrno());
            
    }
}

void EventThread::Entry()
{
    struct eventreq theCurrentEvent;
    ::memset( &theCurrentEvent, '\0', sizeof(theCurrentEvent) );
    
    while (true)
    {
        int theErrno = EINTR;
        while (theErrno == EINTR)
        {
#if XEVENTQUEUE
            int theReturnValue = waitevent(&theCurrentEvent, NULL);
#else
            int theReturnValue = select_waitevent(&theCurrentEvent, NULL);
#endif  
            if (theReturnValue >= 0)
                theErrno = theReturnValue;
            else
                theErrno = OSThread::GetErrno();
        }
        
        AssertV(theErrno == 0, theErrno);
        
        //If there's data waiting on this socket. Send a wakeup.
        if (theCurrentEvent.er_data != NULL)
        {
            StrPtrLen idStr((char*)&theCurrentEvent.er_data, sizeof(theCurrentEvent.er_data));
            OSRef* ref = fRefTable.Resolve(&idStr);
            if (ref != NULL)
            {
                EventContext* theContext = (EventContext*)ref->GetObject();
#if DEBUG
                theContext->fModwatched = false;
#endif
                theContext->ProcessEvent(theCurrentEvent.er_eventbits);
                fRefTable.Release(ref);
               
            }
        }

#if EVENT_CONTEXT_DEBUG
        SInt64  yieldStart = OS::Milliseconds();
#endif

        this->ThreadYield();

#if EVENT_CONTEXT_DEBUG
        SInt64  yieldDur = OS::Milliseconds() - yieldStart;
        static SInt64   numZeroYields;
        
        if ( yieldDur > 1 )
        {
            qtss_printf( "EventThread time in OSTHread::Yield %i, numZeroYields %i\n", (long)yieldDur, (long)numZeroYields );
            numZeroYields = 0;
        }
        else
            numZeroYields++;
#endif
    }
}
