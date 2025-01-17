#include "QTSSModuleUtils.h"
#include "QTSS_Private.h"

#include "StrPtrLen.h"
#include "OSArrayObjectDeleter.h"
#include "OSMemory.h"
#include "MyAssert.h"
#include "StringFormatter.h"
#include "ResizeableStringFormatter.h"
#include "QTAccessFile.h"
#include "StringParser.h"
#include "SafeStdLib.h"
#ifndef __Win32__
#include <netinet/in.h>
#endif

#ifdef __solaris__
#include <limits.h>
#endif

QTSS_TextMessagesObject     QTSSModuleUtils::sMessages = NULL;
QTSS_ServerObject           QTSSModuleUtils::sServer = NULL;
QTSS_StreamRef              QTSSModuleUtils::sErrorLog = NULL;
Bool16                      QTSSModuleUtils::sEnableRTSPErrorMsg = false;
QTSS_ErrorVerbosity         QTSSModuleUtils::sMissingPrefVerbosity = qtssMessageVerbosity;

void    QTSSModuleUtils::Initialize(QTSS_TextMessagesObject inMessages,
                                    QTSS_ServerObject inServer,
                                    QTSS_StreamRef inErrorLog)
{
    sMessages = inMessages;
    sServer = inServer;
    sErrorLog = inErrorLog;
}

QTSS_Error QTSSModuleUtils::ReadEntireFile(char* inPath, StrPtrLen* outData, QTSS_TimeVal inModDate, QTSS_TimeVal* outModDate)
{   
    
    QTSS_Object theFileObject = NULL;
    QTSS_Error theErr = QTSS_NoErr;
    
    outData->Ptr = NULL;
    outData->Len = 0;
    
    do { 

        // Use the QTSS file system API to read the file
        theErr = QTSS_OpenFileObject(inPath, 0, &theFileObject);
        if (theErr != QTSS_NoErr)
            break;
    
        UInt32 theParamLen = 0;
        QTSS_TimeVal* theModDate = NULL;
        theErr = QTSS_GetValuePtr(theFileObject, qtssFlObjModDate, 0, (void**)&theModDate, &theParamLen);
        Assert(theParamLen == sizeof(QTSS_TimeVal));
        if(theParamLen != sizeof(QTSS_TimeVal))
            break;
        if(outModDate != NULL)
            *outModDate = (QTSS_TimeVal)*theModDate;

        if(inModDate != -1) {   
            // If file hasn't been modified since inModDate, don't have to read the file
            if(*theModDate <= inModDate)
                break;
        }
        
        theParamLen = 0;
        UInt64* theLength = NULL;
        theErr = QTSS_GetValuePtr(theFileObject, qtssFlObjLength, 0, (void**)&theLength, &theParamLen);
        if (theParamLen != sizeof(UInt64))
            break;
        
		if (*theLength > kSInt32_Max)
			break;

        // Allocate memory for the file data
        outData->Ptr = NEW char[ (SInt32) (*theLength + 1) ];
        outData->Len = (SInt32) *theLength;
        outData->Ptr[outData->Len] = 0;
    
        // Read the data
        UInt32 recvLen = 0;
        theErr = QTSS_Read(theFileObject, outData->Ptr, outData->Len, &recvLen);
        if (theErr != QTSS_NoErr)
        {
            outData->Delete();
            break;
        }   
        Assert(outData->Len == recvLen);
    
    }while(false);
    
    // Close the file
    if(theFileObject != NULL) {
        theErr = QTSS_CloseFileObject(theFileObject);
    }
    
    return theErr;
}

void    QTSSModuleUtils::SetupSupportedMethods(QTSS_Object inServer, QTSS_RTSPMethod* inMethodArray, UInt32 inNumMethods)
{
    // Report to the server that this module handles DESCRIBE, SETUP, PLAY, PAUSE, and TEARDOWN
    UInt32 theNumMethods = 0;
    (void)QTSS_GetNumValues(inServer, qtssSvrHandledMethods, &theNumMethods);
    
    for (UInt32 x = 0; x < inNumMethods; x++)
        (void)QTSS_SetValue(inServer, qtssSvrHandledMethods, theNumMethods++, (void*)&inMethodArray[x], sizeof(inMethodArray[x]));
}

