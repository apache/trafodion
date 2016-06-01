/* -*-C++-*-
*************************************************************************
*
* File:         NormWA.C
* Description:  The workarea used by the normalizer
* Created:      4/27/94
* Language:     C++
*
*
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
*
*
*************************************************************************
*/

// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "NormWA.h"
#include "CmpContext.h"

#ifdef INCLUDE_SELECTED_HEADERS
#endif // INCLUDE_SELECTED_HEADERS


void NormWA::setComplexScalarExprFlag() 
{ 
  complexScalarExprCount_++; 
  setWalkingAnExprTreeFlag(); 
} // NormWA::setComplexScalarExprFlag() 

void NormWA::restoreComplexScalarExprFlag() 
{ 
  complexScalarExprCount_--; 
  restoreWalkingAnExprTreeFlag(); 
} // NormWA::restoreComplexScalarExprFlag() 

void NormWA::setNullFlag() 
{ 
  nullCount_++;  
  setWalkingAnExprTreeFlag(); 
} // NormWA::setNullFlag() 

void NormWA::restoreNullFlag() 
{ 
  nullCount_--; 
  restoreWalkingAnExprTreeFlag(); 
} // NormWA::restoreNullFlag()

void NormWA::setNotFlag() 
{ 
  notCount_++;  
  setWalkingAnExprTreeFlag(); 
} // NormWA::setNotFlag() 

void NormWA::restoreNotFlag() 
{ 
  notCount_--; 
  restoreWalkingAnExprTreeFlag(); 
} // NormWA::restoreNotFlag()

void NormWA::setOrFlag() 
{ 
  orCount_++; 
  setWalkingAnExprTreeFlag();  
} // NormWA::setOrFlag() 

void NormWA::restoreOrFlag() 
{ 
  orCount_--; 
  restoreWalkingAnExprTreeFlag(); 
} // NormWA::restoreOrFlag() 

CollHeap* NormWA::wHeap()
{
  return (currentCmpContext_) ? (currentCmpContext_->statementHeap()) : 0;
}

void NormWA::locateVEGRegionAndMarkToBeMergedRecursively(const ValueId & vid)
{
    VEGRegion* toBeMergedRegion = locateVEGRegionAndMarkToBeMerged(vid);
  if (toBeMergedRegion)
  {
    ValueIdSet nullInstValues;
    toBeMergedRegion->gatherInstantiateNullMembers(nullInstValues);
    for (ValueId exprId = nullInstValues.init();
                          nullInstValues.next(exprId);
                          nullInstValues.advance(exprId))
    {
      locateVEGRegionAndMarkToBeMergedRecursively(exprId);
    }
  }
}

NABoolean NormWA::findEquivalentInSeqFunctionsCache(ItemExpr * newItem, 
                                            ValueId &cacheEquivTransSeqId ){

  //ValueId equivId = newSeqId;
  CollIndex equivIndex = -1;
  //ItemExpr *newItem = newSeqId.getItemExpr();
  ItmSequenceFunction *newSeq = NULL;
  if(newItem->isASequenceFunction()) {
    newSeq = (ItmSequenceFunction *)newItem;
  }

  if(newSeq) {
    
    for(CollIndex i = 0 ; i < origSeqFunction_.entries(); i++) 
    {
      ItemExpr *seq = origSeqFunction_[i];
      if(newSeq->isEquivalentForBinding(seq)){
        equivIndex = i;
	break;
      }
    }
  }

  if (equivIndex == -1) {
    return FALSE;
  }
  return   retrieveFromSeqFunctionsCache(equivIndex, cacheEquivTransSeqId);
}



ValueId NormWA::getEquivalentItmSequenceFunction(ValueId newSeqId)
{
  ValueId equivId = newSeqId;

  ItemExpr *newItem = newSeqId.getItemExpr();
  ItmSequenceFunction *newSeq = NULL;
  if(newItem->isASequenceFunction()) {
    newSeq = (ItmSequenceFunction *)newItem;
  }

  if(newSeq) {
    for(ValueId seqId = allSeqFunctions_.init(); allSeqFunctions_.next(seqId); 
      allSeqFunctions_.advance(seqId) ){
      ItemExpr *seq = seqId.getItemExpr();
      if(newSeq->isEquivalentForBinding(seq)){
	equivId = seqId;
	if(newSeq->origOpType() != seq->origOpType()) {
	  seq->setOrigOpType(seq->getOperatorType());
	}
	break;
      }
    }
  }

  allSeqFunctions_ += equivId;

  //
  return equivId;
}

