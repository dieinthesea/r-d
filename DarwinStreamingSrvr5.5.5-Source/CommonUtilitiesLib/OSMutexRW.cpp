#include "OSMutexRW.h"
#include "OSMutex.h"
#include "OSCond.h"

#include <stdlib.h>
#include "SafeStdLib.h"
#include <string.h>

#if DEBUGMUTEXRW
    int OSMutexRW::fCount = 0;
    int OSMutexRW::fMaxCount =0;
#endif
    

#if DEBUGMUTEXRW
void OSMutexRW::CountConflict(int i)            
{
    fCount += i;
    if (i == -1) qtss_printf("Num Conflicts: %d\n", fMaxCount);
    if (fCount > fMaxCount)
    fMaxCount = fCount;

}
#endif

void OSMutexRW::LockRead()
{
    OSMutexLocker locker(&fInternalLock);
#if DEBUGMUTEXRW
    if (fState != 0) 
    {   qtss_printf("LockRead(conflict) fState = %d active readers = %d, waiting writers = %d, waiting readers=%d\n",fState,  fActiveReaders, fWriteWaiters, fReadWaiters);
        CountConflict(1);  
    }
 
#endif
    
    AddReadWaiter();
    while   (   ActiveWriter() // active writer so wait
            ||  WaitingWriters() // reader must wait for write waiters
            )
    {   
        fReadersCond.Wait(&fInternalLock,OSMutexRW::eMaxWait);
    }
        
    RemoveReadWaiter();
    AddActiveReader(); // add 1 to active readers
    fActiveReaders = fState;
    
#if DEBUGMUTEXRW
//  qtss_printf("LockRead(conflict) fState = %d active readers = %d, waiting writers = %d, waiting readers=%d\n",fState,  fActiveReaders, fWriteWaiters, fReadWaiters);

#endif
}

void OSMutexRW::LockWrite()
{
    OSMutexLocker locker(&fInternalLock);
    AddWriteWaiter();       //  1 writer queued            
#if DEBUGMUTEXRW

    if (Active()) 
    {   qtss_printf("LockWrite(conflict) state = %d active readers = %d, waiting writers = %d, waiting readers=%d\n", fState, fActiveReaders, fWriteWaiters, fReadWaiters);
        CountConflict(1);  
    }

    qtss_printf("LockWrite 'waiting' fState = %d locked active readers = %d, waiting writers = %d, waiting readers=%d\n",fState, fActiveReaders, fReadWaiters, fWriteWaiters);
#endif

    while   (ActiveReaders())  // active readers
    {       
        fWritersCond.Wait(&fInternalLock,OSMutexRW::eMaxWait);
    }

    RemoveWriteWaiter(); // remove from waiting writers
    SetState(OSMutexRW::eActiveWriterState);    // this is the active writer    
    fActiveReaders = fState; 
#if DEBUGMUTEXRW
//  qtss_printf("LockWrite 'locked' fState = %d locked active readers = %d, waiting writers = %d, waiting readers=%d\n",fState, fActiveReaders, fReadWaiters, fWriteWaiters);
#endif

}


void OSMutexRW::Unlock()
{           
    OSMutexLocker locker(&fInternalLock);
#if DEBUGMUTEXRW
//  qtss_printf("Unlock active readers = %d, waiting writers = %d, waiting readers=%d\n", fActiveReaders, fReadWaiters, fWriteWaiters);

#endif

    if (ActiveWriter()) 
    {           
        SetState(OSMutexRW::eNoWriterState); //active writer 
        if (WaitingWriters()) //waiting writers
        {   fWritersCond.Signal();
        }
        else
        {   fReadersCond.Broadcast();
        }
#if DEBUGMUTEXRW
        qtss_printf("Unlock(writer) active readers = %d, waiting writers = %d, waiting readers=%d\n", fActiveReaders, fReadWaiters, fWriteWaiters);
#endif
    }
    else
    {
        RemoveActiveReader(); //a reader
        if (!ActiveReaders()) // if there's no active readers
        {   SetState(OSMutexRW::eNoWriterState); //the active writer now no actives threads
            fWritersCond.Signal();
        } 
    }
    fActiveReaders = fState;

}




int OSMutexRW::TryLockWrite()
{
    int    status  = EBUSY;
    OSMutexLocker locker(&fInternalLock);

    if ( !Active() && !WaitingWriters()) // no writers, no readers, no waiting writers
    {
        this->LockWrite();
        status = 0;
    }

    return status;
}

int OSMutexRW::TryLockRead()
{
    int    status  = EBUSY;
    OSMutexLocker locker(&fInternalLock);

    if ( !ActiveWriter() && !WaitingWriters() ) //no active and waiting writers, but have readers
    {
        this->LockRead(); 
        status = 0;
    }
    
    return status;
}



