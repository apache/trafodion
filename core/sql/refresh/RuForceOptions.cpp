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
/*
******************************************************************************
*
* File:         ForceOptions.cpp
* Description:  
*				
* Created:      12/16/2001
* Language:     C++
*
*
******************************************************************************
*/
// Updated 2/3/02

#include "dsstring.h"
#include"RuForceOptions.h"


CRUForceOptions::~CRUForceOptions()
{
	// Remove the remaining un-refered force options.
	DSListPosition pos = mvsList_.GetHeadPosition();
	while (NULL != pos)
	{
		CRUMVForceOptions *pMVForceOptions = mvsList_.GetNext(pos);
		delete pMVForceOptions;
	}
}

//------------------------------------------------------
// CForceOptions::AddMV
//------------------------------------------------------
void CRUForceOptions::AddMV(CRUMVForceOptions* aMV) {
	mvsList_.AddTail(aMV);
}

//------------------------------------------------------
// CRUForceOptions::IsMVNameExist
//------------------------------------------------------

BOOL CRUForceOptions::IsMVExist(const CDSString& mvName) const
{
	DSListPosition pos = mvsList_.GetHeadPosition();
	while (NULL != pos) 
	{
		CRUMVForceOptions* aMV = mvsList_.GetNext(pos);
		if (mvName == aMV->GetMVName())
		{
			return TRUE;
		}
	}
	return FALSE;
}

//------------------------------------------------------
// CRUTableForceOptions constructor and destructors
//------------------------------------------------------

CRUTableForceOptions::CRUTableForceOptions(const CDSString& name)
{
	tableName_= name;
}

CRUTableForceOptions::CRUTableForceOptions(const CRUTableForceOptions& srcTbl)
{
	tableName_ = srcTbl.GetFullName();
	mdam_ = srcTbl.GetMdamOptions();
}

//------------------------------------------------------
// CRUTableForceOptions::SetTableName
//------------------------------------------------------
void CRUTableForceOptions::SetTableName(const CDSString& objectName)
{
	tableName_=objectName;
}

//------------------------------------------------------
// CRUMVForceOptions constructors and destructors
//------------------------------------------------------
CRUMVForceOptions::CRUMVForceOptions() :
	groupBy_(CRUForceOptions::GB_NO_FORCE),
	join_(CRUForceOptions::JOIN_NO_FORCE),
	mdam_(CRUForceOptions::MDAM_NO_FORCE),
	usedTableStarOption_(CRUForceOptions::MDAM_NO_FORCE),
	pTablesList_(new CRUTableForceOptionsList()),
	explain_(CRUForceOptions::EXPLAIN_OFF)
{}

CRUMVForceOptions::CRUMVForceOptions(const CDSString& name) :
	mvName_(name),
	groupBy_(CRUForceOptions::GB_NO_FORCE),
	join_(CRUForceOptions::JOIN_NO_FORCE),
	mdam_(CRUForceOptions::MDAM_NO_FORCE),
	usedTableStarOption_(CRUForceOptions::MDAM_NO_FORCE),
	pTablesList_(new CRUTableForceOptionsList()),
	explain_(CRUForceOptions::EXPLAIN_OFF)
{}
/*
CRUMVForceOptions::CRUMVForceOptions(const CRUMVForceOptions& srcMv) :
	mvName_(srcMv.GetMVName()),
	groupBy_(srcMv.GetGroupByoption()),
	join_(srcMv.GetJoinoption()),
	mdam_(srcMv.GetMDAMoption()),
	usedTableStarOption_(srcMv.GetTableStarOption()),
	pTablesList_(new CRUTableForceOptionsList(srcMv.GetTableForceList())),
	explain_(srcMv.GetExplainOption()),
	cqsStmt_(srcMv.GetCQSStatment())

{}
*/

CRUMVForceOptions::~CRUMVForceOptions()
{
	delete pTablesList_;
};

//------------------------------------------------------
// CRUMVForceOptions::AddTable
//------------------------------------------------------
void CRUMVForceOptions::AddTable(CRUTableForceOptions* aTable)
{
	pTablesList_->AddTail(aTable);	
}

//------------------------------------------------------
// CRUMVForceOptions::GetForceMdamOptionForTable
//------------------------------------------------------

CRUForceOptions::MdamOptions CRUMVForceOptions::GetForceMdamOptionForTable(const CDSString& tableName) const
{
	DSListPosition pos = pTablesList_->GetHeadPosition();
	while (NULL != pos) 
	{
		CRUTableForceOptions* aTable = pTablesList_-> GetNext(pos);
		if (tableName == aTable->GetFullName())
		{
			return aTable->GetMdamOptions();
		}
	}
	return CRUForceOptions::MDAM_NO_FORCE;
}



