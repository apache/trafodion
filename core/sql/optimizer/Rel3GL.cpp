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
* File:         Rel3GL.cpp
* Description:  PSM/3GL operators.
*
* Created:      12/8/97
* Language:     C++
*
*
******************************************************************************
*/

#include "AllRelExpr.h"
#include "AllItemExpr.h"
#include "CmpContext.h"
#include "GroupAttr.h"
#include "NormWA.h"
#include "opt.h"
#include "PhyProp.h"
#include "BindWA.h"
#include "Collections.h"

//////////////////////////////////////////////////////////////////////////
//
// CompoundStmt methods.
//
//////////////////////////////////////////////////////////////////////////

RelExpr *CompoundStmt::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    return this;

  // Bind the children. 
  // rowset's modifyTree and modifyTupleNode rely on finding HostArraysArea
  // in bindWA.
  for (Int32 i = 0; i < getArity(); i++) {
    if (child(i)) {
      // If doing a non-first child and the operator is
      // NOT one in which values/names can flow from one scope
      // the sibling scope, then we must clear the current RETDesc
      // (so as to disallow the illegal query in the Binder internals document,
      // section 1.5.3, also in TEST028).
      if (i && !getOperator().match(REL_ANY_TSJ))
        bindWA->getCurrentScope()->setRETDesc(NULL);

      if (child(i)->getOperatorType() == REL_ROOT)
        // each RelRoot child of a compoundstmt has its own hostArraysArea. 
        // tell bindWA about it. 
        bindWA->setHostArraysArea
          (((RelRoot*)getChild(i))->getHostArraysArea());

      child(i) = child(i)->bindNode(bindWA);
      if (bindWA->errStatus()) return this;
    }
  }

  if (bindWA->errStatus()) return this;

  // QSTUFF
  synthPropForBindChecks();   

  if (getGroupAttr()->isStream()){
    *CmpCommon::diags() << DgSqlCode(-4200);
    bindWA->setErrStatus();
    return this;
  }
  if (getGroupAttr()->isEmbeddedUpdateOrDelete() ||
      getGroupAttr()->isEmbeddedInsert()){
    *CmpCommon::diags() << DgSqlCode(-4201) 
      << DgString0(getGroupAttr()->getOperationWithinGroup());
    bindWA->setErrStatus();
    return this;
  }
  // QSTUFF

 
  AssignmentStHostVars *ptrAssign = bindWA->getAssignmentStArea()->getAssignmentStHostVars();
  // Setup the RETDesc.
  RETDesc *leftTable  = child(0)->getRETDesc();
  RETDesc *rightTable = child(1)->getRETDesc();

  leftTable  = new (bindWA->wHeap()) RETDesc(bindWA, *leftTable);
  rightTable = new (bindWA->wHeap()) RETDesc(bindWA, *rightTable);

  RETDesc *resultTable = new (bindWA->wHeap()) RETDesc(bindWA);

  if (!ptrAssign) {

    resultTable->addColumns(bindWA, *leftTable);
    resultTable->addColumns(bindWA, *rightTable);
    setRETDesc(resultTable);
    bindWA->getCurrentScope()->setRETDesc(resultTable);

    // Bind the base class.
    return bindSelf(bindWA);
  }

  // In case we have assignment statements, we want our RetDesc to contain
  // only the columns that will appear in the select list, so we examine
  // each column to return to see if it is in the AssignmentStHostVars area.
  // Note: it is very important that the columns in the resultTable appear
  // in the same order as they are in the list pointed by ptrAssign

  ColumnDescList leftColumnList  =  *(leftTable->getColumnList());
  ColumnDescList rightColumnList  = *(rightTable->getColumnList());

  while (ptrAssign) {
    
    Int32 found = FALSE;
    UInt32 j = 0;
    for (j = 0; j < leftColumnList.entries(); j++) {
      if (ptrAssign->currentValueId() == leftColumnList[j]->getValueId()) {
        resultTable->addColumn(bindWA, leftColumnList[j]->getColRefNameObj(),
                               leftColumnList[j]->getValueId(),USER_COLUMN,
                               leftColumnList[j]->getHeading());
        found = TRUE;
        break;
      }
    }

    if (!found) {
      for (j = 0; j < rightColumnList.entries(); j++) {
        if (ptrAssign->currentValueId() == rightColumnList[j]->getValueId()) {
          resultTable->addColumn(bindWA, rightColumnList[j]->getColRefNameObj(),
                                 rightColumnList[j]->getValueId(),USER_COLUMN,
                                 rightColumnList[j]->getHeading());
          break;
        }
      }
    }

    ptrAssign = ptrAssign->next();

  }

  setRETDesc(resultTable);
  bindWA->getCurrentScope()->setRETDesc(resultTable);

  // Bind the base class.
  return bindSelf(bindWA);

} // CompoundStmt::bindNode()


