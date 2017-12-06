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
#ifndef RELJOIN_H
#define RELJOIN_H
/* -*-C++-*-
******************************************************************************
*
* File:         RelJoin.h
* Description:  Relational joins (both physical and logical operators)
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
#include "ReqGen.h"
#include "RelGrby.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class Join;

// The following are physical operators
class NestedJoin;
class NestedJoinFlow;
class MergeJoin;
class HashJoin;
class MapTable;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class LogicalProperty;
class BindWA;
class NormWA;
class Generator;
class ex_expr;
class ex_cri_desc;
////////////////////
class QueryAnalysis;
class JBB;
class CANodeIdSet;
////////////////////
class GenericUpdate;

// -----------------------------------------------------------------------
// Join Operator:
//
// The join node can be used to represent either a logical join operator
// or the base class for physical join operators.  Objects of this class
// should have one of the join operator types REL_JOIN, REL_TSJ,
// REL_SEMIJOIN, REL_SEMITSJ. They are logical operators.
//
// The distinguishing characteristic of the REL_TSJ is that its right subtree
// references values that are produced by its left subtree. Whereas, this is
// not so for the REL_JOIN. A REL_JOIN can therefore be implemented using
// either of the  nested loops, hash or merge join methods. A REL_TSJ, on the
// other hand, can only be implemented by the nested loops join method.
//
// The REL_SEMIJOIN is a semijoin. A probe value from its left subtree finds
// at most one match in its right subtree. Furthermore, the right subtree
// does not contribute any values as output. The REL_SEMITSJ has the flavour
// of a REL_TSJ with the restriction that it is a semijoin and not a join.
//
// Join predicates:
//
// Before predicates are pushed down from a logical join operator,
// predicates from the ON clause are represented as join predicates;
// those from the WHERE clause are represented as selection predicates.
//
// For physical join nodes, join predicates represent those that are
// evaluated to determine whether rows qualify for the join.  Selection
// predicates are any additional predicates evaluated on the result of
// the join.  In the case of left joins, these would be predicates
// evaluated on potentially null-instantiated rows.  A nested join
// (or TSJ) has no join predicates on the join node. The join predicate
// is evaluated on the scan of the inner table.
// -----------------------------------------------------------------------

class Join : public RelExpr
{

public:

  // constructor
  Join(RelExpr *leftChild,
              RelExpr *rightChild,
              OperatorTypeEnum otype = REL_JOIN,
              ItemExpr *joinPred = NULL,
              NABoolean isNaturalJoin = FALSE,
              NABoolean isTransformComplete = FALSE,
              CollHeap *oHeap = CmpCommon::statementHeap(),
              TableDesc *updateTableDesc = NULL,
              ValueIdMap *updateSelectValueIdMap = NULL)
  : RelExpr(otype, leftChild, rightChild, oHeap)
  , joinPredTree_(joinPred)
  , isNaturalJoin_(isNaturalJoin)
  , transformComplete_(isTransformComplete)
  , updateTableDesc_(updateTableDesc)
  , updateSelectValueIdMap_(updateSelectValueIdMap)
  , leftHasUniqueMatches_(FALSE)
  , rightHasUniqueMatches_(FALSE)
  , considerTSJ_(TRUE)
  , tsjAfterSQO_(FALSE)
  , tsjForWrite_(FALSE)
  , tsjForUndo_(FALSE)
  , tsjForSetNFError_(FALSE)
  , tsjForMerge_(FALSE)
  , tsjForMergeWithInsert_(FALSE)
  , tsjForMergeUpsert_(FALSE)
  , derivedFromRoutineJoin_(FALSE)
  , joinFromPTRule_(FALSE)
  , joinFromMJSynthLogProp_(FALSE)
  , joinForZigZag_(FALSE)
  // QSTUFF
  , cursorUpdate_(FALSE)
  // QSTUFF
  , forcePhysicalJoinType_(NO_FORCING)
  , rowsetRowCountArraySize_(0)
  , avoidHalloweenR2_(FALSE)
  , sourceType_(GENERAL)
  , candidateForSubqueryUnnest_(FALSE)
  , candidateForSubqueryLeftJoinConversion_(FALSE)
  , candidateForSemiJoinTransform_(FALSE)
  , halloweenForceSort_(NO_SELF_REFERENCE)
  , floatingJoin_(FALSE)
  , isIndexJoin_(FALSE)
  , allowPushDown_(TRUE)
  , tsjForSideTreeInsert_(FALSE)
  , enableTransformToSTI_(FALSE)
  , isForTrafLoadPrep_(FALSE)
  , beforeJoinPredOnOuterOnly_(FALSE)
  { }

  // copy ctor
  Join (const Join &) ; // not written

  // virtual destructor
  virtual ~Join() {}

  // get the degree of this node (it is a binary op).
  virtual Int32 getArity() const;

  // append an ascii-version of RelExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // change literals of a cacheable query into ConstantParameters
  virtual RelExpr* normalizeForCache(CacheWA& cwa, BindWA& bwa);

  // set and augment and destructively get the join predicate as parse tree
  void setJoinPredTree(ItemExpr *joinPred)      { joinPredTree_ = joinPred; }
  void addJoinPredTree(ItemExpr *joinPred);
  ItemExpr * removeJoinPredTree();

  ItemExpr * getJoinPredTree() {return joinPredTree_; }

  void setJoinPred(const ValueIdSet & joinPred) { joinPred_ += joinPred; };

  // return a (short-lived) read/write reference to the join predicate
  inline ValueIdSet & joinPred() { return joinPred_; }

  // return a constant reference to the join predicate
  inline const ValueIdSet & getJoinPred() const { return joinPred_; }

  // This join should not be transformed to TSJ later (for performance
  // reasons only). If already been transformed, that's fine.
  inline void doNotTransformToTSJ() {considerTSJ_ = FALSE; }

  // return TRUE if OK to transform to TSJ (performance reasons only)
  inline NABoolean canTransformToTSJ() { return considerTSJ_; }

  // return TRUE if this is a natural join
  NABoolean isNaturalJoin() const { return isNaturalJoin_; }

  // return TRUE if this is an inner join
  NABoolean isInnerJoin() const;

  // return TRUE if this is a cross product
  NABoolean isCrossProduct() const;

  // return TRUE is this join can be part of a JBB
  NABoolean isOuterJoin() const;

  // return TRUE if this is a inner non-semi join
  // with no join or selection predicates. This translates
  // into a "strong" cross product. This method will return TRUE only
  // if the Join::isCrossProduct() method returns TRUE.
  // Join::isCrossProduct will return TRUE if the join or
  // selection predicates contains only predicates other
  // than VEG predicates. But this method will return FALSE.
  NABoolean isInnerNonSemiJoinWithNoPredicates() const;

  // the following five are mutually exlusive (only one of them is TRUE)
  NABoolean isInnerNonSemiJoin() const;
  NABoolean isInnerNonSemiNonTSJJoin() const;
  NABoolean isLeftJoin() const;
  NABoolean isRightJoin() const;
  NABoolean isFullOuterJoin() const;
  NABoolean isSemiJoin() const;
  NABoolean isAntiSemiJoin() const;
  NABoolean ownsVEGRegions() const;

  // independent of the five functions above, is this a TSJ ( a join
  // whose right child can see the values produced by the left child)
  // returns true for REL_ANY_TSJ, including REL_ROUTINE_JOIN
  NABoolean isTSJ() const;

  // returns true for REL_ROUTINE_JOIN
  NABoolean isRoutineJoin() const;

  // returns true for REL_ANY_TSJ, except REL_ROUTINE_JOIN
  NABoolean isNonRoutineTSJ() const;

  NABoolean derivedFromRoutineJoin() const { return derivedFromRoutineJoin_; }
  void setDerivedFromRoutineJoin() { derivedFromRoutineJoin_ = TRUE;}
  
  // What type of join method is used?
  NABoolean isHashJoin() const;
  OperatorTypeEnum getBaseHashType() const;

  NABoolean isNestedJoin() const;
  NABoolean isMergeJoin() const;

  // Flag to indicate that this is an IndexJoin
  NABoolean isIndexJoin() const { return isIndexJoin_; }
  void setIsIndexJoin() { isIndexJoin_ = TRUE; }

  NABoolean isFloatingJoin() const { return floatingJoin_; }
  void setFloatingJoin() { floatingJoin_ = TRUE; }


  NABoolean beforeJoinPredOnOuterOnly() const
  { return beforeJoinPredOnOuterOnly_; }
  void setBeforeJoinPredOnOuterOnly() { beforeJoinPredOnOuterOnly_ = TRUE; }

  // Accessor method for returning any required order that was specified.
  // If there was a required order, it could only have come from a
  // insert statement that specified an ORDER BY clause. The required
  // order would be transferred to a TSJ or TSJFlow node when the TSJ
  // or TSJFlow rule fired.
  const ValueIdList & getReqdOrder() const { return reqdOrder_; }
  // Mutator for the the required order. Called by the TSJ or TSJFlow rule.
  void setReqdOrder(ValueIdList reqdOrder)
    { reqdOrder_ = reqdOrder;  }

  // a virtual function for performing name binding within the query tree
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // MV --
  // We currently want to block LOJ queries, although they are incremental
  NABoolean virtual isIncrementalMV() { return !isLeftJoin(); } // was return TURE;
  void virtual collectMVInformation(MVInfoForDDL *mvInfo,
									NABoolean isNormalized);

  // null instantiated part of the output list
  ValueIdList & nullInstantiatedOutput() { return nullInstantiatedOutput_; }
  const ValueIdList & nullInstantiatedOutput() const { return nullInstantiatedOutput_; }

  // null instantiated part of the output list for RIGHT JOIN
  ValueIdList & nullInstantiatedForRightJoinOutput()
  { return nullInstantiatedForRightJoinOutput_; }
  const ValueIdList & nullInstantiatedForRightJoinOutput() const
  { return nullInstantiatedForRightJoinOutput_; }
  ValueId addNullInstIndicatorVar(BindWA *bindWA,
                                  ItemExpr *indicatorVal = NULL);

//++MV
  // Used for translating the required sort key to the right
  // child sort key and backwards
  // For more information see NestedJoin::synthPhysicalProperty()
  ValueIdMap & rightChildMapForLeftJoin() { return rightChildMapForLeftJoin_; }
  const ValueIdMap & rightChildMapForLeftJoin() const { return rightChildMapForLeftJoin_; }
  void BuildRightChildMapForLeftJoin();

  // Building similar information for right joins.
  // Used for translating the required sort key to the right
  // child sort key and backwards
  // For more information see NestedJoin::synthPhysicalProperty()
  ValueIdMap & leftChildMapForRightJoin() { return leftChildMapForRightJoin_; }
  const ValueIdMap & leftChildMapForRightJoin() const
  { return leftChildMapForRightJoin_; }
  void BuildLeftChildMapForRightJoin();

  GenericUpdate* getFirstIUDNode(RelExpr* currentNode);

//--MV

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

  enum TransformationType
    { NO_TRANSFORMATION = 0,
      SEMI_JOIN_TO_INNER_JOIN,
      UNNESTING };
  // A left-linear tree of Inner Joins is one in which no Inner Join
  // has another Inner Join as its right child. This method implements
  // a transformation rule that produces a left-linear tree of Inner Joins.
  // It replaces, if possible, T1 IJ (T2 IJ T3) with a left-linear sequence
  // T1 IJ T2 IJ T3.
  Join * leftLinearizeJoinTree(NormWA & normWARef, 
                               TransformationType transformationType = NO_TRANSFORMATION);

  // Reorder the join tree based on increasing estimated rowcount,
  //   i.e. join smallest tables first.
  virtual RelExpr * reorderTree(NABoolean & treeModified,
	  NABoolean doReorderJoin);

  // Each operator supports a (virtual) method for rewriting its
  // value expressions.
  virtual void rewriteNode(NormWA & normWARef);

  // Each operator supports a (virtual) method for performing
  // predicate pushdown and computing a "minimal" set of
  // characteristic input and characteristic output values.
  virtual RelExpr * normalizeNode(NormWA & normWARef);

  // used by subquery unnesting. This method creates a Filter node above
  // node Y where Y is given by TSJ(X,GroupBy(Y)), for all TSJs introduced
  // by the Join-Aggregate transformation during subquery transformation.
  // This filter node is created only if the node Y contains outer references
  // The purpose of the filter node is to prevent pushdown of outer references
  // during the normalize phase.
  void createAFilterGrandChildIfNeeded(NormWA & normWARef) ;

  // subqueries are unnested in this method
  virtual RelExpr * semanticQueryOptimizeNode(NormWA & normWARef);


  // A normalizer method for determining whether a left join can be
  // transformed to an inner join.
  NABoolean canConvertLeftJoinToInnerJoin(NormWA & normWARef) ;

  // Methods used for outer join transformations in the normalizer
  void convertToNotOuterJoin();

  // A method for converting a tsj to an ordinary join
  void convertToNotTsj();
  // A method for converting an ordinary join to a tsj
  void convertToTsj();

  void computeRequiredResources(RequiredResources & reqResources,
                                       EstLogPropSharedPtr & inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));
  virtual void computeMyRequiredResources(RequiredResources & reqResources,
                                      EstLogPropSharedPtr & inLP);

  // Map tables: Map a logical op type to a physical/logical op type.
  //             Used by implementation/transformation rules.
  // What should my operator type translate to if a TSJ/nested/merge/hash
  // join implementation is chosen for me?
  OperatorTypeEnum getTSJJoinOpType();
  OperatorTypeEnum getNestedJoinOpType();
  OperatorTypeEnum getHashJoinOpType(NABoolean isNoOverflow = false);
  OperatorTypeEnum getMergeJoinOpType();

  // helper function used in 
  //   HashJoinRule::topMatch() and 
  //   JoinToTSJRule::topMatch()
  // to make sure that only one of them is turned off
  NABoolean allowHashJoin();

  // MV --
  // Allow forcing the physical implementation of the Join.
  enum PhysicalJoinType
    { NO_FORCING = 0,
      NESTED_JOIN_TYPE,
      HASH_JOIN_TYPE,
      MERGE_JOIN_TYPE,
      TSJ_JOIN_TYPE };
  void forcePhysicalJoinType(PhysicalJoinType type)
    { forcePhysicalJoinType_ = type; }
  NABoolean isPhysicalJoinTypeForced() const
    { return forcePhysicalJoinType_ != NO_FORCING; }
  PhysicalJoinType getForcedPhysicalJoinType() const
    { return forcePhysicalJoinType_; }

  // Used to determine the origin of special joins such as the ones produced
  // by the LSRs
  enum JoinSourceType
    { GENERAL = 0,     // The general case. The source was not specified
      STAR_FACT,       // Type1 star join plan and this join to the fact table as inner
      STAR_KEY_JOIN,   // Type1 star join plan and this join dimension key-join tables
      STAR_FILTER_JOIN // Type1 star join plan and this join dimension table after the fact
    };
  void setSource(JoinSourceType type)
    { sourceType_ = type; }
  JoinSourceType getSource() const
    { return sourceType_; }


  // Indicates whether we should apply any transformation rules
  // on this JOIN subtree.
  NABoolean isTransformComplete() const { return transformComplete_; }
  void setTransformComplete() { transformComplete_ = TRUE; }

  // Method to push down predicates from a join node into the
  // children
  virtual
  void pushdownCoveredExpr(const ValueIdSet & outputExprOnOperator,
                           const ValueIdSet & newExternalInputs,
                           ValueIdSet& predOnOperator,
			   const ValueIdSet * nonPredExprOnOperator = NULL,
                           Lng32 childId = (-MAX_REL_ARITY) );

  void pushdownCoveredExprSQO(const ValueIdSet & outputExpr,
                           const ValueIdSet & newExternalInputs,
                           ValueIdSet& predOnOperator,
			   ValueIdSet & nonPredExprOnOperator,
                           NABoolean keepPredsNotCoveredByChild0,
			   NABoolean keepPredsNotCoveredByChild1);

  // The set of values that I can potentially produce as output.
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  // add local predicates for the purposes of GUI display
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const RelExpr & other) const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  // synthesize constraints
  void synthConstraints(NormWA * normWAPtr);

  // synthesize estimated logical properties
  void synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp);

  CostScalar
  doCardSanityChecks(const EstLogPropSharedPtr& inLP,
                     ColStatDescList & joinStatDescList,
                     CostScalar newRowCount);

  CostScalar
  estimateMaxCardinality(const EstLogPropSharedPtr& inLP,
                         ColStatDescList & joinStatDescList);

  CostScalar computeMinEstRCForGroup();

  // get the highest reduction from local predicates for cols of this join
  CostScalar highestReductionForCols(ValueIdSet colSet) ;

  // synthesize estimated log. properties for TSJ operators only
  void synthEstLogPropForTSJ (const EstLogPropSharedPtr& inputEstLogProp);

  // methods to split the order and arrangement req between the
  // two join childs.
  NABoolean splitOrderReq(const ValueIdList& myOrderReq, /*IN*/
                          ValueIdList& orderReqOfChild0, /*OUT*/
                          ValueIdList& orderReqOfChild1  /*OUT*/) const;

  NABoolean splitArrangementReq(const ValueIdSet& myArrangReq, /*IN*/
                                ValueIdSet& ArrangReqOfChild0, /*OUT*/
                                ValueIdSet& ArrangReqOfChild1  /*OUT*/) const;

  // ---------------------------------------------------------------------
  // Split any sort or arrangement requirements between the left and
  // right childs. Add those that are for the left child to the
  // passed in requirement generator object, and return those that
  // are for the right child.
  // ---------------------------------------------------------------------
  void splitSortReqsForLeftChild(const ReqdPhysicalProperty* rppForMe,
                                 RequirementGenerator &rg,
                                 ValueIdList& reqdOrder1,
                                 ValueIdSet& reqdArr1) const;

  // ---------------------------------------------------------------------
  // Split any sort or arrangement requirements between the left and
  // right childs. Add those that are for the right child to the
  // passed in requirement generator object, and return those that
  // are for the left child.
  // ---------------------------------------------------------------------
  void splitSortReqsForRightChild(const ReqdPhysicalProperty* rppForMe,
                                  RequirementGenerator &rg,
                                  ValueIdList& reqdOrder0,
                                  ValueIdSet& reqdArr0) const;

  // ---------------------------------------------------------------------
  // Methods used by the optimizer for equijoin predicates.
  // ---------------------------------------------------------------------
  // When synthesising logical properties
  void findEquiJoinPredicates();
  // By an implementation rule
  void separateEquiAndNonEquiJoinPredicates
           (const NABoolean joinStrategyIsOrderSensitive = FALSE);

  const ValueIdSet& getEquiJoinPredicates() const
                                           { return equiJoinPredicates_; }

  ValueIdMap getEquiJoinExpressions() const
                                           { return equiJoinExpressions_; }

  const ValueIdList& getEquiJoinExprFromChild0() const
                           { return equiJoinExpressions_.getTopValues(); }

  const ValueIdList& getEquiJoinExprFromChild1() const
                        { return equiJoinExpressions_.getBottomValues(); }

  const ValueIdSet& getPredicatesToBeRemoved() const
                                           { return predicatesToBeRemoved_; }

  const ValueIdMap& getOriginalEquiJoinExpressions() const
                        { return originalEquiJoinExpressions_; }
  void setOriginalEquiJoinExpressions(const ValueIdMap& x)
                        { originalEquiJoinExpressions_ = x; }

  // data members for joins that are parents of an update
  TableDesc * updateTableDesc()               { return updateTableDesc_; }
  void setUpdateTableDesc(TableDesc *utd) { updateTableDesc_ = utd;}
  ValueIdMap * updateSelectValueIdMap() { return updateSelectValueIdMap_;}
  void setUpdateSelectValueIdMap(ValueIdMap * m) { updateSelectValueIdMap_ = m;}

  // After the logical properties of a join have been synthesized
  // we can ask if the a each from the first or second child is
  // guaranteed to have a single match from the other child.
  NABoolean rowsFromLeftHaveUniqueMatch() const
                 { return leftHasUniqueMatches_; };
  NABoolean rowsFromRightHaveUniqueMatch() const
                 { return rightHasUniqueMatches_; };
  NABoolean eitherChildHasUniqueMatch() const
                 { return leftHasUniqueMatches_ | rightHasUniqueMatches_; };

  // When commuting a join the left and right private members need
  // to be exchanged of fliped.
  void flipChildren();

  inline void setTSJAfterSQO() { tsjAfterSQO_ = TRUE;}
  inline NABoolean isTSJAfterSQO() {return tsjAfterSQO_;}
  
  // Accessor for the tsjForWrite flag.
  NABoolean isTSJForWrite() const { return tsjForWrite_; }
  // Mutator for the tsjForWrite flag.
  void setTSJForWrite(NABoolean tsjForWrite)
    { tsjForWrite_ = tsjForWrite;  }

  // Accessor for the allowPushDown flag.
  NABoolean allowPushDown() const { return allowPushDown_; }
  // Mutator for the allowPushDown flag.
  void setAllowPushDown(NABoolean allowPushDown)
    { allowPushDown_ = allowPushDown;  }

  // Accessor for the tsjForUndo flag.
  NABoolean isTSJForUndo() const { return tsjForUndo_; }
  // Mutator for the tsjForUndo flag.
  void setTSJForUndo(NABoolean tsjForUndo)
    { tsjForUndo_ = tsjForUndo;  }
  // Accessor for the tsjForSetNFError flag.
  NABoolean isTSJForSetNFError() const { return tsjForSetNFError_; }
  // Mutator for the tsjForSetNFError flag.
  void setTSJForSetNFError(NABoolean tsjForSetNFError)
    { tsjForSetNFError_ = tsjForSetNFError;  }
  // Accessor for the tsjForMerge flag.
  NABoolean isTSJForMerge() const { return tsjForMerge_; }
  // Mutator for the tsjForMerge flag.
  void setTSJForMerge(NABoolean tsjForMerge)
    { tsjForMerge_ = tsjForMerge;  }

  // Accessor for the tsjForMergeWithInsert flag.
  NABoolean isTSJForMergeWithInsert() const { return tsjForMergeWithInsert_; }
  // Mutator for the tsjForMergeWithInsert flag.
  void setTSJForMergeWithInsert(NABoolean tsjForMergeWithInsert)
    { tsjForMergeWithInsert_ = tsjForMergeWithInsert;  }

   NABoolean isTSJForMergeUpsert() const { return tsjForMergeUpsert_; }
  // Mutator for the tsjForMergeUpsert flag.
  void setTSJForMergeUpsert(NABoolean tsjForMergeUpsert)
    { tsjForMergeUpsert_ = tsjForMergeUpsert;  }

  // Accessor for the tsjForSideTree flag.
  NABoolean isTSJForSideTreeInsert() const { return tsjForSideTreeInsert_; }
  // Mutator for the tsjForSideTreeInsert flag.
  void setTSJForSideTreeInsert(NABoolean tsjForSideTreeInsert)
    { tsjForSideTreeInsert_ = tsjForSideTreeInsert;  }

  NABoolean enableTransformToSTI() const { return enableTransformToSTI_;}
  void setEnableTransformToSTI(NABoolean v)
  { enableTransformToSTI_ = v; }

  // ---------------------------------------------------------------------
  // A method that interprets the CONTROL QUERY SHAPE ... to decide
  // whether a matching partitions plan or a replicate child1 plan
  // is desired by the user.
  // ---------------------------------------------------------------------
  JoinForceWildCard::forcedPlanEnum getParallelJoinPlanToEnforce
         (const ReqdPhysicalProperty* const rppForJoin) const;

  // ---------------------------------------------------------------------
  // A virtual function that decides if ESP parallelism is allowed
  // by the settings in the defaults table, or if the number of
  // ESPs is being forced for the join operator.
  // ---------------------------------------------------------------------
  virtual DefaultToken getParallelControlSettings (
            const ReqdPhysicalProperty* const rppForMe, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) const;

  // ---------------------------------------------------------------------
  // Pre-code generation.
  // ---------------------------------------------------------------------
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet &externalInputs,
                               ValueIdSet &pulledNewInputs);

  // -----------------------------------------------------
  // generate CONTROL QUERY SHAPE fragment for this node.
  // -----------------------------------------------------
  virtual short generateShape(CollHeap * space, char * buf, NAString * shapeStr = NULL);

  enum ParallelJoinTypeDetail
    { PAR_NONE = 0, // no additional detail, vanilla join
      PAR_SB,       // Skew Buster (reported as type 1, but really mix of type 1 and 2
      PAR_OCB,      // OCB - Outer Child Broadcast, TYPE2 with reversed roles
      PAR_OCR,      // OCR - Outer Child Repartitioning, TYPE1 nested join
      PAR_N2J       // N**2 opens join, nested parallel  TYPE2 non-OCB join
    };

  // use only after phys props have been synthesized
  // and expression is no longer in MEMO
  Int32 getParallelJoinType(ParallelJoinTypeDetail *optionalDetail = NULL) const;

  // special handling for the case values are to be instantiated.
  short instantiateValuesForLeftJoin(Generator * generator,
                                     short atp, short atp_index,
                                     ex_expr ** lj_expr,
                                     ex_expr ** ni_expr,
                                     ULng32 * rowlen,
				     MapTable ** newMapTable,
				     ExpTupleDesc::TupleDataFormat tdf = ExpTupleDesc::UNINITIALIZED_FORMAT);

  // special handling for the case values are to be instantiated.
  short instantiateValuesForRightJoin(Generator * generator,
                                     short atp, short atp_index,
                                     ex_expr ** rj_expr,
                                     ex_expr ** ni_expr,
                                     ULng32 * rowlen,
				     MapTable ** newMapTable,
				     ExpTupleDesc::TupleDataFormat tdf = ExpTupleDesc::UNINITIALIZED_FORMAT);


  // get a printable string that identifies the operator
  virtual const NAString getText() const;

  // adds Explain information to this node. Implemented in
  // Generator/GenExplain.C

  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);


  // QSTUFF
  inline void setCursorUpdate(NABoolean b) { cursorUpdate_ = b; }
  inline NABoolean isCursorUpdate() { return cursorUpdate_; }
  // QSTUFF

  inline void setRowsetRowCountArraySize(Lng32 b) { rowsetRowCountArraySize_ = b; }
  inline Lng32 getRowsetRowCountArraySize() { return rowsetRowCountArraySize_; }

