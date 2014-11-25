/**********************************************************************
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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

    llist.insert("parameters");
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

// -----------------------------------------------------------------------
// methods for class TableMappingUDF
// -----------------------------------------------------------------------

//! TableMappingUDF::TableMappingUDF Copy Constructor
TableMappingUDF::TableMappingUDF(const TableMappingUDF & other)
: TableValuedFunction(other)
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
  UDFOutputToChildInputMap_ = UDFOutputToChildInputMap_;
  outputChildIndexList_ = outputChildIndexList_;
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
     result = new (outHeap) TableMappingUDF(NULL,
                               getOperatorType(),
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
  result->UDFOutputToChildInputMap_ = UDFOutputToChildInputMap_;
  result->outputChildIndexList_ = outputChildIndexList_;
  return TableValuedFunction::copyTopNode(result, outHeap);
}

//! TableMappingUDF::getText method
const NAString TableMappingUDF::getText() const
{
 NAString op(CmpCommon::statementHeap());
 op = "tmudf ";
 return op + getUserTableName().getTextWithSpecialType();
}

void TableMappingUDF::addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const
{ 
  for(CollIndex i = 0; i < childInfo_.entries(); i++)
  {
    if (NOT childInfo_[i]->partitionBy_.isEmpty())
    {
      xlist.insert(childInfo_[i]->partitionBy_.rebuildExprTree());
      llist.insert("child partBy");
    }
    if (NOT childInfo_[i]->orderBy_.isEmpty())
    {
      xlist.insert(childInfo_[i]->orderBy_.rebuildExprTree());
      llist.insert("child orderBy");
    }
    if (NOT childInfo_[i]->outputs_.isEmpty())
    {
      xlist.insert(childInfo_[i]->outputs_.rebuildExprTree());
      llist.insert("child outputs");
    }
  } 
  RelRoutine::addLocalExpr(xlist,llist);
};

void TableMappingUDF::transformNode(NormWA & normWARef,
    ExprGroupId & locationOfPointerToMe) 
{
  RelExpr::transformNode(normWARef, locationOfPointerToMe);
};
void TableMappingUDF::rewriteNode(NormWA & normWARef)
{
  for(Int32 i = 0; i < getArity(); i++)
  {
    childInfo_[i]->partitionBy_.normalizeNode(normWARef);
    childInfo_[i]->orderBy_.normalizeNode(normWARef);
    childInfo_[i]->outputs_.normalizeNode(normWARef);
  }
  // rewrite group attributes and selection pred
  RelExpr::rewriteNode(normWARef);
} ;
RelExpr * TableMappingUDF::normalizeNode(NormWA & normWARef)
{
  return RelExpr::normalizeNode(normWARef);
};
 
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
				   const ValueIdSet *
				      nonPredNonOutputExprOnOperator,
                                   Lng32 childId)
{
  ValueIdSet exprOnParent;
  if(nonPredNonOutputExprOnOperator)
    exprOnParent = *nonPredNonOutputExprOnOperator;
  for (Int32 i=0; i<getArity(); i++)
  {
    exprOnParent.insertList(childInfo_[i]->getOutputs());
  }

  RelExpr::pushdownCoveredExpr(outputExprOnOperator,
                                              newExternalInputs,
                                              predOnOperator,
					      &exprOnParent,
                                              childId);
};


void TableMappingUDF::synthLogProp(NormWA * normWAPtr)
{
  RelExpr::synthLogProp(normWAPtr);
};

void TableMappingUDF::finishSynthEstLogProp()
{
  RelExpr::finishSynthEstLogProp();
};


void TableMappingUDF::synthEstLogProp(const EstLogPropSharedPtr& inputLP)
{
  // get child(ren) histograms
  EstLogPropSharedPtr childrenInputEstLogProp;
  CostScalar inputFromParentCard = inputLP->getResultCardinality();
  
  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(*inputLP));
  CostScalar udfCard = csOne;
  
  // Set tmUdf output cardinality
  // 1. no children: children histograms will have cardinality of 1
  if(getArity() < 1)
  {  
    udfCard = 
      ActiveSchemaDB()->getDefaults().getAsDouble(TMUDF_LEAF_CARDINALITY);
  }
  // 2. one child
  else if(getArity()==1)
  {
    childrenInputEstLogProp = child(0).outputLogProp(inputLP);
    CostScalar udfCardFactor = 
       ActiveSchemaDB()->getDefaults().getAsDouble(TMUDF_CARDINALITY_FACTOR);

    udfCard = childrenInputEstLogProp->getResultCardinality() * udfCardFactor;
  }
  // 3. more than one child
  else{
    // not implemented yet
    CMPASSERT(FALSE);
  }
  

  
  // use child(ren) histograms to figure out rowcount and column uec
  //const ColStatDescList & childColStatsDescList = childrenInputEstLogProp->getColStats();
  ValueIdSet tmUdfOutputs = getGroupAttr()->getCharacteristicOutputs();
  
  // compute output cardinality, it is a function of
  // * the input from the child(ren) 
  // * a factor (specified via a comp_int)
  myEstProps->setResultCardinality(udfCard);
  
  ColStatDescList & udfColStatDescList = myEstProps->colStats();
  
  for(ValueId udfOutputCol=tmUdfOutputs.init(); 
              tmUdfOutputs.next(udfOutputCol);
              tmUdfOutputs.advance(udfOutputCol))
  {
    NABoolean inputHistFound = FALSE;
    ValueId inputValId;
    ColStatsSharedPtr inputColStats;
    
    UDFOutputToChildInputMap_.mapValueIdDown(udfOutputCol, inputValId);

    // if output is not found then inputValId is set to 
    // udfOutputCol in such a case set inputValId to NULL_VALUE_ID
    if(inputValId == udfOutputCol)
      inputValId = NULL_VALUE_ID;
    
    if(inputValId != NULL_VALUE_ID)
    {    
      CollIndex outputLoc = UDFOutputToChildInputMap_.getTopValues().index(udfOutputCol);
      CollIndex producingChild = outputChildIndexList_[outputLoc];
      
      // get child histogram
      inputColStats =
        child(producingChild).
          outputLogProp(inputLP)->getColStats().
            getColStatsPtrForColumn(inputValId);
      
      if(inputColStats != NULL)
        inputHistFound = TRUE;
    }

    if(!inputHistFound)
    {
      double colUec = udfCard.getValue() * 
        ActiveSchemaDB()->getDefaults().getAsDouble(USTAT_MODIFY_DEFAULT_UEC);
      udfColStatDescList.addColStatDescForVirtualCol(colUec,
                                                     udfCard, 
                                                     udfOutputCol,
                                                     udfOutputCol,
                                                     udfOutputCol,
                                                     NULL,
                                                     TRUE);
    }
    else{
      ColStatDescSharedPtr outputColStatDescSharedPtr(
        new(HISTHEAP) ColStatDesc(inputColStats,udfOutputCol, HISTHEAP),HISTHEAP);
      udfColStatDescList.insert(outputColStatDescSharedPtr);
    }
  }
  
  // synchronize output colum rowcounts
  udfColStatDescList.synchronizeStats(udfCard,udfColStatDescList.entries());
  
  // attach histograms to group attributes
  getGroupAttr()->addInputOutputLogProp (inputLP, myEstProps);
  
  //RelExpr::synthEstLogProp(inputLP);
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

DefaultToken TableMappingUDF::getParallelControlSettings (
            const ReqdPhysicalProperty* const rppForMe, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) const 
{ 
  return RelExpr::getParallelControlSettings(rppForMe, 
        numOfESPs, allowedDeviation, numOfESPsForced ); 
};

NABoolean TableMappingUDF::okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) 
{ 
  return RelExpr::okToAttemptESPParallelism(myContext, 
        pws, numOfESPs, allowedDeviation, numOfESPsForced); 
};

PartitioningFunction* TableMappingUDF::mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0) 
{ 
  return RelExpr::mapPartitioningFunction(partFunc, rewriteForChild0);
};

NABoolean TableMappingUDF::isBigMemoryOperator(const Context* context,
                                        const Lng32 /*planNumber*/)
{
  return FALSE;
};

