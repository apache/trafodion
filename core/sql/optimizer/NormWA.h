#ifndef NormWA_H
#define NormWA_H
/* -*-C++-*-
*************************************************************************
*
* File:         NormWA.h
* Description:  The workarea used by the normalizer
* Created:      December 7, 1994
* Language:     C++
*
*
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
*
*
*************************************************************************
*/

#include "BaseTypes.h"
#include "Collections.h"
#include "VEGTable.h"
#include "ValueDesc.h"
// ----------------------------------------------------------------------
// contents of this file
// ----------------------------------------------------------------------
class NormWA;
class SqoWA;

// ----------------------------------------------------------------------
// Forward declarations
// ----------------------------------------------------------------------
class VEGReference;
class CmpContext;

// ***********************************************************************
// SqoWA
//
// The work area for the Semantic Query Optimizer phase.
//
// For now to keep track of things we have to undo if we hit 
// conditions where we cannot proceed with the SQO and have to restore
// the relExpr tree to its original form.
// ***********************************************************************

enum SqoChangedItemExprEnum { SQO_REPLACED, SQO_NEWCHILD, SQO_NEWOPTYPE };
enum SqoChangedRelExprEnum { SQO_REASSIGNED_VREGION };

class SqoChangedItemExprs: public NABasicObject
{
public:
     
        // Constructor
      SqoChangedItemExprs(ValueId changedId, SqoChangedItemExprEnum what, 
                          ItemExpr *old , Int32 changedChild, Lng32 subqId) : 
                             changedVid_(changedId), 
                             whatChanged_(what), 
                             oldItemExpr_(old), 
                             changedChild_(changedChild),
                             subqId_(subqId) 
      {}

      SqoChangedItemExprs(ValueId changedId, SqoChangedItemExprEnum what, 
                          ItemExpr *old, const OperatorTypeEnum oldOpType, 
                          Lng32 subqId) : 
                             changedVid_(changedId), 
                             whatChanged_(what), 
                             oldItemExpr_(old), 
                             changedChild_(0), 
                             oldOperatorType_(oldOpType),
                             subqId_(subqId) 
      {}

      SqoChangedItemExprs(ValueId changedId, SqoChangedItemExprEnum what, 
                          ItemExpr *old, Lng32 subqId ) : 
                             changedVid_(changedId), 
                             whatChanged_(what), 
                             oldItemExpr_(old), 
                             changedChild_(0),
                             subqId_(subqId) 
      {}

        // Destructor
      ~SqoChangedItemExprs() {}
      
      NABoolean undoChanges(NormWA & normWARef, Lng32 subqId);

private:
      Lng32                      subqId_;
      ValueId                   changedVid_;
      SqoChangedItemExprEnum    whatChanged_;
      ItemExpr                  *oldItemExpr_;
      Lng32                       changedChild_;
      OperatorTypeEnum          oldOperatorType_;
}; // class SqoChangedItemExprs


class SqoChangedRelExprs: public NABasicObject
{
public:
     
        // Constructor
      SqoChangedRelExprs(RegionId vegRegion, SqoChangedRelExprEnum what, 
                         RelExpr *changedExpr, RelExpr *origExpr, 
                         Lng32 origChildId, Lng32 changedChildId, Lng32 subqId) 
          : vregion_(vegRegion), whatChanged_(what), 
            changedRelExpr_(changedExpr), originalRelExpr_(origExpr), 
            origChildId_(origChildId), changedChildId_(changedChildId),
            subqId_(subqId) 
      {}

        // Destructor
      ~SqoChangedRelExprs() {}

      NABoolean undoChanges(NormWA & normWARef, Lng32 subqId);

private:
      Lng32                     subqId_;
      SqoChangedRelExprEnum    whatChanged_;
      RelExpr                  *changedRelExpr_;
      RelExpr                  *originalRelExpr_;
      Lng32                      changedChildId_;
      Lng32                      origChildId_;
      RegionId                 vregion_;
}; // class SqoChangedRelExprs

class SqoWA: public NABasicObject
{
public:
         // Constructors
      SqoWA() 
              : changedItemExprs_(CmpCommon::statementHeap()),
                changedRelExprs_(CmpCommon::statementHeap()),
                subqId_(0)
      {}

      SqoWA(const SqoWA & other) 
              : changedItemExprs_(other.changedItemExprs_),
                changedRelExprs_(other.changedRelExprs_),
                subqId_(other.subqId_)
      {}

         // Destructor
      ~SqoWA() {}

      void incrementSubQId() { subqId_ ++; }
      Lng32 getSubQId() { return subqId_; }

