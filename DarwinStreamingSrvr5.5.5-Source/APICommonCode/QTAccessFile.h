#ifndef _QT_ACCESS_FILE_H_
#define _QT_ACCESS_FILE_H_

#include <stdlib.h>
#include "SafeStdLib.h"
#include "QTSS.h"
#include "StrPtrLen.h"
#include "OSHeaders.h"

class QTAccessFile
{
    public:
        static UInt8 sWhitespaceAndGreaterThanMask[];
        static void Initialize();
        
        static char * GetUserNameCopy(QTSS_UserProfileObject inUserProfile);
        static char*  GetAccessFile_Copy( const char* movieRootDir, const char* dirPath);

        static Bool16 AccessAllowed (   char *userName, char**groupArray, UInt32 numGroups, 
                                        StrPtrLen *accessFileBufPtr,QTSS_ActionFlags inFlags,StrPtrLen* ioRealmNameStr
                                    );

        static void SetAccessFileName(const char *inQTAccessFileName); 
        static char* GetAccessFileName() { return sQTAccessFileName; }; 
               
                static QTSS_AuthScheme FindUsersAndGroupsFilesAndAuthScheme(char* inAccessFilePath, QTSS_ActionFlags inAction, char** outUsersFilePath, char** outGroupsFilePath);
                
        static QTSS_Error AuthorizeRequest(QTSS_StandardRTSP_Params* inParams, Bool16 allowNoAccessFiles, QTSS_ActionFlags noAction, QTSS_ActionFlags authorizeAction);

    private:    
        static char* sQTAccessFileName; 
        static Bool16 sAllocatedName;
        static OSMutex* sAccessFileMutex;
};

#endif //_QT_ACCESS_FILE_H_

