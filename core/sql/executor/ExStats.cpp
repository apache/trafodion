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
 *****************************************************************************
 *
 * File:         ExStats.cpp
 * Description:  methods for classes ExStats and SQLStatistics
 *               
 *               
 * Created:      6/18/1997
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "PortProcessCalls.h"

  #define FAILURE { ex_assert(FALSE, "Invalid SqlBuffer"); }

#include <stdio.h>

#include "NAStdlib.h"

#include "str.h"
#include "cli_stdh.h"
#include "sql_id.h"
#include "Statement.h"

#include <float.h>



#include "ExCextdecs.h"
#include "ex_stdh.h"
#include "ComTdb.h"
#include "ComTdbUdr.h"
#include "ComTdbSplitTop.h"
#include "ComTdbExeUtil.h"
#include "ComTdbHdfsScan.h"
#include "ComTdbHbaseAccess.h"
#include "ComTdbFastTransport.h"
#include "ex_exe_stmt_globals.h"
#include "exp_clause_derived.h"
#include "Int64.h"
#include "ComQueue.h"
#include "ExStats.h"
#include "str.h"
#include "ssmpipc.h"
#include "rts_msg.h"
#include "ComSqlId.h"
#include "ComRtUtils.h"
#include "Statement.h"
#include "ComTdbRoot.h"
#include "ComDistribution.h"
#include "ex_hashj.h"
#include "ex_sort.h"
#include "ex_hash_grby.h"

#include <unistd.h>
#include <errno.h>
#include "seabed/fs.h"
#include "seabed/ms.h"

inline void advanceSize2(UInt32 &size, const char * const buffPtr)
{
  const Int32 lenSize = sizeof(Lng32);
  size += lenSize;
  if (buffPtr != NULL)
      size += str_len(buffPtr) + 1; // 1 is for the buffer's null-terminator
}


/////////////////////////////////////////////////////////////////////////
// class ExStatsBaseClass
/////////////////////////////////////////////////////////////////////////
void ExStatsBase::alignSizeForNextObj(UInt32 &size)
{
  ULng32 s = (ULng32) size; // just for safety

  // clear the last 3 bits of the size to round it down to
  // the next size that is divisible by 8
  ULng32 roundedDown = s LAND 0xFFFFFFF8;

  // if that didn't change anything we're done, the size was
  // a multiple of 8 already
  if (s != roundedDown)
    {
      // else we have to round up and add the filler
      size = (UInt32) roundedDown + 8;
    }
}

void ExStatsBase::alignBufferForNextObj(const char* &buffer)
{
  buffer = (const char *) ROUND8((Long)buffer);
}

UInt32 ExStatsBase::alignedPack(char* buffer)
{
  // c89 needs to do this in 2 steps, it can't automatically conver
  // a char *& into a const char *&
  const char * alignedBuffer1 = buffer;
  alignBufferForNextObj(alignedBuffer1);
  char * alignedBuffer = (char *) alignedBuffer1;
  
  // pack the object and compute the length
  UInt32 result = pack(alignedBuffer);

  // add the filler space that had to be added to the returned result
  result += (alignedBuffer - buffer);

  return result;
}

void ExStatsBase::alignedUnpack(const char* &buffer)
{
  alignBufferForNextObj(buffer);
  
  unpack(buffer);
}

//////////////////////////////////////////////////////////////////
// class ExStatsCounter
//////////////////////////////////////////////////////////////////
ExStatsCounter::ExStatsCounter()
{
  init();
} 

ExStatsCounter& ExStatsCounter::operator=(const ExStatsCounter& other)
{
   //  Check for pathological case of X = X.
   if (this == &other)
      return *this;

   entryCnt_ = other.entryCnt_;
   min_ = other.min_;
   max_ = other.max_;
   sum_ = other.sum_;
   sum2_ = other.sum2_;

   return *this;
}

void ExStatsCounter::addEntry(Int64 value)
{
  // we got another value
  entryCnt_++;

  // ajust the min
  if (value < min_)
    min_ = value;

  // adjust the max
  if (value > max_)
    max_ = value;

  // adjust sum and sum2
  sum_ += value;
  sum2_ += (float)value * (float)value;
};

void ExStatsCounter::merge(ExStatsCounter * other)
{
  entryCnt_ += other->entryCnt();

  if (min_ > other->min())
    min_ = other->min();

  if (max_ < other->max())
    max_ = other->max();

  sum_ += other->sum();
  sum2_ += other->sum2();
};

void ExStatsCounter::init()
{
  entryCnt_ = 0;
  min_  = LLONG_MAX;
  max_  = LLONG_MIN;
  sum_  = 0;
  sum2_ = 0.0F;
}

float ExStatsCounter::mean()
{
  float result = 0.0;
  if (entryCnt_)
    result = ((float) sum_) / (float)entryCnt_;
  return result;
}

float ExStatsCounter::variance()
{
  float result = 0.0;
  if (entryCnt_ > 1)
    result = (1.0F/(float)(entryCnt_ - 1)) *
      (sum2_ - (1.0F/(float)entryCnt_) * (float)sum_ * (float)sum_);
  return result;
}

//////////////////////////////////////////////////////////////////
// class ExClusterStats
//////////////////////////////////////////////////////////////////
ExClusterStats::ExClusterStats()
  : version_(StatsCurrVersion),
    isInner_(FALSE),
    actRows_(0),
    totalSize_(0),
    writeIOCnt_(0),
    readIOCnt_(0),
    next_(NULL) {};

ExClusterStats::ExClusterStats(NABoolean isInner,
			       ULng32 bucketCnt,
			       Int64 actRows,
			       Int64 totalSize,
			       ExStatsCounter hashChains,
			       Int64 writeIOCnt,
			       Int64 readIOCnt) 
  : version_(StatsCurrVersion),
    isInner_(isInner),
    bucketCnt_(bucketCnt),
    actRows_(actRows),
    totalSize_(totalSize),
    hashChains_(hashChains),
    writeIOCnt_(writeIOCnt),
    readIOCnt_(readIOCnt),
    next_(NULL) {}

ExClusterStats& ExClusterStats::operator=(const ExClusterStats& other)
{
  //  Check for pathological case of X = X.
  if (this == &other)
    return *this;

  isInner_ = other.isInner_;
  bucketCnt_ = other.bucketCnt_;
  actRows_ = other.actRows_;
  totalSize_ = other.totalSize_;
  hashChains_ = other.hashChains_;
  writeIOCnt_ = other.writeIOCnt_;
  readIOCnt_ = other.readIOCnt_;
  // we do NOT assign the next pointer!

  return *this;
}

UInt32 ExClusterStats::packedLength()
{
  UInt32  size = sizeof(version_);
  size += sizeof(isInner_);
  size += sizeof(bucketCnt_);
  size += sizeof(actRows_);
  size += sizeof(totalSize_);
  size += sizeof(hashChains_);
  size += sizeof(writeIOCnt_);
  size += sizeof(readIOCnt_);

  return size;
}

UInt32 ExClusterStats::pack(char* buffer){
  UInt32 size = packIntoBuffer(buffer, version_);
  size += packIntoBuffer(buffer, isInner_);
  size += packIntoBuffer(buffer, bucketCnt_);
  size += packIntoBuffer(buffer, actRows_);
  size += packIntoBuffer(buffer, totalSize_);
  size += packIntoBuffer(buffer, hashChains_);
  size += packIntoBuffer(buffer, writeIOCnt_);
  size += packIntoBuffer(buffer, readIOCnt_);

  return size;
}

void ExClusterStats::unpack(const char* &buffer)
{
  unpackBuffer(buffer, version_);
  unpackBuffer(buffer, isInner_);
  unpackBuffer(buffer, bucketCnt_);
  unpackBuffer(buffer, actRows_);
  unpackBuffer(buffer, totalSize_);
  unpackBuffer(buffer, hashChains_);
  unpackBuffer(buffer, writeIOCnt_);
  unpackBuffer(buffer, readIOCnt_);
};

void ExClusterStats::getVariableStatsInfo(char * dataBuffer,
					  char * dataLen,
					  Lng32 maxLen)
{
  char *buf = dataBuffer;

  sprintf (
       buf,
       "NumBuckets: %u ActRows: %ld NumChains: %u MaxChain: %ld VarChain: %f ",
       bucketCnt_,
       actRows_,
       hashChains_.entryCnt(),
       hashChains_.max(),
       hashChains_.variance());
  buf += str_len(buf);

  *(short*)dataLen = (short) (buf - dataBuffer);
}

void ExClusterStats::getVariableStatsInfoAggr(char * dataBuffer,
					      char * dataLen,
					      Lng32 maxLen)
{
  char *buf = dataBuffer;
  ExClusterStats *tempNext = this;

  // form an aggregate of all clusters and display it first
  ULng32 bucketCntTot = 0;
  ExStatsCounter actRowsTot;
  ExStatsCounter totalSizeTot;
  ExStatsCounter hashChainsTot;
  Int64 writeIOCntTot = 0;
  Int64 readIOCntTot = 0;
  ULng32 numClusters = 0;
  ULng32 residentClusters = 0;
  
  while (tempNext)
    {
      numClusters++;
      bucketCntTot += tempNext->bucketCnt_;
      actRowsTot.addEntry(tempNext->actRows_);
      totalSizeTot.addEntry(tempNext->totalSize_);
      hashChainsTot.merge(&(tempNext->hashChains_));
      writeIOCntTot += tempNext->writeIOCnt_;
      readIOCntTot  += tempNext->readIOCnt_;
      if (tempNext->writeIOCnt_ == 0)
	residentClusters++;
      tempNext = tempNext->next_;
    }
  
  sprintf(buf,
	      "NumClusters: %u ResidentClusters: %u TotNumBuckets: %u ",
	      numClusters,
	      residentClusters,
	      bucketCntTot);
  buf += str_len(buf);

  Int64 val;

  val = totalSizeTot.sum();
  if (val)
    {
      sprintf(buf, "TotHashTableSize: %ld ", val);
      buf += str_len(buf);
    }
  val = writeIOCntTot;
  if (val)
    {
      sprintf(buf, "TotWriteIOs: %ld ", val);
      buf += str_len(buf);
    }
  val = readIOCntTot;
  if (val)
    {
      sprintf(buf, "TotReadIOs: %ld ", val );
      buf += str_len(buf);
    }
  val = actRowsTot.min();
  if (val)
    {
      sprintf(buf, "MinClusterRows: %ld ", val);
      buf += str_len(buf);
    }
  val = actRowsTot.max();
  if (val)
    {
      sprintf(buf, "MaxClusterRows: %ld ", val);
      buf += str_len(buf);
    }
  if (actRowsTot.entryCnt())
    {
      sprintf(buf, "ClusterRowsVar: %f ", actRowsTot.variance());
      buf += str_len(buf);
    }

  // while there is space in the buffer, loop over the individual
  // clusters and print out their statistics
  tempNext = this;
  Lng32 tempMaxLen = maxLen - (buf - dataBuffer);
  ULng32 c = 0;
  while (tempNext && tempMaxLen > 200)
    {
      sprintf(buf, "ClusterNo: %u ", c);
      tempMaxLen -= str_len(buf);
      buf += str_len(buf);

      tempNext->getVariableStatsInfo(buf, dataLen, tempMaxLen);
      buf += *((short *) dataLen);
      tempMaxLen -= *((short *) dataLen);

      tempNext = tempNext->next_;
      c++; // yes, it does happen sometimes when writing C++
    }

  *(short*)dataLen = (short) (buf - dataBuffer);
}

//////////////////////////////////////////////////////////////////
// class ExTimeStats
//////////////////////////////////////////////////////////////////

ExTimeStats::ExTimeStats( clockid_t clk_id)
     : version_(StatsCurrVersion)
     , clockid_(clk_id)
{
   reset();
}

void ExTimeStats::reset()
{
  sumTime_ = 0L; 
  isStarted_ = FALSE;
   startTime_.tv_sec = 0L;
   startTime_.tv_nsec = 0L;

}

void ExTimeStats::start()
{
  if (! isStarted_)
    {
      // going from not started to started, record the starting timestamps
      // in the data members.
      isStarted_ = TRUE;
      

      if (clock_gettime(clockid_, &startTime_))
      {
        char buf[256];
        sprintf(buf, "clock_gettime failed, errno %d", errno);
        ex_assert(0, buf);
      }
    }
}

Int64 ExTimeStats::stop()
{
  Int64 incTime = 0;
  if (isStarted_)
    {
      struct timespec endTime;
      
      if (clock_gettime(clockid_, &endTime))
      {
        char buf[256];
        sprintf(buf, "clock_gettime failed, errno %d", errno);
        ex_assert(0, buf);
      }
      
      if (startTime_.tv_nsec > endTime.tv_nsec)
      {
        // borrow 1 from tv_sec, convert to nanosec and add to tv_nsec.
        endTime.tv_nsec += 1LL * 1000LL * 1000LL * 1000LL;
        endTime.tv_sec -= 1LL;
      }

      incTime = ((endTime.tv_sec - startTime_.tv_sec) * 1000LL * 1000LL * 1000LL)
                +  (endTime.tv_nsec - startTime_.tv_nsec);

      incTime /= 1000LL;
      if (incTime < 0)
        incTime = 0;
      sumTime_ += incTime;
      isStarted_ = FALSE;
    }
  return incTime;
}

UInt32 ExTimeStats::packedLength()
{
  UInt32  size = sizeof(version_);
  size += sizeof(sumTime_);
  return size;
}

UInt32 ExTimeStats::pack(char * buffer)
{
  UInt32 size = packIntoBuffer(buffer, version_);
  size += packIntoBuffer(buffer, sumTime_);
  return size;
}

void ExTimeStats::unpack(const char* &buffer)
{
  Int64 elaspedTime;
  // if the version doesn't match, tough. Just lose the contents
  // of mismatched versions. 
  unpackBuffer(buffer, version_);
  if (version_ >= _STATS_PRE_RTS_VERSION)
  {
    unpackBuffer(buffer, sumTime_);
    if (version_ < _STATS_RTS_VERSION_R25)
    {
      unpackBuffer(buffer, elaspedTime);
    }
  }
}

Lng32 ExTimeStats::filterForSEstats(struct timespec currTimespec)
{
  Lng32 diffTime = 0;
  
  if (isStarted_) 
     diffTime = currTimespec.tv_sec - startTime_.tv_sec;
  return diffTime;
}

//////////////////////////////////////////////////////////////////
// class ExeSEtats
//////////////////////////////////////////////////////////////////
UInt32 ExeSEStats::packedLength() 
{
  UInt32 size  = sizeof(accessedRows_);
  size += sizeof(usedRows_);
  size += sizeof(numIOCalls_);
  size += sizeof(numIOBytes_);
  size += sizeof(maxIOTime_);
  return size;
}

UInt32 ExeSEStats::pack(char * buffer) 
{
  UInt32 size  = packIntoBuffer(buffer, accessedRows_);
  size += packIntoBuffer(buffer, usedRows_);
  size += packIntoBuffer(buffer, numIOCalls_);
  size += packIntoBuffer(buffer, numIOBytes_);
  size += packIntoBuffer(buffer, maxIOTime_);
  return size;
}

void ExeSEStats::merge(ExeSEStats * other) 
{
  accessedRows_ = accessedRows_ + other->accessedRows_;
  usedRows_ = usedRows_ + other->usedRows_;
  numIOCalls_ = numIOCalls_ + other->numIOCalls_;
  numIOBytes_ = numIOBytes_ + other->numIOBytes_;
  maxIOTime_ = maxIOTime_ + other->maxIOTime_;
}

void ExeSEStats::copyContents(ExeSEStats *other)
{
  accessedRows_ = other->accessedRows_;
  usedRows_ = other->usedRows_;
  numIOCalls_ = other->numIOCalls_;
  numIOBytes_ = other->numIOBytes_;
  maxIOTime_ = other->maxIOTime_;
}

void ExeSEStats::unpack(const char* &buffer) 
{
  unpackBuffer(buffer, accessedRows_);
  unpackBuffer(buffer, usedRows_);
  unpackBuffer(buffer, numIOCalls_);
  unpackBuffer(buffer, numIOBytes_);
  unpackBuffer(buffer, maxIOTime_);
}

//////////////////////////////////////////////////////////////////
// class ExBufferStats
//////////////////////////////////////////////////////////////////
UInt32 ExBufferStats::packedLength()
{
  UInt32 size  = sizeof(sendBufferSize_);
  size += sizeof(recdBufferSize_);
  size += sizeof(totalSentBytes_);
  size += sizeof(totalRecdBytes_);
  size += sizeof(statsBytes_);
  size += sizeof(sentBuffers_);
  size += sizeof(recdBuffers_);

  return size;
}

UInt32 ExBufferStats::pack(char * buffer) 
{
  UInt32 size  = packIntoBuffer(buffer, sendBufferSize_);
  size += packIntoBuffer(buffer, recdBufferSize_);
  size += packIntoBuffer(buffer, totalSentBytes_);
  size += packIntoBuffer(buffer, totalRecdBytes_);
  size += packIntoBuffer(buffer, statsBytes_);
  size += packIntoBuffer(buffer, sentBuffers_);
  size += packIntoBuffer(buffer, recdBuffers_);
 
  return size;
}

void ExBufferStats::merge(ExBufferStats * other) 
{
  totalSentBytes_ = totalSentBytes_ + other->totalSentBytes_;
  totalRecdBytes_ = totalRecdBytes_ + other->totalRecdBytes_;
  statsBytes_ = statsBytes_ + other->statsBytes_;

  sentBuffers_.merge(&(other->sentBuffers_));
  recdBuffers_.merge(&(other->recdBuffers_));

}

void ExBufferStats::copyContents(ExBufferStats * other)
{
  sendBufferSize_ = other->sendBufferSize_;
  recdBufferSize_ = other->recdBufferSize_;

  totalSentBytes_ = other->totalSentBytes_;
  totalRecdBytes_ = other->totalRecdBytes_;

  statsBytes_ = other->statsBytes_;

  sentBuffers_  = other->sentBuffers_;
  recdBuffers_  = other->recdBuffers_;
}

void ExBufferStats::unpack(const char* &buffer) 
{
  unpackBuffer(buffer, sendBufferSize_);
  unpackBuffer(buffer, recdBufferSize_);
  unpackBuffer(buffer, totalSentBytes_);
  unpackBuffer(buffer, totalRecdBytes_);
  unpackBuffer(buffer, statsBytes_);
  unpackBuffer(buffer, sentBuffers_);
  unpackBuffer(buffer, recdBuffers_);

}
 
//////////////////////////////////////////////////////////////////
// class ExOperStats
//////////////////////////////////////////////////////////////////
ExOperStats::ExOperStats(NAMemory * heap,
			 StatType statType,
			 ex_tcb *tcb,
			 const ComTdb * tdb)
     : ExStatsBaseNew((NAHeap *)heap),
       version_(StatsCurrVersion),
       subReqType_(-1),
       statType_(statType),
       parentTdbId_(-1),
       flags_(0),
       leftChildTdbId_(-1),
       rightChildTdbId_(-1),
       explainNodeId_((tdb ? tdb->getExplainNodeId() : -1)),
       actualRowsReturned_(0),
       numberCalls_(0),
       dop_(0),
       savedDop_(0)
{
  // we should always have a heap pointer
  ex_assert(heap_, "no heap to allocate stats info");

  if (! tdb)
  {
    id_.fragId_     = 0;
    id_.tdbId_      = _UNINITIALIZED_TDB_ID;
    id_.instNum_    = _UNINITIALIZED_TDB_ID;
    id_.subInstNum_ = _UNINITIALIZED_TDB_ID; 
    pertableStatsId_ = -1;
    u.est.estRowsAccessed_ = 0;
    u.est.estRowsUsed_ = 0;
    Lng32 len = str_len(_NO_TDB_NAME);
    str_cpy_all(tdbName_, _NO_TDB_NAME, len);
    tdbName_[len] = 0;
    processNameString_[0] = '\0';      
    collectStatsType_ = (UInt16)ComTdb::NO_STATS;
    tdbType_ = ComTdb::ex_TDB;
  }
  else
  {
    // make a unique id for this entry
    ex_globals *glob = tcb->getGlobals();
    
    id_.fragId_     = glob->getMyFragId();
    id_.tdbId_      = tdb->getTdbId();
    id_.instNum_    = glob->getMyInstanceNumber();
    id_.subInstNum_ = 0; // caller needs to change this if necessary
    pertableStatsId_ = tdb->getPertableStatsTdbId();
  
    if (glob->castToExEidStmtGlobals())
       setStatsInDp2(TRUE);
    setStatsInTcb(TRUE);
    u.est.estRowsAccessed_ = tdb->getEstRowsAccessed();
    u.est.estRowsUsed_ = tdb->getEstRowsUsed();
    // set the tdb name if we have one
    if (tdb->getNodeName() != NULL)
    {
      Lng32 len = (Lng32)str_len(tdb->getNodeName());
      if (len > MAX_TDB_NAME_LEN)
        len = MAX_TDB_NAME_LEN;
      str_cpy_all(tdbName_, tdb->getNodeName(), len);
      tdbName_[len] = 0;
    }
    tdbType_ = tdb->getNodeType();
    collectStatsType_ = (UInt16)((ComTdb*)tdb)->getCollectStatsType();
    incDop();
  }
  sprintf(processNameString_, "%03d,%05d", GetCliGlobals()->myCpu(),
                                 GetCliGlobals()->myPin());
  allStats.downQueueSize_ = 0;
  allStats.upQueueSize_ = 0;
  allStats.ntProcessId_ = -1;
  allStats.fillerForFutureUse_ = 0;
  for (ULng32 i = 0; i < STATS_WORK_LAST_RETCODE + 2; i++)
    allStats.retCodes_[i] = 0;
}

ExOperStats::ExOperStats(NAMemory * heap,
			 StatType type)
     : ExStatsBaseNew((NAHeap *)heap),
       version_(StatsCurrVersion),
       subReqType_(-1),
       statType_(type),
       dop_(0),
       savedDop_(0),
       tdbType_(ComTdb::ex_LAST),
       collectStatsType_(ComTdb::OPERATOR_STATS),
       flags_(0),
       parentTdbId_(-1),
       leftChildTdbId_(-1),
       rightChildTdbId_(-1),
       explainNodeId_(-1),
       pertableStatsId_(-1),
       actualRowsReturned_(0),
       numberCalls_(0)
{
  u.est.estRowsAccessed_ = 0;
  u.est.estRowsUsed_ = 0;

  id_.fragId_     = 0;
  id_.tdbId_      = _UNINITIALIZED_TDB_ID;
  id_.instNum_    = _UNINITIALIZED_TDB_ID;
  id_.subInstNum_ = _UNINITIALIZED_TDB_ID; 
  processNameString_[0] = '\0';
   
  Lng32 len = str_len(_NO_TDB_NAME);
  str_cpy_all(tdbName_, _NO_TDB_NAME, len);
  tdbName_[len] = 0;
  tdbType_ = ComTdb::ex_TDB;
  allStats.downQueueSize_ = 0;
  allStats.upQueueSize_ = 0;
  allStats.ntProcessId_ = -1;
  allStats.fillerForFutureUse_ = 0;
  for (ULng32 i = 0; i < STATS_WORK_LAST_RETCODE + 2; i++)
    allStats.retCodes_[i] = 0;
}

ExOperStats::ExOperStats()
    : ExStatsBaseNew((NAHeap *)NULL),
    version_(StatsCurrVersion),
    subReqType_(-1),
    statType_(ExOperStats::EX_OPER_STATS),
    dop_(0),
    savedDop_(0),
    tdbType_(ComTdb::ex_LAST),
    collectStatsType_(ComTdb::OPERATOR_STATS),
    flags_(0),
    parentTdbId_(-1),
    explainNodeId_(-1),
    pertableStatsId_(-1),
    actualRowsReturned_(0),
    numberCalls_(0)
{
  id_.fragId_     = 0;
  id_.tdbId_      = _UNINITIALIZED_TDB_ID;
  id_.instNum_    = _UNINITIALIZED_TDB_ID;
  id_.subInstNum_ = _UNINITIALIZED_TDB_ID;
  processNameString_[0] = '\0';
  u.est.estRowsAccessed_ = 0;
  u.est.estRowsUsed_ = 0;
  tdbName_[0] = '\0';
  tdbType_ = ComTdb::ex_TDB;
  allStats.downQueueSize_ = 0;
  allStats.upQueueSize_ = 0;
  allStats.fillerForFutureUse_ = 0; 
  for (ULng32 i = 0; i < STATS_WORK_LAST_RETCODE + 2; i++)
    allStats.retCodes_[i] = 0;
}

ExOperStats::ExOperStats(NAMemory *heap,
              StatType statType,
              ComTdb::CollectStatsType collectStatsType,
              ExFragId fragId,
              Lng32 tdbId,
              Lng32 explainTdbId,
              Lng32 instNum,
              ComTdb::ex_node_type tdbType,
              char *tdbName,
              Lng32 tdbNameLen)
      : ExStatsBaseNew((NAHeap *)NULL),
        version_(StatsCurrVersion),
        subReqType_(-1),
        statType_(statType),
        dop_(0), 
        savedDop_(0),
        tdbType_(tdbType),
        collectStatsType_(collectStatsType),
        flags_(0),
        parentTdbId_(-1),
        leftChildTdbId_(-1),
        rightChildTdbId_(-1),
        explainNodeId_(explainTdbId),
        pertableStatsId_(-1),
        actualRowsReturned_(0),
        numberCalls_(0)
{
  u.est.estRowsAccessed_ = 0;
  u.est.estRowsUsed_ = 0;

  id_.fragId_     = fragId;
  id_.tdbId_      = tdbId;
  id_.instNum_    = instNum;
  id_.subInstNum_ = 0; 
  sprintf(processNameString_, "%03d,%05d", GetCliGlobals()->myCpu(),
                                 GetCliGlobals()->myPin());
  str_cpy_all(tdbName_, tdbName, tdbNameLen);
  tdbName_[tdbNameLen] = 0;
  tdbType_ = tdbType;
  collectStatsType_ = (UInt16)collectStatsType;
  allStats.downQueueSize_ = 0;
  allStats.upQueueSize_ = 0;
  allStats.ntProcessId_ = -1;
  allStats.fillerForFutureUse_ = 0;
  for (ULng32 i = 0; i < STATS_WORK_LAST_RETCODE + 2; i++)
    allStats.retCodes_[i] = 0;
  setStatsInTcb(TRUE);
  // dop is set to 1 since this stats entry is created without fragment using CLI to register the queryId
  // like in BDR
  incDop();
}

void ExOperStats::initTdbForRootOper()
{
  id_.fragId_ = 0;
  id_.tdbId_ = 1;
  id_.instNum_ = 0;
  id_.subInstNum_ = 0;
  str_cpy_all(tdbName_, "EX_ROOT", 7);
  tdbName_[7] = '\0';
  tdbType_ = ComTdb::ex_ROOT;
}

ExOperStats::~ExOperStats()
{
}

void ExOperStats::init(NABoolean resetDop)
{
  operTimer_.reset();
  actualRowsReturned_ = 0;
  numberCalls_ = 0;
  if (resetDop)
     dop_ = 0;
  clearHasSentMsgIUD();
  // fillerForFutureUse_ = 0;
  allStats.downQueueStats_.init();
  allStats.upQueueStats_.init();

  for (ULng32 i = 0; i < STATS_WORK_LAST_RETCODE + 2; i++)
    allStats.retCodes_[i] = 0;
}

void ExOperStats::subTaskReturn(ExWorkProcRetcode rc)
{
  // determine the index from the retcode. If the retcode is
  // not knows or greater than 0, count it in a seperate counter
  if ((rc > 0) || rc < -STATS_WORK_LAST_RETCODE)
    rc = -STATS_WORK_LAST_RETCODE - 1;
  allStats.retCodes_[-rc]++;
}

UInt32 ExOperStats::packedLength()
{
  Int32  size;
  
  if (collectStatsType_ == ComTdb::ALL_STATS)
    size = sizeof(ExOperStats)-sizeof(ExStatsBaseNew);
  else
    size = sizeof(ExOperStats)-sizeof(ExStatsBaseNew)-sizeof(allStats);
  return size;
}

UInt32 ExOperStats::pack(char* buffer)
{
  UInt32 srcLen;
  if (collectStatsType_ == ComTdb::ALL_STATS)
    srcLen = sizeof(ExOperStats)-sizeof(ExStatsBaseNew);
  else
    srcLen = sizeof(ExOperStats)-sizeof(ExStatsBaseNew)-sizeof(allStats);
  char * srcPtr = (char *)this+sizeof(ExStatsBaseNew);
  memcpy(buffer, (void *)srcPtr, srcLen);
  return srcLen;
}

void ExOperStats::unpack(const char* &buffer)
{
  unpackBuffer(buffer, version_);
  if (version_ >= _STATS_RTS_VERSION_R25)
  {
    UInt32 srcLen;
    // CollectStatsType_ is already set in ExStatisticsArea::unpackThisClass
    if (collectStatsType_ == ComTdb::ALL_STATS)
      srcLen = sizeof(ExOperStats)-sizeof(ExStatsBaseNew)-sizeof(version_);
    else
      srcLen = sizeof(ExOperStats)-sizeof(ExStatsBaseNew)-sizeof(allStats)-sizeof(version_);
    char * destPtr = (char *)this+sizeof(ExStatsBaseNew)+sizeof(version_);
    memcpy((void *)destPtr, buffer, srcLen);
    buffer += srcLen;
    return;
  }
  // we do not ship the statType in the object. Instead, it is shipped in front of
  // the object so that we later can assemble the StatsArea
  // (see ExStatisticssArea::packObjIntoMessage)
  Lng32 len;
  unpackBuffer(buffer, id_);
  unpackBuffer(buffer, parentTdbId_);
  unpackBuffer(buffer, leftChildTdbId_);
  unpackBuffer(buffer, rightChildTdbId_);
  unpackBuffer(buffer, allStats.ntProcessId_);
  unpackBuffer(buffer, explainNodeId_);
  unpackBuffer(buffer, len);
  unpackStrFromBuffer(buffer, tdbName_, len);
  tdbName_[len-1] = '\0';
  unpackBuffer(buffer, tdbType_);
  unpackBuffer(buffer, collectStatsType_);
  unpackBuffer(buffer, flags_);
  if (collectStatsType_ == ComTdb::ALL_STATS || collectStatsType_ == ComTdb::OPERATOR_STATS
                 || version_ <= _STATS_RTS_VERSION_R23)
  {
    unpackBuffer(buffer, allStats.downQueueSize_);
    unpackBuffer(buffer, allStats.upQueueSize_);
    unpackBuffer(buffer, allStats.downQueueStats_);
    unpackBuffer(buffer, allStats.upQueueStats_);
  }
  if (version_ >= _STATS_RTS_VERSION_R22)
  {
    unpackBuffer(buffer, u.est.estRowsAccessed_);
    unpackBuffer(buffer, u.est.estRowsUsed_);
  }
  else    
    unpackBuffer(buffer, u.estRowsReturned_);
  unpackBuffer(buffer, actualRowsReturned_);
  unpackBuffer(buffer, numberCalls_);
  unpackBuffer(buffer, allStats.fillerForFutureUse_);
  if (collectStatsType_ == ComTdb::ALL_STATS || collectStatsType_ == ComTdb::OPERATOR_STATS
                   || version_ <= _STATS_RTS_VERSION_R23)
  {
    // unpack the retcodes, one element at a time
    for (Lng32 i = 0; i < STATS_WORK_LAST_RETCODE_PREV + 2; i++)
      { unpackBuffer(buffer, allStats.retCodes_[i]); };
  }
}

void ExOperStats::merge(ExOperStats * other)
{
  if (version_ >= _STATS_RTS_VERSION_R22)
  {
    if (other->version_ >= _STATS_RTS_VERSION_R22)
    {
      if (other->u.est.estRowsAccessed_ > 0)
        u.est.estRowsAccessed_ = other->u.est.estRowsAccessed_;
      if (other->u.est.estRowsUsed_ > 0)
        u.est.estRowsUsed_ = other->u.est.estRowsUsed_;
    }
    else
    {
      if (other->u.estRowsReturned_ > 0)
        u.est.estRowsUsed_ = (float)other->u.estRowsReturned_;
    }
  }
  else
    ex_assert(0, "Stats version mismatch");
  //if (other->statsInDp2() && castToExFragRootOperStats())
  //  return;
  switch (collectStatsType_)
  {
  case ComTdb::ACCUMULATED_STATS:
  case ComTdb::PERTABLE_STATS:
      dop_ += other->dop_;
     break;
  default:
     dop_ += other->dop_;
     break;
  }
  actualRowsReturned_ += other->actualRowsReturned_;
  operTimer_ = operTimer_ + other->operTimer_;
  numberCalls_ += other->numberCalls_;
  if (other->hasSentMsgIUD())
    setHasSentMsgIUD();

  if (collectStatsType_ == ComTdb::ALL_STATS)
  {
    allStats.fillerForFutureUse_ += other->allStats.fillerForFutureUse_;

    allStats.downQueueStats_.merge(&other->allStats.downQueueStats_);
    allStats.upQueueStats_.merge(&other->allStats.upQueueStats_);

    if (allStats.downQueueSize_ < other->allStats.downQueueSize_)
      allStats.downQueueSize_ = other->allStats.downQueueSize_;
    if (allStats.upQueueSize_ < other->allStats.upQueueSize_)
      allStats.upQueueSize_ = other->allStats.upQueueSize_;

    for (ULng32 i = 0; i < STATS_WORK_LAST_RETCODE + 2; i++)
      allStats.retCodes_[i] += other->allStats.retCodes_[i];
  }
}

void ExOperStats::copyContents(ExOperStats * other)
{
  // Donot copy the collectStatsType_ and flags_
  UInt16 collectStatsType = collectStatsType_;
  UInt16 flags = flags_;
  UInt32 srcLen = sizeof(ExOperStats)-sizeof(ExStatsBaseNew)-sizeof(version_);
  char * srcPtr = (char *)other+sizeof(ExStatsBaseNew)+sizeof(version_);
  char * destPtr = (char *)this+sizeof(ExStatsBaseNew)+sizeof(version_);
  memcpy((void *)destPtr, (void *)srcPtr, srcLen);
  flags_ = flags;
  if (other->hasSentMsgIUD())
    setHasSentMsgIUD();
  collectStatsType_ = collectStatsType;
  return;
}

ExOperStats * ExOperStats::copyOper(NAMemory * heap)
{
  ExOperStats * stat = new(heap) ExOperStats(heap);

  stat->copyContents(this);

  return stat;
}

ExMeasStats * ExOperStats::castToExMeasStats()
{
  return NULL;
}

ExMeasBaseStats * ExOperStats::castToExMeasBaseStats()
{
  return NULL;
}

ExFragRootOperStats * ExOperStats::castToExFragRootOperStats()
{
  return NULL;
}

ExHdfsScanStats *
ExOperStats::castToExHdfsScanStats()
{
  return NULL;
}

ExHbaseAccessStats *
ExOperStats::castToExHbaseAccessStats()
{
  return NULL;
}

ExPartitionAccessStats * ExOperStats::castToExPartitionAccessStats()
{
  return NULL;
}

ExProbeCacheStats * ExOperStats::castToExProbeCacheStats()
{
  return NULL;
}


ExFastExtractStats * ExOperStats::castToExFastExtractStats()
{
  return NULL;
}

ExHashGroupByStats * ExOperStats::castToExHashGroupByStats()
{
  return NULL;
}

ExUDRStats * ExOperStats::castToExUDRStats()
{
  return NULL;
}

ExHashJoinStats * ExOperStats::castToExHashJoinStats()
{
  return NULL;
}

ExESPStats * ExOperStats::castToExESPStats()
{
  return NULL;
}
ExSplitTopStats * ExOperStats::castToExSplitTopStats()
{
  return NULL;
}

ExSortStats * ExOperStats::castToExSortStats()
{
  return NULL;
}

ExMasterStats * ExOperStats::castToExMasterStats()
{
  return NULL;
}

ExBMOStats *ExOperStats::castToExBMOStats()
{
  return NULL;
}
ExUDRBaseStats *ExOperStats::castToExUDRBaseStats()
{
  return NULL;
}

const char * ExOperStats::getNumValTxt(Int32 i) const 
{ 
  switch (i)
  {
    case 1:
      return "OperCpuTime";
  }
  return NULL;
}

Int64 ExOperStats::getNumVal(Int32 i) const 
{
  switch (i)
  {
  case 1:
    return operTimer_.getTime();
  }
  return 0; 
}

const char * ExOperStats::getTextVal()
{
  if (processNameString_[0] == '\0')
    return NULL;
  else
    return processNameString_;
}

void ExOperStats::getVariableStatsInfo(char * dataBuffer,
				       char * dataLen,
				       Lng32 maxLen)
{
  char *buf = dataBuffer;

  ex_assert(maxLen > 1000, "Assume varchar has plenty of room");

  sprintf(buf, "statsRowType: %d ", statType());
  buf += str_len(buf);
  if (collectStatsType_ == ComTdb::OPERATOR_STATS ||
    collectStatsType_ == ComTdb::ALL_STATS ||
    collectStatsType_ == ComTdb::QID_DETAIL_STATS)
  {
    sprintf(buf, "DOP: %d OperCpuTime: %ld ", dop_, operTimer_.getTime());
    buf += str_len(buf);
  }
  if (hasSentMsgIUD())
  {
     sprintf(buf, "HasSentIudMsg: 1 ");
     buf += str_len(buf);
  }
  if (collectStatsType_ == ComTdb::ALL_STATS)
  {
  // queue utilization
    if (allStats.upQueueSize_ || allStats.downQueueSize_)
    {
      sprintf(
	   buf,
	   "QUtilUpMax: %u QUtilUpAvg: %u QUtilDnMax: %u QUtilDnAvg: %u ",
	   (ULng32) allStats.upQueueStats_.max(),
	   (ULng32) allStats.upQueueStats_.mean(),
	   (ULng32) allStats.downQueueStats_.max(),
	   (ULng32) allStats.downQueueStats_.mean());
    }

    // Work procedure return codes other than WORK_OK
    if (allStats.retCodes_[-WORK_OK])
    {
      sprintf(buf, "RetOK: %u ", allStats.retCodes_[-WORK_OK]);
      buf += str_len(buf);
    }
    if (allStats.retCodes_[-WORK_CALL_AGAIN])
    {
      sprintf(buf, "RetCallAgain: %u ", allStats.retCodes_[-WORK_CALL_AGAIN]);
      buf += str_len(buf);
    }
    if (allStats.retCodes_[-WORK_POOL_BLOCKED])
    {
      sprintf(buf, "RetPoolBlocked: %u ", allStats.retCodes_[-WORK_POOL_BLOCKED]);
      buf += str_len(buf);
    }
    if (allStats.retCodes_[-WORK_BAD_ERROR])
    {
      sprintf(buf, "RetBadError: %u ",allStats. retCodes_[-WORK_BAD_ERROR]);
      buf += str_len(buf);
    }
  }
  *(short*)dataLen = (short) (buf - dataBuffer);
}

// sets sqlStats_item->error_code
//   0 if stats_item is found OK
//   EXE_ERROR_IN_STAT_ITEM if stats_item is found but is truncated
//   -EXE_STAT_NOT_FOUND if statsItem_Id is not found

Lng32 ExOperStats::getStatsItem(SQLSTATS_ITEM* sqlStats_item)
{
  sqlStats_item->error_code = 0;
  switch (sqlStats_item->statsItem_id)
  { 
  case SQLSTATS_EXPLAIN_NODE_ID:
    sqlStats_item->int64_value = getExplainNodeId();
    break;

  case SQLSTATS_TDB_ID:
    sqlStats_item->int64_value = getTdbId();
    break;
  
  case SQLSTATS_TDB_NAME:
    if (sqlStats_item->str_value != NULL)
    {
      Lng32 len = str_len(getTdbName());
      if (len > sqlStats_item->str_max_len)
      {
        len = sqlStats_item->str_max_len;
        sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
      }
      str_cpy(sqlStats_item->str_value, getTdbName(), len);
      sqlStats_item->str_ret_len = len;
    }    
    break;

   case SQLSTATS_EST_ROWS_ACCESSED:
    sqlStats_item->double_value = getEstRowsAccessed();
    break;
   
   case SQLSTATS_EST_ROWS_USED:
    sqlStats_item->double_value = getEstRowsUsed();
    break;
    
  case SQLSTATS_ACT_ROWS_USED:
    sqlStats_item->int64_value = getActualRowsReturned();
    break;
    
  case SQLSTATS_NUM_CALLS:
    sqlStats_item->int64_value = getNumberCalls();
    break;
    
  case SQLSTATS_LEFT_CHILD:
    sqlStats_item->int64_value = getLeftChildTdbId();
    break;
    
  case SQLSTATS_RIGHT_CHILD:
    sqlStats_item->int64_value = getRightChildTdbId();
    break;
  case SQLSTATS_PARENT_TDB_ID:
    sqlStats_item->int64_value = parentTdbId_;
    break;
  case SQLSTATS_FRAG_NUM:
    sqlStats_item->int64_value = getFragId();
    break;
  case SQLSTATS_OPER_CPU_TIME:
    sqlStats_item->int64_value = operTimer_.getTime();
    break;
  case SQLSTATS_INST_NUM:
    sqlStats_item->int64_value = getInstNum();
    break;
  case SQLSTATS_INST_ID:
    if (sqlStats_item->str_value != NULL)
    {
      Lng32 len = str_len(processNameString_);
      if (len > sqlStats_item->str_max_len)
      {
        len = sqlStats_item->str_max_len;
        sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
      }
      str_cpy(sqlStats_item->str_value, processNameString_, len);
      sqlStats_item->str_ret_len = len;
    }    
    break;
  case SQLSTATS_DOP:
    sqlStats_item->int64_value = dop_;
    break;
  default:
    sqlStats_item->error_code = -EXE_STAT_NOT_FOUND;
    break;
  }

  return 0;
}

NABoolean ExOperStats::operator==(ExOperStats * other) 
{
  if (statType() == ExOperStats::MEAS_STATS)
    return TRUE;
  else 
  if (statType() == ExOperStats::BMO_STATS ||
       statType() == ExOperStats::UDR_BASE_STATS ||
       statType() == ExOperStats::REPLICATOR_STATS ||
       statType() == ExOperStats::REORG_STATS || 
       statType() == ExOperStats::HBASE_ACCESS_STATS ||
       statType() == ExOperStats::HDFSSCAN_STATS) 

  {
    if (getId()->tdbId_ == other->getId()->tdbId_)
      return TRUE;
    else
      return FALSE;
  }
  else 
  {
    if (getId()->compare(*other->getId(), getCollectStatsType()))
      return TRUE;
    else
      return FALSE;
  }
}


Int64 ExOperStats::getHashData(UInt16 statsMergeType)
{
  UInt16 tempStatsMergeType;
  if (statsMergeType == SQLCLI_SAME_STATS)
    tempStatsMergeType = getCollectStatsType();
  else
    tempStatsMergeType = statsMergeType;
  switch (tempStatsMergeType)
  {
  case ComTdb::ACCUMULATED_STATS:
    return 1;
  case ComTdb::PERTABLE_STATS:
    if (statType() == ExOperStats::ROOT_OPER_STATS)
      return 1;
    else if (statType() == ExOperStats::BMO_STATS)
      return 1;
    else if (statType() == ExOperStats::UDR_BASE_STATS)
      return 1;
    else
      return getId()->tdbId_;
  case ComTdb::PROGRESS_STATS:
    if (statType() == ExOperStats::ROOT_OPER_STATS)
      return 1;
    else
      return getId()->tdbId_;
  case ComTdb::OPERATOR_STATS:
    if (statType() == ExOperStats::MEAS_STATS)
      return 1;
    else
      return (getId()->tdbId_);
   default:
    if (statType() == ExOperStats::MEAS_STATS)
      return 1;
    else
      return (getId()->fragId_ + getId()->tdbId_ + 
	      getId()->instNum_ + getId()->subInstNum_);
  }
}

//////////////////////////////////////////////////////////////////
// class ExFragRootOperStats
//////////////////////////////////////////////////////////////////

ExFragRootOperStats::ExFragRootOperStats(NAMemory * heap,
					 ex_tcb *tcb,
					 const ComTdb * tdb)
  : ExOperStats(heap,
		ROOT_OPER_STATS,
		tcb,
		tdb),
    flags_(0)
{
  executionCount_ = 0;
  init(FALSE);
  initHistory();
  queryId_ = NULL;
  queryIdLen_ = 0;

  if (tdb && tcb && tcb->getGlobals())
    {
      ExExeStmtGlobals *glob = tcb->getGlobals()->castToExExeStmtGlobals();
      if ((glob && glob->castToExMasterStmtGlobals()) &&
	  (tdb->getNodeType() == ComTdb::ex_ROOT))
	{
	  ComTdbRoot * root = (ComTdbRoot*)tdb;
	  setHdfsAccess(root->hdfsAccess());
	}
    }
}

ExFragRootOperStats::ExFragRootOperStats(NAMemory * heap)
  : ExOperStats(heap,
		ROOT_OPER_STATS),
    flags_(0)
{
  executionCount_ = 0;
  init(FALSE);
  initHistory();
  queryId_ = NULL;
  queryIdLen_ = 0;
  stmtIndex_ = -1;
  timestamp_ = 0;
}

ExFragRootOperStats::ExFragRootOperStats(NAMemory *heap,
                      ComTdb::CollectStatsType collectStatsType,           
                      ExFragId fragId,
                      Lng32 tdbId,
                      Lng32 explainNodeId,
                      Lng32 instNum,
                      ComTdb::ex_node_type tdbType,
                      char *tdbName,
                      Lng32 tdbNameLen)
     : ExOperStats(heap, ROOT_OPER_STATS, collectStatsType, fragId, tdbId, 
		   explainNodeId,
		   instNum, tdbType, tdbName, tdbNameLen),
       flags_(0)
{
   // Set the Id so that 
  init(FALSE);
  initHistory();
  queryId_ = NULL;
  queryIdLen_ = 0;
  stmtIndex_ = -1;
  timestamp_ = 0;
}

ExFragRootOperStats::~ExFragRootOperStats()
{
  ExProcessStats *processStats;

  if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS && queryId_ != NULL)
  {
    NADELETEBASIC(queryId_, getHeap());
    queryId_ = NULL;
  }
  else 
  if (queryId_ != NULL)
  {
    processStats = GetCliGlobals()->getExProcessStats();
    if (processStats != NULL)
       processStats->setRecentQidToNull(queryId_);
  }
}

void ExFragRootOperStats::init(NABoolean resetDop)
{
  ExOperStats::init(resetDop);
  spaceUsage_ = 0;
  spaceAlloc_ = 0;
  heapUsage_ = 0;
  heapAlloc_ = 0;
  heapWM_ = 0;
  cpuTime_ = 0;
  newprocess_ = 0;
  newprocessTime_ = 0;
  espSpaceUsage_ = 0;
  espSpaceAlloc_ = 0;
  espHeapUsage_ = 0;
  espHeapAlloc_ = 0;
  espHeapWM_ = 0;
  espCpuTime_ = 0;
  histCpuTime_ = 0;
  reqMsgCnt_ = 0;
  reqMsgBytes_ = 0;
  replyMsgCnt_ = 0;
  replyMsgBytes_ = 0;
  pagesInUse_ = 0;
  executionCount_++;
  XPROCESSHANDLE_GETMINE_(&phandle_);
  isFragSuspended_ = false;
  localCpuTime_ = 0;
  scratchOverflowMode_ = -1;
  scratchFileCount_ = 0;
  scratchIOSize_ = 0;
  spaceBufferSize_ = 0;
  spaceBufferCount_ = 0;
  scratchWriteCount_ = 0;
  scratchReadCount_ = 0;
  scratchIOMaxTime_ = 0;
  interimRowCount_ = 0;
  udrCpuTime_ = 0;
  topN_ = -1;
  waitTime_ = 0;
  maxWaitTime_ = 0;
  diffCpuTime_ = 0;

  //  flags_ = 0;
}

void ExFragRootOperStats::initHistory()
{
}

UInt32 ExFragRootOperStats::packedLength()
{
  UInt32 size;
  size = ExOperStats::packedLength();
  if (statsInDp2())
  {
    size += sizeof(spaceUsage_);
    size += sizeof(spaceAlloc_);
    size += sizeof(heapUsage_);
    size += sizeof(heapAlloc_);
    size += sizeof(cpuTime_);
  }
  else
  {
    alignSizeForNextObj(size);
    size += sizeof(ExFragRootOperStats)-sizeof(ExOperStats);
    if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS)
      size += queryIdLen_;
  }
  return size;
}

UInt32 ExFragRootOperStats::pack(char * buffer)
{
  UInt32 packedLen;
  UInt32 srcLen = 0;
  packedLen = ExOperStats::pack(buffer);
  if (statsInDp2())
  {
    buffer += packedLen;
    packedLen += packIntoBuffer(buffer, spaceUsage_);
    packedLen += packIntoBuffer(buffer, spaceAlloc_);
    packedLen += packIntoBuffer(buffer, heapUsage_);
    packedLen += packIntoBuffer(buffer, heapAlloc_);
    packedLen += packIntoBuffer(buffer, cpuTime_);
  }
  else
  {
    alignSizeForNextObj(packedLen);
    buffer += packedLen;
    srcLen = sizeof(ExFragRootOperStats)-sizeof(ExOperStats);
    char * srcPtr = (char *)this+sizeof(ExOperStats);
    memcpy(buffer, srcPtr, srcLen);
    packedLen += srcLen;
    if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS)
    {
      buffer += srcLen;
      if (queryIdLen_ != 0 && queryId_ != NULL)
        packedLen +=  packStrIntoBuffer(buffer, queryId_, queryIdLen_);
    }
  }
  return packedLen;
}

void ExFragRootOperStats::unpack(const char* &buffer)
{
  NABoolean dp2Stats = statsInDp2();
  NABoolean espStats = statsInEsp();
  UInt32 srcLen;
  ExOperStats::unpack(buffer);
  if (dp2Stats)
  {
    if (getVersion() < _STATS_RTS_VERSION_R25)
    {
      short phandle[10];
      str_cpy_all((char *) phandle, buffer, sizeof(phandle));
      buffer += sizeof(phandle);
      NABoolean temp;
      unpackBuffer(buffer, temp);
      unpackBuffer(buffer, temp);
    }
    if (getVersion() >= _STATS_RTS_VERSION_R25)
      unpackBuffer(buffer, cpuTime_);
    else
    {
      ExTimeStats times;
      times.alignedUnpack(buffer);
      cpuTime_ = times.getTime();
    }
  }
  else
  {
    alignBufferForNextObj(buffer); 
    srcLen = sizeof(ExFragRootOperStats)-sizeof(ExOperStats);
    char * srcPtr = (char *)this+sizeof(ExOperStats);
    memcpy((void *)srcPtr, buffer, srcLen);
    buffer += srcLen;
    if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS)
    {
      if (queryIdLen_ != 0)
      {
        queryId_ = new ((NAHeap *)(getHeap())) char[queryIdLen_+1];
        unpackStrFromBuffer(buffer, queryId_, queryIdLen_);
        queryId_[queryIdLen_] = '\0';
      }
      else
      {
        queryId_ = NULL;
        queryIdLen_ = 0;
      }
    }
  }
  if (espStats)
  {
    espSpaceUsage_ += spaceUsage_;
    espSpaceAlloc_ += spaceAlloc_;
    espHeapUsage_  += heapUsage_;
    espHeapAlloc_ += heapAlloc_;
    espHeapWM_ += heapWM_;
    espCpuTime_ += cpuTime_;
    spaceUsage_ = 0;
    spaceAlloc_ = 0;
    heapUsage_ = 0;
    heapAlloc_ = 0;
    heapWM_ = 0;
    cpuTime_ = 0;
  }
}

void ExFragRootOperStats::merge(ExFragRootOperStats* other)
{
  ExOperStats::merge(other);
  if (other->statsInEsp())
  {
    espCpuTime_  += other->cpuTime_;
    espSpaceUsage_ += other -> spaceUsage_;
    espSpaceAlloc_ += other -> spaceAlloc_;
    espHeapUsage_ += other -> heapUsage_;
    espHeapAlloc_ += other -> heapAlloc_;
    espHeapWM_ += other->heapWM_;
  }
  else
  {
    cpuTime_ += other->cpuTime_;
    spaceUsage_ += other -> spaceUsage_;
    spaceAlloc_ += other -> spaceAlloc_;
    heapUsage_ += other -> heapUsage_;
    heapAlloc_ += other -> heapAlloc_;
    heapWM_ += other->heapWM_;
  }
  newprocess_ += other -> newprocess_;
  newprocessTime_ += other -> newprocessTime_;
  espSpaceUsage_ += other -> espSpaceUsage_;
  espSpaceAlloc_ += other -> espSpaceAlloc_;
  espHeapUsage_ += other -> espHeapUsage_;
  espHeapAlloc_ += other -> espHeapAlloc_;
  espHeapWM_ += other->espHeapWM_;
  espCpuTime_ += other -> espCpuTime_;
  reqMsgCnt_ += other -> reqMsgCnt_;
  reqMsgBytes_ += other -> reqMsgBytes_;
  replyMsgCnt_ += other -> replyMsgCnt_;
  replyMsgBytes_ += other -> replyMsgBytes_;
  if (scratchOverflowMode_ == -1)
    scratchOverflowMode_ = other->scratchOverflowMode_;
  scratchFileCount_ += other->scratchFileCount_;
  Float32 mFactor = 1;
  if (spaceBufferSize_ == 0 &&
     other->spaceBufferSize_ > 0)
     spaceBufferSize_ = other->spaceBufferSize_;
  mFactor = 1;
  if(spaceBufferSize_ > 0)
    mFactor = (Float32)other->spaceBufferSize_ / spaceBufferSize_;
  spaceBufferCount_ += Int32(other->spaceBufferCount_ * mFactor);
  if (scratchIOSize_ == 0 &&
     other->scratchIOSize_ > 0)
     scratchIOSize_ = other->scratchIOSize_;
  mFactor = 1;
  if(scratchIOSize_ > 0)
    mFactor = (Float32)other->scratchIOSize_ / scratchIOSize_;

  scratchReadCount_ += Int32(other->scratchReadCount_ * mFactor);
  scratchWriteCount_ += Int32(other->scratchWriteCount_ * mFactor);
  if (other->scratchIOMaxTime_ > scratchIOMaxTime_)
     scratchIOMaxTime_ = other->scratchIOMaxTime_;
  interimRowCount_ += other->interimRowCount_;
  udrCpuTime_ += other->udrCpuTime_;
  if(topN_ == -1 && other->topN_ > 0)
    topN_ = other->topN_;
  // Remember, don't merge or copy  executionCount_ !
  waitTime_ += other->waitTime_;
  if (other->maxWaitTime_ > maxWaitTime_)
     maxWaitTime_ = other->maxWaitTime_;
  flags_ |= other->flags_;
}

void ExFragRootOperStats::merge(ExUDRBaseStats *other)
{
  reqMsgCnt_        += other->reqMsgCnt_;
  reqMsgBytes_      += other->reqMsgBytes_;
  replyMsgCnt_      += other->replyMsgCnt_;
  replyMsgBytes_    += other->replyMsgBytes_;
  udrCpuTime_       += other->udrCpuTime_;
}

void ExFragRootOperStats::merge(ExBMOStats *other)
{
  scratchFileCount_ += other->scratchFileCount_;
  scratchOverflowMode_ = other->scratchOverflowMode_;
  Float32 mFactor = 1;
  if (spaceBufferSize_ == 0 &&
     other->spaceBufferSize_ > 0)
     spaceBufferSize_ = other->spaceBufferSize_;
  mFactor = 1;
  if(spaceBufferSize_ > 0)
    mFactor = (Float32)other->spaceBufferSize_ / spaceBufferSize_;
  spaceBufferCount_ += Int32(other->spaceBufferCount_ * mFactor);
  if (scratchIOSize_ == 0 &&
     other->scratchIOSize_ > 0)
     scratchIOSize_ = other->scratchIOSize_;
  mFactor = 1;
  if(scratchIOSize_ > 0)
    mFactor = (Float32)other->scratchIOSize_ / scratchIOSize_;
  scratchReadCount_ += Int32 (other->scratchReadCount_ * mFactor);
  scratchWriteCount_ += Int32 (other->scratchWriteCount_ * mFactor);
  scratchIOMaxTime_ += other->scratchIOMaxTime_;
  interimRowCount_ += other->interimRowCount_;
  if(topN_ == -1 && other->topN_ > 0)
    topN_ = other->topN_;
}

void ExFragRootOperStats::merge(ExOperStats * other)
{
  switch (other->statType())
  {
    case ROOT_OPER_STATS:
      merge((ExFragRootOperStats*) other);
      break;
    case UDR_BASE_STATS:
      merge((ExUDRBaseStats *)other);
      break;
    case BMO_STATS:
      merge((ExBMOStats *)other);
      break;
    case SE_STATS:
      merge((ExStorageEngineStats *)other);
      break;
    default:
      // do nothing - This type of stat has no merge data
      break;
  }
}

void ExFragRootOperStats::copyContents(ExFragRootOperStats *stat)
{
  ExOperStats::copyContents(stat);
  char * srcPtr = (char *)stat+sizeof(ExOperStats);
  char * destPtr = (char *)this+sizeof(ExOperStats);
  UInt32 srcLen = sizeof(ExFragRootOperStats)-sizeof(ExOperStats);
  memcpy((void *)destPtr, (void *)srcPtr, srcLen);
  if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS)
  {
    if (queryIdLen_ != 0)
    {
      queryId_ = new ((NAHeap *)(getHeap())) char[queryIdLen_+1];
      str_cpy_all(queryId_, stat->queryId_, queryIdLen_);
      queryId_[queryIdLen_] = '\0';
    }
    else
      queryId_ = NULL;
    pagesInUse_  = stat->pagesInUse_;
  // Remember, don't merge or copy  executionCount_ !
  }
}

ExOperStats * ExFragRootOperStats::copyOper(NAMemory * heap)
{
  ExFragRootOperStats * stat =  new(heap) ExFragRootOperStats(heap);
  stat->copyContents(this);
  return stat;
}


ExFragRootOperStats * ExFragRootOperStats::castToExFragRootOperStats()
{
  return this;
}

const char * ExFragRootOperStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "OperCpuTime";
    case 2:
      return "messageBytes";
    case 3:
      return "messageCount";
    case 4:
      return "memoryAllocated";
    }
  return NULL;
}

Int64 ExFragRootOperStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
         return reqMsgBytes_ + replyMsgBytes_;
    case 3:
         return reqMsgCnt_ + replyMsgCnt_;
    case 4:
      return heapAlloc_+espHeapAlloc_+spaceAlloc_+espSpaceAlloc_;
    }
  return 0;
}

const char * ExFragRootOperStats::getTextVal()
{
  return ExOperStats::getTextVal();
}

void ExFragRootOperStats::getVariableStatsInfo(char * dataBuffer,
					       char * dataLen,
					       Lng32 maxLen)
{
  char *buf = dataBuffer;
  const char *txtVal;
  if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS)
  {
    sprintf(buf, "statsRowType: %d ProcessId: %s Qid: %s CpuTime: %ld SpaceUsed: %u "
      "SpaceTotal: %u HeapUsed: %u HeapTotal: %u PMemUsed: %ld diffCpuTime: %ld ",
      statType(),
      (((txtVal = getTextVal()) != NULL) ? txtVal : "NULL"),
      ((queryId_ != NULL) ? queryId_ : "NULL"),
                cpuTime_,
      (UInt32)spaceUsage_,
      (UInt32)spaceAlloc_,
      (UInt32)heapUsage_,
      (UInt32)heapAlloc_,
      pagesInUse_ * 16,
      diffCpuTime_);
  }
  else
  {
    ExOperStats::getVariableStatsInfo(dataBuffer, dataLen, maxLen);
    buf += *((short *) dataLen);
    sprintf(buf,
		"CpuTime: %ld ProcessId: %s StmtIndex: %d Timestamp: %ld "
		"SpaceUsed: %u SpaceTotal: %u HeapUsed: %u HeapTotal: %u HeapWM: %u "
		"Newprocess: %u NewprocessTime: %ld reqMsgCnt: %ld "
		"regMsgBytes: %ld replyMsgCnt: %ld replyMsgBytes: %ld "
		"PMemUsed: %ld scrOverFlowMode: %d sortTopN: %d "
		"scrFileCount: %d bmoSpaceBufferSize: %d bmoSpaceBufferCount: %ld scrIOSize: %d " 
		"scrWriteCount:%ld scrReadCount: %ld scrIOMaxTime: %ld bmoInterimRowCount: %ld udrCpuTime: %ld "
		"maxWaitTime: %ld avgWaitTime: %ld hdfsAccess: %d ",
		cpuTime_,
		(((txtVal = getTextVal()) != NULL) ? txtVal : "NULL"),
		stmtIndex_,
		timestamp_,
		(UInt32)spaceUsage_,
		(UInt32)spaceAlloc_,
		(UInt32)heapUsage_,
		(UInt32)heapAlloc_,
		(UInt32)heapWM_,
		newprocess_,
		newprocessTime_,
		reqMsgCnt_,
		reqMsgBytes_,
		replyMsgCnt_,
		replyMsgBytes_,
		pagesInUse_ * 16,
		scratchOverflowMode_,
		topN_,
		scratchFileCount_,
                spaceBufferSize_,
                spaceBufferCount_,
		scratchIOSize_,
		scratchWriteCount_,
		scratchReadCount_,
                scratchIOMaxTime_,
                interimRowCount_,
		udrCpuTime_,
		maxWaitTime_,
		getAvgWaitTime(),
		(hdfsAccess() ? 1 : 0)
        );
  }
  buf += str_len(buf);

  // dataLen is really the varchar indicator
  *(short *)dataLen = (short) (buf - dataBuffer);
}

Lng32 ExFragRootOperStats::getStatsItem(SQLSTATS_ITEM* sqlStats_item)
{
  char tmpBuf[100];
  Int32 len;
  Lng32 retcode = 0;
    sqlStats_item->error_code = 0;
    switch (sqlStats_item->statsItem_id)
    {
    case SQLSTATS_SQL_CPU_BUSY_TIME:
         sqlStats_item->int64_value = cpuTime_ + espCpuTime_;
      break;
    case SQLSTATS_SQL_SPACE_ALLOC:
        sqlStats_item->int64_value = spaceAlloc_ + espSpaceAlloc_;
      break;
    case SQLSTATS_SQL_SPACE_USED:
        sqlStats_item->int64_value = spaceUsage_+ espSpaceUsage_;
      break;
    case SQLSTATS_SQL_HEAP_ALLOC:
        sqlStats_item->int64_value = heapAlloc_+ espHeapAlloc_;
      break;
    case SQLSTATS_SQL_HEAP_USED:
        sqlStats_item->int64_value = heapUsage_ + espHeapUsage_;
      break;
    case SQLSTATS_SQL_HEAP_WM:
        sqlStats_item->int64_value = heapWM_+ espHeapWM_;
      break;
    case SQLSTATS_PROCESS_CREATED:
      sqlStats_item->int64_value = newprocess_;
      break;
    case SQLSTATS_PROCESS_CREATE_TIME:
      sqlStats_item->int64_value = newprocessTime_;
      break;
    case SQLSTATS_REQ_MSG_CNT:
      sqlStats_item->int64_value = reqMsgCnt_;
      break;
    case SQLSTATS_REQ_MSG_BYTES:
      sqlStats_item->int64_value = reqMsgBytes_;
      break;
    case SQLSTATS_REPLY_MSG_CNT:
      sqlStats_item->int64_value = replyMsgCnt_;
      break;
    case SQLSTATS_REPLY_MSG_BYTES:
      sqlStats_item->int64_value = replyMsgBytes_;
      break;
    case SQLSTATS_PHYS_MEM_IN_USE:
      sqlStats_item->int64_value = pagesInUse_ * 16;
      break;
    case SQLSTATS_SCRATCH_FILE_COUNT:
      sqlStats_item->int64_value = scratchFileCount_;
      break;
    case SQLSTATS_SCRATCH_OVERFLOW_MODE:
      sqlStats_item->int64_value = scratchOverflowMode_;
      break;
    case SQLSTATS_TOPN:
      sqlStats_item->int64_value = topN_;
      break;
    case SQLSTATS_BMO_SPACE_BUFFER_SIZE:
      sqlStats_item->int64_value = spaceBufferSize_;
      break;
    case SQLSTATS_BMO_SPACE_BUFFER_COUNT:
      sqlStats_item->int64_value = spaceBufferCount_;
      break;
    case SQLSTATS_SCRATCH_IO_SIZE:
      sqlStats_item->int64_value = scratchIOSize_;
      break;
    case SQLSTATS_SCRATCH_READ_COUNT:
      sqlStats_item->int64_value = scratchReadCount_;
      break;
    case SQLSTATS_SCRATCH_WRITE_COUNT:
      sqlStats_item->int64_value = scratchWriteCount_;
      break;
    case SQLSTATS_SCRATCH_IO_MAX_TIME:
      sqlStats_item->int64_value = scratchIOMaxTime_;
      break;
    case SQLSTATS_INTERIM_ROW_COUNT:
      sqlStats_item->int64_value = interimRowCount_;
      break;
    case SQLSTATS_UDR_CPU_BUSY_TIME:
      sqlStats_item->int64_value = udrCpuTime_;
      break;
    case SQLSTATS_SQL_AVG_WAIT_TIME:
      sqlStats_item->int64_value = getAvgWaitTime();
      break;
    case SQLSTATS_SQL_MAX_WAIT_TIME:
      sqlStats_item->int64_value = maxWaitTime_;
      break;
    case SQLSTATS_DETAIL:
      if (sqlStats_item->str_value != NULL)
      {
         sprintf(tmpBuf, "%ld|%ld|%ld|%ld|",
             cpuTime_ + espCpuTime_, getNumVal(2), getNumVal(3), getNumVal(4));
         len =  strlen(tmpBuf);

         if (len > sqlStats_item->str_max_len)
            sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
         else
            str_cpy(sqlStats_item->str_value, tmpBuf, len);
         sqlStats_item->str_ret_len = len;
      }
      break;
    default:
      retcode = ExOperStats::getStatsItem(sqlStats_item);
      break;
    }
  return retcode;
}

NABoolean ExFragRootOperStats::filterForCpuStats()
{
  NABoolean retcode;

  if (histCpuTime_ == 0)
    retcode = FALSE;
  else
    if ((diffCpuTime_ = cpuTime_ - histCpuTime_) > 0)
      retcode = TRUE;
    else
      retcode = FALSE;
  
  setCpuStatsHistory();
  return retcode;
}

//////////////////////////////////////////////////////////////////
// class ExStorageEngineStats
//////////////////////////////////////////////////////////////////

ExStorageEngineStats::ExStorageEngineStats(NAMemory * heap,
                          ex_tcb *tcb,
                          ComTdb * tdb)
  : ExOperStats(heap,
                HDFSSCAN_STATS,
                tcb, 
                tdb)
  ,  timer_(CLOCK_MONOTONIC)
{
  const char * name;
  switch (tdb->getNodeType())
  {
    case ComTdb::ex_HBASE_ACCESS:
    {
       ComTdbHbaseAccess *hbaseTdb = (ComTdbHbaseAccess *) tdb;
       name = hbaseTdb->getTableName();
       break;
    }
    case ComTdb::ex_HDFS_SCAN:
    {
       ComTdbHdfsScan *hdfsTdb = (ComTdbHdfsScan *) tdb;
       name = hdfsTdb->tableName();
       break;
    }
    case ComTdb::ex_FAST_EXTRACT:
    {
       ComTdbFastExtract *feTdb = (ComTdbFastExtract *)tdb;
       name = feTdb->getTargetName();
       break;
    }
    default:
       name = "";
  }
     
  // allocate memory and copy the ansi name into the stats entry
  Lng32 len = (Lng32)str_len(name); 
  tableName_ = (char *)heap_->allocateMemory(len + 1);
  sprintf(tableName_, "%s", name);

  queryId_ = NULL;
  queryIdLen_ = 0;
  init(FALSE);
}

ExStorageEngineStats::ExStorageEngineStats(NAMemory * heap)
  : ExOperStats(heap,
                SE_STATS)
  , tableName_(NULL)
  , timer_(CLOCK_MONOTONIC)
{
  queryId_ = NULL;
  queryIdLen_ = 0;
  init(FALSE);
}

void ExStorageEngineStats::init(NABoolean resetDop)
{
  ExOperStats::init(resetDop);
  timer_.reset();

  numBytesRead_ = 0;
  accessedRows_ = 0;
  usedRows_     = 0;
  numIOCalls_ = 0;
  maxIOTime_ = 0;
  blockTime_ = 0;
}

ExStorageEngineStats::~ExStorageEngineStats()
{
  if (tableName_ != NULL)
  {
     NADELETEBASIC(tableName_,getHeap());
     tableName_ = NULL;
  }
  if ((Int32)getCollectStatsType() == SQLCLI_SE_OFFENDER_STATS && queryId_ != NULL)
  {
    NADELETEBASIC(queryId_, getHeap());
    queryId_ = NULL;
  }
}

UInt32 ExStorageEngineStats::packedLength()
{
  UInt32 size = ExOperStats::packedLength();
  size += sizeof(timer_);

  advanceSize2(size, tableName_);

  size += sizeof(numBytesRead_);
  size += sizeof(accessedRows_);
  size += sizeof(usedRows_);
  size += sizeof(numIOCalls_);
  size += sizeof(maxIOTime_);
  if ((Int32)getCollectStatsType() == SQLCLI_SE_OFFENDER_STATS)
  {
    size += sizeof(blockTime_);
    size += sizeof(queryIdLen_);
    size += queryIdLen_;
  }
  return size;
}

UInt32 ExStorageEngineStats::pack(char *buffer)
{
  UInt32 size = ExOperStats::pack(buffer);
  buffer += size;
  size += packIntoBuffer(buffer, timer_);

  size += packCharStarIntoBuffer(buffer, tableName_);

  size += packIntoBuffer(buffer, numBytesRead_);
  size += packIntoBuffer(buffer, accessedRows_);
  size += packIntoBuffer(buffer, usedRows_);
  size += packIntoBuffer(buffer, numIOCalls_);
  size += packIntoBuffer(buffer, maxIOTime_);
  if ((Int32)getCollectStatsType() == SQLCLI_SE_OFFENDER_STATS)
  {
    size += packIntoBuffer(buffer, blockTime_);
    size += packIntoBuffer(buffer, queryIdLen_);
    if (queryIdLen_ != 0 && queryId_ != NULL)
      size += packStrIntoBuffer(buffer, queryId_, queryIdLen_);
  }

  return size;
}

void ExStorageEngineStats::unpack(const char* &buffer)
{
  ExOperStats::unpack(buffer);

  unpackBuffer(buffer, timer_);

  unpackBuffer(buffer, tableName_, heap_);

  unpackBuffer(buffer, numBytesRead_);
  unpackBuffer(buffer, accessedRows_);
  unpackBuffer(buffer, usedRows_);
  unpackBuffer(buffer, numIOCalls_);
  unpackBuffer(buffer, maxIOTime_);
  if ((Int32)getCollectStatsType() == SQLCLI_SE_OFFENDER_STATS)
  {
    unpackBuffer(buffer, blockTime_);
    unpackBuffer(buffer, queryIdLen_);
    if (queryIdLen_ != 0)
    {
      queryId_ = new ((NAHeap *)(getHeap())) char[queryIdLen_+1];
      unpackStrFromBuffer(buffer, queryId_, queryIdLen_);
      queryId_[queryIdLen_] = '\0';
    }
  }
}

void ExStorageEngineStats::merge(ExStorageEngineStats *other)
{
  ExOperStats::merge(other);
  timer_ = timer_ + other->timer_;
  numBytesRead_ += other->numBytesRead_;
  accessedRows_ += other->accessedRows_;
  usedRows_     += other->usedRows_;
  numIOCalls_    += other->numIOCalls_;
  if (maxIOTime_ < other->maxIOTime_) // take the larger value
    maxIOTime_ = other->maxIOTime_;
}

void ExStorageEngineStats::copyContents(ExStorageEngineStats *other)
{
  ExOperStats::copyContents(other);

  // copy names only if we don't have one
  if (tableName_ == NULL && other->tableName_) 
  {
    Lng32 len = (Lng32)str_len(other->tableName_);
    tableName_ = (char *)heap_->allocateMemory(len + 1);
    str_cpy_all(tableName_, other->tableName_, len);
    tableName_[len] = 0;
  }

  timer_ = other->timer_;
  numBytesRead_ = other->numBytesRead_;
  accessedRows_ = other->accessedRows_;
  usedRows_     = other->usedRows_;
  numIOCalls_  = other->numIOCalls_;
  maxIOTime_ = other->maxIOTime_;
  if ((Int32)getCollectStatsType() == SQLCLI_SE_OFFENDER_STATS)
  {
    blockTime_ = other->blockTime_;
    queryIdLen_ = other->queryIdLen_;
    if (queryIdLen_ != 0)
    {
      queryId_ = new ((NAHeap *)(getHeap())) char[queryIdLen_+1];
      str_cpy_all(queryId_, other->queryId_, queryIdLen_);
      queryId_[queryIdLen_] = '\0';
    }
    else
      queryId_ = NULL;
  }
  else
  {
    queryId_ = other->queryId_;
    queryIdLen_ = other->queryIdLen_;
  }
}

ExOperStats * ExStorageEngineStats::copyOper(NAMemory * heap)
{
  ExStorageEngineStats *stat = new(heap) ExStorageEngineStats(heap);
  stat->copyContents(this);
  return stat;
}

ExHdfsScanStats 
*ExHdfsScanStats::castToExHdfsScanStats()
{
  return this;
}

ExHbaseAccessStats 
*ExHdfsScanStats::castToExHbaseAccessStats()
{
  return this;
}

const char *ExStorageEngineStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "OperCpuTime";
    case 2:
      return "SE_IO_KBytes";
    case 3:
      return "SE_IO_SumTime";
    case 4:
      return "ActRowsAccessed";
    }
  return NULL;
}

Int64 ExStorageEngineStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
      return numBytesRead_;
    case 3:
      return timer_.getTime();
    case 4:
      return accessedRows_;
    }
  return 0;
}

NABoolean ExStorageEngineStats::filterForSEstats(struct timespec currTimespec, Lng32 filter)
{
   Int64 sumIOTime;

   if (filter > 0) {
      blockTime_ = timer_.filterForSEstats(currTimespec);
      if (blockTime_ >= filter)
         return TRUE;
   }
   else
   if (queryId_ != NULL && (sumIOTime = timer_.getTime()) > 0 && (sumIOTime = sumIOTime /(1000000LL)) >= -filter) {
      blockTime_ = sumIOTime;
      return TRUE;
   }
   return FALSE;
}

void ExStorageEngineStats::getVariableStatsInfo(char * dataBuffer,
						   char * dataLen,
						   Lng32 maxLen)
{
  char *buf = dataBuffer;
  if ((Int32)getCollectStatsType() == SQLCLI_SE_OFFENDER_STATS)
  {
     sprintf(buf, "statsRowType: %d Qid: %s blockedFor: %d ",
        statType(),
        ((queryId_ != NULL) ? queryId_ : "NULL"), blockTime_);
     buf += str_len(buf);
  }
  else 
  {
     ExOperStats::getVariableStatsInfo(dataBuffer, dataLen, maxLen);
     buf += *((short *) dataLen);
  }
  sprintf (buf, 
	   "AnsiName: %s  MessagesBytes: %ld AccessedRows: %ld UsedRows: %ld HiveIOCalls: %ld HiveSumIOTime: %ld HdfsMaxIOTime: %ld "
           "HbaseSumIOCalls: %ld HbaseSumIOTime: %ld HbaseMaxIOTime: %ld ",
           
	       (char*)tableName_,
	       numBytesRead(), 
	       rowsAccessed(),
	       rowsUsed(),
               numIOCalls_,
	       timer_.getTime(),
	       maxIOTime_,
               hbaseCalls(),
               timer_.getTime(),
               maxHbaseIOTime() 
	       );
  buf += str_len(buf);
  
 *(short*)dataLen = (short) (buf - dataBuffer);
}

Lng32 ExStorageEngineStats::getStatsItem(SQLSTATS_ITEM* sqlStats_item)
{
  sqlStats_item->error_code = 0;
  Int32 len;
  Int32 len1;
  const char *tableName;
  char tmpBuf[100];

  switch (sqlStats_item->statsItem_id)
  {
  case SQLSTATS_TABLE_ANSI_NAME:
    if (sqlStats_item->str_value != NULL)
    {
      if (tableName_ != NULL)
        tableName = tableName_;
      else
        tableName = "NO_NAME_YET";
      len = str_len(tableName);
      if (len > sqlStats_item->str_max_len)
      {
        len = sqlStats_item->str_max_len;
        sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
      }
      str_cpy(sqlStats_item->str_value, tableName, len);
      sqlStats_item->str_ret_len = len;
    }
    break;
  case SQLSTATS_EST_ROWS_ACCESSED:
    sqlStats_item->double_value = getEstRowsAccessed();
    break;
  case SQLSTATS_EST_ROWS_USED:
    sqlStats_item->double_value = getEstRowsUsed();
    break;
  case SQLSTATS_ACT_ROWS_ACCESSED:
    sqlStats_item->int64_value = accessedRows_;
    break;
  case SQLSTATS_ACT_ROWS_USED:
    sqlStats_item->int64_value = usedRows_;
    break;
  case SQLSTATS_HIVE_IOS:
  case SQLSTATS_HBASE_IOS:
    sqlStats_item->int64_value = numIOCalls_;
    break;
  case SQLSTATS_HIVE_IO_BYTES:
  case SQLSTATS_HBASE_IO_BYTES:
    sqlStats_item->int64_value = numBytesRead_;
    break;
  case SQLSTATS_HIVE_IO_ELAPSED_TIME:
  case SQLSTATS_HBASE_IO_ELAPSED_TIME:
    sqlStats_item->int64_value = timer_.getTime();
    break;
  case SQLSTATS_HIVE_IO_MAX_TIME:
  case SQLSTATS_HBASE_IO_MAX_TIME:
    sqlStats_item->int64_value = maxIOTime_;
    break;
  case SQLSTATS_DETAIL:
    if (sqlStats_item->str_value != NULL)
    {
      if (tableName_ != NULL)
      {
        len = str_len(tableName_);
        if (len > sqlStats_item->str_max_len)
        {
           sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
           str_cpy_all(sqlStats_item->str_value, tableName_, sqlStats_item->str_max_len);
        }
        else
           str_cpy_all(sqlStats_item->str_value, tableName_, len);
      }
      else 
        len = 0; 

      sprintf(tmpBuf, "|%ld|%ld|%ld", getNumVal(2), getNumVal(3), getNumVal(4));
      len1 = str_len(tmpBuf);
      if ((len+len1) > sqlStats_item->str_max_len)
        sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
      else
        str_cpy(sqlStats_item->str_value+len, tmpBuf, len1);
      sqlStats_item->str_ret_len = len+len1;
    }
    break;
  default:
    ExOperStats::getStatsItem(sqlStats_item);
    break;
  }
  return 0;
}
  
//////////////////////////////////////////////////////////////////
// class ExProbeCacheStats
//////////////////////////////////////////////////////////////////
ExProbeCacheStats::ExProbeCacheStats(NAMemory * heap,
			 ex_tcb *tcb,
			 ComTdb * tdb,
			 ULng32 bufferSize,
                         ULng32 numCacheEntries)
  : ExOperStats(heap,
		PROBE_CACHE_STATS,
		tcb,
		tdb),
    bufferSize_(bufferSize),
    numCacheEntries_(numCacheEntries)
{
  init(FALSE);
  longestChain_ = 0;
  numChains_ = 0;
  maxNumChains_ = 0;
}

ExProbeCacheStats::ExProbeCacheStats(NAMemory * heap)
  : ExOperStats(heap,
		PROBE_CACHE_STATS),
    bufferSize_(0),
    numCacheEntries_(0)
{
  init(FALSE);
  longestChain_ = 0;
  numChains_ = 0;
  maxNumChains_ = 0;
}

UInt32 ExProbeCacheStats::packedLength()
{
  UInt32 size = ExOperStats::packedLength();
  size += sizeof(longestChain_);
  size += sizeof(numChains_);
  size += sizeof(maxNumChains_);
  size += sizeof(highestUseCount_);
  size += sizeof(bufferSize_);
  size += sizeof(numCacheEntries_);
  size += sizeof(cacheHits_);
  size += sizeof(cacheMisses_);
  size += sizeof(canceledHits_);
  size += sizeof(canceledMisses_);
  size += sizeof(canceledNotStarted_);

  return size;
}

UInt32
ExProbeCacheStats::pack(char * buffer)
{
  UInt32 size = ExOperStats::pack(buffer);
  buffer += size;
  size += packIntoBuffer(buffer, longestChain_);
  size += packIntoBuffer(buffer, numChains_);
  size += packIntoBuffer(buffer, maxNumChains_);
  size += packIntoBuffer(buffer, highestUseCount_);
  size += packIntoBuffer(buffer, bufferSize_);
  size += packIntoBuffer(buffer, numCacheEntries_);
  size += packIntoBuffer(buffer, cacheHits_);
  size += packIntoBuffer(buffer,  cacheMisses_);
  size += packIntoBuffer(buffer,  canceledHits_);
  size += packIntoBuffer(buffer,  canceledMisses_);
  size += packIntoBuffer(buffer,  canceledNotStarted_);

  return size;
}

void ExProbeCacheStats::unpack(const char* &buffer)
{
  ExOperStats::unpack(buffer);

  unpackBuffer(buffer, longestChain_);
  unpackBuffer(buffer, numChains_);
  unpackBuffer(buffer, maxNumChains_);
  unpackBuffer(buffer, highestUseCount_);
  unpackBuffer(buffer, bufferSize_);
  unpackBuffer(buffer, numCacheEntries_);
  unpackBuffer(buffer,  cacheHits_);
  unpackBuffer(buffer,  cacheMisses_);
  unpackBuffer(buffer,  canceledHits_);
  unpackBuffer(buffer,  canceledMisses_);
  unpackBuffer(buffer,  canceledNotStarted_);
}

void ExProbeCacheStats::init(NABoolean resetDop)
{
  ExOperStats::init(resetDop);

  highestUseCount_ = 0;
  cacheHits_ = 0;
  cacheMisses_ = 0;
  canceledHits_ = 0;
  canceledMisses_ = 0;
  canceledNotStarted_ = 0;

  // Note that bufferSize_ and numCacheEntries_ do not change,
  // and so do not need to be cleared between executions.

  // Also, the hash table persists between executions (but is 
  // effectively invalidated because "execution count" is part
  // of the hash key) so counters for the physical hash 
  // table should persist between executions too.  These
  // are longestChain_, numChains_, and maxNumChains_.

}

void ExProbeCacheStats::merge(ExProbeCacheStats* other)
{
  ExOperStats::merge(other);
  
  numChains_ += other -> numChains_;
  cacheHits_ += other -> cacheHits_;
  cacheMisses_ += other -> cacheMisses_;
  canceledHits_ += other -> canceledHits_;
  canceledMisses_ += other -> canceledMisses_;
  canceledNotStarted_ += other -> canceledNotStarted_;
  
  updateLongChain(other -> longestChain_);
  updateUseCount(other -> highestUseCount_);
  if(maxNumChains_ < other -> maxNumChains_)
    maxNumChains_ = other -> maxNumChains_;

}

void ExProbeCacheStats::copyContents(ExProbeCacheStats* other)
{
  ExOperStats::copyContents(other);
  
  longestChain_ = other->longestChain_;
  numChains_ = other->numChains_;
  maxNumChains_ = other->maxNumChains_;
  highestUseCount_ = other->highestUseCount_;
  cacheHits_ = other->cacheHits_;
  cacheMisses_ = other->cacheMisses_;
  canceledHits_ = other->canceledHits_;
  canceledMisses_ = other->canceledMisses_;
  canceledNotStarted_ = other->canceledNotStarted_;
  bufferSize_ = other->bufferSize_;
  numCacheEntries_ = other->numCacheEntries_;
}

ExOperStats * ExProbeCacheStats::copyOper(NAMemory * heap)
{
  ExProbeCacheStats* stat = new(heap) ExProbeCacheStats(heap);
  stat->copyContents(this);
  return stat;
}

ExProbeCacheStats * ExProbeCacheStats::castToExProbeCacheStats()
{
  return this;
}

const char * ExProbeCacheStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "OperCpuTime";
    case 2:
      return "CacheHits";
    case 3:
      return "MaxProbeRefCnt";
    case 4:
      return "LongestChain";
    case 5:
      return "MaxNumChains";
    }
  return NULL;
}

Int64 ExProbeCacheStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
      return cacheHits_;
    case 3:
      return highestUseCount_;
    case 4:
      return longestChain_;
    case 5:
      return maxNumChains_;
    }
  return 0;
}

void ExProbeCacheStats::getVariableStatsInfo(char * dataBuffer,
					    char * dataLen,
					    Lng32 maxLen)
{
  char *buf = dataBuffer;

  ExOperStats::getVariableStatsInfo(dataBuffer, dataLen, maxLen);
  buf += *((short *) dataLen);

  sprintf(buf, "CacheHits: %d CacheMisses: %d ",
	            cacheHits_,    cacheMisses_ );
  buf += str_len(buf);

  sprintf(buf, 
             "CanceledHits: %d CanceledMisses: %d CanceledNotStarted: %d ",
	      canceledHits_,    canceledMisses_,    canceledNotStarted_);
  buf += str_len(buf);

  sprintf(buf, "LongestChain: %d MaxNumChains: %d ",
	            longestChain_,    maxNumChains_);
  buf += str_len(buf);

  sprintf(buf, 
    "HighestUseCount: %d ResultBufferSizeBytes: %d NumCacheEntries: %d",
     highestUseCount_,    bufferSize_,                numCacheEntries_);
  buf += str_len(buf);

  *(short*)dataLen = (short) (buf - dataBuffer);
}

////////////////////////////////////////////////////////////////
// class ExPartitionAccessStats
////////////////////////////////////////////////////////////////
ExPartitionAccessStats::ExPartitionAccessStats(NAMemory * heap,
					       ex_tcb *tcb,
					       ComTdb * tdb,
					       ULng32 bufferSize)
  : ExOperStats(heap,
		PARTITION_ACCESS_STATS,
		tcb,
		tdb),
    ansiName_(NULL),
    fileName_(NULL),
    opens_(0),
    openTime_(0)
{
}

ExPartitionAccessStats::ExPartitionAccessStats(NAMemory * heap)
  : ExOperStats(heap,
		PARTITION_ACCESS_STATS),
    ansiName_(NULL),
    fileName_(NULL),
    opens_(0),
    openTime_(0)
{}

ExPartitionAccessStats::~ExPartitionAccessStats()
{
  if (ansiName_)
    heap_->deallocateMemory((void*)ansiName_);
  if (fileName_)
    heap_->deallocateMemory((void*)fileName_);
}

void ExPartitionAccessStats::init(NABoolean resetDop)
{
  ExOperStats::init(resetDop);

  exeSEStats()->init(resetDop);

  bufferStats()->init(resetDop);
}

void ExPartitionAccessStats::copyContents(ExPartitionAccessStats* other)
{
  ExOperStats::copyContents(other);
  exeSEStats()->copyContents(other->exeSEStats());
  bufferStats()->copyContents(other->bufferStats());
  
  // copy names only if we don't have one
  if (ansiName_ == NULL && other->ansiName_) 
  {
    Lng32 len = (Lng32)str_len(other->ansiName_);
    ansiName_ = (char *)heap_->allocateMemory(len + 1);
    str_cpy_all(ansiName_, other->ansiName_, len);
    ansiName_[len] = 0;
  }

  if (fileName_ == NULL && other->fileName_) 
  {
    Lng32 len = (Lng32)str_len(other->fileName_);
    fileName_ = (char *)heap_->allocateMemory(len + 1);
    str_cpy_all(fileName_, other->fileName_, len);
    fileName_[len] = 0;
  }
  opens_ = other->opens_;
  openTime_ = other->openTime_;
}

void ExPartitionAccessStats::merge(ExPartitionAccessStats* other)
{
  ExOperStats::merge(other);
  exeSEStats()->merge(other ->exeSEStats());

  bufferStats()->merge(other -> bufferStats());

  opens_ += other->opens_;
  openTime_ += other->openTime_;
}

UInt32        ExPartitionAccessStats::packedLength()
{
  UInt32 size = ExOperStats::packedLength();
  alignSizeForNextObj(size);
  size += exeSEStats()->packedLength();
  
  alignSizeForNextObj(size);
  size += bufferStats()->packedLength();
  
  advanceSize2(size, ansiName_);
  advanceSize2(size, fileName_);
  
  size += sizeof(opens_);
  size += sizeof(openTime_);

  return size;
}

UInt32
ExPartitionAccessStats::pack(char* buffer)
{
  // first pack my base class (i.e., ExOperStats) into the buffer
  // ExOperStats better calls packBaseClassIntoMessage()!!!!!
  UInt32  size = ExOperStats::pack(buffer);
  buffer += size;

  UInt32 temp = exeSEStats()->alignedPack(buffer);
  buffer += temp;
  size += temp;

  temp = bufferStats()->alignedPack(buffer);
  buffer += temp;
  size += temp;

  size += packCharStarIntoBuffer(buffer, ansiName_);
  size += packCharStarIntoBuffer(buffer, fileName_);

  size += packIntoBuffer(buffer, opens_);
  size += packIntoBuffer(buffer, openTime_);

  return size;
}

void ExPartitionAccessStats::unpack(const char* &buffer)
{
  ExOperStats::unpack(buffer);

  exeSEStats()->alignedUnpack(buffer);

  bufferStats()->alignedUnpack(buffer);

  unpackBuffer(buffer, ansiName_, heap_);
  unpackBuffer(buffer, fileName_, heap_);
  unpackBuffer(buffer, opens_);
  unpackBuffer(buffer, openTime_);
}

ExOperStats * ExPartitionAccessStats::copyOper(NAMemory * heap)
{
  ExPartitionAccessStats* stat = new(heap) ExPartitionAccessStats(heap);
  stat->copyContents(this);
  return stat;
}

ExPartitionAccessStats * ExPartitionAccessStats::castToExPartitionAccessStats()
{
  return this;
}

const char * ExPartitionAccessStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "OperCpuTime";
    case 2:
      return "BuffersSent";
    case 3:
      return "BuffersRcvd";
    case 4:
      return "MsgBytesSent";
    case 5:
      return "MsgBytesRcvd";
    }
  return NULL;
}

Int64 ExPartitionAccessStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
	return ((ExBufferStats&)bufferStats_).sentBuffers().entryCnt();
	//      return bufferStats_.sentBuffers().entryCnt();
    case 3:
      return ((ExBufferStats&)bufferStats_).recdBuffers().entryCnt();
    case 4:
      return ((ExBufferStats&)bufferStats_).totalSentBytes();
    case 5:
      return ((ExBufferStats&)bufferStats_).totalRecdBytes();
    }
  return 0;
}

const char * ExPartitionAccessStats::getTextVal()
{
  return ansiName_;
}

void ExPartitionAccessStats::getVariableStatsInfo(char * dataBuffer,
						  char * dataLen,
						  Lng32 maxLen)
{
  char *buf = dataBuffer;

  ExOperStats::getVariableStatsInfo(dataBuffer, dataLen, maxLen);
  buf += *((short *) dataLen);

  sprintf(buf,
	      "AnsiName: %s PhysName: %s BuffersSize: %u BuffersSent: %u BuffersRcvd: %u NumMessages: %ld MsgBytes: %ld MsgBytesSent: %ld MsgBytesRcvd: %ld ",
	      ansiName_, fileName_,
	      bufferStats()->sendBufferSize(), bufferStats()->sentBuffers().entryCnt(), bufferStats()->recdBuffers().entryCnt(), 
	      exeSEStats()->getNumIOCalls(),
	      exeSEStats()->getNumIOBytes(),
	      bufferStats()->totalSentBytes(),
	      bufferStats()->totalRecdBytes());
  buf += str_len(buf);

  sprintf(buf,
    "SendUtilMin: %ld SendUtilMax: %ld SendUtilAvg: %f RecvUtilMin: %ld RecvUtilMax: %ld RecvUtilAvg: %f Opens: %u OpenTime: %ld ",
	      bufferStats()->sentBuffers().min(),
	      bufferStats()->sentBuffers().max(),
	      bufferStats()->sentBuffers().mean(), 
	      bufferStats()->recdBuffers().min(),
	      bufferStats()->recdBuffers().max(),
	      bufferStats()->recdBuffers().mean(),
              opens_,
              openTime_);
  buf += str_len(buf);

  *(short*)dataLen = (short) (buf - dataBuffer);
}

Lng32 ExPartitionAccessStats::getStatsItem(SQLSTATS_ITEM* sqlStats_item)
{
  ExOperStats::getStatsItem(sqlStats_item);
  if(sqlStats_item -> error_code == -EXE_STAT_NOT_FOUND)
  {  
    sqlStats_item->error_code = 0;
    switch (sqlStats_item->statsItem_id)
    {
    case SQLSTATS_TABLE_ANSI_NAME:
      if (sqlStats_item->str_value != NULL)
      {
        Lng32 len = str_len(ansiName_);
        if (len > sqlStats_item->str_max_len)
        {
          len = sqlStats_item->str_max_len;
          sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
        }
        str_cpy(sqlStats_item->str_value, ansiName_, len);
        sqlStats_item->str_ret_len = len;
      }
      break;
    case SQLSTATS_SE_IOS:
      sqlStats_item->int64_value = exeSEStats()->getNumIOCalls();
      break;
    case SQLSTATS_SE_IO_BYTES:
      sqlStats_item->int64_value = exeSEStats()->getNumIOBytes();
      break;
    case SQLSTATS_SE_IO_MAX_TIME:
      sqlStats_item->int64_value = exeSEStats()->getMaxIOTime();
      break;
    default:
      sqlStats_item->error_code = -EXE_STAT_NOT_FOUND;
      break;
    }
  }
  return 0;
}

//////////////////////////////////////////////////////////////////
// class ExHashGroupByStats
//////////////////////////////////////////////////////////////////
ExHashGroupByStats::ExHashGroupByStats(NAMemory * heap,
				       ex_tcb *tcb,
				       const ComTdb * tdb)
  : ExBMOStats(heap,
		GROUP_BY_STATS,
		tcb,
		tdb),
    partialGroups_(0),
    memSize_(0),
    ioSize_(0),
    clusterCnt_(0),
    clusterStats_(NULL),
    lastStat_(NULL) {}

ExHashGroupByStats::ExHashGroupByStats(NAMemory * heap)
  : ExBMOStats(heap, GROUP_BY_STATS),
    partialGroups_(0),
    memSize_(0),
    ioSize_(0),
    clusterCnt_(0),
    clusterStats_(NULL),
    lastStat_(NULL) {}

void ExHashGroupByStats::init(NABoolean resetDop)
{
  ExBMOStats::init(resetDop);
  partialGroups_ = 0;
  memSize_ = 0;
  ioSize_ = 0;
  ExClusterStats* i = clusterStats_;
  while (clusterCnt_) {
    ExClusterStats* j = i->getNext();
    NADELETE(i, ExClusterStats, heap_);
    clusterCnt_--;
    i = j;
  }
  clusterStats_ = NULL;
  lastStat_ = NULL;
}

void ExHashGroupByStats::copyContents(ExHashGroupByStats* other)
{
  ExBMOStats::copyContents(other);
  partialGroups_ = other->partialGroups_;
  memSize_ = other->memSize_;
  ioSize_ = other->ioSize_;
  ExClusterStats* clusterStats = other->clusterStats_;
  while (clusterStats) {
    addClusterStats(*clusterStats);
    clusterStats = clusterStats->getNext();
  }
}

void ExHashGroupByStats::merge(ExHashGroupByStats* other)
{
  ExBMOStats::merge(other);
  partialGroups_ += other -> partialGroups_;
  if (other -> memSize_ > memSize_)
    memSize_ = other -> memSize_;
  ioSize_ += other -> ioSize_;
  ExClusterStats* clusterStats = other -> clusterStats_;
  while (clusterStats) {
    addClusterStats(*clusterStats);
    clusterStats = clusterStats->getNext();
  }
}

UInt32 ExHashGroupByStats::packedLength()
{
  UInt32 size = ExBMOStats::packedLength();
  size += sizeof(partialGroups_);
  size += sizeof(memSize_);
  size += sizeof(ioSize_);
  size += sizeof(clusterCnt_);
  ExClusterStats* s = clusterStats_;

  for (ULng32 i=0; i < clusterCnt_; i++)
    {
      ex_assert(s,"Inconsistency between clusterCnt_ and clusterStats_");
      alignSizeForNextObj(size);
      size += s->packedLength();
      s = s->getNext();
    }

  return size;
}

UInt32
ExHashGroupByStats::pack(char * buffer)
{
  UInt32 size = ExBMOStats::pack(buffer);
  buffer += size;
  size += packIntoBuffer(buffer, partialGroups_);
  size += packIntoBuffer(buffer, memSize_);
  size += packIntoBuffer(buffer, ioSize_);
  size += packIntoBuffer(buffer, clusterCnt_);
  // pack all the clusterstats
  ExClusterStats* s = clusterStats_;
  for (ULng32 i=0; i < clusterCnt_; i++)
    {
      ex_assert(s, "clusterCnt_ and clusterStats_ are inconsistent");
 
      UInt32 clusterStatsSize = s->alignedPack(buffer);

      buffer += clusterStatsSize;
      size += clusterStatsSize;
      s = s->getNext();
    }

  return size;
}

void ExHashGroupByStats::unpack(const char* &buffer)
{
  ExBMOStats::unpack(buffer);
  unpackBuffer(buffer, partialGroups_);
  unpackBuffer(buffer, memSize_);
  unpackBuffer(buffer, ioSize_);
  ULng32 clusterCnt; // note: addClusterStats() updates the actual field clusterCnt_
  unpackBuffer(buffer, clusterCnt);
  // unpack the clusterstats. make sure that the are in the same order
  // in the chain as they were when they were packed
  for (ULng32 i = 0; i < clusterCnt; i++)
    {
      ExClusterStats* stats = new(heap_) ExClusterStats();
      stats->alignedUnpack(buffer);
      addClusterStats(stats);
    }
}

ExOperStats * ExHashGroupByStats::copyOper(NAMemory * heap)
{
  ExHashGroupByStats* stat = new(heap) ExHashGroupByStats(heap);
  stat->copyContents(this);
  return stat;
}

ExHashGroupByStats * ExHashGroupByStats::castToExHashGroupByStats()
{
  return this;
}

ExBMOStats *ExHashGroupByStats::castToExBMOStats()
{
  return (ExBMOStats *)this;
}

const char * ExHashGroupByStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "OperCpuTime";
    case 2:
      return "Memory";
    case 3:
      return "IOBytes";
    case 4:
      return "NumClusters";
    case 5:
      return "PartialGroups";
    }
  return NULL;
}

Int64 ExHashGroupByStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
      return memSize_;
    case 3:
      return ioSize_;
    case 4:
      return clusterCnt_;
    case 5:
      return partialGroups_;
    }
  return 0;
}

void ExHashGroupByStats::getVariableStatsInfo(char * dataBuffer,
					      char * dataLen,
					      Lng32 maxLen)
{
  char *buf = dataBuffer;

  ExOperStats::getVariableStatsInfo(dataBuffer, dataLen, maxLen);
  buf += *((short *) dataLen);

  sprintf(buf, "Memory: %ld IOBytes: %ld ", memSize_, ioSize_);
  buf += str_len(buf);

  if (partialGroups_)
    {
      sprintf(buf, "PartialGroups: %ld ", partialGroups_);
      buf += str_len(buf);
    }

  clusterStats_->getVariableStatsInfoAggr(buf,
					  dataLen,
					  maxLen - (buf - dataBuffer));
  buf += *((short *) dataLen);

  *(short*)dataLen = (short) (buf - dataBuffer);
}

void ExHashGroupByStats::addClusterStats(ExClusterStats* stats)
{
  clusterCnt_++;
  if (!clusterStats_)
    clusterStats_ = stats;
  else
    lastStat_->setNext(stats);
  lastStat_ = stats;
};


void ExHashGroupByStats::addClusterStats(ExClusterStats stats)
{
  ExClusterStats* s = new(heap_) ExClusterStats();
  *s = stats;
  addClusterStats(s);
};

void ExHashGroupByStats::deleteClusterStats()
{
  ExClusterStats* i = clusterStats_;
  while (clusterCnt_) {
    ExClusterStats* j = i->getNext();
    NADELETE(i, ExClusterStats, heap_);
    clusterCnt_--;
    i = j;
  } 
}

//////////////////////////////////////////////////////////////////
// class ExHashJoinStats
//////////////////////////////////////////////////////////////////
ExHashJoinStats::ExHashJoinStats(NAMemory * heap,
				 ex_tcb *tcb,
				 const ComTdb * tdb)
     : ExBMOStats(heap,
		   HASH_JOIN_STATS,
		   tcb,
		   tdb),
       phase_(0),
       memSize_(0),
       ioSize_(0),
       emptyChains_(0),
       clusterCnt_(0),
       clusterSplits_(0),
       hashLoops_(0),
       clusterStats_(NULL),
       lastStat_(NULL) {};

ExHashJoinStats::ExHashJoinStats(NAMemory * heap)
  : ExBMOStats(heap,
		HASH_JOIN_STATS),
    phase_(0),
    memSize_(0),
    ioSize_(0),
    emptyChains_(0),
    clusterCnt_(0),
    clusterSplits_(0),
    hashLoops_(0),
    clusterStats_(NULL),
    lastStat_(NULL) {};

void ExHashJoinStats::init(NABoolean resetDop)
{
  ExBMOStats::init(resetDop);
  for (short p = 0; p < 3; p++)
    phaseTimes_[p].reset();
  phase_ = 0;
  memSize_ = 0;
  ioSize_ = 0;
  emptyChains_ = 0;
  ExClusterStats* i = clusterStats_;
  while (clusterCnt_) {
    ExClusterStats* j = i->getNext();
    NADELETE(i, ExClusterStats, heap_);
    clusterCnt_--;
    i = j;
  }
  clusterSplits_ = 0;
  hashLoops_ = 0;
  clusterStats_ = NULL;
  lastStat_ = NULL;
}

UInt32 ExHashJoinStats::packedLength()
{
  UInt32 size = ExBMOStats::packedLength();
  size += sizeof(memSize_);
  size += sizeof(ioSize_);
  size += sizeof(emptyChains_);
  size += sizeof(clusterCnt_);
  size += sizeof(clusterSplits_);
  size += sizeof(hashLoops_);
  if (clusterCnt_)
    {
      ExClusterStats* s = clusterStats_;
      for (ULng32 i=0; i < clusterCnt_; i++)
	{
	  ex_assert(s, "clusterCnt_ and clusterStats_ are inconsistent");
	  alignSizeForNextObj(size);
	  size += s->packedLength();
	  s = s->getNext();
	}
    }

  for (Int32 i=0; i < 3; i++)
    {
      alignSizeForNextObj(size);
      size += phaseTimes_[i].packedLength();
    }
  return size;
}

UInt32
ExHashJoinStats::pack(char * buffer)
{
  UInt32 size = ExBMOStats::pack(buffer);
  buffer += size;
  size += packIntoBuffer(buffer, memSize_);
  size += packIntoBuffer(buffer, ioSize_);
  size += packIntoBuffer(buffer, emptyChains_);
  size += packIntoBuffer(buffer, clusterCnt_);
  size += packIntoBuffer(buffer, clusterSplits_);
  size += packIntoBuffer(buffer, hashLoops_);
  // pack all the clusterstats
  ExClusterStats* s = clusterStats_;
  for (ULng32 i=0; i < clusterCnt_; i++)
    {
      UInt32 clusterStatsSize = s->alignedPack(buffer);

      buffer += clusterStatsSize;
      size += clusterStatsSize;
      s = s->getNext();
    }
  for (short p = 0; p < 3; p++)
    {
      UInt32 phaseTimerSize = phaseTimes_[p].alignedPack(buffer);

      buffer += phaseTimerSize;
      size += phaseTimerSize;
    }
  return size;
}

void ExHashJoinStats::unpack(const char* &buffer)
{
  ExBMOStats::unpack(buffer);
  unpackBuffer(buffer, memSize_);
  unpackBuffer(buffer, ioSize_);
  unpackBuffer(buffer, emptyChains_);
  ULng32 clusterCnt; // note: addClusterStats() updates the actual field clusterCnt_
  unpackBuffer(buffer, clusterCnt);
  unpackBuffer(buffer, clusterSplits_);
  unpackBuffer(buffer, hashLoops_);
  // unpack the clusterstats. make sure that the are in the same order
  // in the chain as they were when they were packed
  for (ULng32 i = 0; i < clusterCnt; i++)
    {
      ExClusterStats* stats = new(heap_) ExClusterStats();

      stats->alignedUnpack(buffer);

      addClusterStats(stats);
    }
  for (short p = 0; p < 3; p++)
    {
      phaseTimes_[p].alignedUnpack(buffer);
    }
}

void ExHashJoinStats::copyContents(ExHashJoinStats* other)
{
  ExBMOStats::copyContents(other);

  memSize_ = other->memSize_;
  ioSize_ = other->ioSize_;

  for (short p = 0; p < 3; p++)
    phaseTimes_[p] = other->phaseTimes_[p];
  emptyChains_ = other->emptyChains_;
  ExClusterStats* clusterStats = other->clusterStats_;
  while (clusterStats) {
    addClusterStats(*clusterStats);
    clusterStats = clusterStats->getNext();
  }
  clusterSplits_ = other->clusterSplits_;
  hashLoops_ = other->hashLoops_;
}

void ExHashJoinStats::merge(ExHashJoinStats* other)
{
  ExBMOStats::merge(other);

  if (other -> memSize_ > memSize_)
    memSize_ = other -> memSize_;

  ioSize_ += other -> ioSize_;

  for (short p = 0; p < 3; p++)
    phaseTimes_[p] = phaseTimes_[p] + other -> phaseTimes_[p];
  emptyChains_ += other -> emptyChains_;
  ExClusterStats* clusterStats = other -> clusterStats_;
  while (clusterStats) {
    addClusterStats(*clusterStats);
    clusterStats = clusterStats->getNext();
  }
  clusterSplits_ += other->clusterSplits_;
  hashLoops_ += other->hashLoops_;
}

ExOperStats * ExHashJoinStats::copyOper(NAMemory * heap)
{
  ExHashJoinStats * stat =  new(heap) ExHashJoinStats(heap);
  stat->copyContents(this);
  return stat;
}

ExHashJoinStats * ExHashJoinStats::castToExHashJoinStats()
{
  return this;
}

ExBMOStats *ExHashJoinStats::castToExBMOStats()
{
  return (ExBMOStats *)this;
}

const char * ExHashJoinStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "OperCpuTime";
    case 2:
      return "Memory";
    case 3:
      return "IOBytes";
    case 4:
      return "NumClusters";
    }
  return NULL;
}

Int64 ExHashJoinStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
      return memSize_;
    case 3:
      return ioSize_;
    case 4:
      return clusterCnt_;
    }
  return 0;
}

void ExHashJoinStats::getVariableStatsInfo(char * dataBuffer,
					   char * dataLen,
					   Lng32 maxLen)
{
  char *buf = dataBuffer;

  ExOperStats::getVariableStatsInfo(dataBuffer, dataLen, maxLen);
  buf += *((short *) dataLen);

  sprintf(buf, "Memory: %ld IOBytes: %ld ", memSize_, ioSize_);
  buf += str_len(buf);

  if ( clusterSplits_ || hashLoops_ ) {
    sprintf(buf, "ClusterSplits: %u HashLoops: %u ", clusterSplits_, hashLoops_);
    buf += str_len(buf);
  }

  // more to be done here: Phase1CPUTime, Phase2CPUTime, Phase3CPUTime,
  // Phase1ElapsedTime, Phase2ElapsedTime, Phase3ElapsedTime, EmptyChainHits

  clusterStats_->getVariableStatsInfoAggr(buf,
					  dataLen,
					  maxLen - (buf - dataBuffer));
  buf += *((short *) dataLen);

  *(short*)dataLen = (short) (buf - dataBuffer);
}

void ExHashJoinStats::addClusterStats(ExClusterStats* stats)
{
  clusterCnt_++;
  if (!clusterStats_)
    clusterStats_ = stats;
  else
    lastStat_->setNext(stats);
  lastStat_ = stats;
};


void ExHashJoinStats::addClusterStats(ExClusterStats stats)
{
  ExClusterStats* s = new(heap_) ExClusterStats();
  *s = stats;
  addClusterStats(s);
};

void ExHashJoinStats::deleteClusterStats()
{
  ExClusterStats* i = clusterStats_;
  while (clusterCnt_) {
    ExClusterStats* j = i->getNext();
    NADELETE(i, ExClusterStats, heap_);
    clusterCnt_--;
    i = j;
  } 
}
////////////////////////////////////////////////////////////////
// class ExESPStats
////////////////////////////////////////////////////////////////
ExESPStats::ExESPStats(NAMemory * heap,
		       ULng32 sendBufferSize,
		       ULng32 recdBufferSize,
		       Lng32 numSubInst,
		       ex_tcb *tcb,
		       const ComTdb * tdb)
  : ExOperStats(heap,
		ESP_STATS,
		tcb,
		tdb),
    sendTopStatID_(-1)
{
  setSubInstNum(numSubInst);

  bufferStats()->sendBufferSize() = sendBufferSize;
  bufferStats()->recdBufferSize() = recdBufferSize;
}

ExESPStats::ExESPStats(NAMemory * heap)
  : ExOperStats(heap, ESP_STATS),
    sendTopStatID_(-1){}

void ExESPStats::init(NABoolean resetDop)
{
  ExOperStats::init(resetDop);

  bufferStats_.init(resetDop);
}

UInt32 ExESPStats::packedLength()
{
  UInt32 size = ExOperStats::packedLength();

  alignSizeForNextObj(size);
  size += bufferStats()->packedLength();

  size += sizeof(sendTopStatID_);

  return size;
}

UInt32 ExESPStats::pack(char * buffer)
{
  UInt32 size = ExOperStats::pack(buffer);
  buffer += size;

  UInt32 temp = bufferStats()->alignedPack(buffer);
  buffer += temp;
  size += temp;

  size += packIntoBuffer(buffer, sendTopStatID_);

  return size;
}

void ExESPStats::unpack(const char* &buffer)
{
  ExOperStats::unpack(buffer);

  bufferStats()->alignedUnpack(buffer);

  unpackBuffer(buffer, sendTopStatID_);
}

void ExESPStats::copyContents(ExESPStats* other)
{
  ExOperStats::copyContents(other);

  bufferStats()->copyContents(other->bufferStats());

  sendTopStatID_ = other->sendTopStatID_;
}

void ExESPStats::merge(ExESPStats* other)
{
  ExOperStats::merge(other);
  
  bufferStats()->merge(other -> bufferStats());
  
  sendTopStatID_ = other -> sendTopStatID_;
}

ExOperStats * ExESPStats::copyOper(NAMemory * heap)
{
  ExESPStats * stat = new(heap) ExESPStats(heap);
  stat->copyContents(this);
  return stat;
}

ExESPStats * ExESPStats::castToExESPStats()
{
  return this;
}

const char * ExESPStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "OperCpuTime";
    case 2:
      return "SendCount";
    case 3:
      return "RecvCount";
    case 4:
      return "SendBytes";
    case 5:
      return "RecvBytes";
    }
  return NULL;
}

Int64 ExESPStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
      return ((ExBufferStats&)bufferStats_).sentBuffers().entryCnt();
    case 3:
      return ((ExBufferStats&)bufferStats_).recdBuffers().entryCnt();
    case 4:
      return ((ExBufferStats&)bufferStats_).totalSentBytes();
    case 5:
      return ((ExBufferStats&)bufferStats_).totalRecdBytes();
    }
  return 0;
}

void ExESPStats::getVariableStatsInfo(char * dataBuffer,
				      char * dataLen,
				      Lng32 maxLen)
{
  char *buf = dataBuffer;

  ExOperStats::getVariableStatsInfo(dataBuffer, dataLen, maxLen);
  buf += *((short *) dataLen);
  maxLen -= (buf - dataBuffer);

  sprintf(buf, "SendBufferSize: %u ", bufferStats()->sendBufferSize());
  buf += str_len(buf);
  
  sprintf(buf,
	      "BuffersSent: %u MsgBytesSent: %ld SendUtilMin: %ld SendUtilMax: %ld SendUtilAvg: %f ",
	      bufferStats()->sentBuffers().entryCnt(),
	      bufferStats()->totalSentBytes(),
	      bufferStats()->sentBuffers().min(),
	      bufferStats()->sentBuffers().max(),
	      bufferStats()->sentBuffers().mean());
  buf += str_len(buf);

  sprintf(buf, "RecvBufferSize: %u ", bufferStats()->recdBufferSize());
  buf += str_len(buf);

  sprintf(buf,
	      "BuffersRcvd: %u MsgBytesRcvd: %ld RecvUtilMin: %ld RecvUtilMax: %ld RecvUtilAvg: %f ",
	      bufferStats()->recdBuffers().entryCnt(),
	      bufferStats()->totalRecdBytes(),
	      bufferStats()->recdBuffers().min(),
	      bufferStats()->recdBuffers().max(),
	      bufferStats()->recdBuffers().mean());
  buf += str_len(buf);

  *(short*)dataLen = (short) (buf - dataBuffer);
}

////////////////////////////////////////////////////////////////
// class ExSplitTopStats
////////////////////////////////////////////////////////////////
ExSplitTopStats::ExSplitTopStats(NAMemory * heap,
				 ex_tcb *tcb,
				 const ComTdb * tdb)
  : ExOperStats(heap,
		SPLIT_TOP_STATS,
		tcb,
		tdb),
    maxChildren_(0),
    actChildren_(0)
{
  maxChildren_ = ((ComTdbSplitTop*)tdb)->getBottomNumParts();
}

ExSplitTopStats::ExSplitTopStats(NAMemory * heap)
  : ExOperStats(heap, SPLIT_TOP_STATS),
    maxChildren_(0),
    actChildren_(0)
{}

void ExSplitTopStats::init(NABoolean resetDop)
{
  ExOperStats::init(resetDop);
  actChildren_ = 0;
}

UInt32 ExSplitTopStats::packedLength()
{
  UInt32 size = ExOperStats::packedLength();

  size += sizeof(maxChildren_);
  size += sizeof(actChildren_);
  return size;
}

UInt32 ExSplitTopStats::pack(char * buffer)
{
  UInt32 size = ExOperStats::pack(buffer);
  buffer += size;

  size += packIntoBuffer(buffer, maxChildren_);
  size += packIntoBuffer(buffer, actChildren_);
  return size;
}

void ExSplitTopStats::unpack(const char* &buffer)
{
  ExOperStats::unpack(buffer);

  unpackBuffer(buffer, maxChildren_);
  unpackBuffer(buffer, actChildren_);
}

void ExSplitTopStats::merge(ExSplitTopStats* other)
{
  ExOperStats::merge(other);
  maxChildren_ += other -> maxChildren_;
  actChildren_ += other -> actChildren_;
}

ExOperStats * ExSplitTopStats::copyOper(NAMemory * heap)
{
  ExSplitTopStats * stat = new(heap) ExSplitTopStats(heap);
  stat->copyContents(this);
  return stat;
}

void ExSplitTopStats::copyContents(ExSplitTopStats * other)
{
  ExOperStats::copyContents(other);
  maxChildren_  = other->maxChildren_;
  actChildren_ = other->actChildren_;
}

ExSplitTopStats * ExSplitTopStats::castToExSplitTopStats()
{
  return this;
}

const char * ExSplitTopStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "OperCpuTime";
    case 2:
      return "MaxChildren";
    case 3:
      return "ActChildren";
    }
  return NULL;
}

Int64 ExSplitTopStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
      return maxChildren_;
    case 3:
      return actChildren_;
    }
  return 0;
}

void ExSplitTopStats::getVariableStatsInfo(char * dataBuffer,
					   char * dataLen,
					   Lng32 maxLen)
{
  char *buf = dataBuffer;

  ExOperStats::getVariableStatsInfo(dataBuffer, dataLen, maxLen);
  buf += *((short *) dataLen);
  maxLen -= (buf - dataBuffer);

  sprintf(buf, "MaxChildren: %u ActChildren: %u", 
	      maxChildren_, actChildren_);
  buf += str_len(buf);
  
  *(short*)dataLen = (short) (buf - dataBuffer);
}

//////////////////////////////////////////////////////////////////
// class ExSortStats
//////////////////////////////////////////////////////////////////
ExSortStats::ExSortStats(NAMemory * heap,
			 ex_tcb *tcb,
			 const ComTdb * tdb)
     : ExBMOStats(heap,
		   SORT_STATS,
		   tcb,
		   tdb),
       runSize_(0),          
       numRuns_(0),
       numCompares_(0), 
       numDupRecs_(0),
       
       scrBlockSize_(0),
       scrNumBlocks_(0),
       scrNumWrites_(0),
       scrNumReads_(0), 
       scrNumAwaitio_(0)
{}

ExSortStats::ExSortStats(NAMemory * heap)
     : ExBMOStats(heap, SORT_STATS),
       runSize_(0),          
       numRuns_(0),
       numCompares_(0), 
       numDupRecs_(0),
       
       scrBlockSize_(0),
       scrNumBlocks_(0),
       scrNumWrites_(0),
       scrNumReads_(0), 
       scrNumAwaitio_(0)
{}

void ExSortStats::init(NABoolean resetDop)
{
  ExBMOStats::init(resetDop);

  runSize_ = 0;          
  numRuns_ = 0;
  numCompares_ = 0; 
  numDupRecs_ = 0;
  
  scrBlockSize_ = 0;
  scrNumBlocks_ = 0;
  scrNumWrites_ = 0;
  scrNumReads_ = 0; 
  scrNumAwaitio_ = 0;
}

void ExSortStats::copyContents(ExSortStats* other)
{
  ExBMOStats::copyContents(other);

  runSize_ = other->runSize_;          
  numRuns_ = other->numRuns_;
  numCompares_ = other->numCompares_; 
  numDupRecs_ = other->numDupRecs_;
  
  scrBlockSize_ = other->scrBlockSize_;
  scrNumBlocks_ = other->scrNumBlocks_;
  scrNumWrites_ = other->scrNumWrites_;
  scrNumReads_ = other->scrNumReads_; 
  scrNumAwaitio_ = other->scrNumAwaitio_;
}

void ExSortStats::merge(ExSortStats* other)
{
  ExBMOStats::merge(other);

  runSize_ += other -> runSize_;          
  numRuns_ += other -> numRuns_;
  numCompares_ += other -> numCompares_; 
  numDupRecs_ += other -> numDupRecs_;
  
  scrBlockSize_ += other -> scrBlockSize_;
  scrNumBlocks_ += other -> scrNumBlocks_;
  scrNumWrites_ += other -> scrNumWrites_;
  scrNumReads_ += other -> scrNumReads_; 
  scrNumAwaitio_ += other -> scrNumAwaitio_;
}

UInt32 ExSortStats::packedLength()
{
  UInt32 size = ExBMOStats::packedLength();
  
  size += sizeof(runSize_);          
  size += sizeof(numRuns_);
  size += sizeof(numCompares_); 
  size += sizeof(numDupRecs_);
  
  size += sizeof(scrBlockSize_);
  size += sizeof(scrNumBlocks_);
  size += sizeof(scrNumWrites_);
  size += sizeof(scrNumReads_); 
  size += sizeof(scrNumAwaitio_);
  
  return size;
}

UInt32 ExSortStats::pack(char * buffer)
{
  UInt32 size = ExBMOStats::pack(buffer);
  buffer += size;
  //  size += packIntoBuffer(buffer, ioSize_);
  
  size += packIntoBuffer(buffer,runSize_);          
  size += packIntoBuffer(buffer,numRuns_);
  size += packIntoBuffer(buffer,numCompares_); 
  size += packIntoBuffer(buffer,numDupRecs_);
  
  size += packIntoBuffer(buffer,scrBlockSize_);
  size += packIntoBuffer(buffer,scrNumBlocks_);
  size += packIntoBuffer(buffer,scrNumWrites_);
  size += packIntoBuffer(buffer,scrNumReads_); 
  size += packIntoBuffer(buffer,scrNumAwaitio_);

  return size;
}

void ExSortStats::unpack(const char* &buffer)
{
  ExBMOStats::unpack(buffer);
  unpackBuffer(buffer, runSize_);          
  unpackBuffer(buffer, numRuns_);
  unpackBuffer(buffer, numCompares_); 
  unpackBuffer(buffer, numDupRecs_);
  
  unpackBuffer(buffer, scrBlockSize_);
  unpackBuffer(buffer, scrNumBlocks_);
  unpackBuffer(buffer, scrNumWrites_);
  unpackBuffer(buffer, scrNumReads_); 
  unpackBuffer(buffer, scrNumAwaitio_);
}

ExOperStats * ExSortStats::copyOper(NAMemory * heap)
{
  ExSortStats* stat = new(heap) ExSortStats(heap);
  stat->copyContents(this);
  return stat;
}

ExSortStats * ExSortStats::castToExSortStats()
{
  return this;
}

ExBMOStats *ExSortStats::castToExBMOStats()
{
  return (ExBMOStats *)this;
}

const char * ExSortStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "OperCpuTime";
    case 2:
      return "RunSize";
    case 3:
      return "NumRuns";
    case 4:
      return "NumScratchBlocks";
    case 5:
      return "NumScratchWrites";
    }
  return NULL;
}

Int64 ExSortStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
      return runSize_;
    case 3:
      return numRuns_; //ioSize_;
    case 4:
      return scrNumBlocks_; 
    case 5:
      return scrNumWrites_; 
    }
  return 0;
}

void ExSortStats::getVariableStatsInfo(char * dataBuffer,
				       char * dataLen,
				       Lng32 maxLen)
{
  char *buf = dataBuffer;

  ExOperStats::getVariableStatsInfo(dataBuffer, dataLen, maxLen);
  buf += *((short *) dataLen);

  sprintf(buf, "RunSize: %u NumRuns: %u NumCompares: %u NumDupRecs: %u ScratchBlockSize: %u NumScratchBlocks: %u NumScratchWrites: %u NumScratchReads: %u", 
	     runSize_, numRuns_, numCompares_, numDupRecs_,
	      scrBlockSize_, scrNumBlocks_, scrNumWrites_, scrNumReads_);
  buf += str_len(buf);

  //  buf += *((short *) dataLen);

  *(short*)dataLen = (short) (buf - dataBuffer);
}

//////////////////////////////////////////////////////////////////
// class ExMeasBaseStats
//////////////////////////////////////////////////////////////////
ExMeasBaseStats::ExMeasBaseStats(NAMemory * heap,
				 StatType statType,
				 ex_tcb * tcb,
				 const ComTdb * tdb)
     : ExOperStats(heap,
                   statType,
		   tcb, tdb)
{
}

ExMeasBaseStats::ExMeasBaseStats(NAMemory * heap, StatType statType)
     : ExOperStats(heap,
	           statType,
		   NULL, NULL)
{  
}

ExMeasBaseStats * ExMeasBaseStats::castToExMeasBaseStats()
{
  return this;
}

UInt32 ExMeasBaseStats::packedLength() {
  UInt32 size = 0;
  size = ExOperStats::packedLength();
  alignSizeForNextObj(size);
  size += sizeof(getEstRowsAccessed());
  size += sizeof(getEstRowsUsed());
  size += sizeof(exeSEStats()->getAccessedRows());
  size += sizeof(exeSEStats()->getUsedRows());
  size += sizeof(exeSEStats()->getNumIOCalls());
  size += sizeof(exeSEStats()->getNumIOBytes());
  size += sizeof(exeSEStats()->getMaxIOTime());
  return size;
}

UInt32
ExMeasBaseStats::pack(char * buffer) {
  UInt32 size = 0;
  size = ExOperStats::pack(buffer);
  alignSizeForNextObj(size);
  buffer += size;
  size += packIntoBuffer(buffer, getEstRowsAccessed());
  size += packIntoBuffer(buffer, getEstRowsUsed());
  size += packIntoBuffer(buffer, exeSEStats()->getAccessedRows());
  size += packIntoBuffer(buffer, exeSEStats()->getUsedRows());
  size += packIntoBuffer(buffer, exeSEStats()->getNumIOCalls());
  size += packIntoBuffer(buffer, exeSEStats()->getNumIOBytes());
  size += packIntoBuffer(buffer, exeSEStats()->getMaxIOTime());
  return size;
}

void ExMeasBaseStats::unpack(const char* &buffer) {
   ExOperStats::unpack(buffer);
   alignBufferForNextObj(buffer);
   float temp;
   unpackBuffer(buffer, temp);
   setEstRowsAccessed(temp);
   unpackBuffer(buffer, temp);
   setEstRowsUsed(temp);
   Int64 temp1;
   unpackBuffer(buffer, temp1);
   exeSEStats()->setAccessedRows(temp1);
   unpackBuffer(buffer, temp1);
   exeSEStats()->setUsedRows(temp1);
   unpackBuffer(buffer, temp1);
   exeSEStats()->setNumIOCalls(temp1);
   unpackBuffer(buffer, temp1);
   exeSEStats()->setNumIOBytes(temp1);
   unpackBuffer(buffer, temp1);
   exeSEStats()->setMaxIOTime(temp1);
}

void ExMeasBaseStats::init(NABoolean resetDop) {
  ExOperStats::init(resetDop);
  exeSEStats()->init(resetDop);
}

void ExMeasBaseStats::merge(ExMeasBaseStats* other)
{
  ExOperStats::merge(other);
  exeSEStats()    -> merge(other -> exeSEStats());
}

void ExMeasBaseStats::copyContents(ExMeasBaseStats * other) 
{
   ExOperStats::copyContents(other);
  char * srcPtr = (char *)other+sizeof(ExOperStats);
  char * destPtr = (char *)this+sizeof(ExOperStats);
  UInt32 srcLen = sizeof(ExMeasBaseStats)-sizeof(ExOperStats);
  memcpy((void *)destPtr, (void *)srcPtr, srcLen);
}

const char * ExMeasBaseStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "OperCpuTime";
    case 2:
      return "MessageBytes";
    case 3:
      return "AccessedDP2Rows";
    case 4:
      return "DiskReads";
    }
  return NULL;
}

Int64 ExMeasBaseStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
      return seStats_.getNumIOBytes();
    case 3:
      return seStats_.getAccessedRows();
    case 4:
      return seStats_.getNumIOCalls();
    }
  return 0;
}

void ExMeasBaseStats::getVariableStatsInfo(char * dataBuffer, char * datalen, 
				       Lng32 maxLen)
{
  char * buf = dataBuffer;

  sprintf (buf, 
	   "NumMessages: " PF64 " MessagesBytes: " PF64 " StatsBytes: %d  AccessedRows: " PF64 " UsedRows: " PF64 " DiskIOs: " PF64 "  MaxIOTime: " PF64 " ",
	      exeSEStats()->getNumIOCalls(),
	      exeSEStats()->getNumIOBytes(),
              0,
	      exeSEStats()->getAccessedRows(),
	      exeSEStats()->getUsedRows(),
	      exeSEStats()->getNumIOCalls(),
              exeSEStats()->getMaxIOTime()
              );
  buf += str_len(buf);

  // dataLen is really the varchar indicator
  *(short *)datalen = (short) (buf - dataBuffer);
}

void ExMeasBaseStats::setVersion(Lng32 version)
{
}
//////////////////////////////////////////////////////////////////
// class ExMeasStats
//////////////////////////////////////////////////////////////////
ExMeasStats::ExMeasStats(NAMemory * heap,
			 ex_tcb * tcb,
			 const ComTdb * tdb)
     : ExMeasBaseStats(heap, MEAS_STATS, tcb, NULL)
{
  executionCount_ = 0;
  init(FALSE);
  initHistory();
  if (tdb != NULL)
    scratchOverflowMode_ = ((ComTdb *)tdb)->getOverFlowMode();
  queryId_ = NULL;
  queryIdLen_ = 0;
}

ExMeasStats::ExMeasStats(NAMemory * heap)
     : ExMeasBaseStats(heap, MEAS_STATS)
{  
  executionCount_ = 0;
  init(FALSE);
  initHistory();
  queryId_ = NULL;
  queryIdLen_ = 0;
}

ExMeasStats::~ExMeasStats()
{
  if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS && queryId_ != NULL)
  {
    NADELETEBASIC(queryId_, getHeap());
    queryId_ = NULL;
  }
}

void ExMeasStats::initHistory()
{
}

UInt32 ExMeasStats::packedLength()
{
  UInt32 size;
  if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS)
  {
    size = ExMeasBaseStats::packedLength();
    size += sizeof(queryIdLen_);
    size += queryIdLen_;
    size += sizeof(spaceUsage_);
    size += sizeof(spaceAlloc_);
    size += sizeof(heapUsage_);
    size += sizeof(heapAlloc_);
    size += sizeof(heapWM_);
    size += sizeof(cpuTime_);
    size += sizeof(executionCount_);
    size += sizeof(phandle_);
  }
  else
  if ((Int32)getCollectStatsType() == SQLCLI_QID_DETAIL_STATS)
  {
    size = ExMeasBaseStats::packedLength();
    size += sizeof(spaceUsage_);
    size += sizeof(spaceAlloc_);
    size += sizeof(heapUsage_);
    size += sizeof(heapAlloc_);
    size += sizeof(heapWM_);
    size += sizeof(cpuTime_);
    size += sizeof(reqMsgCnt_);
    size += sizeof(reqMsgBytes_);
    size += sizeof(replyMsgCnt_);
    size += sizeof(replyMsgBytes_);
    size += sizeof(executionCount_);
    size += sizeof(phandle_);
  }
  else
  {
    size = ExMeasBaseStats::packedLength();
    alignSizeForNextObj(size);
    size += sizeof(ExMeasStats)-sizeof(ExMeasBaseStats);
  }
  return size;
}

UInt32 ExMeasStats::pack(char * buffer)
{
  UInt32 srcLen = 0;

  UInt32 size;
  if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS)
  {
    size = ExMeasBaseStats::pack(buffer);
    buffer += size;
    size += packIntoBuffer(buffer, queryIdLen_);
    if (queryIdLen_ != 0 && queryId_ != NULL)
      size += packStrIntoBuffer(buffer, queryId_, queryIdLen_);
    size += packIntoBuffer(buffer, spaceUsage_);
    size += packIntoBuffer(buffer, spaceAlloc_);
    size += packIntoBuffer(buffer, heapUsage_);
    size += packIntoBuffer(buffer, heapAlloc_);
    size += packIntoBuffer(buffer, heapWM_);
    size += packIntoBuffer(buffer, cpuTime_);  
    size += packIntoBuffer(buffer, executionCount_);
    memcpy(buffer, (const void *)&phandle_, sizeof(phandle_));
    size += sizeof(phandle_);
    buffer += sizeof(phandle_);
  }
  else
  if ((Int32)getCollectStatsType() == SQLCLI_QID_DETAIL_STATS)
  {
    size = ExMeasBaseStats::pack(buffer);
    buffer += size;
    size += packIntoBuffer(buffer, spaceUsage_);
    size += packIntoBuffer(buffer, spaceAlloc_);
    size += packIntoBuffer(buffer, heapUsage_);
    size += packIntoBuffer(buffer, heapAlloc_);
    size += packIntoBuffer(buffer, heapWM_);
    size += packIntoBuffer(buffer, cpuTime_);  
    size += packIntoBuffer(buffer, reqMsgCnt_);
    size += packIntoBuffer(buffer, reqMsgBytes_);
    size += packIntoBuffer(buffer, replyMsgCnt_);
    size += packIntoBuffer(buffer, replyMsgBytes_);  
    size += packIntoBuffer(buffer, executionCount_);
    memcpy(buffer, (const void *)&phandle_, sizeof(phandle_));
    size += sizeof(phandle_);
    buffer += sizeof(phandle_);
  }
  else
  {
    size = ExMeasBaseStats::pack(buffer);
    alignSizeForNextObj(size);
    buffer += size;
    srcLen = sizeof(ExMeasStats)-sizeof(ExMeasBaseStats);
    char * srcPtr = (char *)this+sizeof(ExMeasBaseStats);
    memcpy(buffer, (void *)srcPtr, srcLen);
    size += srcLen;
  }
  return size;
}

void ExMeasStats::unpack(const char* &buffer)
{
  ExMeasBaseStats::unpack(buffer);
  if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS)
  {
    unpackBuffer(buffer, queryIdLen_);
    if (queryIdLen_ != 0)
    {
      queryId_ = new ((NAHeap *)(getHeap())) char[queryIdLen_+1];
      unpackStrFromBuffer(buffer, queryId_, queryIdLen_);
      queryId_[queryIdLen_] = '\0';
    }
    unpackBuffer(buffer, spaceUsage_);
    unpackBuffer(buffer, spaceAlloc_);
    unpackBuffer(buffer, heapUsage_);
    unpackBuffer(buffer, heapAlloc_);
    unpackBuffer(buffer, heapWM_);
    unpackBuffer(buffer, cpuTime_);
    unpackBuffer(buffer, executionCount_);
    memcpy((void *)&phandle_, buffer, sizeof(phandle_));
    buffer += sizeof(phandle_);
  }
  else
  if ((Int32)getCollectStatsType() == SQLCLI_QID_DETAIL_STATS)
  {
    unpackBuffer(buffer, spaceUsage_);
    unpackBuffer(buffer, spaceAlloc_);
    unpackBuffer(buffer, heapUsage_);
    unpackBuffer(buffer, heapAlloc_);
    unpackBuffer(buffer, heapWM_);
    unpackBuffer(buffer, cpuTime_);
    unpackBuffer(buffer, reqMsgCnt_);
    unpackBuffer(buffer, reqMsgBytes_);
    unpackBuffer(buffer, replyMsgCnt_);
    unpackBuffer(buffer, replyMsgBytes_);
    unpackBuffer(buffer, executionCount_);
    memcpy((void *)&phandle_, buffer, sizeof(phandle_));
    buffer += sizeof(phandle_);
  }
  else
  {
     alignBufferForNextObj(buffer); 
     UInt32 srcLen = sizeof(ExMeasStats)-sizeof(ExMeasBaseStats);
     char * srcPtr = (char *)this+sizeof(ExMeasBaseStats);
     memcpy((void *)srcPtr, buffer, srcLen);
     buffer += srcLen;
     if (statsInEsp())
     {
        espSpaceUsage_ += spaceUsage_;
        espSpaceAlloc_ += spaceAlloc_;
        espHeapUsage_  += heapUsage_;
        espHeapAlloc_ += heapAlloc_;
        espHeapAlloc_ += heapWM_;
        espCpuTime_ += cpuTime_;
        spaceUsage_ = 0;
        spaceAlloc_ = 0;
        heapUsage_ = 0;
        heapAlloc_ = 0;
        heapWM_ = 0;
        cpuTime_ = 0;
    }
  }
}

void ExMeasStats::init(NABoolean resetDop)
{
  ExMeasBaseStats::init(resetDop);
  newprocess_ = 0;
  newprocessTime_ = 0;
  timeouts_ = 0;
  numSorts_ = 0;
  sortElapsedTime_ = 0;
  spaceUsage_ = 0;
  spaceAlloc_ = 0;
  heapUsage_ = 0;
  heapAlloc_ = 0;
  heapWM_ = 0;
  cpuTime_ = 0;
  espSpaceUsage_ = 0;
  espSpaceAlloc_ = 0;
  espHeapUsage_ = 0;
  espHeapAlloc_ = 0;
  espHeapWM_ = 0;
  espCpuTime_ = 0;
  histCpuTime_ = 0;
  reqMsgCnt_ = 0;
  reqMsgBytes_ = 0;
  replyMsgCnt_ = 0;
  replyMsgBytes_ = 0;
  executionCount_++;
  XPROCESSHANDLE_GETMINE_(&phandle_);
  isFragSuspended_ = false;
  localCpuTime_ = 0;
  scratchOverflowMode_ = -1;
  scratchFileCount_ = 0;
  spaceBufferSize_ = 0;
  spaceBufferCount_ = 0;
  scratchIOSize_ = 0;
  interimRowCount_ = 0;
  scratchIOMaxTime_ = 0;
  scratchWriteCount_ = 0;
  scratchReadCount_ = 0;
  udrCpuTime_ = 0;
  topN_ = -1;
}

void ExMeasStats::merge(ExFragRootOperStats* other)
{
  ExOperStats::merge(other);
  spaceUsage_ += other -> spaceUsage_; 
  spaceAlloc_ += other -> spaceAlloc_; 
  heapUsage_  += other -> heapUsage_; 
  heapAlloc_  += other -> heapAlloc_;
  heapWM_  += other -> heapWM_;
  if (scratchOverflowMode_ == -1)
    scratchOverflowMode_ = other->scratchOverflowMode_;
  cpuTime_          += other -> cpuTime_;
  newprocess_       += other -> newprocess_; 
  newprocessTime_   += other -> newprocessTime_; 
  espSpaceUsage_ += other -> espSpaceUsage_;
  espSpaceAlloc_ += other -> espSpaceAlloc_;
  espHeapUsage_  += other -> espHeapUsage_;
  espHeapAlloc_  += other -> espHeapAlloc_;
  espHeapWM_     += other -> espHeapWM_;
  espCpuTime_       += other -> espCpuTime_;
  reqMsgCnt_        += other -> reqMsgCnt_;
  reqMsgBytes_      += other -> reqMsgBytes_;
  replyMsgCnt_      += other -> replyMsgCnt_;
  replyMsgBytes_    += other -> replyMsgBytes_;
}

void ExMeasStats::merge(ExUDRBaseStats *other)
{
  reqMsgCnt_        += other->reqMsgCnt_;
  reqMsgBytes_      += other->reqMsgBytes_;
  replyMsgCnt_      += other->replyMsgCnt_;
  replyMsgBytes_    += other->replyMsgBytes_;
  udrCpuTime_       += other->udrCpuTime_;
}

void ExMeasStats::merge(ExBMOStats *other)
{
  scratchFileCount_ += other->scratchFileCount_;
  scratchOverflowMode_ = other->scratchOverflowMode_;
  if (spaceBufferSize_ == 0 &&
     other->spaceBufferSize_ > 0)
     spaceBufferSize_ = other->spaceBufferSize_;
  Float32 mFactor = 1;
  if(spaceBufferSize_ > 0)
    mFactor = (Float32)other->spaceBufferSize_ / spaceBufferSize_;
  spaceBufferCount_ += Int32 (other->spaceBufferCount_ * mFactor);
  mFactor = 1;
  if(scratchIOSize_ > 0)
    mFactor = (Float32)other->scratchIOSize_ / scratchIOSize_;
  scratchReadCount_ += (other->scratchReadCount_ * mFactor);
  scratchWriteCount_ += (other->scratchWriteCount_ * mFactor);
  scratchIOMaxTime_ += other->scratchIOMaxTime_;   
  interimRowCount_ += other->interimRowCount_;
  if (topN_ == -1 && other->topN_ > 0)
      topN_ = other->topN_;
}

void ExMeasStats::merge(ExStorageEngineStats* other)
{
  exeSEStats()->incAccessedRows(other->rowsAccessed());
  exeSEStats()->incUsedRows(other->rowsUsed());
  exeSEStats()->incNumIOCalls(0);
  exeSEStats()->incNumIOBytes(other->numBytesRead());
  exeSEStats()->incMaxIOTime(other->maxHdfsIOTime());
}

void ExMeasStats::merge(ExMeasStats* other)
{
  ExMeasBaseStats::merge(other);
  newprocess_      += other -> newprocess_; 
  newprocessTime_  += other -> newprocessTime_; 
  timeouts_        += other -> timeouts_; 
  numSorts_        += other -> numSorts_; 
  sortElapsedTime_ += other -> sortElapsedTime_; 
  spaceUsage_   += other -> spaceUsage_; 
  spaceAlloc_   += other -> spaceAlloc_; 
  heapUsage_    += other -> heapUsage_; 
  heapAlloc_    += other -> heapAlloc_; 
  heapWM_       += other -> heapWM_; 
  if (scratchOverflowMode_ == -1)
    scratchOverflowMode_ = other->scratchOverflowMode_;
  cpuTime_          += other -> cpuTime_;
  espSpaceUsage_ += other -> espSpaceUsage_;
  espSpaceAlloc_ += other -> espSpaceAlloc_;
  espHeapUsage_  += other -> espHeapUsage_;
  espHeapAlloc_  += other -> espHeapAlloc_;
  espHeapWM_     += other -> espHeapWM_;
  espCpuTime_       += other -> espCpuTime_;
  reqMsgCnt_        += other -> reqMsgCnt_;
  reqMsgBytes_      += other -> reqMsgBytes_;
  replyMsgCnt_      += other -> replyMsgCnt_;
  replyMsgBytes_    += other -> replyMsgBytes_;
  scratchFileCount_ += other->scratchFileCount_;

  if (spaceBufferSize_ == 0 &&
     other->spaceBufferSize_ > 0)
     spaceBufferSize_ = other->spaceBufferSize_;
  Float32 mFactor = 1;
  if(spaceBufferSize_ > 0)
    mFactor = (Float32)other->spaceBufferSize_ / spaceBufferSize_;
  spaceBufferCount_ += Int32 (other->spaceBufferCount_ * mFactor);
  mFactor = 1;
  if(scratchIOSize_ > 0)
    mFactor = (Float32)other->scratchIOSize_ / scratchIOSize_;
  scratchReadCount_ += (other->scratchReadCount_ * mFactor);
  scratchWriteCount_ += (other->scratchWriteCount_ * mFactor);
  if (other->scratchIOMaxTime_ > scratchIOMaxTime_)
     scratchIOMaxTime_ = other->scratchIOMaxTime_;   
  interimRowCount_ += other->interimRowCount_;
  udrCpuTime_ += other->udrCpuTime_;
  if (topN_ == -1 && other->topN_ > 0)
      topN_ = other->topN_;
}


void ExMeasStats::merge(ExOperStats * other)
{
  switch (other->statType())
  {
    case ROOT_OPER_STATS:
      merge((ExFragRootOperStats*) other);
      break;
    case MEAS_STATS:
      merge((ExMeasStats*) other);
      break;
    case UDR_BASE_STATS:
      merge((ExUDRBaseStats *)other);
      break;
    case BMO_STATS:
      merge((ExBMOStats *)other);
      break;
    case SE_STATS:
      merge((ExStorageEngineStats *)other);
      break;
      break;
    default:
      // do nothing - This type of stat has no merge data
      break;
  }
}

ExOperStats * ExMeasStats::copyOper(NAMemory * heap)
{
  ExMeasStats * stat =  new(heap) ExMeasStats(heap);
  stat->copyContents(this);
  return stat;
}

void ExMeasStats::copyContents(ExMeasStats *other)
{
  if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS)
  {
    cpuTime_ = other->cpuTime_;
    spaceUsage_ = other->spaceUsage_;
    spaceAlloc_ = other->spaceAlloc_;
    heapUsage_ = other->heapUsage_;
    heapAlloc_ = other->heapAlloc_;
    heapWM_ = other->heapWM_;
    queryIdLen_ = other->queryIdLen_;
    if (queryIdLen_ != 0)
    {
      queryId_ = new ((NAHeap *)(getHeap())) char[queryIdLen_+1];
      str_cpy_all(queryId_, other->queryId_, queryIdLen_);
      queryId_[queryIdLen_] = '\0';
    }
    else
      queryId_ = NULL;
  }
  else
  if ((Int32)getCollectStatsType() == SQLCLI_QID_DETAIL_STATS)
  {
    cpuTime_ = other->cpuTime_;
    spaceUsage_ = other->spaceUsage_;
    spaceAlloc_ = other->spaceAlloc_;
    heapUsage_ = other->heapUsage_;
    heapAlloc_ = other->heapAlloc_;
    heapWM_ = other->heapWM_;
    reqMsgCnt_ = other->reqMsgCnt_;
    reqMsgBytes_ = other->reqMsgBytes_;
    replyMsgCnt_  = other->replyMsgCnt_;
    replyMsgBytes_  = other->replyMsgBytes_;
  }
  else
  {
  ExMeasBaseStats::copyContents(other);
  newprocess_ = other->newprocess_; 
  newprocessTime_ = other->newprocessTime_; 
  timeouts_ = other->timeouts_; 
  numSorts_ = other->numSorts_;
  sortElapsedTime_ = other->sortElapsedTime_;
  spaceUsage_ = other->spaceUsage_;
  spaceAlloc_ = other->spaceAlloc_;
  heapUsage_ = other->heapUsage_;
  heapAlloc_ = other->heapAlloc_;
  heapWM_ = other->heapWM_;
  if (scratchOverflowMode_ == -1)
    scratchOverflowMode_ = other->scratchOverflowMode_;
  cpuTime_ = other->cpuTime_;
  espSpaceUsage_ = other->espSpaceUsage_; 
  espSpaceAlloc_ = other->espSpaceAlloc_; 
  espHeapUsage_ = other->espHeapUsage_; 
  espHeapAlloc_ = other->espHeapAlloc_; 
  espHeapWM_ = other->espHeapWM_; 
  espCpuTime_ = other->espCpuTime_;
  histCpuTime_ = other->histCpuTime_;
  queryIdLen_ = other->queryIdLen_;
  queryId_ = other->queryId_;
  reqMsgCnt_ = other->reqMsgCnt_;
  reqMsgBytes_ = other->reqMsgBytes_;
  replyMsgCnt_  = other->replyMsgCnt_;
  replyMsgBytes_  = other->replyMsgBytes_;
  scratchFileCount_ = other->scratchFileCount_;
  spaceBufferSize_ = other->spaceBufferSize_;
  spaceBufferCount_ = other->spaceBufferCount_;
  scratchIOSize_ = other->scratchIOSize_;
  scratchReadCount_ = other->scratchReadCount_;
  scratchWriteCount_ = other->scratchWriteCount_;
  scratchIOMaxTime_ = other->scratchIOMaxTime_;
  interimRowCount_ = interimRowCount_;
  udrCpuTime_ = other->udrCpuTime_;
  topN_ = other->topN_;
  }
}

ExMeasStats * ExMeasStats::castToExMeasStats()
{
  return this;
}

const char * ExMeasStats::getNumValTxt(Int32 i) const
{
  return ExMeasBaseStats::getNumValTxt(i);
}

Int64 ExMeasStats::getNumVal(Int32 i) const
{
  return ExMeasBaseStats::getNumVal(i);
}

void ExMeasStats::getVariableStatsInfo(char * dataBuffer, char * datalen, 
				       Lng32 maxLen)
{
  char * buf = dataBuffer;
  if ((Int32)getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS)
  {
    sprintf(buf, "statsRowType: %d ProcessId: %s Qid: %s CpuTime: %ld SpaceUsed: %u SpaceTotal: %u HeapUsed: %u HeapTotal: %u ",
      statType(),
      "NULL",
      ((queryId_ != NULL) ? queryId_ : "NULL"),
      cpuTime_,
      (UInt32)spaceUsage_,
      (UInt32)spaceAlloc_,
      (UInt32)heapUsage_,
      (UInt32)heapAlloc_);
  }
  else
  if ((Int32)getCollectStatsType() == SQLCLI_QID_DETAIL_STATS)
  {
    sprintf(buf, "statsRowType: %d ProcessId: %s CpuTime: %ld SpaceUsed: %u SpaceTotal: %u HeapUsed: %u HeapTotal: %u HeapWM: %u reqMsgCnt: %ld reqMsgBytes: %ld replyMsgCnt: %ld replyMsgBytes: %ld ",
      statType(),
      "NULL",
      cpuTime_,
      (UInt32)spaceUsage_,
      (UInt32)spaceAlloc_,
      (UInt32)heapUsage_,
      (UInt32)heapAlloc_,
      (UInt32)heapWM_,
      reqMsgCnt_,
      reqMsgBytes_,
      replyMsgCnt_,
      replyMsgBytes_);
  }
  else

  {
  ExMeasBaseStats::getVariableStatsInfo(dataBuffer, datalen, maxLen);
  buf += *((short *) datalen);

  sprintf(buf, 
    "statsRowType: %d Newprocess: %u NewprocessTime: %ld Timeouts: %u NumSorts: %u SortElapsedTime: %ld "
    "SpaceTotal: %d  SpaceUsed: %d HeapTotal: %d HeapUsed: %d HeapWM: %u CpuTime: %ld "
    "reqMsgCnt: %ld reqMsgBytes: %ld replyMsgCnt: %ld "
    "replyMsgBytes: %ld scrOverflowMode: %d sortTopN: %d "
    "scrFileCount: %d bmoSpaceBufferSize: %d bmoSpaceBufferCount: %ld scrIoSize: %d "
    "scrWriteCount: %ld scrReadCount: %ld scrIOMaxTime: %ld interimRowCount: %ld udrCpuTime: %ld ",
	      statType(),
              getNewprocess(),
	      getNewprocessTime(),
	      getTimeouts(),
	      getNumSorts(),
	      getSortElapsedTime(),
              spaceAlloc_ + espSpaceAlloc_,
              spaceUsage_ + espSpaceUsage_,
              heapAlloc_ + espHeapAlloc_,
              heapUsage_ + espHeapUsage_,
              heapWM_ + espHeapWM_,
              cpuTime_ + espCpuTime_,
              reqMsgCnt_,
              reqMsgBytes_,
              replyMsgCnt_,
              replyMsgBytes_,
              scratchOverflowMode_,
              topN_,
              scratchFileCount_,
              spaceBufferSize_,
              spaceBufferCount_,
              scratchIOSize_,
              scratchWriteCount_,
              scratchReadCount_,
              scratchIOMaxTime_,
              interimRowCount_,
              udrCpuTime_
              );
  }
  buf += str_len(buf);

  // dataLen is really the varchar indicator
  *(short *)datalen = (short) (buf - dataBuffer);
}

void ExMeasStats::updateSpaceUsage(Space *space,
					   CollHeap *heap)
{
  if (space)
  {
      spaceUsage_ = (Int32)(((Space *) space)->getAllocSize() >> 10);
      spaceAlloc_ = (Int32)(((Space *) space)->getTotalSize() >> 10);
      
  }
  if (heap)
  {
      heapUsage_ = (Int32)(((NAMemory *) heap)->getAllocSize() >> 10);
      heapAlloc_ = (Int32)(((NAMemory *) heap)->getTotalSize() >> 10);
      heapWM_ = (Int32)(((NAMemory *) heap)->getHighWaterMark() >> 10);
  }
}

Lng32 ExMeasStats::getStatsItem(SQLSTATS_ITEM* sqlStats_item)
{
  sqlStats_item->error_code = 0;
  switch (sqlStats_item->statsItem_id)
  {
  case SQLSTATS_ACT_ROWS_ACCESSED:
    sqlStats_item->int64_value = exeSEStats()->getAccessedRows();
    break;
  case SQLSTATS_ACT_ROWS_USED:
    sqlStats_item->int64_value = exeSEStats()->getUsedRows();
    break;
  case SQLSTATS_SE_IOS:
    sqlStats_item->int64_value = exeSEStats()->getNumIOCalls();
    break;
  case SQLSTATS_SE_IO_BYTES:
    sqlStats_item->int64_value = exeSEStats()->getNumIOBytes();
    break;
  case SQLSTATS_SE_IO_MAX_TIME:
    sqlStats_item->int64_value = exeSEStats()->getMaxIOTime();
    break;
  case SQLSTATS_SQL_CPU_BUSY_TIME:
    sqlStats_item->int64_value = cpuTime_ + espCpuTime_;
    break;
  case SQLSTATS_SQL_SPACE_ALLOC:
    sqlStats_item->int64_value = spaceAlloc_ + espSpaceAlloc_;
    break;
  case SQLSTATS_SQL_SPACE_USED:
    sqlStats_item->int64_value = spaceUsage_ + espSpaceUsage_;
    break;
  case SQLSTATS_SQL_HEAP_ALLOC:
    sqlStats_item->int64_value = heapAlloc_ + espHeapAlloc_;
    break;
  case SQLSTATS_SQL_HEAP_USED:
    sqlStats_item->int64_value = heapUsage_ + espHeapUsage_;
    break;
  case SQLSTATS_SQL_HEAP_WM:
    sqlStats_item->int64_value = heapWM_ + espHeapWM_;
    break;
  case SQLSTATS_PROCESS_CREATED:
    sqlStats_item->int64_value = newprocess_;
    break;
  case SQLSTATS_PROCESS_CREATE_TIME:
    sqlStats_item->int64_value = newprocessTime_;
    break;
  case SQLSTATS_REQ_MSG_CNT:
    sqlStats_item->int64_value = reqMsgCnt_;
    break;
  case SQLSTATS_REQ_MSG_BYTES:
    sqlStats_item->int64_value = reqMsgBytes_;
    break;
  case SQLSTATS_REPLY_MSG_CNT:
    sqlStats_item->int64_value = replyMsgCnt_;
    break;
  case SQLSTATS_REPLY_MSG_BYTES:
    sqlStats_item->int64_value = replyMsgBytes_;
    break;
  case SQLSTATS_SCRATCH_FILE_COUNT:
    sqlStats_item->int64_value = scratchFileCount_;
    break;
  case SQLSTATS_SCRATCH_OVERFLOW_MODE:
    sqlStats_item->int64_value = scratchOverflowMode_;
    break;
  case SQLSTATS_BMO_SPACE_BUFFER_SIZE:
    sqlStats_item->int64_value = spaceBufferSize_;
    break;
  case SQLSTATS_BMO_SPACE_BUFFER_COUNT:
    sqlStats_item->int64_value = spaceBufferCount_;
    break;
  case SQLSTATS_SCRATCH_READ_COUNT:
    sqlStats_item->int64_value = scratchReadCount_;
    break;
  case SQLSTATS_SCRATCH_WRITE_COUNT:
    sqlStats_item->int64_value = scratchWriteCount_;
    break;
  case SQLSTATS_SCRATCH_IO_SIZE:
    sqlStats_item->int64_value = scratchIOSize_;
    break;
  case SQLSTATS_SCRATCH_IO_MAX_TIME:
    sqlStats_item->int64_value = scratchIOMaxTime_;
    break;
  case SQLSTATS_INTERIM_ROW_COUNT:
    sqlStats_item->int64_value = interimRowCount_;
    break;
  case SQLSTATS_TOPN:
    sqlStats_item->int64_value = topN_;
    break;
  case SQLSTATS_UDR_CPU_BUSY_TIME:
    sqlStats_item->int64_value = udrCpuTime_;
    break;
  default:
    sqlStats_item->error_code = -EXE_STAT_NOT_FOUND;
    break;
  }
  return 0;
}

NABoolean ExMeasStats::filterForCpuStats()
{
  NABoolean retcode;
  if (histCpuTime_ == 0)
    retcode = FALSE;
  else
  if (cpuTime_ > histCpuTime_)
    retcode = TRUE;
  else
    retcode = FALSE;
  setCpuStatsHistory();
  return retcode;
}

////////////////////////////////////////////////////////////////
// class ExUDRStats
////////////////////////////////////////////////////////////////
ExUDRStats::ExUDRStats(NAMemory * heap,
                       ULng32 sendBufferSize,
                       ULng32 recBufferSize,
                       const ComTdb * tdb,
                       ex_tcb * tcb)
  : ExUDRBaseStats(heap,
                UDR_STATS,
		tcb,
                tdb),
     UDRName_(NULL)
{ 
  if (! tdb)
    return;

  ComTdbUdr * udrTdb = (ComTdbUdr *) tdb;
  const char * name = udrTdb->getSqlName();

  // name is valid for CALL stmts but not for RS stmts.
  if (name != NULL)
  {
    Lng32 len = str_len(name);
    UDRName_ = (char *)heap_->allocateMemory(len+1);
    str_cpy_all(UDRName_, name, len);
    UDRName_[len] = 0;
  }

  bufferStats()->sendBufferSize() = sendBufferSize;
  bufferStats()->recdBufferSize() = recBufferSize;

}

ExUDRStats::ExUDRStats(NAMemory * heap)
  : ExUDRBaseStats(heap, UDR_STATS),
    UDRName_(NULL) {};

ExUDRStats::~ExUDRStats()
{
  if (UDRName_)
    heap_->deallocateMemory((void*)UDRName_);
}

void ExUDRStats::init(NABoolean resetDop)
{
  ExUDRBaseStats::init(resetDop);
  bufferStats()->init(resetDop);
  sentControlBuffers_.init();
  sentContinueBuffers_.init();
}

UInt32 ExUDRStats::packedLength()
{
  UInt32 size = ExUDRBaseStats::packedLength();

  alignSizeForNextObj(size);
  size += bufferStats()->packedLength();

  size += sizeof(sentControlBuffers_);
  size += sizeof(sentContinueBuffers_);
  size += sizeof(UDRServerInit_);
  size += sizeof(UDRServerStart_);

  advanceSize2(size, UDRName_);

  return size;
}

UInt32 ExUDRStats::pack(char * buffer)
{
  UInt32 size = ExUDRBaseStats::pack(buffer);
  buffer += size;

  UInt32 tempSize = bufferStats()->alignedPack(buffer);
  buffer += tempSize;
  size += tempSize;

  size += packIntoBuffer(buffer, sentControlBuffers_);
  size += packIntoBuffer(buffer, sentContinueBuffers_);
  size += packIntoBuffer(buffer, UDRServerInit_);
  size += packIntoBuffer(buffer, UDRServerStart_);
  size += packCharStarIntoBuffer(buffer, UDRName_);

  return size;
}

void ExUDRStats::unpack(const char * &buffer)
{
  ExUDRBaseStats::unpack(buffer);
  bufferStats()->alignedUnpack(buffer);
  unpackBuffer(buffer, sentControlBuffers_);
  unpackBuffer(buffer, sentContinueBuffers_);
  unpackBuffer(buffer, UDRServerInit_);
  unpackBuffer(buffer, UDRServerStart_);
  unpackBuffer(buffer, UDRName_, heap_);
}

void ExUDRStats::copyContents(ExUDRStats * other)
{
  ExUDRBaseStats::copyContents(other);
  bufferStats()->copyContents(other->bufferStats());
  sentControlBuffers_ = other->sentControlBuffers_;
  sentContinueBuffers_ = other->sentContinueBuffers_;
}

void ExUDRStats::merge(ExUDRStats* other)
{
  ExUDRBaseStats::merge(other);
  bufferStats()->merge(other -> bufferStats());
  sentControlBuffers_.merge(&(other -> sentControlBuffers_));
  sentContinueBuffers_.merge(&(other -> sentContinueBuffers_));

  // Assume a merge is is for same UDR server instance
}

ExOperStats * ExUDRStats::copyOper(NAMemory * heap)
{
  ExUDRStats * stat = new(heap) ExUDRStats(heap);
  stat->copyContents(this);
  return stat;
}

ExUDRStats * ExUDRStats::castToExUDRStats()
{
  return this;
}



void ExUDRStats::getVariableStatsInfo(char * dataBuffer, 
                                      char * datalen, 
                                      Lng32 maxLen)
{
  char *buf = dataBuffer;

  ExOperStats::getVariableStatsInfo(dataBuffer, datalen, maxLen);
  buf += *((short *) datalen);

  ULng32 startUp = (ULng32) (UDRServerStart_ - UDRServerInit_);
  ULng32 startUpSec = startUp / 1000000;
  ULng32 startUpMilli = (startUp % 1000000) / 10000;

  sprintf(buf, "SendBufferSize: %u RecvBufferSize: %u UDRServerId: %s ", 
              (ULng32) bufferStats()->sendBufferSize(),
              (ULng32) bufferStats()->recdBufferSize(),
              getUDRServerId() ? getUDRServerId() : "No Server Id info");
  buf += str_len(buf);

  sprintf(buf, "UDRServerInit: %u.%03u CtrlMsgNum: %u CtrlMsgMean: %f DataMsgNum: %u DataMsgMean: %f ContMsgNum: %u ContMsgMean: %f ",
              startUpSec, startUpMilli,
              (ULng32) sentControlBuffers_.entryCnt(),
              sentControlBuffers_.mean(),
              (ULng32) bufferStats()->sentBuffers().entryCnt(),
              bufferStats()->sentBuffers().mean(),         
              (ULng32) sentContinueBuffers_.entryCnt(),
              sentContinueBuffers_.mean());

  buf += str_len(buf);
  *(short *)datalen = (short) (buf - dataBuffer);
}

const char * ExUDRStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
      case 1:
        return "OperCpuTime";
      case 2:
        return "CtrlMsgNum";
      case 3:
        return "DataMsgNum";
      case 4:
        return "ContMsgNum";
      default:
        return "";
    }
}

Int64 ExUDRStats::getNumVal(Int32 i) const
{
  switch (i)
    {
      case 1:
        return ExOperStats::getNumVal(i);
      case 2:
        return sentControlBuffers_.entryCnt();
      case 3:
        return ((ExBufferStats&)bufferStats_).sentBuffers().entryCnt();
      case 4:
        return sentContinueBuffers_.entryCnt();
      default:
        return 0;
    }
}

const char * ExUDRStats::getTextVal()
{
  return UDRName_;
}

//////////////////////////////////////////////////////////////////
// class ExStatisticsArea
//////////////////////////////////////////////////////////////////
ExStatisticsArea::ExStatisticsArea(NAMemory * heap, Lng32 sendBottomNum,
                                   ComTdb::CollectStatsType cst,
                                   ComTdb::CollectStatsType origCst)
     : IpcMessageObj(IPC_SQL_STATS_AREA, StatsCurrVersion), //IpcCurrSqlStatisticsVersion),
    heap_(heap),
    sendBottomNum_(sendBottomNum),
    sendBottomCnt_(0),
    collectStatsType_(cst) ,
    flags_(0),
    masterStats_(NULL),
    rootStats_(NULL),
    detailLevel_(0),
    subReqType_(-1)
{
  if (collectStatsType_ == ComTdb::ACCUMULATED_STATS)
    entries_ = new(heap_) HashQueue(heap_, 2);
  else if (collectStatsType_ == ComTdb::PERTABLE_STATS || 
    collectStatsType_ == ComTdb::PROGRESS_STATS || 
    collectStatsType_ == ComTdb::OPERATOR_STATS)
    entries_ = new(heap_) HashQueue(heap_, 32);
  else if (collectStatsType_ == ComTdb::ALL_STATS)
    entries_ = new(heap_) HashQueue(heap_, 128);
  else
    entries_ = new(heap_) HashQueue(heap_, 32); // Keep it same as the default collect stats type
  if (origCst == ComTdb::NO_STATS)
    origCollectStatsType_ = collectStatsType_;
  else
    origCollectStatsType_ = origCst;
}

void ExStatisticsArea::removeEntries()
{
  if (entries_) {
    entries_->position();
      
    ExOperStats * stat;
    while ((stat = getNext()) != NULL) 
    {    
      // we assume that all the ExOperStats in an ExStatisticsArea
      // are allocated for the same heap as the ExStatisticsArea
      ExOperStats::StatType statType;
      statType = stat->statType();
      switch (statType) {
      case ExOperStats::EX_OPER_STATS:
        NADELETE(stat, ExOperStats, heap_);
        break;
      case ExOperStats::MEAS_STATS:
        NADELETE((ExMeasStats *)stat, ExMeasStats, heap_);
        break;
      case ExOperStats::ROOT_OPER_STATS:
        NADELETE((ExFragRootOperStats *)stat, ExFragRootOperStats, heap_);
        break;
      case ExOperStats::PARTITION_ACCESS_STATS:
        NADELETE((ExPartitionAccessStats *)stat, ExPartitionAccessStats, heap_);
        break;
      case ExOperStats::GROUP_BY_STATS:
        ((ExHashGroupByStats *)stat)->deleteClusterStats();
        NADELETE((ExHashGroupByStats *)stat, ExHashGroupByStats, heap_);
        break;
      case ExOperStats::HASH_JOIN_STATS:
        ((ExHashJoinStats *)stat)->deleteClusterStats();
        NADELETE((ExHashJoinStats *)stat, ExHashJoinStats, heap_);
        break;
      case ExOperStats::PROBE_CACHE_STATS:
        NADELETE((ExProbeCacheStats *)stat, ExProbeCacheStats, heap_);
        break;
      case ExOperStats::FAST_EXTRACT_STATS:
        NADELETE((ExFastExtractStats *)stat, ExFastExtractStats, heap_);
        break;
      case ExOperStats::ESP_STATS:
        NADELETE((ExESPStats *)stat, ExESPStats, heap_);
        break;
      case ExOperStats::SORT_STATS:
        NADELETE((ExSortStats *)stat, ExSortStats, heap_);
        break;
      case ExOperStats::UDR_STATS:
        NADELETE((ExUDRStats *)stat, ExUDRStats, heap_);
        break;
      case ExOperStats::RMS_STATS:
        NADELETE((ExRMSStats *)stat, ExRMSStats, heap_);
        break;
      case ExOperStats::BMO_STATS:
        NADELETE((ExBMOStats *)stat, ExBMOStats, heap_);
        break;
      case ExOperStats::UDR_BASE_STATS:
        NADELETE((ExUDRBaseStats *)stat, ExUDRBaseStats, heap_);
        break;
      case ExOperStats::SE_STATS:
        NADELETE((ExStorageEngineStats *)stat, ExStorageEngineStats, heap_);
        break;
      default:
        NADELETE(stat, ExOperStats, heap_);
      }
    }
    //    delete entries_;
    NADELETE(entries_, HashQueue, heap_);
    entries_ = NULL;
    rootStats_ = NULL;
  }
}

ExStatisticsArea::~ExStatisticsArea()
{
  removeEntries();

  if (masterStats_ != NULL)
  {
    NADELETE(masterStats_, ExMasterStats, masterStats_->getHeap());
  }
}

void ExStatisticsArea::initEntries()
{
  sendBottomCnt_ = 0;

  entries_->position();

  ExOperStats * stat;
  while ((stat = getNext()) != NULL) {
    stat->init(TRUE);
  }
  
  if (masterStats_ != NULL)
     masterStats_->setRowsReturned(0);
}

void ExStatisticsArea::restoreDop()
{
  entries_->position();
  ExOperStats * stat;
  while ((stat = getNext()) != NULL) 
     stat->restoreDop();
}


Lng32 ExStatisticsArea::numEntries()
{
  return entries_->numEntries();
}

NABoolean ExStatisticsArea::merge(ExOperStats * other, UInt16 statsMergeType)
{
  // search for the 'other' entry by matching the statID
  //  position();
  Int64 hashData = other->getHashData(statsMergeType);
  position((char*)&hashData, sizeof(Int64));

  ExOperStats * stat;
  switch (statsMergeType)
  {
    case SQLCLI_ACCUMULATED_STATS:
      if ((stat = getNext()) == NULL)
        return FALSE;
      ((ExMeasStats*)stat)->merge(other);
      return TRUE;
      break;
    case SQLCLI_PERTABLE_STATS:
    case SQLCLI_PROGRESS_STATS:
      while ((stat = getNext()) != NULL)
      {
        switch (other->statType())
        {
        case ExOperStats::ROOT_OPER_STATS:
          if (stat->statType() == ExOperStats::ROOT_OPER_STATS)
          {
            ((ExFragRootOperStats *)stat)->merge((ExFragRootOperStats *)other);
            return TRUE;
          }
          break;
        case ExOperStats::BMO_STATS:
          if (statsMergeType == SQLCLI_PROGRESS_STATS &&
            stat->statType() == ExOperStats::BMO_STATS &&
            stat->getTdbId() == other->getTdbId())
          {
            ((ExBMOStats *)stat)->merge((ExBMOStats *)other);
            return TRUE;
          }
          else if (stat->statType() == ExOperStats::ROOT_OPER_STATS)
          {
            ((ExFragRootOperStats *)stat)->merge((ExBMOStats *)other);
            return TRUE;
          }
          break;
        case ExOperStats::UDR_BASE_STATS:
          if (statsMergeType == SQLCLI_PROGRESS_STATS &&
            stat->statType() == ExOperStats::UDR_BASE_STATS &&
            stat->getTdbId() == other->getTdbId())
          {
            ((ExUDRBaseStats *)stat)->merge((ExUDRBaseStats *)other);
            return TRUE;
          }
          else if (stat->statType() == ExOperStats::ROOT_OPER_STATS)
          {
            ((ExFragRootOperStats *)stat)->merge((ExUDRBaseStats *)other);
            return TRUE;
          }
          break;
        case ExOperStats::SE_STATS:
          if (stat->statType() == ExOperStats::SE_STATS
            && stat->getPertableStatsId() == other->getPertableStatsId())
          {
            ((ExStorageEngineStats *)stat)->merge((ExStorageEngineStats *)other);
            return TRUE;
          }
          break;
         case ExOperStats::EX_OPER_STATS:
          return TRUE;
          break;
        default:
          ex_assert(FALSE, "Merging unknown operator statistics type");
          break;
        }  // switch
      }// while
      break;
    case SQLCLI_OPERATOR_STATS:
      if (other->getVersion() < _STATS_RTS_VERSION_R25 &&
        other->statType() != ExOperStats::EX_OPER_STATS &&
        other->statType() != ExOperStats::ROOT_OPER_STATS &&
        other->statType() != ExOperStats::BMO_STATS &&
        other->statType() != ExOperStats::UDR_BASE_STATS &&
        other->statType() != ExOperStats::HBASE_ACCESS_STATS &&
        other->statType() != ExOperStats::HDFSSCAN_STATS &&
        other->statType() != ExOperStats::REORG_STATS)

        // Ignore stats and return as if merge is done
        return TRUE;
      while ((stat = getNext()) != NULL)
      {
        if (stat->getTdbId() == other->getTdbId()) 
        {
          // found it. Now merge it.
          switch (other->statType())
          {
          case ExOperStats::EX_OPER_STATS:
            ((ExOperStats *)stat)->merge((ExOperStats *)other);
            break;
          case ExOperStats::ROOT_OPER_STATS:
            ((ExFragRootOperStats *)stat)->merge((ExFragRootOperStats *)other);
            break;
          case ExOperStats::BMO_STATS:
            ((ExBMOStats *)stat)->merge((ExBMOStats *)other);
            break;
          case ExOperStats::UDR_BASE_STATS:
            ((ExUDRBaseStats *)stat)->merge((ExUDRBaseStats *)other);
            break;
         case ExOperStats::SE_STATS:
            ((ExStorageEngineStats*)stat)->merge((ExStorageEngineStats*)other);
            break;
          default:
              ex_assert(FALSE, "Merging unknown operator statistics type");
          }  // switch 
          return true;
        }
      }
      break;
    default:
      while ((stat = getNext()) != NULL)
      {
        if (*stat == other) {
          // found it. Now merge it.
          stat->merge(other);
          return TRUE;
        }
      }
      break;
  }
  // didn't find
  return FALSE;
}

// merges all entries of otherStatArea. If entries in otherStatArea
// are not present in 'this', insert the new entries.
NABoolean ExStatisticsArea::merge(ExStatisticsArea * otherStatsArea, UInt16 statsMergeType)
{
  ExOperStats::StatType statType;
  ComTdb::CollectStatsType tempStatsMergeType;

  if (otherStatsArea == NULL)
    return TRUE;

  if (otherStatsArea->masterStats_ != NULL)
    {
      if (masterStats_ != NULL)
	{
	  NADELETE(masterStats_, ExMasterStats, masterStats_->getHeap());
	}
      masterStats_ = new (heap_) ExMasterStats((NAHeap *)heap_);
      masterStats_->copyContents(otherStatsArea->masterStats_);
    }

  if (otherStatsArea->numEntries() == 0)
    return TRUE; // nothing to merge

  if (statsMergeType == SQLCLI_SAME_STATS)
    tempStatsMergeType = getCollectStatsType();
  else
    tempStatsMergeType = (ComTdb::CollectStatsType) statsMergeType;
  
  otherStatsArea->position();
  ExOperStats * stat;
  ExOperStats *newStat;
  while ((stat = otherStatsArea->getNext()) != NULL)
    {
      if (merge(stat, tempStatsMergeType) == FALSE)
	{
	  statType = stat->statType();
	  switch (tempStatsMergeType)
	    {
	    case SQLCLI_ACCUMULATED_STATS:
	      newStat = new(heap_) ExMeasStats(heap_);
	      newStat->setCollectStatsType(tempStatsMergeType);
	      insert(newStat);
	      if (merge(stat, tempStatsMergeType) == FALSE)
		ex_assert(FALSE, "Failed to merge accumulated stats");
	      break;

	    case SQLCLI_PERTABLE_STATS:
	    case SQLCLI_PROGRESS_STATS:
	      switch (statType)
		{
		case ExOperStats::ROOT_OPER_STATS:
		  newStat = new(heap_)ExFragRootOperStats(heap_);
		  newStat->setCollectStatsType(tempStatsMergeType);
		  newStat->initTdbForRootOper();
		  ((ExFragRootOperStats *)newStat)->merge((ExFragRootOperStats *)stat);
		  insert(newStat);
		  break;

		case ExOperStats::BMO_STATS:
		  if ((Int32)tempStatsMergeType == SQLCLI_PROGRESS_STATS)
		    {
		      newStat = new(heap_)ExBMOStats(heap_);
		      ((ExBMOStats *)newStat)->copyContents((ExBMOStats *)stat);
		      newStat->setCollectStatsType(tempStatsMergeType);
		      insert(newStat);
		    }
		  else
		    {
		      newStat = new(heap_)ExFragRootOperStats(heap_);
		      newStat->setCollectStatsType(tempStatsMergeType);
		      newStat->initTdbForRootOper();
		      ((ExFragRootOperStats *)newStat)->merge((ExBMOStats *)stat);
		      insert(newStat);
		    }
		  break;
		case ExOperStats::UDR_BASE_STATS:
		  if ((Int32)tempStatsMergeType == SQLCLI_PROGRESS_STATS)
		    {
		      newStat = new(heap_)ExUDRBaseStats(heap_);
		      ((ExUDRBaseStats *)newStat)->copyContents((ExUDRBaseStats *)stat);
		      newStat->setCollectStatsType(tempStatsMergeType);
		      insert(newStat);
		    }
		  else
		    {
		      newStat = new(heap_)ExFragRootOperStats(heap_);
		      newStat->setCollectStatsType(tempStatsMergeType);
		      newStat->initTdbForRootOper();
		      ((ExFragRootOperStats *)newStat)->merge((ExUDRBaseStats *)stat);
		      insert(newStat);
		    }
		  break;
		case ExOperStats::SE_STATS:
		  {
		    newStat = new(heap_) ExStorageEngineStats(heap_);
		    newStat->setCollectStatsType(tempStatsMergeType);
		    ((ExStorageEngineStats *)newStat)->copyContents((ExStorageEngineStats *)stat);
		    newStat->setCollectStatsType(tempStatsMergeType);
		    insert(newStat);
		  }
		  break;
		  
		default:
		  break;
		}
	      break;

	    default:
	      {
		switch (statType)
		  {
		  case ExOperStats::MEAS_STATS:
		    newStat = new(heap_) ExMeasStats(heap_);
		    ((ExMeasStats *)newStat)->copyContents((ExMeasStats *)stat);
		    newStat->setCollectStatsType(tempStatsMergeType);
		    insert(newStat);
		    break;
		  case ExOperStats::ROOT_OPER_STATS:
		    newStat = new(heap_)ExFragRootOperStats(heap_);
		    ((ExFragRootOperStats *)newStat)->copyContents((ExFragRootOperStats *)stat);
		    newStat->setCollectStatsType(tempStatsMergeType);
		    insert(newStat);
		    break;

		  case ExOperStats::BMO_STATS:
		    newStat = new(heap_) ExBMOStats(heap_);
		    ((ExBMOStats *)newStat)->copyContents((ExBMOStats *)stat);
		    newStat->setCollectStatsType(tempStatsMergeType);
		    insert(newStat);
		    break;
		  case ExOperStats::EX_OPER_STATS:
		    newStat = new(heap_) ExOperStats(heap_);
		    ((ExOperStats *)newStat)->copyContents((ExOperStats *)stat);
		    newStat->setCollectStatsType(tempStatsMergeType);
		    insert(newStat);
		    break;

		  case ExOperStats::UDR_BASE_STATS:
		    newStat = new(heap_)ExUDRBaseStats(heap_);
		    ((ExUDRBaseStats *)newStat)->copyContents((ExUDRBaseStats *)stat);
		    newStat->setCollectStatsType(tempStatsMergeType);
		    insert(newStat);
		    break;

		  case ExOperStats::SE_STATS:
		    newStat = new(heap_) ExStorageEngineStats(heap_);
		    ((ExStorageEngineStats *)newStat)->copyContents((ExStorageEngineStats *)stat);
		    newStat->setCollectStatsType(tempStatsMergeType);
		    insert(newStat);
		    break;
		  default:
		    if ((Int32)tempStatsMergeType == SQLCLI_OPERATOR_STATS || (Int32)tempStatsMergeType == SQLCLI_PERTABLE_STATS)
		      //ignore merging the rest of the stats
		      break;
		    switch (statType)
		      {
		      case ExOperStats::PARTITION_ACCESS_STATS:
			newStat = new(heap_) ExPartitionAccessStats(heap_);
			((ExPartitionAccessStats *)newStat)->copyContents((ExPartitionAccessStats *)stat);
			newStat->setCollectStatsType(tempStatsMergeType);
			insert(newStat);
			break;

		      case ExOperStats::GROUP_BY_STATS:
			newStat = new(heap_) ExHashGroupByStats(heap_);
			((ExHashGroupByStats *)newStat)->copyContents((ExHashGroupByStats *)stat);
			newStat->setCollectStatsType(tempStatsMergeType);
			insert(newStat);
			break;

		      case ExOperStats::HASH_JOIN_STATS:
			newStat = new(heap_) ExHashJoinStats(heap_);
			((ExHashJoinStats *)newStat)->copyContents((ExHashJoinStats *)stat);
			newStat->setCollectStatsType(tempStatsMergeType);
			insert(newStat);
			break;

		      case ExOperStats::PROBE_CACHE_STATS:
			newStat = new(heap_) ExProbeCacheStats(heap_);
			((ExProbeCacheStats *)newStat)->copyContents((ExProbeCacheStats *)stat);
			newStat->setCollectStatsType(tempStatsMergeType);
			insert(newStat);
			break;

		      case ExOperStats::FAST_EXTRACT_STATS:
			newStat = new(heap_) ExFastExtractStats(heap_);
			((ExFastExtractStats *)newStat)->copyContents((ExFastExtractStats *)stat);
			newStat->setCollectStatsType(tempStatsMergeType);
			insert(newStat);
			break;
			
		      default:
			if ((Int32)tempStatsMergeType == SQLCLI_OPERATOR_STATS || (Int32)tempStatsMergeType == SQLCLI_PERTABLE_STATS)
			  //ignore merging the rest of the stats
			  break;

			switch (statType) 
			  {
			  case ExOperStats::PARTITION_ACCESS_STATS:
			    newStat = new(heap_) ExPartitionAccessStats(heap_);
			    ((ExPartitionAccessStats *)newStat)->copyContents((ExPartitionAccessStats *)stat);
			    newStat->setCollectStatsType(tempStatsMergeType);
			    insert(newStat);
			    break;

			  case ExOperStats::GROUP_BY_STATS:
			    newStat = new(heap_) ExHashGroupByStats(heap_);
			    ((ExHashGroupByStats *)newStat)->copyContents((ExHashGroupByStats *)stat);
			    newStat->setCollectStatsType(tempStatsMergeType);
			    insert(newStat);
			    break;

			  case ExOperStats::HASH_JOIN_STATS:
			    newStat = new(heap_) ExHashJoinStats(heap_);
			    ((ExHashJoinStats *)newStat)->copyContents((ExHashJoinStats *)stat);
			    newStat->setCollectStatsType(tempStatsMergeType);
			    insert(newStat);
			    break;

			  case ExOperStats::PROBE_CACHE_STATS:
			    newStat = new(heap_) ExProbeCacheStats(heap_);
			    ((ExProbeCacheStats *)newStat)->copyContents((ExProbeCacheStats *)stat);
			    newStat->setCollectStatsType(tempStatsMergeType);
			    insert(newStat);
			    break;

			  case ExOperStats::FAST_EXTRACT_STATS:
			    newStat = new(heap_) ExFastExtractStats(heap_);
			    ((ExFastExtractStats *)newStat)->copyContents((ExFastExtractStats *)stat);
			    newStat->setCollectStatsType(tempStatsMergeType);
			    insert(newStat);
			    break;
			  case ExOperStats::ESP_STATS:
			    newStat = new(heap_) ExESPStats(heap_);
			    ((ExESPStats *)newStat)->copyContents((ExESPStats *)stat);
			    newStat->setCollectStatsType(tempStatsMergeType);
			    insert(newStat);
			    break;

			  case ExOperStats::SORT_STATS:
			    newStat = new(heap_) ExSortStats(heap_);
			    ((ExSortStats *)newStat)->copyContents((ExSortStats *)stat);
			    newStat->setCollectStatsType(tempStatsMergeType);
			    insert(newStat);
			    break;

			  case ExOperStats::UDR_STATS:
			    newStat = new(heap_) ExUDRStats(heap_);
			    ((ExUDRStats *)newStat)->copyContents((ExUDRStats *)stat);
			    newStat->setCollectStatsType(tempStatsMergeType);
			    insert(newStat);
			    break;

			  case ExOperStats::SPLIT_TOP_STATS:
			    newStat = new(heap_) ExSplitTopStats(heap_);
			    ((ExSplitTopStats *)newStat)->copyContents((ExSplitTopStats *)stat);
			    newStat->setCollectStatsType(tempStatsMergeType);
			    insert(newStat);
			    break;

			  case ExOperStats::SE_STATS:
			    newStat = new(heap_)ExStorageEngineStats(heap_);
			    ((ExStorageEngineStats *)newStat)->
			      copyContents((ExStorageEngineStats *)stat);
			    newStat->setCollectStatsType(tempStatsMergeType);
			    insert(newStat);
			    break;

			  default:
			    ex_assert(FALSE, "Merging new unknown operator statistics type");
			  } // switch (statType)
			break;
		      }// switch     
		  }
	      } // default
	    } // switch
	}  // if merge == false
    }  // while
  return TRUE;
}

// inserts a new entry, stats, to 'this'
NABoolean ExStatisticsArea::insert(ExOperStats * stats)
{
  stats->setCollectStatsType(getCollectStatsType());
  stats->setSubReqType(subReqType_);
  // use addr of stats as its hash value
  Int64 hashData = stats->getHashData();
  entries_->insert((char*)&hashData, sizeof(Int64), stats);
  return TRUE;
}

// positions to the head of ExOperStats list
void ExStatisticsArea::position()
{
  entries_->position();
}

void ExStatisticsArea::position(char * hashData, Lng32 hashDatalen)
{
  entries_->position(hashData, hashDatalen);
}

void ExStatisticsArea::fixup(ExStatisticsArea *other)
{

  char *addrOfStatsVFTPtr, *myStatsVFTPtr;
  addrOfStatsVFTPtr = (char *)(this);
  myStatsVFTPtr = (char *)(other);
  *((Long *)addrOfStatsVFTPtr) = *((Long *)myStatsVFTPtr);
  // update the Vfptr table of Queue
  if (entries_ != NULL)
  {
    addrOfStatsVFTPtr = (char *)(entries_);
    myStatsVFTPtr = (char *)(other->entries_);
    *((Long *)addrOfStatsVFTPtr) = *((Long *)myStatsVFTPtr);
  }

  position();
  other->position();
  ExOperStats *myStat;
  ExOperStats *stat;
  if ((myStat = other->getNext()) != NULL)
  {
    myStatsVFTPtr = (char *)(myStat);
    while ((stat = getNext()) != NULL)
    {
      addrOfStatsVFTPtr = (char *)(stat);
      *((Long *)addrOfStatsVFTPtr) = *((Long *)myStatsVFTPtr);
    }
  }
  else
  {
    if ((stat = getNext()) != NULL)
    {
      ExOperStats tempStats;
      myStatsVFTPtr = (char *)(&tempStats);
      do
      {
        addrOfStatsVFTPtr = (char *)(stat);
        *((Long *)addrOfStatsVFTPtr) = *((Long *)myStatsVFTPtr);
      }
      while ((stat = getNext()) != NULL);
    }
  }
  if (masterStats_ != NULL)
  {
      if (other->masterStats_ != NULL)
      {
         masterStats_->fixup(other->masterStats_);
      }
      else
      {
        ExMasterStats masterTempStats;
        addrOfStatsVFTPtr = (char *)(masterStats_);
        myStatsVFTPtr = (char *)(&masterTempStats);
        *((Long *)addrOfStatsVFTPtr) = *((Long *)myStatsVFTPtr);
      }
  }

}

Int64 ExStatisticsArea::getHashData(ExOperStats::StatType type,
                                     Lng32 tdbId)
{
  switch (getCollectStatsType())
  {
  case ComTdb::ACCUMULATED_STATS:
    return 1;
  case ComTdb::PERTABLE_STATS:
  case ComTdb::PROGRESS_STATS:
    if (type == ExOperStats::ROOT_OPER_STATS)
      return 1;
    else if (type == ExOperStats::NO_OP)
      return -1;
    else
      return tdbId;
  case ComTdb::OPERATOR_STATS:
    if (type == ExOperStats::MEAS_STATS)
      return 1;
    else 
      return tdbId;
  default:
    return -1;
  }
}

ExOperStats * ExStatisticsArea::getNext()
{
  return (ExOperStats *)(entries_->getNext());
}

ExOperStats * ExStatisticsArea::get(const ExOperStatsId & id)
{
  entries_->position();

  ExOperStats * stat;
  while ((stat = getNext()) != NULL) {
    //    if (*stat->getId() == id) {
    if (stat->getId()->compare(id, getCollectStatsType())) {
      // found it. 
      return stat;
    }
  }

  // didn't find.
  return NULL; 
}

NABoolean ExStatisticsArea::anyHaveSentMsgIUD()
{
  if ((collectStatsType_ == ComTdb::OPERATOR_STATS) ||
      (collectStatsType_ == ComTdb::PERTABLE_STATS) ||
      (collectStatsType_ == ComTdb::ALL_STATS))
  {
    entries_->position();

    ExOperStats * stat;
    while ((stat = getNext()) != NULL) {
      if (stat->hasSentMsgIUD())
        return TRUE;
    }  
  }
  return FALSE;
}

// gets the first 'type' of stat entry with 'operType'.
ExOperStats * ExStatisticsArea::get(ExOperStats::StatType type, 
				    ComTdb::ex_node_type tdbType)
{
  entries_->position();

  ExOperStats * stat;
  while ((stat = getNext()) != NULL) {
    if ((stat->statType() == type) &&
	(stat->getTdbType() == tdbType)) {
      // found it. 
      return stat;
    }
  }

  // didn't find.
  return NULL;
}

// gets the first 'type' of stat entry with 'operType'.
ExOperStats * ExStatisticsArea::get(ExOperStats::StatType type, 
				    Lng32 tdbId)
{
  Int64 hashData = getHashData(type, tdbId);
  if (hashData != -1)
    entries_->position((char*)&hashData, sizeof(Int64));
  else
    entries_->position();

  ExOperStats * stat;
  while ((stat = getNext()) != NULL)
  {
    if ((collectStatsType_ == ComTdb::PERTABLE_STATS 
      || collectStatsType_ == ComTdb::PROGRESS_STATS) &&
      stat->statType() == type &&
      stat->getPertableStatsId() == tdbId)
      return stat;
    if ((stat->statType() == type) &&
	(stat->getTdbId() == tdbId)) 
      return stat;
    if ((type == ExOperStats::NO_OP) &&
	(stat->getTdbId() == tdbId)) 
      return stat;
  }

  // didn't find.
  return NULL;
}
// gets the next stat entry with 'tdbId'
ExOperStats * ExStatisticsArea::get(Lng32 tdbId)
{
  return get(ExOperStats::NO_OP, tdbId); 
}

void ExStatisticsArea::setMasterStats(ExMasterStats *masterStats)
{ 
  masterStats_ = masterStats; 
}

IpcMessageObjSize ExStatisticsArea::packedLength()
{

  IpcMessageObjSize  size = 0;
  if (smallStatsObj())
    {
      size = sizeof(unsigned char) // version
	+ sizeof(detailedFlags_.smallFlags_);

      entries_->position();
      ExOperStats * stat = getNext();
      //alignSizeForNextObj(size);
      size += stat->packedLength();
      return size;
    }

  if (statsInDp2())
    {
      //size += sizeof(getVersion());
      size += sizeof(unsigned char) // version info
	+ sizeof(detailedFlags_.smallFlags_) 
	+ sizeof(detailedFlags_.otherFlags_);
    }
  else
    {
      size += baseClassPackedLength();

      // add the flags size
      size += sizeof(flags_);
      size += sizeof(collectStatsType_);
      size += sizeof(collectStatsType_);
      size += sizeof(subReqType_);
    }

  // then we tell unPack how may ExOperStats it will find in this
  // stats area. This counter is a long
  size += sizeof(Lng32);

  // now visit all the entries and all their packed length
  entries_->position();

  ExOperStats * stat = NULL;
  while ((stat = getNext()) != NULL) {
    // we ship an indicator of the ExOperStats
    size += sizeof(ExOperStats::StatType);
    alignSizeForNextObj(size);
    size += stat->packedLength();
  };
  
  // Boolean to denote if MasterStats is packed in or not
  size += sizeof(NABoolean);
  if (masterStats_ != NULL)
  {
    alignSizeForNextObj(size);
    size += masterStats_->packedLength();
    /// we do not ship the other data members
  }
  return size;
  
}

IpcMessageObjSize
ExStatisticsArea::packSmallObjIntoMessage(IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize size = 0;

  unsigned char version = (unsigned char)getVersion();
  size += packIntoBuffer(buffer, version);

  size += packIntoBuffer(buffer, detailedFlags_.smallFlags_);

  // now pack the one entry in this stats area.
  // Caller has already checked that this stats area contains
  // only one entry.
  entries_->position();
  
  ExOperStats * stat = getNext();
  //  UInt32 temp = stat->alignedPack(buffer);
  UInt32 temp = stat->pack(buffer);
  size += temp;
  buffer += temp;

  return size;
}

IpcMessageObjSize
ExStatisticsArea::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize size = 0;
  
  if (smallStatsObj())
    {
      return packSmallObjIntoMessage(buffer);
    }

  if (statsInDp2())
    {      
      // When statistics related information is sent
      // from ESPs, it goes thru the IPC message protocol. 
      // Since eid doesn't send stats via ipc, we don't need the ipc overhead
      // when sending stats. 
      // Just send the version.
      unsigned char version = (unsigned char)getVersion();
      size += packIntoBuffer(buffer, version);

      size += packIntoBuffer(buffer, detailedFlags_.smallFlags_);

      size += packIntoBuffer(buffer, detailedFlags_.otherFlags_);
    }
  else
    {
      size += packBaseClassIntoMessage(buffer);

      // first pack the flags
      size += packIntoBuffer(buffer, flags_);
      size += packIntoBuffer(buffer, collectStatsType_);
      size += packIntoBuffer(buffer, origCollectStatsType_);
      size += packIntoBuffer(buffer, subReqType_);
    }

  // then pack the entry count
  Lng32 count = numEntries();
  size += packIntoBuffer(buffer, count);

  // now pack all the entries
  entries_->position();
  
  ExOperStats * stat;
  while ((stat = getNext()) != NULL) {
    size += packIntoBuffer(buffer, stat->statType());
    UInt32 temp = stat->alignedPack(buffer);
    size += temp;
    buffer += temp;
  }

  NABoolean isMasterStats;
  if (masterStats_ != NULL)
  {
    isMasterStats = TRUE;
    size += packIntoBuffer(buffer, isMasterStats); 
    UInt32 temp = masterStats_->alignedPack(buffer);
    size += temp;
    buffer += temp;
  }
  else
  {
    isMasterStats = FALSE;
    size += packIntoBuffer(buffer, isMasterStats); 
  }
  return size;
}

void ExStatisticsArea::unpackThisClass(const char* &buffer, ExOperStats *parentStatsEntry,
                                  Lng32 parentTdb)
{
  Lng32 counter;
  // get the entry count
  unpackBuffer(buffer, counter);
  // set up all the ExOperStat entries
  for (Lng32 i = 0; i < counter; i++) {
    ExOperStats::StatType statType;
    ExOperStats * stat = NULL;
    unpackBuffer(buffer, statType);
    switch (statType) {
    case ExOperStats::EX_OPER_STATS:
      stat = new(heap_) ExOperStats(heap_);
      break;
    case ExOperStats::MEAS_STATS:
      stat = new(heap_) ExMeasStats(heap_);
      break;
    case ExOperStats::ROOT_OPER_STATS:
      stat = new(heap_) ExFragRootOperStats(heap_);
      break;
    case ExOperStats::PARTITION_ACCESS_STATS:
      stat = new(heap_) ExPartitionAccessStats(heap_);
      break;
    case ExOperStats::GROUP_BY_STATS:
      stat = new(heap_) ExHashGroupByStats(heap_);
      break;
    case ExOperStats::HASH_JOIN_STATS:
      stat = new(heap_) ExHashJoinStats(heap_);
      break;
    case ExOperStats::PROBE_CACHE_STATS:
      stat = new(heap_) ExProbeCacheStats(heap_);
      break;
    case ExOperStats::FAST_EXTRACT_STATS:
      stat = new(heap_) ExFastExtractStats(heap_);
      break;
    case ExOperStats::ESP_STATS:
      stat = new(heap_) ExESPStats(heap_);
      break;
    case ExOperStats::SPLIT_TOP_STATS:
      stat = new(heap_) ExSplitTopStats(heap_);
      break;
    case ExOperStats::SORT_STATS:
      stat = new(heap_) ExSortStats(heap_);
      break;
    case ExOperStats::UDR_STATS:
      stat = new(heap_) ExUDRStats(heap_);
      break;
    case ExOperStats::RMS_STATS:
      stat = new(heap_) ExRMSStats((NAHeap *)heap_);
      break;
    case ExOperStats::BMO_STATS:
      stat = new(heap_) ExBMOStats((NAHeap *)heap_);
      break;
    case ExOperStats::UDR_BASE_STATS:
      stat = new(heap_) ExUDRBaseStats((NAHeap *)heap_);
      break;
    case ExOperStats::MASTER_STATS:
      stat = new(heap_) ExMasterStats((NAHeap *)heap_);
      break;
    case ExOperStats::PROCESS_STATS:
      stat = new(heap_) ExProcessStats((NAHeap *)heap_);
      break;
    case ExOperStats::SE_STATS:
     stat = new(heap_) ExStorageEngineStats((NAHeap *)heap_);
     break;
    default:
      FAILURE ;
    }
    stat->setStatsInDp2(statsInDp2());
    stat->setStatsInEsp(statsInEsp());
    stat->setCollectStatsType(getCollectStatsType());
    stat->alignedUnpack(buffer);
    insert(stat);
  }
}

void ExStatisticsArea::unpackObj(IpcMessageObjType objType,
				 IpcMessageObjVersion objVersion,
				 NABoolean sameEndianness,
				 IpcMessageObjSize objSize,
				 IpcConstMessageBufferPtr buffer)
{
  if (objVersion <= _STATS_PRE_RTS_VERSION)
    // an old statistics area is from SQL/MX Release 1 which did not
    // have full statistics support. Just throw the contents away.
    // Customers should not expect statistics measurements from
    // Release 1 systems.
    return;
  unpackBaseClass(buffer);

  // get the flags
  unpackBuffer(buffer, flags_);
  unpackBuffer(buffer, collectStatsType_);
  unpackBuffer(buffer, origCollectStatsType_);
  unpackBuffer(buffer, subReqType_);

  unpackThisClass(buffer);

  NABoolean isMasterStatsIn;

  unpackBuffer(buffer, isMasterStatsIn);
  if (isMasterStatsIn)
  {
    masterStats_ = new (heap_) ExMasterStats((NAHeap *)heap_);
    masterStats_->alignedUnpack(buffer);
  }
}

void
ExStatisticsArea::unpackSmallObjFromEid(IpcConstMessageBufferPtr buffer,
					Lng32 version)
{
  if (getCollectStatsType() == ComTdb::ACCUMULATED_STATS)
  {
    ExMeasStats * stat = new(heap_) ExMeasStats(heap_);
    stat->setStatsInDp2(statsInDp2());
    stat->setVersion(version);
    stat->unpack(buffer);
    insert(stat);
  }
  else
    ex_assert(0, "Statistics Area can't be a small object");
}

void ExStatisticsArea::unpackObjFromEid(IpcConstMessageBufferPtr buffer,
					ExOperStats *parentStatsEntry,
                                        Lng32 parentTdb)
{
  // get the version
  char version;
  unpackBuffer(buffer, version);
  
  setVersion(version);

  unpackBuffer(buffer, detailedFlags_.smallFlags_);
  
  if (smallStatsObj())
    {
      unpackSmallObjFromEid(buffer, version);
    }
  else
    {
      unpackBuffer(buffer, detailedFlags_.otherFlags_);
      unpackThisClass(buffer, parentStatsEntry, parentTdb);
    }
}

void ExStatisticsArea::updateSpaceUsage(Space *space,
         	   CollHeap *heap)
{
  if (rootStats_ != NULL)
  {
    if (rootStats_->castToExFragRootOperStats())
    {
      // determine the high water mark for space
      // and heap usage
      rootStats_->castToExFragRootOperStats()->
        updateSpaceUsage(space, heap);
    }
    else
    if (rootStats_->castToExMeasStats())
    {
      // determine the high water mark for space
      // and heap usage
      rootStats_->castToExMeasStats()->
        updateSpaceUsage(space, heap);
    }
  }
}

void ExStatisticsArea::initHistoryEntries()
{
  if (rootStats_ != NULL)
  {
    if (rootStats_->castToExFragRootOperStats())
      rootStats_->castToExFragRootOperStats()->initHistory();
    else
    if (rootStats_->castToExMeasStats())
      rootStats_->castToExMeasStats()->initHistory();
  }
}

Lng32 ExStatisticsArea::getStatsItems(Lng32 no_of_stats_items,
	    SQLSTATS_ITEM sqlStats_items[])
{
  Lng32 retcode = 0;
  Lng32 tempRetcode = 0;
  ExESPStats* espStats = NULL;
  ExFragRootOperStats* rootStats = NULL;
  ExHashGroupByStats* groupByStats = NULL;
  ExHashJoinStats* hashJoinStats = NULL;
  ExMasterStats *masterStats = NULL;
  ExMeasStats* measStats = NULL;
  ExOperStats* operStats = NULL;
  ExPartitionAccessStats* partitionAccessStats = NULL;
  ExProbeCacheStats* probeCacheStats = NULL;
  ExFastExtractStats* fastExtractStats = NULL;
  ExStorageEngineStats *seStats = NULL;
  ExHdfsScanStats* hdfsScanStats = NULL;
  ExHbaseAccessStats* hbaseAccessStats = NULL;
  ExSortStats* sortStats = NULL;
  ExSplitTopStats* splitTopStats = NULL;
  ExUDRStats* udrStats = NULL;
  ExRMSStats* rmsStats = NULL;
  ExBMOStats *bmoStats = NULL;
  ExUDRBaseStats *udrBaseStats = NULL;

  for (Int32 i = 0; i < no_of_stats_items; i++)
  {
    switch (sqlStats_items[i].stats_type)
    {
    case ExOperStats::EX_OPER_STATS:
      if (operStats == NULL) 
         operStats = (ExOperStats*)get(ExOperStats::EX_OPER_STATS, sqlStats_items[i].tdb_id);
      if (operStats != NULL)
        tempRetcode = operStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::MASTER_STATS:
      masterStats = getMasterStats();
      if (masterStats != NULL)
        tempRetcode = masterStats->getStatsItem(&sqlStats_items[i]);
     else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::MEAS_STATS:
      if (measStats == NULL)
        measStats = (ExMeasStats *)get(ExOperStats::MEAS_STATS, _UNINITIALIZED_TDB_ID);
      if (measStats != NULL)
        tempRetcode = measStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::ROOT_OPER_STATS:
      if (rootStats == NULL)
         rootStats = (ExFragRootOperStats *)get(ExOperStats::ROOT_OPER_STATS, 
                                      sqlStats_items[i].tdb_id);
      if (rootStats != NULL)
        tempRetcode = rootStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::BMO_STATS:
      if (bmoStats == NULL)
         bmoStats = (ExBMOStats*)get(ExOperStats::BMO_STATS, sqlStats_items[i].tdb_id);
      if (bmoStats != NULL)
        tempRetcode = bmoStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::UDR_BASE_STATS:
      if (udrBaseStats == NULL)
         udrBaseStats = (ExUDRBaseStats*)get(ExOperStats::UDR_BASE_STATS, sqlStats_items[i].tdb_id);
      if (udrBaseStats != NULL)
        tempRetcode = udrBaseStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::ESP_STATS:
      if (espStats == NULL)
         espStats = (ExESPStats*)get(ExOperStats::ESP_STATS, sqlStats_items[i].tdb_id);
      if (espStats != NULL)
        tempRetcode = espStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::GROUP_BY_STATS:
      if (groupByStats == NULL) 
         groupByStats = (ExHashGroupByStats*)get(ExOperStats::GROUP_BY_STATS, sqlStats_items[i].tdb_id);
      if (groupByStats != NULL)
        tempRetcode = groupByStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::HASH_JOIN_STATS:
      if (hashJoinStats == NULL) 
         hashJoinStats = (ExHashJoinStats*)get(ExOperStats::HASH_JOIN_STATS, sqlStats_items[i].tdb_id);
      if (hashJoinStats != NULL)
        tempRetcode = hashJoinStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::PARTITION_ACCESS_STATS:
      if (partitionAccessStats == NULL)
         partitionAccessStats = (ExPartitionAccessStats*)get(ExOperStats::PARTITION_ACCESS_STATS, sqlStats_items[i].tdb_id);
      if (partitionAccessStats != NULL)
        tempRetcode = partitionAccessStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::PROBE_CACHE_STATS:
      if (probeCacheStats == NULL)
         probeCacheStats = (ExProbeCacheStats*)get(ExOperStats::PROBE_CACHE_STATS, sqlStats_items[i].tdb_id);
      if (probeCacheStats != NULL)
        tempRetcode = probeCacheStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::FAST_EXTRACT_STATS:
      if (fastExtractStats == NULL)
        fastExtractStats = (ExFastExtractStats*)get(ExOperStats::FAST_EXTRACT_STATS, sqlStats_items[i].tdb_id);
      if (fastExtractStats != NULL)
        tempRetcode = fastExtractStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::SORT_STATS:
      if (sortStats == NULL)
         sortStats = (ExSortStats*)get(ExOperStats::SORT_STATS, sqlStats_items[i].tdb_id);
      if (sortStats != NULL)
        tempRetcode = sortStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::SPLIT_TOP_STATS:
      if (splitTopStats == NULL)
         splitTopStats = (ExSplitTopStats*)get(ExOperStats::SPLIT_TOP_STATS, sqlStats_items[i].tdb_id);
      if (splitTopStats != NULL)
        tempRetcode = splitTopStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::UDR_STATS:
      if (udrStats == NULL)
         udrStats = (ExUDRStats*)get(ExOperStats::UDR_STATS, sqlStats_items[i].tdb_id);
      if (udrStats != NULL)
        tempRetcode = udrStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::SE_STATS:
      if (seStats == NULL)
        seStats = (ExStorageEngineStats*)get(ExOperStats::HDFSSCAN_STATS, sqlStats_items[i].tdb_id);
      if (seStats != NULL)
        tempRetcode = seStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    case ExOperStats::RMS_STATS:
      if (rmsStats == NULL)
      {
        if (sqlStats_items[i].tdb_id == 0)
        {
          position();
          rmsStats = (ExRMSStats *)getNext();
        }
        else
          rmsStats = (ExRMSStats *)getNext();
      }
      if (rmsStats != NULL)
        tempRetcode = rmsStats->getStatsItem(&sqlStats_items[i]);
      else
        tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    default:
      tempRetcode = -EXE_STAT_NOT_FOUND;
      break;
    }
    
    // keep track of first error
    if (tempRetcode != 0 && retcode == 0)
        retcode = tempRetcode;
//     go thru all stats - don't break loop
//     break;
  }
  return retcode;
}

Lng32 ExStatisticsArea::getStatsDesc(short *statsCollectType,
	    /* IN/OUT */ SQLSTATS_DESC sqlstats_desc[],
	    /* IN */ 	Lng32 max_stats_desc,
	    /* OUT */	Lng32 *no_returned_stats_desc)
{
  ExOperStats *stat;
  Lng32 no_of_stats_desc = 0;
  Lng32 currTdbId = 0;
  Int32 i = 0;
  Lng32 tdbId;

  switch (getCollectStatsType())
  {
  case SQLCLI_ACCUMULATED_STATS:
    if (numEntries() == 0)
      no_of_stats_desc = 1;
    else
      no_of_stats_desc = 2;
    if (no_returned_stats_desc != NULL)
      *no_returned_stats_desc = no_of_stats_desc;
    if (max_stats_desc < no_of_stats_desc)
      return -CLI_INSUFFICIENT_STATS_DESC;
    sqlstats_desc[0].tdb_id = (short)_UNINITIALIZED_TDB_ID;
    sqlstats_desc[0].stats_type = ExOperStats::MASTER_STATS;
    sqlstats_desc[0].tdb_name[0] = '\0';
    if (no_of_stats_desc == 2)
    {
      sqlstats_desc[1].tdb_id =  (short)_UNINITIALIZED_TDB_ID;
      sqlstats_desc[1].stats_type = ExOperStats::MEAS_STATS;
      sqlstats_desc[1].tdb_name[0] = '\0';
    }
    break;
  case SQLCLI_OPERATOR_STATS:

    no_of_stats_desc = numEntries() + 1;
    if (no_returned_stats_desc != NULL)
      *no_returned_stats_desc = no_of_stats_desc;
    if (max_stats_desc < no_of_stats_desc)
      return -CLI_INSUFFICIENT_STATS_DESC;

    sqlstats_desc[0].tdb_id = (short)_UNINITIALIZED_TDB_ID;
    sqlstats_desc[0].stats_type = ExOperStats::MASTER_STATS;
    sqlstats_desc[0].tdb_name[0] = '\0';
    // get Max tdb_id for first id
    position();
    currTdbId = 0; 
    while ((stat = getNext()) != NULL)
    {
      if ((tdbId = stat->getTdbId()) > currTdbId)
          currTdbId = tdbId;
    } 
      
    // now get stats in reverse tdb_id order
    for (i = 1; i < no_of_stats_desc && currTdbId > 0;)
    {
        stat = get(currTdbId--);
        if(stat != NULL)
        {
          sqlstats_desc[i].tdb_id = (short)stat->getTdbId();
          sqlstats_desc[i].stats_type = stat->statType();
          str_cpy_all(sqlstats_desc[i].tdb_name, stat->getTdbName(), str_len(stat->getTdbName())+1);
          i++;
        }
    }
    no_of_stats_desc = i;
    if (no_returned_stats_desc != NULL)
      *no_returned_stats_desc = no_of_stats_desc;
    break;
  case SQLCLI_PERTABLE_STATS:
  case SQLCLI_PROGRESS_STATS:
    if (no_returned_stats_desc != NULL)
        *no_returned_stats_desc = numEntries()+1;
    if (max_stats_desc < numEntries()+1)
      return -CLI_INSUFFICIENT_STATS_DESC;
    i=0;
    sqlstats_desc[i].tdb_id = (short)_UNINITIALIZED_TDB_ID;
    sqlstats_desc[i].stats_type = ExOperStats::MASTER_STATS;
    sqlstats_desc[i].tdb_name[0] = '\0';
    i++;
    // Populate the stats descriptor for ROOT_OPER_STATS first
    position();
    for (; (stat = getNext()) != NULL; )
    {
      if (stat->statType() == ExOperStats::ROOT_OPER_STATS)
      {
        sqlstats_desc[i].tdb_id = (short)stat->getTdbId();
        sqlstats_desc[i].stats_type = ExOperStats::ROOT_OPER_STATS;
        sqlstats_desc[i].tdb_name[0] = '\0';
        i++;
      }
    } 
    position();
    for (; (stat = getNext()) != NULL; )
    {
      if (stat->statType() == ExOperStats::HBASE_ACCESS_STATS ||
         stat->statType() == ExOperStats::HDFSSCAN_STATS)
      {
        sqlstats_desc[i].tdb_id = (short)stat->getTdbId();
        sqlstats_desc[i].stats_type = stat->statType();
        sqlstats_desc[i].tdb_name[0] = '\0';
        i++;
      }
    }
    if ((Int32)getCollectStatsType() == SQLCLI_PROGRESS_STATS)
    {
      position();
      for ( ; ((stat = getNext()) != NULL && i < max_stats_desc) ; )
      { 
        if (stat->statType() == ExOperStats::BMO_STATS)
        {
          sqlstats_desc[i].tdb_id = (short)stat->getTdbId();
          sqlstats_desc[i].stats_type = stat->statType();
          sqlstats_desc[i].tdb_name[0] = '\0';
          i++;
        }
      }
      position();
      for ( ; ((stat = getNext()) != NULL && i < max_stats_desc) ; )
      { 
      }
      position();
      for ( ; ((stat = getNext()) != NULL && i < max_stats_desc) ; )
      { 
        if (stat->statType() == ExOperStats::REPLICATOR_STATS)
        {
          sqlstats_desc[i].tdb_id = (short)stat->getTdbId();
          sqlstats_desc[i].stats_type = stat->statType();
          sqlstats_desc[i].tdb_name[0] = '\0';
          i++;
          break;
        }
      }
      // Get UDR_BASE_STATS stats descriptor   
      position();
      for ( ; ((stat = getNext()) != NULL && i < max_stats_desc) ; )
      { 
        if (stat->statType() == ExOperStats::UDR_BASE_STATS)
        {
          sqlstats_desc[i].tdb_id = (short)stat->getTdbId();
          sqlstats_desc[i].stats_type = stat->statType();
          sqlstats_desc[i].tdb_name[0] = '\0';
          i++;
        }
      }
    }
    break;
  case SQLCLI_RMS_INFO_STATS:
     if (no_returned_stats_desc != NULL)
        *no_returned_stats_desc = numEntries();
    if (max_stats_desc < numEntries())
      return -CLI_INSUFFICIENT_STATS_DESC;
    position();
    for (i = 0; (stat = getNext()) != NULL; i++)
    {
      sqlstats_desc[i].tdb_id = i;
      sqlstats_desc[i].stats_type = stat->statType();
      sqlstats_desc[i].tdb_name[0] = '\0';
    }
    break;
  case SQLCLI_SAME_STATS:
  case SQLCLI_NO_STATS:
    // This is the case when only master stats is returned, SSMP doesn't know what is
    // the stats type
    if (no_returned_stats_desc != NULL)
        *no_returned_stats_desc = 1;
    if (max_stats_desc < 1)
      return -CLI_INSUFFICIENT_STATS_DESC;
    sqlstats_desc[0].tdb_id = (short)_UNINITIALIZED_TDB_ID;
    sqlstats_desc[0].stats_type = ExOperStats::MASTER_STATS;
    sqlstats_desc[0].tdb_name[0] = '\0';
    break;
  default:
    if (no_returned_stats_desc != NULL)
        *no_returned_stats_desc = 0;
    break;
  }
  return 0;
}

const char *ExStatisticsArea::getStatsTypeText(short statsType)
{
  switch (statsType)
  {
  case SQLCLI_NO_STATS: 
    return "NO_STATS";
  case SQLCLI_ACCUMULATED_STATS:
    return "ACCUMULATED_STATS";
  case SQLCLI_PERTABLE_STATS:
    return "PERTABLE_STATS";
  case SQLCLI_ALL_STATS:
    return "ALL_STATS";
  case SQLCLI_OPERATOR_STATS: 
    return "OPERATOR_STATS";
  case SQLCLI_PROGRESS_STATS:
    return "PROGRESS_STATS";
  default:
    return "UNKNOWN";
  }
}

void ExStatisticsArea::setCpuStatsHistory()
{
  position();
  
  ExOperStats *stat;
  ExOperStats::StatType statType;
  while ((stat = getNext()) != NULL)
  {
    statType = stat->statType();
    if (statType == ExOperStats::MEAS_STATS
        || statType == ExOperStats::ROOT_OPER_STATS)
    {
      stat->setCpuStatsHistory();
      break;
    }
  }
}

NABoolean ExStatisticsArea::appendCpuStats(ExStatisticsArea *stats, 
       NABoolean appendAlways, Lng32 filter, struct timespec currTimespec)
{
  ExMasterStats *masterStats; 
  ExOperStats *stat;
  ExOperStats::StatType statType;
  NABoolean retcode = FALSE;
  NABoolean retcode1 = FALSE;

  stats->position();
  while ((stat = stats->getNext()) != NULL) {
     statType = stat->statType();
     if (statType == ExOperStats::HBASE_ACCESS_STATS || statType == ExOperStats::HDFSSCAN_STATS) 
        retcode1 = appendCpuStats(stat, appendAlways, filter, currTimespec);
     if (retcode1 == TRUE)
        retcode = TRUE;
  }
  return retcode;
}

NABoolean ExStatisticsArea::appendCpuStats(ExMasterStats *masterStats, 
       NABoolean appendAlways, short subReqType,
       Lng32 etFilter, Int64 currTimestamp)
{
  NABoolean append = appendAlways;
  NABoolean retcode = FALSE;
  ExMasterStats *append_masterStats;

  if (!append)
     append = masterStats->filterForCpuStats(subReqType, 
                  currTimestamp, etFilter);
  if (append)
  {
     append_masterStats = new (getHeap()) ExMasterStats((NAHeap *)getHeap());
     append_masterStats->setCollectStatsType(getCollectStatsType());
     append_masterStats->copyContents(masterStats);
     insert(append_masterStats);
     retcode = TRUE;
  }
  return retcode;
}

NABoolean ExStatisticsArea::appendCpuStats(ExOperStats *stat, 
       NABoolean appendAlways, 
       Lng32 filter, struct timespec currTimespec)
{
  NABoolean append = appendAlways;
  NABoolean retcode = FALSE;
  ExHbaseAccessStats *append_hbaseStats;
  ExHdfsScanStats *append_hdfsStats;
  ExOperStats::StatType statType;
  
  statType = stat->statType();
  if (!append)
  {
     if (statType == ExOperStats::HBASE_ACCESS_STATS)
        append = ((ExHbaseAccessStats *)stat)->filterForSEstats(currTimespec, filter);
     else
     if (statType == ExOperStats::HDFSSCAN_STATS)
        append = ((ExHdfsScanStats *)stat)->filterForSEstats(currTimespec, filter);
  }
  if (append)
  {
     if (statType == ExOperStats::HBASE_ACCESS_STATS) {
        append_hbaseStats = new (getHeap()) ExHbaseAccessStats((NAHeap *)getHeap());
        append_hbaseStats->setCollectStatsType(getCollectStatsType());
        append_hbaseStats->copyContents((ExHbaseAccessStats *)stat);
        insert(append_hbaseStats);
     }
     else
     if (statType == ExOperStats::HDFSSCAN_STATS) {
        append_hdfsStats = new (getHeap()) ExHdfsScanStats((NAHeap *)getHeap());
        append_hdfsStats->setCollectStatsType(getCollectStatsType());
        append_hdfsStats->copyContents((ExHdfsScanStats *)stat);
        insert(append_hdfsStats);
     }
     retcode = TRUE;
  }
  return retcode;
}

NABoolean ExStatisticsArea::appendCpuStats(ExStatisticsArea *other, 
       NABoolean appendAlways)
{
  ComTdb::CollectStatsType collectStatsType, appendStatsType;
  ExOperStats *stat;
  ExMeasStats *measStats;
  ExFragRootOperStats *rootOperStats;
  ExRMSStats *rmsStats;
  ExBMOStats *bmoStats;
  ExUDRBaseStats *udrBaseStats;
  ExMasterStats *masterStats;
  ExProcessStats *processStats;
  ExStorageEngineStats *seStats;
  ExHdfsScanStats *hdfsScanStats;
  ExHbaseAccessStats *hbaseAccessStats;
  NABoolean retcode = FALSE;
  ExOperStats::StatType statType;
  ExOperStats *stat1;
  ComTdb::ex_node_type tdbType;
  collectStatsType = other->getCollectStatsType();
  appendStatsType = getCollectStatsType(); 
  short subReqType = getSubReqType();
  // It is possible that StatisticsArea at the ESP might contain
  // ROOT_OPER_STATS entries from other ESPs/EID. We should not
  // be looking at those ROOT_OPER_STATS when CPU offender stats is requested
  // AppendAlways is set to TRUE when the CPU offender stats is shipped from 
  // SSCP to SSMP or SSMP to Collector. 
  if (!appendAlways && (Int32)appendStatsType == SQLCLI_CPU_OFFENDER_STATS)
  {
    stat = other->getRootStats();
    if (stat != NULL)
    {
       statType = stat->statType();
       switch (statType)
       {
          case ExOperStats::ROOT_OPER_STATS:
             if (((ExFragRootOperStats *)stat)->filterForCpuStats())
             {
                rootOperStats = new (getHeap()) ExFragRootOperStats(getHeap());
                rootOperStats->setCollectStatsType(getCollectStatsType());
                rootOperStats->copyContents((ExFragRootOperStats *)stat);
                insert(rootOperStats);
                retcode = TRUE;
              }
              break;
          case ExOperStats::MEAS_STATS:
             if (((ExMeasStats *)stat)->filterForCpuStats())
             {
                measStats = new (getHeap()) ExMeasStats(getHeap());
                measStats->setCollectStatsType(getCollectStatsType());
                measStats->copyContents((ExMeasStats *)stat);
                insert(measStats);
                retcode = TRUE;
             }
             break;
          default:
             break;
       } 
    }
    return retcode;
  }
  other->position();
  while ((stat = other->getNext()) != NULL)
  {
    statType = stat->statType();
    switch (appendStatsType)
    {
      case SQLCLI_CPU_OFFENDER_STATS:
      {
        switch (statType)
        {
        case ExOperStats::MEAS_STATS:
          if (appendAlways || ((ExMeasStats *)stat)->filterForCpuStats())
          {
            measStats = new (getHeap()) ExMeasStats(getHeap());
            measStats->setCollectStatsType(getCollectStatsType());
            measStats->copyContents((ExMeasStats *)stat);
            insert(measStats);
            retcode = TRUE;
          }
          break;
        case ExOperStats::ROOT_OPER_STATS:
          tdbType = stat->getTdbType();
          if (tdbType == ComTdb::ex_SPLIT_BOTTOM || tdbType == ComTdb::ex_ROOT)
          {
            if (appendAlways || ((ExFragRootOperStats *)stat)->filterForCpuStats())
            {
              rootOperStats = new (getHeap()) ExFragRootOperStats(getHeap());
              rootOperStats->setCollectStatsType(getCollectStatsType());
              rootOperStats->copyContents((ExFragRootOperStats *)stat);
              insert(rootOperStats);
              retcode = TRUE;
            }
          }
          break;
        default:
          break;
        }
        break;
      }
      case SQLCLI_ET_OFFENDER_STATS:
      {
         if (statType ==  ExOperStats::MASTER_STATS)
         {
            masterStats = new (getHeap()) ExMasterStats((NAHeap *)getHeap());
            masterStats->setCollectStatsType(getCollectStatsType());
            masterStats->copyContents((ExMasterStats *)stat);
            insert(masterStats);
            retcode = TRUE;
         } 
         break;
      }
      case SQLCLI_QID_DETAIL_STATS:
      {
         switch (statType)
        {
        case ExOperStats::EX_OPER_STATS:
          if (detailLevel_ == stat->getTdbId())
          {
            stat1 = new (getHeap()) ExOperStats(getHeap());
            stat1->setCollectStatsType(getCollectStatsType());
            stat1->copyContents((ExOperStats *)stat);
            insert(stat1);
            retcode = TRUE;
          }
          break;
        case ExOperStats::ROOT_OPER_STATS:
          tdbType = stat->getTdbType();
          if ((detailLevel_ == -1 && 
            (tdbType == ComTdb::ex_SPLIT_BOTTOM || tdbType == ComTdb::ex_ROOT))
                 || detailLevel_ == stat->getTdbId())
          {
            rootOperStats = new (getHeap()) ExFragRootOperStats(getHeap());
            rootOperStats->setCollectStatsType(getCollectStatsType());
            rootOperStats->copyContents((ExFragRootOperStats *)stat);
            insert(rootOperStats);
            retcode = TRUE;
          }
          break;
        case ExOperStats::BMO_STATS:
          if (detailLevel_ == stat->getTdbId())
          {
            bmoStats = new (getHeap()) ExBMOStats(getHeap());
            bmoStats->setCollectStatsType(getCollectStatsType());
            bmoStats->copyContents((ExBMOStats *)stat);
            insert(bmoStats);
            retcode = TRUE;
          }
          break;
        case ExOperStats::UDR_BASE_STATS:
          if (detailLevel_ == stat->getTdbId())
          {
            udrBaseStats = new (getHeap()) ExUDRBaseStats(getHeap());
            udrBaseStats->setCollectStatsType(getCollectStatsType());
            udrBaseStats->copyContents((ExUDRBaseStats *)stat);
            insert(udrBaseStats);
            retcode = TRUE;
          }
          break;
        case ExOperStats::SE_STATS:
          if (detailLevel_ == stat->getTdbId())
          {
            seStats = new (getHeap()) ExStorageEngineStats(getHeap());
            seStats->setCollectStatsType(getCollectStatsType());
            seStats->copyContents((ExHdfsScanStats *)stat);
            insert(seStats);
            retcode = TRUE;
          }
          break;
        default:
          break;
        } // StatType case
      }
      break;
    case SQLCLI_RMS_INFO_STATS:
      if (statType == ExOperStats::RMS_STATS)
      {
        rmsStats = new (getHeap()) ExRMSStats((NAHeap *)getHeap());
        rmsStats->setCollectStatsType(getCollectStatsType());
        rmsStats->copyContents((ExRMSStats *)stat);
        insert(rmsStats);
        retcode = TRUE;
      }
      break;
    case SQLCLI_MEM_OFFENDER_STATS:
      if (statType == ExOperStats::PROCESS_STATS)
      {
        processStats = new (getHeap()) ExProcessStats((NAHeap *)getHeap());
        processStats->setCollectStatsType(getCollectStatsType());
        processStats->copyContents((ExProcessStats *)stat);
        insert(processStats);
        retcode = TRUE;
      }
      break;
    case SQLCLI_SE_OFFENDER_STATS:
      if (appendAlways)
      {
         if (stat->castToExFragRootOperStats())
         {
            rootOperStats = new (getHeap()) ExFragRootOperStats(getHeap());
            rootOperStats->setCollectStatsType(getCollectStatsType());
            rootOperStats->copyContents((ExFragRootOperStats *)stat);
            insert(rootOperStats);
            retcode = TRUE;
         }
         else if (stat->castToExHdfsScanStats())
         {
            hdfsScanStats = new (getHeap()) ExHdfsScanStats(getHeap());
            hdfsScanStats->setCollectStatsType(getCollectStatsType());
            hdfsScanStats->copyContents((ExHdfsScanStats *)stat);
            insert(hdfsScanStats);
            retcode = TRUE;
         }
         else if (stat->castToExHbaseAccessStats())
         {
            hbaseAccessStats = new (getHeap()) ExHbaseAccessStats(getHeap());
            hbaseAccessStats->setCollectStatsType(getCollectStatsType());
            hbaseAccessStats->copyContents((ExHbaseAccessStats *)stat);
            insert(hbaseAccessStats);
            retcode = TRUE;
         }
      }
      break;
    default:
      break;
   }
 }
 return retcode;
}

void ExStatisticsArea::incReqMsg(Int64 msgBytes)
{
  ExFragRootOperStats *fragRootOperStats;
  ExMeasStats *measStats;

  if (rootStats_ != NULL)
  {
    if ((fragRootOperStats = rootStats_->castToExFragRootOperStats()) != NULL)
      fragRootOperStats->incReqMsg(msgBytes);
    else
    if ((measStats = rootStats_->castToExMeasStats()) != NULL)
      measStats->incReqMsg(msgBytes);
  }
}

void ExStatisticsArea::incReplyMsg(Int64 msgBytes)
{
  ExFragRootOperStats *fragRootOperStats;
  ExMeasStats *measStats;

  if (rootStats_ != NULL)
  {
    if ((fragRootOperStats = rootStats_->castToExFragRootOperStats()) != NULL)
      fragRootOperStats->incReplyMsg(msgBytes);
    else
    if ((measStats = rootStats_->castToExMeasStats()) != NULL)
      measStats->incReplyMsg(msgBytes);
  }
}

void ExStatisticsArea::setQueryId(char *queryId, Lng32 queryIdLen)
{
  if (!statsInDp2())
     return;
  entries_->position();
  ExOperStats * stat;
  while ((stat = getNext()) != NULL) 
  {
    if (stat->castToExFragRootOperStats())
       stat->castToExFragRootOperStats()->setQueryId(queryId, queryIdLen);
  }
}

///////////////////////////////////////////////////////////////////
// Methods for ExStatsTdb and ExStatsTcb.
///////////////////////////////////////////////////////////////////
ex_tcb * ExStatsTdb::build(ex_globals * glob)
{

  // Allocate and initialize a new stats TCB.
  ExStatsTcb *statsTcb = new(glob->getSpace()) ExStatsTcb(*this, glob);

  // add the stats tcb to the scheduler's task list.
  statsTcb->registerSubtasks();
  
  return statsTcb;
}

// Constructor called during build phase (ExStatsTdb::build()).
ExStatsTcb::ExStatsTcb(const ExStatsTdb & statsTdb, ex_globals *glob)
: ex_tcb(statsTdb, 1, glob)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);

  // Allocate the buffer pool
  // Allocate the specified number of buffers each can hold 5 tuples.
  pool_ = new(space) sql_buffer_pool(statsTdb.numBuffers_,
				     (Lng32)statsTdb.bufferSize_,
				     space);

  // Allocate the queues used to communicate with parent
  qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				      statsTdb.queueSizeDown_,
				      statsTdb.criDescDown_,
				      space);

  // Allocate the private state in each entry of the down queue
  ExStatsPrivateState *p = new(space) ExStatsPrivateState();
  qparent_.down->allocatePstate(p, this);
  delete p;

  qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				    statsTdb.queueSizeUp_,
				    statsTdb.criDescUp_,
				    space);

  // allocate space where the stats row will be created.
  pool_->get_free_tuple(tuppData_, statsTdb.tupleLen_);
  data_ = tuppData_.getDataPointer();

  if (statsTdb.workCriDesc_)
    {
      workAtp_ = allocateAtp(statsTdb.workCriDesc_, glob->getSpace());
      pool_->get_free_tuple(workAtp_->getTupp(statsTdb.getStatsTupleAtpIndex()), 0);
      if (statsTdb.inputExpr_)
	pool_->get_free_tuple(workAtp_->getTupp(statsTdb.getInputTupleAtpIndex()), (Lng32)statsTdb.getInputTupleLength());
    }
  else
    workAtp_ = 0;

  // fixup expressions
  if (statsTdb.scanExpr_)
    (void) statsTdb.scanExpr_->fixup(0, getExpressionMode(), this,
				     space, heap, FALSE, glob);

  if (statsTdb.inputExpr_)
    (void) statsTdb.inputExpr_->fixup(0, getExpressionMode(), this,
				     space, heap, FALSE, glob);
 
  if (statsTdb.projExpr_)
    (void) statsTdb.projExpr_->fixup(0, getExpressionMode(), this,
				     space, heap, FALSE, glob);
 
  stats_ = NULL;
  inputModName_ = NULL;
  inputStmtName_ = NULL;
  cpu_ = -1;
  pid_ = -1;
  nodeName_[0] = '\0';
  timeStamp_ = -1;
  queryNumber_ = -1;
  qid_ = NULL;
  reqType_ = SQLCLI_STATS_REQ_STMT;
  retryAttempts_ = 0;
  diagsArea_ = NULL;
  activeQueryNum_ = RtsQueryId::ANY_QUERY_;
  statsMergeType_ = SQLCLI_SAME_STATS;
  flags_ = 0;
  detailLevel_ = 0;
  stmtName_ = 0;
  subReqType_ = -1;
  filter_ = -1;
}

// Destructor for stats tcb
ExStatsTcb::~ExStatsTcb()
{
  delete qparent_.up;
  delete qparent_.down;
  freeResources();
}
  
void ExStatsTcb::freeResources()
{
  if (qid_ != NULL)
  {
    NADELETEBASIC(qid_, getHeap());
    qid_ = NULL;
  }
  if (stats_ != NULL && deleteStats())
  {
    NADELETE(stats_, ExStatisticsArea, stats_->getHeap());
    stats_ = NULL;
    setDeleteStats(FALSE);
  }
  if (inputStmtName_)
  {
    NADELETEBASIC(inputStmtName_, getHeap());
    inputStmtName_ = NULL;
  }
  if (inputModName_)
  {
    NADELETEBASIC(inputModName_, getHeap());
    inputModName_ = NULL;
  }
  if (diagsArea_)
  {
    diagsArea_->clear();
    diagsArea_->deAllocate();
    diagsArea_ = NULL;
  }
  if (stmtName_ != NULL)
  {
    NADELETEBASIC(qid_, getHeap());
    stmtName_ = NULL;
  }
}


short ExStatsTcb::work()
{
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExStatsPrivateState * pstate = 
    (ExStatsPrivateState*) pentry_down->pstate;
  NABoolean found = FALSE;
  Lng32 statsParamType = SQLCLI_STATS_REQ_NONE;
  char * stmtName = NULL;
  ExStatisticsArea *stats = NULL;
 
  while (1)
    {
      switch (pstate->step_)
	{
	case INITIAL_:
	  {
          ContextCli * currContext = NULL;
          freeResources();

	    
            if (statsTdb().getInputExpr())
	      {
		ex_expr::exp_return_type evalRetCode;
		evalRetCode = 
		  statsTdb().getInputExpr()->eval(pentry_down->getAtp(), 
						  workAtp_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    ex_assert(0, "ExStatsTcb::work(). Error from inputExpr_->eval()");
		  }

		char * inputTuple = 
		  workAtp_->getTupp(statsTdb().getInputTupleAtpIndex()).getDataPointer();
		Attributes * modAttr = statsTdb().getAttrModName();
		Attributes * stmtAttr = statsTdb().getAttrStmtName();

		if ((stmtAttr->getNullFlag()) &&
		    (str_cmp(&inputTuple[stmtAttr->getNullIndOffset()],
			     "\377\377", 2) == 0))
		  stmtName = NULL; // NULL stmt, get current Stats.
		else
		  stmtName = &inputTuple[stmtAttr->getOffset()];
		    
		if (stmtName)
		{
		  inputStmtName_ = new(getHeap()) char[stmtAttr->getLength()+1];
                  str_cpy_all(inputStmtName_, stmtName, stmtAttr->getLength());
		  inputStmtName_[stmtAttr->getLength()] = 0;
                  // remove trailing blanks
		  str_cpy_and_null(inputStmtName_, inputStmtName_,
				   stmtAttr->getLength(), 
				   '\0', ' ');
                  statsParamType = parse_stmt_name(inputStmtName_, str_len(inputStmtName_));
                  stmtName = stmtName_;
		}
                else
                {
                  stmtName = inputStmtName_;
                  statsParamType = SQLCLI_STATS_REQ_STMT;
                }
                switch (statsParamType)
                {
                  case SQLCLI_STATS_REQ_STMT:
                    {
		      // find this statement in the list of all the statements
		      // in the current context.
	              currContext = getGlobals()->castToExExeStmtGlobals()->
	                castToExMasterStmtGlobals()->getStatement()->
	                getContext();
	              HashQueue * stmtList = currContext->getStatementList();
		      stmtList->position();
		      Statement * stmt;
                      if (stmtName != NULL && (strncasecmp(stmtName, "CURRENT", 7) != 0))
                      {
		        while ((NOT found) &&
		               (stmt = (Statement *)stmtList->getNext()))
		        {
		          const char *ident = stmt->getIdentifier();

		          if ((ident) &&
			      (str_len(stmtName) == str_len(ident)) &&
			      (str_cmp(stmtName, ident, str_len(ident)) == 0)) // matches
		            {
			      stats = stmt->getStatsArea();
			      found = TRUE;
		            }
                        } // while
                      }
                      else
                        stats = currContext->getStats();
                      if ((stats == NULL) ||
                          (NOT stats->statsEnabled()))
                        pstate->step_ = DONE_;
                      else
                      { 
                        if (statsMergeType_ == SQLCLI_SAME_STATS || 
                            statsMergeType_ == stats->getCollectStatsType() ||
                            // if the collection type is ALL_STATS, ignore statsMergeType_
                            stats->getOrigCollectStatsType() == ComTdb::ALL_STATS) 
                        {
                            stats_ = stats;
                        }
                        else
                        {
                          stats_ = new (getHeap()) ExStatisticsArea(getHeap(),
                                        0, (ComTdb::CollectStatsType)statsMergeType_,
                                        stats->getOrigCollectStatsType());
                          StatsGlobals *statsGlobals = getGlobals()->getStatsGlobals();
                          Long semId;
                          if (statsGlobals != NULL)
                          {
                            semId = getGlobals()->getSemId();
                            int error = statsGlobals->getStatsSemaphore(semId, getGlobals()->getPid());
                            stats_->merge(stats, statsMergeType_);
                            setDeleteStats(TRUE);
                            statsGlobals->releaseStatsSemaphore(semId, getGlobals()->getPid());
                          }
                          else
                          {
                            stats_->merge(stats, statsMergeType_);                             
                            setDeleteStats(TRUE);
                          }
                        }
                        stats_->position();
                        pstate->step_ = GET_NEXT_STATS_ENTRY_;
                      }
                    }
                    break;
                  case SQLCLI_STATS_REQ_CPU:
                  case SQLCLI_STATS_REQ_PID:
                  case SQLCLI_STATS_REQ_QID:
                  case SQLCLI_STATS_REQ_QID_DETAIL:
                  case SQLCLI_STATS_REQ_QID_INTERNAL:
                  case SQLCLI_STATS_REQ_CPU_OFFENDER:
                  case SQLCLI_STATS_REQ_SE_OFFENDER:
                  case SQLCLI_STATS_REQ_RMS_INFO:
                  case SQLCLI_STATS_REQ_ET_OFFENDER:
                  case SQLCLI_STATS_REQ_MEM_OFFENDER:
                  case SQLCLI_STATS_REQ_PROCESS_INFO:
                    pstate->step_ = SEND_TO_SSMP_;
                    break;
                  case SQLCLI_STATS_REQ_QID_CURRENT:
                    {
                      currContext = getGlobals()->castToExExeStmtGlobals()->
	                castToExMasterStmtGlobals()->getStatement()->
	                getContext();
                      stats = currContext->getStats();
                      if ((stats == NULL) || 
		          (NOT stats->statsEnabled()))
                      {
                        if (stats == NULL && (! diagsArea_))
                        {
                          IpcAllocateDiagsArea(diagsArea_, getHeap());
                          (*diagsArea_) << DgSqlCode(-EXE_RTS_QID_NOT_FOUND) << DgString0("CURRENT QID");
                        }
                        if (diagsArea_)
                          pstate->step_ = ERROR_;
                        else
                          pstate->step_ = DONE_;
                      }
	              else
                      {
                        if (stats->getMasterStats() == NULL)
                          pstate->step_ = DONE_;
                        else
                        {
                          if (statsMergeType_ == SQLCLI_SAME_STATS || 
                              (statsMergeType_ == stats->getCollectStatsType()))
                          {
                              stats_ = stats;
                          }
                          else
                          {
                            stats_ = new (getHeap()) ExStatisticsArea(getHeap(),
                                          0, (ComTdb::CollectStatsType)statsMergeType_,
                                          stats->getOrigCollectStatsType());
                            StatsGlobals *statsGlobals = getGlobals()->getStatsGlobals();
                            Long semId;
                            if (statsGlobals != NULL)
                            {
                              semId = getGlobals()->getSemId();
                              int error = statsGlobals->getStatsSemaphore(semId, getGlobals()->getPid());
                              stats_->merge(stats, statsMergeType_);
                              setDeleteStats(TRUE);
                              statsGlobals->releaseStatsSemaphore(semId, getGlobals()->getPid());
                            }
                            else
                            {
                              stats_->merge(stats, statsMergeType_);                             
                              setDeleteStats(TRUE);
                            }
                          }
                          if (stats_->getMasterStats() != NULL)
                          {
                            stats_->getMasterStats()->setNumCpus((short)stats_->getMasterStats()->compilerStatsInfo().dop());
                          }
                          pstate->step_ = GET_MASTER_STATS_ENTRY_;
                        }
                      }
                      break;
                    }
                  default:
                    IpcAllocateDiagsArea(diagsArea_, getHeap());
                    (*diagsArea_) << DgSqlCode(-EXE_RTS_INVALID_QID) << DgString0(inputStmtName_);
                    pstate->step_ = ERROR_;
                    break;
                }
            } // input Expr Present
            else
            {
              stats_ = currContext->getStats();
              if ((stats_ == NULL) ||
                  (NOT stats_->statsEnabled()))
                pstate->step_ = DONE_;
              else
                pstate->step_ = GET_MASTER_STATS_ENTRY_;
            }
          }
	  break;
        case SEND_TO_SSMP_:
          {
            stats_ = sendToSsmp();
            getGlobals()->castToExExeStmtGlobals()->
	     castToExMasterStmtGlobals()->getCliGlobals()->getEnvironment()->deleteCompletedMessages();
  	    if ((stats_ == NULL) || 
		(NOT stats_->statsEnabled()))
            {
              if (stats_ == NULL && reqType_ == SQLCLI_STATS_REQ_QID 
                             && (! diagsArea_))
              {
                IpcAllocateDiagsArea(diagsArea_, getHeap());
                (*diagsArea_) << DgSqlCode(-EXE_RTS_QID_NOT_FOUND) << DgString0(qid_);
              }
              if (diagsArea_ && diagsArea_->getNumber(DgSqlCode::ERROR_) != 0)
                pstate->step_ = ERROR_;
              else
                pstate->step_ = DONE_;
            }
	    else
            {
              setDeleteStats(TRUE);
              if ((Int32)stats_->getCollectStatsType() == SQLCLI_CPU_OFFENDER_STATS ||
                  (Int32)stats_->getCollectStatsType() == SQLCLI_MEM_OFFENDER_STATS ||
                  (Int32)stats_->getCollectStatsType() == SQLCLI_SE_OFFENDER_STATS ||
                  (Int32)stats_->getCollectStatsType() == SQLCLI_QID_DETAIL_STATS ||
                  (Int32)stats_->getCollectStatsType() == SQLCLI_RMS_INFO_STATS ||
                  (Int32)stats_->getCollectStatsType() == SQLCLI_ET_OFFENDER_STATS ||
                  reqType_ == SQLCLI_STATS_REQ_PROCESS_INFO)

              {
                if (stats_->getMasterStats() == NULL)
                   pstate->step_ = GET_MASTER_STATS_ENTRY_;
                else
                {
                   stats_->position();
                   pstate->step_ = GET_NEXT_STATS_ENTRY_;
                }
              }
              else
              {
                if (stats_->getMasterStats() == NULL)
                  pstate->step_ = DONE_;
                else
                  pstate->step_ = GET_MASTER_STATS_ENTRY_;
              }
            }
          }
          break;
        case GET_MASTER_STATS_ENTRY_:
          if (stats_->getMasterStats() == NULL)
          {
            stats_->position();
            pstate->step_ = GET_NEXT_STATS_ENTRY_;
          }
          else
          {
            ExOperStats *stat = stats_->getMasterStats();
            getColumnValues(stat);
            stats_->position();
            pstate->step_ = APPLY_SCAN_EXPR_;
          }
          break;      
	case GET_NEXT_STATS_ENTRY_:
	  {
	    ExOperStats * stat = stats_->getNext();
	    if (stat == NULL)
              pstate->step_ = DONE_;
	    else
	    {
              getColumnValues(stat);
              pstate->step_ = APPLY_SCAN_EXPR_;
            }
	  }
	break;
	
	case APPLY_SCAN_EXPR_:
	  {
	    workAtp_->getTupp(statsTdb().getStatsTupleAtpIndex()).
	      setDataPointer(data_);
	    NABoolean rowSelected = TRUE;
	    if (statsTdb().scanExpr_)
	      {
		ex_expr::exp_return_type evalRetCode;
		evalRetCode = 
		  statsTdb().scanExpr_->eval(pentry_down->getAtp(), 
					     workAtp_);
		if (evalRetCode == ex_expr::EXPR_FALSE)
		  rowSelected = FALSE;
		else if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    ex_assert(
			 0,
			 "ExStatsTcb::work(). Error from scanExpr_->eval()");
		    rowSelected = FALSE;
		  }
	      }

	    if (rowSelected)
	      pstate->step_ = PROJECT_;
	    else
	      pstate->step_ = GET_NEXT_STATS_ENTRY_;
	  }
	break;

	case PROJECT_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    ex_queue_entry *pentry_up = qparent_.up->getTailEntry();
	    pentry_up->copyAtp(pentry_down);
	    if (pool_->get_free_tuple
		(pentry_up->getTupp(statsTdb().criDescUp_->noTuples()-1), 
		 statsTdb().tupleLen_))
	      {
		return WORK_POOL_BLOCKED;
	      }

	    pstate->matchCount_++;
	       
	    str_cpy_all(
		 pentry_up->getTupp(
		      statsTdb().criDescUp_->noTuples()-1).getDataPointer(),
		 data_, statsTdb().tupleLen_);
	    pentry_up->upState.downIndex = qparent_.down->getHeadIndex();
	    pentry_up->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    pentry_up->upState.setMatchNo(pstate->matchCount_);
	    pentry_up->upState.status = ex_queue::Q_OK_MMORE;

	    qparent_.up->insert();
    
	    pstate->step_ = GET_NEXT_STATS_ENTRY_;
	    
	  }
	break;

	case ERROR_:
	  {
            if (qparent_.up->isFull())
	      return WORK_OK;

            if (diagsArea_)
            {
              // Calling ExHandleArkcmpErrors - this is a misnomer - the 
              // point of this routine is to merge diags, if not null,
              // into the qparent's diags, and otherwise, create a new 
              // condition with the 2024 error and put that in the 
              // qparent's diags.
              ExHandleArkcmpErrors(qparent_,
			     pentry_down,
			     0,
			     getGlobals(),
                             getDiagsArea(),
			     (ExeErrorCode)(-2024)
                            );

              //Now that diags have been merged into the qparent_'s diags are,
              //clear it out and deallocate it.
              diagsArea_->clear();
              diagsArea_->deAllocate();
              diagsArea_ = NULL;
            }
            pstate->step_ = DONE_;
	  }
	break;

	case DONE_:
          {
	    if (qparent_.up->isFull())
	      return WORK_OK;
            ex_queue_entry *pentry_up = qparent_.up->getTailEntry();
	    pentry_up->copyAtp(pentry_down);
	    pentry_up->upState.status = ex_queue::Q_NO_DATA;
	    pentry_up->upState.parentIndex = 
	        pentry_down->downState.parentIndex;
	    pentry_up->upState.downIndex = qparent_.down->getHeadIndex();
	    pentry_up->upState.setMatchNo(pstate->matchCount_);
            if (diagsArea_)
            {
              ComDiagsArea *da = pentry_up->getDiagsArea();
              if (da == NULL)
                da = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
              else
                da->incrRefCount();
              da->mergeAfter(*diagsArea_);
              pentry_up->setDiagsArea(da);
              diagsArea_->clear();
              diagsArea_->deAllocate();
              diagsArea_ = NULL;
            }     
            qparent_.up->insert();
	    // remove the down entry
      	    qparent_.down->removeHead();
            pstate->step_ = INITIAL_;
	    return WORK_CALL_AGAIN;
	  }
	break;

	} // switch
    }
}

void ExStatsTcb::getColumnValues(ExOperStats *stat)
{
  ExpTupleDesc * tDesc =
    statsTdb().workCriDesc_->getTupleDescriptor(
       statsTdb().getStatsTupleAtpIndex());
  for (UInt32 i = 0; i < tDesc->numAttrs(); i++)
  {
    Attributes * attr = tDesc->getAttr(i);
    Int64 int64Val = 0;
    char * src = (char *)&int64Val;
    short srcType;
    Lng32 srcLen;
    short valIsNull = 0;
    srcType = REC_BIN64_SIGNED;
    srcLen = 8;
    NABoolean callConvDoit = TRUE;
    char sName[42];
    Int64 sNameLen = 40;

    Int32 valNum = 1;
    switch (i)
      {
      case STAT_MOD_NAME:
	src = inputModName_;
	if (src)
	  srcLen = str_len(src);
	else
	  valIsNull = -1;
	srcType = REC_BYTE_F_ASCII;
	break;

      case STAT_STATEMENT_NAME:
	src = inputStmtName_;
	if (src)
        {
          // If the statement name starts with QID=, return only the actual 
          // statement name by extracting it from the QID. The statement_name 
          // field of the statistics virtual table is supposed to be only 60 
          // chars long, and we end up truncating the statement_name if we return 
          // the entire QID, which is at least 70 chars long.
          if (strncasecmp(src, "QID=", 4) == 0)
          {            
            ComSqlId::getSqlQueryIdAttr(ComSqlId::SQLQUERYID_STMTNAME,
			      src+4,
			      (str_len(src) - 4),
			      sNameLen,
			      sName);
            src = sName;
            srcLen = str_len(sName);
          }
          else
            srcLen = str_len(src);
        }
	else
	  valIsNull = -1;
	srcType = REC_BYTE_F_ASCII;
	break;

      case STAT_PLAN_ID:
	int64Val = stats_->getExplainPlanId();
	break;

      case STAT_TDB_ID:
	int64Val = stat->getTdbId();
	break;

      case STAT_FRAG_NUM:
        int64Val = stat->getFragId();
        break;

      case STAT_INST_NUM:
	int64Val = stat->getInstNum();
	break;

      case STAT_SUB_INST_NUM:
	int64Val = stat->getSubInstNum();
	break;

      case STAT_LINE_NUM:
	// statistics have one line for now
	int64Val = 0;
	break;

      case STAT_PARENT_TDB_ID:
	int64Val = stat->getParentTdbId();
	if (int64Val < 0)
	  valIsNull = -1;
	break;

      case STAT_LC_TDB_ID:
	int64Val = stat->getLeftChildTdbId();
	if (int64Val < 0)
	  valIsNull = -1;
	break;

      case STAT_RC_TDB_ID:
	int64Val = stat->getRightChildTdbId();
	if (int64Val < 0)
	  valIsNull = -1;
	break;

      case STAT_SEQ_NUM:
	int64Val = stat->getExplainNodeId();
	if (int64Val < 0)
	  valIsNull = -1;
	break;

      case STAT_TDB_NAME:
	src = stat->getTdbName();
        srcLen = str_len(src);
        srcType = REC_BYTE_F_ASCII;
	break;

      case STAT_WORK_CALLS:
	int64Val = stat->getNumberCalls();
	break;

      case STAT_EST_ROWS:
	int64Val = (Int64)stat->getEstRowsUsed();
	break;

      case STAT_ACT_ROWS:
	int64Val = stat->getActualRowsReturned();
	break;

      case STAT_UP_Q_SZ:
	int64Val = stat->getUpQueueSize();
	if (int64Val <= 0)
	  valIsNull = -1;
	break;

      case STAT_DN_Q_SZ:
	int64Val = stat->getDownQueueSize();
	if (int64Val <= 0)
	  valIsNull = -1;
	break;

      case STAT_VAL4_TXT:
	valNum++; // fall through
      case STAT_VAL3_TXT:
	valNum++; // fall through
      case STAT_VAL2_TXT:
	valNum++; // fall through
      case STAT_VAL1_TXT:
	src = (char *) stat->getNumValTxt(valNum);
	if (src)
	  srcLen = str_len(src);
	else
	  valIsNull = -1;
	srcType = REC_BYTE_F_ASCII;
	break;

      case STAT_VAL4:
	valNum++; // fall through
      case STAT_VAL3:
	valNum++; // fall through
      case STAT_VAL2:
	valNum++; // fall through
      case STAT_VAL1:
	if (stat->getNumValTxt(valNum) == NULL)
	  valIsNull = -1;
	else
	  int64Val = stat->getNumVal(valNum);
        break;

      case STAT_TEXT:
	src = (char *) stat->getTextVal();
	if (src)
	  srcLen = str_len(src);
	else
	  valIsNull = -1;
	srcType = REC_BYTE_F_ASCII;
	break;

      case STAT_VARIABLE_INFO:
        {
          stat->getVariableStatsInfo(
	     &data_[attr->getOffset()],
	     &data_[attr->getVCLenIndOffset()],
	    attr->getLength());
          callConvDoit = FALSE;
        }
	break;
      default:
	ex_assert(0, "bad case index in ExStatsTcb::work");
	break;

      } // switch i

    // set NULL indicator of target field
    if (attr->getNullFlag())
      {
	// target is nullable
	if (attr->getNullIndicatorLength() == 2)
	  {
	    // set the 2 byte NULL indicator to -1
	    *(short *) (&data_[attr->getNullIndOffset()]) =
	      valIsNull;
	  }
	else
	  {
	    ex_assert(attr->getNullIndicatorLength() == 4,
		      "NULL indicator must be 2 or 4 bytes");
	    *(Lng32 *) (&data_[attr->getNullIndOffset()]) =
	      valIsNull;
	  }
      }
    else
      ex_assert(!valIsNull,
		"NULL source for NOT NULL stats column");

    if (!valIsNull)
      {
	if (callConvDoit && 
	    ::convDoIt(src, srcLen, srcType, 0, 0,
		       &data_[attr->getOffset()], 
		       attr->getLength(),
		       attr->getDatatype(),0,0,
		       0, 0, NULL) != ex_expr::EXPR_OK)
	  {
	    ex_assert(
		 0,
		 "Error from ExStatsTcb::work::convDoIt.");
	  }
      }
  } // for i
}


ExStatisticsArea *ExStatsTcb::sendToSsmp()
{
  ExStatisticsArea *stats = NULL;
  CliGlobals *cliGlobals = getGlobals()->castToExExeStmtGlobals()->
	                castToExMasterStmtGlobals()->getCliGlobals();
  if (cliGlobals->getStatsGlobals() == NULL)
  {
    // Runtime Stats not running.
    // Fill in diagsArea with details and return NULL
    IpcAllocateDiagsArea(diagsArea_, getHeap());
    (*diagsArea_) << DgSqlCode(-EXE_RTS_NOT_STARTED);
    return NULL; 
  }

  // Verify that we have valid parameters for each kind of request. If not, fill in the diagsArea
  // and return NULL.
  switch (reqType_)
  {
    case SQLCLI_STATS_REQ_QID:
    case SQLCLI_STATS_REQ_QID_DETAIL:
      if (nodeName_[0] == '\0' || cpu_ == -1)
      {
        // Invalid queryID. We were unable to extract the NodeNumber and/or CPU from it.
        IpcAllocateDiagsArea(diagsArea_, getHeap());
        (*diagsArea_) << DgSqlCode(-EXE_RTS_INVALID_QID) << DgString0(qid_);
        return NULL;
      }
      break;
    case SQLCLI_STATS_REQ_CPU:
      if (cpu_ == -1)
      {
        //Invalid CPU number. We will not be able to retrieve runtime stats for it.
        IpcAllocateDiagsArea(diagsArea_, getHeap());
        (*diagsArea_) << DgSqlCode(-EXE_RTS_INVALID_CPU_PID);
        return NULL;
      }
      break;
    case SQLCLI_STATS_REQ_PID:
    case SQLCLI_STATS_REQ_PROCESS_INFO:
      if (cpu_ == -1 || pid_ == -1)
      {
        //Invalid CPU number or PID number. We will not be able to retrieve runtime stats for it.
        IpcAllocateDiagsArea(diagsArea_, getHeap());
        (*diagsArea_) << DgSqlCode(-EXE_RTS_INVALID_CPU_PID);
        return NULL;
      }
      break;
    case SQLCLI_STATS_REQ_CPU_OFFENDER:
    case SQLCLI_STATS_REQ_MEM_OFFENDER:
    case SQLCLI_STATS_REQ_SE_OFFENDER:
    case SQLCLI_STATS_REQ_RMS_INFO:
    case SQLCLI_STATS_REQ_ET_OFFENDER:
      break;
    case SQLCLI_STATS_REQ_QID_INTERNAL:
      if (cpu_ == -1 || pid_ == -1 || timeStamp_ == -1 || queryNumber_ == -1 )
      {
         // Invalid internal QID we will not be able to retrieve runtime stats for it.
         IpcAllocateDiagsArea(diagsArea_, getHeap());
         (*diagsArea_) << DgSqlCode(-EXE_RTS_INVALID_QID_INTERNAL);
         return NULL;
      }
      break;
    default:
      // Invalid queryID. We were unable to extract the NodeNumber and/or CPU from it.
      IpcAllocateDiagsArea(diagsArea_, getHeap());
      (*diagsArea_) << DgSqlCode(-EXE_RTS_INVALID_QID) << DgString0(inputStmtName_);
      return NULL;
  }

  ExSsmpManager *ssmpManager = cliGlobals->getSsmpManager();
  short cpu;
  if (cpu_ == -1)
    cpu = cliGlobals->myCpu();
  else
    cpu = cpu_;
  IpcServer *ssmpServer = ssmpManager->getSsmpServer((NAHeap *)getHeap(), nodeName_, cpu, diagsArea_);
  if (ssmpServer == NULL)
    return NULL; // diags are in diagsArea_

  //Create the SsmpClientMsgStream on the IpcHeap, since we don't dispose of it immediately.
  //We just add it to the list of completed messages in the IpcEnv, and it is disposed of later.
  //If we create it on the ExStatsTcb's heap, that heap gets deallocated when the statement is 
  //finished, and we can corrupt some other statement's heap later on when we deallocate this stream.

  SsmpClientMsgStream *ssmpMsgStream  = new (cliGlobals->getIpcHeap())
        SsmpClientMsgStream((NAHeap *)cliGlobals->getIpcHeap(), ssmpManager);

  ssmpMsgStream->addRecipient(ssmpServer->getControlConnection());
  RtsHandle rtsHandle = (RtsHandle) this;
  RtsStatsReq *statsReq = NULL;
  RtsCpuStatsReq *cpuStatsReq = NULL;
  RtsQueryId *rtsQueryId = NULL;

  // Retrieve the Rts collection interval and active queries. If they are valid, calculate the timeout
  // and send to the SSMP process.
  SessionDefaults *sd = cliGlobals->currContext()->getSessionDefaults();
  Lng32 RtsTimeout;
  NABoolean wmsProcess;
  if (sd)
  {
    RtsTimeout = sd->getRtsTimeout();
    wmsProcess = sd->getWmsProcess();
  }
  else
  {
    RtsTimeout = 0;
    wmsProcess = FALSE;
  }

  if (reqType_ == SQLCLI_STATS_REQ_CPU_OFFENDER || 
      reqType_ == SQLCLI_STATS_REQ_MEM_OFFENDER || 
      reqType_ == SQLCLI_STATS_REQ_SE_OFFENDER || 
      reqType_ == SQLCLI_STATS_REQ_ET_OFFENDER || 
      reqType_ == SQLCLI_STATS_REQ_RMS_INFO)
  {
    cpuStatsReq = new (cliGlobals->getIpcHeap())RtsCpuStatsReq(rtsHandle, cliGlobals->getIpcHeap(),
        nodeName_, cpu_, activeQueryNum_, reqType_);
    cpuStatsReq->setSubReqType(subReqType_);
    cpuStatsReq->setFilter(filter_);
    *ssmpMsgStream << *cpuStatsReq;
  }
  else
  {
    statsReq = new (cliGlobals->getIpcHeap()) RtsStatsReq(rtsHandle, cliGlobals->getIpcHeap(), wmsProcess);
    *ssmpMsgStream << *statsReq;
    switch (reqType_)
    {
      case SQLCLI_STATS_REQ_QID:
      case SQLCLI_STATS_REQ_QID_DETAIL:
        rtsQueryId = new (cliGlobals->getIpcHeap()) RtsQueryId(cliGlobals->getIpcHeap(), qid_, (Lng32)str_len(qid_), 
            statsMergeType_, activeQueryNum_, reqType_, detailLevel_);
        break;
      case SQLCLI_STATS_REQ_CPU:
        rtsQueryId = new (cliGlobals->getIpcHeap()) RtsQueryId(cliGlobals->getIpcHeap(), nodeName_,  cpu_, 
          statsMergeType_, activeQueryNum_);
        break;
      case SQLCLI_STATS_REQ_PID:
      case SQLCLI_STATS_REQ_PROCESS_INFO:
        rtsQueryId = new (cliGlobals->getIpcHeap()) RtsQueryId(cliGlobals->getIpcHeap(), nodeName_,  cpu_, pid_, 
          statsMergeType_, activeQueryNum_, reqType_);
        rtsQueryId->setSubReqType(subReqType_);
        break;
      case SQLCLI_STATS_REQ_QID_INTERNAL:
        rtsQueryId = new (cliGlobals->getIpcHeap()) RtsQueryId(cliGlobals->getIpcHeap(), nodeName_, cpu_, pid_,
          timeStamp_, queryNumber_, statsMergeType_, activeQueryNum_);
        break;
      default:
        rtsQueryId = NULL;
        break;
    }
    if (NULL != rtsQueryId)
      *ssmpMsgStream << *rtsQueryId;
  }
  if (RtsTimeout != 0)
  {
    // We have a valid value for the timeout, so we use it by converting it to centiseconds.
    RtsTimeout = RtsTimeout * 100;
  }
  else
    //Use the default value of 4 seconds, or 400 centiseconds.
    RtsTimeout = 400;
  
  // Send the message
  ssmpMsgStream->send(FALSE, -1); 
  Int64 startTime = NA_JulianTimestamp();
  Int64 currTime;
  Int64 elapsedTime;
  IpcTimeout timeout = (IpcTimeout) RtsTimeout;
  while (timeout > 0 && ssmpMsgStream->hasIOPending())
  {
    ssmpMsgStream->waitOnMsgStream(timeout);
    currTime = NA_JulianTimestamp();
    elapsedTime = (Int64)(currTime - startTime) / 10000;
    timeout = (IpcTimeout)(RtsTimeout - elapsedTime);
  }
  // Callbacks would have placed broken connections into 
  // ExSsmpManager::deletedSsmps_.  Delete them now.
  ssmpManager->cleanupDeletedSsmpServers();
  if (ssmpMsgStream->getState() == IpcMessageStream::ERROR_STATE && retryAttempts_ < 3) 
  {
    switch (reqType_)
    {
        case SQLCLI_STATS_REQ_CPU_OFFENDER:
        case SQLCLI_STATS_REQ_MEM_OFFENDER:
        case SQLCLI_STATS_REQ_SE_OFFENDER:
        case SQLCLI_STATS_REQ_RMS_INFO:
        case SQLCLI_STATS_REQ_ET_OFFENDER:
           cpuStatsReq->decrRefCount();
           break;
        default:
           rtsQueryId->decrRefCount();
           statsReq->decrRefCount();
           break;
    }
    DELAY(100);
    retryAttempts_++;
    stats = sendToSsmp();
    retryAttempts_ = 0;
    return stats;
  }    
  if (ssmpMsgStream->getState() == IpcMessageStream::BREAK_RECEIVED)
  {     
    // Break received - set diags area
    IpcAllocateDiagsArea(diagsArea_, getHeap());
    (*diagsArea_) << DgSqlCode(-EXE_CANCELED);
    
    return NULL;
  }
  if (! ssmpMsgStream->isReplyReceived())
  {
    IpcAllocateDiagsArea(diagsArea_, getHeap());
    (*diagsArea_) << DgSqlCode(-EXE_RTS_TIMED_OUT) 
      << DgString0(inputStmtName_) << DgInt0(RtsTimeout/100) ;
    return NULL;
  }
  retryAttempts_ = 0;
  if (ssmpMsgStream->getRtsQueryId() != NULL)
  {
    rtsQueryId->decrRefCount();
    statsReq->decrRefCount();
    if (inputStmtName_)
      NADELETEBASIC(inputStmtName_, getHeap());
    inputStmtName_ = new(getHeap()) 
        char[ssmpMsgStream->getRtsQueryId()->getQueryIdLen()+1+4]; // 4 for QID=
                                                                                       
    str_cat("QID=", ssmpMsgStream->getRtsQueryId()->getQueryId(), inputStmtName_);
    Lng32 retcode = parse_stmt_name(inputStmtName_, str_len(inputStmtName_));
    ssmpMsgStream->getRtsQueryId()->decrRefCount();
    stats = sendToSsmp();
    return stats;
  }
  stats = ssmpMsgStream->getStats();
  switch (reqType_)
  {
     case SQLCLI_STATS_REQ_CPU_OFFENDER:
     case SQLCLI_STATS_REQ_MEM_OFFENDER:
     case SQLCLI_STATS_REQ_SE_OFFENDER:
     case SQLCLI_STATS_REQ_RMS_INFO:
     case SQLCLI_STATS_REQ_ET_OFFENDER:
        cpuStatsReq->decrRefCount();
        break;
     default:
        rtsQueryId->decrRefCount();
        statsReq->decrRefCount();
        break;
  }
  if (ssmpMsgStream->getNumSscpReqFailed() > 0)
  {
    IpcAllocateDiagsArea(diagsArea_, getHeap());
    (*diagsArea_) << DgSqlCode(EXE_RTS_REQ_PARTIALY_SATISFIED) 
        << DgInt0(ssmpMsgStream->getNumSscpReqFailed());
    if (stats != NULL && stats->getMasterStats() != NULL)
        stats->getMasterStats()->setStatsErrorCode(EXE_RTS_REQ_PARTIALY_SATISFIED);
  }
  return stats;
}


Lng32 ExStatsTcb::parse_stmt_name(char *string, Lng32 len)
{
  short idOffset;
  short idLen;

  reqType_ = (short)str_parse_stmt_name(string, len, nodeName_, &cpu_, 
    &pid_, &timeStamp_, &queryNumber_, &idOffset, &idLen, &activeQueryNum_, 
    &statsMergeType_, &detailLevel_, &subReqType_, &filter_);
  if (reqType_ == SQLCLI_STATS_REQ_QID || reqType_ == SQLCLI_STATS_REQ_QID_DETAIL)
  {
    qid_ = new (getHeap()) char[idLen+1]; 
    str_cpy_all(qid_, string+idOffset, idLen);
    qid_[idLen] ='\0';
    if (strncasecmp(qid_, "CURRENT", 7) == 0)
      reqType_ = SQLCLI_STATS_REQ_QID_CURRENT;
    else
    {
      if (getMasterCpu(qid_, (Lng32)str_len(qid_), nodeName_, MAX_SEGMENT_NAME_LEN+1, cpu_) == -1)
      {
        nodeName_[0] = '\0';
        cpu_ = -1;
      }
    }
  }
  if (reqType_ == SQLCLI_STATS_REQ_STMT)
  {
    stmtName_ = new (getHeap()) char[idLen+1]; 
    str_cpy_all(stmtName_, string+idOffset, idLen);
    stmtName_[idLen] ='\0';
  }
  if (reqType_ == SQLCLI_STATS_REQ_QID_INTERNAL)
     nodeName_[0] = '\0';
  return reqType_;
}

ExStatsPrivateState::ExStatsPrivateState()
{
  matchCount_        = 0;
  step_              = ExStatsTcb::INITIAL_;
}

ExStatsPrivateState::~ExStatsPrivateState()
{
}

ex_tcb_private_state * ExStatsPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ExStatsPrivateState();
};


void ExMasterStats::init(NABoolean resetDop)
{
  elapsedStartTime_ = -1;
  elapsedEndTime_ = -1;
  firstRowReturnTime_ = -1;
  compStartTime_ = -1;
  compEndTime_ = -1;
  exeStartTime_ = -1;
  exeEndTime_ = -1;
  canceledTime_ = -1;
  fixupStartTime_ = -1;
  fixupEndTime_ = -1;
  freeupStartTime_ = -1;
  freeupEndTime_ = -1;
  returnedRowsIOTime_ = -1;
  rowsAffected_ = -1;
  rowsReturned_ = 0;
  sqlErrorCode_ = 0;
  numOfTotalEspsUsed_ = 0;
  numOfNewEspsStarted_ = -1;
  numOfRootEsps_ = -1;
  exePriority_ = -1;
  espPriority_ = -1;
  cmpPriority_ = -1;
  dp2Priority_ = -1;
  fixupPriority_ = -1;
  queryType_ = SQL_OTHER;
  subqueryType_ = SQL_STMT_NA;
  statsErrorCode_ = 0;
  stmtState_ = Statement::INITIAL_;
  numCpus_ = 0;
  masterFlags_ = 0;
  parentQid_ = NULL;
  parentQidLen_ = 0;
  parentQidSystem_[0] = '\0';
  transId_ = -1;
  childQid_ = NULL;
  childQidLen_ = 0;
  isQuerySuspended_ = false;
  querySuspendedTime_ = -1;
  cancelComment_ = NULL;
  cancelCommentLen_ = 0;
  suspendMayHaveAuditPinned_ = false;
  suspendMayHoldLock_ = false;
  readyToSuspend_ = NOT_READY;
  validPrivs_ = false;
  numSIKeys_ = 0;
  if ((sIKeys_ != NULL) && (sIKeys_ != &preallocdSiKeys_[0]))
    NADELETEBASIC(sIKeys_, getHeap());
  sIKeys_ = &preallocdSiKeys_[0];
  memset(sIKeys_, 0, PreAllocatedSikKeys * sizeof(SQL_QIKEY));
  validDDL_ = false;
  numObjUIDs_ = 0;
  if ((objUIDs_ != NULL) && (objUIDs_ != &preallocdObjUIDs_[0]))
    NADELETEBASIC(objUIDs_, getHeap());
  objUIDs_ = &preallocdObjUIDs_[0];
  memset(objUIDs_, 0, PreAllocatedObjUIDs * sizeof(Int64));
  isBlocking_ = false;
  lastActivity_ = 0;
  blockOrUnblockSince_.tv_sec = blockOrUnblockSince_.tv_usec = 0;
}

void ExMasterStats::reuse()
{
  init(FALSE);
}

void ExMasterStats::initBeforeExecute(Int64 currentTimeStamp)
{
  exeEndTime_ = -1;
  canceledTime_ = -1;
  fixupStartTime_ = currentTimeStamp ;
  fixupEndTime_ = -1;
  freeupStartTime_ = currentTimeStamp;
  freeupEndTime_ = -1;
  returnedRowsIOTime_ = -1;
  rowsAffected_ = -1;
  rowsReturned_ = 0;
  sqlErrorCode_ = 0;
  numOfTotalEspsUsed_ = 0;
  numOfNewEspsStarted_ = -1;
  numOfRootEsps_ = -1;
  numCpus_ = 0;
  transId_ = -1;
  childQid_ = NULL;
  childQidLen_ = 0;
  isQuerySuspended_ = false;
  querySuspendedTime_ = -1;
  if (cancelComment_ != NULL)
    cancelComment_[0] = '\0';
  cancelCommentLen_ = 0;
}

void ExMasterStats::resetAqrInfo()
{
  aqrLastErrorCode_= 0;
  numAqrRetries_= 0;
  delayBeforeAqrRetry_= 0;
}

ExMasterStats::ExMasterStats(NAHeap *heap)
  : ExOperStats(heap, MASTER_STATS)
{
  sourceStr_ = NULL;
  storedSqlTextLen_ = 0;
  originalSqlTextLen_ = 0;
  queryId_ = NULL;
  queryIdLen_ = 0;
  reclaimSpaceCount_ = 0;
  aqrLastErrorCode_=0;
  numAqrRetries_=0;
  delayBeforeAqrRetry_=0;
  sIKeys_ = NULL;
  objUIDs_ = NULL;
  init(FALSE);
}

ExMasterStats::ExMasterStats()
  : ExOperStats()
{
  sourceStr_ = NULL;
  storedSqlTextLen_ = 0;
  originalSqlTextLen_ = 0;
  queryId_ = NULL;
  queryIdLen_ = 0;
  reclaimSpaceCount_ = 0;
  aqrLastErrorCode_=0;
  numAqrRetries_=0;
  delayBeforeAqrRetry_=0;
  sIKeys_ = NULL;
  objUIDs_ = NULL;
  init(FALSE);
}

ExMasterStats::ExMasterStats(NAHeap *heap, char *sourceStr, Lng32 storedSqlTextLen, Lng32 originalSqlTextLen, 
                        char *queryId, Lng32 queryIdLen)
  : ExOperStats(heap, MASTER_STATS)
{
  if (queryId != NULL)
  {
    queryId_ = new (heap_) char[queryIdLen+1];
    str_cpy_all(queryId_, queryId, queryIdLen);
    queryId_[queryIdLen] = '\0';
    queryIdLen_ = queryIdLen;
  }
  else
  {
    queryId_ = NULL;
    queryIdLen_ = 0;
  }
  
 originalSqlTextLen_ =originalSqlTextLen;
  if (storedSqlTextLen != 0)
  {
    sourceStr_ = new (heap_) char[storedSqlTextLen+2];
    str_cpy_all(sourceStr_, sourceStr, storedSqlTextLen);
    sourceStr_[storedSqlTextLen] = '\0';
    sourceStr_[storedSqlTextLen+1] = '\0';
    storedSqlTextLen_ = storedSqlTextLen;
  }
  else
  {
    sourceStr_ = NULL;
    storedSqlTextLen_ = 0;
  }
  reclaimSpaceCount_ = 0;
  aqrLastErrorCode_=0;
  numAqrRetries_=0;
  delayBeforeAqrRetry_=0;
  sIKeys_ = NULL;
  objUIDs_ = NULL;
  init(FALSE);
}

ExMasterStats::~ExMasterStats()
{
  deleteMe();
}

void ExMasterStats::deleteMe()
{
  if (queryId_ != NULL)
  {
    NADELETEBASIC(queryId_, getHeap());
    queryId_ = NULL;
  }
  if (sourceStr_ != NULL)
  {
    NADELETEBASIC(sourceStr_, getHeap());
    sourceStr_ = NULL;
  }
  if (parentQid_ != NULL)
  {
    NADELETEBASIC(parentQid_, getHeap());
    parentQid_ = NULL;
  }
  if (childQid_ != NULL)
  {
    NADELETEBASIC(childQid_, getHeap());
    childQid_ = NULL;
  }
  if (cancelComment_ != NULL)
  {
    NADELETEBASIC(cancelComment_, getHeap());
    cancelComment_ = NULL;
  }
  if ((sIKeys_ != NULL) && (sIKeys_ != &preallocdSiKeys_[0]))
  {
    NADELETEBASIC(sIKeys_, getHeap());
    sIKeys_ = NULL;
    numSIKeys_ = 0;
  }
  if ((objUIDs_ != NULL) && (objUIDs_ != &preallocdObjUIDs_[0]))
  {
    NADELETEBASIC(objUIDs_, getHeap());
    objUIDs_ = NULL;
    numObjUIDs_ = 0;
  }
}

UInt32 ExMasterStats::packedLength()
{
  UInt32 size;
  size = sizeof(ExMasterStats)-sizeof(ExOperStats);
  size += queryIdLen_;
  size += storedSqlTextLen_;
  size += parentQidLen_;
  size += childQidLen_;
  size += cancelCommentLen_;
  // the SQL_QIKEY array is not packed or unpacked.
  return size;
}

void ExMasterStats::unpack(const char* &buffer)
{
  UInt32 srcLen;
  srcLen = sizeof(ExMasterStats)-sizeof(ExOperStats);
  char * srcPtr = (char *)this+sizeof(ExOperStats);
  memcpy((void *)srcPtr, buffer, srcLen);
  buffer += srcLen;
  if (queryIdLen_ != 0)
  {
    queryId_ = new ((NAHeap *)(getHeap())) char[queryIdLen_+1];
    unpackStrFromBuffer(buffer, queryId_, queryIdLen_);
    queryId_[queryIdLen_] = '\0';
  }
  else
    queryId_ = NULL;
  if (storedSqlTextLen_ != 0)
  {
    sourceStr_ = new ((NAHeap *)(getHeap())) char[storedSqlTextLen_+2];
    unpackStrFromBuffer(buffer, sourceStr_, storedSqlTextLen_);
    sourceStr_[storedSqlTextLen_] = '\0';
    sourceStr_[storedSqlTextLen_+1] = '\0';
  }
  else
    sourceStr_ = NULL;
  if (parentQidLen_ != 0)
  {
    parentQid_ = new ((NAHeap *)(getHeap())) char[parentQidLen_+1];
    unpackStrFromBuffer(buffer, parentQid_, parentQidLen_);
    parentQid_[parentQidLen_] = '\0';
  }
  else
    parentQid_ = NULL;
  if (childQidLen_ != 0)
  {
    childQid_ = new ((NAHeap *)(getHeap())) char[childQidLen_+1];
    unpackStrFromBuffer(buffer, childQid_, childQidLen_);
    childQid_[childQidLen_] = '\0';
  }
  else
    childQid_ = NULL;
  if (cancelCommentLen_ != 0)
  {
    cancelComment_ = new ((NAHeap *)(getHeap())) char[cancelCommentLen_+1];
    unpackStrFromBuffer(buffer, cancelComment_, cancelCommentLen_);
    cancelComment_[cancelCommentLen_] = '\0';
  }
  else
    cancelComment_ = NULL;
  // SQL_QIKEY array was not packed, so make sure the local copy's values are
  // consistent.  We don't pack them because they aren't reported in GET
  // STATISTICS and so they don't need to leave the local node.  Same issue
  // for objectUIDs_.
  numSIKeys_ = 0;
  sIKeys_ = NULL;
  numObjUIDs_ = 0;
  objUIDs_ = NULL;
}

UInt32 ExMasterStats::pack(char* buffer)
{
  UInt32 size;
  UInt32 srcLen = sizeof(ExMasterStats)-sizeof(ExOperStats);
  char * srcPtr = (char *)this+sizeof(ExOperStats);
  memcpy(buffer, (void *)srcPtr, srcLen);
  size = srcLen;
  buffer += size;
  if (queryIdLen_ != 0 && queryId_ != NULL)
    size += packStrIntoBuffer(buffer, queryId_, queryIdLen_);
 if (storedSqlTextLen_ != 0 && sourceStr_ != NULL)
    size += packStrIntoBuffer(buffer, sourceStr_, storedSqlTextLen_);
  if (parentQidLen_ != 0 && parentQid_ != NULL)
    size += packStrIntoBuffer(buffer, parentQid_, parentQidLen_);
  if (childQidLen_ != 0 && childQid_ != NULL)
    size += packStrIntoBuffer(buffer, childQid_, childQidLen_);
  if (cancelCommentLen_ != 0 && cancelComment_ != NULL)
    size += packStrIntoBuffer(buffer, cancelComment_, cancelCommentLen_);
  // Don't pack the SQL_QIKEY array.  See comment and compensating code 
  // in this class' unpack method.
  return size;
}

void ExMasterStats::getVariableStatsInfo(char * dataBuffer,
			  char * dataLen,
          		  Lng32 maxLen)
{
  char *buf = dataBuffer;
  
  Int64 exeElapsedTime;
  Int64 compElapsedTime;

  if (compStartTime_ == -1)
      compElapsedTime = 0;
  else
  if (compEndTime_ == -1)
  {
    compElapsedTime = NA_JulianTimestamp() - compStartTime_;
    if (compElapsedTime < 0)
      compElapsedTime = 0;
  }
  else
    compElapsedTime = compEndTime_ - compStartTime_;
  if (exeStartTime_ == -1)
      exeElapsedTime = 0;
  else
  if (exeEndTime_ == -1)
  {
    exeElapsedTime = NA_JulianTimestamp() - exeStartTime_;
    if (exeElapsedTime < 0) // This could happen to time insync between segments
      exeElapsedTime = 0;
  }
  else
    exeElapsedTime = exeEndTime_ - exeStartTime_;
  short stmtState = stmtState_;

  if (isQuerySuspended_)
    stmtState = Statement::SUSPENDED_;
  if ((Int32)getCollectStatsType() == SQLCLI_ET_OFFENDER_STATS)
  {
     sprintf(buf,"statsRowType: %d Qid: %s CompStartTime: %ld "
    "CompEndTime: %ld "
    "ExeStartTime: %ld ExeEndTime: %ld CanceledTime: %ld RowsAffected: %ld "
    "SqlErrorCode: %d StatsErrorCode: %d State: %s StatsType: %s queryType: %s "
    "subqueryType: %s EstRowsAccessed: %f EstRowsUsed: %f compElapsedTime: %ld "
    "exeElapsedTime: %ld parentQid: %s parentQidSystem: %s childQid: %s "
    "rowsReturned: %ld firstRowReturnTime: %ld numSqlProcs: %d  numCpus: %d "
    "exePriority: %d transId: %ld suspended: %s lastSuspendTime: %ld "
    "LastErrorBeforeAQR: %d AQRNumRetries: %d DelayBeforeAQR: %d "
    "reclaimSpaceCnt: %d "
    "blockedInSQL: %d blockedInClient: %d  lastActivity: %d "
                  "sqlSrcLen: %d sqlSrc: \"%s\"",
              statType(),
              ((queryId_ != NULL) ? queryId_ : "NULL"),
              compStartTime_,
              compEndTime_,
              exeStartTime_,
              exeEndTime_,
              canceledTime_,
	      rowsAffected_,
              sqlErrorCode_,
              statsErrorCode_,
              Statement::stmtState((Statement::State)stmtState), 
              ExStatisticsArea::getStatsTypeText((Int32)compilerStatsInfo_.collectStatsType()),
              ComTdbRoot::getQueryTypeText(queryType_), 
              ComTdbRoot::getSubqueryTypeText(subqueryType_), 
              compilerStatsInfo_.dp2RowsAccessed(),
              compilerStatsInfo_.dp2RowsUsed(),
              compElapsedTime,
              exeElapsedTime,
              ((parentQid_ != NULL) ? parentQid_ : "NONE"),
              ((parentQid_ != NULL) ? (parentQidSystem_[0] != '\0' ? 
                                  parentQidSystem_ : "SAME") : "NONE"),
              ((childQid_ != NULL) ? childQid_ : "NONE"),
              rowsReturned_,
              firstRowReturnTime_,
              getNumSqlProcs(),
              numCpus_,
              exePriority_,
              transId_,
              (isQuerySuspended_ ? "yes" : "no" ),
              querySuspendedTime_,
              aqrLastErrorCode_,
              numAqrRetries_,
              delayBeforeAqrRetry_,
              reclaimSpaceCount_,
              timeSinceBlocking(0),
              timeSinceUnblocking(0),
              lastActivity_,
              originalSqlTextLen_,
              ((sourceStr_ != NULL) ? sourceStr_ : ""));
   }
   else
   {
     sprintf(buf,"statsRowType: %d Qid: %s CompStartTime: %ld "
    "CompEndTime: %ld "
    "ExeStartTime: %ld ExeEndTime: %ld CanceledTime: %ld RowsAffected: %ld "
    "SqlErrorCode: %d StatsErrorCode: %d State: %d StatsType: %d queryType: %d "
    "subqueryType: %d EstRowsAccessed: %f EstRowsUsed: %f compElapsedTime: %ld "
    "exeElapsedTime: %ld parentQid: %s parentQidSystem: %s childQid: %s "
    "rowsReturned: %ld firstRowReturnTime: %ld numSqlProcs: %d  numCpus: %d "
    "exePriority: %d transId: %ld suspended: %s lastSuspendTime: %ld "
    "LastErrorBeforeAQR: %d AQRNumRetries: %d DelayBeforeAQR: %d reclaimSpaceCnt: %d "
    "blockedInSQL: %d blockedInClient: %d  lastActivity: %d "
    "exeCount: %u, exeTimeMin: %ld exeTimeMax: %ld exeTimeAvg: %f "
                  "sqlSrcLen: %d sqlSrc: \"%s\"",
              statType(),
              ((queryId_ != NULL) ? queryId_ : "NULL"),
              compStartTime_,
              compEndTime_,
              exeStartTime_,
              exeEndTime_,
              canceledTime_,
	      rowsAffected_,
              sqlErrorCode_,
              statsErrorCode_,
              stmtState,
              compilerStatsInfo_.collectStatsType(),
              queryType_,
              subqueryType_,
              compilerStatsInfo_.dp2RowsAccessed(),
              compilerStatsInfo_.dp2RowsUsed(),
              compElapsedTime,
              exeElapsedTime,
              ((parentQid_ != NULL) ? parentQid_ : "NONE"),
              ((parentQid_ != NULL) ? (parentQidSystem_[0] != '\0' ? 
                                  parentQidSystem_ : "SAME") : "NONE"),
              ((childQid_ != NULL) ? childQid_ : "NONE"),
              rowsReturned_,
              firstRowReturnTime_,
              getNumSqlProcs(),
              numCpus_,
              exePriority_,
              transId_,
              (isQuerySuspended_ ? "yes" : "no" ),
              querySuspendedTime_,
              aqrLastErrorCode_,
              numAqrRetries_,
              delayBeforeAqrRetry_,
              reclaimSpaceCount_,
              timeSinceBlocking(0),
              timeSinceUnblocking(0),
              lastActivity_,
              exeTimes_.entryCnt(),
              exeTimes_.min(),
              exeTimes_.max(),
              exeTimes_.mean(),  
              originalSqlTextLen_,
              ((sourceStr_ != NULL) ? sourceStr_ : ""));
  }
  buf += str_len(buf);
  *(short *)dataLen = (short) (buf - dataBuffer);
}

ExOperStats *ExMasterStats::copyOper(NAMemory * heap)
{
  ExMasterStats * stat = new(heap) ExMasterStats((NAHeap *)heap);
  stat->copyContents(this);
  return stat;

}

ExMasterStats *ExMasterStats::castToExMasterStats()
{
  return this;
}

void ExMasterStats::copyContents(ExMasterStats * other)
{
  ExOperStats::copyContents(other);
  if (queryId_ != NULL)
  {
    NADELETEBASIC(queryId_, getHeap());
  }
  if (sourceStr_ != NULL)
  {
      NADELETEBASIC(queryId_, getHeap());
  }
  if (parentQid_ != NULL)
  {
    NADELETEBASIC(parentQid_, getHeap());
  }
  if (childQid_ != NULL)
  {
    NADELETEBASIC(childQid_, getHeap());
  }
  if (cancelComment_ != NULL)
  {
    NADELETEBASIC(cancelComment_, getHeap());
  }
  char * srcPtr = (char *)other+sizeof(ExOperStats);
  char * destPtr = (char *)this+sizeof(ExOperStats);
  UInt32 srcLen = sizeof(ExMasterStats)-sizeof(ExOperStats);
  memcpy((void *)destPtr, (void *)srcPtr, srcLen);
  if (other->queryId_ != NULL)
  {
    queryId_ = new ((NAHeap *) getHeap()) char[other->queryIdLen_+1];
    str_cpy_all(queryId_, other->queryId_, other->queryIdLen_);
    queryId_[other->queryIdLen_] = '\0';
    queryIdLen_ = other->queryIdLen_;
  }
  else
  {
    queryId_ = NULL;
    queryIdLen_ = 0;
  }
  if (other->sourceStr_ != NULL)
  {
    sourceStr_ = new ((NAHeap *) getHeap()) char[other->storedSqlTextLen_+2];
    str_cpy_all(sourceStr_, other->sourceStr_, other->storedSqlTextLen_);
    sourceStr_[other->storedSqlTextLen_] = '\0';
    sourceStr_[other->storedSqlTextLen_+1] = '\0';
    storedSqlTextLen_ = other->storedSqlTextLen_;
    originalSqlTextLen_ = other->originalSqlTextLen_;
  }
  else
  {
    sourceStr_ = NULL;
    storedSqlTextLen_ = 0;
    originalSqlTextLen_ = 0;
  }
  if (other->parentQid_ != NULL)
  {
    parentQid_ = new ((NAHeap *) getHeap()) char[other->parentQidLen_+1];
    str_cpy_all(parentQid_, other->parentQid_, other->parentQidLen_);
    parentQid_[other->parentQidLen_] = '\0';
    parentQidLen_ = other->parentQidLen_;
  }
  else
  {
    parentQid_ = NULL;
    parentQidLen_ = 0;
  }
  if (other->childQid_ != NULL)
  {
    childQid_ = new ((NAHeap *) getHeap()) char[other->childQidLen_+1];
    str_cpy_all(childQid_, other->childQid_, other->childQidLen_);
    childQid_[other->childQidLen_] = '\0';
    childQidLen_ = other->childQidLen_;
  }
  else
  {
    childQid_ = NULL;
    childQidLen_ = 0;
  }
  if (other->cancelComment_ != NULL)
  {
    cancelComment_ = 
       new ((NAHeap *) getHeap()) char[other->cancelCommentLen_+1];
    str_cpy_all(cancelComment_, other->cancelComment_, 
                               other->cancelCommentLen_);
    cancelComment_[other->cancelCommentLen_] = '\0';
    cancelCommentLen_ = other->cancelCommentLen_;
  }
  else
  {
    cancelComment_ = NULL;
    cancelCommentLen_ = 0;
  }
  isBlocking_ = other->isBlocking_;
  blockOrUnblockSince_ = other->blockOrUnblockSince_;
  sIKeys_ = NULL;
  numSIKeys_ = 0;
  objUIDs_ = NULL;
  numObjUIDs_ = 0;
}

void ExMasterStats::setEndTimes(NABoolean updateExeEndTime)
{
  if (compStartTime_ != -1 && compEndTime_ == -1)
    compEndTime_ = NA_JulianTimestamp();
  if (updateExeEndTime && exeStartTime_ != -1 && exeEndTime_ == -1)
    exeEndTime_ = NA_JulianTimestamp();
}

void ExMasterStats::fixup(ExMasterStats *other)
{

  char *addrOfStatsVFTPtr, *myStatsVFTPtr;
  addrOfStatsVFTPtr = (char *)(this);
  myStatsVFTPtr = (char *)(other);
  *((Long  *)addrOfStatsVFTPtr) = *((Long *)myStatsVFTPtr);
}

Lng32 ExMasterStats::getStatsItem(SQLSTATS_ITEM* sqlStats_item)
{
  sqlStats_item->error_code = 0;
  switch (sqlStats_item->statsItem_id)
  {
  case SQLSTATS_QUERY_ID:
  case SQLSTATS_PARENT_QUERY_ID:
  case SQLSTATS_CHILD_QUERY_ID:
    char *queryId;
    Lng32 queryIdLen;
    if (sqlStats_item->statsItem_id == SQLSTATS_QUERY_ID)
    {
      queryId = queryId_;
      queryIdLen = queryIdLen_;
    }
    else if (sqlStats_item->statsItem_id == SQLSTATS_PARENT_QUERY_ID)
    {
      queryId = parentQid_;      
      queryIdLen = parentQidLen_;
    }
    else
    {
      queryId = childQid_;      
      queryIdLen = childQidLen_;
    }

    if (sqlStats_item->str_value != NULL)
    {
      if (queryId != NULL)
      {
        if (queryIdLen <= sqlStats_item->str_max_len)
        {
          str_cpy(sqlStats_item->str_value, queryId, queryIdLen);
          sqlStats_item->str_ret_len = queryIdLen;
        }
        else
          sqlStats_item->error_code = -EXE_ERROR_IN_STAT_ITEM;
      }
      else
      {
        if (sqlStats_item->str_max_len >= 4)
        {
          str_cpy(sqlStats_item->str_value, "NONE", 4);
          sqlStats_item->str_ret_len = 4;
        }
        else
          sqlStats_item->error_code = -EXE_ERROR_IN_STAT_ITEM;
      }
    }
    break;
  case SQLSTATS_SOURCE_STR:
    Lng32 sourceLen;
    if (sqlStats_item->str_value != NULL)
    {
      if (sourceStr_ != NULL)
      {
        if (storedSqlTextLen_ <= sqlStats_item->str_max_len)
          sourceLen = storedSqlTextLen_;
        else
          sourceLen = sqlStats_item->str_max_len;
        str_cpy(sqlStats_item->str_value, sourceStr_, sourceLen);
        sqlStats_item->str_ret_len = sourceLen;
      }
      else
      {
        *sqlStats_item->str_value = '\0';
        sqlStats_item->str_ret_len = 0;
      }
    }
    break;
  case SQLSTATS_PARENT_QUERY_SYSTEM:
    Lng32 systemNameLen;
    const char *systemName;
    if (sqlStats_item->str_value != NULL)
    {
      systemName = (parentQid_ != NULL ? (parentQidSystem_[0] != '\0' ? 
                   parentQidSystem_ : "SAME") : "NONE");
      systemNameLen = str_len(systemName);
      if (systemNameLen <= sqlStats_item->str_max_len)
      {
        str_cpy(sqlStats_item->str_value, systemName, systemNameLen);
        sqlStats_item->str_ret_len = systemNameLen;
      }
      else
        sqlStats_item->error_code = -EXE_ERROR_IN_STAT_ITEM;
    }
    break;
  case SQLSTATS_COMP_START_TIME:
    sqlStats_item->int64_value = compStartTime_;
    break;
  case SQLSTATS_COMP_END_TIME:
    sqlStats_item->int64_value = compEndTime_;
    break;
  case SQLSTATS_COMP_TIME:
    if (compStartTime_ == -1)
      sqlStats_item->int64_value = 0;
    else
    if (compEndTime_ == -1)
    {
      sqlStats_item->int64_value = NA_JulianTimestamp() - compStartTime_;
      if (sqlStats_item->int64_value < 0)
        sqlStats_item->int64_value = 0;
    }
    else
      sqlStats_item->int64_value = compEndTime_ - compStartTime_;
    break;
  case SQLSTATS_EXECUTE_START_TIME:
    sqlStats_item->int64_value = exeStartTime_;
    break;
  case SQLSTATS_FIRST_ROW_RET_TIME:
    sqlStats_item->int64_value = firstRowReturnTime_;
    break;
  case SQLSTATS_CANCEL_TIME_ID:
    sqlStats_item->int64_value = canceledTime_;
    break;
  case SQLSTATS_QUERY_SUSPENDED:
    sqlStats_item->int64_value = isQuerySuspended_;
    break;
  case SQLSTATS_SUSPEND_TIME_ID:
    sqlStats_item->int64_value = querySuspendedTime_;
    break;
  case SQLSTATS_EXECUTE_END_TIME:
    sqlStats_item->int64_value = exeEndTime_;
    break;
  case SQLSTATS_EXECUTE_TIME:
    if (exeStartTime_ == -1)
      sqlStats_item->int64_value = 0;
    else
    if (exeEndTime_ == -1)
    {
      sqlStats_item->int64_value = NA_JulianTimestamp() - exeStartTime_;
      if (sqlStats_item->int64_value < 0)
        sqlStats_item->int64_value = 0;
    }
    else
      sqlStats_item->int64_value = exeEndTime_ - exeStartTime_;
    break;
  case SQLSTATS_FIXUP_TIME:
    if (fixupStartTime_ == -1)
      sqlStats_item->int64_value = 0;
    else
    if (fixupEndTime_ == -1)
    {
      sqlStats_item->int64_value = NA_JulianTimestamp() - fixupStartTime_;
      if (sqlStats_item->int64_value < 0)
        sqlStats_item->int64_value = 0;
    }
    else
      sqlStats_item->int64_value = fixupEndTime_ - fixupStartTime_;
    break;
  case SQLSTATS_STMT_STATE:
    if (isQuerySuspended_)
      sqlStats_item->int64_value = Statement::SUSPENDED_;
    else
      sqlStats_item->int64_value = stmtState_;
    break;
  case SQLSTATS_ROWS_AFFECTED:
    sqlStats_item->int64_value = rowsAffected_;
    break;
  case SQLSTATS_ROWS_RETURNED:
    sqlStats_item->int64_value = rowsReturned_;
    break;
  case SQLSTATS_SQL_ERROR_CODE:
    sqlStats_item->int64_value = sqlErrorCode_;
    break;
  case SQLSTATS_STATS_ERROR_CODE:
    sqlStats_item->int64_value = statsErrorCode_;
    break;
  case SQLSTATS_QUERY_TYPE:
    sqlStats_item->int64_value = queryType_;
    break;
  case SQLSTATS_SUBQUERY_TYPE:
    sqlStats_item->int64_value = subqueryType_;
    break;
  case SQLSTATS_EST_ROWS_ACCESSED:
    sqlStats_item->double_value = compilerStatsInfo_.dp2RowsAccessed();
    break;
  case SQLSTATS_EST_ROWS_USED:
    sqlStats_item->double_value = compilerStatsInfo_.dp2RowsUsed();
    break;
  case SQLSTATS_SOURCE_STR_LEN:
    sqlStats_item->int64_value = originalSqlTextLen_;
    break;    
  case SQLSTATS_NUM_SQLPROCS:
    sqlStats_item->int64_value = getNumSqlProcs();
    break;
  case SQLSTATS_NUM_CPUS:
    sqlStats_item->int64_value = numCpus_;
    break;
  case SQLSTATS_MASTER_PRIORITY:
    sqlStats_item->int64_value = exePriority_;
    break;
  case SQLSTATS_TRANSID:
    sqlStats_item->int64_value = transId_;
    break;
  case SQLSTATS_AQR_LAST_ERROR:
    sqlStats_item->int64_value = aqrLastErrorCode_;
    break;
  case SQLSTATS_AQR_NUM_RETRIES:
    sqlStats_item->int64_value = numAqrRetries_;
    break;
  case SQLSTATS_AQR_DELAY_BEFORE_RETRY:
    sqlStats_item->int64_value = delayBeforeAqrRetry_;
    break;
  case SQLSTATS_RECLAIM_SPACE_COUNT:
    sqlStats_item->int64_value = reclaimSpaceCount_;
    break;
  case SQLSTATS_EXECUTE_COUNT:
    sqlStats_item->int64_value = exeTimes_.entryCnt();
    break;
  case SQLSTATS_EXECUTE_TIME_MIN:
    sqlStats_item->int64_value = exeTimes_.min();
    break;
  case SQLSTATS_EXECUTE_TIME_MAX:
    sqlStats_item->int64_value = exeTimes_.max();
    break;
  case SQLSTATS_EXECUTE_TIME_AVG:
    sqlStats_item->int64_value = exeTimes_.mean();
    break;
  default:
    sqlStats_item->error_code = -EXE_STAT_NOT_FOUND;
    break;
  }
  return 0;
}

void ExMasterStats::setParentQid(char *queryId, Lng32 queryIdLen)
{
  if (parentQid_ != NULL)
  {
    NADELETEBASIC(parentQid_, (NAHeap *) getHeap());
    parentQid_ = NULL;
    parentQidLen_ = 0;
  }
  if (queryId != NULL)
  {
    parentQid_ = new ((NAHeap *) getHeap()) char[queryIdLen+1];
    str_cpy_all(parentQid_, queryId, queryIdLen);
    parentQid_[queryIdLen] = '\0';
    parentQidLen_ = queryIdLen;
  }
}

void ExMasterStats::setParentQidSystem(char *parentQidSystem, Lng32 len)
{
  if (parentQidSystem != NULL)
  {
     str_cpy_all(parentQidSystem_, parentQidSystem, len);
     parentQidSystem_[len] = '\0';
  }
  else
     parentQidSystem_[0] = '\0';
}

void ExMasterStats::setChildQid(char *queryId, Lng32 queryIdLen)
{
  if (childQid_ != NULL)
  {
    NADELETEBASIC(childQid_, (NAHeap *) getHeap());
    childQid_ = NULL;
    childQidLen_ = 0;
  }
  if (queryId != NULL)
  {
    childQid_ = new ((NAHeap *) getHeap()) char[queryIdLen+1];
    str_cpy_all(childQid_, queryId, queryIdLen);
    childQid_[queryIdLen] = '\0';
    childQidLen_ = queryIdLen;
  }
}

void ExMasterStats::setCancelComment(char const* comment)
{
  if (cancelComment_ != NULL)
  {
    NADELETEBASIC(cancelComment_, (NAHeap *) getHeap());
    cancelComment_ = NULL;
    cancelCommentLen_ = 0;
  }

  cancelCommentLen_ = str_len((char *) comment);
  cancelComment_ = new ((NAHeap *)(getHeap())) char[cancelCommentLen_+1];
  str_cpy_all(cancelComment_, comment, cancelCommentLen_);
  cancelComment_[cancelCommentLen_] = '\0';
}

void ExMasterStats::setIsBlocking()
{
  isBlocking_ = true;
  gettimeofday(&blockOrUnblockSince_, NULL);
}

void ExMasterStats::setNotBlocking()
{
  isBlocking_ = false;
  gettimeofday(&blockOrUnblockSince_, NULL);
}

Int32 ExMasterStats::timeSinceBlocking(Int32 s)
{ 
  Int32 r = 0;
  if (blockOrUnblockSince_.tv_sec  == 0 &&
          blockOrUnblockSince_.tv_usec == 0)
     return r;
  if (isBlocking_)
  {
    timeval timeNow;
    if (exeEndTime_ != -1)
       r = 1; 
    else
    { 
       gettimeofday(&timeNow, NULL);
       r = timeNow.tv_sec - blockOrUnblockSince_.tv_sec;
       if (r < s)
          r = 0;
       else
       if (r == s && (timeNow.tv_usec < blockOrUnblockSince_.tv_usec))
          r = 0;
    }
  }
  return r;
}

Int32 ExMasterStats::timeSinceUnblocking(Int32 s)
{
  Int32 r = 0;
  if (blockOrUnblockSince_.tv_sec  == 0 &&
          blockOrUnblockSince_.tv_usec == 0)
     return r;
  if (! isBlocking_)
  {
    if (exeEndTime_ != -1)
       r = 1;
    else
    {
       timeval timeNow;
       gettimeofday(&timeNow, NULL);
       r = timeNow.tv_sec - blockOrUnblockSince_.tv_sec;
       if (r < s)
          r = 0;
       else
       if (r == s && (timeNow.tv_usec < blockOrUnblockSince_.tv_usec))
          r = 0;
    }
  }
  return r;
}

NABoolean ExMasterStats::filterForCpuStats(short subReqType, 
    Int64 currTimestamp, Lng32  etTimeInSecs)

{
   NABoolean retcode = FALSE;
   Int64 tsToCompare;

   if (queryId_ == NULL)
      return FALSE;
   if (subReqType != SQLCLI_STATS_REQ_UNMONITORED_QUERIES && 
         subReqType != SQLCLI_STATS_REQ_QUERIES_IN_COMPILE  &&
         (collectStatsType_ == (UInt16)ComTdb::ALL_STATS || 
         collectStatsType_ == (UInt16)ComTdb::NO_STATS))
      return FALSE;
   if (subReqType == SQLCLI_STATS_REQ_DEAD_QUERIES)
   {
      if (stmtState_ == Statement::PROCESS_ENDED_)
      {
         if (exeStartTime_ == -1)
            tsToCompare = compEndTime_;
         else
            tsToCompare = exeEndTime_;
         lastActivity_ = (Int32) ((currTimestamp-tsToCompare)/(Int64)1000000);
         if (lastActivity_ > etTimeInSecs)
            retcode = TRUE;
      }
   }
   else
   if (subReqType == SQLCLI_STATS_REQ_UNMONITORED_QUERIES)
   {
      if (collectStatsType_ == (UInt16)ComTdb::NO_STATS ||
          collectStatsType_ == (UInt16)ComTdb::ALL_STATS)
      {
         tsToCompare = -1;
         if (exeEndTime_ != -1)
            tsToCompare = exeEndTime_;
         else
         {
            if (exeStartTime_ != -1)
               tsToCompare = exeStartTime_;
         }
         if (tsToCompare == -1) 
         {
            if (compEndTime_ != -1)
               tsToCompare = compEndTime_;
            else
               tsToCompare = compStartTime_;
         }
         if (tsToCompare == -1)
            retcode = FALSE;
         else
         {
           lastActivity_ = (Int32) ((currTimestamp-tsToCompare)/(Int64)1000000);
           if (lastActivity_ > etTimeInSecs)
              retcode = TRUE;
           else
              retcode = FALSE;
         }
      }
   }
   else
   if (subReqType == SQLCLI_STATS_REQ_QUERIES_IN_CLIENT)
   {
      if (exeStartTime_ != -1 && exeEndTime_ == -1 && (NOT isBlocking_))
      {
         if (timeSinceUnblocking(etTimeInSecs))
            retcode = TRUE;
      }
   }
   else
   if (subReqType == SQLCLI_STATS_REQ_INACTIVE_QUERIES)
   {
      if (stmtState_ != Statement::PROCESS_ENDED_
 	  && stmtState_ != Statement::DEALLOCATED_)
      {
         if ((exeStartTime_ == -1 && exeEndTime_ == -1 && compEndTime_ != -1)
             || (exeStartTime_ != -1 && exeEndTime_ != -1))
         {
            if (exeStartTime_ == -1)
               tsToCompare = compEndTime_;
            else
               tsToCompare = exeEndTime_; 
            lastActivity_ = (Int32)((currTimestamp-tsToCompare) / (Int64)1000000);
            if (lastActivity_ > etTimeInSecs)
               retcode = TRUE;
         }
      }
   }
   else
   if (subReqType == SQLCLI_STATS_REQ_ACTIVE_QUERIES)
   {
      if (stmtState_ != Statement::PROCESS_ENDED_)
      {
         if (exeStartTime_ != -1) {
            if (exeEndTime_ != -1)
               tsToCompare = exeEndTime_;
            else
               tsToCompare = exeStartTime_;
            lastActivity_ = (Int32)((currTimestamp-tsToCompare) / (Int64)1000000);
            if (exeEndTime_ == -1)
               return TRUE;
            else
            if (lastActivity_ <= etTimeInSecs) {
               lastActivity_ = -lastActivity_;
               retcode = TRUE;
            }
         }
      }
   }
   else
   if (subReqType == SQLCLI_STATS_REQ_QUERIES_IN_SQL)
   {
      if (exeStartTime_ != -1 && exeEndTime_ == -1 && isBlocking_)
      {
         if (timeSinceBlocking(etTimeInSecs))
            retcode = TRUE;
      }
   }
   else
   if (subReqType == SQLCLI_STATS_REQ_QUERIES_IN_COMPILE) 
   {
      if (stmtState_ != Statement::PROCESS_ENDED_) 
      {
         if (compStartTime_ != -1 && compEndTime_ == -1) 
         {
            tsToCompare = compStartTime_;
            lastActivity_ = (Int32)((currTimestamp-tsToCompare) / (Int64)1000000);
            if (lastActivity_ >= etTimeInSecs) 
               retcode = TRUE;
         }
      }
   }
   return retcode;
}

void ExMasterStats::setInvalidationKeys(CliGlobals *cliGlobals, 
        SecurityInvKeyInfo *sikInfo, Int32 numObjUIDs, 
        const Int64 *objectUIDs)
{
  ex_assert((numObjUIDs_ ==0) && (numSIKeys_ == 0), "setKeys called twice.");
  Int32 numSIKeys = sikInfo ? sikInfo->getNumSiks() : 0;
  if ((numSIKeys > PreAllocatedSikKeys) || 
      (numObjUIDs > PreAllocatedObjUIDs))
  {
    Long semId = cliGlobals->getSemId();
    StatsGlobals *statsGlobals = cliGlobals->getStatsGlobals();
    if (statsGlobals)
    {
      int error = statsGlobals->getStatsSemaphore(
                    semId, cliGlobals->myPin());
    }

    if (numSIKeys > PreAllocatedSikKeys)
    {
      if (sIKeys_ != &preallocdSiKeys_[0])
        NADELETEBASIC(sIKeys_, (NAHeap *) getHeap());
      sIKeys_ = new ((NAHeap *)(getHeap())) SQL_QIKEY[numSIKeys];
    }
    if (numObjUIDs > PreAllocatedObjUIDs)
    {
      if (objUIDs_ != &preallocdObjUIDs_[0])
        NADELETEBASIC(objUIDs_, (NAHeap *) getHeap());
      objUIDs_ =  new ((NAHeap *)(getHeap())) Int64[numObjUIDs];
    }

    if (statsGlobals)
      statsGlobals->releaseStatsSemaphore(
                      semId, cliGlobals->myPin());
  }

  numSIKeys_ = numSIKeys;
  if (numSIKeys_ > 0)
  {
    // Make sure filler bytes are zero so that memcmp can be used.
    memset(sIKeys_, 0, numSIKeys_ * sizeof(SQL_QIKEY));
    const ComSecurityKey *pComSecurityKey = sikInfo->getSikValues();
    for (int i = 0; i < numSIKeys_; i++)
    {
      sIKeys_[i].revokeKey.subject = pComSecurityKey[i].getSubjectHashValue();
      sIKeys_[i].revokeKey.object  = pComSecurityKey[i].getObjectHashValue();
      char sikOpLit[4];
      ComQIActionTypeEnumToLiteral(pComSecurityKey[i].getSecurityKeyType(),
                                  sikOpLit  );
      sIKeys_[i].operation[0]  =  sikOpLit[0];
      sIKeys_[i].operation[1]  =  sikOpLit[1];
    }
  }
  validPrivs_ = true;

  numObjUIDs_ = numObjUIDs;
  memcpy(objUIDs_, objectUIDs, numObjUIDs_ * sizeof(Int64));
  validDDL_ = true;

}
Lng32 ExStatsTcb::str_parse_stmt_name(char *string, Lng32 len, char *nodeName,
                         short *cpu, pid_t *pid, 
                         Int64 *timeStamp,
                         Lng32 *queryNumber,
                         short *idOffset,
                         short *idLen,
                         short *activeQueryNum,
                         UInt16 *statsMergeType,
                         short *detailLevel,
                         short *subReqType,
                         Lng32 *filter)
{
  char temp[500];
  char *ptr;
  char *internal;
  char *nodeNameTemp = NULL;
  char *pidTemp = NULL;
  char *cpuTemp = NULL;
  char *timeTemp = NULL;
  char *queryNumTemp = NULL;
  char *qidTemp = NULL;
  char *etTemp = NULL;
  char *stmtTemp = NULL;
  char *activeQueryTemp = NULL;
  char *mergeTemp = NULL;
  char *detailTemp = NULL;
  char *tdbIdDetailTemp = NULL;
  char *seTemp = NULL;
  char *seOffendTemp = NULL;
  char *memThreshold = NULL;
  short retcode = SQLCLI_STATS_REQ_NONE;
  Int64 tempNum;
  Lng32 tempLen;
  NABoolean cpuOffender = FALSE;
  NABoolean diskOffender = FALSE;
  NABoolean etOffender = FALSE;
  NABoolean rmsInfo = FALSE;
  NABoolean memOffender = FALSE;
  NABoolean processStats  = FALSE;
  NABoolean pidStats = FALSE;
  NABoolean qidInternalStats = FALSE;

 *cpu = -1;
 *pid = -1;
 *activeQueryNum = RtsCpuStatsReq::ALL_ACTIVE_QUERIES_;

  ex_assert((len > 0 && len < sizeof(temp)), "Len should be between 1 and 500");
  str_cpy_all(temp, string, len);
  temp[len] ='\0';
  *detailLevel = 0;
  ptr = str_tok(temp, '=', &internal);
  if (ptr == NULL)
    return SQLCLI_STATS_REQ_NONE;
  do
  {
    if (strncasecmp(ptr, "STMT", 4) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      stmtTemp = ptr;
      if (stmtTemp != NULL)
        *idLen = (short)str_len(ptr);
      else
        *idLen = 0;
    }
    else
    if (strncasecmp(ptr, "QID_INTERNAL", 12) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      cpuTemp = ptr;
      ptr = str_tok(NULL, ',', &internal);
      pidTemp = ptr;
      ptr = str_tok(NULL, ',', &internal);
      timeTemp = ptr;
      ptr = str_tok(NULL, ',', &internal);
      queryNumTemp = ptr;
      qidInternalStats = TRUE;
    }
    else
    if (strncasecmp(ptr, "QID", 3) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      qidTemp = ptr;
      if (qidTemp != NULL)
        *idLen = (short)str_len(ptr);
      else
        *idLen = 0;
    }
    else
    if (strncasecmp(ptr, "SYSTEM", 6) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      nodeNameTemp = ptr;
    }
    else
    if (strncasecmp(ptr, "CPU_OFFENDER", 12) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      cpuTemp = ptr;
      cpuOffender = TRUE;
    }
    else
    if (strncasecmp(ptr, "HIGHWM_MEM_OFFENDER", 19) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      memThreshold = ptr;
      *subReqType = (short)SQLCLI_STATS_REQ_MEM_HIGH_WM;
      memOffender = TRUE;
    }
    else
    if (strncasecmp(ptr, "ALLOC_MEM_OFFENDER",18) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      memThreshold = ptr;
      *subReqType = (short)SQLCLI_STATS_REQ_MEM_ALLOC;
      memOffender = TRUE;
    }
    else
    if (strncasecmp(ptr, "PFS_USE_PERCENT",15) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      memThreshold = ptr;
      *subReqType = (short)SQLCLI_STATS_REQ_PFS_USE;
      memOffender = TRUE;
    }
    else
    if ((strncasecmp(ptr, "CPU", 3) == 0)  ||
        (strncasecmp(ptr, "NODE", 4) == 0))
    {
      ptr = str_tok(NULL, ',', &internal);
      cpuTemp = ptr;
    }
    else
    if (strncasecmp(ptr, "SE_BLOCKED", 10) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      seTemp = ptr;
      diskOffender = TRUE;
    }
    else
    if (strncasecmp(ptr, "SE_OFFENDER", 10) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      seOffendTemp = ptr;
      diskOffender = TRUE;
    }
    else
    if (strncasecmp(ptr, "QUERIES_IN_SQL", 14) == 0) 
    {
      ptr = str_tok(NULL, ',', &internal);
      etTemp = ptr;
      etOffender = TRUE; 
      *subReqType = (short) SQLCLI_STATS_REQ_QUERIES_IN_SQL;
      retcode = SQLCLI_STATS_REQ_ET_OFFENDER;
    }
    else  
    if (strncasecmp(ptr, "QUERIES_IN_CLIENT", 17) == 0) 
    {
      ptr = str_tok(NULL, ',', &internal);
      etTemp = ptr;
      etOffender = TRUE; 
      *subReqType = (short)SQLCLI_STATS_REQ_QUERIES_IN_CLIENT;
      retcode = SQLCLI_STATS_REQ_ET_OFFENDER;
    }
    else  
    if (strncasecmp(ptr, "INACTIVE_QUERIES", 16) == 0) 
    {
      ptr = str_tok(NULL, ',', &internal);
      etTemp = ptr;
      etOffender = TRUE; 
      *subReqType = (short)SQLCLI_STATS_REQ_INACTIVE_QUERIES;
      retcode = SQLCLI_STATS_REQ_ET_OFFENDER;
    }
    else  
    if (strncasecmp(ptr, "ACTIVE_QUERIES", 14) == 0) 
    {
      ptr = str_tok(NULL, ',', &internal);
      etTemp = ptr;
      etOffender = TRUE; 
      *subReqType = (short)SQLCLI_STATS_REQ_ACTIVE_QUERIES;
      retcode = SQLCLI_STATS_REQ_ET_OFFENDER;
    }
    else	
    if (strncasecmp(ptr, "DEAD_QUERIES", 12) == 0) 
    {
      ptr = str_tok(NULL, ',', &internal);
      etTemp = ptr;
      etOffender = TRUE; 
      *subReqType = SQLCLI_STATS_REQ_DEAD_QUERIES;
      retcode = SQLCLI_STATS_REQ_ET_OFFENDER;
    }
    else  
    if (strncasecmp(ptr, "UNMONITORED_QUERIES", 22) == 0) 
    {
      ptr = str_tok(NULL, ',', &internal);
      etTemp = ptr;
      etOffender = TRUE; 
      *subReqType = (short)SQLCLI_STATS_REQ_UNMONITORED_QUERIES;
      retcode = SQLCLI_STATS_REQ_ET_OFFENDER;
    }
    else
    if (strncasecmp(ptr, "QUERIES_IN_COMPILE", 21) == 0) 
    {
      ptr = str_tok(NULL, ',', &internal);
      etTemp = ptr;
      etOffender = TRUE; 
      *subReqType = (short)SQLCLI_STATS_REQ_QUERIES_IN_COMPILE;
      retcode = SQLCLI_STATS_REQ_ET_OFFENDER;
    }
    else  
    if (strncasecmp(ptr, "PID", 3) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      pidTemp = ptr;
      pidStats = TRUE;
      retcode = SQLCLI_STATS_REQ_PID;
    }
    else
    if (strncasecmp(ptr, "PROCESS_STATS", 13) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      pidTemp = ptr;
      processStats = TRUE;
      retcode = SQLCLI_STATS_REQ_PROCESS_INFO;
    }
    else
    if (strncasecmp(ptr, "ACTIVE", 6) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      activeQueryTemp = ptr;
    }
    else
    if (strncasecmp(ptr, "MERGE", 5) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      mergeTemp = ptr;
    }
    else
    if (strncasecmp(ptr, "DETAIL", 6) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      detailTemp = ptr;
    }
    else
    if ((strncasecmp(ptr, "TDBID_DETAIL", 12) == 0)
        || (strncasecmp(ptr, "TCBID_DETAIL", 12) == 0))
    {
      ptr = str_tok(NULL, ',', &internal);
      tdbIdDetailTemp = ptr;
    }
    else
    if (strncasecmp(ptr, "RMS_INFO", 7) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      cpuTemp = ptr;
      rmsInfo = TRUE;
    }
    else
    if (strncasecmp(ptr, "RMS_CHECK", 9) == 0)
    {
      ptr = str_tok(NULL, ',', &internal);
      cpuTemp = ptr;
      rmsInfo = TRUE;
      *subReqType = SQLCLI_STATS_REQ_RMS_CHECK;
    }
    else
    {
      retcode = SQLCLI_STATS_REQ_STMT;
      *idLen = (short)len;
      *idOffset = 0;
      *cpu = -1;
      *pid = -1;
      nodeName[0] = '\0';
     *statsMergeType = SQLCLI_SAME_STATS;
      return retcode;
    }
  }
  while (ptr != NULL && (ptr = str_tok(NULL, '=', &internal)) != NULL);
  *cpu = -1;
  if (stmtTemp != NULL)
  {
    *idOffset = (short)(stmtTemp - temp);
    retcode = SQLCLI_STATS_REQ_STMT;
  }
  if (qidTemp != NULL)
  {
    *idOffset = (short)(qidTemp - temp);
    if (detailTemp == NULL && tdbIdDetailTemp == NULL)
      retcode = SQLCLI_STATS_REQ_QID;
    else
    {
      retcode = SQLCLI_STATS_REQ_QID_DETAIL;
      // Negative number means - append only root stats at every ESP level and master root level
      // Postive number append stats at that tdb level
      if (detailTemp != NULL)
      {
        tempNum =  str_atoi(detailTemp, str_len(detailTemp));
        if (tempNum > 0)
          tempNum = -tempNum;
        *detailLevel = (short)tempNum;
      }
      else
      if (tdbIdDetailTemp != NULL)
      {
        tempNum =  str_atoi(tdbIdDetailTemp, str_len(tdbIdDetailTemp));
        if (tempNum < 0)
          tempNum = 0;
        *detailLevel = (short)tempNum;
      }
    }
  }
  if (cpuTemp != NULL)
  {
    tempNum =  str_atoi(cpuTemp, str_len(cpuTemp));
    if (tempNum < 0)
      tempNum = -1;
    *cpu = (short)tempNum;
    *pid = -1;
    if (cpuOffender)
      retcode = SQLCLI_STATS_REQ_CPU_OFFENDER;
    else
    if (etOffender)
      retcode = SQLCLI_STATS_REQ_ET_OFFENDER;
    else
    if (rmsInfo)
      retcode = SQLCLI_STATS_REQ_RMS_INFO;
    else if (processStats)
      retcode = SQLCLI_STATS_REQ_PROCESS_INFO;
    else if (pidStats)
      retcode = SQLCLI_STATS_REQ_PID;
    else if (qidInternalStats)
      retcode = SQLCLI_STATS_REQ_QID_INTERNAL;
    else
      retcode = SQLCLI_STATS_REQ_CPU;
  }
  if (seTemp != NULL)
  {
    tempNum =  atoi(seTemp);
    *filter = (Lng32)tempNum;
    retcode = SQLCLI_STATS_REQ_SE_OFFENDER;
  }
  if (seOffendTemp != NULL)
  {
    tempNum =  atoi(seOffendTemp);
    *filter = (Lng32)-tempNum;
    retcode = SQLCLI_STATS_REQ_SE_OFFENDER;
  }
  if (pidTemp != NULL)
  {
    if (strncasecmp(pidTemp, "CURRENT", 7) == 0)
    {
       CliGlobals *cliGlobals = getGlobals()->castToExExeStmtGlobals()->
	                castToExMasterStmtGlobals()->getCliGlobals();
       *pid = cliGlobals->myPin();
       *cpu = cliGlobals->myCpu();
    }
    else
    {
       tempNum =  str_atoi(pidTemp, str_len(pidTemp));
       if (tempNum < 0)
          tempNum = -1;
       *pid = (pid_t)tempNum;
    }
  }
  if (timeTemp != NULL)
  {
     tempNum = str_atoi(timeTemp, str_len(timeTemp));
     if (tempNum < 0)
        tempNum = -1;
     *timeStamp = (Int64)tempNum;
  }
  if (queryNumTemp != NULL)
  {
     tempNum = str_atoi(queryNumTemp, str_len(queryNumTemp));
     if (tempNum < 0)
        tempNum = -1;
     *queryNumber = (Lng32)tempNum;
  }
  if (memThreshold != NULL)
  {
    tempNum =  str_atoi(memThreshold, str_len(memThreshold));
    if (tempNum < 0)
      tempNum = -1;
    *filter = tempNum;
    retcode = SQLCLI_STATS_REQ_MEM_OFFENDER;
  }
  if (etTemp != NULL)
  {
    tempNum =  atoi(etTemp);
    *filter = (Lng32)tempNum;
    retcode = SQLCLI_STATS_REQ_ET_OFFENDER;
  }
  if (nodeNameTemp != NULL)
  {
    tempLen = str_len(nodeNameTemp);
    if (tempLen > MAX_SEGMENT_NAME_LEN)
      tempLen = MAX_SEGMENT_NAME_LEN;
    str_cpy_all(nodeName, nodeNameTemp, tempLen);
    nodeName[tempLen] = '\0';
  }
  else
    nodeName[0] = '\0';
  if (mergeTemp != NULL)
  {
    tempNum = str_atoi(mergeTemp, str_len(mergeTemp));
    if (tempNum == SQLCLI_ACCUMULATED_STATS || tempNum == SQLCLI_PERTABLE_STATS ||
        tempNum == SQLCLI_PROGRESS_STATS)
      *statsMergeType = (UInt16)tempNum;
    else if (tempNum == 0)
      *statsMergeType = SQLCLI_DEFAULT_STATS; // SET session default statistics_view_type
    else
      *statsMergeType = SQLCLI_SAME_STATS;
  }
  else
    *statsMergeType = SQLCLI_SAME_STATS;
  if (*statsMergeType == SQLCLI_DEFAULT_STATS)
  {
    ContextCli *context = getGlobals()->castToExExeStmtGlobals()->
	                castToExMasterStmtGlobals()->getStatement()->getContext();
    SessionDefaults *sd = context->getSessionDefaults();
    if (sd)
      *statsMergeType = (UInt16)sd->getStatisticsViewType();
    else
      *statsMergeType = SQLCLI_SAME_STATS;
  }
  if (activeQueryTemp != NULL)
  {
    tempNum =  str_atoi(activeQueryTemp, str_len(activeQueryTemp));
    if (tempNum < -1)
      tempNum = -1;
    *activeQueryNum = (short)tempNum;
  }
  else
  {
    switch (retcode)
    {
    case SQLCLI_STATS_REQ_PID:
    case SQLCLI_STATS_REQ_CPU:
      *activeQueryNum = 1;
      break;
    case SQLCLI_STATS_REQ_QID:
    case SQLCLI_STATS_REQ_QID_DETAIL:
    case SQLCLI_STATS_REQ_PROCESS_INFO:
      *activeQueryNum = RtsQueryId::ANY_QUERY_;
      break;
    case SQLCLI_STATS_REQ_CPU_OFFENDER:
    case SQLCLI_STATS_REQ_MEM_OFFENDER:
    case SQLCLI_STATS_REQ_SE_OFFENDER:
    case SQLCLI_STATS_REQ_ET_OFFENDER:
      *activeQueryNum = RtsCpuStatsReq::ALL_ACTIVE_QUERIES_;
      break;
    default:
      *activeQueryNum = RtsQueryId::ANY_QUERY_;
      break;
    }
  }
  if (qidInternalStats)
  {
     retcode = SQLCLI_STATS_REQ_QID_INTERNAL;
     if ( -1 == *cpu || -1 == *pid || -1 == *timeStamp || -1 == *queryNumber )
     {
         *cpu = -1;
         *pid = -1;
         *timeStamp = -1;
         *queryNumber = -1;
     }
  }
  if (retcode == SQLCLI_STATS_REQ_NONE)
  {
    retcode = SQLCLI_STATS_REQ_STMT;
    *idLen = (short)len;
    *idOffset = 0;
    *cpu = -1;
    *pid = -1;
    nodeName[0] = '\0';
  }
  return retcode;
}

NABoolean ExOperStatsId::compare(const ExOperStatsId &other, 
    ComTdb::CollectStatsType cst) const
{
  switch (cst)
  {
  case ComTdb::ACCUMULATED_STATS:
  case ComTdb::PERTABLE_STATS:
  case ComTdb::OPERATOR_STATS:
  case ComTdb::PROGRESS_STATS:
    return (tdbId_ == other.tdbId_);
    break;
  default:
    return (fragId_ == other.fragId_ && 
	    tdbId_ == other.tdbId_ &&
	    instNum_ == other.instNum_ && 
	    subInstNum_ == other.subInstNum_); 
  }
}

ExRMSStats::ExRMSStats(NAHeap *heap)
  :ExOperStats(heap, RMS_STATS)
{
  rmsVersion_ = 0;
  nodeName_[0] = '\0';
  cpu_ = -1;
  sscpPid_ = -1;
  sscpPriority_ = -1;
  sscpTimestamp_ = -1;
  ssmpPid_ = -1;
  ssmpPriority_ = -1;
  ssmpTimestamp_ = -1;
  storeSqlSrcLen_ = -1;
  rmsEnvType_ = -1;
  currGlobalStatsHeapAlloc_ = 0;
  currGlobalStatsHeapUsage_ = 0;
  globalStatsHeapWatermark_ = 0;
  currNoOfStmtStats_ = 0;
  currNoOfRegProcesses_ = 0;
  currNoOfProcessStatsHeap_ = 0;
  semPid_ = -1;
  sscpOpens_ = 0;
  sscpDeletedOpens_ = 0;
  stmtStatsGCed_ = 0;
  lastGCTime_ = NA_JulianTimestamp();
  totalStmtStatsGCed_ = 0;
  ssmpReqMsgCnt_ = 0;
  ssmpReqMsgBytes_ = 0;
  ssmpReplyMsgCnt_ = 0;
  ssmpReplyMsgBytes_ = 0;
  sscpReqMsgCnt_ = 0;
  sscpReqMsgBytes_ = 0;
  sscpReplyMsgCnt_ = 0;
  sscpReplyMsgBytes_ = 0;
  rmsStatsResetTimestamp_ = NA_JulianTimestamp();
  numQueryInvKeys_ = 0;
  nodesInCluster_ = 0;
  configuredPidMax_ = 0;
}

void ExRMSStats::reset()
{
  totalStmtStatsGCed_ = 0;
  ssmpReqMsgCnt_ = 0;
  ssmpReqMsgBytes_ = 0;
  ssmpReplyMsgCnt_ = 0;
  ssmpReplyMsgBytes_ = 0;
  sscpReqMsgCnt_ = 0;
  sscpReqMsgBytes_ = 0;
  sscpReplyMsgCnt_ = 0;
  sscpReplyMsgBytes_ = 0;
  rmsStatsResetTimestamp_ = NA_JulianTimestamp();
}

void ExRMSStats::setNodeName(char *nodeName)
{
  short len = str_len(nodeName);
  str_cpy_all(nodeName_, nodeName, len);
  nodeName_[len] = '\0';
}

UInt32 ExRMSStats::packedLength()
{
  UInt32 size;
  size = ExOperStats::packedLength();
  alignSizeForNextObj(size);
  size += sizeof(ExRMSStats)-sizeof(ExOperStats);
  return size;
}

UInt32 ExRMSStats::pack(char * buffer)
{
  UInt32 packedLen;
  packedLen = ExOperStats::pack(buffer);
  alignSizeForNextObj(packedLen);
  buffer += packedLen;
  UInt32 srcLen = sizeof(ExRMSStats)-sizeof(ExOperStats);
  char * srcPtr = (char *)this+sizeof(ExOperStats);
  memcpy(buffer, (void *)srcPtr, srcLen);
  return packedLen+srcLen;
}

void ExRMSStats::unpack(const char* &buffer)
{
  UInt32 srcLen;
  ExOperStats::unpack(buffer);
  alignBufferForNextObj(buffer); 
  srcLen = sizeof(ExRMSStats)-sizeof(ExOperStats);
  char * srcPtr = (char *)this+sizeof(ExOperStats);
  memcpy((void *)srcPtr, buffer, srcLen);
  buffer += srcLen;
}

ExOperStats *ExRMSStats::copyOper(NAMemory * heap)
{
  ExRMSStats * stat = new(heap) ExRMSStats((NAHeap *)heap);
  stat->copyContents(this);
  return stat;
}

void ExRMSStats::copyContents(ExRMSStats * other)
{
  char * srcPtr = (char *)other+sizeof(ExOperStats);
  char * destPtr = (char *)this+sizeof(ExOperStats);
  UInt32 srcLen = sizeof(ExRMSStats)-sizeof(ExOperStats);
  memcpy((void *)destPtr, (void *)srcPtr, srcLen);
}

void ExRMSStats::getVariableStatsInfo(char * dataBuffer,
			  char * dataLen,
          		  Lng32 maxLen)
{
  char *buf = dataBuffer;
  char tmpbuf[150];
  int status = 0;  // 0 - ok - 1 warning 2- Error
  const char *statusStr[3] = {
     "OK",
     "WARNING",
     "ERROR"
  };

  if (subReqType()== SQLCLI_STATS_REQ_RMS_CHECK)
  {
      sprintf(buf,
         "statsRowType: %d nodeId: %d nodeName: %s ",
         SQLCLI_RMS_CHECK_STATS,
         cpu_,
         nodeName_
      );
      if (sscpOpens_ != 0 && nodesInCluster_ != sscpOpens_)
      {    
         sprintf(tmpbuf, "sscpOpens: %d ", sscpOpens_);
         if (status < 1)
            status = 1;
         strcat(buf, tmpbuf);
      }   
      if (sscpDeletedOpens_ > 0)
      {    
         sprintf(tmpbuf, "sscpDeletedOpens: %d ", sscpDeletedOpens_);
         if (status < 1)
            status = 1;
         strcat(buf, tmpbuf);
      }   
      short statsHeapFreePercent = 0;
      char *tempCValue = GetCliGlobals()->getEnv("STATS_HEAP_FREE_PERCENT");
      if (tempCValue != NULL)
          statsHeapFreePercent = atoi(tempCValue);
      if (statsHeapFreePercent <= 0)
          statsHeapFreePercent = 40;
      if (((double) currGlobalStatsHeapUsage_  * 100 /
                  (double) currGlobalStatsHeapAlloc_) > 
                    (100 - statsHeapFreePercent))
      {
         sprintf(tmpbuf, "statsHeapUsed: %ld statsHeapTotal: %ld statsHeapWM: %ld ",
                  currGlobalStatsHeapUsage_,
                  currGlobalStatsHeapAlloc_,
                  globalStatsHeapWatermark_);
         if (status < 1)
            status = 1;
         strcat(buf, tmpbuf);
      } 
      if ((currNoOfProcessStatsHeap_ - currNoOfRegProcesses_) > 100)
      {
         sprintf(tmpbuf, "noOfProcessRegd: %d  noOfProcessStatsHeaps: %d ",
                   currNoOfRegProcesses_,
                   currNoOfProcessStatsHeap_);
         if (status < 1)
            status = 1;
         strcat(buf, tmpbuf);
      } 
      sprintf(tmpbuf, "Status: %s ", statusStr[status]);
      strcat(buf, tmpbuf);
  }
  else
  {
    sprintf (
       buf,
       "statsRowType: %d rmsVersion: %d nodeName: %s cpu: %d nodeId: %d "
       "sscpPid: %d sscpPri: %d sscpTimestamp: %ld "
       "ssmpPid: %d ssmpPri: %d ssmpTimestamp: %ld srcLen: %d rtsEnvType: %d "
        "statsHeapUsed: %ld "
        "statsHeapTotal: %ld statsHeapWM: %ld noOfProcessRegd: %d  noOfProcessStatsHeaps: %d "
        "noOfQidRegd: %d semPid: %d sscpOpens: %d sscpDeleted: %d "
        "stmtStatsGCed: %d lastGCTime: %ld "
        "totalStmtStatsGCed: %ld ssmpReqMsgCnt: %ld ssmpReqMsgBytes: %ld ssmpReplyMsgCnt: %ld ssmpReplyMsgBytes: %ld "
        "sscpReqMsgCnt: %ld sscpReqMsgBytes: %ld sscpReplyMsgCnt: %ld sscpReplyMsgBytes: %ld resetTimestamp: %ld " 
        "numQueryInvKeys: %d  configuredPidMax: %d",
        statType(),
        rmsVersion_,
        nodeName_,
        cpu_,
        cpu_,
        sscpPid_,
        sscpPriority_,
        sscpTimestamp_,
        ssmpPid_,
        ssmpPriority_,
        ssmpTimestamp_,
        storeSqlSrcLen_,
        rmsEnvType_,
        currGlobalStatsHeapUsage_,
        currGlobalStatsHeapAlloc_,
        globalStatsHeapWatermark_,
        currNoOfRegProcesses_,
        currNoOfProcessStatsHeap_,
        currNoOfStmtStats_,
        semPid_,
        sscpOpens_,
        sscpDeletedOpens_,
        stmtStatsGCed_,
        lastGCTime_,
        totalStmtStatsGCed_,
        ssmpReqMsgCnt_,
        ssmpReqMsgBytes_,
        ssmpReplyMsgCnt_,
        ssmpReplyMsgBytes_,
        sscpReqMsgCnt_,
        sscpReqMsgBytes_,
        sscpReplyMsgCnt_,
        sscpReplyMsgBytes_,
        rmsStatsResetTimestamp_,
        numQueryInvKeys_,
        configuredPidMax_
       );
  }
  buf += str_len(buf);
  *(short*)dataLen = (short) (buf - dataBuffer);
}

ExRMSStats *ExRMSStats::castToExRMSStats()
{
  return this;
}

void ExRMSStats::merge(ExRMSStats* other)
{
  copyContents(other);
}

Lng32 ExRMSStats::getStatsItem(SQLSTATS_ITEM* sqlStats_item)
{
  sqlStats_item->error_code = 0;
  short len;
  switch (sqlStats_item->statsItem_id)
  {
  case SQLSTATS_RMS_VER:
    sqlStats_item->int64_value = rmsVersion_;
    break;
  case SQLSTATS_NODE_NAME:
    if (sqlStats_item->str_value != NULL)
    {
      len = str_len(nodeName_);
      if (len <= sqlStats_item->str_max_len)
      {
        str_cpy(sqlStats_item->str_value, nodeName_, len);
        sqlStats_item->str_ret_len = len;
      }
      else
        sqlStats_item->error_code = -EXE_ERROR_IN_STAT_ITEM;
    }
    break;
  case SQLSTATS_CPU:
    sqlStats_item->int64_value = cpu_;
    break;
  case SQLSTATS_SSCP_PID:
    sqlStats_item->int64_value = sscpPid_;
    break;
  case SQLSTATS_SSCP_PRIORITY:
    sqlStats_item->int64_value = sscpPriority_;
    break;
  case SQLSTATS_SSCP_TIMESTAMP:
    sqlStats_item->int64_value = sscpTimestamp_;
    break;
  case SQLSTATS_SSMP_PID:
    sqlStats_item->int64_value = ssmpPid_;
    break;
  case SQLSTATS_SSMP_PRIORITY:
    sqlStats_item->int64_value = ssmpPriority_;
    break;
  case SQLSTATS_SSMP_TIMESTAMP:
    sqlStats_item->int64_value = ssmpTimestamp_;
    break;
  case SQLSTATS_STORE_SRC_LEN:
    sqlStats_item->int64_value = storeSqlSrcLen_;
    break;
  case SQLSTATS_RMS_ENV_TYPE:
    sqlStats_item->int64_value = rmsEnvType_;
    break;
  case SQLSTATS_STATS_HEAP_ALLOC:
    sqlStats_item->int64_value = currGlobalStatsHeapAlloc_;
    break;
  case SQLSTATS_STATS_HEAP_USED:
    sqlStats_item->int64_value = currGlobalStatsHeapUsage_;
    break;
  case SQLSTATS_STATS_HEAP_HIGH_WM:
    sqlStats_item->int64_value = globalStatsHeapWatermark_;
    break;
  case SQLSTATS_PROCESS_STATS_HEAPS:
    sqlStats_item->int64_value = currNoOfProcessStatsHeap_;
    break;
  case SQLSTATS_PROCESSES_REGD:
    sqlStats_item->int64_value = currNoOfRegProcesses_;
    break;
  case SQLSTATS_QUERIES_REGD:
    sqlStats_item->int64_value = currNoOfStmtStats_;
    break;
  case SQLSTATS_RMS_SEMAPHORE_PID:
    sqlStats_item->int64_value = semPid_;
    break;
  case SQLSTATS_SSCPS_OPENED:
    sqlStats_item->int64_value = sscpOpens_;
    break;
  case SQLSTATS_SSCPS_DELETED_OPENS:
    sqlStats_item->int64_value = sscpDeletedOpens_;
    break;
  case SQLSTATS_LAST_GC_TIME:
    sqlStats_item->int64_value = lastGCTime_;
    break;
  case SQLSTATS_QUERIES_GCED_IN_LAST_RUN:
    sqlStats_item->int64_value = stmtStatsGCed_;
    break;
  case SQLSTATS_TOTAL_QUERIES_GCED:
    sqlStats_item->int64_value = totalStmtStatsGCed_;
    break;
  case SQLSTATS_SSMP_REQ_MSG_CNT:
    sqlStats_item->int64_value = ssmpReqMsgCnt_;
    break;
  case SQLSTATS_SSMP_REQ_MSG_BYTES:
    sqlStats_item->int64_value = ssmpReqMsgBytes_;
    break;
  case SQLSTATS_SSMP_REPLY_MSG_CNT:
    sqlStats_item->int64_value = ssmpReplyMsgCnt_;
    break;
  case SQLSTATS_SSMP_REPLY_MSG_BYTES:
    sqlStats_item->int64_value = ssmpReplyMsgBytes_;
    break;
  case SQLSTATS_SSCP_REQ_MSG_CNT:
    sqlStats_item->int64_value = sscpReqMsgCnt_;
    break;
  case SQLSTATS_SSCP_REQ_MSG_BYTES:
    sqlStats_item->int64_value = sscpReqMsgBytes_;
    break;
  case SQLSTATS_SSCP_REPLY_MSG_CNT:
    sqlStats_item->int64_value = sscpReplyMsgCnt_;
    break;
  case SQLSTATS_SSCP_REPLY_MSG_BYTES:
    sqlStats_item->int64_value = sscpReplyMsgBytes_;
    break;  
  case SQLSTATS_RMS_STATS_RESET_TIMESTAMP:
    sqlStats_item->int64_value = rmsStatsResetTimestamp_;
    break;
  case SQLSTATS_RMS_STATS_NUM_SQL_SIK:
    sqlStats_item->int64_value = numQueryInvKeys_;
    break;
  case SQLSTATS_RMS_CONFIGURED_PID_MAX:
    sqlStats_item->int64_value = configuredPidMax_;
    break;
  default:
    sqlStats_item->error_code = -EXE_STAT_NOT_FOUND;
    break;
  }
  return 0;
}

ExBMOStats::ExBMOStats(NAMemory *heap)
  :ExOperStats(heap, BMO_STATS)
{
  init(FALSE);
  spaceBufferSize_ = -1;
  scratchIOSize_ = -1;
  scratchOverflowMode_ = -1;
}

ExBMOStats::ExBMOStats(NAMemory *heap, StatType statType)
  :ExOperStats(heap, statType)
{
  init(FALSE);
  spaceBufferSize_ = -1;
  scratchIOSize_ = -1;
  scratchOverflowMode_ = -1;
  estMemoryUsage_ = 0; 
}

ExBMOStats::ExBMOStats(NAMemory *heap, StatType statType,
			 ex_tcb *tcb,
			 const ComTdb * tdb)
  :ExOperStats(heap, statType, tcb, tdb)
{
  init(FALSE);
  spaceBufferSize_ = -1;
  scratchIOSize_ = -1;
  if (tdb != NULL) {
    scratchOverflowMode_ = ((ComTdb *)tdb)->getOverFlowMode();
    estMemoryUsage_ = ((ComTdb *)tdb)->getEstimatedMemoryUsage();
  }
  else {
    scratchOverflowMode_ = -1;
    estMemoryUsage_ = 0;
  }
}

ExBMOStats::ExBMOStats(NAMemory *heap, 
			 ex_tcb *tcb,
			 const ComTdb * tdb)
  : ExOperStats(heap, BMO_STATS, tcb, tdb)
  , timer_(CLOCK_MONOTONIC)
{
  init(FALSE);
  spaceBufferSize_ = -1;
  scratchIOSize_ = -1;
  if (tdb != NULL) {
    scratchOverflowMode_ = ((ComTdb *)tdb)->getOverFlowMode();
    estMemoryUsage_ = ((ComTdb *)tdb)->getEstimatedMemoryUsage();
  }
  else {
    scratchOverflowMode_ = -1;
    estMemoryUsage_ = 0;
  }
}

void ExBMOStats::init(NABoolean resetDop)
{
  ExOperStats::init(resetDop);
  bmoHeapAlloc_ = 0;
  bmoHeapUsage_ = 0;
  bmoHeapWM_ = 0;
  spaceBufferCount_ = 0;
  scratchFileCount_ = 0;
  scratchBufferBlockSize_ = -1;
  scratchBufferBlockRead_ = 0;
  scratchBufferBlockWritten_ = 0;
  scratchReadCount_ = 0;
  scratchWriteCount_ = 0;
  topN_ = -1;
  timer_.reset();
  scratchIOMaxTime_ = 0;
  phase_ = 0;
  interimRowCount_ = 0;
}

UInt32 ExBMOStats::packedLength()
{
  UInt32 size;
  size = ExOperStats::packedLength();
  alignSizeForNextObj(size);
  size += sizeof(ExBMOStats)-sizeof(ExOperStats);
  return size;
}

UInt32 ExBMOStats::pack(char * buffer)
{
  UInt32 packedLen;
  packedLen = ExOperStats::pack(buffer);
  alignSizeForNextObj(packedLen);
  buffer += packedLen;
  UInt32 srcLen = sizeof(ExBMOStats)-sizeof(ExOperStats);
  char * srcPtr = (char *)this+sizeof(ExOperStats);
  memcpy(buffer, (void *)srcPtr, srcLen);
  return packedLen+srcLen;
}

void ExBMOStats::unpack(const char* &buffer)
{
  UInt32 srcLen;
  ExOperStats::unpack(buffer);
  if (getVersion() >= _STATS_RTS_VERSION_R25)
  {
    alignBufferForNextObj(buffer); 
    srcLen = sizeof(ExBMOStats)-sizeof(ExOperStats);
    char * srcPtr = (char *)this+sizeof(ExOperStats);
    memcpy((void *)srcPtr, buffer, srcLen);
    buffer += srcLen;
  }
}

ExOperStats *ExBMOStats::copyOper(NAMemory * heap)
{
  ExBMOStats * stat = new(heap) ExBMOStats((NAHeap *)heap);
  stat->copyContents(this);
  return stat;
}

void ExBMOStats::copyContents(ExBMOStats * other)
{
  ExOperStats::copyContents(other);
  char * srcPtr = (char *)other+sizeof(ExOperStats);
  char * destPtr = (char *)this+sizeof(ExOperStats);
  UInt32 srcLen = sizeof(ExBMOStats)-sizeof(ExOperStats);
  memcpy((void *)destPtr, (void *)srcPtr, srcLen);
}
const char * ExBMOStats::getNumValTxt(Int32 i) const
{
  switch (i)
  {
    case 1:
      return "OperCpuTime";
    case 2:
      return "scrIOCount";
    case 3:
      return "bmoHeapAllocated";
    case 4:
      return "scrFileCount";
  }
  return NULL;
}

Int64 ExBMOStats::getNumVal(Int32 i) const
{
  switch (i)
  {
     case 1:
        return ExOperStats::getNumVal(i);
     case 2:
        return scratchReadCount_+scratchWriteCount_;
     case 3:
        return bmoHeapAlloc_;
     case 4:
        return scratchFileCount_;
  }
  return 0;
}
  
void ExBMOStats::getVariableStatsInfo(char * dataBuffer,
			  char * dataLen,
          		  Lng32 maxLen)
{
  char *buf = dataBuffer;
  sprintf (
       buf,
       "statsRowType: %d explainTdbId: %d bmoPhase: %s bmoIntCount: %ld estMemory: %f bmoHeapUsed: %d bmoHeapTotal: %d bmoHeapWM: %d "
       "bmoSpaceBufferSize: %d bmoSpaceBufferCount: %d "
       "scrOverFlowMode: %d scrFileCount: %d scrBufferBlockSize: %d scrBuffferRead: %d scrBufferWritten: %d "
       "scrWriteCount: %ld scrReadCount: %ld sortTopN: %d scrIOSize: %d scrIOTime: %ld scrIOMaxTime: %ld ",
        statType(),
        getExplainNodeId(),
        getBmoPhaseStr(),
        interimRowCount_,
        estMemoryUsage_, 
        bmoHeapUsage_,
        bmoHeapAlloc_,
        bmoHeapWM_,
        spaceBufferSize_,
        spaceBufferCount_,
        scratchOverflowMode_,
        scratchFileCount_,
        scratchBufferBlockSize_,
        scratchBufferBlockRead_,
        scratchBufferBlockWritten_,
        scratchWriteCount_,
        scratchReadCount_,
        topN_,
        scratchIOSize_,
        timer_.getTime(),
        scratchIOMaxTime_
       );
  buf += str_len(buf);
  *(short*)dataLen = (short) (buf - dataBuffer);
}

ExBMOStats *ExBMOStats::castToExBMOStats()
{
  return this;
}

void ExBMOStats::merge(ExBMOStats* other)
{
  ExOperStats::merge(other);
  timer_ = timer_ + other->timer_;
  bmoHeapUsage_ += other->bmoHeapUsage_;
  bmoHeapAlloc_ += other->bmoHeapAlloc_;
  bmoHeapWM_ += other->bmoHeapWM_;
  if (other->spaceBufferSize_ != -1)
    spaceBufferSize_ = other->spaceBufferSize_;
  spaceBufferCount_ += other->spaceBufferCount_;
  if (other->scratchBufferBlockSize_ != -1)
    scratchBufferBlockSize_ = other->scratchBufferBlockSize_;
  if (other->scratchIOSize_ != -1)
    scratchIOSize_ = other->scratchIOSize_;
  if (other->topN_ != -1)
     topN_ = other->topN_;
  if (other->phase_ > phase_)
     phase_ = other->phase_;
  scratchOverflowMode_ = other->scratchOverflowMode_;
  scratchFileCount_ += other->scratchFileCount_;
  scratchBufferBlockRead_ += other->scratchBufferBlockRead_;
  scratchBufferBlockWritten_ += other->scratchBufferBlockWritten_;
  scratchReadCount_ += other->scratchReadCount_;
  scratchWriteCount_ += other->scratchWriteCount_;
  interimRowCount_ += other->interimRowCount_;
  if (other->scratchIOMaxTime_ > scratchIOMaxTime_)
     scratchIOMaxTime_ = other->scratchIOMaxTime_;
}

const char *ExBMOStats::getBmoPhaseStr()
{
  ComTdb::ex_node_type tdbType = getTdbType();
  
  if (tdbType == ComTdb::ex_HASHJ)
     return ex_hashj_tcb::HashJoinPhaseStr[phase_];
  else if (tdbType == ComTdb::ex_SORT)
    return ExSortTcb::SortPhaseStr[phase_];
  else if (tdbType == ComTdb::ex_HASH_GRBY)
     return ex_hash_grby_tcb::HashGrbyPhaseStr[phase_];
  else
    return "UNKNOWN";  
}

Lng32 ExBMOStats::getStatsItem(SQLSTATS_ITEM* sqlStats_item)
{
  sqlStats_item->error_code = 0;
  short len;
  char tmpBuf[100];

  switch (sqlStats_item->statsItem_id)
  {
  case SQLSTATS_BMO_HEAP_USED:
    sqlStats_item->int64_value = bmoHeapUsage_;
    break;
  case SQLSTATS_BMO_HEAP_ALLOC:
    sqlStats_item->int64_value = bmoHeapAlloc_;
    break;
  case SQLSTATS_BMO_HEAP_WM:
    sqlStats_item->int64_value = bmoHeapWM_;
    break;
  case SQLSTATS_BMO_SPACE_BUFFER_SIZE:
    sqlStats_item->int64_value = spaceBufferSize_;
    break;
  case SQLSTATS_BMO_SPACE_BUFFER_COUNT:
    sqlStats_item->int64_value = spaceBufferCount_;
    break;
  case SQLSTATS_SCRATCH_OVERFLOW_MODE:
    sqlStats_item->int64_value = scratchOverflowMode_;
    break;
  case SQLSTATS_TOPN:
    sqlStats_item->int64_value = topN_;
    break;
  case SQLSTATS_SCRATCH_FILE_COUNT:
    sqlStats_item->int64_value = scratchFileCount_;
    break;
  case SQLSTATS_SCRATCH_BUFFER_BLOCK_SIZE:
    sqlStats_item->int64_value = scratchBufferBlockSize_;
    break;
  case SQLSTATS_SCRATCH_BUFFER_BLOCKS_READ:
    sqlStats_item->int64_value = scratchBufferBlockRead_;
    break;
  case SQLSTATS_SCRATCH_BUFFER_BLOCKS_WRITTEN:
    sqlStats_item->int64_value = scratchBufferBlockWritten_;
    break;
  case SQLSTATS_SCRATCH_READ_COUNT:
    sqlStats_item->int64_value = scratchReadCount_;
    break;
  case SQLSTATS_SCRATCH_WRITE_COUNT:
    sqlStats_item->int64_value = scratchWriteCount_;
    break;
  case SQLSTATS_SCRATCH_IO_TIME:
    sqlStats_item->int64_value = timer_.getTime();
    break;
  case SQLSTATS_SCRATCH_IO_SIZE:
    sqlStats_item->int64_value = scratchIOSize_;
    break;
  case SQLSTATS_SCRATCH_IO_MAX_TIME:
    sqlStats_item->int64_value = scratchIOMaxTime_;
    break;
  case SQLSTATS_BMO_EST_MEMORY:
    sqlStats_item->double_value = estMemoryUsage_;
    break;
  case SQLSTATS_INTERIM_ROW_COUNT:
    sqlStats_item->int64_value = interimRowCount_;
    break;
  case SQLSTATS_BMO_PHASE:
    if (sqlStats_item->str_value != NULL)
    {
       const char *bmoPhase = getBmoPhaseStr();
       len = strlen(bmoPhase);
       if (len > sqlStats_item->str_max_len)
          sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
       else
          str_cpy(sqlStats_item->str_value, bmoPhase, len);
       sqlStats_item->str_ret_len = len;
    }
    else
       sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
    break;
  case SQLSTATS_DETAIL:
   if (sqlStats_item->str_value != NULL)
    {
      char* buf = tmpBuf;
      sprintf(buf, "%ld|%ld|%ld|", getNumVal(2), getNumVal(3), getNumVal(4)); 
      len = str_len(tmpBuf);
      if (len > sqlStats_item->str_max_len)
        sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
      else
        str_cpy(sqlStats_item->str_value, tmpBuf, len);
      sqlStats_item->str_ret_len = len;
    }
    else
       sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
    break;
  default:
    ExOperStats::getStatsItem(sqlStats_item);
    break;
  }
  return 0;
}

const char *ExBMOStats::getScratchOverflowMode(Int16 overflowMode)
{
  switch(overflowMode)
  {
  case SQLCLI_OFM_DISK_TYPE:
    return "DISK";
  case SQLCLI_OFM_SSD_TYPE:
    return "SSD";
  case SQLCLI_OFM_MMAP_TYPE:
    return "MMAP";
  default:
    return "UNKNOWN";
  }
}

ExUDRBaseStats::ExUDRBaseStats(NAMemory *heap)
  :ExOperStats(heap, UDR_BASE_STATS)
{
  init(FALSE);
}

ExUDRBaseStats::ExUDRBaseStats(NAMemory *heap, StatType statType)
  :ExOperStats(heap, statType)
{
  init(FALSE);
}

ExUDRBaseStats::ExUDRBaseStats(NAMemory *heap, StatType statType,
			 ex_tcb *tcb,
			 const ComTdb * tdb)
  :ExOperStats(heap, statType, tcb, tdb)
{
  init(FALSE);
}

ExUDRBaseStats::ExUDRBaseStats(NAMemory *heap, 
			 ex_tcb *tcb,
			 const ComTdb * tdb)
  :ExOperStats(heap, UDR_BASE_STATS, tcb, tdb)
{
  init(FALSE);
}

void ExUDRBaseStats::init(NABoolean resetDop)
{
  ExOperStats::init(resetDop);
  reqMsgCnt_ = 0;
  reqMsgBytes_ = 0;
  replyMsgCnt_ = 0;
  replyMsgBytes_ = 0;
  udrCpuTime_ = 0;
  UDRServerId_[0] = '\0';
  recentReqTS_ = 0;
  recentReplyTS_ = 0;
}

UInt32 ExUDRBaseStats::packedLength()
{
  UInt32 size;
  size = ExOperStats::packedLength();
  alignSizeForNextObj(size);
  size += sizeof(ExUDRBaseStats)-sizeof(ExOperStats);
  return size;
}

UInt32 ExUDRBaseStats::pack(char * buffer)
{
  UInt32 packedLen;
  packedLen = ExOperStats::pack(buffer);
  alignSizeForNextObj(packedLen);
  buffer += packedLen;
  UInt32 srcLen = sizeof(ExUDRBaseStats)-sizeof(ExOperStats);
  char * srcPtr = (char *)this+sizeof(ExOperStats);
  memcpy(buffer, (void *)srcPtr, srcLen);
  return packedLen+srcLen;
}

void ExUDRBaseStats::unpack(const char* &buffer)
{
  UInt32 srcLen;
  ExOperStats::unpack(buffer);
  if (getVersion() >= _STATS_RTS_VERSION_R25_1)
  {
    alignBufferForNextObj(buffer); 
    srcLen = sizeof(ExUDRBaseStats)-sizeof(ExOperStats);
    char * srcPtr = (char *)this+sizeof(ExOperStats);
    memcpy((void *)srcPtr, buffer, srcLen);
    buffer += srcLen;
  }
}

ExOperStats *ExUDRBaseStats::copyOper(NAMemory * heap)
{
  ExUDRBaseStats * stat = new(heap) ExUDRBaseStats((NAHeap *)heap);
  stat->copyContents(this);
  return stat;
}

void ExUDRBaseStats::copyContents(ExUDRBaseStats * other)
{
  ExOperStats::copyContents(other);
  char * srcPtr = (char *)other+sizeof(ExOperStats);
  char * destPtr = (char *)this+sizeof(ExOperStats);
  UInt32 srcLen = sizeof(ExUDRBaseStats)-sizeof(ExOperStats);
  memcpy((void *)destPtr, (void *)srcPtr, srcLen);
}

void ExUDRBaseStats::getVariableStatsInfo(char * dataBuffer,
			  char * dataLen,
          		  Lng32 maxLen)
{
  char *buf = dataBuffer;
  char tmpBuf[50];
  sprintf (
       buf,
       "statsRowType: %d reqMsgCnt: %ld regMsgBytes: %ld replyMsgCnt: %ld replyMsgBytes: %ld "
       "udrCpuTime: %ld recentReqTS: %ld recentReplyTS: %ld ",
        statType(),
        reqMsgCnt_,
        reqMsgBytes_,
        replyMsgCnt_,
        replyMsgBytes_,
        udrCpuTime_,
        recentReqTS_,
        recentReplyTS_ 
       );
  if ((Int32)getCollectStatsType() == SQLCLI_QID_DETAIL_STATS)
  { 
    sprintf(tmpBuf, " UDRServerId: %s ", (UDRServerId_[0] =='\0' ? "None" : UDRServerId_));
    str_cat(buf, tmpBuf, buf);
  }
  buf += str_len(buf);
  *(short*)dataLen = (short) (buf - dataBuffer);
}

ExUDRBaseStats *ExUDRBaseStats::castToExUDRBaseStats()
{
  return this;
}

void ExUDRBaseStats::merge(ExUDRBaseStats* other)
{
  ExOperStats::merge(other);
  reqMsgCnt_ += other->reqMsgCnt_;
  reqMsgBytes_ += other->reqMsgBytes_;
  replyMsgCnt_ += other->replyMsgCnt_;
  replyMsgBytes_ += other->replyMsgBytes_;
  udrCpuTime_ += other->udrCpuTime_;
  //UDRServerId_ ??
  if (other->recentReqTS_ > recentReqTS_)
    recentReqTS_ = other->recentReqTS_;
  if (other->recentReplyTS_ > recentReplyTS_)
    recentReplyTS_ = other->recentReplyTS_;
 }

Lng32 ExUDRBaseStats::getStatsItem(SQLSTATS_ITEM* sqlStats_item)
{
  sqlStats_item->error_code = 0;
  Lng32 len;
  switch (sqlStats_item->statsItem_id)
  {
    case SQLSTATS_REQ_MSG_CNT:
      sqlStats_item->int64_value = reqMsgCnt_;
      break;
    case SQLSTATS_REQ_MSG_BYTES:
      sqlStats_item->int64_value = reqMsgBytes_;
      break;
    case SQLSTATS_REPLY_MSG_CNT:
      sqlStats_item->int64_value = replyMsgCnt_;
      break;
    case SQLSTATS_REPLY_MSG_BYTES:
      sqlStats_item->int64_value = replyMsgBytes_;
      break;
    case SQLSTATS_UDR_CPU_BUSY_TIME:
      sqlStats_item->int64_value = udrCpuTime_;
      break;
    case SQLSTATS_RECENT_REQ_TS:
      sqlStats_item->int64_value = recentReqTS_;
      break;
    case SQLSTATS_RECENT_REPLY_TS:
      sqlStats_item->int64_value = recentReplyTS_;
      break;
    case SQLSTATS_UDR_SERVER_ID:
      if (sqlStats_item->str_value != NULL)
      {
        len = str_len(UDRServerId_);
        if (len > sqlStats_item->str_max_len)
        {
          len = sqlStats_item->str_max_len;
          sqlStats_item->error_code = EXE_ERROR_IN_STAT_ITEM;
        }
        str_cpy(sqlStats_item->str_value, UDRServerId_, len);
        sqlStats_item->str_ret_len = len;
      }
      break;
    default:
      ExOperStats::getStatsItem(sqlStats_item);
      break;
  }
  return 0;
}



void ExUDRBaseStats::setUDRServerId(const char * serverId, Lng32 maxlen)
{
  if (maxlen >= (sizeof(UDRServerId_)-1))
    maxlen = sizeof(UDRServerId_)-1;
  str_cpy_all(UDRServerId_, serverId, maxlen);
  UDRServerId_[maxlen] = 0;
}


//////////////////////////////////////////////////////////////////
// class ExFastExtractStats
//////////////////////////////////////////////////////////////////
ExFastExtractStats::ExFastExtractStats(NAMemory * heap,
                         ex_tcb *tcb,
                         const ComTdb * tdb)
     : ExOperStats(heap,
                   FAST_EXTRACT_STATS,
                   tcb,
                   tdb),
        buffersCount_(0),
        processedRowsCount_(0),
        errorRowsCount_(0),
        readyToSendBuffersCount_(0),
        sentBuffersCount_(0),
        partitionNumber_(0),
        bufferAllocFailuresCount_(0),
        readyToSendBytes_(0),
        sentBytes_(0)
{}

ExFastExtractStats::ExFastExtractStats(NAMemory * heap)
     : ExOperStats(heap, FAST_EXTRACT_STATS),
       buffersCount_(0),
       processedRowsCount_(0),
       errorRowsCount_(0),
       readyToSendBuffersCount_(0),
       sentBuffersCount_(0),
       partitionNumber_(0),
       bufferAllocFailuresCount_(0),
       readyToSendBytes_(0),
       sentBytes_(0)
{}

void ExFastExtractStats::init(NABoolean resetDop)
{
  ExOperStats::init(resetDop);
  buffersCount_ = 0;
  processedRowsCount_ = 0;
  errorRowsCount_ = 0;
  readyToSendBuffersCount_ = 0;
  sentBuffersCount_ = 0;
  partitionNumber_ = 0;
  bufferAllocFailuresCount_ = 0;
  readyToSendBytes_ = 0;
  sentBytes_ = 0;
}

void ExFastExtractStats::copyContents(ExFastExtractStats* other)
{
  ExOperStats::copyContents(other);

  buffersCount_ = other->buffersCount_;
  processedRowsCount_ = other->processedRowsCount_;
  errorRowsCount_ = other->errorRowsCount_;
  readyToSendBuffersCount_ = other->readyToSendBuffersCount_;
  sentBuffersCount_ = other->sentBuffersCount_;
  partitionNumber_ = other->partitionNumber_;
  bufferAllocFailuresCount_ = other->bufferAllocFailuresCount_;
  readyToSendBytes_ = other->readyToSendBytes_;
  sentBytes_ = other->sentBytes_;
}

void ExFastExtractStats::merge(ExFastExtractStats* other)
{
  ExOperStats::merge(other);

  buffersCount_ += other->buffersCount_;
  processedRowsCount_ += other->processedRowsCount_;
  errorRowsCount_ += other->errorRowsCount_;
  readyToSendBuffersCount_ += other->readyToSendBuffersCount_;
  sentBuffersCount_ += other->sentBuffersCount_;
  partitionNumber_ += other->partitionNumber_;
  bufferAllocFailuresCount_ += other->bufferAllocFailuresCount_;
  readyToSendBytes_ = +other->readyToSendBytes_;
  sentBytes_ = +other->sentBytes_;
}

UInt32 ExFastExtractStats::packedLength()
{
  UInt32 size = ExOperStats::packedLength();

  size += sizeof(buffersCount_);
  size += sizeof(processedRowsCount_);
  size += sizeof(errorRowsCount_);
  size += sizeof(readyToSendBuffersCount_);
  size += sizeof(sentBuffersCount_);
  size += sizeof(partitionNumber_);
  size += sizeof(bufferAllocFailuresCount_);
  size += sizeof(readyToSendBytes_);
  size += sizeof(sentBytes_);
  return size;
}

UInt32 ExFastExtractStats::pack(char * buffer)
{
  UInt32 size = ExOperStats::pack(buffer);
  buffer += size;

  size +=  packIntoBuffer(buffer,buffersCount_);
  size +=  packIntoBuffer(buffer,processedRowsCount_);
  size +=  packIntoBuffer(buffer,errorRowsCount_);
  size +=  packIntoBuffer(buffer,readyToSendBuffersCount_);
  size +=  packIntoBuffer(buffer,sentBuffersCount_);
  size +=  packIntoBuffer(buffer,partitionNumber_);
  size +=  packIntoBuffer(buffer,bufferAllocFailuresCount_);
  size +=  packIntoBuffer(buffer,readyToSendBytes_);
  size +=  packIntoBuffer(buffer,sentBytes_);
  return size;
}

void ExFastExtractStats::unpack(const char* &buffer)
{
  ExOperStats::unpack(buffer);
  unpackBuffer(buffer, buffersCount_);
  unpackBuffer(buffer, processedRowsCount_);
  unpackBuffer(buffer, errorRowsCount_);
  unpackBuffer(buffer, readyToSendBuffersCount_);
  unpackBuffer(buffer, sentBuffersCount_);
  unpackBuffer(buffer, partitionNumber_);
  unpackBuffer(buffer, bufferAllocFailuresCount_);
  unpackBuffer(buffer, readyToSendBytes_);
  unpackBuffer(buffer, sentBytes_);
}

ExOperStats * ExFastExtractStats::copyOper(NAMemory * heap)
{
  ExFastExtractStats* stat = new(heap) ExFastExtractStats(heap);
  stat->copyContents(this);
  return stat;
}

ExFastExtractStats * ExFastExtractStats::castToExFastExtractStats()
{
  return this;
}



const char * ExFastExtractStats::getNumValTxt(Int32 i) const
{
  switch (i)
  {
    case 1:
      return "OperCpuTime";
    case 2:
      return "processedRows";
    case 3:
      return "errorRows";
    case 4:
      return "readyButNotSentBuffers";

  }
  return NULL;
}

Int64 ExFastExtractStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return ExOperStats::getNumVal(i);
    case 2:
      return processedRowsCount_;
    case 3:
      return errorRowsCount_;
    case 4:
      return readyToSendBuffersCount_ - sentBuffersCount_;
    }
  return 0;
}

void ExFastExtractStats::getVariableStatsInfo(char * dataBuffer,
                                       char * dataLen,
                                       Lng32 maxLen)
{
  char *buf = dataBuffer;

  ExOperStats::getVariableStatsInfo(dataBuffer, dataLen, maxLen);
  buf += *((short *) dataLen);

  sprintf(buf,
              "BufferCount: %u processedRowsCount: %u errorRowsCount: %u "
              "readyToSendBuffersCount: %u sentBuffersCount: %u "
              "partition Number: %u bufferAllocFailuresCount: %u "
              "readyToSendBytes: %u sentBytes: %u",
              buffersCount_, processedRowsCount_, errorRowsCount_,
              readyToSendBuffersCount_, sentBuffersCount_,
              partitionNumber_, bufferAllocFailuresCount_,
              readyToSendBytes_, sentBytes_);
  buf += str_len(buf);

  *(short*)dataLen = (short) (buf - dataBuffer);
}

ExProcessStats::ExProcessStats(NAMemory * heap)
  : ExOperStats(heap, PROCESS_STATS)
{
  nid_ = 0;
  pid_ = 0;
  startTime_ = -1;
  numESPsStarted_ = 0;
  numESPsStartupCompleted_ = 0;
  numESPsDeleted_ = 0;
  numESPsBad_ = 0;
  numESPsInUse_ = 0;
  numESPsFree_ = 0;
  init(FALSE);  
}

ExProcessStats::ExProcessStats(NAMemory * heap, 
                   short nid, pid_t pid)
  : ExOperStats(heap, PROCESS_STATS)
{
  nid_ = nid;
  pid_ = pid;
  startTime_ = -1;
  numESPsStarted_ = 0;
  numESPsStartupCompleted_ = 0;
  numESPsDeleted_ = 0;
  numESPsBad_ = 0;
  numESPsInUse_ = 0;
  numESPsFree_ = 0;
  init(FALSE);
}

void ExProcessStats::init(NABoolean resetDop)

{
  exeMemHighWM_ = 0;
  exeMemAlloc_ = 0;
  exeMemUsed_ = 0;
  ipcMemHighWM_ = 0;
  ipcMemAlloc_ = 0;
  ipcMemUsed_ = 0;
  staticStmtCount_ = 0;
  dynamicStmtCount_ = 0;
  openStmtCount_ = 0;
  closeStmtCount_ = 0;
  reclaimStmtCount_ = 0;
  pfsSize_ = 0;
  pfsCurUse_ = 0;
  pfsMaxUse_ = 0;
  sqlOpenCount_ = 0;
  arkfsSessionCount_ = 0;
  delQid_ = FALSE;
  qidLen_ = 0;
  recentQid_ = 0;
}

void ExProcessStats::copyContents(ExProcessStats *stat)
{
  ExOperStats::copyContents(stat);
  char * srcPtr = (char *)stat+sizeof(ExOperStats);
  char * destPtr = (char *)this+sizeof(ExOperStats);
  UInt32 srcLen = sizeof(ExProcessStats)-sizeof(ExOperStats);
  memcpy((void *)destPtr, (void *)srcPtr, srcLen);
  if (stat->recentQid_)
  {
     recentQid_ = new ((NAHeap *)(getHeap())) char[qidLen_+1];
     strcpy(recentQid_, stat->recentQid_);
     delQid_ = TRUE;
  }
}

ExOperStats * ExProcessStats::copyOper(NAMemory * heap)
{
  ExProcessStats * stat =  new(heap) ExProcessStats(heap);
  stat->copyContents(this);
  return stat;
}

ExProcessStats * ExProcessStats::castToExProcessStats()
{
  return this;
}

const char * ExProcessStats::getNumValTxt(Int32 i) const
{
  switch (i)
    {
    case 1:
      return "nodeId";
    case 2:
      return "ProcessId";
    case 3:
      return "exeMemAllocInMB";
    case 4:
      return "ipcMemAllocInMB";
    }
  return NULL;
}

Int64 ExProcessStats::getNumVal(Int32 i) const
{
  switch (i)
    {
    case 1:
      return nid_;
    case 2:
      return pid_;
    case 3:
      return (exeMemAlloc_ >> 20);
    case 4:
      return (ipcMemAlloc_ >> 20);
    }
  return 0;
}

const char * ExProcessStats::getTextVal()
{
  return ExOperStats::getTextVal();
}

UInt32 ExProcessStats::packedLength()
{
  UInt32 size;
  size = ExOperStats::packedLength();
  alignSizeForNextObj(size);
  size += sizeof(ExProcessStats)-sizeof(ExOperStats);
  if (recentQid_ != NULL)
      size += qidLen_;
  return size;
}

UInt32 ExProcessStats::pack(char * buffer)
{
  UInt32 packedLen;
  packedLen = ExOperStats::pack(buffer);
  alignSizeForNextObj(packedLen);
  buffer += packedLen;
  UInt32 srcLen = sizeof(ExProcessStats)-sizeof(ExOperStats);
  char * srcPtr = (char *)this+sizeof(ExOperStats);
  memcpy(buffer, (void *)srcPtr, srcLen);
  if (recentQid_ != NULL)
  {
     buffer += srcLen;
     packedLen +=  packStrIntoBuffer(buffer, recentQid_, qidLen_);
  }
  return packedLen+srcLen;
}

void ExProcessStats::unpack(const char* &buffer)
{
  UInt32 srcLen;
  ExOperStats::unpack(buffer);
  alignBufferForNextObj(buffer);
  srcLen = sizeof(ExProcessStats)-sizeof(ExOperStats);
  char * srcPtr = (char *)this+sizeof(ExOperStats);
  memcpy((void *)srcPtr, buffer, srcLen);
  buffer += srcLen;
  if (recentQid_ != NULL)
  {     
     recentQid_ = new ((NAHeap *)(getHeap())) char[qidLen_+1];
     unpackStrFromBuffer(buffer, recentQid_, qidLen_);
     recentQid_[qidLen_] = '\0';
     delQid_ = TRUE;
  }
}

void ExProcessStats::getVariableStatsInfo(char * dataBuffer,
                          char * dataLen,
                          Lng32 maxLen)
{
  char *buf = dataBuffer;
  sprintf (
       buf,
       "statsRowType: %d nodeId: %d processId: %d startTime: %ld "
       "exeMemHighWMInMB: %ld exeMemAllocInMB: %ld exeMemUsedInMB: %ld "
       "ipcMemHighWMInMB: %ld ipcMemAllocInMB: %ld ipcMemUsedInMB: %ld "
       "pfsAllocSize: %d pfsCurUse: %d pfsMaxUse: %d "
       "numSqlOpens: %d arkfsSessionCount: %d "
       "staticStmtCount: %d dynamicStmtCount: %d "
       "openStmtCount: %d reclaimableStmtCount: %d reclaimedStmtCount: %d "
       "totalESPsStarted: %d totalESPsStartupCompleted: %d " 
       "totalESPsStartupError: %d totalESPsDeleted: %d "
       "numESPsInUse: %d numESPsFree: %d "
       "recentQid: %s ", 
       statType(), nid_, pid_, startTime_, 
       (exeMemHighWM_ >> 20), (exeMemAlloc_ >> 20), (exeMemUsed_ >> 20),
       (ipcMemHighWM_ >> 20), (ipcMemAlloc_ >> 20), (ipcMemUsed_ >> 20),
       pfsSize_, pfsCurUse_, pfsMaxUse_,
       sqlOpenCount_, arkfsSessionCount_,
       staticStmtCount_, dynamicStmtCount_,
       openStmtCount_, closeStmtCount_, reclaimStmtCount_,
       numESPsStarted_, numESPsStartupCompleted_, numESPsBad_,
       numESPsDeleted_, numESPsInUse_, numESPsFree_,
       (recentQid_ != NULL ? recentQid_ : "NULL")
      );
  buf += str_len(buf);
  *(short*)dataLen = (short) (buf - dataBuffer);
}

void ExProcessStats::updateMemStats(NAHeap *exeHeap, NAHeap *ipcHeap)
{
    exeMemHighWM_ = exeHeap->getHighWaterMark();
    exeMemAlloc_ = exeHeap->getTotalSize();
    exeMemUsed_ = exeHeap->getAllocSize();
    ipcMemHighWM_ = ipcHeap->getHighWaterMark();
    ipcMemAlloc_ = ipcHeap->getTotalSize();
    ipcMemUsed_ = ipcHeap->getAllocSize();
}
