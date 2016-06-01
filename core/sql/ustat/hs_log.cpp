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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_log.C
 * Description:  Utilities for logging, diagnostics handling.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define HS_FILE "hs_log"

#include "CmpCommon.h"
#include "CmpErrLog.h"
#include "SchemaDB.h"
#include "NAError.h"
#include "cli_stdh.h"
#include "sql_id.h"

#include "hs_cont.h"
#include "hs_globals.h"

#include "SQLCLIdev.h"
#include "ComDiags.h"
#include "ComCextdecs.h"

#include "NAClusterInfo.h"

#define ULOG_PATH_WARN 2244

// -----------------------------------------------------------------------
// Fill diagnostic area:
//  - merge in CLI diagnostic info if needed.
//  - add in our own diagnostic info.
// -----------------------------------------------------------------------
void HSFuncMergeDiags( Lng32 sqlcode
                     , const char *string0
                     , const char *string1
                     , NABoolean needCLIDiags
                     , Lng32 fsError
                     )
{
  HSLogMan *LM = HSLogMan::Instance();
  HSGlobalsClass *hs_globals = GetHSContext();
  if (hs_globals == NULL) return;
  if (hs_globals->requestedByCompiler) return; // Do not create errors if quick stats.
  ComDiagsArea &diagsArea = hs_globals->diagsArea;
  Lng32 sqlcode2 = 0;

  ComDiagsArea diags(STMTHEAP);
  if (needCLIDiags)
    {
      SQL_EXEC_MergeDiagnostics_Internal(diags); // copy out diags area.

      //Make sure all 6007 and 6008 warnings are removed from DiagsArea. We do
      //not want user to know about these warnings while executing Update
      //Statistics.
      ComCondition *wEntry;
      for(Lng32 i = 1; i <= diags.getNumber(DgSqlCode::WARNING_) ; i++)
        {
          wEntry = diags.getWarningEntry(i);
          if (wEntry->getSQLCODE() == 6007 ||
              wEntry->getSQLCODE() == 6008)
            {
              diags.deleteWarning(--i);
            }
        }

      sqlcode2 = diags.mainSQLCODE();
    }

  diagsArea << DgSqlCode(sqlcode);
  if (hs_globals->objDef)
    {
      diagsArea << DgCatalogName(hs_globals->objDef->getCatName().data());
      diagsArea << DgSchemaName(hs_globals->objDef->getSchemaName().data());
      diagsArea << DgTableName(hs_globals->objDef->getObjectName().data());
    }
  if (string0 != NULL)
    {
      diagsArea << DgString0(string0);
      
      // Inaccessible object error uses TableName field as parameter value, but
      // it is not in hs_globals when we issue this error, so we have to use
      // the name passed to this fn as string0.
      if (sqlcode == -UERR_OBJECT_INACCESSIBLE)
        diagsArea << DgTableName(string0);
    }
  if (string1 != NULL)
    diagsArea << DgString1(string1);
    
  // If an internal error was triggered by some other error, add a parameter
  // that specifies that error number. The format of the message is such that
  // it will read correctly with or without this parameter.
  // LCOV_EXCL_START :rfi
  if (sqlcode == -UERR_INTERNAL_ERROR)
    {
      if (sqlcode2 < 0)
        {
          char errNumParam[10];
          snprintf(errNumParam, sizeof(errNumParam), " (%d)", -sqlcode2);
          diagsArea << DgString2(errNumParam);
        }
      else
        diagsArea << DgString2("");
    }
  // LCOV_EXCL_STOP
    
  if (fsError)
    diagsArea << DgInt0(fsError);
  if (needCLIDiags && sqlcode2)
    diagsArea.mergeAfter(diags);

  CmpCommon::diags()->mergeAfter(diagsArea);

  if (LM->LogNeeded())
    {
      snprintf(LM->msg, sizeof(LM->msg), "*** MERGEDIAGS: SQLCODE=%d", sqlcode );
      LM->Log(LM->msg);
      if (string0 != NULL)
        {
          snprintf(LM->msg, sizeof(LM->msg), ", TOK1=%s", string0);
          LM->Log(LM->msg);
        }
      if (string1 != NULL)
        {
          snprintf(LM->msg, sizeof(LM->msg), ", TOK2=%s", string1);
          LM->Log(LM->msg);
        }
      if (sqlcode2)
        {
          snprintf(LM->msg, sizeof(LM->msg), ", SQLCODE2=%d", sqlcode2);
          LM->Log(LM->msg);
        }
    }
}



