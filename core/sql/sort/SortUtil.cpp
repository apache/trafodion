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
* File:         SortUtil.C
* RCS:          $Id: SortUtil.cpp,v 1.12 1998/07/29 22:02:24  Exp $
*                               
* Description:  This file contains the implementation of all member functions
*               of the class SortUtil. This class is the interface provided
*               by the sort utility to callers like Executor.
*               
* Created:	07/12/96
* Modified:     $ $Date: 1998/07/29 22:02:24 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
******************************************************************************
*/
#include "ex_ex.h"
#include "Platform.h"
#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_sort.h"
#include "SortUtil.h"
#include "Qsort.h"
#include "SortTopN.h"
#include "ComCextdecs.h"
#include "logmxevent.h"
#include "ExStats.h"
#include  "ex_exe_stmt_globals.h"
#include "memorymonitor.h"
#include "sql_buffer_size.h"

//------------------------------------------------------------------------
// This file contains the all member function definitions of SortUtil class. 
//------------------------------------------------------------------------

SortUtil::SortUtil(Lng32 explainNodeId) :
  explainNodeId_(explainNodeId)
{
  version_             = 1;
  state_               = SORT_INIT;
  config_              = NULL;
  sortAlgo_            = NULL;
  scratch_             = NULL;
  memoryQuotaUtil_     = 0;
  memMonitor_          = NULL;
  overheadPerRecord_   = 0;
  bmoStats_ = NULL;
}

SortUtil::~SortUtil(void)
{
 if (sortAlgo_ != NULL) { //delete indirect space in Tree, Qsort
   delete sortAlgo_;      //delete Tree or Qsort
   sortAlgo_ = NULL;
 }
 if (scratch_ != NULL) {
   delete scratch_;       // delete ScratchSpace
   scratch_ = NULL;
 }
}

void SortUtil::DeleteSortAlgo() //delete Tree or Qsort after each merge
{
 if (sortAlgo_ != NULL) { //delete indirect space in Tree, Qsort
   delete sortAlgo_;      //delete Tree or Qsort
   sortAlgo_ = NULL;
 }
}


NABoolean SortUtil::scratchInitialize(void)
{
  if(!scratch_)
  {

    scratch_ = new (config_->heapAddr_) SortScratchSpace(config_->heapAddr_, 
                                             &sortError_,
                                             explainNodeId_,
                                             config_->scratchIOBlockSize_,
                                             config_->getScratchIOVectorSize(),
                                             config_->logInfoEvent_,
                                             config_->scratchMgmtOption_);  

    if (scratch_ == NULL)
      {
        sortError_.setErrorInfo( EScrNoMemory   //sort error
                                 ,NULL          //syserr: the actual FS error
                                 ,NULL          //syserrdetail
                                 ,"SortUtil::scratchInitialize"  //methodname
                                );
        return SORT_FAILURE;
      }

    scratch_->setScratchDirListSpec(config_->scratchDirListSpec_);
    scratch_->setNumDirsSpec(config_->numDirsSpec_);
    scratch_->setNumEsps(config_->numEsps_);
    scratch_->setEspInstance(config_->espInstance_);

    scratch_->setIoEventHandler(config_->ioEventHandler_);
    scratch_->setIpcEnvironment(config_->ipcEnv_);
    scratch_->setCallingTcb(config_->callingTcb_);
    scratch_->setScratchThreshold(config_->scratchThreshold_);
    scratch_->setPreallocateExtents(config_->preAllocateExtents_);
    scratch_->setScratchMaxOpens(config_->scratchMaxOpens_);
    scratch_->setAsyncReadQueue(TRUE);
    scratch_->setSortMergeBlocksPerBuffer(config_->sortMergeBlocksPerBuffer_);
    scratch_->setScratchDiskLogging(config_->scratchDiskLogging_);
    scratch_->setScratchIOVectorSize(config_->scratchIOVectorSize_);
    scratch_->setScratchOverflowMode(config_->getScratchOverflowMode());
    stats_.scrBlockSize_ = scratch_->getBlockSize();  

  }
  return SORT_SUCCESS; 
}

//----------------------------------------------------------------------
// Name         : sortInitialize
// 
// Parameters   : ...
//
// Description  : 
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------
NABoolean SortUtil::sortInitialize(SortUtilConfig& config, ULng32 topNSize)
{
  
  //---------------------------------------------------------------
  // Do some cleanup since we may be re-initializing SortUtil. 
  // Basically we delete any memory that was allocated dynamically
  // but was not yet released.  Also, the sortError is reset.
  //---------------------------------------------------------------
  doCleanUp();
  
  //if topNSize_ is set, then use TopN.
  if(config.topNSort_ && topNSize)
  {
    sortAlgo_ =
      new (config.heapAddr_) SortTopN(topNSize,
                                  config.maxMem_,
                                  config.recSize_,
                                  config.sortType_.doNotAllocRec_,
                                  config.keySize_,
                                  scratch_,
                                  TRUE,
                                  config.heapAddr_,
                                  &sortError_,
                                  explainNodeId_,
                                  this);

  }
  else
  {
    sortAlgo_ =
      new (config.heapAddr_) Qsort(config.runSize_,
                                   config.maxMem_,
                                   config.recSize_,
                                   config.sortType_.doNotAllocRec_,
                                   config.keySize_,
                                   scratch_,
                                   TRUE,
                                   config.heapAddr_,
                                   &sortError_,
                                   explainNodeId_,
                                   this);
  }
  if (sortAlgo_ == NULL)
  {
    sortError_.setErrorInfo(EScrNoMemory   //sort error
                            ,NULL          //syserr: the actual FS error
                            ,NULL          //syserrdetail
                            ,"SortUtil::sortInitialize"     //methodname
                            );
    return SORT_FAILURE;
  }

  //The maximum memory that sort can consume is governed by three parameters.
  //(config.maxNumBuffers_ * sorttdb.maxbufferSize) + config..maxMem_.
  //yield any excess quota to other operators.
  overheadPerRecord_ = sortAlgo_->getOverheadPerRecord();
  returnExcessMemoryQuota(overheadPerRecord_);
  
  //--------------------------------------------------------------------
  // The Class SortAlgo by default assumes internal sorting. If we are
  // using external sort we should indicate it to SortAlgo.
  //--------------------------------------------------------------------
  state_ = SORT_SEND;

  return SORT_SUCCESS; 
}

