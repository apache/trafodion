// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
// **********************************************************************
#ifndef COMPILERTRACKING_H
#define COMPILERTRACKING_H

#include "NAString.h"
#include "CmpCommon.h"
#include "SchemaDB.h"
#include "opt.h"
#include "NATable.h"
#include <time.h>
#include <fstream>
#include "QCache.h"
#include "CmpCliCalls.h"
#include "CmpProcess.h"

//
// the private user table where the data can be loggeed
#define COMPILER_TRACKING_TABLE_NAME_PRIVATE "STATE_TRACKING_COMPILERS_TABLE_PRIVATE"
/************************************************************************
class CompilerTrackingInfo

Singleton class to collect information about this compiler 
and to be logged into a file or a private user table on some interval 
(every N minutes)

************************************************************************/
class CompilerTrackingInfo
{
public:

  // get the singleton instance
  //static CompilerTrackingInfo * getInstance();
  CompilerTrackingInfo(CollHeap *outHeap = CmpCommon::contextHeap());
  //
  // initialize the singleton
  //static void initGlobalInstance();
  //
  // log all of the data members of this object
  void logCompilerStatusOnInterval(Int32 intervalLengthMins);
  // 
  // the cpu time for the longest compilation is updated
  // if the parameter is longer than the current longest
  void updateLongestCompile( Lng32 c );
  //
  // the largest statement heap size so far 
  // update this right before deleting the CmpStatement
  void updateStmtIntervalWaterMark();
  //
  // the largest system heap size so far
  // update this right before cleanup (in CmpMain.cpp)
  void updateSystemHeapWtrMark();
  //
  // the largest qcache heap size so far
  // update this right before deleting the qcache
  void updateQCacheIntervalWaterMark();
  //
  // the number of successfully compiled queries
  void incrementQueriesCompiled( NABoolean success );  
  //
  // compiled successfully but with warnings 2053 or 2078
  void incrementCaughtExceptions()
    { caughtExceptionCount_++; }
  //
  // when the QueryCache is being deleted, clear the
  // markers we used to determine difference during 
  // the interval.
  void clearQCacheCounters();
  //
  // maximum # of characters for compiler info
  enum { MAX_COMPILER_INFO_LEN = 4096 };

  // start the new interval (at the current time, clock())
  // if the previous interval tracking was disabled
  void resetIntervalIfNeeded();
  
// methods used internally within the class...
protected:
  //
  // check to see if an interval has expired
  NABoolean intervalExpired(Int32 intervalLengthMins);
  //
  // start the new interval (at the current time, clock())
  void resetInterval();

  // the duration of the interval in minutes
  Int32 currentIntervalDuration(Int64 endTime);
  //
  // the cpu time for the interval
  Lng32 cpuPathLength();
  //
  // getters
  inline Int64 beginIntervalTime() 
    { return beginIntervalTime_ ; }

  // return the beginning of the interval in unix epoch
  inline Int64 beginIntervalTimeUEpoch()
    { return beginIntervalTimeUEpoch_ ; }

  inline Int64 endIntervalTime()
    { return endIntervalTime_; }

  inline Lng32 beginIntervalClock()
    { return beginIntervalClock_; }
  //
  // the compiler age in minutes
  inline Lng32 compilerAge()
  {
    Int64 seconds = (processInfo_->getProcessDuration() / 1000000);
    Int64 minutes = (seconds / 60);
    return int64ToInt32(minutes); 
  }
  //
  // statement heap
  inline size_t stmtHeapCurrentSize()
    { return CmpCommon::statementHeap()->getAllocSize(); }

  inline size_t stmtHeapIntervalWaterMark();  
  //
  // context heap
  inline size_t cxtHeapCurrentSize()
    { return CmpCommon::contextHeap()->getAllocSize(); }

  inline size_t cxtHeapIntervalWaterMark()
    { return CmpCommon::contextHeap()->getIntervalWaterMark(); }
  //
  // metadata cache
  inline ULng32 metaDataCacheCurrentSize()
    { return ActiveSchemaDB()->getNATableDB()->currentCacheSize(); }

