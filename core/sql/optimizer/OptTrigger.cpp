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
******************************************************************************
*
* File:         OptTrigger.cpp
* Description:  Implementation of after trigger reorder & transformation
*               
*               
* Created:      06/25/98
* Language:     C++
*
*
*
*
*
*******************************************************************************/
//-----------------------------------------------------------------------------
//
// Triggers internal spec describes the input tree to OptTriggersBackbone.
//
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <search.h>

#include "GroupAttr.h"
#include "AllItemExpr.h"
#include "AllRelExpr.h"
#include "Triggers.h"
#include "OptTrigger.h"
#include "BindWA.h"
#include "ItemSample.h"
#include "ChangesTable.h"
#include "NormWA.h"

//-----------------------------------------------------------------------------------
//            Static utility methods (part of OptTriggersBackbone class)
//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
//
//
//	SetUnionCharacteristicInputs
//
// Calculate union Characteristic Inputs based on both children.
// The parent input is the aggregate input of the children after reducing the 
// duplicates.
//
//-----------------------------------------------------------------------------------

void
OptTriggersBackbone::SetUnionCharacteristicInputs(RelExpr *pUnion)
{
  ValueIdSet tmpInput = ((RelExpr *)pUnion->getChild(1))->getGroupAttr()->getCharacteristicInputs(); 
  ValueIdSet tmpInputLeft = ((RelExpr *)pUnion->getChild(0))->getGroupAttr()->getCharacteristicInputs();

  // reduce duplicates between left and right
  tmpInput.removeCoveredExprs(tmpInputLeft);
  tmpInputLeft.removeCoveredExprs(tmpInput);

  // add left
  tmpInput += tmpInputLeft;

  pUnion->getGroupAttr()->setCharacteristicInputs(tmpInput);
}

//-----------------------------------------------------------------------------------
//
//
//	SetTsjCharacteristicInputs
//
// Calculate TSJ Characteristic Inputs based on its children.
// The parent input is the aggregate input of the children after reducing the 
// duplicates and the output of the left child, which is the input of the right 
// child.
//
//-----------------------------------------------------------------------------------

void
OptTriggersBackbone::SetTsjCharacteristicInputs(RelExpr *pTsj)
{
  // add left
  ValueIdSet tmpInput = ((RelExpr *)pTsj->getChild(0))->getGroupAttr()->getCharacteristicInputs();
  ValueIdSet rightTopInput = ((RelExpr *)pTsj->getChild(1))->getGroupAttr()->getCharacteristicInputs();

  // remove the input that are covered by the tsj left child outputs
  rightTopInput.removeCoveredExprs(((RelExpr *)pTsj->getChild(0))->getGroupAttr()->getCharacteristicOutputs());
  // reduce duplicates
  rightTopInput.removeCoveredExprs(tmpInput);
  tmpInput.removeCoveredExprs(rightTopInput);

  // add right
  tmpInput += rightTopInput;
  pTsj->getGroupAttr()->setCharacteristicInputs(tmpInput);
}

//-----------------------------------------------------------------------------------
// This method mainly deals with how to properly handle the UniqueExecuteId, 
// without using the same BindWA used for binding the rest of the tree:
// We want the UniqueExecuteId function to have the same ValueId as in the rest of
// the tree. To achieve this, we create a RETDesc in the current scope of the new
// BindWA, and add to it the ValueId used for the UniqueExecId (saved in the
// BindInfo), and give it a name as if it was a "virtual" column. The built sub-trees
// will use a ColReference with this name, and when bound, will find this ValueId in
// an upper scope, and correctly mark the ValueId as an input.
//-----------------------------------------------------------------------------------

void
OptTriggersBackbone::PrepareNewBindWA(BindWA &bindWA, TriggerBindInfo *bindInfo)
{
  // Set the IudId from this backbone into the BindWA.
  bindWA.setUniqueIudNum(bindInfo->getBackboneIudNum());

  RETDesc *retDesc = new(CmpCommon::statementHeap()) RETDesc(&bindWA);

  // Add the existing ExecId to the current RETDesc.
  ColRefName *execIdName = new(CmpCommon::statementHeap())
    ColRefName(InliningInfo::getExecIdVirtualColName());
  ValueId topExecId = bindInfo->getExecuteId()->getValueId();
  retDesc->addColumn(&bindWA, *execIdName, topExecId);
  
  bindWA.getCurrentScope()->setRETDesc(retDesc);
}

//-----------------------------------------------------------------------------------
// Complete the necessary compilation steps: Bind, Transform and Normalize.
//-----------------------------------------------------------------------------------

RelExpr *
OptTriggersBackbone::BindAndNormalizeNewlyConstructedTree(BindWA *bindWA, RelExpr *topNode)
{
  // Bind the tree.
  topNode =  topNode->bindNode(bindWA);
  CMPASSERT(topNode != NULL && !bindWA->errStatus());

  // Create a new NormWA for the normalization..
  NormWA normWA(CmpCommon::context());
  normWA.allocateAndSetVEGRegion(IMPORT_AND_EXPORT, topNode);
  ExprGroupId eg(topNode);
  topNode->transformNode(normWA, eg);
  topNode = eg.getPtr();
  // The rewrite phase has to be invoked explicitly, instead of by the 
  // RelRoot at the top of the tree.
  topNode->rewriteNode(normWA);
  CMPASSERT (topNode != NULL);
  topNode = topNode->normalizeNode(normWA);
  CMPASSERT (topNode != NULL);

  return topNode;
}

//-----------------------------------------------------------------------------------
//			Methods For Class OptTrigger
//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
//
// The ctor of OptTrigger
//
//-----------------------------------------------------------------------------------

OptTrigger::OptTrigger(SubTreeAccessSet *treeAccessSet, 
		       NABoolean isRowTrigger,
		       RelExpr *triggerSubTree, 
		       RelExpr *parentSubTree)
: treeAccessSet_(treeAccessSet),
  isRowTrigger_(isRowTrigger),
  triggerSubtree_(triggerSubTree),
  triggerParentSubtree_(parentSubTree)
{
}

//-----------------------------------------------------------------------------------
//			Methods For Class OptTriggerList
//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
//
//	createUnionSubTree
//
// Build left union tree from the trigger list (called by toTree)
//
//-----------------------------------------------------------------------------------

RelExpr	*
OptTriggerList::createUnionSubTree() const
{
  // Put the first trigger in the group in previous sub tree
  OptTriggerPtr optTrigger = (*this)[0];
  RelExpr *pPrev = (RelExpr *)optTrigger->getTriggerSubTree(); 

  for (CollIndex i = 1; i < entries(); i++)
  {
    // Create a union on top of the previous  sub tree and the current trigger
    // If getTriggerParentSubTree() exists it can be reused as the union
    optTrigger = (*this)[i];
    Union *pUnion = (Union *)optTrigger->getTriggerParentSubTree();
    if (pUnion == NULL){
      pUnion = new(CmpCommon::statementHeap()) 
      Union(pPrev, (RelExpr *)optTrigger->getTriggerSubTree(),NULL,NULL,
            REL_UNION,CmpCommon::statementHeap(),TRUE);
    }
    else{
      // reuse 
      pUnion->setChild(0,(ExprNode *)pPrev);
      pUnion->setChild(1,(ExprNode *)optTrigger->getTriggerSubTree());
    }
    
    pUnion->setGroupAttr(new(CmpCommon::statementHeap()) GroupAttributes());
    // set characteristic input based on children
    OptTriggersBackbone::SetUnionCharacteristicInputs(pUnion);

    pUnion->synthLogProp();

    pPrev = pUnion;
  }

  return pPrev;
}

//-----------------------------------------------------------------------------------
//			Methods For Class OptTriggerGroup
//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
//
// The ctor of OptTriggerGroup
//
//-----------------------------------------------------------------------------------

OptTriggerGroup::OptTriggerGroup(TriggerGroupFlags flag,
				 OptTriggersBackbone *backbone)
: groupAccessSet_(CmpCommon::statementHeap()),
  Flags_(flag),
  backbone_(backbone)
{
}

//-----------------------------------------------------------------------------------
//
//	isConflicting
//
// Check if the given access set conflict with the group access set
//
//-----------------------------------------------------------------------------------

NABoolean 
OptTriggerGroup::isConflicting(const SubTreeAccessSet *accessSet) const
{
  //If the group is empty or the access set is null return FALSE - no conflict
  if (entries() == 0 || accessSet == NULL)
    return FALSE;


  //check if the given accessSet conflict with the group
  return (groupAccessSet_.isConflicting(accessSet));
}

//-----------------------------------------------------------------------------------
//	addTrigger
//
// add trigger to the group and trigger access set to group access set
//
//-----------------------------------------------------------------------------------

void
OptTriggerGroup::addTrigger(const OptTriggerPtr trigger)
{
  if (trigger->isRowTrigger())
    rowTriggerGroup_.insert(trigger);
  else
    statmentTriggerGroup_.insert(trigger);

  groupAccessSet_.add(trigger->getAccessSet());
}

