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
#ifndef _HDFSHOOK_H
#define _HDFSHOOK_H

// HDFS-level statistics for Hive tables (Hive HDFS = HHDFS)
//

// We assume a Hive table to look like this in HDFS
//
// + top-level directory for Hive table
// |
// ---> + partition directories for partitioned tables only
//      |
//      ---> + buckets (_<bucket> file suffixes) for bucketed tables only
//           |
//           ---> + files (one or more HDFS files with data)
//                |
//                ---> + blocks of the file
//                     |
//                     ---> list of hosts with replicas for a given block

#include "Collections.h"
#include "NAString.h"
#include "CmpContext.h"
#include "hdfs.h"
#include <stdio.h>

// forward declarations
struct hive_tbl_desc;
class HivePartitionAndBucketKey;
class HHDFSTableStats;
class OsimHHDFSStatsBase;
class OptimizerSimulator;
class ExLobGlobals;

typedef CollIndex HostId;
typedef Int64 BucketNum;
typedef Int64 BlockNum;
typedef Int64 Offset;

class HHDFSMasterHostList : public NABasicObject
{
friend class OptimizerSimulator;
public:
  HHDFSMasterHostList(NAMemory *heap) {}
  ~HHDFSMasterHostList();

  // translate a host name to a number (add host if needed)
  static HostId getHostNum(const char *hostName);

  // get host name from host number
  static const char * getHostName(HostId hostNum);

  static CollIndex entries()                 { return getHosts()->entries(); }
  static CollIndex getNumSQNodes()                     { return numSQNodes_; }
  static void resetNumSQNodes() { numSQNodes_ = 0; }
  static void resethasVirtualSQNodes() { hasVirtualSQNodes_ = FALSE; }
  static CollIndex getNumNonSQNodes() { return getHosts()->entries()-numSQNodes_; }
  static NABoolean usesRemoteHDFS()         { return getNumNonSQNodes() > 0; }
  static NABoolean hasVirtualSQNodes()          { return hasVirtualSQNodes_; }

  static const CollIndex InvalidHostId = NULL_COLL_INDEX;

  // make sure all SeaQuest nodes are recorded with their SQ node numbers 0...n-1
  static NABoolean initializeWithSeaQuestNodes();

private:
  static HostId getHostNumInternal(const char *hostName);
  static ARRAY(const char *) *getHosts()
        {
          return CmpCommon::context()->getHosts();
        }

  // an array that translates host names into integer indexes
  static THREAD_P CollIndex numSQNodes_;
  static THREAD_P NABoolean hasVirtualSQNodes_;
};

class HHDFSDiags
{
public:

  HHDFSDiags() { success_ = TRUE; }
  NABoolean isSuccess() const { return success_; }
  const char *getErrLoc() const { return errLoc_; }
  const char *getErrMsg() const { return errMsg_; }
  void recordError(const char *errMsg,
                   const char *errLoc = NULL);
  void reset() { errLoc_ = ""; errMsg_ = ""; success_ = TRUE; }

private:
  NABoolean success_;
  NAString errLoc_;
  NAString errMsg_;
};

class HHDFSStatsBase : public NABasicObject
{
  friend class OsimHHDFSStatsBase;
public:
  HHDFSStatsBase(HHDFSTableStats *table) : numBlocks_(0),
                                           numFiles_(0),
                                           totalRows_(0),
                                           totalStringLengths_(0),
                                           totalSize_(0),
                                           numStripes_(0),
                                           modificationTS_(0),
                                           sampledBytes_(0),
                                           sampledRows_(0),
                                           table_(table) {}

  void add(const HHDFSStatsBase *o);
  void subtract(const HHDFSStatsBase *o);

