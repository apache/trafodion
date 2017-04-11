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
#ifndef SCRATCHSPACE_H
#define SCRATCHSPACE_H

/* -*-C++-*-
******************************************************************************
*
* File:         ScratchSpace.h
* RCS:          $Id: ScratchSpace.h,v 1.3.2.2 1998/07/08 21:47:28  Exp $
*                               
* Description:  This class provide a virtual scratch space to SortUtil for
*               all its scratch space requirement. The actual management of 
*               physical scratch disk file is hidden to other componenets.
*               
*               
* Created:	    05/20/96
* Modified:     $ $Date: 1998/07/08 21:47:28 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
*
******************************************************************************/

#include "CommonStructs.h"
#include "Const.h"
#include "ScratchFileMap.h"
#include "RunDirectory.h"
#include "DiskPool.h"
#include "NABasicObject.h"
#include "Int64.h"
#include "SortError.h"

class ExSubtask;
class IpcEnvironment;
class ExExeStmtGlobals;
class ScratchFile;
class ExBMOStats;
class SortScratchSpace;
class SortMergeNode;

#define BUFFERSIZE (56 * 1024)
//----------------------------------------------------------------------
//  SortMergeNode is used during the merge phase. Each tree node of the 
//  tournament has a merge node associated with it.
//----------------------------------------------------------------------
class SortMergeBuffer : public AsyncIOBuffer
{
  public:
    SortMergeBuffer(void){this->reset();}
    ~SortMergeBuffer(void){};

    //mnext_ is used to exclusively link this object into one of the two possible link lists.
    //To begin with, this object is linked to SortScratchSpace::freeSortMergeBufferPool_.
    //Once this object is used by SortMergeNode, this object is unlinked from 
    //freeSortMergeBufferPool_ and relinked to SortMergeBuffer *readQHead_.
    //Also note that this parent class AsyncIOBuffer contains a pointer that is used to 
    //link the parent object to ScratchFile::asynchronousReadQueueHead_ and
    //ScratchFile::associatedAsyncIOBuffer.
    SortMergeBuffer *mnext_;
    SortMergeNode *sortMergeNodeRef_;
    void reset(void)
    {
      AsyncIOBuffer::reset();
      mnext_ = NULL;
      sortMergeNodeRef_ = NULL;
    }
    virtual void processMisc(void);
  private:
};

class SortMergeNode
{
  public:
  Lng32 associatedRun_;
  SortMergeBuffer *readQHead_; //read queue head for this node. linklist of buffers in order.
  SortMergeBuffer *readQTail_;  //pointer to the last io buffer in the read queue.
  Lng32 numRecsRead_; 
  char* nextReadPosition_;
  ScrBlockHeader blockHead_;
  SBN beginBlockNum_;//beginning block number of run associated with this node
  SBN endBlockNum_;//end block number of the run associated with this node
  SBN nextIOBlockNum_;      //next block to be read for this run.
  Int32 numReadQBlocks_;  //realtime num of blocks attached to readQHead
  Int32 numOutStandingIO_;//realtime num of blocks beginning from readQHead that have outstanding IO.
  SortScratchSpace *scratch_;
  SortMergeNode(Lng32 associatedrun, SortScratchSpace* sortScratchSpace);
  ~SortMergeNode();
  void cleanup(void);
  RESULT checkIO(Int64 & ioWaitTime, NABoolean waited = FALSE);
  void linkToReadQ(SortMergeBuffer *mb);
  SortMergeBuffer* delinkReadQ(void);
};



class ScratchSpace : public NABasicObject {
  public : 

   ScratchSpace(CollHeap* heap, SortError* error, Lng32 blocksize,
               Int32 scratchIOVectorSize,
               Int32 explainNodeId,
               NABoolean logInfoEvent = FALSE,
               Int32 scratchMgmtOption = 0);
   ~ScratchSpace(void);

   virtual RESULT writeFile( char* block,
                      ULng32 blockNum,
                      ULng32 blockLen );

   
   RESULT writeThru( char* buf, ULng32 bufLen,
                 DWORD &blockNum );
   RESULT readThru(char* buf, Lng32 blockNum,
                 ULng32 buflen,
                 ScratchFile *readScratchFile = NULL,
                 Int32 readBlockOffset = -1);
    
   DiskPool *getDiskPool();
   NABoolean generateDiskTable( SortError *sortError);

   RESULT checkIO(ScratchFile *sFile = NULL, NABoolean checkAll = FALSE);
   RESULT completeWriteIO(void);

