#include "StringParser.h"
#include "StringFormatter.h"
#include "StrPtrLen.h"
#include "UserAgentParser.h"

UserAgentParser::UserAgentFields UserAgentParser::sFieldIDs[] = 
{   /* fAttrName, len, id */
    { "qtid",   strlen("qtid"),     eQtid   },
    { "qtver",  strlen("qtver"),    eQtver  },
    { "lang",   strlen("lang"),     eLang   },
    { "os",     strlen("os"),       eOs     },
    { "osver",  strlen("osver"),    eOsver  },
    { "cpu",    strlen("cpu"),      eCpu    }
};

    
UInt8 UserAgentParser::sEOLWhitespaceEqualMask[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, //0-9     
    1, 0, 0, 1, 0, 0, 0, 0, 0, 0, //10-19    
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
    0, 0, 1, 0, 0, 0, 0, 0, 0, 0, //30-39  
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //40-49
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //50-59
    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, //60-69   
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //70-79
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //80-89
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //90-99
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //100-109
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //110-119
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //120-129
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //130-139
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //140-149
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //150-159
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //160-169
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //170-179
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //180-189
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //190-199
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //200-209
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //210-219
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //220-229
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //230-239
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //240-249
    0, 0, 0, 0, 0, 0             //250-255
};

UInt8 UserAgentParser::sEOLSemicolonCloseParenMask[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, //0-9     
    1, 0, 0, 1, 0, 0, 0, 0, 0, 0, //10-19    
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //30-39   
    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, //40-49  
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, //50-59  
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //60-69  
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //70-79
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //80-89
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //90-99
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //100-109
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //110-119
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //120-129
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //130-139
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //140-149
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //150-159
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //160-169
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //170-179
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //180-189
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //190-199
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //200-209
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //210-219
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //220-229
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //230-239
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //240-249
    0, 0, 0, 0, 0, 0             //250-255
};


void UserAgentParser::Parse(StrPtrLen *inStream)
{
    StrPtrLen tempID;
    StrPtrLen tempData;
    StringParser parser(inStream);
    StrPtrLen startFields;
        

    memset(&fFieldData,0,sizeof(fFieldData) );
    
    parser.ConsumeUntil(&startFields, '(' ); 
    
    while (startFields.Len != 0)
    {
        //stop when we reach an empty line.
        tempID.Set(NULL,0);
        tempData.Set(NULL,0);
        
        parser.ConsumeLength(NULL, 1); // step past '(' or ';' if not found or at end of line does nothing
        parser.ConsumeWhitespace(); 
        parser.ConsumeUntil(&tempID, sEOLWhitespaceEqualMask ); 
        if (tempID.Len == 0) break;
    
        parser.ConsumeUntil(NULL, '=' ); // find the '='
        parser.ConsumeLength(NULL, 1); // step past if not found or at end of line does nothing
        parser.ConsumeUntil(&tempData, sEOLSemicolonCloseParenMask ); // search for end of data if not found does nothing
        if (tempData.Len == 0) break;
        
        StrPtrLen   testID;
        UInt32      fieldID;
        for (short testField = 0; testField < UserAgentParser::eNumAttributes; testField++)
        {
            testID.Set(sFieldIDs[testField].fFieldName,sFieldIDs[testField].fLen);
            fieldID = sFieldIDs[testField].fID;
            if ( (fFieldData[fieldID].fFound == false) && testID.Equal(tempID) ) 
            {   
                fFieldData[fieldID].fData = tempData;
                fFieldData[fieldID].fFound = true;              
            }
        }
        
    }
    
    if (fFieldData[eOs].fFound && !fFieldData[eOsver].fFound)
    {
        UInt16 len = (UInt16)fFieldData[eOs].fData.Len;
        char* cp = (char*)fFieldData[eOs].fData.Ptr;
        while(*cp != '%')
        {
            len--;
            if (*cp == '\0' || len == 0)
            {
                return;
            }
            cp++;
        }
        cp += 3; len -= 3;
        fFieldData[eOsver].fData.Set(cp, len);
        fFieldData[eOsver].fFound = true;
        fFieldData[eOs].fData.Len -= len+3;
    }
}