      void insertChangedItemExpr(ValueId changedId, SqoChangedItemExprEnum whatChanged, ItemExpr *origExpr )
      {
         changedItemExprs_.insert( new (CmpCommon::statementHeap())
                                     SqoChangedItemExprs(changedId, 
                                     whatChanged,
                                     origExpr, subqId_ )); 
      }

      void insertChangedItemExpr(ValueId changedId, SqoChangedItemExprEnum whatChanged, ItemExpr *origExpr, Int32 changedChild )
      {
         changedItemExprs_.insert( new (CmpCommon::statementHeap())
                                     SqoChangedItemExprs(changedId, 
                                     whatChanged,
                                     origExpr,
                                     changedChild, subqId_ )); 
      }

      void insertChangedItemExpr(ValueId changedId, SqoChangedItemExprEnum whatChanged, ItemExpr *origExpr, OperatorTypeEnum origOp )
      {
         changedItemExprs_.insert( new (CmpCommon::statementHeap())
                                     SqoChangedItemExprs(changedId, 
                                     whatChanged,
                                     origExpr,
                                     origOp, subqId_ )); 
      }
      void insertChangedRelExpr(RelExpr *originalRel, RelExpr *changedRel, SqoChangedRelExprEnum whatChanged, RegionId vregionId, Int32 origRegionChild, Int32 newRegionChild)
      {
         changedRelExprs_.insert( new (CmpCommon::statementHeap())
                                     SqoChangedRelExprs(vregionId, whatChanged,
                                        changedRel, originalRel,
                                        origRegionChild, 
                                        newRegionChild, subqId_)); 
      }

      void undoChanges(NormWA & normWARef );
  
private:
     Lng32   subqId_;
     // We need a list of ItemExprs that we have changed
     LIST(SqoChangedItemExprs *) changedItemExprs_;


     // We need to remember a list of RelExprs that we have changed
     // For now the only thing we change for RelExprs are the ownership
     // of VEGRegions, thus we have to remember the original owner and 
     // the RegionId.
     LIST(SqoChangedRelExprs *)  changedRelExprs_;

     
}; // class SqoWA



// ***********************************************************************
//
// Region
//
// A ValueId Equality Group (VEG) contains the ValueIds of all
// expressions that are related by an "=" relationship. This means,
// that each expression that belongs to a certain VEG will produce
// the same value as any other expression that belongs to the same 
// VEG. Aggregeting operation, such as, summation, averaging or 
// even an outer join impose certain restrictions on how VEGs are
// formed. For example, consider the query tree 
//
//                       IJ-> T1.c = T3.c
//                      /  \ 
//                    T1    IJ -> T2.c = T3.c
//                         /  \ 
//                       T2    T3
//
// The predicate T1.c = T3.c causes the columns T1.c and T3.c to 
// belong to a VEG, VEG1. The predicate T2.c = T3.c defines another 
// VEG, VEG2. Since each inner join transmits values without any 
// aggregation, it is clear that since T1.c = T3.c and T2.c = T3.c
// T1.c must also be equal to T2.c. Hence VEG1 and VEG2 can be 
// into a single VEG. Now consider the query tree,
//
//                                               R1
//                          ..........................
//                       LJ.-> T1.c = T3.c       R2
//                      / . \   ......................
//                    T1 .  LJ. -> T2.c = T3.c   R3
//                      .  / .\ 
//                     . T2  :  T3
//                     :     :.......................
//                     :..............................
//
// For the sake of disucssion, the query tree is partitioned into
// three zones or regions, called Regions R1, R2 and R3. Each Region
// defines a scope within which a certain equality relationship is
// valid. Thus the relationship T1.c = T3.c is valid only within R2
// while the relationship T2.c = T3.c is valid only within R3. The 
// effect of such a partionining is seen in the manner in which VEGs
// are formed. The predicate T1.c = T3.c causes the columns T1.c and 
// T3.c to belong to a VEG, VEG1. Similarly, the predicate T2.c = T3.c
// causes the columns T2.c and T3.c to belong to another VEG, VEG2.
// VEG1 and VEG2 cannot be combined because T3.c values that flow
// from R3 into R2 can suffer null augmentation and hence change.
//
// The normalizer creates a new Region for 
// a) the right subtree of a left outer join 
// b) the left subtree of a right outer join and 
// c) each of the left and right subtrees of a full outer join
//   
// ***********************************************************************

// ***********************************************************************
// NormWA
//
// The work area for the Transform/Normalize/Rewrite phase.
//
// ***********************************************************************
class NormWA : public NABasicObject
{
public:
  // --------------------------------------------------------------------
  // Constructor functions
  // --------------------------------------------------------------------

