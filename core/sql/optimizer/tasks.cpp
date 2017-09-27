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
 * File:         tasks.C
 * Description:  Cascades optimizer search engine
 *               Implementation for the CascadesTask class and its subclasses
 * Created:      7/22/94
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "RelExpr.h"
#include "PhyProp.h"
#include "Cost.h"
#include "opt.h"
#include "CmpContext.h"
#include "CmpStatement.h"
#include "TransRule.h"

// xxx tmp include, will be taken out
#include "Analyzer.h"

extern THREAD_P NAUnsigned SortEnforcerRuleNumber;

//<pb>
// -----------------------------------------------------------------------
// Tasks
// -----------------------------------------------------------------------

CascadesTask::CascadesTask(Guidance * guidance, Context * context, 
                           Lng32 parentTaskId, short stride)
            : parentTaskId_(parentTaskId),
	      guidance_(guidance),
	      context_(context),
	      next_(NULL),
              stride_(stride)
{
  // search guidance
  if(guidance_ != NULL)
    guidance_->incrementReferenceCount();
  
} // CascadesTask::CascadesTask

CascadesTask::~CascadesTask()
{
  
  if (guidance_ != NULL)
    guidance_->decrementReferenceCount();
  
} // CascadesTask::~CascadesTask

void CascadesTask::print(FILE *, const char *, const char *) const
{
  ABORT("CascadesTask::print() needs to be redefined");
} // CascadesTask::print

NAString CascadesTask::taskText() const
{
  return "Shouldn't ever see a generic task";
}

CascadesGroupId CascadesTask::getGroupId() const
{
  return INVALID_GROUP_ID;
}
//<pb>

RelExpr * CascadesTask::getExpr()
{
  return NULL;
}

CascadesPlan * CascadesTask::getPlan()
{
  return NULL;
}

void CascadesTask::garbageCollection(RelExpr *,
			     RelExpr *,
			     Int32)
{
  // do nothing, unless the method is redefined
}

// This is really no reason to have this function as a method in
// CascadesTask but I left it as is, for now.
NABoolean CascadesTask::taskNumberExceededLimit(Lng32 id)
{    
  return (id > getDefaultAsLong(OPTIMIZATION_TASKS_LIMIT));
    // note that if OPTIMIZATION_TASKS_LIMIT was inserted incorrectly
    // such as 'w3' the getUnit() will return a -1 as the conversion
    // from string to long; hence inserting OPTIMIZATION_TASKS_LIMIT
    // incorrectly is similar to setting it to "-1".
}


//<pb>
/* ============================================================ */

/*
  Moves
  =====
  Pair of rule and promise for heuristic guidance.
  */

/* Function to compare the promise of rule applications */
Int32 comparePromisingMoves(const void * ax, const void * bx)
{
  
  RuleWithPromise *a = (RuleWithPromise *) ax;
  RuleWithPromise *b = (RuleWithPromise *) bx;
  Int32 result = 0;
  
  if (a->promise > b->promise)
    result = -1;
  else if (a->promise < b->promise)
    result = 1;
  else if (a->tieBreaker > b->tieBreaker)
    result = -1;
  else if (a->tieBreaker < b->tieBreaker)
    result = 1;
  else
    result = 0;
  
  return( result );
} // comparePromisingMoves
//<pb>
/* ============================================================ */

/*
  Task to optimize a group
  */

OptimizeGroupTask::OptimizeGroupTask(Context * context,
				     Guidance * guidance,
				     NABoolean reoptimize,
				     Lng32 parentTaskId, short stride)
                 : CascadesTask(guidance, context, parentTaskId, stride),
		   groupId_(context->getGroupId()),
		   reoptimize_(reoptimize)
{
} // OptimizeGroupTask::OptimizeGroupTask

void OptimizeGroupTask::print(FILE * f, const char * prefix, 
			      const char * suffix) const
{
#ifndef NDEBUG 
  fprintf(f, "%soptimize group %d -- parent task %d subtask %d\n",
	  prefix, groupId_, parentTaskId_, stride_);
  context_->print(f, "\t", suffix);
#endif  //NDEBUG
} // OptimizeGroupTask::print

NAString OptimizeGroupTask::taskText() const
{
  char returnText[100];

  sprintf(returnText,"%d %d Optimize Group %d, ",
          getParentTaskId(), getSubTaskId(), groupId_);
  return NAString(returnText) + context_->getRequirementsString();
}

CascadesGroupId OptimizeGroupTask::getGroupId() const
{
  return groupId_;
}

// method that returns true if scheduling a CreatePlanTask for this plan's 
// physExpr for this context_ can generate a DAG (non-tree) plan
NABoolean OptimizeGroupTask::canCauseCycle(const CascadesPlan *plan) const
{
  if (CURRSTMT_OPTDEFAULTS->maxDepthToCheckForCyclicPlan() < 0) 
    return FALSE;
  // enumerate below the situations that can cause cyclic plans:
  if (plan && plan->getPhysicalExpr() &&
      plan->getPhysicalExpr()->getOperatorType()==REL_EXCHANGE &&
      context_ != plan->getContext() && plan == context_->getSolution()
      // an enforcer stolen from otherContext in previous pass can result in
      // this non-tree plan (if allowed to be reoptimized in current pass)
      //   context_ -> CascadesPlan1 -> Exchange1
      //                 |                  ^
      //                 V                  |
      //               otherContext -> CascadesPlan2
      ) 
    {
      return TRUE; // yes it can result in a DAG (non-tree) plan
    }
  return FALSE;
}

