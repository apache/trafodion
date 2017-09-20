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
#ifndef OPTTRIGGER_H
#define OPTTRIGGER_H
/* -*-C++-*-
**************************************************************************
*
* File:         OptTrigger.h
* Description:  After trigger backbone class for after trigger reorder & transformation
* Created:      6/24/98
* Language:     C++
*
*
*
*
**************************************************************************
*/

#include "AllRelExpr.h"
#include "AccessSets.h"
#include "Triggers.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class OptTriggersBackbone;
class OptTriggerGroupList;
class OptTriggerGroup;
class OptTriggerList;
class OptTrigger;
class OptColumnMapping;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class BindWA;
class TriggerBindInfo;

//***************************************************************************
//			OptTrigger
//
//	Hold single trigger information, needed in optimizer pilot phase for reorder	
//***************************************************************************

class OptTrigger : public NABasicObject
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  OptTrigger(SubTreeAccessSet *treeAccessSet, 
	     NABoolean isRowTrigger,
	     RelExpr *triggerSubTree, 
	     RelExpr *parentSubTree);

  // Dummy copy constructor - method body does not exist
  OptTrigger(const OptTrigger &orig, NAMemory *h=0); 
  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  inline const SubTreeAccessSet *getAccessSet()	const    {return treeAccessSet_;}
  inline NABoolean isRowTrigger() const			 {return isRowTrigger_;}
  inline const RelExpr * getTriggerSubTree() const		 {return triggerSubtree_;}		
  inline const RelExpr * getTriggerParentSubTree() const {return triggerParentSubtree_;}
  inline Int64 getTriggerTimeStamp() const
  {return (triggerSubtree_->getInliningInfo().getTriggerObject()->getTimeStamp());}

private:
  const SubTreeAccessSet *treeAccessSet_;
  const NABoolean isRowTrigger_;
  const RelExpr *triggerSubtree_; // from the Driving union down
  const RelExpr *triggerParentSubtree_;// union or NULL (for reuse)
};// OptTrigger

typedef OptTrigger *OptTriggerPtr; // this typedef is used in OptTriggersBackbone::reorder

//***********************************************************************
//	A list of OptTrigger
//***********************************************************************
class OptTriggerList : public LIST(OptTriggerPtr)
{
public:
  OptTriggerList(): LIST(OptTriggerPtr)(CmpCommon::statementHeap()) { }

  // Dummy copy constructor - method body does not exist
  OptTriggerList(const OptTriggerList &orig, NAMemory *h=0);

  RelExpr *createUnionSubTree() const;
};

//***********************************************************************
//
// OptTriggerGroup
//
// A group of trigger that has no conflict in their access sets
//
//***********************************************************************

class OptTriggerGroup  : public NABasicObject
{
public:

  enum TriggerGroupFlags {
    // placed in piplined action
    PIPELINED_ROW_TRIGGERS = 1,
    // executed in paraller to IUD
    IUD_PARALLEL_STATEMENT_TRIGGER,	
    // have conflict with previus group, 
    // can includes both row and statment triggers
    BLOCKED_GROUP						
  };

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  OptTriggerGroup(TriggerGroupFlags flag, OptTriggersBackbone *backbone);

  // Dummy copy constructor - method body does not exist
  OptTriggerGroup(const OptTriggerGroup &orig);

  virtual ~OptTriggerGroup() {}

  inline CollIndex entries() const 
    { return (rowTriggerGroup_.entries()+statmentTriggerGroup_.entries());}

  //If the group is empty the answer is FALSE
  NABoolean isConflicting(const SubTreeAccessSet *) const; 
  // add trigger to group
  void addTrigger(const OptTriggerPtr trigger); 
  // get group tree representation
  RelExpr *toTree();
  // creates the sub-tree that handles the row triggers in the group
  RelExpr *createRowTriggersSubTree();

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------

  inline NABoolean isPipeLinedRowTriggesGroup() const
    {return ((Flags_ == PIPELINED_ROW_TRIGGERS) ? TRUE : FALSE);}
  inline NABoolean isIudParallelStatmentGroup() const
    {return ((Flags_ == IUD_PARALLEL_STATEMENT_TRIGGER) ? TRUE : FALSE);}
  inline NABoolean isBlockedGroup() const
    {return ((Flags_ == BLOCKED_GROUP) ? TRUE :FALSE);}

private:

  //---------------------------------------------------------------------------
  // methods for generating the temp-table scan for blocked triggers' groups
  //---------------------------------------------------------------------------