void    QTSSModuleUtils::LogError(  QTSS_ErrorVerbosity inVerbosity,
                                    QTSS_AttributeID inTextMessage,
                                    UInt32 /*inErrNumber*/,
                                    char* inArgument,
                                    char* inArg2)
{
    static char* sEmptyArg = "";
    
    if (sMessages == NULL)
        return;
        
    // Retrieve the specified text message from the text messages dictionary.
    
    StrPtrLen theMessage;
    (void)QTSS_GetValuePtr(sMessages, inTextMessage, 0, (void**)&theMessage.Ptr, &theMessage.Len);
    if ((theMessage.Ptr == NULL) || (theMessage.Len == 0))
        (void)QTSS_GetValuePtr(sMessages, qtssMsgNoMessage, 0, (void**)&theMessage.Ptr, &theMessage.Len);

    if ((theMessage.Ptr == NULL) || (theMessage.Len == 0))
        return;
    
    // qtss_sprintf and ::strlen will crash if inArgument is NULL
    if (inArgument == NULL)
        inArgument = sEmptyArg;
    if (inArg2 == NULL)
        inArg2 = sEmptyArg;
    
    // Create a new string, and put the argument into the new string.
    
    UInt32 theMessageLen = theMessage.Len + ::strlen(inArgument) + ::strlen(inArg2);

    OSCharArrayDeleter theLogString(NEW char[theMessageLen + 1]);
    qtss_sprintf(theLogString.GetObject(), theMessage.Ptr, inArgument, inArg2);
    Assert(theMessageLen >= ::strlen(theLogString.GetObject()));
    
    (void)QTSS_Write(sErrorLog, theLogString.GetObject(), ::strlen(theLogString.GetObject()),
                        NULL, inVerbosity);
}

void QTSSModuleUtils::LogErrorStr( QTSS_ErrorVerbosity inVerbosity, char* inMessage) 
{  	
	if (inMessage == NULL) return;  
	(void)QTSS_Write(sErrorLog, inMessage, ::strlen(inMessage), NULL, inVerbosity);
}

                        
char* QTSSModuleUtils::GetFullPath( QTSS_RTSPRequestObject inRequest,
                                    QTSS_AttributeID whichFileType,
                                    UInt32* outLen,
                                    StrPtrLen* suffix)
{
    Assert(outLen != NULL);
    
	(void)QTSS_LockObject(inRequest);

    StrPtrLen theFilePath;
    (void)QTSS_GetValuePtr(inRequest, whichFileType, 0, (void**)&theFilePath.Ptr, &theFilePath.Len);
		
    StrPtrLen theRootDir;
    QTSS_Error theErr = QTSS_GetValuePtr(inRequest, qtssRTSPReqRootDir, 0, (void**)&theRootDir.Ptr, &theRootDir.Len);
	Assert(theErr == QTSS_NoErr);
    
	if (theRootDir.Len && theRootDir.Ptr[theRootDir.Len -1] == kPathDelimiterChar
	    && theFilePath.Len  && theFilePath.Ptr[0] == kPathDelimiterChar)
	{
	    char *thePathEnd = &(theFilePath.Ptr[theFilePath.Len]);
	    while (theFilePath.Ptr != thePathEnd)
	    {
	        if (*theFilePath.Ptr != kPathDelimiterChar)
	            break;
	            
	        theFilePath.Ptr ++;
	        theFilePath.Len --;
	    }
	}

    *outLen = theFilePath.Len + theRootDir.Len + 2;
    if (suffix != NULL)
        *outLen += suffix->Len;
    
    char* theFullPath = NEW char[*outLen];

    StringFormatter thePathFormatter(theFullPath, *outLen);
    thePathFormatter.Put(theRootDir);
    thePathFormatter.Put(theFilePath);
    if (suffix != NULL)
        thePathFormatter.Put(*suffix);
    thePathFormatter.PutTerminator();

    *outLen = *outLen - 2;
	
	(void)QTSS_UnlockObject(inRequest);
	
    return theFullPath;
}

QTSS_Error  QTSSModuleUtils::AppendRTPMetaInfoHeader(   QTSS_RTSPRequestObject inRequest,
                                                        StrPtrLen* inRTPMetaInfoHeader,
                                                        RTPMetaInfoPacket::FieldID* inFieldIDArray)
{

    char tempBuffer[128];
    ResizeableStringFormatter theFormatter(tempBuffer, 128);
    
    StrPtrLen theHeader(*inRTPMetaInfoHeader);
   
    Bool16 foundFieldArray[RTPMetaInfoPacket::kNumFields];
    ::memset(foundFieldArray, 0, sizeof(Bool16) * RTPMetaInfoPacket::kNumFields);
    
    char* theEndP = theHeader.Ptr + theHeader.Len;
    UInt16 fieldNameValue = 0;
    
    while (theHeader.Ptr <= (theEndP - sizeof(RTPMetaInfoPacket::FieldName)))
    {
        RTPMetaInfoPacket::FieldName* theFieldName = (RTPMetaInfoPacket::FieldName*)theHeader.Ptr;
        ::memcpy (&fieldNameValue, theFieldName, sizeof(UInt16));

        RTPMetaInfoPacket::FieldIndex theFieldIndex = RTPMetaInfoPacket::GetFieldIndexForName(ntohs(fieldNameValue));
  
        if ((theFieldIndex == RTPMetaInfoPacket::kIllegalField) ||
            (inFieldIDArray[theFieldIndex] == RTPMetaInfoPacket::kFieldNotUsed))
        {
            theHeader.Ptr += 3;
            continue;
        }
        
        foundFieldArray[theFieldIndex] = true;

        theFormatter.Put(theHeader.Ptr, sizeof(RTPMetaInfoPacket::FieldName));
        
        if (inFieldIDArray[theFieldIndex] != RTPMetaInfoPacket::kUncompressed)
        {

            theFormatter.PutChar('=');
            theFormatter.Put(inFieldIDArray[theFieldIndex]);
        }
        

        theFormatter.PutChar(';');
            

        theHeader.Ptr += 3;
    }


    for (UInt32 x = 0; x < RTPMetaInfoPacket::kNumFields; x++)
    {
        if (!foundFieldArray[x])
            inFieldIDArray[x] = RTPMetaInfoPacket::kFieldNotUsed;
    }

    if (theFormatter.GetCurrentOffset() == 0)
        return QTSS_ValueNotFound; // Not really the greatest error!

    return QTSS_AppendRTSPHeader(inRequest, qtssXRTPMetaInfoHeader, theFormatter.GetBufPtr(), theFormatter.GetCurrentOffset() - 1);
}

