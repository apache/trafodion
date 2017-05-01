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
* File:         TourTree.C
* RCS:          $Id: tourtree.cpp,v 1.1 2006/11/01 01:44:37  Exp $
*                               
* Description:  This file contains the member functions definitions of the
*               Tree class.
*
* Created:	    05/20/96
* Modified:     $ $Date: 2006/11/01 01:44:37 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
*
******************************************************************************
*/

#include <iostream>
#include <fstream>
#include <string.h>
#include "ex_stdh.h"
#include "TourTree.h"
#include "SortUtil.h"
#include "ex_ex.h"
#include "ExStats.h"

//------------------------------------------------------------------------
// This file contains the all member function definitions of Tree class. 
//------------------------------------------------------------------------

//----------------------------------------------------------------------
// Name         : Tree 
// 
// Parameters   : ...
//
// Description  : This is the contructor of the Tree class. It builds 
//                the tournament tree and initializes it for use during
//                the actual sorting phase.
//   
// Return Value :
//          None.
//
//----------------------------------------------------------------------

Tree::Tree(ULng32 numruns, ULng32 runsize, ULng32 recsize, 
           NABoolean doNotAllocRec, ULng32 keysize,
           SortScratchSpace* scratch, CollHeap *heap, SortError *sorterror, 
           Lng32 explainNodeId, ExBMOStats *bmoStats, SortUtil* sortUtil, Lng32 runnum, NABoolean merge,NABoolean waited) :
           SortAlgo(runsize, recsize, doNotAllocRec, keysize, scratch, explainNodeId, bmoStats),
           maxRuns_(0), currentRun_(0), winnerRun_(0), sortError_(sorterror),
           heap_(heap), sortUtil_(sortUtil) 
{
   ULng32 nodenum;
   numRuns_ = numruns;

    if(! scratch_)
    {
      sortUtil_->scratchInitialize();
      scratch_ = sortUtil_->getScratch();
      assert(scratch_ != NULL);
    }

   //Allocation failure will cause longjmp to jmp handler in ex_sort.
   rootNode_ = (TreeNode*)heap_->allocateMemory(numRuns_ * sizeof(TreeNode));
   rootRecord_ = (Record*)heap_->allocateMemory(numRuns_ * sizeof(Record));
   keyOfLastWinner_ = (char*)heap_->allocateMemory(sizeof(char) * keysize);
   
   ex_assert(keyOfLastWinner_ != NULL, "Tree::Tree, keyOfLastWinner_ is NULL");
   ex_assert(rootNode_ != NULL, "Tree::Tree, root_Node_ is NULL");
   ex_assert(rootRecord_ != NULL, "Tree::Tree, rootRecord_ is NULL");
   
#pragma nowarn(1506)   // warning elimination 
   str_pad(keyOfLastWinner_, keysize, '\377');
#pragma warn(1506)  // warning elimination 
   baseRun_    = runnum;
   winner_     = rootNode_;

   for (nodenum=0;nodenum<numRuns_;nodenum++) {
       rootNode_[nodenum].initialize(nodenum,
                                     runnum++, 
                                     &rootNode_[nodenum/2],              //fi_
				     &rootNode_[(nodenum+numRuns_)/2],   //fe_
                                     rootRecord_,
				     heap_,
                                     sorterror,
                                     scratch_,
                                     merge,
                                     waited);
       rootRecord_[nodenum].initialize(recSize_, doNotallocRec_, heap_, sorterror); 
   };
}

Tree::~Tree(void) {
//----------------------------------------------------------------------
// delete the root record
//----------------------------------------------------------------------
  ULng32 nodenum;
  if (rootNode_ != NULL) {
   for (nodenum = 0; nodenum < numRuns_; nodenum++)
          rootNode_[nodenum].deallocate();
   heap_->deallocateMemory((void*)rootNode_);
   rootNode_ = NULL;
  }
  if (rootRecord_ != NULL) {
   heap_->deallocateMemory((void*)rootRecord_);
   rootRecord_ = NULL;
  }
  if(keyOfLastWinner_ != NULL){
     heap_->deallocateMemory((void*)keyOfLastWinner_);
     keyOfLastWinner_ = NULL;
    }
}

