/*
This code defines the overloaded functions for the new and delete operators in C++. 
These functions are called automatically when the new and delete operators are used. 
The overloaded functions here use the QTSS_New and QTSS_Delete functions to allocate and free memory, 
which are provided by the QTSS (QuickTime Streaming Server) library.

If the MEMORY_DEBUGGING macro is defined, the new operator passes two additional parameters: the source file name and the line number. 
These parameters are used for debugging purposes so that the source of the code can be traced in the event of memory leaks and other problems.

In addition, for cases where the new[] operator is used, corresponding overloaded functions are provided to allocate and free array memory. 
*/
/*
    File:       OSMemory_Modules.cpp

    Contains:   
                    

*/

#include "OSMemory.h"
#include "QTSS.h"

#if MEMORY_DEBUGGING

void* operator new(size_t s, char* /*inFile*/, int /*inLine*/)
{
    return QTSS_New (FOUR_CHARS_TO_INT('0', '0', '0', '0'), s);
}

void* operator new[](size_t s, char* /*inFile*/, int /*inLine*/)
{
    return QTSS_New (FOUR_CHARS_TO_INT('0', '0', '0', '0'), s);
}

#else

void* operator new (size_t s)
{
    return QTSS_New (FOUR_CHARS_TO_INT('0', '0', '0', '0'), s);
}

void* operator new[](size_t s)
{
    return QTSS_New (FOUR_CHARS_TO_INT('0', '0', '0', '0'), s);
}

void operator delete(void* mem)
{
    QTSS_Delete(mem);
}

void operator delete[](void* mem)
{
    QTSS_Delete(mem);
}


#endif //__OS_MEMORY_H__

