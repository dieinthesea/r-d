/*
The main methods of this class are: initialising the static character array sWhitespaceAndGreaterThanMask;
getting a copy of the user name; getting a copy of the access file; checking if access is allowed and authorising the request.

The class also contains static member variables sQTAccessFileName and AllocatedName to manage the names of QTAccess files 
and uses a mutex lock sAccessFileMutex to secure access in a multi-threaded environment.
*/

/*
    File:       QTAccessFile.h

    Contains:   This object contains an interface for finding and parsing qtaccess files.                

*/
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

        //AccessAllowed
        //
        // This routine is used to get the Realm to send back to a user and to check if a user has access
        // userName: may be null.
        // accessFileBufPtr:If accessFileBufPtr is NULL or contains a NULL PTR or 0 LEN then false is returned
        // ioRealmNameStr:  ioRealmNameStr and ioRealmNameStr->Ptr may be null. 
        //                  To get a returned ioRealmNameStr value the ioRealmNameStr and ioRealmNameStr->Ptr must be non-NULL
        //                  valid pointers. The ioRealmNameStr.Len should be set to the ioRealmNameStr->Ptr's allocated len.
        // numGroups:       The number of groups in the groupArray. Use GetGroupsArrayCopy to create the groupArray.
        static Bool16 AccessAllowed (   char *userName, char**groupArray, UInt32 numGroups, 
                                        StrPtrLen *accessFileBufPtr,QTSS_ActionFlags inFlags,StrPtrLen* ioRealmNameStr
                                    );

        static void SetAccessFileName(const char *inQTAccessFileName); //makes a copy and stores it
        static char* GetAccessFileName() { return sQTAccessFileName; }; 
                // returns the auth scheme
                static QTSS_AuthScheme FindUsersAndGroupsFilesAndAuthScheme(char* inAccessFilePath, QTSS_ActionFlags inAction, char** outUsersFilePath, char** outGroupsFilePath);
                
        static QTSS_Error AuthorizeRequest(QTSS_StandardRTSP_Params* inParams, Bool16 allowNoAccessFiles, QTSS_ActionFlags noAction, QTSS_ActionFlags authorizeAction);

    private:    
        static char* sQTAccessFileName; // managed by the QTAccess module
        static Bool16 sAllocatedName;
        static OSMutex* sAccessFileMutex;
};

#endif //_QT_ACCESS_FILE_H_