  NormWA(CmpContext* cmpContext)
    : currentCmpContext_(cmpContext), subqUnderExprTreeCount_(0),
      walkingAnExprTreeCount_(0), complexScalarExprCount_(0), 
      nullCount_(0), notCount_(0), orCount_(0), inConstraintsCount_(0),
	  inBlockedUnionCount_(0) //++Triggers -
      // QSTUFF
      ,inEmbeddedUpdateOrDelete_(FALSE)
      // QSTUFF
      ,inEmbeddedInsert_(FALSE)
      ,inBeforeTrigger_(FALSE)
      ,inJoinPredicate_(FALSE)
      ,mergeUpdDelCount_(0)
      ,leftJoinConversionCount_(0)
      ,correlatedSubqCount_(0)
      ,inSelectListCount_(0)
      ,inHavingClause_(FALSE)
      ,inValueIdProxy_(FALSE)
      ,inGenericUpdateAssign_(FALSE)
      ,inMergeUpdWhere_(FALSE)
      ,inMVQueryRewrite_(FALSE)
      ,containsJoinsToBeEliminated_(FALSE)
      ,containsSemiJoinsToBeTransformed_(FALSE)
      ,extraHubVertex_(NULL)
      ,leftJoinChildVEGRegion_(NULL)
      ,containsGroupBysToBeEliminated_(FALSE)
      ,checkForExtraHubTables_(FALSE)
      ,compilingMVDescriptor_(FALSE)
      ,requiresRecursivePushdown_(FALSE)
  {
    vegTable_ = new (wHeap()) VEGTable;
    sqoWARef_ = new (CmpCommon::statementHeap()) SqoWA;
  }

  
  NormWA(const NormWA & other) 
    : currentCmpContext_(other.currentCmpContext_),
      walkingAnExprTreeCount_(other.walkingAnExprTreeCount_),
      subqUnderExprTreeCount_(other.subqUnderExprTreeCount_),
      complexScalarExprCount_(other.complexScalarExprCount_), 
      nullCount_(other.nullCount_), 
      notCount_(other.notCount_), 
      orCount_(other.orCount_),
      inConstraintsCount_(other.inConstraintsCount_),
      inBlockedUnionCount_(other.inBlockedUnionCount_), //++ Triggers -
      vegTable_(other.vegTable_)
      // QSTUFF
      ,inEmbeddedUpdateOrDelete_(other.inEmbeddedUpdateOrDelete_)
      // QSTUFF
      ,inEmbeddedInsert_(other.inEmbeddedInsert_)
      ,inBeforeTrigger_(other.inBeforeTrigger_)
      ,inJoinPredicate_(other.inJoinPredicate_)
      ,mergeUpdDelCount_(other.mergeUpdDelCount_)
      ,leftJoinConversionCount_(other.leftJoinConversionCount_)
      ,correlatedSubqCount_(other.correlatedSubqCount_)
      ,inSelectListCount_(other.inSelectListCount_)
      ,inHavingClause_(other.inHavingClause_)
      ,inValueIdProxy_(other.inValueIdProxy_)
      ,inGenericUpdateAssign_(other.inGenericUpdateAssign_)
      ,inMergeUpdWhere_(other.inMergeUpdWhere_)
      ,inMVQueryRewrite_(other.inMVQueryRewrite_)
      ,containsJoinsToBeEliminated_(other.containsJoinsToBeEliminated_)
      ,containsSemiJoinsToBeTransformed_(other.containsSemiJoinsToBeTransformed_)
      ,extraHubVertex_(other.extraHubVertex_)
      ,sqoWARef_(other.sqoWARef_)
      ,leftJoinChildVEGRegion_(other.leftJoinChildVEGRegion_)
      ,containsGroupBysToBeEliminated_(other.containsGroupBysToBeEliminated_)
      ,checkForExtraHubTables_(FALSE)
      ,compilingMVDescriptor_(FALSE)
      ,requiresRecursivePushdown_(other.requiresRecursivePushdown_)
  {}
  
  // --------------------------------------------------------------------
  // Destructor functions
  // --------------------------------------------------------------------
  ~NormWA(){}
  
  // --------------------------------------------------------------------
  // Initialize a NormWA free of all state information.
  // This method is called at the root of each (sub)query tree.
  // --------------------------------------------------------------------
  void clearStateInformation()
  { walkingAnExprTreeCount_ = 
    complexScalarExprCount_ =
    nullCount_ = notCount_ = orCount_ = inConstraintsCount_ = 0;
    inValueIdProxy_ = FALSE;  // not a recursive flag.
  }
  
  // --------------------------------------------------------------------
  // Region
  // --------------------------------------------------------------------
  void allocateAndSetVEGRegion(const VEGRegionTypeEnum tev,
                               ExprNode * const ownerExprPtr,
                               Lng32 subtreeId = 0)
  { 
    vegTable_->allocateAndSetVEGRegion(tev,ownerExprPtr,subtreeId); 
  } 