  Int64 getTotalSize() const { return totalSize_; }
  Int64 getNumFiles() const { return numFiles_; }
  Int64 getNumBlocks() const { return numBlocks_; }
  Int64 getTotalRows() const { return totalRows_; }
  Int64 getTotalStringLengths() { return totalStringLengths_; }
  Int64 getNumStripes() const { return numStripes_; }
  Int64 getSampledBytes() const { return sampledBytes_; }
  Int64 getSampledRows() const { return sampledRows_; }
  time_t getModificationTS() const { return modificationTS_; }
  Int64 getEstimatedBlockSize() const;
  Int64 getEstimatedRowCount() const;
  Int64 getEstimatedRecordLength() const;
  void print(FILE *ofd, const char *msg);
  const HHDFSTableStats *getTable() const { return table_; }
  HHDFSTableStats *getTable() { return table_; }
  
  virtual OsimHHDFSStatsBase* osimSnapShot(NAMemory * heap){ return NULL; }

protected:
  Int64 numBlocks_;
  Int64 numFiles_;
  Int64 totalRows_;  // for ORC files
  Int64 numStripes_;  // for ORC files
  Int64 totalStringLengths_;  // for ORC files
  Int64 totalSize_;
  time_t modificationTS_; // last modification time of this object (file, partition/directory, bucket or table)
  Int64 sampledBytes_;
  Int64 sampledRows_;
  HHDFSTableStats *table_;
};

class HHDFSFileStats : public HHDFSStatsBase
{
  friend class OsimHHDFSFileStats;
public:
  HHDFSFileStats(NAMemory *heap,
                 HHDFSTableStats *table) :
       HHDFSStatsBase(table),
       heap_(heap),
       fileName_(heap),
       blockHosts_(NULL) {}
  ~HHDFSFileStats();
  void populate(hdfsFS fs,
                hdfsFileInfo *fileInfo,
                Int32& samples,
                HHDFSDiags &diags,
                NABoolean doEstimation = TRUE,
                char recordTerminator = '\n'
                );
  const NAString & getFileName() const                   { return fileName_; }
  Int32 getReplication() const                        { return replication_; }
  Int64 getBlockSize() const                            { return blockSize_; }
  HostId getHostId(Int32 replicate, Int64 blockNum) const
                        { return blockHosts_[replicate*numBlocks_+blockNum]; }
  virtual void print(FILE *ofd);
  
  virtual OsimHHDFSStatsBase* osimSnapShot(NAMemory * heap);

private:

  NAString fileName_;
  Int32 replication_;
  Int64 blockSize_;

  // list of blocks for this file
  HostId *blockHosts_;
  NAMemory *heap_;

};

class HHDFSBucketStats : public HHDFSStatsBase
{
  friend class OsimHHDFSBucketStats;
public:
  HHDFSBucketStats(NAMemory *heap,
                   HHDFSTableStats *table) :
       HHDFSStatsBase(table),
       heap_(heap), fileStatsList_(heap), scount_(0) {}
  ~HHDFSBucketStats();

  const CollIndex entries() const         { return fileStatsList_.entries(); }
  const HHDFSFileStats * operator[](CollIndex i) const 
                                                 { return fileStatsList_[i]; }
  HHDFSFileStats * operator[](CollIndex i)       { return fileStatsList_[i]; }

  void addFile(hdfsFS fs, hdfsFileInfo *fileInfo, 
               HHDFSDiags &diags,
               NABoolean doEstimate = TRUE,
               char recordTerminator = '\n',
               CollIndex pos = NULL_COLL_INDEX);
                    
  void removeAt(CollIndex i);
  void print(FILE *ofd);

  void insertAt(Int32 pos, HHDFSFileStats* st){  fileStatsList_.insertAt(pos, st);  }
  
  virtual OsimHHDFSStatsBase* osimSnapShot(NAMemory * heap);

private:

  // list of files in this bucket
  LIST(HHDFSFileStats *) fileStatsList_;
  NAMemory *heap_;

  Int32 scount_; // total # of sampled files associated with this bucket
};

