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
#ifndef ITEMLOG_H
#define ITEMLOG_H
/* -*-C++-*-
******************************************************************************
*
* File:         ItemLog.h
* Description:  Logical (boolean) item expressions (AND, OR, =, >, ...)
*
*
* Created:      11/04/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "QRExprElement.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class BiLogic;
class UnLogic;
class BiRelat;

// Global func for Normalizer, should be an ItemExpr:: member but I'm too lazy!
ItemExpr * UnLogicMayBeAnEliminableTruthTest(ItemExpr *item,
					     NABoolean aggsAreEliminable=FALSE);

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------
class Generator;
class MdamCodeGenHelper;
class MdamPred;
class NormWA;
class IntegerList;
class CostScalar;

class ColStatDesc;
template <class T> class SharedPtr;
typedef SharedPtr<ColStatDesc> ColStatDescSharedPtr;
class ColStatDescList;
// -----------------------------------------------------------------------
// binary logic operators (ITM_AND, ITM_OR)
// -----------------------------------------------------------------------
class BiLogic : public ItemExpr
{
  // ITM_AND, ITM_OR
public:
  BiLogic(OperatorTypeEnum otype,
	  ItemExpr *child0 = NULL,
	  ItemExpr *child1 = NULL)
    : ItemExpr(otype,child0,child1)
    , createdFromINlist_(FALSE)
  {
    numLeaves_ = 0;
  }

  // virtual destructor
  virtual ~BiLogic() {}

  // we want BiLogic to be cacheable
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // selectively change literals of cacheable query into constant parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  // if BiLogic ever gets its own data member(s), we must make its data
  // member(s) contribute to its cache key via our own generateCacheKey here

  // get the degree of this node (it is a binary op).
  virtual Int32 getArity() const;

  // An indicator whether this item expression is a predicate.
  virtual NABoolean isAPredicate() const;

  // return true iff we're an expansion of a LIKE predicate
  virtual NABoolean isLike() const;

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

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
  // if its descendant is a multi-value predicate.
  // Returns NULL if no transformation was necessary.
  virtual ItemExpr * transformMultiValuePredicate(
				    NABoolean flattenSubqueries = TRUE,
				    ChildCondition condBiRelat = ANY_CHILD);

  // determines whether the predicate is capable of discarding
  // null augmented rows produced by a left join.
  virtual NABoolean predicateEliminatesNullAugmentedRows(NormWA &, ValueIdSet &);

  // Each operator supports a (virtual) method for transforming its
  // query tree to a canonical form. The parameter setOfPredExpr is
  // supplied only when predicates are normalized.
  virtual ItemExpr * normalizeNode(NormWA & normWARef);

  // A transformation method for protecting sequence functions from not
  // being evaluated due to short-circuit evaluation.
  //
  virtual void protectiveSequenceFunctionTransformation(Generator *generator);

  virtual NABoolean isCovered(const ValueIdSet& newExternalInputs,
			      const GroupAttributes& newRelExprAnchorGA,
	   	              ValueIdSet& referencedInputs,
			      ValueIdSet& coveredSubExpr,
			      ValueIdSet& unCoveredExpr) const;

  virtual NABoolean duplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  void setCreatedFromINlist(NABoolean v){createdFromINlist_ = v;}
  NABoolean createdFromINlist() { return createdFromINlist_;}
  ItemExpr* getINlhs();

  // virtual method to fixup tree for code generation.
  virtual ItemExpr * preCodeGen(Generator *);

  // method to do code generation
  short codeGen(Generator*);

  // method to generate Mdam predicates
  short mdamPredGen(Generator * generator,
                    MdamPred ** head,
                    MdamPred ** tail,
                    MdamCodeGenHelper & mdamHelper,
                    ItemExpr * parent);

  // method to generate MDAM_BETWEEN predicate
  virtual void mdamPredGenSubrange(Generator* generator,
                                   MdamPred** head,
                                   MdamPred** tail,
                                   MdamCodeGenHelper& mdamHelper);

  // get a printable string that identifies the operator
  const NAString getText() const;

  virtual NABoolean isOrderPreserving() const;

  // This operator is supported by the synthesis functions.
  virtual NABoolean synthSupportedOp() const;

  virtual NABoolean applyDefaultPred(ColStatDescList & histograms,
                                                 OperatorTypeEnum exprOpCode,
                                                 ValueId predValueId,
                                     NABoolean & globalPredicate,
                                     CostScalar *maxSelectivity=NULL);

  // MDAM related methods
  // Performs the MDAM tree walk.  See ItemExpr.h for a detailed description.
  DisjunctArray * mdamTreeWalk();

  inline Int32 getNumLeaves () const { return numLeaves_; }
  void setNumLeaves(Int32 num)  {numLeaves_ = num; }

  virtual QR::ExprElement getQRExprElem() const;
  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

  private:
    Int32 numLeaves_;

    NABoolean createdFromINlist_;
}; // class BiLogic

// -----------------------------------------------------------------------
// unary logic operators (NOT, IS_TRUE, ...)
// -----------------------------------------------------------------------
class UnLogic : public ItemExpr
{
  // ITM_NOT, ITM_IS_TRUE, ITM_IS_FALSE, ITM_IS_NULL, ITM_IS_NOT_NULL,
  // ITM_IS_UNKNOWN, ITM_IS_NOT_UNKNOWN
public:
  UnLogic(OperatorTypeEnum otype, ItemExpr *child0 = NULL)
  : ItemExpr(otype,child0)
  {}

  // virtual destructor
  virtual ~UnLogic() {}

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr *bindNode(BindWA *bindWA);

  // we want UnLogic to be cacheable. we do this by inheriting and
  // not overriding ItemExpr::isCacheableExpr.

  // do NOT parameterize literals of UnLogic ItemExpr if we
  // can NOT guarantee the correctness of such parameterization
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA)
    { return this; }

  // if UnLogic ever gets its own data member(s), we must make its data
  // member(s) contribute to its cache key via our own generateCacheKey here

  // An indicator whether this item expression is a predicate.
  virtual NABoolean isAPredicate() const;

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInputs);

  // Note: Only the UnLogic operator needs transformNode2 so not made virtual.
          void transformNode2(NormWA & normWARef,
			     ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
			     const ValueIdSet & externalInputs);

  // Each operator supports a (virtual) method for transforming its
  // query tree to a canonical form.
  virtual ItemExpr * normalizeNode(NormWA & normWARef);

  // A method for inverting (finding the inverse of) the operators
  // in a subtree that is rooted in a NOT.
  virtual ItemExpr * transformSubtreeOfNot(NormWA & normWARef,
                                           OperatorTypeEnum falseOrNot);

  // A virtual method for transforming this predicate
  // if its descendant is a multi-value predicate.
  // Returns NULL if no transformation was necessary.
  virtual ItemExpr * transformMultiValuePredicate(
				    NABoolean flattenSubqueries = TRUE,
				    ChildCondition condBiRelat = ANY_CHILD);

  // determines whether the predicate is capable of discarding
  // null augmented rows produced by a left join.
  virtual NABoolean predicateEliminatesNullAugmentedRows(NormWA &, ValueIdSet &);

  // scalar expressions to a canonical form
  void transformIsNull(NormWA & normWARef,
		       ExprValueId & locationOfPointerToMe,
		       ExprGroupId & introduceSemiJoinHere,
		       const ValueIdSet & externalInputs);

  // method to do code generation
  short codeGen(Generator*);

  // method to generate Mdam predicates
  short mdamPredGen(Generator * generator,
                    MdamPred ** head,
                    MdamPred ** tail,
                    MdamCodeGenHelper & mdamHelper,
                    ItemExpr * parent);

  virtual NABoolean duplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // get a printable string that identifies the operator
  const NAString getText() const;

  // Performs the MDAM tree walk.  See ItemExpr.h for a detailed description.
  DisjunctArray * mdamTreeWalk();

  // default selectivity for unary relational predicates
  virtual double defaultSel();

  virtual NABoolean applyDefaultPred(ColStatDescList & histograms,
                                                 OperatorTypeEnum exprOpCode,
                                                 ValueId predValueId,
                                     NABoolean & globalPredicate,
                                     CostScalar *maxSelectivity=NULL);

  // Is this operator supported by the synthesis functions?
  virtual NABoolean synthSupportedOp() const;

  virtual QR::ExprElement getQRExprElem() const;
  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

}; // class UnLogic


// -----------------------------------------------------------------------
// binary "relational" operators (=, <, >=, ...)
// -----------------------------------------------------------------------
class BiRelat : public ItemExpr
{
  // ITM_EQUAL, ITM_NOT_EQUAL, ITM_LESS, ITM_LESS_EQ, ITM_GREATER, ITM_GREATER_EQ

public:
  BiRelat(OperatorTypeEnum otype,
	  ItemExpr *child0 = NULL,
	  ItemExpr *child1 = NULL,
	  NABoolean specialNulls = FALSE,
          NABoolean isaPartKeyPred = FALSE)
  : ItemExpr(otype,child0,child1),
    specialNulls_(specialNulls),
    specialMultiValuePredicateTransformation_(FALSE),
    directionVector_(NULL),
    isaPartKeyPred_(isaPartKeyPred),
    originalLikeExprId_(NULL_VALUE_ID),
    likeSelectivity_(-1.0),
    derivedFromMCRP_(FALSE),
    listOfComparisonExprs_(NULL),
    preferForSubsetScanKey_(FALSE),
    createdFromINlist_(FALSE),
    collationEncodeComp_(FALSE),
    isNotInPredTransform_(FALSE),
    outerNullFilteringDetected_ (FALSE),
    innerNullFilteringDetected_ (FALSE),
    rollupColumnNum_(-1),
    flags_(0)
  {
#ifndef NDEBUG
    if (NULL != getenv("FORCE_SPECIAL_NULLS")) {
      specialNulls_ = TRUE;
    }
#endif
  }
 
  // virtual destructor
  virtual ~BiRelat() {}

  // we want BiRelat to be cacheable
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // selectively change literals of a cacheable query into input parameters
  virtual ItemExpr* normalizeForCache(CacheWA& cachewa, BindWA& bindWA);

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cachewa) const;

  // get the degree of this node (it is a binary op).
  virtual Int32 getArity() const;

  // a virtual function for performing name binding within the query tree
  virtual ItemExpr *bindNode(BindWA *bindWA);

  virtual void synthTypeAndValueId(NABoolean redriveTypeSynthesisFlag = FALSE, NABoolean redriveChildTypeSynthesis = FALSE);

  // a virtual function for relaxing char type match rules
  ItemExpr* tryToRelaxCharTypeMatchRules(BindWA *bindWA);

  virtual NABoolean isRelaxCharTypeMatchRulesPossible() {return TRUE;};

  // An indicator whether this item expression is a predicate.
  virtual NABoolean isAPredicate() const;

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

  // Get the comparison operator for performing the comparison in reverse
  OperatorTypeEnum getReverseOperatorType() const;

  void *unused_NONvirtual_method_1();	//## placeholder: reuse w/ any signature

  void *unused_NONvirtual_method_2();	//## placeholder: reuse w/ any signature

  // A virtual method for transforming (a,b) <op> (c,d) for any <op>,
  // but only if both (or just one of) children are ItemList.
  // Returns NULL if no transformation was necessary.
  virtual ItemExpr * transformMultiValuePredicate(
					  NABoolean flattenSubqueries = TRUE,
					  ChildCondition tfmIf = ANY_CHILD);

  // determines whether the predicate is capable of discarding
  // null augmented rows produced by a left join.
  virtual NABoolean predicateEliminatesNullAugmentedRows(NormWA &, ValueIdSet &);

  // Each operator supports a (virtual) method for transforming its
  // query tree to a canonical form. The parameter setOfPredExpr is
  // supplied only when predicates are normalized.
  virtual ItemExpr * normalizeNode(NormWA & normWARef);

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  // method to do code generation
  ItemExpr * preCodeGen(Generator*);
  short codeGen(Generator*);

  // method to generate Mdam predicates
  short mdamPredGen(Generator * generator,
                    MdamPred ** head,
                    MdamPred ** tail,
                    MdamCodeGenHelper & mdamHelper,
                    ItemExpr * parent);

  // method to get information on a BiRelat that is one of the operands of an
  // AND that represents an interval (MDAM_BETWEEN).
  void getMdamPredDetails(Generator* generator,
                          MdamCodeGenHelper& mdamHelper, 
                          enum MdamPred::MdamPredType& predType,
                          ex_expr** vexpr);

  virtual NABoolean duplicateMatch(const ItemExpr &other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // get a printable string that identifies the operator
  const NAString getText() const;

  NABoolean getSpecialNulls() const
  {
      return specialNulls_;
  }

  void setSpecialNulls(NABoolean flag)
  {
    specialNulls_ = flag;
  }

  NABoolean & specialMultiValuePredicateTransformation()
  { return specialMultiValuePredicateTransformation_; }

  void setSpecialMultiValuePredicateTransformation(NABoolean flag)
  {specialMultiValuePredicateTransformation_ = flag;}

  NABoolean isaPartKeyPred() const      { return isaPartKeyPred_; }
  void setIsaPartKeyPred(NABoolean flag) {isaPartKeyPred_ = flag; }

  ValueId originalLikeExprId() const	{ return originalLikeExprId_; }
  void setOriginalLikeExprId(ValueId originalLikeExprId)
    {originalLikeExprId_ = originalLikeExprId; }

  NABoolean derivativeOfLike();

  void adjustRowcountAndUecForLike(const ColStatDescSharedPtr &colStats,
				  CostScalar rowcountBeforePreds,
				  CostScalar totalUecBeforePreds,
				  CostScalar baseUecBeforePreds);

  virtual NABoolean applyDefaultPred(ColStatDescList & histograms,
                                                 OperatorTypeEnum exprOpCode,
                                                 ValueId predValueId,
                                     NABoolean & globalPredicate,
                                     CostScalar *maxSelectivity=NULL);

private:
  enum {
    FOR_LIKE = 0x0001
  };

  // Helper methods for applyDefaultPred() method
  NABoolean isSimpleComplexPredInvolved();

  void applyEquiJoinExpr(ColStatDescSharedPtr & leftStatDesc,
				 ColStatDescSharedPtr & rightStatDesc,
				 ColStatDescList & histograms);

  void applyLocalPredExpr(ColStatDescSharedPtr & leftStatDesc,
                          ColStatDescSharedPtr & rightStatDesc,
                          OperatorTypeEnum exprOpCode,
                          ValueIdSet leftLeafValues,
                          ValueIdSet rightLeafValues,
                          CostScalar *maxSelectivity=NULL);

  void applyRangePredExpr(ColStatDescSharedPtr & leftStatDesc,
				    ValueIdSet leftLeafValues,
				    ValueIdSet rightLeafValues);

public:

  // default selectivity for birelational predicates
  virtual double defaultSel();

  // Is this operator supported by the synthesis functions?
  virtual NABoolean synthSupportedOp() const;

  const IntegerList *getDirectionVector() const { return directionVector_;}
  void setDirectionVector(IntegerList *v) { directionVector_ = v; }

  void setLikeSelectivity (double sel) { likeSelectivity_ = sel; }
  double getLikeSelectivity() const { return likeSelectivity_ ; }

  // MCRP Code - Begin

  // Is this range predicate derived from a MCRP,
  // i.e. MultiColumn Range Predicate. A MCRP e.g. (a,b) > (1,2)
  // is transformed into (a >= 1) and (a, b) > (1,2). The transformation is
  // performed because the single subset scan optimizer does recognize MCRPs.
  // Therefore a single column range predicate is add (in this case a >= 1
  // was added) to help specify a single subset scan begin key.
  // This will return TRUE for the added predicate i.e. a >= 1.
  NABoolean isDerivedFromMCRP() { return derivedFromMCRP_;}

  // Mark this predicate as derived from a MCRP, this is set in method
  // static ItemExpr * transformMultiValueComparison().
  // transfromMultiValueComparison, transforms an MCRP and adds the extra
  // predicate mentioned above.
  void setDerivedFromMCRP() { derivedFromMCRP_ = TRUE; }

  void setListOfComparisons(ItemExprList * comparisonList)
                           { listOfComparisonExprs_ = comparisonList;}

  ItemExprList * getListOfComparisons() { return listOfComparisonExprs_;}

  void getListOfComparisonIds(ValueIdList & listOfComparisonIds)
  { listOfComparisonIds = listOfComparisonExprIds_; }

  void getLeftMCRPChildList(ValueIdList & leftMCRPChildList);
  void getRightMCRPChildList(ValueIdList & rightMCRPChildList);

  void setPreferForSubsetScanKey(NABoolean preference = TRUE)
  { preferForSubsetScanKey_ = preference; }

  NABoolean isPreferredForSubsetScanKey()
  { return preferForSubsetScanKey_;}

  // converts listOfComparisonExprs_ to listOfComparisonExprIds_
  // this is called after the comparisons have be bound, therefore
  // each comparison should have a ValueId
  void translateListOfComparisonsIntoValueIds();

  //MCRP Code - End

  void setCreatedFromINlist(NABoolean v){createdFromINlist_ = v;}
  NABoolean createdFromINlist() { return createdFromINlist_;}

  void setCollationEncodeComp(NABoolean v) {collationEncodeComp_= v;}
  NABoolean getCollationEncodeComp() { return collationEncodeComp_;}
  virtual NABoolean hasEquivalentProperties(ItemExpr * other) { return TRUE;}

  Int16 &rollupColumnNum() { return rollupColumnNum_; }

  //Not In optimization methods
  // indicate that this birelat is a transformation of a NotIn.
  NABoolean getIsNotInPredTransform() const
  {
    return isNotInPredTransform_;
  }

  void setIsNotInPredTransform(NABoolean v)
  {
    isNotInPredTransform_=v;
  }

  // indicate whether Null filtering was detected when the predicates
  // were pulled up.
  NABoolean getOuterNullFilteringDetected() const
  {
    return outerNullFilteringDetected_;
  }
 
  void setOuterNullFilteringDetected(NABoolean v)
  {
    outerNullFilteringDetected_ = v;
  }
 
  NABoolean getInnerNullFilteringDetected() const
  {
    return innerNullFilteringDetected_;
  }
 
  void setInnerNullFilteringDetected(NABoolean v)
  {
    innerNullFilteringDetected_ = v;
  }
  // Not in optimization --end

  // relax < to <=, > to >=, or return ori
  OperatorTypeEnum getRelaxedComparisonOpType() const;

  // get and set for flags_. See enum Flags.
  NABoolean addedForLikePred()   { return (flags_ & FOR_LIKE) != 0; }
  void setAddedForLikePred(NABoolean v)
  { (v ? flags_ |= FOR_LIKE : flags_ &= ~FOR_LIKE); }

 private:

  // helper to change literals of a cacheable query into input parameters
  void parameterizeMe(CacheWA& cachewa, BindWA& bindWA, ExprValueId& child,
                      BaseColumn *base, ConstValue *val);

  // MDAM related methods:

  // Performs the MDAM tree walk.  See ItemExpr.h for a detailed description.
  DisjunctArray * mdamTreeWalk();

  // if we are allowing certain incompatible comparisons handle them.
  // currently the following incompatible comparisons are supported:
  // 1. Numeric and Character
  // 2. Date and Character literal
  // This returns a pointer to itself if everything goes fine
  // otherwise it returns NULL
  BiRelat * handleIncompatibleComparison(BindWA *bindWA);

  // The flag, if set, indicates that 'nulls are values'.
  // That means that nulls are equal to other nulls, and they
  // sort higher than other values. Used for groupby, order
  // by, hashing, or any other operation where nulls are NOT
  // to be ignored.
  //
  // MVs:
  // The flag is used in specific equi-join predicates
  // Irena
  NABoolean specialNulls_;

  // See QuantifiedComp::getBlackBoxExpr
  NABoolean specialMultiValuePredicateTransformation_;

  // -- MVs
  // The direction vector is used for comparing lists of index columns
  // that were created with different orders, for example (A ASC, B DESC).
  IntegerList *directionVector_;

  // Predicate is a partitioning key predicate
  NABoolean isaPartKeyPred_;

  // If this Range expression is derived from 'Like' expression, then
  // the following attribute contains the ValueId of the parent Like
  // expression. If it is a regular logical expression, then it contains
  // NULL_VALUE_ID. It will be set with the ValueId when the BiRelat
  // constructor is called from Like::applyBeginEndKeys
  ValueId originalLikeExprId_;

  // This contains the portion of the default selectivity for the like
  // predicate, that should be applied to this predicate. In the
  // constructor for this expression, we would set it to -1. It would be
  // set by the computed value in the Like::applyBeginEndKeys method
  // where we set the originalLikeExprId_ with the ValueId of the original
  // expression, in case the original expression is a Like predicate.

  double likeSelectivity_;

  // MCRP Code - Begin

  // Flag indicating that this comparison is derived from a MCRP
  // For MCRPs (MultiColumnRangePredicates), i.e predicates of the form
  // (a, b, c) > (1, 2, 3), we add a predicate and transform the predicate
  // to (a >= 1) and ((a, b, c) > (1, 2, 3)). This flag will be set for the
  // added comparison predicate '>=' i.e. (a >= 1).
  // If this flag is set then the datamembers below should also be set.
  // If this flag is not set then the datamembers below should not be set
  NABoolean derivedFromMCRP_;

  // list of item expressions representing comparisons on each item of a MCRP
  ItemExprList * listOfComparisonExprs_;

  // This is derived from listOfComparisonExprs_
  // This is the list of ValueIds corresponding to each comparison
  // in the listOfComparisonExprs_
  ValueIdList listOfComparisonExprIds_;

  // This is the list of ValueIds for the left Children of the MCRP
  // Given MCRP (a, b, c) > (1, 2, 3), this list will contain ValueIds
  // of (a, b, c). The list will likely contain the ValueId for the VEG
  // representing each of a, b, c. Since this is populated on demand,
  // therefore whenever the leftMCRPChildList_ is request for the first time
  // if the VEGs have formed, then the ValueIds will represent VEGs
  ValueIdList leftMCRPChildList_;

  // Similar to the list above, except for the right children of the
  // MCRP. Therefore for MCRP (a, b, c) > (1, 2, 3) this will be
  // list of ValueIds representing (1, 2, 3).
  ValueIdList rightMCRPChildList_;

  // flag to indicate if this predicate should be preferred for defining
  // subset scan begin/end keys
  NABoolean preferForSubsetScanKey_;

  // MCRP Code - End

  NABoolean createdFromINlist_;

  virtual QR::ExprElement getQRExprElem() const;

  NABoolean collationEncodeComp_;

  // flag to indicate that this birelat is a transformation of a NotIn.
  NABoolean isNotInPredTransform_;

  // flags to indicate whether Null filtering was detected when the predicates
  // were pulled up.
  NABoolean outerNullFilteringDetected_;

  NABoolean innerNullFilteringDetected_;

  // Used for groupby rollup.
  // Set in generator during groupby comparison expression generation.
  // It indicates the position of grouping column that caused the group
  // change during groupby computation. 
  // Used to return the rollup groups.
  Int16 rollupColumnNum_;

  UInt32 flags_;

}; // class BiRelat

class BiConnectByRelat :public BiRelat {
public:
  BiConnectByRelat( OperatorTypeEnum otype,
          ItemExpr *child0 = NULL,
          ItemExpr *child1 = NULL)
   :BiRelat(otype, child0, child1)
  {
    parentIe_ = NULL;
    childIe_ = NULL;
  }

  virtual ~BiConnectByRelat() {}

  void setParentColName(char * v) { parentColName_ = v; }
  void setParentColIE(ItemExpr* v) { parentIe_= v; }
  NAString getParentColName () { return parentColName_; }
  ItemExpr* getParentColIE() { return parentIe_; }

  void setChildColName(char * v) { childColName_ = v; }
  void setChildColIE(ItemExpr* v) { childIe_ = v; }
  NAString getChildColName() { return childColName_ ; }
  ItemExpr* getChildColIE() { return childIe_; }

private:
  NAString parentColName_;
  NAString childColName_;
  ItemExpr * parentIe_;
  ItemExpr *childIe_;
};

class KeyRangeCompare : public BiRelat
{

public:
  KeyRangeCompare(const CorrName &objectName,
		  OperatorTypeEnum otype,
		  ItemExpr *child0,
		  ItemExpr *child1)
    : BiRelat(otype, 
	      child0, 
	      child1, 
	      TRUE /* specialNulls */, 
	      FALSE /* isaPartKeyPred */
	      ),
      objectName_(objectName)
  {
    setSpecialMultiValuePredicateTransformation
      (FALSE);
  }

  // virtual destructor
  virtual ~KeyRangeCompare() {}

  // Does semantic checks and binds the function.
  virtual ItemExpr * bindNode(BindWA * bindWA);

  // get a printable string that identifies the operator
  //
  virtual const NAString getText() const;

  const CorrName & getObjectName() const  { return objectName_; }
  CorrName & getObjectName()        { return objectName_; }

  void setObjectName(const CorrName& corrName) {objectName_ = corrName;}

  NABoolean verifyPartitioningKeys(BindWA *bindWA,
				   ItemExpr *tree, 
				   const NAColumnArray &partKeyCols,
				   CollHeap* heap);

 private:

  // ---------------------------------------------------------------//
  // the user-specified name of the table or an index or any other
  // SQL/MX object.
  // -------------------------------------------------------------- //
  CorrName objectName_;
};

