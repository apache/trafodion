/**********************************************************************
//
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
**************************************************************************
*
* File:         RelRoutine.cpp
* Description:  RelExprs related to support for Routines and UDFs
*               Old RelRoutine class got renamed to RelRoutine
*               class resturcturing effort to support UDFs.
* Created:      6/29/09
* Language:     C++
*
*************************************************************************
*/

#define SQLPARSERGLOBALS_FLAGS               // must precede all #include's
#define SQLPARSERGLOBALS_NADEFAULTS

#include "Debug.h"
#include "Sqlcomp.h"
#include "AllRelExpr.h"
#include "GroupAttr.h"
#include "opt.h"
#include "PhyProp.h"
#include "mdam.h"
#include "ControlDB.h"
#include "disjuncts.h"
#include "ScanOptimizer.h"
#include "CmpContext.h"
#include "StmtDDLCreateTrigger.h"
#include "ExpError.h"
#include "ComTransInfo.h"
#include "BindWA.h"
#include "Refresh.h"
#include "CmpMain.h"
#include "ControlDB.h"
#include "ElemDDLColDef.h"
#include "Analyzer.h"
#include "OptHints.h"
#include "ComTdbSendTop.h"
#include "DatetimeType.h"
#include "ItemNAType.h"
#include "SequenceGeneratorAttributes.h"
#include "SqlParserGlobals.h"
#include "RelRoutine.h"
#include "ElemDDLColDefArray.h"
#include "CmpSeabaseDDL.h"
#include "UdfDllInteraction.h"
#include "TrafDDLdesc.h"


// -----------------------------------------------------------------------
// methods for class RelRoutine
// -----------------------------------------------------------------------

//! RelRoutine::RelRoutine Copy Constructor
RelRoutine::RelRoutine(const RelRoutine & other, CollHeap *h )
: RelExpr(other.getOperatorType(), NULL, NULL, h)
{
  setSelectionPredicates(other.getSelectionPred());
  procAllParamsTree_ = other.procAllParamsTree_;
  procAllParamsVids_ = other.procAllParamsVids_;
  procInputParamsVids_ = other.procInputParamsVids_;
  procOutputParamsVids_ = other.procOutputParamsVids_;
  routineName_ = other.routineName_;

   // Just make a deep copy of the Routine Desc
  if (other.routined_ != NULL)
    routined_ = new (h) RoutineDesc(*other.routined_, h);
  else 
    routined_ = NULL;
  hasSubquery_ = other.hasSubquery_;
}

//! RelRoutine::~RelRoutine Destructor
RelRoutine::~RelRoutine()
{
   // do not deallocate routine_ - shallow copy
}

//! RelRoutine::getPotentialOutputValues method
//  gets the output values for the Routine from the RoutineDesc
void RelRoutine::getPotentialOutputValues(
     ValueIdSet & outputValues) const
{
  outputValues.clear();
  //
  // The attached Routine descriptor has a list of all the columns
  // that this node can possibly generate.
  //
  RoutineDesc *rDesc = getRoutineDesc();
  if (rDesc != NULL) 
    outputValues.insertList( getRoutineDesc()->getOutputColumnList() );
} // RelRoutine::getPotentialOutputValues()

//! RelRoutine::topHash method
HashValue RelRoutine::topHash()
{
  HashValue result = RelExpr::topHash();

  result ^= arity_;
  result ^= procAllParamsVids_;
  result ^= routineName_.getQualifiedNameAsAnsiString();

  return result;
}

//! RelRoutine::duplicateMatch method
NABoolean RelRoutine::duplicateMatch(const RelExpr & other) const
{
  if (!RelExpr::duplicateMatch(other))
    return FALSE;

  RelRoutine &o = (RelRoutine &) other;

  if (NOT (procAllParamsVids_ == o.procAllParamsVids_))
   return FALSE;
  if (NOT (routineName_ == o.routineName_))
   return FALSE;
  if (NOT (arity_ == o.arity_))
   return FALSE;

  return TRUE;
}

//! RelRoutine::copyTopNode method
RelExpr * RelRoutine::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelRoutine *result;

  if (derivedNode == NULL)
    result = new (outHeap) RelRoutine(NULL,
                                      REL_ROUTINE,
                                      outHeap);
  else
    result = (RelRoutine *) derivedNode;

  result->hasSubquery_           = hasSubquery_;
  result->routineName_           = routineName_;
  result->procAllParamsVids_     = procAllParamsVids_;
  result->procInputParamsVids_   = procInputParamsVids_;
  result->procOutputParamsVids_  = procOutputParamsVids_;
  result->arity_ = arity_;

  // Make a deep copy unless we have been transformed. Once we have 
  // ValueIds for all the parameters, we don't need the tree.
  if (nodeIsTransformed())
  {
    result->procAllParamsTree_ = NULL;
  }
  else
  {
    // Since we are not transformed, we know that  we will not have any
    // VEGReferences in the tree and it is safe to copy it, otherwise
    // we would assert since VEGReferences do not have a copyTopNode() 
    // method.. 
    if (procAllParamsTree_ != NULL)
                    result->procAllParamsTree_ = procAllParamsTree_->
                                           copyTree(outHeap)->castToItemExpr();
  }

    // Copy routine descriptor, by creating another instance
  if (routined_ != NULL)
  {
    result->routined_ = new (outHeap) RoutineDesc(*routined_, outHeap);

  } 
  else
  {
    result->routined_ = NULL;
  }
      
    // we make sure we copy the rest of the class structure by calling the 
    // parent class copyTopNode()
  return RelExpr::copyTopNode(result, outHeap);
}

//! RelRoutine::addLocalExpr method
void RelRoutine::addLocalExpr(LIST(ExprNode *) &xlist,
                              LIST(NAString) &llist) const
{
  if (procAllParamsTree_ != NULL OR
      NOT procAllParamsVids_.isEmpty())
  {
    if(procAllParamsVids_.isEmpty())
      xlist.insert(procAllParamsTree_);
    else
      xlist.insert(procAllParamsVids_.rebuildExprTree());

    llist.insert("actual_parameters");
  }
  
  RelExpr::addLocalExpr(xlist,llist);
}

//! RelRoutine::getText method
const NAString RelRoutine::getText() const
{
  return "relroutine";
}

// -----------------------------------------------------------------------
// methods for class TableValuedFunction
// -----------------------------------------------------------------------


//! TableValuedFunction::~TableValuedFunction Destructor
TableValuedFunction::~TableValuedFunction()
{
}

//! TableValuedFunction::getPotentialOutputValues method
void TableValuedFunction::getPotentialOutputValues(
     ValueIdSet & outputValues) const
{
  outputValues.clear();
  //
  // The attached table descriptor has a list of all the columns
  // that this node can possibly generate.
  //
  outputValues.insertList( getTableDesc()->getColumnList() );
} // TableValuedFunction::getPotentialOutputValues()


//! TableValuedFunction::duplicateMatch method
NABoolean TableValuedFunction::duplicateMatch(const RelExpr & other) const
{
   if (! RelRoutine::duplicateMatch(other)) 
      return FALSE;

   TableValuedFunction &o = (TableValuedFunction &) other;
   if (tabId_ == o.tabId_)
   {
      return TRUE;
   }

   return FALSE;

}

//! TableValuedFunction::copyTopNode method
RelExpr * TableValuedFunction::copyTopNode(RelExpr *derivedNode,
                                CollHeap* outHeap)

{
   TableValuedFunction *result;

   if (derivedNode == NULL)
   {
     result = new (outHeap) TableValuedFunction(NULL,
                               getOperatorType(),
                               outHeap);
   }
   else
     result = (TableValuedFunction *) derivedNode;

   // Make a shallow copy of the table decriptor. This works the same was 
   // as what we do for  a Scan node.
   result->tabId_ = tabId_;
   result->userTableName_ = userTableName_ ;

   return RelRoutine::copyTopNode(result, outHeap);
}

