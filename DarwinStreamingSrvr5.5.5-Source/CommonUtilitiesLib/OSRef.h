#ifndef _OSREF_H_
#define _OSREF_H_


#include "StrPtrLen.h"
#include "OSHashTable.h"
#include "OSCond.h"

class OSRefKey;

class OSRefTableUtils
{
    private:

        static UInt32   HashString(StrPtrLen* inString);    

        friend class OSRef;
        friend class OSRefKey;
};

class OSRef
{
    public:

        OSRef() :   fObjectP(NULL), fRefCount(0), fNextHashEntry(NULL)
            {
#if DEBUG
                fInATable = false;
                fSwapCalled = false;
#endif          
            }
        OSRef(const StrPtrLen &inString, void* inObjectP)
                                : fRefCount(0), fNextHashEntry(NULL)
                                    {   Set(inString, inObjectP); }
        ~OSRef() {}
        
        void Set(const StrPtrLen& inString, void* inObjectP)
            { 
#if DEBUG
                fInATable = false;
                fSwapCalled = false;
#endif          
                fString = inString; fObjectP = inObjectP;
                fHashValue = OSRefTableUtils::HashString(&fString);
            }
        
#if DEBUG
        Bool16  IsInTable()     { return fInATable; }
#endif
        void**  GetObjectPtr()  { return &fObjectP; }
        void*   GetObject()     { return fObjectP; }
        UInt32  GetRefCount()   { return fRefCount; }
        StrPtrLen *GetString()  { return &fString; }
    private:
        
        void*   fObjectP;

        StrPtrLen   fString;
        
        UInt32  fRefCount;
#if DEBUG
        Bool16  fInATable;
        Bool16  fSwapCalled;
#endif
        OSCond  fCond;
        
        UInt32              fHashValue;
        OSRef*              fNextHashEntry;
        
        friend class OSRefKey;
        friend class OSHashTable<OSRef, OSRefKey>;
        friend class OSHashTableIter<OSRef, OSRefKey>;
        friend class OSRefTable;

};


class OSRefKey
{
public:

    OSRefKey(StrPtrLen* inStringP)
        :   fStringP(inStringP)
         { fHashValue = OSRefTableUtils::HashString(inStringP); }
            
    ~OSRefKey() {}

    StrPtrLen*  GetString()         { return fStringP; }
    
    
private:

    SInt32      GetHashKey()        { return fHashValue; }

    OSRefKey(OSRef *elem) : fStringP(&elem->fString),
                            fHashValue(elem->fHashValue) {}
                                    
    friend int operator ==(const OSRefKey &key1, const OSRefKey &key2)
    {
        if (key1.fStringP->Equal(*key2.fStringP))
            return true;
        return false;
    }
    
    //data:
    StrPtrLen *fStringP;
    UInt32  fHashValue;

    friend class OSHashTable<OSRef, OSRefKey>;
};

typedef OSHashTable<OSRef, OSRefKey> OSRefHashTable;
typedef OSHashTableIter<OSRef, OSRefKey> OSRefHashTableIter;

class OSRefTable
{
    public:
    
        enum
        {
            kDefaultTableSize = 1193 
        };
   
        OSRefTable(UInt32 tableSize = kDefaultTableSize) : fTable(tableSize), fMutex() {}
        ~OSRefTable() {}
        
        //Allows access to the mutex in case you need to lock the table down between operations
        OSMutex*    GetMutex()      { return &fMutex; }
        OSRefHashTable* GetHashTable() { return &fTable; }
        
        //Registers a Ref in the table. Once the Ref is in, clients may resolve the ref by using its string ID. 
        OS_Error        Register(OSRef* ref);
        
        // RegisterOrResolve
        // If the ID of the input ref is unique, this function is equivalent to Register, and returns NULL.
        // If there is a duplicate ID already in the map, this funcion leave it, resolves it, and returns it.
        OSRef*              RegisterOrResolve(OSRef* inRef);
        
        //This function may block. If several threads have the ref currently, the calling thread will wait until the other threads stop using the ref (by calling Release, below)

        void        UnRegister(OSRef* ref, UInt32 refCount = 0);
        
        Bool16      TryUnRegister(OSRef* ref, UInt32 refCount = 0);
        
        //call release in a timely manner, and be aware of potential deadlocks because you now own a resource being contended over.
        OSRef*      Resolve(StrPtrLen*  inString);
        
        void        Release(OSRef*  inRef);

        void        Swap(OSRef* newRef);
        
        UInt32      GetNumRefsInTable() { UInt64 result =  fTable.GetNumEntries(); Assert(result < kUInt32_Max); return (UInt32) result; }
        
    private:
    
        

        OSRefHashTable  fTable;
        OSMutex         fMutex;
};


class OSRefReleaser
{
    public:

        OSRefReleaser(OSRefTable* inTable, OSRef* inRef) : fOSRefTable(inTable), fOSRef(inRef) {}
        ~OSRefReleaser() { fOSRefTable->Release(fOSRef); }
        
        OSRef*          GetRef() { return fOSRef; }
        
    private:

        OSRefTable*     fOSRefTable;
        OSRef*          fOSRef;
};



#endif
