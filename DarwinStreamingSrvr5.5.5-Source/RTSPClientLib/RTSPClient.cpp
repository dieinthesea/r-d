/*
    File:       RTSPClient.cpp

    Contains:   .  
                    
    
*/

#ifndef __Win32__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <unistd.h>

#endif


#include "RTSPClient.h"
#include "StringParser.h"
#include "OSMemory.h"
#include "OSHeaders.h"

#include <errno.h>

#define ENABLE_AUTHENTICATION 1

// STRTOCHAR is used in verbose mode and for debugging
static char temp[2048]; 
static char * STRTOCHAR(StrPtrLen *theStr)
{
    temp[0] = 0;
    UInt32 len = theStr->Len < 2047 ? theStr->Len : 2047;
    if (theStr->Len > 0 || NULL != theStr->Ptr)
    {   memcpy(temp,theStr->Ptr,len); 
        temp[len] = 0;
    }
    else 
        strcpy(temp,"Empty Ptr or len is 0");
    return temp;
}

//======== includes for authentication ======
#include "base64.h"
#include "md5digest.h"
#include "OS.h"
//===========================================
static UInt8 sWhiteQuoteOrEOLorEqual[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, //0-9     // \t is a stop
    1, 0, 0, 1, 0, 0, 0, 0, 0, 0, //10-19    //'\r' & '\n' are stop conditions
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //20-29
    0, 0, 1, 0, 1, 0, 0, 0, 0, 0, //30-39   ' ' , '"' is a stop
    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, //40-49   ',' is a stop
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //50-59
    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, //60-69  '=' is a stop
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
static UInt8 sNOTWhiteQuoteOrEOLorEqual[] = // don't stop
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, //0-9      //  on '\t'
    0, 1, 1, 0, 1, 1, 1, 1, 1, 1, //10-19    // '\r', '\n'
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //20-29
    1, 1, 0, 1, 0, 1, 1, 1, 1, 1, //30-39   //  ' '
    1, 1, 1, 1, 0, 1, 1, 1, 1, 1, //40-49   // ','
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //50-59
    1, 0, 1, 1, 1, 1, 1, 1, 1, 1, //60-69   //  '='
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //70-79
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //80-89
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //90-99
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //100-109
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //110-119
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //120-129
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //130-139
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //140-149
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //150-159
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //160-169
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //170-179
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //180-189
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //190-199
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //200-209
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //210-219
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //220-229
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //230-239
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //240-249
    1, 1, 1, 1, 1, 1             //250-255
};

StrPtrLen   Authenticator::sAuthorizationStr("Authorization:");
StrPtrLen   Authenticator::sAuthBasicStr("Basic");
StrPtrLen   Authenticator::sAuthDigestStr("Digest");
StrPtrLen   Authenticator::sUsernameStr("username");
StrPtrLen   Authenticator::sRealmStr("realm");
StrPtrLen   Authenticator::sWildCardMatch("*");
StrPtrLen   Authenticator::sTrue("true");
StrPtrLen   Authenticator::sFalse("false");

Authenticator::Authenticator()
{
    char *emptyBuff = "";
    StrPtrLen emptySPL(emptyBuff);
    this->SetName(&emptySPL);
    this->SetPassword(&emptySPL);
    this->SetRealm(&emptySPL);

}

void Authenticator::Clean()
{  
    delete [] fAuthBuffer.Ptr; fAuthBuffer.Set(NULL,0); 
    delete [] fNameSPL.Ptr; fNameSPL.Set(NULL,0); 
    delete [] fPasswordSPL.Ptr; fPasswordSPL.Set(NULL,0); 
    delete [] fRealmSPL.Ptr;fRealmSPL.Set(NULL,0); 
    delete [] fMethodSPL.Ptr; fMethodSPL.Set(NULL,0); 
    
}


Bool16 Authenticator::ParseRealm(StringParser *realmParserPtr)
{   
    StrPtrLen authRealmTag("");
    Bool16 result = false;
    this->ParseTag(realmParserPtr,&authRealmTag);
    if (authRealmTag.EqualIgnoreCase(Authenticator::sRealmStr.Ptr, Authenticator::sRealmStr.Len))
    {
        result = this->GetParamValueAsNewCopy(realmParserPtr, &this->fRealmSPL);
    }
    
    return result;
}

void Authenticator::SetName(StrPtrLen *inNamePtr)       
{   this->CopyParam(inNamePtr, &this->fNameSPL);    
 }

void Authenticator::SetPassword(StrPtrLen *inPasswordPtr) 
{   this->CopyParam(inPasswordPtr, &fPasswordSPL); 
}

void Authenticator::SetMethod(StrPtrLen *inMethodStr)   
{   this->CopyParam(inMethodStr, &fMethodSPL);  
}

void Authenticator::SetRealm(StrPtrLen *inRealmPtr)     
{   this->CopyParam(inRealmPtr, &fRealmSPL);        
}

void Authenticator::SetURI(StrPtrLen *inURIPtr) 
{   // always send absolute path.
    Assert(inURIPtr); 
    UInt16 uriLen = (UInt16) (inURIPtr->Len + 2);
    
    delete [] fURISPL.Ptr; 
    fURISPL.Ptr = NEW char[uriLen];
    fURISPL.Len = inURIPtr->Len;
    memset(fURISPL.Ptr, 0, uriLen);
    char *destinationPtr = fURISPL.Ptr;
    if (*inURIPtr->Ptr != '/')
    {   *destinationPtr = '/';
        destinationPtr ++;
        fURISPL.Len++;
    }
    memcpy(destinationPtr, inURIPtr->Ptr,inURIPtr->Len);
    
}


void Authenticator::ParseTag(StringParser *parserPtr,StrPtrLen *outTagPtr)
{   
    Assert(parserPtr);
    Assert(outTagPtr);
    outTagPtr->Ptr = NULL;
    outTagPtr->Len = 0;
    
    parserPtr->ConsumeUntil(NULL,sNOTWhiteQuoteOrEOLorEqual);
    parserPtr->ConsumeUntil(outTagPtr, sWhiteQuoteOrEOLorEqual); // stop on whitespace " or =
    
    //qtss_printf("Authenticator::ParseTag =%s\n",STRTOCHAR(outTagPtr));

}

Bool16 Authenticator::CopyParam(StrPtrLen *inPtr, StrPtrLen *destPtr)
{   
    Assert(inPtr);
    Assert(destPtr);    
    
    delete [] destPtr->Ptr; destPtr->Set(NULL,0);
    destPtr->Ptr = NEW char[inPtr->Len + 1];
    
    if (destPtr->Ptr == NULL)
    {   destPtr->Len = 0;
        return false;
    }

    destPtr->Ptr[inPtr->Len] = 0;
    destPtr->Len = inPtr->Len;
    if (destPtr->Len > 0)
    {   ::memcpy(destPtr->Ptr,inPtr->Ptr,inPtr->Len);
    }

    return true;
    
}


Bool16 Authenticator::GetParamValue(StringParser *valueSourcePtr, StrPtrLen *outParamValuePtr)
{
    Assert(valueSourcePtr);
    Assert(outParamValuePtr);
    Assert(outParamValuePtr->Ptr == NULL);
    StrPtrLen temp;
    outParamValuePtr->Set(NULL,0);

    {   char temp[1024]; 
        memcpy(temp,valueSourcePtr->GetCurrentPosition(),valueSourcePtr->GetDataRemaining()); 
        temp[valueSourcePtr->GetDataRemaining()] = 0;
    }
    
    valueSourcePtr->ConsumeUntil(&temp, sNOTWhiteQuoteOrEOLorEqual);
    if ( (temp.Len > 0) &&  ( '"' == temp.Ptr[temp.Len -1] )  )// if quote read to next quote or end
    {   
        valueSourcePtr->ConsumeUntil(outParamValuePtr, '"');
        //qtss_printf("Found quoted value=%s\n",STRTOCHAR(outParamValuePtr));
    }
    else // get just the non-whitespace or EOL
    {   //qtss_printf("Authenticator::GetParamValue No quotes\n");
        valueSourcePtr->ConsumeWhitespace();
        valueSourcePtr->ConsumeUntilWhitespace(outParamValuePtr);
    }

    //qtss_printf("Authenticator::GetParamValue len = %lu =%s\n",outParamValuePtr->Len, STRTOCHAR(outParamValuePtr));
    
    return true;
}
Bool16 Authenticator::GetParamValueAsNewCopy(StringParser *valueSourcePtr, StrPtrLen *outParamValueCopyPtr)
{
    StrPtrLen theParamValue;
    if (!this->GetParamValue(valueSourcePtr, &theParamValue))
        return false;
    
    return this->CopyParam(&theParamValue, outParamValueCopyPtr);
}

Bool16 Authenticator::GetMatchListParamValueAsNewCopy(StringParser *valueSourcePtr, StrPtrLen *inMatchListParamValuePtr, SInt16 numToMatch, StrPtrLen *outParamValueCopyPtr)
{
    StrPtrLen theParamValue;
    if (!this->GetParamValue(valueSourcePtr, &theParamValue))
        return false;
        
    if(inMatchListParamValuePtr && numToMatch > 0)
    {   StringParser paramList(&theParamValue);
        StrPtrLen theListParamValue;
        while (this->GetParamValue(&paramList, &theListParamValue))
        {   for (SInt16 count = 0;count < numToMatch; count++)
            {   
                if  (   theListParamValue.Equal(inMatchListParamValuePtr->Ptr) 
                     || sWildCardMatch.Equal (inMatchListParamValuePtr->Ptr)
                    )
                {   return this->CopyParam(&theListParamValue, outParamValueCopyPtr);
                }
                inMatchListParamValuePtr++;
            }
        }
        return false;
    }
    return this->CopyParam(&theParamValue, outParamValueCopyPtr);
}

