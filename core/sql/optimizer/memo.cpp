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
/* -*-C++-*-
******************************************************************************
*
* File:         memo.C
* Description:  Cascades optimizer search engine
*               implementation for methods on the CascadesGroup and CascadesMemo
*               classes (everything that has to do with storing
*               equivalence classes of expressions). These classes
*               are defined in opt.h
* Created:      7/22/94
* Language:     C++
*
*
*
*
******************************************************************************
*/
//<pb>
// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "PhyProp.h"
#include "Cost.h"
#include "opt.h"
#include "RelScan.h"
#include "CmpStatement.h"

//<pb>
// -----------------------------------------------------------------------
// Methods for class CascadesGroup
// -----------------------------------------------------------------------
CascadesGroup::CascadesGroup(CascadesGroupId groupId,
			     GroupAttributes *groupAttr):
			     plans_(CmpCommon::statementHeap()),
			     goals_(CmpCommon::statementHeap()),
                 exploredRules_(CmpCommon::statementHeap())
{
  // empty linked lists, as yet
  logExprs_ = NULL;
  physExprs_ = NULL;
  exploredInPass_ = 0;
  isBelowRoot_=FALSE;
  exploredRules_.clear();

  // set group attributes and pattern memory
  groupAttr_ = groupAttr;
  CMPASSERT(groupAttr_ != NULL);
  groupAttr_->incrementReferenceCount();

  // create back pointer through CascadesMemo to self
  groupId_ = groupId;

} // CascadesGroup::CascadesGroup
//<pb>
CascadesGroup::~CascadesGroup()
{
  // delete the expression and pattern list
  RelExpr *next_expr = logExprs_;
  while (next_expr != NULL)
    {
      RelExpr *next_next_expr = next_expr->getNextInGroup();
      // if this expression wasn't taken out of CascadesMemo, delete it
      if (next_expr->getGroupId() != INVALID_GROUP_ID)
	delete next_expr;
      else
	next_expr->setNextInGroup(NULL);
      next_expr = next_next_expr;
    }

  next_expr = physExprs_;
  while (next_expr != NULL)
    {
      RelExpr *next_next_expr = next_expr->getNextInGroup();
      if (next_expr->getGroupId() != INVALID_GROUP_ID)
	delete next_expr;
      else
	next_expr->setNextInGroup(NULL);
      next_expr = next_next_expr;
    }

  // Group Attributes
  if (groupAttr_ != NULL)
    groupAttr_->decrementReferenceCount();

  // delete associated contexts
#pragma nowarn(1506)   // warning elimination
  Lng32 maxc = goals_.entries();
#pragma warn(1506)  // warning elimination
  for (Lng32 i = 0; i < maxc; i++)
    delete goals_[i];

} // CascadesGroup::~CascadesGroup
//<pb>


// LCOV_EXCL_START
void CascadesGroup::print(FILE * f, const char * prefix, const char *) const
{
} // CascadesGroup::print

// LCOV_EXCL_STOP
//<pb>
void CascadesGroup::addLogExpr(RelExpr * expr, RelExpr *src)
{
  CMPASSERT(expr->isLogical());

  expr->setGroupId(groupId_);

  // This is a kludge, I will need to find a better place to put this in.
  // We need to make sure that the available indexes of the group picks up
  // the indexes of any scan inserted to the group.
  if (expr->getOperatorType() == REL_SCAN)
  {
    Scan* scanNode = (Scan*) expr;

    scanNode->addIndexInfo();
    getGroupAttr()->addToAvailableBtreeIndexes(scanNode->deriveIndexOnlyIndexDesc());
    getGroupAttr()->addToAvailableBtreeIndexes(scanNode->deriveIndexJoinIndexDesc());
  }

  // 12 years later, and the kludge continues
  if (expr->getArity() == 1 &&
      CmpCommon::getDefault(GA_PROP_INDEXES_ARITY_1) == DF_ON)
  {
    expr->getGroupAttr()->addToAvailableBtreeIndexes
      (expr->child(0).getGroupAttr()->getAvailableBtreeIndexes());
  }
  // reconcile group attributes and logical properties, if necessary

  // if the group attributes are not already the same, merge them
  if (getGroupAttr() != expr->getGroupAttr())
    {
      CMPASSERT(expr->getGroupAttr() != NULL);

      // improve log prop's/group attributes; recost all plans if required
      if (expr->reconcileGroupAttr(getGroupAttr()))
	// XXX recost all plans in this group
	ABORT("reoptimizing after improveLogProp() not implemented"); // LCOV_EXCL_LINE
    }

  // remember that expr came from src
  if (expr)
    expr->setCascadesTraceInfo(src);

  // insert new expression into the list
  if (CmpCommon::getDefault(COMP_BOOL_19) == DF_ON)
  {
    expr->setNextInGroup(logExprs_);
    logExprs_ = expr;
    return;
  }

  if (!logExprs_)
  {
    expr->setNextInGroup(logExprs_);
    logExprs_ = expr;
    return;
  }

  RelExpr* currExpr = logExprs_;

  if(CmpCommon::getDefault(COMP_BOOL_120) == DF_ON)
  {
  while(currExpr->getNextInGroup())
  {
    currExpr = currExpr->getNextInGroup();
  }
  currExpr->setNextInGroup(expr);
  expr->setNextInGroup(NULL);
  }
  else{

    RelExpr * prevExpr = currExpr;

    while(currExpr)
    {
      if(expr->getPotential() < currExpr->getPotential())
      {
        prevExpr = currExpr;
        currExpr = currExpr->getNextInGroup();
      }
      else
        break;
    }

    if(prevExpr == currExpr)
    {
      logExprs_ = expr;
      expr->setNextInGroup(currExpr);
    }
    else
    {
      prevExpr->setNextInGroup(expr);
      expr->setNextInGroup(currExpr);
    }
  }

} // CascadesGroup::addLogExpr

