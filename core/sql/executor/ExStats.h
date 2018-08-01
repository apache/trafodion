/*********************************************************************
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
#ifndef __EX_STATS_H__
#define __EX_STATS_H__

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExStats.h
 * Description:  TCB and work method for STATISTICS stored procedure.
 *               This file also contains the declarations of the
 *               ExStatisticsArea class.
 *               
 * Created:      6/18/1997
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Int64.h"
#include "ComTdb.h"
#include "ex_stdh.h"
#include "ExScheduler.h"
#include "ComTdbStats.h"
#include "ComTdbUdr.h"
#include "ComRtUtils.h"
#include "Globals.h"
#include "SqlStats.h"
#include "ssmpipc.h"
#include "ComCextdecs.h"
#include "ex_tcb.h"
#include "SQLCLIdev.h"
#include "ExExeUtil.h"
#include "ExpLOBstats.h"
#include "ComGuardianFileNameParts.h"
#include "Statement.h"

#include <sys/times.h>

#include "seabed/fs.h"

//////////////////////////////////////////////////////////////////
// classes defined in this file
//////////////////////////////////////////////////////////////////
class ExStatisticsArea;
class ExOperStats;
class ExStatsCounter;
class ExClusterStats;
class ExTimeStats;
class ExFragRootOperStats;
class ExPartitionAccessStats;
class ExHashGroupByStats;
class ExHashJoinStats;
class ExESPStats;
class ExSplitTopStats;
class ExMeasBaseStats;
class ExMeasStats;
class ExSortStats;
class ExUDRStats;
class ExProbeCacheStats;
class ExMasterStats;
class ExRMSStats;
class ExBMOStats;
class ExUDRBaseStats;
class ExFastExtractStats;
class ExStorageEngineStats;

typedef ExStorageEngineStats ExHbaseAccessStats; 
typedef ExStorageEngineStats ExHdfsScanStats; 

//////////////////////////////////////////////////////////////////
// forward classes
//////////////////////////////////////////////////////////////////
class Queue;
class HashQueue;
class NAMemory;
class ComTdb;
class IDInfo;
class ex_tcb;
class SsmpClientMsgStream;

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#ifdef sum
#undef sum
#endif

#define _STATS_RTS_VERSION_R25_1   6
#define _STATS_RTS_VERSION_R25     5
#define _STATS_RTS_VERSION_R23_1   4
#define _STATS_RTS_VERSION_R23     3
#define _STATS_RTS_VERSION_R22     2
#define _STATS_RTS_VERSION         1
#define _STATS_PRE_RTS_VERSION     0

#define _UNINITIALIZED_TDB_ID       9999999
#define _NO_TDB_NAME                "NO_TDB"

const Lng32 StatsCurrVersion = _STATS_RTS_VERSION_R25_1;

//////////////////////////////////////////////////////////////////////
// this class is used to provide some utility methods (like, pack
// and unpack) that are used by all derived classes.
// Any utility virtual or non-virtual methods could be added here.
// DO NOT ADD ANY FIELDS TO THIS CLASS. 
// We don't want to increase the size of any derived class.
//////////////////////////////////////////////////////////////////////
class ExStatsBase
{
public:
  virtual UInt32 pack(char * buffer){return 0;};

  virtual void unpack(const char* &buffer){};

  UInt32 alignedPack(char * buffer);

  void alignedUnpack(const char* &buffer);

  void alignSizeForNextObj(UInt32 &size);

  void alignBufferForNextObj(const char* &buffer);
};

class ExStatsBaseNew : public ExStatsBase
{
public:
  virtual UInt32 pack(char * buffer){return 0;};

  virtual void unpack(const char* &buffer){};

  NAMemory *getHeap() { return heap_; }
  ExStatsBaseNew(NAHeap *heap)
  {
    heap_ = heap;
  }

  ExStatsBaseNew()
  {
    heap_ = NULL;
  }
protected:
  NAMemory * heap_;
};

//////////////////////////////////////////////////////////////////
// class ExStatsCounter
//
// a generic counter used in ExOperStats (and deerived classes).
// ExStatsCounter provides cnt, min, max, avg, sum, and var
//////////////////////////////////////////////////////////////////
class ExStatsCounter {
public:
  ExStatsCounter();

  ~ExStatsCounter(){};

  ExStatsCounter& operator=(const ExStatsCounter &other);

  void addEntry(Int64 value);

  ULng32 entryCnt() const { return entryCnt_; };

  Int64 min() const { return (entryCnt_ ? min_ : 0); }

  Int64 max() const { return (entryCnt_ ? max_ : 0); }

  Int64 sum() const { return sum_; };

  float sum2() const { return sum2_; };

  void merge(ExStatsCounter * other);

  void init();

//////////////////////////////////////////////////////////////////
// the following methods are only used to finalize statistics
// (display them). They are never used in DP2.
//////////////////////////////////////////////////////////////////
  float mean();

  float variance();

private:
  ULng32 entryCnt_;
  Int64 min_;
  Int64 max_;
  Int64 sum_;              // sum of all values
  float sum2_;             // sum of the square of all values
};

//////////////////////////////////////////////////////////////////
// class ExClusterStats
//
// Statistics for operators which use clusters (HJ, HGB,
// materialize)
//////////////////////////////////////////////////////////////////
class ExClusterStats  : public ExStatsBase {
public:
  ExClusterStats();
  ExClusterStats(NABoolean isInner,
		 ULng32 bucketCnt,
		 Int64 actRows,
		 Int64 totalSize,
		 ExStatsCounter hashChains,
		 Int64 writeIOCnt,
		 Int64 readIOCnt);

  ~ExClusterStats(){};

  ExClusterStats& operator=(const ExClusterStats &other);

  void getVariableStatsInfo(char * dataBuffer, char * datalen, Lng32 maxLen);

  void getVariableStatsInfoAggr(char * dataBuffer,
				char * datalen,
				Lng32 maxLen);

//////////////////////////////////////////////////////////////////
// accessors, mutators
//////////////////////////////////////////////////////////////////
// Returning const char *
  inline const char * getInner() const { return ((isInner_) ? "inner" : "outer"); };

  inline ULng32 getBucketCnt() const {return bucketCnt_; };

  inline Int64 getActRows() const {return actRows_; };

  inline Int64 getTotalSize() const {return totalSize_; };

  inline ExStatsCounter getHashChains() const {return hashChains_; };

  inline Int64 getWriteIOCnt() const {return writeIOCnt_; };

  inline Int64 getReadIOCnt() const {return readIOCnt_; };

  inline void setNext(ExClusterStats* i) { next_ = i; };

  inline ExClusterStats* getNext() const {return next_; };

  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

private:
  Lng32 version_;
  NABoolean        isInner_;       // inner or outer Cluster
  ULng32    bucketCnt_;     // number of buckets in this cluster
  Int64            actRows_;       // for this cluster
  Int64            totalSize_;     // total size in bytes
  ExStatsCounter   hashChains_;    // stats about the hash table
  Int64            writeIOCnt_;    // write operations to scratch file
  Int64            readIOCnt_;     // read operations from scratch file
  ExClusterStats * next_;
};

//////////////////////////////////////////////////////////////////
// class ExTimeStats
//
// ExTimeStats collects time information. Real-time, process or 
// thread CPU time.
//////////////////////////////////////////////////////////////////
class ExTimeStats  : public ExStatsBase {
friend class ExOperStats;
friend class ExHashJoinStats;

public:

  ExTimeStats(
            clockid_t clk_id = CLOCK_THREAD_CPUTIME_ID
             );
  
  ~ExTimeStats() {}

  inline ExTimeStats& operator+(const ExTimeStats &other) {
  sumTime_  += other.sumTime_;

  return *this;
}

   inline void incTime(Int64 time)
   { 
     sumTime_ += time;
   }

  // start an active period (record the starting time)
  void start();

  // stop an active period and record the spent time
  // returns increment Time
  Int64 stop();

  inline void reset(); 

  inline Int64 getTime() const { return sumTime_; }

  UInt32 packedLength();

  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);
  void setVersion(Lng32 ver) { version_ = ver; }
  
  Lng32 filterForSEstats(struct timespec currTimespec);
private:
  Lng32 version_;
  // aggregated times (microseconds) over multiple start/stop operations
  // This is computed for the process, but the merge of DP2 stats will
  // add to this also. 
  Int64 sumTime_;

  // start/stop indicator and timestamps of the latest start() operation
  NABoolean isStarted_;
  timespec startTime_;
  clockid_t clockid_;
};


//////////////////////////////////////////////////////////////////
// struct ExOperStatsId
//
// Data members that identify an ExOperStats uniquely within
// a given statement
//////////////////////////////////////////////////////////////////
struct ExOperStatsId
{
  ExFragId fragId_;
  Lng32     tdbId_;
  Lng32     instNum_;
  Lng32     subInstNum_;

  // some methods that make use of this struct like a basic data type easier
  // fragID_ is a type ExFragId.  Set to 0, rather than a negative value
  ExOperStatsId() { fragId_=0; tdbId_=-1; instNum_=-1; subInstNum_=-1; }
  ExOperStatsId(const ExOperStatsId &other) : fragId_(other.fragId_),
    tdbId_(other.tdbId_), instNum_(other.instNum_),
    subInstNum_(other.subInstNum_) {}

  /*
  operator ==(const ExOperStatsId &other) const 
  { return (fragId_ == other.fragId_ && tdbId_ == other.tdbId_ &&
	    instNum_ == other.instNum_ && subInstNum_ == other.subInstNum_); }
  */	    
  NABoolean compare(const ExOperStatsId &other, 
    ComTdb::CollectStatsType cst) const;

 };


// Define the number of retcodes here instead of taking the enum
// value WORK_LAST_RETCODE from file ExScheduler.h. The reason is
// that otherwise we couldn't change WORK_LAST_RETCODE without
// versioning implications, and that would be worse than the
// hard-coded literal here.
#define STATS_WORK_LAST_RETCODE 6
#define STATS_WORK_LAST_RETCODE_PREV 4


///////////////////////////////////////////
// class ExeSEStats
///////////////////////////////////////////
class ExeSEStats  : public ExStatsBase
{
public:
  ExeSEStats() :
    accessedRows_(0),
    usedRows_(0),
    numIOCalls_(0),
    numIOBytes_(0),
    maxIOTime_(0)
    {};

  inline Int64 getAccessedRows() const { return accessedRows_; }
  inline void setAccessedRows(Int64 cnt) { accessedRows_ = cnt; }
  inline void incAccessedRows(Int64 i = 1) { accessedRows_ += i; }

  inline Int64 getUsedRows() const { return usedRows_; }
  inline void setUsedRows(Int64 cnt) { usedRows_ = cnt; }
  inline void incUsedRows(Int64 i = 1) { usedRows_ +=  + i; }

  inline Int64 getNumIOCalls() const { return numIOCalls_; }
  inline void setNumIOCalls(Int64 cnt) { numIOCalls_ = cnt; }
  inline void incNumIOCalls(Int64 i = 1) { numIOCalls_ +=  + i; }

  inline Int64 getNumIOBytes() const { return numIOBytes_; }
  inline void setNumIOBytes(Int64 cnt) { numIOBytes_ = cnt; }
  inline void incNumIOBytes(Int64 i = 1) { numIOBytes_ +=  + i; }

  inline Int64 getMaxIOTime() const { return maxIOTime_; }
  inline void setMaxIOTime(Int64 cnt) { maxIOTime_ = cnt; }
  inline void incMaxIOTime(Int64 i = 1) { maxIOTime_ +=  + i; }

  void init(NABoolean resetDop)
  {
    accessedRows_ = 0;
    usedRows_ = 0;
    numIOCalls_ = 0;
    numIOBytes_ = 0;
    maxIOTime_ = 0;
  }

  UInt32 packedLength();

  UInt32 pack(char * buffer);

  void merge(ExeSEStats * other);
  void merge(ExStorageEngineStats * other);

  void unpack(const char* &buffer);

  void copyContents(ExeSEStats * other);

private:
  Int64 accessedRows_;
  Int64 usedRows_;
  Int64 numIOCalls_;
  Int64 numIOBytes_;
  Int64 maxIOTime_;
};


/////////////////////////////////////////////////////////////////////
// This class keeps track of the statistics related to the
// messages that are sent/received between exe/esp or fs/dp2.
// Used by ExPartitionAccessStats and ExEspStats.
// It contains:
//  -- compile time size estimate of sent/recvd buffers
//  -- number of buffers that are sent/recvd
//  -- Total number of bytes that are sent/recvd. This number
//     could be less than the estimated size if dense buffers are
//     used.
//  -- number of statistics info related bytes that are recvd
//     (this number is included in Total number of bytes sent/recvd
//  -- sent/recvd counters (See ExStatsCounter)
/////////////////////////////////////////////////////////////////////
class ExBufferStats  : public ExStatsBase
{
public:
  ExBufferStats() :
       sendBufferSize_(0),
       recdBufferSize_(0),
       totalSentBytes_(0),
       totalRecdBytes_(0),
       statsBytes_(0)
    {};

  ULng32& sendBufferSize() { return sendBufferSize_; }

  ULng32& recdBufferSize() { return recdBufferSize_; }

  Int64& totalSentBytes() { return totalSentBytes_;}

  Int64& totalRecdBytes() { return totalRecdBytes_;}

  Int64& statsBytes() { return statsBytes_;}

  ExStatsCounter& sentBuffers() { return sentBuffers_;}

  ExStatsCounter& recdBuffers() { return recdBuffers_; }

  void init(NABoolean resetDop)
  {
    totalSentBytes_ = 0;
    totalRecdBytes_ = 0;
    statsBytes_ = 0;

    sentBuffers_.init();
    recdBuffers_.init();
  }

  UInt32 packedLength();

  UInt32 pack(char * buffer);

//////////////////////////////////////////////////////////////////
// merge two ExBufferStats of the same type. Merging accumulated
// the counters of other into this.
//////////////////////////////////////////////////////////////////
  void merge(ExBufferStats * other);

  void unpack(const char* &buffer);

  void copyContents(ExBufferStats * other);

private:
  ULng32 sendBufferSize_;
  ULng32 recdBufferSize_;
  Int64 totalSentBytes_;
  Int64 totalRecdBytes_;

  // number of bytes out of the totalRecdBytes used for statistics data.
  Int64 statsBytes_;