  RelExpr *buildTempScanTree();
  
  RelExpr *buildTempTableScanNodes(OptColumnMapping& colMapping, 
				   BindWA           *bindWA);

  //---------------------------------------------------------------------------
  // data members
  //---------------------------------------------------------------------------

  OptTriggerList rowTriggerGroup_;// The triggers in the current group
  OptTriggerList statmentTriggerGroup_;// The triggers in the current group
  SubTreeAccessSet groupAccessSet_;
  const TriggerGroupFlags Flags_; // see triggerGroupFlags
  OptTriggersBackbone * backbone_;

  ValueIdSet reqOutput_; // required output of the generated temp-table scan
};// OptTriggerGroup



// ***********************************************************************
// A list of OptTriggerGrp
// ***********************************************************************
//
class OptTriggerGroupList : public LIST(OptTriggerGroup *)
{
public:
  OptTriggerGroupList(): LIST(OptTriggerGroup *)(CmpCommon::statementHeap()) { }

  // Dummy copy constructor - method body does not exist
  OptTriggerGroupList(const OptTriggerGroupList &orig, NAMemory *h=0);
};


//***********************************************************************
//
//			OptTriggersBackbone
//
// The after trigger backbone of single IUD statement. This class contains
// the backbone elements, to enable reassemble of the backbone after the
// reorder and grouping of the triggers.
//
//***********************************************************************
class OptTriggersBackbone  : public NABasicObject
{
public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------

  OptTriggersBackbone(RelExpr *drivingBackbone)
    : beforeTriggerDrivingNode_(NULL),
      tempDeleteDrivingNode_(NULL),
      statmentTriggerDrivingNode_(NULL),
      rowTriggerDrivingNode_(NULL),
      riDrivingNode_(NULL),
      pipeLineDrivingNode_(NULL),
      pipeLineDrivingNodeParent_(NULL),
      tempInsertDrivingNode_(NULL),
      tempInsertDrivingNodeParent_(NULL),
      iudDrivingNode_(NULL),
      iudDrivingNodeParent_(NULL),
      iudNode_(NULL),
      beforeTriggersExist_(FALSE),
      triggerList_(NULL),
      triggerGroups_(NULL),
      drivingBackbone_(drivingBackbone),
      pParent_(NULL),
      pNext_(NULL),
      triggerBindInfo_(NULL),
      tempTableUsage_(0L)
  {}

  ~OptTriggersBackbone();

  // initialize the object - set pointers to key nodes in the backbone
  void init();

  RelExpr *getTransformedTree();

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------

  inline NABoolean hasBeforeTriggers() const
    { return beforeTriggersExist_; }

  inline GenericUpdate *getTriggeringNode() const
    { return iudNode_; }

  inline RelExpr *getIudDrivingNode() const
    { return iudDrivingNode_; }

  inline TriggerBindInfo *getTriggerBindInfo() const
    { return triggerBindInfo_; }

  // force the cardinality of the given expression to be the same as the
  // IUD node that started the whole backbone
  void forceCardinalityAsIud(RelExpr *expr) const;

  ColReference *createNewExecIdRef();

  // ---------------------------------------------------------------------
  // Static utility methods for general use
  // ---------------------------------------------------------------------

  static void SetUnionCharacteristicInputs(RelExpr *pUnion);
  static void SetTsjCharacteristicInputs(RelExpr *pTsj);
  static void PrepareNewBindWA(BindWA &bindWA, TriggerBindInfo *bindInfo);
  static RelExpr * BindAndNormalizeNewlyConstructedTree(BindWA *bindWA, RelExpr *topNode);

private:

  // dummy copy constructor - method body does not exist
  OptTriggersBackbone(const OptTriggersBackbone &orig);

  NABoolean hasConflicts() const;
  NABoolean leftTreeHasConflicts(RelExpr *drivingNode) const;
  void removeTempInsert();
  RelExpr *removeTempDelete();
  void createTriggerList();
  void addToTriggerList(RelExpr *drivingNode, NABoolean isRowTrigger);
  void reorder();
  void group();
  RelExpr *cleanDummies(RelExpr *);
  void replacePipelinedRowTriggers(RelExpr *rowTriggerTree);
  RelExpr *toTree();