Int32 TableMappingUDF::getArity() const {return RelRoutine::getArity();};

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
    NATypeToItem *paramTypeItem = new (bindWA->wHeap())
                                      NATypeToItem (naColumn.mutateType());
    ItemExpr *outputExprToBind = NULL;
    outputExprToBind = paramTypeItem->bindNode (bindWA);
    if ( bindWA->errStatus()) return;

    // Fill the ValueIdList for the output params
    addProcOutputParamsVids(outputExprToBind->getValueId());

    const NAString &formalParamName = naColumn.getColName();
    const NAString *colParamName = &formalParamName;
    ColRefName *columnName = 
        new (bindWA->wHeap())
            ColRefName(*colParamName, bindWA->wHeap());
    resultTable->addColumn(bindWA, *columnName, outputExprToBind->getValueId());
  }
  return ;
}
	

// -----------------------------------------------------------------------
// methods for class PhysicalTableMappingUDF
// -----------------------------------------------------------------------
Context* PhysicalTableMappingUDF::createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  // If one Context has been generated for each child, return NULL
  // to signal completion. This will also take care of 0 child case.
  // ---------------------------------------------------------------------
  if (pws->getCountOfChildContexts() == getArity())
    return NULL;

  childIndex = 0;

  // must be looking for a valid child
  if ((childIndex+1) > getArity())
    CMPASSERT(FALSE);
    
  Lng32 planNumber = 0;
  const ReqdPhysicalProperty* rppForMe = myContext->getReqdPhysicalProperty();
  Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
  float childNumPartsAllowedDeviation = 0.0;
  NABoolean numOfESPsForced = FALSE;

  RequirementGenerator rg(child(childIndex),rppForMe);
  TMUDFInputPartReq childPartReqType = getChildInfo(0)->getPartitionType();
  PartitioningRequirement* partReqForChild = NULL;

  TableMappingUDFChildInfo * childInfo = getChildInfo(childIndex);
  
  if (  childPartReqType == SPECIFIED_PARTITIONING)
  {
    // if some specified partitioning is to be required from the child
    // then the required partitioning columns should be mentioned
    CMPASSERT(NOT childInfo->getPartitionBy().isEmpty());
    /* if (   NOT isinBlockStmt() OR
            ( ( rppForMe->getCountOfPipelines() > 1 ) OR
              ( CmpCommon::getDefault(COMP_BOOL_36) == DF_OFF )
            ) */
     rg.addPartitioningKey(childInfo->getPartitionBy());
     rg.addArrangement(childInfo->getPartitionBy(),ESP_SOT);
  }
  else if(childPartReqType == REPLICATE_PARTITIONING)
  {
    // get the number of replicas
    // for right now just get what ever number of streams the parent requires
    Lng32 countOfPartitions = CURRSTMT_OPTDEFAULTS->getMaximumDegreeOfParallelism();
    if(rppForMe->getPartitioningRequirement() &&
	(countOfPartitions < rppForMe->getCountOfPartitions()))
      countOfPartitions = rppForMe->getCountOfPartitions();
  
    if(countOfPartitions > 1)
      partReqForChild = new (CmpCommon::statementHeap() )
        RequireReplicateViaBroadcast(countOfPartitions);
    else
      partReqForChild = new(CmpCommon::statementHeap())
        RequireExactlyOnePartition();

     rg.addPartRequirement(partReqForChild);
  }

  if (NOT childInfo->getOrderBy().isEmpty())
  {
     ValueIdList sortKey(getChildInfo(0)->getPartitionBy());
     for (Int32 i=0;i<(Int32)getChildInfo(0)->getOrderBy().entries();i++)
      sortKey.insert(getChildInfo(0)->getOrderBy()[i]);
     rg.addSortKey(sortKey, ESP_SOT);
  }


  if (okToAttemptESPParallelism(myContext,
                                pws,
                                childNumPartsRequirement,
                                childNumPartsAllowedDeviation,
                                numOfESPsForced))
  {
    if (NOT numOfESPsForced)
      rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                &childNumPartsAllowedDeviation);
    rg.addNumOfPartitions(childNumPartsRequirement,
                          childNumPartsAllowedDeviation);
  } // end if ok to try parallelism

  // ---------------------------------------------------------------------
  // Done adding all the requirements together, now see whether it worked
  // and give up if it is not possible to satisfy them
  // ---------------------------------------------------------------------
  if (NOT rg.checkFeasibility())
    return NULL;

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);

  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as those in
  // rppForChild. Reuse it, if found. Otherwise, create a new Context
  // that contains rppForChild as the required physical properties..
  // ---------------------------------------------------------------------
  Context* result = shareContext(
         childIndex,
         rg.produceRequirement(),
         myContext->getInputPhysicalProperty(),
         costLimit,
         myContext,
         myContext->getInputLogProp());


  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, planNumber, result);

  return result;
};

