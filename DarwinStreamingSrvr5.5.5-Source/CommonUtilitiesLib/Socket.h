#ifndef __SOCKET_H__
#define __SOCKET_H__

#ifndef __Win32__
#include <netinet/in.h>
#endif

#include "EventContext.h"
#include "ev.h"

#define SOCKET_DEBUG 0

class Socket : public EventContext
{
    public:
    
        enum
        {
            // Pass this in on socket constructors to specify whether the socket should be non-blocking or blocking
            kNonBlockingSocketType = 1
        };

        static void Initialize() { sEventThread = new EventThread(); }
        static void StartThread() { sEventThread->Start(); }
        static EventThread* GetEventThread() { return sEventThread; }
       
        OS_Error    Bind(UInt32 addr, UInt16 port);
        void            Unbind();   
        
        void            ReuseAddr();
        void            NoDelay();
        void            KeepAlive();
        void            SetSocketBufSize(UInt32 inNewSize);

        // Returns an error if the socket buffer size is too big
        OS_Error        SetSocketRcvBufSize(UInt32 inNewSize);
      
        OS_Error    Send(const char* inData, const UInt32 inLength, UInt32* outLengthSent);

        OS_Error    Read(void *buffer, const UInt32 length, UInt32 *rcvLen);
        
        OS_Error        WriteV(const struct iovec* iov, const UInt32 numIOvecs, UInt32* outLengthSent);
        
        //You can query for the socket's state
        Bool16  IsConnected()   { return (Bool16) (fState & kConnected); }
        Bool16  IsBound()       { return (Bool16) (fState & kBound); }
        
        UInt32  GetLocalAddr()  { return ntohl(fLocalAddr.sin_addr.s_addr); }
        UInt16  GetLocalPort()  { return ntohs(fLocalAddr.sin_port); }
        
        StrPtrLen*  GetLocalAddrStr();
        StrPtrLen*  GetLocalPortStr();
        StrPtrLen* GetLocalDNSStr();
      
        enum
        {
            kMaxNumSockets = 4096   //UInt32
        };

    protected:

        //TCPSocket takes an optional task object which will get notified when certain events happen on this socket. 
        
        Socket(Task *notifytask, UInt32 inSocketType);
        virtual ~Socket() {}

        OS_Error    Open(int theType);
        
        UInt32          fState;
        
        enum
        {
            kPortBufSizeInBytes = 8,    //UInt32
            kMaxIPAddrSizeInBytes = 20  //UInt32
        };
        
#if SOCKET_DEBUG
        StrPtrLen       fLocalAddrStr;
        char            fLocalAddrBuffer[kMaxIPAddrSizeInBytes]; 
#endif

        struct sockaddr_in  fLocalAddr;
        struct sockaddr_in  fDestAddr;
        
        StrPtrLen* fLocalAddrStrPtr;
        StrPtrLen* fLocalDNSStrPtr;
        char fPortBuffer[kPortBufSizeInBytes];
        StrPtrLen fPortStr;

        enum
        {
            kBound      = 0x0004,
            kConnected  = 0x0008
        };
        
        static EventThread* sEventThread;
        
};

#endif 