//-----------------------------------------------------------------------------------
//
// toTree
//
// Write group sub tree
//
// Group can be a cobmination of row triggers and statment triggers.
//
// All row triggers will be organized in one left union tree.
// If it is a PIPELINED_ROW_TRIGGERS, then this tree is the group tree 
// representation.
// If is a BLOCKED_GROUP then the tree has additional temp scan with TSJ feeding  
//	the row trigger tree.
// All statement triggers will be organized in another left union tree.
// If the group has both row and statement triggers then the two sub trees will have   
// additional union on top of them.  
// 
//-----------------------------------------------------------------------------------

RelExpr	*
OptTriggerGroup::toTree()
{
  RelExpr *pRowSubTree = NULL;
  RelExpr *pStatmentUnion = NULL;

  // Create row triggers sub-tree
  if (rowTriggerGroup_.entries())
  {
    // Create row trigger sub-tree
    pRowSubTree = createRowTriggersSubTree();

    // In pipelined group return the sub tree as is and it will be connected 
    // to the flow coming from the IUD.
    if (isPipeLinedRowTriggesGroup())
    {
      return(pRowSubTree);
    }
  }

  // Create statment triggers left union sub-tree
  if (statmentTriggerGroup_.entries())
  {
    pStatmentUnion = statmentTriggerGroup_.createUnionSubTree();
  }

  // If there is no mix of statement triggers and row trigger return
  if (pStatmentUnion == NULL)
    return pRowSubTree;
  if (pRowSubTree == NULL)
    return pStatmentUnion;

  // Connect row trigger sub-tree and statement trigger sub-tree with union
  RelExpr *pUnion = new(CmpCommon::statementHeap()) Union
    (pRowSubTree, pStatmentUnion, NULL, NULL, REL_UNION,
     CmpCommon::statementHeap(),TRUE);
  
  pUnion->setGroupAttr(new(CmpCommon::statementHeap()) GroupAttributes());

  // Set characteristic input based on children, triggers have no outputs
  OptTriggersBackbone::SetUnionCharacteristicInputs(pUnion);

  pUnion->synthLogProp();
  return pUnion;
}

//-----------------------------------------------------------------------------------
//
// createRowTriggersSubTree
//
// This method returns the built sub-tree of all row-triggers in the group along with
// the generated feeding temp-table sub-tree.
// 
//-----------------------------------------------------------------------------------

RelExpr	*
OptTriggerGroup::createRowTriggersSubTree()
{
  // Create row trigger left union tree
  RelExpr *pRowUnion = rowTriggerGroup_.createUnionSubTree();

  if (isPipeLinedRowTriggesGroup())
  {
    // In pipelined group, there's no need to connect to the feeding sub-tree.
    return(pRowUnion);
  }

  // Generate the temporary table scan to feed the triggers
  reqOutput_ = pRowUnion->getGroupAttr()->getCharacteristicInputs();
  RelExpr *pLeft = buildTempScanTree();

  // Create TSJ between the generated temp-scan and the triggers' sub-tree
  RelExpr *pTsj = new(CmpCommon::statementHeap()) Join(pLeft, pRowUnion, REL_TSJ);
  pTsj->setGroupAttr(new(CmpCommon::statementHeap()) GroupAttributes());

  OptTriggersBackbone::SetTsjCharacteristicInputs(pTsj);

  // disable parallele execution for TSJs that control row triggers
  // execution. Parallel execution for triggers TSJ introduces the 
  // potential for non-deterministic execution 
  pTsj->getInliningInfo().setFlags(II_SingleExecutionForTriggersTSJ);

  pTsj->synthLogProp();

  return pTsj;
}

//-----------------------------------------------------------------------------------
//
// buildTempScanTree
//
// Build the subtree with temp Scan nodes, and connect it to the triggers using
// a MapValueIds node.
// This tree looks like this one of these three options:
//
//        MapValueIds         MapValueIds     MapValueIds
//             |                   |               |
//           RelRoot             RelRoot         RelRoot
//             |                   |               |
//           Join              Temp Scan       Temp Scan
//          /    \               @OLD            @NEW
//  Temp Scan   Temp Scan
//    @OLD        @NEW
//
// The first is used only for Update operations when the triggers in this 
// group actually use both the OLD and the NEW values. 
//
//-----------------------------------------------------------------------------------
RelExpr *
OptTriggerGroup::buildTempScanTree()
{
  // Create a new BindWA - we don't want to have anything left in some RETDesc
  // that may affect this binding.
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());
  TriggerBindInfo *bindInfo = backbone_->getTriggerBindInfo();

  // Prepare the new BindWA for binding our little sub-tree.
  OptTriggersBackbone::PrepareNewBindWA(bindWA, bindInfo);

  // Analyze the mappings between the ValuesIds required by the triggers, and
  // the OLD and NEW values that will be produced by the temp Scans.
  OptColumnMapping colMapping(bindInfo, 
			      backbone_->getTriggeringNode()->getOperatorType(),
			      CmpCommon::statementHeap());
  colMapping.initializeMappings(reqOutput_);

  // Construct the temp Scan node, or Join of two Scans..
  RelExpr *topNode = buildTempTableScanNodes(colMapping, &bindWA);

  // Put a RelRoot node on top of it, with a select list of just the minimal
  // columns required by the triggers.
  ItemExpr *selectList = colMapping.buildSelectListFromNeededInputs();
  topNode = new(CmpCommon::statementHeap())
    RelRoot(topNode, REL_ROOT, selectList);

  // Do the necessary compilation steps.
  topNode = OptTriggersBackbone::BindAndNormalizeNewlyConstructedTree(&bindWA, topNode);

  // Now that we know the ValueIds produced by the Scans, build the 
  // MapValueIds node on top of the tree, so the triggers will get the 
  // ValueIds they are expecting.
  topNode = colMapping.buildMapValueIdNode(topNode);

  return topNode;
}

//-----------------------------------------------------------------------------------
// If both NEW and OLD values are needed for an Update trigger, create a Join of
// two temp Scan nodes. Otherwise - create just the tempScan node that is needed.
//-----------------------------------------------------------------------------------

RelExpr *
OptTriggerGroup::buildTempTableScanNodes(OptColumnMapping& colMapping, 
					 BindWA           *bindWA)
{
  // Create a temp table object (derived from ChangesTable).
  TriggersTempTable tempTableObj(backbone_->getTriggeringNode(), bindWA);

  // Make the temp table object use the UniqueExecuteID we already have.
  tempTableObj.setBoundExecId(backbone_->createNewExecIdRef());

  if (colMapping.isNewNeeded() && colMapping.isOldNeeded())
  {
    return tempTableObj.buildOldAndNewJoin();
  }

  if (colMapping.isNewNeeded())
  {
    NAString newCorrName(NEWCorr);
    RelExpr *newSubTree = NULL;

    // The NEW values are needed, so create a Scan node on the inserted rows 
    // and call it @NEW.
    tempTableObj.addCorrelationName(newCorrName);
    newSubTree = tempTableObj.buildScan(ChangesTable::INSERTED_ROWS);
    backbone_->forceCardinalityAsIud(newSubTree);
    CMPASSERT(!colMapping.isOldNeeded())
    return newSubTree;
  }
  else
  {
    NAString oldCorrName(OLDCorr);
    RelExpr *oldSubTree = NULL;

    CMPASSERT(colMapping.isOldNeeded())
    // The OLD values are needed, so create a Scan node on the deleted rows 
    // and call it @OLD.
    tempTableObj.addCorrelationName(oldCorrName);
    oldSubTree = tempTableObj.buildScan(ChangesTable::DELETED_ROWS);
    backbone_->forceCardinalityAsIud(oldSubTree);
    CMPASSERT(!colMapping.isNewNeeded())
    return oldSubTree;
  }
}

//-----------------------------------------------------------------------------------
//			Methods For Class OptTriggersBackbone
//-----------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
//
//
// ~OptTriggersBackbone
//
//-----------------------------------------------------------------------------------

OptTriggersBackbone::~OptTriggersBackbone()
{
  if (triggerList_)
  {
    for (CollIndex i = 1; i < triggerList_->entries(); i++)
    {
      OptTriggerPtr optTrigger = (*triggerList_)[i];
      // exclude coverage - we don't seem to handle macro expansion very well
      NADELETEBASIC(optTrigger, CmpCommon::statementHeap());
    }
    NADELETEBASIC(triggerList_, CmpCommon::statementHeap());
  }

  if (triggerGroups_)
  {
    for (CollIndex i = 1; i < triggerGroups_->entries(); i++)
    {
      OptTriggerGroup *optGroup = (*triggerGroups_)[i];
      NADELETE(optGroup, OptTriggerGroup, (CmpCommon::statementHeap()));
    }
    NADELETEBASIC(triggerGroups_, CmpCommon::statementHeap());
  }

}