//----------------------------------------------------------------------
// Name         : sortEnd
// 
// Parameters   : none
//
// Description  : Does any de-init tasks 
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------
NABoolean SortUtil::sortEnd(void)
{
  state_ = SORT_END;
  doCleanUp();
  
  //Yield any excess quota before exiting this operator. Note that
  //yield retains the original quota for next round if this operator
  //gets used.
  returnExcessMemoryQuota(overheadPerRecord_);

//  SQLMXLoggingArea::logExecRtInfo(NULL,0,"Sort operation has ended", explainNodeId_);
  return SORT_SUCCESS;
}



//----------------------------------------------------------------------
// Name         : sortSend
// 
// Parameters   : ...
//
// Description  : 
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------
Lng32 SortUtil::sortSend(void* record, ULng32 len, void* tupp)
{
  stats_.numRecs_++;
  return sortAlgo_->sortSend(record, len, tupp);
}


//----------------------------------------------------------------------
// Name         : sortClientOutOfMem(void)
// 
// Parameters   : ...
//
// Description  : 
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------
Lng32 SortUtil::sortClientOutOfMem(void)
{
  Lng32 retcode = SORT_SUCCESS;
  retcode = sortAlgo_->sortClientOutOfMem() ;
  if (retcode != SCRATCH_SUCCESS)
       return retcode;
  sortAlgo_->setExternalSort();
  return SORT_SUCCESS;
}

//----------------------------------------------------------------------
// Name         : sortSendEnd
// 
// Parameters   : ...
//
// Description  : 
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------
Lng32 SortUtil::sortSendEnd(NABoolean& internalSort)
{
  Lng32 retcode = SORT_SUCCESS;
  state_ = SORT_SEND_END;

  retcode =   sortAlgo_->sortSendEnd() ;

  if (retcode)
   return retcode;

  if (sortAlgo_->isInternalSort()) 
  {
    internalSort = TRUE_L;
    if(config_->logInfoEvent())
    {
      char msg[500];
      str_sprintf(msg,
      "Sort is performing internal sort: NumRecs:%d", stats_.numRecs_);
      
      SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
    }
  }
  else 
  {
    internalSort = FALSE_L;
    retcode =  sortSendEndProcessing() ; 
    return retcode;
  }

  return retcode;
}

