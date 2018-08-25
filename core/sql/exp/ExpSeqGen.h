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
#ifndef EXP_SEQ_GEN_EXPR_H
#define EXP_SEQ_GEN_EXPR_H


/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExpSeqGen.h
 * Description:  
 *               
 *               
 * Created:      7/20/2014
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "SequenceGeneratorAttributes.h"
#include "ComQueue.h"

  
class SeqGenEntry : public NABasicObject
{
 public:
  SeqGenEntry(Int64 sgUID, CollHeap * heap);

  short getNextSeqVal(SequenceGeneratorAttributes &sga, Int64 &seqVal);
  short getCurrSeqVal(SequenceGeneratorAttributes &sga, Int64 &seqVal);

  Int64 getSGObjectUID() { return sgUID_; }

  void setRetryNum(UInt32 n) { retryNum_ = n; }
  UInt32 getRetryNum() { return retryNum_ ; }

 private:
  short fetchNewRange(SequenceGeneratorAttributes &inSGA);

  CollHeap * heap_;

  Int64 sgUID_;

  NABoolean fetchNewRange_;
  Int64 cachedStartValue_;
  Int64 cachedEndValue_;
  Int64 cachedCurrValue_;

  void * cliInterfaceArr_;

  UInt32 retryNum_;
};

class SequenceValueGenerator : public NABasicObject
{
 public:
  SequenceValueGenerator(CollHeap * heap);
  SeqGenEntry * getEntry(SequenceGeneratorAttributes &sga);
  short getNextSeqVal(SequenceGeneratorAttributes &sga, Int64 &seqVal);
  short getCurrSeqVal(SequenceGeneratorAttributes &sga, Int64 &seqVal);

  HashQueue * sgQueue() { return sgQueue_;}
  CollHeap * getHeap() { return heap_; }
  void setRetryNum(UInt32 n) { retryNum_ = n; }
  UInt32 getRetryNum() { return retryNum_; }

 private:
  CollHeap * heap_;

  HashQueue * sgQueue_;

  UInt32 retryNum_;
};


#endif
