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

#include <iostream>

#include "SortAlgo.h"

//-----------------------------------------------------------------------
//  Constructor.
//-----------------------------------------------------------------------
SortAlgo::SortAlgo(ULng32 runsize, ULng32 recsize, 
                   NABoolean doNotAllocRec, ULng32 keysize, 
                   SortScratchSpace* scratch, Lng32 explainNodeId, ExBMOStats *bmoStats)
{
  sendNotDone_   = TRUE_L;
  runSize_       = runsize;
  recSize_       = recsize;
  doNotallocRec_ = doNotAllocRec;
  keySize_       = keysize;
  scratch_       = scratch;
  numCompares_   = 0L;
  internalSort_  = TRUE_L;
  explainNodeId_ = explainNodeId;
  bmoStats_      = bmoStats;
}


//-----------------------------------------------------------------------
// Name         : getNumOfCompares
// 
// Parameters   : 
//
// Description  : This function retrieves number of comparisons.

//   
// Return Value : unsigned long numCompares_
//                 
//-----------------------------------------------------------------------

ULng32 SortAlgo::getNumOfCompares() const
{
  return numCompares_;
} 

//-----------------------------------------------------------------------
// Name         : getScratch
// 
// Parameters   : 
//
// Description  : This function retrieves the scratch space pointer.

//   
// Return Value : ScratchSpace* scratch
//                 
//-----------------------------------------------------------------------

SortScratchSpace* SortAlgo::getScratch() const
{
  return scratch_;
} 

Lng32 SortAlgo::getRunSize() const
{
#pragma nowarn(1506)   // warning elimination 
  return runSize_;
#pragma warn(1506)  // warning elimination 
}

//-----------------------------------------------------------------------
// Name         : keyCompare
// 
// Parameters   : 
//
// Description  : This function is used to compare two keys and is 
//                independent of the sort algorithm itself. Note the use
//                of the overloaded operators for key comparision.
//   
// Return Value :
//  KEY1_IS_SMALLER
//  KEY1_IS_GREATER
//  KEYS_ARE_EQUAL
//-----------------------------------------------------------------------

short SortAlgo :: compare(char* key1, char* key2) 
{
  Int32 result;
  //numCompares_ ++;
  if (key1 && key2 ) {
    result = str_cmp(key1,key2,(Int32)keySize_);
    //return (memcmp(key1,key2,(int)keySize_));       
#pragma nowarn(1506)   // warning elimination 
    return result;
#pragma warn(1506)  // warning elimination 
  }
  else {
    if (key1 == NULL && key2 == NULL) return KEYS_ARE_EQUAL;
    if (key1 == NULL) return KEY1_IS_SMALLER;
    /*if (key2 == NULL)*/ return KEY1_IS_GREATER;
  };
#pragma nowarn(203)   // warning elimination 
  return 0; // NT_PORT ( ls 2/7/97 )
#pragma warn(203)  // warning elimination 
}










