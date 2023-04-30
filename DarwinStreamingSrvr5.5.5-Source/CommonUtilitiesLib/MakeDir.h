#ifndef __makedir__
#define __makedir__

#if (! __MACOS__)
    #include <sys/file.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #ifndef __solaris__ || __hpux__
        #include <sys/sysctl.h>
    #endif
    #include <sys/time.h>
#else
    #include "BogusDefs.h"
#endif

#ifndef S_IRWXU
    #define S_IRWXU 0
#endif


    #ifdef __cplusplus
    extern "C" {
    #endif


    int     MakeDir( const char* path, int mode );
    int     RecursiveMakeDir( const char*inPath, int mode);

    #ifdef __cplusplus
    }   
    #endif


#endif
