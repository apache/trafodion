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
* File:         RuSQLDynamicStatementContainer.cpp
* Description:  Implementation of class CRUSQLDynamicStatementContainer
*				
*
* Created:      09/08/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuSQLDynamicStatementContainer.h"
#include "dmconnection.h"
#include "RuException.h"
#include "RuGlobals.h"
#include "RuJournal.h"
#include "uofsIpcMessageTranslator.h"
#include "uosessioninfo.h"

// For Charset: get system ISO_MAPPING value
#include "ComRtUtils.h"

//--------------------------------------------------------------------------//
//	Static member initialization
//--------------------------------------------------------------------------//

// ### SAP POC ### 11/21/2008 ### BEGIN
// Per request from SAP POC, we now allow the forward slash in delimited names.
// We can no longer rely on a single / character to separate compiled parameters.
// ### const char* const CRUSQLDynamicStatementContainer::COMPILED_PARAM_TOKEN = "/";
// Try using Ctrl-a (SOH - Start of Heading) character for now.
// Hopefully nobody use this character within delimited names.
const char* const CRUSQLDynamicStatementContainer::COMPILED_PARAM_TOKEN = "\001";
// ### SAP POC ### 11/21/2008 ### END

//--------------------------------------------------------------------------//
//	Constructors and destructors
//--------------------------------------------------------------------------//

CRUSQLDynamicStatementContainer::CRUSQLDynamicStatementContainer(short nStmts)
:	CRUSQLStatementContainer(nStmts), 
	pDynamicStmtVec_(new CRUSQLDynamicStatementContainer::DynamicStmt[nStmts])
{}

CRUSQLDynamicStatementContainer::~CRUSQLDynamicStatementContainer()
{
	delete [] pDynamicStmtVec_;
}


CRUSQLDynamicStatementContainer::DynamicStmt::DynamicStmt() : 
					sql_(NULL),
					prepared_(FALSE),
					paramNum_(0),
					paramSumStringSize_(0)
					{} ;
	
