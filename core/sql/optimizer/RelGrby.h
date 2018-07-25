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
#ifndef RELGRBY_H
#define RELGRBY_H
/* -*-C++-*-
******************************************************************************
*
* File:         RelGrby.h
* Description:  Relational aggregates and group by
*               (both physical and logical operators)
* Created:      4/28/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "ObjectNames.h"
#include "RelExpr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class GroupByAgg;
class SortGroupBy;
class ShortCutGroupBy;
class PhysShortCutGroupBy;
class HashGroupBy;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;
class NormWA;
class Generator;
class PartitioningFunction;
class RequirementGenerator;
class ex_expr;
class ExpTupleDesc;
class ComTdb;
////////////////////
class QueryAnalysis;
class GBAnalysis;
class CANodeIdSet;
class JBBSubsetAnalysis;
////////////////////

// -----------------------------------------------------------------------
// The GroubByAgg operator has three additional expressions. The first
// one, called grouping expression, is used to split its child rows into
// groups. Each group of child rows has the same value for the grouping
// expression.
// The second expression, the aggregate expression, can reference the
// result of the grouping expression and it can reference the columns of
// the child table by means of aggregate functions.
// The third expression, if present, specifies a single result row
// that is produced when the GROUPBY operator has no child rows. This
// can be used to implement ANSI style aggregates that have a special
// return value (NULL or 0) for an empty set. This is called the empty
// child result expression. If the expression is not present, then
// no result is produced if the child of the operator is empty.
//
// If the operator type is REL_GROUPBY then the third expression must
// not be present.
// -----------------------------------------------------------------------

class GroupByAgg : public RelExpr
{
public:

  // constructor
  GroupByAgg(RelExpr *child,
             OperatorTypeEnum otype = REL_GROUPBY,
             ItemExpr *groupExpr = NULL,
             ItemExpr *aggregateExpr = NULL,
             CollHeap *oHeap = CmpCommon::statementHeap())
  : RelExpr(otype, child, NULL, oHeap),
    groupExprTree_(groupExpr),
    aggregateExprTree_(aggregateExpr),
    gbAggPushedBelowTSJ_(FALSE),
    formEnum_(FULL_GROUPBY),
    gbAnalysis_(NULL),
    selIndexInHaving_(FALSE),
    requiresMoveUp_(FALSE),
    containsNullRejectingPredicates_(FALSE),
    parentRootSelectList_(NULL),
    isMarkedForElimination_(FALSE),
    aggDistElimRuleCreates_(FALSE),
    groupByOnJoinRuleCreates_(FALSE),
    extraGrpOrderby_(NULL),
    isRollup_(FALSE)
  {}

  // constructor
  GroupByAgg(RelExpr *child, const ValueIdSet & aggregateExpr)
  : RelExpr(REL_GROUPBY, child),
    groupExprTree_(NULL), aggregateExprTree_(NULL),
    aggregateExpr_(aggregateExpr),
    gbAggPushedBelowTSJ_(FALSE),
    formEnum_(FULL_GROUPBY),
    gbAnalysis_(NULL),
    selIndexInHaving_(FALSE),
    requiresMoveUp_(FALSE),
    containsNullRejectingPredicates_(FALSE),
    parentRootSelectList_(NULL),
    isMarkedForElimination_(FALSE),
    aggDistElimRuleCreates_(FALSE),
    groupByOnJoinRuleCreates_(FALSE),
    extraGrpOrderby_(NULL),
    isRollup_(FALSE)
  {}

  // virtual destructor
  virtual ~GroupByAgg();

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  // get and set the grouping and aggregate expressions as parse trees
  ItemExpr * getGroupExprTree() const	{ return groupExprTree_; }
  void addGroupExprTree(ItemExpr *groupExpr);
  ItemExpr * removeGroupExprTree();
  void addAggregateExprTree(ItemExpr *aggrExpr);
  ItemExpr * removeAggregateExprTree();

  // return a (short-lived) read/write reference to the item expressions
  inline ValueIdSet & groupExpr() { return groupExpr_; }
  inline const ValueIdSet & groupExpr() const { return groupExpr_; }
  inline void setGroupExpr(ValueIdSet &expr) { groupExpr_ = expr;}
  inline void addGroupExpr(ValueIdSet &expr) { groupExpr_ += expr;}

  inline void setExtraOrderExpr(const ValueIdList &newExtraOrder) { extraOrderExpr_ = newExtraOrder; }
  inline const ValueIdList & extraOrderExpr() const { return extraOrderExpr_; }
  void normalizeExtraOrderExpr( NormWA & normWARef  ) { extraOrderExpr_.normalizeNode(normWARef); }
  
  ValueIdList & rollupGroupExprList() { return rollupGroupExprList_; }
  const ValueIdList & rollupGroupExprList() const { return rollupGroupExprList_; }
  void setRollupGroupExprList(ValueIdList &expr) { rollupGroupExprList_ = expr;}

  // return a (short-lived) read/write reference to the item expressions
  inline ValueIdSet & leftUniqueExpr() { return leftUniqueExpr_; }
  inline const ValueIdSet & leftUniqueExpr() const { return leftUniqueExpr_; }
  inline void setLeftUniqueExpr(ValueIdSet &expr) { leftUniqueExpr_ = expr;}

  inline ValueIdSet & aggregateExpr() { return aggregateExpr_; }
  inline const ValueIdSet & aggregateExpr() const { return aggregateExpr_; }
  inline void setAggregateExpr(ValueIdSet &expr) { aggregateExpr_ = expr;}

  // Mutator method to change the gb pushed below TSJ flag
  inline NABoolean & gbAggPushedBelowTSJ()
    { return gbAggPushedBelowTSJ_; }
  // Accessor method to access the gb pushed below TSJ flag
  inline NABoolean gbAggPushedBelowTSJ() const
    { return gbAggPushedBelowTSJ_; }

  // Get the values that are needed for evaluating the aggregate.
  void getValuesRequiredForEvaluatingAggregate(ValueIdSet& relevantValues);

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // MV --
  NABoolean virtual isIncrementalMV() { return TRUE; }
  void virtual collectMVInformation(MVInfoForDDL *mvInfo,
				    NABoolean isNormalized);

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe);

  // a method used during subquery transformation for pulling up predicates
  // towards the root of the transformed subquery tree
  virtual void pullUpPreds();

  // a method used for recomputing the outer references (external dataflow
  // input values) that are still referenced by each operator in the
  // subquery tree after the preidcate pull up is complete.
  virtual void recomputeOuterReferences();

  // Each operator supports a (virtual) method for rewriting its
  // value expressions.
  virtual void rewriteNode(NormWA & normWARef);

  // Each operator supports a (virtual) method for performing
  // predicate pushdown and computing a "minimal" set of
  // characteristic input and characteristic output values.
  virtual RelExpr * normalizeNode(NormWA & normWARef);

  virtual RelExpr * semanticQueryOptimizeNode(NormWA & normWARef);

  virtual void checkForCascadedGroupBy(NormWA &normWaRef);

  virtual void eliminateCascadedGroupBy(NormWA &normWaRef);

  virtual NABoolean prepareMeForCSESharing(
       const ValueIdSet &outputsToAdd,
       const ValueIdSet &predicatesToRemove,
       const ValueIdSet &commonPredicatesToAdd,
       const ValueIdSet &inputsToRemove,
       ValueIdSet &valuesForVEGRewrite,
       ValueIdSet &keyColumns,
       CSEInfo *info);

  // flows compRefOpt constraints up the query tree.
  virtual void processCompRefOptConstraints(NormWA * normWAPtr) ;

  // Method to push down predicates from a groupby node into the
  // children
  virtual
  void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                           const ValueIdSet & newExternalInputs,
                           ValueIdSet& predOnOperator,
			   const ValueIdSet * nonPredExprOnOperator = NULL,
                           Lng32 childId = (-MAX_REL_ARITY) );

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  virtual void computeMyRequiredResources(RequiredResources & reqResources,
                                      EstLogPropSharedPtr & inLP);
  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  // Helper method for the above synthEstLogProp() method
  void handleIndirectDepInGroupingcols(ValueIdSet & workGroup,
				       ValueIdSet & interestingColSet,
				       ValueIdSet & updatedColumnsToRemove,
				       ValueIdSet & probableRedundantColSet,
				       const ColStatDescList & groupStatDescList);

  // ---------------------------------------------------------------------
  // The following method examines the aggregate functions that are
  // members of the aggrExpr(). If any aggregate function cannot be
  // computed using a two-step process, a partial aggregation followed
  // by finalization, it returns FALSE. A typical aggregate that
  // cannot be computed using such a two-step process is the one row
  // aggregate that is introduced by the transformation of a scalar/row
  // subquery to a Join-Aggregate form.
  // ---------------------------------------------------------------------
  NABoolean aggregateEvaluationCanBeStaged() const;

  // ---------------------------------------------------------------------
  //
  // The optimizer views a GroupByAgg to be in one of four forms:
  //
  // 1) A "full" groupby, i.e., it is not a partial groupby.
  //
  //    The parser creates a group by of this form. A groupby of this
  //    form is seen by all the phases of processing from name binding
  //    through code generation.
  //
  //    The method isNotAPartialGroupBy() returns TRUE for a "full"
  //    groupby and false for all other forms.
  //
  // 2) Partial groupby operators.
  //
  //    Partial groupby operators can appear in the solution for a
  //    group by because they are created by the application of the
  //    transformation rule, the GroupBySplitRule. The GroupBySplitRule
  //    fires once for a "full" groupby and can fire once more for a
  //    partial groupby that is a leaf. The rule replaces
  //                    GB(X) with GB(GB(X))
  //    Partial groupby operators are only seen by the optimizer, the
  //    pre code generator and the code generator.
  //
  //    When the GroupBySplitRule fires on a "full" groupby it produces
  //    the following pairs of groupbys
  //
  //    a) Root of the partial groupby subtree
  //
  //       The root of the partial groupby consolidates the partial groups
  //       that are formed by one or more groupbys that independently
  //       compute partial groups.
  //
  //       The method isAPartialGroupByRoot() returns TRUE only for such
  //       a partial groupby.
  //
  //    b) 1st Leaf of the partial groupby subtree.
  //
  //       The 1st leaf of the partial groupby subtree is a simply the
  //       the lowermost partial groupby created when the GroupBySplitRule
  //       fires on a "full" groupby. The term "leaf" is convenient for
  //       naming the partial groupby operators that are contained in the
  //       substitute. The reader should realize that, in reality, the
  //       GroupByAgg is a unary operator; the partial groupby will always
  //       posses a child, regardless of whether it is called a "leaf"!
  //
  //       The method isAPartialGroupByLeaf1() returns TRUE only for such
  //       a partial groupby.
  //
  //    The GroupBySplitRule can fire on the 1st leaf to produce another
  //    pair of parital groupby operators called the non-leaf partial
  //    groupby together with the 2nd leaf.
  //
  //    c) Non-leaf partial groupby
  //
  //       A plan can contain three partial groupbys that span two
  //       levels of the tree, the root, a non-leaf and a leaf.
  //       The non-leaf partially consolidates partial groups
  //       that are formed by the 2nd leaf of the partial groupby
  //       subtree. It adds one more operator to the pipeline but
  //       increases the degree of parallelism for the leaves.
  //
  //       The method isAPartialGroupByNonLeaf() returns TRUE only for such
  //       a partial groupby.
  //
  //    d) 2nd Leaf of the partial groupby subtree.
  //
  //       The 2nd leaf of the partial groupby subtree is a simply the
  //       the lowermost partial groupby created when the GroupBySplitRule
  //       fires on a partial groupby that is a 1st leaf.
  //
  //       The method isAPartialGroupByLeaf2() returns TRUE only for such
  //       a partial groupby.
  //
  // The distinction between the 1st leaf and the 2nd leaf is made purely to
  // aid costing in the optimizer. The code generator processes both
  // the forms of the leafs in an identical manner. The method
  // isAPartialGroupByLeaf() is defined to permit the code generator
  // to identify a leaf, independent of its form.
  //
  // A GroupByAgg operator in any one of the above forms uses the same
  // implementation for grouping the data at run time. This means,
  // regardless of its form, a GroupByAgg will either use the hash
  // grouping or sorted grouping implementation at run time.
  //
  // ---------------------------------------------------------------------
  NABoolean isNotAPartialGroupBy() const
                                   { return (formEnum_ == FULL_GROUPBY); }
  NABoolean isAPartialGroupByRoot() const
                           { return (formEnum_ == PARTIAL_GROUPBY_ROOT); }
  NABoolean isAPartialGroupByNonLeaf() const
                       { return (formEnum_ == PARTIAL_GROUPBY_NON_LEAF); }
  NABoolean isAPartialGroupByLeaf() const
        { return (isAPartialGroupByLeaf1() OR isAPartialGroupByLeaf2()); }


  NABoolean isAPartialGroupByLeaf1() const
                          { return (formEnum_ == PARTIAL_GROUPBY_LEAF1); }
  NABoolean isAPartialGroupByLeaf2() const
                          { return (formEnum_ == PARTIAL_GROUPBY_LEAF2); }

  void markAsPartialGroupByRoot()    { formEnum_ = PARTIAL_GROUPBY_ROOT; }

  void markAsPartialGroupByNonLeaf()
                                 { formEnum_ = PARTIAL_GROUPBY_NON_LEAF; }

  void markAsPartialGroupByLeaf1()  { formEnum_ = PARTIAL_GROUPBY_LEAF1; }

  void markAsPartialGroupByLeaf2()
                                    { formEnum_ = PARTIAL_GROUPBY_LEAF2; }

  // ---------------------------------------------------------------------
  // The following method is used for marking a copy of this GroupByAgg
  // as being in the same form as the original.
  // ---------------------------------------------------------------------
  void assignForm(const GroupByAgg& other)
                                          { formEnum_ = other.formEnum_; }

  // ---------------------------------------------------------------------
  // Common implementation for testing the eligibility of this
  // GroupByAgg to form a plan using the given required physical
  // properties.
  // This method is called by various implementations of the
  // method ...GroupByRule::topMatch().
  // ---------------------------------------------------------------------
  NABoolean rppAreCompatibleWithOperator
              (const ReqdPhysicalProperty* const rppForGrby) const;

  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);

  virtual void addArrangementAndOrderRequirements(RequirementGenerator &rg);

  virtual RelExpr * preCodeGen(Generator *generator,
			       const ValueIdSet &externalInputs,
			       ValueIdSet &pulledNewInputs);

  NABoolean executeInDP2() const;

  // generates aggregate and groupby expressions. Called by
  // SortGroupBy::codeGen.
  // Defined in file GenRelGrby.C
  short genAggrGrbyExpr(Generator * generator,
			ValueIdSet &aggregateExpr, 
                        ValueIdSet &groupExpr,
                        ValueIdList &rollupGroupExprList,
			ValueIdSet &selectionPred,
			Int32 workAtp, Int32 workAtpIndex,
			short returnedAtpIndex,
			ex_expr ** aggr_expr, ex_expr ** grby_expr,
			ex_expr ** move_expr, ex_expr ** having_expr,
			ComTdb ** child_tdb, ExpTupleDesc ** tuple_desc);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // adds information about this node to the Explain tree
  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);


  // method to do code generation
  virtual short codeGen(Generator*);

  // append an ascii-version of GroupByAgg into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  NABoolean selIndexInHaving() { return selIndexInHaving_; }
  void setSelIndexInHaving(NABoolean v) { selIndexInHaving_ = v; }

//////////////////////////////////////////////////////
  virtual NABoolean pilotAnalysis(QueryAnalysis* qa);
  virtual void jbbAnalysis(QueryAnalysis* qa);
  JBBSubsetAnalysis* getJBBSubsetAnalysis();
  virtual void primeGroupAnalysis();
  virtual RelExpr * generateLogicalExpr(CANodeIdSet &, CANodeIdSet &);
  virtual RelExpr * generateMatchingExpr(CANodeIdSet &, CANodeIdSet &,
                                          RelExpr *);
  inline GBAnalysis* getGBAnalysis()
  {
    return gbAnalysis_;
  }
  inline void setGBAnalysis(GBAnalysis* gbAnalysis)
  {
    gbAnalysis_ = gbAnalysis;
  }

  inline NABoolean requiresMoveUp() { return requiresMoveUp_ ;}
  inline void setRequiresMoveUp(NABoolean val) 
    { requiresMoveUp_ = val ;}

  inline NABoolean containsNullRejectingPredicates() 
  { return containsNullRejectingPredicates_ ;}
  inline void setContainsNullRejectingPredicates(NABoolean val) 
    { containsNullRejectingPredicates_ = val ;}

  ItemExpr * getParentRootSelectList() const	{ return parentRootSelectList_; }
  void setParentRootSelectList(ItemExpr *selectList)
  {parentRootSelectList_ = selectList;};
  void removeParentRootSelectList()
  {parentRootSelectList_ = NULL;};

  inline NABoolean aggDistElimRuleCreates() 
    { return aggDistElimRuleCreates_ ;}
  inline void setAggDistElimRuleCreates(NABoolean val)
    { aggDistElimRuleCreates_ = val ;}
  inline NABoolean groupByOnJoinRuleCreates()
    { return groupByOnJoinRuleCreates_; }
  inline void setGroupByOnJoinRuleCreates (NABoolean val)
    { groupByOnJoinRuleCreates_ = val ;}

 // SQO methods

  // used to refine the group expresiion of a GroupBy
  // as the node is moved/pulled up during subquery unnesting.
  void computeGroupExpr(const ValueIdSet& seed, 
			ValueIdSet& superSet,
			NormWA& normWARef);

  // performs post processing for subquery unnesting after
  // the two main transformations (pullUp and moveUp GroupBy)
  // have been applied. Returns FALSE if an error is encountered.
  NABoolean subqueryUnnestFinalize(ValueIdSet& newGrbyGroupExpr, 
				    NormWA& normWARef);

  // a method that creates a MapValueId node that can be used
  // to map between regular column references through instantiateNull
  // references back to column references again. This is used by the
  // unnesting code when a join is transformed from a Join to a LeftJoin.
  // Since we are past the binder state, and LeftJoins can only produce
  // instantiateNull outputs, we need to map between those outputs and the
  // regular base column outputs the nodes above the GroupBy expects the 
  // child to produce as outputs...
  MapValueIds * buildMapValueIdNode(ValueIdMap *map); 

  // used by subquery unnesting as an additional step to the pullUpGroupBy
  // transformation, if left joins are needed.
  RelExpr* nullPreservingTransformation(GroupByAgg* oldGB, 
                                        NormWA& normWARef);

  // used by subquery unnesting as an additional step to the pullUpGroupBy
  // moveUpGroupBy transformation, to rewrite the groupby's expression
  // in case we need to put it on top of a Left Join.
  RelExpr* nullPreserveMyExprs( NormWA& normWARef);

  NABoolean isMarkedForElimination() 
                      { return isMarkedForElimination_; }

  void setIsMarkedForElimination(NABoolean val) 
                      { isMarkedForElimination_ = TRUE; }

  inline ValueIdSet & aggrExprsToBeDeleted() { return aggrExprsToBeDeleted_; }

  NABoolean tryToPullUpPredicatesInPreCodeGen(
     const ValueIdSet &valuesAvailableInParent, // pull preds that are covered by these
     ValueIdSet       &pulledPredicates,        // return the pulled-up preds
     ValueIdMap       *optionalMap);            // optional map to rewrite preds

  void setIsRollup(NABoolean v) { isRollup_ = v; }
  NABoolean isRollup() { return isRollup_; }
  const NABoolean isRollup() const { return isRollup_; }

  ItemExpr * getExtraGrpOrderby() { return extraGrpOrderby_; }
  void setExtraGrpOrderby(ItemExpr *ie) { extraGrpOrderby_ = ie; }

//////////////////////////////////////////////////////

private:

  // ---------------------------------------------------------------------
  // An enumerated type that indicates the form for this GroupByAgg.
  // ---------------------------------------------------------------------
  enum GroupByAggFormEnum { FULL_GROUPBY,
                            PARTIAL_GROUPBY_ROOT,
	                    PARTIAL_GROUPBY_NON_LEAF,
	                    PARTIAL_GROUPBY_LEAF1,
	                    PARTIAL_GROUPBY_LEAF2
	                  };

  // ---------------------------------------------------------------------
  // Its form (see description above.)
  // ---------------------------------------------------------------------
  GroupByAggFormEnum formEnum_;

  // ---------------------------------------------------------------------
  // The expression specifying the group by columns/expressions
  // ---------------------------------------------------------------------
  ItemExpr    * groupExprTree_;
  ValueIdSet  groupExpr_;
  
  ItemExpr  * extraGrpOrderby_;
  ValueIdList  extraOrderExpr_;
  // --------------------------------------
  // used for processing groupby rollup
  // --------------------------------------
  ValueIdList rollupGroupExprList_;

  // ---------------------------------------------------------------------
  // The expression specifying the aggregates to be generated
  // ---------------------------------------------------------------------
  ItemExpr    * aggregateExprTree_;
  ValueIdSet  aggregateExpr_;

  // ---------------------------------------------------------------------
  // Indicates if this partial GB node has been pushed below a TSJ
  // ---------------------------------------------------------------------
  NABoolean gbAggPushedBelowTSJ_;

  // ---------------------------------------------------------------------
  // GBAnalysis defines the relation between this GroupBy and its JBB
  // ---------------------------------------------------------------------
  GBAnalysis  * gbAnalysis_;

  // indicates if there are any select list indices or renamed cols
  // used in the having clause.
  NABoolean selIndexInHaving_;
  // ---------------------------------------------------------------------
  // Flag used by subquery unnesting to determine if a GroupBy needs to move
  // up more than 1 level above a parent tsj.
  // ---------------------------------------------------------------------
  NABoolean requiresMoveUp_ ;

  // ---------------------------------------------------------------------
  // Flag used to determine is this Group By needs a seperate VEGRegion, 
  // if the groupExpr is empty (i.e. this is a scalar aggregate). Scalar 
  // aggregates that contain a null rejecting predicates do not need a
  // seperate VEGRegion. This is similar to logic used to determine if a 
  //left join can be converted to an inner join.
  // ---------------------------------------------------------------------
  NABoolean containsNullRejectingPredicates_ ;

  // ---------------------------------------------------------------------
  // Used by subquery unnesting only. During the SQO phase this member is
  // set to a list of values that gurantee uniqueness from the left side 
  // of the join/tsj that is being unnested. Later this value will be used 
  // during the MoveUpGroupBy transformation (part of SQO subpahse) as the 
  // groupExpr is adjusted to reflect the new position of this node in the
  // query tree. The groupExpr_ must always contains the columns listed in 
  // this member.
  // ---------------------------------------------------------------------
  ValueIdSet leftUniqueExpr_ ;

  // ---------------------------------------------------------------------
  // Used by "MVQR GroupBy Rewrite Rule" only. During GroupByAgg MVQR rewrite,
  // this flag will be used to not to fire MVQR GroupBy Rewrite Rule. 
  // MV candidates are saved in gbAnalysis of GroupBy RelExpr and it will 
  // be referenced by all applicables Rules in that CascadeGroup. 
  // Two such rules are AggrDistinctEliminationRule and GroupByOnJoinRule, 
  // which creates a new GroupByAgg expresion and puts that in a new CascadeGroup. 
  // When optimizer tries to optimize the new group, GroupByMVQRRule is also 
  // fired since this new expression references MVCandidates from the original 
  // groupBy expression of previous CascadeGroup, but this should be avoided.
  // When this flag is set TRUE, "MVQR GroupBy Rewrite Rule" will not be
  // fired.
  // ---------------------------------------------------------------------
  NABoolean aggDistElimRuleCreates_ ;
  NABoolean groupByOnJoinRuleCreates_;

  // ---------------------------------------------------------------------
  // Used by the Group By Ordinal feature, when renamed columns are 
  // present in groupby or having clauses. A ptr to the the select list of 
  // the parent root is saved here so that the select index can be 
  // interpreted locally in the groupby without accessing the parent root.
  // After use in GroupByAgg::bindNode, this datamember is set to NULL.
  // ---------------------------------------------------------------------
  ItemExpr * parentRootSelectList_;

  NABoolean isMarkedForElimination_;

  ValueIdSet aggrExprsToBeDeleted_;

  NABoolean isRollup_;
};

class SortGroupBy : public GroupByAgg
{
public:

  // constructor
  SortGroupBy(RelExpr *child,
		     OperatorTypeEnum otype = REL_ORDERED_GROUPBY,
		     ItemExpr *groupExpr = NULL,
		     ItemExpr *aggregateExpr = NULL,
		     CollHeap *oHeap = CmpCommon::statementHeap())
  : GroupByAgg(child,otype,groupExpr,aggregateExpr, oHeap) {}

  virtual ~SortGroupBy();

  virtual NABoolean isLogical () const;
  virtual NABoolean isPhysical() const;

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // context generation
  virtual void addArrangementAndOrderRequirements(RequirementGenerator &rg);

  // cost and physical property functions
  virtual CostMethod* costMethod() const;
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

}; // class SortGroupBy


// Logical Short Cut Group By
class ShortCutGroupBy : public GroupByAgg
{
public:

  // constructor
  ShortCutGroupBy(RelExpr *child,
		        OperatorTypeEnum otype = REL_SHORTCUT_GROUPBY,
		        ItemExpr *groupExpr = NULL,
		        ItemExpr *aggregateExpr = NULL,
			CollHeap *oHeap = CmpCommon::statementHeap())
  : GroupByAgg(child,otype,groupExpr,aggregateExpr, oHeap) {}

  virtual ~ShortCutGroupBy() {};

  virtual NABoolean isLogical () const { return TRUE; };
  virtual NABoolean isPhysical() const { return FALSE; };

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // can we apply MDAM-like access for the anytrue expression?
  inline NABoolean canApplyMdam() { return FALSE; }

  // accessor functions
  inline void setOptForMax (NABoolean value) { opt_for_max_ = value; }
  inline void setOptForMin (NABoolean value) { opt_for_min_ = value; }
  inline void setIsNullable (NABoolean value) { isnullable_ = value; }
  inline void set_lhs (ItemExpr * ptr) { lhs_anytrue_ = ptr; }
  inline void set_rhs (ItemExpr * ptr) { rhs_anytrue_ = ptr; }
  inline NABoolean isOptForMax() { return opt_for_max_;}
  inline NABoolean isOptForMin() { return opt_for_min_;}
  inline NABoolean isNullable()  { return isnullable_;}

  virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

protected:

  NABoolean opt_for_max_;   // anytrue lhs <, <= rhs
  NABoolean opt_for_min_;   // anytrue lhs >, >= rhs
  NABoolean isnullable_;    // true if anytrue expression contains nullable
                            //   columns
  ItemExpr * lhs_anytrue_;  // -> lhs
  ItemExpr * rhs_anytrue_;  // -> rhs

}; // class ShortCutGroupBy

// Physical Short Cut Group By
class PhysShortCutGroupBy : public ShortCutGroupBy
{
public:

  // constructor
  PhysShortCutGroupBy(RelExpr *child,
                             OperatorTypeEnum otype = REL_SHORTCUT_GROUPBY,
                             ItemExpr *groupExpr = NULL,
                             ItemExpr *aggregateExpr = NULL,
                             CollHeap *oHeap = CmpCommon::statementHeap())
  : ShortCutGroupBy(child,otype,groupExpr,aggregateExpr, oHeap) {}

  virtual ~PhysShortCutGroupBy() {};

  virtual NABoolean isLogical () const { return FALSE; };
  virtual NABoolean isPhysical() const { return TRUE; };

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // context generation
  virtual void addArrangementAndOrderRequirements(RequirementGenerator &rg);

  // cost and physical property functions
  virtual CostMethod* costMethod() const;
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

}; // class PhysShortCutGroupBy


class HashGroupBy : public GroupByAgg
{
public:

  // constructor
  HashGroupBy(RelExpr *child,
		     OperatorTypeEnum otype = REL_HASHED_GROUPBY,
		     ItemExpr *groupExpr = NULL,
		     ItemExpr *aggregateExpr = NULL,
		     CollHeap *oHeap = CmpCommon::statementHeap())
  : GroupByAgg(child,otype,groupExpr,aggregateExpr,oHeap) {}

  virtual ~HashGroupBy();

  virtual NABoolean isLogical () const;
  virtual NABoolean isPhysical() const;

  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  // cost and physical property functions
  virtual CostMethod* costMethod() const;
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  virtual RelExpr * preCodeGen(Generator *generator,
                               const ValueIdSet &externalInputs,
                               ValueIdSet &pulledNewInputs);


  // method to do code generation
  virtual short codeGen(Generator*);

  // The method gets refined since HGB may be a BMO depending on its inputs.
  virtual NABoolean isBigMemoryOperator(const PlanWorkSpace* pws,
                                        const Lng32 planNumber);

  virtual CostScalar getEstimatedRunTimeMemoryUsage(Generator *generator, NABoolean perNode, Lng32 *numStreams = NULL);

  virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

  ExpTupleDesc::TupleDataFormat determineInternalFormat( const ValueIdList & valIdList,
                                                           RelExpr * relExpr,
                                                           NABoolean & resizeCifRecord,
                                                           Generator * generator,
                                                           NABoolean bmo_affinity,
                                                           NABoolean & considerBufferDefrag);
}; // class HashGroupBy


#endif /* RELGRBY_H */
