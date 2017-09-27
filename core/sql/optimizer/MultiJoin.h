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
#ifndef MULTIJOIN_H
#define MULTIJOIN_H
/* -*-C++-*-
 **************************************************************************
 *
 * File:         MultiJoin.h
 * Description:  MultiJoin Operator Methods
 * Created:      02/17/2003
 * Language:     C++
 *
 *
 *
 **************************************************************************
 */

//#include "RelExpr.h"
#include "Analyzer.h"
//#include "RelJoin.h"

// Classes Defined in this file
class MultiJoin;
class JBBCExprGroupMap;
class JBBCExprGroupEntry;
class MJJoinDirective;

// -----------------------------------------------------------------------
// JBBCExprGroupEntry :
// -----------------------------------------------------------------------

class JBBCExprGroupEntry : public NABasicObject
{
public:

  // Constuctor
  JBBCExprGroupEntry(CANodeId jbbcId,
                     const ExprGroupId & exprGroupId,
                     CollHeap *outHeap = CmpCommon::statementHeap()):
    jbbcId_(jbbcId),
    exprGroupId_(exprGroupId),
    heap_(outHeap)
  {}

  // Destructor
  ~JBBCExprGroupEntry()
  {}

  ExprGroupId & getExprGroupId()
  {
    return exprGroupId_;
  }

  const ExprGroupId & getExprGroupId() const
  {
    return exprGroupId_;
  }

  inline CANodeId getJBBCId() const
  {
    return jbbcId_;
  }

private:

  ExprGroupId                   exprGroupId_;
  CANodeId                      jbbcId_;
  CollHeap*                     heap_;

};

// -----------------------------------------------------------------------
// JBBCExprGroupMap :
// -----------------------------------------------------------------------

#pragma nowarn(1506)  // warning elimination
class JBBCExprGroupMap
{
public:

  // Constuctor
  JBBCExprGroupMap(CollHeap *outHeap = CmpCommon::statementHeap()):
    heap_(outHeap),
    array_(outHeap)
  {}

  // Destructor
  ~JBBCExprGroupMap()
  {}

  inline ExprGroupId & getExprGroupId(Lng32 index)
  {
    CMPASSERT(array_.used(index));
    return array_[index]->getExprGroupId();
  }

  const ExprGroupId & getExprGroupId(Lng32 index) const
  {
    CMPASSERT(array_.used(index));
    return array_[index]->getExprGroupId();
  }

  const ExprGroupId & getExprGroupIdOfJBBC(CANodeId jbbc) const;

  void insertAt(Lng32 index, JBBCExprGroupEntry* entry)
  {
    array_.insertAt(index, entry);
    jbbcs_.insert(entry->getJBBCId());
  }

  inline const CANodeIdSet & getJBBCs() const
  {
    return jbbcs_;
  }

  inline Lng32 entries() const
  {
    return jbbcs_.entries();
  }

private:

  ARRAY(JBBCExprGroupEntry*)       array_;
  CANodeIdSet                      jbbcs_;
  CollHeap*                        heap_;

};
#pragma warn(1506)  // warning elimination

// -----------------------------------------------------------------------
// member functions for class LSRConfidence
// -----------------------------------------------------------------------

// range of confidence values -1 - 10
// -1 = rule not applied
//  0 = rule failed
// 10 = rule applied, very confident do not try enumeration
class LSRConfidence : public NABasicObject
{
  public:
    LSRConfidence(CollHeap *outHeap = CmpCommon::statementHeap()):
                  starBDRuleConfidence_(-1),
                  starJoinRuleConfidence_(-1),
                  primeTableRuleConfidence_(-1),
                  heap_(outHeap)
    {};

    void setStarBDRuleConfidence(Int32 confidence)
    {
      if(confidence < 0)
        confidence = 0;

      if(confidence > 10)
        confidence = 10;

      starBDRuleConfidence_ = confidence;
    }
    void setStarJoinRuleConfidence(Int32 confidence)
    {
      if(confidence < 0)
        confidence = 0;

      if(confidence > 10)
        confidence = 10;

      starJoinRuleConfidence_ = confidence;
    }