//////////////////////////////////////////////////////
  virtual NABoolean pilotAnalysis(QueryAnalysis* qa);
  virtual NABoolean isASpoilerJoin();
  virtual NABoolean canBePartOfJBB();
  virtual void jbbAnalysis(QueryAnalysis* qa);
  virtual void jbbJoinDependencyAnalysis(ValueIdSet & predsWithDependencies);
  virtual void predAnalysis(QueryAnalysis* qa);
  virtual RelExpr* convertToMultiJoinSubtree(QueryAnalysis* qa);
  virtual EstLogPropSharedPtr setJBBInput(EstLogPropSharedPtr & inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));
  void analyseJoinBackBone(JBB* jbb);
  virtual void primeGroupAnalysis();

  // read comment about joinFromPTRule_ member
  inline NABoolean isJoinFromPTRule() const { return joinFromPTRule_; }
  inline void setJoinFromPTRule(NABoolean b = TRUE) { joinFromPTRule_ = b; }

  // read comment about joinFromPTRule_ member
  inline NABoolean isJoinFromMJSynthLogProp() const { return joinFromMJSynthLogProp_; }
  inline void setJoinFromMJSynthLogProp(NABoolean b = TRUE) { joinFromMJSynthLogProp_ = b; }

  inline NABoolean isJoinForZigZag() const { return joinForZigZag_; }
  inline void setJoinForZigZag(NABoolean b = TRUE) { joinForZigZag_ = b; }

  inline NABoolean avoidHalloweenR2() const
  { return avoidHalloweenR2_; }
  inline void setAvoidHalloweenR2(NABoolean b = TRUE)
  { avoidHalloweenR2_ = b; }

  enum HalloweenJoinSortType {
    NO_SELF_REFERENCE,
    NOT_FORCED,
    FORCED
  };

  inline void setHalloweenForceSort(HalloweenJoinSortType h)
  { halloweenForceSort_ = h; }

  inline HalloweenJoinSortType getHalloweenForceSort() const
  { return halloweenForceSort_; }

  
  inline NABoolean candidateForSubqueryUnnest() const 
  { return candidateForSubqueryUnnest_; }
  inline void setCandidateForSubqueryUnnest(NABoolean b) 
  { candidateForSubqueryUnnest_ = b; }

  inline NABoolean candidateForSubqueryLeftJoinConversion() const 
  { return candidateForSubqueryLeftJoinConversion_; }
  inline void setCandidateForSubqueryLeftJoinConversion(NABoolean b) 
  { candidateForSubqueryLeftJoinConversion_ = b; }

  inline NABoolean candidateForSemiJoinTransform() const 
  { return candidateForSemiJoinTransform_; }
  inline void setCandidateForSemiJoinTransform(NABoolean b) 
  { candidateForSemiJoinTransform_ = b; }

 // matches and flows compRefOpt constraints up the query tree. 
  virtual void processCompRefOptConstraints(NormWA * normWAPtr) ;

  NABoolean allowPushDown_;
  // ----------------------------------------------------------------------
  // A method for normalizing the operands of an InstantiateNull operator
  // that appears in the nullInstantiatedOutput_. A special method is
  // necessary to prevent an InstantiateNull that appears in this list
  // from being replaced with a VEGReference for the VEG to which it
  // belongs.
  // ----------------------------------------------------------------------
  void normalizeNullInstantiatedOutput(NormWA &);

  // SQO methods

  // used to eliminate redundant joins during SQO
  RelExpr* eliminateRedundantJoin(NormWA &normWARef);

  // used to transform semijoins to innerjoins during SQO
  RelExpr* transformSemiJoin(NormWA& NormWARef); 

  // used to pull up preds that reference aggrs from this join
  // to the grby that is being moved up.
  NABoolean pullUpPredsWithAggrs(GroupByAgg* grbyNode, MapValueIds *mapNode=NULL);

  // one of two main transformations for subquery unnesting
  GroupByAgg* pullUpGroupByTransformation(NormWA& NormWARef);

  // the other main transformation for subquery unnesting
  GroupByAgg* moveUpGroupByTransformation(GroupByAgg* newGrby, 
					  NormWA & normWARef);

  // heuristic to unnest subquery if inner table has keyed access.
  NABoolean applyInnerKeyedAccessHeuristic(const GroupByAgg* newGrby, 
					       NormWA & normWARef);

  virtual NABoolean prepareMeForCSESharing(
       const ValueIdSet &outputsToAdd,
       const ValueIdSet &predicatesToRemove,
       const ValueIdSet &commonPredicatesToAdd,
       const ValueIdSet &inputsToRemove,
       ValueIdSet &valuesForVEGRewrite,
       ValueIdSet &keyColumns,
       CSEInfo *info);

  // Detect whether rows coming from the ith child contain multi-column skew for
  // a set of join predicates. The output argument vidOfEquiJoinWithSkew is the
  // valueId of a particular join predicate chosen by the method for use by the
  // skew buster.
  NABoolean childNodeContainMultiColumnSkew(
                        CollIndex i,                 // IN: which child
                        const ValueIdSet& joinPreds, // IN: the join predicate
                        double mc_threshold,         // IN: the mc skew threshold
                        double sc_threshold,         // IN: the single column skew
                        Lng32 countOfPipelines,      // IN: countofpipelines
                                                     // threshold
                        SkewedValueList** skLis,     // OUT: the skew list
                        ValueId& vidOfEquiJoinWithSkew // OUT
                                 ) const;

   // This method assumes a MC skew list is available at child i and
   // filters out those skew values which frequencies are below the threshold.
   // Each item in skList contains the run-time version of hash for the MC skew.
   NABoolean childNodeContainMultiColumnSkew(
                      CollIndex i,                 // IN: which child to work on
                      const ValueIdSet& joinPreds, // IN: the join predicate
                      double mc_threshold,         // IN: multi-column threshold
                      Lng32 countOfPipelines,      // IN:
                      SkewedValueList** skList     // OUT: the skew list
                               ) ;


  // Detect whether rows coming from the ith child contain skew for a
  // join predicate. This method assumes that there is only one join
  // predicate.
  NABoolean childNodeContainSkew(
                        CollIndex i,                 // IN: which child
                        const ValueIdSet& joinPreds, // IN: the join predicate
                        double sc_threshold,         // IN: the single column skew
                                                     // threshold
                        SkewedValueList** skLis      // OUT: the skew list
                                 ) const;

  const MergeType getMergeTypeToBeUsedForSynthLogProperties();
  const MCSkewedValueList * getMCSkewedValueListForJoinPreds(ValueIdList & colGroup);

  virtual void resolveSingleColNotInPredicate();

  void rewriteNotInPredicate(ValueIdSet & origSet, ValueIdSet & newSet);
  void rewriteNotInPredicate();

  const ValueIdSet& getExtraHubNonEssentialOutputs() const
                                  { return extraHubNonEssentialOutputs_; }

  void addExtraHubNonEssentialOutputs(const ValueIdSet& vidSet)
          { extraHubNonEssentialOutputs_ += vidSet; }

  NABoolean getIsForTrafLoadPrep() const
  {
    return isForTrafLoadPrep_;
  }

  void setIsForTrafLoadPrep(NABoolean isForTrafLoadPrep)
  {
    isForTrafLoadPrep_ = isForTrafLoadPrep;
  }

