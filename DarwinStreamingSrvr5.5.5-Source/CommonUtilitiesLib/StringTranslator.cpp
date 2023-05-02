#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "StringTranslator.h"
#include "MyAssert.h"
#include "SafeStdLib.h"
#include <errno.h>

SInt32 StringTranslator::DecodeURL(const char* inSrc, SInt32 inSrcLen, char* ioDest, SInt32 inDestLen)
{

    if ( inSrcLen <= 0  || *inSrc != '/' )
        return OS_BadURLFormat;
    
    SInt32 theLengthWritten = 0;
    int tempChar = 0;
    int numDotChars = 0;
    Bool16 inQuery = false; 
    
    while (inSrcLen > 0)
    {
        if (theLengthWritten == inDestLen)
            return OS_NotEnoughSpace;
            
        if (*inSrc == '?')
            inQuery = true;

        if (*inSrc == '%')
        {
            if (inSrcLen < 3)
                return OS_BadURLFormat;

            char tempbuff[3];
            inSrc++;
            if (!isxdigit(*inSrc))
                return OS_BadURLFormat;
            tempbuff[0] = *inSrc;
            inSrc++;
            if (!isxdigit(*inSrc))
                return OS_BadURLFormat;
            tempbuff[1] = *inSrc;
            inSrc++;
            tempbuff[2] = '\0';
            sscanf(tempbuff, "%x", &tempChar);
            Assert(tempChar < 256);
            inSrcLen -= 3;      
        }
        else if (*inSrc == '\0')
            return OS_BadURLFormat;
        else
        {

            tempChar = *inSrc;
            inSrcLen--;
            inSrc++;
        }
        
        if (!inQuery)       // don't do seperator parsing or .. parsing in query
        {
            if ((tempChar == kPathDelimiterChar) && (kPathDelimiterChar != '/'))
                return OS_BadURLFormat;
            
            if ((tempChar == '/') && (numDotChars <= 2) && (numDotChars > 0))
            {
                Assert(theLengthWritten > numDotChars);
                ioDest -= (numDotChars + 1);
                theLengthWritten -= (numDotChars + 1);
            }

            *ioDest = tempChar;

            if (*ioDest == '.')
            {
                Assert(theLengthWritten > 0);
                if ((numDotChars == 0) && (*(ioDest - 1) == '/'))
                    numDotChars++;
                else if ((numDotChars > 0) && (*(ioDest - 1) == '.'))
                    numDotChars++;
            }
            else
                numDotChars = 0;
        }
        else
            *ioDest = tempChar;

        theLengthWritten++;
        ioDest++;
    }
    
    if (numDotChars <= 2)
        theLengthWritten -= numDotChars;
    return theLengthWritten;
}

SInt32 StringTranslator::EncodeURL(const char* inSrc, SInt32 inSrcLen, char* ioDest, SInt32 inDestLen)
{
    // return the number of chars written to ioDest
    
    SInt32 theLengthWritten = 0;
    
    while (inSrcLen > 0)
    {
        if (theLengthWritten == inDestLen)
            return OS_NotEnoughSpace;

        if ((unsigned char)*inSrc > 127)
        {
            if (inDestLen - theLengthWritten < 3)
                return OS_NotEnoughSpace;
                
            qtss_sprintf(ioDest,"%%%X",(unsigned char)*inSrc);
            ioDest += 3;
            theLengthWritten += 3;
                        inSrc++;
                        inSrcLen--;
            continue;
        }

        switch (*inSrc)
        {

            case (' '):
            case ('\r'):
            case ('\n'):
            case ('\t'):
            case ('<'):
            case ('>'):
            case ('#'):
            case ('%'):
            case ('{'):
            case ('}'):
            case ('|'):
            case ('\\'):
            case ('^'):
            case ('~'):
            case ('['):
            case (']'):
            case ('`'):
            case (';'):
            case ('?'):
            case ('@'):
            case ('='):
            case ('&'):
            case ('$'):
            case ('"'):
            {
                if ((inDestLen - theLengthWritten) < 3)
                    return OS_NotEnoughSpace;
                    
                qtss_sprintf(ioDest,"%%%X",(int)*inSrc);
                ioDest += 3;
                theLengthWritten += 3;
                break;
            }
            default:
            {
                *ioDest = *inSrc;
                ioDest++;
                theLengthWritten++;
            }
        }
        
        inSrc++;
        inSrcLen--;
    }
    
    return theLengthWritten;
}

void        StringTranslator::DecodePath(char* inSrc, UInt32 inSrcLen)
{
    for (UInt32 x = 0; x < inSrcLen; x++)
        if (inSrc[x] == '/')
            inSrc[x] = kPathDelimiterChar;
}



#if STRINGTRANSLATORTESTING
Bool16 StringTranslator::Test()
{
    static char dest[1000];
    static char* test1 = "/Hello%23%20 I want%28don't%29";
    SInt32 err = DecodeURL(test1, strlen(test1), dest, 1000);
    if (err != 22)
        return false;
    if (strcmp(dest, "/Hello#  I want(don't)") != 0)
        return false;
    err = DecodeURL(test1, 15, dest, 1000);
    if (err != 11)
        return false;
    if (strncmp(dest, "/Hello#  I ", 11) != 0)
        return false;
    err = DecodeURL(test1, 50, dest, 1000);
    if (err != OS_BadURLFormat)
        return false;
    if (strncmp(dest, "/Hello#  I want(don't)", 22) != 0)
    if (strcmp(dest, "/Hello#  I want(don't)") != 0)
        return false;
        
    err = DecodeURL(test1, strlen(test1), dest, 20);
    if (err != OS_BadURLFormat)
        return false;
    static char* test2 = "/THis%2h is a bad %28 URL!";
    err = DecodeURL(test2, strlen(test2), dest, 1000);
    if (err != OS_BadURLFormat)
        return false;
        
    static char* test3 = "/...whoa/../is./meeee%3e/./";
    static char* test4 = "/I want/to/sleep/..";
    static char* test5 = "/ve....rr/tire.././../..";
    static char* test6 = "/../beginnings/and/.";
    static char* test7 = "/../begin/%2e./../nin/%2e/gs/an/%2e%2e/fklf/%2e%2e./dfds%2e/%2e%2e/d/.%2e";
    err = DecodeURL(test3, strlen(test3), dest, 1000);
    err = DecodeURL(test4, strlen(test4), dest, 1000);
    err = DecodeURL(test5, strlen(test5), dest, 1000);
    err = DecodeURL(test6, strlen(test6), dest, 1000);
    err = DecodeURL(test7, strlen(test7), dest, 1000);
    return true;
}
#endif  
