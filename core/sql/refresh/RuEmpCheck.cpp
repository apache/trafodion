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
* File:         RuEmpCheck.cpp
* Description:  Implementation of class CRUEmpCheck
*
*
* Created:      10/15/2000
* Language:     C++
*
*
******************************************************************************
*/

#include "RuEmpCheck.h"

#include "dmresultset.h"
#include "uofsIpcMessageTranslator.h"

#include "RuSQLComposer.h"
#include "RuSQLDynamicStatementContainer.h"
#include "RuTbl.h"

//--------------------------------------------------------------------------//
//	Constructors and destructor
//--------------------------------------------------------------------------//

CRUEmpCheck::CRUEmpCheck() :
	pVec_(NULL), 
	pSQLContainer_(new CRUSQLDynamicStatementContainer(NUM_STMTS)),
	checkMask_(0)
{}

CRUEmpCheck::CRUEmpCheck(CRUEmpCheckVector &vec) :
	pVec_(new CRUEmpCheckVector(vec)), 
	pSQLContainer_(new CRUSQLDynamicStatementContainer(NUM_STMTS)),
	checkMask_(0)
{}

CRUEmpCheck::~CRUEmpCheck()
{
	delete pVec_;

	delete pSQLContainer_;
}

//--------------------------------------------------------------------------//
//	CRUEmpCheck::LoadData()
//--------------------------------------------------------------------------//

void CRUEmpCheck::LoadData(CUOFsIpcMessageTranslator &translator)
{
	pSQLContainer_->LoadData(translator);

	if (NULL == pVec_)
	{
		pVec_ = new CRUEmpCheckVector();
	}
	
	pVec_->LoadData(translator);

	translator.ReadBlock(&checkMask_, sizeof(Lng32));
}

//--------------------------------------------------------------------------//
//	CRUEmpCheck::StoreData()
//--------------------------------------------------------------------------//

void CRUEmpCheck::StoreData(CUOFsIpcMessageTranslator &translator)
{
	pSQLContainer_->StoreData(translator);
	pVec_->StoreData(translator);

	translator.WriteBlock(&checkMask_, sizeof(Lng32));
}

//--------------------------------------------------------------------------//
//	CRUEmpCheck::ComposeSQL()
//
//	Determine which kind of checks are required (based on the table's)
//	RANGELOG attribute. In accordance with that, compose one or two
//	SQL statements which will probe the data in the positive (for single-rows)
//	and negative (for ranges) epochs, respectively.
//
//--------------------------------------------------------------------------//

void CRUEmpCheck::ComposeSQL(CRUTbl &tbl, TInt32 upperBound)
{
	CDSString sql;

	checkMask_ = tbl.GetIUDLogContentTypeBitmap();

	if (0 != (checkMask_ & CRUTbl::RANGE))
	{
		ComposeSelectStmt(tbl, sql, CHECK_NEG_EPOCHS, -upperBound);
		pSQLContainer_->SetStatementText(CHECK_NEG_EPOCHS, sql);
	}

	if (0 != (checkMask_ & CRUTbl::SINGLE_ROW_INSERTS))
	{
		ComposeSelectStmt(tbl, sql, CHECK_POS_EPOCHS_INSERTS, upperBound);
		pSQLContainer_->SetStatementText(CHECK_POS_EPOCHS_INSERTS, sql);
	}

	if (0 != (checkMask_ & CRUTbl::SINGLE_ROW_OTHER))
	{
		ComposeSelectStmt(tbl, sql, CHECK_POS_EPOCHS_DELETES, upperBound);
		pSQLContainer_->SetStatementText(CHECK_POS_EPOCHS_DELETES, sql);
	}

}

//--------------------------------------------------------------------------//
//	CRUEmpCheck::PrepareSQL()
//--------------------------------------------------------------------------//

void CRUEmpCheck::PrepareSQL()
{
	pSQLContainer_->PrepareSQL();
}

//--------------------------------------------------------------------------//
//	CRUEmpCheck::PerformCheck()
//
//	Iterate through the vector, in the descending order of epochs. 
//	For each epoch value, perform the "SELECT [FIRST 1] ... " query 
//	for the emptiness check.
//	
//	Once a non-empty delta (of a certain kind) is found, then all the deltas
//	of this kind that start at smaller epochs are also non-empty. 
//	The algorithm takes this into account, by minimizing the number
//	of disk I/Os it performs.
//
//--------------------------------------------------------------------------//

