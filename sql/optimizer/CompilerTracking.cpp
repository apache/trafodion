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
#include "CompilerTracking.h"
#ifdef _MSC_VER
#undef _MSC_VER
#endif
#include "common/sql.tracking_compilers.pb.h"
#include "common/evl_sqlog_eventnum.h"
#include "wrapper/amqpwrapper.h"
/************************************************************************
 getCompilerTrackingLogFilename

 helper to get the filename from defaults

************************************************************************/
static
const char* 
getCompilerTrackingLogFilename()
{  
  const char * fname = ActiveSchemaDB()->getDefaults().
		 getValue(COMPILER_TRACKING_LOGFILE);
    
  return (*fname && stricmp(fname,"NONE") != 0) ? fname : NULL;
}
// -----------------------------------------------------------------------
// Methods for class CompilerTrackingInfo
// -----------------------------------------------------------------------
/************************************************************************
constructor CompilerTrackingInfo

************************************************************************/
CompilerTrackingInfo::CompilerTrackingInfo(CollHeap*  heap)
 :  processInfo_(new (heap) CmpProcess()),    
    beginIntervalTime_(0), 
    beginIntervalTimeUEpoch_(0), 
    endIntervalTime_(0),
    beginIntervalClock_(0),
    largestStmtIntervalWaterMark_(0),
    longestCompileClock_(0),
    mdCacheHits_(0),
    mdCacheLookups_(0),
    largestQCacheIntervalWaterMark_(0),
    qCacheHits_(0),
    qCacheLookups_(0),
    qCacheRecompiles_(0),
    hCacheHits_(0),
    hCacheLookups_(0),
    successfulQueryCount_(0),
    failedQueryCount_(0),
    caughtExceptionCount_(0),
    sessionCount_(0),
    prevInterval_(0),
    AMQPConnectionEstablished_(FALSE),
    heap_(heap)
{
  compilerInfo_[0] = '\0';
}
/************************************************************************
destructor CompilerTrackingInfo

************************************************************************/
CompilerTrackingInfo::~CompilerTrackingInfo()
{
}

/************************************************************************
 * reset the class attribute for a new interval value
************************************************************************/
void
CompilerTrackingInfo::resetIntervalIfNeeded()
{
      Lng32 currentInterval =
           ActiveSchemaDB()->getDefaults().getAsLong(COMPILER_TRACKING_INTERVAL);

      if ((0 == prevInterval_) && (0 < currentInterval))
           resetInterval();           
}

