#include "SDPSourceInfo.h"

#include "StringParser.h"
#include "StringFormatter.h"
#include "OSMemory.h"
#include "SocketUtils.h"
#include "StrPtrLen.h"
#include "SDPUtils.h"
#include "OSArrayObjectDeleter.h"

static StrPtrLen    sCLine("c=IN IP4 0.0.0.0");
static StrPtrLen    sControlLine("a=control:*");
static StrPtrLen    sVideoStr("video");
static StrPtrLen    sAudioStr("audio");
static StrPtrLen    sRtpMapStr("rtpmap");
static StrPtrLen    sControlStr("control");
static StrPtrLen    sBufferDelayStr("x-bufferdelay");
static StrPtrLen    sBroadcastControlStr("x-broadcastcontrol");
static StrPtrLen    sAutoDisconnect("RTSP");
static StrPtrLen    sAutoDisconnectTime("TIME");

SDPSourceInfo::~SDPSourceInfo()
{
    fSDPData.Delete();
}

char* SDPSourceInfo::GetLocalSDP(UInt32* newSDPLen)
{
    Assert(fSDPData.Ptr != NULL);

    Bool16 appendCLine = true;
    UInt32 trackIndex = 0;
    
    char *localSDP = NEW char[fSDPData.Len * 2];
    OSCharArrayDeleter charArrayPathDeleter(localSDP);
    StringFormatter localSDPFormatter(localSDP, fSDPData.Len * 2);

    StrPtrLen sdpLine;
    StringParser sdpParser(&fSDPData);
    char trackIndexBuffer[50];

    Bool16 hasControlLine = false;

    while (sdpParser.GetDataRemaining() > 0)
    {
        //stop when we reach an empty line.
        sdpParser.GetThruEOL(&sdpLine);
        if (sdpLine.Len == 0)
            continue;
            
        switch (*sdpLine.Ptr)
        {
            case 'c':
                break;//ignore connection information
            case 'm':
            {
                //append new connection information right before the first 'm'
                if (appendCLine)
                {
                    localSDPFormatter.Put(sCLine);
                    localSDPFormatter.PutEOL();
                   
                    if (!hasControlLine)
                    { 
                      localSDPFormatter.Put(sControlLine);
                      localSDPFormatter.PutEOL();
                    }
                    
                    appendCLine = false;
                }
                //the last "a=" for each m should be the control a=
                if ((trackIndex > 0) && (!hasControlLine))
                {
                    qtss_sprintf(trackIndexBuffer, "a=control:trackID=%ld\r\n",trackIndex);
                    localSDPFormatter.Put(trackIndexBuffer, ::strlen(trackIndexBuffer));
                }
                //now write the 'm' line, but strip off the port information
                StringParser mParser(&sdpLine);
                StrPtrLen mPrefix;
                mParser.ConsumeUntil(&mPrefix, StringParser::sDigitMask);
                localSDPFormatter.Put(mPrefix);
                localSDPFormatter.Put("0", 1);
                (void)mParser.ConsumeInteger(NULL);
                localSDPFormatter.Put(mParser.GetCurrentPosition(), mParser.GetDataRemaining());
                localSDPFormatter.PutEOL();
                trackIndex++;
                break;
            }
            case 'a':
            {
                StringParser aParser(&sdpLine);
                aParser.ConsumeLength(NULL, 2);//go past 'a='
                StrPtrLen aLineType;
                aParser.ConsumeWord(&aLineType);
                if (aLineType.Equal(sControlStr))
                {
                    aParser.ConsumeUntil(NULL, '=');
                    aParser.ConsumeUntil(NULL, StringParser::sDigitMask);
                    
                   StrPtrLen aDigitType;                
                   (void)aParser.ConsumeInteger(&aDigitType);
                    if (aDigitType.Len > 0)
                    {
                      localSDPFormatter.Put("a=control:trackID=", 18);
                      localSDPFormatter.Put(aDigitType);
                      localSDPFormatter.PutEOL();
                      hasControlLine = true;
                      break;
                    }
                }
               
                localSDPFormatter.Put(sdpLine);
                localSDPFormatter.PutEOL();
                break;
            }
            default:
            {
                localSDPFormatter.Put(sdpLine);
                localSDPFormatter.PutEOL();
            }
        }
    }
    
    if ((trackIndex > 0) && (!hasControlLine))
    {
        qtss_sprintf(trackIndexBuffer, "a=control:trackID=%ld\r\n",trackIndex);
        localSDPFormatter.Put(trackIndexBuffer, ::strlen(trackIndexBuffer));
    }
    *newSDPLen = (UInt32)localSDPFormatter.GetCurrentOffset();
    
    StrPtrLen theSDPStr(localSDP, *newSDPLen);//localSDP is not 0 terminated so initialize theSDPStr with the len.
    SDPContainer rawSDPContainer; 
    (void) rawSDPContainer.SetSDPBuffer( &theSDPStr );    
    SDPLineSorter sortedSDP(&rawSDPContainer);

    return sortedSDP.GetSortedSDPCopy(); // return a new copy of the sorted SDP
}


