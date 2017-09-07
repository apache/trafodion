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
#ifndef STATISTICS_H
#define STATISTICS_H

/* -*-C++-*-
******************************************************************************
*
* File:         Statistics.h
* RCS:          $Id: Statistics.h,v 1.3 1998/08/10 15:33:44  Exp $
*                               
* Description:  This file contains the definitions of various structures 
*               common to more than one class in ArkSort.
*    
* Created:      12/12/96
* Modified:     $ $Date: 1998/08/10 15:33:44 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
******************************************************************************
*/

#include "Platform.h"

// fix later UNIBR4
  #ifdef max
    #undef max
  #endif
  #ifdef min
    #undef min
  #endif

#include "Int64.h"

class SortStatistics {
  public :

   SortStatistics();
   ~SortStatistics();
   Lng32 getStatMemSizeB() const;
   Int64 getStatNumRecs() const;
   Lng32 getStatRecLen() const;
   Lng32 getStatRunSize() const;   
   Lng32 getStatNumRuns() const;
   Lng32 getStatNumInitRuns() const;
   Lng32 getStatFirstMergeOrder() const;
   Lng32 getStatFinalMergeOrder() const;
   Lng32 getStatMergeOrder() const;
   Lng32 getStatNumInterPasses() const;
   Lng32 getStatNumCompares() const;
   Lng32 getStatNumDupRecs() const;
   Int64 getStatBeginSortTime() const;
   Int64 getStatElapsedTime() const;
   Int64 getStatIoWaitTime() const;
   Lng32 getStatScrBlockSize() const;
   Lng32 getStatScrNumBlocks() const;
   Lng32 getStatScrNumWrites() const;
   Lng32 getStatScrNumReads() const;
   Lng32 getStatScrAwaitIo() const;

   friend class SortUtil;

  private :

   Lng32 memSizeB_;
   Int64 numRecs_;
   Lng32 recLen_;
   Lng32 runSize_;           // number of nodes in the tournament tree
   Lng32 numRuns_;

   Lng32 numInitRuns_;
   Lng32 firstMergeOrder_;
   Lng32 finalMergeOrder_;
   Lng32 mergeOrder_;
   Lng32 numInterPasses_;  
   Lng32 numCompares_; 
   Lng32 numDupRecs_;
   Int64 beginSortTime_;
   Int64 ioWaitTime_; //hr min sec millisec microsec in each respective word
   Int64 elapsedTime_;    // in seconds
   Lng32 scrBlockSize_;
   Lng32 scrNumBlocks_;
   Lng32 scrNumWrites_;
   Lng32 scrNumReads_; 
   Lng32 scrNumAwaitio_;
};

#endif