    void setPrimeTableRuleConfidence(Int32 confidence)
    {
      if(confidence < 0)
        confidence = 0;
      if(confidence > 10)
        confidence = 10;
      primeTableRuleConfidence_ = confidence;
    }

    Int32 getStarBDRuleConfidence() { return starBDRuleConfidence_;}
    Int32 getStarJoinRuleConfidence() { return starJoinRuleConfidence_;}
    Int32 getPrimeTableRuleConfidence() { return primeTableRuleConfidence_;}

  private:
    Int32 starBDRuleConfidence_;
    Int32 starJoinRuleConfidence_;
    Int32 primeTableRuleConfidence_;
    CollHeap*    heap_;
};

// -----------------------------------------------------------------------
// member functions for class MultiJoin
// -----------------------------------------------------------------------
class MultiJoin : public RelExpr
{

public:

  // constructor
  MultiJoin(const JBBSubset & jbbSubset,
                   //OperatorTypeEnum otype = REL_MULTI_JOIN,
                   CollHeap *oHeap = CmpCommon::statementHeap());

  // copy ctor
  MultiJoin (const MultiJoin &) ; // not written

  // virtual destructor
  virtual ~MultiJoin() {};

  // the number of MultiJoin children
  virtual Int32 getArity() const
  {
    return childrenMap_.entries();
	// xxx jbbSubset_.entries();
  }

  NABoolean isSymmetricMultiJoin() const;

  virtual void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                                 const ValueIdSet & newExternalInputs,
                                 ValueIdSet & predicatesOnParent,
				 const ValueIdSet * setOfValuesReqdByParent = NULL,
                                 Lng32 childIndex =(-MAX_REL_ARITY));

  virtual void getPotentialOutputValues(ValueIdSet & outputValues) const;

  const NAString getText() const;

  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;

  virtual HashValue topHash();

  virtual NABoolean duplicateMatch(const RelExpr & other) const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode, CollHeap* outHeap);

  Join* splitSubset(const JBBSubset & leftSet,
                    const JBBSubset & rightSet,
                    NABoolean reUseMJ = FALSE) const;

  Join* splitByTables(const CANodeIdSet & leftTableSet,
                      const CANodeIdSet & rightTableSet,
                      NABoolean reUseMJ = FALSE) const;

  RelExpr* generateSubsetExpr(const JBBSubset & subset, NABoolean reUseMJ = FALSE) const;

  MultiJoin* createSubsetMultiJoin(const JBBSubset & subset, NABoolean reUseMJ = FALSE) const;

  // do some analysis on the initial plan
  // this is called at the end of the analysis phase
  virtual void analyzeInitialPlan();

  virtual void computeMyRequiredResources(RequiredResources & reqResources,
                                      EstLogPropSharedPtr & inLP);

  // use the input JBBCExprGroupMap to set this MultiJoin childrenMap_
  void setChildren(const JBBCExprGroupMap & map);

  // use origExprs from NodeAnalysis to set this MultiJoin childrenMap_
  void setChildrenFromOrigExprs(QueryAnalysis * qa);

  // To access MultiJoin children
  virtual ExprGroupId & operator[] (Lng32 index)
  {
    return childrenMap_.getExprGroupId(index);
  }

  virtual const ExprGroupId & operator[] (Lng32 index) const
  {
    return childrenMap_.getExprGroupId(index);
  }

  // This method returns the child as a RelExpr. If the child is
  // a group it return a cut-op for that group.
  RelExpr* getJBBCRelExpr(CANodeId jbbc) const;

  // This method returns the child as a cut-op.
  // If the child is a group it return a cut-op for that group.
  // If the child is a RelExpr it return a cut-op that shares its GA
  CutOp * getJBBCCutOpExpr(CANodeId jbbc) const;

  inline const JBBSubset & getJBBSubset() const
  {
    return jbbSubset_;
  }

  // code no longer used. Used to be called from LargeScopeRules.cpp method
  // MJStarJoinIRule::computeCostForFactTable but this code is no longer used
  ExprGroupId getChildFromJBBCId(CANodeId jbbc) const
  {
    return childrenMap_.getExprGroupIdOfJBBC(jbbc);
  }

  Join * getPreferredJoin();

  virtual void recomputeOuterReferences();

  virtual EstLogPropSharedPtr setJBBInput(EstLogPropSharedPtr & inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  virtual RelExpr* expandMultiJoinSubtree();

  virtual void primeGroupAnalysis();

  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  void synthLogPropWithMJReuse(NormWA * normWAPtr = NULL);

  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputLP);

  // fix join order = fix the join order and don't allow JBBCs to move
  Join* leftLinearize(NABoolean fixJoinOrder=FALSE,
                      NABoolean createPriviledgedJoins=FALSE) const;

  Join* createLeftLinearJoinTree(const NAList<CANodeIdSet> * const leftDeepJoinSequence,
                                 NAList<MJJoinDirective *> * joinDirectives) const;

  CostScalar getChildrenDataFlow() const;

  virtual RelExpr * generateLogicalExpr (CANodeIdSet &, CANodeIdSet &);

  inline RuleSubset &scheduledLSRs()
  {
    return scheduledLSRs_;
  }

  LSRConfidence * getLSRConfidence() { return lsrC_;}