NABoolean CascadesGroup::addPhysExpr(RelExpr* & expr, RelExpr *src)
{
  CMPASSERT(expr->isPhysical());

  // set the common values of a group
  expr->setGroupId(groupId_);
  expr->setGroupAttr(groupAttr_);

  // pass along logExpr's isinCS attribute
  if (getFirstLogExpr())
    expr->setBlockStmt(getFirstLogExpr()->isinBlockStmt());

  // insert new expression into the list
  expr->setNextInGroup(physExprs_);
  physExprs_ = expr;

  // remember that expr came from src
  if (expr)
    expr->setCascadesTraceInfo(src);

  return TRUE;
} // CascadesGroup::addLogExpr
//<pb>
void CascadesGroup::addPlan (CascadesPlan * plan)
{
  CURRSTMT_OPTGLOBALS->plans_count++;  // increment global counter for # of plans

#pragma nowarn(1506)   // warning elimination
  Lng32 index = plans_.entries();
#pragma warn(1506)  // warning elimination
  plans_.insertAt(index,plan); // insert plan at end of list
}

RelExpr * CascadesGroup::unlinkLogExpr(RelExpr *expr)
{
  // -------------------------------------------------------------
  // unlink e from the list of expressions in its group
  // (NOTE: this is almost a clone of CascadesGroup::unlinkPhysExpr())
  // -------------------------------------------------------------

  RelExpr *pred = logExprs_;

  if (pred == expr)
    {
      // expr is the first expression in the list, replace the anchor
      logExprs_ = expr->getNextInGroup();
    }
  else
    {
      // find the predecessor of e in the list
      while (pred != NULL AND
	     pred->getNextInGroup() != expr)
	{
	  pred = pred->getNextInGroup();
	}

      if (pred == NULL)
	// expr not found in this group
	return NULL;

      // replace the predecessor's link with e's link
      pred->setNextInGroup(expr->getNextInGroup());
    }

  // return the unlinked expression (same as input parameter, just
  // to remind the calling procedure that it owns that expression now)
  return expr;
}
//<pb>
// A dangling method
// LCOV_EXCL_START
RelExpr * CascadesGroup::unlinkPhysExpr(RelExpr *expr)
{
  // -------------------------------------------------------------
  // unlink e from the list of expressions in its group
  // (NOTE: this is almost a clone of CascadesGroup::unlinkLogExpr())
  // -------------------------------------------------------------

  RelExpr *pred = physExprs_;

  if (pred == expr)
    {
      // expr is the first expression in the list, replace the anchor
      physExprs_ = expr->getNextInGroup();
    }
  else
    {
      // find the predecessor of e in the list
      while (pred != NULL AND
	     pred->getNextInGroup() != expr)
	{
	  pred = pred->getNextInGroup();
	}

      if (pred == NULL)
	// expr not found in this group
	return NULL;

      // replace the predecessor's link with e's link
      pred->setNextInGroup(expr->getNextInGroup());
    }

  // return the unlinked expression (same as input parameter, just
  // to remind the calling procedure that it owns that expression now)
  return expr;
}
// LCOV_EXCL_STOP

//<pb>
CascadesPlan * CascadesGroup::getFirstPlan() const
{
  if (plans_.entries() > 0)
    return plans_[0];
  else
    return NULL;
}

RelExpr * CascadesGroup::getLastLogExpr() const
{
  if (logExprs_ != NULL) // any logical exprs?
  {
    RelExpr * expr = logExprs_;
    while (expr->getNextInGroup() != NULL) // last expr?
      expr = expr->getNextInGroup();
    return expr;
  }
  else
    return NULL;
}

// use by OptDebug::showMemoStats(). Mask it out from code coverage
// // LCOV_EXCL_START
Lng32 CascadesGroup::getCountOfLogicalExpr() const
{
  RelExpr * expr = logExprs_;
  Lng32 count = 0;
  while (expr != NULL)
  {
    count++;
    expr = expr->getNextInGroup();
  }
  return count;
}
//<pb>
Lng32 CascadesGroup::getCountOfPhysicalExpr() const
{
  RelExpr * expr = physExprs_;
  Lng32 count = 0;
  while (expr != NULL)
  {
    count++;
    expr = expr->getNextInGroup();
  }
  return count;
}

double CascadesGroup::calculateNoOfLogPlans() const
{
  RelExpr * expr = logExprs_;
  double result = 0;
  Lng32 numOfMergedExprs = 0;

  while (expr != NULL)
  {
    result += expr->calculateNoOfLogPlans(numOfMergedExprs);
    expr = expr->getNextInGroup();
  }
  // Add in any logical expressions that are derived from
  // expressions that participated in a group merge.
  result = result + (result*numOfMergedExprs);
  return result;
}
// LCOV_EXCL_STOP