  void fixCardinalityAndCollectTempUsage(RelExpr *rootNode,
					 NABoolean enterInnerBackbone);
  void collectTempTableUsage(RelExpr *node);
  void setUsageAccordingToUniqueUidPredicate(const ValueIdSet &uniqueUidPred);
  void fixTempInsertSubtree();
  RelExpr *buildOptimizedTempInsert(BindWA *bindWA);

  inline void addTempTableUsage(Lng32 usage) { tempTableUsage_ |= usage; }

  // Tagging methods for the various key nodes in the backbone

  void initBeforeTriggerDrivingNode();
  void initTempDeleteDrivingNodeAndAdvance();
  void initStatmentTriggerDrivingNodeAndAdvance();
  void initPipeLineDrivingNode();
  void initRowTriggerDrivingNode();
  void initRiDrivingNode();

  GenericUpdate *findLeftmostGU(ExprNode *expr);
  void advanceInTree();

  // The blocked union node separating the before triggers from the after trigger
  RelExpr *beforeTriggerDrivingNode_;
  // The blocked union node on top of the temp-delete node
  RelExpr *tempDeleteDrivingNode_;
  // The ordered union node on top of the statement triggers
  RelExpr *statmentTriggerDrivingNode_;
  // The union node on top of the row triggers if and only if row triggers exist
  RelExpr *rowTriggerDrivingNode_;
  // The TSJ node on top of row triggers and/or RI
  RelExpr *pipeLineDrivingNode_;
  RelExpr *pipeLineDrivingNodeParent_;
  // The union node on top of the row RI, if and only if RI exists
  RelExpr *riDrivingNode_;
  // The TSJ on top of the temp-insert
  RelExpr *tempInsertDrivingNode_;
  RelExpr *tempInsertDrivingNodeParent_;
  // iud sub tree, include im if exists
  RelExpr *iudDrivingNode_;
  RelExpr *iudDrivingNodeParent_;
  // The triggering IUD node - the one that started it all
  GenericUpdate *iudNode_;
  
  NABoolean beforeTriggersExist_;
  OptTriggerList *triggerList_; 
  OptTriggerGroupList *triggerGroups_; 

  // The following members are in use during the initalization of the object
  // and through the tagging of the key nodes in the backbone.

  RelExpr *drivingBackbone_; // the tree to transform
  RelExpr *pNext_; // current node - the next to be evaluated
  RelExpr *pParent_; // parent of current node 

  TriggerBindInfo *triggerBindInfo_; // information collected during binding
  Lng32 tempTableUsage_; // specify which values are actually needed
};// class OptTriggersBackbone


//***********************************************************************
//
// OptColumnMapping
//
// A mapping between the old and new column names and their valueIds.
// This mapping holds only columns needed by a specific row trigger group.
//
//***********************************************************************
class OptColumnMapping  : public NABasicObject
{
public:
  struct ColumnMapping {
    const ColRefName *colName_;
    ValueId           topValueId_;
    ValueId           topVeg_;
  };
  typedef LIST(ColumnMapping *) MappingList;

  OptColumnMapping(TriggerBindInfo  *bindInfo, 
		   OperatorTypeEnum  iudOpType,
		   CollHeap         *heap);

  void initializeMappings(ValueIdSet &reqOutput);

  ItemExpr *buildSelectListFromNeededInputs();

  RelExpr *buildMapValueIdNode(RelExpr *topNode);

  inline NABoolean isOldNeeded() const { return oldNeeded_; }
  inline NABoolean isNewNeeded() const { return newNeeded_; }

private:
  void createInitialMappingLists(ValueIdSet &reqOutput);

  void findCommonColumns();
  void checkWhichTablesAreNeeded();

  ColumnMapping *findEqualCols(MappingList   &colsList,
			       ColumnMapping *colToFind,
			       CollIndex      startAt);

private:
  TriggerBindInfo *bindInfo_;
  OperatorTypeEnum iudOpType_;
  MappingList mappingArray_;  // The final mapping list

  MappingList newCols_;       // temp list for NEW columns.
  MappingList oldCols_;       // temp list for OLD columns.
  MappingList newCommonCols_; // temp list for NEW common columns.
  MappingList oldCommonCols_; // temp list for OLD common columns.

  NABoolean   oldNeeded_;     
  NABoolean   newNeeded_;

  NAString    atNew_;         // "@NEW"
  NAString    atOld_;         // "@OLD"
  NABoolean   dummyFeed_;     // TRUE when the triggers are not using any of the OLD and NEW values.

  CollHeap   *heap_;
};


#endif  /* OPTTRIGGER_H */
