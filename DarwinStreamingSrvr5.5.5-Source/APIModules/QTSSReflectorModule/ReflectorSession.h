/*
The header file of C++code contains two classes: FileDelete and ReflectorSession. 
Used for automatically deleting files and for forwarding RTP packets, respectively.
*/
/*
    File:       ReflectorSession.h

    Contains:   This object supports reflecting an RTP multicast stream to N
                RTPStreams. It spaces out the packet send times in order to
                maximize the randomness of the sending pattern and smooth
                the stream.
                    
 */
#include "QTSS.h"
#include "OSRef.h"
#include "StrPtrLen.h"
#include "ResizeableStringFormatter.h"
#include "MyAssert.h"

#include "ReflectorStream.h"
#include "SourceInfo.h"
#include "OSArrayObjectDeleter.h"

#ifndef _FILE_DELETER_
#define _FILE_DELETER_

//Used to automatically delete files
class FileDeleter
{   public:
        FileDeleter(StrPtrLen* inSDPPath);
        ~FileDeleter();
        
    private:
        StrPtrLen fFilePath;
};

#endif

//Used to achieve the forwarding of RTP packets.

//Public interfaces include initialization functions, 
//adding output functions, removing output functions, 
//and functions for obtaining some information. 
//In addition, this class also includes some enumeration types and constant definitions.
#ifndef __REFLECTOR_SESSION__
#define __REFLECTOR_SESSION__
class ReflectorSession
{
    public:
    
        // Public interface to generic RTP packet forwarding engine        
        // Call initialize before calling any other function in this class
        static void Initialize();
            
        // Create one of these ReflectorSessions per source broadcast. For mapping purposes,
        // the object can be constructred using an optional source ID.
        
        // Caller may also provide a SourceInfo object, though it is not needed and
        // will also need to be provided to SetupReflectorSession when that is called.
        ReflectorSession(StrPtrLen* inSourceID, SourceInfo* inInfo = NULL);
        virtual ~ReflectorSession();
        
        // MODIFIERS
        
        // Call this to initialize and setup the source sockets. Once this function
        // completes sucessfully, Outputs may be added through the function calls below.
        
        // The SourceInfo object passed in here will be owned by the ReflectorSession. Do not
        // delete it.
        
        enum
        {
            kMarkSetup = 0,     //After SetupReflectorSession is called, IsSetup returns true
            kDontMarkSetup = 1, //After SetupReflectorSession is called, IsSetup returns false
            kIsPushSession = 2  // When setting up streams handle port conflicts by allocating.
        };
        
        QTSS_Error      SetupReflectorSession(SourceInfo* inInfo, QTSS_StandardRTSP_Params* inParams,
                                                UInt32 inFlags = kMarkSetup, Bool16 filterState = true, UInt32 filterTimeout = 30);
                                                
        // Packets get forwarded by attaching ReflectorOutput objects to a ReflectorSession.

        void    AddOutput(ReflectorOutput* inOutput, Bool16 isClient);
        void    RemoveOutput(ReflectorOutput* inOutput, Bool16 isClient);
        void    TearDownAllOutputs();
        void    RemoveSessionFromOutput(QTSS_ClientSessionObject inSession);
        void    ManuallyMarkSetup() { fIsSetup = true; }
        
        // For the Relay's status, a ReflectorSession can format an informative bit of HTML to describe the source.
        // This must be called after the ReflectorSession is all setup.
        
        void    FormatHTML(StrPtrLen* inURL);

        // ACCESSORS
        
        OSRef*          GetRef()            { return &fRef; }
        OSQueueElem*    GetQueueElem()      { return &fQueueElem; }
        UInt32          GetNumOutputs()     { return fNumOutputs; }
        UInt32          GetNumStreams()     { return fSourceInfo->GetNumStreams(); }
        StrPtrLen*      GetSourceInfoHTML() { return &fSourceInfoHTML; }
        SourceInfo*     GetSourceInfo()     { return fSourceInfo; }
        StrPtrLen*      GetLocalSDP()       { return &fLocalSDP; }
        StrPtrLen*      GetSourcePath()     { return &fSourceID; }
        Bool16          IsSetup()           { return fIsSetup; }
        ReflectorStream*GetStreamByIndex(UInt32 inIndex) { return fStreamArray[inIndex]; }
        void AddBroadcasterClientSession(QTSS_StandardRTSP_Params* inParams);
        QTSS_ClientSessionObject GetBroadcasterSession() { return fBroadcasterSession;}

        // For the QTSSSplitterModule, this object can cache a QTSS_StreamRef
        void            SetSocketStream(QTSS_StreamRef inStream)    { fSocketStream = inStream; }
        QTSS_StreamRef  GetSocketStream()                           { return fSocketStream; }
        
        // A ReflectorSession keeps track of the aggregate bit rate each stream is reflecting (RTP only).
        // Initially, this will return 0
        // until enough time passes to compute an accurate average.
        UInt32          GetBitRate();
        
        // Returns true if this SourceInfo structure is equivalent to this ReflectorSession.
        Bool16 Equal(SourceInfo* inInfo);
        
        // Each stream has a cookie associated with it. When the stream writes a packet to an output, 
        // this cookie is used to identify which stream is writing the packet.
        // The below function is useful so outputs can get the cookie value for a stream ID,
        // and therefore mux the cookie to the right output stream.
        void*   GetStreamCookie(UInt32 inStreamID);
    
        //Reflector quality levels:
        enum
        {
            kMaxHTMLSize = 128,
            kAudioOnlyQuality = 1,      //UInt32
            kNormalQuality = 0,         //UInt32
            kNumQualityLevels = 2       //UInt32
        };
        
        SInt64  GetInitTimeMS()   { return fInitTimeMS; }

       void SetHasBufferedStreams(Bool16 enableBuffer) { fHasBufferedStreams = enableBuffer; } 
     
    private:
    
        // session setup or not
        Bool16      fIsSetup;

        // For storage in the session map       
        OSRef       fRef;
        StrPtrLen   fSourceID;
        OSQueueElem fQueueElem; // Relay uses this.
        
        unsigned int        fNumOutputs;
        
        ReflectorStream**   fStreamArray;
        
        char        fHTMLBuf[kMaxHTMLSize];
        StrPtrLen   fSourceInfoHTML;
        ResizeableStringFormatter fFormatter;
        
        // The reflector session needs to hang onto the source info object for it's entire lifetime. Right now, this is used for reflector-as-client.
        SourceInfo* fSourceInfo;
        StrPtrLen fLocalSDP;
        
        // For the QTSSSplitterModule, this object can cache a QTSS_StreamRef
        QTSS_StreamRef fSocketStream;
        QTSS_ClientSessionObject fBroadcasterSession;
        SInt64      fInitTimeMS;

        Bool16      fHasBufferedStreams;         
         
};

#endif

