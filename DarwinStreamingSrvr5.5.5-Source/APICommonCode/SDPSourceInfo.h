/*
This code defines a class called SDPSourceInfo.
It inherits from the SourceInfo class.
It contains a constructor that constructs a StreamInfo structure using SDP data.
It provides methods for parsing SDP files and getting local SDP files.
In addition, it has a GetSDPData method for getting SDP data.
In the private part of the class, it defines an enumeration constant kDefaultTTL.
and declares a StrPtrLen object called fSDPData.
*/
/*
    File:       SDPSourceInfo.h

    Contains:   This object takes input SDP data, and uses it to support the SourceInfo
                API.
*/

#ifndef __SDP_SOURCE_INFO_H__
#define __SDP_SOURCE_INFO_H__

#include "StrPtrLen.h"
#include "SourceInfo.h"
#include "StringParser.h"

class SDPSourceInfo : public SourceInfo
{
    public:
    
        // Uses the SDP Data to build up the StreamInfo structures
        SDPSourceInfo(char* sdpData, UInt32 sdpLen) { Parse(sdpData, sdpLen); }
        SDPSourceInfo() {}
        virtual ~SDPSourceInfo();
        
        // Parses out the SDP file provided, sets up the StreamInfo structures
        void    Parse(char* sdpData, UInt32 sdpLen);

        // This function uses the Parsed SDP file, and strips out all the network information,
        // producing an SDP file that appears to be local.
        virtual char*   GetLocalSDP(UInt32* newSDPLen);

        // Returns the SDP data
        StrPtrLen*  GetSDPData()    { return &fSDPData; }
        
        // Utility routines
        
        // Assuming the parser is currently pointing at the beginning of an dotted-
        // decimal IP address, this consumes it (stopping at inStopChar), and returns
        // the IP address (host ordered) as a UInt32
        static UInt32 GetIPAddr(StringParser* inParser, char inStopChar);
      
    private:

        enum
        {
            kDefaultTTL = 15    //UInt16
        };
        StrPtrLen   fSDPData;
};
#endif // __SDP_SOURCE_INFO_H__

