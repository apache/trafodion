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
#ifndef _RU_DUPELIM_SQL_COMPOSER_H_
#define _RU_DUPELIM_SQL_COMPOSER_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuDupElimSQLComposer.h
* Description:  Definition of class CRUDupSQLComposer
*				
*
* Created:      06/14/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "refresh.h"
#include "dsstring.h"

#include "RuSQLComposer.h"

class CRUDupElimTask;
class CRUDupElimGlobals;
class CRUKeyColumn;
class CRUTbl;

//-----------------------------------------------------------//
//	CRUDupElimSQLComposer
//	
//	This class is an auxiliary class for the duplicate 
//	elimination algorithm. It generates the SQL for the delta 
//	computation query and for the Insert/Update/Delete 
//	statements for applying the duplicate elimination 
//	decisions to the IUD- and the range-log.
//
//	The class provides three main methods to generate the SQL:
//	(1) ComposeQueryText()
//	(2) ComposeSingleRowResolvText()
//	(3) ComposeRangeResolvText()
//
//	The GetSQL() method returns the text generated 
//	in the last invocation of one of the ComposeXXX() methods.
//
//-----------------------------------------------------------//

class REFRESH_LIB_CLASS CRUDupElimSQLComposer : public CRUSQLComposer {

private:
	typedef CRUSQLComposer inherited;

public:
	CRUDupElimSQLComposer(CRUDupElimTask *pTask, CRUDupElimGlobals &globals);
	virtual ~CRUDupElimSQLComposer() {}

public:
	// Generate the text of a delta computation query
	void ComposeQueryText(Int32 type);

	// Generate the text of a statement for the single-row resolver
	void ComposeSingleRowResolvText(Int32 type); 

	// Generate the text of a statement for the range resolver
	void ComposeRangeResolvText(Int32 type);

	// Generate the text of the MDAM control statement
	void ComposeControlText(Int32 type);

private:
	//-- Prevent copying
	CRUDupElimSQLComposer(const CRUDupElimSQLComposer& other);	
	CRUDupElimSQLComposer& operator= (const CRUDupElimSQLComposer& other);

private:
	// ComposeQueryText() callees
	void ComposeQControlColNames(CDSString &to);

	void ComposeQClusteringKeyColNames(CDSString &to, BOOL order);
	void ComposeQBlockForEpoch(
		CDSString &qryBase, 
		CDSString &epochCol, 
		TInt32 ep,
		BOOL isPhase0Query);

	void ComposeQBlockForMultipleEpochs(
		CDSString &qryBase, 
		CDSString &epochCol,
		BOOL isPhase0Query);

	void ComposeQBlockForMultipleEpochsBasicPred(
		CDSString &to,
		CDSString &epochCol,
		TInt32 fromEp,
		TInt32 toEp,
		BOOL isPhase0Query);

	CDSString ComposeQBlockLowerBoundPredicate();

private:
	// ComposeSingleRowResolvText() callees
	void ComposeIUDSingleDeleteText();
	void ComposeIUDSingleUpdateIgnoreText();
	void ComposeIUDUpdateBitmapText();
	void ComposeIUDUpdateOptypeText();

	void ComposeUpdateIgnorePrefix(CDSString &cmdPrefix);

	// Search predicates for U/D statements
	void ComposeSingleIUDRecSearchPredicate(CDSString &pred);
	void ComposeUpdateRecsSearchPredicate(CDSString &pred);
	void ComposeCKEqualSearchClause(CDSString &keyPred);

private:
	// ComposeRangeResolvText() callees
	void ComposeRngInsertText();
	void ComposeRngDeleteText();
	void ComposeIUDSubsetDeleteText();
	void ComposeIUDSubsetUpdateIgnoreText();
	void ComposeIUDSubsetUpdateAlwaysIgnoreText();
	
	void ComposeForceMDAMQryShapeText(const CDSString &logName);
	void ComposeResetQryShapeText();
	void ComposeCntrlTableText(BOOL isForceMDAM);

	// ComposeRngInsertText() callees
	void ComposeRngInsertColNames();
	void ComposeRngBoundaryColNames(const CDSString &prefix, CDSString &to);
	void ComposeRngInsertParamStr();

	// ComposeRngDeleteText callees
	void ComposeTupleComparisonRightOperand(CDSString &to);
	void ComposeRngBoundaryParamStr(CDSString &to);

	// ComposeIUDSubsetXXXText() callee
	void ComposeSubsetIUDRecSearchPredicate(
		CDSString &pred, CDSString &epochOp);

	// Search predicates
	void ComposeDirectByClause(CDSString &to);

private:
	static CDSString ComposeQuotedColName(const CDSString &name);
	static CDSString ComposeIUDLogKeyColName(const CRUKeyColumn *pKeyCol);
	
	static CDSString ComposeRngLogKeyColName(
		const CDSString &prefix, const CRUKeyColumn *pKeyCol
	);

private:
	CRUTbl &table_;
	
	Int32  nCtrlColumns_; // The number of control columns in selection list
	Int32  updateBmpSize_;
	TInt32 beginEpoch_;
	TInt32 endEpoch_;

	// The IUD log's name (including namespace)
	CDSString iudLogName_;

	// The range log's name (including namespace)
	CDSString rngLogName_;

	// Are we going to read the single-row records?
	BOOL isSingleRow_;
	// Are we going to read the range records?
	BOOL isRange_;
};

#endif
