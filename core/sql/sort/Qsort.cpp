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
* File:         Qsort.C
* RCS:          $Id: qsort.cpp,v 1.1 2006/11/01 01:44:37  Exp $
*                               
* Description:  This file contains the implementation of all member functions
*               of the class Qsort. Note that Qsort is derived from the 
*               SortAlgo base class which has a pure virtual function - sort.
*               
* Created:	    04/25/96
* Modified:     $ $Date: 2006/11/01 01:44:37 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
******************************************************************************
*/

#include <iostream>
#include <fstream>

#ifndef DEBUG
#undef NDEBUG
#define NDEBUG
#endif
#include "ex_stdh.h"
#include "Qsort.h"
#include "ScratchSpace.h"
#include "logmxevent.h"
#include "SortUtil.h"
#include "ex_ex.h"
#include "ExStats.h"

//------------------------------------------------------------------------
// Class Constructor.
//------------------------------------------------------------------------
Qsort::Qsort(ULng32 runsize, ULng32 sortmaxmem, ULng32  recsize,
             NABoolean doNotallocRec, ULng32  keysize, 
             SortScratchSpace* scratch, NABoolean iterSort,
             CollHeap* heap, SortError* sorterror, Lng32 explainNodeId, ExBMOStats *bmoStats, SortUtil* sortutil):
             SortAlgo(runsize, recsize, doNotallocRec, keysize, scratch, explainNodeId, bmoStats),
             currentRun_(1), loopIndex_(0), heap_(heap), sortError_(sorterror),
             sortMaxMem_(sortmaxmem), sortUtil_(sortutil)
{
  // For recursive quicksort we are going to limit the number of records we can ever sort
  // in memory to 20000. After that we will overflow to scratch. This limit
  // was introduced because with the recursive algorithm
  // we ran into stack space problems whiledoing large in-memory Sorts.
   isIterativeSort_ = iterSort;

   // we do not register this initial memory allocation under the memory quota
   // system. Memory quota system comes into play only in the following memory
   // allocations. This is done for sort to continue working when under 
   // extreme memory pressure environment.
   allocRunSize_ = 1000000;
   
  initialRunSize_ =  allocRunSize_;    //store initial size for later cleanup.
  
  //allocateMemory failureIsFatal is defaulted to TRUE means allocation failure results in 
  //longjump to jump handler defined in ex_sort.cpp. Only applicable on NSK.
  rootRecord_ = (Record *)heap_->allocateMemory(sizeof(Record) * allocRunSize_);  
  recKeys_    = (RecKeyBuffer *)heap_->allocateMemory(sizeof(RecKeyBuffer) * allocRunSize_);  
 
  // Below asserts useful in debug mode. Also asserts if longjmp did not happen.
  ex_assert(rootRecord_!= NULL, "Sort: Initial rootRecord_ allocation failed"); 
  ex_assert(recKeys_  != NULL, "Sort: Initial recKeys_ allocation failed");  

  //-----------------------------------------------------------------------
  // Set loopIndex_ back to 0.
  //-----------------------------------------------------------------------
  loopIndex_ = 0;
  recNum_ = 0;
  if (bmoStats_)
    bmoStats_->updateBMOHeapUsage((NAHeap *)heap_);
}

 
//------------------------------------------------------------------------
// Class Destructor: Delete all the heap space pointed by pointers in Qsort
//------------------------------------------------------------------------
Qsort::~Qsort(void)
{

  for (; recNum_ < loopIndex_; recNum_++)
    recKeys_[recNum_].rec_->releaseTupp();

  if (rootRecord_ != NULL) {
    NADELETEBASIC(rootRecord_, heap_);
    rootRecord_ = NULL;
  }
  if (recKeys_ != NULL) {
    NADELETEBASIC(recKeys_, heap_);
    recKeys_ = NULL;
  }
  this->cleanUpMemoryQuota();
  if (bmoStats_)
    bmoStats_->updateBMOHeapUsage((NAHeap *)heap_);

}


