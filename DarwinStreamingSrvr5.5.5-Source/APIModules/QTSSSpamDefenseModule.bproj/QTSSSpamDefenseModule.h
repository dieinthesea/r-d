/*
    File:       QTSSSpamDefanseModule.h

    Contains:   Protects the server against denial-of-service attacks by
                only allowing X number of RTSP connections from a certain
                IP address

*/


#ifndef __QTSSSPAMDEFENSEMODULE_H__
#define __QTSSSPAMDEFENSEMODULE_H__

#include "QTSS.h"

extern "C"
{
    EXPORT QTSS_Error QTSSSpamDefenseModule_Main(void* inPrivateArgs);
}

#endif // __QTSSSPAMDEFENSEMODULE_H__
