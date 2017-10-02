// **********************************************************************
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
// **********************************************************************
#ifndef COMPILATIONSTATS_H
#define COMPILATIONSTATS_H

#include "CmpProcess.h"
#include "QCache.h"
#include "SchemaDB.h"
#include "NATable.h"
#include "opt.h"
/************************************************************************
class CompilationStats

Singleton class to collect information about this Query compilation 
and to be logged into the repository after the compilation is complete.

************************************************************************/
class CompilationStats
{
public:
  CompilationStats();

  ~CompilationStats() {};

  void takeSnapshotOfCounters();

  // Used to count the CPU path length for each compilation phase
  enum CompilationPhase
  {
    CMP_PHASE_ALL = 0,
    CMP_PHASE_PARSER,
    CMP_PHASE_BINDER,
    CMP_PHASE_TRANSFORMER,
    CMP_PHASE_NORMALIZER,
    CMP_PHASE_SEMANTIC_QUERY_OPTIMIZATION,
    CMP_PHASE_ANALYZER,
    CMP_PHASE_OPTIMIZER,
    CMP_PHASE_PRECODE_GENERATOR,
    CMP_PHASE_GENERATOR,
    CMP_NUM_PHASES
  };

  Int64 compileStartTime();
  Int64 compileEndTime();

  // pass in a buffer of size COMPILER_ID_LEN
  void getCompilerId(char *cmpId, int len); 
  //
  // metadata cache counters
  ULng32 metadataCacheHits();
  ULng32 metadataCacheLookups();
  //
  // See QCacheState enum
  Int32 getQueryCacheState();
  //
  // histogram counters
  ULng32 histogramCacheHits(); 
  ULng32 histogramCacheLookups();
  //
  // statement heap
  static inline size_t stmtHeapCurrentSize()
    { return CmpCommon::statementHeap()->getAllocSize(); }
  //
  // context heap
  static inline size_t cxtHeapCurrentSize()
    { return CmpCommon::contextHeap()->getAllocSize(); }
  //
  // optimization tasks
  static inline Lng32 optimizationTasks()
    { return CURRSTMT_OPTDEFAULTS->getTaskCount(); }
  //
  // optimization contexts
  void incrOptContexts();

  ULng32 optimizationContexts();

  // is recompile
  void setIsRecompile();

  NABoolean isRecompile();

  // pass in a buffer of size CompilationStats::MAX_COMPILER_INFO_LEN
  void getCompileInfo(char *cmpInfo); 
  Int32 getCompileInfoLen();
    
  void enterCmpPhase(CompilationPhase phase);
  void exitCmpPhase(CompilationPhase phase);  
  Lng32 cmpPhaseLength(CompilationPhase phase);  
  //
  // maximum # of characters for compiler info
  enum { MAX_COMPILER_INFO_LEN = 4096 };
  
  enum QCacheState
  {
    QCSTATE_UNKNOWN = -1,
    QCSTATE_TEXT = 0,
    QCSTATE_TEMPLATE,
    QCSTATE_MISS_NONCACHEABLE,
    QCSTATE_MISS_CACHEABLE,
    QCSTATE_NUM_STATES
  };  

  //  the valid phases only go up to one before CMP_NUM_PHASES
  inline
  NABoolean isValidPhase(CompilationPhase phase) 
  {
    return (phase >= CMP_PHASE_ALL && phase < CMP_NUM_PHASES);
  }

  // valid QCacheState up to one before QCSTATE_NUM_STATUSES
  inline
  NABoolean isValidQCacheState(QCacheState state)
  {
    return (state >= QCSTATE_TEXT && state < QCSTATE_NUM_STATES);
  }

  void dumpToFile();

private:
  // timestamp for start/end time of this compilation
  Int64 compileStartTime_;
  Int64 compileEndTime_;
  //
  //  Task Monitor used for CPU path length for each phase
  TaskMonitor cpuMonitor_[CMP_NUM_PHASES];
  //
  // metadata cache counters
  ULng32 mdCacheHitsBegin_;
  ULng32 mdCacheLookupsBegin_;
  //
  // histogram cache counters
  ULng32 hCacheHitsBegin_;
  ULng32 hCacheLookupsBegin_;
  //
  // optimization tasks/contexts counters
  ULng32 optContexts_;
  //
  // is this query a recompile
  NABoolean isRecompile_;
  //
  // additional compiler information
  char compileInfo_[MAX_COMPILER_INFO_LEN];
  //
  // Use snapshot of QueryCacheStats to compare
  // beginning/end of compilation
  QCacheStats qCacheStatsBegin_;
};

#endif // COMPILATIONSTATS_H

