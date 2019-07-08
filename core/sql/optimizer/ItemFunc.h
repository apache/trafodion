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
#ifndef ITEMFUNC_H
#define ITEMFUNC_H
/* -*-C++-*-
******************************************************************************
*
* File:         ItemFunc.h
* Description:  Functions and Aggregate item expressions
*		Aggregates to be used in groupby rel. operators.
*               Functions (and aggregates?) may be built-ins or user-defined.
* Created:      11/04/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------
// When adding new BuiltinFunctions herein,
// take heed of the problem documented at Abs::preCodeGen() ...
// -----------------------------------------------------------------------
#include "IntervalType.h"
#include "CharType.h"
#include "DatetimeType.h"
#include "ItemExpr.h"
#include "NATable.h"
#include "exp_like.h"
#include "QRExprElement.h"
#include "ExpLOBenums.h"

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class CharType;
class DisjunctArray;
class Generator;
class RangePartitioningFunction;
class IntegerList;
class NARoutine;
class ItemList;
class RoutineDesc;

// -----------------------------------------------------------------------
// aggregate functions
//
// The aggregate functions designed for subquery transformations are:
// ITM_ONE_ROW  :
//   Used for transforming a scalar or a row subquery.
//   Its argument is the select list of the subquery.
//   It returns the first group formed by these values.
//   It returns a null if there are no groups.
//   It issues an error if there is more than one group.
// ITM_ONE_TRUE:
//   Used for transforming an exists subquery
//   Its argument is the select list of the subquery.
//   It returns TRUE if the result of the subquery is not empty
//   It returns FALSE, otherwise.
// ITM_ANY_TRUE:
//   Used in for IN, = ANY, <> ANY, = ALL, <> ALL subqueries.
//   Its argument is a boolean predicate that expresses
//   the original predicate without the quantifier.
//   This aggregate function returns a boolean value.
//   It returns TRUE  its argument evaluates to TRUE for any row in
//                    the group,
//   It returns NULL  if its argument evaluates to NULL for at least one
//                    row in the group and if for no row the argument
//                    evaluates to TRUE,
//   It returns FALSE if its argument evaluates to FALSE for each row in
//                    the group or if the group is empty.
//   In other words, it returns an indication whether it is provable that
//   for any row in the group its boolean argument evaluates to TRUE.
//   For > ANY, < ANY, ... subqueries, more specialized functions can be
//   used that allow MIN/MAX optimizations (see class QuantifiedAggregate).
// -----------------------------------------------------------------------
class Aggregate : public ItemExpr
{
  // ITM_AVG, ITM_MAX, ITM_MIN, ITM_SUM, ITM_COUNT,
  // ITM_ONE_ROW, ITM_ONEROW, ITM_ONE_TRUE, ITM_ANY_TRUE
public:
  Aggregate(OperatorTypeEnum otype,
	    ItemExpr *child0,
	    NABoolean isDistinct,
	    OperatorTypeEnum otypeSpecifiedByUser,
	    char /*just to disambiguate from next ctor!*/)
    : ItemExpr(otype, child0),
      origChild_(child0),
      isDistinct_(isDistinct),
      inScalarGroupBy_(FALSE),
      distinctId_(NULL_VALUE_ID),
      treatAsACount_(FALSE),
      amTopPartOfAggr_(FALSE),
      isOLAP_(FALSE),
      olapPartitionBy_(NULL),
      olapOrderBy_(NULL),
      frameStart_(-INT_MAX),
      frameEnd_(INT_MAX),
      rollupGroupIndex_(-1)
    { setOrigOpType(otypeSpecifiedByUser); }
  Aggregate(OperatorTypeEnum otype,
	    ItemExpr *child0 = NULL,
	    NABoolean isDistinct = FALSE,
	    ValueId distinctId = NULL_VALUE_ID)
    : ItemExpr(otype, child0),
      origChild_(child0),
      isDistinct_(isDistinct),
      inScalarGroupBy_(FALSE),
      distinctId_(distinctId),
      treatAsACount_(FALSE),
      amTopPartOfAggr_(FALSE),
      isOLAP_(FALSE),
      olapPartitionBy_(NULL),
      olapOrderBy_(NULL),
      frameStart_(-INT_MAX),
      frameEnd_(INT_MAX),
      rollupGroupIndex_(-1)
    {}
  Aggregate(OperatorTypeEnum otype,
	    ItemExpr *child0,
	    ItemExpr *child1,
	    NABoolean isDistinct = FALSE,
	    ValueId distinctId = NULL_VALUE_ID)
    : ItemExpr(otype, child0, child1),
      origChild_(child0),
      isDistinct_(isDistinct),
      inScalarGroupBy_(FALSE),
      distinctId_(distinctId),
      treatAsACount_(FALSE),
      amTopPartOfAggr_(FALSE),
      isOLAP_(FALSE),
      olapPartitionBy_(NULL),
      olapOrderBy_(NULL),
      frameStart_(-INT_MAX),
      frameEnd_(INT_MAX),
      rollupGroupIndex_(-1)
    {}

  // virtual destructor
  virtual ~Aggregate();

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  virtual NABoolean isAnAggregate() const;
  virtual NABoolean containsAnAggregate() const;

  // Are two Aggregates equivalent?
  virtual NABoolean operator == (const ItemExpr& other) const;

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  // a method for initializing the result
  void setAggrResult(ValueId vid) 		{ result_ = vid; }
  ValueId getAggrResult() 			{ return result_; }

  inline NABoolean isDistinct() const   	{ return isDistinct_; }
  void setDistinct(NABoolean dist = TRUE)
  {
    isDistinct_ = dist;
    distinctId_ = dist ? child(0)->getValueId() : NULL_VALUE_ID;
  }
  inline void setDistinctValueId(ValueId vid)   { distinctId_ = vid; }
  ValueId getDistinctValueId() const	        { return distinctId_; }

  // Agregates in non-Scalar GroupBy cannot evaluate to null
  // if their argument is not nullable
  inline NABoolean inScalarGroupBy() const         { return inScalarGroupBy_;}
  inline void setInScalarGroupBy()                 { inScalarGroupBy_ = TRUE;}

  NABoolean treatAsACount() const { return treatAsACount_;}
  void setTreatAsACount() { treatAsACount_ = TRUE;}

  const OperatorTypeEnum getEffectiveOperatorType() const {
    if(treatAsACount()) {
      return ITM_COUNT_NONULL;
    } 
    return getOperatorType();
  }

  NABoolean topPartOfAggr() const { return amTopPartOfAggr_;}
  void setTopPartOfAggr() { amTopPartOfAggr_ = TRUE;}

  virtual NABoolean isSensitiveToDuplicates() const;

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInputs);

  // Each operator supports a (virtual) method for transforming its
  // query tree to a canonical form.
  virtual ItemExpr * normalizeNode(NormWA & normWARef);

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  // --------------------------------------------------------------------
  // Walk through an ItemExpr tree and gather the ValueIds of those
  // expressions that behave as if they are "leaves" for the sake of
  // the coverage test, e.g., expressions that have no children, or
  // aggregate functions, or instantiate null. These are usually values
  // that are produced in one "scope" and referenced above that "scope"
  // in the dataflow tree for the query.
  // --------------------------------------------------------------------
  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;

  // method to do code generation
  virtual short codeGen(Generator*);
  virtual void codegen_and_set_attributes(Generator *, Attributes **, Lng32);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // A method for determining whether the evaluation of this aggregate
  // function can be staged.
  virtual NABoolean evaluationCanBeStaged() const;

  // rewriting aggregate functions for elimination, parallelization
  virtual ItemExpr * rewriteForElimination();
  virtual ItemExpr * rewriteForStagedEvaluation(
       ValueIdList &initialAggrs,
       ValueIdList &finalAggrs,
       NABoolean sameFormat = FALSE);

  NABoolean isEquivalentForBinding(const ItemExpr * other);

  virtual ValueId mapAndRewrite(ValueIdMap &map,
				NABoolean mapDownwards = FALSE);

  virtual NABoolean duplicateMatch(const ItemExpr & other) const;

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // produce an ascii-version of the object (for display or saving into a file)
  virtual void unparse(NAString &result,
		       PhaseEnum phase = DEFAULT_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;

  #define ITM_COUNT_STAR__ORIGINALLY	REL_AGGREGATE
  ItemExpr *getOriginalChild() const		   { return origChild_; }
  void setOriginalChild(ItemExpr *c)		   { origChild_ = c; }

  NABoolean isOneRowTransformed_;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}
  NABoolean isOLAP() { return isOLAP_; };

  void setOLAPInfo(ItemExpr *partBy, 
                   ItemExpr *orderBy,
                   Lng32 rowsStart,
                   Lng32 rowsEnd)
  {
    olapPartitionBy_ = partBy;
    olapOrderBy_ = orderBy;
    frameStart_ = rowsStart;
    frameEnd_ = rowsEnd;
    isOLAP_ = TRUE;
  };
  /*
  ItemExpr *getOlapPartitionBy() { return olapPartitionBy_; };
  ItemExpr *getOlapOrderBy() { return olapOrderBy_; };
  */

  Lng32 getframeStart()
  {
    return frameStart_;
  }

  Lng32 getframeEnd()
  {
    return frameEnd_;
  }

    NABoolean isFrameStartUnboundedPreceding() const
  {
    return (frameStart_ == -INT_MAX);
  }

   NABoolean isFrameStartUnboundedFollowing() const
  {
    return (frameStart_ == INT_MAX);
  }
  NABoolean isFrameEndUnboundedPreceding() const
  {
    return (frameEnd_ == -INT_MAX);
  }

  NABoolean isFrameEndUnboundedFollowing() const
  {
    return (frameEnd_ == INT_MAX);
  }

  virtual QR::ExprElement getQRExprElem() const;

  void setRollupGroupIndex(Int16 v) { rollupGroupIndex_ = v; }
  Int16 getRollupGroupIndex() { return rollupGroupIndex_; }

private:

  // The ValueId of the Item Expression that represents the result.
  // For example, if T(a int not null, b int not null), then the
  // result of sum(a,b) are the expressions a', b' where a' and b'
  // are both nullable.
  ValueId  result_;

  // indicate whether this is a DISTINCT aggregate
  NABoolean isDistinct_;

  // indicate whether this aggregate is for a scalar group by
  NABoolean inScalarGroupBy_;

  // ValueId of the distinct expression. usually, this is the
  // valueId of the child itemExpr, but in cases such as Variance,
  // it could be something different.
  ValueId distinctId_;

  // The original child specified by the user, before Parser and/or Binder
  // transformed it to something else.
  ItemExpr *origChild_;


  void setOlapOrderBy(ItemExpr *orderBy) { olapOrderBy_ = orderBy;};

  ItemExpr *removeOlapOrderBy()
  {
    ItemExpr *orderBy = olapOrderBy_;
    olapOrderBy_ = NULL;
    return orderBy;
  };

  ItemExpr *transformOlapFunction( BindWA *bindWA );
  OperatorTypeEnum mapOperTypeToRunning() const;
  OperatorTypeEnum mapOperTypeToOlap() const;
  OperatorTypeEnum mapOperTypeToMoving() const;

  // OLAP Info
  NABoolean isOLAP_;

  ItemExpr *olapPartitionBy_;
  ItemExpr *olapOrderBy_;

  Lng32 frameStart_;
  Lng32 frameEnd_;

  // true iff am top part of a rewritten COUNT aggregate
  NABoolean treatAsACount_;

  // true iff am top part of a rewritten aggregate
  NABoolean amTopPartOfAggr_;

  // this field indicates the index into rollupGroupExprList of GroupByAgg
  // that corresponds to the child of GROUPING aggr.
  Int16 rollupGroupIndex_;
}; // class Aggregate


// Variance -  Variance is an aggregate itemExpr derived from the
// Aggregate class.  This class implements the compiler side of the Variance
// and Stddev aggregates. This new class redefines the {con,de}structor, the
// bindNode method, and the getText method.  The other methods are NEVER called
// for this class and have (for now) been redefined to ASSERT if called.
//
// This node will be replaced during binding by an ItemExpr tree rooted
// by a ScalarVariance node. This new tree is bound and returned as the
// result of Variance::bindNode(). Because of this, the Variance node
// should never appear after binding.
//
class Variance : public Aggregate
{
public:
  // Variance implements the Item types ITM_VARIANCE, ITM_STDDEV
  // These Item types are also used by the ScalarVariance node and the
  // ExFunctionVariance and ExFunctionStddev builtin functions.

  // Constructor. - This aggregate can have one or two children.
  // The optional second parameter (child) is the weighting factor.
  //
  Variance(OperatorTypeEnum otype,
		  ItemExpr *child0 = NULL,
		  ItemExpr *child1 = NULL,
		  NABoolean isDistinct = FALSE)
    : Aggregate(otype, child0, child1, isDistinct)
  {}

  // virtual destructor
  virtual ~Variance();

  // a virtual function for performing name binding within the query tree.
  // This method will replace the Variance node with a tree of nodes rooted
  // by a ScalarVariance node. This new tree is bound and returned as the
  // result of bindNode of Variance. Because of this, the Variance node
  // should never appear after binding.
  //
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // MV --
  // In multi-delta refresh, copyTopNode() is called to dulicate the
  // unbound expression.
  virtual ItemExpr * copyTopNode(ItemExpr * = NULL, CollHeap* = 0);

  // The following virtual methods should never be called on the Variance
  // node.  They have been redefined to ASSERT if called.
  //
  virtual void transformNode(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInputs)  {CMPASSERT(0)}
  virtual ItemExpr * normalizeNode(NormWA & normWARef){CMPASSERT(0); return 0;}
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const
  						  {CMPASSERT(0); return TRUE;}
  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                      const GroupAttributes& coveringGA, const ValueIdSet & newExternalInputs
		      ) const {CMPASSERT(0);}
  virtual short codeGen(Generator*)		  {CMPASSERT(0); return 0;}
  virtual NABoolean evaluationCanBeStaged() const {CMPASSERT(0); return TRUE;}
  virtual ItemExpr * rewriteForElimination()	  {CMPASSERT(0); return 0;}
  virtual ItemExpr * rewriteForStagedEvaluation(
       ValueIdList &initialAggrs,
       ValueIdList &finalAggrs,
       NABoolean sameFormat = FALSE)		  {CMPASSERT(0); return 0;}


private:
}; // class Variance

class PivotGroup : public Aggregate
{
public:
  enum PivotOptionType
  {
    DELIMITER_,
    ORDER_BY_,
    MAX_LENGTH_
  };

  enum {
    DEFAULT_MAX_LEN = 1024
  };

  class PivotOption
  {
    friend class PivotGroup;
  public:
  PivotOption(PivotOptionType option, 
              void * optionNode = NULL,
              char * stringVal = NULL,
              Long numericVal = 0)
    : option_(option), 
      optionNode_(optionNode),
      stringVal_(stringVal),
      numericVal_(numericVal)
    {}
    
  private:
    PivotOptionType option_;
    void * optionNode_;
    Long   numericVal_;
    char * stringVal_;
  };

  // PivotGroup implements the Item ITM_PIVOT_GROUP.
  // This is used to implement pivot groups.

  // Constructor. - This aggregate can have one or two children.
  // The optional second parameter (child) is the weighting factor.
  //
  PivotGroup(OperatorTypeEnum otype,
             ItemExpr *child0,
             NAList<PivotOption*> * pivotOptionsList,
             NABoolean isDistinct = FALSE);

  PivotGroup(OperatorTypeEnum otype,
             ItemExpr *child0,
             NAString &delim,
             NABoolean orderBy,
             ValueIdList &reqdOrder,
             Lng32 maxLen,
             NABoolean isDistinct = FALSE)
  : Aggregate(otype, child0, NULL, isDistinct),
    delim_(delim),
    orderBy_(orderBy),
    reqdOrder_(reqdOrder),
    maxLen_(maxLen)
    {}
  
  // virtual destructor
  virtual ~PivotGroup();

  const NAType *synthesizeType();

  // a virtual function for performing name binding within the query tree.
  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual ItemExpr * copyTopNode(ItemExpr * = NULL, CollHeap* = 0);

  virtual ItemExpr * preCodeGen(Generator * generator);

  virtual short codeGen(Generator*);

  // append an ascii-version of BETWEEN into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  virtual NABoolean evaluationCanBeStaged() const {return TRUE;}
  virtual ItemExpr * rewriteForStagedEvaluation(
       ValueIdList &initialAggrs,
       ValueIdList &finalAggrs,
       NABoolean sameFormat = FALSE);

  NAString &delim() { return delim_; }
  NABoolean orderBy() { return orderBy_;}
  ValueIdList &reqdOrder() { return reqdOrder_; }
  Lng32 maxLen() { return maxLen_; }
  ItemExpr *getOrderbyItemExpr() { return orgReqOrder_; } 
private:
  NAList<PivotOption*> * pivotOptionsList_;

  NAString delim_;

  NABoolean orderBy_;
  ValueIdList reqdOrder_;   	// ORDER BY list
  ItemExpr *orgReqOrder_;
  
  Lng32 maxLen_;
}; // class PivotGroup

// -----------------------------------------------------------------------
// Function operators supporting a variable number of children
// -----------------------------------------------------------------------
class Function : public ItemExpr
{
  // ITM_BETWEEN, ITM_LIKE, ITM_USER_DEF_FUNCTION
public:

  Function(OperatorTypeEnum otype = ITM_USER_DEF_FUNCTION,
           NAMemory *h = CmpCommon::statementHeap(),
	   Lng32 argumentCount = 0,
	   ItemExpr *child0 = NULL,
	   ItemExpr *child1 = NULL,
	   ItemExpr *child2 = NULL,
	   ItemExpr *child3 = NULL,
	   ItemExpr *child4 = NULL,
	   ItemExpr *child5 = NULL);
  
  Function(OperatorTypeEnum otype,
           const LIST(ItemExpr *) &children,
           CollHeap *h=CmpCommon::statementHeap());
  
  //Function(OperatorTypeEnum otype, NABoolean isLeaf)
  //  : ItemExpr(otype),
  //    children_(CmpCommon::statementHeap()),
  //    allowsSQLnullArg_(TRUE)
  //{};
  
  // virtual destructor
  virtual ~Function();

  // ## This should be moved to ExprNode.h.
  // ## See how Aggregate::getArity() duplicates its logic...
  Lng32 getNumChildren() const;

  NABoolean &allowsSQLnullArg()		     	{ return allowsSQLnullArg_; }

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual NABoolean duplicateMatch(const ItemExpr & other) const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // since this operator may have variable arity, the access operators
  // to its children are overloaded and use a private child list
  virtual ExprValueId & operator[] (Lng32 ix);
  virtual const ExprValueId & operator[] (Lng32 ix) const;

  // for cases where a named method is more convenient than operator []
  ExprValueId & child(Lng32 index)      		{ return operator[](index); }
  const ExprValueId & child(Lng32 index) const	{ return operator[](index); }