void CRUEmpCheck::PerformCheck()
{	
	CRUEmpCheckVector::Elem *pElem;
	CRUEmpCheckVecIterator it(*pVec_);

	while (NULL != (pElem = it.GetCurrentElem()))
	{
		PerformSingleCheck(
			*pElem, 
			CRUTbl::RANGE,
			CHECK_NEG_EPOCHS);

		PerformSingleCheck(
			*pElem, 
			CRUTbl::SINGLE_ROW_INSERTS,
			CHECK_POS_EPOCHS_INSERTS);

		PerformSingleCheck(
			*pElem, 
			CRUTbl::SINGLE_ROW_OTHER,
			CHECK_POS_EPOCHS_DELETES);

		it.Next();
	}
}

//--------------------------------------------------------------------------//
//	PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUEmpCheck::ComposeSelectStmt()
//
//	Compose the SELECT statement for the positive/negative epoch probe:
//
//	For the positive epoch, for example, it will look like:
//
//	SELECT [FIRST 1] "@EPOCH"
//	FROM <T-IUD-log>
//	WHERE "@EPOCH" >= ?
//	{AND "@EPOCH" <= <upper bound>} - optional
//
//--------------------------------------------------------------------------//

void CRUEmpCheck::
ComposeSelectStmt(CRUTbl &tbl, CDSString &to, 
				  StmtType StmtType, TInt32 upperBound)
{
	RUASSERT(CHECK_POS_EPOCHS_INSERTS == StmtType || 
	         CHECK_POS_EPOCHS_DELETES == StmtType || 
	         CHECK_NEG_EPOCHS == StmtType);

	CDSString iudLogName(tbl.GetIUDLogFullName());

	CDSString epochColName("EPOCH");
	epochColName = 
		CRUSQLComposer::
		ComposeQuotedColName(CRUTbl::logCrtlColPrefix, epochColName);

	CDSString opColName("OPERATION_TYPE");
	opColName = 
		CRUSQLComposer::
		ComposeQuotedColName(CRUTbl::logCrtlColPrefix, opColName);

	CDSString op1, op2;

	if (CHECK_NEG_EPOCHS == StmtType) 
	{
		op1 = " <= ";
		op2 = " >= ";
	}
	else
	{
		op1 = " >= ";
		op2 = " <= ";
	}

	to = "SELECT [FIRST 1] " + epochColName + "\nFROM " + iudLogName;
	to += "\nWHERE " + epochColName + op1 + CRUSQLComposer::ComposeCastExpr("INT");

	if (0 != upperBound)
	{
		to += "\nAND " + epochColName + op2
			+ CRUSQLComposer::TInt32ToStr(upperBound);
	}	
	
	if (CHECK_POS_EPOCHS_INSERTS == StmtType) 
	{
	        // Add a predicate specific for single row inserts
		to += "\nAND " + opColName + " = 0"; 
	}
	else if (CHECK_POS_EPOCHS_DELETES == StmtType) 
	{
	        // Add a predicate specific for single row deletes and updates
		to += "\nAND (" + opColName + " = 1 OR " + opColName + " = 3)"; 
	}
	  
	to += ";";
}

//--------------------------------------------------------------------------//
//	CRUEmpCheck::PerformSingleCheck()
//	
//	Perform a single (SELECT [FIRST 1]) probe for a certain type of records,
//	starting from the given epoch.
//
//	If the check of the larger epoch has produced a positive result (epochs
//	are scanned top-down), this check will always reproduce it, and
//	therefore, can be escaped.
//
//--------------------------------------------------------------------------//

void CRUEmpCheck::
PerformSingleCheck(CRUEmpCheckVector::Elem &elem, 
				   CRUTbl::IUDLogContentType ct,
				   StmtType stmtType)
{
	if (0 == (ct & checkMask_))
	{
		// We are not interested in this check
		return;
	}

	if (0 != (ct & elem.checkBitmap_))
	{
		// The check of the larger epoch has produced a positive result
		return;
	}

	CDMPreparedStatement *pStmt = 
		pSQLContainer_->GetPreparedStatement(stmtType);

	RUASSERT(NULL != pStmt);
	
	TInt32 ep = elem.epoch_;
	if (CRUTbl::RANGE == ct)
	{
		ep = -ep;	// Range records reside in negative epochs
	}

	pStmt->SetInt(1, ep);

	//	Execute the "SELECT [FIRST 1] ... " query.
	//	If a row is fetched, the specific delta is non-empty.
	pStmt->ExecuteQuery();

	CDMResultSet *pResultSet = pStmt->GetResultSet();
	BOOL isNonEmpty = pResultSet->Next();

	// The statement must be closed before the next invocation
	pStmt->Close();

	if (TRUE == isNonEmpty)
	{
		pVec_->SetDeltaNonEmpty(elem.epoch_, ct);
	}
}
