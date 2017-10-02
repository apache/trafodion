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
* File:         RuDupElimTaskExUnit.h
* Description:  Implementation of classes 
*               CRUDupElimTaskExUnit and CRUDupElimResolver
*				
*
* Created:      04/09/2001
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuDupElimTaskExUnit.h"
#include "RuDupElimConst.h"

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRUDupElimTaskExUnit::
CRUDupElimTaskExUnit(const CRUDupElimGlobals &dupElimGlobals,
					 CRUSQLDynamicStatementContainer &ctrlStmtContainer,
					 Int32 nStmts) :
	dupElimGlobals_(dupElimGlobals),
	ctrlStmtContainer_(ctrlStmtContainer),
	stmtContainer_(nStmts) 
{}

//--------------------------------------------------------------------------//
//	CRUDupElimResolver::ExecuteCQSForceMDAM()
//	
//	Execute the CONTROL QUERY SHAPE statement to force the MDAM optimization.
//
//	Since the statements are executed in arkcmp (rather than
//	by the executor), the special syntax parameter must be
//	passed at the execution-time, rather than at compile-time.
//
//--------------------------------------------------------------------------//

void CRUDupElimResolver::ExecuteCQSForceMDAM(LogType logType)
{
	CDMPreparedStatement *pStmt;

	if (IUD_LOG == logType)
	{
		//	CONTROL QUERY SHAPE TSJ 
		//	(EXCHANGE (SCAN (TABLE '<T-IUD-log>', MDAM FORCED)), CUT)
		pStmt =	GetControlStmtContainer().GetPreparedStatement(
			CRUDupElimConst::IUD_LOG_FORCE_MDAM_CQS
		);
	}
	else
	{
		//	CONTROL QUERY SHAPE TSJ 
		//	(EXCHANGE (SCAN (TABLE '<T-range-log>', MDAM FORCED)), CUT)
		pStmt =	GetControlStmtContainer().GetPreparedStatement(
			CRUDupElimConst::RNG_LOG_FORCE_MDAM_CQS
		);
	}

	pStmt->ExecuteUpdate(TRUE/* special syntax*/);
	pStmt->Close();
}

//--------------------------------------------------------------------------//
//	CRUDupElimResolver::ExecuteCQSOff()
//--------------------------------------------------------------------------//

void CRUDupElimResolver::ExecuteCQSOff()
{
	CDMPreparedStatement *pStmt;

	//	CONTROL QUERY SHAPE OFF
	pStmt =	GetControlStmtContainer().GetPreparedStatement(
		CRUDupElimConst::RESET_MDAM_CQS
	);

	pStmt->ExecuteUpdate(TRUE/* special syntax*/);
	pStmt->Close();
}

#ifdef _DEBUG
//--------------------------------------------------------------------------//
//	CRUDupElimResolver::DumpStmtStatistics()
//	
//	Print the number of invocations of a statement
//--------------------------------------------------------------------------//

void CRUDupElimResolver::
DumpStmtStatistics(CDSString &to, CDSString &stmt, Lng32 num) const
{
	char buf[10];

	sprintf(buf, ": %d", num);

	to += CDSString("\t") + stmt + 
		  CDSString(buf) + CDSString(" invocations.\n");
}
#endif
