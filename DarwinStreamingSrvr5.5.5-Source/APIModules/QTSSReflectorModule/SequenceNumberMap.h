/*
 The purpose of this class is to maintain a sliding window 
 that stores the sequence number of received data packets, 
 in order to determine if any data packets have been lost.
*/
/*
    File:       SequenceNumberMap.h

    Contains:   Data structure for keeping track of duplicate sequence numbers.
                Useful for removing duplicate packets from an RTP stream.
                    
*/

#ifndef _SEQUENCE_NUMBER_MAP_H_
#define _SEQUENCE_NUMBER_MAP_H_

#include "OSHeaders.h"

#define SEQUENCENUMBERMAPTESTING 1

//used to initialize a sliding window and receive an integer parameter inSlidingWindowSize, 
//representing the size of the sliding window. The default size is 256.
class SequenceNumberMap
{
    public:
    
        enum
        {
            kDefaultSlidingWindowSize = 256
        };
        //used to destroy sliding windows.
        SequenceNumberMap(UInt32 inSlidingWindowSize = kDefaultSlidingWindowSize);
        ~SequenceNumberMap() { delete [] fSlidingWindow; }
        
        // Used to add a new sequence number to the sliding window, returning a Boolean value indicating 
        //whether the sequence number has already been added.
        Bool16  AddSequenceNumber(UInt16 inSeqNumber);
        
#if SEQUENCENUMBERMAPTESTING
        static void Test();
#endif
        
    private:
        
        Bool16*         fSlidingWindow;

        const SInt32    fWindowSize;
        const SInt32    fNegativeWindowSize;

        UInt16          fHighestSeqIndex;
        UInt16          fHighestSeqNumber;
};


#endif
