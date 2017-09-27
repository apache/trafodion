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
* File:         CRUTestTaskExecutor.cpp
* Description:  Implementation of class CRUTestTaskExecutor
*
*
* Created:      01/09/2001
* Language:     C++
* 
*
* 
******************************************************************************
*/


#include "RuTestTaskExecutor.h"
#include "RuException.h"
#include "dmresultset.h"
#include "RuSQLComposer.h"
#include "uofsIpcMessageTranslator.h"


//--------------------------------------------------------------------------//
//	CRUTestTaskExecutor::Work()
//--------------------------------------------------------------------------//
void CRUTestTaskExecutor::Work()
{
	switch (GetState())
	{
	case EX_READ_GROUP: // REMOTE PROCESS
		{
			ReadSqlStatement();

			SetState(EX_COMPILE_ALL);
			break;
		}
	case EX_COMPILE_ALL: // REMOTE PROCESS
		{
			RUASSERT(NULL != pDynamicSQLContainer_);

			pDynamicSQLContainer_->PrepareSQL();

			SetState(EX_AFTER_COMPILATION_BEFORE_EXECUTION);
			break;
		}
	case EX_AFTER_COMPILATION_BEFORE_EXECUTION: // MAIN PROCESS
		{
			// A dummy state ,used to syncronized all executors before 
			// executing statements
			SetState(EX_EXECUTE);
			break;
		}
	case EX_EXECUTE: // REMOTE PROCESS
		{
			ExecuteAllStatements();
			SetState(EX_COMPLETE);
			break;
		}	
	default: RUASSERT(FALSE);
	}
}

//--------------------------------------------------------------------------//
//	CRUTestTaskExecutor::ReadSqlStatement()
//--------------------------------------------------------------------------//
void CRUTestTaskExecutor::ReadSqlStatement()
{
	pDynamicSQLContainer_ = 
#pragma nowarn(1506)   // warning elimination 
		new CRUSQLDynamicStatementContainer(numberOfStatements_);
#pragma warn(1506)  // warning elimination 

	pNumberOfExecutions_ = new Int32[numberOfStatements_];
	pNumberOfRetries_	 = new Int32[numberOfStatements_];
	pAutoCommit_		 = new Int32[numberOfStatements_];
	pNumberOfFailures_   = new Int32[numberOfStatements_];

	CRUSQLDynamicStatementContainer controlDynamicContainer(1);

	CDSString sqlText;

	sqlText = "	SELECT number_of_executions,sql_text,ordinal,number_of_retries,auto_commit ";
	sqlText +="	FROM CAT1.MVSCHM.RefreshControlTable ";
	sqlText +="	WHERE group_id = ";
	sqlText +=  CRUSQLComposer::ComposeCastExpr("INT UNSIGNED");	
	sqlText +="	AND process_id = ";
	sqlText +=  CRUSQLComposer::ComposeCastExpr("INT UNSIGNED");
	sqlText +="	ORDER BY ordinal;";

	const Int32 kNumber_of_executions		= 1;
	const Int32 kSql_text					= 2;
	const Int32 kSort_num					= 3;
	const Int32 kRetries					= 4;
	const Int32 kAutoCommit				= 5;

	controlDynamicContainer.SetStatementText(0,sqlText);

	CDMPreparedStatement *pStmt =
		controlDynamicContainer.GetPreparedStatement(0);

	pStmt->SetInt(1,groupId_);
	pStmt->SetInt(2,GetProcessId() + 1);

	
	BeginTransaction();
	
	CDMResultSet *pResult = pStmt->ExecuteQuery();

	Int32 i=0;
	while (pResult->Next()) 
	{
		RUASSERT(i < numberOfStatements_);

		char buffer[SQL_TEXT_MAX_SIZE];

		pNumberOfExecutions_[i] = pResult->GetInt(kNumber_of_executions);
		
		pNumberOfRetries_[i] = pResult->GetInt(kRetries);

		pAutoCommit_[i] = pResult->GetInt(kAutoCommit);

		pNumberOfFailures_[i] = 0;

		pResult->GetString(kSql_text, buffer, SQL_TEXT_MAX_SIZE);

		CDSString text(buffer);

		text.TrimLeft();
		
#pragma nowarn(1506)   // warning elimination 
		pDynamicSQLContainer_->SetStatementText(i,text);
#pragma warn(1506)  // warning elimination 

		i++;		
	}

	pStmt->Close();

	CommitTransaction();

	// Error Handling
	sqlText = "INSERT INTO RefreshOutputTable VALUES(?,?,?,?,?,?,?)";

	errorDynamicSQLContainer_.SetStatementText(0,sqlText);
}


