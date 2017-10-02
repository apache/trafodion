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
#ifndef ITEMSUBQ_H
#define ITEMSUBQ_H
/* -*-C++-*-
******************************************************************************
*
* File:         ItemSubq.h
* Description:  Subquery item expressions
*
* Created:      11/04/94
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "ItemExpr.h"
#include "RETDesc.h"
#include "RelExpr.h"
#include "RelJoin.h"
#include "RelGrby.h"


// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class Subquery;
class RowSubquery;
class Exists;
class InSubquery;
class QuantifiedComp;

// Forward references;
class GenericUpdate;

// ***********************************************************************
// A subquery 
// ***********************************************************************
class Subquery : public ItemExpr
{
public:

  Subquery(OperatorTypeEnum otype,
                  RelExpr * tableExpr, ItemExpr * scalarExpr = NULL)
         : ItemExpr(otype,scalarExpr),
           avoidHalloweenR2_(NULL),
           tableExpr_(tableExpr)
  {}

  virtual ~Subquery() {}

  // get the degree of this node
  virtual Int32 getArity() const;

  Int32 getDegree() const			{ return getRETDesc()->getDegree(); }

  const RETDesc *getRETDesc() const
  {
    const RETDesc *rd = getSubquery()->getRETDesc();
    CMPASSERT(rd);
    CMPASSERT(rd->getSystemColumnList()->entries() == 0);
    return rd;
  }

  // Classification of the subquery
  virtual NABoolean isASubquery() const;

  NABoolean isARowSubquery() const 
                      { return (getOperatorType() == ITM_ROW_SUBQUERY); }; 
  NABoolean isAnExistsSubquery() const 
          { return (getOperatorType() == ITM_EXISTS ||
				      getOperatorType() == ITM_NOT_EXISTS); }; 
  NABoolean isNotExists() const { return (getOperatorType() == ITM_NOT_EXISTS); }

  NABoolean isAnAnySubquery() const;
  NABoolean isAnAllSubquery() const;

  NABoolean containsSubquery();
      
  // The subquery operator can be a unary or a binary operator.
  // The following method encapsulates access to the subquery tree.
  RelExpr * getSubquery() const { return tableExpr_; }

  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType *synthesizeType();
  
  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe, 
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInput);
  
  // Method to transform the subquery ItemExpr into a RelExpr tree
  virtual void transformToRelExpr(NormWA &normWARef,
			 ExprValueId & locationOfPointerToMe, 
                         ExprGroupId & introduceSemiJoinHere,
			 const ValueIdSet & externalInputs);

    // Method to check to see if this subquery is a candidate for unnesting
    // and if so set appropriate flags.
  void evaluateCandidateForUnnesting(NormWA & normWARef, 
                                               Join *newJoin,
                                               GroupByAgg *newGrby);

  // Get the operator type for performing a complementary comparison.
  OperatorTypeEnum getComplementOfOperatorType();
  
  // A method for inverting (finding the inverse of) the operators
  // in a subtree that is rooted in a NOT.
  virtual ItemExpr * transformSubtreeOfNot(NormWA & normWARef,
                                           OperatorTypeEnum falseOrNot);
  
  // Each subquery transformation uses an aggregate function that "magically"
  // upholds the ANSI semantics for the transformed subqueries.
  virtual ItemExpr * getBlackBoxExpr(NormWA & normWARef, ValueIdSet & aggSet);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  // we want Subquery to be cacheable
  virtual NABoolean isCacheableExpr(CacheWA& cwa);
  
  // do NOT parameterize literals of Subquery if we 
  // can NOT guarantee the correctness of such parameterization
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA) 
    { return this; }

  // append an ascii-version of Subquery into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cachewa) const;

  // return true if ItemExpr & its descendants have no constants 
  // and no noncacheable nodes
  virtual NABoolean hasNoLiterals(CacheWA& cwa);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap); //<aviv>
  
  inline NABoolean avoidHalloweenR2() const 
  { return avoidHalloweenR2_ ? TRUE : FALSE; }
  inline GenericUpdate *getAvoidHalloweenR2() const 
  { return avoidHalloweenR2_; }
  inline void setAvoidHalloweenR2(GenericUpdate *gu) 
  { avoidHalloweenR2_ = gu; }
  
  // output degree functions to handle multi-output subqueries.
  // Returns the number of outputs and the ItemExpr for each.
  virtual Int32     getOutputDegree();
  virtual ItemExpr *getOutputItem(UInt32 i);

 private:

  // Set to true if we have flattened out a subquery that has a degree
  // > 1. This flag is used for assign::doSynthesizeType() to not check
  // the subquery's type but rather the type of the first element in the 
  // select list.
  NABoolean proxyForFirstElementInSelectList_;


  // If non-Null, then this subquery could potentially run into the
  // Halloween problem.  This subquery is part of a insert in which
  // the source contains a reference to the target. We check for this
  // in binder and issue an error in many cases, but in some cases we
  // allow it and set this flag to TRUE in the insert and the subquery
  // that contains the reference to the target. If this flag is set,
  // then we must generate a safe plan WRT the Halloween problem.  For
  // the subquery operator, this means that the join resulting from
  // subquery transformation should be implemented with a hash
  // join. This causes the source to be blocked.
  //
  GenericUpdate *avoidHalloweenR2_;

  // The derived table.
  RelExpr * tableExpr_;
  
}; // class Subquery

// ***********************************************************************
// Row subquery 
// ***********************************************************************
class RowSubquery : public Subquery
{
  // ITM_ROW_SUBQUERY
public:

  RowSubquery(RelExpr * tableExpr)
         : Subquery(ITM_ROW_SUBQUERY, tableExpr) {}

  virtual ~RowSubquery() {}

  // a virtual function for type propagating the node
  virtual const NAType *synthesizeType();

  // a virtual function for type propagating new type to the node
  virtual const NAType *pushDownType(NAType &desiredType,
                       enum NABuiltInTypeEnum defaultQualifier);

  virtual ItemExpr * getBlackBoxExpr(NormWA & normWARef, ValueIdSet & aggSet);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap); //<aviv>

};


// ***********************************************************************
// Quantified comparison subquery  
// "value op all|some|any (subquery)" (returns true,false, or null)
// ***********************************************************************
class QuantifiedComp : public Subquery
{ 
  // ITM_EQUAL_ALL, ITEM_EQUAL_ANY,
  // ITM_NOT_EQUAL_ALL, ITM_NOT_EQUAL_ANY,
  // ITM_LESS_ALL, ITM_LESS_ANY,
  // ITM_GREATER_ALL, ITM_GREATER_ANY,
  // ITM_LESS_EQ_ALL, ITM_LESS_EQ_ANY,
  // ITM_GREATER_EQ_ALL, ITM_GREATER_EQ_ANY
public:

  QuantifiedComp(OperatorTypeEnum otype,
                 ItemExpr * scalarExpr,
                 RelExpr  * tableExpr,
                 GenericUpdate *avoidHalloweenR2) 
         : Subquery(otype, tableExpr, scalarExpr),
	   createdFromINlist_(FALSE),
	   createdFromALLpred_(FALSE)
   {
    setAvoidHalloweenR2(avoidHalloweenR2);
  }

  virtual ~QuantifiedComp() {}

  // a virtual function for type propagating the node
  virtual const NAType *synthesizeType();

  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual ItemExpr * getBlackBoxExpr(NormWA & normWARef, ValueIdSet & aggSet);

  // in a subtree that is rooted in a NOT.
  virtual ItemExpr * transformSubtreeOfNot(NormWA & normWARef,
                                           OperatorTypeEnum falseOrNot);
  
  // A virtual method for transforming this predicate's child
  // only if the child itself contains a subquery.
  // Returns NULL if no transformation was necessary.
  virtual ItemExpr * transformMultiValuePredicate(
                                          NABoolean flattenSubqueries = TRUE,
					  ChildCondition tfmIf = ANY_CHILD);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // A helper function to compute the comparison operator to
  // use during subquery transformation
  OperatorTypeEnum getSimpleOperatorType() const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap); //<aviv>
  
  void setCreatedFromINlist(NABoolean v){createdFromINlist_ = v;}
  NABoolean createdFromINlist() { return createdFromINlist_;}

  void setCreatedFromALLpred(NABoolean v){createdFromALLpred_ = v;}
  NABoolean createdFromALLpred() { return createdFromALLpred_;}

private:
  NABoolean createdFromINlist_;
  NABoolean createdFromALLpred_;

}; // class QuantifiedComp

// ***********************************************************************
// IN subquery : a synonym for an = ANY subquery
// ***********************************************************************
class InSubquery : public Subquery
{ 
  // ITM_IN_SUBQUERY
public:

  InSubquery(OperatorTypeEnum otype,
		    ItemExpr * scalarExpr,
		    RelExpr  * tableExpr) 
         : Subquery(otype, tableExpr, scalarExpr) {}

  virtual ~InSubquery() {}

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap); //<aviv>
};

// ***********************************************************************
// EXISTS subquery  (returns either a true or a false; never a null)
// ***********************************************************************
class Exists : public Subquery
{ 
  // ITM_EXISTS, ITM_NOT_EXISTS
public:

  Exists(RelExpr  * tableExpr);

  virtual ~Exists();

  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType *synthesizeType();

  virtual ItemExpr * transformSubtreeOfNot(NormWA & normWARef,
                                           OperatorTypeEnum falseOrNot);

  virtual ItemExpr * getBlackBoxExpr(NormWA & normWARef, ValueIdSet & aggSet);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode, CollHeap* outHeap); //<aviv>

};

#endif /* ITEMSUBQ_H */