//! TableValuedFunction::getText method
const NAString TableValuedFunction::getText() const
{
  return "TableValuedFunction";
}

// -----------------------------------------------------------------------
// methods for class TableMappingUDFChildInfo
// -----------------------------------------------------------------------

TableMappingUDFChildInfo::TableMappingUDFChildInfo
                (const TableMappingUDFChildInfo &other)
{
  inputTabName_ = other.inputTabName_;
  inputTabCols_ = other.inputTabCols_; 
  partType_ = other.partType_;
  partitionBy_ = other.partitionBy_;
  orderBy_ = other.orderBy_;
  outputs_ = other.outputs_;
}

void TableMappingUDFChildInfo::removeColumn(CollIndex i)
{
  CMPASSERT(!partitionBy_.contains(outputs_[i]));
  CMPASSERT(!orderBy_.contains(outputs_[i]));
  inputTabCols_.removeAt(i);
  outputs_.removeAt(i);
}

// -----------------------------------------------------------------------
// methods for class TableMappingUDF
// -----------------------------------------------------------------------

//! TableMappingUDF::TableMappingUDF Copy Constructor
TableMappingUDF::TableMappingUDF(const TableMappingUDF & other)
  : TableValuedFunction(other),childInfo_(STMTHEAP)
{
  selectivityFactor_ = other.selectivityFactor_;
  cardinalityHint_ = other.cardinalityHint_;
  for(CollIndex i = 0; i < other.childInfo_.entries(); i++)
  {
    childInfo_.insert(other.childInfo_[i]); // shallow copy, since copt ctor 
  }// called during nextSubstitue
  scalarInputParams_ = other.scalarInputParams_;
  outputParams_ = other.outputParams_;
  dllInteraction_ = other.dllInteraction_ ;
  invocationInfo_ = other.invocationInfo_;
  routineHandle_ = other.routineHandle_;
  constParamBuffer_ = other.constParamBuffer_; // shallow copy is ok
  constParamBufferLen_ = other.constParamBufferLen_;
  udfOutputToChildInputMap_ = other.udfOutputToChildInputMap_;
  isNormalized_ = other.isNormalized_;
}

//! TableMappingUDF::~TableMappingUDF Destructor
TableMappingUDF::~TableMappingUDF()
{
  // delete [] childInfo_ ;
}

//! TableMappingUDF::copyTopNode method
RelExpr * TableMappingUDF::copyTopNode(RelExpr *derivedNode,
                                CollHeap* outHeap)

{
   TableMappingUDF *result;

   if (derivedNode == NULL)
   {
     result = new (outHeap) TableMappingUDF(getArity(),
                                            NULL,
                                            outHeap);
   }
   else
     result = (TableMappingUDF *) derivedNode;

  for(CollIndex i = 0; i < childInfo_.entries(); i++)
  {
    TableMappingUDFChildInfo * ci = new (outHeap)
	TableMappingUDFChildInfo(*(childInfo_[i]));
      result->childInfo_.insert(ci);
  } 
  result->selectivityFactor_ = selectivityFactor_;
  result->cardinalityHint_ = cardinalityHint_;
  result->scalarInputParams_ = scalarInputParams_;
  result->outputParams_ = outputParams_;
  result->dllInteraction_ = dllInteraction_;
  result->invocationInfo_ = invocationInfo_;     // shallow copies for these
  result->routineHandle_ = routineHandle_;       // items are ok, they are
  result->constParamBuffer_ = constParamBuffer_; // shared for this invocation
  result->constParamBufferLen_ = constParamBufferLen_;
  result->predsEvaluatedByUDF_ = predsEvaluatedByUDF_;
  result->udfOutputToChildInputMap_ = udfOutputToChildInputMap_;
  result->isNormalized_ = isNormalized_;
  return TableValuedFunction::copyTopNode(result, outHeap);
}

TableMappingUDF *TableMappingUDF::castToTableMappingUDF()
{
  return this;
}

//! TableMappingUDF::getText method
const NAString TableMappingUDF::getText() const
{
 NAString op(CmpCommon::statementHeap());
 op = "tmudf ";
 return op + getUserTableName().getTextWithSpecialType();
}

PredefinedTableMappingFunction * TableMappingUDF::castToPredefinedTableMappingFunction()
{
  return NULL;
}

void TableMappingUDF::addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const
{
  if (!predsEvaluatedByUDF_.isEmpty())
    {
      xlist.insert(predsEvaluatedByUDF_.rebuildExprTree());
      llist.insert("preds_evaluated_by_udf");
    }

  for(CollIndex i = 0; i < childInfo_.entries(); i++)
  {
    if (NOT childInfo_[i]->partitionBy_.isEmpty())
    {
      xlist.insert(childInfo_[i]->partitionBy_.rebuildExprTree());
      llist.insert("child_part_by");
    }
    if (NOT childInfo_[i]->orderBy_.isEmpty())
    {
      xlist.insert(childInfo_[i]->orderBy_.rebuildExprTree());
      llist.insert("child_order_by");
    }
    if (NOT childInfo_[i]->outputs_.isEmpty())
    {
      xlist.insert(childInfo_[i]->outputs_.rebuildExprTree());
      llist.insert("child_outputs");
    }
  } 
  RelRoutine::addLocalExpr(xlist,llist);
}

void TableMappingUDF::transformNode(NormWA & normWARef,
    ExprGroupId & locationOfPointerToMe) 
{
  RelExpr::transformNode(normWARef, locationOfPointerToMe);
}

void TableMappingUDF::rewriteNode(NormWA & normWARef)
{
  for(Int32 i = 0; i < getArity(); i++)
  {
    childInfo_[i]->partitionBy_.normalizeNode(normWARef);
    childInfo_[i]->orderBy_.normalizeNode(normWARef);
    childInfo_[i]->outputs_.normalizeNode(normWARef);
  }
  udfOutputToChildInputMap_.normalizeNode(normWARef);
  // rewrite group attributes and selection pred
  RelExpr::rewriteNode(normWARef);
}

void TableMappingUDF::primeGroupAnalysis()
{
  RelExpr::primeGroupAnalysis();
};


void TableMappingUDF::getPotentialOutputValues(ValueIdSet & vs) const 
{
  vs.clear();
  vs.insertList(getProcOutputParamsVids());
};
void TableMappingUDF::getPotentialOutputValuesAsVEGs(ValueIdSet& outputs) const
{
  CMPASSERT(0);
};
void TableMappingUDF::pullUpPreds()
{
   // A TMUDF never pulls up predicates from its children.
   for (Int32 i = 0; i < getArity(); i++) 
    child(i)->recomputeOuterReferences();
};
void TableMappingUDF::recomputeOuterReferences()
{
  // ---------------------------------------------------------------------
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  // ---------------------------------------------------------------------
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs(); 

  // Remove from outerRefs those valueIds that are not needed
  // by my selection predicate
  GroupAttributes emptyGA;
  ValueIdSet leafExprSet, emptySet;
  ValueIdSet exprSet(getProcInputParamsVids());

  exprSet.getLeafValuesForCoverTest(leafExprSet, emptyGA, emptySet); 
  leafExprSet += getSelectionPred();

  // Also add any values mentioned in the select list, order by or
  // partition by of the child queries. Those may contain expressions
  // that are not stored anywhere else and may not be evaluated in the
  // child. For example, if the child query is "select a, current_date..."
  // then the child may only produce a, but not current_date.
  for (CollIndex c=0; c<getArity(); c++)
    {
      leafExprSet += childInfo_[c]->getPartitionBy();
      leafExprSet.insertList(childInfo_[c]->getOrderBy());
      leafExprSet.insertList(childInfo_[c]->getOutputs());
    }

  leafExprSet.weedOutUnreferenced(outerRefs);

  // Add to outerRefs those that my children need.
  Int32 arity = getArity();
  for (Int32 i = 0; i < arity; i++)
    {
      outerRefs += child(i).getPtr()->getGroupAttr()->getCharacteristicInputs();
    }

  // set my Character Inputs to this new minimal set.
  getGroupAttr()->setCharacteristicInputs(outerRefs);
};

