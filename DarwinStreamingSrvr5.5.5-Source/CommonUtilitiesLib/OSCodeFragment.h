#ifndef _OS_CODEFRAGMENT_H_
#define _OS_CODEFRAGMENT_H_

#include <stdlib.h>
#include "SafeStdLib.h"
#include "OSHeaders.h"

#ifdef __MacOSX__
#include <CoreFoundation/CFBundle.h>
#endif

class OSCodeFragment
{
    public:
    
        static void Initialize();
    
        OSCodeFragment(const char* inPath);
        ~OSCodeFragment();
        
        Bool16  IsValid() { return (fFragmentP != NULL); }
        void*   GetSymbol(const char* inSymbolName);
        
    private:
    
#ifdef __Win32__
        HMODULE fFragmentP;
#elif __MacOSX__
        CFBundleRef fFragmentP;
#else
        void*   fFragmentP;
#endif
};

#endif
