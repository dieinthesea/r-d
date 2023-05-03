#ifndef _USERAGENTPARSER_H_
#define _USERAGENTPARSER_H_

#include "StringParser.h"
#include "StringFormatter.h"
#include "StrPtrLen.h"

class UserAgentParser 
{
    public:
        enum{   eMaxAttributeSize   =  60 };
        struct UserAgentFields
        {
            char                    fFieldName[eMaxAttributeSize + 1];
            UInt32                  fLen;
            UInt32                  fID;
        };

        struct UserAgentData
        {           
            StrPtrLen               fData;
            bool                    fFound;
        };

        enum 
        {   eQtid   = 0,
            eQtver  = 1,
            eLang   = 2,
            eOs     = 3,
            eOsver  = 4,
            eCpu    = 5,
            eNumAttributes = 6 
        };

        static UserAgentFields sFieldIDs[];
        static UInt8 sEOLWhitespaceEqualMask[];
        static UInt8 sEOLSemicolonCloseParenMask[];
        static UInt8 sWhitespaceMask[];

        UserAgentData fFieldData[eNumAttributes];
            
        void Parse(StrPtrLen *inStream);

        StrPtrLen* GetUserID()          { return    &(fFieldData[eQtid].fData);     };
        StrPtrLen* GetUserVersion()     { return    &(fFieldData[eQtver].fData);    };
        StrPtrLen* GetUserLanguage()    { return    &(fFieldData[eLang].fData);     };
        StrPtrLen* GetrUserOS()         { return    &(fFieldData[eOs].fData);       };
        StrPtrLen* GetUserOSVersion()   { return    &(fFieldData[eOsver].fData);    };
        StrPtrLen* GetUserCPU()         { return    &(fFieldData[eCpu].fData);      };
        
        UserAgentParser (StrPtrLen *inStream)  { if (inStream != NULL) Parse(inStream); }


};


#endif