void TableMappingUDF::pushdownCoveredExpr(
     const ValueIdSet & outputExprOnOperator,
     const ValueIdSet & newExternalInputs,
     ValueIdSet& predOnOperator,
     const ValueIdSet *nonPredNonOutputExprOnOperator,
     Lng32 childId)
{
  if (!isNormalized_)
    {
      ValueIdSet exprOnParent;
      ValueIdSet predsToPushDown;
      // At this point, we have determined the characteristic outputs
      // of the TableMappingUDF and a set of selection predicates to
      // be evaluated on the result of the UDF.

      // Call the UDF code to determine which child outputs we should
      // require as child characteristic outputs and what to do with the
      // selection predicates

      // The logic below only works if we push preds to all children
      // at the same time.
      CMPASSERT(childId == -MAX_REL_ARITY);

      // interact with the UDF
      NABoolean status = dllInteraction_->describeDataflow(
           this,
           exprOnParent,
           selectionPred(),
           predsEvaluatedByUDF_,
           predsToPushDown);
      // no good way to return failure, error will be detected
      // at the end of the normalization phase

      // rewrite the predicates to be pushed down in terms
      // of the values produced by the table-valued inputs
      ValueIdSet childPredsToPushDown;
      udfOutputToChildInputMap_.rewriteValueIdSetDown(
           predsToPushDown, childPredsToPushDown);

      // remaining selection predicates stay with the parent
      exprOnParent += selectionPred();
      if(nonPredNonOutputExprOnOperator)
        exprOnParent += *nonPredNonOutputExprOnOperator;

      RelExpr::pushdownCoveredExpr(outputExprOnOperator,
                                   newExternalInputs,
                                   childPredsToPushDown,
                                   &exprOnParent,
                                   childId);

      // apply any leftover predicates back to the parent, but need
      // to rewrite them in terms of the parent first
      if (!childPredsToPushDown.isEmpty())
        {
          predsToPushDown.clear();
          // Map the leftovers back to the language of out outputs.
          // Note that since we rewrote these values on the way down,
          // a simple mapping will suffice when going back up, as they
          // are recorded already in the map table.
          udfOutputToChildInputMap_.mapValueIdSetUp(predsToPushDown,
                                                    childPredsToPushDown);
          selectionPred() += predsToPushDown;
        }
      // indicate that we did this step and recorded needed columns
      // and pushed predicate in the InvocationInfo, don't change this
      // information from now on (unless recording such changes in
      // UDRPlanInfo)
      isNormalized_ = TRUE;
    }
  // else
  // If this is past the normalizer UDF interface call,
  // pushdownCoveredExpr() has already been called in the
  // normalizer. We cannot push down predicates and/or eliminate
  // columns in this phase, because the UDR has already been told
  // which columns and predicates we need. Also, this situation could
  // apply only to one plan alternative and the UDRInvocationInfo
  // object that records the predicates and columns is shared among
  // all the alternatives. If we want to do such pushdown in the
  // future (and this would be very desirable, something like a
  // "routine join"), then we'll have to record the pushed down
  // predicates and eliminated columns in the UDRPlanInfo object and
  // we'll have to add another compiler interface method for the UDR
  // writer.
};


void TableMappingUDF::synthLogProp(NormWA * normWAPtr)
{
  // avoid multiple calls
  if (getGroupAttr()->existsLogExprForSynthesis())
    return;

  RelExpr::synthLogProp(normWAPtr);
  // +1 on the TMUDFs
  getGroupAttr()->setNumTMUDFs(getGroupAttr()->getNumTMUDFs()+1);

  NABoolean status = dllInteraction_->describeConstraints(this);
  // rely on diags area to communicate failure
};

void TableMappingUDF::finishSynthEstLogProp()
{
  RelExpr::finishSynthEstLogProp();
};


void TableMappingUDF::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp)) return;

  CostScalar inputFromParentCard = inputEstLogProp->getResultCardinality();

  // create an empty result
  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(*inputEstLogProp));

  // call the compiler UDF interaction, this may produce row counts and UECs
  // for the output columns of the UDF, if specified by the UDF writer
  NABoolean status = dllInteraction_->describeStatistics(this, inputEstLogProp);

  CostScalar udfCard = dllInteraction_->getResultCardinality(this);
  NABoolean udfSpecifiedCard = (udfCard >= 0);
  int arity = getArity();

  // Set tmUdf output cardinality, unless specified by UDF writer
  if (!udfSpecifiedCard)
    {
      // 1. no children: children histograms will have cardinality of 1
      if(arity < 1)
        {
          // CQD is used for default cardinality of a leaf TMUDF
          udfCard = 
            ActiveSchemaDB()->getDefaults().getAsDouble(TMUDF_LEAF_CARDINALITY);
        }
      else
        {
          // use information about the function type (e.g. mapper, reducer) to
          // estimate the row count
          CostScalar udfCardFactor =
            dllInteraction_->getCardinalityScaleFactorFromFunctionType(this);

          // if the function type does not help, use a CQD
          if (udfCardFactor < 0)
            udfCardFactor =
              ActiveSchemaDB()->getDefaults().getAsDouble(TMUDF_CARDINALITY_FACTOR);

          // base the result cardinality of the child with the highest cardinality
          for (CollIndex c=0; c<getArity(); c++)
            {
              EstLogPropSharedPtr childrenInputEstLogProp = child(c).outputLogProp(inputEstLogProp);
              CostScalar childCard = child(c).outputLogProp(inputEstLogProp)->getResultCardinality();

              myEstProps->unresolvedPreds() += childrenInputEstLogProp->getUnresolvedPreds();

              if (childCard > udfCard)
                udfCard = childCard;
            }

          udfCard = udfCard * udfCardFactor;
        }
    }

  myEstProps->setResultCardinality(udfCard);

  // use child(ren) histograms to figure out output column uecs
  ValueIdList &tmudfOutputs = getProcOutputParamsVids();
  ColStatDescList udfColStatDescList(CmpCommon::statementHeap());

  for(CollIndex c=0; c<tmudfOutputs.entries(); c++)
  {
    ValueId udfOutputCol = tmudfOutputs[c];
    NABoolean inputHistFound = FALSE;
    ValueId inputValId;
    ColStatsSharedPtr inputColStats;
    // initialize the uec with the value specified by the UDF, or -1
    CostScalar colUec = dllInteraction_->getOutputColumnUEC(this, c);
    
    if (arity > 0)
      {
        udfOutputToChildInputMap_.mapValueIdDown(udfOutputCol, inputValId);

        if(inputValId == udfOutputCol)
          // udfOutputCol is not passed through from a child
          inputValId = NULL_VALUE_ID;

        if(inputValId != NULL_VALUE_ID)
          {
            for (CollIndex c=0; c<arity && !inputHistFound; c++)
              {
                // try to get child histogram from this child
                inputColStats =
                  child(c).outputLogProp(inputEstLogProp)->
                  getColStats().getColStatsPtrForColumn(inputValId);

                if(inputColStats != NULL)
                  inputHistFound = TRUE;
              }
          }
      }

    if(!inputHistFound)
    {
      // if the UDF didn't specify a UEC and we have no histogram, use a CQD
      if (colUec < 1)
        colUec = udfCard.getValue() * 
          ActiveSchemaDB()->getDefaults().getAsDouble(USTAT_MODIFY_DEFAULT_UEC);

      // create a fake histogram with the specified cardinality and UEC
      udfColStatDescList.addColStatDescForVirtualCol(colUec,
                                                     udfCard, 
                                                     udfOutputCol,
                                                     udfOutputCol,
                                                     udfOutputCol,
                                                     NULL,
                                                     TRUE);
    }
    else
    {
      ColStatDescSharedPtr outputColStatDescSharedPtr(
        new(HISTHEAP) ColStatDesc(inputColStats,udfOutputCol, HISTHEAP),HISTHEAP);

      if (colUec >= 1)
        {
          // We have information from two sources: The UDF specified a column UEC
          // and we also have a histogram from the corresponding child column.
          // Try to consolidate these two pieces of information by scaling the
          // UECs in the histogram to match those specified by the UDF.
          outputColStatDescSharedPtr->getColStats()->setRowsAndUec(udfCard, colUec);
        }

      udfColStatDescList.insert(outputColStatDescSharedPtr);
    }
  }

  const ValueIdSet & inputValues = getGroupAttr()->getCharacteristicInputs();
  ValueIdSet outerReferences;
  CollIndex outerRefCount;
  ValueIdSet predsEvaluatedForStats(getSelectionPred());

  if (inputValues.entries() > 0)
    inputValues.getOuterReferences(outerReferences);
  outerRefCount = outerReferences.entries();

  if (!udfSpecifiedCard)
    // The UDF writer did not specify a cardinality, use our conventional
    // methods to estimate the effect of predicates evaluated in the
    // UDF. Note that if the UDF did specify a result cardinality, we
    // assume that it is the cardinality after evaluating those predicates.
    predsEvaluatedForStats += predsEvaluatedByUDF_;
  
  // synchronize output colum rowcounts
  udfColStatDescList.synchronizeStats(udfCard,udfColStatDescList.entries());
  
  // apply predicates
  udfCard = udfColStatDescList.estimateCardinality(
         udfCard,                /*in*/
         predsEvaluatedForStats, /*in*/
         outerReferences,        /*in*/
         NULL,                   /* no join */
         NULL,                   /* for selectivity hint, which can only be given for scan */
         NULL,                   /* for cardinality hint, which can only be given for scan */
         outerRefCount,          /*in/out*/
         myEstProps->unresolvedPreds(), /*in/out*/
         INNER_JOIN_MERGE,       /*in*/
         ITM_FIRST_ITEM_OP,      /*no-op*/
         NULL);                  /* no max. cardinality */

  // This is the number of rows we think we will return
  myEstProps->setResultCardinality(udfCard);

  // From the given ColStatDescList, populate columnStats with column
  // descriptors that are useful based on the characteristic outputs
  // for the group. 
  myEstProps->pickOutputs(udfColStatDescList,
                          inputEstLogProp, 
                          getGroupAttr()->getCharacteristicOutputs(),
                          predsEvaluatedForStats);

  // attach histograms to group attributes
  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstProps);
};

  ///////////////////////////////////////////////////////////

