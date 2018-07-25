/* -*-C++-*- */
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
 * File:         ElemDDLNode.C
 * Description:  definitions of methods for classes representing
 *               elements in DDL statements, excluding methods
 *               for classes representing lists (left-skewed
 *               binary trees), file attributes, constraints,
 *               like options, partitions, elements in like
 *               clauses, load options, privilege actions, etc.
 *
 *
 * Created:      3/9/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#define   SQLPARSERGLOBALS_FLAGS        // must precede all #include's
#include "BaseTypes.h"
#include "ComASSERT.h"
#include "ComDiags.h"
#include "ComOperators.h"
#include "ComLocationNames.h"
#include "ElemDDLAlterTableMove.h"
#include "ElemDDLAuthSchema.h"
#include "ElemDDLLibClientFilename.h"
#include "ElemDDLLibClientName.h"
#include "ElemDDLLibPathName.h"
#include "ElemDDLColNameList.h"
#include "ElemDDLGrantee.h"
#include "ElemDDLGranteeArray.h"
#include "ElemDDLKeyValue.h"
#include "ElemDDLLocation.h"
#include "ElemDDLParallelExec.h"
#include "ElemDDLReferences.h"
#include "ElemDDLSchemaName.h"
#include "ElemDDLPrivActions.h"
#include "ElemDDLPrivileges.h"
#include "ElemDDLWithCheckOption.h"
#include "ElemDDLWithGrantOption.h"
#include "ElemDDLIndexPopulateOption.h"
#include "ElemDDLQualName.h" // MV - RG
#include "ElemDDLLoggable.h"
#include "ElemDDLLobAttrs.h"
#include "ElemDDLDivisionClause.h"
#include "ElemDDLSaltOptions.h"
#include "ElemDDLTableFeature.h"
#include "ElemDDLHbaseOptions.h"
#include "ItemExpr.h"
#include "ItemColRef.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"

#include "OptimizerSimulator.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLNode
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLNode::~ElemDDLNode()
{
}

// cast virtual functions

ElemDDLNode *
ElemDDLNode::castToElemDDLNode()
{
  return this;
}

const ElemDDLNode *
ElemDDLNode::castToElemDDLNode() const
{
  return this;
}

ElemDDLQualName *
ElemDDLNode::castToElemDDLQualName() // MV - RG
{
  return NULL;
}


ElemDDLIndexPopulateOption *
ElemDDLNode::castToElemDDLIndexPopulateOption()
{
  return NULL;
}

ElemDDLAlterTableMove *
ElemDDLNode::castToElemDDLAlterTableMove()
{
  return NULL;
}

ElemDDLAuthSchema *
ElemDDLNode::castToElemDDLAuthSchema()
{
  return NULL;
}

ElemDDLColDef *
ElemDDLNode::castToElemDDLColDef()
{
  return NULL;
}

ElemProxyColDef *
ElemDDLNode::castToElemProxyColDef()
{
  return NULL;
}

ElemDDLColDefault *
ElemDDLNode::castToElemDDLColDefault()
{
  return NULL;
}

ElemDDLColHeading *
ElemDDLNode::castToElemDDLColHeading()
{
  return NULL;
}

ElemDDLColName *
ElemDDLNode::castToElemDDLColName()
{
  return NULL;
}

ElemDDLColNameList *
ElemDDLNode::castToElemDDLColNameList()
{
  return NULL;
}

ElemDDLColNameListNode *
ElemDDLNode::castToElemDDLColNameListNode()
{
  return NULL;
}

ElemDDLColRef *
ElemDDLNode::castToElemDDLColRef()
{
  return NULL;
}

ElemDDLColRefList *
ElemDDLNode::castToElemDDLColRefList()
{
  return NULL;
}

ElemDDLColViewDef *
ElemDDLNode::castToElemDDLColViewDef()
{
  return NULL;
}

ElemDDLConstraint *
ElemDDLNode::castToElemDDLConstraint()
{
  return NULL;
}

ElemDDLConstraintAttr *
ElemDDLNode::castToElemDDLConstraintAttr()
{
  return NULL;
}

ElemDDLConstraintAttrDroppable *
ElemDDLNode::castToElemDDLConstraintAttrDroppable()
{
  return NULL;
}

ElemDDLConstraintAttrEnforced *
ElemDDLNode::castToElemDDLConstraintAttrEnforced()
{
  return NULL;
}

ElemDDLConstraintCheck *
ElemDDLNode::castToElemDDLConstraintCheck()
{
  return NULL;
}

ElemDDLConstraintName *
ElemDDLNode::castToElemDDLConstraintName()
{
  return NULL;
}

ElemDDLConstraintNameList *
ElemDDLNode::castToElemDDLConstraintNameList()
{
  return NULL;
}

ElemDDLConstraintNotNull *
ElemDDLNode::castToElemDDLConstraintNotNull()
{
  return NULL;
}

ElemDDLLoggable *
ElemDDLNode::castToElemDDLLoggable()
{
	return NULL;
}

ElemDDLLobAttrs *
ElemDDLNode::castToElemDDLLobAttrs()
{
	return NULL;
}

ElemDDLSeabaseSerialized *
ElemDDLNode::castToElemDDLSeabaseSerialized()
{
	return NULL;
}


ElemDDLConstraintPK *
ElemDDLNode::castToElemDDLConstraintPK()
{
  return NULL;
}

ElemDDLConstraintPKColumn *
ElemDDLNode::castToElemDDLConstraintPKColumn()
{
  return NULL;
}

ElemDDLConstraintRI *
ElemDDLNode::castToElemDDLConstraintRI()
{
  return NULL;
}

ElemDDLConstraintUnique *
ElemDDLNode::castToElemDDLConstraintUnique()
{
  return NULL;
}


ElemDDLCreateMVOneAttributeTableList  *
ElemDDLNode::castToElemDDLCreateMVOneAttributeTableList() // MV OZ
{
	return NULL;
}

ElemDDLDivisionClause *
ElemDDLNode::castToElemDDLDivisionClause()
{
  return NULL;
}

ElemDDLFileAttr *
ElemDDLNode::castToElemDDLFileAttr()
{
  return NULL;
}



ElemDDLFileAttrAllocate *
ElemDDLNode::castToElemDDLFileAttrAllocate()
{
  return NULL;
}

ElemDDLFileAttrAudit *
ElemDDLNode::castToElemDDLFileAttrAudit()
{
  return NULL;
}

ElemDDLFileAttrAuditCompress *
ElemDDLNode::castToElemDDLFileAttrAuditCompress()
{
  return NULL;
}

ElemDDLFileAttrBlockSize *
ElemDDLNode::castToElemDDLFileAttrBlockSize()
{
  return NULL;
}

ElemDDLFileAttrBuffered *
ElemDDLNode::castToElemDDLFileAttrBuffered()
{
  return NULL;
}

ElemDDLFileAttrClause *
ElemDDLNode::castToElemDDLFileAttrClause()
{
  return NULL;
}

ElemDDLFileAttrClearOnPurge *
ElemDDLNode::castToElemDDLFileAttrClearOnPurge()
{
  return NULL;
}

ElemDDLFileAttrCompression *
ElemDDLNode::castToElemDDLFileAttrCompression()
{
  return NULL;
}

ElemDDLFileAttrDCompress *
ElemDDLNode::castToElemDDLFileAttrDCompress()
{
  return NULL;
}

ElemDDLFileAttrDeallocate *
ElemDDLNode::castToElemDDLFileAttrDeallocate()
{
  return NULL;
}

ElemDDLFileAttrICompress *
ElemDDLNode::castToElemDDLFileAttrICompress()
{
  return NULL;
}

ElemDDLFileAttrList *
ElemDDLNode::castToElemDDLFileAttrList()
{
  return NULL;
}

ElemDDLPartnAttrList *
ElemDDLNode::castToElemDDLPartnAttrList()
{
  return NULL;
}

ElemDDLFileAttrMaxSize *
ElemDDLNode::castToElemDDLFileAttrMaxSize()
{
  return NULL;
}

ElemDDLFileAttrExtents *
ElemDDLNode::castToElemDDLFileAttrExtents()
{
  return NULL;
}

ElemDDLFileAttrMaxExtents *
ElemDDLNode::castToElemDDLFileAttrMaxExtents()
{
  return NULL;
}

ElemDDLFileAttrNoLabelUpdate *
ElemDDLNode::castToElemDDLFileAttrNoLabelUpdate()
{
  return NULL;
}

ElemDDLFileAttrOwner *
ElemDDLNode::castToElemDDLFileAttrOwner()
{
  return NULL;
}

ElemDDLFileAttrUID *
ElemDDLNode::castToElemDDLFileAttrUID()
{
  return NULL;
}

ElemDDLFileAttrRowFormat *
ElemDDLNode::castToElemDDLFileAttrRowFormat()
{
  return NULL;
}

ElemDDLFileAttrColFam *
ElemDDLNode::castToElemDDLFileAttrColFam()
{
  return NULL;
}

//++ MV
ElemDDLFileAttrRangeLog *
ElemDDLNode::castToElemDDLFileAttrRangeLog()
{
  return NULL;
}

ElemDDLFileAttrLockOnRefresh  *
ElemDDLNode::castToElemDDLFileAttrLockOnRefresh()
{
	return NULL;
}

ElemDDLFileAttrInsertLog *
ElemDDLNode::castToElemDDLFileAttrInsertLog()
{
	return NULL;
}

