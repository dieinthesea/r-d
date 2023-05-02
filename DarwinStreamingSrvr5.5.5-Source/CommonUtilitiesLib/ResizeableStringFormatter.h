#ifndef __RESIZEABLE_STRING_FORMATTER_H__
#define __RESIZEABLE_STRING_FORMATTER_H__

#include "StringFormatter.h"

class ResizeableStringFormatter : public StringFormatter
{
    public:
      
        ResizeableStringFormatter(char* inBuffer = NULL, UInt32 inBufSize = 0)
            : StringFormatter(inBuffer, inBufSize), fOriginalBuffer(inBuffer) {}

        virtual ~ResizeableStringFormatter() {  if (fStartPut != fOriginalBuffer) delete [] fStartPut; }

    private:

        virtual void    BufferIsFull(char* inBuffer, UInt32 inBufferLen);
        
        char*           fOriginalBuffer;
        
};

#endif 
