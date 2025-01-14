#ifndef __SDPUtilsH__
#define __SDPUtilsH__

#include "OS.h"
#include "StrPtrLen.h"
#include "ResizeableStringFormatter.h"
#include "StringParser.h"
#include "OSMemory.h"

class SDPLine : public StrPtrLen
{
public:
	SDPLine() : fHeaderType('\0') {}
    virtual ~SDPLine() {}

    char    fHeaderType;
};

class SDPContainer
{
    enum { kBaseLines = 20, kLineTypeArraySize = 256};

    enum {  
            kVPos = 0,
            kSPos,
            kTPos,
            kOPos
         };

    enum {
            kV = 1 << kVPos, 
            kS = 1 << kSPos,
            kT = 1 << kTPos,
            kO = 1 << kOPos,
            kAllReq = kV | kS | kT | kO
          };
            

public:

    SDPContainer(UInt32 numStrPtrs = SDPContainer::kBaseLines) : 
        fNumSDPLines(numStrPtrs), 
        fSDPLineArray(NULL)
    {   
        Initialize();
    }

    ~SDPContainer() {delete [] fSDPLineArray;}
	void		Initialize();
    SInt32      AddHeaderLine (StrPtrLen *theLinePtr);
    SInt32      FindHeaderLineType(char id, SInt32 start);
    SDPLine*    GetNextLine();
    SDPLine*    GetLine(SInt32 lineIndex);
    void        SetLine(SInt32 index);
    void        Parse();
    Bool16      SetSDPBuffer(char *sdpBuffer);
    Bool16      SetSDPBuffer(StrPtrLen *sdpBufferPtr);
    Bool16      IsSDPBufferValid() {return fValid;}
    Bool16      HasReqLines() { return (Bool16) (fReqLines == kAllReq) ; }
    Bool16      HasLineType( char lineType ) { return (Bool16) (lineType == fFieldStr[lineType]) ; }
    char*       GetReqLinesArray;
    void        PrintLine(SInt32 lineIndex);
    void        PrintAllLines();
    SInt32      GetNumLines() { return  fNumUsedLines; }
    
    SInt32      fCurrentLine;
    SInt32      fNumSDPLines;
    SInt32      fNumUsedLines;
    SDPLine*    fSDPLineArray;
    Bool16      fValid;
    StrPtrLen   fSDPBuffer;
    UInt16      fReqLines;

    char        fFieldStr[kLineTypeArraySize];

};

class SDPLineSorter {

public:
	SDPLineSorter(): fSessionLineCount(0),fSDPSessionHeaders(NULL,0), fSDPMediaHeaders(NULL,0) {};
	SDPLineSorter(SDPContainer *rawSDPContainerPtr, Float32 adjustMediaBandwidthPercent = 1.0);
	
	StrPtrLen* GetSessionHeaders() { return &fSessionHeaders; }
	StrPtrLen* GetMediaHeaders() { return &fMediaHeaders; }
	char* GetSortedSDPCopy();
	
	StrPtrLen fullSDPBuffSPL;
	SInt32 fSessionLineCount;
	SDPContainer fSessionSDPContainer;
	ResizeableStringFormatter fSDPSessionHeaders;
	ResizeableStringFormatter fSDPMediaHeaders;
	StrPtrLen fSessionHeaders;
	StrPtrLen fMediaHeaders;
	static char sSessionOrderedLines[];
	static char sessionSingleLines[];
	static StrPtrLen sEOL;
    static StrPtrLen sMaxBandwidthTag;
};


#endif

