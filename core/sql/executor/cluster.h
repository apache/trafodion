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
#ifndef CLUSTER_H
#define CLUSTER_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         cluster.h
 * Description:  Cluster of a hash table for hash grby / hash join
 *               (a cluster is a collection of hash buckets that are
 *               stored in the same overflow file, if needed)
 * Created:      8/14/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "hash_table.h"
#include "CommonStructs.h"

#include "ScratchSpace.h"
#include "ex_exe_stmt_globals.h"
#define END_OF_BUF_LIST ((ULng32) -1)

// To be used as the limits on input size estimates by HJ, HGB
#define MAX_INPUT_SIZE 0x200000000LL
#define MIN_INPUT_SIZE 100000LL


class Cluster;
class ExClusterStats;
class Bucket;
class IOTimer;
class ExBMOStats;

class MemoryMonitor;

#include "HashBufferHeader.h"
#include "ExpError.h"
#include "NAMemory.h"

// forward declarations
class ExScratchFileOptions;
class ClusterDB;
class HashBuffer;
class HashBufferSerial;

/////////////////////////////////////////////////////////////////////////////
// The HashBuffer is written to temporary file
/////////////////////////////////////////////////////////////////////////////
class HashBuffer : public NABasicObject {
  friend class HashBufferSerial;
public:
  HashBuffer (Cluster * cluster);

  // A constructor for cases when the HashBuffer is used without a
  // Cluster object.
  // Currently, this is only used by the UniqueHashJoin (ExUniqueHashJoinTcb).
  //
  HashBuffer (ULng32 bufferSize,
              ULng32 rowSize,
              NABoolean useVariableLength,
              CollHeap *heap,
	      ClusterDB * clusterDb,
              ExeErrorCode * rc);

  ~HashBuffer();

  inline ULng32 getMaxRowLength() const;

  inline HashBuffer * getPrev() const;
  inline void setPrev(HashBuffer * prev);

  inline HashBuffer * getNext() const;
  inline void setNext(HashBuffer * next);

  inline char * getDataPointer() const;
  inline void deallocateData(CollHeap * heap) ;
  inline ULng32 getRowCount() const;
  inline HashBufferHeader * getHeader() const;
  void init(Cluster * cluster);
  inline char * getFirstRow() const;

  // Does this buffer contain variable length or fixed length rows.
  inline NABoolean isVariableLength() const;

  inline NABoolean considerBufferDefrag() const;

  // Cast this instance to a pointer to a HashBufferSerial in order to
  // use the Serial Interface
  inline HashBufferSerial *castToSerial() const;

  inline ULng32 getFreeSpace() const
{
  return freeSpace_;
}

private:

  Cluster * cluster_;             // the cluster to which the buffer belongs
  ULng32 bufferSize_;             // the size of the buffer
  ULng32 maxRowLength_;           // allocated size of a row
  NABoolean isVariableLength_;    // Are the rows in this buffer variable length?
  NABoolean considerBufferDefrag_;
  char * rows_;                   // points to row[0]
  ULng32 maxNumFullRowsSer_;      // the number of full rows that will fit in buffer
  ULng32 freeSpace_;              // amount of space left in buffer
  char * currRow_;                // used for scanning the buffer
  char * nextAvailRow_;           // pointer to next row to allocate
  HashBuffer * next_;             // pointer to next buffer in memory
  HashBuffer * prev_;             // pointer to previous buffer in memory
  CollHeap * heap_ ;              // used when there is no cluster_
  union {
    char * data_;                 // pointer to the buffer itself
    HashBufferHeader * header_;   // another interpretation of the first bytes
  };                              // in the buffer
};

/////////////////////////////////////////////////////////////////////////////
// inline functions of HashBufferHeader
/////////////////////////////////////////////////////////////////////////////
inline ULng32 HashBuffer::getMaxRowLength() const {
  return maxRowLength_; 
};

inline HashBuffer * HashBuffer::getPrev() const {
  return prev_;
};

inline void HashBuffer::setPrev(HashBuffer * prev) {
  prev_ = prev;
};

inline HashBuffer * HashBuffer::getNext() const {
  return next_;
};

inline void HashBuffer::setNext(HashBuffer * next) {
  next_ = next;
};

inline char * HashBuffer::getDataPointer() const {
  return data_;
};

inline void HashBuffer::deallocateData(CollHeap * heap) {
  if ( data_ ) heap->deallocateMemory(data_);
  data_ = NULL;
};


// Access members of Header.
inline ULng32 HashBuffer::getRowCount() const {
  return header_->getRowCount();
};

// Does this buffer contain variable length or fixed length rows.
inline NABoolean HashBuffer::isVariableLength() const {
  return isVariableLength_;
};

inline NABoolean HashBuffer::considerBufferDefrag() const {
  return considerBufferDefrag_;
};

inline char *HashBuffer::getFirstRow() const
{
  return rows_;
};

inline HashBufferHeader * HashBuffer::getHeader() const{
  return header_;
};

// Cast this instance to a pointer to a HashBufferSerial in order to
// use the Serial Interface
inline HashBufferSerial *HashBuffer::castToSerial() const {
  return (HashBufferSerial *)this;
};


// HashBufferSerial - This class contains the methods for the Serial
// Interface to the HashBuffer class.  Using this interface, the
// caller can access rows in the buffer one after the other in series,
// but cannot randomly access any given row based on a row index.
// Currently used by HashJoin and HashGroupBy.
//
class HashBufferSerial : public HashBuffer {
private:
  // Constuctors and Destuctor are private - They are never used.
  HashBufferSerial (Cluster * cluster);

