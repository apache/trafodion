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

#include "GroupAttr.h"
#include "AllItemExpr.h"
#include "ItemSample.h"
#include "Sqlcomp.h"

// -----------------------------------------------------------------------
// member functions for Balance
// -----------------------------------------------------------------------
//
ItmBalance::~ItmBalance() {};

HashValue ItmBalance::topHash()
{
  HashValue result = ItemExpr::topHash();

  result ^= (const ValueId &)child(0);
  result ^= (const ValueId &)child(1);

  return result;

}


ItemExpr * ItmBalance::getNextBalance() const
{
  if (sampleType() == RelSample::PERIODIC ||
      sampleType() == RelSample::CLUSTER)
    return child(3);
  else
    return child(2);
}


ItemExpr * ItmBalance::getSkipSize() const
{
  if (sampleType() == RelSample::PERIODIC)
    return child(2);
  else
    return NULL;
}

ItemExpr * ItmBalance::getClusterSize() const
{
  if (sampleType() == RelSample::CLUSTER)
    return child(2);
  else
    return NULL;
}


void ItmBalance::propagateSampleType(RelSample::SampleTypeEnum sampType)
{
  // Must be called BEFORE rearrangeChildren() is called


  setSampleType(sampType);
  ItmBalance * nextBal = (ItmBalance *)(ItemExpr *)child(2);

  while (nextBal != NULL)
  {
    nextBal->setSampleType(sampType);
    nextBal = (ItmBalance *)(ItemExpr *)(nextBal->child(2));
  }
}


void ItmBalance::rearrangeChildren()
{
  // This is called from the parser before any processing on the
  // balance node. It basically reorders the skipSize/clusterSize and
  // nextBalance children for the entire tree in addition to setting
  // the skipSize for all the balance nodes in the tree if the
  // sampling type is periodic.  NOTE: Access the specific children
  // (e.g., skipSize, etc) only through the accessor functions after
  // this is called.

  if (sampleType() != RelSample::PERIODIC &&
      sampleType() != RelSample::CLUSTER) 
    // Nothing to rearrange
    return;
  
  ItemExpr * skip = (ItemExpr *)child(3);
  ItmBalance * nextBal = (ItmBalance *)(ItemExpr *)child(2);

  // Rearrange for this node 
  setChild(2, skip);
  setChild(3, nextBal);

  // Propagate down the tree
  while (nextBal != NULL)
  {
    ItmBalance * temp = (ItmBalance *)(ItemExpr *)(nextBal->child(2));
    nextBal->setChild(2, skip);
    nextBal->setChild(3, temp);
    nextBal = temp;
  }
}


Int32 ItmBalance::getArity() const
{
  Int32 arity = 2;

  if (sampleType() == RelSample::PERIODIC ||
      sampleType() == RelSample::CLUSTER)
    arity++;
  if (getNextBalance() != NULL)
    arity++;

  return arity;
}


NABoolean ItmBalance::duplicateMatch(const ItemExpr &other) const
{
  if (NOT ItemExpr::duplicateMatch(other))
    return FALSE;

  ItmBalance &o = (ItmBalance &)other;
  if (child(0) != o.child(0))
    return FALSE;

  if (child(1) != o.child(1))
    return FALSE;
  
  if (sampleType() != o.sampleType())
    return FALSE;

  return TRUE;
}

ItemExpr * ItmBalance::copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap)
{
  ItmBalance *result;

  if (derivedNode == NULL)
    result = new (outHeap) ItmBalance(child(0), 
                                      child(1), 
                                      child(2), 
                                      child(3), 
                                      isAbsolute(), 
                                      isExact());
  else
  {
    result = (ItmBalance *)derivedNode;
    result->absolute_ = isAbsolute();
    result->exact_ = isExact();
  }

  result->sampleType_ = sampleType();
  result->skipAbsolute_ = isSkipAbsolute();

  return ItemExpr::copyTopNode(result, outHeap);
}

NABoolean ItmBalance::isCovered(const ValueIdSet& newExternalInputs,
                                const GroupAttributes& newRelExprAnchorGA,
                                ValueIdSet& referencedInputs,
                                ValueIdSet& coveredSubExpr,
                                ValueIdSet& unCoveredExpr) const
{
  ValueIdSet localSubExpr;
  for(Lng32 i = 0; i < (Lng32)getArity(); i++)
  {
    if(newRelExprAnchorGA.covers(child(i)->getValueId(),
                                 newExternalInputs,
                                 referencedInputs,
                                 &localSubExpr))
    {
      coveredSubExpr += child(i)->getValueId();
    }
    coveredSubExpr += localSubExpr;
  }

  // Return FALSE unconditionally. Only the sample operator can perform
  // balancing.
  return FALSE;
}