  ARRAY(ExprValueId) &children()  { return children_; }

  virtual NABoolean isOrderPreserving() const;

  // by default, we do NOT want Function itemexprs to be cacheable as a
  // guard against false cache hits. However, we selectively override this
  // in derived classes after guarding against false cache hits there.
  virtual NABoolean isCacheableExpr(CacheWA& cwa) { return FALSE; }

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const
    { ItemExpr::generateCacheKey(cwa); }

  virtual QR::ExprElement getQRExprElem() const;

private:

  // Whether the SQL NULL keyword (our NULL ConstValue) may appear
  // in the argument list of this function.
  NABoolean allowsSQLnullArg_;

  ARRAY(ExprValueId) children_;

};

// -----------------------------------------------------------------------
// Built-in functions
// -----------------------------------------------------------------------
class BuiltinFunction : public Function
{
  // ITM_BETWEEN, ITM_LIKE, ITM_CURRENT and more
public:
  BuiltinFunction(OperatorTypeEnum otype,
                  NAMemory *h = CmpCommon::statementHeap(),
		  Lng32  argumentCount = 0,
		  ItemExpr *child0 = NULL,
		  ItemExpr *child1 = NULL,
		  ItemExpr *child2 = NULL,
		  ItemExpr *child3 = NULL,
		  ItemExpr *child4 = NULL,
		  ItemExpr *child5 = NULL);
  
  BuiltinFunction(OperatorTypeEnum otype,
                  NAMemory *h,
		  const LIST(ItemExpr *) &children)
       : Function(otype,children,h) {}
  
  // virtual destructor
  virtual ~BuiltinFunction();

  // get the degree of this node (it depends on the type of the operator)
  virtual Int32 getArity() const;

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInputs);

  // Each operator supports a (virtual) method for transforming its
  // query tree to a canonical form.
  virtual ItemExpr * normalizeNode(NormWA & normWARef);

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // see ExprNode.h for explanation.
  virtual const NAString getTextForError() const;

  // This method tells if a VEG transformation can potentially
  // change the result of this function.
  // For ex., a query "select char_length(a) from t where a = 'v'"
  // where column 'a' is 2 bytes, will return 1 if column 'a' in
  // char_length function is replaced by constant "v".
  // This method must be redefined to return TRUE for
  // any function that can get changed by a VEG transformation.
  // See BuiltinFunction::bindNode in BindItemExpr.cpp on
  // how these functions are protected.
  virtual NABoolean protectFromVEGs()
  {
    return FALSE;
  }

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const
    { Function::generateCacheKey(cwa); }

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

private:

}; // class BuiltinFunction

class CacheableBuiltinFunction : public BuiltinFunction
{
public:
  CacheableBuiltinFunction
    (OperatorTypeEnum otype,
     Lng32  argumentCount = 0,
     ItemExpr *child0 = NULL,
     ItemExpr *child1 = NULL,
     ItemExpr *child2 = NULL,
     ItemExpr *child3 = NULL,
     ItemExpr *child4 = NULL,
     ItemExpr *child5 = NULL)
    : BuiltinFunction
      (otype,CmpCommon::statementHeap(),
       argumentCount,child0,child1,child2,child3,child4,child5) {}

  // classes derived from CacheableBuiltinFunction are cacheable
  virtual NABoolean isCacheableExpr(CacheWA& cwa)
    { return ItemExpr::isCacheableExpr(cwa); }

  // -----------------------------------------------------------------
  // NB: By default, we want to parameterize us and our children. This 
  // is accomplished by inheriting our base class' normalizeForCache.
  // If it is unsafe to parameterize, we (the derived class)
  // must supply its own implementation of normalizeForCache.
  // -----------------------------------------------------------------

  // -----------------------------------------------------------------------
  // NB: If derived class has its own data member(s), we must make its data
  // member(s) contribute to its cache key via our own generateCacheKey here.
  // ------------------------------------------------------------------------
}; // class CacheableBuiltinFunction

//++Triggers,
//
// Built in function that should be evaluate only once
class EvaluateOnceBuiltinFunction : public BuiltinFunction
{
public:
  EvaluateOnceBuiltinFunction(OperatorTypeEnum otype,
			 Lng32  argumentCount = 0,
			 ItemExpr *child0 = NULL,
			 ItemExpr *child1 = NULL,
			 ItemExpr *child2 = NULL,
			 ItemExpr *child3 = NULL,
			 ItemExpr *child4 = NULL,
			 ItemExpr *child5 = NULL)
    : BuiltinFunction(otype,CmpCommon::statementHeap(),argumentCount,
                      child0,child1,child2,child3,child4,child5)
     {}

  inline EvaluateOnceBuiltinFunction(OperatorTypeEnum otype,
			 const LIST(ItemExpr *) &children)
			 : BuiltinFunction(otype,
                                           CmpCommon::statementHeap(),
                                           children) {}

  // virtual destructor
  virtual ~EvaluateOnceBuiltinFunction();

  // should return always true
  virtual NABoolean isAUserSuppliedInput() const;

  // should return always false
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	          ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

}; // class EvaluateOnceBuiltinFunction


class UniqueExecuteId : public EvaluateOnceBuiltinFunction
{
public:
  UniqueExecuteId()
	: EvaluateOnceBuiltinFunction(ITM_UNIQUE_EXECUTE_ID) {}

  // virtual destructor
  virtual ~UniqueExecuteId();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);


  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class UniqueExecuteId

class GetTriggersStatus : public EvaluateOnceBuiltinFunction
{
public:
  GetTriggersStatus()
	: EvaluateOnceBuiltinFunction(ITM_GET_TRIGGERS_STATUS) {}

  // virtual destructor
  virtual ~GetTriggersStatus();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);


  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class GetTriggersStatus

