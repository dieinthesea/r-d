#include <stdio.h>
#include <stdlib.h>
#include "SafeStdLib.h"
#include <string.h>
#include <errno.h>

#ifndef __Win32__
    #if __PTHREADS__
        #include <pthread.h>
        #if USE_THR_YIELD
            #include <thread.h>
        #endif
    #else
        #include <mach/mach.h>
        #include <mach/cthreads.h>
    #endif
    #include <unistd.h>
    #include <grp.h>
    #include <pwd.h>
#endif

#include "OSThread.h"
#include "MyAssert.h"

#ifdef __sgi__ 
#include <time.h>
#endif


void*   OSThread::sMainThreadData = NULL;

#ifdef __Win32__
DWORD   OSThread::sThreadStorageIndex = 0;
#elif __PTHREADS__
pthread_key_t OSThread::gMainKey = 0;
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
pthread_attr_t OSThread::sThreadAttr;
#endif
#endif

char  OSThread::sUser[128]= "";
char  OSThread::sGroup[128]= "";

#if __linux__ ||  __MacOSX__
Bool16  OSThread::sWrapSleep = true;
#endif

void OSThread::Initialize()
{


#ifdef __Win32__
    sThreadStorageIndex = ::TlsAlloc();
    Assert(sThreadStorageIndex >= 0);
#elif __PTHREADS__
    pthread_key_create(&OSThread::gMainKey, NULL);
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING

    pthread_attr_init(&sThreadAttr);
 
    pthread_attr_setscope(&sThreadAttr, PTHREAD_SCOPE_SYSTEM);
#endif

#endif
}

OSThread::OSThread()
:   fStopRequested(false),
    fJoined(false),
    fThreadData(NULL)
{
}

OSThread::~OSThread()
{
    this->StopAndWaitForThread();
}



void OSThread::Start()
{
#ifdef __Win32__
    unsigned int theId = 0; 
    fThreadID = (HANDLE)_beginthreadex( NULL,   // Inherit security
                                        0,      // Inherit stack size
                                        _Entry, // Entry function
                                        (void*)this,    // Entry arg
                                        0,      // Begin executing immediately
                                        &theId );
    Assert(fThreadID != NULL);
#elif __PTHREADS__
    pthread_attr_t* theAttrP;
#ifdef _POSIX_THREAD_PRIORITY_SCHEDULING
    theAttrP = 0;
#else
    theAttrP = NULL;
#endif
    int err = pthread_create((pthread_t*)&fThreadID, theAttrP, _Entry, (void*)this);
    Assert(err == 0);
#else
    fThreadID = (UInt32)cthread_fork((cthread_fn_t)_Entry, (any_t)this);
#endif
}

void OSThread::StopAndWaitForThread()
{
    fStopRequested = true;
    if (!fJoined)
        Join();
}

void OSThread::Join()
{
    //waiting for the thread that we want to delete to complete running(waiting for it stop)
    Assert(!fJoined);
    fJoined = true;
#ifdef __Win32__
    DWORD theErr = ::WaitForSingleObject(fThreadID, INFINITE);
    Assert(theErr == WAIT_OBJECT_0);
#elif __PTHREADS__
    void *retVal;
    pthread_join((pthread_t)fThreadID, &retVal);
#else
    cthread_join((cthread_t)fThreadID);
#endif
}

void OSThread::ThreadYield()
{

#if THREADING_IS_COOPERATIVE
    #if __PTHREADS__
        #if USE_THR_YIELD
            thr_yield();
        #else
            sched_yield();
        #endif
    #endif
#endif
}

#include "OS.h"
void OSThread::Sleep(UInt32 inMsec)
{

#ifdef __Win32__
    ::Sleep(inMsec);
#elif __linux__ ||  __MacOSX__

    if (inMsec == 0)
        return;
        
    SInt64 startTime = OS::Milliseconds();
    SInt64 timeLeft = inMsec;
    SInt64 timeSlept = 0;

    do {
        timeLeft = inMsec - timeSlept;
        if (timeLeft < 1)
            break;
            
        ::usleep(timeLeft * 1000);

        timeSlept = (OS::Milliseconds() - startTime);
        if (timeSlept < 0)
            break;
            
    } while (timeSlept < inMsec);


#elif defined(__osf__) || defined(__hpux__)
    if (inMsec < 1000)
        ::usleep(inMsec * 1000); 
    else
        ::sleep((inMsec + 500) / 1000); 
#elif defined(__sgi__) 
	struct timespec ts;
	
	ts.tv_sec = 0;
	ts.tv_nsec = inMsec * 1000000;

	nanosleep(&ts, 0);
#else
    ::usleep(inMsec * 1000);
#endif
}

#ifdef __Win32__
unsigned int WINAPI OSThread::_Entry(LPVOID inThread)
#else
void* OSThread::_Entry(void *inThread)  //static
#endif
{
    OSThread* theThread = (OSThread*)inThread;
#ifdef __Win32__
    BOOL theErr = ::TlsSetValue(sThreadStorageIndex, theThread);
    Assert(theErr == TRUE);
#elif __PTHREADS__
    theThread->fThreadID = (pthread_t)pthread_self();
    pthread_setspecific(OSThread::gMainKey, theThread);
#else
    theThread->fThreadID = (UInt32)cthread_self();
    cthread_set_data(cthread_self(), (any_t)theThread);
#endif
    theThread->SwitchPersonality();
    //
    // Run the thread
    theThread->Entry();
    return NULL;
}


Bool16  OSThread::SwitchPersonality()
{
#if __linux__
   if (::strlen(sGroup) > 0)
    {
        struct group* gr = ::getgrnam(sGroup);
        if (gr == NULL || ::setgid(gr->gr_gid) == -1)
        {
            return false;
        }
            }
    
        
    if (::strlen(sUser) > 0)
    {
        struct passwd* pw = ::getpwnam(sUser);
        if (pw == NULL || ::setuid(pw->pw_uid) == -1)
        {
		return false;
        }

   }
#endif

   return true;
}


OSThread*   OSThread::GetCurrent()
{
#ifdef __Win32__
    return (OSThread *)::TlsGetValue(sThreadStorageIndex);
#elif __PTHREADS__
    return (OSThread *)pthread_getspecific(OSThread::gMainKey);
#else
    return (OSThread*)cthread_data(cthread_self());
#endif
}

#ifdef __Win32__
int OSThread::GetErrno()
{
    int winErr = ::GetLastError();
    switch (winErr)
    {

        case ERROR_FILE_NOT_FOUND: return ENOENT;

        case ERROR_PATH_NOT_FOUND: return ENOENT;       




        case WSAEINTR:      return EINTR;
        case WSAENETRESET:  return EPIPE;
        case WSAENOTCONN:   return ENOTCONN;
        case WSAEWOULDBLOCK:return EAGAIN;
        case WSAECONNRESET: return EPIPE;
        case WSAEADDRINUSE: return EADDRINUSE;
        case WSAEMFILE:     return EMFILE;
        case WSAEINPROGRESS:return EINPROGRESS;
        case WSAEADDRNOTAVAIL: return EADDRNOTAVAIL;
        case WSAECONNABORTED: return EPIPE;
        case 0:             return 0;
        
        default:            return ENOTCONN;
    }
}
#endif
