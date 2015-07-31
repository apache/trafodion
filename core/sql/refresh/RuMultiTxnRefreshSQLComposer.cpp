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
* File:         RuSQLMultiTxnRefreshComposer.cpp
* Description:  Implementation of class RuSQLMultiTxnRefreshComposer
*				
*
* Created:      08/17/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuSQLDynamicStatementContainer.h"
#include "RuMultiTxnRefreshSQLComposer.h"
#include "RuTbl.h"

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRUMultiTxnRefreshSQLComposer::
CRUMultiTxnRefreshSQLComposer(CRURefreshTask *pTask) 
	: CRURefreshSQLComposer(pTask) 
{}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshSQLComposer::ComposeRefresh() 
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshSQLComposer::ComposeRefresh(Int32 phase, BOOL catchup)
{
	StartInternalRefresh();
	
	sql_ += " FROM SINGLEDELTA ";

	CRUDeltaDef *pDdef = GetRefreshTask().GetDeltaDefList().GetAt(0);
	
	CDSString epochParameter(CRUSQLDynamicStatementContainer::COMPILED_PARAM_TOKEN);

	AddDeltaDefClause(pDdef,epochParameter,epochParameter);

	AddNRowsClause(phase, catchup);

	if (GetRefreshTask().GetMVList().GetCount() > 1)
	{
		AddPipeLineClause();
	}
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshSQLComposer::AddNRowsClause() 
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshSQLComposer::AddNRowsClause(Int32 phase, BOOL catchup)
{
	sql_ += "\n\t COMMIT EACH ";
	sql_ += TInt32ToStr(GetRootMV().GetCommitNRows());
	
	sql_ += " PHASE ";
	sql_ += TInt32ToStr(phase);

	if (catchup)
	{
		sql_ += " CATCHUP ";
		sql_ += CRUSQLDynamicStatementContainer::COMPILED_PARAM_TOKEN;
		sql_ += " ";
	}
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshSQLComposer::ComposeReadContextLog() 
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshSQLComposer::ComposeReadContextLog()
{
	CDSString epochColName(ComposeQuotedColName(CRUTbl::logCrtlColPrefix, "EPOCH"));

	sql_ = "SELECT " ;
	sql_ += epochColName;
	sql_ += " FROM TABLE ";
	sql_ += GetContextLogName();
	sql_ += " ORDER BY ";
	sql_ += epochColName;
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnRefreshSQLComposer::ComposeCQSForIRPhase1() 
//
//			  JOIN 
//          /      \
//		  CUT	JOIN
//			   /	\
//           JOIN   UPDATE MV(sub-tree)
//			/    \
//        GRBY   SCAN
//         |      MV
//     DELTA CALC
//     (sub-tree)
//--------------------------------------------------------------------------//

void CRUMultiTxnRefreshSQLComposer::ComposeCQSForIRPhase1()
{
	RUASSERT(NULL != GetRootMV().GetMVForceOption());

	const CRUMVForceOptions& forceOption =
		*GetRootMV().GetMVForceOption();

	sql_ = "CONTROL QUERY SHAPE  ";

	sql_ += " JOIN (CUT, ";
	inherited::ComposeQueryShape();
	sql_ += " ); ";	
}

