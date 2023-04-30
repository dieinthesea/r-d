#include <stdlib.h>
#include "SafeStdLib.h"
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <math.h>

#ifndef __Win32__
#include <sys/time.h>
#endif

#ifdef __sgi__ 
#include <unistd.h>
#endif

#include "OS.h"
#include "OSThread.h"
#include "MyAssert.h"
#include "OSFileSource.h"

#if __MacOSX__

#ifndef __COREFOUNDATION__
#include <CoreFoundation/CoreFoundation.h>
#endif

#endif


#if (__FreeBSD__ ||  __MacOSX__)
    #include <sys/sysctl.h>
#endif

#if (__solaris__ || __linux__ || __linuxppc__)
    #include "StringParser.h"
#endif

#if __sgi__
	#include <sys/systeminfo.h>
#endif


double  OS::sDivisor = 0;
double  OS::sMicroDivisor = 0;
SInt64  OS::sMsecSince1970 = 0;
SInt64  OS::sMsecSince1900 = 0;
SInt64  OS::sInitialMsec = 0;
SInt64  OS::sWrapTime = 0;
SInt64  OS::sCompareWrap = 0;
SInt64  OS::sLastTimeMilli = 0;
OSMutex OS::sStdLibOSMutex;

#if DEBUG || __Win32__
#include "OSMutex.h"
#include "OSMemory.h"
static OSMutex* sLastMillisMutex = NULL;
#endif

void OS::Initialize()
{
    Assert (sInitialMsec == 0);  // do only once
    if (sInitialMsec != 0) return;

    SInt64 the1900Sec = (SInt64) (24 * 60 * 60) * (SInt64) ((70 * 365) + 17) ;
    sMsecSince1900 = the1900Sec * 1000;
    
    sWrapTime = (SInt64) 0x00000001 << 32;
    sCompareWrap = (SInt64) 0xffffffff << 32;
    sLastTimeMilli = 0;
    
    sInitialMsec = OS::Milliseconds(); 

    sMsecSince1970 = ::time(NULL); 
    sMsecSince1970 *= 1000;       


#if DEBUG || __Win32__ 
    sLastMillisMutex = NEW OSMutex();
#endif
}

SInt64 OS::Milliseconds()
{
/*
#if __MacOSX__

#if DEBUG
    OSMutexLocker locker(sLastMillisMutex);
#endif

   UnsignedWide theMicros;
    ::Microseconds(&theMicros);
    SInt64 scalarMicros = theMicros.hi;
    scalarMicros <<= 32;
    scalarMicros += theMicros.lo;
    scalarMicros = ((scalarMicros / 1000) - sInitialMsec) + sMsecSince1970;

#if DEBUG
    static SInt64 sLastMillis = 0;
    //Assert(scalarMicros >= sLastMillis); 
    sLastMillis = scalarMicros;
#endif
    return scalarMicros;
*/
#if __Win32__
    OSMutexLocker locker(sLastMillisMutex);
    
    SInt64 curTimeMilli = (UInt32) ::timeGetTime() + (sLastTimeMilli & sCompareWrap);
    if((curTimeMilli - sLastTimeMilli) < 0)
    {
        curTimeMilli += sWrapTime;
    }
    sLastTimeMilli = curTimeMilli;
    
    return (curTimeMilli - sInitialMsec) + sMsecSince1970; 
#else
    struct timeval t;
    struct timezone tz;
    int theErr = ::gettimeofday(&t, &tz);
    Assert(theErr == 0);

    SInt64 curTime;
    curTime = t.tv_sec;
    curTime *= 1000;                // sec -> msec
    curTime += t.tv_usec / 1000;    // usec -> msec

    return (curTime - sInitialMsec) + sMsecSince1970;
#endif

}

SInt64 OS::Microseconds()
{

#if __Win32__
    SInt64 curTime = (SInt64) ::timeGetTime(); 
    curTime -= sInitialMsec; 
    curTime *= 1000; // convert to microseconds                   
    return curTime;
#else
    struct timeval t;
    struct timezone tz;
    int theErr = ::gettimeofday(&t, &tz);
    Assert(theErr == 0);

    SInt64 curTime;
    curTime = t.tv_sec;
    curTime *= 1000000;     // sec -> usec
    curTime += t.tv_usec;

    return curTime - (sInitialMsec * 1000);
#endif
}

