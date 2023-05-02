#ifndef __TCPLISTENERSOCKET_H__
#define __TCPLISTENERSOCKET_H__

#include "TCPSocket.h"
#include "IdleTask.h"

class TCPListenerSocket : public TCPSocket, public IdleTask
{
    public:

        TCPListenerSocket() :   TCPSocket(NULL, Socket::kNonBlockingSocketType), IdleTask(),
                                fAddr(0), fPort(0), fOutOfDescriptors(false), fSleepBetweenAccepts(false) {this->SetTaskName("TCPListenerSocket");}
        virtual ~TCPListenerSocket() {}
        
        //
        // Send a TCPListenerObject a Kill event to delete it.
                
        OS_Error        Initialize(UInt32 addr, UInt16 port);
        Bool16      IsOutOfDescriptors() { return fOutOfDescriptors; }

        void        SlowDown() { fSleepBetweenAccepts = true; }
        void        RunNormal() { fSleepBetweenAccepts = false; }
        virtual Task*   GetSessionTask(TCPSocket** outSocket) = 0;
        
        virtual SInt64  Run();
            
    private:
    
        enum
        {
            kTimeBetweenAcceptsInMsec = 1000,   
            kListenQueueLength = 128           
        };

        virtual void ProcessEvent(int eventBits);
        OS_Error    Listen(UInt32 queueLength);

        UInt32          fAddr;
        UInt16          fPort;
        
        Bool16          fOutOfDescriptors;
        Bool16          fSleepBetweenAccepts;
};
#endif 

