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
* File:         RuGlobals.cpp
* Description:  Implementation of class CRUGlobals
*
*
* Created:      08/20/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuGlobals.h"
#include "RuException.h"
#include "RuOptions.h"
#include "RuJournal.h"

#include "dsstring.h"
#include "dmconnection.h"
#include "dmprepstatement.h"
#include "dmresultset.h"
#include "uofsSystem.h"

CRUGlobals *CRUGlobals::pInstance_ = NULL;

//--------------------------------------------------------------------------//
//	Constructor
//--------------------------------------------------------------------------//

CRUGlobals::CRUGlobals(CRUOptions &options, 
					   CRUJournal &journal,
					   CUOFsTransManager &transManager) :
	options_(options),
	journal_(journal),
	transManager_(transManager),
        parentQid_(NULL)
{

    getCurrentParentQid();
}

CRUGlobals::~CRUGlobals()
{
  if (parentQid_ != NULL)
    delete parentQid_;
}
//--------------------------------------------------------------------------//
//	CRUGlobals::GetInstance()
//--------------------------------------------------------------------------//

CRUGlobals *CRUGlobals::GetInstance()
{
	RUASSERT(NULL != pInstance_);
	return pInstance_;
}

//--------------------------------------------------------------------------//
//	CRUGlobals::Init()
//--------------------------------------------------------------------------//

void CRUGlobals::Init(CRUOptions &options, 
					  CRUJournal &journal,
					  CUOFsTransManager &transManager)
{
	if (NULL != pInstance_)
	{
		delete pInstance_;
	}

	// Create a new singletone's instance
	pInstance_ = new CRUGlobals(options, journal,transManager);
}

//--------------------------------------------------------------------------//
//	CRUGlobals::Testpoint()
//
//	If the debug option equal to the testpoint id is found -
//	throw the CRUException object!
//--------------------------------------------------------------------------//

void CRUGlobals::Testpoint(Int32 testpointId, const CDSString &objName)
{
	CRUOptions::DebugOption *pDO = 
		options_.FindDebugOption(testpointId, objName);

	if (NULL != pDO)
	{
		CRUException ex;
		ex.SetError(IDS_RU_TESTPOINT);
		ex.AddArgument(testpointId);
		throw ex;
	}
}

//--------------------------------------------------------------------------//
//	CRUGlobals::TestpointSevere()
//
//	If the debug option equal to the testpoint id is found -
//	throw the 0 value!
//--------------------------------------------------------------------------//

void CRUGlobals::TestpointSevere(Int32 testpointId, const CDSString &objName)
{
	CRUOptions::DebugOption *pDO = 
		options_.FindDebugOption(testpointId, objName);

	if (NULL != pDO)
	{
		throw 0;
	}
}

//--------------------------------------------------------------------------//
//	CRUGlobals::GetCurrentTimestamp()
//--------------------------------------------------------------------------//

TInt64 CRUGlobals::GetCurrentTimestamp()
{
	return CUOFsSystem::GetCurrentSystemTime();
}

//--------------------------------------------------------------------------//
//	CRUGlobals::LogMessageWithTime()
//--------------------------------------------------------------------------//

void CRUGlobals::LogMessageWithTime(const char* msg)
{
	if (NULL == options_.FindDebugOption(DUMP_EXECUTION_TIMESTAMPS,"")) 
	{
		return; 
	}

	CDSString message(msg);
	CRUGlobals *pGlobals = CRUGlobals::GetInstance();
	pGlobals->GetJournal().SetTimePrint(TRUE);
	pGlobals->GetJournal().LogMessage(message);
	pGlobals->GetJournal().SetTimePrint(FALSE);
}

//--------------------------------------------------------------------------//
//	CRUGlobals::GetCurrentUser()
//
//  Get the current user name.
//  On NSK, use userId to get the username incase an alias is used.
//  
//--------------------------------------------------------------------------//

CDSString CRUGlobals::GetCurrentUser()
{
  

#ifdef NA_LINUX
  CDSString curUserName = GetCurrentUserName();
#endif

#ifdef NA_WINNT

        CDSString curUserName;
  CDSString dml = "SELECT CURRENT_USER FROM (VALUES(1)) X(A); ";

	CDMConnection connection;
	CDMPreparedStatement *pPrepStmt;
	CDMResultSet *pResult;

	pPrepStmt = connection.PrepareStatement(dml);
	pResult = pPrepStmt->ExecuteQuery();
	pResult->Next();

	// Retrieve the user's name
	enum { NameSize = 65 };

	char userName[NameSize];
	pResult->GetString(1, userName, NameSize);
	curUserName = userName;

	pPrepStmt->DeleteResultSet(pResult);
	connection.DeleteStatement(pPrepStmt); 

#endif 

	//return CDSString(curUserName);
        return curUserName;
}

//--------------------------------------------------------------------------//
//	CRUGlobals::LogDebugMessage()
//
//	Print the debug message, if the user switched the 
//	debug option for the corresponding message type.
//--------------------------------------------------------------------------//

void CRUGlobals::LogDebugMessage(Int32 testpointId,
								 const CDSString &objName,
								 const CDSString &msg,
								 BOOL printRowNum)
{
#ifdef _DEBUG
	CRUOptions::DebugOption *pDO = 
		options_.FindDebugOption(testpointId, objName);

	if (NULL != pDO)
	{
		CRUGlobals *pGlobals = CRUGlobals::GetInstance();
		pGlobals->GetJournal().LogMessage(msg, printRowNum);
	}
#endif
}
#define MAX_QUERY_ID_LEN   160 // Same as ComSqlId::MAX_QUERY_ID_LEN     
void CRUGlobals::getCurrentParentQid()
{
  if (parentQid_ != NULL)
    delete parentQid_;
  Lng32 len;
  
  parentQid_ = new char[MAX_QUERY_ID_LEN+1];
  CDMConnection::getParentQid(parentQid_, MAX_QUERY_ID_LEN, &len);
  parentQid_[len] = '\0';
  if (len == 0)
  {
    delete parentQid_;
    parentQid_ = NULL;
  }
}

void CRUGlobals::setParentQidAtSession(char *parentQid)
{
  CDMConnection conn;
  CDSString src;
  if (parentQid != NULL)
    src = "SET SESSION DEFAULT PARENT_QID '" + CDSString(parentQid) + "'";
  else
    src = "SET SESSION DEFAULT PARENT_QID 'NONE'";
  CDMStatement *pStmt;
  pStmt = conn.CreateStatement();
  pStmt->ExecuteUpdate(src);
  conn.DeleteStatement(pStmt);
}
