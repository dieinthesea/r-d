#ifndef __QTSS_ROLLINGLOG_H__
#define __QTSS_ROLLINGLOG_H__

#include <stdio.h>
#include <time.h>
#ifndef __Win32__
#include <sys/time.h>
#endif
#include "OSHeaders.h"
#include "OSMutex.h"
#include "Task.h"

const Bool16 kAllowLogToRoll = true;

class QTSSRollingLog : public Task
{
    public:

        QTSSRollingLog();

        void    Delete()
            { CloseLog(false); this->Signal(Task::kKillEvent); }

        void    WriteToLog(char* inLogData, Bool16 allowLogToRoll);

        Bool16  RollLog();
   
        void EnableLog( Bool16 appendDotLog = true);

        void CloseLog( Bool16 leaveEnabled = false);

        //mainly to check and see if errors occurred
        Bool16  IsLogEnabled();
        
        //master switch
        Bool16  IsLogging() { return fLogging; }
        void  SetLoggingEnabled( Bool16 logState ) { fLogging = logState; }
        
        //General purpose utility function returns false if some error has occurred
        static Bool16   FormatDate(char *ioDateBuffer, Bool16 logTimeInGMT);

        Bool16          CheckRollLog();
        
        // Set this to true to get the log to close the file between writes.
        static void		SetCloseOnWrite(Bool16 closeOnWrite);

        enum
        {
            kMaxDateBufferSizeInBytes = 30, //UInt32
            kMaxFilenameLengthInBytes = 31  //UInt32
        };
    
    protected:

        // Task object. Do not delete directly
        virtual ~QTSSRollingLog();

        //Derived class must provide a way to get the log & rolled log name
        virtual char* GetLogName() = 0;
        virtual char* GetLogDir() = 0;
        virtual UInt32 GetRollIntervalInDays() = 0;//0 means no interval
        virtual UInt32 GetMaxLogBytes() = 0;//0 means unlimited
                    
        //to record the time the file was created (for time based rolling)
        virtual time_t  WriteLogHeader(FILE *inFile);
        time_t          ReadLogHeader(FILE* inFile);

    private:
    
        // Run function to roll log right at midnight   
        virtual SInt64      Run();

        FILE*           fLog;
        time_t          fLogCreateTime;
        char*           fLogFullPath;
        Bool16          fAppendDotLog;
        Bool16          fLogging;
        Bool16          RenameLogFile(const char* inFileName);
        Bool16          DoesFileExist(const char *inPath);
        static void     ResetToMidnight(time_t* inTimePtr, time_t* outTimePtr);
        char*           GetLogPath(char *extension);
        
        // To make sure what happens in Run doesn't also happen at the same time in the public functions.
        OSMutex         fMutex;
};

#endif // __QTSS_ROLLINGLOG_H__