void CompoundStmt::transformNode(NormWA &normWARef,
                                 ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT(this == locationOfPointerToMe);

  if (nodeIsTransformed())
    return;

  ValueIdSet availableValues = getGroupAttr()->getCharacteristicInputs();

  child(0)->getGroupAttr()->addCharacteristicInputs(availableValues);

  child(0)->transformNode(normWARef, child(0)); 

  // Values may flow from left to right
  availableValues += child(0)->getGroupAttr()->getCharacteristicOutputs();

  child(1)->getGroupAttr()->addCharacteristicInputs(availableValues);

  child(1)->transformNode(normWARef, child(1));
  
  // Pull up predicates and recompute the required inputs of children.
  pullUpPreds();

  // Transform the selection predicates.
  transformSelectPred(normWARef, locationOfPointerToMe);

  markAsTransformed();

} // CompoundStmt::transformNode

void
CompoundStmt::pushdownCoveredExpr(const ValueIdSet &outputExpr,
			    const ValueIdSet &newExternalInputs,
			    ValueIdSet &predicatesOnParent,
			    const ValueIdSet *setOfValuesReqdByParent,
			    Lng32 childIndex
			         )
{
  ValueIdSet requiredValues;
  if (setOfValuesReqdByParent)
    requiredValues =  *setOfValuesReqdByParent;

  // We may flow values from left to right
  requiredValues += child(1)->getGroupAttr()->getCharacteristicInputs();
 
  // Push expressions computable by children, updating their group attrs.
  RelExpr::pushdownCoveredExpr(outputExpr,
				newExternalInputs, 
				predicatesOnParent,
				&requiredValues);
}


RelExpr* CompoundStmt::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;
 
  // Push expressions computable by children, updating their group attrs.
  pushdownCoveredExpr(getGroupAttr()->getCharacteristicOutputs(),
                      getGroupAttr()->getCharacteristicInputs(), 
		      selectionPred());

  // Normalize children
  for (Int32 i = 0; i < getArity(); i++)
  {
    child(i) = child(i)->normalizeNode(normWARef);
  }

  fixEssentialCharacteristicOutputs();

  markAsNormalized();

  return this;
 
} // CompoundStmt::normalizeNode

void CompoundStmt::pullUpPreds()
{
  for (Int32 i = 0; i < getArity(); i++) {
    child(i)->pullUpPreds();
  }

} // CompoundStmt::pullUpPreds

void CompoundStmt::rewriteNode(NormWA & normWARef)
{
  // Rewrite children
  for (Int32 i = 0; i < getArity(); i++) 
  {
    child(i)->rewriteNode(normWARef);
  }

  // Rewrite the predicates.
  if (selectionPred().normalizeNode(normWARef))
  {
  }

  // Rewrite the Group Attributes.
  getGroupAttr()->normalizeInputsAndOutputs(normWARef);

} // CompoundStmt::rewriteNode

