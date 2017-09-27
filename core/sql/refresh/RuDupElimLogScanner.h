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
#ifndef _RU_DUP_ELIM_LOG_SCANNER_H_
#define _RU_DUP_ELIM_LOG_SCANNER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDupElimLogScanner.h
* Description:  Definition of class CRUDupElimLogScanner
*				
*
* Created:      07/02/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuDupElimTaskExUnit.h"
#include "RuDeltaDef.h"
#include "RuEmpCheckVector.h"
#include "RuDupElimLogRecord.h"
#include "RuDupElimConst.h"

//--------------------------------------------------------------------------//
//	CRUDupElimLogScanner
//	
//	The IUD-log reader. Executes the query and performs the reading
//	(through the instance of CDMResultSet). 
//
//	The scanner holds the window of two log records in the memory:
//	the current record and the previous one. The pointers are switched,
//	and the new data is read during the Fetch() method.
//
//	The scanner exposes both records in the window to the other
//	units of the algorithm by the GetCurrentRecord() and 
//	GetPrevRecord() .
//
//	The scanner also computes the log statistics that will be
//	exposed the the Duplicate Elimination task's object.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDupElimLogScanner : public CRUDupElimTaskExUnit {

private:
	typedef CRUDupElimTaskExUnit inherited;

public:
	CRUDupElimLogScanner(
		const CRUDupElimGlobals &globals, 
		CRUSQLDynamicStatementContainer &ctrlStmtContainer);

	virtual ~CRUDupElimLogScanner();

	//-----------------------------------------//
	//	Accessors
	//-----------------------------------------//
public:
	CRUIUDLogRecord *GetCurrentRecord() const
	{
		return pCurrentRec_;
	}

	CRUIUDLogRecord *GetPrevRecord() const
	{
		return pPrevRec_;
	}

public:
	BOOL IsEntireDeltaScanned() const 
	{
		return isEntireDeltaScanned_;
	}

	const CRUEmpCheckVector &GetEmpCheckVector() const
	{
		return empCheckVector_;
	}

	const CRUDeltaStatisticsMap &GetStatisticsMap() const
	{
		return statMap_;
	}

	//-----------------------------------------//
	//	Mutators
	//-----------------------------------------//
public:
	// Copy the emptiness check vector from the table
	void SetEmpCheckVector(const CRUEmpCheckVector &ecVec)
	{
		empCheckVector_ = ecVec;
	}

	// Implementation of a pure virtual
	virtual void Reset();

	// Start executing the query and allocate the buffers
	void StartScan(Int32 phase);
	// Close the currently open statement
	void EndScan();

	// Read the next record from the log and store it in the memory
	void Fetch();

public:
	// Pack/unpack for IPC - refinements & implementation of pure virtuals
        
	// Used in the main process side
	virtual void StoreRequest(CUOFsIpcMessageTranslator &translator)
	{
		inherited::StoreRequest(translator);

		empCheckVector_.StoreData(translator);
	}
	virtual void LoadReply(CUOFsIpcMessageTranslator &translator)
	{
		statMap_.LoadData(translator);
	}

	// Used in the remote process side
	virtual void LoadRequest(CUOFsIpcMessageTranslator &translator)
	{
		inherited::LoadRequest(translator);

		empCheckVector_.LoadData(translator);
	}
	virtual void StoreReply(CUOFsIpcMessageTranslator &translator)
	{
		statMap_.StoreData(translator);
	}

	//-----------------------------------------//
	//	Private area
	//-----------------------------------------//

private:
	// Perform the one-time setup actions at the first scan
	void SetupScan();

	void CopyLowerBoundParams();

	// Update the overall statistics by the current row
	void UpdateStatistics();
	void UpdateRangeStatistics(CRUDeltaStatistics &stat);
	void UpdateSingleRowStatistics(CRUDeltaStatistics &stat);

private:
	CDMPreparedStatement *pCurrentStmt_;
	// The result set to retrieve the data from
	CDMResultSet *pResultSet_;

	// Which index do the CK columns start from in the tuple?
	Int32 ckStartColumn_;

	//-- The input buffer.
	//-- Implemented as an array of pointers to structs
	//-- (rather than structs) because CRUIUDLogRecord
	//-- does not have a default constructor.
	enum { IB_SIZE = 2 }; // The buffer's size

	CRUIUDLogRecord * inputBuf_[IB_SIZE];
	Int32 inputBufIndex_;

	// ... and the pointers into it
	CRUIUDLogRecord *pCurrentRec_;
	CRUIUDLogRecord *pPrevRec_;

	// The least CK value to start the next phase from (initially, null)
	CRUIUDLogRecord *pLowerBoundRec_;

	// The input tuple's descriptor
	CDMSqlTupleDesc tupleDesc_;

	BOOL isEntireDeltaScanned_;

	// A hash table with statistical data about the log.
	// A distinct record in it corresponds to each non-
	// empty epoch in the log.
	CRUDeltaStatisticsMap statMap_;

	CRUEmpCheckVector empCheckVector_;

	// The number of distinct CK values retrieved so far
	TInt64 ckTag_;
};

#endif