  // A constructor for cases when the HashBuffer is used without a
  // Cluster object.
  // Currently, this is only used by the UniqueHashJoin (ExUniqueHashJoinTcb).
  //
  HashBufferSerial (ULng32 bufferSize,
                    ULng32 rowSize,
                    NABoolean useVariableLength,
                    CollHeap *heap,
                    ExeErrorCode * rc);

  ~HashBufferSerial() {};

public:

  // The serial interface to the HashBuffers.  Rows can be accessed in
  // order (series) from the start of the buffer to the end.  Rows cannot be
  // accessed randomly (via a row index or number) 

  inline void setRowCount(ULng32 cnt);
  // Set rowcount to 0 and reset buffer to be empty.
  inline void clearRowCount();

  // Methods dealing with how many rows can fit in a buffer

  // Get the estimated number of rows per buffer.  It is an estimate
  // since the rows can be variable length
  inline ULng32 getMaxNumFullRows() const;

  // Recalculate the number of rows that can be allocated from this buffer.
  inline ULng32 getRemainingNumFullRows() const;

  // Is this buffer full?
  inline NABoolean notFull() const;

  // Methods for accessing rows in series based on an internal cursor (currRow_)
  inline void positionOnFirstRow();
  // return the next row in the buffer
  inline HashRow *getCurrentRowAndAdvance();


  inline HashRow *getCurrentRow();

  // Methods for accessing rows in series based on an external cursor (the previous row)

  // Return a pointer to the first row,
  inline HashRow *getFirstRow() const;
  // Get the next row after 'currentRow' based on the length of currentRow
  inline HashRow *getNextRow(HashRow *currentRow) const;


  // Allocate a row of size maxRowLength_ from this HashBuffer
  inline HashRow *getFreeRow();

  inline HashRow *getFreeRow(ULng32 spaceNeeded);

  // Resize the last row allocated from this HashBuffer
  inline void resizeLastRow(ULng32 newSize, HashRow *dataPointer);

  // Get row length.  If varLen, get length from row. If fixed, based
  // on maxRowLength_
  inline ULng32 getRowLength(HashRow *row) const;
  // Set the row length in the row.  If row is fixed length, this method does nothing
  inline void setRowLength(HashRow *row, UInt32 len);

  void print() const;

  void fixup();


};


// If this buffer has rows, but the internal data members indicate
// that it is empty, reset the data members to the correct
// positions.
inline void HashBufferSerial::fixup() {

  // Fixup the internal data members if they are in an inconsistent state
  if (getRowCount() > 0 && nextAvailRow_ == rows_) {
    if (isVariableLength()) {
      nextAvailRow_ = rows_;
      for(ULng32 i = 0; i < getRowCount(); i++) {
        HashRow *nextRow = (HashRow *)nextAvailRow_;
        nextRow->checkEye();
        nextAvailRow_ = ((char *)nextRow + nextRow->getRowLength() + sizeof(HashRow));
      }
    } else {
      nextAvailRow_ = rows_ + (getRowCount() * maxRowLength_);
    }
    freeSpace_ = (data_ + bufferSize_) - nextAvailRow_;
  }
}


inline void HashBufferSerial::setRowCount(ULng32 cnt) {

  header_->setRowCount(cnt);

  // Initialize nextAvailRow, fixup will set it to the right value based on cnt.
  nextAvailRow_ = rows_;

  // Fixup the internal data members after changing the row count
  fixup();
};

// Set rowcount to 0 and reset buffer to be empty.
inline void HashBufferSerial::clearRowCount() {
  header_->setRowCount(0);
  freeSpace_ = bufferSize_ - ROUND8(sizeof(HashBufferHeader));
  nextAvailRow_ = rows_;
};



// The serial interface to the HashBuffers.  Rows can be accessed in
// order (series) from the start of the buffer to the end.  Rows cannot be
// accessed randomly (via a row index or number) 


// Methods dealing with how many rows can fit in a buffer

// Get the estimated number of rows per buffer.  It is an estimate
// since the rows can be variable length
inline ULng32 HashBufferSerial::getMaxNumFullRows() const {
  return maxNumFullRowsSer_;
};

// Recalculate the number of rows that can be allocated from this buffer.
inline ULng32 HashBufferSerial::getRemainingNumFullRows() const {
  return (freeSpace_ / maxRowLength_);
};

// Is this buffer full?
inline NABoolean HashBufferSerial::notFull() const 
{
  return (freeSpace_ >= maxRowLength_);
};

// Methods for accessing rows in series based on an internal cursor (currRow_)
inline void HashBufferSerial::positionOnFirstRow() 
{
  currRow_ = (char *)getFirstRow();
};

// return a pointer to the current row and advance the current row cursor.
inline HashRow *HashBufferSerial::getCurrentRowAndAdvance()
{
  // The case of an empty buffer is also handled below because at the first
  // call for this buffer     currRow_ == rows_ == nextAvailRow_
  if(currRow_ >= nextAvailRow_) {
    return NULL;
  } else {
    HashRow *nextRow = (HashRow *)currRow_;
    currRow_ = (char *)getNextRow(nextRow);
    if(nextRow) nextRow->checkEye();
    return nextRow;
  }
};

// return a pointer to the current row and do not advance the current row cursor.
inline HashRow *HashBufferSerial::getCurrentRow()
{
  if(currRow_ >= nextAvailRow_)
  {
     return NULL;
  }
  else
  {
     return (HashRow *)currRow_;
   }
};


