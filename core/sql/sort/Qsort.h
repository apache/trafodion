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
#ifndef QSORT_H
#define QSORT_H

/* -*-C++-*-
******************************************************************************
*
* File:         Qsort.h
* RCS:          $Id: Qsort.h,v 1.2.16.2 1998/07/08 21:47:10  Exp $
*                               
* Description:  This class implements the QuickSort Algorithm. It is derived
*               from the SortAlgo base class in compliance with the Strategy
*               Policy design pattern from Gamma. Note that QuickSort is used 
*               only for Run generation and not for merging. Replacement
*               selection is the best for merging. 
*
* Created:	    05/20/96
* Modified:     $ $Date: 1998/07/08 21:47:10 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
******************************************************************************
*/

// -----------------------------------------------------------------------
// Change history:
// 
// $Log: Qsort.h,v $
// Revision 1.6  1998/07/08 15:25:36
// Changes in sort to allocate the initial Sort Memory dynamically. If the
// memory needed exceeds the SortMaxMem_ set by the executor, we spill over to
// a scratch file.
//
// Revision 1.5  1998/04/23 15:13:28
// Merge of NSKPORT branch (tag NSK_T4_980420) into FCS branch.
//
// Revision 1.2.16.1  1998/03/11 22:33:13
// merged FCS0310 and NSKPORT_971208
//
// Revision 1.4  1998/01/16 02:34:33
// merged LuShung's changes with the latest sort changes (from Ganesh).
//
// Revision 1.2  1997/04/23 00:29:01
// Merge of MDAM/Costing changes into SDK thread
//
// Revision 1.1.1.1.2.1  1997/04/11 23:23:04
// Checking in partially resolved conflicts from merge with MDAM/Costing
// thread. Final fixes, if needed, will follow later.
//
// Revision 1.4.4.1  1997/04/10 18:30:53
// *** empty log message ***
//
// Revision 1.1.1.1  1997/03/28 01:38:52
// These are the source files from SourceSafe.
//
// 
// 8     3/06/97 4:54p Sql.lushung
// A fix for the memory delete problem is make in this version of sort.
// Revision 1.4  1997/01/14 03:22:13
//  Error handling and Statistics are implemented in this version of arksort.
//
// Revision 1.3  1996/12/11 22:53:35
// Change is made in arksort to allocate memory from executor's space.
// Memory leaks existed in arksort code are also fixed.
//
// Revision 1.2  1996/11/13 02:20:05
// Record Length, Key Length, Number of records etc have been changed from
// short/int to unsigned long. This was requested by the Executor group.
//
// Revision 1.1  1996/08/15 14:47:36
// Initial revision
//
// Revision 1.1  1996/08/02 03:39:32
// Initial revision
//
// Revision 1.18  1996/05/20 16:32:34  <author_name>
// Added <description of the change>.
// -----------------------------------------------------------------------

#include "SortAlgo.h"
#include "TreeNode.h"
#include "Const.h"
#include "NABasicObject.h"
#include "SortError.h"


class SortUtil;
class ExBMOStats;
//----------------------------------------------------------------------
// This represents the structure used to store the key and record pointers
// to be used for quicksort.
//----------------------------------------------------------------------


void heapSort(RecKeyBuffer keysToSort[], Int32 runsize);
void siftDown(RecKeyBuffer keysToSort[], Int32 root, Int32 bottom);

class Qsort : public SortAlgo { //SortAlgo inherits from NABasicObject

public:

  Qsort(ULng32 recmax,ULng32 sortmaxmem, ULng32 recsize, NABoolean doNotallocRec, 
        ULng32 keysize, SortScratchSpace* scratch,NABoolean iterQuickSort,
        CollHeap* heap, SortError* sorterror, Lng32 explainNodeId, ExBMOStats *bmoStats, SortUtil* sortutil);
  ~Qsort(void);

  Lng32 sortSend(void* rec, ULng32 len, void* tupp);
  
  Lng32 sortClientOutOfMem(void);  
  
  Lng32 sortSendEnd();

  Lng32 sortReceive(void* rec, ULng32& len);
  Lng32 sortReceive(void*& rec, ULng32& len, void*& tupp);

  Lng32 generateInterRuns();

  UInt32 getOverheadPerRecord(void);
  
    
private:
  
  char* median(RecKeyBuffer keysToSort[], Int64 left, Int64 right);
  NABoolean quickSort(RecKeyBuffer keysToSort[], Int64 left, Int64 right); 
  NABoolean iterativeQuickSort(RecKeyBuffer keysToSort[], Int64 left, Int64 right);
  void heapSort(RecKeyBuffer keysToSort[], Int64 runsize);
  void siftDown(RecKeyBuffer keysToSort[], Int64 root, Int64 bottom);
  Lng32 generateARun();
  NABoolean swap(RecKeyBuffer* recKeyOne, RecKeyBuffer* recKeyTwo);
  void cleanUpMemoryQuota(void);
   
  ULng32 loopIndex_;
  ULng32 currentRun_;
  ULng32 recNum_;
  ULng32 allocRunSize_;
  ULng32 sortMaxMem_;
  NABoolean isIterativeSort_;
  Record* rootRecord_;
  RecKeyBuffer* recKeys_;
  SortError* sortError_;
  CollHeap* heap_;
  SortUtil* sortUtil_;
  ULng32 initialRunSize_;
};

#endif