  void locateAndSetVEGRegion(ExprNode * const ownerExprPtr,
                             Lng32 subtreeId = 0) 
           { vegTable_->locateAndSetVEGRegion(ownerExprPtr, subtreeId); }

  void restoreOriginalVEGRegion() { vegTable_->restoreOriginalRegion(); }
  void reassignVEGRegion(ExprNode * ownerExprPtr, 
                         Lng32       ownerSubtreeId,
                         ExprNode * newOwnerExprPtr, 
                         Lng32       newOwnerSubtreeId = 0) 
  { 
    vegTable_->reassignVEGRegion(ownerExprPtr, 
                      ownerSubtreeId, newOwnerExprPtr, newOwnerSubtreeId); 
  }

  // --------------------------------------------------------------------
  // Add a ValueId Equality Group (VEG) for each "=" predicate.
  // --------------------------------------------------------------------
  void addVEG(const ValueId & expr1Id, const ValueId & expr2Id)
                                 { vegTable_->addVEG(expr1Id, expr2Id); }

  void addVEG(const ValueIdSet & setOfValues)
                                      { vegTable_->addVEG(setOfValues); }

  void deleteVEGMember(const ValueId &vId)
                                     { vegTable_->deleteVEGMember(vId); }

  // ++Trigger -, from later version 
  void addVEGInOuterRegion(const ValueId & expr1Id, const ValueId & expr2Id)
                                 { vegTable_->addVEGInOuterRegion(expr1Id, expr2Id); }

  // --------------------------------------------------------------------
  // Accessor method for retrieving a VEG, given the ValueId of a 
  // potential member.
  // --------------------------------------------------------------------
  ItemExpr * getVEGReference(const ValueId & exprId) const
      { return vegTable_->getVEGReference(exprId,leftJoinChildVEGRegion_); }

  // --------------------------------------------------------------------
  // The following method locates the Region where the owner ExprNode
  // produces the given value and marks it as "To Be Merged".
  // --------------------------------------------------------------------
  VEGRegion * locateVEGRegionAndMarkToBeMerged(const ValueId & exprId) const
          { return vegTable_->locateVEGRegionAndMarkToBeMerged(exprId); }

  // --------------------------------------------------------------------
  // Do the same as above. But also look at the contents of the
  // ToBeMerged Regions to see if we could merge some more regions as
  // well because contents of the region might contain null_instantiated
  // columns in some other regions.
  // --------------------------------------------------------------------
  void locateVEGRegionAndMarkToBeMergedRecursively(const ValueId & exprId);

  // --------------------------------------------------------------------
  // The following method locates the Region of which the given ExprNode
  // is the owner and returns TRUE if it is merged; FALSE otherwise.
  // --------------------------------------------------------------------
  NABoolean locateVEGRegionAndCheckIfMerged(ExprNode * const ownerExpr) const
        { return vegTable_->locateVEGRegionAndCheckIfMerged(ownerExpr); }
  
  NABoolean locateVEGRegionAndCheckIfMerged
  (ExprNode * const ownerExpr, Lng32 subtreeId) const
  { return vegTable_->locateVEGRegionAndCheckIfMerged(ownerExpr, subtreeId); }
  

  // --------------------------------------------------------------------
  // Post processing of the VEGTable after it is built.
  // --------------------------------------------------------------------
  void processVEGRegions()            { vegTable_->processVEGRegions(); }

  //-----------------------------------------------------------------------
  //A method to locate the VEG region given an ExprNode
  //-----------------------------------------------------------------------
  VEGRegion* locateVEGRegion(ExprNode *ownerENptr)
                            { return vegTable_->locateVEGRegion(ownerENptr);}

  //-----------------------------------------------------------------------
  //A method to locate the VEG region given an ExprNode and subtreeId
  //-----------------------------------------------------------------------
  VEGRegion* locateVEGRegion(ExprNode *ownerENptr, Lng32 subtreeId)
                            { return vegTable_->locateVEGRegion(ownerENptr, subtreeId);}


  // --------------------------------------------------------------------
  // A method for performing transitive closure of "=" predicates.
  // --------------------------------------------------------------------
  ItemExpr * performTC(const ValueId & vegMember) const
                              { return vegTable_->performTC(vegMember); }
  
