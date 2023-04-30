#ifndef _OS_H_
#define _OS_H_


#include "OSHeaders.h"
#include "OSMutex.h"
#include <string.h>

class OS
{
    public:
    
        //initialize
        static void Initialize();

        static SInt32 Min(SInt32 a, SInt32 b)   { if (a < b) return a; return b; }
        
        static SInt64   Milliseconds();

        static SInt64   Microseconds();
       
        static inline UInt32    GetUInt32FromMemory(UInt32* inP);

        static SInt64   HostToNetworkSInt64(SInt64 hostOrdered);
        static SInt64   NetworkToHostSInt64(SInt64 networkOrdered);
                            
		static SInt64	TimeMilli_To_Fixed64Secs(SInt64 inMilliseconds); //new CISCO provided implementation
		
		static SInt64	TimeMilli_To_1900Fixed64Secs(SInt64 inMilliseconds)
						{ return TimeMilli_To_Fixed64Secs(sMsecSince1900) + TimeMilli_To_Fixed64Secs(inMilliseconds); }

		static SInt64	TimeMilli_To_UnixTimeMilli(SInt64 inMilliseconds)
						{ return inMilliseconds; }

		static time_t	TimeMilli_To_UnixTimeSecs(SInt64 inMilliseconds)
						{ return (time_t)  ( (SInt64) TimeMilli_To_UnixTimeMilli(inMilliseconds) / (SInt64) 1000); }
		
		static time_t 	UnixTime_Secs(void) // Seconds since 1970
						{ return TimeMilli_To_UnixTimeSecs(Milliseconds()); }

        static time_t   Time1900Fixed64Secs_To_UnixTimeSecs(SInt64 in1900Fixed64Secs)
                        { return (time_t)( (SInt64)  ((SInt64)  ( in1900Fixed64Secs - TimeMilli_To_Fixed64Secs(sMsecSince1900) ) /  ((SInt64) 1 << 32)  ) ); }
                            
        static SInt64   Time1900Fixed64Secs_To_TimeMilli(SInt64 in1900Fixed64Secs)
                        { return   ( (SInt64) ( (Float64) ((SInt64) in1900Fixed64Secs - (SInt64) TimeMilli_To_Fixed64Secs(sMsecSince1900) ) / (Float64)  ((SInt64) 1 << 32) ) * 1000) ; }
 
        static SInt32   GetGMTOffset();
                            
        static OS_Error RecursiveMakeDir(char *inPath);
 
        static OS_Error MakeDir(char *inPath);
        
        static UInt32   GetNumProcessors();
        
        // CPU Load
        static Float32  GetCurrentCPULoadPercent();
        
        // Mutex for StdLib calls
         static OSMutex* GetStdLibMutex()  { return &sStdLibOSMutex; }

        static SInt64   InitialMSec()       { return sInitialMsec; }
        static Float32  StartTimeMilli_Float() { return (Float32) ( (Float64) ( (SInt64) OS::Milliseconds() - (SInt64) OS::InitialMSec()) / (Float64) 1000.0 ); }
        static SInt64   StartTimeMilli_Int()      { return (OS::Milliseconds() - OS::InitialMSec()); }

		static Bool16 	ThreadSafe();

   private:
    
        static double sDivisor;
        static double sMicroDivisor;
        static SInt64 sMsecSince1900;
        static SInt64 sMsecSince1970;
        static SInt64 sInitialMsec;
        static SInt32 sMemoryErr;
        static void SetDivisor();
        static SInt64 sWrapTime;
        static SInt64 sCompareWrap;
        static SInt64 sLastTimeMilli;
        static OSMutex sStdLibOSMutex;
};

inline UInt32   OS::GetUInt32FromMemory(UInt32* inP)
{
#if ALLOW_NON_WORD_ALIGN_ACCESS
    return *inP;
#else
    char* tempPtr = (char*)inP;
    UInt32 temp = 0;
    ::memcpy(&temp, tempPtr, sizeof(UInt32));
    return temp;
#endif
}


#endif
