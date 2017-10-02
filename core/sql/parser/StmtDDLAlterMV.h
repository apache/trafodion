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
#ifndef STMTDDLALTERMV_H
#define STMTDDLALTERMV_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlterMV.h
 * Description:  class for parse node representing Alter MV statements
 *
 *
 * Created:      6/11/99
 * Language:     C++
 * Status:       $State: Exp $
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// Revision 1.0  1999/11/06 03:21:44
// Initial revision
//
//
//
// -----------------------------------------------------------------------

#include "ComSmallDefs.h"
#include "StmtDDLNode.h"
#include "BaseTypes.h"
#include "ElemDDLFileAttrMvAudit.h"
#include "ElemDDLFileAttrMVCommitEach.h"

class StmtDDLAlterMV;
class QualifiedName;

//----------------------------------------------------------------------------
// forward references
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
class StmtDDLAlterMV : public StmtDDLNode
{

public:
	enum AlterType { BT_ATTRIBUTES, REWRITE, MV_ATTRIBUTES, MVSTATUS, RENAME, ADD_IGNORE_CHANGES_LIST, REMOVE_IGNORE_CHANGES_LIST };

	StmtDDLAlterMV(QualifiedName & mvName, ComBoolean rewriteEnableStatus);
	StmtDDLAlterMV(QualifiedName & mvName, ElemDDLNode * fileAttributeList);
	StmtDDLAlterMV(QualifiedName & mvName, ComMVStatus mvStatus);
	StmtDDLAlterMV(QualifiedName & mvName, const NAString & newName,
			ComBoolean withDepedentObjects);
        StmtDDLAlterMV(QualifiedName & mvName, 
                       ElemDDLNode *ignoreChangesList,
                       AlterType alterType);


	virtual ~StmtDDLAlterMV() {}

	void synthesize();
	
	virtual StmtDDLAlterMV * castToStmtDDLAlterMV() {  return this; }
	ExprNode * bindNode(BindWA *bindWAPtr);

	virtual const NAString displayLabel1() const;
	virtual const NAString getText() const;
	
	// accessor
	ComBoolean getRewriteEnableStatus() {return rewriteEnableStatus_;}
	ElemDDLFileAttrClause * getAttributeClause() {return pFileAttrClause_;}	
	const NAString getMVName() const;
	UInt32 getNumOfAttributes() const {return numOfAttributes_;}
	NAString getAttributeListString() const {return attributesListString_;}


	const QualifiedName & getMVNameAsQualifiedName() const;
	QualifiedName & getMVNameAsQualifiedName() ;

	AlterType getAlterType() const { return alterType_;}


	NABoolean isMvAuditSpecified() const 
	{ 
		return (NULL != pMvAudit_) ? TRUE : FALSE; 
	}
	
	NABoolean isCommitEachSpecified() const
	{ 
		return (NULL != pCommitEach_) ? TRUE : FALSE; 
	}

	ComMvAuditType getMvAuditType() const
	{
		return pMvAudit_->getMvAuditType();
	}

	ULng32 getCommitEach() const
	{
		return pCommitEach_->getNRows();
	}

        ElemDDLCreateMVOneAttributeTableList *getIgnoreChangesList() const
        {
                return pIgnoreChangesList_;
        }
        
	const NAString & getNewName() const
	{
		return newName_;
	}

	ComBoolean isCascade() const
	{
		return isCascade_;
	}

private:

	void processFileAttributeClause(AlterType type);
	void checkFileAttribute(ElemDDLFileAttr * pFileAttr); 
	const NAString getAuditCompressString();
	const NAString getClearOnPurgeString();
        const NAString getCompressionTypeString();
	const NAString getMaxExtentsString();
	const NAString getLockOnRefreshString();
	const NAString getMvsAllowedString();


	AlterType				alterType_;
	QualifiedName			MVQualName_;
	ElemDDLFileAttrClause	*pFileAttrClause_;
	ElemDDLMVFileAttrClause	*pMVFileAttrClause_;

	ComBoolean			rewriteEnableStatus_;

	ElemDDLFileAttrAuditCompress	*pAuditCompress_;
	ElemDDLFileAttrClearOnPurge	*pClearOnPurge_;
        ElemDDLFileAttrCompression      *pCompressionType_;
        ElemDDLFileAttrMaxExtents       *pMaxExtents_;
	ElemDDLFileAttrMvAudit		*pMvAudit_;
	ElemDDLFileAttrMVCommitEach	*pCommitEach_;
	ElemDDLFileAttrLockOnRefresh	*pLockOnRefresh_;
	ElemDDLFileAttrMvsAllowed	*pMvsAllowed_;
        ElemDDLCreateMVOneAttributeTableList *pIgnoreChangesList_;

	UInt32				numOfAttributes_;
	ComBoolean				isFirstAttribute_;
	NAString				attributesListString_;

	NAString				newName_;
	ComBoolean				isCascade_;

}; // class StmtDDLAlterMV

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLAlterMV
// -----------------------------------------------------------------------

inline const NAString StmtDDLAlterMV::displayLabel1() const
{  
	return NAString("Materialized View name: ") + getMVName();  
}
  
inline const NAString StmtDDLAlterMV::getText() const
{
	return "StmtDDLAlterMV";      
}

inline const NAString StmtDDLAlterMV::getMVName() const
{
  return MVQualName_.getQualifiedNameAsAnsiString();
}

/*
inline QualifiedName  &
StmtDDLAlterMV::getMVNameAsQualifiedName()
{
  return MVQualName_;
}

inline const QualifiedName &
StmtDDLAlterMV::getMVNameAsQualifiedName() const
{
  return MVQualName_;
}

inline const NAString 
StmtDDLAlterMV::getMVName() const
{
  return MVQualName_.getQualifiedNameAsAnsiString();
}
*/

#endif // STMTDDLALTERMV_H