QTSS_Error  QTSSModuleUtils::SendErrorResponse( QTSS_RTSPRequestObject inRequest,
                                                        QTSS_RTSPStatusCode inStatusCode,
                                                        QTSS_AttributeID inTextMessage,
                                                        StrPtrLen* inStringArg)
{
    static Bool16 sFalse = false;
    
    //set RTSP headers necessary for this error response message
    (void)QTSS_SetValue(inRequest, qtssRTSPReqStatusCode, 0, &inStatusCode, sizeof(inStatusCode));
    (void)QTSS_SetValue(inRequest, qtssRTSPReqRespKeepAlive, 0, &sFalse, sizeof(sFalse));
    StringFormatter theErrorMsgFormatter(NULL, 0);
    char *messageBuffPtr = NULL;
    
    if (sEnableRTSPErrorMsg)
    {
        // Retrieve the specified message out of the text messages dictionary.
        StrPtrLen theMessage;
        (void)QTSS_GetValuePtr(sMessages, inTextMessage, 0, (void**)&theMessage.Ptr, &theMessage.Len);

        if ((theMessage.Ptr == NULL) || (theMessage.Len == 0))
        {

            (void)QTSS_GetValuePtr(sMessages, qtssMsgNoMessage, 0, (void**)&theMessage.Ptr, &theMessage.Len);
        }
        Assert(theMessage.Ptr != NULL);
        Assert(theMessage.Len > 0);

        UInt32 theMsgLen = 256;
        if (inStringArg != NULL)
            theMsgLen += inStringArg->Len;
        
        messageBuffPtr = NEW char[theMsgLen];
        messageBuffPtr[0] = 0;
        theErrorMsgFormatter.Set(messageBuffPtr, theMsgLen);

        char* stringLocation = ::strstr(theMessage.Ptr, "%s");
        if (stringLocation != NULL)
        {

            theErrorMsgFormatter.Put(theMessage.Ptr, stringLocation - theMessage.Ptr);
            
            if (inStringArg != NULL && inStringArg->Len > 0)
            {

                theErrorMsgFormatter.Put(inStringArg->Ptr, inStringArg->Len);
                stringLocation += 2;
            }

            theErrorMsgFormatter.Put(stringLocation, (theMessage.Ptr + theMessage.Len) - stringLocation);
        }
        else
            theErrorMsgFormatter.Put(theMessage);
        
        
        char buff[32];
        qtss_sprintf(buff,"%lu",theErrorMsgFormatter.GetBytesWritten());
        (void)QTSS_AppendRTSPHeader(inRequest, qtssContentLengthHeader, buff, ::strlen(buff));
    }
    
    (void)QTSS_SendRTSPHeaders(inRequest);

    (void)QTSS_Write(inRequest, theErrorMsgFormatter.GetBufPtr(), theErrorMsgFormatter.GetBytesWritten(), NULL, 0);
    (void)QTSS_SetValue(inRequest, qtssRTSPReqRespMsg, 0, theErrorMsgFormatter.GetBufPtr(), theErrorMsgFormatter.GetBytesWritten());
    
    delete [] messageBuffPtr;
    return QTSS_RequestFailed;
}

QTSS_Error	QTSSModuleUtils::SendErrorResponseWithMessage( QTSS_RTSPRequestObject inRequest,
														QTSS_RTSPStatusCode inStatusCode,
														StrPtrLen* inErrorMessagePtr)
{
    static Bool16 sFalse = false;
    
    //set RTSP headers necessary for this error response message
    (void)QTSS_SetValue(inRequest, qtssRTSPReqStatusCode, 0, &inStatusCode, sizeof(inStatusCode));
    (void)QTSS_SetValue(inRequest, qtssRTSPReqRespKeepAlive, 0, &sFalse, sizeof(sFalse));
    StrPtrLen theErrorMessage(NULL, 0);
    
    if (sEnableRTSPErrorMsg)
    {
		Assert(inErrorMessagePtr != NULL);
		theErrorMessage.Set(inErrorMessagePtr->Ptr, inErrorMessagePtr->Len);
		
        char buff[32];
        qtss_sprintf(buff,"%lu",inErrorMessagePtr->Len);
        (void)QTSS_AppendRTSPHeader(inRequest, qtssContentLengthHeader, buff, ::strlen(buff));
    }
    

    (void)QTSS_SendRTSPHeaders(inRequest);

    (void)QTSS_Write(inRequest, theErrorMessage.Ptr, theErrorMessage.Len, NULL, 0);
    (void)QTSS_SetValue(inRequest, qtssRTSPReqRespMsg, 0, theErrorMessage.Ptr, theErrorMessage.Len);
    
    return QTSS_RequestFailed;
}


