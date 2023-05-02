#include "SDPUtils.h"

#include "OS.h"
#include "StrPtrLen.h"
#include "ResizeableStringFormatter.h"
#include "StringParser.h"
#include "ResizeableStringFormatter.h"
#include "StringParser.h"
#include "OSMemory.h"


SInt32 SDPContainer::AddHeaderLine (StrPtrLen *theLinePtr)
{   
    Assert(theLinePtr);
    UInt32 thisLine = fNumUsedLines;
    Assert(fNumUsedLines < fNumSDPLines);
    fSDPLineArray[thisLine].Set(theLinePtr->Ptr, theLinePtr->Len);
    fNumUsedLines++;
    if (fNumUsedLines == fNumSDPLines)
    {
        SDPLine   *tempSDPLineArray = NEW SDPLine[fNumSDPLines * 2];
        for (int i = 0; i < fNumSDPLines; i++)
        {
            tempSDPLineArray[i].Set(fSDPLineArray[i].Ptr,fSDPLineArray[i].Len);
            tempSDPLineArray[i].fHeaderType = fSDPLineArray[i].fHeaderType;
        }
        delete [] fSDPLineArray;
        fSDPLineArray = tempSDPLineArray;
        fNumSDPLines = (fNumUsedLines * 2);
    }
    
    if (theLinePtr->Ptr)
        fSDPLineArray[thisLine].fHeaderType = theLinePtr->Ptr[0];
        
    return thisLine;
}

SInt32 SDPContainer::FindHeaderLineType(char id, SInt32 start)
{   
    SInt32 theIndex = -1;
    
    if (start >= fNumUsedLines || start < 0)
        return -1;
        
    for (int i = start; i < fNumUsedLines; i++)
    {   if (fSDPLineArray[i].fHeaderType == id)
        {   theIndex = i;
            fCurrentLine = theIndex;
            break;
        }
    }
    
    return theIndex;
}

SDPLine* SDPContainer::GetNextLine()
{
    if (fCurrentLine < fNumUsedLines)
    {   fCurrentLine ++;
        return &fSDPLineArray[fCurrentLine];
    }
    
    return NULL;

}

SDPLine* SDPContainer::GetLine(SInt32 lineIndex)
{
    
    if (lineIndex > -1 && lineIndex < fNumUsedLines)
    {   return &fSDPLineArray[lineIndex];
    }

    return NULL;
}

void SDPContainer::SetLine(SInt32 index)
{
    if (index > -1 && index < fNumUsedLines)
    {   fCurrentLine = index;
    }
    else
        Assert(0);
        
}

void SDPContainer::Parse()
{
	char*	    validChars = "vosiuepcbtrzkam";
	char        nameValueSeparator = '=';
	
	Bool16      valid = true;

	StringParser	sdpParser(&fSDPBuffer);
	StrPtrLen		line;
	StrPtrLen 		fieldName;
	StrPtrLen		space;
	Bool16          foundLine = false;
	
	while ( sdpParser.GetDataRemaining() != 0 )
	{
		foundLine = sdpParser.GetThruEOL(&line);  
		if (!foundLine) 
		{ 
		   break;
		}
        StringParser lineParser(&line);

        lineParser.ConsumeWhitespace();
        if (lineParser.GetDataRemaining() == 0) // must be an empty line
            continue;

        char firstChar = lineParser.PeekFast();
        if (firstChar == '\0')
            continue; 
        
        fFieldStr[firstChar] = firstChar;
        switch (firstChar)
        {
            case 'v': fReqLines |= kV;
            break;
    
            case 's': fReqLines |= kS ;
            break;
    
            case 't': fReqLines |= kT ;
            break;
    
            case 'o': fReqLines |= kO ;
            break;
        
        }

		lineParser.ConsumeUntil(&fieldName, nameValueSeparator);
		if ((fieldName.Len != 1) || (::strchr(validChars, fieldName.Ptr[0]) == NULL))
		{
			valid = false; 
			break;
		}
		
		if (!lineParser.Expect(nameValueSeparator))
		{
			valid = false; 
			break;
		}
		
		lineParser.ConsumeUntil(&space, StringParser::sWhitespaceMask);
		
		if (space.Len != 0)
		{
			valid = false; 
			break;
		}
		AddHeaderLine(&line);
	}
	
	if (fNumUsedLines == 0) // didn't add any lines
	{   valid = false;
	}
	fValid = valid;
	
}

void SDPContainer::Initialize()
{
    fCurrentLine = 0;
    fNumUsedLines = 0;
    delete [] fSDPLineArray;
    fSDPLineArray = NEW SDPLine[fNumSDPLines]; 
    fValid = false;
    fReqLines = 0;
    ::memset(fFieldStr, sizeof(fFieldStr), 0);
}

Bool16 SDPContainer::SetSDPBuffer(char *sdpBuffer) 
{ 
    
    Initialize();
    if (sdpBuffer != NULL)
    {   fSDPBuffer.Set(sdpBuffer); 
        Parse(); 
    }
    
    return IsSDPBufferValid();
}

Bool16 SDPContainer::SetSDPBuffer(StrPtrLen *sdpBufferPtr)
{ 
    Initialize();
    if (sdpBufferPtr != NULL)
    {   fSDPBuffer.Set(sdpBufferPtr->Ptr, sdpBufferPtr->Len); 
        Parse(); 
    }
    
    return IsSDPBufferValid();
}


void  SDPContainer::PrintLine(SInt32 lineIndex)
{
    StrPtrLen *printLinePtr = GetLine(lineIndex);
    if (printLinePtr)
    {   printLinePtr->PrintStr();
        qtss_printf("\n");
    }

}