class HHDFSListPartitionStats : public HHDFSStatsBase
{
    friend class OsimHHDFSListPartitionStats;
public:
  HHDFSListPartitionStats(NAMemory *heap,
                          HHDFSTableStats *table) :
       HHDFSStatsBase(table),
       heap_(heap), partitionDir_(heap),
    bucketStatsList_(heap),
    doEstimation_(FALSE),
    recordTerminator_(0)
    {}
  ~HHDFSListPartitionStats();

  const NAString &getDirName() const                 { return partitionDir_; }
  const CollIndex entries() const       { return bucketStatsList_.entries(); }
  const HHDFSBucketStats * operator[](CollIndex i) const 
           { return (bucketStatsList_.used(i) ? bucketStatsList_[i] : NULL); }

  Int32 getNumOfBuckets() const { return (defaultBucketIdx_ ? defaultBucketIdx_ : 1); }
  Int32 getLastValidBucketIndx() const               { return defaultBucketIdx_; }

  const hdfsFileInfo * dirInfo() const {return &dirInfo_; }

  void populate(hdfsFS fs, const NAString &dir, Int32 numOfBuckets, 
                HHDFSDiags &diags,
                NABoolean doEsTimation, char recordTerminator);
  NABoolean validateAndRefresh(hdfsFS fs, HHDFSDiags &diags, NABoolean refresh);
  Int32 determineBucketNum(const char *fileName);
  void print(FILE *ofd);

  void insertAt(Int32 pos, HHDFSBucketStats* st){  bucketStatsList_.insertAt(pos, st);  }
  
  virtual OsimHHDFSStatsBase* osimSnapShot(NAMemory * heap);

private:

  // directory of the partition
  NAString partitionDir_;
  NAString partitionKeyValues_;
  int partIndex_; // index in HDFSTableStats list

  // number of buckets (from table DDL) or 0 if partition is not bucketed
  // Note this value can never be 1. This value indicates the last
  // valid index in bucketStatsList_ below.
  Int32 defaultBucketIdx_;

  // array of buckets in this partition (index is bucket #)
  ARRAY(HHDFSBucketStats *) bucketStatsList_;

  NABoolean doEstimation_;
  char recordTerminator_;
  
  hdfsFileInfo dirInfo_;

  NAMemory *heap_;
};

// HDFS file-level statistics for a Hive table. This includes
// partitioned and bucketed tables, binary and compressed tables
class HHDFSTableStats : public HHDFSStatsBase
{
  friend class HivePartitionAndBucketKey; // to be able to make a subarray of the partitions
  friend class OsimHHDFSTableStats;
  friend class OptimizerSimulator;
public:
  HHDFSTableStats(NAMemory *heap) : HHDFSStatsBase(this),
                                    currHdfsPort_(-1),
                                    fs_(NULL),
                                    hdfsPortOverride_(-1),
                                    tableDir_(heap),
                                    numOfPartCols_(0),
                                    totalNumPartitions_(0),
                                    recordTerminator_(0),
                                    fieldTerminator_(0),
                                    nullFormat_(NULL),
                                    validationJTimestamp_(-1),
                                    listPartitionStatsList_(heap),
                                    hiveStatsSize_(0),
                                    heap_(heap) {}
  ~HHDFSTableStats();

  const CollIndex entries() const          { return listPartitionStatsList_.entries(); }
  const HHDFSListPartitionStats * operator[](CollIndex i) const 
                                                  { return listPartitionStatsList_[i]; }

  // Populate the HDFS statistics for this table and set diagnostics info
  // return success status: TRUE = success, FALSE = failure.
  // Use getDiags() method to get error details.
  NABoolean populate(struct hive_tbl_desc *htd);

  // Check for HDFS file changes, return TRUE if stats are still valid
  // or could be refreshed. Diagnostics could be set if returning FALSE,
  // checking for diagnostics is optional.
  NABoolean validateAndRefresh(Int64 expirationTimestamp=-1, NABoolean refresh=TRUE);

  // Split a location into its parts.
  // If you want to set a ComDiagsArea for errors, use
  // TableDesc::splitHiveLocation
  static NABoolean splitLocation(const char *tableLocation,
                                 NAString &hdfsHost,
                                 Int32 &hdfsPort,
                                 NAString &tableDir,
                                 HHDFSDiags &diags,
                                 int hdfsPortOverride);

