#ifndef _IDLETASK_H_
#define _IDLETASK_H_

#include "Task.h"

#include "OSThread.h"
#include "OSHeap.h"
#include "OSMutex.h"
#include "OSCond.h"

class IdleTask;

class IdleTaskThread : private OSThread
{
private:

    IdleTaskThread() : OSThread(), fHeapMutex() {}
    virtual ~IdleTaskThread() { Assert(fIdleHeap.CurrentHeapSize() == 0); }

    void SetIdleTimer(IdleTask *idleObj, SInt64 msec);
    void CancelTimeout(IdleTask *idleObj);
    
    virtual void Entry();
    OSHeap  fIdleHeap;
    OSMutex fHeapMutex;
    OSCond  fHeapCond;
    friend class IdleTask;
};


class IdleTask : public Task
{

public:

    //Initialize before using this class
    static void Initialize();
    
    IdleTask() : Task(), fIdleElem() { this->SetTaskName("IdleTask"); fIdleElem.SetEnclosingObject(this); }
    
    virtual ~IdleTask();
    

    void SetIdleTimer(SInt64 msec) { sIdleThread->SetIdleTimer(this, msec); }
    
    void CancelTimeout() { sIdleThread->CancelTimeout(this); }

private:

    OSHeapElem fIdleElem;

    //there is only one idle thread shared by all idle tasks.
    static IdleTaskThread*  sIdleThread;    

    friend class IdleTaskThread;
};
#endif