//----------------------------------------------------------------------
// Name         : sort
// 
// Parameters   : ...
//
// Description  : Qsort is derived from SortAlgo which has a 
//                pure virtual function called SortSen. Hence Qsort has to 
//                provide a sort member function. This function in turn 
//                calls the quickSort private member function which 
//                implements the core Quick Sort algorithm.
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------
Lng32 Qsort::sortSend(void *rec, ULng32 len, void* tupp)		                        
{
  ex_assert(loopIndex_ >= 0, "Qsort::sortSend: loopIndex_ is < 0");
  ex_assert(loopIndex_ < allocRunSize_, "Qsort::sortSend: loopIndex_ > allocRunSize_");
  ex_assert(sendNotDone_, "Qsort::sortSend: sendNotDone_ is false");
  ex_assert(rec != NULL, "Qsort::sortSend: rec is NULL");
  rootRecord_[loopIndex_].initialize(len /*recSize_*/, doNotallocRec_,
                                     heap_, sortError_);
  rootRecord_[loopIndex_].setRecordTupp(rec, recSize_, tupp);//??? recSize_ passed but not used???

  recKeys_[loopIndex_].key_ = rootRecord_[loopIndex_].extractKey(keySize_, sortUtil_->config()->numberOfBytesForRecordSize());
  recKeys_[loopIndex_].rec_ = &rootRecord_[loopIndex_];
  if (++loopIndex_ < allocRunSize_)
     return SORT_SUCCESS;

  // If we reach here, allocRunSize_ is not enough. Either overflow or double & copy.
  // For recursive we do not do the double and copy But for the iterative 
  // we can do it since we won't run out of stack space.
  NABoolean overFlow = FALSE;
  
  //In HYBRID mode, we are using mmap to overflow. no need to do double
  //and copy. 1M row sort with mmap is measured to provide good performance.
  //Hence, we continue doing the old way if it is HYBRID mode.  
  if(sortUtil_->config()->getScratchOverflowMode() != SCRATCH_MMAP)
  {
	  if (isIterativeSort_) 
	  {
		UInt32 estimateSize = 
		  2*allocRunSize_*(sizeof(RecKeyBuffer) + sizeof(Record));
		  
		//If memory quota system is not enabled, we would behave the old
		//way. Quota system will not be enabled if sort resides in the master
		//process.
		if(sortUtil_->memoryQuotaSystemEnabled())
		{
		  //Note that we are performing double and copy. So we may consume
		  //half the estimate size more than what is already consumed until
		  //copy completes. This is not optimal since we may overflow in 
		  //some cases where we could have avoided by consuming an extra half
		  //of estimate memory temporarily. However we cannot take chances and
		  //we have seen cases where memory allocation has failed because we 
		  //would have underestimated. 
		  //Also ensure each allocation does not exceed 127MB size.
		  if(((2 * allocRunSize_ * sizeof(RecKeyBuffer)) > MAX_ALLOC_SIZE) ||
			 ((2 * allocRunSize_ * sizeof(Record)) > MAX_ALLOC_SIZE )      ||
			 (!sortUtil_->consumeMemoryQuota(estimateSize)))
		  {
			overFlow = TRUE;
		  }
		}
		//Quota system not enabled means use CQD value set in sortMaxMem_.
		else if ( estimateSize >= sortMaxMem_ )
		{
		  overFlow = TRUE;
		}
		
		// Now try double and copy once the above quota system/memory pressure/memory limits
		// checks are passed.
		if(overFlow == FALSE)              
		{
		  ULng32 oldrunsize = allocRunSize_;
		  // Allocate a larger array and copy the old array to the new one
		  allocRunSize_ = allocRunSize_*2;

		  // allocateMemory failureIsFatal FALSE means we handle memory failure error.	
		  Record *tempRootRecord = (Record *)
					  heap_->allocateMemory(sizeof(Record) * allocRunSize_, FALSE);  
						  
		  //allocateMemory failureIsFatal FALSE means we handle memory failure error.
		  RecKeyBuffer *tempRecKeys  = (RecKeyBuffer *)
						heap_->allocateMemory(sizeof(RecKeyBuffer) * allocRunSize_, FALSE); 	
		  
		  if ((tempRootRecord == NULL) || (tempRecKeys == NULL))
		  {
			// if memory allocation fails even after memory quota and memory pressure
			// checks have passed, then it is possible that there is no free segment
			// available to honor the request. When this happens, it is better to overflow.
			overFlow = TRUE;

			// reset back allocRunsize
			allocRunSize_ = oldrunsize;

			// deallocate if memory was allocated partially.
			if(tempRootRecord != NULL) NADELETEBASIC(tempRootRecord, heap_);
			if(tempRecKeys != NULL) NADELETEBASIC(tempRecKeys, heap_);

			// return the estimate memory to memory quota system which we consumed
			// just before double & copy.
			if(sortUtil_->memoryQuotaSystemEnabled())
			  sortUtil_->returnConsumedMemoryQuota(estimateSize);

			if (sortUtil_->config()->logInfoEvent() && (!sortUtil_->config()->logInfoEventDone()))
			{
			  char msg[500];
			  str_sprintf(msg,
						  "QSort::SortSend, Sort has overflowed due to unavailable memory request of size %ld MB",
						   (sizeof(Record) * allocRunSize_ * 2)/ ONE_MB);
			  SQLMXLoggingArea::logExecRtInfo(NULL,0,msg,explainNodeId_);
		  sortUtil_->config()->setLogInfoEventDone();
			}
		  }
		  else
		  {
		  
			// Copy the old array to the first half of the new array
			memcpy (tempRootRecord,rootRecord_,sizeof(Record)*oldrunsize);

			// Loop through and :
			// 1.initialize the rest of the new array
			// 2. Assign the rec_ and key_ pointers to point to the new
			//    tempRootRecord array members

			ULng32 i = 0;
			for (i=0;i<oldrunsize;i++) 
			{
			 tempRecKeys[i].key_ = tempRootRecord[i].extractKey(keySize_, sortUtil_->config()->numberOfBytesForRecordSize());
			 tempRecKeys[i].rec_ = &tempRootRecord[i];
			}

			NADELETEBASIC(rootRecord_, heap_);
			NADELETEBASIC(recKeys_, heap_);
			
			rootRecord_ = tempRootRecord;
			recKeys_ = tempRecKeys;
			if (bmoStats_)
			   bmoStats_->updateBMOHeapUsage((NAHeap *)heap_);
			return SORT_SUCCESS;
		  }
		}
	  }//iterative sort
  }
  else
  { 
    overFlow = TRUE;  //mmap mode, overflow now.
  }
  if(overFlow == TRUE) 
  {
    //------------------------------------------------------------
    // No longer an internal sort. Since we have to generate runs.
    //------------------------------------------------------------
    internalSort_ = FALSE_L;
    if(! scratch_)
    {
       if(sortUtil_->scratchInitialize())
         return SORT_FAILURE;
       scratch_ = sortUtil_->getScratch();
    }

    if (sortUtil_->config()->logInfoEvent() && (!sortUtil_->config()->logInfoEventDone()))
    {
      SQLMXLoggingArea::logExecRtInfo(NULL,0,"QSort::SortSend, Sort has overflowed", explainNodeId_);
      sortUtil_->config()->setLogInfoEventDone();
    }
    return generateARun();
  }
  
  return SORT_FAILURE;
}