NABoolean TableMappingUDF::pilotAnalysis(QueryAnalysis* qa)
{
  return RelExpr::pilotAnalysis(qa);
};
void TableMappingUDF::jbbAnalysis(QueryAnalysis* qa)
{
  RelExpr::jbbAnalysis(qa);
};
void TableMappingUDF::jbbJoinDependencyAnalysis(ValueIdSet & predsWithDependencies)
{
  RelExpr::jbbJoinDependencyAnalysis(predsWithDependencies);
};
void TableMappingUDF::predAnalysis(QueryAnalysis* qa)
{
  RelExpr::predAnalysis(qa);
};
RelExpr* TableMappingUDF::convertToMultiJoinSubtree(QueryAnalysis* qa)
{
  return RelExpr::convertToMultiJoinSubtree(qa);
};
RelExpr* TableMappingUDF::expandMultiJoinSubtree()
{
 return RelExpr::expandMultiJoinSubtree();
};
void TableMappingUDF::analyzeInitialPlan()
{
  RelExpr::analyzeInitialPlan();
};

  ///////////////////////////////////////////////////////////

  // ---------------------------------------------------------------------
  // comparison, hash, and copy methods
  // ---------------------------------------------------------------------

SimpleHashValue TableMappingUDF::hash()
{
  return RelExpr::hash(); 
}; // from ExprNode
HashValue TableMappingUDF::topHash()
{
  HashValue result = RelRoutine::topHash();
  for (Int32 i=0; i<getArity(); i++)
  {
    result ^= (childInfo_[i]->getPartitionBy());
    result ^= (childInfo_[i]->getOrderBy());
  }
  return result;
};
NABoolean TableMappingUDF::patternMatch(const RelExpr & other) const
{
  // will need additional code for CQS
  return (RelExpr::patternMatch(other) &&
    getArity() == other.getArity());
};
NABoolean TableMappingUDF::duplicateMatch(const RelExpr & other) const
{
  if (!TableValuedFunction::duplicateMatch(other))
    return FALSE;

  TableMappingUDF &o = (TableMappingUDF &) other;

  for (Int32 i=0; i<getArity(); i++)
  {
    if (NOT ((childInfo_[i]->getPartitionBy()) == (o.childInfo_[i]->getPartitionBy()))) 
      return FALSE;
    if (NOT ((childInfo_[i]->getOrderBy()) == (o.childInfo_[i]->getOrderBy()))) 
      return FALSE;
  }

  return TRUE;
};


NABoolean TableMappingUDF::isLogical() const { return TRUE; }
NABoolean TableMappingUDF::isPhysical() const { return FALSE; }

void TableMappingUDF::checkAndCoerceScalarInputParamTypes(BindWA* bindWA)
{
  if (getProcInputParamsVids().entries() != getScalarInputParamCount())
  {
    *CmpCommon::diags() << DgSqlCode(-4452)
                        << DgString0(getUserTableName().getCorrNameAsString())
                        << DgInt0((Lng32) getScalarInputParamCount())
                        << DgInt1((Lng32) getProcInputParamsVids().entries());
    
    bindWA->setErrStatus();
    return ;
  }

  for(Int32 i =0; i < getScalarInputParamCount(); i++)
  {
    const NAType &paramType = *(getScalarInputParams()[i]->getType());
    ValueId inputTypeId = getProcInputParamsVids()[i];

    // If function argument is character type, get detailed info.
    if (inputTypeId.getType().getTypeQualifier() == NA_CHARACTER_TYPE)
    {
      const CharType* stringLiteral = (CharType*)&(inputTypeId.getType());

      if(CmpCommon::wantCharSetInference())
      {
	const CharType* desiredType =
	CharType::findPushDownCharType(((CharType&)paramType).getCharSet(),
								stringLiteral, 0);
	if ( desiredType )
	inputTypeId.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
      }
    }
    // Get final type of function argument.
    const NAType &inputType = inputTypeId.getType();


    // Do type checking,
    // If it is not a compatible type report an error
    if (!( NA_UNKNOWN_TYPE == inputType.getTypeQualifier() ||
	   paramType.isCompatible(inputType) ||
	   inputTypeId.getItemExpr()->getOperatorType() == ITM_DYN_PARAM))
    {
      // Error, data types dont match
      // Create error for user-defined function.
      *CmpCommon::diags() << DgSqlCode(-4455)
                          << DgInt0((Lng32) i+1)
                          << DgString0(getUserTableName().getCorrNameAsString())
                          << DgString1(inputType.getTypeSQLname(TRUE))
                          << DgString2(paramType.getTypeSQLname(TRUE));

      bindWA->setErrStatus();
      return;
    } // if NOT isCompatible

    if ( ! ( paramType == inputType) )
    {
      // Create a Cast node to cast the argument from the function call
      // to the type specified by the metadata (if the two are compatible.)
      Cast *castExpr = new (bindWA->wHeap()) 
	      Cast(inputTypeId.getItemExpr(), &paramType, ITM_CAST, TRUE);
      ItemExpr *boundCast = castExpr->bindNode(bindWA);
      if (bindWA->errStatus()) return;

      // Add to input vid list.
      getProcInputParamsVids().removeAt(i);
      getProcInputParamsVids().insertAt(i,boundCast->getValueId());
    } 
  }
}

