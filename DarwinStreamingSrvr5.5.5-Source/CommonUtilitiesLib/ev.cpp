#define EV_DEBUGGING 0 //Enables a lot of printfs

    #include <sys/time.h>
    #include <sys/types.h>

#ifndef __MACOS__
#ifndef __hpux__
    #include <sys/select.h>
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/errno.h>

#include "ev.h"
#include "OS.h"
#include "OSHeaders.h"
#include "MyAssert.h"
#include "OSThread.h"
#include "OSMutex.h"

static fd_set   sReadSet;
static fd_set   sWriteSet;
static fd_set   sReturnedReadSet;
static fd_set   sReturnedWriteSet;
static void**   sCookieArray = NULL;
static int*     sFDsToCloseArray = NULL;
static int sPipes[2];

static int sCurrentFDPos = 0;
static int sMaxFDPos = 0;
static bool sInReadSet = true;
static int sNumFDsBackFromSelect = 0;
static UInt32 sNumFDsProcessed = 0;
static OSMutex sMaxFDPosMutex;


static bool selecthasdata();
static int constructeventreq(struct eventreq* req, int fd, int event);


void select_startevents()
{
    FD_ZERO(&sReadSet);
    FD_ZERO(&sWriteSet);
    FD_ZERO(&sReturnedReadSet);
    FD_ZERO(&sReturnedWriteSet);

    sCookieArray = new void*[sizeof(fd_set) * 8];
    ::memset(sCookieArray, 0, sizeof(void *) * sizeof(fd_set) * 8);
    
    sFDsToCloseArray = new int[sizeof(fd_set) * 8];
    for (int i = 0; i < (int) (sizeof(fd_set) * 8); i++)
        sFDsToCloseArray[i] = -1;
    
    //create a pipe that gets written to from modwatch, and read when select returns to wakeup select when the masks have changed
    int theErr = ::pipe((int*)&sPipes);
    Assert(theErr == 0);
    
    //Add the read end of the pipe to the read mask
    FD_SET(sPipes[0], &sReadSet);
    sMaxFDPos = sPipes[0];
}

int select_removeevent(int which)
{

    {
        OSMutexLocker locker(&sMaxFDPosMutex);
        
        FD_CLR(which, &sWriteSet);
        FD_CLR(which, &sReadSet);
        
        FD_CLR(which, &sReturnedReadSet);
        FD_CLR(which, &sReturnedWriteSet);
    
        sCookieArray[which] = NULL; // Clear out the cookie
        
        if (which == sMaxFDPos)
        {
            while (!FD_ISSET(sMaxFDPos, &sReadSet) && !FD_ISSET(sMaxFDPos, &sWriteSet) &&
                (sMaxFDPos > 0))
                {
#if EV_DEBUGGING                  
                     qtss_printf("removeevent: reset MaxFDPos = %d to %d\n", sMaxFDPos , sMaxFDPos -1);
#endif                
                    sMaxFDPos--;
                }
        }

        UInt32 theIndex = 0;
        while ((sFDsToCloseArray[theIndex] != -1) && (theIndex < sizeof(fd_set) * 8))
            theIndex++;
        Assert(sFDsToCloseArray[theIndex] == -1);
        sFDsToCloseArray[theIndex] = which;
#if EV_DEBUGGING
    qtss_printf("removeevent: Disabled %d \n", which);
#endif
    }
    
    int theErr = ::write(sPipes[1], "p", 1);
    Assert(theErr == 1);

    return 0;
}

int select_watchevent(struct eventreq *req, int which)
{
    return select_modwatch(req, which);
}

