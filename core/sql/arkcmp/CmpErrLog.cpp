// ***************************************************************************
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
// -*-C++-*-
// ***************************************************************************
//
// File:         CmpErrLog.cpp
// Description:  This file contains the implementation of the CmpErrLog
//               class. It is used to log the transient properties that were
//               in affect when certain errors occur. This is meant solely to
//               make it easier for support personnel and SQL developers to
//               tell what was going on when errors occur. The need for 
//               logging resulted from error 2008 problems, but this logging
//               may be useful for other reasons too.
//
//               When an object of this class is constructed, this class
//               will log information about the system and process memory
//               statistics, the heap that caused the error, the context heap,
//               the statement heap, the CQDs that were in affect, the query
//               that was being executed, and a stack trace of the current
//               process.
//
//               This class has no outside functions that need to be called.
//               Simply constructing a class will cause the logging to occur.
//               For example, this class may be used as follows:
//
//                  if (errOccurred) {
//                    CmpErrLog("Memory allocation failure", heapPtr, size);
//                  }
//
// Created:      9/19/2008
// Language:     C++
//
// ***************************************************************************

#include "Platform.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>



#include <ctype.h>
#include <execinfo.h>
#include <sys/file.h>

#include "CmpErrLog.h"
#include "CmpCommon.h"
#include "CmpContext.h"
#include "CmpStatement.h"
#include "SchemaDB.h"
#include "Analyzer.h"
#include "vproc.h"

#define MAX_LOGFILE_SIZE (1 * 1024 * 1024) // One megabyte
#define MAX_LOGFILE_NAME_LEN  125

#define INITIAL_RESERVE_MEM_SIZE 2048
#define SUBSEQUENT_MEM_RESERVE_SIZE 1024

// Allocate some memory globally that can be freed to allow the
// CmpErrLog class to work.  Without any available memory, it could
// be difficult to open files or walk the stack. Windows, in
// particular, needs plenty of memory to lookup symbols.
void *CmpErrLog::memPtr = malloc(INITIAL_RESERVE_MEM_SIZE);

// CmpErrLog::renameBigLogFile() will resize the old log file if
// it is larger than MAX_LOGFILE_SIZE.  This function checks the
// current size of the file.
void CmpErrLog::renameBigLogFile(const char *fileName)
{
  struct stat sbuf;
  if (stat(fileName, &sbuf) != 0)
    return;

  if (sbuf.st_size >= MAX_LOGFILE_SIZE)
  {
    // Create the .old filename string
    char oldFileName[MAX_LOGFILE_NAME_LEN + 5];
    strncpy(oldFileName, fileName, sizeof(oldFileName));
    strncat(oldFileName, ".old", sizeof(oldFileName));

    // Rename the old file name.
    rename(fileName, oldFileName);
  }
}

// CmpErrLog::openLogFile() opens the logfile specified by the 
// CMP_ERR_LOG_FILE CQD and may lock the file to prevent concurrent
// compiler processes from trying to write to the file.
void CmpErrLog::openLogFile()
{
  CmpContext *context = CmpCommon::context();
  if (context == NULL)
    return;

  // Lookup the CMP_ERR_LOG_FILE CQD.
  const char *logFileName;
  logFileName = context->schemaDB_->getDefaults().getValue(CMP_ERR_LOG_FILE);

  // If the logfile is empty or is too long, then don't try to open it.
  // This code tries to not allocate any memory that isn't absolutely
  // needed and it makes assumptions as to the maximum pathname.
  if (!logFileName
      || logFileName[0] == '\0' 
      || strlen(logFileName) > MAX_LOGFILE_NAME_LEN)
    return;


  // On Linux determine the log location based off of TRAF_HOME environment
  // variable.
  char logFilePath[MAX_LOGFILE_NAME_LEN+1];
  char *sqlogs = getenv("TRAF_LOG");

  // Use a generated path for the logfile if 3 conditions are met:
  //   1. TRAF_LOG is set in the environment.
  //   2. The first character of the logfile starts with a character or digit.
  //   3. The complete path can fit into the size of the "logFilePath" buffer.
  if ((sqlogs != NULL) &&
      (isalnum(logFileName[0])) &&
      (strlen(sqlogs) + strlen(logFileName) < sizeof(logFilePath) - 2))
  {
    // Create the full path to the log file in the logFilePath buffer.
    snprintf(logFilePath, sizeof(logFilePath), "%s/%s",
             sqlogs, logFileName);

    // Call renameBigLogFile() to check the size and rename the file
    // if it is too big.
    renameBigLogFile(logFilePath);

    // Open the logfile.
    fp = fopen(logFilePath, "a");
  }
  else
  {
    renameBigLogFile(logFileName);
    fp = fopen(logFileName, "a");
  }

  // If the file could not be opened, then return.
  if (fp == NULL)
    return;

  // Change permissions so anybody can write to the file.
  (void)fchmod(fileno(fp), 0666);

  flock(fileno(fp), LOCK_EX);


  // Now that we are ensured to have exclusive access, seek to the
  // end for the case where another MXCMP may have just appended
  // to the file.
  (void)fseek(fp, 0, SEEK_END);
}