//////////////////////////////////////////////////////

protected:
    
  void clearEquiJoinPredicates()          { equiJoinPredicates_.clear(); }

  void storeEquiJoinPredicates(const ValueIdSet& newPredicates)
                                  { equiJoinPredicates_ = newPredicates; }

  void clearPredicatesToBeRemoved()    { predicatesToBeRemoved_.clear(); }

  void setPredicatesToBeRemoved(const ValueIdSet& preds)
				       { predicatesToBeRemoved_ = preds; }

  NABoolean hasRIMatchingPredicates(const ValueIdList& fkCols, 
				    const ValueIdList& ucCols,
				    const TableDesc* compRefTabId,
				    ValueIdSet & matchingPreds) const;

  NABoolean matchRIConstraint(GroupAttributes& leftGA, 
			      GroupAttributes& rightGA,
			      NormWA * normWAPtr);

  NABoolean hasMatchingRIConstraint(ValueId fkId, 
				    ValueId ukId,
				    NormWA * normWAPtr);

  ValueIdSet & equiJoinPredicates() { return equiJoinPredicates_; }

  NABoolean singleColumnjoinPredOKforSB(ValueIdSet& joinPreds);
  NABoolean multiColumnjoinPredOKforSB(ValueIdSet& joinPreds);


private:

  // ---------------------------------------------------------------------
  // PRIVATE METHODS.
  // ----------------------------------------------------------------------

  // ----------------------------------------------------------------------
  // try to rewrite join predicate from
  //   not((t1.c <> t2.c) is true) and t1.c is not null and t2.c is not null
  // to
  //   t1.c = t2.c and t1.c is not null and t2.c is not null
  // ----------------------------------------------------------------------
  void tryToRewriteJoinPredicate(NormWA & normWARef);

  // ----------------------------------------------------------------------
  // A method that is used for checking whether an outer join can be
  // converted to an inner join. It walks through a prediate tree to
  // decide whether the given predicate can eliminate null values from
  // the result. For example, T1 LJ T2 ... where T2.a = 5 eliminates all
  // those rows from the result in which T2.a contains a null value.
  // ----------------------------------------------------------------------
  NABoolean outerJoinIsConvertible(const ValueIdSet & whereClausePreds) const;
  NABoolean eliminatesNullValues(const ItemExpr *, const GroupAttributes & ) const;

  void normalizeNullInstantiatedForRightJoinOutput(NormWA & normWARef);

  // Helper method for left and full outer joins
  // The method simulates the behavior of left join
  // ----------------------------------------------------------------------
  void
  instantiateLeftHistsWithNULLs(ValueIdSet EqLocalPreds, /*in*/
                                CollIndex outerRefCount, /*in*/
                                ColStatDescList &joinStatDescList, /*in*/
                                const ColStatDescList &leftColStatsList, /*in*/
                                CostScalar &oJoinResultRows /*in, out*/,
                                CostScalar initialRowcount /*in*/,
                                CollIndex &rootStatIndex,
                                NAList<CollIndex>& statsToMerge);

  // ----------------------------------------------------------------------
  // Helper method for full outer joins
  // The method simulates the behavior of full outer joins
  // it is the second part of full outer join, where right histograms
  // are being NULL instantiated
  // ----------------------------------------------------------------------
  void
  instantiateRightHistsWithNULLs(CollIndex outerRefCount, /*in*/
                              ColStatDescList &innerJoinCopy, /*in*/
                              ColStatDescList &joinStatDescList, /*in*/
                              const ColStatDescList &rightColStatsList, /*in*/
                              CostScalar &oJoinResultRows /*in, out*/,
                              CostScalar initialRowcount /*in*/,
                              CollIndex rootStatIndex,
                              NAList<CollIndex> statsToMerge);

  // ---------------------------------------------------------------------
  // Common synthEstLogProp code form Joins and TSJs
  // ---------------------------------------------------------------------
  CostScalar commonSynthEst(const EstLogPropSharedPtr& myEstProps,
                            const EstLogPropSharedPtr& intermedEstProps,
                            CollIndex & outerRefCount,
                            const ColStatDescList & leftColStatsList,
                            const ColStatDescList & rightColStatsList,
                            ColStatDescList & joinStatDescList,
                            CostScalar initialCardinality);

  // return information about a child's partitioning function
  NABoolean usesReplicationPartFunction(CollIndex childNo) const;

  // ---------------------------------------------------------------------
  // PRIVATE DATA.
  // ---------------------------------------------------------------------

  // QSTUFF
  // indicates that this join performs a cursor update, delete or insert
  NABoolean cursorUpdate_;
  // QSTUFF

  // Flags.
  NABoolean  isNaturalJoin_;

  // Indicates that this subtree should not be transformed further.
  NABoolean  transformComplete_;

  // Indicates to (or not to) consider TSJ transformation for this
  // Join Node
  NABoolean  considerTSJ_;

  // Flag to indicate that this is an indexJoin
  NABoolean  isIndexJoin_;

  NABoolean  floatingJoin_;

  ItemExpr   * joinPredTree_;
  ValueIdSet joinPred_;

  // ---------------------------------------------------------------------
  // Expressions that cause the instantiation of null values in order to
  // null augment the row that is to be preserved.
  // Presently, it is used for the Left Join operator only/
  // ---------------------------------------------------------------------
  ValueIdList nullInstantiatedOutput_;


  // ---------------------------------------------------------------------
  // Expressions that cause the instantiation of null values in order to
  // null augment the row that is to be preserved.
  // This particular one is used for right joins only.
  // ---------------------------------------------------------------------
  ValueIdList nullInstantiatedForRightJoinOutput_;