ElemDDLFileAttrMvsAllowed *
ElemDDLNode::castToElemDDLFileAttrMvsAllowed()
{
	return NULL;
}

ElemDDLMVFileAttrClause *
ElemDDLNode::castToElemDDLMVFileAttrClause()
{
	return NULL;
}

ElemDDLFileAttrMVCommitEach	*
ElemDDLNode::castToElemDDLFileAttrMVCommitEach()
{
	return NULL;
}

ElemDDLFileAttrMvAudit *
ElemDDLNode::castToElemDDLFileAttrMvAudit()
{
	return NULL;
}

//-- MV


ElemDDLGrantee *
ElemDDLNode::castToElemDDLGrantee()
{
  return NULL;
}

ElemDDLKeyValue *
ElemDDLNode::castToElemDDLKeyValue()
{
  return NULL;
}

ElemDDLKeyValueList *
ElemDDLNode::castToElemDDLKeyValueList()
{
  return NULL;
}

ElemDDLLibrary *
ElemDDLNode::castToElemDDLLibrary()
{
  return NULL;
}

ElemDDLLibClientFilename *
ElemDDLNode::castToElemDDLLibClientFilename()
{
  return NULL;
}

ElemDDLLibClientName *
ElemDDLNode::castToElemDDLLibClientName()
{
  return NULL;
}

ElemDDLLibPathName *
ElemDDLNode::castToElemDDLLibPathName()
{
  return NULL;
}

ElemDDLLike *
ElemDDLNode::castToElemDDLLike()
{
  return NULL;
}

ElemDDLLikeCreateTable *
ElemDDLNode::castToElemDDLLikeCreateTable()
{
  return NULL;
}

ElemDDLLikeOpt *
ElemDDLNode::castToElemDDLLikeOpt()
{
  return NULL;
}

ElemDDLLikeOptWithoutConstraints *
ElemDDLNode::castToElemDDLLikeOptWithoutConstraints()
{
  return NULL;
}

ElemDDLLikeOptWithHeadings *
ElemDDLNode::castToElemDDLLikeOptWithHeadings()
{
  return NULL;
}

ElemDDLLikeOptWithHorizontalPartitions *
ElemDDLNode::castToElemDDLLikeOptWithHorizontalPartitions()
{
  return NULL;
}

ElemDDLLikeOptWithoutSalt * ElemDDLNode::castToElemDDLLikeOptWithoutSalt()
{
  return NULL;
}

ElemDDLLikeSaltClause * ElemDDLNode::castToElemDDLLikeSaltClause()
{
  return NULL;
}

ElemDDLLikeOptWithoutDivision * ElemDDLNode::castToElemDDLLikeOptWithoutDivision()
{
  return NULL;
}

ElemDDLLikeLimitColumnLength * ElemDDLNode::castToElemDDLLikeLimitColumnLength()
{
  return NULL;
}

ElemDDLLikeOptWithoutRowFormat * ElemDDLNode::castToElemDDLLikeOptWithoutRowFormat()
{
  return NULL;
}

ElemDDLLikeOptWithoutLobColumns * ElemDDLNode::castToElemDDLLikeOptWithoutLobColumns()
{
  return NULL;
}

ElemDDLList *
ElemDDLNode::castToElemDDLList()
{
  return NULL;
}

ElemDDLLocation *
ElemDDLNode::castToElemDDLLocation()
{
  return NULL;
}

ElemDDLOptionList *
ElemDDLNode::castToElemDDLOptionList()
{
  return NULL;
}

ElemDDLParallelExec *
ElemDDLNode::castToElemDDLParallelExec()
{
  return NULL;
}

ElemDDLParamDef *
ElemDDLNode::castToElemDDLParamDef()
{
  return NULL;
}

ElemDDLPartition *
ElemDDLNode::castToElemDDLPartition()
{
  return NULL;
}

ElemDDLPartitionByOpt *
ElemDDLNode::castToElemDDLPartitionByOpt()
{
  return NULL;
}

ElemDDLPartitionByColumnList *
ElemDDLNode::castToElemDDLPartitionByColumnList()
{
  return NULL;
}

ElemDDLPartitionClause *
ElemDDLNode::castToElemDDLPartitionClause()
{
  return NULL;
}

ElemDDLPartitionList *
ElemDDLNode::castToElemDDLPartitionList()
{
  return NULL;
}

ElemDDLPartitionRange *
ElemDDLNode::castToElemDDLPartitionRange()
{
  return NULL;
}

ElemDDLPartitionSingle *
ElemDDLNode::castToElemDDLPartitionSingle()
{
  return NULL;
}

ElemDDLPartitionSystem *
ElemDDLNode::castToElemDDLPartitionSystem()
{
  return NULL;
}

ElemDDLPassThroughParamDef *
ElemDDLNode::castToElemDDLPassThroughParamDef()
{
  return NULL;
}

ElemDDLPrivAct *
ElemDDLNode::castToElemDDLPrivAct()
{
  return NULL;
}

ElemDDLPrivActAlter *
ElemDDLNode::castToElemDDLPrivActAlter()
{
  return NULL;
}

ElemDDLPrivActAlterLibrary *
ElemDDLNode::castToElemDDLPrivActAlterLibrary()
{
  return NULL;
}

ElemDDLPrivActAlterMV *
ElemDDLNode::castToElemDDLPrivActAlterMV()
{
  return NULL;
}

ElemDDLPrivActAlterMVGroup *
ElemDDLNode::castToElemDDLPrivActAlterMVGroup()
{
  return NULL;
}

ElemDDLPrivActAlterRoutine *
ElemDDLNode::castToElemDDLPrivActAlterRoutine()
{
  return NULL;
}

ElemDDLPrivActAlterRoutineAction *
ElemDDLNode::castToElemDDLPrivActAlterRoutineAction()
{
  return NULL;
}

ElemDDLPrivActAlterSynonym *
ElemDDLNode::castToElemDDLPrivActAlterSynonym()
{
  return NULL;
}

ElemDDLPrivActAlterTable *
ElemDDLNode::castToElemDDLPrivActAlterTable()
{
  return NULL;
}

ElemDDLPrivActAlterTrigger *
ElemDDLNode::castToElemDDLPrivActAlterTrigger()
{
  return NULL;
}

ElemDDLPrivActAlterView *
ElemDDLNode::castToElemDDLPrivActAlterView()
{
  return NULL;
}

ElemDDLPrivActCreate *
ElemDDLNode::castToElemDDLPrivActCreate()
{
  return NULL;
}

ElemDDLPrivActCreateLibrary *
ElemDDLNode::castToElemDDLPrivActCreateLibrary()
{
  return NULL;
}

ElemDDLPrivActCreateMV *
ElemDDLNode::castToElemDDLPrivActCreateMV()
{
  return NULL;
}

ElemDDLPrivActCreateMVGroup *
ElemDDLNode::castToElemDDLPrivActCreateMVGroup()
{
  return NULL;
}

ElemDDLPrivActCreateProcedure *
ElemDDLNode::castToElemDDLPrivActCreateProcedure()
{
  return NULL;
}

ElemDDLPrivActCreateRoutine *
ElemDDLNode::castToElemDDLPrivActCreateRoutine()
{
  return NULL;
}

ElemDDLPrivActCreateRoutineAction *
ElemDDLNode::castToElemDDLPrivActCreateRoutineAction()
{
  return NULL;
}

ElemDDLPrivActCreateSynonym *
ElemDDLNode::castToElemDDLPrivActCreateSynonym()
{
  return NULL;
}

ElemDDLPrivActCreateTable *
ElemDDLNode::castToElemDDLPrivActCreateTable()
{
  return NULL;
}

ElemDDLPrivActCreateTrigger *
ElemDDLNode::castToElemDDLPrivActCreateTrigger()
{
  return NULL;
}

ElemDDLPrivActCreateView *
ElemDDLNode::castToElemDDLPrivActCreateView()
{
  return NULL;
}

ElemDDLPrivActDBA *
ElemDDLNode::castToElemDDLPrivActDBA()
{
  return NULL;
}

ElemDDLPrivActDelete *
ElemDDLNode::castToElemDDLPrivActDelete()
{
  return NULL;
}

ElemDDLPrivActDrop *
ElemDDLNode::castToElemDDLPrivActDrop()
{
  return NULL;
}

ElemDDLPrivActDropLibrary *
ElemDDLNode::castToElemDDLPrivActDropLibrary()
{
  return NULL;
}

ElemDDLPrivActDropMV *
ElemDDLNode::castToElemDDLPrivActDropMV()
{
  return NULL;
}

ElemDDLPrivActDropMVGroup *
ElemDDLNode::castToElemDDLPrivActDropMVGroup()
{
  return NULL;
}

ElemDDLPrivActDropProcedure *
ElemDDLNode::castToElemDDLPrivActDropProcedure()
{
  return NULL;
}
ElemDDLPrivActDropRoutine *
ElemDDLNode::castToElemDDLPrivActDropRoutine()
{
  return NULL;
}

ElemDDLPrivActDropRoutineAction *
ElemDDLNode::castToElemDDLPrivActDropRoutineAction()
{
  return NULL;
}

ElemDDLPrivActDropSynonym *
ElemDDLNode::castToElemDDLPrivActDropSynonym()
{
  return NULL;
}

