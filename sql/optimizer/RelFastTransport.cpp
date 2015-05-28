/**********************************************************************
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
* File:         RelFastTransport.cpp
* Description:  RelExprs related to support FastTransport (Load and Extract)
* Created:      9/29/12
* Language:     C++
*
*************************************************************************
*/

#include "CostMethod.h"
#include "AllRelExpr.h"
#include "Globals.h"

// -----------------------------------------------------------------------
// methods for class RelFastExtract
// -----------------------------------------------------------------------

//! FastExtract::FastExtract Copy Constructor
FastExtract::FastExtract(const FastExtract & other)
: RelExpr(other)
{
  targetType_ = other.targetType_;
  targetName_ = other.targetName_;
  hdfsHostName_ = other.hdfsHostName_;
  hdfsPort_ = other.hdfsPort_;
  isHiveInsert_ = other.isHiveInsert_;
  hiveTableName_ = other.hiveTableName_;
  delimiter_ = other.delimiter_;
  isAppend_ = other.isAppend_;
  includeHeader_ = other.includeHeader_;
  header_ = other.header_;
  cType_ = other.cType_,
  nullString_ = other.nullString_,
  recordSeparator_ = other.recordSeparator_;
  selectList_ = other.selectList_;
  isSequenceFile_ = other.isSequenceFile_;
  overwriteHiveTable_ = other.overwriteHiveTable_;
}

//! FastExtract::~FastExtract Destructor
FastExtract::~FastExtract()
{

}

//! FastExtract::copyTopNode method
RelExpr * FastExtract::copyTopNode(RelExpr *derivedNode,
                                CollHeap* outHeap)

{
   FastExtract *result;

   if (derivedNode == NULL)
   {
     result = new (outHeap) FastExtract(NULL, outHeap);
   }
   else
     result = (FastExtract *) derivedNode;

  result->targetType_ = targetType_;
  result->targetName_ = targetName_;
  result->hdfsHostName_ = hdfsHostName_;
  result->hdfsPort_ = hdfsPort_;
  result->isHiveInsert_= isHiveInsert_;
  result->hiveTableName_ = hiveTableName_;
  result->delimiter_ = delimiter_;
  result->isAppend_ = isAppend_;
  result->includeHeader_ = includeHeader_;
  result->header_ = header_;
  result->cType_ = cType_;
  result->nullString_ = nullString_;
  result->recordSeparator_ = recordSeparator_ ;
  result->selectList_ = selectList_;
  result->isSequenceFile_ = isSequenceFile_;

  return RelExpr::copyTopNode(result, outHeap);
}

//! FastExtract::getText method
const NAString FastExtract::getText() const
{

  if (isHiveInsert())
  {
    NAString op(CmpCommon::statementHeap());
    op = "hive_insert";
    NAString tname(hiveTableName_,CmpCommon::statementHeap());
    return op + " " + tname;
  }
  else
	return "UNLOAD";
}

void FastExtract::addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const
{ 
  if (NOT selectList_.isEmpty())
  {
    xlist.insert(selectList_.rebuildExprTree());
    llist.insert("select_list");
  }

  RelExpr::addLocalExpr(xlist,llist);
};

void FastExtract::transformNode(NormWA & normWARef,
    ExprGroupId & locationOfPointerToMe) 
{
  RelExpr::transformNode(normWARef, locationOfPointerToMe);
};
void FastExtract::rewriteNode(NormWA & normWARef)
{
  selectList_.normalizeNode(normWARef);
  // rewrite group attributes and selection pred
  RelExpr::rewriteNode(normWARef);
} ;
RelExpr * FastExtract::normalizeNode(NormWA & normWARef)
{
  return RelExpr::normalizeNode(normWARef);
};
 
void FastExtract::getPotentialOutputValues(ValueIdSet & vs) const
{
  vs.clear();
};

void FastExtract::pullUpPreds()
{
   // A FastExtract never pulls up predicates from its children.
   for (Int32 i = 0; i < getArity(); i++) 
    child(i)->recomputeOuterReferences();
};


void FastExtract::pushdownCoveredExpr(
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

    exprOnParent.insertList(getSelectList());

    RelExpr::pushdownCoveredExpr(outputExprOnOperator,
                                  newExternalInputs,
                                  predOnOperator,
                                  &exprOnParent,
                                  childId);
};

void FastExtract::synthEstLogProp(const EstLogPropSharedPtr& inputLP)
{
  // get child histograms
  EstLogPropSharedPtr childrenInputEstLogProp;
  CostScalar inputFromParentCard = inputLP->getResultCardinality();

  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(*inputLP));
  childrenInputEstLogProp = child(0).outputLogProp(inputLP);
  CostScalar extractCard = childrenInputEstLogProp->getResultCardinality();
  myEstProps->setResultCardinality(extractCard);
  // attach histograms to group attributes
  getGroupAttr()->addInputOutputLogProp (inputLP, myEstProps);

};

