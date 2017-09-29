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
#include "CompilationStats.h"

// -----------------------------------------------------------------------
// Methods for class CompilationStats
// -----------------------------------------------------------------------
/************************************************************************
constructor CompilationStats

************************************************************************/
CompilationStats::CompilationStats()
  :  compileStartTime_(0),
     compileEndTime_(0),
     mdCacheHitsBegin_(0),
     mdCacheLookupsBegin_(0),
     hCacheHitsBegin_(0),
     hCacheLookupsBegin_(0),
     optContexts_(0),
     isRecompile_(FALSE)
{      
  memset(&qCacheStatsBegin_, 0, sizeof(qCacheStatsBegin_));
  compileInfo_[0] = '\0';
}

void CompilationStats::takeSnapshotOfCounters()
{      
  // metadata cache counters start point
  mdCacheHitsBegin_    = ActiveSchemaDB()->getNATableDB()->hits(); 
  mdCacheLookupsBegin_ = ActiveSchemaDB()->getNATableDB()->lookups();

  // histogram cache counters start point
  if (CURRCONTEXT_HISTCACHE)
  {
    hCacheHitsBegin_    = CURRCONTEXT_HISTCACHE->hits();
    hCacheLookupsBegin_ = CURRCONTEXT_HISTCACHE->lookups();
  }
    
  // get a snapshot of the QueryCacheStats prior to the compilation
  // this will be used to determine the qCacheStatus_ at the end  
  CURRENTQCACHE->getCompilationCacheStats(qCacheStatsBegin_);

  compileInfo_[0] = '\0';
}

/************************************************************************
method CompilationStats::getCompilerId

 pass in a buffer and get the compiler id back

************************************************************************/
void
CompilationStats::getCompilerId(char *cmpId, int len)
{
  CmpProcess p;
  p.getCompilerId(cmpId, len);
}
/************************************************************************
method CompilationStats::enterCmpPhase

 mark the begining of a compilation phase

************************************************************************/
void 
CompilationStats::enterCmpPhase(CompilationPhase phase)
{
  if (!isValidPhase(phase)) return;

  // always initialize it to zero
  cpuMonitor_[phase].init(0);
  cpuMonitor_[phase].enter();
  //
  // mark the start of the compilation
  if( CMP_PHASE_ALL == phase )
  {
    compileStartTime_ = getCurrentTimestamp();
  }
}
/************************************************************************
method CompilationStats::exitCmpPhase

 mark the end of a compilation phase

************************************************************************/
void 
CompilationStats::exitCmpPhase(CompilationPhase phase)
{
  if (!(isValidPhase(phase))) return;

  cpuMonitor_[phase].exit();
  //
  // mark the end of the compilation
  if( CMP_PHASE_ALL == phase )
  {
    compileEndTime_ = getCurrentTimestamp();
  }
}
/************************************************************************
method CompilationStats::cmpPhaseLength

 get the cpu path length for the given compilation phase

************************************************************************/
Lng32 
CompilationStats::cmpPhaseLength(CompilationPhase phase)
{
  if (!isValidPhase(phase)) return 0;

  // for now, combine the semantic query optimization phase counter
  // into the normalizer counter
  if( CMP_PHASE_NORMALIZER == phase )
  {
    return 
      cpuMonitor_[CMP_PHASE_SEMANTIC_QUERY_OPTIMIZATION].timer() + 
      cpuMonitor_[CMP_PHASE_NORMALIZER].timer();
  }
  else
  {
    return cpuMonitor_[phase].timer();
  }
}
/************************************************************************
method CompilationStats::metadataCacheHits

The current metadata cache hit counter is calculated by
 subtracting the total from the place where it was first marked at
 the start of the query

************************************************************************/
ULng32 
CompilationStats::metadataCacheHits()
{ 
  return ActiveSchemaDB()->getNATableDB()->hits() - mdCacheHitsBegin_; 
}
/************************************************************************
method CompilationStats::metadataCacheLookups

The current metadata cache lookup counter is calculated by
 subtracting the total from the place where it was first marked at
 the start of the query

************************************************************************/
ULng32 
CompilationStats::metadataCacheLookups()
{ 
  return ActiveSchemaDB()->getNATableDB()->lookups() - 
    mdCacheLookupsBegin_; 
}
/************************************************************************
method CompilationStats::QueryCacheState

 get the QueryCacheState for this compilation

************************************************************************/
Int32
CompilationStats::getQueryCacheState()
{
  // so we can assert later that it found a valid status  
  QCacheState state = QCSTATE_UNKNOWN; 

  QCacheStats qCacheStatsEnd;
  CURRENTQCACHE->getCompilationCacheStats(qCacheStatsEnd);
  //
  // check if it’s cacheable or not
  if (qCacheStatsEnd.nCacheableP+qCacheStatsEnd.nCacheableB > 
      qCacheStatsBegin_.nCacheableP+
      qCacheStatsBegin_.nCacheableB)
  {   
    // it’s cacheable

    // check if it’s template or text cache hit or miss
    if (qCacheStatsEnd.nCacheHitsPP > qCacheStatsBegin_.nCacheHitsPP)
    {
      // it’s a text cache hit
      state = QCSTATE_TEXT;
    }
    else 
    { 
      // it’s a text cache miss
      if ( qCacheStatsEnd.nCacheHitsP+qCacheStatsEnd.nCacheHitsB > 
           qCacheStatsBegin_.nCacheHitsP+
           qCacheStatsBegin_.nCacheHitsB )
      {
        //
        // it’s a template cache hit
        state = QCSTATE_TEMPLATE;
      }
      else 
      { 
        // it’s a template cache miss
        state = QCSTATE_MISS_CACHEABLE;
      }
    }
  }
  else 
  { 
    //
    // it’s not cacheable
    state = QCSTATE_MISS_NONCACHEABLE;
  }

  // should have a valid qcache state now
  if (!isValidQCacheState(state)) return QCSTATE_UNKNOWN; 

  return state;
}
/************************************************************************
method CompilationStats::histogramCacheHits

The current histogram cache hit counter is calculated by
 subtracting the total from the place where it was first marked at
 the start of the query

************************************************************************/
ULng32 
CompilationStats::histogramCacheHits()
{ 
  if (CURRCONTEXT_HISTCACHE)
    return CURRCONTEXT_HISTCACHE->hits() - hCacheHitsBegin_; 
  else
    return 0;
}
/************************************************************************
method CompilationStats::histogramCacheLookups

The current histogram cache lookup counter is calculated by
 subtracting the total from the place where it was first marked at
 the start of the query

************************************************************************/
ULng32 
CompilationStats::histogramCacheLookups()
{ 
  if (CURRCONTEXT_HISTCACHE)
    return CURRCONTEXT_HISTCACHE->lookups() - hCacheLookupsBegin_; 
  else
    return 0;
}
/************************************************************************
method CompilationStats::getCompileInfo

 pass in a buffer and get the compile info back

************************************************************************/
void
CompilationStats::getCompileInfo(char *cmpInfo)
{
  cmpInfo[0] = '\0'; //TBD
}

