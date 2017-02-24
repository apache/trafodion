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

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "ExSMTrace.h"
#include "ExSMCommon.h"
#include "ExSMGlobals.h"

__thread bool EXSM_TRACE_ENABLED = false;

static __thread FILE *EXSM_OUTFILE = NULL;
static __thread uint32_t EXSM_TRACE_LEVEL = 0;
static __thread int EXSM_TID = 0;
static __thread int EXSM_NODE_NUMBER = 0;
static __thread char *EXSM_OUTFILE_NAME = NULL;

// Function to set the tracing level
void ExSM_SetTraceLevel(uint32_t lvl)
{
  EXSM_TRACE_LEVEL = lvl;
}

// Function to figure out the effective trace level and trace file prefix
// Also sets the effective trace level in SM globals.
void ExSM_SetTraceInfo(uint32_t sessionTraceLevel, 
                       const char *sessionTraceFilePrefix,
                       uint32_t *effectiveTraceLevel, 
                       const char **effectiveTraceFilePrefix)
{
  const char *smEnvTrcFilePrefix = getenv("EXSM_TRACE_FILE");
  const char *smEnvTrcLevel = getenv("EXSM_TRACE_LEVEL");
  const char *smTrcFileDefaultPrefix = "exsm.out";
  const char *smTrcFilePrefix =
    (sessionTraceFilePrefix == NULL) ? 
    ((smEnvTrcFilePrefix == NULL) ? 
     smTrcFileDefaultPrefix : 
     smEnvTrcFilePrefix) :
    sessionTraceFilePrefix;
  
  const int smTrcLevel = (sessionTraceLevel == 0) ?
    ((smEnvTrcLevel == NULL) ?
     EXSM_TRACE_OFF :
     (int) strtoul(smEnvTrcLevel, NULL, 16)) :
    sessionTraceLevel;
  
  ExSM_SetTraceLevel(smTrcLevel);
  
  if (effectiveTraceLevel)
    *effectiveTraceLevel = smTrcLevel;
  
  if (effectiveTraceFilePrefix)
    *effectiveTraceFilePrefix = (char *) smTrcFilePrefix;
}

// Function to turn on/off tracing based on effective trace level.
// Also creates the trace file name and opens the file.
// There are several global variables that holds the current trace state
//
// EXSM_TRACE_ENABLED  - boolean indicating if trace is on/off
// EXSM_OUTFILE        - stream pointer for open file
// EXSM_OUTFILE_NAME   - string of current opened trace file name
// EXSM_NODE_NUMBER    - node number for the node the process is on
// EXSM_TID            - thread id
//
void ExSM_SetTraceEnabled(bool b, ExSMGlobals *smGlobals)
{
  exsm_assert(smGlobals, "Invalid SM globals pointer");

  // Close the output file if tracing goes from ON to OFF and the file
  // is not stdout
  if ((EXSM_TRACE_ENABLED == true) &&
      (b == false) && (EXSM_OUTFILE != NULL))
  {
    char timeString[40];
    timeval tv;
    tm tx;
    int rc;
    
    rc = gettimeofday(&tv, NULL);
    exsm_assert_rc(rc, "gettimeofday");
      
    localtime_r(&tv.tv_sec, &tx);
    sprintf(timeString, "%04d%02d%02d %02d:%02d:%02d.%06d",
            tx.tm_year + 1900, tx.tm_mon + 1, tx.tm_mday,
            tx.tm_hour, tx.tm_min, tx.tm_sec, (int) tv.tv_usec);
    
    fprintf(EXSM_OUTFILE, "%d:%d %s Trace disabled\n", 
            EXSM_NODE_NUMBER, EXSM_TID, timeString);
    fflush(EXSM_OUTFILE);
    fclose(EXSM_OUTFILE);
    EXSM_OUTFILE = NULL;
  }

  EXSM_TRACE_ENABLED = b;

  if (b)
  {
    // Generate a timestamp string for trace messages generated
    // internally by this function
    timeval tv;
    tm tx;
    char timeString[40];
    int rc = gettimeofday(&tv, NULL);
    exsm_assert_rc(rc, "gettimeofday");
    localtime_r(&tv.tv_sec, &tx);
    sprintf(timeString, "%04d%02d%02d %02d:%02d:%02d.%06d",
            tx.tm_year + 1900, tx.tm_mon + 1, tx.tm_mday,
            tx.tm_hour, tx.tm_min, tx.tm_sec, (int) tv.tv_usec);
    
    EXSM_NODE_NUMBER = smGlobals->getSQNodeNum();
    EXSM_TID = ExSM_GetThreadID();

    int node = EXSM_NODE_NUMBER;
    int tid = EXSM_TID;
    const char *sqRoot = getenv("TRAF_HOME");
    exsm_assert(sqRoot, "getenv(TRAF_HOME) returned NULL");
    char *filename = new char[PATH_MAX + 1];
    exsm_assert(filename, "Could not allocate SM trace file name");

    // Generate the trace file name
    // * If the file prefix is "stdout", use stdout
    // * Otherwise
    //    * The generated name will be <prefix>.<other-info>
    //      where <other-info> includes the node ID and process ID
    //    * If <prefix> starts with "/", it is an absolute path
    //    * Otherwise the file is placed under $TRAF_HOME/tmp
    if (!strncmp(smGlobals->getTraceFilePrefix(), "stdout", 6))
    {
      EXSM_OUTFILE = stdout;
      strcpy(filename, "stdout");
      EXSM_OUTFILE_NAME = filename;
      fprintf(EXSM_OUTFILE,
              "%d:%d %s trcLvl=%x, outfile %s\n",
              node, tid, timeString, smGlobals->getTraceLevel(), filename);
      fflush(EXSM_OUTFILE);
    }
    else
    {
      if (!strncmp(smGlobals->getTraceFilePrefix(), "/", 1))
        sprintf(filename, "%s.%03d.%s.%d.%d.log", 
                smGlobals->getTraceFilePrefix(), node,
                smGlobals->getSessionID(), getpid(), tid);
      else
        sprintf(filename, "%s/tmp/%s.%03d.%s.%d.%d.log", sqRoot,
                smGlobals->getTraceFilePrefix(), node,
                smGlobals->getSessionID(), getpid(), tid);
    }

    // Now consider whether the file name has changed. After a change
    // we close the old file and open a new one.

    if ((EXSM_OUTFILE_NAME == NULL) ||
        (strcmp(filename, EXSM_OUTFILE_NAME)))
    {
      // The name changed or we did not previously have a name

      if (EXSM_OUTFILE)
      {
        fprintf(EXSM_OUTFILE,
                "%d:%d %s File already open: old %s, new %s\n",
                node, tid, timeString, EXSM_OUTFILE_NAME, filename);
        fclose(EXSM_OUTFILE);
        EXSM_OUTFILE = NULL;
      }

      if (EXSM_OUTFILE_NAME)
      {
        delete [] EXSM_OUTFILE_NAME;
        EXSM_OUTFILE_NAME = NULL;
      }

      EXSM_OUTFILE_NAME = filename;
      EXSM_OUTFILE = fopen(filename, "w");
      exsm_assert(EXSM_OUTFILE, "Failed to open tracefile");
      
      fprintf(EXSM_OUTFILE, "%d:%d %s trcLvl=%x, outfile %s\n",
              node, tid, timeString, smGlobals->getTraceLevel(), filename);
      fflush(EXSM_OUTFILE);
    }
    else
    {
      // Reuse the current output file. filename can be deleted
      // because it matches EXSM_OUTFILE_NAME.

      delete [] filename;
      filename = NULL;

      if (EXSM_OUTFILE == NULL)
      {
        EXSM_OUTFILE = fopen(EXSM_OUTFILE_NAME, "w+");
        exsm_assert(EXSM_OUTFILE, "Failed to open tracefile");
      }

      fprintf(EXSM_OUTFILE,
              "%d:%d %s trcLvl=%x, REUSING outfile %s\n",
              node, tid, timeString, smGlobals->getTraceLevel(), filename);
      fflush(EXSM_OUTFILE);

    } // if (output file changed) else ...
  } // if (b)
}

