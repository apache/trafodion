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
#ifndef STMTDDLALTERMRVGROUP_H
#define STMTDDLALTERMRVGROUP_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterMvRGroup.h
 * Description:  class representing Create MV group Statement parser nodes
 *
 *
 * Created:      5/06/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#ifndef OZY_TEST
#define OZY_TEST
#endif

#include "ElemDDLNode.h"
#include "NAString.h"

#include "StmtDDLNode.h"
#include "ComSmallDefs.h"

class StmtDDLAlterMvRGroup;


class StmtDDLAlterMvRGroup : public StmtDDLNode
{

public:

	enum alterMvGroupType { ADD_ACTION, REMOVE_ACTION
	
#ifdef OZY_TEST
		
		, testRedefTimeStamp_action
		, testOpenBlownAway_action

#endif
	
	}; 

	StmtDDLAlterMvRGroup(const QualifiedName & mvRGroupName,
						alterMvGroupType action,
						ElemDDLNode * pMVList);

	virtual ~StmtDDLAlterMvRGroup();

	virtual StmtDDLAlterMvRGroup * castToStmtDDLAlterMvRGroup();


	inline const NAString getMvRGroupName() const;
	inline const QualifiedName & getMvRGroupNameAsQualifiedName() const;
	inline       QualifiedName & getMvRGroupNameAsQualifiedName() ;

	
	QualifiedName & getFirstMvInList();
	ComBoolean	listHasMoreMVs() const;
	QualifiedName & getNextMvInList();

	alterMvGroupType getAction() const;

	ExprNode * bindNode(BindWA * pBindWA);


	// methods for tracing
	virtual const NAString displayLabel1() const;
	virtual const NAString getText() const;


private:

	QualifiedName mvRGroupQualName_; // The syntax of an mv group name is
					// [ [ catalog-name . ] schema-name . ] mvrg-name

	ElemDDLNode				* pMVList_;
	const alterMvGroupType  action_;
	CollIndex				listIndex_;


}; // class StmtDDLAlterMvRGroup 

//----------------------------------------------------------------------------
inline const NAString 
StmtDDLAlterMvRGroup::getMvRGroupName() const
{
  return mvRGroupQualName_.getQualifiedNameAsAnsiString();
}


inline QualifiedName &
StmtDDLAlterMvRGroup::getMvRGroupNameAsQualifiedName()
{
  return mvRGroupQualName_;
}

inline const QualifiedName & 
StmtDDLAlterMvRGroup::getMvRGroupNameAsQualifiedName() const
{
  return mvRGroupQualName_;
}




#endif // STMTDDLALTERMRVGROUP_H