HashValue CascadesGroup::hash()
{
  // take hash value of logical properties
  HashValue hash_value = getGroupAttr()->hash();

  return ( hash_value );
} // CascadesGroup::hash
//<pb>
void CascadesGroup::merge(CascadesGroup * other)
{

  // a merged group gets the smaller of the two indices ...
  // ... to ensure proper deletion in CascadesMemo::~CascadesMemo()
  if (other->groupId_ < groupId_)
    {
      other->merge(this);
    }
  else
    {

      CMPASSERT(other->groupId_ != groupId_);

      // -----------------------------------------------------------------
      // insert all logical and physical expressions of "other" into "this"
      // -----------------------------------------------------------------
      RelExpr * move_expr;
      RelExpr * next_expr;

      for (next_expr = other->logExprs_;
	   (move_expr = next_expr) != NULL;  )
	{
	  next_expr = next_expr->getNextInGroup();
	  addLogExpr(move_expr);
	} // insert all expressions of "other" into "this"

      for (next_expr = other->physExprs_;
	   (move_expr = next_expr) != NULL;  )
	{
	  next_expr = next_expr->getNextInGroup();
	  addPhysExpr(move_expr);
	} // insert all expressions of "other" into "this"

      other->logExprs_ = NULL; // to preserve moved expressions
      other->physExprs_ = NULL;

      // -----------------------------------------------------------------
      // move all contexts from the other group into this group
      // -----------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination
      Lng32 maxg = other->goals_.entries();
#pragma warn(1506)  // warning elimination
      for (Lng32 i = 0; i < maxg; i++)
	{
#pragma nowarn(1506)   // warning elimination
	  Lng32 maxc = goals_.entries();
#pragma warn(1506)  // warning elimination
	  NABoolean found = FALSE;

	  // change the context to the new group number
	  other->goals_[i]->setGroupId(groupId_);

	  // search the already existing contexts for this group for
	  // an equivalent one
	  for (Lng32 j = 0; j < maxc; j++)
	    {
	      // compare the contexts
	      if (NOT goals_[j]->isADuplicate() AND
		  goals_[j]->compareContexts(*(other->goals_[i])) == SAME)
		{
		  found = TRUE;

		  // both groups have the same context in them;
		  // this should only happen in very rare cases where
		  // a group merge is detected by applying a rule that
		  // wasn't used in a previous pass or that was disabled
		  // in one of the groups by guidance or by cost-based pruning

		  // we should never have two contexts where both of
		  // them have outstanding optimization tasks; if one of
		  // them has outstanding tasks then make that one the primary
		  if (other->goals_[i]->getOutstanding() > 0)
		    {
		      Context *moveThis;
		      // move the context in this group to the back of the
		      // list and mark it as a duplicate; copy the other one
		      CMPASSERT(goals_[j]->getOutstanding() == 0);
		      moveThis = goals_[j];
		      goals_.insert(moveThis);
		      // NOTE: looks like goals_.insert(goals_[j]); would
		      // be easier, but that may fail if we resize the
		      // collection while inserting
		      goals_[j]->setDuplicateOf(other->goals_[i]);
		      goals_[j] = other->goals_[i];
		    }
		  else
		    {
		      // delete the context in the other group, it has
		      // no outstanding tasks and should not be optimal
		      // in this current pass
		      goals_.insert(other->goals_[i]);
		      other->goals_[i]->setDuplicateOf(goals_[j]);
		    }
		}
	    }

	  if (NOT found)
	    goals_.insert(other->goals_[i]);
	}

      // all contexts in the other group have now been moved or deleted
      other->goals_.clear();

      // update memo to reflect the group merge
      CURRSTMT_OPTGLOBALS->memo->update(other, this);

      // keep some statistics
      CURRSTMT_OPTGLOBALS->group_merge_count++;

      // delete old group structure
      delete other;

      // schedule a garbage collection task
      CURRSTMT_OPTGLOBALS->task_list->push(new (CmpCommon::statementHeap()) GarbageCollectionTask());

    }
} // CascadesGroup::merge
//<pb>
// -----------------------------------------------------------------------
// CascadesGroup::shareContext()
//
// The algorithm for sharing the Context is as described below:
//
// 1) Iterate over the Contexts that are associated with this Group
// 2) For each Context, compare the given set of required physical
//    properties with the set of required physical properties
//    associated with that Context.
// 3) If the required physical properties compare to SAME, then
//    a) If the Context contains a solution that was created in
//       an earlier pass, clear the status indicators and
//       assign the given cost limit to it.
//    b) If the Context contains a solution that was created in the
//       current pass, then if the given cost limit is less than
//       the cost for the solution, it cannot create a plan.
//    c) If the Context was created in the current pass and
//       contains a failed plan, then compare the given cost limit
//       with the cost limit that caused the failure. If the
//       given cost limit is greater, clear the failed status and
//       assign the given cost limit to the Context.
// 4) If no Context is found, then create a new one and assign the
//    given cost limit to it.
//
// -----------------------------------------------------------------------
//<pb>
/* ============================================================ */
Context * CascadesGroup::shareContext(
                           const ReqdPhysicalProperty* const reqdPhys,
                           const InputPhysicalProperty* const inputPhys,
                           CostLimit* costLimit,
                           Context* parentContext,
                           const EstLogPropSharedPtr &inputLogProp)
{
  // create a brand new context like the one the caller wants
  Context *newContext = new (CmpCommon::statementHeap())
    Context(groupId_, reqdPhys, inputPhys, inputLogProp) ;
  Context* result = newContext;
#pragma nowarn(1506)   // warning elimination
  Lng32 maxc       = goals_.entries();
#pragma warn(1506)  // warning elimination
  NABoolean found = FALSE;

  // The "setCostLimitInContext" below is used to indicate whether the
  // CostLimit is set in a previous Context. If it is not set in a
  // Context within the main loop and is not set within the new Context,
  // then the CostLimit is freed.
  NABoolean setCostLimitInContext = FALSE;

  // search the already existing contexts for this group for an equivalent
  // one that can be shared
  for (Lng32 i = 0; i < maxc AND NOT found; i++)
  {
    Context *existingContext = goals_[i];

    // Compare the requested context with the ith context in the group.
    // Look for an existing context that is NOT marked duplicate and that
    // compares the SAME as the new context.
    if ( NOT existingContext->isADuplicate() AND
         newContext->compareContexts(*existingContext) == SAME )
    {
      if (existingContext->getOutstanding() == 0)
      {
        // an equivalent context has been used on this group before;
        // reuse the existing one instead of creating a new one
        result = existingContext;
        found = TRUE;
        DBGLOGMSGCNTXT(" *** Found context to share ",result);

        // 3a) Found a Context that was created in an earlier pass?
        //     Reuse the Context after clearing flags and cached info.
        if (NOT result->optimizedInCurrentPass())
	{
	  result->clearFailedStatus();

          // when pruning is on and CQD OPH_REDUCE_COST_LIMIT_FROM_
          // PASS1_SOLUTION 'ON' we want to use information about
          // previous pass solution (if any) to reduce our current cost
          // limit. This will give pruning more chances.
          if ( costLimit AND result->getSolution() AND
               CURRSTMT_OPTDEFAULTS->OPHreduceCLfromPass1Solution() )
            costLimit->tryToReduce(*(result->getCostForSolution()),
                                     result->getReqdPhysicalProperty());

          result->setCostLimit(costLimit);
          setCostLimitInContext = TRUE;

          DBGLOGMSG(" *** has to be reoptimized in cur.pass ");
        }
	// 3b) Context created during the current pass contains an optimal
        //     solution, reuse it as-is.
	else
	  if ( result->hasOptimalSolution() )
	  {
            DBGLOGMSG(" *** has optimal solution ");

	    // If the given cost limit is less than the cost for the
            // solution, then it is not a feasible cost limit for
            // creating a plan. But if we want to reuse an existing
            // solution we can do it, it won't cost much.
	    if ( costLimit AND
                 ( costLimit->compareWithPlanCost
                   ((CascadesPlan *)result->getSolution(),
                    result->getReqdPhysicalProperty()) == LESS )
	       )
	    {
              DBGLOGMSG("*** Cost limit exceeded when sharing context ");

              // if we want to reuse solution that exceeded cost limit
              // we need to set its costLimitExceeded_ flag.
              // Previously we were setting new costLimit but didn't change
              // the flag unless costLimit becomes "negative", therefore in
              // createPlan() we considered it as a normal solution which
              // was not completely true. Now we try to set this flag and
              // make pruning possible ASAP, when using failed plan cost.
              if (CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
              {
                result->setCostLimitExceeded();
              }
              else
              {
                result->setCostLimit(costLimit);
                setCostLimitInContext = TRUE;
              }

            } // if (costlimit AND ... LESS
            else
            {
              result->clearFailedStatus();
            }

          }// if (result->hasOptimalSolution()
	   // OR if OPHuseFailedPlanCost() AND result->hasSolution()

          // 3c) Context created during the current pass but does
	  // not contain  an optimal solution
	  else
	    if (costLimit)
            {
              // re-optimize the old context if the new
              // context has a higher costlimit than the old.
              if (costLimit->compareCostLimits
                   (result->getCostLimit(), reqdPhys) == MORE )
              {
                DBGLOGMSG(" *** CL increased for previously failed context ");
                result->clearFailedStatus();
                result->resetCurrentPassNumber();
                result->setCostLimit(costLimit);
                setCostLimitInContext = TRUE;
              } // costLimit MORE that result->costLimit
	      else
              {
                DBGLOGMSG(" *** CL reused for previously failed context ");
                // if we don't resetCurrentPassNumber the next OptimizeGroup
                // task will check it and quit right away. We can skip creating
                // this task and saving a little if we return NULL now. For
                // this CQD OPH_SKIP_OGT_FOR_SHARED_GC_FAILED_CL should be ON.
		if ( CURRSTMT_OPTDEFAULTS->OPHskipOGTforSharedGCfailedCL())
                {
		  delete newContext;
                  delete costLimit;
		  return NULL;
                }
              }

            } //if (costLimit and no solution)
	  } // if (outstanding == 0
	  else
	  {
	    // This context is already in use. There are two explanations
	    // for this: a) we screwed up, or b) a loop in CascadesMemo caused
	    // us to recursively arrive at one of our parent contexts
            DBGLOGMSG(" *** context already in use ");

	    Context *ancestor = parentContext;

	    while (ancestor != NULL AND ancestor != existingContext)
		  ancestor = ancestor->getCurrentAncestor();

	    // make sure this isn't case a)
	    CMPASSERT(ancestor == existingContext);

	    // Make sure this context isn't going to create any tasks,
	    // since a valid plan must not have a node in it whose child
	    // is also one of its ancestors.

	    // So, make an exception to the rule that only one context
	    // with a certain optimization goal can exist in a group:
	    // make a duplicate context and insert it at the end of
	    // the goal list. However: brand this duplicate context as
	    // one whose optimization has failed.
	    found = TRUE;
	    result->markAsDoneForCurrentPass();
	    result->setDuplicateOf(existingContext);
	  }
	} // if ( contexts are SAME
  } // for on group contexts

  if (result == newContext)
  {
    // no sharable context found, add this one to the list of contexts
    goals_.insert(result);
    result->setCostLimit(costLimit);
  }
  else
  {
    // If a valid CostLimit was passed in and it wasn't set in a previous
    // Context, then delete it here.
    if (costLimit != NULL && !setCostLimitInContext)
      delete costLimit;

    // the brand new one won't be needed, we were able to recycle an old one
    delete newContext;
  }

  // save the parent context that is currently using this one
  // Coverity incorrectly flags the following result reference as a
  // USE_AFTER_FREE cid. It is a false alarm because line 712
  // "if (result==newContext)" assures that the "delete newContext" of
  // line 72 occurs only when result is not aliased to newContext.
  // This code annotation should suppresses this false positive.
  // coverity[use_after_free]
  result->setCurrentAncestor(parentContext);
  result->clearCurrentPlan();

  return result;
} // CascadesGroup::shareContext

CascadesMemo::CascadesMemo(CascadesGroupId groups, Lng32 buckets)
             : group_(CmpCommon::statementHeap(),groups),
	       hash_(CmpCommon::statementHeap(),buckets)
{

  if (groups <= 1  OR  buckets <= 1)
    ABORT("defining CascadesMemo structure too small");

  hashSize_ = buckets;

} // CascadesMemo::CascadesMemo
//<pb>

// memo is allocated in opt.cpp on statement heap by method
// QueryOptimizerDriver::initializeOptimization(), and deleted in
// RelExpr::optimizeNode() by setting memo = NULL. This is a speeding
// deletion.

// LCOV_EXCL_START
CascadesMemo::~CascadesMemo()
{
  if (NOT (CURRSTMT_OPTGLOBALS->BeSilent))
    {
      // collect and print some statistics about hash table usage

      float m1 = (float)0, m2 = (float)0;
      for (Lng32 bucket_no = 0; bucket_no < hashSize_;  bucket_no++)
	{
	  Int32 count = 0;
	  RelExpr * e;
	  if (hash_.used(bucket_no))
	    {
	      for (e = hash_[bucket_no];  e != NULL;
		   e = e->getNextInBucket())
		++ count;
	    }
#pragma nowarn(1506)   // warning elimination
	  m1 += count;
#pragma warn(1506)  // warning elimination
#pragma nowarn(1506)   // warning elimination
	  m2 += count * count;
#pragma warn(1506)  // warning elimination
	}
#pragma nowarn(1506)   // warning elimination
      m1 /= hashSize_; m2 /= hashSize_;
#pragma warn(1506)  // warning elimination

      printf("%.0lf entries / %d buckets = %.6lf, var %.6lf\n",
#pragma nowarn(1506)   // warning elimination
	     m1 * hashSize_, hashSize_, m1, m2 - m1 * m1);
#pragma warn(1506)  // warning elimination
    }

  // weed out those groups that have been merged, to avoid deleting
  // some groups twice
#pragma nowarn(1506)   // warning elimination
  Lng32 groupEntries = group_.entries();
#pragma warn(1506)  // warning elimination

  CascadesGroupId groupId = 0;
  for (groupId = 0; (Lng32)groupId < groupEntries;  groupId++)
    {
      if (groupId != group_[groupId]->getGroupId())
	  group_[groupId] = NULL;
    }

  // now delete all groups that are left over
  for (groupId = 0; (Lng32)groupId < groupEntries;  groupId++)
    delete group_[groupId];

} // CascadesMemo::~CascadesMemo
// LCOV_EXCL_STOP

//<pb>
// LCOV_EXCL_START
void CascadesMemo::print(FILE * f, const char *, const char *) const
{
} // CascadesMemo::print
//<pb>

void CascadesMemo::print_all_trees(CascadesGroupId root,
				   NABoolean incl_log,
				   NABoolean incl_phys, FILE * f) const
{
  // Browsing optimizer/memo.cpp@@/main/1 indicates this method was part of
  // an attempt to implement a sql compiler "display" GUI tool using MOTIF.
  // All that code is in ClearCase history if it's ever needed again.
  // But, for now, we're trying to clean up our source code as seen by
  // Coverity. Coverity flags "tree" as an UNUSED_VALUE cid. In fact, the
  // entire body of this method is essentially dead code until someone
  // resurrects the MOTIF/X11 based mxcmp "display" GUI too.
} // CascadesMemo::print_all_trees
// LCOV_EXCL_STOP

//<pb>
RelExpr * CascadesMemo::include(RelExpr * expr,
				NABoolean & duplicateExprFlag,
				NABoolean & groupMergeFlag,
				NABoolean GrpIdIsBinding, //soln-10-070330-3667
				CascadesGroupId groupId,
				Context * context,
                                RelExpr * before
				)
{

  RelExpr * result = expr;

  duplicateExprFlag = groupMergeFlag = FALSE;

  // for a cut operator, return CascadesGroupId bound earlier
  if (expr->isCutOp())
    {
      // identify the cut and determine the group it is bound to
      CutOp * acut = (CutOp *) expr;
      CascadesGroupId groupIdOfCut = acut->getGroupId();

      // if the cut points to a group other than the one it is supposed
      // to be inserted in, we got a reduction rule (a rule that proves
      // some expression to be unnecessary)
      if (groupId != INVALID_GROUP_ID AND
	  groupId != groupIdOfCut AND
	  (*CURRSTMT_OPTGLOBALS->memo)[groupId] != (*CURRSTMT_OPTGLOBALS->memo)[groupIdOfCut])
	{
	  groupMergeFlag = TRUE;
	}

      duplicateExprFlag = TRUE;
      result = (* CURRSTMT_OPTGLOBALS->memo) [groupIdOfCut]->getFirstLogExpr();
      // when Multijoin child node points to  a merged group(duplicate) in the memo.
      // inclusion of multijoin child node in the memo  will check whether the groupid
      // it is referring is duplicate.if so it will assaign the merged groupid to the
      // child node:soln-10-070330-3667
      if ( result != NULL AND groupIdOfCut != (*CURRSTMT_OPTGLOBALS->memo)[groupIdOfCut]->getGroupId()
           AND GrpIdIsBinding)
        result-> setGroupId(groupIdOfCut);

      if (result == NULL)
	result = (* CURRSTMT_OPTGLOBALS->memo) [groupIdOfCut]->getFirstPhysExpr();
    } // return CascadesGroupId bound earlier for cut in rule application

  // for a logical expression, check for duplicates etc.
  else
    {
      // -----------------------------------------------------------------
      // recursively include all the node's children into CascadesMemo
      // -----------------------------------------------------------------
      Int32 arity = result->getArity();
      NABoolean inserted = FALSE;
      NABoolean childDuplicateExprFlag; // ignore duplicates in the child nodes
      NABoolean childGroupMergeFlag;
      NABoolean childGrpIdIsBinding;    // soln-10-070330-3667

      for (Lng32 childIndex = 0;  childIndex < arity;  childIndex++)
	{
	  // insert the child into the CascadesMemo structure and change
	  // the pointer to an child RelExpr to a group number
	  // do not pass a group no or a context, assume that all children
	  // are either cut nodes or logical expressions that go into
	  // a newly formed group
	  // Check whether the GroupIdMode is BINDING. If it is TRUE while including
	  // it in memo we do change the groupId ID of the merged expression in the
	  // above if condition during CutOp processing://soln-10-070330-3667
	  if ( result->child(childIndex).getMode() == ExprGroupId::BINDING )
	     childGrpIdIsBinding = TRUE;
	  else
	     childGrpIdIsBinding = FALSE;

	  result->child(childIndex) = include(result->child(childIndex).getPtr(),
					      childDuplicateExprFlag,
					      childGroupMergeFlag,
					      childGrpIdIsBinding,
                                              INVALID_GROUP_ID,
		                              NULL,
                                              before)->getGroupId();
	}

      if (expr->isLogical())
	{
	  // search for duplicates already in "memo"
	  RelExpr * other = CURRSTMT_OPTGLOBALS->memo->findDuplicate(result);

	  // delete if there has already been a duplicate in memo
	  if (other != NULL)  // previous duplicate found!
	    {
	      // check whether group merging is required
	      CascadesGroupId groupIdOfOther = other->getGroupId();
	      if (groupId != INVALID_GROUP_ID  AND
		  groupIdOfOther != groupId  AND
		  // check whether groups already got merged
		  (* CURRSTMT_OPTGLOBALS->memo) [groupId] != (* CURRSTMT_OPTGLOBALS->memo) [groupIdOfOther])
		{
		  groupMergeFlag = TRUE;
		}

	      // merge any (newly generated) group attributes from
	      // this new expression into the group of the duplicate expression.
	      if (other->getGroupAttr() != NULL AND
		  expr->getGroupAttr() != NULL)
                {
		other->getGroupAttr()->reconcile (*(expr->getGroupAttr()));

                // --------------------------------------------------------
                // For the purpose of synthesis, ensure that any references
                // to this expr references the "duplicate" expr in memo.
                // ------------------------------------------------------------
                if (expr->getGroupAttr()->getLogExprForSynthesis() == expr)
                  expr->getGroupAttr()->setLogExprForSynthesis (other);
                }

	      groupId = groupIdOfOther;
	      duplicateExprFlag = TRUE;
	      CURRSTMT_OPTGLOBALS->duplicate_expr_count ++;
	      result = other;
	    }
	  else // truly a new logical expression
	    {
	      // if no group was given, create one
	      if (groupId == INVALID_GROUP_ID)
		groupId = makeNewGroup(result->getGroupAttr());

	      // insert into group
	      (* this)[groupId]->addLogExpr(result, before);

	      // include in hash bucket
	      addExpr(result, result->treeHash());

              CURRSTMT_OPTGLOBALS->logical_expr_count++;

	      inserted = TRUE;
	    } // truly a new logical expression
	} // for a logical expression, check for duplicates etc.

      // for a physical expression, insert into group if not done yet
      // and also make the expression a candidate for a solution
      // of the context
      if (expr->isPhysical())
	{

	  // insert into list of phys expressions only if not already
	  // inserted as a logical operator
	  if (NOT inserted AND NOT duplicateExprFlag)
	    {
	      // if no group was given, create one
	      if (groupId == INVALID_GROUP_ID)
		groupId = makeNewGroup(result->getGroupAttr());

	      // insert into group
	      (* this)[groupId]->addPhysExpr(result, before);

	      inserted = TRUE;

	      CURRSTMT_OPTGLOBALS->physical_expr_count;
	    }

	  if (context == NULL)
	    {
	      // Trying to insert a physical expression without a context
	      // (this could happen during exploration or when a tree
	      // node that is both physical and logical is inserted)

	      // Create or share an empty context
	      EstLogPropSharedPtr inputLP(new (CmpCommon::statementHeap())
		EstLogProp ((float)1.0));
          // Coverity flags "context" as an UNUSED_VALUE. That is true.
          // We don't care. So, we silence it for now.
          // coverity[returned_pointer]
	      context =
                group_[groupId]->shareContext(NULL,NULL,NULL,NULL,inputLP);
	    }

	} // insert physical expression

      // if the expression has not been inserted because it is a duplicate,
      // then delete it now
      if (NOT inserted &&
          (CmpCommon::getDefault(COMP_BOOL_4) != DF_ON))
	delete expr;
    } // other than a cut operator

  return( result );
} // CascadesMemo::include
//<pb>
void CascadesMemo::addExpr(RelExpr * expr, HashValue hash_value)
{
  Int32 bucket = (Int32) (hash_value.getValue() % hashSize_);
  RelExpr * old_ptr = NULL;

  if (hash_.used(bucket))
    old_ptr = hash_[bucket];
  expr->setNextInBucket(old_ptr);
  hash_.insertAt(bucket, expr);
} // CascadesMemo::addExpr
//<pb>
RelExpr * CascadesMemo::findDuplicate(RelExpr * expr) const
{

  Lng32 bucket = (Lng32)(expr->treeHash().getValue() % hashSize_);

  if (NOT hash_.used(bucket))
    {
      return NULL;
    }
  else
    {
      Int32 arity = expr->getArity();
      GroupAttributes *ga;

      // try all expressions in the appropriate hash bucket
      for (RelExpr * old = hash_[bucket];
	   old != NULL;  old = old->getNextInBucket())
	{
	  Lng32 childIndex;

	  // compare the children
	  if (old->getArity() != arity)
	    goto not_a_duplicate;
	  for (childIndex = arity;  -- childIndex >= 0;  )
	    if ((* expr) [childIndex] != (* old) [childIndex])
	      goto not_a_duplicate;

	  // compare characteristic inputs/outputs
	  if ((ga = expr->getGroupAttr()) != NULL AND
	      NOT (ga->getCharacteristicInputs() ==
		   old->getGroupAttr()->getCharacteristicInputs()) OR
	      NOT (ga->getCharacteristicOutputs() ==
		   old->getGroupAttr()->getCharacteristicOutputs()))
	    goto not_a_duplicate;

	  // compare operators and their arguments
	  if (NOT expr->duplicateMatch(*old))
	    goto not_a_duplicate;

	  // "expr" is a duplicate of "old"
	  return( old );

	  not_a_duplicate :
	  continue;       // check next expression in hash bucket
	} // try all expressions in the appropriate hash bucket
    }
  return( NULL );
} // CascadesMemo::findDuplicate
//<pb>
CascadesGroupId CascadesMemo::makeNewGroup(GroupAttributes *ga)
{
  CascadesGroupId newGroupId = group_.entries();

  group_.insertAt(newGroupId,
		  new (CmpCommon::statementHeap()) CascadesGroup(newGroupId,ga));
  return(newGroupId);
}

void CascadesMemo::update(CascadesGroup * oldGroup, CascadesGroup * newGroup)
{

  // scan through entire table, because the old group
  // might be the result of an earlier group merging
  for (Lng32 groupId = (Int32)(group_.entries()); --groupId >= 0; )
    if (group_[groupId] == oldGroup)
      {
	group_[groupId] = newGroup;
      } // update group pointer

} // CascadesMemo::update
//<pb>
Int32 CascadesMemo::garbageCollection()
{
  LIST(RelExpr *) changed(STMTHEAP); // list of outdated RelExprs
  RelExpr *e;              // a single rel expr
  RelExpr *pred;           // predecessor in the hash chain
  Int32 nc;                  // number of children
  Int32 i;                   // index for a child
  CascadesGroupId childGroupId;   // group # of child
  NABoolean found;         // found an entry to clean up

  // ---------------------------------------------------------------------
  // go through all the hash chains
  // ---------------------------------------------------------------------
  for (Lng32 bucket_no = 0; bucket_no < (Lng32)hashSize_;  bucket_no++)
    {
      if (hash_.used(bucket_no))
	{
          // -------------------------------------------------------------
	  // walk a single hash chain
          // -------------------------------------------------------------
	  pred = NULL;
	  for (e = hash_[bucket_no];  e != NULL;
	       e = e->getNextInBucket())
	    {
              // ---------------------------------------------------------
	      // check an individual expression in the hash chain
              // ---------------------------------------------------------
	      nc = e->getArity();
	      found = FALSE;

	      for (i = 0; i < nc; i++)
		{
		  childGroupId = e->child(i).getGroupId();
		  if (childGroupId != INVALID_GROUP_ID AND
		      group_[childGroupId]->getGroupId() != childGroupId)
		    {
                      // -------------------------------------------------
		      // OK, found a case where the input of a RelExpr
		      // points to a group that no longer exists.
		      // Unlink that expression from the hash chain
		      // and remember it in the global list.
                      // -------------------------------------------------
		      found = TRUE;
		    }
		}

	      if (found)
		{
		  // -----------------------------------------------------
		  // This expression "e" needs to be brought up-to-date,
		  // remember it in a dynamic list and unlink it from
		  // the linked list that represents the hash bucket.
		  // -----------------------------------------------------

		  // -----------------------------------------------------
		  // unlink e from its current hash chain
		  // -----------------------------------------------------
		  if (pred != NULL)
		    pred->setNextInBucket(e->getNextInBucket());
		  else
		    {
		      hash_[bucket_no] = e->getNextInBucket();
		      if (hash_[bucket_no] == NULL)
			// delete the NULL entry
			hash_.remove(bucket_no);
		    }

		  // -----------------------------------------------------
		  // reset the hash chain link and fix the input
		  // group numbers in e
		  // -----------------------------------------------------
		  e->setNextInBucket(NULL);
		  for (i = 0; i < nc; i++)
		    {
		      e->child(i) =
		          group_[e->child(i).getGroupId()]->getGroupId();
		    }

		  // -----------------------------------------------------
		  // remember e in the global list of entries
		  // to clean up
		  // -----------------------------------------------------
		  changed.insert(e);
		} // done handling a "found" case

	      // e becomes the predecessor for the next expression
	      pred = e;
	    } // done with one expression
	} // done with a used hash chain
    } // done with a hash chain

  // remember the state of group merging at this point
  Int32 lowWaterMark = CURRSTMT_OPTGLOBALS->group_merge_count;

  // ---------------------------------------------------------------------
  // now re-insert all the changed expressions into the hash chains
  // ---------------------------------------------------------------------
  for (i = 0; i < (Int32)changed.entries(); i++)
    {
      RelExpr *d;

      e = changed[i];

      // -----------------------------------------------------------------
      // Expression "e" has been identified as an expression that needed
      // to be updated (the input group numbers have already been fixed
      // in the previous loop). Now do the following:
      //
      // - recalculate the hash value for "e"
      // - check whether a duplicate expression "d" already exists in
      //   CascadesMemo and mark the expression "e" for deletion, if it does
      // - if the duplicate "d" exists in another group, then
      //   merge the two groups
      // - if the expression was marked for deletion, then update all
      //   tasks that refer to it (make them reference the duplicate "d"
      //   instead), unlink "e" from the chain of expressions in its
      //   group, and, finally, delete "e".
      // -----------------------------------------------------------------

      if ((d = findDuplicate(e)) != NULL)
	{
	  // -------------------------------------------------------------
	  // oops, this expression was unnecessary, check
	  // whether its duplicate belongs to another group
	  // -------------------------------------------------------------
	  if (d->getGroupId() != e->getGroupId())
	    {
	      // merge groups of d and e
	      group_[d->getGroupId()]->merge(group_[e->getGroupId()]);
	    }

	  // -------------------------------------------------------------
	  // now hunt through the task stack, modifying any
	  // tasks that referenced e to use d from now on
	  // -------------------------------------------------------------
	  for (CascadesTask * task = CURRSTMT_OPTGLOBALS->task_list->getFirst();
	       task != NULL;
	       task = CURRSTMT_OPTGLOBALS->task_list->getNext(task))
	    {
	      task->garbageCollection(e,d,lowWaterMark);
	    }

          // ------------------------------------------------------------
          // For the purpose of synthesis, ensure that any references
          // to expr e are modified to expr d
          // ------------------------------------------------------------
          if (e->getGroupAttr() != NULL AND
              e->getGroupAttr()->getLogExprForSynthesis() == e)
             e->getGroupAttr()->setLogExprForSynthesis (d);

	  // -------------------------------------------------------------
	  // Der Mohr hat seine Schuldigkeit getan, der Mohr kann geh'n.
	  // (Goethe?)
	  // -------------------------------------------------------------
	  delete group_[e->getGroupId()]->unlinkLogExpr(e);

	}
      else
	{
	  // -------------------------------------------------------------
	  // re-insert e into its new hash chain
	  // -------------------------------------------------------------
	  addExpr(e, e->treeHash());
	}
    }

  // return the number of changed expressions
#pragma nowarn(1506)   // warning elimination
  return changed.entries();
#pragma warn(1506)  // warning elimination

} // CascadesMemo::garbageCollection()

NABoolean CascadesMemo::consistencyCheck() const
{
  return TRUE;
} // CascadesMemo::consistencyCheck()