// Methods for accessing rows in series based on an external cursor (the previous row)

// Return a pointer to the first row,
inline HashRow *HashBufferSerial::getFirstRow() const
{
  return (HashRow *)rows_;
};

// Return a pointer to the next row
// The value of currentRow, must point to the current row
// of the buffer
inline HashRow *HashBufferSerial::getNextRow(HashRow *currentRow) const
{
  if(!currentRow) {
    return NULL;
  }
  char *row = NULL;
  if (isVariableLength()) {
    row = ((char *)currentRow + currentRow->getRowLength() + sizeof(HashRow));
  } else {
    row = ((char *)currentRow + maxRowLength_);
  }
  if(row >= nextAvailRow_) {
    return NULL;
  } else {
    ((HashRow *)row)->checkEye();
    return (HashRow *)row;
  }
};


// Allocate a row of size maxRowLength_ from this HashBuffer
inline HashRow *HashBufferSerial::getFreeRow() {
  if (freeSpace_ >= maxRowLength_) {
    // we have space, "allocate" a row
    HashRow *nextRow = (HashRow *)nextAvailRow_;
    nextAvailRow_ = nextAvailRow_ + maxRowLength_;
    freeSpace_ -= maxRowLength_;
    header_->incRowCount();
    nextRow->setEye();
    return nextRow;
  } else {
    return NULL;
  }
};

// Allocate a row of size a given length from this HashBuffer
inline HashRow *HashBufferSerial::getFreeRow(ULng32 spaceNeeded)
{
  spaceNeeded = ROUND4(spaceNeeded);  

  if (freeSpace_ >= spaceNeeded)
  {
    // we have space, "allocate" a row
    HashRow *nextRow = (HashRow *)nextAvailRow_;
    nextAvailRow_ = nextAvailRow_ + spaceNeeded;
    freeSpace_ -= spaceNeeded;
    header_->incRowCount();
    nextRow->setEye();
    return nextRow;
  }
  else
  {
    return NULL;
  }
};


// Resize the last row allocated from this HashBuffer
inline void HashBufferSerial::resizeLastRow(ULng32 newSize, HashRow *dataPointer)
{

  dataPointer->checkEye();

  if (isVariableLength()) {
    UInt32 size = newSize + sizeof(HashRow);
    UInt32 oldSize = nextAvailRow_ - (char *)dataPointer;
    if (size <= maxRowLength_ && oldSize == maxRowLength_) {
      nextAvailRow_ = ((char *)dataPointer) + size;
      freeSpace_ = (data_ + bufferSize_) - nextAvailRow_;
    } else {
      // Trying to make row larger.  This is not expected.
      // return false indicating an error.
      ex_assert( 0 , "Trying to resize a row to a larger size");
    }
  } else {
    // Nothing to do for fixed length rows.
  }
  dataPointer->checkEye();
}

// Get row length.  If varLen, get length from row. If fixed, based
// on maxRowLength_
inline ULng32 HashBufferSerial::getRowLength(HashRow *row) const {
  row->checkEye();
  if (isVariableLength()) {
    ULng32 len = row->getRowLength();
    ex_assert(len <= maxRowLength_ - sizeof(HashRow), "Bad rowlength in HashBuffer");
    return len;
  } else {
    return maxRowLength_ - sizeof(HashRow);
  }
}

// Set the row length in the row.  If row is fixed length, this method does nothing
inline void HashBufferSerial::setRowLength(HashRow *row, UInt32 len) {
  row->checkEye();
  if (isVariableLength()) {
    ex_assert(len <= maxRowLength_ - sizeof(HashRow), "Bad rowlength in HashBuffer");
    row->setRowLength(len);
  }
  row->checkEye();
}



/////////////////////////////////////////////////////////////////////////////
// A Hash Bucket. Several Buckets might share a Cluster
/////////////////////////////////////////////////////////////////////////////
class Bucket {
public:
  Bucket();
   ~Bucket() {};
  void init();
  inline ULng32 getRowCount() const;
  inline Cluster * getInnerCluster () const;
  inline Cluster * getOuterCluster () const;
  inline NABoolean insert (atp_struct * newEntry,
		           ex_expr * moveExpr,
			   SimpleHashValue hashValue,
			   ExeErrorCode * rc,
			   NABoolean skipMemoryCheck = FALSE);
  inline void setInnerCluster (Cluster * innerCluster);
  inline void setOuterCluster (Cluster * outerCluster);
private:
  Cluster * innerCluster_;    // corresponding inner Cluster
  Cluster * outerCluster_;    // corresponding outer Cluster
  ULng32 rowCount_;    // # of inner Cluster rows in this bucket
};

/////////////////////////////////////////////////////////////////////////////
// inline functions for Bucket
/////////////////////////////////////////////////////////////////////////////
inline ULng32 Bucket::getRowCount() const {
  return rowCount_;
};

inline Cluster * Bucket::getInnerCluster () const {
  return innerCluster_;
};

inline Cluster * Bucket::getOuterCluster () const {
  return outerCluster_;
};

inline void Bucket::setInnerCluster (Cluster * innerCluster) {
  innerCluster_ = innerCluster;
};

inline void Bucket::setOuterCluster (Cluster * outerCluster) {
  outerCluster_ = outerCluster;
};