//<pb>
void OptimizeGroupTask::perform(Lng32 taskId)
{
  CascadesGroup * group = (* CURRSTMT_OPTGLOBALS->memo) [groupId_];
  short stride = 0;

  // Rememember the task scheduled just prior to this OptimizeGroupTask
  //context_->setPredecessorTask (getNextTask());

  // --------------------------------------------------------------      
  // if the optimal plan hasn't been determined yet, optimize each expression
  // --------------------------------------------------------------      
  if (NOT context_->findBestSolution() OR reoptimize_)
    {

      // --------------------------------------------------------------      
      // Schedule an ApplyRuleTask for each enforcer rule which
      // has not already been applied for this context.  NOTE:
      // Enforcer rules are unlike other rules in that they are
      // expression-insensitive.  They need only be applied once
      // per context, and not per expression.  
      // --------------------------------------------------------------      
      RuleSubset applRules = GlobalRuleSet->enforcerRules();
      applRules -= context_->getTriedEnforcerRules();

      // This is needed for "nice context"
      NABoolean exchangeEnforcerWillFire = FALSE;
      NABoolean sortEnforcerWillFire = FALSE;
      const ReqdPhysicalProperty * rpp = 
          context_->getReqdPhysicalProperty();
      NABoolean partSortReqPresent = FALSE;
      if ( rpp AND 
           ( rpp->getSortKey() OR
             rpp->getArrangedCols() OR 
             rpp->getPartitioningRequirement()
           )
         )
      {
        partSortReqPresent = TRUE; 
      }
      
      for (CollIndex ruleId = 0; applRules.nextUsed(ruleId); ++ ruleId)
	{
	  Rule * rule = GlobalRuleSet->rule(ruleId);
	  if (rule->topMatch (NULL, context_) AND
	      (rule->promiseForOptimization(NULL,guidance_,context_)) > 0)
            {
              CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
                              ApplyRuleTask(rule,
                                            NULL, /* no assoc. expression */
                                            NULL,
                                            guidance_,
                                            context_,
                                            taskId, ++stride));
              if (rule->getNumber() == SortEnforcerRuleNumber )
              {
                sortEnforcerWillFire = TRUE;
              }
              else
              {
                exchangeEnforcerWillFire = TRUE;
              }
            }		
          else
            // Mark the rules that don't match ... so we won't invoke
            // topMatch() again for this context.
            context_->triedEnforcerRules() += ruleId;
	}

      // --------------------------------------------------------------      
      // Schedule an OptimizeExprTask for each logical expression
      // --------------------------------------------------------------  

      // Rememember the task scheduled just prior to this OptimizeGroupTask
      context_->setPredecessorTask (getNextTask());
  
      // in case parallelHeuristic2 is TRUE we check if the current group is 
      // small (less than ROWS_PARALLEL_THRESHOLD) and has only one base 
      // table to skip re-optimization of logical expressions and existing 
      // plans for this group if the context has partitioning requirements 
      // other than  "exactly one partition". see optimizeExprAndPlans()
      // before returning tasks must delete themselves.
      if ( CURRSTMT_OPTDEFAULTS->parallelHeuristic2() AND 
           NOT CURRSTMT_OPTDEFAULTS->pushDownDP2Requested()  AND
           NOT context_->reoptimizeExprAndPlans() 
         )
      {
        delete this;
        return;
      }

      // This is for "nice context", remember if some expressions
      // will be optimized
      NABoolean optExprTaskWillFire = FALSE;

      RelExpr * expr = group->getFirstLogExpr();
      if (expr != NULL)
	{
	  // optimize each logical expression
	    for ( ;
	        expr != NULL;
	        expr = expr->getNextInGroup())
	    {
	      CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
			      OptimizeExprTask(expr,guidance_,context_,
                                               taskId, ++stride));
              optExprTaskWillFire = TRUE;
	    }
        } // endif (expr != NULL)

      // Simple prototype of "nice context". When group is optimized
      // only transformation rules fire. This means partitioning and sort/arg
      // requirement won't be pushed down below this group but all logical
      // expressions will be created as soon as possible.
      expr = group->getFirstLogExpr();
      if ( context_->isNiceContextEnabled() AND
           partSortReqPresent AND (expr != NULL) AND 
           (expr->getOperatorType() != REL_ROOT) AND
           (context_->getGroupAttr()->getNumBaseTables() > 1) AND 
           ( exchangeEnforcerWillFire OR
             sortEnforcerWillFire //OR optExprTaskWillFire 
           )
         )
      {
        delete this;
        return;
      }

      // --------------------------------------------------------------      
      // Reschedule a CreatePlanTask for reoptimizing each plan for this 
      // context that did not succeed in the current optimization pass.
      // (This will include those plans that were optimized in previous 
      // passes).
      // --------------------------------------------------------------      
      // In re-optimizing plans that have not yet succeeded in current pass,
      // we must guarantee that we see each PhysExpr at most once in a plan
      // tree. Don't re-optimize CascadesPlans that involve a cycle (visits 
      // the same group more than once). Cycles can occur for enforcers and
      // for unnecessary operators (eg, groupbys).
      // In that case we re-optimize in pass2 a 96 partition 
      // exchange plan that was stolen by current context (req: inESP, 96 
      // partitions) from another context (req: inESP).
      // --------------------------------------------------------------      

      Lng32 countOfPlans = group->getCountOfPlans();

      // Loop thru the initial # of plans.  Please note that scheduling
      // the CreatePlanTask will have the side-effect of adding more
      // plans to the end of the plans list.
      for (Lng32 i = 0; i < countOfPlans; i++)
	  {
	    const CascadesPlan * plan = group->getPlans()[i];

        // Reoptimize this plan if it points to this context or
        // if this context's optimal solution points to it, and 
        // it was not successfully optimized in this pass yet. 
        // Note that we need to check if the context's optimal
        // solution points to this plan because if the optimal
        // solution for this context was stolen from another 
        // context in pass1, the plan will not point to this 
        // context, it will point to the context it was stolen from.
        // We must reoptimize this plan so that it becomes a pass2
        // plan and the old pass1 plan is thrown away. Otherwise,
        // if task-based pruning occurs of all the   
        // implementation rules for this context, we could end up
        // with a pass1 plan being the only plan for this context
        // and it is not allowed to have a pass1 plan as the
        // optimal solution for pass2.

        double limit_for_CPT = CURRSTMT_OPTDEFAULTS->level1SafetyNetMultiple();
        if (limit_for_CPT < 0) 
          limit_for_CPT = 0;

        if ( plan != NULL AND 
             ((context_ == plan->getContext()) OR 
               (context_->getSolution() == plan)
              ) AND NOT plan->succeededInCurrentPass() AND 
              // this is for "nice context", we reoptimize only
              // context without part/sort requirements or when
              // there was no enforcer task put on the stack for it
              // otherwise we are stuck with pass1 plan.
              ( (NOT context_->isNiceContextEnabled()) OR
                (NOT partSortReqPresent) OR
                (partSortReqPresent AND NOT
                  (exchangeEnforcerWillFire OR sortEnforcerWillFire) 
                ) OR
                ( (expr != NULL) AND 
                  ( (expr->getOperatorType() == REL_ROOT) OR
                    ( (CmpCommon::getDefault(COMP_BOOL_27) == DF_OFF) AND 
                      ( (expr->getOperatorType() == REL_EXCHANGE) OR
                        (expr->getOperatorType() == REL_SORT) 
                      )
                    )
                  )
                )
              )
             AND canCauseCycle(plan) == FALSE
             // skip CPT if we're way over level1SafetyNet
             AND (limit_for_CPT == 0.0 OR
                  (CURRSTMT_OPTDEFAULTS->getTaskCount() <= 
                   limit_for_CPT * CURRSTMT_OPTDEFAULTS->level1SafetyNet()
                   ))
           )
        {
          // if OPH_REUSE_FAILED_PLAN is ON then reuse plan an existing
          // object structure for the plan that failed instead of
          // creating a new one (which causes failed plan explosion
          // for some big queries). Reusing  existing structure
          // would also allow to reuse some of the properties
          // of failed plan: rollup cost, phys.properties and so on.
	  if ( CURRSTMT_OPTDEFAULTS->OPHreuseFailedPlan() AND 
                ( plan->failedInCurrentPass() OR
                  ( plan->getRollUpCost() == NULL ) )
             )
          { 
             // We don't want to reoptimize plan that has been costed
             // already (succeeded) in the current pass but failed to 
             // satisfy its context. The whole re-optimizing logic can 
             // be reviewed to save time and space. Do we really need 
             // to keep all plans that failed?
             if( NOT plan->failedInCurrentPass() )
               CURRSTMT_OPTGLOBALS->task_list->push (new(CmpCommon::statementHeap())
                  CreatePlanTask (NULL, plan->getPhysicalExpr(),
                                  guidance_, context_, taskId, ++stride,
                                  (CascadesPlan *)plan)); //extra par.
          }         
	  else
            CURRSTMT_OPTGLOBALS->task_list->push (new(CmpCommon::statementHeap())
                CreatePlanTask (NULL, plan->getPhysicalExpr(),
                                guidance_, context_, taskId, ++stride));
        } 
                  
      } // for every plan

    } // if there is no best solution or reoptimize

  // tasks must delete themselves
  delete this;
  
} // OptimizeGroupTask::perform
//<pb>
/* ------------------------------------------------------------ */

OptimizeExprTask::OptimizeExprTask(RelExpr * expr,
				   Guidance * guidance,
				   Context * context,
				   Lng32 parentTaskId, short stride)
                : CascadesTask(guidance, context, parentTaskId, stride),
                  expr_(expr)
{
  context_->incrOutstanding();
} // OptimizeExprTask::OptimizeExprTask

void OptimizeExprTask::print(FILE * f, const char *, const char * suffix) const
{
#ifndef NDEBUG
  fprintf(f, "optimize log-expr -- parent task %d subtask %d\n\t",
	  parentTaskId_, stride_);
  expr_->print(f, "\t");
  context_->print(f, "\t", suffix);
#endif  //NDEBUG
} // OptimizeExprTask::print