// CmpErrLog::closeLogFile() unlocks and closes the log file.
void CmpErrLog::closeLogFile()
{
  if (fp != NULL)
  {
    fflush(fp);

    flock(fileno(fp), LOCK_UN);

    fclose(fp); 
    fp = NULL;
  }

  // Attempt to reserve memory in case this class is needed for a
  // subsequent memory log.
  if (memPtr == NULL)
    memPtr = malloc(SUBSEQUENT_MEM_RESERVE_SIZE);
}

// Two macros to help with displaying of vproc string.
#define DefToStr2(a) #a
#define DefToStr(a) "" DefToStr2(a) ""

// CmpErrLog::writeHeader() writes to the log file information 
// about the time and a little information about the current
// process.
void CmpErrLog::writeHeader(const char *failureTxt, CollHeap *failedHeap, size_t size)
{
  fprintf(fp, "--------------------------------------------------------\n");
  // Save the local time to the log file.
  time_t curTime = time(NULL);
  fprintf(fp, "%s\n", ctime(&curTime));

  // Save the failure text.
  fprintf(fp, "%s\n", failureTxt); 

  // If a heap was passed in or the size is not equal to zero, then
  // save information about the heap and size of the allocation.
  if (failedHeap != NULL)
  {
    if (size != 0)
      fprintf(fp, "Failed while allocating " PFSZ " bytes from the heap at %p.\n\n",
              size, failedHeap);
    else
      fprintf(fp, "Failed while allocating from the heap at %p.\n\n",
              failedHeap);
  }
  else if (size != 0)
  {
    fprintf(fp, "Failed while allocating " PFSZ " bytes.\n", size);
  }

  // Write vproc string to the log file
  char vprocBuffer[80];;

  strcpy(vprocBuffer, DefToStr(PRODNUMCMP));
  strcat(vprocBuffer, "_");
  strcat(vprocBuffer, DefToStr(DATE1CMP));
  strcat(vprocBuffer, "_");
  strcat(vprocBuffer, DefToStr(CMP_CC_LABEL));

  fprintf(fp, "Software version: %s\n\n", vprocBuffer);
}

// CmpErrLog::writeMemoryStats() writes to the log file information
// about the available system memory and also any process statistics
// that are available.
void CmpErrLog::writeMemoryStats()
{

  fprintf(fp, "System Memory Statistics:\n");

  Int32 meminfoFD;
  ssize_t numBytes;
  char buf[256];

  if ((meminfoFD = open("/proc/meminfo", O_RDONLY)) == -1)
  {
    fprintf(fp, "Unable to open /proc/meminfo: %s\n\n", strerror(errno));
  }
  else
  {
    // Copy /proc/meminfo to the logfile.
    while ((numBytes = read(meminfoFD, buf, sizeof(buf))) > 0)
      fwrite(buf, sizeof(char), numBytes, fp);
    fputc('\n', fp);

    close(meminfoFD);
  }

  fprintf(fp, "Process Statistics:\n");

  // Copy /proc/<pid>/status to the log file.
  char procStatusFile[40];
  snprintf(procStatusFile, sizeof(procStatusFile), "/proc/%d/status", getpid());

  Int32 procStatusFD;
  if ((procStatusFD = open(procStatusFile, O_RDONLY)) == -1)
  {
    fprintf(fp, "Unable to open %s: %s\n\n", procStatusFile, strerror(errno));
  }
  else
  {
    // The copying of the file takes place in this loop.
    while ((numBytes = read(procStatusFD, buf, sizeof(buf))) > 0)
      fwrite(buf, sizeof(char), numBytes, fp);
    fputc('\n', fp);

    close(procStatusFD);
  }

}