void    Authenticator::ResetRequestLen(StrPtrLen *theRequestPtr, StrPtrLen *theParamsPtr)
{   // makes a new buffer and copies everything after first \r\n into the buffer and terminates the req.
    // after the \r\n 
    
    static const char *requestParamStart = "\r\n";
    Assert (theRequestPtr);
    Assert (theRequestPtr->Ptr);
    Assert (theRequestPtr->Len > ::strlen(requestParamStart) );
    
    Assert (theParamsPtr);
    Assert (theParamsPtr->Ptr == NULL);
    Assert (theParamsPtr->Len == 0);

    char *theLastChar = ::strstr(theRequestPtr->Ptr, requestParamStart);
    if (theLastChar)
    {   StrPtrLen tempParams;
        tempParams.Ptr = &theLastChar[2];
        tempParams.Len = ::strlen(&theLastChar[2]);
        CopyParam(&tempParams, theParamsPtr);   
        theLastChar[2] = 0; // terminate after \r\n
    }   

}


char * Authenticator::GetRequestHeader( StrPtrLen *inSourceStr, StrPtrLen *searchHeaderStr,StrPtrLen *outHeaderStr)
{
    StrPtrLen   headers;
    StrPtrLen   headersTerminator("\r\n\r\n");  
    char *endSourceCharPtr = inSourceStr->FindString(&headersTerminator);
    if (endSourceCharPtr == NULL)
        return NULL;

    //qtss_printf("Authenticator::GetRequestHeader source=%s\n find=|%s|\n", inSourceStr->Ptr, headersTerminator.Ptr);   
    headers.Set(inSourceStr->Ptr,endSourceCharPtr - inSourceStr->Ptr);
    
    return headers.FindStringIgnoreCase(searchHeaderStr);
}

void Authenticator::RemoveAuthLine(StrPtrLen *theRequestPtr)
{
    Assert( theRequestPtr);
    Assert( theRequestPtr->Ptr);
    Assert( theRequestPtr->Len == ::strlen(theRequestPtr->Ptr) );
    
    if (theRequestPtr->Ptr != NULL) 
    {           
        char *theHeaderStart = GetRequestHeader(theRequestPtr, &Authenticator::sAuthorizationStr);
        char *eol = StrPtrLen(theHeaderStart).FindString("\r\n");
            
        // finally remove the Authorization: line
        if (theHeaderStart && eol)
        {   strcpy(theHeaderStart,eol + 2); 
        }               
    }

}


//===========================================

// request tags
StrPtrLen   DigestAuth::sResponseStr("response"); //the response 
StrPtrLen   DigestAuth::sUriStr("uri"); // copy of of the URL must 
StrPtrLen   DigestAuth::sCnonceStr("cnonce"); // in request only if qop is in response

// response tags
StrPtrLen   DigestAuth::sStaleStr("stale");

// request and response tags
StrPtrLen   DigestAuth::sQopStr("qop"); // quoted string list of one or more options -- in request if in response
StrPtrLen   DigestAuth::sNonceStr("nonce"); // quoted string return back to the server in the request
StrPtrLen   DigestAuth::sNonceCountStr("nc"); // in request only if qop is in response
StrPtrLen   DigestAuth::sOpaqueStr("opaque");// quoted string return back to the server in the request
StrPtrLen   DigestAuth::sDomainStr("domain"); // quoted string list of one or more URLs on in response
StrPtrLen   DigestAuth::sAlgorithmStr("algorithm"); // quoted string list of one or more URLs on in response

// response values
StrPtrLen   DigestAuth::sQopAuthStr("auth");
StrPtrLen   DigestAuth::sQopAuthIntStr("auth-int");
StrPtrLen   DigestAuth::sMD5Str("MD5");
StrPtrLen   DigestAuth::sMD5SessStr("MD5-sess");


DigestAuth::DigestAuth()
{
    ReqFieldsClean();
    fAlgorithm = 0; 
    fStale = false;
    fNonceCount = 0;

}
Bool16  DigestAuth::ParseParams(StrPtrLen *authParamsPtr)
{
    Bool16 result = false;
    StrPtrLen authTag("");
    
    if (authParamsPtr != NULL && authParamsPtr->Ptr != NULL) do 
    {   fNonceCount = 0;
    
        //qtss_printf("DigestAuth::ParseParams authParams=%s\n",STRTOCHAR(authParamsPtr));
        
        StringParser paramParser(authParamsPtr);
        if (!this->ParseRealm(&paramParser))
            break;
        
        while (paramParser.GetDataRemaining() > 0)
        {
            this->ParseTag(&paramParser,&authTag);
            
            //qtss_printf("parsed tag = %s\n",STRTOCHAR(&authTag));
            
            if (authTag.EqualIgnoreCase(this->sNonceStr.Ptr, this->sNonceStr.Len))
            {   // NONCE in Response
                result = this->GetParamValueAsNewCopy(&paramParser, &this->fnonce);
                if (!result) 
                {   
                    break;
                }
                else continue;
            }

            if (authTag.EqualIgnoreCase(this->sStaleStr.Ptr, this->sStaleStr.Len))
            {   // STALE in Response
                result = this->GetParamValueAsNewCopy(&paramParser, &this->fStaleStr);
                if (!result) 
                {   
                    break;
                }
                else 
                {   
                    if (this->fStaleStr.EqualIgnoreCase(Authenticator::sTrue.Ptr, Authenticator::sTrue.Len))
                    {   fStale = true;
                    }
                    else
                    {   fStale = false;
                    }
                    continue;
                }
            }
            
            if (authTag.EqualIgnoreCase(this->sOpaqueStr.Ptr, this->sOpaqueStr.Len))
            {   // OPAQUE in Response
                result = this->GetParamValueAsNewCopy(&paramParser, &this->fopaque);
                if (!result) 
                {   
                    break;
                }
                else  continue;
            }
            
            if (authTag.EqualIgnoreCase(this->sQopStr.Ptr, this->sQopStr.Len))
            {   // QOP in Response
                StrPtrLen matchList[2];
                matchList[0].Set(sQopAuthStr.Ptr,sQopAuthStr.Len);
                matchList[1].Set(sQopAuthIntStr.Ptr,sQopAuthIntStr.Len);
                result = Authenticator::GetMatchListParamValueAsNewCopy(&paramParser, matchList, 2, &this->fqop);
                if (!result) 
                {   
                    break;
                }
                else  continue;
            }

            if (authTag.EqualIgnoreCase(this->sAlgorithmStr.Ptr, this->sAlgorithmStr.Len))
            {   // ALGORITHM in Response
                // This is for completeness we only do MD5
                result = this->GetParamValueAsNewCopy(&paramParser, &this->fAlgorithmStr);
                if (!result) 
                {   
                    break;
                }
                else continue;
            }

            if (authTag.EqualIgnoreCase(this->sDomainStr.Ptr, this->sDomainStr.Len))
            {   // DOMAIN in Response
                // just get the first URL in the domain list
                result = Authenticator::GetMatchListParamValueAsNewCopy(&paramParser, &Authenticator::sWildCardMatch, 1, &this->fqop);
                if (!result) 
                {   
                    break;
                }
                else continue;
            }
        }
            
    } while (false);

    return result;
}

void DigestAuth::AddAuthParam(StrPtrLen *theTagPtr, StrPtrLen *theValuePtr, Bool16 quoted)
{
    Assert(fReqFields.fNumFields < kMaxReqParams);
    Assert(theTagPtr); // must be valid
    if (fReqFields.fNumFields < kMaxReqParams && theValuePtr && theValuePtr->Ptr && theValuePtr->Len)
    {
        fReqFields.fNumFields ++;
        fReqFields.fReqParamTags[fReqFields.fNumFields -1] = theTagPtr;
        fReqFields.fReqParamValues[fReqFields.fNumFields -1] = theValuePtr;
        fReqFields.fQuoted[fReqFields.fNumFields -1] = quoted;
        //qtss_printf("DigestAuth::AddAuthParam [%lu] tag=%s ",fReqFields.fNumFields -1, STRTOCHAR(theTagPtr));
        //qtss_printf("value=%s \n",STRTOCHAR(theValuePtr));
        
    }
    else
    {   //qtss_printf("DigestAuth::AddAuthParam ignored [%lu] tag=%s value=%s \n",fReqFields.fNumFields -1, STRTOCHAR(theTagPtr),STRTOCHAR(theValuePtr));            
    }
    
}

void DigestAuth::SetNonceCountStr()
{
    char tempBuff[32];      
    qtss_sprintf(tempBuff,"%08x",fNonceCount); // return hex string value
    StrPtrLen tempCountStr(tempBuff);
    CopyParam(&tempCountStr, &fNonceCountStr);
}