QTSS_Error	QTSSModuleUtils::SendHTTPErrorResponse( QTSS_RTSPRequestObject inRequest,
													QTSS_SessionStatusCode inStatusCode,
                                                    Bool16 inKillSession,
                                                    char *errorMessage)
{
    static Bool16 sFalse = false;
    
    //set status code for access log
    (void)QTSS_SetValue(inRequest, qtssRTSPReqStatusCode, 0, &inStatusCode, sizeof(inStatusCode));

    if (inKillSession) // tell the server to end the session
        (void)QTSS_SetValue(inRequest, qtssRTSPReqRespKeepAlive, 0, &sFalse, sizeof(sFalse));
    
    ResizeableStringFormatter theErrorMessage(NULL, 0); //allocates and deletes memory
    ResizeableStringFormatter bodyMessage(NULL,0); //allocates and deletes memory

    char messageLineBuffer[64]; // used for each line
    static const int maxMessageBufferChars = sizeof(messageLineBuffer) -1;
    messageLineBuffer[maxMessageBufferChars] = 0; // guarantee termination

    char* errorMsg = "error"; 

    DateBuffer theDate;
    DateTranslator::UpdateDateBuffer(&theDate, 0); // get the current GMT date and time

    UInt32 realCode = 0;
    UInt32 len = sizeof(realCode);
    (void) QTSS_GetValue(inRequest, qtssRTSPReqRealStatusCode, 0,  (void*)&realCode,&len);

    char serverHeaderBuffer[64]; // the qtss Server: header field
    len = sizeof(serverHeaderBuffer) -1; // leave room for terminator
    (void) QTSS_GetValue(sServer, qtssSvrRTSPServerHeader, 0,  (void*)serverHeaderBuffer,&len);
    serverHeaderBuffer[len] = 0; // terminate.
 
    qtss_snprintf(messageLineBuffer,maxMessageBufferChars, "HTTP/1.1 %lu %s",realCode, errorMsg);
    theErrorMessage.Put(messageLineBuffer,::strlen(messageLineBuffer));
    theErrorMessage.PutEOL();

    theErrorMessage.Put(serverHeaderBuffer,::strlen(serverHeaderBuffer));
    theErrorMessage.PutEOL();
 
    qtss_snprintf(messageLineBuffer,maxMessageBufferChars, "Date: %s",theDate.GetDateBuffer());
    theErrorMessage.Put(messageLineBuffer,::strlen(messageLineBuffer));
    theErrorMessage.PutEOL();
 
    Bool16 addBody =  (errorMessage != NULL && ::strlen(errorMessage) != 0); // body error message so add body headers
    if (addBody) // body error message so add body headers
    {
        // first create the html body
        static const StrPtrLen htmlBodyStart("<html><body>\n");
        bodyMessage.Put(htmlBodyStart.Ptr,htmlBodyStart.Len);
 
        //<h1>errorMessage</h1>\n
        static const StrPtrLen hStart("<h1>");
        bodyMessage.Put(hStart.Ptr,hStart.Len);

        bodyMessage.Put(errorMessage,::strlen(errorMessage));

        static const StrPtrLen hTerm("</h1>\n");
        bodyMessage.Put(hTerm.Ptr,hTerm.Len);
 
        static const StrPtrLen htmlBodyTerm("</body></html>\n");
        bodyMessage.Put(htmlBodyTerm.Ptr,htmlBodyTerm.Len);

        // write body headers
        static const StrPtrLen bodyHeaderType("Content-Type: text/html");
        theErrorMessage.Put(bodyHeaderType.Ptr,bodyHeaderType.Len);
        theErrorMessage.PutEOL();

        qtss_snprintf(messageLineBuffer,maxMessageBufferChars, "Content-Length: %lu", bodyMessage.GetBytesWritten());
        theErrorMessage.Put(messageLineBuffer,::strlen(messageLineBuffer));        
        theErrorMessage.PutEOL();
    }

    static const StrPtrLen headerClose("Connection: close");
    theErrorMessage.Put(headerClose.Ptr,headerClose.Len);
    theErrorMessage.PutEOL();

    theErrorMessage.PutEOL();  // terminate headers with empty line

    if (addBody) // add html body
    {
        theErrorMessage.Put(bodyMessage.GetBufPtr(),bodyMessage.GetBytesWritten());
    }

    (void)QTSS_Write(inRequest, theErrorMessage.GetBufPtr(), theErrorMessage.GetBytesWritten(), NULL, 0);
    (void)QTSS_SetValue(inRequest, qtssRTSPReqRespMsg, 0, theErrorMessage.GetBufPtr(), theErrorMessage.GetBytesWritten());
    
    return QTSS_RequestFailed;
}
														
