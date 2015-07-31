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
#ifndef ITEMSAMPLE_H
#define ITEMSAMPLE_H

#include "RelSample.h"  


// For Balance Expression. 
//
class ItmBalance : public BuiltinFunction
{
public:
  ItmBalance(ItemExpr *predicate,
	     ItemExpr *sampleSize,
             ItemExpr *nextBalance,
             ItemExpr *skipSize,
             NABoolean absSize,
             NABoolean exact)
    : BuiltinFunction(ITM_BALANCE, 
                      CmpCommon::statementHeap(),
                      4, 
                      predicate, 
                      sampleSize, 
                      nextBalance, 
                      skipSize),
                      sampleType_(RelSample::ANY),
                      absolute_(absSize),
                      exact_(exact),
                      skipAbsolute_(TRUE)
  {
  };

  // virtual destructor
  virtual ~ItmBalance();

  // Accessors
  ItemExpr * getPredicate() const { return child(0); };

  ItemExpr * getSampleSize() const { return child(1); };

  ItemExpr * getNextBalance() const;

  ItemExpr * getSkipSize() const;

  ItemExpr * getClusterSize() const;

  void propagateSampleType(RelSample::SampleTypeEnum sampType);

  virtual void setSampleType(RelSample::SampleTypeEnum sampType)
  {
    sampleType_ = sampType;
  };

  void rearrangeChildren();

  virtual Int32 getArity() const;

  virtual HashValue topHash();

  virtual NABoolean duplicateMatch(const ItemExpr &other) const;

  virtual RelSample::SampleTypeEnum sampleType() const
  {
    return sampleType_;
  };

  virtual NABoolean isAbsolute() const { return absolute_; };

  virtual NABoolean isExact() const { return exact_; };

  virtual NABoolean isSkipAbsolute() const { return skipAbsolute_; };

  // a virtual function for type propagating the node
  //
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // method to to preCode generation
  //
  virtual ItemExpr *preCodeGen(Generator*);

  // method to do code generation
  //
  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const
  {
    return "ItmBalance";
  };

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
                              const GroupAttributes& newRelExprAnchorGA,
                              ValueIdSet& referencedInputs,
                              ValueIdSet& coveredSubExpr,
                              ValueIdSet& unCoveredExpr) const;

  CostScalar computeResultSize(CostScalar initialRowCount);

  Int32 checkErrors();

  double getSampleConstValue() const;
  double getSkipConstValue() const;
  double getClusterConstValue() const;

  virtual void setSkipAbs(NABoolean skipAbs) { skipAbsolute_ = skipAbs; };


private:

  RelSample::SampleTypeEnum sampleType_;

  NABoolean absolute_;
  
  NABoolean exact_;

  NABoolean skipAbsolute_;

}; // class ItmBalance

// A dummy expression to avoid column predicates and references pushed down
// below an operator 
//
class NotCovered : public BuiltinFunction
{
public:
  NotCovered(ItemExpr *column)
    : BuiltinFunction(ITM_NOTCOVERED, CmpCommon::statementHeap(), 1, column)
  {
  };

  // virtual destructor
  virtual ~NotCovered();

  ItemExpr *getColumnRef() { return child(0); };

  // a virtual function for type propagating the node
  //
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // method to do code generation
  //
  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const
  {
    return "NotCovered";
  };

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
                              const GroupAttributes& newRelExprAnchorGA,
                              ValueIdSet& referencedInputs,
                              ValueIdSet& coveredSubExpr,
                              ValueIdSet& unCoveredExpr) const;

  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) {return TRUE;}

  virtual NABoolean isOrderPreserving() const { return child(0)->isOrderPreserving(); }

}; // class NotCovered


// Function to perform random selection based on a selection probability
// specified (as a data member at construct time). It first generates 
// a random 32 bit unsigned integer. It is then compared to the selection 
// probability (scaled to a 32 bit integer) and an integer value of 1 is 
// returned if it is smaller, and 0 otherwise. For example, if the 
// selection probability is 0.3, the function returns 1 in 30% of cases
// and 0 in 70% of cases. The selection probability can be larger than 1, 
// in which case, the difference is added to the result. For example, if 
// it is 2.7, the result will be 3 in 70% of the cases and 2 in 30 % of 
// the cases. Currently, the seed is initialized automatically to a 
// random value the first time the function is called. This can be 
// modified as required in the future. 
//
class RandomSelection : public BuiltinFunction
{
public:
  RandomSelection(float selProb = 0.0) : 
      BuiltinFunction(ITM_RAND_SELECTION, 0)
  {
    if (selProb < 0)
      selProb = 0.0;
    selProbability_ = selProb;  
  };

  virtual ~RandomSelection();

  void setSelProbability(float selProb = 0.0) 
  {
    if (selProb < 0)
      selProb = 0.0;
    selProbability_ = selProb;
  };

  float getSelProbability() const
  {
    return selProbability_;
  };

  virtual const NAType * synthesizeType();

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const
  { return TRUE; };

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual short codeGen(Generator*);

private:

  float selProbability_;

}; // class RandomSelection

#endif /* ITEMSAMPLE_H */