/************************************************************************
method CompilerTrackingInfo::logCompilerStatusOnInterval

Dump the fields of this class out to a file (or to repository) if
the tracking compiler interval has expired

************************************************************************/
void
CompilerTrackingInfo::logCompilerStatusOnInterval(Int32 intervalLengthMins)
{

  if( intervalExpired(intervalLengthMins) )
  {
    //
    // this interval is now done/expired
    endIntervalTime_ = getCurrentTimestamp();
    //
    // get the latest cache stats once per interval
    if (!CURRENTQCACHE->getCompilationCacheStats(currentQCacheStats_))
    {
       // if query is disabled, clear the cache counters
       clearQCacheCounters();
    }

    //
    // log this interval
    if( NULL != getCompilerTrackingLogFilename() )
    {
      printToFile();      
    }

    //
    // log directly to a private table using dynamic SQL 
    // instead of using the Repository infrastructure to
    // populate repository table
    if (CmpCommon::getDefault(COMPILER_TRACKING_LOGTABLE) == DF_ON)
    {
       logIntervalInPrivateTable();    
    }

//
// This table doesn't exist on Windows, so don't log there
    // always log to the repository table
    Int32 rc = logIntervalInRepository();    
    if (rc)
    {
       // raise a warning that compiler process is unable to log 
       // its status and health information to the repository
       *CmpCommon::diags() << DgSqlCode(2242);
    }

    //
    // since the interval is expired, reset to begin tracking new interval
    resetInterval();        
  }
}
/************************************************************************
method CompilerTrackingInfo::intervalExpired

Check whether the defined interval for logging has expired and it's
OK to log CompilerTrackingInfo again.

************************************************************************/
inline
NABoolean
CompilerTrackingInfo::intervalExpired(Int32 intervalLengthMins)
{  
  return ( currentIntervalDuration(getCurrentTimestamp()) 
                                                >= intervalLengthMins );
}
/************************************************************************
method CompilerTrackingInfo::resetInterval

start the new interval (at the current time, clock())

************************************************************************/
inline
void CompilerTrackingInfo::resetInterval()
{  
  beginIntervalTime_  = getCurrentTimestamp();
  beginIntervalTimeUEpoch_  = getCurrentTimestampUEpoch();
  beginIntervalClock_ = clock();
  //
  // water marks for stmt and context heap back to 0
  CmpCommon::statementHeap()->resetIntervalWaterMark();
  CmpCommon::contextHeap()->resetIntervalWaterMark();
  //
  // metadata cache counters maintained on each interval
  resetMetadataCacheCounters();  
  //
  // query cache
  resetQueryCacheCounters();
  //
  // histogram cache counters reset on interval
  resetHistogramCacheCounters();  
  //
  // other counters
  largestStmtIntervalWaterMark_ = 0;
  systemHeapWaterMark_          = 0;
  longestCompileClock_          = 0;
  successfulQueryCount_         = 0;
  failedQueryCount_             = 0;
  caughtExceptionCount_         = 0;
  sessionCount_                 = 0;
}
/************************************************************************
method CompilerTrackingInfo::currentIntervalDuration

the current duration of this interval is the distance between
 the begin time and current time

************************************************************************/
inline
Int32 
CompilerTrackingInfo::currentIntervalDuration(Int64 endTime)
{  
  // return in minutes
  return (Int32)(((endTime - beginIntervalTime_) / 1000000)/60);    
}
/************************************************************************
method CompilerTrackingInfo::cpuPathLength

 the CPU path length of the interval

************************************************************************/
inline
Lng32
CompilerTrackingInfo::cpuPathLength()
{
  return (clock() - beginIntervalClock_);
}
/************************************************************************
method CompilerTrackingInfo::updateSystemHeapWtrMark

 update the system heap high water mark. Store the largest value for 
 the system heap size so far.  Update this right before cleanup in
 CmpMain.cpp

************************************************************************/
void
CompilerTrackingInfo::updateSystemHeapWtrMark()
{
  CMPASSERT(NULL!=processInfo_);

  Lng32 c = processInfo_->getCurrentSystemHeapSize();

  systemHeapWaterMark_ = ( c > systemHeapWaterMark_ ) ? 
                                  c : systemHeapWaterMark_;
}
/************************************************************************
method CompilerTrackingInfo::updateLongestCompile

 update the cpu path length of the longest compilation so far.  only
 update if the new duration is bigger than the longest one known

************************************************************************/
void 
CompilerTrackingInfo::updateLongestCompile( Lng32 c )
{
  longestCompileClock_ = ( c > longestCompileClock_ ) ? 
                                    c : longestCompileClock_; 
}
/************************************************************************
method CompilerTrackingInfo::incrementQueriesCompiled

 Add 1 to the appropriate counter given a parameter specifying whether
 this compiled query was a success.

************************************************************************/
void 
CompilerTrackingInfo::incrementQueriesCompiled( NABoolean success )
{
  if( success )
  {
    successfulQueryCount_++;
  }
  else
  {
    failedQueryCount_++;
  }
}
/************************************************************************
method CompilerTrackingInfo::metaDataCacheHits

The current metadata cache hit counter is calculated by
 subtracting the total from the last time we marked an interval
 (see CompilerTrackingInfo::resetInterval and resetMetadataCacheCounters
   for interval marking)

************************************************************************/
inline 
ULng32 
CompilerTrackingInfo::metaDataCacheHits()
{ 
  return ActiveSchemaDB()->getNATableDB()->hits() - mdCacheHits_; 
}
/************************************************************************
method CompilerTrackingInfo::metaDataCacheLookups

The current metadata cache lookups counter is calculated by
 subtracting the total from the last time we marked an interval
 (see CompilerTrackingInfo::resetInterval and resetMetadataCacheCounters
   for interval marking)

************************************************************************/
inline 
ULng32 
CompilerTrackingInfo::metaDataCacheLookups()
{ 
  return ActiveSchemaDB()->getNATableDB()->lookups() - mdCacheLookups_; 
}
/************************************************************************
method CompilerTrackingInfo::resetMetadataCacheCounters

  Mark the current values for hits and lookups so we can
  subtract from the total on the next interval

************************************************************************/
void
CompilerTrackingInfo::resetMetadataCacheCounters()
{
  mdCacheHits_    = ActiveSchemaDB()->getNATableDB()->hits();
  mdCacheLookups_ = ActiveSchemaDB()->getNATableDB()->lookups();

  ActiveSchemaDB()->getNATableDB()->resetIntervalWaterMark();
}
/************************************************************************
method CompilerTrackingInfo::qCacheHits

The current query cache hit counter is calculated by
 subtracting the total from the last time we marked an interval
 (see CompilerTrackingInfo::resetInterval and resetQueryCacheCounters
   for interval marking)

************************************************************************/
inline 
ULng32 
CompilerTrackingInfo::qCacheHits()
{ 
  //
  // nCacheHitsT is the total for all stages
  return currentQCacheStats_.nCacheHitsT - qCacheHits_; 
}
/************************************************************************
method CompilerTrackingInfo::qCacheLookups

The current query cache lookups counter is calculated by
 subtracting the total from the last time we marked an interval
 (see CompilerTrackingInfo::resetInterval and resetQueryCacheCounters
   for interval marking)

************************************************************************/
inline 
ULng32 
CompilerTrackingInfo::qCacheLookups()
{ 
  return currentQCacheStats_.nLookups - qCacheLookups_; 
}  
/************************************************************************
method CompilerTrackingInfo::qCacheRecompiles

The current query cache recompiles counter is calculated by
 subtracting the total from the last time we marked an interval
 (see CompilerTrackingInfo::resetInterval and resetQueryCacheCounters
   for interval marking)

************************************************************************/
inline 
ULng32 
CompilerTrackingInfo::qCacheRecompiles()
{ 
  return currentQCacheStats_.nRecompiles - qCacheRecompiles_; 
}  
/************************************************************************
method CompilerTrackingInfo::qCacheCurrentSize

  The current query cache heap size (no need to reset this on interval)

************************************************************************/
inline 
ULng32 
CompilerTrackingInfo::qCacheCurrentSize()
{ 
  return currentQCacheStats_.currentSize; 
} 
/************************************************************************
method CompilerTrackingInfo::qCacheIntervalWaterMark

  The current query cache heap high water mark. This is maintained
  by the NAHeap class as a second high water mark which is reset on
  the interval

************************************************************************/
inline 
ULng32 
CompilerTrackingInfo::qCacheIntervalWaterMark()
{ 
  updateQCacheIntervalWaterMark();
  return largestQCacheIntervalWaterMark_; 
} 
/************************************************************************
method CompilerTrackingInfo::updateQCacheIntervalWaterMark

 update the QCache heap interval water mark. Store the largest value for 
 the heap size so far.  Update this right before cleanup 
 QueryCache::finalize()
************************************************************************/
void
CompilerTrackingInfo::updateQCacheIntervalWaterMark()
{
  QueryCacheStats stats;
  CURRENTQCACHE->getCacheStats(stats); 
  //
  // update the largest if its bigger
  largestQCacheIntervalWaterMark_ = 
    ( stats.intervalWaterMark > largestQCacheIntervalWaterMark_ ) ? 
      stats.intervalWaterMark : largestQCacheIntervalWaterMark_;
}
/************************************************************************
method CompilerTrackingInfo::clearQCacheCounters

 This is called when the QueryCache is going away.  Since its going
 away, we shouldn't keep the old cache markers.

************************************************************************/
void
CompilerTrackingInfo::clearQCacheCounters()
{
  qCacheHits_ = 0;
  qCacheLookups_ = 0;
  qCacheRecompiles_ = 0;
}
/************************************************************************
method CompilerTrackingInfo::resetQueryCacheCounters

  Mark the current values for hits, lookups, and recompiles so we can
  subtract from the total on the next interval

************************************************************************/
void
CompilerTrackingInfo::resetQueryCacheCounters()
{
  //
  // get the latest cache stats
  CURRENTQCACHE->getCompilationCacheStats(currentQCacheStats_);

  qCacheHits_       = currentQCacheStats_.nCacheHitsT;
  qCacheLookups_    = currentQCacheStats_.nLookups;
  qCacheRecompiles_ = currentQCacheStats_.nRecompiles;

  CURRENTQCACHE->resetIntervalWaterMark();

  largestQCacheIntervalWaterMark_ = 0;
}
/************************************************************************
method CompilerTrackingInfo::hCacheHits

The current histogram cache hit counter is calculated by
 subtracting the total from the last time we marked an interval
 (see CompilerTrackingInfo::resetInterval and resetMetadataCacheCounters
   for interval marking)

************************************************************************/
inline 
ULng32 
CompilerTrackingInfo::hCacheHits()
{ 
  CMPASSERT(NULL!=CURRCONTEXT_HISTCACHE);
  return CURRCONTEXT_HISTCACHE->hits() - hCacheHits_; 
}
/************************************************************************
method CompilerTrackingInfo::hCacheLookups

The current histogram cache lookups counter is calculated by
 subtracting the total from the last time we marked an interval
 (see CompilerTrackingInfo::resetInterval and resetMetadataCacheCounters
   for interval marking)

************************************************************************/
inline 
ULng32 
CompilerTrackingInfo::hCacheLookups()
{ 
  CMPASSERT(NULL!=CURRCONTEXT_HISTCACHE);
  return CURRCONTEXT_HISTCACHE->lookups() - hCacheLookups_; 
}
/************************************************************************
method CompilerTrackingInfo::resetHistogramCacheCounters

  Mark the current values for hits and lookups so we can
  subtract from the total on the next interval

************************************************************************/
void
CompilerTrackingInfo::resetHistogramCacheCounters()
{
  CMPASSERT(NULL!=CURRCONTEXT_HISTCACHE);
  hCacheHits_    = CURRCONTEXT_HISTCACHE->hits();
  hCacheLookups_ = CURRCONTEXT_HISTCACHE->lookups();

  CURRCONTEXT_HISTCACHE->resetIntervalWaterMark();  
}
/************************************************************************
method CompilerTrackingInfo::sessionCount

The sessionCount counter is calculated by
 subtracting the total from the last time we marked an interval
 (see CompilerTrackingInfo::resetInterval for interval marking)

************************************************************************/
inline
ULng32
CompilerTrackingInfo::sessionCount()
{
  return CmpCommon::context()->sqlSession()->getNumSessions() - sessionCount_;
}
/************************************************************************
method CompilerTrackingInfo::updateStmtIntervalWaterMark

The statement interval water mark needs to be maintained for the entire
interval (across multiple statements).  So this largestIntervalWaterMark
should get saved immediately before the statement is deleted.

************************************************************************/
void
CompilerTrackingInfo::updateStmtIntervalWaterMark()
{
  size_t stmtIntervalWaterMark = 
    CmpCommon::statementHeap()->getIntervalWaterMark();
  //
  // update the largest if its bigger
  largestStmtIntervalWaterMark_ = 
    ( stmtIntervalWaterMark > largestStmtIntervalWaterMark_ ) ? 
      stmtIntervalWaterMark : largestStmtIntervalWaterMark_;
}
/************************************************************************
method CompilerTrackingInfo::stmtHeapIntervalWaterMark

Return whichever is bigger... the current interval water mark or
the largest previous statement water mark for this interval

Could also make a call to updateStmtIntervalWaterMark first, then return
largestStmtIntervalWaterMark_.

************************************************************************/
inline 
size_t 
CompilerTrackingInfo::stmtHeapIntervalWaterMark()
{ 
  size_t stmtIntervalWaterMark = 
    CmpCommon::statementHeap()->getIntervalWaterMark();

  return ( stmtIntervalWaterMark > largestStmtIntervalWaterMark_ ) ? 
          stmtIntervalWaterMark : largestStmtIntervalWaterMark_; 
}
/************************************************************************
method CompilerTrackingInfo::logIntervalInPrivateTabale
log the interval data into a private table 

************************************************************************/