//--------------------------------------------------------------------------//
//	CRUTestTaskExecutor::ExecuteAllStatements()
//--------------------------------------------------------------------------//
void CRUTestTaskExecutor::ExecuteAllStatements()
{
	Int32 i=0;
	for (Int32 j=0;j<pNumberOfRetries_[0];j++)
	{
		try 
		{
			BeginTransaction();

			BOOL more = TRUE;
			while (more)
			{
				more = FALSE;
				for(i=0;i<numberOfStatements_;i++)
				{
					if (0 < pNumberOfExecutions_[i])
					{
						more = TRUE;
						ExecuteStatement(i);
						pNumberOfFailures_[i] = 0;
						if (pAutoCommit_[i] != 0 && TRUE == IsTransactionOpen())
						{
							CommitTransaction();
							BeginTransaction();
						}
						pNumberOfExecutions_[i]--;
					}
				}
			}

			CommitTransaction();
		}
		catch (CDMException &e)
		{
			
			HandleError(groupId_,GetProcessId(),i,e);

			if (e.GetErrorCode(0) != -8551)
			{
				throw e;
			}
			
			CDMPreparedStatement *pStmt =
#pragma nowarn(1506)   // warning elimination 
			pDynamicSQLContainer_->GetPreparedStatement(i);
#pragma warn(1506)  // warning elimination 

			pStmt->Close();
			continue;
		}
	}

}

//--------------------------------------------------------------------------//
//	CRUTestTaskExecutor::ExecuteStatement()
//--------------------------------------------------------------------------//
void CRUTestTaskExecutor::ExecuteStatement(Int32 i)
{
	CDMPreparedStatement *pStmt =
#pragma nowarn(1506)   // warning elimination 
		pDynamicSQLContainer_->GetPreparedStatement(i);
#pragma warn(1506)  // warning elimination 

#pragma nowarn(1506)   // warning elimination 
	if (pDynamicSQLContainer_->GetLastSQL(i)[0] == 'S' ||
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
		pDynamicSQLContainer_->GetLastSQL(i)[0] == 's' )
#pragma warn(1506)  // warning elimination 
	{
//		Sleep(10);

		CDMResultSet *pResult = pStmt->ExecuteQuery();		
		
		while (pResult->Next()) {}
		
		pStmt->Close();
	}
	else
	{
#pragma nowarn(1506)   // warning elimination 
		if (pDynamicSQLContainer_->GetLastSQL(i)[0] == 'R' ||
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
			pDynamicSQLContainer_->GetLastSQL(i)[0] == 'r' )
#pragma warn(1506)  // warning elimination 
		{
			if (TRUE == IsTransactionOpen())
			{
				CommitTransaction();
			}
			
//			Sleep(100);

			pStmt->ExecuteUpdate();
		
			pStmt->Close();
			
			BeginTransaction();
		}
		else
		{
//			Sleep(1);

			pStmt->ExecuteUpdate();
		
			pStmt->Close();
		}
	}
}


//----------------------------------------------------------//
//	CRUTestTaskExecutor::HandleError()
//----------------------------------------------------------//

void CRUTestTaskExecutor::HandleError(Int32 groupId ,
									  Int32 processId,
									  Int32 ordinal,
									  CDMException &e)
{
	pNumberOfFailures_[ordinal]++;

	try {
		if (TRUE == IsTransactionOpen())
		{
			CommitTransaction();
		}
	}
	catch(...)
	{}

	BeginTransaction();

	CDMPreparedStatement *pStmt =
		errorDynamicSQLContainer_.GetPreparedStatement(0);
	
	
	Lng32 numErrors = e.GetNumErrors();
		
	for (Int32 index = 0; index < numErrors; index++) {
		char errorMsg[1024];

		pStmt->SetInt(1,groupId);
		pStmt->SetInt(2,processId + 1);
		pStmt->SetInt(3,ordinal + 1);
		pStmt->SetInt(4,pNumberOfFailures_[ordinal]);
		pStmt->SetInt(5,index);
		pStmt->SetInt(6,e.GetErrorCode(index));
		e.GetErrorMsg(index, errorMsg, 1024);
		pStmt->SetString(7,errorMsg);

		pStmt->ExecuteUpdate();
	}	
	

	pStmt->Close();

	CommitTransaction();
}

//----------------------------------------------------------//
//	CRUTestTaskExecutor::StoreData()
//----------------------------------------------------------//

void CRUTestTaskExecutor::
	StoreData(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreData(translator);

	translator.WriteBlock(&numberOfStatements_,sizeof(Int32));
	translator.WriteBlock(&groupId_,sizeof(Int32));
	translator.SetMessageType(CUOFsIpcMessageTranslator::
								RU_TEST_EXECUTOR);	
}

//----------------------------------------------------------//
//	CRUTestTaskExecutor::LoadData()
//----------------------------------------------------------//

void CRUTestTaskExecutor::
	LoadData(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadData(translator);

	translator.ReadBlock(&numberOfStatements_,sizeof(Int32));
	translator.ReadBlock(&groupId_,sizeof(Int32));
}

