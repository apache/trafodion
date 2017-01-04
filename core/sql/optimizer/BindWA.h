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
#ifndef BindWA_H
#define BindWA_H
/* -*-C++-*-
*************************************************************************
*
* File:         BindWA.h
* Description:  The workarea used by the name binder
* Created:      4/27/94
* Language:     C++
*
*
*	But when his friends did understand
*	His fond and foolish mind,
*	They sent him up to fair London,
*	An apprentice for to bind.
*		-- The Ballad of the Bailiff's Daughter of Islington
*
*************************************************************************
*/

#include "BaseTypes.h"
#include "Collections.h"
#include "ColumnNameMap.h"
#include "CmpContext.h"
#include "NAMemory.h"
#include "ObjectNames.h"
#include "OperTypeEnum.h"
#include "Rel3GL.h"
#include "RelMisc.h"
#include "RelRoutine.h"
#include "RETDesc.h"
#include "TableNameMap.h"
#include "ValueDesc.h"
#include "OptUtilIncludes.h"
#include "ComSchemaName.h" // for ComSchemaName

// ----------------------------------------------------------------------
// contents of this file
// ----------------------------------------------------------------------
class BindContext;
class MvBindContext;
class XTNMStack;
class BindScope;
class BindScopeList;
class HostArraysWA;
class BindWA;

// ----------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------
class CheckConstraint;
class CmpContext;
class GenericUpdate;
class ItemList;
class Join;
class ParNameLocList;
class SchemaDB;
class StmtDDLCreateView;
class StmtLevelAccessOptions;
class StmtDDLCreateTrigger;
struct TrafDesc;
class NARoutine;
class HbaseColUsageInfo;
class ExeUtilHbaseCoProcAggr;
class CommonSubExprRef;

// ***********************************************************************
// BindContext
//
// The context in which the current node is being bound.
//
// ***********************************************************************

#define NOT_UpdateOrInsert NO_OPERATOR_TYPE

class BindContext : public NABasicObject
{
public:
  // --------------------------------------------------------------------
  // Constructor function -- additional code in BindWA::initNewScope()
  // --------------------------------------------------------------------
  BindContext()
  : inSelectList_		(0)
  , inOrderBy_			(FALSE)
  , inExistsPredicate_		(FALSE)
  , inGroupByClause_		(FALSE)
  , inScalarGroupBy_		(TRUE)
  , inWhereClause_		(FALSE)
  , inHavingClause_		(FALSE)
  , inSubquery_			(FALSE)
  , inRowSubquery_		(FALSE)
  , inUnion_                    (FALSE)
  , lookAboveToDecideSubquery_  (FALSE)
  , counterForRowValues_	(NULL)
  , updateOrInsertNode_		(NULL)
  , updateOrInsertScope_	(NULL)
  , inUpdateOrInsert_		(NOT_UpdateOrInsert)
  , deleteNode_		 	(NULL)
  , deleteScope_		(NULL)
  , inCheckConstraint_		(NULL)
  , inRIConstraint_		(FALSE)
  , inAggregate_		(FALSE)
  , colRefInAgg_		(FALSE)
  , outerColRefInAgg_		(FALSE)
  , PLACEHOLDER1_		(FALSE)
  , inTupleList_		(FALSE)
  , unaggColRefInSelectList_	(NULL)
  , aggScope_			(NULL)
  , inItemList_			(NULL)
  , inJoin_			(NULL)
  , inJoinPred_			(NULL)
  , inRangePred_		(FALSE)
  , inMultaryPred_		(NULL)
  , inRowsSince_                (FALSE)
  , inOtherSequenceFunction_    (FALSE)
  , inGroupByOrdinal_           (FALSE)
  , inGroupByExpr_              (FALSE)
  , inCreateAfterTrigger_       (FALSE)
  , inCheckOption_              (FALSE)
  , inCreateView_               (FALSE)
  , inTransposeClause_          (FALSE)
  , inSequenceFunction_         (FALSE)
  , inPredicate_		(FALSE)
  , firstN_                     (FALSE)
  , triggerObj_			(NULL)
  , pMvBindContext_		(NULL)
  , stmtLevelAccessOptions_	(NULL)
  , inQualifyClause_            (FALSE)
  , inTDFunction_               (FALSE)
  , inUDFunction_               (FALSE)
  , inPrecOlapFunction_         (FALSE)
  , inOlapOrderBy_              (FALSE)
  , inOlapPartitionBy_          (FALSE)
  , inComputedColumnExpr_       (FALSE)
  {
    CMPASSERT(FALSE != REL_UPDATE && FALSE != REL_INSERT);
  }

  // --------------------------------------------------------------------
  // Accessor/mutator functions
  // --------------------------------------------------------------------
  Int32		   &inSelectList()    	   { return inSelectList_; }
  NABoolean	   &inOrderBy()    	   { return inOrderBy_; }
  NABoolean	   &inExistsPredicate()	   { return inExistsPredicate_; }
  NABoolean	   &inGroupByClause() 	   { return inGroupByClause_; }
  NABoolean	   &inScalarGroupBy() 	   { return inScalarGroupBy_; }
  NABoolean	   &inWhereClause()        { return inWhereClause_; }
  NABoolean	   &inHavingClause() 	   { return inHavingClause_; }
  NABoolean	   &inSubquery()     	   { return inSubquery_; }
  NABoolean	   &inRowSubquery()	   { return inRowSubquery_; }
  NABoolean        &inUnion()              { return inUnion_; }
  NABoolean        &lookAboveToDecideSubquery() { return lookAboveToDecideSubquery_; }
  NABoolean	   &inAggregate() 	   { return inAggregate_; }
  NABoolean	   &colRefInAgg() 	   { return colRefInAgg_; }
  NABoolean	   &outerColRefInAgg()	   { return outerColRefInAgg_; }
  NABoolean	   &PLACEHOLDER1()	   { return PLACEHOLDER1_; }
  NABoolean	   &inTupleList()	   { return inTupleList_; }
  ItemExpr	  *&unaggColRefInSelectList() { return unaggColRefInSelectList_;}
  BindScope	  *&aggScope()		   { return aggScope_; }
  ItemList	  *&inItemList()	   { return inItemList_; }
  Join		  *&inJoin()		   { return inJoin_; }
  ItemExpr	  *&inJoinPred()	   { return inJoinPred_; }

  NABoolean	   &inRangePred()	   { return inRangePred_;}
  ItemExpr	  *&inMultaryPred()	   { return inMultaryPred_; }
  CollIndex	  *&counterForRowValues()  { return counterForRowValues_; }
  GenericUpdate	  *&updateOrInsertNode()   { return updateOrInsertNode_; }
  BindScope	  *&updateOrInsertScope()  { return updateOrInsertScope_; }
  OperatorTypeEnum &inUpdateOrInsert() 	   { return inUpdateOrInsert_; }
  NABoolean	    inUpdate() const	   { return inUpdateOrInsert_==REL_UPDATE;}
  NABoolean	    inInsert() const	   { return inUpdateOrInsert_==REL_INSERT;}
  RelExpr	  *&deleteNode()	   { return deleteNode_; }
  BindScope	  *&deleteScope()	   { return deleteScope_; }
  NABoolean        &inCreateAfterTrigger() { return inCreateAfterTrigger_; }
  NABoolean        &inCheckOption()        { return inCheckOption_; }
  NABoolean        &inCreateView()         { return inCreateView_; }
  NABoolean        &inTransposeClause()    { return inTransposeClause_; }
  NABoolean        &inSequenceFunction()   { return inSequenceFunction_; }
  NABoolean	   &inPredicate()          { return inPredicate_; }
  NABoolean        &firstN()               { return firstN_; }
  NABoolean	   &inRIConstraint()	   { return inRIConstraint_; }
  CheckConstraint *&inCheckConstraint()	   { return inCheckConstraint_; }
  NABoolean	    inTableCheckConstraint() const {
				return inCheckConstraint_ &&
				!inCheckConstraint_->isViewWithCheckOption(); }
  NABoolean	    inViewCheckConstraint() const {
				return inCheckConstraint_ &&
				inCheckConstraint_->isViewWithCheckOption(); }
  NABoolean	    inAnyConstraint() const {
				return inCheckConstraint_ || inRIConstraint_; }

  CollIndex counterForRowValuesIncr()	{ return ++(*counterForRowValues_); }

  NABoolean        &inRowsSince()             { return inRowsSince_; }
  NABoolean        &inOtherSequenceFunction() { return inOtherSequenceFunction_; }
  NABoolean        &inGroupByOrdinal() { return inGroupByOrdinal_; }
  NABoolean        &inGroupByExpr() { return inGroupByExpr_; }
  NABoolean        &inQualifyClause() { return inQualifyClause_; }
  NABoolean        &inTDFunction() { return inTDFunction_; }
  NABoolean        &inUDFunction() { return inUDFunction_; }

  NABoolean        &inPrecOlapFunction() { return inPrecOlapFunction_; }
  
  NABoolean        &inOlapOrderBy() { return inOlapOrderBy_; }
  NABoolean        &inOlapPartitionBy() { return inOlapPartitionBy_; }
  NABoolean        &inComputedColumnExpr() { return inComputedColumnExpr_; }
  StmtDDLCreateTrigger *&triggerObj(){ return triggerObj_; }

  //++ MV
  inline MvBindContext *&getMvBindContext() { return pMvBindContext_; }

  const StmtLevelAccessOptions *stmtLevelAccessOptions() const {
  				return stmtLevelAccessOptions_;}
  void setStmtLevelAccessOptions(StmtLevelAccessOptions &ao)
  {
    if (!stmtLevelAccessOptions_) stmtLevelAccessOptions_ = &ao;
    else { CMPASSERT(*stmtLevelAccessOptions_ == ao); }
  }

private:
  // --------------------------------------------------------------------
  // True if binding a select list.
  // --------------------------------------------------------------------
  Int32 inSelectList_;

  // --------------------------------------------------------------------
  // True if binding an order-by list.
  // --------------------------------------------------------------------
  NABoolean inOrderBy_;

  // --------------------------------------------------------------------
  // True if simply contained within an EXISTS predicate.
  // --------------------------------------------------------------------
  NABoolean inExistsPredicate_;