Lng32 Qsort::generateARun() 
{

#if defined(_DEBUG)
  char * envCifLoggingLocation= getenv("CIF_SORT_LOG_LOC");
  // This env var must be less than 238 charcaters long, 
  // else the file will not open.
#endif

  RESULT status = SCRATCH_SUCCESS;
  runSize_ = loopIndex_;
  if(runSize_ != 1)
  {
    if (sortUtil_->config()->sortType_.useQSForRunGeneration_)
       quickSort(recKeys_,0,(Int64)runSize_-1);
    else if (sortUtil_->config()->sortType_.useIterHeapForRunGeneration_)
       heapSort(recKeys_, (Int64)runSize_ );
    else
       iterativeQuickSort(recKeys_,0,(Int64)runSize_-1);
  }

  if (internalSort_)
  {
   // do not write run to scratch. 
#if defined(_DEBUG)
       if (envCifLoggingLocation)
       {
         char file2 [255];
         snprintf(file2, 255, "%s%s",envCifLoggingLocation,"/sort_logging.log");
         FILE * p2 = NULL;
         p2 =fopen(file2, "a");
         if (p2== NULL)
         {
           printf("Error in opening a file..");
         }
         time_t rawtime;
         struct tm * timeinfo;
         time ( &rawtime );
         timeinfo = localtime ( &rawtime );
         fprintf(p2,"%s\n","=======================================================");
         fprintf(p2,"%s\t%d\n","Explain Node Id:",explainNodeId_ );
         fprintf(p2,"%s\t%d\n","numberOfBytesForRecSize_:",sortUtil_->config()->numberOfBytesForRecordSize());
         fprintf(p2,"%s\t%d\n","keySize_:",keySize_ );
         fprintf(p2,"%s\t%d\n","Max Record Size:",recSize_ );

         fprintf(p2,"%s\t%s\tRun Size: %d\n",asctime (timeinfo), "generateARun No Overflow",runSize_);
         fclose(p2);
       }
#endif

  }
  else 
  { // write to run scratch
	if (scratch_->getDiskPool() == NULL)
	{
		// Need to generate the list of disks at this time
		scratch_->generateDiskTable(sortError_);
	}

       for (loopIndex_=0; loopIndex_ < runSize_; loopIndex_++) 
       {    
         ULng32 actRecLen = recKeys_[loopIndex_].rec_->getRecSize();

         status = 
	   recKeys_[loopIndex_].rec_->putToScr(currentRun_, actRecLen, 
					       scratch_);
         if (status ==IO_NOT_COMPLETE)
           return SORT_IO_IN_PROGRESS;
         if (status == SCRATCH_FAILURE)
	   return SORT_FAILURE;
       }

#if defined(_DEBUG)
       if (envCifLoggingLocation)
       {
         char file2 [255];
         snprintf(file2, 255, "%s%s",envCifLoggingLocation,"/sort_logging.log");
         FILE * p2 = NULL;
         p2 =fopen(file2, "a");
         if (p2== NULL)
         {
           printf("Error in opening a file..");
         }
         time_t rawtime;
         struct tm * timeinfo;
         time ( &rawtime );
         timeinfo = localtime ( &rawtime );
         if (currentRun_==1)
         {
           fprintf(p2,"%s\n","=======================================================");
           fprintf(p2,"%s\t%d\n","Explain Node Id:",explainNodeId_ );
           fprintf(p2,"%s\t%d\n","numberOfBytesForRecSize_:",sortUtil_->config()->numberOfBytesForRecordSize());
           fprintf(p2,"%s\t%d\n","keySize_:",keySize_ );
           fprintf(p2,"%s\t%d\n","Max Record Size:",recSize_ );
         }
         fprintf(p2,"%s\t%s\tCurrentRun: %d\tRun Size: %d\n",asctime (timeinfo), "generateARun",currentRun_, runSize_);
         fclose(p2);
       }
#endif
      loopIndex_ = 0;
      currentRun_++;
      status =  scratch_->flushRun(TRUE_L, TRUE_L)  ;
      if (status == SCRATCH_FAILURE)
        return SORT_FAILURE; 
      if (status == IO_NOT_COMPLETE)
        return SORT_IO_IN_PROGRESS;
  }
  
return SORT_SUCCESS;
}

