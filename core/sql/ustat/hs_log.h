/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef HSLOG_H
#define HSLOG_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_log.h
 * Description:  Logging utilities
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include <stdio.h>
#include <fstream>

#include "hs_const.h"

#include <time.h>        // for getimeofday
#include "ComSysUtils.h" // for NA_gettimeofday

// Merge diags area.
void HSFuncMergeDiags( Lng32 sqlcode
                     , const char *string0 = NULL
                     , const char *string1 = NULL
                     , NABoolean needCLIDiags = FALSE
                     , Lng32 fsError = 0
                     );

// Clean up logs as determined by log settings; assumes that it
// is called after diagnostics have been merged into CmpCommon::diags()
void HSCleanUpLog();

// -----------------------------------------------------------------------
// Class to catch and summarize diagnostic info at procedure return.
//
// Example:
//  long retcode;
//  HSErrorCatcher ec( retcode
//                   , sqlcode
//                   , NULL
//                   , TRUE
//                   );
//  retcode = somefunc(...);
//  if (retcode)
//     return retcode;
//  retcode = somefunc2(...);
//  if (retcode)
//     return retcode;
// -----------------------------------------------------------------------
class HSErrorCatcher {

public:

  HSErrorCatcher( Lng32 &retcode
                       , short sqlcode
                       , const char *msg
                       , Int32 needCLIDiags
                       , NABoolean inactivate=FALSE
                       )
    : retcode_(retcode),
      sqlcode_(sqlcode),
      string0_(msg),
      needCLIDiags_(needCLIDiags),
      string1_(NULL),
      isFinalized_(FALSE),
      inactivated_(inactivate)
    {
      retcode_ = 0;
    }

  ~HSErrorCatcher()
    {
      finalize();
    }

  inline void setString1(const char *str)
    {
      string1_ = str;
    }

  // This function is used to write an error to the diagnostics area if one
  // occurred. It is typically called by the destructor when a stack-allocated
  // HSErrorCatcher goes out of scope, but may be called directly. If so, it
  // becomes a no-op for any subsequent calls, including the one by the dtor.
  // 
  void finalize()
    {
      // Add error to diagnostic area if not EOF or float primary key (-1120).
      if ( !isFinalized_ &&
           !inactivated_ &&
           retcode_ &&
           retcode_ != HS_EOF &&
           retcode_ != -HS_PKEY_FLOAT_ERROR &&
           sqlcode_ )
        HSFuncMergeDiags(sqlcode_, string0_, string1_, needCLIDiags_);

      isFinalized_ = TRUE;
    }

private:
  Lng32 &retcode_;         // TRUE if error occurs.
  short sqlcode_;          // error (<0) or warning (>0).
  const char *string0_;    // optional string0.
  NABoolean needCLIDiags_; // TRUE if need to retrieve additional diags
                           // from CLI.
  const char *string1_;    // optional string1.
  NABoolean isFinalized_;  // becomes true if error caught before object destroyed
                           //   (prevents action by dtor)
  NABoolean inactivated_;  // Set to TRUE if we want to suppress the action
                           // of finalize() in the destructor. We might do
                           // this because this HSErrorCatcher was created
                           // within the scope of another; if we don't suppress
                           // we will report the same error twice. Another
                           // reason for doing this is we might be in a retry
                           // loop and don't want to report diagnostics for
                           // failures we will retry. (In this latter case, 
                           // there typically will be another HSErrorCatcher
                           // above the retry loop.
};

// -----------------------------------------------------------------------
// Standalone functions.
// -----------------------------------------------------------------------

// Log the location of the error.
void HSFuncLogError(Lng32 error, char *filename, Lng32 lineno);