// -----------------------------------------------------------------------
// Log the location of error.
// All non-zero returns will invoke this procedure, including warnings,
// which are denoted by positive integers. If we handle an error here and
// logging is not enabled, we activate Just-In-Time logging (subject to
// CQD setting) dump as much information as possible about the error.
// -----------------------------------------------------------------------
void HSFuncLogError(Lng32 error, char *filename, Lng32 lineno)
{
  HSGlobalsClass *hs_globals = GetHSContext();
  HSLogMan *LM = HSLogMan::Instance();

  // put this fn call in all error routines
  if (error != 100)
    NAError_stub_for_breakpoints() ;

  if (LM->LogNeeded())
    {
      if (error < 0)
        snprintf(LM->msg, sizeof(LM->msg), "*** ERROR[%d] in %s:%d\n", error, filename, lineno);
      else
        snprintf(LM->msg, sizeof(LM->msg), "*** WARNING[%d] in %s:%d\n", error, filename, lineno);
      LM->Log(LM->msg);
    }
  else if (error < 0 && CmpCommon::getDefault(USTAT_JIT_LOGGING) == DF_ON)
    {
      // Just-in-time logging is enabled; write critical info to log.
      LM->StartLog(FALSE);
      LM->Log("#####>>  Update Statistics Just-In-Time logging activated");
      snprintf(LM->msg, sizeof(LM->msg), "*** ERROR[%d] in %s:%d", error, filename, lineno);
      LM->Log(LM->msg);
      LM->LogTimestamp("Error occurred");
      if (hs_globals)
        {
          // This function is called from HSHandleError, which may be invoked
          // more than once for a single error as the stack is unwound. The
          // information from HSGlobalsClass is only needed in the log once
          // whether or not subsequent errors are distinct from the first one.
          // For errors after the first, we append a warning that it may be a
          // duplicate of one already seen.
          Int32 offset = snprintf(LM->msg, sizeof(LM->msg), "Error number %d for this statement", 
                                        hs_globals->errorCount+1);
          if (hs_globals->errorCount > 0)
            snprintf(LM->msg + offset, sizeof(LM->msg)-offset, " (may be duplicate posting)");
          LM->Log(LM->msg);
          if (hs_globals->errorCount == 0)
            hs_globals->log(LM);  // Add information from HSGlobalsClass
        }

      // Log this error to the file indicated by CMP_ERR_LOG_FILE CQD.
      // first construct the message
      snprintf(LM->msg,  sizeof(LM->msg), "Failure in ustat code: Error %d in %s:%d", error, filename, lineno);
      CmpErrLog(LM->msg);
      LM->Log("#####>>  Recorded this failure in CMP_ERR_LOG_FILE \n");

      LM->Log("#####>>  End of JIT log entry\n");
      LM->StopLog();
    }

  // If not a warning, bump global error count and set file and line if first one.
  if (hs_globals && error < 0)
    {
      if (hs_globals->errorCount == 0)
        {
          hs_globals->errorFile = filename;
          hs_globals->errorLine = lineno;
        }
      hs_globals->errorCount++;
    }
}



/*****************************************************************************/
/* CLASS:   HSLogMan                                                         */
/* PURPOSE: Log manager for all of Update Statistics needs.                  */
/* DEPENDENCY:                                                               */
/* NOTES:   This is a singleton class, which means that there could only be  */
/*          one instance of this class.                                      */
/*****************************************************************************/
THREAD_P HSLogMan* HSLogMan::instance_ = 0;
HSLogMan::HSLogMan()
: currentTimingEvent_(-1), logNeeded_(FALSE)
  {
    memset(startTime_, 0, MAX_TIMING_EVENTS*sizeof(Int64));
    logFile_ = new (CTXTHEAP) NAString();
    prevTime_.tv_sec = prevTime_.tv_usec = 0;
  }

/***********************************************/
/* METHOD:  Instance()                         */
/* PURPOSE: Returns the instance of the class. */
/* NOTES:   We need to instantiate using the   */
/*          contextHeap because we need this   */
/*          class to stay around through       */
/*          multiple statements.               */
/***********************************************/
HSLogMan* HSLogMan::Instance()
  {
    if (instance_ == 0)
      instance_ = new (GetCliGlobals()->exCollHeap()) HSLogMan;
    return instance_;
  }