void    QTSSModuleUtils::SendDescribeResponse(QTSS_RTSPRequestObject inRequest,
                                                    QTSS_ClientSessionObject inSession,
                                                    iovec* describeData,
                                                    UInt32 inNumVectors,
                                                    UInt32 inTotalLength)
{
    //write content size header
    char buf[32];
    qtss_sprintf(buf, "%ld", inTotalLength);
    (void)QTSS_AppendRTSPHeader(inRequest, qtssContentLengthHeader, &buf[0], ::strlen(&buf[0]));

    (void)QTSS_SendStandardRTSPResponse(inRequest, inSession, 0);

#ifdef __solaris__
    if (inNumVectors > IOV_MAX )
    {
            char* describeDataBuffer = QTSSModuleUtils::CoalesceVectors(describeData, inNumVectors, inTotalLength);
            (void)QTSS_Write(inRequest, (void *)describeDataBuffer, inTotalLength, NULL, qtssWriteFlagsNoFlags);
            // deleting memory allocated by the CoalesceVectors call
            delete [] describeDataBuffer;
    }
    else
        (void)QTSS_WriteV(inRequest, describeData, inNumVectors, inTotalLength, NULL);
#else
    (void)QTSS_WriteV(inRequest, describeData, inNumVectors, inTotalLength, NULL);
#endif

}

char*   QTSSModuleUtils::CoalesceVectors(iovec* inVec, UInt32 inNumVectors, UInt32 inTotalLength)
{
    if (inTotalLength == 0)
        return NULL;
    
    char* buffer = NEW char[inTotalLength];
    UInt32 bufferOffset = 0;
    
    for (UInt32 index = 0; index < inNumVectors; index++)
    {
        ::memcpy (buffer + bufferOffset, inVec[index].iov_base, inVec[index].iov_len);
        bufferOffset += inVec[index].iov_len;
    }
    
    Assert (bufferOffset == inTotalLength);
    
    return buffer;
}

QTSS_ModulePrefsObject QTSSModuleUtils::GetModulePrefsObject(QTSS_ModuleObject inModObject)
{
    QTSS_ModulePrefsObject thePrefsObject = NULL;
    UInt32 theLen = sizeof(thePrefsObject);
    QTSS_Error theErr = QTSS_GetValue(inModObject, qtssModPrefs, 0, &thePrefsObject, &theLen);
    Assert(theErr == QTSS_NoErr);
    
    return thePrefsObject;
}

QTSS_Object QTSSModuleUtils::GetModuleAttributesObject(QTSS_ModuleObject inModObject)
{
    QTSS_Object theAttributesObject = NULL;
    UInt32 theLen = sizeof(theAttributesObject);
    QTSS_Error theErr = QTSS_GetValue(inModObject, qtssModAttributes, 0, &theAttributesObject, &theLen);
    Assert(theErr == QTSS_NoErr);
    
    return theAttributesObject;
}

QTSS_ModulePrefsObject QTSSModuleUtils::GetModuleObjectByName(const StrPtrLen& inModuleName)
{
    QTSS_ModuleObject theModule = NULL;
    UInt32 theLen = sizeof(theModule);
    
    for (int x = 0; QTSS_GetValue(sServer, qtssSvrModuleObjects, x, &theModule, &theLen) == QTSS_NoErr; x++)
    {
        Assert(theModule != NULL);
        Assert(theLen == sizeof(theModule));
        
        StrPtrLen theName;
        QTSS_Error theErr = QTSS_GetValuePtr(theModule, qtssModName, 0, (void**)&theName.Ptr, &theName.Len);
        Assert(theErr == QTSS_NoErr);
        
        if (inModuleName.Equal(theName))
            return theModule;
            
#if DEBUG
        theModule = NULL;
        theLen = sizeof(theModule);
#endif
    }
    return NULL;
}

void    QTSSModuleUtils::GetAttribute(QTSS_Object inObject, char* inAttributeName, QTSS_AttrDataType inType, 
                                                void* ioBuffer, void* inDefaultValue, UInt32 inBufferLen)
{

    QTSS_AttributeID theID = QTSSModuleUtils::CheckAttributeDataType(inObject, inAttributeName, inType, inDefaultValue, inBufferLen);

    QTSS_Error theErr = QTSS_GetValue(inObject, theID, 0, ioBuffer, &inBufferLen);

    Assert(theErr != QTSS_NotEnoughSpace);
    
    if (theErr != QTSS_NoErr)
    {

        ::memcpy(ioBuffer, inDefaultValue, inBufferLen);

        if (inBufferLen > 0)
        {

            char* theValueAsString = NULL;
            theErr = QTSS_ValueToString(inDefaultValue, inBufferLen, inType, &theValueAsString);
            Assert(theErr == QTSS_NoErr);
            OSCharArrayDeleter theValueStr(theValueAsString);
            QTSSModuleUtils::LogError(  sMissingPrefVerbosity, 
                                        qtssServerPrefMissing,
                                        0,
                                        inAttributeName,
                                        theValueStr.GetObject());
        }
        
        //
        // Create an entry for this attribute                           
        QTSSModuleUtils::CreateAttribute(inObject, inAttributeName, inType, inDefaultValue, inBufferLen);
    }
}