void DigestAuth::GenerateAuthorizationRequestLine(StrPtrLen *requestPtr)
{
    
    static const UInt32 ktempbuffSize = 1024;
    char tempBuffer[ktempbuffSize];
    UInt32 buffsize = this->ParamsLen(requestPtr); 
    delete [] fAuthBuffer.Ptr;
    fAuthBuffer.Ptr = NEW char[buffsize];
    memset(fAuthBuffer.Ptr,0,buffsize);
    fAuthBuffer.Len = buffsize;
    
    qtss_sprintf(fAuthBuffer.Ptr, "%s %s ",sAuthorizationStr.Ptr, sAuthDigestStr.Ptr);
    
    //qtss_printf ("DigestAuth::GenerateAuthorizationRequestLine buffsize = %lu fAuthBuffer= %s\n",buffsize, fAuthBuffer.Ptr);
        
    StrPtrLen *theTagPtr;
    StrPtrLen *theValuePtr;
    UInt32 paramLen;
    Bool16 quoted;
    for (SInt32 paramIndex = 0; paramIndex < fReqFields.fNumFields; paramIndex++)
    {   
        paramLen = 0;
        theTagPtr  = fReqFields.fReqParamTags[paramIndex];
        theValuePtr = fReqFields.fReqParamValues[paramIndex];
        Assert(theTagPtr); // shouldn't ever happen
        
        paramLen += theTagPtr->Len;
        quoted = fReqFields.fQuoted[paramIndex];
        if (theValuePtr)  // this can be NULL
            paramLen += theValuePtr->Len + 5;
                
        Assert(paramLen < ktempbuffSize);
        if (quoted)
        {   if(theValuePtr && theValuePtr->Len > 0)
                qtss_sprintf(tempBuffer, "%s=\"%s\"",theTagPtr->Ptr, theValuePtr->Ptr);
            else // empty send: param=""
                qtss_sprintf(tempBuffer, "%s=\"\"",theTagPtr->Ptr);
        }
        else
        {   if(theValuePtr && theValuePtr->Len > 0)
                qtss_sprintf(tempBuffer, "%s=%s",theTagPtr->Ptr, theValuePtr->Ptr);
            else // empty send: param=""
                qtss_sprintf(tempBuffer, "%s=\"\"",theTagPtr->Ptr);
        }
        //qtss_printf("add %s to auth line\n",tempBuffer);
        strcat(fAuthBuffer.Ptr,tempBuffer);
        if (paramIndex < fReqFields.fNumFields -1)
            strcat(fAuthBuffer.Ptr,",");
        
        //qtss_printf("DigestAuth::GenerateAuthorizationRequestLine bufferLen=%ld fAuthBuffer= %s\n",::strlen(fAuthBuffer.Ptr),fAuthBuffer.Ptr);
    }
    
    ::strcat(fAuthBuffer.Ptr, "\r\n");

    //qtss_printf("DigestAuth::GenerateAuthorizationRequestLine bufferLen=%ld fAuthBuffer= %s\n",::strlen(fAuthBuffer.Ptr),fAuthBuffer.Ptr);
}

void DigestAuth::ResetAuthParams()
{
    ReqFieldsClean();
    delete [] fAuthBuffer.Ptr;  fAuthBuffer.Set(NULL,0);

}

void DigestAuth::MakeRequestDigest()
{
    delete [] fRequestDigestStr.Ptr;
    fRequestDigestStr.Ptr = NULL;
    StrPtrLen emptyStr;
    StrPtrLen hA1;
    //qtss_printf("DigestAuth::MakeRequestDigest fNameSPL=%s\n",STRTOCHAR(&fNameSPL));
    //qtss_printf("DigestAuth::MakeRequestDigest fRealmSPL=%s\n",STRTOCHAR(&fRealmSPL));
    //qtss_printf("DigestAuth::MakeRequestDigest fPasswordSPL=%s\n",STRTOCHAR(&fPasswordSPL));
    //qtss_printf("DigestAuth::MakeRequestDigest fnonce=%s\n",STRTOCHAR(&fnonce));
    //qtss_printf("DigestAuth::MakeRequestDigest fcnonce=%s\n",STRTOCHAR(&fcnonce));

    ::CalcHA1( &sMD5Str, &fNameSPL, &fRealmSPL, &fPasswordSPL, &fnonce, &fcnonce, &hA1);

    //qtss_printf("DigestAuth::MakeRequestDigest CalcHA1=%s\n",STRTOCHAR(&hA1));
    //qtss_printf("DigestAuth::MakeRequestDigest fnonce=%s\n",STRTOCHAR(&fnonce));
    //qtss_printf("DigestAuth::MakeRequestDigest fNonceCountStr=%s\n",STRTOCHAR(&fNonceCountStr));
    //qtss_printf("DigestAuth::MakeRequestDigest fcnonce=%s\n",STRTOCHAR(&fcnonce));
    //qtss_printf("DigestAuth::MakeRequestDigest fqop=%s\n",STRTOCHAR(&fqop));
    //qtss_printf("DigestAuth::MakeRequestDigest fMethodSPL=%s\n",STRTOCHAR(&fMethodSPL));
    //qtss_printf("DigestAuth::MakeRequestDigest fURISPL=%s\n",STRTOCHAR(&fURISPL));
    //qtss_printf("DigestAuth::MakeRequestDigest emptyStr=%s\n",STRTOCHAR(&emptyStr));
    //qtss_printf("DigestAuth::MakeRequestDigest fMethodSPL=%s\n",STRTOCHAR(&emptyStr));

    ::CalcRequestDigest(&hA1, &fnonce, &fNonceCountStr, &fcnonce, &fqop, &fMethodSPL, &fURISPL, &emptyStr, &fRequestDigestStr); 
    delete [] hA1.Ptr;
    //qtss_printf("DigestAuth::MakeRequestDigest fRequestDigestStr=%s\n",STRTOCHAR(&fRequestDigestStr));
}

void DigestAuth::MakeCNonce()
{
    fAuthTime = OS::UnixTime_Secs();
    
    char timeStr[64];
    qtss_sprintf(timeStr,"%lu",(UInt32) fAuthTime);
    StrPtrLen timeSPL(timeStr);
    
    delete [] fcnonce.Ptr;
    fcnonce.Ptr = NULL;
    StrPtrLen emptyStr;
    StrPtrLen hA1;
    ::CalcHA1( &sMD5Str, &timeSPL, &fRealmSPL, &fPasswordSPL, &fnonce, &timeSPL, &hA1);
    ::CalcRequestDigest(&hA1, &fnonce, &timeSPL, &timeSPL, &fqop, &timeSPL, &fURISPL, &emptyStr, &fcnonce);
    delete [] hA1.Ptr;
}

void DigestAuth::AttachAuthParams(StrPtrLen *theRequestPtr)
{
    StrPtrLen requestParams;
    
    //qtss_printf(" DigestAuth::AttachAuthParams request IN =%s\n", STRTOCHAR(theRequestPtr));
    
    char*  hasAuthorization = ::strstr(theRequestPtr->Ptr, sAuthorizationStr.Ptr);
    if (NULL != hasAuthorization) 
        return;

    this->ResetRequestLen(theRequestPtr,&requestParams);
    this->ResetAuthParams();
    
    fNonceCount ++; 
    AddAuthParam( &sUsernameStr, &fNameSPL, true);              
    AddAuthParam( &sRealmStr, &fRealmSPL, true);        
    AddAuthParam( &sNonceStr, &fnonce, true);       
    AddAuthParam( &sOpaqueStr, &fopaque, true);     
    AddAuthParam( &sUriStr, &fURISPL, true);        
    
    if (fqop.Ptr != NULL)  // 
    {
        AddAuthParam( &sQopStr, &sQopAuthStr, false);// only auth   
        SetNonceCountStr();
        AddAuthParam( &sNonceCountStr, &fNonceCountStr, false);// only auth 
        MakeCNonce();
        AddAuthParam(&sCnonceStr, &fcnonce, true);
    }
    
    MakeRequestDigest();
    AddAuthParam (&sResponseStr, &fRequestDigestStr, true);
    GenerateAuthorizationRequestLine(theRequestPtr);
    ::strcat(theRequestPtr->Ptr, fAuthBuffer.Ptr); 
    ::strcat(theRequestPtr->Ptr, requestParams.Ptr); // put the request params back
    theRequestPtr->Len = ::strlen(theRequestPtr->Ptr);
    delete [] requestParams.Ptr; requestParams.Set(NULL,0);
    //qtss_printf(" DigestAuth::AttachAuthParams request OUT =%s\n", STRTOCHAR(theRequestPtr));
}

UInt32 DigestAuth::ParamsLen(StrPtrLen *requestPtr)
{   UInt32 fieldLens  = requestPtr->Len;
    StrPtrLen   *theParamPtr = NULL;
    StrPtrLen   *theTagPtr = NULL;
    SInt32 numParams = 0;
    
    while (numParams != fReqFields.fNumFields )
    {    
        theParamPtr = fReqFields.fReqParamValues[numParams];
        theTagPtr = fReqFields.fReqParamTags[numParams];
        if (theParamPtr != NULL) 
        {   fieldLens += theParamPtr->Len;
        }
        if (theTagPtr != NULL) 
        {   fieldLens += theTagPtr->Len;
        }       
        fieldLens += 5;// room for spaces or " = and ,
        numParams ++;
    }

    return fieldLens;
}

DigestAuth::~DigestAuth()
{   ResetAuthParams();
    delete [] fNonceCountStr.Ptr;   fNonceCountStr.Set(NULL,0);  
    delete [] fRequestDigestStr.Ptr;fRequestDigestStr.Set(NULL,0);  
    delete [] fURIStr.Ptr;          fURIStr.Set(NULL,0);  
    delete [] fcnonce.Ptr;          fcnonce.Set(NULL,0);  
    delete [] fNonceCountStr.Ptr;   fNonceCountStr.Set(NULL,0);  
    delete [] fnonce.Ptr;           fnonce.Set(NULL,0);  
    delete [] fopaque.Ptr;          fopaque.Set(NULL,0);  
    delete [] fqop.Ptr;             fqop.Set(NULL,0);  
    delete [] fAlgorithmStr.Ptr;    fAlgorithmStr.Set(NULL,0);  
    delete [] fStaleStr.Ptr;        fStaleStr.Set(NULL,0);  
    Clean();
}

//===========================================

Bool16  BasicAuth::ParseParams(StrPtrLen *authParamsPtr)
{
    StringParser realmParser(authParamsPtr);
    return this->ParseRealm(&realmParser);
}

UInt32 BasicAuth::ParamsLen(StrPtrLen *requestPtr)
{
    return requestPtr->Len + fNameSPL.Len + fPasswordSPL.Len + 1;
}

