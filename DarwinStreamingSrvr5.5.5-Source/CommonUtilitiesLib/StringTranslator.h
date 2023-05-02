#ifndef __STRINGTRANSLATOR_H__
#define __STRINGTRANSLATOR_H__

#include "OSHeaders.h"

#define STRINGTRANSLATORTESTING 0

class StringTranslator
{
    public:
    
               static SInt32   DecodeURL(const char* inSrc, SInt32 inSrcLen, char* ioDest, SInt32 inDestLen);

               static SInt32   EncodeURL(const char* inSrc, SInt32 inSrcLen, char* ioDest, SInt32 inDestLen);
        
               static void     DecodePath(char* inSrc, UInt32 inSrcLen);
        
#if STRINGTRANSLATORTESTING
        static Bool16       Test();
#endif  
};
#endif 