  void processDirectory(const NAString &dir, Int32 numOfBuckets, 
                        NABoolean doEstimation, char recordTerminator);

  void setPortOverride(Int32 portOverride)         { hdfsPortOverride_ = portOverride; }

  Int32 getPortOverride() const {return hdfsPortOverride_;}

  char getRecordTerminator() const {return recordTerminator_;}
  char getFieldTerminator() const {return fieldTerminator_;}
  char *getNullFormat() const { return nullFormat_; }

  Int32 getNumPartitions() const {return totalNumPartitions_;}

  Int64 getValidationTimestamp() const                 { return validationJTimestamp_; }

  // return # of buckets if all partns are consistently bucketed, 0 otherwise
  // caller has to check for same bucket cols
  Int32 getNumOfConsistentBuckets() const;
  
  // for the NATable cache
  void setupForStatement();
  void resetAfterStatement();

  void print(FILE *ofd);
  // heap size used by the hive stats
  Int32 getHiveStatsUsedHeap()
   {
     return (hiveStatsSize_);
   }

  const HHDFSDiags &getDiags() const { return diags_; }
  const NABoolean hasError() const { return !diags_.isSuccess(); }

  const NABoolean isTextFile() const { return (type_ == TEXT_);}
  const NABoolean isSequenceFile() const { return (type_ == SEQUENCE_);}  
  const NABoolean isOrcFile() const { return (type_ == ORC_);}

  const NAString &tableDir() const { return tableDir_; }

  void insertAt(Int32 pos, HHDFSListPartitionStats * st) {  listPartitionStatsList_.insertAt(pos, st);  }
  virtual OsimHHDFSStatsBase* osimSnapShot(NAMemory * heap);
  void captureHiveTableStats(const NAString &tableName, Int64 lastModificationTs, hive_tbl_desc* hvt_desc );
  static HHDFSTableStats * restoreHiveTableStats(const NAString & tableName,  Int64 lastModificationTs,  hive_tbl_desc* hvt_desc, NAMemory* heap);

  const Lng32 numOfPartCols() const { return numOfPartCols_; }
  const Lng32 totalNumPartitions() const { return totalNumPartitions_; }

private:
  enum FileType
  {
    UNKNOWN_ = 0,
    TEXT_ = 1,
    SEQUENCE_ = 2,
    ORC_ = 3
  };

  NABoolean connectHDFS(const NAString &host, Int32 port);
  void disconnectHDFS();

  NAString currHdfsHost_;
  Int32    currHdfsPort_;
  hdfsFS   fs_;

  // host/port used to connect to HDFS
  Int32 hdfsPortOverride_; // if > -1, use this port # instead of what's in the Hive metadata
  // HDFS directory name of the table
  NAString tableDir_;
  // indicates how many levels of partitioning directories the table has
  int numOfPartCols_;
  // total number of actual partitions, at all levels,
  // or 1 if the table is not partitioned
  int totalNumPartitions_;

  char recordTerminator_ ;
  char fieldTerminator_ ;

  char *nullFormat_;

  Int64 validationJTimestamp_;
  // heap size used by the hive stats
  Int32 hiveStatsSize_;

  // List of partitions of this file. The reason why this is
  // an array and not a list is that we want to use an NASubarray
  // for it in class HivePartitionAndBucketKey (file SearchKey.h).
  ARRAY(HHDFSListPartitionStats *) listPartitionStatsList_;

  // These diags get reset and populated by the two top-level calls
  // for HDFS statistics: populate() and validateAndRefresh().
  // Check the diags after these two calls.

  // For validateAndRefresh() the best way to deal with any errors is
  // to treat them as an invalid cache entry (FALSE return value) and
  // retry reading all stats - it may succeed with fresh information.
  HHDFSDiags diags_;

  NAMemory *heap_;

  FileType type_;
};

#endif