  ExStatsCounter sentBuffers_;       // counts buffers sent and the
                                     // used data bytes in these buffers
  ExStatsCounter recdBuffers_;       // counts buffers received and the
                                     // used data bytes in these buffers

};


//////////////////////////////////////////////////////////////////
// class ExOperStats
//
// ExOperStats collects basic statistics for each tcb. Classes
// derived from ExOperStats collect more operator specific
// statistics
//////////////////////////////////////////////////////////////////
class ExOperStats : public ExStatsBaseNew
{
friend class ExMasterStats;
public:
//////////////////////////////////////////////////////////////////
// StatType enumerates the existing variants of ExOperStats. If
// you want to add  a new type, add it here, too.
//////////////////////////////////////////////////////////////////
  enum StatType {
    EX_OPER_STATS           = SQLSTATS_DESC_OPER_STATS,
    ROOT_OPER_STATS         = SQLSTATS_DESC_ROOT_OPER_STATS,
    PARTITION_ACCESS_STATS  = SQLSTATS_DESC_PARTITION_ACCESS_STATS,
    GROUP_BY_STATS          = SQLSTATS_DESC_GROUP_BY_STATS,
    HASH_JOIN_STATS         = SQLSTATS_DESC_HASH_JOIN_STATS, 
    PROBE_CACHE_STATS       = SQLSTATS_DESC_PROBE_CACHE_STATS,
    ESP_STATS               = SQLSTATS_DESC_ESP_STATS,
    SPLIT_TOP_STATS         = SQLSTATS_DESC_SPLIT_TOP_STATS,
    MEAS_STATS              = SQLSTATS_DESC_MEAS_STATS,
    SORT_STATS              = SQLSTATS_DESC_SORT_STATS,
    UDR_STATS               = SQLSTATS_DESC_UDR_STATS,
    NO_OP                   = SQLSTATS_DESC_NO_OP,
    MASTER_STATS            = SQLSTATS_DESC_MASTER_STATS,
    RMS_STATS               = SQLSTATS_DESC_RMS_STATS,
    BMO_STATS               = SQLSTATS_DESC_BMO_STATS,
    UDR_BASE_STATS          = SQLSTATS_DESC_UDR_BASE_STATS,
    REPLICATE_STATS         = SQLSTATS_DESC_REPLICATE_STATS,
    REPLICATOR_STATS        = SQLSTATS_DESC_REPLICATOR_STATS,
    FAST_EXTRACT_STATS      = SQLSTATS_DESC_FAST_EXTRACT_STATS,
    REORG_STATS             = SQLSTATS_DESC_REORG_STATS,
    SE_STATS                = SQLSTATS_DESC_SE_STATS,
    HDFSSCAN_STATS          = SQLSTATS_DESC_SE_STATS,
    HBASE_ACCESS_STATS      = SQLSTATS_DESC_SE_STATS,
    PROCESS_STATS           = SQLSTATS_DESC_PROCESS_STATS
  };

//////////////////////////////////////////////////////////////////
// constructor, destructor
// Keep a heap pointer for dynamic allocations.
//////////////////////////////////////////////////////////////////
  ExOperStats(NAMemory * heap,
	      StatType statType,
	      ex_tcb *tcb,
	      const ComTdb * tdb);

  // second constructor is used only when unpacking objects from a message
  ExOperStats(NAMemory * heap,
	      StatType statType = EX_OPER_STATS);

  ExOperStats(NAMemory *heap,
              StatType statType,
              ComTdb::CollectStatsType collectStatsType,
              ExFragId fragId,
              Lng32 tdbId,
              Lng32 explainTdbId,
              Lng32 instNum,
              ComTdb::ex_node_type tdbType,
              char *tdbName,
              Lng32 tdbNameLen);
  ExOperStats();

  ~ExOperStats();

//////////////////////////////////////////////////////////////////
// Accessors, mutators
//////////////////////////////////////////////////////////////////


  inline StatType statType() const {return statType_; }


  inline short dop() const {return dop_; }

  inline short subReqType() { return subReqType_; }

   inline void setSubReqType(short subReqType) 
           { subReqType_ = subReqType; }

  inline void restoreDop() { 
     if (statsInTcb())
        dop_ = savedDop_;
  }

  inline void incDop() { 
     dop_++;
     if (statsInTcb())
        savedDop_++;
  }

  inline Lng32 getExplainNodeId() const {return explainNodeId_;}

  inline queue_index getDownQueueSize() const {return allStats.downQueueSize_;}

  inline void setDownQueueSize(queue_index size) {allStats.downQueueSize_ = size; }

  inline queue_index getUpQueueSize() const {return allStats.upQueueSize_;}

  inline void setUpQueueSize(queue_index size) {allStats.upQueueSize_ = size; }

  inline ExStatsCounter& getDownQueueStats() {return allStats.downQueueStats_;}

  inline ExStatsCounter& getUpQueueStats() {return allStats.upQueueStats_;}

  inline Lng32 getParentTdbId() const {return (Lng32) parentTdbId_;}

  inline Lng32 getLeftChildTdbId() const {return (Lng32) leftChildTdbId_;}

  inline Lng32 getRightChildTdbId() const {return (Lng32) rightChildTdbId_;}

  inline Lng32 getNTProcessId() const {return (Lng32) allStats.ntProcessId_;}

  inline void setParentTdbId(Lng32 id) { parentTdbId_ = id; }

  inline void setLeftChildTdbId(Lng32 id) { leftChildTdbId_ = id; }

  inline void setRightChildTdbId(Lng32 id) { rightChildTdbId_ = id; }

  inline void setPertableStatsId(Lng32 id) { pertableStatsId_ = id; }

  inline Lng32 getPertableStatsId() { return pertableStatsId_; }

  inline const ExOperStatsId * getId() const { return &id_; }

  inline Lng32 getFragId() const {return (Lng32) id_.fragId_;}

  inline Lng32 getTdbId() const {return id_.tdbId_;}

  inline Lng32 getInstNum() const {return id_.instNum_;}

  inline Lng32 getSubInstNum() const {return id_.subInstNum_;}

  inline void setSubInstNum(Lng32 n) {id_.subInstNum_ = n;}

  inline char * getTdbName() {return tdbName_;}

  inline ComTdb::ex_node_type getTdbType() const {return tdbType_;}
   
  inline ComTdb::CollectStatsType getCollectStatsType() const 
  { return (ComTdb::CollectStatsType)collectStatsType_;}

  inline void setCollectStatsType(ComTdb::CollectStatsType s)  
  {collectStatsType_ = s;}

  virtual const char * getStatTypeAsString() const {return "EX_OPER_STATS";}; 

  inline Int64 getActualRowsReturned() const { return actualRowsReturned_; }

  inline void setActualRowsReturned(Int64 cnt) { actualRowsReturned_ = cnt; }

  inline void incActualRowsReturned(Int64 i = 1) 
              { actualRowsReturned_ = actualRowsReturned_ + i; }

  inline Int64 getEstimatedRowsReturned() const { return u.estRowsReturned_; }

  inline void setEstimatedRowsReturned(Int64 cnt) { u.estRowsReturned_ = cnt; }

  inline Float32 getEstRowsUsed() const { return u.est.estRowsUsed_; }

  inline void setEstRowsUsed(Float32 cnt) { u.est.estRowsUsed_ = cnt; }

  inline Float32 getEstRowsAccessed() const { return u.est.estRowsAccessed_; }

  inline void setEstRowsAccessed(Float32 cnt) { u.est.estRowsAccessed_ = cnt; }

  inline Int64 getNumberCalls() const { return numberCalls_; }

  inline void setNumberCalls(Int64 cnt) { numberCalls_ = cnt; }

  inline void incNumberCalls() { numberCalls_++; }


//////////////////////////////////////////////////////////////////
// reset all counters in the stats. Init affects only counters
// and does NOT reset the tdbName and similar data members.
//////////////////////////////////////////////////////////////////
  virtual void init(NABoolean resetDop);

//////////////////////////////////////////////////////////////////
// subTaskReturn is used by the scheduler to set the return code
// of a tcb
//////////////////////////////////////////////////////////////////
  void subTaskReturn(ExWorkProcRetcode rc);

//////////////////////////////////////////////////////////////////
// calculate the packed length. This is usually just
// sizeof(*this). If the object has dynamically allocated data
// members (strings, arrays, other obejcts) packedLength() has to
// be adjusted accordingly
//////////////////////////////////////////////////////////////////
  virtual UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  virtual UInt32 pack(char * buffer);

  virtual void unpack(const char* &buffer);

//////////////////////////////////////////////////////////////////
// merge two ExOperStats of the same type. Merging accumulated
// the counters of other into this.
//////////////////////////////////////////////////////////////////
  void merge(ExOperStats * other);

//////////////////////////////////////////////////////////////////
// copies the content of other to this. copyContent does NOT
// merge counters.
//////////////////////////////////////////////////////////////////
  void copyContents(ExOperStats * other);

//////////////////////////////////////////////////////////////////
// allocates a new ExOperStats object on heap and copies this
// to the new object.
//////////////////////////////////////////////////////////////////
  virtual ExOperStats * copyOper(NAMemory * heap);

/////////////////////////////////////////////////////////////////
// cast to more specific ExStatsEntry classes.
//////////////////////////////////////////////////////////////////
  virtual ExMeasStats * castToExMeasStats();

  virtual ExMeasBaseStats * castToExMeasBaseStats();

  virtual ExFragRootOperStats * castToExFragRootOperStats();

  virtual ExPartitionAccessStats * castToExPartitionAccessStats();

  virtual ExProbeCacheStats * castToExProbeCacheStats();

  virtual ExFastExtractStats * castToExFastExtractStats();

  virtual ExHdfsScanStats * castToExHdfsScanStats();

  virtual ExHbaseAccessStats * castToExHbaseAccessStats();

  virtual ExHashGroupByStats * castToExHashGroupByStats();

  virtual ExHashJoinStats * castToExHashJoinStats();

  virtual ExESPStats * castToExESPStats();
  virtual ExSplitTopStats * castToExSplitTopStats();

  virtual ExSortStats * castToExSortStats();

  virtual ExeSEStats * castToExeSEStats()
  {
    return NULL;
  }

  virtual ExUDRStats * castToExUDRStats();

  virtual ExMasterStats * castToExMasterStats();

  virtual ExBMOStats * castToExBMOStats();
  virtual ExUDRBaseStats * castToExUDRBaseStats();

  ExTimeStats *getTimer()
  {
    return &operTimer_;
  }

  inline void incCpuTime(Int64 cpuTime) { };

  NABoolean operator==(ExOperStats * other);

  Int64 getHashData(UInt16 statsMergeType = SQLCLI_SAME_STATS);

//////////////////////////////////////////////////////////////////
// format stats for display
//////////////////////////////////////////////////////////////////

  // return 3 characteristics counters for this operator and a short text
  // identification what the counter means
  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual const char * getTextVal();

  // this method returns the variable part of stats related information.
  // This info is delimited by tokens. Each Stats class redefines this
  // method.
  // It returns data in dataBuffer provided by caller and the length
  // of data(2 bytes short) in location pointed to by datalen.
  // This method is called by ExStatsTcb::work().
  virtual void getVariableStatsInfo(char * dataBuffer,
				    char * datalen,
				    Lng32 maxLen);
  virtual Lng32 getStatsItem(SQLSTATS_ITEM* sqlStats_item);


void setCpuStatsHistory() { return; }

  void setVersion(short version)
    {version_ = version; } 

  short getVersion() { return version_; }

  NABoolean statsInDp2()      
  {
    return (flags_ & STATS_IN_DP2)    != 0; 
  }

  void setStatsInDp2(NABoolean v)      
  {
    (v ? flags_ |= STATS_IN_DP2 : flags_ &= ~STATS_IN_DP2);
  }

  NABoolean statsInEsp()      
  {
    return (flags_ & STATS_IN_ESP)    != 0; 
  }

  void setStatsInEsp(NABoolean v)      
  {
    (v ? flags_ |= STATS_IN_ESP : flags_ &= ~STATS_IN_ESP);
  }

  NABoolean statsInTcb()      
  {
    return (flags_ & STATS_IN_TCB)    != 0; 
  }

  void setStatsInTcb(NABoolean v)      
  {
    (v ? flags_ |= STATS_IN_TCB : flags_ &= ~STATS_IN_TCB);
  }

  NABoolean hasSentMsgIUD()
  {
    return (flags_ & MSG_SENT_IUD) != 0;
  }

  void setHasSentMsgIUD()
  {
    flags_ |= MSG_SENT_IUD;
  }

  void clearHasSentMsgIUD()
  {
    flags_ &= ~MSG_SENT_IUD;
  }

  void initTdbForRootOper();

  void setQueryId(char *queryId, Lng32 queryIdLen) {} 

private:
  enum Flags
  {
    STATS_IN_DP2 = 0x0001,
    STATS_IN_ESP = 0x0002,
    STATS_IN_TCB = 0x0004,
    MSG_SENT_IUD = 0x0008
  };

  short version_;
  short subReqType_;
 
  StatType statType_;
  // Using the implicit gap in the structure to accomodate the new field
  short dop_;
  short savedDop_;
  // this ID is assigned at runtime. It is used to identify
  // the stats area. It never changes once it is assigned, except
  // that in some cases the caller alters the subInstNum_ immediately
  // after the constructor call.
  struct ExOperStatsId id_;
  Lng32 parentTdbId_;
  Lng32 leftChildTdbId_;
  Lng32 rightChildTdbId_;
  Lng32 explainNodeId_;
  Lng32 pertableStatsId_;

  char tdbName_[MAX_TDB_NAME_LEN+1];
  ComTdb::ex_node_type tdbType_;

  UInt16 collectStatsType_;
  UInt16 flags_;

  ExTimeStats operTimer_;

  union
  {
    // optimizer estimate of rows returned by this operator
    Int64 estRowsReturned_;

    struct
    {
      Float32 estRowsAccessed_;
      Float32 estRowsUsed_;
    } est; 
  } u;
    
  // actual rows returned by this operator at runtime
  Int64 actualRowsReturned_;
  // Number of times this operator was called
  Int64 numberCalls_;

