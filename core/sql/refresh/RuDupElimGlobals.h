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
#ifndef _RU_DUPELIM_GLOBALS_H_
#define _RU_DUPELIM_GLOBALS_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDupElimGlobals.h
* Description:  Definition of class CRUDupElimGlobals
*
* Created:      06/12/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "refresh.h"
#include "Platform.h"
#include "dsplatform.h"

class CUOFsIpcMessageTranslator;

//--------------------------------------------------------------------------//
//	CRUDupElimGlobals
//
//	A collection of values required by the different units of the 
//	Duplicate Elimination task executor.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDupElimGlobals {

public:
	CRUDupElimGlobals();
	virtual ~CRUDupElimGlobals() {}

	void Init(
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
		BOOL isSkipCrossTypeResoultion
	);

	// Pack/Unpack methods for IPC
	void LoadData(CUOFsIpcMessageTranslator &translator);
	void StoreData(CUOFsIpcMessageTranslator &translator);

	// Which resolvers are created?
	BOOL IsRangeResolv() const
	{
		return isRangeResolv_;
	}
	BOOL IsSingleRowResolv() const
	{
		return isSingleRowResolv_;
	}

	// The number of bytes in the @UPDATE_BITMAP column
	Lng32 GetUpdateBmpSize() const 
	{ 
		return updateBmpSize_; 
	}
	// The number of control columns to be retrieved by the query
	Lng32 GetNumCtrlColumns() const
	{
		return nCtrlColumns_;
	}
	// The number of clustering key columns in the table	
	Lng32 GetNumCKColumns() const 
	{ 
		return nCKColumns_; 
	}
	// The last epoch scanned by DE until now
	TInt32 GetLastDEEpoch() const 
	{ 
		return lastDEEpoch_; 
	}
	TInt32 GetBeginEpoch() const
	{
		return beginEpoch_;
	}
	TInt32 GetEndEpoch() const
	{
		return endEpoch_;
	}
	Lng32 GetRangeLogType() const 
	{ 
		return rangeLogType_; 
	}
	BOOL WasPrevDEInvocationCompleted() const
	{
		return wasPrevDEInvocationCompleted_;
	}
	BOOL IsSkipCrossTypeResoultion() const
	{
		return isSkipCrossTypeResoultion_;
	}

private:
	BOOL isRangeResolv_;
	BOOL isSingleRowResolv_;
	Lng32 updateBmpSize_;
	Lng32 nCtrlColumns_;
	Lng32 nCKColumns_;
	TInt32 lastDEEpoch_;
	TInt32 beginEpoch_;
	TInt32 endEpoch_;
	Lng32 rangeLogType_;
	BOOL wasPrevDEInvocationCompleted_;
	BOOL isSkipCrossTypeResoultion_;
};

#endif