Lng32 Qsort::sortClientOutOfMem()
{
 internalSort_ = FALSE_L;
 if(! scratch_)
  {
    if(sortUtil_->scratchInitialize())
    return SORT_FAILURE;
    scratch_ = sortUtil_->getScratch();
  }
 if (sortUtil_->config()->logInfoEvent() && (!sortUtil_->config()->logInfoEventDone()))
 {
   SQLMXLoggingArea::logExecRtInfo(NULL,0,"Sort buffer is out of memory, Sort has overflowed", explainNodeId_);
   sortUtil_->config()->setLogInfoEventDone();
 }
 return generateARun();
}

Lng32 Qsort::sortSendEnd()
{
  Lng32 retcode = SORT_SUCCESS;
  ex_assert(loopIndex_ >= 0, "Qsort::sortSendEnd: loopIndex_ is < 0");
  ex_assert(loopIndex_ < allocRunSize_, "Qsort::sortSendEnd: loopIndex_ > allocRunSize_");
  ex_assert(sendNotDone_, "Qsort::sortSendEnd: sendNotDone_ is false");

  sendNotDone_ = FALSE_L;
  retcode = generateARun();
  return retcode;
}


Lng32 Qsort::sortReceive(void *rec, ULng32& len)
{
  //---------------------------------------------------------------
  // We use Qsort to receive records only in case of internal sort
  // for merging.
  //---------------------------------------------------------------
  if (recNum_ < runSize_) {
    recKeys_[recNum_].rec_->getRecord(rec, recSize_);
    len = recSize_;
    recNum_++;
  }
  else {
    len = 0;
  }

  return SORT_SUCCESS;
}