  char processNameString_[40]; // PROCESSNAME_STRING_LEN in ComRtUtils.h
  struct
  {
    Lng32 ntProcessId_;
    queue_index downQueueSize_;
    queue_index upQueueSize_;
    ExStatsCounter downQueueStats_;
    ExStatsCounter upQueueStats_; 


    // no filler is strictly needed here, but we pack and send this
    // filler in messages, which may come in handy some day
    Int64 fillerForFutureUse_;

    // counters for the different return codes returned by the work
    // procedure of a tcb. These counters are incremented by the
    // scheduler via subTaskReturn()
    ULng32 retCodes_[STATS_WORK_LAST_RETCODE + 2];
  } allStats;
  
};

class ExBMOStats : public ExOperStats
{
  friend class ExMeasStats;
  friend class ExFragRootOperStats;
public:
  ExBMOStats(NAMemory *heap);
  ExBMOStats(NAMemory *heap, StatType statType);
  ExBMOStats(NAMemory *heap, StatType statType,
			 ex_tcb *tcb,
			 const ComTdb * tdb);
  ExBMOStats(NAMemory *heap,
	      ex_tcb *tcb,
	      const ComTdb * tdb);
  void init(NABoolean resetDop);
  UInt32 packedLength();
  UInt32 pack(char * buffer);
  void unpack(const char* &buffer);
  void deleteMe() { }
  ExOperStats *copyOper(NAMemory * heap);
  void copyContents(ExBMOStats * other);
  virtual const char * getNumValTxt(Int32 i) const;
  virtual Int64 getNumVal(Int32 i) const;
  void getVariableStatsInfo(char * dataBuffer,
			  char * dataLen,
          		  Lng32 maxLen);
  ExBMOStats *castToExBMOStats();
  void merge(ExBMOStats* other);
  Lng32 getStatsItem(SQLSTATS_ITEM* sqlStats_item);
  inline void setScratchBufferBlockSize(Int32 size) 
  { 
    scratchBufferBlockSize_ = size >> 10;
  }
  inline void incSratchFileCount() { scratchFileCount_++; }
  inline void incScratchBufferBlockRead(Int32 count = 1) { scratchBufferBlockRead_ += count; }
  inline void incScratchBufferBlockWritten(Int32 count = 1) { scratchBufferBlockWritten_ += count; }
  inline void incScratchReadCount(Int32 count = 1) { scratchReadCount_ += count; }
  inline void incScratchWriteCount(Int32 count = 1) { scratchWriteCount_ += count; }
  inline void incScratchIOMaxTime(Int64 v) {scratchIOMaxTime_ += v;}
  inline void setSpaceBufferSize(Int32 size)
  {
    spaceBufferSize_ = size >> 10; 
  }
  inline void setSpaceBufferCount(Int32 count) { spaceBufferCount_ = count; }
  inline void updateBMOHeapUsage(NAHeap *heap)
  {
    bmoHeapAlloc_ = (Int32)(heap->getTotalSize() >> 10);
    bmoHeapUsage_ = (Int32)(heap->getAllocSize() >> 10);
    bmoHeapWM_ = (Int32)(heap->getHighWaterMark() >> 10);
    if (bmoHeapAlloc_ > bmoHeapWM_)
       bmoHeapWM_ = bmoHeapAlloc_;
  }
  inline void setScratchOverflowMode(Int16 overflowMode)
  {
    scratchOverflowMode_ = overflowMode;
  }
  inline void setTopN(Int32 size) 
  { 
    topN_ = size;
  }
  inline Int64 getScratchReadCount(void) { return scratchReadCount_; }
  static const char *getScratchOverflowMode(Int16 overflowMode);
  ExTimeStats &getScratchIOTimer() { return timer_; }
  inline void setScratchIOSize(Int64 size) { scratchIOSize_ = size; }
  const char *getBmoPhaseStr();
  inline void setBmoPhase(Int16 phase) { phase_ = phase; }
  inline Int16 getBmoPhase() { return phase_; }
  void resetInterimRowCount() { interimRowCount_ = 0;}
  void incInterimRowCount() { interimRowCount_++; }
private:
  ExTimeStats timer_;
  Int32 bmoHeapAlloc_;
  Int32 bmoHeapUsage_;
  Int32 bmoHeapWM_;
  Int32 spaceBufferSize_;
  Int32 spaceBufferCount_;
  Int32 scratchFileCount_;
  Int32 scratchBufferBlockSize_;
  Int32 scratchBufferBlockRead_;
  Int32 scratchBufferBlockWritten_;
  Int32 scratchIOSize_;
  Int64 scratchReadCount_;
  Int64 scratchWriteCount_;
  Int64 scratchIOMaxTime_;
  Int16 scratchOverflowMode_;   // 0 - disk 1 - SSD
  Int32 topN_;                 // TOPN value
  Int64 interimRowCount_;
  Float32 estMemoryUsage_;
  Int16 phase_;
};


/////////////////////////////////////////////////////////////////
// class ExFragRootOperStats
/////////////////////////////////////////////////////////////////
class ExFragRootOperStats : public ExOperStats {
  friend class ExMeasStats;
public:
ExFragRootOperStats(NAMemory * heap,
		    ex_tcb *tcb,
		    const ComTdb * tdb);

  ExFragRootOperStats(NAMemory * heap);

  ExFragRootOperStats(NAMemory *heap,
                      ComTdb::CollectStatsType collectStatsType,
                      ExFragId fragId,
                      Lng32 tdbId,
                      Lng32 explainNodeId,
                      Lng32 instNum,
                      ComTdb::ex_node_type tdbType,
                      char *tdbName,
                      Lng32 tdbNameLen);


  ~ExFragRootOperStats();

  void init(NABoolean resetDop);
  
  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

  void copyContents(ExFragRootOperStats* other);

  void merge(ExOperStats * other);
  void merge(ExFragRootOperStats* other);
  void merge(ExUDRBaseStats * other);
  void merge(ExBMOStats * other);

  ExOperStats * copyOper(NAMemory * heap);
/////////////////////////////////////////////////////////////////
// accessors, mutators
/////////////////////////////////////////////////////////////////
  inline const SB_Phandle_Type * getPhandle() const     
	{ return (const SB_Phandle_Type *) &phandle_; }

  inline SB_Phandle_Type * getPhandlePtr()              { return &phandle_; }

   bool isFragSuspended() const { return isFragSuspended_; }

   void setFragSuspended(bool s) { isFragSuspended_ = s; }

  inline Int64 getCpuTime() const { return cpuTime_; }

  inline Int64 getLocalCpuTime() const { return localCpuTime_; }

  inline void incCpuTime(Int64 cpuTime) 
  { 
    cpuTime_ += cpuTime;
    localCpuTime_ += cpuTime;
  }

  inline void setCpuTime(Int64 cpuTime) { cpuTime_ = cpuTime; }

  inline Int64 getMaxSpaceUsage() const         { return spaceUsage_; }
 
  inline Int64 getMaxHeapUsage() const          { return heapUsage_; }
  inline Lng32 getStmtIndex() const                  { return stmtIndex_; }
 
  inline void setStmtIndex(Lng32 i)                     { stmtIndex_ = i; }
 
  inline Int64 getTimestamp() const                 { return timestamp_; }
 
  inline void setTimestamp(Int64 t)                    { timestamp_ = t; }

   inline void updateSpaceUsage(Space *space, CollHeap *heap)
   {
    spaceUsage_ = (Int32)(space->getAllocSize() >> 10);
    spaceAlloc_ = (Int32)(space->getTotalSize() >> 10);
    heapUsage_ = (Int32)(heap->getAllocSize() >> 10);
    heapAlloc_ = (Int32)(heap->getTotalSize() >> 10);
    heapWM_ = (Int32)(heap->getHighWaterMark() >> 10);
   }

  ExFragRootOperStats * castToExFragRootOperStats();

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual const char * getTextVal();

  virtual void getVariableStatsInfo(char * dataBuffer,
				    char * datalen,
				    Lng32 maxLen);
  void initHistory();

  Int32 getExecutionCount() const { return executionCount_; }

  inline Int32 getNewprocess()          { return newprocess_; }
  inline void setNewprocess(Int32 n)    { newprocess_ = n; }
  inline void incNewprocess(Int32 n = 1)    { newprocess_ = newprocess_ + n; }

  inline Int64 getNewprocessTime()          { return newprocessTime_; }
  inline void setNewprocessTime(Int64 t)    { newprocessTime_ = t; }
  inline void incNewprocessTime(Int64 t)    { newprocessTime_ = newprocessTime_ + t; }
  
  inline Int64 getEspCpuTime() const { return espCpuTime_; }

  Lng32 getStatsItem(SQLSTATS_ITEM* sqlStats_item);
  void setCpuStatsHistory() 
  { 
    histCpuTime_ = cpuTime_; 
  }

  NABoolean filterForCpuStats();
  void setQueryId(char *queryId, Lng32 queryIdLen)
    {queryId_ = queryId;
     queryIdLen_ = queryIdLen;} 
  char *getQueryId() { return queryId_; }
  Lng32 getQueryIdLen() { return queryIdLen_; }
  inline void incReqMsg(Int64 msgBytes)
  {
    reqMsgCnt_++;
    reqMsgBytes_ += msgBytes;
  }
  inline void incReplyMsg(Int64 msgBytes)
  {
    replyMsgCnt_++;
    replyMsgBytes_ += msgBytes;
  }

  inline Int64 getPagesInUse() { return pagesInUse_; }
  inline void setPagesInUse(Int64 pagesInUse) { pagesInUse_ = pagesInUse; }
  inline void incWaitTime(Int64 incWaitTime) 
  { 
    waitTime_ += incWaitTime; 
    maxWaitTime_ = waitTime_;
  }
  inline Int64 getAvgWaitTime() 
  { 
    if (dop() == 0)
       return -1;
    else
       return waitTime_/ dop();
  }
  inline Int64 getMaxWaitTime() { return maxWaitTime_; }

  NABoolean hdfsAccess()
    { return (flags_ & HDFS_ACCESS) != 0; }
  void setHdfsAccess(NABoolean v)
    { (v ? flags_ |= HDFS_ACCESS : flags_ &= ~HDFS_ACCESS); }	

private:
  enum Flags
  {
    HDFS_ACCESS = 0x0001
  };

  // some heap statistics for the entire fragment instance
  Int32 spaceUsage_;
  Int32 spaceAlloc_;
  Int32 heapUsage_;
  Int32 heapAlloc_;
  Int32 heapWM_;
  Int64 cpuTime_;
  Int16 scratchOverflowMode_;
  Int32 newprocess_;
  Int64 newprocessTime_; 
  Int32 espSpaceUsage_;
  Int32 espSpaceAlloc_;
  Int32 espHeapUsage_;
  Int32 espHeapAlloc_;
  Int32 espHeapWM_;
  Int64 espCpuTime_;
  Int64 histCpuTime_;
  Int64 reqMsgCnt_;
  Int64 reqMsgBytes_;
  Int64 replyMsgCnt_;
  Int64 replyMsgBytes_;
  Int64 pagesInUse_;
  // Helps with cancel escalation.  Local only.  Do not merge.
  Int32 executionCount_;
  Lng32 stmtIndex_;  // Statement index used by Measure
  Int64 timestamp_; // timestamp indicating when the statement executed
                    // (master executor only)
  char *queryId_;
  Lng32 queryIdLen_;
  Int32 scratchFileCount_;
  Int32 spaceBufferSize_;
  Int64 spaceBufferCount_;
  Int32 scratchIOSize_;
  Int64 scratchReadCount_;
  Int64 scratchWriteCount_;
  Int64 interimRowCount_;
  Int64 scratchIOMaxTime_;
  Int64 udrCpuTime_;
  Int32 topN_;
  // process id of this fragment instance (to correlate it with MEASURE data)
  // Also used by logic on runtimestats/CancelBroker.cpp
  SB_Phandle_Type phandle_;
  // This is aggregated only for the process.  It is never merged into or
  // from.
  Int64 localCpuTime_;

  // Set to true and reset to false by the MXSSCP process under the
  // stats global semaphore.  Read by master and ESP EXE without the
  // semaphore.
  bool isFragSuspended_;
  Int64 maxWaitTime_;
  Int64 waitTime_;
  Int64 diffCpuTime_;

  Int32 flags_;
};


/////////////////////////////////////////////////////////////////
// class ExPartitionAccessStats
/////////////////////////////////////////////////////////////////
class ExPartitionAccessStats : public ExOperStats {
public:
  ExPartitionAccessStats(NAMemory * heap,
			 ex_tcb *tcb,
			 ComTdb * tdb,
			 ULng32 bufferSize);

  ExPartitionAccessStats(NAMemory * heap);

  ~ExPartitionAccessStats();

  void init(NABoolean resetDop);

  void copyContents(ExPartitionAccessStats* other);

  void merge(ExPartitionAccessStats* other);

  ExOperStats * copyOper(NAMemory * heap);
  
  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

/////////////////////////////////////////////////////////////////
// accessors, mutators
/////////////////////////////////////////////////////////////////
  inline char * ansiName() const {return ansiName_;}

  inline char * fileName() const {return fileName_;}

  ExBufferStats * bufferStats()
  {
    return &bufferStats_;
  }

  ExPartitionAccessStats * castToExPartitionAccessStats();

  virtual const char * getNumValTxt(Int32 i) const;

  virtual const char * getTextVal();

  virtual Int64 getNumVal(Int32 i) const;
  ExeSEStats * castToExeSEStats()
  {
    return exeSEStats();
  }

  ExeSEStats * exeSEStats()
  {
    return &seStats_;
  }

  inline Int32 getOpens()          { return opens_; }
  inline void setOpens(Int32 o)    { opens_ = o; }
  inline void incOpens(Int32 o = 1)    { opens_ += o; }
  
  inline Int64 getOpenTime()          { return openTime_; }
  inline void setOpenTime(Int64 t)    { openTime_ = t; }
  inline void incOpenTime(Int64 t)    { openTime_ += t; }

  Lng32 getStatsItem(SQLSTATS_ITEM* sqlStats_item);

  virtual void getVariableStatsInfo(char * dataBuffer,
				    char * datalen,
				    Lng32 maxLen);

private:

  ExeSEStats     seStats_;
  ExBufferStats  bufferStats_;

  char * ansiName_;
  char * fileName_;
  Int32 opens_;
  Int64 openTime_; 

};

