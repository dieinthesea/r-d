/*
A module called SSSHttpFileModule is used to handle HTTP file transfers. 
In it, a TransferType enumeration type is defined,
including three transfer types:
transferHttpFile, transferRefMovieFolder, transferRefMovieFile.
Also, a SSSHttpFileModule_Main function is declared to start the module.
*/
/*
        File:       QTSSHttpFileModule.h

        Contains:   A module for HTTP file transfer of files and for
                    on-the-fly ref movie creation.
                    Uses the Filter module feature of QTSS API.

*/

#ifndef __QTSSHTTPFILEMODULE_H__
#define __QTSSHTTPFILEMODULE_H__

#include "QTSS.h"

enum {
                transferHttpFile            = 1,
                transferRefMovieFolder      = 2,
                transferRefMovieFile        = 3
};
typedef UInt32 TransferType;

extern "C"
{
    QTSS_Error QTSSHttpFileModule_Main(void* inPrivateArgs);
}

#endif // __QTSSHTTPFILEMODULE_H__
