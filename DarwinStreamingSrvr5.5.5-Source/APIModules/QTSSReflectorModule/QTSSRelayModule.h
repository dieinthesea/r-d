
/*
    File:       QTSSReflectorModule.h

    Contains:   QTSS API module 

*/

#ifndef _QTSSRELAYMODULE_H_
#define _QTSSRELAYMODULE_H_

#include "QTSS.h"

extern "C"
{
    EXPORT QTSS_Error QTSSRelayModule_Main(void* inPrivateArgs);
}

#endif //_QTSSRELAYMODULE_H_