const NAType *ItmBalance::synthesizeType()
{
  // returns a signed, non-null integer
  return new HEAP SQLInt(HEAP, TRUE,FALSE);
}

double
ItmBalance::getSampleConstValue() const
{

  NABoolean negate = FALSE;
  ConstValue *sizeExpr = (getSampleSize()
                          ? getSampleSize()->castToConstValue(negate)
                          : NULL);
  CMPASSERT(negate == FALSE);

  double size = 0.0;
  Lng32 scale;

  if(sizeExpr && sizeExpr->canGetExactNumericValue()) {
    size = (double)sizeExpr->getExactNumericValue(scale);
    while (scale--) size /= 10;
  }

  return size;
}

double
ItmBalance::getSkipConstValue() const
{

  NABoolean negate = FALSE;
  ConstValue *skipExpr = (getSkipSize()
                          ? getSkipSize()->castToConstValue(negate)
                          : NULL);
  CMPASSERT(negate == FALSE);

  double skip = 0.0;
  Lng32 scale;

  if(skipExpr && skipExpr->canGetExactNumericValue()) {
    skip = (double)skipExpr->getExactNumericValue(scale);
    while (scale--) skip /= 10;
  }

  return skip;
}

double
ItmBalance::getClusterConstValue() const
{

  NABoolean negate = FALSE;
  ConstValue *clusterExpr = (getClusterSize()
                             ? getClusterSize()->castToConstValue(negate)
                             : NULL);
  CMPASSERT(negate == FALSE);

  double cluster = 0.0;
  Lng32 scale;

  if(clusterExpr && clusterExpr->canGetExactNumericValue()) {
    cluster = (double)clusterExpr->getExactNumericValue(scale);
    while (scale--) cluster /= 10;
  }

  return cluster;
}


CostScalar ItmBalance::computeResultSize(CostScalar initialRowCount)
{
  CostScalar result = 0;

  if(getNextBalance()) {
    
    CMPASSERT(getNextBalance()->getOperatorType() == ITM_BALANCE);

    result = 
      ((ItmBalance *)getNextBalance())->computeResultSize(initialRowCount);
  }
  
  CostScalar size = getSampleConstValue();
  CostScalar skip = getSkipConstValue();
    
  if(!isAbsolute())
    size = (size / 100.0) * initialRowCount;

  if(!isSkipAbsolute())
    skip = (skip / 100.0) * initialRowCount;

  switch(sampleType()) {
  case RelSample::RANDOM:
  case RelSample::FIRSTN:
  case RelSample::CLUSTER:
      result += size;
    break;
  case RelSample::PERIODIC:
    result += ((size/skip) * initialRowCount);
    break;
  }

  if(result.isZero())
    result.minCsOne();
  
  return result;
}

