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
* File:         RuLogCleanupSQLComposer.cpp
* Description:  Implementation of class CRULogCleanupSQLComposer.
*				
*
* Created:      09/25/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuLogCleanupSQLComposer.h"
#include "RuLogCleanupTask.h"

#include "RuTbl.h"

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRULogCleanupSQLComposer::
CRULogCleanupSQLComposer(CRULogCleanupTask *pTask) :
	inherited(), 
	tbl_(pTask->GetTable()),
	maxInapplicableEpoch_(pTask->GetMaxInapplicableEpoch())
{}

//--------------------------------------------------------------------------//
//	CRULogCleanupSQLComposer::ComposeIUDLogCleanup()
//
//	DELETE FROM <T-IUD-log>
//	WHERE
//	"@EPOCH" BETWEEN 101 AND <max-inapplicable-epoch>
//	OR
//	"@EPOCH" BETWEEN <-max-inapplicable-epoch> AND 0
//
//	The range records reside in the negative epochs
//	Take care not to ruin the data in the special epochs (0-100)!
//
//--------------------------------------------------------------------------//

void CRULogCleanupSQLComposer::ComposeIUDLogCleanup(CRULogCleanupTaskExecutor::SQL_STATEMENT type)
{
	ComposeHeader(tbl_.GetIUDLogFullName(), type);

	CDSString epochCol("EPOCH");
	epochCol = ComposeQuotedColName(CRUTbl::logCrtlColPrefix, epochCol);

	// For single-row records
	sql_ += "\nWHERE\n(" + epochCol + " BETWEEN ";

	// MAX_SPECIAL_EPOCH+1 is the smallest non-special epoch
	sql_ += CRUSQLComposer::TInt32ToStr(MAX_SPECIAL_EPOCH+1) + " AND ";
	// The largest obsolete epoch
	sql_ += CRUSQLComposer::TInt32ToStr(maxInapplicableEpoch_) + ")\n";

	// For range records
	sql_ += "OR\n(" + epochCol + " BETWEEN ";
	sql_ += CRUSQLComposer::TInt32ToStr(-maxInapplicableEpoch_);
	sql_ += " AND 0);";
}

//--------------------------------------------------------------------------//
//	CRULogCleanupSQLComposer::ComposeRangeLogCleanup()
//
//	DELETE FROM <T-range-log>
//	WHERE
//	"@EPOCH" <= <max-inapplicable-epoch>
//
//	There is no data in the special epochs of the range log.
//
//--------------------------------------------------------------------------//

void CRULogCleanupSQLComposer::ComposeRangeLogCleanup()
{
	ComposeHeader(tbl_.GetRangeLogFullName(), CRULogCleanupTaskExecutor::CLEAN_IUD_FIRSTN);

	CDSString epochCol("EPOCH");
	epochCol = ComposeQuotedColName(CRUTbl::logCrtlColPrefix, epochCol);

	sql_ += "\nWHERE\n(" + epochCol + " <= ";
	// The largest obsolete epoch
	sql_ += CRUSQLComposer::TInt32ToStr(maxInapplicableEpoch_) + ");";
}

//--------------------------------------------------------------------------//
//  CRULogCleanupSQLComposer::ComposeHeader()
//
//  Compose the statement without the predicate
//  These are the 3 types:
//  1. Basic: "DELETE FROM <table> "
//  2. FirstN: "DELETE [FIRST <limit>] FROM <table> " 
//  3. Multi commit: "DELETE WITH MULTI COMMIT FROM <table> "
//--------------------------------------------------------------------------//

void CRULogCleanupSQLComposer::ComposeHeader(const CDSString &tableName, CRULogCleanupTaskExecutor::SQL_STATEMENT type)
{
  sql_ = "DELETE ";
  switch(type)
  {
    case CRULogCleanupTaskExecutor::CLEAN_IUD_BASIC:
      break;

    case CRULogCleanupTaskExecutor::CLEAN_IUD_FIRSTN:
      sql_ += "[FIRST " + TInt32ToStr(MAX_ROW_TO_DELETE_IN_SINGLE_TXN) + "]";
      break;

    case CRULogCleanupTaskExecutor::CLEAN_IUD_MCOMMIT:
      sql_ += "WITH MULTI COMMIT ";
  }
  sql_ += "\nFROM " + tableName;
}

//--------------------------------------------------------------------------//
//  CRULogCleanupSQLComposer::composeGetRowCount
//
//  On NSK:
//  SELECT ROW COUNT FROM <table>
//
// On NT:
// SELECT COUNT(*) FROM <table>
//--------------------------------------------------------------------------//

void CRULogCleanupSQLComposer::composeGetRowCount()
{
  sql_ = "SELECT ";

#if defined(NA_WINNT)
  sql_ += "COUNT(*) ";
#else
  sql_ += "ROW COUNT ";
#endif

  sql_ += "FROM " + tbl_.GetIUDLogFullName();
}
