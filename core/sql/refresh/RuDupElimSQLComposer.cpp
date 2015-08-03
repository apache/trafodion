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
* File:         RuDupElimSQLComposer.cpp
* Description:  Implementation of class CRUDupSQLComposer
*				
*
* Created:      06/14/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuGlobals.h"
#include "RuOptions.h"
#include "RuDupElimSQLComposer.h"
#include "RuDupElimConst.h"

#include "RuDupElimTask.h"
#include "RuDupElimGlobals.h"
#include "RuTbl.h"
#include "RuKeyColumn.h"

struct CRULogCtlColDesc 
{
	const char *name_;
	const char *datatype_;
};

//-- The descriptors of the IUD log's control columns
//-- (the names are without the @ prefix)

static const CRULogCtlColDesc iudLogCtlColDescVec[] =
{
	{ "EPOCH",  "INT" },
	{ "OPERATION_TYPE", "INT UNSIGNED" },
	{ "RANGE_SIZE",  "INT UNSIGNED" },
	{ "IGNORE", "INT UNSIGNED" },
	{ "UPDATE_BITMAP", "CHAR" },
};

//-- The descriptors of the range log's control columns
//-- (the names are without the @ prefix)

static const CRULogCtlColDesc rngLogCtlColDescVec[] =
{
	{ "EPOCH", "INT" },
	{ "RANGE_ID", "LARGEINT" },
	{ "RANGE_TYPE", "INT UNSIGNED"}
};
 