//----------------------------------------------------------------------
// Name         : sort
// 
// Parameters   : ...
//
// Description  : This function implements the Replacement Selection
//                algorithm. For more details about the algorithm refer
//                to the book by Knuth. Note that this function either
//                performs a sort or merge depending on whether the 
//                Phase is RUN_GENERATION or FINAL_MERGE.
//   
// Return Value :
//   SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
//  The following code appears three times in these three functions
//  sortSend, sortSendEnd, sortReceive.  Make sure that when one
//  if modified the changes propagate.
//
//  if (winnerRun_!= currentRun_) {
//    if ((currentRun_ != 0) && sendNotDone) {
//      scratch_->flushData();
//    }
//    if (winnerRun_ >  maxRuns_) {
//      return;
//    }
//    currentRun_ = winnerRun_;
//  }
//----------------------------------------------------------------------
// remember to output winner after re-organizing the tree
void Tree::determineNewWinner()
{
  TreeNode *temp1, *temp2;
  NABoolean foundNewWinner = FALSE_L;
  ULng32 temp1Run = 0L;
  temp1 = winner_->getFe();
  short r = 0;

  do {
    r = compare(temp1->getLoser()->getKey(), winner_->getKey() );
     if ((temp1->getRun() < winnerRun_) ||
	 ((temp1->getRun() == winnerRun_) &&
          // (compare(temp1->getLoser()->getKeyPtr(), winner_->getKeyPtr() )
          // <  KEY1_IS_GREATER))) {
          (r < KEY1_IS_GREATER))) {
       //------------------------------------------------------ 
       // new winner found, Swap loser and run numbers
       //------------------------------------------------------
       temp2 = temp1->getLoser();
       temp1->setLoser(winner_);
       winner_ = temp2;
       
       temp1Run = temp1->getRun();
       temp1->setRun(winnerRun_);
       winnerRun_ = temp1Run;
      } 
    
     if (temp1 == &rootNode_[1]) {   
       foundNewWinner = TRUE_L;
     }
     else { 
       temp1 = temp1->getFi();
     } 

   } while (!foundNewWinner);
}

RESULT Tree::outputWinnerToScr(void) 
{
  RESULT status;
  memcpy(keyOfLastWinner_, winner_->getKey(), keySize_); 
  //outputScr can get err
  status = winner_->outputScr(baseRun_+currentRun_, recSize_, scratch_);
   return status; 
}



Lng32 Tree::sortReceive(void *rec, ULng32& len)
{

  Lng32 retcode = SORT_SUCCESS; 

  //-----------------------------------------------------------------
  // Case of zero input records.
  //-----------------------------------------------------------------

  if (numRuns_ == 0) {
    len = 0;
    return SORT_SUCCESS;
  }
  
  len = recSize_;

  if (numRuns_ == 1) 
  { 
     //--------------------------------------------------------------
     // There is no need to perform a merge. Keep reading from input
     // and return records to caller one in each call to sortReceive. 
     //-------------------------------------------------------------- 
     while (!(retcode = winner_->inputScr(keySize_, recSize_,scratch_,len,FALSE,
                                          sortUtil_->config()->numberOfBytesForRecordSize())))
     {
        winner_->getRecord(rec, winner_->record()->getRecSize()  /*len*/ /* recSize_*/);
        ///len = recSize_;
          return SORT_SUCCESS;
     }   

     if (retcode == IO_NOT_COMPLETE)
        return SORT_IO_IN_PROGRESS;
     //-------------------------------------------------------------
     // We are done with all the records so return a len of 0.
     //------------------------------------------------------------
     len = 0;
     return SORT_SUCCESS;
  }
  
  while (TRUE_L) {

    if (winnerRun_!= currentRun_) {
     
        if (winnerRun_ >  maxRuns_) { // We are all done so return a len of 0.
           len = 0;
           return SORT_SUCCESS;
        }
        else {
           currentRun_ = winnerRun_;
        } 
    }

    if (winnerRun_) 
   {
      //---------------------------------------------------------------
      // Set the rec and len parameters of sortReceive with the 
      // winner record and length. After this we would input the next
      // record from the scratch run and determine the new winner before 
      // returning control back to the caller.
      //---------------------------------------------------------------
       memcpy(keyOfLastWinner_, winner_->getKey(), keySize_); 
       winner_->getRecord(rec, winner_->record()->getRecSize());
       len = winner_->record()->getRecSize();      // should be actual length instead??? from record?? 
    }

    ULng32 len2 = recSize_;
    retcode = winner_->inputScr(keySize_, recSize_, scratch_,len2,FALSE,
                                sortUtil_->config()->numberOfBytesForRecordSize());

    if (retcode == END_OF_RUN)
       winnerRun_ = maxRuns_ + 1;
    else 
     if (retcode)
     {
        if (retcode == IO_NOT_COMPLETE)
        {
         return SORT_IO_IN_PROGRESS;
        }
	return retcode; // FAILUREkeySize_
     }
    
    if (retcode == SORT_SUCCESS) 
    {
       if ((winnerRun_ == 0) ||
           (compare(keyOfLastWinner_,winner_->getKey())>=KEY1_IS_GREATER))
       {
	       winnerRun_++;
	       if (winnerRun_ > maxRuns_ )
	           maxRuns_ =  winnerRun_;
       }
    }
    
    determineNewWinner();

    if (currentRun_ != 0) {
       return SORT_SUCCESS;
    }
       
  }
} 


