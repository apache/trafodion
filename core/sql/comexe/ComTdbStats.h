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
****************************************************************************
*
* File:         ComTdbStats.h
* Description:  
*
* Created:      12/1/1999
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COMTDBSTATS_H
#define COMTDBSTATS_H

#include "ComTdb.h"
#include "ExpCriDesc.h"
#include "exp_attrs.h"

class StatsDesc;
class StatsTuple;
class ExMasterStats;

// class to hold cost to be returned. 
// Used to populate the struct QueryCostInfo defined in cli/sqlcli.h 
class QueryCostInfo
{
public:
  QueryCostInfo()
       : cpuTime_(0),
	 ioTime_(0),
	 msgTime_(0),
	 idleTime_(0),
         numSeqIOs_(0),
	 numRandIOs_(0),
	 totalTime_(0),
	 cardinality_(0),
	 totalMem_(0),
	 resourceUsage_(0),
	 maxCpuUsage_(0)
  {}
  
  double cpuTime()       { return cpuTime_;}
  double ioTime()        { return ioTime_;}
  double msgTime()       { return msgTime_;}
  double idleTime()      { return idleTime_;}
  double numSeqIOs()     { return numSeqIOs_;}
  double numRandIOs()    { return numRandIOs_;}
  double totalTime()     { return totalTime_;}
  double cardinality()   { return cardinality_;}
  double totalMem()      { return totalMem_;}
  short  resourceUsage() { return resourceUsage_; }
  short maxCpuUsage() { return maxCpuUsage_; }

  void setCostInfo(double cpuT, double ioT, double msgT,
		   double idleT, double numSeqIOs, double numRandIOs,
                   double totalT, double cardinality, double totalM,
                   short maxCU)
  {
    cpuTime_       = cpuT;
    ioTime_        = ioT;
    msgTime_       = msgT;
    idleTime_      = idleT;
    numSeqIOs_     = numSeqIOs;
    numRandIOs_    = numRandIOs;
    totalTime_     = totalT;
    cardinality_   = cardinality;
    totalMem_      = totalM;
    maxCpuUsage_   = maxCU;
  };
  
  void setResourceUsage(short ru) { resourceUsage_ = ru; }
  void translateToExternalFormat(SQL_QUERY_COST_INFO *query_cost_info);
private:
  double cpuTime_;
  double ioTime_;
  double msgTime_;
  double idleTime_;
  double numSeqIOs_;
  double numRandIOs_;
  double totalTime_;
  double cardinality_;
  double totalMem_;
  short  resourceUsage_;
  short  maxCpuUsage_;
  char   fillerCost_[12];
};
// -----------------------------------------------------------------------
//  CompilationStatsData
//
//   Data gathered from the CompilationStats class in the compiler.
//   Used to populate the SQL_COMPILATION_STATS_DATA struct in cli/sqlcli.h 
// -----------------------------------------------------------------------
class CompilationStatsData : public NAVersionedObject
{
public:
  CompilationStatsData()
    : NAVersionedObject(-1),
      compileStartTime_(0),
      compileEndTime_(0),
      compilerId_(NULL),      
      cmpCpuTotal_(0),
      cmpCpuBinder_(0),
      cmpCpuNormalizer_(0),
      cmpCpuAnalyzer_(0),
      cmpCpuOptimizer_(0),
      cmpCpuGenerator_(0),
      metadataCacheHits_(0),
      metadataCacheLookups_(0),
      queryCacheState_(0),
      histogramCacheHits_(0),
      histogramCacheLookups_(0),
      stmtHeapSize_(0),
      cxtHeapSize_(0),
      optTasks_(0),
      optContexts_(0),
      isRecompile_(0),
      compileInfo_(NULL),
      compileInfoLen_(0)
  {};

