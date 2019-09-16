/***********************************************************************
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
/* -*-C++-*-
******************************************************************************
*
* File:         RelExpr.C
* Description:  Relational expressions (both physical and logical operators)
* Created:      5/17/94
* Language:     C++
*
*
******************************************************************************
*/

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "Debug.h"
#include "Sqlcomp.h"
#include "AllRelExpr.h"
#include "AllItemExpr.h"
#include "GroupAttr.h"
#include "opt.h"
#include "PhyProp.h"
#include "mdam.h"
#include "ControlDB.h"
#include "disjuncts.h"
#include "ScanOptimizer.h"
#include "CmpContext.h"
#include "StmtDDLCreateTrigger.h"
#include "ExpError.h"
#include "ComTransInfo.h"
#include "BindWA.h"
#include "Refresh.h"
#include "CmpMain.h"
#include "ControlDB.h"
#include "ElemDDLColDef.h"
#include "Analyzer.h"
#include "OptHints.h"
#include "ComTdbSendTop.h"
#include "DatetimeType.h"
#include "SequenceGeneratorAttributes.h"
#include "SqlParserGlobals.h"	
#include "AppliedStatMan.h"
#include "Generator.h"
#include "CmpStatement.h"

#define TEXT_DISPLAY_LENGTH 1001

// ----------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------

// -----------------------------------------------------------------------
// methods for class ExprGroupId
// -----------------------------------------------------------------------

ExprGroupId::ExprGroupId()
{
  groupIdMode_ = STANDALONE;
  node_        = NULL;
  groupId_     = INVALID_GROUP_ID;
}

ExprGroupId::ExprGroupId(const ExprGroupId & other)
{
  groupIdMode_ = other.groupIdMode_;
  node_        = other.node_;
  groupId_     = other.groupId_;
}

ExprGroupId::ExprGroupId(RelExpr *node)
{
  groupIdMode_ = STANDALONE;
  node_        = node;
  groupId_     = INVALID_GROUP_ID;
}

ExprGroupId::ExprGroupId(CascadesGroupId groupId)
{
  groupIdMode_ = MEMOIZED;
  node_        = NULL;
  groupId_     = groupId;
}

ExprGroupId & ExprGroupId::operator = (const ExprGroupId & other)
{
  groupIdMode_ = other.groupIdMode_;
  node_        = other.node_;
  groupId_     = other.groupId_;
  return *this;
}

ExprGroupId & ExprGroupId::operator = (RelExpr * other)
{
  if (groupIdMode_ == MEMOIZED)
    {
      // Trying to assign an actual pointer to an ExprGroupId that
      // is in CascadesMemo. This is materialization of a binding.
      groupIdMode_ = BINDING;
    }
  else if (groupIdMode_ == BINDING)
    // sanity check, can't directly overwrite another binding
    ABORT("Didn't call BINDING::release_expr()");

  node_ = other;

  return *this;
}

ExprGroupId & ExprGroupId::operator = (CascadesGroupId other)
{
  // The expression is now (again) in CascadesMemo without participating in
  // a binding. This may happen when an expression is copied into CascadesMemo
  // (groupIdMode_ was STANDALONE) or when a binding is released (groupIdMode_
  // was BINDING). The node_ member is no longer a valid pointer.

  if (groupIdMode_ == BINDING && groupId_ != other)
    ABORT("can't change group of an expression during release of binding");

  groupIdMode_ = MEMOIZED;
  groupId_ = other;
  node_ = NULL;

  return *this;
}

NABoolean ExprGroupId::operator == (const ExprGroupId &other) const
{
  // if the two operands have mode ...	 	then do this:
  // ----------------------------------------   ------------------
  // STANDALONE-STANDALONE:			ptrs must match
  // STANDALONE-MEMOIZED:	(x)		return FALSE
  // STANDALONE-BINDING:			ptrs must match
  // MEMOIZED-MEMOIZED:		(x)		groups must match
  // MEMOIZED-BINDING:		(x)		groups must match
  // BINDING-BINDING:				ptrs must match

  if (node_ == NULL OR other.getPtr() == NULL)
    return (groupId_ == other.getGroupId());    // cases with (x)
  else
    return (node_ == other.getPtr());           // ptrs must match
}

NABoolean ExprGroupId::operator == (const RelExpr *other) const
{
  CMPASSERT(groupIdMode_ != MEMOIZED);
  return node_ == other;
}

CascadesGroupId ExprGroupId::getGroupId() const
 { return ((groupIdMode_ != STANDALONE)? groupId_ : INVALID_GROUP_ID); }

void ExprGroupId::releaseBinding()
{
  if (groupIdMode_ != BINDING)
    ABORT("binding to release was not established");

  groupIdMode_ = MEMOIZED;
  node_ = NULL;
}

void ExprGroupId::convertBindingToStandalone()
{
  groupIdMode_ = STANDALONE;
}

void ExprGroupId::setGroupAttr(GroupAttributes *gaPtr)
{
  // If the expression is either in the standalone mode or is
  // a part of a binding, then store the Group Attributes
  // in the node. Group attributes in Cascades can not be set through
  // an individual expression and an attempt to do this results in an abort.

  CMPASSERT(groupIdMode_ == STANDALONE);
  node_->setGroupAttr(gaPtr);
} // ExprGroupId::setGroupAttr()

GroupAttributes * ExprGroupId::getGroupAttr() const
{
  CMPASSERT(node_ != NULL OR groupIdMode_ == MEMOIZED);

  // If the expression is either in the standalone mode or is
  // a part of a binding, then use the Group Attributes that
  // are stored in the node.
  if (node_ != NULL)
    return node_->getGroupAttr();
  else
    // otherwise, use the Cascades group's group attributes
    return  (*CURRSTMT_OPTGLOBALS->memo)[groupId_]->getGroupAttr();
} // ExprGroupId::getGroupAttr()

// shortcut to get the output estimated log props out of the group
// attributes
EstLogPropSharedPtr ExprGroupId::outputLogProp(const EstLogPropSharedPtr& inputLogProp)
{
  return getGroupAttr()->outputLogProp(inputLogProp);
}

// a shortcut to get the bound expression, if it exists ...
// or the first logical expression inserted in the Cascades Group or NULL.
// Note:  The last log expr in the list is the first one inserted.
RelExpr * ExprGroupId::getLogExpr() const
{
  if (node_ != NULL)
    return node_;
  else if (groupId_ != INVALID_GROUP_ID)
    return ((*CURRSTMT_OPTGLOBALS->memo)[groupId_]->getLastLogExpr());
  return 0;
}

RelExpr * ExprGroupId::getFirstLogExpr() const
{
  if (node_ != NULL)
    return node_;
  else if (groupId_ != INVALID_GROUP_ID)
    return ((*CURRSTMT_OPTGLOBALS->memo)[groupId_]->getFirstLogExpr());
  return 0;
}

// -----------------------------------------------------------------------
// member functions for class RelExpr
// -----------------------------------------------------------------------

THREAD_P ObjectCounter (*RelExpr::counter_)(0);

RelExpr::RelExpr(OperatorTypeEnum otype,
		 RelExpr  *leftChild,
		 RelExpr  *rightChild,
		 CollHeap *outHeap)
     : ExprNode(otype)
  ,selection_(NULL)
  ,RETDesc_(NULL)
  ,groupAttr_(NULL)
  ,groupId_(INVALID_GROUP_ID)
  ,groupNext_(NULL)
  ,bucketNext_(NULL)
  ,operatorCost_(NULL)
  ,rollUpCost_(NULL)
  ,physProp_(NULL)
  ,estRowsUsed_((Cardinality)-1)
  ,inputCardinality_((Cardinality)-1)
  ,maxCardEst_((Cardinality)-1)
  ,contextInsensRules_(outHeap)
  ,contextSensRules_(outHeap)
  ,accessSet0_(NULL) // Triggers --
  ,accessSet1_(NULL)
  ,uniqueColumnsTree_(NULL) //++MV
  ,cardConstraint_(NULL) //++MV
  ,isinBlockStmt_(FALSE)
  ,firstNRows_(-1)
  ,flags_(0)
  ,rowsetIterator_(FALSE)
  ,tolerateNonFatalError_(UNSPECIFIED_)
  ,hint_(NULL)
  ,markedForElimination_(FALSE)
  ,isExtraHub_(FALSE)
  ,potential_(-1)
  ,seenIUD_(FALSE)
  ,parentTaskId_(0)
  ,stride_(0)
  ,birthId_(0)
  ,memoExprId_(0)
  ,sourceMemoExprId_(0)
  ,sourceGroupId_(0)
  ,costLimit_(-1)
  ,cachedTupleFormat_(ExpTupleDesc::UNINITIALIZED_FORMAT)
  ,cachedResizeCIFRecord_(FALSE)
  ,dopReduced_(FALSE)
  ,originalExpr_(NULL)
  ,operKey_(outHeap)
{

  child_[0] = leftChild;
  child_[1] = rightChild;
  (*counter_).incrementCounter();
  //  QSTUFF
  setGroupAttr(new (outHeap) GroupAttributes);
  // QSTUFF
}

RelExpr::~RelExpr()
{
  // the group attributes maintain a reference count
  if (groupAttr_ != NULL)
    groupAttr_->decrementReferenceCount();

  // these data structures are always owned by the tree
  delete selection_;

  // delete all children, if this is a standalone query
  // (NOTE: can't use the virtual function getArity() in a destructor!!!)
  for (Lng32 i = 0; i < MAX_REL_ARITY; i++)
    {
      if (child(i).getMode() == ExprGroupId::STANDALONE)
	{
	  // the input was not obtained from CascadesMemo, so delete it
	  if (child(i).getPtr() != NULL)
	    delete child(i).getPtr();
	}
    }
  (*counter_).decrementCounter();

  delete cardConstraint_; //++MV

  if (hint_) delete hint_;
} // RelExpr::~RelExpr()


Int32 RelExpr::getArity() const
{
  switch (getOperatorType())
    {
    case REL_SCAN:
      return 0;

    case REL_EXCHANGE:
      return 1;

    case REL_JOIN:
    case REL_TSJ:
    case REL_ROUTINE_JOIN:
    case REL_SEMIJOIN:
    case REL_SEMITSJ:
    case REL_ANTI_SEMIJOIN:
    case REL_ANTI_SEMITSJ:
    case REL_LEFT_JOIN:
    case REL_FULL_JOIN:
    case REL_LEFT_TSJ:
    case REL_NESTED_JOIN:
    case REL_MERGE_JOIN:
    case REL_INTERSECT:
    case REL_EXCEPT:
      return 2;

    default:
      ABORT("RelExpr with unknown arity encountered");
      return 0;
    }
}

void RelExpr::deleteInstance()
{
  Int32 nc = getArity();
  // avoid deleting the children by resetting all child pointers first
  for (Lng32 i = 0; i < nc; i++)
    {
      child(i) = (RelExpr *) NULL;
    }
  delete this;
} // RelExpr::deleteInstance()

TableMappingUDF *RelExpr::castToTableMappingUDF()
{
  return NULL;
}

ExprNode * RelExpr::getChild(Lng32 index)
{
  return child(index);
} // RelExpr::getChild()

void RelExpr::setChild(Lng32 index, ExprNode * newChild)
{
  if (newChild)
    {
      CMPASSERT(newChild->castToRelExpr());
      child(index) = newChild->castToRelExpr();
    }
  else
    child(index) = (RelExpr *)NULL;
} // RelExpr::setChild()

// get TableDesc from the expression. It could be directly
// attached to the expression, as in Scan, or could be a
// part of GroupAnalysis, as in cut-opp. For expressions
// which do not have a tableDesc attached to them, like Join
// it would be NULL

TableDesc*
RelExpr::getTableDescForExpr()
{
  TableDesc * tableDesc = NULL;

  if (getOperatorType() == REL_SCAN)
  {
	tableDesc = ((Scan *)this)->getTableDesc();
  }
  else
  {
	if(getGroupAttr()->getGroupAnalysis() &&
	   getGroupAttr()->getGroupAnalysis()->getNodeAnalysis() )
	{
	  TableAnalysis * tableAnalysis = getGroupAttr()->getGroupAnalysis()->getNodeAnalysis()->getTableAnalysis();
	  if (tableAnalysis)
		tableDesc = tableAnalysis->getTableDesc();
	}
  }

  return tableDesc;
}

// This method clears all logical expressions uptill the leaf node
// for multi-join. The methid should be called only before optimization phases
// Reason for that is (1)it is very expensive to synthLogProp and should be avoided
// (2) we are resetting number of joined tables, which should not be done once it is
// set during optimization phases

void RelExpr::clearLogExprForSynthDuringAnalysis()
{
	Int32 numChildren = getArity();

	if (numChildren >= 1) 
	{
		GroupAttributes * grp = getGroupAttr();
		grp->setLogExprForSynthesis(NULL); 
		grp->resetNumJoinedTables(1);
	}

	// clear the log expr for all children
	for (Lng32 i = 0; i < numChildren; i++)
	{
		// only if the child is not a CascadesGroup or NULL
		if (child(i).getPtr() != NULL)
		{
			child(i)->clearLogExprForSynthDuringAnalysis();
		}
	}           
}

void RelExpr::releaseBindingTree(NABoolean memoIsMoribund)
{
  Int32 nc = getArity();

  for (Lng32 i = 0; i < nc; i++)
    {
      if (memoIsMoribund || child(i).getMode() == ExprGroupId::BINDING)
	{
	  // recursively release the bindings of the children's children
	  if (child(i).getPtr() != NULL)
	    child(i)->releaseBindingTree(memoIsMoribund);

	  // release the bindings to the children
	  child(i).convertBindingToStandalone();
	}
    } // for each child

  // indicate that this expression is no longer part of CascadesMemo,
  // (although its groupNext_ and bucketNext_ pointers are still valid)
  groupId_ = INVALID_GROUP_ID;

  if (memoIsMoribund)
    {
      groupAttr_ = NULL;
      groupNext_ = bucketNext_ = NULL;
    }

}

void RelExpr::addSelPredTree(ItemExpr *selpred)
{
  ExprValueId sel = selection_;
  ItemExprTreeAsList(&sel, ITM_AND).insert(selpred);
  selection_ = sel.getPtr();
} // RelExpr::addSelPredTree()

ItemExpr * RelExpr::removeSelPredTree()
{
  ItemExpr * result = selection_;
  selection_ = NULL;
  return result;
} // RelExpr::removeSelPredTree()

//++ MV -
void RelExpr::addUniqueColumnsTree(ItemExpr *uniqueColumnsTree)
{
  ExprValueId t = uniqueColumnsTree_;

  ItemExprTreeAsList(&t, ITM_ITEM_LIST).insert(uniqueColumnsTree);
  uniqueColumnsTree_ = t.getPtr();
}

ItemExpr *RelExpr::removeUniqueColumnsTree()
{
  ItemExpr *result = uniqueColumnsTree_;
  uniqueColumnsTree_ = NULL;

  return result;
}
// MV--

void RelExpr::setGroupAttr(GroupAttributes *gaPtr)
{
  // the new group attributes are now used in one more place
  if (gaPtr != NULL)
    gaPtr->incrementReferenceCount();

  // the old group attributes are now used in one place less than before
  // NOTE: old and new group attribute pointers may be the same
  if (groupAttr_ != NULL)
    groupAttr_->decrementReferenceCount();

  // now assign the new group attribute pointer to the local data member
  groupAttr_ = gaPtr;
}

NABoolean RelExpr::reconcileGroupAttr(GroupAttributes *newGroupAttr)
{
  // make sure the new group attributes have all the information needed
  // and are not inconsistent
  newGroupAttr->reconcile(*groupAttr_);

  // unlink from the current group attributes and adopt the new (compatible)
  // ones
  setGroupAttr(newGroupAttr);

  return FALSE; // no re-optimization for now
}

RelExpr * RelExpr::castToRelExpr()
{
  return this;
}

const RelExpr * RelExpr::castToRelExpr() const
{
  return this;
}

NABoolean RelExpr::isLogical() const   { return TRUE;  }

NABoolean RelExpr::isPhysical() const  { return FALSE; }

NABoolean RelExpr::isCutOp() const     { return FALSE; }

NABoolean RelExpr::isSubtreeOp() const { return FALSE; }

NABoolean RelExpr::isWildcard() const  { return FALSE; }

ItemExpr * RelExpr::selectList()
{
  // RelRoot redefines this virtual method (see BindRelExpr.cpp);
  // Tuple and Union use this standard method.
  RETDesc *rd = getRETDesc();
  if (rd) {
    ValueIdList vids;
    const ColumnDescList &cols = *rd->getColumnList();
    for (CollIndex i = 0; i < cols.entries(); i++)
      vids.insert(cols[i]->getValueId());
    return vids.rebuildExprTree(ITM_ITEM_LIST);
  }
  return NULL;
}

SimpleHashValue RelExpr::hash()
{
  // this method is just defined to have a hash method in ExprNode
  // without referencing class HashValue (which is too complicated
  // for the common code directory)
  return treeHash().getValue();
}

HashValue RelExpr::topHash()
{
  HashValue result = (Int32) getOperatorType();

  // hash the required input and output values from the GroupAttributes
  if (groupAttr_ != NULL)
    result ^= groupAttr_->hash();

  // hash the ValueIdSet of the selection predicates
  result ^= predicates_;

  // the other data members are not significant for the hash function
  CMPASSERT(selection_ == NULL); // this method doesn't work in the parser

  return result;
}

// this method is not virtual, since combining the hash values of the
// top node and its children should be independent of the actual node
HashValue RelExpr::treeHash()
{
  HashValue result = topHash();
  Int32 maxc = getArity();

  for (Lng32 i = 0; i < maxc; i++)
    {
      if (child(i).getMode() == ExprGroupId::MEMOIZED)
	// use the numbers of the input CascadesGroup
	result ^= child(i).getGroupId();
      else
	// call this method recursively for the children
	result ^= child(i)->treeHash();
    }

  return result;
}

NABoolean RelExpr::patternMatch(const RelExpr & other) const
{
  return getOperator().match(other.getOperator());
}

// Checks if the selection preds at this join node are of the
// form FKtable.col1 = UKTable.col1 and FKtable.col1 = UKTable.col1
// and ..., where FKTable.col1 is the FK column that points to
// UKTable.col1.
// The third arguments matchingPreds is an output parameter.
// It is used to send the a list of FKtable.col1 = UKTable.col1
// type predicates back to the caller, so that it can be used
// to adjust the selection preds and equiJoinPreds in the join
// node.
NABoolean Join::hasRIMatchingPredicates(const ValueIdList& fkCols,
					const ValueIdList& ucCols,
					const TableDesc * compRefTabId,
					ValueIdSet & matchingPreds) const
{
  // if the size of the fkCols does not match with ucCols then something is wrong.
  // We also assume below that corresponding cols have identical positions
  // in the two valueidlists and that all entries here are in terms of VEG.
  CMPASSERT(fkCols.entries() == ucCols.entries());
  // There is not possibility of finding a full match
  // if number ofselection preds is smaller than the fkCols.
  // number of selection preds can be larger than fkCols.entries,
  // for example there may be a predicate on the fktable and some
  // other table which is being joined up above. Since the fktable
  // is below this join, this join will have that predicate.
  if ((getSelectionPredicates().entries() < fkCols.entries()))
      return FALSE;

  ValueIdList localFKCols(fkCols);
  ValueIdList localUCCols(ucCols);

  ValueIdSet compRefTabNonUCCols(compRefTabId->getColumnVEGList());
  ValueIdSet localUCColsSet(localUCCols);
  compRefTabNonUCCols -= localUCColsSet;

  NABoolean matchFound = FALSE;
  const ValueIdSet& selPreds = getSelectionPredicates();
  matchingPreds.clear();

  for (ValueId x = selPreds.init();
	    selPreds.next(x);
	    selPreds.advance(x))
  {
    ItemExpr *ie = x.getItemExpr();
    matchFound = FALSE;
    if (ie->getOperatorType() == ITM_VEG_PREDICATE)
    {
      ValueId vegRef = ((VEGPredicate *)ie)->getVEG()->getVEGReference()->getValueId();
      CollIndex fkidx = localFKCols.index(vegRef);
      if ((fkidx != NULL_COLL_INDEX)&&(localUCCols[fkidx] == vegRef))
      {
	localFKCols.removeAt(fkidx);
	localUCCols.removeAt(fkidx);
	matchingPreds.insert(x);
      }
      if (compRefTabNonUCCols.contains(vegRef))
      {
	// return false on a predicate
	// of the form fktable.x = uniquetable.x where x  is a nonkey column.
	  matchingPreds.clear();
	  return FALSE;
      }
    }
    else if ((ie->getOperatorType() == ITM_EQUAL)&&
	    (ie->child(0)->getOperatorType() == ITM_VEG_REFERENCE)&&
	    (ie->child(1)->getOperatorType() == ITM_VEG_REFERENCE))
    {
      ValueId vegRef0 = ((VEGReference *)ie->child(0).getPtr())->getValueId();
      ValueId vegRef1 = ((VEGReference *)ie->child(1).getPtr())->getValueId();
      ValueId ukVid = NULL_COLL_INDEX;
      CollIndex fkidx = localFKCols.index(vegRef0);
      if (fkidx == NULL_COLL_INDEX)
      {
	CollIndex fkidx = localFKCols.index(vegRef1);
	if (fkidx != NULL_COLL_INDEX)
	  ukVid = vegRef0;
      }
      else
	ukVid = vegRef1;
      if ((fkidx != NULL_COLL_INDEX)&&(localUCCols[fkidx] == ukVid))
      {
	localFKCols.removeAt(fkidx);
	localUCCols.removeAt(fkidx);
	matchingPreds.insert(x);
      }
    }
    else
    {
      matchingPreds.clear();
      return FALSE; // not a VEG Pred (revisit for char-varchar)
    }
  }
  if (localFKCols.isEmpty())
    return TRUE ; // all preds have a match with a FK-UC column pair.
  else
  {
    matchingPreds.clear();
    return FALSE;
  }
}


// Special method added to check for ordered cross product called by
// RequiredPhysicalProperty::satisfied() to ensure that if a CQS has
// requested an ordered cross product, then one is being produced.
NABoolean HashJoin::patternMatch(const RelExpr &other) const
{
  if (other.getOperator() == REL_FORCE_ORDERED_CROSS_PRODUCT)
    return ((HashJoin *) this)->isOrderedCrossProduct();



  else
    return RelExpr::patternMatch(other);
}

// Two trees match, if their top nodes and their children are duplicates
// (are the same logical or physical expression). This method provides
// the generic part for determining a match. It can be called by
// redefined virtual methods of derived classes.
NABoolean RelExpr::duplicateMatch(const RelExpr & other) const
{
  if (getOperatorType() != other.getOperatorType())
    return FALSE;

  CMPASSERT(selection_ == NULL); // this method doesn't work in the parser

  if (predicates_ != other.predicates_)
    return FALSE;

  if (rowsetIterator_ != other.rowsetIterator_)
    return FALSE;

  if (tolerateNonFatalError_ != other.tolerateNonFatalError_)
    return FALSE;

  Int32 maxc = getArity();

  // determine whether the children match
  for (Lng32 i = 0; i < maxc; i++)
    {
      // different situations, depending on whether the child
      // and the other node's child is in state MEMOIZED,
      // BINDING, or STANDALONE. See ExprGroupId::operator ==
      // for an explanation for each of the cases

      if (child(i).getMode() == ExprGroupId::MEMOIZED OR
	  other.child(i).getMode() == ExprGroupId::MEMOIZED)
	{
	  // cases marked (x) in ExprGroupId::operator ==
	  // (groups must match)
	  if (NOT (child(i) == other.child(i)))
	    return FALSE;
	}
      else
	{
	  // outside of CascadesMemo or in a CascadesBinding, then
	  //  call this method recursively for the children
	  if (NOT child(i)->duplicateMatch(*other.child(i).getPtr()))
	    return FALSE;
	}
    }

  return TRUE;
}
const CorrName RelExpr::invalid = CorrName("~X~invalid");

RelExpr * RelExpr::copyTopNode(RelExpr *derivedNode,CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) RelExpr(getOperatorType(),
				   NULL,
				   NULL,
				   outHeap);
  else
    result = derivedNode;

  // don't copy pointers to required input/output values, since we don't
  // allow duplicate expressions the new node is likely to get new group
  // attributes

  // copy selection predicates
  result->predicates_ = predicates_;

  // copy pointer to the selection expression tree (Parser only)
  if (selection_ != NULL)
    result->selection_ = selection_->copyTree(outHeap)->castToItemExpr();

  // copy possible index column hints for non-VEG equality predicates
  result->possibleIndexColumns_ = possibleIndexColumns_;

  // -- Triggers
  // Copy the inlining information and access sets.
  result->getInliningInfo().merge(&getInliningInfo());
  result->setAccessSet0(getAccessSet0());
  result->setAccessSet0(getAccessSet0());

  //++MV -

  result->setUniqueColumns(getUniqueColumns());

  if (uniqueColumnsTree_ != NULL)
    result->uniqueColumnsTree_ =
      uniqueColumnsTree_->copyTree(outHeap)->castToItemExpr();
  //--MV -

  // leave any physical properties or CascadesMemo-related data
  // off the returned result ??? (see RelExpr::save)

  result->setBlockStmt(isinBlockStmt());

  result->setFirstNRows(getFirstNRows());

  result->oltOptInfo() = oltOptInfo();
  result->setHint(getHint());

  result->setRowsetIterator(isRowsetIterator());

  result->setTolerateNonFatalError(getTolerateNonFatalError());

  result->setIsExtraHub(isExtraHub());
  result->setMarkedForElimination(markedForElimination());

  result->seenIUD_ = seenIUD_;

  // set the expression's potential
  result->potential_ = potential_;
  
  // copy cascades trace info
  result->parentTaskId_ = parentTaskId_; 
  result->stride_ = stride_; 
  result->birthId_ = birthId_;
  result->memoExprId_ = memoExprId_;
  result->sourceMemoExprId_ = sourceMemoExprId_;
  result->sourceGroupId_ = sourceGroupId_;
  result->costLimit_ = costLimit_;
  result->originalExpr_ = this;

  return result;
}

// this method is not virtual, since combining the copies of the
// top node and its children should be independent of the actual node
RelExpr * RelExpr::copyTree(CollHeap* outHeap)
{
  RelExpr * result = copyTopNode(0,outHeap);
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    result->child(i) = child(i)->copyTree(outHeap);

  return result;
}

// this method is also not virtual, It does same thing as copyTree
// except that it copies the RETDesc and groupAttr pointers too
// this is method is used to get a copy of the original tree before
// inserting it to Cascades.
RelExpr * RelExpr::copyRelExprTree(CollHeap* outHeap)
{
  RelExpr * result = copyTopNode(0,outHeap);
  result->setGroupAttr(new (outHeap) GroupAttributes(*(getGroupAttr())));
  result->setRETDesc(getRETDesc());
  result->getGroupAttr()->setLogExprForSynthesis(result);

  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    result->child(i) = child(i)->copyRelExprTree(outHeap);

  return result;
}

const RelExpr * RelExpr::getOriginalExpr(NABoolean transitive) const
{
  if (originalExpr_ == NULL)
    return this;

  RelExpr *result = originalExpr_;

  while (result->originalExpr_ && transitive)
    result = result->originalExpr_;

  return result;
}

RelExpr * RelExpr::getOriginalExpr(NABoolean transitive)
{
  if (originalExpr_ == NULL)
    return this;

  RelExpr *result = originalExpr_;

  while (result->originalExpr_ && transitive)
    result = result->originalExpr_;

  return result;
}

void RelExpr::setBlockStmtRecursively(NABoolean x)
{
  setBlockStmt(x);
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    child(i)->setBlockStmtRecursively(x);
}

// -----------------------------------------------------------------------
// create or share an optimization goal for a child group
// -----------------------------------------------------------------------
Context * RelExpr::shareContext(Lng32 childIndex,
				const ReqdPhysicalProperty* const reqdPhys,
				const InputPhysicalProperty* const inputPhys,
				CostLimit* costLimit,
				Context * parentContext,
				const EstLogPropSharedPtr& inputLogProp,
				RelExpr *explicitlyRequiredShape) const
{
  // no need to do the job if costLimit id already negative
  if ( costLimit AND
       CURRSTMT_OPTDEFAULTS->OPHpruneWhenCLExceeded() AND
       costLimit->getValue(reqdPhys) < 0 )
    return NULL;

  const ReqdPhysicalProperty* searchForRPP;

  // if the required physical properties are empty, don't use them
  if (reqdPhys != NULL AND reqdPhys->isEmpty())
    searchForRPP = NULL;
  else
    searchForRPP = reqdPhys;

  // handle force plan directives: if the parent node must match a
  // certain tree, make sure the child node gets the appropriate
  // requirement to match a child node of the mustMatch pattern
  RelExpr *childMustMatch = explicitlyRequiredShape;

  if (parentContext->getReqdPhysicalProperty() != NULL AND
      parentContext->getReqdPhysicalProperty()->getMustMatch() != NULL AND
      explicitlyRequiredShape == NULL)
    {
      const RelExpr *parentMustMatch =
	parentContext->getReqdPhysicalProperty()->getMustMatch();

      // Reuse the parent's pattern if this node is a map value ids
      // node and the required pattern isn't. This is because a map value
      // ids node, PACK node and UNPACK node is essentially a no-op and
      // does not need to be specified in CONTROL QUERY SHAPE.
      // Sorry for putting this DBI code into
      // places where particular operator types shouldn't be known.
      // It's the summer of 1997 and we have a deadline for FCS.
      // Its (almost) summer of 2003 and I am adding the same thingy
      // for FIRST_N operator.
      if (((getOperatorType() == REL_MAP_VALUEIDS) &&
	   (parentMustMatch->getOperatorType() != REL_MAP_VALUEIDS)) ||
	   ((getOperatorType() == REL_PACK) AND
		(parentMustMatch->getOperatorType() != REL_PACK)) ||
	   ((getOperatorType() == REL_UNPACKROWS) AND
		(parentMustMatch->getOperatorType() != REL_UNPACKROWS)) ||
	  ((getOperatorType() == REL_FIRST_N) AND
	   (parentMustMatch->getOperatorType() != REL_FIRST_N)) ||
	  (CURRSTMT_OPTDEFAULTS->ignoreExchangesInCQS() AND
	   (getOperatorType() == REL_EXCHANGE) AND
	   (parentMustMatch->getOperatorType() != REL_FORCE_EXCHANGE)) ||
	  (CURRSTMT_OPTDEFAULTS->ignoreSortsInCQS() AND
	   (getOperatorType() == REL_SORT) AND
	   (parentMustMatch->getOperatorType() != REL_SORT)))
	{
	  childMustMatch = (RelExpr *) parentMustMatch;
	}
      else
	{
	  // If the "must match" pattern specifies something other than
	  // a cut op for child "childIndex" then this is our new "must match".
	  if (childIndex < parentMustMatch->getArity() AND
	      NOT parentMustMatch->child(childIndex)->isCutOp())
	    childMustMatch = parentMustMatch->child(childIndex);
	}
    }

  if (childMustMatch != NULL OR
      searchForRPP AND searchForRPP->getMustMatch() != NULL)
    {
      // we have to change the "must match" attribute of searchForRPP
      // add the "mustMatch" requirement
      if (searchForRPP != NULL)
	{
	  searchForRPP = new (CmpCommon::statementHeap())
	                     ReqdPhysicalProperty(*searchForRPP,
				                  childMustMatch);
	}
      else
	searchForRPP = new (CmpCommon::statementHeap())
	                   ReqdPhysicalProperty(childMustMatch);
    }

  return (*CURRSTMT_OPTGLOBALS->memo)[child(childIndex).getGroupId()]->shareContext(searchForRPP,
                                                               inputPhys,
							       costLimit,
							       parentContext,
							       inputLogProp);
} // RelExpr::shareContext()

ULng32 RelExpr::getDefault(DefaultConstants id)
{
  return ActiveSchemaDB()->getDefaults().getAsULong(id);
}

void RelExpr::addLocalExpr(LIST(ExprNode *) &xlist,
			   LIST(NAString) &llist) const
{
  if (selection_ != NULL OR
      NOT predicates_.isEmpty())
    {
      if (predicates_.isEmpty())
	xlist.insert(selection_);
      else
	xlist.insert(predicates_.rebuildExprTree());
      llist.insert("selection_predicates");
    }
  if(NOT uniqueColumns_.isEmpty())
  {
    xlist.insert(uniqueColumns_.rebuildExprTree());
    llist.insert("uniqueColumns_");
  }
}

//QSTUFF
// we must pushdown the outputs of a genericupdate root to its
// descendants to ensure that only those required output values are
// tested against indexes when selecting an index for a stream scan
// followed by an embedded update. Since we may allow for unions and
// for inner updates we just follow the isEmbeddedUpdate() thread once
// we reach a generic update root.

void RelExpr::pushDownGenericUpdateRootOutputs( const ValueIdSet &outputs)
{

  ValueIdSet rootOutputs =
    getGroupAttr()->isGenericUpdateRoot() ?
    getGroupAttr()->getCharacteristicOutputs() : outputs;


  for (Int32 i=0; i < getArity(); i++) {
    if (child(i)->castToRelExpr()->getGroupAttr()->isEmbeddedUpdateOrDelete()){
      child(i)->castToRelExpr()->
        pushDownGenericUpdateRootOutputs(rootOutputs);
    }
  }

  if (NOT rootOutputs.isEmpty()){
    getGroupAttr()->setGenericUpdateRootOutputs(rootOutputs);
  }
}

//QSTUFF

void RelExpr::needSortedNRows(NABoolean val)
{ 
  // The operators listed below can create OR propogate a GET_N
  // request. Other operatots will turn a GET_N request into GET_ALL
  // There are a few exceptions like right side of NJ for semi join etc.
  // but these are not relevant for FirstN sort
  // This method should only in the generator since we are using 
  // physical node types.
  OperatorTypeEnum operatorType = getOperatorType();
  if ((operatorType != REL_FIRST_N) &&
      (operatorType != REL_EXCHANGE) &&
      (operatorType != REL_MERGE_UNION) &&
      (operatorType != REL_PROBE_CACHE) &&
      (operatorType != REL_ROOT) && 
      (operatorType != REL_LEFT_NESTED_JOIN) &&
      (operatorType != REL_LEFT_TSJ) &&
      (operatorType != REL_MAP_VALUEIDS))
    return ;
      
  if ((operatorType == REL_LEFT_NESTED_JOIN) || 
      (operatorType == REL_LEFT_TSJ)) {
    // left side of left tsj propagates a GET_N request if afterPred is empty.
    if (getSelectionPred().isEmpty())
      child(0)->castToRelExpr()->needSortedNRows(val);

    return ;
  }
 
  for (Int32 i=0; i < getArity(); i++) {
    if (child(i))
      child(i)->castToRelExpr()->needSortedNRows(val);
  }
}

// -----------------------------------------------------------------------
// computeValuesReqdForPredicates()
//
// There has been some problems with this function (as to how it should
// behave). The issue has been whether we should allow an operator to
// have the boolean value of a predicate as its output. That is to say,
// whether (SCAN T1), for example, could evaluate a predicate such as
// (T1.a > 3) and output a value of true or false to its parent.
//
// In most cases, this wouldn't be an issue since the predicate is used
// to filter out all the non-qualified rows. However, such is not the
// case when a CASE statement is involved. Part of the CASE statement,
// (e.g. the WHEN clause) can be evaluated at the SCAN, while the rest
// of the statement could reference some other tables and therefore must
// be evaluated at an ancestor node of the tree.
//
// A complete example is SELECT CASE WHEN T1.A > 3 THEN T2.A ELSE 0 END
// FROM T1 JOIN T2 ON T1.C = T2.C. In this case, if we allow a boolean
// value to be our output, the (T1.A > 3) could be evaluated at SCAN T1,
// and the CASE statement itself at the JOIN. The alternative would be
// for SCAN T1 to output T1.A and the CASE statement evaluated wholly at
// the JOIN.
//
// Now, how do all these relate to this function? The purpose of this
// function is to turn a predicate into values required to evaluate the
// predicate. Thus, the question is: should we allow the boolean value
// of (T1.A > 3) be the value required to evaluate the predicate (T1.A >
// 3). Or, should the values be T1.A and 3 instead? More generally,
// should we go for the leaf values of a non-VEG predicate (there is a
// separate story for VEG predicates, see later) or just the bool value
// of that predicate?
//
// This function has been implemented to gather the leaf values. However,
// there is no reason why we could
// not just require the boolean value. The logic of predicate pushdown
// mandates that if the child of the operator is unable to produce that
// boolean value, it will figure out what sub-expressions it could produce
// in its outputs in order for the boolean value to be evaluated at its
// parent.
//
// On the other hand, since this function has not been changed for quite
// a while, we are worried the change might trigger problematic spots in
// other places which rely on this function behaving the way it has been.
// Through extensive testing, we didn't seem to identify any problems and
// therefore, we decided to commit this fix.
//
// Now for VEGPred's. A VEGPred is considered "evaluable" at an operator
// if any *one* of its VEG members is "evaluable". For example, VEGPred(
// VEG{T1.a,T2.a}) in the query SELECT T2.B FROM (T1 JOIN T2 ON T1.A =
// T2.A) is "evaluable" at SCAN T1 and will be pushed down. Clearly, only
// evaluating the predicate there is not enough. We have to keep the
// VEGPred at the JOIN as well. The logic in Join::pushdownCoveredExpr()
// correctly handle that now. That is, it keeps the predicate even if
// it has been pushed down. However, doing so means that in the example
// given, SCAN T1 has to return T1.A as an output rather than just the
// boolean value of VEGPred(VEG{T1.A,T2.A}). That boolean value is sort
// of only local to SCAN T1. This function, therefore, declares that the
// value required to evaluate a VEGPred is not the boolean value of the
// VEGPred itself but the VEGRef of its VEG. In our example, the required
// value is VEGRef(VEG{T1.A,T2.A}). The semantics of this is that SCAN T1
// is asked to return as an output one of the VEG members available to
// it. The pre-code generator will thus change this VEGRef into T1.A.
//
//                                                              8/14/1998
//
// -----------------------------------------------------------------------
void RelExpr::computeValuesReqdForPredicates(const ValueIdSet& setOfExpr,
					     ValueIdSet& reqdValues,
                                             NABoolean addInstNull)
{
  for (ValueId exprId = setOfExpr.init();
       setOfExpr.next(exprId);
       setOfExpr.advance(exprId))
  {
    if (exprId.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
    {
      VEG * vegPtr = ((VEGPredicate *)(exprId.getItemExpr()))->getVEG();
      reqdValues += vegPtr->getVEGReference()->getValueId();

      // If the VEG for this VEGPredicate contains a member that is
      // another VEGReference, add it to reqdValues in order to ensure
      // that it gets retrieved.
      //
      for (ValueId x = vegPtr->getAllValues().init();
           vegPtr->getAllValues().next(x);
           vegPtr->getAllValues().advance(x))
      {
        OperatorTypeEnum optype = x.getItemExpr()->getOperatorType();
        if ( optype == ITM_VEG_REFERENCE )
             // **********************************************************
             // Note: this "if" used to have the following cases as well.
             //       We feel that they might not be necessary any more.
             // || optype == ITM_INSTANTIATE_NULL ||
             // optype == ITM_UNPACKCOL )
             // **********************************************************
          reqdValues += x;
        else if ( addInstNull && optype == ITM_INSTANTIATE_NULL ) 
          { // part of fix to soln 10-090618-2434: a full outer join
            //   select ... from t1 inner join t2 on ... 
            //   full outer join t3 on ... where t2.RGN = 'EMEA'
            // whose selection predicate "t2.RGN = <constant>" must have
            // its null-instantiated "t.RGN" column added to reqdValues.
            reqdValues += x;
          }
      } // end inner for
    } // endif is a VEGPredicate
    else
    {
      // Not a VEGPred (either a "normal" pred or a "real" value). In
      // any case, just add the value to the required values set. (For
      // a "normal" pred, it means the boolean value for the predicate
      // is required.
      //
      reqdValues += exprId;
    }
  } // end outer for
} // computeValuesReqdForPredicates()

void RelExpr::computeValuesReqdForOutput(const ValueIdSet& setOfExpr,
                                         const ValueIdSet& newExternalInputs,
                                         ValueIdSet& reqdValues)
{
  // if VEGPreds are in the output, get the underlying VEGRefs
  computeValuesReqdForPredicates(setOfExpr, reqdValues);

  const GroupAttributes emptyGA;
  for (ValueId exprId = setOfExpr.init();
       setOfExpr.next(exprId);
       setOfExpr.advance(exprId))
  {
    if  ((exprId.getType().getTypeQualifier() == NA_CHARACTER_TYPE) &&
         (exprId.getType().getNominalSize() > CONST_32K))
    {
        exprId.getItemExpr()->getLeafValuesForCoverTest(reqdValues, 
                                                        emptyGA, 
                                                        newExternalInputs);
    }
  }
}

// -----------------------------------------------------------------------
// RelExpr::pushdownCoveredExpr()
// -----------------------------------------------------------------------
void RelExpr::pushdownCoveredExpr(const ValueIdSet & outputExpr,
                                  const ValueIdSet & newExternalInputs,
                                  ValueIdSet & predicatesOnParent,
				  const ValueIdSet * setOfValuesReqdByParent,
				  Lng32 childIndex
		                 )
{
  ValueIdSet exprToEvalOnParent, outputSet, extraHubNonEssOutputs;
  Int32 firstChild, lastChild; // loop bounds
  Int32 iter;                  // loop index variable
  NABoolean optimizeOutputs;


  if (getArity() == 0 ) return; // we don't do anything for leaf nodes..

  if ((getOperator().match(REL_ANY_TSJ) ) ||
      (getOperator().match(REL_ANY_GEN_UPDATE) ) )
    optimizeOutputs = FALSE;
  else
    optimizeOutputs = TRUE;


  if (getOperator().match(REL_ANY_JOIN) &&
      isExtraHub())
    extraHubNonEssOutputs = ((Join *)this)->getExtraHubNonEssentialOutputs();
 
    
  // -----------------------------------------------------------------
  // Should the pushdown be attempted on a specific child?
  // -----------------------------------------------------------------
  if ( (childIndex >= 0) AND (childIndex < getArity()) )
    {                             // yes, a child index is given
      firstChild = (Int32)childIndex;
      lastChild = firstChild + 1;
    }
  else                            // no, perform pushdown on all
    {
      firstChild = 0;
      lastChild  = getArity();
    }

  // ---------------------------------------------------------------------
  // Examine the set of values required by the parent. Replace each
  // VEGPredicate with a VEGReferences for its VEG; if its VEG
  // contains other VEGReferences, add them to exprToEvalOnParent.
  // ---------------------------------------------------------------------
  if (setOfValuesReqdByParent)
    computeValuesReqdForPredicates(*setOfValuesReqdByParent,
                                 exprToEvalOnParent);

  computeValuesReqdForOutput(outputExpr,newExternalInputs,outputSet);


  // ---------------------------------------------------------------------
  // Are there any predicates that can be pushed down?
  // ---------------------------------------------------------------------
  if ( (getArity() > 0) AND (NOT predicatesOnParent.isEmpty()) )
    {
      // -----------------------------------------------------------------
      // 1) Figure out which predicates could be push to which child.
      //    Try to give all predicates to all children.
      // 2) Modify predOnParent to be those predicates that no could
      //    could take.
      // 3) Add to the selectionPred() of each child those predicates
      //    it could take (if it is not a cut operator)
      // 4) Add to exprToEvalOnParent the predicates that could not
      //    be push down to any child (predOnParent)
      // 5) Recompute the input and outputs for each child given this
      //    set of exprOnParent.
      // -----------------------------------------------------------------
      // Allocate an array to contain the ValueIds of external inputs
      // that are referenced in the given expressions.
      // -----------------------------------------------------------------
      ValueIdSet referencedInputs[MAX_REL_ARITY];
      // -----------------------------------------------------------------
      // Allocate an array to contain the ValueIds of the roots of
      // sub-expressions that are covered by
      // a) the Group Attributes of a child and
      // b) the new external inputs.
      // Note that the containing expression is not covered for each
      // such sub-expression.
      // -----------------------------------------------------------------
      ValueIdSet coveredSubExprNotUsed[MAX_REL_ARITY];
      // -----------------------------------------------------------------
      // Allocate an array to contain the ValueIds of predicates that
      // can be pushed down to a specific child.
      // -----------------------------------------------------------------
      ValueIdSet predPushSet[MAX_REL_ARITY];
      // -----------------------------------------------------------------
      // Allocate an array to contain the ValueIds of predicates from
      // non-VEG equality predicates that might be useful hints for
      // index selection.
      // -----------------------------------------------------------------
      ValueIdSet possibleIndexColumnsPushSet[MAX_REL_ARITY];
      // -----------------------------------------------------------------
      // Check which predicate factors are fully covered by a certain
      // child. Gather their ValueIds in predPushSet.
      // -----------------------------------------------------------------
      const ValueIdSet emptySet;
      // -----------------------------------------------------------------
      // Join predicates can be pushed below a GU root as the comment a
      // few lines below does applies only to selection predicates
      // and not join predicates. The comment below indicates that in
      // some cases we do not wish to push a user provided predicate on
      // select below the GU root. These user provided predicates are
      // stored as selection predicates.
      // For MTS deletes, an anti-semi-join is used to glue the
      // inlined tree. For such joins all predicates that are pulled
      // are stored as join predicates. The change below facilitates
      // a push down of those predicates. The firstChild condition below
      // ensures that we are considering join predicates here (see
      // Join::pushDownCoveredExpr)
      // -----------------------------------------------------------------
      NABoolean pushPredicateBelowGURoot = FALSE;

      if ((getGroupAttr()->isGenericUpdateRoot() AND
	  getOperator() == REL_ANTI_SEMITSJ AND
	  firstChild == 1 ) OR
	  (NOT (getGroupAttr()->isGenericUpdateRoot())))
      {
	pushPredicateBelowGURoot = TRUE;
      }

      for (iter = firstChild; iter < lastChild; iter++)
	{
        if (NOT child(iter).getPtr()->isCutOp()){

          // QSTUFF
          // we don't push predicates beyond the root of a generic
          // update tree. This is done by pretending that those
          // predicates are not covered by any child. This is
          // required to allows us to distinguish between the
          // following two types of expressions:
          // select * from (delete from x) y where y.x > 3;
          // select * from (delete from x where x.x > 3) y;

          if (pushPredicateBelowGURoot ) {
            // QSTUFF
            child(iter).getGroupAttr()->coverTest(predicatesOnParent,
              newExternalInputs,
              predPushSet[iter],
              referencedInputs[iter],
              &coveredSubExprNotUsed[iter]);
            // QSTUFF            
            ValueIdSet dummyReferencedInputs;
            child(iter).getGroupAttr()->coverTest(possibleIndexColumns_,
              newExternalInputs,
              possibleIndexColumnsPushSet[iter],
              dummyReferencedInputs,
              &possibleIndexColumnsPushSet[iter]);            
          }
          // QSTUFF
        }
          else
            // ----------------------------------------------------------
            // If this is a cutop these predicates were already pushed
            // down to the child during predicate pushdown. Compute
            // which predicates were pushable so that we can remove them
            // from predOnParent and avoid creating a new group that will
            // later be merged
            // ----------------------------------------------------------

            // QSTUFF
            // for more explanation please see comment above

          if ( pushPredicateBelowGURoot ) {
              // QSTUFF
              child(iter).getGroupAttr()->coverTest(predicatesOnParent,
                emptySet,
                predPushSet[iter],
                referencedInputs[iter],
                &coveredSubExprNotUsed[iter]);
              // QSTUFF
            }
            // QSTUFF
      } // for loop to perform coverTest()
      // -----------------------------------------------------------------
      // From the original set of predicates, delete all those predicates
      // that will be pushed down. The remaining predicates will be
      // evaluated on the parent (this node).
      // -----------------------------------------------------------------
      for (iter = firstChild; iter < lastChild; iter++)
		predicatesOnParent -= predPushSet[iter];

      // -----------------------------------------------------------------
      // Add the predicates that could not be pushed to any child to the
      // set of expressions to evaluate on the parent.
      // -----------------------------------------------------------------
      computeValuesReqdForPredicates(predicatesOnParent,
                                     exprToEvalOnParent);

      // -----------------------------------------------------------------
      // Check for equality predicates that could not be pushed down.
      // This may happen if we have an equality predicate that was not
      // transformed into a VEG predicate. 
      //
      // If there are column references in the equality predicate, it
      // may prove useful to tell the leaf nodes about it, as the equality
      // predicate might be pushed down later during a Join to TSJ 
      // transformation. Telling the leaf nodes about it might allow the
      // use of an index in this case (see Scan::addIndexInfo; this is
      // called once at the beginning of optimization, before any Join
      // to TSJ transformations have been attempted).
      // -----------------------------------------------------------------
      if (CmpCommon::getDefault(COMP_BOOL_194) == DF_ON)
        {
          ValueId pred;
          for (pred = predicatesOnParent.init();
               predicatesOnParent.next(pred);
               predicatesOnParent.advance(pred))
            {
              ItemExpr * ie = pred.getItemExpr();
              if (ie->getOperatorType() == ITM_EQUAL) // non-VEG, equality predicate
                {
                  for (iter = firstChild; iter < lastChild; iter++)
                    {
                      for (CollIndex i = 0; i < ie->getArity(); i++)
                        {
                          // If the child is covered, it might contain a column reference
                          // that is useful for index selection. Push that down.
                          ItemExpr * ieChildi = ie->child(i);
                          if (coveredSubExprNotUsed[iter].contains(ieChildi->getValueId()))
                            {
                              possibleIndexColumnsPushSet[iter] += ieChildi->getValueId();
                            }
                        }
                    } 
                }     
            }
        }

      // -----------------------------------------------------------------
      // Perform predicate pushdown
      // -----------------------------------------------------------------
      for (iter = firstChild; iter < lastChild; iter++)
	{
          if (NOT child(iter).getPtr()->isCutOp())
            {
	      // ---------------------------------------------------------
	      // Reassign predicate factors to the appropriate children
	      // ---------------------------------------------------------
	      child(iter).getPtr()->selectionPred().insert(predPushSet[iter]);
	      // ---------------------------------------------------------
	      // Add the input values that are referenced by the predicates
	      // that were pushed down in the above step, to the Group
	      // Attributes of the child.
              // We need to call coverTest again to figure out which inputs
              // are needed for the predicates that will be pushdown.
	      // ---------------------------------------------------------
              ValueIdSet inputsNeededByPredicates;
  	      child(iter).getGroupAttr()->coverTest(predPushSet[iter],
  						    referencedInputs[iter],
  						    predPushSet[iter],
  						    inputsNeededByPredicates,
  						    &coveredSubExprNotUsed[iter]);
	      child(iter).getPtr()->getGroupAttr()->addCharacteristicInputs
	                                            (inputsNeededByPredicates);
	      ValueIdSet essChildOutputs;
	      child(iter).getPtr()->getEssentialOutputsFromChildren
	       (essChildOutputs);
              // ----------------------------------------------------------
              // Have the child compute what output it can provide for
              // the expressions that remain on the parent
              // ----------------------------------------------------------

	     // TBD: Fix the hack described in 
	     // GroupAttributes::resolveCharacteristicOutputs()
	     if(iter==1 AND getOperator().match(REL_ANY_LEFT_JOIN))
	          child(iter).getPtr()->getGroupAttr()->computeCharacteristicIO
	                                              (newExternalInputs,
						       exprToEvalOnParent,
						       outputSet,
						       essChildOutputs,
						       &(getSelectionPred()),
						       TRUE,
						       optimizeOutputs,
                                                       &extraHubNonEssOutputs
						       );
	     else
		 child(iter).getPtr()->getGroupAttr()->computeCharacteristicIO
	                                              (newExternalInputs,
						       exprToEvalOnParent,
						       outputSet,
						       essChildOutputs,
						       NULL,
						       FALSE,
						       optimizeOutputs,
                                                       &extraHubNonEssOutputs
                                                       );

            // pass down index column hints for non-VEG equality predicates
            child(iter).getPtr()->addToPossibleIndexColumns(possibleIndexColumnsPushSet[iter]);
            };
	} // for loop to pushdown predicates
    } // endif (NOT predicatesOnParent.isEmpty())
  else
    {
      // ---------------------------------------------------------------------
      // Compute the characteristic inputs and outputs of each child
      // ---------------------------------------------------------------------
      for (iter = firstChild; iter < lastChild; iter++)
	{
	  // -----------------------------------------------------------------
	  // Ignore CutOps because they exist simply to facilitate
	  // pattern matching. Their Group Attributes are actually those
	  // of the CascadesGroup. So, don't mess with them!
	  // -----------------------------------------------------------------
	  if (NOT child(iter).getPtr()->isCutOp())
	  {
	    ValueIdSet essChildOutputs;
	    child(iter).getPtr()->getEssentialOutputsFromChildren
	       (essChildOutputs);

	    // TBD: Fix the hack described in 
	    // GroupAttributes::resolveCharacteristicOutputs()
	    child(iter).getPtr()->getGroupAttr()->computeCharacteristicIO
	                                            (newExternalInputs,
                                                     exprToEvalOnParent,
						     outputSet,
						     essChildOutputs,
						     NULL,
						     FALSE,
						     optimizeOutputs,
                                                     &extraHubNonEssOutputs
						     );
	  }
	} // for loop to compute characteristic inputs and outputs
    } // endelse predicatesOnParent is empty

} // RelExpr::pushdownCoveredExpr()

// -----------------------------------------------------------------------
// A virtual method for computing output values that an operator can
// produce potentially.
// -----------------------------------------------------------------------
void RelExpr::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  Int32 nc = getArity();
  // For operators that are not leaves, clear the potential outputs
  // and rebuild them.
  if (nc > 0)
    for (Lng32 i = 0; i < nc; i++)
      outputValues += child(i).getGroupAttr()->getCharacteristicOutputs();
  else
    outputValues += getGroupAttr()->getCharacteristicOutputs();

} // RelExpr::getPotentialOutputValues()

void RelExpr::getPotentialOutputValuesAsVEGs(ValueIdSet& outputs) const
{
  getPotentialOutputValues(outputs);
}


// -----------------------------------------------------------------------
// primeGroupAttributes()
// Initialize the Characteristic Inputs And Outputs of this operator.
// -----------------------------------------------------------------------
void RelExpr::primeGroupAttributes()
{
  // Ignore CutOps because they exist simply to facilitate
  // pattern matching. Their Group Attributes are actually those
  // of the CascadesGroup. So, don't mess with them.
  if (isCutOp())
    return;

  // The method sets the characteristic outputs of a node to its
  // potential outputs and sets the required input to the values
  // it needs. It does this by calling two virtual functions
  // on RelExpr.

  ValueIdSet outputValues;
  getPotentialOutputValues(outputValues);
  getGroupAttr()->setCharacteristicOutputs(outputValues);

  recomputeOuterReferences();
} // RelExpr::primeGroupAttributes()

// -----------------------------------------------------------------------
// allocateAndPrimeGroupAttributes()
// This method is for allocating new Group Attributes for the children
// of this operator that were introduced in the dataflow by a rule-
// based transformation. Each new child, or set of children, intervene
// between this operator and another operator that was originally a
// direct child of the latter. The Group Attributes of each newly
// introduced child are recursively primed with the Characteristic
// Inputs and Outputs of the operators of which it is the parent.
// -----------------------------------------------------------------------
void RelExpr::allocateAndPrimeGroupAttributes()
{
  Int32 nc = getArity();

  for (Lng32 i = 0; i < nc; i++)
    {
      CMPASSERT(child(i).getMode() == ExprGroupId::STANDALONE);
      // Terminate the recursive descent upon reaching a CutOp.
      // Ignore CutOps because they exist simply to facilitate
      // pattern matching. Their Group Attributes are actually
      // those for the CascadesGroup that they belong to and
      // must not change.
      if (NOT child(i)->isCutOp())
	{
	  if (child(i).getGroupAttr() == NULL)
	    {
	      // A CutOp must have Group Attributes.
	      child(i)->setGroupAttr(new (CmpCommon::statementHeap())
				         GroupAttributes());
	    }
	  // Assign my Characteristic Inputs to my child.
	  // This is done in order to ensure that they are propagated
	  // recursively to all my children who are not CutOps.
	  child(i).getPtr()->getGroupAttr()
             ->addCharacteristicInputs
                 (getGroupAttr()->getCharacteristicInputs());
	  // Recompute the potential inputs/outputs for each real child
	  // recursively.
          // Terminate the recursive descent upon encountering an
	  // operator whose arity == 0
	  child(i).getPtr()->allocateAndPrimeGroupAttributes();
	  // Prime the Group Attributes of the child.
	  // The following call primes the child's Characteristic Outputs.
	  // It ensures that the inputs are minimal and outputs are maximal.
	  child(i).getPtr()->primeGroupAttributes();
      // Now compute the GroupAnalysis fields
      child(i).getPtr()->primeGroupAnalysis();
	} // endif child is not a CutOp

    } // for loop

} // RelExpr::allocateAndPrimeGroupAttributes()

void RelExpr::getEssentialOutputsFromChildren(ValueIdSet & essOutputs)
{
  Int32 nc = getArity();
  for (Lng32 i = 0; i < nc; i++)
  {
    essOutputs += child(i).getGroupAttr()->
      getEssentialCharacteristicOutputs();
  }
}

void RelExpr::fixEssentialCharacteristicOutputs()
{
  ValueIdSet essChildOutputs,nonEssOutputs;
  getEssentialOutputsFromChildren(essChildOutputs);
  getGroupAttr()->getNonEssentialCharacteristicOutputs(nonEssOutputs);
  nonEssOutputs.intersectSet(essChildOutputs);
  getGroupAttr()->addEssentialCharacteristicOutputs(nonEssOutputs);
}

// do some analysis on the initial plan
// this is called at the end of the analysis phase
void RelExpr::analyzeInitialPlan()
{
  Int32 nc = getArity();

  for (Lng32 i = 0; i < nc; i++)
  {
    child(i)->analyzeInitialPlan();
  }

}

double RelExpr::calculateNoOfLogPlans(Lng32& numOfMergedExprs)
{
  double result = 1;
  Int32 nc = getArity();
  CascadesGroup* group;

  for (Lng32 i = 0; i < nc; i++)
  {
    if (getGroupId() == child(i).getGroupId())
    {
      // This is  a recursive reference of an expression to itself
      // due to a group merge. We cannot call this method on the
      // child, as we would end up calling this method on ourselves
      // again! So, we skip the recursive call on our child and
      // instead return an indication to our caller
      // (CascadesGroup::calculateNoOfLogPlans) that we encountered
      // a merged expression. Our caller will then know what to do
      // to calculate the correct number of logical expressions.
      numOfMergedExprs++;
    }
    else
    {
      group = (*CURRSTMT_OPTGLOBALS->memo)[child(i).getGroupId()];
      result *= group->calculateNoOfLogPlans();
    }
  } // for each child

  return result;
}

// This function is called before any optimization starts
// i.e. applied to the normalizer output (reorderJoinTree OK)
double RelExpr::calculateSubTreeComplexity
                 (NABoolean& enableJoinToTSJRuleOnPass1)
{

  double result = 0;
  Int32 freeLeaves = 1; // # of subtree legs that can permutate
  RelExpr* expr = this;

  while (expr)
  {
    if (expr->getGroupAttr()->isEmbeddedUpdateOrDelete() OR
	expr->getGroupAttr()->isStream())
    {
      enableJoinToTSJRuleOnPass1 = TRUE;
    }
    Int32 nc = expr->getArity();

    // The multi-join case
    if (expr->getOperatorType() == REL_MULTI_JOIN)
    {
      for (Int32 i = 0; i < nc; i++)
      {
        CascadesGroup* groupi = (*CURRSTMT_OPTGLOBALS->memo)[expr->child(i).getGroupId()];

	RelExpr * expri = groupi->getFirstLogExpr();

        result += expri->
          calculateSubTreeComplexity(enableJoinToTSJRuleOnPass1);

      }
      freeLeaves = nc;
      // end the while loop
      expr = NULL;
    }
    // Not multi-join, and not leaf
    else if (nc > 0)
    {
      if (nc == 1)
      {
        // no permutation can take place across groupbys
        if (expr->getOperator().match(REL_ANY_GROUP))
        {
          if (freeLeaves > 1)
          {
            // compute the last permuatation set contribution
            // to the complexity and start a new one
            result += freeLeaves * pow(2,freeLeaves-1);
            freeLeaves = 1;  // start again
          }
        }
      }
      if (nc == 2)
      {
        double child1Complexity;
        CascadesGroup* group1 = (*CURRSTMT_OPTGLOBALS->memo)[expr->child(1).getGroupId()];
        if (group1->getGroupAttr()->getNumBaseTables() > 1)
        {
          // Only one log expr exist in the group at this point
          RelExpr * expr1 = group1->getFirstLogExpr();
          child1Complexity =
            expr1->calculateSubTreeComplexity(enableJoinToTSJRuleOnPass1);

		  // adding this comp_bool guard in case this fix causes regressions
		  // and we need to disable this fix. Should be taken out in a subsequent
		  // release. (say 2.2)
		 if (CmpCommon::getDefault(COMP_BOOL_123) == DF_OFF)
		 {
			  // The factor 2 accounts for the fact that the join could be a
			  // join or a TSJ i.e. two possible logical choices.
			  if (expr->getOperator().match(REL_ANY_NON_TSJ_JOIN))
				child1Complexity = 2*child1Complexity ;
		 }

	  // add the right child subtree contribution to complexity
          result += child1Complexity;
        }
        // only REL_ANY_NON_TSJ_JOINs can permutate
        if (expr->getOperator().match(REL_ANY_NON_TSJ_JOIN))
          freeLeaves++;  // still in same permutation set
        else
        {
          // compute the last permuatation set contribution
          // to the complexity and start a new one
          result += freeLeaves * pow(2,freeLeaves-1);
          freeLeaves = 1;  // start again
        }
      }
      // we do not handle VPJoin yet (nc==3)
      CascadesGroup* group0 = (*CURRSTMT_OPTGLOBALS->memo)[expr->child(0).getGroupId()];
      // Only one log expr exist in the group at this point
      expr = group0->getFirstLogExpr();
    }
    // leaf operators
    else
      expr = NULL;
  }
  // add last permutation set contribution
  result += freeLeaves * pow(2,freeLeaves-1);
  return result;

}

// calculate a query's MJ complexity,
// shoud be called after MJ rewrite
double RelExpr::calculateQueryMJComplexity(double &n,double &n2,double &n3,double &n4)
{
  double result = 0;
  Int32 nc = getArity();
  Int32 freeLeaves = nc; // # of subtree legs that can permutate
 
  RelExpr * expr = this;

  if (getOperatorType() == REL_MULTI_JOIN)
  {
    for (Int32 i = 0; i < nc; i++)
    {
      RelExpr * expri = expr->child(i);

      NABoolean childIsFullOuterJoinOrTSJ =
        child(i)->getGroupAnalysis()->getNodeAnalysis()->
          getJBBC()->isFullOuterJoinOrTSJJBBC();
          

      if (childIsFullOuterJoinOrTSJ)
      {
        NABoolean childIsOuterMost =
          !(child(i)->getGroupAnalysis()->getNodeAnalysis()->
              getJBBC()->getOriginalParentJoin());
        
        if(childIsOuterMost)
          freeLeaves--;
      }

      result += expri->
        calculateQueryMJComplexity(n, n2, n3, n4);
    }
    //only do this for multijoins since only the children
    //of the multijoin will be permuted.
    //Note: This assumes the query tree to be the multijoinized
    //tree produced after multijoin rewrite in the Analyzer
    n += freeLeaves;
    n2 += pow(freeLeaves,2);
    n3 += pow(freeLeaves,3);
    n4 += pow(freeLeaves,4);
    result += freeLeaves * pow(2,freeLeaves-1);
  }
  else if(nc > 0)
  {
    if (nc == 1)
    {
      RelExpr * expr0 = expr->child(0);

      result += expr0->
        calculateQueryMJComplexity(n, n2, n3, n4);      
    }
    else if (nc == 2)
    {
      // only for joins, not for union
      // these will only be TSJ or Full Outer Joins
      // other joins become part of JBB
      if (expr->getOperator().match(REL_ANY_JOIN))
      {
        RelExpr * expr0 = expr->child(0);
        result += expr0->calculateQueryMJComplexity(n, n2, n3, n4);

        RelExpr * expr1 = expr->child(1);
        result += expr1->calculateQueryMJComplexity(n, n2, n3, n4);
      }
    }
  }

  return result;
}


// -----------------------------------------------------------------------
//  the following method is used to created a list of all scan operators
//  in order by size.
// -----------------------------------------------------------------------
void
RelExpr::makeListBySize(LIST(CostScalar) & orderedList, // order list of size
                        NABoolean  recompute)           // recompute memory
                                                        // limit -not used
{
  Int32 nc = getArity();
  RelExpr * expr       = this;
  CostScalar size = 0;

  if (recompute)
  {
    // this needs to be filled in if this ever is redriven by costing
    CMPASSERT(NOT recompute);
  }
  else
  {
    if (expr->getOperatorType() == REL_SCAN  OR
        expr->getOperatorType() == REL_GROUPBY)
    {
      //++MV, use the global empty input logical properties instead of
      //initializing a new one
      size =
       expr->getGroupAttr()->outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->getResultCardinality()
                    * expr->getGroupAttr()->getRecordLength() / 1024;
    }
  }

  if (size > 1)  // don't include anything 1KB or less
  {
    CollIndex idx = 0;

    for (idx = 0; idx < orderedList.entries(); idx++)
    {
      // list should be ordered by increasing estimated rowcount.
      if (orderedList[idx] >= size)
      {
        orderedList.insertAt (idx, size);
        break;
      }
    }

    // insert at end of list
    if (idx >= orderedList.entries())
    {
      orderedList.insertAt (orderedList.entries(), size);
    }

  }

  for (Lng32 i = 0; i < nc; i++)
  {
    CascadesGroup* group1 = (*CURRSTMT_OPTGLOBALS->memo)[expr->child(i).getGroupId()];
    // Only one log expr exist in the group at this point
    // if onlyMemoryOps is ever set true, we will have to traverse
    // the tree differently
    RelExpr * expr1 = group1->getFirstLogExpr();
    expr1->makeListBySize(orderedList, recompute);
  }
}

// Default implementation every RelExpr returns normal priority
PlanPriority RelExpr::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{
  PlanPriority result; // This will create normal plan priority
  return result;
}

// -----------------------------------------------------------------------
// Method for debugging
// -----------------------------------------------------------------------
void RelExpr::print(FILE * f,
		    const char * prefix,
		    const char * suffix) const
{
#ifndef NDEBUG
  ExprNode::print(f,prefix,suffix);

  fprintf(f,"%sRelational Expression:\n",prefix);

  if (selection_ != NULL)
    selection_->print(f,prefix,suffix);
  else
    predicates_.print(f,prefix,suffix);

  // print children or input equivalence classes
  Int32 nc = getArity();
  for (Lng32 i = 0; i < nc; i++)
    {
      fprintf(f,"%sExpression input %d:\n",prefix,i);
      if (child(i).getMode() == ExprGroupId::MEMOIZED)
	{
	  fprintf(f,
		  "%s    input eq. class #%d\n",
		  prefix,
		  child(i).getGroupId());
	}
      else
	{
	  if (child(i).getPtr() != NULL)
	    child(i)->print(f,CONCAT(prefix,"    "));
	  else
	    fprintf(f,"%snonexistent child\n",prefix);
	}
    }
#endif
}

Int32 RelExpr::nodeCount() const
{
  Int32 result = 1; // start from me.
  Int32 nc = getArity();
  for (Lng32 i = 0; i < nc; i++)
	  if (child(i).getPtr() != NULL)
		result += child(i)->nodeCount();

  return result;
}

NABoolean RelExpr::containsNode(OperatorTypeEnum nodeType)
{
  if (getOperatorType() == nodeType)
    return TRUE;

  Int32 nc = getArity();
  for (Int32 i = 0; i < nc; i++)
    {
    
      if (child(i).getPtr() != NULL &&
          child(i)->containsNode(nodeType))
        return TRUE;
    }
    
  return FALSE;
}

double RelExpr::computeMemoryQuota(NABoolean inMaster,
                                   NABoolean perNode,
                                   double BMOsMemoryLimit, // in MB 
                                   UInt16 totalNumBMOs, // per query 
                                   double totalBMOsMemoryUsage, // for all BMOs per node in bytes 
                                   UInt16 numBMOsPerFragment, // per fragment
                                   double bmoMemoryUsage, // for the current BMO/Operator per node in bytes
                                   Lng32 numStreams,
                                   double &bmoQuotaRatio
                                   ) 
{
   if ( perNode == TRUE ) {
      Lng32 exeMem = Lng32(BMOsMemoryLimit/(1024*1024));

     // the quota is allocated in 2 parts
     // The constant part divided equally across all bmo operators
     // The variable part allocated in proportion of the given BMO operator
     // estimated memory usage to the total estimated memory usage of all BMOs
   
     // The ratio can be capped by the CQD
     double equalQuotaShareRatio = 0;
     equalQuotaShareRatio = ActiveSchemaDB()->getDefaults().getAsDouble(BMO_MEMORY_EQUAL_QUOTA_SHARE_RATIO);
     double constMemQuota = 0;
     double variableMemLimit = exeMem;
     if (equalQuotaShareRatio > 0 && totalNumBMOs > 1) {
        constMemQuota = (exeMem * equalQuotaShareRatio )/ totalNumBMOs;
        variableMemLimit = (1-equalQuotaShareRatio) * exeMem;
     }
     double bmoMemoryRatio = bmoMemoryUsage / totalBMOsMemoryUsage;
     bmoQuotaRatio = bmoMemoryRatio;
     double bmoMemoryQuotaPerNode = constMemQuota + (variableMemLimit * bmoMemoryRatio);
     double numInstancesPerNode = numStreams / MINOF(MAXOF(((NAClusterInfoLinux*)gpClusterInfo)->getTotalNumberOfCPUs(), 1), numStreams);
     double bmoMemoryQuotaPerInstance =  bmoMemoryQuotaPerNode / numInstancesPerNode;
     return bmoMemoryQuotaPerInstance;
  } else {
     // the old way to compute quota 
     Lng32 exeMem = getExeMemoryAvailable(inMaster);
     bmoQuotaRatio = BMOQuotaRatio::NO_RATIO;
     return exeMem / numBMOsPerFragment; 
  }
}

Lng32 RelExpr::getExeMemoryAvailable(NABoolean inMaster) const
{
   Lng32 exeMemAvailMB = 
      ActiveSchemaDB()->getDefaults().getAsLong(EXE_MEMORY_AVAILABLE_IN_MB);
   return exeMemAvailMB;
}

// -----------------------------------------------------------------------
// methods for class RelExprList
// -----------------------------------------------------------------------
void RelExprList::insertOrderByRowcount (RelExpr * expr)
{
  Int32 i = 0;
  NABoolean done = FALSE;


  // QSTUFF
  // insert stream expression as the left most expression
  // by articially forcing it to have lowest cost
  // assumes that only one stream and one embedded update clause
  // is in the statement

  if (expr->getGroupAttr()->isStream() ||
    expr->getGroupAttr()->isEmbeddedUpdateOrDelete())
  {
    insertAt(0,expr);
    done = TRUE;
  }
  // QSTUFF

  while (!done && i < (Int32)entries())
  {
    CostScalar thisCard = (*this)[i]->
                            getGroupAttr()->
                              getResultCardinalityForEmptyInput();
    CostScalar exprCard = expr->
                            getGroupAttr()->
                              getResultCardinalityForEmptyInput();

    NABoolean increasing =
       ((ActiveSchemaDB()->getDefaults()).getAsULong(COMP_INT_90)==1);

    // list should be ordered by increasing estimated rowcount.
    if (((thisCard >= exprCard ) && increasing) ||
        ((thisCard < exprCard ) && !increasing))
    {
      // QSTUFF
      // stream and nested updates or deletes expressions should always be
      // left most, i.e of lowest cost

      if (
        (*this)[i]->getGroupAttr()->isStream() ||
        (*this)[i]->getGroupAttr()->isEmbeddedUpdateOrDelete())
        i++;
      // QSTUFF

      insertAt (i, expr);
      done = TRUE;
    }
    else
      i++;
  }

  // insert at end of list
  if (!done) insertAt (entries(), expr);
}

NABoolean RelExprList::operator== (const RelExprList &other) const
{
  if (entries() != other.entries())
    return FALSE;

  for (Lng32 i = 0; i < (Lng32)entries(); i++)
  {
    if ((*this)[i] != other[i])
      return FALSE;
  }
  return TRUE;
}

NABoolean RelExprList::operator!= (const RelExprList &other) const
{
  if ((*this) == other)
    return FALSE;
  else
    return TRUE;
}

// -----------------------------------------------------------------------
// methods for class CutOp
// -----------------------------------------------------------------------

CutOp::~CutOp() {}

void CutOp::print(FILE * f,
		  const char * prefix,
		  const char *) const
{
#ifndef NDEBUG
  if (getGroupId() == INVALID_GROUP_ID)
    fprintf(f, "%sLeaf (%d)\n", prefix, index_);
  else
    fprintf(f, "%sLeaf (%d, bound to group #%d)\n",
	    prefix, index_, getGroupId());

  return;
#endif
}

Int32 CutOp::getArity () const { return 0; }

NABoolean CutOp::isCutOp() const { return TRUE; }

const NAString CutOp::getText() const
{
  char theText[TEXT_DISPLAY_LENGTH];

  if (getGroupId() == INVALID_GROUP_ID)
    sprintf(theText, "Cut (%d)", index_);
  else
    if (index_ < 99)
      sprintf(theText, "Cut (%d, #%d)", index_, getGroupId());
    else
      // don't display funny indexes (>= 99)
      sprintf(theText, "Cut (#%d)", getGroupId());

  return NAString(theText);
}

RelExpr * CutOp::copyTopNode(RelExpr * derivedNode, CollHeap* outHeap)
{
  if (getGroupId() == INVALID_GROUP_ID)
    {
      // this is a standalone cut operator (e.g. in the tree of a
      // CONTROL QUERY SHAPE directive), return a copy of it
      CMPASSERT(derivedNode == NULL);
      CutOp* result = new (outHeap)CutOp(index_, outHeap);
      return RelExpr::copyTopNode(result,outHeap);
    }
  else
    {
      // CutOps are shared among the pattern and the substitute of
      // a rule. Often the substitute is produced by calling the copyTree()
      // method on the "before" expression or a part of it. This implementation
      // of copyTopNode() makes it possible to do that.
      return this;
    }
}

void CutOp::setGroupIdAndAttr(CascadesGroupId groupId)
{
  setGroupId(groupId);

  // set the group attributes of the leaf node to match the group
  if (groupId == INVALID_GROUP_ID)
    setGroupAttr(NULL);
  else
    setGroupAttr((*CURRSTMT_OPTGLOBALS->memo)[groupId]->getGroupAttr());
}

void CutOp::setExpr(RelExpr *e)
{
  expr_ = e;
  if (expr_ == NULL)
    {
      setGroupIdAndAttr(INVALID_GROUP_ID);
    }
  else
    {
      setGroupAttr(expr_->getGroupAttr());	  // ##shouldn't this line..
      // setGroupIdAndAttr(expr_->getGroupId());  // ##..be replaced by this?
    }
}

// -----------------------------------------------------------------------
// methods for class SubtreeOp
// -----------------------------------------------------------------------

SubtreeOp::~SubtreeOp() {}

Int32 SubtreeOp::getArity() const { return 0; }

NABoolean SubtreeOp::isSubtreeOp() const { return TRUE; }

const NAString SubtreeOp::getText() const { return NAString("Tree Op"); }

RelExpr * SubtreeOp::copyTopNode(RelExpr *, CollHeap*) { return this; }

// -----------------------------------------------------------------------
// methods for class WildCardOp
// -----------------------------------------------------------------------

WildCardOp::~WildCardOp() {}

Int32 WildCardOp::getArity() const
{
  switch (getOperatorType())
    {
    case REL_ANY_LEAF_OP:
    case REL_FORCE_ANY_SCAN:
    case REL_ANY_ROUTINE:
    case REL_FORCE_ANY_SCALAR_UDF:
    case REL_ANY_SCALAR_UDF_ROUTINE:
    case REL_ANY_LEAF_GEN_UPDATE:
    case REL_ANY_LEAF_TABLE_MAPPING_UDF:
      return 0;

    case REL_ANY_UNARY_GEN_UPDATE:
    case REL_ANY_UNARY_OP:
    case REL_ANY_GROUP:
    case REL_FORCE_EXCHANGE:
    case REL_ANY_UNARY_TABLE_MAPPING_UDF:
    case REL_ANY_EXTRACT:
      return 1;

    case REL_ANY_BINARY_OP:
    case REL_ANY_JOIN:
    case REL_ANY_TSJ:
    case REL_ANY_SEMIJOIN:
    case REL_ANY_SEMITSJ:
    case REL_ANY_ANTI_SEMIJOIN:
    case REL_ANY_ANTI_SEMITSJ:
    case REL_ANY_INNER_JOIN:
    case REL_ANY_NON_TS_INNER_JOIN:
    case REL_ANY_NON_TSJ_JOIN:
    case REL_ANY_LEFT_JOIN:
    case REL_ANY_LEFT_TSJ:
    case REL_ANY_NESTED_JOIN:
    case REL_ANY_HASH_JOIN:
    case REL_ANY_MERGE_JOIN:
    case REL_FORCE_JOIN:
    case REL_FORCE_NESTED_JOIN:
    case REL_FORCE_HASH_JOIN:
    case REL_FORCE_ORDERED_HASH_JOIN:
    case REL_FORCE_HYBRID_HASH_JOIN:
    case REL_FORCE_MERGE_JOIN:
    case REL_FORCE_ORDERED_CROSS_PRODUCT:
    case REL_ANY_BINARY_TABLE_MAPPING_UDF:
      return 2;

    default:
      ABORT("WildCardOp with unknown arity encountered");
      return 0;
    }
}

NABoolean WildCardOp::isWildcard() const { return TRUE; }

const NAString WildCardOp::getText() const
{
  switch (getOperatorType())
    {
    case ANY_REL_OR_ITM_OP:
      return "ANY_REL_OR_ITM_OP";
    case REL_ANY_LEAF_OP:
      return "REL_ANY_LEAF_OP";
    case REL_ANY_UNARY_OP:
      return "REL_ANY_UNARY_OP";
    case REL_ANY_ROUTINE:
      return "REL_ANY_ROUTINE";
    case REL_ANY_GEN_UPDATE:
      return "REL_ANY_GEN_UPDATE";
    case REL_ANY_UNARY_GEN_UPDATE:
      return "REL_ANY_UNARY_GEN_UPDATE";
    case REL_ANY_LEAF_GEN_UPDATE:
      return "REL_ANY_LEAF_GEN_UPDATE";
    case REL_ANY_GROUP:
      return "REL_ANY_GROUP";
    case REL_ANY_BINARY_OP:
      return "REL_ANY_BINARY_OP";
    case REL_ANY_JOIN:
      return "REL_ANY_JOIN";
    case REL_ANY_TSJ:
      return "REL_ANY_TSJ";
    case REL_ANY_SEMIJOIN:
      return "REL_ANY_SEMIJOIN";
    case REL_ANY_SEMITSJ:
      return "REL_ANY_SEMITSJ";
    case REL_ANY_INNER_JOIN:
      return "REL_ANY_INNER_JOIN";
    case REL_ANY_LEFT_JOIN:
      return "REL_ANY_LEFT_JOIN";
    case REL_ANY_LEFT_TSJ:
      return "REL_ANY_LEFT_TSJ";
    case REL_ANY_NESTED_JOIN:
      return "REL_ANY_NESTED_JOIN";
    case REL_ANY_HASH_JOIN:
      return "REL_ANY_HASH_JOIN";
    case REL_ANY_MERGE_JOIN:
      return "REL_ANY_MERGE_JOIN";
    case REL_ANY_EXTRACT:
      return "REL_ANY_EXTRACT";
    case REL_FORCE_ANY_SCAN:
      return "REL_FORCE_ANY_SCAN";
    case REL_FORCE_EXCHANGE:
      return "REL_FORCE_EXCHANGE";
    case REL_FORCE_JOIN:
      return "REL_FORCE_JOIN";
    case REL_FORCE_NESTED_JOIN:
      return "REL_FORCE_NESTED_JOIN";
    case REL_FORCE_HASH_JOIN:
      return "REL_FORCE_HASH_JOIN";
    case REL_FORCE_HYBRID_HASH_JOIN:
      return "REL_FORCE_HYBRID_HASH_JOIN";
    case REL_FORCE_ORDERED_HASH_JOIN:
      return "REL_FORCE_ORDERED_HASH_JOIN";
    case REL_FORCE_MERGE_JOIN:
      return "REL_FORCE_MERGE_JOIN";
    default:
      return "unknown??";
    }
}

RelExpr * WildCardOp::copyTopNode(RelExpr * derivedNode,
				  CollHeap* outHeap)
{
  if (corrNode_ != NULL)
    return corrNode_->copyTopNode(0, outHeap);
  else
    {
      if (derivedNode != NULL)
	return derivedNode;
      else
	{
	  WildCardOp* result;

	  result = new (outHeap) WildCardOp(getOperatorType(),
					    0,
					    NULL,
					    NULL,
					    outHeap);
	  return RelExpr::copyTopNode(result,outHeap);
	}
    }

  return NULL; // shouldn't really reach here
}

// -----------------------------------------------------------------------
// member functions for class ScanForceWildCard
// -----------------------------------------------------------------------

ScanForceWildCard::ScanForceWildCard(CollHeap * outHeap) :
  WildCardOp(REL_FORCE_ANY_SCAN),
  exposedName_(outHeap),
  indexName_(outHeap)
{initializeScanOptions();}

ScanForceWildCard::ScanForceWildCard(const NAString& exposedName,
				     CollHeap *outHeap) :
  WildCardOp(REL_FORCE_ANY_SCAN,0,NULL,NULL,outHeap),
  exposedName_(exposedName, outHeap),
  indexName_(outHeap)
{initializeScanOptions();}

ScanForceWildCard::ScanForceWildCard(const NAString& exposedName,
				     const NAString& indexName,
				     CollHeap *outHeap) :
  WildCardOp(REL_FORCE_ANY_SCAN,0,NULL,NULL,outHeap),
  exposedName_(exposedName, outHeap),
  indexName_(indexName, outHeap)
{initializeScanOptions();}

ScanForceWildCard::~ScanForceWildCard()
{
  collHeap()->deallocateMemory((void*)enumAlgorithms_);
  // delete enumAlgorithms_ from the same heap were the
  // ScanForceWildCard object belong
}

//----------------------------------------------------------
// initialize class members
//----------------------------------------------------------
void ScanForceWildCard::initializeScanOptions()
{
  mdamStatus_ = UNDEFINED;
  direction_ = UNDEFINED;
  indexStatus_ = UNDEFINED;
  numMdamColumns_ = 0;
  mdamColumnsStatus_ = UNDEFINED;
  enumAlgorithms_ = NULL;
  numberOfBlocksToReadPerAccess_ = -1;
}

//----------------------------------------------------------
// get the enumeration algorithm (density) for column
// if beyound specified columns return COLUMN_SYSTEM
//----------------------------------------------------------
ScanForceWildCard::scanOptionEnum
  ScanForceWildCard::getEnumAlgorithmForColumn(CollIndex column) const
{
  if (column >= numMdamColumns_)
    return ScanForceWildCard::COLUMN_SYSTEM;
  else
    return enumAlgorithms_[column];
}

//----------------------------------------------------------
// set the following scan option. return FALSE only if
// such option does not exist.
//----------------------------------------------------------
NABoolean ScanForceWildCard::setScanOptions(ScanForceWildCard::scanOptionEnum option)
{
  if (option == INDEX_SYSTEM)
    {
      indexStatus_ = INDEX_SYSTEM;
      return TRUE;
    }
  else if (option == MDAM_SYSTEM)
    {
      mdamStatus_ = MDAM_SYSTEM;
      return TRUE;
    }
  else if (option == MDAM_OFF)
    {
      mdamStatus_ = MDAM_OFF;
      return TRUE;
    }
  else if (option == MDAM_FORCED)
    {
      mdamStatus_ = MDAM_FORCED;
      return TRUE;
    }
  else if (option == DIRECTION_FORWARD)
    {
      direction_ = DIRECTION_FORWARD;
      return TRUE;
    }
  else if (option == DIRECTION_REVERSED)
    {
      direction_ = DIRECTION_REVERSED;
      return TRUE;
    }
  else if (option == DIRECTION_SYSTEM)
    {
      direction_ = DIRECTION_SYSTEM;
      return TRUE;
    }
  else return FALSE;
}


NABoolean ScanForceWildCard::setIndexName(const NAString& value)
{
  if (value != "")
    {
      indexName_ = value;
      return TRUE;
    }
  else
    return FALSE;		// Error should be nonempty string
}

//----------------------------------------------------------
// set the columns options based on passed values
//----------------------------------------------------------
NABoolean ScanForceWildCard::
  setColumnOptions(CollIndex numColumns,
		   ScanForceWildCard::scanOptionEnum* columnAlgorithms,
		   ScanForceWildCard::scanOptionEnum mdamColumnsStatus)
{
  mdamStatus_ = MDAM_FORCED;
  mdamColumnsStatus_ = mdamColumnsStatus;
  numMdamColumns_ = numColumns;
  // delete enumAlgorithms_ from the same heap were the
  // ScanForceWildCard object belong
  collHeap()->deallocateMemory((void*)enumAlgorithms_);
  // allocate enumAlgorithms_[numMdamColumns] in the same heap
  // were the ScanForceWildCard object belong
  enumAlgorithms_ = (ScanForceWildCard::scanOptionEnum*)
    collHeap()->allocateMemory(sizeof(ScanForceWildCard::scanOptionEnum)*numMdamColumns_);
  for (CollIndex i=0; i<numMdamColumns_; i++)
    enumAlgorithms_[i] = columnAlgorithms[i];
  return TRUE;
}

//----------------------------------------------------------
// set the columns options based on passed values
// here options for particular columns are not passed
// and hence COLUMN_SYSTEM is assigned
//----------------------------------------------------------
NABoolean ScanForceWildCard::
  setColumnOptions(CollIndex numColumns,
		   ScanForceWildCard::scanOptionEnum mdamColumnsStatus)
{
  mdamStatus_ = MDAM_FORCED;
  mdamColumnsStatus_ = mdamColumnsStatus;
  numMdamColumns_ = numColumns;
  // delete enumAlgorithms_ from the same heap were the
  // ScanForceWildCard object belong
  collHeap()->deallocateMemory((void*)enumAlgorithms_);
  // allocate enumAlgorithms_[numMdamColumns] in the same heap
  // were the ScanForceWildCard object belong
  enumAlgorithms_ = (ScanForceWildCard::scanOptionEnum*)
    collHeap()->allocateMemory(sizeof(ScanForceWildCard::scanOptionEnum)*numMdamColumns_);
  //enumAlgorithms_ = new scanOptionEnum[numMdamColumns_];
  for (CollIndex i=0; i<numMdamColumns_; i++)
    enumAlgorithms_[i] = COLUMN_SYSTEM;
  return TRUE;
}

//----------------------------------------------------------
// check if the forced scan options conflict with the Mdam
// Master switch status
//----------------------------------------------------------
NABoolean ScanForceWildCard::doesThisCoflictMasterSwitch() const
{
  char* globalMdamStatus = getenv("MDAM");
  if (globalMdamStatus != NULL)
    {
      if (strcmp(globalMdamStatus,"OFF")==0 )
	{
	  if ((mdamStatus_ == MDAM_FORCED)||(mdamStatus_ == MDAM_SYSTEM))
	    return TRUE;
	}
    }
  return FALSE;
}

//----------------------------------------------------------
// merge with another ScanForceWildCard object.
// return FALSE if a conflict between the options of
// the two objects exists.
//----------------------------------------------------------
NABoolean ScanForceWildCard::mergeScanOptions(const ScanForceWildCard &other)
{
  if ((other.exposedName_ != "")
      &&(other.exposedName_ != exposedName_))
    {
      if (exposedName_ == "")
	{
	  exposedName_ = other.exposedName_;
	}
      else
	return FALSE; // conflict
    }

  if ((other.indexName_ != "")
      &&(other.indexName_ != indexName_))
    {
      if (indexName_ == "")
	{
	  indexName_ = other.indexName_;
	}
      else
	return FALSE; // conflict
    }

  if (other.indexStatus_ == INDEX_SYSTEM)
    {
      indexStatus_ = INDEX_SYSTEM;
    }

  if (indexStatus_ == INDEX_SYSTEM)
    {
      if (indexName_ != "")
	return FALSE;  // conflict
    }

  if ((other.mdamStatus_ == MDAM_OFF)
      &&(mdamStatus_ != MDAM_OFF))
    {
      if (mdamStatus_ == UNDEFINED)
	{
	  mdamStatus_ = other.mdamStatus_;
	}
      else
	return FALSE; // conflict
    }

  if ((other.mdamStatus_ == MDAM_SYSTEM)
      &&(mdamStatus_ != MDAM_SYSTEM))
    {
      if (mdamStatus_ == UNDEFINED)
	{
	  mdamStatus_ = other.mdamStatus_;
	}
      else
	return FALSE; // conflict
    }

  if ((other.mdamStatus_ == MDAM_FORCED)
      &&(mdamStatus_ != MDAM_FORCED))
    {
      if (mdamStatus_ == UNDEFINED)
	{
	  mdamStatus_ = other.mdamStatus_;
	}
      else
	return FALSE; // conflict
    }

  if (other.numMdamColumns_ > 0)
    {
      if ((mdamStatus_ == UNDEFINED)||(mdamStatus_ == MDAM_FORCED))
	{
	  if (numMdamColumns_ == other.numMdamColumns_)
	    {
	      for (CollIndex i=0; i<numMdamColumns_; i++)
		{
		  if (enumAlgorithms_[i] != other.enumAlgorithms_[i])
		    return FALSE;	// conflict
		}
	      if (other.mdamColumnsStatus_ != mdamColumnsStatus_)
		return FALSE;		// conflict
	    }
	  else if (numMdamColumns_ == 0) // i.e. enumAlgorithm is NULL
	    {
	      numMdamColumns_ = other.numMdamColumns_;
	      collHeap()->deallocateMemory((void*)enumAlgorithms_);
	      //delete enumAlgorithms_;
	      enumAlgorithms_ = (ScanForceWildCard::scanOptionEnum*)
		collHeap()->allocateMemory(sizeof(ScanForceWildCard::scanOptionEnum)*numMdamColumns_);
	      //enumAlgorithms_ = new scanOptionEnum[numMdamColumns_];
	      for (CollIndex i=0; i<numMdamColumns_; i++)
		{
		  enumAlgorithms_[i] = other.enumAlgorithms_[i];
		}
	    }
	  else
	    return FALSE;	// coflict
	}
      else
	return FALSE; // conflict
    }

  if (other.mdamColumnsStatus_ != UNDEFINED)
    {
      if (mdamColumnsStatus_ == UNDEFINED)
	{
	  mdamColumnsStatus_ = other.mdamColumnsStatus_;
	}
      if (mdamColumnsStatus_ != other.mdamColumnsStatus_)
	{
	  return FALSE; // conflict
	}
    }

  if ((other.direction_ == DIRECTION_FORWARD)
      &&(direction_ != DIRECTION_FORWARD))
    {
      if (direction_ == UNDEFINED)
	{
	  direction_ = other.direction_;
	}
      else
	return FALSE; // conflict
    }

  if ((other.direction_ == DIRECTION_REVERSED)
      &&(direction_ != DIRECTION_REVERSED))
    {
      if (direction_ == UNDEFINED)
	{
	  direction_ = other.direction_;
	}
      else
	return FALSE; // conflict
    }

  if ((other.direction_ == DIRECTION_SYSTEM)
      &&(direction_ != DIRECTION_SYSTEM))
    {
      if (direction_ == UNDEFINED)
	{
	  direction_ = other.direction_;
	}
      else
	return FALSE; // conflict
    }

  if (other.numberOfBlocksToReadPerAccess_ > 0)
    {
      numberOfBlocksToReadPerAccess_ = other.numberOfBlocksToReadPerAccess_;
    }

  return TRUE;
}

//----------------------------------------------------------
// if access path is not given then the default is ANY i.e
// system choice unless MDAM is forced then the default is
// the base table.
//----------------------------------------------------------
void ScanForceWildCard::prepare()
{
  if (mdamStatus_ != ScanForceWildCard::MDAM_FORCED)
    {
      if (indexName_ == "")
	{
	  indexStatus_ = ScanForceWildCard::INDEX_SYSTEM;
	}
    }
  else // mdam is forced
    {
      if ((indexName_ == "") &&
	  (indexStatus_ != ScanForceWildCard::INDEX_SYSTEM))
        {
  	  indexName_ = exposedName_;
	}
    }
  if (mdamColumnsStatus_ == ScanForceWildCard::UNDEFINED)
   {
     mdamColumnsStatus_ = ScanForceWildCard::MDAM_COLUMNS_REST_BY_SYSTEM;
   }

  return;
}

RelExpr * ScanForceWildCard::copyTopNode(RelExpr *derivedNode,
					 CollHeap* outHeap)
{
  ScanForceWildCard *temp;
  if (derivedNode != NULL)
    ABORT("No support for classes derived from ScanForceWildcard");

  temp = new (outHeap) ScanForceWildCard(exposedName_,indexName_,outHeap);
  temp->direction_ = direction_;
  temp->indexStatus_ = indexStatus_;
  temp->mdamStatus_ = mdamStatus_;
  temp->mdamColumnsStatus_ = mdamColumnsStatus_;
  temp->numMdamColumns_ = numMdamColumns_;
  temp->numberOfBlocksToReadPerAccess_ = numberOfBlocksToReadPerAccess_;
  temp->enumAlgorithms_ = new (outHeap) scanOptionEnum[numMdamColumns_];
  for (CollIndex i=0; i<numMdamColumns_; i++)
    {
      temp->enumAlgorithms_[i]=enumAlgorithms_[i];
    }
  RelExpr *result = temp;
  WildCardOp::copyTopNode(result,outHeap);

  return result;
}

const NAString ScanForceWildCard::getText() const
{
  NAString result("forced scan", CmpCommon::statementHeap());

  if (exposedName_ != "")
    {
      result += "(";
      result += exposedName_;

      if (indexName_ != "")
	{
	  result += ", index ";
	  result += indexName_;
	}
      result += ")";
    }

  return result;
}

// -----------------------------------------------------------------------
// member functions for class JoinForceWildCard
// -----------------------------------------------------------------------

JoinForceWildCard::JoinForceWildCard(OperatorTypeEnum type,
				     RelExpr *child0,
				     RelExpr *child1,
				     forcedPlanEnum plan,
				     Int32 numOfEsps,
				     CollHeap *outHeap) :
     WildCardOp(type, 0, child0, child1,outHeap)
{
  plan_ = plan;
  numOfEsps_ = numOfEsps;
}

JoinForceWildCard::~JoinForceWildCard() {}

RelExpr * JoinForceWildCard::copyTopNode(RelExpr *derivedNode,
					 CollHeap* outHeap)
{
  RelExpr *result;
  if (derivedNode != NULL)
    ABORT("No support for classes derived from JoinForceWildcard");

  result = new(outHeap) JoinForceWildCard(getOperatorType(),
					  NULL,
					  NULL,
					  plan_,
					  numOfEsps_,
					  outHeap);
  WildCardOp::copyTopNode(result,outHeap);

  return result;
}

const NAString JoinForceWildCard::getText() const
{
  NAString result("forced", CmpCommon::statementHeap());

  if (plan_ == FORCED_PLAN0)
    result += " plan0";
  else if (plan_ == FORCED_PLAN1)
    result += " plan1";
  else if (plan_ == FORCED_PLAN2)
    result += " plan2";
  else if (plan_ == FORCED_TYPE1)
    result += " type1";
  else if (plan_ == FORCED_TYPE2)
    result += " type2";
  else if (plan_ == FORCED_INDEXJOIN)
    result += " indexjoin";

  switch (getOperatorType())
    {
    case REL_FORCE_NESTED_JOIN:
      result += " nested join";
      break;
    case REL_FORCE_MERGE_JOIN:
      result += " merge join";
      break;
    case REL_FORCE_HASH_JOIN:
      result += " hash join";
      break;
    case REL_FORCE_HYBRID_HASH_JOIN:
      result += " hybrid hash join";
      break;
    case REL_FORCE_ORDERED_HASH_JOIN:
      result += " ordered hash join";
      break;
    default:
      result += " join";
      break;
    }

  return result;
}

// -----------------------------------------------------------------------
// member functions for class ExchangeForceWildCard
// -----------------------------------------------------------------------

ExchangeForceWildCard::ExchangeForceWildCard(RelExpr *child0,
					     forcedExchEnum which,
					     forcedLogPartEnum whatLogPart,
					     Lng32 numBottomEsps,
					     CollHeap *outHeap) :
     WildCardOp(REL_FORCE_EXCHANGE, 0, child0, NULL, outHeap),
     which_(which),
     whatLogPart_(whatLogPart),
     howMany_(numBottomEsps)
{
}

ExchangeForceWildCard::~ExchangeForceWildCard() {}

RelExpr * ExchangeForceWildCard::copyTopNode(RelExpr *derivedNode,
					 CollHeap* outHeap)
{
  RelExpr *result;
  if (derivedNode != NULL)
    ABORT("No support for classes derived from ExchangeForceWildcard");

  result = new(outHeap) ExchangeForceWildCard(NULL,
					      which_,
					      whatLogPart_,
					      howMany_,
					      outHeap);
  WildCardOp::copyTopNode(result,outHeap);

  return result;
}

const NAString ExchangeForceWildCard::getText() const
{
  NAString result("forced",CmpCommon::statementHeap());

  if (which_ == FORCED_PA)
    result += " PA";
  else if (which_ == FORCED_PAPA)
    result += " PAPA";
  else if (which_ == FORCED_ESP_EXCHANGE)
    result += " ESP";

  result += " exchange";

  return result;
}

// -----------------------------------------------------------------------
// member functions for class UDFForceWildCard
// -----------------------------------------------------------------------

UDFForceWildCard::UDFForceWildCard(OperatorTypeEnum op, CollHeap *outHeap) :
  WildCardOp(op, 0, NULL, NULL, outHeap),
  functionName_(outHeap),
  actionName_(outHeap)
{}

UDFForceWildCard::UDFForceWildCard(const NAString& functionName, 
                                   const NAString& actionName, 
                                   CollHeap *outHeap) :
  WildCardOp(REL_FORCE_ANY_SCALAR_UDF, 0, NULL, NULL, outHeap),
  functionName_(functionName, outHeap),
  actionName_(actionName, outHeap)
{}

UDFForceWildCard::~UDFForceWildCard()
{
}

//----------------------------------------------------------
// merge with another UDFForceWildCard object.
// return FALSE if a conflict between the options of
// the two objects exists.
//----------------------------------------------------------
NABoolean UDFForceWildCard::mergeUDFOptions(const UDFForceWildCard &other)
{
  if ((other.functionName_ != "")
      &&(other.functionName_ != functionName_))
    {
      if (functionName_ == "")
	{
	  functionName_ = other.functionName_;
	}
      else
	return FALSE; // conflict
    }
  if ((other.actionName_ != "")
      &&(other.actionName_ != actionName_))
    {
      if (actionName_ == "")
	{
	  actionName_ = other.actionName_;
	}
      else
	return FALSE; // conflict
    }

  return TRUE;
}

RelExpr * UDFForceWildCard::copyTopNode(RelExpr *derivedNode,
					 CollHeap* outHeap)
{
  UDFForceWildCard *temp;
  if (derivedNode != NULL)
    ABORT("No support for classes derived from UDFForceWildCard");

  temp = new (outHeap) UDFForceWildCard(functionName_,actionName_,outHeap);
  RelExpr *result = temp;
  WildCardOp::copyTopNode(result,outHeap);

  return result;
}

const NAString UDFForceWildCard::getText() const
{
  NAString result("forced UDF", CmpCommon::statementHeap());
  if (functionName_ != "")
    {
      result += "(";
      result += functionName_;
      if (actionName_ != "")
        {
          result += ", ";
          result += actionName_;
        }
      result += ")";
    }
  return result;
}

// -----------------------------------------------------------------------
// member functions for Control* base class
// -----------------------------------------------------------------------

ControlAbstractClass::~ControlAbstractClass() {}

NABoolean ControlAbstractClass::duplicateMatch(const RelExpr & other) const
{
  if (NOT RelExpr::duplicateMatch(other))
    return FALSE;

  ControlAbstractClass &o = (ControlAbstractClass &) other;

  // We do NOT need to compare sqlText's here
  return (token_ == o.token_     AND value_ == o.value_ AND
  	  dynamic_ == o.dynamic_ AND reset_ == o.reset_);
}

RelExpr *ControlAbstractClass::copyTopNode(RelExpr *derivedNode, CollHeap *h)
{
  CMPASSERT(derivedNode);
  ((ControlAbstractClass *)derivedNode)->reset_ = reset_;
  return derivedNode;
}

NABoolean ControlAbstractClass::alterArkcmpEnvNow() const
{ 
  return NOT (dynamic() || CmpCommon::context()->GetMode() == STMT_DYNAMIC); 
}

StaticOnly ControlAbstractClass::isAStaticOnlyStatement() const
{
  if (dynamic_) return NOT_STATIC_ONLY;
  return (getOperatorType() == REL_CONTROL_QUERY_DEFAULT) ?
    STATIC_ONLY_WITH_WORK_FOR_PREPROCESSOR : STATIC_ONLY_VANILLA;
}

static void removeLeadingSpace(NAString &sqlText)
{
  if (!sqlText.isNull() && isspace((unsigned char)sqlText[size_t(0)])) sqlText.remove(0, 1);  // For VS2003
}

static NABoolean beginsWithKeyword(NAString &sqlText, const char *kwd,
				   NABoolean thenRemoveIt = TRUE)
{
  // Assumes Prettify has been called, so only one space (at most) btw tokens.
  // If this is called more than once, the second time in the text might begin
  // with a delimiting space.
  removeLeadingSpace(sqlText);

  size_t len = strlen(kwd) + 1;		// +1 for delimiter (space)
  if (sqlText.length() > len) {
  NAString tmp(sqlText,CmpCommon::statementHeap());
    tmp.remove(len);
    char c = tmp[--len];		// delimiter
    if (!isalnum(c) && c != '_') {
      tmp.remove(len);			// remove the delimiter 'c' from tmp
      if (tmp == kwd) {
	if (thenRemoveIt)
	  sqlText.remove(0, len);	// leave delimiter, now at beginning
	return TRUE;
      }
    }
  }
  return FALSE;
}

// Convert "PROCEDURE xyz () CONTROL..." to just the "CONTROL..." part.
// Convert a dynamic stmt's text of  "SET SCHEMA 'x.y';"
// into the static "CONTROL QUERY DEFAULT SCHEMA 'x.y';"
// for its round-trip to executor and back here as a SQLTEXT_STATIC_COMPILE.
void ControlAbstractClass::rewriteControlText(
					NAString &sqlText, CharInfo::CharSet sqlTextCharSet,
					ControlAbstractClass *ctrl)
{
  PrettifySqlText(sqlText);	// trim, upcase where okay, cvt tabs to spaces

  if (beginsWithKeyword(sqlText, "PROCEDURE")) {
    size_t rp = sqlText.index(')');
    CMPASSERT(rp != NA_NPOS);
    sqlText.remove(0, ++rp);
    removeLeadingSpace(sqlText);
  }

  // if SHOWSHAPE or SHOWPLAN, remove them from the beginning.
  // beginsWithKeyword will remove the keyword, if found.
  if ((beginsWithKeyword(sqlText, "SHOWSHAPE")) ||
      (beginsWithKeyword(sqlText, "SHOWPLAN"))) {
    removeLeadingSpace(sqlText);
  }

  if (ctrl->dynamic()) {
    if ((beginsWithKeyword(sqlText, "SET")) &&
	(ctrl->getOperatorType() != REL_SET_SESSION_DEFAULT))
      sqlText.prepend(getControlTextPrefix(ctrl));
  }
  else {
    if (beginsWithKeyword(sqlText, "DECLARE"))
      sqlText.prepend(getControlTextPrefix(ctrl));
  }

  //## We'll have to fix SqlParser.y, I think, if this stmt appears in
  //## a compound stmt (IF ... THEN SET SCHEMA 'x' ... ELSE SET SCHEMA 'y' ...)
  if (ctrl->getOperatorType() != REL_SET_SESSION_DEFAULT)
    {
      if ((NOT beginsWithKeyword(sqlText, "CONTROL", FALSE)) &&
	  (NOT beginsWithKeyword(sqlText, "CQD", FALSE)))
	CMPASSERT(0);
    }
}

NAString ControlAbstractClass::getControlTextPrefix(
     const ControlAbstractClass *ctrl)
{
  switch (ctrl->getOperatorType()) {
    case REL_CONTROL_QUERY_SHAPE:	return "CONTROL QUERY SHAPE";
    case REL_CONTROL_QUERY_DEFAULT:	return "CONTROL QUERY DEFAULT";
    case REL_CONTROL_TABLE:		return "CONTROL TABLE";
    case REL_CONTROL_SESSION:		return "CONTROL SESSION";
    case REL_SET_SESSION_DEFAULT:	return "SET SESSION DEFAULT";
    default:				return "CONTROL ??";
  }
}

// -----------------------------------------------------------------------
// member functions for class ControlQueryShape
// -----------------------------------------------------------------------

RelExpr * ControlQueryShape::copyTopNode(RelExpr *derivedNode,
					 CollHeap *h)
{
  ControlQueryShape *result;

  if (derivedNode == NULL)
    result = new (h) ControlQueryShape(NULL, getSqlText(), getSqlTextCharSet(), holdShape_,
				       dynamic_, ignoreExchange_,
				       ignoreSort_, h);
  else
    result = (ControlQueryShape *) derivedNode;

  return ControlAbstractClass::copyTopNode(result,h);
}

const NAString ControlQueryShape::getText() const
{
  NAString result(getControlTextPrefix(this));

  if (ignoreExchange_)
    {
      if (ignoreSort_)
	result += " WITHOUT ENFORCERS";
      else
	result += " WITHOUT EXCHANGE";
    }
  else if (ignoreSort_)
    result += " WITHOUT SORT";

  return result;
}

// -----------------------------------------------------------------------
// member functions for class ControlQueryDefault
// -----------------------------------------------------------------------

ControlQueryDefault::ControlQueryDefault(
     const NAString &sqlText, CharInfo::CharSet sqlTextCharSet,
     const NAString &token,
     const NAString &value,
     NABoolean dyn,
     Lng32 holdOrRestoreCQD,
     CollHeap *h,
     Int32 reset):
   ControlAbstractClass(REL_CONTROL_QUERY_DEFAULT, sqlText, sqlTextCharSet, token, value,
                        dyn, h, reset),
   holdOrRestoreCQD_(holdOrRestoreCQD),
   attrEnum_(__INVALID_DEFAULT_ATTRIBUTE)
{}

RelExpr * ControlQueryDefault::copyTopNode(RelExpr *derivedNode,
					   CollHeap *h)
{
  RelExpr *result;

  if (derivedNode == NULL) {
    result = new (h) ControlQueryDefault(sqlText_, sqlTextCharSet_, token_, value_, dynamic_, holdOrRestoreCQD_, h);
    ((ControlQueryDefault *)result)->attrEnum_ = attrEnum_;
  }
  else
    result = derivedNode;

  return ControlAbstractClass::copyTopNode(result,h);
}

const NAString ControlQueryDefault::getText() const
{
  return getControlTextPrefix(this);
}

// -----------------------------------------------------------------------
// member functions for class ControlTable
// -----------------------------------------------------------------------

ControlTable::ControlTable(
     CorrName *tableName,
     const NAString &sqlText, CharInfo::CharSet sqlTextCharSet,
     const NAString &token,
     const NAString &value,
     NABoolean dyn,
     CollHeap *h):
   ControlAbstractClass(REL_CONTROL_TABLE, sqlText, sqlTextCharSet, token, value, dyn, h),
   tableName_(tableName)
{}

RelExpr * ControlTable::copyTopNode(RelExpr *derivedNode,
				    CollHeap *h)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (h) ControlTable(tableName_, sqlText_, sqlTextCharSet_, token_, value_, dynamic_, h);
  else
    result = derivedNode;

  return ControlAbstractClass::copyTopNode(result,h);
}

const NAString ControlTable::getText() const
{
  return getControlTextPrefix(this);
}

// -----------------------------------------------------------------------
// member functions for class ControlSession
// -----------------------------------------------------------------------

ControlSession::ControlSession(
     const NAString &sqlText, CharInfo::CharSet sqlTextCharSet,
     const NAString &token,
     const NAString &value,
     NABoolean dyn,
     CollHeap *h):
   ControlAbstractClass(REL_CONTROL_SESSION, sqlText, sqlTextCharSet, token, value, dyn, h)
{}

RelExpr * ControlSession::copyTopNode(RelExpr *derivedNode,
				      CollHeap *h)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (h) ControlSession(sqlText_, sqlTextCharSet_, token_, value_, dynamic_, h);
  else
    result = derivedNode;

  return ControlAbstractClass::copyTopNode(result,h);
}

const NAString ControlSession::getText() const
{
  return getControlTextPrefix(this);
}

// -----------------------------------------------------------------------
// member functions for class SetSessionDefault
// -----------------------------------------------------------------------

SetSessionDefault::SetSessionDefault(
     const NAString &sqlText, CharInfo::CharSet sqlTextCharSet,
     const NAString &token,
     const NAString &value,
     CollHeap *h):
     ControlAbstractClass(REL_SET_SESSION_DEFAULT, sqlText, sqlTextCharSet, token, value, TRUE, h)
{}

RelExpr * SetSessionDefault::copyTopNode(RelExpr *derivedNode,
					 CollHeap *h)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (h) SetSessionDefault(sqlText_, sqlTextCharSet_, token_, value_, h);
  else
    result = derivedNode;

  return ControlAbstractClass::copyTopNode(result,h);
}

const NAString SetSessionDefault::getText() const
{
  return getControlTextPrefix(this);
}

// -----------------------------------------------------------------------
// member functions for class OSIMControl
// -----------------------------------------------------------------------
OSIMControl::OSIMControl(OptimizerSimulator::osimMode mode,
                                       NAString & localDir,
                                       NABoolean force,
                                       CollHeap * oHeap)
                                       //the real work is done in OSIMControl::bindNode() to control OSIM.
                                       //We set operator type to REL_SET_SESSION_DEFAULT,
                                       //so as not to define dummy OSIMControl::codeGen() and OSIMControl::work(),
                                       //which will do nothing there,  
                       : ControlAbstractClass(REL_SET_SESSION_DEFAULT, NAString("DUMMYSQLTEXT", oHeap), 
                                                           CharInfo::ISO88591, NAString("OSIM", oHeap), 
                                                           NAString("DUMMYVALUE", oHeap), TRUE, oHeap)
                       , targetMode_(mode), osimLocalDir_(localDir, oHeap), forceLoad_(force)
{}


RelExpr * OSIMControl::copyTopNode(RelExpr *derivedNode, CollHeap *h )
{
      RelExpr *result;

      if (derivedNode == NULL)
          result = new (h) OSIMControl(targetMode_, osimLocalDir_, forceLoad_, h);
      else
          result = derivedNode;
          
      return ControlAbstractClass::copyTopNode(result,h);
}

// -----------------------------------------------------------------------
// member functions for class Sort
// -----------------------------------------------------------------------

Sort::~Sort()
{
}

Int32 Sort::getArity() const { return 1;}

HashValue Sort::topHash()
{
  HashValue result = RelExpr::topHash();

  result ^= sortKey_;
  result ^= arrangedCols_;

  return result;
}

NABoolean Sort::duplicateMatch(const RelExpr & other) const
{
  if (NOT RelExpr::duplicateMatch(other))
    return FALSE;

  Sort &o = (Sort &) other;

  if (NOT (sortKey_ == o.sortKey_) OR
      NOT(arrangedCols_ == o.arrangedCols_))
    return FALSE;

  return TRUE;
}

RelExpr * Sort::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Sort *result;

  if (derivedNode == NULL)
    result = new (outHeap) Sort(NULL, sortKey_, outHeap);
  else
    result = (Sort *) derivedNode;

  // copy arranged columns
  result->arrangedCols_ = arrangedCols_;

  return RelExpr::copyTopNode(result, outHeap);
}

NABoolean Sort::isLogical() const  { return FALSE; }

NABoolean Sort::isPhysical() const { return TRUE;  }

const NAString Sort::getText() const
{
  return "sort";
}

PlanPriority Sort::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{
  const PhysicalProperty* spp = context->getPlan()->getPhysicalProperty();
  Lng32 degreeOfParallelism = spp->getCountOfPartitions();
  double val = 1;
  if (degreeOfParallelism <= 1)
    {
      // serial plans are risky. exact an insurance premium from serial plans.
      val = CURRSTMT_OPTDEFAULTS->riskPremiumSerial();
    }
  CostScalar premium(val);
  PlanPriority result(0, 0, premium);

  if (QueryAnalysis::Instance() AND
      QueryAnalysis::Instance()->optimizeForFirstNRows())
    result.incrementLevels(SORT_FIRST_N_PRIORITY,0);

  // For the option of Max Degree of Parallelism we can either use the
  // value set in comp_int_9 (if positive) or we use the number of CPUs
  // if the CQD is set to -1, or feature is disabled if CQD is 0 (default).
  Lng32 maxDegree = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_9);
  if (CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible() OR ( maxDegree == -1) )
  {
    // if CQD is set to -1 this mean use the number of CPUs
    maxDegree = spp->getCurrentCountOfCPUs();
  }
  if (maxDegree > 1) // CQD set to 0 means feature is OFF
  {
    if (degreeOfParallelism < maxDegree)
      result.incrementLevels(0,-10); // need to replace with constant
  }

  return result;
}

void Sort::addLocalExpr(LIST(ExprNode *) &xlist,
			LIST(NAString) &llist) const
{
  if (sortKey_.entries() > 0)
    {
      xlist.insert(sortKey_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("sort_key");
    }

  if (PartialSortKeyFromChild_.entries() > 0)
    {
      xlist.insert(PartialSortKeyFromChild_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("PartialSort_FromChild");
    }

//  if (NOT arrangedCols_.isEmpty())
//    {
//      xlist.insert(arrangedCols_.rebuildExprTree(ITM_ITEM_LIST));
//      llist.insert("arranged_cols");
//    }

  RelExpr::addLocalExpr(xlist,llist);
}

void Sort::needSortedNRows(NABoolean val)
{
  sortNRows_ = val;
  // Sort changes a GET_N to GET_ALL, so it does not propagate a 
  // Get_N request. It can simply act on one.
}

// -----------------------------------------------------------------------
// member functions for class SortFromTop
// -----------------------------------------------------------------------
RelExpr * SortFromTop::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  SortFromTop *result;

  if (derivedNode == NULL)
    result = new (outHeap) SortFromTop(NULL, outHeap);
  else
    result = (SortFromTop *) derivedNode;

  result->getSortRecExpr() = getSortRecExpr();

  return Sort::copyTopNode(result, outHeap);
}

const NAString SortFromTop::getText() const
{
  return "sort_from_top";
}

// -----------------------------------------------------------------------
// member functions for class Exchange
// -----------------------------------------------------------------------

Exchange::~Exchange()
{
}

Int32 Exchange::getArity() const { return 1;}

RelExpr * Exchange::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Exchange *result;

  if (derivedNode == NULL)
    {
      result = new (outHeap) Exchange(NULL, outHeap);
    }
  else
    result = (Exchange *) derivedNode;

  result->upMessageBufferLength_=upMessageBufferLength_;
  result->downMessageBufferLength_=downMessageBufferLength_;
  result->hash2RepartitioningWithSameKey_ = hash2RepartitioningWithSameKey_;
  if (halloweenSortIsMyChild_) 
    result->markHalloweenSortIsMyChild();

  return RelExpr::copyTopNode(result, outHeap);
}

NABoolean Exchange::isLogical() const  { return FALSE; }

NABoolean Exchange::isPhysical() const { return TRUE;  }

NABoolean Exchange::isAPA() const
{
  if (NOT isDP2Exchange() OR bottomPartFunc_ == NULL)
    return FALSE;

  const LogPhysPartitioningFunction *lpf = bottomPartFunc_->
    castToLogPhysPartitioningFunction();

  return (lpf == NULL OR NOT lpf->getUsePapa());
}

NABoolean Exchange::isAPAPA() const
{
  return (isDP2Exchange() AND bottomPartFunc_ AND NOT isAPA());
}

const NAString Exchange::getText() const
{
  NAString result("exchange",CmpCommon::statementHeap());

  if (isAPA())
    result = "pa_exchange";
  else if (isAPAPA())
    result = "split_top";
  else if (isAnESPAccess())
    result = "esp_access";
  else if (isEspExchange())
    result = "esp_exchange";

  const PartitioningFunction *topPartFunc =
    getTopPartitioningFunction();
  const PartitioningFunction *bottomPartFunc =
    getBottomPartitioningFunction();

  Lng32 topNumParts = ANY_NUMBER_OF_PARTITIONS;
  Lng32 bottomNumParts = ANY_NUMBER_OF_PARTITIONS;

  if (topPartFunc)
    topNumParts = topPartFunc->getCountOfPartitions();
  if (bottomPartFunc)
    bottomNumParts = bottomPartFunc->getCountOfPartitions();

  if (topNumParts != ANY_NUMBER_OF_PARTITIONS OR
      bottomNumParts != ANY_NUMBER_OF_PARTITIONS)
    {
      char str[TEXT_DISPLAY_LENGTH];
      sprintf(str," %d:%d",topNumParts,bottomNumParts);
      result += str;
    }

  if (bottomPartFunc AND isDP2Exchange())
    {
      const LogPhysPartitioningFunction *lpf =
          bottomPartFunc->castToLogPhysPartitioningFunction();
      if (lpf)
	{
	  if (lpf->getUsePapa())
	    {
	      char str[TEXT_DISPLAY_LENGTH];
	      sprintf(str," with %d PA(s)",lpf->getNumOfClients());
	      result += str;
	    }
	  switch (lpf->getLogPartType())
	    {
	    case LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING:
	      result += ", log. subpart.";
	      break;
	    case LogPhysPartitioningFunction::HORIZONTAL_PARTITION_SLICING:
	      result += ", slicing";
	      break;
	    case LogPhysPartitioningFunction::PA_GROUPED_REPARTITIONING:
	      result += ", repartitioned";
	      break;
	    default:
	      break;
	    }
	}
    }
  return result;
}

PlanPriority Exchange::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{
  PlanPriority result;

  OperatorTypeEnum parOperType = context->getCurrentAncestor()->getPlan()->
                                   getPhysicalExpr()->getOperatorType();

  Lng32 cqdValue = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_9);
  if ((cqdValue == -2) AND
      ((parOperType == REL_ROOT) OR
       (parOperType == REL_FIRST_N)))
    result.incrementLevels(10,0);

  return result;
}

void Exchange::addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const
{
  const PartitioningFunction *topPartFunc =
    getTopPartitioningFunction();

  if (topPartFunc AND topPartFunc->getCountOfPartitions() > 1 AND
      topPartFunc->getPartitioningExpression())
    {
      xlist.insert(topPartFunc->getPartitioningExpression());
      llist.insert("partitioning_expression");
    }

  if (NOT sortKeyForMyOutput_.isEmpty())
    {
      xlist.insert(sortKeyForMyOutput_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("merged_order");
    }

  RelExpr::addLocalExpr(xlist,llist);
}

// -----------------------------------------------------------------------
// member functions for class Join
// -----------------------------------------------------------------------
Int32 Join::getArity() const { return 2;}

// helper function used in 
//   HashJoinRule::topMatch() and 
//   JoinToTSJRule::topMatch()
// to make sure that only one of them is turned off.
// Otherwise, error 2235 "pass one skipped, but cannot produce a plan 
// in pass two, originated from file ../optimizer/opt.cpp"
NABoolean Join::allowHashJoin()
{
  // HJ is only implementation for full outer join
  NABoolean fullOuterJoin = 
    (CmpCommon::getDefault(COMP_BOOL_196) != DF_ON) AND isFullOuterJoin();

  if (fullOuterJoin) return TRUE;

  // HJ is only join implementation allowed when avoiding halloween update
  if (avoidHalloweenR2()) return TRUE;

  // favor NJ when only one row from outer for sure
  Cardinality minrows, outerLimit; 
  GroupAttributes *oGrpAttr = child(0).getGroupAttr();
  NABoolean hasConstraint = oGrpAttr->hasCardConstraint(minrows, outerLimit);
  // outerLimit was set to INFINITE_CARDINALITY if no constraint
  CostScalar outerRows(outerLimit); 
  // if it has no constraint and outer is 1 table
  if (!hasConstraint && oGrpAttr->getNumBaseTables() == 1) {
    // use max cardinality estimate
    outerRows = oGrpAttr->getResultMaxCardinalityForEmptyInput();
  }
  NABoolean favorNJ = 
    CURRSTMT_OPTDEFAULTS->isNestedJoinConsidered() AND
    (outerRows <= 1) AND
    GlobalRuleSet->getCurrentPassNumber() >
    GlobalRuleSet->getFirstPassNumber();

  if (favorNJ) return FALSE; // disallow HJ

  return TRUE; // allow HJ
}

OperatorTypeEnum Join::getTSJJoinOpType()
{
  switch (getOperatorType())
    {
    case REL_JOIN:
    case REL_ROUTINE_JOIN:
      return REL_TSJ;
    case REL_LEFT_JOIN:
      return REL_LEFT_TSJ;
    case REL_SEMIJOIN:
      return REL_SEMITSJ;
    case REL_ANTI_SEMIJOIN:
      return REL_ANTI_SEMITSJ;
    default:
      ABORT("Unsupported join type in Join::getTSJJoinOpType()");
      return REL_NESTED_JOIN;    // Return makes MSVC happy.
    } // switch
} // Join::getNestedJoinOpType()


OperatorTypeEnum Join::getNestedJoinOpType()
{
  switch (getOperatorType())
    {
    case REL_JOIN:
    case REL_ROUTINE_JOIN:
    case REL_TSJ:
      return REL_NESTED_JOIN;
    case REL_LEFT_JOIN:
    case REL_LEFT_TSJ:
      return REL_LEFT_NESTED_JOIN;
    case REL_SEMIJOIN:
    case REL_SEMITSJ:
      return REL_NESTED_SEMIJOIN;
    case REL_ANTI_SEMIJOIN:
    case REL_ANTI_SEMITSJ:
      return REL_NESTED_ANTI_SEMIJOIN;
    default:
      ABORT("Unsupported join type in Join::getNestedJoinOpType()");
      return REL_NESTED_JOIN;    // Return makes MSVC happy.
    } // switch
} // Join::getNestedJoinOpType()

OperatorTypeEnum Join::getHashJoinOpType(NABoolean isNoOverflow)
{
  if(isNoOverflow)
  {
      switch (getOperatorType())
    {
    case REL_JOIN:
      return REL_ORDERED_HASH_JOIN;
    case REL_LEFT_JOIN:
      return REL_LEFT_ORDERED_HASH_JOIN;
    case REL_SEMIJOIN:
      return REL_ORDERED_HASH_SEMIJOIN;
    case REL_ANTI_SEMIJOIN:
      return REL_ORDERED_HASH_ANTI_SEMIJOIN;
    default:
      ABORT("Unsupported join type in Join::getHashJoinOpType()");
      return REL_ORDERED_HASH_JOIN;     // return makes MSVC happy.
      } // switch
  }
  else
  {

    switch (getOperatorType())
      {
      case REL_JOIN:
	return REL_HYBRID_HASH_JOIN;
      case REL_LEFT_JOIN:
	return REL_LEFT_HYBRID_HASH_JOIN;
      case REL_FULL_JOIN:
	return REL_FULL_HYBRID_HASH_JOIN;
      case REL_SEMIJOIN:
	return REL_HYBRID_HASH_SEMIJOIN;
      case REL_ANTI_SEMIJOIN:
	return REL_HYBRID_HASH_ANTI_SEMIJOIN;
      default:
	ABORT("Unsupported join type in Join::getHashJoinOpType()");
	return REL_HYBRID_HASH_JOIN;     // return makes MSVC happy.
      } // switch
  }//else

} // Join::getHashJoinOpType()

NABoolean Join::isCrossProduct() const
{
  // only for our beloved inner non semi joins (for now)
  if (NOT isInnerNonSemiJoin()) return FALSE;

  ValueIdSet VEGEqPreds;
  ValueIdSet newJoinPreds = getSelectionPred();
  newJoinPreds += getJoinPred();

  // -----------------------------------------------------------------
  // Find all the VEGPreds in the newJoinPreds.
  // Remove all the ones that are not true join predicates
  //   (i.e. they are covered by the inputs, or one of the
  //         children cannot produce it)
  // -----------------------------------------------------------------
  newJoinPreds.lookForVEGPredicates(VEGEqPreds);

  // remove those VEGPredicates that are covered by the input values
  VEGEqPreds.removeCoveredExprs(getGroupAttr()->
				getCharacteristicInputs());
  VEGEqPreds.removeUnCoveredExprs(child(0).getGroupAttr()->
                                  getCharacteristicOutputs());
  VEGEqPreds.removeUnCoveredExprs(child(1).getGroupAttr()->
                                  getCharacteristicOutputs());
  if (VEGEqPreds.isEmpty())
    return TRUE;    // is a cross product
  else
    return FALSE;
}

NABoolean Join::isOuterJoin() const
{
  if(isLeftJoin() || isRightJoin() || isFullOuterJoin())
    return TRUE;

  return FALSE;
}

NABoolean RelExpr::isAnyJoin() const
{
   switch (getOperatorType())
   {
    case REL_JOIN:
    case REL_ROUTINE_JOIN:
    case REL_LEFT_JOIN:
    case REL_ANY_JOIN:
    case REL_ANY_TSJ:
    case REL_ANY_INNER_JOIN:
    case REL_ANY_NON_TS_INNER_JOIN:
    case REL_ANY_NON_TSJ_JOIN:
    case REL_ANY_LEFT_JOIN:
    case REL_ANY_LEFT_TSJ:
    case REL_ANY_NESTED_JOIN:
    case REL_ANY_HASH_JOIN:
    case REL_ANY_MERGE_JOIN:
    case REL_FORCE_JOIN:
    case REL_FORCE_NESTED_JOIN:
    case REL_FORCE_HASH_JOIN:
    case REL_FORCE_ORDERED_HASH_JOIN:
    case REL_FORCE_HYBRID_HASH_JOIN:
    case REL_FORCE_MERGE_JOIN:
    case REL_INTERSECT:
    case REL_EXCEPT:
      return TRUE;

    default:
      return FALSE;
   }
}

NABoolean Join::isInnerNonSemiJoinWithNoPredicates() const
{
  if (isInnerNonSemiJoin() AND
      getSelectionPred().isEmpty() AND
      getJoinPred().isEmpty())
      return TRUE;
  else
    return FALSE;
}


NABoolean Join:: isInnerJoin() const
{
  return getOperator().match(REL_ANY_INNER_JOIN);
}

NABoolean Join::isInnerNonSemiJoin() const
{
  return getOperator().match(REL_ANY_INNER_JOIN) AND
    NOT getOperator().match(REL_ANY_SEMIJOIN);
}

NABoolean Join::isInnerNonSemiNonTSJJoin() const
{
  return (getOperator().match(REL_ANY_INNER_JOIN) AND
    NOT getOperator().match(REL_ANY_SEMIJOIN)) AND
    NOT getOperator().match(REL_ANY_TSJ);
}
NABoolean Join::isLeftJoin() const
{
  return getOperator().match(REL_ANY_LEFT_JOIN);
}

NABoolean Join::isRightJoin() const
{
  return getOperator().match(REL_ANY_RIGHT_JOIN);
}

NABoolean Join::isFullOuterJoin() const
{
  return (getOperator().match(REL_ANY_FULL_JOIN));
}

NABoolean Join::isSemiJoin() const
{
  return getOperator().match(REL_ANY_SEMIJOIN);
}

NABoolean Join::isAntiSemiJoin() const
{
  return getOperator().match(REL_ANY_ANTI_SEMIJOIN);
}

NABoolean Join::isTSJ() const
{
  return getOperator().match(REL_ANY_TSJ);
}

NABoolean Join::isRoutineJoin() const
{
  return (getOperator() == REL_ROUTINE_JOIN);
}

NABoolean Join::isNonRoutineTSJ() const
{
  return ( getOperator().match(REL_ANY_TSJ) && 
          ( getOperator() != REL_ROUTINE_JOIN));
}

NABoolean Join::isNestedJoin() const
{
  return getOperator().match(REL_ANY_NESTED_JOIN);
}

NABoolean Join::isHashJoin() const
{
  return getOperator().match(REL_ANY_HASH_JOIN);
}

OperatorTypeEnum Join::getBaseHashType() const
{
  switch (getOperatorType())
    {
    case REL_HASH_JOIN:
    case REL_LEFT_HASH_JOIN:
    case REL_HASH_SEMIJOIN:
    case REL_HASH_ANTI_SEMIJOIN:
    case REL_ANY_HASH_JOIN:
      return REL_HASH_JOIN;

    case REL_HYBRID_HASH_JOIN:
    case REL_LEFT_HYBRID_HASH_JOIN:
    case REL_FULL_HYBRID_HASH_JOIN:
    case REL_HYBRID_HASH_SEMIJOIN:
    case REL_HYBRID_HASH_ANTI_SEMIJOIN:
      return REL_HYBRID_HASH_JOIN;

    case REL_ORDERED_HASH_JOIN:
    case REL_LEFT_ORDERED_HASH_JOIN:
    case REL_ORDERED_HASH_SEMIJOIN:
    case REL_ORDERED_HASH_ANTI_SEMIJOIN:
      return REL_ORDERED_HASH_JOIN;

    default:
      return INVALID_OPERATOR_TYPE;
    } // switch
} // Join::getBaseHashType

NABoolean Join::isMergeJoin() const
{
  return getOperator().match(REL_ANY_MERGE_JOIN);
}

OperatorTypeEnum Join::getMergeJoinOpType()
{
  switch (getOperatorType())
    {
    case REL_JOIN:
      return REL_MERGE_JOIN;
    case REL_LEFT_JOIN:
      return REL_LEFT_MERGE_JOIN;
    case REL_SEMIJOIN:
      return REL_MERGE_SEMIJOIN;
    case REL_ANTI_SEMIJOIN:
      return REL_MERGE_ANTI_SEMIJOIN;
    default:
      ABORT("Unsupported join type in Join::getMergeJoinOpType()");
      return REL_MERGE_JOIN;  // return makes MSVC happy.
    } // switch
} // Join::getMergeJoinOpType()

void Join::pushdownCoveredExpr(const ValueIdSet & outputExpr,
                               const ValueIdSet & newExternalInputs,
                               ValueIdSet & predicatesOnParent,
			       const ValueIdSet * setOfValuesReqdByParent,
			       Lng32 childIndex
		              )
{
  #ifdef NDEBUG
  NAString pushdownDebugStr(CmpCommon::statementHeap());
    #define PUSHDOWN_DEBUG_SAVE(str)
  #else
    Int32 PUSHDOWN_DEBUG = !!getenv("PUSHDOWN_DEBUG");
    NAString pushdownDebugStr, pushdownDebugTmp;
    #define PUSHDOWN_DEBUG_SAVE(str)					      \
      { if (PUSHDOWN_DEBUG)						      \
	{ pushdownDebugTmp = "";					      \
	  predicatesOnParent.unparse(pushdownDebugTmp);			      \
	  pushdownDebugStr += NAString("\n") + str + ": " + pushdownDebugTmp; \
	  if (PUSHDOWN_DEBUG == 99) cerr << pushdownDebugStr << endl;	      \
	}								      \
      }
    PUSHDOWN_DEBUG_SAVE("J1");
  #endif

  // ----------------------------------------------------------------------
  // Note: Normally, predicatesOnParent is the set of predicates to be
  // considered for pushing down. For most of the other nodes, this set
  // is usually just the set (or maybe a subset) of selection predicates
  // the node has. However, a Join node distinguishes between selection
  // predicates (which are specified in the WHERE clause of a SQL query)
  // and join predicates (which are specified in the ON clause). The two
  // types of predicates have different criteria (as we will see later)
  // to satisfy in order to be pushed down. The predicatesOnParent supplied
  // are treated as selection predicates for the purpose of consideration
  // for pushing down. Note also that this procedure also considers the
  // pushing down of join predicates (as in joinPred_ of Join).
  // ----------------------------------------------------------------------

  // This method only supports pushing predicates down to both children,
  // but not only to one specified.
  //
  CMPASSERT(childIndex < 0);

  NABoolean isATSJFlag = isTSJ();
  ValueIdSet exprOnParent ;
  if (setOfValuesReqdByParent)
    exprOnParent = *setOfValuesReqdByParent;

  if (isFullOuterJoin())
    {
      // For Full Outer Join, we cannot push down the selctionPred()
      // or the  joinPred() to either child0 or child1.
      // Note that for FOJ, predicates are not pulled up.

      // ---------------------------------------------------------------------
      // STEP 1: We cannot pushdown join predicates to either child,
      //         so compute values required to evaluate the joinPred()
      //         here at the parent (the Join) and add
      //         it to exprOnParent.
      //
      // ---------------------------------------------------------------------
      computeValuesReqdForPredicates(joinPred(),
				     exprOnParent, TRUE);

      // ---------------------------------------------------------------------
      // STEP 2: We cannot pushdown selectionPred()  to either child,
      //         so compute values required to evaluate the selectionPred()
      //         here at the parent (the Join) and add
      //         it to exprOnParent.
      //
      // ---------------------------------------------------------------------
      computeValuesReqdForPredicates(selectionPred(),
				     exprOnParent, TRUE);

      // ---------------------------------------------------------------------
      // STEP 3: Calling pushdownCoveredExpr on an empty set, so that the child
      // inputs and outputs are set properly.
      // ---------------------------------------------------------------------
      ValueIdSet emptySet;

       RelExpr::pushdownCoveredExpr(outputExpr,
				   newExternalInputs,
				   emptySet,
				   &exprOnParent,
				   0);

      // ---------------------------------------------------------------------
      // STEP 4: Calling pushdownCoveredExpr on an empty set, so that the child
      // inputs and outputs are set properly.
      // ---------------------------------------------------------------------
      RelExpr::pushdownCoveredExpr(outputExpr,
				   newExternalInputs,
				   emptySet,
				   &exprOnParent,
				   1);

    } // if (isFullOuterJoin())
  // -----------------------------------------------------------------------
  // It might not be obvious, but it turns out that pushing down of
  // predicates in a Left Join is quite similar to that in an Anti-Semi
  // Join. One striking similarity is that the join predicates cannot be
  // pushed down to the left child. In both cases, pushing down join preds
  // filters out rows from the left child which shouldn't be filtered out.
  // In the case of Left Join, those rows should be null-instantiated while
  // in the case of Anti-Semi Join, they are exactly the set of rows which
  // we *should* return. (An Anti-Semi Join returns rows from the left
  // which do *not* join with rows from the right).
  // -----------------------------------------------------------------------
  else if ((isLeftJoin() || isAntiSemiJoin()) && (!isFullOuterJoin()))
    {
      // ---------------------------------------------------------------------
      // STEP 1: Try to push down the given predicatesOnParent to first child.
      // ---------------------------------------------------------------------

    // ---------------------------------------------------------------------
    // STEP 1A: Gather all values the left child must still produce even if
    //          predicates are pushed down.
    //
    // Selection predicates can only be pushed to the first child of an
    // outer join. Join predicates can only be pushed to the second. Make
    // sure the first child produces what we need for the join predicates.
    // ---------------------------------------------------------------------
    computeValuesReqdForPredicates(joinPred(),
                                   exprOnParent);

    // ---------------------------------------------------------------------
    // If this is a TSJ, the left child should also produce those values
    // that the parent needs to give as inputs to the second child.
    // ---------------------------------------------------------------------
    if (isATSJFlag)
         exprOnParent += child(1).getGroupAttr()->getCharacteristicInputs();

    // ---------------------------------------------------------------------
    // This seems to be the only difference between the case of a Left Join
    // and the case of a Anti-Semi Join. For an Anti-Semi Join, selection
    // predicates should be in terms of columns on the left child only (by
    // definition). Therefore, we don't have to worry about retaining things
    // like VEGPred(VEG{T1.a,T2.a}).
    // ---------------------------------------------------------------------
    ValueIdSet VEGEqPreds1;
    if (isLeftJoin())
    {
    // ---------------------------------------------------------------------
    // Find all the VEGPreds in predicatesOnParent. VEGPred(VEG{T1.a,T2.a})
    // will be pushed down to Scan T1 even if T2.a is not available there.
    // Therefore, we still need to keep a copy of this type of predicates
    // here at this Join node where both T1.a and T2.a will be available.
    // ---------------------------------------------------------------------
    predicatesOnParent.lookForVEGPredicates(VEGEqPreds1);

    // ---------------------------------------------------------------------
    // Remove those VEGPreds that are covered by the input values, since
    // VEGPred(VEG{T1.a,3}) needn't be retained at this Join node after it's
    // pushed down to Scan T1.
    // ---------------------------------------------------------------------
    VEGEqPreds1.removeCoveredExprs(newExternalInputs);

    // ---------------------------------------------------------------------
    // Remove those VEGPreds which are not covered at second child. For
    // example VEGPred(VEG{T1.a,T2.a}) in JOIN2 of ((T1 JOIN1 T2) JOIN2 T3)
    // is not covered at the second child. The predicate should be pushed
    // down to the first child without being retained at JOIN2. Note that
    // since predicatesOnParent are selection predicates evaluated after
    // a Left Join, they are in terms of the null-instantiated outputs from
    // the Join rather than direct outputs from the second child.
    // ---------------------------------------------------------------------
    VEGEqPreds1.removeUnCoveredExprs(nullInstantiatedOutput());
    PUSHDOWN_DEBUG_SAVE("J2");

    // ---------------------------------------------------------------------
    // ??? First child not needed ??? since we are trying to push down to
    // child0, if it's uncovered, it wouldn't be pushed down anyway.
    //VEGEqPreds1.removeUnCoveredExprs(
    //                 child(0).getGroupAttr()->getCharacteristicOutputs());
    // ---------------------------------------------------------------------

    // ---------------------------------------------------------------------
    // Since these VEGEqPreds1 will be added back to predicatesOnParent
    // after the attempt to push down to first child, make sure the first
    // child produces the required values to evaluate them.
    // ---------------------------------------------------------------------
    computeValuesReqdForPredicates(VEGEqPreds1,
                                   exprOnParent);
    } // endif (isLeftJoin())

    // ---------------------------------------------------------------------
    // STEP 1B: Perform pushdown to the first child, and add VEGEqPreds
    // back to predicatesOnParent after the push down.
    // ---------------------------------------------------------------------
    RelExpr::pushdownCoveredExpr(outputExpr,
                                 newExternalInputs,
                                 predicatesOnParent,
				 &exprOnParent,
                                 0);

    // ---------------------------------------------------------------------
    // All selection predicates could be pushed to the first child for an
    // Anti-Semi Join should those predicates should involve columns from
    // the second child by definition.
    // ---------------------------------------------------------------------
    if (isAntiSemiJoin())
    {
      CMPASSERT(predicatesOnParent.isEmpty()
		// QSTUFF
		OR getGroupAttr()->isGenericUpdateRoot()
		// QSTUFF
	  );
    }
    else
      predicatesOnParent += VEGEqPreds1;
    PUSHDOWN_DEBUG_SAVE("J3");

    // ---------------------------------------------------------------------
    // STEP 2: Try to push down the join predicates to second child.
    // ---------------------------------------------------------------------

    // ---------------------------------------------------------------------
    // STEP 2A: Gather all values the second child must still produce even
    //          if predicates are pushed down. Start with all the required
    //          values specified by the caller of this method.
    // ---------------------------------------------------------------------
    if (setOfValuesReqdByParent)
      exprOnParent = *setOfValuesReqdByParent;
    else exprOnParent.clear();

    // ---------------------------------------------------------------------
    // Since the remaining predicatesOnParent could not be pushed down to
    // the second child and must be evaluated on the Left Join, values reqd
    // for their evaluation must be included to make sure the second child
    // produces them. For Anti-Semi Join, predicatesOnParent is empty.
    // ---------------------------------------------------------------------
    ValueIdSet inputs = newExternalInputs;
    ValueIdSet inputsTakenOut;
    if (isLeftJoin())
    {
      computeValuesReqdForPredicates(predicatesOnParent,
                                     exprOnParent);

      // -------------------------------------------------------------------
      // Special case: If this left join is the right child of a Nested
      // Join, it could happen that the inputs we get from above already
      // contains a value which our push down logic considers to have
      // covered the predicatesOnParent which we don't attempt to push
      // down. This is a very peculiar failure of our cover logic, where
      // the predicates should have been pushed down but held back cos of
      // the semantics of an operator (case in point, the left join). To
      // deal with this, we remove from the available inputs to my child
      // those values so that the child will produce this as an output.
      // -------------------------------------------------------------------
      // inputTakenOut = inputs;
      // predicatesOnParent.weedOutUnreferenced(inputsTakenOut);
      // inputs -= inputsTakenOut;
    }

    // ---------------------------------------------------------------------
    // Also, if this is NOT a TSJ, there are some join predicates which need
    // to be retained even if they are pushed down to the second child. All
    // Join predicates are pushable to the second child of a TSJ without
    // being retained at the TSJ. (See later for an exception)
    // ---------------------------------------------------------------------
    ValueIdSet VEGEqPreds2;
    ValueIdSet availableInputs = inputs;
    if (isATSJFlag)
    {
      availableInputs += child(0).getGroupAttr()->getCharacteristicOutputs();
    }
    else
    {
      // -------------------------------------------------------------------
      // First, find all the VEGPreds in join predicates. This is similar to
      // what we did above with predicatesOnParent. VEGPred(VEG{T1.a,T2.a})
      // will be pushed down to Scan T2 even if T1.a is not available there.
      // Therefore, we still need to keep a copy of this type of predicates
      // here at this Join node where both T1.a and T2.a will be available.
      // -------------------------------------------------------------------
      joinPred().lookForVEGPredicates(VEGEqPreds2);

      // -------------------------------------------------------------------
      // Remove those VEGPreds that are covered by the input values, since
      // VEGPred(VEG{T2.a,3}) needn't be retained at this Join node after
      // pushed down to Scan T2. (There is an exception to this. See later.)
      // -------------------------------------------------------------------
      VEGEqPreds2.removeCoveredExprs(availableInputs); //newExternalInputs

      // -------------------------------------------------------------------
      // Remove those VEGPreds which are not covered at first child. For
      // example VEGPred(VEG{T2.a,T3.a}) in JOIN1 of (T1 JOIN1 (T2 JOIN2 T3))
      // is not covered at the first child. The predicate could be pushed
      // down to the second child without being retained at JOIN2.
      // -------------------------------------------------------------------
      VEGEqPreds2.removeUnCoveredExprs(
                       child(0).getGroupAttr()->getCharacteristicOutputs());

      // -------------------------------------------------------------------
      // Since these predicates will be added back to the join predicates
      // after the attempt to push down to second child, make sure the second
      // child produces the required values to evaluate them.
      // -------------------------------------------------------------------
      computeValuesReqdForPredicates(VEGEqPreds2,
                                     exprOnParent);
    }

    // ---------------------------------------------------------------------
    // Now, there are additional join predicates that must be retained
    // even if they are pushable to the second child. An example would be
    // VEGPred(VEG{T1.a,T2.a,10}). For an inner join, this predicate can
    // be pushed to Scan T1 and Scan T2 and evaluated as (T1.a=10) and
    // (T2.a=10) respectively. However, for a Left Join or Anti-Semi Join,
    // this predicate (if it's a join predicate) cannot be pushed down to
    // the first child. The (T1.a=10) part must then be retained at this
    // Join node. These types of VEGPreds are those covered by T1 and the
    // external inputs.
    // ---------------------------------------------------------------------
    ValueIdSet joinPredsThatStay;
    joinPredsThatStay = joinPred();
    ValueIdSet availableValues = availableInputs; //newExternalInputs
    availableValues += child(0).getGroupAttr()->getCharacteristicOutputs();
    joinPredsThatStay.removeUnCoveredExprs(availableValues);

    // ---------------------------------------------------------------------
    // However, we don't want VEGPred like VEGPred(VEG{T2.a,10}) which
    // actually does not reference an output of T1.
    // ---------------------------------------------------------------------
    joinPredsThatStay.removeUnReferencedVEGPreds(
                       child(0).getGroupAttr()->getCharacteristicOutputs());

    // ---------------------------------------------------------------------
    // Also, if some inputs have been taken out deliberately, we want to
    // make sure other predicates which references the inputs taken out
    // are going to stay. Otherwise, we will have the issue that not
    // sufficient values are available at the child to ensure correctness
    // in evaluating the predicates pushed down to it. The same predicate
    // must be re-evaluated at this JOIN node.
    // ---------------------------------------------------------------------
    if (NOT inputsTakenOut.isEmpty())
    {
      ValueIdSet moreJoinPredsThatStay;
      joinPred().lookForVEGPredicates(moreJoinPredsThatStay);
      moreJoinPredsThatStay.removeUnReferencedVEGPreds(inputsTakenOut);
      joinPredsThatStay += moreJoinPredsThatStay;
    }

    // ---------------------------------------------------------------------
    // Since these predicates will be added back to the join predicates
    // after the attempt to push down to second child, make sure the second
    // child produces the required values to evaluate them.
    // ---------------------------------------------------------------------
    computeValuesReqdForPredicates(joinPredsThatStay,
                                   exprOnParent);

    //----------------------------------------------------------------------
    // Solution 10-030728-8252: check if the second child could produce
    // expressions of type Instnull(CAST(aggregate)).
    // See if the CAST could be pushed
    // up. The Groupby node does not manufacture expressions of the type
    // cast(aggregate) as outputs in the generator. So do not ask for them
    //----------------------------------------------------------------------
    exprOnParent.replaceInstnullCastAggregateWithAggregateInLeftJoins(this);

    // ---------------------------------------------------------------------
    // STEP 2B: Perform pushdown to the second child, and add reqd preds
    // back to the join predicates after the push down.
    // ---------------------------------------------------------------------
    RelExpr::pushdownCoveredExpr(outputExpr,
                                 availableInputs,
                                 joinPred(),
				 &exprOnParent,
                                 1);


    // ---------------------------------------------------------------------
    // Add back those predicates which must stay with the JOIN even after
    // they are pushed to the second child.
    // ---------------------------------------------------------------------
    joinPred() += VEGEqPreds2;
    joinPred() += joinPredsThatStay;
    PUSHDOWN_DEBUG_SAVE("J4");

  }
  else
  // -----------------------------------------------------------------------
  // For other types of Join's: Semi and Inner (either TSJ or non-TSJ),
  // processing is quite similar. Inner Joins has no join prediciates. For
  // Semi-Join, although we distinguish between join predicates and
  // selection predicates, both types of predicates are equally pushable.
  // The only thing is "true join VEGPreds" like VEGPred(VEG{T1.a,T2.a})
  // should be retained as join predicates in a Semi-Join but as selection
  // predicates in an Inner Join after being pushed down.
  // -----------------------------------------------------------------------
  {
    ValueIdSet predicates1 = predicatesOnParent;
    ValueIdSet predicates2 = predicatesOnParent;
    if (isSemiJoin())
    {
      // Join predicates in a Semi-Join are "as pushable as" its selection
      // predicates.
      //
      predicates1 += joinPred();
      predicates2 += joinPred();
    }
    else
    {
      // Inner Join should have no join predicates.
      CMPASSERT(joinPred().isEmpty()
		// QSTUFF
		OR getGroupAttr()->isGenericUpdateRoot()
		// QSTUFF
	  );
    }

    // ---------------------------------------------------------------------
    // STEP 1: Gather all values the children must still produce even if
    //         predicates are pushed down.
    //
    // Find all the "true join VEGPreds" in predicates. E.g, VEGPred(VEG{
    // T1.a,T2.a}) will be pushed down to Scan T1 and to Scan T2 even if
    // not both values are availble at either node. Therefore, we still
    // need to keep a copy of this type of predicates here at this Join node
    // where both T1.a and T2.a will be available. That means the children
    // need to provide these values to the Join node. The only exception is
    // when we are doing a TSJ. The predicates are then all pushed to the
    // right child, and the right child could then *not* provide the value
    // to the Join node if it's not a required output from the Join.
    // ---------------------------------------------------------------------
    ValueIdSet VEGEqPreds;
    predicates1.lookForVEGPredicates(VEGEqPreds);

    // ---------------------------------------------------------------------
    // Remove those VEGPreds that are covered by the input values, since
    // VEGPred(VEG{T1.a,3}) needn't be retained at this Join node after
    // it's pushed down to Scan T1.
    // ---------------------------------------------------------------------
    VEGEqPreds.removeCoveredExprs(newExternalInputs);

    // ---------------------------------------------------------------------
    // Remove those VEGPreds which are not covered at first child. For
    // example VEGPred(VEG{T2.a,T3.a}) in JOIN1 of (T1 JOIN1 (T2 JOIN2 T3))
    // is not covered at the first child. The predicate could be pushed
    // down to the second child without being retained at JOIN2.
    // ---------------------------------------------------------------------
    VEGEqPreds.removeUnCoveredExprs(
		       child(0).getGroupAttr()->getCharacteristicOutputs());

    // ---------------------------------------------------------------------
    // Remove those VEGPreds which are not covered at second child. For
    // example VEGPred(VEG{T1.a,T2.a}) in JOIN2 of ((T1 JOIN1 T2) JOIN2 T3)
    // is not covered at the second child. The predicate could be pushed
    // down to the first child without being retained at JOIN2.
    // ---------------------------------------------------------------------
    VEGEqPreds.removeUnCoveredExprs(
                       child(1).getGroupAttr()->getCharacteristicOutputs());

    // ---------------------------------------------------------------------
    // Since these predicates will be retained at the Join (or pushed down
    // to the second child in the case of a TSJ), make sure the first
    // child produces the required values to evaluate them.
    // ---------------------------------------------------------------------
    computeValuesReqdForPredicates(VEGEqPreds,
                                   exprOnParent);

    // ---------------------------------------------------------------------
    // First child of a TSJ should produce inputs required from the second
    // child as well.
    // ---------------------------------------------------------------------
    if (isATSJFlag)
       exprOnParent += child(1).getGroupAttr()->getCharacteristicInputs();

    // ---------------------------------------------------------------------
    // STEP 2: Try pushing down to the first child.
    // ---------------------------------------------------------------------
    RelExpr::pushdownCoveredExpr(outputExpr,
                                 newExternalInputs,
                                 predicates1,
				 &exprOnParent,
                                 0);
    PUSHDOWN_DEBUG_SAVE("J5");

    // ---------------------------------------------------------------------
    // Find subset of predicatesOnParent which have *not* been pushed down
    // to first child.
    // ---------------------------------------------------------------------
    predicatesOnParent.intersectSet(predicates1);
    PUSHDOWN_DEBUG_SAVE("J6");

    // ---------------------------------------------------------------------
    // For a Semi-Join, all selection predicates (which should not involve
    // columns from the second child) should be pushed down to the first
    // child by now. Also get rid of the join predicates which have been
    // pushed down.
    // ---------------------------------------------------------------------
    if (isSemiJoin())
    {
      joinPred().intersectSet(predicates1);
      CMPASSERT(predicatesOnParent.isEmpty()
		// QSTUFF
		OR getGroupAttr()->isGenericUpdateRoot()
		// QSTUFF
	  );
    }

    // ---------------------------------------------------------------------
    // If this is a TSJ, we do not even need to retain VEGEqPreds at the
    // Join. Everything remaining should be pushable to the right child.
    // Therefore, we don't need the right child to output values required
    // for evaluating VEGEqPreds, unless it's an required output from the
    // 
    // We do not want to push the predicate down to the right child now for
    // the RoutineJoin. That will happen later when the RoutineJoin gets 
    // transfered back to a TSJ/nested join by the optimizer impl rules.
    // 
    // The reason we don't want it pushed here is so that the analyzer does
    // not have to differentiate what imputs are required for predicates and
    // which is required for a UDF. By knowing what inputs are required for
    // the UDF, the analyzer can determine if there is a different join order
    // that might be cheaper. We will attempt to push the predicate during
    // the optimizer phase..
    // ---------------------------------------------------------------------
    if (!isRoutineJoin())
    {
       ValueIdSet availableInputs = newExternalInputs;
       if (isATSJFlag)
       {
         if (setOfValuesReqdByParent)
	   exprOnParent = *setOfValuesReqdByParent;
         else exprOnParent.clear();
         availableInputs += child(0).getGroupAttr()->getCharacteristicOutputs();
       }

       // ---------------------------------------------------------------------
       // STEP 3: Try pushing to second child now.
       // ---------------------------------------------------------------------
       RelExpr::pushdownCoveredExpr(outputExpr,
                                 availableInputs,
                                 predicates2,
				 &exprOnParent,
                                 1);
    }

    // ---------------------------------------------------------------------
    // Find subset of predicatesOnParent which have *not* been pushed down
    // to second child.
    // ---------------------------------------------------------------------
    predicatesOnParent.intersectSet(predicates2);
    PUSHDOWN_DEBUG_SAVE("J7");

    if (isSemiJoin())
    {
      // -------------------------------------------------------------------
      // set joinPred to have those predicates that were not pushed down to
      // the second child.
      // -------------------------------------------------------------------
      joinPred().intersectSet(predicates2);

      // -------------------------------------------------------------------
      // If this is a semi-join that is not a TSJ we need to add all the
      // true join VEGPreds back to joinPred().
      // -------------------------------------------------------------------
      if (NOT isATSJFlag)
        joinPred() += VEGEqPreds;
      else
        // -----------------------------------------------------------------
        // If it is a TSJ all join predicates should be pushable, no preds
        // should be remaining in joinPred().
        // -----------------------------------------------------------------
        CMPASSERT(joinPred().isEmpty()
		  // QSTUFF
		  OR getGroupAttr()->isGenericUpdateRoot()
		  // QSTUFF
		);
    }
    else
    {
      // -------------------------------------------------------------------
      // If this is a inner-join that is not a TSJ we need to add all the
      // true join VEGPreds back to selection predicates.
      // -------------------------------------------------------------------
      if (NOT isATSJFlag OR isRoutineJoin())
      {
        predicatesOnParent += VEGEqPreds;
        PUSHDOWN_DEBUG_SAVE("J9");
      }
      else
        // -----------------------------------------------------------------
        // If it is a TSJ all selection predicates should be pushable, no
        // preds should remain.
        // -----------------------------------------------------------------
        CMPASSERT(predicatesOnParent.isEmpty()
		  // QSTUFF
		  OR getGroupAttr()->isGenericUpdateRoot()
		  // QSTUFF
		);
    }
  }
} // Join::pushdownCoveredExpr

// --------------------------------------------------------------------------
// Join::pushdownCoveredExprSQO
// Rules for pushdown from Join during the SemanticQueryOptimize(SQO)
// subphase are different in two ways from the usual. 
// 1) If left child does not cover any part of a 
// VEGPred it will still be retained in the Join, so that it can be pulled
// further up the query tree as we apply this transformation at other levels
// In the usual rules, the VEGPred will be pushed down to the right child
// without being retained at the Join. This behaviour is controlled by the 
// boolean input parameter keepPredsNotCoveredByChild0. Similarly preds not 
// covered by the right child can also be retained at the Join. This is 
// controlled by keepPredsNotCoveredByChild1.
// 2) If left child is a semiJoin or a TSJ we do not push any predicates 
// down that side as those selection predicates are supposed to be empty 
// at this phase of compilation.
// ---------------------------------------------------------------------------
void Join::pushdownCoveredExprSQO(const ValueIdSet & outputExpr,
                               const ValueIdSet & newExternalInputs,
                               ValueIdSet & predicatesOnParent,
			       ValueIdSet & setOfValuesReqdByParent,
			       NABoolean keepPredsNotCoveredByChild0,
			       NABoolean keepPredsNotCoveredByChild1
		              )
{

  ValueIdSet exprOnParent1 = setOfValuesReqdByParent;
  ValueIdSet exprOnParent  = setOfValuesReqdByParent;
  ValueIdSet exprOnParent2 = setOfValuesReqdByParent;

 
  ValueIdSet predicates1 = predicatesOnParent;
  ValueIdSet predicates2 = predicatesOnParent;
  
  if (isLeftJoin())
    {
      // ---------------------------------------------------------------------
      // STEP 1: Try to push down the given predicatesOnParent to first child.
      // ---------------------------------------------------------------------

    // ---------------------------------------------------------------------
    // STEP 1A: Gather all values the left child must still produce even if
    //          predicates are pushed down.
    //
    // Selection predicates can only be pushed to the first child of an
    // outer join. Join predicates can only be pushed to the second. Make
    // sure the first child produces what we need for the join predicates.
    // ---------------------------------------------------------------------
    computeValuesReqdForPredicates(joinPred(),
                                   exprOnParent);


    ValueIdSet VEGEqPreds1;

    // ---------------------------------------------------------------------
    // Find all the VEGPreds in predicatesOnParent. VEGPred(VEG{T1.a,T2.a})
    // will be pushed down to Scan T1 even if T2.a is not available there.
    // Therefore, we still need to keep a copy of this type of predicates
    // here at this Join node where both T1.a and T2.a will be available.
    // ---------------------------------------------------------------------
    predicatesOnParent.lookForVEGPredicates(VEGEqPreds1);

    // ---------------------------------------------------------------------
    // Remove those VEGPreds that are covered by the input values, since
    // VEGPred(VEG{T1.a,3}) needn't be retained at this Join node after it's
    // pushed down to Scan T1.
    // ---------------------------------------------------------------------
    VEGEqPreds1.removeCoveredExprs(newExternalInputs);

    // ---------------------------------------------------------------------
    // Remove those VEGPreds which are not covered at second child. For
    // example VEGPred(VEG{T1.a,T2.a}) in JOIN2 of ((T1 JOIN1 T2) JOIN2 T3)
    // is not covered at the second child. The predicate should be pushed
    // down to the first child without being retained at JOIN2. Note that
    // since predicatesOnParent are selection predicates evaluated after
    // a Left Join, they are in terms of the null-instantiated outputs from
    // the Join rather than direct outputs from the second child.
    // ---------------------------------------------------------------------
    if (NOT keepPredsNotCoveredByChild1)
      VEGEqPreds1.removeUnCoveredExprs(nullInstantiatedOutput());

    // ---------------------------------------------------------------------
    // Since these VEGEqPreds1 will be added back to predicatesOnParent
    // after the attempt to push down to first child, make sure the first
    // child produces the required values to evaluate them.
    // ---------------------------------------------------------------------
    computeValuesReqdForPredicates(VEGEqPreds1,
                                   exprOnParent);

    // ---------------------------------------------------------------------
    // STEP 1B: Perform pushdown to the first child, and add VEGEqPreds
    // back to predicatesOnParent after the push down.
    // ---------------------------------------------------------------------
    RelExpr::pushdownCoveredExpr(outputExpr,
				  newExternalInputs,
				  predicatesOnParent,
				  &exprOnParent,
				  0);


    // ---------------------------------------------------------------------
    // All selection predicates could be pushed to the first child for an
    // Anti-Semi Join should those predicates should involve columns from
    // the second child by definition.
    // ---------------------------------------------------------------------

    predicatesOnParent += VEGEqPreds1;

    // ---------------------------------------------------------------------
    // STEP 2: Try to push down the join predicates to second child.
    // ---------------------------------------------------------------------

    // ---------------------------------------------------------------------
    // STEP 2A: Gather all values the second child must still produce even
    //          if predicates are pushed down. Start with all the required
    //          values specified by the caller of this method.
    // ---------------------------------------------------------------------
    exprOnParent = outputExpr;

    // ---------------------------------------------------------------------
    // Since the remaining predicatesOnParent could not be pushed down to
    // the second child and must be evaluated on the Left Join, values reqd
    // for their evaluation must be included to make sure the second child
    // produces them. For Anti-Semi Join, predicatesOnParent is empty.
    // ---------------------------------------------------------------------
    ValueIdSet inputs = newExternalInputs;
    ValueIdSet inputsTakenOut;
    computeValuesReqdForPredicates(predicatesOnParent,
                                     exprOnParent);


    // ---------------------------------------------------------------------
    // Also, if this is NOT a TSJ, there are some join predicates which need
    // to be retained even if they are pushed down to the second child. All
    // Join predicates are pushable to the second child of a TSJ without
    // being retained at the TSJ. (See later for an exception)
    // ---------------------------------------------------------------------
    ValueIdSet VEGEqPreds2;
    ValueIdSet availableInputs = inputs;
      // -------------------------------------------------------------------
      // First, find all the VEGPreds in join predicates. This is similar to
      // what we did above with predicatesOnParent. VEGPred(VEG{T1.a,T2.a})
      // will be pushed down to Scan T2 even if T1.a is not available there.
      // Therefore, we still need to keep a copy of this type of predicates
      // here at this Join node where both T1.a and T2.a will be available.
      // -------------------------------------------------------------------
      joinPred().lookForVEGPredicates(VEGEqPreds2);

      // -------------------------------------------------------------------
      // Remove those VEGPreds that are covered by the input values, since
      // VEGPred(VEG{T2.a,3}) needn't be retained at this Join node after
      // pushed down to Scan T2. (There is an exception to this. See later.)
      // -------------------------------------------------------------------
      VEGEqPreds2.removeCoveredExprs(availableInputs); //newExternalInputs

      // -------------------------------------------------------------------
      // Remove those VEGPreds which are not covered at first child. For
      // example VEGPred(VEG{T2.a,T3.a}) in JOIN1 of (T1 JOIN1 (T2 JOIN2 T3))
      // is not covered at the first child. The predicate could be pushed
      // down to the second child without being retained at JOIN2.
      // -------------------------------------------------------------------
      if (NOT keepPredsNotCoveredByChild0)
        VEGEqPreds2.removeUnCoveredExprs(
                       child(0).getGroupAttr()->getCharacteristicOutputs());

      // -------------------------------------------------------------------
      // Since these predicates will be added back to the join predicates
      // after the attempt to push down to second child, make sure the second
      // child produces the required values to evaluate them.
      // -------------------------------------------------------------------
      computeValuesReqdForPredicates(VEGEqPreds2,
                                     exprOnParent);
    

    // ---------------------------------------------------------------------
    // Now, there are additional join predicates that must be retained
    // even if they are pushable to the second child. An example would be
    // VEGPred(VEG{T1.a,T2.a,10}). For an inner join, this predicate can
    // be pushed to Scan T1 and Scan T2 and evaluated as (T1.a=10) and
    // (T2.a=10) respectively. However, for a Left Join or Anti-Semi Join,
    // this predicate (if it's a join predicate) cannot be pushed down to
    // the first child. The (T1.a=10) part must then be retained at this
    // Join node. These types of VEGPreds are those covered by T1 and the
    // external inputs.
    // ---------------------------------------------------------------------
    ValueIdSet joinPredsThatStay;
    joinPredsThatStay = joinPred();
    ValueIdSet availableValues = availableInputs; //newExternalInputs
    availableValues += child(0).getGroupAttr()->getCharacteristicOutputs();
    joinPredsThatStay.removeUnCoveredExprs(availableValues);

    // ---------------------------------------------------------------------
    // However, we don't want VEGPred like VEGPred(VEG{T2.a,10}) which
    // actually does not reference an output of T1.
    // ---------------------------------------------------------------------
    if (NOT keepPredsNotCoveredByChild0)
      joinPredsThatStay.removeUnReferencedVEGPreds(
                       child(0).getGroupAttr()->getCharacteristicOutputs());

    // ---------------------------------------------------------------------
    // Also, if some inputs have been taken out deliberately, we want to
    // make sure other predicates which references the inputs taken out
    // are going to stay. Otherwise, we will have the issue that not
    // sufficient values are available at the child to ensure correctness
    // in evaluating the predicates pushed down to it. The same predicate
    // must be re-evaluated at this JOIN node.
    // ---------------------------------------------------------------------
    if (NOT inputsTakenOut.isEmpty())
    {
      ValueIdSet moreJoinPredsThatStay;
      joinPred().lookForVEGPredicates(moreJoinPredsThatStay);
      moreJoinPredsThatStay.removeUnReferencedVEGPreds(inputsTakenOut);
      joinPredsThatStay += moreJoinPredsThatStay;
    }

    // ---------------------------------------------------------------------
    // Since these predicates will be added back to the join predicates
    // after the attempt to push down to second child, make sure the second
    // child produces the required values to evaluate them.
    // ---------------------------------------------------------------------
    computeValuesReqdForPredicates(joinPredsThatStay,
                                   exprOnParent);

    //----------------------------------------------------------------------
    // Solution 10-030728-8252: check if the second child could produce
    // expressions of type Instnull(CAST(aggregate)).
    // See if the CAST could be pushed
    // up. The Groupby node does not manufacture expressions of the type
    // cast(aggregate) as outputs in the generator. So do not ask for them
    //----------------------------------------------------------------------
    exprOnParent.replaceInstnullCastAggregateWithAggregateInLeftJoins(this);

    // ---------------------------------------------------------------------
    // STEP 2B: Perform pushdown to the second child, and add reqd preds
    // back to the join predicates after the push down.
    // ---------------------------------------------------------------------
     RelExpr::pushdownCoveredExpr(outputExpr,
				  availableInputs,
				  joinPred(),
				  &exprOnParent,
				  1);
    // ---------------------------------------------------------------------
    // Add back those predicates which must stay with the JOIN even after
    // they are pushed to the second child.
    // ---------------------------------------------------------------------
    joinPred() += VEGEqPreds2;
    joinPred() += joinPredsThatStay;

  } 
  else 
  {
  // STEP 1: Gather all values the children must still produce even if
  //         predicates are pushed down.
  //
  // Find all the "true join VEGPreds" in predicates. E.g, VEGPred(VEG{
  // T1.a,T2.a}) will be pushed down to Scan T1 and to Scan T2 even if
  // not both values are availble at either node. Therefore, we still
  // need to keep a copy of this type of predicates here at this Join node
  // where both T1.a and T2.a will be available. That means the children
  // need to provide these values to the Join node. The only exception is
  // when we are doing a TSJ. The predicates are then all pushed to the
  // right child, and the right child could then *not* provide the value
  // to the Join node if it's not a required output from the Join.
  // ---------------------------------------------------------------------
  ValueIdSet VEGEqPreds;
  predicates1.lookForVEGPredicates(VEGEqPreds);

  // ---------------------------------------------------------------------
  // Remove those VEGPreds that are covered by the input values, since
  // VEGPred(VEG{T1.a,3}) needn't be retained at this Join node after
  // it's pushed down to Scan T1.
  // ---------------------------------------------------------------------
  VEGEqPreds.removeCoveredExprs(newExternalInputs);

  // ---------------------------------------------------------------------
  // Remove those VEGPreds which are not covered at first child. For
  // example VEGPred(VEG{T2.a,T3.a}) in JOIN1 of (T1 JOIN1 (T2 JOIN2 T3))
  // is not covered at the first child. The predicate could be pushed
  // down to the second child without being retained at JOIN2.
  // ---------------------------------------------------------------------
  if (NOT keepPredsNotCoveredByChild0)
    VEGEqPreds.removeUnCoveredExprs(
		       child(0).getGroupAttr()->getCharacteristicOutputs());

  // ---------------------------------------------------------------------
  // Remove those VEGPreds which are not covered at second child. For
  // example VEGPred(VEG{T1.a,T2.a}) in JOIN2 of ((T1 JOIN1 T2) JOIN2 T3)
  // is not covered at the second child. The predicate could be pushed
  // down to the first child without being retained at JOIN2.
  // ---------------------------------------------------------------------
  if (NOT keepPredsNotCoveredByChild1)
    VEGEqPreds.removeUnCoveredExprs(
                     child(1).getGroupAttr()->getCharacteristicOutputs());

  // ---------------------------------------------------------------------
  // Since these predicates will be retained at the Join (or pushed down
  // to the second child in the case of a TSJ), make sure the first
  // child produces the required values to evaluate them.
  // ---------------------------------------------------------------------

  computeValuesReqdForPredicates(VEGEqPreds,
                                 exprOnParent);

  // ---------------------------------------------------------------------
  // STEP 2: Try pushing down to the first child.
  // ---------------------------------------------------------------------

   
    if (child(0).getPtr()->getOperator().match(REL_ANY_SEMIJOIN) ||
	child(0).getPtr()->getOperator().match(REL_ANY_TSJ))
    {
      computeValuesReqdForPredicates(predicates1,
				     exprOnParent1);
      ValueIdSet emptySet;
      RelExpr::pushdownCoveredExpr(outputExpr,
				   newExternalInputs,
				   emptySet,
				   &exprOnParent1,
				   0);
    }
    else
    {
      RelExpr::pushdownCoveredExpr(outputExpr,
				   newExternalInputs,
				   predicates1,
				   &exprOnParent,
				   0);
    }


  // ---------------------------------------------------------------------
  // Find subset of predicatesOnParent which have *not* been pushed down
  // to first child.
  // ---------------------------------------------------------------------
  predicatesOnParent.intersectSet(predicates1);


  // ---------------------------------------------------------------------
  // STEP 3: Try pushing to second child now.
  // ---------------------------------------------------------------------

    if (child(1).getPtr()->getOperator().match(REL_ANY_SEMIJOIN) ||
        (child(1).getPtr()->getOperator().match(REL_ANY_TSJ) &&
        (child(1).getPtr()->getOperator() != REL_ROUTINE_JOIN)))
    {
      computeValuesReqdForPredicates(predicates2,
				     exprOnParent2);
      ValueIdSet emptySet;
      RelExpr::pushdownCoveredExpr(outputExpr,
				   newExternalInputs,
				   emptySet,
				   &exprOnParent2,
				   1);
    }
    else
    {
      // We do not want to push predicates to the right child of a 
      // routineJoin.
      if (!isRoutineJoin())
      {
        RelExpr::pushdownCoveredExpr(outputExpr,
                                     newExternalInputs,
                                     predicates2,
                                     &exprOnParent,
                                     1);
      }
    }

  // ---------------------------------------------------------------------
  // Find subset of predicatesOnParent which have *not* been pushed down
  // to second child.
  // ---------------------------------------------------------------------
  predicatesOnParent.intersectSet(predicates2);

  
  // -------------------------------------------------------------------
  // If this is a inner-join that is not a TSJ we need to add all the
  // true join VEGPreds back to selection predicates.
  // -------------------------------------------------------------------
  
  predicatesOnParent += VEGEqPreds;
  }

} // Join::pushdownCoveredExprSQO


void Join::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  switch (getOperatorType())
    {
    case REL_JOIN:
    case REL_MERGE_JOIN:
    case REL_NESTED_JOIN:
    case REL_HASH_JOIN:
    case REL_HYBRID_HASH_JOIN:
    case REL_ORDERED_HASH_JOIN:
    case REL_INDEX_JOIN:
    case REL_ROUTINE_JOIN:
    case REL_TSJ:
      {
	// Potentially, all the values that are produced by
	// my left child as well as my right child.
	outputValues += child(0).getGroupAttr()->getCharacteristicOutputs();
	outputValues += child(1).getGroupAttr()->getCharacteristicOutputs();
	break;
      }
    case REL_LEFT_JOIN:
    case REL_LEFT_NESTED_JOIN:
    case REL_LEFT_MERGE_JOIN:
    case REL_LEFT_ORDERED_HASH_JOIN:
    case REL_LEFT_HYBRID_HASH_JOIN:
    case REL_LEFT_TSJ:
      {
	// Potentially, all the values that are produced by
	// my left child and all null instantiated values from
	// my right child.
	outputValues += child(0).getGroupAttr()->getCharacteristicOutputs();
	outputValues.insertList(nullInstantiatedOutput());
 	break;
      }
    case REL_FULL_JOIN:
    case REL_UNION_JOIN:
    case REL_FULL_HYBRID_HASH_JOIN:
      {
	// Potentially, all the values that are produced by
	// my left child and the right child. Since it's a FULL_OUTER_JOIN
	// all null instantiated values from my right and left child.

	outputValues.insertList(nullInstantiatedOutput());
	outputValues.insertList(nullInstantiatedForRightJoinOutput());
	break;
      }
    case REL_SEMIJOIN:
    case REL_ANTI_SEMIJOIN:
    case REL_SEMITSJ:
    case REL_ANTI_SEMITSJ:
    case REL_HASH_SEMIJOIN:
    case REL_HASH_ANTI_SEMIJOIN:
    case REL_MERGE_SEMIJOIN:
    case REL_MERGE_ANTI_SEMIJOIN:
    case REL_HYBRID_HASH_SEMIJOIN:
    case REL_HYBRID_HASH_ANTI_SEMIJOIN:
    case REL_ORDERED_HASH_SEMIJOIN:
    case REL_ORDERED_HASH_ANTI_SEMIJOIN:
    case REL_NESTED_SEMIJOIN:
    case REL_NESTED_ANTI_SEMIJOIN:
      {
	// No value from my right child can appear in my output.
	outputValues += child(0).getGroupAttr()->getCharacteristicOutputs();
	break;
      }
    case REL_TSJ_FLOW:
    case REL_NESTED_JOIN_FLOW:
      {
	// No value from my left child can appear in my output.
	outputValues += child(1).getGroupAttr()->getCharacteristicOutputs();
	break;
      }
    default:
      {
	ABORT("Unsupported join type in Join::getPotentialOutputValues()");
	break;
      }
    } // switch
} // Join::getPotentialOutputValues()

CostScalar Join::computeMinEstRCForGroup()
{
  CostScalar minCard =  csOne;

  GroupAttributes * ga = getGroupAttr();

  RelExpr * logExpr = ga->getLogExprForSynthesis();
  if (logExpr != NULL)
  {
    logExpr->finishSynthEstLogProp();
    minCard = ga->getMinChildEstRowCount();
  }

  return minCard;
}

// get the highest reduction from local predicates for cols of this join
CostScalar
Join::highestReductionForCols(ValueIdSet colSet) 
{
  // if the child is anything other than scan, then we assume the reduction to be 1
  // but before that we still need to see if the column set that we are looking for 
  // belongs to this child or not.
  // since we don't know to which child tableOne belongs, we shall look at both left 
  // and right histograms for the columns. Start with the left child
  ColStatDescList completeList = child(0).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->colStats();
  ColStatDescList rightColStatList = child(1).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->colStats();

  // form a complete list of histograms from both sides
  completeList.makeDeepCopy(rightColStatList);
  // Compute reduction for this column set
  CostScalar highestUecRedByLocalPreds = highestUecRedByLocalPreds = completeList.getHighestUecReductionByLocalPreds(colSet);
  return highestUecRedByLocalPreds;
}

const NAString Join::getText() const
{
  NAString result;

  switch (getOperatorType())
    {
    case REL_JOIN:
      result += "join";
      break;
    case REL_LEFT_JOIN:
      result += "left_join";
      break;
    case REL_RIGHT_JOIN:
      result += "right_join";
      break;
    case REL_FULL_JOIN:
      result += "full_join";
      break;
    case REL_UNION_JOIN:
      result += "union_join";
      break;
    case REL_ROUTINE_JOIN:
      result += "routine_join";
      break;
    case REL_TSJ:
      result += "tsj";
      break;
    case REL_TSJ_FLOW:
      result += "tsj_flow";
      break;
    case REL_LEFT_TSJ:
      result += "left_tsj";
      break;
    case REL_SEMIJOIN:
      result += "semi_join";
      break;
    case REL_ANTI_SEMIJOIN:
      result += "anti_semi_join";
      break;
    case REL_SEMITSJ:
      result += "semi_tsj";
      break;
    case REL_ANTI_SEMITSJ:
      result += "anti_semi_tsj";
      break;
    case REL_INDEX_JOIN:
      result += "index_join";
      break;

    default:
      result += "UNKNOWN??";
      break;
    } // switch

    if(CmpCommon::getDefault(COMP_BOOL_183) == DF_ON)
    {
      Int32 potential = getPotential();
      if(potential < 0)
      {
        result += "_-"+ istring(-1*potential);
      }
      else
        result += "_" + istring(potential);
    }

    return result;
} // Join::getText()

HashValue Join::topHash()
{
  HashValue result = RelExpr::topHash();

  result ^= joinPred_;

  return result;
}

NABoolean Join::duplicateMatch(const RelExpr & other) const
{
  if (!RelExpr::duplicateMatch(other))
    return FALSE;

  Join &o = (Join &) other;

  if (joinPred_ != o.joinPred_)
    return FALSE;

  // Temp member to seperate joins PTRule from others in cascades memo
  if (joinFromPTRule_ != o.joinFromPTRule_)
    return FALSE;

  if (joinForZigZag_ != o.joinForZigZag_)
    return FALSE;

  if (avoidHalloweenR2_ != o.avoidHalloweenR2_)
    return FALSE;

  if (halloweenForceSort_ != o.halloweenForceSort_)
    return FALSE;

  return TRUE;
}

 
RelExpr * Intersect::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    {
      result = new (outHeap) Intersect(NULL,
                                        NULL
                                        );
    }
  else
    result = derivedNode;

  return RelExpr::copyTopNode(result, outHeap);
}

RelExpr * Except::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    {
      result = new (outHeap) Except(NULL,
                                        NULL
                                        );
    }
  else
    result = derivedNode;

  return RelExpr::copyTopNode(result, outHeap);
}

RelExpr * Join::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Join *result;

  if (derivedNode == NULL)
    result = new (outHeap) Join(NULL,
			        NULL,
			        getOperatorType(),
				NULL,
				FALSE,
				FALSE,
				outHeap);
  else
    result = (Join *) derivedNode;

  // copy join predicate parse tree (parser only)
  if (joinPredTree_ != NULL)
    result->joinPredTree_ = joinPredTree_->copyTree(outHeap)->castToItemExpr();

  result->joinPred_ = joinPred_;

  // Copy the uniqueness flags
  result->leftHasUniqueMatches_ = leftHasUniqueMatches_;
  result->rightHasUniqueMatches_ = rightHasUniqueMatches_;

  // Copy the equijoin predicates
  result->equiJoinPredicates_ = equiJoinPredicates_;
  result->equiJoinExpressions_ = equiJoinExpressions_;

  result->nullInstantiatedOutput() =  nullInstantiatedOutput();

  result->nullInstantiatedForRightJoinOutput() =
    nullInstantiatedForRightJoinOutput();

  result->transformComplete_ =  transformComplete_;

  // Copy the required order, if any, that originated from an insert node
  result->reqdOrder_ = reqdOrder_;

  // copy flag that marks a mandatory TSJ which could not be unnested
  result->tsjAfterSQO_ = tsjAfterSQO_;
  
  // Copy the flag that indicates if this is a TSJ for a write operation
  result->tsjForWrite_ = tsjForWrite_;
  result->tsjForUndo_ = tsjForUndo_;
  result->tsjForSetNFError_ = tsjForSetNFError_;
  result->tsjForMerge_ = tsjForMerge_;
  result->tsjForMergeWithInsert_ = tsjForMergeWithInsert_;
  result->tsjForMergeUpsert_ = tsjForMergeUpsert_;
  result->tsjForSideTreeInsert_ = tsjForSideTreeInsert_;
  result->enableTransformToSTI_ = enableTransformToSTI_;

  result->forcePhysicalJoinType_ = forcePhysicalJoinType_;
  result->derivedFromRoutineJoin_ = derivedFromRoutineJoin_;

  // Temp member to seperate joins PTRule from others in cascades memo
  result->joinFromPTRule_ = joinFromPTRule_;

  result->joinForZigZag_ = joinForZigZag_;

  result->sourceType_ = sourceType_;


  result->rowsetRowCountArraySize_ = rowsetRowCountArraySize_;

  result->avoidHalloweenR2_ = avoidHalloweenR2_;

  result->halloweenForceSort_ = halloweenForceSort_;

  result->candidateForSubqueryUnnest_ = candidateForSubqueryUnnest_;

  result->candidateForSubqueryLeftJoinConversion_ = candidateForSubqueryLeftJoinConversion_;

  result->candidateForSemiJoinTransform_ = candidateForSemiJoinTransform_;

  result->predicatesToBeRemoved_ = predicatesToBeRemoved_;

//++MV
  result->rightChildMapForLeftJoin_ = rightChildMapForLeftJoin_;
//--MV

  result->isIndexJoin_ = isIndexJoin_;

  if(!result->isInnerNonSemiJoin())
    result->floatingJoin_ = floatingJoin_;

  result->allowPushDown_ = allowPushDown_;

  result->extraHubNonEssentialOutputs_ = extraHubNonEssentialOutputs_;

  result->isForTrafLoadPrep_ = isForTrafLoadPrep_;

  result->beforeJoinPredOnOuterOnly_ = beforeJoinPredOnOuterOnly_;

  return RelExpr::copyTopNode(result, outHeap);
}

void Join::addJoinPredTree(ItemExpr *joinPred)
{
  ExprValueId j = joinPredTree_;

  ItemExprTreeAsList(&j, ITM_AND).insert(joinPred);
  joinPredTree_ = j.getPtr();
}

ItemExpr * Join::removeJoinPredTree()
{
  ItemExpr * result = joinPredTree_;

  joinPredTree_ = NULL;

  return result;
}

void Join::addLocalExpr(LIST(ExprNode *) &xlist,
			LIST(NAString) &llist) const
{
  if (joinPredTree_ != NULL OR
      NOT joinPred_.isEmpty())
    {
      if (joinPred_.isEmpty())
	xlist.insert(joinPredTree_);
      else
	xlist.insert(joinPred_.rebuildExprTree());
      llist.insert("other_join_predicates");
    }

  RelExpr::addLocalExpr(xlist,llist);
}

void Join::convertToTsj()
{
  switch (getOperatorType())
    {
    case REL_JOIN:
      setOperatorType(REL_TSJ);
      break;
    case REL_LEFT_JOIN:
      setOperatorType(REL_LEFT_TSJ);
      break;
    case REL_SEMIJOIN:
      setOperatorType(REL_SEMITSJ);
      break;
    case REL_ANTI_SEMIJOIN:
      setOperatorType(REL_ANTI_SEMITSJ);
      break;
    default:
      ABORT("Internal error: Join::convertTsj()");
      break;
    }
} // Join::convertToTsj()

void Join::convertToNotTsj()
{
  switch (getOperatorType())
    {
    case REL_TSJ:
    case REL_TSJ_FLOW:
    case REL_ROUTINE_JOIN:
      setOperatorType(REL_JOIN);
      break;
    case REL_LEFT_TSJ:
      setOperatorType(REL_LEFT_JOIN);
      break;
    case REL_SEMITSJ:
      setOperatorType(REL_SEMIJOIN);
      break;
    case REL_ANTI_SEMITSJ:
      setOperatorType(REL_ANTI_SEMIJOIN);
      break;
    default:
      ABORT("Internal error: Join::convertTsj()");
      break;
    }
} // Join::convertToNotTsj()

void Join::convertToNotOuterJoin()
{
  switch (getOperatorType())
    {
    case REL_LEFT_JOIN:
      setOperatorType(REL_JOIN);
      break;
    case REL_LEFT_TSJ:
      setOperatorType(REL_TSJ);
      break;
    default:
      ABORT("Internal error: Join::convertOuterJoin()");
      break;
    } // end switch
} // Join::convertToNotOuterJoin()

// ----------------------------------------------------------------------------
// This procedure gets called when synthesising logical properties.
// It finds all the equijoin predicates and saves them in equiJoinPredicates_
// But leaves them in the originating selectionPred()/joinPred()
// ---------------------------------------------------------------------------
void Join::findEquiJoinPredicates()
{
  ValueIdSet  allJoinPredicates;
  ValueId     leftExprId, rightExprId;
  NABoolean   predicateIsOrderPreserving;
  ItemExpr*   expr;

  equiJoinPredicates_.clear();
  equiJoinExpressions_.clear();

  // If this is a TSJ there is nothing to analyze. All join predicates
  // have been pushed down to the second child.
  if(isTSJ())
    return;

  if (isInnerNonSemiJoin())
    {
      allJoinPredicates = selectionPred();
      CMPASSERT(joinPred().isEmpty());
    }
  else
    {
      // for an outer or semi join, the ON clause is stored in "joinPred"
      // while the WHERE clause is stored in "selectionPred".
      allJoinPredicates = joinPred();
    }

  // remove any predicates covered by the inputs
  allJoinPredicates.removeCoveredExprs(getGroupAttr()->
				          getCharacteristicInputs());

  for (ValueId exprId = allJoinPredicates.init();
       allJoinPredicates.next(exprId);
       allJoinPredicates.advance(exprId))
    {
      expr = exprId.getItemExpr();

      if (expr->isAnEquiJoinPredicate(child(0).getGroupAttr(),
				      child(1).getGroupAttr(),
                                      getGroupAttr(),
				      leftExprId, rightExprId,
				      predicateIsOrderPreserving))
	{
	  equiJoinPredicates_ += exprId;
	  equiJoinExpressions_.addMapEntry(leftExprId, rightExprId);
	}
    }

} // Join::findEquiJoinPredicates()

// ---------------------------------------------------------------------------
// separateEquiAndNonEquiJoinPredicates is called from the Join
// implementation rules to weed out of equiJoinPredicates_ all
// the predicates that can be used by the join. The equiJoin
// predicates for the physical operator will be remove from
// the selectioPred() or joinPred() where they came from.
// ---------------------------------------------------------------------------

void Join::separateEquiAndNonEquiJoinPredicates
               (const NABoolean joinStrategyIsOrderSensitive)
{
  ValueId     leftExprId, rightExprId;
  NABoolean   predicateIsOrderPreserving;
  ItemExpr*   expr;

  // equiJoinPredicates_ has all the equijoin predicates found
  // when synthesing logical properties. It is a subset of
  // either selectionPred() or joinPred()

  ValueIdSet  foundEquiJoinPredicates = equiJoinPredicates_;
  equiJoinPredicates_.clear();
  equiJoinExpressions_.clear();

  // remove any predicates covered by the inputs
  foundEquiJoinPredicates.removeCoveredExprs(getGroupAttr()->
				          getCharacteristicInputs());

  for (ValueId exprId = foundEquiJoinPredicates.init();
       foundEquiJoinPredicates.next(exprId);
       foundEquiJoinPredicates.advance(exprId))
    {
      expr = exprId.getItemExpr();

      if (expr->isAnEquiJoinPredicate(child(0).getGroupAttr(),
				      child(1).getGroupAttr(),
                                      getGroupAttr(),
				      leftExprId, rightExprId,
				      predicateIsOrderPreserving))
	{
	  if ( (NOT joinStrategyIsOrderSensitive) OR
               (joinStrategyIsOrderSensitive AND predicateIsOrderPreserving) )
	    {
	      equiJoinPredicates_ += exprId;
	      equiJoinExpressions_.addMapEntry(leftExprId, rightExprId);
	    }
	}
     else
        {
          CMPASSERT(0); // We knew it was an equijoin predicate already
        }
    }

  if (isInnerNonSemiJoin())
    {
      selectionPred() -= equiJoinPredicates_;
      CMPASSERT(joinPred().isEmpty());
    }
  else
    {
      // for an outer or semi join, the ON clause is stored in "joinPred"
      // while the WHERE clause is stored in "selectionPred".
      joinPred() -= equiJoinPredicates_;
    }

  // Since we have changed the set of equijoin predicates we will consider
  // we should resyhtnesize the left/rightHasUnqiueMatches_ flags
  synthConstraints(NULL);

} // Join::separateEquiAndNonEquiJoinPredicates()

void Join::flipChildren()
{
  NABoolean flipUnique;
  flipUnique             = leftHasUniqueMatches_;
  leftHasUniqueMatches_  = rightHasUniqueMatches_;
  rightHasUniqueMatches_ = flipUnique;

  equiJoinExpressions_.flipSides();

} // Join::flipChildren()

// ---------------------------------------------------------------------------
// get the parallel join type and return additional info (optional)
//
// 0: serial join
// 1: TYPE1 join (matching partitions on both sides, including SkewBuster)
// 2: TYPE2 join (join one partition on one side with the
//                entire table on the other side)
// ---------------------------------------------------------------------------

Int32 Join::getParallelJoinType(ParallelJoinTypeDetail *optionalDetail) const
{
  Int32 result = 0;
  ParallelJoinTypeDetail detailedType = Join::PAR_NONE;
  const PartitioningFunction *mpf = NULL;
  const PartitioningFunction *cpf = NULL;

  if (getPhysicalProperty())
    mpf = getPhysicalProperty()->getPartitioningFunction();

  if (mpf == NULL OR mpf->getCountOfPartitions() <= 1)
    {
      // no parallelism or unknown parallelism, not a parallel join
      if (optionalDetail)
        *optionalDetail = detailedType;

      return 0;
    }

  if (child(1)->getPhysicalProperty())
    cpf = child(1)->getPhysicalProperty()->getPartitioningFunction();

  CMPASSERT( cpf );
  if (cpf->castToLogPhysPartitioningFunction())
  {
    // only the child of a join in DP2 can have a logphys part func
    DCMPASSERT(getPhysicalProperty()->executeInDP2());

    cpf = cpf->castToLogPhysPartitioningFunction()->
      getPhysPartitioningFunction();
  }

  if (cpf->isAReplicateViaBroadcastPartitioningFunction() OR
      cpf->isAReplicateNoBroadcastPartitioningFunction())
  {
    // Right child replicates, now check my own partitioning
    // function to see whether this node just passes on the
    // replication function.

    if (mpf->castToLogPhysPartitioningFunction())
    {
      // only a join in DP2 can have a logphys part func
      DCMPASSERT(getPhysicalProperty()->executeInDP2());

      // check the physical part. func of the join in DP2
      mpf = mpf->castToLogPhysPartitioningFunction()->
        getPhysPartitioningFunction();
    }


    if (NOT mpf->isAReplicateViaBroadcastPartitioningFunction() AND
        NOT mpf->isAReplicateNoBroadcastPartitioningFunction())
    {
      // See if the right child REALLY replicates data. If this is
      // a nested join and the chosen plan was a "preferred probing
      // order" plan, then this is really a type 1 join, because a
      // ppo plan always demands the two tables be logically
      // partitioned the same way.
      if (isNestedJoin() && ((NestedJoin*)this)->probesInOrder())
        {
          result = 1;
          detailedType = PAR_OCR;
        }
      else
        {
          // right child replicates data, and the node itself doesn't,
          // this is a type 2 join
          result = 2;
          if (isNestedJoin())
            detailedType = PAR_N2J;
        }
    }
    else
    {
      // Both the right child and the parent replicate data.
      // This is not a parallel join, it is a join that simply
      // passes its replication requirement down to both of its
      // children. The join will be executed in multiple ESPs,
      // but it will not employ one of the two parallel algorithms
      // (TYPE1 or TYPE2).
      result = 0;
    }
  }
  else
  {
    // right child is partitioned, but does not replicate, parallel type 1 join or SkewBuster or OCB
    PartitioningFunction *opf = NULL;

    if (child(0)->getPhysicalProperty())
      opf = child(0)->getPhysicalProperty()->getPartitioningFunction();

    if (opf->isAReplicateViaBroadcastPartitioningFunction())
      {
        // this is an OCB join, which is considered type2
        result = 2;
        detailedType = PAR_OCB;
      }
    else
      {
        // the regular TYPE1 join (including SkewBuster)
        result = 1;

        if (opf->isASkewedDataPartitioningFunction())
          detailedType = PAR_SB;

      }
  }

  if (optionalDetail)
    *optionalDetail = detailedType;

  return result;
}

// ---------------------------------------------------------------------
// Method to split the order req between the two join children.
// return FALSE if not possible
// ---------------------------------------------------------------------
NABoolean Join::splitOrderReq(
                  const ValueIdList& myOrderReq, /*IN*/
                        ValueIdList& orderReqOfChild0, /*OUT*/
                        ValueIdList& orderReqOfChild1  /*OUT*/) const
{
  NABoolean partOfChild0List = TRUE;
  ValueId exprId;

  GroupAttributes* child0GA = child(0).getGroupAttr();
  GroupAttributes* child1GA = child(1).getGroupAttr();

  orderReqOfChild0.clear();
  orderReqOfChild1.clear();

  for (CollIndex ix = 0; ix < myOrderReq.entries(); ix++)
  {
    exprId = myOrderReq.at(ix);

    // dummy variables for the cover test
    ValueIdSet newInputs,referencedInputs,
               coveredSubExpr,uncoveredExpr;
    NABoolean coveredByChild0 =
      child0GA->covers(exprId,
                       newInputs,
                       referencedInputs,
                       &coveredSubExpr,
                       &uncoveredExpr);

    if (NOT coveredByChild0)
      partOfChild0List = FALSE;

    if (partOfChild0List)
      orderReqOfChild0.insertAt(orderReqOfChild0.entries(),exprId);
    else // i.e. NOT partOfChild0List
    {
      //++MV
      // For left join we need to translate the required sort key to
      // the right child outputs because there is an InstantiateNull function
      // on all of the right child outputs. The InstantiateNull function will
      // cause the cover test to fail and therefore the optimization that merge
      // the left child sort key with the right child sort key will fail
      // For more information see NestedJoin::synthPhysicalProperty()
      if (isLeftJoin())
      {
	const ValueIdMap &map = rightChildMapForLeftJoin();
	ValueId tempExprId = exprId;
	map.mapValueIdDown(tempExprId, exprId);
      }
      //--MV

      coveredSubExpr.clear();
      uncoveredExpr.clear();
      NABoolean coveredByChild1 =
      child1GA->covers(exprId,
                       newInputs,
                       referencedInputs,
                       &coveredSubExpr,
                       &uncoveredExpr);

      if (coveredByChild1)
      {
        orderReqOfChild1.insertAt(orderReqOfChild1.entries(),exprId);
      }
      else // i.e NOT (partOfChild0List || coveredByChild1)
      {
        orderReqOfChild0.clear();
        orderReqOfChild1.clear();
        return FALSE;
      }
    }
  } // end for all expressions in the required order

  // Check to see if it is possible to split the order
  if (child0GA->isUnique(orderReqOfChild0) OR
      (child0GA->getMaxNumOfRows() <= 1) OR
      (orderReqOfChild1.entries() == 0))
  {
    return TRUE;
  }
  else
  {
    orderReqOfChild0.clear();
    orderReqOfChild1.clear();
    return FALSE;
  }
} // end splitOrderReq()

// ---------------------------------------------------------------------
// method to split the arrangement req between the two join childs.
// return FALSE if not possible
// ---------------------------------------------------------------------
NABoolean Join::splitArrangementReq(
                  const ValueIdSet& myArrangReq, /*IN*/
                        ValueIdSet& ArrangReqOfChild0, /*OUT*/
                        ValueIdSet& ArrangReqOfChild1  /*OUT*/) const
{
  ArrangReqOfChild0.clear();
  ArrangReqOfChild1.clear();

  ValueId exprId;

  GroupAttributes* child0GA = child(0).getGroupAttr();
  GroupAttributes* child1GA = child(1).getGroupAttr();

  for (exprId = myArrangReq.init();
       myArrangReq.next(exprId);
       myArrangReq.advance(exprId))
  {
    // dummy variables for the cover test
    ValueIdSet newInputs,referencedInputs,
               coveredSubExpr,uncoveredExpr;

    // First we see if this element is covered by child 0
    if (child0GA->covers(exprId,
                         newInputs,
                         referencedInputs,
                         &coveredSubExpr,
                         &uncoveredExpr))
    {
      ArrangReqOfChild0.insert(exprId);
    }
    // Only if an element is not covered by Child0 then we check
    // Child1. i.e. if it is covered by both we bill it to Child0.
    else
    {
      coveredSubExpr.clear();
      uncoveredExpr.clear();
      if (child1GA->covers(exprId,
                           newInputs,
                           referencedInputs,
                           &coveredSubExpr,
                           &uncoveredExpr))
      {
        ArrangReqOfChild1.insert(exprId);
      }
      else
      {
        // If the expression was not covered soley by one of the children, then
        // we must give up. For example, "T1.a * T2.a" needs both children.
        ArrangReqOfChild0.clear();
        ArrangReqOfChild1.clear();
        return FALSE;
      }
    } // end if not covered by child0
  } // end for all expressions in the required arrangement

  // Check to see if it is possible to split the arrangement
  if (child0GA->isUnique(ArrangReqOfChild0) OR
      (child0GA->getMaxNumOfRows() <= 1) OR
      (ArrangReqOfChild1.entries() == 0))
  {
    return TRUE;
  }
  else
  {
    ArrangReqOfChild0.clear();
    ArrangReqOfChild1.clear();
    return FALSE;
  }
} // end splitArrangementReq()

NABoolean Join::ownsVEGRegions() const 
{
  return isLeftJoin() OR isAntiSemiJoin() OR isFullOuterJoin();
}

PlanPriority NestedJoin::computeOperatorPriority
    (const Context* context,
     PlanWorkSpace *pws,
     Lng32 planNumber)
{
  const PhysicalProperty* spp = context->getPlan()->getPhysicalProperty();
  Lng32 degreeOfParallelism = spp->getCountOfPartitions();
  NABoolean applySerialPremium = TRUE;

  double val;
  Cardinality minrows, maxrows;

  CostScalar expectedrows = 
    child(0).getGroupAttr()->getResultCardinalityForEmptyInput();

  if (child(0).getGroupAttr()->hasCardConstraint(minrows, maxrows) && 
     (maxrows <= ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_99)
       OR CostScalar(maxrows) < CostScalar(1.2) * expectedrows))
    {
      // a nested join with at most N outer rows is NOT risky
      val = 1.0;

      // In this case premium for serial plan can be waived because cost of
      // starting ESPs over weighs any benefit we get from parallel plan.
      // Fix is controlled by COMP_BOOL_75, default value is ON.
      if (CmpCommon::getDefault(COMP_BOOL_75) == DF_ON)
        applySerialPremium = FALSE;
    }
  else if (context->getInputLogProp() && 
           context->getInputLogProp()->getResultCardinality().value() > 1)
    {
      // temporary workaround until we cost HJ under NJ correctly
      val = 1.0;
    }
  else
    {
      // a nested join with more than N outer rows is considered risky
      val = CURRSTMT_OPTDEFAULTS->riskPremiumNJ();
      // nested join cache should have a lower risk premium
      GroupAttributes &rightGA = *child(1).getGroupAttr();
      NABoolean probeIsUnique = rightGA.isUnique(rightGA.getCharacteristicInputs());
      NABoolean isTypeOfSemiJoin = isSemiJoin() || isAntiSemiJoin();

      if ((probeIsUnique  || isTypeOfSemiJoin)              &&
          (rowsFromRightHaveUniqueMatch() == FALSE)         &&
          (getOperatorType() != REL_NESTED_JOIN_FLOW)       &&
          (isTSJForWrite() == FALSE )                       &&  
          (getGroupAttr()->
           isEmbeddedUpdateOrDelete() == FALSE )       &&
          (!spp->executeInDP2())                       &&
          (CmpCommon::getDefault(NESTED_JOIN_CACHE) != DF_OFF))
        {
          double red=ActiveSchemaDB()->getDefaults().getAsDouble(COMP_INT_89);
          if (red > 1) 
            {
              // reduce risk premium because it's a nested join cache operator
              val = 1 + (val - 1) / red;
            }
        }
    }

  if (degreeOfParallelism <= 1 && applySerialPremium)
    {
    // serial plans are risky. exact an insurance premium from serial plans.
    val *= CURRSTMT_OPTDEFAULTS->riskPremiumSerial();
  }
  CostScalar premium(val);
  PlanPriority result(0, 0, premium);

  // esp parallelism priority logic below does not apply to operators in dp2
  if(spp->executeInDP2())
    return result;

  // For the option of Max Degree of Parallelism we can either use the
  // value set in comp_int_9 (if positive) or we use the number of CPUs
  // if the CQD is set to -1, or feature is disabled if CQD is 0 (default).
  Lng32 maxDegree = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_9);
  if (CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible() OR (maxDegree == -1) )
  {
    // if CQD is set to -1 this mean use the number of CPUs
    maxDegree = spp->getCurrentCountOfCPUs();
  }
  if (maxDegree > 1) // CQD set to 0 means feature is OFF
  {
    if (degreeOfParallelism < maxDegree)
      result.incrementLevels(0,-10); // need to replace with constant
  }

    // fix for SAP case 10-100602-2913, soln 10-100602-0803
  // long-running select for DSO activation, query plan for empty table

  // if nested join has
  // 1) a tuple list (something with 0 base tables) on left, and
  // 2) a table on right, and
  // 3) prefer_key_nested_join is set, and
  // 4) table's predicate (including pushed join pred) forms begin/end
  //    key on table, and
  // 5) tuple list is of reasonable size (<= tuplelist_size_threshold), 
  //    and
  // 6) table is small or has no stats
  // then give nested join plan higher priority
  //   push it by 1 if it has a key range predicate
  //   push it by 2 if it has a unique key predicate

  // is prefer_key_nested_join active?
  NABoolean prefer_key_nested_join = 
    (CmpCommon::getDefault(SAP_PREFER_KEY_NESTED_JOIN) == DF_ON);
  if (prefer_key_nested_join) {
    GroupAttributes *grpAttr0 = child(0).getGroupAttr();
    GroupAttributes *grpAttr1 = child(1).getGroupAttr();
    GroupAnalysis *grpA0 = grpAttr0->getGroupAnalysis();
    GroupAnalysis *grpA1 = grpAttr1->getGroupAnalysis();

    // is left child guaranteed small?
    NABoolean leftIsSmall = FALSE;
    Cardinality minLeft, maxLeft;
    if (grpAttr0->hasCardConstraint(minLeft,maxLeft) AND
        maxLeft <= ActiveSchemaDB()->getDefaults().getAsLong
        (SAP_TUPLELIST_SIZE_THRESHOLD)) {
      leftIsSmall = TRUE;
    }

    // is right a single table?
    FileScan *rScan = NULL;
    NABoolean rightIsTable = pws->getScanLeaf(1, planNumber, rScan);

    // is right table small?
    NABoolean isSmallTable = 
      grpAttr1->getResultCardinalityForEmptyInput() <= 
      ActiveSchemaDB()->getDefaults().getAsLong
      (SAP_KEY_NJ_TABLE_SIZE_THRESHOLD);

    // prefer this nested_join iff all above conditions are met
    if (leftIsSmall && rightIsTable && isSmallTable && rScan) {

      // is predicate on unique key or prefix key?
      NABoolean hasUniqKeyPred = FALSE;
      NABoolean hasPrefixKeyPred = FALSE;
      const SearchKey *sKey = rScan->getSearchKey();
      if (sKey) {
        hasUniqKeyPred = sKey->isUnique();
        // TBD: check if key prefix selects few or many rows
        hasPrefixKeyPred = sKey->getKeyPredicates().entries() > 0;
      }
      // TBD: take care of MDAM case

      // push priority by 2 if it has a unique key predicate
      if (hasUniqKeyPred)
        result.incrementLevels(2,0);

      // push priority by 1 if it has a prefix key predicate
      else if (hasPrefixKeyPred)
        result.incrementLevels(1,0);
    }
  }

  return result;
}

// -----------------------------------------------------------------------
// member functions for class NestedJoinFlow
// -----------------------------------------------------------------------
RelExpr * NestedJoinFlow::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  NestedJoinFlow *result;

  if (derivedNode == NULL)
    {
      result = new (outHeap) NestedJoinFlow(NULL,
					    NULL,
					    NULL,
					    NULL,
					    outHeap);
    }
  else
    result = (NestedJoinFlow*)derivedNode;

  result->sendEODtoTgt_ = sendEODtoTgt_;

  return NestedJoin::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class NestedJoin
// -----------------------------------------------------------------------

NABoolean NestedJoin::isLogical()  const {return FALSE;}

NABoolean NestedJoin::isPhysical() const {return TRUE;}

const NAString NestedJoin::getText() const
{
  switch (getOperatorType())
    {
    case REL_NESTED_JOIN:
      return "nested_join";
    case REL_LEFT_NESTED_JOIN:
      return "left_nested_join";
    case REL_NESTED_SEMIJOIN:
      return "nested_semi_join";
    case REL_NESTED_ANTI_SEMIJOIN:
      return "nested_anti_semi_join";
    case REL_NESTED_JOIN_FLOW:
      return "tuple_flow";
    default:
      return "UNKNOWN??";
    } // switch
} // NestedJoin::getText()

RelExpr * NestedJoin::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    {
      result = new (outHeap) NestedJoin(NULL,
				        NULL,
				        getOperatorType(),
					outHeap);
    }
  else
    result = derivedNode;

  return Join::copyTopNode(result, outHeap);
}

NABoolean NestedJoin::allPartitionsProbed()
{
  return TRUE;//all partitions probed
}

// Conditions to check before applying the nested join probing cache:
//   1. The right child has a cardinality constraint of at most one row,
//      or else the join is a semi-join or anti-semi-join.
//   2. The right child's characteristic inputs are not unique for every
//      request (with exceptions, see below).
//   3. The nested join is not a NestedJoinFlow.
//   4. The right child does not contain any IUD operations.
//   5. The nested join's GroupAttributes do not include embedded IUD.
//   6. The execution location is not in DP2.
//   7. The nested join cache feature is not suppressed by a default.
//   8. The right child does not contain non-deterministic UDRs
NABoolean NestedJoin::isProbeCacheApplicable(PlanExecutionEnum loc) const
{
  NABoolean result = FALSE;
  GroupAttributes &rightGA = *child(1).getGroupAttr();
  NABoolean probeIsUnique = rightGA.isUnique(rightGA.getCharacteristicInputs());

  if ( !probeIsUnique ) {
     // dig deep into the right child to see if the searchKey associated with the
     // only Scan node is unique. If it is unique, we also declare the probe is
     // unique (i.e., for each probe, there is at most one row returned). The
     // probe uniqueness property check is for the current implementation in executor
     // where only one entry per probe in the hash table in probe cache is allocated.

     RelExpr *childExpr = child(1);

     // skip over Exchange nodes
     while (childExpr && (childExpr->getOperator() == REL_EXCHANGE))
        childExpr = childExpr->child(0);

     if (childExpr)
        {
           OperatorTypeEnum x = childExpr->getOperator();
           if (x == REL_HBASE_ACCESS || x == REL_HBASE_COPROC_AGGR)
           {
             HbaseAccess *hbscan = (HbaseAccess*)childExpr;
             const SearchKey *skey = hbscan->getSearchKey();
             if (skey && skey->isUnique())
               probeIsUnique = TRUE;
           }
        }
  }

  NABoolean isTypeOfSemiJoin = isSemiJoin() || isAntiSemiJoin();

  if
    ((probeIsUnique  || isTypeOfSemiJoin)                  &&
     (getOperatorType() != REL_NESTED_JOIN_FLOW)           &&
     (isTSJForWrite() == FALSE )                           &&
     (getGroupAttr()->
        isEmbeddedUpdateOrDelete() == FALSE )              &&
     loc != EXECUTE_IN_DP2                                 &&
     (CmpCommon::getDefault(NESTED_JOIN_CACHE) != DF_OFF)  &&
     (rightGA.getHasNonDeterministicUDRs() == FALSE))
    {
      if (! rowsFromRightHaveUniqueMatch())
        {
          // big if passed and we have a chance of duplicate probes from the left
          result = TRUE;
        }
      else
        {
          // If left probes are unique, there isn't a reason for a probe
          // cache. However, we might be able to pull up some predicate from
          // the right into the ProbeCache, which might give us non-unique
          // probes. The code below targets a specific case (ALM 4783):
          //
          //   NestedJoin
          //   /       \
          //          Aggregate (one equi-join pred is a HAVING pred)
          //
          // We can't detect this in the optimizer (where the nested join
          // may point to a filter or a MEMO group), but that's fine, since
          // we don't really want to give this unusual case a cost advantage.
          RelExpr *childExpr = child(1);
          
          // skip over Exchange and MapValueIds nodes
          while (childExpr &&
                 (childExpr->getOperator() == REL_EXCHANGE ||
                  childExpr->getOperator() == REL_MAP_VALUEIDS))
            childExpr = childExpr->child(0);
          
          if (childExpr && 
              childExpr->getOperator().match(REL_ANY_GROUP) &&
              CmpCommon::getDefault(NESTED_JOIN_CACHE_PREDS) != DF_OFF)
            {
              GroupByAgg *childGB = (GroupByAgg *) childExpr;

              if (childGB->groupExpr().isEmpty() &&
                  ! childGB->selectionPred().isEmpty())
                // This is a scalar aggregate with a HAVING predicate,
                // at least we know that there is a reasonable chance that
                // we can pull up a HAVING predicate into the probe cache
                // in method GroupByAgg::tryToPullUpPredicatesInPreCodeGen()
                result = TRUE;
            }
        }
    }
  return result;
}

// -----------------------------------------------------------------------
// member functions for class MergeJoin
// -----------------------------------------------------------------------

NABoolean MergeJoin::isLogical()  const {return FALSE;}

NABoolean MergeJoin::isPhysical() const {return TRUE;}

const NAString MergeJoin::getText() const
{
  switch (getOperatorType())
    {
    case REL_MERGE_JOIN:
      return "merge_join";
    case REL_LEFT_MERGE_JOIN:
      return "left_merge_join";
    case REL_MERGE_SEMIJOIN:
      return "merge_semi_join";
    case REL_MERGE_ANTI_SEMIJOIN:
      return "merge_anti_semi_join";
    default:
      return "UNKNOWN merge join??";
    } // switch
} // MergeJoin::getText()

RelExpr * MergeJoin::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) MergeJoin(NULL,
				     NULL,
				     getOperatorType(),
				     NULL,
				     outHeap);
  else
    result = derivedNode;

  return Join::copyTopNode(result, outHeap);
}

void MergeJoin::addLocalExpr(LIST(ExprNode *) &xlist,
			     LIST(NAString) &llist) const
{
  xlist.insert(orderedMJPreds_.rebuildExprTree());
  llist.insert("merge_join_predicate");

  Join::addLocalExpr(xlist,llist);
}

PlanPriority MergeJoin::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{
  const PhysicalProperty* spp = context->getPlan()->getPhysicalProperty();
  Lng32 degreeOfParallelism = spp->getCountOfPartitions();
  double val = CURRSTMT_OPTDEFAULTS->riskPremiumMJ();
  if (degreeOfParallelism <= 1)
    {
      // serial plans are risky. exact an insurance premium from serial plans.
      val *= CURRSTMT_OPTDEFAULTS->riskPremiumSerial();
    }
  CostScalar premium(val);
  PlanPriority result(0, 0, premium);

  // For the option of Max Degree of Parallelism we can either use the
  // value set in comp_int_9 (if positive) or we use the number of CPUs
  // if the CQD is set to -1, or feature is disabled if CQD is 0 (default).
  Lng32 maxDegree = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_9);
  if (CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible() OR (maxDegree == -1) )
  {
    // if CQD is set to -1 this mean use the number of CPUs
    maxDegree = spp->getCurrentCountOfCPUs();
  }
  if (maxDegree > 1) // CQD set to 0 means feature is OFF
  {
    if (degreeOfParallelism < maxDegree)
      result.incrementLevels(0,-10); // need to replace with constant
  }

  return result;
}

// -----------------------------------------------------------------------
// member functions for class HashJoin
// -----------------------------------------------------------------------

NABoolean HashJoin::isLogical()  const { return FALSE; }

NABoolean HashJoin::isPhysical() const { return TRUE; }

const NAString HashJoin::getText() const
{
  switch (getOperatorType())
    {
    case REL_HASH_JOIN:
      return "hash_join";
    case REL_LEFT_HASH_JOIN:
      return "left_hash_join";
    case REL_HASH_SEMIJOIN:
      return "semi_join";
    case REL_HASH_ANTI_SEMIJOIN:
      return "hash_anti_semi_join";

    case REL_HYBRID_HASH_JOIN:
     {
        if(((HashJoin *)this)->isOrderedCrossProduct())
          return "ordered_cross_product";
        else
          return "hybrid_hash_join";
      }
    case REL_LEFT_HYBRID_HASH_JOIN:
      return "left_hybrid_hash_join";
    case REL_FULL_HYBRID_HASH_JOIN:
      return "full_hybrid_hash_join";
    case REL_HYBRID_HASH_SEMIJOIN:
      return "hybrid_hash_semi_join";
    case REL_HYBRID_HASH_ANTI_SEMIJOIN:
      return "hybrid_hash_anti_semi_join";

    case REL_ORDERED_HASH_JOIN:
      {
	if (getEquiJoinPredicates().isEmpty())
          return "ordered_cross_product";
        else
	  return "ordered_hash_join";
      }
    case REL_LEFT_ORDERED_HASH_JOIN:
      return "left_ordered_hash_join";
    case REL_ORDERED_HASH_SEMIJOIN:
      return "ordered_hash_semi_join";
    case REL_ORDERED_HASH_ANTI_SEMIJOIN:
      return "ordered_hash_anti_semi_join";
    default:
      return "UNKNOWN hash join??";
    } // switch
} // HashJoin::getText()




RelExpr * HashJoin::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL) {
    result = new (outHeap) HashJoin(NULL,
				    NULL,
				    getOperatorType(),
				    NULL,
				    outHeap);
    ((HashJoin *)result)->setIsOrderedCrossProduct(isOrderedCrossProduct());
    ((HashJoin *)result)->setReuse(isReuse());
    ((HashJoin *)result)->setNoOverflow(isNoOverflow());
  }
  else
    result = derivedNode;

  ((HashJoin*)result)->isNotInSubqTransform_ = isNotInSubqTransform_;
  ((HashJoin*)result)->requireOneBroadcast_  = requireOneBroadcast_;

  ((HashJoin*)result)->innerAccessOnePartition_ = innerAccessOnePartition_;

  return Join::copyTopNode(result, outHeap);
}

PlanPriority HashJoin::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{
  const PhysicalProperty* spp = context->getPlan()->getPhysicalProperty();
  Lng32 degreeOfParallelism = spp->getCountOfPartitions();
  double val = 1;
  if (degreeOfParallelism <= 1 && getInnerAccessOnePartition() == FALSE ) 
    {
      // serial plans are risky. exact an insurance premium from serial plans.
      // The exception is when only one partition is accessed.
      val = CURRSTMT_OPTDEFAULTS->riskPremiumSerial();
    }
  CostScalar premium(val);
  PlanPriority result(0, 0, premium);

  if (QueryAnalysis::Instance() AND
      QueryAnalysis::Instance()->optimizeForFirstNRows())
    result.incrementLevels(HASH_JOIN_FIRST_N_PRIORITY,0);

  // For the option of Max Degree of Parallelism we can either use the
  // value set in comp_int_9 (if positive) or we use the number of CPUs
  // if the CQD is set to -1, or feature is disabled if CQD is 0 (default).
  Lng32 maxDegree = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_9);
  if (CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible() OR (maxDegree == -1) )
  {
    // if CQD is set to -1 this mean use the number of CPUs
    maxDegree = spp->getCurrentCountOfCPUs();
  }
  if (maxDegree > 1) // CQD set to 0 means feature is OFF
  {
    if (degreeOfParallelism < maxDegree)
      result.incrementLevels(0,-10); // need to replace with constant
  }

  return result;
}

void HashJoin::addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const
{
  if (NOT getEquiJoinPredicates().isEmpty())
  {
  xlist.insert(getEquiJoinPredicates().rebuildExprTree());
    llist.insert("hash_join_predicates");
  }

  if (NOT valuesGivenToChild_.isEmpty())
  {
    xlist.insert(valuesGivenToChild_.rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("reuse_comparison_values");
  }

  if (NOT checkInnerNullExpr_.isEmpty())
  {
    xlist.insert(checkInnerNullExpr_.rebuildExprTree());
    llist.insert("check_inner_null_expr");
  }
  
  if (NOT checkOuterNullExpr_.isEmpty())
  {
    xlist.insert(checkOuterNullExpr_.rebuildExprTree());
    llist.insert("check_outer_null_expr");
  }

  Join::addLocalExpr(xlist,llist);
}
void HashJoin::resolveSingleColNotInPredicate()
{

  if (!isAntiSemiJoin() ||
      !isHashJoin())
  {
    return;
  }

  ValueIdSet jPred = joinPred();
  short notinCount=0;
  
  for (	ValueId valId = jPred.init(); 
	jPred.next(valId); 
	jPred.advance(valId))
  {
    ItemExpr * itmExpr = valId.getItemExpr();
    
    if (itmExpr->getOperatorType() == ITM_NOT_IN)
    {
      if (((NotIn*)itmExpr)->getEquivEquiPredicate() == NULL_VALUE_ID)
      {
        ((NotIn*)itmExpr)->cacheEquivEquiPredicate();

      }

      // use cached value ids
      equiJoinPredicates() += ((NotIn*)itmExpr)->getEquivEquiPredicate();
        
      joinPred() -=valId;
      joinPred() += ((NotIn*)itmExpr)->getEquivEquiPredicate();

      notinCount++;

      setIsNotInSubqTransform(TRUE); 
      setRequireOneBroadcast(((NotIn*)itmExpr)->getIsOneInnerBroadcastRequired());
    }
  }


  DCMPASSERT(notinCount <=1);
}//void HashJoin::resolveSingleColNotInPredicate()


void Join::resolveSingleColNotInPredicate()
{
  // applies only to anti_semi_joins
  if (!isAntiSemiJoin()) {
    return; 
  }

  short notinCount = 0;

  ValueIdSet jPred = joinPred();

  for (	ValueId valId = jPred.init(); 
	jPred.next(valId); 
	jPred.advance(valId))
  {
    ItemExpr * itmExpr = valId.getItemExpr();

    if (itmExpr->getOperatorType() == ITM_NOT_IN)
    {
      if (((NotIn*)itmExpr)->getEquivNonEquiPredicate() == NULL_VALUE_ID )
      {
        ((NotIn*)itmExpr)->cacheEquivNonEquiPredicate();
      }
      
      //use cached valueids
      joinPred() -= valId;
      joinPred() += ((NotIn*)itmExpr)->getEquivNonEquiPredicate();

      notinCount++;
    }
  }


  DCMPASSERT(notinCount <=1);

}//void Join::resolveSingleColNotInPredicate()


// Join::rewriteNotInPredicate() 
// is method is called right after the predicates are pushed down and
// the goal is to make sure that only the NotIn predicate is present.
// in any other preduicate exist besides the NotIn Predicate than we can not
// optimize the hash anti semi join when the outer column is nullable and may 
// have null values
void Join::rewriteNotInPredicate()
{
  // applies only to anti_semi_joins
  if (!isAntiSemiJoin()) 
  {
    return; 
  }

  ValueIdSet jPred = joinPred();

  ItemExpr * notinExpr=NULL;

  NABoolean otherPredicatesExist = FALSE;
  for (	ValueId valId = jPred.init(); 
	jPred.next(valId); 
	jPred.advance(valId))
  {
    ItemExpr * itmExpr = valId.getItemExpr();
    if (itmExpr->getOperatorType() != ITM_NOT_IN)
    {
      otherPredicatesExist = TRUE;
    }
    else
    {
      // assert if we already encoutered a not in
      DCMPASSERT(notinExpr == NULL);
      notinExpr = itmExpr;
    }
  }

  if (notinExpr)
  {
    //single column
    DCMPASSERT (notinExpr->child(0)->getOperatorType() != ITM_ITEM_LIST);
    const NAType &outerType = notinExpr->child(0)->getValueId().getType();

    GroupAttributes * leftChildGrpAttr = child(0).getGroupAttr();
    GroupAttributes * rightChildGrpAttr = child(1).getGroupAttr();
    const ValueIdSet &inputs = getGroupAttr()->getCharacteristicInputs();
    ValueIdSet refs;

    ValueId valId = notinExpr->getValueId();

    if ((outerType.supportsSQLnull() && 
        !((NotIn*)notinExpr)->getOuterNullFilteringDetected() &&
        otherPredicatesExist) ||
        //fix for solution Id:10-100331-9194 
        //select count(*) from h2_data_1k_37 where col_lar2 <> all (select col_id from
        //h2_data_1k_37 where col_id=100) ;
        //NotIn(VEGRef(col_lar2),VEGRef(col_id=100)) is in the join prdicate and in t he characteristc
        //outputs of the child. changing it to euipredicate may lead to wrong results
        //the below code will change the NotIn(a,b) predicate to NOT(a<>B is true) when the predicate is covered 
        //by one of of children
        leftChildGrpAttr->covers(valId, inputs, refs) || 
        rightChildGrpAttr->covers(valId, inputs, refs))
    {
      ValueId tmpId = ((NotIn *)notinExpr)->createEquivNonEquiPredicate();
      ItemExpr * tmpItemExpr = tmpId.getItemExpr();
      valId.replaceItemExpr(tmpItemExpr);
    }
  }
  
}//Join::rewriteNotInPredicate()

// Join::rewriteNotInPredicate( ValueIdSet & origVidSet, ValueIdSet & newVidSet)
// if both the outer and the inner columns are not nullable or are nullable but
// have no NULL values then the NotIn Predicate is changed to an equi-predicate.
// otherwise the NotIn predicate is not changed and the optimizer will decide what
// to do with it
// this method is called right after the pull up of the predicates in join::transformNode()
// in the case of anti semi join the inner predicates are pulled and added to join pred
// and the outer predicates are pulled and added to selectionPredicates
// When we look for outer NUll filetering predicates we look in the selection predocates 
// and when we look inner NULL filtering predicates we look in the join predicates
void Join::rewriteNotInPredicate( ValueIdSet & origVidSet, ValueIdSet & newVidSet)
{
  // applies only to anti_semi_joins
  if (!isAntiSemiJoin()) 
  {
    return; 
  }

  ValueIdSet jPred = joinPred();
  ValueIdSet selPred = selectionPred();

  short notinCount = 0;

  for (	ValueId valId = joinPred().init(); 
	joinPred().next(valId); 
	joinPred().advance(valId))
  {
    ItemExpr * itmExpr = valId.getItemExpr();
 
    if (itmExpr->getOperatorType() == ITM_NOT_IN)
    {
      //single column
      if (itmExpr->child(0)->getOperatorType() != ITM_ITEM_LIST)
      {
        const NAType &innerType = itmExpr->child(1)->getValueId().getType();
        const NAType &outerType = itmExpr->child(0)->getValueId().getType();

        selPred -= valId;
        jPred -= valId;

        NABoolean child0IsNotNullable = selPred.isNotNullable(itmExpr->child(0)) ;
        NABoolean child1IsNotNullable = jPred.isNotNullable(itmExpr->child(1)) ;
        
        if ((!innerType.supportsSQLnull() || child1IsNotNullable) &&
              (!outerType.supportsSQLnull() || child0IsNotNullable) )
        {
          origVidSet += valId;
          // we can change the not in predicate to an equi-predicate in this case
          newVidSet  += ((NotIn *)itmExpr)->createEquivEquiPredicate();
        }
        else
        { 
          // outer refrences case are not handled by optimization
          ValueIdSet rightSideofPred;
          ValueIdSet tempSet;

          rightSideofPred.insert(itmExpr->child(1)->getValueId());
          
          rightSideofPred.getReferencedPredicates(child(0)->getGroupAttr()->getCharacteristicOutputs(), tempSet) ;

          if (!tempSet.isEmpty())
          {
             origVidSet += valId;
             // we can change the not in predicate to an equi-predicate in this case
             newVidSet  += ((NotIn *)itmExpr)->createEquivNonEquiPredicate();
          }
          else
          {
            if (CmpCommon::getDefault(NOT_IN_OUTER_OPTIMIZATION) == DF_OFF)
            {
              //NOT_IN_OUTER_OPTIMIZATION == OFF ==> if outer is nullable and may have NULL values
              //			       change to Non equi-predicate here
              if ( outerType.supportsSQLnull() &&  
                 !child0IsNotNullable) 
              {
                origVidSet += valId;
                // we can change the not in predicate to an equi-predicate in this case
                newVidSet  += ((NotIn *)itmExpr)->createEquivNonEquiPredicate();
              }
            }
            else
            {
              // case where outer or inner columns (or both) is nullable and may have NULL values
              // optimizer will decide depending on the type of join
              // hash join      ==> equi-predicate with cancel expression when inner is nullbale
              //		    ==> filter to filter out NULL values coming from outer side
              //		    ==> when inner is not empty
              //		    ==> NUILL values coming from outer side are not filtered out when 
              //		    ==> inner is empty
              // non hash join  ==> Non equi-predicate
              if (child0IsNotNullable)
              {
                ((NotIn*)itmExpr)->setOuterNullFilteringDetected(TRUE);
              }
              if (child1IsNotNullable)
              {
                ((NotIn*)itmExpr)->setInnerNullFilteringDetected(TRUE);
              }
            }
          }
        }
      }
      else
      {
        ValueIdSet predSet ;
        //ValueIdSet jPreds;
        //ValueIdSet selPred;

        //jPreds = joinPred();
        //selPred = selectionPred();

        predSet = NotIn::rewriteMultiColNotInPredicate( valId, 
                                                        joinPred(),
                                                        selectionPred());
        
        DCMPASSERT(predSet.entries() >0);
        
        origVidSet += valId;
        
        newVidSet += predSet;

      }
      
      notinCount++;

    }//if (itmExpr->getOperatorType() == ITM_NOT_IN)
 
  }
  DCMPASSERT(notinCount <=1);

}//void Join::rewriteNotInPredicate()

// -----------------------------------------------------------------------
// member functions for class Intersect
// -----------------------------------------------------------------------

Intersect::Intersect(RelExpr *leftChild,
	     RelExpr *rightChild)
: RelExpr(REL_INTERSECT, leftChild, rightChild)
{ }

Intersect::~Intersect() {}

Int32 Intersect::getArity() const { return 2; }

const NAString Intersect::getText() const
{
  return "intersect";
}

// -----------------------------------------------------------------------
// // member functions for class Except
// // -----------------------------------------------------------------------
Except::Except(RelExpr *leftChild,
             RelExpr *rightChild)
: RelExpr(REL_EXCEPT, leftChild, rightChild)
{ }

Except::~Except() {}

Int32 Except::getArity() const { return 2; }

const NAString Except::getText() const
{
  return "except";
}

// -----------------------------------------------------------------------
// member functions for class Union
// -----------------------------------------------------------------------
Union::Union(RelExpr *leftChild,
	     RelExpr *rightChild,
	     UnionMap *unionMap,
	     ItemExpr *condExpr,
	     OperatorTypeEnum otype,
         CollHeap *oHeap,
         NABoolean sysGenerated,
         NABoolean mayBeCacheable
         )
: RelExpr(otype, leftChild, rightChild, oHeap),
  condExprTree_(condExpr)
  ,trigExceptExprTree_(NULL)
  ,previousIF_(NULL)
  ,flags_(0)
  ,leftList_(NULL)
  ,rightList_(NULL)
  ,currentChild_(-1)
  ,alternateRightChildOrderExprTree_(NULL) //++MV
  ,isSystemGenerated_(sysGenerated)
  ,isSerialUnion_(FALSE)
  ,variablesSet_(oHeap)
  
{
  if ( NOT mayBeCacheable )
    setNonCacheable();

  if (unionMap != NULL)
    {
      unionMap_ = unionMap;
      unionMap_->count_++;
    }
   else
      unionMap_ = new (oHeap) UnionMap;

   condExpr_.clear();
   trigExceptExpr_.clear();
   alternateRightChildOrderExpr_.clear(); //++MV
   variablesSet_.clear();
   controlFlags_ = 0; //++ Triggers -
}

Union::~Union() { if (unionMap_->count_ == 0) delete unionMap_;}

Int32 Union::getArity() const { return 2; }

void Union::rewriteUnionExpr(const ValueIdSet &unionExpr,
                             ValueIdSet &leftExpr,
                             ValueIdSet &rightExpr) const
{
  // walk the original selection predicates and rewrite them in terms
  // of the mapped value ids of the union's inputs
  for (ValueId x = unionExpr.init(); unionExpr.next(x); unionExpr.advance(x))
    {
      ValueId newLeftExpr =
	x.getItemExpr()->mapAndRewrite(getLeftMap(),TRUE);
      ValueId newRightExpr =
	x.getItemExpr()->mapAndRewrite(getRightMap(),TRUE);

      leftExpr += newLeftExpr;
      rightExpr += newRightExpr;
    }
} // Union::rewriteExprs()

void Union::pushdownCoveredExpr(const ValueIdSet & outputExpr,
                                const ValueIdSet & newExternalInputs,
                                ValueIdSet & predicatesOnParent,
				const ValueIdSet * setOfValuesReqdByParent,
			        Lng32 // childIndex ignored
		               )
{
  ValueIdSet resultSet = outputExpr;
  if (setOfValuesReqdByParent)
    resultSet += *setOfValuesReqdByParent;
  resultSet += getGroupAttr()->getCharacteristicInputs();

  // alternateRightChildOrderExpr expressions should not be pushed down
  resultSet.insertList(alternateRightChildOrderExpr()); // ++MV
  // ---------------------------------------------------------------------
  // Not all the output columns from the union may be needed.
  // Map the required input list to the corresponding left
  // and right required outputs list
  // ---------------------------------------------------------------------
  ValueIdSet valuesRequiredFromLeft, valuesRequiredFromRight;
  rewriteUnionExpr(resultSet,
               valuesRequiredFromLeft,
               valuesRequiredFromRight);

  // ---------------------------------------------------------------------
  // Rewrite selectionPred()
  // ---------------------------------------------------------------------
  ValueIdSet leftPred, rightPred, emptySet;
  rewriteUnionExpr(predicatesOnParent, leftPred, rightPred);

  // push the left predicates to the left subtree
  // empty set for the first argument indicates that there are no
  // non-essential outputs, (in other words, outputs that are
  // simply passed through)
  RelExpr::pushdownCoveredExpr(emptySet,
                               newExternalInputs,
                               leftPred,
			       &valuesRequiredFromLeft,
                               0
                               );

  // push the right predicates to the right subtree
  RelExpr::pushdownCoveredExpr(emptySet,
                               newExternalInputs,
                               rightPred,
			       &valuesRequiredFromRight,
                               1
                               );

  // Verify that all the predicates were pushed
  leftPred -= child(0)->selectionPred();
  CMPASSERT( leftPred.isEmpty() );

  rightPred -= child(1)->selectionPred();
  CMPASSERT( rightPred.isEmpty() );

  // All the predicates have been pushed down to the children.
  predicatesOnParent.clear();

} // Union::pushdownCoveredExpr

void Union::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  //
  // The output of the union is defined by the ValueIdUnion
  // expressions that are maintained in the colMapTable_.
  //
  Lng32 ne = unionMap_->colMapTable_.entries();
  for (Lng32 index = 0; index < ne; index++)
    {
      // Accumulate the ValueIds of the result of the union
      // in the set provided by the caller.
      outputValues += ((ValueIdUnion *) (unionMap_->colMapTable_[index].getItemExpr()))->getResult();
    }
} // Union::getPotentialOutputValues()


HashValue Union::topHash()
{
  HashValue result = RelExpr::topHash();

  // result ^= colMapTable_;

  return result;
}

NABoolean Union::duplicateMatch(const RelExpr & other) const
{
  if (NOT RelExpr::duplicateMatch(other))
    return FALSE;

  Union &o = (Union &) other;

  if (NOT ((unionMap_ == o.unionMap_) AND
	   (condExpr_ == o.condExpr_) AND
	   (trigExceptExpr_ == o.trigExceptExpr_) AND
	   (alternateRightChildOrderExpr_ == o.alternateRightChildOrderExpr_))) //++MV
    return FALSE;

  return TRUE;
}

RelExpr * Union::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Union *result;

  if (derivedNode == NULL)
    result = new (outHeap) Union(NULL,
				 NULL,
				 unionMap_,
				 NULL,
				 getOperatorType(),
                 outHeap);
  else
    result = (Union *) derivedNode;

  if (condExprTree_ != NULL)
    result->condExprTree_ = condExprTree_->copyTree(outHeap)->castToItemExpr();

  if (trigExceptExprTree_ != NULL)
    result->trigExceptExprTree_ = trigExceptExprTree_->copyTree(outHeap)->castToItemExpr();

  //++MV -
  if (alternateRightChildOrderExprTree_ != NULL)
    result->alternateRightChildOrderExprTree_ =
      alternateRightChildOrderExprTree_->copyTree(outHeap)->castToItemExpr();
  //--MV -

  result->condExpr_ = condExpr_;
  result->trigExceptExpr_ = trigExceptExpr_;
  result->alternateRightChildOrderExpr_ = alternateRightChildOrderExpr_;
  result->setUnionFlags(getUnionFlags());
  //++Triggers -
  result->controlFlags_ = controlFlags_;

  result->isSystemGenerated_ = isSystemGenerated_;

  if (getSerialUnion())
  {
    result->setSerialUnion();
  }
  return RelExpr::copyTopNode(result, outHeap);
}

void Union::addValueIdUnion(ValueId vidUnion, CollHeap* heap)
{
  ValueIdUnion *xvid = (ValueIdUnion *) vidUnion.getItemExpr();

  CMPASSERT(vidUnion.getItemExpr()->getOperatorType() == ITM_VALUEIDUNION);

  // This method is only called by the binder when it is first
  // building the unionMap
  if(unionMap_->count_ > 1)
    {
    unionMap_->count_--;
    unionMap_ = new (heap) UnionMap;
    }

  CMPASSERT(unionMap_->count_ == 1);


  // add the value id to the list of value ids for ValueIdUnion expressions
  // and also add entries to the two maps that describe the same information
  unionMap_->colMapTable_.insert(vidUnion);
  unionMap_->leftColMap_.addMapEntry(vidUnion,xvid->getLeftSource());
  unionMap_->rightColMap_.addMapEntry(vidUnion,xvid->getRightSource());
}

//++ Triggers -
void Union::setNoOutputs()
{
  CMPASSERT(flags_ == UNION_BLOCKED || flags_ == UNION_ORDERED);

  controlFlags_ |= NO_OUTPUTS;
}

void Union::addLocalExpr(LIST(ExprNode *) &xlist,
			 LIST(NAString) &llist) const
{
  if (condExprTree_ != NULL)
  {
    xlist.insert(condExprTree_);
    llist.insert("condExprTree");
  }

  if (NOT condExpr_.isEmpty())
  {
    xlist.insert(condExpr_.rebuildExprTree());
    llist.insert("condExpr");
  }

  if (trigExceptExprTree_ != NULL)
  {
    xlist.insert(trigExceptExprTree_);
    llist.insert("trigExceptExprTree");
  }

  if (NOT trigExceptExpr_.isEmpty())
  {
    xlist.insert(trigExceptExpr_.rebuildExprTree());
    llist.insert("trigExceptExpr");
  }

  if (alternateRightChildOrderExprTree_ != NULL)
  {
    xlist.insert(alternateRightChildOrderExprTree_);
    llist.insert("alternateRightChildOrderExprTree");
  }

  if (NOT alternateRightChildOrderExpr_.isEmpty())
  {
    xlist.insert(alternateRightChildOrderExpr_.rebuildExprTree());
    llist.insert("alternateRightChildOrderExpr");
  }

  RelExpr::addLocalExpr(xlist,llist);
}

const NAString Union::getText() const
{
  NAString text;
  switch (getUnionFlags())
  {
	case UNION_ORDERED    : text += "ordered_union"; break;
	case UNION_BLOCKED    : text += "blocked_union"; break;
	case UNION_COND_UNARY : text += "unary_union"; break;
	default               : text += "merge_union"; break;
  }

  if (getOperatorType() == REL_MERGE_UNION)
	  text += " (phys.)";

  return text;
}

ItemExpr *Union::getCondExprTree()
{
  return condExprTree_;
}

void Union::addCondExprTree(ItemExpr *condExpr)
{
  ExprValueId t = condExprTree_;

  ItemExprTreeAsList(&t, ITM_ITEM_LIST).insert(condExpr);
  condExprTree_ = t.getPtr();
}

ItemExpr *Union::removeCondExprTree()
{
  ItemExpr *result = condExprTree_;
  condExprTree_ = NULL;

  return result;
}

ItemExpr *Union::getTrigExceptExprTree()
{
  return trigExceptExprTree_;
}

void Union::addTrigExceptExprTree(ItemExpr *trigExceptExpr)
{
  ExprValueId t = trigExceptExprTree_;

  ItemExprTreeAsList(&t, ITM_ITEM_LIST).insert(trigExceptExpr);
  trigExceptExprTree_ = t.getPtr();
}

ItemExpr *Union::removeTrigExceptExprTree()
{
  ItemExpr *result = trigExceptExprTree_;
  trigExceptExprTree_ = NULL;

  return result;
}

// If this Union node is an IF node of a compound statement, this function
// returns either the left or right list of value ids associated with the node.
// It returns the left one if we are currently visiting the left child.
// Otherwise we return the right one.
AssignmentStHostVars *Union::getCurrentList(BindWA *bindWA)
{
  if (currentChild_ == 0) {
    if (!leftList_) {
      leftList_ = new (bindWA->wHeap()) AssignmentStHostVars(bindWA);
    }
    return leftList_;
  }
  else {
   if (!rightList_) {
     rightList_ = new (bindWA->wHeap()) AssignmentStHostVars(bindWA);
   }
   return rightList_;
  }
}


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
RETDesc * Union::createReturnTable(AssignmentStArea *assignArea, BindWA *bindWA)
{
  AssignmentStHostVars * leftList =  leftList_;
  AssignmentStHostVars * rightList = rightList_;
  NABoolean foundAMatch = FALSE;
  AssignmentStHostVars *globalList = assignArea->getAssignmentStHostVars();
  NAString const *nameOfLeftVar;

  RETDesc *resultTable = new (bindWA->wHeap()) RETDesc(bindWA);
  ColRefName *refName =  new (bindWA->wHeap()) ColRefName();

  AssignmentStHostVars *listOfPreviousIF = NULL;

  // We find the list of variables of the previous IF node. We will
  // need to update it since some of its variables may get new value ids
  // within the IF statement

  if (previousIF_) {

    short currentChild = previousIF_->currentChild();

    if (currentChild == 0) {
      listOfPreviousIF = previousIF_->leftList();
    }
    else {
      listOfPreviousIF = previousIF_->rightList();
    }
  }

  // Scan the left list and look for matches in the right List
  while (leftList && (leftList->var())) {

    foundAMatch = FALSE;

    nameOfLeftVar = &(leftList->var()->getName());

    rightList = rightList_;

    while (rightList && rightList->var()) {

       NAString const *nameOfRightVar = &(rightList->var()->getName());

       if (*nameOfLeftVar == *nameOfRightVar) {
	 foundAMatch = TRUE;
	 break;
       }

       rightList = rightList->next();
    }

    AssignmentStHostVars *ptrLeftVar = globalList->findVar(*nameOfLeftVar);
    CMPASSERT(ptrLeftVar);

    // If we found a match, we create a ValueIdUnion node of the paired match; otherwise
    // we pair the current value id of the variable in question with the value id it
    // had before the IF statement. If the variable does not have a value id, we bind it.
    ValueId value ;
    if (foundAMatch) {
      value = rightList->currentValueId();
    }
    else {
      ValueIdList list = ptrLeftVar->valueIds();
      if (list.entries() > 0) {
        value = ptrLeftVar->currentValueId();
      }
      else {
	// Get a value id for this variable.
	ItemExpr *expr = ptrLeftVar->var()->bindNode(bindWA);
	if (bindWA->errStatus()) {
          return NULL;
	}

	value = expr->getValueId();
      }
    }

    ValueIdUnion *vidUnion = new (bindWA->wHeap())
        ValueIdUnion(leftList->currentValueId(),
                     value,
	             NULL_VALUE_ID);
    vidUnion->bindNode(bindWA);
    if (bindWA->errStatus()) {
      delete vidUnion;
      return NULL;
    }

    ValueId valId = vidUnion->getValueId();
    addValueIdUnion(valId,bindWA->wHeap());

    resultTable->addColumn(bindWA, *refName, valId);

    // The variable inside the IF gets the value id of the ValueIdUnion just
    // generated.
    ptrLeftVar->setCurrentValueId(valId);

    // Also update the variable list in the previous IF node
    if (listOfPreviousIF) {
      listOfPreviousIF->addToListInIF(leftList->var(), valId);
    }

    leftList = leftList->next();
  } // while

  // We now search the right list and do a similar processing for the variables on
  // the right side that are not on the left
  rightList = rightList_;

  while (rightList && (rightList->var())) {

   foundAMatch = FALSE;

   NAString const *nameOfRightVar = &(rightList->var()->getName());

   AssignmentStHostVars *ptrRightVar = globalList->findVar(*nameOfRightVar);
   CMPASSERT(ptrRightVar);

   leftList = leftList_;

   while (leftList && (leftList->var())) {

     nameOfLeftVar = &(leftList->var()->getName());

     if (*nameOfLeftVar == *nameOfRightVar) {
       foundAMatch = TRUE;
       break;
     }

     leftList = leftList->next();
   }

    // Create the ValueIdUnion of the two value ids
    if (!foundAMatch) {
      ValueId value;
      ValueIdList list = ptrRightVar->valueIds();
      if (list.entries() > 0) {
        value = ptrRightVar->currentValueId();
      }
      else {
	// Get a value id for this variable.
	ItemExpr *expr = ptrRightVar->var()->bindNode(bindWA);
	value = expr->getValueId();
      }

      ValueIdUnion *vidUnion = new (bindWA->wHeap())
        ValueIdUnion(value, rightList->currentValueId(),
	             NULL_VALUE_ID);
      vidUnion->bindNode(bindWA);
      if (bindWA->errStatus()) {
        delete vidUnion;
        return NULL;
      }

      ValueId valId = vidUnion->getValueId();
      addValueIdUnion(valId, bindWA->wHeap());

      resultTable->addColumn(bindWA, *refName, valId);

      // The variable inside the IF gets the value id of the ValueIdUnion just
      // generated.
      ptrRightVar->setCurrentValueId(valId);

      // Also update the variable list in the previous IF node
      if (listOfPreviousIF) {
        listOfPreviousIF->addToListInIF(rightList->var(), valId);
      }

    }  // if (!foundAMatch)

    rightList = rightList->next();

  } // while

  return resultTable;
}

//++ MV -
void Union::addAlternateRightChildOrderExprTree(ItemExpr *alternateRightChildOrderExprTree)
{
  ExprValueId t = alternateRightChildOrderExprTree_;

  ItemExprTreeAsList(&t, ITM_ITEM_LIST).insert(alternateRightChildOrderExprTree);
  alternateRightChildOrderExprTree_ = t.getPtr();
}

ItemExpr *Union::removeAlternateRightChildOrderExprTree()
{
  ItemExpr *result = alternateRightChildOrderExprTree_;
  alternateRightChildOrderExprTree_ = NULL;

  return result;
}
// MV--

// -----------------------------------------------------------------------
// member functions for class MergeUnion
// -----------------------------------------------------------------------

MergeUnion::~MergeUnion() {}

NABoolean MergeUnion::isLogical()  const { return FALSE; }

NABoolean MergeUnion::isPhysical() const { return TRUE; }

HashValue MergeUnion::topHash()
{
  HashValue result = Union::topHash();

  // result ^= mergeExpr_;

  return result;
}

NABoolean MergeUnion::duplicateMatch(const RelExpr & other) const
{
  if (!RelExpr::duplicateMatch(other))
    return FALSE;

  MergeUnion &o = (MergeUnion &) other;

  // if (mergeExpr_ != o.mergeExpr_)

  ABORT("duplicateMatch shouldn't be called for physical nodes");
  return FALSE;

}

RelExpr * MergeUnion::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  MergeUnion *result;

  if (derivedNode == NULL)
    result = new (outHeap) MergeUnion(NULL,
				      NULL,
				      new (outHeap)UnionMap(*getUnionMap()),
				      getOperatorType(),
				      outHeap);
  else
    result = (MergeUnion *) derivedNode;

  result->mergeExpr_ = mergeExpr_;

  return Union::copyTopNode(result, outHeap);
}

void MergeUnion::setSortOrder(const ValueIdList &newSortOrder)
{
  sortOrder_ = newSortOrder;
  buildMergeExpr();
}

void MergeUnion::addLocalExpr(LIST(ExprNode *) &xlist,
			      LIST(NAString) &llist) const
{
  if (sortOrder_.entries() > 0)
    {
      xlist.insert(sortOrder_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("sort_order");
    }

  if (mergeExpr_ != NULL)
    {
      xlist.insert(mergeExpr_);
      llist.insert("merge_expr");
    }

  Union::addLocalExpr(xlist,llist);
}

void MergeUnion::buildMergeExpr()
{
  // ---------------------------------------------------------------------
  // build the merge expression (an expression that tells which of the
  // two input rows, left or right, should be returned next) by creating
  // an expression "left <= right" from the sort order.
  // ---------------------------------------------------------------------
  ItemExpr *leftList = NULL;
  ItemExpr *rightList = NULL;
  BiRelat *result = NULL;

  if (sortOrder_.entries() > 0)
    {
      for (Lng32 i = 0; i < (Lng32)sortOrder_.entries(); i++)
	{
	  ItemExpr *leftItem;
	  ItemExpr *rightItem;

	  leftItem = sortOrder_[i].getItemExpr()->
	    mapAndRewrite(getLeftMap(),TRUE).getItemExpr();
	  rightItem = sortOrder_[i].getItemExpr()->
	    mapAndRewrite(getRightMap(),TRUE).getItemExpr();

          // swap left and right if DESC is specified.
          if(leftItem->getOperatorType() == ITM_INVERSE)
          {
            // both streams must be sorted according to the same order.
            CMPASSERT(rightItem->getOperatorType() == ITM_INVERSE);
            ItemExpr *temp = leftItem;
            leftItem = rightItem;
            rightItem = temp;
          }

	  // add the newly formed fields of the sort key to the
	  // left and right lists of sort keys
	  if (leftList != NULL)
	    {
	      leftList = new (CmpCommon::statementHeap())
		             ItemList(leftList,leftItem);
	      rightList = new (CmpCommon::statementHeap())
		              ItemList(rightList,rightItem);
	    }
	  else
	    {
	      // both left and right list must be NULL
	      leftList = leftItem;
	      rightList = rightItem;
	    }
	}

      result = new (CmpCommon::statementHeap())
	           BiRelat(ITM_LESS_EQ,leftList,rightList);
      // make the comparison such that NULLs compare greater than instead
      // of making the expression result NULL
      result->setSpecialNulls(TRUE);

      result->synthTypeAndValueId();
    }

  // store the result in the merge expression
  mergeExpr_ = result;
}


// -----------------------------------------------------------------------
// member functions for class GroupByAgg
// -----------------------------------------------------------------------

GroupByAgg::~GroupByAgg() {}

Int32 GroupByAgg::getArity() const { return 1; }

void GroupByAgg::pushdownCoveredExpr(const ValueIdSet & outputExpr,
                                  const ValueIdSet & newExternalInputs,
                                  ValueIdSet & predicatesOnParent,
				  const ValueIdSet * setOfValuesReqdByParent,
				  Lng32 childIndex
		                 )
{
  // ---------------------------------------------------------------------
  // predicates can only be pushed down if the group by did contain
  // a group by clause or if this is a scalar groupby for a subquery that
  // contains null rejecting predicates. If the subquery contains null-rej.
  // preds then it does not need to do null instantiation for the empty
  // result set and therefore we do not create a separate VEGRegion for this
  // subquery. This means that preds can be freely pushed down in this case.
  // See GroupByAgg::pullUpPreds for a symmetric condition.
  // ---------------------------------------------------------------------
  ValueIdSet pushablePredicates;
  ValueIdSet exprOnParent;
  ValueIdSet emptySet;

  if (NOT groupExpr().isEmpty() || containsNullRejectingPredicates())
    pushablePredicates = predicatesOnParent;
#if 0
  else
    computeValuesReqdForPredicates(predicatesOnParent,
                                   exprOnParent);
#endif

  // ---------------------------------------------------------------------
  // Cause the retrieval of all those values that are needed for
  // computing the aggregate functions and the group by list.
  // ---------------------------------------------------------------------
  getValuesRequiredForEvaluatingAggregate(exprOnParent);
  exprOnParent += groupExpr();

  // ---------------------------------------------------------------------
  RelExpr::pushdownCoveredExpr(emptySet,
                               newExternalInputs,
                               pushablePredicates,
			       &exprOnParent,
                               childIndex
                               );

  // ---------------------------------------------------------------------
  // Set the value of predicatesOnParent appropriately.
  // ---------------------------------------------------------------------
  if (NOT groupExpr().isEmpty() || containsNullRejectingPredicates())
      predicatesOnParent.intersectSet(pushablePredicates);

} // GroupByAgg::pushdownCoveredExpr

void GroupByAgg::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();

  // Assign the grouping expressions and the aggregate functions
  // that are computed here as the outputs.
  //
  outputValues += groupExpr();
  outputValues += aggregateExpr();

  // If we're enforcing an ITM_ONE_ROW on (x,y), then we can produce not
  // merely the ITM_ONE_ROW, but also x and y, so add them to our outputs.
  // For example, if the aggregate is, say,
  //	ITM_ONE_ROW(VEGRef_10(T.A,ixT.A), VEGRef_15(T.B,ixT.B))
  //	{ example query: select * from S where (select A,B from T) < (100,200) }
  // then add value ids 10 and 11 to our characteristic outputs.
  //
  for (ValueId aggid = aggregateExpr().init();
       aggregateExpr().next(aggid);
       aggregateExpr().advance(aggid))
    {
      ItemExpr *aggie = aggid.getItemExpr();
      if (aggie->getOperatorType() == ITM_ONE_ROW)
        {
	  ValueIdSet moreAvailableOutputs;
	  aggie->child(0)->convertToValueIdSet(moreAvailableOutputs,
					       NULL, ITM_ITEM_LIST, FALSE);
	  outputValues += moreAvailableOutputs;
	}
    }
} // GroupByAgg::getPotentialOutputValues()

const NAString GroupByAgg::getText() const
{
  if (NOT groupExpr().isEmpty())
    {
      if (isNotAPartialGroupBy())
	return "groupby";
      else if (isAPartialGroupByRoot())
	return "partial_groupby_root";
      else if (isAPartialGroupByNonLeaf())
	return "partial_groupby_non_leaf";
      else
	return "partial_groupby_leaf";
    }
 else
   {
      if (isNotAPartialGroupBy())
	return "scalar_aggr";
      else if (isAPartialGroupByRoot())
	return "partial_aggr_root";
      else if (isAPartialGroupByNonLeaf())
	return "partial_aggr_non_leaf";
      else
	return "partial_aggr_leaf";
   }

} // GroupByAgg::getText()

HashValue GroupByAgg::topHash()
{
  HashValue result = RelExpr::topHash();

  result ^= groupExpr_;
  result ^= aggregateExpr_;
  result ^= (Int32) formEnum_;        // MSVC requires cast.

  return result;
}

NABoolean GroupByAgg::duplicateMatch(const RelExpr & other) const
{
  if (!RelExpr::duplicateMatch(other))
    return FALSE;

  GroupByAgg &o = (GroupByAgg &) other;

  if (groupExpr_     != o.groupExpr_ OR
      aggregateExpr_ != o.aggregateExpr_ OR
      formEnum_ != o.formEnum_ )

    return FALSE;

  return TRUE;
}

RelExpr * GroupByAgg::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  GroupByAgg *result;

  if (derivedNode == NULL)
    result = new (outHeap) GroupByAgg(NULL,
		                      getOperatorType(),
				      NULL,
				      NULL,
				      outHeap);
  else
    result = (GroupByAgg *) derivedNode;

  // copy parse tree nodes (parser only)
  if (groupExprTree_ != NULL)
    result->groupExprTree_ = groupExprTree_->copyTree(outHeap);
  if (aggregateExprTree_ != NULL)
    result->aggregateExprTree_ = aggregateExprTree_->copyTree(outHeap);

  result->groupExpr_     = groupExpr_;
  result->rollupGroupExprList_ = rollupGroupExprList_;
  result->aggregateExpr_ = aggregateExpr_;
  result->formEnum_      = formEnum_;
  result->gbAggPushedBelowTSJ_ = gbAggPushedBelowTSJ_;
  result->gbAnalysis_ = gbAnalysis_;
  result->requiresMoveUp_ = requiresMoveUp_ ;
  result->leftUniqueExpr_ = leftUniqueExpr_;
  result->containsNullRejectingPredicates_ = containsNullRejectingPredicates_ ;
  result->parentRootSelectList_ = parentRootSelectList_;
  result->isMarkedForElimination_ = isMarkedForElimination_;

  result->selIndexInHaving_ = selIndexInHaving_;
  result->aggrExprsToBeDeleted_ = aggrExprsToBeDeleted_;

  result->isRollup_ = isRollup_;
  result->extraGrpOrderby_= extraGrpOrderby_;
  result->extraOrderExpr_= extraOrderExpr_;

  return RelExpr::copyTopNode(result, outHeap);
}

void GroupByAgg::addGroupExprTree(ItemExpr *groupExpr)
{
  ExprValueId g = groupExprTree_;

  ItemExprTreeAsList(&g, ITM_ITEM_LIST).insert(groupExpr);
  groupExprTree_ = g.getPtr();
}

ItemExpr * GroupByAgg::removeGroupExprTree()
{
  ItemExpr * result = groupExprTree_;

  groupExprTree_ = NULL;

  return result;
}

void GroupByAgg::addAggregateExprTree(ItemExpr *aggrExpr)
{
  ExprValueId g = groupExprTree_;

  ItemExprTreeAsList(&g, ITM_ITEM_LIST).insert(aggrExpr);
  groupExprTree_ = g.getPtr();
}

ItemExpr * GroupByAgg::removeAggregateExprTree()
{
  ItemExpr * result = aggregateExprTree_;

  aggregateExprTree_ = NULL;

  return result;
}

void GroupByAgg::getValuesRequiredForEvaluatingAggregate(ValueIdSet& relevantValues)
{
  // Find the values that are needed to evaluate aggregate functions.
  // NOTE: this should normally just be the direct children of the
  // aggregate functions. However, some aggregate functions such as
  // anyTrue sometimes refer to values further down the tree (and
  // if it's only by using such values as required sort orders).
  // Handle this special case here (or maybe we should have changed
  // the anyTrue aggregate function such that it takes separate arguments:
  // anyTrueGreater(a,b), anyTrueLess(a,b), anyTrueGreaterEq(a,b), ...

  // for each aggregate expression in the groupby node
  for (ValueId x = aggregateExpr_.init();
       aggregateExpr_.next(x);
       aggregateExpr_.advance(x))
    {
      Aggregate *agg = (Aggregate *) x.getItemExpr();
      Lng32 nc = agg->getArity();

      // handle special cases for special aggregate functions
      switch (agg->getOperatorType())
	{
	case ITM_ANY_TRUE:
	case ITM_ANY_TRUE_MAX:
	  {
	    ItemExpr *boolInput = agg->child(0);

	    // if the child is a binary comparison operator, then
	    // require both of the children instead of the comparison op.
	    switch (boolInput->getOperatorType())
	      {
	      case ITM_EQUAL:
	      case ITM_NOT_EQUAL:
	      case ITM_LESS:
	      case ITM_LESS_EQ:
	      case ITM_GREATER:
	      case ITM_GREATER_EQ:
		relevantValues += boolInput->child(0)->getValueId();
		relevantValues += boolInput->child(1)->getValueId();
		break;
              case ITM_VEG_PREDICATE:
                {
                  VEG * vegPtr = ((VEGPredicate *)boolInput)->getVEG();
                  relevantValues += vegPtr->getVEGReference()->getValueId();
		}
                break;
	      default:

		// might not happen right now: an anyTrue with something
		// other than a binary comparison operator
		relevantValues += boolInput->getValueId();
		break;
	      }
	  }

	  break;

        case ITM_ONE_ROW:
         {
           // collect leaf values into relevant Values
           ValueIdSet AvailableOutputs_;
           agg->child(0)->convertToValueIdSet(AvailableOutputs_,
                          NULL, ITM_ITEM_LIST, FALSE);
           relevantValues += AvailableOutputs_;
           break;
         }
	default:
	  {
	    // all other aggregate functions are handled here
            //
            // If we are doing a distinct aggregate we need the
            // distinct value id. E.g. sum(distinct x*x) with distinct
            //                         valueId x, means we eliminate
            //                         distinct x's first, then compute sum(x*x)
            if(agg->isDistinct())
              relevantValues += agg->getDistinctValueId();
            else
              relevantValues += agg->child(0)->getValueId();

	    // for each child of this particular aggregate expression
	    for (Lng32 i = 1; i < nc; i++)
	      {
		// add the value id of that child to "relevantValues"
		relevantValues += agg->child(i)->getValueId();
	      }
	  }
	  break;
	}
    }
}

void GroupByAgg::addLocalExpr(LIST(ExprNode *) &xlist,
			      LIST(NAString) &llist) const
{
  if (groupExprTree_ != NULL OR
      NOT groupExpr_.isEmpty())
    {
      if (groupExpr_.isEmpty())
	xlist.insert(groupExprTree_);
      else if (isRollup() && (NOT rollupGroupExprList_.isEmpty()))
 	xlist.insert(rollupGroupExprList_.rebuildExprTree(ITM_ITEM_LIST));
      else
	xlist.insert(groupExpr_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("grouping_columns");
    }

  if (aggregateExprTree_ != NULL OR
      NOT aggregateExpr_.isEmpty())
    {
      if (aggregateExpr_.isEmpty())
	xlist.insert(aggregateExprTree_);
      else
	xlist.insert(aggregateExpr_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("aggregates");
    }

  RelExpr::addLocalExpr(xlist,llist);
}

// -----------------------------------------------------------------------
// Examine the aggregate functions. If any one of them cannot be
// evaluated in stages, for example, a partial aggregation followed
// by finalization, then do not split this GroupByAgg.
// -----------------------------------------------------------------------
 NABoolean GroupByAgg::aggregateEvaluationCanBeStaged() const
{
  for (ValueId aggrId = aggregateExpr().init();
       aggregateExpr().next(aggrId);
       aggregateExpr().advance(aggrId))
    {
      CMPASSERT(aggrId.getItemExpr()->isAnAggregate());
      if (groupExpr().isEmpty() && 
	  (aggrId.getItemExpr()->getOperatorType() == ITM_ONEROW))
	    return FALSE;

      if (NOT ((Aggregate *)aggrId.getItemExpr())->evaluationCanBeStaged())
	return FALSE;
    }
  return TRUE;
} // GroupByAgg::aggregateEvaluationCanBeStaged()

NABoolean GroupByAgg::executeInDP2() const
{
  CMPASSERT(getPhysicalProperty());
  return getPhysicalProperty()->executeInDP2();
}

// Try to pull up predicates in preCodeGen, to reduce char. inputs of the
// child. Don't actually do this unless "modify" parameter is set to TRUE,
// (we want to test this condition in the optimizer for costing).
// Return TRUE if we could move some predicates.
// Could make this a virtual method on RelExpr if we want to support
// this for other operators as well.
NABoolean GroupByAgg::tryToPullUpPredicatesInPreCodeGen(
     const ValueIdSet &valuesAvailableInParent,    // pull preds that are covered by these
     ValueIdSet       &pulledPredicates,           // return the pulled-up preds
     ValueIdMap       *optionalMap)                // optional map to rewrite preds
{
  // other item expressions needed by the child (excluding
  // selection preds), this is where we make use of the knowledge
  // that we are dealing with a groupby. 
  ValueIdSet myLocalExpr;
  ValueIdSet myNewInputs(getGroupAttr()->getCharacteristicInputs());
  ValueIdSet mappedValuesAvailableInParent;
  ValueIdSet tempPulledPreds(selectionPred()); // be optimistic

  myLocalExpr += child(0).getGroupAttr()->getCharacteristicInputs();
  myLocalExpr += groupExpr();
  myLocalExpr += aggregateExpr();
  // make sure we can still produce our characteristic outputs too
  myLocalExpr += getGroupAttr()->getCharacteristicOutputs();
          
  // consider only preds that we can evaluate in the parent
  if (optionalMap)
    optionalMap->mapValueIdSetDown(valuesAvailableInParent,
                                   mappedValuesAvailableInParent);
  else
    mappedValuesAvailableInParent = valuesAvailableInParent;
  tempPulledPreds.removeUnCoveredExprs(mappedValuesAvailableInParent);

  // add the rest to myLocalExpr
  myLocalExpr += selectionPred();
  myLocalExpr -= tempPulledPreds;
              
  // see which of the char. inputs are needed by my local expressions
  myLocalExpr.weedOutUnreferenced(myNewInputs);
          
  // pull up predicates only if that reduces my char. inputs
  if (NOT (myNewInputs == getGroupAttr()->getCharacteristicInputs()))
    {
      ValueIdSet selPredOnlyInputs(getGroupAttr()->getCharacteristicInputs());
          
      // inputs only used by selection predicates
      selPredOnlyInputs -= myNewInputs;

      // loop through the selection predicates and pull
      // those up that reference myNewInputs
      for (ValueId x=tempPulledPreds.init();
           tempPulledPreds.next(x);
           tempPulledPreds.advance(x))
        {
          if (x.getItemExpr()->referencesOneValueFrom(selPredOnlyInputs))
            {
              // keep this predicate in tempPulledPreds and
              // remove it from the selection predicates
              selectionPred() -= x;
            }
          else
            {
              // this predicate stays on the local node,
              // remove it from tempPulledPreds
              tempPulledPreds -= x;
            }
        }
    }
  else
    {
      // no predicates get pulled up
      tempPulledPreds.clear();
    }

  if (!tempPulledPreds.isEmpty())
    {
      // return pulled predicates
      if (optionalMap)
        {
          ValueIdSet rewrittenPulledPreds;

          optionalMap->rewriteValueIdSetUp(rewrittenPulledPreds, tempPulledPreds);
          pulledPredicates += rewrittenPulledPreds;
        }
      else
        pulledPredicates += tempPulledPreds;

      // just remove pulled up predicates from char. input
      ValueIdSet newInputs(getGroupAttr()->getCharacteristicInputs());
      myLocalExpr += selectionPred();
      myLocalExpr -= tempPulledPreds;
      myLocalExpr.weedOutUnreferenced(newInputs);
      
      // adjust char. inputs - this is not exactly
      // good style, just overwriting the char. inputs, but
      // hopefully we'll get away with it at this stage in
      // the processing
      getGroupAttr()->setCharacteristicInputs(newInputs);
    }

  // note that we removed these predicates from our node, it's the
  // caller's responsibility to take them
  return (NOT tempPulledPreds.isEmpty());
}

// -----------------------------------------------------------------------
// member functions for class SortGroupBy
// -----------------------------------------------------------------------

SortGroupBy::~SortGroupBy() {}

NABoolean SortGroupBy::isLogical()  const {return FALSE;}

NABoolean SortGroupBy::isPhysical() const {return TRUE;}

const NAString SortGroupBy::getText() const
{
  if (isRollup())
    return "sort_" + GroupByAgg::getText() + "_rollup";
  else
    return "sort_" + GroupByAgg::getText();
}

RelExpr * SortGroupBy::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) SortGroupBy(NULL,
				       getOperatorType(),
				       NULL,
				       NULL,
				       outHeap);
  else
    result = derivedNode;

  return GroupByAgg::copyTopNode(result, outHeap);
}

PlanPriority ShortCutGroupBy::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{
  // For min(X) or max(X) where X is the first clustering key column,
  // allow shortcutgroupby plan to compete with other plans based on cost.
  // Specifically, match the priority of the wave fix so that a
  // shortcutgroupby plan can cost compete with the parallel 
  // partialgroupby plan.
  PlanPriority result;
  if (QueryAnalysis::Instance() &&
      QueryAnalysis::Instance()->dontSurfTheWave()) {
   // do this only if the wave fix is a competing plan
    result.incrementLevels(10, 0);
  }
  return result;
}

PlanPriority SortGroupBy::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{
  const PhysicalProperty* spp = context->getPlan()->getPhysicalProperty();
  Lng32 degreeOfParallelism = spp->getCountOfPartitions();
  double val = 1;
  if (degreeOfParallelism <= 1)
    {
      // serial plans are risky. exact an insurance premium from serial plans.
      val = CURRSTMT_OPTDEFAULTS->riskPremiumSerial();

      // when dontSurfTheWave is ON, 
      // consider serial sort_partial_aggr_nonleaf risky 
      if (QueryAnalysis::Instance() &&
          QueryAnalysis::Instance()->dontSurfTheWave() &&
          isAPartialGroupByNonLeaf() && val <= 1)
        {
          val = 1.1; 
        }
    }
  CostScalar premium(val);
  PlanPriority result(0, 0, premium);

  // WaveFix Begin
  // This is part of the fix for the count(*) wave
  // if there is a scalar aggregate query on a single partitioned table,
  // something like Select count(*) from fact;
  // In such a case we would like to get a layer of esps,
  // doing so causes the plan to fixup in parallel avoiding the serial
  // fixup if the plan is just the master executor on top of dp2. The
  // serial fixup causes the query to execute in wave pattern, since
  // each dp2 is fixed up and then starts execution. Due to serial
  // fixup a dp2 is fixed up, and then we move to the next dp2 causing
  // the wave pattern.
  if (QueryAnalysis::Instance() &&
      QueryAnalysis::Instance()->dontSurfTheWave())
  {
    if (isAPartialGroupByLeaf2() && spp->executeInDP2())
      result.incrementLevels(10, 0);
    else if (isAPartialGroupByLeaf1() &&
            (degreeOfParallelism>1) &&
            (!spp->executeInDP2()))
      result.incrementLevels(5, 0);
  }
  // WaveFix End

  // The remaining part of the code in this function relates to parallelism
  // priority and not applicable to scalar aggregates
  if (groupExpr().isEmpty())
    return result;

  if(spp->executeInDP2())
    return result;

  // For the option of Max Degree of Parallelism we can either use the
  // value set in comp_int_9 (if positive) or we use the number of CPUs
  // if the CQD is set to -1, or feature is disabled if CQD is 0 (default).
  Lng32 maxDegree = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_9);
  if (CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible() OR (maxDegree == -1) )
  {
    // if CQD is set to -1 this mean use the number of CPUs
    maxDegree = spp->getCurrentCountOfCPUs();
  }
  if (maxDegree > 1) // CQD set to 0 means feature is OFF
  {
    if (degreeOfParallelism < maxDegree)
      result.incrementLevels(0,-10); // need to replace with constant
  }

  return result;
}

// -----------------------------------------------------------------------
// member functions for class ShortCutGroupBy
// -----------------------------------------------------------------------

const NAString ShortCutGroupBy::getText() const
{
  return "shortcut_" + GroupByAgg::getText();
}

RelExpr * ShortCutGroupBy::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ShortCutGroupBy *result;

  if (derivedNode == NULL)
    // This is the top of the derivation chain
    // Create an empty ShortCutGroupBy node.
    //
    result = new (outHeap) ShortCutGroupBy(NULL,
					  getOperatorType(),
					  NULL,
					  NULL,
					  outHeap);
  else
    // A node has already been constructed as a derived class.
    //
    result = (ShortCutGroupBy *) derivedNode;

  // Copy the relevant fields.

  result->opt_for_max_ = opt_for_max_;
  result->opt_for_min_ = opt_for_min_;
  result->isnullable_ = isnullable_;
  result->lhs_anytrue_ = lhs_anytrue_;
  result->rhs_anytrue_ = rhs_anytrue_;

  // Copy any data members from the classes lower in the derivation chain.
  //
  return GroupByAgg::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class PhysShortCutGroupBy
// -----------------------------------------------------------------------

RelExpr * PhysShortCutGroupBy::copyTopNode(RelExpr *derivedNode,
                                           CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    // This is the top of the derivation chain
    // Create an empty ShortCutGroupBy node.
    //
    result = new (outHeap) PhysShortCutGroupBy(NULL,
					       getOperatorType(),
					       NULL,
					       NULL,
					       outHeap);
  else
    // A node has already been constructed as a derived class.
    //
    result = (PhysShortCutGroupBy *) derivedNode;

  // PhysShortCutGroupBy has no data members.

  // Copy any data members from the classes lower in the derivation chain.
  //
  return ShortCutGroupBy::copyTopNode(result, outHeap);
}


// -----------------------------------------------------------------------
// member functions for class HashGroupBy
// -----------------------------------------------------------------------

HashGroupBy::~HashGroupBy() {}

NABoolean HashGroupBy::isLogical()  const {return FALSE;}

NABoolean HashGroupBy::isPhysical() const {return TRUE;}

const NAString HashGroupBy::getText() const
{
  return "hash_" + GroupByAgg::getText();
}

RelExpr * HashGroupBy::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HashGroupBy(NULL,
				       getOperatorType(),
				       NULL,
				       NULL,
				       outHeap);
  else
    result = derivedNode;

  return GroupByAgg::copyTopNode(result, outHeap);
}


PlanPriority HashGroupBy::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{
  const PhysicalProperty* spp = context->getPlan()->getPhysicalProperty();
  Lng32 degreeOfParallelism = spp->getCountOfPartitions();
  double val = 1;
  if (degreeOfParallelism <= 1)
    {

    // Don't command premium for serial hash partial groupby plan if :
    // 1. Operator is partial group by root
    // 2. process < 5K rows
    // This is to prevent optimizer choosing parallel plans for small queries.
    // The idea is either premium has been already applied for groupby leaf level
    // or leaf is running in parallel, we don't need to run root also in parallel

       if ( isAPartialGroupByRoot() && 
            CostScalar((ActiveSchemaDB()->getDefaults()).
              getAsULong(GROUP_BY_PARTIAL_ROOT_THRESHOLD)) >=
                  this->getChild0Cardinality(context) )
          val = 1;
       else
          // serial plans are risky. extract an insurance premium from serial plans.

          val = CURRSTMT_OPTDEFAULTS->riskPremiumSerial();
    }

  CostScalar premium(val);
  PlanPriority result(0, 0, premium);

  if (QueryAnalysis::Instance() AND
      QueryAnalysis::Instance()->optimizeForFirstNRows())
    result.incrementLevels(HASH_GROUP_BY_FIRST_N_PRIORITY,0);

  // The remaining part of the code in this funtion relates to parallelism
  // priority and not applicable to scalar aggregates
  if (groupExpr().isEmpty())
    return result;

  // esp parallelism priority logic does not apply to operators in dp2
  if(spp->executeInDP2())
    return result;

  // For the option of Max Degree of Parallelism we can either use the
  // value set in comp_int_9 (if positive) or we use the number of CPUs
  // if the CQD is set to -1, or feature is disabled if CQD is 0 (default).
  Lng32 maxDegree = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_9);
  if (CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible() OR (maxDegree == -1) )
  {
    // if CQD is set to -1 this mean use the number of CPUs
    maxDegree = spp->getCurrentCountOfCPUs();
  }
  if (maxDegree > 1) // CQD set to 0 means feature is OFF
  {
    if (degreeOfParallelism < maxDegree)
      result.incrementLevels(0,-10); // need to replace with constant
  }

  //cout<<maxDegree<<"-------"<<spp->getCountOfPartitions()<<endl;

  return result;
}

// -----------------------------------------------------------------------
// member functions for class Scan
// -----------------------------------------------------------------------

void Scan::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  //
  // Assign the set of columns that belong to the table to be scanned
  // as the output values that can be produced by this scan.
  //
  if (potentialOutputs_.isEmpty())
    {
      outputValues.insertList( getTableDesc()->getColumnList() );
      outputValues.insertList( getTableDesc()->hbaseTSList() );
      outputValues.insertList( getTableDesc()->hbaseVersionList() );
    }
  else
    outputValues = potentialOutputs_;

  outputValues += getExtraOutputColumns();
} // Scan::getPotentialOutputValues()

void Scan::getPotentialOutputValuesAsVEGs(ValueIdSet& outputs) const
{
  outputs.clear();
  ValueIdSet tempSet ;

  getPotentialOutputValues(tempSet);
  getTableDesc()->getEquivVEGCols(tempSet, outputs); 
}



Int32 Scan::getArity() const { return 0;}

NABoolean Scan::isHiveTable() const 
{
  return (getTableDesc() && getTableDesc()->getNATable() ?
	  getTableDesc()->getNATable()->isHiveTable() :
	  FALSE);
}

NABoolean Scan::isHbaseTable() const 
{
  return (getTableDesc() && getTableDesc()->getNATable() ?
	  getTableDesc()->getNATable()->isHbaseTable() :
	  FALSE);
}

NABoolean Scan::isSeabaseTable() const 
{
  return (getTableDesc() && getTableDesc()->getNATable() ?
	  getTableDesc()->getNATable()->isSeabaseTable() :
	  FALSE);
}

const NAString Scan::getText() const
{
  NAString op(CmpCommon::statementHeap());

  if (isSampleScan() == TRUE)
    op = "sample_scan ";
  else
    op = "scan ";
  return op + userTableName_.getTextWithSpecialType();
}

HashValue Scan::topHash()
{
  HashValue result = RelExpr::topHash();

  result ^= getTableDesc();
  result ^= potentialOutputs_;
  result ^= numIndexJoins_;

  return result;
}

NABoolean Scan::duplicateMatch(const RelExpr & other) const
{
  if (NOT RelExpr::duplicateMatch(other))
    return FALSE;

  Scan &o = (Scan &) other;

  if (NOT (userTableName_ == o.userTableName_) OR
      NOT (getTableDesc() == o.getTableDesc()) OR
      NOT (potentialOutputs_ == o.potentialOutputs_) OR
      ((forcedIndexInfo_ OR o.forcedIndexInfo_) AND (
      //just comparing the entries is probably not enough????
      NOT (indexOnlyIndexes_.entries() == o.indexOnlyIndexes_.entries()) OR
      NOT (possibleIndexJoins_ == o.possibleIndexJoins_) OR
      NOT (numIndexJoins_ == o.numIndexJoins_)))
      OR
      NOT (suppressHints_ == o.suppressHints_) OR
      NOT (isSingleVPScan_ == o.isSingleVPScan_) OR
      NOT (getExtraOutputColumns() == o.getExtraOutputColumns()) OR
      NOT (samplePercent() == o.samplePercent()) OR
      NOT (clusterSize() == o.clusterSize()))
    return FALSE;

  return TRUE;
}

RelExpr * Scan::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Scan *result;

  if (derivedNode == NULL)
    result = new (outHeap)
                 Scan(userTableName_, getTableDesc(), REL_SCAN, outHeap);
  else
    result = (Scan *) derivedNode;

  result->baseCardinality_  = baseCardinality_;
  result->potentialOutputs_ = potentialOutputs_;
  result->numIndexJoins_    = numIndexJoins_;
  result->accessOptions_    = accessOptions_;
  result->pkeyHvarList_     = pkeyHvarList_;
  result->setOptStoi(stoi_);
  result->samplePercent(samplePercent());
  result->suppressHints_    = suppressHints_;
  result->clusterSize(clusterSize());
  result->scanFlags_ = scanFlags_;
  result->setExtraOutputColumns(getExtraOutputColumns());
  result->isRewrittenMV_ = isRewrittenMV_;
  result->matchingMVs_ = matchingMVs_;
  result->hbaseAccessOptions_ = hbaseAccessOptions_;
  result->commonSubExpr_ = commonSubExpr_;

  // don't copy values that can be calculated by addIndexInfo()
  // (some callers will change selection preds, which requires recomputation)

  return RelExpr::copyTopNode(result, outHeap);
}

void Scan::copyIndexInfo(RelExpr *derivedNode)
{
  CMPASSERT (derivedNode != NULL AND
      derivedNode->getOperatorType() == REL_SCAN);

  Scan * scan = (Scan *)derivedNode;
  forcedIndexInfo_ = scan->forcedIndexInfo_;
  if (NOT scan->getIndexOnlyIndexes().isEmpty() OR
      scan->getPossibleIndexJoins().entries() > 0)
  {
    // first copy the possible index join info
    const LIST(ScanIndexInfo *) & ixJoins = scan->getPossibleIndexJoins();
    for (CollIndex i = 0; i < ixJoins.entries(); i++)
    {
      ScanIndexInfo * ix = new (CmpCommon::statementHeap())
	ScanIndexInfo(*(ixJoins[i]));
      possibleIndexJoins_.insert(ix);
    }

    // now, copy the index descriptors
    const SET(IndexProperty *) & ixDescs = scan->getIndexOnlyIndexes();

    for (CollIndex j = 0; j <ixDescs.entries(); j++)
    {
      indexOnlyIndexes_.insert(ixDescs[j]);
    }
  }
  generatedCCPreds_ = scan->generatedCCPreds_;
}

void Scan::removeIndexInfo()
{
	possibleIndexJoins_.clear();
	indexOnlyIndexes_.clear();
        indexJoinScans_.clear();
        generatedCCPreds_.clear();
	forcedIndexInfo_ = FALSE;
}

/*******************************************************
* Generates set of IndexDesc from the set of IndexProperty
********************************************************/

const SET(IndexDesc *) & Scan::deriveIndexOnlyIndexDesc()
{
    indexOnlyScans_.clear();
    CollIndex ixCount = indexOnlyIndexes_.entries();
    for(CollIndex i=0; i< ixCount; i++)
    {
      indexOnlyScans_.insert(indexOnlyIndexes_[i]->getIndexDesc());
    }
    return indexOnlyScans_;

}

/*******************************************************
* Generates set of IndexDesc from the set of ScanIndexInfo
********************************************************/
const SET(IndexDesc *) & Scan::deriveIndexJoinIndexDesc()
{
    indexJoinScans_.clear();
    CollIndex ijCount = possibleIndexJoins_.entries();
    for(CollIndex i=0; i< ijCount; i++)
    {
      ScanIndexInfo *ixi = possibleIndexJoins_[i];
      CollIndex uixCount = ixi->usableIndexes_.entries();
      for(CollIndex j=0; j < uixCount; j++)
      {
        if (ixi->usableIndexes_[j]->getIndexDesc())
          indexJoinScans_.insert(ixi->usableIndexes_[j]->getIndexDesc());
      }
    }
    return indexJoinScans_;
}

void Scan::addIndexInfo()
{
  // don't do this twice, return if already set
  if (NOT indexOnlyIndexes_.isEmpty() OR
      possibleIndexJoins_.entries() > 0)
    return;

  forcedIndexInfo_ = FALSE;
  const TableDesc     * tableDesc = getTableDesc();
  const LIST(IndexDesc *) & ixlist = tableDesc->getIndexes();
  ValueIdSet preds = selectionPred();
  // changing back to old predicate tree:
  if ((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
      (preds.entries()))
  {
    ValueIdList selectionPredList(preds);
    ItemExpr *inputItemExprTree = selectionPredList.rebuildExprTree(ITM_AND,FALSE,FALSE);
    ItemExpr * resultOld = revertBackToOldTree(CmpCommon::statementHeap(), inputItemExprTree);
    preds.clear();
    resultOld->convertToValueIdSet(preds, NULL, ITM_AND);
    doNotReplaceAnItemExpressionForLikePredicates(resultOld,preds,resultOld);
  }


  if (CmpCommon::getDefault(MTD_GENERATE_CC_PREDS) == DF_ON)
    {
      // compute predicates on computed columns from regular predicates, based
      // on the definition of the computed column. Example: 
      // - regular predicate: a = 99
      // - computed column definition: "_SALT_" = HASH2PARTFUNC(a,2)
      // - computed predicate: "_SALT_" = HASH2PARTFUNC(99,2);
      ValueIdSet clusteringKeyCols(
           getTableDesc()->getClusteringIndex()->getClusteringKeyCols());
      ValueIdSet selectionPreds(preds);

      ScanKey::createComputedColumnPredicates(
           selectionPreds,
           clusteringKeyCols,
           getGroupAttr()->getCharacteristicInputs(),
           generatedCCPreds_);
    }

  // a shortcut for tables with no indexes
  if ((ixlist.entries() == 1)||
      (tableDesc->isPartitionNameSpecified()))
    {
      // that's easy, there is only one index (the base table)
      // and that index better have everything we need
      IndexJoinSelectivityEnum junk;
      MdamFlags flag=ixlist[0]->pruneMdam(preds,TRUE,junk);;
      IndexProperty * ixProp = new(CmpCommon::statementHeap())
			       IndexProperty(ixlist[0],
					     flag);
      indexOnlyIndexes_.insert(ixProp);
      return;
    }

  // all the value ids that are required by the scan and its parents
  ValueIdSet requiredValueIds(getGroupAttr()->getCharacteristicOutputs());


  // VEGPreds can have two forms, an  A IS NOT NULL form and an A=B form
  // when expanded in the generator. If an index does not provide a
  // VEG member that the base table provides, a VEGPredicate could be
  // covered by the index in its IS NOT NULL form (checking a char. input
  // whether it is not null). To avoid this bug, add all the base cols
  // that contribute to VEGPredicates as explicitly required values.
  addBaseColsFromVEGPreds(requiredValueIds);

  // using old predicate tree:
  if ((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
      (preds.entries()))
  {
    requiredValueIds += preds;
  }
  else
    // selection predicates are also required, add them to requiredValueIds
    requiredValueIds += selectionPred();

  // a list of VEGReferences to the clustering key column(s)
  ValueIdSet clusteringKeyColumns;

  // some helper variables
  ValueIdList clusteringKeyColList;

  // a set of join predicates between indexes (same for all indexes)
  ValueIdSet indexJoinPreds;

  // ---------------------------------------------------------------------
  // find out the subset of values that are always covered
  // ---------------------------------------------------------------------
  // get the clustering key columns and transform them into VEGies
  CMPASSERT(tableDesc);
  tableDesc->getEquivVEGCols(
       tableDesc->getClusteringIndex()->getIndexKey(),
       clusteringKeyColList);
  clusteringKeyColumns = clusteringKeyColList;

  // get the VEGPredicates from the list of VEGReferences; they are
  // the join predicates between indexes (who all contain the clustering key)
  for (ValueId x = clusteringKeyColumns.init();
       clusteringKeyColumns.next(x);
       clusteringKeyColumns.advance(x))
    {
      // clustering key columns must be VEGReferences
      CMPASSERT(x.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE);

      if (((VEGReference *) x.getItemExpr())->getVEG()->getSpecialNulls())
        ((VEGReference *) x.getItemExpr())
               ->getVEG()->getVEGPredicate()->setSpecialNulls(TRUE);

      // find the corresponding VEGPredicate and add it to the join preds
      indexJoinPreds += ((VEGReference *) x.getItemExpr())->
	getVEG()->getVEGPredicate()->getValueId();
    }


  const NABoolean updatingCol = tableDesc->getColUpdated().entries() > 0;
  const NABoolean unlimitedIndexJoinsAllowed =
          ((ActiveControlDB()->getRequiredShape() AND
	    ActiveControlDB()->getRequiredShape()->getShape() AND
	    NOT ActiveControlDB()->getRequiredShape()->getShape()->isCutOp())
           OR
           (getGroupAttr()->isEmbeddedUpdateOrDelete())
           OR
	   (getGroupAttr()->isStream())
          );

		      
  const TableAnalysis * tAnalysis = getTableDesc()->getTableAnalysis();

  // with this CQD value set, try to consider minimum indexes possible
  // if ixProp is no better than any of indexOnlyIndexes_ - don't add
  // it. If ixProp is better than some of this set - remove them.
  NABoolean tryToEliminateIndex = 
             CURRSTMT_OPTDEFAULTS->indexEliminationLevel() == OptDefaults::AGGRESSIVE
             AND NOT unlimitedIndexJoinsAllowed 
             AND tAnalysis;

  CostScalar indexEliminationThreshold = 
                ActiveSchemaDB()->getDefaults().getAsLong(INDEX_ELIMINATION_THRESHOLD); 


  NABoolean printIndexElimination = 
         CmpCommon::getDefault(NSK_DBG_PRINT_INDEX_ELIMINATION) == DF_ON &&
         CmpCommon::getDefault(NSK_DBG) == DF_ON;

  ostream &out = CURRCONTEXT_OPTDEBUG->stream();

  if ( printIndexElimination ) {
    out << endl << "call addIndexInfo()" << endl;
    out << "tryToEliminateIndex=" << (Lng32)tryToEliminateIndex << endl;
  }

  // ---------------------------------------------------------------------
  // For each index, check whether it provides any useful values
  // ---------------------------------------------------------------------
  for (CollIndex indexNo = 0; indexNo < ixlist.entries(); indexNo++)
    {
      IndexDesc *idesc = ixlist[indexNo];

      NABoolean dummy; // halloweenProtection is decided using updateableIndex
      // in GU::normalizeNode. Here this parameter is not used.
      // Determine if this index can be used for a scan during an update.
      if (updatingCol AND NOT updateableIndex(idesc, preds, dummy))
        continue;

      ValueIdSet indexColumns(idesc->getIndexColumns());
      ValueIdSet referencedInputs;
      ValueIdSet coveredSubexpr;
      ValueIdSet unCoveredExpr;
      GroupAttributes indexOnlyGA;
      NABoolean indexOnlyScan;

      // make group attributes for an index scan
      indexOnlyGA.addCharacteristicOutputs(idesc->getIndexColumns());
      indexOnlyGA.addCharacteristicOutputs(extraOutputColumns_);

      // does the index cover all required values, and if not, which
      // ones does it cover and which ones are not covered
      indexOnlyScan = requiredValueIds.isCovered(
	   getGroupAttr()->getCharacteristicInputs(),
	   indexOnlyGA,
	   referencedInputs,
	   coveredSubexpr,
	   unCoveredExpr);

      // if this is a sample scan (currently these are only CLUSTER
      // sampling scans) then do not choose index only scan.  Also,
      // due to an artifact of sampling, the 'isCovered' test above
      // will not return TRUE even for the ClusteringIndex, so force
      // it to be true for the ClusteringIndex.  Note that
      // ClusterIndex means that this is the basetable access path.
      //
      if (isSampleScan())
        {
          if (idesc->isClusteringIndex())
            {
              // Force it to be TRUE for the basetable access path.
              // This overrides the value of 'indexOnlyScan' produced
              // above since for sample scans, the isCovered test will
              // always fail, even for the basetable.
              //
              indexOnlyScan = TRUE;
            }
          else
            {
              indexOnlyScan = FALSE;
            }
        }

      // if the user specified IN EXCLUSIVE MODE option for this select,
      // then do not choose index only scan. This is needed so the base
      // table row could be locked in exclusive mode.
      if ((indexOnlyScan) &&
	  (! idesc->isClusteringIndex()) &&
	  (accessOptions().lockMode() == EXCLUSIVE_))
	indexOnlyScan = FALSE;

      //pruneMdam() returns a flag indicating if the index would have
      //has good enough key access for MDAM access to be viable. For index
      //join indexes it also returns a IndexJoinSelectivityEnum that
      //indicates if the index join is going to exceed the cost of just
      //scanning the base table.
      if (indexOnlyScan)
	{
	  // this index supplies all the info we need, consider
	  // it for an index only scan later
	  IndexJoinSelectivityEnum junk;
	  MdamFlags flag=idesc->pruneMdam(preds,TRUE,junk);
	  IndexProperty * ixProp = new(CmpCommon::statementHeap())
			       IndexProperty(idesc,
					     flag);
          if (tryToEliminateIndex) 
          {
            // with this CQD value set, try to consider minimum indexes possible
            // if ixProp is no better than any of indexOnlyIndexes_ - don't add
            // it. If ixProp is better than some of this set - remove them.
            ixProp->updatePossibleIndexes(indexOnlyIndexes_, this);
          }
          else
	    indexOnlyIndexes_.insert(ixProp);
	}
      else
	{
	  GroupAttributes indexGA;
	  ValueIdSet ijCoveredPredicates;
	  if(numIndexJoins_ < MAX_NUM_INDEX_JOINS AND
             NOT unlimitedIndexJoinsAllowed)
	  {
	    //Is any of the predicates covered by key columns of the
	    //alternate index?
	    ValueIdList userKeyColumns(idesc->getIndexKey());
            CollIndex numSecondaryIndexKey = 
              idesc->getNAFileSet()->getCountOfColumns(
                   TRUE,   // key columns only
                   TRUE,   // user-specified key columns only
                   FALSE,  // don't exclude system columns
                   FALSE); // don't exclude salt/divisioning columns
	    CollIndex numClusteringKey = 
              userKeyColumns.entries() - numSecondaryIndexKey;
	    if(NOT idesc->isUniqueIndex())
	    {
	      CollIndex entry = userKeyColumns.entries() -1;
	      for(CollIndex i=0;i<numClusteringKey;i++)
	      {
		userKeyColumns.removeAt(entry);
		entry--;
	      }
	    }
	    indexGA.addCharacteristicOutputs(userKeyColumns);

	    ValueIdSet ijReferencedInputs;
	    ValueIdSet ijUnCoveredExpr;

            ValueId vid;
            ValueIdSet disjuncts;

            // Multi-Index OR optimization requires that the index information
            // is maintained for disjuncts as well. So here we check if the
            // predicate is of the form A OR B OR C. If it is, then the top
            // operator is the OR operator. The optimization is considered only
            // in this case. So if the predicate has an OR on top of the item
            // expression, then we check if the disjuncts are covered by the
            // index.

            if (preds.entries() == 1)
            {
               preds.getFirst(vid);
               if (vid.getItemExpr()->getOperatorType() ==  ITM_OR)
               {
                  vid.getItemExpr()->convertToValueIdSet(disjuncts,
                                                         NULL,
                                                         ITM_OR,
                                                         FALSE);
               }
               else
                 disjuncts=preds;
            }
            else
               disjuncts=preds;

	    // add in index column hints from non-VEG equality predicates
	    disjuncts += possibleIndexColumns();

	    disjuncts.isCovered(
	      getGroupAttr()->getCharacteristicInputs(),
	      indexGA,
	      ijReferencedInputs,
	      ijCoveredPredicates,
	      ijUnCoveredExpr);

	    // we only care about predicates that are entirely covered,
	    // parts of predicates (like constants) that are covered
	    // don't help in this context
	    ijCoveredPredicates.intersectSet(disjuncts);
	  }
	  // This index does not provide all required values.
	  // However, it might be useful to join this index with another
	  // one (most likely the clustering index) to get the rest
	  // of the required values. If this is promising at all, then
	  // add this index to a list of possible index joins.
	  // In that list of possible index joins, group all indexes
	  // that provide the same set of output values (but different
	  // orders) together.
	  if (numIndexJoins_ < MAX_NUM_INDEX_JOINS AND
	      (unlimitedIndexJoinsAllowed OR NOT ijCoveredPredicates.isEmpty()))
	  {
	    // changing back to old predicate tree:
	    ValueIdSet selectionpreds;
	    if((CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON ) &&
	       (selectionPred().entries()))
	    {
	      ValueIdList selectionPredList(selectionPred());
	      ItemExpr *inputItemExprTree = selectionPredList.rebuildExprTree(ITM_AND,FALSE,FALSE);
	      ItemExpr * resultOld = revertBackToOldTree(CmpCommon::statementHeap(), inputItemExprTree);
	      resultOld->convertToValueIdSet(selectionpreds, NULL, ITM_AND);
	      doNotReplaceAnItemExpressionForLikePredicates(resultOld,selectionpreds,resultOld);
	    }

	      // For now, only consider indexes that covers one of the selection
	      // predicates unless control query shape is in effect.
              // NOTE: we should also consider indexes that potentially
              // could provide an interesting order or partitioning. To do
              // that, we would have to check whether their first key column
              // or any of their partitioning key columns is used.
              // For exclusive mode, any index can be called a usable index of
              // of another, only if it produces the same characteristic
              // outputs as the main index, and also both indexes have the same
              // uncovered expressions. This is because, in exclusive mode the
	      // base (clustering key) index must always be read even if the
	      // alternate index is index only, because the locks on the
	      // base index are required for exclusive mode.
	      // We can test the index only case with exclusive mode by
	      // requiring the uncovered expressions to be the same
	      // (both would be NULL for index only).

	      // we now have the following information ready:
	      // - coveredSubexpr are the values that the index can deliver
	      //   (+ clustering key columns)
	      // - unCoveredExpr are the values that the right child of the
	      //   index join should deliver (+ clustering key values)
	      // - we know the clustering key VEGies, whose VEGPredicates
	      //   serve as join predicates between the indexes
	      // - we can find out the selection predicates covered
	      //   by the index by intersecting them with coveredSubexpr

	      ValueIdSet newOutputsFromIndex(coveredSubexpr);
	      ValueIdSet newIndexPredicates(coveredSubexpr);
	      ValueIdSet newOutputsFromRightScan(unCoveredExpr);

	      newOutputsFromIndex += clusteringKeyColumns;
	      if(CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
	      {
		newOutputsFromIndex -= selectionpreds;
	      }
	      else
		newOutputsFromIndex -= selectionPred();
              newOutputsFromIndex -= getGroupAttr()->
	                               getCharacteristicInputs();
	      if(CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
	      {
		newIndexPredicates.intersectSet(selectionpreds);
		newOutputsFromRightScan -= selectionpreds;
	      }
	      else
	      {
		newIndexPredicates.intersectSet(selectionPred());
		newOutputsFromRightScan -= selectionPred();
	      }
	      newOutputsFromRightScan += clusteringKeyColumns;

              NABoolean idescAbsorbed = FALSE;


              // does another index have the same covered values?
              for (CollIndex i = 0; i < possibleIndexJoins_.entries(); i++)
                {
                  NABoolean isASupersetIndex =
                      possibleIndexJoins_[i]->outputsFromIndex_.contains(newOutputsFromIndex) &&
                      possibleIndexJoins_[i]->indexPredicates_.contains(newIndexPredicates);

                  NABoolean isASubsetIndex =
                      newOutputsFromIndex.contains(possibleIndexJoins_[i]->outputsFromIndex_) &&
                      newIndexPredicates.contains(possibleIndexJoins_[i]->indexPredicates_);

                  NABoolean isASuperOrSubsetIndex = isASupersetIndex || isASubsetIndex;

                  NABoolean produceSameIndexOutputs = isASupersetIndex && isASubsetIndex;

                  if ((possibleIndexJoins_[i]->inputsToIndex_ == referencedInputs)
		      && ((accessOptions().lockMode() != EXCLUSIVE_)
			  || possibleIndexJoins_[i]->outputsFromRightScan_ ==
			  newOutputsFromRightScan))
                    {
                      ScanIndexInfo *ixi = possibleIndexJoins_[i];

		      IndexJoinSelectivityEnum isGoodIndexJoin = INDEX_JOIN_VIABLE;
		      MdamFlags mdamFlag = idesc->pruneMdam(ixi->indexPredicates_,FALSE,
						 isGoodIndexJoin,
						 getGroupAttr(),&(ixi->inputsToIndex_));
		      IndexProperty * ixProp;
		      if(getGroupAttr()->getInputLogPropList().entries() >0)
			 ixProp = new(CmpCommon::statementHeap())
					      IndexProperty(idesc, mdamFlag, isGoodIndexJoin,
					      (getGroupAttr()->getInputLogPropList())[0]);
		      else
                        ixProp = new(CmpCommon::statementHeap())
                           IndexProperty(idesc, mdamFlag, isGoodIndexJoin);

                      if ( !tryToEliminateIndex || idesc->indexHintPriorityDelta() > 0) {

                         if ( produceSameIndexOutputs && ixi->indexPredicates_ == newIndexPredicates )
                         {
                             ixi->usableIndexes_.insert(ixProp);
                             idescAbsorbed = TRUE;
                             break;
                         } 

                      } else {

                         CANodeId tableId = tAnalysis->getNodeAnalysis()->getId();

                         // keep the index that provides the maximal coverage of the
                         // predicate. Do this only when the output from one index is
                         // the super set of the other. For example (a,b) in I1 
                         // (CREATE INDEX T1 on T(a, b)) is a superset of (a) in I2 
                         // (CREATE INDEX T2 on T(a)). 
                         if ( isASuperOrSubsetIndex && !produceSameIndexOutputs ) {
   
                             // Score the index's coverage by computing the remaining length of the 
                             // key columns not covering the index predicates. The one with remaining
                             // length of 0 is the best.

                             ValueIdSet indexCols;
                             newIndexPredicates.findAllReferencedIndexCols(indexCols);

                             Lng32 currentPrefixLen =
                                     idesc->getIndexKey().findPrefixLength(indexCols);
   
                             Lng32 currentSuffixLen = idesc->getIndexKey().entries() - currentPrefixLen;
   
                             Lng32 previousPrefixLen = 
                                  ixi->usableIndexes_[0]->getIndexDesc()
                                     ->getIndexKey().findPrefixLength(indexCols);
   
                             Lng32 previousSuffixLen = ixi->usableIndexes_[0]->getIndexDesc()
                                     ->getIndexKey().entries() - previousPrefixLen;
   
                             if ( currentSuffixLen < previousSuffixLen ) {
   
                               if ( printIndexElimination )
                                  out << "Eliminate index join heuristics 1: remove " 
                                      << ixi->usableIndexes_[0]->getIndexDesc()->getExtIndexName().data() 
                                      << endl;

                               ixi = new (CmpCommon::statementHeap())
                                          ScanIndexInfo(referencedInputs, newOutputsFromIndex,
                                          newIndexPredicates, indexJoinPreds,
                                          newOutputsFromRightScan, idesc->getIndexKey(),
                                          ixProp);
   
                                possibleIndexJoins_[i] = ixi;


                             } else {
                                // do nothing. The current index is less useful.
                                 if ( printIndexElimination )
                                    out << "Eliminate index join heuristics 1: remove " 
                                        << idesc->getExtIndexName().data() 
                                        << endl;
                               }

                             idescAbsorbed = TRUE;

                         } else 
                         // if no index is a prefix of the other and the two do not produce
                         // same output, pick one with high selectivity.
                         if ( !isASuperOrSubsetIndex && !produceSameIndexOutputs ) {

                            // two indexes do not produce the same outputs. Select
                            // one with the most selectivity.

                            CostScalar rowsToScan;

                            CostScalar currentDataAccess = 
                                computeCpuResourceForIndexJoin(tableId, idesc, 
                                                               newIndexPredicates, rowsToScan);

                            if ( rowsToScan > indexEliminationThreshold )
                              break;

                            CostScalar previousDataAccess = 
                                  computeCpuResourceForIndexJoin(tableId, 
                                                                ixi->usableIndexes_[0]->getIndexDesc(),
                                                                ixi->indexPredicates_, rowsToScan);


                            if ( currentDataAccess < previousDataAccess ) {
   
                               if ( printIndexElimination )
                                  out << "Eliminate index join heuristics 2: remove " 
                                      << ixi->usableIndexes_[0]->getIndexDesc()->getExtIndexName().data() 
                                      << endl;

                               ixi = new (CmpCommon::statementHeap()) 
                                       ScanIndexInfo(referencedInputs, newOutputsFromIndex,
                                                     newIndexPredicates, indexJoinPreds, 
                                                     newOutputsFromRightScan, idesc->getIndexKey(),
                                                     ixProp);

                               possibleIndexJoins_[i] = ixi;
                             } else {

                                // do nothing. The current index is less useful.
                                if ( printIndexElimination )
                                  out << "Eliminate index join heuristics 2: remove " 
                                      << idesc->getExtIndexName().data() << endl;
                             }

                             idescAbsorbed = TRUE;

                         } else {
   
                           // must be produceSameIndexOutputs when reach here. 
                           CMPASSERT(produceSameIndexOutputs);

                           // Another index produces the same characteristic
                           // outputs. Combine the two indexes in a single
                           // scan. Add this index to the list of indexes,
                           // everything else should be set already
                           if ( possibleIndexJoins_[i]->indexPredicates_ == newIndexPredicates &&
                                ixProp->compareIndexPromise(ixi->usableIndexes_[0]) == MORE ) 
                           {
                               if ( printIndexElimination )
                                  out << "Eliminate index join heuristics 0: remove " 
                                      << ixi->usableIndexes_[0]->getIndexDesc()->getExtIndexName().data() 
                                      << endl;

                               ixi = new (CmpCommon::statementHeap()) 
                                       ScanIndexInfo(referencedInputs, newOutputsFromIndex,
                                                     newIndexPredicates, indexJoinPreds, 
                                                     newOutputsFromRightScan, idesc->getIndexKey(),
                                                     ixProp);

                               possibleIndexJoins_[i] = ixi;

                               idescAbsorbed = TRUE;
                           }
                         }

                         break;
                      } // try to eliminate this index from consideration
                    } // found another index join with the same covered values
		} // for loop: does another index join have the same covered values?

	      if (!idescAbsorbed)
		{
		  // create a new index info struct and add this into the
		  // possible index joins list
		  IndexJoinSelectivityEnum isGoodIndexJoin = INDEX_JOIN_VIABLE;

		  MdamFlags mdamFlag = idesc->pruneMdam(newIndexPredicates,FALSE,
						 isGoodIndexJoin,
						 getGroupAttr(),&referencedInputs);

		  IndexProperty * ixProp = (getGroupAttr()->getInputLogPropList().entries() >0) ?
			new(CmpCommon::statementHeap())
					      IndexProperty(idesc, mdamFlag, isGoodIndexJoin,
					      (getGroupAttr()->getInputLogPropList())[0])
		        :
			new(CmpCommon::statementHeap())
					      IndexProperty(idesc, mdamFlag, isGoodIndexJoin);

                  ScanIndexInfo *ixi = new (CmpCommon::statementHeap()) 
                                ScanIndexInfo(referencedInputs, newOutputsFromIndex,
                                              newIndexPredicates, indexJoinPreds, 
                                              newOutputsFromRightScan, idesc->getIndexKey(),
                                              ixProp);

		  possibleIndexJoins_.insert(ixi);

		} // !idescAbsorbed 
	    } // index delivers new values
	} // not indexOnly access
    } // for each index

                               
  if ( printIndexElimination ) {
    out << "# of index join scans=" << possibleIndexJoins_.entries() << endl;
    out << "# of index only scans=" << indexOnlyIndexes_.entries() << endl;
    out << "==================" << endl;
  }

  CMPASSERT(indexOnlyIndexes_.entries() > 0);

} // Scan::addIndexInfo

void Scan::setTableAttributes(CANodeId nodeId)
{
  NodeAnalysis * nodeAnalysis = nodeId.getNodeAnalysis();

  if (nodeAnalysis == NULL)
    return;

  TableAnalysis * tableAnalysis = nodeAnalysis->getTableAnalysis();

  if (tableAnalysis == NULL)
    return;

  TableDesc * tableDesc = tableAnalysis->getTableDesc();
  const CorrName& name = tableDesc->getNATable()->getTableName();

  setTableName((CorrName &)name);
  setTableDesc(tableDesc);
  setBaseCardinality(MIN_ONE (tableDesc->getNATable()->getEstRowCount())) ;
}

NABoolean Scan::updateableIndex(IndexDesc *idx, ValueIdSet& preds, 
                                NABoolean & needsHalloweenProtection)
{
  //
  // Returns TRUE if the index (idx) can be used for a scan during an UPDATE.
  // Returns TRUE with needsHalloweenProtection also set to TRUE, if the index
  // needs a blocking sort for Halloween protection
  // Otherwise, returns FALSE to prevent use of this index. 
  // Using the index in this case requires Halloween protection, but is likely 
  // inefficient since there are no useful preds on the index key columns, so
  // we don't add this index to list of candidates.
  //

  // The conditions of when this index returns TRUE can also be expressed as
  // returns true for an index if it is 
  // a) a unique/clustering index or 
  // b) has a unique predicate on its key or 
  // c) has an equals or range predicate on all the index columns that 
  // get updated. If one of the key columns being updated has a range predicate
  // then needsHalloweenProtection is set to TRUE.
  // Note that if a key column is being updated and has no predicate on it then
  // we return FALSE.

  // preds has predicate in non-RangeSpec form, 
  // while pred will be in RangeSpec form if feature is enabled.
  ValueIdSet pred = getSelectionPredicates(), dummySet;

  SearchKey searchKey(idx->getIndexKey(),
	              idx->getOrderOfKeyValues(),
	              getGroupAttr()->getCharacteristicInputs(),
		      TRUE,
		      pred,
                      dummySet, // needed by the interface but not used here
                      idx
                      );

  // Unique index is OK to use.
  if (searchKey.isUnique())
    return TRUE;
  
  const ValueIdList colUpdated = getTableDesc()->getColUpdated();
  const ValueIdList indexKey = idx->getIndexKey();

  // Determine if the columns being updated are key columns. Each key
  // column being updated must have an associated equality clause in
  // the WHERE clause of the UPDATE for it to be used.
  for (CollIndex i = 0; i < colUpdated.entries(); i++)
  {
    ItemExpr *updateCol = colUpdated[i].getItemExpr();
    CMPASSERT(updateCol->getOperatorType() == ITM_BASECOLUMN);
    for (CollIndex j = 0; j < indexKey.entries(); j++)
    {
      ItemExpr *keyCol = indexKey[j].getItemExpr();
      ItemExpr *baseCol = ((IndexColumn*)keyCol)->getDefinition().getItemExpr();
      CMPASSERT(baseCol->getOperatorType() == ITM_BASECOLUMN);
      if (getGroupAttr()->isEmbeddedUpdate()){
        if (((BaseColumn*)updateCol)->getColNumber() ==
            ((BaseColumn*)baseCol)->getColNumber())
          return FALSE;
      }
      if ((NOT(idx->isUniqueIndex() || idx->isClusteringIndex()) && 
           (CmpCommon::getDefault(UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY) != DF_AGGRESSIVE))
          ||
	  (CmpCommon::getDefault(UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY) == DF_OFF))
      {
        if (((BaseColumn*)updateCol)->getColNumber() ==
            ((BaseColumn*)baseCol)->getColNumber()) 
        {
          if (preds.containsAsEquiLocalPred(baseCol->getValueId()))
            continue;
          else if (preds.containsAsRangeLocalPred(baseCol->getValueId()))
            needsHalloweenProtection = TRUE;
          else {
            needsHalloweenProtection = FALSE;
            return FALSE;
          }
        } // index key col is being updated
      } // not a clustering or unique index
    } // loop over index key cols
  } // loop over cols being updated

  return TRUE;
} // Scan::updateableIndex

NABoolean Scan::requiresHalloweenForUpdateUsingIndexScan()
{ 
  // Returns TRUE if any index that is in the list of indexes that will be 
  // added later in addIndexInfo() to drive the scan for an UPDATE requires 
  // Halloween protection. This is decided by using Scan::updateableIndex(). 
  // If this method returns TRUE we will use a sort node to prevent the 
  // "Halloween Update Problem".

  // preds are in RangeSpec form
  ValueIdSet preds = getSelectionPredicates();
  const ValueIdList & colUpdated = getTableDesc()->getColUpdated();
  const LIST(IndexDesc *) & ixlist = getTableDesc()->getIndexes();
  
  if ((colUpdated.entries() == 0) || (preds.entries() == 0) ||
      (ixlist.entries() == 1) || // this is the clustering index
      (CmpCommon::getDefault(UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY) 
       == DF_AGGRESSIVE)) // this setting means no Halloween protection
    return FALSE;
 
  if (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON)
  {
    ValueIdList selectionPredList(preds);
    ItemExpr *inputItemExprTree = 
      selectionPredList.rebuildExprTree(ITM_AND,FALSE,FALSE);
    ItemExpr * resultOld = revertBackToOldTree(STMTHEAP, 
                                               inputItemExprTree);
    preds.clear();
    resultOld->convertToValueIdSet(preds, NULL, ITM_AND);
    doNotReplaceAnItemExpressionForLikePredicates(resultOld,preds,
                                                  resultOld);
  }
  
  NABoolean needsHalloweenProtection ;
  for (CollIndex indexNo = 0; indexNo < ixlist.entries(); indexNo++)
  {
    IndexDesc *idx = ixlist[indexNo];
    if (idx->isClusteringIndex() || 
        (idx->isUniqueIndex() && 
         (CmpCommon::getDefault(UPDATE_CLUSTERING_OR_UNIQUE_INDEX_KEY) 
          == DF_ON)))
      continue ; // skip this idesc
    
    needsHalloweenProtection = FALSE;
    if(updateableIndex(idx, preds, needsHalloweenProtection) && 
       needsHalloweenProtection)
      return TRUE; // if even one index requires Halloween, then we add the sort
  }
  return FALSE;
}

// how many index descriptors can be used with this scan node?
CollIndex Scan::numUsableIndexes()
{
  // start with the index-only indexes
  CollIndex result = indexOnlyIndexes_.entries();

  // for each index join, count all the indexes that result in equivalent
  // characteristics inputs and outputs
  for (CollIndex i=0; i < possibleIndexJoins_.entries(); i++)
    result += possibleIndexJoins_[i]->usableIndexes_.entries();

  return result;
}

// this method works like an operator[], with values for indexNum
// between 0 and numUsableIndexes()-1. It returns the associated IndexDesc
// and - depending on whether it is an index-only scan or an index join -
// the appropriate information class (optional).
IndexDesc * Scan::getUsableIndex(CollIndex indexNum,
				 IndexProperty **indexOnlyInfo,
				 ScanIndexInfo **indexJoinInfo)
{
  IndexDesc     *result           = NULL;
  IndexProperty *locIndexOnlyInfo = NULL;
  ScanIndexInfo *locIndexJoinInfo = NULL;

  if (indexNum < indexOnlyIndexes_.entries())
    {
      // indexNum corresponds to an index-only scan
      locIndexOnlyInfo = indexOnlyIndexes_[indexNum];
      result = locIndexOnlyInfo->getIndexDesc();
    }
  else
    {
      // search for index desc "indexNum" in the index joins
      // (which is a list of sets of index descs, making this
      // method somewhat complex)
      indexNum -= indexOnlyIndexes_.entries();
      CollIndex ixJoinIx = 0;
      CollIndex numIndexJoins = possibleIndexJoins_.entries();

      if (numIndexJoins > 0)
	{
	  // loop over the list of index joins, counting index descs until
	  // we find the right index join
	  while (ixJoinIx < numIndexJoins)
	    {
	      ScanIndexInfo *si = possibleIndexJoins_[ixJoinIx];

	      if (indexNum >= si->usableIndexes_.entries())
		{
		  // not there yet, go on to the next index join
		  indexNum -= si->usableIndexes_.entries();
		  ixJoinIx++;
		}
	      else
		{
		  // now we have reached the right index join (if
		  // any), select an index
		  locIndexJoinInfo = si;
		  result = si->usableIndexes_[indexNum]->getIndexDesc();
		  break;
		}
	    }
	}
    }

  // return information or NULL for not found
  if (indexOnlyInfo)
    *indexOnlyInfo = locIndexOnlyInfo;
  if (indexJoinInfo)
    *indexJoinInfo = locIndexJoinInfo;
  return result;
}

void Scan::getRequiredVerticalPartitions
           (SET(IndexDesc  *) & requiredVPs,
            SET(ValueIdSet *) & columnsProvidedByVP) const
{
  // We get requiredVPs and columnsProvidedByVP passed to us that have
  // no entries.  We have to populate them with the vertical partitions
  // required to service the query and the columns that each of those
  // VPs will provide.  Each entry in columnsProvidedByVP is related to
  // the corresponding entry in requiredVPs
  CMPASSERT(requiredVPs.entries() == 0 &&
            columnsProvidedByVP.entries() == 0);

  const TableDesc * tableDesc = getTableDesc();
  const LIST(IndexDesc *) & allVPs = tableDesc->getVerticalPartitions();

#ifdef OLD
  // Get all the value ids that are required by the scan and its parents
  ValueIdSet requiredValueIds(getGroupAttr()->getCharacteristicOutputs());

  // VEGPreds can have two forms, an  A IS NOT NULL form and an A=B form
  // when expanded in the generator. If an index does not provide a
  // VEG member that the base table provides, a VEGPredicate could be
  // covered by the index in its IS NOT NULL form (checking a char. input
  // whether it is not null). To avoid this bug, add all the base cols
  // that contribute to VEGPredicates as explicitly required values.
  addBaseColsFromVEGPreds(requiredValueIds);

  // selection predicates are also required, add them to requiredValueIds
  requiredValueIds += getSelectionPred();

  // Remove any VEGPreds from required Values
  ValueIdSet VEGEqPreds;
  getSelectionPred().lookForVEGPredicates(VEGEqPreds);
  requiredValueIds -= VEGEqPreds;

  // The following code gets all the leaf node value ids.  It deletes the
  // characteristic input list of value ids from the leaf value ids.  It
  // does this since predicates are not pushed down to the VPs and the VPs
  // only provide outputs and do not take any inputs.  It reassigns the
  // remaining leaf value ids as those actually required from the VPs.
  // This code e.g. will only keep the b from a predicate such as b > 3
  // and the c from an expression c + 1 in the select list.
  ValueIdSet leafValues, emptySet;
  GroupAttributes emptyGA;
  requiredValueIds.getLeafValuesForCoverTest(leafValues, emptyGA, emptySet);

  leafValues -= getGroupAttr()->getCharacteristicInputs();
  requiredValueIds = leafValues;
#endif

  // -----------------------------------------------------------------
  // Accumulate the ValueIds of all VEGPredicates.
  // -----------------------------------------------------------------
  ValueIdSet VEGEqPreds;
  getSelectionPred().lookForVEGPredicates(VEGEqPreds);

  // -----------------------------------------------------------------
  // Compute the set of expressions that will be evaluated on the
  // parent. Add a VEGReference for every VEGPredicate in this set.
  // -----------------------------------------------------------------
  // remaining expressions on parent
  ValueIdSet requiredValueIdsMembers;
  RelExpr::computeValuesReqdForPredicates(VEGEqPreds,
                                 requiredValueIdsMembers);

  ValueIdSet requiredValueIds;
  requiredValueIds.replaceVEGExpressionsAndCopy(requiredValueIdsMembers);

  // ---------------------------------------------------------------------
  // Examine the set of values required by the parent (VPJoin node).
  // Replace each VEGPredicate with a VEGReferences for its VEG; if its
  // VEG contains other VEGReferences, add them to requiredValueIds.
  // ---------------------------------------------------------------------
  RelExpr::computeValuesReqdForPredicates(getGroupAttr()->getCharacteristicOutputs(),
				 requiredValueIds);

  requiredValueIds += getSelectionPred();
  requiredValueIds -= VEGEqPreds;   // delete all VEGPredicates

  // The following code gets all the leaf node value ids.  It deletes the
  // characteristic input list of value ids from the leaf value ids.  It
  // does this since predicates are not pushed down to the VPs and the VPs
  // only provide outputs and do not take any inputs.  It reassigns the
  // remaining leaf value ids as those actually required from the VPs.
  // This code e.g. will only keep the b from a predicate such as b > 3
  // and the c from an expression c + 1 in the select list.
  ValueIdSet leafValues, emptySet;
  GroupAttributes emptyGA;
  requiredValueIds.getLeafValuesForCoverTest(leafValues, emptyGA, emptySet);

  leafValues -= getGroupAttr()->getCharacteristicInputs();
  requiredValueIds = leafValues;

  // Remove all basecolumns (logical columns)
  //
  for(ValueId expr = requiredValueIds.init();
      requiredValueIds.next(expr);
      requiredValueIds.advance(expr)) {

    ItemExpr *ie = expr.getItemExpr();

    if(ie->getOperatorType() == ITM_BASECOLUMN)
      requiredValueIds -= expr;
  }

  // the values that are covered by every vertical partition (such as
  // clustering key)
  ValueIdSet alwaysCovered;
  // a list of VEGReferences to the clustering key column(s)
  ValueIdSet clusteringKeyColumns;
  // some helper variables
  ValueIdList clusteringKeyColList;
  GroupAttributes alwaysCoveredGA;
  ValueIdSet dummyReferencedInputs;
  ValueIdSet dummyUnCoveredExpr;

  // ---------------------------------------------------------------------
  // find out the subset of values that are always covered
  // ---------------------------------------------------------------------
  // get the clustering key columns and transform them into VEGies
  tableDesc->getEquivVEGCols(tableDesc->getClusteringIndex()->getIndexKey(),
                             clusteringKeyColList);
  clusteringKeyColumns = clusteringKeyColList;

  // make group attributes that get the original scan node's char.
  // inputs and the clustering key columns as outputs (every VP
  // should have the clustering key), to represent the least common
  // denominator of all VP attributes
  alwaysCoveredGA.addCharacteristicOutputs(clusteringKeyColumns);

  requiredValueIds.isCovered(
       getGroupAttr()->getCharacteristicInputs(),
       alwaysCoveredGA,
       dummyReferencedInputs,
       alwaysCovered,
       dummyUnCoveredExpr);

  // alwaysCovered now contains a set of values that should be covered
  // by every vertical partition


  ValueIdSet remainingValueIds = requiredValueIds;

  // ---------------------------------------------------------------------
  // For each vertical partition, check whether it provides any useful
  // values
  // ---------------------------------------------------------------------
  for (CollIndex indexNo = 0; indexNo < allVPs.entries(); indexNo++)
    {
      IndexDesc *idesc = allVPs[indexNo];

      ValueIdSet indexColumns(idesc->getIndexColumns());
      ValueIdSet *coveredSubexpr = new (CmpCommon::statementHeap())
                                       ValueIdSet();
      ValueIdSet noCharacteristicInputs;
      GroupAttributes vpGroupAttributes;
      NABoolean onlyOneVPRequired;

      // make group attributes for a vertical partition scan
      vpGroupAttributes.addCharacteristicOutputs(idesc->getIndexColumns());

      // does the index cover all required values, and if not, which
      // ones does it cover
      onlyOneVPRequired = requiredValueIds.isCovered(noCharacteristicInputs,
                                                     vpGroupAttributes,
                                                     dummyReferencedInputs,
                                                     *coveredSubexpr,
                                                     dummyUnCoveredExpr);

      if (onlyOneVPRequired)
        {
          // This vertical partition supplies all the required values.
          // That means we have all the required vertical partitions
          // and we are done.  In fact, if we had selected other VPs
          // before we need to clear them out along with the columns
          // they provide.
          // There should not be any selection predicates in this list
          // since they should have been eliminated from
          // requiredValueIdsby the leaf value id code earlier.
          requiredVPs.clear();
          columnsProvidedByVP.clear();
          requiredVPs.insert(idesc);
          columnsProvidedByVP.insert(coveredSubexpr);
          return;
        }
      else
        {
          if(remainingValueIds.entries() > 0) {
            coveredSubexpr->clear();

            requiredValueIds.isCovered(noCharacteristicInputs,
                                       vpGroupAttributes,
                                       dummyReferencedInputs,
                                       *coveredSubexpr,
                                       dummyUnCoveredExpr);


            // This vertical partition does not provide all required values.
            // Normally we wouldn't expect it to.  But does it provide a
            // column value other than the clustering key?  If it does, it's in!
            // We should take out the selection predicates since we will
            // not be evaluating any predicates in the VP scan.
            if ( (*coveredSubexpr != alwaysCovered) &&
                 ((*coveredSubexpr).entries() > 0) )
              {
                requiredVPs.insert(idesc);
                *coveredSubexpr -= getSelectionPred();
                columnsProvidedByVP.insert(coveredSubexpr);
              } // VP delivers column values

            remainingValueIds -= *coveredSubexpr;
          }
        } // not onlyOneVPRequired
    } // for each VP

  return;

} // Scan::getRequiredVerticalPartitions

void Scan::addBaseColsFromVEGPreds(ValueIdSet &vs) const
{
  // get all the base columns of the table (no VEGies)
  ValueIdSet baseCols(tabId_->getColumnList());

  for (ValueId x = getSelectionPred().init();
       getSelectionPred().next(x);
       getSelectionPred().advance(x))
    {
      ItemExpr *ie = x.getItemExpr();

      if (ie->getOperatorType() == ITM_VEG_PREDICATE)
	{
	  // get the VEG members
	  ValueIdSet vegMembers(
	       ((VEGPredicate *)ie)->getVEG()->getAllValues());

	  // filter out the base columns of this table that are VEG members
	  // and add them to the output parameter
	  vegMembers.intersectSet(baseCols);
	  vs += vegMembers;
	}
    }
}

void Scan::addLocalExpr(LIST(ExprNode *) &xlist,
			LIST(NAString) &llist) const
{
  RelExpr::addLocalExpr(xlist,llist);
}


NABoolean Scan::reconcileGroupAttr(GroupAttributes *newGroupAttr)
{
  addIndexInfo();
  const SET(IndexDesc *) & indexOnlyScans = deriveIndexOnlyIndexDesc();
  const SET(IndexDesc *) & indexJoinScans = deriveIndexJoinIndexDesc();
  // we add the available indexes on this scan node to the
  // new GroupAttrs availableBtreeIndexes
  newGroupAttr->addToAvailableBtreeIndexes(indexOnlyScans);
  newGroupAttr->addToAvailableBtreeIndexes(indexJoinScans);
  // This one is not actually necessary
  getGroupAttr()->addToAvailableBtreeIndexes(indexOnlyScans);
  getGroupAttr()->addToAvailableBtreeIndexes(indexJoinScans);
  // Now as usual
  return RelExpr::reconcileGroupAttr(newGroupAttr);
}

// --------------------------------------------------------------------
// 10-040128-2749 -begin
// This will compute based on the context,Current Control table setting
// and defaults.
// Input : Context
// --------------------------------------------------------------------
NABoolean Scan::isMdamEnabled(const Context *context)
{
  NABoolean mdamIsEnabled = TRUE;

    // -----------------------------------------------------------------------
    // Check the status of the enabled/disabled flag in
    // the defaults:
    // -----------------------------------------------------------------------
    if (CmpCommon::getDefault(MDAM_SCAN_METHOD) == DF_OFF)
      mdamIsEnabled = FALSE;

    // -----------------------------------------------------------------------
    // Mdam can also be disabled for a particular scan via Control
    // Query Shape. The information is passed by the context.
    // -----------------------------------------------------------------------
    if (mdamIsEnabled)
	  {
        const ReqdPhysicalProperty* propertyPtr =
            context->getReqdPhysicalProperty();
        if ( propertyPtr
             && propertyPtr->getMustMatch()
             && (propertyPtr->getMustMatch()->getOperatorType()
                 == REL_FORCE_ANY_SCAN))
		  {
            ScanForceWildCard* scanForcePtr =
              (ScanForceWildCard*)propertyPtr->getMustMatch();
            if (scanForcePtr->getMdamStatus() == ScanForceWildCard::MDAM_OFF)
              mdamIsEnabled = FALSE;
		  }
	   }

    // -----------------------------------------------------------------------
    // Mdam can also be disabled for a particular table via a Control
    // Table command.
    // -----------------------------------------------------------------------
    if (mdamIsEnabled)
	  {
        const NAString * val =
	    ActiveControlDB()->getControlTableValue(getTableName().getUgivenName(), "MDAM");
        if ((val) && (*val == "OFF")) // CT in effect
		{
	       mdamIsEnabled = FALSE;
		}
	  }
    return mdamIsEnabled;
}
// 10-040128-2749 -end

// -----------------------------------------------------------------------
// methods for class ScanIndexInfo
// -----------------------------------------------------------------------
ScanIndexInfo::ScanIndexInfo(const ScanIndexInfo & other) :
  outputsFromIndex_ (other.outputsFromIndex_),
  indexPredicates_  (other.indexPredicates_),
  joinPredicates_   (other.joinPredicates_),
  outputsFromRightScan_ (other.outputsFromRightScan_),
  transformationDone_   (other.transformationDone_),
  indexColumns_ (other.indexColumns_),
  usableIndexes_        (other.usableIndexes_)
{}

ScanIndexInfo::ScanIndexInfo(
              const ValueIdSet& inputsToIndex,
              const ValueIdSet& outputsFromIndex,
              const ValueIdSet& indexPredicates,
              const ValueIdSet& joinPredicates,
              const ValueIdSet& outputsFromRightScan,
              const ValueIdSet& indexColumns,
              IndexProperty* ixProp
             ) :
   inputsToIndex_(inputsToIndex),
   outputsFromIndex_(outputsFromIndex),
   indexPredicates_(indexPredicates),
   joinPredicates_(joinPredicates),
   outputsFromRightScan_(outputsFromRightScan),
   indexColumns_(indexColumns),
   transformationDone_(FALSE),
   usableIndexes_(CmpCommon::statementHeap())
{
   usableIndexes_.insert(ixProp);
}


// -----------------------------------------------------------------------
// methods for class FileScan
// -----------------------------------------------------------------------

FileScan::FileScan(const CorrName& tableName,
		   TableDesc * tableDescPtr,
		   const IndexDesc *indexDescPtr,
		   const NABoolean isReverseScan,
		   const Cardinality& baseCardinality,
		   StmtLevelAccessOptions& accessOpts,
		   GroupAttributes * groupAttributesPtr,
		   const ValueIdSet& selectionPredicates,
		   const Disjuncts& disjuncts,
                   const ValueIdSet& generatedCCPreds,
                   OperatorTypeEnum otype) :
     Scan (tableName, tableDescPtr, otype),
     indexDesc_(indexDescPtr),
     reverseScan_(isReverseScan),
     executorPredTree_(NULL),
     mdamKeyPtr_(NULL),
     disjunctsPtr_(&disjuncts),
     pathKeys_(NULL),
     partKeys_(NULL),
     hiveSearchKey_(NULL),
     estRowsAccessed_ (0),
     mdamFlag_(UNDECIDED),
     skipRowsToPreventHalloween_(FALSE),
     doUseSearchKey_(TRUE),
     computedNumOfActivePartitions_(-1)
{
  // Set the filescan properties:


  // Set the base cardinality to that for the logical scan
  setBaseCardinality(baseCardinality);

  // move the statement level access options
  accessOptions() = accessOpts;

  // the top node keeps the original group attributes
  setGroupAttr(groupAttributesPtr);

  // Initialize selection predicates:
  // (they are needed to set the executor predicates later in
  // pre-code gen)
  selectionPred().insert(selectionPredicates);

  // Get the predicates on the partitioning key:
  if (getIndexDesc() && getIndexDesc()->isPartitioned())
    {
      ValueIdSet externalInputs = getGroupAttr()->getCharacteristicInputs();
      ValueIdSet dummySet;
      ValueIdSet selPreds(selectionPredicates);

      // Create and set the Searchkey for the partitioning key:
      partKeys_ =  new (CmpCommon::statementHeap())
                   SearchKey(indexDesc_->getPartitioningKey(),
                                     indexDesc_->getOrderOfPartitioningKeyValues(),
                                     externalInputs,
                                     NOT getReverseScan(),
                                     selPreds,
                                     disjuncts,
                                     dummySet, // needed by interface but not used here
                                     indexDesc_
                                   );

   
      if ( indexDesc_->getPartitioningFunction() &&
           indexDesc_->getPartitioningFunction()->castToRangePartitioningFunction() ) 
      {
         const RangePartitioningFunction* rangePartFunc =
              indexDesc_->getPartitioningFunction()->castToRangePartitioningFunction();

         computedNumOfActivePartitions_ = 
             rangePartFunc->computeNumOfActivePartitions(partKeys_, tableDescPtr);
      }
    }
  setComputedPredicates(generatedCCPreds);

} // FileScan()

void FileScan::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  //
  // Assign the set of columns that belong to the index to be scanned
  // as the output values that can be produced by this scan.
  //
  outputValues.insertList( getIndexDesc()->getIndexColumns() );

  // MV --
  // Add the CurrentEpoch column as well.
  outputValues.insert(getExtraOutputColumns());
} // FileScan::getPotentialOutputValues()

NABoolean FileScan::patternMatch(const RelExpr & other) const
{
  // handle the special case of a pattern to force a
  // specific table or index
  if (other.getOperatorType() == REL_FORCE_ANY_SCAN)
    {
      ScanForceWildCard &w = (ScanForceWildCard &) other;

      if (w.getExposedName() != "")
        {
          QualifiedName wName(w.getExposedName(), 1 /* minimal 1 part name */);

          if (getTableName().getCorrNameAsString() != "")
             {
                // query uses a correlation name, compare that with the wildcard
                // as a string
                if (wName.getQualifiedNameAsAnsiString() != 
                     ToAnsiIdentifier(getTableName().getCorrNameAsString()))
                  return FALSE;
             }
          else
             {
               // no correlation name used in the query, compare catalog, schema
               // and table parts separately, if they exist in the wildcard
               const NAString& catName = wName.getCatalogName();
               const NAString& schName = wName.getSchemaName();
               const QualifiedName& x  = getTableName().
                             getExtendedQualNameObj().getQualifiedNameObj();
   
               if ((catName.length() > 0 && x.getCatalogName() != catName) ||
                   (schName.length() > 0 && x.getSchemaName()  != schName) ||
                   x.getObjectName() != wName.getObjectName())
                 return FALSE;
             }
      }

      // if an index name was specified in the wildcard, check for it
      if (w.getIndexName() != "")
      {
        NAString forcedIndexName(w.getIndexName(),
                                 CmpCommon::statementHeap());

        // The user can specify the index to be the base table in the
        // Control Query Shape statement by using the table name (object
        // name or correlation) as the index name. Ex: scan('t1','t1',..)
        // since t1 might be a correlation name, its necessary to check
        // for the corresponding object name and not the table correlation
        // name when searching for the index match.
        if (forcedIndexName == w.getExposedName())
          forcedIndexName =  ToAnsiIdentifier(
                  getTableName().getQualifiedNameObj().getObjectName()
                                             );

        // get the three-part name of the index
        const NAString &ixName = indexDesc_->getNAFileSet()->getExtFileSetName();

        // Declare a match if either the index name in w is equal to
        // indexName or if it is equal to the last part of indexName.
        //if (w.getIndexName() != ixName)
        if (forcedIndexName != ixName)
        {
          QualifiedName ixNameQ(ixName, 1);

          if ( ToAnsiIdentifier(ixNameQ.getObjectName()) != forcedIndexName )
            return FALSE;
        }
      }

      return TRUE;
  }
  else
    return RelExpr::patternMatch(other);
}

NABoolean FileScan::duplicateMatch(const RelExpr & other) const
{
  if (!Scan::duplicateMatch(other))
    return FALSE;

  FileScan &o = (FileScan &) other;

  if (//beginKeyPred_      != o.beginKeyPred_ OR
      //endKeyPred_        != o.endKeyPred_ OR
      retrievedCols_     != o.retrievedCols_ OR
      getExecutorPredicates() != o.getExecutorPredicates())
    return FALSE;

  return TRUE;
}

RelExpr * FileScan::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{

  // $$$ Function needs to be updated with new fields
  // added to the filescan.
  // If you need to use this function please update it
  // with those new fields (i.e. SearchKey *, etc's)
  // then remove the following abort
  // CMPABORT;
  FileScan *result;

  if (derivedNode == NULL)
    result = new(outHeap) FileScan(getTableName(),
				   getTableDesc(),
				   getIndexDesc(),
				   REL_FILE_SCAN,
				   outHeap);
  else
    result = (FileScan *) derivedNode;

  result->setBaseCardinality(getBaseCardinality());
  result->setEstRowsAccessed(getEstRowsAccessed());
  result->beginKeyPred_ = beginKeyPred_;
  result->endKeyPred_ = endKeyPred_;
  result->setExecutorPredicates(getExecutorPredicates());
  result->retrievedCols_ = retrievedCols_;

  return Scan::copyTopNode(result, outHeap);
}

NABoolean FileScan::isLogical() const  { return FALSE; }

NABoolean FileScan::isPhysical() const { return TRUE;  }

PlanPriority FileScan::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{

  PlanPriority result;

  // ---------------------------------------------------------------------
  // If under interactive_access mode, then give preference to plans that
  // avoid full table scans
  // Similarly if under firstN optimization mode then give preference to
  // plans that avoid full table scans
  // ---------------------------------------------------------------------
  NABoolean interactiveAccess =
    (CmpCommon::getDefault(INTERACTIVE_ACCESS) == DF_ON) OR
    ( QueryAnalysis::Instance() AND
      QueryAnalysis::Instance()->optimizeForFirstNRows());
  int indexPriorityDelta = getIndexDesc()->indexHintPriorityDelta();

  if (interactiveAccess)
  {
    if(getMdamKeyPtr())
    {
      // We have MDAM. Give this a preference
      result.incrementLevels(INTERACTIVE_ACCESS_MDAM_PRIORITY,0);
    }
    else if(getSearchKeyPtr() AND
            getSearchKeyPtr()->getKeyPredicates().entries())
    {
      // We have direct index access. Give this a preference
      result.incrementLevels(INTERACTIVE_ACCESS_PRIORITY,0);
    }
  }

  if (indexPriorityDelta && !areHintsSuppressed())
    // yes, the index is one of the indexes listed in the hint
    result.incrementLevels(indexPriorityDelta, 0);

  return result;
}

// currently only used by MV query rewrite
PlanPriority PhysicalMapValueIds::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{
  PlanPriority result;

  // is this MVI wraps one of the favorite MVs 
  // (included in the MVQR_REWRITE_CANDIDATES default)
  if (includesFavoriteMV())
  {
     result.incrementLevels(MVQR_FAVORITE_PRIORITY,0);
  }

  return result;
}

const NAString FileScan::getText() const
{
  // ---------------------------------------------------------------------
  // returns:
  //
  // file scan t c			for a primary index scan on table
  // 					t with correlation name c
  // index scan ix(t c)			for an index scan on index ix of
  //					table t
  // rev. index scan ix(t c)		if the scan goes backwards
  // ---------------------------------------------------------------------

  NAString op(CmpCommon::statementHeap());
  NAString tname(getTableName().getText(),CmpCommon::statementHeap());

  if (isSampleScan() == TRUE)
    op = "sample_";
  if (indexDesc_ == NULL OR indexDesc_->isClusteringIndex())
    {
      if (isHiveTable())
	op += "hive_scan ";
      else
	op += "file_scan ";
    }
  else {
    op += "index_scan ";
    tname = indexDesc_->getIndexName().getQualifiedNameAsString() +
	    "(" + tname + ")";
  }

  if (reverseScan_)
    op += NAString("rev ");

  return op + tname;
}

const NAString FileScan::getTypeText() const
{
  NAString descr(CmpCommon::statementHeap());
  NAString tname(getTableName().getText(),CmpCommon::statementHeap());

  if (isSampleScan() == TRUE)
    descr = "sample ";

  if (reverseScan_)
    descr += NAString("reverse ");

  if (isFullScanPresent() && !getMdamKeyPtr())
  {
    descr += "full scan ";
    if (getMdamKeyPtr())
      descr += "limited by mdam ";
  }
  else
  {
    descr += "subset scan ";
    if (getMdamKeyPtr())
      descr += "limited by mdam ";
  }

  descr += "of ";

  if (indexDesc_ == NULL OR indexDesc_->isClusteringIndex())
    descr += "table ";
  else {
    descr += "index ";
    tname = indexDesc_->getIndexName().getQualifiedNameAsString() +
	    "(" + tname + ")";
  }
  descr += tname;

  return descr;
}

void FileScan::addLocalExpr(LIST(ExprNode *) &xlist,
			    LIST(NAString) &llist) const
{
  if (getIndexDesc() != NULL)
    {
      const ValueIdList& keyColumns = getIndexDesc()->getIndexKey();
      xlist.insert(keyColumns.rebuildExprTree());
      llist.insert("key_columns");

    }

  if (executorPredTree_ != NULL OR
      NOT getExecutorPredicates().isEmpty())
    {
      if (getExecutorPredicates().isEmpty())
	xlist.insert(executorPredTree_);
      else
	xlist.insert(getExecutorPredicates().rebuildExprTree());
      llist.insert("executor_predicates");
    }

  // -----------------------------------------------------------------------
  // Display key information
  // -----------------------------------------------------------------------
  if (getMdamKeyPtr() != NULL)
    {
      // Mdam access!
      const CollIndex columns = getIndexDesc()->getIndexKey().entries();
      const CollIndex disjEntries =
        getMdamKeyPtr()->getKeyDisjunctEntries();

      // If we are in the optimizer, obtain key preds from
      // the disjuncts,
      // else obtain them from the column order list array:
      if (NOT nodeIsPreCodeGenned())
        {
          // We are in the optimizer...
          // For every disjunct
          for (CollIndex i=0;
               i < disjEntries;
               i++)
            {
              ColumnOrderList kpbc(getIndexDesc()->getIndexKey());
              getMdamKeyPtr()->getKeyPredicatesByColumn(kpbc,i);

              // gather the key predicates:
              ValueIdSet keyPreds;

              for (CollIndex j=0; j < columns; j++)
                {
                  if (kpbc[j])
                    {
                      keyPreds.insert(*(kpbc[j]));
                    }
                }

              // display this disjunct key preds. into the GUI:
              xlist.insert(keyPreds.rebuildExprTree());
              llist.insert("mdam_disjunct");
            } // for every disjunct
        }
      else
        {
          // we are after the generator...

          const ColumnOrderListPtrArray &columnOrderListPtrArray =
            getMdamKeyPtr()->getColumnOrderListPtrArray();

          // we are in the generator, obtain the key preds
          // from thr column order list:
          ValueIdSet *predsPtr = NULL;
          for (CollIndex n = 0; n < columnOrderListPtrArray.entries(); n++)
            {
              // get the list of key predicates associated with the n disjunct:
              const ColumnOrderList &columnOrderList =
                *columnOrderListPtrArray[n];
              	      // get predicates for column order i:

              // gather the key predicates:
              ValueIdSet keyPreds;

              const ValueIdSet *predsPtr = NULL;
              for (CollIndex i = 0; i < columnOrderList.entries(); i++)
                {
                  predsPtr = columnOrderList[i];
                  if (predsPtr)
                    {
                      keyPreds.insert(*predsPtr);
                    }
                }

              // display this disjunct key preds. into the GUI:
              xlist.insert(keyPreds.rebuildExprTree());
              llist.insert("mdam_disjunct");
            }
        } // mdam after the generator



    } // mdam access
  else if (getSearchKeyPtr() != NULL)   // Is Single subset access?
    {
      // yes!
      // display preds from search key only if begin/end keys are
      // not generated yet (e.g. during optimization)
      if (getBeginKeyPred().isEmpty() AND
          getEndKeyPred().isEmpty() AND
          pathKeys_ AND NOT pathKeys_->getKeyPredicates().isEmpty())
        {
          xlist.insert(pathKeys_->getKeyPredicates().rebuildExprTree());
          if (pathKeys_ == partKeys_)
            llist.insert("key_and_part_key_preds");
          else
            llist.insert("key_predicates");
        }
    }

  // display part key preds only if different from clustering key preds
  if (partKeys_ AND pathKeys_ != partKeys_ AND
      NOT partKeys_->getKeyPredicates().isEmpty())
    {
      xlist.insert(partKeys_->getKeyPredicates().rebuildExprTree());
      llist.insert("part_key_predicates");
    }

  if (NOT getBeginKeyPred().isEmpty())
    {
      xlist.insert(getBeginKeyPred().rebuildExprTree());
      llist.insert("begin_key");
    }
  if (NOT getEndKeyPred().isEmpty())
    {
      xlist.insert(getEndKeyPred().rebuildExprTree());
      llist.insert("end_key");
    }

    // xlist.insert(retrievedCols_.rebuildExprTree(ITM_ITEM_LIST));
    // llist.insert("retrieved_cols");

  RelExpr::addLocalExpr(xlist,llist);
}

const Disjuncts& FileScan::getDisjuncts() const
{
  CMPASSERT(disjunctsPtr_ != NULL);
  return *disjunctsPtr_;
}

// -----------------------------------------------------------------------
// methods for class HbaseAccess
// -----------------------------------------------------------------------

HbaseAccess::HbaseAccess(CorrName &corrName,
			 OperatorTypeEnum otype,
			 CollHeap *oHeap)
  : FileScan(corrName, NULL, NULL, otype, oHeap),
    listOfSearchKeys_(oHeap),
    snpType_(SNP_NONE),
    retHbaseColRefSet_(oHeap),
    opList_(oHeap)
{
  accessType_ = SELECT_;
  uniqueHbaseOper_ = FALSE;
  uniqueRowsetHbaseOper_ = FALSE;
}

HbaseAccess::HbaseAccess(CorrName &corrName,
			 TableDesc *tableDesc,
			 IndexDesc *idx,
                         const NABoolean isReverseScan,
                         const Cardinality& baseCardinality,
                         StmtLevelAccessOptions& accessOptions,
                         GroupAttributes * groupAttributesPtr,
                         const ValueIdSet& selectionPredicates,
                         const Disjuncts& disjuncts,
                         const ValueIdSet& generatedCCPreds,
			 OperatorTypeEnum otype,
                         CollHeap *oHeap)
  : FileScan(corrName, tableDesc, idx, 
             isReverseScan, baseCardinality,
             accessOptions, groupAttributesPtr,
             selectionPredicates, disjuncts,
             generatedCCPreds,
             otype),
    listOfSearchKeys_(oHeap),
    snpType_(SNP_NONE),
    retHbaseColRefSet_(oHeap),
    opList_(oHeap)
{
  accessType_ = SELECT_;
  //setTableDesc(tableDesc);
  uniqueHbaseOper_ = FALSE;
  uniqueRowsetHbaseOper_ = FALSE;
}

HbaseAccess::HbaseAccess(CorrName &corrName,
			 NABoolean isRW, NABoolean isCW,
			 CollHeap *oHeap)
  : FileScan(corrName, NULL, NULL, REL_HBASE_ACCESS, oHeap),
    isRW_(isRW),
    isCW_(isCW),
    listOfSearchKeys_(oHeap),
    snpType_(SNP_NONE),
    retHbaseColRefSet_(oHeap),
    opList_(oHeap)
{
  accessType_ = SELECT_;
  uniqueHbaseOper_ = FALSE;
  uniqueRowsetHbaseOper_ = FALSE;
}

HbaseAccess::HbaseAccess( OperatorTypeEnum otype,
			  CollHeap *oHeap)
  : FileScan(CorrName(), NULL, NULL, otype, oHeap),
    listOfSearchKeys_(oHeap),
    snpType_(SNP_NONE),
    retHbaseColRefSet_(oHeap),
    opList_(oHeap)
{
  accessType_ = SELECT_;
  uniqueHbaseOper_ = FALSE;
  uniqueRowsetHbaseOper_ = FALSE;
}

//! HbaseAccess::~HbaseAccess Destructor 
HbaseAccess::~HbaseAccess()
{
}

//! HbaseAccess::copyTopNode method 
RelExpr * HbaseAccess::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  HbaseAccess *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseAccess(REL_HBASE_ACCESS, outHeap);
  else
    result = (HbaseAccess *) derivedNode;

  //result->corrName_ = corrName_;
  result->accessType_ = accessType_;
  result->isRW_ = isRW_;
  result->isCW_ = isCW_;

  result->setTableDesc(getTableDesc());
  result->setIndexDesc(getIndexDesc());

  //result->setTableDesc(getTableDesc());
  result->listOfSearchKeys_ = listOfSearchKeys_;

  result->retColRefSet_ = retColRefSet_;

  result->uniqueHbaseOper_ = uniqueHbaseOper_;
  result->uniqueRowsetHbaseOper_ = uniqueRowsetHbaseOper_;

  return Scan::copyTopNode(result, outHeap);
  //  return BuiltinTableValuedFunction::copyTopNode(result, outHeap);
}

const NAString HbaseAccess::getText() const
{
  NAString op(CmpCommon::statementHeap());
  NAString tname(getTableName().getText(),CmpCommon::statementHeap());

  NAString sampleOpt(CmpCommon::statementHeap());
  if (isSampleScan())
    sampleOpt = "sample_";

  if (getIndexDesc() == NULL OR getIndexDesc()->isClusteringIndex())
    {
      if (isSeabaseTable())
	{
	  if (uniqueRowsetHbaseOper())
	    (op += "trafodion_vsbb_") += sampleOpt += "scan ";
	  else
	    (op += "trafodion_") += sampleOpt += "scan ";
	}
      else
	(op += "hbase_") += sampleOpt += "scan ";
    }
  else 
    {
      if (isSeabaseTable())
	(op += "trafodion_index_") += sampleOpt += "scan ";
      else
	(op += "hbase_index_") += sampleOpt += "scan ";
 
      tname = getIndexDesc()->getIndexName().getQualifiedNameAsString() +
	"(" + tname + ")";
    }
  
  if (getReverseScan())
    op += NAString("rev ");

  return op + tname;
}

RelExpr *HbaseAccess::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }

  CorrName &corrName = getTableName();
  NATable * naTable = NULL;

  naTable = bindWA->getSchemaDB()->getNATableDB()->
    get(&corrName.getExtendedQualNameObj());

  if ( !naTable || bindWA->errStatus())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1388)
        << DgString0("Object")
	<< DgString1(corrName.getExposedNameAsAnsiString());
      
      bindWA->setErrStatus();
      return this;
    }
  
  // Allocate a TableDesc and attach it to this.
  //
  TableDesc * td = bindWA->createTableDesc(naTable, corrName);
  if (! td || bindWA->errStatus())
    return this;
  
  setTableDesc(td);
  setIndexDesc(td->getClusteringIndex());
  if (bindWA->errStatus())
    return this;
  
  RelExpr * re = NULL;
  //  re = BuiltinTableValuedFunction::bindNode(bindWA);
  re = Scan::bindNode(bindWA);
  if (bindWA->errStatus())
    return this;

  return re;
}

void HbaseAccess::getPotentialOutputValues(
     ValueIdSet & outputValues) const
{
  outputValues.clear();

  // since this is a physical operator, it only generates the index columns
  outputValues.insertList( getIndexDesc()->getIndexColumns() );
  outputValues.insertList( getTableDesc()->hbaseTSList() );
  outputValues.insertList( getTableDesc()->hbaseVersionList() );
  
} // HbaseAccess::getPotentialOutputValues()

void
HbaseAccess::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp))
    return;

  // Create a new Output Log Property with cardinality of 10 for now.
  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(10));

  getGroupAttr()->addInputOutputLogProp(inputEstLogProp, myEstProps);

} // HbaseAccess::synthEstLogProp

void
HbaseAccess::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  //  for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  RelExpr::synthLogProp(normWAPtr);

} // HbaseAccess::synthLogProp()

// -----------------------------------------------------------------------
// methods for class HBaseAccessCoProcAggr
// -----------------------------------------------------------------------

HbaseAccessCoProcAggr::HbaseAccessCoProcAggr(CorrName &corrName,
					     ValueIdSet &aggregateExpr,
					     TableDesc *tableDesc,
					     IndexDesc *idx,
					     const NABoolean isReverseScan,
					     const Cardinality& baseCardinality,
					     StmtLevelAccessOptions& accessOptions,
					     GroupAttributes * groupAttributesPtr,
					     const ValueIdSet& selectionPredicates,
					     const Disjuncts& disjuncts,
					     CollHeap *oHeap)
  : HbaseAccess(corrName, tableDesc, idx, 
		isReverseScan, baseCardinality,
		accessOptions, groupAttributesPtr,
		selectionPredicates, disjuncts,
                ValueIdSet(),
		REL_HBASE_COPROC_AGGR),
    aggregateExpr_(aggregateExpr)
{
  accessType_ = COPROC_AGGR_;

}

HbaseAccessCoProcAggr::HbaseAccessCoProcAggr(CorrName &corrName,
					     ValueIdSet &aggregateExpr,
					     CollHeap *oHeap)
  : HbaseAccess(corrName, 
		REL_HBASE_COPROC_AGGR,
		oHeap),
    aggregateExpr_(aggregateExpr)
{
  accessType_ = COPROC_AGGR_;
}

HbaseAccessCoProcAggr::HbaseAccessCoProcAggr( CollHeap *oHeap)
  : HbaseAccess(REL_HBASE_COPROC_AGGR, oHeap)
{
  accessType_ = SELECT_;

  uniqueHbaseOper_ = FALSE;
  uniqueRowsetHbaseOper_ = FALSE;
}

//! HbaseAccessCoProcAggr::~HbaseAccessCoProcAggr Destructor 
HbaseAccessCoProcAggr::~HbaseAccessCoProcAggr()
{
}

//! HbaseAccessCoProcAggr::copyTopNode method 
RelExpr * HbaseAccessCoProcAggr::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  HbaseAccessCoProcAggr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseAccessCoProcAggr(outHeap);
  else
    result = (HbaseAccessCoProcAggr *) derivedNode;

  result->aggregateExpr_ = aggregateExpr_;

  return HbaseAccess::copyTopNode(result, outHeap);
}

const NAString HbaseAccessCoProcAggr::getText() const
{
  NAString op(CmpCommon::statementHeap());
  NAString tname(getTableName().getText(),CmpCommon::statementHeap());

  op += "hbase_coproc_aggr ";

  return op + tname;
}

RelExpr *HbaseAccessCoProcAggr::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }

  RelExpr * re = NULL;
  re = HbaseAccess::bindNode(bindWA);
  if (bindWA->errStatus())
    return this;

  return re;
}

void HbaseAccessCoProcAggr::getPotentialOutputValues(
     ValueIdSet & outputValues) const
{
  outputValues.clear();

  outputValues += aggregateExpr();
} // HbaseAccessCoProcAggr::getPotentialOutputValues()

PhysicalProperty*
HbaseAccessCoProcAggr::synthPhysicalProperty(const Context* myContext,
					     const Lng32     planNumber,
                                             PlanWorkSpace  *pws)
{
  
  //----------------------------------------------------------
  // Create a node map with a single, active, wild-card entry.
  //----------------------------------------------------------
  NodeMap* myNodeMap = new(CmpCommon::statementHeap())
    NodeMap(CmpCommon::statementHeap(),
	    1,
	    NodeMapEntry::ACTIVE);
  
  //------------------------------------------------------------
  // Synthesize a partitioning function with a single partition.
  //------------------------------------------------------------
  PartitioningFunction* myPartFunc =
    new(CmpCommon::statementHeap())
    SinglePartitionPartitioningFunction(myNodeMap);
  
  PhysicalProperty * sppForMe =
    new(CmpCommon::statementHeap())
    PhysicalProperty(myPartFunc,
                     EXECUTE_IN_MASTER,
                     SOURCE_VIRTUAL_TABLE);
  
  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;
  
  return sppForMe;
} //  HbaseAccessCoProcAggr::synthPhysicalProperty()

// -----------------------------------------------------------------------
// methods for class HbaseDelete
// -----------------------------------------------------------------------

HbaseDelete::HbaseDelete(CorrName &corrName,
			 RelExpr *scan,
			 CollHeap *oHeap)
  : Delete(corrName, NULL, REL_HBASE_DELETE, scan, NULL, NULL, NULL, oHeap),
    corrName_(corrName), 
    listOfSearchKeys_(oHeap) 
{ 
   hbaseOper() = TRUE; 
}
 

HbaseDelete::HbaseDelete(CorrName &corrName,
			 TableDesc *tableDesc,
			 CollHeap *oHeap)
  : Delete(corrName, tableDesc, REL_HBASE_DELETE, NULL, NULL, NULL, NULL,oHeap),
    corrName_(corrName),
    listOfSearchKeys_(oHeap) 
{
  hbaseOper() = TRUE;

}

HbaseDelete::HbaseDelete( CollHeap *oHeap)
  : Delete(CorrName(""), NULL, REL_HBASE_DELETE, NULL, NULL, NULL, NULL, oHeap),
    listOfSearchKeys_(oHeap) 
{
  hbaseOper() = TRUE;
}

//! HbaseDelete::~HbaseDelete Destructor 
HbaseDelete::~HbaseDelete()
{
}

//! HbaseDelete::copyTopNode method 
RelExpr * HbaseDelete::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  HbaseDelete *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseDelete(corrName_, getTableDesc(), outHeap);
  else
    result = (HbaseDelete *) derivedNode;

  result->corrName_ = corrName_;
  result->setTableDesc(getTableDesc());

  result->listOfSearchKeys_ = listOfSearchKeys_;

  result->retColRefSet_ = retColRefSet_;

  return Delete::copyTopNode(result, outHeap);
}

RelExpr *HbaseDelete::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }

  RelExpr * re = NULL;
  re = Delete::bindNode(bindWA);
  if (bindWA->errStatus())
    return this;

  return re;
}

//! HbaseDelete::getText method 
const NAString HbaseDelete::getText() const
{
  NABoolean isSeabase = 
    (getTableDesc() && getTableDesc()->getNATable() ? 
     getTableDesc()->getNATable()->isSeabaseTable() : FALSE);

  NAString text;

  if (NOT isSeabase)
    text = "hbase_";
  else
    text = "trafodion_";

  if (uniqueRowsetHbaseOper())
    text += "vsbb_";

  text += "delete";

  return text;
}

Int32 HbaseDelete::getArity() const
{
  return 0;
}

void HbaseDelete::getPotentialOutputValues(
     ValueIdSet & outputValues) const
{
  outputValues.clear();
  // since this is a physical operator, it only generates the index columns
  if (getScanIndexDesc())
    outputValues.insertList(getScanIndexDesc()->getIndexColumns());
} // HbaseDelete::getPotentialOutputValues()

// -----------------------------------------------------------------------
// methods for class HbaseUpdate
// -----------------------------------------------------------------------

HbaseUpdate::HbaseUpdate(CorrName &corrName,
			 RelExpr *scan,
			 CollHeap *oHeap)
  : UpdateCursor(corrName, NULL, REL_HBASE_UPDATE, scan, oHeap),
    corrName_(corrName), 
    listOfSearchKeys_(oHeap) 
{ 
   hbaseOper() = TRUE; 
}
 

HbaseUpdate::HbaseUpdate(CorrName &corrName,
			 TableDesc *tableDesc,
			 CollHeap *oHeap)
  : UpdateCursor(corrName, tableDesc, REL_HBASE_UPDATE, NULL, oHeap),
    corrName_(corrName),
    listOfSearchKeys_(oHeap) 
{
  hbaseOper() = TRUE;

}

HbaseUpdate::HbaseUpdate( CollHeap *oHeap)
  : UpdateCursor(CorrName(""), NULL, REL_HBASE_UPDATE, NULL, oHeap),
    listOfSearchKeys_(oHeap) 
{
  hbaseOper() = TRUE;
}

//! HbaseUpdate::~HbaseUpdate Destructor 
HbaseUpdate::~HbaseUpdate()
{
}

//! HbaseUpdate::copyTopNode method 
RelExpr * HbaseUpdate::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  HbaseUpdate *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseUpdate(corrName_, getTableDesc(), outHeap);
  else
    result = (HbaseUpdate *) derivedNode;

  result->corrName_ = corrName_;
  result->setTableDesc(getTableDesc());

  result->listOfSearchKeys_ = listOfSearchKeys_;

  result->retColRefSet_ = retColRefSet_;

  return Update::copyTopNode(result, outHeap);
}

RelExpr *HbaseUpdate::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }

  RelExpr * re = NULL;
  re = Update::bindNode(bindWA);
  if (bindWA->errStatus())
    return this;

  return re;
}

//! HbaseUpdate::getText method 
const NAString HbaseUpdate::getText() const
{
  NABoolean isSeabase = 
    (getTableDesc() ? getTableDesc()->getNATable()->isSeabaseTable() : FALSE);

  NAString text;
  if (isMerge())
    {
      text = (isSeabase ? "trafodion_merge" : "hbase_merge");
    }
  else
    {
      if (NOT isSeabase)
	text = "hbase_";
      else
	text = "trafodion_";
      
      if (uniqueRowsetHbaseOper())
	text += "vsbb_";
      
      text += "update";
    }
  
  return text;
}

Int32 HbaseUpdate::getArity() const
{
  return 0;
}

void HbaseUpdate::getPotentialOutputValues(
     ValueIdSet & outputValues) const
{
  outputValues.clear();
  // Include the index columns from the original Scan, if any
  if (getScanIndexDesc())
    outputValues.insertList(getScanIndexDesc()->getIndexColumns());

  // Include the index columns from the updated table, if any
  if (getIndexDesc())
  outputValues.insertList (getIndexDesc()->getIndexColumns());

} // HbaseUpdate::getPotentialOutputValues()


// -----------------------------------------------------------------------
// Member functions for DP2 Scan
// -----------------------------------------------------------------------
DP2Scan::DP2Scan(const CorrName& tableName,
		 TableDesc * tableDescPtr,
		 const IndexDesc *indexDescPtr,
		 const NABoolean isReverseScan,
		 const Cardinality& baseCardinality,
		 StmtLevelAccessOptions& accessOpts,
		 GroupAttributes * groupAttributesPtr,
		 const ValueIdSet& selectionPredicates,
		 const Disjuncts& disjuncts)
       :  FileScan(tableName,
		   tableDescPtr,
		   indexDescPtr,
		   isReverseScan,
		   baseCardinality,
		   accessOpts,
		   groupAttributesPtr,
		   selectionPredicates,
		   disjuncts,
                   ValueIdSet())
{
}

// --------------------------------------------------
// methods for class Describe
// --------------------------------------------------
void Describe::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  //
  // Assign the set of columns that belong to the index to be scanned
  // as the output values that can be produced by this scan.
  //
  outputValues.insertList( getTableDesc()->getClusteringIndex()->getIndexColumns() );
} // Describe::getPotentialOutputValues()

// -----------------------------------------------------------------------
// methods for class RelRoot
// -----------------------------------------------------------------------

RelRoot::RelRoot(RelExpr *input,
		 OperatorTypeEnum otype,
		 ItemExpr *compExpr,
		 ItemExpr *orderBy,
		 ItemExpr *updateCol,
		 RelExpr *reqdShape,
		 CollHeap *oHeap)
  : RelExpr(otype, input, NULL, oHeap),
    compExprTree_(compExpr),
    orderByTree_(orderBy),
    updateColTree_(updateCol),
    reqdShape_(reqdShape),
    viewStoiList_(CmpCommon::statementHeap()),
    ddlStoiList_(CmpCommon::statementHeap()),
    stoiUdrList_(CmpCommon::statementHeap()),
    udfList_(CmpCommon::statementHeap()),
    securityKeySet_(CmpCommon::statementHeap()),
    trueRoot_(FALSE),
    subRoot_(FALSE),
    displayTree_(FALSE),
    exeDisplay_(FALSE),
    outputVarCnt_(-1),
    inputVarTree_(NULL),
    outputVarTree_(NULL),
    updatableSelect_(TRUE),
    updateCurrentOf_(FALSE),
    currOfCursorName_(NULL),
    rollbackOnError_(FALSE),
    readOnlyTransIsOK_(FALSE),
    needFirstSortedRows_(FALSE),
    numSimpleVar_(0),
    numHostVar_(0),
    childOperType_(NO_OPERATOR_TYPE),
    hostArraysArea_(NULL),
    assignmentStTree_(NULL),
    assignList_(NULL),
    isRootOfInternalRefresh_(FALSE),
    isQueryNonCacheable_(FALSE),
    pMvBindContextForScope_(NULL),
    parentForRowsetReqdOrder_(NULL),
    isEmptySelectList_(FALSE),
    isDontOpenNewScope_(FALSE),
    triggersList_(NULL),
    spOutParams_(NULL),
    downrevCompileMXV_(COM_VERS_CURR_PLAN),
    numExtractStreams_(0),
    numBMOs_(0),
    BMOsMemoryUsage_(0),
    nBMOsMemoryUsage_(0),
    uninitializedMvList_(NULL),
    allOrderByRefsInGby_(FALSE),
    avoidHalloween_(FALSE),
    containsOnStatementMV_(FALSE),
    containsLRU_(FALSE),
    disableESPParallelism_(FALSE),
    hasOlapFunctions_(FALSE),
    hasTDFunctions_(FALSE),
    isAnalyzeOnly_(FALSE),
    hasMandatoryXP_(FALSE),
    partReqType_(ANY_PARTITIONING),
    partitionByTree_(NULL),
    predExprTree_(NULL),
    firstNRowsParam_(NULL),
    flags_(0)
{
  accessOptions().accessType() = TransMode::ACCESS_TYPE_NOT_SPECIFIED_;
  accessOptions().lockMode() = LOCK_MODE_NOT_SPECIFIED_;
  isCIFOn_ = FALSE;
}

RelRoot::RelRoot(RelExpr *input,
		 TransMode::AccessType at,
		 LockMode lm,
		 OperatorTypeEnum otype,
		 ItemExpr *compExpr,
		 ItemExpr *orderBy,
		 ItemExpr *updateCol,
		 RelExpr *reqdShape,
		 CollHeap *oHeap)
  : RelExpr(otype, input, NULL, oHeap),
    compExprTree_(compExpr),
    orderByTree_(orderBy),
    updateColTree_(updateCol),
    reqdShape_(reqdShape),
    viewStoiList_(CmpCommon::statementHeap()),
    ddlStoiList_(CmpCommon::statementHeap()),
    stoiUdrList_(CmpCommon::statementHeap()),
    udfList_(CmpCommon::statementHeap()),
    securityKeySet_(CmpCommon::statementHeap()),
    trueRoot_(FALSE),
    subRoot_(FALSE),
    displayTree_(FALSE),
    exeDisplay_(FALSE),
    outputVarCnt_(-1),
    inputVarTree_(NULL),
    outputVarTree_(NULL),
    updatableSelect_(TRUE),
    updateCurrentOf_(FALSE),
    currOfCursorName_(NULL),
    rollbackOnError_(FALSE),
    readOnlyTransIsOK_(FALSE),
    needFirstSortedRows_(FALSE),
    numSimpleVar_(0),
    numHostVar_(0),
    childOperType_(NO_OPERATOR_TYPE),
    hostArraysArea_(NULL),
    assignmentStTree_(NULL),
    assignList_(NULL),
    isRootOfInternalRefresh_(FALSE),
    isQueryNonCacheable_(FALSE),
    pMvBindContextForScope_(NULL),
    parentForRowsetReqdOrder_(NULL),
    isEmptySelectList_(FALSE),
    isDontOpenNewScope_(FALSE),
    triggersList_(NULL),
    spOutParams_(NULL),
    downrevCompileMXV_(COM_VERS_CURR_PLAN),
    numExtractStreams_(0),
    numBMOs_(0),
    BMOsMemoryUsage_(0),
    nBMOsMemoryUsage_(0),
    uninitializedMvList_(NULL),
    allOrderByRefsInGby_(FALSE),
    avoidHalloween_(FALSE),
    containsOnStatementMV_(FALSE),
    containsLRU_(FALSE),
    disableESPParallelism_(FALSE),
    hasOlapFunctions_(FALSE),
    hasTDFunctions_(FALSE),
    isAnalyzeOnly_(FALSE),
    hasMandatoryXP_(FALSE),
    partReqType_(ANY_PARTITIONING),
    partitionByTree_(NULL),
    predExprTree_(NULL),
    firstNRowsParam_(NULL),
    flags_(0)
{
  accessOptions().accessType() = at;
  accessOptions().lockMode()   = lm;
  isCIFOn_ = FALSE;
}

// Why not just use the default copy ctor that C++ provides automatically, ##
// rather than having to maintain this??? 			##
// Is it because of the "numXXXVar_(0)" lines below, 		##
// or should those be   "numXXXVar_(other.numXXXVar_)" ?	##
RelRoot::RelRoot(const RelRoot & other)
  : RelExpr(REL_ROOT, other.child(0)),
    compExprTree_(other.compExprTree_),
    orderByTree_(other.orderByTree_),
    updateColTree_(other.updateColTree_),
    reqdShape_(other.reqdShape_),
    viewStoiList_(other.viewStoiList_),
    ddlStoiList_(other.ddlStoiList_),
    stoiUdrList_(other.stoiUdrList_),
    udfList_(other.udfList_),
    securityKeySet_(other.securityKeySet_),
    trueRoot_(other.trueRoot_),
    subRoot_(other.subRoot_),
    displayTree_(other.displayTree_),
    exeDisplay_(other.exeDisplay_),
    outputVarCnt_(other.outputVarCnt_),
    inputVarTree_(other.inputVarTree_),
    outputVarTree_(other.outputVarTree_),
    updatableSelect_(other.updatableSelect_),
    updateCurrentOf_(other.updateCurrentOf_),
    currOfCursorName_(other.currOfCursorName_),
    rollbackOnError_(other.rollbackOnError_),
    readOnlyTransIsOK_(other.readOnlyTransIsOK_),
    needFirstSortedRows_(other.needFirstSortedRows_),
    isRootOfInternalRefresh_(other.isRootOfInternalRefresh_),
    isQueryNonCacheable_(other.isQueryNonCacheable_),
    pMvBindContextForScope_(other.pMvBindContextForScope_),
    isEmptySelectList_(other.isEmptySelectList_),
    isDontOpenNewScope_(other.isDontOpenNewScope_),
    //    oltOptInfo_(other.oltOptInfo_),
    numSimpleVar_(0),						//## bug?
    numHostVar_(0),						//## bug?
    childOperType_(other.childOperType_),
    hostArraysArea_(other.hostArraysArea_),
    assignmentStTree_(other.assignmentStTree_),
    assignList_(other.assignList_),
    triggersList_(other.triggersList_),
    compExpr_(other.compExpr_),
    reqdOrder_(other.reqdOrder_),
    partArrangement_(other.partArrangement_),
    updateCol_(other.updateCol_),
    inputVars_(other.inputVars_),
    accessOptions_(other.accessOptions_),
    pkeyList_(other.pkeyList_),
    rowsetReqdOrder_(other.rowsetReqdOrder_),
    parentForRowsetReqdOrder_(other.parentForRowsetReqdOrder_),
    spOutParams_ (NULL), // Raj P - 12/2000 - stored procedures (for java)
    downrevCompileMXV_(COM_VERS_CURR_PLAN),
    uninitializedMvList_(other.uninitializedMvList_),
    allOrderByRefsInGby_(other.allOrderByRefsInGby_),
    numExtractStreams_(other.numExtractStreams_),
    numBMOs_(other.numBMOs_),
    BMOsMemoryUsage_(other.BMOsMemoryUsage_),
    nBMOsMemoryUsage_(other.nBMOsMemoryUsage_),
    avoidHalloween_(other.avoidHalloween_),
    disableESPParallelism_(other.disableESPParallelism_),
    containsOnStatementMV_(other.containsOnStatementMV_),
    containsLRU_(other.containsLRU_),
    hasOlapFunctions_(other.hasOlapFunctions_),
    hasTDFunctions_(other.hasTDFunctions_ ),
    isAnalyzeOnly_(FALSE),
    hasMandatoryXP_(other.hasMandatoryXP_),
    partReqType_(other.partReqType_),
    partitionByTree_(other.partitionByTree_),
    isCIFOn_(other.isCIFOn_),
    predExprTree_(other.predExprTree_),
    firstNRowsParam_(other.firstNRowsParam_),
    flags_(other.flags_)
{
  oltOptInfo() = ((RelRoot&)other).oltOptInfo();
  setRETDesc(other.getRETDesc());
}

RelRoot::~RelRoot()
{
  //	Explicitly deleting our members is deliberately not being done --
  //	we should allocate all RelExpr's on the appropriate heap
  //	(as a NABasicObject) and free the (stmt) heap all in one blow.
  // delete compExprTree_;
  // delete orderByTree_;
  // delete reqdShape_;
  // delete inputVarTree_;
}

Int32 RelRoot::getArity() const { return 1; }

void RelRoot::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  //
  // Assign the select list as the outputs
  //
  outputValues.insertList(compExpr());
} // RelRoot::getPotentialOutputValues()


void RelRoot::pushdownCoveredExpr(const ValueIdSet & outputExpr,
                                  const ValueIdSet & newExternalInputs,
                                  ValueIdSet & predicatesOnParent,
				  const ValueIdSet * setOfValuesReqdByParent,
				  Lng32   //childIndex ignored
		                 )
{
  //---------------------------------------------------------------------
  // case 10-030708-7671: In the case of a cast, the RelRoot operator
  // needs to ask for original expression it is casting; if it asks for
  // the cast expression, the child may be incapable of producing it. For
  // example, a nested join operator can't produce the cast expression,
  // unless produced by its children
  //---------------------------------------------------------------------
  ValueIdSet originalOutputs = getGroupAttr()->getCharacteristicOutputs();
  ValueIdSet updateOutputs(originalOutputs);

  updateOutputs.replaceCastExprWithOriginal(originalOutputs, this);


  ValueIdSet myCharInput = getGroupAttr()->getCharacteristicInputs();

  // since the orderby list is not a subset of the select list include it.
  ValueIdSet allMyExpr;
  ValueIdList orderByList = reqdOrder();
  allMyExpr.insertList(orderByList);

  // add the primary key columns, if they are to be returned.
  if (updatableSelect() == TRUE)
  {
    allMyExpr.insertList(pkeyList());
  }
  // ---------------------------------------------------------------------
  RelExpr::pushdownCoveredExpr(updateOutputs,
                               myCharInput,
                               predicatesOnParent,
			       &allMyExpr
                               );

  // All expressions should have been pushed
  CMPASSERT(predicatesOnParent.isEmpty());

} // RelRoot::pushdownCoveredExpr

const NAString RelRoot::getText() const
{
	NAString result("root");

#ifdef DEBUG_TRIGGERS
	char totalNodes[20];
	sprintf(totalNodes, "(total %d nodes)", nodeCount());
	result += totalNodes;

	if (isDontOpenNewScope())
		result += "(no_scope)";

	if (isEmptySelectList())
		result += "(empty_select_list)";
#endif

	return result;
}

HashValue RelRoot::topHash()
{
  HashValue result = RelExpr::topHash();

  // it's not (yet) needed to produce a really good hash value for this node
  result ^= compExpr_.entries();
  result ^= inputVars_.entries();
  // result ^= compExpr_;
  // result ^= inputVars_;
  return result;
}

NABoolean RelRoot::duplicateMatch(const RelExpr & other) const
{
  if (!RelExpr::duplicateMatch(other))
    return FALSE;

  RelRoot &o = (RelRoot &) other;

  if (NOT (compExpr_ == o.compExpr_) OR
      NOT (inputVars_ == o.inputVars_))
    return FALSE;

  if (avoidHalloween_ != o.avoidHalloween_)
    return FALSE;

  if (disableESPParallelism_ != o.disableESPParallelism_)
    return FALSE;

  if (isAnalyzeOnly_ != o.isAnalyzeOnly_)
    return FALSE;

  return TRUE;
}

RelExpr * RelRoot::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelRoot *result;

  if (derivedNode == NULL)
    result = new (outHeap) RelRoot(NULL,
				   getOperatorType(),
				   NULL,
				   NULL,
				   NULL,
				   NULL,
				   outHeap);
  else
    result = (RelRoot *) derivedNode;

  if (compExprTree_ != NULL)
    result->compExprTree_ = compExprTree_->copyTree(outHeap)->castToItemExpr();

  if (inputVarTree_ != NULL)
    result->inputVarTree_ = inputVarTree_->copyTree(outHeap)->castToItemExpr();

  if (outputVarTree_ != NULL)
    result->outputVarTree_ = outputVarTree_->copyTree(outHeap)->castToItemExpr();

  if (predExprTree_ != NULL)
    result->predExprTree_ = predExprTree_->copyTree(outHeap)->castToItemExpr();

  result->compExpr_ = compExpr_;
  result->inputVars_ = inputVars_;

  result->accessOptions_ = accessOptions_;

  result->updatableSelect_ = updatableSelect_;
  result->updateCurrentOf_ = updateCurrentOf_;
  if (currOfCursorName())
    result->currOfCursorName_ = currOfCursorName()->copyTree(outHeap)->castToItemExpr();

  result->rollbackOnError_ = rollbackOnError_;

  result->isEmptySelectList_ = isEmptySelectList_;
  result->isDontOpenNewScope_ = isDontOpenNewScope_;

  result->oltOptInfo() = oltOptInfo();

  result->childOperType_ = childOperType_;

  result->rowsetReqdOrder_ = rowsetReqdOrder_;
  result->parentForRowsetReqdOrder_ = parentForRowsetReqdOrder_;

  // Raj P - 12/2000 stored procedures (for java)
  if ( spOutParams_ )
  {
    result->spOutParams_ = new ItemExprList (outHeap);
    (*result->spOutParams_) = *spOutParams_;
  }

  if( uninitializedMvList_ )
  {
      result->uninitializedMvList_ = new UninitializedMvNameList (outHeap);
      result->uninitializedMvList_->insert( *uninitializedMvList_ );
  }

  result->setDownrevCompileMXV(getDownrevCompileMXV());
  result->numExtractStreams_ = numExtractStreams_;
  result->allOrderByRefsInGby_ = allOrderByRefsInGby_;

  result->avoidHalloween_ = avoidHalloween_;

  result->disableESPParallelism_ = disableESPParallelism_;

  result->containsOnStatementMV_ = containsOnStatementMV_;

  result->hasOlapFunctions_ = hasOlapFunctions_;

  result->containsLRU_ = containsLRU_;

  result->isAnalyzeOnly_ = isAnalyzeOnly_;

  result->hasMandatoryXP_ = hasMandatoryXP_ ;

  if (partitionByTree_ != NULL)
    result->partitionByTree_ = partitionByTree_->copyTree(outHeap)->castToItemExpr();
  
  result->partReqType_ = partReqType_ ;

  result->isQueryNonCacheable_ = isQueryNonCacheable_; 

  result->firstNRowsParam_ = firstNRowsParam_;

  result->flags_ = flags_;

  return RelExpr::copyTopNode(result, outHeap);
}

void RelRoot::addCompExprTree(ItemExpr *compExpr)
{
  ExprValueId c = compExprTree_;

  ItemExprTreeAsList(&c, ITM_ITEM_LIST).insert(compExpr);
  compExprTree_ = c.getPtr();
}

ItemExpr * RelRoot::removeCompExprTree()
{
  ItemExpr * result = compExprTree_;

  compExprTree_ = NULL;

  return result;
}

void RelRoot::addPredExprTree(ItemExpr *predExpr)
{
  ExprValueId c = predExprTree_;

  ItemExprTreeAsList(&c, ITM_ITEM_LIST).insert(predExpr);
  predExprTree_ = c.getPtr();
}

ItemExpr * RelRoot::removePredExprTree()
{
  ItemExpr * result = predExprTree_;

  predExprTree_ = NULL;

  return result;
}

void RelRoot::addInputVarTree(ItemExpr *inputVar)
{
  ExprValueId c = inputVarTree_;

  ItemExprTreeAsList(&c, ITM_ITEM_LIST).insert(inputVar);
  inputVarTree_ = c.getPtr();
}

void RelRoot::addAtTopOfInputVarTree(ItemExpr *inputVar)
{
  ExprValueId c = inputVarTree_;

  ItemExprTreeAsList(&c, ITM_ITEM_LIST).insertAtTop(inputVar);
  inputVarTree_ = c.getPtr();
}

ItemExpr * RelRoot::removeInputVarTree()
{
  ItemExpr * result = inputVarTree_;

  inputVarTree_ = NULL;

  return result;
}

void RelRoot::addOutputVarTree(ItemExpr *outputVar)
{
  if (!outputVarTree_) {
    outputVarTree_ = new(CmpCommon::statementHeap()) ItemList(outputVar, NULL);
  }
  else {
    ItemExpr *start = outputVarTree_;
    while (start->child(1)) {
	  start = start->child(1);
    }
    start->child(1) = new(CmpCommon::statementHeap()) ItemList(outputVar, NULL);
  }
}

// Used by Assignment Statement in a Compound Statement. It adds a host variable
// to assignmentStTree_
void RelRoot::addAssignmentStTree(ItemExpr *inputOutputVar)
{
  if (!assignmentStTree_) {
     assignmentStTree_ = new(CmpCommon::statementHeap()) ItemList(inputOutputVar, NULL);
  }
  else {
    ItemExpr *start = assignmentStTree_;
    while (start->child(1)) {
       start = start->child(1);
    }
    start->child(1) = new(CmpCommon::statementHeap()) ItemList(inputOutputVar, NULL);
  }
}

ItemExpr * RelRoot::removeOutputVarTree()
{
  ItemExpr * result = outputVarTree_;

  outputVarTree_ = NULL;

  return result;
}

void RelRoot::addOrderByTree(ItemExpr *orderBy)
{
  ExprValueId c = orderByTree_;

  ItemExprTreeAsList(&c, ITM_ITEM_LIST).insert(orderBy);
  orderByTree_ = c.getPtr();
}

ItemExpr * RelRoot::removeOrderByTree()
{
  ItemExpr * result = orderByTree_;

  orderByTree_ = NULL;

  return result;
}

void RelRoot::addPartitionByTree(ItemExpr *partBy)
{
  ExprValueId c = partitionByTree_;

  ItemExprTreeAsList(&c, ITM_ITEM_LIST).insert(partBy);
  partitionByTree_ = c.getPtr();
}

ItemExpr * RelRoot::removePartitionByTree()
{
  ItemExpr * result = partitionByTree_;

  partitionByTree_ = NULL;

  return result;
}

void RelRoot::addUpdateColTree(ItemExpr *updateCol)
{
  ExprValueId c = updateColTree_;

  ItemExprTreeAsList(&c, ITM_ITEM_LIST).insert(updateCol);
  updateColTree_ = c.getPtr();
}

ItemExpr * RelRoot::removeUpdateColTree()
{
  ItemExpr * result = updateColTree_;

  updateColTree_ = NULL;

  return result;
}

void RelRoot::addLocalExpr(LIST(ExprNode *) &xlist,
			   LIST(NAString) &llist) const
{
  if (compExprTree_ != NULL OR
      compExpr_.entries() == 0)
    xlist.insert(compExprTree_);
  else
    xlist.insert(compExpr_.rebuildExprTree(ITM_ITEM_LIST));
  llist.insert("select_list");

  if (inputVarTree_ != NULL OR
      inputVars_.entries() > 0)
    {
      if (inputVars_.entries() == 0)
	xlist.insert(inputVarTree_);
      else
	xlist.insert(inputVars_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("input_variables");
    }

  if (orderByTree_ != NULL OR
      reqdOrder_.entries() > 0)
    {
      if (reqdOrder_.entries() == 0)
	xlist.insert(orderByTree_);
      else
	xlist.insert(reqdOrder_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("order_by");
    }

    if ((partitionByTree_ != NULL) OR 
        (partArrangement_.entries() > 0))
    {
      if (partArrangement_.entries() == 0)
	xlist.insert(partitionByTree_);
      else
	xlist.insert(partArrangement_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("partition_by");
    }


  if (updateColTree_ != NULL OR
      updateCol_.entries() > 0)
    {
      if (updateCol_.entries() == 0)
	xlist.insert(updateColTree_);
      else
	xlist.insert(updateCol_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("update_col");
    }

  if (reqdShape_ != NULL)
    {
      xlist.insert(reqdShape_);
      llist.insert("must_match");
    }

  RelExpr::addLocalExpr(xlist,llist);
}

//----------------------------------------------------------------------------
//++ MV OZ
NABoolean RelRoot::hasMvBindContext() const
{
  return (NULL != pMvBindContextForScope_) ? TRUE : FALSE;
}

MvBindContext * RelRoot::getMvBindContext() const
{
  return pMvBindContextForScope_;
}

void RelRoot::setMvBindContext(MvBindContext * pMvBindContext)
{
  pMvBindContextForScope_ = pMvBindContext;
}

NABoolean RelRoot::addOneRowAggregates(BindWA* bindWA, NABoolean forceGroupByAgg)
{
  NABoolean groupByAggNodeAdded = FALSE;
  RelExpr * childOfRoot = child(0);
  GroupByAgg *aggNode = NULL;
  // If the One Row Subquery is already enforced by a scalar aggregate
  // then we do not need to add an additional one row aggregate. The exceptions to
  // this rule is if we have count(t1.a) where t1 itself is a one row subquery.
  // Note that the count is needed in order to have a groupby below the root node.
  // See soln. 10-071105-8680 
  // Another exception is when the select list has say max(a) + select a from t1
  // In this case there is a onerowsubquery in the select list but it is a child
  // of BiArith. Due to all these exceptions we are simply going to scan the select 
  // list. As soon as we find something other than an aggregate we take the safe
  // way out and add a one row aggregate.
  // Also if the groupby is non scalar then we need to add a one row aggregate.
  // Also if we have select max(a) + select b from t1 from t2;
  // Still another exception is if there is a [last 0] on top of this node. We
  // need an extra GroupByAgg node with one row aggregates in this case so
  // we can put the FirstN node underneath that.
  if (!forceGroupByAgg &&
      (childOfRoot->getOperatorType() == REL_GROUPBY))
    {
      aggNode = (GroupByAgg *)childOfRoot;

      if (!aggNode->groupExpr().isEmpty())
        aggNode = NULL;

      // Check to see if the compExpr contains a subquery.
      for (CollIndex i=0; i < compExpr().entries(); i++) 
	if (compExpr()[i].getItemExpr()->containsOpType(ITM_ROW_SUBQUERY))
        {
	  aggNode = NULL ;
          break;
        }

    }
  if (aggNode)
    return groupByAggNodeAdded;

  const RETDesc *oldTable = getRETDesc();
  RETDesc *resultTable = new(bindWA->wHeap()) RETDesc(bindWA);
  
  
  // Transform select list such that that each item has a oneRow parent.
  for (CollIndex selectListIndex=0; 
       selectListIndex < compExpr().entries(); 
       selectListIndex++)
  {
  
    ItemExpr *colExpr = compExpr()[selectListIndex].getItemExpr();

    // Build a new OneRow aggregate on top of the existing expression.
    ItemExpr *newColExpr = new(bindWA->wHeap()) 
      Aggregate(ITM_ONEROW, colExpr);

    newColExpr->bindNode(bindWA);

    ColumnNameMap *xcnmEntry = oldTable->findColumn(compExpr()[selectListIndex]);

    if (xcnmEntry)	// ## I don't recall when this case occurs...
      resultTable->addColumn(bindWA,
	  		     xcnmEntry->getColRefNameObj(),
	  		     newColExpr->getValueId(),
			     USER_COLUMN,
			     xcnmEntry->getColumnDesc()->getHeading());
    else
    {
      ColRefName colRefName;
      resultTable->addColumn(bindWA,
	                      colRefName,
			      newColExpr->getValueId());
    }

    // Replace the select list expression with the new one.
    compExpr()[selectListIndex] = newColExpr->getValueId();
  }
 

  ValueIdSet aggregateExpr(compExpr()) ;
  GroupByAgg *newGrby = NULL;
  newGrby = new(bindWA->wHeap())
	                 GroupByAgg(childOfRoot, aggregateExpr);

  newGrby->bindNode(bindWA) ;
  child(0) = newGrby ;
  groupByAggNodeAdded = TRUE;
  // Set the return descriptor
  //
  setRETDesc(resultTable);

  return groupByAggNodeAdded;
}
// -----------------------------------------------------------------------
// member functions for class PhysicalRelRoot
// -----------------------------------------------------------------------

NABoolean PhysicalRelRoot::isLogical()  const { return FALSE; }

NABoolean PhysicalRelRoot::isPhysical() const { return TRUE; }


// -----------------------------------------------------------------------
// methods for class Tuple
// -----------------------------------------------------------------------

THREAD_P Lng32 Tuple::idCounter_(0);

Tuple::Tuple(const Tuple & other) : RelExpr(other.getOperatorType())
{
  selectionPred() = other.getSelectionPred();

  tupleExprTree_ = other.tupleExprTree_;
  tupleExpr_ = other.tupleExpr_;
  rejectPredicates_ = other.rejectPredicates_;
  id_ = other.id_;
}

Tuple::~Tuple() {}

Int32 Tuple::getArity() const { return 0; }

// -----------------------------------------------------------------------
// A virtual method for computing output values that an operator can
// produce potentially.
// -----------------------------------------------------------------------
void Tuple::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  outputValues.insertList( tupleExpr() );
} // Tuple::getPotentialOutputValues()

HashValue Tuple::topHash()
{
  HashValue result = RelExpr::topHash();

  result ^= tupleExpr_.entries();

  return result;
}

NABoolean Tuple::duplicateMatch(const RelExpr & other) const
{
  if (!RelExpr::duplicateMatch(other))
    return FALSE;

  Tuple &o = (Tuple &) other;

  if (NOT (tupleExpr_ == o.tupleExpr_))
    return FALSE;

  if (NOT (id_ == o.id_))
    return FALSE;

  return TRUE;
}

RelExpr * Tuple::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Tuple *result;

  if (derivedNode == NULL)
    result = new (outHeap) Tuple(NULL,outHeap);
  else
    result = (Tuple *) derivedNode;

  if (tupleExprTree_ != NULL)
    result->tupleExprTree_ = tupleExprTree_->copyTree(outHeap)->castToItemExpr();

  result->tupleExpr_ = tupleExpr_;

  result->id_ = id_;

  return RelExpr::copyTopNode(result, outHeap);
}

void Tuple::addTupleExprTree(ItemExpr *tupleExpr)
{
  ExprValueId t = tupleExprTree_;

  ItemExprTreeAsList(&t, ITM_ITEM_LIST).insert(tupleExpr);
  tupleExprTree_ = t.getPtr();
}

ItemExpr * Tuple::removeTupleExprTree()
{
  ItemExpr * result = tupleExprTree_;

  tupleExprTree_ = NULL;

  return result;
}

void Tuple::addLocalExpr(LIST(ExprNode *) &xlist,
			 LIST(NAString) &llist) const
{
  if (tupleExprTree_ != NULL OR
      NOT tupleExpr_.isEmpty())
    {
      if(tupleExpr_.isEmpty())
	xlist.insert(tupleExprTree_);
      else
	xlist.insert(tupleExpr_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("tuple_expr");
    }

  RelExpr::addLocalExpr(xlist,llist);
}

const NAString Tuple::getText() const
{
  NAString tmp("values ", CmpCommon::statementHeap());
  if (tupleExprTree()) tupleExprTree()->unparse(tmp);
  else                 ((ValueIdList &)tupleExpr()).unparse(tmp);
  return tmp;
}

// -----------------------------------------------------------------------
// methods for class TupleList
// -----------------------------------------------------------------------
TupleList::TupleList(const TupleList & other) : Tuple(other)
{
  castToList_ = other.castToList_;
  flags_ = other.flags_;
}

RelExpr * TupleList::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  TupleList *result;

  if (derivedNode == NULL)
    result = new (outHeap) TupleList(NULL,outHeap);
  else
    result = (TupleList *) derivedNode;

  result->castToList() = castToList();

  return Tuple::copyTopNode(result, outHeap);
}

void TupleList::addLocalExpr(LIST(ExprNode *) &xlist,
			 LIST(NAString) &llist) const
{
  Tuple::addLocalExpr(xlist,llist);
}

const NAString TupleList::getText() const
{
  NAString tmp("TupleList",CmpCommon::statementHeap());

  return tmp;
}

// -----------------------------------------------------------------------
// member functions for class PhysicalTuple
// -----------------------------------------------------------------------

NABoolean PhysicalTuple::isLogical()  const { return FALSE; }

NABoolean PhysicalTuple::isPhysical() const { return TRUE; }

// -----------------------------------------------------------------------
// Member functions for class FirstN
// -----------------------------------------------------------------------

RelExpr * FirstN::copyTopNode(RelExpr *derivedNode,
			      CollHeap* outHeap)
{
  FirstN *result;

  if (derivedNode == NULL) {
    result = new (outHeap) FirstN(NULL, getFirstNRows(), isFirstN(), getFirstNRowsParam(),
                                  outHeap);
    result->setCanExecuteInDp2(canExecuteInDp2());
  }
  else
    result = (FirstN *) derivedNode;

  result->reqdOrder().insert(reqdOrder());

  return RelExpr::copyTopNode(result, outHeap);
}

const NAString FirstN::getText() const
{
  NAString result(CmpCommon::statementHeap());

  result = "FirstN";

  return result;
}

// helper method to determine if we have a child Fisrt N node that can execute in Dp2.
// This method will only search nodes with one child. We do not expect the child First N
// to occur below a join or union type node. This method will unwind as soon as the first
// FirstN child is found.
static NABoolean haveChildFirstNInDp2 (RelExpr * node)
{
  if (node->getArity() != 1) return FALSE;
  if (node->child(0))
  {
    if (node->child(0)->getOperatorType() == REL_FIRST_N)
    {  // child is FirstN
      FirstN * innerFirstN = (FirstN *) node->child(0)->castToRelExpr();
	if (innerFirstN->canExecuteInDp2())
	  return TRUE;
	else
	  return FALSE;
    }
    else
    {  // have child but it is not FirstN
      return (haveChildFirstNInDp2(node->child(0)));
    }
  }
  else
    return FALSE;  // no child even though arity is 1!
}

RelExpr * FirstN::bindNode(BindWA *bindWA)
{
  if (nodeIsBound())
    {
      bindWA->getCurrentScope()->setRETDesc(getRETDesc());
      return this;
    }

  if (bindWA->isEmbeddedIUDStatement() && haveChildFirstNInDp2(this))
    {
      setCanExecuteInDp2(TRUE);
    }

  return RelExpr::bindNode(bindWA);
}

NABoolean FirstN::computeRowsAffected() const
{
    if (child(0))
    {
      return child(0)->castToRelExpr()->computeRowsAffected();
    }
    return FALSE;
}
// member functions for class Filter
// -----------------------------------------------------------------------

Filter::~Filter() {}

Int32 Filter::getArity() const { return 1; }

HashValue Filter::topHash()
{
  HashValue result = RelExpr::topHash();

  return result;
}

NABoolean Filter::duplicateMatch(const RelExpr & other) const
{
  return RelExpr::duplicateMatch(other);
}

RelExpr * Filter::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Filter *result;

  if (derivedNode == NULL)
    result = new (outHeap) Filter(NULL, outHeap);
  else
    result = (Filter *) derivedNode;

  return RelExpr::copyTopNode(result, outHeap);
}

void Filter::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  outputValues += child(0)->getGroupAttr()->getCharacteristicOutputs();
} // Filter::getPotentialOutputValues()

const NAString Filter::getText() const { return "filter"; }

// -----------------------------------------------------------------------
// member functions for class Rename
// -----------------------------------------------------------------------

Rename::~Rename() {}

Int32 Rename::getArity() const { return 1; }

HashValue Rename::topHash()
{
  ABORT("Hash functions can't be called in the parser");
  return 0x0;
}

NABoolean Rename::duplicateMatch(const RelExpr & /* other */) const
{
  ABORT("Duplicate match doesn't work in the parser");
  return FALSE;
}

// -----------------------------------------------------------------------
// member functions for class RenameTable
// -----------------------------------------------------------------------

RenameTable::~RenameTable() {}

const NAString RenameTable::getText() const
{
  return ("rename_as " + newTableName_.getCorrNameAsString());
}

RelExpr * RenameTable::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RenameTable *result;

  if (derivedNode == NULL)
    result = new (outHeap) RenameTable(TRUE, NULL, newTableName_, NULL, outHeap);
  else
    result = (RenameTable *) derivedNode;

  if (newColNamesTree_ != NULL)
    result->newColNamesTree_ = newColNamesTree_->copyTree(outHeap)->castToItemExpr();

  if (viewNATable_ != NULL)
    result->viewNATable_ = viewNATable_;

  return RelExpr::copyTopNode(result, outHeap);
}

ItemExpr * RenameTable::removeColNameTree()
{
  ItemExpr * result = newColNamesTree_;

  newColNamesTree_ = NULL;

  return result;
}

void RenameTable::addLocalExpr(LIST(ExprNode *) &xlist,
			       LIST(NAString) &llist) const
{
  xlist.insert(newColNamesTree_);
  llist.insert("new_col_names");

  RelExpr::addLocalExpr(xlist,llist);
}

// -----------------------------------------------------------------------
// member functions for class RenameReference
// -----------------------------------------------------------------------

RenameReference::~RenameReference() {}

const NAString RenameReference::getText() const
{
  NAString text("rename_ref");
  for (CollIndex i=0; i<tableReferences_.entries(); i++)
	  text += " " + tableReferences_[i].getRefName();
  return text;
}

RelExpr * RenameReference::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RenameReference *result;

  if (derivedNode == NULL)
  {
    TableRefList *tableRef = new(outHeap) TableRefList(tableReferences_);
    result = new (outHeap) RenameReference(NULL, *tableRef, outHeap);
  }
  else
    result = (RenameReference *) derivedNode;

  return RelExpr::copyTopNode(result, outHeap);
}

void RenameReference::addLocalExpr(LIST(ExprNode *) &xlist,
			       LIST(NAString) &llist) const
{
  RelExpr::addLocalExpr(xlist,llist);
}

// -----------------------------------------------------------------------
// member functions for class BeforeTrigger
// -----------------------------------------------------------------------

BeforeTrigger::~BeforeTrigger() {}

Int32 BeforeTrigger::getArity() const
{
	if (child(0) == NULL)
		return 0;
	else
		return 1;
}

const NAString BeforeTrigger::getText() const
{
  NAString text("before_trigger: ");

  if (signal_)
	text += "Signal";
  else
	text += "Set";

  return text;
}

RelExpr * BeforeTrigger::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  BeforeTrigger *result;

  if (derivedNode == NULL)
  {
	  TableRefList *tableRef   = new(outHeap) TableRefList(tableReferences_);
	  ItemExpr     *whenClause = NULL;
	  if (whenClause_ != NULL)
		  whenClause = whenClause_->copyTree(outHeap);

	  if (isSignal_)
	  {
		RaiseError *signalClause = (RaiseError *)signal_->copyTree(outHeap);
		result = new (outHeap) BeforeTrigger(*tableRef, whenClause, signalClause, outHeap);
	  }
	  else
	  {
		CMPASSERT(setList_ != NULL);  // Must have either SET or SIGNAL clause.
		ItemExprList *setList = new(outHeap) ItemExprList(setList_->entries(), outHeap);
		for (CollIndex i=0; i<setList_->entries(); i++)
		  setList->insert(setList_->at(i)->copyTree(outHeap));

		result = new (outHeap) BeforeTrigger(*tableRef, whenClause, setList, outHeap);
	  }
  }
  else
    result = (BeforeTrigger *) derivedNode;

  return RelExpr::copyTopNode(result, outHeap);
}

void BeforeTrigger::addLocalExpr(LIST(ExprNode *) &xlist,
			       LIST(NAString) &llist) const
{
  if (whenClause_ != NULL)
  {
	llist.insert("WHEN clause");
	xlist.insert(whenClause_);
  }

  if (signal_)
  {
	llist.insert("SIGNAL clause");
	xlist.insert(signal_);
  }
  else
  {
	for (CollIndex i=0; i<setList_->entries(); i++)
	{
		llist.insert("SET clause");
		xlist.insert(setList_->at(i));
	}
  }

  RelExpr::addLocalExpr(xlist,llist);
}


// -----------------------------------------------------------------------
// member functions for class MapValueIds
// -----------------------------------------------------------------------

MapValueIds::~MapValueIds()
{
}

Int32 MapValueIds::getArity() const { return 1; }

void MapValueIds::pushdownCoveredExpr(
     const ValueIdSet & outputExpr,
     const ValueIdSet & newExternalInputs,
     ValueIdSet & predicatesOnParent,
     const ValueIdSet * setOfValuesReqdByParent,
     Lng32 childIndex)
{
  // ---------------------------------------------------------------------
  // Since the MapValueIds node rewrites predicates, the characteristic
  // outputs of the node usually make no sense to the child node. For
  // this reason, translate the characteristic outputs of this node before
  // passing them down to the children.
  // ---------------------------------------------------------------------
  ValueIdSet requiredValues;
  if (setOfValuesReqdByParent)
    requiredValues = *setOfValuesReqdByParent;
  ValueIdSet translatedOutputs;
  ValueIdSet predsRewrittenForChild;
  ValueIdSet outputValues(outputExpr);

  // first subtract the outputs from the required values, then add back
  // the translated outputs
  outputValues -= getGroupAttr()->getCharacteristicOutputs();
  getMap().rewriteValueIdSetDown(getGroupAttr()->getCharacteristicOutputs(),
				 translatedOutputs);
  outputValues += translatedOutputs;

  translatedOutputs.clear();
  requiredValues -= getGroupAttr()->getCharacteristicOutputs();
  getMap().rewriteValueIdSetDown(getGroupAttr()->getCharacteristicOutputs(),
				 translatedOutputs);
  requiredValues += translatedOutputs;

  if (cseRef_)
    {
      // If this MapValueIds node represents a common subexpression,
      // then don't try to push predicates again that already have
      // been pushed down before. VEGPredicates may not be pushable
      // at all to the rewritten child, and other predicates might
      // be duplicated with different ValueIds for the internal
      // operators such as "=", "+", ">".
      predicatesOnParent -= cseRef_->getPushedPredicates();

      // Also, don't push down VEGPredicates on columns that are
      // characteristic outputs of the MapValueIds. Those predicates
      // (or their equivalents) should have already been pushed down.
      for (ValueId g=predicatesOnParent.init();
           predicatesOnParent.next(g);
           predicatesOnParent.advance(g))
        if (g.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
          {
            VEG *veg =
              static_cast<VEGPredicate *>(g.getItemExpr())->getVEG();
            ValueId vegRef(veg->getVEGReference()->getValueId());

            if (newExternalInputs.contains(vegRef))
              {
                // We are trying to push down a VEGPred and we are at
                // the same time offering the VEGRef as a new
                // characteristic input.  Example: We want to push
                // VEGPred_2(a=b) down and we offer its corresponding
                // VEGRef_1(a,b) as an input. The map in our
                // MapValueIds node maps VEGRef_1(a,b) on the top to
                // VEGRef_99(x,y) on the bottom. Note that either one
                // of the VEGies might contain a constant that the
                // other one doesn't contain. Note also that the
                // MapValueIds node doesn't map characteristic inputs,
                // those are passed unchanged to the child. So, we need
                // to generate a predicate for the child that:
                // a) represents the semantics of VEGPred_2(a,b)
                // b) compares the new characteristic input
                //    VEGRef_1(a,b) with some value in the child
                // c) doesn't change the members of the existing
                //    VEGies.
                // The best solution is probably an equals predicate
                // between the characteristic input and the VEGRef
                // (or other expression) that is the child's equivalent.
                // Example:  VEGRef_1(a,b) = VEGRef_99(x,y)
                ValueId bottomVal;
                ItemExpr *eqPred = NULL;

                map_.mapValueIdDown(vegRef, bottomVal);

                if (vegRef != bottomVal && bottomVal != NULL_VALUE_ID)
                  {
                    eqPred = new(CmpCommon::statementHeap()) BiRelat(
                         ITM_EQUAL,
                         vegRef.getItemExpr(),
                         bottomVal.getItemExpr(),
                         veg->getSpecialNulls());
                    eqPred->synthTypeAndValueId();
                    // replace g with the new equals predicate
                    // when we do the actual rewrite below
                    map_.addMapEntry(g, eqPred->getValueId());
                  }
              }
            else
              {
                // Don't push down VEGPredicates on columns that are
                // characteristic outputs of the MapValueIds. Those
                // predicates (or their equivalents) should have
                // already been pushed down.
                ValueIdSet vegMembers(veg->getAllValues());

                vegMembers += vegRef;
                vegMembers.intersectSet(
                     getGroupAttr()->getCharacteristicOutputs());
                if (!vegMembers.isEmpty())
                  {
                    // a VEGPred on one of my characteristic outputs,

                    // assume that my child tree already has the
                    // associated VEGPreds and remove this predicate
                    // silently
                    predicatesOnParent -= g;
                  }
                // else leave the predicate and let the code below deal
                // with it, this will probably end up in a failed assert
                // below for predsRewrittenForChild.isEmpty()
              }
          }
    }

  // rewrite the predicates so they can be applied in the child node
  getMap().rewriteValueIdSetDown(predicatesOnParent,predsRewrittenForChild);

  // use the standard method with the new required values
  RelExpr::pushdownCoveredExpr(outputValues,
                               newExternalInputs,
                               predsRewrittenForChild,
			       &requiredValues,
                               childIndex);

  // eliminate any VEGPredicates that got duplicated in the parent
  if (NOT child(0)->isCutOp())
    predsRewrittenForChild -= child(0)->selectionPred();

  // all predicates must have been pushed down!!
  CMPASSERT(predsRewrittenForChild.isEmpty());
  predicatesOnParent.clear();

  // Remove entries from the map that are no longer required since
  // they no longer appear in the outputs.
  NABoolean matchWithTopValues = FALSE;
  if (child(0)->isCutOp() || 
      child(0)->getOperatorType() == REL_FILTER ||
      child(0)->getOperatorType() == REL_MAP_VALUEIDS )
  {
    matchWithTopValues = TRUE;
    getMap().removeUnusedEntries(getGroupAttr()->getCharacteristicOutputs(), matchWithTopValues);
  }
  else
  {
    getMap().removeUnusedEntries(child(0)->getGroupAttr()->getCharacteristicOutputs(), matchWithTopValues);
  }

} // MapValueIds::pushdownCoveredExpr

void MapValueIds::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  //
  // The output of the MapValueId is given by the ValueIds
  // contained in the "upper" portion of a two-tiered mapping
  // table.
  //
  outputValues.insertList((getMap2()).getTopValues());
} // MapValueIds::getPotentialOutputValues()

void MapValueIds::addSameMapEntries(const ValueIdSet & newTopBottomValues)
{
  for (ValueId x = newTopBottomValues.init();
	    newTopBottomValues.next(x);
	    newTopBottomValues.advance(x))
    addMapEntry(x,x);
}

void MapValueIds::addLocalExpr(LIST(ExprNode *) &xlist,
			       LIST(NAString) &llist) const
{
  xlist.insert(map_.getTopValues().rebuildExprTree(ITM_ITEM_LIST));
  llist.insert("upper_values");
  xlist.insert(map_.getBottomValues().rebuildExprTree(ITM_ITEM_LIST));
  llist.insert("lower_values");

  RelExpr::addLocalExpr(xlist,llist);
}

HashValue MapValueIds::topHash()
{
  HashValue result = RelExpr::topHash();

  result ^= map_.getTopValues();
  result ^= map_.getBottomValues();

  return result;
}

NABoolean MapValueIds::duplicateMatch(const RelExpr & other) const
{
  if (NOT RelExpr::duplicateMatch(other))
    return FALSE;

  MapValueIds &o = (MapValueIds &) other;

  if (includesFavoriteMV_ != o.includesFavoriteMV_)
    return FALSE;

  if (cseRef_ != o.cseRef_)
    return FALSE;

  if (map_ != o.map_)
    return FALSE;

  return TRUE;
}

RelExpr * MapValueIds::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  MapValueIds *result;

  if (derivedNode == NULL)
    result = new (outHeap) MapValueIds(NULL,map_,outHeap);
  else
    result = static_cast<MapValueIds*>(derivedNode);

  result->includesFavoriteMV_ = includesFavoriteMV_;
  result->cseRef_ = cseRef_;

  return RelExpr::copyTopNode(result, outHeap);
}

const NAString MapValueIds::getText() const
{
    return "map_value_ids";
//    return "expr";
}

// -----------------------------------------------------------------------
// Member functions for class PhysicalMapValueIds
// -----------------------------------------------------------------------

NABoolean PhysicalMapValueIds::isLogical() const  { return FALSE; }

NABoolean PhysicalMapValueIds::isPhysical() const { return TRUE; }


// -----------------------------------------------------------------------
// Member functions for class Describe
// -----------------------------------------------------------------------

RelExpr * Describe::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Describe *result;

  if (derivedNode == NULL)
    result = new (outHeap)
                 Describe(originalQuery_, getDescribedTableName(), format_, labelAnsiNameSpace_);
  else
    result = (Describe *) derivedNode;

  return RelExpr::copyTopNode(result, outHeap);
}

const NAString Describe::getText() const
{
  return "describe";
}

// -----------------------------------------------------------------------
// Member functions for class ProbeCache
// -----------------------------------------------------------------------

NABoolean ProbeCache::isLogical()  const { return FALSE; }

NABoolean ProbeCache::isPhysical() const { return TRUE; }

Int32 ProbeCache::getArity() const { return 1; }

RelExpr * ProbeCache::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  ProbeCache *result;

  if (derivedNode == NULL)
    result = new (outHeap)
                 ProbeCache(NULL, numCachedProbes_, outHeap);
  else
    result = (ProbeCache *) derivedNode;

  result->numCachedProbes_ = numCachedProbes_;
  result->numInnerTuples_  = numInnerTuples_;

  return RelExpr::copyTopNode(result, outHeap);
}

const NAString ProbeCache::getText() const
{
  return "probe_cache";
}

// -----------------------------------------------------------------------
// Member functions for class ControlRunningQuery
// -----------------------------------------------------------------------

NABoolean ControlRunningQuery::isLogical()  const { return TRUE; }

NABoolean ControlRunningQuery::isPhysical() const { return TRUE; }

Int32 ControlRunningQuery::getArity() const { return 0; }

RelExpr * ControlRunningQuery::copyTopNode(RelExpr *derivedNode, 
                                           CollHeap* outHeap)
{
  ControlRunningQuery *result = NULL;
  
  if (derivedNode == NULL)
  {
    switch (qs_)
    {
      case ControlNidPid:
        result = new (outHeap) ControlRunningQuery(nid_, pid_, 
                                ControlNidPid, action_, forced_, outHeap);
        break;
      case ControlPname:
        result = new (outHeap) 
              ControlRunningQuery(pname_, ControlPname, action_, forced_, 
                                  outHeap);
        break;
      case ControlQid:
        result = new (outHeap)
              ControlRunningQuery(queryId_, ControlQid, action_, forced_,
                                  outHeap);
        break;
      default:
        CMPASSERT(0);
    }
    result->setComment(comment_);
  }
  else
    result = (ControlRunningQuery *) derivedNode;
  
  return RelExpr::copyTopNode(result, outHeap);
}

const NAString ControlRunningQuery::getText() const
{
  return "control_running_query";
}

void ControlRunningQuery::setComment(NAString &comment)
{
  NAString cancelComment (comment, CmpCommon::statementHeap());
  comment_ = cancelComment;
}

// -----------------------------------------------------------------------
// member functions for class CSEInfo (helper for CommonSubExprRef)
// -----------------------------------------------------------------------

Int32 CSEInfo::getTotalNumRefs(Int32 restrictToSingleConsumer) const
{
  // shortcut for main query
  if (cseId_ == CmpStatement::getCSEIdForMainQuery())
    return 1;

  // Calculate how many times we will evaluate this common subexpression
  // at runtime:
  //  - If the CSE is shared then we evaluate it once
  //  - Otherwise, look at the consumers that are lexical refs
  //    (avoid double-counting refs from multiple copies of a parent)
  //    and add the times they are being executed.
  Int32 result = 0;
  NABoolean sharedConsumers = FALSE;
  LIST(CountedCSEInfo) countsByCSE(CmpCommon::statementHeap());
  CollIndex minc = 0;
  CollIndex maxc = consumers_.entries();

  if (restrictToSingleConsumer >= 0)
    {
      // count only the executions resulting from a single consumer
      minc = restrictToSingleConsumer;
      maxc = minc + 1;
    }

  // loop over all consumers or look at just one
  for (CollIndex c=minc; c<maxc; c++)
    {
      if (isShared(c))
        {
          sharedConsumers = TRUE;
        }
      else
        {
          CommonSubExprRef *consumer = getConsumer(c);
          CSEInfo *parentInfo = CmpCommon::statement()->getCSEInfoById(
               consumer->getParentCSEId());
          NABoolean duplicateDueToSharedConsumer = FALSE;

          // Don't double-count consumers that originate from the
          // same parent CSE, have the same lexical ref number from
          // the parent, and are shared.
          if (parentInfo->isShared(
                   consumer->getParentConsumerId()))
            {
              for (CollIndex d=0; d<countsByCSE.entries(); d++)
                if (countsByCSE[d].getInfo() == parentInfo &&
                    countsByCSE[d].getLexicalCount() ==
                    consumer->getLexicalRefNumFromParent())
                  {
                    duplicateDueToSharedConsumer = TRUE;
                    break;
                  }

              // First consumer from this parent CSE with this lexical
              // ref number, remember that we are going to count
              // it. Note that we are use the lexical ref "count" in
              // CountedCSEInfo as the lexical ref number in this
              // method, so the name "count" means "ref number" in
              // this context.
              if (!duplicateDueToSharedConsumer)
                countsByCSE.insert(
                     CountedCSEInfo(
                          parentInfo,
                          consumer->getLexicalRefNumFromParent()));
            } // parent consumer is shared

          if (!duplicateDueToSharedConsumer)
            {
              // recursively determine number of times the parent of
              // this consumer gets executed
              result += parentInfo->getTotalNumRefs(
                   consumer->getParentConsumerId());
            }
        } // consumer is not shared
    } // loop over consumer(s)

  if (sharedConsumers)
    // all the shared consumers are handled by evaluating the CSE once
    result++;

  return result;
}

CSEInfo::CSEAnalysisOutcome CSEInfo::getAnalysisOutcome(Int32 id) const
{
  if (idOfAnalyzingConsumer_ != id &&
      analysisOutcome_ == CREATE_TEMP)
    // only the analyzing consumer creates and reads the temp, the
    // others only read it
    return TEMP;
  else
    return analysisOutcome_;
}

Int32 CSEInfo::addChildCSE(CSEInfo *childInfo, NABoolean addLexicalRef)
{
  Int32 result = -1;
  CollIndex foundIndex = NULL_COLL_INDEX;

  // look for an existing entry
  for (CollIndex i=0;
       i<childCSEs_.entries() && foundIndex == NULL_COLL_INDEX;
       i++)
    if (childCSEs_[i].getInfo() == childInfo)
      foundIndex = i;

  if (foundIndex == NULL_COLL_INDEX)
    {
      // create a new entry
      foundIndex = childCSEs_.entries();
      childCSEs_.insert(CountedCSEInfo(childInfo));
    }           

  if (addLexicalRef)
    {
      // The return value for a lexical ref is the count of lexical
      // refs for this particular parent/child CSE relationship so far
      // (0 if this is the first one). Note that we can't say anything
      // abount counts for expanded refs at this time, those will be
      // handled later, during the transform phase of the normalizer.
      result = childCSEs_[foundIndex].getLexicalCount();
      childCSEs_[foundIndex].incrementLexicalCount();
    }

  return result;
}

void CSEInfo::addCSERef(CommonSubExprRef *cse)
{
  CMPASSERT(name_ == cse->getName());
  cse->setId(consumers_.entries());
  consumers_.insert(cse);
  if (cse->isALexicalRef())
    numLexicalRefs_++;
}

void CSEInfo::replaceConsumerWithAnAlternative(CommonSubExprRef *c)
{
  Int32 idToReplace = c->getId();

  if (consumers_[idToReplace] != c)
    {
      CollIndex foundPos = alternativeConsumers_.index(c);

      CMPASSERT(foundPos != NULL_COLL_INDEX);
      CMPASSERT(consumers_[idToReplace]->getOriginalRef() ==
                c->getOriginalRef());
      // c moves from the list of copies to the real list and another
      // consumer moves the opposite way
      alternativeConsumers_.removeAt(foundPos);
      alternativeConsumers_.insert(consumers_[idToReplace]);
      consumers_[idToReplace] = c;
    }
}

// -----------------------------------------------------------------------
// member functions for class CommonSubExprRef
// -----------------------------------------------------------------------

NABoolean CommonSubExprRef::isAChildOfTheMainQuery() const
{
  return (parentCSEId_ == CmpStatement::getCSEIdForMainQuery());
}

CommonSubExprRef::~CommonSubExprRef()
{
}

Int32 CommonSubExprRef::getArity() const
{
  // always return 1 for now, that may change in the future
  return 1;
}

void CommonSubExprRef::addToCmpStatement(NABoolean lexicalRef)
{
  NABoolean alreadySeen = TRUE;

  // look up whether a CSE with this name already exists
  CSEInfo *info = CmpCommon::statement()->getCSEInfo(internalName_);

  // sanity check, make sure that the caller knows whether this is a
  // lexical ref (generated with CommonSubExprRef constructor) or an
  // expanded ref (generated with CommonSubExprRef::copyTopNode()
  // before the bind phase)
  CMPASSERT(isALexicalRef() == lexicalRef);

  if (!info)
    {
      // make a new object to hold a list of all references
      // to this CSE (the first one of them will be "this")
      info = new(CmpCommon::statementHeap())
        CSEInfo(internalName_,
                CmpCommon::statementHeap());
      alreadySeen = FALSE;
    }

  info->addCSERef(this);

  if (!alreadySeen)
    CmpCommon::statement()->addCSEInfo(info);
}

void CommonSubExprRef::addParentRef(CommonSubExprRef *parentRef)
{
  // Establish the parent/child relationship between two
  // CommonSubExprRef nodes, or between the main query and a
  // CommonSubExprRef node (parentRef == NULL). Also, record
  // the dependency between parent and child CSEs in the lexical
  // dependency multigraph. Bookkeeping to do:
  //
  // - Add the child's CSE to the list of child CSEs of the
  //   parent CSE
  // - Set the data members in the child ref that point to the
  //   parent CSE and parent ref

  // parent info is for a parent CSE or for the main query
  CSEInfo *parentInfo =
    (parentRef ? CmpCommon::statement()->getCSEInfo(parentRef->getName())
               : CmpCommon::statement()->getCSEInfoForMainQuery());
  CSEInfo *childInfo =
    CmpCommon::statement()->getCSEInfo(getName());

  CMPASSERT(parentInfo && childInfo);

  // Update the lexical CSE multigraph, and also return the
  // count of previously existing edges in the graph.
  // LexicalRefNumFromParent is set to a positive value only for
  // lexical refs, the other refs will be handled later, in the SQO
  // phase. This is since expanded refs may be processed before their
  // lexical ref and we may not know this value yet.
  lexicalRefNumFromParent_ =
    parentInfo->addChildCSE(childInfo, isALexicalRef());

  parentCSEId_ = parentInfo->getCSEId();
  if (parentRef)
    parentRefId_ = parentRef->getId();
  else
    parentRefId_ = -1;  // main query does not have a CommonSubExprRef
}

NABoolean CommonSubExprRef::isFirstReference() const
{
  return (CmpCommon::statement()->getCSEInfo(internalName_) == NULL);
}

void CommonSubExprRef::addLocalExpr(LIST(ExprNode *) &xlist,
                                    LIST(NAString) &llist) const
{
  if (NOT columnList_.isEmpty())
    {
      xlist.insert(columnList_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("column_list");
    }

  if(NOT pushedPredicates_.isEmpty())
  {
    xlist.insert(pushedPredicates_.rebuildExprTree());
    llist.insert("pushed_predicates");
  }
}

HashValue CommonSubExprRef::topHash()
{
  HashValue result = RelExpr::topHash();

  result ^= internalName_;
  result ^= id_;
  result ^= columnList_;
  result ^= pushedPredicates_;

  return result;
}

NABoolean CommonSubExprRef::duplicateMatch(const RelExpr & other) const
{
  if (NOT RelExpr::duplicateMatch(other))
    return FALSE;

  const CommonSubExprRef &o = static_cast<const CommonSubExprRef &>(other);

  return (internalName_ == o.internalName_ &&
          id_ == o.id_ &&
          columnList_ == o.columnList_ &&
          pushedPredicates_ == o.pushedPredicates_);
}

RelExpr * CommonSubExprRef::copyTopNode(RelExpr *derivedNode,
                                        CollHeap* outHeap)
{
  CommonSubExprRef *result = NULL;

  if (derivedNode == NULL)
    result = new (outHeap) CommonSubExprRef(NULL,
                                            internalName_.data(),
                                            outHeap);
  else
    result = static_cast<CommonSubExprRef *>(derivedNode);

  // copy fields that are common for bound and unbound nodes
  result->hbAccessOptionsFromCTE_ = hbAccessOptionsFromCTE_;

  if (nodeIsBound())
    {
      // if the node is bound, we assume that the copy is serving the same function
      // as the original, as an alternative, create an "alternative ref"
      result->setId(id_);

      result->parentCSEId_ = parentCSEId_;
      result->parentRefId_ = parentRefId_;
      result->lexicalRefNumFromParent_ = lexicalRefNumFromParent_;
      result->columnList_ = columnList_;
      result->nonVEGColumns_ = nonVEGColumns_;
      result->commonInputs_ = commonInputs_;
      result->pushedPredicates_ = pushedPredicates_;
      result->nonSharedPredicates_ = nonSharedPredicates_;
      result->cseEstLogProps_ = cseEstLogProps_;
      // don't copy the tempScan_

      // Mark this as an alternative ref, not that this does not
      // change the status of lexical vs. expanded ref
      result->isAnExpansionOf_ = isAnExpansionOf_;
      result->isAnAlternativeOf_ =
        ( isAnAlternativeOf_ ? isAnAlternativeOf_ : this);

      CmpCommon::statement()->getCSEInfo(internalName_)->
        registerAnAlternativeConsumer(result);
    }
  else
    {
      // If the node is not bound, we assume that we created an
      // "expanded" reference to a common subexpression in a tree that
      // itself is a reference to a CTE (Common Table Expression).
      // See the comment in RelMisc.h for method isAnExpandedRef()
      // to explain the term "expanded" ref.
      result->isAnExpansionOf_ =
        (isAnExpansionOf_ ? isAnExpansionOf_ : this);
      result->addToCmpStatement(FALSE);
    }

  return result;
}

const NAString CommonSubExprRef::getText() const
{
  NAString result("cse ");
  char buf[20];

  result += ToAnsiIdentifier(internalName_);

  snprintf(buf, sizeof(buf), " %d", id_);
  result += buf;

  return result;
}

Union * CommonSubExprRef::makeUnion(RelExpr *lc,
                                    RelExpr *rc,
                                    NABoolean blocked)
{
  // Make a regular or blocked union with no characteristic outputs
  Union *result;
  ValueIdSet newInputs(lc->getGroupAttr()->getCharacteristicInputs());

  result = new(CmpCommon::statementHeap()) Union(lc, rc);

  newInputs += rc->getGroupAttr()->getCharacteristicInputs();

  result->setGroupAttr(new (CmpCommon::statementHeap()) GroupAttributes());
  result->getGroupAttr()->addCharacteristicInputs(newInputs);

  if(blocked)
    result->setBlockedUnion();

  return result;
}

void CommonSubExprRef::display()
{
  if (isAChildOfTheMainQuery())
    printf("Parent: main query, lexical ref %d\n",
           lexicalRefNumFromParent_);
  else
    printf("Parent: %s(consumer %d, lexical ref %d)\n",
           CmpCommon::statement()->getCSEInfoById(
                parentCSEId_)->getName().data(),
           parentRefId_,
           lexicalRefNumFromParent_);
  printf("Original columns:\n");
  columnList_.display();
  printf("\nCommon inputs:\n");
  commonInputs_.display();
  printf("\nPushed predicates:\n");
  pushedPredicates_.display();
  printf("\nNon shared preds to be applied to scan:\n");
  nonSharedPredicates_.display();
  printf("\nPotential values for VEG rewrite:\n");
  nonVEGColumns_.display();
}

void CommonSubExprRef::displayAll(const char *optionalId)
{
  const LIST(CSEInfo *) *cses = CmpCommon::statement()->getCSEInfoList();

  if (cses)
    for (CollIndex i=0; i<cses->entries(); i++)
      if (!optionalId ||
          strlen(optionalId) == 0 ||
          cses->at(i)->getName() == optionalId)
        {
          CSEInfo *info = cses->at(i);
          CollIndex nc = info->getNumConsumers();
          NABoolean isMainQuery =
            (info->getCSEId() == CmpStatement::getCSEIdForMainQuery());

          if (isMainQuery)
            {
              printf("\n\n==========================\n");
              printf("MainQuery:\n");
            }
          else
            {
              printf("\n\n==========================\n");
              printf("CSE: %s (%d consumers, %d lexical ref(s), %d total execution(s))\n",
                     info->getName().data(),
                     nc,
                     info->getNumLexicalRefs(),
                     info->getTotalNumRefs());
            }

          const LIST(CountedCSEInfo) &children(info->getChildCSEs());

          for (CollIndex j=0; j<children.entries(); j++)
            printf("       references CSE: %s %d times\n",
                   children[j].getInfo()->getName().data(),
                   children[j].getLexicalCount());

          if (info->getIdOfAnalyzingConsumer() >= 0)
            {
              const char *outcome = "?";
              ValueIdList cols;
              CommonSubExprRef *consumer =
                info->getConsumer(info->getIdOfAnalyzingConsumer());
              const ValueIdList &cCols(consumer->getColumnList());

              switch (info->getAnalysisOutcome(0))
                {
                case CSEInfo::UNKNOWN_ANALYSIS:
                  outcome = "UNKNOWN";
                  break;
                case CSEInfo::EXPAND:
                  outcome = "EXPAND";
                  break;
                case CSEInfo::CREATE_TEMP:
                  outcome = "CREATE_TEMP";
                  break;
                case CSEInfo::TEMP:
                  outcome = "TEMP";
                  break;
                case CSEInfo::ERROR:
                  outcome = "ERROR";
                  break;
                default:
                  outcome = "???";
                  break;
                }

              printf("  analyzed by consumer %d, outcome: %s\n",
                     info->getIdOfAnalyzingConsumer(),
                     outcome);

              makeValueIdListFromBitVector(
                   cols,
                   cCols,
                   info->getNeededColumns());
              printf("\n  columns of temp table:\n");
              cols.display();
              printf("\n  commonPredicates:\n");
              info->getCommonPredicates().display();
              if (info->getVEGRefsWithDifferingConstants().entries() > 0)
                {
                  printf("\n  vegRefsWithDifferingConstants:\n");
                  info->getVEGRefsWithDifferingConstants().display();
                }
              if (info->getVEGRefsWithDifferingInputs().entries() > 0)
                {
                  printf("\n  vegRefsWithDifferingInputs:\n");
                  info->getVEGRefsWithDifferingInputs().display();
                }
              if (info->getCSETreeKeyColumns().entries() > 0)
                {
                  ValueIdList keyCols;

                  makeValueIdListFromBitVector(
                       keyCols,
                       cCols,
                       info->getCSETreeKeyColumns());
                  printf("\n  CSE key columns:\n");
                  keyCols.display();
                }
              printf("\n  DDL of temp table:\n%s\n",
                     info->getTempTableDDL().data());
            } // analyzed
          else if (info->getAnalysisOutcome(0) ==
                   CSEInfo::ELIMINATED_IN_BINDER)
            printf("  eliminated in the binder\n");
          else if (!isMainQuery)
            printf("  not yet analyzed\n");

          for (int c=0; c<nc; c++)
            {
              printf("\n\n----- Consumer %d:\n", c);
              info->getConsumer(c)->display();
            }
        } // a CSE we want to display
}

void CommonSubExprRef::makeValueIdListFromBitVector(ValueIdList &tgt,
                                                    const ValueIdList &src,
                                                    const NABitVector &vec)
{
  for (CollIndex b=0; vec.nextUsed(b); b++)
    tgt.insert(src[b]);
}


// -----------------------------------------------------------------------
// member functions for class GenericUpdate
// -----------------------------------------------------------------------


GenericUpdate::~GenericUpdate() {}

Int32 GenericUpdate::getArity() const
{
  if (getOperator().match(REL_ANY_LEAF_GEN_UPDATE))
    return 0;
  else if (getOperator().match(REL_ANY_UNARY_GEN_UPDATE))
    return 1;
  else
    ABORT("Don't know opcode in GenericUpdate::getArity()");

  return 0;    // return makes MSVC happy.
}

void GenericUpdate::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues = potentialOutputs_;
  if (producedMergeIUDIndicator_ != NULL_VALUE_ID)
    outputValues += producedMergeIUDIndicator_;
}

const NAString GenericUpdate::getUpdTableNameText() const
{
  return updatedTableName_.getTextWithSpecialType();
}

void GenericUpdate::computeUsedCols()
{
  ValueIdSet          requiredValueIds(newRecExpr_);
  ValueIdSet          coveredExprs;

  // ---------------------------------------------------------------------
  // Call the "coverTest" method, offering it all the index columns
  // as additional inputs. "coverTest" will mark those index columns that
  // it actually needs to satisfy the required value ids, and that is
  // what we actually want. The actual cover test should always succeed,
  // otherwise the update node would have been inconsistent.
  // ---------------------------------------------------------------------

  // use the clustering index, unless set otherwise
  if (indexDesc_ == NULL)
    indexDesc_ = getTableDesc()->getClusteringIndex();

  if (isMerge())
    {
      requiredValueIds.insertList(mergeInsertRecExpr_);
    }

  requiredValueIds.insertList(beginKeyPred_);
  requiredValueIds.insertList(endKeyPred_);
  requiredValueIds.insertList(getCheckConstraints());

  // QSTUFF
  requiredValueIds.insertList(newRecBeforeExpr_);
  // QSTUFF

  getGroupAttr()->coverTest(requiredValueIds,
			    indexDesc_->getIndexColumns(), // all index columns
			    coveredExprs,                  // dummy parameter
			    usedColumns_);                 // needed index cols

  // usedColumns_ is now set correctly
} // GenericUpdate::computeUsedCols


const NAString GenericUpdate::getText() const
{
  return ("GenericUpdate " + getUpdTableNameText());
}

HashValue GenericUpdate::topHash()
{
  HashValue result = RelExpr::topHash();

  result ^= newRecExpr_;

  if (isMerge())
    result ^= mergeInsertRecExpr_;

  // result ^= keyExpr_;

  return result;
}

NABoolean GenericUpdate::duplicateMatch(const RelExpr & other) const
{
  if (NOT RelExpr::duplicateMatch(other))
    return FALSE;

  GenericUpdate &o = (GenericUpdate &) other;

  if (newRecExpr_ != o.newRecExpr_ OR
      (isMerge() && 
       ((mergeInsertRecExpr_ != o.mergeInsertRecExpr_) OR
        (mergeUpdatePred_ != o.mergeUpdatePred_))       ) OR
      NOT (beginKeyPred_ == o.beginKeyPred_) OR
      NOT (endKeyPred_   == o.endKeyPred_))
    return FALSE;

  // later, replace this with the getTableDesc() ???
  if (NOT (updatedTableName_ == o.updatedTableName_))
    return FALSE;

  if (mtsStatement_ != o.mtsStatement_)
    return FALSE;

  if (noRollback_ != o.noRollback_)
    return FALSE;

  if (avoidHalloweenR2_ != o.avoidHalloweenR2_)
    return FALSE;

  if (avoidHalloween_ != o.avoidHalloween_)
    return FALSE;

  if (halloweenCannotUseDP2Locks_ != o.halloweenCannotUseDP2Locks_)
    return FALSE;

  return TRUE;
}

RelExpr * GenericUpdate::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  GenericUpdate *result;

  if (derivedNode == NULL)
    result = new (outHeap)
                 GenericUpdate(updatedTableName_,
		               getTableDesc(),
		               getOperatorType(),
			       NULL,
			       NULL, NULL,
			       outHeap);
  else
    result = (GenericUpdate *) derivedNode;

  result->setIndexDesc((IndexDesc *)getIndexDesc());

  if (newRecExprTree_)
    result->newRecExprTree_ = newRecExprTree_->copyTree(outHeap)->castToItemExpr();

  // ## Should usedColumns_ be copied here?  Is it missing deliberately or only by mistake?
  result->updateToSelectMap_	  = updateToSelectMap_;
  result->newRecExpr_   	  = newRecExpr_;
  result->newRecExprArray_ 	  = newRecExprArray_;
  // QSTUFF
  result->newRecBeforeExpr_   	  = newRecBeforeExpr_;
  result->newRecBeforeExprArray_  = newRecBeforeExprArray_;
  // QSTUFF

  result->mergeInsertRecExpr_ 	  = mergeInsertRecExpr_;
  result->mergeInsertRecExprArray_ 	  = mergeInsertRecExprArray_;
  result->mergeUpdatePred_ 	  = mergeUpdatePred_;

  result->beginKeyPred_ 	  = beginKeyPred_;
  result->endKeyPred_   	  = endKeyPred_;
  result->executorPred_ 	  = executorPred_;
  result->potentialOutputs_ 	  = potentialOutputs_;
  result->indexNewRecExprArrays_  = indexNewRecExprArrays_;
  result->indexBeginKeyPredArray_ = indexBeginKeyPredArray_;
  result->indexEndKeyPredArray_   = indexEndKeyPredArray_;
  result->indexNumberArray_ 	  = indexNumberArray_;
  result->scanIndexDesc_          = scanIndexDesc_;
  result->accessOptions_ 	  = accessOptions_;
  result->checkConstraints_ 	  = checkConstraints_;
  result->rowsAffected_ 	  = rowsAffected_;
  result->setOptStoi(stoi_);
  result->setNoFlow(noFlow_);
  result->setMtsStatement(mtsStatement_);
  result->setNoRollbackOperation(noRollback_);
  result->setAvoidHalloweenR2(avoidHalloweenR2_);
  result->avoidHalloween_         = avoidHalloween_;
  result->halloweenCannotUseDP2Locks_ = halloweenCannotUseDP2Locks_;
  result->setIsMergeUpdate(isMergeUpdate_);
  result->setIsMergeDelete(isMergeDelete_);
  result->subqInUpdateAssign_ = subqInUpdateAssign_;
  result->setUpdateCKorUniqueIndexKey(updateCKorUniqueIndexKey_);
  result->hbaseOper() = hbaseOper();
  result->uniqueHbaseOper() = uniqueHbaseOper();
  result->cursorHbaseOper() = cursorHbaseOper();
  result->uniqueRowsetHbaseOper() = uniqueRowsetHbaseOper();
  result->canDoCheckAndUpdel() = canDoCheckAndUpdel();
  result->setNoCheck(noCheck());
  result->noDTMxn() = noDTMxn();
  result->useMVCC() = useMVCC();
  result->useSSCC() = useSSCC();

  if (currOfCursorName())
    result->currOfCursorName_ = currOfCursorName()->copyTree(outHeap)->castToItemExpr();

  if (preconditionTree_)
    result->preconditionTree_ = preconditionTree_->copyTree(outHeap)->castToItemExpr();
  result->setPrecondition(precondition_);
  result->exprsInDerivedClasses_ = exprsInDerivedClasses_;
  result->producedMergeIUDIndicator_ = producedMergeIUDIndicator_;
  result->referencedMergeIUDIndicator_ = referencedMergeIUDIndicator_;

  return RelExpr::copyTopNode(result, outHeap);
}

PlanPriority GenericUpdate::computeOperatorPriority
(const Context* context,
 PlanWorkSpace *pws,
 Lng32 planNumber)
{

  PlanPriority result;

  NABoolean interactiveAccess =
    (CmpCommon::getDefault(INTERACTIVE_ACCESS) == DF_ON) OR
    ( QueryAnalysis::Instance() AND
      QueryAnalysis::Instance()->optimizeForFirstNRows());

  return result;
}

void GenericUpdate::addNewRecExprTree(ItemExpr *expr)
{
  ExprValueId newRec = newRecExprTree_;
  ItemExprTreeAsList(&newRec, ITM_AND).insert(expr);
  newRecExprTree_ = newRec.getPtr();
}


ItemExpr * GenericUpdate::removeNewRecExprTree()
{
  ItemExpr * result = newRecExprTree_;

  newRecExprTree_ = NULL;

  return result;
}

void GenericUpdate::addLocalExpr(LIST(ExprNode *) &xlist,
				 LIST(NAString) &llist) const
{

  if (newRecExprTree_ != NULL OR
      NOT newRecExpr_.isEmpty())
    {
      if (newRecExpr_.isEmpty())
	xlist.insert(newRecExprTree_);
      else
	xlist.insert(newRecExpr_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("new_rec_expr");
    }

  if ((isMerge()) &&
      (NOT mergeInsertRecExpr_.isEmpty()))
    {
      xlist.insert(mergeInsertRecExpr_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("merge_insert_rec_expr");
    }

  if ((isMerge()) &&
      (NOT mergeUpdatePred_.isEmpty()))
    {
      xlist.insert(mergeUpdatePred_.rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("merge_update_where_pred");
    }

  Int32 indexNo = 0;
  for(; indexNo < (Int32)indexNewRecExprArrays_.entries(); indexNo++) {
    ValueIdArray array = indexNewRecExprArrays_[indexNo];
    ValueIdList list;
    for(Int32 i = 0; i < (Int32)array.entries(); i++)
      list.insert(array[i]);
    xlist.insert(list.rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("new idx rec expr");
  }

  if (executorPredTree_ != NULL OR
      NOT executorPred_.isEmpty())
    {
      if (executorPred_.isEmpty())
	xlist.insert(executorPredTree_);
      else
	xlist.insert(executorPred_.rebuildExprTree());
      llist.insert("predicate");
    }

  // display preds from search key only if begin/end keys are
  // not generated yet (e.g. during optimization)
  if (beginKeyPred_.isEmpty() AND endKeyPred_.isEmpty() AND
      pathKeys_ AND NOT pathKeys_->getKeyPredicates().isEmpty())
    {
      xlist.insert(pathKeys_->getKeyPredicates().rebuildExprTree());
      if (pathKeys_ == partKeys_)
	llist.insert("key_and_part_key_preds");
      else
	llist.insert("key_predicates");
    }

  // display part key preds only if different from clustering key preds
  if (partKeys_ AND pathKeys_ != partKeys_ AND
      NOT partKeys_->getKeyPredicates().isEmpty())
    {
      xlist.insert(partKeys_->getKeyPredicates().rebuildExprTree());
      llist.insert("part_key_predicates");
    }

  if (NOT beginKeyPred_.isEmpty())
    {
      xlist.insert(beginKeyPred_.rebuildExprTree(ITM_AND));
      llist.insert("begin_key");
    }

  for(indexNo = 0; indexNo < (Int32)indexBeginKeyPredArray_.entries(); indexNo++){
    if(NOT indexBeginKeyPredArray_[indexNo].isEmpty()) {
      xlist.insert(indexBeginKeyPredArray_[indexNo]
		   .rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("index_begin_key");
    }
  }

  if (NOT endKeyPred_.isEmpty())
    {
      xlist.insert(endKeyPred_.rebuildExprTree(ITM_AND));
      llist.insert("end_key");
    }

  for(indexNo = 0; indexNo < (Int32)indexEndKeyPredArray_.entries(); indexNo++) {
    if(NOT indexEndKeyPredArray_[indexNo].isEmpty()) {
      xlist.insert(indexEndKeyPredArray_[indexNo]
		   .rebuildExprTree(ITM_ITEM_LIST));
      llist.insert("index_end_key");
    }
  }

  if (NOT getCheckConstraints().isEmpty())
    {
      xlist.insert(getCheckConstraints().rebuildExprTree(ITM_AND));
      llist.insert("check_constraint");
    }

  if (preconditionTree_ != NULL OR
      precondition_.entries() > 0)
    {
      if (preconditionTree_ != NULL)
        xlist.insert(preconditionTree_);
      else
        xlist.insert(precondition_.rebuildExprTree(ITM_AND));
      llist.insert("precondition");
    }

  RelExpr::addLocalExpr(xlist,llist);
}

NABoolean GenericUpdate::updateCurrentOf()
{
  return currOfCursorName() != NULL
	   #ifndef NDEBUG
	     || getenv("FORCE_UPD_CURR_OF")
	   #endif
	   ;
}

//++MV - returns the GenericUpdateOutputFunction's that are in the
// potential outputs

NABoolean GenericUpdate::getOutputFunctionsForMV(ValueId &valueId,
                                                 OperatorTypeEnum opType) const
{
  const ValueIdSet& outputs = getGroupAttr()->getCharacteristicOutputs();
  for (ValueId vid= outputs.init();
       outputs.next(vid);
       outputs.advance(vid) )
  {
    ItemExpr *expr = vid.getItemExpr();

    if (expr->getOperatorType() == ITM_CAST)
      expr = expr->child(0);

    if (expr->getOperator().match(opType) && 
        expr->isAGenericUpdateOutputFunction() )
    {
      valueId = vid;
      return TRUE;
    }
  }

  return FALSE;
}

NABoolean GenericUpdate::computeRowsAffected() const
{
  if (rowsAffected_ == GenericUpdate::COMPUTE_ROWSAFFECTED)
    return TRUE;
  else
    return FALSE;
};

void GenericUpdate::configTSJforHalloween( Join* tsj, OperatorTypeEnum opType,
                                           CostScalar inputCardinality)
{
  if (avoidHalloween())
  {
    // If we use DP2's FELOCKSELF (i.e., DP2Locks) method to
    // protect against Halloween, then lock escalation will
    // be disabled in the Generator.  So DP2 wants us to use
    // no more than 25000 locks per volume.
    // Also, notice that
    // by design, we are relying on good cardinality estimates.
    // If the estimates are too low, then there may be
    // runtime errors.

    const PartitioningFunction *partFunc = getTableDesc()->
      getClusteringIndex()->getPartitioningFunction();
    const Lng32 numParts = partFunc ? partFunc->getCountOfPartitions() :
                          1;
    const Lng32 maxLocksAllParts = 25000 * numParts;

    if ((opType == REL_LEAF_INSERT) &&
        (inputCardinality < maxLocksAllParts)
             &&
         ! getHalloweenCannotUseDP2Locks() &&
         (CmpCommon::getDefault(BLOCK_TO_PREVENT_HALLOWEEN) != DF_ON)
       )
      tsj->setHalloweenForceSort(Join::NOT_FORCED);
    else
      tsj->setHalloweenForceSort(Join::FORCED);
  }
}

void GenericUpdate::pushdownCoveredExpr(const ValueIdSet &outputExpr,
				   const ValueIdSet &newExternalInputs,
				   ValueIdSet &predicatesOnParent,
				   const ValueIdSet *setOfValuesReqdByParent,
				    Lng32 childIndex
				  )
{

  // ---------------------------------------------------------------------
  // determine the set of local expressions that need to be evaluated
  // - assign expressions (reference source & target cols)
  // - source cols alone (in case order is required)
  // - characteristic outputs for this node
  // ---------------------------------------------------------------------
  // QSTUFF ?? again need to understand details 
  ValueIdSet localExprs(newRecExpr());

  if (setOfValuesReqdByParent)
    localExprs += *setOfValuesReqdByParent;

  // QSTUFF
  localExprs.insertList(newRecBeforeExpr());
  // QSTUFF

 
  if (isMerge())
    {
      localExprs.insertList(mergeInsertRecExpr());
    }

  localExprs.insertList(beginKeyPred());

  localExprs.insertList(updateToSelectMap().getBottomValues());
  if (setOfValuesReqdByParent)
    localExprs += *setOfValuesReqdByParent ;
  localExprs += exprsInDerivedClasses_;

  // ---------------------------------------------------------------------
  // Check which expressions can be evaluated by my child.
  // Modify the Group Attributes of those children who inherit some of
  // these expressions.
  // Since an GenericUpdate has no predicates, supply an empty set.
  // ---------------------------------------------------------------------
 RelExpr::pushdownCoveredExpr(	outputExpr,
				newExternalInputs,
				predicatesOnParent,
				&localExprs);

/*to fix jira 18-20180111-2901  
 *For query " insert into to t1 select seqnum(seq1, next) from t1;", there is no SORT as left child of TSJ, and it 
 *is a self-referencing updates Halloween problem. In NestedJoin::genWriteOpLeftChildSortReq(), child(0)
 *producing no outputs for this query, which means that there is no column to sort on. So we solve this by 
 *having the source for Halloween insert produce at least one output column always.
 * */
  if (avoidHalloween() && child(0) &&
      child(0)->getOperatorType() == REL_SCAN &&
      child(0)->getGroupAttr())
    {
      if (child(0)->getGroupAttr()->getCharacteristicOutputs().isEmpty())
        {
          ValueId exprId;
          ValueId atLeastOne;

          ValueIdSet output_source = child(0)->getTableDescForExpr()->getColumnList();
          for (exprId = output_source.init();
               output_source.next(exprId);
               output_source.advance(exprId))
            {
              atLeastOne = exprId;
              if (!(exprId.getItemExpr()->doesExprEvaluateToConstant(FALSE, TRUE)))
                {
                  child(0)->getGroupAttr()->addCharacteristicOutputs(exprId);
                  break;
                }
            }
         if (child(0)->getGroupAttr()->getCharacteristicOutputs().isEmpty())
           {
             child(0)->getGroupAttr()->addCharacteristicOutputs(atLeastOne);
           }
        }
    }	
}

/*
NABoolean Insert::reconcileGroupAttr(GroupAttributes *newGroupAttr)
{
  SET(IndexDesc *) x;
  const IndexDesc* y = getTableDesc()->getClusteringIndex();
  x.insert((IndexDesc*)y);

  newGroupAttr->addToAvailableBtreeIndexes(x);

  // Now as usual
  return RelExpr::reconcileGroupAttr(newGroupAttr);
}
*/

// -----------------------------------------------------------------------
// member functions for class Insert
// -----------------------------------------------------------------------
Insert::Insert(const CorrName &name,
        TableDesc *tabId,
        OperatorTypeEnum otype,
        RelExpr *child ,
        ItemExpr *insertCols ,
        ItemExpr *orderBy ,
        CollHeap *oHeap ,
        InsertType insertType,
        NABoolean createUstatSample)
 : GenericUpdate(name,tabId,otype,child,NULL,NULL,oHeap),
   insertColTree_(insertCols),
   orderByTree_(orderBy),
   targetUserColPosList_(NULL),
   bufferedInsertsAllowed_(FALSE),
   insertType_(insertType),
   noBeginSTInsert_(FALSE),
   noCommitSTInsert_(FALSE),
   enableTransformToSTI_(FALSE),
   enableAqrWnrEmpty_(FALSE),
   systemGeneratesIdentityValue_(FALSE),
   insertSelectQuery_(FALSE),
   boundView_(NULL),
   overwriteHiveTable_(FALSE),
   isSequenceFile_(FALSE),
   isUpsert_(FALSE),
   isTrafLoadPrep_(FALSE),
   createUstatSample_(createUstatSample),
   xformedEffUpsert_(FALSE),
   baseColRefs_(NULL)
{
  insert_a_tuple_ = FALSE;
  if ( child ) {
    if ( child->getOperatorType() == REL_TUPLE ) {
      insert_a_tuple_ = TRUE;
      if (!name.isLocationNameSpecified()) {
        setCacheableNode(CmpMain::PARSE);
      }
    }
    else if ( child->getOperatorType() == REL_TUPLE_LIST &&
              !name.isLocationNameSpecified() ) {
      setCacheableNode(CmpMain::PARSE);
    }
  }
  else
	// this is a patch to pass regression for maximum parallelism project,
	// if we insert a default values not a real tuple the child is NULL
	// but we'd like to identify is as a tuple insert. March,2006
	if (CmpCommon::getDefault(COMP_BOOL_66) == DF_OFF)
	{
       insert_a_tuple_ = TRUE;
	}
}

Insert::~Insert() {}

void Insert::addInsertColTree(ItemExpr *expr)
{
  ExprValueId newCol = insertColTree_;
  ItemExprTreeAsList(&newCol, ITM_AND).insert(expr);
  insertColTree_ = newCol.getPtr();
}

ItemExpr * Insert::removeInsertColTree()
{
  ItemExpr * result = insertColTree_;

  insertColTree_ = NULL;

  return result;
}

ItemExpr * Insert::getInsertColTree()
{
  return insertColTree_;
}

const NAString Insert::getText() const
{
  NAString text("insert",CmpCommon::statementHeap());

  return (text + " " + getUpdTableNameText());
}

RelExpr * Insert::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Insert *result;

  if (derivedNode == NULL)
    result = new (outHeap) Insert(getTableName(),
				  getTableDesc(),
				  getOperatorType(),
				  NULL,
				  NULL,
				  NULL,
				  outHeap,
				  getInsertType());
  else
    result = (Insert *) derivedNode;

  result->rrKeyExpr() = rrKeyExpr();
  result->partNumInput() = partNumInput();
  result->rowPosInput() = rowPosInput();
  result->totalNumPartsInput() = totalNumPartsInput();
  result->reqdOrder()  = reqdOrder();

  result->noBeginSTInsert_  = noBeginSTInsert_;
  result->noCommitSTInsert_ = noCommitSTInsert_;
  result->enableTransformToSTI() = enableTransformToSTI();
  result->enableAqrWnrEmpty()    = enableAqrWnrEmpty();

  if (insertColTree_ != NULL)
    result->insertColTree_ = insertColTree_->copyTree(outHeap)->castToItemExpr();

  result->insertATuple() = insertATuple();
  result->setInsertSelectQuery(isInsertSelectQuery());
  result->setOverwriteHiveTable(getOverwriteHiveTable());
  result->setSequenceFile(isSequenceFile());
  result->isUpsert_ = isUpsert_;
  result->isTrafLoadPrep_ = isTrafLoadPrep_;
  result->createUstatSample_ = createUstatSample_;
  result->xformedEffUpsert_ = xformedEffUpsert_;
  return GenericUpdate::copyTopNode(result, outHeap);
}

void Insert::setNoBeginCommitSTInsert(NABoolean noBeginSTI, NABoolean noCommitSTI)
{
  noBeginSTInsert_ = noBeginSTI;
  noCommitSTInsert_ = noCommitSTI;
}

// -----------------------------------------------------------------------
// member functions for class Update
// -----------------------------------------------------------------------
Update::Update(const CorrName &name,
               TableDesc *tabId,
               OperatorTypeEnum otype,
               RelExpr *child,
               ItemExpr *newRecExpr,
               ItemExpr *currOfCursorName,
               CollHeap *oHeap)
     : GenericUpdate(name,tabId,otype,child,newRecExpr,currOfCursorName,oHeap),
       estRowsAccessed_(0)
{
  setCacheableNode(CmpMain::BIND);
}

Update::~Update() {}

const NAString Update::getText() const
{
  NAString text("update",CmpCommon::statementHeap());

  return (text + " " + getUpdTableNameText());
}

RelExpr * Update::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Update *result;

  if (derivedNode == NULL)
    result = new (outHeap) Update(getTableName(),
				  getTableDesc(),
				  getOperatorType(),
				  NULL,
				  NULL, NULL,
				  outHeap);
  else
    result = (Update *) derivedNode;

  result->setEstRowsAccessed(getEstRowsAccessed());

  return GenericUpdate::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class MergeUpdate
// -----------------------------------------------------------------------
MergeUpdate::MergeUpdate(const CorrName &name,
			 TableDesc *tabId,
			 OperatorTypeEnum otype,
			 RelExpr *child,
			 ItemExpr *setExpr,
			 ItemExpr *insertCols,
			 ItemExpr *insertValues,
			 CollHeap *oHeap,
			 ItemExpr *where)
     : Update(name,tabId,otype,child,setExpr,NULL,oHeap),
       insertCols_(insertCols), insertValues_(insertValues),
       where_(where), xformedUpsert_(FALSE), needsBindScope_(TRUE)
{
  setCacheableNode(CmpMain::BIND);
  
  setIsMergeUpdate(TRUE);

  // if there is a WHERE NOT MATCHED INSERT action, then the scan
  // has to take place in the merge node at run time, so we have
  // to suppress the TSJ transformation on this node
  if (insertValues)
    setNoFlow(TRUE);
}

MergeUpdate::~MergeUpdate() {}

const NAString MergeUpdate::getText() const
{
  NAString text("merge_update",CmpCommon::statementHeap());

  return (text + " " + getUpdTableNameText());
}

RelExpr * MergeUpdate::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  MergeUpdate *result;

  if (derivedNode == NULL)
    result = new (outHeap) MergeUpdate(getTableName(),
				       getTableDesc(),
				       getOperatorType(),
				       child(0),
				       NULL,
				       insertCols(), insertValues(),
				       outHeap, where_);
  else
    result = (MergeUpdate *) derivedNode;
  if (xformedUpsert())
    result->setXformedUpsert();

  return Update::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class Delete
// -----------------------------------------------------------------------

Delete::Delete(const CorrName &name, TableDesc *tabId, OperatorTypeEnum otype,
               RelExpr *child, ItemExpr *newRecExpr,
               ItemExpr *currOfCursorName, 
	       ConstStringList * csl,
	       CollHeap *oHeap)
  : GenericUpdate(name,tabId,otype,child,newRecExpr,currOfCursorName,oHeap),
    csl_(csl),estRowsAccessed_(0)
{
  setCacheableNode(CmpMain::BIND);
}

Delete::~Delete() {}

const NAString Delete::getText() const
{
  NAString text("delete",CmpCommon::statementHeap());

  return (text + " " + getUpdTableNameText());
}

RelExpr * Delete::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  Delete *result;

  if (derivedNode == NULL)
    result = new (outHeap) Delete(getTableName(),
				  getTableDesc(),
				  getOperatorType(),
				  NULL,
				  NULL, NULL,
				  csl_,
				  outHeap);
  else
    result = (Delete *) derivedNode;

  result->csl() = csl();
  result->setEstRowsAccessed(getEstRowsAccessed());

  return GenericUpdate::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class MergeDelete
// -----------------------------------------------------------------------
MergeDelete::MergeDelete(const CorrName &name,
			 TableDesc *tabId,
			 OperatorTypeEnum otype,
			 RelExpr *child,
			 ItemExpr *insertCols,
			 ItemExpr *insertValues,
			 CollHeap *oHeap)
  : Delete(name,tabId,otype,child,NULL,NULL,NULL,oHeap),
       insertCols_(insertCols), insertValues_(insertValues)
{
  setCacheableNode(CmpMain::BIND);
  
  setIsMergeDelete(TRUE);

  // if there is a WHERE NOT MATCHED INSERT action, then the scan
  // has to take place in the merge node at run time, so we have
  // to suppress the TSJ transformation on this node
  if (insertValues)
    setNoFlow(TRUE);
}

MergeDelete::~MergeDelete() {}

const NAString MergeDelete::getText() const
{
  NAString text("merge_delete",CmpCommon::statementHeap());

  return (text + " " + getUpdTableNameText());
}

RelExpr * MergeDelete::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  MergeDelete *result;

  if (derivedNode == NULL)
    result = new (outHeap) MergeDelete(getTableName(),
				       getTableDesc(),
				       getOperatorType(),
				       child(0),
				       insertCols(), insertValues(),
				       outHeap);
  else
    result = (MergeDelete *) derivedNode;

  return Delete::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class InsertCursor
// -----------------------------------------------------------------------

InsertCursor::~InsertCursor() {}

NABoolean InsertCursor::isLogical() const { return FALSE; }

NABoolean InsertCursor::isPhysical() const { return TRUE; }

const NAString InsertCursor::getText() const
{
  NAString text("insert",  CmpCommon::statementHeap());

  if ((insertType_ == VSBB_INSERT_SYSTEM) ||
      (insertType_ == VSBB_INSERT_USER))
    text = text + "_vsbb";
  else if ((insertType_ == VSBB_LOAD) ||
	   (insertType_ == VSBB_LOAD_APPEND) ||
	   (insertType_ == VSBB_LOAD_NO_DUP_KEY_CHECK) ||
	   (insertType_ == VSBB_LOAD_APPEND_NO_DUP_KEY_CHECK))
    text = text + "_sidetree";
  else if (insertType_ == VSBB_LOAD_AUDITED)
    text = text + "_sidetree_audited";

  text = text + " (physical)";

  return (text + " " + getUpdTableNameText());
}

RelExpr * InsertCursor::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) InsertCursor(getTableName(),
				        getTableDesc(),
				        getOperatorType(),
					NULL,
					outHeap);
  else
    result = derivedNode;

  return Insert::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class HiveInsert
// -----------------------------------------------------------------------
const NAString HiveInsert::getText() const
{
  NAString text("hive_insert",  CmpCommon::statementHeap());

  text += " (physical)";

  return (text + " " + getUpdTableNameText());
}

RelExpr * HiveInsert::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HiveInsert(getTableName(),
				        getTableDesc(),
				        getOperatorType(),
					NULL,
					outHeap);
  else
    result = derivedNode;

  return Insert::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class HbaseInsert
// -----------------------------------------------------------------------
const NAString HbaseInsert::getText() const
{
  NABoolean isSeabase = 
    (getTableDesc() && getTableDesc()->getNATable() ? 
     getTableDesc()->getNATable()->isSeabaseTable() : FALSE);

  NAString text;

  if (NOT isSeabase)
    text = "hbase_";
  else
    text = "trafodion_";

  if (isUpsert())
    {
      if (getInsertType() == Insert::UPSERT_LOAD)
        {
          if (getIsTrafLoadPrep())
            text += "load_preparation";
          else
            text += "load";
        }
      else if (vsbbInsert())
	text += "vsbb_upsert";
      else
        text += "upsert";
    }
  else
    {
      if (vsbbInsert())
	text += "vsbb_upsert";
      else
        text += "insert";
    }
  
  return (text + " " + getUpdTableNameText());
}

RelExpr * HbaseInsert::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  HbaseInsert *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseInsert(getTableName(),
				        getTableDesc(),
				        getOperatorType(),
					NULL,
					outHeap);
  else
    result = (HbaseInsert *) derivedNode;
  
  result->returnRow_ = returnRow_;

  return Insert::copyTopNode(result, outHeap);
}

RelExpr * HBaseBulkLoadPrep::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) HbaseInsert(getTableName(),
                                        getTableDesc(),
                                        getOperatorType(),
                                        NULL,
                                        outHeap);
  else
    result = derivedNode;

  return Insert::copyTopNode(result, outHeap);
}
// -----------------------------------------------------------------------
// member functions for class UpdateCursor
// -----------------------------------------------------------------------

UpdateCursor::~UpdateCursor() {}

NABoolean UpdateCursor::isLogical() const { return FALSE; }

NABoolean UpdateCursor::isPhysical() const { return TRUE; }

const NAString UpdateCursor::getText() const
{
  NAString text("cursor_update",CmpCommon::statementHeap());

  return (text + " " + getUpdTableNameText());
}


RelExpr * UpdateCursor::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) UpdateCursor(getTableName(),
				        getTableDesc(),
				        getOperatorType(),
					NULL,
					outHeap);
  else
    result = derivedNode;

  return Update::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// member functions for class DeleteCursor
// -----------------------------------------------------------------------

DeleteCursor::~DeleteCursor() {}

NABoolean DeleteCursor::isLogical() const { return FALSE; }

NABoolean DeleteCursor::isPhysical() const { return TRUE; }

const NAString DeleteCursor::getText() const
{
  NAString text("cursor_delete",CmpCommon::statementHeap());

  return (text + " " + getUpdTableNameText());
}

RelExpr * DeleteCursor::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelExpr *result;

  if (derivedNode == NULL)
    result = new (outHeap) DeleteCursor(getTableName(),
				        getTableDesc(),
				        getOperatorType(),
					NULL,
					outHeap);
  else
    result = derivedNode;

  return Delete::copyTopNode(result, outHeap);
}


/////////////////////////////////////////////////////////////////////

void
RelExpr::unparse(NAString &result,
		 PhaseEnum /* phase */,
		 UnparseFormatEnum /* form */,
		 TableDesc * tabId) const
{
  result += getText();

#ifndef NDEBUG
  if (getenv("UNPARSE_FULL"))
    {
      if (selection_)
	{
	  result += "[";
	  selection_->unparse(result /*, phase, form */);
	  result += "]";
	}
      if (predicates_.entries())
	{
	  result += "{";
	  predicates_.unparse(result /*, phase, form */);
	  result += "}";
	}
    }
#endif

  Int32 maxi = getArity();
  if (maxi)
    {
      result += "(";
      for (Lng32 i = 0; i < maxi; i++)
	{
	  if (i > 0)
	    result += ", ";
          if ( child(i).getPtr() == NULL )
            continue;
	  child(i)->unparse(result);
	}
      result += ")";
    }
}

// -----------------------------------------------------------------------
// methods for class Transpose
// -----------------------------------------------------------------------

// Transpose::~Transpose() -----------------------------------------------
// The destructor
//
Transpose::~Transpose()
{
}

// Transpose::topHash() --------------------------------------------------
// Compute a hash value for a chain of derived RelExpr nodes.
// Used by the Cascade engine as a quick way to determine if
// two nodes are identical.
// Can produce false positives (nodes appear to be identical),
// but should not produce false negatives (nodes are definitely different)
//
// Inputs: none (other than 'this')
//
// Outputs: A HashValue of this node and all nodes in the
// derivation chain below (towards the base class) this node.
//
HashValue Transpose::topHash()
{
  // Compute a hash value of the derivation chain below this node.
  //
  HashValue result = RelExpr::topHash();

  // transUnionVector is the only relevant
  // data members at this point. The other data members do not
  // live past the binder.
  //
  for(CollIndex i = 0; i < transUnionVectorSize(); i++) {
    result ^= transUnionVector()[i];
  }

  return result;
}

// Transpose::duplicateMatch()
// A more thorough method to compare two RelExpr nodes.
// Used by the Cascades engine when the topHash() of two
// nodes returns the same hash values.
//
// Inputs: other - a reference to another node of the same type.
//
// Outputs: NABoolean - TRUE if this node is 'identical' to the
//          'other' node. FALSE otherwise.
//
// In order to match, this node must match all the way down the
// derivation chain to the RelExpr class.
//
// For the Transpose node, the only relevant data member which
// needs to be compared is transUnionVals_. The other data members
// do not exist passed the binder.
//
NABoolean
Transpose::duplicateMatch(const RelExpr & other) const
{
  // Compare this node with 'other' down the derivation chain.
  //
  if (!RelExpr::duplicateMatch(other))
    return FALSE;

  // Cast the RelExpr to a Transpose node. (This must be a Transpose node)
  //
  Transpose &o = (Transpose &) other;

  // If the transUnionVectors are the same size and have the same entries,
  // then the nodes are identical
  //
  if(transUnionVectorSize() != o.transUnionVectorSize())
    return FALSE;

  for(CollIndex i = 0; i < transUnionVectorSize(); i++) {
    if (!(transUnionVector()[i] == o.transUnionVector()[i]))
      return FALSE;
  }

  return TRUE;
}

// Transpose::copyTopNode ----------------------------------------------
// Copy a chain of derived nodes (Calls RelExpr::copyTopNode).
// Needs to copy all relevant fields.
// Used by the Cascades engine.
//
// Inputs: derivedNode - If Non-NULL this should point to a node
//         which is derived from this node.  If NULL, then this
//         node is the top of the derivation chain and a node must
//         be constructed.
//
// Outputs: RelExpr * - A Copy of this node.
//
// If the 'derivedNode is non-NULL, then this method is being called
// from a copyTopNode method on a class derived from this one. If it
// is NULL, then this is the top of the derivation chain and a transpose
// node must be constructed.
//
// In either case, the relevant data members must be copied to 'derivedNode'
// and 'derivedNode' is passed to the copyTopNode method of the class
// below this one in the derivation chain (RelExpr::copyTopNode() in this
// case).
//
RelExpr *
Transpose::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  Transpose *result;

  if (derivedNode == NULL)
    // This is the top of the derivation chain
    // Create an empty Transpose node.
    //
    result = new (outHeap) Transpose(NULL,NULL,NULL,outHeap);
  else
    // A node has already been constructed as a derived class.
    //
    result = (Transpose *) derivedNode;

  // Copy the relavant fields.

  result->transUnionVectorSize_ = transUnionVectorSize();

  result->transUnionVector() =
    new (outHeap) ValueIdList[transUnionVectorSize()];

  for(CollIndex i = 0; i < transUnionVectorSize(); i++) {
    result->transUnionVector()[i] = transUnionVector()[i];
  }

  // copy pointer to expressions
  // These are not available after bindNode()
  //
  if (transValsTree_ != NULL)
    result->transValsTree_ = transValsTree_->copyTree(outHeap)->castToItemExpr();

  if (keyCol_ != NULL)
    result->keyCol_ = keyCol_->copyTree(outHeap)->castToItemExpr();


  // Copy any data members from the classes lower in the derivation chain.
  //
  return RelExpr::copyTopNode(result, outHeap);
}


// Transpose::addLocalExpr() -----------------------------------------------
// Insert into a list of expressions all the expressions of this node and
// all nodes below this node in the derivation chain. Insert into a list of
// names, all the names of the expressions of this node and all nodes below
// this node in the derivation chain. This method is used by the GUI tool
// and by the Explain Function to have a common method to get all the
// expressions associated with a node.
//
// Inputs/Outputs: xlist - a list of expressions.
//                 llist - a list of names of expressions.
//
// The xlist contains a list of all the expressions associated with this
// node. The llist contains the names of these expressions. (This lists
// must be kept in the same order).
// Transpose::addLocalExpr potentially adds the transUnionVals_ expression
// ("transpose_union_values"), the transValsTree_ expression
// ("transpose_values"), and the keyCol_ expression ("key_column").
//
// It then calls RelExpr::addLocalExpr() which will add any RelExpr
// expressions to the list.
//
void Transpose::addLocalExpr(LIST(ExprNode *) &xlist,
			     LIST(NAString) &llist) const
{

  for(CollIndex i = 0; i < transUnionVectorSize(); i++) {
    if (NOT transUnionVector()[i].isEmpty()) {
      xlist.insert(transUnionVector()[i].rebuildExprTree());
      llist.insert("transpose_union_vector");
    }
  }

  // This is only available as an ItemExpr tree.  It is never
  // stored as a ValueIdSet.  This is not available after bindNode().
  //
  if(transValsTree_) {
    xlist.insert(transValsTree_);
    llist.insert("transpose_values");
  }

  // This is only available as an ItemExpr tree.  It is never
  // stored as a ValueIdSet.  This is not available after bindNode().
  //
  if(keyCol_) {
    xlist.insert(keyCol_);
    llist.insert("key_column");
  }

  RelExpr::addLocalExpr(xlist,llist);
}

// Transpose::getPotentialOutputValues() ---------------------------------
// Construct a Set of the potential outputs of this node.
//
// Inputs: none (other than 'this')
//
// Outputs: outputValues - a ValueIdSet representing the potential outputs
//          of this node.
//
// The potential outputs for the transpose node are the new columns
// generated by the transpose node, plus the outputs produced by the
// child node.  The new columns generated by transpose are the key
// column and the value colunms (one for each transpose group).
//
void
Transpose::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  // Make sure the ValueIdSet is empty.
  //
  outputValues.clear();

  // Add the values generated by the transpose node.
  //
  for(CollIndex i = 0; i < transUnionVectorSize(); i++) {
    outputValues.insertList( transUnionVector()[i] );
  }

  // Add the values produced by the child.
  //
  outputValues += child(0).getGroupAttr()->getCharacteristicOutputs();

} // Transpose::getPotentialOutputValues()


// Transpose::pushdownCoveredExpr() ------------------------------------
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
//    assigned new predicates or is expected to compute some of the
//    expressions that are required by its parent.
//
// Parameters:
//
// const ValueIdSet &setOfValuesReqdByParent
//    IN:  a read-only reference to a set of expressions that are
//         associated with this operator. Typically, they do not
//         include the predicates.
//
// ValueIdSet & newExternalInputs
//    IN : a reference to a set of new external inputs (ValueIds)
//         that are provided by this operator for evaluating the
//         the above expressions.
//
// ValueIdSet & predicatesOnParent
//    IN : the set of predicates existing on the operator
//    OUT: a subset of the original predicates. Some of the
//         predicate factors may have been pushed down to
//         the operator's children.
//
// long childIndex
//    IN : This is an optional parameter.
//         If supplied, it is a zero-based index for a specific child
//         on which the predicate pushdown should be attempted.
//         If not supplied, or a null pointer is supplied, then
//         the pushdown is attempted on all the children.
//
// ---------------------------------------------------------------------
void Transpose::pushdownCoveredExpr(const ValueIdSet &outputExpr,
			       const ValueIdSet &newExternalInputs,
			       ValueIdSet &predicatesOnParent,
			       const ValueIdSet *setOfValuesReqdByParent,
			       Lng32 childIndex
			       )
{

  ValueIdSet exprOnParent;
  if (setOfValuesReqdByParent)
    exprOnParent = *setOfValuesReqdByParent;

  // Add all the values required for the transpose expressions
  // to the values required by the parent.
  // Don't add the valueIds of the ValueIdUnion nodes, but the
  // valueIds of the contents of the ValueIdUnion nodes.
  //
  for(CollIndex v = 0; v < transUnionVectorSize(); v++) {
    ValueIdList &valIdList = transUnionVector()[v];

    for(CollIndex i = 0; i < valIdList.entries(); i++) {
      ValueIdUnion *valIdu = ((ValueIdUnion *)valIdList[i].
			      getValueDesc()->getItemExpr());

      exprOnParent.insertList(valIdu->getSources());
    }
  }

  ValueIdSet pushablePredicates(predicatesOnParent);

  RelExpr::pushdownCoveredExpr(outputExpr,
                               newExternalInputs,
                               pushablePredicates,
			       &exprOnParent,
                               childIndex);

  predicatesOnParent.intersectSet(pushablePredicates);
} // Transpose::pushdownCoveredExpr

// Transpose::removeTransValsTree() -------------------------------------
// Return the transValsTree_ ItemExpr tree and set to NULL,
//
// Inputs: none (Other than 'this')
//
// Outputs: ItemExpr * - the value of transValsTree_
//
// Side Effects: Sets the value of transValsTree_ to NULL.
//
// Called by Transpose::bindNode(). The value of transValsTree_ is not
// needed after the binder.
//
const ItemExpr *
Transpose::removeTransValsTree()
{
  ItemExpr *result = transValsTree_;
  transValsTree_ = (ItemExpr *)NULL;
  return result;
}

// Transpose::removeKeyCol() -------------------------------------
// Return the keyCol_ ItemExpr tree and set it to NULL,
//
// Inputs: none (Other than 'this')
//
// Outputs: ItemExpr * - the value of keyCol_
//
// Side Effects: Sets the value of keyCol_ to NULL.
//
// Call by Transpose::bindNode(). The value of keyCol_ is not
// needed after the binder.
//
const ItemExpr *
Transpose::removeKeyCol()
{
  ItemExpr *result = keyCol_;
  keyCol_ = (ItemExpr *)NULL;
  return result;
}

// This method is used in case there are expressions in the Transpose column
// list. It traverses through the expression to get the column under them
// If it is a unary expression, it gets the column directly below the expression.
// If the expression has two children, it goes through both the children
// to see which one of them has a higher UEC. It returns the ValueId of that
// column. This column is later used to determine the UEC of the final
// transpose column.

ValueId Transpose::getSourceColFromExprForUec(ValueId sourceValId,
					      const ColStatDescList & childColStatsList)
{

  if (sourceValId.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
    return sourceValId;

  ValueIdSet vegCols;
  sourceValId.getItemExpr()->
    findAll(ITM_VEG_REFERENCE, vegCols, TRUE, TRUE);

  // case 1 : expression with a constant, return sourceValId
  // case 2 :(only one expr) concentrates on simple expressions only
  // case 3 :(Multiple exprs) Expression of type EXPR1 OP EXPR2
  // where EXPR1 , EXPR2 is VEGREF or EXPR we will assume the max UEC
  // admist the list of base columns found will be used.
  // This is an approximation but better that the worst case.
  if (vegCols.entries() == 0)
  {
    // case 1
    return sourceValId;
  }
  if(vegCols.entries() == 1)
  {
    // case 2
    // There is only one get that.
    vegCols.getFirst(sourceValId);
  }
  else
  {
    //case 3
    //Initialize for safety.
    vegCols.getFirst(sourceValId);
    //CostScalars are initialized by their constructor to zero.
    CostScalar currentMaxUEC,currentUEC;
    CollIndex index = NULL_COLL_INDEX;

    for(ValueId currentValId = vegCols.init()
	;vegCols.next(currentValId)
	;vegCols.advance(currentValId))
    {
       index = NULL_COLL_INDEX;
       childColStatsList.getColStatDescIndex(index, currentValId);
       if (index == NULL_COLL_INDEX) continue;

       currentUEC = childColStatsList[index]->getColStats()
		       ->getTotalUec();
       //get the UEC and find the max and corresponding valueID
       //and assign it ti sourceValId.
       if(currentUEC > currentMaxUEC)
       {
	   currentMaxUEC = currentUEC;
	   sourceValId = currentValId;
       }
    }// end of for
  }//end of elsif
  return sourceValId;
}

// The destructor
//
PhysTranspose::~PhysTranspose()
{
}

// PhysTranspose::copyTopNode ----------------------------------------------
// Copy a chain of derived nodes (Calls Transpose::copyTopNode).
// Needs to copy all relevant fields.
// Used by the Cascades engine.
//
// Inputs: derivedNode - If Non-NULL this should point to a node
//         which is derived from this node.  If NULL, then this
//         node is the top of the derivation chain and a node must
//         be constructed.
//
// Outputs: RelExpr * - A Copy of this node.
//
// If the 'derivedNode is non-NULL, then this method is being called
// from a copyTopNode method on a class derived from this one. If it
// is NULL, then this is the top of the derivation chain and a transpose
// node must be constructed.
//
// In either case, the relevant data members must be copied to 'derivedNode'
// and 'derivedNode' is passed to the copyTopNode method of the class
// below this one in the derivation chain (Transpose::copyTopNode() in this
// case).
//
RelExpr *
PhysTranspose::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  PhysTranspose *result;

  if (derivedNode == NULL)
    // This is the top of the derivation chain
    // Generate an empty PhysTranspose node.
    //
    result = new (outHeap) PhysTranspose(NULL,outHeap);
  else
    // A node has already been constructed as a derived class.
    //
    result = (PhysTranspose *) derivedNode;

  // PhysTranspose has no data members.

  // Copy any data members from the classes lower in the derivation chain.
  //
  return Transpose::copyTopNode(result, outHeap);
}

// -----------------------------------------------------------------------
// methods for class Pack
// -----------------------------------------------------------------------

// Constructor
Pack::Pack(ULng32 pf,
           RelExpr* child,
           ItemExpr* packingExprTree,
           CollHeap* oHeap)
 : RelExpr(REL_PACK,child,NULL,oHeap),
   packingFactorLong_(pf),
   packingFactorTree_(NULL),
   packingExprTree_(packingExprTree)
{
  setNonCacheable();
  packingFactor().clear();
  packingExpr().clear();
  requiredOrder_.clear();
}

// Destructor.
Pack::~Pack()
{
}

// -----------------------------------------------------------------------
// Pack:: some Accessors/Mutators.
// -----------------------------------------------------------------------

ItemExpr* Pack::removePackingFactorTree()
{
  ItemExpr* pf = packingFactorTree_;
  packingFactorTree_ = NULL;
  return pf;
}

ItemExpr* Pack::removePackingExprTree()
{
  ItemExpr* pe = packingExprTree_;
  packingExprTree_ = NULL;
  return pe;
}

// -----------------------------------------------------------------------
// Pack::getText()
// -----------------------------------------------------------------------
const NAString Pack::getText() const
{
  return "PACK";
}

// -----------------------------------------------------------------------
// Pack::topHash()
// -----------------------------------------------------------------------
HashValue Pack::topHash()
{
  // The base class's topHash deals with inputs/outputs and operator type.
  HashValue result = RelExpr::topHash();

  // Packing factor and packing expression are the differentiating factors.
  result ^= packingFactor();
  result ^= packingExpr();
  result ^= requiredOrder();

  return result;
}

// -----------------------------------------------------------------------
// Pack::duplicateMatch()
// -----------------------------------------------------------------------
NABoolean Pack::duplicateMatch(const RelExpr& other) const
{
  // Assume optimizer already matches inputs/outputs in Group Attributes.

  // Base class checks for operator type, predicates and children.
  if(NOT RelExpr::duplicateMatch(other)) return FALSE;

  // Base class implementation already makes sure other is a Pack node.
  Pack& otherPack = (Pack &) other;

  // If the required order keys are not the same
  // then the nodes are not identical
  //
  if (!(requiredOrder() == otherPack.requiredOrder()))
    return FALSE;

  // Packing factor is the only remaining thing to check.
  return (packingFactor_ == otherPack.packingFactor() AND
                                 packingExpr_ == otherPack.packingExpr());
}

// -----------------------------------------------------------------------
// Pack::copyTopNode()
// -----------------------------------------------------------------------
RelExpr* Pack::copyTopNode(RelExpr* derivedNode, CollHeap* outHeap)
{
  Pack* result;

  // This the real node we want to copy. Construct a new Pack node.
  if(derivedNode == NULL)
  {
    result = new (outHeap) Pack (packingFactorLong(),NULL,NULL,outHeap);
    result->packingFactor() = packingFactor();
    result->packingExpr() = packingExpr();
    //result->setRequiredOrder(requiredOrder());
    result->requiredOrder() = requiredOrder();
    result->setFirstNRows(getFirstNRows());
  }
  else
  // ---------------------------------------------------------------------
  // The real node we want to copy is of a derived class. The duplicate
  // has already been made and store in derived node. All I need to do is
  // to copy the members stored with this base class.
  // ---------------------------------------------------------------------
  {
    result = (Pack *) derivedNode;
    result->packingFactorLong() = packingFactorLong();
    result->packingFactor() = packingFactor();
    result->packingExpr() = packingExpr();

    result->requiredOrder() = requiredOrder();
    result->setFirstNRows(getFirstNRows());
  }

  // Call base class to make copies of its own data members.
  return RelExpr::copyTopNode(result,outHeap);
}

// -----------------------------------------------------------------------
// Pack::getPotentialOutputValues()
// -----------------------------------------------------------------------
void Pack::getPotentialOutputValues(ValueIdSet& outputValues) const
{
  // Just the outputs of the packing expression.
  outputValues.clear();
  outputValues.insertList(packingExpr_);
}

// -----------------------------------------------------------------------
// Pack::getNonPackedExpr() returns the non-packed sub-expressions of the
// packing expression.
// -----------------------------------------------------------------------
void Pack::getNonPackedExpr(ValueIdSet& vidset)
{
  for(CollIndex i = 0; i < packingExpr().entries(); i++)
  {
    ItemExpr* packItem = packingExpr().at(i).getItemExpr();
    vidset.insert(packItem->child(0)->getValueId());
  }
}

// -----------------------------------------------------------------------
// Pack::pushdownCoveredExpr() needs to add the sub-expressions of the
// packing expression to nonPredExprOnOperator and then make use of the
// default implementation. It is expected in the first phase, nothing
// can be pushed down though.
// -----------------------------------------------------------------------
void Pack::pushdownCoveredExpr(const ValueIdSet& outputExpr,
                               const ValueIdSet& newExternalInputs,
                               ValueIdSet& predOnOperator,
			       const ValueIdSet* nonPredExprOnOperator,
			       Lng32 childId)
{
  ValueIdSet exprNeededByOperator;
  getNonPackedExpr(exprNeededByOperator);
  if (nonPredExprOnOperator)
    exprNeededByOperator += *nonPredExprOnOperator;
  exprNeededByOperator.insertList(requiredOrder());

  RelExpr::pushdownCoveredExpr(outputExpr,
                               newExternalInputs,
                               predOnOperator,
			       &exprNeededByOperator,
                               childId);
}

// -----------------------------------------------------------------------
// Pack::addLocalExpr() adds the packing expressions to be displayed by
// the GUI debugger.
// -----------------------------------------------------------------------
void Pack::addLocalExpr(LIST(ExprNode*)& xlist,
                        LIST(NAString)& llist) const
{
  if(packingExprTree_ != NULL)
  {
    xlist.insert(packingExprTree_);
    llist.insert("pack_expr_tree");
  }

  if (requiredOrder().entries() > 0) {
    xlist.insert(requiredOrder().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("required_order");
  }

  if(NOT packingExpr_.isEmpty())
  {
    xlist.insert(packingExpr_.rebuildExprTree());
    llist.insert("pack_expr");
  }

  RelExpr::addLocalExpr(xlist,llist);
}

// -----------------------------------------------------------------------
// methods for class PhyPack
// -----------------------------------------------------------------------

// Destructor.
PhyPack::~PhyPack()
{
}

// -----------------------------------------------------------------------
// PhyPack::copyTopNode()
// -----------------------------------------------------------------------
RelExpr* PhyPack::copyTopNode(RelExpr* derivedNode, CollHeap* outHeap)
{
  PhyPack* result;

  // This the real node we want to copy. Construct a new PhyPack node.
  if(derivedNode == NULL)
  {
    result = new (outHeap) PhyPack (0,NULL,outHeap);
  }
  else
  // ---------------------------------------------------------------------
  // The real node we want to copy is of a derived class. The duplicate
  // has already been made and store in derived node. All I need to do is
  // to copy the members stored with this base class.
  // ---------------------------------------------------------------------
  {
    result = (PhyPack *) derivedNode;
  }

  // Tell base class to copy its members. PhyPack has no added members.
  return Pack::copyTopNode(result,outHeap);
}


// -----------------------------------------------------------------------
// methods for class Rowset
// -----------------------------------------------------------------------
// Constructor
Rowset::Rowset(ItemExpr *inputHostvars, ItemExpr *indexExpr,
               ItemExpr *sizeExpr, RelExpr * childExpr, CollHeap* oHeap)
 : RelExpr(REL_ROWSET,childExpr,NULL,oHeap),
   inputHostvars_(inputHostvars),
   indexExpr_(indexExpr),
   sizeExpr_(sizeExpr)
{
  setNonCacheable();
} // Rowset::Rowset()

// Destructor.
Rowset::~Rowset()
{
} // Rowset::~Rowset()

RelExpr * Rowset::copyTopNode(RelExpr *derivedNode,
                              CollHeap* oHeap)
{
  Rowset *result;

  if (derivedNode == NULL)
    result = new (oHeap) Rowset(inputHostvars_, indexExpr_, sizeExpr_, NULL, oHeap);
  else {
    result = (Rowset *) derivedNode;
  }

  return RelExpr::copyTopNode(result,oHeap);
} // Rowset::copyTopNode()

Int32 Rowset::getArity() const
{
  return 0; // This is a leaf node
} // Rowset::getArity()


const NAString Rowset::getText() const
{
  NAString result("RowSet",CmpCommon::statementHeap());

  if (sizeExpr_) {
    if (sizeExpr_->getOperatorType() == ITM_CONSTANT) {
      char str[TEXT_DISPLAY_LENGTH];
      sprintf(str, " " PF64,
              ((ConstValue *)sizeExpr_)->getExactNumericValue());
      result += str;
    } else if (sizeExpr_->getOperatorType() == ITM_HOSTVAR)
      result += " " + ((HostVar *)sizeExpr_)->getName();
    else
      result += " ??";
  }

  result += " (";
  for (ItemExpr *hostVarTree = inputHostvars_;
       hostVarTree != NULL;
       hostVarTree = hostVarTree->child(1)) {
    if (inputHostvars_ != hostVarTree)
      result += ", ";

    HostVar *hostVar = (HostVar *)hostVarTree->getChild(0);
    result += hostVar->getName();
  }
  result += ")";

  if (indexExpr_)
    result += ("KEY BY " +
               ((ColReference *)indexExpr_)->getColRefNameObj().getColName());

  return result;
}

// returns the name of the exposed index of the Rowset
const NAString Rowset::getIndexName() const
{
     // A hack to check if the Rowset has an index expression
     NAString result("",CmpCommon::statementHeap());

     if (indexExpr_)
       result += ((ColReference *)indexExpr_)->getColRefNameObj().getColName();
     return(result);
}

// -----------------------------------------------------------------------
// methods for class Rowset
// -----------------------------------------------------------------------
// Constructor
RowsetRowwise::RowsetRowwise(RelExpr * childExpr,
			     CollHeap* oHeap)
 : Rowset(NULL, NULL, NULL, childExpr, oHeap)
{
} // RowsetRowwise::RowsetRowwise()

RelExpr * RowsetRowwise::copyTopNode(RelExpr *derivedNode,
				     CollHeap* oHeap)
{
  Rowset *result;

  if (derivedNode == NULL)
    result = new (oHeap) RowsetRowwise(NULL, oHeap);
  else {
    result = (RowsetRowwise *) derivedNode;
  }

  return Rowset::copyTopNode(result,oHeap);
} // RowsetRowwise::copyTopNode()

const NAString RowsetRowwise::getText() const
{
  NAString result("RowSet Rowwise",CmpCommon::statementHeap());

  return result;
}

Int32 RowsetRowwise::getArity() const
{
  return 1; 
} // Rowset::getArity()

RowsetFor::RowsetFor(RelExpr  *child,
                     ItemExpr *inputSizeExpr,
                     ItemExpr *outputSizeExpr,
                     ItemExpr *indexExpr,
		     ItemExpr *maxSizeExpr,
		     ItemExpr *maxInputRowlen,
		     ItemExpr *rwrsBuffer,
		     ItemExpr *partnNum,
                     CollHeap *oHeap)
  : RelExpr(REL_ROWSETFOR,child,NULL,oHeap),
    inputSizeExpr_(inputSizeExpr),
    outputSizeExpr_(outputSizeExpr),
    indexExpr_(indexExpr),
    maxSizeExpr_(maxSizeExpr),
    maxInputRowlen_(maxInputRowlen),
    rwrsBuffer_(rwrsBuffer),
    partnNum_(partnNum),
    rowwiseRowset_(FALSE),
    packedFormat_(FALSE),
    compressed_(FALSE),
    dcompressInMaster_(FALSE),
    compressInMaster_(FALSE),
    partnNumInBuffer_(FALSE)
{
  setNonCacheable(); 
}

// Destructor.
RowsetFor::~RowsetFor()
{
}

RelExpr * RowsetFor::copyTopNode(RelExpr *derivedNode,
                                 CollHeap* oHeap)
{
  RowsetFor *result;

  if (derivedNode == NULL)
    result = new (oHeap) RowsetFor(NULL, inputSizeExpr_, outputSizeExpr_,
                                   indexExpr_, 
				   maxSizeExpr_, maxInputRowlen_,
				   rwrsBuffer_, partnNum_, oHeap);
  else
    result = (RowsetFor *) derivedNode;

  result->rowwiseRowset_ = rowwiseRowset_;
  result->setBufferAttributes(packedFormat_,
			      compressed_, dcompressInMaster_,
			      compressInMaster_, partnNumInBuffer_);

  return RelExpr::copyTopNode(result,oHeap);
}

Int32 RowsetFor::getArity() const
{
  return 1;
} // RowsetFor::getArity()

const NAString RowsetFor::getText() const
{
  NAString result("RowSetFor ", CmpCommon::statementHeap());

  if (inputSizeExpr_) {
    if (inputSizeExpr_->getOperatorType() == ITM_CONSTANT) {
      char str[TEXT_DISPLAY_LENGTH];
      sprintf(str, PF64,
              ((ConstValue *)inputSizeExpr_)->getExactNumericValue());
      result += "INPUT SIZE ";
      result += str;
    } else if (inputSizeExpr_->getOperatorType() == ITM_HOSTVAR)
      result += "INPUT SIZE " + ((HostVar *)inputSizeExpr_)->getName();
    else
      result += "INPUT SIZE ??";

    if (outputSizeExpr_ || indexExpr_)
      result += ",";
  }

  if (outputSizeExpr_) {
    if (outputSizeExpr_->getOperatorType() == ITM_CONSTANT) {
      char str[TEXT_DISPLAY_LENGTH];
      sprintf(str, PF64,
              ((ConstValue *)outputSizeExpr_)->getExactNumericValue());
      result += "OUTPUT SIZE ";
      result += str;
    } else if (outputSizeExpr_->getOperatorType() == ITM_HOSTVAR)
      result += "OUTPUT SIZE " + ((HostVar *)outputSizeExpr_)->getName();
    else
      result += "OUTPUT SIZE ??";

    if (indexExpr_)
      result += ",";

  }

  if (indexExpr_)
    result += ("KEY BY " +
               ((ColReference *)indexExpr_)->getColRefNameObj().getColName());

  return result;
}

// -----------------------------------------------------------------------
// methods for class RowsetInto
// -----------------------------------------------------------------------
// Constructor
RowsetInto::RowsetInto(RelExpr *child, ItemExpr *outputHostvars,
                       ItemExpr *sizeExpr, CollHeap* oHeap)
 : RelExpr(REL_ROWSET_INTO,child,NULL,oHeap),
   outputHostvars_(outputHostvars),
   sizeExpr_(sizeExpr),
   requiredOrderTree_(NULL)
{
  setNonCacheable();
   requiredOrder_.clear();
} // RowsetInto::RowsetInto()

// Destructor.
RowsetInto::~RowsetInto()
{
} // RowsetInto::~RowsetInto()

RelExpr * RowsetInto::copyTopNode(RelExpr *derivedNode,
                                  CollHeap* oHeap)
{
  RowsetInto *result;

  if (derivedNode == NULL)
    result = new (oHeap) RowsetInto(NULL, outputHostvars_, sizeExpr_, oHeap);
  else
    result = (RowsetInto *) derivedNode;

  return RelExpr::copyTopNode(result,oHeap);
} // RowsetInto::copyTopNode()

Int32 RowsetInto::getArity() const
{
  return 1; // This select-list root node
} // RowsetInto::getArity()


const NAString RowsetInto::getText() const
{
  NAString result("RowsetINTO",CmpCommon::statementHeap());

  if (sizeExpr_) {
    if (sizeExpr_->getOperatorType() == ITM_CONSTANT) {
      char str[TEXT_DISPLAY_LENGTH];
      sprintf(str, " " PF64 ,
              ((ConstValue *)sizeExpr_)->getExactNumericValue());
      result += str;
    } else if (sizeExpr_->getOperatorType() == ITM_HOSTVAR)
      result += " " + ((HostVar *)sizeExpr_)->getName();
    else
      result += " ??";
  }

  result += " (";
  for (ItemExpr *hostVarTree = outputHostvars_;
       hostVarTree != NULL;
       hostVarTree = hostVarTree->child(1)) {
    if (outputHostvars_ != hostVarTree)
      result += ", ";

    HostVar *hostVar = (HostVar *)hostVarTree->getChild(0);
    result += hostVar->getName();
  }
  result += ")";

  return result;
}


NABoolean RelExpr::treeContainsEspExchange()
{
  Lng32 nc = getArity();
  if (nc > 0)
  {
    if ((getOperatorType() == REL_EXCHANGE) &&
        (child(0)->castToRelExpr()->getPhysicalProperty()->getPlanExecutionLocation()
            != EXECUTE_IN_DP2))
    {
      return TRUE;
    }

    for (Lng32 i = 0; i < nc; i++)
    {
      if (child(i)->treeContainsEspExchange())
        return TRUE;
    }
  }
  return FALSE;
}

NABoolean Exchange::areProbesHashed(const ValueIdSet pkey)
{
  return getGroupAttr()->getCharacteristicInputs().contains(pkey);
}


void Exchange::computeBufferLength(const Context *myContext,
                                   const CostScalar &numConsumers,
                                   const CostScalar &numProducers,
                                   CostScalar &upMessageBufferLength,
                                   CostScalar &downMessageBufferLength)
{

  CostScalar numDownBuffers = (Int32) ActiveSchemaDB()->getDefaults().getAsULong
                              (GEN_SNDT_NUM_BUFFERS);
  CostScalar numUpBuffers = (Int32) ActiveSchemaDB()->getDefaults().getAsULong
                                 (GEN_SNDB_NUM_BUFFERS);
  CostScalar maxOutDegree = MAXOF(numConsumers, numProducers);

  CostScalar upSizeOverride = ActiveSchemaDB()->getDefaults().getAsLong
                             (GEN_SNDT_BUFFER_SIZE_UP);

  // The adjustment is a fudge factor to improve scalability by 
  // reducing the buffer size
  // "penalty" when the number of connections is high due 
  //to a high degree of parallelism.
  // The net result is to increase the memory "floor" and "ceiling"
   // (that are base d on the
   // number of connections) by up to fourfold. 
   //Too high a ceiling can cause memory pressure,
   // a high level of paging activity, etc., 
   //while too low a ceiling can cause a large
   // number of IPC messages and dispatches, and a 
   // resultant increase in path lengt h.
   // The adjustment attempts to strike a balance between 
   // the two opposing clusters  of performance factors.

  CostScalar adjMaxNumConnections =
    maxOutDegree < 32 || upSizeOverride == 1 || upSizeOverride > 2 ? maxOutDegree :
    maxOutDegree < 64 ? 32 :
    maxOutDegree < 128 ? 40 :
    maxOutDegree < 256 ? 50 : 
    maxOutDegree < 512 ? 64 : 70;

  CostScalar overhead = CostScalar(50);

  // compute numProbes, probeSize, cardinality, outputSize
  CostScalar downRecordLength = getGroupAttr()->
                   getCharacteristicInputs().getRowLength();

  CostScalar upRecordLength = getGroupAttr()->
             getCharacteristicOutputs().getRowLength();

  const CostScalar & numOfProbes =
  ( myContext->getInputLogProp()->getResultCardinality() ).minCsOne();

  // use no more than 50 KB and try to send all rows down in a single message
  CostScalar reasonableBufferSpace1 =
    CostScalar(50000) / (maxOutDegree * numDownBuffers);

  reasonableBufferSpace1 =
    MINOF(reasonableBufferSpace1,
           (downRecordLength + overhead) * numOfProbes);

  const EstLogPropSharedPtr inputLP    = myContext->getInputLogProp();
  CostScalar numRowsUp = child(0).outputLogProp(inputLP)->
                         getResultCardinality();

  const PartitioningFunction* const parentPartFunc =
    myContext->getPlan()->getPhysicalProperty()->getPartitioningFunction();

  if (parentPartFunc->isAReplicateViaBroadcastPartitioningFunction())
     numRowsUp = numRowsUp * numConsumers;

  // check for an overriding define for the buffer size
  CostScalar downSizeOverride = ActiveSchemaDB()->getDefaults().getAsLong
                                 (GEN_SNDT_BUFFER_SIZE_DOWN);

  if (downSizeOverride.isGreaterThanZero())
    reasonableBufferSpace1 = downSizeOverride;

  // we MUST be able to fit at least one row into a buffer

  CostScalar controlAppendedLength= ComTdbSendTop::minSendBufferSize(
                                     (Lng32)downRecordLength.getValue());
  downMessageBufferLength =
    MAXOF(controlAppendedLength,
          reasonableBufferSpace1);

  // Total size of output buffer that needs to be sent to the parent.
  CostScalar totalBufferSize = upRecordLength *  numRowsUp;

  // Divide this by number of connections to get total buffer per connection.
  CostScalar bufferSizePerConnection = totalBufferSize / adjMaxNumConnections; 

  // Aim for a situation where atleast 80 messages are sent per connection.
  CostScalar reasonableBufferSpace2 =
               bufferSizePerConnection / ActiveSchemaDB()
                 ->getDefaults().getAsLong(GEN_EXCHANGE_MSG_COUNT);

  // Now Esp has numUpBuffers of size reasonableBufferSpace2 per 
  // each stream (connection), so total memory to be allocated 
  // in this Esp would be:
  // reasonableBufferSpace2 * numUpBuffers * maxOutDegree.
  // We need to apply ceiling and floor to this memory i.e.:
  // 4MB > reasonableBufferSpace2 * numUpBuffers * maxOutDegree > 50KB.
  // OR divide both ceiling and floor by numUpBuffers * maxOutDegree.
  Int32 maxMemKB = ActiveSchemaDB()
                   ->getDefaults().getAsLong(GEN_EXCHANGE_MAX_MEM_IN_KB);
  if (maxMemKB <= 0) maxMemKB = 4000;   // 4MB if not set or negative
  CostScalar maxMem1 = maxMemKB * 1000;
  CostScalar maxMem2 = maxMemKB * 4000;
  CostScalar ceiling = MINOF(maxMem1 /
                             (numUpBuffers * adjMaxNumConnections),
                             maxMem2 /
                             (numUpBuffers * maxOutDegree));

  CostScalar floor   =  MINOF(CostScalar(50000) / 
                             (numUpBuffers * adjMaxNumConnections), 
                             CostScalar(200000) / 
                              (numUpBuffers * maxOutDegree));
    
  // Apply the floor.
  reasonableBufferSpace2 =
     MAXOF(floor, reasonableBufferSpace2);

  // Apply the ceiling.
  reasonableBufferSpace2 =
   MINOF(ceiling, reasonableBufferSpace2);

  // Make sure the floor is at least 5K to avoid performance problem.
  reasonableBufferSpace2 =
   MAXOF(CostScalar(5000), reasonableBufferSpace2);

  // Make sure that it is at most 31k-1356
  reasonableBufferSpace2 =
   MINOF( reasonableBufferSpace2, 31 * 1024 - 1356);
 
  if (upSizeOverride.isGreaterThanZero())
    reasonableBufferSpace2 = upSizeOverride;

  // we MUST be able to fit at least one row into a buffer
  controlAppendedLength = ComTdbSendTop::minReceiveBufferSize(
                                       (Lng32) (upRecordLength.getValue()) );
  upMessageBufferLength =
    MAXOF(  controlAppendedLength, reasonableBufferSpace2);
  
  // convert Buffers to kilo bytes
  upMessageBufferLength_= upMessageBufferLength = upMessageBufferLength/CostScalar(1024);
  downMessageBufferLength_ = downMessageBufferLength = downMessageBufferLength/ CostScalar(1024);  
} // Exchange::computeBufferLength()


//////////////////////////////////////////////////////////////////////
// Class pcgEspFragment related methods
//////////////////////////////////////////////////////////////////////

void pcgEspFragment::addChild(Exchange* esp)
{
   CollIndex newIndex = childEsps_.entries();
   childEsps_.insertAt(newIndex, esp);
}

// Verify that the newly added exchange node at the end of childEsps_[]
// is compatible with others.
// Note that preCodeGen traversal the query tree via a depth-first 
// search. Each time the leave exchange node in this fragment is encountered,
// this method is called,  The order of visit to the child exchanges 
// is from left to right.
// 
NABoolean pcgEspFragment::tryToReduceDoP()
{
   float threshold;
   ActiveSchemaDB()->
      getDefaults().getFloat(DOP_REDUCTION_ROWCOUNT_THRESHOLD, threshold);
   
   if ( threshold == 0.0 || getTotaleRows() >= threshold ) {
     return FALSE;
   }


   // Defensive programming. Nothing to verify when there is no child esp.
   if ( childEsps_.entries() == 0 ) return FALSE;


   // Get the ptr to last exchange 
   CollIndex lastIdx  = childEsps_.entries()-1;
   Exchange* xch = childEsps_[lastIdx];

   //
   // No dop reduction for Parallel Extract
   //
   if ( xch->getExtractProducerFlag() || xch->getExtractConsumerFlag() )
     return FALSE;

   PartitioningFunction* partFunc = 
           xch->getPhysicalProperty()->getPartitioningFunction();

   //
   // If xch's count of partitions is less than newDoP, bail out
   //
   Lng32 newDoP = CURRSTMT_OPTDEFAULTS->getDefaultDegreeOfParallelism();
   if ( partFunc->getCountOfPartitions() < newDoP ) 
     return FALSE;
 

   // Do not reduce dop if this exchange is a PA, except if it is
   // the right child of a TYPE2 join, using replicate-no-broadcast.
   // An extra exchange is needed otherwise to bridge the 
   // new DoP and all original partitions, unless the newDoP is a factor 
   // of #part of the hash2 partfunc for the PA node. 
   // 
   if ( xch->child(0)->getPhysicalProperty()
          ->getPlanExecutionLocation() == EXECUTE_IN_DP2)
   {
      if ( partFunc->isAHash2PartitioningFunction() ) {
         if ( partFunc->getCountOfPartitions() % newDoP != 0 )
           return FALSE;
      } else    
         if (!partFunc->isAReplicateNoBroadcastPartitioningFunction())
           return FALSE;
   }


   Lng32 suggestedNewDoP = newDoP;

   //
   // Make a copy of the part func as the scaling method can side effect.
   //
   PartitioningFunction* partFuncCopy = 
           xch->getPhysicalProperty()->getPartitioningFunction()->copy();

   PartitioningFunction* newPF = 
           partFuncCopy->scaleNumberOfPartitions(suggestedNewDoP);

   //
   // If the part func can not scale to newDoP, bail out.
   //
   if ( suggestedNewDoP != newDoP ) 
     return FALSE;

   //
   // Find a common dop for all child esps in the fragment first. 
   // A common dop is one associated with 1st esp that has non-
   // broadcasting part func. All other child esps with non-broadcasting
   // partFunc should be use the "common dop". This is true prior to
   // the dop reduction attempt. If it is already found (commonDoP_ > 0),
   // just use it.
   //
   if ( commonDoP_ == 0 && 
        partFuncCopy->isAReplicationPartitioningFunction() == FALSE )
      commonDoP_ = partFuncCopy->getCountOfPartitions();


   // If the dop at child exchange A can be reduced but not at
   // child exchange B, we may end up with in an inconsistent state. 
   // The following code detects it. 
   if ( commonDoP_ > 0 ) { 

      if ( 
           partFuncCopy->isAReplicationPartitioningFunction() == FALSE &&
           partFuncCopy->getCountOfPartitions() != commonDoP_
         )
        return FALSE;
   }

   //
   // The new dop is acceptable.
   //
   newDoP_ = newDoP;
   setValid(TRUE);

   return TRUE;
}

void pcgEspFragment::invalidate()
{
   setValid(FALSE);

   //for ( CollIndex i=0; i<childEsps_.entries(); i++ ) {
   //   childEsps_[i]->getEspFragPCG()->invalidate();
   //}
}

void pcgEspFragment::adjustDoP(Lng32 newDop)
{
  for ( CollIndex i=0; i<childEsps_.entries(); i++ ) {

    Exchange* xch = childEsps_[i];

    // Recursively adjust the dop for my child fragments.
    // Each exchange will have its own pcgEspFragment to work with.
    xch->doDopReduction();
  }
}

void RelExpr::doDopReduction()
{
  
  Int32 nc = getArity();

  for (Lng32 i = 0; i < nc; i++)
  {
     child(i)->doDopReduction();
  }
}

void Exchange::doDopReduction()
{
   //
   // Once this method is called for the top most Exchange, we can 
   // recursively call the same method for all the esp fragments via
   // the pointers (to esps) saved in the pcgEsgFragment  objects.
   //
   Lng32 newDop = getEspFragPCG()->getNewDop();

   if ( getEspFragPCG()->isValid() ) {

     // Adjust the partfunc for all nodes within the fragment, starting
     // from my child and down to every bottom-defining exchanges.
     child(0)->doDopReductionWithinFragment(newDop);
   }

   // Next recursively call the same method for all fragments below me.
   getEspFragPCG()->adjustDoP(newDop);
}

void RelExpr::doDopReductionWithinFragment(Lng32 newDoP)
{
  adjustTopPartFunc(newDoP);

  if ( getOperatorType() == REL_EXCHANGE ) {

     //
     // Need to adjust the Logical part of the part func if 
     // my child's part func is a LogPhy partfunc.
     //
     if ( child(0)->getPhysicalProperty()->getPlanExecutionLocation() 
             == EXECUTE_IN_DP2) 
     {
        PartitioningFunction *pf = 
           child(0)->getPhysicalProperty()->getPartitioningFunction();


        if ( pf->isALogPhysPartitioningFunction() ) 
           child(0)->adjustTopPartFunc(newDoP);
     }
     return;
  }

  Int32 nc = getArity();

  for (Lng32 i = 0; i < nc; i++)
  {
     child(i)->doDopReductionWithinFragment(newDoP);
  }
}


void RelExpr::adjustTopPartFunc(Lng32 newDop)
{
   ((PhysicalProperty*)getPhysicalProperty())->scaleNumberOfPartitions(newDop);
   setDopReduced(TRUE);
}

// Required Resource Estimate Methods - Begin
void RelExpr::computeRequiredResources(RequiredResources & reqResources, EstLogPropSharedPtr & inLP)
{
  Int32 nc = getArity();

  for (Lng32 i = 0; i < nc; i++)
  {
    if (child(i))
      child(i)->computeRequiredResources(reqResources, inLP);
    else
      child(i).getLogExpr()->computeRequiredResources(reqResources, inLP);
  }

  computeMyRequiredResources(reqResources, inLP);
}

void Join::computeRequiredResources(RequiredResources & reqResources, EstLogPropSharedPtr & inLP)
{
  Int32 nc = getArity();

  if (child(0))
    child(0)->computeRequiredResources(reqResources, inLP);
  else
    child(0).getLogExpr()->computeRequiredResources(reqResources, inLP);

  EstLogPropSharedPtr inputForRight = inLP;

  EstLogPropSharedPtr leftOutput =
    child(0).getGroupAttr()->outputLogProp(inLP);

  if(isTSJ())
  {
    inputForRight = leftOutput;
  }

  if (child(1))
    child(1)->computeRequiredResources(reqResources, inputForRight);
  else
    child(1).getLogExpr()->computeRequiredResources(reqResources, inputForRight);


  computeMyRequiredResources(reqResources, inLP);
}

void RequiredResources::accumulate(CostScalar memRsrcs,
                                   CostScalar cpuRsrcs,
                                   CostScalar dataAccessCost,
                                   CostScalar maxCard)
{
  memoryResources_ += memRsrcs;
  cpuResources_ += cpuRsrcs;
  dataAccessCost_ += dataAccessCost;

  if(maxOperMemReq_ < memRsrcs)
    maxOperMemReq_ = memRsrcs;

  if(maxOperCPUReq_ < cpuRsrcs)
    maxOperCPUReq_ = cpuRsrcs;

  if(maxOperDataAccessCost_ < dataAccessCost)
    maxOperDataAccessCost_ = dataAccessCost;

  if(maxMaxCardinality_ < maxCard)
    maxMaxCardinality_ = maxCard;
}

void RelExpr::computeMyRequiredResources(RequiredResources & reqResources, EstLogPropSharedPtr & inLP)
{
  CostScalar cpuResourcesRequired = csZero;
  CostScalar A = csOne;
  CostScalar B (getDefaultAsDouble(WORK_UNIT_ESP_DATA_COPY_COST));

  Int32 nc = getArity();

  for (Lng32 i = 0; i < nc; i++)
  {
    GroupAttributes * childGroupAttr = child(i).getGroupAttr();
    CostScalar childCardinality =
      childGroupAttr->outputLogProp(inLP)->getResultCardinality();
    CostScalar childRecordSize = childGroupAttr->getCharacteristicOutputs().getRowLength();

    cpuResourcesRequired +=
      (A * childCardinality) +
      (B * childCardinality * childRecordSize );
  }

  CostScalar myMaxCard = getGroupAttr()->getResultMaxCardinalityForInput(inLP);

  reqResources.accumulate(csZero,
                          cpuResourcesRequired,
                          csZero,
                          myMaxCard);
}

void RelRoot::computeMyRequiredResources(RequiredResources & reqResources, EstLogPropSharedPtr & inLP)
{

  if (hasOrderBy())
  {
    CostScalar memoryResourcesRequired = csZero;

    GroupAttributes * childGroupAttr = child(0).getGroupAttr();
    CostScalar childCardinality =
      childGroupAttr->outputLogProp(inLP)->getResultCardinality();
    CostScalar childRecordSize = childGroupAttr->getCharacteristicOutputs().getRowLength();
    memoryResourcesRequired  = (childCardinality * childRecordSize);

    reqResources.accumulate(memoryResourcesRequired, csZero, csZero);
  }

  // add the cpu resources
  RelExpr::computeMyRequiredResources(reqResources, inLP);
}

void MultiJoin::computeMyRequiredResources(RequiredResources & reqResources, EstLogPropSharedPtr & inLP)
{
  // get the subset analysis for this MultiJoin
  JBBSubsetAnalysis * subsetAnalysis = getJBBSubset().getJBBSubsetAnalysis();

  subsetAnalysis->computeRequiredResources(this,reqResources, inLP);
}


void Join::computeMyRequiredResources(RequiredResources & reqResources, EstLogPropSharedPtr & inLP)
{
  CostScalar memoryResourcesRequired = csZero;

  // only get the max card for this join. The contribution from the children
  // of this join is done inside Join::computeReequiredResource() where
  // child(i)->computeRequiredResources() is called (i=0,1). These two calls
  // will call ::computeMyRequiredResoruce() of the corresponding RelExpr.
  //
  GroupAttributes * myGroupAttr = getGroupAttr();

  CostScalar myMaxCard = myGroupAttr->getResultMaxCardinalityForInput(inLP);

  reqResources.accumulate(csZero, csZero, csZero, myMaxCard);

  if(!isTSJ())
  {
    GroupAttributes * innerChildGroupAttr = child(1).getGroupAttr();
    CostScalar innerChildCardinality =
      innerChildGroupAttr->outputLogProp(inLP)->getResultCardinality();
    CostScalar innerChildRecordSize = innerChildGroupAttr->getCharacteristicOutputs().getRowLength();
    memoryResourcesRequired = (innerChildCardinality * innerChildRecordSize);

    reqResources.accumulate(memoryResourcesRequired, csZero, csZero);

    // add the cpu resources
    RelExpr::computeMyRequiredResources(reqResources, inLP);
  }
  else{
    // isTSJ() == TRUE
    CostScalar cpuResourcesRequired = csZero;
    CostScalar A = csOne;
    CostScalar B (getDefaultAsDouble(WORK_UNIT_ESP_DATA_COPY_COST));

    Int32 nc = getArity();
    EstLogPropSharedPtr inputForChild = inLP;

    for (Lng32 i = 0; i < nc; i++)
    {
      GroupAttributes * childGroupAttr = child(i).getGroupAttr();
      CostScalar childCardinality =
        childGroupAttr->outputLogProp(inputForChild)->getResultCardinality();
      CostScalar childRecordSize = childGroupAttr->getCharacteristicOutputs().getRowLength();

      cpuResourcesRequired += (B * childCardinality * childRecordSize );

      // do this only for the left child
      if(i < 1)
        cpuResourcesRequired += (A * childCardinality);

      inputForChild = child(i).getGroupAttr()->outputLogProp(inputForChild);
    }

    reqResources.accumulate(csZero,
                            cpuResourcesRequired,
                            csZero);
  }
}

void GroupByAgg::computeMyRequiredResources(RequiredResources & reqResources, EstLogPropSharedPtr & inLP)
{
  CostScalar memoryResourcesRequired  = csZero;

  GroupAttributes * childGroupAttr = child(0).getGroupAttr();
  CostScalar childCardinality =
    childGroupAttr->outputLogProp(inLP)->getResultCardinality();
  CostScalar childRecordSize = childGroupAttr->getCharacteristicOutputs().getRowLength();
  memoryResourcesRequired  = (childCardinality * childRecordSize);

  reqResources.accumulate(memoryResourcesRequired, csZero, csZero);

  // add the cpu resources
  RelExpr::computeMyRequiredResources(reqResources, inLP);
}

void Scan::computeMyRequiredResources(RequiredResources & reqResources, EstLogPropSharedPtr & inLP)
{
  if(!(QueryAnalysis::Instance() &&
       QueryAnalysis::Instance()->isAnalysisON()))
    return;


  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();
  const TableAnalysis * tAnalysis = getTableDesc()->getTableAnalysis();
  CANodeId tableId = tAnalysis->getNodeAnalysis()->getId();

  // Find index joins and index-only scans
  addIndexInfo();

  // Base table scan is one of the index only scans.
  CostScalar cpuCostIndexOnlyScan = 
                computeCpuResourceForIndexOnlyScans(tableId);

  CostScalar cpuCostIndexJoinScan = 
                computeCpuResourceForIndexJoinScans(tableId);

  CostScalar cpuResourcesRequired = cpuCostIndexOnlyScan;
  if ( getTableDesc()->getNATable()->isHbaseTable()) 
  {
     if ( cpuCostIndexJoinScan < cpuResourcesRequired )
       cpuResourcesRequired = cpuCostIndexJoinScan;
  } 

  CostScalar dataAccessCost = tAnalysis->getFactTableNJAccessCost();
  if(dataAccessCost < 0)
  {
    CostScalar rowsToScan = appStatMan->
                   getStatsForLocalPredsOnCKPOfJBBC(tableId)->
                     getResultCardinality();

    CostScalar numOfProbes(csZero);

    // skip this for fact table under nested join
    dataAccessCost =
      tAnalysis->computeDataAccessCostForTable(numOfProbes, rowsToScan);
  }


  CostScalar myMaxCard = getGroupAttr()->getResultMaxCardinalityForInput(inLP);
  reqResources.accumulate(csZero, cpuResourcesRequired, 
                          dataAccessCost, myMaxCard
                         );

}
// Required Resource Estimate Methods - End

CostScalar
Scan::computeCpuResourceRequired(const CostScalar& rowsToScan, const CostScalar& rowSize)
{
  CostScalar A = csOne;
  CostScalar B (getDefaultAsDouble(WORK_UNIT_ESP_DATA_COPY_COST));

  CostScalar cpuResourcesRequired = (B * rowsToScan * rowSize);
  cpuResourcesRequired += (A * rowsToScan);

  return cpuResourcesRequired;
}

CostScalar Scan::computeCpuResourceForIndexOnlyScans(CANodeId tableId)
{
  // If index only scans are available, find the most
  // promising index and compute the CPU resource for it.

  const SET(IndexProperty *)& indexOnlyScans = getIndexOnlyIndexes();

  IndexProperty* smallestIndex = findSmallestIndex(indexOnlyScans);

  if ( !smallestIndex )
    return COSTSCALAR_MAX;

  IndexDesc* iDesc = smallestIndex->getIndexDesc();

  const ValueIdList &ikeys = iDesc->getIndexKey();

  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  EstLogPropSharedPtr estLpropPtr = appStatMan->
                  getStatsForLocalPredsOnPrefixOfColList(tableId, ikeys);

  if ( !(estLpropPtr.get()) )
    return COSTSCALAR_MAX;

  return computeCpuResourceRequired(estLpropPtr->getResultCardinality(), 
                                    iDesc->getRecordLength()
                                   );
}


CostScalar Scan::computeCpuResourceForIndexJoinScans(CANodeId tableId)
{
 // If index scans are available, find the index with most promising and
  // compute the CPU resource for it.
  const LIST(ScanIndexInfo *)& scanIndexJoins = getPossibleIndexJoins();

  if ( scanIndexJoins.entries() == 0 )
    return COSTSCALAR_MAX;

  IndexProperty* smallestIndex = findSmallestIndex(scanIndexJoins);
  IndexDesc* iDesc = smallestIndex->getIndexDesc();

  CostScalar rowsToScan;
  return computeCpuResourceForIndexJoin(tableId, iDesc, iDesc->getIndexKey(), rowsToScan);
}

CostScalar Scan::computeCpuResourceForIndexJoin(CANodeId tableId, IndexDesc* iDesc, 
                                                ValueIdSet& indexPredicates, 
                                                CostScalar& rowsToScan)
{
  ValueIdList ikeysCovered;

  UInt32 sz = iDesc->getIndexKey().entries();
  for (CollIndex i=0; i<sz; i++) {
    ValueId x = iDesc->getIndexKey()[i];
    if ( indexPredicates.containsAsEquiLocalPred(x) )
      ikeysCovered.insertAt(i, x);
    else
      break;
  }

  return computeCpuResourceForIndexJoin(tableId, iDesc, ikeysCovered, rowsToScan);
}

CostScalar 
Scan::computeCpuResourceForIndexJoin(CANodeId tableId, IndexDesc* iDesc, 
                                     const ValueIdList& ikeys, CostScalar& rowsToScan)
{
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  EstLogPropSharedPtr estLpropPtr = appStatMan->
                  getStatsForLocalPredsOnPrefixOfColList(tableId, ikeys);

  if ( !(estLpropPtr.get()) ) {
    rowsToScan = COSTSCALAR_MAX;
    return COSTSCALAR_MAX;
  }

  rowsToScan = estLpropPtr->getResultCardinality();
  CostScalar rowSize = iDesc->getRecordLength();

  CostScalar cpuResourceForIndex = computeCpuResourceRequired(rowsToScan, rowSize);

  rowSize = getTableDesc()->getClusteringIndex()->getRecordLength();

  CostScalar cpuResourceForBaseTable = computeCpuResourceRequired(rowsToScan, rowSize);

  return cpuResourceForIndex + cpuResourceForBaseTable;
}

IndexProperty* Scan::findSmallestIndex(const SET(IndexProperty *)& indexes) const
{
   CollIndex entries = indexes.entries();

   if ( entries == 0 ) return NULL;

   IndexProperty* smallestIndex = indexes[0];

   for (CollIndex i=1; i<entries; i++ ) {
     IndexProperty* current = indexes[i];

     if ( smallestIndex->compareIndexPromise(current) == LESS ) {
        smallestIndex = current;
     }
   }

   return smallestIndex;
}

IndexProperty* Scan::findSmallestIndex(const LIST(ScanIndexInfo *)& possibleIndexJoins) const
{
   CollIndex entries = possibleIndexJoins_.entries();

   if ( entries == 0 ) return NULL;

   IndexProperty* smallestIndex = 
            findSmallestIndex(possibleIndexJoins[0]->usableIndexes_);

   for (CollIndex i=1; i<entries; i++ ) {
     
      IndexProperty* current = 
            findSmallestIndex(possibleIndexJoins[i]->usableIndexes_);
                        
      if ( smallestIndex->compareIndexPromise(current) == LESS ) {
          smallestIndex = current;
      }
   }

   return smallestIndex;
}

// This function checks if the passed RelExpr is a UDF rule created by a CQS
// (REL_FORCE_ANY_SCALAR_UDF).  If not, then RelExpr::patternMatch() is called. 
// If the CQS rule includes the UDF name this name is checked against the routine 
// name of this physical isolated scalar UDF.  If the CQS rule includes the action
// name, then this is checked against the action name of this physical isolated
// scalar UDF as well.  The function returns TRUE if so, otherwise FALSE.
NABoolean PhysicalIsolatedScalarUDF::patternMatch(const RelExpr & other) const
{
  // Check if CQS is a scalar UDF rule.
  if (other.getOperatorType() == REL_FORCE_ANY_SCALAR_UDF)
    {
      UDFForceWildCard &w = (UDFForceWildCard &) other;

      // Check function name, if specified in UDFForceWildCard.
      if (w.getFunctionName() != "")
        {
          QualifiedName funcName(w.getFunctionName(), 1 /* minimal 1 part name */);

          // Compare catalog, schema and udf parts separately, 
          // if they exist in the wildcard
          const NAString& catName = funcName.getCatalogName();
          const NAString& schName = funcName.getSchemaName();
          const QualifiedName& x  = getRoutineName();
   
          if ((catName.length() > 0 && x.getCatalogName() != catName) ||
              (schName.length() > 0 && x.getSchemaName()  != schName) ||
              x.getObjectName() != funcName.getObjectName())
            return FALSE;
        }
      // Check action name, if specified in UDFForceWildCard.
      if (w.getActionName() != "")
        {
          NAString actionName = w.getActionName();

          if (getActionNARoutine() &&
              getActionNARoutine()->getActionName())
            {
              // Compare only object parts.  Right now actions don't support catalogs and schemas.
              // This is because action names can have a leading '$' as part of name.
              const NAString& x = *(getActionNARoutine()->getActionName());
   
              if (x != actionName)
                return FALSE;
            }
          else return FALSE;
        }
    return TRUE;
  }
  else
    return RelExpr::patternMatch(other);
}


const NAString RelExpr::getCascadesTraceInfoStr() 
{
  NAString result("RelExpr Cascades Trace Info:\n");
  result += " parent taskid: " + istring(getParentTaskId()) + "\n";
  result += " sub taskid: " + istring(getSubTaskId()) + "\n";
  result += " birth id: " + istring(getBirthId()) + "\n";
  result += " memo exprid: " + istring(memoExprId_) + "\n";
  result += " source memo exprid: " + istring(sourceMemoExprId_) + "\n";
  result += " source groupid: " + istring(sourceGroupId_) + "\n";
  char costLimitStr[50];
  sprintf(costLimitStr," cost limit %g\n", costLimit_);
  result += costLimitStr;
  return result;
}

// remember the creator and source of this relexpr for cascades display gui
void RelExpr::setCascadesTraceInfo(RelExpr *src)
{
  CascadesTask * currentTask = CURRSTMT_OPTDEFAULTS->getCurrentTask();
  if (currentTask)
  {
    // current task created this relexpr 
    parentTaskId_ = currentTask->getParentTaskId();
    stride_ = currentTask->getSubTaskId();

    // remember time of my birth
    birthId_ = CURRSTMT_OPTDEFAULTS->getTaskCount();

    // remember my source
    sourceGroupId_ = currentTask->getGroupId();
    if (src)
      sourceMemoExprId_ = src->memoExprId_;

    // remember current task's context's CostLimit
    Context * context = currentTask->getContext();
    if(context && context->getCostLimit())
      costLimit_ = context->getCostLimit()->getCachedValue();
  }
  // get my MemoExprId and advance it
  memoExprId_ = CURRSTMT_OPTDEFAULTS->updateGetMemoExprCount();
}

NABoolean Join::childNodeContainSkew(
                      CollIndex i,                 // IN: which child
                      const ValueIdSet& joinPreds, // IN: the join predicate
                      double threshold,            // IN: the threshold
                      SkewedValueList** skList      // OUT: the skew list
                               ) const
{
   // Can not deal with multicolumn skew in this method.
   if ( joinPreds.entries() != 1 )
      return FALSE;

   NABoolean statsExist; // a place holder

   Int32 skews = 0;
   for(ValueId vid = joinPreds.init(); joinPreds.next(vid); joinPreds.advance(vid)) {
      *skList = child(i).getGroupAttr()-> getSkewedValues(vid, threshold,
                    statsExist,
                    (*GLOBAL_EMPTY_INPUT_LOGPROP),
                    isLeftJoin()/* include skewed NULLs only for left outer join */
                                                         );

      if (*skList == NULL || (*skList)->entries() == 0)
         break;
      else
         skews++;
    }

   return ( skews == joinPreds.entries() );
}


// 
// Check if some join column is of a SQL type whose run-time
// implementation has a limitation for SB to work.
//
// return 
//   TRUE: no limitation
//   FALSE: has limitation and SB should not be applied
//
NABoolean Join::singleColumnjoinPredOKforSB(ValueIdSet& joinPreds)
{
  ValueId vId((CollIndex)0); joinPreds.next(vId);

  ItemExpr* iePtr = vId.getItemExpr();

  if (iePtr->getOperatorType() == ITM_INSTANTIATE_NULL) {
     iePtr = iePtr -> child(0);
  }

  ValueIdSet vidSet;

  switch (iePtr->getOperatorType()) {
     case ITM_EQUAL: // this case is used to handle char type when
                     // no VEG is formed for a char predicate,
                     // or joins involving subqueries.
     case ITM_VEG_PREDICATE:
     case ITM_VEG_REFERENCE:

       // We only care columns of type ITM_BASECOLUMN (columns belonging to
       // base tables or table-valued stored procedures, see comment on class
       // BaseColumn).
       iePtr->findAll(ITM_BASECOLUMN, vidSet, TRUE, TRUE);

       // If no such columns can be found. Do not bother to continue further,
       // as only base table columns have the potential to be big and skewed.
       if ( vidSet.entries() == 0 )
          return FALSE;

       break;

     default:
       return FALSE;
  }

  ValueId colVid((CollIndex)0); vidSet.next(colVid);

  if ( !colVid.getType().isSkewBusterSupportedType() )
    return FALSE;

  // Additional test
  if ( colVid.getType().getTypeQualifier() == NA_NUMERIC_TYPE &&
       colVid.getType().getTypeName() == LiteralNumeric ) {

      // Exact decimal numeric such as NUMERIC(18,15) can be handled, if
      // all columns involved in join are of the exact same precision and
      // and scale. The comparison ignores NULL attribute of the type (ALM 4953).
      for(ValueId x = vidSet.init(); vidSet.next(x); vidSet.advance(x))
      {
        if ( NOT ((NumericType&)(colVid.getType())).equalIgnoreNull(x.getType()))
           return FALSE;
      }

      return TRUE;
  } else 
  if ( DFS2REC::isAnyCharacter(colVid.getType().getFSDatatype()) ) {

     if ( ((const CharType&)colVid.getType()).getStrCharLimit() > 
          (Lng32) CmpCommon::getDefaultNumeric(USTAT_MAX_CHAR_BOUNDARY_LEN) )
        return FALSE;
  }

  return TRUE;
}


NABoolean Join::multiColumnjoinPredOKforSB(ValueIdSet& joinPreds)
{
   for(ValueId x = joinPreds.init(); joinPreds.next(x); joinPreds.advance(x))
   {
      ValueIdSet dummy(x);
      if ( !singleColumnjoinPredOKforSB(dummy) )
        return FALSE;
   }
   return TRUE;
}

// The new way to capture MC skews. All such skews have been computed during
// update stats. 
NABoolean Join::childNodeContainMultiColumnSkew(
                      CollIndex i,                 // IN: which child to work on
                      const ValueIdSet& joinPreds, // IN: the join predicate
                      double mc_threshold,         // IN: multi-column threshold
                      Lng32 countOfPipelines,      // IN:
                      SkewedValueList** skList     // OUT: the skew list
                               ) 
{
   if (joinPreds.entries() <= 1)
       return FALSE;

   const ColStatDescList& theColList =
          child(i).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->colStats();


  ValueId col;
  ValueIdSet lhsCols;

  CollIndex index = NULL_COLL_INDEX;

  const ValueIdSet& joiningCols = (i==0) ? 
            getEquiJoinExprFromChild0() : getEquiJoinExprFromChild1() ;

  for (col = joiningCols.init();
             joiningCols.next(col);
             joiningCols.advance(col) )
  {
    theColList.getColStatDescIndex(index, col);
    if (index != NULL_COLL_INDEX)
       lhsCols.insert(theColList[index]->getColumn());
  }


   ValueIdList dummyList;

   const MCSkewedValueList* mcSkewList =
          ((ColStatDescList&)theColList).getMCSkewedValueListForCols(lhsCols, dummyList);

   if ( mcSkewList == NULL )
      return FALSE;

   // Apply the frequency threshold to each MC skew and store those passing
   // the thredhold test to the new skList 

   CostScalar rc = child(i).getGroupAttr()->getResultCardinalityForEmptyInput();

   CostScalar thresholdFrequency = rc * mc_threshold;

   *skList = new (CmpCommon::statementHeap()) 
                  SkewedValueList((CmpCommon::statementHeap()));

   for ( CollIndex i=0; i<mcSkewList->entries(); i++ ) {
      MCSkewedValue* itm = mcSkewList->at(i);

      if ( itm->getFrequency() >= thresholdFrequency ) {

        // Use an EncodedValue object to represent the current MC skew
        // and transfer the hash value to it. The hash value is 
        // computed in EncodedValue::computeRunTimeHashValue() and is 
        // the run-time version! No modification should be done to it 
        // from this point on. 
        EncodedValue mcSkewed = itm->getHash();

        (*skList)->insertInOrder(mcSkewed);
      }
   }

   // Set the run-time hash status flag so that we will not try to build
   // the run-time hash again in 
   // SkewedDataPartitioningFunction::buildHashListForSkewedValues().
   (*skList)->setComputeFinalHash(FALSE);
   
   if ( (*skList)->entries() == 0)
     return FALSE;

   return TRUE;
}

// The old way to guess MC skews and repartition the data stream on one
// of the columns with least skews.
NABoolean Join::childNodeContainMultiColumnSkew(
                      CollIndex i,                 // IN: which child to work on
                      const ValueIdSet& joinPreds, // IN: the join predicate
                      double mc_threshold,         // IN: multi-column threshold
                      double sc_threshold,         // IN: single-column threshold
                      Lng32 countOfPipelines,     // IN: 
                      SkewedValueList** skList,    // OUT: the skew list
                      ValueId& vidOfEquiJoinWithSkew // OUT: the valueId of the column
                                                     // whose skew list is returned
                               ) const
{
   if (joinPreds.entries() <= 1)
     return FALSE;

   typedef SkewedValueList* SkewedValueListPtr;

   SkewedValueList** skewLists;
   skewLists =
          new(CmpCommon::statementHeap()) SkewedValueListPtr[joinPreds.entries()];

   CostScalar* skewFactors =
          new(CmpCommon::statementHeap()) CostScalar[joinPreds.entries()];

   // A list of valueIdSets, each valueIdSet element contains a set of 
   // columns from the join predicates. Each set has all columns from the
   // same table participating in the join predicates.
   ARRAY(ValueIdSet) mcArray(CmpCommon::statementHeap(), joinPreds.entries());

   Int32 skews = 0, leastSkewList = 0;
   EncodedValue mostFreqVal;

   CostScalar productOfSkewFactors = csOne;
   CostScalar productOfUecs = csOne;
   CostScalar minOfSkewFactor = csMinusOne;
   CostScalar rc = csMinusOne;
   CostScalar currentSkew;
   CollIndex j = 0;
   NABoolean statsExist;

   for(ValueId vid = joinPreds.init(); joinPreds.next(vid); joinPreds.advance(vid))
   {
      // Get the skew values for the join predicate in question. 
      skewLists[skews] = child(i).getGroupAttr()-> getSkewedValues(vid, sc_threshold,
                    statsExist,
                    (*GLOBAL_EMPTY_INPUT_LOGPROP),
                    isLeftJoin() /* include skewed NULLs only for left outer join */
                                                                   );

      // When the skew list is null, there are two possibilities. 
      // 1. No stats exists, here we assume the worse (stats has not been updated), and
      // move to the next join predicate.
      // 2. The stats is present but we could not detect skews (e.g., the skews are
      // too small to pass the threshold test). We return FALSE to indicate that the
      // column is good enough to smooth out the potential skews in other columns.
      if ( skewLists[skews] == NULL ) {
        if ( !statsExist )
          continue;
        else
          return FALSE; // no stats exist
      }

      // Pick the shortest skew list seen so far. The final shortest skew list
      // will be used for run-time skew detection.
      if ( skews == 0 ||
           (skewLists[skews] && 
           skewLists[skews] -> entries() < skewLists[leastSkewList] -> entries()
           )
         ) 
      {

        // Obtain the colstat for the child of the join predicate on 
        // the other side of the join.
        CollIndex brSide = (i==0) ? 1 : 0;
        ColStatsSharedPtr colStats = child(brSide).getGroupAttr()->
              getColStatsForSkewDetection(vid, (*GLOBAL_EMPTY_INPUT_LOGPROP));

        if ( colStats == NULL )
          return FALSE; // no stats exist for the inner. assume the worst

        // get the skew list 
        const FrequentValueList & skInner = colStats->getFrequentValues();

        CollIndex index = 0;
        const SkewedValueList& newList = *skewLists[skews];

        CostScalar totalFreq = csZero;

        const NAType* nt = newList.getNAType();
        NABoolean useHash = nt->useHashRepresentation();

        for (CollIndex index = 0; index < skInner.entries(); index++)
        {

           const FrequentValue& fv = skInner[index];

          EncodedValue skew = ( useHash ) ? fv.getHash() :  fv.getEncodedValue();

           if ( nt->getTypeQualifier() == NA_NUMERIC_TYPE &&
                nt->getTypeName() == LiteralNumeric ) {

             skew = fv.getEncodedValue().computeHashForNumeric((SQLNumeric*)nt);
           }

           if ( newList.contains(skew) )
             //totalFreq += fv.getFrequency() * fv.getProbability();
             totalFreq += fv.getFrequency() ;
     
        }

        CostScalar totalInnerBroadcastInBytes =
               totalFreq * child(brSide).getGroupAttr()->getRecordLength() *
               countOfPipelines ; 

        if (totalInnerBroadcastInBytes >= 
            ActiveSchemaDB()->getDefaults()
              .getAsLong(MC_SKEW_INNER_BROADCAST_THRESHOLD))
          // ACX QUERY 5 and 8 have skews on the inner side. Better
          // to bet on partitioning on all columns to handle the dual skews.
          // This has been proved by the performance run on 3/21/2012: a 
          // 6% degradation when partition on the remaining non-skew column.
          return FALSE; 
     

        leastSkewList = skews;
        vidOfEquiJoinWithSkew = vid;
      }


      // Get the skew factor for the join predicate in question.
      skewFactors[skews] = currentSkew = child(i).getGroupAttr()->
                        getSkewnessFactor(vid, mostFreqVal, (*GLOBAL_EMPTY_INPUT_LOGPROP));

      // We compute SFa * SFb * SFc ... here
      productOfSkewFactors *= currentSkew;

      // Obtain the colstat for the ith child of the join predicate.
      ColStatsSharedPtr colStats = child(i).getGroupAttr()->
                     getColStatsForSkewDetection(vid, (*GLOBAL_EMPTY_INPUT_LOGPROP));
     
      if ( colStats == NULL )
          return FALSE; // no stats exist. Can not make the decision. return FALSE. 

      // Compute UECa * UECb * UECc ... here
      productOfUecs *= colStats->getTotalUec();

      // get the RC of the table
      if ( rc == csMinusOne )
         rc = colStats->getRowcount();

      // Compute the minimal of the skew factors seen so far
      if ( currentSkew.isGreaterThanZero() ) {
        if ( minOfSkewFactor == csMinusOne || minOfSkewFactor > currentSkew )
           minOfSkewFactor = currentSkew;
      }

      skews++;

     // Collect join columns in this predicate into joinColumns data structure.
     ValueIdSet joinColumns;
     vid.getItemExpr() -> findAll(ITM_BASECOLUMN, joinColumns, TRUE, FALSE);

     // Separate out columns in the join predicates and group them per table.
     //
     // For example, if join predicates are t.a=s.b and t.b=s.b and t.c = s.c,
     // we will have 
     //
     //              mcArray[0] = {t.a, t.b, t.c},
     //              mcArray[1] = {s.a, s.b, s.c},
     //
     // at the end of the loop over join predicates.
     //
     j = 0;
     for(ValueId x = joinColumns.init(); joinColumns.next(x); joinColumns.advance(x))
     {
        if ( !mcArray.used(j) )
           mcArray.insertAt(j, x);
        else {
           ValueIdSet& mcSet = mcArray[j];
           mcSet.insert(x);
        }

        j++;
     }
   } // end of the loop of join predicates

                
   // Now we can find the multi-column UEC, using one of the two multi-column 
   // ValueIdSets (one for each side of the equi-join predicate). The colstats
   // list for the side of the child contains the stats (including the mc ones).
   // one of the mc ones is what we are looking for.
   // 
   ColStatDescList colStatDescList =
         child(i).getGroupAttr()->
                    outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->getColStats();
   
   CostScalar mcUec = csMinusOne;

   const MultiColumnUecList* uecList = colStatDescList.getUecList();

   for(j=0; j < mcArray.entries() && mcUec == csMinusOne && uecList; j++)
   {
       const ValueIdSet& mc = mcArray[j];

       // Do a look up with mc.
       if ( uecList )
          mcUec = uecList->lookup(mc);
   }

   //
   // Compute the final value of 
   //  min( (SFa * SFb * ... *min(UECa * UECb..,RC))/UEC(abc..), 
   //        SFa, SFb, ..., )
   //  = min(productOfSkewFactors * min(productOfUecs, RC)/mcUEC, 
   //        minOfSkewFactor)   
   // 
   //  min(productOfUecs, RC)/mcUEC = 1 when mcUEC is not found
   // 
   CostScalar mcSkewFactor;
   if ( mcUec == csMinusOne || mcUec == csZero )
     mcSkewFactor = MINOF(productOfSkewFactors, minOfSkewFactor);
   else 
     mcSkewFactor = MINOF( 
               productOfSkewFactors * MINOF(productOfUecs, rc) / mcUec,
               minOfSkewFactor
                      );

   if ( mcSkewFactor > mc_threshold ) 
   {
       *skList = skewLists[leastSkewList];
       return TRUE;
   } else
       return FALSE;
}

//
// The content of this method is lifted from 
// DP2InsertCursorRule::nextSubstitute(). 
// A Note has been added in that method so that any changes
// to it should be "copied" here.
//
NABoolean Insert::isSideTreeInsertFeasible()
{
   // Sidetree insert is only supported for key sequenced, non-compressed,
   // non-audited tables with blocksize equal to 4K.
   // Return error, if this is not the case.

   Insert::InsertType itype = getInsertType();

   // Sidetree insert requested?
   if (itype != Insert::VSBB_LOAD )
     return FALSE;

   if ((getTableDesc()->getClusteringIndex()->getNAFileSet()
           ->isCompressed()) ||
        (getTableDesc()->getClusteringIndex()->getNAFileSet()
           ->getBlockSize() < 4096) ||
          (NOT getTableDesc()->getClusteringIndex()->getNAFileSet()
           ->isKeySequenced()) ||
         (getTableDesc()->getClusteringIndex()->getNAFileSet() ->isAudited())
      )
   {
      return FALSE;
   }

   if ( !getInliningInfo().hasPipelinedActions() ) 
     return TRUE;

   if (getInliningInfo().isEffectiveGU() ||
       getTolerateNonFatalError() == RelExpr::NOT_ATOMIC_)
      return FALSE;


   // SideInsert is not allowed when there are pipelined actions (RI,
   // IM or triggers) except MV range logging. This means the only rows
   // projected are the very first and last rows as the beginning and
   // end of the range.

   NABoolean rangeLoggingRequired =
        getTableDesc()->getNATable()->getMvAttributeBitmap().
          getAutomaticRangeLoggingRequired();

   if (getInliningInfo().isProjectMidRangeRows() || !rangeLoggingRequired)
      return FALSE;

   return TRUE;
}

// big memory growth percent (to be used by SSD overlow enhancement project)
short RelExpr::bmoGrowthPercent(CostScalar e, CostScalar m) 
{
  // bmo growth is 10% if 100*abs(maxcard-expected)/expected <= 100%
  // otherwise its 25%
  CostScalar expectedRows = e.minCsOne();
  CostScalar maxRows = m.minCsOne();
  CostScalar difference = maxRows - expectedRows;
  CostScalar uncertainty = 
    (_ABSOLUTE_VALUE_(difference.value()) / expectedRows.value()) * 100;
  if (uncertainty <= 100)
    return 10;
  else
    return 25;
}


CostScalar RelExpr::getChild0Cardinality(const Context* context)
{
   EstLogPropSharedPtr inLogProp = context->getInputLogProp();
   EstLogPropSharedPtr ch0OutputLogProp = child(0).outputLogProp(inLogProp);

   const CostScalar ch0RowCount =
      (ch0OutputLogProp) ? 
                 (ch0OutputLogProp->getResultCardinality()).minCsOne() :
                 csOne;

   return ch0RowCount;
}

NAString *RelExpr::getKey()
{

   if (operKey_.length() == 0)
   {
     char keyBuffer[30];
     snprintf(keyBuffer, sizeof(keyBuffer), "%ld", (Int64)this);
     operKey_ = keyBuffer;
   }
   return &operKey_;
}