void BasicAuth::AttachAuthParams(StrPtrLen *theRequestPtr)
{
    char*  hasAuthorization = ::strstr(theRequestPtr->Ptr, sAuthorizationStr.Ptr);
    if (NULL != hasAuthorization) 
        return;
    
    UInt32 buffLen = ParamsLen(theRequestPtr);
    if (fAuthBuffer.Ptr == NULL)
    {   fAuthBuffer.Ptr = NEW char[buffLen];
        memset(fAuthBuffer.Ptr,0,buffLen);
        fAuthBuffer.Len = buffLen;
    }
    StrPtrLen requestParams;
    this->ResetRequestLen(theRequestPtr,&requestParams);
    //qtss_printf("BasicAuth::parsed requestParams.Ptr = %s \n",requestParams.Ptr);

    char unEncodedBuffer[80];
    qtss_sprintf(unEncodedBuffer, "%s:%s",fNameSPL.Ptr, fPasswordSPL.Ptr);
    //qtss_printf("unEncodedBuffer=%s\n",unEncodedBuffer);
    ::Base64encode(fEncodedBuffer, unEncodedBuffer,::strlen(unEncodedBuffer));  
    qtss_sprintf(fAuthBuffer.Ptr, "%s %s %s\r\n",sAuthorizationStr.Ptr, sAuthBasicStr.Ptr, fEncodedBuffer);
    ::strcat(theRequestPtr->Ptr, fAuthBuffer.Ptr);
    ::strcat(theRequestPtr->Ptr, requestParams.Ptr); // put the request back
    delete [] requestParams.Ptr;
    theRequestPtr->Len = ::strlen(theRequestPtr->Ptr);
        
    //qtss_printf("BasicAuth::theRequestPtr->Ptr = %s \n",theRequestPtr->Ptr);
}

//===========================================


Authenticator *AuthParser::ParseChallenge(StrPtrLen *challengePtr)
{
    Bool16 result = false; 
    Authenticator *authenticator = NULL;
    StrPtrLen   theChallenge;
    StrPtrLen   headerTerminator("\r"); 

    if (NULL == challengePtr) 
        return NULL;
        
    StringParser authParser(challengePtr);
    StrPtrLen   authWord;
    StrPtrLen   authParams;
    
    // consume WWW-Authenticate
    authParser.ConsumeUntilWhitespace(&authWord); 
    authParser.ConsumeWhitespace();

    // Get the authentication type
    authParser.ConsumeUntilWhitespace(&authWord);
    authParser.ConsumeWhitespace();
    
    // Get the params
    authParser.GetThruEOL(&authParams);
    if (authWord.EqualIgnoreCase(Authenticator::sAuthBasicStr.Ptr, Authenticator::sAuthBasicStr.Len))
    {
            
        authenticator =  NEW BasicAuth();
        Assert(authenticator);
        if (authenticator)
            result =  authenticator->ParseParams(&authParams);
    }
    else if (authWord.EqualIgnoreCase(Authenticator::sAuthDigestStr.Ptr, Authenticator::sAuthDigestStr.Len))
    {
        authenticator = NEW DigestAuth();
        Assert(authenticator);
        if (authenticator)
            result =  authenticator->ParseParams(&authParams);
    }

    return  authenticator;  
}

//===========================================



static char* sEmptyString = "";
char* RTSPClient::sUserAgent = "None";
char* RTSPClient::sControlID = "trackID";

RTSPClient::InterleavedParams RTSPClient::sInterleavedParams;

RTSPClient::RTSPClient(ClientSocket* inSocket, Bool16 verbosePrinting, char* inUserAgent)
:   fAuthenticator(NULL),
    fSocket(inSocket),
    fVerbose(verbosePrinting),
    fCSeq(1),
    fStatus(0),
    fSessionID(sEmptyString),
    fServerPort(0),
    fContentLength(0),
    fSetupHeaders(NULL),
    fNumChannelElements(kMinNumChannelElements),
    fNumSSRCElements(0),
    fSSRCMapSize(kMinNumChannelElements),
    fNumFieldIDElements(0),
    fFieldIDMapSize(kMinNumChannelElements),
    fPacketBuffer(NULL),
    fPacketBufferOffset(0),
    fPacketOutstanding(false),
    fRecvContentBuffer(NULL),
    fSendBufferLen(0),
    fContentRecvLen(0),
    fHeaderRecvLen(0),
    fHeaderLen(0),
    fSetupTrackID(0),
    fTransactionStarted(false),
    fReceiveInProgress(false),
    fReceivedResponse(false),
    fConnected(false),
    fResponseCount(0),
    fTransportMode(kPlayMode), 
    fPacketDataInHeaderBufferLen(0),
    fPacketDataInHeaderBuffer(NULL),
    fUserAgent(NULL),
    fControlID(RTSPClient::sControlID)
{
#if DEBUG
    fIsFirstPacket = true;
#endif

    fChannelTrackMap = NEW ChannelMapElem[kMinNumChannelElements];
    ::memset(fChannelTrackMap, 0, sizeof(ChannelMapElem) * kMinNumChannelElements);
    fSSRCMap = NEW SSRCMapElem[kMinNumChannelElements];
    ::memset(fSSRCMap, 0, sizeof(SSRCMapElem) * kMinNumChannelElements);
    fFieldIDMap = NEW FieldIDArrayElem[kMinNumChannelElements];
    ::memset(fFieldIDMap, 0, sizeof(FieldIDArrayElem) * kMinNumChannelElements);
    
    ::memset(fSendBuffer, 0,kReqBufSize + 1);
    ::memset(fRecvHeaderBuffer, 0,kReqBufSize + 1);
    fHaveTransactionBuffer = false;
    
    fSetupHeaders = NEW char[2];
    fSetupHeaders[0] = '\0';
    
    ::memset(&sInterleavedParams, 0, sizeof(sInterleavedParams));
        
        if (inUserAgent != NULL)
        {
            fUserAgent = NEW char[::strlen(inUserAgent) + 1];
            ::strcpy(fUserAgent, inUserAgent);
        }
        else
        {
            fUserAgent = NEW char[::strlen(sUserAgent) + 1];
            ::strcpy(fUserAgent, sUserAgent);
        }
}

RTSPClient::~RTSPClient()
{
    delete [] fRecvContentBuffer;
    delete [] fURL.Ptr;
    delete [] fName.Ptr;
    delete [] fPassword.Ptr;
    if (fSessionID.Ptr != sEmptyString)
        delete [] fSessionID.Ptr;
        
    delete [] fSetupHeaders;
    delete [] fChannelTrackMap;
    delete [] fSSRCMap;
    delete [] fFieldIDMap;
    delete [] fPacketBuffer;
        
    delete [] fUserAgent;
    if (fControlID != RTSPClient::sControlID)
        delete [] fControlID;
}

void RTSPClient::SetControlID(char* controlID)
{
    if (NULL == controlID)
        return;
        
    if (fControlID != RTSPClient::sControlID)
        delete [] fControlID;

	fControlID = NEW char[::strlen(controlID) + 1]; 
	::strcpy(fControlID, controlID);

}

void RTSPClient::SetName(char *name)
{ 
    Assert (name);  
    delete [] fName.Ptr;  
    fName.Ptr = NEW char[::strlen(name) + 2]; 
    ::strcpy(fName.Ptr, name);
    fName.Len = ::strlen(name);
}

void RTSPClient::SetPassword(char *password)
{   
    Assert (password);  
    delete [] fPassword.Ptr;  
    fPassword.Ptr = NEW char[::strlen(password) + 2]; 
    ::strcpy(fPassword.Ptr, password);
    fPassword.Len = ::strlen(password);
}

void RTSPClient::Set(const StrPtrLen& inURL)
{
    delete [] fURL.Ptr;
    fURL.Ptr = NEW char[inURL.Len + 2];
    fURL.Len = inURL.Len;
    char* destPtr = fURL.Ptr;
    
    // add a leading '/' to the url if it isn't a full URL and doesn't have a leading '/'
    if ( !inURL.NumEqualIgnoreCase("rtsp://", strlen("rtsp://")) && inURL.Ptr[0] != '/')
    {
        *destPtr = '/';
        destPtr++;
        fURL.Len++;
    }
    ::memcpy(destPtr, inURL.Ptr, inURL.Len);
    fURL.Ptr[fURL.Len] = '\0';
}

void RTSPClient::SetSetupParams(Float32 inLateTolerance, char* inMetaInfoFields)
{
    delete [] fSetupHeaders;
    fSetupHeaders = NEW char[256];
    fSetupHeaders[0] = '\0';
    
    if (inLateTolerance != 0)
        qtss_sprintf(fSetupHeaders, "x-Transport-Options: late-tolerance=%f\r\n", inLateTolerance);
    if ((inMetaInfoFields != NULL) && (::strlen(inMetaInfoFields) > 0))
        qtss_sprintf(fSetupHeaders + ::strlen(fSetupHeaders), "x-RTP-Meta-Info: %s\r\n", inMetaInfoFields);
}