  // --------------------------------------------------------------------
  // True if binding the GROUP BY list.
  // --------------------------------------------------------------------
  NABoolean inGroupByClause_;

  // --------------------------------------------------------------------
  // True if if we haven't seen a group by list
  // --------------------------------------------------------------------
  NABoolean inScalarGroupBy_;

  // --------------------------------------------------------------------
  // True if binding a WHERE clause or the common columns of a NATURAL JOIN
  // --------------------------------------------------------------------
  NABoolean inWhereClause_;

  // --------------------------------------------------------------------
  // True if binding a HAVING clause.
  // --------------------------------------------------------------------
  NABoolean inHavingClause_;

  // --------------------------------------------------------------------
  // True if binding a subquery.
  // --------------------------------------------------------------------
  NABoolean inSubquery_;

  // --------------------------------------------------------------------
  // True if binding a row subquery.
  // --------------------------------------------------------------------
  NABoolean inRowSubquery_;

  // --------------------------------------------------------------------
  // True if binding the children of UNION.
  // --------------------------------------------------------------------
  NABoolean inUnion_;

  // --------------------------------------------------------------------
  // True if creating trigger of type after.
  // --------------------------------------------------------------------
  NABoolean inCreateAfterTrigger_;

  // --------------------------------------------------------------------
  // True if in check option of create view.
  // --------------------------------------------------------------------
  NABoolean inCheckOption_;

  // --------------------------------------------------------------------
  // True if in Create view.
  // --------------------------------------------------------------------
  NABoolean inCreateView_;

  // --------------------------------------------------------------------
  // True if in transpose clause.
  // --------------------------------------------------------------------
  NABoolean inTransposeClause_;

  // --------------------------------------------------------------------
  // True if in a sequence function.
  // --------------------------------------------------------------------
  NABoolean inSequenceFunction_;

  // --------------------------------------------------------------------
  // True if in a predicate.
  // --------------------------------------------------------------------
  NABoolean inPredicate_;

  // --------------------------------------------------------------------
  // True if a '(first N)' clause is present.
  // --------------------------------------------------------------------
  NABoolean firstN_;

  // --------------------------------------------------------------------
  // True if binding the children of a node who might look at this node's
  // scope to determine whether they (the children) are inside a subquery;
  // use this flag (if TRUE) to say, hey, look at your "grandparent"'s
  // scope instead.
  //
  // See the comments in bindRowValues(), BindRelExpr.cpp, for a more
  // complete discussion of how/why this data member is used.
  // --------------------------------------------------------------------
  NABoolean lookAboveToDecideSubquery_;

  // --------------------------------------------------------------------
  // Non-NULL if binding children of a Tuple, in which case dereferencing
  // it gives the current child number.
  // --------------------------------------------------------------------
  CollIndex *counterForRowValues_;

  // --------------------------------------------------------------------
  // Non-NULL if binding a check constraint (table check constraint OR
  // view with check option).
  // --------------------------------------------------------------------
  CheckConstraint *inCheckConstraint_;

  // --------------------------------------------------------------------
  // TRUE if binding a referential integrity constraint.
  // --------------------------------------------------------------------
  NABoolean inRIConstraint_;

  // --------------------------------------------------------------------
  // Non-NULL if binding an INSERT or UPDATE; else NULL.
  // --------------------------------------------------------------------
  GenericUpdate *updateOrInsertNode_;
  BindScope	*updateOrInsertScope_;
  // REL_INSERT or REL_UPDATE if binding an INSERT or UPDATE; else FALSE.
  OperatorTypeEnum inUpdateOrInsert_;

  // --------------------------------------------------------------------
  // Non-NULL if binding a DELETE; else NULL.
  // --------------------------------------------------------------------
  RelExpr   *deleteNode_;
  BindScope *deleteScope_;

  // --------------------------------------------------------------------
  // True if binding the operand of an aggregate.
  // --------------------------------------------------------------------
  NABoolean inAggregate_;

  // --------------------------------------------------------------------
  // True if binding an aggregate that contains a column reference.
  // --------------------------------------------------------------------
  NABoolean colRefInAgg_;

  // --------------------------------------------------------------------
  // True if binding an aggregate that contains an outer column reference.
  // --------------------------------------------------------------------
  NABoolean outerColRefInAgg_;

  NABoolean PLACEHOLDER1_;

  // --------------------------------------------------------------------
  // True if binding "VALUES(...),(...), ..."
  // --------------------------------------------------------------------
  NABoolean inTupleList_;

  // --------------------------------------------------------------------
  // Non-null if we have bound an unaggregated column reference contained
  // in a select list on a (so far) ungrouped table.
  // --------------------------------------------------------------------
  ItemExpr *unaggColRefInSelectList_;

  // --------------------------------------------------------------------
  // The column reference's local scope (whether the reference is outer
  // or not!).
  // --------------------------------------------------------------------
  BindScope *aggScope_;

  // --------------------------------------------------------------------
  // True if binding an ItemList (row value constructor).
  // --------------------------------------------------------------------
  ItemList *inItemList_;

  // --------------------------------------------------------------------
  // Non-null if binding a JOIN of any type.
  // Non-null if binding the predicate (ON-clause) of a JOIN of any type.
  // --------------------------------------------------------------------
  Join     *inJoin_;
  ItemExpr *inJoinPred_;

  //---------------------------------------------------------------------
  // TRUE only if currently binding a range predicate otherwise false
  //---------------------------------------------------------------------
  NABoolean inRangePred_;

  // --------------------------------------------------------------------
  // Non-null if binding a "multary" predicate, i.e., greater than unary:
  // any BiRelat (<,=,>=, etc) or Function (BETWEEN, CASE, LIKE, ABS, etc)
  // --------------------------------------------------------------------
  ItemExpr *inMultaryPred_;

  // --------------------------------------------------------------------
  // TRUE if inside a ROWS SINCE function
  // --------------------------------------------------------------------
  NABoolean inRowsSince_;

  // --------------------------------------------------------------------
  // TRUE if inside a sequence function other than ROWS SINCE.
  // --------------------------------------------------------------------
  NABoolean inOtherSequenceFunction_;

  // if set, then do not validate that all columns that are part of this expr 
  // belong to group by list. Check done in ColReference::bindNode.
  // Set when a group by element is an ordinal.
  NABoolean inGroupByOrdinal_;

  // Set when a group by element is an expr.
  NABoolean inGroupByExpr_;

  // --------------------------------------------------------------------
  // A pointer a the trigger object.
  // This pointer is used when in the binding of the trigger action.
  // --------------------------------------------------------------------
  StmtDDLCreateTrigger *triggerObj_;


  // -------------------------------------------------------------------------
  //++ MV
  // general purpose mv context.
  // NULL if no context exists.
  // -------------------------------------------------------------------------
  MvBindContext *pMvBindContext_;

  // --------------------------------------------------------------------
  // NonNULL if Tandem-extension "FOR xxx ACCESS [IN locking MODE]" was
  // specified anywhere in this scope (query or subquery).
  // --------------------------------------------------------------------
  StmtLevelAccessOptions *stmtLevelAccessOptions_;

  NABoolean inQualifyClause_;
  NABoolean inTDFunction_;

  NABoolean inPrecOlapFunction_;
  NABoolean inOlapPartitionBy_;
  NABoolean inOlapOrderBy_;

  // --------------------------------------------------------------------
  // TRUE if we are in a UDFunction
  // --------------------------------------------------------------------
  NABoolean inUDFunction_;

  NABoolean inComputedColumnExpr_;
}; // class BindContext

// ***********************************************************************
// MV --
// The MvBindContext class controls which Scans will be on base tables
// and which will be replaced by some other RelExpr tree. 
// All the tables that their names are added to the replacementTreeHash_
// will be switched to the corresponding tree, that was already constructed.
// To bind a sub-tree with no switching at all, use an empty object.
// A pointer to the MvBindContext object is first put in the RelRoot node
// on top of the sub-tree. When being bound, the RelRoot will copy that 
// pointer to the context of the BindScope it just created. There it will
// be found by Scan::bindNode().
//
class MvBindContext : public NABasicObject
{
public:
  MvBindContext(CollHeap *heap)
    : replacementTreeHash_(QualifiedName::hash, 
                           (Lng32)10, // initialHashSize
			   TRUE, // enforceUniqueness 
			   heap),
      builder_(NULL),
      heap_(heap)
  {}

  virtual ~MvBindContext();

  void setReplacementFor(const QualifiedName *tableName, RelExpr *replacementTree);
  RelExpr *getReplacementFor(const QualifiedName& tableName) const;

  // These are for passing the refresh builder to the Scan::bindNode() code
  // in order to continue building the bottom side of the refresh tree.
  void setRefreshBuilder(MvRefreshBuilder *builder) 
  { builder_ = builder; }
  MvRefreshBuilder *getRefreshBuilder() const 
  { return builder_; }
  
private:
  typedef NAHashDictionary<const QualifiedName, RelExpr> ReplacementTreeEntry;

  ReplacementTreeEntry        replacementTreeHash_;
  MvRefreshBuilder           *builder_;
  CollHeap                   *heap_;
}; // MvBindContext


// ***********************************************************************
// XTNM Stack
//
// A stack of the table name scopes for the current select.
//
// ***********************************************************************
class XTNMStack : public LIST(XTNM *)
{
public:
  XTNMStack(CollHeap *h = NULL) : LIST(XTNM *)(h)	{ wHeap_ = h; }
  void createXTNM() 	{ insert(new(wHeap_) XTNM(wHeap_)); }
  XTNM *currentXTNM() 	{ CMPASSERT(!isEmpty()); return at(entries()-1); }
  void removeXTNM()	{ XTNM *xtnm; if (getLast(xtnm)) delete xtnm; }
  ~XTNMStack()		{ while (!isEmpty()) removeXTNM(); }
private:
  CollHeap *wHeap_;
}; // class XTNMStack