//MV++
  // ---------------------------------------------------------------------
  // Used for translating the required sort key to the right
  // child sort key and backwards
  // For more information see NestedJoin::synthPhysicalProperty()
  // ---------------------------------------------------------------------
  ValueIdMap rightChildMapForLeftJoin_;

  ValueIdMap leftChildMapForRightJoin_;
//MV--

  // ---------------------------------------------------------------------
  // Equijoin predicates. They are a subset of the joinPred_.
  // The join also remembers the columns that are produced by its left
  // and right children that are referenced in the equijoin predicate.
  // ---------------------------------------------------------------------
  ValueIdSet equiJoinPredicates_;
  ValueIdMap equiJoinExpressions_;

  // ---------------------------------------------------------------------
  // Original equijoin predicates used by nestedJoin. This is because
  // all equi join predicates have been pushed down to its children. The
  // data member originalEquiJoinExpressions_ records the value before
  // being pushed down. Used in OCR or any type1 nested join where some
  // check on the equi-join predicate is required
  ValueIdMap originalEquiJoinExpressions_;

  // ---------------------------------------------------------------------
  // If this join drives an insert, update, or delete operation, then
  // it remembers the table descriptor of the updated table here. Its
  // purpose is to make the join require different physical properties
  // from its children when it performs an update (...insert,delete).
  // A ValueIdMap is also provided that allows to map expressions
  // on the update side to be rewritten in terms of the select query
  // and vice versa.
  // ---------------------------------------------------------------------
  TableDesc * updateTableDesc_;
  ValueIdMap * updateSelectValueIdMap_;

  // ---------------------------------------------------------------------
  // Temp member to indicate if this join is a direct or indirect result
  // of the application of the PrimeTableRule. These joins are optimized
  // seperate from other joins in R2.0. Later when we add group
  // sharing among LSRs, we will no longer need this member
  // ---------------------------------------------------------------------
  NABoolean joinFromPTRule_;

  // ---------------------------------------------------------------------
  // Tells if the join was created by MultiJoin::synthLogProp
  // Such a join breaks up the group's MultiJoin in a deterministic order
  // ---------------------------------------------------------------------
  NABoolean joinFromMJSynthLogProp_;

  // true iff we're an optional zigzag join
  NABoolean joinForZigZag_;

  // ---------------------------------------------------------------------
  // After synthesising logical properties we can ask if rows from
  // left will have a single matching row from the right (or vice-versa).
  // ---------------------------------------------------------------------
  NABoolean leftHasUniqueMatches_;
  NABoolean rightHasUniqueMatches_;

  ValueIdList reqdOrder_;       // ORDER BY list from an INSERT node

  NABoolean tsjAfterSQO_;  // this is a mandatory tsj which couldn't be unnested
  
  NABoolean tsjForWrite_;  // is the TSJ for an Insert, Update, or Delete?
  NABoolean tsjForUndo_; // is this Tsj used above undo node

  NABoolean tsjForSetNFError_; // is this TSJ used for setting rowindexes
                               //for NF errors
  // this tsj is used to move source rows to target for MERGE sql operator.
  NABoolean tsjForMerge_;

  // this tsj is used to move source rows to target for MERGE sql operator
  // and the merge statement has an INSERT clause. 
  NABoolean tsjForMergeWithInsert_;

   // this tsj is used to flow rows from source RelExpr to merge that is
  // implementing an upsert.
  NABoolean tsjForMergeUpsert_;

  NABoolean derivedFromRoutineJoin_;

  // this tsj is used to insert rows using the sidetree insert method.
  NABoolean tsjForSideTreeInsert_;
  
  // this tsj is used for a 'no rollback' query which could be executed as
  // a sidetree insert.
  NABoolean enableTransformToSTI_;

  // MV --
  PhysicalJoinType forcePhysicalJoinType_;

  JoinSourceType sourceType_;

  // used to send compile-time rowset size to executor
  // for rowset updates and deletes. Has non-zero value only if rowset_row_count
  // feature is enabled. Onlj will send this value through the TDB to executor
  // where an array of this length will be allocated at run-time to collect
  // rows_affected info for each rowset element.
  NABoolean rowsetRowCountArraySize_;

  // Note that the following comments document the handling of the
  // Halloween probem as it was implemented. .
  // Some restrictions in the R2 implementation have been removed for
  // Neo R2.2.  However, a CQD, R2_HALLOWEEN_SUPPORT can be used to
  // get the old R2 behavior, hence the suffix "R2" on the accessor
  // and mutator methods and the class member variable.

  // If TRUE, then this Join could potentially run into the Halloween
  // problem.  The source contains a reference to the target. We check
  // for this in binder and issue an error in many cases, but in some
  // cases we allow it and set this flag to TRUE.  If this flag is
  // set, then we must generate a safe plan WRT the Halloween problem.
  // For the Join operator, if this is tuple flow for write, then the
  // plan should have a SORT operator on the LHS of the Tuple Flow.
  // This causes the source to be blocked.  If this is the join for
  // the subquery, then this should be implemented as a Hybrid Hash
  // Join.  This also causes the source to be blocked.
  //
  NABoolean avoidHalloweenR2_;

  // The next attribute is for the Neo R2.2 implementation of
  // self-referencing updates and handling of the Halloween problem.
  // The main differences from R2 for the Join are that Join will
  // require the use of a SORT only if DP2 Locks method could not be
  // used.  Also, in R2.2 there is no special handling of the join for
  // any subquery.

  HalloweenJoinSortType halloweenForceSort_;


  // if TRUE, it denotes that this Join was created as during subquery
  // transformation in the transform phase to express a subquery in terms 
  // of a join, and that this join is a candidate for subquery unnesting
  // during SemanticQueryOptimization

  NABoolean candidateForSubqueryUnnest_ ;

  // if TRUE, it denotes that this Join needs to be converted to a Left Join
  // during the SemanticQueryOptimization phase, if this join gets unnested.

  NABoolean candidateForSubqueryLeftJoinConversion_ ;

  // if TRUE, it denotes that this Join is a candidate for the 
  // semijoin-to-innerjoin (+ possible groupby) transformation. The actual
  // transformation occurs during SemanticQueryOptimization. This
  // flag is set to TRUE only for SemiJoins

  NABoolean candidateForSemiJoinTransform_ ;

  // This ValueIdSet contains all the equijoin preds from this
  // join that are no longer needed because we have a found 
  // FK-UK (foreign key-unique key) join and this predicate is
  // guaranteed truw by the RI constraint. This determination is
  // done during synthLogProp() tree walk, but the actual removal
  // of preds is done during the SQO tree walk. We use this list to
  // transmit the set of removable preds from from synthLogProp to SQO.
  // We like to do the removal in SQO because (a) the removal is an 
  // process in SQO and (b) if something goes wrong the SQO phase has logic
  // to go back to the old tree, while synthLogProp() is not an expendable
  // step and does not have such logic.

  ValueIdSet predicatesToBeRemoved_;

  // when a join becomes an extra hub then the essential outputs of the 
  // child that became an extra hub are no longer essential so they are
  // saved as extraHubNonEssentialOutputs_ of the join node
  ValueIdSet extraHubNonEssentialOutputs_;



  NABoolean isForTrafLoadPrep_;
  
  // used for HJ and MJ (later). Causes beforeJoinPred to be evaluated prior
  // to equi join pred in work() method.
  NABoolean beforeJoinPredOnOuterOnly_;
}; // class Join