  // --------------------------------------------------------------------
  // The Normalizer sets the "walking an expression tree" flag when
  // processing one of the following expressions:
  // 1) An IF-THEN-ELSE (SQL CASE statement).
  // 2) An aggregate function.
  // 3) A predicate tree that is root in an OR.
  // 4) A predicate tree that is root in a  NOT.
  // 5) A predicate tree that is root in an IS NULL/IS UNKNOWN.
  // The method that sets it is also expected to restore it to its 
  // original state.
  // --------------------------------------------------------------------
  void setWalkingAnExprTreeFlag()       { walkingAnExprTreeCount_++; }
  void restoreWalkingAnExprTreeFlag()   { walkingAnExprTreeCount_--; }
  NABoolean walkingAnExprTree() const   { return walkingAnExprTreeCount_ > 0; }
  Lng32 getWalkingAnExprTreeCount()      { return walkingAnExprTreeCount_; }

  // ---------------------------------------------------------------------
  // Needs to remember this subquery is under an expr, so that when we go
  // back to transform the new Join and its subtree introduced, we won't
  // incorrectly use the selection predicates in the subquery to convert
  // left join into inner join.
  // ---------------------------------------------------------------------
  void setSubqUnderExprTreeFlag()       { subqUnderExprTreeCount_++; }
  void restoreSubqUnderExprTreeFlag()   { subqUnderExprTreeCount_--; }
  NABoolean subqUnderExprTree() const   { return subqUnderExprTreeCount_ > 0; }

  // --------------------------------------------------------------------
  // The Normalizer sets the complex scalar expression flag while 
  // processing one of the following expressions:
  // 1) An IF-THEN-ELSE (SQL CASE statement).
  // 2) A user defined function.
  // The method that sets it is also expected to restore it to its 
  // original state.
  // --------------------------------------------------------------------
  void setComplexScalarExprFlag(); 
  void restoreComplexScalarExprFlag(); 
  NABoolean inAComplexScalarExpr() const { return complexScalarExprCount_ > 0; }

  // --------------------------------------------------------------------
  // The Normalizer NULL flag is usually set when an IS NULL or an
  // IS UNKNOWN is encountered in the transformed tree. The method 
  // that sets it is also expected to restore it to its original state.
  // This is because normalization is performed recursively and an
  // IS NULL or an IS UNKNOWN may be encountered while processing
  // the operand of another ISNULL or IS UNKNOWN.
  // The subquery transformation uses this information. 
  // --------------------------------------------------------------------
  void setNullFlag(); 
  void restoreNullFlag(); 
  NABoolean isChildOfAnIsNull() const   { return nullCount_ > 0; }
  
  // --------------------------------------------------------------------
  // The Normalizer NOT flag is usually set when a NOT is encountered 
  // in the transformed tree. The method that sets it is also expected 
  // to restore it to its original state. This is because normalization 
  // is performed recursively and a NOT may be encountered while 
  // processing the operand of another NOT.
  // The subquery transformation uses this information. 
  // --------------------------------------------------------------------
  void setNotFlag(); 
  void restoreNotFlag(); 
  NABoolean isChildOfANot() const       { return notCount_ > 0; }

  // --------------------------------------------------------------------
  // The Normalizer OR flag is usually set when an OR is encountered 
  // in the transformed tree. The method that sets it is also expected 
  // to restore it to its original state. This is because normalization 
  // is performed recursively and an OR may be encountered while 
  // processing the subtree rooted in another OR.
  // The subquery transformation uses this information. 
  // --------------------------------------------------------------------
  void setOrFlag(); 
  void restoreOrFlag(); 
  NABoolean haveAnOrAncestor() const    { return orCount_ > 0; }

  // --------------------------------------------------------------------
  // Check constraints are relied upon to enforce things, so we cannot
  // rely on them and do certain transforms (eliminate/strength-reduce)
  // when we are transforming constraints -- that would be circulus in probando.
  // --------------------------------------------------------------------
  void setInConstraintsFlag()           { inConstraintsCount_++; }
  void restoreInConstraintsFlag()       { inConstraintsCount_--; }
  NABoolean inConstraints() const       { return inConstraintsCount_ > 0; }

  // --------------------------------------------------------------------
  // 
  // If a recursive/cascaded triggers backbone has a connection to the 
  // triggering generic update the connection is using @old and @new 
  // columns from the triggering generic update.
  // We found out that equality relation between each two levels of cascaded 
  // triggers might cause the creation of a huge VEGREF that the optimizer  
  // can't handle in a reasonable time.
  // To prevent this problem will block the creation of VEGs on equal predicates
  // involving a @old/ @new columns on high cascade levels.
  // Cascade levels are counted by inBlockedUnionCount_, since every cascaded 
  // trigger backbone top node is a BlockedUnion
  //
  // --------------------------------------------------------------------
  void setInBlockedUnionCount()		{ inBlockedUnionCount_++; }
  void restoreInBlockedUnionCount()	{inBlockedUnionCount_--; }
  Lng32 getInBlockedUnionCount()		{ return inBlockedUnionCount_; }

  // retrieve the current CmpContext
  CmpContext* currentCmpContext() const { return currentCmpContext_; }