/////////////////////////////////////////////////////////////////
// class ExProbeCacheStats
/////////////////////////////////////////////////////////////////
class ExProbeCacheStats : public ExOperStats {
public:
  ExProbeCacheStats(NAMemory * heap,
			 ex_tcb *tcb,
			 ComTdb * tdb,
			 ULng32 bufferSize,
                         ULng32 numCacheEntries);

  ExProbeCacheStats(NAMemory * heap);

  ~ExProbeCacheStats(){};

  void init(NABoolean resetDop);

  void merge(ExProbeCacheStats* other);

  void copyContents(ExProbeCacheStats* other);

  ExOperStats * copyOper(NAMemory * heap);
  
  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

/////////////////////////////////////////////////////////////////
// accessors, mutators
/////////////////////////////////////////////////////////////////
  inline void incHit() {++cacheHits_;}

  inline void incMiss() {++cacheMisses_;}

  inline void incCanceledHit() {++canceledHits_;}

  inline void incCanceledMiss() {++canceledMisses_;}

  inline void incCanceledNotStarted() {++canceledNotStarted_;}

  inline void updateLongChain(UInt32 clen) 
  {
    if (clen > longestChain_)
      longestChain_ = clen;
  }
  inline void updateUseCount(UInt32 uc) 
  {
    if (uc > highestUseCount_)
      highestUseCount_ = uc;
  }
  inline void newChain() 
  {
    if (++numChains_ > maxNumChains_)
      maxNumChains_ = numChains_;
  }

  inline void freeChain() 
  {
    --numChains_;
  }

  ULng32 longestChain() const {return longestChain_;}

  ULng32 highestUseCount() const {return highestUseCount_;}

  ULng32 maxNumChains() const {return maxNumChains_;}

  ExProbeCacheStats * castToExProbeCacheStats();

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual void getVariableStatsInfo(char * dataBuffer,
				    char * datalen,
				    Lng32 maxLen);

private:
  ULng32 cacheHits_;
  ULng32 cacheMisses_;
  ULng32 canceledHits_;
  ULng32 canceledMisses_;
  ULng32 canceledNotStarted_;
  ULng32 longestChain_;
  ULng32 numChains_;     // supports maxNumChains_.
  ULng32 maxNumChains_;
  ULng32 highestUseCount_;
  ULng32 bufferSize_;      // static for now.
  ULng32 numCacheEntries_; // static for now.
};

//////////////////////////////////////////////////////////////////
// class ExHashGroupByStats
//////////////////////////////////////////////////////////////////
class ExHashGroupByStats : public ExBMOStats {
public:
  ExHashGroupByStats(NAMemory * heap,
		     ex_tcb *tcb,
		     const ComTdb * tdb);

  ExHashGroupByStats(NAMemory * heap);

  ~ExHashGroupByStats(){};
  
  void init(NABoolean resetDop);

  void copyContents(ExHashGroupByStats* other);

  void merge(ExHashGroupByStats* other);

  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

  ExOperStats * copyOper(NAMemory * heap);

  ExHashGroupByStats * castToExHashGroupByStats();

  ExBMOStats * castToExBMOStats();

  void incPartialGroupsReturned() { partialGroups_++; };

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual void getVariableStatsInfo(char * dataBuffer,
				    char * datalen,
				    Lng32 maxLen);

// just add the pointer as last element to cluster stats list
  void addClusterStats(ExClusterStats* clusterStats);

// make a copy of the clusterstats and add them to the list
  void addClusterStats(ExClusterStats clusterStats);

// delete all the clusterstats
  void deleteClusterStats();

  inline void updMemorySize(Int64 memSize)
                              { if (memSize_ < memSize) memSize_ = memSize; }

  inline void updIoSize(Int64 newSize)
  { ioSize_ = newSize ; }

private:
  Int64 partialGroups_;
  Int64 memSize_;             // max amount of memory used (bytes)
  Int64 ioSize_;              // number of bytes transferred from and to disk
  ULng32 clusterCnt_;
  ExClusterStats* clusterStats_;
  ExClusterStats* lastStat_;
};


//////////////////////////////////////////////////////////////////
// class ExHashJoinStats
//////////////////////////////////////////////////////////////////
class ExHashJoinStats : public ExBMOStats {
public:
  ExHashJoinStats(NAMemory * heap,
		  ex_tcb *tcb,
		  const ComTdb * tdb);

  ExHashJoinStats(NAMemory * heap);

  ~ExHashJoinStats(){};

  void init(NABoolean resetDop);

  void copyContents(ExHashJoinStats* other);

  void merge(ExHashJoinStats* other);

  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

  ExOperStats * copyOper(NAMemory * heap);

  ExHashJoinStats * castToExHashJoinStats();

  ExBMOStats * castToExBMOStats();

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual void getVariableStatsInfo(char * dataBuffer,
				    char * datalen,
				    Lng32 maxLen);

// just add the pointer as last element to cluster stats list
  void addClusterStats(ExClusterStats* clusterStats);

// make a copy of the clusterstats and add them to the list
  void addClusterStats(ExClusterStats clusterStats);
  
// delete all the clusterstats
  void deleteClusterStats();

// set the phase of the HJ so that we charge the right time statistics
// note that we only can increae the phase number (makes sense, right?).
  inline void incPhase() { phase_++; };

  inline void incEmptyChains(ULng32 i = 1) { emptyChains_ += i; };

  inline ULng32 getEmptyChains() const { return emptyChains_; };

  inline void updMemorySize(Int64 memSize)
                              { if (memSize_ < memSize) memSize_ = memSize; }

  inline void updIoSize(Int64 newSize)
  { ioSize_ = newSize ; }

  inline void incrClusterSplits() { clusterSplits_++; }
  inline void incrHashLoops() { hashLoops_++; }

private:
  ExTimeStats phaseTimes_[3];
  short phase_;               // indicates which phase to charge with times
  Int64 memSize_;             // max. amount of memory bytes used
  Int64 ioSize_;              // number of bytes transferred from and to disk
  ULng32 emptyChains_;
  ULng32 clusterCnt_;
  ULng32 clusterSplits_; // how many times clusters were split
  ULng32 hashLoops_; // how many hash loops were performed
  ExClusterStats* clusterStats_;
  ExClusterStats* lastStat_;
};


//////////////////////////////////////////////////////////////////
// class ExESPStats
//////////////////////////////////////////////////////////////////
class ExESPStats : public ExOperStats {
public:
  ExESPStats(NAMemory * heap,
	     ULng32 sendBufferSize,
	     ULng32 recBufferSize,
	     Lng32 subInstNum,
	     ex_tcb *tcb,
	     const ComTdb * tdb);

  ExESPStats(NAMemory * heap);

  ~ExESPStats(){};

  void init(NABoolean resetDop);

  void copyContents(ExESPStats* other);

  void merge(ExESPStats* other);
  
  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

  ExOperStats * copyOper(NAMemory * heap);

  inline Int64 getSendTopStatID() const { return sendTopStatID_; }

  inline void setSendTopStatID(Int64 id) { sendTopStatID_ = id; }

  ExBufferStats * bufferStats()
  {
    return &bufferStats_;
  }

  ExESPStats * castToExESPStats();

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual void getVariableStatsInfo(char * dataBuffer,
				    char * datalen,
				    Lng32 maxLen);

private:
  ExBufferStats  bufferStats_;

  Int64 sendTopStatID_;              // used for send bottoms to make
				     // the connection to the corresponding
				     // send tops
};


//////////////////////////////////////////////////////////////////
// class ExSplitTopStats
//////////////////////////////////////////////////////////////////
class ExSplitTopStats : public ExOperStats {
public:
  ExSplitTopStats(NAMemory * heap,
		   ex_tcb *tcb,
		   const ComTdb * tdb);

  ExSplitTopStats(NAMemory * heap);

  ~ExSplitTopStats(){};

  void init(NABoolean resetDop);

  void merge(ExSplitTopStats* other);
  
  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

  ExOperStats * copyOper(NAMemory * heap);

  void copyContents(ExSplitTopStats* other);

  inline ULng32 getMaxChildren() const { return maxChildren_; }

  inline ULng32 getActChildren() const { return actChildren_; }

  ExSplitTopStats * castToExSplitTopStats();

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual void getVariableStatsInfo(char * dataBuffer,
				    char * datalen,
				    Lng32 maxLen);

  NABoolean isPapa() { return (flags_ & IS_PAPA) != 0; }
  
private:
  enum
  {
    // PAPA or EspExchange
    IS_PAPA = 0x0001
  };

  // this is the maximum number of children(SendTop or PA) that 
  // this operator will communicate with.
  ULng32 maxChildren_;

  // this is the actual number of children(SendTop or PA) that this 
  // operator communicated with during query execution.
  ULng32 actChildren_;

  ULng32 flags_;
};


//////////////////////////////////////////////////////////////////
// class ExSortStats
//////////////////////////////////////////////////////////////////
class ExSortStats : public ExBMOStats {
public:
  ExSortStats(NAMemory * heap,
	      ex_tcb *tcb,
	      const ComTdb * tdb);

  ExSortStats(NAMemory * heap);

  ~ExSortStats(){};
  
  void init(NABoolean resetDop);

  void copyContents(ExSortStats* other);

  void merge(ExSortStats* other);

  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

  ExOperStats * copyOper(NAMemory * heap);

  ExSortStats * castToExSortStats();

  ExBMOStats * castToExBMOStats();

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual void getVariableStatsInfo(char * dataBuffer,
				    char * datalen,
				    Lng32 maxLen);

  UInt32& runSize() {return runSize_;}
  UInt32& numRuns() {return numRuns_;}
  UInt32& numCompares() {return numCompares_;}
  UInt32& numDupRecs() {return numDupRecs_;}

  UInt32& scrBlockSize() {return scrBlockSize_;}
  UInt32& scrNumBlocks() {return scrNumBlocks_;}
  UInt32& scrNumWrites() {return scrNumWrites_;}
  UInt32& scrNumReads() {return scrNumReads_;}
  UInt32& scrNumAwaitio() {return scrNumAwaitio_;}

private:
  // for explanation of these sort stats, see sort/Statistics.h
  UInt32 runSize_;          
  UInt32 numRuns_;
  UInt32 numCompares_; 
  UInt32 numDupRecs_;

  UInt32 scrBlockSize_;
  UInt32 scrNumBlocks_;
  UInt32 scrNumWrites_;
  UInt32 scrNumReads_; 
  UInt32 scrNumAwaitio_;
};

/////////////////////////////////////////////////////////////////
//// class ExStorageEngineStats
///////////////////////////////////////////////////////////////////

class ExStorageEngineStats : public ExOperStats {
public:
  ExStorageEngineStats(NAMemory * heap,
                          ex_tcb *tcb,
                          ComTdb * tdb);  // tbd - other, specific params?

  ExStorageEngineStats(NAMemory * heap);

  ~ExStorageEngineStats();

  void init(NABoolean resetDop);

  void merge(ExStorageEngineStats* other);

  void copyContents(ExStorageEngineStats* other);

  ExOperStats * copyOper(NAMemory * heap);
  
  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
//// packs 'this' into a message. Converts pointers to offsets.
////////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

/////////////////////////////////////////////////////////////////
//// accessors, mutators
///////////////////////////////////////////////////////////////////
char * tableName() const {return tableName_;}

  ExTimeStats &getHdfsTimer() { return timer_; }
  ExTimeStats &getHbaseTimer() { return timer_; }

  inline void incBytesRead(Int64 bytesRead) {numBytesRead_ += bytesRead;}

  inline void incAccessedRows() {++accessedRows_;}
  inline void incAccessedRows(Int64 v) {accessedRows_ += v;}

  inline void incUsedRows() {++usedRows_;}
  inline void incUsedRows(Int64 v) {usedRows_ += v;}

  inline void incMaxHdfsIOTime(Int64 v) {maxIOTime_ += v;}
  inline void incMaxHbaseIOTime(Int64 v) {maxIOTime_ += v;}

  Int64 numBytesRead() const {return numBytesRead_;}
  Int64 rowsAccessed() const {return accessedRows_;}
  Int64 rowsUsed() const {return usedRows_;}

  NABoolean filterForSEstats(struct timespec currTimespec, Lng32 filter);

  Int64 maxHdfsIOTime() const {return maxIOTime_;}
  Int64 maxHbaseIOTime() const {return maxIOTime_;}

  ExHdfsScanStats * castToExHdfsScanStats();

  ExHbaseAccessStats * castToExHbaseAccessStats();

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual void getVariableStatsInfo(char * dataBuffer,
                                    char * datalen,
                                    Lng32 maxLen);

  Lng32 getStatsItem(SQLSTATS_ITEM* sqlStats_item);
  
  void setQueryId(char *queryId, Lng32 queryIdLen)
    {queryId_ = queryId;
     queryIdLen_ = queryIdLen;} 
  char *getQueryId() { return queryId_; }
  Lng32 getQueryIdLen() { return queryIdLen_; }

  inline void incHdfsCalls() {++numIOCalls_;}
  inline void incHbaseCalls() {++numIOCalls_;}

  Int64 hdfsCalls() const {return numIOCalls_;}
  Int64 hbaseCalls() const {return numIOCalls_;}

private:

  ExTimeStats timer_;
  char * tableName_;
  Int64  numBytesRead_;
  Int64  accessedRows_;
  Int64  usedRows_;
  Int64  numIOCalls_;
  Int64  maxIOTime_;
  char *queryId_;
  Lng32 queryIdLen_;
  Lng32 blockTime_;
};

/////////////////////////////////////////////////////////////////
// class ExMeasBaseStats 
/////////////////////////////////////////////////////////////////
class ExMeasBaseStats : public ExOperStats {
public:
  ExMeasBaseStats(NAMemory * heap,
		  StatType statType,
	          ex_tcb * tcb,
		  const ComTdb * tdb);

  ExMeasBaseStats(NAMemory * heap, StatType statType);

  ~ExMeasBaseStats(){};

  virtual void init(NABoolean resetDop);
  
  virtual UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  virtual UInt32 pack(char * buffer);

  virtual void unpack(const char* &buffer);

  void merge(ExMeasBaseStats* other);

  void copyContents(ExMeasBaseStats* other);

  virtual ExeSEStats * castToExeSEStats()
  {
    return exeSEStats();
  }