  inline ULng32 metaDataCacheIntervalWaterMark()
    { return ActiveSchemaDB()->getNATableDB()->intervalWaterMark();}

  inline ULng32 metaDataCacheHits();
  inline ULng32 metaDataCacheLookups();
  void resetMetadataCacheCounters();
  //
  // query cache
  inline ULng32 qCacheCurrentSize();
  inline ULng32 qCacheIntervalWaterMark();  
  inline ULng32 qCacheHits();
  inline ULng32 qCacheLookups();
  inline ULng32 qCacheRecompiles();
  void resetQueryCacheCounters();
  // 
  // histogram cache
  inline ULng32 hCacheCurrentSize()
  {   
      CMPASSERT(NULL!=CURRCONTEXT_HISTCACHE); 
      return CURRCONTEXT_HISTCACHE->getHeap()->getAllocSize(); 
  }

  inline ULng32 hCacheIntervalWaterMark()
  { 
      CMPASSERT(NULL!=CURRCONTEXT_HISTCACHE); 
      return CURRCONTEXT_HISTCACHE->getHeap()->getIntervalWaterMark(); 
  }

  inline ULng32 hCacheHits();
  inline ULng32 hCacheLookups();
  void resetHistogramCacheCounters();

  inline Lng32 systemHeapIntervalWaterMark()
    { return systemHeapWaterMark_; }

  inline Lng32 longestCompile()
    { return longestCompileClock_; } 

  inline ULng32 successfulQueryCount() 
    { return successfulQueryCount_; }

  inline ULng32 failedQueryCount()
    { return failedQueryCount_; }

  inline ULng32 caughtExceptionCount()
    { return caughtExceptionCount_; }

  inline ULng32 sessionCount();

  inline const char *compilerInfo()
    { return compilerInfo_; }  
  //
  // for printing to file
  enum { CACHE_HEAP_HEADER_LEN = 18, 
         CACHE_HEAP_VALUE_LEN = 16 };
  //
  // just do the printing
  void printToFile();

  // log to a user table
  void logIntervalInPrivateTable();

  // log to a log4cpp appender
  void logIntervalInLog4Cpp();

private:
  ~CompilerTrackingInfo();

  // The process info contains information such as cpu num, pin, etc
  CmpProcess *processInfo_;
  //
  // timestamp for when this interval began
  Int64 beginIntervalTime_;
  //
  // timestamp for when this interval began in unix epoch
  Int64 beginIntervalTimeUEpoch_;
  //
  // timestamp for when this interval ended
  Int64 endIntervalTime_;
  //
  //  cpu path length for this interval
  Lng32 beginIntervalClock_;
  //
  // the most memory used in a CmpStatement so far
  size_t largestStmtIntervalWaterMark_;
  //
  // the most memory used by system heap so far
  Lng32 systemHeapWaterMark_;
  //
  // cpu path for the longest compile so far
  Lng32 longestCompileClock_;
  //
  // metadata cache counters
  ULng32 mdCacheHits_;
  ULng32 mdCacheLookups_;
  //
  // query plan cache stats
  QCacheStats currentQCacheStats_;
  ULng32 largestQCacheIntervalWaterMark_;
  ULng32 qCacheHits_;
  ULng32 qCacheLookups_;
  ULng32 qCacheRecompiles_;
  //
  // histogram cache counters
  ULng32 hCacheHits_;
  ULng32 hCacheLookups_;
  //
  // the number of queries compiled during this interval
  ULng32 successfulQueryCount_;
  ULng32 failedQueryCount_;
  //
  // the number of exceptions caught (2053 and 2078)
  ULng32 caughtExceptionCount_;
  //
  // the number of sessions
  ULng32 sessionCount_;
  //
  // the length of the last interval (set by CQD)
  ULng32 prevInterval_;
  //
  // additional compiler information
  char compilerInfo_[MAX_COMPILER_INFO_LEN];
  //
  // the heap created on
  CollHeap *heap_;
};
#endif
