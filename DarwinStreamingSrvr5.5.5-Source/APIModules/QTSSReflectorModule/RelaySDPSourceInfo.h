/*
    File:       RelaySDPSourceInfo.h

    Contains:   This object takes input SDP data, and uses it to support the SourceInfo
                API. It looks for the x-qt-relay lines put in by some broadcasters,
                and uses that information to construct OutputInfo objects.


*/

#ifndef __RELAY_SDP_SOURCE_INFO_H__
#define __RELAY_SDP_SOURCE_INFO_H__

#include "StrPtrLen.h"
#include "SourceInfo.h"

class RelaySDPSourceInfo : public SourceInfo
{
    public:
    
        // Reads in the SDP data from this file, and builds up the SourceInfo structures
        RelaySDPSourceInfo(StrPtrLen* inSDPData) { Parse(inSDPData); }
        virtual ~RelaySDPSourceInfo();
        
    private:
        
        void    Parse(StrPtrLen* inSDPData);
};
#endif // __SDP_SOURCE_INFO_H__