ElemDDLPrivActDropTable *
ElemDDLNode::castToElemDDLPrivActDropTable()
{
  return NULL;
}

ElemDDLPrivActDropTrigger *
ElemDDLNode::castToElemDDLPrivActDropTrigger()
{
  return NULL;
}

ElemDDLPrivActDropView *
ElemDDLNode::castToElemDDLPrivActDropView()
{
  return NULL;
}

ElemDDLPrivActInsert *
ElemDDLNode::castToElemDDLPrivActInsert()
{
  return NULL;
}

ElemDDLPrivActMaintain *
ElemDDLNode::castToElemDDLPrivActMaintain()
{
  return NULL;
}

ElemDDLPrivActReferences *
ElemDDLNode::castToElemDDLPrivActReferences()
{
  return NULL;
}

ElemDDLPrivActRefresh *
ElemDDLNode::castToElemDDLPrivActRefresh()
{
  return NULL;
}

ElemDDLPrivActReorg *
ElemDDLNode::castToElemDDLPrivActReorg()
{
  return NULL;
}

ElemDDLPrivActSelect *
ElemDDLNode::castToElemDDLPrivActSelect()
{
  return NULL;
}

ElemDDLPrivActTransform *
ElemDDLNode::castToElemDDLPrivActTransform()
{
  return NULL;
}

ElemDDLPrivActUpdate *
ElemDDLNode::castToElemDDLPrivActUpdate()
{
  return NULL;
}

ElemDDLPrivActUpdateStats *
ElemDDLNode::castToElemDDLPrivActUpdateStats()
{
  return NULL;
}

ElemDDLPrivActAllDDL *
ElemDDLNode::castToElemDDLPrivActAllDDL()
{
  return NULL;
}

ElemDDLPrivActAllDML *
ElemDDLNode::castToElemDDLPrivActAllDML()
{
  return NULL;
}

ElemDDLPrivActAllOther *
ElemDDLNode::castToElemDDLPrivActAllOther()
{
  return NULL;
}

ElemDDLPrivActUsage *
ElemDDLNode::castToElemDDLPrivActUsage()
{
  return NULL;
}

ElemDDLPrivActWithColumns *
ElemDDLNode::castToElemDDLPrivActWithColumns()
{
  return NULL;
}

ElemDDLPrivileges *
ElemDDLNode::castToElemDDLPrivileges()
{
  return NULL;
}

ElemDDLRefAct *
ElemDDLNode::castToElemDDLRefAct()
{
  return NULL;
}

ElemDDLRefActCascade *
ElemDDLNode::castToElemDDLRefActCascade()
{
  return NULL;
}

ElemDDLRefActNoAction *
ElemDDLNode::castToElemDDLRefActNoAction()
{
  return NULL;
}

ElemDDLRefActRestrict *
ElemDDLNode::castToElemDDLRefActRestrict()
{
  return NULL;
}

ElemDDLRefActSetDefault *
ElemDDLNode::castToElemDDLRefActSetDefault()
{
  return NULL;
}

ElemDDLRefActSetNull *
ElemDDLNode::castToElemDDLRefActSetNull()
{
  return NULL;
}

ElemDDLRefTrigAct *
ElemDDLNode::castToElemDDLRefTrigAct()
{
  return NULL;
}

ElemDDLRefTrigActDeleteRule *
ElemDDLNode::castToElemDDLRefTrigActDeleteRule()
{
  return NULL;
}

ElemDDLRefTrigActUpdateRule *
ElemDDLNode::castToElemDDLRefTrigActUpdateRule()
{
  return NULL;
}

ElemDDLReferences *
ElemDDLNode::castToElemDDLReferences()
{
  return NULL;
}

ElemDDLSaltOptionsClause * 
ElemDDLNode::castToElemDDLSaltOptionsClause()
{
  return NULL;
}

ElemDDLTableFeature *
ElemDDLNode::castToElemDDLTableFeature()
{
  return NULL;
}

ElemDDLHbaseOptions *
ElemDDLNode::castToElemDDLHbaseOptions()
{
  return NULL;
}

ElemDDLSchemaName *
ElemDDLNode::castToElemDDLSchemaName()
{
  return NULL;
}

ElemDDLSGOptions *
ElemDDLNode::castToElemDDLSGOptions()
{
  return NULL;
}

ElemDDLSGOption *
ElemDDLNode::castToElemDDLSGOption()
{
  return NULL;
}
 
ElemDDLSGOptionStartValue *
ElemDDLNode::castToElemDDLSGOptionStartValue()
{
  return NULL;
}
 
ElemDDLSGOptionMinValue *
ElemDDLNode::castToElemDDLSGOptionMinValue()
{
  return NULL;
}

ElemDDLSGOptionMaxValue *
ElemDDLNode::castToElemDDLSGOptionMaxValue()
{
  return NULL;
}

ElemDDLSGOptionIncrement *
ElemDDLNode::castToElemDDLSGOptionIncrement()
{
  return NULL;
}

ElemDDLSGOptionCycleOption *
ElemDDLNode::castToElemDDLSGOptionCycleOption()
{
  return NULL;
}
 
ElemDDLSGOptionCacheOption *
ElemDDLNode::castToElemDDLSGOptionCacheOption()
{
  return NULL;
}
 
ElemDDLSGOptionDatatype *
ElemDDLNode::castToElemDDLSGOptionDatatype()
{
  return NULL;
}
 
ElemDDLStoreOpt *
ElemDDLNode::castToElemDDLStoreOpt()
{
  return NULL;
}

ElemDDLStoreOptDefault *
ElemDDLNode::castToElemDDLStoreOptDefault()
{
  return NULL;
}

ElemDDLStoreOptEntryOrder *
ElemDDLNode::castToElemDDLStoreOptEntryOrder()
{
  return NULL;
}

ElemDDLStoreOptKeyColumnList *
ElemDDLNode::castToElemDDLStoreOptKeyColumnList()
{
  return NULL;
}

ElemDDLStoreOptNondroppablePK *
ElemDDLNode::castToElemDDLStoreOptNondroppablePK()
{
  return NULL;
}

ElemDDLUdfExecutionMode *
ElemDDLNode::castToElemDDLUdfExecutionMode(void)
{
  return NULL;
}

ElemDDLUdfFinalCall *
ElemDDLNode::castToElemDDLUdfFinalCall()
{
  return NULL;
}

ElemDDLUdfOptimizationHint *
ElemDDLNode::castToElemDDLUdfOptimizationHint()
{
  return NULL;
}

ElemDDLUdfParallelism *
ElemDDLNode::castToElemDDLUdfParallelism()
{
  return NULL;
}

ElemDDLUdfSpecialAttributes *
ElemDDLNode::castToElemDDLUdfSpecialAttributes(void)
{
  return NULL;
}

ElemDDLUdfStateAreaSize *
ElemDDLNode::castToElemDDLUdfStateAreaSize()
{
  return NULL;
}

ElemDDLUdfVersionTag *
ElemDDLNode::castToElemDDLUdfVersionTag()
{
  return NULL;
}
ElemDDLUdrDeterministic *
ElemDDLNode::castToElemDDLUdrDeterministic()
{
  return NULL;
}

ElemDDLUdrExternalName *
ElemDDLNode::castToElemDDLUdrExternalName()
{
  return NULL;
}

ElemDDLUdrExternalPath *
ElemDDLNode::castToElemDDLUdrExternalPath()
{
  return NULL;
}

ElemDDLUdrIsolate *
ElemDDLNode::castToElemDDLUdrIsolate()
{
  return NULL;
}

ElemDDLUdrLanguage *
ElemDDLNode::castToElemDDLUdrLanguage()
{
  return NULL;
}

ElemDDLUdrLibrary *
ElemDDLNode::castToElemDDLUdrLibrary()
{
  return NULL;
}

ElemDDLUdrMaxResults *
ElemDDLNode::castToElemDDLUdrMaxResults()
{
  return NULL;
}

ElemDDLUdrParamStyle *
ElemDDLNode::castToElemDDLUdrParamStyle()
{
  return NULL;
}

ElemDDLUdrExternalSecurity *
ElemDDLNode::castToElemDDLUdrExternalSecurity()
{
  return NULL;
}

ElemDDLUdrSqlAccess *
ElemDDLNode::castToElemDDLUdrSqlAccess()
{
  return NULL;
}

ElemDDLUdrTransaction *
ElemDDLNode::castToElemDDLUdrTransaction()
{
  return NULL;
}

ElemDDLUudfParamDef *
ElemDDLNode::castToElemDDLUudfParamDef()
{
  return NULL;
}

ElemDDLWithCheckOption *
ElemDDLNode::castToElemDDLWithCheckOption()
{
  return NULL;
}

ElemDDLWithGrantOption *
ElemDDLNode::castToElemDDLWithGrantOption()
{
  return NULL;
}

StmtDDLAddConstraint *
ElemDDLNode::castToStmtDDLAddConstraint()
{
  return NULL;
}

StmtDDLAddConstraintCheck *
ElemDDLNode::castToStmtDDLAddConstraintCheck()
{
  return NULL;
}

StmtDDLAddConstraintPK *
ElemDDLNode::castToStmtDDLAddConstraintPK()
{
  return NULL;
}

StmtDDLAddConstraintRI *
ElemDDLNode::castToStmtDDLAddConstraintRI()
{
  return NULL;
}