/////////////////////////////////////////////////////////////////////////////
// The ClusterDB (cluster description block) contains all information
// common to all clusters.
/////////////////////////////////////////////////////////////////////////////
class ClusterDB : public NABasicObject {
  friend class Cluster;
  friend class HashBuffer;
  friend class ExSequenceTcb;
public:
  enum HashOperator {
    HASH_JOIN,
    HYBRID_HASH_JOIN,
    ORDERED_HASH_JOIN,
    UNIQUE_HASH_JOIN,
    HASH_GROUP_BY,
    CROSS_PRODUCT,
    SEQUENCE_OLAP // not a hash operator; but uses ClusterDB and Cluster 
  };
  ClusterDB (HashOperator hashOperator,
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
	     ExeErrorCode * rc,
	     NABoolean noOverFlow = FALSE,
	     NABoolean isPartialGroupBy = FALSE,
	     unsigned short minBuffersToFlush = 1,
	     ULng32 numInBatch = 0,

	     UInt16 forceOverflowEvery = FALSE,
	     UInt16 forceHashLoopAfterNumBuffers = FALSE,
	     UInt16 forceClusterSplitAfterMB = FALSE,

	     ExSubtask *ioEventHandler_ = NULL,
	     ex_tcb * callingTcb_ = NULL,
	     UInt16 scratchThresholdPct_ = 10,

	     NABoolean doLog = FALSE,
	     NABoolean bufferedWrites = FALSE,
	     NABoolean disableCmpHintsOverflow = TRUE,
	     ULng32 memoryQuotaMB = 0, // i.e. no limit
	     ULng32 minMemoryQuotaMB = 0, // don't go below this min

	     ULng32 minMemBeforePressureCheck = 0, // min before check
	     Float32 bmoCitizenshipFactor = 0,
	     Int32  pMemoryContingencyMB = 0, 
	     Float32 estimateErrorPenalty = 0,
	     Float32 hashMemEstInKBPerNode = 0,
	     ULng32 initialHashTableSize = 0, // default not resizable
	     ExOperStats * hashOperStats = NULL
);
  ~ClusterDB();

  inline Cluster * getClusterList() const;
  inline void setClusterList(Cluster * clusterList);
  inline Cluster * getClusterToFlush() const;
  inline void setClusterToFlush(Cluster * cluster);
  void setBMOMaxMemThresholdMB(UInt16 mm)
      { bmoMaxMemThresholdMB_ = mm; }
  inline Cluster * getClusterToProbe() const;
  inline void setClusterToProbe(Cluster * cluster);
  inline Cluster * getClusterToRead() const;
  inline void setClusterToRead(Cluster * cluster);
  NABoolean setOuterClusterToRead(Cluster * cluster, ExeErrorCode * rc);
  inline Cluster * getClusterReturnRightRows() const;
  inline void setClusterReturnRightRows(Cluster * cluster);
  inline NABoolean isHashLoop() const { return hashLoop_; }
  inline Int64 getMemoryHWM() const;
  inline ULng32 getBufferSize() const { return bufferSize_; }
  void setScratchIOVectorSize(Int16 vectorSize)
  {
    scratchIOVectorSize_ = vectorSize;
  }
  Int32 getScratchIOVectorSize() { return scratchIOVectorSize_;}
  void setScratchOverflowMode(ScratchOverflowMode ovMode)
  { overFlowMode_ = ovMode;}
  ScratchOverflowMode getScratchOverflowMode(void)
  { return overFlowMode_;}
  
  void chooseClusterToFlush(Cluster * lastResort);

  HashScratchSpace *getScratch() const;

  inline NABoolean sawPressure() const { return sawPressure_; }

  inline Int64 totalPhase3TimeNoHL() { return totalPhase3TimeNoHL_; }
  inline Int64 maxPhase3Time() { return maxPhase3Time_; }
  inline Int64 minPhase3Time() { return minPhase3Time_; }
  inline ULng32 numClustersNoHashLoop() 
  { return numClustersNoHashLoop_; }

  void updatePhase3Time(Int64 someClusterTime);

  // Yield all the memory quota allocated
  void yieldAllMemoryQuota();

  void YieldQuota(UInt32 memNeeded); // internal routine

  // Check if not enough free quota, else yield excess
  NABoolean checkQuotaOrYield(UInt32 numFlushed, UInt32 maxSizeMB);

  // Yield un-needed memory quota
  void yieldUnusedMemoryQuota(Cluster * theOFList = NULL, 
			 ULng32 extraBuffers = 1);

  ULng32 memoryQuotaMB() { return memoryQuotaMB_ ; }

  // each cluster overflows into another sequence; sometimes a cluster needs
  // several sequences (after cluster-split, or for DISTINCT HGB)
  UInt32 generateSequenceID() { return ++sequenceGenerator_; }
  
  void getScratchErrorDetail(Lng32 &scratchError,
                    Lng32 &scratchSysError,
                    Lng32 &scratchSysErrorDetail,
                    char *errorMsg);
  

  static ULng32 roundUpToPrime(ULng32 noOfClusters);

  NABoolean enoughMemory(ULng32 size, NABoolean checkCompilerHints = FALSE);  

private:

  void updateMemoryStats();
  void updateIOStats();

  HashOperator hashOperator_;         // indicates the type of the tcb
				      // using this clusterDB
  NABoolean hashLoop_;                // are we in a hash loop (PHASE_3)
  NABoolean isPartialGroupBy_;        // for hash grouping
  NABoolean noOverFlow_;              // if true, we do not overflow
				      // if set, all memory related info is
				      // ignored and we always try to allocate
				      // new buffers in main memory. If the
				      // allocation is not sucessfull, we die
  ULng32 bufferSize_;          // global size of hash buffers
  NAHeap * bufferHeap_;               // keep the I/O buffers for all the
                                      // clusters in a separate heap. This
				      // should help to reduce fragmentation.
  atp_struct * workAtp_;              // global work Atp
  Lng32 explainNodeId_;                // add to any EMS RT messages.

