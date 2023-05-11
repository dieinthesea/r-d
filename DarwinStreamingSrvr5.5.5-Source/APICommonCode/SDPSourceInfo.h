class SDPSourceInfo : public SourceInfo
{
    public:
    
        // Uses the SDP Data to build up the StreamInfo structures
        SDPSourceInfo(char* sdpData, UInt32 sdpLen) { Parse(sdpData, sdpLen); }
        SDPSourceInfo() {}
        virtual ~SDPSourceInfo();
        
        // Parses out the SDP file provided, sets up the StreamInfo structures
        void    Parse(char* sdpData, UInt32 sdpLen);

        // This function uses the Parsed SDP file, and strips out all the network information,producing an SDP file that appears to be local.
        virtual char*   GetLocalSDP(UInt32* newSDPLen);

        // Returns the SDP data
        StrPtrLen*  GetSDPData()    { return &fSDPData; }
        static UInt32 GetIPAddr(StringParser* inParser, char inStopChar);
      
    private:

        enum
        {
            kDefaultTTL = 15    //UInt16
        };
        StrPtrLen   fSDPData;
};
#endif // __SDP_SOURCE_INFO_H__