//----------------------------------------------------------------------
// Name         : sortSendEndProcessing
// 
// Parameters   : ...
//
// Description  : 
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------
Lng32 SortUtil::sortSendEndProcessing(void)
{
  ULng32 initialRunSize = 0;
  Lng32 runnum = 1L;
  SortScratchSpace* tempScratch;
  
  state_ = SORT_MERGE;

  //when we reach here, scratch must be populated.
  stats_.numCompares_ += sortAlgo_->getNumOfCompares();
  tempScratch = sortAlgo_->getScratch();
  stats_.scrNumBlocks_ = tempScratch->getTotalNumOfScrBlocks();
  
 // Total memory used
#pragma nowarn(1506)   // warning elimination 
  stats_.memSizeB_ = sortAlgo_->getRunSize()*sizeof(Record) +
  sortAlgo_->getRunSize()*sizeof(RecKeyBuffer);
#pragma warn(1506)  // warning elimination 

  initialRunSize = sortAlgo_->getRunSize();
 
  tempScratch->getTotalIoWaitTime(stats_.ioWaitTime_);
  DeleteSortAlgo(); //scratch_ in sortAlgo = scratch_ in SortUtil, not deleted

#ifdef FORDEBUG
  //-----------------------------------------------
  // Output after initial run phase.
  //----------------------------------------------- 
  cout << "Output after initial run phase." << endl;
  scratch_->readData(config_->recSize_);
#endif


  stats_.numInitRuns_ = scratch_->getTotalNumOfRuns();

  stats_.numRuns_ = stats_.numInitRuns_; //why is stats_.numRuns needed?
  
  //Use memory available as indicated by quota system. Else use the one specified
  //by SORT_MAX_HEAP_SIZE_MB
  if(memoryQuotaSystemEnabled())
  {
    //It is during this phase we need the most memory. Hence we will try to grab
    //as much as possible( no memory cap enforced). When we reach this phase,
    //the assumption is all the lower operators will have yielded memory.
    UInt32 maxMergeMemory =
        estimateMemoryToAvoidIntMerge(stats_.numRuns_, config_->sortMergeBlocksPerBuffer_);
    
    if(maxMergeMemory > (getMaxAvailableQuotaMB() * ONE_MB))
    {
      maxMergeMemory = getMaxAvailableQuotaMB() * ONE_MB;  
    }
    
    if(!consumeMemoryQuota(maxMergeMemory))
    {
      //If we reach here, then the system is under memory 
      //pressure. The only options we have is either return back
      //error or continue processing using minimum amount of memory
      //outside the memory quota system. Allocating memory at this
      //point( when under memory pressure) may cause some paging and
      //slow down processing, but it will atleast continue processing.
        
      //memoryQuotaUtil_ is not initialized in this case since this
      //amount of memory is not part of the quota system anymore.
      //Setting mergeOrder to 2 is the minimum value for merging to
      //to proceed.
      config_->mergeOrder_ = 2;
      
      //Log this memory pressure that is encountered, explicitly. 
      {
        char msg[500];
        str_sprintf(msg,
        "Sort Merge Phase encountered memory pressure: NumRuns:%d, MergeOrder:%d, NumRecs:%d, ScratchBlocks:%d",
        stats_.numRuns_, config_->mergeOrder_, stats_.numRecs_,stats_.scrNumBlocks_);
        
        SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
      }
      
    }
    else
    {
      memoryQuotaUtil_ = maxMergeMemory;
      config_->mergeOrder_ = estimateMergeOrder(maxMergeMemory, config_->sortMergeBlocksPerBuffer_);
    }
  }
  else
  {
    //compute merge order based on memory set by CQD SORT_MAX_HEAP_SIZE_MB 
    //instead of quota system.
    UInt32 maxMem;
    config_->getSortMaxMemory(maxMem);
    config_->mergeOrder_= estimateMergeOrder(maxMem, config_->sortMergeBlocksPerBuffer_);
  }
  
  //If by chance the mergeOder turns out to be less then 2, it is readjusted
  //to 2 to avoid looping.
  if(config_->mergeOrder_ < 2) config_->mergeOrder_ = 2;

  Lng32 mergeOrderUsed = (stats_.numRuns_ > (Lng32) config_->mergeOrder_)? (Lng32) config_->mergeOrder_ : stats_.numRuns_;
  RESULT result = scratch_->setupSortMergeBufferPool(mergeOrderUsed * SORT_MERGENODE_NUM_BUFFERS * config_->sortMergeBlocksPerBuffer_);

  if(result != SCRATCH_SUCCESS)
  {
    if(memoryQuotaSystemEnabled() || (mergeOrderUsed <= 2))
    {
      //Return error if memory quota is enabled because, even
      //after all the memory quota and memory pressure checks,
      //allocation failed.
      //Also return error if merge order is least value of 2.
      //This is a really tight situation where memory is scarce.
      sortError_.setErrorInfo( EScrNoMemory   //sort error
		              ,NULL          //syserr: the actual FS error
		              ,NULL          //syserrdetail
		              ,"SortUtil::sortSendEndProcessing1"     //methodname
		              );
      return SORT_FAILURE;
    }
    else
    {
      //Because we computed merge order based on CQD, it may be possible
      //that sort is unable to get all the memory to service the merge order
      //request. In this case, retry memory allocation for half the merge order.
      //retry until merge order goes to its lowest level order of 2. If we 
      //are unable to allocate memory for merger order of 2, then return error.
      while(result != SCRATCH_SUCCESS)
      {
        mergeOrderUsed = mergeOrderUsed / 2;
        if(mergeOrderUsed < 2 ) break;

        result = scratch_->setupSortMergeBufferPool(mergeOrderUsed *
                                                    SORT_MERGENODE_NUM_BUFFERS * 
                                                    config_->sortMergeBlocksPerBuffer_);
      }
      if(result != SCRATCH_SUCCESS)
      {
        sortError_.setErrorInfo( EScrNoMemory   //sort error
		              ,NULL          //syserr: the actual FS error
		              ,NULL          //syserrdetail
		              ,"SortUtil::sortSendEndProcessing2" //methodname
		              );
        return SORT_FAILURE;

      }
      else
      {
        config_->mergeOrder_ = mergeOrderUsed;
      }
    }
  }

  //This is the result merge order. Set this in stats_.mergeOrder_.
  stats_.mergeOrder_ = config_->mergeOrder_;

  if (stats_.numRuns_ > (Lng32) config_->mergeOrder_)
  {
    state_ = SORT_INTERMEDIATE_MERGE;

    if (config_->logInfoEvent())
    {
      char msg[500];
      str_sprintf(msg,
      "Sort is performing intermediate merges, NumRuns:%d, MergeOrder:%d, NumRecs:%d, ScratchBlocks:%d",
      stats_.numRuns_,config_->mergeOrder_,stats_.numRecs_,stats_.scrNumBlocks_); 
    
      SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
    }
 
    while (TRUE_L) 
    {   
     stats_.numInterPasses_ = (stats_.numRuns_/stats_.mergeOrder_);
     stats_.firstMergeOrder_ = 1 + (stats_.numRuns_ - 
                               (stats_.numInterPasses_*stats_.mergeOrder_) );
     if (stats_.firstMergeOrder_ > 1) { 
        sortAlgo_ = new (config_->heapAddr_) Tree(stats_.firstMergeOrder_, 
					          initialRunSize,
                                                  config_->recSize_,
                                                  FALSE_L,
                                                  config_->keySize_,
                                                  scratch_,
                                                  config_->heapAddr_,
                                                  &sortError_,
                                                  explainNodeId_,
                                                  this,
                                                  runnum, 
                                                  TRUE_L, TRUE);
        if (sortError_.getSortError() != 0)
           return SORT_FAILURE;
      
        if ( sortAlgo_->generateInterRuns() )
        {
          // Extract the low level error from the sortError_
          // object from the scratch_ member
          short scratchError = scratch_->getSortError()->getSysError();
          short lowLevelSortError = scratch_->getSortError()->getSortError();
          sortError_.setErrorInfo( EInterRuns     //sort error
              ,scratchError          //syserr: the actual FS error
              ,lowLevelSortError          //syserrdetail
              ,"SortUtil::sortSendEndProcessing3"     //methodname
              );
          return SORT_FAILURE;
        }
        runnum = runnum + stats_.firstMergeOrder_;
        stats_.numCompares_ += sortAlgo_->getNumOfCompares();
        tempScratch = sortAlgo_->getScratch();
        stats_.scrNumBlocks_ = tempScratch->getTotalNumOfScrBlocks();
        tempScratch->getTotalIoWaitTime(stats_.ioWaitTime_);
        DeleteSortAlgo();  //scratch_ in sortAlgo = scratch_ in SortUtil,
                           // not deleted

        //First merge order completed. Now call scratch file cleanup
        //to free up overflow space. Clean up or close scratch files that
        //are not needed anymore. Also note that runnum is already incremented
        //to point to next run.
        if(config_->intermediateScratchCleanup())
        {
          if(tempScratch->cleanupScratchFiles(runnum-1) != SCRATCH_SUCCESS)
          {
            sortError_.setErrorInfo( EInterRuns   //sort error
                                  ,NULL          //syserr: the actual FS error
                                  ,NULL          //syserrdetail
                                  ,"SortUtil::sortSendEndprocessing4"     //methodname
                                );
            return SORT_FAILURE;
          }
        }
      }

      //subsequent merges in loop.
      for (Int32 i = 0; i < stats_.numInterPasses_; i++)
      {   
        sortAlgo_ = new (config_->heapAddr_) Tree(stats_.mergeOrder_, 
					          initialRunSize,
                                                  config_->recSize_,
                                                  FALSE_L,
                                                  config_->keySize_,
                                                  scratch_,
                                                  config_->heapAddr_,
                                                  &sortError_,
                                                  explainNodeId_,
                                                  this,
                                                  runnum, 
                                                  TRUE_L,TRUE);
        if (sortAlgo_ == NULL)
        {
          sortError_.setErrorInfo( EScrNoMemory   //sort error
                                  ,NULL          //syserr: the actual FS error
                                  ,NULL          //syserrdetail
                                  ,"SortUtil::sortSendEndProcessing5"     //methodname
                                 );
          return SORT_FAILURE;
        }

        if (sortError_.getSortError() != 0)
            return SORT_FAILURE;

        if ( sortAlgo_->generateInterRuns())
        {
          short scratchError = scratch_->getSortError()->getSysError();
          short lowLevelSortError = scratch_->getSortError()->getSortError();
          sortError_.setErrorInfo( EInterRuns    //sort error
                                  ,scratchError  //syserr: the actual FS error
                                  ,lowLevelSortError          //syserrdetail
                                  ,"SortUtil::sortSendEndProcessing6"     //methodname
                                );
          return SORT_FAILURE;
        }
        runnum = runnum + stats_.mergeOrder_;
        stats_.numCompares_ += sortAlgo_->getNumOfCompares();
        tempScratch = sortAlgo_->getScratch();
        stats_.scrNumBlocks_ = tempScratch->getTotalNumOfScrBlocks();
        tempScratch->getTotalIoWaitTime(stats_.ioWaitTime_); 
        DeleteSortAlgo();  //scratch_ in sortAlgo = scratch_ in SortUtil,
	                    // not deleted

        //Now call scratch file cleanup to free up overflow space.
        //Clean up or close scratch files that are not needed anymore.
        if(config_->intermediateScratchCleanup())
        {
          if(tempScratch->cleanupScratchFiles(runnum-1) != SCRATCH_SUCCESS)
          {
            sortError_.setErrorInfo( EInterRuns   //sort error
                                  ,NULL          //syserr: the actual FS error
                                  ,NULL          //syserrdetail
                                  ,"SortUtil::sortSendEndprocessing7"     //methodname
                                );
            return SORT_FAILURE;
          }
        }//if cleanup enabled
      } // end of the for loop

      if ((stats_.numInterPasses_) > stats_.mergeOrder_)
      {
        stats_.numRuns_ = stats_.numInterPasses_;
      } 
      else
      {
        break;
      } 
   
    }  // End While TRUE_L.

    stats_.finalMergeOrder_ = stats_.numInterPasses_;
  }
  else
  {
    stats_.mergeOrder_ = 0L; 
    stats_.finalMergeOrder_ = stats_.numRuns_;
    if (config_->logInfoEvent())
    {
      char msg[500];
      str_sprintf(msg,
      "Sort is not performing intermediate merges, NumRuns:%d, MergeOrder:%d, NumRecs:%d, ScratchBlocks:%d",
      stats_.numRuns_,config_->mergeOrder_,stats_.numRecs_,stats_.scrNumBlocks_); 
      
      SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
    } 
  }

  state_ = SORT_FINAL_MERGE;

  stats_.scrNumBlocks_ = scratch_->getTotalNumOfScrBlocks();
  ScratchFileMap* tempFilesMap;
  tempFilesMap = scratch_->getScrFilesMap();
  //stats_.scrNumWrites_ = tempFilesMap->totalNumOfWrites();
  //stats_.scrNumAwaitio_ = tempFilesMap->totalNumOfAwaitio();  
  scratch_->getTotalIoWaitTime(stats_.ioWaitTime_);
  stats_.memSizeB_ += stats_.finalMergeOrder_*stats_.scrBlockSize_*2 +
                     stats_.finalMergeOrder_*sizeof(TreeNode) +
                     stats_.finalMergeOrder_*sizeof(Record);
  
  //Final Merge 
  sortAlgo_ = new (config_->heapAddr_) Tree(stats_.finalMergeOrder_,
					    initialRunSize,  
                                            config_->recSize_,
                                            FALSE_L,
                                            config_->keySize_,
                                            scratch_,
                                            config_->heapAddr_,
                                            &sortError_,
                                            explainNodeId_,
                                            this,
                                            runnum, 
                                            TRUE_L,
                                            FALSE);
  if (sortAlgo_ == NULL)
  {
    sortError_.setErrorInfo( EScrNoMemory   //sort error
                            ,NULL          //syserr: the actual FS error
                            ,NULL          //syserrdetail
                            ,"SortUtil::sortSendEndprocessing8"     //methodname
                           );
    return SORT_FAILURE;
  }
  if (sortError_.getSortError()!= 0)
     return SORT_FAILURE;

  state_ = SORT_RECEIVE;

 return SORT_SUCCESS;

}
  