// -----------------------------------------------------------------------
// NOT IN
// a query like "select * from T1 where a NOT IN (select b from T2);"
// is transformed into an Anti semi join with a join predicate of 
// NotIn(a,b)
//
// NotIn (a,b) -- a and b can be either item expressions in the case 
// of single column NOT IN or itemlists in the case on multi-column
// NOT IN
// -----------------------------------------------------------------------
class NotIn : public BiRelat
{

public:
  NotIn(  ItemExpr *child0 = NULL,
          ItemExpr *child1 = NULL)
  : BiRelat(ITM_NOT_IN,
            child0,
            child1),
    equivEquiPredicate_(NULL_VALUE_ID),
    equivNonEquiPredicate_(NULL_VALUE_ID),
    isOneInnerBroadcastRequired_(NotIn::NOT_SET)
    {}

  // virtual destructor
  virtual ~NotIn() {}

  virtual void transformNode(NormWA & normWARef,
                             ExprValueId & locationOfPointerToMe,
                             ExprGroupId & introduceSemiJoinHere,
                             const ValueIdSet & externalInputs);

  static ValueIdSet rewriteMultiColNotInPredicate( ValueId ninValId, 
                                                  ValueIdSet joinPred,
                                                  ValueIdSet selPred);

  virtual const NAString getText() const
  {
    return "NotIn";
  }
 
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
                                 CollHeap* outHeap = 0);

  ItemExpr * preCodeGen(Generator*);

  ValueId createEquivEquiPredicate() const;
  ValueId createEquivNonEquiPredicate() const;

  void cacheEquivEquiPredicate();
  void cacheEquivNonEquiPredicate();

  ValueId getEquivEquiPredicate() const
  {
    return equivEquiPredicate_;
  }
 
  void setEquivEquiPredicate(ValueId v)
  {
    DCMPASSERT ( child(0)->getOperatorType() != ITM_ITEM_LIST && 
                child(1)->getOperatorType() != ITM_ITEM_LIST );

    equivEquiPredicate_ = v;
  }
 
  ValueId getEquivNonEquiPredicate() const
  {
    return equivNonEquiPredicate_;
  }
 
  void setEquivNonEquiPredicate(ValueId v)
  {
    DCMPASSERT ( child(0)->getOperatorType() != ITM_ITEM_LIST);
    equivNonEquiPredicate_ = v;
  }
 
 
  void cacheIsOneInnerBroadcastRequired();

  NABoolean getIsOneInnerBroadcastRequired()
  {
    if (isOneInnerBroadcastRequired_ == NotIn::NOT_SET)
    {
      cacheIsOneInnerBroadcastRequired();
    }
    if (isOneInnerBroadcastRequired_ == NotIn::REQUIRED)
    {
      return TRUE;
    }
    else
    {
      return FALSE;
    }
  }

private:
  enum enumRequireOneBroadcast
  { 
    NOT_SET = 0,
    REQUIRED=1,
    NOT_REQUIRED=2
  };

  // cache for the equi and non equi-predicate for single col NOT IN
  // used during optimization phase
  ValueId equivEquiPredicate_;
  ValueId equivNonEquiPredicate_;

  // this field serves as a cache to store a value indicating whether
  // broadcast of one row is required and reuse it instead of computing 
  // it multiple times
  enumRequireOneBroadcast isOneInnerBroadcastRequired_;

};//class NotIn


#endif /* ITEMLOG_H */
