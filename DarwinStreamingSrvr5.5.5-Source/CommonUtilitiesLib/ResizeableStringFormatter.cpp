#include "ResizeableStringFormatter.h"
#include "OSMemory.h"

void    ResizeableStringFormatter::BufferIsFull(char* inBuffer, UInt32 inBufferLen)
{
   
    UInt32 theNewBufferSize = this->GetTotalBufferSize() * 2;
    if (theNewBufferSize == 0)
        theNewBufferSize = 64;
        
    char* theNewBuffer = NEW char[theNewBufferSize];
    ::memcpy(theNewBuffer, inBuffer, inBufferLen);

   
    if (inBuffer != fOriginalBuffer)
        delete [] inBuffer;
    
    fStartPut = theNewBuffer;
    fCurrentPut = theNewBuffer + inBufferLen;
    fEndPut = theNewBuffer + theNewBufferSize;
}
