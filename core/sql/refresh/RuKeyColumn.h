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
#ifndef _RU_KEY_COLUMN_H_
#define _RU_KEY_COLUMN_H_

/* -*-C++-*-
******************************************************************************
*
* File:         RuKeyColumn.h
* Description:  Definition of class CRUKeyColumn
*				
*
* Created:      06/27/2000
* Language:     C++
* 
*
* 
******************************************************************************
*/

#include "refresh.h"

#include "dsstring.h"
#include "dsptrlist.h"

#include "ddcolumn.h"
#include "ddkeycolumn.h"

//--------------------------------------------------------------------------//
//	CRUKeyColumn
//	
//	The CRUKeyColumn class combines the subset of properties of CDDColumn 
//	(name, type conversion (casting) string) and CDDKeyColumn 
//	(the STORE BY order).
//
//	This class is used by CRUDupElimSQLComposer for the SQL code generation.
//
//--------------------------------------------------------------------------//

class REFRESH_LIB_CLASS CRUKeyColumn {

public:	
	CRUKeyColumn(CDDColumn *pCol, CDDKeyColumn *pKeyCol) :
		name_(pCol->GetName()),
		typeStr_(pCol->GetTypeStr()),
		sortOrder_(pKeyCol->GetSortOrder()) 
		{}			

	virtual ~CRUKeyColumn() {}

public:
	const CDSString &GetName() const
	{
		return name_;
	}

	const CDSString &GetTypeString() const
	{
		return typeStr_;
	}

	CDDObject::ESortOrder GetSortOrder() const
	{
		return sortOrder_;
	}

private:
	CDSString name_;
	CDSString typeStr_;

	CDDObject::ESortOrder sortOrder_;
};

// Declare the CRUKeyColumnList class through this macro
DECLARE_PTRLIST(REFRESH_LIB_CLASS, CRUKeyColumn);

// Complete the destructor's definition
inline CRUKeyColumnList::~CRUKeyColumnList() {}

#endif