//----------------------------------------------------------------------
// Name         : sortReceive
// 
// Parameters   : ...
//
// Description  : 
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------
Lng32 SortUtil::sortReceive(void* record, ULng32& len)
{
  Lng32 status;
  status = sortAlgo_->sortReceive(record, len);
  if ((len == 0) && (!config_->partialSort_)) {
    if(scratch_)
     {
        ScratchFileMap* tempFilesMap;
        tempFilesMap = scratch_->getScrFilesMap();
        //stats_.scrNumReads_ = tempFilesMap->totalNumOfReads();
        //stats_.scrNumAwaitio_ = tempFilesMap->totalNumOfAwaitio();
        scratch_->getTotalIoWaitTime(stats_.ioWaitTime_);    
     }
    stats_.numCompares_ += sortAlgo_->getNumOfCompares();
    Int64 currentTimeJ = NA_JulianTimestamp();
    stats_.elapsedTime_ = currentTimeJ - stats_.beginSortTime_;
    if (config_->logInfoEvent()) {
      char msg[500];
      str_sprintf(msg, "Sort elapsed time : %Ld; Num runs : %d; runsize :%d",
		stats_.elapsedTime_,stats_.numInitRuns_,sortAlgo_->getRunSize());
      SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
    }
  } 
  return status;
}

