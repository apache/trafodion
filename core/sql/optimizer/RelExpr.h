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
#ifndef RELEXPR_H
#define RELEXPR_H
/* -*-C++-*-
******************************************************************************
*
* File:         RelExpr.h
* Description:  Relational expression base class
* Created:      4/28/94
* Language:     C++
* Status:       $State: Exp $
*
*
*
*
******************************************************************************
*/

#include "ObjectNames.h"
#include "CmpContext.h"
#include "CmpStatement.h"
#include "RETDesc.h"
#include "ValueDesc.h"
#include "Rule.h"
#include "ExprNode.h"
#include "ComTransInfo.h"
#include "mdam.h"
#include "DefaultConstants.h"
#include "Inlining.h"
#include "NADefaults.h"
#include "OptUtilIncludes.h"
#include "ExplainTupleMaster.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ExprGroupId;
class RelExpr;
class RelExprList;
class CutOp;
class SubtreeOp;
class WildCardOp;
class ScanForceWildCard;
class UDFForceWildCard;
class RequiredResources;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;
class CacheWA;
class CascadesPlan;
class Context;
class ContextList;
class Cost;
class CostLimit;
class CostMethod;
class EstLogProp;
class Generator;
class ComTdb;
class ExplainTuple;
class GroupAttibutes;
class Guidance;
class NormWA;
class PhysicalProperty;
class InputPhysicalProperty;
class ReqdPhysicalProperty;
class PlanWorkSpace;
class PartitioningRequirement;
class FileScan;
class Scan;
class SubTreeAccessSet;
class SearchKey;
class ScanKey;
class MVInfoForDDL;
class PlanPriority;
class Hint;
class TableMappingUDF;
class RelSequence;
class CSEInfo;


////////////////////
class CANodeIdSet;
class QueryAnalysis;
class GroupAnalysis;
////////////////////

// used in heuristics to limit VSBB and SIDETREE inserts only if number rows
// being inserted is larger than this constant. Used in ImplRule.cpp and 
// GenPreCode.cpp
const Lng32 MIN_ROWS_FOR_VSBB = 100;

// used to defined input partitioning type of a TMUDF table input (its child)
enum TMUDFInputPartReq 
{
  ANY_PARTITIONING, 
  SPECIFIED_PARTITIONING, 
  REPLICATE_PARTITIONING, 
  NO_PARTITIONING 
};

// -----------------------------------------------------------------------
// An ExprGroupId object is a pointer to a logical or a physical RelExpr
// object or to a Cascades group. It is used to point to the child
// nodes (subtrees) of an ExprNode object. Different modes of addressing
// are available, depending on whether the tree is a standalone object
// outside of Cascades, a materialized binding in Cascades, or a memoized
// expression (a member of the CascadesMemo structure).
// -----------------------------------------------------------------------
class ExprGroupId : public NABasicObject
{
public:

  // state of the object
  enum GroupIdMode
  {
    STANDALONE,
    BINDING,
    MEMOIZED
  };

  // constructors

  ExprGroupId();
  ExprGroupId(const ExprGroupId & other);
  ExprGroupId(RelExpr *node);
  ExprGroupId(CascadesGroupId groupId);

  // assignment operators
  ExprGroupId & operator = (const ExprGroupId & other);
  ExprGroupId & operator = (RelExpr * other);
  ExprGroupId & operator = (CascadesGroupId other);

  // compare an ExprGroupId with another ExprGroupId or a RelExpr *
  NABoolean operator == (const ExprGroupId &other) const;
  NABoolean operator != (const ExprGroupId &other) const
                                        { return NOT operator == (other); }
  NABoolean operator == (const RelExpr *other) const;
  NABoolean operator != (const RelExpr *other) const
                                        { return NOT operator == (other); }

  // dereferencing operator, this class works like a pointer
  inline RelExpr * operator ->() const;         // defined below class Rexpr

  // same as a "normal" method
  inline RelExpr * getPtr() const;              // defined below class RelExpr

  // cast into a RelExpr *
  inline operator RelExpr *() const;            // defined below class RelExpr

  // return a group number instead of a pointer
  CascadesGroupId getGroupId() const;

  // return the mode of the pointer
  GroupIdMode getMode() const           { return groupIdMode_; }

  // change a binding back to mode MEMOIZED
  void releaseBinding();
  void convertBindingToStandalone(); // use after optimization

  // The Group Attributes contains certain attributes that are
  // characteristics for a relational operator.
  void setGroupAttr(GroupAttributes *gaPtr);
  GroupAttributes *getGroupAttr() const;

  // shortcut to get the estimated output log props out of the group
  // given the corresponding input estimated logical property.
  // NOTE:  If the OLP has not yet been synthesized, this method
  //        will synthesize the OLP on demand.
  //        (This means that no const version of this function is
  //         possible)
  EstLogPropSharedPtr outputLogProp(const EstLogPropSharedPtr& inputLogProp);

  // A shortcut to get the bound expression (if it exists) ...
  // or the first logical expression in the Cascades Group.
  RelExpr * getLogExpr() const;

  RelExpr * getFirstLogExpr() const;

private:

  enum GroupIdMode groupIdMode_;
  RelExpr * node_;              // used for all modes but MEMOIZED
  CascadesGroupId groupId_;     // used for the MEMOIZED and BINDING mode

}; // class ExprGroupId

// -----------------------------------------------------------------------
// A generic relational expression node. This class is a base class for
// any relational operator in the system. Every relational operator has
// a selection predicate associated with it. This base class also
// maintains all the data members describing the output row of a
// relational operator and its outer references.
// -----------------------------------------------------------------------

class RelExpr : public ExprNode
{

public:

  // ---------------------------------------------------------------------
  // default constructor
  // ---------------------------------------------------------------------
  RelExpr(OperatorTypeEnum otype,
          RelExpr  *leftChild = NULL,
          RelExpr  *rightChild = NULL,
          CollHeap *outHeap = CmpCommon::statementHeap());

  // operator== required for use in templates
  NABoolean operator==(const RelExpr &other) const
  {
    if(&other == this)
      return TRUE;
    else
      return FALSE;
  }

  // ---------------------------------------------------------------------
  // virtual destructor
  // ---------------------------------------------------------------------
  virtual ~RelExpr();

  virtual Int32 getArity() const;

  NABoolean isAnyJoin() const;

  virtual const NAString getText() const {return NAString("RelExpr"); }

  // are RelExpr's kids cacheable after this phase?
  NABoolean cacheableKids(CacheWA& cwa);

  // is this entire expression cacheable after this phase?
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // is this single ExprNode cacheable after this phase?
  NABoolean isCacheableNode(CmpPhase phase) const;

  // mark this ExprNode as cacheable for this and later phases
  void setCacheableNode(CmpPhase phase);

  // append an ascii-version of RelExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cachewa) const;

  // change literals of a cacheable query into ConstantParameters
  virtual RelExpr* normalizeForCache(CacheWA& cachewa, BindWA& bindWA);

  // helper function on CQS Tree
  virtual RelExpr *generateMatchingExpr(CANodeIdSet &,
                                        CANodeIdSet &,
                                        RelExpr *);

  // helper function on Normalized tree (CQS project)
  virtual RelExpr * generateLogicalExpr (CANodeIdSet &,
                                        CANodeIdSet &);

  Hint *getHint() const { return hint_; }

  void  setHint(Hint *h) { hint_ = h; }

  static const CorrName invalid;
 virtual inline const CorrName& getTableName()const { return invalid;}



 protected:
  // append an ascii-version of RelExpr node into cachewa.qryText_
  void generateCacheKeyNode(CacheWA& cwa) const;

  // append an ascii-version of RelExpr's kids into cachewa.qryText_
  void generateCacheKeyForKids(CacheWA& cachewa) const;

  // change literals in cacheable query's kids into ConstantParameters
  void normalizeKidsForCache(CacheWA& cachewa, BindWA& bindWA);

  // How much memory do we expect this operator's executor to be able to use 
  // per ESP/master fragment ?
  Lng32 getExeMemoryAvailable(NABoolean inMaster) const;

  // compute the memory quota 
  double computeMemoryQuota(NABoolean inMaster,
                            NABoolean perNode,
                            double BMOsMemoryLimit,
                            UInt16 totalNumBMOs,
                            double totalBMOsMemoryUsage,
                            UInt16 numBMOsPerFragment,
                            double BMOMemoryUsage,
                            Lng32  numStreams,
                            double &memQuotaRatio
                           );

 
 public:
  // ---------------------------------------------------------------------
  // perform a safe type cast (return NULL ptr for illegal casts)
  // ---------------------------------------------------------------------
  virtual RelExpr *castToRelExpr();
  const RelExpr * castToRelExpr() const;

  // This method clears all logical expressions uptill the leaf node
  // for multi-join. The methid should be called only before optimization phases
  // Reason for that is (1)it is very expensive to synthLogProp and should be avoided
  // (2) we are resetting number of joined tables, which should not be done once it is
  // set during optimization phases

  void clearLogExprForSynthDuringAnalysis();

  virtual NABoolean  isAControlStatement()    const { return FALSE; }
  virtual StaticOnly isAStaticOnlyStatement() const { return NOT_STATIC_ONLY; }

  NABoolean isParHeuristic4Feasible(Context* myContext,
                           const ReqdPhysicalProperty* rppForMe);

  // safe casting to derived classes
  virtual TableMappingUDF *castToTableMappingUDF();

  // ---------------------------------------------------------------------
  // method required for traversing an ExprNode tree
  // access a child of an ExprNode
  // ---------------------------------------------------------------------
  virtual ExprNode * getChild(Lng32 index);

  // Method for replacing a particular child
  virtual void setChild(Lng32 index, ExprNode *);

  // ---------------------------------------------------------------------
  // Delete this node without deleting its subtree.
  // ---------------------------------------------------------------------
  void deleteInstance();

  // ---------------------------------------------------------------------
  // methods for traversing the tree
  // ---------------------------------------------------------------------

  // operator [] is used to access the children of a tree
  virtual ExprGroupId & operator[] (Lng32 index)
    {
      CMPASSERT(index >= 0 AND index < MAX_REL_ARITY);
      return child_[index];
    }