void TableMappingUDF::createOutputVids(BindWA* bindWA)
{

  // RETDesc has already been allocated
  RETDesc *resultTable = getRETDesc();
  if (!resultTable)
  {
    bindWA->setErrStatus();
    return;
  }
  for(Int32 i =0; i < getOutputParamCount(); i++)
  {
    NAColumn &naColumn = *(getOutputParams()[i]);
    const NAType &paramType = *(naColumn.getType());
    // use a NamedTypeToItem, so EXPLAIN and other
    // tools show a name instead of just the type
    NamedTypeToItem *paramTypeItem =
      new (bindWA->wHeap()) NamedTypeToItem(
           naColumn.getColName().data(),
           naColumn.mutateType(),
           bindWA->wHeap());
    ItemExpr *outputExprToBind = NULL;
    outputExprToBind = paramTypeItem->bindNode (bindWA);
    if ( bindWA->errStatus()) return;

    // Fill the ValueIdList for the output params
    addProcOutputParamsVid(outputExprToBind->getValueId());

    const NAString &formalParamName = naColumn.getColName();
    const NAString *colParamName = &formalParamName;
    ColRefName *columnName = 
        new (bindWA->wHeap())
            ColRefName(*colParamName, bindWA->wHeap());
    resultTable->addColumn(bindWA, *columnName, outputExprToBind->getValueId());
  }
  return ;
}
	
NARoutine * TableMappingUDF::getRoutineMetadata(
     QualifiedName &routineName,
     CorrName &tmfuncName,
     BindWA *bindWA)
{
  NARoutine *result = NULL;
  CmpSeabaseDDL cmpSBD((NAHeap *)bindWA->wHeap());

  TrafDesc *tmudfMetadata =
    cmpSBD.getSeabaseRoutineDesc(
         routineName.getCatalogName(),
         routineName.getSchemaName(),
         routineName.getObjectName());

  // IS req 5: If function name not found, output binder error.
  if (NULL == tmudfMetadata)
  {
    *CmpCommon::diags() << DgSqlCode(-4450)
			<< DgString0(tmfuncName.getQualifiedNameAsString());
    bindWA->setErrStatus();
    return NULL;
  }

  ComRoutineType udfType ;

  udfType = tmudfMetadata->routineDesc()->UDRType ;

  // IS req 6: Check ROUTINE_TYPE column of ROUTINES table.
  // Emit error if invalid type.
  if (udfType != COM_TABLE_UDF_TYPE)
  {
    *CmpCommon::diags() << DgSqlCode(-4457)
			<< DgString0(tmfuncName.getQualifiedNameAsString())
			<< DgString1("Only table-valued functions are supported here");
    bindWA->setErrStatus();
    return NULL;
  } 

  // IS req 3, 7.3. Instantiate NARoutine object.  
  // NARoutine data members will be assigned from udfMetadata.
  Int32 errors=0;
  result = new (bindWA->wHeap())
    NARoutine(tmfuncName.getQualifiedNameObj(),
	      tmudfMetadata, 
	      bindWA,
	      errors,
	      bindWA->wHeap());
  if ( NULL == result || errors != 0)
  {
    bindWA->setErrStatus();
    return NULL;
  }

  return result;
}



// -----------------------------------------------------------------------
// methods for class PredefinedTableMappingFunction
// -----------------------------------------------------------------------

PredefinedTableMappingFunction::PredefinedTableMappingFunction(
       int arity,
       const CorrName &name,
       ItemExpr *params,
       OperatorTypeEnum otype,
       CollHeap *oHeap) :
     TableMappingUDF(name, params, arity, otype, oHeap)
{}

PredefinedTableMappingFunction::~PredefinedTableMappingFunction()
{}

// static method to find out whether a given name is a
// built-in table-mapping function - returns operator type
// of predefined function if so, REL_ANY_TABLE_MAPPING_UDF otherwise
OperatorTypeEnum PredefinedTableMappingFunction::nameIsAPredefinedTMF(const CorrName &name)
{
  // Predefined functions don't reside in a schema, they are referenced with an unqualified name
  const QualifiedName &qualName = name.getQualifiedNameObj();
  const NAString &funcName = qualName.getObjectName();

  if (! qualName.getSchemaName().isNull())
    return REL_ANY_TABLE_MAPPING_UDF;

  if (funcName == "EVENT_LOG_READER")
    return REL_TABLE_MAPPING_BUILTIN_LOG_READER;
  else if (funcName == "TIMESERIES")
    return REL_TABLE_MAPPING_BUILTIN_TIMESERIES;
  else if (funcName == "JDBC")
    return REL_TABLE_MAPPING_BUILTIN_JDBC;
  else
    // none of the built-in names matched, so it must be a UDF
    return REL_ANY_TABLE_MAPPING_UDF;
}

const NAString PredefinedTableMappingFunction::getText() const
{
  switch (getOperatorType())
    {
    case REL_TABLE_MAPPING_BUILTIN_LOG_READER:
      return "event_log_reader";
    case REL_TABLE_MAPPING_BUILTIN_TIMESERIES:
      return "timeseries";
    case REL_TABLE_MAPPING_BUILTIN_JDBC:
      return "jdbc_udf";

    default:
      CMPASSERT(0);
      return "predefined_tmf_with_invalid_operator_type";
    }
}

PredefinedTableMappingFunction * PredefinedTableMappingFunction::castToPredefinedTableMappingFunction()
{
  return this;
}

// a virtual function used to copy most of a Node
RelExpr * PredefinedTableMappingFunction::copyTopNode(RelExpr *derivedNode,
                                                      CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new(outHeap) PredefinedTableMappingFunction(
         getArity(),
         getUserTableName(),
         NULL, // params get copied by parent classes
         getOperatorType(),
         outHeap);
  else
    result = derivedNode;

  return TableMappingUDF::copyTopNode(derivedNode, outHeap);
}

NARoutine * PredefinedTableMappingFunction::getRoutineMetadata(
     QualifiedName &routineName,
     CorrName &tmfuncName,
     BindWA *bindWA)
{
  NARoutine *result = NULL;
  ComRoutineLanguage lang = COM_LANGUAGE_CPP;
  ComRoutineParamStyle paramStyle = COM_STYLE_CPP_OBJ;
  const char *externalName = NULL;
  // By default, predefined UDRs share a DLL.
  const char *libraryFileName = "libudr_predef.so";
  NAString libraryPath;

  // the libraries for predefined UDRs are in the regular
  // library directory $TRAF_HOME/export/lib${SQ_MBTYPE}
  libraryPath += getenv("TRAF_HOME");
  libraryPath += "/export/lib";

  switch (getOperatorType())
    {
    case REL_TABLE_MAPPING_BUILTIN_LOG_READER:
      externalName = "TRAF_CPP_EVENT_LOG_READER";
      libraryPath += getenv("SQ_MBTYPE");
      break;

    case REL_TABLE_MAPPING_BUILTIN_TIMESERIES:
      externalName = "TRAF_CPP_TIMESERIES";
      libraryPath += getenv("SQ_MBTYPE");
      break;

    case REL_TABLE_MAPPING_BUILTIN_JDBC:
      lang = COM_LANGUAGE_JAVA;
      paramStyle = COM_STYLE_JAVA_OBJ;
      externalName = "org.trafodion.sql.udr.predef.JDBCUDR";
      libraryPath += "/trafodion-sql-";
      libraryPath += getenv("TRAFODION_VER");
      libraryPath += ".jar";
      break;

    default:
      *CmpCommon::diags() << DgSqlCode(-4457)
                          << DgString0(routineName.getQualifiedNameAsString())
                          << DgString1("Failed to get metadata for predefined UDF");
      bindWA->setErrStatus();
      return NULL;
    }
 
  // Produce a very simple NARoutine, most of the
  // error checking and determination of output
  // columns is done by the compiler interface of
  // this predefined table mapping function.

  result = new(bindWA->wHeap()) NARoutine(routineName,
                                          bindWA->wHeap());
  result->setExternalName(externalName);
  result->setLanguage(lang);
  result->setRoutineType(COM_TABLE_UDF_TYPE);
  result->setParamStyle(paramStyle);
  result->setFile(libraryFileName);
  result->setExternalPath(libraryPath);

  return result;
}