OS_Error RTSPClient::SendDescribe(Bool16 inAppendJunkData)
{
    if (!fTransactionStarted)
    {
        qtss_sprintf(fMethod,"%s","DESCRIBE");

        if (inAppendJunkData)
        {
            qtss_sprintf(fSendBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %lu\r\nAccept: application/sdp\r\nContent-Length: 200\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fUserAgent);
            UInt32 theBufLen = ::strlen(fSendBuffer);
            Assert((theBufLen + 200) < kReqBufSize);
            for (UInt32 x = theBufLen; x < (theBufLen + 200); x++)
                fSendBuffer[x] = 'd';
            fSendBuffer[theBufLen + 200] = '\0';
        }
        else
        {
            qtss_sprintf(fSendBuffer, "DESCRIBE %s RTSP/1.0\r\nCSeq: %lu\r\nAccept: application/sdp\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fUserAgent);
        }
    }
    return this->DoTransaction();
}

OS_Error RTSPClient::SendSetParameter()
{
    if (!fTransactionStarted)
    {
        qtss_sprintf(fMethod,"%s","SET_PARAMETER");
        qtss_sprintf(fSendBuffer, "SET_PARAMETER %s RTSP/1.0\r\nCSeq:%lu\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fUserAgent);
    }
    return this->DoTransaction();
}

OS_Error RTSPClient::SendOptions()
{
    if (!fTransactionStarted)
    {
        qtss_sprintf(fMethod,"%s","OPTIONS");
        qtss_sprintf(fSendBuffer, "OPTIONS * RTSP/1.0\r\nCSeq:%lu\r\nUser-agent: %s\r\n\r\n", fCSeq, fUserAgent);
     }
    return this->DoTransaction();
}

OS_Error RTSPClient::SendOptionsWithRandomDataRequest(SInt32 dataSize)
{
    if (!fTransactionStarted)
    {
        qtss_sprintf(fMethod,"%s","OPTIONS");
        qtss_sprintf(fSendBuffer, "OPTIONS * RTSP/1.0\r\nCSeq:%lu\r\nUser-agent: %s\r\nRequire: x-Random-Data-Size\r\nx-Random-Data-Size: %ld\r\n\r\n", fCSeq, fUserAgent, dataSize);
     }
    return this->DoTransaction();
}


OS_Error RTSPClient::SendReliableUDPSetup(UInt32 inTrackID, UInt16 inClientPort)
{
    fSetupTrackID = inTrackID; // Needed when SETUP response is received.
    fSendBuffer[0] = 0;
    if (!fTransactionStarted)
    {
        qtss_sprintf(fMethod,"%s","SETUP");
        
        if (fTransportMode == kPushMode)
			qtss_sprintf(fSendBuffer, "SETUP %s/%s=%lu RTSP/1.0\r\nCSeq: %lu\r\n%sTransport: RTP/AVP;unicast;client_port=%u-%u;mode=record\r\nx-Retransmit: our-retransmit\r\nUser-agent: %s\r\n\r\n", fURL.Ptr,fControlID, inTrackID, fCSeq, fSessionID.Ptr, inClientPort, inClientPort + 1, fUserAgent);
        else 
            qtss_sprintf(fSendBuffer, "SETUP %s/%s=%lu RTSP/1.0\r\nCSeq: %lu\r\n%sTransport: RTP/AVP;unicast;client_port=%u-%u\r\nx-Retransmit: our-retransmit\r\nUser-agent: %s\r\n\r\n", fURL.Ptr,fControlID, inTrackID, fCSeq, fSessionID.Ptr, inClientPort, inClientPort + 1, fUserAgent);
    }
    return this->DoTransaction();
}

OS_Error RTSPClient::SendUDPSetup(UInt32 inTrackID, UInt16 inClientPort)
{
    fSetupTrackID = inTrackID; // Needed when SETUP response is received.
    
    if (!fTransactionStarted)
    {
        qtss_sprintf(fMethod,"%s","SETUP");
        
        if (fTransportMode == kPushMode)
            qtss_sprintf(fSendBuffer, "SETUP %s/%s=%lu RTSP/1.0\r\nCSeq: %lu\r\n%sTransport: RTP/AVP;unicast;client_port=%u-%u;mode=record\r\nUser-agent: %s\r\n\r\n", fURL.Ptr,fControlID, inTrackID, fCSeq, fSessionID.Ptr, inClientPort, inClientPort + 1, fUserAgent);
        else 
            qtss_sprintf(fSendBuffer, "SETUP %s/%s=%lu RTSP/1.0\r\nCSeq: %lu\r\n%sTransport: RTP/AVP;unicast;client_port=%u-%u\r\n%sUser-agent: %s\r\n\r\n", fURL.Ptr,fControlID, inTrackID, fCSeq, fSessionID.Ptr, inClientPort, inClientPort + 1, fSetupHeaders, fUserAgent);
    }
    return this->DoTransaction();
}

OS_Error RTSPClient::SendTCPSetup(UInt32 inTrackID, UInt16 inClientRTPid, UInt16 inClientRTCPid)
{
    fSetupTrackID = inTrackID; // Needed when SETUP response is received.
    
    if (!fTransactionStarted)
    {   
        qtss_sprintf(fMethod,"%s","SETUP");
        
        if (fTransportMode == kPushMode)
            qtss_sprintf(fSendBuffer, "SETUP %s/%s=%lu RTSP/1.0\r\nCSeq: %lu\r\n%sTransport: RTP/AVP/TCP;unicast;mode=record;interleaved=%u-%u\r\nUser-agent: %s\r\n\r\n", fURL.Ptr,fControlID, inTrackID, fCSeq, fSessionID.Ptr,inClientRTPid, inClientRTCPid, fUserAgent);
        else 
            qtss_sprintf(fSendBuffer, "SETUP %s/%s=%lu RTSP/1.0\r\nCSeq: %lu\r\n%sTransport: RTP/AVP/TCP;unicast;interleaved=%u-%u\r\n%sUser-agent: %s\r\n\r\n", fURL.Ptr,fControlID, inTrackID, fCSeq, fSessionID.Ptr, inClientRTPid, inClientRTCPid,fSetupHeaders, fUserAgent);
    }

    return this->DoTransaction();

}

OS_Error RTSPClient::SendPlay(UInt32 inStartPlayTimeInSec, Float32 inSpeed)
{
    char speedBuf[128];
    speedBuf[0] = '\0';
    
    if (inSpeed != 1)
        qtss_sprintf(speedBuf, "Speed: %f5.2\r\n", inSpeed);
        
    if (!fTransactionStarted)
    {   qtss_sprintf(fMethod,"%s","PLAY");
        qtss_sprintf(fSendBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %lu\r\n%sRange: npt=%lu.0-\r\n%sx-prebuffer: maxtime=3.0\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fSessionID.Ptr, inStartPlayTimeInSec, speedBuf, fUserAgent);
        //qtss_sprintf(fSendBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %lu\r\n%sRange: npt=7.0-\r\n%sx-prebuffer: maxtime=3.0\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fSessionID.Ptr, speedBuf, fUserAgent);
    }
    return this->DoTransaction();
}

OS_Error RTSPClient::SendPacketRangePlay(char* inPacketRangeHeader, Float32 inSpeed)
{
    char speedBuf[128];
    speedBuf[0] = '\0';
    
    if (inSpeed != 1)
        qtss_sprintf(speedBuf, "Speed: %f5.2\r\n", inSpeed);
        
    if (!fTransactionStarted)
    {
        qtss_sprintf(fMethod,"%s","PLAY");
        qtss_sprintf(fSendBuffer, "PLAY %s RTSP/1.0\r\nCSeq: %lu\r\n%sx-Packet-Range: %s\r\n%sUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fSessionID.Ptr, inPacketRangeHeader, speedBuf, fUserAgent);
    }
    return this->DoTransaction();   
}

OS_Error RTSPClient::SendReceive(UInt32 inStartPlayTimeInSec)
{
    if (!fTransactionStarted)
    {
        qtss_sprintf(fMethod,"%s","RECORD");
        qtss_sprintf(fSendBuffer, "RECORD %s RTSP/1.0\r\nCSeq: %lu\r\n%sRange: npt=%lu.0-\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fSessionID.Ptr, inStartPlayTimeInSec, fUserAgent);
    }
    return this->DoTransaction();
}

OS_Error RTSPClient::SendAnnounce(char *sdp)
{
//ANNOUNCE rtsp://server.example.com/permanent_broadcasts/TestBroadcast.sdp RTSP/1.0
    if (!fTransactionStarted)
    {   
        qtss_sprintf(fMethod,"%s","ANNOUNCE");
        if (sdp == NULL)
            qtss_sprintf(fSendBuffer, "ANNOUNCE %s RTSP/1.0\r\nCSeq: %lu\r\nAccept: application/sdp\r\nUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fUserAgent);
        else
        {   UInt32 len = strlen(sdp);
            if(len > kReqBufSize)
                return OS_NotEnoughSpace;
            qtss_sprintf(fSendBuffer, "ANNOUNCE %s RTSP/1.0\r\nCSeq: %lu\r\nContent-Type: application/sdp\r\nUser-agent: %s\r\nContent-Length: %lu\r\n\r\n%s", fURL.Ptr, fCSeq, fUserAgent, len, sdp);
        }   
    }
    return this->DoTransaction();
}

OS_Error RTSPClient::SendRTSPRequest(iovec* inRequest, UInt32 inNumVecs)
{
    if (!fTransactionStarted)
    {
        UInt32 curOffset = 0;
        for (UInt32 x = 0; x < inNumVecs; x++)
        {
            ::memcpy(fSendBuffer + curOffset, inRequest[x].iov_base, inRequest[x].iov_len);
            curOffset += inRequest[x].iov_len;
        }
        Assert(kReqBufSize > curOffset);
        fSendBuffer[curOffset] = '\0';
    }
    return this->DoTransaction();
}

OS_Error RTSPClient::PutMediaPacket(UInt32 inTrackID, Bool16 isRTCP, char* inData, UInt16 inLen)
{
    // Find the right channel number for this trackID
    for (int x = 0; x < fNumChannelElements; x++)
    {
        if ((fChannelTrackMap[x].fTrackID == inTrackID) && (fChannelTrackMap[x].fIsRTCP == isRTCP))
        {
            char header[5];
            header[0] = '$';
            header[1] = (UInt8)x;
            UInt16* theLenP = (UInt16*)header;
            theLenP[1] = htons(inLen);
                    
            //
            // Build the iovec
            iovec ioVec[2];
            ioVec[0].iov_len = 4;
            ioVec[0].iov_base = header;
            ioVec[1].iov_len = inLen;
            ioVec[1].iov_base = inData;
            
            //
            // Send it
            return fSocket->SendV(ioVec, 2);
        }
    }
    
    return OS_NoErr;
}


OS_Error RTSPClient::SendInterleavedWrite(UInt8 channel, UInt16 len, char*data,Bool16 *getNext)
{

    Assert(len < RTSPClient::kReqBufSize);
    
    iovec ioVEC[1];
    struct iovec* iov = &ioVEC[0];
    UInt16 interleavedLen =0;   
    UInt16 sendLen = 0;
    
    if (sInterleavedParams.extraLen > 0)
    {   *getNext = false; // can't handle new packet now. Send it again
        ioVEC[0].iov_base   = sInterleavedParams.extraBytes;
        ioVEC[0].iov_len    = sInterleavedParams.extraLen;
        sendLen = sInterleavedParams.extraLen;
    }
    else
    {   *getNext = true; // handle a new packet
        fSendBuffer[0] = '$';
        fSendBuffer[1] = channel;
        UInt16 netlen = htons(len);
        memcpy(&fSendBuffer[2],&netlen,2);
        memcpy(&fSendBuffer[4],data,len);
        
        interleavedLen = len+4;
        ioVEC[0].iov_base=&fSendBuffer[0];
        ioVEC[0].iov_len= interleavedLen;
        sendLen = interleavedLen;
        sInterleavedParams.extraChannel =channel;
    }   
        
    UInt32 outLenSent;
    OS_Error theErr = fSocket->GetSocket()->WriteV(iov, 1,&outLenSent);
    if (theErr != 0)
        outLenSent = 0;

    //qtss_printf("RTSPClient::SendInterleavedWrite Send channel=%u bufferlen=%u err=%ld outLenSent=%lu\n",(UInt16) extraChannel, sendLen,theErr,outLenSent);
    if (theErr == 0 && outLenSent != sendLen) 
    {   if (sInterleavedParams.extraLen > 0) // sending extra len so keep sending it.
        {   //qtss_printf("RTSPClient::SendInterleavedWrite partial Send channel=%u bufferlen=%u err=%ld amountSent=%lu \n",(UInt16) extraChannel,sendLen,theErr,outLenSent);
            sInterleavedParams.extraLen = sendLen - outLenSent;
            sInterleavedParams.extraByteOffset += outLenSent;
            sInterleavedParams.extraBytes = &fSendBuffer[sInterleavedParams.extraByteOffset];
        }
        else // we were sending a new packet so record the data
        {   //qtss_printf("RTSPClient::SendInterleavedWrite partial Send channel=%u bufferlen=%u err=%ld amountSent=%lu \n",(UInt16) channel,sendLen,theErr,outLenSent);
            sInterleavedParams.extraBytes = &fSendBuffer[outLenSent];
            sInterleavedParams.extraLen = sendLen - outLenSent;
            sInterleavedParams.extraChannel = channel;
            sInterleavedParams.extraByteOffset = outLenSent;
        }
    }
    else // either an error occured or we sent everything ok
    {
        if (theErr == 0)
        {   
            if (sInterleavedParams.extraLen > 0) // we were busy sending some old data and it all got sent
            {   //qtss_printf("RTSPClient::SendInterleavedWrite FULL Send channel=%u bufferlen=%u err=%ld amountSent=%lu \n",(UInt16) extraChannel,sendLen,theErr,outLenSent);
            }
            else 
            {   // it all worked so ask for more data
                //qtss_printf("RTSPClient::SendInterleavedWrite FULL Send channel=%u bufferlen=%u err=%ld amountSent=%lu \n",(UInt16) channel,sendLen,theErr,outLenSent);
            }
            sInterleavedParams.extraLen = 0;
            sInterleavedParams.extraBytes = NULL;
            sInterleavedParams.extraByteOffset = 0;
        }
        else // we got an error so nothing was sent
        {   //qtss_printf("RTSPClient::SendInterleavedWrite Send ERR sending=%ld \n",theErr);

            if (sInterleavedParams.extraLen == 0) // retry the new packet
            {   
                sInterleavedParams.extraBytes = &fSendBuffer[0];
                sInterleavedParams.extraLen = sendLen;
                sInterleavedParams.extraChannel = channel;              
                sInterleavedParams.extraByteOffset = 0;
            }       
        }
    }   
    return theErr;          
}

OS_Error RTSPClient::SendTeardown()
{
    if (!fTransactionStarted)
    {   qtss_sprintf(fMethod,"%s","TEARDOWN");
        qtss_sprintf(fSendBuffer, "TEARDOWN %s RTSP/1.0\r\nCSeq: %lu\r\n%sUser-agent: %s\r\n\r\n", fURL.Ptr, fCSeq, fSessionID.Ptr, fUserAgent);
    }
    return this->DoTransaction();
}


OS_Error    RTSPClient::GetMediaPacket(UInt32* outTrackID, Bool16* outIsRTCP, char** outBuffer, UInt32* outLen)
{
    static const UInt32 kPacketHeaderLen = 4;
    static const UInt32 kMaxPacketSize = 4096;
    
    // We need to buffer until we get a full packet.
    if (fPacketBuffer == NULL)
        fPacketBuffer = NEW char[kMaxPacketSize];
        
    if (fPacketOutstanding)
    {
        // The previous call to this function returned a packet successfully. We've been holding
        // onto that packet data until now... Now we can blow it away.
        UInt16* thePacketLenP = (UInt16*)fPacketBuffer;
        UInt16 thePacketLen = ntohs(thePacketLenP[1]);
        
        Assert(fPacketBuffer[0] == '$');

        // Move the leftover data (part of the next packet) to the beginning of the buffer
        Assert(fPacketBufferOffset >= (thePacketLen + kPacketHeaderLen));
        fPacketBufferOffset -= thePacketLen + kPacketHeaderLen;
        ::memmove(fPacketBuffer, &fPacketBuffer[thePacketLen + kPacketHeaderLen], fPacketBufferOffset);
#if DEBUG
        if (fPacketBufferOffset > 0)
        {
            Assert(fPacketBuffer[0] == '$');
        }
#endif
        
        fPacketOutstanding = false;
    }
    
    if (fPacketDataInHeaderBufferLen > 0)
    {
        //
        // If there is some packet data in the header buffer, clear it out
        //qtss_printf("%d bytes of packet data in header buffer\n",fPacketDataInHeaderBufferLen);
        
        Assert(fPacketDataInHeaderBuffer[0] == '$');
        Assert(fPacketDataInHeaderBufferLen < (kMaxPacketSize - fPacketBufferOffset));
        ::memcpy(&fPacketBuffer[fPacketBufferOffset], fPacketDataInHeaderBuffer, fPacketDataInHeaderBufferLen);
        fPacketBufferOffset += fPacketDataInHeaderBufferLen;
        fPacketDataInHeaderBufferLen = 0;
    }

    Assert(fPacketBufferOffset < kMaxPacketSize);
    UInt32 theRecvLen = 0;
    OS_Error theErr = fSocket->Read(&fPacketBuffer[fPacketBufferOffset], kMaxPacketSize - fPacketBufferOffset, &theRecvLen);
    if (theErr != OS_NoErr)
        return theErr;

    fPacketBufferOffset += theRecvLen;
    Assert(fPacketBufferOffset <= kMaxPacketSize);

    if (fPacketBufferOffset > kPacketHeaderLen)
    {
        Assert(fPacketBuffer[0] == '$');
        UInt16* thePacketLenP = (UInt16*)fPacketBuffer;
        UInt16 thePacketLen = ntohs(thePacketLenP[1]);
        
        if (fPacketBufferOffset >= (thePacketLen + kPacketHeaderLen))
        {
            // We have a complete packet. Return it to the caller.
            Assert(fPacketBuffer[1] < fNumChannelElements); // This is really not a safe assert, but anyway...
            *outTrackID = fChannelTrackMap[fPacketBuffer[1]].fTrackID;
            *outIsRTCP = fChannelTrackMap[fPacketBuffer[1]].fIsRTCP;
            *outLen = thePacketLen;
            
            // Next time we call this function, we will blow away the packet, but until then
            // we leave it untouched.
            fPacketOutstanding = true;
            *outBuffer = &fPacketBuffer[kPacketHeaderLen];
#if DEBUG
            fIsFirstPacket = false;
#endif
            return OS_NoErr;
        }
    }
    return OS_NoErr;
}

UInt32  RTSPClient::GetSSRCByTrack(UInt32 inTrackID)
{
    for (UInt32 x = 0; x < fNumSSRCElements; x++)
    {
        if (inTrackID == fSSRCMap[x].fTrackID)
            return fSSRCMap[x].fSSRC;
    }
    return 0;
}

RTPMetaInfoPacket::FieldID* RTSPClient::GetFieldIDArrayByTrack(UInt32 inTrackID)
{
    for (UInt32 x = 0; x < fNumFieldIDElements; x++)
    {
        if (inTrackID == fFieldIDMap[x].fTrackID)
            return fFieldIDMap[x].fFieldIDs;
    }
    return NULL;
}


OS_Error RTSPClient::DoTransaction()
{

    OS_Error theErr = OS_NoErr;
    Bool16 isAuthenticated = false;
    
    fSendBufferLen = ::strlen(fSendBuffer);
    StrPtrLen theRequest(fSendBuffer,fSendBufferLen);
    StrPtrLen theMethod(fMethod);
    
    if (fAuthenticator != NULL && !fTransactionStarted) 
    {   
        fAuthenticator->SetMethod(&theMethod);
        fAuthenticator->AttachAuthParams(&theRequest);
        isAuthenticated = true;     
    }

    if (!fTransactionStarted)
    {   
        //
        // Make sure that if there is some packet data in the header buffer, we forget all about it.
        fPacketDataInHeaderBufferLen = 0;
        
        fCSeq++;
    }
    
    if (!fReceiveInProgress)
    {   fResponseCount = 0;
        StrPtrLen theTransaction;
        fTransactionStarted = true;
        if (!fHaveTransactionBuffer)
        {   fHaveTransactionBuffer = true;
            theTransaction.Set(theRequest.Ptr,theRequest.Len);
        }
        theErr = fSocket->Send(theTransaction.Ptr, theTransaction.Len);
        
        if (fVerbose)
            qtss_printf("\n-----REQUEST-----len=%lu\n%s\n", theRequest.Len, STRTOCHAR(&theRequest));
        //qtss_printf("RTSPClient::DoTransaction Send len=%lu err = %ld\n",theTransaction.Len, theErr);
        
        if (theErr != OS_NoErr)
            return theErr;
            
        fHaveTransactionBuffer = false;
        

        // Done sending request, we're moving onto receiving the response
        fConnected = true;
        fContentRecvLen = 0;
        fHeaderRecvLen = 0;
        fReceivedResponse = false;
        fReceiveInProgress = true;
        memset(fRecvHeaderBuffer,0,kReqBufSize+1);

    }
    
    if (fReceiveInProgress)
    {
        Assert(theErr == OS_NoErr);
        theErr = this->ReceiveResponse();
        //qtss_printf("RTSPClient::DoTransaction ReceiveResponse fStatus=%lu len=%lu err = %ld\n",fStatus, fHeaderRecvLen, theErr);
    
        if (theErr != OS_NoErr)
            return theErr;

        fReceiveInProgress = false;
    }
    

    if (!fReceiveInProgress && (401 == fStatus) && (fAuthenticator != NULL) ) // authentication required or authentication failed
    {
        fResponseCount ++;
        if (fResponseCount != 1) // just try to authenticate once against a 401.
            return 0;
        fAuthenticator->RemoveAuthLine(&theRequest); // reset to original request
        fAuthenticator->ResetAuthParams(); // if we had a 401 on an authenticated request clean up old params and try again with the new response data
        fAuthenticator->SetName(&fName);
        fAuthenticator->SetPassword(&fPassword);
        fAuthenticator->SetMethod(&theMethod);
        fAuthenticator->SetURI(&fURL);
        fAuthenticator->AttachAuthParams(&theRequest);

        if (fVerbose)
            qtss_printf("\n-----REQUEST----\n%s\n", STRTOCHAR(&theRequest));
        
        
        StrPtrLen theTransaction;
        if (!fHaveTransactionBuffer)
        {   fHaveTransactionBuffer = true;
            theTransaction.Set(theRequest.Ptr,theRequest.Len);
        }

        theErr = fSocket->Send(theTransaction.Ptr, theTransaction.Len);
        //qtss_printf("fSocket->Send err =%ld len=%lu\n",theErr, theTransaction.Len);
        if (theErr != OS_NoErr)
            return theErr;

        fHaveTransactionBuffer = false; 
        
        fContentRecvLen = 0;
        fHeaderRecvLen = 0;
        fReceivedResponse = false;
        fRecvHeaderBuffer[0] = 0;
        fHeaderLen = 0;
        memset(fRecvHeaderBuffer,0,kReqBufSize+1);
        
        fReceiveInProgress = true;
        fTransactionStarted = true;
        return EAGAIN;
    }
    
    
    return theErr;
}



OS_Error RTSPClient::ReceiveResponse()
{
    OS_Error theErr = OS_NoErr;

    while (!fReceivedResponse)
    {
        UInt32 theRecvLen = 0;
        //fRecvHeaderBuffer[0] = 0;
        theErr = fSocket->Read(&fRecvHeaderBuffer[fHeaderRecvLen], kReqBufSize - fHeaderRecvLen, &theRecvLen);
        if (theErr != OS_NoErr)
            return theErr;
        
        fHeaderRecvLen += theRecvLen;
        fRecvHeaderBuffer[fHeaderRecvLen] = 0;
        if (fVerbose)
            qtss_printf("\n-----RESPONSE----\n%s\n", fRecvHeaderBuffer);

        //fRecvHeaderBuffer[fHeaderRecvLen] = '\0';
        // Check to see if we've gotten a complete header, and if the header has even started       
        // The response may not start with the response if we are interleaving media data,
        // in which case there may be leftover media data in the stream. If we encounter any
        // of this cruft, we can just strip it off.
        char* theHeaderStart = ::strstr(fRecvHeaderBuffer, "RTSP");
        if (theHeaderStart == NULL)
        {
            fHeaderRecvLen = 0;
            continue;
        }
        else if (theHeaderStart != fRecvHeaderBuffer)
        {
            fHeaderRecvLen -= theHeaderStart - fRecvHeaderBuffer;
            ::memmove(fRecvHeaderBuffer, theHeaderStart, fHeaderRecvLen);
            //fRecvHeaderBuffer[fHeaderRecvLen] = '\0';
        }
        //qtss_printf("RTSPClient::ReceiveResponse fRecvHeaderBuffer=%s\n",fRecvHeaderBuffer);
        char* theResponseData = ::strstr(fRecvHeaderBuffer, "\r\n\r\n");    

        if (theResponseData != NULL)
        {               
            // skip past the \r\n\r\n
            theResponseData += 4;
            
            // We've got a new response
            fReceivedResponse = true;
            
            // Figure out how much of the content body we've already received
            // in the header buffer. If we are interleaving, this may also be packet data
            fHeaderLen = theResponseData - &fRecvHeaderBuffer[0];
            fContentRecvLen = fHeaderRecvLen - fHeaderLen;

            // Zero out fields that will change with every RTSP response
            fServerPort = 0;
            fStatus = 0;
            fContentLength = 0;
        
            // Parse the response.
            StrPtrLen theData(fRecvHeaderBuffer, (theResponseData - (&fRecvHeaderBuffer[0])));
            StringParser theParser(&theData);
            
            theParser.ConsumeLength(NULL, 9); //skip past RTSP/1.0
            fStatus = theParser.ConsumeInteger(NULL);
            
            StrPtrLen theLastHeader;
            while (theParser.GetDataRemaining() > 0)
            {
                static StrPtrLen sSessionHeader("Session");
                static StrPtrLen sContentLenHeader("Content-length");
                static StrPtrLen sTransportHeader("Transport");
                static StrPtrLen sRTPInfoHeader("RTP-Info");
                static StrPtrLen sRTPMetaInfoHeader("x-RTP-Meta-Info");
                static StrPtrLen sAuthenticateHeader("WWW-Authenticate");
                static StrPtrLen sSameAsLastHeader(" ,");
                
                StrPtrLen theKey;
                theParser.GetThruEOL(&theKey);
                
                if (theKey.NumEqualIgnoreCase(sSessionHeader.Ptr, sSessionHeader.Len))
                {
                    if (fSessionID.Len == 0)
                    {
                        // Copy the session ID and store it.
                        // First figure out how big the session ID is. We copy
                        // everything up until the first ';' returned from the server
                        UInt32 keyLen = 0;
                        while ((theKey.Ptr[keyLen] != ';') && (theKey.Ptr[keyLen] != '\r') && (theKey.Ptr[keyLen] != '\n'))
                            keyLen++;
                        
                        // Append an EOL so we can stick this thing transparently into the SETUP request
                        
                        fSessionID.Ptr = NEW char[keyLen + 3];
                        fSessionID.Len = keyLen + 2;
                        ::memcpy(fSessionID.Ptr, theKey.Ptr, keyLen);
                        ::memcpy(fSessionID.Ptr + keyLen, "\r\n", 2);//Append a EOL
                        fSessionID.Ptr[keyLen + 2] = '\0';
                    }
                }
                else if (theKey.NumEqualIgnoreCase(sContentLenHeader.Ptr, sContentLenHeader.Len))
                {
                    StringParser theCLengthParser(&theKey);
                    theCLengthParser.ConsumeUntil(NULL, StringParser::sDigitMask);
                    fContentLength = theCLengthParser.ConsumeInteger(NULL);
                    
                    delete [] fRecvContentBuffer;
                    fRecvContentBuffer = NEW char[fContentLength + 1];
                    
                    // Immediately copy the bit of the content body that we've already
                    // read off of the socket.
                    ::memcpy(fRecvContentBuffer, theResponseData, fContentRecvLen);
                    
                }
                else if (theKey.NumEqualIgnoreCase(sAuthenticateHeader.Ptr, sAuthenticateHeader.Len))
                {   
                    if (fVerbose)
                        qtss_printf("\n--CHALLENGE RECEIVED\n");
                    #if ENABLE_AUTHENTICATION   
                        if (fAuthenticator != NULL) // already have an authenticator
                            delete fAuthenticator;
            
                        fAuthenticator = fAuthenticationParser.ParseChallenge(&theKey);
                        Assert(fAuthenticator != NULL);
                        if (!fAuthenticator) 
                            return 401; // what to do? the challenge is bad can't authenticate.
                        else if (fVerbose)
                        {   if (fAuthenticator->GetType() == Authenticator::kBasicType)
                                qtss_printf("--CREATED BASIC AUTHENTICATOR\n");
                            else if (fAuthenticator->GetType() == Authenticator::kDigestType)
                                qtss_printf("--CREATED DIGEST AUTHENTICATOR\n");
                        }
                    #else
                        if (fVerbose) 
                            qtss_printf("--AUTHENTICATION IS DISABLED\n");
                    #endif                  
                }
                else if (theKey.NumEqualIgnoreCase(sTransportHeader.Ptr, sTransportHeader.Len))
                {
                    StringParser theTransportParser(&theKey);
                    StrPtrLen theSubHeader;

                    while (theTransportParser.GetDataRemaining() > 0)
                    {
                        static StrPtrLen sServerPort("server_port");
                        static StrPtrLen sInterleaved("interleaved");

                        theTransportParser.GetThru(&theSubHeader, ';');
                        if (theSubHeader.NumEqualIgnoreCase(sServerPort.Ptr, sServerPort.Len))
                        {
                            StringParser thePortParser(&theSubHeader);
                            thePortParser.ConsumeUntil(NULL, StringParser::sDigitMask);
                            fServerPort = (UInt16) thePortParser.ConsumeInteger(NULL);
                        }
                        else if (theSubHeader.NumEqualIgnoreCase(sInterleaved.Ptr, sInterleaved.Len))
                            this->ParseInterleaveSubHeader(&theSubHeader);                          
                    }
                }
                else if (theKey.NumEqualIgnoreCase(sRTPInfoHeader.Ptr, sRTPInfoHeader.Len))
                    ParseRTPInfoHeader(&theKey);
                else if (theKey.NumEqualIgnoreCase(sRTPMetaInfoHeader.Ptr, sRTPMetaInfoHeader.Len))
                    ParseRTPMetaInfoHeader(&theKey);
                else if (theKey.NumEqualIgnoreCase(sSameAsLastHeader.Ptr, sSameAsLastHeader.Len))
                {
                    //
                    // If the last header was an RTP-Info header
                    if (theLastHeader.NumEqualIgnoreCase(sRTPInfoHeader.Ptr, sRTPInfoHeader.Len))
                        ParseRTPInfoHeader(&theKey);
                }
                theLastHeader = theKey;
            }
            
            //
            // Check to see if there is any packet data in the header buffer
            if (fContentRecvLen > fContentLength)
            {
                fPacketDataInHeaderBuffer = theResponseData + fContentLength;
                fPacketDataInHeaderBufferLen = fContentRecvLen - fContentLength;
            }
        }
        else if (fHeaderRecvLen == kReqBufSize)
            return ENOBUFS; // This response is too big for us to handle!
    }
    
    while (fContentLength > fContentRecvLen)
    {
        UInt32 theContentRecvLen = 0;
        theErr = fSocket->Read(&fRecvContentBuffer[fContentRecvLen], fContentLength - fContentRecvLen, &theContentRecvLen);
        if (theErr != OS_NoErr)
        {
            fEventMask = EV_RE;
            return theErr;
        }
        fContentRecvLen += theContentRecvLen;       
    }
    
    // We're all done, reset all our state information.
    fReceivedResponse = false;
    fReceiveInProgress = false;
    fTransactionStarted = false;

    return OS_NoErr;
}

void    RTSPClient::ParseRTPInfoHeader(StrPtrLen* inHeader)
{
    static StrPtrLen sURL("url");
    StringParser theParser(inHeader);
    theParser.ConsumeUntil(NULL, 'u'); // consume until "url"
    
    if (fNumSSRCElements == fSSRCMapSize)
    {
        SSRCMapElem* theNewMap = NEW SSRCMapElem[fSSRCMapSize * 2];
        ::memset(theNewMap, 0, sizeof(SSRCMapElem) * (fSSRCMapSize * 2));
        ::memcpy(theNewMap, fSSRCMap, sizeof(SSRCMapElem) * fNumSSRCElements);
        fSSRCMapSize *= 2;
        delete [] fSSRCMap;
        fSSRCMap = theNewMap;
    }   
    
    fSSRCMap[fNumSSRCElements].fTrackID = 0;
    fSSRCMap[fNumSSRCElements].fSSRC = 0;
    
    // Parse out the trackID & the SSRC
    StrPtrLen theRTPInfoSubHeader;
    (void)theParser.GetThru(&theRTPInfoSubHeader, ';');
    
    while (theRTPInfoSubHeader.Len > 0)
    {
        static StrPtrLen sURLSubHeader("url");
        static StrPtrLen sSSRCSubHeader("ssrc");
        
        if (theRTPInfoSubHeader.NumEqualIgnoreCase(sURLSubHeader.Ptr, sURLSubHeader.Len))
        {
            StringParser theURLParser(&theRTPInfoSubHeader);
            theURLParser.ConsumeUntil(NULL, StringParser::sDigitMask);
            fSSRCMap[fNumSSRCElements].fTrackID = theURLParser.ConsumeInteger(NULL);
        }
        else if (theRTPInfoSubHeader.NumEqualIgnoreCase(sSSRCSubHeader.Ptr, sSSRCSubHeader.Len))
        {
            StringParser theURLParser(&theRTPInfoSubHeader);
            theURLParser.ConsumeUntil(NULL, StringParser::sDigitMask);
            fSSRCMap[fNumSSRCElements].fSSRC = theURLParser.ConsumeInteger(NULL);
        }

        // Move onto the next parameter
        (void)theParser.GetThru(&theRTPInfoSubHeader, ';');
    }
    
    fNumSSRCElements++;
}

void    RTSPClient::ParseRTPMetaInfoHeader(StrPtrLen* inHeader)
{
    //
    // Reallocate the array if necessary
    if (fNumFieldIDElements == fFieldIDMapSize)
    {
        FieldIDArrayElem* theNewMap = NEW FieldIDArrayElem[fFieldIDMapSize * 2];
        ::memset(theNewMap, 0, sizeof(FieldIDArrayElem) * (fFieldIDMapSize * 2));
        ::memcpy(theNewMap, fFieldIDMap, sizeof(FieldIDArrayElem) * fNumFieldIDElements);
        fFieldIDMapSize *= 2;
        delete [] fFieldIDMap;
        fFieldIDMap = theNewMap;
    }
    
    //
    // Build the FieldIDArray for this track
    RTPMetaInfoPacket::ConstructFieldIDArrayFromHeader(inHeader, fFieldIDMap[fNumFieldIDElements].fFieldIDs);
    fFieldIDMap[fNumFieldIDElements].fTrackID = fSetupTrackID;
    fNumFieldIDElements++;
}

void    RTSPClient::ParseInterleaveSubHeader(StrPtrLen* inSubHeader)
{
    StringParser theChannelParser(inSubHeader);
    
    // Parse out the channel numbers
    theChannelParser.ConsumeUntil(NULL, StringParser::sDigitMask);
    UInt8 theRTPChannel = (UInt8) theChannelParser.ConsumeInteger(NULL);
    theChannelParser.ConsumeLength(NULL, 1);
    UInt8 theRTCPChannel = (UInt8) theChannelParser.ConsumeInteger(NULL);
    
    UInt8 theMaxChannel = theRTCPChannel;
    if (theRTPChannel > theMaxChannel)
        theMaxChannel = theRTPChannel;
    
    // Reallocate the channel-track array if it is too little
    if (theMaxChannel >= fNumChannelElements)
    {
        ChannelMapElem* theNewMap = NEW ChannelMapElem[theMaxChannel + 1];
        ::memset(theNewMap, 0, sizeof(ChannelMapElem) * (theMaxChannel + 1));
        ::memcpy(theNewMap, fChannelTrackMap, sizeof(ChannelMapElem) * fNumChannelElements);
        fNumChannelElements = theMaxChannel + 1;
        delete [] fChannelTrackMap;
        fChannelTrackMap = theNewMap;
    }
    
    // Copy the relevent information into the channel-track array.
    fChannelTrackMap[theRTPChannel].fTrackID = fSetupTrackID;
    fChannelTrackMap[theRTPChannel].fIsRTCP = false;
    fChannelTrackMap[theRTCPChannel].fTrackID = fSetupTrackID;
    fChannelTrackMap[theRTCPChannel].fIsRTCP = true;
}

#define _RTSPCLIENTTESTING_ 0

#include "SocketUtils.h"
#include "OS.h"

#if _RTSPCLIENTTESTING_
int main(int argc, char * argv[]) 
{
    OS::Initialize();
    Socket::Initialize();
    SocketUtils::Initialize();

    UInt32 ipAddr = (UInt32)ntohl(::inet_addr("17.221.41.111"));
    UInt16 port = 554;
    StrPtrLen theURL("rtsp://17.221.41.111/mystery.mov");

    RTSPClient theClient(Socket::kNonBlockingSocketType);
    theClient.Set(ipAddr, port, theURL);
    
    OS_Error theErr = EINVAL;
    while (theErr != OS_NoErr)
    {
        theErr = theClient.SendDescribe();
        sleep(1);
    }
    Assert(theClient.GetStatus() == 200);
    theErr = EINVAL;
    while (theErr != OS_NoErr)
    {
        theErr = theClient.SendSetup(3, 6790);
        sleep(1);
    }
    //qtss_printf("Server port: %d\n", theClient.GetServerPort());
    Assert(theClient.GetStatus() == 200);
    theErr = EINVAL;
    while (theErr != OS_NoErr)
    {
        theErr = theClient.SendSetup(4, 6792);
        sleep(1);
    }
    //qtss_printf("Server port: %d\n", theClient.GetServerPort());
    Assert(theClient.GetStatus() == 200);
    theErr = EINVAL;
    while (theErr != OS_NoErr)
    {
        theErr = theClient.SendPlay();
        sleep(1);
    }
    Assert(theClient.GetStatus() == 200);
    theErr = EINVAL;
    while (theErr != OS_NoErr)
    {
        theErr = theClient.SendTeardown();
        sleep(1);
    }
    Assert(theClient.GetStatus() == 200);
}
#endif