int select_modwatch(struct eventreq *req, int which)
{
    {
      //mutex
        OSMutexLocker locker(&sMaxFDPosMutex);

        if (which & EV_RE)
        {
    #if EV_DEBUGGING
            qtss_printf("modwatch: Enabling %d in readset\n", req->er_handle);
    #endif
            FD_SET(req->er_handle, &sReadSet);
        }
        else
        {
    #if EV_DEBUGGING
            qtss_printf("modwatch: Disbling %d in readset\n", req->er_handle);
    #endif
            FD_CLR(req->er_handle, &sReadSet);
        }
        if (which & EV_WR)
        {
    #if EV_DEBUGGING
            qtss_printf("modwatch: Enabling %d in writeset\n", req->er_handle);
    #endif
            FD_SET(req->er_handle, &sWriteSet);
        }
        else
        {
    #if EV_DEBUGGING
            qtss_printf("modwatch: Disabling %d in writeset\n", req->er_handle);
    #endif
            FD_CLR(req->er_handle, &sWriteSet);
        }

        if (req->er_handle > sMaxFDPos)
            sMaxFDPos = req->er_handle;

#if EV_DEBUGGING
        qtss_printf("modwatch: MaxFDPos=%d\n", sMaxFDPos);
#endif
        Assert(req->er_handle < (int)(sizeof(fd_set) * 8));
        Assert(req->er_data != NULL);
        sCookieArray[req->er_handle] = req->er_data;
    }
    
    int theErr = ::write(sPipes[1], "p", 1);
    Assert(theErr == 1);

    return 0;
}

int constructeventreq(struct eventreq* req, int fd, int event)
{
    req->er_handle = fd;
    req->er_eventbits = event;
    Assert(fd < (int)(sizeof(fd_set) * 8));
    req->er_data = sCookieArray[fd];
    sCurrentFDPos++;
    sNumFDsProcessed++;
    
    //don't want events on this fd until modwatch is called.
    FD_CLR(fd, &sWriteSet);
    FD_CLR(fd, &sReadSet);
    
    return 0;
}

