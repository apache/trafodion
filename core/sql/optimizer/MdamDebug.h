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
#ifndef _MDAM_DEBUG_H_
#define _MDAM_DEBUG_H_

#include "Platform.h"

#ifndef NDEBUG // debug build
#define MDAM_TRACE
#define MDAM_DEBUG
//#define MDAM_MONITOR  //uncomment to enable timing
#endif

#ifdef MDAM_MONITOR
#undef MDAM_DEBUG
#endif

// when do timing, MDAM_DEBUG need be defined and MDAM_TRACE need be undefined

#ifdef MDAM_TRACE
#include <stdarg.h>
#include <stdio.h>
#include "NABoolean.h"

class ScanOptimizer;
class Cost;
class ValueIdSet;
class TaskMonitor;
class FileScanOptimizer;
class SimpleCostVector;
enum MdamTraceLevel {
  MDAM_TRACE_LEVEL_NONE,
  MTL1,
  MTL2,
  MTL3,
  MDAM_TRACE_LEVEL_ALL = 100
};

class MdamTrace{
private:
  static THREAD_P FILE *outputFile_;
  static THREAD_P NABoolean doPrint_; 
  static THREAD_P NABoolean initialized_;
  static THREAD_P const char* msgHeader_;
  static THREAD_P const char* overrideHeader_;
  static THREAD_P const char* indent_;
  static THREAD_P Int32 hStdOut_;
  static THREAD_P NABoolean okToRedirectStdOut_;
  static THREAD_P FILE* console_;
  static THREAD_P enum MdamTraceLevel level_;
  static void mayInit();
public:
  static const char* getHeader();
  static void setHeader(const char *override);
  static void redirectStdOut();
  static void restoreStdOut();
  static void print(const char *formatString, ...);
  static void printCostObject(ScanOptimizer *opt, 
			      const Cost *cost, 
			      const char *msg);
  static void printFileScanInfo(ScanOptimizer *opt, 
				const ValueIdSet & partKeyPreds);
  static void printTaskMonitor(TaskMonitor &monitor, 
			       const char *msg);
  static void printBasicCost(FileScanOptimizer *opt, 
			     SimpleCostVector &prefixFR, 
			     SimpleCostVector &prefixLR, 
			     const char *msg);
  static MdamTraceLevel level();
  static void setLevel(enum MdamTraceLevel l);
};

#define SET_MDAM_TRACE_HEADER(m) MdamTrace::setHeader(m)
#define SET_MDAM_TRACE_LEVEL(l) MdamTrace::setLevel(l)

#else

#define SET_MDAM_TRACE_HEADER(m)
#define SET_MDAM_TRACE_LEVEL(l)

#endif

#ifdef MDAM_DEBUG

#define MDAM_DEBUG0(l, m)          if(l <= MdamTrace::level()) \
                                     MdamTrace::print((m))
#define MDAM_DEBUG1(l,m,a)         if(l <= MdamTrace::level()) \
                                     MdamTrace::print((m),(a))
#define MDAM_DEBUG2(l,m,a,b)       if(l <= MdamTrace::level()) \
                                     MdamTrace::print((m),(a),(b))
#define MDAM_DEBUG3(l,m,a,b,c)     if(l <= MdamTrace::level()) \
                                     MdamTrace::print((m),(a),(b),(c))
#define MDAM_DEBUG4(l,m,a,b,c,d)   if(l <= MdamTrace::level()) \
                                     MdamTrace::print((m),(a),(b),(c),(d))
#define MDAM_DEBUG5(l,m,a,b,c,d,e) if(l <= MdamTrace::level()) \
                                     MdamTrace::print((m),(a),(b),(c),(d),(e))
#define MDAM_DEBUGX(l,f)           if(l <= MdamTrace::level()) \
                                     { \
                                       MdamTrace::redirectStdOut(); \
                                       (f); \
                                       fflush(stdout); \
                                       MdamTrace::restoreStdOut(); \
                                     }
#else
#define MDAM_DEBUG0(l,m)
#define MDAM_DEBUG1(l,m,a)
#define MDAM_DEBUG2(l,m,a,b)
#define MDAM_DEBUG3(l,m,a,b,c)
#define MDAM_DEBUG4(l,m,a,b,c,d)
#define MDAM_DEBUG5(l,m,a,b,c,d,e)
#define MDAM_DEBUGX(l,f)
#endif // MDAM_DEBUG

#ifdef MDAM_MONITOR

#define DECLARE_MDAM_MONITOR(mon) TaskMonitor mon
#define RESET_MDAM_MONITOR(mon)   mon.init(0)
#define ENTER_MDAM_MONITOR(mon)   mon.enter()
#define EXIT_MDAM_MONITOR(mon)    mon.exit()
#define PRINT_MDAM_MONITOR(mon, msg)   MdamTrace::printTaskMonitor(mon, msg)

#else

#define DECLARE_MDAM_MONITOR(mon)
#define RESET_MDAM_MONITOR(mon)  
#define ENTER_MDAM_MONITOR(mon)  
#define EXIT_MDAM_MONITOR(mon)
#define PRINT_MDAM_MONITOR(mon, msg)

#endif // MDAM_MONITOR

#endif // _MDAM_DEBUG_H_