// -----------------------------------------------------------------------
// Nested Join Operator : Physical Operator
// -----------------------------------------------------------------------

class NestedJoin : public Join
{

public:

  // constructor
  NestedJoin(RelExpr *leftChild,
                    RelExpr *rightChild,
                    OperatorTypeEnum otype = REL_NESTED_JOIN,
                    CollHeap *oHeap = CmpCommon::statementHeap(),
                    TableDesc *updTableDesc = NULL,
                    ValueIdMap *updateSelectValueIdMap = NULL)
  : Join(leftChild, rightChild, otype, NULL, FALSE, FALSE,
         oHeap, updTableDesc, updateSelectValueIdMap),
    probesInOrder_(FALSE) {}

  // copy ctor
  NestedJoin (const NestedJoin &) ; // not written

  // virtual destructor
  virtual ~NestedJoin() {}

  // get a printable string that identifies the operator
  virtual NABoolean isLogical () const;
  virtual NABoolean isPhysical() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  // Cascades-related functions
  virtual PlanWorkSpace* allocateWorkSpace() const;
  virtual CostMethod* costMethod() const;
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);
  virtual NABoolean findOptimalSolution(Context* myContext,
                                        PlanWorkSpace* pws);

  virtual PhysicalProperty* synthPhysicalProperty(const Context* myContext,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);
  // method to do code generation
  // method to do code generation
  RelExpr * preCodeGen(Generator * generator,
                       const ValueIdSet &externalInputs,
                       ValueIdSet &pulledNewInputs);

  virtual short codeGen(Generator*);

  // Accessor for the probesInOrder flag.
  NABoolean probesInOrder() const { return probesInOrder_; }
  // Mutator for the probesInOrder flag.
  void setProbesInOrder(NABoolean probesInOrder)
    { probesInOrder_ = probesInOrder;  }

  // -----------------------------------------------------------------------
  // Generate the partitioning requirements for the left child of a
  // preferred probing order nested join plan, or a NJ that is for a
  // write operation. In both cases, we need to base the partitioning
  // requirement for the left child on the partitioning of the right
  // child to ensure that we get a Type-1 join. If the requirements could
  // not be generated because the user is attempting to force something that
  // is not possible, the method returns FALSE. Otherwise, it returns TRUE.
  // -----------------------------------------------------------------------
  NABoolean genLeftChildPartReq(
              Context* myContext, // IN
              PlanWorkSpace* pws, // IN
              const PartitioningFunction* physicalPartFunc, // IN
              PartitioningRequirement* &logicalPartReq) ; // OUT

  // ----------------------------------------------------------------------
  // Generate any sort requirement for the left child of a nested join that
  // is for a write operation - i.e. Insert, Update, or Delete.
  // The generated sort requirement is returned.
  // ----------------------------------------------------------------------
  ValueIdList genWriteOpLeftChildSortReq() ;

  // ---------------------------------------------------------------------
  // Generate the requirements for the right child of a nested join.
  // Returns the generated requirements if they were feasible, otherwise
  // NULL is returned.
  // ---------------------------------------------------------------------
  ReqdPhysicalProperty* genRightChildReqs(
                          const PhysicalProperty* sppForChild, // IN
                          const ReqdPhysicalProperty* rppForMe, // IN 
                          NABoolean avoidNSquareOpens) ; // IN

  // ---------------------------------------------------------------------
  // Generate an input physical property object to contain the sort
  // order of the left child and related group attributes and
  // partitioning functions.
  // ---------------------------------------------------------------------
  InputPhysicalProperty* generateIpp(const PhysicalProperty* sppForChild,
                                     NABoolean isPlan0=FALSE) ;

  const NAString getText() const;

  virtual NABoolean currentPlanIsAcceptable(Lng32 planNo,
            const ReqdPhysicalProperty* const rppForMe) const;

  virtual NABoolean OCBJoinIsFeasible(const Context* myContext) const;
  virtual NABoolean OCRJoinIsFeasible(const Context* myContext) const;

  virtual NABoolean okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) ;

  virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

  //determines if inner table probes just one partition or all of them
 NABoolean allPartitionsProbed();

  // -----------------------------------------------------------------------
  // Performs mapping on the partitioning function, from the join to the
  // designated child.
  // -----------------------------------------------------------------------
  virtual PartitioningFunction* mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0) ;

 // determine if probeCache is applicable
 NABoolean isProbeCacheApplicable(PlanExecutionEnum loc) const;