//-----------------------------------------------------------------------------------
//
//
// init
//
// this method scans the tree, and tags the key nodes in it for further process.
//
//-----------------------------------------------------------------------------------

void OptTriggersBackbone::init()
{
  BindWA *bindWA = (drivingBackbone_->getRETDesc())->getBindWA();
 
  pNext_ = drivingBackbone_;

  initBeforeTriggerDrivingNode();
  initTempDeleteDrivingNodeAndAdvance();
  initStatmentTriggerDrivingNodeAndAdvance();
  initPipeLineDrivingNode();
  if (pNext_->getInliningInfo().isDrivingPipelinedActions())
  {
    initRowTriggerDrivingNode();
    initRiDrivingNode();
    advanceInTree();
  }

  if (bindWA->getHostArraysArea() && !bindWA->getHostArraysArea()->getTolerateNonFatalError()) 
    {
      // If there are no before triggers, temp insert must exist
      CMPASSERT(beforeTriggersExist_ || pNext_->getInliningInfo().isDrivingTempInsert());
    }

  if (pNext_->getInliningInfo().isDrivingTempInsert())
  {
    tempInsertDrivingNodeParent_ = pParent_;
    tempInsertDrivingNode_ = pNext_;
    CMPASSERT(tempInsertDrivingNode_->getOperator().match(REL_ANY_TSJ))
    advanceInTree();
  }

  // Initialize iudDrivingNode_, that includes also IM if exists.
  iudDrivingNodeParent_ = pParent_;
  iudDrivingNode_ = pNext_;

  // Initialize the triggering IUD node
  iudNode_ = findLeftmostGU(iudDrivingNode_);
  CMPASSERT(iudNode_->getOperator().match(REL_ANY_GEN_UPDATE))

  // Initialize the triggerBindInfo_ member
  triggerBindInfo_ = iudNode_->getInliningInfo().getTriggerBindInfo();
  CMPASSERT(triggerBindInfo_ != NULL)
}

//-----------------------------------------------------------------------------------
//
//
// getTransformedTree
//
// transform the tree and return the upper node
// the upper node might change if the temp delete have been removed
//
//-----------------------------------------------------------------------------------

RelExpr *
OptTriggersBackbone::getTransformedTree()
{
  RelExpr *backboneRoot =
    (hasBeforeTriggers() ? beforeTriggerDrivingNode_ : tempDeleteDrivingNode_);

  //
  // If there are NO conflicts in the access set (of IUD, IM, RI and after triggers)
  // Do minimal transformation, end exit
  //
  
  if (!hasConflicts())
  {
    // if there are no before triggers remove temp insert and temp delete
    if (!hasBeforeTriggers())
    {
      removeTempInsert();
      backboneRoot = removeTempDelete();
    }

    if (pipeLineDrivingNode_)
    {
      //  pipelined action sub tree is created in a fixed form with dummy actions 
      //  replacing non-exist subtrees like RI or Row Triggers. Dummy actions 
      //  need to be cleaned.
      RelExpr *res = cleanDummies((RelExpr *)pipeLineDrivingNode_->getChild(1));

      // if there are no pipelined action, eliminate the pipeLineDrivingNode_
      if (res == NULL){
	// remove pipline actions
	pipeLineDrivingNodeParent_->setChild(0, pipeLineDrivingNode_->getChild(0));
	pipeLineDrivingNodeParent_->getGroupAttr()->clearLogProperties();
	pipeLineDrivingNodeParent_->synthLogProp();
      }
      else if (res != (RelExpr *)pipeLineDrivingNode_->getChild(1))
      {
	pipeLineDrivingNode_->setChild(1, res); // new child -reconnect
	pipeLineDrivingNode_->getGroupAttr()->clearLogProperties();
	pipeLineDrivingNode_->synthLogProp();
      }
    }

    return(backboneRoot);
  }

  createTriggerList();
  reorder();
  group();
  backboneRoot = toTree();

  fixCardinalityAndCollectTempUsage(backboneRoot,TRUE);
  fixTempInsertSubtree();

#ifdef TempTableDebug
  //
  // if you need to see the temp table data for debug, define TempTableDebug
  // you will have to delete temp table data manually between every two calls  
  backboneRoot = removeTempDelete();
#endif

  return(backboneRoot);
}

//-----------------------------------------------------------------------------------
//
//	hasConflicts
//
// Are there any conflicts in the IUD + after triggers backbone
//-----------------------------------------------------------------------------------

NABoolean
OptTriggersBackbone::hasConflicts() const
{
  if (statmentTriggerDrivingNode_)
  {
    if (statmentTriggerDrivingNode_->getAccessSet0()->
      isConflicting(statmentTriggerDrivingNode_->getAccessSet1()))
      return TRUE;

    if (leftTreeHasConflicts(statmentTriggerDrivingNode_))
      return TRUE;
  }

  if (pipeLineDrivingNode_)
  {
    if (pipeLineDrivingNode_->getAccessSet0()->
      isConflicting(pipeLineDrivingNode_->getAccessSet1()))
      return TRUE;

    if (rowTriggerDrivingNode_)
    {
      // conflict with RI
      if (rowTriggerDrivingNode_->getAccessSet0()->
	isConflicting(rowTriggerDrivingNode_->getAccessSet1()))
	return TRUE;

      if (leftTreeHasConflicts(rowTriggerDrivingNode_))
	return TRUE;
    }
    //
    // add RI internal conflicts check here (leftTreeHasConflicts(riDrivingNode_))
    //
  }
  return FALSE;
}

// Are there conflicts in the left linear tree?
NABoolean
OptTriggersBackbone::leftTreeHasConflicts(RelExpr *drivingNode) const
{
  for (RelExpr *pUnion = (RelExpr *)drivingNode->getChild(1);
       pUnion;
       pUnion = (RelExpr *)pUnion->getChild(0))
  {
    // a trigger - end of sub tree
    if (pUnion->getInliningInfo().isTriggerRoot())
      return FALSE ;
  
    // verify that it is a left linear tree
    CMPASSERT(pUnion->getOperatorType() == REL_UNION);
    CMPASSERT(((RelExpr *)(pUnion->getChild(1)))->getInliningInfo().isTriggerRoot())

    if (pUnion->getAccessSet0()->isConflicting(pUnion->getAccessSet1()))
      return TRUE;
  }
  return FALSE;
}

//-----------------------------------------------------------------------------------
//
//	createTriggerList
//
// create a trigger list from statement triggers and row triggers
//-----------------------------------------------------------------------------------

void
OptTriggersBackbone::createTriggerList()
{
  triggerList_ = new(CmpCommon::statementHeap())(OptTriggerList);

  if (statmentTriggerDrivingNode_)
    addToTriggerList(statmentTriggerDrivingNode_, FALSE);

  if (rowTriggerDrivingNode_)
    addToTriggerList(rowTriggerDrivingNode_, TRUE);
}

//-----------------------------------------------------------------------------------
//
//
//		removeTempInsert
//
//-----------------------------------------------------------------------------------

void
OptTriggersBackbone::removeTempInsert()
{
  tempInsertDrivingNodeParent_->setChild(0, tempInsertDrivingNode_->getChild(0));
  tempInsertDrivingNodeParent_->getGroupAttr()->clearLogProperties();
  tempInsertDrivingNodeParent_->synthLogProp();
}

//-----------------------------------------------------------------------------------
//
//
//		removeTempDelete
//
//-----------------------------------------------------------------------------------

RelExpr *
OptTriggersBackbone::removeTempDelete()
{
  if (hasBeforeTriggers())
  {
    //The removal of temp delete in before trigger is used only as debug option
    beforeTriggerDrivingNode_->
      setChild(1,(ExprNode *)tempDeleteDrivingNode_->getChild(0));
    return beforeTriggerDrivingNode_;
  }
  return ((RelExpr *)tempDeleteDrivingNode_->getChild(0));
}

//-----------------------------------------------------------------------------------
//
//	addToTriggerList
//
// locate all triggers under a drivingNode and add them to triggerList_
// The tree of unions under  statmentTriggerDrivingNode_ or rowTriggerDrivingNode_ is 
// a left linear tree
//
//-----------------------------------------------------------------------------------

void
OptTriggersBackbone::addToTriggerList(RelExpr *drivingNode, NABoolean isRowTrigger)
{
  OptTriggerPtr trigger;
  SubTreeAccessSet *treeAccessSet;
  RelExpr *parent = drivingNode;

  for (RelExpr *pUnion = (RelExpr *)drivingNode->getChild(1);
       pUnion;
       pUnion = (RelExpr *)pUnion->getChild(0))
  {
    if (pUnion->getInliningInfo().isTriggerRoot())
    {
      // a trigger - end of the left linear tree
      if (pUnion == (RelExpr *)drivingNode->getChild(1)) //single trigger
	treeAccessSet = parent->getAccessSet1();
      else
	treeAccessSet = parent->getAccessSet0();

      trigger = new(CmpCommon::statementHeap()) 
        OptTrigger(treeAccessSet, isRowTrigger, pUnion, NULL);
      triggerList_->insert(trigger);
      break;
    }

    // verify that it is a left linear tree
    CMPASSERT(pUnion->getOperatorType() == REL_UNION);
    CMPASSERT(((RelExpr *)pUnion->getChild(1))->getInliningInfo().isTriggerRoot());

    // set OptTrigger where trigger root is the right child of pUnion
    treeAccessSet = pUnion->getAccessSet1();
    trigger = new(CmpCommon::statementHeap()) 
      OptTrigger(treeAccessSet, 
		 isRowTrigger, 
		 (RelExpr *)pUnion->getChild(1), pUnion);
    triggerList_->insert(trigger);

    parent = pUnion;
  }
}	

