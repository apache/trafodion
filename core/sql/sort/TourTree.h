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
#ifndef TREE_H
#define TREE_H

/* -*-C++-*-
******************************************************************************
*
* File:         TourTree.h
* RCS:          $Id: TourTree.h,v 1.2.16.2 1998/07/08 21:47:39  Exp $
*                               
* Description:  This class represents the tournament tree which is used by
*               Replacement Selection algorithm. The tournament tree is an
*               aggregate of TreeNode objects defined in TreeNode.h. This
*               class is derived from the abstract base class SortAlgo.
*
* Created:	    05/20/96
* Modified:     $ $Date: 1998/07/08 21:47:39 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
******************************************************************************
*/


#include "CommonStructs.h"
#include "Const.h"
#include "SortAlgo.h"
#include "TreeNode.h"
#include "ScratchSpace.h"
#include "NABasicObject.h"
#include "SortError.h"

class SortUtil;
class Tree : public SortAlgo { //SortAlgo inherits from NABasciObject

 public :
  
  Tree(ULng32 numruns,
       ULng32 runsize,
       ULng32 recsize, 
       NABoolean doNotallocRec,
       ULng32 keysize,
       SortScratchSpace* scratch,
       CollHeap* heap,
       SortError* sorterror,
       Lng32 explainNodeId,
       ExBMOStats *bmoStats,
       SortUtil* sortUtil,
       Lng32  runnum = 0,
       NABoolean  merge = FALSE_L,NABoolean waited = FALSE_L);
  
  ~Tree(void);
  
  Lng32 sortSend(void* rec, ULng32 len, void* tupp);
  
  Lng32 sortClientOutOfMem(void);

  Lng32 sortSendEnd();
  
  Lng32 sortReceive(void* rec, ULng32& len);
  Lng32 sortReceive(void*& rec, ULng32& len, void*& tupp);

  Lng32 generateInterRuns(void);

  UInt32 getOverheadPerRecord(void);
    
 private :

  void    determineNewWinner();
  RESULT outputWinnerToScr(void);
  
  TreeNode* rootNode_;
  Record* rootRecord_;
  TreeNode* winner_;
  char*    keyOfLastWinner_;
  short    height_;
  ULng32 numRuns_;
  ULng32 maxRuns_;
  ULng32 currentRun_;
  ULng32 winnerRun_;
  ULng32 baseRun_;
  SortError* sortError_;
  CollHeap* heap_;
  SortUtil* sortUtil_;
};

#endif








