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
                       )
    : retcode_(retcode),
      sqlcode_(sqlcode),
      string0_(msg),
      needCLIDiags_(needCLIDiags),
      string1_(NULL),
      isFinalized_(FALSE)
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
};

// -----------------------------------------------------------------------
// Standalone functions.
// -----------------------------------------------------------------------

// Log the location of the error.
void HSFuncLogError(Lng32 error, char *filename, Lng32 lineno);

// Wrapper to handle assertion failure.
#define HS_ASSERT(b)                                        \
        if (NOT (b))                                        \
          {                                                 \
            HSTranMan *TM = HSTranMan::Instance();          \
            HSLogMan *LM = HSLogMan::Instance();            \
            if (LM->LogNeeded())                            \
              {                                             \
                sprintf(LM->msg, "***[ERROR] INTERNAL ASSERTION (%s) AT %s:%i", "" # b "", __FILE__, __LINE__); \
                LM->Log(LM->msg);                           \
              }                                             \
            if (TM->StartedTransaction())                   \
              TM->Rollback();                               \
            CMPASSERT(b);                                   \
          }
 
//Ignore the following WARNINGS
//    [6008] missing single-column histograms
//    [6007] missing multi-column histograms
//    [4030] non-standard DATETIME format
//    [4]    internal Warning
#define HSFilterWarning(retcode) \
          if ((retcode == 6008) || \
              (retcode == 6007) || \
              (retcode == 4030) || \
              (retcode == HS_WARNING)) \
            retcode = 0;

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
         void StartLog(NABoolean needExplain = TRUE,     /* start the log    */
                       const char* logFileName = NULL);  /* allow override of default */
         void StopLog();                  /* stop the log                    */
         void ClearLog();                 /* erase the log file              */
         void StartTimer(const char *title = ""); /* start time-watch        */
         void StopTimer();                /* stop time-watch                 */
         void LogTimeDiff(const char *text = "", NABoolean reset = FALSE); /* log time difference     */
         void LogTimestamp(const char *text = ""); /* log current local time, to microseconds */
         char msg[2000];                  /* general purpose buffer for      */
                                          /* formmatting messages            */

         NAString* logFileName() { return logFile_; };
         NABoolean ContainDirExist(const char* path);
         NABoolean GetLogFile(NAString & logFile, const char* cqc_value);
    protected:
         HSLogMan();                      /* ensure only 1 instance of class */
    private:
         NABoolean logNeeded_;            /* T: logging is needed            */
         NAString *logFile_;              /* log filename                    */

         Lng32     currentTimingEvent_;
         Int64     startTime_[MAX_TIMING_EVENTS];
         NABoolean explainOn_;
         timeval prevTime_;
         char title_[MAX_TIMING_EVENTS][1000];
         static THREAD_P HSLogMan* instance_;      /* 1 and only 1 instance           */
  };
#endif /* HSLOG_H */