  ExeSEStats * exeSEStats()
  {
    return &seStats_;
  }

  virtual void getVariableStatsInfo(char * dataBuffer, char * datalen, 
				    Lng32 maxLen);

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  void setVersion(Lng32 version);
/*
inline Int32 getOpens()          { return opens_; }
inline void setOpens(Int32 o)    { opens_ = o; }
inline void incOpens(Int32 o = 1)    { opens_ = opens_ + o; }

inline Int64 getOpenTime()          { return openTime_; }
inline void setOpenTime(Int64 t)    { openTime_ = t; }
inline void incOpenTime(Int64 t)    { openTime_ = openTime_ + t; }
*/
  ExMeasBaseStats * castToExMeasBaseStats();

protected:
/* 
  FsDp2MsgsStats fsDp2MsgsStats_;
  ExeDp2Stats exeDp2Stats_;

  UInt16 filler1_;
  Int32 opens_;
  Int64 openTime_; 
*/
  ExeSEStats seStats_;
};


/////////////////////////////////////////////////////////////////
// class ExMeasStats 
/////////////////////////////////////////////////////////////////
class ExMeasStats : public ExMeasBaseStats {
public:
  ExMeasStats(NAMemory * heap,
	      ex_tcb * tcb,
	      const ComTdb * tdb);

  ExMeasStats(NAMemory * heap);

  ~ExMeasStats();

  ExMeasStats * castToExMeasStats();

  void init(NABoolean resetDop);
  
  UInt32 packedLength();



//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

  void merge(ExOperStats * other);
  void merge(ExMeasStats* other);
  void merge(ExFragRootOperStats* other);
  void merge(ExUDRBaseStats * other);
  void merge(ExBMOStats * other);
  void merge(ExStorageEngineStats * other);

  ExOperStats * copyOper(NAMemory * heap);

  void copyContents(ExMeasStats *other);

  virtual void getVariableStatsInfo(char * dataBuffer, char * datalen, 
				    Lng32 maxLen);
  void updateSpaceUsage(Space *space,
         	   CollHeap *heap);

inline Int32 getNewprocess()          { return newprocess_; }
inline void setNewprocess(Int32 n)    { newprocess_ = n; }
inline void incNewprocess(Int32 n = 1)    { newprocess_ = newprocess_ + n; }

inline Int64 getNewprocessTime()          { return newprocessTime_; }
inline void setNewprocessTime(Int64 t)    { newprocessTime_ = t; }
inline void incNewprocessTime(Int64 t)    { newprocessTime_ = newprocessTime_ + t; }
  inline Int32 getTimeouts()          { return timeouts_; }
  inline void setTimeouts(Int32 t)    { timeouts_ = t; }
  virtual const char * getNumValTxt(Int32 i) const;

inline Int32 getNumSorts()          { return numSorts_; }
inline void setNumSorts(Int32 n)    { numSorts_ = n; }
inline void incNumSorts(Int32 n = 1)    { numSorts_ = numSorts_ + n; }

inline Int64 getSortElapsedTime()          { return sortElapsedTime_; }
inline void setSortElapsedTime(Int64 t)    { sortElapsedTime_ = t; }
inline void incSortElapsedTime(Int64 t)    { sortElapsedTime_ = sortElapsedTime_ + t; }

  virtual Int64 getNumVal(Int32 i) const;

  void initHistory();

  bool isFragSuspended() const { return isFragSuspended_; }

  void setFragSuspended(bool s) { isFragSuspended_ = s; }

  inline Int64 getCpuTime() const { return cpuTime_; }

  inline Int64 getLocalCpuTime() const { return localCpuTime_; }

  inline void incCpuTime(Int64 cpuTime) 
  {
    cpuTime_ += cpuTime;
    localCpuTime_ += cpuTime;
  }

  inline void setCpuTime(Int64 cpuTime) { cpuTime_ = cpuTime; }

  Lng32 getStatsItem(SQLSTATS_ITEM* sqlStats_item);
  void setCpuStatsHistory() { histCpuTime_ = cpuTime_; }
  NABoolean filterForCpuStats();
  void setQueryId(char *queryId, Lng32 queryIdLen)
  {
    queryId_ = queryId;
    queryIdLen_ = queryIdLen;
  } 
  char *getQueryId() { return queryId_; }
  Lng32 getQueryIdLen() { return queryIdLen_; }
  inline void incReqMsg(Int64 msgBytes)
  {
    reqMsgCnt_++;
    reqMsgBytes_ += msgBytes;
  }
  inline void incReplyMsg(Int64 msgBytes)
  {
    replyMsgCnt_++;
    replyMsgBytes_ += msgBytes;
  }
  Int32 getExecutionCount() const { return executionCount_; }

  inline const SB_Phandle_Type * getPhandle() const     
        { return (const SB_Phandle_Type *)&phandle_; }

private:

  Int32 newprocess_;
  Int64 newprocessTime_; 
  Int32 timeouts_;
  Int32 numSorts_;
  Int64 sortElapsedTime_;
  // some heap statistics for the entire fragment instance
  Int32 spaceUsage_;
  Int32 spaceAlloc_;
  Int32 heapUsage_;
  Int32 heapAlloc_;
  Int32 heapWM_;
  Int64 cpuTime_;
  Int32 espSpaceUsage_;
  Int32 espSpaceAlloc_;
  Int32 espHeapUsage_;
  Int32 espHeapAlloc_;
  Int32 espHeapWM_;
  Int64 espCpuTime_;
  Int64 histCpuTime_;
  char *queryId_;
  Lng32 queryIdLen_;
  Int64 reqMsgCnt_;
  Int64 reqMsgBytes_;
  Int64 replyMsgCnt_;
  Int64 replyMsgBytes_;
  // Helps with cancel escalation.  Local only.  Do not merge.
  Int32 executionCount_;
  // Used by logic on runtimestats/CancelBroker.cpp (cancel escalation).
  // Local copy, do not merge.
  SB_Phandle_Type phandle_;
  // Set to true and reset to false by the MXSSCP process under the
  // stats global semaphore.  Read by master and ESP EXE without the
  // semaphore.
  bool isFragSuspended_;

  Int64 localCpuTime_;
  Int16 scratchOverflowMode_;
  Int32 scratchFileCount_;
  Int32 spaceBufferSize_;
  Int64 spaceBufferCount_;
  Int64 scratchReadCount_;
  Int64 scratchWriteCount_;
  Int64 udrCpuTime_;
  Int32 topN_;
  Int32 scratchIOSize_;
  Int64 interimRowCount_;
  Int64 scratchIOMaxTime_;
};

class ExUDRBaseStats : public ExOperStats
{
friend class ExMeasStats;
friend class ExFragRootOperStats;
public:
  ExUDRBaseStats(NAMemory *heap);
  ExUDRBaseStats(NAMemory *heap, StatType statType);
  ExUDRBaseStats(NAMemory *heap, StatType statType,
			 ex_tcb *tcb,
			 const ComTdb * tdb);
  ExUDRBaseStats(NAMemory *heap,
	      ex_tcb *tcb,
	      const ComTdb * tdb);
  void init(NABoolean resetDop);
  UInt32 packedLength();
  UInt32 pack(char * buffer);
  void unpack(const char* &buffer);
  void deleteMe() { }
  ExOperStats *copyOper(NAMemory * heap);
  void copyContents(ExUDRBaseStats * other);
  void getVariableStatsInfo(char * dataBuffer,
			  char * dataLen,
          		  Lng32 maxLen);
  ExUDRBaseStats *castToExUDRBaseStats();
  void merge(ExUDRBaseStats* other);
  Lng32 getStatsItem(SQLSTATS_ITEM* sqlStats_item);
  void setUDRServerId(const char * serverId, Lng32 maxlen);
  inline const char * getUDRServerId() const { return UDRServerId_; }
  void incReplyMsg(Int64 msgBytes)
  {
    replyMsgCnt_++;
    replyMsgBytes_ += msgBytes;
    recentReplyTS_ = NA_JulianTimestamp();
  }

  void incReqMsg(Int64 msgBytes)
  {
    reqMsgCnt_++;
    reqMsgBytes_ += msgBytes;
    recentReqTS_ = NA_JulianTimestamp();
  }

private:
  Int64 reqMsgCnt_;
  Int64 reqMsgBytes_;
  Int64 replyMsgCnt_;
  Int64 replyMsgBytes_;
  Int64 udrCpuTime_;
  char UDRServerId_[40]; // PROCESSNAME_STRING_LEN in ComRtUtils.h
  Int64 recentReqTS_;
  Int64 recentReplyTS_;
};

//////////////////////////////////////////////////////////////////
// class ExUDRStats 
//////////////////////////////////////////////////////////////////

class ExUDRStats : public ExUDRBaseStats {
public:
  ExUDRStats(NAMemory * heap,
             ULng32 sendBufferSize,
             ULng32 recBufferSize, 
             const ComTdb * tdb,
             ex_tcb * tcb);

  ExUDRStats(NAMemory * heap);

  ~ExUDRStats();

  void init(NABoolean resetDop);

  void copyContents(ExUDRStats* other);

  void merge(ExUDRStats* other);

  ExOperStats * copyOper(NAMemory * heap);

  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////

  UInt32 pack(char *buffer);

  void unpack(const char * &buffer);

/////////////////////////////////////////////////////////////////
// accessors, mutators
/////////////////////////////////////////////////////////////////
  ExBufferStats * bufferStats() { return &bufferStats_; }

  inline ExStatsCounter& getSentControlBuffers() { return sentControlBuffers_; }

  inline ExStatsCounter& getSentContinueBuffers() { return sentContinueBuffers_; }

  inline void setUDRServerInit(Int64 i = 0) { UDRServerInit_ = i; }

  inline void setUDRServerStart(Int64 s = 0) { UDRServerStart_ = s; } 

  inline const char * getUDRName() const { return UDRName_; }

  void setUDRName(const char * udrName, Lng32 maxlen);

  ExUDRStats * castToExUDRStats();

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual const char * getTextVal();

  virtual void getVariableStatsInfo(char * dataBuffer, char * datalen, Lng32 maxLen);

private:
  ExBufferStats bufferStats_;             // request and reply buffer statistics
  ExStatsCounter sentControlBuffers_;     // counts control msg buffers and bytes
  ExStatsCounter sentContinueBuffers_;    // counts continue msg buffers and bytes

  Int64 UDRServerInit_;                   // UDR Server initialization time 
  Int64 UDRServerStart_;                  // UDR Server successfull start time
  char * UDRName_;                        // UDR name registered in CREATE
};


///////////////////////////////////////////////////////////////////
// class ExStatisticsArea
//
// ExStatisticsArea is a list (or more precisely a hash queue) of
// ExOperStats
///////////////////////////////////////////////////////////////////

class ExStatisticsArea : public IpcMessageObj {
public:

  ExStatisticsArea(NAMemory * heap = NULL, Lng32 sendBottomNum = 0,
		   ComTdb::CollectStatsType cst = ComTdb::ALL_STATS,
                   ComTdb::CollectStatsType origCst = ComTdb::NO_STATS);
  ~ExStatisticsArea();

  void removeEntries();

//////////////////////////////////////////////////////////////////
// Accessors, mutators
//////////////////////////////////////////////////////////////////
  inline void setHeap(NAMemory * heap) { heap_ = heap; }

  void initEntries();

  void restoreDop();

  Lng32 numEntries();


  inline NAMemory *getHeap() { return heap_; }

  inline NABoolean sendStats() {
    sendBottomCnt_++;
    return (sendBottomCnt_ == sendBottomNum_);
}

void allocDynamicStmtCntrs(const char* stmtName);

//////////////////////////////////////////////////////////////////
// Merges statistics from 'stats' to the the corresponding
// entry in the entries_. Searches for matching statID.
// Returns TRUE, if matching entry found. FALSE, if not found.
//////////////////////////////////////////////////////////////////
  NABoolean merge(ExOperStats * stats, UInt16 statsMergeType = SQLCLIDEV_SAME_STATS);

//////////////////////////////////////////////////////////////////
// merges all entries of statArea. If entries in statArea
// are not present in 'this', appends the new entries.
//////////////////////////////////////////////////////////////////
  NABoolean merge(ExStatisticsArea * otherStatArea, UInt16 statsMergeType = SQLCLIDEV_SAME_STATS);

//////////////////////////////////////////////////////////////////
// inserts a new entry, stats, to 'this'
//////////////////////////////////////////////////////////////////
  NABoolean insert(ExOperStats * stats);

//////////////////////////////////////////////////////////////////
// positions to the head of ExOperStats list
//////////////////////////////////////////////////////////////////
  void position();

//////////////////////////////////////////////////////////////////
// positions to the entry with the hash data passed in.
//////////////////////////////////////////////////////////////////
  void position(char * hashData, Lng32 hashDatalen);

//////////////////////////////////////////////////////////////////
// get the next entry
//////////////////////////////////////////////////////////////////
  ExOperStats * getNext();

 
  ExOperStats * get(const ExOperStatsId & id);
 

// gets the next 'type' of stat entry with 'operType'
  ExOperStats * get(ExOperStats::StatType type,
		    ComTdb::ex_node_type tdbType);
 
// gets the next 'type' of stat entry with 'tdbId'
  ExOperStats * get(ExOperStats::StatType type,
		    Lng32 tdbId);

// gets the next stat entry with 'tdbId'
  ExOperStats * get(Lng32 tdbId);

  IpcMessageObjSize packedLength();


//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);

  IpcMessageObjSize packSmallObjIntoMessage(IpcMessageBufferPtr buffer);

  void unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer);

  void unpackObjFromEid(IpcConstMessageBufferPtr buffer,
			ExOperStats *parentStatsEntry = NULL,
                        Lng32 parentTdb = -1);

  void unpackSmallObjFromEid(IpcConstMessageBufferPtr buffer,
			     Lng32 version = _STATS_PRE_RTS_VERSION);

  Int64 getExplainPlanId() const { return explainPlanId_; }

  void setExplainPlanId(Int64 pid) { explainPlanId_ = pid; }

  ComTdb::CollectStatsType getCollectStatsType() { return (ComTdb::CollectStatsType)collectStatsType_; };
  void setCollectStatsType(ComTdb::CollectStatsType s) 
  { collectStatsType_ = s; };

  ComTdb::CollectStatsType getOrigCollectStatsType() 
  { return (ComTdb::CollectStatsType)origCollectStatsType_; };

