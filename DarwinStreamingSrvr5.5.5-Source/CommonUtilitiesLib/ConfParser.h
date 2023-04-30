#ifndef __ConfParser__
#define __ConfParser__

#include "OSHeaders.h"

// the maximum size + 1 of a parameter 
#define kConfParserMaxParamSize 512


// the maximum size + 1 of single line in the file 
#define kConfParserMaxLineSize 1024

// the maximum number of values per config parameter
#define kConfParserMaxParamValues 10


void TestParseConfigFile();

int ParseConfigFile( Bool16 allowNullValues, const char* fname  \
                    , Bool16 (*ConfigSetter)( const char* paramName, const char* paramValue[], void * userData )    \
                    , void* userData );
#endif