int select_waitevent(struct eventreq *req, void* /*onlyForMacOSX*/)
{
    //Check to see if we still have some select descriptors to process
    int theFDsProcessed = (int)sNumFDsProcessed;
    bool isSet = false;
    
    if (theFDsProcessed < sNumFDsBackFromSelect)
    {
        if (sInReadSet)
        {
            OSMutexLocker locker(&sMaxFDPosMutex);
#if EV_DEBUGGING
            qtss_printf("waitevent: Looping through readset starting at %d\n", sCurrentFDPos);
#endif
            while((!(isSet = FD_ISSET(sCurrentFDPos, &sReturnedReadSet))) && (sCurrentFDPos < sMaxFDPos)) 
                sCurrentFDPos++;        

            if (isSet)
            {   
#if EV_DEBUGGING
                qtss_printf("waitevent: Found an fd: %d in readset max=%d\n", sCurrentFDPos, sMaxFDPos);
#endif
                FD_CLR(sCurrentFDPos, &sReturnedReadSet);
                return constructeventreq(req, sCurrentFDPos, EV_RE);
            }
            else
            {
#if EV_DEBUGGING
                qtss_printf("waitevent: Stopping traverse of readset at %d\n", sCurrentFDPos);
#endif
                sInReadSet = false;
                sCurrentFDPos = 0;
            }
        }
        if (!sInReadSet)
        {
            OSMutexLocker locker(&sMaxFDPosMutex);
#if EV_DEBUGGING
            qtss_printf("waitevent: Looping through writeset starting at %d\n", sCurrentFDPos);
#endif
            while((!(isSet = FD_ISSET(sCurrentFDPos, &sReturnedWriteSet))) && (sCurrentFDPos < sMaxFDPos))
                sCurrentFDPos++;

            if (isSet)
            {
#if EV_DEBUGGING
                qtss_printf("waitevent: Found an fd: %d in writeset\n", sCurrentFDPos);
#endif
                FD_CLR(sCurrentFDPos, &sReturnedWriteSet);
                return constructeventreq(req, sCurrentFDPos, EV_WR);
            }
            else
            {
                sNumFDsProcessed = sNumFDsBackFromSelect;
                Assert(sNumFDsBackFromSelect > 0);
            }
        }
    }
    
    if (sNumFDsProcessed > 0)
    {
        OSMutexLocker locker(&sMaxFDPosMutex);
#if DEBUG
        
#endif  
#if EV_DEBUGGING
        qtss_printf("waitevent: Finished with all fds in set. Stopped traverse of writeset at %d maxFD = %d\n", sCurrentFDPos,sMaxFDPos);
#endif
        sNumFDsProcessed = 0;
        sNumFDsBackFromSelect = 0;
        sCurrentFDPos = 0;
        sInReadSet = true;
    }
    
    
    
    while(!selecthasdata())
    {
        {
            OSMutexLocker locker(&sMaxFDPosMutex);
            ::memcpy(&sReturnedReadSet, &sReadSet, sizeof(fd_set));
            ::memcpy(&sReturnedWriteSet, &sWriteSet, sizeof(fd_set));
        }

        SInt64  yieldDur = 0;
        SInt64  yieldStart;
        
        struct timeval  tv;
        tv.tv_usec = 0;

    #if THREADING_IS_COOPERATIVE
        tv.tv_sec = 0;
        
        if ( yieldDur > 4 )
            tv.tv_usec = 0;
        else
            tv.tv_usec = 5000;
    #else
        tv.tv_sec = 15;
    #endif

#if EV_DEBUGGING
        qtss_printf("waitevent: about to call select\n");
#endif

        yieldStart = OS::Milliseconds();
        OSThread::ThreadYield();
        
        yieldDur = OS::Milliseconds() - yieldStart;
#if EV_DEBUGGING
        static SInt64   numZeroYields;
        
        if ( yieldDur > 1 )
        {
            qtss_printf( "select_waitevent time in OSThread::Yield() %i, numZeroYields %i\n", (long)yieldDur, (long)numZeroYields );
            numZeroYields = 0;
        }
        else
            numZeroYields++;

#endif

        sNumFDsBackFromSelect = ::select(sMaxFDPos+1, &sReturnedReadSet, &sReturnedWriteSet, NULL, &tv);

#if EV_DEBUGGING
        qtss_printf("waitevent: back from select. Result = %d\n", sNumFDsBackFromSelect);
#endif
    }
    

    if (sNumFDsBackFromSelect >= 0)
        return EINTR;   //if time out or get some events, call waitevent again.
    return sNumFDsBackFromSelect;
}

bool selecthasdata()
{
    if (sNumFDsBackFromSelect < 0)
    {
        int err=OSThread::GetErrno();
        
#if EV_DEBUGGING
        if (err == ENOENT) 
        {
             qtss_printf("selectHasdata: found error ENOENT==2 \n");
        }
#endif

        if ( 
#if __solaris__
            err == ENOENT || 
#endif      
            err == EBADF || 
            err == EINTR 
           ) 
             return false;
        return true;
    }
        
    if (sNumFDsBackFromSelect == 0)
        return false;
    
    if (FD_ISSET(sPipes[0], &sReturnedReadSet))
    {
#if EV_DEBUGGING
        qtss_printf("selecthasdata: Got some data on the pipe fd\n");
#endif
        char theBuffer[4096]; 
        (void)::read(sPipes[0], &theBuffer[0], 4096);

        FD_CLR(sPipes[0], &sReturnedReadSet);
        sNumFDsBackFromSelect--;
        
        {
            OSMutexLocker locker(&sMaxFDPosMutex);
            for (UInt32 theIndex = 0; ((sFDsToCloseArray[theIndex] != -1) && (theIndex < sizeof(fd_set) * 8)); theIndex++)
            {
                (void)::close(sFDsToCloseArray[theIndex]);
                sFDsToCloseArray[theIndex] = -1;
            }
        }
    }
    Assert(!FD_ISSET(sPipes[0], &sReturnedWriteSet));
    
    if (sNumFDsBackFromSelect == 0)
        return false;
    else
        return true;
}

