/*
    File:       QTSSReflectorModule.h

    Contains:   QTSS API module 
                      
*/

#ifndef _QTSSREFLECTORMODULE_H_
#define _QTSSREFLECTORMODULE_H_

#include "QTSS.h"

extern "C"
{
    EXPORT QTSS_Error QTSSReflectorModule_Main(void* inPrivateArgs);
}

#endif //_QTSSREFLECTORMODULE_H_
