#include <stdlib.h>
#include "SafeStdLib.h"
#include <stdio.h>
#include "MyAssert.h"

#if __Win32__
    
#elif __MacOSX__
    #include <CoreFoundation/CFString.h>
    #include <CoreFoundation/CFBundle.h>
#else
#include <dlfcn.h>
#endif

#include "OSCodeFragment.h"

void OSCodeFragment::Initialize()
{

}

OSCodeFragment::OSCodeFragment(const char* inPath)
: fFragmentP(NULL)
{
#if defined(HPUX) || defined(HPUX10)
    shl_t handle;
    fFragmentP = shl_load(inPath, BIND_IMMEDIATE|BIND_VERBOSE|BIND_NOSTART, 0L);
#elif defined(OSF1) ||\
    (defined(__FreeBSD_version) && (__FreeBSD_version >= 220000))
    fFragmentP = dlopen((char *)inPath, RTLD_NOW | RTLD_GLOBAL);
#elif defined(__FreeBSD__)
    fFragmentP = dlopen(inPath, RTLD_NOW);
#elif defined(__sgi__) 
    fFragmentP = dlopen(inPath, RTLD_NOW); // not sure this should be either RTLD_NOW or RTLD_LAZY
#elif defined(__Win32__)
    fFragmentP = ::LoadLibrary(inPath);
#elif defined(__MacOSX__)
    CFStringRef theString = CFStringCreateWithCString( kCFAllocatorDefault, inPath, kCFStringEncodingASCII);

        CFURLRef    bundleURL = CFURLCreateWithFileSystemPath(  kCFAllocatorDefault,
                                                            theString,
                                                            kCFURLPOSIXPathStyle,
                                                            true);

    fFragmentP = CFBundleCreate( kCFAllocatorDefault, bundleURL );
    Boolean success = false;
    if (fFragmentP != NULL)
        success = CFBundleLoadExecutable( fFragmentP );
    if (!success && fFragmentP != NULL)
    {
        CFRelease( fFragmentP );
        fFragmentP = NULL;
    }
    
    CFRelease(bundleURL);
    CFRelease(theString);
    
#else
    fFragmentP = dlopen(inPath, RTLD_NOW | RTLD_GLOBAL);

#endif
}

OSCodeFragment::~OSCodeFragment()
{
    if (fFragmentP == NULL)
        return;
        
#if defined(HPUX) || defined(HPUX10)
    shl_unload((shl_t)fFragmentP);
#elif defined(__Win32__)
    BOOL theErr = ::FreeLibrary(fFragmentP);
    Assert(theErr);
#elif defined(__MacOSX__)
    CFBundleUnloadExecutable( fFragmentP );
    CFRelease( fFragmentP );
#else
    dlclose(fFragmentP);
#endif
}

void*   OSCodeFragment::GetSymbol(const char* inSymbolName)
{
    if (fFragmentP == NULL)
        return NULL;
        
#if defined(HPUX) || defined(HPUX10)
    void *symaddr = NULL;
    int status;

    errno = 0;
    status = shl_findsym((shl_t *)&fFragmentP, symname, TYPE_PROCEDURE, &symaddr);
    if (status == -1 && errno == 0) /* try TYPE_DATA instead */
        status = shl_findsym((shl_t *)&fFragmentP, inSymbolName, TYPE_DATA, &symaddr);
    return (status == -1 ? NULL : symaddr);
#elif defined(DLSYM_NEEDS_UNDERSCORE)
    char *symbol = (char*)malloc(sizeof(char)*(strlen(inSymbolName)+2));
    void *retval;
    qtss_sprintf(symbol, "_%s", inSymbolName);
    retval = dlsym(fFragmentP, symbol);
    free(symbol);
    return retval;
#elif defined(__Win32__)
    return ::GetProcAddress(fFragmentP, inSymbolName);
#elif defined(__MacOSX__)
    CFStringRef theString = CFStringCreateWithCString( kCFAllocatorDefault, inSymbolName, kCFStringEncodingASCII);
    void* theSymbol = (void*)CFBundleGetFunctionPointerForName( fFragmentP, theString );
    CFRelease(theString);
    return theSymbol;
#else
    return dlsym(fFragmentP, inSymbolName);
#endif  
}