// ***********************************************************************
// BindScope
//
// Each SQL SELECT provides a new scope for name binding according to
// the rules specified in the ANSI SQL standard.  The table references
// that appear in the FROM clause have their names exposed in the scope
// clause of the SELECT.  This means that columns belonging to such
// tables may be freely referenced anywhere in the select list, the
// WHERE clause, the GROUP BY clause and the HAVING clause.  ON clause
// predicates may also reference column belonging to tables whose names
// have been exposed.  However, the ON clause can only see those exposed
// table or correlation names that are referenced in the query subtree
// rooted in its join operator.
//
// The name binder has to implement the above scope rules and issue an
// error when a query violates the semantics described by the ANSI SQL
// standard.  The BindScope class is used during name resolution for the
// different types of table references that can appear in a query, namely,
//   1) simple table name
//   2) derived table
//   3) joined table
//
// The binder requires one BindScope per SELECT statement in the SQL query.
// Typically, a SELECT statement can be encountered anywhere within a query.
// It can even be deeply nested within a number of other SELECT statements.
// For this reason, the binder maintains a stack of BindScope to preserve
// the state of processing for outer SELECTs.
//
// One BindScope is allocated together with the BindWA even before
// the binder begins the tree walk.  Since this BindScope is allocated
// before any others are, it is always at the bottom of the BindScope
// stack while name binding is in progress.  It is not associated with
// any SQL SELECT and is maintained only to represent constants, host
// variables and dynamic parameters as outer references from the
// "user's" scope.  Whenever the binder encounters a constant, host
// variable or a dynamic parameter it updates the Outer References
// set in the present BindScope and also updates the XCNM of the BindWA.
//
// ***********************************************************************
class BindScope : public NABasicObject
{
public:

  // --------------------------------------------------------------------
  // Constructor function
  // --------------------------------------------------------------------
  BindScope(BindWA *bindWA = NULL);

  // --------------------------------------------------------------------
  // Destructor function
  // Each BindScope is destroyed when the binder exits an SQL scope.
  // --------------------------------------------------------------------
  ~BindScope();

  // --------------------------------------------------------------------
  // Accessor functions
  // getTablesInScope() is documented with sister func BindWA::getTablesInScope.
  // --------------------------------------------------------------------
  XTNMStack *xtnmStack() 	 	  { return &xtnm_; }
  XTNM *getXTNM() 	 		  { return xtnmStack()->currentXTNM(); }
  BindContext *context() 		  { return &context_; }

  void getTablesInScope(LIST(TableNameMap*) &xtnmList,
                        NAString *formattedList = NULL) const;

  // --------------------------------------------------------------------
  // Methods for the table derived from the current RelExpr.
  // --------------------------------------------------------------------
  RETDesc *getRETDesc() const 	  	  { return RETDesc_; }
  void setRETDesc(RETDesc *retdesc) 	  { RETDesc_ = retdesc; }

  // --------------------------------------------------------------------
  // Methods for Local References:
  // The Local References set contains the ValueIds for columns whose
  // names are exposed in the current scope.  It is maintained primarily
  // to allow a fast test for checking whether a ValueId belongs to the
  // current scope.
  // --------------------------------------------------------------------
  const ValueIdSet &getLocalRefs() const  { return localRefs_; }
  NABoolean isLocalRef(ValueId vid) const { return localRefs_.contains(vid); }
  void addLocalRef(ValueId vid) 	  { localRefs_.insert(vid); }
  void removeLocalRef(ValueId vid) 	  { localRefs_.remove(vid); }  // -- Triggers

  // --------------------------------------------------------------------
  // Methods for Outer References:
  // The outer References set contains the ValueIds of all outer
  // references that are provided as inputs to the current scope.
  // They include constants, host variables, dynamic parameters
  // as well as columns that belong to an outer scope.  Before the
  // binder exits the current BindScope it augments the set of
  // outer references of its parent scope.
  // --------------------------------------------------------------------

  // --------------------------------------------------------------------
  // Add a ValueId for an outer reference (a constant, host variable,
  // a dynamic parameter or a correlated column reference) to the
  // Outer References set.
  // --------------------------------------------------------------------
  const ValueIdSet &getOuterRefs() const	{ return outerRefs_; }
  void addOuterRef(ValueId vid) 		{ outerRefs_.insert(vid); }
  void removeOuterRefs(ValueIdSet vids)         { outerRefs_ -= vids; }

  // --------------------------------------------------------------------
  // mergeOuterRefs() is called by the parent BindScope to merge the
  // outer references of its child according to the following rule:
  // parent's outer references +=
  //     current's outer references
  //     - parent's local references
  // when keepLocalRefs = TRUE, they are retained (for merge)
  // Note that the + and - operators correspond to the set union and
  // set difference operations.
  // --------------------------------------------------------------------
  void mergeOuterRefs(const ValueIdSet &other, NABoolean keepLocalRefs);

  // --------------------------------------------------------------------
  // Accessor/mutator method for unresolvedAggregates_
  // --------------------------------------------------------------------
  ValueIdSet &getUnresolvedAggregates() { return unresolvedAggregates_; }

  // Add an aggregate to the set of unresolved aggregates.
  // Look for equivalent aggregates in set.
  // Return ValueId of aggr passed in or existing equivalent.
  //
  ValueId addUnresolvedAggregate(ValueId aggrId);
  //
  ValueId getEquivalentItmSequenceFunction(ValueId seqId);

  // --------------------------------------------------------------------
  // Accessor/mutator method for unresolvedSequenceFunctions_
  // --------------------------------------------------------------------
  ValueIdSet &getUnresolvedSequenceFunctions()
  { return unresolvedSequenceFunctions_; }

  // --------------------------------------------------------------------
  // Accessor/mutator method for allSequenceFunctions_
  // This method has a list of all sequencefunctions in this scope.
  // --------------------------------------------------------------------
  ValueIdSet &getAllSequenceFunctions()
  { return allSequenceFunctions_; }

  // --------------------------------------------------------------------
  // Accessor/mutator method for sequenceNode
  // --------------------------------------------------------------------
  RelExpr *&getSequenceNode() { return sequenceNode_; };

  const ValueIdMap &getNcToOldMap() { return ncToOldMap_;}
  void setNCToOldMap(ValueIdMap vmap) {ncToOldMap_ = vmap; }

  ValueIdList getOlapPartition() const {return OlapPartition_; };

  ValueIdList getOlapOrder() const {return OlapOrder_; };

  NABoolean getIsFirstOlapWindowSpec() { return isFirstOlapWindowSpec_  ;}

  void setOlapPartition(ValueIdList  part)  { OlapPartition_ = part; };

  void setOlapOrder( ValueIdList  ord)  { OlapOrder_ = ord ; };
  
  void setIsFirstOlapWindowSpec( NABoolean v) { isFirstOlapWindowSpec_ = v  ;}


  ItmSequenceFunction * getOlapPartitionChange() const { return OlapPartitionChange_; }; 
  
  void setOlapPartitionChange(ItmSequenceFunction * v ) { OlapPartitionChange_ = v ; };

  enum HasOlapFunctionsEnum { OLAPUNKNOWN_, OLAP_ , NONOLAP_};

  HasOlapFunctionsEnum getHasOlapSeqFunctions() const { return HasOlapSeqFunctions_;};

  void setHasOlapSeqFunctions (HasOlapFunctionsEnum v) {HasOlapSeqFunctions_=v;};

  void resetOlapInfo( )  { 
    OlapPartition_.clear(); 
    OlapOrder_.clear(); 
    OlapPartitionChange_ = NULL; 
    HasOlapSeqFunctions_ = OLAPUNKNOWN_; 
  };

  void setInViewExpansion(NABoolean val) {  inViewExpansion_ = val; }
  NABoolean getInViewExpansion() { return inViewExpansion_; }

private:

  // --------------------------------------------------------------------
  // The Exposed Table Name Map associates an exposed table name
  // with its ValueId.
  // The Exposed Column Name Map associates the name of a column
  // whose tablename is exposed with its ValueId.
  // --------------------------------------------------------------------
  XTNMStack xtnm_;

  // --------------------------------------------------------------------
  // Local References Set.
  // ValueIds of columns whose names are exposed in the current scope.
  // --------------------------------------------------------------------
  ValueIdSet localRefs_;

  // --------------------------------------------------------------------
  // Outer References Set.
  // This set contains the ValueIds of all outer references that are
  // provided as inputs to the current scope.  They include constants,
  // host variables, dynamic parameters as well as columns that
  // belong to an outer scope.  Before the binder exits the current
  // BindScope it augments the set of outer references of its parent
  // scope according to the rule:
  // parent's outerRefs_ += current's outerRefs_
  //                        - parent's localRefs_
  // Note that the + and - operators correspond to the set union and
  // set difference operations.
  // --------------------------------------------------------------------
  ValueIdSet outerRefs_;

  // --------------------------------------------------------------------
  // A descriptor for the table derived from the current RelExpr.
  // --------------------------------------------------------------------
  RETDesc *RETDesc_;

  // --------------------------------------------------------------------
  // The unresolved aggregates list contains the value ids of aggregates
  // that were found in parse trees and that need to be relocated to
  // a groupby node.  The binder assigns a value id to those aggregate
  // functions, replaces the original expression with a reference to that
  // value id, and at stores the value id in this list.
  // --------------------------------------------------------------------
  ValueIdSet unresolvedAggregates_;

  // --------------------------------------------------------------------
  // The unresolved sequence functions list contains the value ids of
  // sequence functions that were found in parse trees and that need
  // to be relocated to a RelSequence node.  The binder assigns a
  // value id to those sequence functions, replaces the original
  // expression with a reference to that value id, and it stores the
  // value id in this list.
  // --------------------------------------------------------------------
  ValueIdSet unresolvedSequenceFunctions_;

  // --------------------------------------------------------------------
  // The all sequence functions list contains the value ids of
  // all sequence functions that were found in parse trees and that need
  // to be relocated to a RelSequence node.  
  // --------------------------------------------------------------------
  ValueIdSet allSequenceFunctions_;

  // The sequence node (if any) associated with this scope.  The
  // unresolved sequence functions will be attached to this node.  If
  // unresolved sequence functions exist, but there is no sequence
  // node, an error is issued.
  //
  RelExpr *sequenceNode_;
  ValueIdMap ncToOldMap_;
  // --------------------------------------------------------------------
  // Context info for this scope.
  // --------------------------------------------------------------------
  BindContext context_;

  BindWA *bindWA_;  // a bindWA* to get the CollHeap* for environment setup


  ValueIdList OlapPartition_;
  ValueIdList OlapOrder_;
  NABoolean isFirstOlapWindowSpec_;
  ItmSequenceFunction *OlapPartitionChange_; 
  HasOlapFunctionsEnum HasOlapSeqFunctions_;
  