Lng32 Tree::generateInterRuns()
{
  //ex_assert(0, "CIF not implemented yet")
  Lng32 retcode = SORT_SUCCESS;
  Lng32  numRuns = scratch_->getTotalNumOfRuns(); 
 
  while (TRUE_L) 
  {

    if (winnerRun_!= currentRun_)
    {
#ifdef FORDEBUG
        cout << "End of Inter. Run : " <<  currentRun_+numRuns << endl << endl;
#endif
#if defined(_DEBUG)
       char * envCifLoggingLocation= getenv("CIF_SORT_LOG_LOC");
       FILE *p2 = NULL;

       if (envCifLoggingLocation )
       {
         char file2 [255];
         sprintf(file2,"%s%s",envCifLoggingLocation,"/sort_logging.log");
         p2 =fopen(file2, "a");
         if (p2== NULL)
         {
           printf("Error in opening a file..");
         }
         time_t rawtime;
         struct tm * timeinfo;
         time ( &rawtime );
         timeinfo = localtime ( &rawtime );
         fprintf(p2,"%s\t%s\t%d\n",asctime (timeinfo), "generateInterRun",currentRun_+numRuns);
         fclose(p2);
       }
#endif
       if (currentRun_ != 0) 
       {
          retcode =  scratch_->flushRun(TRUE_L,TRUE_L);
          if (retcode)
            {
              return retcode;
            }
       } 
       if (winnerRun_ >  maxRuns_) 
       { // We are all done so return SORT_SUCCESS
           return SORT_SUCCESS;
       }
       else 
       {
           currentRun_ = winnerRun_;
       } 
    }

    if (winnerRun_) 
    {
       memcpy(keyOfLastWinner_, winner_->getKey(), keySize_);
       ULng32 rSize = recSize_;
       if (sortUtil_->config()->resizeCifRecord())
       {
         rSize = winner_->record()->getRecSize();
       }
       if (bmoStats_)
           bmoStats_->incInterimRowCount();
       retcode = winner_->outputScr(currentRun_+numRuns, rSize, scratch_,TRUE_L);
       if (retcode != 0)
       {
            return retcode;
       }
    }

    ULng32 actRecLen = recSize_;
    retcode = winner_->inputScr(keySize_, recSize_, scratch_,actRecLen,TRUE_L,
                                sortUtil_->config()->numberOfBytesForRecordSize());
   

    if (retcode == END_OF_RUN)
    // readData returns EOF for end of runkeySize_
      winnerRun_ = maxRuns_ + 1;
   else
    if (retcode)
    {
      return retcode; // FAILURE or IO_IN_PROGkeySize_RESS
    }

   if (retcode == SORT_SUCCESS)
   {
   if (compare(keyOfLastWinner_,winner_->getKey())>=KEY1_IS_GREATER)
     {
           winnerRun_++;
           if (winnerRun_ > maxRuns_ )
               maxRuns_ =  winnerRun_;
     }
    } 
    determineNewWinner();

} 
}

//  **********************UNUSED METHODS ********************************
// LCOV_EXCL_START

Lng32 Tree::sortClientOutOfMem(void)
{
  return sortSendEnd();
}

Lng32 Tree::sortSendEnd()
{

  while(TRUE_L) 
  {

     if ((winnerRun_ != currentRun_)) 
     {
        if ((currentRun_ != 0) && sendNotDone_)
        {
          if ( scratch_->flushRun(TRUE_L) )  
            return SORT_FAILURE; 
        }
        if (winnerRun_ >  maxRuns_) 
           return SORT_SUCCESS;
        currentRun_ = winnerRun_;
     }

     if (winnerRun_) 
     {
       if ( outputWinnerToScr() )
         return SORT_FAILURE; 
     }
     winnerRun_ = maxRuns_+1;
     determineNewWinner();
  }
}



Lng32 Tree::sortSend(void *rec, ULng32 len, void* tupp)
{
  ex_assert(rec != NULL, "Tree::sortSend, rec is NULL");

  if (winnerRun_!= currentRun_) {
#ifdef FORDEBUG
      cout << "End of Run : " << currentRun_ << endl << endl;
#endif
      //--------------------------------------------------------------
      // Flush the block since we are starting a new run
      // Also ensure that you do not flush for the dummy run 0.
      //--------------------------------------------------------------     
      if (currentRun_ != 0) {
          scratch_->flushRun(TRUE_L);
      }
      currentRun_ = winnerRun_;   
  }

  if (winnerRun_) {
     outputWinnerToScr();
  }
    
  //------------------------------------------------------------------
  // Input the new record and determine the nekeySize_w winner.
  //------------------------------------------------------------------
 
  winner_->setRecordTupp(rec, recSize_, keySize_, tupp,
                         sortUtil_->config()->numberOfBytesForRecordSize());
  if ((winnerRun_ == 0) 
           ||
      (compare(keyOfLastWinner_, winner_->getKey()) >= KEY1_IS_GREATER)) {
       winnerRun_++;
       if (winnerRun_ > maxRuns_ )
          maxRuns_ =  winnerRun_;
  }

  determineNewWinner();
  return SORT_SUCCESS;
}

Lng32 Tree::sortReceive(void*& rec, ULng32& len, void*& tupp)
{
  // Dummy function. Currently only QuickSort is used for internal sort.
  return SORT_SUCCESS;
}

UInt32 Tree::getOverheadPerRecord(void)
{
  //We will assume at minimum the merge order is 2.
  return(2 * (sizeof(TreeNode) + sizeof(Record)));
}

// LCOV_EXCL_STOP






