#include "MakeDir.h"

#include "PathDelimiter.h"


#if (! __MACOS__)
    #include <sys/file.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #ifndef __solaris__
        #include <sys/sysctl.h>
    #endif
    #include <sys/time.h>
#else
    #include "BogusDefs.h"
#endif

#include <string.h>
#include <stdlib.h>
#include "SafeStdLib.h"


int MakeDir(const char* inPath, int mode)
{
    struct stat theStatBuffer;
    if (stat(inPath, &theStatBuffer) == -1)
    {
        //create a directory
        if (mkdir(inPath, mode) == -1)
            return  -1; //€- (QTSS_ErrorCode)OSThread::GetErrno();
    }
    else if (!S_ISDIR(theStatBuffer.st_mode))
        return  -1; //€- QTSS_FileExists;

    //directory exists
    return  0; //€- QTSS_NoErr;
}

int RecursiveMakeDir(const char* inPath, int mode)
{
    //PL_ASSERT(inPath != NULL);
    char    pathCopy[256];
    char*   thePathTraverser = pathCopy;
    int     theErr;
    char    oldChar;    
    
    
    if ( strlen( inPath ) > 255 )
        return -1;
    
    strcpy( pathCopy, inPath );
    
    if (*thePathTraverser == kPathDelimiterChar )
        thePathTraverser++;
        
    while (*thePathTraverser != '\0')
    {
        if (*thePathTraverser == kPathDelimiterChar)
        {
            //find a filename divider and complete filename, see if this partial path exists.
            
            oldChar = *thePathTraverser;
            *thePathTraverser = '\0';
            theErr = MakeDir(pathCopy, mode);
            //there is a directory here. Just continue in our traversal
            *thePathTraverser = oldChar;
            
            if (theErr)
                return theErr;
        }
        
        thePathTraverser++;
    }
    
    //need to create the last directory in the path
    return MakeDir(inPath, mode);
}