  // copy from BindWA for override_schema
  // to allow subqueries after a view to be considered in override_schema
  NABoolean inViewExpansion_; 
  
}; // class BindScope

// ***********************************************************************
// BindScopeList
//
// The binder requires one BindScope per SELECT statement in the SQL
// query.  Typically, a SELECT statement can be encountered anywhere
// within a query.  It can even be deeply nested within a number of
// other SELECT statements.  For this reason, the binder maintains a
// stack of BindScope to preserve the state of processing for outer SELECTs.
//
// ***********************************************************************
class BindScopeList : public LIST(BindScope *)
{
public:
  BindScopeList(CollHeap *h = NULL) : LIST(BindScope *)(h) {}
  void removeScope()		{ BindScope *s; getLast(s); delete s; }
  ~BindScopeList()		{ while (!isEmpty()) removeScope(); }
}; // class BindScopeList


// This class is used at binding time when there are host arrays in the query.
// Briefly, when we find such arrays, we replace them with scalar variables
// of the same type as the elements of the array. These variables get their
// values from a Rowset node introduced during binding. Please see the Rowset
// internal spec for details.
// There is a data member of type HostArraysWA in BindWA, and that object keeps all
// the necessary information to perform the transformation just mentioned.

class HostArraysWA: public NABasicObject
{
public:

  HostArraysWA(BindWA *bindWA, CollHeap * h = 0)
    : numHostArrays_(0),
      hasDerivedRowsets_(FALSE),
      listOfHostArrays_(NULL),
      lastHostArray_(NULL),
      newItemsList_(NULL),
      lastItemList_(NULL),
      newNames_(NULL),
      lastName_(NULL),
      done_(FALSE),
      bindWA_(bindWA),
      inputSizeExpr_(NULL),
      outputSizeExpr_(NULL),
      indexExpr_(NULL),
      inputArrayMaxSize_(0),
      hasDynamicRowsets_(FALSE),
      hasHostArraysInTuple_(FALSE),
      hasSelectIntoRowsets_(FALSE),
      hasHostArraysInSetClause_(FALSE),
      hasHostArraysInWhereClause_(FALSE),
      tolerateNonFatalError_(FALSE),
      hasInputRowsetsInSelect_(HostArraysWA::UNKNOWN_),
      rowsetRowCountArraySize_(0),
      newTable_("",h),
      root_(NULL),
      rwrsMaxSize_(NULL),
      rwrsMaxInputRowlen_(NULL),
      rwrsBuffer_(NULL),
      partnNum_(NULL),
      rowwiseRowset_(FALSE),
      packedFormat_(FALSE),
      compressed_(FALSE),
      dcompressInMaster_(FALSE),
      compressInMaster_(FALSE),
      partnNumInBuffer_(FALSE)
  {}

  HostArraysWA(const HostArraysWA &other, CollHeap * h = 0)
    : numHostArrays_(other.numHostArrays_),
      hasDerivedRowsets_(other.hasDerivedRowsets_),
      listOfHostArrays_(other.listOfHostArrays_),
      lastHostArray_(other.lastHostArray_),
      newItemsList_(other.newItemsList_),
      lastItemList_(other.lastItemList_),
      newNames_(other.newNames_),
      lastName_(other.lastName_),
      done_(other.done_),
      bindWA_(other.bindWA_),
      inputSizeExpr_(other.inputSizeExpr_),
      outputSizeExpr_(other.outputSizeExpr_),
      indexExpr_(other.indexExpr_),
      inputArrayMaxSize_(other.inputArrayMaxSize_),
      hasHostArraysInTuple_(other.hasHostArraysInTuple_),
      hasDynamicRowsets_(other.hasDynamicRowsets_),
      hasSelectIntoRowsets_(other.hasSelectIntoRowsets_),
      hasHostArraysInSetClause_(other.hasHostArraysInSetClause_),
      hasHostArraysInWhereClause_(other.hasHostArraysInWhereClause_),
      tolerateNonFatalError_(other.tolerateNonFatalError_),
      hasInputRowsetsInSelect_(other.hasInputRowsetsInSelect_),
      rowsetRowCountArraySize_(other.rowsetRowCountArraySize_),
      newTable_(other.newTable_,h),
      root_(other.root_),
      rwrsMaxSize_(other.rwrsMaxSize_),
      rwrsMaxInputRowlen_(other.rwrsMaxInputRowlen_),
      rwrsBuffer_(other.rwrsBuffer_),
      partnNum_(other.partnNum_),
      rowwiseRowset_(other.rowwiseRowset_),
      packedFormat_(other.packedFormat_),
      compressed_(other.compressed_),
      dcompressInMaster_(other.dcompressInMaster_),
      compressInMaster_(other.compressInMaster_),
      partnNumInBuffer_(other.partnNumInBuffer_)
  {}


  HostArraysWA()
    { HostArraysWA(NULL); }

  // Scans list of expressions in a VALUES node and replaces all host vars found with
  // the names used in the RENAME node
  void collectArrays(ItemExpr *parent);

  // Scans list of expressions in a VALUES node and replaces all host vars found with
  // the names used in the RENAME node. Replaces node with a ROOT, RENAME and ROWSET nodes.
  RelExpr *modifyTupleNode(RelExpr *node);

  // If there are host arrays in query, this function modifies the parse tree
  // such that the arrays are replaced with scalar variables and introduces a
  // Rowset node that will provide the values of the array to such variables
  RelExpr *modifyTree(RelExpr *queryExpr, RelExpr::AtomicityType atomicity);

  // Indicates if a Rowset node has been visited
  Int32 &done() { return done_; }

  // Stores all host arrays found in the parse tree
  void collectHostVarsInRelExprTree(RelExpr *root, RelExpr::AtomicityType atomicity);

  // Generates new variable names that will replace the arrays
  void createNewNames();

  ItemExpr *& inputSize()  {return inputSizeExpr_;}
  ItemExpr *& outputSize() {return outputSizeExpr_;}
  ItemExpr *& indexExpr()  {return indexExpr_;}

  ItemExpr *& rwrsMaxSize()        { return rwrsMaxSize_;}
  ItemExpr *& rwrsMaxInputRowlen() { return rwrsMaxInputRowlen_;}
  ItemExpr *& rwrsBuffer()         { return rwrsBuffer_;}
  ItemExpr *& partnNum()           { return partnNum_;}
  NABoolean getRowwiseRowset()       { return rowwiseRowset_; }
  void setRowwiseRowset(NABoolean v) { rowwiseRowset_ = v; }

  void setBufferAttributes(NABoolean packedFormat, 
			   NABoolean compressed,
			   NABoolean dcompressInMaster, 
			   NABoolean compressInMaster,
			   NABoolean partnNumInBuffer)
  {
    packedFormat_ = packedFormat;
    compressed_ = compressed;
    dcompressInMaster_ = dcompressInMaster;
    compressInMaster_ = compressInMaster;
  }
  void getBufferAttributes(NABoolean &packedFormat,
			   NABoolean &compressed,
			   NABoolean &dcompressInMaster, 
			   NABoolean &compressInMaster,
			   NABoolean &partnNumInBuffer)
    {
      packedFormat = packedFormat_;
      compressed = compressed_;
      dcompressInMaster = dcompressInMaster_;
      compressInMaster = compressInMaster_;
      partnNumInBuffer = partnNumInBuffer_;
    }

  // Functions for the current root (it changes dynamically if we have compound statements)
  RelRoot * getRoot() { return root_; }
  void      setRoot(RelRoot *root) { root_ = root; }

  // We have found in the parse tree the variable that appears in the
  // ROWSET FOR KEY BY <var> command (i.e we found <var>). We store it and
  // replace it with a name that will be used in the rename node.
  void processKeyVar(ItemExpr *parent, Int32 childNumber);

  // We have found an array host variable in the parse tree. We store it and replace it
  // with a name that will be used in the rename node
  void processArrayHostVar(ItemExpr *parent, Int32 childNumber);

  // Finds inputVar in list
  NABoolean findHostVar(ItemExpr **inputVar, ItemExpr *list);

  NABoolean hasHostArrays() { return (listOfHostArrays_ != NULL); }

  void setHasDerivedRowsets (const NABoolean flag = TRUE) {hasDerivedRowsets_ = flag;}

  NABoolean getHasDerivedRowsets () { return hasDerivedRowsets_;}

  ULng32 getInputArrayMaxSize() const
  {
    return inputArrayMaxSize_;
  }

  void setInputArrayMaxSize(const ULng32 size) 
  {
    inputArrayMaxSize_ = size;
  }

  NABoolean hasHostArraysInTuple() const
  {
    return hasHostArraysInTuple_;
  }

  void setHasHostArraysInTuple(const NABoolean flag) 
  {
    hasHostArraysInTuple_ = flag;
  }

  void setHasDynamicRowsets (const NABoolean flag = TRUE) {hasDynamicRowsets_ = flag;}

  NABoolean hasDynamicRowsets () { return hasDynamicRowsets_;}

  void setHasSelectIntoRowsets (const NABoolean flag = TRUE) {hasSelectIntoRowsets_ = flag;}

  NABoolean getHasSelectIntoRowsets () { return hasSelectIntoRowsets_;}

  void setHasHostArraysInSetClause (const NABoolean flag = TRUE) {hasHostArraysInSetClause_ = flag;}

  NABoolean hasHostArraysInSetClause () { return hasHostArraysInSetClause_;}

  void setHasHostArraysInWhereClause (const NABoolean flag = TRUE) {hasHostArraysInWhereClause_ = flag;}

  NABoolean hasHostArraysInWhereClause () { return hasHostArraysInWhereClause_;}

  void setTolerateNonFatalError (const NABoolean flag = TRUE) {tolerateNonFatalError_ = flag;}

  NABoolean getTolerateNonFatalError () { return tolerateNonFatalError_;}

  enum SelectStates {NO_ = 0, YES_ = 1, UNKNOWN_ = 2};

  void setHasInputRowsetsInSelectPredicate (const SelectStates flag) {hasInputRowsetsInSelect_ = flag;}

  SelectStates hasInputRowsetsInSelectPredicate () { return hasInputRowsetsInSelect_;}