   void close(void);
   void truncate(void);

   // Configure ScratchSpace attributes
   void configure(const ExExeStmtGlobals* stmtGlobals,
             ExSubtask* ioEventHandler, UInt16 scratchThresholdPct);

   // Get Executor SQLCODE for last ScratchSpace error
   Int16 getLastSqlCode(void);

   Lng32 getTotalNumOfScrBlocks() const;
   void getTotalIoWaitTime(Int64& iowaitTime) const;
   ScratchFileMap* getScrFilesMap() const;
   const IpcEnvironment * getIpcEnvironment()
   {
     return ipcEnv_;
   } 
   void setIpcEnvironment(IpcEnvironment *ipc)
   {
    ipcEnv_ = ipc;
    scrFilesMap_->setBreakEnabled (ipc->breakEnabled());
   }
   void setLogInfoEventDone()
     {
       logDone_=TRUE;
     }
   NABoolean logInfoEventDone()
     {
       return logDone_;
     }
   const ExScratchDiskDrive * getScratchDirListSpec()
   {
     return scratchDirListSpec_;
   }
   void setScratchDirListSpec(const ExScratchDiskDrive * scratch_disk_list_spec)
  {
    scratchDirListSpec_ = scratch_disk_list_spec;
  }
 
  void setNumDirsSpec(ULng32 nd)
  {
    numDirsSpec_ = nd;
  }
  ULng32 getNumDirsSpec()
  {
  return numDirsSpec_;
  }

   void setEspInstance(ULng32 espnum)
   {
   espInstance_ = espnum;

   }
   ULng32 getEspInstance()
   {
   return espInstance_;

   }
  void setNumEsps(ULng32 numesps)
  {
    numEsps_ = numesps;
  }
	 
  ULng32 getNumEsps()
  {
  return numEsps_;
  }
  void setIoEventHandler(ExSubtask *es)
  {
    ioEventHandler_ = es;
  }
	 
  void setCallingTcb(ex_tcb *tcb);

  void setScratchThreshold(unsigned short scratchThreshold)
  {
   scratchThreshold_ = scratchThreshold;
  } 
  
  inline unsigned short getScratchThreshold() {return scratchThreshold_;}
  inline Lng32 getBlockSize() const {return blockSize_;}

  SortError * getSortError() { return sortError_;}

  NABoolean logInfoEvent() { return logInfoEvent_;}

  void setPreallocateExtents(NABoolean v){ preAllocateExtents_ = v;}
  void setScratchDiskLogging(NABoolean v){ scratchDiskLogging_ = v;}
  NABoolean scratchDiskLogging(void) { return scratchDiskLogging_;}
  void setScratchIOVectorSize(Int32 vectorSize)
  {
    scratchIOVectorSize_ = vectorSize;
  }
  inline Int32 getScratchIOVectorSize(void) { return scratchIOVectorSize_; }
  
  void setAsyncReadQueue(NABoolean v){ asyncReadQueue_ = v;}

  void setScratchMaxOpens(Int32 scratchMaxOpens)
      { scratchMaxOpens_ = scratchMaxOpens;}
  Int32 getExplainNodeID(void){ return explainNodeId_;}
  ExBMOStats *bmoStats() { return bmoStats_; }
  void setScratchOverflowMode(ScratchOverflowMode ovMode) { ovMode_ = ovMode;}
  ScratchOverflowMode getScratchOverflowMode(void) { return ovMode_; }
  
protected:
  Lng32 totalNumOfScrBlocks_;       // The total number of scratch blocks used.
  Lng32 blockSize_;                 // Size of the block used for buffering
  
  SortError *sortError_;
  CollHeap * heap_;
  ScratchFileMap *scrFilesMap_;
  Int64  totalIoWaitTime_;  
  RESULT CreateANewScrFileAndWrite(char *buffer, Int32 blockNum, UInt32 blockLen, NABoolean waited= FALSE_L);
  ScratchFile *currentWriteScrFile_;
   
