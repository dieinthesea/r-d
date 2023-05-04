/*
RTPSessionOutput inherits ReflectorOutput 
and includes some other classes and header files. 
The main function of this class is to write RTP packets into the correct RTPStreamObject, 
and track and filter RTP and RTCP packets when needed. 
It includes functions such as WritePacket and PacketMatchesStream, 
which are used to write and match data packets.
*/
/*
    File:       RTSPReflectorOutput.h

    Contains:   Derived from ReflectorOutput, this implements the WritePacket
                method in terms of the QTSS API (that is, it writes to a client
                using the QTSS_RTPSessionObject
                    


*/

#ifndef __RTSP_REFLECTOR_OUTPUT_H__
#define __RTSP_REFLECTOR_OUTPUT_H__

#include "ReflectorOutput.h"
#include "ReflectorSession.h"
#include "QTSS.h"

class RTPSessionOutput : public ReflectorOutput
{
    public:
    
        // Adds some dictionary attributes
        static void Register();
        
        RTPSessionOutput(QTSS_ClientSessionObject inRTPSession, ReflectorSession* inReflectorSession,
                            QTSS_Object serverPrefs, QTSS_AttributeID inCookieAddrID);
        virtual ~RTPSessionOutput() {}
        
        ReflectorSession* GetReflectorSession() { return fReflectorSession; }
        
        // This writes the packet out to the proper QTSS_RTPStreamObject.
        // If this function returns QTSS_WouldBlock, timeToSendThisPacketAgain will
        // be set to # of msec in which the packet can be sent, or -1 if unknown
        virtual QTSS_Error  WritePacket(StrPtrLen* inPacketData, void* inStreamCookie, UInt32 inFlags, SInt64 packetLatenessInMSec, SInt64* timeToSendThisPacketAgain, UInt64* packetIDPtr, SInt64* arrivalTimeMSec );
        virtual void TearDown();
        
        SInt64                  GetReflectorSessionInitTime()                    { return fReflectorSession->GetInitTimeMS(); }
        
        virtual Bool16  IsUDP();
        
        virtual Bool16  IsPlaying();
        
    private:
    
        QTSS_ClientSessionObject fClientSession;
        ReflectorSession*       fReflectorSession;
        QTSS_AttributeID        fCookieAttrID;
        UInt32                  fBufferDelayMSecs;
        SInt64                  fBaseArrivalTime;
        Bool16                  fIsUDP;
        Bool16                  fTransportInitialized;
        Bool16                  fMustSynch;
        Bool16                  fPreFilter;
        
        UInt16 GetPacketSeqNumber(StrPtrLen* inPacket);
        void SetPacketSeqNumber(StrPtrLen* inPacket, UInt16 inSeqNumber);
        Bool16 PacketShouldBeThinned(QTSS_RTPStreamObject inStream, StrPtrLen* inPacket);
        Bool16  FilterPacket(QTSS_RTPStreamObject *theStreamPtr, StrPtrLen* inPacket);
        
        UInt32 GetPacketRTPTime(StrPtrLen* packetStrPtr);
inline  Bool16 PacketMatchesStream(void* inStreamCookie, QTSS_RTPStreamObject *theStreamPtr);
        Bool16 PacketReadyToSend(QTSS_RTPStreamObject *theStreamPtr,SInt64 *currentTimePtr, UInt32 inFlags, UInt64* packetIDPtr, SInt64* timeToSendThisPacketAgainPtr);
        Bool16 PacketAlreadySent(QTSS_RTPStreamObject *theStreamPtr, UInt32 inFlags, UInt64* packetIDPtr);
        QTSS_Error TrackRTCPBaseTime(QTSS_RTPStreamObject *theStreamPtr, StrPtrLen* inPacketStrPtr, SInt64 *currentTimePtr, UInt32 inFlags, SInt64 *packetLatenessInMSec, SInt64* timeToSendThisPacketAgain, UInt64* packetIDPtr, SInt64* arrivalTimeMSecPtr);
        QTSS_Error RewriteRTCP(QTSS_RTPStreamObject *theStreamPtr, StrPtrLen* inPacketStrPtr, SInt64 *currentTimePtr, UInt32 inFlags, SInt64 *packetLatenessInMSec, SInt64* timeToSendThisPacketAgain, UInt64* packetIDPtr, SInt64* arrivalTimeMSecPtr);
        QTSS_Error TrackRTPPackets(QTSS_RTPStreamObject *theStreamPtr, StrPtrLen* inPacketStrPtr, SInt64 *currentTimePtr, UInt32 inFlags, SInt64 *packetLatenessInMSec, SInt64* timeToSendThisPacketAgain, UInt64* packetIDPtr, SInt64* arrivalTimeMSecPtr);
        QTSS_Error TrackRTCPPackets(QTSS_RTPStreamObject *theStreamPtr, StrPtrLen* inPacketStrPtr, SInt64 *currentTimePtr, UInt32 inFlags, SInt64 *packetLatenessInMSec, SInt64* timeToSendThisPacketAgain, UInt64* packetIDPtr, SInt64* arrivalTimeMSecPtr);
        QTSS_Error TrackPackets(QTSS_RTPStreamObject *theStreamPtr, StrPtrLen* inPacketStrPtr, SInt64 *currentTimePtr, UInt32 inFlags, SInt64 *packetLatenessInMSec, SInt64* timeToSendThisPacketAgain, UInt64* packetIDPtr, SInt64* arrivalTimeMSecPtr);
};


Bool16 RTPSessionOutput::PacketMatchesStream(void* inStreamCookie, QTSS_RTPStreamObject *theStreamPtr)
{
    void** theStreamCookie = NULL;
    UInt32 theLen = 0;
    (void) QTSS_GetValuePtr(*theStreamPtr, fCookieAttrID, 0, (void**)&theStreamCookie, &theLen);   

    if ((theStreamCookie != NULL) && (*theStreamCookie == inStreamCookie))
        return true;
    
    return false;
}
#endif //__RTSP_REFLECTOR_OUTPUT_H__
