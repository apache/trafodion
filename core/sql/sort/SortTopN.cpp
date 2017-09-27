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
* File:         SortTopN.cpp
*                               
* Description:  This file contains the implementation of all member functions
*               of the class TopN.
*               
* 1. Sort would initially maintain Top N array of elements to being with.
* 2. Read records into TopN array. 
* 3. Once TopN array is full, heapify the array into max heap. Top node in 
*    the heap is always the highest node.
* 4. Subsequent record read either gets discarded( if greater than top node)
*    or replace top node( if lesser then top node) . if replaced top node, 
*    re-balance the heap.
* 5. Repeat steps 4 until last record is read.
* 6. sort the final heap using heap sort.
*******************************************************************************/

#include <iostream>
#include <fstream>

#ifndef DEBUG
#undef NDEBUG
#define NDEBUG
#endif
#include "ex_stdh.h"
#include "SortTopN.h"
#include "ScratchSpace.h"
#include "logmxevent.h"
#include "SortUtil.h"
#include "ex_ex.h"
#include "ExStats.h"

//------------------------------------------------------------------------
// Class Constructor.
//------------------------------------------------------------------------
SortTopN::SortTopN(ULng32 runsize, ULng32 sortmaxmem, ULng32  recsize,
             NABoolean doNotallocRec, ULng32  keysize, 
             SortScratchSpace* scratch, NABoolean iterSort,
             CollHeap* heap, SortError* sorterror, Lng32 explainNodeId, ExBMOStats *bmoStats, SortUtil* sortutil):
             SortAlgo(runsize, recsize, doNotallocRec, keysize, scratch, explainNodeId, bmoStats),
             loopIndex_(0), heap_(heap), sortError_(sorterror),
             sortUtil_(sortutil)
{
  //runsize is TopN size. Fixed.
  allocRunSize_ = runsize;
  
  //Actual run size after all elements read.
  runSize_ = 0;   
  
  isHeapified_ = FALSE;
  
  topNKeys_  =  (RecKeyBuffer *) new (heap_) BYTE[sizeof(RecKeyBuffer) * allocRunSize_];  
  
  // Below asserts useful in debug mode. 
  ex_assert(topNKeys_  != NULL, "Sort: Initial topNKeys_ allocation failed");  
  
  recNum_ = 0;
  if (bmoStats_)
    bmoStats_->updateBMOHeapUsage((NAHeap *)heap_);
}

 
SortTopN::~SortTopN(void)
{
  if (topNKeys_ != NULL) 
  {
    //No need to release the tupps here
    //since these tupps are consumed by
    //parent operators and released.
    //for (int i = 0; i < runSize_; i++)
    //topNKeys_[i].rec_->releaseTupp();
    
    NADELETEBASIC(topNKeys_, heap_);
    topNKeys_ = NULL;
  }
  
  if (bmoStats_)
    bmoStats_->updateBMOHeapUsage((NAHeap *)heap_);
}

Lng32 SortTopN::sortSend(void *rec, ULng32 len, void* tupp)		                        
{
  //if heap not built means, TopN array has more slots 
  //available to fill. 
  if(! isHeapified_)
  {
    ex_assert(loopIndex_ >= 0, "TopN::sortSend: loopIndex_ is < 0");
    ex_assert(loopIndex_ < allocRunSize_, "TopN::sortSend: loopIndex_ > allocRunSize_");
    ex_assert(rec != NULL, "TopN::sortSend: rec is NULL");

    Record * newRec = new (heap_)Record(rec, len, tupp, heap_, sortError_);
  
    topNKeys_[loopIndex_].key_ = newRec->extractKey(keySize_, 
                            sortUtil_->config()->numberOfBytesForRecordSize());
    topNKeys_[loopIndex_].rec_ = newRec;
    if (++loopIndex_  == allocRunSize_)
    {
        //Reaching here means, we have filled up the array. 
        //Now heapify the array to start accepting/eliminating new elements from now on.
 
       //Note lookIndex_ contains the current number of filled elements.
        buildHeap();
    }
    if (bmoStats_)
       bmoStats_->updateBMOHeapUsage((NAHeap *)heap_);
    return SORT_SUCCESS;
  }
  
  //Reaching here means, heap is already built. 
  //Check if the new rec belongs to this heap by comparing the
  //new rec key with the root node of the heap ( root node is always the greatest).
  insertRec(rec, len, tupp);
  return SORT_SUCCESS;
}


void SortTopN::buildHeap() 
{
  if(!isHeapified_)
  {
    //loopIndex_ is now <= TopN
    runSize_ = loopIndex_;
    
    satisfyHeap();
    
    isHeapified_ = TRUE;
  }
}