Int32
ItmBalance::checkErrors()
{
  RelSample::SampleTypeEnum sampType = sampleType();
  NABoolean absolute = isAbsolute();
  NABoolean absoluteSkip = isSkipAbsolute();
  Int32 error = 0;

  ItmBalance *balNode = this;

  while(balNode) {
    CMPASSERT(balNode->getOperatorType() == ITM_BALANCE);

    CMPASSERT(sampType == balNode->sampleType());

    if(absolute != balNode->isAbsolute()) {
      *CmpCommon::diags() << DgSqlCode(-4112);
      error = -1;
    }

    if(absolute) {
      CMPASSERT(getSampleSize());

      NABoolean negate = FALSE;
      ConstValue *sizeExpr = getSampleSize()->castToConstValue(negate);

      CMPASSERT(negate == FALSE);
      CMPASSERT(sizeExpr);

      Lng32 scale;
      sizeExpr->getExactNumericValue(scale);

      if(scale > 0) {
        *CmpCommon::diags() << DgSqlCode(-4114);
        error = -1;
      }
    }

    if(getSkipSize() && absoluteSkip) {

      NABoolean negate = FALSE;
      ConstValue *skipExpr = getSkipSize()->castToConstValue(negate);

      CMPASSERT(negate == FALSE);
      CMPASSERT(skipExpr);

      Lng32 scale;
      skipExpr->getExactNumericValue(scale);

      if(scale > 0) {
        *CmpCommon::diags() << DgSqlCode(-4114);
        error = -1;
      }
    }

    if(sampType == RelSample::PERIODIC) {
      double size = getSampleConstValue();
      double skip = getSkipConstValue();

      if(size > skip) {
        *CmpCommon::diags() << DgSqlCode(-4115);
        error = -1;
      }
    }

    if(sampType == RelSample::FIRSTN && !balNode->isAbsolute()) {
      *CmpCommon::diags() << DgSqlCode(-4113)
                          << DgString0("First")
                          << DgString1("Absolute");
      error = -1;
    }

    if(sampType == RelSample::PERIODIC && 
       (!balNode->isAbsolute() || !balNode->isSkipAbsolute())) {
      *CmpCommon::diags() << DgSqlCode(-4113)
                          << DgString0("Periodic")
                          << DgString1("Absolute");
      error = -1;
    }

    if(sampType == RelSample::RANDOM && balNode->isAbsolute()) {
      *CmpCommon::diags() << DgSqlCode(-4113)
                          << DgString0("Random")
                          << DgString1("Relative");
      error = -1;
    }

    if(sampType == RelSample::CLUSTER && balNode->isAbsolute()) {
      *CmpCommon::diags() << DgSqlCode(-4113)
                          << DgString0("Cluster")
                          << DgString1("Relative");
      error = -1;
    }

    // CLUSTER sampling does not support oversampling.
    //
    if(sampType == RelSample::CLUSTER) {
      double size = getSampleConstValue();
      if (size >= 100) {
        *CmpCommon::diags() << DgSqlCode(-4113)
                            << DgString0("Cluster")
                            << DgString1("< 100%");
        error = -1;
      }
    }

    balNode = (ItmBalance *)balNode->getNextBalance();
  }

  return error;
}


// -----------------------------------------------------------------------
// member functions for NotCovered
// -----------------------------------------------------------------------
//
NotCovered::~NotCovered() {};

NABoolean NotCovered::isCovered(const ValueIdSet& newExternalInputs,
                                const GroupAttributes& newRelExprAnchorGA,
                                ValueIdSet& referencedInputs,
                                ValueIdSet& coveredSubExpr,
                                ValueIdSet& unCoveredExpr) const
{
  ValueIdSet localSubExpr;
  for(Lng32 i = 0; i < (Lng32)getArity(); i++)
  {
    if(newRelExprAnchorGA.covers(child(i)->getValueId(),
                                 newExternalInputs,
                                 referencedInputs,
                                 &localSubExpr))
    {
      coveredSubExpr += child(i)->getValueId();
    }
    coveredSubExpr += localSubExpr;
  }
  
  // ---------------------------------------------------------------------
  // The NotCovered function is coerced to fail the coverage test even
  // when its operands are isCovered(). This is because only the
  // operator owning this expression can evaluate the function. The
  // function is associated with a NotCovered node at the very
  // beginning and we don't allow it to be pushed down even if the
  // function's operands are covered at the node's child.
  // ---------------------------------------------------------------------
  return FALSE;
}
//++MV 02/27/01
void NotCovered::getLeafValuesForCoverTest(ValueIdSet& leafValues, 
                                           const GroupAttributes& coveringGA,
                                           const ValueIdSet & newExternalInputs) const
{
  // NotCovered is considered a leaf operator for cover test.
  leafValues += getValueId();
}
//--MV 02/27/01

const NAType *NotCovered::synthesizeType()
{
  // The type of this node is the same as its child
  //
  const NAType *childType = &getColumnRef()->getValueId().getType();
  return childType->newCopy(HEAP);
}

ItemExpr *
NotCovered::copyTopNode(ItemExpr *derivedNode, 
                        CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) NotCovered(NULL);
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // NotCovered::copyTopNode()

// -----------------------------------------------------------------------
// Class RandomSelection
// -----------------------------------------------------------------------
//
// -----------------------------------------------------------------------
// member functions for class RandomNum
// -----------------------------------------------------------------------

RandomSelection::~RandomSelection() {}

ItemExpr * RandomSelection::copyTopNode(ItemExpr *derivedNode, 
                                        CollHeap* outHeap)
{
  ItemExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) RandomSelection(getSelProbability());
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);

} // RandomSelection::copyTopNode()

const NAType *RandomSelection::synthesizeType()
{
  // returns an int, unsigned and not null
  return new HEAP SQLInt(HEAP, FALSE,FALSE);
}