CRUSQLDynamicStatementContainer::DynamicStmt::~DynamicStmt() 
{
	if (sql_ != NULL)
		delete sql_;
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::PrepareSQL()
//
//	Prepare all the statements that have a non-empty text.
//	Allow special syntax in the compilation.
//--------------------------------------------------------------------------//

void CRUSQLDynamicStatementContainer::PrepareSQL()
{
	for (Int32 i=0; i < GetNumOfStmt(); i++)
	{
		PrepareStatement(i);
	}
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::DynamicStmt::GetLastSQL()
//--------------------------------------------------------------------------//

char const *
	CRUSQLDynamicStatementContainer::DynamicStmt::GetLastSQL()
{
	if (NULL != GetPreparedStatement() )
	{
		return  GetPreparedStatement()->GetSqlString();
	}
	else
	{
		return sql_;
	}
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::DynamicStmt::SetStatementText()
//--------------------------------------------------------------------------//

void CRUSQLDynamicStatementContainer::DynamicStmt::
	SetStatementText(const CDSString &sql)
{
	sql_ = new char[strlen(sql) + 1];
	strcpy(sql_, sql.c_string());
	AnalyzeSql();
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::GetPreparedStatement()
//--------------------------------------------------------------------------//

CDMPreparedStatement *CRUSQLDynamicStatementContainer::DynamicStmt::
	GetPreparedStatement(BOOL DeleteUsedStmt) 
{
	if (FALSE == prepared_)
	{
		PrepareStatement(DeleteUsedStmt);
	}

	return inherited::GetPreparedStatement();
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::DynamicStmt::SetCompiledTimeParam()
//--------------------------------------------------------------------------//

void CRUSQLDynamicStatementContainer::DynamicStmt::
	SetCompiledTimeParam(short paramIndex,const CDSString &value)
{
	RUASSERT(0 <= paramIndex && paramIndex < paramNum_);
	
	params_[paramIndex] = value;
	
	// needed for estimating the length of the prepared for compilation
	// statement
	paramSumStringSize_ +=     value.GetLength()
							- params_[paramIndex].GetLength();

	RUASSERT(strlen(sql_) + paramSumStringSize_ < 
			 CUOFsIpcMessageTranslator::MaxMsgSize
			);

	// The statement must be recompiled
	prepared_ = FALSE;
}


//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::DynamicStmt::AnalyzeSql()
//
// Find outs how many compile parameters are in the statement and their 
// position
//--------------------------------------------------------------------------//

void CRUSQLDynamicStatementContainer::DynamicStmt::AnalyzeSql()
{
	char *startSearchptr = sql_;

	while (1)
	{
		// ### SAP POC ### 11/21/2008 ### BEGIN
		// Per request from SAP POC, we now allow the forward slash in delimited names.
		// We can no longer rely on a single / character to separate compiled parameters.
		// Try using the Ctrl-a character ('\001') as the separator.
		// Hopefully nobody use this character within delimited names.
		// ### SAP POC ### 11/21/2008 ### END

		char *ptr = strchr(startSearchptr, *COMPILED_PARAM_TOKEN );

		if ( NULL == ptr )
		{
			break;
		}

                paramsPos_[paramNum_] = ptr - startSearchptr;

		// ### SAP POC ### 11/21/2008 ### BEGIN
		// Assume COMPILED_PARAM_TOKEN is 1 byte long.
		// ### SAP POC ### 11/21/2008 ### END
		startSearchptr = ptr + 1;
		paramNum_++;
	}
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::DynamicStmt::PrepareSqlText()
//
// Prepare the sql text for compilation by inserting all compiledtime params
// into the sql text
//--------------------------------------------------------------------------//

void CRUSQLDynamicStatementContainer::DynamicStmt::
	PrepareSqlText(char *buffer)
{
	buffer[0] = '\0';

	// always point to the next char to copy from the sql text
	Int32 pos = 0;

	for(Int32 i=0;i < paramNum_;i++)
	{
	   strncat(buffer,&sql_[pos],paramsPos_[i]);
	   
	   pos += paramsPos_[i] + 1;
	   
	   strcat(buffer,params_[i]);
	}
	
	// add the final string
	strcat(buffer,&sql_[pos]);

#ifdef _DEBUG
	// write to log the sql text
	CDSString msg;
	msg = " Compiling Statement : \n ";
	msg += buffer;

	CRUGlobals::GetInstance()->
		LogDebugMessage(CRUGlobals::DUMP_COMPILED_DYNAMIC_SQL,"",msg);
#endif
	
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::DynamicStmt::PrepareStatement()
//--------------------------------------------------------------------------//

void CRUSQLDynamicStatementContainer::DynamicStmt::PrepareStatement(BOOL DeleteUsedStmt)
{
	if (NULL == sql_)
	{
		return;
	}

	char sqlForCompilation[MAX_SQL_TEXT_SIZE];	
	
	// Prepare the sql text for compilation by inserting all compiled params
	// into the sql text
	PrepareSqlText(sqlForCompilation);

	// We compile the statement in nil transaction state,in such case
	// the compiler starts his own transaction and commits in the end
	// This way the compiler locks are freed as soon as possible
	CUOFsTransManager &transManager = 
		CRUGlobals::GetInstance()->GetTransactionManager();
	// Remember the current txn for later use
	Lng32 transIdx = transManager.GetCurrentTrans();

	transManager.LeaveTransaction();

	// now we can prepare the statement by using DMOL objects;
        CUOSessionInfo sessionInfo(TRUE, FALSE,FALSE);
	CDMConnection *pConnect = inherited::GetConnection();
	pConnect->SetAllowSpecialSyntax(TRUE);
	pConnect->SetAllowServicesOpen(sessionInfo.BelongsToServicesRole());
	prepared_ = TRUE;

	CDMPreparedStatement *pPrepStmt = NULL;

	short retry_delay = 1000 ; // milliseconds.
	for (Int32 retry = 0; retry < 2; retry++)
	{
		retry_delay = retry_delay * (retry + 1);
		try 
		{
		  // need to add here the handling of compiled params
			/* --------------
			   Date: 1/21/08 
			   Charset: Use internal flag SQLCHARSETCODE_ISO_MAPPING
			   ---------------- */

		   /* --- pPrepStmt = 
				  pConnect->PrepareStatement(
				  sqlForCompilation,
				  NULL, // no statement name
				  // The connection does NOT own the statement
					  CDMConnection::eItemIsOwned
				  );
             --- */
#ifdef NA_NSK 
         Lng32 iso_cs = 0 ;
         SQLCHARSET_CODE mapCS = SQLCHARSETCODE_ISO88591;

		 iso_cs = ComRtGetIsoMappingEnum();
		 if (iso_cs == 1) 
			 mapCS = SQLCHARSETCODE_ISO88591;

		 else if (iso_cs == 10)
			 mapCS = SQLCHARSETCODE_SJIS ;

		 else if (iso_cs == 15)
             mapCS = SQLCHARSETCODE_UTF8 ;
		 else 
		 {    
			 CDSException e;
	         e.SetError(IDS_GET_ISO_MAPPING_FAILED);
			 
			 char precision_str[100];
			 sprintf(precision_str, "REFRESH_SQLCHARSETCODE: global define=%d", iso_cs);
		     e.AddArgument(precision_str);
	         throw e;
		 }
#endif // NA_NSK 

		   pPrepStmt = 
				  pConnect->PrepareStmtWithCharSet(
				  sqlForCompilation,
#ifdef NA_NSK 
				  mapCS,  // CLI Charset enum, defined in sqlcli.h
#else 
				  SQLCHARSETCODE_UTF8,
#endif
				  NULL, // no statement name
				  // The connection does NOT own the statement
					  CDMConnection::eItemIsOwned
				  );

		  break; // no retry needed, exit retry loop

		}
		catch (CDSException &ex)
		{
			// The purpose of this method call is to detect compilation errors 
			// that are originated from a temporary lock on the OBJECTS table 
			// (error 73) and execute retry. Due to the catalog error mechanism 
			// the projected error code is currently 1100.
			if (ex.IsErrorFoundAndRetryNeeded(-1100, retry_delay))
			{
				// error was found try again
				continue;
			}

			if (-1 != transIdx)
			{
				// go back to the previous txn
				transManager.SetCurrentTrans(transIdx);
			}
			
#ifdef _DEBUG
			ex.SetError(IDS_RU_DYNAMIC_COMPILATION_FAILED);
			CDSString sqlString(sqlForCompilation);
			sqlString.TrimRight();
			sqlString.TrimLeft();
			ex.AddArgument(sqlString);
#endif
			throw ex;	// Re-throw
		}
	}

	if (-1 != transIdx)
	{
		// go back to the previous txn
		transManager.SetCurrentTrans(transIdx);
	}

	inherited::SetPreparedStatement(pPrepStmt, DeleteUsedStmt);

}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::DynamicStmt::PrepareStatement()
//--------------------------------------------------------------------------//

void CRUSQLDynamicStatementContainer::DynamicStmt::DirectExecStatement()
{
	
	char sqlForCompilation[MAX_SQL_TEXT_SIZE];	
	
	// Prepare the sql text for compilation by inserting all compiled params
	// into the sql text
	PrepareSqlText(sqlForCompilation);

	CDMConnection *pConnect = GetConnection();
	pConnect->SetAllowSpecialSyntax(TRUE);
	CDMStatement  *pStmt = pConnect->CreateStatement();

    // Execute the DDL command 
        CUOSessionInfo sessionInfo(TRUE, FALSE, FALSE);
	pStmt->ExecuteUpdate(sqlForCompilation, TRUE, FALSE, NULL, sessionInfo.BelongsToServicesRole());
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::DynamicStmt::StoreData()
//--------------------------------------------------------------------------//

void CRUSQLDynamicStatementContainer::DynamicStmt::
StoreData(CUOFsIpcMessageTranslator &translator)
{
	inherited::StoreData(translator);

	short size;
	
	if (NULL == sql_)
	{
		size = 0;
		translator.WriteBlock(&size, sizeof(short));
		return;
	}

	size = strlen(sql_)+1;
	translator.WriteBlock(&size, sizeof(short));
	
	translator.WriteBlock(&paramNum_, sizeof(Lng32));

	for (Int32 i=0;i<paramNum_;i++)
	{
		translator.WriteBlock(&(paramsPos_[i]), sizeof(Int32));
	}
		
	translator.WriteBlock(sql_, size);
}

//--------------------------------------------------------------------------//
//	CRUSQLDynamicStatementContainer::DynamicStmt::LoadData()
//--------------------------------------------------------------------------//

void CRUSQLDynamicStatementContainer::DynamicStmt::
	LoadData(CUOFsIpcMessageTranslator &translator)
{
	inherited::LoadData(translator);

	short size;
	translator.ReadBlock(&size,sizeof(short));

	if (0 == size)
	{
		return;
	}

	translator.ReadBlock(&paramNum_, sizeof(Lng32));

	for (Int32 i=0;i<paramNum_;i++)
	{
		translator.ReadBlock(&(paramsPos_[i]), sizeof(Int32));
	}

	sql_ = new char[size];

	translator.ReadBlock(sql_,size);
}