//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRUDupElimSQLComposer::
CRUDupElimSQLComposer(CRUDupElimTask *pTask, CRUDupElimGlobals &globals) :
	inherited(),
	table_(pTask->GetTable()),
	iudLogName_(table_.GetIUDLogFullName()),
	rngLogName_(table_.GetRangeLogFullName()), 
	beginEpoch_(globals.GetBeginEpoch()),
	endEpoch_(globals.GetEndEpoch()),
	updateBmpSize_(globals.GetUpdateBmpSize()),
	isRange_(globals.IsRangeResolv()),
	isSingleRow_(globals.IsSingleRowResolv()),
	nCtrlColumns_(globals.GetNumCtrlColumns())
{}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeQueryText()
//
//	Generate the text of the delta computation query:
//
//	SELECT <T_CK columns, control columns> FROM <T-IUD-log> 
//	WHERE 
//	(
//	@EPOCH = {+/-} begin-epoch 
//	{AND (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)}
//	)
//	UNION ALL
//	SELECT <T_CK columns, control columns> FROM <T-IUD-log> 
//	WHERE 
//	(
//	@EPOCH = {+/-} begin-epoch+1
//	{AND (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)}
//	)
//	...
//	UNION ALL
//	SELECT <T_CK columns, control columns> FROM <T-IUD-log> 
//	(
//	@EPOCH = {+/-} end-epoch 
//	{AND (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)}
//	)
//
//	ORDER BY <T_CK columns>, @TS
//	FOR BROWSE ACCESS;
//
//	The +/- signs demonstrate that actually there are two blocks -
//	one for negative epochs (range records) and one for positive epochs
//	(single-row records). Only one of the two will be used if the table
//	has a NO RANGELOG/MANUAL RANGELOG attribute.
//
//	This query is highly optimizable. It exploits the log's clustering key 
//	structure (@EPOCH, <T_CK>, @TS) to force the optimizer to use the
//	merge-sort union instead of a simple sort. The BROWSE ACCESS clause 
//	exploits the fact that due to the DDL lock protection, no one apart of
//	DE is working on this portion of the log currently.
//
//	The algorithm's correctness demands that the T_CK columns 
//	in the ORDER BY clause should be appear with
//	the same ASC/DESC specifiers that they have in the STORE BY list.
//
//	If the number of epochs to span is too big, a less 
//	optimizable query will be generated:
//
//	SELECT <control columns, T_CK columns> FROM <T-IUD-log> 
//	WHERE @EPOCH BETWEEN begin-epoch AND end-epoch
//	ORDER BY <T_CK columns>, @TS
//	FOR BROWSE ACCESS;
//
//	(actually, the predicate is more complex because of negative epochs).
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeQueryText(Int32 type)
{	
	BOOL isPhase0Query = 
		(CRUDupElimConst::PHASE_0_QUERY == type) ? TRUE : FALSE;

	if (NULL == CRUGlobals::GetInstance()->GetOptions().
		    FindDebugOption(CRUGlobals::DISPLAY_DE_SEL,"") ) 
	{
		  sql_ = "";
	}
	else
	{
		sql_ = "DISPLAY ";
	}

	CDSString qControlColNames, qCKColNames, qOrderByList;

	ComposeQControlColNames(qControlColNames);
	// Generate the list of selected CK columns
	ComposeQClusteringKeyColNames(qCKColNames, FALSE);

	CDSString qryBase = "SELECT " + qControlColNames + ", " + qCKColNames;
	qryBase += "\nFROM " + iudLogName_ + "\nWHERE ";
	
	CDSString epochCol = ComposeQuotedColName(
		iudLogCtlColDescVec[CRUDupElimConst::OFS_EPOCH].name_);

	if (endEpoch_ - beginEpoch_ + 1 < CRUDupElimConst::MAX_SELECT_CLAUSES)
	{
		// Reasonable number of epochs - optimizable query
		for (TInt32 i=beginEpoch_; i<=endEpoch_; i++)		
		{
			if (TRUE == isSingleRow_)
			{
				ComposeQBlockForEpoch(qryBase, epochCol, i, isPhase0Query);
			}

			if (TRUE == isRange_)
			{
				ComposeQBlockForEpoch(qryBase, epochCol, -i, isPhase0Query);
			}
		}
	}
	else
	{
		// Number of epochs too big
		ComposeQBlockForMultipleEpochs(qryBase, epochCol, isPhase0Query);
	}

	// Generate the list of CK columns for the ORDER BY clause
	// (with the ASC/DESC specifiers)
	ComposeQClusteringKeyColNames(qOrderByList, TRUE);

	sql_ += "\nORDER BY " + qOrderByList + ", ";
        sql_ += ComposeQuotedColName("TS");

	// I am the only one working on this portion of the log...
	sql_ += "\nFOR BROWSE ACCESS;";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeSingleRowResolvText()
//
//	Generate the text of a statement for the single-row resolver.
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeSingleRowResolvText(Int32 type)
{
	if (NULL == CRUGlobals::GetInstance()->GetOptions().
		    FindDebugOption(CRUGlobals::DISPLAY_DE_SR,"") ) 
	{
		  sql_ = "";
	}
	else
	{
		sql_ = "DISPLAY ";
	}
	
	switch (type)	
	{
	case CRUDupElimConst::IUD_LOG_SINGLE_DELETE:
		{
			ComposeIUDSingleDeleteText();
			break;
		}
	case CRUDupElimConst::IUD_LOG_SINGLE_UPDATE_IGNORE:
		{
			ComposeIUDSingleUpdateIgnoreText();
			break;
		}
	case CRUDupElimConst::IUD_LOG_UPDATE_BITMAP:
		{
			ComposeIUDUpdateBitmapText();
			break;
		}
	case CRUDupElimConst::IUD_LOG_UPDATE_OPTYPE:	
		{
			ComposeIUDUpdateOptypeText();
			break;
		}
	default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeRangeResolvText()
//
//	Generate the text of a statement for the range resolver.
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeRangeResolvText(Int32 type)
{
	if (NULL == CRUGlobals::GetInstance()->GetOptions().
		    FindDebugOption(CRUGlobals::DISPLAY_DE_RR,"") ) 
	{
		  sql_ = "";
	}
	else
	{
		sql_ = "DISPLAY ";
	}
	
	switch (type)	
	{
	case CRUDupElimConst::RANGE_LOG_INSERT:
		{
			ComposeRngInsertText();
			break;
		}
	case CRUDupElimConst::RANGE_LOG_DELETE:
		{
			ComposeRngDeleteText();
			break;
		}
	case CRUDupElimConst::IUD_LOG_SUBSET_DELETE:
		{
			ComposeIUDSubsetDeleteText();						
			break;
		}
	case CRUDupElimConst::IUD_LOG_SUBSET_UPDATE_IGNORE:
		{
			ComposeIUDSubsetUpdateIgnoreText();
			break;
		}
	case CRUDupElimConst::IUD_LOG_SUBSET_UPDATE_ALWAYS_IGNORE:
		{
			ComposeIUDSubsetUpdateAlwaysIgnoreText();
			break;
		}
	default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeControlText()
//
//	Generate the text of an MDAM control statement (for the range resolver).
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeControlText(Int32 type)
{
	sql_ = "";

	switch (type)
	{
	case CRUDupElimConst::IUD_LOG_FORCE_MDAM_CQS:
		{
			CDSString iudLogName(
				table_.GetIUDLogFullName(FALSE /*without namespace*/)
			);

			ComposeForceMDAMQryShapeText(iudLogName);
			break;
		}
	case CRUDupElimConst::RNG_LOG_FORCE_MDAM_CQS:
		{
			CDSString rngLogName(
				table_.GetRangeLogFullName(FALSE /*without namespace*/)
			);

			ComposeForceMDAMQryShapeText(rngLogName);
			break;
		}
	case CRUDupElimConst::RESET_MDAM_CQS:
		{
			ComposeResetQryShapeText();
			break;
		}
	case CRUDupElimConst::FORCE_MDAM_CT:
		{
			ComposeCntrlTableText(TRUE /*force MDAM*/);
			break;
		}
	case CRUDupElimConst::RESET_MDAM_CT:
		{
			ComposeCntrlTableText(FALSE);
			break;
		}
	default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//		PRIVATE AREA
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	ComposeQueryText() callees
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeQControlColNames()
//
//	Generate the string of comma-separated names of the
//	log's control columns: @EPOCH, ...
//
//	Two control columns (@IGNORE and @UPDATE_BITMAP) 
//	are used only by DE level 3.
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeQControlColNames(CDSString &to)
{
	for (Int32 i=0; i<nCtrlColumns_; i++)
	{
		to += ComposeQuotedColName(iudLogCtlColDescVec[i].name_);
		to += ", ";
	}

	// Add the log's timestamp to the select list
	to += ComposeQuotedColName("TS");
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeQClusteringKeyColNames()
//
//	Generate the string of comma-separated names of the
//	table's clustering key columns.
//
//	If the *order* parameter is TRUE, the columns list will
//	be adopted for the ORDER BY clause, according to the sort
//	order in the table's clustering key, e.g.:
//		C1 ASC, C2 DESC, C3 ASC
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::
ComposeQClusteringKeyColNames(CDSString &to, BOOL order)
{
	to = "";
	CRUKeyColumnList &keyColList = table_.GetKeyColumnList();

	DSListPosition pos = keyColList.GetHeadPosition();
	for (;;)
	{
		CRUKeyColumn *pKeyCol = keyColList.GetNext(pos);

		CDSString name = ComposeIUDLogKeyColName(pKeyCol);
		to += name;
		if (TRUE == order)
		{
			to += (CDDObject::eDescending == pKeyCol->GetSortOrder()) ?
					" DESC" : " ASC";
		}	
		
		if (NULL == pos)
		{
			// Last column
			break;
		}
		else
		{
			to += ", ";
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeQBlockForEpoch()
//
//	SELECT <T_CK columns, control columns> FROM <T-IUD-log> 
//	WHERE 
//	(
//	@EPOCH = epoch 
//	{AND (col1, col2, ...) > (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)}
//	)
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::
ComposeQBlockForEpoch(CDSString &qryBase, 
					  CDSString &epochCol, 
					  TInt32 ep,
					  BOOL isPhase0Query)
{
	if (0 != sql_.GetLength())
	{
		// This is not the first block
		sql_ += "\nUNION ALL\n";
	}

	CDSString pred("\n(\n" + epochCol + " = " + TInt32ToStr(ep));
	
	if (FALSE == isPhase0Query)
	{
		pred += ComposeQBlockLowerBoundPredicate();
	}

	pred += "\n)";

	sql_ += qryBase + pred;
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeQPredicateForMultipleEpochs()
//
//	A non-optimizable query for a large number of epochs.
//	Example:
//
//	SELECT <T_CK columns, control columns> FROM <T-IUD-log> 
//	WHERE 
//	(
//	@EPOCH BETWEEN 105 AND 108
//	{AND (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)}
//	)
//	OR
//	(
//	@EPOCH BETWEEN -108 AND -105
//	{AND (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)}
//	)
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::
ComposeQBlockForMultipleEpochs(CDSString &qryBase, 
							   CDSString &epochCol,
							   BOOL isPhase0Query
							   )
{
	CDSString pred1, pred2, pred;

	ComposeQBlockForMultipleEpochsBasicPred(
		pred1,
		epochCol,
		beginEpoch_,	// from
		endEpoch_,		// to
		isPhase0Query);

	ComposeQBlockForMultipleEpochsBasicPred(
		pred2,
		epochCol,
		-endEpoch_,		// from
		-beginEpoch_,	// to
		isPhase0Query);

	if (TRUE == isSingleRow_ && FALSE == isRange_)
	{
		pred = pred1;
	}
	else if (TRUE == isRange_ && FALSE == isSingleRow_)
	{
		pred = pred2;
	}
	else
	{
		pred = pred1 + "\nOR\n" + pred2;
	}

	sql_ += qryBase + pred;
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeQBlockForMultipleEpochsBasicPred()
//
//	(
//	@EPOCH BETWEEN ... AND ...
//	{AND (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)}
//	)
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::
ComposeQBlockForMultipleEpochsBasicPred(CDSString &to,
										CDSString &epochCol,
										TInt32 fromEp,
										TInt32 toEp,
										BOOL isPhase0Query)
{
	to = "\n(\n" + epochCol + " BETWEEN " + TInt32ToStr(fromEp) + 
		" AND " + TInt32ToStr(toEp);

	if (FALSE == isPhase0Query)
	{
		to += ComposeQBlockLowerBoundPredicate();
	}

	to += "\n)";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeQBlockLowerBoundPredicate()
//--------------------------------------------------------------------------//

CDSString CRUDupElimSQLComposer::ComposeQBlockLowerBoundPredicate()
{
	CDSString rightOp;	// (?, ?, ..) DIRECTEDBY (...)
	ComposeTupleComparisonRightOperand(rightOp);

	CDSString rngBoundaryColNames;
	ComposeRngBoundaryColNames("" /*no prefix*/, rngBoundaryColNames);
	rngBoundaryColNames = "(" + rngBoundaryColNames + ")";

	// (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (...)
	return "\nAND " + rngBoundaryColNames + " >= " + rightOp;
}

//--------------------------------------------------------------------------//
//	ComposeSingleRowResolvText() callees
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeIUDSingleDeleteText()
//
//	Generate the SQL for delete of a single log record 
//	by the clustering key (@EPOCH, <T_CK>, @TS) 
//	in the log:
//
//	DELETE FROM <T-IUD-log>
//	WHERE <single-record search predicate>
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeIUDSingleDeleteText()
{
	CDSString pred;

	sql_ += "DELETE FROM " + iudLogName_;
	
	ComposeSingleIUDRecSearchPredicate(pred);
	sql_ += pred + ";";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeIUDSingleUpdateIgnoreText()
//
//	Generate the SQL for update of the @IGNORE column 
//	in a single log record, located by the clustering key 
//	in the log:
//
//	UPDATE <T-IUD-log>
//	SET @IGNORE=?
//	WHERE <single-record search predicate>
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeIUDSingleUpdateIgnoreText()
{
	CDSString pred;
	CDSString cmdPrefix;

	ComposeUpdateIgnorePrefix(cmdPrefix);
	sql_ += cmdPrefix;

	ComposeSingleIUDRecSearchPredicate(pred);
	sql_ += pred + ";";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeUpdateIgnorePrefix()
//
//	UPDATE <T-IUD-log> SET @IGNORE=?
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeUpdateIgnorePrefix(CDSString &cmdPrefix)
{
	cmdPrefix = "UPDATE " + iudLogName_;
	
	// SET @IGNORE=?
	cmdPrefix += "\nSET ";
	const CRULogCtlColDesc &desc = 
		iudLogCtlColDescVec[CRUDupElimConst::OFS_IGNORE];

	cmdPrefix += ComposeQuotedColName(desc.name_);
	cmdPrefix += "=" + ComposeCastExpr(desc.datatype_);
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeIUDUpdateBitmapText()
//
//	Generate the SQL for updating the @UPDATE_BITMAP column
//	of the log to a superposition of all the non-null update 
//	bitmaps in the duplicate chain.
//
//	UPDATE <T-IUD-log>
//	SET @UPDATE_BITMAP = ?
//	WHERE <update-records search predicate>
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeIUDUpdateBitmapText()
{
	CDSString pred;
	sql_ += "UPDATE " + iudLogName_;
	
	const CRULogCtlColDesc &desc = 
		iudLogCtlColDescVec[CRUDupElimConst::OFS_UPD_BMP];

	// Compose the real datatype 
	// of the @UPDATE_BITMAP column - CHAR(<size>)
	CDSString datatype(desc.datatype_);

	RUASSERT(updateBmpSize_ > 0);
	datatype += "(" + TInt32ToStr(updateBmpSize_) + ")";

	// -------------------------------------------------------------
    // Date: 2008-03-19  Caroline:
	// In UNICODE config: ISO_MAPPING=UTF8, DEFAULT_CHARSET= UCS2
	// The IUD_LOG_TABLE Clause: 
	//       SET "@UPDATE_BITMAP" = CAST (? AS CHAR(8)) 
	// The "@UPDATE_BITMAP" column is in ISO88591, and the CAST Clause 
	// is implied to UCS2, so we got the incompatible error. 
	// To fix the error, we explicitly say "CHARACTER SET ISO88591".

	datatype += " CHARACTER SET ISO88591 ";
	//---------------------------------------------

	sql_ += "\nSET " + ComposeQuotedColName(desc.name_) + " = ";
	sql_ += ComposeCastExpr(datatype);		

	ComposeUpdateRecsSearchPredicate(pred);
	sql_+= pred + ";";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeIUDUpdateOptypeText()
//
//	Generate the SQL for updating the @OPERATION_TYPE column
//	of the log (switching the IS_PART_OF_UPDATE bit off) for
//	all the records in the duplicate chain that are logged as 
//	a part of Update operation.
//
//	UPDATE <T-IUD-log>
//	SET @OPERATION_TYPE = @OPERATION_TYPE - 2
//	WHERE <update-records search predicate>
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeIUDUpdateOptypeText()
{
	CDSString pred;
	sql_ += "UPDATE " + iudLogName_;
	
	const CRULogCtlColDesc &desc = 
		iudLogCtlColDescVec[CRUDupElimConst::OFS_OPTYPE];
	CDSString name = ComposeQuotedColName(desc.name_);

	sql_ += "\nSET " + name + " = ";
	sql_ += name + "-" + TInt32ToStr(CRUDupElimConst::IS_PART_OF_UPDATE);

	ComposeUpdateRecsSearchPredicate(pred);
	sql_+= pred + ";";
}

//--------------------------------------------------------------------------//
//	Search predicates
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeSingleIUDRecSearchPredicate()
//
//	Generate the WHERE predicate for locating a single 
//	record in the log by the clustering key:
//
//	WHERE 
//	@EPOCH=?
//	AND
//	<clustering-key-search-predicate>
//	AND
//	@TS=?
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeSingleIUDRecSearchPredicate(CDSString &pred)
{
	pred = "\nWHERE ";
	// @EPOCH=?
	const CRULogCtlColDesc &desc = 
		iudLogCtlColDescVec[CRUDupElimConst::OFS_EPOCH];

	pred += ComposeQuotedColName(desc.name_);
	pred += "=" + ComposeCastExpr(desc.datatype_) + "\nAND ";

	CDSString ckClause;
	ComposeCKEqualSearchClause(ckClause);
	pred += ckClause;

        pred += "\nAND ";
        pred += ComposeQuotedColName("TS");
	pred += " = " + ComposeCastExpr("LARGEINT");
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeUpdateRecsSearchPredicate()
//
//	Generate the predicate for locating all the log records
//	for the same clustering key that are logged as part
//	of Update.
//
//	WHERE
//	(@EPOCH BETWEEN begin-epoch and end-epoch)
//	AND
//	<clustering-key-search-predicate>
//	AND
//	(@OPERATION_TYPE = 2 OR @OPERATION_TYPE = 3)
//	
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeUpdateRecsSearchPredicate(CDSString &pred)
{
	pred = "\nWHERE \n(";
	const CRULogCtlColDesc &desc1 = 
		iudLogCtlColDescVec[CRUDupElimConst::OFS_EPOCH];

	pred += ComposeQuotedColName(desc1.name_);
	pred += " BETWEEN " + TInt32ToStr(beginEpoch_) 
			+ " AND " + TInt32ToStr(endEpoch_) + ")\nAND ";

	CDSString ckClause;
	ComposeCKEqualSearchClause(ckClause);
	pred += ckClause;

	CDSString type1 = TInt32ToStr(CRUDupElimConst::IS_PART_OF_UPDATE);
	CDSString type2 = TInt32ToStr(
		CRUDupElimConst::IS_PART_OF_UPDATE + CRUDupElimConst::IS_DELETE);

	const CRULogCtlColDesc &desc2 = 
		iudLogCtlColDescVec[CRUDupElimConst::OFS_OPTYPE];
	CDSString optypeColName = ComposeQuotedColName(desc2.name_);

	pred += "\nAND (" + optypeColName + " = " + type1;
	pred += " OR " + optypeColName + " = " + type2 + ")";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeCKEqualSearchClause()
//
//	Generate the clause to locate all the log records
//	that relate to the given clustering key value:
//
//	(<T-CK-col1 = ?> AND ... <T-CK-colN = ?>)
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeCKEqualSearchClause(CDSString &ckClause)
{
	ckClause = "(";

	CRUKeyColumnList &keyColList = table_.GetKeyColumnList();
	DSListPosition pos = keyColList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUKeyColumn *pKeyCol = keyColList.GetNext(pos);
		CDSString name = ComposeIUDLogKeyColName(pKeyCol);
		
		ckClause += name + "=" + ComposeCastExpr(pKeyCol->GetTypeString());
		if (NULL != pos)
		{
			ckClause += "\nAND ";
		}
		else
		{
			ckClause += ")";
		}
	}
}

//--------------------------------------------------------------------------//
//	ComposeRangeResolvText() callees
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeCntrlQryShapeText()
//	
//	CONTROL QUERY SHAPE TSJ 
//	(EXCHANGE (SCAN (TABLE '<T-XXX-log>', MDAM FORCED)), CUT)
//
//	The CQS statement forces the optimizer to generate 
//	the following fragment in the plan:
//
//	                 TSJ
//	             /        \
//          EXCHANGE      EXCHANGE
//              |             |
//	    SCAN(with MDAM)   CURSOR_DELETE
//
//	WARNING: This plan can become problematic when the optimizer
//	is taught to push the TSJ node into DP2.
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::
ComposeForceMDAMQryShapeText(const CDSString &logName)
{
	sql_ += "CONTROL QUERY SHAPE TSJ (EXCHANGE (SCAN (TABLE '";
	sql_ += logName + "', MDAM FORCED)), CUT);";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeResetQryShapeText()
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeResetQryShapeText()
{
	sql_ += "CONTROL QUERY SHAPE OFF;";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeCntrlTableText()
//
//	CONTROL TABLE * MDAM 'ON'
//	/
//	CONTROL TABLE * RESET
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeCntrlTableText(BOOL isForceMDAM)
{
	sql_ += "CONTROL TABLE *";
	sql_ += (TRUE == isForceMDAM) ? " MDAM 'ON';" : " RESET;";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeRngInsertText()
//
//	INSERT INTO <T-range-log>
//	(list-of-columns)
//	VALUES (?, ..., ?,  -- control column values
//			?, ..., ?,  -- begin-range CK values
//			?, ..., ?)	-- end-range CK values
//	
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeRngInsertText()
{
	sql_ += "INSERT INTO " + rngLogName_;
	ComposeRngInsertColNames();

	sql_ += "\nVALUES (";
	ComposeRngInsertParamStr();	
	sql_ += "\n);";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeRngInsertColNames()
//
//	Generate the list of all the columns in the range log.
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeRngInsertColNames()
{
	sql_ += "\n(";
	for (Int32 i=0; i<CRUDupElimConst::NUM_RNG_LOG_CONTROL_COLS; i++)
	{
		sql_ += ComposeQuotedColName(rngLogCtlColDescVec[i].name_);
		sql_ += ", ";
	}

	CDSString rngBoundaryColNames;
	ComposeRngBoundaryColNames("BR_", rngBoundaryColNames);
	sql_ += rngBoundaryColNames + ", ";
	ComposeRngBoundaryColNames("ER_", rngBoundaryColNames);
	sql_ += rngBoundaryColNames + ")";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeRngBoundaryColNames()
//	
//	Compose the list of mappings of all the table's 
//	clustering key columns to the log columns.
//
//	In the case of the IUD log, this is a 1-to-1 mapping.
//	In the case of the range log, a begin-range(@BR_) or 
//	end-range(@ER_) prefix is added to each column.
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::
ComposeRngBoundaryColNames(const CDSString &prefix, CDSString &to)
{
	to = "";
	BOOL noPrefix = (0 == prefix.GetLength()); 

	CRUKeyColumnList &keyColList = table_.GetKeyColumnList();
	DSListPosition pos = keyColList.GetHeadPosition();

	while (NULL != pos)
	{
		CRUKeyColumn *pKeyCol = keyColList.GetNext(pos);
		if (TRUE == noPrefix)
		{
			// IUD log column
			to += ComposeIUDLogKeyColName(pKeyCol);
		}
		else
		{
			// Range log column
			to += ComposeRngLogKeyColName(prefix, pKeyCol);
		}

		if (NULL != pos)
		{
			to += ", ";
		}
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeRngInsertParamStr()
//	
//	Compose the parameter string for the Insert statement
//	into the range log.
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeRngInsertParamStr()
{
	// Generate the placeholders for N clustering index columns
	CDSString rngBoundaryParamStr;
	ComposeRngBoundaryParamStr(rngBoundaryParamStr);

	for (Int32 i=0; i<CRUDupElimConst::NUM_RNG_LOG_CONTROL_COLS; i++)
	{
		sql_ += ComposeCastExpr(rngLogCtlColDescVec[i].datatype_) + ", ";
	}

	// Add the placeholders string twice 
	// (for the begin-range and end-range values)
	sql_ += "\n" + rngBoundaryParamStr;
	sql_ += ",\n" + rngBoundaryParamStr;
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeRngDeleteText()
//
//	DELETE FROM <T-range-log>
//	WHERE 
//	@EPOCH = ? 
//	AND
//	@RANGE_ID = ?
//	AND
//  (@BR_col1, @BR_col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)
//	AND
//  (@ER_col1, @ER_col2, ...) <= (?, ?, ..) DIRECTEDBY (...)
//	AND
//  (@ER_col1, @ER_col2, ...) >= (?, ?, ..) DIRECTEDBY (...)
//
//	All the range fragments to be deleted are identified 
//	by the same (@EPOCH, @RANGE_ID) pair. The predicates
//	on the @BR/@ER columns are required to guarantee that
//	no range fragments that do not overlap with this range,
//	but have the same timestamp (@RANGE_ID) value are
//	accidentally deleted from the range log (they can come
//	from different partitions with independent clocks).
//	The last predicate is not required logically. It is an
//	optimization to help the SQL narrow the search space
//	(the @ER columns are a part of the range log's clustering
//	key).
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeRngDeleteText()
{
	sql_ += "DELETE FROM " + rngLogName_ + "\nWHERE ";

	// @EPOCH = ? AND @RANGE_ID = ? AND
	for (Int32 i=0; i<2; i++)
	{
		sql_ += ComposeQuotedColName(rngLogCtlColDescVec[i].name_) + "=";
		sql_ += ComposeCastExpr(rngLogCtlColDescVec[i].datatype_);
		sql_ += "\nAND ";
	}

	CDSString rightOp;	// (?, ?, ..) DIRECTEDBY (...)
	ComposeTupleComparisonRightOperand(rightOp);

	CDSString rngBoundaryColNames;
	ComposeRngBoundaryColNames("BR_", rngBoundaryColNames);
	rngBoundaryColNames = "(" + rngBoundaryColNames + ")";

	// (@BR_col1, @BR_col2, ...) >= (?, ?, ..) DIRECTEDBY (...)
	sql_ += rngBoundaryColNames + " >= " + rightOp;

	sql_+= "\nAND ";
	// (@ER_col1, @ER_col2, ...) <= (?, ?, ..) DIRECTEDBY (...)
	ComposeRngBoundaryColNames("ER_", rngBoundaryColNames);
	rngBoundaryColNames = "(" + rngBoundaryColNames + ")";
	sql_ += rngBoundaryColNames + " <= " + rightOp;

	// (@ER_col1, @ER_col2, ...) <= (?, ?, ..) DIRECTEDBY (...)
	// This clause is added for performance
	sql_+= "\nAND ";
	sql_ += rngBoundaryColNames + " >= " + rightOp;
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeIUDSubsetDeleteText()
//
//	DELETE FROM <T-IUD-log>
//	WHERE
//	@EPOCH = ?
//	AND
//	<CK-range-search-predicate>
//	AND
//	@TS > ?
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeIUDSubsetDeleteText()
{
	CDSString pred("");

	sql_ += "DELETE FROM " + iudLogName_;
	
	// Common predicate part
	CDSString epochOp("=");
	ComposeSubsetIUDRecSearchPredicate(pred, epochOp);

        pred += "\nAND ";
        pred += ComposeQuotedColName("TS");
	pred += " > " + ComposeCastExpr("LARGEINT");
	
	sql_ += "\n" + pred + ";";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeIUDSubsetUpdateAlwaysIgnoreText()
//
//	UPDATE <T-IUD-log>
//	SET @IGNORE=?
//	WHERE 
//	@EPOCH = ?
//	AND
//	<CK-range-search-predicate>
//	AND 
//	@TS > ?
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeIUDSubsetUpdateAlwaysIgnoreText()
{
	CDSString cmdPrefix;
	CDSString pred;

	// UPDATE ... SET
	ComposeUpdateIgnorePrefix(cmdPrefix);
	sql_ += cmdPrefix;

	// Common predicate part
	CDSString epochOp("=");
	ComposeSubsetIUDRecSearchPredicate(pred, epochOp);

	pred += "\nAND ";
        pred += ComposeQuotedColName("TS");
	pred += " > " + ComposeCastExpr("LARGEINT");

	sql_ += "\n" + pred + ";";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeIUDSubsetUpdateIgnoreText()
//
//	UPDATE <T-IUD-log>
//	SET @IGNORE=?
//	WHERE 
//	@EPOCH = ?
//	AND
//	<CK-range-search-predicate>
//	AND 
//	@IGNORE < ?
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeIUDSubsetUpdateIgnoreText()
{
	CDSString cmdPrefix;
	CDSString pred;

	// UPDATE ... SET
	ComposeUpdateIgnorePrefix(cmdPrefix);
	sql_ += cmdPrefix;

	// Common predicate part
	CDSString epochOp("=");
	ComposeSubsetIUDRecSearchPredicate(pred, epochOp);

	// AND @IGNORE < ?
	const CRULogCtlColDesc &desc = 
		iudLogCtlColDescVec[CRUDupElimConst::OFS_IGNORE];
	pred += "\nAND " + ComposeQuotedColName(desc.name_);
	pred += "<" + ComposeCastExpr(desc.datatype_);

	sql_ += "\n" + pred + ";";
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeSubsetIUDRecSearchPredicate()
//
//	The predicate for searching a range of IUD log records
//	by the Range resolver.	
//
//	WHERE
//	@EPOCH = ?	-- or @EPOCH > ?
//	AND
//  (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)
//	AND
//  (col1, col2, ...) <= (?, ?, ..) DIRECTEDBY (...)
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::
ComposeSubsetIUDRecSearchPredicate(CDSString &pred, CDSString &epochOp)
{
	pred = "WHERE ";

	// @EPOCH <epochOp> ?
	const CRULogCtlColDesc &desc1 = 
		iudLogCtlColDescVec[CRUDupElimConst::OFS_EPOCH];
	pred += ComposeQuotedColName(desc1.name_);
	pred += epochOp + ComposeCastExpr(desc1.datatype_);

	CDSString rightOp;	// (?, ?, ..) DIRECTEDBY (...)
	ComposeTupleComparisonRightOperand(rightOp);

	CDSString rngBoundaryColNames;
	ComposeRngBoundaryColNames("" /*no prefix*/, rngBoundaryColNames);
	rngBoundaryColNames = "(" + rngBoundaryColNames + ")";

	// (col1, col2, ...) >= (?, ?, ..) DIRECTEDBY (...)
	pred += "\nAND " + rngBoundaryColNames + " >= " + rightOp;

	// (col1, col2, ...) <= (?, ?, ..) DIRECTEDBY (...)
	pred += "\nAND " + rngBoundaryColNames + " <= " + rightOp;
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeDirectByClause()
//
//	Generate the clause DIRECTEDBY (ASC/DESC, ASC/DESC, ...)
//	for the CK comparison predicate in the DELETE statement.
//
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeDirectByClause(CDSString &to)
{
	to = " DIRECTEDBY (";
	CRUKeyColumnList &keyColList = table_.GetKeyColumnList();

	DSListPosition pos = keyColList.GetHeadPosition();
	while (NULL != pos)
	{
		CRUKeyColumn *pKeyCol = keyColList.GetNext(pos);
		to += (CDDObject::eDescending == pKeyCol->GetSortOrder()) ?
				" DESC" : " ASC";

		to += (NULL == pos) ? ")" : ",";
	}
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeTupleComparisonRightOperand()
//	
//	Compose the clause (?, ?, ..) DIRECTEDBY (ASC/DESC, ...)
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::
ComposeTupleComparisonRightOperand(CDSString &to)
{
	CDSString directbyClause;
	ComposeDirectByClause(directbyClause);

	// Generate the placeholders for N clustering index columns
	ComposeRngBoundaryParamStr(to);
	to = "(" + to + ")" + directbyClause;
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeRngBoundaryParamStr()
//
//	Compose N placeholders for the clustering key values
//	in the INSERT/DELETE statement on the range log.
//	
//--------------------------------------------------------------------------//

void CRUDupElimSQLComposer::ComposeRngBoundaryParamStr(CDSString &to)
{
	to = "";
	CRUKeyColumnList &keyColList = table_.GetKeyColumnList();
	DSListPosition pos = keyColList.GetHeadPosition();

	while (NULL != pos)
	{
		CRUKeyColumn *pKeyCol = keyColList.GetNext(pos);
		to += ComposeCastExpr(pKeyCol->GetTypeString());
		if (NULL != pos)
		{
			to += ", ";
		}
	}
}

//--------------------------------------------------------------------------//
//	Service methods
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeQuotedColName
//--------------------------------------------------------------------------//

CDSString CRUDupElimSQLComposer::ComposeQuotedColName(const CDSString &name)
{
	return 
		CRUSQLComposer::ComposeQuotedColName(CRUTbl::logCrtlColPrefix, name);
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeIUDLogKeyColName()
//
//	Map the table's column name to the IUD log's one.
//	
//	In the log, the clustering key columns have the same names 
//	as in the base table, except SYSKEY (if the table has one), 
//	which becomes @SYSKEY in the log.
//
//--------------------------------------------------------------------------//

CDSString CRUDupElimSQLComposer::
ComposeIUDLogKeyColName(const CRUKeyColumn *pKeyCol)
{
	CDSString name(pKeyCol->GetName());

	if (CDSString("SYSKEY") == name)
	{
		// If the original table has a syskey, it becomes @SYSKEY in the log
		// (the log has a syskey of its own).
		name = ComposeQuotedColName(name);
	}

	return name;
}

//--------------------------------------------------------------------------//
//	CRUDupElimSQLComposer::ComposeRngLogKeyColName()
//
//	Map the table's column name to the range log's one, 
//	with the @BR_ or @ER_ prefix.
//
//--------------------------------------------------------------------------//

CDSString CRUDupElimSQLComposer::
ComposeRngLogKeyColName(const CDSString &prefix, const CRUKeyColumn *pKeyCol)
{
	return 
		CRUSQLComposer::ComposeQuotedColName(
			CRUTbl::logCrtlColPrefix + prefix,
			pKeyCol->GetName());
}