Int32 CompilationStats::getCompileInfoLen() 
{ return str_len(compileInfo_); }

void CompilationStats::incrOptContexts()
{ optContexts_++; }

ULng32 CompilationStats::optimizationContexts()
{ return optContexts_; } 

void CompilationStats::setIsRecompile()
{ isRecompile_ = TRUE; }

NABoolean CompilationStats::isRecompile() 
{ return isRecompile_; }  

Int64 CompilationStats::compileStartTime() 
{ return compileStartTime_; }

Int64 CompilationStats::compileEndTime() 
{ return compileEndTime_; }

void CompilationStats::dumpToFile()
{
  FILE *myf = fopen ("CmpStatsDump", "ac");
  fprintf(myf, "---========= BEGIN NEW STMT STATS =========---\n");

  char beginTime[100];
  char endTime[100];
  getTimestampAsBuffer(compileStartTime_, beginTime);
  getTimestampAsBuffer(compileEndTime_, endTime);

  fprintf(myf, "  Compilation Start Time: %s\n", beginTime);
  fprintf(myf, "  Compilation End Time: %s\n", endTime); 

  char cmpId[COMPILER_ID_LEN];
  getCompilerId(cmpId, COMPILER_ID_LEN);
  fprintf(myf, "  Compiler Id: %s\n", cmpId);
  fprintf(myf, "  All Cmp Phases Execution length: \n");
  fprintf(myf, "    Binder: %d\n", cmpPhaseLength(CompilationStats::CMP_PHASE_BINDER));
  fprintf(myf, "    normalizer: %d\n", cmpPhaseLength(CompilationStats::CMP_PHASE_NORMALIZER));
  fprintf(myf, "    Analyzer: %d\n", cmpPhaseLength(CompilationStats::CMP_PHASE_ANALYZER));
  fprintf(myf, "    Optimizer: %d\n", cmpPhaseLength(CompilationStats::CMP_PHASE_OPTIMIZER));
  fprintf(myf, "    Generator: %d\n", cmpPhaseLength(CompilationStats::CMP_PHASE_GENERATOR));
  fprintf(myf, "    All Cmp Length: %d\n", cmpPhaseLength(CompilationStats::CMP_PHASE_ALL));
  fprintf(myf, "  Metadata Cache Hits: %d\n", metadataCacheHits());
  fprintf(myf, "  Metadata Look Ups: %d\n", metadataCacheLookups());
  fprintf(myf, "  Query Cache Hits: %d\n", getQueryCacheState());
  fprintf(myf, "  Histogram Cache Hits: %d\n", histogramCacheHits());
  fprintf(myf, "  Histogram Cache Lookups: %d\n", histogramCacheLookups());
  fprintf(myf, "  Stmt Heap current Size: %ld\n", (Int64) stmtHeapCurrentSize());
  fprintf(myf, "  Context Heap current size: %ld\n", (Int64) cxtHeapCurrentSize());
  fprintf(myf, "  Compiler Optimization Tasks: %d\n", optimizationTasks());
  fprintf(myf, "  Compiler Optimization Contexts: %d\n", optimizationContexts());

  fprintf(myf, "  isRecompile: ");
  if (isRecompile())
      fprintf(myf, "Yes\n");
  else
      fprintf(myf, "No\n");

  fprintf(myf, "---========= END NEW STMT STATS =========---\n");
  fclose (myf);
}