SInt32 OS::GetGMTOffset()
{
#ifdef __Win32__
    TIME_ZONE_INFORMATION tzInfo;
    DWORD theErr = ::GetTimeZoneInformation(&tzInfo);
    if (theErr == TIME_ZONE_ID_INVALID)
        return 0;
    
    return ((tzInfo.Bias / 60) * -1);
#else
    struct timeval  tv;
    struct timezone tz;

    int err = ::gettimeofday(&tv, &tz);
    if (err != 0)
        return 0;
        
    return ((tz.tz_minuteswest / 60) * -1);
#endif
}


SInt64  OS::HostToNetworkSInt64(SInt64 hostOrdered)
{
#if BIGENDIAN
    return hostOrdered;
#else
    return (SInt64) (  (UInt64)  (hostOrdered << 56) | (UInt64)  (((UInt64) 0x00ff0000 << 32) & (hostOrdered << 40))
        | (UInt64)  ( ((UInt64)  0x0000ff00 << 32) & (hostOrdered << 24)) | (UInt64)  (((UInt64)  0x000000ff << 32) & (hostOrdered << 8))
        | (UInt64)  ( ((UInt64)  0x00ff0000 << 8) & (hostOrdered >> 8)) | (UInt64)     ((UInt64)  0x00ff0000 & (hostOrdered >> 24))
        | (UInt64)  (  (UInt64)  0x0000ff00 & (hostOrdered >> 40)) | (UInt64)  ((UInt64)  0x00ff & (hostOrdered >> 56)) );
#endif
}

SInt64  OS::NetworkToHostSInt64(SInt64 networkOrdered)
{
#if BIGENDIAN
    return networkOrdered;
#else
    return (SInt64) (  (UInt64)  (networkOrdered << 56) | (UInt64)  (((UInt64) 0x00ff0000 << 32) & (networkOrdered << 40))
        | (UInt64)  ( ((UInt64)  0x0000ff00 << 32) & (networkOrdered << 24)) | (UInt64)  (((UInt64)  0x000000ff << 32) & (networkOrdered << 8))
        | (UInt64)  ( ((UInt64)  0x00ff0000 << 8) & (networkOrdered >> 8)) | (UInt64)     ((UInt64)  0x00ff0000 & (networkOrdered >> 24))
        | (UInt64)  (  (UInt64)  0x0000ff00 & (networkOrdered >> 40)) | (UInt64)  ((UInt64)  0x00ff & (networkOrdered >> 56)) );
#endif
}


OS_Error OS::MakeDir(char *inPath)
{
    struct stat theStatBuffer;
    if (::stat(inPath, &theStatBuffer) == -1)
    {
        //create a directory
#ifdef __Win32__
        if (::mkdir(inPath) == -1)
#else
        if (::mkdir(inPath, S_IRWXU) == -1)
#endif
            return (OS_Error)OSThread::GetErrno();
    }
#ifdef __Win32__
    else if (!(theStatBuffer.st_mode & _S_IFDIR))
        return EEXIST; // there is a file at this point in the path
#else
    else if (!S_ISDIR(theStatBuffer.st_mode))
        return EEXIST;//there is a file at this point in the path
#endif

    //directory exists
    return OS_NoErr;
}

OS_Error OS::RecursiveMakeDir(char *inPath)
{
    Assert(inPath != NULL);
    
   
    char *thePathTraverser = inPath;
    
   
    if (*thePathTraverser == kPathDelimiterChar)
        thePathTraverser++;
        
    while (*thePathTraverser != '\0')
    {
        if (*thePathTraverser == kPathDelimiterChar)
        {
           
            *thePathTraverser = '\0';
            OS_Error theErr = MakeDir(inPath);
           
            *thePathTraverser = kPathDelimiterChar;

            if (theErr != OS_NoErr)
                return theErr;
        }
        thePathTraverser++;
    }
    
   
    return MakeDir(inPath);
}

