#ifndef __TCPSOCKET_H__
#define __TCPSOCKET_H__

#include <stdio.h>
#include <stdlib.h>
#include "SafeStdLib.h"
#ifndef __Win32__
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include "Socket.h"
#include "Task.h"
#include "StrPtrLen.h"

class TCPSocket : public Socket
{
    public:

        //TCPSocket takes an optional task object which will get notified when certain events happen on this socket.
        TCPSocket(Task *notifytask, UInt32 inSocketType)
            :   Socket(notifytask, inSocketType),
                fRemoteStr(fRemoteBuffer, kIPAddrBufSize)  {}
        virtual ~TCPSocket() {}

        //Open
        OS_Error    Open() { return Socket::Open(SOCK_STREAM); }

        // Connect. Attempts to connect to the specified remote host.
        OS_Error    Connect(UInt32 inRemoteAddr, UInt16 inRemotePort);
        //OS_Error  CheckAsyncConnect();

        // Basically a copy constructor for this object, also NULLs out the data in tcpSocket.        
        void        SnarfSocket( TCPSocket & tcpSocket );


        //Returns NULL if not currently available.
        UInt32      GetRemoteAddr() { return ntohl(fRemoteAddr.sin_addr.s_addr); }
        UInt16      GetRemotePort() { return ntohs(fRemoteAddr.sin_port); }
        //This function is NOT thread safe!
        StrPtrLen*  GetRemoteAddrStr();

    protected:

        void        Set(int inSocket, struct sockaddr_in* remoteaddr);
                            
        enum
        {
            kIPAddrBufSize = 20 
        };

        struct sockaddr_in  fRemoteAddr;
        char fRemoteBuffer[kIPAddrBufSize];
        StrPtrLen fRemoteStr;

        
        friend class TCPListenerSocket;
};
#endif 