  virtual const ExprGroupId & operator[] (Lng32 index) const
    {
      CMPASSERT(index >= 0 AND index < MAX_REL_ARITY);
      return child_[index];
    }

  // for cases where a named method is more convenient than operator []
  inline ExprGroupId & child(Lng32 index)
                                             { return operator[](index); }
  // const version of the above
  inline const ExprGroupId & child(Lng32 index) const
                                             { return operator[](index); }

  // ---------------------------------------------------------------------
  // manipulate the selection predicate of the node
  // (read and set the entire predicate, add and remove
  // predicates that are ANDed together).
  // Predicates that are passed through this interface change
  // ownership, meaning that the new "owner" is responsible
  // for deallocation. No copies are made by these methods.
  // ---------------------------------------------------------------------

  // add a new selection predicate (new pred = child pred AND existing pred)
  void addSelPredTree(ItemExpr *selpred);

  // remove the selection predicate tree from this node and return its pointer
  ItemExpr * removeSelPredTree();

  // non-destructive read of the selection predicates
  ItemExpr * selPredTree() const                        { return selection_; }
  const ItemExpr * getSelPredTree() const               { return selection_; }

  // return a (short-lived) read/write reference to the selection predicates
  ValueIdSet & selectionPred() { CMPASSERT(NOT isCutOp()); return predicates_; }

  // return a constant reference to the selection predicates
  const ValueIdSet & getSelectionPred() const           { return predicates_; }
  const ValueIdSet & getSelectionPredicates() const     { return predicates_; }

  void setSelectionPredicates(const ValueIdSet& p)      { predicates_ = p; }

  //++MV
  // Used by inlining mechanism for adding optimizer constraints
  void addUniqueColumnsTree(ItemExpr *uniqueColumnsTree);
  ItemExpr * removeUniqueColumnsTree();

  inline ValueIdSet& getUniqueColumns()			{ return uniqueColumns_; }
  inline void setUniqueColumns(const ValueIdSet& uniqueColumns)
  {
    uniqueColumns_ = uniqueColumns;
  }

  CardConstraint * getCardConstraint() const { return cardConstraint_; }
  void addCardConstraint(Cardinality lowerBound, Cardinality upperBound)
  {
    cardConstraint_ = new(CmpCommon::statementHeap())
			 CardConstraint(lowerBound,upperBound);
  }
  //--MV


  // ---------------------------------------------------------------------
  // The Group Attributes contains certain attributes that are
  // characteristics for this relational operator, namely,
  // 1) the values that it requires as external inputs, which are
  //    outer references such as a column from an outer scope,
  //    a host variable, a dynamic parameter or a constant.
  // 2) the values that it produces as output.
  // 3) the logical properties such as constraints, statistics, etc.
  // ---------------------------------------------------------------------
  void setGroupAttr(GroupAttributes *gaPtr);
  inline GroupAttributes *getGroupAttr() const      { return groupAttr_; }
  virtual NABoolean reconcileGroupAttr(GroupAttributes *newGroupAttr);

  // *********************************************************************
  // Common interfaces for processing DML statements.
  // Each interface implements one phase of query processing.
  // *********************************************************************
  enum AtomicityType
  {UNSPECIFIED_ = 0, ATOMIC_ = 1, NOT_ATOMIC_ = 2};

  // Rowsets processing does some transformations after parsing but before
  // binding. For example, it transforms a query Q that has an input rowset
  // array A into a TSJ that feeds each element of A into the query Q. The argument
  // inputArrayMaxsize is used to set the length of input arrays for ODBC
  // insert queries.
  virtual RelExpr *xformRowsetsInTree(BindWA& wa,
					const ULng32 inputArrayMaxsize = 0,
					const AtomicityType atomicity = UNSPECIFIED_);

  // --------------------------------------------------------------------
  // The name binding and semantics checking phase is implemented
  // by the virtual function bindNode().
  // --------------------------------------------------------------------
  virtual RelExpr * bindNode(BindWA *bindWAPtr);

  // --------------------------------------------------------------------
  // Unconditional query transformations such as the transformation of
  // a subquery to a semijoin are implemented by the virtual function
  // transformNode(). The aim of such transformations is to bring the
  // query tree to a canonical form. transformNode() also ensures
  // that the "required" (or characteristic) input values are "minimal"
  // and the "required" (or characteristic) outputs values are
  // "maximal" for each operator.
  //
  // transformNode() is an overloaded name, which is used for a set
  // of methods that implement the transformation phase of query
  // normalization.
  //
  // We use the term query tree for a tree of relational operators,
  // each of which can contain none or more scalar expression trees.
  // The transformations performed by transformNode() brings scalar
  // expressions into a canonical form. The effect of most such
  // transformations is local to the scalar expression tree.
  // However, the transformation of a subquery requires a semijoin
  // to be performed between the relational operator that contains
  // the subquery and the query tree for the subquery. The effect
  // of such a subquery transformation is therefore visible not
  // only in the scalar expression tree but also in the relational
  // expression tree.
  //
  // Originally, transformNode() was a virtual method on ExprNode.
  // The relational as well as scalar expressions are derived from
  // ExprNode and used to provide proprietary implementations for it.
  // However, relational and scalar expressions have different
  // processing requirements during the transformation phase. Hence,
  // each of them now support a virtual transformNode() method that
  // differ in their interfaces.
  //
  // Parameters:
  //
  // NormWA & normWARef
  //    IN : a pointer to the normalizer work area
  //
  // ExprGroupId & locationOfPointerToMe
  //    IN : a reference to the location that contains a pointer to
  //         the RelExpr that is currently being processed.
  //
  // --------------------------------------------------------------------
  virtual void transformNode(NormWA & normWARef,
                             ExprGroupId & locationOfPointerToMe);

  // --------------------------------------------------------------------
  // rewriteNode() is the virtual function that computes
  // the transitive closure for "=" predicates and rewrites value
  // expressions.
  // --------------------------------------------------------------------
  virtual void rewriteNode(NormWA & normWARef);

  // QSTUFF

  // --------------------------------------------------------------------
  // This routine checks whether a table is both read and updated
  // --------------------------------------------------------------------
  enum rwErrorStatus
    {
      RWERROR,
      RWOKAY
    };

  virtual rwErrorStatus checkReadWriteConflicts(NormWA & normWARef);

  // QSTUFF

  // --------------------------------------------------------------------
  // normalizeNode() performs predicate pushdown and also ensures
  // that the characteristic input as well as characteristic output
  // values are both "minimal".
  // --------------------------------------------------------------------
  virtual RelExpr * normalizeNode(NormWA & normWARef);

  // --------------------------------------------------------------------
  // getEssentialOutputsFromChildren() is a helper method used by the
  // classifyChildOutputs() method. This method will have a set containing
  // all Essential Outputs out a node's direct children in essOutputs,
  // upon completion.
  // --------------------------------------------------------------------
  void getEssentialOutputsFromChildren(ValueIdSet & essOutputs);

  // --------------------------------------------------------------------
  // fixEssentialCharacteristicOutputs() is a used during the bottom-up of
  // normalizeNode(). This method collects the essential outputs of the
  // children nodes and if any nonessential outputs of the cureent node
  // overlap with those essential outputs, then the overlapping nonessential
  // outputs are set to be essential too. In other words this method implements
  // the rule that states if an output is essential in my child, it is
  // essential for me too (if the valueid is part of my output).
  // --------------------------------------------------------------------
  void fixEssentialCharacteristicOutputs();

  // --------------------------------------------------------------------
  // semanticQueryOptimizeNode() performs sematic query optimization
  // In Neo R2.2 this includes only subquery unnesting
  // semantic query optimization is defined as a sematics preserving
  // tranformation on a query tree that uses the logical constraints
  // computed for the query.
  // --------------------------------------------------------------------
  virtual RelExpr * semanticQueryOptimizeNode(NormWA & normWARef);

  // used during SemanticQueryOptimize to remove filter nodes introduced
  // in previous phases, if the subquery is not going to be unnested.
  // The objective is to push predicates as far down as possible so that
  // cardinality can be estimated correctly for all nodes during Analyzer
  // phase
  void eliminateFilterChild();

  // used duirng SQO, to check if the child or grandchild of current
  // node is a filter. Two acceptable patterns are (a) child(0) is filter
  // child(0) is groupby an child(0)->child(0) is filter
  NABoolean hasFilterChild();

  // used during SQO phase to make the left sub tree of the join
  // being unnested produce the extra outputs that is necessary to
  // group by a set of unique columns of of the left sub tree
  NABoolean getMoreOutputsIfPossible(ValueIdSet& outputsNeeded) ;

  // calls pushDownCoveredExpr recursively. Used at the end of the
  // SQO phase to get outputs at every node to be minimal
  void recursivePushDownCoveredExpr(NormWA * normWAPtr, 
                                    NABoolean doSynthLogProp = TRUE) ;

  // synthesizes (in Scan's implemenation), matches (in Join's implementation)
  // and flows CompRefOpt constraints up the query tree.
  virtual void processCompRefOptConstraints(NormWA * normWAPtr) ;

  // Used during SQO to prepare the child tree of a CommonSubExprRef
  // for sharing between multiple consumers. This basically undoes
  // some of the normalizations, like eliminating unneeded outputs
  // and pushing predicates down. This is a recursive tree walk
  // method, most nodes don't override this one.
  // Returns TRUE for success, FALSE if it was unable to prepare
  // (diags will be set).
  // Tree remains unchanged if testRun is TRUE.
  virtual NABoolean prepareTreeForCSESharing(
       const ValueIdSet &outputsToAdd,
       const ValueIdSet &predicatesToRemove,
       const ValueIdSet &commonPredicatesToAdd,
       const ValueIdSet &inputsToRemove,
       ValueIdSet &valuesForVEGRewrite,
       ValueIdSet &keyColumns,
       CSEInfo *info);

