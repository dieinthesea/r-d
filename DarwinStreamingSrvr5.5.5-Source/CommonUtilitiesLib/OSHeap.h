#ifndef _OSHEAP_H_
#define _OSHEAP_H_

#define _OSHEAP_TESTING_ 0

#include "OSCond.h"

class OSHeapElem;

class OSHeap
{
    public:
    
        enum
        {
            kDefaultStartSize = 1024 //UInt32
        };
        
        OSHeap(UInt32 inStartSize = kDefaultStartSize);
        ~OSHeap() { if (fHeap != NULL) delete fHeap; }
        
        UInt32      CurrentHeapSize() { return fFreeIndex - 1; }
        OSHeapElem* PeekMin() { if (CurrentHeapSize() > 0) return fHeap[1]; return NULL; }
        
        void            Insert(OSHeapElem*  inElem);
        OSHeapElem*     ExtractMin() { return Extract(1); }
        //removes specified element from the heap
        OSHeapElem*     Remove(OSHeapElem* elem);
        
#if _OSHEAP_TESTING_
        static Bool16       Test();
#endif
    
    private:
    
        OSHeapElem*     Extract(UInt32 index);
    
#if _OSHEAP_TESTING_
        void            SanityCheck(UInt32 root);
#endif
    
        OSHeapElem**    fHeap;
        UInt32          fFreeIndex;
        UInt32          fArraySize;
};

class OSHeapElem
{
    public:
        OSHeapElem(void* enclosingObject = NULL)
            : fValue(0), fEnclosingObject(enclosingObject), fCurrentHeap(NULL) {}
        ~OSHeapElem() {}
        
        void    SetValue(SInt64 newValue) { fValue = newValue; }
        SInt64  GetValue()              { return fValue; }
        void*   GetEnclosingObject()    { return fEnclosingObject; }
		void	SetEnclosingObject(void* obj) { fEnclosingObject = obj; }
        Bool16  IsMemberOfAnyHeap()     { return fCurrentHeap != NULL; }
        
    private:
    
        SInt64  fValue;
        void* fEnclosingObject;
        OSHeap* fCurrentHeap;
        
        friend class OSHeap;
};
#endif 
