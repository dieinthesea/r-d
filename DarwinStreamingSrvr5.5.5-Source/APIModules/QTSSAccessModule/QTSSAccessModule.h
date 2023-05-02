/*
Header file, 
which mainly serves to declare the main function QTSSAccessModule_Main of the QTSSAccessModule module and references the QTSS.h header file
*/
/* 
    File:       QTSSAccessModule.h

    Contains:   Module that handles authentication and authorization independent of the file system
                    
*/

#ifndef _QTSSACCESSMODULE_H_
#define _QTSSACCESSMODULE_H_

#include "QTSS.h"

extern "C"
{
    EXPORT QTSS_Error QTSSAccessModule_Main(void* inPrivateArgs);
}

#endif //_QTSSACCESSMODULE_H_