NAString OptimizeExprTask::taskText() const
{
  char returnText[100];

#ifdef _DEBUG
  char childrenGroup[12] = "";
  if ( expr_->getArity() == 1 )
    sprintf(childrenGroup, "(%d)", (expr_->child(0)).getGroupId() );
  else if (expr_->getArity() > 1 )
    sprintf(childrenGroup, "(%d,%d)", (expr_->child(0)).getGroupId(),
                                      (expr_->child(1)).getGroupId() );
  childrenGroup[11]='\0';
#endif

  sprintf(returnText," of group %d, ",expr_->getGroupId());
  char prefixText[100];
  sprintf(prefixText, "%d %d Optimize Expression ",
          getParentTaskId(), getSubTaskId());
  return NAString("") + prefixText +  expr_->getText() +

#ifdef _DEBUG
         childrenGroup + 
#endif
         returnText + context_->getRequirementsString();
}

CascadesGroupId OptimizeExprTask::getGroupId() const
{
  return expr_->getGroupId();
}

RelExpr * OptimizeExprTask::getExpr()
{
  return expr_;
}
//<pb>
void OptimizeExprTask::perform(Lng32 taskId)
{
  
  // ---------------------------------------------------------------------
  // identify valid and promising rules
  // ---------------------------------------------------------------------
  RuleWithPromise promisingMove[MAX_RULE_COUNT];
  Int32 numberOfMoves = 0;      // # of moves already collected
  short stride = 0;

  RuleSubset applRules(CmpCommon::statementHeap());

  if (guidance_ != NULL)
    applRules = *guidance_->applicableRules();
  else
    applRules = *GlobalRuleSet->applicableRules();

  // Do not apply enforcer rules for each logical expression.  These are
  // scheduled once per context per group in OptimizeGroupTask.
  applRules -= GlobalRuleSet->enforcerRules();
  applRules -= context_->ignoredRules();

  // Check to see if this rule has already been applied before.
  // For context sensitive rules, check the two-dimensional array which
  // determines whether a rule has been applied for a certain context.

  applRules -= expr_->getContextInsensRules();
  applRules -= expr_->getContextSensRules().getTriedRules(context_);


  // This is needed for "nice context". This would allow only transformation 
  // rules to be fired for this context
  if ( context_->isNiceContextEnabled() )
  {
    const ReqdPhysicalProperty * rpp = context_->getReqdPhysicalProperty();
    if ( rpp AND 
         ( rpp->getSortKey() OR rpp->getArrangedCols() OR 
            ( rpp->getPartitioningRequirement() AND 
              NOT rpp->getPartitioningRequirement()
                  ->castToRequireReplicateNoBroadcast()
            )
         ) AND 
         (context_->getGroupAttr()->getNumBaseTables() > 1)
       )
    {
      applRules -= GlobalRuleSet->implementationRules(); 
    }
  }

////////////////////////////////////xxx
  if (CURRSTMT_OPTDEFAULTS->optimizerHeuristic3())
  {
    NABoolean implement = TRUE;
    const JBBSubset* localView = expr_->getGroupAnalysis()->getLocalJBBView();

    // if this expr has extra XPs
    if (localView &&
        localView->hasMandatoryXP() &&
        !localView->getJBB()->hasMandatoryXP())
	{
      implement = FALSE;
	}

    // or if any of its children has an xp
    Int32 nc = expr_->getArity();
    for (Lng32 i = 0; i < nc; i++)
    {
      const JBBSubset* childlocalView = 
		expr_->child(i).getGroupAttr()->getGroupAnalysis()->getLocalJBBView();

      if (childlocalView &&
          childlocalView->hasMandatoryXP() &&
          !childlocalView->getJBB()->hasMandatoryXP())
	  {
        implement = FALSE;
	  }
    }

    if (!implement)
	{
      // in this case no expr in this group deserve implementation :)
      applRules -= GlobalRuleSet->implementationRules();
  	  applRules -= JoinToTSJRuleNumber;
	}
  }
////////////////////////////////////

  // loop through all applicable rules
  for (CollIndex ruleId = 0;  applRules.nextUsed(ruleId);  ++ ruleId)
  {
    Rule * rule = GlobalRuleSet->rule(ruleId);
    Int32 promise;

    // insert a valid and promising move into the array
    if (rule->topMatch(expr_,context_)  AND
      (promise = rule->promiseForOptimization(expr_,guidance_,context_)) > 0 AND
       // should double check as topMatch of some rules may disable other rules
       NOT expr_->getContextInsensRules().contains(ruleId) AND
       NOT expr_->getContextSensRules().getTriedRules(context_).contains(ruleId))
    {
      // If the context specifies a mustMatch (or FORCE) directive,
      // then check to see whether this rule has a potential for satisfying 
      // the directive.Do this early "pruning" only for context sensitive rules.
      NABoolean checkForce =
        ((context_ != NULL) AND 
         (context_->getReqdPhysicalProperty() != NULL) AND
         (context_->getReqdPhysicalProperty()->getMustMatch() != NULL));

      if (!checkForce OR 
          NOT rule->isContextSensitive() OR
          rule->canMatchPattern(
            context_->getReqdPhysicalProperty()->getMustMatch()))
      {
        promisingMove[numberOfMoves].promise = promise;
        promisingMove[numberOfMoves].tieBreaker = numberOfMoves;
        promisingMove[numberOfMoves++].rule = rule;
      }

    } // insert a valid and promising move into the array
    else
    {
      // Mark this rule as a "tried rule" for the expression.
      // NOTE: don't do this if the rule matches, since other
      // tasks may run between this task and the scheduled ApplyRuleTask
      // task. For matching rules, set the bit when the rule is
      // first applied and then check for duplicate ApplyRuleTask tasks.
      if (rule->isContextSensitive())
          expr_->contextSensRules().addRule (context_, ruleId);
      else
      {
        // Only mark the rule as a tried rule if it is not the
        // JoinToTSJRule in Pass1 for an embedded delete. This rule
        // is only enabled in Pass1 for embedded deletes.
        // Join nodes that aren't embedded deletes still need to have
        // the JoinToTSJRule fire in Pass2, so don't mark the 
        // JoinToTSJRule as a tried rule for them in Pass1.
        if (NOT (GlobalRuleSet->getCurrentPassNumber() == 
		 GlobalRuleSet->getFirstPassNumber() AND 
		 ruleId == JoinToTSJRuleNumber AND 
		 expr_->getArity() > 1 AND
		 (expr_->child(0).getGroupAttr()->isEmbeddedUpdateOrDelete() OR
		  expr_->child(0).getGroupAttr()->isStream())))
        {
          expr_->contextInsensRules() += ruleId;
        }
      }
    } // end if it was not a valid and promising rule

  } // loop through all applicable rules
  
  // order the valid and promising moves in the descending order of their promise
  opt_qsort((char *) promisingMove, numberOfMoves,
	sizeof(RuleWithPromise), comparePromisingMoves);
  
  // determine how many of the promising moves to pursue
  numberOfMoves = expr_->computeOptimizeExprCutoff
                            (promisingMove,numberOfMoves,guidance_,context_);
  
  // ---------------------------------------------------------------------
  // optimize in order of promise
  // ---------------------------------------------------------------------

  while ( -- numberOfMoves >= 0)
    {
      // push future tasks in reverse order(due to LIFO stack)
      
      Rule * rule = promisingMove[numberOfMoves].rule;

      // last task: apply the rule
      CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
		      ApplyRuleTask(rule,
				    expr_,
				    NULL,
				    guidance_,
				    context_,
				    taskId, ++stride));
      
      // create earlier tasks to explore patterns
      RelExpr * pattern = rule->getPattern();

      Lng32 arity = 0;
      if (pattern)
        arity = pattern->getArity();

      for (Lng32 childIndex = arity; -- childIndex >= 0;  )
	{
	  RelExpr * patternInput = (* pattern)[childIndex];
	      
	  // explore only child nodes of the pattern that are neither a
	  // leaf node nor or a tree node requiring only a single binding
	  if (NOT patternInput->isCutOp() AND
	      NOT (patternInput->isSubtreeOp()  AND  NOT
		   ((SubtreeOp *) patternInput)->all_bindings()))
	    {
	      // schedule a task to explore the child group for
	      // the input pattern.

              CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
			      ExploreGroupTask
			      ((* expr_)[childIndex].getGroupId(),
			       patternInput,
			       rule->guidanceForExploringChild
			       (guidance_,
				context_,
				(Int32)childIndex),
			       taskId, ++stride));
	    } // endif expand an input to match the pattern

	} // end for earlier tasks: expand all children to match the pattern
      
    } // while numberOfMoves > 0
  
  // ---------------------------------------------------------------------
  // Determine a cost for an operator that's both log and phys. Such
  // an expression is treated as if it had created itself using an
  // imaginary implementation rule.
  // ---------------------------------------------------------------------
  if (expr_->isPhysical())
    CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
		    CreatePlanTask(NULL, expr_, NULL, context_, 
                                   taskId, ++stride));

  context_->decrOutstanding();

  // tasks must delete themselves
  delete this;
  
} // OptimizeExprTask::perform