//----------------------------------------------------------------------
// Name         : sortReceive
// 
// Parameters   : ...
//
// Description  : 
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------
Lng32 SortUtil::sortReceive(void*& record,ULng32& len,void*& tupp)
{
  NABoolean status;
  status = sortAlgo_->sortReceive(record, len, tupp);
  if ((len == 0) && (!config_->partialSort_)) {
    if(scratch_)
     {
      ScratchFileMap* tempFilesMap;
      tempFilesMap = scratch_->getScrFilesMap();
      //stats_.scrNumReads_ = tempFilesMap->totalNumOfReads();
      //stats_.scrNumAwaitio_ = tempFilesMap->totalNumOfAwaitio();      
      scratch_->getTotalIoWaitTime(stats_.ioWaitTime_);    
     }
    stats_.numCompares_ += sortAlgo_->getNumOfCompares();  
    Int64 currentTimeJ = NA_JulianTimestamp();
    stats_.elapsedTime_ = currentTimeJ - stats_.beginSortTime_; 
    if (config_->logInfoEvent()) {
      char msg[500];
      str_sprintf(msg, "Sort elapsed time : %Ld; Num runs : %d; runsize :%d",
		stats_.elapsedTime_,stats_.numInitRuns_,sortAlgo_->getRunSize());
      SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
    }
  } 
  return status;
}

//----------------------------------------------------------------------
// Name         : getStatistics
// 
// Parameters   : ...
//
// Description  : 
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------
void SortUtil::getStatistics(SortStatistics* statistics)
{
 memcpy((char*)statistics, (char*)&stats_, sizeof(SortStatistics));
}


//----------------------------------------------------------------------
// Name         : getSortError
// 
// Parameters   : SortError* srterr
//
// Description  : 
//
// Return Value : na
//
//----------------------------------------------------------------------
void SortUtil::getSortError(SortError* srterr)
{
 memcpy((char*)srterr, (char*)&sortError_, sizeof(SortError));
} 

//----------------------------------------------------------------------
// Name         : getSortError
//
// Parameters   : none
//
// Description  :
//
// Return Value : sort error number (SQL error values)
//
//----------------------------------------------------------------------
short SortUtil::getSortError() const
{
 return sortError_.getSortError();
}


void SortUtil::doCleanUp()
{
  //Delete Scratch last because algorithm may reference scratch internally.

  if (sortAlgo_ != NULL) {
    delete sortAlgo_;
    sortAlgo_ = NULL;
  }

  if (scratch_ != NULL) {
    delete scratch_; // This will complete all outstanding I/Os. 
    scratch_ = NULL;
  }
  
  //Memory quota consumed at each level(util level or sortAlgo level
  //must be tracked and cleanedup at its own level. Deleting sortAlgo_
  //above would have cleaned up memoryquota if consumed.
  returnConsumedMemoryQuota(memoryQuotaUtil_);
  memoryQuotaUtil_ = 0;
     
  sortError_.initSortError();
}