PhysicalProperty* PhysicalTableMappingUDF::synthPhysicalProperty(const Context* myContext,
                                                  const Lng32     planNumber)
{
  PartitioningFunction* myPartFunc = NULL;
  if (getArity() == 0)
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
    const ReqdPhysicalProperty* rppForMe = myContext->getReqdPhysicalProperty();
    PartitioningRequirement* partReq = rppForMe->getPartitioningRequirement();

      if ( partReq == NULL || partReq->isRequirementExactlyOne())
      {
      myPartFunc = new(CmpCommon::statementHeap())
                          SinglePartitionPartitioningFunction(myNodeMap);
      }
      else 
      {
          myPartFunc =  partReq->realize(myContext);
      }
  }
  else
  {
    // for now, simply propagate the physical property 
    const PhysicalProperty * const sppOfChild =
      myContext->getPhysicalPropertyOfSolutionForChild(0);
    myPartFunc = sppOfChild->getPartitioningFunction();
  }
  
    PhysicalProperty * sppForMe =
    new(CmpCommon::statementHeap()) PhysicalProperty(
         myPartFunc,
         EXECUTE_IN_MASTER_AND_ESP,
          SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;
  return sppForMe ;
};

double PhysicalTableMappingUDF::getEstimatedRunTimeMemoryUsage(ComTdb * tdb) {return 0;}

