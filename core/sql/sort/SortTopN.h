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
#ifndef SORTTOPN_H
#define SORTTOPN_H

/* -*-C++-*-
******************************************************************************
*
* File:         SortTopN.h
*
*
******************************************************************************
*/

#include "SortAlgo.h"
#include "Record.h"
#include "Const.h"
#include "NABasicObject.h"
#include "SortError.h"


class SortUtil;
class ExBMOStats;


class SortTopN : public SortAlgo { //SortAlgo inherits from NABasicObject

public:

  SortTopN(ULng32 recmax,ULng32 sortmaxmem, ULng32 recsize, NABoolean doNotallocRec, 
  ULng32 keysize, SortScratchSpace* scratch,NABoolean iterQuickSort,
  CollHeap* heap, SortError* sorterror, Lng32 explainNodeId, ExBMOStats *bmoStats, SortUtil* sortutil);
  ~SortTopN(void);

  Lng32 sortSend(void* rec, ULng32 len, void* tupp);
  
  Lng32 sortClientOutOfMem(void){ return 0;}  
  
  Lng32 sortSendEnd();

  Lng32 sortReceive(void* rec, ULng32& len);
  Lng32 sortReceive(void*& rec, ULng32& len, void*& tupp);
  UInt32 getOverheadPerRecord(void);
  Lng32 generateInterRuns(){ return 0;}
  
    
private:
  void buildHeap();
  void satisfyHeap();
  void insertRec(void *rec, ULng32 len, void* tupp);
  void sortHeap();
  void siftDown(RecKeyBuffer keysToSort[], Int64 root, Int64 bottom);
  NABoolean swap(RecKeyBuffer* recKeyOne, RecKeyBuffer* recKeyTwo);
   
  ULng32 loopIndex_;
  ULng32 recNum_;
  ULng32 allocRunSize_;
  NABoolean isHeapified_;
  RecKeyBuffer insertRecKey_;
  RecKeyBuffer* topNKeys_;
  SortError* sortError_;
  CollHeap* heap_;
  SortUtil* sortUtil_;
};

#endif