void SortUtil::setErrorInfo(short sorterr, short syserr, short syserrdetail, char *errorMsg)
{
  sortError_.setErrorInfo(sorterr,
                 syserr,
                 syserrdetail,
                 errorMsg);
              
}

NABoolean SortUtil::consumeMemoryQuota(UInt32 bufferSizeBytes)
{
  //BMO only enabled if greater than zero.
  if(config_ == NULL || config_->initialMemoryQuotaMB_ <= 0) 
    return FALSE;
  
  //Just consume the available quota if well within limits.    
  if(((config_->memoryQuotaUsedBytes_ + bufferSizeBytes)/ONE_MB) <= 
        (UInt32)config_->memoryQuotaMB_)
  {
    //Before consuming the memory quota, check for memory pressure.
    if(! this->withinMemoryLimitsAndPressure(bufferSizeBytes))
    {
      return FALSE;
    }
    config_->memoryQuotaUsedBytes_ += bufferSizeBytes;
    return TRUE;
  }
  else
  {
    //Attempt to grab quota from globals if existing quota is insufficient,
    //Note this is not alloaction of memory,it is just memory quota.
    //The actual memory is allocated by the caller.
    
    //calculate how much memory quota is needed taking into account any
    //leftover quota.
    UInt32 memNeededMB = 
         (config_->memoryQuotaUsedBytes_ + bufferSizeBytes -
          (config_->memoryQuotaMB_ * ONE_MB))/ONE_MB;
               
    ExExeStmtGlobals* exe_glob = 
          config_->callingTcb_->getGlobals()->castToExExeStmtGlobals();
    
    //Check if memory quota is available.
    if(exe_glob->unusedMemoryQuota() < memNeededMB)
    {
      return FALSE;
    }
// LCOV_EXCL_START
    //artificially increase the quota before pressure check.
    //if pressure is detected, decrement the quota back.
    config_->memoryQuotaMB_ += (short)memNeededMB;
    if(this->withinMemoryLimitsAndPressure(bufferSizeBytes))
    {
      if(exe_glob->grabMemoryQuotaIfAvailable(memNeededMB))
      {
        config_->memoryQuotaUsedBytes_ += bufferSizeBytes;
        return TRUE;
      }
      else
      {
        //decrement quota back.
        config_->memoryQuotaMB_ -= (short)memNeededMB;
      }
    }
    else
    {
      //decrement quota back.
      config_->memoryQuotaMB_ -= (short)memNeededMB;
    }
  }
// LCOV_EXCL_STOP
  return FALSE;//memory grab failed or reached limit.
}

void SortUtil::returnConsumedMemoryQuota(UInt32 bufferSizeBytes)
{
  //BMO only enabled if greater than zero.
  if(config_ == NULL || config_->initialMemoryQuotaMB_ <= 0)
    return;
  config_->memoryQuotaUsedBytes_ -= bufferSizeBytes;
}


void SortUtil::returnExcessMemoryQuota(UInt32 overheadPerRecord)
{
  //BMO only enabled if greater than zero.
  if(config_ == NULL || config_->initialMemoryQuotaMB_ <= 0)
    return;
  
  //At minimum we want to retain memory quota that generator assigned us.
  UInt32 maxEstimateMemoryBytes = config_->initialMemoryQuotaMB_ * ONE_MB;  
  
  //At minimum we want to retain memory quota that we currently have consumed.
  if(config_->memoryQuotaUsedBytes_ > maxEstimateMemoryBytes )
    maxEstimateMemoryBytes = config_->memoryQuotaUsedBytes_;
   
  //Lets check if we have more quota on hand that we can expense.
  Int32 excessMemoryQuotaMB = 
    config_->memoryQuotaMB_ - (Int16)(maxEstimateMemoryBytes/ONE_MB);
  
  //if no more quota on hand, return, nothing much we can do now.
  if(excessMemoryQuotaMB <= 0)
    return; 
  
  //Since we have more quota, lets estimate how much we may need. The next
  //set of calculations will determine max amount of quota we may need. This
  //estimation is irrespective of how much we have already consumed now. We 
  //will return any excess quota that is greater than this estimation.
  
  // We use config_->maxNumBuffers_-1 because the initial buffer is allocated
  // is allocated outside the memory quota system.
  // Amount of memory to read the tupps from child.
  maxEstimateMemoryBytes =
      (((ExSortTcb*)config_->callingTcb_)->sortBufferSize()) *
      (config_->maxNumBuffers_ -1);

  //Calculate additional overhead involved in sorting the records.
  UInt32 tuppSize = SqlBufferGetTuppSize((Lng32)config_->getRecSize());
  
  //estimateMemory/tuppSize = estimate of records that can be accomodated
  //in memory.
  maxEstimateMemoryBytes = 
    maxEstimateMemoryBytes + ((maxEstimateMemoryBytes/tuppSize) * overheadPerRecord );
          
  excessMemoryQuotaMB = 
    config_->memoryQuotaMB_ - (Int16)(maxEstimateMemoryBytes/ONE_MB);
    
  if(excessMemoryQuotaMB > 0)
  {
    ExExeStmtGlobals* exe_glob = 
        config_->callingTcb_->getGlobals()->castToExExeStmtGlobals();
    
    exe_glob->yieldMemoryQuota((UInt32) excessMemoryQuotaMB);
    config_->memoryQuotaMB_ -= (short)excessMemoryQuotaMB;
  }      
}    

