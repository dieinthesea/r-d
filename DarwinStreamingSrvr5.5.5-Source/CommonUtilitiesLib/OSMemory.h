#ifndef __OS_MEMORY_H__
#define __OS_MEMORY_H__

#include "OSHeaders.h"
#include "OSQueue.h"
#include "OSMutex.h"

class OSMemory
{
    public:
    
#if MEMORY_DEBUGGING
        
        static OSQueue* GetTagQueue() { return &sTagQueue; }
        static OSMutex* GetTagQueueMutex() { return &sMutex;    }
        static UInt32   GetAllocatedMemory() { return sAllocatedBytes; }

        static void*    DebugNew(size_t size, char* inFile, int inLine, Bool16 sizeCheck);
        static void     DebugDelete(void *mem);
        static Bool16       MemoryDebuggingTest();
        static void     ValidateMemoryQueue();

        enum
        {
            kMaxFileNameSize = 48
        };
        
        struct TagElem
        {
            OSQueueElem elem;
            char fileName[kMaxFileNameSize];
            int     line;
            UInt32 tagSize; 
            UInt32 totMemory; 
            UInt32 numObjects;
        };
#endif

        static void*    New(size_t inSize);
        static void     Delete(void* inMemory);
        
        static void SetMemoryError(SInt32 inErr);
        
#if MEMORY_DEBUGGING
    private:
            
        struct MemoryDebugging
        {
            OSQueueElem elem;
            TagElem* tagElem;
            UInt32 size;
        };
        static OSQueue sMemoryQueue;
        static OSQueue sTagQueue;
        static UInt32  sAllocatedBytes;
        static OSMutex sMutex;
        
#endif
};


#if MEMORY_DEBUGGING

#ifdef  NEW
#error Conflicting Macro "NEW"
#endif

#define NEW new (__FILE__, __LINE__)

#else

#ifdef  NEW
#error Conflicting Macro "NEW"
#endif

#define NEW new

#endif


inline void* operator new(size_t, void* ptr) { return ptr;}

#if MEMORY_DEBUGGING


void* operator new(size_t s, char* inFile, int inLine);
void* operator new[](size_t s, char* inFile, int inLine);

#endif

void* operator new (size_t s);
void* operator new[](size_t s);

void operator delete(void* mem);
void operator delete[](void* mem);


#endif //