  // A virtual method called on every node from prepareTreeForCSESharing(),
  // use this to do the actual work of removing predicates other than
  // selection predicates, and to indicate that the node supports this
  // method (the default implementation returns FALSE).
  //
  // This method needs to
  // - make sure no changes are done to the node when testRun is TRUE
  // - add any required new outputs to the char. outputs, unless they
  //   are produced by a child
  // - make sure no local expression reference any of the inputs to
  //   be removed
  // - indicate in its return value whether it was successful doing so
  //
  // This method doesn't need to take care of the following, since the
  // caller of this method already does it:
  // - removing predicates from selectionPred()
  // - adding new common predicates to selectionPred(), unless
  //   they are covered by the children's group attributes
  // - adding required outputs that are produced by children
  // - removing char. inputs that are requested to be removed
  virtual NABoolean prepareMeForCSESharing(
       const ValueIdSet &outputsToAdd,
       const ValueIdSet &predicatesToRemove,
       const ValueIdSet &commonPredicatesToAdd,
       const ValueIdSet &inputsToRemove,
       ValueIdSet &valuesForVEGRewrite,
       ValueIdSet &keyColumns,
       CSEInfo *info);

  // --------------------------------------------------------------------
  // Create a query execution plan.
  // --------------------------------------------------------------------
  virtual RelExpr * optimizeNode();

  // --------------------------------------------------------------------
  // Perform local query rewrites such as for the creation and
  // population of intermediate tables, for accessing partitioned
  // data. Rewrite the value expressions after minimizing the dataflow
  // using the transitive closure of equality predicates.
  // --------------------------------------------------------------------
  virtual RelExpr * preCodeGen(Generator * generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);

  // --------------------------------------------------------------------
  // Perform DoP reduction. Called during preCodeGen().
  // --------------------------------------------------------------------
  virtual void prepareDopReduction(Generator*);
  virtual void doDopReduction();
  virtual void doDopReductionWithinFragment(Lng32 newDoP);

  // --------------------------------------------------------------------
  // Generate executor task definition blocks (ComTdb's).
  // --------------------------------------------------------------------
  virtual short codeGen(Generator *);

  // -----------------------------------------------------
  // generate CONTROL QUERY SHAPE fragment for this node.
  // -----------------------------------------------------
  virtual short generateShape(CollHeap * space, char * buf, NAString * shapeStr = NULL);

  // *********************************************************************
  // Helper functions and methods used by the binder.
  // *********************************************************************

  void bindChildren(BindWA *bindWAPtr);

  RelExpr *bindSelf(BindWA *bindWAPtr);

  void setRETDesc(RETDesc *retdesc) { RETDesc_ = retdesc; }
  RETDesc *getRETDesc() const;
  CollIndex getDegree() const
  {
    RETDesc *rd = getRETDesc();
    return rd ? rd->getDegree() : (CollIndex)0;
  }

  Scan *getScanNode(NABoolean assertExactlyOneScanNode = TRUE) const;
  Scan *getLeftmostScanNode() const;
  Scan *getAnyScanNode() const;
  Join *getLeftJoinChild() const;
  RelSequence *getOlapChild() const;
  void getAllTableDescs(TableDescList &tableDescs);

  TableDesc *findFirstTableDescAndValueIdMap(RelExpr *currentNode,
                                             ValueIdMap &valueIdMap);


  // QSTUFF
  // we must pushdown the outputs of a genericupdate root to its
  // descendants to ensure that only those required output values are
  // tested against indexes when selecting an index for a stream scan
  // followed by an embedded update. Since we may allow for unions and
  // for inner updates we just follow the isEmbeddedUpdate() thread once
  // we reach a generic update root.

  void pushDownGenericUpdateRootOutputs(const ValueIdSet &outputs);

  // in order to bind constraints for views with the check option we
  // have to locate the respective view scan scope. In case of a simple
  // view the scope is associated with the leaf scan node; in case of
  // stacked views the scope is associated with the rename node inserted
  // by view expansion.

  RelExpr *getViewScanNode(NABoolean isTopLevelUpdateInView = FALSE) const;
  // QSTUFF

  // a virtual function to get addressability to the list of output values
  virtual ItemExpr * selectList();    // in the external form

  // Triggers --
  // get/set the InliningInfo object.
  inline       InliningInfo& getInliningInfo()       { return inliningInfo_; }
  inline const InliningInfo& getInliningInfo() const { return inliningInfo_; }
  inline void setInliningInfo(InliningInfo *info) { inliningInfo_ = *info; }

  // Access set handling methods
  inline SubTreeAccessSet *getAccessSet0()     { return accessSet0_;}
  inline SubTreeAccessSet *getAccessSet1()     { return accessSet1_;}
  inline void setAccessSet0(SubTreeAccessSet *as) { accessSet0_ = as; }
  inline void setAccessSet1(SubTreeAccessSet *as) { accessSet1_ = as; }

  virtual SubTreeAccessSet *calcAccessSets(CollHeap *heap);

  // MV --
  virtual NABoolean isIncrementalMV() { return FALSE; }
  virtual void collectMVInformation(MVInfoForDDL *mvInfo,
				    NABoolean isNormalized);

  // determine whether an IUD operation has been seen
  virtual NABoolean seenIUD() { return seenIUD_; }
  virtual void setSeenIUD() { seenIUD_ = TRUE; }

  // *********************************************************************
  // Helper functions and methods used by the normalizer.
  // *********************************************************************

  // procedure to do the common processing of the select predicate
  void transformSelectPred(NormWA & normWARef,
                           ExprGroupId & locationOfPointerToMe);

  // ---------------------------------------------------------------------
  // Inputs and Ouptuts.
  //
  // Each relational operator in the dataflow tree can receive some
  // external dataflow inputs such as constant values or values in
  // host variables or dynamic parameters, for evaluating some scalar
  // expression. Each operator can also produce one or more output
  // values that are either "raw" values from columns or values that
  // result from the evaluation of an expression.
  //
  // An operator can receive more values as inputs than it requires
  // for its own expressions because it passes them down to its
  // children. It is also capable of producing a large number of
  // values as outputs: potentially all those values that its children
  // can produce as well as those that do not require any new external
  // children. For example, if the children can produce values a, b,
  // then the operator is capable of producing a, b, a + b and so on.
  //
  // The Group Attributes that are associated with each relational
  // operator contain its Charcateristic Input and Outputs. The
  // following methods are used for priming the Group Attributes
  // with an set of values that provide the basis for computing
  // the Characteristic Input and Outputs.
  // --------------------------------------------------------------------
  void primeGroupAttributes();
  void allocateAndPrimeGroupAttributes(); // used after a rule-based transformation
  virtual void primeGroupAnalysis();

  // --------------------------------------------------------------------
  // A method that provides the set of values that can potentially be
  // produced as output by this operator. These are values over and
  // above those that are prodcued as Characteristic Outputs.
  // --------------------------------------------------------------------
  virtual void getPotentialOutputValues(ValueIdSet & vs) const;

  virtual void getPotentialOutputValuesAsVEGs(ValueIdSet& outputs) const;

  // ---------------------------------------------------------------------
  // pullUpPreds()
  //
  // This method is used during subquery transformation.
  // It moves predicates from transformed children of the node to the
  // appropriate place in the node.
  // If a child loses some predicates then recomputeOuterReferences()
  // must be called on it.
  //
  // ---------------------------------------------------------------------
  virtual void pullUpPreds();

  // ---------------------------------------------------------------------
  // recomputeOuterReferences()
  //
  // This method is used by the normalizer for recomputing the
  // outer references (external dataflow input values) that are
  // still referenced by each operator in the subquery tree
  // after the predicate pull up is complete.
  //
  // ---------------------------------------------------------------------
  virtual void recomputeOuterReferences();

  // --------------------------------------------------------------------
  // pushdownCoveredExpr()
  //
  // In order to compute the Group Attributes for a relational operator
  // an analysis of all the scalar expressions associated with it is
  // performed. The purpose of this analysis is to identify the sources
  // of the values that each expression requires. As a result of this
  // analysis values are categorized as external dataflow inputs or
  // those that can be produced completely by a certain child of the
  // relational operator.
  //
  // This method is invoked on each relational operator. It causes
  // a) the pushdown of predicates and
  // b) the recomputation of the Group Attributes of each child.
  //    The recomputation is required either because the child is
  //    assigned new predicates or is expected to compute some of
  //    expressions that are required by its parent.
  // c) Classification of outputs into essential and non-essential outputs
  //
  // Parameters:
  //
  // const ValueIdSet &  outputExprOnOperator
  //    IN:  a read-only reference to the outputs of a node. This will
  //         be used to classify the child outputs into essential &
  //         non-essential outputs. Ideally this argument is not needed
  //         as a relExpr is capable of finding out what its outputs are.
  //         This argument is being kept so that many callers do not
  //         have to change.
  //
  // ValueIdSet & newExternalInputs
  //    IN : a reference to a set of new external inputs (ValueIds)
  //         that are provided by this operator for evaluating the
  //         the above expressions.
  //
  // ValueIdSet & predOnOperator
  //    IN : the set of predicates existing on the operator
  //    OUT: a subset of the original predicates. Some of the
  //         predicate factors may have been pushed down to
  //         the operator's children.
  //
  // ValueIdSet &  nonPredNonOutputExprOnOperator
  //    IN:  A pointer to a set of expressions that are
  //         associated with this operator. Typically, they do not
  //         include the selection predicates. For example this will
  //         include the group and aggregate expr of a GroupBy, any
  //         order requirement on a Root node
  //
  // short  childId
  //    IN : This is an optional parameter.
  //         If supplied, it is a zero-based index for a specific child
  //         on which the predicate pushdown should be attempted.
  //         If not supplied, or a null pointer is supplied, then
  //         the pushdown is attempted on all the children.
  //
  // ---------------------------------------------------------------------
  virtual void pushdownCoveredExpr(
                                   const ValueIdSet & outputExprOnOperator,
                                   const ValueIdSet & newExternalInputs,
                                   ValueIdSet& predOnOperator,
				   const ValueIdSet *
				      nonPredNonOutputExprOnOperator = NULL,
                                   Lng32 childId = (-MAX_REL_ARITY));