void
CompilerTrackingInfo::logIntervalInPrivateTable()
{
  /* ------------------------------------------------------
     -- for debugging purposes we may need to dump the tracking
     -- information in a private table. The DDL of this table
     -- should be as follows: (change the name as needed)

     create table STATE_TRACKING_COMPILERS_TABLE_PRIVATE
     (
        LOGGED_AT_LCT_TS            TIMESTAMP(6) NOT NULL,
        COMPILER_ID                 CHAR(28) CHARACTER SET UCS2 NOT NULL,
        PROCESS_ID                  INT UNSIGNED NOT NULL,
        INTERVAL_START_LCT_TS       TIMESTAMP(6),
        INTERVAL_PATH_LEN           LARGEINT,
        LONGEST_COMPILE_PATH        LARGEINT,
        COMPILER_AGE                LARGEINT,
        NUM_SESSIONS                INT UNSIGNED,
        STMT_HEAP_HWTR_MARK         LARGEINT,
        CONTEXT_HEAP_SIZE           LARGEINT,
        CONTEXT_HEAP_HWTR_MARK      LARGEINT,
        SYSTEM_HEAP_SIZE            LARGEINT,
        SYSTEM_HEAP_HWTR_MARK       LARGEINT, 
        METADATA_CACHE_SIZE         LARGEINT,
        METADATA_CACHE_HWTR_MARK    LARGEINT,
        METADATA_CACHE_HITS         LARGEINT,
        METADATA_CACHE_LOOKUPS      LARGEINT,
        QUERY_CACHE_SIZE            LARGEINT,
        QUERY_CACHE_HWTR_MARK       LARGEINT,
        QUERY_CACHE_HITS            LARGEINT,
        QUERY_CACHE_LOOKUPS         LARGEINT,
        HISTOGRAM_CACHE_SIZE        LARGEINT,
        HISTOGRAM_CACHE_HWTR_MARK   LARGEINT,
        HISTOGRAM_CACHE_HITS        LARGEINT,
        HISTOGRAM_CACHE_LOOKUPS     LARGEINT,
        NUM_QUERIES_COMPILED        LARGEINT,
        NUM_FAILED_QUERIES          LARGEINT,
        NUM_CAUGHT_EXCEPTIONS       LARGEINT,
        NUM_RECOMPILES              LARGEINT,
        COMPILER_INFO               VARCHAR(256) CHARACTER SET UCS2,
        primary key (LOGGED_AT_LCT_TS, COMPILER_ID)
     );

     ------------------------------------------------------ */

  // the pointer to the process info for this tracker
  CmpProcess *p = processInfo_;
  char beginTime[100];
  char endTime[100]; 
  char compilerId[COMPILER_ID_LEN];

  CMPASSERT( NULL != p );

  NAString dmlprep = "insert into %s values("; // INSERT INTO TABLE VALUES
  dmlprep += "timestamp '%s'";           // LOGGED_AT_LCT_TS
  dmlprep += ",'%s'";                    // COMPILER_ID
  dmlprep += ",%d";                      // PROCESS_ID
  dmlprep += ",timestamp '%s'";          // INTERVAL_START_LCT_TS
  dmlprep += ",%d";                      // INTERVAL_PATH_LEN
  dmlprep += ",%d";                      // LONGEST_COMPILE_PATH
  dmlprep += ",%d";                      // COMPILER_AGE
  dmlprep += ",%d";                      // NUM_SESSIONS
  dmlprep += ",%d";                      // STMT_HEAP_HWTR_MARK
  dmlprep += ",%d";                      // CONTEXT_HEAP_SIZE
  dmlprep += ",%d";                      // CONTEXT_HEAP_HWTR_MARK
  dmlprep += ",%d";                      // SYSTEM_HEAP_SIZE
  dmlprep += ",%d";                      // SYSTEM_HEAP_HWTR_MARK
  dmlprep += ",%d";                      // METADATA_CACHE_SIZE
  dmlprep += ",%d";                      // METADATA_CACHE_HWTR_MARK
  dmlprep += ",%d";                      // METADATA_CACHE_HITS
  dmlprep += ",%d";                      // METADATA_CACHE_LOOKUPS
  dmlprep += ",%d";                      // QUERY_CACHE_SIZE
  dmlprep += ",%d";                      // QUERY_CACHE_HWTR_MARK
  dmlprep += ",%d";                      // QUERY_CACHE_HITS
  dmlprep += ",%d";                      // QUERY_CACHE_LOOKUPS
  dmlprep += ",%d";                      // HISTOGRAM_CACHE_SIZE
  dmlprep += ",%d";                      // HISTOGRAM_CACHE_HWTR_MARK
  dmlprep += ",%d";                      // HISTOGRAM_CACHE_HITS
  dmlprep += ",%d";                      // HISTOGRAM_CACHE_LOOKUPS
  dmlprep += ",%d";                      // NUM_QUERIES_COMPILED
  dmlprep += ",%d";                      // NUM_FAILED_QUERIES
  dmlprep += ",%d";                      // NUM_CAUGHT_EXCEPTIONS
  dmlprep += ",%d";                      // NUM_RECOMPILES
  dmlprep += ",'%s'";                      // COMPILER_INFO
  dmlprep += ");";                                  

  getTimestampAsBuffer(beginIntervalTime(), beginTime);
  getTimestampAsBuffer(endIntervalTime(), endTime);  
  p->getCompilerId(compilerId);
  NAString tableName = ActiveSchemaDB()->getDefaultSchema().getSchemaNameAsAnsiString() + "." + COMPILER_TRACKING_TABLE_NAME_PRIVATE;
  //
  // update the fields
  char dmlbuffer[8192];  
  str_sprintf( dmlbuffer, 
           (const char*)dmlprep,
           tableName.data(),
           endTime,
           compilerId,
           p->getPin(),
           beginTime,
           cpuPathLength(),
           longestCompile(),
           compilerAge(),
           sessionCount(), 
           stmtHeapIntervalWaterMark(),
           cxtHeapCurrentSize(),
           cxtHeapIntervalWaterMark(),
           p->getCurrentSystemHeapSize(),
           systemHeapIntervalWaterMark(),
           metaDataCacheCurrentSize(),
           metaDataCacheIntervalWaterMark(),
           metaDataCacheHits(),
           metaDataCacheLookups(),
           qCacheCurrentSize(),
           qCacheIntervalWaterMark(),
           qCacheHits(),
           qCacheLookups(),
           hCacheCurrentSize(),
           hCacheIntervalWaterMark(),
           hCacheHits(),
           hCacheLookups(),
           successfulQueryCount(),
           failedQueryCount(),
           caughtExceptionCount(),
           qCacheRecompiles(),
           compilerInfo());

  CmpCliStmt begin("BEGIN WORK;");
  if( begin.executeNoDataExpected() )
  {
    begin.myfree();

    CmpCliStmt interval(dmlbuffer);
    if( interval.executeNoDataExpected() )
    {
      interval.myfree();

      CmpCliStmt commit("COMMIT WORK;");
      commit.executeNoDataExpected();
      commit.myfree();
    }
    else
    {
      interval.myfree();

      CmpCliStmt rollback("ROLLBACK WORK;");
      rollback.executeNoDataExpected();
      rollback.myfree();
    }    
  }
}