class GetBitValueAt : public BuiltinFunction
{
public:
  GetBitValueAt(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
       : BuiltinFunction(ITM_GET_BIT_VALUE_AT, CmpCommon::statementHeap(), 
                         2, val1Ptr, val2Ptr)
	 { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~GetBitValueAt();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class GetBitValueAt
//--Triggers,

// MV,
// Functions that are additional outputs of GenericUpdate nodes.
// Used for MV Inlining.
// Supported functions: ITM_CURRENTEPOCH, ITM_VSBBROWTYPE, ITM_VSBBROWCOUNT.
class GenericUpdateOutputFunction : public BuiltinFunction
{
public:
  GenericUpdateOutputFunction(OperatorTypeEnum otype,
			 Lng32  argumentCount = 0,
			 ItemExpr *child0 = NULL,
			 ItemExpr *child1 = NULL,
			 ItemExpr *child2 = NULL,
			 ItemExpr *child3 = NULL,
			 ItemExpr *child4 = NULL,
			 ItemExpr *child5 = NULL)
    : BuiltinFunction(otype,CmpCommon::statementHeap(),
                      argumentCount,child0,child1,child2,child3,child4,child5)
     {}

  inline GenericUpdateOutputFunction(OperatorTypeEnum otype,
			 const LIST(ItemExpr *) &children)
			 : BuiltinFunction(otype,
                                           CmpCommon::statementHeap(),
                                           children) {}

  // virtual destructor
  virtual ~GenericUpdateOutputFunction() {}

  // should return always false
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	          ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const
  { return FALSE; }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // Is this a subclass of GenericUpdateOutput BuildinFunction?
  virtual NABoolean isAGenericUpdateOutputFunction() const
  { return TRUE; }

}; // class GenericUpdateOutputFunction

//++MV
class IsBitwiseAndTrue : public BuiltinFunction
{
public:
  IsBitwiseAndTrue(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
       : BuiltinFunction(ITM_IS_BITWISE_AND_TRUE, CmpCommon::statementHeap(),
                         2, val1Ptr, val2Ptr)
	 { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~IsBitwiseAndTrue();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class GetBitValueAt
//--MV

class CodeVal : public CacheableBuiltinFunction
{
public:
  CodeVal(OperatorTypeEnum op, ItemExpr *val1Ptr)
    : CacheableBuiltinFunction(op, 1, val1Ptr)
    { allowsSQLnullArg() = FALSE; }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  ItemExpr* tryToRelaxCharTypeMatchRules(BindWA *bindWA);
  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  // special version of getText() that can handle NO_OPERATOR_TYPE opertor type.
  const NAString getText() const;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class CodeVal

/////////////////////////////////////////////////////////////////
// This function is created by generator and is used to
// evaluate MIN and MAX aggr functions. It is created based
// on the original Aggregate node created by the parser.
// The first child points to
// the child of original Aggregate node. The second child points
// to an expression whose result, if TRUE, indicates that the first
// child is the new aggregate value.
// For MIN, the second child is:   "child1 < min-aggr"
// For MAX, the second child is:   "child1 > max-aggr".
// See generateAggrExpr in GenExpGenerator.C for details.
/////////////////////////////////////////////////////////////////
class AggrMinMax : public BuiltinFunction
{
public:
  AggrMinMax(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
    : BuiltinFunction(ITM_AGGR_MIN_MAX, CmpCommon::statementHeap(),
                      2, val1Ptr, val2Ptr) {}

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class AggrMinMax

/////////////////////////////////////////////////////////////////
// This function is created by generator and is used to
// evaluate GROUPING function.
/////////////////////////////////////////////////////////////////
class AggrGrouping : public BuiltinFunction
{
public:
  AggrGrouping(Int16 rollupGroupIndex = -1)
       : BuiltinFunction(ITM_AGGR_GROUPING_FUNC, CmpCommon::statementHeap()),
         rollupGroupIndex_(rollupGroupIndex)
  {}

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

private:
  // this field indicates the index into rollupGroupExprList of GroupByAgg
  // that corresponds to the child of GROUPING aggr.
  Int16 rollupGroupIndex_;

}; // class AggrGrouping

class AnsiUSERFunction : public BuiltinFunction
{
public:
  AnsiUSERFunction(OperatorTypeEnum oper)
         : BuiltinFunction(oper) {}

  // virtual destructor
  virtual ~AnsiUSERFunction();

  virtual NABoolean isAUserSuppliedInput() const;

  virtual ItemExpr * bindNode(BindWA *bindWA);
  virtual const NAType * synthesizeType();
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class AnsiUSERFunction

// Tandem extension, USER(stringArg),
// as opposed to the Ansi USER keyword (no parens).
class MonadicUSERFunction : public BuiltinFunction
{
public:
  MonadicUSERFunction(ItemExpr *val1Ptr,OperatorTypeEnum otype = ITM_USER)
       : BuiltinFunction(otype, CmpCommon::statementHeap(),
                         1, val1Ptr) {}

  // virtual destructor
  virtual ~MonadicUSERFunction();

  virtual NABoolean isAUserSuppliedInput() const;

  virtual ItemExpr * bindNode(BindWA *bindWA);
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;


}; // class MonadicUSERFunction

// Tandem extension, OS_USERID(stringArg),
class MonadicUSERIDFunction : public BuiltinFunction
{
public:
  MonadicUSERIDFunction(ItemExpr *val1Ptr)
       : BuiltinFunction(ITM_USERID, CmpCommon::statementHeap(),
                         1, val1Ptr) {}

  // virtual destructor
  virtual ~MonadicUSERIDFunction();

  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class MonadicUSERIDFunction

class Between : public CacheableBuiltinFunction
{
public:

  // boundriesIncluded for values L,R meens [L,R]. not meens (L,R).
  // the direction vector is for the ASC/DESC attributes of the values item
  Between(ItemExpr *colPtr,
	  ItemExpr *val1Ptr,
	  ItemExpr *val2Ptr,
	  NABoolean leftBoundryIncluded = TRUE, //++ MV OZ
	  NABoolean rightBoundryIncluded = TRUE //++ MV OZ
	  )
         : leftBoundryIncluded_(leftBoundryIncluded),
	   rightBoundryIncluded_(rightBoundryIncluded),
	   pDirectionVector_(NULL),
	   CacheableBuiltinFunction(ITM_BETWEEN, 3, colPtr, val1Ptr, val2Ptr)
  {
    allowsSQLnullArg() = FALSE;
  }

  // virtual destructor
  virtual ~Between();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA * bindWA);

  // if we are allowing certain incompatible comparisons handle them.
  // currently the following incompatible comparisons are supported:
  // 1. Date and Character literal
  // This returns a pointer to itself if everything goes fine
  // otherwise it returns NULL
  Between * handleIncompatibleComparison(BindWA *bindWA);

  //++ MV OZ
  void setDirectionVector(const IntegerList & directionVector,
			  CollHeap* heap);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean isAPredicate() const { return TRUE; }

  // check and apply substr transformation
  ItemExpr* checkAndApplySubstrTransformation();

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInputs);

  // A method for inverting (finding the inverse of) the operators
  // in a subtree that is rooted in a NOT.
  virtual ItemExpr * transformSubtreeOfNot(NormWA & normWARef,
                                           OperatorTypeEnum falseOrNot);

  // A virtual method for transforming this predicate
  // if all children (by default; can be any one child) are ItemList.
  // Returns NULL if no transformation was necessary.
  virtual ItemExpr * transformMultiValuePredicate(
				    NABoolean flattenSubqueries = TRUE,
				    ChildCondition condBiRelat = ANY_CHILD);

  ItemExpr * transformIntoTwoComparisons();

  // produce an ascii-version of the object (for display or saving into a file)
  virtual void unparse(NAString &result,
		       PhaseEnum phase = DEFAULT_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;

  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);

  // for now, do NOT change literals of BETWEEN into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA)
    { return this; } 

  // append an ascii-version of BETWEEN into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

private:

  NABoolean    leftBoundryIncluded_;
  NABoolean    rightBoundryIncluded_;
  IntegerList *pDirectionVector_;

}; // class Between


class Overlaps: public CacheableBuiltinFunction
{
public:
  Overlaps(ItemExpr* d1, ItemExpr* e1, ItemExpr* d2, ItemExpr* e2)
    : CacheableBuiltinFunction(ITM_OVERLAPS, 4, d1, e1, d2, e2)
  {}

  virtual ~Overlaps(){};

  virtual ItemExpr *bindNode(BindWA * bindWA);
  virtual const NAType *synthesizeType();
  virtual ItemExpr *copyTopNode(ItemExpr *derivedNode = NULL
                                , CollHeap *outHeap = 0);
  virtual NABoolean isAPredicate() const {return true;}
  virtual void unparse(NAString &result
                       , PhaseEnum phase = DEFAULT_PHASE
                       , UnparseFormatEnum form = USER_FORMAT
                       , TableDesc *tabId = NULL) const;

  virtual ItemExpr *preCodeGen(Generator*);

};// class Overlaps

class BoolResult : public BuiltinFunction
{
  // Evaluates the final boolean result value for a predicate.
  // A NULL or FALSE final result becomes FALSE.
  // TRUE final result remains TRUE.
public:
  BoolResult(ItemExpr *val1Ptr)
       : BuiltinFunction(ITM_BOOL_RESULT, CmpCommon::statementHeap(),
                         1, val1Ptr)
      {}

  // virtual destructor
  virtual ~BoolResult();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class BoolResult

class CharFunc : public BuiltinFunction
{
public:
  CharFunc(CharInfo::CharSet cs, ItemExpr *val1Ptr)
    : BuiltinFunction((cs == CharInfo::UNICODE) ? ITM_UNICODE_CHAR : ITM_CHAR,
                      CmpCommon::statementHeap(),
                      1, val1Ptr),
      charSet_(cs)
    { allowsSQLnullArg() = FALSE; }

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

private:
  CharInfo::CharSet charSet_;

  virtual NABoolean protectFromVEGs() { return TRUE; };

}; // class CharFunc


class PatternMatchingFunction : public CacheableBuiltinFunction
{
public:
  PatternMatchingFunction(OperatorTypeEnum optype, ItemExpr *matchValue, ItemExpr *pattern)
  : CacheableBuiltinFunction(optype, 2, matchValue, pattern),
   numberOfNonWildcardChars_(-1), patternAStringLiteral_(FALSE),
   bytesInNonWildcardChars_(-1),
   oldDefaultSelForLikeWildCardUsed_(FALSE), beginEndKeysApplied_(FALSE)
  { 
    allowsSQLnullArg() = FALSE;
    setCollation(CharInfo::DefaultCollation);
  }

  PatternMatchingFunction(OperatorTypeEnum optype, ItemExpr *matchValue, ItemExpr *pattern, ItemExpr *escapeChar)
  : CacheableBuiltinFunction(optype, 3, matchValue, pattern, escapeChar),
   numberOfNonWildcardChars_(-1), patternAStringLiteral_(FALSE),
   bytesInNonWildcardChars_(-1),
   oldDefaultSelForLikeWildCardUsed_(FALSE), beginEndKeysApplied_(FALSE)
  { 
    allowsSQLnullArg() = FALSE; 
    setCollation(CharInfo::DefaultCollation);
  }

  PatternMatchingFunction(OperatorTypeEnum optype,ItemExpr *matchValue, ItemExpr *pattern, Int32 numNonWild, Int32 bytesInNonWild,
    NABoolean stringPattern, NABoolean oldDefaultUsed, 
    NABoolean beginEndKeysApplied)
  : CacheableBuiltinFunction(optype, 2, matchValue, pattern),
   numberOfNonWildcardChars_(numNonWild), 
   bytesInNonWildcardChars_(bytesInNonWild),
   patternAStringLiteral_(stringPattern),
   oldDefaultSelForLikeWildCardUsed_(oldDefaultUsed),
   beginEndKeysApplied_(beginEndKeysApplied)
  { 
    allowsSQLnullArg() = FALSE;
    setCollation(CharInfo::DefaultCollation);
  }

  PatternMatchingFunction(OperatorTypeEnum optype,ItemExpr *matchValue, ItemExpr *pattern, ItemExpr *escapeChar,  
    Int32 numNonWild, Int32 bytesInNonWild,NABoolean stringPattern, NABoolean oldDefaultUsed,
    NABoolean beginEndKeysApplied)
  : CacheableBuiltinFunction(optype, 3, matchValue, pattern, escapeChar),
   numberOfNonWildcardChars_(numNonWild), 
   bytesInNonWildcardChars_(bytesInNonWild),
   patternAStringLiteral_(stringPattern),
   oldDefaultSelForLikeWildCardUsed_(oldDefaultUsed),
   beginEndKeysApplied_(beginEndKeysApplied)
  { 
    allowsSQLnullArg() = FALSE; 
    setCollation(CharInfo::DefaultCollation);
  }

  // virtual destructor
  virtual ~PatternMatchingFunction();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // change only constant matchValue of a PatternMatchingFunction into ConstantParameter
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // append an ascii-version of PatternMatchingFunction into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0) { return NULL; }

  // predicateEliminatesNullAugmentedRows() determines whether the
  // the predicate is capable of discarding null augmented rows 
  // produced by a left join.

  virtual NABoolean predicateEliminatesNullAugmentedRows(NormWA &, ValueIdSet &);

  virtual NABoolean isAPredicate() const { return TRUE; }

  // The 'LIKE' builtin function doesn't override the virtual function
  // synthSupportedOp, because it doesn't have shape-changing impact on
  // histograms.  But, it does override the virtual function defaultSel,
  // because a LIKE's default selectivity varies based on the pattern
  // being matched.
  virtual double defaultSel();

  NABoolean beginEndKeysApplied(CollHeap *outHeap);
  Int32 getNoOfNonWildcardChars()    { return numberOfNonWildcardChars_ ; };
  Int32 getBytesInNonWildcardChars()    { return bytesInNonWildcardChars_ ; };

  virtual NABoolean protectFromVEGs() { return TRUE; };

  double computeSelForNonWildcardChars();

  NABoolean isPatternAStringLiteral() { return patternAStringLiteral_ ;}
  void setPatternAStringLiteral(NABoolean flag = TRUE) 
      { patternAStringLiteral_ = flag; }

  CharInfo::Collation getCollation()
  {
    return collation_;
  }

  void setCollation(CharInfo::Collation c)
  {
    collation_ = c;
  }
  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

   virtual void unparse(NAString &result,
     PhaseEnum phase = DEFAULT_PHASE,
     UnparseFormatEnum form = USER_FORMAT,
     TableDesc * tabId = NULL) const;


  virtual ItemExpr *applyBeginEndKeys(BindWA *bindWA, ItemExpr *boundExpr, CollHeap *h) { return NULL; }
protected:
  void setNumberOfNonWildcardChars(const LikePatternString &pattern);

  Int32 numberOfNonWildcardChars_;
  Int32 bytesInNonWildcardChars_;

  NABoolean patternAStringLiteral_;

  NABoolean oldDefaultSelForLikeWildCardUsed_;

  NABoolean beginEndKeysApplied_;

  CharInfo::Collation collation_;
  
}; // class PatternMatchingFunction


class Like : public PatternMatchingFunction
{
public:
  Like(ItemExpr *matchValue, ItemExpr *pattern)
  : PatternMatchingFunction(ITM_LIKE, matchValue, pattern)
  { 
    allowsSQLnullArg() = FALSE;
    setCollation(CharInfo::DefaultCollation);
  }

  Like(ItemExpr *matchValue, ItemExpr *pattern, ItemExpr *escapeChar)
  : PatternMatchingFunction(ITM_LIKE, matchValue, pattern, escapeChar)
  { 
    allowsSQLnullArg() = FALSE; 
    setCollation(CharInfo::DefaultCollation);
  }

  Like(ItemExpr *matchValue, ItemExpr *pattern, Int32 numNonWild, Int32 bytesInNonWild,
    NABoolean stringPattern, NABoolean oldDefaultUsed, 
    NABoolean beginEndKeysApplied)
  : PatternMatchingFunction(ITM_LIKE, matchValue, pattern,numNonWild,bytesInNonWild,
     stringPattern,oldDefaultUsed, beginEndKeysApplied)
  { 
    allowsSQLnullArg() = FALSE;
    setCollation(CharInfo::DefaultCollation);
  }

  Like(ItemExpr *matchValue, ItemExpr *pattern, ItemExpr *escapeChar,  
    Int32 numNonWild, Int32 bytesInNonWild,NABoolean stringPattern, NABoolean oldDefaultUsed,
    NABoolean beginEndKeysApplied)
  : PatternMatchingFunction(ITM_LIKE, matchValue, pattern, escapeChar,numNonWild,bytesInNonWild,
     stringPattern,oldDefaultUsed,beginEndKeysApplied) 
  { 
    allowsSQLnullArg() = FALSE; 
    setCollation(CharInfo::DefaultCollation);
  }

  // virtual destructor
  virtual ~Like();
  ItemExpr *applyBeginEndKeys(BindWA *bindWA, ItemExpr *boundExpr, CollHeap *h);
  ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
}; // class Like

class Regexp : public PatternMatchingFunction
{
public:
  Regexp(ItemExpr *matchValue, ItemExpr *pattern)
  : PatternMatchingFunction(ITM_REGEXP, matchValue, pattern)
  { 
    allowsSQLnullArg() = FALSE;
    setCollation(CharInfo::DefaultCollation);
  }

  Regexp(ItemExpr *matchValue, ItemExpr *pattern, Int32 numNonWild, Int32 bytesInNonWild,
    NABoolean stringPattern, NABoolean oldDefaultUsed, 
    NABoolean beginEndKeysApplied)
  : PatternMatchingFunction(ITM_REGEXP, matchValue, pattern,numNonWild,bytesInNonWild,
     stringPattern,oldDefaultUsed, beginEndKeysApplied)
  { 
    allowsSQLnullArg() = FALSE;
    setCollation(CharInfo::DefaultCollation);
  }

  // virtual destructor
  virtual ~Regexp();
  ItemExpr *applyBeginEndKeys(BindWA *bindWA, ItemExpr *boundExpr, CollHeap *h);
  ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
}; // class Regexp
 
class NoOp : public BuiltinFunction
{
  // Does nothing. An ItemExpr place holder. Used by
  // generator to indicate the end of genrated clauses.
public:
  NoOp(ItemExpr *val1Ptr)
       : BuiltinFunction(ITM_NO_OP, CmpCommon::statementHeap(),
                         1, val1Ptr)
      {};

  // virtual destructor
  virtual ~NoOp();

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class NoOp

// this class return the specified boolean value (TRUE, FALSE, or NULL).
class BoolVal : public BuiltinFunction
{
public:
  BoolVal(OperatorTypeEnum bool_type)
         : BuiltinFunction(bool_type) {}
  BoolVal(OperatorTypeEnum bool_type, ItemExpr *val1Ptr)
         : BuiltinFunction(bool_type,  CmpCommon::statementHeap(),
                           1, val1Ptr) {}

  // virtual destructor
  virtual ~BoolVal();

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  
  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}
  // MDAM related methods
  // Performs the MDAM tree walk.  See ItemExpr.h for a detailed description.
  DisjunctArray * mdamTreeWalk();

private:

}; // class BoolVal

class In : public CacheableBuiltinFunction
{
public:
  In(ItemExpr *leftPtr, ItemExpr *rightPtr)
         : CacheableBuiltinFunction(ITM_IN, 2, leftPtr, rightPtr) {}

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class In


class Concat : public CacheableBuiltinFunction
{
public:
  Concat(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
         : CacheableBuiltinFunction(ITM_CONCAT, 2, val1Ptr, val2Ptr)
	 { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~Concat();

  // do not change literals of a cacheable query into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA)
    { return this; }

  // The following method enables expressions such as
  // iso88591_column = :ucs2_hv1 || :ucs2_hv2
  NABoolean isCharTypeMatchRulesRelaxable()
  { return child(0)->isCharTypeMatchRulesRelaxable() &&
           child(1)->isCharTypeMatchRulesRelaxable();
  } ;

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class Concat

class ConvertHex : public CacheableBuiltinFunction
{
public:
  ConvertHex(OperatorTypeEnum op, ItemExpr *val1Ptr)
         : CacheableBuiltinFunction(op, 1, val1Ptr)
	 { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~ConvertHex();

  // do not change literals of a cacheable query into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA)
    { return this; }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class ConvertHex

class CharLength : public CacheableBuiltinFunction
{
public:
  CharLength(ItemExpr *val1Ptr)
         : CacheableBuiltinFunction(ITM_CHAR_LENGTH, 1, val1Ptr)
	 { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~CharLength();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class CharLength

class OctetLength : public CacheableBuiltinFunction
{
public:
  OctetLength(ItemExpr *val1Ptr)
         : CacheableBuiltinFunction(ITM_OCTET_LENGTH, 1, val1Ptr)
	 { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~OctetLength();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  virtual NABoolean protectFromVEGs() { return TRUE; };

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class OctetLength

class PositionFunc : public CacheableBuiltinFunction
{
public:
  PositionFunc(ItemExpr *val1Ptr, ItemExpr *val2Ptr, ItemExpr *val3Ptr,
               ItemExpr *val4Ptr)
         : CacheableBuiltinFunction(ITM_POSITION, 4, 
                                    val1Ptr, val2Ptr, val3Ptr, val4Ptr)
	 { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~PositionFunc();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  virtual ItemExpr * preCodeGen(Generator*);


  virtual NABoolean protectFromVEGs() { return TRUE; };

  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  void setCollation(CharInfo::Collation  v) 
  {
    collation_= v;
  }
  
  CharInfo::Collation getCollation() 
  { 
    return collation_;
  }


private:
  CharInfo::Collation collation_;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

  

}; // class PositionFunc

class Substring : public CacheableBuiltinFunction
{
public:
  Substring(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
         : CacheableBuiltinFunction(ITM_SUBSTR, 2, val1Ptr, val2Ptr)
	 { allowsSQLnullArg() = FALSE; }
  Substring(ItemExpr *val1Ptr, ItemExpr *val2Ptr, ItemExpr *val3Ptr)
         : CacheableBuiltinFunction(ITM_SUBSTR, 3, val1Ptr, val2Ptr, val3Ptr)
	 { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~Substring();

  // change literals of a cacheable query into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  NABoolean isCharTypeMatchRulesRelaxable()
  { return child(0)->isCharTypeMatchRulesRelaxable(); } ;

  virtual ItemExpr * preCodeGen(Generator*);

  virtual NABoolean protectFromVEGs() { return TRUE; };
  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

  
  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}
  

  

}; // class Substring

class ConvertTimestamp : public CacheableBuiltinFunction
{
public:
  ConvertTimestamp(ItemExpr *val1Ptr)
	: CacheableBuiltinFunction(ITM_CONVERTTIMESTAMP, 1, val1Ptr)
	{ allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~ConvertTimestamp();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual ItemExpr * preCodeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}
}; // class ConvertTimestamp

class SleepFunction : public CacheableBuiltinFunction
{
public:

  SleepFunction( ItemExpr *val1Ptr )
   : CacheableBuiltinFunction(ITM_SLEEP, 1, val1Ptr)
  {}
  // virtual destructor
  virtual ~SleepFunction();

  // A method that returns for "user-given" input values.
  // These are values that are either constants, host variables, parameters,
  // or even values that are sensed from the environment such as
  // current time, the user name, etcetera.
  virtual NABoolean isAUserSuppliedInput() const;

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class SleepFunction 

class UnixTimestamp : public CacheableBuiltinFunction
{
public:
  UnixTimestamp()
   : CacheableBuiltinFunction(ITM_UNIX_TIMESTAMP)
  {}

  UnixTimestamp( ItemExpr *val1Ptr )
   : CacheableBuiltinFunction(ITM_UNIX_TIMESTAMP, 1, val1Ptr)
  {}
  // virtual destructor
  virtual ~UnixTimestamp();

  // A method that returns for "user-given" input values.
  // These are values that are either constants, host variables, parameters,
  // or even values that are sensed from the environment such as
  // current time, the user name, etcetera.
  virtual NABoolean isAUserSuppliedInput() const;

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class UnixTimestamp

class CurrentTimestamp : public CacheableBuiltinFunction
{
public:
 CurrentTimestamp(DatetimeType::Subtype dtCode = DatetimeType::SUBTYPE_SQLTimestamp,
                  Lng32 fractPrec = SQLTimestamp::DEFAULT_FRACTION_PRECISION)
   : CacheableBuiltinFunction(ITM_CURRENT_TIMESTAMP),
    dtCode_(dtCode),
    fractPrec_(fractPrec)
  {}

  // virtual destructor
  virtual ~CurrentTimestamp();

  // A method that returns for "user-given" input values.
  // These are values that are either constants, host variables, parameters,
  // or even values that are sensed from the environment such as
  // current time, the user name, etcetera.
  virtual NABoolean isAUserSuppliedInput() const;

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

  static ItemExpr * construct
    (CollHeap * heap, 
     DatetimeType::Subtype dtCode = DatetimeType::SUBTYPE_SQLTimestamp,
     Lng32 fractPrec = SQLTimestamp::DEFAULT_FRACTION_PRECISION);

 private:
  DatetimeType::Subtype dtCode_;
  Lng32 fractPrec_;
}; // class CurrentTimestamp

class CurrentTimestampRunning : public CacheableBuiltinFunction
{
public:
  CurrentTimestampRunning()
	: CacheableBuiltinFunction(ITM_CURRENT_TIMESTAMP_RUNNING) {}

  // virtual destructor
  virtual ~CurrentTimestampRunning();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  
  

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}
}; // class CurrentTimestampRunning

class DateFormat : public CacheableBuiltinFunction
{
public:
  enum FormatType
  {
    FORMAT_GENERIC = 0,
    FORMAT_TO_DATE,
    FORMAT_TO_TIME,
    FORMAT_TO_CHAR
  };

  enum 
  { 
    DATE_FORMAT_NONE = 0,
    DATE_FORMAT_STR,       // formatStr_ contains date format    
    TIME_FORMAT_STR,       // formatStr_ contains time format
    TIMESTAMP_FORMAT_STR   // formatStr_ contains timestamp format
  };

  DateFormat(ItemExpr *val1Ptr, const NAString &formatStr,
             Lng32 formatType, NABoolean wasDateformat = FALSE);

  // virtual destructor
  virtual ~DateFormat();

  // accessor functions
  Int32 getDateFormat() const { return dateFormat_; }

  Int32 getExpDatetimeFormat() const { return frmt_; }

  // do not change format literals of DateFormat into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA)
    { return this; } 

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  NABoolean errorChecks(Lng32 frmt, BindWA *bindWA, const NAType* opType);

  ItemExpr * quickDateFormatOpt(BindWA * bindWA);

  ItemExpr * bindNode(BindWA * bindWA);

  void setOriginalString(NAString &s) {origString_ = s; }
  const NAString & getOriginalString() const { return origString_; }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  
  void unparse(NAString &result,
               PhaseEnum phase,
               UnparseFormatEnum form,
               TableDesc * tabId) const;
    
  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

  virtual QR::ExprElement getQRExprElem() const
  { 
    return QR::QRFunctionWithParameters; 
  }

private:
  // string contains user specified format string
  NAString formatStr_;

  // TO_DATE or TO_CHAR
  Lng32 formatType_;

  // DATE, TIME or TIMESTAMP
  Int32 dateFormat_;

  // original function was DATEFORMAT
  NABoolean wasDateformat_; 

  // actual datetime format (defined in class ExpDatetime in exp_datetime.h)
  Lng32 frmt_;

  //original string
  NAString origString_;
}; // class DateFormat

class DayOfWeek : public CacheableBuiltinFunction
{
public:
  DayOfWeek(ItemExpr *val1Ptr)
	: CacheableBuiltinFunction(ITM_DAYOFWEEK, 1, val1Ptr)
	{ allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~DayOfWeek();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

}; // class DayOfWeek

///////////////////////////////////////////////////////////////////
// This class explodes or fixes up a varchar for insert or updates.
// It does the following:
//   If forInsert is TRUE, then:
//      -- if the source is a NULL value, then the null indicator
//         is set and the length bytes of varchar are set to 0.
//      -- if the source is non-null, then they are moved as packed
//         format and the length bytes contain the actual length.
//
//  If forInsert is FALSE, then:
//     -- if the source is NULL, then null indicator bytes are set
//        to non-zero, the length bytes are set to the max length.
//     -- if source is non-null, then the source is blankpadded to
//        max value and then moved in. Length bytes are set to max.
////////////////////////////////////////////////////////////////////
class ExplodeVarchar : public BuiltinFunction
{
public:
  ExplodeVarchar(ItemExpr *val1Ptr, const NAType * type,
		 NABoolean forInsert = FALSE)
        : BuiltinFunction(ITM_EXPLODE_VARCHAR,  CmpCommon::statementHeap(),
                          1, val1Ptr),
          type_(type), forInsert_(forInsert)
  {}

  // virtual destructor
  virtual ~ExplodeVarchar();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();
  const NAType *getType() const { return type_; }

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual NABoolean protectFromVEGs() { return TRUE; };

private:
  const NAType *type_;

  NABoolean forInsert_;
}; // class ExplodeVarchar

class Extract : public CacheableBuiltinFunction
{
public:
  Extract(rec_datetime_field extractField, ItemExpr *val1Ptr,
	  NABoolean fieldFunction)
	: CacheableBuiltinFunction(ITM_EXTRACT, 1, val1Ptr),
	  extractField_(extractField),
	  fieldFunction_(fieldFunction)
	  { allowsSQLnullArg() = FALSE; }

  Extract(OperatorTypeEnum op,
	  rec_datetime_field extractField, ItemExpr *val1Ptr,
	  NABoolean fieldFunction)
       : CacheableBuiltinFunction(op, 1, val1Ptr),
	 extractField_(extractField),
	 fieldFunction_(fieldFunction)
         { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~Extract();

  // accessor functions
  rec_datetime_field getExtractField() const	{ return extractField_; }

  NABoolean getFieldFunction() const { return fieldFunction_; }

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual ItemExpr * preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  // returns TRUE for Extract(cast(CurrentTimeStamp))) otherwise FALSE.
  virtual NABoolean isAUserSuppliedInput() const;

  // --------------------------------------------------------------------
  // Walk through an ItemExpr tree and gather the ValueIds of those
  // expressions that behave as if they are "leaves" for the sake of
  // the coverage test. In this case expressions of the type
  // extract(cast(current_timestamp))  will be treated like a leaf.
  // --------------------------------------------------------------------
  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;

  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

  virtual QR::ExprElement getQRExprElem() const;
  

private:

  rec_datetime_field extractField_;

  // set to TRUE if the original function specified was a datetime
  // field function that was translated to an EXTRACT function.
  // Ex.  DAY(<value>)  gets translated to EXTRACT(DAY from <value>).
  // Used for correct error reporting.
  NABoolean fieldFunction_;
}; // class Extract

class ExtractOdbc : public Extract
{
public:
  ExtractOdbc(rec_datetime_field extractField, ItemExpr *val1Ptr,
	      NABoolean fieldFunction)
       : Extract(ITM_EXTRACT_ODBC, extractField, val1Ptr, fieldFunction)
  { allowsSQLnullArg() = FALSE; }

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
}; // class ExtractOdbc

class JulianTimestamp : public CacheableBuiltinFunction
{
public:
  JulianTimestamp(ItemExpr *val1Ptr)
	: CacheableBuiltinFunction(ITM_JULIANTIMESTAMP, 1, val1Ptr)
	{ allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~JulianTimestamp();

  // A method that returns for "user-given" input values.
  // These are values that are either constants, host variables, parameters,
  // or even values that are sensed from the environment such as
  // current time, the user name, etcetera.
  // Fix for cr 10-010718-3967
  // don't need this method since this method is hardcoded to return TRUE
  // and JulianTimestamp is not a user supplied input, therefore we should
  // just be fine calling ItemExpr::isAUserSuppliedInput which always returns
  // FALSE
  // virtual NABoolean isAUserSuppliedInput() const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual ItemExpr * preCodeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  
  
  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}
  
}; // class JulianTimestamp

// supplied by the root node, a count of statement executions
class StatementExecutionCount : public BuiltinFunction
{
public:
  StatementExecutionCount() : BuiltinFunction(ITM_EXEC_COUNT) {}

  virtual NABoolean isAUserSuppliedInput() const;
  virtual const NAType * synthesizeType();
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
};

// supplied by the root node, the current transaction id, if any
class CurrentTransId : public BuiltinFunction
{
public:
  CurrentTransId() : BuiltinFunction(ITM_CURR_TRANSID) {}

  virtual NABoolean isAUserSuppliedInput() const;
  virtual const NAType * synthesizeType();
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
};

class Upper : public CacheableBuiltinFunction
{
public:
  Upper(ItemExpr *val1Ptr)
         : CacheableBuiltinFunction(ITM_UPPER, 1, val1Ptr)
	 { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~Upper();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // do not change literals of a cacheable query into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA)
    { return this; }

  NABoolean isCharTypeMatchRulesRelaxable()
  { return child(0)->isCharTypeMatchRulesRelaxable(); } ;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class Upper

class Lower : public CacheableBuiltinFunction
{
public:
  Lower(ItemExpr *val1Ptr)
         : CacheableBuiltinFunction(ITM_LOWER, 1, val1Ptr)
	 { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~Lower();

  // do not change literals of a cacheable query into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA)
    { return this; }

  NABoolean isCharTypeMatchRulesRelaxable()
  { return child(0)->isCharTypeMatchRulesRelaxable(); } ;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class Lower

class Trim : public CacheableBuiltinFunction
{
public:
  enum    { TRAILING, LEADING, BOTH};

  Trim(Int32  mode, ItemExpr *val1Ptr, ItemExpr *val2Ptr)
         : CacheableBuiltinFunction(ITM_TRIM, 2, val1Ptr, val2Ptr),
           mode_(mode)
	   { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~Trim();

  // for now, do NOT change literals of Trim into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA)
    { return this; } 

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  inline Int32 getTrimMode() const { return mode_; }

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * preCodeGen(Generator*);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  // The following method enables expressions such as
  // iso88591_column = trim(BOTH :ucs2_hv1 FROM :ucs2_hv2)
  NABoolean isCharTypeMatchRulesRelaxable()
  { return child(0)->isCharTypeMatchRulesRelaxable() &&
           child(1)->isCharTypeMatchRulesRelaxable();
  } ;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  virtual NABoolean protectFromVEGs() { return TRUE; };

  void setCollation(CharInfo::Collation  v) 
  {
    collation_= v;
  }
  
  CharInfo::Collation getCollation() 
  { 
    return collation_;
  }
  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

  virtual QR::ExprElement getQRExprElem() const
  { 
    return QR::QRFunctionWithParameters; 
  }

private:
  Int32    mode_;  // holds a value from one of the enum members shown above.
  CharInfo::Collation collation_;

}; // class Trim

class Translate: public CacheableBuiltinFunction
{
public:
  enum {/*UCS2*/UNICODE_TO_SJIS, /*UCS2*/UNICODE_TO_ISO88591,
        ISO88591_TO_UNICODE/*UCS2*/, SJIS_TO_UNICODE/*UCS2*/,
        UCS2_TO_SJIS, SJIS_TO_UCS2,
        UCS2_TO_UTF8, UTF8_TO_UCS2,
        UTF8_TO_SJIS, SJIS_TO_UTF8, UTF8_TO_ISO88591,
        ISO88591_TO_UTF8,
        KANJI_MP_TO_ISO88591, KSC5601_MP_TO_ISO88591,
        GBK_TO_UTF8,
        UNKNOWN_TRANSLATION};

  Translate(ItemExpr *valPtr, NAString* map_table_name);
  Translate(ItemExpr *valPtr, Int32 map_table_id);

  // virtual destructor
  virtual ~Translate() {};

  // for now, do NOT change literals of Translate into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA)
    { return this; } 

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  inline Int32 getTranslateMapTableId() const { return map_table_id_; }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;}
  ItemExpr* tryToRelaxCharTypeMatchRules(BindWA *bindWA);

  NABoolean  isCharTypeMatchRulesRelaxable();

  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  virtual QR::ExprElement getQRExprElem() const
   { return QR::QRFunctionWithParameters; }

  virtual void unparse(NAString &result,
		       PhaseEnum phase = DEFAULT_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;
private:
  Int32 map_table_id_;  // holds a value from one of the enum members shown above.

}; // class Translate

Int32 find_translate_type( CharInfo::CharSet src_cs,   // Source charset
                           CharInfo::CharSet dest_cs ); // Destination charset


// The CASE statement has an operand which is not one of its children,
// which introduces a few special cases in Binder and in Codegen type-synth...
class Case : public CacheableBuiltinFunction
{
public:
  Case(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
  : CacheableBuiltinFunction(ITM_CASE, 1, val2Ptr),
    caseOperand_(val1Ptr),
    caseOperandWasNullable_(FALSE)
    {}

  // virtual destructor
  virtual ~Case();

  // accessor functions
  ItemExpr *removeCaseOperand()
  {
    ItemExpr *caseOperand = caseOperand_;
    caseOperand_ = NULL;
    return caseOperand;
  }

  ItemExpr *getCaseOperand()		     { return caseOperand_; }

  void setCaseOperand(ItemExpr *newOperand)  { caseOperand_ = newOperand; }

  NABoolean &caseOperandWasNullable()	     { return caseOperandWasNullable_; }

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // A transformation method for protecting sequence functions from not
  // being evaluated due to short-circuit evaluation.
  //
  virtual void protectiveSequenceFunctionTransformation(Generator *generator);

  // method to do code generation
  virtual short codeGen(Generator*);

  // we want Case to be cacheable
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // change literals of a cacheable query into input parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cachewa, BindWA& bindWA);

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cachewa) const;

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  // This method recursively removes from coveredSubExpr,
  // all IfThenElse nodes that are part of 'expr' and for those IfThenElse
  // nodes, adds its children to coveredSubExpr.
  // input: expr, a non-null IfThenElse node.
  void fixIfThenElse(ItemExpr * expr, ValueIdSet& coveredSubExpr, NABoolean ifThenElseExists = FALSE) const;

  virtual void unparse(NAString &result,
		       PhaseEnum phase = DEFAULT_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;

  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);
private:
  ItemExpr *caseOperand_;
  NABoolean caseOperandWasNullable_;
}; // class Case

class IfThenElse : public CacheableBuiltinFunction
{
public:
  IfThenElse(ItemExpr *val1Ptr, ItemExpr *val2Ptr, ItemExpr *val3Ptr)
  : CacheableBuiltinFunction(ITM_IF_THEN_ELSE, 3, val1Ptr, val2Ptr, val3Ptr) {}

  // virtual destructor
  virtual ~IfThenElse();

  // for now, do NOT change literals of IfThenElse into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA)
    { return this; } 

  // set else clause
  void setElse(ItemExpr *elseClause) { child(2) = elseClause; }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  NABoolean isCharTypeMatchRulesRelaxable();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  short codeGen(Generator*);

  virtual void unparse(NAString &result,
		       PhaseEnum phase = DEFAULT_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}
}; // class IfThenElse


// The RaiseError class is introduced to raise an error
// given the errorCode and the message. This expression
// will create an error in the Diagnostics area and
// return EXPR_ERROR. Currently this ItemExpr is used
// to raise errors for constraint failures.
//
class RaiseError : public BuiltinFunction
{
public:
  // -- Triggers
  // This Ctor is used for SIGNAL statements with a string expression.
  RaiseError (Lng32 sqlcode,
	      NAString SqlState,
              ItemExpr *messageExpr,
              CollHeap * h=0)
    : BuiltinFunction(ITM_RAISE_ERROR,  CmpCommon::statementHeap(),
                      1, messageExpr),
      theSQLCODE_(sqlcode),
      constraintName_(SqlState, h),
      tableName_("", h)
    {};

  RaiseError (Lng32 sqlcode = 0,
	      const NAString & constraintName = "",
	      const NAString & tableName = "",
              const NAString & optionalStr = "",
              const NAType *type = NULL,
              CollHeap * h=0)
    : BuiltinFunction(ITM_RAISE_ERROR),
      theSQLCODE_(sqlcode),
      constraintName_(constraintName, h),
      tableName_(tableName, h),
      optionalStr_(optionalStr, h),
      type_(type)
    {};

  // copy ctor
  RaiseError (const RaiseError &, CollHeap * h=0) ; // not written

  virtual ~RaiseError();

  const NAType *synthesizeType();
  short codeGen (Generator*);
  ItemExpr *copyTopNode (ItemExpr *derivedNode = NULL, CollHeap *outHeap =NULL);
  virtual const NAString getText() const;

  const NAString &getConstraintName() const	{ return constraintName_; }
  const NAString &getTableName() const		{ return tableName_; }
  Int32 getSQLCODE() const			{ return theSQLCODE_; }
  void setSQLCODE(Lng32 sqlcode) 		{ theSQLCODE_ = sqlcode; }

private:
  Lng32    theSQLCODE_;
  NAString constraintName_;
  NAString tableName_;

  NAString optionalStr_;
  const NAType * type_;
}; // class RaiseError


class Cast : public CacheableBuiltinFunction
{
public:
  Cast(ItemExpr *val1Ptr, const NAType *type,
       OperatorTypeEnum otype = ITM_CAST, NABoolean checkForTrunc = FALSE,
       NABoolean noStringTruncationWarnings = FALSE);

  Cast(ItemExpr *val1Ptr, ItemExpr *errorOutPtr, const NAType *type,
       OperatorTypeEnum otype /* today, should be ITM_NARROW always */,
       NABoolean checkForTrunc = FALSE,
       NABoolean reverseDataErrorConversionFlag = FALSE,
       NABoolean noStringTruncationWarnings = FALSE);

  // virtual destructor
  virtual ~Cast();

  // accessor functions
  ItemExpr * getExpr() const { return child(0).getPtr(); }

  const NAType *getType() const { return type_; }
  void changeType(NAType * newType) { type_ = newType; }

  virtual NABoolean isEquivalentForCodeGeneration(const ItemExpr * other);

  virtual ConstValue *castToConstValue(NABoolean & negate);

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // determine whether the output of the cast can be type relaxed
  NABoolean isCharTypeMatchRulesRelaxable()
  { return child(0)->isCharTypeMatchRulesRelaxable(); } ;

  // convert a cast on top of a constant into a new constant with
  // the correct type
  virtual ItemExpr *foldConstants(ComDiagsArea *diagsArea,
				  NABoolean newTypeSynthesis = FALSE);

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  // is any literal in this expr safely coercible to its target type?
  virtual NABoolean isSafelyCoercible(CacheWA& cwa) const;

  // MVs --
  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;

  // Is this a subclass of GenericUpdateOutputFunction BuildinFunction?
  virtual NABoolean isAGenericUpdateOutputFunction() const
    { return child(0)->isAGenericUpdateOutputFunction(); }

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // method to do code generation
  virtual ItemExpr * preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  NABoolean checkTruncationError()	{ return checkForTruncation_; }
  void setCheckTruncationError(NABoolean v) { checkForTruncation_ = v; }

  // get and set for flags_. See enum Flags.
  NABoolean matchChildType()   { return (flags_ & MATCH_CHILD_TYPE) != 0; }
  void setMatchChildType(NABoolean v)
  { (v ? flags_ |= MATCH_CHILD_TYPE : flags_ &= ~MATCH_CHILD_TYPE); }

  NABoolean treatAllSpacesAsZero()
  { return ((flags_ & TREAT_ALL_SPACES_AS_ZERO) != 0); };
  
  void setTreatAllSpacesAsZero(NABoolean v)
  { (v) ? flags_ |= TREAT_ALL_SPACES_AS_ZERO : flags_ &= ~TREAT_ALL_SPACES_AS_ZERO; }

  NABoolean tgtCharSetSpecified()   { return (flags_ & TGT_CHAR_SET_SPECIFIED) != 0; }
  void setTgtCSSpecified(NABoolean v)
       { (v ? flags_ |= TGT_CHAR_SET_SPECIFIED : flags_ &= ~TGT_CHAR_SET_SPECIFIED); }

  NABoolean srcIsVarcharPtr()   { return (flags_ & SRC_IS_VARCHAR_PTR) != 0; }
  void setSrcIsVarcharPtr(NABoolean v)
       { (v ? flags_ |= SRC_IS_VARCHAR_PTR : flags_ &= ~SRC_IS_VARCHAR_PTR); }

  NABoolean allowSignInInterval()
    { return ((flags_ & ALLOW_SIGN_IN_INTERVAL) != 0); };

  void setAllowSignInInterval(NABoolean v)
    { (v) ? flags_ |= ALLOW_SIGN_IN_INTERVAL : flags_ &= ~ALLOW_SIGN_IN_INTERVAL; }

  NABoolean convertNullWhenError() 
  { return (flags_ & CONV_NULL_WHEN_ERROR) != 0; }
  void setConvertNullWhenError(NABoolean v)
  { (v ? flags_ |= CONV_NULL_WHEN_ERROR : flags_ &= ~CONV_NULL_WHEN_ERROR); }

  NABoolean noStringTruncationWarnings() 
  { return (flags_ & NO_STRING_TRUNC_WARNINGS) != 0; }
  void setNoStringTruncationWarnings(NABoolean v)
  { (v ? flags_ |= NO_STRING_TRUNC_WARNINGS: flags_ &= ~NO_STRING_TRUNC_WARNINGS); }

  const NAType * pushDownType(NAType& desiredType,
                      enum NABuiltInTypeEnum defaultQualifier);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

  UInt16 getFlags() { return flags_; }
  void setFlags(UInt16 f) { flags_ = f; }

private:

  enum Flags
  {
    // if set, then do not generate code if child's type attributes match
    // my type attributes.
    MATCH_CHILD_TYPE = 0x0001,

    // when converting string to numeric, if input string contains only
    // spaces or is a null string(length of zero), then the result becomes
    // zero(numeric value of 0).
    TREAT_ALL_SPACES_AS_ZERO  = 0x0002,

    // Did user specify the target character set?  0=No, 1=Yes
    TGT_CHAR_SET_SPECIFIED = 0x0004,

    // source is a varchar value which is a pointer to the actual data.
    SRC_IS_VARCHAR_PTR = 0x0008,

    ALLOW_SIGN_IN_INTERVAL             = 0x0010,

    // convert error will not be returned, null will be moved into target
    CONV_NULL_WHEN_ERROR               = 0x0020,

    // string truncation warnings are not to be returned
    NO_STRING_TRUNC_WARNINGS           = 0x0040
  
  };

  const NAType *type_;

  // Does the ex_conv_clause generated by this cast node need to
  // check for truncation error?
  NABoolean checkForTruncation_;

  // If true, the run-time data error conversion flag must be reversed.
  // This is a Narrow-only issue.
  NABoolean reverseDataErrorConversionFlag_;

  UInt32 flags_;

}; // class Cast

class CastConvert : public Cast
{
public:
  CastConvert(ItemExpr *val1Ptr, const NAType *type)
    : Cast(val1Ptr, type, ITM_CAST_CONVERT)
  {}

  // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // CastConvert

class CastType : public Cast
{
public:
  CastType(ItemExpr *val1Ptr, const NAType *type)
       : Cast(val1Ptr, type, ITM_CAST_TYPE),
         makeNullable_(FALSE)
  {}

  CastType(ItemExpr *val1Ptr, NABoolean makeNullable)
       : Cast(val1Ptr, NULL, ITM_CAST_TYPE),
         makeNullable_(makeNullable)
  {}

  // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual short codeGen(Generator*);

private:
  // if set, then cast child to nullable type
  NABoolean makeNullable_;
}; // CastType

class Narrow : public Cast
{
public:
  // Begin_Fix 10-040114-2431
  // 02/18/2004
  // Add initialization for matchChildNullability_
  Narrow(ItemExpr * val1Ptr, ItemExpr * errorOutPtr, const NAType * type,
       OperatorTypeEnum otype = ITM_NARROW,
       NABoolean reverseDataErrorConversionFlag = FALSE,
       NABoolean matchChildNullability = FALSE)
         : Cast(val1Ptr, errorOutPtr, type, otype, FALSE,
                reverseDataErrorConversionFlag),
           matchChildNullability_(matchChildNullability){}
  // End_Fix 10-040114-2431

  // virtual destructor
  virtual ~Narrow();

  // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // a virtual function for type propagating the node
  virtual const NAType *synthesizeType();

  // method to do code generation
  // virtual ItemExpr * preCodeGen(Generator*); -- we use Cast::preCodeGen
  virtual short codeGen(Generator*);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

private:

  // Begin_Fix 10-040114-2431
  // 02/18/2004
  //Force by nullability to be the same as my child's nullability
  NABoolean matchChildNullability_;
  // End_Fix 10-040114-2431
}; // class Narrow

class InstantiateNull : public Cast
{
public:
  InstantiateNull(ItemExpr * valuePtr, const NAType * typePtr)
               : Cast(valuePtr, typePtr, ITM_INSTANTIATE_NULL),
                 beginLOJTransform_(FALSE),
                 NoCheckforLeftToInnerJoin(FALSE){}

  virtual ~InstantiateNull();

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInputs);

  // initiateInnerToLeftJoinTransformation() is invoked when
  // a predicateEliminatesNullAugmentedRows().
  virtual ItemExpr * initiateLeftToInnerJoinTransformation(NormWA &);

  // Each operator supports a (virtual) method for transforming its
  // query tree to a canonical form.
  virtual ItemExpr * normalizeNode(NormWA & normWARef);

  virtual ItemExpr * getReplacementExpr() const;

  //10-060105-3714-Begin
  virtual ItemExpr * getReplacementExpr(NABoolean forceNavigate) const; 
  //10-060105-3714-End
  // --------------------------------------------------------------------
  // Walk through an ItemExpr tree and gather the ValueIds of those
  // expressions that behave as if they are "leaves" for the sake of
  // the coverage test, e.g., expressions that have no children, or
  // aggregate functions, or instantiate null. These are usually values
  // that are produced in one "scope" and referenced above that "scope"
  // in the dataflow tree for the query.
  // --------------------------------------------------------------------
  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  NABoolean NoCheckforLeftToInnerJoin;
  NABoolean lojTransformInProgress() {return beginLOJTransform_;}
  void setLOJTransformComplete() {beginLOJTransform_ = FALSE;}
  virtual NABoolean isAColumnReference( );

  virtual NABoolean hasEquivalentProperties(ItemExpr * other);
  virtual NABoolean isEquivalentForCodeGeneration(const ItemExpr * other);

private:
  NABoolean beginLOJTransform_;
}; // class InstantiateNull

// used to format source values to ascii representation.
//
// And source ascii values to specified target datatype.
// This is only suppported to convert to DATE datatype and if
// 'formatCharToDate' parameter is set to TRUE. 
class Format : public BuiltinFunction
{
  NAString formatStr_;
public:
  enum FormatType
  {
    FORMAT_GENERIC = 0,
    FORMAT_TO_DATE,
    FORMAT_TO_CHAR
  };

  Format(ItemExpr *val1Ptr, const NAString &formatStr, 
	 NABoolean formatCharToDate, Lng32 formatType = FORMAT_GENERIC)
       : BuiltinFunction(ITM_FORMAT,  CmpCommon::statementHeap(),
                         1, val1Ptr),
	 formatStr_(formatStr),
	 formatType_(formatType),
	 formatCharToDate_(formatCharToDate)
  {
    formatStr_.strip(NAString::leading);
    formatStr_.strip(NAString::trailing);
    formatStr_.toUpper();
  }
  
  // virtual destructor
  virtual ~Format();

  // method to do precode generation
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  ItemExpr * quickDateFormatOpt(BindWA * bindWA);

  const NAString& getFormatStr() const
  {
    return formatStr_;
  }

  Lng32 getFormatType() const
  {
    return formatType_;
  }

  NABoolean getFormatCharToDate() const
  {
    return formatCharToDate_;
  }

  virtual QR::ExprElement getQRExprElem() const
  { 
    return QR::QRFunctionWithParameters; 
  }

private:
  Lng32 formatType_;
  NABoolean formatCharToDate_;
}; // class Format


class LOBoper : public BuiltinFunction
{
public:

 enum ObjectType
 {
   STRING_, FILE_, BUFFER_, CHUNK_, STREAM_, LOB_, LOAD_, EXTERNAL_, 
   EXTRACT_, EMPTY_LOB_,NOOP_
 };

 LOBoper(OperatorTypeEnum otype,
	 ItemExpr *val1Ptr=NULL, ItemExpr *val2Ptr = NULL,ItemExpr *val3Ptr = NULL, 
	 ObjectType obj = NOOP_)
   : BuiltinFunction(otype,  CmpCommon::statementHeap(),
                     3, val1Ptr, val2Ptr,val3Ptr),
   obj_(obj),
   lobNum_(-1),
   lobStorageType_(Lob_Invalid_Storage),
     lobMaxSize_((Int64)CmpCommon::getDefaultNumeric(LOB_MAX_SIZE)*1024*1024),
     lobMaxChunkMemSize_((Int64)CmpCommon::getDefaultNumeric(LOB_MAX_CHUNK_MEM_SIZE)*1024*1024),
     lobGCLimit_((Int64) CmpCommon::getDefaultNumeric(LOB_GC_LIMIT_SIZE)*1024*1024),
     hdfsPort_((Lng32)CmpCommon::getDefaultNumeric(LOB_HDFS_PORT)),
     hdfsServer_( CmpCommon::getDefaultString(LOB_HDFS_SERVER))
   {
     if ((obj == STRING_) || (obj == BUFFER_) || (obj == FILE_) || (obj ==LOB_) )
       lobStorageType_ = Lob_HDFS_File;
     else if (obj == EXTERNAL_)
       lobStorageType_ = Lob_External_HDFS_File;
     else if (obj == EMPTY_LOB_)
       lobStorageType_ = Lob_Empty;
     else
       lobStorageType_ = Lob_Invalid_Storage;
    
   }

 // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

  virtual NABoolean isCovered
    (const ValueIdSet& newExternalInputs,
     const GroupAttributes& coveringGA,
     ValueIdSet& referencedInputs,
     ValueIdSet& coveredSubExpr,
     ValueIdSet& unCoveredExpr) const;
  
  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);
  virtual NABoolean isCacheableExpr(CacheWA& cwa);
  virtual void generateCacheKey(CacheWA& cwa) const;

  ObjectType getObj() const { return obj_; }

  short &lobNum() {return lobNum_; }
  LobsStorage &lobStorageType() { return lobStorageType_; }
  NAString &lobStorageLocation() { return lobStorageLocation_; }
  Int64 getLobMaxSize() {return lobMaxSize_; }
  Int64 getLobMaxChunkMemSize() { return lobMaxChunkMemSize_;}
  Int64 getLobGCLimit() 
  { 
    if (lobGCLimit_>0) 
      return (Int64)lobGCLimit_;
    else
      return -1;
  }
  Int32 getLobHdfsPort() { return hdfsPort_;}
  NAString &getLobHdfsServer(){return hdfsServer_;}
 protected:
  ObjectType obj_;

  short lobNum_;
  LobsStorage lobStorageType_;
  NAString lobStorageLocation_;
  Int64 lobMaxSize_; // In byte units
  Int64 lobMaxChunkMemSize_; //In MB Units
  Int64 lobGCLimit_ ;//In MB Units
  Int32 hdfsPort_;
  NAString hdfsServer_;
  
}; // LOBoper

class LOBinsert : public LOBoper
{
 public:
  
 LOBinsert(ItemExpr *val1Ptr, 
	   ItemExpr *val2Ptr,
	   ObjectType fromObj, 
	   NABoolean isAppend = FALSE,
           NABoolean treatLobAsVarchar =FALSE,
	   OperatorTypeEnum otype = ITM_LOBINSERT)
   : LOBoper(otype, val1Ptr, val2Ptr,NULL,fromObj),
    objectUID_(-1),
    append_(isAppend),
    lobAsVarchar_(treatLobAsVarchar),
     lobSize_((Int64)CmpCommon::getDefaultNumeric(LOB_MAX_SIZE)*1024*1024),
    fsType_(REC_BLOB)
    {};
  
  // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  ItemExpr *bindNode(BindWA *bindWA);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();
  
  // method to do code generation
  virtual short codeGen(Generator*);

  // returns TRUE so this could be evaluated in master.
  virtual NABoolean isAUserSuppliedInput() const { return TRUE; };
  virtual void generateCacheKey(CacheWA& cwa) const;
  Int64 & insertedTableObjectUID() { return objectUID_; }
  
  NAString &insertedTableSchemaName() { return schName_; }

  //  Lng32 & lobNum() { return lobNum_; }

  Int64 & lobSize() { return lobSize_; }

  Lng32 & lobFsType() { return fsType_; }
  NABoolean lobAsVarchar() const { return lobAsVarchar_;}
 protected:
  // ---------------------------------------------------------------//
  // ObjectUID of the table this blob is being inserted into
  // -------------------------------------------------------------- //
  Int64 objectUID_;

  // schemaname of the table
  NAString schName_;

  // column this blob is being inserted into.
  //  Lng32 lobNum_;

  Int64 lobSize_;

  Lng32 fsType_;

  NABoolean append_;
  NABoolean lobAsVarchar_;//This means this lob insert will get it's input data 
                          // in varchar format
}; // class LOBinsert

class LOBselect : public LOBoper
{
 public:
  
 LOBselect(ItemExpr *val1Ptr, ItemExpr *val2Ptr, ObjectType toObj)
   : LOBoper(ITM_LOBSELECT, val1Ptr, val2Ptr,NULL,toObj)
    {
    };
  
  // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  
  ItemExpr *bindNode(BindWA *bindWA);
 
  // a virtual function for type propagating the node
 
  // method to do code generation
  virtual short codeGen(Generator*);
  
 private:
 
}; // class LOBselect

class LOBdelete : public LOBoper
{
 public:
  
 LOBdelete(ItemExpr *val1Ptr)
   : LOBoper(ITM_LOBDELETE, val1Ptr)
    {};
  
  // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  
  // method to do code generation
  virtual short codeGen(Generator*);

 private:
}; // class LOBdelete


class LOBupdate : public LOBoper
{
 public:
  
  LOBupdate(ItemExpr *val1Ptr,
	    ItemExpr *val2Ptr,
	    ItemExpr *val3Ptr,
	    ObjectType fromObj, 
	    NABoolean isAppend = FALSE,
            NABoolean treatLobAsVarchar =FALSE)
    : LOBoper(ITM_LOBUPDATE, val1Ptr, val2Ptr,val3Ptr,fromObj),
      objectUID_(-1),
      lobSize_((Int64)CmpCommon::getDefaultNumeric(LOB_MAX_SIZE)*1024*1024),
      append_(isAppend),
      lobAsVarchar_(treatLobAsVarchar)
    {};
  
  // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();
  
  // method to do code generation
  virtual short codeGen(Generator*);
  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);
  virtual void generateCacheKey(CacheWA& cwa) const ;
  // virtual Int32 getArity() const;
  NABoolean isAppend() { return append_; };

  Int64 & updatedTableObjectUID() { return objectUID_; }
  
  NAString &updatedTableSchemaName() { return schName_; }
  Int64 & lobSize() { return lobSize_; }
  NABoolean lobAsVarchar() const { return lobAsVarchar_;}
 private:
  // ---------------------------------------------------------------//
  // ObjectUID of the table this blob is being inserted into
  // -------------------------------------------------------------- //
  Int64 objectUID_;

  // schemaname of the table
  NAString schName_;


  NABoolean append_;
  Int64 lobSize_;
  NABoolean lobAsVarchar_;//This means this lob insert will get it's input data 
                          // in varchar format
}; // class LOBupdate

class LOBconvert : public LOBoper
{
 public:
  
 LOBconvert(ItemExpr *val1Ptr, ObjectType toObj,  Lng32 tgtSize= CmpCommon::getDefaultNumeric(LOB_OUTPUT_SIZE) ) 
   : LOBoper(ITM_LOBCONVERT, val1Ptr, NULL,NULL,toObj),
    tgtSize_(tgtSize)     
    {};
  
  // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  
  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  virtual short codeGen(Generator*);
  virtual void generateCacheKey(CacheWA& cwa) const;
  Lng32 getTgtSize() const{ return tgtSize_; }
 private:
  Lng32 tgtSize_;
 
}; // class LOBconvert

class LOBconvertHandle : public LOBoper
{
 public:
  
 LOBconvertHandle(ItemExpr *val1Ptr, ObjectType toObj)
   : LOBoper(ITM_LOBCONVERTHANDLE, val1Ptr, NULL,NULL,toObj)
    {};
  
  // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  
  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

 private:
}; // class LOBconvertHandle


class LOBextract : public LOBoper
{
 public:
  
 LOBextract(ItemExpr *val1Ptr, Lng32 tgtSize = 1000)
   : LOBoper(ITM_LOBEXTRACT, val1Ptr, NULL,NULL,EXTRACT_),
    tgtSize_(tgtSize)
    {};
  
  // copyTopNode method
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  
  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  //virtual short codeGen(Generator*);
  
  Lng32 getTgtSize() { return tgtSize_; }
 private:
  Lng32 tgtSize_;
}; // class LOBext

class CompEncode : public BuiltinFunction
{
protected:

  // Encodes operand so it could be compared using a byte
  // comparison.
  short descFlag_; // if TRUE, encoding to be done for descending comparison.

  // encode so case insensitive comparisons could be done.
  // Valid for char/varchar datatypes only.
  NABoolean caseinsensitiveEncode_;

  CharInfo::Collation encodedCollation_;

  CollationInfo::CollationType collationType_;

  // length of encoded result, if passed in to the constructor.
  // If not passed in, or passed in and is -1, then the total
  // size of child is the length of CompEncode result.
  Lng32 length_;

  // if set, then result is nullable if child is nullable (same as other exprs and function).
  // Only the data bytes are encoded in this case.
  NABoolean regularNullability_;

  // Constructor used by derived class
  CompEncode(ItemExpr *val1Ptr, 
	     short descFlag,
	     Lng32 length,
	     CollationInfo::CollationType collType,
	     NABoolean regularNullability,
	     OperatorTypeEnum operType,
             NAMemory *h)
    : BuiltinFunction(operType, h, 1, val1Ptr),
    descFlag_(descFlag), 
    caseinsensitiveEncode_(FALSE),
    encodedCollation_(CharInfo::DefaultCollation),
    length_(length),
    collationType_(collType),
    regularNullability_(regularNullability)
      {}

public:
  CompEncode(ItemExpr *val1Ptr, 
	     short descFlag = FALSE,
	     Lng32 length = -1,
	     CollationInfo::CollationType collType = CollationInfo::Sort,
	     NABoolean regularNullability = FALSE,
             NAMemory *h = CmpCommon::statementHeap())
    : BuiltinFunction(ITM_COMP_ENCODE, h, 1, val1Ptr),
    descFlag_(descFlag), 
    caseinsensitiveEncode_(FALSE),
    encodedCollation_(CharInfo::DefaultCollation),
    length_(length),
    collationType_(collType),
    regularNullability_(regularNullability)
      {}

  // virtual destructor
  virtual ~CompEncode();

  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  virtual short codeGen(Generator*);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  void setCaseinsensitiveEncode(NABoolean v) {caseinsensitiveEncode_ = v;}

  void setEncodedCollation(CharInfo::Collation v) {encodedCollation_ = v;}

  void setCollationType(CollationInfo::CollationType v) { collationType_ = v;}

  short getDescFlag() const { return descFlag_;}

  NABoolean getCaseInsensitiveEncode() const { return caseinsensitiveEncode_;}

  Int32 getEncodedCollation() const { return encodedCollation_;}

  NABoolean getCollationType() const { return collationType_;}

  NABoolean isDecode() const { return getOperatorType() == ITM_COMP_DECODE; }

  static Lng32  getEncodedLength( const CharInfo::Collation collation,
						 const CollationInfo::CollationType ct,
						 const Lng32 srcLength,
						 const NABoolean nullable); 

  virtual QR::ExprElement getQRExprElem() const {return QR::QRFunctionWithParameters;}

}; // class CompEncode

class CompDecode : public CompEncode
{
public:
  CompDecode(ItemExpr *val1Ptr, 
             const NAType *unencodedType,
	     short descFlag = FALSE,
	     Lng32 length = -1,
	     CollationInfo::CollationType collType = CollationInfo::Sort,
	     NABoolean regularNullability = TRUE,
            NAMemory *h = CmpCommon::statementHeap())
    : CompEncode(val1Ptr, descFlag, length, collType, regularNullability, 
		 ITM_COMP_DECODE, h),
    unencodedType_(unencodedType)
    {}

  // virtual destructor
  virtual ~CompDecode();

  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  virtual short codeGen(Generator*);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

 private:
  const NAType * unencodedType_;
}; // class CompDecode

// -----------------------------------------------------------------------
// Built-in functions
// -----------------------------------------------------------------------
class HashCommon : public BuiltinFunction
{
  enum
  {
    CASESENSITIVE_HASH_ = 0x0001
  };

public:
  HashCommon(OperatorTypeEnum otype,
	     Lng32  argumentCount = 0,
	     ItemExpr *child0 = NULL,
	     ItemExpr *child1 = NULL,
	     ItemExpr *child2 = NULL,
	     ItemExpr *child3 = NULL,
	     ItemExpr *child4 = NULL,
	     ItemExpr *child5 = NULL)
       : BuiltinFunction(otype, CmpCommon::statementHeap(), argumentCount, 
			 child0, child1, child2, child3, child4, child5),
	 flags_(0)
  {
  }

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  void setCasesensitiveHash(NABoolean v)      
  { (v ? flags_ |= CASESENSITIVE_HASH_ : flags_ &= ~CASESENSITIVE_HASH_); }
  NABoolean casesensitiveHash() {return (flags_ & CASESENSITIVE_HASH_) != 0;}

protected:
  NABoolean areChildrenExactNumeric(Lng32 left, Lng32 right);

private:
  UInt32 flags_;
};

class Hash : public HashCommon
{
  // Defined for all data types, returns a 32 bit non-nullable
  // hash value for the data item.
public:
  Hash(ItemExpr *val1Ptr)
         : HashCommon(ITM_HASH, 1, val1Ptr) {}

  // virtual destructor
  virtual ~Hash();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // generator methods
  virtual ItemExpr * preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "Hash"; };
}; // class Hash

class HashComb : public HashCommon
{
  // Defined for two operands of type UNSIGNED INTEGER
  // Returns a value of type UNSIGNED INTEGER
public:
  HashComb(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
    : HashCommon(ITM_HASHCOMB, 2, val1Ptr, val2Ptr) {}

  // virtual destructor
  virtual ~HashComb();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "HashComb"; };
}; // class HashComb

class HiveHashComb : public HashCommon
{
  // Defined for two operands of type UNSIGNED INTEGER
  // Returns a value of type UNSIGNED INTEGER
public:
  HiveHashComb(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
    : HashCommon(ITM_HIVE_HASHCOMB, 2, val1Ptr, val2Ptr) {}

  // virtual destructor
  virtual ~HiveHashComb() {};

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "HiveHashComb"; };
}; // class HiveHashComb


// The following two classes (HashDistPartHash and
// HashDistPartHashComb) are used by the HashDistPartitioningFunction.
// They are isolated from the other general purpose versions of these
// functions so that they won't be inadvertently altered.  Any changes
// these functions that results in a different runtime behavior will
// cause existing hash partitioned tables to be invalid.
//
class HashDistPartHash : public HashCommon
{
  // Hash Function used by Hash Partitioning. This function
  // cannot change once Hash Partitioning is released!
  // Defined for all data types, returns a 32 bit non-nullable
  // hash value for the data item.
public:
  HashDistPartHash(ItemExpr *val1Ptr)
         : HashCommon(ITM_HDPHASH, 1, val1Ptr) {}

  // virtual destructor
  virtual ~HashDistPartHash();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // generator methods
  virtual ItemExpr * preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "HashDistPartHash"; };
}; // class HashDistPartHash

class HashDistPartHashComb : public HashCommon
{
  // This function is used to combine two hash values to produce a new
  // hash value. Used by Hash Partitioning. This function cannot
  // change once Hash Partitioning is released!  Defined for all data
  // types, returns a 32 bit non-nullable hash value for the data
  // item.

  // Defined for two operands of type UNSIGNED INTEGER
  // Returns a value of type UNSIGNED INTEGER
public:
  HashDistPartHashComb(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
    : HashCommon(ITM_HDPHASHCOMB, 2, val1Ptr, val2Ptr) {}

  // virtual destructor
  virtual ~HashDistPartHashComb();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "HashDistPartHashComb"; };
}; // class HashDistPartHashComb

class HiveHash : public HashCommon
{
  // Defined for all data types, returns a 32 bit non-nullable
  // hash value for the data item.
public:
  HiveHash(ItemExpr *val1Ptr)
         : HashCommon(ITM_HIVE_HASH, 1, val1Ptr) {}

  // virtual destructor
  virtual ~HiveHash() {};

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // generator methods
  virtual ItemExpr * preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "HiveHash"; };
}; // class HiveHash


////////////////////////////////////////////////////////////////
// Various Math functions are represented by this operator.
// The operatorType() tells which math function it is.
////////////////////////////////////////////////////////////////
class MathFunc : public CacheableBuiltinFunction
{
public:
  MathFunc(OperatorTypeEnum oper,
	   ItemExpr *val1Ptr = NULL, ItemExpr *val2Ptr = NULL)
         : CacheableBuiltinFunction(oper, 2, val1Ptr, val2Ptr)
  { allowsSQLnullArg() = FALSE; }

  // get the degree of this node (it depends on the type of the operator)
  virtual Int32 getArity() const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do pre code generation
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}
  
  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

  NAType* findReturnTypeForFloorCeil(NABoolean nullable);
};

class Abs : public MathFunc
{
public:
  Abs(ItemExpr *val1Ptr)
    : MathFunc(ITM_ABS, val1Ptr)
    { allowsSQLnullArg() = FALSE; }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual ItemExpr * bindNode(BindWA *bindWA);

  // method to do pre code generation
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class Abs

////////////////////////////////////////////////////////////////
// Various bit functions are represented by this operator.
// The operatorType() tells which bit function it is.
////////////////////////////////////////////////////////////////
class BitOperFunc : public CacheableBuiltinFunction
{
public:
  BitOperFunc(OperatorTypeEnum oper,
	      ItemExpr *val1Ptr = NULL, ItemExpr *val2Ptr = NULL,
	      ItemExpr *val3Ptr = NULL)
       : CacheableBuiltinFunction(oper, 3, val1Ptr, val2Ptr, val3Ptr)
  { allowsSQLnullArg() = FALSE; }
  
  // get the degree of this node (it depends on the type of the operator)
  virtual Int32 getArity() const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // method to do pre code generation
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

};

class Modulus : public CacheableBuiltinFunction
{
  // Defined for two operands of exact numeric data types with scale 0.
  // Returns a value of the data type of the second operand.
public:
  Modulus(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
         : CacheableBuiltinFunction(ITM_MOD, 2, val1Ptr, val2Ptr)
  { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~Modulus();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do pre code generation
  virtual ItemExpr * preCodeGen(Generator*);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

}; // class Modulus

/////////////////////////////////////////////////////////////////////
// Returns a string composed of val1ptr repeated val2ptr times.
/////////////////////////////////////////////////////////////////////
class Repeat : public BuiltinFunction
{
public:
  Repeat(ItemExpr *val1Ptr, ItemExpr *val2Ptr, Int32 maxLength = -1)
       : BuiltinFunction(ITM_REPEAT, CmpCommon::statementHeap(),
                         2, val1Ptr, val2Ptr),
         maxLength_(maxLength),
         maxLengthWasExplicitlySet_(FALSE)
  { 
    allowsSQLnullArg() = FALSE; 
    if (maxLength > 0)
      maxLengthWasExplicitlySet_ = TRUE;
  }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * preCodeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  NABoolean isCharTypeMatchRulesRelaxable()
  { return child(0)->isCharTypeMatchRulesRelaxable(); } ;

  virtual NABoolean protectFromVEGs() { return TRUE; };

  const Int32 getMaxLength() { return maxLength_ ;} ;
  void setMaxLength(Int32 val) {maxLength_ = val;} ;

 private:

  // max length of Repeat expression. 
  // If not passed in during constrtuctor, then it is set only when Repeat
  // is used certain expansions of LPAD and RPAD. 
  // It is initialized to the value -1, which indicates that maxLength 
  // has not been computed or passed in.
  Int32 maxLength_;

  // if max length was specified in REPEAT function 
  // and passed in during constructor.
  NABoolean maxLengthWasExplicitlySet_;

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class Repeat


/////////////////////////////////////////////////////////////////////
// Replaces all occurances of string val2ptr in string val1Ptr with
// string val3Ptr.
/////////////////////////////////////////////////////////////////////
class Replace : public CacheableBuiltinFunction
{
public:
  Replace(ItemExpr *val1Ptr, ItemExpr *val2Ptr, ItemExpr *val3Ptr)
       : CacheableBuiltinFunction(ITM_REPLACE, 3, val1Ptr, val2Ptr, val3Ptr)
  { allowsSQLnullArg() = FALSE; }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  // The following method enables expressions such as
  // iso88591_column = replace(:ucs2_hv1,:ucs2_hv2, :ucs2_hv3)
  NABoolean isCharTypeMatchRulesRelaxable()
  { return child(0)->isCharTypeMatchRulesRelaxable() &&
           child(1)->isCharTypeMatchRulesRelaxable() &&
           child(2)->isCharTypeMatchRulesRelaxable();
  } ;

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean protectFromVEGs() { return TRUE; };

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class Replace


class ReplaceNull : public BuiltinFunction
{
  // Returns third argument if first argument is null,
  // Otherwise returns second argument.
  // Defined for all types.
public:
  ReplaceNull(ItemExpr *val1Ptr, ItemExpr *val2Ptr, ItemExpr *val3Ptr)
         : BuiltinFunction(ITM_REPLACE_NULL, CmpCommon::statementHeap(),
                           3, val1Ptr, val2Ptr, val3Ptr) {}

  // virtual destructor
  virtual ~ReplaceNull();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // generator methods
  virtual ItemExpr * preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class ReplaceNull

class HashDistrib : public HashCommon
{
  // Defined for two operands with scale 0.
  // The first operand is the a arbitrary integer value.
  // The second operand is the number of mapped values.
  // Returns a value of the data type of the second operand.
  // The result is a value in the range 0 - (<number of mapped values> - 1)
  // distributed in a way that is useful for Hash Partitioning.
  //
public:
  HashDistrib(OperatorTypeEnum otype, ItemExpr *val1Ptr, ItemExpr *val2Ptr)
    : HashCommon(otype, 2, val1Ptr, val2Ptr) {}

  // virtual destructor
  virtual ~HashDistrib();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // generator methods
  virtual ItemExpr * preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "HashDistrib"; };

}; // class HashDistrib;

class Hash2Distrib : public HashDistrib
{
  // Defined for two operands with scale 0.
  // The first operand is the a arbitrary integer value.
  // The second operand is the number of mapped values.
  // Returns a value of the data type of the second operand.
  // The result is a value in the range 0 - (<number of mapped values> - 1)
  // distributed in a way that is useful for Hash Partitioning.
  //
public:
  Hash2Distrib(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
    : HashDistrib(ITM_HASH2_DISTRIB, val1Ptr, val2Ptr) {}

  // virtual destructor
  virtual ~Hash2Distrib();

  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "Hash2Distrib"; };

}; // class Hash2Distrib;

class ProgDistrib : public HashDistrib
{
  // Defined for two operands with scale 0.
  // The first operand is the a arbitrary integer value.
  // The second operand is the number of mapped values.
  // Returns a value of the data type of the second operand.
  // The result is a value in the range 0 - (<number of mapped values> - 1)
  // distributed in a way that is useful for Hash Partitioning.
  //
public:
  ProgDistrib(ItemExpr *val1Ptr, ItemExpr *val2Ptr)
    : HashDistrib(ITM_PROGDISTRIB, val1Ptr, val2Ptr) {}

  // virtual destructor
  virtual ~ProgDistrib();

  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "ProgDistrib"; };

}; // class ProgDistrib;

class ProgDistribKey : public BuiltinFunction
{
public:
  ProgDistribKey(ItemExpr *value,
                 ItemExpr *offset,
                 ItemExpr *totalNumValues)
    : BuiltinFunction(ITM_PROGDISTRIBKEY, CmpCommon::statementHeap(),
                      3, value, offset, totalNumValues) {}

  // virtual destructor
  virtual ~ProgDistribKey();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // generator methods
  virtual ItemExpr * preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "ProgDistribKey"; };

}; // class ProgDistribKey;

class PAGroup : public BuiltinFunction
{
  // Defined for three operands with scale 0.  The first operand is
  // the an integer value representing the partition number (before
  // grouping).  The second operand is the number of groups and the
  // third is the number of partitions before grouping.  Returns a
  // value of the data type of the first operand.  The result is a
  // value in the range 0 - (<number of groups> - 1).
  //
public:
  PAGroup(ItemExpr *val1Ptr, ItemExpr *val2Ptr, ItemExpr *val3Ptr)
    : BuiltinFunction(ITM_PAGROUP, CmpCommon::statementHeap(),
                      3, val1Ptr, val2Ptr, val3Ptr) {}

  // virtual destructor
  virtual ~PAGroup();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // generator methods
  virtual ItemExpr * preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const { return "PAGroup"; };

}; // class PAGroup;

class InverseOrder : public BuiltinFunction
{
  // Defined for all data types that can be used in an ORDER BY clause.
  // This function returns a result that has the same data type as its
  // input and has the following property:
  //     if a > b then inverse(a) < inverse(b)
  // (for numeric data types this is similar to the unary "-" operator)
  // This function is not usually needed in the executor, it is mainly
  // for the optimizer to indicate descending order. For example, the
  // SQL syntax "order by t1.a desc" is internally represented as
  // "order by inverse(t1.a)".
public:
  InverseOrder(ItemExpr *val1Ptr)
         : BuiltinFunction(ITM_INVERSE, CmpCommon::statementHeap(),
                           1, val1Ptr) {}

  // virtual destructor
  virtual ~InverseOrder();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual ItemExpr * simplifyOrderExpr(OrderComparison *newOrder = NULL);

  virtual ItemExpr * removeInverseOrder();

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}


}; // class InverseOrder

// Defined for all data types.
// Increments the value that is its operand by one.
// It is used for transforming a > comparison in an index search key
// to a >= comparison.
// Increment(MaxValue for a datatype) causes an overflow.
class Increment : public BuiltinFunction
{
public:
  Increment(ItemExpr *val1Ptr)
         : BuiltinFunction(ITM_INCREMENT, CmpCommon::statementHeap(),
                           1, val1Ptr) {}

  // virtual destructor
  virtual ~Increment();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class Increment

// Defined for all data types.
// Decrements the value that is its operand by one.
// It is used for transforming a < comparison in an index search key
// to a <= comparison.
// Decrement(MinValue for a datatype) causes an underflow.
class Decrement : public BuiltinFunction
{
public:
  Decrement(ItemExpr *val1Ptr)
         : BuiltinFunction(ITM_DECREMENT, CmpCommon::statementHeap(),
                           1, val1Ptr) {}

  // virtual destructor
  virtual ~Decrement();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

}; // class Decrement

// A ternary version of the comparison operator from class BiRelat. The
// first two operands <op1> and <op2> are two values to be compared,
// the boolean third operand <op3>, together with the operator type,
// indicates the type of comparison:
//
//   Operator type          op3 is FALSE             op3 is TRUE
// -----------------   ----------------------  ---------------------
//  ITM_LESS_OR_LE           op1 <= op2               op1 < op2
// ITM_GREATER_OR_GE         op1 >= op2               op1 > op2
//
// ITM_LESS_OR_LE is simply a shortcut for
//            case when op3 then op1 < op2 else op1 <= op2 end
// and ITM_GREATER_OR_GE is a shortcut for
//            case when op3 then op1 > op2 else op1 >= op2 end
//
// The TriRelational operator is used for cases where it is not known
// at compile time whether a greater or less comparison includes or
// excludes the case that both values are equal. In all practical cases,
// this is used for key predicates and is translated into an appropriate
// flag for the file system. In the unlikely case that this has to be
// evaluated for a non-key predicate, it gets transformed into an
// IfThenElse executor predicate: if (op3) then op1 < op2 else op1 <= op2
// or if (op3) then op1 > op2 else op1 >= op2, respectively.
class TriRelational : public BuiltinFunction
{
public:
  TriRelational(OperatorTypeEnum optype,
		ItemExpr *val1Ptr,
		ItemExpr *val2Ptr,
		ItemExpr *val3Ptr);

  // virtual destructor
  virtual ~TriRelational();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual ItemExpr * preCodeGen(Generator*);

  // method to generate Mdam predicates
  short mdamPredGen(Generator * generator,
                    MdamPred ** head,
                    MdamPred ** tail,
                    MdamCodeGenHelper & mdamHelper,
                    ItemExpr * parent);
  inline NABoolean getSpecialNulls()             { return specialNulls_; }
  inline void setSpecialNulls(NABoolean flag)    { specialNulls_ = flag; }

private:

  // the next field, if set, indicates that 'nulls are values'.
  // That means that nulls are equal to other nulls, and they
  // sort higher than other values. Used for groupby, order
  // by, hashing, or any other operation where nulls are NOT
  // to be ignored.
  NABoolean specialNulls_;

}; // class TriRelational

class RangeLookup : public BuiltinFunction
{
public:
  RangeLookup(ItemExpr *val1Ptr,
	      const RangePartitioningFunction *partFunc);

  virtual ~RangeLookup();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

private:

  // private methods to avoid teaching the expression generator about
  // partitioning functions
  Lng32 splitKeysLen();
  void copySplitKeys(char *tgt, Lng32 tgtLen);
  Lng32 getNumOfPartitions();
  Lng32 getEncodedBoundaryKeyLength();

  // private data members

  const RangePartitioningFunction *partFunc_;
}; // class RangeLookup

class ScalarVariance : public CacheableBuiltinFunction
{
public:
  ScalarVariance(OperatorTypeEnum opType,  // ITM_VARIANCE or ITM_STDDEV
		 ItemExpr *SumOfValSquared,
		 ItemExpr *SumOfVal,
		 ItemExpr *CountOfVal)
    : CacheableBuiltinFunction(opType, 3, SumOfValSquared, SumOfVal, CountOfVal) {}

  // virtual destructor
  virtual ~ScalarVariance();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);
  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * preCodeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    switch(getOperatorType())
      {
      case ITM_VARIANCE:
	return "Scalar Variance";
      case ITM_STDDEV:
	return "Scalar Stddev";
      default:
	return "Unknown Scalar Variance Function";
      }
  };

  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class ScalarVariance

// Class RowsetArrayScan
// This built in function is used to extract an element of a Rowset array
// Both the Rowset array and the index value are provided at run time.
// Items of type ArrayIndex are attached to an UnpackRows node at compile
// time. When the generator finds an UnpackRows node, it creates an
// PhysUnPackRows, and when it finds an ArrayIndex item, it generates a
// ex_RowsetArrayScan node. At run time, each ExRowsetArrayScan node is
// evaluated and the corresponding array element is returned by the
// PhysUnPackRows node to its parent.
//
// opType == ITM_ROWSETARRAY_SCAN to access the elements of the array
//        == ITM_ROWSETARRAY_INDEX to retrieve the current index

class RowsetArrayScan : public BuiltinFunction
{
public:
  RowsetArrayScan(ItemExpr     *source,
                  ItemExpr     *index,
                  Lng32         maxNumElem,
                  Lng32         elemSize,
                  NABoolean    elemNullInd,
                  const NAType *elemType,
                  OperatorTypeEnum opType = ITM_ROWSETARRAY_SCAN)
    : BuiltinFunction(opType, CmpCommon::statementHeap(), 2, source, index),
    maxNumElem_(maxNumElem),
    elemSize_(elemSize),
    elemNullInd_(elemNullInd),
    elemType_(elemType)
    {
    };

  // virtual destructor
  virtual ~RowsetArrayScan();

  // a virtual function for type propagating the node
  //
  virtual const NAType * synthesizeType();

  // allow relaxation for rowset array scan (similary to a regular hostvar)
  NABoolean isCharTypeMatchRulesRelaxable();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  // RowsetArrayScan behaves as an Aggregate, which is NOT covered even
  // if its children are.
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
                              const GroupAttributes& newRelExprAnchorGA,
                              ValueIdSet& referencedInputs,
                              ValueIdSet& coveredSubExpr,
                              ValueIdSet& unCoveredExpr) const;

  // RowsetArrayScan is considered a leaf node for cover test.
  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;

  // Return the type of the array elements
  const NAType *getType() const { return elemType_; }

  // method to do code generation
  //
  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const
  {
    return "RowsetArrayScan";
  };

  // This method is used to figure out the type of the array elements in case
  // they were not specified. This may happen in a dynamic rowset query that
  // contains unspecified parameters, like for instance "insert into t values(?)"
  const NAType * pushDownType(NAType& desiredType,
                        enum NABuiltInTypeEnum defaultQualifier);

private:
  Lng32         maxNumElem_;      // Maximum number of elements
  Lng32         elemSize_;        // Element storage length in bytes
  NABoolean    elemNullInd_;     // Null Indicator ?
  const NAType *elemType_;       // The datatype of the array elements

};

class RowsetArrayInto : public BuiltinFunction
{
public:
  RowsetArrayInto(ItemExpr     *source,
                  ItemExpr     *numElemExpr,
                  Lng32         maxNumElem,
                  Lng32         elemSize,
                  NABoolean    elemNullInd,
                  const NAType *hostVarType,
                  OperatorTypeEnum opType = ITM_ROWSETARRAY_INTO)
    : BuiltinFunction(opType, CmpCommon::statementHeap(),
                      2, source, numElemExpr),
      maxNumElem_(maxNumElem),
      elemSize_(elemSize),
      elemNullInd_(elemNullInd),
      hostVarType_(hostVarType)
    {
    };

  // virtual destructor
  virtual ~RowsetArrayInto();

  // a virtual function for type propagating the node
  //
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  // RowsetArrayInto behaves as an Aggregate, which is NOT covered even
  // if its children are.
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
                              const GroupAttributes& newRelExprAnchorGA,
                              ValueIdSet& referencedInputs,
                              ValueIdSet& coveredSubExpr,
                              ValueIdSet& unCoveredExpr) const;

  // RowsetArrayInto is considered a leaf node for cover test.
  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;

  // Return the type of the array elements
  const NAType *getType() const { return hostVarType_; }

  // method to do code generation
  //
  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const
  {
    return "RowsetArrayInto";
  };

private:
  Lng32         maxNumElem_;      // Maximum number of elements
  Lng32         elemSize_;        // Element storage length in bytes
  NABoolean    elemNullInd_;     // Null Indicator ?
  const NAType *hostVarType_;    // The datatype of the array elements

};


// Class unPackCol ------------------------------------------------
// This built in function is used to 'unpack' the values in a packed
// row.
//
class UnPackCol : public BuiltinFunction
{
public:
  UnPackCol(ItemExpr *source,
	    ItemExpr *index,
	    Lng32 width,
	    Lng32 base,
	    NABoolean nullsPresent,
	    const NAType *type,
	    OperatorTypeEnum opType = ITM_UNPACKCOL)
    : BuiltinFunction(opType, CmpCommon::statementHeap(), 2, source, index),
      width_(width),
      base_(base),
      nullsPresent_(nullsPresent),
      type_(type)
  {
  };

  // virtual destructor
  virtual ~UnPackCol();

  // Return the type of the original unpacked column.
  //
  const NAType *getType() const { return type_; }

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
    return "UnPackCol";
  };

  // UnPackCol behaves like an Aggregate, which is NOT covered even if its
  // children are.
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
                              const GroupAttributes& newRelExprAnchorGA,
                              ValueIdSet& referencedInputs,
                              ValueIdSet& coveredSubExpr,
                              ValueIdSet& unCoveredExpr) const;

  // UnPackCol is considered a leaf node for cover test.
  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;

private:
  // The width of the packed column in bits.
  //
  Lng32 width_;

  // The base offset of the packed data in bytes.
  //
  Lng32 base_;

  // If TRUE, then a null bitmap is present in the packed row.
  //
  NABoolean nullsPresent_;

  // The datatype of the original unpacked column.
  //
  const NAType *type_;

}; // class UnPackCol

// Function to generate a random 32 bit unsigned integer.  Each
// instance of this function maintains a seed internally, which gets
// automatically set to a random value the first time the function is
// called.  In the future there may be a requirement to allow the
// caller to manage the seed, but for now this simpler approach of
// having the callee do it is sufficient.
// The future is here: adding support for user specified seed.
//
class RandomNum : public BuiltinFunction
{
public:
  RandomNum(ItemExpr * seed = NULL, NABoolean simpleRandom = FALSE) 
       : BuiltinFunction(ITM_RANDOMNUM, CmpCommon::statementHeap(), 1, seed),
	 simpleRandom_(simpleRandom)
         { allowsSQLnullArg() = FALSE; }

  virtual ~RandomNum();

  virtual Int32 getArity() const { return (child(0) ? 1 : 0); };

  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual ItemExpr * preCodeGen(Generator*);

  virtual short codeGen(Generator*);

  virtual RandomNum *castToRandomNum() { return this; }

private:
  NABoolean simpleRandom_;
}; // class RandomNum

// Function to right/left shift a 32 or 64 bit integer.
//
class Shift : public BuiltinFunction
{
public:
  Shift(OperatorTypeEnum otype, // ITM_SHIFT_RIGHT || ITM_SHIFT_LEFT
        ItemExpr *child0,
        ItemExpr *child1)
    : BuiltinFunction(otype, CmpCommon::statementHeap(), 2, child0, child1) {}

  virtual ~Shift();

  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    switch(getOperatorType())
      {
      case ITM_SHIFT_RIGHT:
	return "Right Shift";
      case ITM_SHIFT_LEFT:
	return "Left Shift";
      default:
	return "Unknown Shift Operation";
      }
  };

}; // class Shift

// InternalTimestamp is a version of CurrentTimestamp that is evaluated
// again on every call
class InternalTimestamp : public BuiltinFunction
{
public:
  InternalTimestamp(): BuiltinFunction(ITM_INTERNALTIMESTAMP) {}

  // virtual destructor
  virtual ~InternalTimestamp();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const
  { return TRUE; };

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);


}; // class InternalTimestamp

// Function to mask (clear, set) bits in a 32 or 64 bit integer.
//
class Mask : public BuiltinFunction
{
public:
  Mask(OperatorTypeEnum otype,
       ItemExpr *child0,
       ItemExpr *child1)
    : BuiltinFunction(otype, CmpCommon::statementHeap(), 2, child0, child1) {}

  virtual ~Mask();

  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    switch(getOperatorType())
      {
      case ITM_MASK_SET:
	return "Mask Set";
      case ITM_MASK_CLEAR:
	return "Mask Clear";
      default:
	return "Unknown Mask Operation";
      }
  };

}; // class Mask

// -----------------------------------------------------------------------
// PackFunc is a built-in function to support packing (of multiple rows
// into one row). It takes two parameters. One is an ItemExpr specifying
// the row to be packed. The other is the packing factor which says how
// many rows are to be packed into one row.
// -----------------------------------------------------------------------
class PackFunc : public BuiltinFunction
{
  // ITM_PACK_FUNC
public:

  // Constructor.
  PackFunc(ItemExpr* val1Ptr, ItemExpr* pf)
   : BuiltinFunction(ITM_PACK_FUNC,CmpCommon::statementHeap(),2,val1Ptr,pf),
     isFormatInfoValid_(FALSE)                                          {}

  // This constructor is used when format is predetermined.
  PackFunc(ItemExpr* val1Ptr,
           ItemExpr* pf,
           Lng32 base,
           Lng32 width,
           NABoolean nullsPresent);

  // The type given is the type of one logical row before packing is done.
  PackFunc(ItemExpr* val1Ptr,
           ItemExpr* pf,
           const NAType* unpackType);

  // Destructor.
  virtual ~PackFunc();

  // Allow setting and clearing of format information after construction.
  void setFormatInfo(Lng32 base,
                     Lng32 width,
                     NABoolean nullsPresent,
                     const NAType* type)
  {
    base_ = base;
    width_ = width;
    nullsPresent_ = nullsPresent,
    type_ = type;
    isFormatInfoValid_ = TRUE;
  }

  void clearFormatInfo()                   { isFormatInfoValid_ = FALSE; }

  // Derive type from base, width and nullsPresent information.
  void deriveTypeFromFormatInfo();

  // Derive type, base, width and nullsPresent information from unpack type.
  void deriveFormatInfoFromUnpackType(const NAType* unpackType);

  // Needn't refine bindNode(). Default suffice. Refine synthesizeType().
  virtual const NAType* synthesizeType();

  // Transform node eliminates subqueries from the expression trees.
  // virtual void transformNode(NormWA& normWARef,
  //                         ExprValueId& locationOfPointerToMe,
  //                         ExprGroupId& introduceSemiJoinHere,
  //                         const ValueIdSet& externalInputs);

  // Normalize node rewrites some sub-expressions into VEGRef.
  // virtual ItemExpr* normalizeNode(NormWA& normWARef);

  // PackFunc behaves like an Aggregate, which is NOT covered even if its
  // children are.
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;
  // Code Generation.
  virtual short codeGen(Generator*);

  // Make a copy of the operator.
  virtual ItemExpr* copyTopNode(ItemExpr* derivedNode = NULL,
                                 CollHeap* outHeap = 0);
private:

  // Whether the format information below are valid or not. If not, PackFunc
  // will compute these information based on its first operand.
  NABoolean isFormatInfoValid_;

  // Offset of where packed data begin.
  Lng32 base_;

  // Width of one logical row. Negative means bits; Positive means bytes.
  Lng32 width_;

  // Whether the null bit map is going to be present in the packed record.
  NABoolean nullsPresent_;

  // Type of logical row stored.
  const NAType* type_;

}; // class PackFunc

///////////////////////////////////////////////////////////////////
// This function only exists upto the bindNode phase.
// The bindNode method returns a new tree depending on
// the oper. This new tree uses the existing operators to
// perform this function. Some of this kind of transformation
// could and is done in the parser. But for complicated expressions
// it is easier to use Parser::parseDML method as an internal
// expression by passing it a string instead of creating the
// complicated tree in the parser. The parseDML could not be called
// from the parser since it in not a reentrant code (parser uses
// globals).
///////////////////////////////////////////////////////////////////
class ZZZBinderFunction : public BuiltinFunction
{
public:
  ZZZBinderFunction(OperatorTypeEnum oper,
		    ItemExpr *val1Ptr = NULL, ItemExpr *val2Ptr = NULL,
		    ItemExpr *val3Ptr = NULL, ItemExpr *val4Ptr = NULL,
		    ItemExpr *val5Ptr = NULL, ItemExpr *val6Ptr = NULL)
       : BuiltinFunction(oper, CmpCommon::statementHeap(), 6,
			 val1Ptr, val2Ptr, val3Ptr, val4Ptr, val5Ptr, val6Ptr),
         flags_(0)
  {}

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);
  
  // helper function used by bindNode; returns true if there is an error
  bool enforceDateOrTimestampDatatype(BindWA *bindWA, CollIndex child, int operand);

  // the synthesizeType method is needed only when we process an item
  // expression at DDL time, for DML the function gets transformed into
  // another function before we reach type synthesis
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  // No transformation for this node. Raise an error, if transformation
  // is reached. This is the next step after bindNode.
  virtual void transformNode(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInputs);
  
  Int32 getPadLength (ItemExpr* padLengthExpr, BindWA* bindWA);
  Int32 getPadStringLength (ItemExpr* padLengthExpr, BindWA* bindWA);
  NABoolean isPadWithSpace (ExprValueId& padExpr, CharInfo::CharSet cs);

  static ItemExpr *tryToUndoBindTransformation(ItemExpr *expr);

  // OVERLAY clause was created for STUFF syntax.
  NABoolean overlayFuncWasStuff()   { return (flags_ & WAS_STUFF_) != 0; }
  void setOverlayFuncWasStuff(NABoolean v)
  { (v ? flags_ |= WAS_STUFF_ : flags_ &= ~WAS_STUFF_); }

private:
  enum {
    WAS_STUFF_ = 0x0001
  };

  Int64 flags_;
};

// --------------------------------------------------------------------------
//
// Sequence functions. These perform running and moving aggregates
// based on a defined sort order
//
// --------------------------------------------------------------------------
// Sequence function base class
//
// Allows for up to 3 function arguments.
// --------------------------------------------------------------------------
//
class ItmSequenceFunction : public BuiltinFunction
{
public:
  ItmSequenceFunction(OperatorTypeEnum itemType, ItemExpr *val1Ptr, ItemExpr *val2Ptr=NULL, ItemExpr *val3Ptr=NULL)
    : BuiltinFunction(itemType, CmpCommon::statementHeap(),
                      3, val1Ptr, val2Ptr, val3Ptr),
      isOLAP_(FALSE),
      olapPartitionBy_(NULL),
      olapOrderBy_(NULL),
      isTDFunction_(FALSE)
      {  }

  // virtual destructor
  virtual ~ItmSequenceFunction();

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  // method to do precode generation
  // virtual ItemExpr * preCodeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  virtual  NABoolean isASequenceFunction() const ;  // virtual method
  virtual NABoolean containsTHISFunction();

  //  Sequence functions behave like an Aggregate, which is NOT covered even if its
  // children are.
  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
                              const GroupAttributes& newRelExprAnchorGA,
                              ValueIdSet& referencedInputs,
                              ValueIdSet& coveredSubExpr,
                              ValueIdSet& unCoveredExpr) const;

  // A sequence function is considered a leaf node for cover test.
  virtual void getLeafValuesForCoverTest(ValueIdSet & leafValues, 
                                         const GroupAttributes& coveringGA,
                                         const ValueIdSet & newExternalInputs) const;

  virtual NABoolean calculateMinMaxUecs(ColStatDescList & histograms,
					       CostScalar & minUec,
					       CostScalar & maxUec);

  NABoolean isOLAP() { return isOLAP_; };
  void setIsOLAP( NABoolean v ) { isOLAP_ = v; }

  void setOlapPartitionBy(ItemExpr *partBy) { olapPartitionBy_ = partBy;};

  ItemExpr *getOlapPartitionBy() { return olapPartitionBy_; };
  ItemExpr *removeOlapPartitionBy()
  {
    ItemExpr *partBy = olapPartitionBy_;
    olapPartitionBy_ = NULL;
    return partBy;
  };

  void setOlapOrderBy(ItemExpr *orderBy) { olapOrderBy_ = orderBy;};

  ItemExpr *getOlapOrderBy() { return olapOrderBy_; };
  ItemExpr *removeOlapOrderBy()
  {
    ItemExpr *orderBy = olapOrderBy_;
    olapOrderBy_ = NULL;
    return orderBy;
  };

  void setOLAPInfo(ItemExpr *partBy, 
                   ItemExpr *orderBy,                   
                   NABoolean isTDFunction=FALSE)
  {
    olapPartitionBy_ = partBy;
    olapOrderBy_ = orderBy;

    // default is that the isOLAP flag is ON here unless
    // its a TD function, then its OFF
    isOLAP_ = (isTDFunction) ? FALSE : TRUE;

    isTDFunction_ = isTDFunction;    
  };

  NABoolean isTDFunction() { return isTDFunction_; };
  void setIsTDFunction( NABoolean v ) { isTDFunction_ = v; }

  ItemExpr * transformTDFunction(BindWA * bindWA);
  NABoolean isEquivalentForBinding(const ItemExpr * other);

  virtual QR::ExprElement getQRExprElem() const 
  {
    return QR::QRFunctionWithParameters;
  }

private:
  // OLAP Info
  NABoolean isOLAP_;
  ItemExpr *olapPartitionBy_;
  ItemExpr *olapOrderBy_;
  NABoolean isTDFunction_;
} ;

//------------------------------------------------------------------------
// Running function class.
//
//------------------------------------------------------------------------
//
class ItmSeqRunningFunction : public ItmSequenceFunction
{
public:
  ItmSeqRunningFunction(OperatorTypeEnum itemType, ItemExpr *valPtr)
         : ItmSequenceFunction(itemType, valPtr)
         {  }

  // virtual destructor
  virtual ~ItmSeqRunningFunction();

  // methods for code generation
  virtual ItemExpr *preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  // method to do precode generation
  // virtual ItemExpr * preCodeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

 void transformNode(NormWA & normWARef,
               ExprValueId & locationOfPointerToMe,
               ExprGroupId & introduceSemiJoinHere,
               const ValueIdSet & externalInputs);


  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

 ItemExpr *transformRunningVariance();
 ItemExpr *transformRunningSDev();
 ItemExpr *transformRunningAvg();
 ItemExpr *transformRunningRank();

 virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}
  
} ;
//------------------------------------------------------------------------
// Olap function class.
//
//------------------------------------------------------------------------
//
class ItmSeqOlapFunction : public ItmSequenceFunction
{
public:
  ItmSeqOlapFunction(OperatorTypeEnum itemType, ItemExpr *val1Ptr, ItemExpr *val2Ptr = NULL, ItemExpr *val3Ptr = NULL): 
        ItmSequenceFunction(itemType, val1Ptr, val2Ptr, val3Ptr),
        frameStart_(0),
        frameEnd_(0)
        {  }

  // virtual destructor
  virtual ~ItmSeqOlapFunction();

  // methods for code generation
  virtual ItemExpr *preCodeGen(Generator*);  //transfomr into running seq functions
  //virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);
  OperatorTypeEnum mapOperTypeToRunning() const;
  OperatorTypeEnum mapOperTypeToMoving() const;

  void transformNode(NormWA & normWARef,
               ExprValueId & locationOfPointerToMe,
               ExprGroupId & introduceSemiJoinHere,
               const ValueIdSet & externalInputs);

  ItemExpr *transformOlapVariance(CollHeap *heap);
  ItemExpr *transformOlapSDev(CollHeap *heap);
  ItemExpr *transformOlapAvg(CollHeap *heap);
  ItemExpr *transformOlapRank(CollHeap *heap);
  ItemExpr *transformOlapDRank(CollHeap *heap);
  virtual ItemExpr *transformOlapFunction(CollHeap *heap);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();


  Lng32 getframeStart()
  {
    return frameStart_;
  }

  Lng32 getframeEnd()
  {
    return frameEnd_;
  }

  void setframeStart(Lng32 v)
  {
    frameStart_=v;
  }

  void setframeEnd(Lng32 v)
  {
    frameEnd_=v;
  }

  void setOlapWindowFrame(Lng32 s, Lng32 e)
  {
    frameStart_ = s;
    frameEnd_ = e;
  }

  NABoolean isFrameStartUnboundedPreceding() const
  {
    return (frameStart_ == -INT_MAX);
  }

   NABoolean isFrameStartUnboundedFollowing() const
  {
    return (frameStart_ == INT_MAX);
  }
  NABoolean isFrameEndUnboundedPreceding() const
  {
    return (frameEnd_ == -INT_MAX);
  }

  NABoolean isFrameEndUnboundedFollowing() const
  {
    return (frameEnd_ == INT_MAX);
  }


  virtual  NABoolean isOlapFunction() const ;  // virtual method
  inline NABoolean
  canInverseOLAPOrder()
  {
    return !(frameStart_ == -INT_MAX &&
              frameEnd_ != INT_MAX &&
              (getOperatorType() == ITM_OLAP_MIN || 
               getOperatorType() == ITM_OLAP_MAX) ||
             getOperatorType() == ITM_OLAP_RANK ||
             getOperatorType() == ITM_OLAP_DRANK);

  }

  inline NABoolean
  mustInverseOLAPOrder()
  {
    return (frameStart_ != -INT_MAX &&
            frameEnd_ == INT_MAX &&
            (getOperatorType() == ITM_OLAP_MIN || getOperatorType() == ITM_OLAP_MAX));
  }

  NABoolean
  inverseOLAPOrder(CollHeap *heap);

private:
  //olap window frame -- start and end-- 
  // will add patition and order by here too later and remove them from ItemSequenceFunction
  //
  Lng32 frameStart_;
  Lng32 frameEnd_;
} ;

class ItmLeadOlapFunction: public ItmSeqOlapFunction 
{
public:
  ItmLeadOlapFunction(ItemExpr *valPtr, ItemExpr* offsetExpr = NULL, ItemExpr* defaultValue = NULL): 
        ItmSeqOlapFunction(ITM_OLAP_LEAD, valPtr, offsetExpr, defaultValue),
        offset_(-1)
        { }

  ItmLeadOlapFunction(ItemExpr *valPtr, Int32 offset): 
        ItmSeqOlapFunction(ITM_OLAP_LEAD, valPtr),
        offset_(offset)
        { }

  // virtual destructor
  virtual ~ItmLeadOlapFunction();

  // methods for code generation
  virtual ItemExpr *preCodeGen(Generator*);  //transfomr into running seq functions
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual  NABoolean isOlapFunction() const { return TRUE; } ;  // virtual method

  Int32  getOffset() { return offset_; };
  void setOffset(Int32 x) { offset_ = x; };

  ItemExpr* transformOlapFunction(CollHeap *wHeap);

  void transformNode(NormWA & normWARef, 
                     ExprValueId & locationOfPointerToMe,
                     ExprGroupId & introduceSemiJoinHere, 
                     const ValueIdSet & externalInputs);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
    { return "LEAD"; };

private:

   Int32 offset_;
} ;

// ------------------------------------------------------------------------
//
//  OFFSET sequence function.
//
//  OFFSET(x, y) returns SELECT list item x in row at offset (+ or -)
//  y in the sorted sequence.
//
//  Similar to the cursor FETCH RELATIVE operation, except returns
//  result in context of the current row.
// ------------------------------------------------------------------------
//
class ItmSeqOffset : public ItmSequenceFunction
{
public:
  ItmSeqOffset(ItemExpr *val1Ptr, ItemExpr *val2Ptr, ItemExpr *val3Ptr = NULL,
               NABoolean nullRowIsZero = FALSE, NABoolean leading = TRUE, Lng32 winSize = 0)
         : ItmSequenceFunction(ITM_OFFSET, val1Ptr, val2Ptr, val3Ptr),
           nullRowIsZero_(nullRowIsZero),
           offsetConstantValue_ (0),
           leading_(leading),
           winSize_(winSize)
  { allowsSQLnullArg() = FALSE; }

  ItmSeqOffset(ItemExpr *valPtr, 
               Int32 offsetConstantValue,
               NABoolean nullRowIsZero = FALSE,
               NABoolean leading = TRUE,
               Lng32 winSize = 0)
         : ItmSequenceFunction(ITM_OFFSET, valPtr),
           nullRowIsZero_(nullRowIsZero),
           offsetConstantValue_ (offsetConstantValue),
           leading_(leading),
           winSize_(winSize)
  { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~ItmSeqOffset();

  // methods to do code generation
  virtual ItemExpr *preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();


  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  void transformNode(NormWA & normWARef,
                    ExprValueId & locationOfPointerToMe,
                    ExprGroupId & introduceSemiJoinHere,
                    const ValueIdSet & externalInputs);

  // get the degree of this node. Although it is a binary op, this will return 1 if offset value was
  // set via constant below.

  virtual Int32 getArity() const;

  NABoolean nullRowIsZero() const {return nullRowIsZero_;}

  void setNullRowIsZero(NABoolean b) {nullRowIsZero_ = b;}

  //
  Int32 getOffsetConstantValue () const { return  offsetConstantValue_; }

  NABoolean isLeading() const {return leading_;}
  Lng32 winSize() const {return winSize_;}

  virtual NABoolean hasEquivalentProperties(ItemExpr * other);
  
private:
  NABoolean nullRowIsZero_;
  Int32 offsetConstantValue_;
  NABoolean leading_;
  Lng32 winSize_;

}; // class ItmSeqOffset

class ItmLagOlapFunction: public ItmSeqOlapFunction 
{
public:
  ItmLagOlapFunction(ItemExpr *seqColumn, ItemExpr *offsetExpr, ItemExpr* defaultValue = NULL)
         : ItmSeqOlapFunction(ITM_OLAP_LAG, seqColumn, offsetExpr, defaultValue)
         {}

  // virtual destructor
  virtual ~ItmLagOlapFunction(){}
  // methods to do code generation
  virtual ItemExpr *preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();
  
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL, CollHeap* outHeap = 0);
  
  void transformNode(NormWA & normWARef,
                ExprValueId & locationOfPointerToMe,
                ExprGroupId & introduceSemiJoinHere,
                const ValueIdSet & externalInputs);
	
  virtual  NABoolean isOlapFunction() const { return TRUE; }

  ItemExpr * transformOlapFunction(CollHeap *heap) { return this; }
  // get a printable string that identifies the operator
  virtual const NAString getText() const    { return "LAG"; };
};


// --------------------------------------------------------------------------
//
//  DIFF1 sequence function.
//
//  This function has both 1-parameter and two-parameter forms:
//  DIFF1(x) is equivalent to:  x-OFFSET(x, 1)
//  DIFF1(x, y) is equivalent to:  (x-OFFSET(x, 1)) / (y-OFFSET (y, 1)
//
// --------------------------------------------------------------------------
//
class ItmSeqDiff1 : public ItmSequenceFunction
{
public:
  ItmSeqDiff1(ItemExpr *valPtr1, ItemExpr *valPtr2 = NULL)
         : ItmSequenceFunction(ITM_DIFF1, valPtr1, valPtr2)
  { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~ItmSeqDiff1();

  // method to do code generation
  virtual short codeGen(Generator*);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  ItemExpr *transformDiff1();

 void transformNode(NormWA & normWARef,
                    ExprValueId & locationOfPointerToMe,
                    ExprGroupId & introduceSemiJoinHere,
                    const ValueIdSet & externalInputs);


private:

}; // class ItmSeqDiff1

// --------------------------------------------------------------------------
//
//  DIFF2 sequence function.
//
//  This function has both 1-parameter and two-parameter forms:
//  Diff2(x) is equivalent to:  DIFF1(DIFF1(x))
//  Diff2(x, y) is equivalent to:  DIFF1(DIFF1(x))/ DIFF1(DIFF1(y))
// --------------------------------------------------------------------------
//
class ItmSeqDiff2 : public ItmSequenceFunction
{
public:
  ItmSeqDiff2(ItemExpr *valPtr1, ItemExpr *valPtr2 = NULL)
         : ItmSequenceFunction(ITM_DIFF2, valPtr1, valPtr2)
  { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~ItmSeqDiff2();

  // method to do code generation
  virtual short codeGen(Generator*);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

 ItemExpr *transformDiff2();

 void transformNode(NormWA & normWARef,
               ExprValueId & locationOfPointerToMe,
               ExprGroupId & introduceSemiJoinHere,
               const ValueIdSet & externalInputs);


private:

}; // class ItmSeqDiff2

// --------------------------------------------------------------------------
//
//  ROWS SINCE sequence function.
//
//  This function has two forms:
//  ROWS SINCE <condition>
//  ROWS SINCE <condition> INCLUSIVE
//
//  The <condition> is similar to CASE WHEN <condition>
// --------------------------------------------------------------------------
//
class ItmSeqRowsSince : public ItmSequenceFunction
{
public:
  ItmSeqRowsSince(ItemExpr *valPtr, ItemExpr *valPtr2 = NULL, NABoolean inclusionSpec = FALSE)
         : ItmSequenceFunction(ITM_ROWS_SINCE, valPtr, valPtr2),
         includeCurrentRow_ (inclusionSpec)
  { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~ItmSeqRowsSince();

  // methods for code generation
  virtual ItemExpr *preCodeGen(Generator*);
  virtual short codeGen(Generator*);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual void transformNode(NormWA & normWARef,
                    ExprValueId & locationOfPointerToMe,
                    ExprGroupId & introduceSemiJoinHere,
                    const ValueIdSet & externalInputs);

  NABoolean includeCurrentRow() const { return includeCurrentRow_; }

private:

  NABoolean includeCurrentRow_;

}; // class ItmSeqRowsSince

//------------------------------------------------------------------------
// Moving function class.
//
//------------------------------------------------------------------------
//
class ItmSeqMovingFunction : public ItmSequenceFunction
{
public:
  ItmSeqMovingFunction (OperatorTypeEnum itemType,
                        ItemExpr *valPtr1, ItemExpr *valPtr2, ItemExpr *valPtr3 = NULL)
         : ItmSequenceFunction(itemType, valPtr1, valPtr2, valPtr3),
         skipMovingMinMaxTransformation_(FALSE)
  { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~ItmSeqMovingFunction();

  // method to do code generation
  virtual short codeGen(Generator*);

  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

 void transformNode(NormWA & normWARef,
               ExprValueId & locationOfPointerToMe,
               ExprGroupId & introduceSemiJoinHere,
               const ValueIdSet & externalInputs);


  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();
  virtual NABoolean hasEquivalentProperties(ItemExpr * other);

private:                               // the following are invoked only from within transformNode()

 ItemExpr *transformMovingVariance();
 ItemExpr *transformMovingSDev();
 ItemExpr *transformMovingAvg();
 ItemExpr *transformMovingSum();
 ItemExpr *transformMovingCount();
 ItemExpr *transformMovingMinMax();
 ItemExpr *transformMovingRank();

  // the following are used only in transformNode() for MovingMin/Max
  NABoolean skipMovingMinMaxTransformation_;
  NABoolean getSkipMovingMinMaxTransformation() { return skipMovingMinMaxTransformation_; }
  void setSkipMovingMinMaxTransformation() { skipMovingMinMaxTransformation_ = TRUE; }

} ;  // class ItmSeqMovingFunction

//------------------------------------------------------------------------
// Function class for sequence function THIS().
//
//------------------------------------------------------------------------
//
class ItmSeqThisFunction : public ItmSequenceFunction
{
public:
  ItmSeqThisFunction(ItemExpr *valPtr)
         : ItmSequenceFunction(ITM_THIS, valPtr)
         {  }
  // virtual destructor

  virtual ~ItmSeqThisFunction();

  // method to do code generation
  virtual short codeGen(Generator*);

  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();
} ;     // class ItmSeqThisFunction

//------------------------------------------------------------------------
// Function class for ItmScalarMinMax.
//
// This node represends the traditional min/max functions of the form:
//
// SCALAR_MIN(A,B) : CASE WHEN (A < B) THEN A ELSE B
// SCALAR_MAX(A,B) : CASE WHEN (A > B) THEN A ELSE B
//------------------------------------------------------------------------
//
class ItmScalarMinMax : public BuiltinFunction
{
public:
  ItmScalarMinMax (OperatorTypeEnum opType, ItemExpr *val1Ptr, ItemExpr *val2Ptr)
    : BuiltinFunction(opType, CmpCommon::statementHeap(), 2, val1Ptr, val2Ptr)
    { allowsSQLnullArg() = FALSE; }

  // virtual destructor
  virtual ~ItmScalarMinMax();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // A transformation method for protecting sequence functions from not
  // being evaluated due to short-circuit evaluation.
  //
  virtual void protectiveSequenceFunctionTransformation(Generator *generator);

  // method to do code generation
  virtual short codeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

}; // class ItmScalarMinMax

//------------------------------------------------------------------------
// Function class for sequence function NotTHIS().
// Merely a No-op wrapper used during tranformation
//  to indicate that no THIS function occurs
// below this point.
//------------------------------------------------------------------------
//
class ItmSeqNotTHISFunction : public ItmSequenceFunction
{
public:
  ItmSeqNotTHISFunction(ItemExpr *valPtr)
         : ItmSequenceFunction(ITM_NOT_THIS, valPtr)
         {  }
  // virtual destructor

  virtual ~ItmSeqNotTHISFunction();

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

  // method to do precode generation
  virtual ItemExpr * preCodeGen(Generator*);

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

} ;     // class ItmSeqNotTHISFunction

// lookup a column in a native hbase table that is being accessed in row format
class HbaseColumnLookup : public BuiltinFunction
{
public:
  HbaseColumnLookup(
       ItemExpr *child0,
       NAString &hbaseCol,
       const NAType * naType = NULL)
    : BuiltinFunction(ITM_HBASE_COLUMN_LOOKUP, 
		      CmpCommon::statementHeap(), 1, child0),
    hbaseCol_(hbaseCol),
    naType_(naType)
    {}

  virtual ~HbaseColumnLookup();

  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  ItemExpr *bindNode(BindWA *bindWA);

  virtual short codeGen(Generator*);

  const NAType * naType() { return naType_; }
  NAString &hbaseCol() { return hbaseCol_; }

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    return "HBASE_COLUMN_LOOKUP";
  };

 private:
  NAString hbaseCol_;
  const NAType *naType_;
}; // class HbaseColumnLookup

// format hbase columns that are being accessed to be displayed
class HbaseColumnsDisplay : public BuiltinFunction
{
public:
  HbaseColumnsDisplay(
		      ItemExpr *child0,
		      ConstStringList * csl = NULL,
		      Lng32 displayWidth = 100000)
    : BuiltinFunction(ITM_HBASE_COLUMNS_DISPLAY, 
		      CmpCommon::statementHeap(), 1, child0),
    csl_(csl),
    displayWidth_(displayWidth)
    {}

  virtual ~HbaseColumnsDisplay();

  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  ItemExpr *bindNode(BindWA *bindWA);

  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    return "HBASE_COLUMNS_DISPLAY";
  };

  ConstStringList* &csl() { return csl_; }

 private:
 ConstStringList * csl_;

  Lng32 displayWidth_;
}; // class HbaseColumnsDisplay

// create a column to update/insert into an hbase native table
class HbaseColumnCreate : public BuiltinFunction
{
public:
  class HbaseColumnCreateOptions
  {
    friend class HbaseColumnCreate;
  public:
    enum ConvType
    {
      CAST_AS,
      TYPE_AS,
      NONE
    };
    
    HbaseColumnCreateOptions
      (ItemExpr * colName,
       ItemExpr * colVal,
       const NAType * naType,
       ConvType convType) 
      : colName_(colName),
      colVal_(colVal),
      naType_(naType),
      convType_(convType)
      {}

    const NAType * naType() { return naType_;}
    ConvType convType() { return convType_;}
    ItemExpr * colName() { return colName_; }
    ItemExpr * colVal() { return colVal_; }
    void setColName(ItemExpr * c) { colName_ = c; }
    void setColVal(ItemExpr * c) { colVal_ = c; }
    ItemExpr * colNameIE() { return colNameIE_; }
    void setColNameIE(ItemExpr * cn) { colNameIE_ = cn; }

  private:
    ItemExpr * colNameIE_;
    ItemExpr * colName_;
    ItemExpr * colVal_;
    const NAType * naType_;
    ConvType convType_;

  };

  HbaseColumnCreate(
		    NAList<HbaseColumnCreateOptions*> * hccol)		    
    : BuiltinFunction(ITM_HBASE_COLUMN_CREATE, 
		      CmpCommon::statementHeap(), 0, NULL),
    resultType_(NULL),
    hccol_(hccol),
    colNameMaxLen_(0),
    colValMaxLen_(0)
    {}

  virtual ~HbaseColumnCreate();

  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr * bindNode(BindWA *bindWA);

  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    return "HBASE_COLUMN_CREATE";
  };

  NABoolean createListSpecified() { return (hccol_ != NULL); };

 private:
  NAString hbaseCol_;
  const NAType *resultType_;

  short colNameMaxLen_;
  Int32 colValMaxLen_;

  NAList<HbaseColumnCreateOptions*> * hccol_;
}; // class HbaseColumnCreate

class HbaseTimestamp : public BuiltinFunction
{
public:
 HbaseTimestamp(ItemExpr * col)
   : BuiltinFunction(ITM_HBASE_TIMESTAMP,
                     CmpCommon::statementHeap(), 1, NULL),
    col_(col),
    tsVals_(NULL),
    colIndex_(-1)
    {}
  
  virtual ~HbaseTimestamp();
  
  virtual NABoolean isCovered
    (const ValueIdSet& newExternalInputs,
     const GroupAttributes& coveringGA,
     ValueIdSet& referencedInputs,
     ValueIdSet& coveredSubExpr,
     ValueIdSet& unCoveredExpr) const;

  virtual Int32 getArity() const { return (child(0) ? 1 : 0); }

  virtual NABoolean isCacheableExpr(CacheWA& cwa)
    { return TRUE; }

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  virtual const NAType * synthesizeType();
  
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  ItemExpr *bindNode(BindWA *bindWA);

  virtual ItemExpr * preCodeGen(Generator * generator);

  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    return "HBASE_TIMESTAMP";
  };
  
  void setColIndex(Lng32 idx) { colIndex_ = idx;}
  Lng32 getColIndex() { return colIndex_; }

  const ItemExpr * col() const { return col_; }
  const ItemExpr * tsVals() { return tsVals_; }
 private:
  ItemExpr * col_;
  ItemExpr * tsVals_;

  Lng32 colIndex_;

  NAString colName_;
}; // class HbaseTimestamp

class HbaseTimestampRef : public BuiltinFunction
{
public:
 HbaseTimestampRef(ItemExpr * col)
   : BuiltinFunction(ITM_HBASE_TIMESTAMP_REF,
                     CmpCommon::statementHeap()),
    col_(col)
    {}
  
  virtual ~HbaseTimestampRef();
  
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  ItemExpr *bindNode(BindWA *bindWA);

  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    return "HBASE_TIMESTAMP_REF";
  };
  
 private:
  ItemExpr * col_;
}; // class HbaseTimestampRef

class HbaseVersion : public BuiltinFunction
{
public:
 HbaseVersion(ItemExpr * col)
   : BuiltinFunction(ITM_HBASE_VERSION,
                     CmpCommon::statementHeap(), 1, NULL),
    col_(col),
    tsVals_(NULL),
    colIndex_(-1)
    {}
  
  virtual ~HbaseVersion();
  
  virtual NABoolean isCovered
    (const ValueIdSet& newExternalInputs,
     const GroupAttributes& coveringGA,
     ValueIdSet& referencedInputs,
     ValueIdSet& coveredSubExpr,
     ValueIdSet& unCoveredExpr) const;

  virtual Int32 getArity() const { return (child(0) ? 1 : 0); }

  virtual NABoolean isCacheableExpr(CacheWA& cwa)
    { return TRUE; }

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  virtual const NAType * synthesizeType();
  
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  ItemExpr *bindNode(BindWA *bindWA);

  virtual ItemExpr * preCodeGen(Generator * generator);

  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    return "HBASE_VERSION";
  };
  
  void setColIndex(Lng32 idx) { colIndex_ = idx;}
  Lng32 getColIndex() { return colIndex_; }

  const ItemExpr * col() const { return col_; }
  const ItemExpr * tsVals() { return tsVals_; }
 private:
  ItemExpr * col_;
  ItemExpr * tsVals_;

  Lng32 colIndex_;

  NAString colName_;
}; // class HbaseVersion

class HbaseVersionRef : public BuiltinFunction
{
public:
 HbaseVersionRef(ItemExpr * col)
   : BuiltinFunction(ITM_HBASE_VERSION_REF,
                     CmpCommon::statementHeap()),
    col_(col)
    {}
  
  virtual ~HbaseVersionRef();
  
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  ItemExpr *bindNode(BindWA *bindWA);

  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    return "HBASE_VERSION_REF";
  };
  
 private:
  ItemExpr * col_;
}; // class HbaseVersionRef

// generate a sequence number in a dml query.
class SequenceValue : public BuiltinFunction
{
public:
  SequenceValue(
		const CorrName &seqCorrName,
		NABoolean currVal = FALSE,
		NABoolean nextVal = TRUE)
    : BuiltinFunction(ITM_SEQUENCE_VALUE,
		      CmpCommon::statementHeap(), 0, NULL),
    seqCorrName_(seqCorrName),
    nextVal_(nextVal),
    currVal_(currVal),
    naTable_(NULL)
    {}

  virtual ~SequenceValue();

  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  ItemExpr *bindNode(BindWA *bindWA);

  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const
  {
    return "SEQUENCE_VALUE";
  };

  CorrName &seqCorrName() { return seqCorrName_; }

  inline const NATable  *getNATable()           const { return naTable_; }
  void setNATable (NATable *naTable) {naTable_ = naTable;}

  virtual NABoolean isCacheableExpr(CacheWA& cwa)
    { return TRUE; }

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

 private:
  CorrName seqCorrName_;
  NABoolean currVal_;
  NABoolean nextVal_;

  // natable for the sequence object
  const NATable *naTable_;

}; // class SequenceValue


// ROWNUM().
// Returns the current number of row being returned to application.
// Starts at 1 and increments after it is returned.
// Can only be used in the outermost select and in the outermost
// where predicate.
class RowNumFunc : public BuiltinFunction
{
public:
  RowNumFunc()
       : BuiltinFunction(ITM_ROWNUM, CmpCommon::statementHeap(),
                         0, NULL) {}

  // virtual destructor
  virtual ~RowNumFunc();

  virtual ItemExpr * bindNode(BindWA *bindWA);
  virtual const NAType * synthesizeType();

  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  NABoolean canBeUsedInGBorOB(NABoolean setErr);

}; // class RowNumFunc

class SplitPart : public CacheableBuiltinFunction
{
public: 
  SplitPart(ItemExpr *val1Ptr, ItemExpr *val2Ptr, ItemExpr *val3Ptr)
    :CacheableBuiltinFunction(ITM_SPLIT_PART, 3, val1Ptr, val2Ptr, val3Ptr)
    {} 

   virtual ~SplitPart();

   // a virtual function for type propagating the node
   virtual const NAType * synthesizeType();
   
   virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                      CollHeap *outheap = 0);
   virtual ItemExpr * preCodeGen(Generator*);
}; //class SplitPart

#endif /* ITEMFUNC_H */