   CompilationStatsData(Int64 compileStartTime,
                         Int64 compileEndTime,
                         char *compilerId,                  
                         Lng32 cmpCpuTotal,
                         Lng32 cmpCpuBinder,
                         Lng32 cmpCpuNormalizer,
                         Lng32 cmpCpuAnalyzer,
                         Lng32 cmpCpuOptimizer,
                         Lng32 cmpCpuGenerator,
                         ULng32 metadataCacheHits,
                         ULng32 metadataCacheLookups,
                         Int32 queryCacheState,
                         ULng32 histogramCacheHits,
                         ULng32 histogramCacheLookups,
                         Lng32 stmtHeapSize,
                         Lng32 cxtHeapSize,
                         Lng32 optTasks,
                         Lng32 optContexts,
                         short isRecompile,
                         char *compileInfo,
                         Int32 compileInfoLen)
    : NAVersionedObject(-1),
      compileStartTime_(compileStartTime),
      compileEndTime_(compileEndTime),
      compilerId_(compilerId),      
      cmpCpuTotal_(cmpCpuTotal),
      cmpCpuBinder_(cmpCpuBinder),
      cmpCpuNormalizer_(cmpCpuNormalizer),
      cmpCpuAnalyzer_(cmpCpuAnalyzer),
      cmpCpuOptimizer_(cmpCpuOptimizer),
      cmpCpuGenerator_(cmpCpuGenerator),
      metadataCacheHits_(metadataCacheHits),
      metadataCacheLookups_(metadataCacheLookups),
      queryCacheState_(queryCacheState),
      histogramCacheHits_(histogramCacheHits),
      histogramCacheLookups_(histogramCacheLookups),
      stmtHeapSize_(stmtHeapSize),
      cxtHeapSize_(cxtHeapSize),
      optTasks_(optTasks),
      optContexts_(optContexts),
      isRecompile_(isRecompile),
      compileInfo_(compileInfo),
      compileInfoLen_(compileInfoLen)
  {};

 
  CompilationStatsData& 
  operator=(CompilationStatsData &csd);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize() { return (short)sizeof(CompilationStatsData); }

  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);

  Int64 compileStartTime() { return compileStartTime_; }
  Int64 compileEndTime() { return compileEndTime_; }
  char *compilerId() { return (char*)compilerId_.getPointer(); }  
  Lng32 cmpCpuTotal() { return cmpCpuTotal_; }
  Lng32 cmpCpuBinder() { return cmpCpuBinder_; }
  Lng32 cmpCpuNormalizer() { return cmpCpuNormalizer_; }
  Lng32 cmpCpuAnalyzer() { return cmpCpuAnalyzer_; }
  Lng32 cmpCpuOptimizer() { return cmpCpuOptimizer_; }
  Lng32 cmpCpuGenerator() { return cmpCpuGenerator_; }
  ULng32 metadataCacheHits() { return metadataCacheHits_; }
  ULng32 metadataCacheLookups() { return metadataCacheLookups_; }
  Int32 queryCacheState() { return queryCacheState_; }
  ULng32 histogramCacheHits() { return histogramCacheHits_; }
  ULng32 histogramCacheLookups() { return histogramCacheLookups_; }
  Lng32 stmtHeapSize() { return stmtHeapSize_; }
  Lng32 contextHeapSize() { return cxtHeapSize_; }
  Lng32 optTasks() { return optTasks_; }
  Lng32 optContexts() { return optContexts_; }
  short isRecompile() { return isRecompile_; }
  char *compileInfo() { return (char*)compileInfo_.getPointer(); }  
  Int32 compileInfoLen() { return compileInfoLen_; }
  //
  // these values are found after the object is created and added back in
  void setCmpCpuGenerator(Lng32 cmpCpuGenerator ) 
    { cmpCpuGenerator_ = cmpCpuGenerator; }
  void setCmpCpuTotal(Lng32 cmpCpuTotal ) 
    { cmpCpuTotal_ = cmpCpuTotal; }
  void setCompileEndTime(Int64 endTime)
    { compileEndTime_ = endTime; }
  void translateToExternalFormat(SQL_COMPILATION_STATS_DATA *cmpData, 
          Int64 cmpStartTime, Int64 cmpEndTime);

private:  
  Int64 compileStartTime_;                          //  0 -  7
  Int64 compileEndTime_;                            //  8 - 15
  NABasicPtr compilerId_;                           // 16 - 23
  Int32 cmpCpuTotal_;                               // 24 - 27 
  Int32 cmpCpuBinder_;                              // 28 - 31
  Int32 cmpCpuNormalizer_;                          // 32 - 35
  Int32 cmpCpuAnalyzer_;                            // 36 - 39
  Int32 cmpCpuOptimizer_;                           // 40 - 43
  Int32 cmpCpuGenerator_;                           // 44 - 47
  Int32 metadataCacheHits_;                         // 48 - 51
  Int32 metadataCacheLookups_;                      // 52 - 55
  Int32 queryCacheState_;                           // 56 - 59
  Int32 histogramCacheHits_;                        // 60 - 63
  Int32 histogramCacheLookups_;                     // 64 - 67
  Int32 stmtHeapSize_;                              // 68 - 71
  Int32 cxtHeapSize_;                               // 72 - 75
  Int32 optTasks_;                                  // 76 - 79
  Int32 optContexts_;                               // 80 - 83
  Int16 isRecompile_;                               // 84 - 85

  Int16 fillBoundary_;                              // 86 - 87

  NABasicPtr compileInfo_;                          // 88 - 95
  Int32 compileInfoLen_;                            // 96 - 99
  char filler_[42];                                 // 100 - 141
};

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for CompilationStatsData
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<CompilationStatsData> CompilationStatsDataPtr;