void CompoundStmt::recomputeOuterReferences()
{
  // ---------------------------------------------------------------------
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  // ---------------------------------------------------------------------
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs(); 

  // Remove from outerRefs those valueIds that are not needed
  // by my selection predicate
  selectionPred().weedOutUnreferenced(outerRefs);

  // Add to outerRefs those that my children need.
  Int32 arity = getArity();
  for (Int32 i = 0; i < arity; i++)
    {
      outerRefs += child(i).getPtr()->getGroupAttr()->getCharacteristicInputs();
    }

  // If some of those values are produced by the left child, then we do not
  // need them from above
  outerRefs -= child(0)->getGroupAttr()->getCharacteristicOutputs();

  // set my Character Inputs to this new minimal set.
  getGroupAttr()->setCharacteristicInputs(outerRefs);
} // RelExpr::recomputeOuterReferences()  

RelExpr *CompoundStmt::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if ( derivedNode == NULL)
    result = new (outHeap) CompoundStmt(NULL, NULL, getOperatorType(), outHeap);
  else 
    result = (CompoundStmt*) derivedNode;

  return RelExpr::copyTopNode(result, outHeap);

} // CompoundStmt::copyTopNode


void CompoundStmt::enterVEGRegion(NormWA &normWARef, Int32 id, NABoolean create)
{
  if (child(id)->getOperatorType() != REL_COMPOUND_STMT)
  {
    if (create)
      normWARef.allocateAndSetVEGRegion(IMPORT_AND_EXPORT, this, id);
    else
      normWARef.locateAndSetVEGRegion(this, id);
  }

} // CompoundStmt::enterVEGRegion


void CompoundStmt::leaveVEGRegion(NormWA & normWARef, Int32 id)
{
  if (child(id)->getOperatorType() != REL_COMPOUND_STMT)
    normWARef.restoreOriginalVEGRegion();

} // CompoundStmt::leaveVEGRegion

// ***********************************************************************
// Methods for class AssignmentStHostVars.
// This class is used by assignment statements in compound statements.
// A list of this type is kept in class BindWA so that it is globally
// accessible at binding time. This class mantains a current and previous
// valueId, so that when we find SET <var> = <select statement>, we update
// the value id of <var> to that returned by the select statement. See 
// assignment statements internal spec for details. 
// ***********************************************************************

// Adds var to end of this list with new value id; if it is already there, it
// updates the value id
AssignmentStHostVars & AssignmentStHostVars::addVar(HostVar *var, const ValueId &id)
{
  AssignmentStHostVars *ptr, *current;

  current = ptr = this;

  // Empty list
  if (!(ptr->var())) {
    ptr->var() = var;
    ptr->setCurrentValueId(id);
    return *ptr;
  }

  // Search for variable name and update value id
  // Note that ptr can become null when this loop ends
  while (ptr && ptr->var()) {

    if (ptr->var()->getName() == var->getName()) {
      
      // New value id must replace the latest one
      ptr->setCurrentValueId(id);
      return *ptr;
    }

    current = ptr;
    ptr = ptr->next_;
  }


  // Now add variable to the end of the list
  ptr = current->next_ = new (bindWA_->wHeap()) AssignmentStHostVars(bindWA_);
  ptr->var() = var;
  ptr->setCurrentValueId(id);
  return *ptr;
}

// Update the latest value id of this host var. 
void AssignmentStHostVars::setCurrentValueId(const ValueId &id)
{

  if (valueIds_.entries() == 0) {
    valueIds_.insert(id);
    return;
  }

  AssignmentStArea * ptrArea = bindWA_->getAssignmentStArea();
  Union *ifNode = ptrArea->getCurrentIF();
  AssignmentStHostVars *varList = NULL;

  // If we are not inside an IF statement, then we replace the 
  // value id this variable has with the one provided in the parameter
  // to this function.  
  if (!ifNode) {
    valueIds_[valueIds_.entries() - 1] = id;
    return;
  }

  // If we are within an IF statement, we must figure out if this is the
  // first time we are setting the value id of this variable. If it is so, 
  // then we insert a new value id at the end of the list of value ids for
  // this variable. Otherwise we overwrite the current value id
  varList = ifNode->getCurrentList(bindWA_);
  if (varList && (varList->findVar(var()->getName()))) {
    valueIds_[valueIds_.entries() - 1] = id;
  }
  else {
    valueIds_.insert(id);
  }
}