/************************************************************************

method CompilerTrackingInfo::logIntervalInRepository
log the interval data into a repository table 

The repository table used is: 
        MANAGEABILITY.INSTANCE_REPOSITORY.STATE_TRACKING_COMPILERS_TABLE

************************************************************************/

// Don't log this on Windows since the repository table does not exist
Int32 CompilerTrackingInfo::logIntervalInRepository()
{
#ifdef SP_DIS
   return 0;
#else
   // declare a stack variable and populate
   sql::tracking_compilers trc_row;

   Int32 rc;
   if (!isAMQPConnectionEstablished())
   {
      rc = createAMQPConnection (NULL, -1);
      if (rc)
         return rc;
      AMQPConnectionEstablished(TRUE);
      
   }

   // the pointer to the process info for this tracker
   CmpProcess *p = processInfo_;
   CMPASSERT( NULL != p );

   // ============================================
   // -- start filling up the message structure --
   // ============================================
  
   // -- compiler Id
   char compilerId[COMPILER_ID_LEN];
   memset (compilerId, '\0', COMPILER_ID_LEN);
   p->getCompilerId(compilerId);
   trc_row.set_compiler_id(compilerId);

   // interval start time
   trc_row.set_interval_start_lct_ts(beginIntervalTimeUEpoch());

   // -- the CPU usage of this interval
   trc_row.set_interval_path_len(cpuPathLength());

   // -- longest CPU path length for single compile
   // -- within this interval
   trc_row.set_longest_compile_path(longestCompile());

   // -- age of compiler being tracked
   trc_row.set_compiler_age(compilerAge());

   // -- number of sessions during this interval
   trc_row.set_num_sessions(sessionCount());

   // -- largest size of the statement heap so far
   trc_row.set_stmt_heap_hwtr_mark(stmtHeapIntervalWaterMark());

   // -- number of bytes currently allocated for the
   // -- context heap of this process
   trc_row.set_context_heap_size(cxtHeapCurrentSize());

   // -- largest size of the context heap so far
   trc_row.set_context_heap_hwtr_mark(cxtHeapIntervalWaterMark());

   // -- number of bytes currently allocated for the
   // -- system heap of this process
   trc_row.set_system_heap_size(p->getCurrentSystemHeapSize());

   // -- largest size of the system heap so far
   trc_row.set_system_heap_hwtr_mark(systemHeapIntervalWaterMark());

   // -- number of bytes currently allocated for the
   // -- metadata cache
   trc_row.set_metadata_cache_size(metaDataCacheCurrentSize());

   // -- largest size of the metadata cache so far
   trc_row.set_metadata_cache_hwtr_mark(metaDataCacheIntervalWaterMark());

   // -- number of hits on the metadata cache
   trc_row.set_metadata_cache_hits(metaDataCacheHits());

   // -- number of misses on the metadata cache
   trc_row.set_metadata_cache_lookups(metaDataCacheLookups());

   // -- number of bytes currently allocated for the
   // -- query cache
   trc_row.set_query_cache_size(qCacheCurrentSize());

   // -- largest size of the query cache so far
   trc_row.set_query_cache_hwtr_mark(qCacheIntervalWaterMark());

   // -- number of hits on the query cache
   trc_row.set_query_cache_hits(qCacheHits());

   // -- number of misses on the query cache
   trc_row.set_query_cache_lookups(qCacheLookups());

   // -- number of bytes currently allocated for the
   // -- histogram cache
   trc_row.set_histogram_cache_size(hCacheCurrentSize());

   // -- largest size of the histogram cache so far
   trc_row.set_histogram_cache_hwtr_mark(hCacheIntervalWaterMark());

   // -- number of hits on the histogram cache
   trc_row.set_histogram_cache_hits(hCacheHits());

   // -- number of misses on the histogram cache
   trc_row.set_histogram_cache_lookups(hCacheLookups());

   // -- number of compiled queries (DDL and DML)
   trc_row.set_num_queries_compiled(successfulQueryCount());

   // -- number of failed compilations
   trc_row.set_num_failed_queries(failedQueryCount());

   // -- number of queries compiled successfully
   // -- but with warnings (2053 and 2078)
   trc_row.set_num_caught_exceptions(caughtExceptionCount());

   // -- the number of recompiles
   trc_row.set_num_recompiles(qCacheRecompiles());

   // -- extend any new counters
   trc_row.set_compiler_info(compilerInfo());

   // ============================================
   //  -- done filling up the message structure --
   // ============================================
  
   // populate the info header
   common::info_header *infoHeader = NULL;
   infoHeader = trc_row.mutable_header();
   rc =  initAMQPInfoHeader(infoHeader, SQEVL_SQL);
   if (rc)
   {
      closeAMQPConnection();
      AMQPConnectionEstablished(FALSE);
      return rc;
      
   }

   // set the routing key and send the message
   AMQPRoutingKey routingKey (SP_HEALTH_STATE, SP_SQLPACKAGE, SP_INSTANCE, SP_PUBLIC, 
                              SP_GPBPROTOCOL, "tracking_compilers");
   try {
     rc = sendAMQPMessage (true, trc_row.SerializeAsString(), 
                           SP_CONTENT_TYPE_APP, routingKey);
   
     if (rc) throw 1;
   } catch (...) {
     if (rc == SP_SUCCESS) rc = SP_SEND_FAILED; 
     closeAMQPConnection();
     AMQPConnectionEstablished(FALSE);
   }
  
   return rc;
#endif
}

       // doesn't exist. Use CQD to log to a private table instead if needed

