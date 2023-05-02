#include <string.h>
#include "StringFormatter.h"
#include "MyAssert.h"

char*   StringFormatter::sEOL = "\r\n";
UInt32  StringFormatter::sEOLLen = 2;

void StringFormatter::Put(const SInt32 num)
{
    char buff[32];
    qtss_sprintf(buff, "%ld", num);
    Put(buff);
}

void StringFormatter::Put(char* buffer, UInt32 bufferSize)
{
    if((bufferSize == 1) && (fCurrentPut != fEndPut)) {
        *(fCurrentPut++) = *buffer;
        fBytesWritten++;
        return;
    }       
        
    UInt32 spaceLeft = this->GetSpaceLeft();
    UInt32 spaceInBuffer =  spaceLeft - 1;
    UInt32 resizedSpaceLeft = 0;
    
    while ( (spaceInBuffer < bufferSize) || (spaceLeft == 0) ) 
    {
        if (spaceLeft > 0)
        {
            ::memcpy(fCurrentPut, buffer, spaceInBuffer);
            fCurrentPut += spaceInBuffer;
            fBytesWritten += spaceInBuffer;
            buffer += spaceInBuffer;
            bufferSize -= spaceInBuffer;
        }
        this->BufferIsFull(fStartPut, this->GetCurrentOffset()); 
        resizedSpaceLeft = this->GetSpaceLeft();
        if (spaceLeft == resizedSpaceLeft) 
        {  
           return; 
        }
        spaceLeft = resizedSpaceLeft;
        spaceInBuffer =  spaceLeft - 1;
    }
    
    ::memcpy(fCurrentPut, buffer, bufferSize);
    fCurrentPut += bufferSize;
    fBytesWritten += bufferSize;
    
}

