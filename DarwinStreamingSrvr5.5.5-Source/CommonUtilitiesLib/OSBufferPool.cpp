#include "OSBufferPool.h"
#include "OSMemory.h"

void*   OSBufferPool::Get()
{
    OSMutexLocker locker(&fMutex);
    if (fQueue.GetLength() == 0)
    {
        fTotNumBuffers++;
        char* theNewBuf = NEW char[fBufSize + sizeof(OSQueueElem)];
        
        (void)new (theNewBuf) OSQueueElem(theNewBuf + sizeof(OSQueueElem));

        return theNewBuf + sizeof(OSQueueElem);
    }
    return fQueue.DeQueue()->GetEnclosingObject();
}

void OSBufferPool::Put(void* inBuffer)
{
    OSMutexLocker locker(&fMutex);
    fQueue.EnQueue((OSQueueElem*)((char*)inBuffer - sizeof(OSQueueElem)));
}