/***********************************************/
/* METHOD:  Log()                              */
/* PURPOSE: Write input parameter contents to  */
/*          log.                               */
/* INPUT:   data - data string to be written   */
/***********************************************/
void HSLogMan::Log(const char *data)
  {
    if (logNeeded_)
      {
        ofstream fileout(logFile_->data(), ios::app);
        fileout << data << endl;
      }
  }

// -----------------------------------------------------------------------
// Check whether the dir containing ULOG file exists,
// making sure ULOG can be created in it later.
// -----------------------------------------------------------------------
NABoolean HSLogMan::ContainDirExist(const char* path)
{
  CMPASSERT(strlen(path)>0);
  struct stat sb;
  NAString containDir = path;
  Int32 pos = containDir.last('/');
  if(-1 == pos)
    return TRUE;
  containDir = containDir.remove(pos);
  Int32 rVal = stat(containDir.data(), &sb);
  
  NABoolean dirExist = rVal != -1 && S_ISDIR(sb.st_mode);
    
  if(!dirExist)
  {
    char buf[256];
    snprintf(buf, sizeof(buf), "Directory %s does not exist", containDir.data());
    *CmpCommon::diags() << DgSqlCode(ULOG_PATH_WARN) << DgString0(buf);
  }
  //contain dir exist but not accessible
  if(dirExist&&
       (!((sb.st_mode & S_IRWXU)&S_IXUSR)
      ||!((sb.st_mode & S_IRWXU)&S_IRUSR)
      ||!((sb.st_mode & S_IRWXU)&S_IWUSR)))
  {
    char buf[256];
    snprintf(buf, sizeof(buf), "Directory %s Permission denied", containDir.data());
    *CmpCommon::diags() << DgSqlCode(ULOG_PATH_WARN) << DgString0(buf);
    //return FALSE so we don't start logging
    dirExist = FALSE;
  }

  return dirExist;
}

// -----------------------------------------------------------------------
// @param: path(OUT), return changed real path on success.
// @param: cqd_valud(IN), filename set by CQD USTAT_LOG ...
// In situation of cluster:
// prefix $TAR_DOWNLOAD_ROOT to value of USTAT_LOG to form a safe log path, 
// and change file name to filename.<tdm_arkcmp>.<NodeId>.<Hostname>.<pid>.log,
// e.g. USTAT_LOG="ULOG", real path will be $TAR_DOWNLOAD_ROOT/ULOG.<tdm_arkcmp>.<NodeId>.<Hostname>.<pid>.log.
// For cqd_value, an absolute path is not allowd, and will return FALSE.
// -----------------------------------------------------------------------
NABoolean HSLogMan::GetLogFile(NAString & path, const char* cqd_value)
{
    CMPASSERT(strlen(cqd_value) > 0);
    if('/' == cqd_value[0])
    {//Absolute path is not allowed on cluster.
      *CmpCommon::diags() << DgSqlCode(ULOG_PATH_WARN) << DgString0("Absolute path is not allowed on cluster.");
      return FALSE;
    }
    else if( getenv("TAR_DOWNLOAD_ROOT") )
    {// relative path
      const size_t HOSTNAME_SIZE = 64; 
      char hostname[HOSTNAME_SIZE];
      Int32 nodeNum;
      Int32 pin;
      SB_Phandle_Type procHandle;
      XPROCESSHANDLE_GETMINE_(&procHandle);
      XPROCESSHANDLE_DECOMPOSE_(&procHandle, &nodeNum, &pin);
      gethostname(hostname, sizeof(hostname));
      path = NAString(getenv("TAR_DOWNLOAD_ROOT")) + "/" + NAString(cqd_value)
				+ "."  + "tdm_arkcmp" 
				+ "." + LongToNAString((Lng32)nodeNum)
                                + "." + hostname
				+ "." + LongToNAString((Lng32)pin)
				+".log";
    }
    else
    {//Environment variable $TAR_DOWNLOAD_ROOT not set
      *CmpCommon::diags() << DgSqlCode(ULOG_PATH_WARN) << DgString0("Environment variable $TAR_DOWNLOAD_ROOT is not set on cluster.");
      return FALSE;
    }
    return TRUE;
}