short PhysicalTableMappingUDF::generateShape(CollHeap * space, char * buf, NAString * shapeStr)
{
  CMPASSERT(0);
  return 0;
};

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
                                   SchemaName* schName,
                                   CollHeap *oHeap)
  : BuiltinTableValuedFunction(NULL,REL_HIVEMD_ACCESS,oHeap)
{
  if (mdt)
    mdType_ = *mdt;
  if (schName)
    schemaName_ = *schName;
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
    result = new (outHeap) HiveMDaccessFunc(NULL,NULL,outHeap);
  else
    result = (HiveMDaccessFunc *) derivedNode;

  result->mdType_ = mdType_;
  result->schemaName_ = schemaName_;
  

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
                                   const Lng32     planNumber)
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
                                   desc_struct *&colDescs,
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

   cmpSBD.buildColInfoArray(&colArray, 
                            colInfoArray, 
                            FALSE, 0, FALSE, NULL,
                            CmpCommon::statementHeap());

   colDescs = cmpSBD.convertVirtTableColumnInfoArrayToDescStructs(&tableName,
                                                                  colInfoArray,
                                                                  numCols) ;

  // calculate the record length 
  desc_struct *tempDescs = colDescs;
  for (ComUInt32 colNum = 0; colNum < numCols; colNum++)
  { 
    NAType *naType = getColumn(colNum).getColumnDataType();
    Int32 fsType = naType->getFSDatatype();

    if (tempDescs->body.columns_desc.null_flag)
      reclen += SQL_NULL_HDR_SIZE;
    if (DFS2REC::isSQLVarChar(fsType))
      reclen += SQL_VARCHAR_HDR_SIZE;

    reclen += tempDescs->body.columns_desc.length;
    tempDescs = tempDescs->header.next;
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
