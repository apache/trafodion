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
#include "Measure.h"
#ifdef MEASURE
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

FILE * Timer::logFile = 0;

LogAgent::LogAgent()
{
  Timer::openLog();
}

LogAgent::~LogAgent()
{
  Timer::closeLog();
}

void Timer::timerOn() { startClock = times(&startTms); }

void Timer::timerOff() { endClock = times(&endTms); }

void Timer::timerLog(const char *msg, const char *file, Int32 line)
{
  fprintf(logFile, "---------------------------------\n");
  fprintf(logFile, "%s %s %d\n", msg, file, line);
  fprintf(logFile, "Elapsed Time: %fs\n", (endClock - startClock)/1000000.0);
  fprintf(logFile, "User    Time: %fs\n", (endTms.tms_utime - startTms.tms_utime)/1000000.0);
  fprintf(logFile, "System  Time: %fs\n", (endTms.tms_stime - startTms.tms_stime)/1000000.0);
}

void Timer::openLog() {
  pid_t pid = getpid();
  char fileName[256];
  char *logFileEnv = 0;
  if((logFileEnv = getenv("MEASURE_LOG_FILE")) != NULL){
    sprintf(fileName, "%s.%d", logFileEnv, pid);
    logFile = fopen(fileName, "w+t");
    if(logFile == 0){
      logFile = stderr;
      cerr << "Can not open " << fileName << endl;
      cerr << "Timing results logging to standard error" << endl;
    }
    else
      cerr << "Timing results logging to " << fileName << endl;
  }
  else{
    logFile = stderr;
    cerr << "Timing results logging to standard error" << endl;
  }
}

void Timer::closeLog() {
  if(logFile)
    fclose(logFile);
}

void Timer::flushLog() {
  if(logFile)
    fflush(logFile);
}

#endif





