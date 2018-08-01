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
******************************************************************************
*
* File:         cluster.C
* Description:  Methods for Cluster and Buckets of a hash join 
*
*               
*               
* Created:      08/14/96
* Language:     C++
*
*
*
*
******************************************************************************
*/

// begining of regular compilation
#include "cluster.h"
#include "memorymonitor.h"
#include "ExStats.h"
#include "ComResourceInfo.h"
#include "logmxevent.h"
#include "SortError.h"

HashBufferHeader::HashBufferHeader()
  : rowCount_(0),
    bucketCount_(0)
{
};

/////////////////////////////////////////////////////////////////////////////

HashBuffer::HashBuffer (Cluster * cluster)
  : cluster_(NULL),
    bufferSize_(0),
    maxRowLength_(0),
    rows_(NULL),
    maxNumFullRowsSer_(0),
    freeSpace_(0),
    currRow_(NULL),
    nextAvailRow_(NULL),
    next_(NULL),
    prev_(NULL),
    heap_(NULL),
    data_(NULL) 
{
  ClusterDB * clusterDb = cluster->clusterDb_;  
  bufferSize_ = clusterDb->bufferSize_;

  data_ = (char *)clusterDb->bufferHeap_->allocateAlignedHeapMemory((UInt32)bufferSize_, 512, FALSE);
      
  if ( !data_ ) return; // memory allocation failed

  // adjust memory usage statistics
  clusterDb->availableMemory_ -= bufferSize_;
  clusterDb->memoryUsed_ += bufferSize_;
  if ( clusterDb->hashOperStats_ ) clusterDb->updateMemoryStats();
  
  // if we did not see pressure yet, the maximum cluster
  // size is as big as the total memory used
  if ( clusterDb->memMonitor_  &&  !clusterDb->sawPressure_ )
    clusterDb->maxClusterSize_ = clusterDb->memoryUsed_;
  
  // initialize this buffer
  init(cluster);

  if (clusterDb->bmoStats_)
    clusterDb->bmoStats_->updateBMOHeapUsage((NAHeap *)clusterDb->collHeap());

};

// A constructor for cases when the HashBuffer is used without a
// Cluster object.
// Currently, this is only used by the UniqueHashJoin (ExUniqueHashJoinTcb).
//
HashBuffer::HashBuffer(ULng32 bufferSize,
                       ULng32 rowSize,
                       NABoolean useVariableLength,
                       CollHeap *heap,
		       ClusterDB * clusterDb,
                       ExeErrorCode * rc)
  : cluster_(NULL),
    bufferSize_(bufferSize),
    maxRowLength_(rowSize),
    isVariableLength_(useVariableLength),
    rows_(NULL),
    maxNumFullRowsSer_(0),
    freeSpace_(0),
    currRow_(NULL),
    nextAvailRow_(NULL),
    next_(NULL),
    prev_(NULL),
    heap_(heap),
    data_(NULL)
{

  // assume success
  *rc = EXE_OK;

  data_ = (char *)((NAHeap *)heap)->allocateAlignedHeapMemory((UInt32)bufferSize, 512, FALSE);
      
  if (!data_) {
    *rc = EXE_NO_MEM_TO_EXEC;
    return;
  };

  if ( clusterDb ) {
    // adjust memory usage statistics
    clusterDb->availableMemory_ -= bufferSize;
    clusterDb->memoryUsed_ += bufferSize;
    if ( clusterDb->hashOperStats_ ) clusterDb->updateMemoryStats();
    
    if (clusterDb->bmoStats_)
      clusterDb->bmoStats_->updateBMOHeapUsage((NAHeap *)clusterDb->collHeap());
  }
  // initialize this buffer

  // Make sure that all rows in buffer are aligned.
  //
  maxRowLength_ = ROUND4(rowSize);

  if(maxRowLength_ < sizeof(HashRow) + sizeof(UInt32)) {
    isVariableLength_ = false;
  }

  // first row starts after the buffer header
  rows_ = data_ + ROUND8(sizeof(HashBufferHeader));
  nextAvailRow_ = rows_;

  // The number of allocated rows from this buffer.
  header_->setRowCount(0);

  // Determine the number of rows that can be allocated from this buffer.
  //
  maxNumFullRowsSer_ =  ((bufferSize_ - ROUND8(sizeof(HashBufferHeader)) - 8 ) 
                         / maxRowLength_);

  freeSpace_ = bufferSize_ - ROUND8(sizeof(HashBufferHeader)) -8;

};

HashBufferSerial::HashBufferSerial (Cluster * cluster)
  : HashBuffer(cluster)
{};

// A constructor for cases when the HashBuffer is used without a
// Cluster object.
// Currently, this is only used by the UniqueHashJoin (ExUniqueHashJoinTcb).
//
/*
HashBufferSerial::HashBufferSerial(ULng32 bufferSize,
                                   ULng32 rowSize,
                                   NABoolean useVariableLength,
                                   CollHeap *heap,
                                   ExeErrorCode * rc)
  : HashBuffer(bufferSize, rowSize, useVariableLength, heap, rc)
{};
*/

/////////////////////////////////////////////////////////////////////////////

void HashBuffer::init(Cluster * cluster) {
  cluster_ = cluster;
  maxRowLength_ = ROUND4(cluster->rowLength_);
  bufferSize_ = cluster_->clusterDb_->bufferSize_;
  isVariableLength_ = cluster->useVariableLength_;
  considerBufferDefrag_ = cluster->considerBufferDefrag_;

  if(maxRowLength_ < sizeof(HashRow) + sizeof(UInt32)) {
    isVariableLength_ = false;
    considerBufferDefrag_ = FALSE;
  }

  // first row starts after the buffer header
  rows_ = data_ + ROUND8(sizeof(HashBufferHeader));
  nextAvailRow_ = rows_;

  header_->setRowCount(0);
 
  maxNumFullRowsSer_ =  ((bufferSize_ - ROUND8(sizeof(HashBufferHeader)) - 8 ) 
                        / maxRowLength_);

  freeSpace_ = bufferSize_ - ROUND8(sizeof(HashBufferHeader)) - 8;
};

/////////////////////////////////////////////////////////////////////////////

HashBuffer::~HashBuffer() {
  if (data_) {
    if ( ! cluster_ ) {
      heap_->deallocateMemory(data_);
      return;
    }
    // NOTE: we do NOT ajust the memory usage statistics for the cluster,
    // because totalClusterSize_ denotes the overall size of a cluster.
    // this is later used in PHASE 3 do decide if we switch the role of
    // the inner and the outer cluster and to determine if hash loops
    // are required.
    // But we adjust the overall availableMemory_, because it denotes
    // the currently available main memory.
    ClusterDB * clusterDb = cluster_->clusterDb_;
    NAHeap *heap = clusterDb->bufferHeap_;
    heap->deallocateHeapMemory(data_);
    clusterDb->availableMemory_ += bufferSize_;
    clusterDb->memoryUsed_ -= bufferSize_;
    if (clusterDb->bmoStats_)
      clusterDb->bmoStats_->updateBMOHeapUsage((NAHeap *)clusterDb->collHeap());
  };
  // unchain buffer from the list of buffers
  if (cluster_) {
    cluster_->bufferPool_ = next_;
    cluster_->numInMemBuffers_-- ; // one less in-memory buffer


#ifdef DO_DEBUG
    if ( ! next_ && ! cluster_->keepRecentBuffer_ ) {  // DEBUG
      char msg[256];
      sprintf(msg, 
		  "NULL bufferPool_ for (%lu %lu). recent %lu inMemBuffs %lu ",
		  (ULng32)cluster_ & 0x0FFF, 
		  (ULng32)cluster_->clusterDb_ & 0x0FFF,
		  (ULng32)cluster_->keepRecentBuffer_ & 0x0FFF,
		  cluster_->numInMemBuffers_);
      // log an EMS event and continue
      SQLMXLoggingArea::logExecRtInfo(NULL,0,msg,cluster_->clusterDb_->explainNodeId_);
    }  

    //ex_assert( next_ || cluster_->numInMemBuffers_ == 0 ||    // ** DEBUG **
      //       cluster_->keepRecentBuffer_ && cluster_->numInMemBuffers_ == 1,
    //       "Buffer pool is empty, but not num-in-mem");
#endif

  }
};

/////////////////////////////////////////////////////////////////////////////


Bucket::Bucket() {
  init();
};


void Bucket::init() {
  innerCluster_ = NULL;
  outerCluster_ = NULL; 
  rowCount_ = 0; 
};
/////////////////////////////////////////////////////////////////////////////

ClusterDB::ClusterDB(HashOperator hashOperator,
		     ULng32 bufferSize,
		     atp_struct * workAtp,
                     Lng32 explainNodeId,
		     short hashTableRowAtpIndex1,
		     short hashTableRowAtpIndex2,
		     ex_expr * searchExpr,
		     Bucket * buckets,
		     ULng32 bucketCount,
		     ULng32 availableMemory,
		     MemoryMonitor * memMonitor,
		     short pressureThreshold,
		     ExExeStmtGlobals * stmtGlobals,
		     ExeErrorCode *rc,
		     NABoolean noOverFlow,
		     NABoolean isPartialGroupBy,
		     unsigned short minBuffersToFlush,
		     ULng32 numInBatch,

		     UInt16 forceOverflowEvery,
		     UInt16 forceHashLoopAfterNumBuffers,
		     UInt16 forceClusterSplitAfterMB,

		     ExSubtask * ioEventHandler,
		     ex_tcb * callingTcb,
		     UInt16 scratchThresholdPct,
		     NABoolean doLog,
		     NABoolean bufferedWrites,
		     NABoolean disableCmpHintsOverflow,
		     ULng32 memoryQuotaMB,
		     ULng32 minMemoryQuotaMB,
		     ULng32 minMemBeforePressureCheck,
		     Float32 bmoCitizenshipFactor,
		     Int32  pMemoryContingencyMB, 
		     Float32 estimateErrorPenalty,
		     Float32 hashMemEstInKBPerNode,
		     ULng32 initialHashTableSize,
		     ExOperStats * hashOperStats
		     )
  : hashOperator_(hashOperator),
    bufferSize_(bufferSize),
    bufferHeap_(NULL),
    workAtp_(workAtp),
    explainNodeId_(explainNodeId),
    hashTableRowAtpIndex1_(hashTableRowAtpIndex1),
    hashTableRowAtpIndex2_(hashTableRowAtpIndex2),
    searchExpr_(searchExpr),
    buckets_(buckets),
    bucketCount_(bucketCount),
    memoryUsed_(0),
    memMonitor_(memMonitor),
    pressureThreshold_(pressureThreshold),
    sawPressure_(FALSE),
    stmtGlobals_(stmtGlobals),
    hashLoop_(FALSE),
    noOverFlow_(noOverFlow),
    isPartialGroupBy_(isPartialGroupBy),
    availableMemory_(availableMemory),
    maxClusterSize_(0),
    clusterToFlush_(NULL),
    clusterToProbe_(NULL),
    clusterToRead_(NULL),
    clusterReturnRightRows_(NULL),
    clusterList_(NULL),
    tempFile_(NULL),
    minBuffersToFlush_(minBuffersToFlush),
    minNumWriteOuterBatch_((UInt16)(numInBatch ? (numInBatch/100) % 100 
				  : minBuffersToFlush)),
    maxNumWriteOuterBatch_((UInt16)(numInBatch/10000)),
    numReadOuterBatch_((UInt16)( numInBatch ? numInBatch % 100 : 
				 minBuffersToFlush ) ),

    forceOverflowEvery_(forceOverflowEvery),
    forceOverflowCounter_(forceOverflowEvery), // initialize counter
    forceHashLoopAfterNumBuffers_(forceHashLoopAfterNumBuffers),
    forceClusterSplitAfterMB_(forceClusterSplitAfterMB),

    ioEventHandler_(ioEventHandler),
    callingTcb_(callingTcb),
    scratchThresholdPct_(scratchThresholdPct),
    sequenceGenerator_(0),
    outerReadBuffers_(NULL),
    doLog_(doLog),
    bufferedWrites_(bufferedWrites),
    disableCmpHintsOverflow_(disableCmpHintsOverflow),
    memoryQuotaMB_(memoryQuotaMB),
    minMemoryQuotaMB_(minMemoryQuotaMB),
    minMemBeforePressureCheck_(minMemBeforePressureCheck),
    bmoCitizenshipFactor_(bmoCitizenshipFactor),
    pMemoryContingencyMB_(pMemoryContingencyMB), 
    estimateErrorPenalty_(estimateErrorPenalty),
    hashMemEstInKBPerNode_(hashMemEstInKBPerNode),

    totalPhase3TimeNoHL_(0),
    maxPhase3Time_(0),
    minPhase3Time_(0), // zero means not set yet!
    numClustersNoHashLoop_(0),
    totalIOCnt_(0),
    earlyOverflowStarted_(FALSE),
    bmoMaxMemThresholdMB_(0),
    hashOperStats_(NULL),
    bmoStats_(NULL),
    initialHashTableSize_(initialHashTableSize),
    scratchIOVectorSize_(0),
    overFlowMode_(SCRATCH_DISK)
{
  if (hashOperStats)
    {
      bmoStats_ = hashOperStats->castToExBMOStats();
      if ( bmoStats_ && bmoStats_->statType() != ExOperStats::BMO_STATS)
	hashOperStats_ = hashOperStats;
      else
	hashOperStats_ = NULL;
    }
  else
    {
      hashOperStats_ = NULL;
      bmoStats_ = NULL;
    }
  // assume success
  *rc = EXE_OK;

  // set up the bufferHeap. The default is collHeap(), which is just
  // the regular statement heap. We want this in DP2. If we are not running
  // in DP2, we set up a seperate heap (see below)
  bufferHeap_ = (NAHeap*)collHeap();

  // we are not running in DP2. Setup our own bufferHeap. We want at least
  // 10 buffers in each block of this heap. Also add a few bytes to the buffer
  // size to account for some memory management overhead.
  bufferHeap_ = new(collHeap()) NAHeap("Buffer Heap",
				       bufferHeap_,
				       10 * ((Lng32)bufferSize_ + 20));

  // These fields are used to ensure that #buckets and #hash-table-entries
  // have no common prime factors (to make even use of the hash table entries)
  evenFactor_ = 0 == bucketCount % 2  ;
  primeFactor_ = bucketCount_ ;
  // If needed (HJ), remove factor of buckets-per-cluster (always a power of 2)
  if ( primeFactor_ )  // skip it for UHJ
    while ( 0 == primeFactor_ % 2 ) primeFactor_ /= 2 ; 
};