char*   QTSSModuleUtils::GetStringAttribute(QTSS_Object inObject, char* inAttributeName, char* inDefaultValue)
{
    UInt32 theDefaultValLen = 0;
    if (inDefaultValue != NULL)
        theDefaultValLen = ::strlen(inDefaultValue);

    QTSS_AttributeID theID = QTSSModuleUtils::CheckAttributeDataType(inObject, inAttributeName, qtssAttrDataTypeCharArray, inDefaultValue, theDefaultValLen);

    char* theString = NULL;
    (void)QTSS_GetValueAsString(inObject, theID, 0, &theString);
    if (theString != NULL)
        return theString;
    
    QTSSModuleUtils::CreateAttribute(inObject, inAttributeName, qtssAttrDataTypeCharArray, inDefaultValue, theDefaultValLen);

    if (theDefaultValLen > 0)
    {
        QTSSModuleUtils::LogError(  sMissingPrefVerbosity,
                                    qtssServerPrefMissing,
                                    0,
                                    inAttributeName,
                                    inDefaultValue);
    }
    
    if (inDefaultValue != NULL)
    {

        theString = NEW char[theDefaultValLen + 1];
        ::strcpy(theString, inDefaultValue);
        return theString;
    }
    return NULL;
}

void    QTSSModuleUtils::GetIOAttribute(QTSS_Object inObject, char* inAttributeName, QTSS_AttrDataType inType,
                            void* ioDefaultResultBuffer, UInt32 inBufferLen)
{
    char *defaultBuffPtr = NEW char[inBufferLen];
    ::memcpy(defaultBuffPtr,ioDefaultResultBuffer,inBufferLen);
    QTSSModuleUtils::GetAttribute(inObject, inAttributeName, inType, ioDefaultResultBuffer, defaultBuffPtr, inBufferLen);
    delete [] defaultBuffPtr;

}
                            

QTSS_AttributeID QTSSModuleUtils::GetAttrID(QTSS_Object inObject, char* inAttributeName)
{

    QTSS_Object theAttrInfo = NULL;
    QTSS_Error theErr = QTSS_GetAttrInfoByName(inObject, inAttributeName, &theAttrInfo);
    if (theErr != QTSS_NoErr)
        return qtssIllegalAttrID;

    QTSS_AttributeID theID = qtssIllegalAttrID; 
    UInt32 theLen = sizeof(theID);
    theErr = QTSS_GetValue(theAttrInfo, qtssAttrID, 0, &theID, &theLen);
    Assert(theErr == QTSS_NoErr);

    return theID;
}

QTSS_AttributeID QTSSModuleUtils::CheckAttributeDataType(QTSS_Object inObject, char* inAttributeName, QTSS_AttrDataType inType, void* inDefaultValue, UInt32 inBufferLen)
{
    //
    // Get the attribute type of this attribute.
    QTSS_Object theAttrInfo = NULL;
    QTSS_Error theErr = QTSS_GetAttrInfoByName(inObject, inAttributeName, &theAttrInfo);
    if (theErr != QTSS_NoErr)
        return qtssIllegalAttrID;

    QTSS_AttrDataType theAttributeType = qtssAttrDataTypeUnknown;
    UInt32 theLen = sizeof(theAttributeType);
    theErr = QTSS_GetValue(theAttrInfo, qtssAttrDataType, 0, &theAttributeType, &theLen);
    Assert(theErr == QTSS_NoErr);
    
    QTSS_AttributeID theID = qtssIllegalAttrID; 
    theLen = sizeof(theID);
    theErr = QTSS_GetValue(theAttrInfo, qtssAttrID, 0, &theID, &theLen);
    Assert(theErr == QTSS_NoErr);

    if (theAttributeType != inType)
    {
        char* theValueAsString = NULL;
        theErr = QTSS_ValueToString(inDefaultValue, inBufferLen, inType, &theValueAsString);
        Assert(theErr == QTSS_NoErr);
        OSCharArrayDeleter theValueStr(theValueAsString);
        QTSSModuleUtils::LogError(  qtssWarningVerbosity,
                                    qtssServerPrefWrongType,
                                    0,
                                    inAttributeName,
                                    theValueStr.GetObject());
                                    
        theErr = QTSS_RemoveInstanceAttribute( inObject, theID );
        Assert(theErr == QTSS_NoErr);
        return  QTSSModuleUtils::CreateAttribute(inObject, inAttributeName, inType, inDefaultValue, inBufferLen);
    }
    return theID;
}

