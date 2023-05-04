/*
A class called RCFSourceInfo is defined, 
inheriting from the SourceInfo class. 
The role of this class is to construct the StreamInfo structure by parsing the SDP data. 
This class contains functions for parsing the SDP file and constructing the OutputInfo structure.
Among other things, 
the SetName function is used to set the name of RCFSourceInfo and the Name function returns 
the name of RCFSourceInfo. 
The class also has protected functions 
such as ParseDestination and ParseAnnouncedDestination for parsing targets 
and announced targets. Finally, the class also defines a pointer of type char, fName, 
as the name of the RCFSourceInfo.
*/
/*
    File:       RCFSourceInfo.h

    Contains:   This object takes input RCF data, and uses it to support the SourceInfo
                API.

    

*/

#ifndef __RCF_SOURCE_INFO_H__
#define __RCF_SOURCE_INFO_H__

#include "StrPtrLen.h"
#include "SourceInfo.h"
#include "XMLParser.h"
#include "StringParser.h"

class RCFSourceInfo : public SourceInfo
{
    public:
    
        // Uses the SDP Data to build up the StreamInfo structures
        RCFSourceInfo() : fName(NULL) {}
        RCFSourceInfo(XMLTag* relayTag) : fName(NULL) { Parse(relayTag); }
        RCFSourceInfo(const RCFSourceInfo& copy):SourceInfo(copy) { this->SetName(copy.fName); }
        virtual ~RCFSourceInfo();
        
        // Parses out the SDP file provided, sets up the StreamInfo structures
        void    Parse(XMLTag* relayTag);
        
        // Parses relay_destination lines and builds OutputInfo structs
        void    ParseRelayDestinations(XMLTag* relayTag);
        
                void    SetName(const char* inName);
                char*   Name() { return fName; }
                
    protected:
        virtual void ParseDestination(XMLTag* destTag, UInt32 index);
        virtual void ParseAnnouncedDestination(XMLTag* destTag, UInt32 index);
        virtual void AllocateOutputArray(UInt32 numOutputs);
                
                char*   fName;

};
#endif // __SDP_SOURCE_INFO_H__