//Satisfy Heap makes sure the heap is balanced maxHeap.
//Note this does not mean heap is sorted. It just makes sure
//the higher level nodes are greater than lower level nodes.
//Topmost node or root will be the highest.
void SortTopN::satisfyHeap() 
{
  //nothing to do if zero or one record.
  if(runSize_ <= 1)
    return;
  
  for (int i = (runSize_/2 ); i >= 0; i--)
    siftDown(topNKeys_, i, runSize_-1);
}


void SortTopN::insertRec(void *rec, ULng32 len, void* tupp) 
{
  ex_assert(isHeapified_, "TopN::insertRec: not heapified");
  
  int root = 0; //Always, root node is the zero'th element in array.
  
  Record * newrec = new (heap_)Record(rec, len, tupp, heap_, sortError_); 
  
  insertRecKey_.key_ = newrec->extractKey(keySize_, 
                  sortUtil_->config()->numberOfBytesForRecordSize());
  insertRecKey_.rec_ = newrec;
  
  if (compare(topNKeys_[root].key_ ,insertRecKey_.key_) == KEY1_IS_GREATER)
  {
    swap( &topNKeys_[root],&insertRecKey_);
  
    
    //After swap, satisfy or rebalance the heap.
    satisfyHeap();
    
  }
  
  //Once swapped, or not swapped, delete the unneeded record.
  //This step is very important to discard the unwanted record.
  //Note releaseTupp() also deletes the tupp allocated in sql 
  //buffer. This makes space for new records read in.
  insertRecKey_.rec_->releaseTupp();
  NADELETEBASIC(insertRecKey_.rec_, heap_);
  
}

Lng32 SortTopN::sortSendEnd()
{
  Lng32 retcode = SORT_SUCCESS;
  ex_assert(loopIndex_ >= 0, "TopN::sortSendEnd: loopIndex_ is < 0");
  
  buildHeap();
  sortHeap();
  
  return retcode;
}

//----------------------------------------------------------------------
// Name         : sortHeap
// 
//
// Description  : The heap is already balanced. This step sorts the heap.
//----------------------------------------------------------------------
void SortTopN::sortHeap()
{
  //nothing to do if zero or one record.
  if(runSize_ <= 1)
    return;
  
  for (int i = runSize_-1; i >= 1; i--)
  {
    swap(&topNKeys_[0],&topNKeys_[i]);
    siftDown(topNKeys_, 0, i-1);
  }
}

Lng32 SortTopN::sortReceive(void *rec, ULng32& len)
{
  //This method applicable to overflow records only
  return SORT_FAILURE;
}

Lng32 SortTopN::sortReceive(void*& rec, ULng32& len, void*& tupp)
{
  if (recNum_ < runSize_)
  {
    topNKeys_[recNum_].rec_->getRecordTupp(rec, recSize_, tupp);
    len = recSize_;
    recNum_++;
  }
  else
  {
    len = 0;
  }
  return SORT_SUCCESS;
}


//----------------------------------------------------------------------
// Name         : siftDown
// 
// Parameters   : ..
//
// Description  : Given the root node,rebalances the heap.Child nodes are less
//                value than parent nodes. Top most node or root contains
//                highest value.
//                 
//
//----------------------------------------------------------------------
void SortTopN::siftDown(RecKeyBuffer keysToSort[], Int64 root, Int64 bottom)
{
  Int64 done, maxChild;

  done = 0;
  while ((root*2 <= bottom) && (!done))
  {
    if (root*2 == bottom)
      maxChild = root * 2;
    else if (compare(keysToSort[root * 2].key_ ,
      keysToSort[root * 2 + 1].key_) >= KEY1_IS_GREATER)
      maxChild = root * 2;
    else
      maxChild = root * 2 + 1;

    if (compare(keysToSort[root].key_ ,keysToSort[maxChild].key_) <=KEY1_IS_SMALLER)
    {
      swap( &keysToSort[root],&keysToSort[maxChild]);
      root = maxChild;
      
    }
    else
      done = 1;
  }
}

//----------------------------------------------------------------------
// Name         : swap
// 
// Parameters   : ..
//
// Description  : Swaps two elements from the QuickSort workspace. May
//                consider making this inline rather than a seperate
//                procedure call for performance reasons.
//                 
// Recompareturn Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------

NABoolean SortTopN::swap(RecKeyBuffer* recKeyOne, RecKeyBuffer* recKeyTwo)
{
  char* tempKey;
  Record* tempRec;
  
  
  tempKey = recKeyOne->key_;
  tempRec = recKeyOne->rec_;
  
  
  recKeyOne->key_ = recKeyTwo->key_;
  recKeyOne->rec_ = recKeyTwo->rec_;
  
  
  recKeyTwo->key_ = tempKey;
  recKeyTwo->rec_ = tempRec;
  return SORT_SUCCESS;
}

UInt32 SortTopN::getOverheadPerRecord(void)
{
  return (sizeof(RecKeyBuffer) + sizeof(Record)); 
}
