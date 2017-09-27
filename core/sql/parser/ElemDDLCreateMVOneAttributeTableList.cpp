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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLCreateMVOneAttributeTableList.cpp
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

#include "ElemDDLCreateMVOneAttributeTableList.h"


ElemDDLCreateMVOneAttributeTableList::ElemDDLCreateMVOneAttributeTableList
								(ComMVSUsedTableAttribute type, ElemDDLNode *pTableList)
					: ElemDDLNode(ELM_CREATE_MV_ONE_ATTRIBUTE_TABLE_LIST), 
					type_(type), 
					pTableList_(pTableList)
{
}

QualifiedName & 
ElemDDLCreateMVOneAttributeTableList::getFirstTableInList()
{
	listIndex_ = 0;
	return getNextTableInList();
}
ComBoolean	
ElemDDLCreateMVOneAttributeTableList::listHasMoreTables() const
{
	return (listIndex_ <  pTableList_->entries()) ? TRUE : FALSE ;
}

QualifiedName & 
ElemDDLCreateMVOneAttributeTableList::getNextTableInList()
{
	ComASSERT(TRUE == listHasMoreTables());	

	return ((*pTableList_)[listIndex_++]->
	castToElemDDLQualName())->getQualifiedName();
}

ComMVSUsedTableAttribute 
ElemDDLCreateMVOneAttributeTableList::getType() const
{
	return type_;
}


// methods for tracing
const NAString 
ElemDDLCreateMVOneAttributeTableList::displayLabel1() const
{
	switch(type_)
	{
	case COM_IGNORE_CHANGES:
		return NAString("IGNORE CAHNGES ON");
	case COM_INSERT_ONLY:
		return NAString("INSERT ONLY ARE");
	}

	return "UNKNOWN TYPE";
}

const NAString 
ElemDDLCreateMVOneAttributeTableList::getText() const
{
	return "ElemDDLCreateMVOneAttributeTableList";
}



