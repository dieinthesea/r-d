#ifndef OSHeaders_H
#define OSHeaders_H
#include <limits.h>

#define kSInt16_Max USHRT_MAX
#define kUInt16_Max USHRT_MAX

#define kSInt32_Max LONG_MAX
#define kUInt32_Max ULONG_MAX

#define kSInt64_Max LONG_LONG_MAX
#define kUInt64_Max ULONG_LONG_MAX


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif



//platform
#if __linux__ || __linuxppc__ || __FreeBSD__ || __MacOSX__
    
    #define _64BITARG_ "q"

    #define kEOLString "\n"
    #define kPathDelimiterString "/"
    #define kPathDelimiterChar '/'
    #define kPartialPathBeginsWithDelimiter 0

    #include <sys/types.h>
    
    #define QT_TIME_TO_LOCAL_TIME   (-2082844800)
    #define QT_PATH_SEPARATOR       '/'

    typedef unsigned int        PointerSizedInt;
    typedef unsigned char       UInt8;
    typedef signed char         SInt8;
    typedef unsigned short      UInt16;
    typedef signed short        SInt16;
    typedef unsigned long       UInt32;
    typedef signed long         SInt32;
    typedef signed long long    SInt64;
    typedef unsigned long long  UInt64;
    typedef float               Float32;
    typedef double              Float64;
    typedef UInt16              Bool16;
    typedef UInt8               Bool8;
    
    typedef unsigned long       FourCharCode;
    typedef FourCharCode        OSType;

    #ifdef  FOUR_CHARS_TO_INT
    #error Conflicting Macro "FOUR_CHARS_TO_INT"
    #endif

    #define FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

    #ifdef  TW0_CHARS_TO_INT
    #error Conflicting Macro "TW0_CHARS_TO_INT"
    #endif
        
    #define TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )







#elif __Win32__
    
    #define _64BITARG_ "I64"

    #define kEOLString "\r\n"
    #define kPathDelimiterString "\\"
    #define kPathDelimiterChar '\\'
    #define kPartialPathBeginsWithDelimiter 0
    
    #define crypt(buf, salt) ((char*)buf)
    
    #include <windows.h>
    #include <winsock2.h>
    #include <mswsock.h>
    #include <process.h>
    #include <ws2tcpip.h>
    #include <io.h>
    #include <direct.h>
    #include <errno.h>

    
    #define R_OK 0
    #define W_OK 1

    #define ENOTCONN 1002
    #define EADDRINUSE 1004
    #define EINPROGRESS 1007
    #define ENOBUFS 1008
    #define EADDRNOTAVAIL 1009

    struct iovec {
        u_long  iov_len; // this is not the POSIX definition, it is rather defined to be
        char FAR*   iov_base; // equivalent to a WSABUF for easy integration into Win32
    };

    #define QT_TIME_TO_LOCAL_TIME   (-2082844800)
    #define QT_PATH_SEPARATOR       '/'

    typedef unsigned int        PointerSizedInt;
    typedef unsigned char       UInt8;
    typedef signed char         SInt8;
    typedef unsigned short      UInt16;
    typedef signed short        SInt16;
    typedef unsigned long       UInt32;
    typedef signed long         SInt32;
    typedef LONGLONG            SInt64;
    typedef ULONGLONG           UInt64;
    typedef float               Float32;
    typedef double              Float64;
    typedef UInt16              Bool16;
    typedef UInt8               Bool8;
    
    typedef unsigned long       FourCharCode;
    typedef FourCharCode        OSType;

    #ifdef  FOUR_CHARS_TO_INT
    #error Conflicting Macro "FOUR_CHARS_TO_INT"
    #endif

    #define FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

    #ifdef  TW0_CHARS_TO_INT
    #error Conflicting Macro "TW0_CHARS_TO_INT"
    #endif
        
    #define TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )

    #define kSInt16_Max USHRT_MAX
    #define kUInt16_Max USHRT_MAX
    
    #define kSInt32_Max LONG_MAX
    #define kUInt32_Max ULONG_MAX
    
    #undef kSInt64_Max
    #define kSInt64_Max  9223372036854775807i64
    
    #undef kUInt64_Max
    #define kUInt64_Max  (kSInt64_Max * 2ULL + 1)

#elif __sgi__
    #define _64BITARG_ "ll"

    #define kPathDelimiterString "/"
    #define kPathDelimiterChar '/'
    #define kPartialPathBeginsWithDelimiter 0
	#define	kEOLString "\n"

    #include <sys/types.h>
    #include <netinet/in.h>
    #include <pthread.h>

    #define QT_TIME_TO_LOCAL_TIME   (-2082844800)
    #define QT_PATH_SEPARATOR       '/'

    typedef unsigned char       boolean;
    #define true                1
    #define false               0

    typedef unsigned int        PointerSizedInt;
    typedef unsigned char       UInt8;
    typedef signed char         SInt8;
    typedef unsigned short      UInt16;
    typedef signed short        SInt16;
    typedef unsigned long       UInt32;
    typedef signed long         SInt32;
    typedef signed long long    SInt64;
    typedef unsigned long long  UInt64;
    typedef float               Float32;
    typedef double              Float64;
	
	typedef UInt16				Bool16;

	typedef unsigned long		FourCharCode;
	typedef FourCharCode		OSType;

    #define thread_t    pthread_t
    #define cthread_errno() errno

    #ifdef  FOUR_CHARS_TO_INT
    #error Conflicting Macro "FOUR_CHARS_TO_INT"
    #endif

    #define FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

    #ifdef  TW0_CHARS_TO_INT
    #error Conflicting Macro "TW0_CHARS_TO_INT"
    #endif
        
    #define TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )

#elif defined(sun) // && defined(sparc)

    #define _64BITARG_ "ll"

    #define kPathDelimiterString "/"
    #define kPathDelimiterChar '/'
    #define kPartialPathBeginsWithDelimiter 0
    #define kEOLString "\n"

    #include <sys/types.h>
    #include <sys/byteorder.h>

    #define QT_TIME_TO_LOCAL_TIME   (-2082844800)
    #define QT_PATH_SEPARATOR       '/'

    typedef unsigned int        PointerSizedInt;
    typedef unsigned char       UInt8;
    typedef signed char         SInt8;
    typedef unsigned short      UInt16;
    typedef signed short        SInt16;
    typedef unsigned long       UInt32;
    typedef signed long         SInt32;
    typedef signed long long    SInt64;
    typedef unsigned long long  UInt64;
    typedef float               Float32;
    typedef double              Float64;
    typedef UInt16              Bool16;
    typedef UInt8               Bool8;
    
    typedef unsigned long       FourCharCode;
    typedef FourCharCode        OSType;

    #ifdef  FOUR_CHARS_TO_INT
    #error Conflicting Macro "FOUR_CHARS_TO_INT"
    #endif

    #define FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

    #ifdef  TW0_CHARS_TO_INT
    #error Conflicting Macro "TW0_CHARS_TO_INT"
    #endif
        
    #define TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )

#elif defined(__hpux__)

    #define _64BITARG_ "ll"

    #define kPathDelimiterString "/"
    #define kPathDelimiterChar '/'
    #define kPartialPathBeginsWithDelimiter 0
    #define kEOLString "\n"

    #include <sys/types.h>
    #include <sys/byteorder.h>

    #define QT_TIME_TO_LOCAL_TIME   (-2082844800)
    #define QT_PATH_SEPARATOR       '/'

    typedef unsigned int        PointerSizedInt;
    typedef unsigned char       UInt8;
    typedef signed char         SInt8;
    typedef unsigned short      UInt16;
    typedef signed short        SInt16;
    typedef unsigned long       UInt32;
    typedef signed long         SInt32;
    typedef signed long long    SInt64;
    typedef unsigned long long  UInt64;
    typedef float               Float32;
    typedef double              Float64;
    typedef UInt16              Bool16;
    typedef UInt8               Bool8;

    typedef unsigned long       FourCharCode;
    typedef FourCharCode        OSType;

    #ifdef  FOUR_CHARS_TO_INT
    #error Conflicting Macro "FOUR_CHARS_TO_INT"
    #endif

    #define FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

    #ifdef  TW0_CHARS_TO_INT
    #error Conflicting Macro "TW0_CHARS_TO_INT"
    #endif

    #define TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )

#elif defined(__osf__)
    
    #define _64BITARG_ "l"

    #define kEOLString "\n"
    #define kPathDelimiterString "/"
    #define kPathDelimiterChar '/'
    #define kPartialPathBeginsWithDelimiter 0

    #include <sys/types.h>
    #include <machine/endian.h>
    
    #define QT_TIME_TO_LOCAL_TIME   (-2082844800)
    #define QT_PATH_SEPARATOR       '/'

    typedef unsigned long       PointerSizedInt;
    typedef unsigned char       UInt8;
    typedef signed char         SInt8;
    typedef unsigned short      UInt16;
    typedef signed short        SInt16;
    typedef unsigned int        UInt32;
    typedef signed int          SInt32;
    typedef signed long         SInt64;
    typedef unsigned long       UInt64;
    typedef float               Float32;
    typedef double              Float64;
    typedef UInt16              Bool16;
    typedef UInt8               Bool8;
    
    typedef unsigned int        FourCharCode;
    typedef FourCharCode        OSType;

    #ifdef  FOUR_CHARS_TO_INT
    #error Conflicting Macro "FOUR_CHARS_TO_INT"
    #endif

    #define FOUR_CHARS_TO_INT( c1, c2, c3, c4 )  ( c1 << 24 | c2 << 16 | c3 << 8 | c4 )

    #ifdef  TW0_CHARS_TO_INT
    #error Conflicting Macro "TW0_CHARS_TO_INT"
    #endif
        
    #define TW0_CHARS_TO_INT( c1, c2 )  ( c1 << 8 | c2 )


#endif

typedef SInt32 OS_Error;

enum
{
    OS_NoErr = (OS_Error) 0,
    OS_BadURLFormat = (OS_Error) -100,
    OS_NotEnoughSpace = (OS_Error) -101
};


#endif
