#ifndef __OS_ARRAY_OBJECT_DELETER_H__
#define __OS_ARRAY_OBJECT_DELETER_H__

#include "MyAssert.h"

template <class T>
class OSArrayObjectDeleter
{
    public:
        OSArrayObjectDeleter(T* victim) : fT(victim)  {}
        ~OSArrayObjectDeleter() { delete [] fT; }
        
        void ClearObject() { fT = NULL; }

        void SetObject(T* victim) 
        {
            Assert(fT == NULL);
            fT = victim; 
        }
        T* GetObject() { return fT; }
        
        operator T*() { return fT; }
    
    private:
    
        T* fT;
};


template <class T>
class OSPtrDeleter
{
    public:
        OSPtrDeleter(T* victim) : fT(victim)  {}
        ~OSPtrDeleter() { delete fT; }
        
        void ClearObject() { fT = NULL; }

        void SetObject(T* victim) 
        {   Assert(fT == NULL);
            fT = victim; 
        }
            
    private:
    
        T* fT;
};


typedef OSArrayObjectDeleter<char*> OSCharPointerArrayDeleter;
typedef OSArrayObjectDeleter<char> OSCharArrayDeleter;

#endif 
