#include "UDPSocketPool.h"

UDPSocketPair* UDPSocketPool::GetUDPSocketPair(UInt32 inIPAddr, UInt16 inPort,
                                                UInt32 inSrcIPAddr, UInt16 inSrcPort)
{
    OSMutexLocker locker(&fMutex);
    if ((inSrcIPAddr != 0) || (inSrcPort != 0))
    {
        for (OSQueueIter qIter(&fUDPQueue); !qIter.IsDone(); qIter.Next())
        {
            
            UDPSocketPair* theElem = (UDPSocketPair*)qIter.GetCurrent()->GetEnclosingObject();
            if ((theElem->fSocketA->GetLocalAddr() == inIPAddr) &&
                ((inPort == 0) || (theElem->fSocketA->GetLocalPort() == inPort)))
            {
                
                if ((theElem->fSocketB->GetDemuxer() == NULL) ||
                    ((!theElem->fSocketB->GetDemuxer()->AddrInMap(0, 0)) &&
                    (!theElem->fSocketB->GetDemuxer()->AddrInMap(inSrcIPAddr, inSrcPort))))
                {
                    theElem->fRefCount++;
                    return theElem;
                }
               
                else if (inPort != 0)
                    return NULL;
            }
        }
    }

    return this->CreateUDPSocketPair(inIPAddr, inPort);
}

void UDPSocketPool::ReleaseUDPSocketPair(UDPSocketPair* inPair)
{
    OSMutexLocker locker(&fMutex);
    inPair->fRefCount--;
    if (inPair->fRefCount == 0)
    {
        fUDPQueue.Remove(&inPair->fElem);
        this->DestructUDPSocketPair(inPair);
    }
}

UDPSocketPair*  UDPSocketPool::CreateUDPSocketPair(UInt32 inAddr, UInt16 inPort)
{
    OSMutexLocker locker(&fMutex);
    UDPSocketPair* theElem = NULL;
    Bool16 foundPair = false;
    UInt16 curPort = kLowestUDPPort;
    UInt16 stopPort = kHighestUDPPort -1; 
    UInt16 socketBPort = kLowestUDPPort + 1;

     if (inPort != 0)
        curPort = inPort;
     if (inPort != 0)
        stopPort = inPort;
        

    while ((!foundPair) && (curPort < kHighestUDPPort))
    {
        socketBPort = curPort +1; 
        
        theElem = ConstructUDPSocketPair();
        Assert(theElem != NULL);
        if (theElem->fSocketA->Open() != OS_NoErr)
        {
            this->DestructUDPSocketPair(theElem);
            return NULL;
        }
        if (theElem->fSocketB->Open() != OS_NoErr)
        {
            this->DestructUDPSocketPair(theElem);
            return NULL;
        }
            
        this->SetUDPSocketOptions(theElem);
        
        OS_Error theErr = theElem->fSocketA->Bind(inAddr, curPort);
        if (theErr == OS_NoErr)
        {  
            theErr = theElem->fSocketB->Bind(inAddr, socketBPort);
            if (theErr == OS_NoErr)
            {   
                foundPair = true;
                fUDPQueue.EnQueue(&theElem->fElem);
                theElem->fRefCount++;
                return theElem;
            }
           
         }
        
        if (inPort != 0)
            break;
            
        if (curPort >= stopPort) 
            break;
            
        curPort += 2; 
        
        this->DestructUDPSocketPair(theElem); //a bind failure
        theElem = NULL;
    }
   
    if (theElem != NULL)
        this->DestructUDPSocketPair(theElem); 
       
    return NULL;
}