  // -----------------------------------------------------------------------
  // RelExpr::computeValuesReqdForPredicates()
  // This function obtains the values required to evaluate the predicates
  // contained in setOfExpr (first argument). The
  // computed list of values is returned in the second argument
  // -----------------------------------------------------------------------
  static void computeValuesReqdForPredicates(const ValueIdSet& setOfExpr,
				      ValueIdSet& reqdValues,
                                             NABoolean addInstNull = FALSE);

  // -----------------------------------------------------------------------
  // RelExpr::computeValuesReqdForOutput()
  // This function obtains the values required to evaluate the predicates
  // contained in setOfExpr (first argument). The
  // computed list of values is returned in the third argument.
  // -----------------------------------------------------------------------
  static void computeValuesReqdForOutput(const ValueIdSet& setOfExpr,
                                         const ValueIdSet& newExternalInputs,
                                         ValueIdSet& outputsOnOperator);

  // -----------------------------------------------------------------------
  // RelExpr::preventPushDownPartReq()
  // This function returns TRUE if both children have plans, are small and
  // not partitioned. Looking through all group's plans could be expensive,
  // especially at the end of the compilation process when we have lots of
  // plans in the memo. The idea of heuristic4, implemented here, is to check
  // just the first plan's partitioning properties for the child's group.
  // Under SYSTEM option we try to create parallel plan first. So if this
  // first plan happend to be non-parallel because we couldn't create a
  // parallel one we don't want to try parallelism again.
  // SO, if this is the only child and the child is small and non-partitioned
  // then the function returns TRUE preventing creation of parallel plan. If
  // the child is partitioned the function will return FALSE allowing creation
  // of parallel plan for the child's group. If the current expression has two
  // children they should be both "small" and have their first plans non-
  // parallel and succeeded in the current pass to prevent attempt to create
  // another parallel plan.

  NABoolean preventPushDownPartReq
                                    (const ReqdPhysicalProperty* rppForMe,
                                     const EstLogPropSharedPtr& inLogProp     ); 
  // *********************************************************************
  // Helper functions and methods used by the optimizer.
  // *********************************************************************

  // synthesize logical properties
  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  // finish the synthesis of logical properties by setting various cardinality
  // related attributes
  virtual void finishSynthEstLogProp();

  // synthesize estimated logical properties given a specific set of
  // input log. properties
  virtual void synthEstLogProp(const EstLogPropSharedPtr& inputLP);

  // synthesize estimated logical properties for unary and leaf ops
  EstLogPropSharedPtr synthEstLogPropForUnaryLeafOp
              (const EstLogPropSharedPtr& inputEstLogProp,
               const ColStatDescList & childStats,
               CostScalar initialRowcount = 1,
               CostScalar *childMaxCardEst = NULL) const;

  // call the optimizer with requirements
  RelExpr * optimize(NABoolean               exception = FALSE,
	                 Guidance*               guidance = NULL,
                     ReqdPhysicalProperty  * rpp = NULL,
                     CostLimit*              costLimit = NULL);

  RelExpr * optimize2();

  RelExpr * checkAndReorderTree();

  void clearAllEstLogProp();

  // clear logical properties of me and mine children
  void clearAllLogProp();

  virtual RelExpr * reorderTree(NABoolean & treeModified,
	  NABoolean doReorderJoin);

  // added for trigger project, to activate after trigger transformation
  virtual RelExpr * fixupTriggers(NABoolean & treeModified);

  // ---------------------------------------------------------------------
  // comparison, hash, and copy methods
  // ---------------------------------------------------------------------

  virtual SimpleHashValue hash(); // from ExprNode
  virtual HashValue topHash();
  HashValue treeHash();
  virtual NABoolean patternMatch(const RelExpr & other) const;
  virtual NABoolean duplicateMatch(const RelExpr & other) const;

