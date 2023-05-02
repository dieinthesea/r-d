#ifndef __STRINGPARSER_H__
#define __STRINGPARSER_H__

#include "StrPtrLen.h"
#include "MyAssert.h"

#define STRINGPARSERTESTING 0


class StringParser
{
    public:
        
        StringParser(StrPtrLen *inStream)
            :   fStartGet(inStream == NULL ? NULL : inStream->Ptr),
                fEndGet(inStream == NULL ? NULL : inStream->Ptr + inStream->Len),
                fCurLineNumber(1),
                fStream(inStream) {}
        ~StringParser() {}
	
        static UInt8 sDigitMask[];      // stop when you hit a digit
        static UInt8 sWordMask[];       // stop when you hit a word
        static UInt8 sEOLMask[];        // stop when you hit an eol
        static UInt8 sEOLWhitespaceMask[]; // stop when you hit an EOL or whitespace
        static UInt8 sWhitespaceMask[]; // skip over whitespace

        StrPtrLen*      GetStream() { return fStream; }
        
        Bool16          Expect(char stopChar);
        Bool16          ExpectEOL();
        
        //Returns the next word
        void            ConsumeWord(StrPtrLen* outString = NULL)
                            { ConsumeUntil(outString, sNonWordMask); }

        //Returns all the data before inStopChar
        void            ConsumeUntil(StrPtrLen* outString, char inStopChar);

        //Returns whatever integer is currently in the stream
        UInt32          ConsumeInteger(StrPtrLen* outString = NULL);
        Float32         ConsumeFloat();
        Float32         ConsumeNPT();

        //Keeps on going until non-whitespace
        void            ConsumeWhitespace()
                            { ConsumeUntil(NULL, sWhitespaceMask); }

        void            ConsumeUntil(StrPtrLen* spl, UInt8 *stop);

        void            ConsumeUntilWhitespace(StrPtrLen* spl = NULL)
                            { ConsumeUntil( spl, sEOLWhitespaceMask); }

        void            ConsumeUntilDigit(StrPtrLen* spl = NULL)
                            { ConsumeUntil( spl, sDigitMask); }

		void			ConsumeLength(StrPtrLen* spl, SInt32 numBytes);

		void			ConsumeEOL(StrPtrLen* outString);

        inline Bool16       GetThru(StrPtrLen* spl, char stop);
        inline Bool16       GetThruEOL(StrPtrLen* spl);
        inline Bool16       ParserIsEmpty(StrPtrLen* outString);

        inline char     PeekFast() { if (fStartGet) return *fStartGet; else return '\0'; }
        char operator[](int i) { Assert((fStartGet+i) < fEndGet);return fStartGet[i]; }
        

        UInt32          GetDataParsedLen() 
            { Assert(fStartGet >= fStream->Ptr); return (UInt32)(fStartGet - fStream->Ptr); }
        UInt32          GetDataReceivedLen()
            { Assert(fEndGet >= fStream->Ptr); return (UInt32)(fEndGet - fStream->Ptr); }
        UInt32          GetDataRemaining()
            { Assert(fEndGet >= fStartGet); return (UInt32)(fEndGet - fStartGet); }
        char*           GetCurrentPosition() { return fStartGet; }
        int         GetCurrentLineNumber() { return fCurLineNumber; }
        
        static void UnQuote(StrPtrLen* outString);


#if STRINGPARSERTESTING
        static Bool16       Test();
#endif

    private:

        void        AdvanceMark();
        
        static UInt8 sNonWordMask[];

        char*       fStartGet;
        char*       fEndGet;
        int         fCurLineNumber;
        StrPtrLen*  fStream;
        
};


Bool16 StringParser::GetThru(StrPtrLen* outString, char inStopChar)
{
    ConsumeUntil(outString, inStopChar);
    return Expect(inStopChar);
}

Bool16 StringParser::GetThruEOL(StrPtrLen* outString)
{
    ConsumeUntil(outString, sEOLMask);
    return ExpectEOL();
}

Bool16 StringParser::ParserIsEmpty(StrPtrLen* outString)
{
    if (NULL == fStartGet || NULL == fEndGet)
    {
        if (NULL != outString)
        {   outString->Ptr = NULL;
            outString->Len = 0;
        }
        
        return true;
    }
    
    Assert(fStartGet <= fEndGet);
    
    return false; // parser ok to parse
}


#endif 