  // retrieve the wHeap
  CollHeap* wHeap();

  //QSTUFF
  // these lists contain the names of tables read and updates
  // respectively to check whether there is a potential read/write
  // conflict
  LIST(NAString) &getWriteList()  { return writeList_; }
  LIST(NAString) &getReadList()   { return readList_; }

  NABoolean isInEmbeddedUpdateOrDelete() { return inEmbeddedUpdateOrDelete_; }
  void      setInEmbeddedUpdateOrDelete(NABoolean i) { inEmbeddedUpdateOrDelete_ = i;}
  //QSTUFF

  // Support for embedded inserts from VALUES
  NABoolean isInEmbeddedInsert() { return inEmbeddedInsert_; }
  void      setInEmbeddedInsert (NABoolean i) { inEmbeddedInsert_ = i;}
  
  NABoolean isInBeforeTrigger() { return inBeforeTrigger_; }
  void      setInBeforeTrigger(NABoolean i) { inBeforeTrigger_ = i;}

  NABoolean isInJoinPredicate() { return inJoinPredicate_; }
  void      setInJoinPredicate(NABoolean i) { inJoinPredicate_ = i;}
  
  long getMergeUpdDelCount()	    { return mergeUpdDelCount_; }
  void setMergeUpdDelCount(long val) {mergeUpdDelCount_ = val;}
  void incrementMergeUpdDelCount()   { mergeUpdDelCount_++; }

  NABoolean requiresLeftTSJLinearization() 
  { return leftJoinConversionCount_ > 1; }
  
  void incrementLeftJoinConversionCount()   { leftJoinConversionCount_++; }
  void decrementLeftJoinConversionCount()   { leftJoinConversionCount_--; }
  Lng32 getLeftJoinConversionCount()	    { return leftJoinConversionCount_; }
  void setLeftJoinConversionCount(Lng32 val) {leftJoinConversionCount_ = val;}

  NABoolean requiresSemanticQueryOptimization() 
  { return ((correlatedSubqCount_ > 0) || 
	    containsJoinsToBeEliminated_ ||
            checkForExtraHubTables_ ||
	    containsGroupBysToBeEliminated_ ||
	    containsSemiJoinsToBeTransformed_ ); }
  
  void incrementCorrelatedSubqCount()		{ correlatedSubqCount_++; }
  void decrementCorrelatedSubqCount()	{correlatedSubqCount_--; }
  Lng32 getCorrelatedSubqCount()		{ return correlatedSubqCount_; }
  void setCorrelatedSubqCount(Lng32 val)  {correlatedSubqCount_ = val;}

  NABoolean requiresRecursivePushdown() 
          { return requiresRecursivePushdown_;}
  void setRequiresRecursivePushdown(NABoolean val) 
	  {requiresRecursivePushdown_ = val;}

  NABoolean containsJoinsToBeEliminated() 
          { return containsJoinsToBeEliminated_;}
  void setContainsJoinsToBeEliminated(NABoolean val) 
	  {containsJoinsToBeEliminated_ = val;}

  NABoolean checkForExtraHubTables()
          { return checkForExtraHubTables_;}
  void setCheckForExtraHubTables(NABoolean val)
          {checkForExtraHubTables_ = val;}

  NABoolean containsGroupBysToBeEliminated() 
                   { return containsGroupBysToBeEliminated_;}
  void setContainsGroupBysToBeEliminated(NABoolean val) 
	           {containsGroupBysToBeEliminated_ = val;}

  NABoolean containsSemiJoinsToBeTransformed() 
          { return containsSemiJoinsToBeTransformed_;}
  void setContainsSemiJoinsToBeTransformed(NABoolean val) 
	  {containsSemiJoinsToBeTransformed_ = val;}
  RelExpr* getExtraHubVertex() { return extraHubVertex_;}
  void setExtraHubVertex(RelExpr* ptr) 
	  {extraHubVertex_ = ptr;}

  // Return a reference to the Semantic Query WA
  SqoWA * getSqoWA() { return sqoWARef_;}


  // --------------------------------------------------------------------
  // counter to record if we are currently transforming expressions in a select 
  // list (i.e. compExpr). This is used for subquery unesting. A subquery in a 
  // select list does not reject null values since there is no predicate 
  // outside the subquery and the result if the subquery is produced as is. 
  // If the subquery produces a null value that must be retained. Therefore 
  // subqureries in a select list are equivalent to other predicates that do 
  // not reject null values (for e.g. IS NOT NULL, count(*) = 0, ...)
  // --------------------------------------------------------------------
  void setInSelectList()           { inSelectListCount_++; }
  void restoreInSelectList()       { inSelectListCount_--; }
  NABoolean inSelectList() const        { return inSelectListCount_ > 0; }