// CmpErrLog::writeHeapInfo() writes to the log file information
// about a single heap.
void CmpErrLog::writeHeapInfo(CollHeap *heap)
{
  if (heap == NULL)
    return;

  fprintf(fp, "Heap           : %p\n", heap);
  fprintf(fp, "Name           : %s\n", heap->getName());
  fprintf(fp, "Total Size     : " PFSZ "\n", heap->getTotalSize());
  fprintf(fp, "Alloc Size     : " PFSZ "\n", heap->getAllocSize());
  fprintf(fp, "Highwater Mark : " PFSZ "\n\n", heap->getHighWaterMark());
}

// CmpErrLog::writeHeapInfo() writes to the log file information 
// about the heap passed in and about the context heap and
// statement heap (if they are different than the current heap).
void CmpErrLog::writeAllHeapInfo(CollHeap *failedHeap)
{
  fprintf(fp, "Heap Information:\n");

  if (failedHeap != NULL)
    writeHeapInfo(failedHeap);

  CmpContext *context = CmpCommon::context();

  // Write the context heap information
  CollHeap *contextHeap = context->heap();
  if (contextHeap != NULL && contextHeap != failedHeap)
    writeHeapInfo(contextHeap);

  // Write the statement heap information
  CollHeap *statementHeap = context->statementHeap();  
  if (statementHeap != NULL && statementHeap != failedHeap)
    writeHeapInfo(statementHeap);
}

// CmpErrLog::writeCQDInfo() writes any modified CQDs to the
// log file.
void CmpErrLog::writeCQDInfo()
{
  fprintf(fp, "Control Query Defaults:\n");

  NADefaults &defs = CmpCommon::context()->schemaDB_->getDefaults();

  for (UInt32 i = 0; i < defs.numDefaultAttributes(); i++)
  {
    const char *attrName = defs.lookupAttrName(i);
    DefaultConstants attrEnum = NADefaults::lookupAttrName(attrName);
    const char *val;
    switch(defs.getProvenance(attrEnum))
    {
      case NADefaults::DERIVED:
      case NADefaults::READ_FROM_SQL_TABLE:
      case NADefaults::COMPUTED:
      case NADefaults::SET_BY_CQD:
        val=defs.getValue(i);
        fprintf(fp, "CONTROL QUERY DEFAULT %s '%s';\n", attrName, val);
        break;
      case NADefaults::UNINITIALIZED:
      case NADefaults::INIT_DEFAULT_DEFAULTS:
      case NADefaults::IMMUTABLE:
      default:
        // Don't write this CQD to the output file.
        break;
    }
  }

  fputc('\n', fp);
}

// CmpErrLog::writeQueryInfo() writes information about the current
// query to the log file.
void CmpErrLog::writeQueryInfo()
{
  CmpContext *context = CmpCommon::context();
  CmpStatement *statement = context->statement();

  if (statement == NULL)
  {
    fprintf(fp, "There is no current statement\n\n");
    return;
  }

  fprintf(fp, "Statement Information:\n");

  // The task count is logged when the compiler has 
  // reached at least the optimization stage.  The
  // task_count_fmt points to the format string that is
  // used.
  const char *task_count_fmt = "Task Count     : %d\n"; 

  QueryAnalysis *qa = QueryAnalysis::Instance();
  if (qa != NULL)
  {
    fprintf(fp, "Compiler Phase : ");
    switch (qa->getCompilerPhase())
    {
      case QueryAnalysis::PRE_BINDER:
        fprintf(fp, "PRE_BINDER\n");
        break;
      case QueryAnalysis::BINDER:
        fprintf(fp, "BINDER\n");
        break;
      case QueryAnalysis::NORMALIZER:
        fprintf(fp, "NORMALIZER\n");
        break;
      case QueryAnalysis::ANALYZER:
        fprintf(fp, "ANALYZER\n");
        break;
      case QueryAnalysis::OPTIMIZER:
        fprintf(fp, "OPTIMIZER\n");
        fprintf(fp, task_count_fmt, CURRSTMT_OPTDEFAULTS->getTaskCount());
        break;
      case QueryAnalysis::PRECODE_GENERATOR:
        fprintf(fp, "PRECODE_GENERATOR\n");
        fprintf(fp, task_count_fmt, CURRSTMT_OPTDEFAULTS->getTaskCount());
        break;
      case QueryAnalysis::GENERATOR:
        fprintf(fp, "GENERATOR\n");
        fprintf(fp, task_count_fmt, CURRSTMT_OPTDEFAULTS->getTaskCount());
        break;
      case QueryAnalysis::POST_GENERATOR:
        fprintf(fp, "POST_GENERATOR\n");
        break;
      default:
        fprintf(fp, "UNKNOWN: CompilerPhaseEnum=%d\n",
                qa->getCompilerPhase());
        break;
    }
  }
  fputc('\n', fp);
 
  if (statement->userSqlText() != NULL)
    fprintf(fp, "SQL Text:\n%s\n\n", statement->userSqlText());
}

