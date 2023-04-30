#ifndef __EVENT_CONTEXT_H__
#define __EVENT_CONTEXT_H__

#include "OSThread.h"
#include "Task.h"
#include "OSRef.h"
#include "ev.h"

//enable to trace event context execution and the task associated with the context
#define EVENTCONTEXT_DEBUG 0

class EventThread;

class EventContext
{
    public:
    
        //Pass in the EventThread received,events for this context, and the fd that this context applies to
        EventContext(int inFileDesc, EventThread* inThread);
        virtual ~EventContext() { if (fAutoCleanup) this->Cleanup(); }
    
        // Sets inFileDesc to be non-blocking. Once this is called, the EventContext object "owns" the file descriptor, and will close it when Cleanup is called. This is necessary because of the select() behavior.
        void            InitNonBlocking(int inFileDesc);

        //
        // Cleanup.
        void            Cleanup();

        //
        // Arms this EventContext. Pass in the events you would like to receive
        void            RequestEvent(int theMask = EV_RE);

        
        void            SetTask(Task* inTask)
        {  
            fTask = inTask; 
            if (EVENTCONTEXT_DEBUG)
            {
                if (fTask== NULL)  
                    qtss_printf("EventContext::SetTask context=%lu task= NULL\n", (UInt32) this); 
                else 
                    qtss_printf("EventContext::SetTask context=%lu task= %lu name=%s\n",(UInt32) this,(UInt32) fTask, fTask->fTaskName); 
            }
        }
        
        void            SnarfEventContext( EventContext &fromContext );
        
        // Don't cleanup this socket automatically
        void            DontAutoCleanup() { fAutoCleanup = false; }
        
        // Direct access to the FD is not recommended, but is needed for modules that want to use the Socket classes and need to request events on the fd.
        int             GetSocketFD()       { return fFileDesc; }
        
        enum
        {
            kInvalidFileDesc = -1   //int
        };

    protected:

        //
        // ProcessEvent
        //
        // When an event occurs on this file descriptor, this function will get called.
        // Generate a Task::kReadEvent
		virtual void ProcessEvent(int /*eventBits*/) 
        {   
            if (EVENTCONTEXT_DEBUG)
            {
                if (fTask== NULL)  
                   qtss_printf("EventContext::ProcessEvent context=%lu task=NULL\n",(UInt32) this); 
                else 
                    qtss_printf("EventContext::ProcessEvent context=%lu task=%lu TaskName=%s\n",(UInt32)this,(UInt32) fTask, fTask->fTaskName); 
            }

            if (fTask != NULL)
                fTask->Signal(Task::kReadEvent); 
        }

        int             fFileDesc;

    private:

        struct eventreq fEventReq;
        
        OSRef           fRef;
        PointerSizedInt fUniqueID;
        StrPtrLen       fUniqueIDStr;
        EventThread*    fEventThread;
        Bool16          fWatchEventCalled;
        int             fEventBits;
        Bool16          fAutoCleanup;

        Task*           fTask;
#if DEBUG
        Bool16          fModwatched;
#endif
        
        static unsigned int sUniqueID;
        
        friend class EventThread;
};

class EventThread : public OSThread
{
    public:
    
        EventThread() : OSThread() {}
        virtual ~EventThread() {}
    
    private:
    
        virtual void Entry();
        OSRefTable      fRefTable;
        
        friend class EventContext;
};

#endif //__EVENT_CONTEXT_H__
