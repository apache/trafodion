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
#ifndef TREENODE_H
#define TREENODE_H
/* -*-C++-*-
******************************************************************************
*
* File:         TreeNode.h
* RCS:          $Id: TreeNode.h,v 1.2.16.2 1998/07/08 21:47:43  Exp $
*                               
* Description:  This class represents the TreeNode objects which make up the 
*               tournament used by Replacement Selection algorithm.
*               
* Created:	    05/20/96
* Modified:     $ $Date: 1998/07/08 21:47:43 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
*
*
******************************************************************************
*/

#include <iostream>
#include "CommonStructs.h"
#include "Record.h"
#include "NABasicObject.h"
#include "SortError.h"

class TreeNode : public NABasicObject {

 public :
   TreeNode();
   ~TreeNode();

   void initialize(ULng32 nodenum,
                   ULng32 associatedrun,
                   TreeNode *fi,
                   TreeNode *fe,
                   Record   *rec,
                   CollHeap* heap,  
                   SortError* sorterror,
                   SortScratchSpace* scratch,
                   NABoolean merge,
                   NABoolean waited);
   void deallocate();
   
   //NABoolean setRecord(void* rec, ULng32 reclen,  ULng32 keylen, Int16 numberOfBytesForRecordSize);
   NABoolean setRecordTupp(void *rec, ULng32 reclen,
                           ULng32 keylen, void* tupp,
                           Int16 numberOfBytesForRecordSize);
   NABoolean getRecord(void* rec, ULng32 reclen);  
 
   RESULT inputScr(ULng32 keylen, ULng32 reclen,
                  SortScratchSpace *scratch,
                  ULng32 &actRecLen,
                  //ULng32 keySize,
                  NABoolean waited = FALSE,
                  Int16 numberOfBytesForRecordSize = 0);
   RESULT outputScr(ULng32 run, ULng32 reclen,
                       SortScratchSpace *scratch, NABoolean waited = FALSE);

   TreeNode *getFe();
   TreeNode *getFi();
   
   void setLoser(TreeNode *node);
   TreeNode *getLoser();
   char* getKey();
   ULng32 getRun();
   void setRun(ULng32 run);

   Record * &record()
   {
     return record_;
   }

   SortMergeNode *sortMergeNode_; //adapter to read the run from scratch.
  
 private :

  char     *key_;
  Record   *record_;
  TreeNode *fi_;
  TreeNode *fe_; 
 
  TreeNode *loser_;
  ULng32 run_;
  ULng32 nodenum_;

  SortError *sortError_;
  CollHeap *heap_;
};

#endif