/************************************************************************
method CompilerTrackingInfo::printToFile

helper to simply print the tracker info into the specified file. 

************************************************************************/
void
CompilerTrackingInfo::printToFile()
{
  const char *trackerLogFilename = getCompilerTrackingLogFilename();

  CMPASSERT( NULL != trackerLogFilename );

  // the pointer to the process info for this tracker
  CmpProcess *p = processInfo_;

  CMPASSERT( NULL != p );

  ofstream fileout(trackerLogFilename, ios::app);
    
  fileout << "--------------------------------\n";
  fileout << "         Start Interval\n";
  fileout << endl;
  char beginTime[100];
  char endTime[100];  
  getTimestampAsBuffer(beginIntervalTime(), beginTime);
  getTimestampAsBuffer(endIntervalTime(), endTime);
  fileout << "Logged Interval At\t: " << endTime << endl;
  fileout << "Interval Start Time\t: " << beginTime << endl;  
  fileout << "Interval Duration\t: " << currentIntervalDuration(endIntervalTime()) 
                << " minutes" << endl;

  fileout << endl;
     
  char compilerId[COMPILER_ID_LEN];
  p->getCompilerId(compilerId);
  fileout << "Compiler ID\t\t: " << compilerId << endl; 
  fileout << "Process ID\t\t: " << p->getPin() << endl;  
  fileout << "Compiler Age\t\t: " << compilerAge() << " minutes\n";
  fileout << "Successful Compilations : " << successfulQueryCount() << endl;
  fileout << "Failed Compilations \t: " << failedQueryCount() << endl;
  fileout << "Recompiles \t\t: " << qCacheRecompiles() << endl;
  fileout << "Sessions \t\t: " << sessionCount() << endl;
  fileout << "Caught Exceptions \t: " << caughtExceptionCount() << endl;
  fileout << endl;
  fileout << "Interval CPU time\t: " << cpuPathLength() << endl;
  fileout << "Longest Compile (CPU)\t: " << longestCompile() << endl;
  //
  // additional compiler info
  fileout << endl;
  fileout << "Compiler Info\t\t: " << compilerInfo() << endl;
  //
  // heap/cache table
  fileout << endl;  
  fileout.width(CACHE_HEAP_HEADER_LEN);
  fileout << "";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "CurrentSize";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "HighWaterMark";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "Hits";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "Lookups";
  fileout << endl;

  fileout.width(CACHE_HEAP_HEADER_LEN);
  fileout << "";
  fileout.width(CACHE_HEAP_VALUE_LEN*4);
  fileout << "----------------------------------------------------------------";
  fileout << "\n\n";    
  //
  // system heap
  fileout.width(CACHE_HEAP_HEADER_LEN);
  fileout << "System Heap: ";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << p->getCurrentSystemHeapSize();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << systemHeapIntervalWaterMark();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "N/A";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "N/A";
  fileout << endl;
  //
  // context heap
  fileout.width(CACHE_HEAP_HEADER_LEN);
  fileout << "Context Heap: ";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << cxtHeapCurrentSize();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << cxtHeapIntervalWaterMark();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "N/A";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "N/A";
  fileout << endl;
  //
  // statement heap
  fileout.width(CACHE_HEAP_HEADER_LEN);
  fileout << "Statement Heap: "; 
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "N/A";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << stmtHeapIntervalWaterMark();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "N/A";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << "N/A";
  fileout << endl;
  //
  // metadata cache
  fileout.width(CACHE_HEAP_HEADER_LEN);
  fileout << "Metadata Cache: ";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << metaDataCacheCurrentSize();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << metaDataCacheIntervalWaterMark();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << metaDataCacheHits();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << metaDataCacheLookups();  
  fileout << endl;
  //
  // query cache
  fileout.width(CACHE_HEAP_HEADER_LEN);
  fileout << "Query Cache: ";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << qCacheCurrentSize();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << qCacheIntervalWaterMark();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << qCacheHits();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << qCacheLookups();    
  fileout << endl;
  //
  // histogram cache
  fileout.width(CACHE_HEAP_HEADER_LEN);
  fileout << "Histogram Cache: ";
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << hCacheCurrentSize();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << hCacheIntervalWaterMark();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << hCacheHits();
  fileout.width(CACHE_HEAP_VALUE_LEN);
  fileout << hCacheLookups();    
  fileout << endl;  
  
  fileout << endl;
  fileout << "         End Interval\n";
  fileout << "--------------------------------\n";
  fileout.close();
}
