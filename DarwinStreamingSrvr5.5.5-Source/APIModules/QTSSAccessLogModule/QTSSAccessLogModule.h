/*
A module called AccessLogModule is defined and a function called AccessLogModule_Main is declared. 
This function will be called when the module is loaded.
Also, this code contains the SSS.h header file.
*/
/*
    File:       QTSSAccessLogModule.h

    Contains:   A QTSS API module that runs as an RTSP Post Processor and an RTP
                timeout processor. It generates access log files compatible with
                the Lariat Logging tool.

*/

#ifndef _QTSSACCESSLOGMODULE_H_
#define _QTSSACCESSLOGMODULE_H_

#include "QTSS.h"

extern "C"
{
    EXPORT QTSS_Error QTSSAccessLogModule_Main(void* inPrivateArgs);
}

#endif //_QTSSACCESSLOGMODULE_H_