UInt32 SortUtil::getMaxAvailableQuotaMB(void)
{
   //BMO only enabled if greater than zero.
   if(config_ == NULL || config_->initialMemoryQuotaMB_ <= 0)
    return 0;
   
   ExExeStmtGlobals* exe_glob = 
          config_->callingTcb_->getGlobals()->castToExExeStmtGlobals();
   
   UInt32 maxAvailableQuotaMB =
            config_->memoryQuotaMB_ -
            (config_->memoryQuotaUsedBytes_/ONE_MB) +
            exe_glob->unusedMemoryQuota();
                   
   return (maxAvailableQuotaMB);
}

//The memory checks performed in this method pertain to 
//overall memory consumption of the operator.
NABoolean SortUtil::withinMemoryLimitsAndPressure(Int64 reqMembytes)
{
  //BMO only enabled if greater than zero.
  if(config_ == NULL || config_->initialMemoryQuotaMB_ <= 0)
    return FALSE;

  //config_.heapAddr_->getUsage will return false as long as there exists a 
  //possibility of allocating additional executor segments when there
  //is demand for memory allocation. Once we reach a limit of allocating
  //the last possible segment(segment size being 32MB) this call returns
  //true. This is an indication that we are reaching limits.
    
  size_t lastSegSize, freeSize, totalSize;
  if((config_->heapAddr_)->getUsage(&lastSegSize, &freeSize, &totalSize)) {
// LCOV_EXCL_START
    if(config_->logInfoEvent())
    {
      char msg[500];
      str_sprintf(msg,
      "Sort encountered memory pressure: Memory used %Ld, total size %d, last seg size %d",
       config_->memoryQuotaUsedBytes_,totalSize, lastSegSize);
      
      SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
    }
    return FALSE;
// LCOV_EXCL_STOP
  }
  
  if(!memMonitor_)
  {
// LCOV_EXCL_START
     memMonitor_ =
     config_->callingTcb_->getGlobals()->castToExExeStmtGlobals()->getMemoryMonitor();
     ex_assert(memMonitor_ != NULL, "SortUtil::withinMemoryLimitsAndPressure, memMonitor_ is NULL");
// LCOV_EXCL_STOP
  }


  /*
  Compiler hints:
  ================
  Compiler provides two hints to BMO operators.

  1. bmoExpectedSize:
      This is the compiler estimate of total memory usage by the BMO operator

  2. bmoEstErrorPenalty
      This is a fraction penalty factor that can be used at runtime to adjust the estimated bmo size as described below.

  Explanation:
  -------------
  bmoEstErrorPenalty can also be viewed as uncertainty penalty factor. Higher the uncertainty of expected memory consumption, better to overflow early.

  bmoEstErrorPenalty is derived by the following equation:

  uncertainty =  { [max potential cardinality - expected cardinality] / expected cardinality  } * 100%

  bmoEstErrorPenalty is set to 10% if uncertainty <= 100%, else bmogrowthpercent is set at 25%.


  Executor usage:
  ---------------
  On Seaquest, operators overflow once memory quota is reached or physical memory pressure is detected. Memory pressure is currently disabled but eventually memory pressure detection mechanisms would be enabled.

  Memory quota limits and memory pressure checks do help enforce limits, however BMO operators could utilize compiler hints to trigger overflow and behave as a good citizen even before quota limits and memory pressure enforcements kick in.

  BMO operators often consume small chunks of memory or buffer, in repeated cycles until memory quota limit is reached. Compiler hints would be exercised each time a buffer allocation is required. However the size of buffer allocation is not relevant in the calculation below.

  C = Total consumed memory by BMO operator. Initially this is zero.

  E = Total estimated size of memory that will be consumed by BMO. This is initially the bmoExpectedSize hint given by compiler, but adjusted if C started exceeding the estimate.
    E = max(bmoExpectedSize, C * (1+bmoEstErrorPenalty) )
    This allow us to modify the estimate at runtime after receiving more data and penalize operators that have the chance of being much bigger than initially thought. Note, it's possible for the runtime logic to increase the value of bmoEstErrorPenalty on its own after certain threshold (e.g C > 10*bmoExpectedSize) but bmoEstErrorPenalty should not exceed 1 i.e. 100%.

  Z = CitizenshipFactor which is a parameter representing how much of the available physical memory an operator can assume for itself as compared to leaving it to other potential players. It can have the value of 1>= Z > 0. While we can suggest an initial value e.g. 0.5 we think this is better to set after tuning based on some performance tests. In the future this may also be adjusted as input from WMS based on existing concurrency on the system.

  M = Available physical memory at that instant.

  U = Contingency physical memory (e.g. 10% of Phys memory in the node. Point it to keep always phys memory available for executables, buffers, non-BMO ops, etc)

  m = [E- C]  or in other words the estimated delta memory required by BMO to complete processing.

  If ( m < Z * (M-U))  then continue memory allocation 
   else overflow.
  */
// Hints checks can be disable with the CQD EXE_BMO_DISABLE_CMP_HINTS_OVERFLOW
if(!config_->getDisableCmpHintsOverflow())
{
  Float32 M = (Float32) memMonitor_->availablePhyMemKb()/1024; //free physical memory in MB
  Float32 C = config_->memoryQuotaUsedBytes_/ONE_MB; //consumed memory by sort

  //U : minimum percent free physical memory to be spared for other players 
  //on the node. Could be other processes, etc.
  Float32 U = (Float32)config_->getMemoryContingencyMB(); 

  //Z: percent free physical memory to assume for myself and thereby allowing
  //reminaing free space for other operators in my esp process. WMS could set
  //this based on concurrency, there by it can increase the desity of BMO operators
  //residing on the node.
  Float32 Z = config_->getBmoCitizenshipFactor();

  Float32 m = 0;  //delta memory required to avoid overflow.

  //do the following check if sort has not overflowed.
  if(state_ < SORT_SEND_END)
  {
    Float32 E = config_->getSortMemEstInMbPerCpu();   //expected memory consumption by sort
    
#ifdef FUTURE_WORK
    //check extreme case first. Expected cannot be more than
    //available quota.
    if( E >  config_->memoryQuotaMB_)
// LCOV_EXCL_LINE
      return FALSE;
#endif

    //general comments pasted above for explanation of this field.
    Float32 estimateErrorPenalty = config_->getEstimateErrorPenalty();
    
    //adjust expected memory to higher than consumed so that a target expected 
    //memory is readjusted based on estimateErrorPenalty. estimateErrorPenalty is
    //high if confidance level of E is low and vice versa. This in essence creates  
    //large delta memory requirement for sort, which in essence may trigger overflow.
    if(C > E) //already
    {
// LCOV_EXCL_START
      E = MAXOF( E, C *( 1 + estimateErrorPenalty));
      config_->setSortMemEstInMbPerCpu(E);
// LCOV_EXCL_STOP
    }

    Float32 m = E - C;  //delta memory required to avoid overflow.
    
    //if delta memory required is more than physical memory available
    //overflow.
    if( m > ( Z * (M -U)))
    {
// LCOV_EXCL_START
      if(config_->logInfoEvent())
      {
        char msg[500];
        str_sprintf(msg,
          "Sort encountered memory pressure [m > (Z*(M-U))]: memReq m=%f, memFree M=%f, "
          "memExpected E=%f, memConsumed C=%f, estimateErrorPenalty e=%f,"
          "memContingency U=%f, citizenshipFactor Z=%f", m,M,E,C,estimateErrorPenalty,U,Z);
        SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
      }
      return FALSE;
// LCOV_EXCL_STOP
    }
  }
  else
  {
    //if sort has overflowed, memory checks done here is purely
    //memory requirements against available physical memory.

    m =  (Float32)reqMembytes/ONE_MB;

    if( m > ( Z * (M - U)))
    {
// LCOV_EXCL_START
      if(config_->logInfoEvent())
      {
        char msg[500];
        str_sprintf(msg,
          "Sort encountered memory pressure [m > (Z*(M-U))]: memReq m=%f, memFree M=%f, "
          "memContingency U=%f, citizenshipFactor Z=%f", m,M,U,Z);

        SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
      }
      return FALSE;
// LCOV_EXCL_STOP
    }
  }
}

  if(memMonitor_->memoryPressure() > config_->pressureThreshold_)
  {
// LCOV_EXCL_START
    if(config_->logInfoEvent())
    {
      char msg[500];
      str_sprintf(msg,
      "Sort encountered memory pressure: Memory pressure %d, Pressure Threshold %d",
       memMonitor_->memoryPressure(), config_->pressureThreshold_);
      
      SQLMXLoggingArea::logExecRtInfo(NULL, 0,msg, explainNodeId_);
    }
    return FALSE;
// LCOV_EXCL_STOP
  }

  //The following checks any threshold limits set by the user. This
  //check is static in nature and directly controlled by cqds
  //SSD_BMO_MAX_MEM_THRESHOLD_MB if SSD disk type is in use OR
  //EXE_MEMORY_AVAILABLE_IN_MB if disk type is in use.
  if( (config_->memoryQuotaUsedBytes_/ONE_MB) >=
       config_->bmoMaxMemThresholdMB_)
      return FALSE;

  return TRUE;  
}


