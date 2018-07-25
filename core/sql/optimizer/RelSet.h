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
#ifndef RELSET_H
#define RELSET_H
/* -*-C++-*-
******************************************************************************
*
* File:         RelSet.h
* Description:  Relational set operations (both physical and logical operators)
* Created:      4/28/94
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "ObjectNames.h"
#include "RelExpr.h"
#include "Rel3GL.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class Union;
class Intersect;
class Except;

// The following are physical operators
class MergeUnion;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class LogicalProperty;
class BindWA;
class Generator;
class AssignmentStArea;

// -----------------------------------------------------------------------
// Intersect Operator
// -----------------------------------------------------------------------

class Intersect : public RelExpr
{
public:
  // constructor
  Intersect(RelExpr *leftChild,
	RelExpr *rightChild);

  // virtual destructor
  virtual ~Intersect();

  // get the degree of this node (it is a binary op).
  virtual Int32 getArity() const;

  const NAString getText() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL, CollHeap* outHeap = 0);

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);
};



// Except Operator
// -----------------------------------------------------------------------

class Except: public RelExpr
{
public:
  // constructor
  Except(RelExpr *leftChild,
	RelExpr *rightChild);

  // virtual destructor
  virtual ~Except();

  // get the degree of this node (it is a binary op).
  virtual Int32 getArity() const;

  const NAString getText() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL, CollHeap* outHeap = 0);

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);
};


// -----------------------------------------------------------------------
// Union Operator:
//
// What we call "Union" here is what is called "UNION ALL" in SQL. To
// achieve a true union a la Codd, a duplicate elimination (see class
// GroupByAgg) is needed above the union node.
// -----------------------------------------------------------------------
class UnionMap : public NABasicObject
{
  friend class Union;
  friend class MergeUnion;

  // The union map is a data struture that is shared by a number
  // of Union nodes that need to do the same mappings of values
  // from parent expressions to children expressions.
  // It is intended to be accessible only from Union methods.
private:

  // Constructor
  UnionMap() : count_(1)
  {}

  // copy ctor
  UnionMap (const UnionMap & rhs) :
       colMapTable_(rhs.colMapTable_),
       leftColMap_ (rhs.leftColMap_),
       rightColMap_(rhs.rightColMap_),
       count_(rhs.count_)
  {}

  // normalize all the expressions in the maps that are contributed
  // by the child, whose index is childIndex, in the map.
  void normalizeSpecificChild(NormWA & normWARef, Lng32 childIndex);

  // The map table contains a list of item expressions, where each item
  // expression is a ValueIdUnion that points to two Value Ids from the
  // left and right child. This list implements a mapping table indicating
  // how the columns of the left child and right child map the result
  // columns.
  ValueIdList colMapTable_;

  // two value id maps that contain the identical information as in
  // colMapTable_, for easier mapping
  ValueIdMap leftColMap_;
  ValueIdMap rightColMap_;

  // Reference Count
  Lng32 count_;
};

class AssignmentStHostVars;

class Union : public RelExpr
{

public:

  // constructor
  Union(RelExpr  *leftChild,
	RelExpr  *rightChild,
	UnionMap *colMapTable = NULL,
	ItemExpr *condExpr = NULL,
	OperatorTypeEnum otype = REL_UNION,
	CollHeap *outHeap = CmpCommon::statementHeap(),
    NABoolean sysGenerated = FALSE,
    NABoolean mayBeCacheable = TRUE
  );

  // copy ctor
  Union (const Union &) ; // not written

  // virtual destructor
  virtual ~Union();

  // get the degree of this node (it is a binary op).
  virtual Int32 getArity() const;

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // accessor functions
  // Provide a short-lived read-write access to the contents ofthe colMapTable_
  inline ValueIdList & colMapTable()             { return unionMap_->colMapTable_; }
  inline const ValueIdList & colMapTable() const { return unionMap_->colMapTable_; }
  inline ValueIdMap & getLeftMap()               { return unionMap_->leftColMap_; }
  inline ValueIdMap & getLeftMap() const         { return unionMap_->leftColMap_; }
  inline ValueIdMap & getRightMap()              { return unionMap_->rightColMap_; }
  inline ValueIdMap & getRightMap() const        { return unionMap_->rightColMap_; }
  ValueIdMap & getMap(Lng32 i)
  {
    CMPASSERT(i == 0 OR i == 1);
    if (i == 0) return unionMap_->leftColMap_;
    else return unionMap_->rightColMap_;
  }
  inline UnionMap * getUnionMap()       { return unionMap_; }

  void addValueIdUnion(ValueId vidUnion, CollHeap* heap);

  // This method rewrites unionExpr in terms of the values produced
  // produced by the left and the right children and returns
  // them in the sets exprForLeftChild and exprForRightChild
  // respectively.
  void rewriteUnionExpr(const ValueIdSet & unionExpr,
			ValueIdSet & exprForLeftChild,
			ValueIdSet & exprForRightChild) const;

  // Each operator supports a (virtual) method for transforming its
  // scalar expressions to a canonical form
  virtual void transformNode(NormWA & normWARef,
			     ExprGroupId & locationOfPointerToMe);

  // a method used during subquery transformation for pulling up predicates
  // towards the root of the transformed subquery tree
  virtual void pullUpPreds();

  // a method used for recomputing the outer references (external dataflow
  // input values) that are still referenced by each operator in the
  // subquery tree after the predicate pull up is complete.
  virtual void recomputeOuterReferences();

  // Each operator supports a (virtual) method for rewriting its
  // value expressions.
  virtual void rewriteNode(NormWA & normWARef);

  // Each operator supports a (virtual) method for performing
  // predicate pushdown and computing a "minimal" set of
  // characteristic input and characteristic output values.
  virtual RelExpr * normalizeNode(NormWA & normWARef);

  virtual RelExpr * semanticQueryOptimizeNode(NormWA & normWARef);

  // Method to push down predicates from a Union node into the
  // children
  virtual
  void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                           const ValueIdSet & newExternalInputs,
                           ValueIdSet& predOnOperator,
			   const ValueIdSet * nonPredExprOnOperator = NULL,
                           Lng32 childId = (-MAX_REL_ARITY) );

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // methods for common subexpressions
  virtual NABoolean prepareTreeForCSESharing(
       const ValueIdSet &outputsToAdd,
       const ValueIdSet &predicatesToRemove,
       const ValueIdSet &commonPredicatesToAdd,
       const ValueIdSet &inputsToRemove,
       ValueIdSet &valuesForVEGRewrite,
       ValueIdSet &keyColumns,
       CSEInfo *info);

  // ---------------------------------------------------------------------
  // Function for testing the eligibility of this
  // Union to form a plan using the given required physical properties.
  // This method is called by UnionRule::topMatch().
  // ---------------------------------------------------------------------
  NABoolean rppAreCompatibleWithOperator
              (const ReqdPhysicalProperty* const rppForUnion) const;

  // ---------------------------------------------------------------------
  // Union::createPartitioningRequirement()
  // It is not declared as a const method because it can supply the
  // ValueIdMap, colMapTable(), for copying and rewriting the
  // partitioning requirement.
  // ---------------------------------------------------------------------
  PartitioningRequirement* createPartitioningRequirement
                           (const ReqdPhysicalProperty* const rppForUnion,
			    Lng32 currentChildIndex);

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = CmpCommon::statementHeap());

  // method to do code generation
  virtual short codeGen(Generator*);

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

  // get a printable string that identifies the operator
  const NAString getText() const;

  // adds information about this node to the Explain tree
  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);


// merge of R1.5 to R2, renumbered
  enum UnionFlags {
    UNION_NONE		= 0x000,
    UNION_ORDERED	= 0x001,
    UNION_COND_UNARY	= 0x002,
    UNION_COND_EMPTY_IFELSE	= 0x004,        // set when ELSE part IF statement
                                                // is missing or empty
    UNION_FOR_IF	= 0x008,
    UNION_DISTINCT	= 0x010,		// Ansi SQL default, opp. of "UNION ALL"
    UNION_COND_EMPTY_IFTHEN = 0x020,            // set when we see
                                                // IF (expression) THEN
                                                //  BEGIN   END ;
                                                //  ELSE
                                                //  some sql statement/sqlstatements
                                                //  END IF ;
    UNION_BLOCKED       = 0x040    // UNION_BLOCKED was added for trigger project
  };

  void setSerialUnion()            {isSerialUnion_ = TRUE;}
   NABoolean getSerialUnion() const  {return isSerialUnion_;}

  void setOrderedUnion()	     {flags_ |= UNION_ORDERED;}
  NABoolean getOrderedUnion() const  {return (flags_ & UNION_ORDERED) != 0;}

  void setCondUnary()		     {flags_ |= UNION_COND_UNARY;}
  NABoolean getCondUnary() const     {return (flags_ & UNION_COND_UNARY) != 0;}

  void setCondEmptyIfElse()		     {flags_ |= UNION_COND_EMPTY_IFELSE;}
  NABoolean getCondEmptyIfElse() const     {return (flags_ &UNION_COND_EMPTY_IFELSE) != 0;}

  void setUnionForIF()		     {flags_ |= UNION_FOR_IF;}
  NABoolean getUnionForIF() const    {return (flags_ & UNION_FOR_IF) != 0; }

  void setDistinctUnion()	     {flags_ |= UNION_DISTINCT;}
  NABoolean getDistinctUnion() const {return (flags_ & UNION_DISTINCT) != 0;}

  void setCondEmptyIfThen()	     {flags_ |= UNION_COND_EMPTY_IFTHEN;}
  NABoolean getCondEmptyIfThen() const {return (flags_ & UNION_COND_EMPTY_IFTHEN) != 0;}

  void setUnionFlags(ULng32 f)	{flags_ = f;}
  ULng32 getUnionFlags() const	{return flags_;}
  // UNION_BLOCKED was added for trigger project
  inline void setBlockedUnion(){flags_ = UNION_BLOCKED;}
  inline Int32 getBlockedUnion(){return flags_ == UNION_BLOCKED;}



  // controlFlags were added for trigger project
  // setNoOutput can be applied to OrderUnion or BlockedUnion
  void setNoOutputs();
  inline Int32 hasNoOutputs(){return controlFlags_ & NO_OUTPUTS;}

  // inNotAtomicStmt is applied for unary unions to raise an error during execution
  // if an after trigger is enabled and we are executing a notAtomicStatement.
  void setInNotAtomicStatement()	     {controlFlags_ |= IN_NOT_ATOMIC_STMT;}
  NABoolean getInNotAtomicStatement() const {return (controlFlags_ & IN_NOT_ATOMIC_STMT) != 0;}

  // a blocked union is temporary if it is used to inline
  // a after row trigger (statement MVs too). This union will be
  // removed during optimization.
  void setIsTemporary()	     {controlFlags_ |= IS_TEMPORARY;}
  NABoolean getIsTemporary() const {return (controlFlags_ & IS_TEMPORARY) != 0;}

  inline void setControlFlags(Int32 flg){controlFlags_ = flg;}
  inline Int32 getControlFlags() const {return controlFlags_;}

  void addCondExprTree(ItemExpr *condExpr);
  ItemExpr *getCondExprTree();
  ItemExpr *removeCondExprTree();

  ValueIdList &condExpr()		{return condExpr_;}
  const ValueIdList &condExpr() const	{return condExpr_;}
  ValueIdList getCondExpr()		{return condExpr_;}
  void setCondExpr(ValueIdList condExpr)	{condExpr_ = condExpr;}

  // support methods for the Triggered Action Exception
  void addTrigExceptExprTree(ItemExpr *trigExceptExpr);
  ItemExpr *getTrigExceptExprTree();
  ItemExpr *removeTrigExceptExprTree();

  ValueIdList       &trigExceptExpr()		{return trigExceptExpr_;}
  const ValueIdList &trigExceptExpr() const	{return trigExceptExpr_;}
  ValueIdList       getTrigExceptExpr()		{return trigExceptExpr_;}
  void setTrigExceptExpr(ValueIdList trigExceptExpr)
                                      {trigExceptExpr_ = trigExceptExpr;}

  // These functions are to support functionality of compound statements.
  // Please see documentation of data members below.
  Union * getPreviousIF() { return previousIF_; }
  void setPreviousIF(Union * node) { previousIF_ = node; }
  AssignmentStHostVars *&leftList() { return leftList_; }
  AssignmentStHostVars *&rightList() { return rightList_; }
  short &currentChild() { return currentChild_; }
  AssignmentStHostVars *getCurrentList(BindWA *bindWA);

  // When we are in a CompoundStatement and we have IF statements in it,
  // we must create a RETDesc for this Union node (which is
  // actually an IF node). In this function, we create a list of
  // ValueIdUnion nodes. We figure out which valueids
  // of the left child must be matched with those of the right child
  // (for instance, if SET :a appears in both children) and which must
  // be matched with previously existing valueids (for instance, if
  // SET :a = ... only appears in one branch, then the ValueIdUnion associated
  // with that SET statement must reference the value id of :a that existed before
  // this IF statement).
  RETDesc * createReturnTable(AssignmentStArea *assignArea, BindWA *bindWA);

  //Used only for conditional union. Copies the leftlist and rightlist this conditional
  //union to the leftlist of the conditional union node pointed to by the previousIF argument,
  //if the calling conditional union is in the left subtree of the previousIF conditional union.
  //If the calling conditional union is in the right subtree of the previousIF conditional union
  //then we copy to the rightlist of the previousIF conditional union.
  void copyLeftRightListsToPreviousIF(Union * previousIF, BindWA * bindWA);


  // MV ++
  // MV refresh optimization refer to MVRefreshBuilder.cpp
  // MultiTxnMavBuilder::buildUnionBetweenRangeAndIudBlocks()
  // THIS OPTIMIZATION IS FOR MV REFRESH ONLY AND IS BASED ON THE KNOWLEDGE THAT RANGES RECORDS
  // IN THE RANGE LOG DO NOT OVERLAPPED
  void addAlternateRightChildOrderExprTree(ItemExpr *alternateRightChildOrderExprTree);
  ItemExpr *removeAlternateRightChildOrderExprTree();

  inline ValueIdList &alternateRightChildOrderExpr() {return alternateRightChildOrderExpr_;}
  inline const ValueIdList &alternateRightChildOrderExpr() const {return alternateRightChildOrderExpr_;}
  inline ValueIdList getAlternateRightChildOrderExpr() {return alternateRightChildOrderExpr_;}
  inline void setAlternateRightChildOrderExpr(ValueIdList alternateRightChildOrderExpr)
  {
    alternateRightChildOrderExpr_ = alternateRightChildOrderExpr;
  }
  // MV --

  // added for trigger project, to activate after trigger transformation
  virtual RelExpr * fixupTriggers(NABoolean & treeModified);

  // check for & warn against UNIONs that have inconsistent access/lock modes
  void checkAccessLockModes();

  // append an ascii-version of Union into cachewa.qryText_
  void generateCacheKey(CacheWA &cwa) const;

private:
 // controlFlags were added for trigger project
  enum controlFlags { NO_OUTPUTS = 0x0001,
		      IN_NOT_ATOMIC_STMT = 0x0002,
		      IS_TEMPORARY = 0x0004};

  // ---------------------------------------------------------------------
  // Union::createPartitioningRequirement()
  // It is not declared as a const method because it can supply the
  // ValueIdMap, equiJoinExpressions_, for copying and rewriting the
  // partitioning function to match. The rewrite requires a read-write
  // access to the ValueIdMap (interfaces supported by class ValueIdMap).
  // ---------------------------------------------------------------------
  PartitioningFunction* createPartitioningRequirement
                           (const ReqdPhysicalProperty* const rppForUnion,
                            const PartitioningFunction* const partFuncToMatch,
			    Lng32 currentChildIndex,
                            Lng32 currentPlanNumber);

  // MV --
  // A debugging method for dumping the columns in the RETDesc of both
  // children when they do not match.
  void dumpChildrensRETDescs(const RETDesc& leftTable,
		             const RETDesc& rightTable);

  // Conditional union expression.
  ItemExpr *condExprTree_;
  ValueIdList condExpr_;

  // expression used by triggeres to implement the ANSI
  // triggered action exception.
  ItemExpr *trigExceptExprTree_;
  ValueIdList trigExceptExpr_;

  // MV ++
  ItemExpr *alternateRightChildOrderExprTree_;
  ValueIdList alternateRightChildOrderExpr_;
  // MV --

  // The union map is shared by a number of Union nodes that were
  // transformed from one another
  UnionMap *unionMap_;

  // see unionFlags
  ULng32 flags_;

  // We keep a list of all variables that appear on the left side of a SET
  // statement and that appear below this Union node. Please see compound
  // statements documentation. In this way, we can keep track of SET statements
  // that appear within IF statements
  LIST(HostVar *) variablesSet_;

  // We keep a pointer to the closest Union node that is an ancestor of this
  // one. This is to support functionality for compound statements, in particular
  // for SET statements within IF statements.
  Union *previousIF_;

  // These lists contain all the variables that have been SET in the left and
  // right childs, respectively
  AssignmentStHostVars * leftList_;
  AssignmentStHostVars * rightList_;

  // Child we are currently visiting
  short currentChild_;

  //++ Triggers -
  // see controlFlags
  Int32 controlFlags_;

  // true iff this Union is system-generated (ie, is not user-specified)
  NABoolean isSystemGenerated_;

  NABoolean isSerialUnion_;
};

// -----------------------------------------------------------------------
// physical Union node
// -----------------------------------------------------------------------
class MergeUnion : public Union
{
public:

  // constructor
  MergeUnion(RelExpr *leftChild,
		    RelExpr *rightChild,
		    UnionMap *unionMap = NULL,
		    OperatorTypeEnum otype = REL_MERGE_UNION,
		    CollHeap *oHeap = CmpCommon::statementHeap())
  : Union(leftChild, rightChild, unionMap, NULL, otype, oHeap),
    mergeExpr_(NULL) {}

  // copy ctor
  MergeUnion (const MergeUnion &) ; // not written

  // virtual destructor
  virtual ~MergeUnion();

  virtual NABoolean isLogical () const;
  virtual NABoolean isPhysical() const;

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = CmpCommon::statementHeap());

  // Cascades-related functions
  virtual CostMethod* costMethod() const;
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);
  virtual NABoolean findOptimalSolution(Context* myContext,
                                        PlanWorkSpace* pws);
  virtual PhysicalProperty *synthPhysicalProperty(const Context *context,
                                                  const Lng32    planNumber,
                                                  PlanWorkSpace  *pws);

  // Helper function for plan generation.
  Context* createChildContext(Context* context, PlanWorkSpace* pws,
			      Lng32 childIndex);

  // handling sort order and merge expression
  inline const ValueIdList &getSortOrder()          { return sortOrder_; }
  void setSortOrder(const ValueIdList &newSortOrder);
  inline ItemExpr *getMergeExpr() const             { return mergeExpr_; }

  // method to do code generation
  virtual RelExpr * preCodeGen(Generator * generator,
			       const ValueIdSet & externalInputs,
			       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  // -----------------------------------------------------
  // generate CONTROL QUERY SHAPE fragment for this node.
  // -----------------------------------------------------
  virtual short generateShape(CollHeap * space, char * buf, NAString * shapeStr = NULL);

  // add all the expressions that are local to this
  // node to an existing list of expressions (used by GUI tool)
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const;

// Given an input synthesized child sort key, a required arrangement,
// and a input required sort order (if any), generate a new sort key
// that has all the expressions in the required sort key and required
// arrangement, and only those expressions, in an order that matches
// the child sort key.  Return this new sort key, which can then be used
// as a sort key requirement for the other child of the union or as the
// synthesized union sort key after being mapped.
  void synthUnionSortKeyFromChild (const ValueIdList &childSortKey,
                                   const ValueIdSet &reqArrangement,
                                   ValueIdList &unionSortKey) const;

  // -----------------------------------------------------------------------
  // Performs mapping on the partitioning function, from the union to the
  // designated child.
  // -----------------------------------------------------------------------
  virtual PartitioningFunction* mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0) ;

  // -----------------------------------------------------------------------
  // This help function tries to make the Union sort key the same
  // length as the arrangement requirement of Union cols. The key
  // can be shorter when some of the Union columns are duplicated.
  //
  // -----------------------------------------------------------------------
  NABoolean finalizeUnionSortKey(const ValueIdList& knownSortKey,  // IN
                            Lng32 knownSide,                   // IN
                            const ValueIdSet& arrCols,        // IN
                            ValueIdList& unionSortKey);       // IN&OUT

private:

  // the sorting order of the union result (if any), expressed in terms
  // of the union's outputs
  ValueIdList sortOrder_;

  // The merge expression is a boolean expression telling which of the
  // children to read next (TRUE means read the left child, FALSE means
  // read the right child). It is used to merge two sorted child streams
  // into a sorted output stream if the expression is (left value <= right).
  ItemExpr *mergeExpr_;

  // a private method to build the merge expression from the sort order
  void buildMergeExpr();
};

#endif /* RELSET_H */





