#ifndef __getword__
#define __getword__

    #ifdef __cplusplus
    extern "C" {
    #endif


    char* GetWord( char* toWordPtr, char* fromStrPtr, long limit );
    char* GetQuotedWord( char* toWordPtr, char* fromStrPtr, long limit );

    #ifdef __cplusplus
    }   
    #endif


#endif
