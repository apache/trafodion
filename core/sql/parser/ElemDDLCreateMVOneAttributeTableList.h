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
**********************************************************************/
#ifndef ELEMDDL_CREATE_MV_ONE_ATTRIBUTE_TABLE_LIST_H
#define ELEMDDL_CREATE_MV_ONE_ATTRIBUTE_TABLE_LIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLCreateMVOneAttributeTableList.h
 * Description:  class representing Create MV changes clause
 *
 *
 * Created:      11/27/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h" 
#include "NAString.h"

#include "ComSmallDefs.h"
#include "ObjectNames.h"
#include "ComASSERT.h"
#include "ElemDDLQualName.h"

// ElemDDLCreateMVChangesClause

//----------------------------------------------------------------------------
class ElemDDLCreateMVOneAttributeTableList : public ElemDDLNode
{

public:

  /*
	enum listType {	IGNORE_CHNAGES_ON, 
					INSERTONLY_ARE, 
					UNKNOWN_CHANGES			};
*/
	ElemDDLCreateMVOneAttributeTableList( ComMVSUsedTableAttribute type, 
											ElemDDLNode *pTableList);

	virtual ~ElemDDLCreateMVOneAttributeTableList(){}

	virtual ElemDDLCreateMVOneAttributeTableList* 
										castToElemDDLCreateMVOneAttributeTableList()
	{
		return this;
	}

	QualifiedName & getFirstTableInList();
	ComBoolean	listHasMoreTables() const;
	QualifiedName & getNextTableInList();

	ComMVSUsedTableAttribute getType() const;
	ElemDDLNode* getTableList() { return pTableList_; }

	// methods for tracing
	virtual const NAString displayLabel1() const;
	virtual const NAString getText() const;


private:

	const ComMVSUsedTableAttribute		type_;
	ElemDDLNode				* pTableList_;
	CollIndex				listIndex_;


}; // class ElemDDLCreateMVOneAttributeTableList 

#endif // ELEMDDL_CREATE_MV_ONE_ATTRIBUTE_TABLE_LIST_H
