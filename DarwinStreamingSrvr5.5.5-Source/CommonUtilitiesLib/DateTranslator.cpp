#include "DateTranslator.h"

#include <time.h>

#include "OSHeaders.h"
#include "OS.h"

#include "StringParser.h"
#include "StrPtrLen.h"

const UInt32 kMonthHashTable[] =
{
    12, 12, 12, 12, 12, 12, 12, 12, 12, 11,     // 0 - 9
    1,  12, 12, 12, 12, 12, 12, 12, 12, 12,     // 10 - 19
    12, 12, 0,  12, 12, 12, 7,  12, 12, 2,      // 20 - 29
    12, 12, 3,  12, 12, 9,  4,  8,  12, 12,     // 30 - 39
    6,  12, 5,  12, 12, 12, 12, 12, 10, 12      // 40 - 49
};
const UInt32 kMonthHashTableSize = 49;


SInt64  DateTranslator::ParseDate(StrPtrLen* inDateString)
{
    // Parse the date buffer, filling out a tm struct
    struct tm theDateStruct;
    ::memset(&theDateStruct, 0, sizeof(theDateStruct));

    // All RFC 1123 dates are the same length.
    if (inDateString->Len != DateBuffer::kDateBufferLen)
        return 0;
    
    StringParser theDateParser(inDateString);
    
    theDateParser.ConsumeLength(NULL, 5);
    
    // We are at the date now.
    theDateStruct.tm_mday = theDateParser.ConsumeInteger(NULL);
    theDateParser.ConsumeWhitespace();
    
    // We are at the month now.
    if (theDateParser.GetDataRemaining() < 4)
        return 0;
    
    UInt32 theIndex =   ConvertCharToMonthTableIndex(theDateParser.GetCurrentPosition()[0]) +
                        ConvertCharToMonthTableIndex(theDateParser.GetCurrentPosition()[1]) +
                        ConvertCharToMonthTableIndex(theDateParser.GetCurrentPosition()[2]);
    
    if (theIndex > kMonthHashTableSize)
        return 0;
        
    theDateStruct.tm_mon = kMonthHashTable[theIndex];

    // If the month is illegal, return an error
    if (theDateStruct.tm_mon >= 12)
        return 0;
        
    // Skip over the date
    theDateParser.ConsumeLength(NULL, 4);   
    
    // Grab the year(the year after1900)
    theDateStruct.tm_year = theDateParser.ConsumeInteger(NULL) - 1900;
    theDateParser.ConsumeWhitespace();

    // Grag the hour, minute, second
    theDateStruct.tm_hour = theDateParser.ConsumeInteger(NULL);
    theDateStruct.tm_hour += OS::GetGMTOffset();
    
    theDateParser.ConsumeLength(NULL, 1); //skip over ':'   

    theDateStruct.tm_min = theDateParser.ConsumeInteger(NULL);
    theDateParser.ConsumeLength(NULL, 1); //skip over ':'   

    theDateStruct.tm_sec = theDateParser.ConsumeInteger(NULL);

    //convert the completed tm struct to a time_t
    time_t theTime = ::mktime(&theDateStruct);
    return (SInt64)theTime * 1000; // convert to a time value in our timebase.
}

void DateTranslator::UpdateDateBuffer(DateBuffer* inDateBuffer, const SInt64& inDate, time_t gmtoffset)
{
    if (inDateBuffer == NULL)
        return;

    struct tm* gmt = NULL;
    struct tm  timeResult;
    
    if (inDate == 0)
    {
        time_t calendarTime = ::time(NULL) + gmtoffset;
        gmt = ::qtss_gmtime(&calendarTime, &timeResult);
    }
    else
    {
        time_t convertedTime = (time_t)(inDate / (SInt64)1000) + gmtoffset ; // Convert from msec to sec
        gmt = ::qtss_gmtime(&convertedTime, &timeResult);
    }
        
    Assert(gmt != NULL); //is it safe to assert this?
    size_t size  = 0;
    if (0 == gmtoffset)
        size = qtss_strftime(   inDateBuffer->fDateBuffer, sizeof(inDateBuffer->fDateBuffer),
                            "%a, %d %b %Y %H:%M:%S GMT", gmt);
                            
    Assert(size == DateBuffer::kDateBufferLen);
}

void DateBuffer::InexactUpdate()
{
    SInt64 theCurTime = OS::Milliseconds();
    if ((fLastDateUpdate == 0) || ((fLastDateUpdate + kUpdateInterval) < theCurTime))
    {
        fLastDateUpdate = theCurTime;
        this->Update(0);
    }
}
