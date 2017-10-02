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
#ifndef RELSAMPLE_H
#define RELSAMPLE_H
/* -*-C++-*-
******************************************************************************
*
* File:         RelSample.h
* Description:  Class definitions for the classes RelSample
*               and PhysSample
* Created:      9/24/98
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "RelExpr.h"

///////////////////////////////////////////////////////////////////////////////
//
//             The RelSample class
//
//
///////////////////////////////////////////////////////////////////////////////

// Class RelSample

class RelSample : public RelExpr
{
public:
  enum SampleTypeEnum
  {
    RANDOM,
    PERIODIC,
    FIRSTN,
    CLUSTER,
    ANY
  };

  // the constructor
  RelSample (RelExpr *child,
                    SampleTypeEnum sampleType = ANY,
                    ItemExpr *balanceExpr = NULL,
                    ItemExpr *requiredOrder = NULL,
                    CollHeap *oHeap = CmpCommon::statementHeap());

  // the desctructor
  virtual ~RelSample ();

  // RelSample operator has only one child
  virtual Int32 getArity () const { return 1; };

  virtual const NAString getText() const {return "Sample";};

  ValueIdList mapSortKey(const ValueIdList &sortKey) const;

  // Method to return data members
  //
  // Accessor methods
  inline ValueIdList & requiredOrder() { return requiredOrder_; };
  inline const ValueIdList & requiredOrder() const { return requiredOrder_; }

  inline SampleTypeEnum sampleType () const { return sampleType_; };

  inline void sampleType (SampleTypeEnum type) {sampleType_ = type; };

  inline NABoolean sampleScanSucceeded () {return sampleScanSucceeded_; };

  inline void setSampleScanSucceeded (NABoolean b) {sampleScanSucceeded_ = b;};

  inline ItemExpr *&balanceExprTree() { return balanceExprTree_; };

  inline const ItemExpr *balanceExprTree() const { return balanceExprTree_; };

  inline ValueIdSet & balanceExpr() { return balanceExpr_; };

  inline const ValueIdSet & balanceExpr() const { return balanceExpr_; };

  inline ValueIdSet & sampledColumns() { return sampledColumns_; };

  inline const ValueIdSet & sampledColumns() const { return sampledColumns_; };

  virtual HashValue topHash();

  virtual NABoolean duplicateMatch(const RelExpr & other) const;

  virtual RelExpr *copyTopNode(RelExpr *derivedNode, CollHeap *outHeap);

  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;

  virtual void getPotentialOutputValues(ValueIdSet & outputValues) const;

  virtual void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                                   const ValueIdSet &newExternalInputs,
                                   ValueIdSet &predicatesOnParent,
				   const ValueIdSet * setOfValuesReqdByParent = NULL,
                                   Lng32 childIndex = (-MAX_REL_ARITY)
                                  );

  ItemExpr *removeBalanceExprTree();

  virtual void transformNode(NormWA &normWARef,
                             ExprGroupId &locationOfPointerToMe);

  virtual void rewriteNode(NormWA & normWARef);

  virtual RelExpr *normalizeNode(NormWA & normWARef);

  virtual RelExpr * semanticQueryOptimizeNode(NormWA & normWARef);

  virtual void pullUpPreds();

  virtual void recomputeOuterReferences();

  CostScalar computeResultSize(const CostScalar &childCardinality);

  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  virtual Context* createContextForAChild(Context* myContext,
                                          PlanWorkSpace* pws,
                                          Lng32& childIndex);

  virtual RelExpr *bindNode(BindWA *bindWA);

  NABoolean isSimpleRandomRelative() const;

  Float32 getSamplePercent() const;

  Lng32 getClusterSize() const;

private:
  // Return a pointer to the required order tree after
  // setting it to NULL.
  //
  ItemExpr *removeRequiredOrderTree();

  // An ItemExpr list of columns representing the reqired sort order
  // for this node.
  //
  ItemExpr *requiredOrderTree_;

  // The bound version of requiredOrderTree_.
  //
  ValueIdList requiredOrder_;

  // Sample type is one of RANDOM, PERIODIC, FIRSTN, CLUSTER
  SampleTypeEnum sampleType_;

  // Used by optimizer to avoid firing PhysicalSampleRule for CLUSTER
  // samples which have already succesfully fired the SampleScanRule.
  // If the SampleScanRule has not successfully fired, we want the
  // PhysicalSampleRule to fire so that an appropriate error message
  // can be produced in the generator.  This data member is initially
  // set to FALSE and is set to TRUE by the SampleScanRule.
  //
  NABoolean sampleScanSucceeded_;

  // The balance expression is a list of balance itemexprs
  //
  ItemExpr *balanceExprTree_;

  // The bound balance expression
  ValueIdSet balanceExpr_;

  // The output columns
  ValueIdSet sampledColumns_;

};

// Class PhysSample ----------------------------------------------------
// The PhysSample node replaces the logical RelSample node through the
// application of the PhysSampleRule. This transformation is
// designed to present a purely physical verison of this operator
// that is both a logical and physical node.
// -----------------------------------------------------------------------
class PhysSample : public RelSample
{
public:

  // The constructor
  //
  PhysSample(RelExpr *child = NULL,
                    RelSample::SampleTypeEnum type = ANY,
		    CollHeap *oHeap = CmpCommon::statementHeap())
    : RelSample(child, type, NULL, NULL, oHeap)
  {
  };

  // The destructor.
  //
  virtual ~PhysSample();

  // methods to do code generation of the physical node.
  //
  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  // generate CONTROL QUERY SHAPE fragment for this node.
  //
  virtual short generateShape(CollHeap * space, char * buf, NAString * shapeStr = NULL);

  // Copy a chain of derived nodes (Calls UnPackRows::copyTopNode).
  // Needs to copy all relevant fields (in this case no fields
  // need to be copied)
  // Used by the Cascades engine.
  //
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap *outHeap = NULL);

  // cost functions
  //
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32 planNumber,
                                                  PlanWorkSpace  *pws);
  virtual CostMethod* costMethod() const;

  // Redefine these virtual methods to declare this node as a
  // physical node.
  //
  virtual NABoolean isLogical() const {return FALSE;};
  virtual NABoolean isPhysical() const {return TRUE;};

  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);
}; // class PhysSample


#endif