class CompilerStatsInfo
{
public:
  CompilerStatsInfo();

  CompilerStatsInfo& operator=(CompilerStatsInfo&);

  UInt16 &totalOps() { return totalOps_; }
  UInt16 &exchangeOps() { return exchangeOps_; }
  UInt16 &dp2Ops() { return dp2Ops_; }
  UInt16 &hj() { return hj_; }
  UInt16 &mj() { return mj_; }
  UInt16 &nj() { return nj_; }
  UInt16 totalJoins() { return (hj_ + mj_ + nj_); }
  UInt16 &espTotal() { return espTotal_; }
  UInt16 &espLevels() { return espLevels_; }
  UInt16 &dop() { return dop_; }
  UInt32 &affinityNumber() { return affinityNumber_; }
  Int32 &totalFragmentSize() { return totalFragmentSize_; }
  Int32 &masterFragmentSize() { return masterFragmentSize_; }
  Int32 &espFragmentSize() { return espFragmentSize_; }
  Int32 &dp2FragmentSize() { return dp2FragmentSize_; }
  double &dp2RowsAccessed() { return dp2RowsAccessed_; }
  double &dp2RowsUsed() { return dp2RowsUsed_; }
  double &dp2RowsAccessedForFullScan() { return dp2RowsAccessedForFullScan_; }
  UInt16 &collectStatsType() { return collectStatsType_; }
  UInt16 &bmo() { return bmo_; }
  UInt16 &udr() { return udr_; }
  double &ofSize() { return ofSize_; }
  UInt16 &ofMode() { return ofMode_; }
  Int16 &queryType() { return queryType_; }
  Int16 &subqueryType() { return subqueryType_; }
  double &bmoMemLimitPerNode() { return bmoMemLimitPerNode_; }
  double &estBmoMemPerNode() { return estBmoMemPerNode_; }

  NABoolean mandatoryCrossProduct() { return (flags_ & MANDATORY_CROSS_PRODUCT) != 0; }
  void setMandatoryCrossProduct(NABoolean v)      
  { (v ? flags_ |= MANDATORY_CROSS_PRODUCT : flags_ &= ~MANDATORY_CROSS_PRODUCT); }

  NABoolean missingStats() { return (flags_ & MISSING_STATS) != 0; }
  void setMissingStats(NABoolean v)      
  { (v ? flags_ |= MISSING_STATS : flags_ &= ~MISSING_STATS); }

  NABoolean fullScanOnTable() { return (flags_ & FULL_SCAN_ON_TABLE) != 0; }
  void setFullScanOnTable(NABoolean v)      
  { (v ? flags_ |= FULL_SCAN_ON_TABLE : flags_ &= ~FULL_SCAN_ON_TABLE); }

  NABoolean highDp2MxBufferUsage() { return (flags_ & HIGH_DP2_MX_BUFFER_USAGE) != 0; }
  void setHighDp2MxBufferUsage(NABoolean v)      
  { (v ? flags_ |= HIGH_DP2_MX_BUFFER_USAGE : flags_ &= ~HIGH_DP2_MX_BUFFER_USAGE); }
  void translateToExternalFormat(SQL_QUERY_COMPILER_STATS_INFO *query_comp_stats_info, short xnNeeded);
private:
  enum
  {
    MANDATORY_CROSS_PRODUCT   = 0x0001,
    MISSING_STATS             = 0x0002,
    FULL_SCAN_ON_TABLE        = 0x0004,
    HIGH_DP2_MX_BUFFER_USAGE  = 0x0008
  };

  UInt16 totalOps_;
  UInt16 exchangeOps_;
  UInt16 dp2Ops_;
  UInt16 hj_;
  UInt16 mj_;
  UInt16 nj_;
  UInt16 espTotal_;
  UInt16 espLevels_;
  UInt16 dop_;
  UInt16 collectStatsType_;

