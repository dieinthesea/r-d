#ifndef __OS_BUFFER_POOL_H__
#define __OS_BUFFER_POOL_H__

#include "OSQueue.h"
#include "OSMutex.h"

class OSBufferPool
{
    public:
    
        OSBufferPool(UInt32 inBufferSize) : fBufSize(inBufferSize), fTotNumBuffers(0) {}
        
        ~OSBufferPool() {}
        

        UInt32  GetTotalNumBuffers() { return fTotNumBuffers; }
        UInt32  GetNumAvailableBuffers() { return fQueue.GetLength(); }
        
        void*   Get();
        
        void    Put(void* inBuffer);
    
    private:
    
        OSMutex fMutex;
        OSQueue fQueue;
        UInt32  fBufSize;
        UInt32  fTotNumBuffers;
};

#endif 
