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
#ifndef SORTUTIL_H
#define SORTUTIL_H

/* -*-C++-*-
******************************************************************************
*
* File:         SortUtil.h
* RCS:          $Id: SortUtil.h,v 1.2.16.1 1998/03/11 22:33:37  Exp $
*                               
* Description:  This file contains the definition of the SortUtil class. This
*               class is the interface to the callers like Executor for using
*               the sort utility. 
*
* Created:      07/12/96
* Modified:     $ $Date: 1998/03/11 22:33:37 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/

#include "ComSpace.h"
#include "CommonStructs.h"
#include "SortUtilCfg.h"
#include "SortAlgo.h"
#include "TourTree.h"
#include "Const.h"
#include "ScratchSpace.h"
#include "ScratchFileMap.h"
#include "Statistics.h"
#include "NABasicObject.h"

class MemoryMonitor;
class ExBMOStats;

class SortUtil : public NABasicObject {

public:
  SortUtil(Lng32 explainNodeId);             
  ~SortUtil();            

  NABoolean sortInitialize(SortUtilConfig& config, ULng32 topNSize);
  NABoolean sortEnd(void);
  
  Lng32 sortSend(void* record, ULng32 len, void* tupp);
  
  Lng32 sortClientOutOfMem(void);

  Lng32 sortSendEnd(NABoolean& internalSort);
 
  Lng32 sortReceive(void* record, ULng32& len);
  Lng32 sortReceive(void*& record, ULng32& len, void*& tupp);
  
  void DeleteSortAlgo();
  
  void getStatistics(SortStatistics* statistics);
  void getSortError(SortError* srterr);
  short getSortError() const;
  short getSortSysError() const { return sortError_.getSysError(); } 
  short getSortErrorDetail() const {return sortError_.getErrorDetail();}
  char *getSortErrorMsg() const {return sortError_.getSortErrorMsg();}
  void  setErrorInfo(short sorterr, short syserr = 0, short syserrdetail = 0, char *errorMsg = NULL);

  void doCleanUp();
  
  NABoolean consumeMemoryQuota(UInt32 bufferSizeBytes);
  void returnConsumedMemoryQuota(UInt32 bufferSizeBytes);
  void returnExcessMemoryQuota(UInt32 overheadPerRecord = 0);
  inline NABoolean memoryQuotaSystemEnabled(void)
  {
    //BMO not enabled if negative.
    return(config_ != NULL && config_->initialMemoryQuotaMB_ > 0);
  }
  UInt32 getMaxAvailableQuotaMB(void);
  NABoolean withinMemoryLimitsAndPressure(Int64 reqMembytes);

  SortUtilConfig * config(void){return config_;}
  SortScratchSpace* getScratch() const{return scratch_;} ;
  NABoolean scratchInitialize(void);
  void setupComputations(SortUtilConfig& config);
  void setBMOStats(ExBMOStats *stat) { bmoStats_ = stat; };
  UInt32 estimateMemoryToAvoidIntMerge(UInt32 numruns, Int32 sortMergeBlocksPerBuffer);
  UInt32 estimateMergeOrder(UInt32 maxMergeMemory, Int32 sortMergeBlocksPerBuffer);
protected :
  Lng32 sortReceivePrepare();

private:
  void reInit();
  short    version_;     // The ArkSort version
  SORT_STATE state_;      
  SortUtilConfig *config_;
  NABoolean internalSort_; //indicates if overflowed or not.
  NABoolean sortReceivePrepared_; //if sort overflowed, prepare the merge tree for receive.
  SortAlgo *sortAlgo_;   // Algorithms are implemented as sub-classes
                         // of  Sort algorithm base class.      
                         // This implementation  allows extensibility as
                         // new algorithms can be added easily.  This is
                         // similiar to the Policy design pattern 
                         // suggested by Enrich Gamma.
  
  SortScratchSpace *scratch_;// This object is used for all temporary work space
                         // needed by SortUtil.

  SortStatistics stats_; // A statistics object which accumulates
                         // statistics related to the sorting session.
 
  SortError sortError_;  // Error code of Last Sort error if any.
 
  Lng32 explainNodeId_;   // Help runtime reporting.
  
  UInt32 memoryQuotaUtil_;//memory quota consumed at the util level.
  
  MemoryMonitor *memMonitor_;//memory monitor for checking memory pressure.
  
  UInt32 overheadPerRecord_;//memory per record consumed by initial algorithm.
                            //The algorithm may change during sort processing.
                            //We need this value to retain the memory quota 
                            //when returning any excess quota consumed during
                            //sort processing.
  ExBMOStats *bmoStats_;

};

#endif