  UInt32  affinityNumber_;

  Int32 totalFragmentSize_;
  Int32 masterFragmentSize_;
  Int32 espFragmentSize_;
  Int32 dp2FragmentSize_;

  double dp2RowsAccessed_;
  double dp2RowsUsed_;

  // next field is valid if FULL_SCAN_ON_LARGE_TABLE is set.
  // It returns the size of the largest table in the query which does
  // a full table scan.
  double dp2RowsAccessedForFullScan_;

  UInt32 flags_;  

  UInt16 bmo_;
  UInt16 udr_;
  double ofSize_;
  UInt16 ofMode_;
  Int16 queryType_;
  Int16 subqueryType_;
  double bmoMemLimitPerNode_;
  double estBmoMemPerNode_;
  char   filler_[32];
};

// the enum values assigned to the enums must be the same as the
// position of the corresponding record in StatsVirtTableColumnInfo.
// Values are 0 based, so first entry (TDB_ID) gets enum of 0.
enum StatType
{
  STAT_MOD_NAME       =  0,
  STAT_STATEMENT_NAME =  1,
  STAT_PLAN_ID        =  2,
  STAT_TDB_ID         =  3,
  STAT_FRAG_NUM       =  4,
  STAT_INST_NUM       =  5,
  STAT_SUB_INST_NUM   =  6,
  STAT_LINE_NUM       =  7,
  STAT_PARENT_TDB_ID  =  8,
  STAT_LC_TDB_ID      =  9,
  STAT_RC_TDB_ID      = 10,
  STAT_SEQ_NUM        = 11,
  STAT_TDB_NAME       = 12,
  STAT_WORK_CALLS     = 13,
  STAT_EST_ROWS       = 14, 
  STAT_ACT_ROWS       = 15,
  STAT_UP_Q_SZ        = 16,
  STAT_DN_Q_SZ        = 17,
  STAT_VAL1_TXT       = 18,
  STAT_VAL1           = 19,
  STAT_VAL2_TXT       = 20,
  STAT_VAL2           = 21,
  STAT_VAL3_TXT       = 22,
  STAT_VAL3           = 23,
  STAT_VAL4_TXT       = 24,
  STAT_VAL4           = 25,
  STAT_TEXT           = 26,
  STAT_VARIABLE_INFO  = 27
};

