#include <string.h>

#include "OSHeap.h"
#include "OSMemory.h"

OSHeap::OSHeap(UInt32 inStartSize)
: fFreeIndex(1)
{
    if (inStartSize < 2)
        fArraySize = 2;
    else
        fArraySize = inStartSize;
        
    fHeap = NEW OSHeapElem*[fArraySize];
}

void OSHeap::Insert(OSHeapElem* inElem)
{
    Assert(inElem != NULL);
    
    if ((fHeap == NULL) || (fFreeIndex == fArraySize))
    {
        fArraySize *= 2;
        OSHeapElem** tempArray = NEW OSHeapElem*[fArraySize];
        if ((fHeap != NULL) && (fFreeIndex > 1))
            memcpy(tempArray, fHeap, sizeof(OSHeapElem*) * fFreeIndex);
            
        delete [] fHeap;
        fHeap = tempArray;
    }
    
    Assert(fHeap != NULL);
    Assert(inElem->fCurrentHeap == NULL);
    Assert(fArraySize > fFreeIndex);
    
#if _OSHEAP_TESTING_
    SanityCheck(1);
#endif

    //insert the element into the last leaf of the tree
    fHeap[fFreeIndex] = inElem;
    
    //bubble the new element up to its proper place in the heap
    
    //start at the last leaf of the tree
    UInt32 swapPos = fFreeIndex;
    while (swapPos > 1)
    {
        //bubble this new element to its proper place in the tree
        UInt32 nextSwapPos = swapPos >> 1;
        
        if (fHeap[swapPos]->fValue < fHeap[nextSwapPos]->fValue)
        {
            OSHeapElem* temp = fHeap[swapPos];
            fHeap[swapPos] = fHeap[nextSwapPos];
            fHeap[nextSwapPos] = temp;
            swapPos = nextSwapPos;
        }
        else
            break;
    }
    inElem->fCurrentHeap = this;
    fFreeIndex++;
}


OSHeapElem* OSHeap::Extract(UInt32 inIndex)
{
    if ((fHeap == NULL) || (fFreeIndex <= inIndex))
        return NULL;
        
#if _OSHEAP_TESTING_
    SanityCheck(1);
#endif
    

    OSHeapElem* victim = fHeap[inIndex];
    Assert(victim->fCurrentHeap == this);
    victim->fCurrentHeap = NULL;
    
    fHeap[inIndex] = fHeap[fFreeIndex - 1];
    fFreeIndex--;
    
    UInt32 parent = inIndex;
    while (parent < fFreeIndex)
    {
        UInt32 greatest = parent;
        UInt32 leftChild = parent * 2;
        if ((leftChild < fFreeIndex) && (fHeap[leftChild]->fValue < fHeap[parent]->fValue))
            greatest = leftChild;

        UInt32 rightChild = (parent * 2) + 1;
        if ((rightChild < fFreeIndex) && (fHeap[rightChild]->fValue < fHeap[greatest]->fValue))
            greatest = rightChild;
         
        if (greatest == parent)
            break;
            
        OSHeapElem* temp = fHeap[parent];
        fHeap[parent] = fHeap[greatest];
        fHeap[greatest] = temp;
        
        parent = greatest;
    }
    
    return victim;
}

OSHeapElem* OSHeap::Remove(OSHeapElem* elem)
{
    if ((fHeap == NULL) || (fFreeIndex == 1))
        return NULL;
        
#if _OSHEAP_TESTING_
    SanityCheck(1);
#endif

    //first attempt to locate this element in the heap
    UInt32 theIndex = 1;
    for ( ; theIndex < fFreeIndex; theIndex++)
        if (elem == fHeap[theIndex])
            break;
            
    if (theIndex == fFreeIndex)
        return NULL;
        
    return Extract(theIndex);
}


#if _OSHEAP_TESTING_

void OSHeap::SanityCheck(UInt32 root)
{
    if (root < fFreeIndex)
    {
        if ((root * 2) < fFreeIndex)
        {
            Assert(fHeap[root]->fValue <= fHeap[root * 2]->fValue);
            SanityCheck(root * 2);
        }
        if (((root * 2) + 1) < fFreeIndex)
        {
            Assert(fHeap[root]->fValue <= fHeap[(root * 2) + 1]->fValue);
            SanityCheck((root * 2) + 1);
        }
    }
}


