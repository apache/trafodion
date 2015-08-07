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
#ifndef __MEASURE_H_
#define __MEASURE_H_


#undef MEASURE

#ifdef MEASURE

#include <sys/times.h>
#include <stdio.h>

class LogAgent{
public:
  LogAgent();
  ~LogAgent();
};

class Timer{
public:
  void timerOn();
  void timerOff();
  void timerLog(const char *msg, const char *file, Int32 line);

  static void openLog();
  static void closeLog();
  static void flushLog();
private:
  clock_t startClock;
  clock_t endClock;
  tms startTms;
  tms endTms;
  static FILE *logFile;
};

#define LOG_AGENT \
LogAgent logAgent;

#define LOG_FLUSH \
Timer::flushLog();

#define TIMER_ON(timerId) \
Timer timerId; \
timerId.timerOn();

#define TIMER_OFF(timerId, msg) \
timerId.timerOff(); \
timerId.timerLog(msg, __FILE__, __LINE__);

#else // #ifdef MEASURE

#define LOG_AGENT
#define TIMER_ON(timerId)
#define TIMER_OFF(timerId, msg)
#define LOG_FLUSH

#endif // #ifdef MEASURE

#endif // __MEASURE_H