//-----------------------------------------------------------------------------------
//
//	optTriggerTimeCompare
//
// used in qsort
//
// Return Value
// -1 elem1 less than    elem2
//  0 elem1 equivalent   elem2
//  1 elem1 greater than elem2
//-----------------------------------------------------------------------------------

static Int32 optTriggerTimeCompare(const void *elem1, const void *elem2)
{
  Int64 timestamp1 = (*(OptTriggerPtr *)elem1)->getTriggerTimeStamp();
  Int64 timestamp2 = (*(OptTriggerPtr *)elem2)->getTriggerTimeStamp();

  if (timestamp1 < timestamp2) 
    return -1;
  if (timestamp1 == timestamp2) 
    return 0;
  return 1;
}

//-----------------------------------------------------------------------------------
//	reorder
//
// set a new final order to the triggers, the order is based on timestamp and conflicts
//-----------------------------------------------------------------------------------

void			
OptTriggersBackbone::reorder()
{
  CollIndex idx;
#pragma nowarn(1506)   // warning elimination 
  Int32 triggerArraySize = triggerList_->entries();
#pragma warn(1506)  // warning elimination 
  // optTriggerPtrArray - array of pointers to OptTrigger , used for qsort
  OptTriggerPtr *optTriggerPtrArray = new(CmpCommon::statementHeap()) 
    OptTriggerPtr[triggerArraySize];
  // copy triggerList to triggerArray
  for (idx = 0; idx < triggerList_->entries(); idx++)
    optTriggerPtrArray[idx] = (*triggerList_)[idx];

  // sort trigger array by timestamp
  opt_qsort(optTriggerPtrArray,
	triggerArraySize, 
	sizeof(OptTriggerPtr), 
	optTriggerTimeCompare);

  // move row triggers to the begining, as far as the access set conflicts allow
  for (Int32 i = 1 ; i< triggerArraySize; i++)
  {
    // if it is a statment trigger, don't try to move it
    if (!optTriggerPtrArray[i]->isRowTrigger())
      continue;

    // if row trigger try to move it to the begining of the list
    for (Int32 j = i ; j > 0 ; j--)
    {
      // if trigger j don't conflict with trigger j-1 replace them			
      const SubTreeAccessSet *accessSet1 = optTriggerPtrArray[j]->getAccessSet();
      const SubTreeAccessSet *accessSet2= optTriggerPtrArray[j-1]->getAccessSet();

      if (accessSet1->isConflicting(accessSet2))
	break; // Conflict - can't move the current trigger any more

      // move the row trigger one step back
      OptTriggerPtr sav = optTriggerPtrArray[j-1];
      optTriggerPtrArray[j-1] = optTriggerPtrArray[j];
      optTriggerPtrArray[j] = sav;
    }
  }

  // set triggerList_ based on the reordered array optTriggerPtrArray
  triggerList_->clear();
  for (Int32 k = 0 ; k < triggerArraySize; k++)
    triggerList_->insert(optTriggerPtrArray[k]);

  // free the temporary array
  // exclude coverage - we don't seem to handle macro expansion very well
  NADELETEBASIC(optTriggerPtrArray, (CmpCommon::statementHeap()));
}

//-----------------------------------------------------------------------------------
//
//	group
//
// Divide the triggers in the given order to groups 
// Triggers in one group should have no conflicts between them and thats why the order  
//	inside a group is not important 
//-----------------------------------------------------------------------------------

void
OptTriggersBackbone::group()
{
  // 
  // First, Second and third groups are one logical group that does not have conflicts  
  // between the members of the group.
  //
  // if a trigger don't conflict with the IUD & RI
  // {
  //  row triggers go to first group (pPipelinedGroup)
  //  statment triggers go to second group (pIudParallelGroup)
  // }
  // else
  // {
  //  the trigger goes to the third group which is a blocked group that read from 
  //	temporary table
  // }

  // iudAccessSet represents the access set of the sub tree below the triggers and RI
  SubTreeAccessSet *iudAccessSet;
  SubTreeAccessSet *riAccessSet = NULL;

  if (tempInsertDrivingNode_)
    iudAccessSet = tempInsertDrivingNodeParent_->getAccessSet0();
  else
    iudAccessSet = iudDrivingNodeParent_->getAccessSet0();

  // RI if exist are always in the left side of the union 
  if (riDrivingNode_) 
    riAccessSet = riDrivingNode_->getAccessSet0();
  
  OptTriggerGroup *pPipelinedGroup = new(CmpCommon::statementHeap()) 
    OptTriggerGroup(OptTriggerGroup::PIPELINED_ROW_TRIGGERS, this);
  OptTriggerGroup *pIudParallelGroup = new(CmpCommon::statementHeap()) 
    OptTriggerGroup(OptTriggerGroup::IUD_PARALLEL_STATEMENT_TRIGGER, this);
  OptTriggerGroup *pBlockedGroup = new(CmpCommon::statementHeap()) 
    OptTriggerGroup(OptTriggerGroup::BLOCKED_GROUP, this);

  triggerGroups_ = new(CmpCommon::statementHeap())(OptTriggerGroupList);

  // the for loop stops on the first trigger conflicting with any 
  // previous trigger
  CollIndex i = 0;
  for (i = 0; i < triggerList_->entries(); i++)
  {
    OptTriggerPtr currentTrigger = (*triggerList_)[i];

    // If current trigger conflicts with group1 or group2 or group3 
    // we wil start a new group.
    if (pPipelinedGroup->isConflicting(currentTrigger->getAccessSet()) ||
	pIudParallelGroup->isConflicting(currentTrigger->getAccessSet()) ||
	pBlockedGroup->isConflicting(currentTrigger->getAccessSet()))
	break;

    // If current trigger conflicts with IUD, Temp Table, IM or RI 
    //	add it to the third group 
    // else 
    //	add it to the first or second group 
    if (currentTrigger->getAccessSet()->isConflicting(iudAccessSet) ||
	(riAccessSet && 
	 currentTrigger->getAccessSet()->isConflicting(riAccessSet)))
    {
      pBlockedGroup->addTrigger(currentTrigger);
    }
    else
    {	
      if (currentTrigger->isRowTrigger())
	pPipelinedGroup->addTrigger(currentTrigger);
      else
	pIudParallelGroup->addTrigger(currentTrigger);
    }
  }

  // if there are elements in group1/group2/group3 add them to group list
  if (pPipelinedGroup->entries())
    triggerGroups_->insert(pPipelinedGroup);
  if (pIudParallelGroup->entries())
    triggerGroups_->insert(pIudParallelGroup);	


  OptTriggerGroup *currentGroup;
  if (pBlockedGroup->entries())
    currentGroup = pBlockedGroup;	
  else
  {
    // create the blocked groups
    currentGroup = new(CmpCommon::statementHeap()) 
      OptTriggerGroup(OptTriggerGroup::BLOCKED_GROUP, this);
  }

  // this for loop continues from the trigger that caused the end of the previous 
  // for loop
  CollIndex j = i;
  for (j = i; j < triggerList_->entries(); j++)
  {
    OptTriggerPtr currentTrigger = (*triggerList_)[j];
    // If current trigger conflict with the currentGroup
    // close the current group and open a new one
    if (currentGroup->isConflicting(currentTrigger->getAccessSet()))
    {
      triggerGroups_->insert(currentGroup);
      // triggers are sepearated to new group whenever there is a conflict betweeen 
      // the next trigger to the current group
      currentGroup = new(CmpCommon::statementHeap()) 
	OptTriggerGroup(OptTriggerGroup::BLOCKED_GROUP, this);
    }
    // insert to current group
    currentGroup->addTrigger(currentTrigger);
  }

  // if there are elements in currentGroup add it to group list
  if (currentGroup->entries())
    triggerGroups_->insert(currentGroup);	
}

//-----------------------------------------------------------------------------------
//
//	cleanDummies
//
//  pipelined action sub tree is created in a fixed form with dummy action replacing 
//	empty subtrees like RI or Row triggers.
//	clean union sub tree, if both children are dummy the sub tree is dummy
//		if one of the children is dummy, the other child replace the union
//-----------------------------------------------------------------------------------