Lng32 Qsort::sortReceive(void*& rec, ULng32& len, void*& tupp)
{
  //---------------------------------------------------------------
  // We use Qsort to receive records only in case of internal sort
  // for merging.
  //---------------------------------------------------------------
  if (recNum_ < runSize_) {
    recKeys_[recNum_].rec_->getRecordTupp(rec, recSize_, tupp);
    len = recSize_;
    recNum_++;
  }
  else {
    len = 0;
  }

  return SORT_SUCCESS;
}

//----------------------------------------------------------------------
// Name         : quickSort
// 
// Parameters   : ...
//
// Description  : This member function truly implements he quick sort
//                algorithm. Refer to any book such as Knuth for a 
//                description of this algorithm.
//
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------

NABoolean Qsort::quickSort(RecKeyBuffer keysToSort[], Int64 left, Int64 right)
{
  Int64 i, j;
  char* pivot;

  if (left+1 < right) 
     { // Atleast three elements
      
      pivot = median(keysToSort, left, right);
      i = left; j = right -1;
     
      for(;;)
       {
         while ((compare(keysToSort[++i].key_, pivot)<=KEY1_IS_SMALLER));    
         while ((compare(keysToSort[--j].key_, pivot)>=KEY1_IS_GREATER));   
         if (i < j)
            swap(&keysToSort[i], &keysToSort[j]);
         else
            break;
       }

     //-----------------------------------------------------
     // Put the pivot back to its place.
     //-----------------------------------------------------
     swap(&keysToSort[i], &keysToSort[right-1]);

     quickSort(keysToSort, left, i-1);
     quickSort(keysToSort, i+1, right);
     }
  else
     { // Only two elements
       if (left<right)
         {

          if (compare( keysToSort[left].key_,
                     keysToSort[right].key_ )>=KEY1_IS_GREATER) 
            swap(&keysToSort[left], &keysToSort[right]);

         }
     }
 return 0;

}

//----------------------------------------------------------------------
// Name         : iterativeQuickSort
//
// Parameters   : ...
//
// Description  : This member function implements the quick sort
//                This is the iterative flavor
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd.
//
// NOTE: NOT USED : For this implementation an extra buffer is required
// to simulate the stack. We are going to avoid this algo because of that
// requirement. If the performance of heapSort is not as good we may switch
// back to this. 
//----------------------------------------------------------------------
#define PUSH(l, r) {*stack++ = (l); *stack++ = (r);}
#define POP(l, r) {(r) = *--stack; (l) = *--stack;}
NABoolean Qsort::iterativeQuickSort(RecKeyBuffer keysToSort[], Int64 left, Int64 right)
{

  Int64 *stack;

  Int64 i, last, position;

  if(left >= right)
    return 0;

  // allocate a stack big enough to hold atleast the size of the array to be
  // sorted. Since each stack entry consists of 2 ints, we multiply this by 2

  //stack = (Int64 *)heap_->allocateAlignedHeapMemory(runSize_ * sizeof(Int64)*2, 64, FALSE);
  stack = (Int64 *) new (heap_) char[runSize_ * sizeof(Int64)*2] ;
  if (stack == NULL)
  {
  sortError_->setErrorInfo( EScrNoMemory   //sort error
                    ,0          //syserr: the actual FS error
                    ,0          //syserrdetail
                    ,"Qsort::iterativeQuicksort"     //methodname
                    );
    return SORT_FAILURE;
   }    
  /* Push -1's onto the stack as a signal to us in the 
     future that we've reached the bottom. */

  PUSH(-1, -1);
  PUSH(left, right);

  for(;;) {

    POP(left, right);

    if ((left == -1) && (right == -1))
      break;

    if(left < right) {
      position = (left + right) / 2;
      
      swap(&keysToSort[left],&keysToSort[position]);      
      last = left;
      
      for(i=left+1; i<=right; i++)
        if(compare(keysToSort[i].key_ , keysToSort[left].key_) <= KEY1_IS_SMALLER) {
          last++;
          swap(&keysToSort[last],&keysToSort[i]);
        }
      
      swap(&keysToSort[left],&keysToSort[last]);
      PUSH(left, last-1);
      PUSH(last+1, right);
    }
  }

 //heap_->deallocateMemory(stack);
  NADELETEBASIC(stack,heap_);
  return 0;
}