  // "virtual copy constructor", provides a generic method to duplicate
  // the top node of an expression tree or an entire tree.
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = NULL);
  RelExpr * copyTree(CollHeap* heap = NULL);
  RelExpr * copyRelExprTree(CollHeap* outHeap = NULL);
  const RelExpr * getOriginalExpr(NABoolean transitive = TRUE) const;
  RelExpr * getOriginalExpr(NABoolean transitive = TRUE);

  // --------------------------------------------------------------------
  // Methods used internally by Cascades
  // --------------------------------------------------------------------

  virtual NABoolean  isLogical() const;
  virtual NABoolean  isPhysical() const;

  // these functions must not be redefined, except for Cut, Subtree,
  // or Wildcard nodes
  virtual NABoolean isCutOp() const;
  virtual NABoolean isSubtreeOp() const;
  virtual NABoolean isWildcard() const;

  // get/set the Cascades internal data of a RelExpr
  // NOTE: these methods are not really intended for general use!!!
  inline CascadesGroupId getGroupId() const             { return groupId_; }
  inline void setGroupId(CascadesGroupId g)                { groupId_ = g; }

  inline RelExpr * getNextInGroup()                   { return groupNext_; }
  inline void setNextInGroup(RelExpr * v)                { groupNext_ = v; }
  inline RelExpr * getNextInBucket()                 { return bucketNext_; }
  inline void setNextInBucket(RelExpr * v)              { bucketNext_ = v; }

  inline const RuleSubset &getContextInsensRules() const { return contextInsensRules_; }
  inline RuleSubset &contextInsensRules()          { return contextInsensRules_; }
  inline const RulesPerContextList &getContextSensRules() const { return contextSensRules_; }
  inline RulesPerContextList &contextSensRules() { return contextSensRules_; }

  // ---------------------------------------------------------------------
  // methods needed by Cascades iff an operator is logical
  // ---------------------------------------------------------------------

  // determine how many promising moves to pursue in exploration
  virtual Int32 computeExploreExprCutoff(RuleWithPromise promisingMoves [],
                                       Int32 numberOfMoves,
                                       RelExpr* pattern,
                                       Guidance* myGuidance);

  // determine how many promising moves to pursue in optimization
  virtual Int32 computeOptimizeExprCutoff(RuleWithPromise promisingMoves [],
                                        Int32 numberOfMoves,
                                        Guidance* myGuidance,
                                        Context* myContext);

  // ---------------------------------------------------------------------
  // Methods needed by Cascades iff an operator is physical
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // A virtual function to permit the allocation of an operator-specific
  // work space that is used during plan generation.
  // ---------------------------------------------------------------------
  virtual PlanWorkSpace* allocateWorkSpace() const;

  // ---------------------------------------------------------------------
  // Obtain a pointer to a CostMethod object that provides access
  // to the cost estimation functions for nodes of this type.
  // ---------------------------------------------------------------------
  virtual CostMethod* costMethod() const;

  // ---------------------------------------------------------------------
  // The main driver for plan generation.
  // Generate a context for optimizing a child of this operator,
  // given the optimization context and an operator-specific
  // work space that is used during plan generation.
  // The (implementation) rule and the guidance that was used
  // for creating this instance of the operator are also supplied.
  // They are used for creating a new guidance that is used for
  // optimizing the child.
  // ---------------------------------------------------------------------
  virtual Context* createPlan(Context* myContext,
                              PlanWorkSpace* pws,
                              Rule* rule,
                              Guidance* myGuidance,
                              Guidance* & childGuidance);

  // ---------------------------------------------------------------------
  // Create a Context for a specific child and store it in the
  // PlanWorkSpace.
  // This method is called from within the implementation
  // of createPlan().
  // ---------------------------------------------------------------------
  virtual Context* createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex);

  // ---------------------------------------------------------------------
  // Create a Context for a specific child, given a sortkey and
  // arranged columns.
  // This method should be called from within the implementation
  // of createContextForAChild() for operators that require the dataflow
  // produced by their child to be sorted.
  // ---------------------------------------------------------------------
  Context * generateContext(Context* myContext,
                            PlanWorkSpace* pws,
                            Lng32 childIndex,
                            const ValueIdSet* const arrangedCols,
                            const ValueIdList* const sortKey,
                            PartitioningRequirement* partReq);

  // ---------------------------------------------------------------------
  // A virtual function for computing the cost limit to be imposed
  // on the child that is about to be optimized.
  // ---------------------------------------------------------------------
  virtual CostLimit* computeCostLimit(const Context* myContext,
                                            PlanWorkSpace* pws);

  // ---------------------------------------------------------------------
  // A virtual function that determines if the current plan is
  // compatible with any forced plan constraints and is viable.
  // ---------------------------------------------------------------------
  virtual NABoolean currentPlanIsAcceptable(Lng32 planNo,
            const ReqdPhysicalProperty* const rppForMe) const;

  // ---------------------------------------------------------------------
  // A virtual function for associating one of the Contexts
  // that was created for each child of this operator
  // with the Context for this operator.
  // ---------------------------------------------------------------------
  virtual NABoolean findOptimalSolution(Context* myContext,
                                        PlanWorkSpace* pws);

  // ---------------------------------------------------------------------
  // calculate physical properties from child's phys properties
  // (assuming the children are already optimized and have physical properties)
  // (used in the implementation of createPlan)
  // ---------------------------------------------------------------------
  virtual PhysicalProperty* synthPhysicalProperty(const Context* myContext,
                                                  const Lng32     planNumber,
                                                  PlanWorkSpace  *pws);

  // ---------------------------------------------------------------------
  // A helper method for synthesizing the physical properties for the leaf
  // operators scan, cursor insert, cursor update, and cursor delete.
  // ---------------------------------------------------------------------
  PhysicalProperty* synthDP2PhysicalProperty(
                              const Context*     myContext,
                              const ValueIdList& sortOrder,
                              const IndexDesc*   indexDesc,
                              const SearchKey*   partSearchKey);

  // ---------------------------------------------------------------------
  // create or share an optimization goal for a child group
  // ---------------------------------------------------------------------
  Context* shareContext(Lng32 childIndex,
                        const ReqdPhysicalProperty* const reqdPhys,
                        const InputPhysicalProperty* const inputPhys,
                        CostLimit* costLimit,
                        Context* parentContext,
                        const EstLogPropSharedPtr& inputLogProp,
                        RelExpr *explicitlyRequiredShape = NULL) const;

  // ---------------------------------------------------------------------
  // release a binding that was created by means other than using a
  // BINDING object (e.g. a binding created by Context::bindSolutionTree()
  // ---------------------------------------------------------------------
  void releaseBindingTree(NABoolean memoIsMoribund = FALSE);

  // ---------------------------------------------------------------------
  // A virtual function that decides if ESP parallelism is allowed
  // by the settings in the defaults table, or if the number of
  // ESPs is being forced.
  // ---------------------------------------------------------------------
  virtual DefaultToken getParallelControlSettings (
            const ReqdPhysicalProperty* const rppForMe, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) const;

  // ---------------------------------------------------------------------
  // A virtual function that decides if ESP parallelism is worth it
  // for the operator.
  // ---------------------------------------------------------------------
  virtual NABoolean okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) ;

  // ---------------------------------------------------------------------
  // Checks the required physical properties to see if they require
  // something that only an enforcer (i.e. sort or exchange) can provide.
  // This method is called by all implementations of the
  // method topMatch() except the enforcer rule implementations.
  // ---------------------------------------------------------------------
  NABoolean rppRequiresEnforcer
              (const ReqdPhysicalProperty* const rppForMe) const;

  // ---------------------------------------------------------------------
  // Compares the pivs of this node to those of its children.
  // If they are different, they are made the same.
  // ---------------------------------------------------------------------
  virtual void replacePivs();

  // ---------------------------------------------------------------------
  // Performs mapping on the partitioning function, from this
  // operator to the designated child, if the operator has/requires mapping.
  // Note that this default implementation does no mapping.
  // ---------------------------------------------------------------------
  virtual PartitioningFunction* mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0) ;

  virtual void addPartKeyPredsToSelectionPreds(
                          const ValueIdSet& partKeyPreds,
                          const ValueIdSet& pivs)
  {}

  // *********************************************************************
  // Helper functions and methods during the pre code generation phase.
  // *********************************************************************

  // ---------------------------------------------------------------------
  // get... methods for the code generator.
  // ---------------------------------------------------------------------
  void getOutputValuesOfMyChildren(ValueIdSet & vs) const;
  virtual void getInputValuesFromParentAndChildren(ValueIdSet & vs) const;
  void getInputAndPotentialOutputValues(ValueIdSet& vs) const;

  ULng32 getDefault(DefaultConstants);

  // cleanup after the generator phase
  virtual void cleanupAfterCompilation();

  // ---------------------------------------------------------------------
  // This method is used to propagate to all Sort nodes in the query
  // if the top N sorted rows are needed. Called when the user
  // has specified [first <N>] in their select statement. The actual
  // value of <N> is sent to the operator at runtime via a GET_N request.
  // See executor queue structure for details about GET_N request.
  // The value of 'val' is used to set or reset this in the Sort node.
  // ---------------------------------------------------------------------
  virtual void needSortedNRows(NABoolean val);

  // ---------------------------------------------------------------------
  // Methods for retrieving information from the best plan.
  // NOTE: The following set of methods should be used ONLY AFTER
  //       optimization is complete, at which point physical property,
  //       cost, and estimated # rows are transferred from the optimal plan.
  // ---------------------------------------------------------------------

  inline const PhysicalProperty* getPhysicalProperty() const
                                                      { return physProp_; }
  inline void setPhysicalProperty (const PhysicalProperty* physProp)
                                                  { physProp_ = physProp; }

  inline const Cost * getOperatorCost() const
  { return operatorCost_; }

  inline void setOperatorCost (const Cost * cost)
  { operatorCost_ = cost; }

  inline const Cost * getRollUpCost() const
  { return rollUpCost_; }

  inline void setRollUpCost (const Cost * cost)
  { rollUpCost_ = cost; }

  inline const CostScalar getEstRowsUsed() const { return estRowsUsed_; }
  inline void setEstRowsUsed(CostScalar r)          { estRowsUsed_ = r; }

  inline const CostScalar getInputCardinality() const { return inputCardinality_; }
  inline void setInputCardinality(CostScalar r)          { inputCardinality_ = r; }

  inline const CostScalar getMaxCardEst() const { return maxCardEst_; }
  inline void setMaxCardEst(CostScalar r)       { maxCardEst_ = r; }

  static short bmoGrowthPercent(CostScalar e, CostScalar m);
  // ---------------------------------------------------------------------
  // for debugging
  // ---------------------------------------------------------------------

  // add all the expressions that are local to this
  // node to an existing list of expressions
  virtual void addLocalExpr(LIST(ExprNode *) &xlist,
                            LIST(NAString) &llist) const;

  void addExplainPredicates(ExplainTupleMaster * explainTuple,
				       Generator * generator);

  ExplainTuple *addExplainInfo(ComTdb *tdb,
                                         ExplainTuple *leftChild,
                                         ExplainTuple *rightChild,
                                         Generator *generator);

  virtual ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
					      ComTdb * tdb,
					      Generator *generator);


  virtual void print(FILE * f = stdout,
                     const char * prefix = "",
                     const char * suffix = "") const;

  virtual void unparse(NAString &result,
                       PhaseEnum phase = DEFAULT_PHASE,
                       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;


  // -- Triggers
  // Count the number of nodes in the tree.
  Int32 nodeCount() const;

  // Determine whether a node of the given type exists in the tree.
  NABoolean containsNode(OperatorTypeEnum nodeType);

  // ---------------------------------------------------------------------
  // isBigMemoryOperator() returns TRUE for operators which intend to use
  // an amount of memory per cpu that is greater than the limit. It is
  // used during costing. The context is needed because we need the number of
  // CPU the operator can execute on to estimate its memory needs on each
  // CPU, and we also need the input logical properties to estimate the
  // size of the table the operator is handling. Both of them are stored
  // in the Context object. We also need to know the plan number,
  // since different plans for the same operator may consume different
  // quantities of memory. For example, a Type-2 parallel hash join vs.
  // a Type-1 parallel hash join.
  // ---------------------------------------------------------------------
  virtual NABoolean isBigMemoryOperator(const PlanWorkSpace* pws,
                                        const Lng32 planNumber);

  virtual CostScalar getEstimatedRunTimeMemoryUsage(Generator *generator, NABoolean perNode, Lng32 *numStreams = NULL) {return 0;}
  virtual double getEstimatedRunTimeMemoryUsage(Generator *generator, ComTdb * tdb) {return 0;}

  inline NABoolean isinBlockStmt() const
                          { return isinBlockStmt_; }
  inline NABoolean isinBlockStmt()
                          { return isinBlockStmt_; }
  inline void setBlockStmt(NABoolean cs)
                          { isinBlockStmt_ = cs; }

  // set the isinBlockStmt_ flag to x for the entire tree rooted
  // at this RelExpr.
  void setBlockStmtRecursively(NABoolean x);

  double calculateNoOfLogPlans(Lng32& numOfMergedExprs);

  double calculateSubTreeComplexity(NABoolean& antiSemiJoinQuery);

  // calculate a query's MJ complexity,
  // shoud be called after MJ rewrite
  double calculateQueryMJComplexity(double &n,double &n2,double &n3,double &n4);

  void makeListBySize(LIST(CostScalar) & orderedList,
                        NABoolean  recompute);

  void setFirstNRows(Int64 firstNRows) 		{ firstNRows_ = firstNRows; }
  Int64 getFirstNRows() 			{ return firstNRows_; }

  virtual PlanPriority computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws=NULL,
     Lng32 planNumber=0);

  NABoolean oltOpt() { return oltOptInfo_.oltAnyOpt(); };
  NABoolean oltOptLean() { return oltOptInfo_.oltEidLeanOpt(); };

  OltOptInfo &oltOptInfo() { return oltOptInfo_; };

  // get TableDesc from the expression. It could be directly
  // attached to the expression, as in Scan, or could be a
  // part of GroupAnalysis, as in cut-opp. For expressions
  // which do not have a tableDesc attached to them, like Join
  // it would be NULL
  TableDesc* getTableDescForExpr();

  NABoolean treeContainsEspExchange();

///////////////////////////////////////////////////////////

  virtual NABoolean pilotAnalysis(QueryAnalysis* qa);
  virtual void jbbAnalysis(QueryAnalysis* qa);
  virtual void jbbJoinDependencyAnalysis(ValueIdSet & predsWithDependencies);
  virtual void predAnalysis(QueryAnalysis* qa);
  virtual RelExpr* convertToMultiJoinSubtree(QueryAnalysis* qa);
  virtual EstLogPropSharedPtr setJBBInput(EstLogPropSharedPtr & inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));
  virtual RelExpr* expandMultiJoinSubtree();
  GroupAnalysis* getGroupAnalysis();

///////////////////////////////////////////////////////////

  // do some analysis on the initial plan
  // this is called at the end of the analysis phase
  virtual void analyzeInitialPlan();

  virtual void computeRequiredResources(RequiredResources & reqResources,
                                       EstLogPropSharedPtr & inLP = (*GLOBAL_EMPTY_INPUT_LOGPROP));

  virtual void computeMyRequiredResources(RequiredResources & reqResources,
                                      EstLogPropSharedPtr & inLP);

  // Accessor for the rowsetIterator flag.
  NABoolean isRowsetIterator() const { return rowsetIterator_; }
  // Mutator for the rowsetIterator flag.
  void setRowsetIterator(NABoolean rowsetIterator)
    { rowsetIterator_ = rowsetIterator;  }

   // Accessor for the tolerateNonFatalError flag.
  AtomicityType getTolerateNonFatalError() const { return tolerateNonFatalError_; }
  // Mutator for the tolerateNonFatalError flag.
  void setTolerateNonFatalError(AtomicityType tolerateNonFatalError)
    { tolerateNonFatalError_ = tolerateNonFatalError;  }

  // method used by generator to detrmine if rowsAffected needs to be set in PA.
  // The base class implementaion returns FALSE. FirstN and GenericUpdate are the only
  // classes that return true currently.
  virtual NABoolean computeRowsAffected() const
    {return FALSE;} ;

  inline NABoolean markedForElimination() const
  { return markedForElimination_; }
  inline void setMarkedForElimination(NABoolean b)
  { markedForElimination_ = b; }

  inline NABoolean isExtraHub() const
  { return isExtraHub_; }
  inline void setIsExtraHub(NABoolean b)
  { isExtraHub_ = b; }

  // methods to set, get and update substitute potential
  void setPotential(Int32 potential) { potential_ = potential; };
  Int32 getPotential() const { return potential_;};
  Int32 updatePotential(Int32 potential)
  {
    if ((potential != -1) &&
        (potential_ == -1))
    {
      potential_ = potential;
    }

    return potential_;
  };

  void setExpandShortRows(NABoolean val)
  { (val ? (flags_ |= EXPAND_SHORT_ROWS) : (flags_ &= ~EXPAND_SHORT_ROWS)); }

  NABoolean expandShortRows()
  {  return ((flags_ & EXPAND_SHORT_ROWS) != 0); }

  // For compressed internal format.
  // At codegen some nodes will switch from compressed internal format to
  // the exploded format when they are directly beneath the root node.
  void setParentIsRoot(NABoolean val)
  { (val ? (flags_ |= PARENT_IS_ROOT) : (flags_ &= ~PARENT_IS_ROOT)); }

  NABoolean isParentRoot()
  {  return ((flags_ & PARENT_IS_ROOT) != 0); }

  virtual int getCifBMOWeight()
  {
    return 1;
  }

  NABoolean dopReduced() const { return dopReduced_; };
  void setDopReduced(NABoolean x) { dopReduced_ = x; };

  void adjustTopPartFunc(Lng32 newDop);

  void addToPossibleIndexColumns(ValueIdSet & vids)
  { possibleIndexColumns_ += vids; }

  const ValueIdSet & possibleIndexColumns()
  { return possibleIndexColumns_; }