RelExpr	*
OptTriggersBackbone::cleanDummies(RelExpr *parent)
{
  RelExpr *leftChild = (RelExpr *)parent->getChild(0);
  RelExpr *rightChild = (RelExpr *)parent->getChild(1);


  if (leftChild->getInliningInfo().isDummy() && 
      rightChild->getInliningInfo().isDummy())
    return NULL;

  if (!leftChild->getInliningInfo().isDummy() && 
      !rightChild->getInliningInfo().isDummy())
    return parent;

  if (leftChild->getInliningInfo().isDummy())
    return rightChild;
  return leftChild;
}

//-----------------------------------------------------------------------------------
//
//
//
// Replace the original row trigger sub tree under pipelined actions  
// with the row triggers sub set that the transformation found qualified to be placed
// in the pipelined actions
//
//-----------------------------------------------------------------------------------

void
OptTriggersBackbone::replacePipelinedRowTriggers(RelExpr *rowTriggerTree)
{
  // connect the piplined row triggers to tree in rowTriggerDrivingNode_
  rowTriggerDrivingNode_->setChild(1, rowTriggerTree);

  OptTriggersBackbone::SetUnionCharacteristicInputs(rowTriggerDrivingNode_);

  rowTriggerDrivingNode_->getGroupAttr()->clearLogProperties();
  rowTriggerDrivingNode_->synthLogProp();

  OptTriggersBackbone::SetTsjCharacteristicInputs(pipeLineDrivingNode_);

  pipeLineDrivingNode_->getGroupAttr()->clearLogProperties();
  pipeLineDrivingNode_->synthLogProp();

}

//-----------------------------------------------------------------------------------
//	toTree
//
//	write the final backbone sub tree
//-----------------------------------------------------------------------------------

RelExpr	*
OptTriggersBackbone::toTree()
{
  // Rebuild the tree bottom up
  RelExpr *childNode = iudDrivingNode_;

  if (tempInsertDrivingNode_)
    childNode = tempInsertDrivingNode_;

  if (pipeLineDrivingNode_)
  {
    childNode = pipeLineDrivingNode_;

    // if first group is not PIPELINED_ROW_TRIGGERS  
    //		mark the original row triggers sub tree for removal
    if (rowTriggerDrivingNode_ && !(*triggerGroups_)[0]->isPipeLinedRowTriggesGroup())
    {
      RelExpr *rowTriggers = (RelExpr *)rowTriggerDrivingNode_->getChild(1);
      rowTriggers->getInliningInfo().setFlags(II_DummyStatement);			
    }
  }

  for (CollIndex i = 0; i < triggerGroups_->entries(); i++)
  {
    OptTriggerGroup *optTriggerGroup = (*triggerGroups_)[i]; 

    // if first group isPipeLinedRowTriggesGroup
    if (i==0 && optTriggerGroup->isPipeLinedRowTriggesGroup())
    {
      // Get the new row trigger sub tree, that belong to the pipelined actions
      RelExpr *rowTriggerTree = optTriggerGroup->toTree();
      replacePipelinedRowTriggers(rowTriggerTree);
      continue;
    }

    // group with union or ordered union on top of it
    RelExpr *triggersUnion = optTriggerGroup->toTree();
    Union *pOrder = new(CmpCommon::statementHeap())Union
      (childNode, triggersUnion, NULL, NULL, REL_UNION,
       CmpCommon::statementHeap(),TRUE);
    if (optTriggerGroup->isBlockedGroup())
      pOrder->setOrderedUnion();

    BindWA *bindWA = (drivingBackbone_->getRETDesc())->getBindWA();

    if (bindWA && bindWA->getHostArraysArea() && 
        bindWA->getHostArraysArea()->getTolerateNonFatalError()) 
    {
      pOrder->setInNotAtomicStatement();
    }

    pOrder->setGroupAttr(new(CmpCommon::statementHeap()) GroupAttributes());
    OptTriggersBackbone::SetUnionCharacteristicInputs(pOrder);
    pOrder->synthLogProp();
    childNode = pOrder;

    // we perform cleanDummies in the first group after pipeline
    // because if the pipelined action subtree is empty this group is the  
    // new parent of the pipeline left child
    if (pOrder->getChild(0) == pipeLineDrivingNode_)
    {
      //  Pipelined action sub tree is created in a fix form with dummy  
      //  action replacing empty subtrees like RI or Row triggers. 
      //  Dummy actions need to be cleaned.
      RelExpr *res = 
	cleanDummies((RelExpr *)pipeLineDrivingNode_->getChild(1));
      // if piplined actions empty, remove the pipeline Tsj
      if (res == NULL)
      {
	ValueIdSet emptyIdSet;
	RelExpr *child = (RelExpr *)pipeLineDrivingNode_->getChild(0);
	pOrder->setChild(0, child);

	// If pipelined actions are eliminated
	// The output from its first child is no longer needed
	// Change child from left join to join and clear its 
	// characteristic outputs


	if (child->getOperatorType() == REL_LEFT_TSJ)
	  child->setOperatorType(REL_TSJ);
	else if (child->getOperatorType() == REL_LEFT_JOIN)
	  child->setOperatorType(REL_JOIN);

	child->getGroupAttr()->setCharacteristicOutputs(emptyIdSet);

	// Generic Update
	if (child->getOperator().match(REL_ANY_GEN_UPDATE))
	  ((GenericUpdate *)child)->setPotentialOutputValues(emptyIdSet);

	pOrder->getGroupAttr()->clearLogProperties();
	pOrder->synthLogProp();
      }
      else if (res != pipeLineDrivingNode_->getChild(1))
      {
	pipeLineDrivingNode_->setChild(1, res);

	pipeLineDrivingNode_->getGroupAttr()->clearLogProperties();
	pipeLineDrivingNode_->synthLogProp();
      }
    }
  }

  // reconnect the triggers sub tree to the temp delete tsj
  tempDeleteDrivingNode_->setChild(0, (ExprNode *)childNode);
  


  // return the top node of the backbone
  if (hasBeforeTriggers())
    return (beforeTriggerDrivingNode_);
  return (tempDeleteDrivingNode_);
}

//-----------------------------------------------------------------------------------
//
// forceCardinalityAsIud
//
// Force the cardinality of the given node to be the same as the original IUD that
// is the basis for the whole backbone.
//
//-----------------------------------------------------------------------------------

void OptTriggersBackbone::forceCardinalityAsIud(RelExpr *expr) const
{
#pragma nowarn(1506)   // warning elimination 
#pragma warning (disable : 4244)   //warning elimination

  CostScalar card =
    getIudDrivingNode()->getGroupAttr()->getResultCardinalityForEmptyInput();
  expr->getInliningInfo().BuildForceCardinalityInfo(1.0, // factor ignored
						    card.getValue(),
						    CmpCommon::statementHeap());
#pragma warning (default : 4244)   //warning elimination
#pragma warn(1506)  // warning elimination 
}

//-----------------------------------------------------------------------------------
//
// createNewExecIdRef
//
// This method returns the column reference for the virtual column used in selection
// from the temp-table. The BindWA is prepared in a way that this column name appears
// with the right ValueId.
//
//-----------------------------------------------------------------------------------

ColReference *OptTriggersBackbone::createNewExecIdRef()
{
  ColRefName *execIdName = new(CmpCommon::statementHeap())
    ColRefName(InliningInfo::getExecIdVirtualColName());
  return new(CmpCommon::statementHeap()) ColReference(execIdName);
}

//-----------------------------------------------------------------------------------
//	fixCardinalityAndCollectTempUsage
//
//  fix all temp-table access nodes to use the same cardinality as the
//  iudDrivingNode.
//  Recursively scan the final tree, and find each node that access the
//  temp-table. For each such node, fix the cardinality to be the same as the
//  iudDrivingNode. The scanning doesn't include the subtree that includes the
//  insert node into the temp-table.
//  In addition, the usage of the temp-table is collected for each scan node, so
//  later we'll know whether the insert into the temp-table might be optimized.
//-----------------------------------------------------------------------------------