  void setRowsetRowCountArraySize (const Lng32 size) {rowsetRowCountArraySize_ = size;}

  Lng32 getRowsetRowCountArraySize () { return rowsetRowCountArraySize_;}
private:

  BindWA *bindWA_;

  // Total number of instances of direct host arrays found. i.e. If the same 
  // host array is used twice in a statement (within the same scope) then 
  // numHostArrays = 2.
  Lng32 numHostArrays_;

  // flag to indicate the presence of derived rowsets.  numHostArrays_ is only
  // used to indicate the presence of direct rowsets. 
  NABoolean hasDerivedRowsets_;

  // Points to list of host arrays found in query
  ItemExpr *listOfHostArrays_;

  // Points to end of list of host arrays
  ItemExpr *lastHostArray_;

  // Points to list of expressions from a VALUES node after the host arrays
  // in it have been replaced with a new names (which are listed in the RENAME node).
  ItemExpr *newItemsList_;

  // Points to end of list pointed by newItemsList_
  ItemExpr *lastItemList_;

  // Points to list of names used in the RENAME node
  ItemExpr *newNames_;

  // Points to end of list pointed by newNames_
  ItemExpr *lastName_;

  // Indicates if we have visited a RowSet node or not
  Int32 done_;

  // name of table that is created by Rename node
  NAString newTable_;

  // Indicates size of input arrays
  ItemExpr *inputSizeExpr_;

  // Indicates size of output arrays
  ItemExpr *outputSizeExpr_;

  // Parameter to Rowset class
  ItemExpr *indexExpr_;

  // Pointer to current root node
  RelRoot * root_;

  ItemExpr *rwrsMaxSize_;       // Input rowwise rowset max size.

  ItemExpr *rwrsMaxInputRowlen_;// max length of each input row in the
                                // input rowwise rowset buffer.
  ItemExpr *rwrsBuffer_;        // Contains the address of rowwise-rowset
                                // buffer passed in at runtime to cli.
  ItemExpr *partnNum_;          // partition number where this buffer need
                                // to be shipped to.
  NABoolean rowwiseRowset_;     // layout of input values is rowwise.

  // rows are packed sql/mp style in the buffer. All columns are packed
  // next to each other with no fillers or varchar/null optimizations.
  NABoolean packedFormat_;

  // input buffer is compressed by caller.
  NABoolean compressed_;

  // Next member valid only if compressed_ is TRUE;
  // if TRUE, dcompress the buffer in master before doing other processing.
  // if FALSE, ship the compressed buffer to eid and decompress it there.
  //    This option is only valid if partnNum is specified.
  NABoolean dcompressInMaster_;

  // Next field is valid if buffer is not compressed_.
  // Next field is valid only if partnNum is specified.
  // If TRUE, master exe need to compress the buffer before shipping to eid.
  NABoolean compressInMaster_;

  // TRUE, if partition number is sent in at runtime as part of input buffer,
  // instead of dynamic parameter partnNum_
  NABoolean partnNumInBuffer_;

  // Maximum array size; used for ODBC queries. It is specified by an ODBC 
  // process and passed to mxcmp
  ULng32 inputArrayMaxSize_;

  // Flag to indicate if this HostArraysWA contains any arrays used for insert
  NABoolean hasHostArraysInTuple_;

  // flag to indicate the presence of dynamic rowsets. inputArrayMaxSize_ > 0 
  // indicates the presence of dynamic rowsets from ODBC. hasDynamicRowsets
  // indicates the presence of dynamic rowsets from either ODBC or embedded dynamic SQL
  NABoolean hasDynamicRowsets_;


  //flag to indicate the presence of Select Into Rowset host variables.
  //note that numHostArrays may be zero and listOfHostArrays empty even though
  //this flag is set. Rowset hostvars used in select into do not make it into listofHostArrays.
  NABoolean hasSelectIntoRowsets_;

  //flag to indicate the presence of Rowset host variables in Set clause.
  NABoolean hasHostArraysInSetClause_;

  //flag to indicate the presence of direct Rowset host variables in Where clause.
  NABoolean hasHostArraysInWhereClause_;

  //flag to indicate that this statement (or part of statement when in CS)
  //is a  NOT ATOMIC rowset insert
  NABoolean tolerateNonFatalError_;

  //state to indicate that this statement (or part of statement when in CS)
  //is a select statment that has rowsets for input. This state is set during multiple 
  // passes through collectHostVarsInRelExprTree. Do no read the state before
  // we enter the binder, at which point the UNKNOWN_ state is same as the NO_ state
  SelectStates hasInputRowsetsInSelect_;

  // used to send compile-time rowset size from Rowset/Unpack node to Onlj
  // for rowset updates and deletes. Has non-zero value only if rowset_row_count
  // feature is enabled. Onlj will send this value through the TDB to executor
  // where an array of this length will be allocated at run-time.
  Lng32 rowsetRowCountArraySize_;

  // Scans the node parent->childNumber) searching for host vars. Puts them
  // in list pointed by listOfHostArrays_
  void collectHostVarsInPred(ItemExpr *parent, Int32 childNumber);
};

// ***********************************************************************
// BindWA
//
// The work area for the binder.
//
// ***********************************************************************
class BindWA : public NABasicObject
{
 enum Flags {
   // --------------------------------------------------------------------
   // Flag to indicate we are accessing an object which is defined in an
   // external (native) hive or hbase.
   // --------------------------------------------------------------------
    ALLOW_EXT_TABLES          = 0x00000001,

    // external table being dropped.
    EXT_TABLE_DROP            = 0x00000002,

    // return underlying hive table defn, if the table has an associated
    // external table
    RETURN_HIVE_TABLE_DEFN    = 0x00000004
 };

public:

  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------
  BindWA(SchemaDB *schemaDB,
  	 CmpContext *currentCmpContext = NULL,
	 NABoolean inDDL = FALSE,
	 NABoolean allowExternalTables = FALSE);

  // copy ctor
  BindWA (const BindWA & orig, CollHeap * h=0) ; // not written

  // --------------------------------------------------------------------
  // Destructor function
  // --------------------------------------------------------------------
  ~BindWA();

  void initNewScope();

  NABoolean  inDDL() const		   { return inDDL_; }
  const NATable *&inViewWithCheckOption()  { return inViewWithCheckOption_; }
  ValueIdSet &predsOfViewWithCheckOption() { return predsOfViewWithCheckOption_; }
  NABoolean inReadOnlyQuery() { return inReadOnlyQuery_;}
  void setReadOnlyQuery() { inReadOnlyQuery_=TRUE;}

  // --------------------------------------------------------------------
  // The following methods require the implementation of an iterator
  // over LIST.
  // --------------------------------------------------------------------
  BindScope *getCurrentScope() const;
  BindScope *getPreviousScope(BindScope *currentScope) const;
  void removeCurrentScope(NABoolean keepLocalRefs = FALSE);

  BindScope *findNextScopeWithTriggerInfo(BindScope *currentScope = NULL);
  
  //--------------------------------------------------------------------------
  //++MV --
  // The isBindingMvRefresh flag is set during binding of INTERNAL REFRESH.
  void setBindingMvRefresh() { isBindingMvRefresh_ = TRUE; }
  NABoolean isBindingMvRefresh() const { return isBindingMvRefresh_; }
  // Use a separate flag for propagating the OP and SYSKEY cols.
  void setPropagateOpAndSyskeyColumns() { isPropagateOpAndSyskeyColumns_ = TRUE; }
  NABoolean isPropagateOpAndSyskeyColumns() const { return isPropagateOpAndSyskeyColumns_; }
  // This flag is used during CREATE MV, for inlining MVs just like views.
  void setExpandMvTree() { isExpandMvTree_ = TRUE; }
  NABoolean isExpandMvTree() const { return isExpandMvTree_; }
  // The isBindingOnStatementMV flag is used for inlining ON STATMENT MVs.
  void setBindingOnStatementMv() { isBindingOnStatementMv_ = TRUE; }
  NABoolean isBindingOnStatementMv() const { return isBindingOnStatementMv_; }
  // See MvBindContext above for more details.
  void markScopeWithMvBindContext(MvBindContext *mvContext);
  const MvBindContext * 
    getClosestMvBindContext(BindScope *currentScope = NULL) const;

  // add uninitialized mv names to list
  void addUninitializedMv( const char * physicalName, const char *ansiName );

  // flag is set if an Insert/Update/Delete operation is bound
  void setBindingIUD() { isBindingIUD_ = TRUE; }
  NABoolean isBindingIUD() { return isBindingIUD_; }

  // flag is set if statement is a FastExtract
  void setIsFastExtract() { isFastExtract_ = TRUE; }
  NABoolean isFastExtract() { return isFastExtract_; }

  void setIsTrafLoadPrep(NABoolean x) { isTrafLoadPrep_ = x; }
  NABoolean isTrafLoadPrep() { return isTrafLoadPrep_; }

  // The following method returns the BindScope* with which we determine
  // correctly whether we're in a subquery (for the purposes to
  // determining a certain syntax error).  This involves checking the
  // previousScope, and if that's a GroupByAgg (or possibly some other
  // node, in some, as yet unfound bug ...?), then we look ahead one
  // scope further.
  //
  // See the comments in bindRowValues(), BindRelExpr.cpp, for a more
  // complete discussion of how/why this method is used.
  BindScope *getSubqueryScope(BindScope *currentScope) const;

  static ULng32 qualNameHashFunc(const QualifiedName& qualName);

  // --------------------------------------------------------------------
  // Methods for the XCNM of constants, params, and host variables.
  // --------------------------------------------------------------------
  void addInputValue(const NAString &name, ValueId valId)
  {
    inputVars_.insert(new(wHeap()) ColumnNameMap(name, valId, wHeap()));
  }

  ColumnNameMap *findInputValue(const NAString &name) const
  {
    ColRefName lookupName(name);
    return inputVars_.get(&lookupName);
  }