void OptimizeExprTask::garbageCollection(RelExpr *oldx,
					 RelExpr *newx,
					 Int32 /* groupMergeCount */)
{
  if (expr_ == oldx)
    expr_ = newx;
} // OptimizeExprTask::garbageCollection
//<pb>
/* ------------------------------------------------------------ */

/*
  Task to explore a group
  */

ExploreGroupTask::ExploreGroupTask(CascadesGroupId group_no,
				   RelExpr * pattern,
				   Guidance * guidance,
				   Lng32 parentTaskId, short stride)
                : CascadesTask(guidance, NULL, parentTaskId, stride),
		  groupId_(group_no),
		  pattern_(pattern)
{
} // ExploreGroupTask::ExploreGroupTask

void ExploreGroupTask::print(FILE * f, const char * prefix, const char * suffix) const
{
#ifndef NDEBUG
  fprintf(f, "%sexplore group %d -- parent task %d subtask %d\n",
	  prefix, groupId_, parentTaskId_, stride_);
  pattern_->print(f, "\tpattern: ", suffix);
#endif  //NDEBUG
} // ExploreGroupTask::print

NAString ExploreGroupTask::taskText() const
{
  char returnText[100];

  sprintf(returnText,"%d %d Explore Group %d, ",
          getParentTaskId(), getSubTaskId(), groupId_);
  return returnText;
}

CascadesGroupId ExploreGroupTask::getGroupId() const
{
  return groupId_;
}
//<pb>
void ExploreGroupTask::perform(Lng32 taskId)
{
  Lng32 currentPass = GlobalRuleSet->getCurrentPassNumber();
  short stride = 0;

  RuleSubset applicableRules(CmpCommon::statementHeap());

  if (guidance_ != NULL)
    applicableRules = *guidance_->applicableRules();
  else
    applicableRules = *GlobalRuleSet->applicableRules();

  // subtract rules that were applied before
  applicableRules -= (*CURRSTMT_OPTGLOBALS->memo)[groupId_]->getExploredRules(); 

  // explore, if not already done so 
  // the first check is realy not necessary now since applicableRules
  // check would cover it anyway
  if (//(*CURRSTMT_OPTGLOBALS->memo)[groupId_]->getExploredInPass() != currentPass OR
      (NOT applicableRules.isEmpty()))
    {
      // explore each logical expression in the group
      for (RelExpr * expr = (* CURRSTMT_OPTGLOBALS->memo) [groupId_]->getFirstLogExpr();
	   expr != NULL;  expr = expr->getNextInGroup())
	if (expr->isLogical())
	  CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
			  ExploreExprTask(expr,
					  pattern_,
					  guidance_,
					  taskId, ++stride));

      // if this is a "vanilla" exploration, remember it in the group
      if (guidance_ == NULL)
	(*CURRSTMT_OPTGLOBALS->memo)[groupId_]->setExploredInPass(currentPass);

      (*CURRSTMT_OPTGLOBALS->memo)[groupId_]->addToExploredRules(applicableRules);
    } // for a new pattern, explore each expression
  
  // tasks must destroy themselves
  delete this;
  
} // ExploreGroupTask::perform
//<pb>
/* ------------------------------------------------------------ */

/*
  Task to explore a logical expression
  */

ExploreExprTask::ExploreExprTask(RelExpr * expr,
				 RelExpr * pattern,
				 Guidance * guidance,
				 Lng32 parentTaskId, short stride)
               : CascadesTask(guidance, NULL, parentTaskId, stride),
                 expr_(expr),
		 pattern_(pattern)
{
} // ExploreExprTask::ExploreExprTask

void ExploreExprTask::print(FILE * f, const char *, const char * suffix) const
{
#ifndef NDEBUG
  fprintf(f, "explore log-expr -- parent task %d subtask %d\n", 
          parentTaskId_, stride_);
  expr_->print(f, "\t");
  if (pattern_ != NULL)
    pattern_->print(f, "\tpattern: ", suffix);
#endif  //NDEBUG
} // ExploreExprTask::print

NAString ExploreExprTask::taskText() const
{
  char returnText[100], prefix[100];
  sprintf(prefix, "%d %d Explore Expression \"",
          getParentTaskId(), getSubTaskId());
  sprintf(returnText,"\" of group %d", expr_->getGroupId());

  if (pattern_ != NULL)
    return NAString(prefix) + expr_->getText() + returnText +
      " for pattern \"" + pattern_->getText() + "\"";
  else
    return NAString(prefix) + expr_->getText() + returnText;
}

//<pb>
CascadesGroupId ExploreExprTask::getGroupId() const
{
  return expr_->getGroupId();
}