void OptTriggersBackbone::fixCardinalityAndCollectTempUsage(RelExpr *rootNode,
							    NABoolean enterInnerBackbone)
{
  // stop condition
  // we stop the recursion on the following conditions:
  // 1. we reached the end of tree
  // 2. we reached the temp-insert subtree. This sub-tree is not scanned since the
  //    cardinality usually set correctly by the optimizer according to the driving
  //    IUD node. In update operations the insert is fed by a union of tuples, so
  //    setting the cardinality is not that straight forward, so we avoid it.
  if (rootNode == NULL || rootNode == tempInsertDrivingNode_)
  {
    return;
  }

  // find whether rootNode is a root of a backbone of triggers.
  InliningInfo &info = rootNode->getInliningInfo();
  NABoolean backboneRoot = FALSE;
  if (info.isDrivingBeforeTriggers() ||
      (info.isDrivingTempDelete() && !info.isBeforeTriggersExist()))
  {
    backboneRoot = TRUE;
  }

  // don't enter encountered backbone if instructed not to do so.
  if (!enterInnerBackbone && backboneRoot)
  {
    // if instructed not to enter inner backbones and we've reached such
    // backbone, do the job only for the IUD node that is the basis for
    // this inner backbone, since this is the only place in the inner
    // backbone where the temp-table values of the current backbone might
    // be used. Other sub-trees will use the new values generated in this
    // inner backbone.
    GenericUpdate *iudNode = findLeftmostGU(rootNode);
    fixCardinalityAndCollectTempUsage(iudNode, FALSE);
    return; // skip inner backbone - stop recursion
  }

  // handle scan nodes on temp-table
  if (rootNode->getOperatorType() == REL_SCAN)
  {
    Scan *scan = (Scan *) rootNode;
    if (scan->getTableName().getSpecialType() == ExtendedQualName::TRIGTEMP_TABLE)
    {
      forceCardinalityAsIud(scan);
      collectTempTableUsage(scan);
    }
  }

  // handle GU nodes on temp-table
  if (rootNode->getOperator().match(REL_ANY_GEN_UPDATE))
  {
    GenericUpdate *gu = (GenericUpdate *) rootNode;
    if (gu->getTableName().getSpecialType() == ExtendedQualName::TRIGTEMP_TABLE)
    {
      forceCardinalityAsIud(gu);
    }
  }

  // recursive call for each child
  for (Int32 i = 0; i < rootNode->getArity(); i++)
  {
    fixCardinalityAndCollectTempUsage(rootNode->getChild(i)->castToRelExpr(), FALSE);
  }

}

//-----------------------------------------------------------------------------------
//	collectTempTableUsage
//
//	This method collects information how the temp-table is actually used. It
//      looks for a predicate on the "@UNIQUE_IUD_NO" special column of the
//      temp-table. If the predicate found, if looks at the constant value this column
//      is compared against (the constant must exist and be numric). If the value is
//      even, the selected rows from the temp-table will contain the NEW values,
//      otherwise it will contain the OLD values. This way we find the actual usage of
//      the values from the temp-table.
//-----------------------------------------------------------------------------------
void OptTriggersBackbone::collectTempTableUsage(RelExpr *node)
{
  ValueIdSet preds = node->selectionPred();
  for (ValueId vid = preds.init(); preds.next(vid); preds.advance(vid))
  {
    if (vid.getItemExpr()->getOperatorType() != ITM_VEG_PREDICATE)
    {
      continue; // only VEG predicates are inspected
    }

    VEG *referencedVEG = ((VEGPredicate*)vid.getItemExpr())->getVEG();
    ValueIdSet valuesInVEG = referencedVEG->getAllValues();

    // look for the UNIQUEIUD_COLUMN base column in the VEG
    for (ValueId vid2 = valuesInVEG.init();
	 valuesInVEG.next(vid2);
	 valuesInVEG.advance(vid2))
    {
      if (vid2.getItemExpr()->getOperatorType() != ITM_BASECOLUMN)
      {
	continue; // look only at base columns
      }

      BaseColumn *bc = vid2.castToBaseColumn();
      if (!bc->getColName().compareTo(UNIQUEIUD_COLUMN))
      {
	// the current VEG is the desired predicate
	setUsageAccordingToUniqueUidPredicate(valuesInVEG);
      }
    }
  }
}

//-----------------------------------------------------------------------------------
//	setUsageAccordingToUniqueUidPredicate
//
//	The given predicate is using the UNIQUEIUD_COLUMN column. This predicate
//      controls which rows actually fetched from the temp-table, so it gives us the
//      knowledge which rows in the temp-table are actually used.
//	The predicate containing the UNIQUEIUD_COLUMN column must also contain a
//      constant unmeric value that it is compared against. The constant might be
//      referenced directly or indirectly by inner VEG reference (for example).
//      Selection of rows with even value means fetching NEW values, while selection
//      of rows with odd value means fetching OLD values.
//-----------------------------------------------------------------------------------
void OptTriggersBackbone::setUsageAccordingToUniqueUidPredicate(const ValueIdSet &uniqueUidPred)
{
  ItemExpr *constValue = NULL;
  uniqueUidPred.referencesAConstValue(&constValue);

  // This predicate must reference a numeric constant value
  CMPASSERT(constValue != NULL && constValue->getOperatorType() == ITM_CONSTANT);
  CMPASSERT(((ConstValue *)constValue)->canGetExactNumericValue());

  Int64 uniqueUidValue = ((ConstValue *)constValue)->getExactNumericValue();

  // if constant is odd - this scan fetches OLD values, otherwise it fetches NEW values
  if (uniqueUidValue % 2)
  {
    tempTableUsage_ |= ChangesTable::DELETED_ROWS;
  }
  else
  {
    tempTableUsage_ |= ChangesTable::INSERTED_ROWS;
  }
}

//-----------------------------------------------------------------------------------
//	fixTempInsertSubtree
//
//	In case only one set of values (NEW or OLD) from the temp-table is in use,
//      there's no point inserting the other set from the beginning. Since the
//      insertion of both sets has bad affection on performance, we try as much as
//      we could to avoid it when not really necessary.
//-----------------------------------------------------------------------------------

void OptTriggersBackbone::fixTempInsertSubtree()
{
BindWA *obindWA = (drivingBackbone_->getRETDesc())->getBindWA();
 if ((tempTableUsage_ == (ChangesTable::INSERTED_ROWS | ChangesTable::DELETED_ROWS)) || (obindWA->getHostArraysArea() && !obindWA->getHostArraysArea()->getTolerateNonFatalError()) )
  {
    return; // both NEW and OLD sets are used - nothing to optimize
  }

  // Create a new BindWA - we don't want to have anything left in some RETDesc
  // that may affect this binding.
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());

  // Prepare the new BindWA for binding our little sub-tree.
  // In addition, add the OLD and NEW column list from the original RETDesc, so
  // these columns will be available for the temp-table insert node during bind
  // time.
  OptTriggersBackbone::PrepareNewBindWA(bindWA, triggerBindInfo_);
  RETDesc *retDesc = bindWA.getCurrentScope()->getRETDesc();
  retDesc->addColumns(&bindWA,
		      *(triggerBindInfo_->getIudColumnList()),
		      USER_AND_SYSTEM_COLUMNS);

  // build the optimized version of the tmep-insert sub-tree
  RelExpr *topNode = buildOptimizedTempInsert(&bindWA);

  // Since this is a compilation with GenericUpdate node, we must mark it
  // as internal compilation, so no real root will be searched
  CmpContext::InternalCompileEnum savedMode =
    CmpCommon::context()->internalCompile();
  CmpCommon::context()->internalCompile() = CmpContext::INTERNAL_MODULENAME;

  // Do the necessary compilation steps.
  topNode = OptTriggersBackbone::BindAndNormalizeNewlyConstructedTree(&bindWA, topNode);

  // return to the mode we were in before the above compilation
  CmpCommon::context()->internalCompile() = savedMode;

  // Find the node that drives the temp-insert subtree.
  if (obindWA->getHostArraysArea() && !obindWA->getHostArraysArea()->getTolerateNonFatalError()) 
    {
      RelExpr *insertDrivingNode = tempInsertDrivingNode_;
      if (insertDrivingNode == NULL)
	{
	  CMPASSERT(beforeTriggersExist_);
	  insertDrivingNode = beforeTriggerDrivingNode_->child(0)->child(1);
	  CMPASSERT(insertDrivingNode->getOperatorType() == REL_UNARY_INSERT ||
		    insertDrivingNode->getOperatorType() == REL_LEAF_INSERT);
	}

      // connect sub-tree to the backbone tree
      insertDrivingNode->setChild(1, topNode);
      insertDrivingNode->getGroupAttr()->clearLogProperties();
      insertDrivingNode->synthLogProp();
    }
}

//-----------------------------------------------------------------------------------
//	buildOptimizedTempInsert
//
//	Here the optimized version of the temp-insert sub-tree is constructed.
//-----------------------------------------------------------------------------------

RelExpr *OptTriggersBackbone::buildOptimizedTempInsert(BindWA *bindWA)
{
  // Create a temp table object (derived from ChangesTable).
  TriggersTempTable tempTableObj(iudNode_, bindWA);

  // Make the temp table object use the UniqueExecuteID we already have.
  tempTableObj.setBoundExecId(createNewExecIdRef());

  RelExpr *topNode = NULL;
  if (tempTableUsage_ & ChangesTable::INSERTED_ROWS)
  {
    topNode = tempTableObj.buildInsert(TRUE, ChangesTable::INSERTED_ROWS);
  }
  else
  {
    topNode = tempTableObj.buildInsert(TRUE, ChangesTable::DELETED_ROWS);
  }
  topNode = new (CmpCommon::statementHeap()) RelRoot(topNode);
  CMPASSERT(topNode); // make sure topNode is pointing to the result tree

  return topNode;
}