// -----------------------------------------------------------------------
// methods for class PhysicalTableMappingUDF
// -----------------------------------------------------------------------
double PhysicalTableMappingUDF::getEstimatedRunTimeMemoryUsage(Generator *generator, ComTdb * tdb) {return 0;}

RelExpr * PhysicalTableMappingUDF::copyTopNode(RelExpr *derivedNode,
                                               CollHeap* outHeap)
{
  PhysicalTableMappingUDF *result;

  if (derivedNode == NULL)
    result = new(outHeap) PhysicalTableMappingUDF(getArity(), outHeap);
  else
    result = (PhysicalTableMappingUDF *) derivedNode;

  result->planInfo_ = planInfo_;

  return TableMappingUDF::copyTopNode(result, outHeap);
}

//! PhysicalTableMappingUDF::getText method
const NAString PhysicalTableMappingUDF::getText() const
{
	return TableMappingUDF::getText();
}

// -----------------------------------------------------------------------
// methods for class BuiltinTableValuedFunction
// -----------------------------------------------------------------------

//! BuiltinTableValuedFunction::~BuiltinTableValuedFunction Destructor 
BuiltinTableValuedFunction::~BuiltinTableValuedFunction()
{
}

//! BuiltinTableValuedFunction::getText method 
const NAString BuiltinTableValuedFunction::getText() const
{
  return "BuiltinTableValuedFunction";
}

// -----------------------------------------------------------------------
// methods for class TableValuedUDF
// -----------------------------------------------------------------------

//! TableValuedUDF::~TableValuedUDF Destructor 
TableValuedUDF::~TableValuedUDF()
{
}



//! TableValuedUDF::copyTopNode method 
RelExpr * TableValuedUDF::copyTopNode(RelExpr *derivedNode,
                                      CollHeap* outHeap)
{
   TableValuedUDF      *result;

   if (derivedNode == NULL)
   {
     result  = new (outHeap) TableValuedUDF(NULL,
                               getOperatorType(),
                               outHeap);
   }
   else
     result = (TableValuedUDF *) derivedNode;


   return TableValuedFunction::copyTopNode(result, outHeap);

}

//! TableValuedUDF::getText method 
const NAString TableValuedUDF::getText() const
{
  return "TableValuedUDF";
}



// -----------------------------------------------------------------------
// methods for class IsolatedNonTableUDR
// -----------------------------------------------------------------------

//! IsolatedNonTableUDR::~IsolatedNonTableUDR Destructor 
IsolatedNonTableUDR::~IsolatedNonTableUDR()
{
}

//! IsolatedNonTableUDR::getText method 
const NAString IsolatedNonTableUDR::getText() const
{
  return "IsolatedNonTableUDR";
}

//! IsolatedNonTableUDR::copyTopNode method 
RelExpr *IsolatedNonTableUDR::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  IsolatedNonTableUDR *result;
  if (derivedNode == NULL)
    result = new (outHeap) IsolatedNonTableUDR(getRoutineName(), outHeap);
  else
    result = (IsolatedNonTableUDR *) derivedNode;

  result->neededValueIds_ = neededValueIds_;

  return RelRoutine::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// methods for class IsolatedScalarUDF
// -----------------------------------------------------------------------

//! IsolatedScalarUDF::~IsolatedScalarUDF Destructor 
IsolatedScalarUDF::~IsolatedScalarUDF()
{
}

//! IsolatedScalarUDF::getText method 
const NAString IsolatedScalarUDF::getText() const
{

  NAString name("Isolated_Scalar_UDF ");
  name += getRoutineName().getQualifiedNameAsAnsiString();
  if (getRoutineDesc() && getRoutineDesc()->isUUDFRoutine() == TRUE &&
      getRoutineDesc()->getActionNARoutine() && 
      getRoutineDesc()->getActionNARoutine()->getActionName())
  {
     name += ":";
     name += *(getRoutineDesc()->getActionNARoutine()->getActionName());
  }
  return name;
}

//! IsolatedScalarUDF::copyTopMethod method 
RelExpr *IsolatedScalarUDF::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  IsolatedScalarUDF *result;

  if (derivedNode == NULL)
  {
    result = new (outHeap) IsolatedScalarUDF(getRoutineName(),
                                       outHeap);
  }
  else
  {
    result = (IsolatedScalarUDF *) derivedNode;
  }


  return super::copyTopNode(result, outHeap);
}

//! IsolatedScalarUDF::addLocalExpr method
void IsolatedScalarUDF::addLocalExpr(LIST(ExprNode *) &xlist,
                                     LIST(NAString) &llist) const
{

  RelRoutine::addLocalExpr(xlist,llist);
}

//! IsolatedScalarUDF::getPotentialOutputValues method 
void IsolatedScalarUDF::getPotentialOutputValues(ValueIdSet &vs) const
{
  vs.clear();
  vs.insertList(getProcOutParams());
}


// -----------------------------------------------------------------------
// methods for class PhysicalIsolatedScalarUDF
// -----------------------------------------------------------------------
//! PhysicalIsolatedScalarUDF::PhysicalIsolatedScalarUDF Constructor 
PhysicalIsolatedScalarUDF::PhysicalIsolatedScalarUDF(CollHeap *heap)
  : IsolatedScalarUDF(NULL, heap)
{
}

// -----------------------------------------------------------------------
// methods for class ExplainFunc
// -----------------------------------------------------------------------

//! ExplainFunc::~ExplainFunc Destructor 
ExplainFunc::~ExplainFunc()
{
}

//! ExplainFunc::copyTopNode method 
RelExpr * ExplainFunc::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ExplainFunc *result;

  if (derivedNode == NULL)
    result = new (outHeap) ExplainFunc(NULL,outHeap);
  else
    result = (ExplainFunc *) derivedNode;

  return BuiltinTableValuedFunction::copyTopNode(result, outHeap);
}


//! ExplainFunc::getText method 
const NAString ExplainFunc::getText() const
{
  return "explain";
}

// -----------------------------------------------------------------------
// methods for class HiveMDaccessFunc
// -----------------------------------------------------------------------

HiveMDaccessFunc::HiveMDaccessFunc(NAString *mdt,
                                   NAString* schName,
                                   NAString* objName,
                                   CollHeap *oHeap)
  : BuiltinTableValuedFunction(NULL,REL_HIVEMD_ACCESS,oHeap)
{
  if (mdt)
    mdType_ = *mdt;
  if (schName)
    schemaName_ = *schName;
  if (objName)
    objectName_ = *objName;
}

//! HiveMDaccessFunc::~HiveMDaccessFunc Destructor 
HiveMDaccessFunc::~HiveMDaccessFunc()
{
}

//! HiveMDaccessFunc::copyTopNode method 
RelExpr * HiveMDaccessFunc::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  HiveMDaccessFunc *result;

  if (derivedNode == NULL)
    result = new (outHeap) HiveMDaccessFunc(NULL,NULL,NULL,outHeap);
  else
    result = (HiveMDaccessFunc *) derivedNode;

  result->mdType_ = mdType_;
  result->schemaName_ = schemaName_;
  result->objectName_ = objectName_;

  return BuiltinTableValuedFunction::copyTopNode(result, outHeap);
}

RelExpr *HiveMDaccessFunc::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }

  RelExpr * re = BuiltinTableValuedFunction::bindNode(bindWA);
  if (bindWA->errStatus())
    return this;

  return re;
}


//! HiveMDaccessFunc::getText method 
const NAString HiveMDaccessFunc::getText() const
{
  return "hiveMD";
}

void
HiveMDaccessFunc::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp))
    return;

  // Create a new Output Log Property with cardinality of 10 for now.
  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(10));

  getGroupAttr()->addInputOutputLogProp(inputEstLogProp, myEstProps);

} // HiveMDaccessFunc::synthEstLogProp

