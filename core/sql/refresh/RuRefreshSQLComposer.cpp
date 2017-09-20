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
* File:         RuSQLRefreshComposer.cpp
* Description:  Implementation of class RuSQLRefreshComposer
*				
*
* Created:      08/13/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuRefreshSQLComposer.h"
#include "RuGlobals.h"
#include "RuOptions.h"
#include "RuSQLDynamicStatementContainer.h"

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRURefreshSQLComposer::CRURefreshSQLComposer(CRURefreshTask *pTask) : 
	inherited(),
	pTask_(pTask),
	pRootMV_(&(pTask->GetRootMV()))
{}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::StartInternalRefresh()
// 
//  Start a new INTERNAL REFRESH statement composition
//--------------------------------------------------------------------------//

void CRURefreshSQLComposer::StartInternalRefresh()
{
	if (NULL == CRUGlobals::GetInstance()->GetOptions().
							FindDebugOption(CRUGlobals::DISPLAY_REFRESH,"") ) 
	{
		sql_ = "INTERNAL REFRESH ";
	}
	else
	{
		sql_ = "DISPLAY INTERNAL REFRESH ";
	}
	
	sql_  += GetRootMV().GetFullName();
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::AddDeltaDefClause()
//
//	Generate the DeltaDef clause 
//
//--------------------------------------------------------------------------//

void CRURefreshSQLComposer::AddDeltaDefClause(const CRUDeltaDef *pDdef,
											  CDSString &fromEpoch,
											  CDSString &toEpoch)
{
	// <table-name> BETWEEN <begin-epoch> AND <end-epoch>
	sql_ += "\n\t" + pDdef->tblName_;
	sql_ += " BETWEEN " + fromEpoch + " AND " + toEpoch;
	sql_ += " DE LEVEL " + TInt32ToStr(pDdef->deLevel_);
		
	// The delta statistics clauses (based on the emptiness check
	// and the duplicate elimimation results)
	AddRngDeltaStatisticsClause(pDdef);	
	AddIUDDeltaStatisticsClause(pDdef);
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::AddRngDeltaStatisticsClause()
//
//	AddDeltaDefClause() callee
//
//--------------------------------------------------------------------------//

void CRURefreshSQLComposer::
AddRngDeltaStatisticsClause(const CRUDeltaDef *pDdef)
{
	sql_ += "\n\t";

	if (FALSE == pDdef->isRangeLogNonEmpty_)
	{
		sql_ += " USE NO RANGELOG";
		return;
	}

	CRUDeltaStatistics *pStat = pDdef->pStat_;
	RUASSERT(CRUDeltaDef::NO_DE != pDdef->deLevel_ && NULL != pStat);

	// USE RANGELOG <number> NUM_OF_RANGES <number> ROWS_COVERED
	TInt32 nRanges = pStat->nRanges_;
	RUASSERT(nRanges > 0);

	sql_ += "USE RANGELOG " + TInt32ToStr(nRanges) + " NUM_OF_RANGES ";

	TInt32 nCoveredRows = pStat->nRangeCoveredRows_;
	if (CRUDeltaStatistics::RANGE_SIZE_UNKNOWN != nCoveredRows)
	{
		sql_ += TInt32ToStr(nCoveredRows) + " ROWS_COVERED";
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::AddIUDDeltaStatisticsClause()
//
//	AddDeltaDefClause() callee
//
//--------------------------------------------------------------------------//

void CRURefreshSQLComposer::
AddIUDDeltaStatisticsClause(const CRUDeltaDef *pDdef)
{
	sql_ += "\n\t";

	if (FALSE == pDdef->isIUDLogNonEmpty_)
	{
		sql_ += " USE NO IUDLOG";
		return;
	}

	sql_ += "USE IUDLOG ";

	if (CRUDeltaDef::SINGLE_2_SINGLE != pDdef->deLevel_)
	{
		// We can only tell whether the delta is insert-only or not
		if (TRUE == pDdef->isIUDLogInsertOnly_)
		{
			// USE IUDLOG INSERT ONLY
			sql_ += "INSERT ONLY";
		}

		return;
	}

	// Single-row resolution was enforced. We have complete statistics.
	CRUDeltaStatistics *pStat = pDdef->pStat_;
	if (NULL == pStat)
	{
		// DE came up with empty stats.
		// Just write zeroes, and INTERNAL REFRESH will handle it.
		pStat = new CRUDeltaStatistics();
	}

	// USE IUDLOG
	//	<number> ROWS_INSERTED
	//	<number> ROWS_DELETED
	//	<number> ROWS_UPDATED [COLUMNS <col>,<col>,<col>...]

	sql_ += "\n\t" + TInt32ToStr(pStat->nInsertedRows_) + " ROWS_INSERTED ";
	sql_ += "\n\t" + TInt32ToStr(pStat->nDeletedRows_) + " ROWS_DELETED ";
	sql_ += "\n\t" + TInt32ToStr(pStat->nUpdatedRows_) + " ROWS_UPDATED ";

	if (0 != pStat->nUpdatedRows_)
	{
		CRUUpdateBitmap *pUpdateBitmap = pStat->pUpdateBitmap_;
		RUASSERT(NULL != pUpdateBitmap);

		AddUpdatedColumnsClause(pUpdateBitmap);
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::AddUpdatedColumnsClause()
//
//	COLUMNS <col>,<col>,<col>...
//--------------------------------------------------------------------------//

void CRURefreshSQLComposer::
AddUpdatedColumnsClause(const CRUUpdateBitmap *pUpdateBitmap)
{
	sql_ += "COLUMNS (";

	BOOL isFirst = TRUE;
	Int32 numOfColumns = pUpdateBitmap->GetSize() * 8;
	
	for	(Int32 index=0;index < numOfColumns;index++)
	{
		if (FALSE == pUpdateBitmap->IsColumnUpdated(index))
		{
			continue;
		}
		
		if (TRUE == isFirst)
		{
			isFirst = FALSE;
		}
		else
		{
			sql_ += ",";
		}
		
		sql_ += TInt32ToStr(index);
	}

	sql_ += ") ";
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::AddPhaseParam()
// 
//  Add keyword PHASE with a placeholder for the phase number comple-time
//	parameter. This parameter will be replaced through the SQL Container 
//  before compiling the statement.
//
//--------------------------------------------------------------------------//

void CRURefreshSQLComposer::AddPhaseParam()
{
	sql_+= " \n\tPHASE "; 
	sql_+= CDSString(CRUSQLDynamicStatementContainer::COMPILED_PARAM_TOKEN);
	sql_+= " ";
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::AddPipeLineClause()
//
//	Generate the PIPELINE clause for each MV that the delta is pipelined to.
//
//--------------------------------------------------------------------------//

void CRURefreshSQLComposer::AddPipeLineClause()
{
	CRUMVList &mvList = GetRefreshTask().GetMVList();
	RUASSERT(mvList.GetCount() > 1);

	DSListPosition pos = mvList.GetHeadPosition();
	mvList.GetNext(pos);	// Skip the root MV
	
	// Generate root clause
	CRUMV *pTopMV = mvList.GetNext(pos);

	sql_ += " PIPELINE " ;
	sql_ += "(" + pTopMV->GetFullName() + ")";

	// Continue with the upper mv's
	while (NULL != pos)
	{
		sql_ += pTopMV->GetFullName();
		
		// Retrieve the next MV from the list
		pTopMV = mvList.GetNext(pos);

		sql_ += " PIPELINE " ;
		sql_ += "(" + pTopMV->GetFullName() + ")";

		if (NULL != pos)
		{
			sql_ += ",";
		}
		else
		{
			return;
		}
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::ComposeCntrlTableMDAMText()
//
// If the name pointer is null then the table_name will be star.
//
//	CONTROL TABLE table_name MDAM mdam_option
//	
//
//--------------------------------------------------------------------------//
void CRURefreshSQLComposer::ComposeCntrlTableMDAMText(
								CRUForceOptions::MdamOptions mdamOption,
								const CDSString* pName)
{
	sql_ = "CONTROL TABLE ";

	if (NULL == pName)
	{
		sql_ += " * ";
	}
	else
	{
		sql_ += *pName;
	}

	sql_ += " MDAM ";

	switch (mdamOption)
	{
		case CRUForceOptions::MDAM_ENABLE:
			sql_ += " 'ENABLE'; ";
			break;
		case CRUForceOptions::MDAM_ON:
			sql_ += " 'ON'; ";
			break;
		case CRUForceOptions::MDAM_OFF:
			sql_ += " 'OFF'; ";
			break;
		default:
			ASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::ComposeResetCntrlTableText()
//
// If the name pointer is null then the table_name will be star.
//
//	CONTROL TABLE table_name RESET
//	
//
//--------------------------------------------------------------------------//
void CRURefreshSQLComposer::ComposeResetCntrlTableMDAMText(const CDSString* pName)
{
	sql_ = "CONTROL TABLE ";

	if (NULL == pName)
	{
		sql_ += " * ";
	}
	else
	{
		sql_ += *pName;
	}

	sql_ += " RESET; ";
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::GetContextLogName() 
//--------------------------------------------------------------------------//

CDSString CRURefreshSQLComposer::GetContextLogName() const
{
	return " (RANGE_LOG_TABLE " + GetRootMV().GetFullName() + " ) ";
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::ComposeDeleteContextLogTable() 
//--------------------------------------------------------------------------//

void CRURefreshSQLComposer::ComposeDeleteContextLogTable()
{
	sql_ = " DELETE FROM TABLE " + GetContextLogName();
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::ComposeControlQueryShape() 
//
//
//--------------------------------------------------------------------------//
void CRURefreshSQLComposer::ComposeControlQueryShape()
{
	RUASSERT(NULL != GetRootMV().GetMVForceOption());

	sql_ = "CONTROL QUERY SHAPE  ";

	ComposeQueryShape();

	sql_ += ";";
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::ComposeQueryShape() 
//
// The IR tree for internal refresh  is as follows :
//
//				JOIN
//			   /	\
//           JOIN   UPDATE MV(sub-tree)
//			/    \
//        GRBY   SCAN
//         |      MV
//     DELTA CALC
//     (sub-tree)
//
// We allow the user to force the join between the MV and the GROUP BY
// block and the GROUP BY node above the DELTA CALCULATION sub-tree 
//--------------------------------------------------------------------------//
void CRURefreshSQLComposer::ComposeQueryShape()
{
	CDSString grbyStr,joinStr;
	
	const CRUMVForceOptions& forceOption =
		*GetRootMV().GetMVForceOption();

	switch(forceOption.GetGroupByoption())
	{
		case CRUForceOptions::GB_HASH:
			grbyStr = " HASH_GROUPBY(CUT) ";
			break;
		case CRUForceOptions::GB_SORT:
			grbyStr = " SORT_GROUPBY(CUT) ";
			break;
		case CRUForceOptions::GB_NO_FORCE:
			grbyStr = " GROUPBY(CUT) ";
			break;
		default:
			RUASSERT(FALSE);
	}

	switch(forceOption.GetJoinoption())
	{
		case CRUForceOptions::JOIN_MERGE:
			joinStr = " MERGE_JOIN( ";
			break;
		case CRUForceOptions::JOIN_HASH:
			joinStr = " HYBRID_HASH_JOIN( ";
			break;
		case CRUForceOptions::JOIN_NESTED:
			joinStr = " NESTED_JOIN( ";
			break;
		case CRUForceOptions::JOIN_NO_FORCE:
			joinStr = " JOIN(";
			break;
		default:
			RUASSERT(FALSE);
	}

	sql_ += " JOIN( " + joinStr + grbyStr + ",CUT),CUT)";
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::ComposeControlQueryShapeCut() 
//--------------------------------------------------------------------------//
void CRURefreshSQLComposer::ComposeControlQueryShapeCut()
{
	sql_ = "CONTROL QUERY SHAPE CUT;";
}

//--------------------------------------------------------------------------//
//	CRURefreshSQLComposer::ComposeControlQueryShapeCut() 
//
//--------------------------------------------------------------------------//
void CRURefreshSQLComposer::ComposeShowExplain()
{
	sql_ = "INSERT INTO ";
	sql_ +=	GetRootMV().GetCatName() + "." + GetRootMV().GetSchName() + ".MV_REFRESH_PLANS ";
	sql_ += "SELECT '";
	sql_ += GetRootMV().GetName() +"',";
	sql_ += "PLAN_ID,";
	sql_ += "SEQ_NUM,";
	sql_ += "SUBSTRING(OPERATOR,1,30),";
	sql_ += "LEFT_CHILD_SEQ_NUM,";
	sql_ += "RIGHT_CHILD_SEQ_NUM,";
	sql_ += "SUBSTRING(TNAME,1,60),";
	sql_ += "CARDINALITY,";
	sql_ += "OPERATOR_COST,";
	sql_ += "TOTAL_COST,";
	sql_ += "SUBSTRING(DETAIL_COST,1,1024),";
	sql_ += "SUBSTRING(DESCRIPTION,1,2024)";
	sql_ += " FROM TABLE(EXPLAIN(NULL,'DESCRIPTOR_FOR_SQLSTMT'));";
}

/*
// This is the definition of the table in which the explain data is inserted
create table MV_REFRESH_PLANS 
(
	MV_NAME char(200) NOT NULL NOT DROPPABLE,
	PLAN_ID LARGEINT,
	SEQ_NUM INT,
	OPERATOR CHAR(30),
	LEFT_CHILD_SEQ_NUM INT,
	RIGHT_CHILD_SEQ_NUM INT,
	TNAME CHAR(60),
	CARDINALITY REAL,
	OPERATOR_COST REAL,
	TOTAL_COST REAL,
	DETAIL_COST VARCHAR(1024),
	DESCRIPTION VARCHAR(2024)
) store by (MV_NAME);
*/