/////////////////////////////////////////////////////////////////////////////

ClusterDB::~ClusterDB() {
  // there is no point of checking pending I/O, as either there shouldn't
  // be any pending I/O, or we should aband pending I/Os (by closing the 
  // file at delete) in case of error.
  if (tempFile_) {
    SortError *sortError = tempFile_->getSortError();
    delete tempFile_; // this dtor does not delete the sort error
    if ( sortError ) delete sortError;
    tempFile_ = NULL;
  };

  // deallocate the read buffers
  for ( HashBuffer *tmpB = outerReadBuffers_ ; tmpB; tmpB = outerReadBuffers_){
    outerReadBuffers_ = outerReadBuffers_->getNext() ;    
    tmpB->deallocateData(collHeap()); // must deallocate before calling dtor
    delete tmpB;
    memoryUsed_ -= bufferSize_;
    availableMemory_ += bufferSize_;
  };

  while (clusterList_) {
    Cluster * p = clusterList_->next_;
    Cluster * q;
    if (clusterList_->isInner_)
      q = clusterList_->buckets_->getOuterCluster();
    else
      q = clusterList_->buckets_->getInnerCluster();
    if (q)
      delete q;
    delete clusterList_;
    clusterList_ = p;
  };

  // if we allocated a seperate heap for buffers, get rid of it now
  if (bufferHeap_ != collHeap())
    delete bufferHeap_;
};

/////////////////////////////////////////////////////////////////////////////
// given the time it took some cluster to run phase 3, add to local stats
void ClusterDB::updatePhase3Time(Int64 someClusterTime)
{
  totalPhase3TimeNoHL_ += someClusterTime;
  if ( ! minPhase3Time_ ||   // first time
       minPhase3Time_ > someClusterTime ) // found smaller time
    minPhase3Time_ = someClusterTime ; 
  if ( maxPhase3Time_ < someClusterTime ) maxPhase3Time_ = someClusterTime ;
  ++numClustersNoHashLoop_;
}

//////  Q U O T A  /////////

// Return all the memory quota allocation for this operator to the global pool
// (Not exactly "all"; keep at least minMemoryQuotaMB_ )
void ClusterDB::yieldAllMemoryQuota()
{
  if ( memoryQuotaMB_ == 0 || memoryQuotaMB_ <= minMemoryQuotaMB_ ) return; 

  stmtGlobals_->yieldMemoryQuota( memoryQuotaMB_ - minMemoryQuotaMB_ );

  if ( doLog_ ) { // LOG -- to show that memory was yielded
    char msg[256];
    sprintf(msg, 
		"YIELDED ALL MEMORY ALLOWED: %u MB (%u). Unused pool %u MB",
		memoryQuotaMB_-minMemoryQuotaMB_,
                0,
		stmtGlobals_->unusedMemoryQuota() );
    // log an EMS event and continue
    SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, explainNodeId_);
  }

  memoryQuotaMB_ = minMemoryQuotaMB_ ; 
}

////////
void ClusterDB::YieldQuota(UInt32 memNeeded)
{
  // Convert to MegaBytes, upper ceiling
  ULng32 memNeededMB = memNeeded / ONE_MEG ;
  if ( memNeeded % ONE_MEG ) memNeededMB++ ; // one more

  // Do not get below the minimum quota
  memNeededMB = MAXOF(memNeededMB, minMemoryQuotaMB_);

  // mem quota to yield (could be negative; e.g. with a big flushed cluster)
  Lng32 memToYieldMB = (Lng32) memoryQuotaMB_ - (Lng32) memNeededMB ;

  // if there is no memory to yield - then return 
  if ( memToYieldMB <= 1 ) return; // 1 MB - to avoid thrashing

  stmtGlobals_->yieldMemoryQuota( memToYieldMB );  // Now yield 

  if ( doLog_ ) { // LOG -- to show that memory was yielded
    char msg[256], msg1[64];
    unsigned short id = (unsigned short)((ULong)this & 0x0FFF);
    if ( memoryUsed_ ) sprintf(msg1,"Memory used %d, ",memoryUsed_);
    else msg1[0] = (char) 0;

    sprintf(msg, "%s YIELDED %d MB (%u). %s needed %u MB, unused pool %u",
	    hashOperator_ == HASH_GROUP_BY ? "HGB" : 
	        hashOperator_ == SEQUENCE_OLAP ? "OLAP" : "HJ", 
	    memToYieldMB, id, msg1, memNeededMB,
	    stmtGlobals_->unusedMemoryQuota());

    // log an EMS event and continue
    SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, explainNodeId_);
  }

  memoryQuotaMB_ = memNeededMB ; // adjust the new limit
}

// At the end of HJ phase 1, check quota to see if there's enough for (the
// outer clusters of) phase-2. If not then return TRUE (and some in-memory
// inner may get flushed to free some memory).
NABoolean ClusterDB::checkQuotaOrYield(UInt32 numFlushed, UInt32 maxSizeMB)
{
  if ( memoryQuotaMB_ == 0 ) return FALSE; // memory quota not used

  UInt32 sizeNeededOuter = bufferSize_ * numFlushed * minNumWriteOuterBatch_;
  if ( sizeNeededOuter + memoryUsed_ > memoryQuotaMB_ * ONE_MEG )
    return TRUE ; // need to flush another cluster to free some memory

  UInt32 sizeDesiredOuter = bufferSize_ * numFlushed * maxNumWriteOuterBatch_;
  UInt32 sizeNeededInPhase2 = 
    memoryUsed_ + MAXOF( sizeNeededOuter, sizeDesiredOuter );
  UInt32 sizeNeededInPhase3 = 
    bufferSize_ * numReadOuterBatch_ +
    ONE_MEG * (maxSizeMB + 1) ; // + 1 to round up

  UInt32 memNeeded = MAXOF(sizeNeededInPhase3, sizeNeededInPhase2 ); 

  YieldQuota(memNeeded);
  
  return FALSE;
}

// Return some memory allocation of this operator to the global pool.
// HGB gives theOFList (HJ keeps OF clusters with the regular cluster list)
// Calculate memory needed - max of what is in memory, or the largest on
// disk plus those extra buffers - and yield the rest.
void ClusterDB::yieldUnusedMemoryQuota(Cluster * theOFList, 
				       ULng32 extraBuffers )
{
  if ( memoryQuotaMB_ == 0 ) return; // nothing to yield

  // Need to keep the greater of what is currently used, or the largest
  // flushed (inner) cluster plus a needed hash table

  Int64 maxFlushedClusterSize = 0;   // find max size of flushed cluster 
  ULng32 maxRowCount = 0;
  // HGB uses a seperate list for the overflown clusters
  Cluster * clusters = theOFList ? theOFList : clusterList_ ;

  for (Cluster * cl = clusters ; cl ; cl = cl->getNext() ) {
    if ( cl->getState() == Cluster::FLUSHED && cl->isInner_ ) {
      if ( maxFlushedClusterSize < cl->clusterSize() ) { 
	maxFlushedClusterSize = cl->clusterSize() ;
	maxRowCount = cl->getRowCount();
      }
    }
  }
  // add the size of the needed hash table
  maxFlushedClusterSize += maxRowCount * sizeof(HashTableHeader) * 3 / 2;

  // Needed is max: either what's in memory now, or the largest flushed cluster
  ULng32 memNeeded 
    = MAXOF(memoryUsed_, (ULng32) maxFlushedClusterSize) ;
  // add buffers (either one for the outer for HJ, or one per each new
  // cluster for HGB) 
  memNeeded += extraBuffers * bufferSize_ ; 
  // Convert to MegaBytes, upper ceiling
  ULng32 memNeededMB = memNeeded / ONE_MEG ;
  if ( memNeeded % ONE_MEG ) memNeededMB++ ; // one more

  // Do not get below the minimum quota
  memNeededMB = MAXOF(memNeededMB, minMemoryQuotaMB_);

  // mem quota to yield (could be negative; e.g. with a big flushed cluster)
  Lng32 memToYieldMB = (Lng32) memoryQuotaMB_ - (Lng32) memNeededMB ;

  // if there is no memory to yield - then return 
  if ( memToYieldMB <= 1 ) return; // 1 MB - to avoid thrashing

  stmtGlobals_->yieldMemoryQuota( memToYieldMB );  // Now yield 

  if ( doLog_ ) { // LOG -- to show that memory was yielded
    char msg[256], msg1[64];
    if ( memoryUsed_ ) sprintf(msg1,"Memory used %d, ",memoryUsed_);
    else msg1[0] = (char) 0;

    sprintf(msg, "%s YIELDED %d MB (%u). %s needed %u MB, unused pool %u",
		extraBuffers == 1 ? "HJ" : "HGB", memToYieldMB, 
		0,
                msg1, memNeededMB,
		stmtGlobals_->unusedMemoryQuota());

    // log an EMS event and continue
    SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, explainNodeId_);
  }

  memoryQuotaMB_ = memNeededMB ; // adjust the new limit
}

//
// Return a prime number equal or greater than the given (# clusters)
// (Used to avoid common prime factors with the hash table size)
// (Smallest value returned == 3)
//
static const ULng32 primes[70] = {
  3 /*2*/, 3,	  5,	  7,	 11,  	 13,	 17,	 19,	 23,	 29,
  31,	  37,	 41,	 43,	 47, 	 53,	 59,	 61,	 67,	 71,
  73,	  79,	 83,	 89,	 97, 	101,	103,	107,	109,	113,
  127,	 131,	137,	139,	149, 	151,	157,	163,	167,	173,
  179,	 181,	191,	193,	197, 	199,	211,	223,	227,	229,
  233,	 239,	241,	251,	257, 	263,	269,	271,	277,	281,
  283,	 293,	307,	311,	313, 	317,	331,	337,	347,	349 };

ULng32 ClusterDB::roundUpToPrime(ULng32 noOfClusters)
{
  ULng32 ind = noOfClusters / 5 ; // approximate (floor) index
  if ( ind >= 70 ) return noOfClusters; // handle absurd case: #clusters >= 350
  while ( primes[ind] < noOfClusters ) ind++ ;  // find next prime
  return primes[ind] ;
}

/////////////////////////////////////////////////////////////////////////////
//  Perform checks to find if memory allocation of reqSize bytes is possible
//  Return FALSE if allocation is not possible.
//  Checks made:
//    1. Are we within the memory quota system limits.
//    2. Is the process' memory "crowded".
//    3. Is there a system memory pressure.
//    4. Do the hints from the compiler suggest we start overflow early ?
/////////////////////////////////////////////////////////////////////////////
NABoolean ClusterDB::enoughMemory(ULng32 reqSize, NABoolean checkCompilerHints)
{
  char msg[512]; // for logging messages  

  // For testing overflow only -- Simulate extreme memory conditions
  // Force overflow after every "count" of requests
  if ( forceOverflowEvery_ ) 
    if ( ! --forceOverflowCounter_ ) { 
      sawPressure_ = TRUE;
      forceOverflowCounter_ = forceOverflowEvery_; // reinitialize the counter
      return FALSE;
    }

  // Check the forced memory limit
  if (memoryQuotaMB_ &&  // a forced max memory allowed was set
      // would we exceed the max memory allowed ?
      memoryQuotaMB_ * ONE_MEG < memoryUsed_ + reqSize ) { 

    // calculate how much memory (in MB) is needed
    UInt32 memNeededMB = 1 ;  // usually a small request - just take 1 MB
    if ( reqSize > ONE_MEG ) {
      memNeededMB = 
	(memoryUsed_ + reqSize - memoryQuotaMB_ * ONE_MEG) / ONE_MEG ; 
      if ( reqSize % ONE_MEG ) memNeededMB++ ;    // upper ceiling
    }

    // Try to increase the memory quota (from the global "pool") to meet need
    if ( stmtGlobals_->grabMemoryQuotaIfAvailable(memNeededMB) ) {

      memoryQuotaMB_ += memNeededMB ;  // got it

      // Even though we saw pressure before; having more memory now may mean
      // we can have bigger inner clusters (useful only for HJ in phase 1)
      if ( sawPressure_ && memoryQuotaMB_ * ONE_MEG > maxClusterSize_ )
	maxClusterSize_ = memoryQuotaMB_ * ONE_MEG ;

      if ( doLog_ && memNeededMB > 1 ) { // LOG -- only for more than a buffer
	sprintf(msg, 
		    "GRABBED %u MB (%u). Memory used %u, now allowed %u MB, request size %u, unused pool %u",
		    memNeededMB, 0,
                    memoryUsed_, 
		    memoryQuotaMB_, reqSize,stmtGlobals_->unusedMemoryQuota() );
	// log an EMS event and continue
	SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, explainNodeId_);
      }

    } 
    else { // sorry - not enough memory

      if ( ! sawPressure_ && doLog_ ) { // log first time quota overflow
	sprintf(msg, 
		"QUOTA LIMIT OVERFLOW started. Total memory used %u, quota allowed %u MB",
		memoryUsed_, memoryQuotaMB_ );
	// log an EMS event and continue
	SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, explainNodeId_);
      }

      sawPressure_ = TRUE;
      return FALSE;
    }
  }