protected:

  void synthPropForBindChecks();

private:

  enum Flags {
    EXPAND_SHORT_ROWS  = 0x00000001     // expand short rows when added columns
   ,PARENT_IS_ROOT     = 0x00000002     // compressed internal format
  };

  // every relational expression node has the ability to perform
  // a selection on its result and to project some of its output
  // columns (or expressions on its output columns).

  ItemExpr             * selection_;   // generated by the parser
  ValueIdSet           predicates_;    // predicates represented as a set

  // A descriptor for the table derived from the RelExpr.
  RETDesc              * RETDesc_;

  // During the bind and normalization phase, each relational
  // operator has a private set of Group Attributes. Within
  // Cascades, once the operator is memoized, it shares the
  // Group Attributes for the Cascades Group.
  GroupAttributes      * groupAttr_;

  // Cascades-specific part of a relational expression
  CascadesGroupId        groupId_;     // pointer back to eq. class

  RelExpr              * groupNext_;   // list within group
  RelExpr              * bucketNext_;  // list within hash bucket
  RuleSubset           contextInsensRules_;  // context insensitive rules tried
                                             // on this expression

  RulesPerContextList  contextSensRules_; // context sensitive rules tried
                                          // per context

  // The children of an expression (its operands). Only a fixed number of
  // children can be stored here (could use a virtual operator[] to
  // overcome this, if it ever becomes necessary).
  ExprGroupId          child_[MAX_REL_ARITY];

  // The following fields are applicable only after optimization is complete.
  // After the optimal plan is selected, the following properties are transferred
  // from the CascadesPlan structure to the following placeholders for the
  // purposes of code generation.

  const PhysicalProperty * physProp_;  // physical properties for the optimal plan

  // ---------------------------------------------------------------------
  // The operator cost is the cost for this operator independent
  // of its children.
  // ---------------------------------------------------------------------
  const Cost             * operatorCost_;      // operator cost for the optimal plan

  // ---------------------------------------------------------------------
  // The roll-up cost is the cost of the sub-plan rooted at
  // this operator (operator cost of this operator combined with
  // the operator's children cost)
  // ---------------------------------------------------------------------
  const Cost             * rollUpCost_;      // roll up cost for the optimal plan
  CostScalar              estRowsUsed_; // est. # rows produced from this plan per probe
  CostScalar		  inputCardinality_;  // est. # of probes
                                             // (cardinality of input logical prop.) for this plan
  CostScalar          maxCardEst_; // maximum cardinality estimate

  // Triggers --
  // Information for Triggers, RI and IM.
  InliningInfo inliningInfo_;

  //++MV
  // Used by inlining mechanism for adding optimizer constraints
  ItemExpr *uniqueColumnsTree_;
  ValueIdSet uniqueColumns_;

  CardConstraint *cardConstraint_;

  //--MV

  SubTreeAccessSet *accessSet0_;
  SubTreeAccessSet *accessSet1_;

  // An object counter for debugging purpose only
  static THREAD_P ObjectCounter *counter_;

  NABoolean isinBlockStmt_;  // is this operator inside a compound statement?

  // if set to -1, return all rows.
  // Otherwise, return firstNRows_ at runtime.
  Int64 firstNRows_;

  UInt32 flags_;

  // various olt type optimizations. See OptUtilIncludes.h.
  // Set during preCodeGen phase.
  OltOptInfo oltOptInfo_;

  // Should this node be enabled for counting rowset rownumber counting?
  // Currently only UnPack, TupleFlow and NestedJoin nodes will have this attribute
  // set to TRUE for rowset IUD statements
  NABoolean rowsetIterator_;

  // Should this node continue execution after a Nonfatal error has been seen?
  // Currently only UnPack & Insert & NestedJoin  have this attribute set to NOT_ATOMIC for NOT ATOMIC
  // rowset insert statements. Root, PA & TupleFlow also have a similar flag set
  // in their TDBs during codegen.
  AtomicityType tolerateNonFatalError_;

  // optimizer hint
  Hint *hint_;

  // if TRUE, this join can be eliminated because
  // (a) it is guaranteed to match one and only one row
  // (b) no outputs from the inner child are used beyond this join
  // The marking is done during Join::synthLogProp() and the actual
  // removal during Join::SemanticQueryOptimization()
  NABoolean markedForElimination_ ;

  // if TRUE, this join can be placed at the top of the join chain because
  // (a) it is guaranteed to match one and only one row
  // (therefore does not increase/decrease dataflow)
  // (b) no outputs from the inner child are used beyond this join,
  // excepts as outputs of the root node (part of select list)
  // The marking is done during Join::synthLogProp() and the actual
  // reordering is done during Join::leftLinearizeTree()

  NABoolean isExtraHub_;

  // potential of the substitute
  Int32 potential_;

  // Used to help determine whether hash join 
  // optimizations may be turned on
  // This flag is set to true when an Insert/Update/Delete has been
  // seen
  NABoolean seenIUD_; 

  // the following data members were added solely for the cascades display gui
  // so we can trace each relexpr to its creator and source
  // begin relexpr tracking info
  Lng32  parentTaskId_; // {parentTaskId_,stride_} of the CascadesTask 
  short stride_;       // that created this RelExpr

  CascadesGroupId sourceGroupId_; // GroupId of creator task of this relexpr
  Int32 birthId_; // TaskCount when this relexpr was created

  ULng32 memoExprId_; // MemoExprId of this relexpr
  ULng32 sourceMemoExprId_; // MemoExprId of the source of this relexpr

  double costLimit_; // CostLimit of task's context that created this relexpr
  // end relexpr tracking info

  ExpTupleDesc::TupleDataFormat cachedTupleFormat_;
  NABoolean cachedResizeCIFRecord_;
  NABoolean cachedDefrag_;

  // Record the status whether my dop has been reduced. That is, whether
  // the partfunc pointed by my spp has a dop reduction.
  NABoolean dopReduced_;

  // if we copy an expression with copyTopNode() then
  // remember the original here, e.g. to find VEG regions
  RelExpr *originalExpr_;

  NAString operKey_;

  // Predicates are pushed down during the normalize pass. Some equality
  // predicates cannot be represented as VEGs; these do not get pushed
  // down. Nevertheless, during optimization, such an equality predicate
  // may get pushed down as a result of a Join to TSJ transformation.
  // The Optimizer (via Scan::addIndexInfo) chooses a set of interesting
  // indexes just once at the beginning of optimization; an index 
  // becomes interesting if its columns are referenced by some predicate
  // on that node. (It may become interesting for other reasons as well.)
  // Since non-VEG equality predicates don't get pushed down, columns
  // referenced in them are not available at the node at Scan::addIndexInfo
  // time. So, we have an alternate mechanism for making such column
  // references available, namely this member. During predicate push down,
  // this member holds columns that are of interest for indexes for this
  // node (if it is a Scan node) or its children (if it is not a Scan 
  // node). Note that possibleIndexColumns_ for non-leaf nodes might 
  // contain more complex sorts of expressions (e.g. VEGReferences);
  // these are eventually resolved as we push down to the leaves.
  ValueIdSet possibleIndexColumns_;

public:

  // begin: accessors & mutators for relexpr tracking info
  Lng32 getParentTaskId() { return parentTaskId_; }
  short getSubTaskId() { return stride_; }
  Int32 getBirthId() { return birthId_; }

  const NAString getCascadesTraceInfoStr();
  void setCascadesTraceInfo(RelExpr *src);
  // end: accessors & mutators for relexpr tracking info
  enum CifUseOptions
  {
    CIF_ON,
    CIF_OFF,
    CIF_SYSTEM
  };


  void cacheTupleFormatAndResizeFlag(ExpTupleDesc::TupleDataFormat tf, NABoolean r, NABoolean d)
  {
    cachedTupleFormat_ = tf;
    cachedResizeCIFRecord_ = r;
    cachedDefrag_ = d;
  }

  ExpTupleDesc::TupleDataFormat getCachedTupleFormat()
  {
    return cachedTupleFormat_;
  }

  NABoolean getCachedResizeCIFRecord()
  {
    return cachedResizeCIFRecord_;
  }

  NABoolean getCachedDefrag()
  {
    return cachedDefrag_;
  }

  CostScalar getChild0Cardinality(const Context*);

  NAString *getKey();
}; // class RelExpr

// -----------------------------------------------------------------------
// List of RelExprs.
// -----------------------------------------------------------------------
class RelExprList : public LIST (RelExpr *)
{
  public:

  RelExprList(CollHeap* h) : LIST (RelExpr *)(h) {}