//-----------------------------------------------------------------------------
//	initBeforeTriggerDrivingNode
//
//	sets a pointer to the node that drives the before triggers (if exist).
//      This method also sets a flag that before triggers exist.
//-----------------------------------------------------------------------------

void
OptTriggersBackbone::initBeforeTriggerDrivingNode()
{
  if (pNext_->getInliningInfo().isDrivingBeforeTriggers())
  {
    beforeTriggerDrivingNode_ = pNext_;
    beforeTriggersExist_ = TRUE;
  } 
}

//-----------------------------------------------------------------------------
//	initTempDeleteDrivingNode
//
//	sets a pointer to the node that drives the delete from the temp-table.
//-----------------------------------------------------------------------------

void
OptTriggersBackbone::initTempDeleteDrivingNodeAndAdvance()
{
  const InliningInfo &info = pNext_->getInliningInfo();

  // The root must either drive before triggers or temp-table delete 
  CMPASSERT(beforeTriggersExist_ || info.isDrivingTempDelete())

  if (beforeTriggersExist_)
  {
    CMPASSERT(info.isDrivingBeforeTriggers())
    tempDeleteDrivingNode_ = pNext_->getChild(1)->castToRelExpr();
  } 
  else
  {
    tempDeleteDrivingNode_ = pNext_;
  }

  // Sanity check for the pointer
  CMPASSERT(tempDeleteDrivingNode_->getInliningInfo().isDrivingTempDelete())

  pNext_ = tempDeleteDrivingNode_;
  advanceInTree();
}

//-----------------------------------------------------------------------------
//	initStatmentTriggerDrivingNode
//
//	sets a pointer to the node that drives the statement triggers. All the
//      triggers are connected using UNION nodes.
//-----------------------------------------------------------------------------

void
OptTriggersBackbone::initStatmentTriggerDrivingNodeAndAdvance()
{
  if (pNext_->getInliningInfo().isDrivingStatementTrigger())
  {
    statmentTriggerDrivingNode_ = pNext_;
    CMPASSERT(statmentTriggerDrivingNode_->getOperatorType() == REL_UNION);
    advanceInTree();
  }
}

//-----------------------------------------------------------------------------
//	initPipeLineDrivingNode
//
//	sets a pointer to the node that drives all the pipelined actions, i.e.
//      RI, row triggers etc.
//-----------------------------------------------------------------------------

void
OptTriggersBackbone::initPipeLineDrivingNode()
{
  if (pNext_->getInliningInfo().isDrivingPipelinedActions())
  {
    pipeLineDrivingNodeParent_ = pParent_;
    pipeLineDrivingNode_ = pNext_;
  }
}

//-----------------------------------------------------------------------------
//	initRowTriggerDrivingNode
//
//	sets a pointer to the node that drives the row triggers. All the
//      triggers are connected using UNION nodes.
//-----------------------------------------------------------------------------

void
OptTriggersBackbone::initRowTriggerDrivingNode()
{
  CMPASSERT(pNext_->getInliningInfo().isDrivingPipelinedActions())
  RelExpr *e = pNext_->getChild(1)->castToRelExpr();

  if (e->getInliningInfo().isDrivingRowTrigger())
  {
    rowTriggerDrivingNode_ = e;
    CMPASSERT(rowTriggerDrivingNode_->getOperatorType() == REL_UNION);
  }
}

//-----------------------------------------------------------------------------
//	initRiDrivingNode
//
//	sets a pointer to the node that drives the RI. All the RI constraints
//      are connected using UNION nodes.
//-----------------------------------------------------------------------------

void
OptTriggersBackbone::initRiDrivingNode()
{
  CMPASSERT(pNext_->getInliningInfo().isDrivingPipelinedActions())
  RelExpr *e = pNext_->getChild(1)->castToRelExpr();

  if (e->getInliningInfo().isDrivingRI())
  {
    riDrivingNode_ = e;
    CMPASSERT(riDrivingNode_->getOperatorType() == REL_UNION);
  }
}

//-----------------------------------------------------------------------------
//	findLeftmostGU
//
//	find the leftmost GenericUpdate node in the tree. This node is the one
//      that started the whole process (i.e. the triggering node).
//
//      The method scans the given tree in in-oreder style, i.e. left child,
//      current node and then right child. The method looks for the first
//      GenericUpdate node encountered in this order of scanning. First there's
//      a dril down into the left most child. From there on, the method goes
//      back in the tree. If the current inspected node is a GeericUpdate, it
//      is returned. Otherwise, it's right child is scanned also, and if no
//      GenericUpdate is found yet, there's a withdrawing to the parent node.
//
//      The reason for looking into the right childs also is that when before
//      triggers exist, the GenericUpdate is the right child of the EffectiveGU
//      node.
//-----------------------------------------------------------------------------

GenericUpdate *
OptTriggersBackbone::findLeftmostGU(ExprNode *expr)
{
  GenericUpdate *guNode = NULL;
  CMPASSERT(expr)

   if (expr->getOperator().match(REL_ANY_GEN_UPDATE))
   {
     if (((GenericUpdate *) expr)->getUpdateCKorUniqueIndexKey())
       return (GenericUpdate *) expr;

     if (expr->getOperator().match(REL_UNARY_INSERT))
     {
       Insert * ins = (Insert *)((GenericUpdate *) expr);
       if (ins->systemGeneratesIdentityValue())
         return (GenericUpdate *) expr;
     }
  
     if (iudNode_ && iudNode_->getTableName().getSpecialType() == ExtendedQualName::SG_TABLE)
       return (GenericUpdate *) expr;
  }

  if (expr->getChild(0))
  {
    // recursive call to left child
    if ((guNode = findLeftmostGU(expr->getChild(0))) != NULL)
    {
      return guNode;
    }
  }

  if (expr->getOperator().match(REL_ANY_GEN_UPDATE))
  {
    return (GenericUpdate *) expr;
  }

  if (expr->getChild(1))
  {
    // recursive call to right child
    return findLeftmostGU(expr->getChild(1));
  }

  return NULL; // GenericUpdate node not found in entire tree
}

//-----------------------------------------------------------------------------
//	advanceInTree
//
//	advance the current node (and its parent) in the tree. The advancing is
//      always done leftwards.
//-----------------------------------------------------------------------------

void
OptTriggersBackbone::advanceInTree()
{
  pParent_ = pNext_;
  pNext_ = pNext_->getChild(0)->castToRelExpr();
}

//-----------------------------------------------------------------------------------
//			Methods For Class OptTriggerGroup
//-----------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
OptColumnMapping::OptColumnMapping(TriggerBindInfo  *bindInfo, 
				   OperatorTypeEnum  iudOpType,
				   CollHeap         *heap)
  : bindInfo_(bindInfo),
    iudOpType_(iudOpType),
    mappingArray_(heap),
    newCols_(heap),
    oldCols_(heap),
    newCommonCols_(heap),
    oldCommonCols_(heap),
    oldNeeded_(FALSE),
    newNeeded_(FALSE),
    atNew_(NEWCorr, heap),
    atOld_(OLDCorr, heap),
    dummyFeed_(FALSE),
    heap_(heap)
{
}

//-----------------------------------------------------------------------------------
// Initialize and minimize the mapping between the ValueIds expected by the 
// triggers, and the OLD and NEW columns supplied by the new Scans on the temp table.
// At this stage, the new temp scan tree was not constructed yet, so the mappings are
// only done to the OLD and NEW column names.
//-----------------------------------------------------------------------------------
void OptColumnMapping::initializeMappings(ValueIdSet &reqOutput)
{
  // First phase: insert mappings for all the OLD/NEW columns that match the inputs
  // required by the triggers into the newCols_ and oldCols_ lists.
  createInitialMappingLists(reqOutput);

  // Skip the rest of it if the trigger is not using any of these values anyway.
  if (dummyFeed_)
    return;

  // Second phase: remove redundant columns from each lists (columns that are part 
  // of the same VEG). Then find OLD columns that are equal to NEW columns (are part
  // of the same VEG) and move them to the oldCommonCols_ and newCommonCols_ lists.
  findCommonColumns();

  // Third phase: find out if we really need both the OLD and the NEW tables.
  checkWhichTablesAreNeeded();
}