//----------------------------------------------------------------------
// Name         : heapSort
//
// Parameters   : ...
//
// Description  : This member function implements the heap sort
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd.
//
// NOTE: For this implementation no extra buffer is required. It is done all
// in place. This is the algorithm used if SORT_ITERATIVE_ALGO is set.
// requirement. If the performance of heapSort is not as good we may switch
// back to quickSort or iterativeQuickSort.

//----------------------------------------------------------------------
void Qsort::heapSort(RecKeyBuffer keysToSort[], Int64 runsize)
{
  Int64 i;

  for (i = (runsize/2 ); i >= 0; i--)
    siftDown(keysToSort, i, runsize-1);

  for (i = runsize-1; i >= 1; i--)
  {
    
    swap(&keysToSort[0],&keysToSort[i]);
    siftDown(keysToSort, 0, i-1);
  }
}

void Qsort::siftDown(RecKeyBuffer keysToSort[], Int64 root, Int64 bottom)
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
// Name         : median
// 
// Parameters   : ...
//
// Description  : This function finds the median of the keys of the 
//                first, center and right elements. It can be proved
//                mathematically that Quicksort can performance can 
//                become quadratic O(n2) if the pivot element is chosen
//                badly. By choosing the median as a pivot helps in 
//                providing a performance of the order of NlogN.              
//
// Return Value :
//   Key* - A pointer to the key which represents the median of the first
//          last and center element. 
//
//----------------------------------------------------------------------

char* Qsort::median(RecKeyBuffer keysToSort[], Int64 left, Int64 right)
{ 
 Int64 center =  (left+right)/2;
 
 if (compare(keysToSort[left].key_, keysToSort[center].key_) 
                                                         >= KEY1_IS_GREATER)
   {
     swap(&keysToSort[left], &keysToSort[center]);
   }

 if (compare(keysToSort[left].key_, keysToSort[right].key_)
                                                         >= KEY1_IS_GREATER)
   {
     swap(&keysToSort[left], &keysToSort[right]);
   }

 if ( compare(keysToSort[center].key_, keysToSort[right].key_)
                                                         >= KEY1_IS_GREATER )
                                                       
                                                        
   {
     swap(&keysToSort[center], &keysToSort[right]); 
   }
 
 swap(&keysToSort[center], &keysToSort[right-1]);

 return (keysToSort[right-1].key_);

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
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------

NABoolean Qsort::swap(RecKeyBuffer* recKeyOne, RecKeyBuffer* recKeyTwo)
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

Lng32 Qsort::generateInterRuns()
{
  cout << " You should not be using Quick sort for intermediate runs." << endl;
  return SORT_SUCCESS;
}

void Qsort::cleanUpMemoryQuota(void)
{
  sortUtil_->returnConsumedMemoryQuota((allocRunSize_ - initialRunSize_) *
                                (sizeof(RecKeyBuffer) + sizeof(Record)));
}

UInt32 Qsort::getOverheadPerRecord(void)
{
  return (sizeof(RecKeyBuffer) + sizeof(Record)); 
}