  // the following three data members are only used for hash join and hash
  // table materialize. They are required for sorted insert of a row into
  // a hash chain. HGB does not need this, because every row (group) is unique
  short hashTableRowAtpIndex1_;       // index into atp for serach and move expr
  short hashTableRowAtpIndex2_;
  ex_expr * searchExpr_;              // to insert a row in the hash table

  Bucket * buckets_;                  // global Buckets array
  ULng32 bucketCount_;         // total number of buckets
  ULng32 availableMemory_;     // for the HJ (in bytes)
  ULng32 memoryUsed_;          // main memory used for buffers and HT
  // this is different from Cluster.totalClusterSize_. memoryUsed_ tells us how
  // much main memory the operator uses right now. Cluster.totalClusterSize_
  // tells us the size of a cluster!
  ExOperStats * hashOperStats_;       // stats for this hash operator
  ULng32 totalIOCnt_;          // total IO ( # buffers read or writen )
  MemoryMonitor * memMonitor_;        // for dynamic memory management
  short pressureThreshold_;
  NABoolean sawPressure_;             // set when we see pressure the
                                      // first time
  ExExeStmtGlobals * stmtGlobals_;    // for dynamic memory allocation
  ULng32 maxClusterSize_;      // in bytes
  Cluster * clusterToFlush_;          // cluster which is to be flushed
  Cluster * clusterToProbe_;          // cluster which is to be probed
  Cluster * clusterToRead_;           // cluster which is to be read
  Cluster * clusterReturnRightRows_;           // cluster which is to be read
  Cluster * clusterList_;             // list of all clusters
  HashScratchSpace * tempFile_;           // for overflow handling
  ExSubtask *ioEventHandler_;     
  ex_tcb * callingTcb_;
  UInt16 scratchThresholdPct_;
  UInt32 sequenceGenerator_;          // to generate unique IDs for clusters

  HashBuffer * outerReadBuffers_ ;    // buffers for reading an outer cluster

  // for testing/regressions only !!
  UInt16 forceOverflowEvery_;  // deny memory after every so many requests
  UInt16 forceOverflowCounter_;  // dynamic counter to track when to deny
  UInt16 forceHashLoopAfterNumBuffers_;  // After #buffers were read from o/f 
  UInt16 forceClusterSplitAfterMB_; // split after cluster reached such size

  NABoolean doLog_;       // log to EMS major overflow events
  NABoolean bufferedWrites_;  // use bufferred writes
  NABoolean disableCmpHintsOverflow_;
  ULng32 memoryQuotaMB_ ; // can't use more than that (0 = no limit) 
  ULng32 minMemoryQuotaMB_ ; // don't go below the minimum
  ULng32 minMemBeforePressureCheck_; // no pressure/hints checks below that
  Float32 bmoCitizenshipFactor_;
  Int32  pMemoryContingencyMB_; 
  Float32 estimateErrorPenalty_;
  Float32 hashMemEstInKBPerNode_;

  Int64 totalPhase3TimeNoHL_;
  Int64 maxPhase3Time_;
  Int64 minPhase3Time_;
  ULng32 numClustersNoHashLoop_;
  ULng32 bmoMaxMemThresholdMB_;

  NABoolean earlyOverflowStarted_;  // if o/f based on compiler hints 

  // These fields are used to ensure that #buckets and #hash-table-entries
  // have no common prime factors (to make even use of the hash table entries)
  NABoolean evenFactor_;  // # buckets is even (i.e., Hash-Join)
  ULng32 primeFactor_; // # buckets (divided by buckets-per-cluster)

  // used by HGB only - initial HT size to be resized up as needed
  ULng32 initialHashTableSize_;

  // The num of concurrent IOs is used as a minimum for the number of
  // buffers to keep for a flushed cluster before it is flushed again
  // (to improve IO of sequential buffers)
  unsigned short minBuffersToFlush_;

  unsigned short minNumWriteOuterBatch_;
  unsigned short maxNumWriteOuterBatch_;
  unsigned short numReadOuterBatch_;
  Int32 scratchIOVectorSize_;
  ScratchOverflowMode overFlowMode_;
  ExBMOStats *bmoStats_;



};
/////////////////////////////////////////////////////////////////////////////
// inline functions of ClusterDB
/////////////////////////////////////////////////////////////////////////////
inline Cluster * ClusterDB::getClusterList() const {
  return clusterList_;
};

inline void ClusterDB::setClusterList(Cluster * clusterList) {
  clusterList_ = clusterList;
};
inline Cluster * ClusterDB::getClusterToFlush() const {
  return clusterToFlush_;
};

inline Cluster * ClusterDB::getClusterToProbe() const {
  return clusterToProbe_;
};

inline void ClusterDB::setClusterToProbe(Cluster * cluster) {
  clusterToProbe_ = cluster;
};

inline Cluster * ClusterDB::getClusterToRead() const {
  return clusterToRead_;
};

inline void ClusterDB::setClusterToRead(Cluster * cluster) {
  clusterToRead_ = cluster;
};

inline Cluster * ClusterDB::getClusterReturnRightRows() const {
  return clusterReturnRightRows_;
};

inline void ClusterDB::setClusterReturnRightRows(Cluster * cluster) {
  clusterReturnRightRows_ = cluster;
};

