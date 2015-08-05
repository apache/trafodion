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
* File:         RuSQLStaticStatementContainer.cpp
* Description:  Implementation of class CRUSQLStaticStatementContainer
*				
*
* Created:      09/08/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuSQLStaticStatementContainer.h"
#include "dmconnection.h"
#include "RuException.h"

//--------------------------------------------------------------------------//
//	Constructor and destructor
//--------------------------------------------------------------------------//

CRUSQLStaticStatementContainer::CRUSQLStaticStatementContainer(short nStmts)
:	CRUSQLStatementContainer(nStmts),
	pStaticStmtVec_(new CRUSQLStaticStatementContainer::StaticStmt[nStmts])
{}

CRUSQLStaticStatementContainer::~CRUSQLStaticStatementContainer()
{
	delete [] pStaticStmtVec_;
}

//--------------------------------------------------------------------------//
//	CRUSQLStaticStatementContainer::GetPreparedStatement()
//--------------------------------------------------------------------------//

CDMPreparedStatement *CRUSQLStaticStatementContainer::StaticStmt::
	GetPreparedStatement(BOOL DeleteUsedStmt)
{
	RUASSERT(pInfo_ != NULL)

	if (NULL == inherited::GetPreparedStatement())
	{
		CDMConnection *pConnect = GetConnection();
		pConnect->SetAllowSpecialSyntax(TRUE);

		// just loading the module
		CDMPreparedStatement *pPrepStmt =
				pConnect->PrepareStatement(
                                schemaVersion_,
				pInfo_,
				// The connection does NOT own the statement
				CDMConnection::eItemIsOwned
				);

		inherited::SetPreparedStatement(pPrepStmt, DeleteUsedStmt);

	}
	return  inherited::GetPreparedStatement();

}


