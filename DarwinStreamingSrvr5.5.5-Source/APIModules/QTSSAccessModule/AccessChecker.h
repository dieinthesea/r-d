/*
This class is used to check access rights.
The class contains the structure UserProfile which represents some basic information about the user.
such as user name, encrypted password, summary password and user group.
The functions UpdateFilePaths and UpdateUserProfiles are used to update the paths to user files and group files, and to update user information.
The RetrieveUserProfile function is used to retrieve the user information for a given username.
Other functions are used to retrieve certain variables in the class
such as the authentication field, the user file path and the group file path.
*/
 /*
    File:       AccessChecker.h

    Contains:

*/

#ifndef _QTSSACCESSCHECKER_H_
#define _QTSSACCESSCHECKER_H_

#include "QTSS.h"
#include "StrPtrLen.h"
#include "OSHeaders.h"

class AccessChecker
{
/*
Access check logic:

If "modAccess_enabled" == "Enabled

Start from the URL directory and traverse up the directory to the movie folder until the "qtaccess" file is found

If not found, the then allow access
If found, send a query to the client

Authenticate the user against QTSSPasswd
Verify that the user or member group is at the lowest ".qtacess"

Traverse the directory until ".qtaccess" is found

If found, allow access
If not found, deny access

TODO:

It is a good idea to have some caching of the ".qtaccess" data
to avoid roaming multiple directories
*/

public:
    struct UserProfile
    {
        StrPtrLen   username;
        StrPtrLen   cryptPassword;
        StrPtrLen   digestPassword;
        char**      groups;
        UInt32      maxGroupNameLen;
        UInt32      numGroups;
        UInt32      groupsSize;
    };
    
    AccessChecker();
    virtual ~AccessChecker();
    
    void UpdateFilePaths(const char* inUsersFilePath, const char* inGroupsFilePath);
    UInt32 UpdateUserProfiles();

    Bool16  HaveFilePathsChanged(const char* inUsersFilePath, const char* inGroupsFilePath);
    UserProfile* RetrieveUserProfile(const StrPtrLen* inUserName);
    inline StrPtrLen* GetAuthRealm() {return &fAuthRealm;}
    inline char* GetUsersFilePathPtr() {return fUsersFilePath;}
    inline char* GetGroupsFilePathPtr() {return fGroupsFilePath;}
    
    enum { kDefaultNumProfiles = 10, kDefaultNumGroups = 2 };
    enum {  kNoErr                  = 0x00000000, 
            kUsersFileNotFoundErr   = 0x00000001, 
            kGroupsFileNotFoundErr  = 0x00000002, 
            kBadUsersFileErr        = 0x00000004, 
            kBadGroupsFileErr       = 0x00000008,
            kUsersFileUnknownErr    = 0x00000010,
            kGroupsFileUnknownErr   = 0x00000020
        };
    
protected:
    char*               fGroupsFilePath;
    char*               fUsersFilePath;
    QTSS_TimeVal        fUsersFileModDate;
    QTSS_TimeVal        fGroupsFileModDate;
    StrPtrLen           fAuthRealm;
    
    UserProfile**       fProfiles;
    UInt32              fNumUsers;
    UInt32              fCurrentSize;
        
    static const char*  kDefaultUsersFilePath;
    static const char*  kDefaultGroupsFilePath;
    
private:
    void DeleteProfilesAndRealm();
};

#endif //_QTSSACCESSCHECKER_H_