inline HashScratchSpace * ClusterDB::getScratch() const {
 return tempFile_;
};


/////////////////////////////////////////////////////////////////////////////
// ClusterBitMap is used for left joins with a hash loop or for right joins
/////////////////////////////////////////////////////////////////////////////
class ClusterBitMap : public NABasicObject {
public:
  ClusterBitMap(ULng32 size, ExeErrorCode * rc);
  ~ClusterBitMap();
  void setBit(ULng32 bitIndex);
  NABoolean testBit(ULng32 bitIndex);
private:
  ULng32 size_;      // size of bitmap (# of bits)
  char * bitMap_;           // the bitmap itself
};

/////////////////////////////////////////////////////////////////////////////
// a Cluster is the unit of overflow handling
/////////////////////////////////////////////////////////////////////////////
class Cluster : public NABasicObject {
  friend class ClusterDB;
  friend class HashBuffer;
  friend class ExSequenceTcb;
public:
  enum ClusterState {
    FLUSHED,          // cluster is on temporary file, max 1 buffer in memory
    IN_MEMORY,        // all buffers of the cluster are in main memory
    CHAINED           // cluster is IN_MEMORY and rows are chained into
                      // hash table
  };

  Cluster(ClusterState state,
	  ClusterDB * clusterDb,
	  Bucket * buckets,
	  ULng32 bucketCount,           // for ths cluster
	  ULng32 rowLength,
          NABoolean useVariableLength,
          NABoolean considerBufferDefrag,
	  short insertAtpIndex,
	  NABoolean isInner,                   // is this an inner Cluster?
          NABoolean bitMapEnable,              // Enable bitmap allocation for
                                               // left outer join and semi join
                                               // 
                                               // A bitmap is allocated for the
                                               // outer cluster in Phase 3
                                               // when the inner cluster 
                                               // cannot be read in memory
                                               // in full. 
	  Cluster * next,                      // next cluster in list
	  ExeErrorCode *rc,
	  HashBuffer * bufferPool = NULL);     // an already existing buffer

  ~Cluster();

  inline ClusterState getState() const;
  inline ULng32 getRowsInBuffer() const;
  inline ULng32 getRowCount() const;
  inline ULng32 getReadCount() const;
  inline void incReadCount();
  inline NABoolean endOfCluster();
  inline NABoolean hasBitMap();
  void updateBitMap();
  NABoolean testBitMap();

  // (The three methods below are only for Non-Aggregate Hash-Groupby)
  inline HashRow *getLastDataPointer() { return lastDataPointer_; }
  inline NABoolean returnOverflowRows() { return returnOverflowRows_; }
  inline void initializeReadPosition();

  inline NABoolean bitMapEnable() const;
  inline NABoolean isInner() const;
  inline Cluster * getNext() const;
  Cluster * getOuterCluster() const { return buckets_->getOuterCluster(); };
  inline void setOuterCluster(Cluster * outerCluster);
  HashTable * getHashTable() const { return hashTable_; };
  inline HashBuffer * getFirstBuffer() const { return bufferPool_; };
  inline void setNext(Cluster * next);
  void positionOnFirstRowInBuffer() 
  {    // position on the first row in main memory 
    scanPosition_ = bufferPool_;
    scanPosition_->castToSerial()->positionOnFirstRow();
  };

  // remove the hash table from the cluster
  void removeHashTable();

  // insert a new row into the cluster. If insert() was not sucessful, it
  // returns FALSE. In this case we have to flush a cluster. The only reason
  // for insert() not to be sucessful is, if we could not t allocate a
  // new buffer for this cluster. In case of a partial group by, we do not
  // flush a cluster, instead, we return the current row as a partial group.
  NABoolean insert(atp_struct * newEntry,
		   ex_expr * moveExpr,
		   SimpleHashValue hashValue,
		   ExeErrorCode * rc,
		   NABoolean skipMemoryCheck = FALSE);

  void positionOnFirstRowInFirstOuterBuffer() 
  {  // position on the first row in the first outer buffer in memory 

    for ( HashBuffer * tmpB = clusterDb_->outerReadBuffers_; tmpB ; tmpB = tmpB->getNext() )
      tmpB->castToSerial()->fixup();

    scanPosition_ = clusterDb_->outerReadBuffers_ ;
    scanPosition_->castToSerial()->positionOnFirstRow();
  };

  // flush the Cluster. If the flushing is not complete yet (I/Os pending)
  // flush() returns FALSE. Otherwise TRUE.
  NABoolean flush(ExeErrorCode * rc);
  
  // encapsulate error handling within flush. it is wrapper 
  // around flush(ExeErrorCode * rc) call. Used by ExSequence.
  NABoolean flush(ComDiagsArea *&da, CollHeap *heap);

  // spill a cluster. Spill is the same as flush. The only difference is,
  // that a spilled cluster still has a hash table (this is only used for
  // hash group by)
  NABoolean spill(ExeErrorCode * rc,
		  NABoolean noAggregates = FALSE );

  // read the Cluster. If the reading is not complete yet (I/Os pending) 
  // read() returns FALSE. Otherwise TRUE. If the Cluster is an inner
  // Cluster read() reads as many buffers as possible. If it is an outer
  // Cluster, read() reads just one bufffer.
  NABoolean read(ExeErrorCode * rc);
  
  // encapsulate error handling within read. it is wrapper 
  // around read(ExeErrorCode * rc) call. Used by ExSequence.
  NABoolean read(ComDiagsArea *&da, CollHeap *heap);
 
