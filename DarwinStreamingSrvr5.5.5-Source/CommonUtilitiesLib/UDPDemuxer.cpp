#include "UDPDemuxer.h"

#include <errno.h>


OS_Error UDPDemuxer::RegisterTask(UInt32 inRemoteAddr, UInt16 inRemotePort,
                                        UDPDemuxerTask *inTaskP)
{
    Assert(NULL != inTaskP);
    OSMutexLocker locker(&fMutex);
    if (this->GetTask(inRemoteAddr, inRemotePort) != NULL)
        return EPERM;
    inTaskP->Set(inRemoteAddr, inRemotePort);
    fHashTable.Add(inTaskP);
    return OS_NoErr;
}

OS_Error UDPDemuxer::UnregisterTask(UInt32 inRemoteAddr, UInt16 inRemotePort,
                                            UDPDemuxerTask *inTaskP)
{
    OSMutexLocker locker(&fMutex);
    UDPDemuxerTask* theTask = this->GetTask(inRemoteAddr, inRemotePort);

    if ((NULL != theTask) && (theTask == inTaskP))
    {
        fHashTable.Remove(theTask);
        return OS_NoErr;
    }
    else
        return EPERM;
}

UDPDemuxerTask* UDPDemuxer::GetTask(UInt32 inRemoteAddr, UInt16 inRemotePort)
{
    UDPDemuxerKey theKey(inRemoteAddr, inRemotePort);
    return fHashTable.Map(&theKey);
}