/***********************************************/
/* METHOD:  StartLog()                         */
/* PURPOSE: Begin capturing log information    */
/* INPUT:   needExplain - Indicator of whether */
/*            to turn on cqd                   */
/*            to get explain data for dynamic  */
/*            queries. TRUE except for just-in-*/
/*            time logging, where we want to   */
/*            avoid clearing dx area.          */
/*          logFileName - If NULL (default), a */
/*            cqd determines log file name.    */
/* NOTES:   It is possible for the USTAT_LOG   */
/*          attribute to be modified many      */
/*          times but will not be official     */
/*          until either StartLog() or         */
/*          ClearLog() methods are called.     */
/***********************************************/
void HSLogMan::StartLog(NABoolean needExplain, const char* logFileName)
  {
    // The GENERATE_EXPLAIN cqd captures explain data pertaining to dynamic
    // queries. Ordinarily we want it on, but for just-in-time logging triggered
    // by an error, we don't need it, and can't set it because HSFuncExecQuery
    // clears the diagnostics area, which causes the error to be lost.
    explainOn_ = needExplain;
    if (!needExplain ||
        HSFuncExecQuery("CONTROL QUERY DEFAULT GENERATE_EXPLAIN 'ON'") == 0)
      {
        CollIndex activeNodes = gpClusterInfo->numOfSMPs();
        if (logFileName)
        {
          *logFile_ = logFileName;
           currentTimingEvent_ = -1;
           startTime_[0] = 0;           /* reset timer           */
           logNeeded_ = TRUE;
        }
        else if(activeNodes > 2)
        {//we consider we are running on cluster 
         //if gpClusterInfo->numOfSMPs() > 2
           NABoolean ret = FALSE;
           if(GetLogFile(*logFile_, ActiveSchemaDB()->getDefaults().getValue(USTAT_LOG)))
             ret = ContainDirExist(logFile_->data());

           if(ret)
             logNeeded_ = TRUE;

           currentTimingEvent_ = -1;
           startTime_[0] = 0;           /* reset timer           */
        }
        else
        {
          *logFile_ = ActiveSchemaDB()->getDefaults().getValue(USTAT_LOG);
           currentTimingEvent_ = -1;
           startTime_[0] = 0;           /* reset timer           */
           logNeeded_ = TRUE;
        }
      }
  }

/***********************************************/
/* METHOD:  StopLog()                          */
/* PURPOSE: Stop capturing log information     */
/* INPUT:   none                               */
/***********************************************/
void HSLogMan::StopLog()
  {
    // Stop capturing explain data for dynamic queries. Skip this if it is not
    // on, to avoid clearing the diagnostics area for just-in-time logging.
    if (explainOn_)
      HSFuncExecQuery("CONTROL QUERY DEFAULT GENERATE_EXPLAIN RESET");
    logNeeded_ = FALSE;
  }

/***********************************************/
/* METHOD:  ClearLog()                         */
/* PURPOSE: Erase the contents of the logfile  */
/* INPUT:   none                               */
/* NOTES:   It is possible for the USTAT_LOG   */
/*          attribute to be modified many      */
/*          times but will not be official     */
/*          until either StartLog() or         */
/*          ClearLog() methods are called.     */
/***********************************************/
void HSLogMan::ClearLog()
  {
    //*logFile_ = ActiveSchemaDB()->getDefaults().getValue(USTAT_LOG);
    struct stat sb;
    if(0==stat(logFile_->data(), &sb))//if log exists, empty it.
      ofstream fileout(logFile_->data(), ios::out);
    currentTimingEvent_ = -1;
    startTime_[0] = 0;                 /* reset timer           */
  }

/***********************************************/
/* METHOD:  StartTimer()                       */
/* PURPOSE: Capture initial timestamp for      */
/*          determining how long a specific    */
/*          operation takes.                   */
/* INPUT:   Header/title information for a     */
/*          specific operation.                */
/***********************************************/
void HSLogMan::StartTimer(const char *title)
  {
    if (logNeeded_)
      {
        char timerMsg[2000];
        currentTimingEvent_++;

        strcpy(timerMsg, ":");
        for (Lng32 i=1; i<=currentTimingEvent_; i++)  // no spacer for unnested event
          strcat(timerMsg, "|  ");
        strcat(timerMsg, "BEGIN ");

        if (currentTimingEvent_ >= MAX_TIMING_EVENTS)  // 0-based
          {
            snprintf(timerMsg + strlen(timerMsg), sizeof(timerMsg) - strlen(timerMsg),
                     "Logging error: timing events nested too deeply at event '%s'.",
                     title);
            Log(timerMsg);
          }
        else
          {
            strcpy(title_[currentTimingEvent_], title);

            strcat(timerMsg, title);
            Log(timerMsg);

            startTime_[currentTimingEvent_] = NA_JulianTimestamp();
        }
      }
  }