void
HiveMDaccessFunc::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  //  for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  RelExpr::synthLogProp(normWAPtr);

} // HiveMDaccessFunc::synthLogProp()

//<pb>
//==============================================================================
//  Synthesize physical properties for HiveMD operator's current plan
// extracted from a spcified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
HiveMDaccessFunc::synthPhysicalProperty(const Context* myContext,
                                        const Lng32     planNumber,
                                        PlanWorkSpace  *pws)
{

  //----------------------------------------------------------
  // Create a node map with a single, active, wild-card entry.
  //----------------------------------------------------------
  NodeMap* myNodeMap = new(CmpCommon::statementHeap())
                        NodeMap(CmpCommon::statementHeap(),
                                1,
                                NodeMapEntry::ACTIVE);

  //------------------------------------------------------------
  // Synthesize a partitioning function with a single partition.
  //------------------------------------------------------------
  PartitioningFunction* myPartFunc =
    new(CmpCommon::statementHeap())
    SinglePartitionPartitioningFunction(myNodeMap);

  PhysicalProperty * sppForMe =
    new(CmpCommon::statementHeap())
    PhysicalProperty(myPartFunc,
                     EXECUTE_IN_MASTER,
                     SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe ;

} // HiveMDaccessFunc::synthPhysicalProperty()


// -----------------------------------------------------------------------
// methods for class StatisticsFunc
// -----------------------------------------------------------------------
//! StatisticsFunc::copyTopNode method 
RelExpr * StatisticsFunc::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  StatisticsFunc *result;

  if (derivedNode == NULL)
    result = new (outHeap) StatisticsFunc(NULL,outHeap);
  else
    result = (StatisticsFunc *) derivedNode;

  return BuiltinTableValuedFunction::copyTopNode(result, outHeap);
}

//! StatisticsFunc::getText method 
const NAString StatisticsFunc::getText() const
{
  return "statistics";
}

// -----------------------------------------------------------------------
// methods for class ProxyFunc
// -----------------------------------------------------------------------

//! ProxyFunc::ProxyFunc Constructor 
ProxyFunc::ProxyFunc(OperatorTypeEnum otype,
                     ElemDDLNode *columnsFromParser,
                     CollHeap *heap)
  : BuiltinTableValuedFunction(NULL, otype, heap),
    columnListFromParser_(columnsFromParser),
    virtualTableName_("", heap)
{
}

//! ProxyFunc::ProxyFunc Copy Constructor 
ProxyFunc::ProxyFunc(const ProxyFunc &other, CollHeap *h)
  : BuiltinTableValuedFunction(other, h),
    columnListFromParser_(other.columnListFromParser_),
    virtualTableName_(other.virtualTableName_, h)
{
}


//! ProxyFunc::~ProxyFunc Destructor 
ProxyFunc::~ProxyFunc()
{
}

//! ProxyFunc::getVirtualTableName method 
const char *ProxyFunc::getVirtualTableName()
{
  return virtualTableName_.data();
}

//! ProxyFunc::getNumColumns method 
ComUInt32 ProxyFunc::getNumColumns() const
{
  ComUInt32 result = 0;
  if (columnListFromParser_)
    result = columnListFromParser_->entries();
  return result;
}

//! ProxyFunc::getColumn method 
const ElemProxyColDef &ProxyFunc::getColumn(ComUInt32 i) const
{
  CMPASSERT(i < getNumColumns());
  ElemProxyColDef *result =
    (*columnListFromParser_)[i]->castToElemProxyColDef();
  CMPASSERT(result);
  return *result;
}

//! ProxyFunc::getColumnType method 
const NAType &ProxyFunc::getColumnType(ComUInt32 i) const
{
  const ElemProxyColDef &c = getColumn(i);
  const NAType *t = c.getColumnDataType();
  CMPASSERT(t);
  return *t;
}

//! ProxyFunc::getColumnNameForDescriptor method 
const NAString *ProxyFunc::getColumnNameForDescriptor(ComUInt32 i) const
{
  const ElemProxyColDef &c = getColumn(i);
  const NAString &s = c.getColumnName();
  return &s;
}

//! ProxyFunc::getTableNameForDescriptor method 
const QualifiedName *ProxyFunc::getTableNameForDescriptor(ComUInt32 i) const
{
  const ElemProxyColDef &c = getColumn(i);
  const QualifiedName *qn = c.getTableName();
  return qn;
}

//! ProxyFunc::getColumnHeading method 
const NAString *ProxyFunc::getColumnHeading(ComUInt32 i) const
{
  const ElemProxyColDef &c = getColumn(i);
  if (c.getIsHeadingSpecified())
  {
    const NAString &s = c.getHeading();
    return &s;
  }
  return NULL;
}

//! ProxyFunc::populateColumnDesc method 
void ProxyFunc::populateColumnDesc(char *tableNam,
                                   TrafDesc *&colDescs,
                                   Lng32 &reclen) const
{
    ElemDDLColDefArray colArray;
    ComObjectName tableName(tableNam);
    ComUInt32 numCols = getNumColumns();

  
    for (ComUInt32 colNum = 0; colNum < numCols; colNum++)
    {
      ElemProxyColDef *result =
        (*columnListFromParser_)[colNum]->castToElemProxyColDef();

      colArray.insert(result);
    }

   CmpSeabaseDDL cmpSBD(CmpCommon::statementHeap());

   ComTdbVirtTableColumnInfo * colInfoArray = (ComTdbVirtTableColumnInfo*)
                      new(STMTHEAP) char[numCols * sizeof(ComTdbVirtTableColumnInfo)];

   cmpSBD.buildColInfoArray(COM_USER_DEFINED_ROUTINE_OBJECT,
                            &colArray, 
                            colInfoArray, 
                            FALSE, FALSE, NULL, NULL, NULL, NULL,
                            CmpCommon::statementHeap());

   for (size_t colNum = 0; colNum < colArray.entries(); colNum++)
   {
     char buf[128];
     str_sprintf(buf, "C%d", (Int32) colNum + 1);
     const char *colName = &buf[0];
     char * col_name = new(STMTHEAP) char[strlen(colName) + 1];
     strcpy(col_name, (char*)colName);
     colInfoArray[colNum].colName = col_name;
   }

   colDescs = cmpSBD.convertVirtTableColumnInfoArrayToDescStructs(&tableName,
                                                                  colInfoArray,
                                                                  numCols) ;

  // calculate the record length 
  TrafDesc *tempDescs = colDescs;
  for (ComUInt32 colNum = 0; colNum < numCols; colNum++)
  { 
    NAType *naType = getColumn(colNum).getColumnDataType();
    Int32 fsType = naType->getFSDatatype();

    if (tempDescs->columnsDesc()->isNullable())
      reclen += SQL_NULL_HDR_SIZE;
    if (DFS2REC::isSQLVarChar(fsType))
      reclen += SQL_VARCHAR_HDR_SIZE;

    reclen += tempDescs->columnsDesc()->length;
    tempDescs = tempDescs->next;
  }
}


//! ProxyFunc::copyTopNode method 
RelExpr *ProxyFunc::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  ProxyFunc *result;
  if (derivedNode == NULL)
    result = new (outHeap) ProxyFunc(*this);
  else
    result = (ProxyFunc *) derivedNode;
 
    // make a shallow copy of the parse tree struct. This was originally 
    // instantiated as a shallow copy from the parser too.
    // assumption is that is is read only and that it does not get freed...
  result->columnListFromParser_ = columnListFromParser_;
  result->virtualTableName_ = virtualTableName_;

  return TableValuedFunction::copyTopNode(result, outHeap);
}

//! ProxyFunc::topHash method 
HashValue ProxyFunc::topHash()
{
  HashValue result = BuiltinTableValuedFunction::topHash();
  result ^= columnListFromParser_;
  result ^= virtualTableName_.hash();
  return result;
}