  // copy ctor
  RelExprList (const RelExprList & rlist, CollHeap * h=0) :
    LIST (RelExpr *)(rlist, h) {}

  // ---------------------------------------------------------------------
  // Insert RelExpr into list order by estimated rowcount (of the logical
  // properties).
  // ---------------------------------------------------------------------
  void insertOrderByRowcount (RelExpr * expr);

  NABoolean operator== (const RelExprList &other) const;
  NABoolean operator!= (const RelExprList &other) const;

}; // RelExprList

// -----------------------------------------------------------------------
// Leaf Operator (for use in rule patterns and substitutes only).
// A leaf operator in a pattern matches any logical expression. When
// firing the rule, the binding presented to the rule contains the
// leaf operator as its leaf.
// -----------------------------------------------------------------------
class CutOp : public RelExpr
{

public:

  CutOp (Int32 index,
                CollHeap *oHeap = CmpCommon::statementHeap()) :
    RelExpr(REL_CUT_OP, NULL, NULL, oHeap)
  { index_ = index; expr_ = NULL; }

  virtual ~CutOp();

  virtual void print(FILE * f = stdout,
                     const char * prefix = "",
                     const char * suffix = "") const;
  virtual Int32 getArity () const;
  virtual NABoolean isCutOp () const;
  virtual RelExpr * copyTopNode(RelExpr *,
                                CollHeap* outHeap = 0);
  inline Int32 getIndex()                                 { return index_; }
  void setGroupIdAndAttr(CascadesGroupId groupId);
  inline RelExpr * getExpr() const                       { return expr_; }
  void setExpr(RelExpr *e);

  virtual void synthLogProp(NormWA * normWAPtr = NULL);

  virtual const NAString getText() const;

private:

  Int32 index_;           // in rules, used to distinguish multiple children
  RelExpr *expr_;       // only for bindings fixed to a particular expression

}; // CutOp

// -----------------------------------------------------------------------
// Tree operator (for use in rule patterns and substitutes only).
// A tree operator in a pattern matches any logical expression. When
// firing the rule, the binding is materialized beyond the tree operator
// all the way down to leaf nodes. Depending on whether the "all"
// data member is set to TRUE, all possible bindings for the tree node are
// or just one (randomly selected) binding is presented to the rule.
// Rules using a tree node must redefine their nextSubstitute() method
// since the generic method can not handle tree nodes.
// -----------------------------------------------------------------------
class SubtreeOp : public RelExpr
{

public:

  inline SubtreeOp (NABoolean all) : RelExpr(REL_TREE_OP) {all_ = all;}
  virtual ~SubtreeOp();
  virtual Int32 getArity() const;
  virtual NABoolean isSubtreeOp() const;
  virtual const NAString getText() const;
  virtual RelExpr * copyTopNode(RelExpr *,
                                CollHeap* outHeap = 0);
  inline NABoolean all_bindings() const { return all_; }

private:

  NABoolean       all_;            // find all bindings?

}; // SubtreeOp

// -----------------------------------------------------------------------
// A wildcard node can appear in rule patterns and rule substitutes.
// When used in a substitute, the designator of a wildcard must match
// a designator in the pattern. The wildcard is then replaced with
// the child node that matched the corresponding wildcard in the pattern.
// -----------------------------------------------------------------------
class WildCardOp: public RelExpr
{

public:

  WildCardOp(OperatorType otype,
                    Int32 designator = 0,
                    RelExpr  *child0 = NULL,
                    RelExpr  *child1 = NULL,
                    CollHeap *outHeap = CmpCommon::statementHeap())
    : RelExpr(otype,child0,child1,outHeap)
  { designator_ = designator; corrNode_ = NULL; }

  virtual ~WildCardOp();

  virtual Int32 getArity() const;

  inline Int32 getDesignator()                       { return designator_; }
  virtual NABoolean isWildcard() const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  inline void setCorrespondingNode(RelExpr *corr)    { corrNode_ = corr; }
  inline RelExpr * getCorrespondingNode()            { return corrNode_; }

  virtual const NAString getText() const;

private:

  Int32      designator_;
  RelExpr  *corrNode_;

}; // WildCardOp

// -----------------------------------------------------------------------
// A special form of a wildcard op, used to force scans on specific
// tables and indexes
// -----------------------------------------------------------------------

class ScanForceWildCard : public WildCardOp
{
  // to be used with operator type REL_FORCE_ANY_SCAN

public:

  enum scanOptionEnum
    {
      UNDEFINED,
      INDEX_SYSTEM,
      MDAM_SYSTEM,
      MDAM_FORCED,
      MDAM_OFF,
      MDAM_COLUMNS_REST_BY_SYSTEM,
      MDAM_COLUMNS_NO_MORE,
      MDAM_COLUMNS_ALL,
      COLUMN_SYSTEM,
      COLUMN_SPARSE,
      COLUMN_DENSE,
      DIRECTION_SYSTEM,
      DIRECTION_FORWARD,
      DIRECTION_REVERSED
    };

  // either use any scan on the table with a given exposed name
  // or only use an index scan on the specified index
  ScanForceWildCard(CollHeap *outHeap = CmpCommon::statementHeap());
  ScanForceWildCard(const NAString& exposedName,
                    CollHeap *outHeap = CmpCommon::statementHeap());
  ScanForceWildCard(const NAString& exposedName,
                    const NAString& indexName,
                    CollHeap *outHeap = CmpCommon::statementHeap());

  virtual ~ScanForceWildCard();

  virtual void initializeScanOptions();
  virtual const NAString getText() const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  virtual NABoolean setScanOptions(ScanForceWildCard::scanOptionEnum option);
  virtual NABoolean setIndexName(const NAString& value);
  virtual NABoolean setColumnOptions(CollIndex numColumns,
                                     ScanForceWildCard::scanOptionEnum* columnAlgos,
                                     ScanForceWildCard::scanOptionEnum mdamColumnsStatus);
  virtual NABoolean setColumnOptions(CollIndex numColumns,
                                     ScanForceWildCard::scanOptionEnum mdamColumnsStatus);
  virtual NABoolean mergeScanOptions(const ScanForceWildCard& other);
  virtual void prepare();

  virtual NABoolean doesThisCoflictMasterSwitch() const;
  inline const NAString& getExposedName() const    { return exposedName_; }
  inline const NAString& getIndexName() const      { return indexName_; }
  inline CollIndex getNumMdamColumns() const       { return numMdamColumns_;}
  inline scanOptionEnum getIndexStatus() const     { return indexStatus_;}
  inline scanOptionEnum getDirection() const       { return direction_;}
  inline scanOptionEnum getMdamStatus() const      { return mdamStatus_;}
  inline scanOptionEnum getMdamColumnsStatus() const
    { return mdamColumnsStatus_;}
  scanOptionEnum getEnumAlgorithmForColumn(CollIndex column) const;

  void setNumberOfBlocksToReadPerAccess(const Lng32& blocks)
  {
    DCMPASSERT(blocks > -1);
    numberOfBlocksToReadPerAccess_ = blocks;
  }

  // If this returns -1 it indicates that we are not forcing blocks per access...
  Lng32 getNumberOfBlocksToReadPerAccess() const
  {
    return numberOfBlocksToReadPerAccess_;
  }

  // helper function on CQS Tree
  virtual RelExpr *generateMatchingExpr(CANodeIdSet &,
                                       CANodeIdSet &,
                                       RelExpr *);
private:

  NAString exposedName_;
  NAString indexName_;
  scanOptionEnum indexStatus_;  // UNDEFINED or INDEX_SYSTEM
  scanOptionEnum mdamStatus_;   // MDAM_OFF, _FORCED, _SYSTEM, or UNDEFINED
  scanOptionEnum direction_;    // .._FORWARD, _REVERSE, _SYSTEM, or UNDEFINED
  scanOptionEnum mdamColumnsStatus_; // .._REST_BY_SYSTEM, _NO_MORE, _ALL
  CollIndex numMdamColumns_;
  scanOptionEnum* enumAlgorithms_;   // array of enumeration algorithms
                                     // for each column
  // To force the FileScan member of the same name
  Lng32 numberOfBlocksToReadPerAccess_;

};

// -----------------------------------------------------------------------
// A special form of a wildcard op, used to force joins and parallel join
// plan2 (any plan, plan1 or plan2, number of ESPs used)
// -----------------------------------------------------------------------

class JoinForceWildCard : public WildCardOp
{
  // to be used with operator types REL_FORCE_JOIN, REL_FORCE_NESTED_JOIN,
  // REL_FORCE_MERGE_JOIN, REL_FORCE_HASH_JOIN

public:

  enum forcedPlanEnum { ANY_PLAN,         // don't care which plan
                        FORCED_PLAN0,     // plan # 0
                        FORCED_PLAN1,     // plan # 1
                        FORCED_PLAN2,     // plan # 2
                        FORCED_PLAN3,     // plan # 3
                        FORCED_TYPE1,     // join matching partitions
                        FORCED_TYPE2,
                        FORCED_INDEXJOIN };   // replicate one child

  // either use any scan on the table with a given exposed name
  // or only use an index scan on the specified index
  JoinForceWildCard(OperatorTypeEnum type,
                    RelExpr *child0,
                    RelExpr *child1,
                    forcedPlanEnum plan = ANY_PLAN,
                    Int32 numOfEsps = 0,
                    CollHeap *outHeap = CmpCommon::statementHeap());

  virtual ~JoinForceWildCard();

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  inline forcedPlanEnum getPlan() const                  { return plan_; }
  inline Int32 getNumOfEsps() const                   { return numOfEsps_; }

  virtual const NAString getText() const;

  // helper function on CQS Tree
  virtual RelExpr *generateMatchingExpr(CANodeIdSet &,
                                       CANodeIdSet &,
                                       RelExpr *);

private:

  forcedPlanEnum plan_; // ANY_PLAN means don't force the plan type
  Int32 numOfEsps_;       // 0 means don't force number of ESPs

};

// -----------------------------------------------------------------------
// A special form of a wildcard op, used to force specific Exchange
// nodes (3 forms are distinguished, ESP exchange, PA node and PAPA node).
// -----------------------------------------------------------------------