  // --------------------------------------------------------------------
  // flag to record if we are currently transforming expressions in a HAVING 
  // clause (i.e. selection pred of GroupByAgg). This is used for subquery 
  // unesting. A subquery in a HAVING clause currently cannot be unnested.
  // --------------------------------------------------------------------
  void setInHavingClause(NABoolean val )       { inHavingClause_ = val; }
  NABoolean inHavingClause() const             { return inHavingClause_; }

  // --------------------------------------------------------------------
  // flag to record if we are currently transforming a ValueIdProxy
  // The assumption is that the we have already split out the different 
  // outputs of MVFs or Subqueries of degree > 1. Thus we want the transform
  // to only return a descrete value, not a list.
  // This is used for flattening ITM_ITEM_LISTS of subqueries with degree > 1 
  // and MVFs
  // --------------------------------------------------------------------
  void setInValueIdProxy(NABoolean val ) { inValueIdProxy_ = val; }
  NABoolean inValueIdProxy() const       { return inValueIdProxy_; }

  // --------------------------------------------------------------------
  // flag to record if we are currently transforming expressions 
  // associated with a assign expression of a GenericUpdate node 
  // Examples are newRecExpr, insert expresion for mergeThis is used for subquery unesting. 
  // A subquery in an INSERT/UPDATE/DELETE needs a left join to be unnested.
  // --------------------------------------------------------------------
  void setInGenericUpdateAssign(NABoolean val) {inGenericUpdateAssign_ = val;}
  NABoolean inGenericUpdateAssign() const           {return inGenericUpdateAssign_;}

  // flag to record if we are currently transforming expressions in 
  // a Merge ... Update ... Where predicate. 
  void setInMergeUpdWhere(NABoolean val) {inMergeUpdWhere_ = val;}
  NABoolean inMergeUpdWhere() const {return inMergeUpdWhere_;}

  // Use this to avoid rangespec transformation in the Normalizer if called by
  // MVQR for a node in rewritten query (since it will already have been done).
  void setInMVQueryRewrite(NABoolean val) { inMVQueryRewrite_ = val; }
  NABoolean inMVQueryRewrite() const      { return inMVQueryRewrite_; }

  //----------------------------------------------------------------------
  //
  void setAllSeqFunctions(ValueIdSet v) { allSeqFunctions_ = v;}
  ValueIdSet getAllSeqFunctions() const { return allSeqFunctions_; }
  void resetAllSeqFunctions () { allSeqFunctions_.clear(); }
  void addSeqFunction( ValueId v) { allSeqFunctions_ += v ; }
  ValueId getEquivalentItmSequenceFunction(ValueId newSeqId);
  void optimizeSeqFunctions( ItemExpr * ie, ItemExpr * pie, Int32 idx );


  void resetSeqFunctionsCache(){
    origSeqFunction_.clear();
    equiTransformedExpr_.clear();
  }

  void insertIntoSeqFunctionsCache (ItemExpr * orig, ValueId transformed) {
    origSeqFunction_.insert( orig);
    equiTransformedExpr_.insert(  transformed );
    CMPASSERT (origSeqFunction_.entries() == equiTransformedExpr_.entries());
  }

  NABoolean retrieveFromSeqFunctionsCache(CollIndex index, ValueId &transformed) {
    if (index <0 || index > origSeqFunction_.entries()-1) {
      return FALSE;
    }
    //orig = origSeqFunction_[index];
    transformed = equiTransformedExpr_[index];
    return TRUE;
  }

  CollIndex SeqFunctionsCacheEntries(){
    CMPASSERT (origSeqFunction_.entries() == equiTransformedExpr_.entries());
    return origSeqFunction_.entries();
  }
  
  NABoolean findEquivalentInSeqFunctionsCache( ItemExpr * newItem , ValueId &cacheEquivTransSeqId);

  VEGTable * getVEGTable() {return vegTable_ ;}

  const RelExpr * getCurrentOwnerExpr() ;

  void saveLeftJoinChildVEGRegion(ExprNode * const ownerExprPtr, 
                            Lng32 subtreeId = 0) 
  {leftJoinChildVEGRegion_ = 
           vegTable_->locateVEGRegion(ownerExprPtr, subtreeId); } 

  void resetLeftJoinChildVEGRegion() {leftJoinChildVEGRegion_ = NULL; }

  void setCompilingMVDescriptor(NABoolean b)  { compilingMVDescriptor_ = b; }

  NABoolean compilingMVDescriptor() { return compilingMVDescriptor_; }

private:

