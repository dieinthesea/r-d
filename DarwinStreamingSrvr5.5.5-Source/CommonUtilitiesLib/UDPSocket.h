#ifndef __UDPSOCKET_H__
#define __UDPSOCKET_H__

#ifndef __Win32__
#include <sys/socket.h>
#include <sys/uio.h>
#endif

#include "Socket.h"
#include "UDPDemuxer.h"


class   UDPSocket : public Socket
{
    public:

        enum
        {
            kWantsDemuxer = 0x0100 //UInt32
        };
    
        UDPSocket(Task* inTask, UInt32 inSocketType);
        virtual ~UDPSocket() { if (fDemuxer != NULL) delete fDemuxer; }

        //Open
        OS_Error    Open() { return Socket::Open(SOCK_DGRAM); }

        OS_Error    JoinMulticast(UInt32 inRemoteAddr);
        OS_Error    LeaveMulticast(UInt32 inRemoteAddr);
        OS_Error    SetTtl(UInt16 timeToLive);
        OS_Error    SetMulticastInterface(UInt32 inLocalAddr);

        OS_Error        SendTo(UInt32 inRemoteAddr, UInt16 inRemotePort,
                                    void* inBuffer, UInt32 inLength);
                        
        OS_Error        RecvFrom(UInt32* outRemoteAddr, UInt16* outRemotePort,
                                        void* ioBuffer, UInt32 inBufLen, UInt32* outRecvLen);

        UDPDemuxer*         GetDemuxer()    { return fDemuxer; }
        
    private:
    
        UDPDemuxer* fDemuxer;
        struct sockaddr_in  fMsgAddr;
};
#endif 