StmtDDLAddConstraintUnique *
ElemDDLNode::castToStmtDDLAddConstraintUnique()
{
  return NULL;
}

StmtDDLAlterAuditConfig *
ElemDDLNode::castToStmtDDLAlterAuditConfig()
{
  return NULL;
}

StmtDDLAlterCatalog *
ElemDDLNode::castToStmtDDLAlterCatalog()
{
  return NULL;
}

StmtDDLAlterSchema *
ElemDDLNode::castToStmtDDLAlterSchema()
{
  return NULL;
}

StmtDDLAlterSynonym *
ElemDDLNode::castToStmtDDLAlterSynonym()
{
  return NULL;
}

StmtDDLAlterIndex *
ElemDDLNode::castToStmtDDLAlterIndex()
{
  return NULL;
}

StmtDDLAlterIndexAttribute *
ElemDDLNode::castToStmtDDLAlterIndexAttribute()
{
  return NULL;
}

StmtDDLAlterIndexHBaseOptions *
ElemDDLNode::castToStmtDDLAlterIndexHBaseOptions()
{
  return NULL;
}

StmtDDLAlterLibrary *
ElemDDLNode::castToStmtDDLAlterLibrary()
{
  return NULL;
}

StmtDDLAlterRoutine *
ElemDDLNode::castToStmtDDLAlterRoutine()
{
  return NULL;
}

StmtDDLAlterTable *
ElemDDLNode::castToStmtDDLAlterTable()
{
  return NULL;
}

StmtDDLAlterTableAttribute *
ElemDDLNode::castToStmtDDLAlterTableAttribute()
{
  return NULL;
}

StmtDDLAlterTableColumn *
ElemDDLNode::castToStmtDDLAlterTableColumn()
{
  return NULL;
}

StmtDDLAlterTableAddColumn *
ElemDDLNode::castToStmtDDLAlterTableAddColumn()
{
  return NULL;
}

StmtDDLAlterTableDropColumn *
ElemDDLNode::castToStmtDDLAlterTableDropColumn()
{
  return NULL;
}


StmtDDLAlterTableAlterColumnLoggable *
ElemDDLNode::castToStmtDDLAlterTableAlterColumnLoggable()
{
	return NULL;
}

StmtDDLAlterTableDisableIndex *
ElemDDLNode::castToStmtDDLAlterTableDisableIndex()
{
        return NULL;
}

StmtDDLAlterTableEnableIndex *
ElemDDLNode::castToStmtDDLAlterTableEnableIndex()
{
        return NULL;
}


StmtDDLAlterTableMove *
ElemDDLNode::castToStmtDDLAlterTableMove()
{
  return NULL;
}

StmtDDLAlterTablePartition *
ElemDDLNode::castToStmtDDLAlterTablePartition()
{
  return NULL;
}

StmtDDLAlterTableRename *
ElemDDLNode::castToStmtDDLAlterTableRename()
{
  return NULL;
}

StmtDDLAlterTableStoredDesc *
ElemDDLNode::castToStmtDDLAlterTableStoredDesc()
{
  return NULL;
}

StmtDDLAlterTableNamespace *
ElemDDLNode::castToStmtDDLAlterTableNamespace()
{
  return NULL;
}

StmtDDLAlterTableAlterColumnDefaultValue *
ElemDDLNode::castToStmtDDLAlterTableAlterColumnDefaultValue()
{
  return NULL;
}

StmtDDLAlterTableAlterColumnDatatype *
ElemDDLNode::castToStmtDDLAlterTableAlterColumnDatatype()
{
  return NULL;
}

StmtDDLAlterTableAlterColumnRename *
ElemDDLNode::castToStmtDDLAlterTableAlterColumnRename()
{
  return NULL;
}

StmtDDLAlterTableAlterColumnSetSGOption *
ElemDDLNode::castToStmtDDLAlterTableAlterColumnSetSGOption()
{
  return NULL;
}

StmtDDLAlterTableSetConstraint *
ElemDDLNode::castToStmtDDLAlterTableSetConstraint()
{
  return NULL;
}

StmtDDLAlterTableToggleConstraint *
ElemDDLNode::castToStmtDDLAlterTableToggleConstraint()
{
        return NULL;
}

StmtDDLAlterTableHBaseOptions *
ElemDDLNode::castToStmtDDLAlterTableHBaseOptions()
{
        return NULL;
}

StmtDDLAlterMvRGroup *
ElemDDLNode::castToStmtDDLAlterMvRGroup() // OZ
{
  return NULL;
}

StmtDDLAlterTrigger *
ElemDDLNode::castToStmtDDLAlterTrigger()
{
  return NULL;
}

StmtDDLAlterMV *
ElemDDLNode::castToStmtDDLAlterMV()
{
  return NULL;
}

StmtDDLAlterUser *
ElemDDLNode::castToStmtDDLAlterUser()
{
  return NULL;
}

StmtDDLAlterView *
ElemDDLNode::castToStmtDDLAlterView()
{
  return NULL;
}

StmtDDLAlterDatabase *
ElemDDLNode::castToStmtDDLAlterDatabase()
{
  return NULL;
}

StmtDDLCreateSynonym *
ElemDDLNode::castToStmtDDLCreateSynonym()
{
  return NULL;
}

StmtDDLCreateExceptionTable *
ElemDDLNode::castToStmtDDLCreateExceptionTable()
{
  return NULL;
}

StmtDDLCreateComponentPrivilege *
ElemDDLNode::castToStmtDDLCreateComponentPrivilege()
{
  return NULL;
}

StmtDDLCreateCatalog *
ElemDDLNode::castToStmtDDLCreateCatalog()
{
  return NULL;
}

StmtDDLCreateIndex *
ElemDDLNode::castToStmtDDLCreateIndex()
{
  return NULL;
}

StmtDDLPopulateIndex *
ElemDDLNode::castToStmtDDLPopulateIndex()
{
  return NULL;
}

StmtDDLCreateLibrary *
ElemDDLNode::castToStmtDDLCreateLibrary()
{
  return NULL;
}

StmtDDLCommentOn *
ElemDDLNode::castToStmtDDLCommentOn()
{
  return NULL;
}



StmtDDLCreateRoutine *
ElemDDLNode::castToStmtDDLCreateRoutine()
{
  return NULL;
}

StmtDDLCreateSchema *
ElemDDLNode::castToStmtDDLCreateSchema()
{
  return NULL;
}

StmtDDLCreateSequence *
ElemDDLNode::castToStmtDDLCreateSequence()
{
  return NULL;
}

StmtDDLCreateTable *
ElemDDLNode::castToStmtDDLCreateTable()
{
  return NULL;
}

StmtDDLCreateHbaseTable *
ElemDDLNode::castToStmtDDLCreateHbaseTable()
{
  return NULL;
}

StmtDDLCreateMvRGroup *
ElemDDLNode::castToStmtDDLCreateMvRGroup()
{

	return NULL;  // OZ

}

StmtDDLCreateTrigger *
ElemDDLNode::castToStmtDDLCreateTrigger()
{
  return NULL;
}

StmtDDLCreateMV *
ElemDDLNode::castToStmtDDLCreateMV()
{
  return NULL;
}


StmtDDLCreateView *
ElemDDLNode::castToStmtDDLCreateView()
{
  return NULL;
}

StmtDDLDropSynonym *
ElemDDLNode::castToStmtDDLDropSynonym()
{
  return NULL;
}

StmtDDLDropCatalog *
ElemDDLNode::castToStmtDDLDropCatalog()
{
  return NULL;
}

StmtDDLDropComponentPrivilege *
ElemDDLNode::castToStmtDDLDropComponentPrivilege()
{
  return NULL;
}

StmtDDLDropIndex *
ElemDDLNode::castToStmtDDLDropIndex()
{
  return NULL;
}

StmtDDLDropLibrary *
ElemDDLNode::castToStmtDDLDropLibrary()  
{
  return NULL;
}

StmtDDLDropModule *
ElemDDLNode::castToStmtDDLDropModule()
{
  return NULL;
}

StmtDDLDropRoutine *
ElemDDLNode::castToStmtDDLDropRoutine()
{
  return NULL;
}

StmtDDLDropSchema *
ElemDDLNode::castToStmtDDLDropSchema()
{
  return NULL;
}

StmtDDLDropSequence *
ElemDDLNode::castToStmtDDLDropSequence()
{
  return NULL;
}

StmtDDLDropSQL *
ElemDDLNode::castToStmtDDLDropSQL()
{
  return NULL;
}

StmtDDLDropTable *
ElemDDLNode::castToStmtDDLDropTable()
{
  return NULL;
}

StmtDDLDropHbaseTable *
ElemDDLNode::castToStmtDDLDropHbaseTable()
{
  return NULL;
}

StmtDDLDropMvRGroup *
ElemDDLNode::castToStmtDDLDropMvRGroup()
{
	return NULL; 
}

StmtDDLDropTrigger *
ElemDDLNode::castToStmtDDLDropTrigger()
{
  return NULL;
}

StmtDDLDropMV *
ElemDDLNode::castToStmtDDLDropMV()
{
  return NULL;
}

StmtDDLDropView *
ElemDDLNode::castToStmtDDLDropView()
{
  return NULL;
}

StmtDDLDropExceptionTable *
ElemDDLNode::castToStmtDDLDropExceptionTable()
{
  return NULL;
}

StmtDDLGrant *
ElemDDLNode::castToStmtDDLGrant()
{
  return NULL;
}