  // --------------------------------------------------------------------
  // Counters that keep track of whether an expression is the child of
  // an ISNULL, IS UNKNOWN or a NOT.
  // Whenever a new counter is added, update the following methods:
  // 1) default constructor
  // 2) copy constructor
  // 3) clearStateInformation()
  // --------------------------------------------------------------------
  Lng32 walkingAnExprTreeCount_;
  Lng32 subqUnderExprTreeCount_;

  Lng32 complexScalarExprCount_;

  Lng32 nullCount_;
  
  Lng32 notCount_;

  Lng32 orCount_;

  Lng32 inConstraintsCount_;

  // ++Triggers -
  Lng32 inBlockedUnionCount_;

  // QSTUFF

  // this flag is set to prevent a constant equalitity predicate, e.g.
  // x = 10, to become veggyfied when normalizing an expression containing
  // an embedded delete or update. This prevents the predicate from being
  // lost when not pushing it down to leave operators due to the veg relation
  
  NABoolean inEmbeddedUpdateOrDelete_;

 // this flag is set to prevent a constant equalitity predicate, e.g.
  // x = 10, to become veggyfied when normalizing an expression containing
  // an embedded insert from VALUES. This prevents the predicate from being
  // lost when not pushing it down to leave operators due to the veg relation
  
  NABoolean inEmbeddedInsert_;
   
  // this flag is set to prevent some transformations for binary logic
  // that does not take before triggers into account

  NABoolean inBeforeTrigger_;

  // this flag is set so that ItemExpr trees are not modified during calls
  // to ItemExpr::normalizeNode, when normalizing (i.e. rewriting) the join 
  // predicate. All modifications will be done on a copy of the original 
  // itemexpr. This is necessary since the same valueid may be used in 
  // others parts of the query tree that are in different veg regions. 
  // and any veg rewite that is correct in one region may not be in other 
  // regions.

  NABoolean inJoinPredicate_ ;

  // counter to record the number of mergeupdate/delete in query
  long mergeUpdDelCount_;
  
  // counter to record the number of correlated subqueries where we need to
  // transform the TSJ into a Left Join. During transform 
  // this counter is incremented for each subquery that matches criteria for
  // subquery unnesting.  The criteria is that we have a correlated Subquery
  // (TSJ->GB->Filter), and the GroupBy contains nonNullRejecting predicates
  // (like count() etc)
  Lng32 leftJoinConversionCount_ ;

  // counter to record the number of correlated subqueries. During transform 
  // this counter is incremented for each subquery that matches criteria for
  // subquery unnesting. Then during normalize the counter will be decremented
  // for each qualifying subquery that is not correlated. The during the SQO phase
  // we undertake a tree walk only if there is at least one correlated subquery.
  Lng32 correlatedSubqCount_ ;

  // counter to record if we are currently transforming expressions in a select 
  // list (i.e. compExpr). This is used for subquery unesting. A subquery in a 
  // select list does not reject null values since there is no predicate outside
  // the subquery and the result if the subquery is produced as is. If the 
  // subquery produces a null value that must be reatined. Therefor 
  // subqureries in a select list are equivalent to other predicates that 
  // do not reject null values (like IS NOT NULL, count(*) = 0, ...)

  Lng32 inSelectListCount_ ;

  NABoolean inHavingClause_ ;

  NABoolean inValueIdProxy_ ;

  NABoolean inGenericUpdateAssign_ ;

  NABoolean inMergeUpdWhere_ ;

  NABoolean inMVQueryRewrite_ ;

  NABoolean containsJoinsToBeEliminated_ ;

  NABoolean checkForExtraHubTables_;

  NABoolean containsGroupBysToBeEliminated_;

  NABoolean containsSemiJoinsToBeTransformed_ ;

  RelExpr * extraHubVertex_;

  // Pointer to Semantic Query Optimization WA. Used for error recovery
  SqoWA * sqoWARef_;

  // this variable are used to check whether the same table is both
  // updated and read
  LIST(NAString)  readList_;
  LIST(NAString)  writeList_;
  // QSTUFF

  // --------------------------------------------------------------------
  // A VEGTable instance is allocted in the NormWA
  // Once could either call it a vegTable_ because it is a table of
  // VEG's or a salad_ because it is a collection of VEG's. The name
  // salad_ is more appropriate because the vegTable_ is not a table
  // but a list. 
  // --------------------------------------------------------------------
  VEGTable * vegTable_;          
  
  // place to hold the current CmpContext
  CmpContext* currentCmpContext_;
  
  ValueIdSet allSeqFunctions_;

  LIST(ItemExpr *) origSeqFunction_;
  LIST(ValueId) equiTransformedExpr_;

  VEGRegion *leftJoinChildVEGRegion_; 

  NABoolean compilingMVDescriptor_;

  NABoolean requiresRecursivePushdown_;

}; // class NormWA


#endif /* NormWA_H */