//! ProxyFunc::duplicateMatch method 
NABoolean ProxyFunc::duplicateMatch(const RelExpr & other) const
{
  if (!BuiltinTableValuedFunction::duplicateMatch(other))
    return FALSE;
  ProxyFunc &o = (ProxyFunc &) other;
  if (columnListFromParser_ != o.columnListFromParser_)
    return FALSE;
  if (virtualTableName_ != o.virtualTableName_)
    return FALSE;
  return TRUE;
}

// -----------------------------------------------------------------------
// methods for class SPProxyFunc
// -----------------------------------------------------------------------
//! SPProxyFunc::SPProxyFunc Constructor 
SPProxyFunc::SPProxyFunc(ElemDDLNode *columnsFromParser,
                         ItemExprList *optionalStrings,
                         CollHeap *heap)
  : ProxyFunc(REL_SP_PROXY, columnsFromParser, heap),
    optionalStrings_(optionalStrings)
{
}

//! SPProxyFunc::SPProxyFunc Constructor 
SPProxyFunc::SPProxyFunc(CollHeap *heap)
  : ProxyFunc(REL_SP_PROXY, NULL, heap),
    optionalStrings_(NULL)
{
}

//! SPProxyFunc::SPProxyFunc Copy Constructor 
SPProxyFunc::SPProxyFunc(const SPProxyFunc &other, CollHeap *h)
  : ProxyFunc(other,h),
    optionalStrings_(other.optionalStrings_)
{
}

//! SPProxyFunc::~SPProxyFunc Destructor 
SPProxyFunc::~SPProxyFunc()
{
}

//! SPProxyFunc::getNumOptionalStrings method 
ComUInt32 SPProxyFunc::getNumOptionalStrings() const
{
  ComUInt32 result = 0;
  if (optionalStrings_)
    result = optionalStrings_->entries();
  return result;
}

//! SPProxyFunc::getOptionalString method 
const char *SPProxyFunc::getOptionalString(ComUInt32 i) const
{
  const char *result = NULL;
  const ItemExpr *e = NULL;
  const ConstValue *cv = NULL;
  const NAString *s = NULL;
  NABoolean dummyNegate = FALSE;

  if (i < getNumOptionalStrings())
    e = (*optionalStrings_)[i];

  if (e)
    cv = ((ItemExpr *) e)->castToConstValue(dummyNegate);

  if (cv)
    s = cv->getRawText();

  if (s)
    result = s->data();

  return result;
}

//! SPProxyFunc::copyTopNode method 
RelExpr *SPProxyFunc::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  SPProxyFunc *result;
  if (derivedNode == NULL)
    result = new (outHeap) SPProxyFunc(*this);
  else
    result = (SPProxyFunc *) derivedNode;

  // Make a shallow copy of the pointer.
  result->optionalStrings_ = optionalStrings_;

  return ProxyFunc::copyTopNode(result, outHeap);
}

//! SPProxyFunc::getText method 
const NAString SPProxyFunc::getText() const
{
  return "sp_result_set";
}

//! SPProxyFunc::topHash method 
HashValue SPProxyFunc::topHash()
{
  HashValue result = ProxyFunc::topHash();
  result ^= optionalStrings_;
  return result;
}

//! SPProxyFunc::duplicateMatch method 
NABoolean SPProxyFunc::duplicateMatch(const RelExpr & other) const
{
  if (!ProxyFunc::duplicateMatch(other))
    return FALSE;
  SPProxyFunc &o = (SPProxyFunc &) other;
  if (optionalStrings_ != o.optionalStrings_)
    return FALSE;
  return TRUE;
}

// -----------------------------------------------------------------------
// methods for class CallSP
// -----------------------------------------------------------------------

// Raj P - 12/2000
//! CallSP::copyTopNode method 
// virtual copy constructor for the CallSP class
RelExpr *CallSP::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  CallSP *result;
  if (derivedNode == NULL)
    result = new (outHeap) CallSP(getRoutineName(), outHeap);
  else
    result = (CallSP *) derivedNode;

  result->duplicateVars_ = duplicateVars_;
  result->outDupVars_ = outDupVars_;
  result->hasUDF_ = hasUDF_;

  return super::copyTopNode(result, outHeap);
}


//! CallSP::getPotentialOutputValues method 
void CallSP::getPotentialOutputValues(ValueIdSet &vs) const
{
  vs.clear();
  vs.insertList(getProcOutputParamsVids());
}

//! CallSP::getText method 
const NAString CallSP::getText() const
{
  NAString result("call ", CmpCommon::statementHeap ());
  result += getRoutineName().getQualifiedNameAsString();
  if (nodeIsBound())
  {
    getProcAllParamsVids().unparse(result);
  }
  else
  {
    result += "(";
    if (getProcAllParamsTree() != NULL)
    {
      getProcAllParamsTree()->unparse(result);
    }
    result += ")";
  }
  return result;
}

//! CallSP::pushdownCoveredExpr method 
void CallSP::pushdownCoveredExpr(const ValueIdSet &outputExprOnOperator,
			    const ValueIdSet &newExternalInputs,
			    ValueIdSet &predicatesOnOperator,
			    const ValueIdSet *nonPredNonOutputExprOnOperator,
			    Lng32 childIndex
			   )
{

  // We use an emptySet for the outputs for the call to 
  // Relexpr::pushdowCoveredExpr because our child is a tuple with a
  // subquery in it and that subquery is an input to the Routine, so none
  // of the CallSPs output pertains to the child. 
  ValueIdSet emptySet;  

  RelExpr::pushdownCoveredExpr( emptySet,
				newExternalInputs,
				predicatesOnOperator,
				&getNeededValueIds());
}

// -----------------------------------------------------------------------
// methods for class ExtractSource
// -----------------------------------------------------------------------
//! ExtractSource::ExtractSource Constructor
ExtractSource::ExtractSource(ElemDDLNode *columnsFromParser,
                             NAString &espPhandle,
                             NAString &securityKey,
                             CollHeap *heap)
  : ProxyFunc(REL_EXTRACT_SOURCE, columnsFromParser, heap),
    espPhandle_(espPhandle, heap),
    securityKey_(securityKey, heap)
{
}

//! ExtractSource::ExtractSource Constructor
ExtractSource::ExtractSource(CollHeap *heap)
  : ProxyFunc(REL_EXTRACT_SOURCE, NULL, heap),
    espPhandle_("", heap),
    securityKey_("", heap)
{
}
//! ExtractSource::ExtractSource Copy Constructor
ExtractSource::ExtractSource(const ExtractSource &other, CollHeap *h)
  : ProxyFunc(other,h),
    espPhandle_(other.espPhandle_),
    securityKey_(other.securityKey_)
{
}


//! ExtractSource::~ExtractSource Destructor
ExtractSource::~ExtractSource()
{
}

//! ExtractSource::copyTopNode method
RelExpr *ExtractSource::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  ExtractSource *result;
  if (derivedNode == NULL)
    result = new (outHeap) ExtractSource(*this, outHeap);
  else
    result = (ExtractSource *) derivedNode;

  result->espPhandle_ = espPhandle_;
  result->securityKey_ = securityKey_;

  return ProxyFunc::copyTopNode(result, outHeap);
}

//! ExtractSource::getText method
const NAString ExtractSource::getText() const
{
  return "extract_source";
}

//! ExtractSource::topHash method
HashValue ExtractSource::topHash()
{
  HashValue result = ProxyFunc::topHash();
  result ^= espPhandle_.hash();
  result ^= securityKey_.hash();
  return result;
}

//! ExtractSource::duplicateMatch method
NABoolean ExtractSource::duplicateMatch(const RelExpr & other) const
{
  if (!ProxyFunc::duplicateMatch(other))
    return FALSE;
  ExtractSource &o = (ExtractSource &) other;
  if (espPhandle_ != o.espPhandle_)
    return FALSE;
  if (securityKey_ != o.securityKey_)
    return FALSE;
  return TRUE;
}