//////////////////////////////////////////////////////////////////
// scan all ExOperStats entries and update stmtCntrs.
//////////////////////////////////////////////////////////////////
 
Int32 updateStmtCntrs(ExMeasStmtCntrs * stmtCntrs,
				      Int32 statementCount,
				      char *moduleName, Int32 moduleNameLen);
	
 

//////////////////////////////////////////////////////////////////
// get Opens counter from  ExMeasStats entry.
//////////////////////////////////////////////////////////////////
  Int32 getMeasOpensCntr();
  Int64 getMeasOpenTimeCntr();

//////////////////////////////////////////////////////////////////
// get Newprocess counter from  ExMeasStats entry.
//////////////////////////////////////////////////////////////////
  Int32 getMeasNewprocessCntr();
  Int64 getMeasNewprocessTimeCntr();

  NABoolean statsInDp2()      { return (detailedFlags_.smallFlags_ & STATS_IN_DP2)    != 0; }

  void setStatsInDp2(NABoolean v)      
  { (v ? detailedFlags_.smallFlags_ |= STATS_IN_DP2 : detailedFlags_.smallFlags_ &= ~STATS_IN_DP2); }

  NABoolean statsInEsp()      { return (detailedFlags_.smallFlags_ & STATS_IN_ESP)    != 0; }

  void setStatsInEsp(NABoolean v)      
  { (v ? detailedFlags_.smallFlags_ |= STATS_IN_ESP : detailedFlags_.smallFlags_ &= ~STATS_IN_ESP); }

  NABoolean smallStatsObj() { return (detailedFlags_.smallFlags_ & SMALL_STATS_OBJ) != 0; }

  void setSmallStatsObj(NABoolean v)      
  { (v ? detailedFlags_.smallFlags_ |= SMALL_STATS_OBJ : detailedFlags_.smallFlags_ &= ~SMALL_STATS_OBJ);}


  NABoolean statsEnabled()  { return (detailedFlags_.otherFlags_ & STATS_ENABLED)    != 0; }

  void setStatsEnabled(NABoolean v)      
  { (v ? detailedFlags_.otherFlags_ |= STATS_ENABLED : detailedFlags_.otherFlags_ &= ~STATS_ENABLED); }

  NABoolean rtsStatsCollectEnabled()  { return (detailedFlags_.otherFlags_ & RTS_STATS_COLLECT_ENABLED) != 0; }

  void setRtsStatsCollectEnabled(NABoolean v)      
  { (v ? detailedFlags_.otherFlags_ |= RTS_STATS_COLLECT_ENABLED : detailedFlags_.otherFlags_ &= ~RTS_STATS_COLLECT_ENABLED); }

  void setDonotUpdateCounters(NABoolean v)      
  { (v ? detailedFlags_.smallFlags_ |= DONT_UPDATE_COUNTERS : detailedFlags_.smallFlags_ &= ~DONT_UPDATE_COUNTERS); }
  NABoolean donotUpdateCounters()      { return (detailedFlags_.smallFlags_ & DONT_UPDATE_COUNTERS)    != 0; }
  void setMasterStats(ExMasterStats *masterStats);

  ExMasterStats *getMasterStats()
  { return masterStats_; }

  void fixup(ExStatisticsArea *other);
  void updateSpaceUsage(Space *space,
         	   CollHeap *heap);
  void initHistoryEntries();

  ExOperStats *getRootStats() 
  { return rootStats_; };
  void setRootStats(ExOperStats *root)
  {
    rootStats_ = root;
  }
  Lng32 getStatsItems(Lng32 no_of_stats_items,
	    SQLSTATS_ITEM sqlStats_items[]);
  Lng32 getStatsDesc(short *statsCollectType,
	    /* IN/OUT */ SQLSTATS_DESC sqlstats_desc[],
	    /* IN */ 	Lng32 max_stats_desc,
	    /* OUT */	Lng32 *no_returned_stats_desc);
  static const char *getStatsTypeText(short statsType);
  void setCpuStatsHistory();
  NABoolean appendCpuStats(ExStatisticsArea *other, 
         NABoolean appendAlways = FALSE);
  NABoolean appendCpuStats(ExMasterStats *masterStats,
           NABoolean appendAlways,
           short subReqType,
           Lng32 etTimeFilter,
           Int64 currTimestamp);  // Return TRUE if the stats is appended
  NABoolean appendCpuStats(ExOperStats *stat,
           NABoolean appendAlways,
           Lng32 filter,
           struct timespec currTimespec);  // Return TRUE if the stats is appended
  NABoolean appendCpuStats( ExStatisticsArea *stats,
           NABoolean appendAlways,
           Lng32 filter,
           struct timespec currTimespec);  // Return TRUE if the stats is appended
  void incReqMsg(Int64 msgBytes);
  void incReplyMsg(Int64 msgBytes);
  void setDetailLevel(short level) { detailLevel_ = level; }
  void setSubReqType(short subReqType) { subReqType_ = subReqType; }
  short getSubReqType() { return subReqType_; }
  void setQueryId(char *queryId, Lng32 queryIdLen);
  Int64 getHashData(ExOperStats::StatType type,
                                     Lng32 tdbId);

  NABoolean anyHaveSentMsgIUD();

private:
  void unpackThisClass(const char* &buffer,
                        ExOperStats *parentStatsEntry = NULL,
                        Lng32 parentTdb = -1);

  enum SmallFlags
  {
    STATS_IN_DP2 = 0x01,
    SMALL_STATS_OBJ = 0x02,
    STATS_IN_ESP = 0x04,
    DONT_UPDATE_COUNTERS = 0x08 // Don't update counters in the scheduler once the stats is shipped
  };

  enum OtherFlags
  {
    // This flag is set by executor before returning statistics to cli,
    // if stats area was allocated AND stats collection was enabled
    // (that is, statistics were actually collected). 
    // Statistics collection could be enabled or disabled by application.
    // Currently, stats collection is disabled if measure stats was chosen
    // at compile time but measure was not up or stmt counters were not
    // being collected at runtime.
    STATS_ENABLED = 0x0001,
    RTS_STATS_COLLECT_ENABLED = 0x0002
  };

  IDInfo * IDLookup(HashQueue * hq, Int64 id);

  void preProcessStats();

  NAMemory * heap_;
  HashQueue * entries_;

  // some info that relates to the entire statement
  // (used only in the master fragment)
  Int64 explainPlanId_;

  // the following 2 members are only meaningful in an ESP.
  // The split-bottom sets the sendBottomNum_ member. Whenever
  // a send-bottom sees an EOF, it increments the sendBottomCnt_.
  // Statistics are only sent by the "last" send-bottom
  // (sendBottomCnt == sendBottomNum)
  Lng32 sendBottomNum_; 
  Lng32 sendBottomCnt_;

  // ---------------------------------------------------------------------
  // this fields contains details about what
  // kind of statistics are to be collected.
  // ---------------------------------------------------------------------
  UInt16                  collectStatsType_;
  UInt16                  origCollectStatsType_;
  struct DetailedFlags
  {
    unsigned char smallFlags_;
    unsigned char filler;
    UInt16        otherFlags_;
  };
  union
  {
    UInt32 flags_;
    DetailedFlags detailedFlags_;
  };
  
  ExMasterStats *masterStats_;
  ExOperStats   *rootStats_;     // ExOperStats entry that keeps track of memory usage
  short         detailLevel_;    // Detail Level for SQLCLI_QID_DETAIL_STATS
  short         subReqType_;
  char filler_[16];
};


//////////////////////////////////////////////////////////////////
// the following class is used to:
// 1 - map statIDs to more redeable ids
// 2 - build the tree of TCBs for detailed statistics display
// IDInfo is used only during the preparation of the stats for
// display
//////////////////////////////////////////////////////////////////
class IDInfo : public NABasicObject {
public:
  IDInfo(Int64 id, ExOperStats * stat);

  Int64 newId_;
  ExOperStats * stat_;
  // count the number of child TCBs processed during the tree building
  ULng32 childrenProcessed_;
};


// -----------------------------------------------------------------------
// ExStatsTdb
// -----------------------------------------------------------------------
class ExStatsTdb : public ComTdbStats
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExStatsTdb()
  {}

  virtual ~ExStatsTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

private:
  // ---------------------------------------------------------------------
  // !!!!!!! IMPORTANT -- NO DATA MEMBERS ALLOWED IN EXECUTOR TDB !!!!!!!!
  // *********************************************************************
  // The Executor TDB's are only used for the sole purpose of providing a
  // way to supplement the Compiler TDB's (in comexe) with methods whose
  // implementation depends on Executor objects. This is done so as to
  // decouple the Compiler from linking in Executor objects unnecessarily.
  //
  // When a Compiler generated TDB arrives at the Executor, the same data
  // image is "cast" as an Executor TDB after unpacking. Therefore, it is
  // a requirement that a Compiler TDB has the same object layout as its
  // corresponding Executor TDB. As a result of this, all Executor TDB's
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the ComTdbStats instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};


class ExStatsTcb : public ex_tcb
{
public:

  // Constructor called during build phase (ExStatsTdb::build()).
  ExStatsTcb(const ExStatsTdb & statsTdb, ex_globals *glob);

  // Default destructor
  ~ExStatsTcb();  

  // Free resources (Don't know exactly what this should do)
  void  freeResources();

  // The work procedure for ExStatsTcb.
  // For details see ExStats.cpp
  short work();

  // The queue pair used to communicate with the parent TCB
  ex_queue_pair  getParentQueue() const {return qparent_; };

  // A virtual function used by the GUI.  Will always return 0 for
  // ExStatsTcb
  virtual Int32 numChildren() const {return 0;};
  virtual const ex_tcb *getChild(Int32 pos) const { return NULL;} ;
  ExStatisticsArea *sendToSsmp();
  Lng32 parse_stmt_name(char *string, Lng32 len);
  ComDiagsArea * getDiagsArea() { return diagsArea_; }
  
  Lng32 str_parse_stmt_name(char *string, Lng32 len, char *nodeName, short *cpu,       pid_t *pid,Int64 *timeStamp, Lng32 *queryNumber,
       short *qidOffset, short *qidLen, short *activeQueryNum, 
       UInt16 *statsMergeType, short *detailLevel, short *subReqType, 
       Lng32 *filterTimeInSecs);
  enum StatsStep
  {
    INITIAL_, GET_NEXT_STATS_ENTRY_, APPLY_SCAN_EXPR_, PROJECT_, ERROR_, DONE_, SEND_TO_SSMP_,
        GET_MASTER_STATS_ENTRY_,
  };
  void getColumnValues(ExOperStats *stat);

private:
  NABoolean deleteStats()
  {
    return (flags_ & STATS_DELETE_IN_TCB_) != 0; 
  }

  void setDeleteStats(NABoolean v)      
  {
    (v ? flags_ |= STATS_DELETE_IN_TCB_ : flags_ &= ~STATS_DELETE_IN_TCB_); 
  }
  
  enum Flags
  {
    STATS_DELETE_IN_TCB_  = 0x0001,
  };
  // A reference to the corresponding TDB (ExStatsTdb)
  ComTdbStats &statsTdb() const
  {
    return (ComTdbStats &) tdb;
  };

  // Queues used to communicate with the parent TCB.
  ex_queue_pair  qparent_;

  // this is where the stats row will be created.
  tupp tuppData_;
  char * data_;

  atp_struct * workAtp_;

  ExStatisticsArea * stats_;

  char *inputModName_;
  char *inputStmtName_;

  char *qid_;
  pid_t pid_;
  short cpu_;
  Int64 timeStamp_;
  Lng32 queryNumber_;
  char nodeName_[MAX_SEGMENT_NAME_LEN+1];
  short reqType_;
  short retryAttempts_;
  ComDiagsArea * diagsArea_;
  short activeQueryNum_;
  UInt16 statsMergeType_;
  ULng32 flags_;
  short detailLevel_;
  char *stmtName_;
  short subReqType_;
  Lng32 filter_;
};


class ExStatsPrivateState : public ex_tcb_private_state
{
  friend class ExStatsTcb;
  
public:

  ExStatsPrivateState();
  ~ExStatsPrivateState();
  ex_tcb_private_state * allocate_new(const ex_tcb * tcb);

private:

  ExStatsTcb::StatsStep step_;

  Int64 matchCount_; // number of rows returned for this parent row
};


//////////////////////////////////////////////////////////////////
// class ExMasterStats
//////////////////////////////////////////////////////////////////
class ExMasterStats : public ExOperStats
{
public:
  ExMasterStats(NAHeap *heap);
  ExMasterStats(NAHeap *heap, char *sourceStr, Lng32 storedSqlTextLen, Lng32 originalSqlTextLen, char *queryId, Lng32 queryIdLen);
  ExMasterStats();
  ~ExMasterStats();
  UInt32 packedLength();
//////////////////////////////////////////////////////////////////
// packs 'this' into a message.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

  void deleteMe();

  ExOperStats *copyOper(NAMemory * heap);

  void copyContents(ExMasterStats * other);

  void getVariableStatsInfo(char * dataBuffer,
			  char * dataLen,
          		  Lng32 maxLen);

  ExMasterStats *castToExMasterStats();

  void setElapsedStartTime(Int64 elapsedStartTime)
  {
    elapsedStartTime_ = elapsedStartTime;
    firstRowReturnTime_ = -1;
    elapsedEndTime_ = -1;
  }

  void setElapsedEndTime(Int64 elapsedEndTime)
  {
    elapsedEndTime_ = elapsedEndTime;
  }

  void setCompStartTime(Int64 compStartTime)
  {
    compStartTime_ = compStartTime;
    compEndTime_ = -1;
  }

  void setCompEndTime(Int64 compEndTime)
  {
    compEndTime_ = compEndTime;
  }

  void setExeStartTime(Int64 exeStartTime)
  {
    exeStartTime_ = exeStartTime;
    exeEndTime_ = -1;
    canceledTime_ = -1;
    querySuspendedTime_ = -1;
  }

  void setExeEndTime(Int64 exeEndTime)
  {
    exeEndTime_ = exeEndTime;
    exeTimes_.addEntry(exeEndTime_-exeStartTime_);
  }