private:

  JBBSubset               jbbSubset_;
  JBBCExprGroupMap        childrenMap_;

  LSRConfidence *         lsrC_;
  RuleSubset              scheduledLSRs_;   // follow up rules scheduled
                                            // By other rules.
};

class MJJoinDirective: public NABasicObject
{
  public:

    enum JoinTypes
    {
      NESTED_JOIN,
      MERGE_JOIN,
      HASH_JOIN
    };

    MJJoinDirective(CollHeap *outHeap = CmpCommon::statementHeap());

    inline void setSkipNestedJoin(){ skipNestedJoin_ = TRUE; };
    inline void setSkipMergeJoin(){ skipMergeJoin_ = TRUE; };
    inline void setSkipHashJoin(){ skipHashJoin_ = TRUE;};
    inline void setJoinSource(Join::JoinSourceType jSource) { joinSource_ = jSource; };
    inline void setSkipJoinLeftShift() { skipJoinLeftShift_ = TRUE; };
    inline void setSkipJoinCommutativity() { skipJoinCommutativity_ = TRUE; };
    inline void setJoinFromPTRule() { joinFromPTRule_ = TRUE; };

    inline void scheduleLSROnLeftChild(NAUnsigned lsrRuleNum){ leftScheduledLSRs_ += lsrRuleNum;};
    inline void scheduleLSROnRightChild(NAUnsigned lsrRuleNum){ rightScheduledLSRs_ += lsrRuleNum;};

    void setupJoin(Join * join);


  private:

    NABoolean skipNestedJoin_;
    NABoolean skipMergeJoin_;
    NABoolean skipHashJoin_;

    NABoolean skipJoinLeftShift_;
    NABoolean skipJoinCommutativity_;

    Join::JoinSourceType joinSource_;
    RuleSubset rightScheduledLSRs_;
    RuleSubset leftScheduledLSRs_;

    NABoolean joinFromPTRule_;

    // the heap
    CollHeap* heap_;

};

// -----------------------------------------------------------------------
// Rules that apply on a MultiJoin instance can save information in this
// work area
// -----------------------------------------------------------------------
/*
class MultiJoinWA : public NABasicObject
{

public:

  // constructor
  MultiJoinWA()
  { }

  static NABoolean Test1(RelExpr* expr);

private:
  MultiJoin*                       mjoin_;
  CASortedList*                    byLocalKeyPrefixPredsCard_;
  CASortedList*                    byLocalKeyPrefixPredsData_;
  CASortedList*                    byLocalPredsCard_;
  CASortedList*                    byLocalPredsData_;
  CASortedList*                    byBaseCard_;
  CASortedList*                    byBaseData_;

};
*/
// -----------------------------------------------------------------------
// Tester for class MultiJoin
// -----------------------------------------------------------------------
class MultiJoinTester : public NABasicObject
{

public:

  // constructor
  inline MultiJoinTester()
  { }

  static NABoolean Test1(RelExpr* originalNonMultiJoinTree, RelExpr* treeConvertedToMultiJoin);

private:
  //MultiJoin*                       mjoin_;
};

#endif /* MULTIJOIN_H */