  // --------------------------------------------------------------------
  // The following method searches for the given name in the BindScopes.
  // It starts from the current BindScope.  If the given name is not
  // found in the ColumnNameMap associated with a BindScope, the
  // search progresses to the previous BindScope.  The search terminates
  // either when a ColumnNameMap corresponding to the given name is
  // found or the outermost BindScope has been searched unsucessfully.
  //
  // If the name is found, the method returns a pointer to the xcnmEntry
  // and a pointer to the BindScope in which it was found.  Otherwise, it
  // returns a NULL and a pointer to the outermost BindScope.
  //
  // The second findColumn() locates the xcnmEntry to a ColumnDesc
  // known to exist in the current scope (no backward searching thru scopes!).
  //
  // findCorrName() operates like the first findColumn(), mutatis mutandis.
  //
  // --------------------------------------------------------------------
  ColumnNameMap  *findColumn(const ColRefName&, BindScope*&);

  ColumnNameMap  *findColumn(const ColumnDesc&);

  ColumnDescList *findCorrName(const CorrName&, BindScope*&);

  // --------------------------------------------------------------------
  // This method, called by BindRelExpr Join code and BindItemExpr ColReference,
  // is implemented and documented in BindItemExpr.C.
  // --------------------------------------------------------------------
  void markAsReferencedColumn(const ValueId &vid, NABoolean groupByRefForSingleIntHist = FALSE);
  void markAsReferencedColumn(const ColumnDesc *cd, NABoolean groupByRefForSingleIntHist = FALSE);
  void markAsReferencedColumn(const ItemExpr *ie, NABoolean groupByRefForSingleIntHist = FALSE)
    				  { markAsReferencedColumn(ie->getValueId(), groupByRefForSingleIntHist); }
  void setColumnRefsInStoi(const char* fileName, Lng32 colPosition);


  // --------------------------------------------------------------------
  // A rather miscellaneous utility, in the class so as to be more
  // acccessible to multiple calling files.
  // --------------------------------------------------------------------
  RelExpr *bindView(const CorrName &viewName,
		    const NATable *naTable,
		    const StmtLevelAccessOptions &accessOptions,
		    ItemExpr *predicate,
                    GroupAttributes *groupAttrs,
		    NABoolean catmanCollectUsages = FALSE);

  // --------------------------------------------------------------------
  // - Lookup (read from catalog and construct if need be) the NATable
  //   for the table named in corrName, and
  // - Create a TableDesc from the NATable for that corrName.
  // The same NATable can underlie many TableDescs
  // (i.e. the same table can be referred to many times, via many corr names).
  // --------------------------------------------------------------------
  NATable *getNATable(CorrName &corrName,
		      NABoolean catmanCollectTableUsages = TRUE,
		      TrafDesc *inTableDesc = NULL);

  NATable *getNATableInternal(CorrName &corrName,
                              NABoolean catmanCollectTableUsages = TRUE,
                              TrafDesc *inTableDesc = NULL,
                              NABoolean extTableDrop = FALSE);

  TableDesc *createTableDesc(const NATable *naTable,
                             CorrName &corrName,
                             NABoolean catmanCollectUsages = FALSE,
                             Hint *hint=NULL);

  // --------------------------------------------------------------------
  // - Lookup (read from catalog and construct if need be) the NARoutine
  //   for the table named in corrName, and
  // - Create a RoutineDesc from the NARoutine for that corrName.
  // The same NARoutine can underlie many RoutineDescs
  // (i.e. the same table can be referred to many times, via many corr names).
  // --------------------------------------------------------------------
//  NARoutine *getNARoutine(QualifiedName &routineName,
//		          TrafDesc *inRoutineDesc = NULL);


  // --------------------------------------------------------------------
  // BindWA::getTablesInScope() [and BindScope::getTablesInScope()]
  //
  // Return a list of TableNameMaps of all tables that are in scope.
  // Caller can get the exposed names of the tables and display them,
  // or tell this function to do that passing the NAString parameter.
  //
  // Same search strategy as the first findColumn().
  // --------------------------------------------------------------------
  void getTablesInScope(LIST(TableNameMap*) &xtnmList,
                        NAString *formattedList = NULL) const;

  // --------------------------------------------------------------------
  // Get the SchemaDB that is valid for this query
  // --------------------------------------------------------------------
  SchemaDB *getSchemaDB() const		{ return schemaDB_; }

  // --------------------------------------------------------------------
  // Get the default schema (includes default catalog) for this query
  // --------------------------------------------------------------------
  const SchemaName &getDefaultSchema() const;

  // --------------------------------------------------------------------
  // Set the default schema (includes default catalog) for this query.
  //
  // The only callers should be Compiler Main (or its delegate, SchemaDB funx)
  // and the CREATE SCHEMA statement in CatMan.
  //
  // Caller needs to set the correct default according to whether query is
  // - within-a-create-schema-stmt vs
  //   dynamic (execute-immediate or inside a prepare) vs
  //   static (all other statements, including the auto-recompile of
  //	       an execute of a previously prepared stmt!)
  // - compilation vs auto-recompilation
  //
  // Additional requirements and comments found in .C file.
  // --------------------------------------------------------------------
  void setDefaultSchema(const SchemaName &defaultSchema);

  // ---------------------------------------------------------------------
  // Get pointer pointing a list of locations of names in a statement
  // input string.  The list helps with the computing of view text,
  // check constraint search condition text, etc.
  // ---------------------------------------------------------------------
  ParNameLocList *getNameLocListPtr() const	{ return pNameLocList_; }
  void setNameLocListPtr(ParNameLocList *p)	{ pNameLocList_ = p; }

  // ---------------------------------------------------------------------
  // Get pointer pointing the parse node containing the usages
  // information (to help with computing view or check constraint definition).
  // ---------------------------------------------------------------------
  ExprNode *getUsageParseNodePtr() const	{ return pUsageParseNode_; }
  void setUsageParseNodePtr(ExprNode *p)	{ pUsageParseNode_ = p; }

  StmtDDLCreateView *getCreateViewParseNode() const;

  NABoolean inViewDefinition() const;
  NABoolean inMVDefinition() const;
  NABoolean inCheckConstraintDefinition() const;
 
  //----------------------------------------------------------------------
  // Get the NARoutine associated with this routine name
  //----------------------------------------------------------------------
  NARoutine *getNARoutine ( const QualifiedName &name );

  //----------------------------------------------------------------------
  // Insert the NARoutine in the NARoutineDB
  //----------------------------------------------------------------------
//  void setNARoutine ( NARoutine *routine );

  // ---------------------------------------------------------------------
  // Fabrication of unique names
  // (implementation dependent or implementation defined names).
  // Each call generates a new name guaranteed unique over this query.
  // ---------------------------------------------------------------------
  UInt32 getUniqueNum() 			{ return ++uniqueNum_; }
  NAString fabricateUniqueName() { return UnsignedToNAString(getUniqueNum()); }

  //++ Trigger -
  enum uniqueIudNumOffset 
    { uniqueIudNumForInsert = 0, 
      uniqueIudNumForDelete = 1 };
  // set uniqueIudNum_ to the next unused number
  inline void setUniqueIudNum() 	    { uniqueIudNum_ = maxIudNum_+=2; }
  inline void setUniqueIudNum(Lng32 newNum)  { uniqueIudNum_ = newNum; }
  inline void resetUniqueIudNum(Lng32 iudNum)	{ uniqueIudNum_ = iudNum; } 
  inline Lng32 getUniqueIudNum(uniqueIudNumOffset offset = uniqueIudNumForInsert) 
    { return uniqueIudNum_+offset; }

  inline LIST(ComTimestamp)* getTriggersList() const { return triggersList_; } 
  CollIndex addTrigger(const ComTimestamp& tr);
  //--Triggers,

  inline 
      UninitializedMvNameList * getUninitializedMvList() const
      { return uninitializedMvList_; }

  // --------------------------------------------------------------------
  // Temporary measure for errors.
  // --------------------------------------------------------------------
  NABoolean errStatus() const		{ return errFlag_; }
  void setErrStatus() 			{ errFlag_ = TRUE; }
  void resetErrStatus() 		{ errFlag_ = FALSE; }

  // ---------------------------------------------------------------------
  // Display/print, for debugging.
  // ---------------------------------------------------------------------
  void display() const;

  void print(FILE *ofd = stdout,
	     const char *indent = DEFAULT_INDENT,
             const char *title = "BindWA") const;

  // ---------------------------------------------------------------------
  // Get current CmpContext and working heap and RETDescList
  // ---------------------------------------------------------------------
  CmpContext *currentCmpContext() const { return currentCmpContext_; }

  // Get the statementHeap from currentCmpContext_ as current working
  // heap.  The memory allocated from this heap will be wiped out
  // at the end of each statement.
  CollHeap *wHeap() const
  {
    return currentCmpContext_ ? currentCmpContext_->statementHeap() : NULL;
  }

  RETDescList &getRETDescList()	// not const, so result can be an lvalue
  {
    return RETDescList_;
  }

  HostArraysWA *getHostArraysArea()	   { return hostArraysArea_; }
  void setHostArraysArea(HostArraysWA *wa) { hostArraysArea_ = wa; }
  AssignmentStArea *&getAssignmentStArea() { return assignmentStArea_; }
  AssignmentStHostVars *getAssignmentStHostVars()
  { return assignmentStArea_ ?
  	   assignmentStArea_->getAssignmentStHostVars() : NULL;
  }

  short &viewCount()			   { return viewCount_; }

  NABoolean allowExternalTables() const 
  { return (flags_ & ALLOW_EXT_TABLES) != 0; }
  void setAllowExternalTables(NABoolean v) 
  { v ? flags_ |= ALLOW_EXT_TABLES : flags_ &= ~ALLOW_EXT_TABLES; }

  NABoolean externalTableDrop() const 
  { return (flags_ & EXT_TABLE_DROP) != 0; }
  void setExternalTableDrop(NABoolean v) 
  { v ? flags_ |= EXT_TABLE_DROP : flags_ &= ~EXT_TABLE_DROP; }

  NABoolean returnHiveTableDefn() const 
  { return (flags_ & RETURN_HIVE_TABLE_DEFN) != 0; }
  void setReturnHiveTableDefn(NABoolean v) 
  { v ? flags_ |= RETURN_HIVE_TABLE_DEFN : flags_ &= ~RETURN_HIVE_TABLE_DEFN; }

  LIST(OptSqlTableOpenInfo *) &getStoiList()  { return stoiList_; }
  LIST(OptUdrOpenInfo *) &getUdrStoiList()  { return udrStoiList_; }
  LIST(ExeUtilHbaseCoProcAggr *) &getCoProcAggrList() { return coProcAggrList_; }
  LIST(SequenceValue *) &getSeqValList() { return seqValList_; }

