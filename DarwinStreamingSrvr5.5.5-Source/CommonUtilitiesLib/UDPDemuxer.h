#ifndef __UDPDEMUXER_H__
#define __UDPDEMUXER_H__

#include "OSHashTable.h"
#include "OSMutex.h"
#include "StrPtrLen.h"

class Task;
class UDPDemuxerKey;

class UDPDemuxerUtils
{
    private:
    
        static UInt32 ComputeHashValue(UInt32 inRemoteAddr, UInt16 inRemotePort)
            { return ((inRemoteAddr << 16) + inRemotePort); }
            
    friend class UDPDemuxerTask;
    friend class UDPDemuxerKey;
};

class UDPDemuxerTask
{
    public:
    
        UDPDemuxerTask()
            :   fRemoteAddr(0), fRemotePort(0),
                fHashValue(0), fNextHashEntry(NULL) {}
        virtual ~UDPDemuxerTask() {}
        
        UInt32  GetRemoteAddr() { return fRemoteAddr; }
        
    private:

        void Set(UInt32 inRemoteAddr, UInt16 inRemotePort)
            {   fRemoteAddr = inRemoteAddr; fRemotePort = inRemotePort;
                fHashValue = UDPDemuxerUtils::ComputeHashValue(fRemoteAddr, fRemotePort);
            }
        
        //key values
        UInt32 fRemoteAddr;
        UInt16 fRemotePort;
        
        //precomputed for performance
        UInt32 fHashValue;
        
        UDPDemuxerTask  *fNextHashEntry;

        friend class UDPDemuxerKey;
        friend class UDPDemuxer;
        friend class OSHashTable<UDPDemuxerTask,UDPDemuxerKey>;
};



class UDPDemuxerKey
{
    private:

        UDPDemuxerKey(UInt32 inRemoteAddr, UInt16 inRemotePort)
            :   fRemoteAddr(inRemoteAddr), fRemotePort(inRemotePort)
                { fHashValue = UDPDemuxerUtils::ComputeHashValue(inRemoteAddr, inRemotePort); }
                
        ~UDPDemuxerKey() {}
        
        
    private:
  
        UInt32      GetHashKey()        { return fHashValue; }

        //these functions are only used by the hash table itself. This constructor will break the "Set" functions.
        UDPDemuxerKey(UDPDemuxerTask *elem) :   fRemoteAddr(elem->fRemoteAddr),
                                                fRemotePort(elem->fRemotePort), 
                                                fHashValue(elem->fHashValue) {}
                                            
        friend int operator ==(const UDPDemuxerKey &key1, const UDPDemuxerKey &key2) {
            if ((key1.fRemoteAddr == key2.fRemoteAddr) &&
                (key1.fRemotePort == key2.fRemotePort))
                return true;
            return false;
        }
        
        //data:
        UInt32 fRemoteAddr;
        UInt16 fRemotePort;
        UInt32  fHashValue;

        friend class OSHashTable<UDPDemuxerTask,UDPDemuxerKey>;
        friend class UDPDemuxer;
};

typedef OSHashTable<UDPDemuxerTask, UDPDemuxerKey> UDPDemuxerHashTable;

class UDPDemuxer
{
    public:

        UDPDemuxer() : fHashTable(kMaxHashTableSize), fMutex() {}
        ~UDPDemuxer() {}

        //These functions grab the mutex and are therefore premptive safe
        OS_Error RegisterTask(UInt32 inRemoteAddr, UInt16 inRemotePort,
                                        UDPDemuxerTask *inTaskP);

        OS_Error UnregisterTask(UInt32 inRemoteAddr, UInt16 inRemotePort,
                                        UDPDemuxerTask *inTaskP);
        
        UDPDemuxerTask* GetTask(UInt32 inRemoteAddr, UInt16 inRemotePort);

        Bool16  AddrInMap(UInt32 inRemoteAddr, UInt16 inRemotePort)
                    { return (this->GetTask(inRemoteAddr, inRemotePort) != NULL); }
                    
        OSMutex*                GetMutex()      { return &fMutex; }
        UDPDemuxerHashTable*    GetHashTable()  { return &fHashTable; }
        
    private:
    
        enum
        {
            kMaxHashTableSize = 2747
        };
        UDPDemuxerHashTable fHashTable;
        OSMutex             fMutex;
};

#endif 