//-----------------------------------------------------------------------------------
// First phase: insert mappings for all the OLD/NEW columns that match the inputs
// required by the triggers into the newCols_ and oldCols_ lists.
// For each column of the original IUD RETDesc, do:
//   If it's ValueId is referenced by the required Inputs of the trigger, do
//     BEGIN
//       Create a mapping object for it.
//       If it's an OLD column - insert it into the oldCols_ list.
//       If it's a  NEW column - insert it into the newCols_ list.
//     END
//-----------------------------------------------------------------------------------
void OptColumnMapping::createInitialMappingLists(ValueIdSet &reqOutput)
{
  // Get the OLD and NEW column list from the original RETDesc.
  const ColumnDescList *topColumnList = bindInfo_->getIudColumnList();
  // These are the inputs required by the triggers of this group.
  ValueIdSet  NeededOutputs(reqOutput);

  for (CollIndex i=0; i<topColumnList->entries(); i++)
  {
    const ColumnDesc *topColDesc = topColumnList->at(i);
    ValueId topValueId = topColDesc->getValueId();

    // Find the VEG in the required inputs that references this column.
    ValueId topVeg;
    if (NeededOutputs.referencesTheGivenValue(topValueId, topVeg))
    {
      const ColRefName &colName = topColDesc->getColRefNameObj();

      // Create a mapping object.
      ColumnMapping *newMapping = new(heap_) ColumnMapping;
      newMapping->colName_    = &colName;
      newMapping->topValueId_ = topValueId;
      newMapping->topVeg_     = topVeg;

      // Check which list to insert the new object into.
      const NAString &corrName = colName.getCorrNameObj().getCorrNameAsString();
      if (corrName == atNew_)
	newCols_.insert(newMapping);
      else if (corrName == atOld_)
	oldCols_.insert(newMapping);
    }
  }

  if (oldCols_.isEmpty() && newCols_.isEmpty())
  {
    // The trigger is not using any of the OLD and NEW values.
    dummyFeed_ = TRUE;

    // Check the IUD operator type. If the Scan is on the wrong table, there
    // will be no rows there to drive the trigger.
    if (iudOpType_ == REL_UNARY_INSERT || iudOpType_ == REL_LEAF_INSERT)
      newNeeded_ = TRUE;
    else
      oldNeeded_ = TRUE;
  }
}

//-----------------------------------------------------------------------------------
// A utility method used by findCommonColumns() to find a mapping in list colsList
// that has the same topVeg_ as colToFind. startAt can be specified to iterativly
// find all such mappings, or to find mappings in the same list as colToFind.
//-----------------------------------------------------------------------------------
OptColumnMapping::ColumnMapping *
OptColumnMapping::findEqualCols(MappingList   &colsList,
				ColumnMapping *colToFind,
				CollIndex      startAt)
{
  for (CollIndex i=startAt; i<colsList.entries(); i++)
  {
    ColumnMapping *currentCol = colsList[i];
    if (colToFind->topVeg_ == currentCol->topVeg_)
      return currentCol;
  }

  return NULL;
}

//-----------------------------------------------------------------------------------
// Second phase: remove redundant columns from each lists (columns that are part 
// of the same VEG). Then find OLD columns that are equal to NEW columns (are part
// of the same VEG) and move them to the oldCommonCols_ and newCommonCols_ lists.
// Both sets are saved for now, because we don't yet know if they will be used 
// as OLD or NEW columns.
//-----------------------------------------------------------------------------------
void OptColumnMapping::findCommonColumns()
{
  CollIndex i;
  ColumnMapping *redundantCol = NULL;
  ColumnMapping *currentCol = NULL;

  // Remove redundant OLD columns
  if (!oldCols_.isEmpty())
  {
    for (i=0; i<oldCols_.entries()-1; i++)
    {
      currentCol = oldCols_[i];

      // Search the mappings after currentCol in the list
      while ((redundantCol = findEqualCols(oldCols_, currentCol, i+1)) != NULL)
	oldCols_.remove(redundantCol);
    }
  }

  // Remove redundant NEW columns
  if (!newCols_.isEmpty())
  {
    for (i=0; i<newCols_.entries()-1; i++)
    {
      currentCol = newCols_[i];

      // Search the mappings after currentCol in the list
      while ((redundantCol = findEqualCols(newCols_, currentCol, i+1)) != NULL)
	newCols_.remove(redundantCol);
    }
  }

  // Move common columns to commonCols
  if (!oldCols_.isEmpty() && !newCols_.isEmpty())
  {
    for (i=0; i<oldCols_.entries(); i++)
    {
      ColumnMapping *oldCol = oldCols_[i];

      // Search the entire other list.
      ColumnMapping *newCol = findEqualCols(newCols_, oldCol, 0);
      if (newCol != NULL)
      {
	// We keep both common columns for now, because even though they are
	// equal, their mapping object is not identical - the name and ValueId
	// are different, and are used later. Only one of the two common lists
	// will actually be used later, but we don't yet know which one.
	newCommonCols_.insert(newCol);
	oldCommonCols_.insert(oldCol);
	newCols_.remove(newCol);
	oldCols_.remove(oldCol);

	// Compensate for entry number i, that was just deleted from this list.
	i--;
      }
    }
  }
}

//-----------------------------------------------------------------------------------
// Third phase: find out if we really need both the OLD and the NEW tables.
// Even an Update trigger that uses both OLD and NEW values can be optimized to 
// use just one temp Scan node, if all the columns used from one of the tables
// are common (are in the same VEG) because they were not changed.
//-----------------------------------------------------------------------------------
void OptColumnMapping::checkWhichTablesAreNeeded()
{
  if (!oldCommonCols_.isEmpty())
  {
    // There are common columns - check if one of the other lists is empty.

    if (newCols_.isEmpty())
    {
      // No new cols - use the common cols as old.
      oldCols_.insert(oldCommonCols_);
    }
    else 
    {
      // Use the common cols as new.
      newCols_.insert(newCommonCols_);
    }

    oldCommonCols_.clear();
    newCommonCols_.clear();
  }

  CMPASSERT(oldCommonCols_.isEmpty());
  CMPASSERT(newCommonCols_.isEmpty());

  // OK, now that we fixed the commonCols, let's see what we have left.
  if (!newCols_.isEmpty())
  {
    // If we get here, we definetly need the NEW columns.
    newNeeded_ = TRUE;
    mappingArray_.insert(newCols_);
    newCols_.clear();
  }

  if (!oldCols_.isEmpty())
  {
    // If we get here, we definetly need the OLD columns.
    oldNeeded_ = TRUE;
    mappingArray_.insert(oldCols_);
    oldCols_.clear();
  }

  CMPASSERT(newNeeded_ || oldNeeded_);
}

//-----------------------------------------------------------------------------------
// Build a select list on top of the temp Scan or Join of temp Scans.
// This select list has only the minimal columns needed by the triggers. Binding
// and normalizing the tree with this select list will guarantee minimal outputs.
//-----------------------------------------------------------------------------------
ItemExpr *OptColumnMapping::buildSelectListFromNeededInputs()
{
  ItemExpr *selectList = NULL;

  for (CollIndex i=0; i<mappingArray_.entries(); i++)
  {
    ColumnMapping *currentCol = mappingArray_[i];

    // Create a ColReference to the OLD or NEW column.
    ItemExpr *colRef = new(heap_)
      ColReference(new(heap_) ColRefName(*currentCol->colName_, heap_));

    // Add it to the list.
    if (selectList == NULL)
      selectList = colRef;
    else
      selectList = new(heap_) ItemList(selectList, colRef);
  }

  return selectList;
}

//-----------------------------------------------------------------------------------
// Build the MapValueIds node above the bound temp Scan tree.
// For each entry in the (minimized) mapping list, find the corresponding bottom
// ValueId in the RETDesc of the bound tree, and add a mapEntry using the top and 
// bottom ValueIds.
// Also update the outputs of the MapValueIds node accordingly.
//-----------------------------------------------------------------------------------
RelExpr *OptColumnMapping::buildMapValueIdNode(RelExpr *tempScanTree)
{
  ValueIdSet OutputsOfMap;
  ValueIdSet tempScanOutputs(tempScanTree->getGroupAttr()->getCharacteristicOutputs());
  ValueIdMap *map = new(heap_) ValueIdMap;
  const RETDesc *bottomRetDesc = tempScanTree->getRETDesc();

  for (CollIndex i=0; i<mappingArray_.entries(); i++)
  {
    ColumnMapping *currentCol = mappingArray_[i];

    // Add the topValueId to the outputs of the MapValueIds node.
    OutputsOfMap  += currentCol->topValueId_;

    // Find the bottom ValueId from the RETDesc.
    ValueId bottomValueId = 
      bottomRetDesc->findColumn(*currentCol->colName_)->getValueId();

    // Find the VEG from the outputs of the temp Scan tree, that references
    // the ValueId we found in the RETDesc.
    ValueId bottomVeg;
    if (!tempScanOutputs.referencesTheGivenValue(bottomValueId, bottomVeg))
      CMPASSERT(FALSE);

    // Add a new mapping entry for the MapValeIds node.
    map->addMapEntry(currentCol->topValueId_, bottomVeg);
  }

  // Create the MapValueIds node with the mapping.
  MapValueIds *mapNode = new(heap_) MapValueIds(tempScanTree, *map);

  // The outputs are the top ValueIds.
  mapNode->getGroupAttr()->setCharacteristicOutputs(OutputsOfMap);

  // The only input is the ExecId.
  ValueId topExecId = bindInfo_->getExecuteId()->getValueId();
  mapNode->getGroupAttr()->addCharacteristicInputs(topExecId);

  return mapNode;
}