/*
  // Check if we are running out of address space or swap space.
  // getUsage() would return TRUE if and only if memory gets crowded (i.e. we 
  // failed at least once to allocate a desired flat segment size, and the 
  // free memory is less than half the total memory allocated.)
  size_t lastSegSize, freeSize, totalSize;
  if ( collHeap()->getUsage(&lastSegSize, &freeSize, &totalSize) ) {
    // Another safety check - are we using at least half of the memory in the
    // last flat segment allocated ?
    if (lastSegSize / 2 < memoryUsed_) {
      if ( ! sawPressure_ && doLog_ ) { // log first time
	sprintf(msg, 
		"USAGE OVERFLOW started. Memory used %u, last seg size %u, free size %u, total size %u",
		memoryUsed_, (ULng32)lastSegSize,
		(ULng32)freeSize, (ULng32)totalSize );
	// log an EMS event and continue
	SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, explainNodeId_);
      }
      sawPressure_ = TRUE;

      return FALSE;
    }
  }
*/

  if (memMonitor_ && memoryUsed_ >= minMemBeforePressureCheck_ ) {

    NABoolean pressure = (memMonitor_->memoryPressure() > pressureThreshold_);

    // the first time we see pressure, we set the sawPressure flag.
    // The total memory used at this point determines the maximum
    // cluster size.
    if (pressure) {
	if ( ! sawPressure_ && doLog_ ) { // first time system memory pressure
	  sprintf(msg, 
		  "PRESSURE OVERFLOW started. Total memory used %u",
		  memoryUsed_);
	  // log an EMS event and continue
	  SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, explainNodeId_);
	 
	}
	sawPressure_ = TRUE;
    }
    if ( pressure ) return FALSE; // cannot allocate memory
    if ( isPartialGroupBy_ ) return  availableMemory_ >= reqSize ;

    // -----------------------------------------------------------------
    // Below - Check compiler hints to trigger a possible early overflow
    // -----------------------------------------------------------------
    // Can be disabled with the CQD EXE_BMO_DISABLE_CMP_HINTS_OVERFLOW

    if ( ! disableCmpHintsOverflow_ ) {
      /*
	Compiler hints:
	================
	Compiler provides two hints to BMO operators.
	
	1. bmoExpectedSize:
	   The compiler's estimate of total memory usage by the BMO operator
	
	2. bmoEstErrorPenalty
	   The fraction penalty factor that can be used at runtime to adjust 
           the estimated bmo size as described below.
	
	Explanation:
	-------------
	bmoEstErrorPenalty can also be viewed as uncertainty penalty factor. 
        The higher the uncertainty of expected memory consumption, 
        the better is to overflow early.
	
	bmoEstErrorPenalty is derived by the following equation:
	
	uncertainty =  { [max potential cardinality - expected cardinality] / 
                                                expected cardinality  } * 100%
	
	bmoEstErrorPenalty is set to 10% if uncertainty <= 100%, 
                           else bmogrowthpercent is set at 25%.
	
	Executor usage:
	---------------
	On Seaquest, operators overflow once memory quota is reached or 
        physical memory pressure is detected. Memory pressure is currently
        disabled but eventually memory pressure detection mechanisms would be
        enabled.
	
	Memory quota limits and memory pressure checks do help enforce limits,
        however BMO operators could utilize compiler hints to trigger overflow
        and behave as a good citizen even before quota limits and memory
        pressure enforcements kick in.
	
	BMO operators often consume small chunks of memory or buffer, in
        repeated cycles until memory quota limit is reached. Compiler hints
        would be exercised each time a buffer allocation is required. However
        the size of buffer allocation is not relevant in the calculation below.
	
	C = Total consumed memory by BMO operator. Initially this is zero.
	
	E = Total estimated size of memory that will be consumed by BMO.
            This is initially the bmoExpectedSize hint given by compiler, 
            but adjusted if C started exceeding the estimate:

 	        E = max(bmoExpectedSize, C * (1+bmoEstErrorPenalty) )

	This allow us to modify the estimate at runtime after receiving
        more data and penalize operators that have the chance of being much
        bigger than initially thought. Note, it's possible for the runtime
        logic to increase the value of bmoEstErrorPenalty on its own after
        certain threshold (e.g C > 10*bmoExpectedSize) but bmoEstErrorPenalty
        should not exceed 1 i.e. 100%.
	
	Z = CitizenshipFactor which is a parameter representing how much of
            the available physical memory an operator can assume for itself
            as compared to leaving it to other potential players. It can have
            the value of 1>= Z > 0. While we can suggest an initial value
            e.g. 0.5 we think this is better to set after tuning based on
            some performance tests. In the future this may also be adjusted
            as input from WMS based on existing concurrency on the system.
	
	M = Available physical memory at that instant.
	
	U = Contingency physical memory (e.g. 10% of Phys memory in the node.
            Point it to keep always phys memory available for executables,
            buffers, non-BMO ops, etc)
	
	m = [E- C]  or in other words the estimated delta memory required
            by BMO to complete processing.
	
	If ( m < Z * (M-U))  then continue memory allocation else overflow.
      */

      // free physical memory in MB
      Float32 M = (Float32) memMonitor_->availablePhyMemKb()/1024;
      // consumed memory in MB
      Float32 C = memoryUsed_ / ONE_MEG; 

      //U : minimum percent free physical memory to be spared for other players 
      //on the node. Could be other processes, etc.
      Float32 U = pMemoryContingencyMB_ ;
      
      // Z: percent free physical memory to assume for myself and thereby
      // allowing reminaing free space for other operators in my esp process.
      // WMS could set this based on concurrency, there by it can increase 
      // the desity of BMO operators residing on the node.
      Float32 Z = bmoCitizenshipFactor_ ;
      
      Float32 m = 0;  //delta memory required to avoid overflow.
      
      // do the following check if HJ still in phase 1.
      if ( checkCompilerHints )
	{
	  Float32 E = hashMemEstInKBPerNode_ / 1024 ; //expected memory consumption
	  
#ifdef FUTURE_WORK
	  //check extreme case first. Expected cannot be more than
	  //available quota.
	  if( memoryQuotaMB_ && E > memoryQuotaMB_ ) 
	    {
	      if ( ! earlyOverflowStarted_ && doLog_ ) { // log first time
		sprintf(msg, 
			"Estimate %ld MB exceeded quota %ld MB: OVERFLOW started. Total memory used %lu",
			(Lng32) E, (Lng32) memoryQuotaMB_,
			memoryUsed_);
		// log an EMS event and continue
		SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, explainNodeId_);
	      }
	      sawPressure_ = TRUE;
	      earlyOverflowStarted_ = TRUE;
	      
	      return FALSE;
	    }
#endif
	  
	  //general comments pasted above for explanation of this field.
	  Float32 estimateErrorPenalty = estimateErrorPenalty_ ;
	  
	  /*
	    adjust expected memory to higher than consumed so that a target
	    expected memory is readjusted based on estimateErrorPenalty. 
	    estimateErrorPenalty is high if confidance level of E is low and
	    vice versa. This in essence creates large delta memory requirement
	    for sort, which in essence may trigger overflow.
	  */
	  if ( C > E ) // consumed memory exceeded the expected -- adjust E
	    {
	      E = C * ( 1 + estimateErrorPenalty ) ;
	      hashMemEstInKBPerNode_ = E * 1024;
	    }
	  
	  Float32 m = E - C;  //delta memory required to avoid overflow.
	  
	  // if delta memory required is more than physical memory available
	  // then overflow.
	  if( m > ( Z * (M -U))) {

	    if ( ! earlyOverflowStarted_ && doLog_ ) { // log first time
	      sprintf(msg,
		      "Hash encountered memory pressure [m > (Z*(M-U))]: memReq m=%f, memFree M=%f, "
		      "memExpected E=%f, memConsumed C=%f, estimateErrorPenalty e=%f,"
		      "memContingency U=%f, citizenshipFactor Z=%f", m,M,E,C,estimateErrorPenalty,U,Z);
	      SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
	    }
	    sawPressure_ = TRUE;
	    earlyOverflowStarted_ = TRUE;
	    return FALSE;
	  }
	}  // if ( checkCompilerHints )
      else
	{
	  // if HJ is in phase 2, memory checks done here is purely
	  // memory requirements against available physical memory.
	  
	  m =  (Float32)reqSize/ONE_MEG;
	  
	  if( m > ( Z * (M - U))) {
	    if ( doLog_ ) {
	      sprintf(msg,
		      "HJ phase 2/3 encountered memory pressure [m > (Z*(M-U))]: memReq m=%f, memFree M=%f, memContingency U=%f, citizenshipFactor Z=%f", m,M,U,Z);
	      SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
	    }
	    return FALSE;
	  }
	} // else -- if ( checkCompilerHints )
    } // if ( ! disableCmpHintsOverflow_ )

    // ---------------------------------------------------------------
    // End of hints' check
    // ---------------------------------------------------------------

    //The following checks any threshold limits set by the user. This
    //check is static in nature and directly controlled by cqds
    //SSD_BMO_MAX_MEM_THRESHOLD_MB if SSD disk type is in use OR
    //EXE_MEMORY_AVAILABLE_IN_MB if disk type is in use.
    if( (memoryUsed_ / ONE_MEG) >= bmoMaxMemThresholdMB_)
      return FALSE;


  }  // if (memMonitor_ && memoryUsed_ >= minMemBeforePressureCheck_ )

  // Always return TRUE, but for Partial HGB there's one more check
  return (!isPartialGroupBy_ || availableMemory_ >= reqSize);
};
/////////////////////////////////////////////////////////////////////////////
// A superset of setClusterToRead, for an outer cluster.
// Also allocates the global list of buffers (in the first time) and 
// initializes the outer cluster to start reading more
NABoolean ClusterDB::setOuterClusterToRead(Cluster * oCluster, 
					   ExeErrorCode * rc)
{
  * rc = EXE_OK;
  // In case oCluster has its own hash buffers allocated, remove them
  oCluster->releaseAllHashBuffers();

  // when called first time for this ClusterDB, allocate the buffer list
  if ( NULL == outerReadBuffers_ ) { 
    // Allocate a list of hash buffers to be kept at the ClusterDB
    for ( Int32 bnum = 0; bnum < numReadOuterBatch_; bnum++ ) {
      HashBuffer * newBuffer = NULL; 
      newBuffer = new(collHeap(), FALSE) 
	HashBuffer(bufferSize_, oCluster->rowLength_, oCluster->useVariableLength_, collHeap(), this, rc );
      // if buffer allocation failed
      if ( !newBuffer || *rc ) {
	if (newBuffer) delete newBuffer;
	if ( ! bnum ) { // we must get at least one buffer, else fail
	  *rc = EXE_NO_MEM_TO_EXEC;
	  return TRUE;
	};
	break; // we'd have a shorter list
      }; // if failed
      memoryUsed_ += bufferSize_; // the above ctor does not add
      availableMemory_ -= bufferSize_;

      // chain buffer into the list
      newBuffer->setNext(outerReadBuffers_) ;
      outerReadBuffers_ = newBuffer ;
    }; // for bnum
  }; // buffer list is allocated

  // Mark each buffer as having zero rows, in case we finish reading in the
  // middle of the buffer list
  for ( HashBuffer * tmpB = outerReadBuffers_; tmpB ; tmpB = tmpB->getNext() )
    tmpB->castToSerial()->clearRowCount();

  // start reading into the first buffer in the list.
  oCluster->nextBufferToRead_ = outerReadBuffers_ ;

  oCluster->completeCurrentRead_ = FALSE;
  oCluster->buffersRead_ = 0;

  setClusterToRead(oCluster);

  return FALSE;
};

////////////////////////////////////////////////////////////////////////

ClusterBitMap::ClusterBitMap(ULng32 size, ExeErrorCode * rc)
  : size_(size) {
    // assume success
    *rc = EXE_OK;
    ULng32 charCount = size_/8;
    if (size_ % 8)
      charCount++;
    bitMap_ = (char *) collHeap()->allocateMemory((size_t)charCount,FALSE);
    if (!bitMap_) {
      *rc = EXE_NO_MEM_TO_EXEC;
      return;
    };

  // initialize bitMap to zero
  for (ULng32 i = 0; i < charCount; i++)
    bitMap_[i] = 0;
};

ClusterBitMap::~ClusterBitMap() {
  if (bitMap_)
    collHeap()->deallocateMemory((void *)bitMap_);
};

void ClusterBitMap::setBit(ULng32 bitIndex) {
  ex_assert ((bitIndex < size_), "ClusterBitMap::setBit() ot of bounds");
  char bitMask = (char) (1 << (bitIndex & 7));
  bitMap_[bitIndex >> 3] |= bitMask;
};

NABoolean ClusterBitMap::testBit(ULng32 bitIndex) { 
  ex_assert ((bitIndex < size_), "ClusterBitMap::testBit() ot of bounds");
  char bitMask = (char) (1 << (bitIndex & 7));
  return (NABoolean)(bitMap_[bitIndex >> 3] & bitMask);
};

/////////////////////////////////////////////////////////////////////////////

// Internal utility: Calculate memory and create a hash table
HashTable * Cluster::createHashTable(UInt32 memForHashTable,  
				     ExeErrorCode * rc,
				     NABoolean checkMemory // enough avail ?
				     )
{
  ex_assert(!hashTable_, "hash table exists already");

  // do we have enough memory for this hash table ?
  if ( checkMemory &&
       !clusterDb_->enoughMemory(memForHashTable) ) return NULL;

  UInt32 hashTableEntryCount = memForHashTable / sizeof(HashTableHeader);

  NABoolean doResize = clusterDb_->hashOperator_ == ClusterDB::HASH_GROUP_BY ;
  NABoolean noHVDups = 
    // only real HJ enforces no-Hash-Value-duplicates in the Hash Table chains
    // HGB has no value duplicates, and X-product uses a single chain
    ! ( clusterDb_->hashOperator_ == ClusterDB::HASH_GROUP_BY ||
	clusterDb_->hashOperator_ == ClusterDB::CROSS_PRODUCT ) ;

  //
  //   create the hash table
  //
  hashTable_ = new(collHeap(), FALSE) 
    HashTable(hashTableEntryCount, 
	      clusterDb_->evenFactor_,   // can #entries be even ? 
	      clusterDb_->primeFactor_,  // #entries not divisible by this PF
	      noHVDups,
	      doResize );

  // If ctor failed, or could not even get a table of MIN_HEADER_COUNT
  // then we return (with error code in case we ignore memory pressure,
  // otherwise no error as we would overflow to rectify the situation.)
  if ( ! checkMemory )
    *rc = EXE_NO_MEM_TO_EXEC; // just in case we return below
  if ( !hashTable_ ) return NULL;
  else if ( hashTable_->noTableAllocated() ) {
    delete hashTable_;
    hashTable_ = NULL;
    return NULL;
  }

  // we got a new hash table. The constructor of HashTable might
  // have adjusted the size of the table. Report the memory usage
  clusterDb_->memoryUsed_ +=
    (hashTable_->getHeaderCount() * sizeof(HashTableHeader));
  if ( clusterDb_->hashOperStats_ ) clusterDb_->updateMemoryStats();

  *rc = EXE_OK; // no error
  if (clusterDb_->bmoStats_)
    clusterDb_->bmoStats_->updateBMOHeapUsage((NAHeap *)clusterDb_->collHeap());
  return hashTable_;
}

