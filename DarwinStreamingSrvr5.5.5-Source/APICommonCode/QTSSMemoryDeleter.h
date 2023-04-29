/*
    A template class called QTSSMemoryDeleter is defined to delete heap objects at the end of their life cycle. The class has the following methods:

    Constructor: initialises the fT member to the passed pointer parameter.
    Destructor: calls the QTSS_Delete method to delete the heap object pointed to by fT at the end of the object's life cycle.
    ClearObject method: sets the fT member to NULL to prevent the object from being deleted at the end of its life cycle.
    SetObject method: sets the fT member to the passed pointer parameter, but requires fT to be NULL to ensure that objects are not accidentally deleted.
    GetObject method: returns a pointer to the object pointed to by the fT member.
    Type conversion operator: allows the QTSSMemoryDeleter object to be converted to a pointer of type T.
    In addition, the code defines a QTSSCharArrayDeleter type for deleting heap objects of type char.
  
*/
/*
    File:       QTSSMemoryDeleter.h

    Contains:   Auto object for deleting memory allocated by QTSS API callbacks,
                such as QTSS_GetValueAsString

*/

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