Bool16 OSHeap::Test()
{
    OSHeap victim(2);
    OSHeapElem elem1;
    OSHeapElem elem2;
    OSHeapElem elem3;
    OSHeapElem elem4;
    OSHeapElem elem5;
    OSHeapElem elem6;
    OSHeapElem elem7;
    OSHeapElem elem8;
    OSHeapElem elem9;

    OSHeapElem* max = victim.ExtractMin();
    if (max != NULL)
        return false;
        
    elem1.SetValue(100);
    victim.Insert(&elem1);
    
    max = victim.ExtractMin();
    if (max != &elem1)
        return false;
    max = victim.ExtractMin();
    if (max != NULL)
        return false;
    
    elem1.SetValue(100);
    elem2.SetValue(80);
    
    victim.Insert(&elem1);
    victim.Insert(&elem2);
    
    max = victim.ExtractMin();
    if (max != &elem2)
        return false;
    max = victim.ExtractMin();
    if (max != &elem1)
        return false;
    max = victim.ExtractMin();
    if (max != NULL)
        return false;
    
    victim.Insert(&elem2);
    victim.Insert(&elem1);

    max = victim.ExtractMin();
    if (max != &elem2)
        return false;
    max = victim.ExtractMin();
    if (max != &elem1)
        return false;
        
    elem3.SetValue(70);
    elem4.SetValue(60);

    victim.Insert(&elem3);
    victim.Insert(&elem1);
    victim.Insert(&elem2);
    victim.Insert(&elem4);
    
    max = victim.ExtractMin();
    if (max != &elem4)
        return false;
    max = victim.ExtractMin();
    if (max != &elem3)
        return false;
    max = victim.ExtractMin();
    if (max != &elem2)
        return false;
    max = victim.ExtractMin();
    if (max != &elem1)
        return false;

    elem5.SetValue(50);
    elem6.SetValue(40);
    elem7.SetValue(30);
    elem8.SetValue(20);
    elem9.SetValue(10);

    victim.Insert(&elem5);
    victim.Insert(&elem3);
    victim.Insert(&elem1);
    
    max = victim.ExtractMin();
    if (max != &elem5)
        return false;
    
    victim.Insert(&elem4);
    victim.Insert(&elem2);

    max = victim.ExtractMin();
    if (max != &elem4)
        return false;
    max = victim.ExtractMin();
    if (max != &elem3)
        return false;
    
    victim.Insert(&elem2);

    max = victim.ExtractMin();
    if (max != &elem2)
        return false;

    victim.Insert(&elem2);
    victim.Insert(&elem6);

    max = victim.ExtractMin();
    if (max != &elem6)
        return false;

    victim.Insert(&elem6);
    victim.Insert(&elem3);
    victim.Insert(&elem4);
    victim.Insert(&elem5);

    max = victim.ExtractMin();
    if (max != &elem6)
        return false;
    max = victim.ExtractMin();
    if (max != &elem5)
        return false;

    victim.Insert(&elem8);
    max = victim.ExtractMin();
    if (max != &elem8)
        return false;
    max = victim.ExtractMin();
    if (max != &elem4)
        return false;
        
    victim.Insert(&elem5);
    victim.Insert(&elem4);
    victim.Insert(&elem9);
    victim.Insert(&elem7);
    victim.Insert(&elem8);
    victim.Insert(&elem6);

    max = victim.ExtractMin();
    if (max != &elem9)
        return false;
    max = victim.ExtractMin();
    if (max != &elem8)
        return false;
    max = victim.ExtractMin();
    if (max != &elem7)
        return false;
    max = victim.ExtractMin();
    if (max != &elem6)
        return false;
    max = victim.ExtractMin();
    if (max != &elem5)
        return false;
    max = victim.ExtractMin();
    if (max != &elem4)
        return false;
    max = victim.ExtractMin();
    if (max != &elem3)
        return false;
    max = victim.ExtractMin();
    if (max != &elem2)
        return false;
    max = victim.ExtractMin();
    if (max != &elem2)
        return false;
    max = victim.ExtractMin();
    if (max != &elem1)
        return false;
    max = victim.ExtractMin();
    if (max != NULL)
        return false;
        
    victim.Insert(&elem1);
    victim.Insert(&elem2);
    victim.Insert(&elem3);
    victim.Insert(&elem4);
    victim.Insert(&elem5);
    victim.Insert(&elem6);
    victim.Insert(&elem7);
    victim.Insert(&elem8);
    victim.Insert(&elem9);
    
    max = victim.Remove(&elem7);
    if (max != &elem7)
        return false;
    max = victim.Remove(&elem9);
    if (max != &elem9)
        return false;
    max = victim.ExtractMin();
    if (max != &elem8)
        return false;
    max = victim.Remove(&elem2);
    if (max != &elem2)
        return false;
    max = victim.Remove(&elem2);
    if (max != NULL)
        return false;
    max = victim.Remove(&elem8);
    if (max != NULL)
        return false;
    max = victim.Remove(&elem5);
    if (max != &elem5)
        return false;
    max = victim.Remove(&elem6);
    if (max != &elem6)
        return false;
    max = victim.Remove(&elem1);
    if (max != &elem1)
        return false;
    max = victim.ExtractMin();
    if (max != &elem4)
        return false;
    max = victim.Remove(&elem1);
    if (max != NULL)
        return false;
    
    return true;
}
#endif
