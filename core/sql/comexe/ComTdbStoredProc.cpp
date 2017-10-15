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
****************************************************************************
*
* File:         ComTdbStoredProc.cpp
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#include "ComTdbStoredProc.h"
#include "ComTdbCommon.h"

// ---------------------------------------------------------------------
// ComTdbStoredProc::ComTdbStoredProc()
// ---------------------------------------------------------------------
ComTdbStoredProc::ComTdbStoredProc(char * spName,
				   ex_expr * inputExpr,
				   ULng32 inputRowlen,
				   ex_expr * outputExpr,
				   ULng32 outputRowlen,
				   ex_cri_desc * workCriDesc,
				   const unsigned short workAtpIndex,
				   ex_cri_desc * criDescDown,
				   ex_cri_desc * criDescUp,	
				   ExSPInputOutput * extractInputExpr,
				   ExSPInputOutput * moveOutputExpr,
				   queue_index fromParent,
				   queue_index toParent,
				   Cardinality estimatedRowCount,
				   Lng32 numBuffers,
				   ULng32 bufferSize,
				   ex_expr *predExpr,
				   UInt16 arkcmpInfo)
  : ComTdb(ComTdb::ex_STORED_PROC,
	   eye_STORED_PROC,
	   estimatedRowCount,
           criDescDown,
	   criDescUp,
           fromParent,
	   toParent,
	   numBuffers,
	   bufferSize),
    spName_(spName),
    inputExpr_(inputExpr),
    inputRowlen_(inputRowlen),
    outputExpr_(outputExpr),
    outputRowlen_(outputRowlen),
    workCriDesc_(workCriDesc),
    workAtpIndex_(workAtpIndex),
    extractInputExpr_(extractInputExpr),
    moveOutputExpr_(moveOutputExpr),
    flags_(0),
    predExpr_(predExpr),
    arkcmpInfo_(arkcmpInfo)
{
}

ComTdbStoredProc::~ComTdbStoredProc()
{
}

Long ComTdbStoredProc::pack(void * space)
{
  spName_.pack(space);
  workCriDesc_.pack(space);
  inputExpr_.pack(space);
  outputExpr_.pack(space);
  predExpr_.pack(space);

  // The contents of extractInputExpr_ and moveOutputExpr_ are packed
  // separately outside of this function in RelInternalSP::codeGen() via a
  // call to generateSPIOExpr() (both in generator/GenStoredProc.cpp).
  //
  // Offsets of pointers in these expressions are based on a different base
  // address (the beginning of the expressions themselves) since they are
  // sent by the executor to the hosting process of the SP without being 
  // unpacked first. The hosting process will then unpack those expressions.
  //
  extractInputExpr_.packShallow(space);
  moveOutputExpr_.packShallow(space);

  return ComTdb::pack(space);
}

Lng32 ComTdbStoredProc::unpack(void * base, void * reallocator)
{
  if(spName_.unpack(base)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  if(inputExpr_.unpack(base, reallocator)) return -1;
  if(outputExpr_.unpack(base, reallocator)) return -1;
  if(predExpr_.unpack(base, reallocator)) return -1;

  // DO NOT unpack the contents of extractInput and moveOutput expressions
  // here. The executor will send them in their packed forms to the hosting
  // process of the SP, where they will then be unpacked.
  //
  if(extractInputExpr_.unpackShallow(base)) return -1;
  if(moveOutputExpr_.unpackShallow(base)) return -1;

  return ComTdb::unpack(base, reallocator);
}


/////////////////////////////////////////////////
ExSPInputOutput::ExSPInputOutput() : NAVersionedObject(-1)
{
  str_cpy_all(eyeCatcher_, "SPIO", 4);
  totalLen_ = 0;
  tupleDesc_ = (ExpTupleDescPtr)NULL;
  caseIndexArray_ = (Int16Ptr) NULL;
  flags_ = 0;
}

void ExSPInputOutput::initialize(ExpTupleDesc * tupleDesc,
				 ULng32 totalLen,
				 ConvInstruction * caseIndexArray)
{
  tupleDesc_ = tupleDesc;
  totalLen_ = totalLen;
  caseIndexArray_ = Int16Ptr((Int16 *)caseIndexArray);
}

Long ExSPInputOutput::pack(void * space)
{
  tupleDesc_.pack(space);
  caseIndexArray_.pack(space);
  return NAVersionedObject::pack(space);
}

Lng32 ExSPInputOutput::unpack(void * base, void * reallocator)
{
  if(tupleDesc_.unpack(base, reallocator)) return -1;
  if(caseIndexArray_.unpack(base)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}

