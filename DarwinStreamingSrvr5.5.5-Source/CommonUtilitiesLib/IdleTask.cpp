#include "IdleTask.h"
#include "OSMemory.h"
#include "OS.h"

// the implementation of idle task thread
IdleTaskThread*     IdleTask::sIdleThread = NULL;

void IdleTaskThread::SetIdleTimer(IdleTask *activeObj, SInt64 msec)
{
    if (activeObj->fIdleElem.IsMemberOfAnyHeap())
        return;
    activeObj->fIdleElem.SetValue(OS::Milliseconds() + msec);
    
    {
        OSMutexLocker locker(&fHeapMutex);
        fIdleHeap.Insert(&activeObj->fIdleElem);
    }
    fHeapCond.Signal();
}

void IdleTaskThread::CancelTimeout(IdleTask* idleObj)
{
    Assert(idleObj != NULL);
    OSMutexLocker locker(&fHeapMutex);
    fIdleHeap.Remove(&idleObj->fIdleElem);  
}

void
IdleTaskThread::Entry()
{
    OSMutexLocker locker(&fHeapMutex);
    
    while (true)
    {   //if there are no events to process, block.
        if (fIdleHeap.CurrentHeapSize() == 0)
            fHeapCond.Wait(&fHeapMutex);
        SInt64 msec = OS::Milliseconds();
        
        //if the timeout has arrived, pop elements out of the heap.
        while ((fIdleHeap.CurrentHeapSize() > 0) && (fIdleHeap.PeekMin()->GetValue() <= msec))
        {
            IdleTask* elem = (IdleTask*)fIdleHeap.ExtractMin()->GetEnclosingObject();
            Assert(elem != NULL);
            elem->Signal(Task::kIdleEvent);
        }
                        
     
        if (fIdleHeap.CurrentHeapSize() > 0)
        {
            SInt64 timeoutTime = fIdleHeap.PeekMin()->GetValue();
            timeoutTime -= msec;
            Assert(timeoutTime > 0);
            UInt32 smallTime = (UInt32)timeoutTime;
            fHeapCond.Wait(&fHeapMutex, smallTime);
        }
    }   
}

void IdleTask::Initialize()
{
    if (sIdleThread == NULL)
    {
        sIdleThread = NEW IdleTaskThread();
        sIdleThread->Start();
    }
}

IdleTask::~IdleTask()
{
    //clean up stuff used by idle thread
    Assert(sIdleThread != NULL);
    
    OSMutexLocker locker(&sIdleThread->fHeapMutex);

    //Check if there is a pending timeout. If so, get this object out of the heap
    if (fIdleElem.IsMemberOfAnyHeap())
        sIdleThread->CancelTimeout(this);
}



