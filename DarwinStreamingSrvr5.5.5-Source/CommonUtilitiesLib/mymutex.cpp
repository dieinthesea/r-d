#include "mymutex.h"
#include <stdlib.h>
#include "SafeStdLib.h"
#if __MacOSX__

#ifndef __CORESERVICES__
#include <CoreServices/CoreServices.h>
#endif

#endif

#include "MyAssert.h"

struct MyMutex
{
    int                 fCount;
    pthread_t           fHolder;
    unsigned long       fMutexLock;
    mach_port_t         fWaitPort;
    long                fNumWaiting;
};

typedef struct MyMutex MyMutex;

MyMutex* MMAllocateMutex();
void MMDisposeMutex(MyMutex* theMutex);
void MMGrab(MyMutex* theMutex);
int MMTryGrab(MyMutex* theMutex);
void MMRelease(MyMutex* theMutex);
pthread_t MMGetFirstWaitingThread(MyMutex* theMutex, int* listWasEmpty);
int MMAlreadyHaveControl(MyMutex* theMutex, pthread_t thread);
int MMTryAndGetControl(MyMutex* theMutex, pthread_t thread);
void MMReleaseControl(MyMutex* theMutex);
void MMStripOffWaitingThread(MyMutex* theMutex);
void MMAddToWaitList(MyMutex* theMutex, pthread_t thread);
void MMBlockThread(MyMutex* theMutex);
void MMUnblockThread(MyMutex* theMutex);

mymutex_t mymutex_alloc()
{
    return (mymutex_t)MMAllocateMutex();
}

void mymutex_free(mymutex_t theMutex_t)
{
    MMDisposeMutex((MyMutex*)theMutex_t);
}

void mymutex_lock(mymutex_t theMutex_t)
{
    MMGrab((MyMutex*)theMutex_t);
}

int mymutex_try_lock(mymutex_t theMutex_t)
{
    return MMTryGrab((MyMutex*)theMutex_t);
}

void mymutex_unlock(mymutex_t theMutex_t)
{
    MMRelease((MyMutex*)theMutex_t);
}

SInt32 sNumMutexes = 0;

MyMutex* MMAllocateMutex()
{
    kern_return_t ret;
    MyMutex* newMutex = (MyMutex*)malloc(sizeof(MyMutex));
    if (newMutex == NULL)
    {
        Assert(newMutex != NULL);
        return NULL;
    }
        
    newMutex->fCount = 0;
    newMutex->fHolder = 0;
    newMutex->fNumWaiting = 0;
    newMutex->fMutexLock = 0;
    ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &newMutex->fWaitPort);
    if (ret != KERN_SUCCESS)
    {
        AssertV(ret == 0, ret);
        free(newMutex);
        return NULL;
    }
    ret = mach_port_insert_right(mach_task_self(), newMutex->fWaitPort, newMutex->fWaitPort, MACH_MSG_TYPE_MAKE_SEND);
    if (ret != KERN_SUCCESS)
    {
        AssertV(ret == 0, ret);
        free(newMutex);
        return NULL;
    }
    AssertV(ret == 0, ret);
    IncrementAtomic(&sNumMutexes);
    return newMutex;
}

void MMDisposeMutex(MyMutex* theMutex)
{
    int err = noErr;
    err = mach_port_destroy(mach_task_self(), theMutex->fWaitPort);
    DecrementAtomic(&sNumMutexes);
    AssertV(err == noErr, err);
    free(theMutex);
}

void MMGrab(MyMutex* theMutex)
{
    pthread_t thread = pthread_self();
    
    if (theMutex->fHolder != thread) 
    {
        int waiting = IncrementAtomic(&theMutex->fNumWaiting) + 1;
         
        if ((waiting > 1) || !CompareAndSwap(0, 1, &theMutex->fMutexLock))
        {
            do
            {
                // suspend ourselves until something happens
                MMBlockThread(theMutex);
            } while (!CompareAndSwap(0, 1, &theMutex->fMutexLock));
        }

        DecrementAtomic(&theMutex->fNumWaiting);

        theMutex->fCount = 0;
        theMutex->fHolder = thread;
    }

    ++theMutex->fCount;
}

int MMTryGrab(MyMutex* theMutex)
{
    pthread_t thread = pthread_self();
    int haveControl;
    
    haveControl = (theMutex->fHolder == thread);
    if (!haveControl)
        haveControl = CompareAndSwap(0, 1, &theMutex->fMutexLock);

    if (haveControl)
    {
        theMutex->fHolder = thread;
        ++theMutex->fCount;
    }
        
    return haveControl;
}

void MMRelease(MyMutex* theMutex)
{
    pthread_t thread = pthread_self();
    if (theMutex->fHolder != thread)
        return;
    
    if (!--theMutex->fCount) 
    {
        theMutex->fHolder = NULL;
        theMutex->fMutexLock = 0;   
        if (theMutex->fNumWaiting > 0)
            MMUnblockThread(theMutex);
    }
}

typedef struct {
    mach_msg_header_t header;
    mach_msg_trailer_t trailer;
} mHeader;

void MMBlockThread(MyMutex* theMutex)
{
    kern_return_t ret;
    mHeader msg;

    memset(&msg, 0, sizeof(msg));
    ret = mach_msg(&msg.header,MACH_RCV_MSG,0,sizeof(msg),
                                theMutex->fWaitPort,MACH_MSG_TIMEOUT_NONE,MACH_PORT_NULL);
    AssertV(ret == 0, ret);
}

void MMUnblockThread(MyMutex* theMutex)
{
    kern_return_t ret;
    mHeader msg;

    memset(&msg, 0, sizeof(msg));
    msg.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    msg.header.msgh_size = sizeof msg - sizeof msg.trailer;
    msg.header.msgh_local_port = MACH_PORT_NULL; 
    msg.header.msgh_remote_port = theMutex->fWaitPort;
    msg.header.msgh_id = 0;
    ret = mach_msg(&msg.header,MACH_SEND_MSG,msg.header.msgh_size,0,MACH_PORT_NULL,MACH_MSG_TIMEOUT_NONE,
                            MACH_PORT_NULL);
    AssertV(ret == 0, ret);
}