  void setCanceledTime(Int64 canceledTime)
  {
    canceledTime_ = canceledTime;
  }

  void setFixupStartTime(Int64 fixupStartTime)
  {
    fixupStartTime_ = fixupStartTime;
    fixupEndTime_ = -1;
  }

  void setFixupEndTime(Int64 fixupEndTime)
  {
    fixupEndTime_ = fixupEndTime;
  }

  void setFreeupStartTime(Int64 freeupStartTime)
  {
    freeupStartTime_ = freeupStartTime;
    freeupEndTime_ = -1;
  }

  void setFreeupEndTime(Int64 freeupEndTime)
  {
    freeupEndTime_ = freeupEndTime;
  }

  void setReturnedRowsIOTime(Int64 rrIOtime)
  {
    returnedRowsIOTime_ = rrIOtime;
  }

  void setStmtState(short state)
  {
    stmtState_ = state;
  }

  char * getQueryId() { return queryId_; }
  Lng32   getQueryIdLen() { return queryIdLen_; }

  Int64 getCompStartTime() { return compStartTime_; }

  Int64 getCompEndTime() { return compEndTime_; }
  
  Int64 getElapsedStartTime() { return elapsedStartTime_; }

  Int64 getElapsedEndTime() { return elapsedEndTime_; }

  Int64 getFirstRowReturnTime() { return firstRowReturnTime_; }
  
  Int64 getExeStartTime() { return exeStartTime_; }

  Int64 getExeEndTime() { return exeEndTime_; }

  Int64 getCanceledTime() { return canceledTime_; }

  void setRowsAffected(Int64 rowsAffected)
  {
    rowsAffected_ = rowsAffected;
  }

  void setSqlErrorCode(Lng32 sqlErrorCode)
  {
    sqlErrorCode_ = sqlErrorCode;
  }
  
  Lng32 getSqlErrorCode()
  {
    return sqlErrorCode_;
  }

  void setStatsErrorCode(Lng32 errorCode)
  {
    statsErrorCode_ = errorCode;
  }

  Lng32 getStatsErrorCode()
  {
    return statsErrorCode_;
  }

  Int64 getFixupStartTime() { return fixupStartTime_; }

  Int64 getFixupEndTime() { return fixupEndTime_; }

  Int64 getFreeupStartTime() { return freeupStartTime_; }

  Int64 getFreeupEndTime() { return freeupEndTime_; }

  Int64 getReturnedRowsIOTime() { return returnedRowsIOTime_; }

  Int64 getRowsAffected() { return rowsAffected_; }

  Int32  &numOfTotalEspsUsed() { return numOfTotalEspsUsed_; }
  Int32  &numOfNewEspsStarted() { return numOfNewEspsStarted_; }
  Int32  &numOfRootEsps() { return numOfRootEsps_; }
  short  &exePriority() {return exePriority_;}
  short  &espPriority() {return espPriority_;}
  short  &cmpPriority() {return cmpPriority_;}
  short  &dp2Priority() {return dp2Priority_;}
  short  &fixupPriority() {return fixupPriority_;}
  void  incNumEspsInUse() { numOfTotalEspsUsed_++; }
  inline void setNumCpus(short i) {numCpus_ = i; }
  inline short getNumSqlProcs() { return numOfTotalEspsUsed_+1; }
  inline short getNumCpus() { return numCpus_; }

  inline void setAqrLastErrorCode(Lng32 ec) {aqrLastErrorCode_ = ec;}
  inline Lng32 getAqrLastErrorCode() {return aqrLastErrorCode_;}

  inline void setNumAqrRetries(Lng32 numRetries) {numAqrRetries_ = numRetries;}
  inline Lng32 getNumAqrRetries() {return numAqrRetries_;}

  inline void setAqrDelayBeforeRetry(Lng32 d ) {delayBeforeAqrRetry_ = d;}
  inline Lng32 getAqrDelayBeforeRetry() { return delayBeforeAqrRetry_;}

  short getState() { return stmtState_; }

  void fixup(ExMasterStats *other);

  void setEndTimes(NABoolean updateExeEndTime);

  QueryCostInfo &queryCostInfo() { return queryCostInfo_; }
  void setQueryType(Int16 queryType, Int16 subqueryType)
  { 
    queryType_ = queryType; 
    subqueryType_ = subqueryType;
  }
  Int16 getQueryType() { return queryType_;}

  CompilerStatsInfo &compilerStatsInfo() { return compilerStatsInfo_; }

  NABoolean compilerCacheHit() { return (masterFlags_ & COMPILER_CACHE_HIT) != 0; }
  void setCompilerCacheHit(NABoolean v)
  { (v ? masterFlags_ |= COMPILER_CACHE_HIT : masterFlags_ &= ~COMPILER_CACHE_HIT); }

  NABoolean executorCacheHit() { return (masterFlags_ & EXECUTOR_CACHE_HIT) != 0; }
  void setExecutorCacheHit(NABoolean v)
  { (v ? masterFlags_ |= EXECUTOR_CACHE_HIT : masterFlags_ &= ~EXECUTOR_CACHE_HIT); }

  NABoolean isPrepare() { return (masterFlags_ & IS_PREPARE) != 0; }
  void setIsPrepare(NABoolean v)
  { (v ? masterFlags_ |= IS_PREPARE : masterFlags_ &= ~IS_PREPARE); }

  NABoolean isPrepAndExec() { return (masterFlags_ & IS_PREP_AND_EXEC) != 0; }
  void setIsPrepAndExec(NABoolean v)
  { (v ? masterFlags_ |= IS_PREP_AND_EXEC : masterFlags_ &= ~IS_PREP_AND_EXEC); }

  NABoolean xnReqd() { return (masterFlags_ & XN_REQD) != 0; }
  void setXnReqd(NABoolean v)
  { (v ? masterFlags_ |= XN_REQD : masterFlags_ &= ~XN_REQD); }

  void setSuspendMayHaveAuditPinned(bool v)
    { suspendMayHaveAuditPinned_ = v; }

  bool getSuspendMayHaveAuditPinned()
    { return suspendMayHaveAuditPinned_;}

  void setSuspendMayHoldLock(bool v)
    { suspendMayHoldLock_ = v; }

  bool getSuspendMayHoldLock()
    { return suspendMayHoldLock_ ; }

  bool isReadyToSuspend() const { return (readyToSuspend_ == READY) ; }
  void setReadyToSuspend() { readyToSuspend_ = READY; }
  void setNotReadyToSuspend() { readyToSuspend_ = NOT_READY; }

  void init(NABoolean resetDop);
  void reuse();
  void initBeforeExecute(Int64 currentTimestamp);
  void resetAqrInfo();

  Lng32 getStatsItem(SQLSTATS_ITEM* sqlStats_item);
  void setParentQid(char *queryId, Lng32 queryIdLen);
  char *getParentQid() { return parentQid_; }
  Lng32 getParentQidLen() { return parentQidLen_; }
  void setParentQidSystem(char *parentQidSystem, Lng32 len);
  char *getParentQidSystem() { return parentQidSystem_; }
  Int64 getTransId() { return transId_; }
  void setTransId(Int64 transId) { transId_ = transId; }
  void setChildQid(char *queryId, Lng32 queryIdLen);
  char *getChildQid() { return childQid_; }
  Lng32 getChildQidLen() { return childQidLen_; }
  Lng32 getReclaimSpaceCount() { return reclaimSpaceCount_; }
  void incReclaimSpaceCount() { reclaimSpaceCount_++; }
  NABoolean filterForCpuStats(short subReqType, Int64 currTimestamp, 
                Lng32  etTimeInSecs);
  Int64 getRowsReturned() const { return rowsReturned_; }
  void setRowsReturned(Int64 cnt) { rowsReturned_ = cnt; }
  void incRowsReturned(Int64 i = 1)
  { 
    rowsReturned_ += i;
    if (firstRowReturnTime_ == -1)
      firstRowReturnTime_ = NA_JulianTimestamp();
  }

  bool isQuerySuspended() const { return isQuerySuspended_; }
  void setQuerySuspended(bool s) 
    { 
      isQuerySuspended_ = s;
      if (isQuerySuspended_) 
        querySuspendedTime_ = NA_JulianTimestamp();
    }
  Int64 getQuerySuspendedTime() const { return querySuspendedTime_; }
  char *getCancelComment() const { return cancelComment_; }

  // setCancelComment is called by Control Broker mxssmp under semaphore.
  void setCancelComment(const char *c);

  void setIsBlocking();
  void setNotBlocking();
  Int32 timeSinceBlocking(Int32 seconds);
  Int32 timeSinceUnblocking(Int32 seconds);

  void setValidPrivs(bool v) { validPrivs_ = v; }
  bool getValidPrivs()       { return validPrivs_; }
  // Security Invalidation Keys -- no need to pack or unpack.
  Int32 getNumSIKeys() const { return numSIKeys_; }
  SQL_QIKEY *getSIKeys() const { return sIKeys_; }
  void setInvalidationKeys( CliGlobals *cliGlobals, 
                            SecurityInvKeyInfo *sikInfo, Int32 numObjUIDs,
                            const Int64 *objectUIDs );
  void setValidDDL(bool v) { validDDL_ = v; }
  bool getValidDDL()       { return validDDL_; }
  Int32 getNumObjUIDs() const { return numObjUIDs_; }
  Int64 *getObjUIDs() const { return objUIDs_; }

private:
  enum Flags
  {
    COMPILER_CACHE_HIT = 0x0001,
    EXECUTOR_CACHE_HIT = 0x0002,
    IS_PREPARE         = 0x0004,
    IS_PREP_AND_EXEC   = 0x0008,
    XN_REQD            = 0x0010
  };

  char *sourceStr_;
  Lng32 storedSqlTextLen_;
  Lng32 originalSqlTextLen_;
  char *queryId_;
  Lng32  queryIdLen_;
  Int64 elapsedStartTime_;
  Int64 elapsedEndTime_;
  Int64 firstRowReturnTime_;
  Int64 compStartTime_;
  Int64 compEndTime_;
  Int64 exeStartTime_;
  Int64 exeEndTime_;
  Int64 canceledTime_;
  Int64 fixupStartTime_;
  Int64 fixupEndTime_;
  Int64 freeupStartTime_;
  Int64 freeupEndTime_;
  Int64 returnedRowsIOTime_;
  Int64 rowsAffected_;
  Lng32  sqlErrorCode_;
  Lng32  statsErrorCode_;
  
  short stmtState_;
  UInt16 masterFlags_;

  short numCpus_;

  QueryCostInfo queryCostInfo_;

  Int32 numOfTotalEspsUsed_;
  Int32 numOfNewEspsStarted_;
  Int32 numOfRootEsps_;

  short exePriority_;
  short espPriority_;
  short cmpPriority_;
  short dp2Priority_;
  short fixupPriority_;
  short queryType_;
  short subqueryType_;

  CompilerStatsInfo compilerStatsInfo_;
  char *parentQid_;
  Lng32 parentQidLen_;
  char  parentQidSystem_[24];
  Int64 transId_;
  Int64 rowsReturned_;
  Lng32 aqrLastErrorCode_;
  Lng32 numAqrRetries_;
  Lng32 delayBeforeAqrRetry_;
  char *childQid_;
  Lng32 childQidLen_;
  Lng32 reclaimSpaceCount_;
  bool isQuerySuspended_;
  Int64 querySuspendedTime_;
  Int32 cancelCommentLen_;
  char *cancelComment_;
  // These helpers for suspend/resume are written in the master of the 
  // subject query and they are read by ssmpipc.cpp.
  bool suspendMayHaveAuditPinned_;
  bool suspendMayHoldLock_;
  enum  {
  READY = 1,
  NOT_READY= 2
  } readyToSuspend_;
  struct timeval blockOrUnblockSince_;
  bool isBlocking_;
  Int32 lastActivity_;
  // query priv invalidation
  bool validPrivs_;
  // preallocate sIKeys_ while we have the semaphore.  Can add more later.
  enum { PreAllocatedSikKeys = 
#ifdef _DEBUG
  1
#else
  30 
#endif
       , PreAllocatedObjUIDs =
#ifdef _DEBUG
  1
#else
  20
#endif
  };
  Int32 numSIKeys_;
  SQL_QIKEY * sIKeys_;
  SQL_QIKEY preallocdSiKeys_[PreAllocatedSikKeys];
  bool validDDL_;
  Int32 numObjUIDs_;
  Int64 *objUIDs_;
  Int64  preallocdObjUIDs_[PreAllocatedObjUIDs];
  ExStatsCounter exeTimes_;
};