void SortUtil::setupComputations(SortUtilConfig& config)
{
  config_ = &config;
  config.runSize_ = 2*config.runSize_; 
  
#pragma nowarn(1506)   // warning elimination 
    stats_.recLen_ = config.recSize_;
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
    stats_.runSize_ = config.runSize_;
#pragma warn(1506)  // warning elimination 

}  

UInt32 SortUtil::estimateMergeOrder(UInt32 maxMergeMemory, Int32 sortMergeBlocksPerBuffer)
{
  UInt32 memoryPerTreeNode = 
    ( (SORT_MERGENODE_NUM_BUFFERS * 
       sortMergeBlocksPerBuffer * 
       (sizeof(SortMergeBuffer) + config_->getScratchIOBlockSize())) + //We allocate IO block separately, hence..
       sizeof(TreeNode) + 
       sizeof(Record)+ 
       sizeof(SortMergeNode));
  return (maxMergeMemory / memoryPerTreeNode);
}

UInt32 SortUtil::estimateMemoryToAvoidIntMerge(UInt32 numruns,Int32 sortMergeBlocksPerBuffer)
{
  //treeNodes and Record structure are allocated as an array. 
  
  UInt32 maxMem = numruns *
                  SORT_MERGENODE_NUM_BUFFERS *
                  sortMergeBlocksPerBuffer *
                  (sizeof(SortMergeBuffer) + config_->getScratchIOBlockSize());
                                          //We allocate IO block separately, hence..
  
  //class objects
  maxMem += numruns * sizeof(SortMergeNode); 
  maxMem += numruns * sizeof(TreeNode);
  maxMem += numruns * sizeof(Record);
  return maxMem;
}