  BindWA& insertCoProcAggr(ExeUtilHbaseCoProcAggr *coProcAggr)
  {
    coProcAggrList_.insert(coProcAggr);
    return *this;
  }

  BindWA& insertSeqVal(SequenceValue *seqVal)
  {
    seqValList_.insert(seqVal);
    return *this;
  }

  LIST(OptUDFInfo *) &getUDFList()  { return udfList_; }

  ValueIdList &inputFunction()		   { return inputFunction_; }
  TableViewUsageList &tableViewUsageList() { return tableViewUsageList_; }
  RelRoot * getTopRoot() { return topRoot_; }
  void setTopRoot(RelRoot *root) { topRoot_ = root; }

  HbaseColUsageInfo * hbaseColUsageInfo()
  {
    return hcui_;
  }
  
  // QSTUFF
  NABoolean renameToScanTable() const		{ return renameToScanTable_;}
  void setRenameToScanTable(NABoolean t)	{ renameToScanTable_ = t; }

  NABoolean inGenericUpdate() const		{ return inGenericUpdate_;}
  NABoolean setInGenericUpdate(NABoolean t)
  {
    NABoolean r = inGenericUpdate_;
    inGenericUpdate_ = t;
    return r;
  }

  NABoolean inViewExpansion() const		{ return inViewExpansion_;}
  NABoolean setInViewExpansion(NABoolean t)
  {
    NABoolean r = inViewExpansion_;
    inViewExpansion_ = t;
    return r;
  }

  CommonSubExprRef *inCSE() const       { return currCSE_; }
  void setInCSE(CommonSubExprRef *cte)  { currCSE_ = cte; }
  
  NABoolean inCTAS() const		{ return inCTAS_; }
  void setInCTAS(NABoolean t)
  {
    inCTAS_ = t;
  }

  ValueIdMap &getUpdateToScanValueIds()		{ return updateToScanValueIds_;}
  // QSTUFF

  void appendViewName(const char* v) { viewsUsed_ += v; viewsUsed_ += "+"; }
  NAString getViewsUsed() const { return viewsUsed_; }

  // QSTUFF

  // Raj P -  7/2000
  // Used for semantic checks in CALL
  NABoolean isInTrigger () const { return inTrigger_; }
  void setInTrigger () { inTrigger_ = TRUE; }
  void clearInTrigger () { inTrigger_ = FALSE;}

  // Used for to check presence of dynamic rowsets in subqueries from RelRoot.
  void setHasDynamicRowsetsInQuery (const NABoolean flag = TRUE) {hasDynamicRowsetsInQuery_ = flag;}
  NABoolean hasDynamicRowsetsInQuery () { return hasDynamicRowsetsInQuery_;}

  // CLI support, make IN and OUT params accessible to RelRoot
  // Used by CALL statement
  inline ItemExprList &getSpInParams()  { return spInParams_;}
  inline ItemExprList &getSpOutParams() { return spOutParams_;}	
  inline CollIndex getCurrOrdinalPosition() const { return currOrdinalPosition_;}
  inline ComColumnDirection getCurrParamMode() const { return currParamMode_;}
  inline NABoolean bindingCall() const {return bindingCall_;}
  inline short getMaxResultSets() const { return maxResultSets_; }
  inline void setCurrOrdinalPosition(CollIndex v){ currOrdinalPosition_ = v;}
  inline void setCurrParamMode(ComColumnDirection v){ currParamMode_ = v;}
  inline void setBindingCall(NABoolean val) { bindingCall_ = val;}
  inline void setMaxResultSets(short maxRS) { maxResultSets_ = maxRS; }
  inline void addHVorDPToSPDups (ItemExpr *h) { spHVDPs_.insert (h);}
  inline LIST(ItemExpr *) &getSpHVDPs() { return spHVDPs_; }
  ItemExpr *getHVorDPFromSPDups (ItemExpr *h);  
  NABoolean checkHVorDPinSPDups (ItemExpr *h);
  NABoolean checkMultiOutSPParams (ItemExpr *h);
  inline void clearHVorDPinSPDups () {spHVDPs_.clear ();}
  inline const QualifiedName &getCurrSPName () const {return *currSPName_;}
  inline void setCurrSPName (QualifiedName *name){currSPName_ = name;}
  inline NABoolean getDupWarning () const {return dupWarning_;}
  inline void setDupWarning (NABoolean val) {dupWarning_ = val;}

  inline Int32 getRoutineInvocationNum() { return routineInvocationNum_++ ; }
  void setInliningInfoFlagsToSetRecursivly(Int32 flags) { inliningInfoFlagsToSetRecursivly_ = flags; }
  Int32  getInliningInfoFlagsToSetRecursivly() const { return inliningInfoFlagsToSetRecursivly_; }

  StmtLevelAccessOptions *findUserSpecifiedAccessOption();

  inline NABoolean isEmbeddedIUDStatement () const {return embeddedIUDStatement_;}
  inline void setEmbeddedIUDStatement (NABoolean val) {embeddedIUDStatement_ = val;}

  inline NABoolean isInsertSelectStatement () const {return insertSelectStatement_;}
  inline void setInsertSelectStatement (NABoolean val) {insertSelectStatement_ = val;}

  inline NABoolean isMergeStatement () const {return mergeStatement_;}
  inline void setMergeStatement (NABoolean val) {mergeStatement_ = val;}

  void appendCorrNameToken(char c) { corrNameTokens_ += c; };
  NAString getCorrNameTokens() const { return corrNameTokens_; }

  void setHoldableType(SQLATTRHOLDABLE_INTERNAL_TYPE h)
  { holdableType_ = h; }

  SQLATTRHOLDABLE_INTERNAL_TYPE getHoldableType() { return holdableType_; } 

  NABoolean isBindTrueRoot() const { return isBindTrueRoot_; }
  void setBindTrueRoot(NABoolean x) { isBindTrueRoot_ = x; }

  inline NAString getOsFromSchema() const { return osFromSchema_; }
  inline NAString getOsToSchema() const { return osToSchema_; }
  inline NABoolean overrideSchemaEnabled() const 
    {  return overrideSchemaEnabled_;  }
  inline void setToOverrideSchema(const NABoolean val) 
    {  toOverrideSchema_ = (overrideSchemaEnabled_? val : FALSE);  }
  inline NABoolean getToOverrideSchema() const
    {  return (overrideSchemaEnabled_? toOverrideSchema_ : FALSE);  }
  void doOverrideSchema(CorrName& corrName);
  void initializeOverrideSchema();
  NABoolean violateAccessDefaultSchemaOnly(const QualifiedName&);
  NABoolean raiseAccessDefaultSchemaOnlyError();

  NABoolean noNeedToLimitSchemaAccess() const {
    return noNeedToLimitSchemaAccess_;
  }
  void setNoNeedToLimitSchemaAccess(NABoolean val) {
    noNeedToLimitSchemaAccess_ = val;
  }

  void setFailedForPrivileges(NABoolean val) {
    failedForPrivileges_ = val;
  }
  NABoolean failedForPrivileges() const {
    return failedForPrivileges_ ;
  }

  void setShouldLogAccessViolations(NABoolean val) {
    shouldLogAccessViolations_ = val;
  }
  NABoolean shouldLogAccessViolations() const {
    return shouldLogAccessViolations_ ;
  }

  NABoolean queryCanUseSeaMonster();
  NABoolean &volatileTableFound() { return volatileTableFound_; }
  
  BindScope  *&outerAggScope()   { return outerAggScope_; }

  inline NABoolean hasCallStmts()     { return hasCallStmts_; }
  inline void setHasCallStmts(NABoolean v)      { hasCallStmts_ = v; }

  inline void setISPExecLocation(const NAString & locationStr) { ISPExecLocation_ = locationStr; }

  const NAString & getISPExecLocation() const { return ISPExecLocation_ ;}

private:

  // --------------------------------------------------------------------
  // source and target schemas for override_schema
  // --------------------------------------------------------------------
  NAString osFromSchema_;
  NAString osToSchema_;
  NABoolean overrideSchemaEnabled_;
  NABoolean toOverrideSchema_;

  // --------------------------------------------------------------------
  // pointer to current CmpContext
  // --------------------------------------------------------------------
  CmpContext *currentCmpContext_;

  // --------------------------------------------------------------------
  // Set on error.  Causes termination of processing.
  // --------------------------------------------------------------------
  NABoolean errFlag_;

  // --------------------------------------------------------------------
  // Used for fabricating unique names
  // (implementation dependent or implementation defined names).
  // --------------------------------------------------------------------
  UInt32 uniqueNum_;

  //++ Trigger -
  // ---------------------------------------------------------------------
  // Unique identifier of a Generic Update inlining backbone.
  // This value belongs to the uniquifier columns (UNIQUEIUD_COLUMN)
  // in the temporary table used for triggers.
  // ---------------------------------------------------------------------
  // current identifier
  Lng32 uniqueIudNum_;
  // next unused identifier - max is not the current because of cascading
  Lng32 maxIudNum_;

  // List of all triggers inlined in this statement. Used for trigger-enable.
  LIST(ComTimestamp) *triggersList_;
  //-- Trigger -

  // MVs --
  // Are we binding an INTERNAL REFRESH command.
  NABoolean isBindingMvRefresh_;
  NABoolean isPropagateOpAndSyskeyColumns_;
  // Should Materialized Views be opened just like regular views?
  NABoolean isExpandMvTree_;

  // MV
  // Are we bining an ON STATEMENT MV (immediate refresh)?
  NABoolean isBindingOnStatementMv_;

  // set to TRUE when binding an insert/update/delete
  NABoolean isBindingIUD_;

  // --------------------------------------------------------------------
  // A stack of BindScopes.
  // Implemented by a LIST maintained using the LIFO policy.
  // --------------------------------------------------------------------
  BindScopeList scopes_;

  // --------------------------------------------------------------------
  // A hash table of the input variables (constants, params, and host
  // variables) contained in this query.
  // --------------------------------------------------------------------
  XCNM inputVars_;