/***********************************************/
/* METHOD:  StopTimer()                        */
/* PURPOSE: Capture final timestamp to         */
/*          determine how long a specific      */
/*          operation takes. Result is written */
/*          to the logfile.                    */
/* INPUT:   none                               */
/***********************************************/
void HSLogMan::StopTimer()
  {
    if (!logNeeded_)
      return;

    // Capture elapsed time.
    Int64 timeDiff = 0;
    if (currentTimingEvent_ < MAX_TIMING_EVENTS)
      timeDiff = NA_JulianTimestamp() - startTime_[currentTimingEvent_];

    // Prefix of timer termination string to reflect nesting level.
    strcpy(msg, ":");
    for (Lng32 i=1; i<=currentTimingEvent_; i++)  // no spacer for unnested event
      strcat(msg, "|  ");
    strcat(msg, "END   ");

    if (currentTimingEvent_ >= MAX_TIMING_EVENTS)  // 0-based
      {
        strcat(msg, "Untimed event.");
        Log(msg);
        currentTimingEvent_--;
      }
    else if (currentTimingEvent_ >= 0 &&
             startTime_[currentTimingEvent_] != 0)
      {
        startTime_[currentTimingEvent_] = 0;

        ULng32 ms  = (ULng32) (((timeDiff % 1000000) + 500) / 1000);
        ULng32 sec = (ULng32) (timeDiff / 1000000);
        ULng32 min = sec/60;
        sec = sec % 60;
        ULng32 hour = min/60;
        min = min % 60;
        
        snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg),
                 "%s elapsed time (%02u:%02u:%02u.%03u)\n",
                 title_[currentTimingEvent_], hour, min, sec, ms);
        Log(msg);
        currentTimingEvent_--;
      }
    else
      Log("Logging error: StopTimer() called without matching StartTimer().");
  }

/***********************************************/
/* METHOD:  LogTimeDiff()                      */
/* PURPOSE: Log time difference                */
/*          between calls.                     */
/*          Result is written to the logfile.  */
/* INPUT:   Log message and reset flag         */
/***********************************************/
void HSLogMan::LogTimeDiff(const char *text, NABoolean reset) 
  {
    if (!logNeeded_) return;

    float elapsed = 0.0;

    timeval curTime;
    if (gettimeofday(&curTime, 0) != 0)
      Log("ERROR: gettimeofday failed.\n");  // LCOV_EXCL_LINE :rfi
    else
      // calcualte time difference only when this is not a reset and prevtime has been set
      if (!reset && (prevTime_.tv_sec != 0 || prevTime_.tv_usec != 0))  
        elapsed = (float)(curTime.tv_sec - prevTime_.tv_sec) +
                  (float)(curTime.tv_usec - prevTime_.tv_usec)/1000000;

    snprintf(msg, sizeof(msg), "%s Time Difference  %.6f\n", text, elapsed);
    Log(msg);
    prevTime_ = curTime;
  }

/************************************************/
/* METHOD:  LogTimestamp()                      */
/* PURPOSE: Log current timestamp in local time */
/*          prefixed by the given text message. */
/*          Result is written to the logfile.   */
/* INPUT:   Message to prepend to timestamp.    */
/************************************************/
void HSLogMan::LogTimestamp(const char *text)
  {
    if (!logNeeded_) return;

    Int64 gmtTS = NA_JulianTimestamp();
    short ts[8] = {0,0,0,0,0,0,0,0};
    Int64 localTS = NA_ConvertTimestamp(gmtTS);
    NA_InterpretTimestamp(localTS, ts);
    snprintf(msg, sizeof(msg),
            "%s: %04d-%02d-%02d %02d:%02d:%02d.%03d%03d",
            text, ts[0], ts[1], ts[2], ts[3], ts[4], ts[5], ts[6], ts[7]);
    Log(msg);
  }
