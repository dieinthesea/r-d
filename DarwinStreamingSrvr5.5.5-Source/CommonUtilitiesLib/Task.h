#ifndef __TASK_H__
#define __TASK_H__

#include "OSQueue.h"
#include "OSHeap.h"
#include "OSThread.h"
#include "OSMutexRW.h"

#define TASK_DEBUG 0

class  TaskThread;

class Task
{
    public:
        
        typedef unsigned int EventFlags;

        //events
        //here are all the events that can be sent to a task
        enum
        {
            kKillEvent =    0x1 << 0x0, //these are all of type "EventFlags"
            kIdleEvent =    0x1 << 0x1,
            kStartEvent =   0x1 << 0x2,
            kTimeoutEvent = 0x1 << 0x3,
       
          //socket events
            kReadEvent =        0x1 << 0x4, //All of type "EventFlags"
            kWriteEvent =       0x1 << 0x5,
           
           //update event
            kUpdateEvent =      0x1 << 0x6
        };
        

                                Task();
        virtual                 ~Task() {}

        virtual SInt64          Run() = 0;
        
        //Send an event to this task.
        void                    Signal(EventFlags eventFlags);
        void                    GlobalUnlock();     
        Bool16                  Valid(); 
		char            fTaskName[48];
		void            SetTaskName(char* name);
        
    protected:

        EventFlags              GetEvents();

        void                    ForceSameThread()   {
                                                        fUseThisThread = (TaskThread*)OSThread::GetCurrent();
                                                        Assert(fUseThisThread != NULL);
                                                        if (TASK_DEBUG) if (fTaskName[0] == 0) ::strcpy(fTaskName, " corrupt task");
                                                        if (TASK_DEBUG) qtss_printf("Task::ForceSameThread fUseThisThread %lu task %s enque elem=%lu enclosing %lu\n", (UInt32)fUseThisThread, fTaskName,(UInt32) &fTaskQueueElem,(UInt32) this);
                                                    }
        SInt64                  CallLocked()        {   ForceSameThread();
                                                        fWriteLock = true;
                                                        return (SInt64) 10; // minimum of 10 milliseconds between locks
                                                    }

    private:

        enum
        {
            kAlive =            0x80000000, //EventFlags, again
            kAliveOff =         0x7fffffff
        };

        void            SetTaskThread(TaskThread *thread);
        
        EventFlags      fEvents;
        TaskThread*     fUseThisThread;
        Bool16          fWriteLock;

#if DEBUG
     
        volatile UInt32 fInRunCount;
#endif

        OSHeapElem      fTimerHeapElem;
        OSQueueElem     fTaskQueueElem;

        static unsigned int sThreadPicker;
        
        friend class    TaskThread; 
};

class TaskThread : public OSThread
{
    public:
   
                        TaskThread() :  OSThread(), fTaskThreadPoolElem()
                                        {fTaskThreadPoolElem.SetEnclosingObject(this);}
						virtual         ~TaskThread() { this->StopAndWaitForThread(); }
           
    private:
    
        enum
        {
            kMinWaitTimeInMilSecs = 10  
        };

        virtual void    Entry();
        Task*           WaitForTask();
        
        OSQueueElem     fTaskThreadPoolElem;
        
        OSHeap              fHeap;
        OSQueue_Blocking    fTaskQueue;
        
        
        friend class Task;
        friend class TaskThreadPool;
};


class TaskThreadPool {
public:

    //Adds some threads to the pool
    static Bool16   AddThreads(UInt32 numToAdd);
    static void     SwitchPersonality( char *user = NULL, char *group = NULL);
    static void     RemoveThreads();
    
private:

    static TaskThread**     sTaskThreadArray;
    static UInt32           sNumTaskThreads;
    static OSMutexRW        sMutexRW;
    
    friend class Task;
    friend class TaskThread;
};

#endif