class ExRMSStats : public ExOperStats
{
public:
  ExRMSStats(NAHeap *heap);
  UInt32 packedLength();
  UInt32 pack(char * buffer);
  void unpack(const char* &buffer);
  void deleteMe() { }
  ExOperStats *copyOper(NAMemory * heap);
  void copyContents(ExRMSStats * other);
  void getVariableStatsInfo(char * dataBuffer,
			  char * dataLen,
          		  Lng32 maxLen);
  ExRMSStats *castToExRMSStats();
  void merge(ExRMSStats* other);
  inline void setRmsVersion(Lng32 version) { rmsVersion_ = version; }
  void setNodeName(char *nodeName);
  inline pid_t getSsmpPid() { return ssmpPid_; }
  inline pid_t getSscpPid() { return sscpPid_; }
  inline Int64 getSsmpTimestamp() { return ssmpTimestamp_; }
  inline void setCpu(short cpu) { cpu_ = cpu; }
  inline void setSscpPid(pid_t pid) { sscpPid_ = pid; }
  inline void setSsmpPid(pid_t pid) { ssmpPid_ = pid; }
  inline void setSscpPriority(short pri) { sscpPriority_ = pri; }
  inline void setSsmpPriority(short pri) { ssmpPriority_ = pri; }
  inline void setStoreSqlSrcLen(short srcLen) { storeSqlSrcLen_ = srcLen; }
  inline void setRmsEnvType(short envType) { rmsEnvType_ = envType; }
  inline void setGlobalStatsHeapAlloc(Int64 size) { currGlobalStatsHeapAlloc_ = size; }
  inline void setGlobalStatsHeapUsed(Int64 size) { currGlobalStatsHeapUsage_ = size;  }
  inline void setStatsHeapWaterMark(Int64 size) {  globalStatsHeapWatermark_ = size; }
  inline void setNoOfStmtStats(Lng32 noOfStmts) { currNoOfStmtStats_ = noOfStmts;  }
  inline void incProcessStatsHeaps() { currNoOfProcessStatsHeap_++;}
  inline void decProcessStatsHeaps() { currNoOfProcessStatsHeap_--; }
  inline void incProcessRegd() { currNoOfRegProcesses_++;}
  inline void decProcessRegd() { currNoOfRegProcesses_--; }
  inline Lng32 getProcessRegd() { return currNoOfProcessStatsHeap_; }
  inline void setSemPid(pid_t pid) { semPid_ = pid; }
  inline void setSscpOpens(short numSscps) { sscpOpens_ = numSscps; }
  inline void setSscpDeletedOpens(short numSscps) { sscpDeletedOpens_ = numSscps;  }
  inline void setSscpTimestamp(Int64 timestamp) {sscpTimestamp_ = timestamp; }
  inline void setSsmpTimestamp(Int64 timestamp) {ssmpTimestamp_ = timestamp; }
  inline void setRMSStatsResetTimestamp(Int64 timestamp) {rmsStatsResetTimestamp_ = timestamp; }
  inline void incStmtStatsGCed(short inc)
  {
    stmtStatsGCed_ = inc;
    totalStmtStatsGCed_ += inc;
  }
  inline Int64 getLastGCTime() { return lastGCTime_; }
  inline void setLastGCTime(Int64 gcTime) { lastGCTime_ = gcTime; }
  inline void incSsmpReqMsg(Int64 msgBytes)
  {
    ssmpReqMsgCnt_++;
    ssmpReqMsgBytes_ += msgBytes;
  }
  inline void incSsmpReplyMsg(Int64 msgBytes)
  {
    ssmpReplyMsgCnt_++;
    ssmpReplyMsgBytes_ += msgBytes;
  }
  inline void incSscpReqMsg(Int64 msgBytes)
  {
    sscpReqMsgCnt_++;
    sscpReqMsgBytes_ += msgBytes;
  }
  inline void incSscpReplyMsg(Int64 msgBytes)
  {
    sscpReplyMsgCnt_++;
    sscpReplyMsgBytes_ += msgBytes;
  }
  inline void setNumQueryInvKeys(Int32 n) { numQueryInvKeys_ = n; }
  inline void setNodesInCluster(short n) { nodesInCluster_ = n; }
  inline void setConfiguredPidMax(pid_t pid) { configuredPidMax_ = pid; }
  Lng32 getStatsItem(SQLSTATS_ITEM* sqlStats_item);
  void reset();
private: 
  Lng32  rmsVersion_;
  char  nodeName_[MAX_SEGMENT_NAME_LEN+1];
  short cpu_;
  pid_t  sscpPid_;
  short sscpPriority_;
  Int64 sscpTimestamp_;
  pid_t  ssmpPid_;
  short ssmpPriority_;
  Int64 ssmpTimestamp_;
  short storeSqlSrcLen_;
  short rmsEnvType_;
  Int64 currGlobalStatsHeapAlloc_;
  Int64 currGlobalStatsHeapUsage_;
  Int64 globalStatsHeapWatermark_;
  Lng32  currNoOfStmtStats_;
  Lng32 currNoOfRegProcesses_;
  Lng32  currNoOfProcessStatsHeap_;
  pid_t semPid_;
  short sscpOpens_;
  short sscpDeletedOpens_;
  short stmtStatsGCed_;
  Int64 lastGCTime_;
  Int64 totalStmtStatsGCed_;
  Int64 ssmpReqMsgCnt_;
  Int64 ssmpReqMsgBytes_;
  Int64 ssmpReplyMsgCnt_;
  Int64 ssmpReplyMsgBytes_;
  Int64 sscpReqMsgCnt_;
  Int64 sscpReqMsgBytes_;
  Int64 sscpReplyMsgCnt_;
  Int64 sscpReplyMsgBytes_;
  Int64 rmsStatsResetTimestamp_;
  Int32 numQueryInvKeys_;
  short nodesInCluster_;
  pid_t configuredPidMax_;
};


//////////////////////////////////////////////////////////////////
// class ExFastExtractStats
//////////////////////////////////////////////////////////////////
class ExFastExtractStats : public ExOperStats {
public:
  ExFastExtractStats(NAMemory * heap,
                     ex_tcb *tcb,
                     const ComTdb * tdb);

  ExFastExtractStats(NAMemory * heap);

  ~ExFastExtractStats(){};

  void init(NABoolean resetDop);

  void copyContents(ExFastExtractStats* other);

  void merge(ExFastExtractStats* other);

  UInt32 packedLength();

//////////////////////////////////////////////////////////////////
// packs 'this' into a message. Converts pointers to offsets.
//////////////////////////////////////////////////////////////////
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

  ExOperStats * copyOper(NAMemory * heap);

  ExFastExtractStats * castToExFastExtractStats();


  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual void getVariableStatsInfo(char * dataBuffer,
                                    char * datalen,
                                    Lng32 maxLen);
  UInt32& buffersCount()  { return buffersCount_;}
  UInt32& processedRowsCount() { return processedRowsCount_;}
  UInt32& readyToSendBuffersCount() { return readyToSendBuffersCount_;}
  UInt32& sentBuffersCount()   { return sentBuffersCount_;}
  UInt32& partitionsCount()    { return partitionNumber_;}
  UInt32& bufferAllocFailuresCount() { return bufferAllocFailuresCount_;}
  void  setBuffersCount(UInt32 v) { buffersCount_ = v;}
  void  setProcessedRowsCount(UInt32 v) {processedRowsCount_ = v;}
  UInt32  getProcessedRowsCount() { return processedRowsCount_ ;}
  void  incProcessedRowsCount() {processedRowsCount_++;}
  void  setErrorRowsCount(UInt32 v) {errorRowsCount_ = v;}
  UInt32  getErrorRowsCount() { return errorRowsCount_ ;}
  void  incErrorRowsCount() {errorRowsCount_++;}
  void  incReadyToSendBuffersCount() {readyToSendBuffersCount_++;}
  void  incSentBuffersCount() { sentBuffersCount_++;}
  void  incReadyToSendBytes( UInt32 v = 1) {readyToSendBytes_+= v;}
  void  incSentBytes( UInt32 v = 1) { sentBytes_+= v;}

  void  setPartitionNumber(UInt32 v) { partitionNumber_ = v;}
  void  incBufferAllocFailuresCount() { bufferAllocFailuresCount_++;}
  void  setBufferAllocFailuresCount(UInt32 v) { bufferAllocFailuresCount_ = v;}

private:

  UInt32 buffersCount_;
  UInt32 processedRowsCount_;
  UInt32 errorRowsCount_;
  UInt32 readyToSendBuffersCount_;
  UInt32 sentBuffersCount_;
  UInt32 partitionNumber_;
  UInt32 bufferAllocFailuresCount_;
  UInt32 readyToSendBytes_;
  UInt32 sentBytes_;
};

///////////////////////////////////////////////////////////////
// class ExProcessStats
//////////////////////////////////////////////////////////////////
class ExProcessStats : public ExOperStats {
public:
  ExProcessStats(NAMemory * heap);
  ExProcessStats(NAMemory * heap, 
                   short nid, pid_t pid);
  ~ExProcessStats()
  {
     if (delQid_)
        NADELETEBASIC(recentQid_, getHeap());
     recentQid_ = NULL;
  };

  void init(NABoolean resetDop);

  void copyContents(ExProcessStats* other);

  void merge(ExProcessStats* other);

  UInt32 packedLength();
  UInt32 pack(char * buffer);

  void unpack(const char* &buffer);

  ExOperStats * copyOper(NAMemory * heap);

  ExProcessStats * castToExProcessStats();

  virtual const char * getNumValTxt(Int32 i) const;

  virtual Int64 getNumVal(Int32 i) const;

  virtual void getVariableStatsInfo(char * dataBuffer,
                                    char * datalen,
                                    Lng32 maxLen);
  virtual const char * getTextVal();
  inline size_t getExeMemHighWM() { return exeMemHighWM_; }
  inline size_t getExeMemAlloc() { return exeMemAlloc_; }
  inline size_t getExeMemUsed() { return exeMemUsed_; }
  inline size_t getIpcMemHighWM() { return ipcMemHighWM_; }
  inline size_t getIpcMemAlloc() { return ipcMemAlloc_; }
  inline size_t getIpcMemUsed() { return ipcMemUsed_; }
  inline short getNid() { return nid_; }
  inline pid_t getPid() { return pid_; }
  inline Int32 getStaticStmtCount() { return staticStmtCount_; }
  inline void incStaticStmtCount() { staticStmtCount_++; }
  inline void decStaticStmtCount() { staticStmtCount_--; }
  inline Int32 getDynamicStmtCount() { return dynamicStmtCount_; }
  inline void incDynamicStmtCount() { dynamicStmtCount_++; }
  inline void decDynamicStmtCount() { dynamicStmtCount_--; }
  inline void incStmtCount(Statement::StatementType stmtType)
  {
    if (stmtType == Statement::STATIC_STMT)
       incStaticStmtCount();
    else
       incDynamicStmtCount();
  }
  inline void decStmtCount(Statement::StatementType stmtType)
  {
    if (stmtType == Statement::STATIC_STMT)
       decStaticStmtCount();
    else
       decDynamicStmtCount();
  }
  inline short getOpenStmtCount() { return openStmtCount_; }
  inline void incOpenStmtCount() { openStmtCount_++; }
  inline void decOpenStmtCount() { openStmtCount_--; }
  inline short getCloseStmtCount() { return closeStmtCount_; }
  inline void incCloseStmtCount() { closeStmtCount_++; }
  inline void decCloseStmtCount() 
  {
    if (closeStmtCount_ > 0)
       closeStmtCount_--;
  }
  inline short getReclaimStmtCount() { return reclaimStmtCount_; }
  inline void incReclaimStmtCount() { reclaimStmtCount_++; }
  inline void decReclaimStmtCount() { reclaimStmtCount_--; }
  inline Int64 getStartTime() { return startTime_; }
  void setStartTime(Int64 startTime) { startTime_ = startTime; }
  inline Int32 getPfsSize() { return pfsSize_; }
  void setPfsSize(Int32 pfsSize) { pfsSize_ = pfsSize; }
  inline Int32 getPfsCurUse() { return pfsCurUse_; } 
  void setPfsCurUse(Int32 pfsCurUse) { pfsCurUse_ = pfsCurUse; }
  inline Int32 getPfsMaxUse() { return pfsMaxUse_; } 
  void setPfsMaxUse(Int32 pfsMaxUse) { pfsMaxUse_ = pfsMaxUse; }
  inline short getNumArkFsSessions() { return arkfsSessionCount_; }
  inline void incArkFsSessionCount() { arkfsSessionCount_++; }
  void decArkFsSessionCount() 
   { 
      if (arkfsSessionCount_ > 0)
         arkfsSessionCount_--; 
   }
  inline short getNumSqlOpens() { return sqlOpenCount_; }
  inline void incNumSqlOpens() { sqlOpenCount_++; }
  void decNumSqlOpens() 
   { 
      if (sqlOpenCount_ > 0)
         sqlOpenCount_--; 
   }
  void updateMemStats(NAHeap *exeHeap, NAHeap *ipcHeap);
  void setRecentQid(char *recentQid) 
  { 
     recentQid_ = recentQid;
     if (recentQid_ != NULL)
        qidLen_ = strlen(recentQid_);
     else
        qidLen_ = 0;
  }
  void setRecentQidToNull(char *recentQid)
  {
    if (recentQid_ == recentQid)
    {
       recentQid_ = NULL;
       delQid_ = FALSE;
       qidLen_ = 0;
    }
  }
  inline char *getRecentQid() { return recentQid_; }
  inline void incStartedEsps() { numESPsStarted_++; }
  inline Int32 getNumESPsStarted() { return numESPsStarted_; }
  inline void incStartupCompletedEsps() 
  { 
     numESPsStartupCompleted_++;
  }
  inline Int32 getNumESPsStartupCompleted() { return numESPsStartupCompleted_; }
  inline void incBadEsps() { numESPsBad_++; }
  inline Int32 getNumESPsBad() { return numESPsBad_; }
  inline void incDeletedEsps() { numESPsDeleted_ ++; }
  inline Int32 getNumESPsDeleted() { return numESPsDeleted_; }
  inline Int32 getNumESPsInUse() { return numESPsInUse_; }
  inline Int32 getNumESPsFree() { return numESPsFree_; }
  inline void incNumESPsInUse(NABoolean gotFromCache) 
  {
     numESPsInUse_++; 
     if (gotFromCache)
        decNumESPsFree();
  }

  void decNumESPsInUse()
  {
     if (numESPsInUse_ > 0)
        numESPsInUse_--;
  }

  inline void decNumESPsFree()
  {
     if (numESPsFree_ > 0)
        numESPsFree_--;
  }
  void decNumESPs(NABoolean wasInUse, NABoolean beingDeleted)
  {
     if (wasInUse)
     {
        decNumESPsInUse();
        if (beingDeleted)
           numESPsDeleted_++;
        else
           numESPsFree_++;
     }
     else
     {
        decNumESPsFree();
        if (beingDeleted)
           numESPsDeleted_++;
     }
  }
 private:
  short  nid_;
  pid_t  pid_;
  size_t exeMemHighWM_;
  size_t exeMemAlloc_;
  size_t exeMemUsed_;
  size_t ipcMemHighWM_;
  size_t ipcMemAlloc_;
  size_t ipcMemUsed_;
  Int64  startTime_;
  Int32  staticStmtCount_;
  Int32  dynamicStmtCount_;
  short  openStmtCount_;
  short  closeStmtCount_;
  short  reclaimStmtCount_;   
  Int32  pfsSize_;
  Int32  pfsCurUse_;
  Int32  pfsMaxUse_;
  short  sqlOpenCount_;
  short  arkfsSessionCount_;  
  char   *recentQid_;
  Lng32  qidLen_;
  NABoolean delQid_;
  Int32  numESPsStarted_;
  Int32  numESPsStartupCompleted_;
  Int32  numESPsDeleted_;
  Int32  numESPsBad_;
  Int32  numESPsInUse_;
  Int32  numESPsFree_;
}; 

#endif