Cluster::Cluster(ClusterState state,
		 ClusterDB * clusterDb,
		 Bucket * buckets,
		 ULng32 bucketCount,
		 ULng32 rowLength,
                 NABoolean useVariableLength,
                 NABoolean considerBufferDefrag,
		 short insertAtpIndex,
		 NABoolean isInner,
		 NABoolean bitMapEnable,
		 Cluster * next,
		 ExeErrorCode * rc,
		 HashBuffer * bufferPool)
  : state_(state),
    clusterDb_(clusterDb),
    buckets_(buckets),
    bucketCount_(bucketCount),
    rowLength_(rowLength),
    useVariableLength_(useVariableLength),
    considerBufferDefrag_(considerBufferDefrag),
    insertAtpIndex_(insertAtpIndex),
    isInner_(isInner),
    bitMapEnable_(bitMapEnable),
    next_(next),
    ioPending_(FALSE),
    totalClusterSize_(0),
    outerBitMap_(NULL),
    tempFile_(NULL),
    readHandle_(NULL),
    completeCurrentRead_(FALSE),
    buffersRead_(0),
    hashTable_(NULL),
    rowCount_(0),
    readCount_(0),
    writeIOCnt_(0),
    readIOCnt_(0),
    bufferPool_(bufferPool),
    numInMemBuffers_(0),
    scanPosition_(NULL)
    ,returnOverflowRows_(FALSE)
    ,batchCountDown_(clusterDb->numReadOuterBatch_)
    ,lastSqueezedBuffer_(NULL) // no buffer squeezed yet
    ,dontSplit_(FALSE)
    ,nextBufferToFlush_(NULL)
    ,firstBufferInList_(NULL)
    ,afterLastBufferToFlush_(NULL)
    ,nextBufferToRead_(NULL)
    ,afterLastBufferToRead_(NULL)
    ,numLoops_(0)
    ,startTimePhase3_(0)
    ,keepRecentBuffer_(NULL)
    ,flushMe_(FALSE)
    ,defragBuffer_(NULL)
{
      // assume success
      *rc = EXE_OK;

      if (bufferPool_) {
	// reset the given buffer to this cluster
	bufferPool_->init(this);
	// the cluster got a buffer. Thus, it used memory
	totalClusterSize_ += clusterDb_->bufferSize_;
	numInMemBuffers_++ ; 
      };

      // set up pointers from buckets to clusters
      ULng32 i = 0;
      for (; i < bucketCount_; i++)
	if (isInner) {
	// determine the rowCount_. For the initial clusters this will
	// be 0. But in case of a cluster split, we have to sum up
	// the rows in the corresponding buckets. We do this only for
	// inner clusters. New outer clusters are always empty.
	  buckets_[i].setInnerCluster(this);
	  rowCount_ += buckets_[i].getRowCount();
	}
	else
	  buckets_[i].setOuterCluster(this);
      // in case of the partial group by we allocate one buffer, to
      // make sure we can group at least a few rows. If we can't
      // allocate this one buffer, we give up. In all other cases, the
      // buffer is only allocated, if it really gets a row. Note, that
      // in case of a partial group by we always have only one cluster
      // and therefore allocate only one buffer.
      if (clusterDb_->isPartialGroupBy_) {
	HashBuffer * newBuffer = NULL;
	newBuffer = new(collHeap(), FALSE) HashBuffer(this);
	if (!newBuffer || !newBuffer->getDataPointer() ) {
	  if (newBuffer)
	    delete newBuffer;
	  *rc = EXE_NO_MEM_TO_EXEC;
	  return;
	};
	bufferPool_ = newBuffer;
	totalClusterSize_ += clusterDb_->bufferSize_;
	numInMemBuffers_++;
      };

      // if the initial state is CHAINED (i.e. hash groupby), then allocate a
      // hash table of the initial size (to be resized later, as needed).
      if (state_ == CHAINED) {
	hashTable_ = createHashTable(clusterDb_->initialHashTableSize_, rc);
	if (!hashTable_) return;
      };

      // set up a unique sequence ID for this cluster to use for overflow
      maxSeqIDIndex_ = seqIDIndex_ = 0 ;
      seqID_[seqIDIndex_] = clusterDb_->generateSequenceID();
      seqID_[1] = seqID_[2] = 0 ; // 0 is an invalid seq value

      if (considerBufferDefrag_)
      {
         defragBuffer_ = (char *)clusterDb->bufferHeap_->allocateAlignedHeapMemory((UInt32)rowLength_, 512, FALSE);
}
}; // Cluster::Cluster()

/////////////////////////////////////////////////////////////////////////////

Cluster::~Cluster() {
  releaseAllHashBuffers();

  if (outerBitMap_) {
    delete outerBitMap_;
    outerBitMap_ = NULL;
  };
  if ( readHandle_ ) delete readHandle_;

  removeHashTable();

  // remove references to this cluster from the buckets
  if ( buckets_ ) 
    for (ULng32 i = 0; i < bucketCount_; i++)
      if (isInner_)
	buckets_[i].setInnerCluster(NULL);
      else
	buckets_[i].setOuterCluster(NULL);

  if (defragBuffer_)
  {
     NAHeap *heap = clusterDb_->bufferHeap_;
      heap->deallocateHeapMemory(defragBuffer_);
  }
  defragBuffer_ = NULL;
}

/////////////////////////////////////////////////////////////////////////////

void Cluster::removeHashTable() {
  if (hashTable_) {
    clusterDb_->memoryUsed_ -= 
      (hashTable_->getHeaderCount() * sizeof(HashTableHeader));
    delete hashTable_;
    hashTable_ = NULL;
  };
  if (clusterDb_->bmoStats_)
    clusterDb_->bmoStats_->updateBMOHeapUsage((NAHeap *)clusterDb_->collHeap());
};

/////////////////////////////////////////////////////////////////////////////

// Decide which cluster to flush: Find the FLUSHED cluster with the max
// #buffers, and use that one if that #buffers > minimum ; else pick one of
// the non-flushed clusters, and if none -- use the "last resort" cluster
void ClusterDB::chooseClusterToFlush(Cluster * lastResort)
{
  clusterToFlush_ = NULL ;

  Cluster * maxBuffsFlushedCluster = NULL;
  Cluster * anyNonFlushedCluster = NULL;
  ULng32 maxBuffers = 0;
  NABoolean outersOnly = ! lastResort->isInner_ ; // HJ in phase 2
  unsigned short minNumBuffersToFlush = 
    outersOnly ? minNumWriteOuterBatch_ : minBuffersToFlush_ ;

  // search thru all clusters, find the flushed cluster with max buffers
  // (in case a "flush me" cluster is found, just return this one)
  for ( Cluster * currCluster = clusterList_ ; 
	currCluster ; 
	currCluster = currCluster->next_ ) {

    // When HJ in phase 2 -- only select outer clusters
    if ( outersOnly && currCluster->isInner_ ) continue;

    // Skip clusters with fewer (full) buffers than the minimum
    // (We use "<=" below because the last buffer is typically not full)
    if ( currCluster->numInMemBuffers_ <= minNumBuffersToFlush ) continue;

    if ( currCluster->flushMe_ ) {  // higher priority
      currCluster->flushMe_ = FALSE;
      clusterToFlush_ = currCluster ;
      break;
    }
    
    if ( currCluster->state_ == Cluster::FLUSHED ) {
      if ( maxBuffers < currCluster->numInMemBuffers_ ) {
	maxBuffsFlushedCluster = currCluster;
	maxBuffers = currCluster->numInMemBuffers_ ;
      }
      else // lastResort gets some priority (as its recent buffer is full)
	// (when it is flushed, and has the min # buffers, and 1 <= the max)
	if ( lastResort == currCluster &&
	     lastResort->numInMemBuffers_ + 1 >= maxBuffers ) 
	  maxBuffsFlushedCluster = lastResort;
    } 
    else  // pick a "random" non flushed cluster      
      anyNonFlushedCluster = currCluster ; 
  }
  
  if ( ! clusterToFlush_ )
    clusterToFlush_ = 
      maxBuffsFlushedCluster ? maxBuffsFlushedCluster :
      anyNonFlushedCluster ? anyNonFlushedCluster : 
      lastResort ;

  // Keep most recent buffer if not full
  if ( clusterToFlush_->bufferPool_->castToSerial()->notFull() ) {
    clusterToFlush_->keepRecentBuffer_ = clusterToFlush_->bufferPool_ ;
    // start flushing from the second most recent buffer
    clusterToFlush_->bufferPool_ = clusterToFlush_->bufferPool_->getNext();
    // the buffer to keep would become the new buffer list
    clusterToFlush_->keepRecentBuffer_->setNext(NULL);
  }
  else // in case it is not NULL from a previous flush
    clusterToFlush_->keepRecentBuffer_ = NULL;

  // start with the first (also most recent) buffer in chain
  clusterToFlush_->nextBufferToFlush_ = clusterToFlush_->bufferPool_ ;

#ifdef DO_DEBUG
  if ( doLog_ && clusterToFlush_->seqIDIndex_ > 0 ) {  // DEBUG
    char msg[256];
    sprintf(msg, 
		"chooseClusterToFlush() (%lu %lu). next buff %lu inMemBuffs %lu ",
		(ULng32)clusterToFlush_ & 0x0FFF, 
		(ULng32)this & 0x0FFF,
		(ULng32)clusterToFlush_->nextBufferToFlush_ & 0x0FFF,
		clusterToFlush_->numInMemBuffers_);
    // log an EMS event and continue
    SQLMXLoggingArea::logExecRtInfo(NULL,0,msg,explainNodeId_);
  }  
#endif

}

/////////////////////////////////////////////////////////////////////////////