class ExchangeForceWildCard : public WildCardOp
{
  // to be used with operator type REL_FORCE_EXCHANGE

public:

  enum forcedExchEnum
  {
    ANY_EXCH,              // don't care which exchange
    FORCED_PA,             // single PA node
    FORCED_PAPA,           // PAPA on top of PA node
    FORCED_ESP_EXCHANGE    // split/send nodes with ESPs
  };

  enum forcedLogPartEnum
  {
    ANY_LOGPART,           // don't care which form of logical partitioning
    FORCED_GROUP,          // PA partition grouping
    FORCED_SPLIT,          // subpartitioning or partition slicing
    FORCED_REPART          // PA grouped repartitioning
  };

  // either use any scan on the table with a given exposed name
  // or only use an index scan on the specified index
  ExchangeForceWildCard(RelExpr *child0,
                        forcedExchEnum which = ANY_EXCH,
                        forcedLogPartEnum whatLogPart = ANY_LOGPART,
                        Lng32 numBottomEsps = -1,
                        CollHeap *outHeap = CmpCommon::statementHeap());

  virtual ~ExchangeForceWildCard();

  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  inline forcedExchEnum getWhich() const                { return which_; }
  inline forcedLogPartEnum getWhichLogPart() const { return whatLogPart_;}
  inline Lng32 getHowMany() const                      { return howMany_; }

  virtual const NAString getText() const;

  // helper function on CQS Tree
  virtual RelExpr *generateMatchingExpr(CANodeIdSet &,
                                       CANodeIdSet &,
                                       RelExpr *);

private:

  forcedExchEnum    which_;       // which of the flavors of Exchange we want

  forcedLogPartEnum whatLogPart_; // which flavor of logical partitioning

  Lng32              howMany_;     // how many exchanges on the bottom

}; // ExchangeForceWildCard

// -----------------------------------------------------------------------
// A special form of a wildcard op, used to force the use of UDFs via CQS.
// -----------------------------------------------------------------------

class UDFForceWildCard : public WildCardOp
{
  // to be used with operator type REL_FORCE_ANY_SCLAR_UDF.
public:

  // Constructor used when parsing ISOLATED_SCALAR_UDF in controlDB.cpp.
  // op used is REL_FORCE_ANY_SCALAR_UDF.
  UDFForceWildCard(OperatorTypeEnum op, 
                   CollHeap *outHeap = CmpCommon::statementHeap());
  // Instantiated when parsing 'SCALAR_UDF' or 'UDF_ACTION' followed by name.
  // Uses 'FORCE_ANY_SCALAR_UDF' as operator.
  UDFForceWildCard(const NAString& functionName,
                   const NAString& actionName,
                   CollHeap *outHeap = CmpCommon::statementHeap());

  virtual ~UDFForceWildCard();

  virtual const NAString getText() const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
                                CollHeap* outHeap = 0);

  virtual NABoolean mergeUDFOptions(const UDFForceWildCard& other);

  inline const NAString& getFunctionName() const { return functionName_; }
  inline const NAString& getActionName() const   { return actionName_;   }

  // helper function on CQS Tree - this is never called.  Not implemented
  //virtual RelExpr *generateMatchingExpr(CANodeIdSet &,
  //                                      CANodeIdSet &,
  //                                      RelExpr *);
private:

  NAString functionName_;
  NAString actionName_;
}; // UDFForceWildCard


class RequiredResources : public NABasicObject {
public:
  RequiredResources() : memoryResources_(0),
                        cpuResources_(0),
                        dataAccessCost_(0),
                        maxOperMemReq_(0),
                        maxOperCPUReq_(0),
                        maxOperDataAccessCost_(0),
                        maxMaxCardinality_(0){};
  void accumulate(CostScalar memRsrcs, CostScalar cpuRsrcs, CostScalar dataAccessCost, CostScalar maxCard = csZero);
  CostScalar getMemoryResources(){ return memoryResources_; };
  CostScalar getCpuResources(){ return cpuResources_; };
  CostScalar getDataAccessCost() { return dataAccessCost_;}
  CostScalar getMaxOperMemoryResources(){ return maxOperMemReq_; };
  CostScalar getMaxOperCpuReq(){ return maxOperCPUReq_; };
  CostScalar getMaxOperDataAccessCost() { return maxOperDataAccessCost_;}
  CostScalar getMaxMaxCardinality() { return maxMaxCardinality_;}

  void setMemoryResources(CostScalar memRes) { memoryResources_ = memRes; };
  void setCpuResources(CostScalar cpuRes) { cpuResources_ = cpuRes; };
  void setDataAccessCost(CostScalar dataAccessCost) { dataAccessCost_ = dataAccessCost; };

private:
  CostScalar memoryResources_;
  CostScalar cpuResources_;
  CostScalar dataAccessCost_;
  CostScalar maxOperMemReq_;
  CostScalar maxOperCPUReq_;
  CostScalar maxOperDataAccessCost_;
  CostScalar maxMaxCardinality_;
}; // RequiredResources

// *********************************************************************
// The following five enumerated types are specific to plan generation.
// They are housed in this file so that many derived classes of
// RelExpr can use them. Another alternative is to create a separate
// ".h" file for them.
// *********************************************************************

// ---------------------------------------------------------------------
// Location where a plan or a plan fragment should be executed.
//
// EXECUTE_IN_MASTER_AND_ESP
//   Can execute in the master and also can execute in an ESP
// EXECUTE_IN_MASTER_OR_ESP
//   Used for requirements only: require EXECUTE_IN_MASTER_AND_ESP
//   or EXECUTE_IN_MASTER or EXECUTE_IN_ESP
// EXECUTE_IN_MASTER
//   Execute ONLY in the root exector server process that is at
//   thr root of the tree of processes.
// EXECUTE_IN_ESP
//   Execute in an executor server process (ESP) only
// EXECUTE_IN_DP2
//   Execute in a disk server process.
// ---------------------------------------------------------------------
enum PlanExecutionEnum
  {
     EXECUTE_IN_MASTER_AND_ESP,
     EXECUTE_IN_MASTER_OR_ESP,
     EXECUTE_IN_MASTER,
     EXECUTE_IN_ESP,
     EXECUTE_IN_DP2
  };

// ---------------------------------------------------------------------
// Source for the data.
// 1) A persistent table.
// 2) An SQL  temporary table.
// 3) A table that is materialized transiently at the optimizer's behest.
// 4) A virtual table, that is, a table-values stored procedure.
// 5) A tuple provided by the user.
// 6) An ESP where the different data streams are dependent on each
//    other in the sense that if one were to read only from some of
//    them and not from others one could run into a deadlock
//    (this is true for repartitioned data)
// 7) An ESP where the individual data streams can be read independently
//    from each other without causing a deadlock
// ---------------------------------------------------------------------
enum DataSourceEnum
  {
    SOURCE_PERSISTENT_TABLE,
    SOURCE_TEMPORARY_TABLE,
    SOURCE_TRANSIENT_TABLE,
    SOURCE_VIRTUAL_TABLE,
    SOURCE_TUPLE,
    SOURCE_ESP_DEPENDENT,
    SOURCE_ESP_INDEPENDENT
  };

// ---------------------------------------------------------------------
// The next enum is used for both synthesized and required sort
// order types. Only the first four are valid as a synthesized
// sort order type. The meaning of each literal is slightly
// different depending on whether it is being used as a requirement
// or a synthesized value.
// ---------------------------------------------------------------------
// Synthesized Sort Order Type (SOT)
//
// NO_SOT
//   The spp contains no sort order, or we are in DP2 and the
//   sort order type is irrelevant.
// ESP_NO_SORT_SOT
//   The sort order in the spp was generated naturally via a disk
//   process access path. If necessary, a merge of sorted streams
//   or synchronous access will be done to maintain the order.
// ESP_VIA_SORT_SOT
//   The sort order in the spp was generated by a sort operator.
//   If necessary, a merge of sorted streams will be done to
//   maintain the order.
// DP2_SOT
//   The sort order in the spp is only valid in DP2. Synchronous
//   access or a merge of sorted streams will not be done. The
//   sort order is only usable by another operator if it is also
//   in DP2 and the physical partitioning function of it's access
//   path matches exactly the physical partitioning function in
//   this operator's spp.
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Sort Order Type (SOT) Requirement
//
// NO_SOT
//   There is no required sort order or arrangement in the rpp, or we
//   are in DP2 and the sort order type is irrelevant.
// ESP_NO_SORT_SOT
//   A complete executor process order is required, and the child is
//   is not allowed to sort to satisfy the requirement.
// ESP_VIA_SORT_SOT
//   A complete executor process order is required, and the child must
//   utilize sort to satisfy the requirement.
// DP2_SOT
//   The required sort order or arrangement must only be valid in
//   DP2. A complete executor process order is not necessary and does
//   NOT satisfy the requirement. This means a merge of sorted
//   streams or synchronous access cannot be done.
// ESP_SOT
//   A complete executor process order is required. This can be
//   achieved naturally or via a sort.
// DP2_OR_ESP_NO_SORT_SOT
//   A synthesized sort order type of ESP_NO_SORT or DP2 satisfies
//   this sort order type requirement.
// INCOMPATIBLE_SOT
//   Used to signify that two required sort order types are not
//   compatible with each other.
// ---------------------------------------------------------------------
enum SortOrderTypeEnum
{
  NO_SOT,
  ESP_NO_SORT_SOT,
  ESP_VIA_SORT_SOT,
  DP2_SOT,
  ESP_SOT,
  DP2_OR_ESP_NO_SORT_SOT,
  INCOMPATIBLE_SOT
};

// -----------------------------------------------------------------------
// Inlines that couldn't be defined till now due to dependencies
// -----------------------------------------------------------------------
inline RelExpr * ExprGroupId::getPtr() const
{
  // part of fix to genesis case: 10-981030-4975
  if (node_) node_->checkInvalidObject(this);
  // else plan is not valid
  return node_;
}

inline RelExpr * ExprGroupId::operator ->() const       { return getPtr(); }
inline ExprGroupId::operator RelExpr *() const          { return getPtr(); }

#endif /* RELEXPR_H */