void NormWA::optimizeSeqFunctions( ItemExpr * ie, ItemExpr * pie, Int32 idx)
{
  
  if (ie){
    for(Int32 i = 0; i < ie->getArity(); i++ ){
      optimizeSeqFunctions( ie->child(i), ie , i);
    }
    if (ie->isASequenceFunction()) {
      ValueId equivid;
      equivid = getEquivalentItmSequenceFunction(ie->getValueId());
      if (equivid != ie->getValueId()) {
	pie->child(idx)= equivid;
      }
      //ie = getEquivalentItmSequenceFunction(ie->getValueId()).getItemExpr();
    }
  }
  else {
    return;
  }


}

const RelExpr * NormWA::getCurrentOwnerExpr()
{
  if (getVEGTable() && getVEGTable()->getCurrentVEGRegion())
    return getVEGTable()->getCurrentVEGRegion()->getOwnerExpr()->castToRelExpr() ;
  else
    return NULL ;
}


// Methods of the SqoWA 

// This function restores a changed RelExpr back to its original form
// based on what we originally changed.
NABoolean SqoChangedRelExprs::undoChanges(NormWA & normWARef, Lng32 subqId)
{
   NABoolean performedUndo = FALSE;
   if (subqId_ == subqId)
   {
      switch (whatChanged_)
      {
         case SQO_REASSIGNED_VREGION:
            normWARef.reassignVEGRegion(changedRelExpr_, changedChildId_,
                                        originalRelExpr_, origChildId_);
            performedUndo = TRUE;
         break;
      }
   }
   return performedUndo;
}

// This function restores a changed Vid back to its original form
// based on what we originally changed.
NABoolean SqoChangedItemExprs::undoChanges(NormWA & normWARef, Lng32 subqId)
{
   NABoolean performedUndo = FALSE;

   if (subqId_ == subqId)
   {
      switch (whatChanged_)
      {
         case SQO_REPLACED:
            changedVid_.replaceItemExpr(oldItemExpr_);
            performedUndo = TRUE;
            break;
         case SQO_NEWCHILD:
            changedVid_.getItemExpr()->child(changedChild_) = oldItemExpr_;
            performedUndo = TRUE;
            break;
         case SQO_NEWOPTYPE: 
            changedVid_.getItemExpr()->setOperatorType(oldOperatorType_);
            performedUndo = TRUE;
            break;
      }
   }
   return performedUndo;
}

void SqoWA::undoChanges(NormWA & normWARef )
{
   // We need a list of indexes to ItemExprs that we have restored
   LIST(SqoChangedItemExprs * ) undoneItemExprs;

   // We need a list indexes to of RelExprs that we have restored
   LIST(SqoChangedRelExprs * ) undoneRelExprs;

   for (UInt32 itemIdx=0; itemIdx < changedItemExprs_.entries(); itemIdx++)
   {
      if (changedItemExprs_[itemIdx]->undoChanges(normWARef, subqId_))
         undoneItemExprs.insert(changedItemExprs_[itemIdx]); 
   }
   for (UInt32 relIdx=0; relIdx < changedRelExprs_.entries(); relIdx++)
      {
         if (changedRelExprs_[relIdx]->undoChanges(normWARef, subqId_)) 
            undoneRelExprs.insert(changedRelExprs_[relIdx]); 
      }

   // Remove the things we undid.
   for (UInt32 idx=0; idx < undoneItemExprs.entries(); idx++)
      changedItemExprs_.remove(undoneItemExprs[idx]);
   for (UInt32 idx=0; idx < undoneRelExprs.entries(); idx++)
      changedRelExprs_.remove(undoneRelExprs[idx]);
        
}