short FastExtract::setOptions(NAList<UnloadOption*> *
                              fastExtractOptionList,
                              ComDiagsArea * da)
{
  if (!fastExtractOptionList)
    return 0;

  for (CollIndex i = 0; i < fastExtractOptionList->entries(); i++)
  {
    UnloadOption::UnloadOption * feo = (*fastExtractOptionList)[i];
    switch (feo->option_)
    {
      case UnloadOption::DELIMITER_:
      {
        if (delimiter_.length() == 0)
          delimiter_ = feo->stringVal_;
        else
        {
          *da << DgSqlCode(-4376) << DgString0("DELIMITER");
          return 1;
        }
      }
      break;
      case UnloadOption::NULL_STRING_:
      {
        if (nullString_.length() == 0)
          nullString_ = feo->stringVal_;
        else
        {
          *da << DgSqlCode(-4376) << DgString0("NULL_STRING");
          return 1;
        }
      }
      break;
      case UnloadOption::RECORD_SEP_:
      {
        if (recordSeparator_.length() == 0)
          recordSeparator_ = feo->stringVal_;
        else
        {
          *da << DgSqlCode(-4376) << DgString0("RECORD_SEPARATOR");
          return 1;
        }
      }
      break;
      case UnloadOption::APPEND_:
      {
        if (!isAppend_)
          isAppend_ = TRUE;
        else
        {
          *da << DgSqlCode(-4376) << DgString0("APPEND");
          return 1;
        }
      }
      break;
      case UnloadOption::HEADER_:
      {
        if (includeHeader_)
          includeHeader_ = FALSE;
        else
        {
          *da << DgSqlCode(-4376) << DgString0("HEADER");
          return 1;
        }
      }
      break;
      case UnloadOption::COMPRESSION_:
      {
        if (cType_ == NONE)
          cType_ = (CompressionType) feo->numericVal_;
        else
        {
          *da << DgSqlCode(-4376) << DgString0("COMPRESSION");
          return 1;
        }
      }
      break;
      default:
        return 1;
    }
  }
  return 0;

};




  ///////////////////////////////////////////////////////////

  // ---------------------------------------------------------------------
  // comparison, hash, and copy methods
  // ---------------------------------------------------------------------

HashValue FastExtract::topHash()
{
  HashValue result = RelExpr::topHash();
  result ^= getSelectList() ;
  return result;
};

NABoolean FastExtract::duplicateMatch(const RelExpr & other) const
{
  if (!RelExpr::duplicateMatch(other))
    return FALSE;

  FastExtract &o = (FastExtract &) other;

  if (NOT (getSelectList() == o.getSelectList()))
      return FALSE;

  return TRUE;
};

// -----------------------------------------------------------------------
// methods for class PhysicalFastExtract
// -----------------------------------------------------------------------


PhysicalProperty* PhysicalFastExtract::synthPhysicalProperty(const Context* myContext,
                                                             const Lng32     planNumber,
                                                             PlanWorkSpace  *pws)
{
  PartitioningFunction* myPartFunc = NULL;
  const IndexDesc* myIndexDesc = NULL;

    // simply propagate the physical property
    const PhysicalProperty * const sppOfChild =
      myContext->getPhysicalPropertyOfSolutionForChild(0);
    myPartFunc = sppOfChild->getPartitioningFunction();
    myIndexDesc = sppOfChild->getIndexDesc();
  
    PhysicalProperty * sppForMe =
    new(CmpCommon::statementHeap()) PhysicalProperty(
         myPartFunc,
         EXECUTE_IN_MASTER_AND_ESP,
         SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;
  sppForMe->setIndexDesc(myIndexDesc);

  return sppForMe ;
};

double PhysicalFastExtract::getEstimatedRunTimeMemoryUsage(ComTdb * tdb) 
{

// The executor attempts to get buffers, each of size 1 MB. This memory
// is not from the SQLMXBufferSpace though, but is got directly from TSE
// through EXECUTOR_DP2_ADD_MEMORY. We may not get 1MB of memory for each 
// buffer at runtime. This value is interpreted as a potential/likely
// maximum. totalMemory is per TSE now.
  double totalMemory = ((ActiveSchemaDB()->getDefaults()).
                        getAsULong(FAST_EXTRACT_IO_BUFFERS))*1024*1024;

  const PhysicalProperty* const phyProp = getPhysicalProperty();
  if (phyProp != NULL)
  {
    PartitioningFunction * partFunc = phyProp -> getPartitioningFunction() ;
    // totalMemory is for all TSEs at this point of time.
    totalMemory *= partFunc->getCountOfPartitions();
  }

  return totalMemory;

}

// -----------------------------------------------------------------------
// PhysicalFastExtract::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod* PhysicalFastExtract::costMethod() const
{
  static THREAD_P CostMethodFastExtract *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap())  CostMethodFastExtract();
  return m;

}