Bool16 OS::ThreadSafe()
{

#if (__MacOSX__) 
	char releaseStr[32] = "";
  	size_t strLen = sizeof(releaseStr);
	int mib[2];
    mib[0] = CTL_KERN;
    mib[1] = KERN_OSRELEASE;

	UInt32 majorVers = 0;
    int err =  sysctl(mib,2, releaseStr, &strLen, NULL,0);
    if (err == 0)
    {
		StrPtrLen rStr(releaseStr,strLen);
		char* endMajor = rStr.FindString(".");
		if (endMajor != NULL) 
			*endMajor = 0;
			
		if (::strlen(releaseStr) > 0)
			::sscanf(releaseStr, "%lu", &majorVers);
	}
	if (majorVers < 7) 
		return false; 
	
#endif

	return true;

}


UInt32  OS::GetNumProcessors()
{
#if (__Win32__)
    SYSTEM_INFO theSystemInfo;
    ::GetSystemInfo(&theSystemInfo);
    
    return (UInt32)theSystemInfo.dwNumberOfProcessors;
#endif

#if (__MacOSX__ || __FreeBSD__)
    int numCPUs = 1;
    size_t len = sizeof(numCPUs);
	int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    (void) ::sysctl(mib,2,&numCPUs,&len,NULL,0);
    if (numCPUs < 1) 
        numCPUs = 1;
    return (UInt32) numCPUs;
#endif

#if(__linux__ || __linuxppc__)
    
    char cpuBuffer[8192] = "";
    StrPtrLen cpuInfoBuf(cpuBuffer, sizeof(cpuBuffer));
    FILE    *cpuFile = ::fopen( "/proc/cpuinfo", "r" );
    if (cpuFile)
    {   cpuInfoBuf.Len = ::fread(cpuInfoBuf.Ptr, sizeof(char),  cpuInfoBuf.Len, cpuFile);
        ::fclose(cpuFile);
    }
    
    StringParser cpuInfoFileParser(&cpuInfoBuf);
    StrPtrLen line;
    StrPtrLen word;
    UInt32 numCPUs = 0;
    
    while( cpuInfoFileParser.GetDataRemaining() != 0 ) 
    {
        cpuInfoFileParser.GetThruEOL(&line);    // Read each line   
        StringParser lineParser(&line);
        lineParser.ConsumeWhitespace();        

        if (lineParser.GetDataRemaining() == 0) // must be an empty line
            continue;

        lineParser.ConsumeUntilWhitespace(&word);
               
        if ( word.Equal("processor") ) 
        {   numCPUs ++; 
        }
    }
    
    if (numCPUs == 0)
        numCPUs = 1;
        
    return numCPUs;
#endif

#if(__solaris__)
{
    UInt32 numCPUs = 0;
    char linebuff[512] = "";
    StrPtrLen line(linebuff, sizeof(linebuff));
    StrPtrLen word;

    FILE *p = ::popen("uname -X","r");
    while((::fgets(linebuff, sizeof(linebuff -1), p)) > 0)
    {
        StringParser lineParser(&line);
        lineParser.ConsumeWhitespace(); 

        if (lineParser.GetDataRemaining() == 0) // must be an empty line
            continue;

        lineParser.ConsumeUntilWhitespace(&word);

        if ( word.Equal("NumCPU")) 
        {
            lineParser.GetThru(NULL,'=');
            lineParser.ConsumeWhitespace();  
            lineParser.ConsumeUntilWhitespace(&word);
            if (word.Len > 0)
                ::sscanf(word.Ptr, "%lu", &numCPUs);

            break;
        }
    }
    if (numCPUs == 0)
        numCPUs = 1;
        
    ::pclose(p);
    
	return numCPUs;
}
#endif

#if(__sgi__) 
    UInt32 numCPUs = 0;

    numCPUs = sysconf(_SC_NPROC_ONLN);
	
	return numCPUs;
#endif		


    return 1;
}


//CISCO provided fix for integer + fractional fixed64.
SInt64 OS::TimeMilli_To_Fixed64Secs(SInt64 inMilliseconds)
{
       SInt64 result = inMilliseconds / 1000; 
       result <<= 32;  // shift it to higher 32 bits
       // Take the remainder (rem = inMilliseconds%1000) and multiply by
       // 2**32, divide by 1000, effectively this gives (rem/1000) as a
       // binary fraction.
       double p = ldexp((double)(inMilliseconds%1000), +32) / 1000.;
       UInt32 frac = (UInt32)p;
       result |= frac;
       return result;
}