StmtDDLGrantComponentPrivilege *
ElemDDLNode::castToStmtDDLGrantComponentPrivilege()
{
  return NULL;
}

StmtDDLSchGrant *
ElemDDLNode::castToStmtDDLSchGrant()
{
  return NULL;
}

StmtDDLGiveAll *
ElemDDLNode::castToStmtDDLGiveAll()
{
  return NULL;
}

StmtDDLGiveCatalog *
ElemDDLNode::castToStmtDDLGiveCatalog()
{
  return NULL;
}

StmtDDLGiveObject *
ElemDDLNode::castToStmtDDLGiveObject()
{
  return NULL;
}

StmtDDLGiveSchema *
ElemDDLNode::castToStmtDDLGiveSchema()
{
  return NULL;
}

StmtDDLReInitializeSQL *
ElemDDLNode::castToStmtDDLReInitializeSQL()
{
  return NULL;
}


StmtDDLInitializeSQL *
ElemDDLNode::castToStmtDDLInitializeSQL()
{
  return NULL;
}

StmtDDLRevoke *
ElemDDLNode::castToStmtDDLRevoke()
{
  return NULL;
}


StmtDDLRevokeComponentPrivilege *
ElemDDLNode::castToStmtDDLRevokeComponentPrivilege()
{
  return NULL;
}

StmtDDLSchRevoke *
ElemDDLNode::castToStmtDDLSchRevoke()
{
  return NULL;
}

StmtDDLDropConstraint *
ElemDDLNode::castToStmtDDLDropConstraint()
{
  return NULL;
}

StmtDDLRegisterCatalog *
ElemDDLNode::castToStmtDDLRegisterCatalog()
{
  return NULL;
}

StmtDDLUnregisterCatalog *
ElemDDLNode::castToStmtDDLUnregisterCatalog()
{
  return NULL;
}

ElemDDLFileAttrPOSNumPartns * ElemDDLNode::castToElemDDLFileAttrPOSNumPartns()
{
  return NULL;
}

ElemDDLFileAttrPOSTableSize * ElemDDLNode::castToElemDDLFileAttrPOSTableSize()
{
  return NULL;
}

ElemDDLFileAttrPOSDiskPool * ElemDDLNode::castToElemDDLFileAttrPOSDiskPool()
{
  return NULL;
}

ElemDDLFileAttrPOSIgnore * ElemDDLNode::castToElemDDLFileAttrPOSIgnore()
{
  return NULL;
}

StmtDDLRegisterComponent *
ElemDDLNode::castToStmtDDLRegisterComponent()
{
  return NULL;
}

StmtDDLRegisterUser *
ElemDDLNode::castToStmtDDLRegisterUser()
{
  return NULL;
}

StmtDDLRegOrUnregObject *
ElemDDLNode::castToStmtDDLRegOrUnregObject()
{
  return NULL;
}

StmtDDLCreateRole *
ElemDDLNode::castToStmtDDLCreateRole()
{
  return NULL;
}

StmtDDLCleanupObjects *
ElemDDLNode::castToStmtDDLCleanupObjects()
{
  return NULL;
}

StmtDDLRoleGrant *
ElemDDLNode::castToStmtDDLRoleGrant()
{
  return NULL;
}

StmtDDLonHiveObjects *
ElemDDLNode::castToStmtDDLonHiveObjects()
{
  return NULL;
}

//
// accessors
//

Int32
ElemDDLNode::getArity() const
{
  return 0;
}

ExprNode *
ElemDDLNode::getChild(Lng32 /* index */ )
{
  return NULL;
}

// Treat this node as an array (of one element).
// The number of elements in this array is 1.
// For more information about this method,
// please read the description of the corresponding
// method in file ElemDDLList.h
CollIndex
ElemDDLNode::entries() const
{
  return 1;
}

// Treat this node as an array (of one element).
// The only legal index is 0.
// For more information about this method,
// please read the description of the corresponding
// method in file ElemDDLList.h
ElemDDLNode *
ElemDDLNode::operator[](CollIndex index)
{
  ComASSERT(index EQU 0);
  return this;
}

// Treat this node as a list (of one element)
// represented by a left linear tree.
// For more information about this method,
// please read the description of the corresponding
// method in file ElemDDLList.h
/*virtual*/ void
ElemDDLNode::traverseList(ElemDDLNode * pOtherNode,
                          void (*visitNode)(ElemDDLNode * pOtherNode,
                                            CollIndex indexOfLeafNode,
                                            ElemDDLNode * pLeafNode))
{
  // visit the first (and only) element in the list
  (*visitNode)(pOtherNode, 0, this);
}

//
// mutators
//

void
ElemDDLNode::setChild(Lng32        /* index */ ,
                      ExprNode *  /* pElemDDLNode */ )
{
  ABORT("virtual function ElemDDLNode::setChild() must be redefined");
}

//
// method for binding
//

ExprNode *
ElemDDLNode::bindNode(BindWA * /*pBindWA*/)
{
  markAsBound();
  return this;
}

ComBoolean ElemDDLNode::applyDefaultsAndValidateObject(BindWA * pBindWA,
						       QualifiedName *qn)
{
  qn->applyDefaults(pBindWA->getDefaultSchema());

  return FALSE;
}

//
// methods for tracing
//

const NAString
ElemDDLNode::displayLabel1() const
{
  return NAString();
}

const NAString
ElemDDLNode::displayLabel2() const
{
  return NAString();
}

const NAString
ElemDDLNode::displayLabel3() const
{
  return NAString();
}

NATraceList
ElemDDLNode::getDetailInfo() const
{
  NATraceList detailTextList;
  if (NOT displayLabel1().isNull())
  {
    detailTextList.append(displayLabel1());
  }
  if (NOT displayLabel2().isNull())
  {
    detailTextList.append(displayLabel2());
  }
  if (NOT displayLabel3().isNull())
  {
    detailTextList.append(displayLabel3());
  }
  return detailTextList;
}

const NAString
ElemDDLNode::getText() const
{
  ABORT("internal logic error");
  return "ElemDDLNode";
}

void
ElemDDLNode::print(FILE * f,
                   const char * prefix,
                   const char * suffix) const
{
#ifdef TRACING_ENABLED 
  fprintf(f,"%s%s(%s)\n", prefix, (const char *)getText(), suffix);
#endif
}

// -----------------------------------------------------------------------
// methods for class ElemDDLAlterTableMove
// -----------------------------------------------------------------------

// constructor
ElemDDLAlterTableMove::ElemDDLAlterTableMove(ElemDDLNode * sourceLocationList,
                                             ElemDDLNode * destLocationList)
: ElemDDLNode(ELM_ALTER_TABLE_MOVE_ELEM)
{
  setChild(INDEX_SOURCE_LOCATION_LIST, sourceLocationList);
  setChild(INDEX_DEST_LOCATION_LIST, destLocationList);
}

// virtual destructor
ElemDDLAlterTableMove::~ElemDDLAlterTableMove()
{
}

// cast virtual function
ElemDDLAlterTableMove *
ElemDDLAlterTableMove::castToElemDDLAlterTableMove()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLAlterTableMove::getArity() const
{
  return MAX_ELEM_DDL_ALTER_TABLE_MOVE_ARITY;
}

ExprNode *
ElemDDLAlterTableMove::getChild(Lng32 index)
{
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

// mutator
void
ElemDDLAlterTableMove::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    children_[index] = pChildNode->castToElemDDLNode();
  }
  else
  {
    children_[index] = NULL;
  }
}

//
// methods for tracing
//

const NAString
ElemDDLAlterTableMove::getText() const
{
  return "ElemDDLAlterTableMove";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLAuthSchema
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLAuthSchema::~ElemDDLAuthSchema()
{
}

// cast virtual function
ElemDDLAuthSchema *
ElemDDLAuthSchema::castToElemDDLAuthSchema()
{
  return this;
}



// -----------------------------------------------------------------------
// methods for class ElemDDLGrantee
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLGrantee::~ElemDDLGrantee()
{
}

// cast virtual function
ElemDDLGrantee *
ElemDDLGrantee::castToElemDDLGrantee()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLGrantee::displayLabel1() const
{
  if (isPublic())
  {
    return NAString("is public? ") + YesNo(isPublic());
  }
  else
  {
    return (NAString("Authorization identifier: ") +
            getAuthorizationIdentifier());
  }
}

const NAString
ElemDDLGrantee::getText() const
{
  return "ElemDDLGrantee";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLGranteeArray
//
//   Note that class ElemDDLGranteeArray is not derived from class
//   ElemDDLGrantee.  The former represents an array of pointers
//   pointing to ElemDDLGrantee parse node.
// -----------------------------------------------------------------------

// constructor
ElemDDLGranteeArray::ElemDDLGranteeArray(CollHeap *heap)
  : LIST(ElemDDLGrantee *)(heap)
{
}

// virtual destructor
ElemDDLGranteeArray::~ElemDDLGranteeArray()
{
}

//----------------------------------------------------------------------------
// methods for class ElemDDLKeyValue
//----------------------------------------------------------------------------

// constructor
ElemDDLKeyValue::ElemDDLKeyValue(ItemExpr * pConstValue)
: ElemDDLNode(ELM_KEY_VALUE_ELEM)
{
  //
  // The parser currently does not handle negative
  // value well.  So we currently use castToConstValue
  // to return the nagation information of the constant.
  // This is a kludge, and it is temporary.
  //
  NABoolean isNegative;
  ComASSERT(pConstValue NEQ NULL AND
            pConstValue->castToConstValue(isNegative) NEQ NULL);
  keyValue_ = pConstValue->castToConstValue(isNegative);
}

// virtual destructor
ElemDDLKeyValue::~ElemDDLKeyValue()
{
  // delete all children
  for (Int32 i = 0; i < MAX_ELEM_DDL_KEY_VALUE_ARITY; i++)
  {
    if (getChild(i) NEQ NULL)
      delete getChild(i);
  }
}

// cast
ElemDDLKeyValue *
ElemDDLKeyValue::castToElemDDLKeyValue()
{
  return this;
}

// accessors

// get the degree of this node
Int32
ElemDDLKeyValue::getArity() const
{
  return MAX_ELEM_DDL_KEY_VALUE_ARITY;
}

ExprNode *
ElemDDLKeyValue::getChild(Lng32 index)
{
  ComASSERT(index EQU INDEX_KEY_VALUE);
  return keyValue_;
}

// mutator
void
ElemDDLKeyValue::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index EQU INDEX_KEY_VALUE);
  if (pChildNode NEQ NULL)
  {
    //
    // The parser currently does not handle negative
    // value well.  So we currently use castToConstValue
    // to return the nagation information of the constant.
    // This is a kludge, and it is temporary.
    //
    NABoolean isNegative;
    ComASSERT(pChildNode->castToItemExpr() NEQ NULL AND
              pChildNode->castToItemExpr()
                        ->castToConstValue(isNegative) NEQ NULL);
    keyValue_ = pChildNode->castToItemExpr()->castToConstValue(isNegative);
  }
  else
    keyValue_ = NULL;
}