RelExpr * ExploreExprTask::getExpr()
{
  return expr_;
}
//<pb>
void ExploreExprTask::perform(Lng32 taskId)
{
  // arity of the log expr's top node
  Int32 arity = expr_->getArity();
  
  // identify valid and promising rules
  RuleWithPromise promisingMove[MAX_RULE_COUNT];
  Int32 numberOfMoves = 0;      // # of moves already collected
  short stride = 0;

  const RuleSubset *applRules;

  if (guidance_ != NULL)
    applRules = guidance_->applicableRules();
  else
    applRules = GlobalRuleSet->applicableRules();  

  // Iterate over all the applicable rules
  for (CollIndex ruleId = 0;  applRules->nextUsed(ruleId); ruleId++)
  {
    Rule * rule = GlobalRuleSet->rule(ruleId);
    Int32 promise;
      
    // Consider transformation rules only, ignore all implementation rules.
    if (rule->isTransformationRule())
    {
      // If this rule may be considered for application,
      if ( NOT expr_->getContextInsensRules().contains(ruleId))
      {
        // check whether it is indeed applicable and 
        // also whether it is a promising one.
        if (rule->topMatch(expr_,NULL)  AND
            (promise = rule->promiseForExploration(expr_,
                                                   pattern_,
                                                   guidance_)) > 0)
        {
          // insert valid and promising move into the array
          promisingMove[numberOfMoves].promise = promise;
          promisingMove[numberOfMoves].tieBreaker = numberOfMoves;
          promisingMove[numberOfMoves ++ ].rule = rule;
        } 
        // Otherwise, mark the transformation rule as a "tried" rule. 
        else
        {
          // Only mark the rule as a tried rule if it is not the
          // JoinToTSJRule in Pass1 for a non-TSJ join. This rule
          // is only enabled in Pass1 for anti-semi-joins (ASJ).
          // Non-TSJ join nodes that aren't ASJ's still need to have
          // the JoinToTSJRule fire in Pass2, so don't mark the 
          // JoinToTSJRule as a tried rule for them in Pass1.
          if (NOT ((GlobalRuleSet->getCurrentPassNumber() == 
                    GlobalRuleSet->getFirstPassNumber()) 
                   AND (ruleId == JoinToTSJRuleNumber)
                   AND expr_->getOperator().match(REL_ANY_NON_TSJ_JOIN)))
          {
            expr_->contextInsensRules() += ruleId;
          }
        }
      } // end if rule has not already been tried
    } // end if rule is a transformation rule
      
  } // identify valid and promising rules

  // order the valid and promising moves in the descending order of their promise
  opt_qsort((char *) promisingMove, numberOfMoves,
	sizeof(RuleWithPromise), comparePromisingMoves);
  
  // determine how many of the promising moves to pursue
  numberOfMoves = expr_->computeExploreExprCutoff
                            (promisingMove, numberOfMoves, pattern_, guidance_);
  
  // schedule valid rules in order of promise
  while (-- numberOfMoves >= 0)
    {
      Rule * rule = promisingMove[numberOfMoves].rule;
      
      // push future tasks in reverse order (due to LIFO stack)
      
      // note that "guidance" is simply passed through
      
      // last task: apply the rule
      CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
		      ApplyRuleTask(rule,
				    expr_,
				    pattern_,
				    guidance_,
				    NULL,
				    taskId, ++stride));
      
      RelExpr * new_rule_pattern = rule->getPattern();
      
      // by pass remaining part of the loop if new_rule_pattern is null
      if (!new_rule_pattern)
        continue;

      // earlier tasks: expand all children to match the pattern
      for (Lng32 childIndex = arity;  -- childIndex >= 0;  )
        {
	  RelExpr * patternInput = (* new_rule_pattern)[childIndex]->
	                                                     castToRelExpr();
	  
	  // not for sub-patterns that represent anon. children
	  if (patternInput->isCutOp())
	    ; // do nothing -- no expansion required
	  else if (patternInput->isSubtreeOp() AND NOT
		   ((SubtreeOp *) patternInput)->all_bindings())
	    ; // do nothing -- no expansion required
	  else // expand an child to match the pattern
            {
	      // schedule a task with new guidance and context
	      CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
			      ExploreGroupTask((* expr_) [childIndex].getGroupId(),
					       (* new_rule_pattern) [childIndex],
					       rule->guidanceForExploringChild
					       (guidance_,
						NULL,
						(Int32)childIndex),
					       taskId, ++stride));
            } // expand an child to match the pattern
        } // earlier tasks: expand all children to match the pattern
    } // schedule valid rules in order of promise
  
  // for rules with more than two level patterns, shouldn't we explore
  // the child groups of this expression to match the patterns children?
  // (loop over all non-leaf child nodes of pattern and create explore
  // group tasks)?????

  // tasks must destroy themselves
  delete this;
  
} // ExploreExprTask::perform

void ExploreExprTask::garbageCollection(RelExpr *oldx,
					RelExpr *newx,
				 Int32 /* groupMergeCount */)
{
  if (expr_ == oldx)
    expr_ = newx;
} // ExploreExprTask::garbageCollection
//<pb>
/* ------------------------------------------------------------ */

ApplyRuleTask::ApplyRuleTask(Rule * rule,
			     RelExpr * expr,
			     RelExpr * pattern,
			     Guidance * guidance,
			     Context * context,
			     Lng32 parentTaskId, short stride)
             : CascadesTask(guidance, context, parentTaskId, stride),
	       rule_(rule),
	       expr_(expr),
	       pattern_(pattern)
{
  if (context_ != NULL)
    context_->incrOutstanding();
} // ApplyRuleTask::ApplyRuleTask

void ApplyRuleTask::print(FILE * f, const char * prefix, const char *) const
{
#ifndef NDEBUG
  fprintf(f, "%sapply rule \"%s\" on group %d -- parent task %d subtask %d\n",
	  prefix, rule_->getName(), getGroupId(), parentTaskId_, stride_);
  if (expr_ != NULL)
    expr_->print(f, CONCAT(prefix,"\torig expr = "));
  if (pattern_ != NULL)
    pattern_->print(f,CONCAT(prefix,"\tpattern = "));
  if (context_ != NULL)
    context_->print(f, CONCAT(prefix,"\tcontext = "));
#endif  //NDEBUG
} // ApplyRuleTask::print
//<pb>
NAString ApplyRuleTask::taskText() const
{
  char returnText[100], prefix[100];
  sprintf(prefix, "%d %d Apply ",
          getParentTaskId(), getSubTaskId());
  sprintf(returnText,"\" on group %d", getGroupId());
  if (expr_ != NULL)
    return NAString(prefix) + "Rule \"" +
      rule_->getName() + "\" on expr \"" + expr_->getText() + returnText;
  else
    return NAString(prefix) + "Enforcer Rule \"" + 
      rule_->getName() + returnText;
}

CascadesGroupId ApplyRuleTask::getGroupId() const
{
  if (expr_ != NULL)
    return expr_->getGroupId();
  else
  {
    // If no expr is associated with ApplyRuleTask, then this
    // must be a context sensitive rule.
    CMPASSERT (context_ != NULL);
    return context_->getGroupId();
  }
}

