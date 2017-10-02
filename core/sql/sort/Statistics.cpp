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
#include "Platform.h"

#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "Statistics.h"

//----------------------------------------------------------------------
// SortStatistics Constructor.
//----------------------------------------------------------------------

SortStatistics::SortStatistics()
{
   memSizeB_        = 0L;
   numRecs_         = 0L;
   recLen_          = 0L;
   runSize_         = 0L;         //number of nodes in the tournament tree
   numRuns_         = 0L;

   numInitRuns_     = 0L;
   firstMergeOrder_ = 0L;
   finalMergeOrder_ = 0L;
   mergeOrder_      = 0L;
   numInterPasses_  = 0L;  

   numCompares_     = 0L; 
   numDupRecs_      = 0L;
   beginSortTime_ = 0;
   elapsedTime_ = 0;
   ioWaitTime_ = 0;
   scrBlockSize_    = 0L;
   scrNumBlocks_    = 0L;
   scrNumWrites_    = 0L;
   scrNumReads_     = 0L; 
   scrNumAwaitio_   = 0L;
}


//----------------------------------------------------------------------
// SortStatistics Desstructor.
//----------------------------------------------------------------------

SortStatistics::~SortStatistics() {}

//----------------------------------------------------------------------
// SortStatistics Retrieval Functions.
//----------------------------------------------------------------------

Lng32 SortStatistics::getStatRunSize() const
{
   return runSize_;
}
Lng32 SortStatistics::getStatNumRuns() const
{
   return numRuns_;
}



Lng32 SortStatistics::getStatMemSizeB() const
{
  return memSizeB_;
}

Int64 SortStatistics::getStatNumRecs() const
{
  return numRecs_;
}

Lng32 SortStatistics::getStatRecLen() const
{
  return recLen_;
}

Lng32 SortStatistics::getStatNumInitRuns() const
{
   return numInitRuns_;
}

Lng32 SortStatistics::getStatFirstMergeOrder() const 
{ 
  return firstMergeOrder_; 
}

Lng32 SortStatistics::getStatFinalMergeOrder() const
{
  return finalMergeOrder_;
}

Lng32 SortStatistics::getStatMergeOrder() const
{
  return mergeOrder_;
}

Lng32 SortStatistics::getStatNumInterPasses() const
{
  return numInterPasses_; 
}

Lng32 SortStatistics::getStatNumCompares() const
{
  return numCompares_;
}

Lng32 SortStatistics::getStatNumDupRecs() const
{
  return numDupRecs_;
}

Int64 SortStatistics::getStatBeginSortTime() const
{
 return beginSortTime_;
}

Int64 SortStatistics::getStatElapsedTime() const
{
 return elapsedTime_;
}


Int64 SortStatistics::getStatIoWaitTime() const
{
  return ioWaitTime_;
}


Lng32 SortStatistics::getStatScrBlockSize() const
{
  return scrBlockSize_;
}

Lng32 SortStatistics::getStatScrNumBlocks() const
{
  return scrNumBlocks_;
}

Lng32 SortStatistics::getStatScrNumWrites() const
{
  return scrNumWrites_;
}

Lng32 SortStatistics::getStatScrNumReads() const
{
  return scrNumReads_;
}

Lng32 SortStatistics::getStatScrAwaitIo() const
{
  return scrNumAwaitio_;
}