private:
  // return the part. fuction for the clustering index (i.e., the base table)
  virtual 
  PartitioningFunction * getClusteringIndexPartFuncForRightChild() const;
  NABoolean JoinPredicateCoversChild1PartKey() const;

  // check the sort order for the source as sppForChild0 and the target table. 
  NABoolean checkCompleteSortOrder(const PhysicalProperty* sppForChild0);

private:
  // Set to TRUE if the probes to the inner table are at least partially
  // in order. Default is FALSE.
  NABoolean probesInOrder_;

}; // class NestedJoin


// -----------------------------------------------------------------------
// The NestedJoinFlow operator emulates the dataflow that is established
// by the NestedJoin from its left child to its right child. It does
// not perform a join as such. It simply transfers rows (values) that
// are produced by its left child to its right child. It produces no
// output and has no predicates.
// -----------------------------------------------------------------------

class NestedJoinFlow : public NestedJoin
{

public:

  // constructor
  NestedJoinFlow(RelExpr  *leftChild,
                        RelExpr  *rightChild,
                        TableDesc *updTableDesc,
                        ValueIdMap *updateSelectValueIdMap,
                        CollHeap *oHeap = CmpCommon::statementHeap())
     : NestedJoin(leftChild, rightChild, REL_NESTED_JOIN_FLOW,
                  oHeap, updTableDesc, updateSelectValueIdMap),
       sendEODtoTgt_(FALSE)
  {}