void SDPSourceInfo::Parse(char* sdpData, UInt32 sdpLen)
{
    if (fSDPData.Ptr != NULL)
        return;
        
    Assert(fStreamArray == NULL);
    
    char *sdpDataCopy = NEW char[sdpLen];
    Assert(sdpDataCopy != NULL);
    
    memcpy(sdpDataCopy,sdpData, sdpLen);
    fSDPData.Set(sdpDataCopy, sdpLen);

    UInt32 currentTrack = 1;
    
    Bool16 hasGlobalStreamInfo = false;
    StreamInfo theGlobalStreamInfo; 

    StrPtrLen sdpLine;
    StringParser trackCounter(&fSDPData);
    StringParser sdpParser(&fSDPData);
    UInt32 theStreamIndex = 0;

    while (trackCounter.GetDataRemaining() > 0)
    {
        //each 'm' line in the SDP file corresponds to another track.
        trackCounter.GetThruEOL(&sdpLine);
        if ((sdpLine.Len > 0) && (sdpLine.Ptr[0] == 'm'))
            fNumStreams++;  
    }

    
    fStreamArray = NEW StreamInfo[fNumStreams];

    // set the default destination as our default IP address and set the default ttl
    theGlobalStreamInfo.fDestIPAddr = INADDR_ANY;
    theGlobalStreamInfo.fTimeToLive = kDefaultTTL;

    theGlobalStreamInfo.fBufferDelay = (Float32) eDefaultBufferDelay;
    
    while (sdpParser.GetDataRemaining() > 0)
    {
        sdpParser.GetThruEOL(&sdpLine);
        if (sdpLine.Len == 0)
            continue;//skip over any blank lines

        switch (*sdpLine.Ptr)
        {
            case 't':
            {
                StringParser mParser(&sdpLine);
                                
                mParser.ConsumeUntil(NULL, StringParser::sDigitMask);
                UInt32 ntpStart = mParser.ConsumeInteger(NULL);
                
                mParser.ConsumeUntil(NULL, StringParser::sDigitMask);               
                UInt32 ntpEnd = mParser.ConsumeInteger(NULL);
                
                SetActiveNTPTimes(ntpStart,ntpEnd);
            }
            break;
            
            case 'm':
            {
                if (hasGlobalStreamInfo)
                {
                    fStreamArray[theStreamIndex].fDestIPAddr = theGlobalStreamInfo.fDestIPAddr;
                    fStreamArray[theStreamIndex].fTimeToLive = theGlobalStreamInfo.fTimeToLive;
                }
                fStreamArray[theStreamIndex].fTrackID = currentTrack;
                currentTrack++;
                
                StringParser mParser(&sdpLine);
                
                //find out what type of track this is
                mParser.ConsumeLength(NULL, 2);//go past 'm='
                StrPtrLen theStreamType;
                mParser.ConsumeWord(&theStreamType);
                if (theStreamType.Equal(sVideoStr))
                    fStreamArray[theStreamIndex].fPayloadType = qtssVideoPayloadType;
                else if (theStreamType.Equal(sAudioStr))
                    fStreamArray[theStreamIndex].fPayloadType = qtssAudioPayloadType;
                    
                //find the port for this stream
                mParser.ConsumeUntil(NULL, StringParser::sDigitMask);
                SInt32 tempPort = mParser.ConsumeInteger(NULL);
                if ((tempPort > 0) && (tempPort < 65536))
                    fStreamArray[theStreamIndex].fPort = (UInt16) tempPort;
                    
                // find out whether this is TCP or UDP
                mParser.ConsumeWhitespace();
                StrPtrLen transportID;
                mParser.ConsumeWord(&transportID);
                
                static const StrPtrLen kTCPTransportStr("RTP/AVP/TCP");
                if (transportID.Equal(kTCPTransportStr))
                    fStreamArray[theStreamIndex].fIsTCP = true;
                    
                theStreamIndex++;
            }
            break;
            case 'a':
            {
                StringParser aParser(&sdpLine);

                aParser.ConsumeLength(NULL, 2);//go past 'a='

                StrPtrLen aLineType;

                aParser.ConsumeWord(&aLineType);



                if (aLineType.Equal(sBroadcastControlStr))

                {   
                    aParser.ConsumeUntil(NULL,StringParser::sWordMask);

                    StrPtrLen sessionControlType;

                    aParser.ConsumeWord(&sessionControlType);

                    if (sessionControlType.Equal(sAutoDisconnect))
                    {
                       fSessionControlType = kRTSPSessionControl; 
                    }       
                    else if (sessionControlType.Equal(sAutoDisconnectTime))
                    {
                       fSessionControlType = kSDPTimeControl; 
                    }       
                    

                }

                //if we haven't even hit an 'm' line yet, just ignore all 'a' lines
                if (theStreamIndex == 0)
                    break;
                    
                if (aLineType.Equal(sRtpMapStr))
                {
                    if ((fStreamArray[theStreamIndex - 1].fPayloadName.Len == 0) &&
                        (aParser.GetThru(NULL, ' ')))
                    {
                        StrPtrLen payloadNameFromParser;
                        (void)aParser.GetThruEOL(&payloadNameFromParser);
                                                char* temp = payloadNameFromParser.GetAsCString();
                        (fStreamArray[theStreamIndex - 1].fPayloadName).Set(temp, payloadNameFromParser.Len);
                    }
                }
                else if (aLineType.Equal(sControlStr))
                {           
                    //mark the trackID if that's what this line has
                    aParser.ConsumeUntil(NULL, '=');
                    aParser.ConsumeUntil(NULL, StringParser::sDigitMask);
                    fStreamArray[theStreamIndex - 1].fTrackID = aParser.ConsumeInteger(NULL);
                }
                else if (aLineType.Equal(sBufferDelayStr))
                {   // if a BufferDelay is found then set all of the streams to the same buffer delay (it's global)
                    aParser.ConsumeUntil(NULL, StringParser::sDigitMask);
                    theGlobalStreamInfo.fBufferDelay = aParser.ConsumeFloat();
                }

            }
            break;
            case 'c':
            {
                //get the IP address off this header
                StringParser cParser(&sdpLine);
                cParser.ConsumeLength(NULL, 9);//strip off "c=in ip4 "
                UInt32 tempIPAddr = SDPSourceInfo::GetIPAddr(&cParser, '/');
                                
                //grab the ttl
                SInt32 tempTtl = kDefaultTTL;
                if (cParser.GetThru(NULL, '/'))
                {
                    tempTtl = cParser.ConsumeInteger(NULL);
                    Assert(tempTtl >= 0);
                    Assert(tempTtl < 65536);
                }

                if (theStreamIndex > 0)
                {
                    //if this c= line is part of a stream, it overrides the
                    //global stream information
                    fStreamArray[theStreamIndex - 1].fDestIPAddr = tempIPAddr;
                    fStreamArray[theStreamIndex - 1].fTimeToLive = (UInt16) tempTtl;
                } else {
                    theGlobalStreamInfo.fDestIPAddr = tempIPAddr;
                    theGlobalStreamInfo.fTimeToLive = (UInt16) tempTtl;
                    hasGlobalStreamInfo = true;
                }
            }
        }
    }       
    
    // Add the default buffer delay
    Float32 bufferDelay = (Float32) eDefaultBufferDelay;
    if (theGlobalStreamInfo.fBufferDelay != (Float32) eDefaultBufferDelay)
        bufferDelay = theGlobalStreamInfo.fBufferDelay;
    
    UInt32 count = 0;
    while (count < fNumStreams)
    {   fStreamArray[count].fBufferDelay = bufferDelay;
        count ++;
    }
        
}

UInt32 SDPSourceInfo::GetIPAddr(StringParser* inParser, char inStopChar)
{
    StrPtrLen ipAddrStr;

    // Get the IP addr str
    inParser->ConsumeUntil(&ipAddrStr, inStopChar);
    
    if (ipAddrStr.Len == 0)
        return 0;
    
    // NULL terminate it
    char endChar = ipAddrStr.Ptr[ipAddrStr.Len];
    ipAddrStr.Ptr[ipAddrStr.Len] = '\0';
    
    //inet_addr returns numeric IP addr in network byte order, make
    //sure to convert to host order.
    UInt32 ipAddr = SocketUtils::ConvertStringToAddr(ipAddrStr.Ptr);
    
    // Make sure to put the old char back!
    ipAddrStr.Ptr[ipAddrStr.Len] = endChar;

    return ipAddr;
}

