#ifndef __Win32__
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#include <errno.h>

#include "TCPSocket.h"
#include "SocketUtils.h"
#include "OS.h"

#ifdef USE_NETLOG
#include <netlog.h>
#endif

void TCPSocket::SnarfSocket( TCPSocket & fromSocket )
{
    // take the connection away from the other socket and use it as our own.
    Assert(fFileDesc == EventContext::kInvalidFileDesc);
    this->Set( fromSocket.fFileDesc, &fromSocket.fRemoteAddr );
    // clear the old socket
    struct  sockaddr_in  remoteaddr;
    
    ::memset( &remoteaddr, 0, sizeof( remoteaddr ) );

    fromSocket.Set( EventContext::kInvalidFileDesc, &remoteaddr );
    // get the event context too
    this->SnarfEventContext( fromSocket );

}

void TCPSocket::Set(int inSocket, struct sockaddr_in* remoteaddr)
{
    fRemoteAddr = *remoteaddr;
    fFileDesc = inSocket;
    
    if ( inSocket != EventContext::kInvalidFileDesc ) 
    {
       
#if __Win32__ || __osf__ || __sgi__ || __hpux__	
        int len = sizeof(fLocalAddr);
#else
        socklen_t len = sizeof(fLocalAddr);
#endif
        int err = ::getsockname(fFileDesc, (struct sockaddr*)&fLocalAddr, &len);
        AssertV(err == 0, OSThread::GetErrno());
        fState |= kBound;
        fState |= kConnected;
    }
    else
        fState = 0;
}

StrPtrLen*  TCPSocket::GetRemoteAddrStr()
{
    if (fRemoteStr.Len == kIPAddrBufSize)
        SocketUtils::ConvertAddrToString(fRemoteAddr.sin_addr, &fRemoteStr);
    return &fRemoteStr;
}

OS_Error  TCPSocket::Connect(UInt32 inRemoteAddr, UInt16 inRemotePort)
{
    ::memset(&fRemoteAddr, 0, sizeof(fRemoteAddr));
    fRemoteAddr.sin_family = AF_INET;       
    fRemoteAddr.sin_port = htons(inRemotePort); 
    fRemoteAddr.sin_addr.s_addr = htonl(inRemoteAddr);

    int err = ::connect(fFileDesc, (sockaddr *)&fRemoteAddr, sizeof(fRemoteAddr));
    fState |= kConnected;
    
    if (err == -1)
    {
        fRemoteAddr.sin_port = 0;
        fRemoteAddr.sin_addr.s_addr = 0;
        return (OS_Error)OSThread::GetErrno();
    }
    
    return OS_NoErr;

}