QTSS_AttributeID QTSSModuleUtils::CreateAttribute(QTSS_Object inObject, char* inAttributeName, QTSS_AttrDataType inType, void* inDefaultValue, UInt32 inBufferLen)
{
    QTSS_Error theErr = QTSS_AddInstanceAttribute(inObject, inAttributeName, NULL, inType);
    Assert((theErr == QTSS_NoErr) || (theErr == QTSS_AttrNameExists));
    
    QTSS_AttributeID theID = QTSSModuleUtils::GetAttrID(inObject, inAttributeName);
    Assert(theID != qtssIllegalAttrID);
        
    //
    // Caller can pass in NULL for inDefaultValue, in which case we don't add the default
    if (inDefaultValue != NULL)
    {
        theErr = QTSS_SetValue(inObject, theID, 0, inDefaultValue, inBufferLen);
        Assert(theErr == QTSS_NoErr);
    }
    return theID;
}

QTSS_ActionFlags QTSSModuleUtils::GetRequestActions(QTSS_RTSPRequestObject theRTSPRequest)
{
    // Don't touch write requests
    QTSS_ActionFlags action = qtssActionFlagsNoFlags;
    UInt32 len = sizeof(QTSS_ActionFlags);
    QTSS_Error theErr = QTSS_GetValue(theRTSPRequest, qtssRTSPReqAction, 0, (void*)&action, &len);
    Assert(theErr == QTSS_NoErr);
    Assert(len == sizeof(QTSS_ActionFlags));
    return action;
}

char* QTSSModuleUtils::GetLocalPath_Copy(QTSS_RTSPRequestObject theRTSPRequest)
{   char*   pathBuffStr = NULL;
    QTSS_Error theErr = QTSS_GetValueAsString(theRTSPRequest, qtssRTSPReqLocalPath, 0, &pathBuffStr);
    Assert(theErr == QTSS_NoErr);
    return pathBuffStr;
}

char* QTSSModuleUtils::GetMoviesRootDir_Copy(QTSS_RTSPRequestObject theRTSPRequest)
{   char*   movieRootDirStr = NULL;
    QTSS_Error theErr = QTSS_GetValueAsString(theRTSPRequest,qtssRTSPReqRootDir, 0, &movieRootDirStr);
    Assert(theErr == QTSS_NoErr);
    return movieRootDirStr;
}

QTSS_UserProfileObject QTSSModuleUtils::GetUserProfileObject(QTSS_RTSPRequestObject theRTSPRequest)
{   QTSS_UserProfileObject theUserProfile = NULL;
    UInt32 len = sizeof(QTSS_UserProfileObject);
    QTSS_Error theErr = QTSS_GetValue(theRTSPRequest, qtssRTSPReqUserProfile, 0, (void*)&theUserProfile, &len);
    Assert(theErr == QTSS_NoErr);
    return theUserProfile;
}

char *QTSSModuleUtils::GetUserName_Copy(QTSS_UserProfileObject inUserProfile)
{
    char*   username = NULL;    
    (void) QTSS_GetValueAsString(inUserProfile, qtssUserName, 0, &username);
    return username;
}

char**  QTSSModuleUtils::GetGroupsArray_Copy(QTSS_UserProfileObject inUserProfile, UInt32 *outNumGroupsPtr)
{
    Assert(NULL != outNumGroupsPtr);

    char** outGroupCharPtrArray = NULL;
    *outNumGroupsPtr = 0;
    
    if (NULL == inUserProfile)
        return NULL;
    
    QTSS_Error theErr = QTSS_GetNumValues (inUserProfile,qtssUserGroups, outNumGroupsPtr);
    if (theErr != QTSS_NoErr || *outNumGroupsPtr == 0)
        return NULL;
        
    outGroupCharPtrArray = NEW char*[*outNumGroupsPtr]; // array of char *
    UInt32 len = 0;
    for (UInt32 index = 0; index < *outNumGroupsPtr; index++)
    {   outGroupCharPtrArray[index] = NULL;
        QTSS_GetValuePtr(inUserProfile, qtssUserGroups, index,(void **) &outGroupCharPtrArray[index], &len);
    }   

    return outGroupCharPtrArray;
}

Bool16 QTSSModuleUtils::UserInGroup(QTSS_UserProfileObject inUserProfile, char* inGroup, UInt32 inGroupLen)
{
	if (NULL == inUserProfile || NULL == inGroup  ||  inGroupLen == 0) 
		return false;
		
	char *userName = NULL;
	UInt32 len = 0;
	QTSS_GetValuePtr(inUserProfile, qtssUserName, 0, (void **)&userName, &len);
	if (len == 0 || userName == NULL || userName[0] == 0) // no user to check
		return false;

	UInt32 numGroups = 0;
	QTSS_GetNumValues (inUserProfile,qtssUserGroups, &numGroups);
	if (numGroups == 0) // no groups to check
		return false;

	Bool16 result = false;
	char* userGroup = NULL;
	StrPtrLenDel userGroupStr; //deletes pointer in destructor
	
	for (UInt32 index = 0; index < numGroups; index++)
	{  
		userGroup = NULL;
		QTSS_GetValueAsString(inUserProfile, qtssUserGroups, index, &userGroup); //allocates string
		userGroupStr.Delete();
		userGroupStr.Set(userGroup);					
		if(userGroupStr.Equal(inGroup))
		{	
			result = true;
			break;
		}
	}   

	return result;
	
}


