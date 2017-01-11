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
#ifndef COMMONSTRUCTS_H
#define COMMONSTRUCTS_H

/* -*-C++-*-
******************************************************************************
*
* File:         CommonStructs.h
* RCS:          $Id: CommonStructs.h,v 1.6 1998/08/10 15:33:34  Exp $
*                               
* Description:  This file contains the definitions of various structures 
*               common to more than one class in ArkSort.
*                              
* Created:      05/20/96
* Modified:     $ $Date: 1998/08/10 15:33:34 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
*
******************************************************************************
*/

#include <Platform.h>


// fix later UNIBR4
  #ifdef max
    #undef max
  #endif
  #ifdef min
    #undef min
  #endif
  #ifdef ERROR
    #undef ERROR
  #endif

#include "NABasicObject.h"

//----------------------------------------------------------------------
// This is the structure which is stored at the start of each scratch
// file block used for storing runs.
//----------------------------------------------------------------------

struct ScrBlockHeader
{
 Lng32 thisBlockNum_;
 Lng32 nextBlockNum_;
 Lng32 bytesUsed_;
 Lng32 runNum_;
 Lng32 numRecs_;
};



//---------------------------------------------------------------------
// The fields withing SortType structure are  used to select options 
// such as :
//     -  I need to sort to determine the top 5 records.
//     -  I need to  determine the keys which would split a given range. 
//        of records into <n> sets containing almost equal number of 
//        records.
//     -  I would like to use QuickSort for generating runs.
//     -  I would like sort to remove records with duplicate keys.
//     -  I need the sort in a seperate process. As mentioned earlier
//        our attempt here is to implement UPS with external sorting as 
//        the general sorting method. But we are not restricting the 
//        design to prevent this implementation from being extended to
//        introduce a separate sort processes.
//---------------------------------------------------------------------

struct SortType {
 UInt32 useQSForRunGeneration_  : 1;
 UInt32 useIterQSForRunGeneration_  : 1;
 UInt32 useRSForRunGeneration_  : 1;
 UInt32 useIterHeapForRunGeneration_ : 1;
 UInt32 internalSort_           : 1;
 UInt32 doNotAllocRec_          : 1;
 UInt32 upsSort_                : 1;
 UInt32 topNOnly_               : 1;
 UInt32 splittingVectorNeeded_  : 1;
 UInt32 mergeOnly_              : 1;
 UInt32 removeDuplicates_       : 1;
 UInt32 filler_                 : 6;
};

enum ScratchOverflowMode
{
  SCRATCH_DISK =1,
  SCRATCH_SSD,
  SCRATCH_MMAP
};

#endif




