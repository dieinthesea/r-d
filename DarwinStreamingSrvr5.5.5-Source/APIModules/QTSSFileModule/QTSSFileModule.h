/*
    File:       QTSSFileModule.h

    Contains:   Content source module that uses the QTFileLib to serve Hinted QuickTime
                files to clients. 
                    

*/

#ifndef _RTPFILEMODULE_H_
#define _RTPFILEMODULE_H_

#include "QTSS.h"

extern "C"
{
    EXPORT QTSS_Error QTSSFileModule_Main(void* inPrivateArgs);
}

#endif //_RTPFILEMODULE_H_