Bool16 QTSSModuleUtils::AddressInList(QTSS_Object inObject, QTSS_AttributeID listID, StrPtrLen *inAddressPtr)
{
    StrPtrLenDel strDeleter;
    char*   theAttributeString = NULL;    
    IPComponentStr inAddress(inAddressPtr);
    IPComponentStr addressFromList;
    
    if (!inAddress.Valid())
        return false;

    UInt32 numValues = 0;
    (void) QTSS_GetNumValues(inObject, listID, &numValues);
    
    for (UInt32 index = 0; index < numValues; index ++)
    { 
        strDeleter.Delete();
        (void) QTSS_GetValueAsString(inObject, listID, index, &theAttributeString);
        strDeleter.Set(theAttributeString);
 
        addressFromList.Set(&strDeleter);
        if (addressFromList.Equal(&inAddress))
            return true;
    }

    return false;
}

Bool16 QTSSModuleUtils::FindStringInAttributeList(QTSS_Object inObject, QTSS_AttributeID listID, StrPtrLen *inStrPtr)
{
    StrPtrLenDel tempString;
     
    if (NULL == inStrPtr || NULL == inStrPtr->Ptr || 0 == inStrPtr->Len)
        return false;

    UInt32 numValues = 0;
    (void) QTSS_GetNumValues(inObject, listID, &numValues);
    
    for (UInt32 index = 0; index < numValues; index ++)
    { 
        tempString.Delete();
        (void) QTSS_GetValueAsString(inObject, listID, index, &tempString.Ptr);
        tempString.Set(tempString.Ptr);

        if (inStrPtr->FindString(tempString.Ptr))
            return true;
            
   }

    return false;
}

Bool16 QTSSModuleUtils::HavePlayerProfile(QTSS_PrefsObject inPrefObjectToCheck, QTSS_StandardRTSP_Params* inParams, UInt32 feature)
{
    StrPtrLenDel userAgentStr;    	
    (void)QTSS_GetValueAsString(inParams->inClientSession, qtssCliSesFirstUserAgent, 0, &userAgentStr.Ptr);
    userAgentStr.Set(userAgentStr.Ptr);
    
    switch (feature)
    {
        case QTSSModuleUtils::kRequiresRTPInfoSeqAndTime:
        {        
            return QTSSModuleUtils::FindStringInAttributeList(inPrefObjectToCheck,  qtssPrefsPlayersReqRTPHeader, &userAgentStr);

        }
        break;
        
        case QTSSModuleUtils::kAdjustBandwidth:
        {
            return QTSSModuleUtils::FindStringInAttributeList(inPrefObjectToCheck,  qtssPrefsPlayersReqBandAdjust, &userAgentStr);
        }
        break;
		
        case QTSSModuleUtils::kDisablePauseAdjustedRTPTime:
        {
            return QTSSModuleUtils::FindStringInAttributeList(inPrefObjectToCheck,  qtssPrefsPlayersReqNoPauseTimeAdjust, &userAgentStr);
        }
        break;
    }
    
    return false;
}


IPComponentStr IPComponentStr::sLocalIPCompStr("127.0.0.*");

IPComponentStr::IPComponentStr(char *theAddressPtr)
{
    StrPtrLen sourceStr(theAddressPtr);
     (void) this->Set(&sourceStr);    
}

IPComponentStr::IPComponentStr(StrPtrLen *sourceStrPtr)
{
    (void) this->Set(sourceStrPtr);    
}

Bool16 IPComponentStr::Set(StrPtrLen *theAddressStrPtr)
{
    fIsValid = false;
   
    StringParser IP_Paser(theAddressStrPtr);
    StrPtrLen *piecePtr = &fAddressComponent[0];

    while (IP_Paser.GetDataRemaining() > 0) 
    {
        IP_Paser.ConsumeUntil(piecePtr,'.');    
        if (piecePtr->Len == 0) 
            break;
        
        IP_Paser.ConsumeLength(NULL, 1);
        if (piecePtr == &fAddressComponent[IPComponentStr::kNumComponents -1])
        {
           fIsValid = true;
           break;
        }
        
        piecePtr++;
    };
     
    return fIsValid;
}


Bool16 IPComponentStr::Equal(IPComponentStr *testAddressPtr)
{
    if (testAddressPtr == NULL) 
        return false;
    
    if ( !this->Valid() || !testAddressPtr->Valid() )
        return false;

    for (UInt16 component= 0 ; component < IPComponentStr::kNumComponents ; component ++)
    {
        StrPtrLen *allowedPtr = this->GetComponent(component);
        StrPtrLen *testPtr = testAddressPtr->GetComponent(component);
                       
        if ( testPtr->Equal("*") || allowedPtr->Equal("*") )
            continue;
            
        if (!testPtr->Equal(*allowedPtr) ) 
            return false; 
    };  
    
    return true;
}