// methods for tracing

const NAString
ElemDDLKeyValue::getText() const
{
  return "ElemDDLKeyValue";
}


// method for building text
// virtual
NAString ElemDDLKeyValue::getSyntax() const
{
  return keyValue_->getConstStr(FALSE);


} // getSyntax()




// -----------------------------------------------------------------------
// methods for class ElemDDLLibClientName
// -----------------------------------------------------------------------

//
// contructor
//

ElemDDLLibClientName::ElemDDLLibClientName(const NAString &theName)
: ElemDDLNode(ELM_CLIENTNAME_ELEM),
  clientName_(theName)
  
{

  ComASSERT(!theName.isNull());
  
}

// virtual destructor
ElemDDLLibClientName::~ElemDDLLibClientName()
{
}

// casting
ElemDDLLibClientName *
ElemDDLLibClientName::castToElemDDLLibClientName()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLLibClientName::getText() const
{
  return "ElemDDLLibClientName";
}

const NAString
ElemDDLLibClientName::displayLabel1() const
{
  return NAString("Client name: ") + getClientName();
}

// method for building text
//virtual
NAString ElemDDLLibClientName::getSyntax() const
{
  NAString syntax = "CLIENTNAME ";

  syntax += getClientName();

  return syntax;

} // getSyntax


// -----------------------------------------------------------------------
// methods for class ElemDDLLibClientFilename
// -----------------------------------------------------------------------

//
// contructor
//

ElemDDLLibClientFilename::ElemDDLLibClientFilename(const NAString &theName)
: ElemDDLNode(ELM_CLIENTFILE_ELEM),
  theFilename_(theName)
  
{

  ComASSERT(!theName.isNull());
  
}

// virtual destructor
ElemDDLLibClientFilename::~ElemDDLLibClientFilename()
{
}

// casting
ElemDDLLibClientFilename *
ElemDDLLibClientFilename::castToElemDDLLibClientFilename()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLLibClientFilename::getText() const
{
  return "ElemDDLLibClientName";
}

const NAString
ElemDDLLibClientFilename::displayLabel1() const
{
  return NAString("Client file: ") + getFilename();
}

// method for building text
//virtual
NAString ElemDDLLibClientFilename::getSyntax() const
{
  NAString syntax = "CLIENTFILE ";

  syntax += getFilename();

  return syntax;

} // getSyntax


// -----------------------------------------------------------------------
// methods for class ElemDDLLibPathName
// -----------------------------------------------------------------------

//
// contructor
//

ElemDDLLibPathName::ElemDDLLibPathName(
      const NAString     &theName)
: ElemDDLNode(ELM_PATHNAME_ELEM),
  pathName_(theName)
  
{

  ComASSERT(!theName.isNull());
  
}

// virtual destructor
ElemDDLLibPathName::~ElemDDLLibPathName()
{
}

// casting
ElemDDLLibPathName *
ElemDDLLibPathName::castToElemDDLLibPathName()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLLibPathName::getText() const
{
  return "ElemDDLLibPathName";
}

const NAString
ElemDDLLibPathName::displayLabel1() const
{
  return NAString("Path: ") + getPathName();
}

// method for building text
//virtual
NAString ElemDDLLibPathName::getSyntax() const
{

  return NAString("PATH ") + getPathName();

} // getSyntax





// -----------------------------------------------------------------------
// methods for class ElemDDLLocation
// -----------------------------------------------------------------------

//
// contructor
//

ElemDDLLocation::ElemDDLLocation(locationNameTypeEnum locationNameType,
                                 const NAString & aLocationName)
: ElemDDLNode(ELM_LOCATION_ELEM),
  locationNameInputFormat_(ComLocationName::UNKNOWN_INPUT_FORMAT),
  flags_(0),
  locationNameType_(locationNameType),
  locationName_(aLocationName, PARSERHEAP()),
  partitionName_(PARSERHEAP())
{
  //
  // checks the syntax of the input aLocationName
  //

  ComASSERT(NOT aLocationName.isNull());
  ComLocationName locName;

  switch(locationNameType)
  {
  case LOCATION_ENVIRONMENT_VARIABLE:
    //
    // This feature is currently not supported.
    // The parser has already issued error message(s).
    //
    // Does not check for valid syntax.
    //
    break;

  case LOCATION_GUARDIAN_NAME:
  case LOCATION_OSS_NAME:
    {
      if (locationNameType EQU LOCATION_GUARDIAN_NAME)
      {
        locName.copy(aLocationName,
                     ComLocationName::GUARDIAN_LOCATION_NAME_FORMAT);
      }
      else
      {
        locName.copy(aLocationName,
                     ComLocationName::OSS_LOCATION_NAME_FORMAT);
      }

      if (NOT locName.isValid())
      {
        // Illegal location name.
        *SqlParser_Diags << DgSqlCode(-3061) << DgString0(aLocationName);
      }
      locationNameInputFormat_ = locName.getInputFormat();

      //
      // check system name part
      //

      const ComNodeName & systemName = locName.getGuardianSystemNamePart();
      if (!systemName.isNull())
      {
        Lng32 systemNumber;
        if (systemName.getNodeNumber(systemNumber) &&
	   (CmpCommon::getDefault(CREATE_OBJECTS_IN_METADATA_ONLY,0) == DF_OFF))
           //|| !OSIM_runningSimulation()))
          // Currently system name is not defined
          *SqlParser_Diags << DgSqlCode(-3122) << DgString0(aLocationName);
      }

      //
      // check subvolume name part
      //

      const ComSubvolumeName & svNamePart = locName.getSubvolumeNamePart();
      if (!svNamePart.isNull())
      {
        size_t svMinLength = 4;
        if (!svNamePart.isMXSubvol (svMinLength))
        {
          flags_ |= INVALIDSUBVOLNAME;
          // Subvolume name part must be eight characters long and
          // contain the ZSD prefix
          *SqlParser_Diags << DgSqlCode(-3025) << DgString0(aLocationName);
        }
      }

      //
      // check file name part
      //

      const ComFileName & fileNamePart = locName.getFileNamePart();
      if (!fileNamePart.isNull())
      {
        if (NOT fileNamePart.isMXDataFork())
        {
          flags_ |= INVALIDFILENAME;
          // File name part must be eight characters long and
          // contain the two zero digit suffix
          *SqlParser_Diags << DgSqlCode(-3123) << DgString0(aLocationName);
        }
      }
    }
    break;

  default:
    ABORT("internal logic error");
    break;
  }
}

// virtual destructor
ElemDDLLocation::~ElemDDLLocation()
{
}

// casting
ElemDDLLocation *
ElemDDLLocation::castToElemDDLLocation()
{
  return this;
}

// accessor
NAString
ElemDDLLocation::getLocationNameTypeAsNAString() const
{
  switch (getLocationNameType())
  {
  case LOCATION_OSS_NAME :
    return NAString("OSS path name");
  case LOCATION_GUARDIAN_NAME :
    return NAString("Guardian device name");
  case LOCATION_ENVIRONMENT_VARIABLE :
    return NAString("OSS environment variable name");
  default :
    ABORT("internal logic error");
    return NAString();
  }
}

//
// methods for tracing
//

const NAString
ElemDDLLocation::getText() const
{
  return "ElemDDLLocation";
}

const NAString
ElemDDLLocation::displayLabel1() const
{
  return NAString("Location name: ") + getLocationName();
}

const NAString
ElemDDLLocation::displayLabel2() const
{
  return NAString("Location name type: ") + getLocationNameTypeAsNAString();
}

const NAString
ElemDDLLocation::displayLabel3() const
{
  return NAString("Partition name: ") + getPartitionName();
}