  // --------------------------------------------------------------------
  // List of all RETDescs created in this query compilation, saved so we
  // can delete them and reclaim lots-o'-memory, because RETDescs contain
  // lots of NAStrings, which don't get reclaimed by the CollHeap mechanism.
  //
  // RETDescs append themselves onto this list upon creation.
  // BindWA dtor invokes RETDescList dtor invokes clearAndDestroy & delete;
  // compiler main can do bindWA->getRETDescList().clearAndDestroy() explicitly
  // if desired.
  // --------------------------------------------------------------------
  RETDescList RETDescList_;

  // --------------------------------------------------------------------
  // Open access info (security info) for all tables/views in a statement
  // --------------------------------------------------------------------
  LIST(OptSqlTableOpenInfo *) stoiList_;

  // --------------------------------------------------------------------
  // Open access info (security info) for all UDR's referenced in a statement
  // --------------------------------------------------------------------
  LIST(OptUdrOpenInfo *) udrStoiList_;

  // --------------------------------------------------------------------
  // All coprocessors referenced in a statement
  // --------------------------------------------------------------------
  LIST(ExeUtilHbaseCoProcAggr *) coProcAggrList_;

  // --------------------------------------------------------------------
  // All sequence generators referenced in a statement
  // --------------------------------------------------------------------
  LIST(SequenceValue *) seqValList_;

  // --------------------------------------------------------------------
  // All UDF's referenced in a statement
  // --------------------------------------------------------------------
  LIST(OptUDFInfo *) udfList_;

  // information on column usage when a native hbase is accessed in the query.
  HbaseColUsageInfo * hcui_;

  // --------------------------------------------------------------------
  // Default schema and catalog for this query.
  // Initially the same as SchemaDB's current default, but can be
  // overwritten (by CREATE SCHEMA, e.g.).
  // --------------------------------------------------------------------
  SchemaName defaultSchema_;

  // --------------------------------------------------------------------
  // Main memory database of physical schema information.
  // --------------------------------------------------------------------
  SchemaDB *schemaDB_;

  // ---------------------------------------------------------------------
  // positions of names specified in the input string (to help with
  // computing text; e.g., view text)
  // ---------------------------------------------------------------------
  ParNameLocList *pNameLocList_;

  // ---------------------------------------------------------------------
  // pointer to the parse node containing the usages information.
  // This field is used to help with collecting the usages information
  // for view and check constraint definitions.  If it is not used,
  // it is set to the NULL pointer value.
  // ---------------------------------------------------------------------
  ExprNode *pUsageParseNode_;

  // --------------------------------------------------------------------
  // Used as a "return variable" communicating from markAsRefdCol()
  // up to a caller at some higher stack level and possibly binder scope,
  // e.g. bindCheckConstraint().
  // --------------------------------------------------------------------
  ValueIdSet referencedCols_;

  // --------------------------------------------------------------------
  // Flag whether CatMan is the caller or not --
  // if binding DDL we can avoid executing some bulky code.
  // --------------------------------------------------------------------
  NABoolean inDDL_;

  //---------------------------------------------------------------------
  // does the query tree contain any update nodes?
  //---------------------------------------------------------------------
  NABoolean inReadOnlyQuery_;
  // --------------------------------------------------------------------
  // In the bindView method, this flag implements CASCADED CHECK OPTION
  // on a view, per Ansi 11.19 GR 9-11a.
  // Yes, it cascades down over view scopes till it hits a basetable,
  // hence it is a BindWA flag not a BindScope one.
  // --------------------------------------------------------------------
  const NATable *inViewWithCheckOption_;
  ValueIdSet predsOfViewWithCheckOption_;

  // --------------------------------------------------------------------
  // to let the bindNode() know that it is binding the view tree, which
  // is useful for creating the access information for underlying tables
  // --------------------------------------------------------------------
  short viewCount_;

  ULng32 flags_;

  // points to a class used by RowSets code.
  HostArraysWA *hostArraysArea_;
  // Used to be allocated inline. But, now, is dynamically set to point to
  // the current SQL statement's hostArraysArea so that rowset queries can
  // be used in compound statements.

  // --------------------------------------------------------------------
  // Contains value ids of all 'functions' that are to be evaluated only
  // once at the beginning of the query and then to be used anywhere
  // they appear in the query. They are treated like an input value
  // (param or hostvar) to the root node. Examples are current time
  // function, or user id, etc.
  // --------------------------------------------------------------------
  ValueIdList inputFunction_;

  TableViewUsageList tableViewUsageList_;

  // --------------------------------------------------------------------
  // Points to a list of host variables used by assignment statements in
  // compound statements. See definition of class.
  // --------------------------------------------------------------------
  AssignmentStArea *assignmentStArea_;

  // Pointer to root at the top of the tree
  RelRoot *topRoot_;

  // QSTUFF
  NABoolean  inGenericUpdate_;
  NABoolean  renameToScanTable_;
  NABoolean  inViewExpansion_;
  ValueIdMap updateToScanValueIds_;
  // QSTUFF

  // set if we are currently under a CommonSubExprRef node
  CommonSubExprRef *currCSE_;

  NABoolean  inCTAS_;

  // names of referenced views. used by query caching to guard against false
  // cache hits from view queries that expand to & get identical query plans.
  NAString viewsUsed_;

  // --------------------------------------------------------------------
  // Flag to indicate we are compiling a trigger
  // Currently only used in UDR compilation
  // --------------------------------------------------------------------
  NABoolean inTrigger_;

  // --------------------------------------------------------------------
  // For CALL statements:
  // - List of input parameters
  // - List of output parameters
  // - Max number of result sets that can be returned
  // - Boolean set to TRUE when we are in CallSP::bindNode
  // - ANSI name of the procedure when we are in CallSP::bindNode
  // --------------------------------------------------------------------
  ItemExprList spInParams_;
  ItemExprList spOutParams_;
  short maxResultSets_;
  NABoolean bindingCall_;
  QualifiedName *currSPName_;

  // The following are used to set Host Var and Dynamic Parameter
  // attributes for CALL statements
  CollIndex currOrdinalPosition_;
  ComColumnDirection currParamMode_;
  LIST (ItemExpr *) spHVDPs_;
  NABoolean dupWarning_;

  // Set these flags recursivly in every RelExpr node being bound.
  Int32  inliningInfoFlagsToSetRecursivly_;

  // denotes that this is a SELECT ... INSERT/DELETE/UPDATE ...
  // type statement, but NOT the Pub/Sub embedded update/delete statement.
  // Currently such MTS (multi-transaction support) features support 
  // an embedded insert or delete
  NABoolean embeddedIUDStatement_;

  // denotes that this is a INSERT-SELECT type of statement.
  // identified in genericUpdate::BindNode. Note that there is 
  // another flag similar to this in insert object. See Insert object
  // for more details.
  NABoolean insertSelectStatement_;

  // denotes that this is a MERGE statement.
  // Currently used to turn off ESP parallelism.
  // See class RelRoot, file RelMisc.h.
  NABoolean mergeStatement_;

  // a invocation number that uniquely identifies a routine invocation
  // in the SqlText.
  Int32 routineInvocationNum_;
  // --------------------------------------------------------------------
   // Rowset flags
  // --------------------------------------------------------------------

   // flag to indicate the presence of dynamic rowsets somewhere in the whole query. 
   // the hasDynamicRowsets_ flag in hostArraysWA is scoped, so in TrueRoot we do not
  // we have a rowset, it the rowset is present only in a subquery.
  NABoolean hasDynamicRowsetsInQuery_;

  NAString corrNameTokens_;
  UninitializedMvNameList *uninitializedMvList_;

  NABoolean isBindTrueRoot_;
  SQLATTRHOLDABLE_INTERNAL_TYPE holdableType_;

  // TRUE if there is no need to limit schema access. Default is FALSE.
  // This is used to temporarily remove the schema access limitation 
  // for some cases when DEFAULT_SCHEMA_ACCESS_ONLY is ON.
  NABoolean noNeedToLimitSchemaAccess_;

  // True if this a UNLOAD query
  NABoolean isFastExtract_;

  NABoolean isTrafLoadPrep_;

  NABoolean failedForPrivileges_;
  NABoolean shouldLogAccessViolations_;

  int queryCanUseSeaMonster_;
  NABoolean volatileTableFound_;

  // used to help bind agg functions that are compositions of other
  // primitive aggr-funcs such as AVG and VARIANLE
  BindScope *outerAggScope_;

  NABoolean hasCallStmts_;

  NAString ISPExecLocation_;

}; // class BindWA

class HbaseColUsageInfo : public NABasicObject
{
 public:
  HbaseColUsageInfo(NAMemory *h, Lng32 initSize = 67) 
    {
      HashFunctionPtr hashFunc = (HashFunctionPtr)(&BindWA::qualNameHashFunc);
      usageInfo_ = new (h) 
	NAHashDictionary<QualifiedName, NASet<NAString>>
	(hashFunc,initSize,TRUE,h);

      heap_ = h;
    }
  
  void insert(QualifiedName *tableName)
  {
    NASet<NAString> * v = new(heap_) NASet<NAString>(heap_);
    usageInfo_->insert(tableName, v);
  }
 
  void insert(QualifiedName *tableName, NAString *colName)
  {
    NASet<NAString> * cns = hbaseColNameSet(tableName);
    if (! cns)
      {
	insert(tableName);
	cns = hbaseColNameSet(tableName);
      }

    cns->insert(*colName);
  }
 
  NASet<NAString> *hbaseColNameSet(QualifiedName *tableName)
    { 
      NASet<NAString> *hcns = usageInfo_->getFirstValue(tableName);
      return hcns;
    }
  
 private:
 NAHashDictionary <QualifiedName, NASet<NAString>> * usageInfo_;

 NAMemory *heap_;
};

inline void setInUpdateOrInsert(BindWA *bindWA,
				GenericUpdate *gu = NULL,
				OperatorTypeEnum updins = NOT_UpdateOrInsert)
{
  BindContext *context = bindWA->getCurrentScope()->context();
  context->inUpdateOrInsert()    = updins;
  context->updateOrInsertNode()  = gu;
  context->updateOrInsertScope() =
    (updins == NOT_UpdateOrInsert) ? NULL : bindWA->getCurrentScope();
}

void extractOverrideSchemas(const char *value, NAString& fromSchema, NAString& toSchema);


#endif /* BindWA_H */