  // create a hash table for this Cluster and chain all rows of the Cluster
  // into this hash table.
  NABoolean chain(NABoolean forceChaining, ExeErrorCode * rc);

  // set up an outer Cluster for this inner Cluster (if required).
  ExeErrorCode setUpOuter(ULng32 rowLength,
			  short atpIndex,
			  NABoolean bitMapEnable);

  // initialize the global ClusterDB scratch file if needed, and set it for
  // this cluster. Also create a new read handle.
  NABoolean initScratch(ExeErrorCode * rc);

  // reset the cluster pair (inner and outer) for the next iteration
  // of a hash loop
  void resetForHashLoop();

  ULng32 numLoops()
  { return numLoops_; } // if hash loop - how many times so far (logging)

  Int64 startTimePhase3() { return startTimePhase3_; };
  void setStartTimePhase3(Int64 theTime) { startTimePhase3_ = theTime ; };

  // set up a cluster stats entry in heap and return it
  ExClusterStats* getStats(CollHeap* heap);

  // release all buffers of the Cluster
  void releaseAllHashBuffers();

  // return next row from buffer
  HashRow * advance();
  
  HashRow * getCurrentRow();

  // Report the size (in bytes) of this cluster
  inline Int64 clusterSize() {return totalClusterSize_; } // total size (bytes)

  // Internal utility: Calculate memory and create a hash table
  HashTable * createHashTable(UInt32 memForHashTable,
			      ExeErrorCode * rc,
			      // skip mem check for HGB; it starts at min HT 
			      NABoolean checkMemory = FALSE );

    inline char * getDefragBuffer()
  {
    return defragBuffer_;
  }


private:

  // split a Cluster into two. Adjust buckets accordingly.
  NABoolean checkAndSplit(ExeErrorCode * rc) ;
  HashBuffer * lastSqueezedBuffer_ ; // last buffer "squeezed"
  NABoolean dontSplit_; // don't split this cluster (e.g., it's skewed)

  // Normally every cluster keeps its sequence ID in seqID_[0]. But for HJ
  // after every split, the cluster gets a new sequence ID (like a stack
  // seqID_[1}, and last seqID_[2]). HGB DISTINCT uses seqID_[1] when it needs
  // to keep seperately 2 sequences, of returned and not-returned rows. 
  UInt32 seqID_[3];    // 3 = log( bucketsPerCluster , 2 ) + 1
  UInt16 seqIDIndex_;  // current seq-ID (between 0 and the top of the stack)
  UInt16 maxSeqIDIndex_;  // the max seq-ID, at the top of the stack
  UInt16 batchCountDown_; // count buffers in a batch (for split only)

  HashBuffer * keepRecentBuffer_; // don't flush most recent (not full) buffer

  // return the memory size for building a hash table of given entry count
  ULng32 getMemorySizeForHashTable(ULng32 entryCount);

  ClusterState state_;
  ClusterDB * clusterDb_;
  Bucket * buckets_;               // the buckets of this Cluster
  ULng32 bucketCount_;      // # of buckets of the Cluster
  ULng32 rowLength_;
  NABoolean useVariableLength_;
  NABoolean considerBufferDefrag_;
  short insertAtpIndex_;
  NABoolean isInner_;
  NABoolean bitMapEnable_;

  HashScratchSpace * tempFile_;
  ClusterPassBack * readHandle_;

  Cluster * next_;                        // next Cluster in list
  NABoolean ioPending_;                   // this Cluster has an I/O in flight

  Int64 totalClusterSize_;                // total size of this cluster (bytes)

  ClusterBitMap * outerBitMap_;
  HashTable * hashTable_;
  ULng32 rowCount_;                // # of rows stored in this cluster
  ULng32 readCount_;               // for inner: number of rows read in
                                          // this loop of a hash loop
                                          // (== rowCount_ if no hash loop)
                                          // for outer: total number of rows
                                          // read
  ULng32 writeIOCnt_;              // counters for overflow I/Os
  ULng32 readIOCnt_;
  HashBuffer * bufferPool_;               // first buffer of a potential list
  ULng32 numInMemBuffers_;         // number of in-memory buffers for this cluster
  HashBuffer * scanPosition_;             // for scanning all buffers

  // for concurrent writing of multiple buffers, need to know which is next
  HashBuffer * nextBufferToFlush_;
  // For OLAP - when reading buffers from overflow for "bounded following", 
  // where Cluster::read() may need to jump back to first buffer in the list.
  HashBuffer * firstBufferInList_;
  // Usually NULL; only OLAP sets this pointer at the last flush (for that
  // partition) to avoid flushing the remaining buffers.
  HashBuffer * afterLastBufferToFlush_;
  // for concurrent reading of multiple buffers, need to know who is next
  HashBuffer * nextBufferToRead_;
  // Usually NULL; only OLAP with unbounded following that starts from a
  // bounded following needs to set this pointer.
  HashBuffer * afterLastBufferToRead_;

  // keep hash-buffer location where the most recent row was inserted
  // (Used for non-aggr hash groupby overflow, to return this row immediately)
  HashRow *lastDataPointer_ ;
  NABoolean returnOverflowRows_;

  ULng32 numLoops_; // if hash loop - how many times so far (logging)

  Int64 startTimePhase3_; // the time this cluster started phase 3 (logging)

  NABoolean flushMe_ ; // make this cluster a candidate for a flush

  // Internal methods
  NABoolean setHashLoop(ExeErrorCode * rc);
  NABoolean squeezeOutRowsOfOtherClusters(ExeErrorCode * rc);