 private :
  ScratchFile *previousWriteScrFile_;     //It is possible when having multiple opens
                            //to scratch file, one of the opens is busy 
                            //wiriting to it when new writes require to go
                            //to a new scratch file. CreatenewScratchFileAndWrite
                            //handles this situation.
  ScratchFile *currentIOScrFile_;  // Current I/O performed on the scr file
  ScratchFile *currentReadScrFile_;
  IpcEnvironment *ipcEnv_;
  DiskPool *diskPool_;
  Int32 explainNodeId_;  //For logging support. 
  NABoolean logInfoEvent_;
  NABoolean logDone_;
  const ExScratchDiskDrive * scratchDirListSpec_;   // Information about scratchvols to  include or exclude.
  ULng32 numDirsSpec_;
  ULng32 espInstance_;
  ULng32 numEsps_;
  unsigned short scratchThreshold_;
  ExSubtask *ioEventHandler_;
  ex_tcb *callingTcb_;
  Int32 scratchMgmtOption_;
  Int32 scratchMaxOpens_;
  Int32 scratchExtentSize_;
  NABoolean preAllocateExtents_;
  NABoolean scratchDiskLogging_;
  ExBMOStats *bmoStats_;
  NABoolean asyncReadQueue_;
  Int32 scratchIOVectorSize_;
  ScratchOverflowMode ovMode_;
};

class SortScratchSpace : public ScratchSpace{
  public :
    SortScratchSpace(CollHeap* heap, SortError* error,
                Int32 explainNodeId,
                Int32 scratchIOBlockSize,
                Int32 scratchIOVectorSize,
		NABoolean logInfoEvent=FALSE,
               Int32 scratchMgmtOption = 0);
   ~SortScratchSpace(void);
    
    RESULT writeRunData(char* data, ULng32 reclen, 
                 ULng32 run,NABoolean waited);
    RESULT flushRun(NABoolean endrun = FALSE_L, NABoolean waited = FALSE_L);

    RESULT initiateSortMergeNodeRead(SortMergeNode *sortMergeNode,NABoolean waited = FALSE);
    RESULT readSortMergeNode(SortMergeNode* sortMergeNode, char*& rec, 
                      ULng32 reclen, 
                      ULng32 &actRecLen,
                      NABoolean waited = FALSE,
                      Int16 numberOfBytesForRecordSize = 0);
    RESULT serveAnyFreeSortMergeBufferRead(void);
    Lng32 getTotalNumOfRuns(void); 
    SortMergeBuffer *getFreeSortMergeBuffer(void);
    void returnFreeSortMergeBuffer(SortMergeBuffer *mb);
    RESULT setupSortMergeBufferPool(Int32 numBuffers);
    void cleanupSortMergeBufferPool(void);
    void setSortMergeBlocksPerBuffer(Int32 smbb)
    { sortMergeBlocksPerBuffer_ = smbb; }

    //Cleanup scratch files in between intermediate merges. This call
    // is not to be called for general cleanup of scratch files.
    //The run number this call expects includes the run specified.
    RESULT cleanupScratchFiles(Lng32 inRun);
    RunDirectory *runDirectory_;

  protected:
    char* currentBlock_;             // Pointer to one of the two block used 
                                    // for scratch file I/O buffering.
    ScrBlockHeader blockHead_;       // The header structure which holds informa
                                    // tion relevant to the records in scratch 
                                    // blocks.
    char *scrBlock1_;                // Double buffering is used for all   
    char *scrBlock2_;                // scratch file related I/O to increase
                                    // the overlapping of sorting with scratch
                                    // file I/O.

   
    char* nextWritePosition_;        // This is a pointer to an offset within 
                                    // the currentBlock_ where the next record
                                    // should be written to. 
  
    ULng32  currentRun_;      // The run being written to the scratch
                                    // scratch blocks. May need adjustments
                                    // when implementing read. 
    
    //pool of sort merge buffer blocks for use by several sort merge nodes.
    //pool is established before merge begins.
    //No need to deallocate in scratchSpace destructor
    SortMergeBuffer *freeSortMergeBufferPool_;    
                                
    Int32 sortMergeBlocksPerBuffer_; //number of blocks that constitute a merge buffer, to reduce disk seek time. 

    NABoolean switchScratchBuffers(void);

  private:

  
};

// HashScratchSpace is a specialization over ScratchSpace providing 
// Hash operator specific interface. Book keeping of cluster Ids and 
// corresponding blocks are maintained by this class. Write requests
// register the block against a cluster ID. Read requests given a
// cluster ID would lookup the corresponding block from the internal 
// cluster map and corresponding scratch file and scratch block are 
// determined. Once the scratch file and scratch block are determined,
// a read request from the base class is invoked.
// To optimize, lookup of scratch block, a passback token is passed 
// between read requests. This token is passed back to hashScratchSpace
// for lookup of next block.
// The cluster map is a simple link list of clusterIds(ClusterDirectory), 
// and each cluster id contains a link list of cluster blocks(CBlock).