void  SDPContainer::PrintAllLines()
{
    if (fNumUsedLines > 0)
    {   for (int i = 0; i < fNumUsedLines; i++)
            PrintLine(i);
    }
    else
        qtss_printf("SDPContainer::PrintAllLines no lines\n"); 
}


char SDPLineSorter::sSessionOrderedLines[] = "vosiuepcbtrzka"; 
char SDPLineSorter::sessionSingleLines[]  = "vosiuepcbzk";   
StrPtrLen  SDPLineSorter::sEOL("\r\n");
StrPtrLen  SDPLineSorter::sMaxBandwidthTag("b=AS:");

SDPLineSorter::SDPLineSorter(SDPContainer *rawSDPContainerPtr, Float32 adjustMediaBandwidthPercent) : fSessionLineCount(0),fSDPSessionHeaders(NULL,0), fSDPMediaHeaders(NULL,0)
{

	Assert(rawSDPContainerPtr != NULL);
	if (NULL == rawSDPContainerPtr) 
		return;
		
	StrPtrLen theSDPData(rawSDPContainerPtr->fSDPBuffer.Ptr,rawSDPContainerPtr->fSDPBuffer.Len);
	StrPtrLen *theMediaStart = rawSDPContainerPtr->GetLine(rawSDPContainerPtr->FindHeaderLineType('m',0));
 	if (theMediaStart && theMediaStart->Ptr && theSDPData.Ptr)
	{
		UInt32  mediaLen = theSDPData.Len - (UInt32) (theMediaStart->Ptr - theSDPData.Ptr);
		char *mediaStartPtr= theMediaStart->Ptr;
		fMediaHeaders.Set(mediaStartPtr,mediaLen);
        StringParser sdpParser(&fMediaHeaders);
        StrPtrLen sdpLine;
        Bool16 foundLine = false;
        while (sdpParser.GetDataRemaining() > 0)
        {               
            foundLine = sdpParser.GetThruEOL(&sdpLine);
            if (!foundLine)
            {  
                break;
            }  
            
            if ( ( 'b' == sdpLine.Ptr[0]) && (1.0 != adjustMediaBandwidthPercent) )
            {   
                StringParser bLineParser(&sdpLine);
                bLineParser.ConsumeUntilDigit();
                UInt32 bandwidth = (UInt32) (.5 + (adjustMediaBandwidthPercent * (Float32) bLineParser.ConsumeInteger() ) );
                if (bandwidth < 1) 
                    bandwidth = 1;
                
                char bandwidthStr[10];
                qtss_snprintf(bandwidthStr,sizeof(bandwidthStr) -1, "%lu", bandwidth);
                bandwidthStr[sizeof(bandwidthStr) -1] = 0;
                
                fSDPMediaHeaders.Put(sMaxBandwidthTag);
                fSDPMediaHeaders.Put(bandwidthStr);
            }
            else
                fSDPMediaHeaders.Put(sdpLine);

            fSDPMediaHeaders.Put(SDPLineSorter::sEOL);
        }       
        fMediaHeaders.Set(fSDPMediaHeaders.GetBufPtr(),fSDPMediaHeaders.GetBytesWritten());
    }

	fSessionLineCount = rawSDPContainerPtr->FindHeaderLineType('m',0);
	if (fSessionLineCount < 0) // didn't find it use the whole buffer
	{   fSessionLineCount = rawSDPContainerPtr->GetNumLines();
	}

	for (SInt16 sessionLineIndex = 0; sessionLineIndex < fSessionLineCount; sessionLineIndex++)
		fSessionSDPContainer.AddHeaderLine( (StrPtrLen *) rawSDPContainerPtr->GetLine(sessionLineIndex));


	SInt16 numHeaderTypes = sizeof(SDPLineSorter::sSessionOrderedLines) -1;

	for (SInt16 fieldTypeIndex = 0; fieldTypeIndex < numHeaderTypes; fieldTypeIndex ++)
	{
		SInt32 lineIndex = fSessionSDPContainer.FindHeaderLineType(SDPLineSorter::sSessionOrderedLines[fieldTypeIndex], 0);
		StrPtrLen *theHeaderLinePtr = fSessionSDPContainer.GetLine(lineIndex);
		while (theHeaderLinePtr != NULL)
		{
			fSDPSessionHeaders.Put(*theHeaderLinePtr);
			fSDPSessionHeaders.Put(SDPLineSorter::sEOL);

			if (NULL != ::strchr(sessionSingleLines, theHeaderLinePtr->Ptr[0] ) ) 
				break; 

			lineIndex = fSessionSDPContainer.FindHeaderLineType(SDPLineSorter::sSessionOrderedLines[fieldTypeIndex], lineIndex + 1);
			theHeaderLinePtr = fSessionSDPContainer.GetLine(lineIndex);
		}
	}
	fSessionHeaders.Set(fSDPSessionHeaders.GetBufPtr(),fSDPSessionHeaders.GetBytesWritten());

}

char* SDPLineSorter::GetSortedSDPCopy()
{
	char* fullbuffCopy = NEW char[fSessionHeaders.Len + fMediaHeaders.Len + 2];
	SInt32 buffPos = 0;
	memcpy(&fullbuffCopy[buffPos], fSessionHeaders.Ptr,fSessionHeaders.Len);
	buffPos += fSessionHeaders.Len;
	memcpy(&fullbuffCopy[buffPos], fMediaHeaders.Ptr,fMediaHeaders.Len);
	buffPos += fMediaHeaders.Len;
	fullbuffCopy[buffPos] = 0;	
	
	return fullbuffCopy;
}


