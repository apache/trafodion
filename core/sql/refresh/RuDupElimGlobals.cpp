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
//
**********************************************************************/
/* -*-C++-*-
******************************************************************************
*
* File:         RuDupElimGlobals.cpp
* Description:  Implementation of class CRUDupElimGlobals
*
* Created:      06/12/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "RuDupElimGlobals.h"
#include "uofsIpcMessageTranslator.h"

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRUDupElimGlobals::CRUDupElimGlobals() :
	isRangeResolv_(FALSE),
	isSingleRowResolv_(FALSE),
	updateBmpSize_(0),
	nCtrlColumns_(0),
	nCKColumns_(0),
	lastDEEpoch_(0),
	beginEpoch_(0),
	endEpoch_(0),
	rangeLogType_(0),
	wasPrevDEInvocationCompleted_(FALSE),
	isSkipCrossTypeResoultion_(FALSE)
{}

//--------------------------------------------------------------------------//
//	CRUDupElimGlobals::Init()
//
//	Direct singletone initialization
//--------------------------------------------------------------------------//

void CRUDupElimGlobals::Init(
	BOOL isRangeResolv,
	BOOL isSingleRowResolv,
	Lng32 updateBmpSize,
	Lng32 nCtrlColumns,
	Lng32 nCKColumns,
	TInt32 lastDEEpoch,
	TInt32 beginEpoch,
	TInt32 endEpoch,
	Lng32 rangeLogType,
	BOOL wasPrevDEInvocationCompleted,
	BOOL isSkipCrossTypeResoultion)
{
	isRangeResolv_ = isRangeResolv;
	isSingleRowResolv_ = isSingleRowResolv;
	updateBmpSize_ = updateBmpSize;
	nCtrlColumns_ = nCtrlColumns;
	nCKColumns_ = nCKColumns;
	lastDEEpoch_ = lastDEEpoch;
	beginEpoch_ = beginEpoch;
	endEpoch_ = endEpoch;
	rangeLogType_ = rangeLogType;
	wasPrevDEInvocationCompleted_ = wasPrevDEInvocationCompleted;
	isSkipCrossTypeResoultion_ = isSkipCrossTypeResoultion;
}

//--------------------------------------------------------------------------//
//	CRUDupElimGlobals::LoadData()
//
//	Singletone initialization from the IPC buffer
//--------------------------------------------------------------------------//
void CRUDupElimGlobals::LoadData(CUOFsIpcMessageTranslator &translator)
{
	translator.ReadBlock(&isRangeResolv_, sizeof(BOOL));
	translator.ReadBlock(&isSingleRowResolv_, sizeof(BOOL));
	translator.ReadBlock(&updateBmpSize_, sizeof(Lng32));
	translator.ReadBlock(&nCtrlColumns_, sizeof(Lng32));
	translator.ReadBlock(&nCKColumns_, sizeof(Lng32));
	translator.ReadBlock(&lastDEEpoch_, sizeof(TInt32));
	translator.ReadBlock(&beginEpoch_, sizeof(TInt32));
	translator.ReadBlock(&endEpoch_, sizeof(TInt32));
	translator.ReadBlock(&rangeLogType_, sizeof(Lng32));
	translator.ReadBlock(&wasPrevDEInvocationCompleted_, sizeof(BOOL));
	translator.ReadBlock(&isSkipCrossTypeResoultion_, sizeof(BOOL));
}

//--------------------------------------------------------------------------//
//	CRUDupElimGlobals::StoreData()
//
//	Serialization to the IPC buffer
//--------------------------------------------------------------------------//
void CRUDupElimGlobals::StoreData(CUOFsIpcMessageTranslator &translator)
{
	translator.WriteBlock(&isRangeResolv_, sizeof(BOOL));
	translator.WriteBlock(&isSingleRowResolv_, sizeof(BOOL));
	translator.WriteBlock(&updateBmpSize_, sizeof(Lng32));
	translator.WriteBlock(&nCtrlColumns_, sizeof(Lng32));
	translator.WriteBlock(&nCKColumns_, sizeof(Lng32));
	translator.WriteBlock(&lastDEEpoch_, sizeof(TInt32));
	translator.WriteBlock(&beginEpoch_, sizeof(TInt32));
	translator.WriteBlock(&endEpoch_, sizeof(TInt32));
	translator.WriteBlock(&rangeLogType_, sizeof(Lng32));
	translator.WriteBlock(&wasPrevDEInvocationCompleted_, sizeof(BOOL));
	translator.WriteBlock(&isSkipCrossTypeResoultion_, sizeof(BOOL));
}