// -----------------------------------------------------------------------
// Columns of the statistics built-in stored procedure:
// - The first few columns are similar to the EXPLAIN stored procedure
//   to allow a join between them. One can do a NATURAL JOIN, which
//   will pick STATEMENT_NAME, PLAN_ID and SEQ_NUM as join columns:
//
//      select ... from table(explain(NULL,'S')) natural join
//                      table(statistics(NULL,'S'));
//
// - If there were a primary key on this table, it would at least have
//   to contain the following columns: PLAN_ID, TDB_ID, INST_NO,
//   SUB_INST_NO:
//     - PLAN_ID is a unique number, representing MODULE_NAME and
//       STATEMENT_NAME (can't use these two columns themselves because
//       the data in them may have been truncated to 60 columns).
//     - TDB_ID uniquely identifies each TDB in a statement (can't use
//       the SEQ_NUM field because we sometimes generate more than
//       one TDB for a given explain operator).
//     - INST_NUM uniquely identifies the parallel instance, if the
//       TDB is downloaded into a DP2 of a partitioned table or into
//       an ESP. Note that FRAG_NUM is not needed, for a given TDB
//       the fragment number is always constant. FRAG_NUM is just
//       provided to allow users to group TDBs according to fragments.
//     - Now, even INST_NUM is not enough if a particular fragment
//       instance makes multiple copies of a TCB for one TDB. This
//       happens currently with partition access nodes under a PAPA,
//       with send top and with send bottom nodes. The SUB_INST_NUM
//       is used to number such multiple TCBs, it is set to 0 in
//       all other cases.
//     - LINE_NUM is a field that allows us to return multiple rows
//       for one TCB instance if the VARIABLE_INFO field is too small
//       to contain all the variable information. LINE_NUM is 0 for
//       the first part of VARIABLE_INFO.
//     - PARENT_TDB_ID shows the tree structure of the data. It points
//       to the parent node in the TDB tree (see TDB_ID field above).
//     - So far we have described the columns that identify a particular
//       TCB. The rest of the columns, beginning with UP_Q_SZ, provide
//       the actual statistics. Column TDB_NAME is kind of in-between,
//       it provides the type of the TDB/TCB pair.
//     - The up/down queue sizes are the maximum sizes after dynamic
//       resize operations.
//     - WORK_CALLS counts the number of calls to a work method for
//       this TCB.
//     - ELAPSED time measures the wall clock time spent in the work
//       method for this TCB, measured in microseconds.
//     - EST_ROWS give the compiler estimate of how many rows are
//       returned by this TDB (note: this is the estimated sum for
//       all parallel instances, not just one).
//     - ACT_ROWS is the actual number of rows returned by this TCB.
//     - VARIABLE_INFO contains token-value pairs, similar to the
//       DESCRIPTION column of the EXPLAIN stored procedure.
// The statistics table uses SQLARK_EXPLODED_FORMAT tuple format.
// The column offset information comments can be useful for debugging.
// -----------------------------------------------------------------------
static const ComTdbVirtTableColumnInfo statsVirtTableColumnInfo[] =
  {                                                                               // offset
    { "MOD_NAME",       0, COM_USER_COLUMN, REC_BYTE_F_ASCII,     60, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL ,COM_UNKNOWN_DIRECTION_LIT, 0},  //    0
    { "STATEMENT_NAME", 1, COM_USER_COLUMN, REC_BYTE_F_ASCII,     60, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //   62
    { "PLAN_ID",        2, COM_USER_COLUMN, REC_BIN64_SIGNED,      8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  124
    { "TDB_ID",         3, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  136
    { "FRAG_NUM",       4, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  140
    { "INST_NUM",       5, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  144
    { "SUB_INST_NUM",   6, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  148
    { "LINE_NUM",       7, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  152
    { "PARENT_TDB_ID", 8, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  156
    { "LC_TDB_ID",      9, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  156
    { "RC_TDB_ID",      10, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  156
    { "SEQ_NUM",        11, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  164
    { "TDB_NAME",       12, COM_USER_COLUMN, REC_BYTE_F_ASCII,     24, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  172
    { "WORK_CALLS",    13, COM_USER_COLUMN, REC_BIN64_SIGNED,      8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  196
    { "EST_ROWS",        14, COM_USER_COLUMN, REC_BIN64_SIGNED,      8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  208
    { "ACT_ROWS",        15, COM_USER_COLUMN, REC_BIN64_SIGNED,      8, FALSE, SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  216
    { "UP_Q_SZ",            16, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  224
    { "DN_Q_SZ",           17, COM_USER_COLUMN, REC_BIN32_UNSIGNED,    4, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  232
    { "VAL1_TXT",        18, COM_USER_COLUMN, REC_BYTE_F_ASCII,     16, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  240
    { "VAL1",                19, COM_USER_COLUMN, REC_BIN64_SIGNED,      8, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  258
    { "VAL2_TXT",        20, COM_USER_COLUMN, REC_BYTE_F_ASCII,     16, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  272
    { "VAL2",                21, COM_USER_COLUMN, REC_BIN64_SIGNED,      8, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  290
    { "VAL3_TXT",        22, COM_USER_COLUMN, REC_BYTE_F_ASCII,     16, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  304
    { "VAL3",                23, COM_USER_COLUMN, REC_BIN64_SIGNED,      8, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  322
    { "VAL4_TXT",         24, COM_USER_COLUMN, REC_BYTE_F_ASCII,     16, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  336
    { "VAL4",                25, COM_USER_COLUMN, REC_BIN64_SIGNED,      8, TRUE , SQLCHARSETCODE_UNKNOWN, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "" ,NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  354
    { "TEXT",                26, COM_USER_COLUMN, REC_BYTE_F_ASCII,     60, TRUE , SQLCHARSETCODE_UTF8, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT,0},  //  368
    { "VARIABLE_INFO",  27, COM_USER_COLUMN, REC_BYTE_V_ASCII,   3000, FALSE, SQLCHARSETCODE_UTF8, 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL,COM_UNKNOWN_DIRECTION_LIT,0}   //  430
};

static const ComTdbVirtTableKeyInfo statsVirtTableKeyInfo[] =
{
  // indexname keyseqnumber tablecolnumber ordering
  {    NULL,          1,            0,            0 , 0, NULL, NULL }
};

//
// Task Definition Block for Stats Function:
//
// -  scanPred_ a scan predicate to select the returned stats tuples.
//
// -  projExpr - to project out the needed output values.
//////////////////////////////////////////////////////////////////////////
class ComTdbStats : public ComTdb
{
  friend class ExStatsTcb;

public:

  // Constructors

  // Default constructor (used in ComTdb::fixupVTblPtr() to extract
  // the virtual table after unpacking.

  ComTdbStats();

  // Constructor used by the generator.
  ComTdbStats(ULng32 tupleLen,
	      ULng32 returnedTuplelen,
	      ULng32 inputTuplelen,
	      ex_cri_desc *criDescParentDown,
	      ex_cri_desc *criDescParentUp,
	      queue_index queueSizeDown,
	      queue_index queueSizeUp,
	      Lng32 numBuffers,
	      ULng32 bufferSize,
	      ex_expr *scanExpr,
	      ex_expr *inputExpr,
	      ex_expr *projExpr,
	      ex_cri_desc *workCriDesc,
	      UInt16 stats_row_atp_index,
	      UInt16 input_row_atp_index
	      );
  
  // This always returns TRUE for now
  Int32 orderedQueueProtocol() const { return -1; };

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(ComTdbStats); }

  // Pack and Unpack routines
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  // For the GUI, Does nothing right now
  void display() const {};

  UInt32 getTupleLength() const { return tupleLen_; };
  UInt32 getReturnedTupleLength() const { return returnedTupleLen_; };
  UInt32 getInputTupleLength() const { return inputTupleLen_; };

  UInt16 getStatsTupleAtpIndex() const { return statsTupleAtpIndex_; };
  UInt16 getInputTupleAtpIndex() const { return inputTupleAtpIndex_; };

  // A predicate to be applied to each tuple contained in the
  // statement whose stats info is being returned.
  inline ex_expr *getScanExpr() const { return scanExpr_;};

  inline ex_expr *getInputExpr() const { return inputExpr_;};

  // Virtual routines to provide a consistent interface to TDB's

  virtual const ComTdb *getChild(Int32 /*child*/) const { return NULL; };

  // numChildren always returns 0 for ComTdbStats
  virtual Int32 numChildren() const { return 0; };

  virtual const char *getNodeName() const { return "EX_STATS"; };

  // numExpressions always returns 2 for ComTdbStats
  virtual Int32 numExpressions() const { return 2; };
  
  // The names of the expressions
  virtual const char * getExpressionName(Int32) const;

  // The expressions themselves
  virtual ex_expr* getExpressionNode(Int32);

  static Int32 getVirtTableNumCols()
  {
    return sizeof(statsVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }

  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)statsVirtTableColumnInfo;
  }
  
  static Int32 getVirtTableNumKeys()
  {
    return sizeof(statsVirtTableKeyInfo)/sizeof(ComTdbVirtTableKeyInfo);
  }

  static ComTdbVirtTableKeyInfo * getVirtTableKeyInfo()
  {
    return (ComTdbVirtTableKeyInfo *)statsVirtTableKeyInfo;
  }

protected:

  ExExprPtr scanExpr_;                                           // 00-07
  ExExprPtr projExpr_;                                           // 08-15

  // A contiguous move expression which when evaluated will place values
  // for the params (Module Name and Statement Pattern) in the paramsTuple_
  // of the TCB for this node.
  ExExprPtr inputExpr_;                                          // 16-23

  // Length of stats tuple to be allocated
  Int32 tupleLen_;                                               // 24-27

  Int32 returnedTupleLen_;                                       // 28-31

  Int32 inputTupleLen_;                                          // 32-35 

  Int32 filler0ComTdbStats_;                                     // 36-39 unused

  ExCriDescPtr workCriDesc_;                                     // 40-47

  // position in workAtp where stats row will be created.
  UInt16 statsTupleAtpIndex_;                                    // 48-49

  // position in workAtp where input row will be created.
  UInt16 inputTupleAtpIndex_;                                    // 50-51

  char fillersComTdbStats_[44];                                  // 52-95 unused

private:
 
  inline Attributes * getAttrModName();
  inline Attributes * getAttrStmtName();
 
};


inline Attributes * ComTdbStats::getAttrModName()
{
  // The moduleName is the first attribute in the tuple.
  return
    workCriDesc_->
      getTupleDescriptor(getInputTupleAtpIndex())->getAttr(0);
};

inline Attributes * ComTdbStats::getAttrStmtName()
{
  // The statement Pattern is the second attribute in the tuple.
  return
    workCriDesc_->
      getTupleDescriptor(getInputTupleAtpIndex())->getAttr(1);
};

#endif


