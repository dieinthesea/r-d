/*
RelayOutput, which inherits from the ReflectorOutput class. 
The role of the RelayOutput class is to implement forwarding an RTSP/RTP stream to another server
*/
/*
    File:       RelayOutput.h

    Contains:   An implementation of the ReflectorOutput abstract base class,
                that just writes data out to UDP sockets.

*/

#ifndef __RELAY_OUTPUT_H__
#define __RELAY_OUTPUT_H__

#include "ReflectorOutput.h"
#include "RelaySession.h"
#include "SourceInfo.h"
#include "RTSPClient.h"
#include "ClientSocket.h"

#include "OSQueue.h"
#include "OSMutex.h"

class RelayAnnouncer;

class RelayOutput : public ReflectorOutput
{
    public:
    
        static void Register();
        
        RelayOutput(SourceInfo* inInfo, UInt32 inWhichOutput, RelaySession* inSession, Bool16 isRTSPSourceInfo);
        virtual ~RelayOutput();
        
        // Used to compare whether two RelayOutput objects are equal
        // Also marks the proper SourceInfo::OutputInfo "fAlreadySetup" flag as true 
        Bool16  Equal(SourceInfo* inInfo);
        
        // Used to bind a UDP socket.
        OS_Error BindSocket();
        
        // Used to write an RTP packet to a UDP socket.
        virtual QTSS_Error  WritePacket(StrPtrLen* inPacket, void* inStreamCookie, UInt32 inFlags, SInt64 packetLatenessInMSec,  SInt64* timeToSendThisPacketAgain, UInt64* packetIDPtr, SInt64* arrivalTime);
        
        virtual Bool16              IsUDP() { return true; }
        
        virtual Bool16              IsPlaying() {if (!fValid || fDoingAnnounce) return false; return true; }
        
        // ACCESSORS
        
        RelaySession*   GetRelaySession() { return fRelaySession; }
        StrPtrLen*          GetOutputInfoHTML() { return &fOutputInfoHTML; }

        UInt32              GetCurPacketsPerSecond() { return fPacketsPerSecond; }
        UInt32              GetCurBitsPerSecond()    { return fBitsPerSecond; }//Gets the current RTP packet size transmitted per second (in bits).
        UInt64&             GetTotalPacketsSent()    { return fTotalPacketsSent; }//The total number of RTP packets obtained for transmission.
        UInt64&             GetTotalBytesSent()      { return fTotalBytesSent; }//The total size of RTP packets obtained for transmission
        Bool16              IsValid()               { return fValid; }
        
      
        static OSMutex* GetQueueMutex() { return &sQueueMutex; }
        static OSQueue* GetOutputQueue(){ return &sRelayOutputQueue; }
        void TearDown() {};//Used to clean up the current RelayOutput object.

        SInt64 RunAnnounce();
        
    private:
        
        void SetupRelayOutputObject(RTSPOutputInfo* inRTSPInfo);
        
        class RelayAnnouncer : public Task
        {
            public:
                RelayAnnouncer(RelayOutput* output) : fOutput(output) {this->SetTaskName("RelayAnnouncer");}
                
                virtual SInt64 Run();
                RelayOutput* fOutput;
        };

        enum
        {
            kMaxHTMLSize = 255, // Note, this may be too short and we don't protect!
            kStatsIntervalInMilSecs = 10000 // Update "current" statistics every 10 seconds
        };
    
        RelaySession* fRelaySession;

        // Relay streams all share this one socket for writing.
        UDPSocket   fOutputSocket;
        UInt32      fNumStreams;
        SourceInfo::OutputInfo fOutputInfo;
        void**      fStreamCookieArray;//Each stream has a cookie
        UInt32*     fTrackIDArray;
        
        OSQueueElem fQueueElem;

        char        fHTMLBuf[kMaxHTMLSize];
        StrPtrLen   fOutputInfoHTML;
        ResizeableStringFormatter fFormatter;
        
        // Statistics
        UInt32      fPacketsPerSecond;
        UInt32      fBitsPerSecond;
        
        SInt64      fLastUpdateTime;
        UInt64      fTotalPacketsSent;
        UInt64      fTotalBytesSent;
        UInt64      fLastPackets;
        UInt64      fLastBytes;
        
        TCPClientSocket* fClientSocket;
        RTSPClient* fClient;
        Bool16      fDoingAnnounce;
        Bool16      fValid;
        char*       fOutgoingSDP;
        RelayAnnouncer* fAnnounceTask;
        
                RTSPOutputInfo* fRTSPOutputInfo;
                
        enum    // anounce states
        {
            kSendingAnnounce    = 0,
            kSendingSetup       = 1,
            kSendingPlay        = 2,
            kDone               = 3
        };
        UInt32          fAnnounceState;
        UInt32          fCurrentSetup;

        // Queue of all current RelayReflectorOutput objects, for use in the        
        static OSQueue  sRelayOutputQueue;
        static OSMutex  sQueueMutex;
        
        QTSS_Object                 fRelaySessionObject;
        QTSS_Object                 fRelayOutputObject;
        
        // attributes of the qtssRelayOutputObjectType
        static QTSS_ObjectType          qtssRelayOutputObjectType;

        static QTSS_AttributeID         sOutputType;
        static QTSS_AttributeID         sOutputDestAddr;
        static QTSS_AttributeID         sOutputLocalAddr;
        static QTSS_AttributeID         sOutputUDPPorts;
        static QTSS_AttributeID         sOutputRTSPPort;
        static QTSS_AttributeID         sOutputURL;
                static QTSS_AttributeID         sOutputTTL;
        static QTSS_AttributeID         sOutputCurPacketsPerSec;
        static QTSS_AttributeID         sOutputCurBitsPerSec;
        static QTSS_AttributeID         sOutputTotalPacketsSent;
        static QTSS_AttributeID         sOutputTotalBytesSent;
};
        
        
#endif //__RELAY_OUTPUT_H__