// Wrapper to handle assertion failure. Do not assert a condition with any
// side effects, as it is evaluated a second time if false.
#define HS_ASSERT(b)                                        \
      {                                                     \
        if (NOT (b))                                        \
          {                                                 \
            GetHSContext()->preAssertionFailure("" # b "", __FILE__, __LINE__); \
            CMPASSERT(b);                                   \
          }                                                 \
      }
 
//Ignore the following WARNINGS
//    [6008] missing single-column histograms
//    [6007] missing multi-column histograms
//    [4030] non-standard DATETIME format
//    [2053] Optimizer pass two assertion failure (optimizer still attempts to produce a plan)
//    [4]    internal Warning
#define HSFilterWarning(retcode) \
        { \
          if ((retcode == 6008) || \
              (retcode == 6007) || \
              (retcode == 4030) || \
              (retcode == 2053) || \
              (retcode == 2024) || \
              (retcode == HS_WARNING)) \
            retcode = 0; \
        }

// Map any error (<0) code other than HS_PKEY_FLOAT_ERROR to -1.
#define  HSFilterError(retcode) \
      {  \
         if (retcode < 0 && retcode != -HS_PKEY_FLOAT_ERROR) retcode = -1; \
      }

      
#define  HSHandleErrorIUS(retcode) \
      {  \
         HSFilterWarning(retcode); \
         if (retcode) { \
            end_IUS_work(); \
         } \
         HSHandleError(retcode); \
      }

#define  HSHandleError(retcode) \
      {  \
         HSFilterWarning(retcode); \
         if (retcode) { \
           HSFuncLogError(retcode, (char *)HS_FILE, (Lng32)__LINE__); \
           HSFilterError(retcode); \
           return retcode; \
         } \
      }

#define  HSLogError(retcode) \
      {  \
         HSFilterWarning(retcode); \
         if (retcode) { \
           HSFuncLogError(retcode, (char *)HS_FILE, (Lng32)__LINE__); \
         } \
      }

#define HSExitIfError(retcode) \
      {  \
         HSFilterWarning(retcode); \
         if (retcode) { \
           HSFuncLogError(retcode, (char *)HS_FILE, (Lng32)__LINE__); \
           HSFilterError(retcode); \
           HSCleanUpLog(); \
           HSClearCLIDiagnostics(); \
           return retcode; \
         } \
      }


// -----------------------------------------------------------------------
// CLASS: HSLogMan                                             Log Manager
// -----------------------------------------------------------------------

const Lng32   MAX_TIMING_EVENTS = 10; //3; //@ZXtemp 10;

class HSLogMan
  {
    public:
         static HSLogMan* Instance();
         inline NABoolean LogNeeded() const {return logNeeded_;}
         void Log(const char *data);      /* data string to write to log     */
         void StartLog(NABoolean needExplain = TRUE);    /* start the log    */
         void StopLog();                  /* stop the log                    */
         void StopAndDeleteLog();         /* stop the log and delete it      */
         void ClearLog();                 /* erase the log file              */
         void StartTimer(const char *title = ""); /* start time-watch        */
         void StopTimer();                /* stop time-watch                 */
         void LogTimeDiff(const char *text = "", NABoolean reset = FALSE); /* log time difference     */
         void LogTimestamp(const char *text = ""); /* log current local time, to microseconds */
         char msg[2000];                  /* general purpose buffer for      */
                                          /* formmatting messages            */

         NAString* logFileName() { return logFile_; };

         enum LogSetting 
           {
             ON,      // log all activity continuously in one log until explicitly 
                      // turned off
             OFF,     // do not log any activity 
             SYSTEM   // log each UPDATE STATS command in a separate log; keep log
                      // only if interesting errors occur
           };

         LogSetting GetLogSetting() const { return logSetting_; }
         void SetLogSetting(LogSetting logSetting);

         // the following are intended as a long-term replacement for the
         // use of the fixed-length buffer msg and the Log() method
         HSLogMan & operator<<(const char * data);
         HSLogMan & operator<<(Int64 data);
         void FlushToLog();  // writes out logStreamBuffer_ and clears it

    protected:
         HSLogMan();                      /* ensure only 1 instance of class */
    private:
         LogSetting logSetting_;
         NABoolean logNeeded_;            /* T: logging is needed            */
         NAString *logFile_;              /* log filename                    */

         Lng32     currentTimingEvent_;
         Int64     startTime_[MAX_TIMING_EVENTS];
         NABoolean explainOn_;
         timeval prevTime_;
         char title_[MAX_TIMING_EVENTS][1000];
         static THREAD_P HSLogMan* instance_;      /* 1 and only 1 instance           */

         // intended as a long-term replacement for msg + Log()
         NAString logStreamBuffer_;
  };
#endif /* HSLOG_H */