// The trace function
void ExSM_Trace(uint32_t traceLevel, const char *formatString, ...)
{
  static bool printed_disabled_msg = false;
  if (!EXSM_TRACE_ENABLED)
  {
    if ((EXSM_OUTFILE != NULL) && ( printed_disabled_msg == false )) 
    {
      char timeString[40];
      timeval tv;
      tm tx;
      int rc;
    
      rc = gettimeofday(&tv, NULL);
      exsm_assert_rc(rc, "gettimeofday");
      
      localtime_r(&tv.tv_sec, &tx);
      sprintf(timeString, "%04d%02d%02d %02d:%02d:%02d.%06d",
              tx.tm_year + 1900, tx.tm_mon + 1, tx.tm_mday,
              tx.tm_hour, tx.tm_min, tx.tm_sec, (int) tv.tv_usec);
    
      fprintf(EXSM_OUTFILE, "%d:%d %s Trace disabled **\n", 
              EXSM_NODE_NUMBER, EXSM_TID, timeString);
      fflush(EXSM_OUTFILE);
      printed_disabled_msg = true;
    }

    return;
  }
  else
  {
    // if tracing got enabled again, make sure we can print a disabled
    // msg again
    printed_disabled_msg = false;
  }
  
  // Return early without printing anything if the global trace level
  // (EXSM_TRACE_LEVEL) does not include the wanted level (traceLevel)
  if ((EXSM_TRACE_LEVEL & traceLevel) == 0)
    return;

  va_list args;
  char timeString[40];
  timeval tv;
  int rc;
  tm tx;
  int tid = EXSM_TID;
  int node = EXSM_NODE_NUMBER;

  rc = gettimeofday(&tv, NULL);
  exsm_assert_rc(rc, "gettimeofday");
  
  localtime_r(&tv.tv_sec, &tx);
  sprintf(timeString, "%04d%02d%02d %02d:%02d:%02d.%06d",
          tx.tm_year + 1900, tx.tm_mon + 1, tx.tm_mday,
          tx.tm_hour, tx.tm_min, tx.tm_sec, (int) tv.tv_usec);

  char buf[1024];
  va_start(args, formatString);
  vsnprintf(buf, 1024, formatString, args);
  fprintf(EXSM_OUTFILE, "%d:%d %s %s\n", node, tid, timeString, buf);
  fflush(EXSM_OUTFILE);
}
