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
* File:         RuMultiTxnContext.cpp
* Description:  Implementation of class CRUMultiTxnContext.
*				
*
* Created:      08/17/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "RuMultiTxnContext.h"
#include "dmresultset.h"

//--------------------------------------------------------------------------//
//	PUBLIC METHODS
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//	CRUMultiTxnContext::ReadRowsFromContextLog()
//  This function fills the stack with context from the UMD
//  table 
//--------------------------------------------------------------------------//

void CRUMultiTxnContext::ReadRowsFromContextLog(CDMPreparedStatement *readStmt)
{
	RUASSERT(NULL != readStmt);
 
	try
	{
		// The rows are sorted by BEGIN_EPOCH asc

		CDMResultSet *pResult = readStmt->ExecuteQuery();
		
		while (pResult->Next()) 
		{
			const Int32 kEpoch  = 1;
			
			stack_.AddHead(pResult->GetInt(kEpoch));
		}
	}
	catch (CDSException &ex)
	{
		ex.SetError(IDS_RU_FETCHCTX_FAILED);
		throw ex;	// Re-throw
	}

	readStmt->Close();
}

//--------------------------------------------------------------------------//
//	CRUMultiTxnContext::GetRowByIndex()
//
// This function return ROW_DOES_NOT_EXIST if no such row exists
//--------------------------------------------------------------------------//

TInt32 CRUMultiTxnContext::GetRowByIndex(Lng32 index) 
{
	RUASSERT(0 <= index);

	if ((stack_.GetCount() <= index))
	{
		return ROW_DOES_NOT_EXIST;
	}
	
	DSListPosition pos = stack_.FindIndex(index);

	return stack_.GetAt(pos);
}