// method for building text
//virtual
NAString ElemDDLLocation::getSyntax() const
{
  NAString syntax = "LOCATION ";

  syntax += getLocation();

  if(getPartitionName() != "")
  {
	  syntax += "NAME ";
	  syntax += getPartitionName();
  }

  return syntax;

} // getSyntax

// -----------------------------------------------------------------------
// methods for class ElemDDLParallelExec
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLParallelExec::~ElemDDLParallelExec()
{
}

// cast
ElemDDLParallelExec *
ElemDDLParallelExec::castToElemDDLParallelExec()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLParallelExec::displayLabel1() const
{
  return (NAString("is parallel exec enabled? ") +
          YesNo(isParallelExecEnabled()));
}

const NAString
ElemDDLParallelExec::displayLabel2() const
{
  if (isParallelExecEnabled())
  {
    return NAString("config file name: ") + getConfigFileName();
  }
  else
  {
    return NAString();
  }
}

const NAString
ElemDDLParallelExec::getText() const
{
  return "ElemDDLParallelExec";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPrivileges
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPrivileges::~ElemDDLPrivileges()
{
}

// cast virtual function
ElemDDLPrivileges *
ElemDDLPrivileges::castToElemDDLPrivileges()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLPrivileges::getArity() const
{
  return MAX_ELEM_DDL_PRIVILEGES_ARITY;
}

ExprNode *
ElemDDLPrivileges::getChild(Lng32 index)
{
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutators
//

void
ElemDDLPrivileges::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    children_[index] = pChildNode->castToElemDDLNode();
  }
  else
  {
    children_[index] = NULL;
  }
}

NABoolean ElemDDLPrivileges::containsPriv(OperatorTypeEnum whichPriv) const
{
  ElemDDLNode *pActionList = getPrivilegeActionList();
  if (!pActionList)
    return FALSE;

  ElemDDLList * pDDLList = pActionList->castToElemDDLList();
  if (!pDDLList)
  {
    ElemDDLPrivAct *pPrivAct = pActionList->castToElemDDLPrivAct();
    if ( pPrivAct->getOperatorType() == whichPriv )
      return TRUE;
  }
  else
  {
    for (CollIndex i = 0 ; i < pDDLList->entries() ; i++)
    {
      ElemDDLPrivAct *pPrivAct = (*pDDLList)[i]->castToElemDDLPrivAct();
      if ( pPrivAct->getOperatorType() == whichPriv )
        return TRUE;
    }
  }
  return FALSE;
} 

NABoolean ElemDDLPrivileges::containsColumnPrivs() const
{
  ElemDDLNode * privActs = getPrivilegeActionList();
  if (!privActs)
    return FALSE;

  ElemDDLList * privActsList = privActs->castToElemDDLList();
  if (!privActsList)
  {
    if (privActs->castToElemDDLPrivActWithColumns())
    {
      if(privActs->castToElemDDLPrivActWithColumns()
          ->getColumnNameArray().entries() > 0)
          return TRUE;
    }
  }
  else
  {
    for (CollIndex i = 0 ; i < privActsList->entries() ; i++)
    {
      if ((*privActsList)[i]->castToElemDDLPrivActWithColumns())
      {
        if((*privActsList)[i]->castToElemDDLPrivActWithColumns()
          ->getColumnNameArray().entries() > 0)
          return TRUE;
      }
    }
  }
  return FALSE;
}

//
// methods for tracing
//

const NAString
ElemDDLPrivileges::displayLabel1() const
{
  return NAString("is all privileges? ") + YesNo(isAllPrivileges());
}

NATraceList
ElemDDLPrivileges::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;
  ElemDDLNode   * pPrivActList = getPrivilegeActionList();

  if (isAllPrivileges())
  {
    ComASSERT(pPrivActList EQU NULL);
    detailTextList.append(displayLabel1());   // All Privileges
    return detailTextList;
  }

  ComASSERT(pPrivActList NEQ NULL);

  detailText = "Privilege Action List [";
  detailText += LongToNAString((Lng32)pPrivActList->entries());
  detailText += " element(s)]:";
  detailTextList.append(detailText);

  for (CollIndex i = 0; i < pPrivActList->entries(); i++)
  {
    detailText = "[privilege action ";
    detailText += LongToNAString((Lng32)i);
    detailText += "]";
    detailTextList.append(detailText);

    detailTextList.append("    ", (*pPrivActList)[i]->getDetailInfo());
  }

  return detailTextList;
}

const NAString
ElemDDLPrivileges::getText() const
{
  return "ElemDDLPrivileges";
}


// -----------------------------------------------------------------------
// methods for class ElemDDLReferences
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLReferences::~ElemDDLReferences()
{
  // delete all children
  for (Int32 index = 0; index < getArity(); index++)
  {
    delete getChild(index);
  }
}

//cast
ElemDDLReferences *
ElemDDLReferences::castToElemDDLReferences()
{
  return this;
}

//
// accessors
//

Int32
ElemDDLReferences::getArity() const
{
  return MAX_ELEM_DDL_REFERENCES_ARITY;
}

ExprNode *
ElemDDLReferences::getChild(Lng32 index)
{
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

NAString
ElemDDLReferences::getReferencedTableName() const
{
  return getReferencedNameAsQualifiedName().getQualifiedNameAsAnsiString();
}

//
// mutator
//

void
ElemDDLReferences::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode EQU NULL)
  {
    children_[index] = NULL;
  }
  else
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    children_[index] = pChildNode->castToElemDDLNode();
  }
}

//
// methods for tracing
//

const NAString
ElemDDLReferences::displayLabel1() const
{
  return NAString("Referenced table name: ") + getReferencedTableName();
}

NATraceList
ElemDDLReferences::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  //
  // referenced table name
  //

  detailTextList.append(displayLabel1());

  //
  // referenced columns
  //

  if (getReferencedColumns() EQU NULL)
  {
    detailTextList.append("Referenced column list not specified.");
  }
  else
  {
    CollIndex nbrCols = getReferencedColumns()->entries();
    ComASSERT(nbrCols > 0);

    detailText = "Referenced column list [";
    detailText += LongToNAString((Lng32)nbrCols);
    detailText += " element(s)]:";
    detailTextList.append(detailText);

    for (CollIndex i = 0; i < nbrCols; i++)
    {
      detailText = "[referenced column ";
      detailText += LongToNAString((Lng32)i);
      detailText += "]";
      detailTextList.append(detailText);

      NATraceList refColDetailTextList
        = (*getReferencedColumns())[i]->getDetailInfo();

      for (CollIndex j = 0; j < refColDetailTextList.entries(); j++)
      {
        detailTextList.append(NAString("    ") + refColDetailTextList[j]);
      }
    }
  } // else (getReferencedColumns() NEQ NULL)

  return detailTextList;

} // ElemDDLReferences::getDetailInfo()

