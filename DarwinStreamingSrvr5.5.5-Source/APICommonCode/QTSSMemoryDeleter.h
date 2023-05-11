#ifndef __QTSS_MEMORY_DELETER_H__
#define __QTSS_MEMORY_DELETER_H__

#include "MyAssert.h"
#include "QTSS.h"

template <class T>
class QTSSMemoryDeleter
{
    public:
        QTSSMemoryDeleter(T* victim) : fT(victim)  {}
        ~QTSSMemoryDeleter() { QTSS_Delete(fT); }
        
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

typedef QTSSMemoryDeleter<char> QTSSCharArrayDeleter;

#endif //__QTSS_MEMORY_DELETER_H__