  // copy ctor
  NestedJoinFlow (const NestedJoinFlow &) ; // not written

  // Cascades-related functions
  virtual CostMethod* costMethod() const;

  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet &externalInputs,
                               ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

private:
  NABoolean sendEODtoTgt_;
}; // class NestedJoinFlow


// -----------------------------------------------------------------------
// Merge Join Operator : Physical Operator
//
// This node can be used to represent either a merge join (REL_MERGE_JOIN)
// or a left merge join (REL_LEFT_MERGE_JOIN).
// -----------------------------------------------------------------------

class MergeJoin : public Join
{

public:

  // constructor
  MergeJoin(RelExpr *leftChild,
                   RelExpr *rightChild,
                   OperatorTypeEnum otype = REL_MERGE_JOIN,
                   ItemExpr *joinPred = NULL,
                   CollHeap *oHeap = CmpCommon::statementHeap())
  : Join (leftChild, rightChild, otype, joinPred, FALSE, FALSE, oHeap),
  leftUnique_(FALSE), rightUnique_(FALSE), deadLockIsPossible_(FALSE) {}

  // copy ctor
  MergeJoin (const MergeJoin &) ; // not written

  // virtual destructor
  virtual ~MergeJoin() { }

  virtual NABoolean isLogical () const;
  virtual NABoolean isPhysical() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  // Cascades-related functions
  virtual CostMethod* costMethod() const;
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);
  virtual NABoolean findOptimalSolution(Context* myContext,
                                        PlanWorkSpace* pws);

  virtual NABoolean currentPlanIsAcceptable(Lng32 planNo,
            const ReqdPhysicalProperty* const rppForMe) const;

  virtual PhysicalProperty* synthPhysicalProperty(const Context* myContext,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  // method to do code generation
  RelExpr * preCodeGen(Generator * generator,
                       const ValueIdSet &externalInputs,
                       ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  const NAString getText() const;

  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;

  // Accessor functions
  inline const ValueIdList & getLeftSortOrder() const
                                           { return leftSortOrder_; }
  inline const ValueIdList & getRightSortOrder() const
                                           { return rightSortOrder_; }
  inline ValueIdList & orderedMJPreds()    { return orderedMJPreds_; }
  inline const ValueIdList & getOrderedMJPreds() const
                                           { return orderedMJPreds_; }

  inline const ValueIdList & getCandidateLeftSortOrder() const
                                       { return candidateLeftSortOrder_; }
  inline const ValueIdList & getCandidateRightSortOrder() const
                                       { return candidateRightSortOrder_; }
  inline const ValueIdList & getCandidateMJPreds() const
                                       { return candidateMJPreds_; }

  NABoolean &leftUnique() { return leftUnique_; }
  NABoolean &rightUnique() { return rightUnique_; }

  // ---------------------------------------------------------------------
  // Method for checking whether parent partitioning requirements are
  // compatible with the Merge Join's partitioning requirements.
  // ---------------------------------------------------------------------
  NABoolean parentAndChildPartReqsCompatible
               (const ReqdPhysicalProperty* const rppForMe) const;

  // Manipulation Methods
  inline void setOrderedMJPreds (const ValueIdList &op)  { orderedMJPreds_ = op; }
  inline void setLeftSortOrder (const ValueIdList &so)   { leftSortOrder_  = so; }
  inline void setRightSortOrder (const ValueIdList &so)  { rightSortOrder_ = so; }
  inline void setCandidateLeftSortOrder (const ValueIdList &so)
                                         { candidateLeftSortOrder_ = so; }
  inline void setCandidateRightSortOrder (const ValueIdList &so)
                                         { candidateRightSortOrder_ = so; }
  inline void setCandidateMJPreds (const ValueIdList &p)
                                         { candidateMJPreds_ = p; }

  // From the ordering this is provided as input, and a set of predicates,
  // generate new sort orders that contains only the children that are
  // covered by the predicates.
  // merge join predicates.
  void generateSortOrders (const ValueIdList & ordering,
                           const ValueIdSet & preds,
                           ValueIdList  &leftSortOrder,
                           ValueIdList  &rightSortOrder,
                           ValueIdList  &orderedMJPreds,
                           NABoolean    &completelyCovered) const;

  // Generate a arrangement requirement, and possible a sort order
  // requirement, that is compatible with any existing required order
  // or arrangement.  Used only for creating an arrangement
  // requirement for the right child when we try the right child first.
  // This needs a seperate method because it is very complex.
  void genRightChildArrangementReq(
         const ReqdPhysicalProperty* const rppForMe,
         RequirementGenerator &rg) const;

  // -----------------------------------------------------------------------
  // Performs mapping on the partitioning function, from the join to the
  // designated child.
  // -----------------------------------------------------------------------
  virtual PartitioningFunction* mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0) ;

  virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

private:

  ValueIdList leftSortOrder_;   // sort order for left child
  ValueIdList rightSortOrder_;  // sort order for right child
  ValueIdList orderedMJPreds_;  // ordered list of merge join preds

  // Set to TRUE, if left/right child will have atmost one matching row.
  // Set in MergeJoin::preCodeGen().
  NABoolean leftUnique_;
  NABoolean rightUnique_;

  // This is set to TRUE in MergfeJoin::synthPhysicalProperty() if
  // parallel merge join could possible cause a deadlock. It is checked
  // in MergeJoin::currentPlanIsAcceptable(). See solution 10-051219-3501.
  NABoolean deadLockIsPossible_;

  // The following is a temporary holding area for merge join plans
  // that are considered.
  ValueIdList candidateLeftSortOrder_;
  ValueIdList candidateRightSortOrder_;
  ValueIdList candidateMJPreds_;
}; // class MergeJoin

// -----------------------------------------------------------------------
// Hash Join : Physical Operator
//
// This node can be used to represent either a hash join (REL_HASH_JOIN)
// or a left hash join (REL_LEFT_HASH_JOIN).
// Or now it may also represent REL_HYBRID_HASH_JOIN and REL_ORDERED_HASH_JOIN
// -----------------------------------------------------------------------

class HashJoin : public Join
{

public:

  // constructor
  HashJoin(RelExpr *leftChild,
                  RelExpr *rightChild,
                  OperatorTypeEnum otype = REL_HASH_JOIN,
                  ItemExpr *joinPred = NULL,
                  CollHeap *oHeap = CmpCommon::statementHeap())
  : Join (leftChild, rightChild, otype, joinPred, FALSE, FALSE, oHeap),
    isNoOverflow_(FALSE), reuse_(FALSE), multipleCalls_(-1),
    isOrderedCrossProduct_(FALSE), returnRightOrdered_(FALSE),
    isNotInSubqTransform_(FALSE),
    requireOneBroadcast_(FALSE),
    innerAccessOnePartition_(FALSE)
    {}

  HashJoin(RelExpr *leftChild,
                  RelExpr *rightChild,
                  CollHeap *oHeap = CmpCommon::statementHeap())
  : Join (leftChild, rightChild, REL_HASH_JOIN, NULL, FALSE, FALSE, oHeap),
     isNoOverflow_(FALSE), reuse_(FALSE), multipleCalls_(-1),
    isOrderedCrossProduct_(FALSE), returnRightOrdered_(FALSE),
    isNotInSubqTransform_(FALSE),
    requireOneBroadcast_(FALSE),
    innerAccessOnePartition_(FALSE)
    {}


  // virtual destructor
  virtual ~HashJoin() { }

  // Special method added to check for ordered cross product called by
  // RequiredPhysicalProperty::satisfied() to ensure that if a CQS has
  // requested an ordered cross product, then one is being produced.
  virtual NABoolean patternMatch(const RelExpr &other) const;

