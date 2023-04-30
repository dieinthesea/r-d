#include "GetWord.h"

char* GetWord( char* toWordPtr, char* fromStrPtr, long limit )
{

    while ( (unsigned char)*fromStrPtr <= 0x20 && *fromStrPtr )
        fromStrPtr++;
    
    while ( limit && (unsigned char)*fromStrPtr > 0x20 && *fromStrPtr )
    {
        *toWordPtr++ = *fromStrPtr++;
        limit--;
    }

    *toWordPtr = 0x00;
    
    return (char *) fromStrPtr;
}

char * GetQuotedWord( char* toWordPtr, char* fromStrPtr, long limit )
{
    // get a quote encoded word from a string
    int lastWasQuote = 0;
    
    while ( ( (unsigned char)*fromStrPtr <= 0x20 ) && *fromStrPtr )
        fromStrPtr++;
    
    
    if (  (unsigned char)*fromStrPtr == '"' )
    {   // must lead with quote sign after white space
        fromStrPtr++;
    
    
    
        // copy until we find the last single quote
        while ( limit && *fromStrPtr )
        {
            if ( (unsigned char)*fromStrPtr == '"' )
            {
                if ( lastWasQuote )
                {
                    *toWordPtr++ = '"';
                    lastWasQuote = 0;
                    limit--;
                }
                else
                    lastWasQuote = 1;
            }
            else
            {
                if ( !lastWasQuote )
                {   *toWordPtr++ = *fromStrPtr;
                    limit--;
                }
                else // we're done, hit a quote by itself
                    break;

            }
            
            // consume the char we read
            fromStrPtr++;
            
        }
    }
    
    *toWordPtr = 0x00;
    
    return (char *) fromStrPtr;
}