RelExpr * ApplyRuleTask::getExpr()
{
  return expr_;
}
//<pb>
void ApplyRuleTask::perform(Lng32 taskId)
{
  if (((expr_ == NULL AND 
        NOT context_->getTriedEnforcerRules().contains(rule_->getNumber()))
       OR
       (expr_ != NULL AND
        ((rule_->isContextSensitive() AND
          NOT expr_->getContextSensRules().applied(context_,rule_->getNumber()))
         OR
         (NOT rule_->isContextSensitive() AND
          NOT expr_->getContextInsensRules().contains(rule_->getNumber())))))
      AND
      ((GlobalRuleSet->getCurrentPassNumber() == 1)
       OR NOT taskNumberExceededLimit(taskId)))
    {
      //---------------------------------------------------------
      // Set the appropriate bitmap to indicate that this rule is
      // being applied. 
      //---------------------------------------------------------
      if (expr_ != NULL)
      {
	if (rule_->isContextSensitive())
	  expr_->contextSensRules().addRule (context_, rule_->getNumber());
	else
	  expr_->contextInsensRules() += rule_->getNumber();
      }
      else
      {
        context_->triedEnforcerRules() += rule_->getNumber();
      }
      GlobalRuleSet->bumpRuleApplCount();
      
      // main variables for the loop over all possible bindings
      CascadesBinding    * binding;      // state of iteration through "memo"
      RelExpr            * before;       // expression before the transformation
      RelExpr            * after = NULL; // substitute expression
      RelExpr            * new_expr;     // expr in "memo" after transformation
      CascadesGroupId   groupToMerge = NULL_COLL_INDEX;   // group number for a group merge (if needed)
      
      // try all possible bindings
      if (expr_ != NULL)
        binding = new(CmpCommon::statementHeap())
	  CascadesBinding(expr_, rule_->getPattern(), NULL, TRUE, FALSE);
      else
      {
        CMPASSERT(rule_->getPattern() != NULL AND rule_->getPattern()->isCutOp());
        binding = new(CmpCommon::statementHeap())
	  CascadesBinding(getGroupId(), rule_->getPattern(), NULL, TRUE, FALSE);
      }

      NABoolean checkForce =
        ( (context_ != NULL) AND 
          (context_->getReqdPhysicalProperty() != NULL) AND
          (context_->getReqdPhysicalProperty()->getMustMatch() != NULL)
        );
      short stride = 0;

      for (  ;  binding->advance();  binding->release_expr())
	{

	  RuleSubstituteMemory * ruleMemory = NULL;
	  Int32 count = 0;
	  
	  // extract an actual expression from the binding
	  before = binding->extract_expr();

          // Forcing a CQS causes the pruning to be turned off except 
          // when memory  usage exceeds the preset threshold set using a 
          // cqd (MEMORY_USAGE_SAFETY_NET).  

          NABoolean memorySafe = 
            CURRSTMT_OPTDEFAULTS->getMemUsed() < CURRSTMT_OPTDEFAULTS->getMemUsageSafetyNet();

          if ( (checkForce AND memorySafe) OR
               NOT CURRSTMT_OPTDEFAULTS->pruneByOptDefaults(rule_, before)
             )
            {	  
	      // loop to generate all substitutes, to include them
	      // in "memo", and to trigger further rules
	      while (count == 0 OR ruleMemory != NULL)
	        {
	          // fire the rule
	          after = rule_->nextSubstitute(before,context_,ruleMemory);
	          count++;
	      
	          // terminate loop if no further substitute could be derived
	          if (after == NULL)
		    {
		      CMPASSERT(ruleMemory == NULL);
		      break;
		    }
	      
	          // the substitute must have compatible, if not the same,
	          // group attributes as the before-image
	          CMPASSERT(after->getGroupAttr() == before->getGroupAttr() OR
		         *(after->getGroupAttr()) == *(before->getGroupAttr()));

              // pass along before's isinCS attribute to after
              after->setBlockStmt(before->isinBlockStmt());

	          // update the potential of the substitute:
            // if the potential is not already set (i.e. has a value of -1)
            // it should be the same as the before expression's potential
            after->updatePotential(before->getPotential());

	          NABoolean duplicateExprFlag;
	          NABoolean groupMergeFlag;
	      
	          // include substitute in "memo", find duplicates, etc.
	          new_expr = CURRSTMT_OPTGLOBALS->memo->include(after,
				           duplicateExprFlag,
				           groupMergeFlag,
					   FALSE,         //soln-10-070330-3667
				           getGroupId(),
				           context_,
                                           before
				           );
	      
	          // ---------------------------------------------------------
	          // if inserting the new expression caused a group merge,
	          // continue optimizing or exploring all expressions in the
	          // new group, otherwise create a follow-on task for the
	          // new expression
	          // ---------------------------------------------------------
	          if (groupMergeFlag)
		    {
		      groupToMerge = new_expr->getGroupId();
		  
		      // start with the first expression in the new group
		      new_expr = (*CURRSTMT_OPTGLOBALS->memo)[groupToMerge]->getFirstLogExpr();
		    }
	          else if (duplicateExprFlag)
		    {
		      // a duplicate expression that didn't cause a group
		      // merge is ignored, no further work needs to be done
		      // (we've probably applied the same rule twice)
		      new_expr = NULL;
		    }
	      
	          // now loop over the one new expression or over all expressions
	          // in a merged group
	          while (new_expr != NULL)
		    {
		      // -----------------------------------------------------
		      // create a follow-up task for the expression that was
		      // inserted by the rule:
		      //
		      // --> optimize the (logical) result, if the rule was
		      //     triggered by an optimization task
		      // --> explore the (logical) result, if the rule was
		      //     triggered by an exploration task
		      // --> optimize the children of the result, if the result
		      //     is a physical expression and the rule was triggered
		      //     by an optimization task
		      // -----------------------------------------------------
		  
		      if (context_ == NULL) // optimizer is exploring
		        {
		          CURRSTMT_OPTGLOBALS->task_list->push
			               (new(CmpCommon::statementHeap())
				        ExploreExprTask
				        (new_expr,
				         pattern_,
				         rule_->guidanceForExploringSubstitute(guidance_),
				         taskId, ++stride));
		        } // optimizer is exploring
		      else // optimizer is optimizing
		        {
		      
		          // for a logical op, try further transformations
		          if (new_expr->isLogical())
			    {
			      // First, see if there are any outstanding contexts
			      // for this group.  If so, schedule a optimizeExprTask
			      // for those outstanding contexts also for this new expression.
			      Lng32 numContexts = (*CURRSTMT_OPTGLOBALS->memo)[new_expr->getGroupId()]->getCountOfContexts();
			      if (numContexts > 1 && context_ != NULL)
			      {
			        for (Lng32 i = 0; i < numContexts; i++)
			        {
			          Context * otherContext = (*CURRSTMT_OPTGLOBALS->memo)[new_expr->getGroupId()]->getContext(i);
			          if (otherContext != NULL AND
				      context_ != otherContext AND
				      otherContext->getOutstanding() > 0 AND
				      NOT otherContext->isADuplicate())
			          {

            if (CmpCommon::getDefault(COMP_BOOL_120) == DF_ON)
				    // insert this new OptimizeExprTask to be performed just
				    // before the stored predecessor task for this context.
				    CURRSTMT_OPTGLOBALS->task_list->insertTask (new(CmpCommon::statementHeap())
						           OptimizeExprTask
						           (new_expr,
							    rule_->guidanceForOptimizingSubstitute
							    (guidance_,
							     otherContext),
							    otherContext,
							    taskId, ++stride),
						           otherContext->getPredecessorTask());
            else
            // insert this new OptimizeExprTask to be performed just
            // before the stored predecessor task for this context.
            CURRSTMT_OPTGLOBALS->task_list->insertOptimizeExprTask (new(CmpCommon::statementHeap())
                       OptimizeExprTask
                       (new_expr,
                  rule_->guidanceForOptimizingSubstitute
                  (guidance_,
                   otherContext),
                  otherContext,
                  taskId, ++stride),
                       otherContext->getPredecessorTask());

			          }
			        }
			      }

			      // Now, schedule an OptimizeExprTask for the new expr
			      // for this context.
			      CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
					      OptimizeExprTask
					      (new_expr,
					       rule_->guidanceForOptimizingSubstitute
					       (guidance_,
					        context_),
					       context_,
					       taskId, ++stride));

			    } // further transformations to optimize new expr
		      
		          // for a physical operator, optimize the children
		          // must be done even if new_expr->getArity() == 0
		          // in order to calculate costs
		          if (new_expr->isPhysical())
			    {
			      // $$ check for different new phys expression address
			  
			      CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
					      CreatePlanTask
					      (rule_,
					       new_expr,
					       guidance_,
					       context_,
					       taskId, ++stride));
			    } // for a physical operator, optimize the children
		        } // optimizer is optimizing
		  
		  
		      // leave the while loop after the first time, unless this
		      // was a group merge (where we have to follow up on all
		      // the expression in the merged group)
		      if (groupMergeFlag)
		        new_expr = new_expr->getNextInGroup();
		      else
		        new_expr = NULL;
		  
		    } // while (new_expr != NULL)
	      
	          // If a group merge is necessary then do it now that we've
	          // handled all the logical expressions in the new group.
	          if (groupMergeFlag)
		    {
		      (*CURRSTMT_OPTGLOBALS->memo)[expr_->getGroupId()]->merge((*CURRSTMT_OPTGLOBALS->memo)[groupToMerge]);
		    }
	      
	        } // loop to generate all substitutes, etc.
            }
	} // try all possible bindings
      
      delete binding;
      
    } // rule hasn't beeen tried before

  // prepare to end the task
  if (context_ != NULL)
    context_->decrOutstanding();

  // tasks must destroy themselves
  delete this;
  
} // ApplyRuleTask::perform