NABoolean Cluster::insert(atp_struct * newEntry,
			  ex_expr * moveExpr,
			  SimpleHashValue hashValue,
			  ExeErrorCode * rc,
			  NABoolean skipMemoryCheck) {

  // assume success
  *rc = EXE_OK;

  HashRow *dataPointer = NULL;
  
  // make sure that we have a buffer
  if (!bufferPool_) {
    // no buffer means, we don't have rows yet
    ex_assert(!rowCount_,
	      "cluster without a buffer has rows");

    HashBuffer * newBuffer = new(collHeap(), FALSE) HashBuffer(this);
    if ( !newBuffer || !newBuffer->getDataPointer() ) {
      // this is the first buffer for this cluster. If we couldn't
      // allocate it, we give up
      if (newBuffer)
	delete newBuffer;
      *rc = EXE_NO_MEM_TO_EXEC;
      return FALSE;
    };
    bufferPool_ = newBuffer;
    numInMemBuffers_ = 1; // the first in-memory buffer
    totalClusterSize_ += clusterDb_->bufferSize_;
  };

  lastDataPointer_ = NULL; 

  NABoolean defragmented = FALSE;
  // allocate space for the new row in the buffer pool
  if ((dataPointer = bufferPool_->castToSerial()->getFreeRow()) == NULL)
  {
    if (defragBuffer_)
    {
#if defined(_DEBUG)
      assert(defragBuffer_);
#endif
      atp_struct * workAtp = clusterDb_->workAtp_;
      workAtp->getTupp(insertAtpIndex_).setDataPointer(defragBuffer_);

      UInt32 maxDataLen = bufferPool_->getMaxRowLength() - sizeof(HashRow);
      UInt32 rowLen = maxDataLen;
      UInt32 *rowLenPtr = &rowLen;

      if (moveExpr)
      {
        if(moveExpr->eval(newEntry, workAtp, 0, -1, rowLenPtr) == ex_expr::EXPR_ERROR)
          {
            *rc = (ExeErrorCode)(-newEntry->getDiagsArea()->mainSQLCODE());
            return FALSE;
          }
      }
      if(rowLen != maxDataLen)
      {
        UInt32 len = ROUND4(rowLen) + sizeof(HashRow);
        if ((dataPointer = bufferPool_->castToSerial()->getFreeRow(len)) != NULL)
        {
          // we got a row
          defragmented = TRUE;
          char *rowPointer = dataPointer->getData();
          //set work atp (insertAtpIndex_) data address -- needed for hash group by
          workAtp->getTupp(insertAtpIndex_).setDataPointer(rowPointer);
          str_cpy_all(rowPointer,
                      defragBuffer_,
                      rowLen);
          //rows are variable-- set row length
          bufferPool_->castToSerial()->setRowLength(dataPointer, rowLen);

#if (defined(_DEBUG))
          char txt[] = "Cluster::insert";
          sql_buffer_pool::logDefragInfo(txt,bufferPool_->getMaxRowLength(),
                                         ROUND4(rowLen) + sizeof(HashRow),
                                         bufferPool_->getFreeSpace(),
                                         bufferPool_,
                                         bufferPool_->getRowCount());
#endif

        }
      }
    }
  }

  if (dataPointer == NULL)
  {
    // old buffer is full -- allocate a new buffer 
    if ( skipMemoryCheck ||     // first check the memory
	 clusterDb_->enoughMemory(clusterDb_->bufferSize_, isInner_) ) {
      // either we generally do not overflow, or
      // just for this row we should avoid overflow, or
      // we have enough memory to allocate another buffer,
      // so create a new buffer and chain it into the bufferlist
      HashBuffer * newBuffer = new(collHeap(), FALSE) HashBuffer(this);
      if ( !newBuffer || !newBuffer->getDataPointer() ) {
	if (skipMemoryCheck) {
	  // we skipped memory pressure check (e.g., due to no-overflow) 
	  // and we did not get a new buffer, then we must fail
	  *rc = EXE_NO_MEM_TO_EXEC;
	  return FALSE;
	};
	// reset rc in case it was set in the constructor of HashBuffer
	*rc = EXE_OK;
	// set saw pressure as we indeed can not allocate memory
	// fix for CR 10-071012-6254
	clusterDb_->sawPressure_ = TRUE;
	// our estimate for available memory was wrong. We don't
	// have memory left. Select a cluster for flushing and return
	// if we are in DP2 and it is a partial group by, DP2 starts
	// returning partial groups
	if (!clusterDb_->isPartialGroupBy_)
	  clusterDb_->chooseClusterToFlush(this);
	return FALSE;  
      };
      
      totalClusterSize_ += clusterDb_->bufferSize_;
      numInMemBuffers_++;
      // we got a new buffer. Chain it into the list of buffers
      newBuffer->setNext(bufferPool_);
      bufferPool_ = newBuffer;
      
      // allocate space for new row
      dataPointer = bufferPool_->castToSerial()->getFreeRow();
    }
    else {
      // not enough memory avaliable. Select a cluster
      // for flushing
      
      if (!clusterDb_->isPartialGroupBy_)
	clusterDb_->chooseClusterToFlush(this);
      
      return FALSE;
    };
  };

  if (!defragmented)
  {
    char *rowPointer = dataPointer->getData();

    // now we have a slot in a buffer and dataPointer points to it
    // move input row to pool
    atp_struct * workAtp = clusterDb_->workAtp_;
    workAtp->getTupp(insertAtpIndex_).setDataPointer(rowPointer);


    UInt32 maxDataLen = bufferPool_->getMaxRowLength() - sizeof(HashRow);
    UInt32 rowLen = maxDataLen;
    UInt32 *rowLenPtr = &rowLen;

    if (moveExpr) {

      if(moveExpr->eval(newEntry, workAtp, 0, -1, rowLenPtr) == ex_expr::EXPR_ERROR)
        {
          *rc = (ExeErrorCode)(-newEntry->getDiagsArea()->mainSQLCODE());
          return FALSE;
        }
    }
    if(bufferPool_->isVariableLength()) {
      bufferPool_->castToSerial()->setRowLength(dataPointer, rowLen);

      if(rowLen != maxDataLen) {
        bufferPool_->castToSerial()->resizeLastRow(rowLen, dataPointer);
      }
    }
  }

  lastDataPointer_ = dataPointer; // keep location of most recent row

  // set the hash value in the row header
  dataPointer->setHashValueRaw(hashValue);
  // SimpleHashValue * headerHashValue = (SimpleHashValue *)dataPointer;
  // * headerHashValue = hashValue;

  // For hash-groupby, insert the row into the hash chain.
  if ( clusterDb_->hashOperator_ == ClusterDB::HASH_GROUP_BY ) {
    NABoolean resizeNeeded; 

    // We know that we don't have duplicates (each group is unique
    // with respect to the grouping (hash) columns. Just insert
    // the row at the beginning of the chain.
    resizeNeeded = hashTable_->insert(dataPointer);

    if ( resizeNeeded ) {
      NABoolean enoughMem =  
	clusterDb_->enoughMemory(hashTable_->resizeTo());

      ULng32 memAdded = hashTable_->resize( enoughMem );

      // if Hash-Table was resized up, then update memory use
      clusterDb_->memoryUsed_ += memAdded ; 

      // if could not create a new HT, better pick this cluster for a flush
      if ( ! memAdded ) flushMe_ = TRUE;
    }
  }

  rowCount_++;
  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

// initialize the cluster's scratch file and read handler (and the clusterDB
// global scratch file, if needed). Return TRUE if error.
NABoolean Cluster::initScratch(ExeErrorCode * rc)
{
  // First time -- need to create the scratch space for all the clusters
  if ( ! clusterDb_->tempFile_ ) {
    ExExeStmtGlobals * stmtGlobals = clusterDb_->stmtGlobals_ ;
    const ExScratchFileOptions *sfo = stmtGlobals->getScratchFileOptions();
    Lng32 numOfInstances = stmtGlobals->getNumOfInstances();
    Lng32 myInstanceNumber = stmtGlobals->getMyInstanceNumber();
    SortError * sortError = new(collHeap(), FALSE) SortError();

    HashScratchSpace * tempFile = new(collHeap(), FALSE) 
      HashScratchSpace(collHeap(),
		    sortError,
                   clusterDb_->explainNodeId_,
                   clusterDb_->bufferSize_,
                   clusterDb_->getScratchIOVectorSize(),
		    clusterDb_->doLog_,
		    sfo->getScratchMgmtOption());
    
    if ( ! tempFile ){
      *rc = EXE_NO_MEM_TO_EXEC;
      return TRUE;
    };

    if (sortError->getSortError() != 0) {
      *rc = EXE_SORT_ERROR;
      return TRUE;
    };

    tempFile->setScratchDirListSpec(sfo->getSpecifiedScratchDirs());
    tempFile->setNumDirsSpec(sfo->getNumSpecifiedDirs());
    tempFile->setNumEsps(numOfInstances);
    tempFile->setEspInstance(myInstanceNumber);

    tempFile->setPreallocateExtents(sfo->getScratchPreallocateExtents());
    tempFile->setScratchMaxOpens(sfo->getScratchMaxOpensHash());
    tempFile->setScratchDiskLogging(sfo->getScratchDiskLogging());
    tempFile->setIoEventHandler(clusterDb_->ioEventHandler_);
    tempFile->setCallingTcb(clusterDb_->callingTcb_);
    tempFile->setIpcEnvironment(stmtGlobals->getIpcEnvironment());
    tempFile->setScratchThreshold(clusterDb_->scratchThresholdPct_);
    tempFile->setScratchIOVectorSize(clusterDb_->getScratchIOVectorSize());
    tempFile->setScratchOverflowMode(clusterDb_->getScratchOverflowMode());
    clusterDb_->tempFile_ = tempFile ;
  };

  // initialize for this cluster
  tempFile_ = clusterDb_->tempFile_ ;

  readHandle_ =  new(collHeap(), FALSE) ClusterPassBack();

  if ( ! readHandle_ ) {
    *rc = EXE_NO_MEM_TO_EXEC;
    return TRUE;
  };
    
  return FALSE;
}

//Wrapper function on flush() to populate diags area.
//Returns: FALSE - either error, or need to call again
//         TRUE -- flush is done
NABoolean Cluster::flush(ComDiagsArea *&da, CollHeap *heap) {
  ExeErrorCode rc = EXE_OK;
  
  //Flush returning FALSE means either Error or IO_NOT_COMPLETE.
  //if rc != EXE_OK then it is error. 
  if(!flush(&rc)) {
    if(rc != EXE_OK) {
      if(da == NULL) {
        da = ComDiagsArea::allocate(heap);
      }
      *da << DgSqlCode(-rc);
      
      char msg[512];
      if(rc == EXE_SORT_ERROR) {
        char errorMsg[100];
        Lng32 scratchError = 0;
        Lng32 scratchSysError = 0;
        Lng32 scratchSysErrorDetail = 0;

        if(clusterDb_ != NULL) {
          clusterDb_->getScratchErrorDetail(scratchError,
                                 scratchSysError,
                                 scratchSysErrorDetail,
                                 errorMsg);

          snprintf(msg, sizeof(msg), "Scratch IO Error occurred. Scratch Error: %d, System Error: %d, System Error Detail: %d, Details: %s",
              scratchError, scratchSysError, scratchSysErrorDetail, errorMsg);
        }
        else {
          snprintf(msg, sizeof(msg), "Scratch IO Error occurred. clusterDb_ is NULL" );
        }
      } else {
        snprintf(msg, sizeof(msg), "Cluster Flush Error occurred."); 
      }
      
      *da << DgString0(msg);
    }
    return FALSE;
  }
  return TRUE;
}

// Flush the in-memory buffers of this cluster
// Returns: FALSE - either error, or need to call again
//          TRUE -- flush is done
NABoolean Cluster::flush(ExeErrorCode * rc) {
  // assume success
  *rc = EXE_OK;
  RESULT writeStatus = SCRATCH_SUCCESS;
  ULng32 bufferSize = clusterDb_->bufferSize_;

  // ensure that the scratch file was created
  if ( ! tempFile_ )
    if ( initScratch(rc) ) return FALSE; // return if there was an error

#ifdef DO_DEBUG
  if ( clusterDb_->doLog_ && seqIDIndex_ > 0 ) {  // DEBUG
    char msg[256];
    sprintf(msg, 
		"Cluster::flush() (%lu %lu). next buff %lu writeIOCnt %lu ",
		(ULong)this & 0x0FFF, 
		(ULong)clusterDb_ & 0x0FFF,
		(ULong)nextBufferToFlush_ & 0x0FFF,
		writeIOCnt_);
    // log an EMS event and continue
    SQLMXLoggingArea::logExecRtInfo(NULL,0,msg,clusterDb_->explainNodeId_);
  }  
#endif

  // while there are buffers, and the IO is ready, write them to disk
  // ( afterLastBufferToFlush_ is usually NULL ; it's only used by OLAP, where
  // it is guaranteed to point to one of the following buffers.)
  while ( nextBufferToFlush_ != afterLastBufferToFlush_ ) { 

    // are the previous I/Os still in flight, and we can not issue a new I/O ?
    if ( IONotReady(rc) ) return FALSE; // return to scheduler (or error)

    // first make sure that this buffer has rows
    ex_assert( clusterDb_->hashOperator_ == ClusterDB::SEQUENCE_OLAP ||
	       nextBufferToFlush_->getRowCount(), 
	       "flushed an empty buffer" );

    // fire the I/O
    writeStatus = tempFile_->writeThru(nextBufferToFlush_->getDataPointer(),
				       seqID_[seqIDIndex_] );

    if ( writeStatus != SCRATCH_SUCCESS && writeStatus != IO_NOT_COMPLETE ) {
        *rc = EXE_SORT_ERROR;
        return FALSE;
    }

    // and do the remaining book-keeping
    writeIOCnt_++;
    clusterDb_->totalIOCnt_++;
    if ( clusterDb_->hashOperStats_ ) clusterDb_->updateIOStats();

    nextBufferToFlush_ = nextBufferToFlush_->getNext() ;
    
  }  // while ( nextBufferToFlush_ )

  // No more buffers to flush -- ensure that all I/Os are done
  if ( IONotComplete(rc) ) return FALSE; // return to scheduler (or error)

  state_ = FLUSHED;

  // For OLAP -- do not deallocate buffers (anyway bufferPool is not used).
  if ( clusterDb_->hashOperator_ == ClusterDB::SEQUENCE_OLAP )  return TRUE;

  // All writes completed -- remove the written buffers
  while ( bufferPool_->getNext() ) {  // remove all but the last buffer
    delete bufferPool_;
  }

  // if the recent buffer was kept, then the last written buffer can be removed
  // and the kept recent buffer would be used instead
  if ( keepRecentBuffer_ ) {  // make the kept buffer to be the new list
    delete bufferPool_;
    
    bufferPool_ = keepRecentBuffer_;
    keepRecentBuffer_ = NULL ;
  }
  else { // the last buffer was flushed; reuse it as the new buffer list 
    // we are done with the cluster flush. reset the last buffer
    bufferPool_->castToSerial()->clearRowCount();
    
    // increase totalClusterSize_ because bufferPool_ is now empty but
    // will be reused when inserting new rows.
    totalClusterSize_ += bufferSize;
  }
  
  numInMemBuffers_ = 1; // only 1 in-memory buffer now
      
  // Check if  CLUSTER-SPLIT  is needed, and perform the split if possible
  //    return FALSE if error ( *rc != 0 ) or if still processing the most
  //    recent (not full) buffer (the schedule would return us here later)
  if ( checkAndSplit( rc ) ) return FALSE;
  
  return TRUE;  // Done writing this list of buffers.
};  // Cluster::flush()


/////////////////////////////////////////////////////////////////////////////

NABoolean Cluster::spill(ExeErrorCode * rc, NABoolean noAggregates) {
  // assume success
  *rc = EXE_OK;
  NABoolean firstFlushDistinctGroupby = state_ == CHAINED && noAggregates ;

  if ( firstFlushDistinctGroupby && keepRecentBuffer_ ) {
    // first spill for distinct must flush that remaining not-full buffer
    // because it contains groups that were already returned !!
    // So re-chain the recent buffer into the buffer pool to be flushed
    keepRecentBuffer_->setNext(bufferPool_);
    bufferPool_ = keepRecentBuffer_;
    keepRecentBuffer_ = NULL;
    // start with the (just restored) first buffer in chain
    nextBufferToFlush_ = bufferPool_ ;
  }

  if ( flush(rc) ) {
    // the flush is done; reinitialize/renew the hash-table 
    if ( hashTable_->originalSize() ) {
      // the old HT is at the original size; just re-initialize it
      hashTable_->init();
    }
    else {
      // Reallocate a (smaller) hash table.
      removeHashTable();

      // after a flush, a cluster starts with the minimal hash table
      hashTable_ = createHashTable(clusterDb_->initialHashTableSize_, rc);
      if (!hashTable_) return FALSE;
    }

    // If last buffer was kept, add its rows to the new hash-table
    if ( keepRecentBuffer_ ) {
      HashBuffer *hb = bufferPool_;
      HashRow *dataPointer = hb->castToSerial()->getFirstRow();
      for ( ULng32 rowIndex = 0 ;
	    rowIndex < hb->getRowCount() ;
	    rowIndex++ ) {
	hashTable_->insert(dataPointer); // no resize
        dataPointer = hb->castToSerial()->getNextRow(dataPointer);

      }
    }

    if ( firstFlushDistinctGroupby ) {

      ex_assert( 0 == maxSeqIDIndex_ ,"First time spill, index should be 0");
      // For non-blocking, rows up to here were returned up, while the
      // following rows would only be returned when the overflowed cluster 
      // is read from file, hence we keep the two lists seperate 	

      // start a new sequence for the current cluster -- but at same index 0
      // while the previous sequence-num is pushed to index 1 (because later
      // we read first the buffers from index 1)
      maxSeqIDIndex_++ ; // update the max
      seqID_[1 + seqIDIndex_] = seqID_[seqIDIndex_] ; // push up
      seqID_[seqIDIndex_] = clusterDb_->generateSequenceID();
    }
    return TRUE;
  }
  else
    return FALSE;
};

// internal method: Set HL, create bitmap, return TRUE if no memory error
NABoolean Cluster::setHashLoop(ExeErrorCode * rc) {
  if ( clusterDb_->doLog_ && ! clusterDb_->hashLoop_ ) { // first time
    char msg[256];
    sprintf(msg, 
		"HASH LOOP started (%lu %lu). Total memory used %u, cluster size " PF64 " ",
		(ULong)this & 0x0FFF, (ULong)clusterDb_ & 0x0FFF,
		clusterDb_->memoryUsed_, totalClusterSize_ );
    // log an EMS event and continue
     SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, clusterDb_->explainNodeId_);
   
  }  
  clusterDb_->hashLoop_ = TRUE ;
  
  // we might have to allocate the bitmap for the outer cluster;
  // this is only done if the outer cluster doesn't have a
  // bitmap already
  Cluster * oCluster = getOuterCluster();
  if ( oCluster &&  // right-HJ may have an empty outer cluster
       oCluster->bitMapEnable_ && 
       !oCluster->outerBitMap_ ) {
    oCluster->outerBitMap_ =
      new(collHeap(), FALSE) ClusterBitMap(oCluster->rowCount_, rc);
    if (!oCluster->outerBitMap_ || *rc) {
      *rc = EXE_NO_MEM_TO_EXEC;
      return FALSE;
    };
  }

  completeCurrentRead_ = TRUE; // End of a loop is like an end of cluster
  state_ = IN_MEMORY; // the (read part of the) cluster is IN_MEMORY

  if ( IONotComplete(rc) ) return FALSE; // return to scheduler (or error)

  // squeeze out rows of other clusters (not needed for the first sequence)
  if ( seqIDIndex_ < maxSeqIDIndex_ ) // if beyond the first sequence
    if ( squeezeOutRowsOfOtherClusters(rc) ) return FALSE; // return if error

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// internal method: If cluster contains variable length rows, move
// rows belonging to this cluster from original buffers new
// buffers. All rows of this cluster must be moved.  Can reuse buffers
// after they have been processed.
//
// If cluster contains fixed length rows, move rows from last read
// buffer(s) into locations of rows that belong to other clusters
//
// (used only after a cluster split) Works with _multiple_ mixed
// buffers !!  return TRUE if error
/////////////////////////////////////////////////////////////////////////////

NABoolean Cluster::squeezeOutRowsOfOtherClusters(ExeErrorCode * rc)
{
  if(bufferPool_->isVariableLength()) {

    // If the rows in these buffers are variable length, then we
    // cannot do this squeeze operation in place.  We must copy each
    // row from the existing buffers to the new buffers.  However, we
    // can reuse the buffers that we have already processed.

    // The source data
    HashBuffer *oldBuffs = bufferPool_;

    // The destination.  Only rows that belong to this cluster will be
    // moved here.
    HashBuffer *newBuffs = NULL;

    // Keep track of the buffers that have been processed.  They can
    // be reused.
    HashBuffer *freeBuffs = NULL;
  
    // Process all the buffers
    while(oldBuffs) {
      HashBuffer *oldBuff = oldBuffs;

      oldBuff->castToSerial()->fixup();

      // Check each row of the current buffer
      HashRow *row = oldBuff->castToSerial()->getFirstRow();
      for(UInt32 i = 0; i < oldBuff->getRowCount(); i++) {
      
        ULng32 bucketId = row->hashValue() % clusterDb_->bucketCount_;
        
        // If this row belongs to this cluster, copy it to the
        // destination.  Otherwise, ignore it.
        if ( clusterDb_->buckets_[bucketId].getInnerCluster() == this ) {

          // If we need a new buffer, allocate one or take one off of
          // the free list.
          if(!newBuffs || !newBuffs->castToSerial()->notFull()) {
            HashBuffer *newBuff = NULL;
            // If there are free buffers, use one
            if(freeBuffs) {
              newBuff = freeBuffs;
              freeBuffs = freeBuffs->getNext();
              newBuff->setNext(NULL);
          
            } else {
              // Otherwise, we must allocate a new one.
              newBuff = new(collHeap(), FALSE) HashBuffer(this);
              if ( ! newBuff || ! newBuff->getDataPointer() ) {
                *rc = EXE_NO_MEM_TO_EXEC;
                return TRUE;
              }
            }

            // Place new buffer at the head of the new buffers list.
            newBuff->setNext(newBuffs);
            newBuffs = newBuff;
        
          }

          // Copy the row from the source (oldBuff) to the destination
          // (newBuffs).  Resize the row based on the size of the source row.
          HashRow *dstRow = newBuffs->castToSerial()->getFreeRow();
          UInt32 len = oldBuff->castToSerial()->getRowLength(row);
          memcpy((char *)dstRow, (char *)row, len + sizeof(HashRow));
          newBuffs->castToSerial()->resizeLastRow(len, dstRow);
        }
        
        // Move to the next row of the source.
        row = oldBuff->castToSerial()->getNextRow(row);
      }

      // Move to the next buffer of the source.
      // Move the buffer we just finished onto the free list.
      oldBuffs = oldBuff->getNext();
      oldBuff->setNext(freeBuffs);
      freeBuffs = oldBuff;
      freeBuffs->init(this);
    
    }
  
    // If there are any remaining free buffers, delete them.
    while (freeBuffs) {
      HashBuffer *freeBuff = freeBuffs;
      freeBuffs = freeBuffs->getNext();
      delete freeBuff;
    }

    bufferPool_ = newBuffs;
    lastSqueezedBuffer_ = bufferPool_;
    return FALSE;

  } else {
    // If this cluster contains fixed length rows, then we can move
    // rows from the source into vacant slots in the destination.

    // assume lastSqueezedBuffer_ points to the last squeezed buffer in the 
    // buffer list, and the buffers ahead have mixed rows (incl other clusters)
  
    //                                                 lastSqueezedBuffer_
    //                                                         |
    //                                                         V
    //                 ----    ----    ----    ----     ---------------
    // bufferPool_ ->  |  | -> |  | -> |  | -> |  | --> |      RRRRRRR| -> ...
    //                 ----    ----    ----    ----     ---------------

    // When no lastSqueezedBuffer_ exists (e.g., with hash-loop, or if no input
    // rows came after the split), then create an empty one and chain it at the
    // end of the buffers list.
    if ( ! lastSqueezedBuffer_ ) {
      // no memory pressure check
      lastSqueezedBuffer_ = new(collHeap(), FALSE) HashBuffer(this);
      if ( ! lastSqueezedBuffer_ || ! lastSqueezedBuffer_->getDataPointer() ) {
        *rc = EXE_NO_MEM_TO_EXEC;
        return TRUE;
      };
      // chain the new buffer at the end of the buffer pool list
      HashBuffer * checkBuff = bufferPool_;
      HashBuffer * nextBuff = checkBuff->getNext() ;
      while ( nextBuff != NULL ) {
        checkBuff = nextBuff;
        nextBuff = nextBuff->getNext() ;
      }
      checkBuff->setNext(lastSqueezedBuffer_) ;
    }

    // ========================================================================
    // The squeeze algorithm: First find the bottom most slot for the row (either
    // in the lastSqueezedBuffer_, or if full, in the buffer above it), then
    // find the top most row (of this cluster) to move to that slot.
    // If both are found -- move that row to that slot. If not (e.g., reached 
    // the end of the buffer) then rectify and try again.
    // Invariants to keep:
    //  1. lastSqueezedBuffer_ is always in a complete state (i.e., its row count 
    //     is correct; no rows in theat buffer above that count).
    //  2. When returning, bufferPool_ equals lastSqueezedBuffer_ .
    // ========================================================================

    // Rows that belong to this cluster are moved from the fromBuff to the
    // toBuff, and when fromBuff gets empty, it is deallocated.
    HashBuffer * toBuff = lastSqueezedBuffer_ ;
    HashBuffer * fromBuff = bufferPool_ ; // fromBuff is always == bufferPool_

    // invariant - when toSlot/fromRow is NULL, then toRowPos/fromRowPos has
    // the position to check next; when not null, then they have the
    // current position of that slot/row.
    char * toSlot = NULL, * fromRow = NULL;
    UInt32 numConfirmedRowsInBuff = lastSqueezedBuffer_->getRowCount() ;
    Int32 toRowPos = (Int32) toBuff->getRowCount(), 
      fromRowPos = (Int32) fromBuff->getRowCount() - 1;
    const UInt32 rowLength = lastSqueezedBuffer_->getMaxRowLength() ;
    const UInt32 maxRowsInBuffer = lastSqueezedBuffer_->castToSerial()->getMaxNumFullRows();

    //
    // Search for a slot, then for a row to move, then is possible move that
    // row down.
    //
    while ( TRUE ) { // while there are still rows to move

      // ******* find a free toSlot in toBuff *******

      while ( ! toSlot ) {
      
        // update toBuff to next buffer, if needed
        if ( toRowPos >= (Int32) maxRowsInBuffer ) { // is this "to" buffer full?
          lastSqueezedBuffer_ = toBuff ; // toBuff is all squeezed
          lastSqueezedBuffer_->castToSerial()->setRowCount(maxRowsInBuffer); 

          // in some rare case ...
          if ( bufferPool_ == lastSqueezedBuffer_ ) return FALSE;
	
          HashBuffer * checkBuff = bufferPool_;
          HashBuffer * nextBuff = checkBuff->getNext() ;
          while ( nextBuff != lastSqueezedBuffer_ ) {
            checkBuff = nextBuff;
            nextBuff = nextBuff->getNext() ;
          }
          toBuff = checkBuff;
          toRowPos = 0; // start from row position zero
          numConfirmedRowsInBuff = 0;

        }  // if ( toRowPos >= maxRowsInBuffer )   // is this "to" buffer full

        char * toDataPointer = toBuff->getFirstRow() + toRowPos * rowLength ;
        HashRow * row = (HashRow *) toDataPointer;
        ULng32 bucketId = row->hashValue() % clusterDb_->bucketCount_;
        // Are we done ? If end of this batch -- finish up and return
        if ( toBuff == fromBuff &&  // only check when "to" and "from" are same
             ( row->bitSet() ||  // was this new row already checked by "from" ?
               toRowPos >= fromRowPos ) ) { // "from" hasn't checked this row yet	
          // handle case of a row not yet checked by "from"
          if ( ! row->bitSet()  &&
               clusterDb_->buckets_[bucketId].getInnerCluster() == this ) 
            ++numConfirmedRowsInBuff;

          if ( numConfirmedRowsInBuff ) { // there are rows in this buffer
            lastSqueezedBuffer_ = toBuff ; // toBuff is all squeezed
            lastSqueezedBuffer_->castToSerial()->setRowCount(numConfirmedRowsInBuff); 
          }
          else { // another rare case: just crossed over to an empty bufferPool
            delete bufferPool_ ;
          }
          ex_assert ( bufferPool_ == lastSqueezedBuffer_ ,
                      "bufferPool is not the last squeezed buffer");
          return FALSE;

        } else { // find an available slot

          if ( toRowPos >= (Int32) toBuff->getRowCount()
               // this buffer was not full, and we reached over the top row
               ||  // or 
               clusterDb_->buckets_[bucketId].getInnerCluster() != this 
               // This slot was held by another cluster's row; reuse it
               )
            toSlot = toDataPointer;
          else {
            ++toRowPos; // prepare for the next one
            ++numConfirmedRowsInBuff;
          }
        }
      } // while ( ! toSlot )
      
      // **** find the last row (of this cluster) in fromBuff and move it ****

      while ( ! fromRow ) {

        // if no more rows in this buffer, deallocate and use next buffer
        if ( fromRowPos < 0 ) { 
          ex_assert ( bufferPool_ != lastSqueezedBuffer_ ,
                      "Empty bufferPool is the last squeezed buffer");
          delete bufferPool_; // the dtor removes the first buffer in list
          fromBuff = bufferPool_;
          fromRowPos = (Int32) fromBuff->getRowCount() - 1;
          // just in case ...
          if ( fromBuff == lastSqueezedBuffer_ ) return FALSE;
        }

        // If row at from-pos belongs to this cluster -- use it !
        char * fromDataPointer = fromBuff->getFirstRow() + fromRowPos * rowLength ;
        HashRow * row = (HashRow *) fromDataPointer;
        ULng32 bucketId = row->hashValue() % clusterDb_->bucketCount_;
        if ( clusterDb_->buckets_[bucketId].getInnerCluster() == this ) 
          fromRow = fromDataPointer;
        else
          --fromRowPos; // prepare for the next one
	
        // in case the "from" just crossed the "to", in the same buffer
        if ( toBuff == fromBuff && toRowPos >= fromRowPos ) {

          // mark this buffer as last squeezed and return
          if ( numConfirmedRowsInBuff ) { // there are rows in this buffer
            lastSqueezedBuffer_ = toBuff ; // toBuff is all squeezed
            lastSqueezedBuffer_->castToSerial()->setRowCount(numConfirmedRowsInBuff); 
          }
          else { // another rare case: just crossed over to an empty bufferPool
            delete bufferPool_ ;
          }
          ex_assert ( bufferPool_ == lastSqueezedBuffer_ ,
                      "bufferPool is not the last squeezed buffer");
          return FALSE;

        }

        // mark this row to make the "to" stop if needed
        // (this row is either skipped or would be moved)
        row->setBit(TRUE); 

      } // while ( ! fromRow )

      // ***** Move the row into the slot **********

      memcpy(toSlot, fromRow, rowLength );
      
      // in case the toBuff is the last squeezed, or incomplete
      // - then update its row count
      if ( toBuff == lastSqueezedBuffer_ || 
           toRowPos == toBuff->getRowCount() )
        toBuff->castToSerial()->setRowCount(toBuff->getRowCount()+1);
    
      HashRow * row = (HashRow *) toSlot;
      row->setBit(FALSE); // erase the mark in the moved row
      toSlot = fromRow = NULL ; // both were used; reset them
      ++toRowPos; // prepare for the next one
      --fromRowPos; // prepare for the next one
      ++numConfirmedRowsInBuff; // one more row of this cluster in this buffer

    } // while ( TRUE )
  }
} // Cluster::squeezeOutRowsOfOtherClusters()

/////////////////////////////////////////////////////////////////////////////
// Read a cluster. If it is an inner Cluster, read as many buffers as
// possible. If it is an inner and we couldn't read all buffers, set
// hashLoop_ == TRUE (this is for inner Clusters during PHASE_3).
// If it in an outer Cluster, read a batch of buffers (this is for outer
// clusters during PHASE_3). 
/////////////////////////////////////////////////////////////////////////////
NABoolean Cluster::read(ExeErrorCode * rc) {
  // assume success
  *rc = EXE_OK;
  RESULT readStatus = SCRATCH_SUCCESS;
  char * dataPtr = NULL;

  // FIRST -- handle end of (batch) read
  if ( completeCurrentRead_ ) {
    // Synchronize with previously issued READs - ensure that all I/Os are done
    if ( IONotComplete(rc) ) return FALSE; // return to scheduler (or error)

    completeCurrentRead_ = FALSE ; // prepare for next time, if needed

    // this is a regular flushed cluster (i.e., not 
    //             -- an (inner) cluster that was split
    //         or  -- an (outer) cluster from HGB Distinct
    if ( ! maxSeqIDIndex_ ) {

      if ( readHandle_->endOfSequence() )
	state_ = IN_MEMORY; // mark the cluster as IN_MEMORY

      return TRUE;  // this read is done !!
    }

    // Handle an (outer) cluster from HGB Distinct
    if ( ! isInner_ ) {      
      // once we finished the first sequence, return all the new groups 
      if ( seqIDIndex_ < maxSeqIDIndex_ )
	returnOverflowRows_ = TRUE;

      if ( readHandle_->endOfSequence() ) {
	if ( seqIDIndex_ ) {
	  --seqIDIndex_ ; // end of first sequence; go to 2nd/last
	  readHandle_->initCPB(); // start reading from begining of sequence
	}
	else state_ = IN_MEMORY; // 2nd sequence: mark the cluster as IN_MEMORY
      }
      return TRUE;
    }

    // code below: only for handling an inner cluster that had a split 

    // squeeze out rows of other clusters (not needed for the first sequence)
    if ( seqIDIndex_ < maxSeqIDIndex_ ) // if beyond the first sequence
      if ( squeezeOutRowsOfOtherClusters(rc) ) return FALSE; // return if error

    // if reached end of sequence (and there was a split)
    if ( readHandle_->endOfSequence() ) {
      if ( ! seqIDIndex_ ) { // was that the last sequence ?

	state_ = IN_MEMORY; // mark the cluster as IN_MEMORY
	return TRUE;
      }
      if ( seqIDIndex_ == maxSeqIDIndex_ ) // if this was the first sequence
	lastSqueezedBuffer_ = bufferPool_ ; // initialize

      ex_assert( lastSqueezedBuffer_ == bufferPool_,
		 "Last squeezed buffer must be the first in the buffer pool");

      // prepare for next sequence
      --seqIDIndex_;
      readHandle_->initCPB(); // start reading from begining of sequence

    } // if ( readHandle_->endOfSequence() ) and there was a split
  } //  if ( completeCurrentRead_ )
  
  // ****  read several buffers  ********
  while ( TRUE ) { // there are more buffers to read
    HashBuffer * newBuffer = NULL;

    // are all previous I/Os still in flight, and we can not issue a new I/O ?
     if ( IONotReady(rc, (UInt32) seqID_[seqIDIndex_]) ) 
      return FALSE; // return to scheduler (or error)

    if ( isInner_ ) { // get a buffer, or if can't then switch to hash-loop

      // If this is not the first buffer, check if we're out of memory and
      // need to switch to a Hash-Loop. For the first buffer the check is
      // skipped, and we try and allocate that buffer.
      if ( buffersRead_ ) {
	// We need enough space for another buffer (we might need memory for
	// two buffers, because we need one buffer for the outer cluster)
	ULng32 requiredMem = clusterDb_->bufferSize_;
	if ( getOuterCluster() && // avoid case of right join without an outer
	     ! getOuterCluster()->bufferPool_ ) requiredMem *= 2 ;
	// we also need memory to build the hash table for the rows already
	// read plus the potential rows in the next buffer
	requiredMem += getMemorySizeForHashTable(readCount_ +
						 bufferPool_ ? bufferPool_->getRowCount() : 0);

	if ( !clusterDb_->enoughMemory(requiredMem)
	     // For debugging only !! Force HL after reading so many buffers
	     || clusterDb_->forceHashLoopAfterNumBuffers_ == buffersRead_ 
	    ) 
	  // Not enough memory -- we have to use Hash-Loop for this cluster
	  return ( setHashLoop(rc) ); // return FALSE if no-mem error

      }  // if ( buffersRead_ )

      newBuffer = new(collHeap(), FALSE) HashBuffer(this);
      if ( !newBuffer || !newBuffer->getDataPointer() ) {
	if ( ! buffersRead_ ) { // no buffers read yet -- can't even hash loop
	  *rc = EXE_NO_MEM_TO_EXEC;
	  return FALSE;
	};
	// reset rc in case it was set by the constructor of HashBuffer
	*rc = EXE_OK;
	
	state_ = IN_MEMORY; // mark the cluster as IN_MEMORY for the hash loop
	
	// Inner and No memory ==> calls for a Hash Loop
	return ( setHashLoop(rc) );
      };

      // we got a new buffer. Chain it into the bufferlist
      newBuffer->setNext(bufferPool_);
      bufferPool_ = newBuffer;

      dataPtr = newBuffer->getDataPointer() ;

      // if this cluster had a split, and reading its 2nd or 3rd sequence
      // then only read a batch of buffers, not all at once
      if ( seqIDIndex_ < maxSeqIDIndex_ ) {	
	if ( 0 == --batchCountDown_ ) { // only one more left in this batch
	  batchCountDown_ = clusterDb_->numReadOuterBatch_ ; // for next batch
	  // the next read would be the last for this batch
	  completeCurrentRead_ = TRUE ;
	} 
      }

    }  // if ( isInner_ )

    else  {  // *** outer ***
 
     // if no more read buffers are available, then complete this batch
      if ( nextBufferToRead_ == afterLastBufferToRead_ ) {
	completeCurrentRead_ = TRUE;
	return read(rc); // to also check if IO happened to complete
      }

      // only for the case of OLAP with "bounded following" -- need to
      // continue cyclically to the first buffer in the list.
      if ( NULL == nextBufferToRead_ ) {
	ex_assert(firstBufferInList_,"first buffer not set for OLAP");
	nextBufferToRead_ = firstBufferInList_;
      }

      dataPtr = nextBufferToRead_->getDataPointer() ;

      nextBufferToRead_ = nextBufferToRead_->getNext();
    }
      
    // ******  ISSUE THE READ   ********
    readStatus = tempFile_->readThru( dataPtr ,
				      seqID_[seqIDIndex_],
				      readHandle_ );
    
    if ( readStatus == SCRATCH_SUCCESS || readStatus == IO_NOT_COMPLETE ) {
      // update counters; likely ahead of the actual read but that's OK
      buffersRead_++; 
      readIOCnt_++;
      clusterDb_->totalIOCnt_++;
      if ( clusterDb_->hashOperStats_ ) clusterDb_->updateIOStats();
    }
    else { // probably SCRATCH_FAILURE
      NABoolean realFailure = TRUE;
      // check for a special case -- nothing was written to that sequence
      // (e.g., when no more input came immediately after a cluster split)
      // this case is benign -- just complete the read below.
      SortError *sortError = tempFile_->getSortError();
      if ( sortError && readHandle_->endOfSequence() ) 
	realFailure = EInvScrBlockNum != - sortError->getSortError();
      if ( realFailure ) {
	*rc = EXE_SORT_ERROR;
	return FALSE;
      }
    }

    // At end of a sequence
    if ( readHandle_->endOfSequence() ) { 
      if ( isInner_ ) {

	ex_assert( maxSeqIDIndex_ /* split */ || readIOCnt_ == writeIOCnt_ ,
		   "Inner cluster did not read as many buffers as written!");

	// in case we were in a hash-loop (and if split, the last sequence)
	if ( ! seqIDIndex_ ) clusterDb_->hashLoop_ = FALSE ;
      }
      completeCurrentRead_ = TRUE;
    }
    
    if ( completeCurrentRead_ ) 
      return read(rc); // to also check if IO happened to complete
    
  } // WHILE ( TRUE )    
};

//Wrapper function on read() to populate diags area.
//Returns: FALSE - either error, or need to call again
//       TRUE -- flush is done
NABoolean Cluster::read(ComDiagsArea *&da, CollHeap *heap) {
  ExeErrorCode rc = EXE_OK;

  //read returning FALSE means either Error or IO_NOT_COMPLETE.
  //if rc != EXE_OK then it is error. 
  if(!read(&rc)) {
    if(rc != EXE_OK) {
      if(da == NULL) {
       da = ComDiagsArea::allocate(heap);
      }
      *da << DgSqlCode(-rc);
      
      char msg[512];
      if(rc == EXE_SORT_ERROR) {
        char errorMsg[100];
        Lng32 scratchError = 0;
        Lng32 scratchSysError = 0;
        Lng32 scratchSysErrorDetail = 0;
  
        if(clusterDb_ != NULL) {
          clusterDb_->getScratchErrorDetail(scratchError,
                                 scratchSysError,
                                 scratchSysErrorDetail,
                                 errorMsg);
  
          snprintf(msg, sizeof(msg), "Cluster::read Scratch IO Error occurred. Scratch Error: %d, System Error: %d, System Error Detail: %d, Details: %s",
              scratchError, scratchSysError, scratchSysErrorDetail, errorMsg);
        }
        else {
          snprintf(msg, sizeof(msg), "Cluster::read Scratch IO Error occurred. clusterDb_ is NULL" );
        }
      } else {
        snprintf(msg, sizeof(msg), "Cluster::read Error occurred."); 
      }
      
      *da << DgString0(msg);
    }
    return FALSE;
  }
  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

// Return: FALSE - No split needed, or split was done successfully
//         TRUE -- there was an error
NABoolean Cluster::checkAndSplit(ExeErrorCode * rc) 
{
  * rc = EXE_OK;

  if ( ! isInner_         || 
       bucketCount_ <= 1  || 
       dontSplit_ ) return FALSE; // then do not split

  // Check if a Cluster-Split is needed

  // total memory needed == inner cluster + outer cluster buffer + hash table
  Int64 requiredMem = totalClusterSize_ + clusterDb_->bufferSize_ +
    getMemorySizeForHashTable(rowCount_);
  if ( ( requiredMem <= clusterDb_->maxClusterSize_ // need more than max ?
	 || clusterDb_->earlyOverflowStarted_ ) // can't tell max cluster size
       && ( ! clusterDb_->forceClusterSplitAfterMB_  // testing CQD was set ?
	    || requiredMem <= clusterDb_->forceClusterSplitAfterMB_* ONE_MEG ))
    return FALSE ;  // split is not needed

  // do not split if data is significantly skewed (i.e., mostly in 1 bucket)
  for ( UInt32 ib = 0; ib < bucketCount_; ib++)
    // does some single bucket hold more than %80 of the cluster's rows ?
    if ( buckets_[ib].getRowCount() * 5 >= 4 * getRowCount() ) {

      if ( clusterDb_->doLog_ ) { // report that skew
	char msg[256];
	sprintf(msg, 
		    "Skewed data; no cluster split (%lu %lu). Cluster size " PF64 " ",
		    (ULong)this & 0x0FFF, 
		    (ULong)clusterDb_ & 0x0FFF,
		    totalClusterSize_ );
	// log an EMS event and continue
	SQLMXLoggingArea::logExecRtInfo(NULL,0,msg,clusterDb_->explainNodeId_);
      }  
      dontSplit_ = TRUE;
      return FALSE;
    } // if more than %80

  // Can not split if most recent (not full) buffer was kept in memory !
  if ( bufferPool_->getRowCount() > 0 ) { // that was  keepRecentBuffer_
    // set the next buffer to be flushed to be that remaining kept buffer
    nextBufferToFlush_ = bufferPool_ ; 

    // flush that last buffer (note: this is a recursive call!) and return
    return ! flush(rc);
  }    

  // the cluster has only half as many buckets left
  bucketCount_ /= 2;

  // now create a new cluster and chain it into the chain after the
  // current cluster
  next_ = new(collHeap(), FALSE) Cluster(Cluster::FLUSHED,
                                         clusterDb_,
				         &buckets_[bucketCount_],
				         bucketCount_,
				         rowLength_,
                                         useVariableLength_,
                                         considerBufferDefrag_,
				         insertAtpIndex_,
				         isInner_,
				         bitMapEnable_,
			        	 next_,
				         rc);
  
  if ( !next_ || (*rc != EXE_OK) ) {
    if (EXE_OK == *rc) *rc = EXE_NO_MEM_TO_EXEC;
    return TRUE;
  }

  // Update the number of rows in the current cluster: Subtract the rows
  // taken by the new cluster.
  rowCount_ -= next_->rowCount_ ;

  // Calculate the total size of the new cluster based on the ratio of
  // rows in the new cluster to total rows.  Assume that the ratio of
  // rows will reflect the ratio of total size.
  next_->totalClusterSize_ = 
    (next_->rowCount_/(rowCount_ + next_->rowCount_)) * totalClusterSize_;

  // Update the total size of the current cluster: subtract the new's size
  totalClusterSize_ -= next_->totalClusterSize_ ;
  // and add the size of the current buffer (which was flushed and now is empty)
  totalClusterSize_ += clusterDb_->bufferSize_;
  numInMemBuffers_ = 1; // only 1 in-memory buffer now

  // Allocate a buffer for the new cluster. This allocation happens
  // unconditionally, i.e., we don't care if there is enough memory.
  HashBuffer * newBuffer = new(collHeap(), FALSE) HashBuffer(next_);
  if (!newBuffer || !newBuffer->getDataPointer() ) {
    if (newBuffer)  delete newBuffer;
    *rc = EXE_NO_MEM_TO_EXEC;
    return TRUE;
  };
  next_->bufferPool_ = newBuffer;
  next_->numInMemBuffers_ = 1; // only 1 in-memory buffer
  // and add the size of the new buffer 
  next_->totalClusterSize_ += clusterDb_->bufferSize_;

  //
  // Now set the overflow sequences, for both clusters
  //
  // Update the new cluster to have the same previous sequences as the current
  for ( next_->seqIDIndex_ = 0 ; 
	next_->seqIDIndex_ <= seqIDIndex_ ;
	next_->seqIDIndex_++ )
    next_->seqID_[next_->seqIDIndex_] = seqID_[next_->seqIDIndex_] ; 
  next_->maxSeqIDIndex_ = next_->seqIDIndex_ ; // update the max idx
  // A new sequence for the new cluster
  next_->seqID_[next_->seqIDIndex_] = clusterDb_->generateSequenceID();

  // start a new sequence for the current cluster
  maxSeqIDIndex_ = ++seqIDIndex_ ; // increment, and update the max
  seqID_[seqIDIndex_] = clusterDb_->generateSequenceID();

  // Just in case the new cluster gets no more rows, initialize its scratch
  if ( next_->initScratch(rc) ) return TRUE;

  if ( clusterDb_->doLog_ ) { // report that a split happened
    char msg[256];
    sprintf(msg, 
		"Cluster-Split: New/old (%lu/%lu %lu): sizes " PF64 "/" PF64 " rowCount %u/%u seqIDindex %hu",
		(ULong)next_ & 0x0FFF, 
		(ULong)this & 0x0FFF, 
		(ULong)clusterDb_ & 0x0FFF,
		next_->totalClusterSize_, totalClusterSize_, 
		next_->rowCount_,         rowCount_, 
		seqIDIndex_ );
    // log an EMS event and continue
    SQLMXLoggingArea::logExecRtInfo(NULL,0,msg,clusterDb_->explainNodeId_);
  }  

  if ( clusterDb_->hashOperStats_ && clusterDb_->hashOperStats_->castToExHashJoinStats() ) // keep stats about splits (HJ only)
    ((ExHashJoinStats*)clusterDb_->hashOperStats_)->incrClusterSplits();

  return FALSE;
};

/////////////////////////////////////////////////////////////////////////////

NABoolean Cluster::chain(NABoolean forceChaining, ExeErrorCode * rc) {
  // assume success
  *rc = EXE_OK;

  // we can only chain clusters in memory
  ex_assert((state_ == Cluster::IN_MEMORY),
	    "tried to chain a unchainable cluster");

  // if we can't get enough space for the hash table, don't chain
  // the hash table has rowCount_ * 1.5 entries.
  // if we have a readCount_, we are in a hash loop, thus, we don't
  // allocate hash chain headers for all row in the inner table 
  // (rowCount_). Instead, we use readCount_ which tells us how many
  // rows we have read in this loop.
  ULng32 hashTableEntryCount = ( readCount_ ? readCount_ : rowCount_ );

  hashTable_ = createHashTable( getMemorySizeForHashTable(hashTableEntryCount),
				rc, 
				! forceChaining ); // force? -> no mem check
  if (!hashTable_) return FALSE;

  HashBuffer *hb = bufferPool_;

  // If performing a CROSS_PRODUCT operation, reverse the list of
  // buffers to keep the rows in order.
  if (clusterDb_->hashOperator_ == ClusterDB::CROSS_PRODUCT && hb != NULL) {
    HashBuffer *revList = hb;

    hb = hb->getNext();
    revList->setNext(NULL);

    while(hb) {
      HashBuffer *next = hb->getNext();
      hb->setNext(revList);
      revList = hb;
      hb = next;
    }
    hb = revList;
  }

  // scan through all the rows in the bufferPool_ and chain them into the HT
  tupp tupp1 =clusterDb_->workAtp_->getTupp(clusterDb_->hashTableRowAtpIndex1_);
  tupp tupp2 =clusterDb_->workAtp_->getTupp(clusterDb_->hashTableRowAtpIndex2_);

  while (hb) {

    hb->castToSerial()->fixup();

    HashRow *dataPointer = hb->castToSerial()->getFirstRow();
    for ( ULng32 rowIndex = 0 ;
	  rowIndex < hb->getRowCount() ;
	  rowIndex++)
      {
	if (clusterDb_->hashOperator_ == ClusterDB::CROSS_PRODUCT) {
	  hashTable_->insertSingleChain(dataPointer);
	} else {
	  // insert row into hash table 
	  hashTable_->insert(clusterDb_->workAtp_,
			     dataPointer,
                             tupp1,
                             tupp2,
			     clusterDb_->searchExpr_);
	}
        dataPointer = hb->castToSerial()->getNextRow(dataPointer);
      }

    hb = hb->getNext();
  } //  while (hb)

  // the cluster is chained now
  state_ = CHAINED;
  return TRUE;
};

/////////////////////////////////////////////////////////////////////////////

ExeErrorCode Cluster::setUpOuter(ULng32 rowLength,
				 short atpIndex,
				 NABoolean bitMapEnable) {

  // assume success
  ExeErrorCode rc = EXE_OK;

  ex_assert(state_ == FLUSHED, "Only a FLUSHED inner cluster gets an outer");
  ex_assert(isInner_, "can't set up outer Cluster for an outer Cluster");

  ex_assert(bufferPool_->getNext() == NULL, "cluster has too many buffers");
  ex_assert(!bufferPool_->getRowCount(), "empty buffer has positive rowCount");

  // when the last buffer in the cluster is flushed, bufferPool_ is empty
  // but it still counts as part of totalClusterSize_. so we need to subtract
  // the empty buffer.
  totalClusterSize_ -= clusterDb_->bufferSize_;

  // the inner cluster is flushed and contains rows (all rows on temporary
  // file). We need an outer cluster for it.
  Cluster * oCluster = new(collHeap(), FALSE) Cluster(FLUSHED,
					              clusterDb_,
					              buckets_,
					              bucketCount_,
					              rowLength,
                                                      useVariableLength_,
                                                      considerBufferDefrag_,
					              atpIndex,
					              FALSE,  // outer cluster
					              bitMapEnable,
					              NULL,   // no next ptr
					              &rc,
					              bufferPool_);
  
  if (!oCluster) return EXE_NO_MEM_TO_EXEC;

  if (rc) return rc;

  bufferPool_ = NULL;

  for (ULng32 i = 0; i < bucketCount_; i++)
    buckets_[i].setOuterCluster(oCluster);

  return rc;
};

/////////////////////////////////////////////////////////////////////////////

void Cluster::resetForHashLoop() {

  numLoops_++; // finished another iteration

  if ( clusterDb_->hashOperStats_  && clusterDb_->hashOperStats_->castToExHashJoinStats()) // keep stats about hash loops (HJ only)
    ((ExHashJoinStats*)clusterDb_->hashOperStats_)->incrHashLoops();

  // delete all buffers of the inner cluster. It would be nice
  // to keep one buffer, but Cluster::read() always allocates a buffer
  // for inner, i.e. it is not aware of this special case.
  releaseAllHashBuffers();

  // reset the readCount_ of the inner cluster. This is used to
  // determine the size of the hash table in a hash loop. 
  readCount_ = 0;

  // clear lastSqueezedBuffer_ after each hash loop
  lastSqueezedBuffer_ = NULL;

  // remove the hash table
  removeHashTable();

  completeCurrentRead_ = FALSE; // Beginning of a loop is like a new cluster
  state_ = FLUSHED; // now the cluster is not IN_MEMORY

  buffersRead_ = 0;

  // get the outer cluster
  Cluster * oCluster = getOuterCluster();

  if ( oCluster ) { // if not a right HJ with an empty outer
    // reset the outer cluster so that it is read again
    oCluster->readCount_ = 0;
    // reset the file position
    oCluster->readHandle_->initCPB();
  }

  // we don't release the buffer of the outer, since we need it
};

/////////////////////////////////////////////////////////////////////////////
ExClusterStats* Cluster::getStats(CollHeap* heap) {
  ExStatsCounter hashChains;
  if (hashTable_)
  for (ULng32 i = 0; i < hashTable_->getHeaderCount(); i++) {
    ULng32 value = hashTable_->getChainSize(i);
    if (value != 0)
      hashChains.addEntry(value);
  };

  ExClusterStats* stats = new(heap)ExClusterStats(isInner_,
						  bucketCount_,
						  rowCount_,
						  totalClusterSize_,
						  hashChains,
						  writeIOCnt_,
						  readIOCnt_);
  return stats;
};

/////////////////////////////////////////////////////////////////////////////

void Cluster::releaseAllHashBuffers() {
  // release all hashbuffers of this cluster
  while (bufferPool_)
    delete bufferPool_;
  bufferPool_ = NULL;
};

/////////////////////////////////////////////////////////////////////////////

// get the next row in main memory
HashRow * Cluster::advance() {
  ex_assert(scanPosition_, "tried to advance on a non-existent hash buffer");
  HashRow * dataPointer = scanPosition_->castToSerial()->getCurrentRowAndAdvance();
  // If finished this outer buffer, try the next buffer in the list
  if ( !dataPointer && scanPosition_->getNext() 
       && scanPosition_->getNext()->getRowCount() ) {
    scanPosition_ = scanPosition_->getNext();
    scanPosition_->castToSerial()->positionOnFirstRow();
    dataPointer = scanPosition_->castToSerial()->getCurrentRowAndAdvance();
  };
  return dataPointer;
};
HashRow * Cluster::getCurrentRow() {
  ex_assert(scanPosition_, "tried to advance on a non-existent hash buffer");
  HashRow * dataPointer = scanPosition_->castToSerial()->getCurrentRow();
  if ( !dataPointer && scanPosition_->getNext()
       && scanPosition_->getNext()->getRowCount() ) {
    scanPosition_ = scanPosition_->getNext();
    scanPosition_->castToSerial()->positionOnFirstRow();
    dataPointer = scanPosition_->castToSerial()->getCurrentRow();
  };
  return dataPointer;
};

ULng32 Cluster::getMemorySizeForHashTable(ULng32 entryCount)
{
  // see constraints in hash_table.h
  entryCount = MINOF(entryCount, MAX_HEADER_COUNT);
  entryCount = MAXOF(entryCount, MIN_HEADER_COUNT); // just a sanity

  // + %50 for hash chains
  return (entryCount * sizeof(HashTableHeader) * 3 / 2);
}

void Cluster::updateBitMap() {
  ex_assert(outerBitMap_, "no bitmap to update");
  outerBitMap_->setBit(readCount_);
};

NABoolean Cluster::testBitMap() {
  ex_assert(outerBitMap_, "no bitmap to test");
  return (outerBitMap_->testBit(readCount_));
};


#include "ComCextdecs.h"
void IOTimer::resetTimer()
{
  ioStarted_ = FALSE;
  accumTime_ = 0;
};

NABoolean IOTimer::startTimer()
{
  if (ioStarted_)
    return FALSE;
  else
    {
      startTime_ = NA_JulianTimestamp();
      ioStarted_ = TRUE;
      return TRUE;
    }
};

Int64 IOTimer::endTimer()
{
  if (ioStarted_)
    {
      ioStarted_ = FALSE;
      accumTime_ += NA_JulianTimestamp() - startTime_;
    }
  return accumTime_;
};

void ClusterDB::getScratchErrorDetail(Lng32 &scratchError,
                         Lng32 &scratchSysError,
                         Lng32 &scratchSysErrorDetail,
                         char *errorMsg)
{
  SortError *sError = NULL;
  if(tempFile_)
     sError = tempFile_->getSortError();
  
  if(sError)
    {
      scratchError = (Lng32)sError->getSortError();
      scratchSysError = (Lng32)sError->getSysError(); 
      scratchSysErrorDetail = (Lng32)sError->getErrorDetail();
      strcpy(errorMsg, sError->getSortErrorMsg());
    }
}

// Every time totalIOCnt_ grows, update the stats for this operator
void ClusterDB::updateIOStats()
{
  Int64 ioSize = (Int64) bufferSize_ * (Int64) totalIOCnt_ ;
  if (hashOperStats_->castToExHashGroupByStats())
      hashOperStats_->castToExHashGroupByStats()->updIoSize(ioSize);
  else
  if (hashOperStats_->castToExHashJoinStats())
      hashOperStats_->castToExHashJoinStats()->updIoSize(ioSize);
}

// Every time memoryUsed_ grows, update the stats for max memory used by this operator
void ClusterDB::updateMemoryStats()
{
  if (hashOperStats_->castToExHashGroupByStats())
    hashOperStats_->castToExHashGroupByStats()->updMemorySize(memoryUsed_);
  else
  if (hashOperStats_->castToExHashJoinStats())
      hashOperStats_->castToExHashJoinStats()->updMemorySize(memoryUsed_);
}

