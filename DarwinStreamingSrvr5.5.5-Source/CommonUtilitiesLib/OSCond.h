#ifndef _OSCOND_H_
#define _OSCOND_H_

#ifndef __Win32__
    #if __PTHREADS_MUTEXES__
        #include <pthread.h>
    #else
        #include "mycondition.h"
    #endif
#endif

#include "OSMutex.h"
#include "MyAssert.h"

class OSCond 
{
    public:

        OSCond();
        ~OSCond();
        
        inline void     Signal();
        inline void     Wait(OSMutex* inMutex, SInt32 inTimeoutInMilSecs = 0);
        inline void     Broadcast();

    private:

#ifdef __Win32__
        HANDLE              fCondition;
        UInt32              fWaitCount;
#elif __PTHREADS_MUTEXES__
        pthread_cond_t      fCondition;
        void                TimedWait(OSMutex* inMutex, SInt32 inTimeoutInMilSecs);
#else
        mycondition_t       fCondition;
#endif
};

inline void OSCond::Wait(OSMutex* inMutex, SInt32 inTimeoutInMilSecs)
{ 
#ifdef __Win32__
    DWORD theTimeout = INFINITE;
    if (inTimeoutInMilSecs > 0)
        theTimeout = inTimeoutInMilSecs;
    inMutex->Unlock();
    fWaitCount++;
    DWORD theErr = ::WaitForSingleObject(fCondition, theTimeout);
    fWaitCount--;
    Assert((theErr == WAIT_OBJECT_0) || (theErr == WAIT_TIMEOUT));
    inMutex->Lock();
#elif __PTHREADS_MUTEXES__
    this->TimedWait(inMutex, inTimeoutInMilSecs);
#else
    Assert(fCondition != NULL);
    mycondition_wait(fCondition, inMutex->fMutex, inTimeoutInMilSecs);
#endif
}

inline void OSCond::Signal()
{
#ifdef __Win32__
    BOOL theErr = ::SetEvent(fCondition);
    Assert(theErr == TRUE);
#elif __PTHREADS_MUTEXES__
    pthread_cond_signal(&fCondition);
#else
    Assert(fCondition != NULL);
    mycondition_signal(fCondition);
#endif
}

inline void OSCond::Broadcast()
{
#ifdef __Win32__
   
    UInt32 waitCount = fWaitCount;
    for (UInt32 x = 0; x < waitCount; x++)
    {
        BOOL theErr = ::SetEvent(fCondition);
        Assert(theErr == TRUE);
    }
#elif __PTHREADS_MUTEXES__
    pthread_cond_broadcast(&fCondition);
#else
    Assert(fCondition != NULL);
    mycondition_broadcast(fCondition);
#endif
}

#endif 
