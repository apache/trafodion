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
 *****************************************************************************
 *
 * File:         OptPhysRelExpr.C
 * Description:  Optimizer methods related to Physical Expressions
 *               These include methods related to costing and plan generation
 *               defined on physical operator classes as well as the
 *               base RelExpr class.
 * Created:      12/10/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// ---------------------------------------------------------------------------

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "AllRelExpr.h"
#include "AllItemExpr.h"
#include "opt.h"
#include "PhyProp.h"
#include "Cost.h"
#include "ControlDB.h"
#include "CostMethod.h"
#include "EstLogProp.h"
#include "ScanOptimizer.h"
#include "DefaultConstants.h"
#include "PartKeyDist.h"
#include "OptimizerSimulator.h"
#include "HDFSHook.h"
#include "Globals.h"
#include "CmpStatement.h"
#include "UdfDllInteraction.h"

extern THREAD_P NAUnsigned              SortEnforcerRuleNumber;

//<pb>

// -----------------------------------------------------------------------
// Methods on class RelExpr associated with physical exprs.  These
// include methods for plan generation and costing.
// -----------------------------------------------------------------------

// static helper function for data source synthesis from two sources
static DataSourceEnum combineDataSources(DataSourceEnum a,
                                         DataSourceEnum b)
{
  if (a == b)
    return a;
  // there is kind of an order on the data source enums, just
  // take the one with the higher order
  if (a == SOURCE_ESP_DEPENDENT OR
      b == SOURCE_ESP_DEPENDENT)
    return SOURCE_ESP_DEPENDENT;
  if (a == SOURCE_ESP_INDEPENDENT OR
      b == SOURCE_ESP_INDEPENDENT)
    return SOURCE_ESP_INDEPENDENT;
  if (a == SOURCE_PERSISTENT_TABLE OR
      b == SOURCE_PERSISTENT_TABLE)
    return SOURCE_PERSISTENT_TABLE;
  if (a == SOURCE_TEMPORARY_TABLE OR
      b == SOURCE_TEMPORARY_TABLE)
    return SOURCE_TEMPORARY_TABLE;
  if (a == SOURCE_TRANSIENT_TABLE OR
      b == SOURCE_TRANSIENT_TABLE)
    return SOURCE_TRANSIENT_TABLE;
  if (a == SOURCE_VIRTUAL_TABLE OR
      b == SOURCE_VIRTUAL_TABLE)
    return SOURCE_VIRTUAL_TABLE;
  return a;
}

// ---------------------------------------------------------------------
// Allocate a workspace for plan generation.
// ---------------------------------------------------------------------
PlanWorkSpace * RelExpr::allocateWorkSpace() const
{
  return new(CmpCommon::statementHeap()) PlanWorkSpace(getArity());
} // RelExpr::allocateWorkSpace()

// -----------------------------------------------------------------------
// RelExpr::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this type.
// -----------------------------------------------------------------------
// Each physical operator class that is derived from RelExpr
// should redefine this virtual function, unless the default
// implementation is acceptable.  This default implementation
// returns a CostMethod object that yields a local cost of 1.0.
// -----------------------------------------------------------------------
CostMethod*
RelExpr::costMethod() const
{
  static THREAD_P CostMethodFixedCostPerRow *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap())
              CostMethodFixedCostPerRow( 1.0  // constant cost for the node
                                       , 0.0  // cost per child row
                                       , 0.0  // cost per output row
                                       );
  return m;
}

//<pb>
// -----------------------------------------------------------------------
// RelExpr::createPlan()
// -----------------------------------------------------------------------
Context* RelExpr::createPlan(Context* myContext,
                                         PlanWorkSpace* pws,
                                         Rule* rule,
                                         Guidance* guidance,
                                         Guidance* & guidanceForChild)
{
  Context* result = NULL;

  // if we already have a solution and are way over level1SafetyNet_
  // then return null (ie, skip create plan)
  double limit_for_CPT = CURRSTMT_OPTDEFAULTS->level1SafetyNetMultiple();
  if (myContext->getSolution() != NULL AND
      (limit_for_CPT > 0.0 AND CURRSTMT_OPTDEFAULTS->getTaskCount() > 
       limit_for_CPT * CURRSTMT_OPTDEFAULTS->level1SafetyNet()))
    return result;

  Lng32 childIndex;
  // check the cost limit if the context has a cost limit,
  // but don't check it if pruning has been disabled:
  NABoolean checkCostLimit =
    ((myContext->getCostLimit() != NULL) AND
     (myContext->isPruningEnabled()));

  CostMethod* cm = this->costMethod();
  const ReqdPhysicalProperty* rppForMe =
                                     myContext->getReqdPhysicalProperty();

  if ( isParHeuristic4Feasible(myContext, rppForMe) )
    return NULL;

  // ---------------------------------------------------------------------
  // First call to createPlan() for this operator.
  // We want to recompute operatorCost depending on the plan number
  // because for example Type2 and Type1 join operators would have
  // quite different costs. BMO flag should be also recomputed. Although
  // computeOperatorCost has a plan as an input parameter we were computing
  // operatorCost only for the first plan and then reusing it with
  // getOperatorCost for other plans. That caused overestimating of
  // operator cost and pruning the better plan.
  // ---------------------------------------------------------------------
  if (pws->isEmpty() OR
      ( CURRSTMT_OPTDEFAULTS->optimizerPruning()) AND
       (pws->getPlanChildCount() == getArity() )
     )
  {
    CascadesPlan* myPlan = myContext->getPlan();

    // -----------------------------------------------------------------
    // Check if the operator is a big memory operator. Cache result in
    // pws and in the plan.
    // -----------------------------------------------------------------
    Lng32 planNumber =
      pws->getCountOfChildContexts()/(getArity()>0 ? getArity() : 1);
    if(isBigMemoryOperator(pws,planNumber))
    {
      pws->setBigMemoryOperator(TRUE);
      myPlan->setBigMemoryOperator(TRUE);
    }
    else
    {
      pws->setBigMemoryOperator(FALSE);
      myPlan->setBigMemoryOperator(FALSE);
    }
/*
    I was trying to reuse an operatorCost not to recompute it when
    coming to reoptimize the plan that failed exceeding cost limit.
    But because of some problems I commented out this reusing for
    a while. Issue needs more investigation. 

    if ( NOT ( CURRSTMT_OPTDEFAULTS->OPHreuseOperatorCost()
               AND myPlan->exceededCostLimit() )
       )
    {
*/
      // -----------------------------------------------------------------
      // Synthesize physical props of leaves right away so that they are
      // available when computeOperatorCost is called.
      // -----------------------------------------------------------------
      if (getArity()==0)
      {
        if (myPlan->getPhysicalProperty() == NULL)
        {
          PhysicalProperty* sppForMe = synthPhysicalProperty(myContext,-1,pws);
          if (sppForMe == NULL)
          {
            // bad properties, no plan can ever be found
            return NULL;
          }
          myPlan->setPhysicalProperty(sppForMe);
        }
      }

      // -----------------------------------------------------------------
      // Now compute the operator cost.
      // -----------------------------------------------------------------
      Lng32 countOfStreams;
      Cost* operatorCost;

      if ( (myContext->isPruningEnabled() AND
            checkCostLimit
           )
           OR (getArity() == 0)
           OR (getOperatorType() == REL_ROOT)
         )
      {
      if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
          operatorCost =
            cm->scmComputeOperatorCost(this, pws, countOfStreams);
        else
          operatorCost =
            cm->computeOperatorCost(this, myContext, countOfStreams);
       }
      else
      // If pruning disabled - skip computeOperatorCost on the way down
      // because it will be recomputed on the way up anyway.
      {
        operatorCost = new HEAP Cost();
        countOfStreams=0;
      }
      pws->initializeCost(operatorCost);
      pws->setCountOfStreams(countOfStreams);

      delete operatorCost;
//  } // of if for reusing operatorCost
  }
  // ---------------------------------------------------------------------
  // Subsequent calls to createPlan() for this operator.
  //
  //  Do partial plan costing when all of the following conditions hold:
  //
  //  1) Operator is not unary and not a leaf.
  //  2) Not all children have been optimized for most recent plan.
  //  3) Cost limit checking is required
  // ---------------------------------------------------------------------

//  else
  // This change is to provide compatibility with the old behaviour of
  // plan generation without pruning. When complete testing is done
  // this part will be simplified. 

  if ( pws->getCountOfChildContexts() > 0 )
    {
      if ( getArity() > 1 )
        {
          if ( pws->getPlanChildCount() < getArity() )
            {
              if ( checkCostLimit )
                {

                 // ------------------------------------------------------------
                  //  Compute partial plan cost for this operator's current
                  // plan.  First calculate known costs of any children and
                  // combine them with this operator's preliminary cost to
                  // produce a partial plan cost.  Store both the known children
                  //  cost and the partial plan cost in the plan workspace.
                  // ------------------------------------------------------------
                  cm->computePartialPlanCost( this, pws, myContext);
                }
            }
          else
            {
              // -----------------------------------------------------------
              //  All children for latest plan have been optimized, so reset
              // for next possible plan.
              // -----------------------------------------------------------
              pws->resetPlanChildCount();
              pws->setPartialPlanCostToOperatorCost();
              // Without clearing known children cost is was used
              // after failed plan for the next one because staying
              // in the loop in createContextForAChild()
              pws->setKnownChildrenCost((Cost *)NULL);
            }
        }
    }

  // ---------------------------------------------------------------------
  // Check cost limits (perform pruning within branch-and-bound).
  //
  // This is only done once for the preliminary cost and once for each
  // child (except the last child) of each plan in the plan workspace.
  // ---------------------------------------------------------------------

  if (    checkCostLimit
       && (    pws->isEmpty()
            || pws->getPlanChildCount() > 0) )
    {

      // ------------------------------------------------------------------
      // Compare the known cost for performing this operation with the cost
      // limit using the cost weight that may be specified in the required
      // physical properties, if any.
      // -----------------------------------------------------------------
      const Cost* partialPlanCost = pws->getPartialPlanCost();
      if (    (myContext
                 ->getCostLimit()
                     ->compareWithCost(*partialPlanCost,rppForMe) == LESS))
        {
          // The intention was to stop optimizing a context when operator
          // cost exceeded costLimit. This is rather aggressive because
          // the next plan (Type1 join instead of Type 2 join as the first
          // plan for HHJ ) can have cheaper operator cost but would never
          // be considered. It is better to do this check in createContext
          // forAChild() giving other plan a chance to complete. 

          if ( pws->isEmpty() AND CURRSTMT_OPTDEFAULTS->OPHpruneWhenCLExceeded() )
          {
            myContext->getPlan()->setExceededCostLimit();
            return NULL;
          }

          // -----------------------------------------------------------------
          // Mark the plan for the previous child Context as failed
          // because it causes the cost for this operation to reach
          // or exceed the cost limit.
          // -----------------------------------------------------------------
          Int32 numChildContextsPruned = getArity() - (Int32)pws->getPlanChildCount();
          if (numChildContextsPruned > 0)
            CURRSTMT_OPTGLOBALS->pruned_tasks_count += numChildContextsPruned;
          pws->markLatestContextAsViolatesCostLimit();
	  DBGLOGMSG("  *** Cost limit exceeded by plan partial cost ");
        }

    }

  //------------------------------------------------------------------------
  //  If we have optimized all children for the latest plan (A leaf operator
  // satisfies this requirement trivially), get the cost of this most recent
  // plan and see if it is the best plan so far.
  //
  //  NOTE: for leaf operators there is only a single implicit plan.
  //------------------------------------------------------------------------
  if (   (   NOT pws->isEmpty()
          && pws->getPlanChildCount() == 0)
      || getArity() == 0                    )
    {
      Cost* cost = pws->getCostOfPlan(pws->getLatestPlan());
      // Only give this plan a chance to be stored as the plan
      // with the best cost if it is acceptable, i.e. it is a valid
      // plan and it satisfies any forced plan constraints.

      // We should check if plan is acceptable when cost!=NULL? SP.
      if (cost AND CmpCommon::getDefault(NSK_DBG_SHOW_PLAN_LOG) == DF_ON )
      {
        // This is to print each intermediate (not only the best) plans
        CascadesPlan * myPlan = myContext->getPlan();
        Cost * costBefore = (Cost *)myPlan->getRollUpCost();

        // The setRollUpCost is going to delete what the plan is currently
        // pointing to.  We need to make a copy of the Cost before
        // setRollUpCost() is called.
        if (costBefore != NULL)
          costBefore = costBefore->duplicate();

		DBGLOGMSG("  *** Latest plan *** ");
        myPlan->setRollUpCost(cost->duplicate());  // deletes old Cost
        DBGLOGPLAN(myPlan);
        myPlan->setRollUpCost(costBefore);

        RelExpr * op = myPlan->getPhysicalExpr();
        Lng32 opArity = op->getArity();
        for (Lng32 childIndex=0; childIndex<opArity; childIndex++)
        {
          Context * childContext =
            pws->getChildContext(childIndex,pws->getLatestPlan());
          if (childContext)
            CURRCONTEXT_OPTDEBUG->showTree(NULL,childContext->getSolution(),
                              "  *** ",TRUE);
        }
      }
      if (currentPlanIsAcceptable(pws->getLatestPlan(),rppForMe))
      {
#ifdef _DEBUG
        if(cost)
        {
          DBGLOGMSG("  *** is acceptable *** ");
        }
#endif
        pws->updateBestCost( cost, pws->getLatestPlan() );
      }
      else if (cost)
      {
#ifdef _DEBUG
        DBGLOGMSG("  *** is not acceptable *** ");
#endif
        delete cost;
      }

    } // I optimized all my children or I am a leaf


  // ---------------------------------------------------------------------
  // Iterator over child Contexts.
  // It creates a child Context, assigns a cost limit and returns the
  // newly created Context to the caller iff the computed cost limit
  // can yield a feasible solution.
  // ---------------------------------------------------------------------
  NABoolean done = FALSE;

  if ( NOT CURRSTMT_OPTDEFAULTS->optimizerPruning() OR getArity() > 0 )
  // need createContextForAChild() call only if there is a child
  while (NOT done)
    {
      // -----------------------------------------------------------------
      // Create a Context for a child.
      // Either create a new Context or reuse an existing one for
      // for optimizing a specific child. The Context is also remembered
      // in the PlanWorkSpace.
      // The method returns a number that identifies the child for which
      // a context was created.
      // -----------------------------------------------------------------
      result = createContextForAChild(myContext, pws, childIndex);

      // -----------------------------------------------------------------
      // If the computed cost limit cannot yield a feasible solution,
      // iterate and create another context for the child
      // -----------------------------------------------------------------
      if (result AND checkCostLimit)
        {
          if (result->isCostLimitFeasible())
            done = TRUE;
          else
	  {
            if ( CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
            {
              if ( result->hasSolution() )
                // here we set costLimitExceeded_ flag to FALSE to be
                // able to reuse this solution. Otherwise the call for
                // method hasOptimalSolution() which is used in many
                // places in the code would return FALSE and we would
                // ignore this existing solution in the future.
                // This is a simple communication from createContext
                // ForAChild() method. If we didn't set this flag to
                // TRUE we would follow if branch above and didn't
                // mark the latest context as violating costLimit.
                // Now this flag when set affects the logic and allow
                // us to prune earlier, for example, when one child
                // of a join already has a solution that exceeds
                // the current costLimit.
                result->clearFailedStatus();
              if ( pws->getPlanChildCount() == getArity() )
                done = TRUE;
            }

            pws->markLatestContextAsViolatesCostLimit();
            DBGLOGMSGCNTXT(" *** Created context not CLFeasible ",result);
	  }
        }
      else
        done = TRUE;
    } // end while

  // ---------------------------------------------------------------------
  // If a Context was created
  // ---------------------------------------------------------------------
  if (result)
    {
      // -----------------------------------------------------------------
      // Obtain a guidance for optimizing the child from the rule.
      // -----------------------------------------------------------------
      if (rule != NULL)
        guidanceForChild = rule->guidanceForOptimizingChild
                                    (guidance, myContext, childIndex);
      else
        guidanceForChild = NULL;
    }
  else
    {
      // -----------------------------------------------------------------
      // If either one of the children did not yield an optimal
      // solution and we are unable to create a plan for this
      // operator, return NULL.
      // -----------------------------------------------------------------
      if (NOT findOptimalSolution(myContext, pws)) return NULL;

      // -----------------------------------------------------------------
      // Check cost limits (perform pruning within branch-and-bound).
      // -----------------------------------------------------------------
      if (checkCostLimit)
        {
          // -------------------------------------------------------------
          // Compare the cost for performing this operation with the cost
          // limit using the cost weight that may be specified in the
          // required physical properties, if any.
          //
          // If the cost for this operation has reached or exceeded
          // the cost limit, return NULL to signal that no plan can
          // be created.
          // -------------------------------------------------------------
          if ( myContext->getCostLimit()->compareWithPlanCost
                            (myContext->getPlan(), rppForMe) == LESS )
            {
              CURRSTMT_OPTGLOBALS->pruned_tasks_count += (Int32)pws->getCountOfChildContexts();
              DBGLOGMSGCNTXT(" *** CLExceeded in createPlan() ",myContext);
              return NULL;
            }
        }

      // -----------------------------------------------------------------
      // If we haven't done so already, synthesize properties now.
      // The only operator who should not already have spp is RelRoot.
      // -----------------------------------------------------------------
      CascadesPlan* myPlan = myContext->getPlan();
      if (myPlan->getPhysicalProperty() == NULL)
      {
        Lng32 planNumber = pws->getBestPlanSoFar();
        myPlan->setPhysicalProperty(synthPhysicalProperty(myContext,
                                                          planNumber,
                                                          pws)
                                   );
      }

      // -----------------------------------------------------------------
      // NOTE: At this point we have found a plan  but we do not know
      // whether the plan actually satisfies the myContext. This check is
      // done in the search engine.
      // -----------------------------------------------------------------
    }

  return result;

} // RelExpr::createPlan()

//<pb>
// -----------------------------------------------------------------------
// RelExpr::isParHeuristic4Feasible()
// Heuristic4 (if it is ON) will try to avoid creating a parallel plan
// for "small" non-partitioned table, base or intermediate, by checking
// estimated logical properties like the number of rows coming from the
// left and right child of the expression to make a decision about
// parallelism like it's done in okToAttemptESPParallelism() function.
// Note that heuristic4 won't prevent parallel plan for GroupBy operator
// if its child is bigger than a threshold.
// The size check will be done in preventPushDownPartReq() function. It
// will also check some partitioning(physical) properties of children's
// plans, if there is any, using getFirstPlan() and
// getPhysicalProperty->isPartitioned() functions.
// We don't want to prevent creation of parallel plan for Exchange
// operator or if we looking at the right child of nested join (because
// Exchange operator cannot be used in this case to enforce partitioning)
// or correlated subquery. In both cases non-empty histogram is passed
// to this expression through input logical properties.
// We don't want to abort creating a plan if Context requires exactly
// one partition or replication of the table which might be necessary
// for the right child of nested or hash join.
NABoolean RelExpr::isParHeuristic4Feasible(Context* myContext,
                           const ReqdPhysicalProperty* rppForMe)
{
  EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();

  if ( rppForMe AND
       CURRSTMT_OPTDEFAULTS->parallelHeuristic4() AND
       NOT CURRSTMT_OPTDEFAULTS->pushDownDP2Requested()  AND
       ( getOperatorType() != REL_EXCHANGE ) AND
       ( (inLogProp->getColStats()).entries() == 0 )
     )
  {
    NABoolean conditionToAbort = FALSE;
    const PartitioningRequirement* pr =
          rppForMe->getPartitioningRequirement();
    if ( pr )
    {
      conditionToAbort = NOT ( (pr->getCountOfPartitions()<2)
         OR pr->isRequirementReplicateViaBroadcast()
         OR pr->isRequirementReplicateNoBroadcast() );
    }
    else
    {
      const LogicalPartitioningRequirement *lpr =
          rppForMe->getLogicalPartRequirement();
      if ( lpr )
      {
        const PartitioningRequirement* logreq = lpr->getLogReq();
        conditionToAbort = NOT ( (logreq->getCountOfPartitions()<2)
           OR logreq->isRequirementReplicateViaBroadcast()
           OR logreq->isRequirementReplicateNoBroadcast() );
      }
    }
    if ( conditionToAbort AND preventPushDownPartReq(rppForMe,inLogProp) )
    {
      return TRUE;
    }
  }
  return FALSE;
}

//<pb>
// -----------------------------------------------------------------------
// RelExpr::createContextForAChild()
// Since the normalizer transforms bushy trees to left linear
// trees, this default implementation optimizes the children from
// right to left.  The idea is that optimization for the right
// child (in general) is cheaper than optimizing the left child.
// Therefore, perform the "easy" step first and come up with a
// cost limit earlier.
// -----------------------------------------------------------------------
Context * RelExpr::createContextForAChild(Context* myContext,
                                 PlanWorkSpace* pws,
                                 Lng32& childIndex)
{
  const ReqdPhysicalProperty* rppForMe =
                                    myContext->getReqdPhysicalProperty();


  // ---------------------------------------------------------------------
  // The default implementation creates exactly one Context per child.
  // EXAMPLE:  If arity = 2, contexts are created in the following order:
  //           childIndex = 1, 0
  // ---------------------------------------------------------------------
  childIndex = getArity() - pws->getCountOfChildContexts() - 1;

  // return for RelExprs with arity 0
  if (childIndex < 0)
    return NULL;

  RequirementGenerator rg(child(childIndex), rppForMe);

  if (childIndex > 0)
    {
      // Don't pass any of the sort, arrangement, or partitioning
      // requirements to the other children, assume that only the left
      // child needs to satisfy them (Union doesn't use this code).
      rg.removeSortKey();
      rg.removeArrangement();
      rg.removeAllPartitioningRequirements();
    }

  if (NOT pws->isEmpty())
  {
    const Context* childContext = pws->getLatestChildContext();

    // ------------------------------------------------------------------
    // Cost limit exceeded or got no solution? Give up since we only
    // try one plan.
    // ------------------------------------------------------------------
    if(NOT (childContext AND childContext->hasOptimalSolution()))
      return NULL;

    if (NOT pws->isLatestContextWithinCostLimit())
      return NULL;

  }

  if (NOT rg.checkFeasibility())
    return NULL;

  Lng32 planNumber = 0;

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);

  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as myContext.
  // Reuse it, if found. Otherwise, create a new Context that contains
  // the same rpp and input log prop as in myContext.
  // ---------------------------------------------------------------------
  Context* result = shareContext(childIndex, rg.produceRequirement(),
                                 myContext->getInputPhysicalProperty(),
                                 costLimit, myContext,
                                 myContext->getInputLogProp());

  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, planNumber, result);

  //--------------------------------------------------------------------------
  //  Only need to keep track of plan child count for operators with more than
  // one child, since only these operators make use of partial plan costing.
  //--------------------------------------------------------------------------
  if (getArity() > 1)
    {
      pws->incPlanChildCount();
    }

  return result;

} // RelExpr::createContextForAChild()

//<pb>
//==============================================================================
//  Using the cost limit from a specified context as a starting point, produce
// a new cost limit for a child by accumulating the parent preliminary cost and
// the known children cost from a specified plan workspace.
//
//
// Input:
//  myContext  -- specified context.
//
//  pws        -- specified plan workspace.
//
// Output:
//  none
//
// Return:
//  Copy of accumulated cost limit.  NULL if context contains no cost limit.
//
//==============================================================================
CostLimit*
RelExpr::computeCostLimit(const Context*       myContext,
                                PlanWorkSpace* pws)
{

  //----------------------------------------------------------------------------
  //  Context contains no cost limit.  Interpret this as an infinite cost limit.
  // Returning NULL indicates an infinite cost limit.
  //----------------------------------------------------------------------------
  if (myContext->getCostLimit() == NULL)
    {
      return NULL;
    }

  //---------------------------------------------------
  //  Create copy of cost limit from specified context.
  //---------------------------------------------------
  CostLimit* costLimit = myContext->getCostLimit()->copy();

  //---------------------------------------------------------------------
  //  Accumulate this operator's preliminary cost into the ancestor cost.
  //---------------------------------------------------------------------
  Cost* tempCost = pws->getOperatorCost();
  costLimit->ancestorAccum(*tempCost, myContext->getReqdPhysicalProperty());

  //-------------------------------------------------------------------------
  //  Accumulate this operator's known children's cost into the sibling cost.
  //-------------------------------------------------------------------------
  tempCost = pws->getKnownChildrenCost();
  if (tempCost != 0)
    {
      costLimit->otherKinAccum(*tempCost);
    }

  //---------------------------------------------------------------------------
  //  Use best plan so far for this operator to try and reduce  the cost limit.
  //---------------------------------------------------------------------------
  tempCost = pws->getBestRollUpCostSoFar();
  if (tempCost != NULL)
    {
      const ReqdPhysicalProperty* rpp = myContext->getReqdPhysicalProperty();
      costLimit->tryToReduce(*tempCost,rpp);
    }

  return costLimit;

} // RelExpr::computeCostLimit()

//<pb>
// -----------------------------------------------------------------------
// RelExpr::findOptimalSolution()
// It associates one of the Contexts that was created for each child
// of this operator with the Context for this operator.
// -----------------------------------------------------------------------
NABoolean RelExpr::findOptimalSolution(Context * myContext,
                                       PlanWorkSpace * pws)
{
  // ---------------------------------------------------------------------
  // The code originally here has been moved to a method with the same
  // name under the class PlanWorkSpace. The rationale for the move are:
  //
  // 1. Since the plans are stored in pws, it's more natural to place the
  //    method under pws.
  //
  // 2. Since pws knows how many plans it has as well as how many children
  //    there are for the operator, the generic implementation there is
  //    more general and can be applicable in more cases. This reduces the
  //    number of subclasses of RelExpr which need to refine the method.
  //    Now, a refinement is needed only if there is processing that must
  //    occur after the optimal plan is chosen, and this processing is
  //    dependent on the operator type.
  // ---------------------------------------------------------------------

  // Plan # is only an output param, initialize it to an impossible value.
  Lng32 planNumber = -1;
  return pws->findOptimalSolution(planNumber);

} // RelExpr::findOptimalSolution()

// -----------------------------------------------------------------------
// RelExpr::currentPlanIsAcceptable()
//
// The virtual implementations of this method ensure the current plan
// for the current operator is acceptable, i.e. it is a valid plan
// and it satisfies any forced plan constraints.
// This default implementation assumes all plans for this operator are
// acceptable.
// -----------------------------------------------------------------------
NABoolean RelExpr::currentPlanIsAcceptable(Lng32 planNo,
            const ReqdPhysicalProperty* const rppForMe) const
{
  return TRUE;

} // RelExpr::currentPlanIsAcceptable()

//<pb>
//==============================================================================
//  Synthesize physical properties for this operator's current plan extracted
// from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used in derived
//                 versions of this member function for synthesizing
//                 partitioning functions.  Unused in this base class version.
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
RelExpr::synthPhysicalProperty(const Context* myContext,
                               const Lng32     planNumber,
                               PlanWorkSpace  *pws)
{
  // ---------------------------------------------------------------------
  // By the time synthPhysicalProperty() is done, the plan whose spp is
  // to be synthesized is stored as currentPlan_ in myContext.
  // ---------------------------------------------------------------------
  CascadesPlan* plan = myContext->getPlan();
  CMPASSERT(plan != NULL);

  Lng32 currentCountOfCPUs = 0;

  // ---------------------------------------------------------------------
  // Get the count of CPUs from child0, if it exists.
  // ---------------------------------------------------------------------
  if(getArity() != 0)
  {
    const Context* childContext = plan->getContextForChild(0);
    CMPASSERT(childContext != NULL);
    const PhysicalProperty* sppForChild =
                         childContext->getPhysicalPropertyForSolution();

    currentCountOfCPUs = sppForChild->getCurrentCountOfCPUs();
    CMPASSERT(currentCountOfCPUs >= 1);
  }
  else
  // Operator has no child. Cost should be available for plan.
  // Get the count of CPUs from there.
  {
    currentCountOfCPUs = plan->getRollUpCost()->getCountOfCPUs();
    CMPASSERT(currentCountOfCPUs >= 1);
  }

  // ---------------------------------------------------------------------
  // The only physical property which this default implementation can
  // synthesize is the no of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppForMe =
                      new (CmpCommon::statementHeap()) PhysicalProperty();
  sppForMe->setCurrentCountOfCPUs(currentCountOfCPUs);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe;

} // RelExpr::synthPhysicalProperty()

//<pb>

DefaultToken RelExpr::getParallelControlSettings (
            const ReqdPhysicalProperty* const rppForMe, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) const
{
  // This default implementation does not handle forcing the number
  // of ESPs.
  numOfESPsForced = FALSE;

  // Get the value from the defaults table that specifies when
  // the optimizer should attempt ESP parallelism.
  DefaultToken attESPPara = CURRSTMT_OPTDEFAULTS->attemptESPParallelism();

  if ( rppForMe->getCountOfPipelines() == 1 )
  {
    // If there is only 1 cpu or the user doesn't want the optimizer
    // to try ESP parallelism for any operators, set the result to OFF.
    attESPPara = DF_OFF;
  }
  return attESPPara;

} // RelExpr::getParallelControlSettings()

NABoolean RelExpr::okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/)
{
  const ReqdPhysicalProperty* rppForMe =
     myContext->getReqdPhysicalProperty();

 // CS or REPLICA: do not consider ESP parallelism if we are in DP2.
  if ( rppForMe->executeInDP2() )
     return FALSE;

  // rowsetIterator cannot be ESP parallelized. Counting logic for rowNumber
  // is no designed to work in nodes that are executing in ESP parallelism.
  if (isRowsetIterator())
    return FALSE;

  NABoolean result = FALSE;

  DefaultToken parallelControlSettings =
    getParallelControlSettings(rppForMe,
                               numOfESPs,
                               allowedDeviation,
                               numOfESPsForced);

  if (parallelControlSettings == DF_OFF)
  {
    result = FALSE;
  }

  else if ( (parallelControlSettings == DF_MAXIMUM) AND
             CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible()
          )
  {
    numOfESPs = rppForMe->getCountOfPipelines();
    // currently, numberOfPartitionsDeviation_ is set to 0 in
    // OptDefaults when ATTEMT_ESP_PARALLELISM is 'MAXIMUM'
    allowedDeviation =  CURRSTMT_OPTDEFAULTS->numberOfPartitionsDeviation();

    // allow deviation by default
    if (CmpCommon::getDefault(COMP_BOOL_61) == DF_OFF)
    {
      EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
      EstLogPropSharedPtr child0OutputLogProp = child(0).outputLogProp(inLogProp);
      const CostScalar child0RowCount =
        (child0OutputLogProp->getResultCardinality()).minCsOne();
      if ( child0RowCount.getCeiling() <
            MINOF(numOfESPs,CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold())
         )
      {
        // Fewer outer table rows then pipelines - allow one or more parts
        allowedDeviation = 1.0;
      }
    }
    result = TRUE;
  }

  else if (parallelControlSettings == DF_ON)
  {
    // Either user wants to try ESP parallelism for all operators,
    // or they are forcing the number of ESPs for this operator.
    // Set the result to TRUE. If the number of ESPs is not being forced,
    // set the number of ESPs that should be used to the maximum number
    // with the allowable deviation percentage from the defaults table.
    // NEW HEURISTIC: If there are fewer outer table rows than the number
    // of pipelines, then set the deviation to allow any level of natural
    // partitioning, including one. This is because we don't want to
    // repartition so few rows to get more parallelism, since we would
    // end up with a lot of ESPs doing nothing.
    if (NOT numOfESPsForced)
    {
      if (getArity() > 0)
        {
          // Determine the number of outer table rows
          EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
          EstLogPropSharedPtr child0OutputLogProp = child(0).outputLogProp(inLogProp);
          const CostScalar child0RowCount =
            (child0OutputLogProp->getResultCardinality()).minCsOne();
          numOfESPs = rppForMe->getCountOfPipelines();
          if (child0RowCount.getCeiling() < numOfESPs)
            {
              // Fewer outer table rows than pipelines - allow one or more parts
              allowedDeviation = 1.0;
            }
          else
            {
              allowedDeviation =  CURRSTMT_OPTDEFAULTS->numberOfPartitionsDeviation();
            }
        }
      else
        allowedDeviation =  CURRSTMT_OPTDEFAULTS->numberOfPartitionsDeviation();
    } // end if number of ESPs not forced
    result = TRUE;
  }
  else
  {
    // Otherwise, the user must have specified "SYSTEM" for the
    // ATTEMPT_ESP_PARALLELISM default. This means it is up to the
    // optimizer to decide.

    // This default implementation will return TRUE if the number of
    // rows returned by child(0) exceeds the threshold from the
    // defaults table. The recommended number of ESPs is also computed
    // to be 1 process per <threshold> number of rows. This is then
    // used to indicate the MINIMUM number of ESPs that will be
    // acceptable. This is done by setting the allowable deviation
    // to a percentage of the maximum number of partitions such
    // that the recommended number of partitions is the lowest
    // number allowed. We make the recommended number of partitions
    // a minimum instead of a hard requirement because we don't
    // want to be forced to repartition the child just to get "less"
    // parallelism.

    EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
    EstLogPropSharedPtr child0OutputLogProp = child(0).outputLogProp(inLogProp);
    CostScalar rowCount =
      (child0OutputLogProp->getResultCardinality()).minCsOne();

    // This is to test better parallelism taking into account
    // not only child0 but also this operator cardinality
    // this could be important for joins but it seems feasible to
    // have it in default method to use for example for TRANSPOSE
    if(CmpCommon::getDefault(COMP_BOOL_125) == DF_ON)
    {
      rowCount += (getGroupAttr()->outputLogProp(inLogProp)->
                     getResultCardinality()).minCsOne();
    }

    const CostScalar numberOfRowsThreshold =
      CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold();

    if (rowCount > numberOfRowsThreshold)
    {
      numOfESPs = rppForMe->getCountOfPipelines();
      allowedDeviation = (float) MAXOF(1.001 -
         ceil((rowCount / numberOfRowsThreshold).value()) / numOfESPs, 0);
      result = TRUE;
    }
    else
    {
      result = FALSE;
    }
  } // end if the user let the optimizer decide

  return result;

} // RelExpr::okToAttemptESPParallelism()

//==============================================================================
// Returns TRUE if only an enforcer operator (either exchange or sort)
// can satisfy the required physical properties. Otherwise, it returns FALSE.
//==============================================================================
NABoolean RelExpr::rppRequiresEnforcer
                    (const ReqdPhysicalProperty* const rppForMe) const
{

  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();

  // Only an exchange operator can satisfy a replicate via broadcast
  // partitioning requirement.
  if ((partReqForMe != NULL) AND
      (
       partReqForMe->isRequirementReplicateViaBroadcast() 
       OR partReqForMe->isRequirementSkewBusterBroadcast()
      )
     )
    return TRUE;

  SortOrderTypeEnum sortOrderTypeReq =
    rppForMe->getSortOrderTypeReq();

  // If a sort order type requirement of ESP_VIA_SORT exists,
  // then return TRUE now. Only a sort operator can satisfy
  // this requirement. The sort rule will be allowed to succeed
  // for this requirement. The exchange rule is also allowed to
  // succeed for this requirement, to allow for parallel sorts. All
  // other implementation rules will fail. This means that when an
  // operator issues an ESP_VIA_SORT requirement the sort to satisfy
  // this requirement will be restricted to being the immediate child
  // of the operator issuing the requirement.
  if (sortOrderTypeReq == ESP_VIA_SORT_SOT)
    return TRUE;

  return FALSE;

} // RelExpr::rppRequiresEnforcer()


// -----------------------------------------------------------------------
//  Called at precodegen time. The executor requires that all operators
// in the same ESP process set use the same partition input values (PIVs),
// and that all partitioning key predicates and partitioning expressions
// that are used are based on this same set of PIVs.
//  This method ensures that this is so by checking if this operators PIVs
// are the same as it's child. If not, it makes the child's PIVs, partitioning
// key predicates, and partitioning expression the same as the parent's.
//  For some operators, the parent partitioning key predicates and
// partitioning expression might need to be mapped first before assigning
// them to the child.
//  Most of the time, the PIVs will be the same. They could only be different
// if the child's plan was stolen from another context.
// -----------------------------------------------------------------------
void RelExpr::replacePivs()
{
  const PhysicalProperty* sppForMe = getPhysicalProperty();

  CMPASSERT(sppForMe != NULL);

  const PartitioningFunction* myPartFunc =
          sppForMe->getPartitioningFunction();

  // The only operator who should not have a partitioning function is
  // RelRoot (although I don't know why it doesn't). If the operator does
  // not have a partitioning function then there is nothing to do, so
  // just return.
  if (myPartFunc == NULL)
    return;

  const ValueIdSet  &myPartKeyPreds = myPartFunc->getPartitioningKeyPredicates();
  const ValueIdSet  &myPivs         = myPartFunc->getPartitionInputValues();
  const ValueIdList &myPivLayout    = myPartFunc->getPartitionInputValuesLayout();

  if (myPivs.entries() == 0)
    return;

  // Process all children.
  for (Lng32 childIndex = 0; childIndex < getArity(); childIndex++)
  {
    PhysicalProperty* sppOfChild  =
      (PhysicalProperty*)(child(childIndex)->getPhysicalProperty());

    CMPASSERT(sppOfChild != NULL);

    PartitioningFunction* childPartFunc =
      (PartitioningFunction*)(sppOfChild->getPartitioningFunction());

    CMPASSERT(childPartFunc != NULL);

    const ValueIdSet &childPartKeyPreds =
                       childPartFunc->getPartitioningKeyPredicates();
    const ValueIdList &childPivLayout =
                       childPartFunc->getPartitionInputValuesLayout();

    // The parent's mapped partitioning function and the child's MUST be
    // compatible, or the child has a replication partitioning function, or
    // the parent is a DP2 Exchange, the child has a logPhysPartitioning
    // function, and it's logical partitioning function is equivalent to the
    // parent's mapped partitioning function.

    // "compatible" means that the functions can use the same set of PIVs
    // to produce valid values. For example, two HASH2 part functions are
    // compatible if they have the same number of partitions. Two range 
    // part functions are compatible if they have the same number of
    // key columns and the data types of the keys match.

    // Check if I have some pivs and the child has some pivs and they
    // are different. Note that it could be possible for this operator
    // to have some pivs and the child to not have any, and yet this
    // is ok. For example, the right child of a replicated join would
    // not have any pivs and yet the join itself would. This is ok
    // and in this case there is nothing to do - you would not want
    // to give the child pivs that it does not need.

    if (NOT childPivLayout.isEmpty() AND
        !(myPivLayout == childPivLayout))
    {
      // Child's pivs exist and are different from mine.
      // Make the child pivs and parent pivs the same and map
      // all item expressions in the child that refer to them
      // to the new PIVs.

      ValueIdMap pivMap(myPartFunc->getPartitionInputValuesLayout(),
                        childPartFunc->getPartitionInputValuesLayout());
      ValueIdSet rewrittenChildPartKeyPreds;

      // check for "compatible" partitioning functions
      CMPASSERT(myPartFunc->getPartitionInputValuesLayout().entries() ==
                childPartFunc->getPartitionInputValuesLayout().entries());
      CMPASSERT(myPartFunc->getPartitioningFunctionType() ==
                childPartFunc->getPartitioningFunctionType());
      CMPASSERT(myPartFunc->getCountOfPartitions() ==
                childPartFunc->getCountOfPartitions());
      // could also check column types of range part. func. but
      // since that's more complicated we won't check for now

      pivMap.mapValueIdSetDown(childPartKeyPreds, rewrittenChildPartKeyPreds);

      // Update the child's partitioning function. Note that since replacePivs()
      // is called before calling preCodeGen() on the child, we do this before
      // any predicates that use PIVs are generated in the child. So, no other
      // places in the child need to be updated.
      childPartFunc->replacePivs(
           myPivLayout,
           rewrittenChildPartKeyPreds);
    } // end if my part key preds are not the same as the childs
  } // end for all children

} // end RelExpr::replacePivs()

// ---------------------------------------------------------------------
// Performs mapping on the partitioning function, from this
// operator to the designated child, if the operator has/requires mapping.
// Note that this default implementation does no mapping.
// ---------------------------------------------------------------------
PartitioningFunction* RelExpr::mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0)
{
  return (PartitioningFunction*)partFunc;
} // end RelExpr::mapPartitioningFunction()

//<pb>
// -----------------------------------------------------------------------
// member functions for class Sort
// -----------------------------------------------------------------------
NABoolean Sort::isBigMemoryOperator(const PlanWorkSpace* pws,
                                    const Lng32 planNumber)
{
  const Context* context = pws->getContext();

  // Get addressability to the defaults table and extract default memory.
  // CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU() should not be called since it can
  // set to > 20MB in some cases, for example if the query tree contains
  // REL_GROUPBY operator. This will result in the assumption that even BMO
  // plans can be sorted in memory(leads to under estimation of cost), but
  // executor uses internal sorts only if the sort table  size < 20MB.
  NADefaults &defs               = ActiveSchemaDB()->getDefaults();
  double memoryLimitPerCPU = defs.getAsDouble(MEMORY_UNITS_SIZE);

  // ---------------------------------------------------------------------
  // Without memory constraints, the sort operator would like to sort all
  // the rows internally.
  // ---------------------------------------------------------------------
  const ReqdPhysicalProperty* rppForMe = context->getReqdPhysicalProperty();
  // Start off assuming that the sort will use all available CPUs.
  Lng32 cpuCount = rppForMe->getCountOfAvailableCPUs();
  PartitioningRequirement* partReq = rppForMe->getPartitioningRequirement();
  const PhysicalProperty* spp = context->getPlan()->getPhysicalProperty();
  Lng32 numOfStreams;

  // If the physical properties are available, then this means we
  // are on the way back up the tree. Get the actual level of
  // parallelism from the spp to determine if the number of cpus we
  // are using are less than the maximum number available.
  if (spp != NULL)
  {
    PartitioningFunction* partFunc = spp->getPartitioningFunction();
    numOfStreams = partFunc->getCountOfPartitions();
    if (numOfStreams < cpuCount)
      cpuCount = numOfStreams;
  }
  else
  if ((partReq != NULL) AND
      (partReq->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS))
  {
    // If there is a partitioning requirement, then this may limit
    // the number of CPUs that can be used.
    numOfStreams = partReq->getCountOfPartitions();
    if (numOfStreams < cpuCount)
      cpuCount = numOfStreams;
  }

  EstLogPropSharedPtr inLogProp = context->getInputLogProp();
  const double probeCount =
                      MAXOF(1.,inLogProp->getResultCardinality().value());

  EstLogPropSharedPtr child0LogProp = child(0).outputLogProp(inLogProp);
  const double child0RowCount =
                  MAXOF(1.,child0LogProp->getResultCardinality().value());

  const double rowsPerCpu = MAXOF(1.0f,(child0RowCount / cpuCount));
  const double rowsPerCpuPerProbe = MAXOF(1.0f,(rowsPerCpu / probeCount));

  const GroupAttributes* groupAttr = getGroupAttr();
  const ValueIdSet& outputVis = groupAttr->getCharacteristicOutputs();

  //------------------------------------------------------------------------
  //  Produce the final Sort Key before determining whether or not this sort
  // should be a big memory operator.
  //------------------------------------------------------------------------
  produceFinalSortKey();

  ValueIdSet sortKeyVis;
  sortKeyVis.insertList(sortKey_);

  const Lng32 keyLength (sortKeyVis.getRowLength());
  const Lng32 rowLength (keyLength + outputVis.getRowLength());
  // Executor`s  rowlength  includes size of tuple descriptor (12 bytes).
  // We need to consider this while determing whether the plan is BMO or NOT,
  // otherwise optimizer thinks some queries as non-BMO but the executor
  // consideres them as BMO.
  const double fileSizePerCpu =
                        ((rowsPerCpuPerProbe * (rowLength + 12)) / 1024. );

  if (spp != NULL &&
      CmpCommon::getDefault(COMP_BOOL_51) == DF_ON
     )
  {
     CurrentFragmentBigMemoryProperty * bigMemoryProperty =
                       new (CmpCommon::statementHeap())
                          CurrentFragmentBigMemoryProperty();


     ((PhysicalProperty *)spp)->setBigMemoryEstimationProperty(bigMemoryProperty);

     bigMemoryProperty->setCurrentFileSize(fileSizePerCpu);
     bigMemoryProperty->setOperatorType(getOperatorType());

     // get cumulative file size of the fragment; get the child spp??
     PhysicalProperty *childSpp =
       (PhysicalProperty *) context->getPhysicalPropertyOfSolutionForChild(0);

     if (childSpp != NULL)
     {
       CurrentFragmentBigMemoryProperty * memProp =
         (CurrentFragmentBigMemoryProperty *) childSpp->
                  getBigMemoryEstimationProperty();
       if (memProp != NULL)
       {
         double childCumulativeMemSize = memProp->getCumulativeFileSize();
         bigMemoryProperty->incrementCumulativeMemSize(childCumulativeMemSize);
         memoryLimitPerCPU -= childCumulativeMemSize;
       }
     }
  }

  return (fileSizePerCpu >= memoryLimitPerCPU);
}

//<pb>
// -----------------------------------------------------------------------
// Sort::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this type.
// -----------------------------------------------------------------------
CostMethod*
Sort::costMethod() const
{
  static THREAD_P CostMethodSort *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodSort();
  return m;
} // Sort::costMethod()

//<pb>
// -----------------------------------------------------------------------
// sort::createContextForAChild() could be redefined to try different
// optimization goals for subsets (prefixes) of the actually required
// sort key. Right now, we always perform a full sort in the sort node
// without making use of partial orders of the child.
// -----------------------------------------------------------------------
Context* Sort::createContextForAChild(Context* myContext,
                             PlanWorkSpace* pws,
                             Lng32& childIndex)
{
  childIndex = 0;

  const ReqdPhysicalProperty* rppForMe =
                                     myContext->getReqdPhysicalProperty();
  Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
  float childNumPartsAllowedDeviation = 0.0;
  NABoolean numOfESPsForced = FALSE;

  // If one Context has been generated for each child, return NULL
  // to signal completion.
  if (pws->getCountOfChildContexts() == getArity())
    return NULL;

  // ---------------------------------------------------------------------
  // Construct a new required physical property vector for my child
  // subtree that replicates the properties that are required of me.
  // The new property vector must not contain any requirements
  // for arrangements or orderings because I will be fulfiling them.
  // ---------------------------------------------------------------------
  RequirementGenerator rg(child(0),rppForMe);

  if (myContext->requiresOrder())
  {
    rg.removeSortKey();
    rg.removeArrangement();
  }

  // If ESP parallelism seems like a good idea, give it a try.
  // Note that if this conflicts with the parent requirement, then
  // "makeNumOfPartsFeasible" will change it to something that
  // doesn't conflict with the parent.
  if (okToAttemptESPParallelism(myContext,
                                pws,
                                childNumPartsRequirement,
                                childNumPartsAllowedDeviation,
                                numOfESPsForced))
  {
    if (NOT numOfESPsForced)
      rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                &childNumPartsAllowedDeviation);
    rg.addNumOfPartitions(childNumPartsRequirement,
                          childNumPartsAllowedDeviation);
  } // end if ok to try parallelism

  if (NOT rg.checkFeasibility())
    return NULL;

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);

  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as those in
  // rppForChild. Reuse it, if found. Otherwise, create a new Context
  // that contains rppForChild as the required physical properties..
  // ---------------------------------------------------------------------
  Context* result = shareContext(childIndex, rg.produceRequirement(),
                                 myContext->getInputPhysicalProperty(),
                                 costLimit, myContext,
                                 myContext->getInputLogProp());

  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, 0, result);

  return result;

} // Sort::createContextForAChild()

//<pb>
//==============================================================================
//  Synthesize physical properties for sort operator's current plan extracted
// from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
Sort::synthPhysicalProperty(const Context* myContext,
                            const Lng32     planNumber,
                            PlanWorkSpace  *pws)
{

  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(myContext,
                                                             planNumber,
                                                             pws);

  if (CmpCommon::getDefault(COMP_BOOL_86) == DF_ON)
  {
     synthPartialSortKeyFromChild(myContext);
  }
  // ---------------------------------------------------------------------
  // Replace the sort key for my spp.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppForMe =
    new (CmpCommon::statementHeap())
      PhysicalProperty(
                   *(myContext->getPhysicalPropertyOfSolutionForChild(0)),
                   sortKey_,
                   ESP_VIA_SORT_SOT,
                   NULL // No Dp2 sort order part func
                      );

  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());
  sppForMe->setDataSourceEnum(SOURCE_TRANSIENT_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  delete sppTemp;

  return sppForMe;
} // Sort::synthPhysicalProperty()

//========================================================================
// See if child is already sorted on a prefix; for an arrangement use 
// existing child sort order if any
//
//========================================================================
void Sort::synthPartialSortKeyFromChild(const Context* myContext)
{
  // child properties: is child sorted?
  const PhysicalProperty *sppOfChild = 
                myContext->getPhysicalPropertyOfSolutionForChild(0);

  PartialSortKeyFromChild_.clear();
  ValueIdList childSortKey = (ValueIdList) sppOfChild->getSortKey();

  // if the child is not sorted there is nothing to do
  if (childSortKey.isEmpty()) return;

  // Properties required from me
  const ReqdPhysicalProperty * rpp =
    myContext->getReqdPhysicalProperty();

  // Does parent require some sort order
  NABoolean parentRequiresOrder =
    rpp->getSortKey() != NULL;

  // Does parent require some arrangement of columns
  NABoolean parentRequiresArrangement =
     rpp->getArrangedCols() != NULL;

  // Is partial sort key applicable?
  NABoolean rewriteSortKey=FALSE; 

  if (parentRequiresArrangement)
  {
    ValueIdSet parentArrangement= *(rpp->getArrangedCols());
    ValueIdSet myChildSortKeyAsASet(childSortKey);
    myChildSortKeyAsASet = myChildSortKeyAsASet.removeInverseOrder();

    // check if mySortKeySet is contained in requiredArrangement
    ValueIdSet commonElements =
      parentArrangement.intersect(myChildSortKeyAsASet);

    if (!commonElements.isEmpty())
    {
       // rewrite Sortkey to make use of existing order
       // populate partialsort key as well
       // childSortKey[i] is in commonElements then 
       // add it to partial sort key
       for (UInt32 i=0; i < childSortKey.entries(); i++)
       {
          if (commonElements.contains(
               childSortKey[i].getItemExpr()->removeInverseOrder()->getValueId()
                                     )
             )
          {
            rewriteSortKey=TRUE;
            PartialSortKeyFromChild_.insert(childSortKey[i]);
          }
          else 
            break;
       }
    }

    if (rewriteSortKey)
    {
      // sortkey_ = PartialSortKeyFromChild_ + remaining elements from sort key
      ValueIdSet mySortkey=sortKey_;
      ValueIdSet mySortkeyTemp = mySortkey;
      ValueIdSet partialSortKey = PartialSortKeyFromChild_.removeInverseOrder();

      // mySortkey = mySortkey - (mySortKey /\ partialSortKey);
      mySortkey.subtractSet(
                  mySortkey.intersect(partialSortKey));

      sortKey_ = PartialSortKeyFromChild_.removeInverseOrder();
      for (ValueId x=mySortkey.init() ; 
                mySortkey.next(x); 
            mySortkey.advance(x))
      {
        sortKey_.insert(x);
      }
    }
  } // is Arrangement requirement

  else if (parentRequiresOrder)
  {
    const ValueIdList parentSortKey = *(rpp->getSortKey());
    UInt32 size = childSortKey.entries() > parentSortKey.entries() ? 
                        parentSortKey.entries() : childSortKey.entries();
    // size is at least one else we would not be here..

    for (UInt32 i=0; i <= (size-1); i++)
    {
        // Make sure non-empty prefix of the sortkey from the parent and
        // the child are exactly the same (semantically). Note for descending
        // case, childSortKey[i] can be different from parentSortKey[i] but if
        // the underneath item expressions are inverse of a common expression,
        // the non-empty prefix test should pass.
        //
        // Solu. 10-090303-9701 (NF:R2.4:Partial Sort not used in sort of
        // descending keys).

        if (childSortKey[i] == parentSortKey[i] ||
            (childSortKey[i].getItemExpr()->getOperatorType() == ITM_INVERSE 
                                        &&
             parentSortKey[i].getItemExpr()->getOperatorType() == ITM_INVERSE 
                                        &&
             childSortKey[i].getItemExpr()->removeInverseOrder()->getValueId()
                                   ==
             parentSortKey[i].getItemExpr()->removeInverseOrder()->getValueId()
            )
           )
       {
         rewriteSortKey=TRUE;
         PartialSortKeyFromChild_.insert(childSortKey[i]);
       }
       else
         break;
    } // for loop
  } // Is order requirement
} // Sort::synthPartialSortKeyFromChild()
//<pb>
//==============================================================================
//  Combine arrangement columns with sort key to produce a final sort key.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  none
//
//==============================================================================
void
Sort::produceFinalSortKey()
{
  // ----------------------------------------------------------------------
  // if the parent asked for an arrangement rather than (or in addition to)
  // a particular sort order, choose the final sort order now.
  // ----------------------------------------------------------------------
  if (NOT arrangedCols_.isEmpty())
  {
    ValueId svid;
    ValueIdList simpleSortCols;
    NABoolean parentSortOrderReq = FALSE;

    //---------------------------------
    // Determine if we have a sort key.
    //---------------------------------
    if (NOT sortKey_.isEmpty())
    {

      //------------------------------------------------
      //  Indicate that parent required a sort ordering.
      //------------------------------------------------
      parentSortOrderReq = TRUE;

      //----------------------------------------------------------------
      // Compute required sort order in simplified form.  This will
      // be used to ensure that the combined sort key (required sort
      // order and required arrangement) doesn't reflect the same
      // underlying column more than once.
      //
      // Note: the parent already eliminated duplicate expressions from
      // the required sort order and arrangement when they were created.
      //----------------------------------------------------------------
      simpleSortCols = sortKey_.simplifyOrderExpr();
    }

    //-------------------------------------------------
    //  Combine arranged columns with sort key columns.
    //-------------------------------------------------
    for (ValueId x = arrangedCols_.init();
         arrangedCols_.next(x);
         arrangedCols_.advance(x))
    {

      //----------------------------------------------------------------
      // If there was no required sort order, we need not do any checks.
      //----------------------------------------------------------------
      if (NOT parentSortOrderReq)
        sortKey_.insert(x);
      else
      {

        //--------------------------------------------------------------------
        //  Required sort order exists, so ensure that the underlying columns
        // from the required arrangement don't already exist in the simplified
        // form of the required sort order.
        //--------------------------------------------------------------------
        svid = x.getItemExpr()->simplifyOrderExpr()->getValueId();
        if (NOT simpleSortCols.contains(svid))
        {

          //---------------------------------------------------------
          // The sort key doesn't already reflect an ordering on this
          // column. Go ahead and add it.
          //---------------------------------------------------------
          sortKey_.insert(x);
        }
      }
    }
    arrangedCols_.clear();
  }

} //Sort::produceFinalSortKey()

//<pb>
// -----------------------------------------------------------------------
// member functions for class Exchange
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Exchange::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this type.
// -----------------------------------------------------------------------
CostMethod*
Exchange::costMethod() const
{
  static THREAD_P CostMethodExchange *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodExchange();
  return m;
} // Exchange::costMethod()

//<pb>
// -----------------------------------------------------------------------
// Exchange::createContextForAChild()
//
// The optimizer introduces an Exchange operator in the dataflow tree
// in order to redistribute the workload over multiple processes. A
// process can either be the master executor, an instance of an
// Executor Server Process (ESP), or a DP2, the Disk server
// Process.
//
// The ESP Exchange is introduced either when an operator in
// the query execution plan is expected to consume memory very
// heavily, or when a tree of operators implement a CPU-intensive
// operation that is likely to saturate the cpu, or asynchronous
// access to partitioned data can improve the throughput of an
// operator, or operator trees. It encapsulates parallelism.
//
// The DP2 exchange is introduced whenever we want the child tree to
// execute in DP2.
//
// The Exchange is just another dataflow operator. It is represented
// using the notation Exchange(m:n), where m is the number of data
// streams it receives as its internal dataflow inputs and n is the
// number of data streams it produces as its own output. Using Cascades
// jargon, it is a physical operator which enforces one or both of the
// following two properties:
//
// 1) The partitioning of data
//
//    The term "partition" is an artifact of the product's association
//    with Tandem. It is synonymous with the term data stream, which is,
//    perhaps, more relevant in the context of the dataflow architecture
//    of this product.
//
// 2) The location where the plan (fragment) should execute.
//
//    A plan can be able to execute in one of the following "locations":
//      a) In the MASTER only.
//      b) In an ESP only.
//      c) In either the master or an ESP.
//      d) In DP2 only.
//
// Every Exchange enforces the execution of its parent outside of DP2.
// The optimizer uses a single Exchange for enforcing a location as
// well as the desired partitioning. However, when the plan executes,
// two instances of the Exchange operator may be necessary, one for
// enforcing the DP2 location and another for repartitioning of data.
// The query rewrite phase that follows after the optimal plan is
// selected introduces another Exchange as required.
//
// The Exchange is a "CISC" operator. The Exchange which is seen by the
// optimizer is replaced with one of the following three sets of
// operators by the code generator:
//
// 1) Exchange(plan fragment) -> PartitionAccess(plan fragment)
//
//    Produced by plan 0 below.
//
//    The Exchange is replaced with a PartitionAccess (PA).
//
//    The PartitionAccess implements the messaging protocol between the
//    SQL executor, running in an Executor Server Process (ESP), and
//    DP2, the disk server process. It provides a dedicated connection
//    between an ESP and a particular DP2. The ESP can establish one
//    or more sessions successively on the same connection. A session
//    is established when the PartitionAccess downloads a fragment of
//    a query execution plan to DP2. A session permits a bi-directional
//    dataflow during which the ESP can provide external dataflow inputs
//    and the DP2 returns the corresponding result of executing the plan
//    fragment.
//
//    The PartitionAccess follows the policy of establishing the
//    successor session only after the one that is in progress completes.
//    According to this policy, there can be at most one session in
//    progress per DP2 with which an ESP has established a connection.
//    This policy permits an ESP to overlap the processing of rows that
//    are provided by a particular DP2, while that DP2 is processing the
//    successive set of rows. EXPLAIN shows this exchange as
//    PARTITION_ACCESS.
//
//    A PartitionAccess operator in the executor talks to at most one
//    DP2 process at a time.
//
// 2) Exchange(plan fragment) -> SplitTop(PartitionAccess(plan fragment))
//
//    Produced by plan 0 below.
//
//    The Exchange is replaced with a SplitTop, PartitionAccess pair.
//    The split top node in this configuration is called the PAPA (PArent
//    of PA). Its purpose is to overcome the restriction that a PA node
//    can only communicate with one DP2 at a time. The PAPA node enables
//    parallel DP2 requests to partitioned tables. EXPLAIN shows this
//    form of an exchange as SPLIT_TOP_PA.
//
// 3) Exchange(plan fragment) -> Exchange(plan fragment) ->
//       SplitTop(SendTop(SendBottom(SplitBottom(
//            SplitTop(PartitionAccess(plan fragment))))))
//
//    Produced by plan 0 below.
//
//    Combination of results 2) and 4). Alternatively, we may also produce
//    a combination of results 1) and 4) (not shown). This is done if we
//    need to produce a partitioning scheme that can not be produced by
//    a PA or a PAPA node and if the child needs to execute in DP2. We
//    need to add a repartitioning step above the PA or PAPA.
//
// 4) Exchange(plan fragment) ->
//       SplitTop(SendTop(SendBottom(SplitBottom(plan fragment))))
//
//    Produced by plan 1 below.
//
//    This is the case when ESP parallelism is used. Also called the
//    ESP Exchange or REPARTITION in EXPLAIN.
//
// -----------------------------------------------------------------------
Context * Exchange::createContextForAChild(Context* myContext,
                                  PlanWorkSpace* pws,
                                  Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  // An enforcer rule MUST be given a required physical property to enforce.
  // Compute the number of plans that will be created for this Exchange.
  // Since the Exchange enforces the location where a plan should
  // execute as well as the partitioning function for the data streams
  // that it produces, the required physical properties should contain
  // either a specific location or a partitioning criterion.
  // ---------------------------------------------------------------------
  const ReqdPhysicalProperty* rppForMe =
                                     myContext->getReqdPhysicalProperty();
  PartitioningRequirement *partReqForMe =
    rppForMe->getPartitioningRequirement();
  SortOrderTypeEnum sortOrderTypeReq = NO_SOT;
  PartitioningRequirement* dp2SortOrderPartReq = NULL;
  childIndex = 0;

  // ---------------------------------------------------------------------
  // The location requirement MUST be an ESP because the Exchange can
  // only enforce the execution of its parent outside of DP2.
  // ---------------------------------------------------------------------
  CMPASSERT(rppForMe AND NOT rppForMe->executeInDP2());

  // ---------------------------------------------------------------------
  // Cost limit exceeded? Erase latest Context from the workspace.
  // Erasing the Context does not decrement the count of Contexts
  // considered so far.
  // ---------------------------------------------------------------------
  if ((NOT pws->isEmpty()) AND (NOT pws->isLatestContextWithinCostLimit()))
    pws->eraseLatestContextFromWorkSpace();

  // ---------------------------------------------------------------------
  // Allocate local variables.
  // ---------------------------------------------------------------------
  Lng32 planNumber = pws->getCountOfChildContexts();
  PlanExecutionEnum plenum;
  ReqdPhysicalProperty* rppForChild = NULL;
  Context* result = NULL;

  // ---------------------------------------------------------------------
  // Loop over the possible plans and try to find one that we actually
  // want to try.
  //
  // We consider two plans for an exchange operator (unless forced
  // otherwise):
  //
  // Plan 0: try executing the child tree in DP2 and pass our own
  //         partitioning requirement (if any) as a logical partitioning
  //         requirement to the child.
  // Plan 1: if the parent requires some partitioning, try to enforce
  //         that partitioning here with an ESP exchange and require no
  //         partitioning and location ESP from the child.
  //
  // ---------------------------------------------------------------------
  while (planNumber < 2 AND result == NULL)
    {
      // do we actually want to TRY this plan or do we want to skip it?
      NABoolean tryThisPlan = FALSE;

      if (planNumber == 0)
      {
        // ||opt some day we may consider some optimizations where we don't
        // need to create a DP2 context:
        // - for immediate children of an EXCHANGE
        // - for groups that definitely can't execute in DP2
        // - for groups that we don't want to execute in DP2

        // Don't create a DP2 context if there is a partitioning requirement
        // that requires broadcast replication or hash repartitioning. Only
        // an ESP exchange can perform replication or hash repartitioning.
        // When parallel heuristics were added we put an extra condition to
        // create DP2 context: parallelHeuristic1_ is FALSE(not used) or
        // numBaseTables_ in the current group is set to 1, or when the
        // OPTS_PUSH_DOWN_DAM default is turned on to push operators in DP2.

        // For "nice context" one more check was added to skip DP2 context.
        // In fact, parallelHeuristic1 can be TRUE only when CQD
        // ATTEMPT_ESP_PARALLELISM is SYSTEM. If it is ON then we always
        // try DP2 context. Now OPH_USE_NICE_CONTEXT should be OFF for DP2
        // context to be tried unless this is required to be in DP2.
        if ( ( ( NOT ( CURRSTMT_OPTDEFAULTS->parallelHeuristic1() OR
                       myContext->isNiceContextEnabled()
                     )
               ) OR CURRSTMT_OPTDEFAULTS->pushDownDP2Requested() OR
               ( myContext->getGroupAttr()->getNumBaseTables() == 1)
             )  AND
             ( (partReqForMe == NULL) OR
                NOT partReqForMe->isRequirementReplicateViaBroadcast()
             )
           )
        {
          plenum = EXECUTE_IN_DP2;
          // do not try DP2 plans in Trafodion
          // tryThisPlan = TRUE;

          // Never pass a sort order type requirement to DP2.
          // But, pass any dp2SortOrderPartReq.
          sortOrderTypeReq = NO_SOT;
          dp2SortOrderPartReq = rppForMe->getDp2SortOrderPartReq();
        }
      }
      else if (planNumber == 1)
      {
        const Context* childContext = pws->getLatestChildContext();

        // If we had a DP2_OR_ESP_NO_SORT requirement, and the
        // first DP2 context we tried failed because the child
        // was not physically partitioned the same as the
        // dp2SortOrderPartReq requirement, then retry plan 0
        // without the dp2SortOrderPartReq this time.
        if ((rppForMe->getSortOrderTypeReq() == DP2_OR_ESP_NO_SORT_SOT) AND
            (childContext != NULL) AND
            (childContext->getReqdPhysicalProperty()->
               getDp2SortOrderPartReq() != NULL) AND
            NOT childContext->hasOptimalSolution())
        {
          // Get rid of the context for the failed plan
          pws->deleteChildContext(0,0);
          // redo plan 0 with no dp2SortOrderPartReq this time
          planNumber = 0;
          plenum = EXECUTE_IN_DP2;
          // do not try DP2 plans in Trafodion
          // tryThisPlan = TRUE;
          sortOrderTypeReq = NO_SOT;
          dp2SortOrderPartReq = NULL;
        }
        else
        {
          //   Try generating a execute in ESP location context for
          // repartitioning if there is a partitioning requirement
          // that can be enforced by repartitioning or if the
          // requirement is to execute in exactly one stream, and
          // if parallelism is enabled/possible.
          //   If the requirement is to execute in exactly one stream,
          // then repartitioning is not needed to coalesce multiple
          // streams into one stream, but we need to generate an ESP
          // context here to give the child a chance to execute in
          // parallel. The one partitioning requirement that we should not
          // and cannot generate an ESP context for is a no broadcast
          // replication. This is because if we did, this could lead
          // to repartitioning, which would have to be a broadcast
          // replication, and if the requirement is for no broadcast
          // replication, this means the parent is a nested join and
          // this could lead to wrong answers, because each nested
          // join instance must get back only the rows for its probes,
          // not all the rows from all the probes.
          if ( (partReqForMe != NULL) AND
               NOT partReqForMe->castToRequireReplicateNoBroadcast() AND
               NOT rppForMe->getNoEspExchangeRequirement() AND
               ( (rppForMe->getCountOfPipelines() > 1)
                  // this is needed for nice context, otherwise when
                  // ATTEMPT_ESP_PARALLELISM is OFF getCountOfPipelines()
                  // would return 1 (?) ESP context with no partReq won't
                  // be created and implementation rules won't fire on
                  // any logical expressions. Otherwise CORE/test060
                  // fails because it uses OFF. 
                  OR myContext->isNiceContextEnabled()
                )
             )
          {
            // Apply heuristics:
            // if the previous DP2 context returned a plan and if we
            // would repartition without having a requirement for
            // more than one partition, then just forget about that
            // option. Why do we do that: we assume that using the
            // DP2 context will be cheaper than the repartitioning
            // plan.
            if (NOT (partReqForMe->getCountOfPartitions() <= 1 AND
                     childContext AND
                     childContext->hasOptimalSolution() AND
                     rppForMe->getMustMatch() == NULL AND
                     CmpCommon::getDefault(TRY_DP2_REPARTITION_ALWAYS)
                       == DF_OFF))
            {
              plenum = EXECUTE_IN_ESP;
              tryThisPlan = TRUE;
              // For ESP contexts, just pass along any existing
              // sort order type requirements
              sortOrderTypeReq = rppForMe->getSortOrderTypeReq();
              dp2SortOrderPartReq = rppForMe->getDp2SortOrderPartReq();
            }
          } // end if an enforcable partitioning requirement
        } // end if redo DP2 plan
      } // end if planNumber == 1

      if (tryThisPlan)
        {
          // -------------------------------------------------------------
          // Construct a new required physical property vector for my
          // child.  No need to use the requirement generator, since
          // an enforcer rule has its own group as a child and since
          // we simply strip off the partitioning requirement and
          // replace the location requirement.
          // -------------------------------------------------------------
          rppForChild = new (CmpCommon::statementHeap())
            ReqdPhysicalProperty(*rppForMe,
                                 rppForMe->getArrangedCols(),
                                 rppForMe->getSortKey(),
                                 sortOrderTypeReq,
                                 dp2SortOrderPartReq,
                                 NULL,
                                 plenum);

          // -------------------------------------------------------------
          // Add logical partitioning requirement if the child context
          // is for DP2 and the exchange was given a part requirement
          // -------------------------------------------------------------
          if ((plenum == EXECUTE_IN_DP2) AND
              (rppForMe->getPartitioningRequirement() != NULL))
            rppForChild->setLogicalPartRequirement(
                 new(CmpCommon::statementHeap()) LogicalPartitioningRequirement(
                      rppForMe->getPartitioningRequirement()));

          // -------------------------------------------------------------
          // Indicate we are giving the child a logical ordering or
          // arrangement requirement if the child context
          // is for DP2 and the exchange was given a ordering or
          // arrangement requirement.
          // -------------------------------------------------------------
          if ((plenum == EXECUTE_IN_DP2) AND
              ((rppForMe->getArrangedCols() != NULL) OR
               (rppForMe->getSortKey() != NULL)))
            rppForChild->setLogicalOrderOrArrangementFlag(TRUE);

          // -------------------------------------------------------------
          // Check for a CONTROL QUERY SHAPE directive and process
          // it. Sorry, this is actually the most complex part of the
          // method.
          // -------------------------------------------------------------
          rppForChild = processCQS(rppForMe,rppForChild);

        } // tryThisPlan

      if (tryThisPlan AND rppForChild)
        {
          // -------------------------------------------------------------
          // Compute the cost limit to be applied to the child.
          // -------------------------------------------------------------
          CostLimit* costLimit = computeCostLimit(myContext, pws);

          // -------------------------------------------------------------
          // Get a Context for optimizing the child.  Search for an
          // existing Context in the CascadesGroup to which the child
          // belongs that requires the same properties as those in
          // rppForChild. Reuse it, if found. Otherwise, create a new
          // Context that contains rppForChild as the required
          // physical properties.
          // -------------------------------------------------------------
          result = shareContext(
               childIndex, rppForChild,
               myContext->getInputPhysicalProperty(), costLimit,
               myContext, myContext->getInputLogProp());
        } // tryThisPlan(2)

      // -----------------------------------------------------------------
      // Store the Context for the child in the PlanWorkSpace, even if
      // it is NULL.
      // -----------------------------------------------------------------
      pws->storeChildContext(childIndex, planNumber, result);

      // -----------------------------------------------------------------
      // increment loop variable
      // -----------------------------------------------------------------
      planNumber++;

    } // while loop

  return result;

} // Exchange::createContextForAChild()

//<pb>
// -----------------------------------------------------------------------
// Add required physical properties from the CONTROL QUERY SHAPE directive
// for an Exchange operator
// -----------------------------------------------------------------------
ReqdPhysicalProperty * Exchange::processCQS(
     const ReqdPhysicalProperty *rppForMe,
     ReqdPhysicalProperty *rppForChild)
{
  ReqdPhysicalProperty *result = rppForChild;

  if (rppForMe->getMustMatch())
    {
      if (rppForMe->getMustMatch()->getOperatorType() == REL_FORCE_EXCHANGE)
        {
          ExchangeForceWildCard *mm =
            (ExchangeForceWildCard *) rppForMe->getMustMatch();
          ExchangeForceWildCard::forcedExchEnum whichType = mm->getWhich();
          ExchangeForceWildCard::forcedLogPartEnum logPart =
            mm->getWhichLogPart();
          Lng32 numClients = mm->getHowMany();

          // translate invalid numClients into literal
          if (numClients <= 0)
            numClients = ANY_NUMBER_OF_PARTITIONS;

          if (rppForChild->executeInDP2())
            {
              // don't try a DP2 child context for a forced ESP exchange
              if (whichType == ExchangeForceWildCard::FORCED_ESP_EXCHANGE)
                return NULL;

              LogPhysPartitioningFunction::logPartType logPartType = LogPhysPartitioningFunction::ANY_LOGICAL_PARTITIONING;
              Lng32 numClients = mm->getHowMany();
              NABoolean mustUsePapa = FALSE;
              NABoolean numberOfPAsForced = FALSE;

              // translate invalid numClients into literal
              if (numClients <= 0)
                numClients = ANY_NUMBER_OF_PARTITIONS;

              // signal PA/PAPA decision if forced
              if (whichType == ExchangeForceWildCard::FORCED_PAPA)
                mustUsePapa = TRUE;
              else if (whichType == ExchangeForceWildCard::FORCED_PA)
              {
                numClients = 1;
                numberOfPAsForced = TRUE;
              }

              // map CQS literal into LogPhysPartitioningFunction
              // literals
              switch (mm->getWhichLogPart())
                {
                case ExchangeForceWildCard::ANY_LOGPART:
                  logPartType =
                    LogPhysPartitioningFunction::ANY_LOGICAL_PARTITIONING;
                  break;
                case ExchangeForceWildCard::FORCED_GROUP:
                  logPartType =
                    LogPhysPartitioningFunction::PA_PARTITION_GROUPING;
                  break;
                case ExchangeForceWildCard::FORCED_SPLIT:
                  logPartType =
                    LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING;
                  break;
                default:
                  CMPASSERT(0); // internal error
                }

              // force a log part scheme only if there is a partitioning
              // requirement and if it is for more than one partition
              if (NOT rppForMe->requiresPartitioning() OR
                  rppForMe->getPartitioningRequirement()->
                  castToRequireExactlyOnePartition())
		{
		  logPartType =
                    LogPhysPartitioningFunction::ANY_LOGICAL_PARTITIONING;
		}

              if (logPartType != 
		  LogPhysPartitioningFunction::ANY_LOGICAL_PARTITIONING OR
                  numClients != ANY_NUMBER_OF_PARTITIONS OR
                  mustUsePapa)
                {
                  // now replace the logical partitioning requirement
                  // (note that this may result in a logical partitioning
                  // requirement with a NULL PartitioningRequirement in it)
                  result->setLogicalPartRequirement(
                       new(CmpCommon::statementHeap())
                       LogicalPartitioningRequirement(
                            rppForMe->getPartitioningRequirement(),
                            logPartType,
                            numClients,
                            mustUsePapa,
                            numberOfPAsForced ));
                }
            } // child is required to execute in DP2
          else
            {
              // child is required to execute in an ESP

              // don't try an ESP child context for a forced PA/PAPA
              if (whichType == ExchangeForceWildCard::FORCED_PA OR
                  whichType == ExchangeForceWildCard::FORCED_PAPA)
                return NULL;

              if (numClients != ANY_NUMBER_OF_PARTITIONS)
                {
                  // Process a CQS requirement for a given number of
                  // ESP partitions: Alter the required physical
                  // properties by adding a requirement for a given
                  // number of ESPs
                  RequirementGenerator rg(child(0),rppForChild);

                  rg.addNumOfPartitions(numClients,0.0);

                  // Replace the requirement if it is still feasible,
                  // give up if it is not feasible. We should never run
                  // into the case where the requirement is not feasible
                  // because the exchange node should not require any
                  // partitioning at this point.
                  if (rg.checkFeasibility())
                    result = rg.produceRequirement();
                  else
                    CMPASSERT(0); // for debugging
                }
            } // child is required to execute in an ESP
        } // must match specifies some exchange node
      else
        {
	  if (NOT CURRSTMT_OPTDEFAULTS->ignoreExchangesInCQS())
	    // We found out that the we must match a pattern that is not
	    // an exchange. Operators usually don't check this, but
	    // since we did, we might as well notice and save ourself
	    // some work.
	    return NULL;
        }
    } // end CQS processing

  return result;
}

//<pb>
// -----------------------------------------------------------------------
// Exchange::storePhysPropertiesInNode()
// -----------------------------------------------------------------------
void Exchange::storePhysPropertiesInNode(const ValueIdList &partialSortKey)
{
  const PhysicalProperty *sppForMe   = getPhysicalProperty();
  const PhysicalProperty *sppOfChild = child(0)->getPhysicalProperty();
  CMPASSERT(sppForMe AND sppOfChild);

  // ---------------------------------------------------------------------
  // determine whether this Exchange node is redundant
  // (it is redundant if it enforces neither location nor partitioning)
  // ---------------------------------------------------------------------
  isRedundant_ =
    (sppOfChild->getPlanExecutionLocation() ==
     sppForMe->getPlanExecutionLocation() AND
     sppForMe->getPartitioningFunction()->
     comparePartFuncToFunc(*sppOfChild->getPartitioningFunction()) == SAME);

  // ---------------------------------------------------------------------
  // Copy some of the physical properties into data members of this
  // object, so that the code generator and preCodeGen can look at them
  // (they may also modify them if needed).
  // ---------------------------------------------------------------------

  // Don't copy the sort key if the sort order type is DP2.
  // This is because copying the sort key to sortKeyForMyOutput_
  // will cause a merge of sorted streams to occur, and we don't
  // want that if the sort order type is DP2.
  if (sppForMe->getSortOrderType() != DP2_SOT)
  {
    sortKeyForMyOutput_ = sppForMe->getSortKey();
  }
  else if (CmpCommon::getDefault(COMP_BOOL_86) == DF_ON) 
  {
    // begin fix to partial sort: comment out this if
    // if (partialSortKey == sppForMe->getSortKey())
    // otherwise, partial sort with split top can give wrong results
      sortKeyForMyOutput_ = partialSortKey;
  }
  topPartFunc_        = sppForMe->getPartitioningFunction();
  bottomPartFunc_     = sppOfChild->getPartitioningFunction();
  bottomLocation_     = sppOfChild->getPlanExecutionLocation();
  bottomLocIsSet_     = TRUE;
  indexDesc_          = sppOfChild->getIndexDesc();

  NABoolean isChildSyncAccess=FALSE;
  const LogPhysPartitioningFunction *lpf =
    bottomPartFunc_->castToLogPhysPartitioningFunction();
  if (lpf)
    isChildSyncAccess = lpf->getSynchronousAccess();

  // solution 10-040505-5752: we check for synchronous sequential access. In
  // that case, we check for the reverse scan flag. We manufacture a partition
  // search key as if the scan direction is forward. We then set the direction
  // of the scan as a flag on the partition access node; this gets copied from
  // the flag on the exchange node.
  if ( (CmpCommon::getDefault(ATTEMPT_ASYNCHRONOUS_ACCESS) == DF_OFF ||
        isChildSyncAccess) &&
       CmpCommon::getDefault(ATTEMPT_REVERSE_SYNCHRONOUS_ORDER) == DF_ON
     )
  {
    OperatorTypeEnum ot = child(0)->castToRelExpr()->getOperatorType();
    RelExpr *relExpr = child(0)->castToRelExpr();
    NABoolean noScanFound = FALSE;

    while (ot != REL_SCAN && ot != REL_FILE_SCAN)
    {
      if (relExpr->getArity() > 1 ||
        relExpr->getOperatorType() == REL_EXCHANGE
       )
      {
        noScanFound= TRUE;
        break;
      }

      if (relExpr->getArity() != 0)
      {
        relExpr = relExpr->child(0)->castToRelExpr();
        ot = relExpr->getOperatorType();
      }
      else
      {
        noScanFound = TRUE;
        break;
      }
    }

    FileScan *scan = NULL;
    if (relExpr->getOperatorType() == REL_FILE_SCAN ||
        relExpr->getOperatorType() == REL_SCAN)
       {
         scan = (FileScan *) relExpr;
       }
       else
       {
         noScanFound = TRUE;
       }

    if (!noScanFound &&
        scan->getReverseScan() &&
        scan->getIndexDesc()->isPartitioned())
    {
      ValueIdSet externalInputs = scan->getGroupAttr()->
                                     getCharacteristicInputs();
      ValueIdSet dummySet;
      ValueIdSet selPreds(scan->getSelectionPred());

      setOverReverseScan();

      // Create and set the Searchkey for the partitioning key:
      partSearchKey_ =  new (CmpCommon::statementHeap())
                   SearchKey(scan->getIndexDesc()->getPartitioningKey(),
			     scan->getIndexDesc()->
                                 getOrderOfPartitioningKeyValues(),
			     externalInputs,
			     TRUE,
			     selPreds,
                             dummySet, // not used
                             scan->getIndexDesc() 
                             );
    }

    else
      partSearchKey_      = sppOfChild->getPartSearchKey();
  }
  else
    partSearchKey_      = sppOfChild->getPartSearchKey();
}

//<pb>
//==============================================================================
//  Synthesize physical properties for exchange operator's current plan
// extracted from a specified context.
// Calls: synthPhysicalPropertyDP2() OR synthPhysicalPropertyESP();
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
Exchange::synthPhysicalProperty(const Context *myContext,
                                const Lng32     planNumber,
                                PlanWorkSpace  *pws)
{
  const PhysicalProperty      *sppOfChild = myContext->
                                 getPhysicalPropertyOfSolutionForChild(0);

  if (sppOfChild->executeInDP2())
  {
    return synthPhysicalPropertyDP2(myContext);
  }
  else // child executes in ESP
  {
    return synthPhysicalPropertyESP(myContext);
  }

} // Exchange::synthPhysicalProperty()

//==============================================================================
// Synthesize physical properties for exchange operator's current plan
// extracted from a specified context when child executes in DP2.
//
// Helper method for Exchange::synthPhysicalProperty()
//==============================================================================
PhysicalProperty*
Exchange::synthPhysicalPropertyDP2(const Context *myContext)
{
  NABoolean usingSynchronousAccess = FALSE;
  NABoolean usePapa = FALSE;
  NABoolean canMaintainSortOrder = TRUE;
  PartitioningFunction *myPartFunc;
  NodeMap *myNodeMap = NULL;

  const ReqdPhysicalProperty  *rppForMe = myContext->getReqdPhysicalProperty();
  const PhysicalProperty *sppOfChild = myContext->getPhysicalPropertyOfSolutionForChild(0);
  PartitioningFunction *childPartFunc = sppOfChild->getPartitioningFunction();
  const LogPhysPartitioningFunction *logPhysChildPartFunc =
    childPartFunc->castToLogPhysPartitioningFunction();

  // -------------------------------------------------------------
  // DP2 exchange (PA or PAPA)
  // -------------------------------------------------------------
  if (logPhysChildPartFunc)
    {
      usingSynchronousAccess = logPhysChildPartFunc->getSynchronousAccess();
      usePapa = logPhysChildPartFunc->getUsePapa();

      // Can this logPhys partitioning function maintain the order of
      // an individual partition of the physical partitioning
      // function.  In order to maintain the order, a merge expression
      // may be required.
      //
      canMaintainSortOrder =
        logPhysChildPartFunc->canMaintainSortOrder(sppOfChild->getSortKey());

      // Child synthesized a LogPhysPartitioningFunction which has all
      // the instructions on what to do. Unpack these instructions.
      myPartFunc  = logPhysChildPartFunc->getLogPartitioningFunction();

      // ---------------------------------------------------------------
      // For all cases (PA_PARTITION_GROUPING, SUBPARTITIONING and
      // replicate-no-broadcast), try to use any existing logical or physical
      // partitioning function's nodemap that matches our partition count
      // requirement. Only as a last resort do we synthesize a node map which
      // attempts to co-locate ESPs and the corresponding DP2 partitions.
      // We do this because synthesizeLogicalMap() is very expensive.
      //
      // Note: although the grouping model does not exactly fit for
      // the cases of SUBPARTITIONING or a replicate-no-broadcast
      // partitioning function, it results in a reasonable allocation
      // of ESPs
      // ---------------------------------------------------------------
      if (CmpCommon::getDefault(COMP_BOOL_82) == DF_ON) {
        myNodeMap = logPhysChildPartFunc->getOrMakeSuitableNodeMap
          (FALSE/*forDP2*/);
      }
      else {
        const NodeMap* childNodeMap = logPhysChildPartFunc
          ->getPhysPartitioningFunction()
          ->getNodeMap();

        Lng32 myPartitionCount = myPartFunc->getCountOfPartitions();
        myNodeMap = ((NodeMap *)(childNodeMap))
          ->synthesizeLogicalMap(myPartitionCount, FALSE/*forDP2*/);

        if(myPartFunc->castToReplicateNoBroadcastPartitioningFunction()) {
          for(CollIndex i = 0; i < (CollIndex)myPartitionCount; i++) {
            myNodeMap->setPartitionState(i, NodeMapEntry::ACTIVE);
          }
        }
      }
    } // if logPhysChildPartFunc
  else
    {
      // we don't allow any child partitioning functions other
      // than a LogPhysPartitioningFunction or a
      // SinglePartitionPartitioningFunc for DP2 children.
      CMPASSERT(childPartFunc AND
                childPartFunc->castToSinglePartitionPartitioningFunction());

      // child synthesized one partition, synthesize one partition and
      // perform  the simplest case of PA_PARTITION_GROUPING
      myPartFunc  = childPartFunc;
      myNodeMap =
        childPartFunc->getNodeMap()->copy(CmpCommon::statementHeap());
      if (CmpCommon::getDefault(COMP_BOOL_83) == DF_ON) {
        // we want to evenly distribute single-cpu ESP
        // (replicate broadcasters & others)
        myNodeMap->setToRandCPU(0);
      }
    } // end if not a logPhysChildPartFunc

  // Now, set the synthesized sort order type and the corresponding
  // dp2SortOrderPartFunc.
  PartitioningFunction *dp2SortOrderPartFunc = NULL;
  SortOrderTypeEnum sortOrderType = NO_SOT;

  if (sppOfChild->getSortKey().isEmpty())
    sortOrderType = NO_SOT;
  else
    {
      if (myContext->requiresOrder())
        {
          // Since there is a required order/arrangement, we want to set
          // the sort order type to DP2 if we are accessing ALL partitions
          // asynchronously, there is a requirement for a DP2 sort order,
          // and the synthesized DP2 sort order partitioning function
          // (i.e. the physical partitioning function) matches the
          // requirement.
          if (usePapa AND
              NOT usingSynchronousAccess AND
              ((rppForMe->getSortOrderTypeReq() == DP2_SOT) OR
               (rppForMe->getSortOrderTypeReq() == DP2_OR_ESP_NO_SORT_SOT)) AND
              rppForMe->getDp2SortOrderPartReq()->
              partReqAndFuncCompatible(sppOfChild->getDp2SortOrderPartFunc()))
            {
              sortOrderType = DP2_SOT;
              dp2SortOrderPartFunc = sppOfChild->getDp2SortOrderPartFunc();
            }
          else if(canMaintainSortOrder)
            {
              // If we can maintain the sort order, lets do it.
              sortOrderType = ESP_NO_SORT_SOT;
            }
          else if (usePapa AND NOT usingSynchronousAccess)
            {
              // If we can claim DP_SOT, lets do it.
              sortOrderType = DP2_SOT;
              dp2SortOrderPartFunc = sppOfChild->getDp2SortOrderPartFunc();
            }
          else
            {
              // If all else fail, we cannot claim we are sorted.
              sortOrderType = NO_SOT;
            }
        }
      else // no required order or arrangement
        {
          // Since there is no required order or arrangement,
          // we want to set the sort order type to ESP_NO_SORT if
          // we are not accessing ALL partitions asynchronously.
          // NOTE. Here we can create a potential problem that
          // sortKey and sortOrderType will conflict requirements
          // for the parent merge union.

          if (((NOT usePapa) OR
               usingSynchronousAccess) AND
              canMaintainSortOrder)
            {
              // If we can maintain the sort order with no extra effort,
              // lets do it.
              sortOrderType = ESP_NO_SORT_SOT;
            }
          else if (usePapa AND NOT usingSynchronousAccess)
            {
              // If we can claim DP_SOT, lets do it.
              sortOrderType = DP2_SOT;
              dp2SortOrderPartFunc = sppOfChild->getDp2SortOrderPartFunc();
            }
          else
            {
              // If all else fail, we cannot claim we are sorted.
              sortOrderType = NO_SOT;
            }
        } // end if no required order or arrangement
    } // end if there is a synthesized sort order

  // For all cases there should have been a nodemap synthesized.
  //
  CMPASSERT(myNodeMap);

  // --------------------------------------------------------------------
  // Make a copy of this Exchange's partitioning function and replace the
  // partitioning function's node map with the newly synthesized logical
  // node map.
  // --------------------------------------------------------------------
  // Note that this makes a deep copy of the partitioning function,
  // including the nodeMap. Then this copied nodeMap is replaced with
  // the new nodeMap.  May want to prevent the original nodemap from
  // being copied.

  myPartFunc = myPartFunc->copy();
  myPartFunc->replaceNodeMap(myNodeMap);

  return synthPhysicalPropertyFinalize
    (myContext,
     myPartFunc,
     sortOrderType,
     childPartFunc,
     sppOfChild,
     rppForMe,
     dp2SortOrderPartFunc);
} // Exchange::synthPhysicalPropertyDP2()

//==============================================================================
// Synthesize physical properties for exchange operator's current plan
// extracted from a specified context when child executes in ESP.
//
// Helper method for Exchange::synthPhysicalProperty()
//==============================================================================
PhysicalProperty*
Exchange::synthPhysicalPropertyESP(const Context *myContext)
{
  const PhysicalProperty *sppOfChild = myContext->getPhysicalPropertyOfSolutionForChild(0);
  PartitioningFunction *childPartFunc = sppOfChild->getPartitioningFunction();

  // -------------------------------------------------------------
  // ESP exchange, realize the partitioning requirement
  // -------------------------------------------------------------
  PartitioningRequirement* softPartRequirement = NULL;
  // some dummy variables for the cover test
  ValueIdSet newInputs;
  ValueIdSet referencedInputs;
  ValueIdSet coveredSubExpr;
  ValueIdSet uncoveredExpr;

  // We may need to use the child's partitioning key columns
  // (the "soft" partitioning requirement to realize()) for
  // repartitioning, if the requirement did not specify a required
  // partitioning key. So, we must make sure they are covered
  // by the group attributes. Otherwise, we would attempt to
  // repartition on a value that DP2 did not produce, and this
  // would fail. If we don't pass a soft partitioning
  // requirement to realize(), it will repartition on all
  // the group attribute outputs, if the requirement does
  // not specify a required partitioning key.

  const ReqdPhysicalProperty *rppForMe = myContext->getReqdPhysicalProperty();
  PartitioningRequirement *myPartReq = rppForMe->getPartitioningRequirement();

  // we don't need to require coverage of child partitioning key if
  // requirement is fuzzy and doea not have partitioning key. This means
  // we could use current partitioning of the child to satisfy this
  // requirement, therefore we can use child partitioning as soft requirement.
  // comp_bool_126 should be ON to avoid coverage check.
  NABoolean noNeedToCoverChildPartKey = myPartReq AND
    myPartReq->isRequirementFuzzy() AND
    myPartReq->getPartitioningKey().isEmpty();

  // We use the partial partfunc from the child here because when the
  // rppForMe contains a solid skew requirement, then the child partfunc
  // will be guaranteed to be a SkewedData partfunc. Otherwise, the child
  // partfunc will be some other type and its partial partfunc will be the
  // same as the child part func itself. In both cases, the partial
  // partfunc is sufficient to provide a base for the coverage test below.
  if( ((CmpCommon::getDefault(COMP_BOOL_126) == DF_ON) AND
        noNeedToCoverChildPartKey) OR
      (childPartFunc->getPartialPartitioningKey().isCovered(
                                                    newInputs,
                                                    *(getGroupAttr()),
                                                    referencedInputs,
                                                    coveredSubExpr,
                                                    uncoveredExpr))
    )
  {
     softPartRequirement = childPartFunc->makePartitioningRequirement();
  }

  PartitioningFunction *myPartFunc =
    myPartReq->realize(myContext,
                       FALSE,
                       softPartRequirement);

  myPartFunc->createPartitioningKeyPredicates();

  // --------------------------------------------------------------------
  // Make a copy of this Exchange's partitioning function.
  // --------------------------------------------------------------------
  myPartFunc = myPartFunc->copy();


  // Double check or memorize the skewness handling requirement
  // in the part func for exchange so that the right code can be
  // generated for it.
  //
  // We do this only for partfuncs that can deal with skewed values. For
  // others, they will fail the :satisfied() method because they are not
  // SkewedDataPartFunc
  if ( myPartReq -> isRequirementFuzzy() AND
       myPartFunc -> castToSkewedDataPartitioningFunction()
  ) {
    if (NOT (myPartReq ->castToFuzzyPartitioningRequirement()->
               getSkewProperty()).isAnySkew())
    {
      CMPASSERT(myPartFunc->castToSkewedDataPartitioningFunction()->
                 getSkewProperty() ==
              myPartReq->castToFuzzyPartitioningRequirement()->
                 getSkewProperty());
    } else {
      // "Any" skew can be present in myPartReq if this exchange node
      // receives an ApproxNPart requirement and is located immediately
      // above a skew insensitive hash join node. For exmaple, the following
      // query
      //
      //    select name, mysk.ssn, age from mysk, mydm1
      //     where mysk.ssn = mydm1.ssn order by mysk.ssn;
      //
      // will place a SORT node on top of the de-skewing exchange node.
      // Since no skew requirement is present in myPartReq, we set the
      // skew property for my partfunc to "Any". This can save the effort
      // to repartition the skew data.
       ((SkewedDataPartitioningFunction*)(myPartFunc)) ->
           setSkewProperty(myPartReq->castToFuzzyPartitioningRequirement()->getSkewProperty());
    }
  }



  // --------------------------------------------------------------------
  // If the partitioning requirement is a fully specified part func,
  // then use its nodeMap, otherwise, replace the partitioning
  // function's node map with the newly synthesized logical node map.
  // --------------------------------------------------------------------
  if (CmpCommon::getDefault(COMP_BOOL_82) == DF_ON) {
    myPartFunc->useNodeMapFromReqOrChild(myPartReq, childPartFunc,
                                         TRUE/*forESP*/);
    // "float" single partition ESPs
    NodeMap* mynodemap = (NodeMap*)(myPartFunc->getNodeMap());
    if (mynodemap->getNumEntries() == 1 &&
        CmpCommon::getDefault(COMP_BOOL_83) == DF_ON) {
      mynodemap->setToRandCPU(0);
    }
  }
  else {
    if(!(myPartReq->isRequirementFullySpecified() &&
         (CmpCommon::getDefault(COMP_BOOL_87) != DF_ON) &&
         myPartFunc->getNodeMap() &&
         (myPartFunc->getNodeMap()->getNumEntries() ==
          (ULng32)myPartFunc->getCountOfPartitions()))) {
      Lng32 myPartitionCount = myPartFunc->getCountOfPartitions();

      const NodeMap* childNodeMap = childPartFunc->getNodeMap();

      // Synthesize a nodemap based on the nodemap of the child and the
      // desired number of ESPs.  Using synthesizeLogicalMap() assumes
      // that the lower and upper ESPs are associated via grouping.  This
      // assumption is not valid when considering the communication
      // patterns between the upper and lower ESPs, but this assumption
      // will lead to a reasonable nodemap for the upper ESPs.
      // "float" single partition ESPs.
      //
      NodeMap *myNodeMap =
        ((NodeMap *)(childNodeMap))->synthesizeLogicalMap
        (myPartitionCount, TRUE/*forESP*/);

      CMPASSERT(myNodeMap);

      for(CollIndex i = 0; i < (CollIndex)myPartitionCount; i++) {
        myNodeMap->setPartitionState(i, NodeMapEntry::ACTIVE);
      }

      CMPASSERT(myNodeMap->getNumActivePartitions() == (CollIndex)myPartitionCount);

      myPartFunc->replaceNodeMap(myNodeMap);
    }
  }

  return synthPhysicalPropertyFinalize
    (myContext,
     myPartFunc,
     sppOfChild->getSortOrderType(),
     childPartFunc,
     sppOfChild,
     rppForMe,
     sppOfChild->getDp2SortOrderPartFunc());

} // Exchange::synthPhysicalPropertyESP()

//==============================================================================
// Helper method for Exchange::synthPhysicalProperty()
// Synthesize those physical properties for exchange operator's that are common
// to both case (child executes in DP2 and child executes in DP2)
//
//==============================================================================
PhysicalProperty*
Exchange::synthPhysicalPropertyFinalize(const Context *myContext,
                                        PartitioningFunction *myPartFunc,
                                        SortOrderTypeEnum sortOrderType,
                                        PartitioningFunction *childPartFunc,
                                        const PhysicalProperty *sppOfChild,
                                        const ReqdPhysicalProperty *rppForMe,
                                        PartitioningFunction* dp2SortOrderPartFunc)
{
  // ---------------------------------------------------------------------
  // Determine if a merge of sorted streams is required.
  // A merge of sorted streams is required if there is a order to
  // preserve whose sort order type is not DP2, and there is a required
  // order or arrangement, and the child is in DP2 and is being
  // accessed via a PAPA node (i.e. asynchronous access) or the child
  // is not in DP2 and has more than one partition.
  //
  //
  // NOTE: once we support clustering keys that are other than
  // partitioning keys we MUST use a merge of sorted streams in order
  // to maintain the order of multiple partitions. A PA node can only
  // maintain the order of multiple partitions via synchronous access
  // if a prefix of the clustering key is the partitioning key.
  // Do this check in the DP2 scan node when determining whether to
  // use a PAPA.
  // ---------------------------------------------------------------------
  const LogPhysPartitioningFunction *logPhysChildPartFunc =
    childPartFunc->castToLogPhysPartitioningFunction();

  NABoolean mustMergeSortedStreamsToPreserveOrder =
   ((sortOrderType != DP2_SOT) AND
    (((logPhysChildPartFunc != NULL) AND logPhysChildPartFunc->getUsePapa()) OR
     ((logPhysChildPartFunc == NULL) AND
      (childPartFunc->getCountOfPartitions() > 1))
    )
   );

  NABoolean mergesSortedStreams =
    sppOfChild->isSorted() AND
    (rppForMe->getSortKey() OR rppForMe->getArrangedCols()) AND
    mustMergeSortedStreamsToPreserveOrder;

  // Synthesize a sort key for the exchange if a sort key exists, and
  // either we can do so without doing a merge of sorted streams or we
  // must do so because it is required.

  ValueIdList  mySortKey;

  if (sppOfChild->isSorted() AND
      (NOT mustMergeSortedStreamsToPreserveOrder OR
       (rppForMe->getSortKey() OR rppForMe->getArrangedCols())))
  {
    mySortKey = sppOfChild->getSortKey();
  }
  else
  {
    // If we are not going to synthesize a sort key, then we must
    // make sure we don't synthesize a sort order type or a
    // dp2SortOrderPartFunc.
    sortOrderType = NO_SOT;
    dp2SortOrderPartFunc = NULL;
  }

  // this should be TRUE if we are compiling for transform
  NABoolean maintainOrderAfterRepartitioning =
    (CmpCommon::getDefault(COMP_BOOL_164) == DF_ON);

  PartitioningFunction * childLogPartFunc = childPartFunc;

  if(logPhysChildPartFunc)
    childLogPartFunc = logPhysChildPartFunc->getLogPartitioningFunction();

  // set this flag to FALSE, this is a defensive measure
  // in case the code does not actually go through the if
  // condition below.
  hash2RepartitioningWithSameKey_ = FALSE;

  // hash2RepartitioningWithSameKey_ should be TRUE if the repartitioning
  //  operation is just repartitioning to a different # of partitions
  // but the partitioning key is the same. Also the partitioning should be
  // a hash2 partitioning
  if (childLogPartFunc &&
      (myPartFunc->getPartitioningFunctionType()==
       childLogPartFunc->getPartitioningFunctionType()) &&
      myPartFunc->castToHash2PartitioningFunction() &&
      maintainOrderAfterRepartitioning)
  {
    const Hash2PartitioningFunction *myHash2PartFunc =
      myPartFunc->castToHash2PartitioningFunction();

    const Hash2PartitioningFunction *childHash2PartFunc =
      childLogPartFunc->castToHash2PartitioningFunction();

    ValueIdList myPartKeyList = myHash2PartFunc->
                                  getKeyColumnList();
    ValueIdList childPartKeyList = childHash2PartFunc->
                                     getKeyColumnList();

    if (myPartKeyList.entries() == childPartKeyList.entries())
    {
      hash2RepartitioningWithSameKey_ = TRUE;

      // compare the key columns and their order
      for (CollIndex i = 0; i < myPartKeyList.entries(); i++)
      {
        if (myPartKeyList[i] != childPartKeyList[i])
          hash2RepartitioningWithSameKey_ = FALSE;

        if (!(myHash2PartFunc->getOriginalKeyColumnList()[i].getType() ==
                childHash2PartFunc->getOriginalKeyColumnList()[i].getType()))
          hash2RepartitioningWithSameKey_ = FALSE;
      }
    }
  }

  // ---------------------------------------------------------------------
  // determine my location and data source (can execute in master only
  // if top part function produces a single partition)
  // ---------------------------------------------------------------------

  PlanExecutionEnum           location;
  DataSourceEnum              dataSource;

  if (myPartFunc->getCountOfPartitions() == 1)
  {
    location   = EXECUTE_IN_MASTER_AND_ESP;
    dataSource = SOURCE_ESP_INDEPENDENT;
  }
  else
  {
    location   = EXECUTE_IN_ESP;
    dataSource = SOURCE_ESP_DEPENDENT;
  }

  if (sppOfChild->executeInDP2())
  {
    dataSource = SOURCE_PERSISTENT_TABLE;
  }
  else
  {
    // -----------------------------------------------------------------
    // Avoid potential deadlocks for the following cases of merging of
    // sorted streams in an ESP exchange:
    //
    // - Don't merge if there are multiple sources and multiple merging
    //   ESPs. Skew in the distribution could lead to a deadlock that
    //   would involve at least 4 ESPs.
    // - Don't even merge with a single merging ESP if the source data
    //   is not being produced from independent sources. Again, skew
    //   in the merge process could cause a deadlock that would
    //   involve the data source, the repartitioning ESPs, and the
    //   single merging ESP.
    // This check is kind of late, a partial check could also be done
    // in the topMatch method if necessary for performance reasons.
    // -----------------------------------------------------------------
    if (mergesSortedStreams AND
        ((myPartFunc->getCountOfPartitions() > 1 AND
          sppOfChild->getCountOfPartitions() > 1)
         OR
         sppOfChild->getDataSourceEnum() == SOURCE_ESP_DEPENDENT) AND
         (!hash2RepartitioningWithSameKey_))
    {
      mySortKey.clear();
      // If we are not going to synthesize a sort key, then we must
      // make sure we don't synthesize a sort order type or a
      // dp2SortOrderPartFunc.
      sortOrderType = NO_SOT;
      dp2SortOrderPartFunc = NULL;
    }
  }

  // This is to prevent possible deadlock for parallel merge join.
  // If child got repartitioned 1:n then Sort will be needed to satisfy
  // order or arrangement. See solution 10-051219-3501
  if ( (CmpCommon::getDefault(DESTROY_ORDER_AFTER_REPARTITIONING) == DF_ON) AND
       (myPartFunc->getCountOfPartitions() > 1 AND
        sppOfChild->getCountOfPartitions() == 1)
     )
  {
    if(!hash2RepartitioningWithSameKey_)
    {
      mySortKey.clear();
      // If we are not going to synthesize a sort key, then we must
      // make sure we don't synthesize a sort order type or a
      // dp2SortOrderPartFunc.
      sortOrderType = NO_SOT;
      dp2SortOrderPartFunc = NULL;
    }
  }

  // ---------------------------------------------------------------------
  // Now put it all together.
  // ---------------------------------------------------------------------
  PhysicalProperty *sppForMe = new (CmpCommon::statementHeap())
    PhysicalProperty(mySortKey,
                     sortOrderType,
                     dp2SortOrderPartFunc,
                     myPartFunc,
                     location,
                     dataSource,
                     NULL,
                     NULL);

  // -----------------------------------------------------------------------
  //  The number of cpus after an exchange boundary must be the
  //  total number of cpus in the system
  //
  //  BGZ451: no change for Linux as the logic below implements 
  //  the above statement.
  // -----------------------------------------------------------------------
  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  Lng32 smpVal  = defs.getAsLong(DEF_NUM_SMP_CPUS);
  Lng32 nodeVal=  gpClusterInfo->numOfSMPs();

  if (CURRSTMT_OPTDEFAULTS->isFakeHardware())
  {
     nodeVal = defs.getAsLong(DEF_NUM_NODES_IN_ACTIVE_CLUSTERS);
  }

  const CostScalar totalCPUsInCluster =
                             MAXOF((smpVal*nodeVal),1.);

  sppForMe->setCurrentCountOfCPUs((Lng32)totalCPUsInCluster.getValue());

  // transfer the onePartitionAccess flag to my physical property.
  // We check the flag in HashJoin::computeOperatorPriority().
  if (sppOfChild->executeInDP2())
    sppForMe->setAccessOnePartition(sppOfChild->getAccessOnePartition());

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe;

} // Exchange::synthPhysicalPropertyFinalize()

//<pb>
// ---------------------------------------------------------------------
// A method that interprets the CONTROL QUERY SHAPE ... to decide
// whether a matching partitions plan or a replicate child1 plan
// is desired by the user.
// ---------------------------------------------------------------------
JoinForceWildCard::forcedPlanEnum Join::getParallelJoinPlanToEnforce
              (const ReqdPhysicalProperty* const rppForMe) const
{
  if (rppForMe AND rppForMe->getMustMatch())
    {
      OperatorTypeEnum op = rppForMe->getMustMatch()->getOperatorType();
      JoinForceWildCard *wc;

      // check whether a join is being forced, this is indicated
      // by a bunch of opcodes representing different shapes in
      // CONTROL QUERY SHAPE
      switch (op)
        {
        case REL_FORCE_JOIN:
        case REL_FORCE_NESTED_JOIN:
        case REL_FORCE_MERGE_JOIN:
        case REL_FORCE_HASH_JOIN:
	case REL_FORCE_ORDERED_HASH_JOIN:
	case REL_FORCE_HYBRID_HASH_JOIN:
	case REL_FORCE_ORDERED_CROSS_PRODUCT:
          wc = (JoinForceWildCard *) rppForMe->getMustMatch();
          return wc->getPlan();
        default:
          // something else is being forced
          return JoinForceWildCard::ANY_PLAN;
        }
    }
  else
    return JoinForceWildCard::ANY_PLAN;
} // Join::getParallelJoinPlanToEnforce()


DefaultToken Join::getParallelControlSettings (
            const ReqdPhysicalProperty* const rppForMe, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) const
{
  DefaultToken result;
  Lng32 forcedNumOfESPs = ANY_NUMBER_OF_PARTITIONS;

  // Check for the number of ESPs being forced.
  if (rppForMe->getMustMatch())
  {
    JoinForceWildCard* mustMatch =
      (JoinForceWildCard *)rppForMe->getMustMatch();
    forcedNumOfESPs = mustMatch->getNumOfEsps();
  }

  if (forcedNumOfESPs > 0)
  {
    numOfESPs = forcedNumOfESPs;
    // If the number of ESPs is being forced, then we should always
    // attempt ESP parallelism, no matter what, so no need to do
    // any more checks. This is true even if only one ESP is being
    // forced - in this case, we need to attempt ESP parallelism
    // with exactly one ESP, in order to guarantee we get only one ESP.
    numOfESPsForced = TRUE;
    // If the number of ESPs are being forced, then we must use
    // exactly that many, so the allowable deviation is 0.
    allowedDeviation = 0.0;
    result = DF_ON;
  }
  else // Number of ESPs is not being forced.
  {
    result = RelExpr::getParallelControlSettings(rppForMe,
                                                 numOfESPs,
                                                 allowedDeviation,
                                                 numOfESPsForced);
  } // end if the number of ESPs is not being forced

  return result;

} // Join::getParallelControlSettings()

// -----------------------------------------------------------------------
// Split any sort or arrangement requirements between the left and
// right children. Add those that are for the left child to the
// passed in requirement generator object, and return those that
// are for the right child.
// -----------------------------------------------------------------------
void Join::splitSortReqsForLeftChild(
  const ReqdPhysicalProperty* rppForMe,
  RequirementGenerator &rg,
  ValueIdList& reqdOrder1,
  ValueIdSet& reqdArr1) const
{
    CMPASSERT(isNestedJoin() OR
	    (getOperatorType() == REL_HYBRID_HASH_JOIN AND
	    ((HashJoin *) this)->isOrderedCrossProduct()));

  // If there is a required sort order and/or arrangement, then
  // split off any part of the requirement that is for the
  // right child and only pass on the portion of the requirement
  // that is for the left child.
  if (rppForMe->getSortKey() AND rppForMe->getSortKey()->entries() > 0)
  {
    ValueIdList reqdOrder0;
    if (splitOrderReq(*(rppForMe->getSortKey()),reqdOrder0,reqdOrder1))
    {
      rg.removeSortKey();
      // Sort Order type and dp2SortOrderPartReq might have been
      // removed by the call above, so get them again from the rpp
      SortOrderTypeEnum childSortOrderTypeReq =
        rppForMe->getSortOrderTypeReq();
      PartitioningRequirement* childDp2SortOrderPartReq =
        rppForMe->getDp2SortOrderPartReq();
      rg.addSortKey(reqdOrder0,
                    childSortOrderTypeReq,
                    childDp2SortOrderPartReq);
    }
  } // end if required order
  if (rppForMe->getArrangedCols() AND
      rppForMe->getArrangedCols()->entries() > 0)
  {
    ValueIdSet reqdArr0;
    if (splitArrangementReq(*(rppForMe->getArrangedCols()),
                            reqdArr0,reqdArr1))
    {
      rg.removeArrangement();
      // Sort Order type and dp2SortOrderPartReq might have been
      // removed by the call above, so get them again from the rpp
      SortOrderTypeEnum childSortOrderTypeReq =
        rppForMe->getSortOrderTypeReq();
      PartitioningRequirement* childDp2SortOrderPartReq =
        rppForMe->getDp2SortOrderPartReq();
      rg.addArrangement(reqdArr0,
                        childSortOrderTypeReq,
                        childDp2SortOrderPartReq);
    }
  } // end if required arrangement

} // Join::splitSortReqsForLeftChild

// -----------------------------------------------------------------------
// Split any sort or arrangement requirements between the left and
// right children. Add those that are for the right child to the
// passed in requirement generator object, and return those that
// are for the left child.
// -----------------------------------------------------------------------
void Join::splitSortReqsForRightChild(
  const ReqdPhysicalProperty* rppForMe,
  RequirementGenerator &rg,
  ValueIdList& reqdOrder0,
  ValueIdSet& reqdArr0) const
{
    CMPASSERT(isNestedJoin() OR
	    (getOperatorType() == REL_HYBRID_HASH_JOIN AND
	    ((HashJoin *) this)->isOrderedCrossProduct()));

  // If there is a required sort order and/or arrangement, then
  // split off any part of the requirement that is for the
  // left child and only pass on the portion of the requirement
  // that is for the right child.
  if (rppForMe->getSortKey() AND
      rppForMe->getSortKey()->entries() > 0)
  {
    ValueIdList reqdOrder1;
    if (splitOrderReq(*(rppForMe->getSortKey()),
                      reqdOrder0,reqdOrder1))
    {
      // Get the sort order type again from the rpp
      SortOrderTypeEnum childSortOrderTypeReq =
        rppForMe->getSortOrderTypeReq();
      // If the sort order type requirement is DP2 or
      // DP2_OR_ESP_NO_SORT, then convert this for the right
      // child to ESP_NO_SORT. We only pass the dp2SortOrderPartReq
      // to the left child.
      if ((childSortOrderTypeReq == DP2_SOT) OR
          (childSortOrderTypeReq == DP2_OR_ESP_NO_SORT_SOT))
      {
        childSortOrderTypeReq = ESP_NO_SORT_SOT;
      }
      rg.addSortKey(reqdOrder1,childSortOrderTypeReq,NULL);
    }
  } // end if required order
  if (rppForMe->getArrangedCols() AND
      rppForMe->getArrangedCols()->entries() > 0)
  {
    ValueIdSet reqdArr1;
    if (splitArrangementReq(*(rppForMe->getArrangedCols()),
                            reqdArr0,reqdArr1))
    {
      // Get the sort order type again from the rpp
      SortOrderTypeEnum childSortOrderTypeReq =
        rppForMe->getSortOrderTypeReq();
      // If the sort order type requirement is DP2 or
      // DP2_OR_ESP_NO_SORT, then convert this for the right
      // child to ESP_NO_SORT. We only pass the dp2SortOrderPartReq
      // to the left child.
      if ((childSortOrderTypeReq == DP2_SOT) OR
          (childSortOrderTypeReq == DP2_OR_ESP_NO_SORT_SOT))
      {
        childSortOrderTypeReq = ESP_NO_SORT_SOT;
      }
      rg.addArrangement(reqdArr1,childSortOrderTypeReq,NULL);
    }
  } // end if required arrangement
} // Join::splitSortReqsForRightChild

//<pb>

// -----------------------------------------------------------------------
// member functions for class NestedJoin
// -----------------------------------------------------------------------

PlanWorkSpace * NestedJoin::allocateWorkSpace() const
{
  return new(CmpCommon::statementHeap()) NestedJoinPlanWorkSpace(getArity());
}

// -----------------------------------------------------------------------
// NestedJoin::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
NestedJoin::costMethod() const
{
  static THREAD_P CostMethodNestedJoin *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodNestedJoin();
  return m;
}

// -----------------------------------------------------------------------
// Generate the partitioning requirements for the left child of a
// preferred probing order nested join plan, or a NJ that is for a
// write operation. In both cases, we need to base the partitioning
// requirement for the left child on the partitioning of the right
// child to ensure that we get a Type-1 join. If the requirements could
// not be generated because the user is attempting to force something that
// is not possible, the method returns FALSE. Otherwise, it returns TRUE.
// -----------------------------------------------------------------------
NABoolean NestedJoin::genLeftChildPartReq(
            Context* myContext, // IN
            PlanWorkSpace* pws, // IN
            const PartitioningFunction* physicalPartFunc,  // IN
            PartitioningRequirement* &logicalPartReq) // OUT
{
  const ReqdPhysicalProperty* rppForMe =
          myContext->getReqdPhysicalProperty();
  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();
  Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
  float childNumPartsAllowedDeviation = 0.0;
  NABoolean numOfESPsForced = FALSE;
  NABoolean createPartReqForChild = TRUE;



  // Get some values from the defaults table
  NABoolean baseNumPartsOnAP = FALSE;
  if (CmpCommon::getDefault(BASE_NUM_PAS_ON_ACTIVE_PARTS) == DF_ON)
    baseNumPartsOnAP = TRUE;

  NABoolean isUpdateOfHBaseTable = updateTableDesc() &&
       updateTableDesc()->getNATable()->isHbaseTable();

  //adjust plan for hbase bulk load:
  // -use range partitioning for salted tables
  // -use hash on primary key for non salted tables
  NABoolean adjustPFForTrafBulkLoadPrep = isUpdateOfHBaseTable &&
                                         getIsForTrafLoadPrep() &&
                                         CmpCommon::getDefault(TRAF_LOAD_PREP_ADJUST_PART_FUNC) == DF_ON ;//&&
                                         //!updateTableDesc()->getClusteringIndex()->getNAFileSet()->hasSyskey();

  if (adjustPFForTrafBulkLoadPrep)
  {
    baseNumPartsOnAP = TRUE;
  }
  Lng32 numActivePartitions = 1;
  if (baseNumPartsOnAP)
  {
    if ((physicalPartFunc == NULL) OR
        (physicalPartFunc->castToRangePartitioningFunction() == NULL))
      baseNumPartsOnAP = FALSE;
    else
    {
      CostScalar activePartitions =
        ((NodeMap *)(physicalPartFunc->getNodeMap()))->getNumActivePartitions();
      numActivePartitions = (Lng32)activePartitions.getValue();
    }
  }

  NABoolean okToAttemptESPParallelismFlag =
       okToAttemptESPParallelism(myContext,
                                 pws,
                                 childNumPartsRequirement,
                                 childNumPartsAllowedDeviation,
                                 numOfESPsForced);

  ValueIdList pkList;

  // notice if parent requires a particular number of partitions
  if ((partReqForMe != NULL) AND
      (partReqForMe->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS) AND
      !adjustPFForTrafBulkLoadPrep)
  {
    if (NOT numOfESPsForced)
      childNumPartsRequirement = partReqForMe->getCountOfPartitions();
    // don't ask for more ESPs than there are target partitions,
    // the ESPs must be a grouping of that partitioning scheme. Also,
    // can't change the number of partitions at all if we are in DP2.
    if (physicalPartFunc == NULL)
    {
      // If the user was forcing the number of ESPs and it
      // was not 1, then we cannot honor the request and so we
      // must give up now.
      if (numOfESPsForced AND (childNumPartsRequirement != 1))
        return FALSE;
      else
        childNumPartsRequirement = 1;
    }
    else if ((childNumPartsRequirement >
              physicalPartFunc->getCountOfPartitions()) OR
             rppForMe->executeInDP2())
    {
      // If the user was forcing the number of ESPs, then we
      // cannot honor the request and so we must give up now.
      if (numOfESPsForced)
        return FALSE;
      childNumPartsRequirement = physicalPartFunc->getCountOfPartitions();
    }
    // If childNumPartsRequirement no longer satisfies the original requirement
    // then give up now.
    if (NOT
          ((childNumPartsRequirement == partReqForMe->getCountOfPartitions()) OR
           ((partReqForMe->castToRequireApproximatelyNPartitions() != NULL) AND
              partReqForMe->castToRequireApproximatelyNPartitions()->
                isPartitionCountWithinRange(childNumPartsRequirement))))
      return FALSE;
  }
  else if (okToAttemptESPParallelismFlag)
  {
    // don't ask for more ESPs than there are target partitions,
    // the ESPs must be a grouping of that partitioning scheme
    if (isUpdateOfHBaseTable)
    {
      if (adjustPFForTrafBulkLoadPrep )
      {
        if (baseNumPartsOnAP && numActivePartitions >1 )
        {
          // this case will use base number of parts on Active Partitions and handle the case
          //of partitioned table. Condition on syskey is removed based on comment from
          //previous delivery
          childNumPartsRequirement = numActivePartitions;
        }
        else if (numActivePartitions ==1 && childNumPartsRequirement > 1 )
        {
          //if table is not partitioned we use the clustering index to produce a hash2 partition
          //function based on the clustering key. one thing to mention and is useful here
          //is that rows with same keys will go to the same esp --> can be used for deduping
          //Condition on syskey is removed based on comment from
          //previous delivery
          pkList.insert(updateTableDesc()->getClusteringIndex()->getClusteringKeyCols());
          physicalPartFunc = new(CmpCommon::statementHeap())
                Hash2PartitioningFunction(pkList, pkList,childNumPartsRequirement);
          createPartReqForChild = TRUE;
        }
        else
        {
          //if none of the 2 cases apply then used the default random partitioning
          ValueIdSet partKey;
          ItemExpr *randNum =
            new(CmpCommon::statementHeap()) RandomNum(NULL, TRUE);
          randNum->synthTypeAndValueId();
          partKey.insert(randNum->getValueId());
          physicalPartFunc = new(CmpCommon::statementHeap())
              Hash2PartitioningFunction (partKey, partKey, childNumPartsRequirement);
        }
     }
     else
        // HBase tables can be updated in any way, no restrictions on 
        // parallelism
        if (numOfESPsForced && physicalPartFunc)
          childNumPartsRequirement = numOfESPsForced;
        else
          createPartReqForChild = FALSE;
    }
    else if (physicalPartFunc == NULL)
    {
      // If the user was forcing the number of ESPs and it
      // was not 1, then we cannot honor the request and so we
      // must give up now.
      if (numOfESPsForced AND (childNumPartsRequirement != 1))
        return FALSE;
      else
        childNumPartsRequirement = 1;
    }
    else if ((baseNumPartsOnAP AND
              (childNumPartsRequirement > numActivePartitions) AND
              NOT numOfESPsForced) OR
             (childNumPartsRequirement >
              physicalPartFunc->getCountOfPartitions()))
    {
      // If the user was forcing the number of ESPs, then we
      // cannot honor the request and so we must give up now.
      if (numOfESPsForced)
        return FALSE;
      else
      {
        // The preferred level of parallelism is more than the number
        // of active/physical partitions, which is undesirable/not allowed.
        // Lower the level of parallelism to the number of active
        // partitions/physical partitions.
        if (baseNumPartsOnAP)
          childNumPartsRequirement = numActivePartitions;
        else
          childNumPartsRequirement = physicalPartFunc->getCountOfPartitions();
      }
    }
  } // end if ok to try a parallel plan
  else
  {
    if ( rppForMe->executeInDP2() AND (physicalPartFunc != NULL) )
    {
       // If we are in DP2, then we cannot change the number of partitions
       // at all.
       childNumPartsRequirement = physicalPartFunc->getCountOfPartitions();
    } else
       childNumPartsRequirement = 1;
  }

  // now create the partitioning requirement to match the target table
  if (createPartReqForChild == FALSE)
    logicalPartReq = NULL;
  else if (childNumPartsRequirement == 1)
  {
    logicalPartReq = new(CmpCommon::statementHeap())
                         RequireExactlyOnePartition();
  }
  else
  {
    // Take the target table's partitioning function and scale it down
    // to the appropriate size.
    PartitioningFunction *cpf = physicalPartFunc->copy();
    // If we haven't estimated the active parts yet, all parts will be active.
    // So, both grouping distribution algorithms are the same in this case.
    Lng32 scaleNumPartReq = childNumPartsRequirement;
    cpf = cpf->scaleNumberOfPartitions(scaleNumPartReq);
    // Was scale able to do it's job?
    if ((NOT cpf->isAGroupingOf(*physicalPartFunc)) &&
        (scaleNumPartReq != childNumPartsRequirement)) // No
    {
      // If scale didn't work (it should have), are only choice is 1 part.
      logicalPartReq = new(CmpCommon::statementHeap())
                           RequireExactlyOnePartition();
    }
    else // Yes
    {
      logicalPartReq = cpf->makePartitioningRequirement();
    }
  } //end if childNumPartsRequirement == 1

  return TRUE;

} // NestedJoin::genLeftChildPartReq()

// -----------------------------------------------------------------------
// Generate a preferred probing order sort requirement for the left child
// of a nested join that is for a write operation - i.e. Insert, Update,
// or Delete. The generated sort requirement is returned.
// -----------------------------------------------------------------------
ValueIdList NestedJoin::genWriteOpLeftChildSortReq()
{
  ValueIdList reqdOrder;

  // Only generate a preferred probing order requirement if there is
  // no required order stored in the join node. This is because the
  // first nested join plan already required this order, and so there
  // is no point in doing it again.
  if (getReqdOrder().isEmpty())
  {
    NABoolean requireOrderedWrites;
    // QSTUFF
    if (NOT (getGroupAttr()->isStream() ||
        getGroupAttr()->isEmbeddedUpdateOrDelete()))
    // QSTUFF
      requireOrderedWrites = TRUE;
    else
      requireOrderedWrites = FALSE;

    // Get some of the logical property information so that we can
    // see if it is worth sorting the writes.

    // get # of probes estimate from the left child
    CostScalar noOfProbes =
      child(0).getGroupAttr()->getResultCardinalityForEmptyInput();

    // We do not sort a small number of inserts or user provided data
    // in rowsets.  see Genesis case 10-021021-2615 for details on why
    // rowset data is not sorted.
    // We always want to sort if we need to avoid a potential halloween
    // problem.
    if (!(avoidHalloweenR2() OR (getHalloweenForceSort() == FORCED)
	  OR isTSJForSideTreeInsert() == TRUE
	  OR enableTransformToSTI() == TRUE
       ) AND
        (noOfProbes < 100 OR child(0).getGroupAttr()->getNumBaseTables() == 0))
      {
	// This is used to prevent compiler failure under control
	// query shape with sort enforcement. Because in this case
	// we want to keep requireOrderedWrites as TRUE.
	// It is better to check the CQS on this join (via getMustMatch())
	// and its left child instead of only checking the CQS requirement
	// on the root. This will allow us to focus only on CQS statements
	// that actually force this join left child.
	NABoolean underCQS =
	  ActiveControlDB()->getRequiredShape() &&
	  ActiveControlDB()->getRequiredShape()->getShape() &&
	  NOT ActiveControlDB()->getRequiredShape()->getShape()->isCutOp();
	if (NOT underCQS)
	  requireOrderedWrites = FALSE;
      }
      // for bulk load we require that data is sorted
      if (updateTableDesc() && updateTableDesc()->getNATable()->isHbaseTable() &&
          getIsForTrafLoadPrep() && !requireOrderedWrites)
        requireOrderedWrites = TRUE;

    if (requireOrderedWrites)
    {
      // require the select data to be sorted by the clustering key
      // of the target table
      updateSelectValueIdMap()->rewriteValueIdListDown(
           updateTableDesc()->getClusteringIndex()->getOrderOfKeyValues(),
           reqdOrder);

     // Fix 10-100412-9445.
     // Truncate the trailing columns of reqdOrder starting at the 
     // syskey column (if exist).  Anything beyond the syskey should
     // not be sorted. Note that the order of the sortkey of the inner table
     // and the order of the mapped required sort order on the outer source
     // are the same.
     Int32 sysKeyPosition = updateTableDesc()->getClusteringIndex()
                             ->getNAFileSet()->getSysKeyPosition();

     if ( sysKeyPosition >= 0 ) {
       // truncation anything starting from the sysKey.
       for (Int32 i=reqdOrder.entries()-1; i>= sysKeyPosition; i--) {
          reqdOrder.removeAt(i);
       }
     }


      // Remove from the required order any columns that are equal to
      // constants or input values.
      reqdOrder.removeCoveredExprs(getGroupAttr()->getCharacteristicInputs());
      // Remove from the required order any columns that cannot be provided
      // by left child outputs.
      reqdOrder.removeUnCoveredExprs(child(0).getGroupAttr()->getCharacteristicOutputs());

      // fix for case 10-080826-7916, soln 10-080826-5421
      // make sure reqdOrder has some column(s) to sort on
      // also soln 10-091002-5072. For Halloween queries the
      // sort is used as a blocking operator and therefore
      // it does not matter (for correctness) which column is used for sorting as long 
      // as the sort operator is present. If reqdOrder is empty then a sort node 
      // will not be present as an empty sortOrder is always satisfied. 
      // For performance it does help if the sort order is the same as 
      // clustering key of target side. If we get into the following statement
      // then we know that the source side is not able to provide the target clustering
      // key cols. In all known cases this is because syskey is the only clustering key,
      // however this code has been written generally, without reference to syskey.
      if (reqdOrder.isEmpty() AND 
          (avoidHalloweenR2() OR 
           (getHalloweenForceSort() == FORCED)))
      {
        const ValueIdSet& child0Outputs = 
          child(0).getGroupAttr()->getCharacteristicOutputs();

        for (ValueId exprId = child0Outputs.init();
              child0Outputs.next(exprId);
              child0Outputs.advance(exprId))
        {
          if (!(exprId.getItemExpr()->doesExprEvaluateToConstant(FALSE,TRUE)))
          {
            reqdOrder.insert(exprId);  // do not insert constants. If all outputs are constant
            break ; // then there no need to sort/block
          }
        }
      }
    } // end if reqdOrderedWrites
  } // end if the user did not specify an ORDER BY clause

  // fix for soln 10-090826-4155 
  // make sure reqdOrder has some column(s) to sort on
  if (reqdOrder.isEmpty() AND
      updateTableDesc()->getClusteringIndex()->
                         getNAFileSet()->hasSyskey() AND
      (avoidHalloweenR2() OR 
      (getHalloweenForceSort() == FORCED) OR
      (getHalloweenForceSort() == NOT_FORCED)))
  {
    reqdOrder = ValueIdList
      (child(0).getGroupAttr()->getCharacteristicOutputs());
  }

  return reqdOrder; // Will be empty if no required order

} // NestedJoin::genWriteOpLeftChildSortReq()


PartitioningFunction *
isOneToOneMatchHashPartitionJoinPossible(NestedJoin* nj,
    const PartitioningFunction *leftPartFunc, const ValueIdMap &map)
{
  if (CmpCommon::getDefault(COMP_BOOL_94) == DF_ON) {
    return NULL;
  }

  const PartitioningFunction *origLeftHashPartFunc = leftPartFunc;

  if (leftPartFunc->castToHashDistPartitioningFunction() != NULL) {
    // If dealing with HashDist, then the original number of partitions
    // must match the current number of partitions.

    const TableHashPartitioningFunction *leftHashPartFunc;
    leftHashPartFunc = leftPartFunc->castToHashDistPartitioningFunction();
    
    if (leftHashPartFunc->getCountOfOrigHashPartitions() !=
        leftHashPartFunc->getCountOfPartitions()) {
      return NULL;
    }
  } else {

    // If it is a skewed data part func, get the partition func for the
    // unskewed data.
    if ( leftPartFunc->castToSkewedDataPartitioningFunction() ) {
       leftPartFunc = leftPartFunc->
                        castToSkewedDataPartitioningFunction()->
                            getPartialPartitioningFunction();
    }


    // Check whether this is hash2 partitioning. If not, return FALSE.
    if ( leftPartFunc->castToHash2PartitioningFunction() == NULL )
       return NULL;
  }

  PartitioningFunction *mappedLeftPartFunc = NULL;
  ValueIdMap mapCopy(map);

  // remap the left part func to right
  mappedLeftPartFunc = leftPartFunc->copyAndRemap(mapCopy, FALSE);

  GroupAttributes *ga = nj->child(1).getGroupAttr();
  const SET(IndexDesc *) &availIndexes = ga->getAvailableBtreeIndexes();

  for (CollIndex i = 0; i < availIndexes.entries(); i++) {
    IndexDesc *iDesc = availIndexes[i];

    if (iDesc->isClusteringIndex()) {
      PartitioningFunction *rightPartFunc = iDesc->getPartitioningFunction();
      PartitioningFunction *result = NULL;
      NABoolean allowGrouping = (CmpCommon::getDefault(NESTED_JOINS_OCR_GROUPING) == DF_ON);

      if (rightPartFunc AND
          (
            (
             (allowGrouping
                ? leftPartFunc->isAGroupingOf(*rightPartFunc)
                : (leftPartFunc->comparePartFuncToFunc(*rightPartFunc) == SAME))
            )
                  OR
            (
             mappedLeftPartFunc AND
             (allowGrouping
                ? mappedLeftPartFunc->isAGroupingOf(*rightPartFunc)
                : (mappedLeftPartFunc->comparePartFuncToFunc(*rightPartFunc) == SAME))
            )
          )
        ) {
        if ( origLeftHashPartFunc->castToSkewedDataPartitioningFunction() ) {

              // Require the inner to be rep-n so that both the non-skewed and
              // skewed rows from the outer table can be distributed to the
              // right partitions. Note that the non-skewed and the skewed can
              // reach different partitions, hence the need for a rep-n part func.
              result = new (CmpCommon::statementHeap() )
                ReplicateNoBroadcastPartitioningFunction
                  (rightPartFunc->getCountOfPartitions());
        } else {
          result = mappedLeftPartFunc;
        } 
      } 
      return result;
    }
  }

  // Should not be reachable
  return NULL;
}

// -----------------------------------------------------------------------
// Generate the requirements for the right child of a nested join.
// Returns the generated requirements if they were feasible, otherwise
// NULL is returned.
// sppForChild: Physical property of the left child
// rppForMe:    Required properties of the nested join
// -----------------------------------------------------------------------
ReqdPhysicalProperty* NestedJoin::genRightChildReqs(
                        const PhysicalProperty* sppForChild, // IN
                        const ReqdPhysicalProperty* rppForMe, // IN
                         NABoolean avoidNSquareOpens)
{
  ReqdPhysicalProperty* rppForChild = NULL;

  // ---------------------------------------------------------------
  // spp should have been synthesized for child's optimal plan.
  // ---------------------------------------------------------------
  CMPASSERT(sppForChild != NULL);
  PartitioningFunction* childPartFunc =
                          sppForChild->getPartitioningFunction();

  PartitioningRequirement* partReqForChild;
  RequirementGenerator rg (child(1),rppForMe);

  // Remove any parent requirements for the sort key or arrangement,
  // since most likely these only refer to the left child. But,
  // if there is a portion that refers to the right child, then
  // pass this along.
  if ((rppForMe->getSortKey() AND
       (rppForMe->getSortKey()->entries() > 0)) OR
      (rppForMe->getArrangedCols() AND
       (rppForMe->getArrangedCols()->entries() > 0)))
  {
    rg.removeSortKey();
    rg.removeArrangement();
    if (updateTableDesc() == NULL)
    {
      // If there is a required sort order and/or arrangement, then
      // split off any part of the requirement that is for the
      // left child and only pass on the portion of the requirement
      // that is for the right child.
      ValueIdList reqdOrder0;
      ValueIdSet reqdArr0;
      splitSortReqsForRightChild(rppForMe, rg, reqdOrder0, reqdArr0);

    } // end if NJ is not for a write operation
  } // end if context requires order

  // If the partitioning function of the left child was only
  // one partition, then pass a requirement for that to the right,
  // since no parallelism is possible. Otherwise, pass a
  // replication function without broadcast to the right child.
  //
  // Note that for write operations, we are really doing a Type-1
  // join and so we don't need to pass a replication function -
  // we should just pass the left child part func as the part
  // requirement to the right. But, this would allow a
  // repartitioning plan to be considered. This should never be
  // chosen, since repartitioning is not necessary, but it is
  // too risky to allow the optimizer to even consider it.
  //

  PartitioningFunction *npf = NULL;

  if (childPartFunc->isASinglePartitionPartitioningFunction()) {
          partReqForChild = childPartFunc->makePartitioningRequirement();
  } else 
    if (rppForMe->executeInDP2()) {
      partReqForChild = childPartFunc->makePartitioningRequirement();
  } else {
    if ((npf=isOneToOneMatchHashPartitionJoinPossible(this, childPartFunc, getOriginalEquiJoinExpressions())) && 
      npf->getCountOfPartitions() <= rppForMe->getCountOfPipelines()
     ) {
      partReqForChild = npf->makePartitioningRequirement();
    } else {
      // Use the node map of the left childs partitioning function.
      // (Note: this NestedJoin will have the same partFunc and Nodemap as the left child)
      NABoolean useNodeMap  = (CmpCommon::getDefault(COMP_BOOL_82) == DF_ON);

      PartitioningFunction *mappedChildPartFunc = childPartFunc;
      if (updateTableDesc() && isTSJForWrite()) {
        mappedChildPartFunc = 
          childPartFunc->copyAndRemap(*updateSelectValueIdMap(),
                                      TRUE);
      }

      if ( !avoidNSquareOpens || updateTableDesc() )
         partReqForChild = new (CmpCommon::statementHeap() )
           RequireReplicateNoBroadcast(mappedChildPartFunc, useNodeMap);
      else
         return NULL;
    }
  }


  // Add a push-down requirement based on left child's push
  // down property.
  if ( sppForChild->getPushDownProperty() )
     rg.addPushDownRequirement(
        sppForChild->getPushDownProperty()->makeRequirement());


  // Remove any parent partitioning requirement, since only the left
  // child must satisfy it.
  rg.removeAllPartitioningRequirements();

  // Now, add in the Join's partitioning requirement for the
  // right child.

  // If this nested join is for a write operation and it is in Dp2,
  // then don't need to pass the source partitioning function as a
  // requirement to the target because we have already
  // guaranteed that the two are partitioned the same by passing
  // the target partitioning function to the source as a
  // requirement. Furthermore, if we try to pass it it will fail
  // because the partitioning key columns will not be covered
  // by the target table characteristic outputs because a target
  // table produces no values, i.e. it has no char. outputs.
  //
  if (NOT( updateTableDesc() AND rppForMe->executeInDP2())) 
  {
    rg.addPartRequirement(partReqForChild);
   
    // we add a requirement that ESP-Exchanges are not allowed
    // under right side of a nested join except when cb_106 is on or 
    // (cb_102 is on and rowsets are involved)
    if ( (CmpCommon::getDefault(COMP_BOOL_106) == DF_OFF) &&
         NOT((CmpCommon::getDefault(COMP_BOOL_102) == DF_ON) && isRowsetIterator()) )
      rg.addNoEspExchangeRequirement();
  }

  // Produce the requirements and make sure they are feasible.
  if (rg.checkFeasibility())
  {
    // Produce the required physical properties.
    rppForChild = rg.produceRequirement();

  } // end if the requirements were feasible

  return rppForChild;

} // NestedJoin::genRightChildReqs()

// -----------------------------------------------------------------------
// Generate an input physical property object to contain the sort
// order of the left child and related group attributes and
// partitioning functions.
// -----------------------------------------------------------------------
InputPhysicalProperty* NestedJoin::generateIpp(
                         const PhysicalProperty* sppForChild,
                         NABoolean isPlan0)
{
  InputPhysicalProperty* ipp = NULL;

  // ----------------------------------------------------------------
  // Get the sort order of my left child and pass this information to
  // the context for optimizing my right child.  (Costing the inner
  // table access has a dependency on the order from the outer table).
  // If this is a write op, use the map to map the left child sort
  // key values to their right child equivalents. We can't do this
  // for read because there is no map for read. So, for read, we will
  // have to do some complicated processing in FileScanOptimizer to
  // determine if the orders are equivalent.
  // ----------------------------------------------------------------
  ValueIdList mappedLeftChildOrder(sppForChild->getSortKey());
  ValueIdSet mappedCharOutputs(
               child(0).getGroupAttr()->getCharacteristicOutputs());
  PartitioningFunction* mappedChildPartFunc =
    sppForChild->getPartitioningFunction();
  PartitioningFunction* mappedDp2SortOrderPartFunc =
    sppForChild->getDp2SortOrderPartFunc();

  if (updateTableDesc() != NULL AND
      NOT mappedLeftChildOrder.isEmpty())
  {
    // The control comes here for an Update operation

    // map the char outputs from left to right
    mappedCharOutputs.clear();

    // map the sort key from left to right
    mappedLeftChildOrder.clear();

    NABoolean useListInsteadOfSet = FALSE;
    ValueIdSet childCharOutput = child(0).getGroupAttr()->getCharacteristicOutputs();
    ValueIdList childSortKey = sppForChild->getSortKey();

    // We go through the new logic, only if there are some columns in the bottom list
    // which are duplicated in the characteristics output of the child. An example of this
    // is: Insert * into t1 from (select a, b, a from t2); Here column 'a' of t2
    // is being matched to two columns of t1. Characteristics output of t2 would be
    // only 'a' and 'b'. But we want to have two occurences of 'a' in order to match to
    // the three columns of t1. Hence we work with the list, instead of columns.
    // For all other cases we continue to work with the set.

    // Sol: 10-040416-5166.
    // First thing we do is to eliminate if this nestedJoin is due to create index
    // for this we check if the right child of the nestedJoin is an index_table
    // or a normal_table

    CascadesGroup* group1 = (*CURRSTMT_OPTGLOBALS->memo)[child(1).getGroupId()];

    GenericUpdate* rightChild = (GenericUpdate *) group1->getFirstLogExpr();

    if (rightChild->getTableName().getSpecialType() == ExtendedQualName::NORMAL_TABLE )
    {
      // In this case, we have columns from Scan duplicated
      // Hence the bottom values in updateSelectValueIdMap have some columns
      // appearing twice. When writing the top values in the updateSelectValueIdMap
      // if we use characteristicsOutput then these duplicate values are removed.
      // Hence use the bottom values that are covered by the child to map the
      // top values of the updateSelectValueIdMap. Use list object to allow duplicates.


      ValueIdList bottomValues = updateSelectValueIdMap()->getBottomValues();

      // We go through the new logic, only if there are some columns in the bottom list
      // which are duplicated in the characteristics output of the child. An example of this
      // is: Insert * into t1 from (select a, b, a from t2); Here column 'a' of t2
      // is being matched to two columns of t1. Characteristics output of t2 would be
      // only 'a' and 'b'. But we want to have two occurences of 'a' in order to match to
      // the three columns of t1. Hence we work with the list, instead of columns.
      // For all other cases we continue to work with the set.
      ValueIdSet bottomValuesSet(bottomValues);
      if (bottomValuesSet.entries() != bottomValues.entries())
      {
	ValueIdList mappedCharOPs;
	// from here get all the bottom values that appear in the characteristic
	// output of the child.
	ValueIdList bottomOutputValues = bottomValues;

	bottomOutputValues.findCommonElements(childCharOutput );

	bottomValuesSet = bottomOutputValues;

	// see if the characteristics output contained some expressions. In that case,
	// cannot use bottom values instead of outputs.

	if (bottomValuesSet == childCharOutput)
	{
	  // so we saw that Characteristics Outputs were duplicated.
	  // Check if the bottom values have key columns duplicated.
	  // If not, then we cannot use bottom lists

	  // find common elements between bottom values and the sort key of the Child
	  // we shall use bottomValues only if there were some columns of the
	  // sort key duplicated in the bottom values, and that the duplicate columns
	  // have not been handled in the sortkey. I have found cases where the problem of duplicate
	  // columns has been handled in various ways: 1. By assigning different valueId to the
	  // same column,2. appending _XX to the column name (for populate indexes),3. or retaining the
	  // duplicate columns in the key. Since, I do not understand a lot of cases that can arise
	  // and at this phase of release we do not have time to clean up everything, I will go with
	  // the new logic only if there are duplicate columns in the Select list, but they
	  // have not been handled till now. Sol: 10-040527-6486

	  ValueIdSet keySet(childSortKey);

	  // if there are no duplicates in childSortKey, then only proceed. Else if the
	  // duplicate columns exist in the sortKey, then use those.
	  if (keySet.entries() == childSortKey.entries())
	  {
	    bottomValues = updateSelectValueIdMap()->getBottomValues();

            ValueIdList keyFromBottomList = bottomValues;
            keyFromBottomList.findCommonElements(childSortKey);

	    bottomValuesSet = keyFromBottomList;
	    if (
	      // if there are duplicate columns in the select list
	      (keyFromBottomList.entries() != bottomValuesSet.entries()) &&
	      // and these duplicate columns have single entry in the childSortKey
	      // remember we are here only if there are no duplicates in the
	      // sortKey
	      (keyFromBottomList.entries() > childSortKey.entries() )
	      )
	    {
	      useListInsteadOfSet = TRUE;
	      updateSelectValueIdMap()->rewriteValueIdListUpWithIndex(
		mappedCharOPs,
		bottomOutputValues );

	      mappedCharOutputs = mappedCharOPs;

	      updateSelectValueIdMap()->rewriteValueIdListUpWithIndex(
		mappedLeftChildOrder,
		keyFromBottomList);

	      // If key of the child contained some extra or duplicate columns, then there
	      // could be some redundant columns of the parent picked up.
	      // from whatever topValues you got matching to the sortKey of the child
	      // select the one which is also the orderKey of the target.

	      ValueIdList targetSortKey = updateTableDesc()->getClusteringIndex()->getOrderOfKeyValues();
              ValueIdList mappedLeftChildTmp = mappedLeftChildOrder;
              mappedLeftChildOrder =
                mappedLeftChildTmp.findCommonElementsFromList(targetSortKey);
	    }
	  }
	}
      }
    }
    if (!useListInsteadOfSet)
    {
      // No duplicate entries, hence use the old logic for the set

      // Update top values with the characteristics outputs of the child's
      // characteristic outputs

      updateSelectValueIdMap()->rewriteValueIdSetUp(
	mappedCharOutputs,
	childCharOutput);

      // map the sort key from left to right
      updateSelectValueIdMap()->rewriteValueIdListUp(
	mappedLeftChildOrder,
	sppForChild->getSortKey());

    }

    // map the partitioning function from the left to the right
    mappedChildPartFunc =
      mappedChildPartFunc->copyAndRemap(*updateSelectValueIdMap(),
                                        TRUE);
    if (mappedDp2SortOrderPartFunc != NULL)
    {
      // map the Dp2 sort order part func from the left to the right
      mappedDp2SortOrderPartFunc =
        mappedDp2SortOrderPartFunc->copyAndRemap(
                                      *updateSelectValueIdMap(),
                                      TRUE);
    }
  } // end if this is a base table write op

  if (NOT mappedLeftChildOrder.isEmpty() &&
      NOT isPlan0)
  {
    ValueIdList * leftChildOrder =
      new(CmpCommon::statementHeap())
        ValueIdList(mappedLeftChildOrder);

    ipp = new(CmpCommon::statementHeap())
            InputPhysicalProperty(
              mappedCharOutputs,
              leftChildOrder,
              mappedChildPartFunc,
              mappedDp2SortOrderPartFunc,
              FALSE,
              sppForChild->getexplodedOcbJoinProperty());
  }
  else
  {
    if (NOT isPlan0)
       return NULL;

    ipp = new(CmpCommon::statementHeap())
            InputPhysicalProperty(TRUE,mappedChildPartFunc,sppForChild->getexplodedOcbJoinProperty());

  }

  return ipp;

} // NestedJoin::generateIpp()

PartitioningFunction * 
NestedJoin::getClusteringIndexPartFuncForRightChild() const
{
  GroupAttributes *ga = child(1).getGroupAttr();
  const SET(IndexDesc *) &availIndexes = ga->getAvailableBtreeIndexes();

  Int32 x = availIndexes.entries();

  for (CollIndex i = 0; i < availIndexes.entries(); i++) {
      IndexDesc *iDesc = availIndexes[i];
      if (iDesc->isClusteringIndex()) {
           return iDesc->getPartitioningFunction();
      }
  }

  return NULL;
}

NABoolean 
NestedJoin::checkCompleteSortOrder(const PhysicalProperty* sppForChild0)
{
   // Map the target table sort key to the source. We could map
   // from the source to the target and do the comparison that
   // way. But any columns that are covered by constants will
   // only be covered on the source side, not the target side.
   // So we can only correctly identify target key columns that are
   // covered by constants by mapping them to the source side first.

   ValueIdList mappedTargetTableOrder;
   updateSelectValueIdMap()->rewriteValueIdListDown(
         updateTableDesc()->getClusteringIndex()->getOrderOfKeyValues(),
         mappedTargetTableOrder);

   // Remove from the mapped target table order any columns that are
   // equal to constants or input values.
   mappedTargetTableOrder.removeCoveredExprs(
             getGroupAttr()->getCharacteristicInputs());

   if (mappedTargetTableOrder.isEmpty())
      return FALSE;
   
   // The outer table sort order will not help if #of sort key columns from 
   // the child0 is less than #of the clustiner key columns of the target table.
   CollIndex n = sppForChild0->getSortKey().entries();

   if ( n < mappedTargetTableOrder.entries() )
      return FALSE;

   if ( n > mappedTargetTableOrder.entries() )
     n = mappedTargetTableOrder.entries();

   for ( CollIndex i=0; i<n; i++ ) {

      ValueId targetKeyCol = mappedTargetTableOrder[i];
      ValueId outerOrderCol = sppForChild0->getSortKey()[i];

      if ( outerOrderCol.getItemExpr()->sameOrder(targetKeyCol.getItemExpr()) 
           != SAME_ORDER )
          return FALSE;
  }

  
  // If there is a dp2SortOrderPartFunc, map it and see if it
  // is exactly the same as the target partitioning function.
  // If it is not, then we cannot use the outer table sort order.
  PartitioningFunction* mappedDp2SortOrderPartFunc =
                             sppForChild0->getDp2SortOrderPartFunc();
  if (mappedDp2SortOrderPartFunc != NULL)
  {
     mappedDp2SortOrderPartFunc =
           mappedDp2SortOrderPartFunc->copyAndRemap(
                                        *updateSelectValueIdMap(),
                                         TRUE);
     PartitioningFunction* targetTablePartFunc =
         updateTableDesc()->getClusteringIndex()-> getPartitioningFunction();
                  
     // If the target table is not partitioned, it cannot
     // match the dp2SortOrderPartFunc, since we will never
     // synthesize a sort order type of DP2 for an unpart table.
     if ((targetTablePartFunc == NULL) OR
           mappedDp2SortOrderPartFunc->
              comparePartFuncToFunc(*targetTablePartFunc) != SAME)
     {
       return FALSE;
     }
  }

  return TRUE;
}

//<pb>
Context* NestedJoin::createContextForAChild(Context* myContext,
                                            PlanWorkSpace* pws,
                                            Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  // For a nested join we try up to 5 child plans, named plan 0 ... plan 4 
  // (10 contexts, 5 for each child). Here is a short summary of those
  // plans:
  //
  // Plan 0: Requires no sort order from the left (outer) child.
  //         For read query:  Leave left child the freedom to pick a
  //                          partitioning and force right child to match it.
  //         For write query: Force left child to match partitioning
  //                          of the target table (or a grouping of it)
  //
  // Plan 1: Similar to Plan0, except that this time we force the left child
  //         to match the order of the right child.
  //         Plan1 is skipped if OCB is being considered.
  //         For read query. try OCR, if applicable.
  //
  // Plan 2: Similar to plan1, but this time we request the left child to match
  //         the sort order naturally, without using a sort.
  //         Plan2 is skipped if preferred probing order is off (read) or
  //         UPD_ORDERED is off (write).
  //         In some cases we do plan 3 instead of plan 2.
  //
  // Plan 3: Similar to plan1, but this time we request the left child to match
  //         the sort order by using a sort operator
  //         Plan3 is skipped if preferred probing order is off (read) or
  //         UPD_ORDERED is off (write).
  //
  // Plan 4: Generate an OCB plan, if applicable. Also require the left to match
  //         the order of the right child. Unlike plans 0-3, we try the right child
  //         first in plan 4. We ask the left child to broadcast data via ESPs.

  // Previous comment - probably no longer applicable: The old second plan 
  // is one where no order is required from the left child,
  // but ordering information is passed to the right child if the
  // left child synthesizes a sort key.

  // Details for plan 2 and plan3:
  // These two plans are only attempted when Preferred Probing Order 
  // (PREFERRED_PROBING_ORDER_FOR_NESTED_JOIN) is ON for reads or UPD_ORDERED
  // is ON for writes. For reads, plan 2 and plan 3 will choose an index
  // from the right child group attributes information that satisfies any
  // parent ordering or partitioning requirements and has the fewest number
  // of uncovered key columns.  The sort key columns of this index
  // that are equijoin columns are then passed to the left child as
  // a order requirement. For writes, the left child is required to
  // be ordered on the target table primary key columns.
  //   For the third plan, the order requirement must be satisfied
  // without sorting. If this does not produce a plan that satisfies
  // the requirement without using synchronous access, then a fourth
  // plan is tried that demands the left child satisfy the order
  // requirement via sorting.
  //
  // Details for plan 4 (OCB):
  // Plan 4 replicates outer child of nested join. For this
  // plan we start from inner child and check if it can satisfy partitioning
  // requirement for join itself. If it does then we create a plan for inner
  // child first then create context for outer child to replicate with
  // broadcast if certain conditions are met (see below). This is to prevent
  // uneven load of ESPs running  the join in case when data is highly
  // skewed.
  //
  // Summary of abbreviations used
  // -----------------------------
  // OCB: Outer Child Broadcast, broadcast left child to all ESPs,
  //      each ESP only joins with local partitions on the right, this
  //      is done when there are very few outer rows, to distribute
  //      the work more evenly across ESPs. Can't be used for semi-
  //      or left outer joins.
  // OCR: Outer Child Repartitioning (which includes a natural partitioning
  //      of the left child). Force the left child to be partitioned the
  //      same (or a grouping) of the right child's clustering index.
  //      This avoids a large number of opens in the ESPs, which would
  //      otherwise all open every inner partitions (n^2 opens).
  //      - Can't be used for non-equi joins. 
  //      - Either OCR or OCB is considered, but not both.
  // N2J: N square Join (or "vanilla" Nested plan 2 Join), using neither
  //      OCB nor OCR. In some cases, N2J competes with OCR or OCB.
  //      - Can be used in all cases (but not always considered)
  //
  // PPO: Preferred Probing Order, ask left child to match the order
  //      of the right one, to reduce random accesses to the right table.
  //
  // CQDs used to control OCB
  // ------------------------
  //
  // COMP_BOOL_130, if ON - consider OCB join plan, by default ON
  // COMP_BOOL_131, if ON - let OCB compete with plan 0,1 by cost, default OFF
  // COMP_BOOL_132, if ON - compete with plan 2,3 by cost, default OFF
  // COMP_BOOL_133, if ON - force OCB join, default OFF
  // COMP_BOOL_134, if ON - force check for inner child #of partitions,
  //                        default ON
  // COMP_BOOL_135, if ON - force OCB for large inner child, not only
  //                        start join into fact table, default ON
  // COMP_INT_30 - defines a threshold for cardinality of outer child not
  //               to get broadcasted, default 5, threshold is
  //               #of NJ ESPs time COMP_INT_30 (for example 128*2 = 256)
  // COMP_INT_31 - number of base table (of outer child) threshold,
  //               default 5
  // COMP_INT_32 - minimum threshold for size of inner child per probe
  //               to prevent OCB, default 100 bytes
  // 
  // Other relevant CQDs
  // -------------------
  //
  // UPD_ORDERED                         - OFF    - disables plans 3 and 4 for writes,
  //                                                unless required for Halloween
  //                                                protection or side tree insert
  //                                       SYSTEM - same as OFF
  //                                       ON     - enables plans 3 and 4 for writes
  // PREFERRED_PROBING_ORDER_FOR_NESTED_JOIN
  //                                     - OFF    - disables plans 3 and 4 for reads
  //                                       SYSTEM - same as OFF
  //                                       ON     - enables plans 3 and 4 for reads
  // NESTED_JOINS_NO_NSQUARE_OPENS       - when ON, tries to suppress N2J plans
  //                                       that use an excessive number of opens.
  //                                       Also overrides the following CQD and
  //                                       enables OCR without a threshold.
  // NESTED_JOINS_OCR_MAXOPEN_THRESHOLD  - enables OCR if (max degree of ||ism *
  //                                       #partitions of inner table) exceed
  //                                       the value of this CQD
  // NESTED_JOINS_ANTISKEW_ESPS          - if >0, we consider a skew requirement
  //                                       h2-rd for the left child in plan 1 (OCR)

  NestedJoinPlanWorkSpace *njPws = (NestedJoinPlanWorkSpace *) pws;
  const ReqdPhysicalProperty* rppForMe = myContext->getReqdPhysicalProperty();
  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  // ---------------------------------------------------------------------
  // Code that is executed only the first time for each PlanWorkSpace
  // ---------------------------------------------------------------------

  // If this is the first time we invoke this method for a CreatePlanTask,
  // initialize some values and store them in our custom plan work space
  if (njPws->isEmpty())
    {
      Lng32     childPlansToConsider = 4; // means plan 0 and plan 1 for this join
      NABoolean OCBJoinIsConsidered  = FALSE;
      NABoolean OCRJoinIsConsidered  = FALSE;
 
      if (((CURRSTMT_OPTDEFAULTS->preferredProbingOrderForNJ() OR
            ((getSource() == Join::STAR_FACT) AND // star fact join is always ppo
             (CmpCommon::getDefault(COMP_BOOL_72) != DF_ON))) AND
           (getGroupAttr()->getNumBaseTables() > 1) AND // to avoid Index joins
           (updateTableDesc() == NULL)) // read
          OR
          ((CURRSTMT_OPTDEFAULTS->orderedWritesForNJ() OR
            avoidHalloweenR2() OR  // Always want to sort if avoidHalloweenR2
            getHalloweenForceSort() == FORCED OR
            CURRSTMT_OPTDEFAULTS->isSideTreeInsert() == TRUE 
            ) // Always sort if halloween forces
           AND
           (updateTableDesc() != NULL))) // write
        childPlansToConsider = 8; // try plans 2 and 3 for this join

      CostScalar child0card=
        child(0).getGroupAttr()->getResultCardinalityForEmptyInput();

      const SET(IndexDesc *) &availIndexes1=
        child(1).getGroupAttr()->getAvailableBtreeIndexes();

      Cardinality minrows, maxrows;
      child(0).getGroupAttr()->hasCardConstraint(minrows, maxrows);

      CostScalar maxOCBRowsLimit = defs.getAsLong(COMP_INT_30) *
         myContext->getReqdPhysicalProperty()->getCountOfPipelines();

      // -----------------------------------------------------------------
      // determine whether OCB is feasible
      // -----------------------------------------------------------------

      if ( ( (CmpCommon::getDefault(COMP_BOOL_133) == DF_ON) OR
             ( (CmpCommon::getDefault(COMP_BOOL_130) == DF_ON) AND
               // this condition will prevent OCB for DDL, internal refresh
               // and embedded update/delete statement. It could be ignored
               // by setting COMP_BOOL_9 to 'ON'. The code for checking these
               // conditions need to be refactored because it affects already
               // several features: pruning, maximum parallelism, parallel
               // insert/select and OCB. PruningIsFeasible is just a flag
               // set to FALSE if any of these conditions holds.
               CURRSTMT_OPTGLOBALS->pruningIsFeasible AND
               // cannot use OCB for left-, semi-joins because it can produce
               // the wrong result - multiple non matching rows.
               ( NOT ( isLeftJoin() OR isSemiJoin() OR isAntiSemiJoin() ) ) 
               AND
               (
                 ( CostScalar(maxrows) <= maxOCBRowsLimit ) //heuristic0
                 OR
                 ( // heuristic1 
                   // OCB for top join of star join into fact table
                   ( (getSource() == Join::STAR_FACT ) OR
                     // OCB for small outer child and big inner child
                     ( (CmpCommon::getDefault(COMP_BOOL_135) == DF_ON) AND
                       // if this is not a star join we don't want outer child
                       // to have more than 2 base table because cardinality
                       // estimation is becomung less accurate
                       ( child(0).getGroupAttr()->getNumBaseTables() <=
                         defs.getAsLong(COMP_INT_31)
                       ) AND
                       // the size of inner child per probe is greater
                       // than a threshold
                       ( getGroupAttr()->getResultCardinalityForEmptyInput()/
                            child0card.minCsOne() *
                         ( child(1).getGroupAttr()->getRecordLength() +
                           // this is, in fact, row header size in a message
                           // CQD name is a misnomer.
                           defs.getAsLong( DP2_MESSAGE_HEADER_SIZE_BYTES )
                         )
                       ) > defs.getAsLong(COMP_INT_32)
                     )
                   ) AND
                   ( child0card < maxOCBRowsLimit AND
                     child(0).getGroupAttr()->getResultMaxCardinalityForEmptyInput() / 10 <
                        maxOCBRowsLimit 
                   )
                 )
                 OR
                 ( // heuristic2 -  forced OCB
                   // When join predicates do not cover inner table partition key,
                   // We do OCB to avoid O(n^2) connections (from Joining ESPs to
                   // DP2s).
                   JoinPredicateCoversChild1PartKey() == FALSE  AND
                   availIndexes1.entries() >= 1 AND
                   (availIndexes1[0]->getPrimaryTableDesc()->getIndexes()).entries() == 1

                   // This line does not work as the indexes are masked out here 
                   // (eliminated). The index shows up in the plan (e.g., in 
                   // optdml03 Q17).
                   //(child(1).getGroupAttr()->getAvailableBtreeIndexes()).entries() 
                   //           == 1
                 )
               )
               // for the first prototype this will check partitioning and coverage
               // of prefix of inner child clustering key by join columns, local
               // predicates and and constant expressions
             )
           ) AND OCBJoinIsFeasible(myContext)
         )
        {
          OCBJoinIsConsidered = TRUE;
          childPlansToConsider = 10;
        }

      Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
      float childNumPartsAllowedDeviation = 0.0;
      NABoolean numOfESPsForced = FALSE;

      NABoolean useParallelism =
        okToAttemptESPParallelism(myContext,
                                  pws,
                                  childNumPartsRequirement,
                                  childNumPartsAllowedDeviation,
                                  numOfESPsForced);

      // -----------------------------------------------------------------
      // determine whether OCR is feasible
      // -----------------------------------------------------------------

      if ( //CmpCommon::getDefault(NESTED_JOINS_OCR) == DF_ON AND
           OCBJoinIsConsidered == FALSE AND
           updateTableDesc() == NULL /* is read case? */ AND
           OCRJoinIsFeasible(myContext) == TRUE AND
           useParallelism ) 
        {
          OCRJoinIsConsidered = TRUE;
        }


      // Decide if it is a fast trafodion load query
      NABoolean isFastLoadIntoTrafodion = FALSE;
      OperatorTypeEnum childOpType = child(1).getLogExpr()->getOperatorType();
      if ( childOpType == REL_LEAF_INSERT ) {
          RelExpr* c1 = child(1).getLogExpr();
          Insert* ins = (Insert*)c1;
          isFastLoadIntoTrafodion = ins->getIsTrafLoadPrep();
       }


      // store results in njPws for later reuse
      njPws->setParallelismItems(useParallelism,
                                 childNumPartsRequirement,
                                 childNumPartsAllowedDeviation,
                                 numOfESPsForced);
      njPws->setChildPlansToConsider(childPlansToConsider);
      njPws->setOCBJoinIsConsidered(OCBJoinIsConsidered);
      njPws->setOCRJoinIsConsidered(OCRJoinIsConsidered);
      njPws->setFastLoadIntoTrafodion(isFastLoadIntoTrafodion);
    } // end njPws->isEmpty()
  // ---------------------------------------------------------------------
  // If enough Contexts are generated, return NULL
  // to signal completion.
  // ---------------------------------------------------------------------
  if (njPws->getCountOfChildContexts() == njPws->getChildPlansToConsider())
    return NULL;

  Context* result = NULL;
  Lng32 planNumber = 0;
  Context* childContext = NULL;
  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();
  const ReqdPhysicalProperty* rppForChild = NULL;

  // Initialize the ipp to what the parent specifies. If the left child
  // does not synthesize a sort key, then this is what we will pass to
  // the right child.
  const InputPhysicalProperty* ippForMyChild =
                                    myContext->getInputPhysicalProperty();

                   
  NABoolean noN2JForRead  = ((CmpCommon::getDefault(NESTED_JOINS_NO_NSQUARE_OPENS) == DF_ON) && 
                             (updateTableDesc() == NULL));

  NABoolean noEquiN2J = 
                   (njPws->getOCRJoinIsConsidered() || njPws->getOCBJoinIsConsidered()) &&
                   (!( isLeftJoin() OR isSemiJoin() OR isAntiSemiJoin() )) &&
                   (rppForMe->getMustMatch() == NULL) &&
                   getOriginalEquiJoinExpressions().getTopValues().entries() > 0 &&
                   noN2JForRead
                                ; 

  NABoolean trySortPlanInPlan3 = 
                   (CmpCommon::getDefault(NESTED_JOINS_PLAN3_TRY_SORT) == DF_ON);

  // if this IUD, but UPD_ORDER is ON then don't try NJ plan 0
  NABoolean shutDownPlan0 = FALSE;
  if ( (updateTableDesc() != NULL) &&
       (rppForMe->getMustMatch() == NULL) &&
       (!rppForMe->executeInDP2()) &&
       (!getReqdOrder().entries() || njPws->getFastLoadIntoTrafodion())  &&
        CURRSTMT_OPTDEFAULTS->orderedWritesForNJ()
     )
    shutDownPlan0 = TRUE;

  // ---------------------------------------------------------------------
  // The creation of the next Context for a child depends upon the
  // the number of child Contexts that have been created in earlier
  // invocations of this method.
  // ---------------------------------------------------------------------
  while ((pws->getCountOfChildContexts() < njPws->getChildPlansToConsider()) AND
         (rppForChild == NULL))
  {
    // If we stay in this loop because we didn't generate some
    // child contexts, we need to reset the child plan count when
    // it gets to be as large as the arity, because otherwise we
    // would advance the child plan count past the arity.
    if (pws->getPlanChildCount() >= getArity())
      pws->resetPlanChildCount();

    planNumber = pws->getCountOfChildContexts() / 2;

    switch (pws->getCountOfChildContexts())
    {
      case 0:
      childIndex = 0;
    // -------------------------------------------------------------------
    // Case 0: Plan 0, child 0
    // +++++++++++++++++++++++
    // Create the 1st Context for left child. Disable this context if
    // OCB is not feasible or feasible but should compete with this
    // plan, and OCR is not feasible. When OCB is feasible, OCR is 
    // automatically disabled.
    // -------------------------------------------------------------------
    if ( currentPlanIsAcceptable(planNumber,rppForMe) AND
        ( NOT njPws->getOCBJoinIsConsidered() OR rppForMe->getMustMatch() != NULL OR 
          rppForMe->getSortOrderTypeReq() == DP2_OR_ESP_NO_SORT_SOT OR
          (CmpCommon::getDefault(COMP_BOOL_131) == DF_ON)) AND
        (
          CmpCommon::getDefault(NESTED_JOINS_PLAN0) == DF_ON AND
          njPws->getOCRJoinIsConsidered() == FALSE
        ) 
       )
    {
      // If using sort to prevent Halloween, then avoid this plan
      //
      if(! (avoidHalloweenR2() || getHalloweenForceSort() == FORCED) )
      {
        RequirementGenerator rg(child(0), rppForMe);

        // Do not check the null-ness of updateTableDesc() as NestedJoinFlow
        // is a subclass of this NestedJoin class, and it can flow tuples
        // to an insert node.
        if (rppForMe->executeInDP2()) 
        {
          if (rppForMe->getPushDownRequirement() == NULL) {
            // Add a co-location requirement (type-1 join in DP2) if the parent
            // does not provide one, regardless we are in CS or not.
            rg.addPushDownRequirement(
                 new (CmpCommon::statementHeap())
                   PushDownColocationRequirement()
                                     );
          }
        }

        if (updateTableDesc() != NULL)
        {
          // -----------------------------------------------------------------
          // TSJ on top of an insert/update/delete statement.
          // Make the left side match the partitioning scheme of the
          // updated table.
          // -----------------------------------------------------------------
          const PartitioningFunction *physicalPartFunc =
            updateTableDesc()->getClusteringIndex()->getPartitioningFunction();
          PartitioningRequirement* logicalPartReq = NULL;

          // Generate the partitioning requirements.
          // If the requirements could not be generated because the user
          // is attempting to force something that is not possible,
          // the method returns FALSE. Otherwise, it returns TRUE.
          if (genLeftChildPartReq(myContext,
                                  pws,
                                  physicalPartFunc,
                                  logicalPartReq))
          {
            // Push down IUD queries involving MVs.
            // 
            // If the execution location is DP2, and the parent partition req
            // is verified to be the same as the part func of the inner, we
            // remove the initial requirement from rg. Without this step, 
            // this initial partition requirement will be determined not 
            // compatible (by rg) with the mapped part func for 
            // the left child, even though both are connected properly by 
            // the updateSelectValueIdMap()!
            if ( physicalPartFunc != NULL AND
                 rppForMe->executeInDP2() AND
                 partReqForMe AND
                 partReqForMe->partReqAndFuncCompatible(physicalPartFunc)
               )
                rg.removeAllPartitioningRequirements();

            // Map the partitioning requirement partitioning key columns
            // from the right child to the left child.
            if (logicalPartReq)
              {
                logicalPartReq =
                  logicalPartReq->copyAndRemap(*updateSelectValueIdMap(),FALSE);
                rg.addPartRequirement(logicalPartReq);
              }
          }
          else
            // Tried to force something that was not possible. Give up now.
            return NULL;

          // If there is a required order stored in the join node, then require
          // the select data to be sorted in this order.
          // There will only be a required order if the write operation
          // was an insert and the user specified an ORDER BY clause.
          // Note that we don't need to map the req. order because it was
          // based on the original form of the insert, which was an insert
          // with the select as it's child, so the valueid's in the req. order
          // are already in terms of the select valueid's.
          if (getReqdOrder().entries() > 0)
          {
	    ValueIdSet reqdOrder = getReqdOrder();
	    reqdOrder.removeCoveredExprs(getGroupAttr()->getCharacteristicInputs());
            rg.addSortKey(reqdOrder);
          }
        } // this is for an write operation
        else // read
        {
          ValueIdList reqdOrder1;
          ValueIdSet reqdArr1;
          // If there is a required sort order and/or arrangement, then
          // split off any part of the requirement that is for the
          // right child and only pass on the portion of the requirement
          // that is for the left child. Pass back the requirements for
          // the right child in case they are needed.
          splitSortReqsForLeftChild(rppForMe, rg, reqdOrder1, reqdArr1);

          // this is for the patch below related to MAXIMUM parallelism
          RelExpr *child0LogExpr = child(0).getLogExpr();

          if (njPws->getUseParallelism()
              // This is to prevent parallel plan for inserting a single tuple.
              // Currently, when a single tuple is inserted in the table with
              // indexes, parallel plan caused by MAXIMUM parallelism option
              // results in a message that 0 rows inserted although tuple did
              // get inserted. The next check is to prevent such a plan which
              // does not make much sense anyway. March 2006.
              AND NOT
              ( child0LogExpr AND
                child0LogExpr->getOperatorType() == REL_LEAF_INSERT AND
                ((Insert *)child0LogExpr)->insertATuple()
                )
              )

            {
              // If we want to try ESP parallelism, go for it. Note we may still
              // not get it if the parent's requirement does not allow it.
              // If we don't go for ESP parallelism, then we will specify either
              // the parent's required number of partitions, or we will specify
              // that we don't care how many partitions we get - i.e.
              // ANY_NUMBER_OF_PARTITIONS.
              njPws->transferParallelismReqsToRG(rg);
            } // end if ok to try parallelism

        } // end if NJ is for a read operation

        // Produce the requirements and make sure they are feasible.
        if (rg.checkFeasibility())
        {
          // Produce the required physical properties.
          rppForChild = rg.produceRequirement();
        }
      }
          // this is an update case. I leave this now but we might need to
          // review it later because we might want to try another plan
    } // endif (pws->getCountOfChildContexts() == 0)
      break;

      case 1:
      childIndex = 1;
    // -------------------------------------------------------------------
    // Case 1: Plan 0, child 1
    // +++++++++++++++++++++++
    // Create the 1st Context for right child:
    // -------------------------------------------------------------------
   if ( currentPlanIsAcceptable(planNumber,rppForMe) 
        AND isTSJForSideTreeInsert() == FALSE 
	AND enableTransformToSTI() == FALSE
        AND !shutDownPlan0
	//	AND NOT CURRSTMT_OPTDEFAULTS->orderedWritesForNJ()
      )
       // Try this plan regardless of NESTED_JOINS_PLAN0 CQD setting
       // if OCR is not feasible
    {
      // -----------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // -----------------------------------------------------------------
      if (pws->getLatestChildContext() AND
          NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
        pws->eraseLatestContextFromWorkSpace();

      childContext = pws->getChildContext(0,0);

      // -----------------------------------------------------------------
      // Make sure a plan has been produced by the latest context.
      // -----------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty* sppForChild =
          childContext->getPhysicalPropertyForSolution();

        rppForChild = genRightChildReqs(sppForChild,rppForMe, noEquiN2J);

	if (isRowsetIterator()) {
	  // This condition ensures that this nested join
	  // is the parent of an unpack node used for rowsets.
	  // The ipp created to pass information about a rowset to the right child
	  // is unique in that all the nj fields of the ipp are NULL and only
	  // the assumeSortedForCosting_ field is TRUE.
	  ippForMyChild = new(CmpCommon::statementHeap())
			    InputPhysicalProperty( NULL, NULL,
			                           NULL, NULL, TRUE);

	}
        else if (updateTableDesc() == NULL &&
                 CmpCommon::getDefault(COMP_BOOL_60) == DF_ON)
        {
          // we would like to send left child's partitioning function 
          // for plan1. This is needed so that right child can accurately
          // analyze grouping relationship from the sibling
          ippForMyChild = generateIpp(sppForChild, TRUE);
        }
        else
        // If we produced an rpp for the child and there is a CQ Shape
        // command for this operator, then check if they are forcing PLAN1.
        if ((rppForChild != NULL) AND
            (rppForMe->getMustMatch() != NULL))
        {
          JoinForceWildCard::forcedPlanEnum forcePlanToken =
            getParallelJoinPlanToEnforce(rppForMe);

          // If the user is forcing plan #1, the plan where we pass the
          // left child order to the right child, then pass the order
          // now. This will be the only nj plan we will try.

          // Simulate PLAN1's right context here, only if OCR is not feasible. 
          // When feasible, OCR will reuse PLAN0's left context and as such
          // it will be incorrect to allow PLAN0's right context to continue. 
          // OCR can only be forces with PLAN1 or TYPE1 type specifier in CQS.
          // Plan0 is strictly type-2.
          if (forcePlanToken == JoinForceWildCard::FORCED_PLAN1 AND
              njPws->getOCRJoinIsConsidered() == FALSE)
          {
            // ----------------------------------------------------------------
            // Get the sort order of my left child and pass this information to
            // the context for optimizing my right child.  (Costing the inner
            // table access has a dependency on the order from the outer table).
            // ----------------------------------------------------------------
	    ippForMyChild = generateIpp(sppForChild);
            if (ippForMyChild == NULL)
              ippForMyChild = myContext->getInputPhysicalProperty();
          } // end if the user is forcing the plan where we pass the
            // left child order to the right child
        } // end if we produced an rpp for the child and
          // if there is a CQ Shape command to check
      } // end if child0 had an optimal solution
    } // endif (pws->getCountOfChildContexts() == 1)
      break;

      case 2:
      childIndex = 0;
    // -------------------------------------------------------------------
    // Case 2: Plan 1, child 0
    // +++++++++++++++++++++++
    // Create the 2nd Context for left child:
    // -------------------------------------------------------------------
    if ( currentPlanIsAcceptable(planNumber,rppForMe) AND
         ( NOT njPws->getOCBJoinIsConsidered() OR rppForMe->getMustMatch() != NULL OR
          rppForMe->getSortOrderTypeReq() == DP2_OR_ESP_NO_SORT_SOT OR
          (CmpCommon::getDefault(COMP_BOOL_131) == DF_ON)
        ) AND
        ( NOT derivedFromRoutineJoin()) AND
        isTSJForSideTreeInsert() == FALSE AND
	enableTransformToSTI() == FALSE
        )
    {
      NABoolean usableSortOrder = FALSE;
      NABoolean OCRJoinIsConsideredInCase2 = njPws->getOCRJoinIsConsidered();

      // If using sort to avoid Halloween, then avoid this plan
      //
      if(!(avoidHalloweenR2() || getHalloweenForceSort() == FORCED))
      {

        // Get the child context that was created for the left child of
        // the first nested join plan.
        childContext = pws->getChildContext(0,0);

        // -----------------------------------------------------------------
        // Make sure a plan was produced by the previous context for the
        // left child.
        // -----------------------------------------------------------------
        if((childContext != NULL) AND childContext->hasOptimalSolution())
        {
          const PhysicalProperty* sppForChild0Plan0 =
            childContext->getPhysicalPropertyForSolution();
          CMPASSERT(sppForChild0Plan0 != NULL);


          // See if the sort order for the left child of the previous plan
          // attempted (plan #0) is not empty.  If it is not, see if it
          // can help keep the I/Os down when accessing the inner table.
          if (NOT sppForChild0Plan0->getSortKey().isEmpty())
          {
            if (updateTableDesc() != NULL) // WRITE
            {
                usableSortOrder = checkCompleteSortOrder(sppForChild0Plan0);

            } // end if write
            else // READ
            if ( OCRJoinIsConsideredInCase2 == FALSE )
            {
              // Allocate dummy requirement generator object so we can
              // emulate the right child requirements.
              RequirementGenerator rg1(child(1),rppForMe);

              // If there is a required sort order and/or arrangement, then
              // split off any part of the requirement that is for the
              // right child and only pass on the portion of the requirement
              // that is for the left child. Pass back the requirements for
              // the right child. For our purposes here we
              // really only want the requirements for the right child.
              ValueIdList reqdOrder1;
              ValueIdSet reqdArr1;
              splitSortReqsForLeftChild(rppForMe, rg1, reqdOrder1, reqdArr1);

              // Get rid of all the left child requirements since we are
              // pretending we are processing the right child.
              rg1.removeSortKey();
              rg1.removeArrangement();
              rg1.removeAllPartitioningRequirements();

              // Add in the the left child sort key as a requirement for
              // the right child. This will force us to only pick a
              // index whose sort order is compatible with the left child
              // sort key. If the left child synthesized a
              // dp2SortOrderPartFunc, then we need to make sure we
              // pick an index which can satisfy it. So, we will also
              // specify the left child sort order type and the
              // dp2SortOrderPartFunc as requirements, too. The left
              // child synthesized sort order type must be "DP2" or
              // "NO" (if still in DP2), and so this will force
              // the recommendedOrderForNJProbing method to only
              // consider indices that satisfy the dp2SortOrderPartReq.

              if (sppForChild0Plan0->getDp2SortOrderPartFunc() == NULL)
                rg1.addSortKey(sppForChild0Plan0->getSortKey());
              else
                rg1.addSortKey(sppForChild0Plan0->getSortKey(),
                               sppForChild0Plan0->getSortOrderType(),
                               sppForChild0Plan0->
                                 getDp2SortOrderPartFunc()->
                                 makePartitioningRequirement());
              // Add in the left child partitioning function as a
              // requirement so we will be forced to pick an index
              // whose partitioning function is compatible with it.
              rg1.addPartRequirement(sppForChild0Plan0->
                                      getPartitioningFunction()->
                                      makePartitioningRequirement());

	      // This is required to give a chance for ordered NJ plan when the
	      // LC has more culstering keys than needed. This function removes
	      // extra keys if any.
	      if ( CmpCommon::getDefault(COMP_BOOL_188) == DF_OFF )
		rg1.removeExtraSortKeys();

              // Make sure the sort key and partitioning function from
              // the left child are covered by the right child ga. This
              // means the sort key and part key cols must all be equijoin
              // columns.
              if (rg1.checkFeasibility())
              {
                // produce a new rpp with what we have added
                const ReqdPhysicalProperty* rppForChild1Plan1 =
                  rg1.produceRequirement();

                // Now allocate a new rg to carry these requirements
                // over to the recommend probing method.
                RequirementGenerator rg2(child(1),rppForChild1Plan1);

                // Find an index that is compatible with the left child
                // sort order and partitioning and also is compatible
                // with any right child sort requirements.
                IndexDesc* ppoIDesc = NULL;
                NABoolean partKeyColsAreMappable = TRUE;
                ValueIdList preferredOrder=
                  child(1).getGroupAttr()->recommendedOrderForNJProbing(
                  child(0).getGroupAttr(),
                  ANY_NUMBER_OF_PARTITIONS,
                  rg2,
                  reqdOrder1,
                  reqdArr1,
                  ppoIDesc,
                  partKeyColsAreMappable);

                // If we got back a non-empty sort key, then this
                // means we found an index that is compatible with
                // the left child sort order, etc.
                if (NOT preferredOrder.isEmpty())
                {
                  usableSortOrder = TRUE;
                }
              } // end if feasible
            } // end allow plan2 for read (OCRJoinIsFeasible FALSE)

            // If we found a right child index that can use the left
            // child sort order, go ahead and generate this plan.
            // Otherwise, we will skip this plan.
            if (usableSortOrder)
            {
              // Produce the required physical properties.
              // Just get them from those we created for the left child
              // of plan#0 because we want to use the exact same
              // requirements. The only difference for this plan is
              // we are going to pass the sort key of the left child to
              // the right child when we create the context for the
              // right child. This means we should do no work to
              // optimize the left child for this plan - we should
              // reuse the same context we created for the first plan.
              rppForChild = childContext->getReqdPhysicalProperty();
              OCRJoinIsConsideredInCase2 = FALSE;
            }
          } // end if left child of previous plan had a sort key
        } // end if child0 of plan0 had an optimal solution
      } // end if not Halloween sort

      // Try OCR here if we do not get an useable Sort Order from the
      // optimal plan for the left child generated in plan1, 
      // or the sort order is cerated to protect against Halloween.
      // 
      // Note the existence of an usable sort order implies the the
      // right child of this plan will match the partition function of 
      // the left child, which is an OCR by itself. 
      // 
      // We do not try OCR with plan1 because we want the plan
      // configuration where potential repartition on the right
      // hand side to have a chance to exist.
      // 
      // We do not alter PPO plans (plan3 and plan4) for OCR either 
      // because both are superset of OCR (i.e., push the part func and
      // sort order from the right side to the left).
      if ( OCRJoinIsConsideredInCase2 )
      {
         PartitioningFunction * innerTablePartFunc = 
              getClusteringIndexPartFuncForRightChild(); 

         ValueIdMap map(getOriginalEquiJoinExpressions());

         // remap the right part func to the left (up)
         PartitioningFunction *rightPartFunc = 
              innerTablePartFunc -> copyAndRemap(map, TRUE);
  
         Lng32 childNumPartsRequirement = njPws->getChildNumPartsRequirement();

         // Scale down the number of partitions if necessary so that 
         // childNUmPartsrequirement evenly divides the # of partitions
         // of the rightPartFunc. We can do so because OCR works with
         // hash2 partitioned table only and hash2 has the nice 
         // property: 
         //
         // If down-scaling factor is x (x>1), then a row flows from 
         // the outer table of partition p will land in one of the 
         // partitions in the inner table ranging from partition  
         // (p-1)x to partition p x. If rep-n partitioning function is 
         // choosen for the inner table, the # of partitions that each
         // outer table partition will drive is x. Since open operations
         // are on-demand in nature, we will not open all (xp) partitions
         // for each outer table partition.
         
         // Use the original inner table part func if down-scaling fails.
         if ( rightPartFunc ->
             scaleNumberOfPartitions(childNumPartsRequirement) == FALSE ) 
           rightPartFunc = innerTablePartFunc;

         Int32 antiskewSkewEsps =
            (Int32)CmpCommon::getDefaultNumeric(NESTED_JOINS_ANTISKEW_ESPS);

         if ( antiskewSkewEsps > 0 ) {
            ValueIdSet joinPreds = getOriginalEquiJoinExpressions().getTopValues();

            double threshold = defs.getAsDouble(SKEW_SENSITIVITY_THRESHOLD) / rppForMe->getCountOfPipelines();

            SkewedValueList* skList = NULL;

            // If the join is on a skewed set of columns from the left child,
            // then generate the skew data partitioning requirement to deal
            // with skew.

            // We ignore whether the nested join probe cache is applicable here
            // because we want to deal with both the ESP and DP2 skew, Probe
            // cache can deal with DP2 skew, but not the ESP skew.
            if ( childNodeContainSkew(0, joinPreds, threshold, &skList) )
            {
              Lng32 cop = rightPartFunc->getCountOfPartitions();

              antiskewSkewEsps = MINOF(cop, antiskewSkewEsps);

              rightPartFunc = new (CmpCommon::statementHeap())
                     SkewedDataPartitioningFunction(
                         rightPartFunc,
                         skewProperty(skewProperty::UNIFORM_DISTRIBUTE,
                                      skList, antiskewSkewEsps)
                                                   );
            }
         }

         
         // Produce the part requirement for the outer child from
         // the part function of the inner child.
         PartitioningRequirement* partReqForChild
             = rightPartFunc->makePartitioningRequirement();
   
         // Now, add in the partitioning requirement for the
         // outer child.
         RequirementGenerator rg(child(0), rppForMe);
         rg.removeAllPartitioningRequirements();
         rg.addPartRequirement(partReqForChild);
   
   
         if (rg.checkFeasibility())
         {
           // Produce the required physical properties.
           rppForChild = rg.produceRequirement();
         }
      } // end of if OCR 
    } // endif (pws->getCountOfChildContexts() == 2)
      break;

      case 3:
      childIndex = 1;
    // -------------------------------------------------------------------
    // Case 3: Plan 1, child 1
    // +++++++++++++++++++++++
    // Create the 2nd Context for right child:
    // -------------------------------------------------------------------
    if ( currentPlanIsAcceptable(planNumber,rppForMe) )
    {
      // -----------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // -----------------------------------------------------------------
      if (pws->getLatestChildContext() AND
          NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
        pws->eraseLatestContextFromWorkSpace();

      childContext = pws->getChildContext(0,1);

      // -----------------------------------------------------------------
      // Make sure a plan has been produced by the latest context.
      // -----------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty* sppForChild =
          childContext->getPhysicalPropertyForSolution();

        // can not shut-down this code path when OCR is feasible
        rppForChild = genRightChildReqs(sppForChild,rppForMe, 
                                        !njPws->getOCRJoinIsConsidered() && noN2JForRead);

        // ----------------------------------------------------------------
        // Get the sort order of my left child and pass this information to
        // the context for optimizing my right child.  (Costing the inner
        // table access has a dependency on the order from the outer table).
        // If sort order of my left child is empty, then pass second argument
        // (TRUE) to generateIpp() so that we pass left child partfunc to 
        // correctly compute probeCache cost adjustment factor
        // ----------------------------------------------------------------

        if (rppForChild != NULL) {

          if (sppForChild->getSortKey().isEmpty())
          {
             // if it's an OCR plan and sort key is empty, then the ipp
             // created to pass information to the right child is unique
             // in that all the nj fields of the ipp are NULL and only
             // assumeSortedForCosting_ field is TRUE.
             // This is to let optimizer pick ocr plan vs serial Nj P3.
             if ( njPws->getOCRJoinIsConsidered() )
               ippForMyChild = new(CmpCommon::statementHeap())
                                 InputPhysicalProperty( NULL, NULL,
                                                        NULL, NULL, TRUE);
             else
               ippForMyChild = generateIpp(sppForChild, TRUE);
          }
          else
             ippForMyChild = generateIpp(sppForChild);

          if ( ippForMyChild ) {


            // Check to see if the dp2 partfunc of the outer child in
            // ippForMyChild is compatible with that of the clustering index,
            // If so, we will use ippForChild to help pick the clustering 
            // index for the inner side. Only when these two partition
            // functions are not the same, then we set ippForChild to NULL,
            // which will still make the OCR a candidate plan with a 
            // higher cost (than if the not NULL ippForChild is passed). 
            //
            // Solu 10-090210-9101 (OCR can select a serial plan), where
            // njDp2OuterOrderPartFunc=[F2.C, F2.B, F2.A, F2.D, F2.G, F2.F] and
            // innerTablePartFunc = [F2.A, F2.B, F2.C, F2.D, F2.E, F2.F],
            // and njDp2OuterOrderPartFunc and innerTablePartFunc are not
            // the same. If ippForMyChild is not set to NULL, we will not 
            // get the OCR plan.
            if ( njPws->getOCRJoinIsConsidered() ) {

               const PartitioningFunction* njDp2OuterOrderPartFunc =
                  ippForMyChild->getNjDp2OuterOrderPartFunc();

               PartitioningFunction * innerTablePartFunc =
                    getClusteringIndexPartFuncForRightChild();

               if ((njDp2OuterOrderPartFunc != NULL) AND
                   (njDp2OuterOrderPartFunc->
                    comparePartFuncToFunc(*innerTablePartFunc) != SAME))
               {
                  ippForMyChild = NULL;
               }
             }   
           } else
             ippForMyChild = myContext->getInputPhysicalProperty();

         } // end if we produced an rpp for the child
      } // end if child0 had an optimal solution
    } // endif (pws->getCountOfChildContexts() == 3)
      break;

    //********************************************************
    // The part of the code between the ***** is only executed
    // if preferred probing order is ON

      case 4:
      childIndex = 0;
    // -------------------------------------------------------------------
    // Case 4: Plan 2, child 0
    // +++++++++++++++++++++++
    // Create the 3rd Context for left child:
    // -------------------------------------------------------------------
    // TBD: consider collapsing plans 2 and 3
    if ( currentPlanIsAcceptable(planNumber,rppForMe) AND
         ( NOT njPws->getOCBJoinIsConsidered() OR rppForMe->getMustMatch() != NULL OR
           rppForMe->getSortOrderTypeReq() == DP2_OR_ESP_NO_SORT_SOT OR
           (CmpCommon::getDefault(COMP_BOOL_132) == DF_ON)
         )  AND
         ( NOT derivedFromRoutineJoin())
       )
    {
      // -----------------------------------------------------------------
      // Try a preferred probing order plan where we demand that the
      // left child satisfy the order naturally.
      // -----------------------------------------------------------------

      RequirementGenerator rg(child(0), rppForMe);

      if ( rppForMe->getPushDownRequirement() == NULL AND
           rppForMe->executeInDP2()
         )
      {
        // Add a co-location requirement (type-1 join in DP2) if the parent
        // does not provide one, regardless we are in CS or not.
        rg.addPushDownRequirement(
            new (CmpCommon::statementHeap())
                PushDownColocationRequirement()
                                 );
      }

      // FIRST: Determine the preferred order for accessing the right
      // child table and add a partitioning requirement that is based
      // on the corresponding right child access path.

      ValueIdList preferredOrder;
      // The following flag will be set to FALSE if the partitioning key
      // columns of the chosen index for read are not mappable, i.e.
      // they are not covered by the equijoin columns. For write,
      // the partitioning key columns of the target table primary key
      // are always mappable since there is an explicit map for this.
      NABoolean partKeyColsAreMappable = TRUE;
      PartitioningFunction* physicalPartFunc =  NULL;
      PartitioningRequirement* logicalPartReq = NULL;

      if (updateTableDesc() != NULL) // WRITE
      {
        // Determine the preferred order for accessing the target
        // table via the primary index.
        preferredOrder = genWriteOpLeftChildSortReq();
        if (preferredOrder.isEmpty() AND
           !(avoidHalloweenR2() || getHalloweenForceSort() == FORCED))
        {
          // Might as well give up, since this context will be
          // exactly the same as the first child context generated.
          //
          // Do it only when it is not sidetree inserts.
          if ( isTSJForSideTreeInsert() == FALSE AND !shutDownPlan0 )
             return NULL;

          // this is an update case. I leave this now but we might need to
          // review it later because we might want to try another plan. SP.
        }
        physicalPartFunc =
          updateTableDesc()->getClusteringIndex()->getPartitioningFunction();

        // Generate the partitioning requirements.
        // If the requirements could not be generated because the user
        // is attempting to force something that is not possible,
        // the method returns FALSE. Otherwise, it returns TRUE.
        if (genLeftChildPartReq(myContext,
                                pws,
                                physicalPartFunc,
                                logicalPartReq))
        {
          if (logicalPartReq)
            {
              // Map the partitioning requirement partitioning key columns
              // from the right child to the left child.
              logicalPartReq =
                logicalPartReq->copyAndRemap(*updateSelectValueIdMap(),FALSE);
              rg.addPartRequirement(logicalPartReq);
            }
        }
        else
          // Tried to force something that was not possible. Give up now.
          return NULL;
          // this is an update case. I leave this now but we might need to
          // review it later because we might want to try another plan. SP.
      } // end if WRITE
      else // READ
      {
        NABoolean numOfESPsForced = FALSE;

        Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
        float childNumPartsAllowedDeviation = 0.0;
        DefaultToken parallelControlSettings =
          getParallelControlSettings(rppForMe,
                                     childNumPartsRequirement,
                                     childNumPartsAllowedDeviation,
                                     numOfESPsForced);

        // If there is a required sort order and/or arrangement, then
        // split off any part of the requirement that is for the
        // right child and only pass on the portion of the requirement
        // that is for the left child. The portion that is for the
        // right child (child1) is passed back so we can use it
        // for verifying the suitability of any right child preferred
        // probing order indexes.
        ValueIdList reqdOrder1;
        ValueIdSet reqdArr1;
        splitSortReqsForLeftChild(rppForMe, rg, reqdOrder1, reqdArr1);

        // Determine the preferred probing order and the associated index
        IndexDesc* ppoIDesc = NULL;
        preferredOrder =
          child(1).getGroupAttr()->recommendedOrderForNJProbing(
            child(0).getGroupAttr(),
            childNumPartsRequirement,
            rg,
            reqdOrder1,
            reqdArr1,
            ppoIDesc,
            partKeyColsAreMappable);

        if (preferredOrder.isEmpty())
        {
          // Might as well give up, since this context will be
          // exactly the same as the first child context generated.
          //return NULL;
          break;
          // we don't want to give up now because we might want to
          // consider another plan like OCB join
        }

        physicalPartFunc =
          ppoIDesc->getPartitioningFunction();

        // Don't add a partitioning requirement of our own if we have
        // received a ReplicateNoBroadcast partitioning requirement
        // from our parent. This is because there is no way any
        // partitioning requirement we add will be compatible with
        // the parent requirement, and there is no way to get rid
        // of a ReplicateNoBroadcast partitioning requirement, as
        // Exchange cannot get rid of it for us, unfortunately.
        // So, if we don't make this exception, we will never be
        // able to try preferred probing order plans for joins
        // inside correlated subqueries if the nested join that
        // connects the subquery to the main query is executing
        // in parallel. Because of this, we may end up with
        // a non-type-1 parallel join, and so the order will not
        // be completely preserved, but this is deemed better
        // then getting no ordering at all.
        if ((partReqForMe == NULL) OR
            NOT partReqForMe->isRequirementReplicateNoBroadcast())
        {
          // Generate the partitioning requirements.
          // If the requirements could not be generated because the user
          // is attempting to force something that is not possible,
          // the method returns FALSE. Otherwise, it returns TRUE.
          if (genLeftChildPartReq(myContext,
                                  pws,
                                  physicalPartFunc,
                                  logicalPartReq))
          {
            if (logicalPartReq)
              rg.addPartRequirement(logicalPartReq);
          }
          else
            // Tried to force something that was not possible. Give up now.
            //return NULL;
            break;
            // we don't want to give up now because we might want to
            // consider another plan like OCB join
        }
        else
          // Just set the logical part. req. to what we got from our parent.
          logicalPartReq = partReqForMe;
      } // end if READ

      // SECOND: Determine and add the correct sort order type requirement
      // for the preferred probing order.
      SortOrderTypeEnum preferredSortOrderTypeReq = NO_SOT;
      PartitioningRequirement* preferredDp2SortOrderPartReq = NULL;
      JoinForceWildCard::forcedPlanEnum forcePlanToken =
        JoinForceWildCard::ANY_PLAN;

      // If there is a CQ Shape command for this operator,
      // then check if they are forcing PLAN3.
      if (rppForMe->getMustMatch() != NULL)
      {
        forcePlanToken = getParallelJoinPlanToEnforce(rppForMe);
        // If the user is forcing the left child of the nested join
        // to be a sort operator, then we need to try a sorted plan,
        // as this will be only way we can honor the CQS request and
        // try to force an order of our own. So, in this case we need
        // to do PLAN3 now as well.
        if ((forcePlanToken != JoinForceWildCard::FORCED_PLAN3) AND
            (rppForMe->getMustMatch()->child(childIndex)->
                getOperatorType() == REL_SORT))
          forcePlanToken = JoinForceWildCard::FORCED_PLAN3;
      }

      // Determine the sort order type requirement:
      // If we're in DP2, we must not specify a sort order type.
      //
      // If our parent requires an ESP_VIA_SORT sort order type,
      // then we better ask for that now as asking for a natural
      // sort order will not be compatible. If we do this now, we won't
      // do the 4th nested join plan since we will have already done
      // a sorted plan.
      // If the user is forcing plan 3, the plan where we set the
      // required sort order type to ESP_VIA_SORT, then require
      // that sort order type now.
      // If we do this now, we won't do plan 3
      // since we will have already done a sorted plan.
      //
      // If there is only one physical partition, or if there
      // will only be one partition per logical partition, then a
      // dp2 sort order will be the same as an esp no sort order,
      // so set the sort order type requirement to ESP_NO_SORT.
      // If we can't map the partitioning key columns of the
      // physical partitioning function, then we won't be able to
      // use it as a dp2SortOrderPartReq, so set the sort order type
      // requirement to ESP_NO_SORT.
      // If synchronous access is forced, no point in asking for a
      // DP2 sort order, so set the sort order type req. to ESP_NO_SORT.

      // If avoidHalloweenR2 is TRUE or getHalloweenForceSort() == FORCED
      // then, then we always want ESP_VIA_SORT_SOT to force a SORT
      // operator.  The blocking nature of sort avoids the potential
      // Halloween problem.
      //
      if (rppForMe->executeInDP2())
        preferredSortOrderTypeReq = NO_SOT;
      else if ((rppForMe->getSortOrderTypeReq() == ESP_VIA_SORT_SOT) OR
               (forcePlanToken == JoinForceWildCard::FORCED_PLAN3) OR
               avoidHalloweenR2() OR
               getHalloweenForceSort() == FORCED)
        preferredSortOrderTypeReq = ESP_VIA_SORT_SOT;
      else if ((physicalPartFunc == NULL) OR
               (logicalPartReq == NULL) OR
               (logicalPartReq->getCountOfPartitions() ==
                physicalPartFunc->getCountOfPartitions()) OR
               NOT partKeyColsAreMappable OR
               (CmpCommon::getDefault(ATTEMPT_ASYNCHRONOUS_ACCESS) ==
                DF_OFF))
        preferredSortOrderTypeReq = ESP_NO_SORT_SOT;
      else
      {
        preferredSortOrderTypeReq = DP2_OR_ESP_NO_SORT_SOT;
        preferredDp2SortOrderPartReq =
          physicalPartFunc->makePartitioningRequirement();
        if (updateTableDesc() != NULL) // WRITE
        {
          // map physical part func part key to left child value ids.
          // Don't need to do this for read because we have
          // ensured that all part key cols for read are covered
          // by the equijoin columns, which means they are in
          // the same VEG group so both sides of the join use
          // the same valueids.
          preferredDp2SortOrderPartReq =
            preferredDp2SortOrderPartReq->copyAndRemap(
                       *updateSelectValueIdMap(),FALSE);
        }
      }

      // Now add in a requirement that the data be sorted in the
      // preferred probing order with the preferred sort order type.
      rg.addSortKey(preferredOrder,
                    preferredSortOrderTypeReq,
                    preferredDp2SortOrderPartReq);


      if (rg.checkFeasibility())
      {
        // Produce the required physical properties.
        rppForChild = rg.produceRequirement();
      }

    } // endif (pws->getCountOfChildContexts() == 4)
      break;

      case 5:
      childIndex = 1;
    // -------------------------------------------------------------------
    // Case 5: Plan 2, child 1
    // +++++++++++++++++++++++
    // Create the 3rd Context for right child:
    // -------------------------------------------------------------------
    if ( currentPlanIsAcceptable(planNumber,rppForMe) )
    {
      // -----------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // -----------------------------------------------------------------
      if (pws->getLatestChildContext() AND
          NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
        pws->eraseLatestContextFromWorkSpace();

      childContext = pws->getChildContext(0,2);

      // -----------------------------------------------------------------
      // Make sure a plan has been produced by the latest context.
      // -----------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty* sppForChild =
          childContext->getPhysicalPropertyForSolution();
        rppForChild = genRightChildReqs(sppForChild,rppForMe, noEquiN2J);
        if (rppForChild != NULL)
        {
          // ----------------------------------------------------------------
          // Get the sort order of my left child and pass this information to
          // the context for optimizing my right child.  (Costing the inner
          // table access has a dependency on the order from the outer table).
          // ----------------------------------------------------------------
          ippForMyChild = generateIpp(sppForChild);
          if (ippForMyChild == NULL)
            ippForMyChild = myContext->getInputPhysicalProperty();
        } // end if we produced an rpp for the child
      } // end if child0 had an optimal solution
    } // endif (pws->getCountOfChildContexts() == 5)
      break;

      case 6:
      childIndex = 0;
    // -------------------------------------------------------------------
    // Case 6: Plan 3, child 0
    // +++++++++++++++++++++++
    // Create the 4th Context for left child:
    // -------------------------------------------------------------------
    if ( currentPlanIsAcceptable(planNumber,rppForMe) AND
         ( NOT njPws->getOCBJoinIsConsidered() OR rppForMe->getMustMatch() != NULL OR
           rppForMe->getSortOrderTypeReq() == DP2_OR_ESP_NO_SORT_SOT OR
           (CmpCommon::getDefault(COMP_BOOL_132) == DF_ON)
         ) AND
         ( NOT derivedFromRoutineJoin())
       )
    {
      // -----------------------------------------------------------------
      // Try a preferred probing order plan where we demand that the
      // left child satisfy the order via sorting.
      // -----------------------------------------------------------------

      // Get the context for the preferred probing order plan without
      // sorting to see if it succeeded in getting a DP2 sort order
      // type plan.
      childContext = pws->getChildContext(0,2);

      // If the first ppo plan did not produce a context, then this
      // plan won't be able to succeed, either, since all we are
      // doing is changing the sort order type. So, only try this
      // plan if the first ppo plan created a context. Also can't
      // sort in DP2 so don't try this if in DP2.
      if ((childContext != NULL) AND NOT rppForMe->executeInDP2())
      {
        NABoolean trySortedPlan = FALSE;

        // If the first ppo plan did not get a plan, then this
        // means no natural order was available, so we definitely
        // want to try sorting.
        if (NOT childContext->hasOptimalSolution())
          trySortedPlan = TRUE;
        else
        {
          // The first ppo plan did get a plan, so try a sorted plan
          // if the first plan was not really a true sorted-in-ESP plan 3 
          // already (ESP_VIA_SORT_SOT).
          const PhysicalProperty* sppForChild0Plan2 =
            childContext->getPhysicalPropertyForSolution();

          if ( trySortPlanInPlan3 ) {
             // The new way: try the explicit sort plan when plan2 does not produce
             // such a sort plan.
             if ( sppForChild0Plan2->getSortOrderType() != ESP_VIA_SORT_SOT )
                trySortedPlan = TRUE;
          } else {
             // The old way of doing business:
             //   The first ppo plan did get a plan, so try a sorted plan
             //   if the first plan used synchronous access to deliver the
             //   order and we did not already do a sorted plan.
             // Remove the ELSE branch in M9 after the new logic is tested in M8SP2.
             if ( sppForChild0Plan2->getSortOrderType() == ESP_NO_SORT_SOT )
                trySortedPlan = TRUE;
          }
        }

        // We also test if the first ppo plan (plan#2) has a sort requirement
        // for its left child. If so we continue with the new ppo plan
        // (force the order via sort).
        //
        // In special case such as inserting a sort operator to prevent
        // halloween condition on a syskey-only table, there is no sort
        // requirement. In this situation, we simply do not create
        // a new ppo context. This is because the new ppo context would be
        // exactly the same as the context for the left child in the first
        // ppo plan (no sort requirement).
        if (trySortedPlan AND
            childContext->getReqdPhysicalProperty()->getSortKey())
        {
          RequirementGenerator rg(child(0),rppForMe);
          if (updateTableDesc() == NULL) // READ?
          {
            // If there is a required sort order and/or arrangement, then
            // split off any part of the requirement that is for the
            // right child and only pass on the portion of the requirement
            // that is for the left child.
            ValueIdList reqdOrder1;
            ValueIdSet reqdArr1;
            splitSortReqsForLeftChild(rppForMe, rg, reqdOrder1, reqdArr1);
          }
          // Add a requirement that is exactly like the requirement
          // we created for the first ppo plan, except change the
          // required sort order type to ESP_VIA_SORT.
          const ReqdPhysicalProperty* rppForChild0Plan2 =
            childContext->getReqdPhysicalProperty();
          PartitioningRequirement* partReqForChild =
            rppForChild0Plan2->getPartitioningRequirement();
          const ValueIdList* const reqdOrderForChild =
            rppForChild0Plan2->getSortKey();
          SortOrderTypeEnum sortOrderTypeReqForChild = ESP_VIA_SORT_SOT;

          // The first ppo plan must have specified a partitioning
          // requirement and a sort requirement, or we should not
          // have done it!
          CMPASSERT((partReqForChild != NULL OR
                     (updateTableDesc() AND 
                      updateTableDesc()->getNATable()->isHbaseTable()))
                    AND
                    reqdOrderForChild != NULL);
          if (partReqForChild)
            rg.addPartRequirement(partReqForChild);
          rg.addSortKey(*reqdOrderForChild,sortOrderTypeReqForChild);

          // See if we are able to produce a plan. If the parent
          // requirement does not allow sorting, this will fail.
          if (rg.checkFeasibility())
          {
            // Produce the required physical properties for the child.
            rppForChild = rg.produceRequirement();
          }
        } // end if trySortedPlan
      } // end if a context was created for an unsorted ordered plan
        // and we're not in DP2

    } // endif (pws->getCountOfChildContexts() == 6)
      break;

      case 7:
      childIndex = 1;
    // -------------------------------------------------------------------
    // Case 7: Plan 3, child 1
    // +++++++++++++++++++++++
    // Create the 4th Context for right child:
    // -------------------------------------------------------------------
    if ( currentPlanIsAcceptable(planNumber,rppForMe) )
    {
      // -----------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // -----------------------------------------------------------------
      if (pws->getLatestChildContext() AND
          NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
        pws->eraseLatestContextFromWorkSpace();

      childContext = pws->getChildContext(0,3);

      // -----------------------------------------------------------------
      // Make sure a plan has been produced by the latest context.
      // -----------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty* sppForChild =
          childContext->getPhysicalPropertyForSolution();
        rppForChild = genRightChildReqs(sppForChild,rppForMe, noEquiN2J);
        if (rppForChild != NULL)
        {
          // ----------------------------------------------------------------
          // Get the sort order of my left child and pass this information to
          // the context for optimizing my right child.  (Costing the inner
          // table access has a dependency on the order from the outer table).
          // ----------------------------------------------------------------
          ippForMyChild = generateIpp(sppForChild);
          if (ippForMyChild == NULL)
            ippForMyChild = myContext->getInputPhysicalProperty();
        } // end if we produced an rpp for the child
      } // end if child0 had an optimal solution
    } // endif (pws->getCountOfChildContexts() == 7)
      break;
    //***************************************************

      case 8:
      childIndex = 1;
    // -------------------------------------------------------------------
    // Case 8: Plan 4, child 1 (note we start with child 1 this time)
    // +++++++++++++++++++++++
    // Create the 5th Context for right child: this is a new plan for OCB
    // -------------------------------------------------------------------
    if ( currentPlanIsAcceptable(planNumber,rppForMe) AND
         njPws->getOCBJoinIsConsidered() AND
         ( NOT derivedFromRoutineJoin())
       )
    {
      // -----------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // -----------------------------------------------------------------
      if (pws->getLatestChildContext() AND
          NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
        pws->eraseLatestContextFromWorkSpace();


      // -----------------------------------------------------------------
      // Split the order requirement for the left and right child and
      // ask the right child to satisfy its sort requirement, if split
      // is possible (see Join::splitOrderReq() for details)
      // -----------------------------------------------------------------
      RequirementGenerator rg(child(1),rppForMe);
      if (myContext->requiresOrder())
      {
        rg.removeSortKey();
        rg.removeArrangement();

        if (rppForMe->getSortKey() AND
            (rppForMe->getSortKey()->entries() > 0))
        {
          ValueIdList reqdOrder0, reqdOrder1;
          if (splitOrderReq(*(rppForMe->getSortKey()),
                            reqdOrder0,reqdOrder1))
          {
            rg.addSortKey(reqdOrder1);
          }
        }
      }

      // We must insist that the right child match the parent partitioning
      // requirements, because we are dealing with the right child first.
      // The right child will satisfy the parent somehow (if possible).
      // So, we don't remove the parent requirements.
      if (njPws->getUseParallelism())
        njPws->transferParallelismReqsToRG(rg);
      if( CmpCommon::getDefault(COMP_BOOL_53) == DF_ON )
        rg.addOcbCostingRequirement();
      // Produce the requirements and make sure they are feasible.
      if (rg.checkFeasibility())
        {
          // Produce the required physical properties.
          rg.addNoEspExchangeRequirement();
          rppForChild = rg.produceRequirement();
        }
    } // endif (pws->getCountOfChildContexts() == 8)
      break;

      case 9:
      childIndex = 0;
    // -------------------------------------------------------------------
    // Case 9: Plan 4, child 0
    // +++++++++++++++++++++++
    // Create the 5th Context for left child: this is a new plan for OCB
    // -------------------------------------------------------------------
    if ( currentPlanIsAcceptable(planNumber,rppForMe) AND
         njPws->getOCBJoinIsConsidered()
       )
    {
      // -----------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // -----------------------------------------------------------------
      if (pws->getLatestChildContext() AND
          NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
        pws->eraseLatestContextFromWorkSpace();

      childContext = pws->getChildContext(1,4);
      // -----------------------------------------------------------------
      // Make sure a plan has been produced by the latest context.
      // -----------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty* sppForChild =
          childContext->getPhysicalPropertyForSolution();

        CMPASSERT(sppForChild != NULL);
        PartitioningFunction*
          childPartFunc = sppForChild->getPartitioningFunction();
        PartitioningRequirement* partReqForChild;

        if (CmpCommon::getDefault(COMP_BOOL_82) == DF_ON)
        {
          // Use the node map of the inner child partitioning function.
          partReqForChild = new (CmpCommon::statementHeap() )
            RequireReplicateViaBroadcast(childPartFunc, TRUE);
        }
        else
        {
           partReqForChild = new (CmpCommon::statementHeap())
             RequireReplicateViaBroadcast(
               childPartFunc->getCountOfPartitions());

           // Use the node map of the inner child partitioning function.
           NodeMap *myNodeMap =
             childPartFunc->getNodeMap()->copy(CmpCommon::statementHeap());

           partReqForChild->castToFullySpecifiedPartitioningRequirement()->
             getPartitioningFunction()->replaceNodeMap(myNodeMap);
        }

        RequirementGenerator rg (child(0),rppForMe);

        // Remove any parent partitioning requirements, since we
        // have already enforced this on the left child.
        rg.removeAllPartitioningRequirements();

        // Now, add in broadcast partitioning requirement for the
        // leht child.
        rg.addPartRequirement(partReqForChild);

        // Split the order requirement for the left and right child and
        // ask the left child to satisfy its sort requirement, if split
        // is possible (see Join::splitOrderReq() for details)
        if (myContext->requiresOrder())
        {
          rg.removeSortKey();
          rg.removeArrangement();

          if (rppForMe->getSortKey() AND
              (rppForMe->getSortKey()->entries() > 0))
          {
            ValueIdList reqdOrder0, reqdOrder1;
            if (splitOrderReq(*(rppForMe->getSortKey()),
                              reqdOrder0,reqdOrder1))
            {
              rg.addSortKey(reqdOrder0);
            }
          }
        }

        // Produce the requirements and make sure they are feasible.
        if (rg.checkFeasibility())
        {
          // Produce the required physical properties.
          rppForChild = rg.produceRequirement();
        }
      } // end if child0 had an optimal solution
    } // endif (pws->getCountOfChildContexts() == 9)
      break;
    } // end of switch statement

    if (rppForChild != NULL)
    {
      // ----------------------------------------------------------------------
      // Compute the cost limit to be applied to the child.
      // ----------------------------------------------------------------------
      CostLimit* costLimit = computeCostLimit(myContext, pws);

      // ----------------------------------------------------------------------
      // Get a Context for optimizing the child.
      // Search for an existing Context in the CascadesGroup to which
      // the child belongs that requires the same properties as those
      // in rppForChild. Reuse it, if found. Otherwise, create a new
      // Context that contains rppForChild as the required physical
      // properties.
      // ----------------------------------------------------------------------
      EstLogPropSharedPtr inputLPForChild;
      EstLogPropSharedPtr copyInputLPForChild;

      if (childIndex == 0)
        inputLPForChild = myContext->getInputLogProp();
      else
      {
        inputLPForChild = child(0).outputLogProp
                               (myContext->getInputLogProp());

        if ( isSemiJoin() OR isAntiSemiJoin() )
        {
          // don't alter the estlogprop returned above, alter its copy.
          copyInputLPForChild = EstLogPropSharedPtr(
            new(CmpCommon::statementHeap()) EstLogProp(*inputLPForChild));
          if ( isSemiJoin() )
            copyInputLPForChild->setInputForSemiTSJ(EstLogProp::SEMI_TSJ);
          else
            copyInputLPForChild->setInputForSemiTSJ(EstLogProp::ANTI_SEMI_TSJ);
        }
      }

      if ( childIndex == 0 || !isSemiJoin() || !isAntiSemiJoin() )
        result = shareContext(childIndex, rppForChild,
                              ippForMyChild, costLimit,
                              myContext, inputLPForChild);
      else
        result = shareContext(childIndex, rppForChild,
                              ippForMyChild, costLimit,
                              myContext, copyInputLPForChild);

      if ( NOT (pws->isLatestContextWithinCostLimit() OR
                result->hasSolution() )
         )
       result = NULL;

    } // end if OK to create a child context
    else
      result = NULL;

    // -------------------------------------------------------------------
    // Remember the cases for which a Context could not be generated,
    // or store the context that was generated.
    // -------------------------------------------------------------------
    pws->storeChildContext(childIndex, planNumber, result);
    pws->incPlanChildCount();

    if ( CURRSTMT_OPTDEFAULTS->optimizerPruning() AND
         ( pws->getPlanChildCount() == getArity() ) AND
         CURRSTMT_OPTDEFAULTS->OPHexitNJcrContChiLoop()
       )
    {
      pws->resetAllChildrenContextsConsidered();
      break;
    }

  }  // end while loop

  if ( pws->getCountOfChildContexts() == njPws->getChildPlansToConsider() )
    pws->setAllChildrenContextsConsidered();

  return result;

} // NestedJoin::createContextForAChild()

//<pb>
NABoolean NestedJoin::findOptimalSolution(Context* myContext,
                                          PlanWorkSpace* pws)
{
  NABoolean hasOptSol;
  // Plan # is only an output param, initialize it to an impossible value.
  Lng32 planNumber = -1;

  hasOptSol = pws->findOptimalSolution(planNumber);

  // All nested join plans other than the first one are plans where the
  // probes from the outer table are in the same order as the key of the
  // inner table. Indicate this in the nested join relexpr so we can
  // report this information in EXPLAIN.
  if(planNumber != 0)
  {
    setProbesInOrder(TRUE);
  }

  return hasOptSol;

} // NestedJoin::findOptimalSolution()

NABoolean NestedJoin::currentPlanIsAcceptable(Lng32 planNo,
            const ReqdPhysicalProperty* const rppForMe) const
{
  // ---------------------------------------------------------------------
  // Check whether the user wants to enforce a particular plan type.
  // ---------------------------------------------------------------------

  // If nothing is being forced, return TRUE now.
  if (rppForMe->getMustMatch() == NULL)
    return TRUE;

  // Check for the correct forced plan type.

  JoinForceWildCard::forcedPlanEnum forcePlanToken =
       getParallelJoinPlanToEnforce(rppForMe);

  NABoolean OCR_CQD_is_on = ActiveSchemaDB()->getDefaults().
                getAsLong(NESTED_JOINS_OCR_MAXOPEN_THRESHOLD) > 0;

  switch (forcePlanToken)
  {
    case JoinForceWildCard::FORCED_PLAN0:
      if (planNo != 0)
        return FALSE;
      break;
    case JoinForceWildCard::FORCED_PLAN1:
      // if we are forcing plan #1 - a plan which passes any
      // sort order from the right child to the left - then we
      // will do this as plan #0 instead, so we will not need
      // to generate plan #1.
      if ( planNo == 0)
         return TRUE;
      
      // If CQD NESTED_JOINS_OCR is on, and PLAN1 is forced, then we will
      // do OCR. In this case, we can not ignore plan1 
      if (planNo == 1 AND OCR_CQD_is_on == TRUE)
        return TRUE;
      else
        return FALSE;
      break;
    case JoinForceWildCard::FORCED_PLAN2:
      if (planNo != 2)
        return FALSE;
      break;
    case JoinForceWildCard::FORCED_PLAN3:
      // if we are forcing plan #3 - a plan which passes
      // a required sort order type of ESP_VIA_SORT -
      // then we will do this as plan #2 instead, so we will not need
      // to generate plan #3.
      if (planNo != 2)
        return FALSE;
      break;
    case JoinForceWildCard::FORCED_TYPE1:
      // Plan0 and the old Plan1 plans do not require the left child
      // to be partitioned in any particular way, so they are "type 2".

      // But the OCR embedded in Plan1 is "type 1".
      if (planNo == 0) 
        return FALSE;

      if (planNo == 1 AND OCR_CQD_is_on == FALSE)
        return FALSE;

      break;
    case JoinForceWildCard::FORCED_TYPE2:
      // Nested Join plans #2 and #3 require the left child partitioning
      // to be a grouping of the right child partitioning, so that the
      // right child partitions will only see probes from at most one
      // left child partition. So, these are type 1 joins.
      if ((planNo == 2) OR (planNo == 3))
        return FALSE;

      // The following conditions test if OCR is sought for. If so we need 
      // return FALSE here, becaues OCR is "type-1", 
      if (planNo == 1 AND OCR_CQD_is_on == TRUE)
        return FALSE;
      break;
    case JoinForceWildCard::ANY_PLAN:
      // Any plan satisfies this - break out so we'll return TRUE
      break;

    case JoinForceWildCard::FORCED_INDEXJOIN:
      // this is a forced index join
      break;

    default:
      return FALSE; // must be some option that Nested Join doesn't support
  }

  // If we get here, the plan must have passed all the checks.
  return TRUE;

} // NestedJoin::currentPlanIsAcceptable()

NABoolean NestedJoin::OCBJoinIsFeasible(const Context* myContext) const
{
  // Here we do the main analysis if OCB join can be considered.
  // The main purpose of this method is to guarantee correctness
  // by avoiding duplicate results if the same tuple is sent to more
  // than one partition of inner child. The easiest way to avoid this
  // - not allowing split_top over inner child. For this we require
  // that the number of ESPs running OCB join is the same as the number
  // of partitions of inner child. Exception of this rule is when child
  // is hash2 partitioned and ratio of number of partitions over number
  // of ESPs is the power of 2. In this case hash2 partition grouping
  // can guarantee that each partition will be accessed by only one ESP.
  // We can also add a check if prefix of inner child clustering key
  // is covered by join predicates, characteristic input and constant.
  // this would guarantee ordered nested join. If not each probe will
  // cause full inner child scan. this will cause very expensive plan
  // that will most probably lose when compared with other plans for this
  // join. Therefore, adding this type of check here will avoid costing
  // of obviously expensive plan. This is planned for next phase.

  const ReqdPhysicalProperty* rppForMe =
    myContext->getReqdPhysicalProperty();

  if ( rppForMe->executeInDP2() == TRUE )
    return FALSE;

  PartitioningRequirement *partReq =
    rppForMe->getPartitioningRequirement();

  // For now we allow only fuzzy (or no )
  // partitioning requirement without specified partitioning key,
  // when there is no sort requirement for me.
  if ( (myContext->requiresOrder() AND
        partReq AND 
        partReq->isRequirementExactlyOne() == FALSE) OR
       (partReq AND
        partReq->isRequirementApproximatelyN() AND
        NOT partReq->partitioningKeyIsSpecified()
       )
     )
  {
    if (CmpCommon::getDefault(COMP_BOOL_134) == DF_OFF)
    {
      // ignore Scan check for now. To force a check for
      // the number of partitions of inner child we need to set
      // COMP_BOOL_134 to ON
      return TRUE;
    }

    // inner child should be a Scan node. For now just check
    // the number of base tables, more complex check will be
    // implemented later. For now to guarantee correctness we
    // require all available indexes to be hash2 partitioned
    // with the number of partitions greater or equal to the
    // number of ESPs with ratio being power of 2.
    if (child(1).getGroupAttr()->getNumBaseTables() != 1 )
    {
      return FALSE;
    }

    PartitioningFunction *rightPartFunc = NULL;
    Lng32 numOfESPs = rppForMe->getCountOfPipelines();
    const SET(IndexDesc *) &availIndexes=
      child(1).getGroupAttr()->getAvailableBtreeIndexes();

    for (CollIndex i = 0; i < availIndexes.entries(); i++)
    {
      rightPartFunc = availIndexes[i]->getPartitioningFunction();

      if ( rightPartFunc )
      {
        Lng32 numOfparts = rightPartFunc->getCountOfPartitions();

        if ( numOfparts < numOfESPs )
        {
          // don't use OCB if the number of partition of inner child
          // is less than the number of ESPs.
          return FALSE;
        }

        if ( numOfparts == numOfESPs )
          continue;

        // if we came here then the number of partition of inner child
        // is greater than the number of ESPs. Allow OCB only if inner
        // child is hash2 partitioned and the ratio is the power of 2
        UInt32 d = numOfparts%numOfESPs;
        if ( d > 0 )
        {
          // don't use OCB if the number of partition of inner child
          // is not multiple of number of ESPs.
          return FALSE;
        }

        d = numOfparts/numOfESPs;
        if ( CmpCommon::getDefault(COMP_BOOL_161) == DF_ON AND (d & (d-1)) )
        {
          // if at least one bit of d&(d-1) is not 0 then d is not
          // power of 2, don't use OCB. Do this only when CB_161 is on (
          // default is off). Need this fix for 3-drive Blades system.
          return FALSE;
        }
        else
        {
          // return true if inner child is hash2 partitioned
          // otherwise FALSE will be returned
          if (rightPartFunc->isAHash2PartitioningFunction())
            continue;
          else
            return FALSE;
        }
      }
      else
      {
       // no partitioning function found, don't use OCB join
       DBGLOGMSG("  *** Index PartFunc is NULL - don't use OCB ");
       return FALSE;
      }
    } //end of loop over available indexes

    if ( rightPartFunc )
    {
      // Since we come here and didn't return FALSE - all
      // indexes are good and we can use OCB
      return TRUE;
    }
    else
    {
      // no available indexes. If index was available but not good
      // then we would have returned FALSE earlier.
      return FALSE;
    }
  }
  else
  {
    // don't use OCB with partitioning requirement
    return FALSE;
  }
  // we shouldn't have come here, but just in case for the future
  return FALSE;
}

// Here are the conditions checked in this routine:
// 1. Number of base tables on right must be 1;
// 2. We only look at the clustering partitioning function on the right,
//    not at any indexes;
// 3. execution location must not be DP2;
// 4. The base table on right is hash2 partitioned;
// 5. partreq is fuzzy, NULL or hash2;
// 6. The join predicates cover the partitioning key of the table on right;
// 7. The base table on right has more than one partition.
// 8. The join columns are not skewed (only if SkewBuster for NJ is disabled)
//
// If the parent requires a specific part function, we don't check whether 
// that matches the partitioning function on the right (if it doesn't 
// we can't use OCR and may be better off with a TYPE2 join)

NABoolean NestedJoin::OCRJoinIsFeasible(const Context* myContext)  const
{
  // Inner child should be a Scan node. For now just check
  // the number of base tables, more complex check will be
  // implemented later. 
  if (child(1).getGroupAttr()->getNumBaseTables() != 1 )
  {
    return FALSE;
  }

  const ReqdPhysicalProperty* rppForMe =
     myContext->getReqdPhysicalProperty();

  // Do not consider OCR if we are in DP2.
  if ( rppForMe->executeInDP2() )
     return FALSE;

  PartitioningRequirement *partReq =
    rppForMe->getPartitioningRequirement();

  // For now we allow only fuzzy part req without specified partitioning key, 
  // no part req, or hash2 part req.
  if (NOT ((partReq == NULL)
           OR
           (partReq->isRequirementApproximatelyN() AND
            NOT partReq->partitioningKeyIsSpecified())
           OR
           (partReq-> castToRequireHash2() != NULL)))
     return FALSE;

  // Do not consider OCR if the join predicates do not cover part key of the 
  // inner (correctness).
  if ( JoinPredicateCoversChild1PartKey() == FALSE )
     return FALSE;

   PartitioningFunction *rightPartFunc =
         getClusteringIndexPartFuncForRightChild();

  // Do not consider OCR if the inner is not hash2 partitioned
   if ( rightPartFunc->isAHash2PartitioningFunction() == FALSE )
      return FALSE;

  // Do not consider OCR if the inner is single partitioned
  if ( rightPartFunc->getCountOfPartitions() == 1 )
     return FALSE;

  // IF N2Js, which demand opens, are not to be disabled, make sure 
  // OCR is allowed only if the threshold of #opens is reached.
  if (CmpCommon::getDefault(NESTED_JOINS_NO_NSQUARE_OPENS) == DF_OFF) {

     Lng32 threshold = ActiveSchemaDB()->getDefaults().getAsLong(NESTED_JOINS_OCR_MAXOPEN_THRESHOLD);

     // the threshold is -1, do not do OCR.
     if ( threshold == -1 )
        return FALSE;

     // If the total # of opens for this nested join is less than the threshold, return FALSE
     if (CURRSTMT_OPTDEFAULTS->getMaximumDegreeOfParallelism() * 
           rightPartFunc->getCountOfPartitions() < threshold)
        return FALSE;
  }

  Lng32 antiskewSkewEsps =
       ActiveSchemaDB()->getDefaults().getAsLong(NESTED_JOINS_ANTISKEW_ESPS);

  if ( antiskewSkewEsps > 0 )
     return TRUE; // the skew busting for OCR is enabled. No more check.

  //
  // If the join is on a skewed set of columns from the left child and the
  // nested join probing cache is not applicable, then do not try OCR.
  //
  if ( isProbeCacheApplicable(rppForMe->getPlanExecutionLocation()) == FALSE ) {
     ValueIdSet joinPreds = getOriginalEquiJoinExpressions().getTopValues();

      double threshold =
       (ActiveSchemaDB()->getDefaults().getAsDouble(SKEW_SENSITIVITY_THRESHOLD))                             /
       rppForMe->getCountOfPipelines();

     SkewedValueList* skList = NULL;

     if ( childNodeContainSkew(0, joinPreds, threshold, &skList) == TRUE )
        return FALSE;
  }


  return TRUE;
}
          
NABoolean NestedJoin::JoinPredicateCoversChild1PartKey() const
{
   PartitioningFunction *rightPartFunc =
         getClusteringIndexPartFuncForRightChild();

   if ( rightPartFunc == NULL )
      return FALSE;

   const ValueIdSet& equiJoinExprFromChild1AsSet =
        getOriginalEquiJoinExpressions().getBottomValues();

   ValueIdSet child1PartKey(rightPartFunc->getPartitioningKey());

  // The equi-join predicate should contain the part key for OCR
  // to guarantee the correct result. This is because each row from
  // the outer table will be partitioned using child1's partitioning
  // function. If some column in child1's partitioning key is not
  // covered by the equi-join predicate, then some column from child0
  // will not be partitioned, this implies that the row can go to an
  // incorrect partition.

  if ( equiJoinExprFromChild1AsSet.contains(child1PartKey) ) {
      return TRUE;
  }

  // double check the uncovered park key columns here. If they are 
  // all constants, then the join columns still cover the part keys.

  // first get the columns that are covered.
  ValueIdSet coveredPartKey(child1PartKey);
  coveredPartKey.intersectSet(equiJoinExprFromChild1AsSet);

  // second get the columns that are not covered.
  ValueIdSet unCoveredPartKey(child1PartKey);
  unCoveredPartKey.subtractSet(coveredPartKey);

  // remove all columns that are constants.
  unCoveredPartKey.removeConstExprReferences(FALSE /*consider expressions*/);

  // if nothing is left, we return true.
  if ( unCoveredPartKey.entries() == 0 )
     return TRUE;

  return FALSE;
}

NABoolean NestedJoin::okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace*, /*IN, ignored*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/)
{
  const ReqdPhysicalProperty* rppForMe =
     myContext->getReqdPhysicalProperty();

  // CS or REPLICA: do not consider ESP parallelism if we are in DP2.
  if ( rppForMe->executeInDP2() == TRUE )
     return FALSE;

  // rowsetIterator cannot be parallelized. Counting logic for rowNumber
  // is not designed to work in nodes that are executing in parallel.
  if (isRowsetIterator())
    return FALSE;

  // merge statement cannot be ESP parallelised if it has an INSERT clause.
  // But if it is forced using CB189, then parallelise it at user's own risk.
  if (isTSJForMerge())
    {
      if ((isTSJForMergeWithInsert()) &&
	  (CmpCommon::getDefault(COMP_BOOL_189) == DF_OFF))
	return FALSE;
    }

  NABoolean result = FALSE;

  DefaultToken parallelControlSettings =
    getParallelControlSettings(rppForMe,
                               numOfESPs,
                               allowedDeviation,
                               numOfESPsForced);

  if (parallelControlSettings == DF_OFF)
  {
    result = FALSE;
  }

  else if ( (parallelControlSettings == DF_MAXIMUM)  AND
             CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible()
           )
  {
    numOfESPs = rppForMe->getCountOfPipelines();
    // currently, numberOfPartitionsDeviation_ is set to 0 in
    // OptDefaults when ATTEMT_ESP_PARALLELISM is 'MAXIMUM'
    allowedDeviation =  CURRSTMT_OPTDEFAULTS->numberOfPartitionsDeviation();

    // allow deviation by default
    if (CmpCommon::getDefault(COMP_BOOL_62) == DF_OFF)
    {
      EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
      EstLogPropSharedPtr child0OutputLogProp = child(0).outputLogProp(inLogProp);
      const CostScalar child0RowCount =
        (child0OutputLogProp->getResultCardinality()).minCsOne();
      if ( child0RowCount.getCeiling() <
            MINOF(numOfESPs,CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold())
         )
      {
        // Fewer outer table rows then pipelines - allow one or more parts
        allowedDeviation = 1.0;
      }
    }
    result = TRUE;
  }

  else if (parallelControlSettings == DF_ON)
  {
    // Either user wants to try ESP parallelism for all operators,
    // or they are forcing the number of ESPs for this operator.
    // Set the result to TRUE. If the number of ESPs is not being forced,
    // set the number of ESPs that should be used to the maximum number.
    // Set the allowable deviation to either what we get from the
    // defaults table, or a percentage that allows any
    // number of partitions from 2 to the maximum number. i.e. allow
    // the natural partitioning of the child as long as the child is
    // partitioned and does not have more partitions than max pipelines.
    // NEW HEURISTIC: If there are fewer outer table rows than the number
    // of pipelines, then set the deviation to allow any level of natural
    // partitioning, including one. This is because we don't want to
    // repartition so few rows to get more parallelism, since we would
    // end up with a lot of ESPs doing nothing.
    if (NOT numOfESPsForced)
    {
      // Determine the number of outer table rows
      EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
      EstLogPropSharedPtr child0OutputLogProp = child(0).outputLogProp(inLogProp);
      const CostScalar child0RowCount =
        (child0OutputLogProp->getResultCardinality()).minCsOne();
      numOfESPs = rppForMe->getCountOfPipelines();
      if (child0RowCount.getCeiling() < numOfESPs)
      {
        // Fewer outer table rows then pipelines - allow one or more parts
        allowedDeviation = 1.0;
      }
      else
      {
        if ( CURRSTMT_OPTDEFAULTS->deviationType2JoinsSystem() )
        {
          // -------------------------------------------------------------------
          // A value for NUM_OF_PARTS_DEVIATION_TYPE2_JOINS exists.
          // -------------------------------------------------------------------
          allowedDeviation = CURRSTMT_OPTDEFAULTS->numOfPartsDeviationType2Joins();
        }
        else
        {
          // Need to make 2 the minimum number of parts to support. Use
          // 1.99 to protect against rounding errors.
          allowedDeviation =
            ((float)numOfESPs - 1.99f) / (float)numOfESPs;
        }
      } // end if fewer outer table rows than pipelines
    } // end if number of ESPs not forced
    result = TRUE;
  }
  else
  {
    // Otherwise, the user must have specified "SYSTEM" for the
    // ATTEMPT_ESP_PARALLELISM default. This means it is up to the
    // optimizer to decide.
    EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
    EstLogPropSharedPtr child0OutputLogProp = child(0).outputLogProp(inLogProp);
    CostScalar child0Rows =
        (child0OutputLogProp->getResultCardinality()).minCsOne();

    if (updateTableDesc())
    {
      // -----------------------------------------------------------------
      // NJ on top of an insert/update/delete statement, make the
      // left side match the partitioning scheme of the updated table
      // -----------------------------------------------------------------
      const PartitioningFunction *updPartFunc =
        updateTableDesc()->getClusteringIndex()->getPartitioningFunction();

      if (updPartFunc == NULL OR
          updPartFunc->getCountOfPartitions() == 1)
      {
        numOfESPs = 1;
      }
      else // Target table is partitioned
      {
        // get an estimate of how many rows and what rowsize will be
        // returned from the left child (the select part of the update)
        RowSize child0RowSize = child(0).getGroupAttr()->getRecordLength();
        CostScalar child0TableSize = child0Rows * child0RowSize;

        // now divide the amount of data returned by the left by a
        // default constant to determine how many ESPs we would like
        // to work on this
        double sizePerESP = CURRSTMT_OPTDEFAULTS->updatedBytesPerESP();
        double numOfESPsDbl = ceil(child0TableSize.value() / sizePerESP);
        Lng32 countOfPipelines = rppForMe->getCountOfPipelines();

	// require no more ESPs than there are pipelines in the system,
	if ( numOfESPsDbl > countOfPipelines )
	   numOfESPs = countOfPipelines;
	else
	   numOfESPs = (Lng32) numOfESPsDbl;

	// don't ask for more ESPs than there are updated partitions,
        // the ESPs must be a grouping of that partitioning scheme
	Lng32 countOfPartitions = updPartFunc->getCountOfPartitions();
	if ( numOfESPs > countOfPartitions )
	   numOfESPs = countOfPartitions;

        // This in an adjustment to allow control of parallelism by CQDs
        // MINIMUM_ESP_PARALLELISM and NUMBER_OF_ROWS_PARALLEL_THRESHOLD
        // Without this adjustment insert/select will be serial in most
        // of the cases because the number of ESPs calculated above
        // may not allow partition grouping. This adjustment is controled
        // by CQD COMP_BOOL_30 (ON by default)
        if (CmpCommon::getDefault(COMP_BOOL_30) == DF_ON)
        {
          const CostScalar rowCount =
            (CmpCommon::getDefault(COMP_BOOL_125) == DF_ON)
            ? child0Rows + (getGroupAttr()->outputLogProp(inLogProp)->
              getResultCardinality()).minCsOne()
            : child0Rows;

          const CostScalar numberOfRowsThreshold =
            CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold();

          if ( rowCount > numberOfRowsThreshold )
          {
            Lng32 optimalNumOfESPs = MINOF(countOfPipelines,
              (Lng32)(rowCount / numberOfRowsThreshold).value());

            // make numOfESPs as available level of parallelism
            // 16*N, 8*N, 4*N,..., N,1 where N is the number of segments
            Lng32 i = CURRSTMT_OPTDEFAULTS->getMaximumDegreeOfParallelism();
            if (numOfESPs > i )
            {
              numOfESPs = i;
            }
            else
            {
              Lng32 MinParallelism =
                MAXOF( MAXOF(CURRSTMT_OPTDEFAULTS->getMinimumESPParallelism(),
                             optimalNumOfESPs),
                       numOfESPs);

              while(i > MinParallelism)
                i/=2;

              optimalNumOfESPs = (i<MinParallelism) ? i*=2 : i;
              numOfESPs = MAXOF(numOfESPs, optimalNumOfESPs);
            }
          }
        }

      } // we can choose the number of partitions for the update

      // For write operations, we always want to specify the number
      // of ESPs we want to the child, and this is an exact number,
      // so the deviation is 0.
      allowedDeviation = 0.0;
	  if ( numOfESPs <= 1 )
		result = FALSE;
	  else
		result = TRUE;
    } // end if Nested Join for a write operation
    else
    {
      // Nested Join for a read operation.

      // Return TRUE if the number of
      // rows returned by child(0) exceeds the threshold from the
      // defaults table. The recommended number of ESPs is also computed
      // to be 1 process per <threshold> number of rows. This is then
      // used to indicate the MINIMUM number of ESPs that will be
      // acceptable. This is done by setting the allowable deviation
      // to a percentage of the maximum number of partitions such
      // that the recommended number of partitions is the lowest
      // number allowed. We make the recommended number of partitions
      // a minimum instead of a hard requirement because we don't
      // want to be forced to repartition the child just to get "less"
      // parallelism.

     CostScalar rowCount = child0Rows;

      // This is to test better parallelism taking into account
      // not only child0 but also this operator cardinality
      // this could be important for joins
      if(CmpCommon::getDefault(COMP_BOOL_125) == DF_ON)
      {
        rowCount += (getGroupAttr()->outputLogProp(inLogProp)->
          getResultCardinality()).minCsOne();
      }

      const CostScalar numberOfRowsThreshold =
        CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold();

      if (rowCount > numberOfRowsThreshold)
      {
        numOfESPs = rppForMe->getCountOfPipelines();

        allowedDeviation = (float) MAXOF(1.001 -
          ceil((rowCount / numberOfRowsThreshold).value()) / numOfESPs,0);
        result = TRUE;
      }
      else
      {
        result = FALSE;
      }
    } // end if for a read operation

  } // end if the user let the optimizer decide

  return result;

} // NestedJoin::okToAttemptESPParallelism()

//<pb>
//==============================================================================
//  Synthesize physical properties for nested join operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
NestedJoin::synthPhysicalProperty(const Context *myContext,
                                  const Lng32     planNumber,
                                  PlanWorkSpace  *pws)
{
  const PhysicalProperty* const sppOfLeftChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(0);
  const PhysicalProperty* const sppOfRightChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(1);
  const ReqdPhysicalProperty* rppForMe =
          myContext->getReqdPhysicalProperty();

  ValueIdList newSortKey(sppOfLeftChild->getSortKey());
  SortOrderTypeEnum newSortOrderType = sppOfLeftChild->getSortOrderType();
  PartitioningFunction* newDp2SortOrderPartFunc =
                          sppOfLeftChild->getDp2SortOrderPartFunc();

  // ---------------------------------------------------------------------
  // add the sort columns of the second child to the ones of the first
  // child, but only if the sorted columns of the first child are unique
  // ---------------------------------------------------------------------

  NABoolean canAppendRightColumns = FALSE;
  NABoolean leftChildSortColsAreReq = FALSE;
  NABoolean rightChildSortColsAreReq = FALSE;
  NABoolean childSortOrderTypesAreSame = FALSE;

  // should we even bother?
  if (sppOfRightChild->isSorted() AND
      NOT rowsFromLeftHaveUniqueMatch() AND
      NOT (getGroupAttr()->getMaxNumOfRows() <= 1))
  {
      GroupAttributes *leftGA = child(0).getGroupAttr();
      // can append right sort cols, if the left sort cols form a
      // candidate key
      // Example why this is: imagine the left table ordered by (a,b)
      // and the right table ordered by c.
      //
      // If (a,b) is unique, the output of a join might look like
      //
      //  a   b   c        first child:      second child:
      // --  --  --         a   b              c
      //  1   1   1        --  --             --
      //  1   1   2         1   1              1
      //  1   2   1         1   2              2
      //  1   2   2
      //
      // On the other hand, if (a,b) is not unique, you might get
      //
      //  a   b   c        left table:      right table:
      // --  --  --         a   b              c
      //  1   1   1        --  --             --
      //  1   1   2         1   1              1
      //  1   2   1         1   2              2
      //  1   2   2         1   2
      //  1   2   1
      //  1   2   2
      //
      // which is of course not ordered by (a,b,c)
      //
      // Left join special case:
      //
      // For nested left-join the same rules may be applied.
      // The fact that left-joins produce null values in the right side, does
      // not violate the required order. In this case the order is defined by
      // the columns of the left child.
      // Example:
      // The left-join join predicate is (c between a and b) and  (a,b) is unique.
      // The output of a join might look like
      //
      //  a   b   c	    left child:         right child:
      // --  --  --	      a   b              c
      //  1   2   1           --  --             --
      //  1   2   2	      1   2              1
      //  2   1   NULL	      2   1              2
      //  2   3   2	      2   3
      //
      // Notice that whenever the left row has no matching rows in the right table
      // (the right child columns are null) it produces only a single row in the result set
      // and therefor the order is defined by the left child columns only.

    ValueIdSet leftSortCols;

    // make the list of sort cols into a ValueIdSet
    leftSortCols.insertList(sppOfLeftChild->getSortKey());

    // check for uniqueness of the sort columns
    if (leftGA->isUnique(leftSortCols))
    {
      // Determine if the sort cols are required from the left and
      // right children.
      if (rppForMe->getSortKey() AND
          (rppForMe->getSortKey()->entries() > 0))
      {
        ValueIdList reqdOrder0, reqdOrder1;
        if (splitOrderReq(*(rppForMe->getSortKey()),
                          reqdOrder0,reqdOrder1))
        {
          if (NOT reqdOrder0.isEmpty())
            leftChildSortColsAreReq = TRUE;
          if (NOT reqdOrder1.isEmpty())
            rightChildSortColsAreReq = TRUE;
        }
      }
      if ((NOT (leftChildSortColsAreReq AND rightChildSortColsAreReq)) AND
          rppForMe->getArrangedCols() AND
          (rppForMe->getArrangedCols()->entries() > 0))
      {
        ValueIdSet reqdArr0, reqdArr1;
        if (splitArrangementReq(*(rppForMe->getArrangedCols()),
                                reqdArr0,reqdArr1))
        {
          if (NOT reqdArr0.isEmpty())
            leftChildSortColsAreReq = TRUE;
          if (NOT reqdArr1.isEmpty())
            rightChildSortColsAreReq = TRUE;
        }
      }
      // if we passed the uniqueness test because the left
      // child has a cardinality constraint of at most one row,
      // then only include the left child sort key columns if
      // there was a requirement for them.
      if ((leftGA->getMaxNumOfRows() <= 1) AND
          NOT newSortKey.isEmpty())
      {
        if (NOT leftChildSortColsAreReq)
        {
          newSortKey.clear();
          newSortOrderType = NO_SOT;
          newDp2SortOrderPartFunc = NULL;
        }
      } // end if there was a left child card constraint
      // If we aren't passing up any sort key columns from the left
      // child, then simply set the sort key columns and sort order
      // type to those of the right child. Otherwise, we need to
      // append the right child sort key columns, so set the flag to TRUE.
      if (newSortKey.isEmpty())
      {
        // Only use the right child sort key columns without
        // any left child sort key columns if the sort order type
        // of the right child is not DP2.
        if (sppOfRightChild->getSortOrderType() != DP2_SOT)
        {
          newSortKey = sppOfRightChild->getSortKey();
          newSortOrderType = sppOfRightChild->getSortOrderType();
        }
      }
      else
      {
        canAppendRightColumns = TRUE;
        // The child sort order types are the same if they are equal and both
        // children's dp2SortOrderPartFunc are the same (as in both being
        // NULL), or they are both not null but they are equivalent.
        if ((sppOfLeftChild->getSortOrderType() ==
             sppOfRightChild->getSortOrderType()) AND

            ((sppOfLeftChild->getDp2SortOrderPartFunc() ==
             sppOfRightChild->getDp2SortOrderPartFunc()) OR
             ((sppOfLeftChild->getDp2SortOrderPartFunc() != NULL) AND
              (sppOfRightChild->getDp2SortOrderPartFunc() != NULL) AND
              (sppOfLeftChild->getDp2SortOrderPartFunc()->
                 comparePartFuncToFunc(
                  *sppOfRightChild->getDp2SortOrderPartFunc()) == SAME)
             )
            )
           )
         childSortOrderTypesAreSame = TRUE;
      }
    } // end if left child sort cols are unique
  } // right child table is sorted


  // We can only append the sort key columns from the right child
  // if it passed the tests above, AND the sort order types are the
  // same OR there is a requirement for the right child sort key
  // columns AND the right child sort order type is not DP2.
  if (canAppendRightColumns AND
      (childSortOrderTypesAreSame OR
       (rightChildSortColsAreReq AND
        sppOfRightChild->getSortOrderType() != DP2_SOT))
     )
  {
    // append right child sort key columns
    const ValueIdList & rightSortKey = sppOfRightChild->getSortKey();

      if (!isLeftJoin())
      {
	for (Lng32 i = 0; i < (Lng32)rightSortKey.entries(); i++)
	  newSortKey.insert(rightSortKey[i]);
      }
      //++MV
      else
      {
	// For left-join we need to translate the right child sort key
	// to the left join output value id's in order to allow
	// the cover test on the left-join to pass .
	ValueIdList newRightSortKey;
	ValueIdMap &map = rightChildMapForLeftJoin();
	map.mapValueIdListUp(newRightSortKey, rightSortKey);
	for (Lng32 i = 0; i < (Lng32)newRightSortKey.entries(); i++)
	  newSortKey.insert(newRightSortKey[i]);
      }
      //--MV

    // The sort order type stays that of the left child, unless the
    // the left child sort order type is ESP_NO_SORT and the right
    // child sort order type is ESP_VIA_SORT. In this case, we want
    // to set the new sort order type to ESP_VIA_SORT, since a sort was
    // done in producing at least part of the key, and setting the
    // sort order type to ESP_NO_SORT would not be correct. If the
    // left child sort order type was DP2 then this is the new sort
    // order type because an executor process order is not produced.
    if ((sppOfLeftChild->getSortOrderType() == ESP_NO_SORT_SOT) AND
        (sppOfRightChild->getSortOrderType() == ESP_VIA_SORT_SOT))
      newSortOrderType = ESP_VIA_SORT_SOT;
  } // end if we can add sort key columns from the right child

  // ---------------------------------------------------------------------
  // synthesize plan execution location
  // ---------------------------------------------------------------------

  PlanExecutionEnum loc = sppOfLeftChild->getPlanExecutionLocation();

  // merge statement cannot be ESP parallelised if it has an INSERT clause.
  // But if it is forced using CB189, then parallelise it at user's own risk.
  if (isTSJForMerge())
    {
      if ((isTSJForMergeWithInsert()) &&
	  (CmpCommon::getDefault(COMP_BOOL_189) == DF_OFF))
	loc = EXECUTE_IN_MASTER;
    }

  // ||opt make a method to combine left and right location

  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(myContext,
                                                             planNumber,
                                                             pws);

  // ---------------------------------------------------------------------
  // The result of a nested join has the sort order of the outer
  // table. The nested join maintains the partitioning of the outer table
  // for plans 0,1,2,3. Otherwise (plan 4) it's outer child broadcast and
  // we take the properties of the inner child as nested join properties
  // ---------------------------------------------------------------------
  PhysicalProperty* sppForMe = NULL;
  NABoolean expldocbjoin = FALSE;
  if ( planNumber < 4 )
  {
    if  (rppForMe->executeInDP2() && updateTableDesc() &&
         rppForMe->getPushDownRequirement())
    {  
      // Push down IUD queries involving MVs.
      //
      // Pick the right child's partition function when 
      // the execution location is DP2, the updateTableDesc is not NULL
      // and the push-down requirement is not NULL. When all three
      // conditions are met, then this join is an implementation of
      // in-DP2 IUD involving MVs. We can safely use the right partfunc
      // because we have already verified the required partition function
      // is the same as the partition function of the inner table (the 
      // IUD table) in childIndex=0 block of code. 
      //
      // In all other cases, the rg checkFeasibilty test will fail and
      // thus we will not get to this call.

      sppForMe = new (CmpCommon::statementHeap()) PhysicalProperty(
         newSortKey,
         newSortOrderType,
         newDp2SortOrderPartFunc,
         sppOfRightChild->getPartitioningFunction(),
         loc,
         combineDataSources(sppOfLeftChild->getDataSourceEnum(),
                            sppOfRightChild->getDataSourceEnum()),
         sppOfRightChild->getIndexDesc(),
         NULL,
         sppOfRightChild->getPushDownProperty()
        );
   } else {
      sppForMe = new (CmpCommon::statementHeap()) PhysicalProperty(
         newSortKey,
         newSortOrderType,
         newDp2SortOrderPartFunc,
         sppOfLeftChild->getPartitioningFunction(),
         loc,
         combineDataSources(sppOfLeftChild->getDataSourceEnum(),
                            sppOfRightChild->getDataSourceEnum()),
         sppOfLeftChild->getIndexDesc(),
         sppOfLeftChild->getPartSearchKey(),
         sppOfLeftChild->getPushDownProperty(),
		 sppOfLeftChild->getexplodedOcbJoinProperty());
   }
  }
  else
  {
    sppForMe =
    new (CmpCommon::statementHeap()) PhysicalProperty(
         newSortKey,
         newSortOrderType,
         newDp2SortOrderPartFunc,
         sppOfRightChild->getPartitioningFunction(),
         loc,
         combineDataSources(sppOfLeftChild->getDataSourceEnum(),
                            sppOfRightChild->getDataSourceEnum()),
         sppOfRightChild->getIndexDesc(),
         sppOfRightChild->getPartSearchKey(),
         sppOfRightChild->getPushDownProperty(), expldocbjoin);
  }

  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  delete sppTemp;
  return sppForMe;

} //  NestedJoin::synthPhysicalProperty()

// ---------------------------------------------------------------------
// Performs mapping on the partitioning function, from the
// nested join to the designated child.
// ---------------------------------------------------------------------
PartitioningFunction* NestedJoin::mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0)
{
  ValueIdMap map(getOriginalEquiJoinExpressions());
  PartitioningFunction* newPartFunc =
      partFunc->copyAndRemap(map,rewriteForChild0);

  SkewedDataPartitioningFunction* oldSKpf = NULL;
  SkewedDataPartitioningFunction* newSKpf = NULL;

  if ( rewriteForChild0 == FALSE /* map for child 1 */ AND
       (oldSKpf=(SkewedDataPartitioningFunction*)
                  (partFunc->castToSkewedDataPartitioningFunction())) AND
       (newSKpf=(SkewedDataPartitioningFunction*)
                  (newPartFunc->castToSkewedDataPartitioningFunction()))
     )
  {
     if ( oldSKpf->getSkewProperty().isUniformDistributed() ) {
       skewProperty newSK(oldSKpf->getSkewProperty());
       newSK.setIndicator(skewProperty::BROADCAST);
       newSKpf->setSkewProperty(newSK);
     }
  } else {
       // map for child 0. Do nothing
  }


  return newPartFunc;

} // end NestedJoin::mapPartitioningFunction()

//<pb>

// -----------------------------------------------------------------------
// member functions for class NestedJoinFlow
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// NestedJoinFlow::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
NestedJoinFlow::costMethod() const
{
  static THREAD_P CostMethodNestedJoinFlow *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodNestedJoinFlow;
  return m;
}

//<pb>

// -----------------------------------------------------------------------
// member functions for class MergeJoin
// -----------------------------------------------------------------------

// ---------------------------------------------------------------------
// Performs mapping on the partitioning function, from the
// merge join to the designated child.
// ---------------------------------------------------------------------
PartitioningFunction* MergeJoin::mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0)
{
  ValueIdMap map(getEquiJoinExpressions());
  return partFunc->copyAndRemap(map,rewriteForChild0);
} // end MergeJoin::mapPartitioningFunction()

// -----------------------------------------------------------------------
// Determine if the merge join will be able to satisfy the parent
// partitioning requirements.
// -----------------------------------------------------------------------
NABoolean MergeJoin::parentAndChildPartReqsCompatible(
                       const ReqdPhysicalProperty* const rppForMe) const
{
  PartitioningRequirement* partReq = rppForMe->getPartitioningRequirement();

  // If there are any parent partitioning requirements, then check them.
  if (partReq != NULL)
  {
    ValueIdSet reqPartKey = partReq->getPartitioningKey();
    Lng32 reqPartCount = partReq->getCountOfPartitions();
    ValueIdSet joinLeftPartKey(getEquiJoinExprFromChild0());
    ValueIdSet joinRightPartKey(getEquiJoinExprFromChild1());

    if (partReq->isRequirementFullySpecified())
    {
      // The parent's required partitioning columns must be a subset
      // of the either the left or right child's join columns, if we
      // are to run in parallel.
      // Contains will always return TRUE if the required part key
      // is the empty set.
      if (NOT (joinLeftPartKey.contains(reqPartKey) OR
               joinRightPartKey.contains(reqPartKey)))
      {
        // Parent required part key can only be satisfied with a
        // single partition partitioning function.
        // See if this is allowed by the parent requirement
        if (reqPartCount > EXACTLY_ONE_PARTITION)
          return FALSE;
      }
    }
    else // fuzzy requirement
    {
      CMPASSERT(partReq->isRequirementApproximatelyN());

      // Did the parent specify a required partitioning key?
      if (reqPartKey.entries() > 0)
      { // yes
        joinLeftPartKey.intersectSet(reqPartKey);
        joinRightPartKey.intersectSet(reqPartKey);
        // If the required partitioning key columns and the join cols
        // are disjoint, then the requirement must allow a single
        // partition part func, because this will be the only way
        // to satisfy both the requirement and the join cols.
        if (joinLeftPartKey.isEmpty() AND
            joinRightPartKey.isEmpty() AND
            (reqPartCount != ANY_NUMBER_OF_PARTITIONS) AND
            ((reqPartCount -
              (reqPartCount * partReq->castToRequireApproximatelyNPartitions()->
                                       getAllowedDeviation())) >
             EXACTLY_ONE_PARTITION)
           )
          return FALSE;
      }
    } // end if fuzzy requirement
  } // end if there was a partitioning requirement for the join

  return TRUE;

}; // MergeJoin::parentAndChildPartReqsCompatible()



// -----------------------------------------------------------------------
// Given an input ordering and a set of potential merge join predicates,
// generate the new merge join sort orders for both the left and right children.
// The ordering should contain the leading prefix of the provided ordering
// which is referenced by the set of (merge join) predicates.
// -----------------------------------------------------------------------
void MergeJoin::generateSortOrders (const ValueIdList & ordering, /* input */
                                    const ValueIdSet & preds,     /* input */
                                    ValueIdList &leftSortOrder, /* output */
                                    ValueIdList &rightSortOrder,/* output */
                                    ValueIdList &orderedMJPreds,/* output */
                                    NABoolean &completelyCovered  /* output */
                                    ) const
{
  NABoolean done = FALSE;
  Lng32 i = 0;

  ValueId predId, referencedValId;
  ValueId leftOrderValId, rightOrderValId;
  NABoolean isOrderPreserving;
  OrderComparison order1,order2;

  while (!done && i < (Lng32)ordering.entries())
  {
    // Get the value id for the simplified form of the ordering expression.
    referencedValId = ordering[i].getItemExpr()->
                        simplifyOrderExpr(&order1)->getValueId();

    // Check whether the ordering expression is covered by one
    // of the merge join predicates.
    if (preds.referencesTheGivenValue (referencedValId, predId))
    {
      ItemExpr *predExpr = predId.getItemExpr();

      if (predExpr->isAnEquiJoinPredicate(child(0).getGroupAttr(),
                                          child(1).getGroupAttr(),
                                          getGroupAttr(),
                                          leftOrderValId, rightOrderValId,
                                          isOrderPreserving))
      {
        //
        // Fix solu 10-081117-7343 (R2.4 - Error 7000 - GenRelJoin.cpp:
        // Merge Join: expression not found).
        //
        // The root cause of the assertion is that mergeJoin::codeGen()
        // is not written to deal with constant in the orderMJPreds.
        // Unfortunately that method is not easy to be extended for
        // constants, primarily because the constant is not added to
        // the maptable until at very end of the method in a codegen call
        // to the container expression. But without the generated Attributes
        // for the constant, the left dup check expression happening before
        // codeGen on the constant can not be generated.
        //
        // The fix in the optimizer will disable MJ when orderdMJpreds contain
        // a constant by clearing the orderJoinPreds produced in this method.
        // An empty orderJoinPreds set will force method
        // MergeJoin::createContextforChild() to produce no context
        // (thus no MJ plan).

         ValueIdSet mjPredAsVidSet(predId);

         for (Lng32 childIdx = 0; childIdx<predExpr->getArity(); childIdx++) {
            mjPredAsVidSet.insert(predExpr->child(childIdx)->
                                    castToItemExpr()->getValueId());
         }

         ItemExpr* constant = NULL;
         if (mjPredAsVidSet.referencesAConstValue(&constant) == TRUE)
         {
             leftSortOrder.clear();
             rightSortOrder.clear();
             orderedMJPreds.clear();
             break;
         }

        // Determine if the left side of the equijoin pred is an
        // expression that results in an inverse order.
        const ValueId & simpLeftOrderValId =
	  leftOrderValId.getItemExpr()->simplifyOrderExpr(&order2)->
            getValueId();
        // if the order expression and the left side equijoin expression
        // do not have the same order, then generate an INVERSE
        // expression on top of the left side equijoin expression.
        if (order1 != order2)
        {
          InverseOrder * leftInverseExpr =
            new(CmpCommon::statementHeap())
                InverseOrder (leftOrderValId.getItemExpr());
          leftInverseExpr->synthTypeAndValueId();
          leftOrderValId = leftInverseExpr->getValueId();
        }

        // Determine if the right side of the equijoin pred is an
        // expression that results in an inverse order.
        const ValueId & simpRightOrderValId =
	  rightOrderValId.getItemExpr()->simplifyOrderExpr(&order2)->
            getValueId();
        // if the order expression and the right side equijoin expression
        // do not have the same order, then generate an INVERSE
        // expression on top of the right side equijoin expression.
        if (order1 != order2)
        {
          InverseOrder * rightInverseExpr =
            new(CmpCommon::statementHeap())
                InverseOrder (rightOrderValId.getItemExpr());
          rightInverseExpr->synthTypeAndValueId();
          rightOrderValId = rightInverseExpr->getValueId();
        }

        // Prevent duplicate predicates; this is a correctness issue, but
        // more importantly helps to avoid assertions in pre-CodeGen.
        // The only way we could try to insert the same equijoin pred
        // into the orderedMJPreds would be if there was a duplicate
        // expression in the input ordering - this should not happen,
        // but we will check just to be sure.
        if ( NOT orderedMJPreds.contains(predId) )
        {
          leftSortOrder.insert (leftOrderValId);
          rightSortOrder.insert (rightOrderValId);
          orderedMJPreds.insert (predId);
        }
      }
      else
        // These preds should be equi-join predicates.
        ABORT ("Internal Error in MergeJoin::generateSortOrder.");
    }
    else
      done = TRUE;
    i++;
  }

  // Were all columns of the provided order covered by join predicates?
  if (i == (Lng32)ordering.entries())
    completelyCovered = TRUE;
  else
    completelyCovered = FALSE;

} // MergeJoin::generateSortOrders()

// -----------------------------------------------------------------------
// Generate an arrangement requirement. Used only for creating an
// arrangement requirement for the right child when we try the right
// child first.
// -----------------------------------------------------------------------
void MergeJoin::genRightChildArrangementReq(
                  const ReqdPhysicalProperty* const rppForMe,
                  RequirementGenerator &rg) const
{
  ValueIdSet rightJoinColumns;
  rightJoinColumns.insertList(getEquiJoinExprFromChild1());
  ValueIdList rightChildOrder;

  NABoolean reqOrderExists = FALSE;
  NABoolean reqOrderCompletelyCovered = FALSE;
  NABoolean reqArrangementExists = FALSE;
  if (rppForMe->getSortKey() AND
      (rppForMe->getSortKey()->entries() > 0))
  {
    reqOrderExists = TRUE;
  }
  if (rppForMe->getArrangedCols() AND
      (rppForMe->getArrangedCols()->entries() > 0))
  {
    reqArrangementExists = TRUE;
  }

  // If there is a required order, we need to make sure that for the
  // left child join columns that are a prefix of the req. order cols,
  // we require the corresponding right child join columns to be in
  // that order. If all of the required order cols are covered by
  // join columns, we can add the join columns as a required
  // arrangement. This will allow any join predicates for the excess
  // join columns to be used as merge join predicates.
  if (reqOrderExists)
  {
    ValueIdList leftChildOrder;
    ValueIdList orderedMJPreds;

    generateSortOrders (*(rppForMe->getSortKey()),
                        getEquiJoinPredicates(),
                        leftChildOrder,
                        rightChildOrder,
                        orderedMJPreds,
                        reqOrderCompletelyCovered);

    if (orderedMJPreds.entries() > 0)
    {
      // At least one of the left child merge join columns was compatible
      // with the required sort order columns. This should always be
      // true, because if it wasn't we should have given up when
      // generating the first child context.

      // Add the right child join columns whose left child equivalents
      // were a prefix of the required order columns as a required
      // sort order.
      // Example: Req. order: ABC  Join Columns: ACDE
      // Child req. to generate: Ordered on A
      rg.addSortKey(rightChildOrder);

      // If all of the required order columns were covered, we can
      // add the join columns as an arrangement, so we
      // can use any join columns that were not part of
      // the required order as merge join predicates. But, if there
      // is a required arrangement, defer doing this until we check
      // the join columns against the required arrangement.
      // Example: Req. order: C  Join Columns: ABCD
      // Child req. to generate: Ordered on C, Arranged on ABCD
      if (reqOrderCompletelyCovered AND NOT reqArrangementExists)
        rg.addArrangement(rightJoinColumns);
    }
  }

  // If there is a required arrangement, we must make sure that the
  // join columm arrangement we ask for will be able to satisfy the
  // required arrangement. We can only add a join column arrangement
  // requirement if there was no required order or there was one but
  // the join columns completely covered the required order columns.
  if (reqArrangementExists AND
      ((NOT reqOrderExists) OR
       (reqOrderExists AND reqOrderCompletelyCovered)))
  {
    // To ensure that the join column arrangement requirement is compatible
    // with the required arrangement, we may have to set up a required
    // order for some of the join columns in addition to a required
    // arrangement, or we may have to remove some of the join
    // columns from the required arrangement. Note that for any
    // columns that are removed, they will not be able to be used
    // as merge join predicates.

    ValueIdSet leftJoinColumns;
    ValueIdList rightChildOrder;
    ValueIdSet reqArrangement(*(rppForMe->getArrangedCols()));
    ValueIdSet simpleLeftJoinColumns;
    ValueIdSet simpleReqArrangement;
    ValueId vid,svid;
    ValueId lvid,rvid;
    ValueIdMap map(getEquiJoinExpressions());

    leftJoinColumns.insertList(getEquiJoinExprFromChild0());

    // First, build the simplified versions of the left join predicate
    // columns and the required arrangement columns.
    for (vid = leftJoinColumns.init();
         leftJoinColumns.next(vid);
         leftJoinColumns.advance(vid))
      simpleLeftJoinColumns +=
        vid.getItemExpr()->simplifyOrderExpr()->getValueId();

    for (vid = reqArrangement.init();
         reqArrangement.next(vid);
         reqArrangement.advance(vid))
      simpleReqArrangement +=
        vid.getItemExpr()->simplifyOrderExpr()->getValueId();

    if (simpleReqArrangement.contains(simpleLeftJoinColumns))
      // The left child join columns are a subset of the required
      // arrangement columns. Nothing special to do - just add
      // the right child join cols as the arrangement requirement.
      // Example: Req. arrangement: BCD  Join Columns: CD
      // Child req. to generate: Arranged on CD
      rg.addArrangement(rightJoinColumns);
    else if (simpleLeftJoinColumns.contains(simpleReqArrangement))
    {
      // The required arrangement columns are a subset of the left
      // child join columns.
      // Example: Req. arrangement: BC  Join Columns: ABCD
      // Child req. to generate: Ordered on BC, Arranged on ABCD

      // Determine which left child join columns are also in the
      // the required arrangement.
      simpleLeftJoinColumns.intersectSet(simpleReqArrangement);

      // Set up a required order consisting of the right child
      // join columns whose left child equivalents were in the
      // required arrangement. Note that if there was a required
      // order from the parent, then rightChildOrder will already
      // contain some columns. We may end up adding the same columns
      // again to rightChildOrder. Not to worry, the requirement
      // generator will eliminate the duplicates before processing
      // it in the addSortKey method.
      // Example: Req. order: C Req. arrangement: BC  Join Columns: ABCD
      // Child req. to generate: Ordered by CBC (CB), Arranged on ABCD
      for (lvid = leftJoinColumns.init();
           leftJoinColumns.next(lvid);
           leftJoinColumns.advance(lvid))
      {
        svid = lvid.getItemExpr()->simplifyOrderExpr()->getValueId();
        if (simpleLeftJoinColumns.contains(svid))
        {
          map.mapValueIdDown(lvid,rvid);
          rightChildOrder.insert(rvid);
        }
      }
      rg.addSortKey(rightChildOrder);
      rg.addArrangement(rightJoinColumns);
    }
    else // neither is a subset or equal to the other
         // Example: Req. arrangement: ABC  Join Columns: BD
         // Child req. to generate: Arranged on B
    {
      // Determine which left child join columns are also in the
      // the required arrangement. Note that the resultant set
      // cannot be empty. This is because at least one of the join
      // columns must be compatible with the required arrangement,
      // or we would have given up when generating the first child context.
      simpleLeftJoinColumns.intersectSet(simpleReqArrangement);

      // Remove the right child join columns whose left child
      // equivalents are not in the required arrangement.
      for (lvid = leftJoinColumns.init();
           leftJoinColumns.next(lvid);
           leftJoinColumns.advance(lvid))
      {
        svid = lvid.getItemExpr()->simplifyOrderExpr()->getValueId();
        if (NOT simpleLeftJoinColumns.contains(svid))
        {
          map.mapValueIdDown(lvid,rvid);
          rightJoinColumns -= rvid;
        }
      }
      // Add the remaining right child join columns as the child
      // arrangement requirement.
      rg.addArrangement(rightJoinColumns);
    } // end if neither is a subset of the other
  } // end if a required arrangement

  // If there is no required order and no required arrangement,
  // then we can just add the join columns as a required arrangement.
  if (NOT reqOrderExists AND
      NOT reqArrangementExists)
  {
    rg.addArrangement(rightJoinColumns);
  }

} // MergeJoin::genRightChildArrangementReq()

//<pb>
// -----------------------------------------------------------------------
// MergeJoin::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
MergeJoin::costMethod() const
{
  static THREAD_P CostMethodMergeJoin *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodMergeJoin();
  return m;
} // MergeJoin::costMethod()

//<pb>
Context* MergeJoin::createContextForAChild(Context* myContext,
                                  PlanWorkSpace* pws,
                                  Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  // Merge Join generates at most 2 context pairs. The first is
  // either a non-parallel plan or a matching partitions parallel plan,
  // where we generate the left child context first. The second pair is
  // either a non-parallel plan or a matching partitions parallel plan,
  // where we generate the right child context first.
  //   The reason we try matching partitions plans both ways is to try
  // and avoid having to repartition both tables. If we only try one
  // way and the first table must repartition, then the second table
  // must repartition if it is going to be able to match the hash
  // repartitioning function that the first table synthesized. If we
  // were to try the other way and the first table this time did not
  // need to repartition, then we would only have to range repartition
  // the second table.
  //   The reason we try non-parallel plans both ways is because the
  // first child tried only has to match an arrangement of the merge
  // join columns, but the second child must match an order of the
  // merge join columns. This might force us to sort the second child
  // since it is harder to match an order than an arrangement.
  // The second child might be large and thus more expensive to
  // sort then the first child, so we might want to try it both ways.
  // ---------------------------------------------------------------------

  Context* result = NULL;
  Lng32 planNumber;
  Context* childContext = NULL;
  const ReqdPhysicalProperty* rppForMe =
          myContext->getReqdPhysicalProperty();
  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();
  const ReqdPhysicalProperty* rppForChild = NULL;
  Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
  float childNumPartsAllowedDeviation = 0.0;
  NABoolean numOfESPsForced = FALSE;

  ValueIdSet  equiJoinPreds = getEquiJoinPredicates();

  // If either child of the merge join has a constraint that limits
  // the number of rows it can produce to one or less, than merge
  // join is not a good plan, so just give up now.
  GroupAttributes *child0GA = child(0).getGroupAttr();
  GroupAttributes *child1GA = child(1).getGroupAttr();
  if ((child0GA != NULL AND (child0GA->getMaxNumOfRows() <= 1)) OR
      (child1GA != NULL AND (child1GA->getMaxNumOfRows() <= 1)))
    return NULL;;

  // ---------------------------------------------------------------------
  // Compute the number of child plans to consider.
  // ---------------------------------------------------------------------
  Lng32 childPlansToConsider = 4;
  CollIndex numJoinCols = getEquiJoinExprFromChild0().entries();
  NABoolean mustTryBothChildrenFirst = TRUE;

  // Do we need to generate two different plans where we alternate
  // which child to try first, in order to guarantee that we only
  // sort if absolutely necessary and if we do sort the smallest
  // child? The crux of the matter is that it is easier to satisfy
  // an arrangement than a sort order, and whoever goes first only
  // has to satisfy an arrangement of the join columns, not a sort
  // order of the join columns. But, we can get away with trying
  // just one of the plans if there already is a required sort order
  // whose number of entries is greater than or equal to the number
  // of join columns, because then the join columns are restricted
  // to be in this order and so it does not matter who we try first -
  // they both must satisfy a sort order of the join columns instead
  // of an arrangement. We can also get away with only trying one of
  // the plans if there is only one join column, because then it costs
  // the same to satisfy an arrangement of the one column as a sort
  // order of the one column.
  if (((rppForMe->getSortKey() != NULL) AND
       (rppForMe->getSortKey()->entries() >= numJoinCols)) OR
      (numJoinCols == 1))
    mustTryBothChildrenFirst = FALSE;

  // If we don't need to try two plans (four child plans) for sort
  // purposes and we don't need to try two for parallel purposes,
  // then indicate we will only try one plan (two child plans).
  if (NOT mustTryBothChildrenFirst AND
      ((rppForMe->getCountOfPipelines() == 1) OR
       ((partReqForMe != NULL) AND
        partReqForMe->isRequirementFullySpecified())
      )
     )
    childPlansToConsider = 2;

  // ---------------------------------------------------------------------
  // The creation of the next Context for a child depends upon the
  // the number of child Contexts that have been created in earlier
  // invocations of this method.
  // ---------------------------------------------------------------------
  while ((pws->getCountOfChildContexts() < childPlansToConsider) AND
         (rppForChild == NULL))
  {
    // If we stay in this loop because we didn't generate some
    // child contexts, we need to reset the child plan count when
    // it gets to be as large the arity, because otherwise we
    // would advance the child plan count past the arity.
    if (pws->getPlanChildCount() >= getArity())
      pws->resetPlanChildCount();

    planNumber = pws->getCountOfChildContexts() / 2;
    switch (pws->getCountOfChildContexts())
    {
      case 0:
        childIndex = 0;
        break;
      case 1:
        childIndex = 1;
        break;
      case 2:
        childIndex = 1;
        break;
      case 3:
        childIndex = 0;
        break;
    }

    // -------------------------------------------------------------------
    // Create the 1st Context for left child:
    // -------------------------------------------------------------------
    if ((pws->getCountOfChildContexts() == 0) AND
        currentPlanIsAcceptable(planNumber,rppForMe))
    {
      ValueIdSet  joinColumns;

      RequirementGenerator rg(child(0),rppForMe);

      // Convert equijoin columns from a list to a set.
      joinColumns.insertList(getEquiJoinExprFromChild0());
      // ---------------------------------------------------------------
      // If this is an equijoin, create a partitioning requirement,
      // which uses the expressions from the left child that are
      // referenced in the equijoin predicate, as partitioning keys.
      // ---------------------------------------------------------------
      rg.addPartitioningKey(joinColumns);

      // Optimize the left child with required arrangement = the
      // joining columns. If my Context has a required order or
      // arrangement, then potentially modify the join columns
      // to be something that is compatible with the existing
      // required order or arrangement.
      if (myContext->requiresOrder())
      {
        rg.makeArrangementFeasible(joinColumns);
        if (joinColumns.isEmpty())
          // Impossible to satisfy the parent order or arrangement
          // and the merge join arrangement at the same time.
          // Give up now.
          return NULL;
      }
      rg.addArrangement(joinColumns);

      // --------------------------------------------------------------------
      // If this is a CPU or memory-intensive operator then add a
      // requirement for a maximum number of partitions, unless
      // that requirement would conflict with our parent's requirement.
      //
      // --------------------------------------------------------------------
      if (okToAttemptESPParallelism(myContext,
                                    pws,
                                    childNumPartsRequirement,
                                    childNumPartsAllowedDeviation,
                                    numOfESPsForced))
      {
        if (NOT numOfESPsForced)
          rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                    &childNumPartsAllowedDeviation);
        rg.addNumOfPartitions(childNumPartsRequirement,
                              childNumPartsAllowedDeviation);
      } // end if ok to try parallelism

      // Produce the requirements and make sure they are feasible.
      if (rg.checkFeasibility())
      {
        // Produce the required physical properties.
        rppForChild = rg.produceRequirement();
      }
    } // end if 1st child context

    // -------------------------------------------------------------------
    // Create 1st Context for right child:
    // Any required order matches the order that is produced by the
    // optimal solution for my left child.
    //
    // NOTE: We assume that order of the rows that are produced by
    //       the merge join is only affected by its left child.
    //       So, the input for the right child has no dependency
    //       on the required order that is specified in my myContext.
    // -------------------------------------------------------------------
    else if ((pws->getCountOfChildContexts() == 1) AND
             currentPlanIsAcceptable(planNumber,rppForMe))
    {
      ValueIdList leftChildOrder;
      ValueIdList rightChildOrder;
      ValueIdList orderedMJPreds;
      NABoolean completelyCovered = FALSE;

      // ---------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // ---------------------------------------------------------------
      if (NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
        pws->eraseLatestContextFromWorkSpace();

      childContext = pws->getChildContext(0,0);

      // ---------------------------------------------------------------
      // If the Context that was created for the left child has an
      // optimal solution whose cost is within the specified cost
      // limit, create the corresponding Context for the right child.
      // ---------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty*
              sppForChild = childContext->getPhysicalPropertyForSolution();
        // ---------------------------------------------------------------
        // spp should have been synthesized for child's optimal plan.
        // ---------------------------------------------------------------
        CMPASSERT(sppForChild != NULL);
        PartitioningFunction* childPartFunc =
                                sppForChild->getPartitioningFunction();
        NABoolean rewriteForChild0 = FALSE;
        ValueIdMap map(getEquiJoinExpressions());
        PartitioningFunction* childPartFuncRewritten =
          childPartFunc->copyAndRemap(map,
                                      rewriteForChild0);
        PartitioningRequirement* partReqForChild =
          childPartFuncRewritten->makePartitioningRequirement();

        RequirementGenerator rg (child(1),rppForMe);
        // Remove any parent requirements for the sort key or arrangement,
        // since they do not need to be satisfied by the
        // right child of a join (we only insist that the left child
        // satisfy these requirements).
        if (myContext->requiresOrder())
        {
          rg.removeSortKey();
          rg.removeArrangement();
        }

        // Remove any parent partitioning requirements, since we have
        // already enforced this on the left child.
        rg.removeAllPartitioningRequirements();

        // Now, add in the Join's partitioning requirement for the
        // left child.
        rg.addPartRequirement(partReqForChild);

        // -----------------------------------------------------------
        // Use the sort key of the solution for the left child for
        // creating a required order, which is expressed in terms of
        // the corresponding columns from the right child that appear
        // in the merge join predicates.
        // -----------------------------------------------------------
        generateSortOrders (sppForChild->getSortKey(),
                            equiJoinPreds,
                            leftChildOrder,
                            rightChildOrder,
                            orderedMJPreds,
                            completelyCovered);

        if (orderedMJPreds.entries() > 0)
        {
          // At least one merge join predicate on the sort key
          // columns was found. This will always be true at this
          // point, unless there was a constraint that limited
          // the number of left child rows to at most one. If this
          // occurs, the satisfied method will allow a sort key
          // that does not satisfy the arrangement requirement that
          // merge join generated, and so the sort key may not cover
          // any of the equijoin predicates. We could allow a merge
          // join plan in this case, but if the other child only
          // returns one row then merge join is not a good idea anyway.
          // Note that in this case we should have given up on generating
          // any child contexts in the beginning of this method.

          // Add the ValueIdList returned in "rightChildOrder" as
          // the required sort order for the right child
          rg.addSortKey(rightChildOrder);

          // Produce the requirements and make sure they are feasible.
          if (rg.checkFeasibility())
          {
            // Produce the required physical properties.
            rppForChild = rg.produceRequirement();
            // Remember the merge join predicates in the RelExpr
            setOrderedMJPreds (orderedMJPreds);
            setLeftSortOrder (leftChildOrder);
            setRightSortOrder (rightChildOrder);

            // Need to save any equijoin predicates that were not
            // used for orderedMJPreds. Store them in selectionPred(),
            // if not for an outer join or a semijoin, or in
            // joinPred(), otherwise.
            if (orderedMJPreds.entries() != equiJoinPreds.entries())
            {
              ValueIdSet leftOverEquiJoinPreds = equiJoinPreds;
              ValueIdSet ordMJPreds(orderedMJPreds);
              leftOverEquiJoinPreds -= ordMJPreds;
              // NB: to preserve the separation of equi- and nonequi- join
              // predicates (see Join::separateEquiAndNonEquiJoinPrediates),
              // we must not only save leftOverEquiJoinPreds into but also 
              // remove orderedMJPreds from joinPred or selectionPred.
              // Otherwise, duplicate equijoinpredicates can cause
              // MergeJoin::preCodeGen to GenAssert(!mjp.isEmpty()) because
              // mjp.replaceVEGExpressions refuses to replace VEGExpr that
              // it has already replaced.
              if (isInnerNonSemiJoin())
              {
                selectionPred() -= ordMJPreds;
                selectionPred() += leftOverEquiJoinPreds;
              }
              else
              {
                joinPred() -= ordMJPreds;
                joinPred() += leftOverEquiJoinPreds;
              }
            }

          } // end if merge join and parent requirements are compatible

        } // end if merge join predicates were found

      } // endif previous Context has an optimal solution

    } // end if 2nd child context

    // -------------------------------------------------------------------
    // Create 2nd Context for the right child:
    // The Context for the right child contains
    // required arrangement = joining columns.
    // -------------------------------------------------------------------
    else if ((pws->getCountOfChildContexts() == 2) AND
             currentPlanIsAcceptable(planNumber,rppForMe))
    {
      ValueIdSet  joinColumns;

      RequirementGenerator rg(child(1),rppForMe);
      // Remove any parent requirements for the sort key or arrangement,
      // since they do not need to be satisfied by the
      // right child of a join (we only insist that the left child
      // satisfy these requirements).
      if (myContext->requiresOrder())
      {
        rg.removeSortKey();
        rg.removeArrangement();
      }

      // Generate the arrangement requirement for the right child.
      // This needs a special method because it is very complicated
      // when we process the right child first. This is because we
      // must take the required order and/or arrangement into account
      // without actually enforcing it.
      genRightChildArrangementReq(rppForMe, rg);

      // We must insist that the right child match the parent partitioning
      // requirements, because we are dealing with the right child first.
      // The right child will satisfy the parent somehow (if possible) and
      // the requirement we get from the right child will then be given to
      // the left child, and so the parent requirement will be enforced
      // on the left child in this way.
      // So, we don't remove the parent requirements.
      joinColumns.insertList(getEquiJoinExprFromChild1());
      rg.addPartitioningKey(joinColumns);

      // --------------------------------------------------------------------
      // If this is a CPU or memory-intensive operator then add a
      // requirement for a maximum number of partitions, unless
      // that requirement would conflict with our parent's requirement.
      //
      // --------------------------------------------------------------------
      if (okToAttemptESPParallelism(myContext,
                                    pws,
                                    childNumPartsRequirement,
                                    childNumPartsAllowedDeviation,
                                    numOfESPsForced))
      {
        if (NOT numOfESPsForced)
          rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                    &childNumPartsAllowedDeviation);
        rg.addNumOfPartitions(childNumPartsRequirement,
                              childNumPartsAllowedDeviation);
      } // end if ok to try parallelism

      // Produce the requirements and make sure they are feasible.
      if (rg.checkFeasibility())
      {
        // Produce the required physical properties.
        rppForChild = rg.produceRequirement();
      }

    } // end if 3rd child context

    // -------------------------------------------------------------------
    // Create 2nd Context for the left child:
    // Force the left child to have the same order as the
    // right child.  If there is a required order in my
    // myContext, ensure that it is satisfied.
    // -------------------------------------------------------------------
    else if ((pws->getCountOfChildContexts() == 3) AND
             currentPlanIsAcceptable(planNumber,rppForMe))
    {
      ValueIdList leftChildOrder;
      ValueIdList rightChildOrder;
      ValueIdList orderedMJPreds;
      NABoolean completelyCovered = FALSE;

      // ---------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // ---------------------------------------------------------------
      if (NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
        pws->eraseLatestContextFromWorkSpace();

      childContext = pws->getChildContext(1,1);

      // ---------------------------------------------------------------
      // If the Context that was created for the right child has an
      // optimal solution whose cost is within the specified cost
      // limit, create the corresponding Context for the left child.
      // ---------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty*
              sppForChild = childContext->getPhysicalPropertyForSolution();
        // ---------------------------------------------------------------
        // spp should have been synthesized for child's optimal plan.
        // ---------------------------------------------------------------
        CMPASSERT(sppForChild != NULL);
        PartitioningFunction* childPartFunc =
                                sppForChild->getPartitioningFunction();
        NABoolean rewriteForChild0 = TRUE;
        ValueIdMap map(getEquiJoinExpressions());
        PartitioningFunction* childPartFuncRewritten =
          childPartFunc->copyAndRemap(map,
                                      rewriteForChild0);
        PartitioningRequirement* partReqForChild =
          childPartFuncRewritten->makePartitioningRequirement();

        RequirementGenerator rg (child(0),rppForMe);

        // We have already applied the parent partitioning requirements
        // to the right child, so no need to apply them to the left child.
        rg.removeAllPartitioningRequirements();

        // Now, add in the Join's partitioning requirement for the
        // right child.
        rg.addPartRequirement(partReqForChild);

        // -----------------------------------------------------------
        // Use the sort key of the solution for the right child for
        // creating a required order, which is expressed in terms of
        // the corresponding columns from the left child that appear
        // in the merge join predicates.
        // -----------------------------------------------------------
        generateSortOrders (sppForChild->getSortKey(),
                            equiJoinPreds,
                            leftChildOrder,
                            rightChildOrder,
                            orderedMJPreds,
                            completelyCovered);

        if (orderedMJPreds.entries() > 0)
        {
          // At least one merge join predicate on the sort key
          // columns was found. This will always be true at this
          // point, unless there was a constraint that limited
          // the number of right child rows to at most one. If this
          // occurs, the satisfied method will allow a sort key
          // that does not satisfy the arrangement requirement that
          // merge join generated, and so the sort key may not cover
          // any of the equijoin predicates. We could allow a merge
          // join plan in this case, but if the other child only
          // returns one row then merge join is not a good idea anyway.
          // Note that in this case we should have given up on generating
          // any child contexts in the beginning of this method.

          // If there was a required order or arrangement, then it
          // is possible that the synthesized sort key we got back
          // contained some right child join columns whose left child
          // equivalents are not compatible with the required order
          // or arrangement. If so, we must remove any of these
          // join columns from the required order that we are going
          // to ask for. Note that there must be at least one
          // compatible join column or else we would have given up
          // when generating the first context.
          // Example: Required order: ABC  Join cols: ABD
          // Child requirement generated for right child: ordered by AB
          // Right child synthesized sort key cols: ABD
          // D is not compatible and so must not be used.
          ValueIdList feasibleLeftChildOrder(leftChildOrder);
          if (myContext->requiresOrder())
            rg.makeSortKeyFeasible(feasibleLeftChildOrder);

          if (feasibleLeftChildOrder.isEmpty() &&
              CmpCommon::getDefault(COMP_BOOL_84) == DF_OFF)
            return NULL;

          // If we did drop some join columns, then we must regenerate
          // "orderedMJPreds", "leftChildOrder", "rightChildOrder"
          if (feasibleLeftChildOrder.entries() != leftChildOrder.entries())
          {
            leftChildOrder.clear();
            rightChildOrder.clear();
            orderedMJPreds.clear();

            generateSortOrders (feasibleLeftChildOrder,
                                equiJoinPreds,
                                leftChildOrder,
                                rightChildOrder,
                                orderedMJPreds,
                                completelyCovered);
            CMPASSERT(orderedMJPreds.entries() > 0);
          }

          // Add the ValueIdList returned in "leftChildOrder" as
          // the required sort order for the left child
          rg.addSortKey(leftChildOrder);

          // Produce the requirements and make sure they are feasible.
          if (rg.checkFeasibility())
          {
            // Produce the required physical properties.
            rppForChild = rg.produceRequirement();
            // Remember the merge join predicates in the RelExpr
            setCandidateMJPreds (orderedMJPreds);
            setCandidateLeftSortOrder (leftChildOrder);
            setCandidateRightSortOrder (rightChildOrder);

            // Need to save any equijoin predicates that were not
            // used for orderedMJPreds. Store them in selectionPred(),
            // if not for an outer join or a semijoin, or in
            // joinPred(), otherwise.
            if (orderedMJPreds.entries() != equiJoinPreds.entries())
            {
              ValueIdSet leftOverEquiJoinPreds = equiJoinPreds;
              ValueIdSet ordMJPreds(orderedMJPreds);
              leftOverEquiJoinPreds -= ordMJPreds;
              // NB: to preserve the separation of equi- and nonequi- join
              // predicates (see Join::separateEquiAndNonEquiJoinPrediates),
              // we must not only save leftOverEquiJoinPreds into but also 
              // remove orderedMJPreds from joinPred or selectionPred.
              // Otherwise, duplicate equijoin predicates can cause
              // MergeJoin::preCodeGen to GenAssert(!mjp.isEmpty()) because
              // mjp.replaceVEGExpressions refuses to replace VEGExpr that
              // it has already replaced.
              if (isInnerNonSemiJoin())
              {
                selectionPred() -= ordMJPreds;
                selectionPred() += leftOverEquiJoinPreds;
              }
              else
              {
                joinPred() -= ordMJPreds;
                joinPred() += leftOverEquiJoinPreds;
              }
            }

          } // end if merge join and parent requirements are compatible

        } // end if merge join predicates were found

      } // endif previous Context has an optimal solution

    } // end if 4th child context

    // -------------------------------------------------------------------
    // Create a Context using the partitioning requirement that was
    // generated for the current plan.
    // -------------------------------------------------------------------
    if (rppForChild != NULL)
    {

      // ---------------------------------------------------------------
      // Compute the cost limit to be applied to the child.
      // ---------------------------------------------------------------
      CostLimit* costLimit = computeCostLimit(myContext,pws);

      // ---------------------------------------------------------------
      // Get a Context for optimizing the child.
      // Search for an existing Context in the CascadesGroup to which
      // the child belongs that requires the same properties as those
      // in rppForChild. Reuse it, if found. Otherwise, create a new
      // Context that contains rppForChild as the required physical
      // properties.
      // ----------------------------------------------------------------
      result = shareContext(childIndex, rppForChild,
                            myContext->getInputPhysicalProperty(),
                            costLimit,
                            myContext, myContext->getInputLogProp());

      if ( NOT (pws->isLatestContextWithinCostLimit() OR
                result->hasSolution() )
         )
       result = NULL;

      if(CURRSTMT_OPTDEFAULTS->isMergeJoinControlEnabled() AND result)
      {
        result->ignoredRules() = *(GlobalRuleSet->applicableRules());
        result->ignoredRules() -= SortEnforcerRuleNumber;
      }
    }
    else
      result = NULL;

    // -------------------------------------------------------------------
    // Remember the cases for which a Context could not be generated,
    // or store the context that was generated.
    // -------------------------------------------------------------------
    pws->storeChildContext(childIndex, planNumber, result);
    pws->incPlanChildCount();

    if ( CURRSTMT_OPTDEFAULTS->optimizerPruning() AND
         ( pws->getPlanChildCount() == getArity() ) AND
         CURRSTMT_OPTDEFAULTS->OPHexitMJcrContChiLoop()
       )
    {
      pws->resetAllChildrenContextsConsidered();
      break;
    }
  }  // end while loop

  if ( pws->getCountOfChildContexts() == childPlansToConsider )
    pws->setAllChildrenContextsConsidered();

  return result;

} // MergeJoin::createContextForAChild()

//<pb>
NABoolean MergeJoin::findOptimalSolution(Context* myContext,
                                         PlanWorkSpace* pws)
{
  NABoolean hasOptSol;
  // Plan # is only an output param, initialize it to an impossible value.
  Lng32 planNumber = -1;

  hasOptSol = pws->findOptimalSolution(planNumber);

  // Reset data members to reflect that of plan 1 (not plan 0) when picked.
  if(planNumber == 1)
  {
    setOrderedMJPreds(getCandidateMJPreds());
    setLeftSortOrder(getCandidateLeftSortOrder());
    setRightSortOrder(getCandidateRightSortOrder());
  }
  return hasOptSol;

} // MergeJoin::findOptimalSolution()

NABoolean MergeJoin::currentPlanIsAcceptable(Lng32 planNo,
            const ReqdPhysicalProperty* const rppForMe) const
{
  // This is probably not the best place to check it. It should work
  // temporarily. We are trying to force potentially hanging plan
  // to fail. See solution 10-051219-3501.
  if ( deadLockIsPossible_)
	  return FALSE;

  // ---------------------------------------------------------------------
  // Check whether the user wants to enforce a particular plan type.
  // ---------------------------------------------------------------------

  // If nothing is being forced, return TRUE now.
  if (rppForMe->getMustMatch() == NULL)
    return TRUE;

  // Check for the correct forced plan type.

  JoinForceWildCard::forcedPlanEnum forcePlanToken =
       getParallelJoinPlanToEnforce(rppForMe);

  switch (forcePlanToken)
  {
    case JoinForceWildCard::FORCED_PLAN0:
      if (planNo != 0)
        return FALSE;
      break;
    case JoinForceWildCard::FORCED_PLAN1:
      if (planNo != 1)
        return FALSE;
      break;
    case JoinForceWildCard::FORCED_TYPE1:
      // All Merge Joins are Type 1 - break out so we'll return TRUE
      break;
    case JoinForceWildCard::ANY_PLAN:
      // Any plan satisfies this - break out so we'll return TRUE
      break;
    default:
      return FALSE; // must be some option that Merge Join doesn't support
  }

  // If we get here, the plan must have passed all the checks.
  return TRUE;

} // MergeJoin::currentPlanIsAcceptable()

//<pb>
//==============================================================================
//  Synthesize physical properties for merge join operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used for
//                 synthesizing partitioning functions.
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
MergeJoin::synthPhysicalProperty(const Context* myContext,
                                 const Lng32     planNumber,
                                 PlanWorkSpace  *pws)
{

  const PhysicalProperty* const sppOfLeftChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(0);

  const PhysicalProperty* const sppOfRightChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(1);


  // This is to prevent possible deadlock for parallel merge join.
  // If both children got repartitioned and didn't use Sort then
  // force this plan to fail. See solution 10-051219-3501
  if ( (CmpCommon::getDefault(MERGE_JOIN_WITH_POSSIBLE_DEADLOCK) == DF_OFF) AND
       (sppOfLeftChild->getSortOrderType() == ESP_NO_SORT_SOT) AND
	   (sppOfLeftChild->getDataSourceEnum() == SOURCE_ESP_DEPENDENT) AND
       (sppOfRightChild->getSortOrderType() == ESP_NO_SORT_SOT) AND
	   (sppOfRightChild->getDataSourceEnum() == SOURCE_ESP_DEPENDENT)
	 )
  {
	  deadLockIsPossible_ = TRUE;
  }
  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(myContext,
                                                             planNumber,
                                                             pws);

  // ----------------------------------------------------------------
  // Synthesize the partitioning function from the first child of the
  // winning plan.  Merge Joins have two potential plans--see member
  // function MergeJoin::createContextForAChild().  In plan 0, the
  // left child is first; in plan 1, the right child is first.
  // ----------------------------------------------------------------
  CMPASSERT(planNumber == 0 || planNumber == 1);
  PartitioningFunction* myPartFunc;
  const SearchKey* myPartSearchKey;
  if (planNumber == 0)
    {
      myPartFunc = sppOfLeftChild->getPartitioningFunction();
      myPartSearchKey = sppOfLeftChild->getPartSearchKey();
    }
  else
    {
      myPartFunc = sppOfRightChild->getPartitioningFunction();
      myPartSearchKey = sppOfRightChild->getPartSearchKey();
    }

  // ---------------------------------------------------------------------
  // Parallel merge join plans are set up such that
  // they maintain the partitioning of the left child table.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppForMe =
    new(CmpCommon::statementHeap()) PhysicalProperty(
         sppOfLeftChild->getSortKey(),
         sppOfLeftChild->getSortOrderType(),
         sppOfLeftChild->getDp2SortOrderPartFunc(),
         myPartFunc,
         sppOfLeftChild->getPlanExecutionLocation(),//||opt synth from both kids
         combineDataSources(
              sppOfLeftChild->getDataSourceEnum(),
              sppOfRightChild->getDataSourceEnum()),
         sppOfLeftChild->getIndexDesc(),
         myPartSearchKey);

  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  delete sppTemp;
  return sppForMe;

} // MergeJoin::synthPhysicalProperty()

//<pb>

// -----------------------------------------------------------------------
// member functions for class HashJoin
// -----------------------------------------------------------------------

// ---------------------------------------------------------------------
// Performs mapping on the partitioning function, from the
// hash join to the designated child.
// ---------------------------------------------------------------------
PartitioningFunction* HashJoin::mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0)
{
  ValueIdMap map(getEquiJoinExpressions());
  PartitioningFunction* newPartFunc =
      partFunc->copyAndRemap(map,rewriteForChild0);

  SkewedDataPartitioningFunction* oldSKpf = NULL;
  SkewedDataPartitioningFunction* newSKpf = NULL;

  if ( rewriteForChild0 == FALSE /* map for child 1 */ AND
       (oldSKpf=(SkewedDataPartitioningFunction*)
                  (partFunc->castToSkewedDataPartitioningFunction())) AND
       (newSKpf=(SkewedDataPartitioningFunction*)
                  (newPartFunc->castToSkewedDataPartitioningFunction()))
     )
  {
     if ( oldSKpf->getSkewProperty().isUniformDistributed() ) {
       skewProperty newSK(oldSKpf->getSkewProperty());
       newSK.setIndicator(skewProperty::BROADCAST);
       newSKpf->setSkewProperty(newSK);
     }
  } else {
       // map for child 0. Do nothing
  }



  return newPartFunc;
} // end HashJoin::mapPartitioningFunction()


NABoolean HashJoin::isBigMemoryOperator(const PlanWorkSpace* pws,
                                        const Lng32 planNumber)
{
  double dummy;
  return isBigMemoryOperatorSetRatio( pws->getContext(), planNumber, dummy);
}

NABoolean HashJoin::isBigMemoryOperatorSetRatio(const Context* context,
                                                const Lng32 planNumber,
						double & ratio)
{
  double memoryLimitPerCPU = CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU();

  // ---------------------------------------------------------------------
  // Not given any memory constraints, the HJ would like to have enough
  // memory to hold the whole inner table and auxiliary hash structures.
  // Each row is stored with its hash key and a chain pointer (8 bytes in
  // total).
  // ---------------------------------------------------------------------
  const ReqdPhysicalProperty* rppForMe = context->getReqdPhysicalProperty();
  // Start off assuming that the operator will use all available CPUs.
  Lng32 cpuCount = rppForMe->getCountOfAvailableCPUs();
  PartitioningRequirement* partReq = rppForMe->getPartitioningRequirement();

  // This check to ensure that a plan exists before calling getPhysProp <oa>
  PhysicalProperty* spp = NULL;
  if ( context->getPlan())
    spp = context->getPlan()->getPhysicalProperty();

  Lng32 numOfStreams;

  // If the physical properties are available, then this means we
  // are on the way back up the tree. Get the actual level of
  // parallelism from the spp to determine if the number of cpus we
  // are using are less than the maximum number available.
  if (spp != NULL)
  {
    PartitioningFunction* partFunc = spp->getPartitioningFunction();
    numOfStreams = partFunc->getCountOfPartitions();
    if (numOfStreams < cpuCount)
      cpuCount = numOfStreams;
  }
  else
  if ((partReq != NULL) AND
      (partReq->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS))
  {
    // If there is a partitioning requirement, then this may limit
    // the number of CPUs that can be used.
    numOfStreams = partReq->getCountOfPartitions();
    if (numOfStreams < cpuCount)
      cpuCount = numOfStreams;
  }

  EstLogPropSharedPtr inLogProp = context->getInputLogProp();
  const double probeCount =
                      MAXOF(1.,inLogProp->getResultCardinality().value());
  const double innerRowCount =
        child(1).outputLogProp(inLogProp)->getResultCardinality().value();

  double rowsPerCpu;
  if (planNumber != 0)  // Type-1 Hash Join?
  {
    rowsPerCpu = MAXOF(1.,(innerRowCount / cpuCount));
  }
  else // Type-2 Hash Join
  {
    // Each ESP must build a hash table of all the rows.
    rowsPerCpu = MAXOF(1.,innerRowCount);
  }

  const double rowsPerCpuPerProbe = MAXOF(1.,(rowsPerCpu / probeCount));

  const Lng32 innerRowLength =
       child(1).getGroupAttr()->getCharacteristicOutputs().getRowLength();
  const Lng32 extInnerRowLength = innerRowLength + 8;
  const double fileSizePerCpu =
    ((rowsPerCpuPerProbe * extInnerRowLength) / 1024.);

  if (spp != NULL &&
      CmpCommon::getDefault(COMP_BOOL_51) == DF_ON
     )
  {
      CurrentFragmentBigMemoryProperty * bigMemoryProperty =
                 new (CmpCommon::statementHeap())
                      CurrentFragmentBigMemoryProperty();

      spp->setBigMemoryEstimationProperty(bigMemoryProperty);

      bigMemoryProperty->setCurrentFileSize(fileSizePerCpu);
      bigMemoryProperty->setOperatorType(getOperatorType());

      for (Int32 i=0; i<=getArity(); i++)
      {
         const PhysicalProperty *childSpp =
           context->getPhysicalPropertyOfSolutionForChild(i);

        if (childSpp != NULL)
        {
           CurrentFragmentBigMemoryProperty * memProp =
            (CurrentFragmentBigMemoryProperty *)
             ((PhysicalProperty *)childSpp)->getBigMemoryEstimationProperty();

          if (memProp != NULL)
          {
             double childCumulativeMemSize = memProp->getCumulativeFileSize();
             bigMemoryProperty->incrementCumulativeMemSize(childCumulativeMemSize);
             memoryLimitPerCPU -= childCumulativeMemSize;
          }
        }
      }
  }

  if (memoryLimitPerCPU < 1)
     memoryLimitPerCPU =1;

  ratio = fileSizePerCpu/memoryLimitPerCPU;

  return (fileSizePerCpu >= memoryLimitPerCPU);
}

// <pb>

NABoolean
HashJoin::isSkewBusterFeasible(SkewedValueList** skList, Lng32 countOfPipelines,
                               ValueId& vidOfEquiPredWithSkew)
{
   CMPASSERT(skList != NULL);

   double threshold =
       (ActiveSchemaDB()->getDefaults().getAsDouble(SKEW_SENSITIVITY_THRESHOLD))                             / countOfPipelines;

   // Disable skew buster if the threshold is less than 0. A value of -1 is set
   if ( threshold < 0 )
      return FALSE;

   // skew buster is not allowed for Full Outer Join, since it involves
   // broadcasting the inner rows.

   if (isFullOuterJoin())
     return FALSE;

   ValueIdSet joinPreds = getEquiJoinPredicates();

   if ( joinPreds.entries() > 1 ) {

     double mc_threshold =
         (ActiveSchemaDB()->getDefaults().getAsDouble(MC_SKEW_SENSITIVITY_THRESHOLD))
        / countOfPipelines ;
                         
     if ( mc_threshold < 0 )
        return FALSE;

     if ( !multiColumnjoinPredOKforSB(joinPreds) )
        return FALSE;

       vidOfEquiPredWithSkew = NULL_VALUE_ID;

       // First try to detect the true MC skews directly.   
       NABoolean ok =  CmpCommon::getDefault(HJ_NEW_MCSB_PLAN) == DF_ON &&
                       childNodeContainMultiColumnSkew(0, joinPreds, mc_threshold, 
                                                       countOfPipelines, skList);

       if ( ok )
          return TRUE;
       else
          // If fail, we go the old way of guessing the MC skews from the SC 
          // skew exist at the participating columns of the multi-column join 
          // predicate.   
          return childNodeContainMultiColumnSkew(0, joinPreds, mc_threshold, 
                                             threshold, countOfPipelines,
                                             skList, vidOfEquiPredWithSkew);

   } 

   // single column SB
   return ( singleColumnjoinPredOKforSB(joinPreds) &&
            childNodeContainSkew(0, joinPreds, threshold, skList)
          );
}

void HashJoin::addNullToSkewedList(SkewedValueList** skList)
{

  DCMPASSERT(skList != NULL);

  //if not a NOTIN subquery return
  if(!getIsNotInSubqTransform())
  {
    return;
  }

  ValueIdList child0ColList = getEquiJoinExprFromChild0();

  // get the first column id and use it as skew list identifier 
  // same as in skew buster
  ValueId colVid = child0ColList[0];

  // Create a new skew value list for not in subq if not already created 
  // by skewbuster routines
  if (*skList == NULL)
  {
    *skList = new (STMTHEAP) SkewedValueList(colVid.getType().newCopy(STMTHEAP), STMTHEAP, 1);
  }

  if (!(*skList)->getIsNullInList())
  {
    // add null value to the list
    EncodedValue encNullValue;
    encNullValue.setValueToNull();
    (*skList)->insertInOrder(encNullValue);
    (*skList)->setIsNullInList(TRUE);
  }

} // HashJoin::addNullToSkewedList



// -----------------------------------------------------------------------
// HashJoin::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
HashJoin::costMethod() const
{
  static THREAD_P CostMethodHashJoin *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodHashJoin;
  return m;
} // HashJoin::costMethod()

//<pb>
// -----------------------------------------------------------------------
// HashJoin::createContextForAChild() can create up to 3 sets of contexts for
// its children.
// -----------------------------------------------------------------------
Context* HashJoin::createContextForAChild(Context* myContext,
                                 PlanWorkSpace* pws,
                                 Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  // Hash Join generates at most 3 context pairs. The first pair is
  // either a non-parallel plan or Type 2 parallel plan. The second
  // pair is a matching partitions parallel plan,
  // where we generate the left child context first. The third pair is
  // a parallel matching partitions plan where we generate the right
  // child context first.
  // The reason we try matching partitions plans both ways is to try
  // and avoid having to repartition both tables. If we only try one
  // way and the first table must repartition, then the second table
  // must be repartitioned if it is going to be able to match the hash
  // repartitioning function that the first table synthesized. If we
  // were to try the other way and the first table this time did not
  // need to repartition, then we would only have to range repartition
  // the second table.
  // ---------------------------------------------------------------------

  Context* result = NULL;
  Lng32 planNumber = 0;
  Context* childContext = NULL;
  const ReqdPhysicalProperty* rppForMe =
                                     myContext->getReqdPhysicalProperty();
  const PartitioningRequirement* partReqForMe =
          rppForMe->getPartitioningRequirement();
  ReqdPhysicalProperty* rppForChild = NULL;
  Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
  float childNumPartsAllowedDeviation = 0.0;
  NABoolean numOfESPsForced = FALSE;

  // ---------------------------------------------------------------------
  // Compute the number of child plans to consider.
  // ---------------------------------------------------------------------
  Lng32 childPlansToConsider = 6;

  Lng32 baseTableThreshold = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_19);

  // part of fix to genesis case 10-061222-1068, soln 10-061222-1347.
  // our goal here is to avoid arithmetic overflow in the computation
  // of innerChildSize that can result in a replicate-broadcast of so much
  // data to cause hashjoin to spill resulting in very poor performance.
  const CostScalar innerChildSize =
    child(1).getGroupAttr()->getResultCardinalityForEmptyInput()
    * child(1).getGroupAttr()->getRecordLength();

  // parallelHeuristic3(if TRUE) will prevent two extra pairs of contexts
  // by checking if parallelism was worth trying after the first pair.
  // If not, it will set childPlansToConsider to 2 instead of 6. By doing
  // this we won't create, at least, one extra parallel context for each
  // child. It will change pallelismIsOK for the workspace to FALSE when
  // okToAttemptESPParallelism() returns FALSE and the heuristics3 is ON.

  // Only repartitioning plans are allowed for Full Outer Join. That is
  // because, broadcast is not compatible with it. Hence, no need to cut
  // down on the number of plans to consider. Keep childPlansToConsider at 6.

  if ((NOT isFullOuterJoin())
      AND (
	   (rppForMe->getCountOfPipelines() == 1) OR
       getEquiJoinPredicates().isEmpty() OR
       ( (partReqForMe != NULL) AND
         ( partReqForMe->isRequirementExactlyOne() OR
           partReqForMe->isRequirementReplicateNoBroadcast()
         )
       ) OR
       ( CURRSTMT_OPTDEFAULTS->parallelHeuristic3() AND
         (pws->getParallelismIsOK() == FALSE)
       ) OR
       ( isOrderedCrossProduct() AND
	  myContext->requiresOrder()
       ) OR
       ( // New heuristics to limit repartitioning of small right child.
         // If right child has very few rows, for example, dimension
         // table with local predicate resulting in one row only, then
         // repartitioning of this child will cause few (or one) join ESP
         // active. So we could consider only type2 join in this case

         // Now limited to star filter join but could be generalized
         // by setting COMP_BOOL_68 to ON
         ( (getSource() == Join::STAR_FILTER_JOIN) OR
           (CmpCommon::getDefault(COMP_BOOL_68) == DF_ON)
         ) AND
         // To avoid complex subqueries due to cardinality uncertainties
         ( child(1).getGroupAttr()->getNumBaseTables() <=  baseTableThreshold) AND
         // the child is small - defined by COMP_INT_7
         ( innerChildSize < CostScalar(ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_7))) AND
         ( rppForMe->getMustMatch() == NULL )
       )
	   ) // AND
      ) // (NOT isFullOuterJoin())
  {

    // we want don't want to repartition the inner child/table
    // rather we want to broadcast it.Do this only if:
    // the child/table size is < COMP_INT_70
    // or
    // inner child/table size > COMP_INT_70 and the join
    // column(s) from the left child are skewed.

    // The decision to broadcast and not repartition is based on
    // the size of the inner table (i.e. the table to be broadcast)
    // We can define it visually as the range below
    //
    //             comp_int_70            comp_int_7
    // ------------------|---------------------|--------------------
    //      Range 1               Range 2              Range 3
    //
    // * If inner table size is in range 1 i.e. < comp_int_70 then repartitioning
    //   plan is not tried, the table is unconditionally broadcast
    // * If inner table size is in range 2 i.e. > comp_int_70 and < comp_int_7
    //   then repartition plan is not tried i.e. only broadcast plan is tried
    //   if the join column from the left child is skewed.
    // * If inner table size is in range 3 i.e. > comp_int_7 then repartitioning
    //   plan is tried.

    if(innerChildSize <= ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_70))
    {
      // size of inner child is < conditional broadcast threshold (COMP_INT_70)
      // therefore disallow the repartitioning plan
      // By disallowing the repartitioning plan we force only the broadcast
      // plan to be considered
      childPlansToConsider = 2;
    }
    else
    {
      // size of inner child is > conditional broadcast threshold (COMP_INT_70)
      // there disallow the repartitioning plan only if the join is
      // on a skewed set of columns
      // By disallowing the repartitioning plan we force only the broadcast
      // plan to be considered
      CostScalar skewFactor(1);
      EncodedValue mostFreqVal(UNINIT_ENCODEDVALUE);
      ValueIdSet joinPreds = getEquiJoinPredicates();

      // iterate over all the join predicates between the left and the right
      // In each iteration:
      // * The skew factor is returned for the join column from the left child
      // * The skew factor is combined with the skew factor from other predicates.
      //   We keep a running multiplication of skew factors from the different
      //   predicates to combine the skew factors for all the join predicates
      //   being applied at this join. This is based on the assumption of independence
      //   between columns if there is more than one join predicate. Multicolumn stats
      //   could perhaps be used to get better skew estimate in case we are joining
      //   on more than one column, but this is simpler and should be good enough for
      //   for a simple heuristic like this.
      for(ValueId vid = joinPreds.init(); joinPreds.next(vid); joinPreds.advance(vid))
      {
        // get the skewFactor and combine it
        skewFactor*=child(0).getGroupAttr()->getSkewnessFactor(vid,mostFreqVal);
      }

      // If the join is on a skewed set of columns from the left child.
      //    Skew threshold is defined by COMP_FLOAT_3
      //
      // Don't try repartitioning plan
      if( (skewFactor > (ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_3)) ||
	  // not in subquery --> set childPlansToConsider = 2 --> only plan0 is considered  
	  getIsNotInSubqTransform()) 
        childPlansToConsider = 2;
    }
  }
      else if ((NOT isFullOuterJoin()) AND
	       (partReqForMe != NULL) AND
           partReqForMe->isRequirementFullySpecified())
  {
    childPlansToConsider = 4;
  }


  // Force serial plans for full outer join.
  if (isFullOuterJoin() AND
      CmpCommon::getDefault(COMP_BOOL_197) == DF_ON )
    childPlansToConsider = 2;



  NABoolean considerMixedHashJoin = FALSE;
  NABoolean considerHashJoinPlan2 = TRUE;
  NABoolean broadcastOneRow  = FALSE;
  NABoolean notInLeftChildOptimization = FALSE;

  // ---------------------------------------------------------------------
  // The creation of the next Context for a child depends upon the
  // the number of child Contexts that have been created in earlier
  // invocations of this method.
  // ---------------------------------------------------------------------
  while ( (pws->getCountOfChildContexts() < childPlansToConsider)
          AND (rppForChild == NULL) )
  {

    // If we stay in this loop because we didn't generate some
    // child contexts, we need to reset the child plan count when
    // it gets to be as large the arity, because otherwise we
    // would advance the child plan count past the arity.
    if (pws->getPlanChildCount() >= getArity())
      pws->resetPlanChildCount();

    planNumber = pws->getCountOfChildContexts() / 2;
    switch (pws->getCountOfChildContexts())
    {
      case 0:
        childIndex = 0;
        break;
      case 1:
        childIndex = 1;
        break;
      case 2:
        childIndex = 0;
        break;
      case 3:
        childIndex = 1;
        break;
      case 4:
        childIndex = 1;
        break;
      case 5:
        childIndex = 0;
        break;
    }

    SkewedValueList* skList = NULL;
    considerMixedHashJoin = FALSE;
    considerHashJoinPlan2 = TRUE;
    broadcastOneRow  = FALSE;
    notInLeftChildOptimization = FALSE;
    ValueId vidOfEquiPredWithSkew = NULL_VALUE_ID;
        
    // Here, we consult our histogram data to identify potential skew values
    // for the left side, when ESP parallel plans (plan 1 and 2) are
    // sought for. In the 1st phase of the Mixed Hybrid HashJoin Project,
    // only plan 1 is augmented to deal with skewed values. But we
    // turn on 'considerMixedHashJoin' anyway here so that we can use it
    // to turn off plan 2 generation when the skewness is found.

    if (planNumber == 1 OR planNumber == 2)
    {

      if( isSkewBusterFeasible(&skList, MAXOF(1, rppForMe->getCountOfPipelines()),
                               vidOfEquiPredWithSkew) == TRUE )
      {
        considerMixedHashJoin = TRUE;

        if ( CmpCommon::getDefault(COMP_BOOL_101) == DF_OFF  )
          considerHashJoinPlan2 = FALSE;
      }

      // When adding null value the next time isSkewBusterFeasible will return true even
      // if there are no skew values. Note that this has to do with an executor optimization
      // for anti-semijoins used for NOT IN. If the subquery has at least one NULL value,
      // then the entire NOT IN predicate returns NULL. Handling this requires special
      // logic in the executor. When we have a parallel TYPE1 anti-semijoin, then we
      // use SkewBuster to broadcast all inner NULL values to all the joins, to be able
      // to handle this case correctly.


      if (getIsNotInSubqTransform())
      {
        //add null to the skewed list
        addNullToSkewedList(&skList);
        // is broadcast of one row required 
        broadcastOneRow = getRequireOneBroadcast();

        considerMixedHashJoin = TRUE;
        // if this is a notin subq then we don't want to consider plan2
        considerHashJoinPlan2 = FALSE;

        if ( CmpCommon::getDefault(NOT_IN_SKEW_BUSTER_OPTIMIZATION) == DF_ON)
        {
          if (skList->hasOnlyNonSkewedNull() &&
              skList->getNAType()->getTypeQualifier() != NA_CHARACTER_TYPE)  
              // no real skewed values. null is the only item in the list and is not skewed
              // in this case we don't really need to uniform-distribute rows on the right side
              // all we need is the broadcast null (and one row if applicable) on the right side
              // NOTE: This will cause a regular "hash2" function to be used on the left side
              // and a "h2-br" function on the right. We have to guarantee that those produce
              // the same hash value for every non-NULL column value. This is NOT true for
              // character columns, since those get converted to VARCHAR in
              // SkewedDataPartitioningFunction::createPartitioningExpression(), which does
              // not happen in TableHashPartitioningFunction::createPartitioningExpression()
          {
            considerMixedHashJoin = FALSE;
            notInLeftChildOptimization = TRUE;
          }
        }
      }
    }



    if ((pws->getCountOfChildContexts() == 0) AND
        currentPlanIsAcceptable(planNumber,rppForMe))
    {
      if ( NOT CURRSTMT_OPTDEFAULTS->optimizerPruning() OR
           pws->isLatestContextWithinCostLimit() OR
           NOT CURRSTMT_OPTDEFAULTS->OPHpruneWhenCLExceeded() )
      {
      RequirementGenerator rg(child(0), rppForMe);

      if (isOrderedCrossProduct())
      {
	ValueIdList reqdOrder1;
	ValueIdSet reqdArr1;
	// If there is a required sort order and/or arrangement, then
	// split off any part of the requirement that is for the
	// right child and only pass on the portion of the requirement
	// that is for the left child. Pass back the requirements for
	// the right child in case they are needed.
	splitSortReqsForLeftChild(rppForMe, rg, reqdOrder1, reqdArr1);
      }

      // If we want to try ESP parallelism, go for it, only if
      // its NOT isFullOuterJoin. Note we may still
      // not get it if the parent's requirement does not allow it.
      // If we don't go for ESP parallelism, then we will specify either
      // the parent's required number of partitions, or we will specify
      // that we don't care how many partitions we get - i.e.
      // ANY_NUMBER_OF_PARTITIONS.
      if (isFullOuterJoin())
	{
	  // If it's full outer join, then force the requirement
	  // to be one partition - (serial).
	  rg.addNumOfPartitions(1);
	}
      else if (okToAttemptESPParallelism(myContext,
                                    pws,
                                    childNumPartsRequirement,
                                    childNumPartsAllowedDeviation,
                                    numOfESPsForced))
      {
        if (NOT numOfESPsForced)
          rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                    &childNumPartsAllowedDeviation);
        rg.addNumOfPartitions(childNumPartsRequirement,
                              childNumPartsAllowedDeviation);
      } // end if ok to try parallelism
      else
      {
	  if (CURRSTMT_OPTDEFAULTS->parallelHeuristic3())
          pws->setParallelismIsOK(FALSE);
        }


      // Produce the requirements and make sure they are feasible.
      if (rg.checkFeasibility())
      {
        // Produce the required physical properties.
        rppForChild = rg.produceRequirement();
      }
      } // NOT CURRSTMT_OPTDEFAULTS->optimizerPruning() OR ..
    } // endif (pws->getCountOfChildContexts() == 0)
    else if ((pws->getCountOfChildContexts() == 1) AND
        currentPlanIsAcceptable(planNumber,rppForMe))
    {
      // -----------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // -----------------------------------------------------------------
      if (pws->getLatestChildContext() AND
          NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
        pws->eraseLatestContextFromWorkSpace();

      childContext = pws->getChildContext(0,0);

      // -----------------------------------------------------------------
      // Make sure a plan has been produced by the latest context.
      // -----------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        NABoolean considerReplicateBroadcast = TRUE; // unless revoked below
        const PhysicalProperty*
              sppForChild = childContext->getPhysicalPropertyForSolution();
        // ---------------------------------------------------------------
        // spp should have been synthesized for child's optimal plan.
        // ---------------------------------------------------------------
        CMPASSERT(sppForChild != NULL);
        PartitioningFunction* childPartFunc =
                                sppForChild->getPartitioningFunction();
        PartitioningRequirement* partReqForChild;

        // If the partitioning function of the left child was only
        // one partition, then pass a requirement for that to the right,
        // since no parallelism is possible.
        // BEGIN TEMPORARY CODE FIX COMMENT
        // If there was a parent requirement and it was replicate no
        // broadcast, then we must require replicate no broadcast from
        // the right child as well, because the executor currently
        // cannot handle broadcast replication under a nested join in
        // certain cases. See Materialize::createContextForAChild for details.
        // END TEMPORARY CODE FIX COMMENT
        // Otherwise, pass a broadcast replication req. to the right child.
        if (childPartFunc->isASinglePartitionPartitioningFunction())
          partReqForChild = childPartFunc->makePartitioningRequirement();
        else if ((partReqForMe != NULL) AND  // TEMP CODE FIX
                 partReqForMe->isRequirementReplicateNoBroadcast()) // TEMP
          partReqForChild = (PartitioningRequirement *)partReqForMe; // TEMP
        else { // replicate broadcast inner
          CostScalar innerMaxSize = 
            child(1).getGroupAttr()->getResultMaxCardinalityForEmptyInput()
            * child(1).getGroupAttr()->getRecordLength();
          CostScalar outerSize = 
            child(0).getGroupAttr()->getResultCardinalityForEmptyInput()
            * child(0).getGroupAttr()->getRecordLength();
          // disallow replicate broadcast only if:
          // 1) we are an optional zigzag join, and
          // 2) inner can be dangerously large
          if (isJoinForZigZag() && innerMaxSize >= 
              outerSize * childPartFunc->getCountOfPartitions())
            {
              considerReplicateBroadcast = FALSE;
            }
          if (CmpCommon::getDefault(COMP_BOOL_82) == DF_ON) {
          // Use the node map of the left childs partitioning function.
          partReqForChild = new (CmpCommon::statementHeap() )
            RequireReplicateViaBroadcast(childPartFunc, TRUE);
        }
          else {
            partReqForChild = new (CmpCommon::statementHeap() )
              RequireReplicateViaBroadcast
              (childPartFunc->getCountOfPartitions());

            // Use the node map of the left childs partitioning function.
            //
            NodeMap *myNodeMap =
              childPartFunc->getNodeMap()->copy(CmpCommon::statementHeap());

            partReqForChild->
              castToFullySpecifiedPartitioningRequirement()->
              getPartitioningFunction()->replaceNodeMap(myNodeMap);
          }
        }
        RequirementGenerator rg (child(1),rppForMe);

        if (myContext->requiresOrder())
	{
	    // the ordering of an ordered hash join is provided by the left child
	    // for cross products we need the next two methods to prevent a sort key from
	    // being added due to startRequirements,
	    rg.removeSortKey();
	    rg.removeArrangement();
	    if (isOrderedCrossProduct())
	    {
	      ValueIdList reqdOrder0;
	      ValueIdSet reqdArr0;
	      // If there is a required sort order and/or arrangement, then
	      // split off any part of the requirement that is for the
	      // left child and only pass on the portion of the requirement
	      // that is for the right child. Pass back the requirements for
	      // the left child in case they are needed.
	      splitSortReqsForRightChild(rppForMe, rg, reqdOrder0, reqdArr0);
	    }
	  }

        // Remove any parent partitioning requirements, since we
        // have already enforced this on the left child.
        rg.removeAllPartitioningRequirements();

        // Now, add in the Join's partitioning requirement for the
        // right child.
        rg.addPartRequirement(partReqForChild);

        // Produce the requirements and make sure they are feasible.
        if (rg.checkFeasibility() 
            && considerReplicateBroadcast)
        {
          // Produce the required physical properties.
          rppForChild = rg.produceRequirement();
        }

      } // end if child0 had an optimal solution
    } // end if 2nd child context

    // -----------------------------------------------------------------
    // The next plan, (child0, plan1) and (child1, plan1), attempts to
    // create matching partitions.  (child1, plan1) constructs a
    // partitioning key from the values that are produced by child0 and
    // are referenced in the equijoin predicates.
    // -----------------------------------------------------------------
    else if ((pws->getCountOfChildContexts() == 2) AND
        currentPlanIsAcceptable(planNumber,rppForMe))
    {
      if ( NOT CURRSTMT_OPTDEFAULTS->optimizerPruning() OR
           pws->isLatestContextWithinCostLimit() OR
           NOT CURRSTMT_OPTDEFAULTS->OPHpruneWhenCLExceeded() )
      {
      RequirementGenerator rg(child(0), rppForMe);
      ValueIdSet partitioningKey;

      if ( considerMixedHashJoin == TRUE && 
           getEquiJoinExprFromChild0().entries() > 1 ) {

        if ( vidOfEquiPredWithSkew != NULL_VALUE_ID ) {
 
           // For multi-column SB, use one of the 
           // the skew partition column in the equi predicates 
           // in the partition key. 
           ValueId vref = 
                   getEquiJoinExprFromChild0().
                       extractVEGRefForEquiPredicate(vidOfEquiPredWithSkew);
  
           // Expect to get the valueId of a VegRef for a join predicate.
           if (vref != NULL_VALUE_ID)
              partitioningKey.insert(vref);
           else
              partitioningKey.insertList(getEquiJoinExprFromChild0());

        } else
            partitioningKey.insertList(getEquiJoinExprFromChild0());

      } else
         // For regular HJ or single-column SB, use the only equi predicate
         // as the partition column. 
        partitioningKey.insertList(getEquiJoinExprFromChild0());

      rg.addPartitioningKey(partitioningKey);

      // --------------------------------------------------------------------
      // If this is a CPU or memory-intensive operator then add a
      // requirement for a maximum number of partitions, unless
      // that requirement would conflict with our parent's requirement.
      //
      // --------------------------------------------------------------------
      if (okToAttemptESPParallelism(myContext,
                                    pws,
                                    childNumPartsRequirement,
                                    childNumPartsAllowedDeviation,
                                    numOfESPsForced))
      {
        if (NOT numOfESPsForced)
          rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                    &childNumPartsAllowedDeviation);
        rg.addNumOfPartitions(childNumPartsRequirement,
                              childNumPartsAllowedDeviation);

        if ( considerMixedHashJoin == TRUE )
           rg.addSkewRequirement(skewProperty::UNIFORM_DISTRIBUTE, skList, broadcastOneRow);

      } // end if ok to try parallelism

      // Produce the requirements and make sure they are feasible.
      if (rg.checkFeasibility())
      {
        // Produce the required physical properties.
        rppForChild = rg.produceRequirement();
      }
      }
    } // end if 3rd child context

    // -----------------------------------------------------------------
    // (child1, plan1) is the counterpart plan for (child0, plan1).
    // -----------------------------------------------------------------
    else if ((pws->getCountOfChildContexts() == 3) AND
        currentPlanIsAcceptable(planNumber,rppForMe))
    {
      // -------------------------------------------------------------
      // Cost limit exceeded? Erase the Context (child0, plan1).
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // -------------------------------------------------------------
      if (pws->getLatestChildContext() AND
          NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost()
         )
        pws->eraseLatestContextFromWorkSpace();

      const Context* const childContext = pws->getChildContext(0,1);

      // -----------------------------------------------------------------
      // Make sure a plan has been produced by the latest context.
      // -----------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty*
              sppForChild = childContext->getPhysicalPropertyForSolution();
        // ---------------------------------------------------------------
        // spp should have been synthesized for child's optimal plan.
        // ---------------------------------------------------------------
        CMPASSERT(sppForChild != NULL);
        PartitioningFunction* childPartFunc =
                                sppForChild->getPartitioningFunction();
        NABoolean rewriteForChild0 = FALSE;
        PartitioningFunction* childPartFuncRewritten =
           mapPartitioningFunction(childPartFunc, rewriteForChild0);

        RequirementGenerator rg (child(1),rppForMe);

        // ignore single part func case since run-time skew does not exist.
        if ( notInLeftChildOptimization &&
             !childPartFuncRewritten->isASinglePartitionPartitioningFunction())
        {
          skewProperty skp (skewProperty::BROADCAST, skList);
          skp.setBroadcastOneRow(broadcastOneRow);
          //rg.addSkewRequirement(skewProperty::BROADCAST, skList, broadcastOneRow);
          childPartFuncRewritten = new SkewedDataPartitioningFunction(childPartFuncRewritten, skp);
        }



        PartitioningRequirement* partReqForChild =
          childPartFuncRewritten->makePartitioningRequirement();

        //RequirementGenerator rg (child(1),rppForMe);

	if (myContext->requiresOrder())
	{
	    // the ordering of an ordered hash join is provided by the left child
	    rg.removeSortKey();
	    rg.removeArrangement();
	}

        // Remove any parent partitioning requirements, since we
        // have already enforced this on the left child.
        rg.removeAllPartitioningRequirements();

        // Double check broadcasting skewed data requirment for the right child
        // if my left child will produce randomly distributed skewed values,
        // and the partfunc for the right child can do so and both partfuncs
        // are of same type. The last check is necessary to avoid
        // hash2-random_distribute on left side and some other non-hash2
        // broadcast on right side.
        //
        // The above called HashJoin::mapPartitioningFunction() should perform
        // the mapping from UNIFORM DISTRIBUTED to BROADCAST.
        const SkewedDataPartitioningFunction* skpf = NULL;
        if (childPartFunc->getPartitioningFunctionType() ==
            childPartFuncRewritten->getPartitioningFunctionType() AND
            (skpf=childPartFunc->castToSkewedDataPartitioningFunction()) AND
            skpf->getSkewProperty().isUniformDistributed()
           )
        {
           const SkewedDataPartitioningFunction* otherSKpf =
              (SkewedDataPartitioningFunction*)childPartFuncRewritten;

           CMPASSERT(
              otherSKpf->getPartialPartitioningFunction()->canHandleSkew()
                AND
              otherSKpf->getSkewProperty().isBroadcasted()
                    );
           //rg.addSkewRequirement(skewProperty::BROADCAST, skList);
        }


        // Now, add in the Join's partitioning requirement for the
        // right child.
        rg.addPartRequirement(partReqForChild);

        // Produce the requirements and make sure they are feasible.
        if (rg.checkFeasibility())
        {
          // Produce the required physical properties.
          rppForChild = rg.produceRequirement();
        }

      } // end if child0 had an optimal solution
    } // end if 4th child context

    // -----------------------------------------------------------------
    // The next plan, (child0, plan2) and (child1, plan2), attempt to
    // create matching partitions.  They are created iff an equijoin
    // predicate exists.  It differs from the previous plan in that a
    // new partitioning requirement is created and applied to child1
    // first, instead of to child0.  (child0, plan2) constructs a
    // partitioning key from the values that are produced by child1 and
    // are referenced in the equijoin predicates.
    // -----------------------------------------------------------------
    else if ((pws->getCountOfChildContexts() == 4) AND
        currentPlanIsAcceptable(planNumber,rppForMe) AND
        considerHashJoinPlan2 == TRUE)
    {

      if ( NOT CURRSTMT_OPTDEFAULTS->optimizerPruning() OR
           pws->isLatestContextWithinCostLimit() OR
           NOT CURRSTMT_OPTDEFAULTS->OPHpruneWhenCLExceeded() )
      {
      RequirementGenerator rg(child(1), rppForMe);

      if (myContext->requiresOrder())
      {
	  // the ordering of an ordered hash join is provided by the left child
	  rg.removeSortKey();
	  rg.removeArrangement();
	}

      // We must insist that the right child match the parent partitioning
      // requirements, because we are dealing with the right child first.
      // The right child will satisfy the parent somehow (if possible) and
      // the requirement we get from the right child will then be given to
      // the left child, and so the parent requirement will be enforced
      // on the left child in this way.
      // So, we don't remove the parent requirements.

      ValueIdSet partitioningKey;
      partitioningKey.insertList(getEquiJoinExprFromChild1());
      rg.addPartitioningKey(partitioningKey);

      // --------------------------------------------------------------------
      // If this is a CPU or memory-intensive operator then add a
      // requirement for a maximum number of partitions, unless
      // that requirement would conflict with our parent's requirement.
      //
      // --------------------------------------------------------------------
      if (okToAttemptESPParallelism(myContext,
                                    pws,
                                    childNumPartsRequirement,
                                    childNumPartsAllowedDeviation,
                                    numOfESPsForced))
      {
        if (NOT numOfESPsForced)
          rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                    &childNumPartsAllowedDeviation);
        rg.addNumOfPartitions(childNumPartsRequirement,
                              childNumPartsAllowedDeviation);
      } // end if ok to try parallelism

      // Produce the requirements and make sure they are feasible.
      if (rg.checkFeasibility())
      {
        // Produce the required physical properties.
        rppForChild = rg.produceRequirement();
      }
      }
    } // end if 5th child context

    // -----------------------------------------------------------------
    // (child0, plan2) is the counterpart plan for (child1, plan2).
    // -----------------------------------------------------------------
    else if ((pws->getCountOfChildContexts() == 5) AND
        currentPlanIsAcceptable(planNumber,rppForMe) AND
        considerHashJoinPlan2 == TRUE
       )
    {
      // -------------------------------------------------------------
      // Cost limit exceeded? Erase the Context (child1, plan2).
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // -------------------------------------------------------------
      if (pws->getLatestChildContext() AND
          NOT pws->isLatestContextWithinCostLimit() AND
          NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost()
         )
        pws->eraseLatestContextFromWorkSpace();

      const Context* const childContext = pws->getChildContext(1,2);

      // -----------------------------------------------------------------
      // Make sure a plan has been produced by the latest context.
      // -----------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty*
              sppForChild = childContext->getPhysicalPropertyForSolution();
        // ---------------------------------------------------------------
        // spp should have been synthesized for child's optimal plan.
        // ---------------------------------------------------------------
        CMPASSERT(sppForChild != NULL);
        PartitioningFunction* childPartFunc =
                                sppForChild->getPartitioningFunction();
        NABoolean rewriteForChild0 = TRUE;
        ValueIdMap map(getEquiJoinExpressions());
        PartitioningFunction* childPartFuncRewritten =
          childPartFunc->copyAndRemap(map,
                                      rewriteForChild0);
        PartitioningRequirement* partReqForChild =
          childPartFuncRewritten->makePartitioningRequirement();

        RequirementGenerator rg (child(0),rppForMe);
        // We have already applied the parent partitioning requirements
        // to the right child, so no need to apply them to the left child.
        rg.removeAllPartitioningRequirements();

        // Now, add in the Join's partitioning requirement for the
        // left child.
        rg.addPartRequirement(partReqForChild);

        // Produce the requirements and make sure they are feasible.
        if (rg.checkFeasibility())
        {
          // Produce the required physical properties.
          rppForChild = rg.produceRequirement();
        }

      } // end if child1 had an optimal solution
    } // end if 5th child context

    // -------------------------------------------------------------------
    // Create a Context using the partitioning requirement that was
    // generated for the current plan.
    // -------------------------------------------------------------------
    if (rppForChild != NULL)
    {

      // ---------------------------------------------------------------
      // Compute the cost limit to be applied to the child.
      // ---------------------------------------------------------------
      CostLimit* costLimit = computeCostLimit(myContext,pws);

      EstLogPropSharedPtr inputLogPropForChild;

      if (childIndex == 1 AND isReuse())
	{
	  // -----------------------------------------------------------
	  // Reusing logic from the Materialize node to calculate
	  // input logical properties for the child (see comment in
	  // Materialize::createContextForAChild)
	  // -----------------------------------------------------------
	  inputLogPropForChild =
	    child(1).getGroupAttr()->materializeInputLogProp(
	       myContext->getInputLogProp(),&multipleCalls_);

	}
      else
	inputLogPropForChild = myContext->getInputLogProp();



      // ---------------------------------------------------------------
      // Get a Context for optimizing the child.
      // Search for an existing Context in the CascadesGroup to which
      // the child belongs that requires the same properties as those
      // in rppForChild. Reuse it, if found. Otherwise, create a new
      // Context that contains rppForChild as the required physical
      // properties.
      // ----------------------------------------------------------------
      result = shareContext(childIndex, rppForChild,
                            myContext->getInputPhysicalProperty(),
                            costLimit,
                            myContext, inputLogPropForChild );

      NABoolean pruneFixOff = (CmpCommon::getDefault(OPTIMIZER_PRUNING_FIX_1) == DF_OFF);

      if ( (NOT (pws->isLatestContextWithinCostLimit() OR
                 result->hasSolution() )) AND
                (CURRSTMT_OPTDEFAULTS->OPHpruneWhenCLExceeded() OR pruneFixOff)
         )
       result = NULL;
    }
    else
      result = NULL;

    // -------------------------------------------------------------------
    // Remember the cases for which a Context could not be generated,
    // or store the context that was generated.
    // -------------------------------------------------------------------
    pws->storeChildContext(childIndex, planNumber, result);
    pws->incPlanChildCount();

    // To prevent mixing children for knownChildrenCost we want to
    // exit when plan failed for the right or left (therefore for
    // the right also) child. The only case we can stay in this loop is
    // when left child failed and we want to store NULL context for
    // both children and need to advance in this loop without
    // creating an extra CreatePlanTask, which was probably an initial
    // intention. 

    if ( CURRSTMT_OPTDEFAULTS->optimizerPruning() AND
         ( pws->getPlanChildCount() == getArity() ) AND
         CURRSTMT_OPTDEFAULTS->OPHexitHJcrContChiLoop()
       )
    {
      pws->resetAllChildrenContextsConsidered();
      break;
    }
  }  // end while loop

  if ( pws->getCountOfChildContexts() == childPlansToConsider )
    pws->setAllChildrenContextsConsidered();

  return result;

} // HashJoin::createContextForAChild()

NABoolean HashJoin::currentPlanIsAcceptable(Lng32 planNo,
            const ReqdPhysicalProperty* const rppForMe) const
{
  // ---------------------------------------------------------------------
  // Check whether the user wants to enforce a particular plan type.
  // ---------------------------------------------------------------------

  // If nothing is being forced, return TRUE now.
  if (rppForMe->getMustMatch() == NULL)
    return TRUE;

  // Check for the correct forced plan type.

  JoinForceWildCard::forcedPlanEnum forcePlanToken =
       getParallelJoinPlanToEnforce(rppForMe);

  switch (forcePlanToken)
  {
    case JoinForceWildCard::FORCED_PLAN0:
      if (planNo != 0)
        return FALSE;
      break;
    case JoinForceWildCard::FORCED_PLAN1:
      if (planNo != 1)
        return FALSE;
      break;
    case JoinForceWildCard::FORCED_PLAN2:
      if (planNo != 2)
        return FALSE;
      break;
    case JoinForceWildCard::FORCED_TYPE1:
      if (planNo == 0)
        return FALSE;
      break;
    case JoinForceWildCard::FORCED_TYPE2:
      if (planNo != 0)
        return FALSE;
      break;
    case JoinForceWildCard::ANY_PLAN:
      // Any plan satisfies this - break out so we'll return TRUE
      break;
    default:
      return FALSE; // must be some option that doesn't apply to Hash Join
  }

  // If we get here, the plan must have passed all the checks.
  return TRUE;

} // HashJoin::currentPlanIsAcceptable()

NABoolean HashJoin::okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/)
{
  const ReqdPhysicalProperty* rppForMe =
     myContext->getReqdPhysicalProperty();

  NABoolean result = FALSE;

  DefaultToken parallelControlSettings =
    getParallelControlSettings(rppForMe,
                               numOfESPs,
                               allowedDeviation,
                               numOfESPsForced);

  if (parallelControlSettings == DF_OFF)
  {
    result = FALSE;
  }

  else if ( (parallelControlSettings == DF_MAXIMUM)  AND
             CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible()
          )
  {
    numOfESPs = rppForMe->getCountOfPipelines();
    // currently, numberOfPartitionsDeviation_ is set to 0 in
    // OptDefaults when ATTEMT_ESP_PARALLELISM is 'MAXIMUM'
    allowedDeviation =  CURRSTMT_OPTDEFAULTS->numberOfPartitionsDeviation();

    // allow deviation by default
    if (CmpCommon::getDefault(COMP_BOOL_63) == DF_OFF)
    {
      EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
      EstLogPropSharedPtr child0OutputLogProp = child(0).outputLogProp(inLogProp);
      const CostScalar child0RowCount =
        (child0OutputLogProp->getResultCardinality()).minCsOne();
      if ( child0RowCount.getCeiling() <
            MINOF(numOfESPs,CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold())
         )
      {
        // Fewer outer table rows then pipelines - allow one or more parts
        allowedDeviation = 1.0;
      }
    }
    result = TRUE;
  }

  else if (parallelControlSettings == DF_ON)
  {
    // Either user wants to try ESP parallelism for all operators,
    // or they are forcing the number of ESPs for this operator.
    // Set the result to TRUE. If the number of ESPs is not being forced,
    // set the number of ESPs that should be used to the maximum number.
    // Set the allowable deviation to either the default or, for type2
    // plans where no default was specified, a percentage that allows any
    // number of partitions from 2 to the maximum number. i.e. allow
    // the natural partitioning of the child as long as the child is
    // partitioned and does not have more partitions than max pipelines.
    // NEW HEURISTIC: If there are fewer outer table rows than the number
    // of pipelines, then set the deviation to allow any level of natural
    // partitioning, including one. This is because we don't want to
    // repartition so few rows to get more parallelism, since we would
    // end up with a lot of ESPs doing nothing.
    if (NOT numOfESPsForced)
    {
      // Determine the number of outer table rows
      EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
      EstLogPropSharedPtr child0OutputLogProp = child(0).outputLogProp(inLogProp);
      const CostScalar child0RowCount =
        (child0OutputLogProp->getResultCardinality()).minCsOne();
      numOfESPs = rppForMe->getCountOfPipelines();
      if (child0RowCount.getCeiling() < numOfESPs)
      {
        // Fewer outer table rows then pipelines - allow one or more parts
        allowedDeviation = 1.0;
      }
      else
      {
        if (pws->getCountOfChildContexts() == 0)
        {
          // Type 2 plan.
          if ( CURRSTMT_OPTDEFAULTS->deviationType2JoinsSystem() )
          {
            // ------------------------------------------------------
            // A value for NUM_OF_PARTS_DEVIATION_TYPE2_JOINS exists.
            // ------------------------------------------------------
            allowedDeviation =  CURRSTMT_OPTDEFAULTS->numOfPartsDeviationType2Joins();
          }
          else
          {
            // Need to make 2 the minimum number of parts to support. Use
            // 1.99 to protect against rounding errors.
            allowedDeviation =
              ((float)numOfESPs - 1.99f) / (float)numOfESPs;
          }
        }
        else
        {
          // Type 1 plan.
          allowedDeviation = CURRSTMT_OPTDEFAULTS->numberOfPartitionsDeviation();
        }
      } // end if fewer outer table rows than pipelines
    } // end if number of ESPs not forced
    result = TRUE;
  }
  else
  {
    // Otherwise, the user must have specified "SYSTEM" for the
    // ATTEMPT_ESP_PARALLELISM default. This means it is up to the
    // optimizer to decide.

    // Return TRUE if the size of the inner table in KB exceeds the
    // memory limit for BMOs. Also return TRUE if
    // the # of rows returned by child(0) exceeds the threshold from the
    // defaults table. The recommended number of ESPs is also computed
    // to be 1 process per <threshold> number of rows. This is then
    // used to indicate the MINIMUM number of ESPs that will be
    // acceptable. This is done by setting the allowable deviation
    // to a percentage of the maximum number of partitions such
    // that the recommended number of partitions is the lowest
    // number allowed. We make the recommended number of partitions
    // a minimum instead of a hard requirement because we don't
    // want to be forced to repartition the child just to get "less"
    // parallelism.

    EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
    EstLogPropSharedPtr child0OutputLogProp = child(0).outputLogProp(inLogProp);
    CostScalar rowCount =
      (child0OutputLogProp->getResultCardinality()).minCsOne();

    // This is to test better parallelism taking into account
    // not only child0 but also this operator cardinality
    if(CmpCommon::getDefault(COMP_BOOL_125) == DF_ON)
    {
      rowCount += (getGroupAttr()->outputLogProp(inLogProp)->
                         getResultCardinality()).minCsOne();
    }

    EstLogPropSharedPtr child1OutputLogProp = child(1).outputLogProp(inLogProp);
    const CostScalar child1RowCount =
      (child1OutputLogProp->getResultCardinality()).minCsOne();

    const Lng32 child1RowLength =
      child(1).getGroupAttr()->getCharacteristicOutputs().getRowLength();
    const Lng32 extChild1RowLength = child1RowLength + 8;
    const CostScalar child1SizeInKB =
      ((child1RowCount * extChild1RowLength) / 1024.);

    const CostScalar numberOfRowsThreshold =
      CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold();
    const CostScalar bigMemoryLimit = CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU();

    // First check for the inner table exceeding the memory limit,
    // since for a HJ it is more important to have the number of ESPs
    // used based on the memory requirements of the inner table than
    // the number of rows of the outer table.

    CostScalar parThreshold = child1SizeInKB / bigMemoryLimit;

    // If we consider only child1 to define the level of parallelism
    // then deviation could be too close to 1 and we can have not enough
    // parallelism from child0 point of view

    parThreshold = MAXOF(parThreshold, rowCount / numberOfRowsThreshold);

    if (parThreshold.isGreaterThanOne())
    {
      numOfESPs = rppForMe->getCountOfPipelines();
      allowedDeviation = (float) MAXOF(1.001 -
        ceil(parThreshold.value() / numOfESPs), 0);
      result = TRUE;
    }
    else // no parallelism needed
    {
      result = FALSE;
    }
  } // end if the user let the optimizer decide

  return result;

} // HashJoin::okToAttemptESPParallelism()

//<pb>
//==============================================================================
//  Starting with a cost limit from a specified hash join operator's context,
// produce a new cost limit for a hash join operator's child by accumulating the
// hash join operator's preliminary cost and directly subtracting out the
// elapsed time of the known children cost.  Both the preliminary cost and the
// known children cost come from the hash join operator's plan workspace.
//
//
// Input:
//  myContext  -- specified context hash join operator.
//
//  pws        -- specified plan workspace for hash join operator.
//
// Output:
//  none
//
// Return:
//  Copy of accumulated cost limit.  NULL if context contains no cost limit.
//
//==============================================================================
CostLimit*
HashJoin::computeCostLimit(const Context*       myContext,
                                 PlanWorkSpace* pws)
{

  //----------------------------------------------------------------------------
  //  Context contains no cost limit.  Interpret this as an infinite cost limit.
  // Returning NULL indicates an infinite cost limit.
  //----------------------------------------------------------------------------
  if (myContext->getCostLimit() == NULL)
    {
      return NULL;
    }

  //---------------------------------------------------
  //  Create copy of cost limit from specified context.
  //---------------------------------------------------
  CostLimit* costLimit = myContext->getCostLimit()->copy();

  //---------------------------------------------------------------------
  //  Accumulate this operator's preliminary cost into the ancestor cost.
  //---------------------------------------------------------------------
  Cost* tempCost = pws->getOperatorCost();
  costLimit->ancestorAccum(*tempCost, myContext->getReqdPhysicalProperty());

  //-----------------------------------------------------------------------
  //  For hash joins, we know that the right child must complete before the
  // the left child can begin, so we can directly subtract the elapsed time
  // of one child's cost from the cost limit given to the other child.
  //-----------------------------------------------------------------------
  const ReqdPhysicalProperty* rpp = myContext->getReqdPhysicalProperty();
  tempCost = pws->getKnownChildrenCost();
  if (tempCost != 0)
    {
      ElapsedTime et = tempCost->convertToElapsedTime(rpp);
      costLimit->unilaterallyReduce(et);
    }

  //---------------------------------------------------------------------------
  //  Use best plan so far for this operator to try and further reduce the cost
  // limit.
  //---------------------------------------------------------------------------
  tempCost = pws->getBestRollUpCostSoFar();
  if (tempCost != NULL)
    {
      costLimit->tryToReduce(*tempCost,rpp);
    }

  return costLimit;

} // HashJoin::computeCostLimit()

//<pb>
//==============================================================================
//  Synthesize physical properties for hash join operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used for
//                 synthesizing partitioning functions.
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
HashJoin::synthPhysicalProperty(const Context* myContext,
                                const Lng32     planNumber,
                                PlanWorkSpace  *pws)
{
  if(isNoOverflow()){
    CMPASSERT( (getOperatorType() == REL_ORDERED_HASH_JOIN) OR
             (getOperatorType() == REL_ORDERED_HASH_SEMIJOIN) OR
	     (getOperatorType() == REL_ORDERED_HASH_ANTI_SEMIJOIN) OR
             (getOperatorType() == REL_LEFT_ORDERED_HASH_JOIN) OR
	     (getOperatorType() == REL_HYBRID_HASH_JOIN AND
	       isOrderedCrossProduct()));  // hybrid_hash_join used to implement a
                                         // cross product should not overflow.
  }
  else
    CMPASSERT( (getOperatorType() == REL_HYBRID_HASH_JOIN) OR
             (getOperatorType() == REL_HYBRID_HASH_SEMIJOIN) OR
	     (getOperatorType() == REL_HYBRID_HASH_ANTI_SEMIJOIN) OR
             (getOperatorType() == REL_LEFT_HYBRID_HASH_JOIN) OR
	       (getOperatorType() == REL_FULL_HYBRID_HASH_JOIN));

  const PhysicalProperty* const sppOfLeftChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(0);

  const PhysicalProperty* const sppOfRightChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(1);


  ValueIdList emptySortKey;

  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(myContext,
                                                             planNumber,
                                                             pws);

  // ----------------------------------------------------------------
  // Synthesize the partitioning function from the first child of the
  // winning plan.  Hash Joins have three potential plans--see member
  // function HashJoin::createContextForAChild().  In plans 0 and 1,
  // the left child is first; in plan 2, the right child is first.
  // ----------------------------------------------------------------
  CMPASSERT(planNumber == 0 || planNumber == 1 || planNumber == 2);
  PartitioningFunction* myPartFunc;
  const SearchKey* myPartSearchKey;
  if (planNumber == 0 || planNumber == 1)
    {
      myPartFunc = sppOfLeftChild->getPartitioningFunction();
      myPartSearchKey = sppOfLeftChild->getPartSearchKey();
    }
  else
    {
      myPartFunc = sppOfRightChild->getPartitioningFunction();
      myPartSearchKey = sppOfRightChild->getPartSearchKey();
    }

  // Can not strip off skewness property from the (hash2) partfunc, because
  // otherwise the hash join output can be joined with another
  // normal hash2 part func. This is incorrect because only
  // hash2WithBroadcase or replicateViaBroadcast is compatible with
  // hash2RandomDistribute part func. Other combinations will lead to
  // incorrect join results.


  PhysicalProperty* sppForMe;
  NABoolean canAppendRightColumns = FALSE;
  NABoolean childSortOrderTypesAreSame = FALSE;

  if (isOrderedCrossProduct()) {
      // right columns can be appended to give (left child order, right child order)
      // only if left child rows are unique.
      // see comment in NestedJoin::synthPhysicalProperties for an explanation.
      GroupAttributes *leftGA = child(0).getGroupAttr();
      ValueIdSet leftSortCols;

      // make the list of sort cols into a ValueIdSet
      leftSortCols.insertList(sppOfLeftChild->getSortKey());

      // check for uniqueness of the sort columns
      if (leftGA->isUnique(leftSortCols))
      {
	canAppendRightColumns = TRUE;
	// The child sort order types are the same if they are equal and both
        // children's dp2SortOrderPartFunc are the same (as in both being
        // NULL), or they are both not null but they are equivalent.
        if ((sppOfLeftChild->getSortOrderType() ==
             sppOfRightChild->getSortOrderType()) AND

            ((sppOfLeftChild->getDp2SortOrderPartFunc() ==
             sppOfRightChild->getDp2SortOrderPartFunc()) OR
             ((sppOfLeftChild->getDp2SortOrderPartFunc() != NULL) AND
              (sppOfRightChild->getDp2SortOrderPartFunc() != NULL) AND
              (sppOfLeftChild->getDp2SortOrderPartFunc()->
                 comparePartFuncToFunc(
                  *sppOfRightChild->getDp2SortOrderPartFunc()) == SAME)
             )
            )
           )
         childSortOrderTypesAreSame = TRUE;
      }
      if (NOT (canAppendRightColumns AND
		childSortOrderTypesAreSame))
      {
	// even though this node seemed to be an appropriate choice
	// to be an order preserving cross product in HashJoinRule::nextSubstitute,
	// we now find due to (a) non-uniqueness of left child
	// or (b) child sort orders not being the same that this cross
	// product cannot be order preserving
	// So we are unsetting the flag so that right join type will be shown by explain
	// Overflow is set to false because a non-crossproduct HHJ has overflow set
	// to false no ordering is promised.
	setIsOrderedCrossProduct(FALSE);
	setNoOverflow(FALSE);
      }
  }

  if(isNoOverflow())
  {
    // ---------------------------------------------------------------------
    // the ordered join delivers its result sorted (with respect to the left child)
    // and preserves the partitioning of the left child table
    // ---------------------------------------------------------------------
    SortOrderTypeEnum newSortOrderType = sppOfLeftChild->getSortOrderType();
    ValueIdList newSortKey;
    newSortKey.insert(sppOfLeftChild->getSortKey());

    // if this Hash Join is implementing a order preserving cross product
    // then set the physical properties to have (left child order, right child order).
    if (canAppendRightColumns && childSortOrderTypesAreSame)
    {
      const ValueIdList & rightSortKey = sppOfRightChild->getSortKey();
      for (Lng32 i = 0; i < (Lng32)rightSortKey.entries(); i++)
	newSortKey.insert(rightSortKey[i]);
    }

    PartitioningFunction *
      newDP2SortOrderPartFunc = sppOfLeftChild->getDp2SortOrderPartFunc();

    sppForMe =
      new(CmpCommon::statementHeap()) PhysicalProperty(
	   newSortKey,
	   newSortOrderType, // no sort key, so no sort order type either
           newDP2SortOrderPartFunc,
	   myPartFunc,
	   sppOfLeftChild->getPlanExecutionLocation(), //||opt synth fr both kids
	   combineDataSources(
		sppOfLeftChild->getDataSourceEnum(),
		sppOfRightChild->getDataSourceEnum()),
	   sppOfLeftChild->getIndexDesc(),
	   myPartSearchKey);

  }

  else
  {
    // ---------------------------------------------------------------------
    // the hybrid join delivers its result unsorted and preserves the
    // partitioning of the left child table
    // ---------------------------------------------------------------------
    sppForMe =
      new(CmpCommon::statementHeap()) PhysicalProperty(
	   emptySortKey,
	   NO_SOT, // no sort key, so no sort order type either
	   NULL, // no dp2SortOrderPartFunc either
	   myPartFunc,
	   sppOfLeftChild->getPlanExecutionLocation(), //||opt synth fr both kids
	   combineDataSources(
		sppOfLeftChild->getDataSourceEnum(),
		sppOfRightChild->getDataSourceEnum()),
	   sppOfLeftChild->getIndexDesc(),
	   myPartSearchKey);
  }

  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());

  // remove anything that's not covered by the group attributes
  // (NB: no real need to call this method, since the sortKey is empty,
  // and that's the only thing that this method checks currently)
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  // transfer the onePartitionAccess flag to join.
  // We check the flag in HashJoin::computeOperatorPriority().
  setInnerAccessOnePartition(sppOfRightChild->getAccessOnePartition());

  delete sppTemp;

  return sppForMe;

} // HashJoin::synthPhysicalProperty()

//<pb>

// -----------------------------------------------------------------------
// member functions for class Union
// -----------------------------------------------------------------------

NABoolean Union::rppAreCompatibleWithOperator
                    (const ReqdPhysicalProperty* const rppForMe) const
{
  if ( rppForMe->executeInDP2() )
  {
    if ( !CURRSTMT_OPTDEFAULTS->pushDownDP2Requested() )
       return FALSE;

    // no push-down union if the containing CS is not pushed down
    if ( isinBlockStmt() == TRUE )
    { 
       if ( NOT PushDownCSRequirement::isInstanceOf(
                   rppForMe->getPushDownRequirement()
                                                )
        )
          return FALSE;
    } else {
       // no push-down union if it is not the one introduced 
       // for the IUD log table associated with a MV. 
       if ( getInliningInfo().isUsedForMvLogging() == FALSE )
          return FALSE;
    }
  }

  // ---------------------------------------------------------------------
  // If a partitioning requirement is specified, and it specifies some
  // required partitioning key columns, then ensure that the required
  // partitioning keys are contained in the output columns of the Union.
  // ---------------------------------------------------------------------
  if (rppForMe->requiresPartitioning() AND
      NOT rppForMe->
          getPartitioningRequirement()->getPartitioningKey().isEmpty() AND
      NOT getUnionForIF())
    {
      // -----------------------------------------------------------------
      // Construct a ValueIdSet of expressions that are produced as
      // output by the Union.
      // -----------------------------------------------------------------
      ValueIdSet partKeyForUnion(colMapTable());
      // -----------------------------------------------------------------
      // If the (potential) partitioning key for the Union is contained
      // in the partitioning key for the partitioning requirement, then
      // the Union is capable of satisfying that requirement.
      // -----------------------------------------------------------------
      return partKeyForUnion.contains
                               (rppForMe->getPartitioningRequirement()
                                             ->getPartitioningKey());
    } // endif (rppForMe->requiresPartitioning())


  return TRUE;

} // Union::rppAreCompatibleWithOperator

// -----------------------------------------------------------------------
// member functions for class MergeUnion
// -----------------------------------------------------------------------

//<pb>
// -----------------------------------------------------------------------
// MergeUnion::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
MergeUnion::costMethod() const
{
  static THREAD_P CostMethodMergeUnion *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodMergeUnion();
  return m;
} // MergeUnion::costMethod()

// ---------------------------------------------------------------------
// Performs mapping on the partitioning function, from the
// union to the designated child.
// ---------------------------------------------------------------------
PartitioningFunction* MergeUnion::mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0)
{
  NABoolean mapItUp = FALSE;
  if (rewriteForChild0)
    return partFunc->copyAndRemap(getMap(0),mapItUp);
  else
    return partFunc->copyAndRemap(getMap(1),mapItUp);
} // end MergeUnion::mapPartitioningFunction()

//<pb>
// -----------------------------------------------------------------------
// Given an input synthesized child sort key, a required arrangement,
// and a input required sort order (if any), generate a new sort key
// that has all the expressions in the required sort key and required
// arrangement, and only those expressions, in an order that matches
// the child sort key.  Return this new sort key, which can then be used
// as a sort key requirement for the other child of the union or as the
// synthesized union sort key after being mapped.
//
// One reason why we need this method: If the required arrangement contains
// an simple arithmethic expression, like "b+1", it is possible that the
// sort key that is synthesized will be "b". So, when we try to map the
// sort key to the other side of the union, to use it as a requirement,
// we will not map the "b" to "b+1", which is what we want. We will either
// fail to map "b" or, if the select list of the union contained "b" in
// addition to "b+1", we will map "b" to "b" - even though "b" may not
// have been in the required arrangement. This would cause us to require
// something from the other side of the union that was not in
// the original required arrangement. This could lead to incorrect results.
// This is because even though the one side of the union selected
// "b+1" and "b", and they happen to be equivalent as far as sort order
// is concerned, there is no guarantee that their positional equivalents
// on the other side of the union will have equivalent sort orders.
// For example, take the following query:
//
//   Select y,sum(z)
//   from (select b+1,b
//         from t005t1
//         union all
//         select c,a+1
//         from t005t1) as t(y,z)
//   group by y;
//
// Notice:
// "b+1" on the left side of the union maps to "c" on the right side.
// "b" on the left side of the union maps to "a+1" on the right side.
//
// "c" and "a+1" DO NOT have equivalent sort orders. If we map the
// sort key we get from the left side to "b", even though "b+1" is
// what was required, we will then map "b" from the left side to
// "a+1" on the right side - even though we need to require the
// right side to be ordered on "c" ! THIS WILL LEAD TO WRONG ANSWERS!
//
// So, this is why we need to make sure that the sort key
// from one side of the union contains only Value Id's from the
// required sort key and arrangement (as translated to be in terms of
// the Value Id's from that side of the union), before we can use the
// sort key as a requirement for the other side of the union or as the
// synthesized sort key for the union itself.
//
// NOTE: One thing this method does not handle, and the union mapping
// in general cannot handle, is if the user selects the exact same
// column more than once, such as
//      "select b,b,b from t union select x,y,z from t2".
//   The mapping wouldn't know which "b" to map to. Also, the
// extra "b"'s would always disappear from the required arrangement
// for the left child, since a required arrangement is a ValueIdSet,
// and a ValueIdSet does not allow duplicates. So, we would never
// decide upon an order for the extra "b"'s, and so there would be
// no required order for "y" and "z" on the other side of the union,
// for the plan where we talk to the left child first. This will
// yield incorrect results. To solve this, we must refuse to
// synthesize any sort keys, partitioning keys, etc. for the union,
// because we cannot get it right. One way to do this would be to
// disallow entries in the union map for any duplicate select list
// entries. This would cause the mapping for these expressions to
// always fail and so the children could never satisfy any
// requirements that require mapping these expreessions.
// We don't do this now - this is a bug.
// -----------------------------------------------------------------------
void MergeUnion::synthUnionSortKeyFromChild (
       const ValueIdList &childSortKey, /* input */
       const ValueIdSet &reqArrangement, /* input */
       ValueIdList &unionSortKey /* input,output */
                                         ) const
{
  // Make a copy of the req arrangement so we can delete from it
  // every time we add an entry to the union sort key. This way we
  // can make sure all required arrangement columns are present in
  // the union sort key we produce.
  ValueIdSet reqArrangementCopy = reqArrangement;

  // Translate the input unionSortKey, which represents any required
  // sort key, into a set, with any INVERSE nodes (i.e. DESC order)
  // stripped off. This is so we can make sure any required
  // arrangement expression is not part of the required sort key,
  // as we do not want any duplicate expressions in the union sort
  // key we produce. Note that we don't get the simplified form of
  // the required sort key cols, because we want to check the
  // unsimplified form of the required arrangement expression against
  // the unsimplified form of the required sort key.  For example,
  // even though b+1 is already in the required sort key, we would
  // still want to include "b" if it was in the required arrangement,
  // because these two do not represent the same piece of data outside
  // of the union or on the other side of the union. But, if "b+1"
  // is in both the req. sort key and req. arrangement, then clearly
  // this is the same piece of data and we don't want to include it again.
  //
  ValueIdSet reqSortKeyCols;
  ValueId noInverseVid;
  for (CollIndex j = 0; j < unionSortKey.entries(); j++)
  {
    noInverseVid = unionSortKey[j].getItemExpr()->
                     removeInverseOrder()->getValueId();
    reqSortKeyCols += noInverseVid;
  }

  ValueId sortKeySvid, reqVid, reqSvid;
  CollIndex i = 0;
  NABoolean done = FALSE;
  OrderComparison order1,order2;

  ValueIdSet childSortKeyProcessed;

  // Loop until we've processed all the sort key entries, or until
  // we find a sort key entry that is not in the required arrangement.
  while (!done && i < childSortKey.entries())
  {
    done = TRUE; // Assume we won't find a match

    sortKeySvid = childSortKey[i].getItemExpr()->
      simplifyOrderExpr(&order1)->getValueId();

    // The following 'if' assures that duplicated simplified sort key
    // expressions are not processed.
    //
    // Here is an exmaple of childSortKey with duplicates.
    // The index I on T(a), where T(a,b) has both a and b as primary key column
    // will generate child sort key list [a, a, b]. The list is generated
    // inside IndexDesc::IndexDesc() under name orderOfPartitioningKeyValues_.
    //
    // As another example, I1 on T(a desc).
    //
    // Duplications require extra sort requirement for the other side; it's bad
    // from performance point of view. Worse, the method finalizeUnionSortKey()
    // requires the size of unionSortKey to be the same as the reqArrangement.
    // So childSortKey with duplicates will lead to an assertion failure there.
    //
    // We can remove duplicates simplified expressions because they do not
    // provide any useful information, as in the following code,
    // regSvid (=sortKeySvid, simplified) becomes part of the union sort key.
    //
    // The logic is added to fix Solution 10-020913-1676.

    if ( NOT childSortKeyProcessed.contains(sortKeySvid) )
    {
      childSortKeyProcessed.insert(sortKeySvid);

      // Get the value id for the simplified form of the sort key expression.

      // For each sort key column, loop over all required arrangement expr.s
      // There may be multiple arrangement expressions that match the same
      // sort key entry, e.g. b+1 and b+2 will both match a sort key entry of b
      for (reqVid = reqArrangement.init();
	   reqArrangement.next(reqVid);
	   reqArrangement.advance(reqVid))
	{
	  // Get the value id for the simplified form of the required
	  // arrangement expression.
	  reqSvid = reqVid.getItemExpr()->
	    simplifyOrderExpr(&order2)->getValueId();

	  if (sortKeySvid == reqSvid)
	    {
	      // Remove the req. arrangement expression from the copy
	      reqArrangementCopy.remove(reqVid);
	      done = FALSE; //found this sort key entry in the req. arrangement
	      // Add the required arrangement expression if it is not part
	      // of the required sort key.
	      if (NOT reqSortKeyCols.contains(reqVid))
	      {
		// If the sort key expression and the required arrangement
		// expression do not specify the same order, then
		// generate an INVERSE expression on top of the req. arr. expr.
		if (order1 != order2)
		{
		  InverseOrder * inverseExpr =
		    new(CmpCommon::statementHeap())
		    InverseOrder (reqVid.getItemExpr());
		  inverseExpr->synthTypeAndValueId();
		  // Add the INVERSE expression plus it's child (which is the
		  // val Id from the required arrangement) to the new sort key
		  unionSortKey.insert(inverseExpr->getValueId());
		}
		else
		{
		  // Add the value Id from the requirement to the new sort key.
		  unionSortKey.insert(reqVid);
		}
	      } // end if not a duplicate entry
	    } // end if found sort key entry in the req. arrangement

	} // end for all req. arrangement entries
    } // end if not contained in childSortKeyProcessed
    i++;
  } // end while more sort key entries to process

  // If there are any required arrangement columns left, this means
  // we did not find them in the synthesized child sort key. This
  // means they must have been equal to a constant, so just tack any
  // remaining arrangement expressions with ASC order to the end of
  // the union sort key if they are not part of the required sort key.
  for (reqVid = reqArrangementCopy.init();
                reqArrangementCopy.next(reqVid);
                reqArrangementCopy.advance(reqVid))
  {
    // Add the required arrangement expression if it is not part
    // of the required sort key.
    if (NOT reqSortKeyCols.contains(reqVid))
    {
      unionSortKey.insert(reqVid);
    }
  } // end for all req. arrangement entries

} // MergeUnion::synthUnionSortKeyFromChild()


// This help function tries to make the Union sort key the same
// length as the arrangement requirement of Union cols. The key
// can be shorter when some of the Union columns are duplicated,
// as t2.i2 in the query
//
//  SELECT T2.i2, T2.i2, T2.i2 FROM D2 T2
//    UNION
//  SELECT -39 , T0.i2, t0.i2 FROM D1 T0  ;
//
// Because the Union sort key is generated based on the first
// optimized child of the Union, and is used in derivation of
// the sort order requirement for the un-optimized child, it is
// important for the Union sort key to maintain all the arragement
// (sort) requirements from the parent of the Union. Otherwise,
// the output from the other child may not be properly sorted, and
// makes it impossible to completely remove the duplicated Unioned
// rows.
//
// The task is accomplished by appending to the Union sort key those
// ValueIds in the arrangement set that are missing from the key.
//
// This function is added to fix CR 10-010502-2586 (MX0412
// With partitioned tables get assertion failure in OptPhysRelExpr).
//
// Args:
//   knownSortKey:  the sort key already generated
//   knownSide:     on which side the sort key is generated (0 left, 1 right)
//   arrCols:       the arragement requirement on columns
//   unionSortKey:  the union sort key to extend if necessary.
//
NABoolean
MergeUnion::finalizeUnionSortKey(const ValueIdList& knownSortKey,
                                 Lng32 knownSide,
                                 const ValueIdSet& arrCols,
                                 ValueIdList& unionSortKey)
{

  // Before finalizing the sortKey, we shall remove any duplicate
  // entries that are already present in the unionSortKey
  // This takes care of queries like
  //  SELECT T0.i2 , T0.i2 , T0.i2 FROM d1 T0
  //  UNION
  //  VALUES( '1', '2', '3')
  //  ORDER BY 1, 2 ;
  // If there are any duplicate entries coming from the arrangement
  // requirement, those will still be retained. Sol: 10-030708-7691

  // We only search for a simplified version of the Union sort key
  // in which all inverse nodes (on top of a sort order) are removed.

  ValueIdList simplifiedUnionSortKey = unionSortKey.removeInverseOrder();

  ValueIdSet copyUnionSortKey(simplifiedUnionSortKey); // to avoid any duplicates
  ValueIdList sortKeyWithoutDuplicates;

  for (CollIndex i = 0; i < simplifiedUnionSortKey.entries(); i++)
  {
    ValueId vid = simplifiedUnionSortKey[i];

    if ( copyUnionSortKey.contains(vid))
    {
      // It has still not been added to new key without duplicates, so add
      // in there and remove all its entries from the copy, to make sure it is
      // not added again
      sortKeyWithoutDuplicates.insert(unionSortKey[i]);
      copyUnionSortKey -= vid;
    }
  }

  DCMPASSERT(copyUnionSortKey.entries() == 0);

  // copy back the result in UnionSortKey for further processing
  unionSortKey = sortKeyWithoutDuplicates;


  // For the time being, till Sol: 10-030812-8714 is fixed,
  // we would like to disable this piece of
  // code if the sorting order is different from the arrangement
  // This is done by returning FALSE. This is checked at the time
  // we are creating context for the child.

  if (unionSortKey.entries() != arrCols.entries() )
    return FALSE;

  if ( unionSortKey.entries() < arrCols.entries() ) {

      CollIndex index = unionSortKey.entries();

     for (ValueId vid = arrCols.init(); arrCols.next(vid);
                  arrCols.advance(vid))
     {
        if ( NOT simplifiedUnionSortKey.contains(vid) ) {

          // duplications of source vids, which means duplicated
          // columns in the Union columns. We need to insert such vids
          // into the UnionSortKey. But before that, we need to determine
          // the ASC or DESC order for each such vid.
          //
          // Added to fix CR 10-010814-4545 - assertion failure
          // (unionSortKey.entries() == arrCols.entries())

          // First get the source valueId corresponding to the vid.
          ValueId sourceVid =
              ((ValueIdUnion*)vid.getItemExpr())->getSource(knownSide);

          // If the source vid is in the known sort key (directly),
          // then the vid corresponds to an ASC order. Otherwise, the vid
          // is hidden below an InverseOrder node, and hence the DESC
          // order. Note the vid can only have one sort order.
          //
          if ( knownSortKey.contains(sourceVid) ) // ASC
          {
             unionSortKey.insertAt(index++,vid);
          } else {                                // DESC
             // fabricate a new InverseOrder node.
             ItemExpr *inverseCol = new(CmpCommon::statementHeap())
                    InverseOrder(vid.getItemExpr());
             inverseCol->synthTypeAndValueId();
             unionSortKey.insertAt(index++, inverseCol->getValueId());
          }
        }
     }
   }

   CMPASSERT(unionSortKey.entries() == arrCols.entries());
   return TRUE;
}

//<pb>
Context* MergeUnion::createContextForAChild(Context* myContext,
                                  PlanWorkSpace* pws,
                                  Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  //   Merge Union generates at most 2 context pairs. The first is
  // either a non-parallel plan or a matching partitions parallel plan,
  // where we generate the left child context first. The second pair is
  // either a non-parallel plan or a matching partitions parallel plan,
  // where we generate the right child context first.
  //   The reason we try matching partitions plans both ways is to try
  // and avoid having to repartition both tables. If we only try one
  // way and the first table must repartition, then the second table
  // must repartition if it is going to be able to match the hash
  // repartitioning function that the first table synthesized. If we
  // were to try the other way and the first table this time did not
  // need to repartition, then we would only have to range repartition
  // the second table.
  //   The reason we might try non-parallel plans both ways is
  // because if there is a required arrangement, then the
  // first child tried only has to match the required arrangement,
  // but the second child must match a required order.
  // This might force us to sort the second child
  // since it is harder to match an order than an arrangement.
  // The second child might be large and thus more expensive to
  // sort then the first child, so we might want to try it both ways.
  //   Note that there is no fundamental requirement that both sides
  // of a union must be partitioned the same way to execute them in
  // parallel. But, if we were to allow the two children of a union
  // to exhibit different partitioning functions, what would the
  // partitioning function for the union be? Until we solve this
  // problem, we must insist that both sides of the union always
  // exhibit the same partitioning function.
  // ---------------------------------------------------------------------

  Context* result = NULL;
  Lng32 planNumber = 0;
  Context* childContext = NULL;
  const ReqdPhysicalProperty* rppForMe =
          myContext->getReqdPhysicalProperty();
  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();
  const ReqdPhysicalProperty* rppForChild = NULL;
  Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
  float childNumPartsAllowedDeviation = 0.0;
  NABoolean numOfESPsForced = FALSE;

  SortOrderTypeEnum childSortOrderTypeReq = NO_SOT;
  PartitioningRequirement* dp2SortOrderPartReq = NULL;

  // ---------------------------------------------------------------------
  // Compute the number of child plans to consider.
  // ---------------------------------------------------------------------
  Lng32 childPlansToConsider = 4;
  NABoolean mustTryBothChildrenFirst = FALSE;

  // If there is a required arrangement of more than one column,
  // we need to generate two different plans where we alternate
  // which child to try first, in order to guarantee that we only
  // sort if absolutely necessary and if we do sort the smallest
  // child. The crux of the matter is that it is easier to satisfy
  // an arrangement than a sort order, and whoever goes first only
  // has to satisfy that arrangement, not a sort
  // order.
  if ((rppForMe->getArrangedCols() != NULL) AND
      (rppForMe->getArrangedCols()->entries() > 1))
    mustTryBothChildrenFirst = TRUE;

  // If we don't need to try two plans (four child plans) for sort
  // purposes and we don't need to try two for parallel purposes,
  // then indicate we will only try one plan (two child plans).
  if (NOT mustTryBothChildrenFirst AND
      ((rppForMe->getCountOfPipelines() == 1) OR
       ((partReqForMe != NULL) AND
        (partReqForMe->isRequirementExactlyOne() OR
         partReqForMe->isRequirementReplicateNoBroadcast()))
      )
     )
    childPlansToConsider = 2;

  // ---------------------------------------------------------------------
  // The creation of the next Context for a child depends upon the
  // the number of child Contexts that have been created in earlier
  // invocations of this method.
  // ---------------------------------------------------------------------
  while ((pws->getCountOfChildContexts() < childPlansToConsider) AND
         (rppForChild == NULL))
  {
    // If we stay in this loop because we didn't generate some
    // child contexts, we need to reset the child plan count when
    // it gets to be as large the arity, because otherwise we
    // would advance the child plan count past the arity.
    if (pws->getPlanChildCount() >= getArity())
      pws->resetPlanChildCount();

    // -------------------------------------------------------------------
    // Create the 1st Context for left child:
    // -------------------------------------------------------------------
    if (pws->getCountOfChildContexts() == 0)
    {
      childIndex = 0;
      planNumber = 0;

      RequirementGenerator rg(child(0),rppForMe);

      // If there was a required order and/or arrangement, then we
      // need to remove it, and replace it with a required order
      // and/or arrangement that is expressed in terms of the child's
      // ValueIds. This is because the requirement will be in terms
      // of ValueIdUnions for the Union.
      if (myContext->requiresOrder())
      {
        rg.removeSortKey();
        rg.removeArrangement();
        // Sort Order type was removed by the calls above, so
        // get it again from the rpp
        childSortOrderTypeReq = rppForMe->getSortOrderTypeReq();
        // Convert any DP2_OR_ESP_NO_SORT requirement to ESP_NO_SORT,
        // to avoid the possibility of one union child choosing a
        // DP2 sort order type and the other union child choosing a
        // ESP_NO_SORT sort order type, which are incompatible for union.
        if (childSortOrderTypeReq == DP2_OR_ESP_NO_SORT_SOT)
          childSortOrderTypeReq = ESP_NO_SORT_SOT;
        else if (rppForMe->getDp2SortOrderPartReq() != NULL)
        {
          // Need to map any dp2SortOrderPartReq
          NABoolean mapItUp = FALSE;
          dp2SortOrderPartReq = rppForMe->getDp2SortOrderPartReq()->
                                  copyAndRemap(getMap(childIndex),
                                               mapItUp);
        }
      }
      if (rppForMe->getArrangedCols() != NULL)
      {
        ValueIdSet newArrangedCols;
        getMap(childIndex).rewriteValueIdSetDown(*rppForMe->getArrangedCols(),
                                                 newArrangedCols);
        rg.addArrangement(newArrangedCols,
                          childSortOrderTypeReq,
                          dp2SortOrderPartReq);
      }
      if (rppForMe->getSortKey() != NULL)
      {
        ValueIdList newSortKey;
        getMap(childIndex).rewriteValueIdListDown(*rppForMe->getSortKey(),
                                                  newSortKey);
        rg.addSortKey(newSortKey,
                      childSortOrderTypeReq,
                      dp2SortOrderPartReq);
      }

      // If there is a required partitioning function, then we need
      // to remove it, and replace it with a partitioning function
      // that is expressed in terms of the child's ValueIds.
      if (rppForMe->requiresPartitioning())
      {
        // remove the original requirement
        rg.removeAllPartitioningRequirements();
        // Rewrite parent req. part func part key in terms of the
        // values that appear below this Union.
        NABoolean mapItUp = FALSE;
        PartitioningRequirement* parentPartReq =
          rppForMe->getPartitioningRequirement();
        PartitioningRequirement* parentPartReqRewritten =
          parentPartReq->copyAndRemap(getMap(childIndex),mapItUp);

        // add in the new, rewritten partitioning requirement
        rg.addPartRequirement(parentPartReqRewritten);
      }

      // --------------------------------------------------------------------
      // If this is a CPU or memory-intensive operator then add a
      // requirement for a maximum number of partitions, unless
      // that requirement would conflict with our parent's requirement.
      //
      // --------------------------------------------------------------------
      if (okToAttemptESPParallelism(myContext,
                                    pws,
                                    childNumPartsRequirement,
                                    childNumPartsAllowedDeviation,
                                    numOfESPsForced))
      {
        if (NOT numOfESPsForced)
          rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                    &childNumPartsAllowedDeviation);
        rg.addNumOfPartitions(childNumPartsRequirement,
                              childNumPartsAllowedDeviation);
      } // end if ok to try parallelism

      // Produce the requirements and make sure they are feasible.
      if (rg.checkFeasibility())
      {
        // Produce the required physical properties.
        rppForChild = rg.produceRequirement();
      }

    } // end if 1st child context

    // -------------------------------------------------------------------
    // Create 1st Context for right child:
    // Any required order matches the order that is produced by the
    // optimal solution for my left child.
    //
    // -------------------------------------------------------------------
    else if (pws->getCountOfChildContexts() == 1)
    {
      childIndex = 1;
      planNumber = 0;

      // ---------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // ---------------------------------------------------------------
      if (NOT pws->isLatestContextWithinCostLimit())
        pws->eraseLatestContextFromWorkSpace();

      childContext = pws->getChildContext(0,0);

      // ---------------------------------------------------------------
      // If the Context that was created for the left child has an
      // optimal solution whose cost is within the specified cost
      // limit, create the corresponding Context for the right child.
      // ---------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty*
              sppForChild = childContext->getPhysicalPropertyForSolution();
        // ---------------------------------------------------------------
        // spp should have been synthesized for child's optimal plan.
        // ---------------------------------------------------------------
        CMPASSERT(sppForChild != NULL);
        PartitioningFunction* leftPartFunc =
                                sppForChild->getPartitioningFunction();
        // Take the synthesized partitioning function from the left
        // child, translate it to be in terms of the ValueIdUnions of
        // the union, then translate this to be in terms of the valueids
        // of the right child.
        NABoolean mapItUp = TRUE;
        PartitioningFunction* unionPartFunc =
          leftPartFunc->copyAndRemap(getMap(0),mapItUp);
        mapItUp = FALSE;
        PartitioningFunction* rightPartFunc =
          unionPartFunc->copyAndRemap(getMap(1),mapItUp);
        // Now transform this partitioning function into a partitioning
        // requirement.
        PartitioningRequirement* partReqForRight = 
               rightPartFunc->makePartitioningRequirement();

        // Try a new requirement (require right child of the UNION to be 
        // hash2 partitioned with the same number of partitions as the 
        // left child) when the partitioning key is not covered by the 
        // characteristic output AND if both children are hash2 partitioned
        // with the same number of partitions. Please note that, in this 
        // scenario, a parallel UNION plan is considered only for hash2 
        // partitioned children and NOT for hash (hash1) and range
        // partitioned children.
        if ((CURRSTMT_OPTDEFAULTS->isSideTreeInsert() == FALSE) &&
            CmpCommon::getDefault(COMP_BOOL_22) == DF_ON   &&
            leftPartFunc->castToHash2PartitioningFunction() &&
            !(child(0).getGroupAttr()->getCharacteristicOutputs().contains(
                                 leftPartFunc->getPartialPartitioningKey()))) 
        {
          // Get the number of partitions from the left child.
          Lng32 numOfPartitions = leftPartFunc->getCountOfPartitions();
          // Create a new requirement for the right child and require a 
          // hash2 partitioning only (TRUE).
          partReqForRight =
            new (CmpCommon::statementHeap()) 
              RequireApproximatelyNPartitions(0.0, numOfPartitions, TRUE);
        } 

        RequirementGenerator rg (child(1),rppForMe);

        // Remove any parent partitioning requirements, since we have
        // already enforced this on the left child.
        rg.removeAllPartitioningRequirements();

        if ( getUnionForIF() && sppForChild->getPushDownProperty() )
        {
        // Add the push-down requirement based on left child's push
        // down property.
        //
        // There is no need to enforce sorting order/partitioning etc.
        // requirements.
             rg.addPushDownRequirement(
                sppForChild->getPushDownProperty()->makeRequirement());

        } else {

           // Now, add in the Union's partitioning requirement for the
           // right child.
           rg.addPartRequirement(partReqForRight);

           // If there is a required order and/or arrangement, then they
           // must be removed and replaced with requirements that are
           // in terms of the child's valueids.
           if (myContext->requiresOrder())
           {
             rg.removeSortKey();
             rg.removeArrangement();
             // Sort Order type was removed by the calls above, so
             // get it again from the rpp
             childSortOrderTypeReq = rppForMe->getSortOrderTypeReq();
             // Convert any DP2_OR_ESP_NO_SORT requirement to ESP_NO_SORT,
             // to avoid the possibility of one union child choosing a
             // DP2 sort order type and the other union child choosing a
             // ESP_NO_SORT sort order type, which are incompatible for union.
             if (childSortOrderTypeReq == DP2_OR_ESP_NO_SORT_SOT)
               childSortOrderTypeReq = ESP_NO_SORT_SOT;
             else if (rppForMe->getDp2SortOrderPartReq() != NULL)
             {
               // Need to map any dp2SortOrderPartReq
               NABoolean mapItUp = FALSE;
               dp2SortOrderPartReq = rppForMe->getDp2SortOrderPartReq()->
                                       copyAndRemap(getMap(childIndex),
                                                    mapItUp);
             }
           }
           // If there was a required arrangement, then the left child
           // chose a particular order. We need to take the sort key
           // from the left child and translate it into the corresponding
           // valueIds for the right child. We then require this order
           // of the right child.
           if (rppForMe->getArrangedCols() != NULL)
           {
             // First, we need to make sure the sort key from the left child
             // contains all the expressions from the required arrangement
             // and any sort key it was given, and only those expressions.
             // (Taking inverse nodes into account, of course).
             ValueIdSet reqArrangementForChild;
             getMap(0).rewriteValueIdSetDown(*rppForMe->getArrangedCols(),
                                             reqArrangementForChild);
             ValueIdList leftSortKey;
             // Is there also a required sort key?
             if (rppForMe->getSortKey() != NULL)
             {
               // Initialize the left child sort key to the required sort key cols.
               getMap(0).rewriteValueIdListDown(*rppForMe->getSortKey(),
                                                leftSortKey);
             }
             synthUnionSortKeyFromChild(sppForChild->getSortKey(),
                                       reqArrangementForChild,
                                       leftSortKey);

             ValueIdList unionSortKey;
             // map left child sort key cols to their output ValueIdUnion
             // equivalents
             getMap(0).rewriteValueIdListUp(unionSortKey,
                                            leftSortKey);

          // Temporary fix - if the sorting order is different than the arrangement
	  // force the sort above Union. See comment in finalizeUnionSortKey

	     if (finalizeUnionSortKey(leftSortKey, 0, *rppForMe->getArrangedCols(),
	                                         unionSortKey) == FALSE)
	       return NULL;;

             if (unionSortKey.entries() > 0) // is usually true
             {
               ValueIdList rightSortKey;

	     if (alternateRightChildOrderExpr().isEmpty())
	     {
               // map output ValueIdUnions to their equivalent right
               // child ValueIds
               getMap(childIndex).rewriteValueIdListDown(unionSortKey,
                                                         rightSortKey);
	     }
	     else
	     {
	       //MV specific optimization refer to MVRefreshBuilder.cpp
	       // MultiTxnMavBuilder::buildUnionBetweenRangeAndIudBlocks()
               getMap(childIndex).rewriteValueIdListDown(alternateRightChildOrderExpr(),
                                                         rightSortKey);

	     }

               // Add the sort key from the left child, now translated
               // to be in terms of the right child valueid's, as the
               // required sort order for the right child.
               rg.addSortKey(rightSortKey,
                             childSortOrderTypeReq,
                             dp2SortOrderPartReq);
             }

           }
           // If there was only a required order then the left child must
           // have picked this order, so we don't need to look
           // at the sort key for the left child. Just translate
           // the required order into the valueIds for the right child.
           else if (rppForMe->getSortKey() != NULL)
           {
	     ValueIdList newSortKey;
	     //++MV
	     if (alternateRightChildOrderExpr().isEmpty())
	     {
	       getMap(childIndex).rewriteValueIdListDown(*rppForMe->getSortKey(),
							 newSortKey);
	     }
	     else
	     {
	       //MV specific optimization refer to MVRefreshBuilder.cpp
	       // MultiTxnMavBuilder::buildUnionBetweenRangeAndIudBlocks()
	       getMap(childIndex).rewriteValueIdListDown(alternateRightChildOrderExpr(),
							 newSortKey);
	     }
	     rg.addSortKey(newSortKey,
		   childSortOrderTypeReq,
		   dp2SortOrderPartReq);
           }
        }

        // Produce the requirements and make sure they are feasible.
        if (rg.checkFeasibility())
        {
          // Produce the required physical properties.
          rppForChild = rg.produceRequirement();
        }

      } // endif previous Context has an optimal solution

    } // end if 2nd child context

    // -------------------------------------------------------------------
    // Create 2nd Context for the right child
    // -------------------------------------------------------------------
    else if (pws->getCountOfChildContexts() == 2)
    {
      childIndex = 1;
      planNumber = 1;

      RequirementGenerator rg(child(1),rppForMe);

      // If there was a required order and/or arrangement, then we
      // need to remove it, and replace it with a required order
      // and/or arrangement that is expressed in terms of the child's
      // ValueIds. This is because the requirement will be in terms
      // of ValueIdUnions for the Union.
      if (myContext->requiresOrder())
      {
        rg.removeSortKey();
        rg.removeArrangement();
        // Sort Order type was removed by the calls above, so
        // get it again from the rpp
        childSortOrderTypeReq = rppForMe->getSortOrderTypeReq();
        // Convert any DP2_OR_ESP_NO_SORT requirement to ESP_NO_SORT,
        // to avoid the possibility of one union child choosing a
        // DP2 sort order type and the other union child choosing a
        // ESP_NO_SORT sort order type, which are incompatible for union.
        if (childSortOrderTypeReq == DP2_OR_ESP_NO_SORT_SOT)
          childSortOrderTypeReq = ESP_NO_SORT_SOT;
        else if (rppForMe->getDp2SortOrderPartReq() != NULL)
        {
          // Need to map any dp2SortOrderPartReq
          NABoolean mapItUp = FALSE;
          dp2SortOrderPartReq = rppForMe->getDp2SortOrderPartReq()->
                                  copyAndRemap(getMap(childIndex),
                                               mapItUp);
        }
      }
      if (rppForMe->getArrangedCols() != NULL)
      {
        ValueIdSet newArrangedCols;
        getMap(childIndex).rewriteValueIdSetDown(*rppForMe->getArrangedCols(),
                                                 newArrangedCols);
        rg.addArrangement(newArrangedCols,
                          childSortOrderTypeReq,
                          dp2SortOrderPartReq);
      }
      if (rppForMe->getSortKey() != NULL)
      {
        ValueIdList newSortKey;
        getMap(childIndex).rewriteValueIdListDown(*rppForMe->getSortKey(),
                                                  newSortKey);
        rg.addSortKey(newSortKey,
                      childSortOrderTypeReq,
                      dp2SortOrderPartReq);
      }

      // If there is a required partitioning function, then we need
      // to remove it, and replace it with a partitioning function
      // that is expressed in terms of the child's ValueIds.
      if (rppForMe->requiresPartitioning())
      {
        // remove the original requirement
        rg.removeAllPartitioningRequirements();
        // Rewrite parent req. part func part key in terms of the
        // values that appear below this Union.
        NABoolean mapItUp = FALSE;
        PartitioningRequirement* parentPartReq =
          rppForMe->getPartitioningRequirement();
        PartitioningRequirement* parentPartReqRewritten =
          parentPartReq->copyAndRemap(getMap(childIndex),mapItUp);

        // add in the new, rewritten partitioning requirement
        rg.addPartRequirement(parentPartReqRewritten);
      }

      // --------------------------------------------------------------------
      // If this is a CPU or memory-intensive operator then add a
      // requirement for a maximum number of partitions, unless
      // that requirement would conflict with our parent's requirement.
      //
      // --------------------------------------------------------------------
      if (okToAttemptESPParallelism(myContext,
                                    pws,
                                    childNumPartsRequirement,
                                    childNumPartsAllowedDeviation,
                                    numOfESPsForced))
      {
        if (NOT numOfESPsForced)
          rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                    &childNumPartsAllowedDeviation);
        rg.addNumOfPartitions(childNumPartsRequirement,
                              childNumPartsAllowedDeviation);
      } // end if ok to try parallelism

      // Produce the requirements and make sure they are feasible.
      if (rg.checkFeasibility())
      {
        // Produce the required physical properties.
        rppForChild = rg.produceRequirement();
      }

    } // end if 3rd child context

    // -------------------------------------------------------------------
    // Create 2nd Context for the left child:
    // Force the left child to have the same order as the
    // right child.  If there is a required order in my
    // myContext, ensure that it is satisfied.
    // -------------------------------------------------------------------
    else if (pws->getCountOfChildContexts() == 3)
    {
      childIndex = 0;
      planNumber = 1;

      // ---------------------------------------------------------------
      // Cost limit exceeded? Erase the Context from the workspace.
      // Erasing the Context does not decrement the count of Contexts
      // considered so far.
      // ---------------------------------------------------------------
      if (NOT pws->isLatestContextWithinCostLimit())
        pws->eraseLatestContextFromWorkSpace();

      childContext = pws->getChildContext(1,1);

      // ---------------------------------------------------------------
      // If the Context that was created for the right child has an
      // optimal solution whose cost is within the specified cost
      // limit, create the corresponding Context for the left child.
      // ---------------------------------------------------------------
      if((childContext != NULL) AND childContext->hasOptimalSolution())
      {
        const PhysicalProperty*
              sppForChild = childContext->getPhysicalPropertyForSolution();
        // ---------------------------------------------------------------
        // spp should have been synthesized for child's optimal plan.
        // ---------------------------------------------------------------
        CMPASSERT(sppForChild != NULL);
        PartitioningFunction* rightPartFunc =
                                sppForChild->getPartitioningFunction();
        // Take the synthesized partitioning function from the right
        // child, translate it to be in terms of the ValueIdUnions of
        // the union, then translate this to be in terms of the valueids
        // of the left child.
        NABoolean mapItUp = TRUE;
        PartitioningFunction* unionPartFunc =
          rightPartFunc->copyAndRemap(getMap(1),mapItUp);
        mapItUp = FALSE;
        PartitioningFunction* leftPartFunc =
          unionPartFunc->copyAndRemap(getMap(0),mapItUp);
        // Now translate the partitioning function into a requirement
        PartitioningRequirement* partReqForLeft =
          leftPartFunc->makePartitioningRequirement();

        // Try a new requirement (require left child of the UNION to be 
        // hash2 partitioned with the same number of partitions as the 
        // right child) when the partitioning key is not covered by the 
        // characteristic output AND if both children are hash2 partitioned
        // with the same number of partitions. Please note that, in this 
        // scenario, a parallel UNION plan is considered only for hash2 
        // partitioned children and NOT for hash (hash1) and range
        // partitioned children.
        if ((CURRSTMT_OPTDEFAULTS->isSideTreeInsert() == FALSE) &&
            CmpCommon::getDefault(COMP_BOOL_22) == DF_ON     &&
            rightPartFunc->castToHash2PartitioningFunction() &&
            !(child(1).getGroupAttr()->getCharacteristicOutputs().contains(
                          rightPartFunc->getPartialPartitioningKey())))
        {
          // Get the number of partitions from the right child.
          Lng32 numOfPartitions = rightPartFunc->getCountOfPartitions();
          // Create a new requirement for the left child and require a
          // hash2 partitioning only (TRUE).
          partReqForLeft =
            new (CmpCommon::statementHeap()) 
              RequireApproximatelyNPartitions(0.0, numOfPartitions, TRUE);
        }

        RequirementGenerator rg (child(0),rppForMe);

        // We have already applied the parent partitioning requirements
        // to the right child, so no need to apply them to the left child.
        rg.removeAllPartitioningRequirements();

        // Now, add in the Union's partitioning requirement for the
        // left child.
        rg.addPartRequirement(partReqForLeft);

        // If there is a required order and/or arrangement, then they
        // must be removed and replaced with requirements that are
        // in terms of the child's valueids.
        if (myContext->requiresOrder())
        {
          rg.removeSortKey();
          rg.removeArrangement();
          // Sort Order type was removed by the calls above, so
          // get it again from the rpp
          childSortOrderTypeReq = rppForMe->getSortOrderTypeReq();
          // Convert any DP2_OR_ESP_NO_SORT requirement to ESP_NO_SORT,
          // to avoid the possibility of one union child choosing a
          // DP2 sort order type and the other union child choosing a
          // ESP_NO_SORT sort order type, which are incompatible for union.
          if (childSortOrderTypeReq == DP2_OR_ESP_NO_SORT_SOT)
            childSortOrderTypeReq = ESP_NO_SORT_SOT;
          else if (rppForMe->getDp2SortOrderPartReq() != NULL)
          {
            // Need to map any dp2SortOrderPartReq
            NABoolean mapItUp = FALSE;
            dp2SortOrderPartReq = rppForMe->getDp2SortOrderPartReq()->
                                    copyAndRemap(getMap(childIndex),
                                                 mapItUp);
          }
        }
        // If there was a required arrangement, then the right child
        // chose a particular order. We need to take the sort key
        // from the right child and translate it into the corresponding
        // valueIds for the left child. We then require this order
        // of the left child.
        if (rppForMe->getArrangedCols() != NULL)
        {
          // First, we need to make sure the sort key from the right child
          // contains all the expressions from the required arrangement
          // and any sort key it was given, and only those expressions.
          // (Taking inverse nodes into account, of course).
          ValueIdSet reqArrangementForChild;
          getMap(1).rewriteValueIdSetDown(*rppForMe->getArrangedCols(),
                                          reqArrangementForChild);
          ValueIdList rightSortKey;
          // Is there also a required sort key?
          if (rppForMe->getSortKey() != NULL)
          {
            // Initialize the right child sort key to the required sort key cols.
            getMap(1).rewriteValueIdListDown(*rppForMe->getSortKey(),
                                             rightSortKey);
          }
          synthUnionSortKeyFromChild(sppForChild->getSortKey(),
                                    reqArrangementForChild,
                                    rightSortKey);

          ValueIdList unionSortKey;
          // map right child sort key cols to their output ValueIdUnion
          // equivalents
          getMap(1).rewriteValueIdListUp(unionSortKey,
                                        rightSortKey);

          // Temporary fix - if the sorting order is different than the arrangement
	  // force the sort above Union - See comment in finalizeUnionSortKey
	  if (finalizeUnionSortKey(rightSortKey, 1, *rppForMe->getArrangedCols(),
	                                             unionSortKey) == FALSE)
	    return NULL;

          if (unionSortKey.entries() > 0) // is usually true
          {
            ValueIdList leftSortKey;
            // map output ValueIdUnions to their equivalent left
            // child ValueIds
            getMap(childIndex).rewriteValueIdListDown(unionSortKey,
                                                      leftSortKey);

            // Add the sort key from the right child, now translated
            // to be in terms of the left child valueid's, as the
            // required sort order for the left child.
            rg.addSortKey(leftSortKey,
                          childSortOrderTypeReq,
                          dp2SortOrderPartReq);
          }

        }
        // If there was only a required order then the right child must
        // have picked this order, so we don't need to look
        // at the sort key for the right child. Just translate
        // the required order into the valueIds for the left child.
        else if (rppForMe->getSortKey() != NULL)
        {
	   ValueIdList newSortKey;
	   //++MV
	   if (alternateRightChildOrderExpr().isEmpty())
	   {
	     getMap(childIndex).rewriteValueIdListDown(*rppForMe->getSortKey(),
						       newSortKey);
	   }
	   else
	   {
	     // MV specific optimization refer to MVRefreshBuilder.cpp
	     // MultiTxnMavBuilder::buildUnionBetweenRangeAndIudBlocks()
	     getMap(childIndex).rewriteValueIdListDown(alternateRightChildOrderExpr(),
						       newSortKey);
	   }
	   rg.addSortKey(newSortKey,
		 childSortOrderTypeReq,
		 dp2SortOrderPartReq);
        }

        // Produce the requirements and make sure they are feasible.
        if (rg.checkFeasibility())
        {
          // Produce the required physical properties.
          rppForChild = rg.produceRequirement();
        }

      } // endif previous Context has an optimal solution

    } // end if 4th child context

    // -------------------------------------------------------------------
    // Create a Context using the partitioning requirement that was
    // generated for the current plan.
    // -------------------------------------------------------------------
    if (rppForChild != NULL)
    {

      // ---------------------------------------------------------------
      // Compute the cost limit to be applied to the child.
      // ---------------------------------------------------------------
      CostLimit* costLimit = computeCostLimit(myContext,pws);

      // ---------------------------------------------------------------
      // Get a Context for optimizing the child.
      // Search for an existing Context in the CascadesGroup to which
      // the child belongs that requires the same properties as those
      // in rppForChild. Reuse it, if found. Otherwise, create a new
      // Context that contains rppForChild as the required physical
      // properties.
      // ----------------------------------------------------------------
      result = shareContext(childIndex, rppForChild,
                            myContext->getInputPhysicalProperty(),
                            costLimit,
                            myContext, myContext->getInputLogProp());
    }
    else
      result = NULL;

    // -------------------------------------------------------------------
    // Remember the cases for which a Context could not be generated,
    // or store the context that was generated.
    // -------------------------------------------------------------------
    pws->storeChildContext(childIndex, planNumber, result);
    pws->incPlanChildCount();

  }  // end while loop

  return result;

} // MergeUnion::createContextForAChild()

//<pb>
NABoolean MergeUnion::findOptimalSolution(Context* myContext,
                                          PlanWorkSpace* pws)
{
  // Plan # is only an output param, initialize it to an impossible value.
  Lng32 planNumber = -1;
  NABoolean hasOptSol = pws->findOptimalSolution(planNumber);

  if (hasOptSol)
  {
    // Set the sort order in the union node from the sort key
    // in the synthesized physical properties. The union sort key
    // is also set when the union physical properties are synthesized,
    // but this is only valid if spp is synthesized AFTER the
    // optimal plan is chosen. We must make sure that the union
    // sort key reflects the sort key of the union plan that was
    // actually chosen, not just the last one to have it's
    // properties synthesized.
    CascadesPlan* myOptimalPlan = myContext->getPlan();
    PhysicalProperty* myOptimalSPP = myOptimalPlan->getPhysicalProperty();

    // Make sure SPP is not null. Since we always synthesize properties
    // early now so we can use them during costing, this should always
    // be true.
    if (myOptimalSPP != NULL)
      setSortOrder(myOptimalSPP->getSortKey());
  }

  return hasOptSol;

} // MergeUnion::findOptimalSolution()

//<pb>
//==============================================================================
//  Synthesize physical properties for merge union operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used for
//                 synthesizing partitioning functions.
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
MergeUnion::synthPhysicalProperty(const Context* myContext,
                                  const Lng32     planNumber,
                                  PlanWorkSpace  *pws)
{
  const ReqdPhysicalProperty* rppForMe =
                      myContext->getReqdPhysicalProperty();
  const PhysicalProperty* const sppOfLeftChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(0);
  const PhysicalProperty* const sppOfRightChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(1);
  SortOrderTypeEnum unionSortOrderType = NO_SOT;
  PartitioningFunction* unionDp2SortOrderPartFunc = NULL;

  // ----------------------------------------------------------------
  // Merge Unions have two potential plans--see member
  // function MergeUnion::createContextForAChild().  In plan 0, the
  // left child is first; in plan 1, the right child is first.
  // We utilize this information when synthesizing the union sort
  // key if there was a required arrangement. We also utilize this
  // information when synthesizing the union partitioning function.
  // ----------------------------------------------------------------
  CMPASSERT(planNumber == 0 || planNumber == 1);

  if (rppForMe->getArrangedCols() != NULL)
  {
    ValueIdList unionSortKey;
    // We must synthesize the union sort key from the child that was
    // optimized first. This is because sometimes, the required
    // arrangement expressions are equivalent to each other on
    // one side of the union, but not the other side. Any duplicate
    // arrangement expressions are only reflected in the child
    // sort key once, so when we map the child sort key columns
    // back to the union sort key the order of the duplicate
    // arrangement expressions in the union sort key is arbitrary,
    // so long as they do not appear before the corresponding
    // child sort key entry. If we optimized the child with the
    // duplicate arrangement expressions first, then the order
    // that this child chooses will be the order of the other
    // child, and thus the union. But if we optimized the child
    // with no duplicate arrangement expressions first, then this
    // child may have chosen a quite different order for these
    // arrangement expressions.  Thus the union sort key is
    // based on the child sort key of the child that was
    // optimized first.
    if (planNumber == 0)
    {
      // Get the union sort key from the left child's sort key.
      ValueIdSet reqArrangementForChild;
      getMap(0).rewriteValueIdSetDown(*rppForMe->getArrangedCols(),
                                      reqArrangementForChild);
      ValueIdList leftSortKey;
      // Is there also a required sort key?
      if (rppForMe->getSortKey() != NULL)
      {
        // Initialize the left child sort key to the required sort key cols.
        getMap(0).rewriteValueIdListDown(*rppForMe->getSortKey(),
                                         leftSortKey);
      }
      synthUnionSortKeyFromChild(sppOfLeftChild->getSortKey(),
                                 reqArrangementForChild,
                                 leftSortKey);

      // map left child sort key cols to their output ValueIdUnion equivalents
      getMap(0).rewriteValueIdListUp(unionSortKey,
                                     leftSortKey);
    }
    else
    {
      // Get the union sort key from the right child's sort key.
      ValueIdSet reqArrangementForChild;
      getMap(1).rewriteValueIdSetDown(*rppForMe->getArrangedCols(),
                                      reqArrangementForChild);
      ValueIdList rightSortKey;
      // Is there also a required sort key?
      if (rppForMe->getSortKey() != NULL)
      {
        // Initialize the right child sort key to the required sort key cols.
        getMap(1).rewriteValueIdListDown(*rppForMe->getSortKey(),
                                         rightSortKey);
      }
      synthUnionSortKeyFromChild(sppOfRightChild->getSortKey(),
                                 reqArrangementForChild,
                                 rightSortKey);

      // map right child sort key cols to their output ValueIdUnion equivalents
      getMap(1).rewriteValueIdListUp(unionSortKey,
                                    rightSortKey);
    }
    // remember the child sort order in the union node
    setSortOrder(unionSortKey);
  } // end if there was a required arrangement
  else if (rppForMe->getSortKey() != NULL)
  {
    // If there wasn't a required arrangement but there was a required
    // order, then remember the required order in the union node.
    setSortOrder(*rppForMe->getSortKey());
  }

  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(myContext, -1, pws);

  PartitioningFunction* myPartFunc;
  PartitioningFunction* leftPartFunc;
  PartitioningFunction* rightPartFunc;
  const SearchKey* myPartSearchKey;

  // if any child has random partitioning then union has random partitioning
  leftPartFunc = sppOfLeftChild->getPartitioningFunction();
  rightPartFunc = sppOfRightChild->getPartitioningFunction();
  if (leftPartFunc && leftPartFunc->isARandomPartitioningFunction())
  {
    myPartFunc = leftPartFunc;
    myPartSearchKey = sppOfLeftChild->getPartSearchKey();
    unionSortOrderType = NO_SOT;
    unionDp2SortOrderPartFunc = NULL;
  }
  else if (rightPartFunc && rightPartFunc->isARandomPartitioningFunction())
  {
    myPartFunc = rightPartFunc;
    myPartSearchKey = sppOfRightChild->getPartSearchKey();
    unionSortOrderType = NO_SOT;
    unionDp2SortOrderPartFunc = NULL;
  }
  else // neither child uses random partitioning.
  {
    CMPASSERT(leftPartFunc != NULL AND rightPartFunc != NULL);

    // check if union children have the same partitioning

    // Rewrite partitioning key in terms of values that are above Union
    NABoolean mapItUp = TRUE;
    leftPartFunc = leftPartFunc->copyAndRemap(getMap(0), mapItUp);
    rightPartFunc = rightPartFunc->copyAndRemap(getMap(1), mapItUp);


    const PushDownRequirement* pdreq = rppForMe->getPushDownRequirement();

    NABoolean comparable = ( leftPartFunc->
                   comparePartFuncToFunc(*rightPartFunc) == SAME )
                             OR
                      (pdreq && pdreq ->castToPushDownCSRequirement());

    // are union children partitioning same?
    if (comparable)
    {
      // yes, children have same partitioning or 
      // it is a special case for pushing down CS,
      // reflect it as union's partitition scheme.
      if (planNumber == 0)
      {
        myPartFunc = leftPartFunc;
        myPartSearchKey = sppOfLeftChild->getPartSearchKey();
      }
      else
      {
        myPartFunc = rightPartFunc;
        myPartSearchKey = sppOfRightChild->getPartSearchKey();
      }
    }
    else // union children have incompatible partitioning
    {
      // union partitioning is random and has: 
      // no sort, no arrangement, no partitioning key predicates, nothing
      ValueIdSet partKey;
      ItemExpr *randNum = 
        new(CmpCommon::statementHeap()) RandomNum(NULL, TRUE);
      randNum->synthTypeAndValueId();
      partKey.insert(randNum->getValueId());
      myPartFunc = new(CmpCommon::statementHeap()) Hash2PartitioningFunction
        (partKey, partKey, leftPartFunc->getCountOfPartitions(),
         const_cast<NodeMap*>(leftPartFunc->getNodeMap()));

      myPartFunc->createPartitioningKeyPredicates();
     
      myPartSearchKey = NULL;
      unionSortOrderType = NO_SOT;
      unionDp2SortOrderPartFunc = NULL;
    }
  }

  if (NOT sortOrder_.isEmpty())
  {
    // It's possible one or both children did not synthesize a sort
    // key at all, if all sort key columns were equal to constants.
    // If both did not synthesize a sort key, then arbitrarily set the
    // sort order type to ESP_NO_SORT. If only one child did not synthesize
    // a sort key, then synthesize the sort order type from the other child.

    NABoolean leftChildNotOrdered =
        sppOfLeftChild->getSortOrderType() == NO_SOT OR
        NOT myContext->getPlan()->getContextForChild(0)->requiresOrder();
    NABoolean rightChildNotOrdered =
        sppOfRightChild->getSortOrderType() == NO_SOT OR
        NOT myContext->getPlan()->getContextForChild(1)->requiresOrder();

    if ( leftChildNotOrdered AND rightChildNotOrdered )
      unionSortOrderType = ESP_NO_SORT_SOT;
    else if ( leftChildNotOrdered )
      unionSortOrderType = sppOfRightChild->getSortOrderType();
    else if ( rightChildNotOrdered )
      unionSortOrderType = sppOfLeftChild->getSortOrderType();
    else if (sppOfLeftChild->getSortOrderType() !=
             sppOfRightChild->getSortOrderType())
    {
      // The sort order types of the children are not the same,
      // so we must choose a sort order type for the union.
      // This should only happen if the required sort order type was ESP.
      // Set the sort order type to ESP_VIA_SORT in this case since
      // we did do a sort in producing at least part of the sort key.
      CMPASSERT(rppForMe->getSortOrderTypeReq() == ESP_SOT);
      // Shouldn't be any synthesized dp2SortOrderPartFunc in this case.
      CMPASSERT(sppOfLeftChild->getDp2SortOrderPartFunc() == NULL);
      // Only synthesized sort order types that satisfy an ESP sort
      // order type requirement are ESP_NO_SORT and ESP_VIA_SORT.
      CMPASSERT(((sppOfLeftChild->getSortOrderType() == ESP_NO_SORT_SOT) OR
                 (sppOfLeftChild->getSortOrderType() == ESP_VIA_SORT_SOT)) AND
                ((sppOfRightChild->getSortOrderType() == ESP_NO_SORT_SOT) OR
                 (sppOfRightChild->getSortOrderType() == ESP_VIA_SORT_SOT)));
      // Finally, set the sort order type
      unionSortOrderType = ESP_VIA_SORT_SOT;
    } // end if sort order types of the children were not the same
    else
    {
      // Sort order types of the children are the same, so
      // arbitrarily get it from the left child.
      unionSortOrderType = sppOfLeftChild->getSortOrderType();
      if (sppOfLeftChild->getDp2SortOrderPartFunc() != NULL)
      {
        NABoolean mapItUp = TRUE;
        unionDp2SortOrderPartFunc =
          sppOfLeftChild->getDp2SortOrderPartFunc()->copyAndRemap(getMap(0),
                                                                  mapItUp);
      }
    }
  } // end if sort order is not empty

  //++MV
  // MV specific optimization refer to MVRefreshBuilder.cpp
  // MultiTxnMavBuilder::buildUnionBetweenRangeAndIudBlocks()

  ValueIdList unionSortKey;
  if (alternateRightChildOrderExpr().isEmpty())
  {
    unionSortKey = sortOrder_;
  }
  else
  {
    // Because we replaced the order of the right child with the
    // alternate order we now need to pretend that the union
    // is doing the requested order which we can get from the left child

    // map left child sort key cols to their output ValueIdUnion equivalents
    getMap(0).rewriteValueIdListUp(unionSortKey,
                                   sppOfLeftChild->getSortKey());
  }

  PhysicalProperty* sppForMe =
      new (CmpCommon::statementHeap()) PhysicalProperty(
	   unionSortKey,
	   unionSortOrderType,
	   unionDp2SortOrderPartFunc,
	   myPartFunc,
	   sppOfLeftChild->getPlanExecutionLocation(), //||opt
				// synth from both child props
	   combineDataSources(sppOfLeftChild->getDataSourceEnum(),
			      sppOfRightChild->getDataSourceEnum()),
	   sppOfLeftChild->getIndexDesc(),
           myPartSearchKey,
           sppOfLeftChild->getPushDownProperty());

  //--MV


  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  delete sppTemp;
  return sppForMe;

} //  MergeUnion::synthPhysicalProperty()

//<pb>

// -----------------------------------------------------------------------
// Helper function for the physical exprs that derive from GroupByAgg
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// GroupByAgg::rppAreCompatibleWithOperator()
// The following method is used by GroupBySplitRule::topMatch(),
// SortGroupBy::topMatch(), HashGroupBy::topMatch() and
// PhysShortCutGroupBy::topMatch() to determine whether it is worth to fire
// those rules.
// -----------------------------------------------------------------------
NABoolean GroupByAgg::rppAreCompatibleWithOperator
                         (const ReqdPhysicalProperty* const rppForMe) const
{
  PartitioningRequirement* partReq = rppForMe->getPartitioningRequirement();
  // ---------------------------------------------------------------------
  // The groupbys are labeled as partial leaf etc. to see from which
  // part of the groupby split rule they were generated, if at all.
  // Use this labels for suppressing unwanted configurations.
  // ---------------------------------------------------------------------
  if (rppForMe->executeInDP2())
  {
    // allow a in-CS full GroupBy when push-down to DP2, if a
    // push-down requirement is available.
    if ( isinBlockStmt() )
    {
       // do not allow push-down if no push-down req is available.
       // i.e., the aggr must be below CS which can generate push-down.
       // This applies to all forms of groupbys.
       const PushDownRequirement* pdr = rppForMe->getPushDownRequirement();
       if ( pdr == NULL OR NOT PushDownCSRequirement::isInstanceOf(pdr) )
         return FALSE;
    }

    // we only want leaf groupbys to execute in DP2
    if (isAPartialGroupByRoot() OR
        isAPartialGroupByNonLeaf())
      return FALSE;
  }
  else
  {
    // allowing a partialGBLeaf in ESP is a good idea if:
    // Parallelism is possible.
    // The partial GB Leaf has no partitioning requirement, so there
    // must be an exchange in between the partial GB Leaf and the
    // partial GB Root. This requirement exists because we do not
    // want the partial GB Leaf and partial GB Root to be in the
    // same process.
    // The partial GB Leaf has not been pushed below a TSJ. If the
    // partial GB Leaf has been pushed below a TSJ, it is probably
    // best for us to wait until it gets to DP2.
    NABoolean partialGBLeafInESPOK =
      (rppForMe->getCountOfPipelines() > 1) AND
      (partReq == NULL) AND
      NOT gbAggPushedBelowTSJ();

    // we don't want leaf groupbys to execute outside of DP2
    if (isAPartialGroupByLeaf() AND
        NOT partialGBLeafInESPOK)
      return FALSE; // partial GB leaf in ESP not allowed
  }

  // ---------------------------------------------------------------------
  // Compute the number of pipelines that are available for executing
  // the plan for this GroupBy.
  // ---------------------------------------------------------------------
  Lng32 numberOfPipelines = rppForMe->getCountOfPipelines();
  // ---------------------------------------------------------------------
  // A partial groupby that is a non leaf performs an intermediate
  // consolidation of partial groups/aggregates. The heuristic that
  // is applied below requires such an intermediate consolidator to
  // employ parallel execution (numberOfPipelines > 1) or to produce
  // multiple partitions. The intention is to prevent such an
  // intermediate consolidator from becoming a bottleneck, which is
  // an inhibitor for scalability.
  // ---------------------------------------------------------------------
  if (isAPartialGroupByNonLeaf() AND (numberOfPipelines == 1))
    return FALSE;

  // ---------------------------------------------------------------------
  //  Ensure that the parent partitioning requirement is compatible
  //  with the GB.
  // ---------------------------------------------------------------------
  if (partReq != NULL)
  {
    if (partReq->isRequirementFullySpecified())
    {
      if (groupExpr().isEmpty())
      {
        // Scalar aggregates can not execute with ESP parallelism,
        // except as part of a ReplicateNoBroadcast.
        if (NOT(partReq->isRequirementExactlyOne() OR
                partReq->isRequirementReplicateNoBroadcast()))
          return FALSE;
      }
      else
      {
        // If there is a group by expression, then the required
        // partitioning key columns must be a subset of the gb cols.
        // Contains will always return TRUE if the required part key
        // is the empty set.
        if (NOT groupExpr().contains(partReq->getPartitioningKey()))
          return FALSE;
      }
    }
    else // fuzzy requirement
    {
      CMPASSERT(partReq->isRequirementApproximatelyN());
      Lng32 reqPartCount = partReq->getCountOfPartitions();
      if (groupExpr().isEmpty())
      {
        // Scalar aggregates cannot execute in parallel, so the
        // requirement must allow a single partition part func.
        if ((reqPartCount != ANY_NUMBER_OF_PARTITIONS) AND
            ((reqPartCount -
              (reqPartCount * partReq->castToRequireApproximatelyNPartitions()->
                                      getAllowedDeviation())) >
             EXACTLY_ONE_PARTITION)
           )
          return FALSE;
      }
      else if (partReq->getPartitioningKey().entries() > 0)
      {
        ValueIdSet myPartKey = groupExpr();
        myPartKey.intersectSet(partReq->getPartitioningKey());
        // If the required partitioning key columns and the GB cols
        // are disjoint, then the requirement must allow a single
        // partition part func, because this will be the only way
        // to satisfy both the requirement and the GB cols.
        if (myPartKey.isEmpty() AND
            (reqPartCount != ANY_NUMBER_OF_PARTITIONS) AND
            ((reqPartCount -
              (reqPartCount * partReq->castToRequireApproximatelyNPartitions()->
                                        getAllowedDeviation())) >
             EXACTLY_ONE_PARTITION))
          return FALSE;
      }
    } // end if fuzzy requirement
  } // end if there was a partitioning requirement for the GB

  return TRUE;

}; // GroupByAgg::rppAreCompatibleWithOperator()

//<pb>
Context* GroupByAgg::createContextForAChild(Context* myContext,
                                   PlanWorkSpace* pws,
                                   Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  // If one Context has been generated for each child, return NULL
  // to signal completion.
  // ---------------------------------------------------------------------
  if (pws->getCountOfChildContexts() == getArity())
    return NULL;

  childIndex = 0;

  Lng32 planNumber = 0;
  const ReqdPhysicalProperty* rppForMe = myContext->getReqdPhysicalProperty();
  Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
  float childNumPartsAllowedDeviation = 0.0;
  NABoolean numOfESPsForced = FALSE;

  RequirementGenerator rg(child(0),rppForMe);

  PartitioningRequirement* preq = rppForMe->getPartitioningRequirement();

  // ---------------------------------------------------------------------
  // If this is not a partial groupby then the child must be partitioned
  // on the groupby columns (even if there are none!).
  // By adding an empty part key when there are no GB cols, this will
  // force a scalar aggregate to not execute in parallel.
  //
  // For pushed down groupbys, do not add the groupExpr() if it is empty.
  // Otherwise, we will get a singlePartFunc, which will lead to
  // no-match for partitioned-tables down in the tree.
  // ---------------------------------------------------------------------
  if ( (isNotAPartialGroupBy() OR isAPartialGroupByRoot()) )
  {
     if ( NOT rppForMe->executeInDP2() OR NOT isinBlockStmt() OR
          ( ( NOT groupExpr().isEmpty() ) AND
            ( ( rppForMe->getCountOfPipelines() > 1 ) OR
              ( CmpCommon::getDefault(COMP_BOOL_36) == DF_OFF )
            )
          )
        )
       rg.addPartitioningKey(groupExpr());
  }

  // ---------------------------------------------------------------------
  // Add the order or arrangement requirements needed for this groupby
  // ---------------------------------------------------------------------
  addArrangementAndOrderRequirements(rg);

  // ---------------------------------------------------------------------
  // If this is a CPU or memory-intensive operator or if we could benefit
  // from parallelism by performing multiple sort groupbys on different
  // ranges at the same time, then add a requirement for a maximum number
  // of partitions, unless that requirement would conflict with our
  // parent's requirement.
  //
  // Don't add a required number of partitions if we must execute in DP2,
  // because you can't change the number of DP2s.
  // Also don't specify a required number of partitions for a scalar
  // aggregate, because a scalar aggregate cannot execute in parallel.
  // ---------------------------------------------------------------------
  if (isRollup())
    {
      // GROUP BY ROLLUP needs to be evaluated in a single process
      rg.addNumOfPartitions(1);
    }
  else if ( !isAPartialGroupByLeaf1() && !isAPartialGroupByLeaf2() &&
       okToAttemptESPParallelism(myContext,
                                   pws,
                                   childNumPartsRequirement,
                                   childNumPartsAllowedDeviation,
                                   numOfESPsForced) AND
         NOT rppForMe->executeInDP2() AND
         NOT groupExpr().isEmpty())
   {
       if (NOT numOfESPsForced)
         rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                   &childNumPartsAllowedDeviation);
       rg.addNumOfPartitions(childNumPartsRequirement,
                             childNumPartsAllowedDeviation);
   
       // Can not allow skewBuster hash join co-exist with a full groupby
       // or a partial groupby root in the same ESPs because the a group
       // (in a non-skewbuster plan) can be broken into pieces across
       // different ESPs.
   
       // The BR case (i.e. the right child of a skew buster join is a full
       // groupby) is dealt with in RelExpr::rppRequiresEnforcer().
       //
       // The UD case (i.e., the left child of a skew buster join is a full
       // groupby) is dealt with through the partitioning key requirement
       // (i.e., the partitioning keys added by statement
       // rg.addPartitioningKey(groupExpr()) above will NEVER be satisfied
       // by a SkewedDataPartitioningFunction).
       //
       // A full groupBy node is prevented from being the immediate parent of
       // the skew-buster join due to the reason similar to the UD case.
   } // end if ok to try parallelism

  // ---------------------------------------------------------------------
  // Done adding all the requirements together, now see whether it worked
  // and give up if it is not possible to satisfy them
  // ---------------------------------------------------------------------
  if (NOT rg.checkFeasibility())
    return NULL;

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);

  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as those in
  // rppForChild. Reuse it, if found. Otherwise, create a new Context
  // that contains rppForChild as the required physical properties..
  // ---------------------------------------------------------------------
  Context* result = shareContext(
         childIndex,
         rg.produceRequirement(),
         myContext->getInputPhysicalProperty(),
         costLimit,
         myContext,
         myContext->getInputLogProp());

  // try to create a nice context for the child of groupBy below the ROOT
  // that requires repartitioning for grouping. The check for the parent
  // might need to be more elaborate, now it's just a flag in corresponding
  // group. 
  if ( (CmpCommon::getDefault(COMP_BOOL_33) == DF_ON) AND
       (result->getReqdPhysicalProperty()->getCountOfPipelines() > 1) AND
       (NOT groupExpr().isEmpty()) AND
       (isNotAPartialGroupBy() OR isAPartialGroupByRoot()) AND
       (*CURRSTMT_OPTGLOBALS->memo)[myContext->getGroupId()]->isBelowRoot()
     )
  {
    result->setNice(TRUE);
  }


  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, planNumber, result);

  return result;

} // GroupByAgg::createContextForAChild()

//<pb>
void GroupByAgg::addArrangementAndOrderRequirements(
   RequirementGenerator &rg)
{
  // the default implementation is to do nothing
  // (works fine for hash groupby)
}

//<pb>
// -----------------------------------------------------------------------
// member functions for class SortGroupBy
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// SortGroupBy::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
SortGroupBy::costMethod() const
{
  static THREAD_P CostMethodSortGroupBy *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodSortGroupBy();
  return m;
} // SortGroupBy::costMethod()

//<pb>
void SortGroupBy::addArrangementAndOrderRequirements(
   RequirementGenerator &rg)
{
  // A sort groupby needs an arrangement by the grouping
  // columns, it needs nothing if there are no grouping
  // columns.
  // Once we have "partial" arrangement requirements we
  // could indicate those for partial groupbys.
  // A GROUP BY ROLLUP needs the exact order as specified.
  if (isRollup() && (NOT rollupGroupExprList().isEmpty()))
    {
      if( NOT extraOrderExpr().isEmpty())
      {
        ValueIdList groupExprCpy(rollupGroupExprList());
        groupExprCpy.insert(extraOrderExpr());
        rg.addSortKey(groupExprCpy);
      }
      else
        rg.addSortKey(rollupGroupExprList());
    }
  else if (NOT groupExpr().isEmpty())
    {
      if( NOT extraOrderExpr().isEmpty())
      {
        ValueIdList groupExprCpy(groupExpr());
        groupExprCpy.insert(extraOrderExpr());
        rg.addSortKey(groupExprCpy);
      }
      else {
        if (rg.getStartRequirements()->executeInDP2())
          rg.addArrangement(groupExpr(),NO_SOT);
        else
          rg.addArrangement(groupExpr(),ESP_SOT);
      }
    }
}

//<pb>
//==============================================================================
//  Synthesize physical properties for Sort-group-by operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
SortGroupBy::synthPhysicalProperty(const Context* myContext,
                                   const Lng32     planNumber,
                                   PlanWorkSpace  *pws)
{
  const PhysicalProperty* const sppOfChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(0);

  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(myContext,
                                                             planNumber,
                                                             pws);

  PhysicalProperty* sppForMe =
    new (CmpCommon::statementHeap())
      PhysicalProperty((isRollup() ? ValueIdList() : sppOfChild->getSortKey()),
                       sppOfChild->getSortOrderType(),
                       sppOfChild->getDp2SortOrderPartFunc(),
                       sppOfChild->getPartitioningFunction(),
                       sppOfChild->getPlanExecutionLocation(),
                       sppOfChild->getDataSourceEnum(),
                       sppOfChild->getIndexDesc(),
                       sppOfChild->getPartSearchKey(),
                       sppOfChild->getPushDownProperty());

  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());
  // aggregate stores entire result before it gets sent on
  if (groupExpr().isEmpty())
    sppForMe->setDataSourceEnum(SOURCE_TRANSIENT_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  DP2CostDataThatDependsOnSPP * dp2CostInfo = (DP2CostDataThatDependsOnSPP *)
                     sppOfChild->getDP2CostThatDependsOnSPP();

  if (sppForMe->executeInDP2())
     sppForMe->setDP2CostThatDependsOnSPP( dp2CostInfo);

  delete sppTemp;
  return sppForMe;

} // SortGroupBy::synthPhysicalProperty()

//<pb>

// -----------------------------------------------------------------------
// member functions for class PhysShortCutGroupBy
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// PhysShortCutGroupBy::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
PhysShortCutGroupBy::costMethod() const
{
  static THREAD_P CostMethodShortCutGroupBy *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodShortCutGroupBy();
  return m;
} // PhysShortCutGroupBy::costMethod()

//<pb>
void PhysShortCutGroupBy::addArrangementAndOrderRequirements(
   RequirementGenerator &rg)
{
  // The ShortCutGroupBy requires an order instead of an arrangement
  // right now it only uses a single value id
  //ShortCutGroupBy handles AnyTrue expr as well as min-max expressions
  //opt_for_min indicates x>anytrue y and as a result we want y to be
  //sorted increasing. Realize that this also works for min(y) so opt_for_min
  //is true for min(y).
  //In the same way opt_for_max works with x<anytrue y and max(y) and we ask
  //a inverse order on y(i.e. decreasing order)
  ValueIdList sk;

  if (opt_for_min_)
    {
      // the order of rhs becomes the optimization goal of the child
      sk.insert(rhs_anytrue_->getValueId());
    }
  else if (opt_for_max_)
    {
      // the order of the inverse of the rhs becomes the optimization goal
      ItemExpr *inverseCol =
        new(CmpCommon::statementHeap()) InverseOrder(rhs_anytrue_);
      inverseCol->synthTypeAndValueId();
      sk.insert(inverseCol->getValueId());
    }
  const ValueIdSet &aggrs = aggregateExpr();

  CMPASSERT(groupExpr().isEmpty());

  CMPASSERT(NOT aggrs.isEmpty());

  ValueId aggr_valueid = aggrs.init();
  // "next" is probably called here for its side effect because we know
  // "aggrs.next()" returns true (ie, aggrs is not empty).
  // coverity[unchecked_value]
  aggrs.next(aggr_valueid);

  ItemExpr *item_expr = aggr_valueid.getItemExpr();
  OperatorTypeEnum op = item_expr->getOperatorType();

  if(op==ITM_ANY_TRUE)
  {
    // Shouldn't/Can't add a sort order type requirement
    // if we are in DP2
    if (rg.getStartRequirements()->executeInDP2())
      rg.addSortKey(sk,NO_SOT);
    else
      {
        if (CmpCommon::getDefault(COSTING_SHORTCUT_GROUPBY_FIX) != DF_ON)
          rg.addSortKey(sk,ESP_SOT);
        else
          rg.addSortKey(sk,ESP_NO_SORT_SOT);
      }
  }
  else
  {
    // Shouldn't/Can't add a sort order type requirement
    // if we are in DP2
    if (rg.getStartRequirements()->executeInDP2())
      rg.addSortKey(sk,NO_SOT);
    else
      rg.addSortKey(sk,ESP_NO_SORT_SOT);
  }
}

//<pb>
//==============================================================================
//  Synthesize physical properties for Any-true-group-by operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
PhysShortCutGroupBy::synthPhysicalProperty(const Context* myContext,
                                           const Lng32     planNumber,
                                           PlanWorkSpace  *pws)
{
  ValueIdList emptySortKey;
  const PhysicalProperty* sppOfChild = myContext->
                                 getPhysicalPropertyOfSolutionForChild(0);

  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(myContext,
                                                             planNumber,
                                                             pws);

  PhysicalProperty* sppForMe =
    new (CmpCommon::statementHeap())
      PhysicalProperty(emptySortKey,
                       NO_SOT, // no sort key, so no sort order type either
                       NULL, // no dp2SortOrderPartFunc either
                       sppOfChild->getPartitioningFunction(),
                       sppOfChild->getPlanExecutionLocation(),
                       sppOfChild->getDataSourceEnum(),
                       sppOfChild->getIndexDesc(),
                       sppOfChild->getPartSearchKey(),
                       sppOfChild->getPushDownProperty());

  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());
  // aggregate stores entire result before it gets sent on
  if (groupExpr().isEmpty())
    sppForMe->setDataSourceEnum(SOURCE_TRANSIENT_TABLE);

  DP2CostDataThatDependsOnSPP * dp2CostInfo = (DP2CostDataThatDependsOnSPP *)
                       sppOfChild->getDP2CostThatDependsOnSPP();

  if (sppForMe->executeInDP2())
    sppForMe->setDP2CostThatDependsOnSPP( dp2CostInfo);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  delete sppTemp;
  return sppForMe;

} // PhysShortCutGroupBy::synthPhysicalProperty()

//<pb>

// -----------------------------------------------------------------------
// member functions for class HashGroupBy
// -----------------------------------------------------------------------
NABoolean HashGroupBy::isBigMemoryOperator(const PlanWorkSpace* pws,
                                           const Lng32 planNumber)
{
  const Context* context = pws->getContext();
  double memoryLimitPerCPU = CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU();

  // ---------------------------------------------------------------------
  // With no memory constraints, a HGB operator could use as much as an
  // amount of memory to store all the groups plus any aggregates. Hash
  // key with the chain pointer make up 8 more bytes for each group.
  // ---------------------------------------------------------------------
  const ReqdPhysicalProperty* rppForMe = context->getReqdPhysicalProperty();
  // Start off assuming that the operator will use all available CPUs.
  Lng32 cpuCount = rppForMe->getCountOfAvailableCPUs();
  PartitioningRequirement* partReq = rppForMe->getPartitioningRequirement();
  const PhysicalProperty* spp = context->getPlan()->getPhysicalProperty();
  Lng32 numOfStreams;

  // If the physical properties are available, then this means we
  // are on the way back up the tree. Get the actual level of
  // parallelism from the spp to determine if the number of cpus we
  // are using are less than the maximum number available.
  if (spp != NULL)
  {
    PartitioningFunction* partFunc = spp->getPartitioningFunction();
    numOfStreams = partFunc->getCountOfPartitions();
    if (numOfStreams < cpuCount)
      cpuCount = numOfStreams;
  }
  else
  if ((partReq != NULL) AND
      (partReq->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS))
  {
    // If there is a partitioning requirement, then this may limit
    // the number of CPUs that can be used.
    numOfStreams = partReq->getCountOfPartitions();
    if (numOfStreams < cpuCount)
      cpuCount = numOfStreams;
  }

  EstLogPropSharedPtr inLogProp = context->getInputLogProp();
  const double probeCount =
                      MAXOF(1.,inLogProp->getResultCardinality().value());
  const double myGroupCount = getGroupAttr()->
         intermedOutputLogProp(inLogProp)->getResultCardinality().value();

  const double rowsPerCpu = MAXOF(1.,(myGroupCount / cpuCount));
  const double rowsPerCpuPerProbe = MAXOF(1.,(rowsPerCpu / probeCount));

  const Lng32 myRowLength =
                getGroupAttr()->getCharacteristicOutputs().getRowLength();
  const Lng32 extRowLength = myRowLength + 8;
  const double fileSizePerCpu = ((rowsPerCpuPerProbe * extRowLength) / 1024.);

  if (spp != NULL &&
      CmpCommon::getDefault(COMP_BOOL_51) == DF_ON
     )
  {
    CurrentFragmentBigMemoryProperty * bigMemoryProperty =
                       new (CmpCommon::statementHeap())
                          CurrentFragmentBigMemoryProperty();

    ((PhysicalProperty *)spp)->
        setBigMemoryEstimationProperty(bigMemoryProperty);

    bigMemoryProperty->setCurrentFileSize(fileSizePerCpu);
    bigMemoryProperty->setOperatorType(getOperatorType());

    // get cumulative file size of the fragment; get the child spp??
    const PhysicalProperty *childSpp =
       context->getPhysicalPropertyOfSolutionForChild(0);

    if (childSpp != NULL)
    {
      CurrentFragmentBigMemoryProperty * memProp =
         (CurrentFragmentBigMemoryProperty *)
             ((PhysicalProperty *)childSpp)->getBigMemoryEstimationProperty();

      if (memProp != NULL)
      {
         double childCumulativeMemSize = memProp->getCumulativeFileSize();
         bigMemoryProperty->incrementCumulativeMemSize(childCumulativeMemSize);
         memoryLimitPerCPU -= childCumulativeMemSize;
      }
    }
  }

  return (fileSizePerCpu >= memoryLimitPerCPU);
}  // HashGroupBy::isBigMemoryOperator

//<pb>
// -----------------------------------------------------------------------
// HashGroupBy::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
HashGroupBy::costMethod() const
{
  static THREAD_P CostMethodHashGroupBy *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodHashGroupBy();
  return m;
} // HashGroupBy::costMethod()

//<pb>
//==============================================================================
//  Synthesize physical properties for Hash-group-by operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
HashGroupBy::synthPhysicalProperty(const Context* myContext,
                                   const Lng32     planNumber,
                                   PlanWorkSpace  *pws)
{
  ValueIdList emptySortKey;
  const PhysicalProperty* sppOfChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(0);

  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(myContext,
                                                             planNumber,
                                                             pws);

  // ---------------------------------------------------------------------
  // The output of a hash groupby is not sorted.
  // It produces a hash partitioned set of rows (groups).
  // ---------------------------------------------------------------------
  PhysicalProperty* sppForMe =
    new (CmpCommon::statementHeap())
      PhysicalProperty(emptySortKey,
                       NO_SOT,
                       NULL,
                       sppOfChild->getPartitioningFunction(),
                       sppOfChild->getPlanExecutionLocation(),
                       sppOfChild->getDataSourceEnum(),
                       sppOfChild->getIndexDesc(),
                       sppOfChild->getPartSearchKey());

  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());
  // aggregate stores entire result before it gets sent on,
  // non-partial hash groupby also stores entire result
  if (groupExpr().isEmpty() OR
      isNotAPartialGroupBy() OR isAPartialGroupByRoot())
    sppForMe->setDataSourceEnum(SOURCE_TRANSIENT_TABLE);

  // remove anything that's not covered by the group attributes
  // (NB: no real need to call this method, since the sortKey is empty, and that's
  // the only thing that this method checks currently)
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  DP2CostDataThatDependsOnSPP * dp2CostInfo = (DP2CostDataThatDependsOnSPP *)
                       sppOfChild->getDP2CostThatDependsOnSPP();

  if (sppForMe->executeInDP2())
    sppForMe->setDP2CostThatDependsOnSPP( dp2CostInfo);

  delete sppTemp;
  return sppForMe;

} // HashGroupBy::synthPhysicalProperty()

//<pb>

// -----------------------------------------------------------------------
// Member functions for class RelRoot
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// RelRoot::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
RelRoot::costMethod() const
{
  static THREAD_P CostMethodRelRoot *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodRelRoot();
  return m;
} // RelRoot::costMethod()

//<pb>
Context* RelRoot::createContextForAChild(Context* myContext,
                                PlanWorkSpace* pws,
                                Lng32& childIndex)
{
  childIndex = 0;
  Lng32 planNumber = pws->getCountOfChildContexts();

  const ReqdPhysicalProperty* rppForMe =
                                    myContext->getReqdPhysicalProperty();
  const ReqdPhysicalProperty* rppForChild;
  PartitioningRequirement* partReq;
  PlanExecutionEnum loc;
  ValueIdList* sortKey = NULL;
  SortOrderTypeEnum  sortOrderTypeReq = NO_SOT;

  // Get the value from the defaults table that specifies whether
  // the optimizer should attempt ESP parallelism.
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  DefaultToken attESPPara = CURRSTMT_OPTDEFAULTS->attemptESPParallelism();
  NABoolean hasTMUDFsOrLRU = (getGroupAttr()->getNumTMUDFs() > 0 ||
                              containsLRU());

  // heuristics for nice context we want to detect the group which is just
  // below root. For example if group by is just below root we want it to
  // create a nice context (with repartitioning on grouping columns) that
  // will not be propagated below top join.
  (*CURRSTMT_OPTGLOBALS->memo)[child(0).getGroupId()]->setBelowRoot(TRUE);

  // ---------------------------------------------------------------------
  // If we've created all the necessary child contexts, we're done.
  // ---------------------------------------------------------------------
  if (((planNumber == 1)
       AND ((attESPPara != DF_SYSTEM)
#ifndef NDEBUG
            OR getenv("SINGLE_ROOT_CONTEXT") != NULL
#endif
       )) OR
      (planNumber > 1))
    return NULL;

  // ---------------------------------------------------------------------
  // Decide how many cpus are available. There are four default table
  // entries that have an influence on this value:
  //
  // - ESP parallelism can be switched off altogether by setting
  //   the ATTEMPT_ESP_PARALLELISM defaults table entry to "OFF".
  // - The PARALLEL_NUM_ESPS default attribute is either the keyword "SYSTEM",
  //   or the positive number of ESPs to use.
  // - If PARALLEL_NUM_ESPS is "SYSTEM",
  //   then we get the number of nodes in the cluster and multiply it by
  //   the number of processors per (SMP) node by reading the
  //   DEF_NUM_NODES_IN_ACTIVE_CLUSTERS and DEF_NUM_SMP_CPUS entries.
  // - If PARALLEL_NUM_ESPS is a positive number, ensure that it does not
  //   exceed the number of available CPUs in the system.
  // ---------------------------------------------------------------------
  Lng32 countOfCPUs = DEFAULT_SINGLETON;

  Lng32 pipelinesPerCPU = DEFAULT_SINGLETON;

  // Regardless of how the ATTEMPT_ESP_PARALLELISM CQD is set, some operations
  // like LRU always execute under ESPs for partitioned tables.
  // Note that TMUDFs can control their own degree of parallelism,
  // and that the method to do this is called after we reach here.
  // Note also that some conditions below may still override this.
  if ( ((CURRSTMT_OPTDEFAULTS->attemptESPParallelism() != DF_OFF) OR
        hasTMUDFsOrLRU)
      // QSTUFF
      // we don't support paralled execution in the get_next
      // protocol yet - but for streams there should not be
      // a problem.
      && NOT getGroupAttr()->isEmbeddedUpdateOrDelete()
      && NOT getGroupAttr()->isStream()
      && NOT getTriggersList()
      && NOT containsOnStatementMV()
      // QSTUFF
     )
  {

    // Get the maximum number of processes per cpu for a given operator
    // from the defaults table.
    pipelinesPerCPU = 1;

    // -------------------------------------------------------------------
    // Extract number of cpus per node and number of nodes in cluster from
    //  defaults table.
    // -------------------------------------------------------------------

    NABoolean canAdjustDoP = TRUE;
    NABoolean isASON = FALSE;

    NABoolean fakeEnv = FALSE;
    countOfCPUs = defs.getTotalNumOfESPsInCluster(fakeEnv);

    // Do not enable the Adaptive Segmentation functionality for
    // parallel label (DDL) operations or for a parallel extract
    // operation, or fast loading into traf tables. This will 
    // prevent AS from reducing the max degree of
    // parallelism for these operations.

    
    OperatorTypeEnum childOpType = child(0).getLogExpr()->getOperatorType();

    // Decide if it is a fast trafodion load query
    NABoolean isFastLoadIntoTrafodion = FALSE;

    if ( childOpType == REL_UNARY_INSERT ) {

       RelExpr* c0 = child(0).getLogExpr();
       Insert* ins = (Insert*)c0;

       isFastLoadIntoTrafodion = ins->getIsTrafLoadPrep();
    }

    if ((CmpCommon::getDefault(ASG_FEATURE) == DF_ON) &&
        (childOpType != REL_EXE_UTIL) &&
        (numExtractStreams_ == 0) &&
        !isFastLoadIntoTrafodion &&
        !hasTMUDFsOrLRU)
    {
        countOfCPUs = 
            CURRSTMT_OPTDEFAULTS->getMaximumDegreeOfParallelism();

        // Adaptive segmentation is ON
        isASON = TRUE;
    }

    // Get the value as a token code, no errmsg if not a keyword.
    if (CmpCommon::getDefault(PARALLEL_NUM_ESPS, 0) != DF_SYSTEM)
    {
      // -------------------------------------------------------------------
      // A value for PARALLEL_NUM_ESPS exists.  Use it for the count of cpus
      // but don't exceed the number of cpus available in the cluster.
      // On SQ, include the number of ESPs allowed per cpu.
      // -------------------------------------------------------------------

      canAdjustDoP = FALSE;
    }

    // final adjustment to countOfCPUs and pipelinesPerCPU - special cases
    // 
    RelExpr* x = child(0).getGroupAttr()->getLogExprForSynthesis();

    // for multi-commit DELETE (expressed as DELETE WITH MULTI COMMIT FROM <t>)
    if ( containsLRU() ) 
    {
       RelExpr* x = child(0).getGroupAttr()->getLogExprForSynthesis();

       if ( x && x->getOperatorType() == REL_EXE_UTIL )
       {
          PartitioningFunction *pf = ((ExeUtilExpr*)x)->getUtilTableDesc()
                                                 ->getClusteringIndex()
                                                 ->getPartitioningFunction();

          const NodeMap* np;
          UInt32 partns = 1;
          if ( pf && (np = pf->getNodeMap()) ) {
             // set countOfCPUs to the number of partitions
             partns = np->getNumEntries();
          }
          countOfCPUs = partns;
          pipelinesPerCPU = 1;
          CURRSTMT_OPTDEFAULTS->setRequiredESPs(partns);

          canAdjustDoP = FALSE;
       }
    }
   
    if (isFastLoadIntoTrafodion)
    {
      Insert *ins = (Insert*)(x->castToRelExpr());

       PartitioningFunction *pf = ins->getTableDesc()
                                     ->getClusteringIndex()
                                     ->getPartitioningFunction();

       const NodeMap* np;
       Lng32 partns = 1;
       if ( pf && (np = pf->getNodeMap()) )
       {
          partns = np->getNumEntries();
       }
       if (partns>1)
       {
         countOfCPUs = partns;
         pipelinesPerCPU = 1;
         CURRSTMT_OPTDEFAULTS->setRequiredESPs(partns);
         canAdjustDoP = FALSE;
       }
    }

    // for side-tree INSERT (expressed as INSERT USING SIDEINSERTS INTO <t> 
    // <source> )

    // Set countOfCPUs to the number of partitions of the inner table
    // if countOfCPUs is not a multiple of the of the # of partitions
    // (e.g., 8 vs 34). The key is to avoid more than one TSJ ESPs to
    // send rows to an inner table partition.

    NADefaults &defs = ActiveSchemaDB()->getDefaults();

    Lng32 ocrControl = defs.getAsLong(OCR_FOR_SIDETREE_INSERT);
  
    // setting for OCR_FOR_SIDETREE_INSERT:
    //  0: OCR repartition is disableld
    //  1: OCR repartition is enable and will be done if DoP can not divide
    //     # of partitions of the inner table 
    //  2: OCR repartition is enabled and will be done

    if ( ocrControl > 0 && x && x->getOperatorType() == REL_UNARY_INSERT )
    {
       Insert *ins = (Insert*)(x->castToRelExpr());

       if ( ins->getInsertType() == Insert::VSBB_LOAD )
       {

          PartitioningFunction *pf = ins->getTableDesc()
                                        ->getClusteringIndex()
                                        ->getPartitioningFunction();

          const NodeMap* np;
          Lng32 partns = 1;
          if ( pf && (np = pf->getNodeMap()) ) {
             partns = np->getNumEntries();
          }

          if (
              ( ocrControl == 1 &&
               (
                ((countOfCPUs < partns) && (partns % countOfCPUs != 0)) ||
                (countOfCPUs > partns) 
               ) 
              ) ||
              ocrControl == 2
             )
          { 
             countOfCPUs = partns;
             pipelinesPerCPU = 1;
             CURRSTMT_OPTDEFAULTS->setRequiredESPs(partns);
             canAdjustDoP = FALSE;
          }
       }
    }
             
                               
    Lng32 minBytesPerESP = defs.getAsLong(HBASE_MIN_BYTES_PER_ESP_PARTITION);

    // To be replaced later by a different CQD
    if ( CmpCommon::getDefault(HBASE_RANGE_PARTITIONING) == DF_OFF  ||
         isASON ||
         hasTMUDFsOrLRU)
      canAdjustDoP = FALSE;

    // Adjust DoP based on table size, if possible
    if ( canAdjustDoP ) {
       QueryAnalysis* qAnalysis = CmpCommon::statement()->getQueryAnalysis();
       TableAnalysis * tAnalysis = qAnalysis->getLargestTable();
   
       if ( tAnalysis ) {
          CostScalar tableSize = tAnalysis->getCardinalityOfBaseTable() *
                                 tAnalysis->getRecordSizeOfBaseTable() ;
   
          CostScalar espsInCS = tableSize / CostScalar(minBytesPerESP);
   
          Lng32 esps = (Lng32)(espsInCS.getCeiling().getValue());
   
          if ( esps < 0 )
            esps = countOfCPUs;
          else {
             if ( esps < 1 )
                countOfCPUs = 1;
             else {
   
               if ( esps < countOfCPUs )
                  countOfCPUs = esps;
             }
          }
          pipelinesPerCPU = 1;
   
       }
    }

    // --------------------------------------
    // Ensure cpu count is a positive number.
    // --------------------------------------
    countOfCPUs = MIN_ONE(countOfCPUs);

#ifdef _DEBUG
    if ((CmpCommon::getDefault( NSK_DBG ) == DF_ON) &&
        (CmpCommon::getDefault( NSK_DBG_GENERIC ) == DF_ON))
    {
      CURRCONTEXT_OPTDEBUG->stream() << endl << "countOfCPUs= " << countOfCPUs << endl;
    }
#endif
  } // end if parallel execution is enabled


  // Setup the range partition flag in OptDefaults when the query
  // is a count query
/*
  if ( child(0)->getOperatorType == REL_GROUPBY ) {
     CURRSTMT_OPTDEFAULTS->setDoRangePartitionForHbase(TRUE);
  }
*/


  // Create a "teaser" context if the user is letting the optimizer
  // decide heuristically whether parallelism is a good idea or not
  // for each operator.
  if ((planNumber == 0) AND
      (attESPPara == DF_SYSTEM)
#ifndef NDEBUG
      // set this env var to disable the "teaser" context
      AND getenv("SINGLE_ROOT_CONTEXT") == NULL
#endif
     )
    {
      // Start off with a context that is less stringent than what the
      // root node needs.  Don't impose any partitioning or location
      // requirements other than a requirement to be outside of
      // DP2. Why do we do this: we hope that the optimal solution
      // will actually execute in the master and that it has one
      // partition. If that is the case, the second context (see a few
      // lines below) will cause no work, it will just take the
      // optimal solution of the first context.  The effect is that we
      // do much less work, since we save all the partitioning
      // enforcers. This trick works only if the optimal solution is
      // not parallel in the node below the root, but that is the case
      // where optimization cost matters most.
      // RelRoot::currentPlanIsAcceptable makes sure that we don't
      // use the solution of this context.
      partReq = NULL;
      loc = EXECUTE_IN_MASTER_OR_ESP;
    }
  else
    {
      // The real context: the root node executes in the master
      // executor, always wants 1 data stream
      if (CmpCommon::getDefault(COMP_BOOL_82) == DF_ON) {
      partReq=new(CmpCommon::statementHeap())RequireExactlyOnePartition(TRUE);
      } else {
        partReq = new(CmpCommon::statementHeap())RequireExactlyOnePartition();

      }
      loc = EXECUTE_IN_MASTER;
    }

  // get the order by list
  ValueIdList orderByList(reqdOrder_);

  // Are there any required ordering columns?
  if (orderByList.entries() > 0)
  {
    // get input constant values
    ValueIdSet inputVals = getGroupAttr()->getCharacteristicInputs();

    // remove any input constant values from the required ordering,
    // since ordering by a constant is a no-op
    orderByList.removeCoveredExprs(inputVals);

    // Will need to remember the simplified valueId's of the expressions
    // in the order by list.
    ValueIdList simpleOrderByList;

    CollIndex i = 0;
    while (i < orderByList.entries())
    {
      // Need to check if we have seen the simplified version
      // of this valueId before. If we have, we need to remove it,
      // because we don't want or need to ask to order on an
      // expression that has the same simplified valueId as an
      // expression we have already seen in the list.
      ValueId svid = orderByList[i].getItemExpr()->
                       simplifyOrderExpr()->getValueId();
      if (simpleOrderByList.contains(svid))
        orderByList.removeAt(i);
      else
      {
        // If we haven't seen it before, then remember it.
        simpleOrderByList.insert(svid);
        // Only increment the array counter "i" if we didn't remove
        // the current entry, because if we did remove the current
        // entry, "i" will already point to the next entry in the list!
        i++;
      }
    } // end while more required order columns
  } // end if there was a required order

  if (orderByList.entries() > 0)             // got a required ordering ?
  {
    sortKey = new(CmpCommon::statementHeap()) ValueIdList(orderByList);
    sortOrderTypeReq = ESP_SOT;
  }

  // ---------------------------------------------------------------------
  // A root node is never partitioned and never excludes properties.
  // It requires an order if the query had an order by clause.
  // ---------------------------------------------------------------------
  if (rppForMe)
    rppForChild = new (CmpCommon::statementHeap())
                    ReqdPhysicalProperty(
                               *rppForMe,
                               NULL,    // no arrangement of rows required
                               sortKey, // user-specified sort order
                               sortOrderTypeReq,
                               NULL, // never a dp2SortOrderPartReq at the root
                               partReq,
                               loc,
                               countOfCPUs,
                               pipelinesPerCPU);
  else
    rppForChild = new (CmpCommon::statementHeap())
                    ReqdPhysicalProperty(
                               NULL,    // no arrangement of rows required
                               sortKey, // user-specified sort order
                               sortOrderTypeReq,
                               NULL, // never a dp2SortOrderPartReq at the root
                               FALSE,   // no logical order or arrangement
                               partReq,
                               NULL,    // no logical part. requirement
                               loc,
                               countOfCPUs,
                               pipelinesPerCPU,
                               CURRSTMT_OPTDEFAULTS->getDefaultCostWeight(),
                               CURRSTMT_OPTDEFAULTS->getDefaultPerformanceGoal(),
                               NULL, NULL);

  // Support for CONTROL QUERY SHAPE
  if (reqdShape_ AND reqdShape_->isCutOp())
    reqdShape_ = NULL;

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);

  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as those in
  // rppForChild. Reuse it, if found. Otherwise, create a new Context
  // that contains rppForChild as the required physical properties.
  // The child context's solution has to match the required shape
  // specified in a CONTROL QUERY SHAPE statement, if applicable.
  // ---------------------------------------------------------------------
  Context* result = shareContext(childIndex,
                                 rppForChild,
                                 myContext->getInputPhysicalProperty(),
                                 costLimit,
                                 myContext,
                                 myContext->getInputLogProp(),
                                 reqdShape_);

  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, planNumber, result);

  return result;

} //  RelRoot::createContextForAChild()

NABoolean RelRoot::currentPlanIsAcceptable(Lng32 planNo,
            const ReqdPhysicalProperty* const rppForMe) const
{
  DefaultToken attESPPara = CURRSTMT_OPTDEFAULTS->attemptESPParallelism();

  // Don't consider plan 0 when ATTEMPT_ESP_PARALLELISM is set
  // to level SYSTEM, it has the wrong requirements!
  // See RelRoot::createContextForAChild for details
  if ((planNo == 0) AND
      (attESPPara == DF_SYSTEM)
#ifndef NDEBUG
      // set this env var to disable the "teaser" context
      AND getenv("SINGLE_ROOT_CONTEXT") == NULL
#endif
     )
    return FALSE;
  else
    return TRUE;

} // RelRoot::currentPlanIsAcceptable()

//<pb>
//==============================================================================
//  Synthesize physical properties for RelRoot operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
RelRoot::synthPhysicalProperty(const Context* myContext,
                               const Lng32     planNumber,
                               PlanWorkSpace  *pws)
{
  const PhysicalProperty* sppForChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(0);

  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(myContext,
                                                             planNumber,
                                                             pws);

  PhysicalProperty* sppForMe =
    new (CmpCommon::statementHeap())
      PhysicalProperty(NULL,
                       EXECUTE_IN_MASTER,
                       sppForChild->getDataSourceEnum());

  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  delete sppTemp;
  return sppForMe;

} // RelRoot::synthPhysicalProperty()

//<pb>

// -----------------------------------------------------------------------
// Member functions for class MapValueIds
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// MapValueIds::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
MapValueIds::costMethod() const
{
  static THREAD_P CostMethodFixedCostPerRow *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap())
            CostMethodFixedCostPerRow( 0.001     // constant cost for the node
                                     , 0.0       // cost per child row
                                     , 0.0       // cost per output row
                                     );
  return m;
}

// ---------------------------------------------------------------------
// Performs mapping on the partitioning function, from the
// MapValueIds node to the child.
// ---------------------------------------------------------------------
PartitioningFunction* MapValueIds::mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0)
{
  NABoolean mapItUp = FALSE;
  return partFunc->copyAndRemap(map_,mapItUp);
} // end MapValueIds::mapPartitioningFunction()

//<pb>
Context* MapValueIds::createContextForAChild(Context* myContext,
                                    PlanWorkSpace* pws,
                                    Lng32& childIndex)
{
  childIndex = 0;
  Lng32 planNumber = 0;

  const ReqdPhysicalProperty* rppForMe =
                                     myContext->getReqdPhysicalProperty();
  const ReqdPhysicalProperty* rppForChild;
  ValueIdSet * arrangedColsReqForChild = NULL;
  ValueIdList * sortKeyReqForChild = NULL;
  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();
  PartitioningRequirement* partReqForChild = partReqForMe;
  PartitioningRequirement* dp2SortOrderPartReqForMe =
    rppForMe->getDp2SortOrderPartReq();
  PartitioningRequirement* dp2SortOrderPartReqForChild =
    dp2SortOrderPartReqForMe;

  // ---------------------------------------------------------------------
  // If one Context has been generated for each child, return NULL
  // to signal completion.
  // ---------------------------------------------------------------------
  if (pws->getCountOfChildContexts() == getArity())
    return NULL;

  // ---------------------------------------------------------------------
  // now map all components of the required props that do use value ids
  // ---------------------------------------------------------------------
  if (rppForMe->getArrangedCols() != NULL)
  {
    arrangedColsReqForChild = new(CmpCommon::statementHeap()) ValueIdSet();
    map_.rewriteValueIdSetDown(*rppForMe->getArrangedCols(),
                               *arrangedColsReqForChild);
  }

  if (rppForMe->getSortKey() != NULL)
  {
    sortKeyReqForChild = new(CmpCommon::statementHeap()) ValueIdList();
    map_.rewriteValueIdListDown(*rppForMe->getSortKey(),
                                *sortKeyReqForChild);
  }

  if ((partReqForMe != NULL) AND
      (NOT partReqForMe->getPartitioningKey().isEmpty()))
  {
    // -------------------------------------------------------------------
    // Rewrite the partitioning key in terms of the values that appear
    // below this MapValueIds.
    // -------------------------------------------------------------------
    NABoolean mapItUp = FALSE;
    partReqForChild =
      partReqForMe->copyAndRemap(map_, mapItUp);
  }

  if ((dp2SortOrderPartReqForMe != NULL) AND
      (NOT dp2SortOrderPartReqForMe->getPartitioningKey().isEmpty()))
  {
    // -------------------------------------------------------------------
    // Rewrite the partitioning key in terms of the values that appear
    // below this MapValueIds.
    // -------------------------------------------------------------------
    NABoolean mapItUp = FALSE;
    dp2SortOrderPartReqForChild =
      dp2SortOrderPartReqForMe->copyAndRemap(map_, mapItUp);
  }

  rppForChild = new (CmpCommon::statementHeap())
                  ReqdPhysicalProperty(*rppForMe,
                                       arrangedColsReqForChild,
                                       sortKeyReqForChild,
                                       rppForMe->getSortOrderTypeReq(),
                                       dp2SortOrderPartReqForChild,
                                       partReqForChild);

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);

  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as those in
  // rppForChild. Reuse it, if found. Otherwise, create a new Context
  // that contains bottomRPP as the required physical properties..
  // ---------------------------------------------------------------------
  Context* result = shareContext(childIndex, rppForChild,
                                 myContext->getInputPhysicalProperty(),
                                 costLimit,
                                 myContext, myContext->getInputLogProp());

  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, planNumber, result);

  return result;

} // MapValueIds::createContextForAChild()

//<pb>
//==============================================================================
//  Synthesize physical properties for MapValueIds operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
MapValueIds::synthPhysicalProperty(const Context* myContext,
                                   const Lng32     planNumber,
                                   PlanWorkSpace  *pws)
{
  const PhysicalProperty* const sppOfChild =
                      myContext->getPhysicalPropertyOfSolutionForChild(0);

  PartitioningFunction* actualPartFunc;

  // ---------------------------------------------------------------------
  // Rewrite the partitioning keys in terms of the values that appear
  // above this MapValueIds.
  // ---------------------------------------------------------------------
  NABoolean mapItUp = TRUE;

  ValueIdList newSortKey;
  PartitioningFunction* oldDp2SortOrderPartFunc =
    sppOfChild->getDp2SortOrderPartFunc();
  PartitioningFunction* newDp2SortOrderPartFunc =
    oldDp2SortOrderPartFunc;

  // ---------------------------------------------------------------------
  // map the child value ids to the output value ids
  // ---------------------------------------------------------------------
  map_.rewriteValueIdListUp(newSortKey,sppOfChild->getSortKey());

  if ((oldDp2SortOrderPartFunc != NULL) AND
      (NOT oldDp2SortOrderPartFunc->getPartitioningKey().isEmpty()))
  {
    newDp2SortOrderPartFunc =
      oldDp2SortOrderPartFunc->copyAndRemap(map_,mapItUp);
  }

  actualPartFunc =
    sppOfChild->getPartitioningFunction()->copyAndRemap(map_, mapItUp);

  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(myContext,
                                                             planNumber,
                                                             pws);

  PhysicalProperty* sppForMe =
    new (CmpCommon::statementHeap())
      PhysicalProperty(*sppOfChild,
                       newSortKey,
                       sppOfChild->getSortOrderType(),
                       newDp2SortOrderPartFunc,
                       actualPartFunc);

  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  delete sppTemp;
  return sppForMe;

} // MapValueIds::synthPhysicalProperty()

//<pb>

// -----------------------------------------------------------------------
// Helper methods for leaf operators
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Helper method for synthDP2PhysicalProperty.
// -----------------------------------------------------------------------
static void
computeDP2CostDataThatDependsOnSPP(
       PartitioningFunction &physicalPartFunc //in/out
       ,DP2CostDataThatDependsOnSPP &dp2CostInfo //out
       ,const IndexDesc& indexDesc
       ,const ScanKey& partKey
       ,GroupAttributes &scanGroupAttr
       ,const Context& myContext
       ,NAMemory *heap
       ,const RelExpr& scan
       )

{
  // -----------------------------------------------------------------------
  // Estimate CPUs executing DP2s:
  // -----------------------------------------------------------------------

  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  NABoolean isHbaseTable = indexDesc.getPrimaryTableDesc()->getNATable()->isHbaseTable();

  NABoolean fakeEnv = FALSE; // do not care
  CostScalar totalCPUsExecutingDP2s = defs.getTotalNumOfESPsInCluster(fakeEnv);
  if(!isHbaseTable)
  {
     // seabed api doesn't return audit count
     totalCPUsExecutingDP2s--; // do not count the system volume
     totalCPUsExecutingDP2s = MAXOF(totalCPUsExecutingDP2s,1.);
  }

  CostScalar activePartitions =
    ((NodeMap *)(physicalPartFunc.getNodeMap()))->getNumActivePartitions();

  //  Assume at least one DP2 volume even if node map indicates otherwise.
  Lng32 numOfDP2Volumes =
    MIN_ONE(((NodeMap *)(physicalPartFunc.getNodeMap()))->getNumOfDP2Volumes());

  //  The number of cpus executing DP2's cannot be more than the number
  //  of active partitions :
  Lng32 cpusExecutingDP2s  =
    MINOF((Lng32)totalCPUsExecutingDP2s.getValue(),
          (Lng32)activePartitions.getValue());

  //  The number of cpus executing DP2's cannot be more than the number
  //  of DP2 volumes:
  if (!isHbaseTable)
     cpusExecutingDP2s  = MINOF(cpusExecutingDP2s, numOfDP2Volumes);

  dp2CostInfo.setCountOfCPUsExecutingDP2s(cpusExecutingDP2s);



  // -----------------------------------------------------------------------
  // Set default estimation for repeat count. Then, refine this estimate
  // if possible below
  // -----------------------------------------------------------------------
  dp2CostInfo.setRepeatCountForOperatorsInDP2(
     (myContext.getInputLogProp()->getResultCardinality()).minCsOne());

   // check if we are doing updates
  if (scan.getOperator().match(REL_ANY_LEAF_GEN_UPDATE) ||
      scan.getOperator().match(REL_ANY_UNARY_GEN_UPDATE) )

     dp2CostInfo.setRepeatCountState
       (DP2CostDataThatDependsOnSPP::UPDATE_OPERATION);

  // only do this code if we have more than one partition:
  if (physicalPartFunc.getNodeMap()->getNumEntries() > 0)
    {
      // ------------------------------------------------------------
      // only detect AP for range partitioning (for now)
      // Also, only do this for Scans (not for update, nor insert, nor
      // delete)
      // ------------------------------------------------------------

      if ( physicalPartFunc.isARangePartitioningFunction() AND
           ( (scan.getOperatorType() == REL_FILE_SCAN) OR
             (scan.getOperatorType() == REL_HBASE_ACCESS)
           )
         )
        {

          // ------------------------------------------------------------
          // ESTIMATE ACTIVE PARTITIONS:
          // and only estimate if there actually are any predicates
          // over the leading part. key column
          // ------------------------------------------------------------

          // Get the key columns from the partKey since the
          // part key in the part. func. contains veg references.
          const ValueIdList& partKeyList = partKey.getKeyColumns();

          ColumnOrderList keyPredsByCol(partKeyList);

          // This only works for the case of single disjunct.
          // This is ok as far as the partkey is a search key.
          // $$$ When we add support for Mdam to the PA this
          // $$$ will need to change to support several
          // $$$ disjuncts. See AP doc for algorithm.
          CMPASSERT(partKey.getKeyDisjunctEntries() == 1);
          partKey.getKeyPredicatesByColumn(keyPredsByCol,0);

          //But we only care about the leading column now...
          // $$$ Revisit when multicol. hists. are added
          const ValueId& leadingColumn = partKeyList[0];
          ValueIdSet keyPredsForLeadingColumn =
            keyPredsByCol.getPredicatesForColumn(leadingColumn);
          // Note that there can be more than one predicate for
          // the leading column (as in a range)
          // but if there are no predicates then we just return
          // (default estimates were set above)
          if  (keyPredsForLeadingColumn.isEmpty())
            return;

          // Obtain  a new node map to decorate:
          NodeMap *newNodeMapPtr =
            physicalPartFunc.getNodeMap()->copy(heap);

          // Run estimation algorithm:

          CostScalar probes =
            myContext.getInputLogProp()->getResultCardinality();

          RangePartitioningFunction *prpfPtr =
            (RangePartitioningFunction *) &physicalPartFunc;


          // ------------------------------------------------------------
          // Obtain distribution for leading key of part. column:
          // ------------------------------------------------------------


          Histograms leadColHist(heap);
          const ColStatDescList &primaryTableCSDL =
            indexDesc.getPrimaryTableDesc()->getTableColStats();

          // This code is based on method
          // void
          // IndexDescHistograms::appendHistogramForColumnPosition(
          // const CollIndex& columnPosition)
          // on ScanOptimizer.h/cpp
          // However here the situation is a lot simpler
          // because we don't need to synchronize histograms
          CollIndex i;
          // AQ02-009 ATOMIC TEST SUITE Costing Anomaly Fix
          ColStatsSharedPtr colStats;
          if (primaryTableCSDL.getColStatDescIndexForColumn(i, // out
                                                            leadingColumn))
            {
              leadColHist.append(primaryTableCSDL[i]);
              colStats = primaryTableCSDL[i]->getColStats();
            }
          else
            {
//              CMPABORT; // there must exist a CSD for every part. col!
              // Deletes don't work because of the
              // Delete.IndexDesc/Scan.IndexDesc->DeleteCursor(
              //   Delete.IndexDesc, Scan.PartKey) problem
              return;
            }
          if( colStats->isOrigFakeHist() )
             return;

          // Are any histograms fake?
          // AQ02-009 ATOMIC TEST SUITE Costing Anomaly Fix
          // PROBE COUNT is estimated wrong if predicates search key is beyond
          // the range of lowerbound and upper bounds of search key's value in
          // the table.

          const ColStatDescList & colStatDescList = myContext.getInputLogProp()->getColStats();
          for ( i = 0; i < colStatDescList.entries(); i++ )
          {
              if ( colStatDescList[i]->getColStats()->isOrigFakeHist() )
              {
                  return;
              }
          }



          // ------------------------------------------------------------
          // Apply predicates for leadingColumn:
          // ------------------------------------------------------------
	  const SelectivityHint * selHint = indexDesc.getPrimaryTableDesc()->getSelectivityHint();
	  const CardinalityHint * cardHint = indexDesc.getPrimaryTableDesc()->getCardinalityHint();

	  OperatorTypeEnum opType = ITM_FIRST_ITEM_OP;
          if ( (scan.getOperatorType() == REL_FILE_SCAN) OR
               (scan.getOperatorType() == REL_HBASE_ACCESS))
	     opType = REL_SCAN;

          if ( (probes.isGreaterThanZero())
               AND
               (myContext.getInputLogProp()->getColStats().entries() > 0) )
            {
              // NJ case:
              leadColHist.applyPredicatesWhenMultipleProbes(
                   keyPredsForLeadingColumn
                   ,*(myContext.getInputLogProp())
                   ,scanGroupAttr.getCharacteristicInputs()
                   ,FALSE // MDAM irrelevant here
		           ,selHint
		           ,cardHint
				   ,NULL
				   ,opType
                   );
            }
          else
            {
              leadColHist.applyPredicates(keyPredsForLeadingColumn, scan, selHint, cardHint, opType) ;
              if (leadColHist.getRowCount() == 1)
                {
                  newNodeMapPtr->setNumActivePartitions(1);
                }
            }

          // At this point, leadColHist has the modified histograms

          // ------------------------------------------------------------
          // Now create the partition histograms:
          // ------------------------------------------------------------
          PartitionKeyDistribution partKeyDist(
               *prpfPtr
               ,leadColHist.getColStatDescList()
               );

          DCMPASSERT(partKeyDist.isValid());
          // We cannot procede with an invalid part key dist, but
          // it's not a good idea to abort, a bad plan is better
          // than no plan
          if (NOT partKeyDist.isValid())
          {
            return;
          }


          // ------------------------------------------------------------
          // Traverse the part. histogram to find out the AP,
          // set the part. state accordingly
          // ------------------------------------------------------------
          CollIndex numParts = partKeyDist.getNumPartitions();
          DCMPASSERT(numParts
                     ==
                     newNodeMapPtr->getNumEntries());

          for (i=0; i < numParts; i++)
            {
              // NB: we used to only count those with >1 rows; however,
              // for most cases, a rowcount that's at all non-zero means
              // that the partition is active.
              if (partKeyDist.getRowsForPartition(i) > 0.)
                newNodeMapPtr->setPartitionState(i,
                  NodeMapEntry::ACTIVE);
              else
                newNodeMapPtr->setPartitionState(i,
                  NodeMapEntry::NOT_ACTIVE);
            }

          //--------------------------------------------------------------------
          // If key predicate for leading column has an equality predicate
          // on a host variable or parameter, then set the maximum
          // number of active partition at runtime estimate to the max partition
	  // factor of the partition distribution key.
	  // Currently this variable if set will be used only for costing of scans.
          // ------------------------------------------------------------------
          if (keyPredsForLeadingColumn.referencesAHostvariableorParam())
                newNodeMapPtr->setEstNumActivePartitionsAtRuntime(partKeyDist.getMaxPartitionFactor());

// done!, replace new map in existing part func:
          physicalPartFunc.replaceNodeMap(newNodeMapPtr);


          // -------------------------------------------------------------------
          // Estimate the RC
          // -------------------------------------------------------------------


          // Find out the highest column covered by an equijoin

          Lng32 highestColumn = 0;
          const ValueIdSet &inputValues =
            scanGroupAttr.getCharacteristicInputs();

          const ValueIdSet& operatorValues =
                indexDesc.getIndexKey();

          ValueId firstPredId;
          for (i=0; i < partKeyList.entries(); i++)
            {
              ValueId currentColumn = partKeyList[i];

              ValueIdSet keyPredsForRC =
                keyPredsByCol.getPredicatesForColumn(currentColumn);
              if (keyPredsForRC.isEmpty())
                break;

              // find a predicate in the set that is an equijoin pred:
              NABoolean found = FALSE;
              for (ValueId predId = keyPredsForRC.init();
                   NOT found AND keyPredsForRC.next(predId);
                   keyPredsForRC.advance(predId))
                {
                  // does it cover the column?
                  ItemExpr *predIEPtr = predId.getItemExpr();
                  if (predIEPtr->
                      isANestedJoinPredicate(inputValues, operatorValues))
                    {
                      // is it en equijoin?
                      if (predIEPtr->getOperatorType() == ITM_VEG_PREDICATE
                          OR
                          predIEPtr->getOperatorType() == ITM_EQUAL)
                        {
                          if (i==0)
                            {
                              // save first equijoin for the estimation
                              // of affected partitions
                              firstPredId = predId;
                            }
                          highestColumn++;
                          found = TRUE;
                        } // if equijoin

                    } // if nested join pred
                } // for every pred for current column
              if (NOT found)
                {
                  // we found a column not covered, do not
                  // continue
                  break;
                }
            } // for every column


          dp2CostInfo.setHighestLeadingPartitionColumnCovered(highestColumn);


          if (highestColumn > 0)
            {
              // Obtain distribution for leading key of part. column:

              // Reuse the partKeyHist:
              // the synthesis for AP:

              Lng32 affectedPartitions =
                newNodeMapPtr->getNumActivePartitions();

              // Now estimate the RC:

              // minRC would be the RC if all the columns were
              // covered
              CostScalar minRC = (probes/affectedPartitions).getCeiling();

              // If not all partitions are covered, minRC must be
              // multiplied by a "fanout". The "fanout" represents
              // the number of partitions each probe goes to.
              // our first estimate for the fanout is the maximum
              // number of partitions sharing the same boundary
              // value for the first column:
              CostScalar fanout = partKeyDist.getMaxPartitionFactor();

              // But if more columns of the partition key are covered
              // then the fanout must decrease. Compute that using
              // formula below which interpolates given the number
              // of columns in the key and the highest column covered.
              // Note that the fanout factor
              // becomes one when all columns are covered,
              // and it becomes "fanout" when only the first column is covered
              // note that if we are here then highestColumn >= 1
              CostScalar numPartKeyColumns = partKeyList.entries();
              CostScalar fanoutRatio =
                CostScalar(highestColumn-1)/(numPartKeyColumns-1);
              CostScalar fanoutFactor =
                (fanout * (csOne - fanoutRatio)).minCsOne();

              CostScalar RC = ((minRC * fanoutFactor));

              dp2CostInfo.setRepeatCountForOperatorsInDP2(RC.minCsOne());

            } // if at least the first col. is covered by an equijoin

        } // if we have range partitioning
      else
        if ( ( physicalPartFunc.isATableHashPartitioningFunction() AND
               (scan.getOperatorType() == REL_FILE_SCAN)
             ) OR
             ( indexDesc.isPartitioned() AND
               (scan.getOperatorType() == REL_HBASE_ACCESS) AND
               (CmpCommon::getDefault(NCM_HBASE_COSTING) == DF_ON))
           )
        {

          // ------------------------------------------------------------
          // The inner table is hash-partitioned.
	  // For details on the logic within this IF block, please read
	  // "Support for hash-partitioned tables in method
	  // computeDP2CostDataThatDependsOnSPP" by Sunil Sharma.
          // ------------------------------------------------------------

          // Obtain access to the node-map
          NodeMap *NodeMapPtr = (NodeMap *) physicalPartFunc.getNodeMap();

          const ValueIdSet &inputValues =
            scanGroupAttr.getCharacteristicInputs();

          const ValueIdSet& operatorValues =
                indexDesc.getIndexKey();


	  //-------------------------------------------------------------------
          // Determine number of partkey columns covered by an nested-join pred.
	  //  or by a constant expr.
	  // Determine the number of partkey columns covered by constant
	  //  expressions alone.
	  //-------------------------------------------------------------------

          // Get the key columns from the partKey since the
          // part key in the part. func. contains VEG references.
          const ValueIdList& partKeyList = partKey.getKeyColumns();

          ColumnOrderList keyPredsByCol(partKeyList);

          // This only works for the case of single disjunct.
          // This is ok as far as the partkey is a search key.
          // $$$ When we add support for Mdam to the PA this
          // $$$ will need to change to support several
          // $$$ disjuncts.
          CMPASSERT(partKey.getKeyDisjunctEntries() == 1);

	  // populate keyPredsByCol with predicates
	  partKey.getKeyPredicatesByColumn(keyPredsByCol);

          ULng32 keyColsCoveredByNJPredOrConst = 0;
	  ULng32 keyColsCoveredByConst = 0;

	  // iterate over all partition-key columns
          ValueId firstPredId;
          for (CollIndex i=0; i < partKeyList.entries(); i++)
            {
              ValueId currentColumn = partKeyList[i];

              ValueIdSet keyPredsForCurCol =
                keyPredsByCol.getPredicatesForColumn(currentColumn);
              if (keyPredsForCurCol.isEmpty())
		{
		  break;
		}

              // find a predicate in the set that is a nested-join pred or
	      //  involves a constant expression.
              // find a predicate in the set that involves a constant.
              NABoolean foundNJPredOrConstPred = FALSE;
              NABoolean foundConstPred = FALSE;
              for (ValueId predId = keyPredsForCurCol.init();
                   keyPredsForCurCol.next(predId);
                   keyPredsForCurCol.advance(predId))
                {
                  ItemExpr *predIEPtr = predId.getItemExpr();
                  if ((predIEPtr->getOperatorType() == ITM_VEG_PREDICATE)
		      OR
		      (predIEPtr->getOperatorType() == ITM_EQUAL))
                    {
		      // This pred is an equi-join pred or a constant pred.
		      // If this pred is an equi-join pred, then ensure that
		      // it links the current/inner
		      // table with the outer composite,i.e. that it is a
		      // nested-join pred.

		      // if needed, determine whether the pred is a constant pred
		      NABoolean isAConstPred = FALSE;

		      if ((NOT foundConstPred) OR (NOT foundNJPredOrConstPred))
			{
			  // does the pred cover the column with a constant expression
			  // (i.e. a constant value, a host-var or a param)?

			  // is the predicate a  VEG-predicate?
			  if (predIEPtr->getOperatorType() == ITM_VEG_PREDICATE)
			    {
			      const VEG * predVEG=
				((VEGPredicate*)predIEPtr)->getVEG();
			      // Now, get all members of the VEG group
			      const ValueIdSet & VEGGroup=predVEG->getAllValues();

			      if (VEGGroup.referencesAConstExpr())
				{
				  if (NOT foundConstPred)
				    {
				      keyColsCoveredByConst ++;
				      foundConstPred = TRUE;
				    }
				  isAConstPred = TRUE;
				}
			    }
			  else
			    {
			      // Pred uses binary relational operator ITM_EQUAL.
			      // (It's not a VEG predicate but an ItemExpr.)

			      const ItemExpr *leftExpr  = predIEPtr->child(0);
			      const ItemExpr *rightExpr = predIEPtr->child(1);

			      //Check if the other operand of
			      //the equality condition is a non-strict constant.
			      //A strict constant is something like cos(1), CAST(1),
			      //whereas cos(?p), CAST(?p) can be considered a constant
			      //in the non-strict sense since they remain
			      //constant for a given execution of a query.
			      if ( leftExpr->doesExprEvaluateToConstant(FALSE) OR
				   rightExpr->doesExprEvaluateToConstant(FALSE) )
				{
				  if (NOT foundConstPred)
				    {
				      keyColsCoveredByConst ++;
				      foundConstPred = TRUE;
				    }
				  isAConstPred = TRUE;
				}
			    }
			}

		      if ((NOT foundNJPredOrConstPred)
			  && (isAConstPred ||
			      (predIEPtr->
			       isANestedJoinPredicate(inputValues, operatorValues))))
			{
			  keyColsCoveredByNJPredOrConst ++;
			  foundNJPredOrConstPred = TRUE;
			}

		      if (foundNJPredOrConstPred && foundConstPred)
			// we're done with this partitioning key column;
			// end the iteration over its predicates
			break;
		    }
		}
	    }


          CollIndex numParts = 1;
          if (scan.getOperatorType() == REL_HBASE_ACCESS)
            numParts = indexDesc.getPartitioningFunction()->getCountOfPartitions();
          else
            numParts = physicalPartFunc.getCountOfPartitions();

	  // The order of the IF conditions in the following statement, is
	  // CRITICAL. It is possible that partitioning-key list may be
	  // fully covered by constant expressions and also by some combination
	  // of equijoin predicates and constant expressions. If so, the first
	  // condition takes precedence during query execution in that if it's
	  // true, then all probes are routed to one specific partition. This
	  // precedence is reflect in the ordering of the IF conditions below.

	  if (keyColsCoveredByConst == partKeyList.entries())
	    {
	      // All inner table partitioning key columns are covered by
	      // constant expr. Hence, all outer probes go to one specific
	      // inner table partition, which is the only active partition.
	      dp2CostInfo.setRepeatCountForOperatorsInDP2(
                (myContext.getInputLogProp()->
                 getResultCardinality()).minCsOne()
              );

              dp2CostInfo.setRepeatCountState
                 (DP2CostDataThatDependsOnSPP::KEYCOLS_COVERED_BY_CONST);

	      // If all partition-key columns are covered by constant values
	      // (not host-vars or params), then we can determine the one
	      // active partition by computing the hash-partitioning function
	      // (using the constant-folding technique) and can set the active
	      // partition bitmap appropriately. However, my investigation shows
	      // that this bitmap is used only for range-partitioned tables.
	      // Moreover, query-caching (in ODBC/JDBC & in MXCMP) replaces
	      // constant values in equality predicates with param. Hence, the
	      // likelihood that all partition-key columns are covered by
	      // constants is low. These factors reduce the value of or the
	      // need for accurately setting the active bitmap in the nodemap
	      // for hash-partitioned tables. The bitmap will be left as-is.

	      // Indicate in the node map that only one partition will be
	      // accessed during plan evaluation.
	      NodeMapPtr->setEstNumActivePartitionsAtRuntime(1);
	    }
	  else if (keyColsCoveredByNJPredOrConst == partKeyList.entries())
	    {
	      // All inner table partitioning key columns are covered by
	      // equijoin predicates or constant expressions.
	      // Hence, each outer probe goes to a single
	      // inner table partition.

              if (CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting() AND
                  physicalPartFunc.isATableHashPartitioningFunction())
              {
                 CostScalar probesAtBusyStream =
                     myContext.getInputLogProp()->getCardOfBusiestStream(
                                      &physicalPartFunc,
                                      numParts,
                                      &scanGroupAttr,
                                      numParts,
                                      TRUE);

                 dp2CostInfo.setProbesAtBusiestStream(probesAtBusyStream);
              }

	      dp2CostInfo.setRepeatCountForOperatorsInDP2
		(
		   (myContext.getInputLogProp()->getResultCardinality()
			  /numParts).getCeiling().minCsOne()
		 );

              dp2CostInfo.setRepeatCountState
               (DP2CostDataThatDependsOnSPP::KEYCOLS_COVERED_BY_PROBE_COLS_CONST);

	      // indicate in the node map that all partitions will be
	      // accessed during plan evaluation.
	      NodeMapPtr->setEstNumActivePartitionsAtRuntime(numParts);
	    }
	  else
	    {
	      // The default applies: Each outer probe goes to all inner
	      // partitions.
	      dp2CostInfo.setRepeatCountForOperatorsInDP2(
                (myContext.getInputLogProp()->
                 getResultCardinality()).minCsOne()
              );

              dp2CostInfo.setRepeatCountState(
                DP2CostDataThatDependsOnSPP::KEYCOLS_NOT_COVERED);

	      // indicate in the node map that all partitions will be
	      // accessed during plan evaluation.
	      NodeMapPtr->setEstNumActivePartitionsAtRuntime
		(numParts);
	    };

	  }; // inner table is hash-partitioned


    }// If we have more than one partition
} // computeDP2CostDataThatDependsOnSPP()



//<pb>
// -----------------------------------------------------------------------
// Helper method for DP2 cursor operators (scan, cursor ins/upd/del)
//
// This method interprets the partitioning requirements, the type of
// the operator, and the physical partitioning function of the file
// and comes up with a synthesized partitioning function. We are using
// a standalone procedure because at this time there is no common
// base class (other than RelExpr) among DP2 scan and DP2 updates.
//
// This method decides things that relate to the DP2 operator and to
// the DP2 exchange above it. Decisions are sent to the DP2 exchange
// via a special "LogPhysPartitioningFunction" object that is only
// used in DP2.
//
// Here is what this method decides:
//
// - the "logical partitioning function", meaning the top partitioning
//   function of the DP2 exchange above,
// - the type of logical partitioning used (for documentation see class
//   LogPhysPartitioningFunction in file PartFunc.h),
// - how many PAs (total among all client processes) will be used and
//   whether a PAPA node will be used in the executor process(es),
// - how many executor processes will be used (equal to the number
//   of logical partitions, unless we do load balancing).
//
// There are certain constraints with special situations, such as
// VSBB inserts and merging of sorted streams, which may require one PA
// per DP2 partition in each executor process.
// -----------------------------------------------------------------------

PhysicalProperty * RelExpr::synthDP2PhysicalProperty(
                     const Context*     myContext,
                     const ValueIdList& sortOrder,
                     const IndexDesc*   indexDesc,
                     const SearchKey*   partSearchKey)
{
  // my required phys props (always non-NULL)
  const ReqdPhysicalProperty* rppForMe =
                                     myContext->getReqdPhysicalProperty();

  // the result
  PhysicalProperty *sppForMe;

  // variables that help to come up with my partitioning function

  const LogicalPartitioningRequirement *lpr =
    rppForMe->getLogicalPartRequirement();
  PartitioningRequirement * logPartReq = NULL;

  Lng32      numPAs  = ANY_NUMBER_OF_PARTITIONS;
  Lng32      numEsps = 1;
  NABoolean usePapa = FALSE;
  NABoolean shouldUseSynchronousAccess = FALSE;
  NABoolean mergeOfSortedStreams = FALSE;
  NABoolean numPAsForced = FALSE;
  PlanExecutionEnum location = EXECUTE_IN_DP2;

  // get the maximum number of access nodes per process that can be allowed from
  // MAX_ACCESS_NODES_PER_ESP. This is an absolute value
  // that should be based on file system and executor buffer size
  // restrictions, message system restrictions, etc.
  Int32 maxPAsPerProcess =
    (Int32) getDefaultAsLong(MAX_ACCESS_NODES_PER_ESP);

  LogPhysPartitioningFunction::logPartType logPartType;
  PartitioningFunction * physicalPartFunc =
    indexDesc->getPartitioningFunction();
  ValueIdList physicalClusteringKey =
    indexDesc->getOrderOfKeyValues();
  PartitioningFunction * logicalPartFunc = NULL;
  PartitioningFunction * logPhysPartFunc = NULL;

  // convert a non-existent part func to a single part func
  if (physicalPartFunc == NULL)
    {
      NodeMap* nodeMap = indexDesc->getNAFileSet()
                                       ->getPartitioningFunction()
                                       ->getNodeMap()
                                       ->copy(CmpCommon::statementHeap());
      physicalPartFunc = new(CmpCommon::statementHeap())
        SinglePartitionPartitioningFunction(nodeMap);

    }

  // -----------------------------------------------------------------------
  // Vector to put all costing data that is computed at synthesis time
  // Make it a local variable for now. If we ever reach the end of
  // this routine create a variable from the heap, initialize it with this,
  // and then set the sppForMe slot.
  // -----------------------------------------------------------------------
  DP2CostDataThatDependsOnSPP dp2CostInfo;
  // ---------------------------------------------------------------------
  // Estimate the number of active partitions and other costing
  // data that depends on SPP:
  // ---------------------------------------------------------------------
  computeDP2CostDataThatDependsOnSPP(*physicalPartFunc // in/out
                                     ,dp2CostInfo //out
                                     ,*indexDesc // in
                                     ,*partSearchKey // in
                                     ,*getGroupAttr() //in
                                     ,*myContext // in
                                     ,CmpCommon::statementHeap() // in
                                     , *this
                                     );

  Lng32 currentCountOfCPUs = dp2CostInfo.getCountOfCPUsExecutingDP2s();

  // ---------------------------------------------------------------------
  // determine the logical partitioning type
  // ---------------------------------------------------------------------
  if (lpr)
  {
    logPartReq  = lpr->getLogReq();
    logPartType = lpr->getLogPartTypeReq();
    numPAs      = lpr->getNumClientsReq();
    usePapa     = lpr->getMustUsePapa();

    if ( lpr->getNumClientsReq() == 1 &&
         lpr->isNumberOfPAsForced() &&
         (CmpCommon::getDefault(ATTEMPT_ASYNCHRONOUS_ACCESS) == DF_ON)
       )
    {
      // number of PAs are being forced through CQS. This should be number
      // of PA nodes over all the partitions.
      numPAs= MINOF( currentCountOfCPUs,
                     physicalPartFunc->getCountOfPartitions()
                   );

      // the following is needed to make sure that subsequent calls to
      // to shouldUseSynchronousAccess() do get the right number of PAs
      // this should be set ideally in Exchange::processCQS(), but there
      // the number of partitions is not available
      LogicalPartitioningRequirement *pr=(LogicalPartitioningRequirement *)lpr;
      pr->setNumClientsReq(numPAs);
    }
    // TEMPORARY CODE
    // The code cannot handle parallelism for vertically partitioned
    // tables right now. Also, the code cannot handle parallelism for
    // tables with float columns in the partitioning key.
    // So, if someone wants parallelism they are out of luck until
    // the problems are fixed.
    if ((physicalPartFunc->isARoundRobinPartitioningFunction() OR
         physicalPartFunc->partKeyContainsFloatColumn()) AND
        ((logPartReq == NULL) OR
         NOT (logPartReq->isRequirementExactlyOne() OR
              logPartReq->isRequirementReplicateNoBroadcast())))
      return NULL;

    // The synthesized partitioning function needs to satisfy the
    // partitioning requirements in the context. This would be an
    // argument for considering these requirements when calling
    // realize() below. However, the result of realize() is not
    // the synthesized partitioning function and therefore doesn't
    // have to satisfy those requirements!
    //
    // This is a tricky problem, but for now we just hope that not
    // too many physical partitioning requirements will be generated
    // by DP2 operators.
    NABoolean considerPhysicalReqs = FALSE;

    // try to realize the logical partitioning requirement in the way
    // that is most similar to the physical partitioning function
    if (logPartReq)
    {
      logicalPartFunc = logPartReq->realize(
           myContext,
           considerPhysicalReqs,
           physicalPartFunc->makePartitioningRequirement());
      logicalPartFunc->createPartitioningKeyPredicates();
    }

    // -----------------------------------------------------------------
    // Determine the type of logical partitioning.
    // -----------------------------------------------------------------
    // check for partition grouping first
    if (logicalPartFunc == NULL OR
        logicalPartFunc->isAGroupingOf(*physicalPartFunc) OR
        (logPartReq AND logPartReq->castToRequireReplicateNoBroadcast()))
    {
      if (logPartType ==
          LogPhysPartitioningFunction::ANY_LOGICAL_PARTITIONING)
      {
        // Partition grouping is the preferred way of doing things
        // when there is no specific requirement or when the
        // requirement allows it or when we are replicating data.
        logPartType = LogPhysPartitioningFunction::PA_PARTITION_GROUPING;
        //                                         =====================
      }
    }
    else if (logPartType ==
             LogPhysPartitioningFunction::PA_PARTITION_GROUPING)
    {
      // trying to force grouping for a case where grouping doesn't work
      return NULL;
    }

    // If we didn't pick partition grouping, try logical subpartitioning
    if (logPartType !=
        LogPhysPartitioningFunction::PA_PARTITION_GROUPING AND
        logicalPartFunc AND
        logicalPartFunc->canProducePartitioningKeyPredicates())
    {
      // check whether it would be beneficial to apply the partitioning
      // key predicates to the scan node
      ValueIdSet pkp = logicalPartFunc->getPartitioningKeyPredicates();
      const RangePartitioningFunction *rpfFromRequirement;

      if (logPartReq->castToRequireRange())
        rpfFromRequirement = logPartReq->castToRequireRange()->
          getPartitioningFunction()->castToRangePartitioningFunction();
      else
        rpfFromRequirement = NULL;

      // Test whether the partitioning key columns of a required
      // range partitioning scheme (if any) are a leading prefix
      // of and in the same order as the physical clustering
      // key columns, and either the physical partitioning
      // function is a SinglePartitionPartitioningFunction, or
      // the physical partitioning function is a range partitioning
      // function and the partitioning key columns are a leading
      // prefix of the clustering key columns.
      // Choose logical subpartitioning if this is indeed the case.
      // Note that we could some day allow an INVERSE_ORDER
      // result, too, because the partitioning key predicates
      // would work on the inverse order just as well.
      if (rpfFromRequirement AND
          (physicalClusteringKey.satisfiesReqdOrder(
            rpfFromRequirement->getOrderOfKeyValues()) == SAME_ORDER) AND
          (physicalPartFunc->isASinglePartitionPartitioningFunction() OR
          (physicalPartFunc->isARangePartitioningFunction() AND
            (physicalClusteringKey.satisfiesReqdOrder(
              physicalPartFunc->
                castToRangePartitioningFunction()->
                getOrderOfKeyValues()) == SAME_ORDER)))
         )
      {
        logPartType =
          LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING;
          //                           =======================
      }
      else if (logPartType ==
               LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING)
      {
        // trying to force subpartitioning where it doesn't work
        return NULL;
      }
      else
      {
        // should check whether applying the part key preds pkp
        // results in a good key pred selectivity ||opt
        //logPartType =
        //LogPhysPartitioningFunction::HORIZONTAL_PARTITION_SLICING;
        //                             ============================
      }
    }

    if (logPartType ==
        LogPhysPartitioningFunction::ANY_LOGICAL_PARTITIONING)
    {
      // nothing worked, give up and hope for a double exchange
      return NULL;
    }
  }
  else
  {
    // no logical partitioning requirement, choose a simple scheme
    logPartType = LogPhysPartitioningFunction::PA_PARTITION_GROUPING;
    //                                         =====================
  }

  // ---------------------------------------------------------------------
  // at this point we have chosen logPartType, now determine the
  // number of PA clients to be used and whether to use a PAPA
  // ---------------------------------------------------------------------

  // see if user wants us to base the number of PAs on the number of
  // active partitions
  NABoolean baseNumPAsOnAP = FALSE;

  if (CmpCommon::getDefault(BASE_NUM_PAS_ON_ACTIVE_PARTS) == DF_ON)
    baseNumPAsOnAP = TRUE;

  // Get the number of active partitions - used to limit the # of PAs
  // if the above defaults entry says it is ok.
  CostScalar activePartitions =
    ((NodeMap *)(physicalPartFunc->getNodeMap()))->getNumActivePartitions();

  // -----------------------------------------------------------------------
  // Determine number of PAs to use
  // -----------------------------------------------------------------------
  // Calculating the number of PA nodes is a difficult task. Here are some
  // of the factors that should influence the decision:
  //
  // - the number of physical partitions (no need to use more PAs than
  //   physical partitions in a PAPA node),
  // - the selectivity of the partitioning key predicates (influences number
  //   of active physical partitions),
  // - the distribution of DP2s over the available CPUs or nodes (influences
  //   the degree of DP2 parallelism that we can get),
  // - the logPartType (influences whether the ESPs go after the same
  //   DP2s or have distinct set of DP2s) and the number of ESPs if the
  //   ESPs access the same partitions (HORIZONTAL_PARTITION_SLICING),
  // - whether there is a required order in the DP2 exchange (requires
  //   a sufficient number of PAs to do the merge), whether it matches the
  //   clustering key and/or partitioning key, and the ratio of rows
  //   accessed in DP2 vs. rows returned by the DP2 exchange (low
  //   selectivity queries with sort can still benefit from parallelism).

  if ((numPAs != ANY_NUMBER_OF_PARTITIONS) OR
      (CmpCommon::getDefault(ATTEMPT_ASYNCHRONOUS_ACCESS) == DF_OFF))
  {
    // The number of PAs are being forced via C.Q. Shape, or
    // synchronous access is being forced via C.Q. Default.

    // if the number of PAs were not specified via C.Q. Shape,
    // then we must be forcing synchronous access.
    if (numPAs == ANY_NUMBER_OF_PARTITIONS)
      numPAs = 1;
    else // number of PAs were forced
      numPAsForced = TRUE;

    // If the user is forcing any amount of synchronous access
    // (numPAs < number of physical partitions),
    // then this could lead to incorrect results if there is a
    // required order or arrangement, the table is partitioned,
    // and the table is not range partitioned on the required
    // order or arrangement columns. Call the shouldUseSynchronousAccess
    // method. If synchronous access is ok this method will
    // return TRUE, note that this method knows if synch. access
    // is being forced. If this method returns false
    // then we must give up now.
    if (numPAs < physicalPartFunc->getCountOfPartitions())
    {
      if (physicalPartFunc->shouldUseSynchronousAccess(
                 rppForMe,myContext->getInputLogProp(),getGroupAttr()))
        shouldUseSynchronousAccess = TRUE;
      else
        return NULL;
    }
  } // end if forcing number of PAs or asynchronous access
  else if (physicalPartFunc->shouldUseSynchronousAccess(
             rppForMe,myContext->getInputLogProp(),getGroupAttr()))
  {
    shouldUseSynchronousAccess = TRUE;
    // For synchronous access, set the numPAs to 1. This is really
    // the number of PAs we need PER PROCESS. But, since numPAs
    // reflects the total number of PAs for all processes, it needs
    // to be scaled up by the number of processes. This will be
    // done after we compute the number of ESPs.
    numPAs = 1;
  }
  else // # of PAs not forced and no synchronous access
  {
    // Set the number of PAs to the number of active partitions if
    // allowed by the defaults table.
    // Don't limit the # of PAs by the number of DP2 volumes (yet).
    // We must be very careful about limiting the number of PAs if
    // a merge of sorted streams is required. A merge of sorted streams
    // cannot allow any kind of synchronous access, or the wrong
    // answer might be returned. So, if we are going to do a
    // merge of sorted streams, there must be one PA for every
    // TRULY active partition. Since we can't guarantee the active
    // partition estimate, set the number of PAs to the number of
    // physical partitions if a merge of sorted streams is necessary.
    if (NOT baseNumPAsOnAP OR
        (rppForMe->getLogicalOrderOrArrangementFlag() AND
         NOT physicalPartFunc->isASinglePartitionPartitioningFunction()))
      numPAs = physicalPartFunc->getCountOfPartitions();
    else
      numPAs = (Lng32)activePartitions.getValue();
  }

  // Now that we know if synchronous access will be done, see if
  // we will need to do a merge of sorted streams.
  // We will need a merge of sorted streams if there is a logical
  // order or arrangement requirement, the required order does
  // not require a DP2 sort order type (no merge of sorted streams
  // is necessary to satisfy a DP2 sort order type requirement),
  // and we are not accessing all partitions synchronously.
  mergeOfSortedStreams =
    rppForMe->getLogicalOrderOrArrangementFlag() AND
    (rppForMe->getDp2SortOrderPartReq() == NULL) AND
    (numPAs != 1);

  // -----------------------------------------------------------------------
  // Determine the number of processes (ESPs or master) used. That number
  // must be identical to the number of partitions in the logical
  // partitioning function (if any).
  // -----------------------------------------------------------------------
  if ( logicalPartFunc AND logPartReq AND
       (logPartReq->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS)
     )
    {
      // the logical partitioning function, which was derived from
      // the logical partitioning requirement, has specified the
      // number of ESPs
      numEsps =
        ( (CmpCommon::getDefault(COMP_BOOL_129) == DF_ON) AND
          (CURRSTMT_OPTDEFAULTS->attemptESPParallelism() == DF_SYSTEM) AND
           logPartReq->isRequirementApproximatelyN()
        ) ? logPartReq->castToRequireApproximatelyNPartitions()->
              getCountOfPartitionsLowBound()
          : logicalPartFunc->getCountOfPartitions();
    }
  else if (numPAs == 1)
    {
      // If numPAs is 1, there must be only one partition,
      // or synchronous access is being used. So, it wouldn't
      // do us any good to have more than one ESP.
      numEsps = 1;
      logicalPartFunc = new(CmpCommon::statementHeap())
        SinglePartitionPartitioningFunction();
      logicalPartFunc->createPartitioningKeyPredicates();
    }
  else
    {
      // The DP2 exchange above us has not had any partitioning
      // requirement or has not specified a number of partitions in
      // its logical partitioning requirement. Produce a logical
      // partitioning function in a way that we get a reasonable
      // number of PAs per ESP (or in the master).  Make the
      // logical partitioning function by scaling the
      // physical partitioning function or the previously realized
      // partitioning function to numEsps partitions.

      // initialize numEsps to the number of PAs
      numEsps = numPAs;

      NABoolean numOfESPsForced = FALSE; // Not used
      float allowedDeviation = 0.0; // Not used
      if (NOT okToAttemptESPParallelism(myContext,
                                        NULL, //don't need/have pws
                                        numEsps,
                                        allowedDeviation,
                                        numOfESPsForced) OR
          (numEsps == 1)
          // TEMPORARY CODE - until vert. part tables support parallelism
          OR physicalPartFunc->isARoundRobinPartitioningFunction()
          // TEMPORARY CODE - until the executor can handle float columns
          //                  in the partitioning key
          OR physicalPartFunc->partKeyContainsFloatColumn()
         )
      {
        numEsps = 1;
        logicalPartFunc = new(CmpCommon::statementHeap())
          SinglePartitionPartitioningFunction();
      }
      else
      {
        if (logicalPartFunc == NULL)
          logicalPartFunc = physicalPartFunc->copy();
        else
          // logicalPartFunc should be the physical part function
          logicalPartFunc = logicalPartFunc->copy();

        // Now group the number of partitions down to a reasonable #
        Lng32 scaleNumOfParts = numEsps;

        // First, need to see if we need to base the parallelism on
        // the number of active partitions.
        Lng32 numActivePartitions = 1;
        if ((CmpCommon::getDefault(BASE_NUM_PAS_ON_ACTIVE_PARTS)
             == DF_ON) AND
            (logicalPartFunc->castToRangePartitioningFunction() != NULL))
        {
          CostScalar activePartitions =
            ((NodeMap *)(logicalPartFunc->getNodeMap()))->
                           getNumActivePartitions();
          numActivePartitions = (Lng32)activePartitions.getValue();
          // If we are grouping based on the number of active partitions,
          // and there won't be enough active partitions to go around,
          // then reduce the number of groups to the number of active
          // partitions. This will ensure that the scaling will work
          // and that we don't end up with more ESPs than active partitions.
          if (scaleNumOfParts > numActivePartitions)
            scaleNumOfParts = numActivePartitions;
        }

        // Actually do the grouping now
        logicalPartFunc =
          logicalPartFunc->scaleNumberOfPartitions(scaleNumOfParts);
        // Was scale able to do it's job?
        if (scaleNumOfParts != numEsps) // No
        {
          // Scaling failed, so use the # of parts
          // in the logical function, if there aren't too many partitions.
          if (logicalPartFunc->getCountOfPartitions() <=
                rppForMe->getCountOfPipelines())
            numEsps = logicalPartFunc->getCountOfPartitions();
          else // No choice left but no parallelism
          {
            numEsps = 1;
            logicalPartFunc = new(CmpCommon::statementHeap())
              SinglePartitionPartitioningFunction();
          }
        } // end if scaling failed
      }

      logicalPartFunc->createPartitioningKeyPredicates();
    }
  // We better have at least one ESP (or master)
  CMPASSERT(numEsps >= 1);
  // We must have a logical partitioning function at this point
  CMPASSERT(logicalPartFunc != NULL);

  // ---------------------------------------------------------------------
  // Perform adjustments of the number of PAs that are based on the
  // number of ESPs.
  // ---------------------------------------------------------------------
  // Can't/Don't need to adjust the number of PAs if the # is being forced
  // or if synchronous access is being done, or if there is only one
  // logical partition.
  if (NOT numPAsForced AND NOT shouldUseSynchronousAccess AND numEsps > 1)
  {
    Lng32 maxPartsPerGroup;

    if (logicalPartFunc->isAReplicateNoBroadcastPartitioningFunction())
    {

      // Does the REP-N really replicate to all partitions or is there
      // some grouping going on.  If it does replicate, then each
      // instance may access all partitions via PAs.  If there is
      // grouping, then each instance will access a non-overlapping
      // group of partitions.  So in this case the total number of PAs
      // across all instances is just the total number of partitions.
      NABoolean grouping = FALSE;

      const RequireReplicateNoBroadcast *rnbReq =
        logPartReq->castToRequireReplicateNoBroadcast();

      if(rnbReq) 
        {
          const PartitioningFunction *parentPartFunc = rnbReq->getParentPartFunc();
          Lng32 factor;
          if(parentPartFunc)
            grouping = parentPartFunc->isAGroupingOf(*physicalPartFunc, &factor);
        }

      // This is a Type-2 join, so all logical partitions might need
      // to access all physical partitions. So, each ESP needs to have all
      // the PAs, so we must multiply the number of PAs by the number of ESPs.
      // Only do this if it's not a unique scan or if we must do a merge of
      // sorted streams.
      // If there is grouping going on, then we do not need to multiply.
      if (((mergeOfSortedStreams) ||
           (CmpCommon::getDefault(COMP_BOOL_67) == DF_OFF)) && !grouping)
        numPAs = numPAs * numEsps;
    }
    else if (logicalPartFunc->isAGroupingOf(*physicalPartFunc,&maxPartsPerGroup)
             AND (logPartType ==
                    LogPhysPartitioningFunction::PA_PARTITION_GROUPING))
    {
      // Partition grouping is being done.
      // Compute the maximum number of physical partitions that would result
      // if the groups were formed based on a uniform distribution of all
      // physical partitions.
      Lng32 maxPartsPerUniformGroup =
        (physicalPartFunc->getCountOfPartitions() + numEsps - 1) / numEsps;
      if (mergeOfSortedStreams)
      {
        //   Since a merge of sorted streams is being done, each group
        // (ESP) will need to have as many PAs as the group that has
        // the most partitions, to ensure that the ESP with the most
        // partitions gets as many PAs as it has partitions.
        //   If this exceeds the maximum # of PAs allowed for a process,
        // and this grouping was the result of a call to
        // scaleNumberOfPartitions that was performed here and was
        // based on an active partition distribution, see if a uniform
        // grouping of the partitions would work. If so, call
        // scaleNumberOfPartitions again, passing a parameter that indicates
        // a uniform distribution of all physical partitions must be used.
        if ((maxPartsPerGroup > maxPAsPerProcess) AND
            ((logPartReq == NULL) OR logPartReq->isRequirementFuzzy()) AND
            (maxPartsPerUniformGroup <= maxPAsPerProcess))
        {
          logicalPartFunc = logicalPartFunc->copy(); // just in case
          logicalPartFunc->scaleNumberOfPartitions(numEsps,
            UNIFORM_PHYSICAL_PARTITION_GROUPING);
          logicalPartFunc->createPartitioningKeyPredicates();
        }
        else
          numPAs = maxPartsPerGroup * numEsps;
      }
      else
      {
        // IF this grouping was the result of a call to scaleNumberOfPartitions
        // performed here, AND was based on an active partition distribution,
        // AND there are enough inactive partitions to make it worthwhile
        // to allocate some extra PAs to handle them, then do that.
        Lng32 roundedUpNumOfPAs =
          ((numPAs + numEsps - 1) / numEsps) * numEsps;
        if (((logPartReq == NULL) OR logPartReq->isRequirementFuzzy()) AND
            (maxPartsPerUniformGroup != maxPartsPerGroup) AND
            (roundedUpNumOfPAs < physicalPartFunc->getCountOfPartitions()))
          numPAs += numEsps;
      }
    } // end if partition grouping
    else if (mergeOfSortedStreams)
    {
      // logical subpartitioning + merge of sorted streams
      // Because it is possible that ALL the physical partitions could end
      // up in one of the logical partitions (due to skew), the only safe
      // number of PAs in this case is to have (numPAs * numEsps) PAs.
      // Perhaps eventually the executor will implement allocate-PA-on-demand
      // and then the optimizer won't have to figure out how many PAs
      // are needed.
      numPAs = numPAs * numEsps;
    }
  } // end if # of PAs not forced, no synch. access, > 1 logical part

  // ---------------------------------------------------------------------
  // determine use of PAPA and make numPAs a multiple of the number of ESPs
  // ---------------------------------------------------------------------
  if (numPAs < numEsps)
  {
    // Synchronous access, or there just weren't enough partitions to
    // go around.
    numPAs = numEsps;
  }
  else if ((numPAs % numEsps) != 0)
  {
    // numPAs is not a multiple of numEsps. Round up if we are doing
    // PA partition grouping, round down if we are doing logical
    // subpartitioning. Note that if we round up, we will have
    // some PAs in some ESPs that are not doing any work. But,
    // this will help to minimize the effect of any imbalance,
    // because even though some ESPs must access more partitions
    // than others, they will access all partitions asynchronously.
    if (logPartType ==
        LogPhysPartitioningFunction::PA_PARTITION_GROUPING)
    {
      // Increase numPAs by numEsps-1 so that the truncation
      // below will result in rounding the # of PAs up.
      numPAs += numEsps - 1;
      numPAs = (numPAs / numEsps) * numEsps;
    }
    else // logical subpartitioning
    {
      // Round down. We round down because otherwise, we would have
      // multiple ESPs trying to access a portion of the same disk.
      numPAs = (numPAs / numEsps) * numEsps;
    }
  } // end if numPAs is not a multiple of numEsps

  if ((numPAs / numEsps) > maxPAsPerProcess)
  {
    // We have exceeded the maximum number of PAs per process, so we must
    // reduce the number to the limit. This will result in synchronous
    // access to some partitions.  We will not be able to preserve the order
    // via a merge of sorted streams, so if that's required give up now.
    if (mergeOfSortedStreams)
      return NULL;

    // Reduce the number of PAs so that the number of PAs per
    // process will be below the limit.
    numPAs = maxPAsPerProcess * numEsps;
  }

  usePapa = (usePapa OR numPAs > numEsps);

  //  The number of cpus executing DP2's cannot be more than the number
  //  of PAs. In other words, cannot be using more cpus than there are
  // streams.
  currentCountOfCPUs  = MINOF(currentCountOfCPUs,numPAs);


  // ---------------------------------------------------------------------
  // create a partitioning function for the scan node
  // ---------------------------------------------------------------------
  if (logPartType == LogPhysPartitioningFunction::PA_PARTITION_GROUPING AND
      physicalPartFunc->isASinglePartitionPartitioningFunction() AND
      numPAs == 1 AND NOT usePapa)
    {
      // Shortcut, no parallelism, no PAPA, and no logical
      // partitioning at all.  For this case ONLY we create a single
      // partition partitioning function.  The DP2 exchange node above
      // will recognize this special case.
      logPhysPartFunc = new(CmpCommon::statementHeap())
        SinglePartitionPartitioningFunction(
                            physicalPartFunc->getNodeMap()
                                            ->copy(CmpCommon::statementHeap())
                                           );
      logPhysPartFunc->createPartitioningKeyPredicates(); // just to be nice
    }
  else
    {
      // Make the logphys partitioning function which describes the way
      // in which we will actually perform the query. Each partition of the
      // logphys part function will correspond to one DP2 session.
      LogPhysPartitioningFunction *lpf = new(CmpCommon::statementHeap())
        LogPhysPartitioningFunction(
             logicalPartFunc,
             physicalPartFunc,
             logPartType,
             numPAs,
             usePapa,
             shouldUseSynchronousAccess);

      // lpf becomes const once added to synthLogProp, so calculate all
      // the info here.
      lpf->createPartitioningKeyPredicates();
      logPhysPartFunc = lpf;

      // Make a new partSearchKey with the partitioning key preds of
      // the partitioning function, if there are any. Note that ignoring
      // the part key preds will result in a wrong answer if we use
      // PA_PARTITION_GROUPING, since the PA node is the node responsible
      // for the grouping. If it doesn't select a subgroup of partitions,
      // too much data may be returned. For now we only consider a
      // search key for the PA node, MDAM to be implemented later.
      // MDAM will be useful for combining user-specified part key preds
      // with logicalPartFunc->getPartitioningKeyPredicates().
      if (NOT (logicalPartFunc->getPartitioningKeyPredicates().isEmpty() &&
               logicalPartFunc->usesFSForPartitionSelection()))
	{

          // the key preds have the group's char. inputs and the
          // partition input variables available
          ValueIdSet
            availInputs(getGroupAttr()->getCharacteristicInputs());
          ValueIdSet dummy;

          SearchKey *newPartSearchKey =
            logicalPartFunc->createSearchKey(indexDesc, availInputs, dummy);

          if(newPartSearchKey)
          {
            partSearchKey = newPartSearchKey;

            // again, for PA_PARTITION_GROUPING we have to interpret
            // all of the partitioning key predicates!

            CMPASSERT((partSearchKey->getKeyPredicates() ==
                       logicalPartFunc->getPartitioningKeyPredicates()) OR
                      (logPartType !=
                       LogPhysPartitioningFunction::PA_PARTITION_GROUPING));
          }

        }
    }

  // location is in master or ESP for hive tables
  if ((indexDesc->getPrimaryTableDesc()->getNATable()->isHiveTable()) ||
      (indexDesc->getPrimaryTableDesc()->getNATable()->isHbaseTable()))
    location = EXECUTE_IN_MASTER_AND_ESP;

  // Should never be a sort order type requirement in DP2
  CMPASSERT(rppForMe->getSortOrderTypeReq() == NO_SOT);

  PartitioningFunction* dp2SortOrderPartFunc = NULL;
  // Synthesize the dp2SortOrderPartFunc if the sort key is not empty
  if (NOT sortOrder.isEmpty())
    dp2SortOrderPartFunc = physicalPartFunc;

  PushDownProperty* pushDownProperty = NULL;
  const PushDownRequirement* pdr = rppForMe->getPushDownRequirement();
  if ( pdr ) {

       // Depend on the pushdown requirement, generate a colocation
       // or a CS push-down property.
       if ( PushDownCSRequirement::isInstanceOf(pdr) )
          pushDownProperty = new (CmpCommon::statementHeap())
                PushDownCSProperty(physicalPartFunc, partSearchKey);
       else {
         if ( PushDownColocationRequirement::isInstanceOf(pdr) )
            pushDownProperty = new (CmpCommon::statementHeap())
                PushDownColocationProperty(physicalPartFunc->getNodeMap());
         else
            CMPASSERT(1==0);
       }
  }

  // ---------------------------------------------------------------------
  // create a physical property object
  // ---------------------------------------------------------------------
  sppForMe = new (CmpCommon::statementHeap()) PhysicalProperty(
       sortOrder,
       NO_SOT, // Don't synthesize a sort order type until the exchange node
       dp2SortOrderPartFunc,
       logPhysPartFunc,
       location,
       SOURCE_PERSISTENT_TABLE,
       indexDesc,
       partSearchKey,
       pushDownProperty);


  DP2CostDataThatDependsOnSPP *dp2CostInfoPtr =
    new HEAP DP2CostDataThatDependsOnSPP(dp2CostInfo);
  sppForMe->setDP2CostThatDependsOnSPP(dp2CostInfoPtr);
  // ---------------------------------------------------------------------
  // Store more information about the decisions made in the synthesized
  // property
  // ---------------------------------------------------------------------
  sppForMe->setCurrentCountOfCPUs(currentCountOfCPUs);

  return sppForMe;

} // RelExpr::synthDP2PhysicalProperty()

//<pb>

// -----------------------------------------------------------------------
// FileScan::synthHiveScanPhysicalProperty()
// Synthesize physical property for a Hive table scan node,
// running in the master or an ESP
// -----------------------------------------------------------------------
PhysicalProperty * FileScan::synthHiveScanPhysicalProperty(
     const Context *context,
     const Lng32    planNumber,
     ValueIdList   &sortOrderVEG)
{
  PhysicalProperty *sppForMe = NULL;
  PartitioningFunction *myPartFunc = NULL;

  // my required phys props (always non-NULL)
  const ReqdPhysicalProperty* rppForMe = context->getReqdPhysicalProperty();
  PartitioningRequirement * partReq = rppForMe->getPartitioningRequirement();
  PlanExecutionEnum location = EXECUTE_IN_MASTER_AND_ESP;
  PartitioningFunction * ixDescPartFunc = indexDesc_->getPartitioningFunction();
  Lng32 numESPs = 1;

  // CQDs related to # of ESPs for a Hive table scan
  double bytesPerESP = getDefaultAsDouble(HIVE_MIN_BYTES_PER_ESP_PARTITION);
  Lng32 maxESPs = getDefaultAsLong(HIVE_MAX_ESPS);
  Lng32 numESPsPerDataNode = getDefaultAsLong(HIVE_NUM_ESPS_PER_DATANODE);
  Lng32 numSQNodes = HHDFSMasterHostList::getNumSQNodes();

  // minimum # of ESPs required by the parent
  Lng32 minESPs = (partReq ?  partReq->getCountOfPartitions() : 1);

  if (partReq && partReq->castToRequireApproximatelyNPartitions())
    minESPs = partReq->castToRequireApproximatelyNPartitions()->
                                             getCountOfPartitionsLowBound();

  NABoolean requiredESPsFixed = 
    partReq && partReq->castToFullySpecifiedPartitioningRequirement();

  const HHDFSTableStats *tableStats = hiveSearchKey_->getHDFSTableStats();

  // stats for partitions/buckets selected by predicates
  HHDFSStatsBase selectedStats((HHDFSTableStats *)tableStats);

  hiveSearchKey_->accumulateSelectedStats(selectedStats);

  // limit the number of ESPs to HIVE_NUM_ESPS_PER_DATANODE * nodes
  maxESPs = MAXOF(MINOF(numSQNodes*numESPsPerDataNode, maxESPs),1);

  // check for ATTEMPT_ESP_PARALLELISM CQD
  if (CURRSTMT_OPTDEFAULTS->attemptESPParallelism() == DF_OFF)
    maxESPs = 1;

  NABoolean useLocality = NodeMap::useLocalityForHiveScanInfo();

  // Take the smallest # of ESPs in the allowed range as a start
  numESPs = MINOF(minESPs, maxESPs);


  // We can adjust #ESPs only when the required ESPs is not fully specified 
  // from the parent.
  if ( !requiredESPsFixed ) {
    // following are soft adjustments to numESPs, within the allowed range
    double numESPsBasedOnTotalSize = 1;

    // adjust minESPs based on HIVE_MIN_BYTES_PER_ESP_PARTITION CQD
    if (bytesPerESP > 1.01)
      numESPsBasedOnTotalSize = selectedStats.getTotalSize()/(bytesPerESP-1.0);

    if (numESPsBasedOnTotalSize >= maxESPs)
      numESPs = maxESPs;
    else
      numESPs = MAXOF(numESPs, (Int32) ceil(numESPsBasedOnTotalSize));

    // if we use locality, generously increase # of ESPs to cover all the nodes
    if (useLocality &&
        maxESPs >= numSQNodes &&
        (numESPs > numSQNodes / 2 ||
         numESPs > numSQNodes - 10))
      numESPs = MAXOF(numESPs, numSQNodes);
  }

  if (numESPs > 1)
    {
      // Try to make the # of ESPs a factor, the same or a multiple
      // of the # of SQ nodes to avoid an imbalance. If we use locality,
      // make the # of ESPs a multiple of the # of nodes for now.
      double allowedDev = 1.0 + ActiveSchemaDB()->
        getDefaults().getAsDouble(HIVE_NUM_ESPS_ROUND_DEVIATION)/100.0;
      Lng32 maxRoundedESPs = MINOF((Lng32) (numESPs * allowedDev), maxESPs);
      Lng32 minRoundedESPs = MAXOF((Lng32) (numESPs / allowedDev), minESPs);
      Lng32 delta = 0;

      // starting with numESPs, search in ever larger
      // circles until we find a "nice" number
      NABoolean done = FALSE;
      while (! done)
        {
          Lng32 numOutOfRange = 0;
          // try i=+1 and i=-1 in this order
          for (Lng32 i=1; i<2 && !done; i=((i==1) ? -1 : 2))
            {
              // our candidate number, c is numESPs +/- delta
              Lng32 c = numESPs + i*delta;

              if (c >= minRoundedESPs && c <= maxRoundedESPs)
                {
                  NABoolean canUse = TRUE;

                  if ( partReq && 
                       partReq->castToRequireApproximatelyNPartitions() && 
                       !( ((RequireApproximatelyNPartitions*)partReq)->
                          isPartitionCountWithinRange(c) ) )
                     canUse = FALSE;

                  // let's check if we like this number c
                  // - same as or factor of # of SQ nodes
                  // - multiple of # of SQ nodes
                  // - multiple of # of SQ nodes + factor of # of SQ nodes
                  if ((c % numSQNodes == 0 ||
                      (! useLocality &&
                       (numSQNodes % c == 0 ||
                        (numSQNodes % (c % numSQNodes) == 0 && (c % numSQNodes > 1))))) && canUse )
                    {
                      // pick this candidate
                      numESPs = c;
                      done = TRUE;
                    }
                }
              else
                if (++numOutOfRange >= 2)
                  done = TRUE; // exceeded both limits, leave numESPs unchanged
            } // for
          // widen the circle by 1
          delta++;
        } // end while loop to try getting a "nice" number
    } // end if numESPs > 1
   
  NodeMap* myNodeMap = NULL;

  if (numESPs > 1)
    {
      // create a HASH2 partitioning function with numESPs partitions
      // and a RandomNum as the partitioning key (i.e. no usable part key)

      const HHDFSTableStats *tableStats = hiveSearchKey_->getHDFSTableStats();

      myNodeMap = new(CmpCommon::statementHeap())
        NodeMap(CmpCommon::statementHeap(),
                numESPs,
                NodeMapEntry::ACTIVE, NodeMap::HIVE);

      PartitioningFunction* pf = getTableDesc()
             ->getClusteringIndex() ->getPartitioningFunction();

      NABoolean useHash2Only = 
         CmpCommon::getDefault(HIVE_USE_HASH2_AS_PARTFUNCION) == DF_ON;

      if ( useHash2Only || 
           tableStats->getNumOfConsistentBuckets() == 0 || pf==NULL ) 
      {

         ItemExpr *randNum = new(CmpCommon::statementHeap()) RandomNum(NULL, TRUE);
         randNum->synthTypeAndValueId();

         ValueIdSet partKey;
         partKey.insert(randNum->getValueId());

         ValueIdList partKeyList(partKey);

         myPartFunc = new(CmpCommon::statementHeap())
           Hash2PartitioningFunction(partKey,
                                    partKeyList,
                                    numESPs,
                                    myNodeMap);
      } else {

         ValueIdSet partKey = pf->getPartitioningKey();

         ValueIdList partKeyList(partKey);

         myPartFunc = new(CmpCommon::statementHeap())
           HivePartitioningFunction(partKey,
                                     partKeyList,
                                     numESPs,
                                     myNodeMap);
      }

      myPartFunc->createPartitioningKeyPredicates();
    }
  else
    {
      myNodeMap = new(CmpCommon::statementHeap())
        NodeMap(CmpCommon::statementHeap(),
                1,
                NodeMapEntry::ACTIVE, NodeMap::HIVE);

      myPartFunc = new(CmpCommon::statementHeap())
        SinglePartitionPartitioningFunction(myNodeMap);
    }

  // create a very simple physical property for now, no sort order
  // and no partitioning key for now
  sppForMe = new(CmpCommon::statementHeap()) PhysicalProperty(myPartFunc,
                                                              location);

  //FILE* fd = fopen("nodemap.log", "a");
  //myNodeMap->print(fd, "", "hiveNodeMap");
  //fclose(fd);

  return sppForMe;
}

RangePartitionBoundaries * createRangePartitionBoundariesFromStats 
                                     (const IndexDesc* idesc,
                                      HistogramSharedPtr& hist,
                                      Lng32 numberOfPartitions,
                                      const NAColumnArray & partColArray,
                                      const ValueIdList& partitioningKeyColumnsOrder,
                                      const Int32 statsColsCount,
                                      NAMemory* heap);

//
// Parameter partns:  
//
//   On input:  the desired # of partitions
//   On output:  the final scaled-to # of partitions
// 
RangePartitioningFunction* 
FileScan::createRangePartFuncForHbaseTableUsingStats(
                     Int32& partns,
                     const ValueIdSet& partitioningKeyColumns,
                     const ValueIdList& partitioningKeyColumnsList,
                     const ValueIdList& partitioningKeyColumnsOrder
                                                    )
{
  Int32 bytesPerESP = getDefaultAsLong(HBASE_MIN_BYTES_PER_ESP_PARTITION);

  NABoolean useMCSplit = (CmpCommon::getDefault(HBASE_RANGE_PARTITIONING_MC_SPLIT) == DF_ON);

  if ( partns == 1 ||
       (!useMCSplit && (partitioningKeyColumns.entries() != 1 ))) // if partition key has more than one column but MC stats based 
                                                                  // partitioning is disabled, then return
    return NULL;

  // do not split the SMD or UMD table for now.
  if ( indexDesc_->getPrimaryTableDesc()->getNATable()->isSMDTable() ||
       indexDesc_->getPrimaryTableDesc()->getNATable()->isUMDTable() )
     return NULL;

  // Now consider the stats 
  const ColStatDescList &primaryTableCSDL = indexDesc_->getPrimaryTableDesc()->getTableColStats();

  // Get the key columns from the partKey since the
  // part key in the part. func. contains veg references.
 
   ValueId leadingColumn;
   NAColumnArray partKeyColArray;

   // if we have a partitioning key, take its first column,
   // otherwise take the first column of the clustering key
   if (indexDesc_->getPartitioningKey().entries() > 0)
   {
      leadingColumn = indexDesc_->getPartitioningKey()[0];

      for (CollIndex j =0; j < partitioningKeyColumns.entries(); j++)
      {
         partKeyColArray.insert(
               indexDesc_-> getNAFileSet()->getPartitioningKeyColumns()[j]
                               );
         partKeyColArray.setAscending(j, 
                              indexDesc_->getNAFileSet()->getPartitioningKeyColumns().
                                isAscending(j)
                                      );
      }
   } else {
      leadingColumn = indexDesc_-> getIndexKey()[0];

      for (CollIndex j =0; j < partitioningKeyColumns.entries(); j++)
      {
         partKeyColArray.insert(
                indexDesc_-> getNAFileSet()->getIndexKeyColumns()[j]
                               );
         partKeyColArray.setAscending(j, 
                          indexDesc_-> getNAFileSet()->getIndexKeyColumns().
                            isAscending(j)
                                     );
      }
   }

  // char types of non ISO88591 charset are currently not supported by the splitting logic
  for (CollIndex i = 0; i < partKeyColArray.entries(); i ++)
  {
     const NAType* nt = partKeyColArray.getColumn(i)->getType();
     if ((nt->getTypeQualifier() == NA_CHARACTER_TYPE) && (((CharType*)nt)->getCharSet() != CharInfo::ISO88591))
        return NULL;
  } 

  CollIndex i;
  if (primaryTableCSDL.getColStatDescIndexForColumn(i, // out
                                                    leadingColumn,
                                                    partKeyColArray))
  {
       ColStatsSharedPtr colStats = primaryTableCSDL[i]->getColStats();

       const NAFileSet* fset = indexDesc_->getNAFileSet();
                               // indexDesc_->getPrimaryTableDesc()->
                               // getNATable()->getClusteringIndex();

       Lng32 recLength = fset->getRecordLength();

       if ( (colStats->getRowcount() * recLength) < CostScalar(bytesPerESP) )
         return NULL;

       if ( !colStats->isOrigFakeHist() ) 
       {
          // Find a new set of boundaries values which will evenly divide
          // the whole table into partns partitions. 

          HistogramSharedPtr hist = NULL;
       
          if (colStats->getStatColumns().entries() > 1)
              hist = colStats->transformOnIntervalsForMC(partns);
          else
              hist = colStats->transformOnIntervals(partns);

          RangePartitionBoundaries * rpb = 
                   createRangePartitionBoundariesFromStats(
                        indexDesc_, 
                        hist, 
                        partns, 
                        partKeyColArray, 
                        partitioningKeyColumnsOrder, 
                        colStats->getStatColumns().entries(), 
                        STMTHEAP);

          if ( !rpb )
               return NULL;

          // Finally create a new range partitioned partition function,
          // with node map set to NULL.
          RangePartitioningFunction* newPartFunc =  new (STMTHEAP) 
                  RangePartitioningFunction(
                               partitioningKeyColumns,
                               partitioningKeyColumnsList,
                               partitioningKeyColumnsOrder,
                               rpb, NULL, STMTHEAP);

          newPartFunc->createPartitioningKeyPredicates();

          return newPartFunc;
       }
  }

  // no stats for the leading key columns, or faked stats. Give up.
  return NULL;
}


// -----------------------------------------------------------------------
// FileScan::synthHbaseScanPhysicalProperty()
// Synthesize physical property for a Hive table scan node,
// running in the master or an ESP
// -----------------------------------------------------------------------
PhysicalProperty * FileScan::synthHbaseScanPhysicalProperty(
     const Context *context,
     const Lng32    planNumber,
     ValueIdList   &sortOrderVEG)
{

 // my required phys props (always non-NULL)
  const ReqdPhysicalProperty* rppForMe = context->getReqdPhysicalProperty();
  PartitioningRequirement * partReq = rppForMe->getPartitioningRequirement();

  PartitioningFunction* myPartFunc = NULL;
              
  NABoolean partnsScaled = FALSE;
  Lng32 oldPartns = 0;
  Lng32 numESPs = 1;
  PartitioningFunction * ixDescPartFunc = NULL;
   
  // Nothing we can do if the requirement is a single partition func
  if ( partReq && partReq->castToRequireExactlyOnePartition() ) {
    myPartFunc = new (CmpCommon::statementHeap())
              SinglePartitionPartitioningFunction();
  } else {
 
     //////////////////////////////////////
     // Perform the scaling
     //////////////////////////////////////

     ixDescPartFunc = indexDesc_->getPartitioningFunction();

     //////////////////////////////////////
     // Compute the desirable #ESPs first
     //////////////////////////////////////
   
     // minimum # of ESPs required by the parent
     Lng32 minESPs = (partReq ? 
                       partReq->getCountOfPartitions() : 
                       CURRSTMT_OPTDEFAULTS->getMaximumDegreeOfParallelism());
   
     if (partReq && partReq->castToRequireApproximatelyNPartitions())
       minESPs = partReq->castToRequireApproximatelyNPartitions()->
                                                getCountOfPartitionsLowBound();
   
   
     Lng32 maxESPs = 1;
     NADefaults &defs = ActiveSchemaDB()->getDefaults();
   
     // check for ATTEMPT_ESP_PARALLELISM CQD
     if ( !(CURRSTMT_OPTDEFAULTS->attemptESPParallelism() == DF_OFF) ) {
        // CQDs related to # of ESPs for a HBase table scan
        maxESPs = getDefaultAsLong(HBASE_MAX_ESPS);

        Int32 numOfPartitions = -1;

        if ( ixDescPartFunc ) 
           numOfPartitions = ixDescPartFunc->getCountOfPartitions();  

        if ( maxESPs == 0 && minESPs <= numOfPartitions ) {
           minESPs = maxESPs = numOfPartitions;
        } else {
           NABoolean fakeEnv = FALSE; 
           CollIndex totalESPsAllowed = defs.getTotalNumOfESPsInCluster(fakeEnv);
      
           if ( !fakeEnv ) {
              // limit the number of ESPs to max(totalESPsAllowed, HBASE_MAX_ESPS)
               maxESPs = MAXOF(MINOF(totalESPsAllowed, maxESPs),1);
   
              if (!partReq && minESPs == 1) {
                minESPs = rppForMe->getCountOfPipelines();
   
                if (ixDescPartFunc && (CmpCommon::getDefault(LIMIT_HBASE_SCAN_DOP) == DF_ON)) {
                   minESPs = MINOF(minESPs, ixDescPartFunc->getCountOfPartitions());
                }
   
              }
   
              if ( getDefaultAsLong(AFFINITY_VALUE) != -2 && ixDescPartFunc ) {
                 Int32 numOfUniqueNodes = 
                     ixDescPartFunc->getNodeMap()->getNumberOfUniqueNodes();
   
                 // #ESPs performing reading from HBase tables is capped at by 
                 // the # of unique nodes or region servers.
                 if ( numOfUniqueNodes > 0 )
                    minESPs = MINOF(minESPs, numOfUniqueNodes);
              }
           }
           else  {
              maxESPs = totalESPsAllowed;
           }
        }
     }
   
     numESPs = MINOF(minESPs, maxESPs);
   
     NABoolean performStatsSplit = 
       (CmpCommon::getDefault(HBASE_STATS_PARTITIONING) != DF_OFF &&
        !(ixDescPartFunc && ixDescPartFunc->isAHash2PartitioningFunction()));

     if (partReq && partReq->castToRequireReplicateNoBroadcast()) {
       myPartFunc = 
         partReq->castToRequireReplicateNoBroadcast()->
         getPartitioningFunction()->copy();
   
     }
     else if ( ixDescPartFunc )
     {
   
       myPartFunc = ixDescPartFunc->copy();
       oldPartns = myPartFunc->getCountOfPartitions();
   
       Lng32 partns = numESPs;
   
       const RangePartitioningFunction* myPartFuncAsRange = NULL;
       if ( (myPartFuncAsRange=myPartFunc->castToRangePartitioningFunction()) ) 
       {

          RangePartitioningFunction* newPartFunc = NULL;

          if (performStatsSplit)
             newPartFunc = 
               createRangePartFuncForHbaseTableUsingStats(partns,
                       myPartFuncAsRange->getPartitioningKey(),
                       myPartFuncAsRange->getKeyColumnList(),
                       myPartFuncAsRange->getOrderOfKeyValues()
                                                         );
          if ( newPartFunc )
             myPartFunc = newPartFunc;
          else
             myPartFunc->scaleNumberOfPartitions(partns);
   
       } else {
          myPartFunc->scaleNumberOfPartitions(partns);
       }
   
       partnsScaled = (oldPartns != partns);
   
     } else {

        // A NULL ixDescPartFunc implies the table is single partitioned 
        // (1 region only).

        if ( performStatsSplit ) {
          Lng32 partns = numESPs;
  
          const ValueIdList& keyColumnList = indexDesc_->getPartitioningKey();
  
          const ValueIdList& orderOfKeyValues = indexDesc_->getOrderOfKeyValues();
          ValueIdSet keyColumnSet(keyColumnList);
  
          RangePartitioningFunction* newPartFunc =
                 createRangePartFuncForHbaseTableUsingStats(partns,
                         keyColumnSet,
                         keyColumnList,
                         orderOfKeyValues);
  
          if ( newPartFunc ) {
             myPartFunc = newPartFunc;
  
             // setup partKeys_
             ValueIdSet externalInputs = getGroupAttr()->getCharacteristicInputs();
             ValueIdSet dummySet;
  
             // Create and set the Searchkey for the partitioning key:
             partKeys_ =  new (CmpCommon::statementHeap())
                     SearchKey(keyColumnList, 
                               orderOfKeyValues, 
                               externalInputs,
                               NOT getReverseScan(),
                               selectionPred(),
                               *disjunctsPtr_,
                               dummySet, // needed by interface but not used here
                               indexDesc_
                               );
  
             partnsScaled = TRUE;
  
          } else
             myPartFunc = new(CmpCommon::statementHeap())
                 SinglePartitionPartitioningFunction();
        } else 
           myPartFunc = new(CmpCommon::statementHeap())
               SinglePartitionPartitioningFunction();
     }
  }

  if (myPartFunc->getNodeMap() == NULL || partnsScaled ) {

    NodeMap* myNodeMap = new(CmpCommon::statementHeap())
       NodeMap(CmpCommon::statementHeap(),
                  myPartFunc->getCountOfPartitions(),
                  NodeMapEntry::ACTIVE,
                  NodeMap::HBASE);
    
    myPartFunc->replaceNodeMap(myNodeMap);
  }

  // colocated ESP logic
  if ( (CmpCommon::getDefault(TRAF_ALLOW_ESP_COLOCATION) == DF_ON) AND
        ixDescPartFunc ) {
    // get region nodeMap which has regions nodeIds populated
    NodeMap* myNodeMap = (NodeMap*) myPartFunc->getNodeMap();
    const NodeMap* regNodeMap = ixDescPartFunc->getNodeMap();

    Int32 m=  myNodeMap->getNumEntries();
    Int32 n = regNodeMap->getNumEntries();

    // m : n allocation strategy where m < n using most popular node num
    if (m < n) {
      Lng32 regionsPerEsp = n / m;
      Lng32 beginPos = 0;
      for (Lng32 index = 0; (index < m && beginPos < n); index++) {
        Lng32 endPos = beginPos + regionsPerEsp;
        Lng32 popularNodeId =
          regNodeMap->getPopularNodeNumber(beginPos, endPos);
        myNodeMap->setNodeNumber(index, popularNodeId);
        beginPos = endPos;
      }
      myNodeMap->smooth(gpClusterInfo->numOfSMPs());
    } else if (m == n) { // 1:1 allocation strategy
      for (Lng32 index = 0; index < n; index++) {
        myNodeMap->setNodeNumber(index, regNodeMap->getNodeNumber(index));
      }
    }
  }

  PhysicalProperty * sppForMe =
    new(CmpCommon::statementHeap())
    PhysicalProperty(sortOrderVEG,
                     ESP_NO_SORT_SOT,
                     NULL, /* no dp2 part func*/
                     myPartFunc,
                     EXECUTE_IN_MASTER_AND_ESP,
                     SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  // -----------------------------------------------------------------------
  // Vector to put all costing data that is computed at synthesis time
  // Make it a local variable for now. If we ever reach the end of
  // this routine create a variable from the heap, initialize it with this,
  // and then set the sppForMe slot.
  // -----------------------------------------------------------------------
  DP2CostDataThatDependsOnSPP dp2CostInfo;
  // ---------------------------------------------------------------------
  // Estimate the number of active partitions and other costing
  // data that depends on SPP:
  // ---------------------------------------------------------------------

  computeDP2CostDataThatDependsOnSPP(*myPartFunc   // in/out
                                     ,dp2CostInfo //out
                                     ,*indexDesc_ // in
                                     ,*partKeys_ // in
                                     ,*getGroupAttr() //in
                                     ,*context // in
                                     ,CmpCommon::statementHeap() // in
                                     , *this
                                     );

  DP2CostDataThatDependsOnSPP *dp2CostInfoPtr =
    new HEAP DP2CostDataThatDependsOnSPP(dp2CostInfo);
  sppForMe->setDP2CostThatDependsOnSPP(dp2CostInfoPtr);

  sppForMe->setCurrentCountOfCPUs(dp2CostInfo.getCountOfCPUsExecutingDP2s());

  return sppForMe ;
}

//<pb>

// -----------------------------------------------------------------------
// FileScan::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
FileScan::costMethod() const
{
  static THREAD_P CostMethodFileScan *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodFileScan();
  return m;
} // FileScan::costMethod()

//<pb>
//==============================================================================
//  Synthesize physical properties for FileScan operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
FileScan::synthPhysicalProperty(const Context* myContext,
                                const Lng32     planNumber,
                                PlanWorkSpace  *pws)
{
  PhysicalProperty *sppForMe;
  // synthesized order
  ValueIdList sortOrderVEG = NULL;

  sortOrderVEG = indexDesc_->getOrderOfKeyValues();

  // ---------------------------------------------------------------------
  // Remove from the sortOrder those columns that are equal to constants
  // or input values.  Also, remove from sortOrder those columns that are
  // not in the characteristic outputs.
  //   It is possible that the characteristic outputs
  // will not contain the base columns, but rather expressions
  // involving the base columns. For example, if the user specified
  // "select b+1 from t order by 1;", and "b" is the primary key,
  // then the characteristic output will only contain the valueId for the
  // expression "b+1" - it will not contain the value id for "b". So,
  // even though "b" is the primary key, we will fail to find it in the
  // characteristic outputs and thus we will not synthesize the sort key.
  // To solve this, we first check for the base column in the
  // characteristic outputs. If we find it, great. But if we don't, we
  // need to see if the base column is included in the SIMPLIFIED form
  // of the characteristic outputs. If it is, then we need to change
  // the sort key to include the valueId for the expression "b+1" instead
  // of "b". This is because we cannot synthesize anything in the sort
  // key that is not in the characteristic outputs, but if something
  // is sorted by "b" then it is surely sorted by "b+1". We have
  // coined the expression "complify" to represent this operation.
  // ---------------------------------------------------------------------
  sortOrderVEG.removeCoveredExprs(getGroupAttr()->getCharacteristicInputs());
  sortOrderVEG.complifyAndRemoveUncoveredSuffix(
       getGroupAttr()->getCharacteristicOutputs(),
       getGroupAttr());

  // ---------------------------------------------------------------------
  // if this is a reverse scan, apply an inversion function to
  // each of the ordering columns
  // ---------------------------------------------------------------------
  if (getReverseScan())
    {
      ItemExpr *inverseCol;

      for (Lng32 i = 0; i < (Lng32)sortOrderVEG.entries(); i++)
        {
          ItemExpr *ix = sortOrderVEG[i].getItemExpr();

          if (ix->getOperatorType() == ITM_INVERSE)
            {
              // remove the inverse operator, the reverse scan
              // cancels it out
              inverseCol = ix->child(0);
            }
          else
            {
              // add an inverse operator on top
              inverseCol = new(CmpCommon::statementHeap())
                InverseOrder(ix);
              inverseCol->synthTypeAndValueId();
            }
          sortOrderVEG[i] = inverseCol->getValueId();
        }
    }

  if (isHiveTable())
    return synthHiveScanPhysicalProperty(myContext, planNumber, sortOrderVEG);
  else if (isHbaseTable())
    return synthHbaseScanPhysicalProperty(myContext, planNumber, sortOrderVEG);

  // ---------------------------------------------------------------------
  // call a static helper method shared between scan and ins/upd/del
  // ---------------------------------------------------------------------
  if ((sppForMe = synthDP2PhysicalProperty(myContext,
                                           sortOrderVEG,
                                           indexDesc_,
                                           partKeys_)) == NULL)
    return NULL;

  // ---------------------------------------------------------------------
  // Apply partitioning key predicates if necessary
  // ---------------------------------------------------------------------
  if (sppForMe->getPartitioningFunction()->
      castToLogPhysPartitioningFunction())
    {
      LogPhysPartitioningFunction *logPhysPartFunc =
        (LogPhysPartitioningFunction *)  // cast away const
        sppForMe->getPartitioningFunction()->
        castToLogPhysPartitioningFunction();
      LogPhysPartitioningFunction::logPartType logPartType =
        logPhysPartFunc->getLogPartType();

      if (logPartType ==
          LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING OR
          logPartType ==
          LogPhysPartitioningFunction::HORIZONTAL_PARTITION_SLICING)
        {
          logPhysPartFunc->createPartitioningKeyPredicates();
          CMPASSERT(FALSE);
          // also need to apply the part key preds and pick up the PIVs in
          // FileScan::preCodGen if we ever use this
        }

    }


  // try to determine if the scan will take place only with one partition.
  // The condition for this to happen is that a local predicate T.A = const
  // exists and the table T is partition on A.
  if  ( partKeys_ ) {

     // get the part key predicate. For local predicate 'T.A = 12',
     // the predicate = { vegRef { T.A, '12' } }
     const ValueIdSet& pkPredicate = partKeys_->getKeyPredicates();

     // get the partitioning key of the indexDesc_. If the T is partitioned
     // on T.A, the pkList then is [ T.A ]
     const ValueIdList & pkList = indexDesc_->getPartitioningKey();

     // We now check if every element of the part key predicate is associated
     // with an element of the partition key of the current indexDesc_ below.

     UInt32 size = pkList.entries();
     NABoolean accessSinglePartition = size > 0;

     for (UInt32 i=0; i < size; i++)
     {
         if ( ! pkPredicate.containsAsEquiLocalPred(pkList[i]) ) {
            accessSinglePartition = FALSE;
             break;
         }
     }
     sppForMe->setAccessOnePartition(accessSinglePartition);
  }


  return sppForMe;

} // FileScan::synthPhysicalProperty()

const PartitioningFunction * FileScan::getPartFunc() const
{
  return getPhysicalProperty()->getPartitioningFunction();
}

void FileScan::addPartKeyPredsToSelectionPreds(
                 const ValueIdSet& partKeyPreds,
                 const ValueIdSet& pivs)
{
  selectionPred() += partKeyPreds;
}

NABoolean FileScan::okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*IN,OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/)
{
  const ReqdPhysicalProperty* rppForMe =
     myContext->getReqdPhysicalProperty();

  NABoolean result = FALSE;

  DefaultToken parallelControlSettings =
    getParallelControlSettings(rppForMe,
                               numOfESPs,
                               allowedDeviation,
                               numOfESPsForced);

  if (parallelControlSettings == DF_OFF)
  {
    result = FALSE;
  }
  else if ((CmpCommon::getDefault(COMP_BOOL_64) == DF_ON) AND
           (parallelControlSettings == DF_MAXIMUM)  AND
             CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible()
          )
  {
    numOfESPs = rppForMe->getCountOfPipelines();
    allowedDeviation = 0.0; // not used by leafs
    result = TRUE;
  }
  else if (parallelControlSettings == DF_ON)
  {
    // Currently, forcing of the number of ESPs for a leaf
    // is not supported. So, numOfESPsForced should always be FALSE.
    if (NOT numOfESPsForced)
    {
      const Int32 optimalNumPAsPerEsp =
        (Int32) getDefaultAsLong(PARTITION_ACCESS_NODES_PER_ESP);

      // divide number of PAs by number of PAs per ESP and round up to
      // the next highest number of ESPs if there is a remainder
      numOfESPs = (numOfESPs+optimalNumPAsPerEsp-1) /
                   optimalNumPAsPerEsp;
      // Can't have more ESPs than the maximum
      numOfESPs = MINOF(numOfESPs,rppForMe->getCountOfPipelines());
      allowedDeviation = 0.0; // not used by leafs
    }
    result = TRUE;
  }
  else
  {
    // Otherwise, the user must have specified "SYSTEM" for the
    // ATTEMPT_ESP_PARALLELISM default. This means it is up to the
    // optimizer to decide.

    // Return TRUE if the number of rows returned
    // by child(0) exceeds the threshold from the defaults
    // table. The recommended number of ESPs is also computed
    // to be 1 process per <threshold> number of rows.

    EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
    EstLogPropSharedPtr outputLogProp = getGroupAttr()->outputLogProp(inLogProp);
    const CostScalar rowCount =
      (outputLogProp->getResultCardinality()).minCsOne();

    const CostScalar numberOfRowsThreshold =
      CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold();
    numOfESPs = rppForMe->getCountOfPipelines();

    if ( (rowCount > numberOfRowsThreshold) AND
         (CmpCommon::getDefault(COMP_BOOL_128) == DF_ON)
       )
    {
      Lng32 optimalNumOfESPs = MINOF(numOfESPs,
        (Lng32)(rowCount / numberOfRowsThreshold).value());

       // make numOfESPs as available level of parallelism
         // 16*N, 8*N, 4*N,..., N,1 where N is the number of segments
      Lng32 i = CURRSTMT_OPTDEFAULTS->getMaximumDegreeOfParallelism();
      Lng32 MinParallelism =
        MAXOF(CURRSTMT_OPTDEFAULTS->getMinimumESPParallelism(),optimalNumOfESPs);
      while(i > MinParallelism)
        i/=2;

      numOfESPs = (i<MinParallelism) ? i*=2 : i;
      allowedDeviation = 0.0; // not used by scan
      result = TRUE;
    }
    else
    {
      result = FALSE;
    }
  } // end if the user let the optimizer decide

  return result;

} // FileScan::okToAttemptESPParallelism()

//<pb>

// -----------------------------------------------------------------------
// member functions for class DP2Scan
// -----------------------------------------------------------------------
CostMethod*
DP2Scan::costMethod() const
{

  static THREAD_P CostMethodDP2Scan *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodDP2Scan();
  return m;

} // DP2Scan::costMethod()


//<pb>
//==============================================================================
//  Synthesize physical properties for Describe operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
Describe::synthPhysicalProperty(const Context* myContext,
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
} //  Describe::synthPhysicalProperty()


//<pb>
//==============================================================================
//  Synthesize physical properties for GenericUtilExpr operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
GenericUtilExpr::synthPhysicalProperty(const Context* myContext,
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
} //  GenericUtilExpr::synthPhysicalProperty()

// -----------------------------------------------------------------------
// FirstN::createContextForAChild()
// The FirstN node may have an order by requirement that it needs to
// pass to its child context. Other than that, this method is quite
// similar to the default implementation, RelExpr::createContextForAChild.
// The arity of FirstN is always 1, so some logic from the default
// implementation that deals with childIndex > 0 is unnecessary and has
// been removed.
// -----------------------------------------------------------------------
Context * FirstN::createContextForAChild(Context* myContext,
                                 PlanWorkSpace* pws,
                                 Lng32& childIndex)
{
  const ReqdPhysicalProperty* rppForMe =
                                    myContext->getReqdPhysicalProperty();

  CMPASSERT(getArity() == 1);

  childIndex = getArity() - pws->getCountOfChildContexts() - 1;

  // return if we are done
  if (childIndex < 0)
    return NULL;

  RequirementGenerator rg(child(childIndex), rppForMe);

  if (reqdOrder().entries() > 0)
    {
      // add our sort requirement as implied by our ORDER BY clause

      // Shouldn't/Can't add a sort order type requirement
      // if we are in DP2
      if (rppForMe->executeInDP2())
        rg.addSortKey(reqdOrder(),NO_SOT);
      else
        rg.addSortKey(reqdOrder(),ESP_SOT);
    }

  if (NOT rg.checkFeasibility())
    return NULL;

  Lng32 planNumber = 0;

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);

  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as myContext.
  // Reuse it, if found. Otherwise, create a new Context that contains
  // the same rpp and input log prop as in myContext.
  // ---------------------------------------------------------------------
  Context* result = shareContext(childIndex, rg.produceRequirement(),
                                 myContext->getInputPhysicalProperty(),
                                 costLimit, myContext,
                                 myContext->getInputLogProp());

  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, planNumber, result);

  return result;

} // FirstN::createContextForAChild()

//<pb>
//==============================================================================
//  Synthesize physical properties for FirstN operator's current plan
// extracted from a specified context.
// FirstN operator has the same properties as its child.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
FirstN::synthPhysicalProperty(const Context* context,
                              const Lng32     planNumber,
                              PlanWorkSpace  *pws)
{
  // ---------------------------------------------------------------------
  // Simply propogate child's physical property.
  // ---------------------------------------------------------------------
  const PhysicalProperty* const sppOfTheChild =
                    context->getPhysicalPropertyOfSolutionForChild(0);

  PhysicalProperty* sppForMe = new (CmpCommon::statementHeap())
    PhysicalProperty (*sppOfTheChild);

  if (canExecuteInDp2())
    sppForMe->setLocation(EXECUTE_IN_DP2);
  else
    sppForMe->setLocation(EXECUTE_IN_MASTER);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe;
}

//<pb>
//==============================================================================
//  Synthesize physical properties for RelTransaction operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
RelTransaction::synthPhysicalProperty(const Context* myContext,
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
} //  RelTransaction::synthPhysicalProperty()

//============================================================================
//  Synthesize physical properties for RelSetTimeout operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//============================================================================
PhysicalProperty*
RelSetTimeout::synthPhysicalProperty(const Context* myContext,
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
} //  RelSetTimeout::synthPhysicalProperty()


//==============================================================================
//  Synthesize physical properties for RelLock operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
RelLock::synthPhysicalProperty(const Context* myContext,
                               const Lng32     planNumber,
                               PlanWorkSpace  *pws)
{

  PartitioningFunction *myPartFunc = NULL;
  const ReqdPhysicalProperty* rpp = myContext->getReqdPhysicalProperty();
  
  if (rpp->getCountOfAvailableCPUs() <= 1)
    parallelExecution_ = FALSE;

  if (parallelExecution_)
    {
      myPartFunc =
	tabIds_[0]->getClusteringIndex()->getPartitioningFunction();
      
      if (!myPartFunc)
	parallelExecution_ = FALSE;
      else
	{
	  PhysicalProperty * sppForMe = new(CmpCommon::statementHeap())
	    PhysicalProperty(myPartFunc,
			     EXECUTE_IN_ESP,
			     SOURCE_VIRTUAL_TABLE);
	  // remove anything that's not covered by the group attributes
	  sppForMe->enforceCoverageByGroupAttributes(getGroupAttr());
	  return sppForMe;
	}
    }

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
  myPartFunc =
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
} //  RelLock::synthPhysicalProperty()

//<pb>
//==============================================================================
//  Synthesize physical properties for ControlAbstractClass operator's current
// plan extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
ControlAbstractClass::synthPhysicalProperty(const Context* myContext,
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
} //  ControlAbstractClass::synthPhysicalProperty()
//<pb>
// -----------------------------------------------------------------------
// methods for class Tuple
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Tuple::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
Tuple::costMethod() const
{
  static THREAD_P CostMethodTuple *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodTuple();
  return m;
} // Tuple::costMethod()

//<pb>
//==============================================================================
//  Synthesize physical properties for Tuple operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
// Note this is a close copy of the synthPhysicalProperties for Tuple,
// so if we change anything here we should evaluate if the Tuple method
// should change as well.
//
//==============================================================================
PhysicalProperty*
Tuple::synthPhysicalProperty(const Context* myContext,
                             const Lng32     planNumber,
                             PlanWorkSpace  *pws)
{
  CMPASSERT(myContext != NULL);

  PlanExecutionEnum planExecutionLocation = EXECUTE_IN_MASTER_AND_ESP;
  PartitioningRequirement *myPartReq = NULL;
  PartitioningFunction *myPartFunc = NULL;

  const ReqdPhysicalProperty* rppForMe = 0;

  rppForMe = myContext->getReqdPhysicalProperty();

  // -----------------------------------------------------------------
  // decide on my partitioning function
  // -----------------------------------------------------------------
  myPartReq = rppForMe->getPartitioningRequirement();

  if (myPartReq ) {
    myPartFunc = myPartReq->realize(myContext);
    if ( NOT rppForMe->executeInDP2() &&
         (myPartFunc -> castToHashPartitioningFunction()  ||
          myPartFunc -> castToTableHashPartitioningFunction()  ||
          myPartFunc -> castToRangePartitioningFunction()  ||
          myPartFunc -> castToRoundRobinPartitioningFunction()
         ) && myPartFunc -> getCountOfPartitions() > 1
       )
       return NULL;
  } else {

     // If the tuple is in DP2, use the partition function in
     // pushdown property (if any).
     if ( rppForMe->executeInDP2() )
     {
        const PushDownRequirement* pdq = rppForMe->getPushDownRequirement();

        if ( pdq AND PushDownCSRequirement::isInstanceOf(pdq) )
           myPartFunc = (PartitioningFunction*)
                ((pdq->castToPushDownCSRequirement())->getPartFunc());
     }

     if ( myPartFunc == NULL )
        myPartFunc = new(CmpCommon::statementHeap())
          SinglePartitionPartitioningFunction();
  }

  // Make sure the Tuple node only produces its tuple when it is
  // part of the partition to be produced. Do this by applying the
  // partitioning key predicates which filter out the data of that
  // partition.
  if (myPartFunc->canProducePartitioningKeyPredicates())
    {
      myPartFunc->createPartitioningKeyPredicates();

      if ( NOT rppForMe->executeInDP2() )
      {
      // for single partition and replication the predicates will be empty
      if (NOT myPartFunc->getPartitioningKeyPredicates().isEmpty())
        {
          // ||opt we would need to add the partitioning key
          // predicates, but I see in the executor that no
          // predicates are evaluated on the tuple node.  This may
          // be an unrelated bug or a problem for this case. For now
          // refuse to generate any partitioning scheme that has
          // partitioning key predicates
          return NULL;
        }
      }
    }
  else
    {
      // A leaf node can only produce a partitioning function if
      // it can apply its partitioning key predicates. If we can't
      // produce the data we better return NULL physical props.
      return NULL;
    }

  // If partitioning function has no node map already, create a new node map
  // having wild-card entries for each partition.
  if (myPartFunc->getNodeMap() == 0)
    {
      NodeMap* myNodeMap = new(CmpCommon::statementHeap())
                            NodeMap(CmpCommon::statementHeap(),
                                    myPartFunc->getCountOfPartitions(),
                                    NodeMapEntry::ACTIVE);
      myPartFunc->replaceNodeMap(myNodeMap);
    }

  const PushDownRequirement* pdr = rppForMe->getPushDownRequirement();

  // Disable inserts under exchange if the compound statement is not
  // under the same exchange.
  if ( isinBlockStmt() AND rppForMe->executeInDP2() AND
       NOT PushDownCSRequirement::isInstanceOf(pdr)
     )
    return 0;

  if ( pdr )
  {
     // If the tuple is the first statement (left most node) or
     // immediately below an EXCHANGE, do not allow it.
     if (pdr->isEmpty()== TRUE)
       return 0;

     planExecutionLocation = EXECUTE_IN_DP2;

     // Any leaf node inside DP2 should produce a physical property.
     // Furthermore, we need a logphy partfunc here so that it can
     // be compared with that of the INSERTed node.
     if (! myPartFunc->isASinglePartitionPartitioningFunction())
     {
       LogicalPartitioningRequirement *lpr;
       lpr = rppForMe->getLogicalPartRequirement();

       PartitioningRequirement *logPartReq = NULL;
       Lng32      numPAs  = ANY_NUMBER_OF_PARTITIONS;
       Lng32      numEsps = 1;
       NABoolean usePapa = FALSE;
       NABoolean shouldUseSynchronousAccess = FALSE;
       LogPhysPartitioningFunction::logPartType logPartType;
       PartitioningFunction * logicalPartFunc = NULL;

       if (!lpr)
       {
         lpr = new(CmpCommon::statementHeap())
                   LogicalPartitioningRequirement(
                      rppForMe->getPartitioningRequirement());

         logPartType = LogPhysPartitioningFunction::ANY_LOGICAL_PARTITIONING;
         numPAs      = ANY_NUMBER_OF_PARTITIONS;
         usePapa     = FALSE;
       }
       else
       {

         logPartType = lpr->getLogPartTypeReq();
         numPAs      = lpr->getNumClientsReq();
         usePapa     = lpr->getMustUsePapa();
       }

       logPartReq  = lpr->getLogReq();
       if (logPartReq)
       {
         logicalPartFunc = logPartReq->realize(
              myContext,
              FALSE,
              myPartFunc->makePartitioningRequirement());

         myPartFunc = new(CmpCommon::statementHeap())
         LogPhysPartitioningFunction(
		logicalPartFunc, // logical
		myPartFunc,      // physical
		logPartType,
		numPAs,
		usePapa,
		shouldUseSynchronousAccess);

         myPartFunc->createPartitioningKeyPredicates();
       }
       // coverity thinks "lpr" is leaked here. "lpr" is eventually freed by
       // CmpCommon::statementHeap's destructor. It is dangerous to free
       // "lpr" here because myPartFunc indirectly holds a pointer to "lpr".
       // coverity[leaked_storage]
     }
  }

  PhysicalProperty* sppForMe =
    new (CmpCommon::statementHeap())
      PhysicalProperty(myPartFunc,
                       planExecutionLocation,
                       SOURCE_TUPLE);

   PushDownProperty* pushDownProperty = 0;

   if ( pdr && !pdr->isEmpty() )
    {
      const PushDownCSRequirement*
         pdcsr = pdr->castToPushDownCSRequirement();

      if ( pdcsr )
      {
      // generate a CS push-down property.
         pushDownProperty = new (CmpCommon::statementHeap())
            PushDownCSProperty(pdcsr->getPartFunc(), pdcsr->getSearchKey());
      } else {

         const PushDownColocationRequirement*
            pdclr = pdr->castToPushDownColocationRequirement();

         // generate a colocation push-down property.
         if ( pdclr )
         {
            pushDownProperty = new (CmpCommon::statementHeap())
               PushDownColocationProperty(pdclr->getNodeMap());
         } else
            CMPASSERT(1==0);
      }
    }

   sppForMe->setPushDownProperty(pushDownProperty);

  // -----------------------------------------------------------------------
  // Estimate the number of cpus executing executing copies of this
  // instance:
  // -----------------------------------------------------------------------
  Lng32 countOfCPUs = 1;

  Lng32 countOfStreams = 1;
  if (myPartFunc != NULL)
    countOfStreams = myPartFunc->getCountOfPartitions();

  Lng32  countOfAvailableCPUs = 1;
  if (myContext->getReqdPhysicalProperty())
    countOfAvailableCPUs =
      myContext->getReqdPhysicalProperty()->getCountOfAvailableCPUs();

  // The number of CPUs is limited by the number of streams
  countOfCPUs = (countOfStreams < countOfAvailableCPUs ?
                      countOfStreams : countOfAvailableCPUs);
  CMPASSERT(countOfCPUs >= 1);

  // A Tuple operator does not normally execute in DP2. Still, it is
  // a leaf operator. So, set the currentCountOfCPUs in the spp for
  // tuple to tuple's count of cpus. This way, if any of tuple's
  // parents decide to base their degree of parallelism on their
  // children they can get their child's degree of parallelism
  // from the currentCountOfCPUs field of their child's spp, regardless
  // of whether the child is really a DP2 operator or not.
  sppForMe->setCurrentCountOfCPUs(countOfCPUs);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe;

} //  Tuple::synthPhysicalProperty()

//<pb>
//==============================================================================
//  Synthesize physical properties for InsertCursor operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
InsertCursor::synthPhysicalProperty(const Context* myContext,
                                    const Lng32     planNumber,
                                    PlanWorkSpace  *pws)
{
  ValueIdList emptySortKey;
  PhysicalProperty * sppForMe = synthDP2PhysicalProperty(myContext,
                                                         emptySortKey,
                                                         getIndexDesc(),
                                                         getPartKey());

  // remove anything that's not covered by the group attributes
  if ( sppForMe )
    sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe ;

} // InsertCursor::synthPhysicalProperty()


//<pb>
//==============================================================================
//  Synthesize physical properties for UpdateCursor operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
UpdateCursor::synthPhysicalProperty(const Context* myContext,
                                    const Lng32     planNumber,
                                    PlanWorkSpace  *pws)
{

  ValueIdList emptySortKey;

  PhysicalProperty * sppForMe = synthDP2PhysicalProperty(myContext,
                                                         emptySortKey,
                                                         getIndexDesc(),
                                                         getPartKey());

  // remove anything that's not covered by the group attributes
  if ( sppForMe )
    sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe ;

} // UpdateCursor::synthPhysicalProperty()


//<pb>
//==============================================================================
//  Synthesize physical properties for DeleteCursor operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
DeleteCursor::synthPhysicalProperty(const Context* myContext,
                                    const Lng32     planNumber,
                                    PlanWorkSpace  *pws)
{


  ValueIdList emptySortKey;
  PhysicalProperty * sppForMe = synthDP2PhysicalProperty(myContext,
                                                         emptySortKey,
                                                         getIndexDesc(),
                                                         getPartKey());

  // remove anything that's not covered by the group attributes
  if ( sppForMe )
    sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe ;

} // DeleteCursor::synthPhysicalProperty()


NABoolean GenericUpdate::okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*IN,OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/)
{
  const ReqdPhysicalProperty* rppForMe =
     myContext->getReqdPhysicalProperty();

  NABoolean result = FALSE;

  DefaultToken parallelControlSettings =
    getParallelControlSettings(rppForMe,
                               numOfESPs,
                               allowedDeviation,
                               numOfESPsForced);

  if (isMerge())
  {
    result = FALSE;
  }
  else
  if (parallelControlSettings == DF_OFF)
  {
    result = FALSE;
  }
  else if ((CmpCommon::getDefault(COMP_BOOL_65) == DF_ON) AND
           (parallelControlSettings == DF_MAXIMUM)  AND
             CURRSTMT_OPTDEFAULTS->maxParallelismIsFeasible()
           )
  {
    numOfESPs = rppForMe->getCountOfPipelines();
    allowedDeviation = 0.0; // not used by leafs
    result = TRUE;
  }
  else if (parallelControlSettings == DF_ON)
  {
    // Currently, forcing of the number of ESPs for a leaf
    // is not supported. So, numOfESPsForced should always be FALSE.
    if (NOT numOfESPsForced)
    {
      const Int32 optimalNumPAsPerEsp =
        (Int32) getDefaultAsLong(PARTITION_ACCESS_NODES_PER_ESP);

      // divide number of PAs by number of PAs per ESP and round up to
      // the next highest number of ESPs if there is a remainder
      numOfESPs = (numOfESPs+optimalNumPAsPerEsp-1) /
                   optimalNumPAsPerEsp;
      // Can't have more ESPs than the maximum
      numOfESPs = MINOF(numOfESPs,rppForMe->getCountOfPipelines());
      allowedDeviation = 0.0; // not used by leafs
    }
    result = TRUE;
  }
  else
  {
    // Otherwise, the user must have specified "SYSTEM" for the
    // ATTEMPT_ESP_PARALLELISM default. This means it is up to the
    // optimizer to decide.

    // Return TRUE if the number of rows returned
    // by child(0) exceeds the threshold from the defaults
    // table. The recommended number of ESPs is also computed
    // to be 1 process per <threshold> number of rows.

    EstLogPropSharedPtr inLogProp = myContext->getInputLogProp();
    EstLogPropSharedPtr outputLogProp = getGroupAttr()->outputLogProp(inLogProp);
    const CostScalar rowCount =
      (outputLogProp->getResultCardinality()).minCsOne();

    const CostScalar numberOfRowsThreshold =
      CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold();

    if (rowCount > numberOfRowsThreshold)
    {
      double optimalNumOfESPsDbl =
	ceil((rowCount / numberOfRowsThreshold).value());

      // Don't need / can't have more ESPs than PAs
      numOfESPs = (Lng32) MINOF(numOfESPs,optimalNumOfESPsDbl);
      // Can't have more ESPs than the maximum
      numOfESPs = MINOF(numOfESPs,rppForMe->getCountOfPipelines());

      allowedDeviation = 0.0; // not used
      result = TRUE;
    }
    else
    {
      result = FALSE;
    }
  } // end if the user let the optimizer decide

  return result;

} // GenericUpdate::okToAttemptESPParallelism()

//<pb>
//==============================================================================
//  Synthesize physical properties for Explain operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
ExplainFunc::synthPhysicalProperty(const Context* myContext,
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

  return sppForMe ;

} // ExplainFunc::synthPhysicalProperty()

PhysicalProperty*
StatisticsFunc::synthPhysicalProperty(const Context* myContext,
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

  return sppForMe ;

} // StatisticsFunc::synthPhysicalProperty()

PhysicalProperty *
PhysicalSPProxyFunc::synthPhysicalProperty(const Context *myContext,
                                           const Lng32 planNumber,
                                           PlanWorkSpace  *pws)
{

  //----------------------------------------------------------
  // Create a node map with a single, active, wild-card entry.
  //----------------------------------------------------------
  NodeMap* myNodeMap = new(CmpCommon::statementHeap())
    NodeMap(CmpCommon::statementHeap(), 1, NodeMapEntry::ACTIVE);

  //------------------------------------------------------------
  // Synthesize a partitioning function with a single partition.
  //------------------------------------------------------------
  PartitioningFunction *myPartFunc = new (CmpCommon::statementHeap())
    SinglePartitionPartitioningFunction(myNodeMap);

  PhysicalProperty *sppForMe = new (CmpCommon::statementHeap())
    PhysicalProperty(myPartFunc, EXECUTE_IN_MASTER, SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes(getGroupAttr());

  return sppForMe;

} // PhysicalSPProxyFunc::synthPhysicalProperty()

PhysicalProperty *
PhysicalExtractSource::synthPhysicalProperty(const Context *myContext,
                                             const Lng32 planNumber,
                                             PlanWorkSpace  *pws)
{
  //----------------------------------------------------------
  // Create a node map with a single, active, wild-card entry.
  //----------------------------------------------------------
  NodeMap* myNodeMap = new(CmpCommon::statementHeap())
    NodeMap(CmpCommon::statementHeap(), 1, NodeMapEntry::ACTIVE);

  //------------------------------------------------------------
  // Synthesize a partitioning function with a single partition.
  //------------------------------------------------------------
  PartitioningFunction *myPartFunc = new (CmpCommon::statementHeap())
    SinglePartitionPartitioningFunction(myNodeMap);

  PhysicalProperty *sppForMe = new (CmpCommon::statementHeap())
    PhysicalProperty(myPartFunc, EXECUTE_IN_ESP, SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes(getGroupAttr());

  return sppForMe;

} // PhysicalExtractSource::synthPhysicalProperty()

// Transpose::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this type.
CostMethod*
PhysTranspose::costMethod() const
{
  static THREAD_P CostMethodTranspose *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodTranspose();
  return m;
} // PhysTranspose::costMethod()

//<pb>
//==============================================================================
//  Synthesize physical properties for Transpose operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
PhysTranspose::synthPhysicalProperty(const Context *context,
                                     const Lng32     planNumber,
                                     PlanWorkSpace  *pws)
{
  // for now, simply propagate the physical property

  PhysicalProperty *sppForMe = new(CmpCommon::statementHeap())
    PhysicalProperty(*context->getPhysicalPropertyOfSolutionForChild(0));

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe ;
} //  PhysTranspose::synthPhysicalProperty()

//<pb>
//==============================================================================
//  Synthesize physical properties for Stored Procedure operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty *
RelInternalSP::synthPhysicalProperty(const Context * myContext,
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
    new(CmpCommon::statementHeap()) PhysicalProperty(
         myPartFunc,
         EXECUTE_IN_MASTER,
         SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe ;

} //  RelInternalSP::synthPhysicalProperty()

// -----------------------------------------------------------------------
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of type RelInternalSP.
// -----------------------------------------------------------------------

CostMethod *
RelInternalSP::costMethod() const
{
  static THREAD_P CostMethodStoredProc *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodStoredProc();
  return m;

} // RelInternalSP::costMethod()
//<pb>

CostMethod *
HbaseDelete::costMethod() const
{
  if (CmpCommon::getDefault(HBASE_DELETE_COSTING) == DF_OFF)
    // RelExpr::costMethod() costin returns a cost 1 cost object. This
    // is the old behavior before the new costing code was written.
    return RelExpr::costMethod();

  static THREAD_P CostMethodHbaseDelete *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodHbaseDelete();
  return m;
} // HbaseDelete::costMethod()

PhysicalProperty*
HbaseDelete::synthPhysicalProperty(const Context* myContext,
                                   const Lng32     planNumber,
                                   PlanWorkSpace  *pws)
{
  const ReqdPhysicalProperty* rppForMe =
          myContext->getReqdPhysicalProperty();
  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();

  //------------------------------------------------------------
  // Synthesize a partitioning function with a single partition
  // unless we have a partitioning requirement that says 
  // otherwise.
  //------------------------------------------------------------
  PartitioningFunction* myPartFunc = NULL;

  if (partReqForMe &&
      partReqForMe->castToRequireReplicateNoBroadcast())
    {
      myPartFunc =
        partReqForMe->castToRequireReplicateNoBroadcast()->
        getPartitioningFunction()->copy();
    }
  else
    {
      // Create a node map with a single, active, wild-card entry.
      NodeMap* myNodeMap = new(CmpCommon::statementHeap())
                               NodeMap(CmpCommon::statementHeap(),
                               1,
                               NodeMapEntry::ACTIVE);
      myPartFunc = new(CmpCommon::statementHeap())
      SinglePartitionPartitioningFunction(myNodeMap);
    }

  PhysicalProperty * sppForMe =
    new(CmpCommon::statementHeap())
    PhysicalProperty(myPartFunc,
                     EXECUTE_IN_MASTER_AND_ESP,
                     SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe ;

} // HbaseDelete::synthPhysicalProperty()

CostMethod *
HbaseUpdate::costMethod() const
{
  if (CmpCommon::getDefault(HBASE_UPDATE_COSTING) == DF_OFF)
    // RelExpr::costMethod() costing returns a cost 1 cost object. This
    // is the old behavior before the new costing code was written.
    return RelExpr::costMethod();

  static THREAD_P CostMethodHbaseUpdate *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodHbaseUpdate();
  return m;
} // HbaseUpdate::costMethod()

PhysicalProperty*
HbaseUpdate::synthPhysicalProperty(const Context* myContext,
                                   const Lng32     planNumber,
                                   PlanWorkSpace  *pws)
{
  const ReqdPhysicalProperty* rppForMe =
          myContext->getReqdPhysicalProperty();
  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();

  //------------------------------------------------------------
  // Synthesize a partitioning function with a single partition
  // unless we have a partitioning requirement that says 
  // otherwise.
  //------------------------------------------------------------
  PartitioningFunction* myPartFunc = NULL;

  if (partReqForMe &&
      partReqForMe->castToRequireReplicateNoBroadcast())
    {
      myPartFunc =
        partReqForMe->castToRequireReplicateNoBroadcast()->
        getPartitioningFunction()->copy();
    }
  else
    {
      // Create a node map with a single, active, wild-card entry.
      NodeMap* myNodeMap = new(CmpCommon::statementHeap())
                               NodeMap(CmpCommon::statementHeap(),
                               1,
                               NodeMapEntry::ACTIVE);
      myPartFunc = new(CmpCommon::statementHeap())
      SinglePartitionPartitioningFunction(myNodeMap);
    }

  PhysicalProperty * sppForMe =
    new(CmpCommon::statementHeap())
    PhysicalProperty(myPartFunc,
                     EXECUTE_IN_MASTER_AND_ESP,
                     SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe ;

} // HbaseUpdate::synthPhysicalProperty()

PhysicalProperty*
HiveInsert::synthPhysicalProperty(const Context* myContext,
                                  const Lng32     planNumber,
                                  PlanWorkSpace  *pws)
{

  const ReqdPhysicalProperty* rppForMe =
                                     myContext->getReqdPhysicalProperty();

  PartitioningRequirement * partReq = rppForMe->getPartitioningRequirement();

  // variables that help to come up with my partitioning function

  const LogicalPartitioningRequirement *lpr =
    rppForMe->getLogicalPartRequirement();

  PartitioningRequirement * logPartReq = NULL;

  PlanExecutionEnum location = EXECUTE_IN_MASTER_AND_ESP;

  PartitioningFunction* myPartFunc;
  if ( partReq->isRequirementFullySpecified() ) {

     FullySpecifiedPartitioningRequirement* fpr = 
                (FullySpecifiedPartitioningRequirement*)
                (partReq->castToFullySpecifiedPartitioningRequirement());

     myPartFunc = fpr -> getPartitioningFunction();

  } else
     myPartFunc = getIndexDesc()->getPartitioningFunction();


  PhysicalProperty *pp = new(CmpCommon::statementHeap()) 
                   PhysicalProperty(myPartFunc, location);

  return pp;

} // HiveInsert::synthPhysicalProperty()


CostMethod *
HbaseInsert::costMethod() const
{
  static THREAD_P CostMethodHbaseInsert *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodHbaseInsert();
  return m;
} // HbaseInsert::costMethod()


PhysicalProperty*
HbaseInsert::synthPhysicalProperty(const Context* myContext,
                                   const Lng32     planNumber,
                                   PlanWorkSpace  *pws)
{
  const ReqdPhysicalProperty* rppForMe =
          myContext->getReqdPhysicalProperty();
  PartitioningRequirement* partReqForMe =
    rppForMe->getPartitioningRequirement();

  //------------------------------------------------------------
  // Synthesize a partitioning function with a single partition
  // unless we have a partitioning requirement that says 
  // otherwise.
  //------------------------------------------------------------
  PartitioningFunction* myPartFunc = NULL;

  if (partReqForMe &&
      partReqForMe->castToRequireReplicateNoBroadcast())
    {
      myPartFunc =
        partReqForMe->castToRequireReplicateNoBroadcast()->
        getPartitioningFunction()->copy();
    }
  else
    {
      // Create a node map with a single, active, wild-card entry.
      NodeMap* myNodeMap = new(CmpCommon::statementHeap())
                               NodeMap(CmpCommon::statementHeap(),
                               1,
                               NodeMapEntry::ACTIVE);
      myPartFunc = new(CmpCommon::statementHeap())
      SinglePartitionPartitioningFunction(myNodeMap);
    }

  PhysicalProperty * sppForMe =
    new(CmpCommon::statementHeap())
    PhysicalProperty(myPartFunc,
                     EXECUTE_IN_MASTER_AND_ESP,
                     SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe ;


} // HbaseInsert::synthPhysicalProperty()

// -----------------------------------------------------------------------
// member functions for class PhyPack
// -----------------------------------------------------------------------

CostMethod* PhyPack::costMethod() const
{
  // Zero costs for now.
  static THREAD_P CostMethodFixedCostPerRow *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodFixedCostPerRow(0.,0.,0.);
  return m;
}

//<pb>
//==============================================================================
//  Synthesize physical properties for Pack operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
//==============================================================================
PhysicalProperty*
PhyPack::synthPhysicalProperty(const Context* context,
                               const Lng32     planNumber,
                               PlanWorkSpace  *pws)
{
/*
  PlanExecutionEnum planExecutionLocation;

  // Get child's properties
  PhysicalProperty const *sppOfChild =
            context->getPhysicalPropertyOfSolutionForChild(0);

  // Execute in DP2 if required
  if ( context->getReqdPhysicalProperty()->executeInDP2()
     )
  {
    planExecutionLocation = EXECUTE_IN_DP2;
  }
  else {
    planExecutionLocation = sppOfChild->getPlanExecutionLocation();
  }

  PhysicalProperty* sppForMe =
  new (CmpCommon::statementHeap())
    PhysicalProperty(sppOfChild->getSortKey(),
                     sppOfChild->getSortOrderType(),
                     sppOfChild->getDp2SortOrderPartFunc(),
                     sppOfChild->getPartitioningFunction(),
                     planExecutionLocation,
                     sppOfChild->getDataSourceEnum(),
                     sppOfChild->getIndexDesc(),
                     sppOfChild->getPartSearchKey()
		    );
*/

  // ---------------------------------------------------------------------
  // Simply propogate child's physical property for now. After packing,
  // there should be no more order and stuffs though.
  // ---------------------------------------------------------------------
  const PhysicalProperty* const sppOfTheChild =
                    context->getPhysicalPropertyOfSolutionForChild(0);

  PhysicalProperty* sppForMe = new (CmpCommon::statementHeap())
    PhysicalProperty (*sppOfTheChild);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe;
}


// -----------------------------------------------------------------------
// PhyCompoundStmt::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
PhysCompoundStmt::costMethod() const
{
  static THREAD_P CostMethodCompoundStmt *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodCompoundStmt();
  return m;
}

PhysicalProperty* PhysCompoundStmt::synthPhysicalProperty(const Context* context,
                                                          const Lng32 /*unused*/,
                                                          PlanWorkSpace  *pws)
{
  const PhysicalProperty* const sppOfLeftChild =
                     context->getPhysicalPropertyOfSolutionForChild(0);

  // ---------------------------------------------------------------------
  // Call the default implementation (RelExpr::synthPhysicalProperty())
  // to synthesize the properties on the number of cpus.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(context,0, pws);

  // ---------------------------------------------------------------------
  // The result of a compound statement has the sort order of the left
  // child. The nested join maintains the partitioning of the left child
  // as well.
  // ---------------------------------------------------------------------
  PhysicalProperty* sppForMe =
  new (CmpCommon::statementHeap())
    PhysicalProperty(sppOfLeftChild->getSortKey(),
                     sppOfLeftChild->getSortOrderType(),
                     sppOfLeftChild->getDp2SortOrderPartFunc(),
                     sppOfLeftChild->getPartitioningFunction(),
                     sppOfLeftChild->getPlanExecutionLocation(),
                     sppOfLeftChild->getDataSourceEnum(),
                     sppOfLeftChild->getIndexDesc(),
                     sppOfLeftChild->getPartSearchKey(),
                     sppOfLeftChild->getPushDownProperty());

  sppForMe->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  delete sppTemp;
  return sppForMe;

} // CompoundStmt::synthPhysicalProperty()

//<pb>
Context* CompoundStmt::createContextForAChild(Context* myContext,
                                              PlanWorkSpace* pws,
                                              Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  // If one Context has been generated for each child, return NULL
  // to signal completion.
  // ---------------------------------------------------------------------
  if (pws->getCountOfChildContexts() == getArity())
    return NULL;

  childIndex = pws->getCountOfChildContexts();
  Lng32 planNumber = 0;
  Context* childContext = NULL;
  const ReqdPhysicalProperty* rppForMe = myContext->getReqdPhysicalProperty();

  RequirementGenerator rg(child(childIndex),rppForMe);

  if ( rppForMe->getPushDownRequirement() == NULL AND
       rppForMe->executeInDP2() AND
       childIndex == 0
     )
  {
   // This must be the top CS and we are optimizing the left child.
   // Add a null-state required push-down property.
    rg.addPushDownRequirement(
	new (CmpCommon::statementHeap()) PushDownCSRequirement()
			     );
  }

  if (
      childIndex == 1 // for right child
      AND
      // a plan has been produced by latest context
      (childContext = pws->getChildContext(0, 0)) != NULL
       //(pws->getLatestChildIndex(), pws->getLatestPlan())) != NULL
      AND
      childContext->hasOptimalSolution()
      )
  {
    // clean the rg.
    rg.removeAllPartitioningRequirements();

    // Since we do not impose the sortKey or arragement of the
    // left child to the right, we have to remove them from the
    // requirement.
    rg.removeSortKey();
    rg.removeArrangement();

    const PhysicalProperty*
      sppForChild = childContext->getPhysicalPropertyForSolution();

    CMPASSERT(sppForChild != NULL);

    if ( rppForMe -> executeInDP2() )
    {
       CMPASSERT(sppForChild->getPushDownProperty() != NULL);

       // Add the push-down requirement if the left child echos back
       // the push down requirement.
          rg.addPushDownRequirement(
	     sppForChild->getPushDownProperty()->makeRequirement());
    } else {
    // ---------------------------------------------------------------
    // spp should have been synthesized for child's optimal plan.
    // ---------------------------------------------------------------

       PartitioningFunction* childPartFunc =
	  sppForChild->getPartitioningFunction();

       PartitioningRequirement* partReqForChild =
          childPartFunc->makePartitioningRequirement();

       CMPASSERT(partReqForChild->
                   castToFullySpecifiedPartitioningRequirement());

    // for above DP2 cases, we need the part req.
    //
    // Note ESP parallism would not work here as partKey is not mapped.
    //
       rg.addPartRequirement(partReqForChild);
    }

  }


  // ---------------------------------------------------------------------
  // If this is a CPU or memory-intensive operator or if we could benefit
  // from parallelism by performing multiple sort groupbys on different
  // ranges at the same time, then add a requirement for a minimum number
  // of partitions, unless that requirement would conflict with our
  // parent's requirement.
  //
  // Don't add a required number of partitions if we must execute in DP2,
  // because you can't change the number of DP2s.
  // Also don't specify a required number of partitions for a scalar
  // aggregate, because a scalar aggregate cannot execute in parallel.
  // ---------------------------------------------------------------------
  // and give up if it is not possible to satisfy them
  // ---------------------------------------------------------------------
  if (NOT rg.checkFeasibility())
    return NULL;

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);

  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as those in
  // rppForChild. Reuse it, if found. Otherwise, create a new Context
  // that contains rppForChild as the required physical properties..
  // ---------------------------------------------------------------------
  Context* result = shareContext(
         childIndex,
         rg.produceRequirement(),
         myContext->getInputPhysicalProperty(),
         costLimit,
         myContext,
         myContext->getInputLogProp());

  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, planNumber, result);

  return result;

} // CompoundStmt::createContextForAChild()

Context* Pack::createContextForAChild(Context* myContext,
                                             PlanWorkSpace* pws,
                                             Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  // If one Context has been generated for each child, return NULL
  // to signal completion.
  // ---------------------------------------------------------------------
  if (pws->getCountOfChildContexts() == getArity())
    return NULL;

  childIndex = 0;

  Lng32 planNumber = 0;
  const ReqdPhysicalProperty* rppForMe = myContext->getReqdPhysicalProperty();

  RequirementGenerator rg(child(0),rppForMe);

  // ---------------------------------------------------------------------
  // Add the order requirements needed for this RelPack node
  // ---------------------------------------------------------------------
  // Shouldn't/Can't add a sort order type requirement
  // if we are in DP2
  if (rppForMe->executeInDP2())
    rg.addSortKey(requiredOrder(),NO_SOT);
  else
    rg.addSortKey(requiredOrder(),ESP_SOT);

  // rg.addLocationRequirement(EXECUTE_IN_ESP);

   // Cannot execute in parallel
   //

   // Do not impose single part requirement if executed in DP2.
   if ( NOT rppForMe->executeInDP2() OR NOT isinBlockStmt() )
     rg.addNumOfPartitions(1);

  // ---------------------------------------------------------------------
  // Done adding all the requirements together, now see whether it worked
  // and give up if it is not possible to satisfy them
  // ---------------------------------------------------------------------
  if (NOT rg.checkFeasibility())
    return NULL;

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);

  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as those in
  // rppForChild. Reuse it, if found. Otherwise, create a new Context
  // that contains rppForChild as the required physical properties..
  // ---------------------------------------------------------------------
  Context* result = shareContext(
         childIndex,
	 rg.produceRequirement(),
         myContext->getInputPhysicalProperty(),
	 costLimit,
	 myContext,
	 myContext->getInputLogProp());

  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, planNumber, result);

  return result;

} // Pack::createContextForAChild()

// -----------------------------------------------------------------------
// IsolatedScalarUDF::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
IsolatedScalarUDF::costMethod() const
{
  static THREAD_P CostMethodIsolatedScalarUDF *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodIsolatedScalarUDF();
  return m;
}

// -----------------------------------------------------------------------
// PhysicalIsolatedScalarUDF::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod*
PhysicalIsolatedScalarUDF::costMethod() const
{
  static THREAD_P CostMethodIsolatedScalarUDF *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodIsolatedScalarUDF();
  return m;
}

//==============================================================================
//  Synthesize physical properties for IsolatedScalarUDF operator's current plan
// extracted from a specified context.
//
// Input:
//  myContext  -- specified context containing this operator's current plan.
//
//  planNumber -- plan's number within the plan workspace.  Used optionally for
//                 synthesizing partitioning functions but unused in this
//                 derived version of synthPhysicalProperty().
//
// Output:
//  none
//
// Return:
//  Pointer to this operator's synthesized physical properties.
//
// Note this is a close copy of the synthPhysicalProperties for Tuple,
// so if we change anything here we should evaluate if the Tuple method
// should change as well.
//
//==============================================================================
PhysicalProperty*
IsolatedScalarUDF::synthPhysicalProperty(const Context* myContext,
                                         const Lng32     planNumber,
                                         PlanWorkSpace  *pws)
{
  CMPASSERT(myContext != NULL);

  PartitioningRequirement *myPartReq = NULL;
  PartitioningFunction *myPartFunc = NULL;

  const ReqdPhysicalProperty* rppForMe = myContext->getReqdPhysicalProperty();

  // -----------------------------------------------------------------
  // decide on my partitioning function
  // -----------------------------------------------------------------
  myPartReq = rppForMe->getPartitioningRequirement();

  if (myPartReq ) {
    myPartFunc = myPartReq->realize(myContext);
    
    CMPASSERT(myPartFunc != NULL);
  
    // we cannot execute in DP2 nor can we do some of these partitioning 
    // schemes.
    if ( rppForMe->executeInDP2() ||
         (NOT rppForMe->executeInDP2() &&
           (myPartFunc -> castToHashPartitioningFunction()  ||
            myPartFunc -> castToTableHashPartitioningFunction()  ||
            myPartFunc -> castToRangePartitioningFunction()  ||
            myPartFunc -> castToRoundRobinPartitioningFunction()
           ) && myPartFunc -> getCountOfPartitions() > 1
         )
       )
       return NULL;
  } else {

    myPartFunc = new(CmpCommon::statementHeap())
          SinglePartitionPartitioningFunction();
  }

  // Make sure the IsolatedScalarUDF node only produces its output when it is
  // part of the partition to be produced. Do this by applying the
  // partitioning key predicates which filter out the data of that
  // partition.
  if (myPartFunc->canProducePartitioningKeyPredicates())
    {
      myPartFunc->createPartitioningKeyPredicates();

      if ( NOT rppForMe->executeInDP2() )
        {
        // for single partition and replication the predicates will be empty
        if (NOT myPartFunc->getPartitioningKeyPredicates().isEmpty())
          {
            // ||opt we would need to add the partitioning key
            // predicates, but I see in the executor that no
            // predicates are evaluated on the IsolatedScalarUDF node.  This may
            // be an unrelated bug or a problem for this case. For now
            // refuse to generate any partitioning scheme that has
            // partitioning key predicates
            return NULL;
          }
        }
    }
  else
    {
      // A leaf node can only produce a partitioning function if
      // it can apply its partitioning key predicates. If we can't
      // produce the data we better return NULL physical props.
      return NULL;
    }

  // If partitioning function has no node map already, create a new node map
  // having wild-card entries for each partition.
  if (myPartFunc->getNodeMap() == 0)
    {
      NodeMap* myNodeMap = new(CmpCommon::statementHeap())
                            NodeMap(CmpCommon::statementHeap(),
                                    myPartFunc->getCountOfPartitions(),
                                    NodeMapEntry::ACTIVE);
      myPartFunc->replaceNodeMap(myNodeMap);
    }

  const PushDownRequirement* pdr = rppForMe->getPushDownRequirement();

  // Disable inserts under exchange if the compound statement is not
  // under the same exchange.
  if ( isinBlockStmt() AND rppForMe->executeInDP2() AND
       NOT PushDownCSRequirement::isInstanceOf(pdr)
     )
    return 0;

  if ( pdr )
  {
     // If the IsolatedScalarUDF is the first statement (left most node) or
     // immediately below an EXCHANGE, do not allow it.
     if (pdr->isEmpty()== TRUE)
       return 0;

  }

    // XXX Verify that AP is what ANY will translate to
  NABoolean canRunInParallel = (getRoutineDesc()->getEffectiveNARoutine()->
                                                 getParallelism() == "AP");

  PlanExecutionEnum planExecutionLocation = canRunInParallel ? 
                                             EXECUTE_IN_MASTER_AND_ESP : 
                                             EXECUTE_IN_MASTER;
  PhysicalProperty* sppForMe =
    new (CmpCommon::statementHeap())
      PhysicalProperty(myPartFunc,
                       planExecutionLocation,
                       SOURCE_TUPLE);

   PushDownProperty* pushDownProperty = 0;

   if ( pdr && !pdr->isEmpty() )
    {
      const PushDownCSRequirement*
         pdcsr = pdr->castToPushDownCSRequirement();

      if ( pdcsr )
      {
      // generate a CS push-down property.
         pushDownProperty = new (CmpCommon::statementHeap())
            PushDownCSProperty(pdcsr->getPartFunc(), pdcsr->getSearchKey());
      } else {

         const PushDownColocationRequirement*
            pdclr = pdr->castToPushDownColocationRequirement();

         // generate a colocation push-down property.
         if ( pdclr )
         {
            pushDownProperty = new (CmpCommon::statementHeap())
               PushDownColocationProperty(pdclr->getNodeMap());
         } else
            CMPASSERT(1==0);
      }
    }

   sppForMe->setPushDownProperty(pushDownProperty);

  // -----------------------------------------------------------------------
  // Estimate the number of cpus executing executing copies of this
  // instance:
  // -----------------------------------------------------------------------
  Lng32 countOfCPUs = 1;

  Lng32 countOfStreams = 1;
  if (myPartFunc != NULL)
    countOfStreams = myPartFunc->getCountOfPartitions();

  Lng32  countOfAvailableCPUs = 1;
  if (myContext->getReqdPhysicalProperty())
    countOfAvailableCPUs =
      myContext->getReqdPhysicalProperty()->getCountOfAvailableCPUs();

  // The number of CPUs is limited by the number of streams
  countOfCPUs = (countOfStreams < countOfAvailableCPUs ?
                      countOfStreams : countOfAvailableCPUs);
  CMPASSERT(countOfCPUs >= 1);

  // A IsolatedScalarUDF operator does not execute in DP2. Still, it is
  // a leaf operator. So, set the currentCountOfCPUs in the spp for
  // IsolatedScalarUDF to IsolatedScalarUDF's count of cpus. This way, if 
  // any of IsolatedScalarUDF's parents decide to base their degree of 
  // parallelism on their children they can get their child's degree of 
  // parallelism from the currentCountOfCPUs field of their child's spp, 
  // regardless of whether the child is really a DP2 operator or not.
  sppForMe->setCurrentCountOfCPUs(countOfCPUs);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe;

} //  IsolatedScalarUDF::synthPhysicalProperty()


PhysicalProperty *CallSP::synthPhysicalProperty(const Context* context,
                                                const Lng32 /*unused*/,
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
    new(CmpCommon::statementHeap()) PhysicalProperty(
         myPartFunc,
         EXECUTE_IN_MASTER,
         SOURCE_VIRTUAL_TABLE);

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr()) ;

  return sppForMe ;
} // CallSP::synthPhysicalProperty()



DefaultToken TableMappingUDF::getParallelControlSettings (
            const ReqdPhysicalProperty* const rppForMe, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) const 
{ 
  return RelExpr::getParallelControlSettings(rppForMe, 
        numOfESPs, allowedDeviation, numOfESPsForced ); 
};

NABoolean TableMappingUDF::okToAttemptESPParallelism (
            const Context* myContext, /*IN*/
            PlanWorkSpace* pws, /*IN*/
            Lng32& numOfESPs, /*OUT*/
            float& allowedDeviation, /*OUT*/
            NABoolean& numOfESPsForced /*OUT*/) 
{ 
  const ReqdPhysicalProperty* rppForMe = myContext->getReqdPhysicalProperty();

  // call the base class method
  NABoolean result = RelExpr::okToAttemptESPParallelism(myContext, 
        pws, numOfESPs, allowedDeviation, numOfESPsForced);

  Lng32 reqdNumOfPartitions = (rppForMe->requiresPartitioning() ?
                               rppForMe->getCountOfPartitions() :
                               ANY_NUMBER_OF_PARTITIONS);
  int udfDoP = 0;

  // also ask the UDF what DoP it would like
  NABoolean status = dllInteraction_->degreeOfParallelism(
       this, (TMUDFPlanWorkSpace *) pws, udfDoP);

  if (udfDoP != 0 && udfDoP != numOfESPs && !numOfESPsForced)
    {
      // the UDF cares about parallelism and it did not
      // return the same DoP as suggested by the base class method
      // (and we are not forcing the # of ESPs)
      DefaultToken parallelControlSetting =
        CURRSTMT_OPTDEFAULTS->attemptESPParallelism();
      Lng32 maxDoP = CURRSTMT_OPTDEFAULTS->getMaximumDegreeOfParallelism();

      switch (udfDoP)
        {
        case tmudr::UDRPlanInfo::MAX_DEGREE_OF_PARALLELISM:
          // UDF desires a DoP of maxDoP
          if (result)
            udfDoP = maxDoP;
          break;

        case tmudr::UDRPlanInfo::ONE_INSTANCE_PER_NODE:
          // override base class implementation and CQDs
          parallelControlSetting = DF_ON;
          numOfESPs =
          udfDoP =
          maxDoP = gpClusterInfo->numOfSMPs();
          numOfESPsForced = TRUE;
          allowedDeviation = 0.0;
          result = TRUE;
          break;

        case 1:
          // UDF wants serial execution
          numOfESPs = 1;
          numOfESPsForced = TRUE;
          result = FALSE;
          break;

        case tmudr::UDRPlanInfo::DEFAULT_DEGREE_OF_PARALLELISM:
          udfDoP = reqdNumOfPartitions;
          break;

        default:
          // leave all values unchanged
          break;
        }

      if (result)
        // try to reconcile the two different DoPs
        // - if parallelism is OFF, ignore UDF parallelism
        switch (parallelControlSetting)
          {
          case DF_OFF:
            // this overrides the UDF method
            if (!numOfESPsForced)
              {
                result = FALSE;
                numOfESPs = 1;
              }
            break;

          case DF_SYSTEM:
          case DF_ON:
          case DF_MAXIMUM:
          default:
            {
              if (!numOfESPsForced)
                if (parallelControlSetting == DF_SYSTEM &&
                    reqdNumOfPartitions != ANY_NUMBER_OF_PARTITIONS ||
                    udfDoP == ANY_NUMBER_OF_PARTITIONS)
                  {
                    // if CQD is SYSTEM and parent requires
                    // a degree of ||ism, go with that
                    numOfESPs = rppForMe->getCountOfPipelines();
                    allowedDeviation = 1.0;
                  }
                else
                  {
                    // allow udfDoP, up to 4 * max. degree of parallelism
                    numOfESPs = MINOF(
                         udfDoP,
                         4*maxDoP);

                    if (numOfESPs == udfDoP)
                      // if we chose the exact DoP requested by the UDF, then
                      // stick with the number the UDF specified, no deviation
                      allowedDeviation = 0.0;
                  }
            }
            break;
          }
    }

  return result;
}

PartitioningFunction* TableMappingUDF::mapPartitioningFunction(
                          const PartitioningFunction* partFunc,
                          NABoolean rewriteForChild0) 
{ 
  return RelExpr::mapPartitioningFunction(partFunc, rewriteForChild0);
};

NABoolean TableMappingUDF::isBigMemoryOperator(const PlanWorkSpace* pws,
                                               const Lng32 /*planNumber*/)
{
  NABoolean result = FALSE;
  const Context* context = pws->getContext();
  const TMUDFPlanWorkSpace *udfPWS = static_cast<const TMUDFPlanWorkSpace *>(pws);
  int udfWriterDop = tmudr::UDRPlanInfo::ANY_DEGREE_OF_PARALLELISM;

  if (udfPWS->getUDRPlanInfo())
    udfWriterDop = udfPWS->getUDRPlanInfo()->getDesiredDegreeOfParallelism();

  if (udfWriterDop > 0)
    {
      // the UDF writer specified a desired degree of parallelism,
      // this means that the UDF needs parallelism, so it is a BMO
      result = TRUE;
    }
  else
    {
      // values <= 0 indicate special instructions for the DoP,
      // defined as enum values in file ../sqludr/sqludr.h
      switch (udfWriterDop)
        {
        case tmudr::UDRPlanInfo::MAX_DEGREE_OF_PARALLELISM:
        case tmudr::UDRPlanInfo::ONE_INSTANCE_PER_NODE:
          result = TRUE;
          break;

        default:
          // leave result at FALSE
          break;
        }
    }

  return result;
};

// -----------------------------------------------------------------------
// PhysicalTableMappingUDF::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this class.
// -----------------------------------------------------------------------
CostMethod* PhysicalTableMappingUDF::costMethod() const
{
  static THREAD_P CostMethodTableMappingUDF *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap()) CostMethodTableMappingUDF();
  return m;
} 

PlanWorkSpace * PhysicalTableMappingUDF::allocateWorkSpace() const
{
  PlanWorkSpace *result =
    new(CmpCommon::statementHeap()) TMUDFPlanWorkSpace(getArity());

  return result;
}

Context* PhysicalTableMappingUDF::createContextForAChild(Context* myContext,
                     PlanWorkSpace* pws,
                     Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  // If one Context has been generated for each child, return NULL
  // to signal completion. This will also take care of 0 child case.
  // ---------------------------------------------------------------------
  childIndex = pws->getCountOfChildContexts();

  if (childIndex == getArity())
    return NULL;

  Lng32 planNumber = 0;
  const ReqdPhysicalProperty* rppForMe = myContext->getReqdPhysicalProperty();
  Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
  float childNumPartsAllowedDeviation = 0.0;
  NABoolean numOfESPsForced = FALSE;

  RequirementGenerator rg(child(childIndex),rppForMe);
  TableMappingUDFChildInfo * childInfo = getChildInfo(childIndex);
  TMUDFInputPartReq childPartReqType = childInfo->getPartitionType();
  PartitioningRequirement* partReqForChild = NULL;

  NABoolean useAParallelPlan = okToAttemptESPParallelism(
       myContext,
       pws,
       childNumPartsRequirement,
       childNumPartsAllowedDeviation,
       numOfESPsForced);

  // add PARTITION BY as a required partitioning key
  if (useAParallelPlan)
    if (childPartReqType == SPECIFIED_PARTITIONING)
      {
        // if some specified partitioning is to be required from the child
        // then the required partitioning columns should be mentioned
        CMPASSERT(NOT childInfo->getPartitionBy().isEmpty());
        rg.addPartitioningKey(childInfo->getPartitionBy());
      }
    else if(childPartReqType == REPLICATE_PARTITIONING)
      {
        // get the number of replicas
        // for right now just get what ever number of streams the parent requires
        Lng32 countOfPartitions = childNumPartsRequirement;
        if(rppForMe->getPartitioningRequirement() &&
           (countOfPartitions < rppForMe->getCountOfPartitions()))
          countOfPartitions = rppForMe->getCountOfPartitions();

        if(countOfPartitions > 1)
          partReqForChild = new (CmpCommon::statementHeap() )
            RequireReplicateViaBroadcast(countOfPartitions);
        else
          partReqForChild = new(CmpCommon::statementHeap())
            RequireExactlyOnePartition();

        rg.addPartRequirement(partReqForChild);
      }

  // Since we treat a TMUDF like a MapReduce operator, we ensure
  // that the TMUDF sees all values of a particular partition
  // together. We do that by requesting an arrangement by th
  // PARTITION BY columns, if any are specified. This applies
  // to parallel and serial plans. Suppress this if the function
  // type is REDUCER_NC. In this case, the UDF has specifically
  // asked not to sort. This typically means that the UDF maintains
  // a hash table (or similar) of keys that allow it to receive its
  // input data in any order, with values for different PARTITION BY
  // keys being intermingled. This can be much more efficient when
  // there are many input rows and a much smaller number of unique
  // PARTITION BY keys. So, a REDUCER_NC is similar to a user-defined
  // hash groupby.
  if (invocationInfo_->getFuncType() != tmudr::UDRInvocationInfo::REDUCER_NC)
    rg.addArrangement(childInfo->getPartitionBy(),ESP_SOT);

  // add ORDER BY as a required order
  if (NOT childInfo->getOrderBy().isEmpty())
  {
     ValueIdList sortKey(getChildInfo(0)->getPartitionBy());
     for (Int32 i=0;i<(Int32)getChildInfo(0)->getOrderBy().entries();i++)
      sortKey.insert(getChildInfo(0)->getOrderBy()[i]);
     rg.addSortKey(sortKey, ESP_SOT);
  }

  // add requirement for the degree of parallelism
  if (useAParallelPlan)
  {
    if (NOT numOfESPsForced)
      rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                &childNumPartsAllowedDeviation);
    rg.addNumOfPartitions(childNumPartsRequirement,
                          childNumPartsAllowedDeviation);
  }
  else
    rg.addNumOfPartitions(1);

  // ---------------------------------------------------------------------
  // Done adding all the requirements together, now see whether it worked
  // and give up if it is not possible to satisfy them
  // ---------------------------------------------------------------------
  if (NOT rg.checkFeasibility())
    {
      // remember this so that we can give an appropriate error in case
      // we fail to produce a plan
      char reason[250];

      snprintf(reason,
               sizeof(reason),
               "%s, use %d parallel streams in context %s",
               getUserTableName().getCorrNameAsString().data(),
               childNumPartsRequirement,
               myContext->getRPPString().data());

      CmpCommon::statement()->setTMUDFRefusedRequirements(reason);
      return NULL;
    }

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);

  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as those in
  // rppForChild. Reuse it, if found. Otherwise, create a new Context
  // that contains rppForChild as the required physical properties..
  // ---------------------------------------------------------------------
  Context* result = shareContext(
         childIndex,
         rg.produceRequirement(),
         myContext->getInputPhysicalProperty(),
         costLimit,
         myContext,
         myContext->getInputLogProp());


  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, planNumber, result);

  return result;
};

PhysicalProperty* PhysicalTableMappingUDF::synthPhysicalProperty(
     const Context* myContext,
     const Lng32    planNumber,
     PlanWorkSpace  *pws)
{
  PartitioningFunction* myPartFunc = NULL;
  Int32 arity = getArity();
  Lng32 numOfESPs = 0;
  NABoolean createSinglePartFunc = FALSE;
  NABoolean createRandomPartFunc = FALSE;

  if (arity == 0)
  {
    // for a TMUDF with no table inputs, call okToAttemptESPParallelism()
    // here to determine the DoP. In the other case where we had table
    // inputs, we did that already in
    // PhysicalTableMappingUDF::createContextForAChild()

    float allowedDeviation = 0.0;
    NABoolean numOfESPsForced = FALSE;

    NABoolean useAParallelPlan = okToAttemptESPParallelism(
         myContext,
         pws,
         numOfESPs,
         allowedDeviation,
         numOfESPsForced);

    if (useAParallelPlan && numOfESPs > 1)
      createRandomPartFunc = TRUE;
    else
      createSinglePartFunc = TRUE;
  }
  else
  {
    const PhysicalProperty * sppOfChild;
    const PartitioningFunction *childPartFunc;
    Int32 childToUse = 0;
    NABoolean foundChildToUse = FALSE;

    // find a child that is not replicated, the first such
    // child will determine our partitioning function
    while (!foundChildToUse && childToUse < arity)
      {
        sppOfChild =
          myContext->getPhysicalPropertyOfSolutionForChild(childToUse);
        childPartFunc =
          sppOfChild->getPartitioningFunction();

        if (childPartFunc &&
            !childPartFunc->isAReplicationPartitioningFunction())
          foundChildToUse = TRUE;
        else
          childToUse++;
      }

    if (!foundChildToUse ||
        childPartFunc->isASinglePartitionPartitioningFunction())
      {
        createSinglePartFunc = TRUE;
      }
    else
      {
        // Check whether the partitioning key of the child is visible
        // in our characteristic outputs. If so, then we can map the
        // first child's partitioning function to our own. Otherwise,
        // we will generate a HASH2 part func on a random number - in
        // other words that's a partitioning function with a known
        // number of partitions but an unknown partitioning key.
        ValueIdSet passThruCols(udfOutputToChildInputMap_.getBottomValues());

        if (passThruCols.contains(childPartFunc->getPartitioningKey()))
          {
            // use a copy of the child's part func, with the key
            // columns remapped to our corresponding pass-through
            // output columns
            myPartFunc = childPartFunc->copyAndRemap(
                 udfOutputToChildInputMap_, TRUE);
          }
        else
          {
            // child's partitioning key is not visible to the
            // parent, create a part func w/o a usable partitioning key
            numOfESPs = childPartFunc->getCountOfPartitions();
            createRandomPartFunc = TRUE;
          }
      } // found a partitioned child
  } // arity > 0

  // a couple of common cases
  if (createSinglePartFunc)
  {
    //----------------------------------------------------------
    // Create a node map with a single, active, wild-card entry.
    //----------------------------------------------------------
    NodeMap* myNodeMap = new(CmpCommon::statementHeap())
      NodeMap(CmpCommon::statementHeap(),
              1,
              NodeMapEntry::ACTIVE);

    myPartFunc = new(CmpCommon::statementHeap())
      SinglePartitionPartitioningFunction(myNodeMap);
  }
  else if (createRandomPartFunc)
  {
    //-----------------------------------------------------------
    // Create a node map with numOfESPs active, wild-card entries
    //-----------------------------------------------------------
    NodeMap* myNodeMap = new(CmpCommon::statementHeap())
      NodeMap(CmpCommon::statementHeap(),
              numOfESPs,
              NodeMapEntry::ACTIVE);

    // set node numbers in the entries, if we need to have one
    // ESP per node
    if (numOfESPs == gpClusterInfo->numOfSMPs())
      for (int i=0; i<numOfESPs; i++)
        myNodeMap->getNodeMapEntry(i)->setNodeNumber(i);

    ValueIdSet partKey;
    ItemExpr *randNum =
      new(CmpCommon::statementHeap()) RandomNum(NULL, TRUE);

    randNum->synthTypeAndValueId();
    partKey.insert(randNum->getValueId());
    myPartFunc = new(CmpCommon::statementHeap())
      Hash2PartitioningFunction (partKey,
                                 partKey,
                                 numOfESPs,
                                 myNodeMap);
    myPartFunc->createPartitioningKeyPredicates();
  }
  
  PhysicalProperty * sppForMe =
    new(CmpCommon::statementHeap()) PhysicalProperty(
         myPartFunc,
         EXECUTE_IN_MASTER_AND_ESP,
          SOURCE_VIRTUAL_TABLE);

  sppForMe->setUDRPlanInfo(
       static_cast<TMUDFPlanWorkSpace *>(pws)->getUDRPlanInfo());

  // remove anything that's not covered by the group attributes
  sppForMe->enforceCoverageByGroupAttributes (getGroupAttr());

  return sppForMe ;
}


//***********************************************************************
//
//
//***********************************************************************
NABoolean RelExpr::isBigMemoryOperator(const PlanWorkSpace *pws,
                                       const Lng32)
{
  const Context* context = pws->getContext();
  const PhysicalProperty* spp = context->getPlan()->getPhysicalProperty();

  if (spp == NULL || CmpCommon::getDefault(COMP_BOOL_51) != DF_ON)
     return FALSE;


  CurrentFragmentBigMemoryProperty * bigMemoryProperty =
                 new (CmpCommon::statementHeap())
                      CurrentFragmentBigMemoryProperty();

  ((PhysicalProperty*) spp)->setBigMemoryEstimationProperty(bigMemoryProperty);
  bigMemoryProperty->setOperatorType(getOperatorType());

  if (getOperatorType() == REL_EXCHANGE)
  {
    bigMemoryProperty->setCurrentFileSize(0);
    bigMemoryProperty->incrementCumulativeMemSize(0);
  }
  else
  {
    //get cumulative file size of the fragment; get the child spp??
    for (Int32 i=0; i<getArity(); i++)
    {
    const PhysicalProperty *childSpp =
       context->getPhysicalPropertyOfSolutionForChild(i);

      if (childSpp != NULL)
      {
       CurrentFragmentBigMemoryProperty * memProp =
         (CurrentFragmentBigMemoryProperty *)
          ((PhysicalProperty *)childSpp)->getBigMemoryEstimationProperty();

       if (memProp != NULL)
       {
         double childCumulativeMemSize = memProp->getCumulativeFileSize();
         bigMemoryProperty->incrementCumulativeMemSize(childCumulativeMemSize);
       }
      }
    }
  }
  return FALSE;

}

PhysicalProperty*
ControlRunningQuery::synthPhysicalProperty(const Context* myContext,
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

  return sppForMe ;

} // ControlRunningQuery::synthPhysicalProperty()