#define INITIAL_MAX_CLUSTERS 512 //array size representing number of clusters

typedef struct ClusterBlock
{
  DWORD blockNum;  //identifies a speific 56kb block
  ClusterBlock *next;  //points to next block that is wrtten
  NABoolean endBatch;// Indicates if the block is the last among a batch of writes.
}CBlock;

struct ClusterDirectory
{
  UInt32 clusterID;    //unique ID given by hash operator identifying a cluster.
  CBlock *first;      //pointer to beginning of CBlock list
  CBlock *last;      //Pointer to last element of CBlock list. For write optimization.
};

class HashScratchSpace;

// ClusterPassBack is like a cookie passed back and forth between hash operator
// and scratch. Contains details of next block to read and other details found from
// mapping the request to a corresponding scratch file.
class ClusterPassBack : public NABasicObject 
{
  friend class HashScratchSpace;
  CBlock *cBlock_;       //next block to read
  ScratchFile *scratchFile_;  //scratch file corresponding to the next block
  Int32 blockOffset_;      //offset inside the scratch file the next block begins
  NABoolean endOfClusterBatch_; //indicates if the next block is end of series writes.
public:
  ClusterPassBack()  {  initCPB();  }
  void initCPB() 
  {
    cBlock_ = NULL;
    scratchFile_ = NULL;
    blockOffset_ = -1;
    endOfClusterBatch_ = FALSE;
  }
  NABoolean endOfSequence() { return cBlock_ == NULL; }
  NABoolean endOfBatch() { return endOfClusterBatch_; } 
};

class HashScratchSpace : public ScratchSpace{
  public :
    HashScratchSpace(CollHeap* heap, SortError* error,
                 Int32 explainNodeId,
                 Int32 blockSize,
                 Int32 scratchIOVectorSize,
                 NABoolean logInfoEvent=FALSE,
                 Int32 scratchMgmtOption = 0);
   ~HashScratchSpace(void);
   
    RESULT writeThru( char* buf, UInt32 clusterID );
    RESULT readThru(char *buf, UInt32 clusterID, ClusterPassBack *cPassBack);

    //The following three calls help in initiating several IO simulataneously.
    //Their usage follows a specific protocol. The protocol is as follows:
    //1. checkIOWrite() and checkIORead() return IO_COMPLETE to indicate
    //   another IO could be initiated. Note that IO_COMPLETE does not mean
    //   previous IO that has been initiated is complete!! It means that
    //   a new IO can be initiated irrespective of IO completion of previous
    //   IOs. It could be that previous IOs have completed or could be that
    //   there are additional free file handles (or vector elements)
    //   to initiate new IO.
    //   checkIoWrite() and checkIORead() is synonymous to "Can I issue 
    //   another write" and "can I issue another read" correspendingly.
    //   If IO_NOT_COMPLETE is returned for these calls, then that indicates
    //   that there is no free handle to initiated new IO. It is expected 
    //   that the caller return control to the scheduler and return to initate
    //   new IO if checkIOwrite() or checkIORead() return IO_COMPLETE.
    //2. Assuming checkIOWrite() or checkIORead() return IO_COMPLETE, then the
    //   caller can issue WriteThru() or ReadThru() to initiate a new IO 
    //   correspondingly.
    //3. If the caller does not have any more data to initiate new IO, however
    //   is not sure if all the previous IO completed, then checkIOAll() is 
    //   called. CheckIOALL() is a IO completion call that checks if all IO
    //   that are in flight are completed. It is not a blocking call. It will
    //   return IO_NOT_COMPLETE even if one IO is pending IO completion.
    //   It returns IO_COMPLETE only when there is no IO to any scratch file is
    //   pending.
    RESULT checkIOWrite(void);   
    RESULT checkIORead(ClusterPassBack *cPassBack, UInt32 clusterID);
    RESULT checkIOAll(void);

  private:
  
  RESULT registerClusterBlock(UInt32 clusterID, DWORD blockNum);
  DWORD getClusterBlockNum(UInt32 clusterID, ClusterPassBack *cPassBack, NABoolean getCurrentBlock = FALSE);
  ClusterDirectory *clusterDList_;
  ClusterDirectory *currentCDir_;
  UInt32 numClusters_;      //Array size count of clusterID elements allocated.
};


#endif