  inline NABoolean IONotReady(ExeErrorCode * rc, 
			      UInt32 readSeqID = 0, // non-zero only for read
			      NABoolean checkAll = FALSE );

  inline NABoolean IONotComplete(ExeErrorCode * rc)
  { return IONotReady(rc, 0, TRUE); };

  NABoolean completeCurrentRead_ ; // synch buffers read so far 
  Int16 buffersRead_ ;

  char * defragBuffer_;
};

/////////////////////////////////////////////////////////////////////////////
// inline functions of Cluster
/////////////////////////////////////////////////////////////////////////////
inline Cluster::ClusterState Cluster::getState() const {
  return state_;
};

// get the number of rows of the Cluster which are in the buffer in main
// memory
inline ULng32 Cluster::getRowsInBuffer() const {
  return bufferPool_->getRowCount();
};

// get the total number of rows in the Cluster
inline ULng32 Cluster::getRowCount() const {
  return rowCount_;
};

inline ULng32 Cluster::getReadCount() const {
  return readCount_;
};

inline void Cluster::incReadCount() {
  readCount_++;
};

inline NABoolean Cluster::hasBitMap() {
  return (outerBitMap_ != NULL);
};

inline void Cluster::initializeReadPosition() {
  // start reading from the earlier written list of buffers (whose seq-ID
  // was pushed an index up, because we always finish reading at index 0)
  seqIDIndex_ = maxSeqIDIndex_  ; // update the max
}

inline NABoolean Cluster::endOfCluster() {
  // we always finish reading with the seq at index 0
  if ( ! maxSeqIDIndex_ )  // there was no (cluster split or) HGB distinct o/f
    return seqIDIndex_ == 0 && readHandle_->endOfSequence() ;

  ex_assert( !isInner_ , "Can not handle a cluster that was split");
  
  // below: only for HGB distinct
  //  Check returnOverflowRows_ because this variable indicates that some read
  // has completed at sequence index 0. Otherwise the rest of the condition is
  // also true before we start reading that sequence.
  return returnOverflowRows_ && 
    seqIDIndex_ == 0 && readHandle_->endOfSequence() ;
};

inline NABoolean Cluster::IONotReady(ExeErrorCode * rc, 
				     UInt32 readSeqID,
				     NABoolean checkAll )
{
  RESULT IOStatus = checkAll ?  tempFile_->checkIOAll() :  // all opens free ?
    !readSeqID ? tempFile_->checkIOWrite() : // any open available for write ?
    tempFile_->checkIORead(readHandle_, readSeqID ); // any open for read ?

  // are all/any previous I/Os still in flight, and we can not issue a new I/O ?
  if ( IOStatus != IO_COMPLETE ) {
    if ( IOStatus == OTHER_ERROR || IOStatus == SCRATCH_FAILURE ) {
      // if it's the first read when there was no prior write -- that's OK
      if ( readSeqID && readHandle_->endOfSequence() )
	return FALSE; // the code after call to readThru would handle that case
      *rc = EXE_SORT_ERROR; // it's an error
    }
    // else the I/O is not complete yet; go back to the scheduler
    return TRUE;
  }
  return FALSE; // IO is ready
}

inline NABoolean Cluster::bitMapEnable() const {
  return bitMapEnable_;
};

inline NABoolean Cluster::isInner() const {
  return isInner_;
};

inline Cluster * Cluster::getNext() const {
  return next_;
};

inline void Cluster::setOuterCluster(Cluster * outerCluster) {
  // mark the given cluster as an outer for this (inner) cluster
  if (outerCluster)
    outerCluster->isInner_ = FALSE;

  // set all references in the buckets
  for (ULng32 i = 0; i < bucketCount_; i++)
    buckets_[i].setOuterCluster(outerCluster);
};

inline void Cluster::setNext(Cluster * next) {
  next_ = next;
};

// This ClusterDB method needs to follow the reclaration of the class Cluster
inline void ClusterDB::setClusterToFlush(Cluster * cluster) {
  clusterToFlush_ = cluster;
  if ( ! cluster ) return; // just performed a reset
  // start with the first (also most recent) buffer in chain
  clusterToFlush_->nextBufferToFlush_ = clusterToFlush_->bufferPool_ ;
  // when cluster is selected explicitly -- flush all the buffers (incl. last)
  clusterToFlush_->keepRecentBuffer_ = NULL;
};

// The Bucket::insert is just a wrapper over Cluster::insert that also
// updates the per-Bucket rows counter
// (This declaration needs to follow the declaration of class Cluster.)
inline NABoolean Bucket::insert(atp_struct * newEntry,
				ex_expr * moveExpr,
				SimpleHashValue hashValue,
				ExeErrorCode * rc,
				NABoolean skipMemoryCheck) {
  NABoolean isInserted = innerCluster_->insert(newEntry,
					       moveExpr,
					       hashValue,
					       rc,
					       skipMemoryCheck);
  if (isInserted)
    rowCount_++;
  return isInserted;
};



class IOTimer 
{
public:
  
  IOTimer () ;

  
  void resetTimer();             // re-zero the elapsed time (not needed on first use)

  
  NABoolean startTimer();        // do nothing, return FALSE is already started

  
  Int64 endTimer();              // Return elapsed time.
  
private:
  NABoolean ioStarted_;
  Int64 startTime_;
  Int64 accumTime_;
};

inline IOTimer::IOTimer () :
  ioStarted_(FALSE) ,
  startTime_(0),
  accumTime_(0)
  {};

#endif