  virtual NABoolean isLogical () const;
  virtual NABoolean isPhysical() const;

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  // cost functions
  virtual CostMethod* costMethod() const;
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);
  virtual CostLimit* computeCostLimit(const Context* myContext,
                                            PlanWorkSpace* pws);

  virtual NABoolean currentPlanIsAcceptable(Lng32 planNo,
            const ReqdPhysicalProperty* const rppForMe) const;

  virtual PhysicalProperty* synthPhysicalProperty(const Context* myContext,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  // method to do code generation
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet &externalInputs,
                               ValueIdSet &pulledNewInputs);
  virtual short codeGen(Generator*);

  // A variation of codeGen used when we can use the UniqueHashJoin
  // for this Join.
  //
  short codeGenForUnique(Generator*);

  // A helper method to determine if this Join can use the
  // UniqueHashJoin TCB.
  //
  NABoolean canUseUniqueHashJoin();

  // get a printable string that identifies the operator
  const NAString getText() const;

  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;

  //This is the original isBMO method.
  virtual NABoolean isBigMemoryOperator(const PlanWorkSpace* pws,
                                        const Lng32 planNumber);

  virtual CostScalar getEstimatedRunTimeMemoryUsage(Generator *generator, NABoolean perNode, Lng32 *numStreams = NULL);

  inline ValueIdSet & checkInputValues() { return checkInputValues_;}
  inline ValueIdSet & moveInputValues()  { return moveInputValues_;}

  // The method gets refined since HJ may be a BMO depending on its inputs.
  // Ratio is the ratio of file-size/memorysize. If the ratio >=1 it
  // is a BMO, if the ratio <1, it is not, and if the ratio > 5 it
  // is a VBMO (Very Big Memory Operator). Default is an improbable value.
  NABoolean isBigMemoryOperatorSetRatio(const Context* context,
                                        const Lng32 planNumber,
					double & ratio);
  // isLeftOrdered has been renamed to noOverflow. Ordered Hash Join or
  // NoOverflow join is an alternative hash join to the hybrid hash join.
  inline NABoolean isNoOverflow() {return isNoOverflow_;}
  inline void setNoOverflow(NABoolean isNoOverflow)
  {
    isNoOverflow_ = isNoOverflow;
  }

  inline NABoolean isReuse() {return reuse_;}
  inline void setReuse(NABoolean reuse){reuse_ = reuse;}
  inline Int32 & multipleCalls() {return multipleCalls_;}
  inline NABoolean isOrderedCrossProduct() {return isOrderedCrossProduct_;}
  inline void setIsOrderedCrossProduct(NABoolean flag){isOrderedCrossProduct_= flag;}
  inline NABoolean returnRightOrdered() { return returnRightOrdered_; }
  inline void setReturnRightOrdered(NABoolean flag) { returnRightOrdered_ = flag ; }

  virtual NABoolean okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) ;

  ExpTupleDesc::TupleDataFormat determineInternalFormat( const ValueIdList & rightList,
                                                         const ValueIdList & leftList,
                                                         RelExpr * relExpr,
                                                         NABoolean & resizeCifRecord,
                                                         Generator * generator,
                                                         NABoolean bmo_affinity,
                                                         NABoolean & considerBufferDefrag,
                                                         NABoolean uniqueHJ = FALSE);


  // -----------------------------------------------------------------------
  // Performs mapping on the partitioning function, from the join to the
  // designated child.
  // -----------------------------------------------------------------------
  virtual PartitioningFunction* mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0) ;
  inline ValueIdSet & valuesGivenToChild() { return valuesGivenToChild_; }


  virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

  // Test whether the skewed values exist in the output of the left
  // child of this join. If so, the pointer to skew value list is returned
  // in argument x.
  virtual NABoolean isSkewBusterFeasible( SkewedValueList** x, Lng32 countOfPipelines, ValueId&);

  //NOT IN optimization methods - start
  inline ValueIdSet& getCheckInnerNullExpr() 
  { 
    return checkInnerNullExpr_; 
  }

  inline ValueIdSet& getCheckOuterNullExpr() 
  { 
    return checkOuterNullExpr_; 
  }
  
  // add check outer and inner Null expressions
  void addCheckNullExpressions(CollHeap *wHeap);

  void addNullToSkewedList(SkewedValueList** skList);

  virtual void resolveSingleColNotInPredicate();

  NABoolean getIsNotInSubqTransform() const
  {
    return isNotInSubqTransform_;
  }

  void setIsNotInSubqTransform( NABoolean v)
  {
    isNotInSubqTransform_ = v;
  }

  NABoolean getRequireOneBroadcast() const
  {
    return requireOneBroadcast_;
  }

  void setRequireOneBroadcast( NABoolean v)
  {
    requireOneBroadcast_ = v;
  }

  //NOT IN optimization methods - end


  // Added for support of the MIN/MAX optimization
  // for HashJoin. Min and Max values are computed
  // during readInnerChild phase and passed to outer
  // before starting the read from the outer child

  // The list of surrogate values representing the 
  // min and max values to be computed.
  // These are system generated hostvars which
  // will be replaced (mapped) to the actual min max
  // values during codeGen();
  inline const ValueIdList& getMinMaxVals() 
  { 
    return minMaxVals_; 
  }

  // The list of values for which min and max
  // values are computed.
  inline const ValueIdList& getMinMaxCols()
  {
    return minMaxCols_;
  }

  // During PreCodeGen, the generator maintains a list of
  // values which are candidates for min/max optimization.
  // Each join is responsible for a subset of those candidates.
  // These indexes define the subset for this join.
  inline const CollIndex getStartMinMaxIndex() { return startMinMaxIndex_; }
  inline void setStartMinMaxIndex(CollIndex i) { startMinMaxIndex_ = i; }

  inline const CollIndex getEndMinMaxIndex() { return endMinMaxIndex_; }
  inline void setEndMinMaxIndex(CollIndex i) { endMinMaxIndex_ = i; }

  NABoolean getInnerAccessOnePartition() const
   { return innerAccessOnePartition_;} ;

  void setInnerAccessOnePartition(NABoolean x)
   { innerAccessOnePartition_ = x;} ;

private:

  // ValueIdSet used to check if input values to be given to right child
  // have changed from previous input. Created/filled by preCodeGen.
  ValueIdSet checkInputValues_;

  // ValueIdSet used to move input values to internal buffers, to be checked
  // (next time) if they have changed. Created by preCodeGen.
  ValueIdSet moveInputValues_;

  ValueIdSet valuesGivenToChild_; // values whose change causes reuse of the table

  // True if the join is Ordered/noOverflow hash join, false if it is Hybrid.
  // False by default.
  NABoolean isNoOverflow_;

  // True if the inner child (1) is a candidate for reuse. This is determined
  // in HashJoinRule::nextSubstitute() by checking whether the
  // (resultCardinality() > 1).
  NABoolean reuse_;

  // This is relevant IFF reuse_ is TRUE. If multipleCalls_ is FALSE,
  // the inner table is called only once and this indicates a FULL REUSE.
  // If multipleCalls is TRUE (which is default), there could be only partial
  // and conditional REUSE (if the incoming oprobes are sorted).
  Int32 multipleCalls_;

  // This HashJoin is used to implement an order-preserving cross-product.
  // It has the property of being ordered as (left child order, right child order).
  // If this flag is set then then NoOverFlow flag must be TRUE.
  // Note that even if the join predicates and selection predicates are empty
  // this flag may not be set if there is a possibility of overflow during runtime.
  // or if the rows from the left are not unique or the SOT of the children are
  // not identical.
  NABoolean isOrderedCrossProduct_;

  // Return right rows with duplicate join key columns in the same order as read.
  // Requires (left) Ordered Hash Join and Key-Uniqueness of the left rows !!
  NABoolean returnRightOrdered_;

  // hash anti semi join check ouyer and inner Null expressions for a query like "select * from 
  // T1 where a NOT IN (select b from T2);", 
  // the check inner null expression is  ISNULL(<inner clumn>);
  // the check outer null expression is  ISNOTNULL(<outer clumn>);
  ValueIdSet checkInnerNullExpr_;
  ValueIdSet checkOuterNullExpr_;

  // NOT IN transformation
  NABoolean isNotInSubqTransform_;

  // this field is set when the notin predicate is resolved into equi-predicate
  NABoolean requireOneBroadcast_;

  // Added for support of the MIN/MAX optimization
  // for HashJoin. Min and Max values are computed
  // during readInnerChild phase and passed to outer
  // before starting the read from the outer child

  // The list of surrogate values representing the 
  // min and max values to be computed.
  // These are system generated hostvars which
  // will be replaced (mapped) to the actual min max 
  // values during codeGen();
  ValueIdList minMaxVals_;

  // The list of values for which min and max
  // values are computed.
  ValueIdList minMaxCols_;

  // During PreCodeGen, the generator maintains a list of
  // values which are candidates for min/max optimization.
  // Each join is responsible for a subset of those candidates.
  // These indexes define the subset for this join.
  CollIndex startMinMaxIndex_;
  CollIndex endMinMaxIndex_;
  // this field is set when only one partition of the inner table can
  // provide data to build the hash table. Such value is available
  // only when the join is immediately above the inner table.
  NABoolean innerAccessOnePartition_;

}; // class HashJoin

#endif /* RELJOIN_H */

