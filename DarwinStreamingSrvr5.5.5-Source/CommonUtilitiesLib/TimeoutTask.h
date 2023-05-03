class TimeoutTaskThread : public IdleTask
{
    public:
   
                    TimeoutTaskThread() : IdleTask(), fMutex() {this->SetTaskName("TimeoutTask");}
        virtual     ~TimeoutTaskThread(){}

    private:
        
        //this thread runs every minute and checks for timeouts
        enum
        {
            kIntervalSeconds = 60   //UInt32
        };

        virtual SInt64          Run();
        OSMutex                 fMutex;
        OSQueue                 fQueue;
        
        friend class TimeoutTask;
};

class TimeoutTask
{
       
    public:
    
        //initialize
        static  void Initialize();
        //Pass in the task you'd like to send timeouts to. 
        //Also pass in the timeout you'd like to use. By default, the timeout is 0 (NEVER).
        TimeoutTask(Task* inTask, SInt64 inTimeoutInMilSecs = 60);
        ~TimeoutTask();

        void        SetTimeout(SInt64 inTimeoutInMilSecs);

        void        RefreshTimeout() { fTimeoutAtThisTime = OS::Milliseconds() + fTimeoutInMilSecs; Assert(fTimeoutAtThisTime > 0); }
        
        void        SetTask(Task* inTask) { fTask = inTask; }
    private:
    
        Task*       fTask;
        SInt64      fTimeoutAtThisTime;
        SInt64      fTimeoutInMilSecs;

        OSQueueElem fQueueElem;
        
        static TimeoutTaskThread*   sThread;
        
        friend class TimeoutTaskThread;
};
#endif 