// CmpErrLog::writeQueryInfo() writes the stack trace of the 
// compiler to the log file.
void CmpErrLog::writeStackTrace()
{
  fprintf(fp, "Process Stack Trace:\n");


  // This is a quick and dirty implementation for Linux. It is easy to
  // get the program counters for the stack trace, but is difficult to
  // look up the function name, line, and file number based off of the
  // program counter. For simplicity, this code just calls addr2line to
  // look up the information. This could be changed in the future if an
  // easy to use API becomes available.
  void *bt[20];
  size_t size = backtrace(bt, 20);

  pid_t myPID = getpid();

  // Write each level of the stack except for the top frame and the
  // bottom two frames, which aren't important here.
  Int32 i = 1;
  while (i < size - 2)
  {
    char buffer[128];  // Used for command-line + addr2line output.

    sprintf(buffer, "/usr/bin/addr2line -e /proc/%d/exe -f -C ", myPID);

    Int32 j;

    // Run addr2line on 5 addresses at a time.
    for (j = i; j < i+5 && j < size-2; j++)
    {
      char addrBuf[12];
      sprintf(addrBuf, " %p", bt[j]);
      strcat(buffer, addrBuf);
    }

    FILE *cmdFP = popen(buffer, "r");
    if (cmdFP == NULL)
    {
      fprintf(fp, "Error %d while popen() of %s\n", errno, buffer);
      break;
    }
    else
    {
      for (j = i; j < i+5 && j < size-2; j++)
      {
        // Read from the addr2line output
        fgets(buffer, sizeof(buffer), cmdFP);

        // Replace newline with null character
        size_t len = strlen(buffer);
        if (buffer[len-1] == '\n')
          buffer[len-1] = '\0';

        fprintf(fp, "%p: %s()\n", bt[j], buffer); 
        fgets(buffer, sizeof(buffer), cmdFP);
        fprintf(fp, "            %s", buffer); 
      }
      fclose(cmdFP);
    }
    i = j;
  }
  fputc('\n', fp);
}

// CmpErrLog::CmpErrLogCallback() is called by the NAMemory code when
// a memory allocation failure occurs. This function just logs the
// failure.
void CmpErrLog::CmpErrLogCallback(NAHeap *heap, size_t userSize)
{
  CmpErrLog("Memory allocation failure", heap, userSize);
}

// CmpErrLog::CmpErrLog() constructs a CmpErrLog object and 
// contains all of the calls that are necessary for logging
// information about the current state of the compiler.
CmpErrLog::CmpErrLog(const char *failureTxt, CollHeap *failedHeap, size_t size)
  : fp(0)
{
  // If memory is allocated, then free it to give some extra memory
  // for logging.
  if (memPtr != NULL)
  {
    free(memPtr);
    memPtr = NULL;
  }

  // Create and lock the log file.
  openLogFile();

  // If the log file was opened successfully, then write the
  // information to the log file.
  if (fp != NULL)
  {
    writeHeader(failureTxt, failedHeap, size);
    writeMemoryStats();
    writeAllHeapInfo(failedHeap);
    writeCQDInfo();
    writeQueryInfo();
    writeStackTrace();
    closeLogFile();
  }
}

// CmpErrLog::~CmpErrLog() is the destructor and frees any resources
// used by the CmpErrLog class.
CmpErrLog::~CmpErrLog()
{
  // Call closeLogFile() to make sure the log file is closed.  This
  // could potentially happen if an exception is thrown.
  closeLogFile();
}