void ApplyRuleTask::garbageCollection(RelExpr *oldx,
				      RelExpr *newx,
				      Int32 /* groupMergeCount */)
{
  if (expr_ != NULL AND expr_ == oldx)
    expr_ = newx;
} // ApplyRuleTask::garbageCollection
//<pb>
/* ------------------------------------------------------------ */

CreatePlanTask::CreatePlanTask(Rule * rule,
			       RelExpr * expr,
			       Guidance * guidance,
			       Context * context,
			       Lng32 parentTaskId, short stride,
				   CascadesPlan * failedPlan)
              : CascadesTask(guidance, context, parentTaskId, stride),
		workSpace_(expr->allocateWorkSpace()),
                rule_(rule)
{
  // this task must have an associated context
  CMPASSERT(context != NULL);

  if (failedPlan)
  { 
    plan_ = failedPlan;
    // so far we don't have an implementation to reuse physical properties
    plan_->setPhysicalProperty(NULL);
  }
  else
  {
    plan_ = new(CmpCommon::statementHeap())
                CascadesPlan(expr, context);

    // Add a new CascadesPlan to the CascadesGroup to which the given
    // physical expression belongs.
    (*CURRSTMT_OPTGLOBALS->memo)[expr->getGroupId()]->addPlan(plan_);
  }
  
   // this expression is not optimized until its children are optimized
  context->incrOutstanding();

} // CreatePlanTask::CreatePlanTask

CreatePlanTask::~CreatePlanTask()
{
  delete workSpace_;
  
} // CreatePlanTask::~CreatePlanTask

CascadesGroupId CreatePlanTask::getGroupId() const
{
  return plan_->getPhysicalExpr()->getGroupId();
}

RelExpr * CreatePlanTask::getExpr()
{
  return NULL;
}

CascadesPlan * CreatePlanTask::getPlan()
{
  return plan_;
}
//<pb>
THREAD_P Lng32 Cyes = 0;
THREAD_P Lng32 Cno = 0;
void CreatePlanTask::perform(Lng32 taskId)
{
  short stride = 0;
  Context  *childContext = NULL;
  Guidance *childGuidance= NULL;

  ostream &out = CURRCONTEXT_OPTDEBUG->stream();
  const char* prefix = "    ";
  NABoolean showThisPlan = FALSE;
  NABoolean showPlanCost = FALSE;
  if ( (CmpCommon::getDefault(NSK_DBG_PRINT_TASK) == DF_ON) &&
       (CmpCommon::getDefault(NSK_DBG) == DF_ON) ) 
  {
    showThisPlan = TRUE;
    if ( CmpCommon::getDefault(NSK_DBG_PRINT_COST) == DF_ON)
      showPlanCost = TRUE;
  }

  if (workSpace_->isEmpty())
    {
      // Assign the new plan to the given Context.
      // The assignment is done when the CreatePlanTask executes
      // and not when it is scheduled. This is because a given
      // Context can be shared by different CreatePlanTasks that
      // are scheduled in succession.
      context_->assignCurrentPlan(plan_);
      context_->setPlanWorkSpace(workSpace_);
      workSpace_->storeContext(context_);
    }

  // let the expression decide which child to optimize next,
  // whether the task is failed or successful, and which combinations
  // of required properties to try
  
#ifdef _DEBUG
  plan_->planMonitor_.enter();
  plan_->lastTaskId_ = taskId;
#endif
  childContext = plan_->getPhysicalExpr()
                          ->createPlan
                              (context_,           // in
			       workSpace_,         // in
			       rule_,              // in
			       guidance_,          // in
			       childGuidance);     // out
#ifdef _DEBUG 
  plan_->planMonitor_.exit();
#endif
  if ( childContext != NULL ) 
    {
      // re-schedule the present task
      CURRSTMT_OPTGLOBALS->task_list->push(this);
      
      // schedule a task to optimize the child, new guidance
      // When pruning is on we don't want to create a task just to 
      // exit right away from findBestSolution() because child 
      // context has been already optimized.
      if ( NOT CURRSTMT_OPTDEFAULTS->optimizerPruning() OR
           NOT ( childContext->hasSolution() AND
                 childContext->optimizedInCurrentPass()
               )
         )
        CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
		      OptimizeGroupTask(childContext,
					childGuidance,
					FALSE,
					taskId, ++stride));

      if ( showThisPlan )
      {
        out << endl << prefix << "*** Incomplete plan ***" << endl;
        CURRCONTEXT_OPTDEBUG->showTree( getExpr(), getPlan(), prefix, showPlanCost );
        out << endl;
      }
    }
  else
    {
       if ( CURRSTMT_OPTDEFAULTS->optimizerPruning() AND
            NOT workSpace_->allChildrenContextsConsidered() 
          )
       {
       // We come here when exiting from createContextForAChild()
       // loop to go through RelExpr::createPlan() again for
       // recomputing operator cost and cleaning knownChildrenCost
       // All we need to do here is to keep the task on the top of 
       // the stack and just return from here to createPlan()
       // To be safe we set a flag in pws in its initial value to be
       // safe. Otherwise we can come into an indefinite loop.
         workSpace_->setAllChildrenContextsConsidered();
         CURRSTMT_OPTGLOBALS->task_list->push(this);
         return;
       }

      // Get a pointer to any previous solution
      const CascadesPlan *oldSolution = context_->getSolution();
      NABoolean planSatisfied = FALSE;

      // check whether a "complete" plan was found that satisfies the context.
      // complete plans must have both a cost and synthesized physical
      // properties.
      if ( plan_->getRollUpCost() != NULL AND 
           (planSatisfied=context_->satisfied(plan_) ) )
	{
          Cyes++;
	  // Success!!!

          // the plan succeeded, flag it as successful in
          // the current pass so
          // that is never re-optimized in this pass:
          plan_->setSuccessInCurrentPass();
          
          if ( showThisPlan )
          {
            out << endl << prefix << "*** Qualified plan ***" << endl;
            CURRCONTEXT_OPTDEBUG->showTree( getExpr(), getPlan(), prefix, showPlanCost );
          }

	  // This is a solution for the optimization goal represented
	  // by the context; now check whether it's the optimal one so far
          // for this pass. Any solution from a previous pass is discarded,
          // because we cannot share solutions between passes. 
	  if ((oldSolution != NULL) AND
              oldSolution->succeededInCurrentPass()) 
	    {
              // There is a previous solution for this pass.
	      // Check if the cost of plan_ is less than the context's current
	      // best cost solution for this pass.
	      if (plan_->getRollUpCost()->
		  compareCosts(*context_->getSolution()->getRollUpCost(),
			       context_->getReqdPhysicalProperty()) == LESS)
		{
		  // found a better solution
		  context_->setSolution(plan_);

                  if ( showThisPlan )
                    out << prefix << "*** Chosen plan ***" << endl << endl;
                  // if because of nice context we have an enforcer above
                  // partial plan that already satisfies requirement -
                  // replace solution with that partial plan because
                  // it has to be cheaper than plan with enforcer
                  if ( CURRSTMT_OPTDEFAULTS->OPHuseEnforcerPlanPromotion() )                       
                  {
                    OperatorTypeEnum op =
                        plan_->getPhysicalExpr()->getOperatorType();
                    if ( (op == REL_SORT) OR (op == REL_EXCHANGE) )
                    {
                      CascadesPlan *planToPromote = (CascadesPlan *)
                        plan_->getContextForChild(0)->getSolution();
                      if (context_->satisfied(planToPromote))
                      {
                        // child satisfies context without enforcer
                        // and can be promoted
		        context_->setSolution(planToPromote);
                        if ( showThisPlan )
                        {
                          out << endl << prefix << 
                              "*** Promoted plan ***" << endl;
                          CURRCONTEXT_OPTDEBUG->showTree( 
                              planToPromote->getPhysicalExpr(), 
                              planToPromote, prefix, showPlanCost );
                        }
                      }
                    }
                  }
		}
	      else
              {
		oldSolution = plan_;
                if ( showThisPlan )
                  out << prefix << "*** Non-optimal plan ***" << endl << endl;
              }
	      
	      // here is some opportunity for pruning: discard the more
	      // expensive plan, if it does not also provide "more" physical
	      // properties than the optimal solution so far
	      // if (oldSolution->getPhyProp()->
	      //     compare(*context_->getSolution()->getPhyProp()) != MORE)
	      //   {
	      //     delete oldSolution;
	      //   }
	    }
          else
	    {
	      // This is the first solution for this pass and therefore 
              // it is the optimal one so far.
	      context_->setSolution(plan_);
              if ( showThisPlan )
                out << prefix << "*** Chosen plan ***" << endl << endl;

// if because of nice context we have an enforcer above
              // partial plan that already satisfies requirement -
              // replace solution with that partial plan because
              // it has to be cheaper than plan with enforcer
              if ( CURRSTMT_OPTDEFAULTS->OPHuseEnforcerPlanPromotion() )                       
              {
                OperatorTypeEnum op =
                  plan_->getPhysicalExpr()->getOperatorType();
                if ( (op == REL_SORT) OR (op == REL_EXCHANGE) )
                {
                  CascadesPlan *planToPromote = (CascadesPlan *)
                    plan_->getContextForChild(0)->getSolution();
                  if (context_->satisfied(planToPromote))
                  {
                     // child satisfies context without enforcer
                     // and can be promoted
	             context_->setSolution(planToPromote);
                     if ( showThisPlan )
                     {
                       out << endl << prefix << 
                         "*** Promoted plan ***" << endl;
                       CURRCONTEXT_OPTDEBUG->showTree( 
                         planToPromote->getPhysicalExpr(), 
                         planToPromote, prefix, showPlanCost );
                     }
                   }
                }
              }
	    }
	  
          // Candidate plans are only used by cost limit pruning, so only
          // add the plan as a candidate plan if pruning is enabled.
          if (context_->isPruningEnabled())
          {
            context_->addCandidatePlan(plan_);
          }

          // Pruning heuristics to use failed plan cost
          if ( CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() AND 
               plan_->exceededCostLimit() )
            context_->setCurrentPlanExceededCostLimit();

	} // a complete plan satisfying the context was found
        else 
        {
          // The current pass plan failed
          Cno++;

          if ( plan_->getRollUpCost() != NULL AND NOT planSatisfied )
          {
            plan_->setFailedInCurrentPass();
            DBGLOGMSGCNTXT(" *** Plan didn't satisfied ",context_);
          }

          // If there's an old solution from a previous pass, and the
          // current pass failed, we must delete the old solution.
          // This is to protect us from being left with nothing but
          // the old pass1 plan if all pass2 plans fail. This should
          // never happen, but if it does we cannot allow it. This is
          // because if this logical operator fails to produce a
          // pass2 plan but some other logical operator (like one
          // of this operator's children) succeeds, we could be stuck
          // with a mixed pass1/pass2 overall plan, which is not allowed.
          if ((oldSolution != NULL) AND
              NOT oldSolution->succeededInCurrentPass())
          {
            DBGLOGMSGCNTXT(" *** Solution from pass1 deleted ",context_);
            context_->setSolution(NULL);
            // although we completely forget about pass1 solution
            // there could be a way to reuse it for the next pass.
            // There was a problem in the past preventing this reusing.
          }
        }

      // the expression plan_ is now optimized in the given context
      context_->decrOutstanding();

      // The plan created by this task is either the solution_ or has been
      // discarded and there are no more outstanding tasks. 
      // Dissociate the plan from the context.
      if (context_->getOutstanding() == 0)
	  {
	     context_->clearCurrentPlan();
             // This is used by pruning heuristic to use failed plan cost
	     // If context has no plans with rollUp cost from current pass
	     // and some of plans exceeded cost limit then setiing this flag
	     // should prevent reoptimizing this context unless given bigger 
	     // cost limit
	     if ( CURRSTMT_OPTDEFAULTS->OPHusePWSflagForContext() AND 
                  CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() AND
		  ( NOT context_->hasSolution() ) AND
		  context_->currentPlanExceededCostLimit() 
		)
             {
               context_->setCostLimitExceeded();
               DBGLOGMSGCNTXT(" *** CLEx flag passed from curCLEX ",context_);
             }
	  }
	  
      // tasks must destroy themselves
      delete this;
    }

} // CreatePlanTask::perform
//<pb>
void CreatePlanTask::garbageCollection(RelExpr * /* oldx */,
				       RelExpr * /* newx */,
				       Int32 /* groupMergeCount */)
{
} // CreatePlanTask::garbageCollection


