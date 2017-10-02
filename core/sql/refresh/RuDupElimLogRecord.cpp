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
* File:         RuDupElimLogRecord.cpp
* Description:  Implementation of class	CRUIUDLogRecord
*
* Created:      06/12/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "dmprepstatement.h"
#include "ddobject.h"

#include "RuDupElimLogRecord.h"
#include "RuDeltaDef.h"
#include "RuException.h"

//--------------------------------------------------------------------------//
//		CLASS CRUIUDLogRecord - PUBLIC AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	Constructors and destructor
//--------------------------------------------------------------------------//

CRUIUDLogRecord::
CRUIUDLogRecord(CDMSqlTupleDesc &ckDesc, Int32 updateBmpSize) :
	// Persistent data members
	ckTuple_(ckDesc),
	syskey_(0),
	epoch_(0),
	opType_(0),
	ignore_(0),
	rangeSize_(0),
	pUpdateBitmap_(NULL),
	// Non-persistent data members
	ckTag_(0),
	action_(0)
{
	if (0 != updateBmpSize)
	{
		pUpdateBitmap_ = new CRUUpdateBitmap(updateBmpSize);
	}
}

CRUIUDLogRecord::CRUIUDLogRecord(const CRUIUDLogRecord &other) :
	// Persistent data members
	ckTuple_(other.ckTuple_),
	syskey_(other.syskey_),
	epoch_(other.epoch_),
	opType_(other.opType_),
	ignore_(other.ignore_),
	rangeSize_(other.rangeSize_),
	pUpdateBitmap_(NULL),
	// Non-persistent data members
	ckTag_(other.ckTag_)
{
	CRUUpdateBitmap *pOtherUpdateBitmap = other.pUpdateBitmap_;

	if (NULL != pOtherUpdateBitmap)
	{
		pUpdateBitmap_ = new CRUUpdateBitmap(*pOtherUpdateBitmap);
	}
}

CRUIUDLogRecord::~CRUIUDLogRecord()
{
	delete pUpdateBitmap_;
}

//--------------------------------------------------------------------------//
//	CRUIUDLogRecord::CopyCKTupleValuesToParams()
//
//	Copy the tuple's values to N consecutive parameters
//	of the statement: firstParam, ... firstParam + N - 1.
//
//--------------------------------------------------------------------------//

void CRUIUDLogRecord::
CopyCKTupleValuesToParams(CDMPreparedStatement &stmt, 
						  Int32 firstParam) const
{
	Lng32 len = GetCKLength();
	for (Int32 i=0; i<len; i++)
	{
		ckTuple_.GetItem(i).SetStatementParam(stmt, firstParam+i);
	}
}

//--------------------------------------------------------------------------//
//	CRUIUDLogRecord::Build()
//	
//	Retrieve the tuple's data from the result set and store it.
//	The tuple's columns are contiguous in the result set,
//	starting from the *startCKColumn* parameter.
//
//--------------------------------------------------------------------------//

void CRUIUDLogRecord::Build(CDMResultSet &rs, Int32 startCKColumn)
{
	ReadControlColumns(rs, startCKColumn);
	ReadCKColumns(rs, startCKColumn);
}

//--------------------------------------------------------------------------//
//		CLASS CRUIUDLogRecord - PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUIUDLogRecord::ReadControlColumns()
//	
//	Get the control columns from the result set (epoch, syskey etc).
//	The operation_type column is stored as a bitmap, and hence
//	requires decoding.
//
//--------------------------------------------------------------------------//

void CRUIUDLogRecord::ReadControlColumns(CDMResultSet &rs, Int32 startCKColumn)
{
	// Performance optimization - switch the IsNull check off!
	rs.PresetNotNullable(TRUE);

	// Read the mandatory columns
	epoch_  = rs.GetInt(CRUDupElimConst::OFS_EPOCH+1);

	opType_  = rs.GetInt(CRUDupElimConst::OFS_OPTYPE+1);
	if (FALSE == IsSingleRowOp())
	{
		// The range records are logged in the negative epochs.
		// Logically, however, they belong to the positive epochs.
		epoch_ = -epoch_;	
	}

	if (FALSE == IsSingleRowOp() && FALSE == IsBeginRange())
	{
		// End-range record
		rangeSize_ = rs.GetInt(CRUDupElimConst::OFS_RNGSIZE+1);
	}
	else
	{
		rangeSize_ = 0;
	}

	Int32 numCKCols = startCKColumn-2;	// Count from 1 + syskey
	if (CRUDupElimConst::NUM_IUD_LOG_CONTROL_COLS_EXTEND == numCKCols)
	{
		// This is DE level 3, read the optional columns
		ignore_ = rs.GetInt(CRUDupElimConst::OFS_IGNORE+1);

		// The update bitmap buffer must be setup
		RUASSERT(NULL != pUpdateBitmap_);
		// The update bitmap can be a null
		rs.PresetNotNullable(FALSE); 

		rs.GetString(CRUDupElimConst::OFS_UPD_BMP+1, 
					pUpdateBitmap_->GetBuffer(), 
					pUpdateBitmap_->GetSize());
	}

	// Syskey is always the last column before the CK
	syskey_ = rs.GetLargeInt(startCKColumn-1);
}	


//--------------------------------------------------------------------------//
//	CRUIUDLogRecord::ReadCKColumns()
//	
//	Retrieve the values of the clustering key columns and store
//	them in an SQL tuple.
//
//--------------------------------------------------------------------------//

void CRUIUDLogRecord::
ReadCKColumns(CDMResultSet &rs, Int32 startCKColumn)
{
	// Performance optimization - switch the IsNull check off!
	rs.PresetNotNullable(TRUE);

	Lng32 len = GetCKLength();

	for (Int32 i=0; i<len; i++)
	{
		Int32 colIndex = i + startCKColumn;

		ckTuple_.GetItem(i).Build(rs, colIndex);
	}

	rs.PresetNotNullable(FALSE);
}

// Define the class CRUIUDLogRecordList with this macro
DEFINE_PTRLIST(CRUIUDLogRecord);
