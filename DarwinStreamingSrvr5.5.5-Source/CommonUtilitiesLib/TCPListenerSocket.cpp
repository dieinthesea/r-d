#ifndef __Win32__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "QTSSModuleUtils.h"

#endif

#include <errno.h>

#include "TCPListenerSocket.h"
#include "Task.h"



OS_Error TCPListenerSocket::Listen(UInt32 queueLength)
{
    if (fFileDesc == EventContext::kInvalidFileDesc)
        return EBADF;
        
    int err = ::listen(fFileDesc, queueLength);
    if (err != 0)
        return (OS_Error)OSThread::GetErrno();
    return OS_NoErr;
}

OS_Error TCPListenerSocket::Initialize(UInt32 addr, UInt16 port)
{
    OS_Error err = this->TCPSocket::Open();
    if (0 == err) do
    {   
        
#ifndef __Win32__
        
        this->ReuseAddr();
#endif
        err = this->Bind(addr, port);
        if (err != 0) break; 

        this->SetSocketRcvBufSize(96 * 1024);       
        err = this->Listen(kListenQueueLength);
        AssertV(err == 0, OSThread::GetErrno()); 
        if (err != 0) break;
        
    } while (false);
    
    return err;
}

void TCPListenerSocket::ProcessEvent(int /*eventBits*/)
{
  
    
    struct sockaddr_in addr;
#if __Win32__ || __osf__ || __sgi__ || __hpux__	
    int size = sizeof(addr);
#else
    socklen_t size = sizeof(addr);
#endif
    Task* theTask = NULL;
    TCPSocket* theSocket = NULL;
    
	int osSocket = accept(fFileDesc, (struct sockaddr*)&addr, &size);

	if (osSocket == -1)
	{
        int acceptError = OSThread::GetErrno();
        if (acceptError == EAGAIN)
        { 

            this->RequestEvent(EV_RE);
            return;
        }

	if (acceptError == EMFILE || acceptError == ENFILE)
        {           
#ifndef __Win32__

			QTSSModuleUtils::LogErrorStr(qtssFatalVerbosity,  "Out of File Descriptors. Set max connections lower and check for competing usage from other processes. Exiting.");
#endif

			exit (EXIT_FAILURE);	
        }
        else
        {   
            char errStr[256];
            errStr[sizeof(errStr) -1] = 0;
            qtss_snprintf(errStr, sizeof(errStr) -1, "accept error = %d '%s' on socket. Clean up and continue.", acceptError, strerror(acceptError)); 
            WarnV( (acceptError == 0), errStr);
            
            theTask = this->GetSessionTask(&theSocket);
            if (theTask == NULL)
            {   
                close(osSocket);
            }
            else
            {  
                theTask->Signal(Task::kKillEvent); // just clean up the task
            }
            
            if (theSocket)
                theSocket->fState &= ~kConnected; // turn off connected state
            
            return;
        }
	}
	
    theTask = this->GetSessionTask(&theSocket);
    if (theTask == NULL)
    {    
        close(osSocket);
        if (theSocket)
            theSocket->fState &= ~kConnected; // turn off connected state
    }
    else
    {   
        Assert(osSocket != EventContext::kInvalidFileDesc);

        int one = 1;
        int err = ::setsockopt(osSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof(int));
        AssertV(err == 0, OSThread::GetErrno());
        
        err = ::setsockopt(osSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&one, sizeof(int));
        AssertV(err == 0, OSThread::GetErrno());
    
        int sndBufSize = 96L * 1024L;
        err = ::setsockopt(osSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sndBufSize, sizeof(int));
        AssertV(err == 0, OSThread::GetErrno());
    
        //setup the socket. When there is data on the socket, theTask will get an kReadEvent event
        theSocket->Set(osSocket, &addr);
        theSocket->InitNonBlocking(osSocket);
        theSocket->SetTask(theTask);
        theSocket->RequestEvent(EV_RE);
    }
    


    if (fSleepBetweenAccepts)
    { 	
        
        this->SetIdleTimer(kTimeBetweenAcceptsInMsec); //sleep 1 second
    }
    else
    { 	
        // sleep until another client want to connect
        this->RequestEvent(EV_RE);
    }

    fOutOfDescriptors = false; // always false if it isn't properly handle 
}

SInt64 TCPListenerSocket::Run()
{
    EventFlags events = this->GetEvents();
 
    if (events & Task::kKillEvent)
        return -1;

    //This function will get called when we have run out of file descriptors.
    //All we need to do is check the listen queue to see if the situation has cleared up.
    (void)this->GetEvents();
    this->ProcessEvent(Task::kReadEvent);
    return 0;
}