void CreatePlanTask::print(FILE * f, const char * prefix, const char * suffix) const
{
  fprintf(f, "%screate plan -- incarnation %d (%d) of %d\n",
          prefix, parentTaskId_, stride_, 
          workSpace_->getCountOfChildContexts());
  plan_->getPhysicalExpr()->print(f, " ");
  context_->print(f, "\t", suffix);
} // CreatePlanTask::print

NAString CreatePlanTask::taskText() const
{
  char tmpText[100];

#ifdef _DEBUG
  char childrenGroup[12] = "";
  RelExpr * ex = plan_->getPhysicalExpr();
  if ( ex->getArity() == 1 )
    sprintf(childrenGroup, "(%d)", (ex->child(0)).getGroupId() );
  else if (ex->getArity() > 1 )
    sprintf(childrenGroup, "(%d,%d)", (ex->child(0)).getGroupId(),
                                      (ex->child(1)).getGroupId() );
  childrenGroup[11]='\0';
#endif

  sprintf( tmpText, "%d %d Create plan (%d) for ", 
           getParentTaskId(), getSubTaskId(),
           workSpace_->getCountOfChildContexts() );
  NAString returnString(tmpText);
  returnString += (const char *) plan_->getPhysicalExpr()->getText();
#ifdef _DEBUG
  returnString += childrenGroup;
#endif
  sprintf( tmpText, " of group %d ", getGroupId() );
  returnString += tmpText;

  return returnString + context_->getRequirementsString();
}


//<pb>
GarbageCollectionTask::GarbageCollectionTask () 
                      : CascadesTask(NULL, NULL, 0, 0)
{
  alreadyDone_ = FALSE;
}

GarbageCollectionTask::~GarbageCollectionTask ()
{}


void GarbageCollectionTask::print (FILE * f,
				   const char * prefix,
				   const char * /*suffix*/) const
{
  fprintf(f, "%sGarbage Collection...\n",prefix);
}

NAString GarbageCollectionTask::taskText() const
{
  return "Garbage Collection";
}


void GarbageCollectionTask::perform (Lng32 /*taskId*/)
{
  if (NOT alreadyDone_)
    {
      Int32 changedExprs = CURRSTMT_OPTGLOBALS->memo->garbageCollection();
      CURRSTMT_OPTGLOBALS->garbage_expr_count += changedExprs;
    }

  delete this;

}

void GarbageCollectionTask::garbageCollection(RelExpr * /* oldx */,
					      RelExpr * /* newx */,
					      Int32 groupMergeCount)
{
  // If we come here, another GarbageCollectionTask task is currently
  // executing. If that task has caught up with all the group
  // merges already, then it is not necessary to schedule
  // further garbage collection tasks at this point.
  if (groupMergeCount >= CURRSTMT_OPTGLOBALS->group_merge_count)
    alreadyDone_ = FALSE;
}

