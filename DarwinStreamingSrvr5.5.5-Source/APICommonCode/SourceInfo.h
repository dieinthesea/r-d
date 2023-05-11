#ifndef __SOURCE_INFO_H__
#define __SOURCE_INFO_H__

#include "QTSS.h"
#include "StrPtrLen.h"
#include "OSQueue.h"
#include "OS.h"

class SourceInfo
{
    public:
    
        SourceInfo() :  fStreamArray(NULL), fNumStreams(0),
                        fOutputArray(NULL), fNumOutputs(0),
                        fTimeSet(false),fStartTimeUnixSecs(0),fEndTimeUnixSecs(0),
                        fSessionControlType(kRTSPSessionControl)  {}
        SourceInfo(const SourceInfo& copy);// Does copy dynamically allocated data
        virtual ~SourceInfo(); // Deletes the dynamically allocated data
        
        enum
        {
            eDefaultBufferDelay = 3
        };
        
        // Returns whether this source is reflectable.
        Bool16  IsReflectable();

        // Each source is comprised of a set of streams. Those streams have the following metadata.
        struct StreamInfo
        {
            StreamInfo() : fSrcIPAddr(0), fDestIPAddr(0), fPort(0), fTimeToLive(0), fPayloadType(0), fPayloadName(NULL), fTrackID(0), fBufferDelay((Float32) eDefaultBufferDelay), fIsTCP(false),fSetupToReceive(false), fTimeScale(0){}
            ~StreamInfo(); // Deletes the memory allocated for the fPayloadName string 
            
            void Copy(const StreamInfo& copy);// Does copy dynamically allocated data
            
            UInt32 fSrcIPAddr;  // Src IP address of content (this may be 0 if not known for sure)
            UInt32 fDestIPAddr; // Dest IP address of content (destination IP addr for source broadcast!)
            UInt16 fPort;       // Dest (RTP) port of source content
            UInt16 fTimeToLive; // Ttl for this stream
            QTSS_RTPPayloadType fPayloadType;   // Payload type of this stream
            StrPtrLen fPayloadName; // Payload name of this stream
            UInt32 fTrackID;    // ID of this stream
            Float32 fBufferDelay; // buffer delay (default is 3 seconds)
            Bool16  fIsTCP;     // Is this a TCP broadcast? If this is the case, the port and ttl are not valid
            Bool16  fSetupToReceive;    // If true then a push to the server is setup on this stream.
            UInt32  fTimeScale;
        };

        UInt32      GetNumStreams() { return fNumStreams; }
        StreamInfo* GetStreamInfo(UInt32 inStreamIndex);
        StreamInfo* GetStreamInfoByTrackID(UInt32 inTrackID);

        struct OutputInfo
        {
            OutputInfo() : fDestAddr(0), fLocalAddr(0), fTimeToLive(0), fPortArray(NULL), fNumPorts(0), fBasePort(0), fAlreadySetup(false) {}
            ~OutputInfo(); // Deletes the memory allocated for fPortArray
            
            // Returns true if the two are equal
            Bool16 Equal(const OutputInfo& info);
            
            void Copy(const OutputInfo& copy);// Does copy dynamically allocated data

            UInt32 fDestAddr;       // Destination address to forward the input onto
            UInt32 fLocalAddr;      // Address of local interface to send out on (may be 0)
            UInt16 fTimeToLive;     // Time to live for resulting output (if multicast)
            UInt16* fPortArray;     // 1 destination RTP port for each Stream.
            UInt32 fNumPorts;       // Size of the fPortArray (usually equal to fNumStreams)
            UInt16 fBasePort;       // The base destination RTP port - for i=1 to fNumStreams fPortArray[i] = fPortArray[i-1] + 2
            Bool16  fAlreadySetup;  // A flag used in QTSSReflectorModule.cpp
        };

        // Returns the number of OutputInfo objects.
        UInt32      GetNumOutputs() { return fNumOutputs; }
        UInt32      GetNumNewOutputs(); // Returns # of outputs not already setup

        OutputInfo* GetOutputInfo(UInt32 inOutputIndex);

        virtual char*   GetLocalSDP(UInt32* /*newSDPLen*/) { return NULL; }
        
        // This is only supported by the RTSPSourceInfo sub class
        virtual Bool16 IsRTSPSourceInfo() { return false; }
        
                // This is only supported by the RCFSourceInfo sub class and its derived classes
                virtual char*   Name()  { return NULL; }
                
        virtual Bool16 Equal(SourceInfo* inInfo);

        time_t  NTPSecs_to_UnixSecs(time_t time) {return (time_t) (time - (UInt32)kNTP_Offset_From_1970);}
        UInt32  UnixSecs_to_NTPSecs(time_t time) {return (UInt32) (time + (UInt32)kNTP_Offset_From_1970);}
        Bool16  SetActiveNTPTimes(UInt32 startNTPTime,UInt32 endNTPTime);
        Bool16  IsValidNTPSecs(UInt32 time) {return time >= (UInt32) kNTP_Offset_From_1970 ? true : false;}
        Bool16  IsPermanentSource() { return ((fStartTimeUnixSecs == 0) && (fEndTimeUnixSecs == 0)) ? true : false; }
        Bool16  IsActiveTime(time_t unixTimeSecs);
        Bool16  IsActiveNow() { return IsActiveTime(OS::UnixTime_Secs()); }
        Bool16  IsRTSPControlled() {return (fSessionControlType == kRTSPSessionControl) ? true : false; }
        Bool16  HasTCPStreams();
        Bool16  HasIncomingBroacast();
        time_t  GetStartTimeUnixSecs() {return fStartTimeUnixSecs; }
        time_t  GetEndTimeUnixSecs() {return fEndTimeUnixSecs; }
        UInt32  GetDurationSecs();
        enum {kSDPTimeControl, kRTSPSessionControl};
    protected:
        
        //utility function used by IsReflectable
        Bool16 IsReflectableIPAddr(UInt32 inIPAddr);

        StreamInfo* fStreamArray;
        UInt32      fNumStreams;
        
        OutputInfo* fOutputArray;
        UInt32      fNumOutputs;
        
        Bool16      fTimeSet;
        time_t      fStartTimeUnixSecs;
        time_t      fEndTimeUnixSecs;
            
        UInt32      fSessionControlType;
        Bool16      fHasValidTime;
};



#endif //__SOURCE_INFO_H__
