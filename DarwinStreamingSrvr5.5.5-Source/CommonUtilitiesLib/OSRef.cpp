#include "OSRef.h"

#include <errno.h>

UInt32  OSRefTableUtils::HashString(StrPtrLen* inString)
{
    Assert(inString != NULL);
    Assert(inString->Ptr != NULL);
    Assert(inString->Len > 0);
    
    UInt8* theData = (UInt8*)inString->Ptr;
    
    UInt32 quarterLen = inString->Len >> 2;
    return (inString->Len * (theData[0] + theData[quarterLen] +
            theData[quarterLen * 2] + theData[quarterLen * 3] +
            theData[inString->Len - 1]));
}

OS_Error OSRefTable::Register(OSRef* inRef)
{
    Assert(inRef != NULL);
#if DEBUG
    Assert(!inRef->fInATable);
#endif
    Assert(inRef->fRefCount == 0);
    
    OSMutexLocker locker(&fMutex);

    OSRefKey key(&inRef->fString);
    OSRef* duplicateRef = fTable.Map(&key);
    if (duplicateRef != NULL)
        return EPERM;
        
#if DEBUG
    inRef->fInATable = true;
#endif
    fTable.Add(inRef);
    return OS_NoErr;
}


OSRef* OSRefTable::RegisterOrResolve(OSRef* inRef)
{
    Assert(inRef != NULL);
#if DEBUG
    Assert(!inRef->fInATable);
#endif
    Assert(inRef->fRefCount == 0);
    
    OSMutexLocker locker(&fMutex);

    OSRef* duplicateRef = this->Resolve(&inRef->fString);
    if (duplicateRef != NULL)
        return duplicateRef;

#if DEBUG
    inRef->fInATable = true;
#endif
    fTable.Add(inRef);
    return NULL;
}

void OSRefTable::UnRegister(OSRef* ref, UInt32 refCount)
{
    Assert(ref != NULL);
    OSMutexLocker locker(&fMutex);

    //make sure that no one else is using the object
    while (ref->fRefCount > refCount)
        ref->fCond.Wait(&fMutex);
    
#if DEBUG
    OSRefKey key(&ref->fString);
    if (ref->fInATable)
        Assert(fTable.Map(&key) != NULL);
    ref->fInATable = false;
#endif
   
    fTable.Remove(ref);
}

Bool16 OSRefTable::TryUnRegister(OSRef* ref, UInt32 refCount)
{
    OSMutexLocker locker(&fMutex);
    if (ref->fRefCount > refCount)
        return false;

    this->UnRegister(ref, refCount);
    return true;
}


OSRef*  OSRefTable::Resolve(StrPtrLen* inUniqueID)
{
    Assert(inUniqueID != NULL);
    OSRefKey key(inUniqueID);

    OSMutexLocker locker(&fMutex);
    OSRef* ref = fTable.Map(&key);
    if (ref != NULL)
    {
        ref->fRefCount++;
        Assert(ref->fRefCount > 0);
    }
    return ref;
}

void    OSRefTable::Release(OSRef* ref)
{
    Assert(ref != NULL);
    OSMutexLocker locker(&fMutex);
    ref->fRefCount--;

    Assert( ref->fRefCount < 1048576L );
    //make sure to wakeup anyone who may be waiting for this resource to be released
    ref->fCond.Signal();
}

void    OSRefTable::Swap(OSRef* newRef)
{
    Assert(newRef != NULL);
    OSMutexLocker locker(&fMutex);
    
    OSRefKey key(&newRef->fString);
    OSRef* oldRef = fTable.Map(&key);
    if (oldRef != NULL)
    {
        fTable.Remove(oldRef);
        fTable.Add(newRef);
#if DEBUG
        newRef->fInATable = true;
        oldRef->fInATable = false;
        oldRef->fSwapCalled = true;
#endif
    }
    else
        Assert(0);
}