// Gets current value id 
const ValueId AssignmentStHostVars::currentValueId()
{
  if (valueIds_.entries() > 0) {
    return valueIds_[valueIds_.entries() - 1];
  }
  else {
    return NULL_VALUE_ID;
  }
}

// Gets the variable
HostVar *& AssignmentStHostVars::var()
{
  return var_;
}

// Finds the variable whose name is given
AssignmentStHostVars * AssignmentStHostVars::findVar(const NAString & name)
{

  AssignmentStHostVars *ptr = this;

  while (ptr && ptr->var_) {

    // We must consider the cases when the variable has or does not have an indicator
    if ((ptr->var_->getName() == name) || 
        ((ptr->var_->getName() + " " + ptr->var_->getIndName()) == name)) {
      return ptr;
    }

    ptr = ptr->next_;
  }      
  
  return NULL;

}

// When we reach a Root node that contains a list of host variables on the
// left hand side of an assignment statement (assignmentStTree_), we update 
// the value ids of such variables with the value ids returned from the 
// subtree below the Root node. This function is called in RelRoot::bindNode
NABoolean AssignmentStHostVars::updateValueIds(const ValueIdList &returnedList, ItemExpr *listInRootNode)
{
  UInt32 listSize = 0;
  ItemExpr *hostVarList = listInRootNode;

  // returnedList contains a list of the value ids returned from below the node we are in.
  // listInRootNode contains a list of the variables being assigned to
  CollIndex i = 0;
  for (i = 0; i < returnedList.entries() && hostVarList; i++) {
    ItemExpr *targetExpr = hostVarList->child(0); 
    CMPASSERT(targetExpr);

    //
    // Check that the operands are compatible.
    //
    ValueId sourceId = returnedList[listSize];
    const NAType& targetType = *(((HostVar *) targetExpr)->getType());
    sourceId.coerceType(targetType);
    const NAType& sourceType = sourceId.getType();

    if (NOT targetType.isCompatible(sourceType)) {

      // Relaxing Characet Data Type matching rule.
      NABoolean relax = FALSE;
      if ( (targetType.getTypeQualifier() == NA_CHARACTER_TYPE) && 
           (sourceType.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	   ((const CharType&)targetType).getCharSet() == CharInfo::UNICODE &&
	   ((const CharType&)sourceType).getCharSet() == CharInfo::ISO88591
         ) {
	  relax = TRUE;
      }
      
      if ( !relax ) {
	if (CmpCommon::getDefault(IMPLICIT_DATETIME_INTERVAL_HOSTVAR_CONVERSION) == DF_OFF)
	  {
	    // Incompatible assignment from type $0~String0 to type $1~String1
	    *CmpCommon::diags() << DgSqlCode(-30007)
				<< DgString0(sourceType.getTypeSQLname(TRUE /*terse*/))
				<< DgString1(targetType.getTypeSQLname(TRUE /*terse*/));
	    bindWA_->setErrStatus();
	    return FALSE;
	  }
      }
    }

    // temp contains the variable; returnedList[listSize] contains its new value id
    addVar((HostVar *) targetExpr, sourceId);
    hostVarList = hostVarList->child(1);
    listSize++;
  }
  
  if (hostVarList || (listSize != returnedList.entries())) {
      // Mismatch in number of variables in SET list and the returned list
     *CmpCommon::diags() << DgSqlCode(-4094)
      << DgInt0(listSize) << DgInt1(returnedList.entries());
     bindWA_->setErrStatus();
     return FALSE;
  }

  // We keep track of all HostVars on the left side of the SET statement
  // and that appear below the Root node

  AssignmentStArea *ptrArea = bindWA_->getAssignmentStArea();
  Union *currentIF = ptrArea->getCurrentIF();

  // We get the list of host vars in the current IF node associated with the
  // subtree (left or right) that we are currently visiting
  if (currentIF) { 
    AssignmentStHostVars *listOfVarsInIF = currentIF->getCurrentList(bindWA_);
   
    listSize = 0;
    hostVarList = listInRootNode;
  
    // And we insert into that list all the variables that have been assigned in this
    // subtree
    for (i = 0; i < returnedList.entries(); i++) {
      if (hostVarList) {
        ItemExpr *temp = hostVarList->child(0); 
	HostVar *var = (HostVar *) temp;

        // temp contains the variable; returnedList[listSize] contains its new value id
	listOfVarsInIF->addToListInIF(var, returnedList[i]);

	// Get next variable
        hostVarList = hostVarList->child(1);
      }
    }
  }
    
  return TRUE;
}

// To know if there are rowsets in the given list
NABoolean AssignmentStHostVars::containsRowsets(ItemExpr * list) 
{

  if (!list) {
    return FALSE;
  }

  while (list) {
       
    if (list->getChild(0) && list->getChild(0)->getOperatorType() == ITM_HOSTVAR) {
      
      HostVar *hostVar = (HostVar *) (list->getChild(0));
      
      if (hostVar->getType()->getTypeQualifier() == NA_ROWSET_TYPE) {
        return TRUE;
      }
    }

    list = list->child(1);
  }

  return FALSE;
}

   
// Updates listOfVars_ when we exit a child of Union node at binding time. For
// each host variable, it determines whether its value id is no longer
// valid outside the branch of the IF statement we are exiting, and removes it if so.
// In this way, the value id this variable had before entering the branch of the IF 
// statement will be the current one
void AssignmentStArea::removeLastValueIds(AssignmentStHostVars * listInUnion, Union * node)
{

  while (listInUnion && (listInUnion->var())) {

    // Get the variable in the Union node
    HostVar * var = listInUnion->var();

    // Now find it in the global list of Host variables
    AssignmentStHostVars * theVar = listOfVars_->findVar(var->getName());

    // The last value id of theVar is no longer valid
    CMPASSERT(theVar);
    theVar->removeLastValueId();

    listInUnion = listInUnion->next();

  }
}



// Adds var to the given list with new value id; if it is already there, it
// overwrites the value. This functio is used only for lists contained in
// an IF node (i.e. left list_ and rightList_ in class Union).
AssignmentStHostVars & AssignmentStHostVars::addToListInIF(HostVar *var, const ValueId &id)
{
  AssignmentStHostVars *ptr      = NULL;
  AssignmentStHostVars *previous = NULL;

  ValueIdList list; 
  list.clear();
  list.insert(id);

  ptr = this;
  NABoolean found = FALSE;

  while (ptr && ptr->var_) {

    if (ptr->var_->getName() == var->getName()) {
      found = TRUE;
      break;
    }

    previous = ptr;
    ptr = ptr->next_;
  }      
 
  if (found) {
    ptr->valueIds() = list;
  }
  else {
    if (ptr) {
      ptr->var_ = var;
      ptr->valueIds_ = list;
    }
    else {
      ptr = new (bindWA_->wHeap()) AssignmentStHostVars(bindWA_);
      ptr->var_ = var;
      ptr->valueIds_ = list;
      previous->next_ = ptr;
      ptr->next_ = NULL;
    }
  }

  return *ptr;
}


// Adds all the hostvars and their associated value ids from the AssignmentStHostVars
// that is passed as an argument to this method to the AssignmentStHostVars pointed to
// this pointer. 
void AssignmentStHostVars::addAllToListInIF(AssignmentStHostVars * copyFromList)
{

while (copyFromList) { 
	HostVar *var = copyFromList->var();
	ValueId id   = copyFromList->currentValueId();
        addToListInIF(var, id);
	copyFromList = copyFromList->next();
      }
}