const NAString
ElemDDLReferences::getText() const
{
  return "ElemDDLReferences";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLSchemaName
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLSchemaName::~ElemDDLSchemaName()
{
}

// cast virtual function
ElemDDLSchemaName *
ElemDDLSchemaName::castToElemDDLSchemaName()
{
  return this;
}

// -----------------------------------------------------------------------
// methods for class ElemDDLTableFeature
// -----------------------------------------------------------------------
//
// methods for tracing
//

const NAString
ElemDDLTableFeature::displayLabel1() const
{
  NAString tableAttr ("Table options: ");
  if (isDroppable())
    tableAttr += "DROPPABLE, ";
  else
    tableAttr += "NOT DROPPABLE, ";
  if (isInsertOnly())
    tableAttr += "INSERT_ONLY ";
  else
    tableAttr += "ALL DML ALLOWED";

  return tableAttr;
}

const NAString
ElemDDLTableFeature::getText() const
{
  return "ElemDDLTableFeature";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLHbaseOptions
// -----------------------------------------------------------------------
//
// methods for tracing
//

const NAString
ElemDDLHbaseOptions::displayLabel1() const
{
  return "";
}

const NAString
ElemDDLHbaseOptions::getText() const
{
  return "ElemDDLHbaseOptions";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLWithCheckOption
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLWithCheckOption::~ElemDDLWithCheckOption()
{
}

// cast virtual function
ElemDDLWithCheckOption *
ElemDDLWithCheckOption::castToElemDDLWithCheckOption()
{
  return this;
}

//
// accessor
//

NAString
ElemDDLWithCheckOption::getLevelAsNAString() const
{
  switch (getLevel())
  {
  case COM_CASCADED_LEVEL :
    return "CASCADED";
  case COM_LOCAL_LEVEL :
    return "LOCAL";
  case COM_UNKNOWN_LEVEL :
    return "UNKNOWN";
  default :
    ABORT("internal logic error");
    return NAString();
  }
}

//
// methods for tracing
//

const NAString
ElemDDLWithCheckOption::displayLabel1() const
{
  return NAString("Level: ") + getLevelAsNAString();
}

const NAString
ElemDDLWithCheckOption::getText() const
{
  return "ElemDDLWithCheckOption";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLWithGrantOption
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLWithGrantOption::~ElemDDLWithGrantOption()
{
}

// cast virtual function
ElemDDLWithGrantOption *
ElemDDLWithGrantOption::castToElemDDLWithGrantOption()
{
  return this;
}

// methods for tracing
const NAString
ElemDDLWithGrantOption::getText() const
{
  return "ElemDDLWithGrantOption";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColNameListNode
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLColNameListNode :: ~ElemDDLColNameListNode()
{
    // delete the column name list

  delete columnNameList_;
}

// cast virtual function
ElemDDLColNameListNode *
ElemDDLColNameListNode::castToElemDDLColNameListNode()
{
  return this;
}

// methods for tracing
const NAString
ElemDDLColNameListNode ::getText() const
{
  return "ElemDDLColNameListNode";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLDivisionClause
// -----------------------------------------------------------------------

ElemDDLDivisionClause::ElemDDLDivisionClause(ItemExpr * pDivisionExprTree)
  : ElemDDLNode(ELM_DIVISION_CLAUSE_ELEM),
    eDivisionType_(DIVISION_BY_EXPR_LIST),
    pDivisionExprTree_(pDivisionExprTree), // shallow copy - Assume *pDivisionExprTree parse tree allocated in PARSERHEAP()
    pDivisionExprList_(NULL),
    pColumnRefList_(NULL),
    columnRefArray_(PARSERHEAP()),
    startPos_(0),
    endPos_(0)
{
  pDivisionExprList_ = new(PARSERHEAP()) ItemExprList(pDivisionExprTree, PARSERHEAP());
}

ElemDDLDivisionClause::ElemDDLDivisionClause(divisionTypeEnum eDivisionType)
  : ElemDDLNode(ELM_DIVISION_CLAUSE_ELEM),
    eDivisionType_(eDivisionType),
    pDivisionExprTree_(NULL),
    pDivisionExprList_(NULL),
    pColumnRefList_(NULL),
    columnRefArray_(PARSERHEAP()),
    startPos_(0),
    endPos_(0)
{
  PARSERASSERT(eDivisionType EQU DIVISION_LIKE_TABLE); // valid in CREATE INDEX syntax only
}

// virtual destructor
ElemDDLDivisionClause::~ElemDDLDivisionClause()
{
  // Note that we do not destroy the parse tree pointed by data member
  // pDivisionExprTree_ (because the constructor did a shallow copy).
  // We assume that the parse tree is allocated in the PARSERHEAP().
  delete pDivisionExprList_;
}

// safe cast-down virtual function
ElemDDLDivisionClause * ElemDDLDivisionClause::castToElemDDLDivisionClause()
{
  return this;
}

// Gather the information from the child sub-tree and store it in this Parse node
void ElemDDLDivisionClause::synthesize(ElemDDLNode * pColRefTree)
{
  this->setColumnRefList(pColRefTree);

  size_t colRefEntries = 0;
  if (pColRefTree NEQ NULL)
    colRefEntries = pColRefTree->entries();
  if (colRefEntries EQU 1)
  {
    columnRefArray_.insert(pColRefTree->castToElemDDLColRef());
  }
  else if (colRefEntries > 1 AND pColRefTree NEQ NULL)
  {
    ElemDDLList * pElemDDLList = pColRefTree->castToElemDDLList();
    ElemDDLColRef * pElemDDLColRef = NULL;
    ItemExpr * pDivExpr = NULL;
    ComColumnOrdering eSortingOrder = COM_UNKNOWN_ORDER;
    Int32 iInverseNodeCount = 0;

    for (CollIndex i = 0; i < colRefEntries; i++)
    {
      pElemDDLColRef = ((*pElemDDLList)[i])->castToElemDDLColRef();
      columnRefArray_.insert(pElemDDLColRef); // shallow copy

      // Get the sorting order setting from the division expression parse tree;
      // then assign that value to the associating ElemDDLColRef parse node
      // to override the ElemDDLColRef default sorting order setting.
      pDivExpr = (*pDivisionExprList_)[i];
      eSortingOrder = COM_ASCENDING_ORDER;
      iInverseNodeCount = 0;
      while (pDivExpr NEQ NULL AND pDivExpr->getOperatorType() EQU ITM_INVERSE)
      {
        pDivExpr = pDivExpr->child(0);
        iInverseNodeCount++;
      }
      if ((iInverseNodeCount % 2) EQU 1) // odd count
        eSortingOrder = COM_DESCENDING_ORDER;
      pElemDDLColRef->setColumnOrdering(eSortingOrder);
    }
  }
}

// Check to make sure that the number of division expressions in the DIVISION BY ( ... ) clause
// matches the number of columns specified in the associating COLUMN NAME[S] ( ... ) clause
NABoolean ElemDDLDivisionClause::isNumOfDivExprsAndColsMatched() const
{
  if (pDivisionExprList_ NEQ NULL)
  {
    if (pDivisionExprList_->entries() NEQ columnRefArray_.entries())
      return FALSE;
  }
  else // pDivisionExprList_ EQU NULL
  {
    if (columnRefArray_.entries() > 0)
      return FALSE;
  }
  return TRUE;
}

// virtual method for tracing
const NAString ElemDDLDivisionClause::getText() const
{
  return "ElemDDLDivisionClause";
}

// method for building text
// virtual 
NAString ElemDDLDivisionClause::getSyntax() const
{
  ElemDDLDivisionClause * ncThis = const_cast<ElemDDLDivisionClause *>(this);
  ItemExprList * pItemExprList = ncThis->getDivisionExprList();
  ParNameLocList & nameLocList = ncThis->getNameLocList();
  const char * pInputStr = nameLocList.getInputStringPtr();
  StringPos startPos = this->getStartPosition();
  StringPos endPos = this->getEndPosition();

  // Temporary code to help with debugging
  // *SqlParser_Diags << DgSqlCode(-1110) 
  //                  << DgString0("Define LOOP_ON_ERROR=1110 in file ms.env before starting sqlci");

  NAString syntax;
  if (pItemExprList EQU NULL OR pItemExprList->entries() EQU 0 OR
      pInputStr EQU NULL OR strlen(pInputStr) EQU 0 OR startPos >= endPos)
    return syntax;

  const char * pDivExprListText = &pInputStr[startPos];
  size_t divExprListTextLenInBytes = (size_t)(endPos + 1 - startPos);

  syntax = " DIVISION BY ";
  if (*pDivExprListText NEQ '(')
    syntax += "(";
  syntax.append(pDivExprListText, divExprListTextLenInBytes);
  if (*pDivExprListText NEQ '(')
    syntax += ")";
  syntax += " ";

  return syntax;

} // getSyntax

// -----------------------------------------------------------------------
// methods for class ElemDDLSaltOptionsClause
// -----------------------------------------------------------------------

ElemDDLSaltOptionsClause::ElemDDLSaltOptionsClause(ElemDDLNode * pSaltExprTree,
                                                   Int32 numPartitions)
  : ElemDDLNode(ELM_SALT_OPTIONS_ELEM),
    numPartitions_(numPartitions),
    saltColumnArray_(PARSERHEAP()),
    likeTable_(FALSE)
{
  setChild(INDEX_SALT_COLUMN_LIST, pSaltExprTree);
  //
  // copies pointers to parse nodes representing
  // column names (appearing in a store option) to
  // keyColumnArray_ so the user can access this
  // information easier.
  //

  if (pSaltExprTree)
    for (CollIndex i = 0; i < pSaltExprTree->entries(); i++)
      {
        saltColumnArray_.insert((*pSaltExprTree)[i]->castToElemDDLColRef());
      }
}

ElemDDLSaltOptionsClause::ElemDDLSaltOptionsClause(NABoolean likeTable)
  : ElemDDLNode(ELM_SALT_OPTIONS_ELEM),
    numPartitions_(0),
    saltColumnArray_(PARSERHEAP()),
    likeTable_(likeTable)
{
}


// virtual destructor
ElemDDLSaltOptionsClause::~ElemDDLSaltOptionsClause()
{
}

// safe cast-down virtual function
ElemDDLSaltOptionsClause * ElemDDLSaltOptionsClause::castToElemDDLSaltOptionsClause()
{
  return this;
}

// virtual method for tracing
const NAString ElemDDLSaltOptionsClause::getText() const
{
  return "ElemDDLSaltOptionsClause";
}

// get the degree of this node
Int32
ElemDDLSaltOptionsClause::getArity() const
{
  return MAX_ELEM_DDL_SALT_OPT_KEY_COLUMN_LIST_ARITY;
}

ExprNode *
ElemDDLSaltOptionsClause::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

// mutator
void
ElemDDLSaltOptionsClause::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < MAX_ELEM_DDL_SALT_OPT_KEY_COLUMN_LIST_ARITY);
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    children_[index] = pChildNode->castToElemDDLNode();
  }
  else
  {
    children_[index] = NULL;
  }
}


NABoolean
ElemDDLSaltOptionsClause::getLikeTable() const
{
  return likeTable_;
}

void
ElemDDLSaltOptionsClause::unparseIt(NAString & result) const
{
  if (likeTable_)
    result = "SALT LIKE TABLE";
  else
    {  
      char buf[40];
      sprintf(buf," SALT USING %d PARTITIONS",numPartitions_);
      result = buf;

      if (saltColumnArray_.entries() > 0)
        {
          result += " ON (";
          const ElemDDLColRef * colRef = saltColumnArray_[0];
          result += colRef->getColumnName();
          for (CollIndex i = 1; i < saltColumnArray_.entries(); i++)
            {
              result += ",";
              colRef = saltColumnArray_[i];
              result += colRef->getColumnName();
            }
          result += ")";
        }
    }
}

//
// End of File
//
