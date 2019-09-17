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
* File:         opt.C
* Description:  optimization code, Cascades optimizer search engine
*               CascadesTaskList, CascadesBinding, Context
*               methods are defined in this file. See also files
*               tasks.C and memo.C for other definitions of classes
*               defined in the include file opt.h.
* Created:      7/22/94
* Language:     C++
*
*
*
******************************************************************************
*/

#include <limits.h>
#include <time.h>
#include "ControlDB.h"
#include "Sqlcomp.h"
#include "CmpContext.h"
#include "CmpErrors.h"
#include "CmpMain.h"		// gui display
#include "CmpMemoryMonitor.h"
#include "Cost.h"
#include "GroupAttr.h"
#include "opt.h"
#include "PhyProp.h"
#include "RelExpr.h"
#include "RelJoin.h"
#include "RelMisc.h"
#include "CostMethod.h"
#include "logmxevent.h"
#include "CompException.h"
#include "CompilationStats.h"

//////////////////////////////
#include "Analyzer.h"
//////////////////////////////


#ifdef NA_DEBUG_GUI
  #include "ComSqlcmpdbg.h"
#endif

#include "CmpStatement.h"

//<pb>
// -----------------------------------------------------------------------
// global variables (defined here and declared elsewhere)
// -----------------------------------------------------------------------
extern const WordAsBits SingleBitArray[];

// the following is used in the DCMPASSERT mechanism to decide whether
// or not we should have our "optimizer asserts" (DCMPASSERT()) be
// real asserts or simply a no-op
// --> these asserts have to be "turned on" by individual
//     developers (in PROFILE.ksh, do EXTERN

// this macro is defined in /common/CmpCommon.h

//#ifdef _DEBUG

// -----------------------------------------------------------------------
// global display methods for purposes of debugging
// -----------------------------------------------------------------------

void displayContext (const Context & context)
{
  context.print();
}


// -----------------------------------------------------------------------
// local helper functions
// -----------------------------------------------------------------------
#ifdef _DEBUG
static const char *getCOMPILE_TIME_MONITOR_OUTPUT_FILEname()
{
  const char * fname = ActiveSchemaDB()->getDefaults().
			 getValue(COMPILE_TIME_MONITOR_OUTPUT_FILE);
  return (*fname && stricmp(fname,"NONE") != 0) ? fname : NULL;
}
#endif //_DEBUG


// -----------------------------------------------------------------------
//     Optimization procedure
//     ======================
//     main procedure for optimization
//     create new CascadesMemo and CascadesTaskList structures
//     copy query into memo
//     for each pass
//         trigger exploring the query root
//         perform search tasks until the query is fully optimized
//     extract optimal plan
//     clean up the CascadesMemo and CascadesTaskList structures
//
// -----------------------------------------------------------------------

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
// children they should be both "small" and have their first plans non-parallel
// and succeeded in the current pass to prevent attempt to create another
// parallel plan.

NABoolean RelExpr::preventPushDownPartReq
                                    (const ReqdPhysicalProperty* rppForMe,
                                     const EstLogPropSharedPtr& inLogProp)
{

  if ( getArity()==0 )
  {
    const CascadesPlan* myPlan =(*CURRSTMT_OPTGLOBALS->memo)[getGroupId()]->getFirstPlan();
    if ( myPlan AND myPlan->succeededInCurrentPass() AND
         ( (getGroupAttr()->outputLogProp(inLogProp))->
            getResultCardinality().value()
            < CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold()
         )
       )
      return (NOT (myPlan->getPhysicalProperty())->isPartitioned());
    else
      return FALSE;
  }

  NABoolean childIsPartitioned = TRUE;
  for (Int32 i=0; i<2; i++)
  {
    const CascadesPlan* childPlan =
     (*CURRSTMT_OPTGLOBALS->memo)[child(i).getGroupId()]->getFirstPlan();
    if ( childPlan AND childPlan->succeededInCurrentPass() AND
         ( (child(i).outputLogProp(inLogProp))->getResultCardinality().value()
            < CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold()
         )
       )
    {
       childIsPartitioned =
         (childPlan->getPhysicalProperty())->isPartitioned();
    }

    if ( (getArity() == 1) OR childIsPartitioned )
    {
      return (NOT childIsPartitioned);
    }
  }
  return FALSE;
}

// Soln: 10-040927-0202 helper function for RelExpr::optimize.
// and optimize2(ie., used in Queryoptimize driver)
// we want to remove potential warnings which we create while
// searching for an acceptable plan, e.g. generated when searching
// for a suitable index  to satisfy an order by clause on a stream.
// At the same time useful warnings like 6008 should not be cleared
// so selectivly remove warings that are resultants of a failed plan.
// Currently removing the following warnings. 4153 / 4207 / 4208 / 4212
void clearFailedPlanWarningsForStream()
{
  ComCondition* warningEntry = NULL;
  ULng32 maxWarns = CmpCommon::diags()->getNumber(DgSqlCode::WARNING_);
  Lng32 currentIndex = 1; //This is important as diags are 1 based Array.
  for(ULng32 index = 0; index < maxWarns ; index++)
  {
     warningEntry = CmpCommon::diags()->getWarningEntry(currentIndex);
     switch(warningEntry->getSQLCODE())
     {
	   case 4153://Stmt may not compile due to an order requirement on stream expression
       case 4207://index failed to cover all column in select caluse of stream
       case 4208://index failed to cover all column in where caluse of stream
       case 4212://index failed to satisfy order requirement on the stream
                 CmpCommon::diags()->deleteWarning(currentIndex - 1);
				 break;
       default:
		       currentIndex++;
			   break;
     }
  }
}

// This method is for the old driver and not used unless CQD NEW_OPT_DRIVER 
// is OFF. We use the new driver by default.
RelExpr * RelExpr::optimize(NABoolean exceptionMode,
                Guidance * guidance,
			    ReqdPhysicalProperty * rpp,
			    CostLimit* costLimit)
{
  CascadesGroupId root;
  RelExpr * plan = NULL;
  Int32 task_count = 0;
  short stride = 0;
#ifdef _DEBUG
  // this is to facilitate debugging in Visual inspect
  // during debugging we would set this variable to the task
  // number we want to stop at then cmpare this value to
  // task_count if comparison is true we would stop at actual
  // no op. but the value of tha variable will be incremented
  // to avoid warning.
  Int32 taskCountToStop = 0;
#endif
  NABoolean duplicate_expr;
  NABoolean group_merge;

  // initialize global counters
  CURRSTMT_OPTGLOBALS->group_merge_count     = 0;
  CURRSTMT_OPTGLOBALS->garbage_expr_count    = 0;
  CURRSTMT_OPTGLOBALS->pruned_tasks_count    = 0;
  CURRSTMT_OPTGLOBALS->duplicate_expr_count  = 0;
  CURRSTMT_OPTGLOBALS->logical_expr_count = 0;
  CURRSTMT_OPTGLOBALS->physical_expr_count = 0;
  CURRSTMT_OPTGLOBALS->plans_count         = 0;
  CURRSTMT_OPTGLOBALS->asm_count = 0;
  CURRSTMT_OPTGLOBALS->cascade_count = 0;
  CURRSTMT_OPTGLOBALS->warningGiven = FALSE;

  CostScalar::initOvflwCount(0);
  CostScalar::initUdflwCount(0);

//////////////////////////////////////////////////////

  RelExpr* treeCopy = NULL;

  if (!exceptionMode &&
      CURRSTMT_OPTDEFAULTS->optLevel() != OptDefaults::MINIMUM)
  {
    treeCopy = copyRelExprTree(CmpCommon::statementHeap());
  }

//////////////////////////////////////////////////////
  for (Int32 ti=0; ti<CascadesTask::NUMBER_OF_TASK_TYPES; ti++)
    (*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[ti]).init(0);
  (*CURRSTMT_OPTGLOBALS->cascadesPassMonitor).init(0);
if (CURRSTMT_OPTDEFAULTS->optimizerHeuristic2()) {//#ifdef _DEBUG
  (*CURRSTMT_OPTGLOBALS->fileScanMonitor).init(0);
  (*CURRSTMT_OPTGLOBALS->nestedJoinMonitor).init(0);
  (*CURRSTMT_OPTGLOBALS->indexJoinMonitor).init(0);
  (*CURRSTMT_OPTGLOBALS->asynchrMonitor).init(0);
  (*CURRSTMT_OPTGLOBALS->singleSubsetCostMonitor).init(0);
  (*CURRSTMT_OPTGLOBALS->singleVectorCostMonitor).init(0);
  (*CURRSTMT_OPTGLOBALS->singleObjectCostMonitor).init(0);
  (*CURRSTMT_OPTGLOBALS->multSubsetCostMonitor).init(0);
  (*CURRSTMT_OPTGLOBALS->multVectorCostMonitor).init(0);
  (*CURRSTMT_OPTGLOBALS->multObjectCostMonitor).init(0);
}//#endif

#ifdef DEBUG
  //  Reset count of PlanWorkSpace objects for each statement.
  PlanWorkSpace::resetPwsCount();

#endif /* DEBUG */

  NABoolean printCounters;
  if (CmpCommon::getDefault(OPTIMIZER_PRINT_INTERNAL_COUNTERS) == DF_OFF)
    printCounters = FALSE;
  else
    printCounters = NOT (CURRSTMT_OPTGLOBALS->BeSilent);

#ifdef _DEBUG


  // This is to allow separation of debuuging output in
  // the tandem services window when tdm services is
  // started with the -noconsole option:
  if (getenv("DEBUG_NOCONSOLE") != NULL)
    {
      fprintf(stdout,
              "\n\n\n#################################################"
              "\n#################################################"
              "\n#################################################"
              "\n\n\n------NEW OPTIMIZATION-----------------------\n\n"
              "\n#################################################"
              "\n#################################################"
              "\n#################################################"
              );
    }

#endif

  // create new structures for optimization
  CMPASSERT(CURRSTMT_OPTGLOBALS->memo == NULL AND CURRSTMT_OPTGLOBALS->task_list == NULL);
  CURRSTMT_OPTGLOBALS->memo = new (CmpCommon::statementHeap()) CascadesMemo();
  CURRSTMT_OPTGLOBALS->task_list = new (CmpCommon::statementHeap()) CascadesTaskList();


#ifdef NA_DEBUG_GUI
  CMPASSERT(gpClusterInfo != NULL);
  if (CmpMain::msGui_ && CURRENTSTMT->displayGraph() )
    CmpMain::pExpFuncs_->fpSqldbgSetCmpPointers(CURRSTMT_OPTGLOBALS->memo
                                                ,CURRSTMT_OPTGLOBALS->task_list
                                                ,QueryAnalysis::Instance()
                                                ,cmpCurrentContext
                                                ,gpClusterInfo
                                                );
#endif

  // ---------------------------------------------------------------------
  // Synthesize the logical properties for the input query.
  // ---------------------------------------------------------------------
  //  synthLogProp(); Now being done in QueryAnalysis

  //++MV 
  // This input cardinality is not estimated , so we keep this knowledge
  // in a special attribute.
  (*GLOBAL_EMPTY_INPUT_LOGPROP)->setCardinalityEqOne();

  if ( (CmpCommon::getDefault(COMP_BOOL_9) == DF_OFF)  AND
       ( child(0)->getOperatorType() == REL_DDL OR
	 ((RelRoot *)this)->accessOptions().isUpdateOrDelete() OR
	 ((RelRoot *)this)->isRootOfInternalRefresh()
       )
     )
    CURRSTMT_OPTGLOBALS->pruningIsFeasible = FALSE;
  else
    // pruning can be forced by setting CDQ COMP_BOOL_9 to ON
    CURRSTMT_OPTGLOBALS->pruningIsFeasible = TRUE;

  CURRSTMT_OPTGLOBALS->forceParallelInsertSelect =
    (CmpCommon::getDefault(FORCE_PARALLEL_INSERT_SELECT) == DF_ON) AND
    ((RelRoot *)this)->accessOptions().isUpdateOrDelete();

  if (((CmpCommon::getDefault(COMP_BOOL_69) == DF_OFF)  AND
       ( child(0)->getOperatorType() == REL_DDL OR
         ((RelRoot *)this)->isRootOfInternalRefresh()))
      )
    CURRSTMT_OPTGLOBALS->maxParIsFeasible = FALSE;
  else
    // maximum parallelism can be forced by setting CDQ COMP_BOOL_69 to ON
    CURRSTMT_OPTGLOBALS->maxParIsFeasible = TRUE;

  Lng32 t0 = clock();

    // ---------------------------------------------------------------------
    // -- Triggers.
    // Collect the access set information and save it in the Trigger backbone.
    // ---------------------------------------------------------------------
    calcAccessSets(CmpCommon::statementHeap());

	checkAndReorderTree();
  // QSTUFF
  // in case of an embedded update we must pushdown the characteristic
  // output values of the generic update root to its descendants to ensure
  // that only those output values are checked against indexes when the
  // index in case of a stream. Since in the future we may also want to
  // allow for unions we push it into every subset and subsequently follow
  // the isEmbeddedUpdate() thread down to their respective children

  if (getGroupAttr()->isStream() && getGroupAttr()->isEmbeddedUpdateOrDelete()){
    ValueIdSet dummy;
    dummy.clear();
    pushDownGenericUpdateRootOutputs(dummy);
  }
  // QSTUFF

  // estimate resources required for this query
  CURRSTMT_OPTDEFAULTS->estimateRequiredResources(this);

  // ---------------------------------------------------------------------
  // copy initial query into CascadesMemo
  // ---------------------------------------------------------------------
  root = CURRSTMT_OPTGLOBALS->memo->include(this,duplicate_expr,group_merge)->getGroupId();

  // ---------------------------------------------------------------------
  // initialize optimization defaults
  // ---------------------------------------------------------------------
  CURRSTMT_OPTDEFAULTS->initialize(this);

  GlobalRuleSet->initializeFirstPass();

  // If the query contains an AntiSemiJoin, (or embedded delete)
  // then we need to add the JoinToTSJ rule to the pass 1 rule set to
  // ensure that we will get a pass1 plan, since Hash Join does not
  // currently work for ASJ (or embedded delete)
  if (CURRSTMT_OPTDEFAULTS->isJoinToTSJRuleNeededOnPass1())
  {

    GlobalRuleSet->enable(JoinToTSJRuleNumber);
  }

  // ---------------------------------------------------------------------
  // create initial context for the optimization of the top level group
  // This was earlier being initialized in a manner similar to
  // GLOBAL_EMPTY_INPUT_LOGPROP. They are now initialized equal to
  // GLOBAL_... to maintain uniformity.
  // ---------------------------------------------------------------------
  EstLogPropSharedPtr initialInputLP = (*GLOBAL_EMPTY_INPUT_LOGPROP);
//    new(CmpCommon::statementHeap())EstLogProp(1.0f);
  Context * context = (*CURRSTMT_OPTGLOBALS->memo)[root]->shareContext(rpp,NULL,costLimit,NULL,
						  (*GLOBAL_EMPTY_INPUT_LOGPROP));


  // ---------------------------------------------------------------------
  // loop over all desired passes
  // ---------------------------------------------------------------------

  //
  // In order to gracefully catch & handle assertion failures in a useful
  // way, we need to temporarily re-route the longjmp target to be inside
  // this loop; so, if any code that a task calls ends up invoking an
  // assertion, we can catch that assertion here and try to recover from
  // it gracefully.
  //
  // For now, what we'll do is : if an assertion fails in optimization pass
  // N, we will abort the loop below and return the best plan from opt. pass
  // N-1.  We have created a new data member in the Context called prevSoln_
  // for this purpose.
  //
  // jmp_buf prev_jmp_target ;
  // memcpy (&prev_jmp_target, &Buf, sizeof(jmp_buf)) ; // store the old longjmp target

  NABoolean didNotCompleteFinalOptimizationPass = FALSE ;

  // possibly skip the short pass
  NABoolean skipShortOptimizationPass =
	(!exceptionMode &&
     !CURRSTMT_OPTDEFAULTS->needShortPassOptimization() &&
     CURRSTMT_OPTDEFAULTS->optLevel() != OptDefaults::MINIMUM);

  if(CmpCommon::getDefault(FORCE_PASS_ONE) == DF_ON)
    skipShortOptimizationPass = FALSE;

  if (skipShortOptimizationPass)
  {
    GlobalRuleSet->nextPass();
  }
///////////////////////////////////////////////


  try
  {
      while (GlobalRuleSet->nextPass())
        {
          if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor())
            (*CURRSTMT_OPTGLOBALS->cascadesPassMonitor).enter();

          if (CURRSTMT_OPTDEFAULTS->optimizerPruning() AND
              GlobalRuleSet->getCurrentPassNumber() == 2 )
          {
            CostScalar forcedPass2costLimit = ActiveSchemaDB()->
              getDefaults().getAsDouble(OPH_PRUNING_PASS2_COST_LIMIT);

            if ( forcedPass2costLimit > csZero )
              //((ElapsedTimeCostLimit *)(context->getCostLimit()))->
              context->getCostLimit()->setUpperLimit(forcedPass2costLimit);
          }
          // start optimization, push initial task

          CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
                          OptimizeGroupTask(context,
                                            guidance,
                                            FALSE,
                                            0, ++stride));

          // -----------------------------------------------------------------
          // optimize
          // while there are tasks undone, do one
          // -----------------------------------------------------------------

          while (NOT CURRSTMT_OPTGLOBALS->task_list->empty())
            {
              CURRSTMT_OPTDEFAULTS->setTaskCount(++ task_count);

              CascadesTask * next_task = CURRSTMT_OPTGLOBALS->task_list->getFirst(); // get a sneak preview
              //                                                // for GUI display

#ifdef NA_DEBUG_GUI
                if (CmpMain::msGui_ && CURRENTSTMT->displayGraph()) {
                  CmpMain::pExpFuncs_->fpDoMemoStep(
                       (Int32)GlobalRuleSet->getCurrentPassNumber(),
                       (Int32) next_task->getGroupId(),
                       task_count,
                       next_task,
                       next_task->getExpr(),
                       next_task->getPlan());
                }
#endif

#ifdef _DEBUG
			  // Show task.
              if ( ( CmpCommon::getDefault( NSK_DBG ) == DF_ON ) &&
                   ( CmpCommon::getDefault( NSK_DBG_PRINT_TASK ) == DF_ON ) )
              {
                CURRCONTEXT_OPTDEBUG->showTask(
                  (Int32)GlobalRuleSet->getCurrentPassNumber(),
                  (Int32) next_task->getGroupId(),
                  task_count,
                  next_task,
                  next_task->getExpr(),
                  next_task->getPlan(),
                  " " );
              }
              // stop at this task if necessary when debugging
              if ( task_count == taskCountToStop )
              {
                // put the breakpoint here then change the value
                // of taskCountToStop. This is much faster than
                // setting conditional breakpoint
                taskCountToStop++;
              }
#endif
              // start the next task
              next_task = CURRSTMT_OPTGLOBALS->task_list->pop();
              CascadesTask::cascadesTaskTypeEnum
                 next_task_type = next_task->taskType();

              if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor())
                (*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[next_task_type]).enter();

              // remember current & previous task (for debugging & tracking)
              CascadesTask* prev = CURRSTMT_OPTDEFAULTS->getCurrentTask();
              CURRSTMT_OPTDEFAULTS->setCurrentTask(next_task);

	      next_task->perform(task_count);  // tasks destroy themselves

              // previous task becomes current task (for debugging & tracking)
              CURRSTMT_OPTDEFAULTS->setCurrentTask(prev);

              if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor())
                (*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[next_task_type]).exit();

            } // main optimization loop over remaining tasks in task list

          // If running in exception mode, do only first pass
		  if (exceptionMode)
            break;

          //
          // After each pass and if this is not the last pass,
          // we bind the current best plan in order to
          // facilitate error-handling in case we catch an assertion
          // failure in the middle of the *next* optimization pass,
          // or if the next pass fails to produce a plan for some
          // reason other than an assertion failure.
          //
          if (NOT GlobalRuleSet->inLastPass())
            context->setPreviousSolution() ;

#ifdef NA_DEBUG_GUI
          //---------------------------------------------------------------------
          // GSH : Hide the Sqlcmpdbg display if it is not hidden. This is possible
          // if you were steping through the memo optimization.
          //---------------------------------------------------------------------
          if (CmpMain::msGui_ && CURRENTSTMT->displayGraph()) {
            CmpMain::pExpFuncs_->fpHideQueryTree(TRUE);
          }
#endif

          if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor())
            (*CURRSTMT_OPTGLOBALS->cascadesPassMonitor).exit();

          if (printCounters)
            {
              char string[200];
              printf ("\n");
              *CmpCommon::diags() << DgSqlCode(arkcmpOptimizerCountersWarning);
              sprintf(string, 
                     "pass %d complete:\n",
                     GlobalRuleSet->getCurrentPassNumber());
              report(string);
              *CmpCommon::diags() << DgString0(string);
              sprintf(string, 
                     " %d groups, %d tasks, %d rules,\n",
                     CURRSTMT_OPTGLOBALS->memo->getCountOfUsedGroups(),
                     task_count,
                     GlobalRuleSet->getRuleApplCount());
              report(string);
              *CmpCommon::diags() << DgString1(string);
              sprintf(string, 
                     "%d groups merged, %d expr. cleaned up, %d tasks pruned\n",
                     CURRSTMT_OPTGLOBALS->group_merge_count,
                     CURRSTMT_OPTGLOBALS->garbage_expr_count,
                     CURRSTMT_OPTGLOBALS->pruned_tasks_count);
              report(string);
              *CmpCommon::diags() << DgString2(string);
              sprintf(string, 
                     "%d/%d/%d/%d log/phys/plans/dupl expressions in CascadesMemo\n",
                     CURRSTMT_OPTGLOBALS->logical_expr_count,
                     CURRSTMT_OPTGLOBALS->physical_expr_count,
                     CURRSTMT_OPTGLOBALS->plans_count,
                     CURRSTMT_OPTGLOBALS->duplicate_expr_count);
              report(string);
              *CmpCommon::diags() << DgString3(string);
#ifdef _DEBUG
              sprintf(string, 
                     "%d asm hits and %d asm misses\n",
                     CURRSTMT_OPTGLOBALS->asm_count,CURRSTMT_OPTGLOBALS->cascade_count);
              report(string);
              *CmpCommon::diags() << DgString4(string);
#endif
            }


#ifdef NA_DEBUG_GUI
          // Display optimal plan at the end of each optimization pass
          Sqlcmpdbg::CompilationPhase optPass = Sqlcmpdbg::AFTER_OPT1;
          switch (GlobalRuleSet->getCurrentPassNumber()) {
          case 1:	optPass = Sqlcmpdbg::AFTER_OPT1; break;
          case 2:	optPass = Sqlcmpdbg::AFTER_OPT2; break;
          default:	break;
          }
          if (CmpMain::msGui_ && CURRENTSTMT->displayGraph())
            {
              CMPASSERT(gpClusterInfo != NULL);
              CmpMain::pExpFuncs_->fpSqldbgSetCmpPointers(
                   CURRSTMT_OPTGLOBALS->memo,
                   CURRSTMT_OPTGLOBALS->task_list,
                   QueryAnalysis::Instance(),
                   cmpCurrentContext,
                   gpClusterInfo);

              CmpMain::pExpFuncs_->fpDisplayQueryTree(optPass, NULL,
                                                      (void*) context->getSolution());
            }

#endif

          if ( context->getSolution() && ( CmpCommon::getDefault( NSK_DBG ) == DF_ON ) )
          {
            Int32 passNumber = GlobalRuleSet->getCurrentPassNumber();
            if ( ( passNumber == 1 &&
                   CmpCommon::getDefault( NSK_DBG_SHOW_PASS1_PLAN ) == DF_ON )
                 OR
                 ( passNumber == 2 &&
		   CmpCommon::getDefault( NSK_DBG_SHOW_PASS2_PLAN ) == DF_ON ) )
            {
              CURRCONTEXT_OPTDEBUG->stream() << endl << "Plan chosen after Pass "
                              << passNumber << endl;
              CURRCONTEXT_OPTDEBUG->showTree( context->getSolution()->getPhysicalExpr(),
                               context->getSolution(),
                               "  " );
              CURRCONTEXT_OPTDEBUG->stream() << endl;
            }
          }

#ifdef _DEBUG
          if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor() &&
              (CmpCommon::getDefault(COMPILE_TIME_MONITOR_LOG_ALLTIME_ONLY) == DF_OFF)
          )
	  {
            const char * fname = getCOMPILE_TIME_MONITOR_OUTPUT_FILEname();
	    if (fname)
              CURRCONTEXT_OPTDEBUG->showMemoStats(CURRSTMT_OPTGLOBALS->memo, " ", CURRCONTEXT_OPTDEBUG->stream());
	    else
	      CURRCONTEXT_OPTDEBUG->showMemoStats(CURRSTMT_OPTGLOBALS->memo, " ", cout);
	  }
#endif
        } // loop over all desired passes
    }
    catch (AssertException & compEx)
    {
      // should print some sort of warning message here

      // need to check : did pass 1 optimizations (at least) complete?
      // --> if not, then we can't do any better than simply failing
      //     an assertion at this point
      // NB: we had to catch an assertion failure to reach this code
      //     anyway, so we're saying :  if we didn't complete
      //     pass 1 successfully, then we cannot do any better than
      //     simply passing along the assertion failure

      NABoolean compilation_did_not_complete_pass_one =
        GlobalRuleSet->getCurrentPassNumber() > 1 ;

      Int32 currentPass = GlobalRuleSet->getCurrentPassNumber();

      // turn off any CQSWA pointers
      CURRENTSTMT->clearCqsWA();

      // If the query contains an AntiSemiJoin (or embedded delete),
      // then we added the
      // JoinToTSJ rule to the pass 1 rule set. Need to remove it
      // now so it won't be enabled in pass1 for the next query.
      if (CURRSTMT_OPTDEFAULTS->isJoinToTSJRuleNeededOnPass1())
      {
        GlobalRuleSet->disable(JoinToTSJRuleNumber,
                               GlobalRuleSet->getFirstPassNumber(),
                               GlobalRuleSet->getFirstPassNumber());
      }

      // Unbind context-heap rules' substitute-expressions from
      // statement-heap groupAttrs, to prevent invalid dangling pointers
      // when optimizing the next query.
      ReinitRuleSet(GlobalRuleSet);

      // NB: ambiguous (opposite boolean value as expected from name)
      //     boolean variable intended for comprehensibility for
      //     end-user (I really should create a new error message anyway)


// Logging the optimization failure indicator.
      NAString msg;
      if ( currentPass == 1 ) 
      {
          // Spit out a more descriptive msg, and then do a longjmp.
          msg = "SQL compiler : Pass one optimization failed. "  
                "A previously logged assertion failure may contain " 
                "more information about the nature of the problem. ";

          SQLMXLoggingArea::logSQLMXPredefinedEvent(msg.data(), LL_WARN);

          CmpInternalException("Pass1 Failed", __FILE__ , __LINE__).throwException();
      }
      else

      if ( currentPass >= 2 ) 
      {
          msg = "SQL compiler: Optimization failed at pass two or higher. " 
                "Execution will use the plan generated at pass one instead.";

          SQLMXLoggingArea::logSQLMXPredefinedEvent(msg.data(), LL_WARN);
      }

      // ASSUME : if we're looking at the debugging messages,
      //          we want to see all assertion failures
      const char * USER_WANTS_ALL_ASSERTIONS = getenv("SQL_WANT_ALL_ASSERTIONS");
      CMPASSERT(!USER_WANTS_ALL_ASSERTIONS);

      // Otherwise, we're going to recover from this assertion
      // failure gracefully

      // print out the diagnostics error messages
      // NADumpDiags(cerr,CmpCommon::diags()) ;

      // Make all previous errors warnings
      for (Lng32 i=0; i<CmpCommon::diags()->getNumber(DgSqlCode::ERROR_); i++)
        CmpCommon::diags()->negateCondition(i);

      // Produce a new warning saying that we are trying to recover
      *CmpCommon::diags() << DgSqlCode(arkcmpErrorAfterPassOne);

      cerr << "*** WARNING: ASSERTION FAILURE CAUGHT BY OPTIMIZER! ***" << endl ;
      cerr << "***    attempting to recover and produce a plan     ***" << endl ;
      cerr << "*** (ignore the previous assertion failure message) ***" << endl ;

      didNotCompleteFinalOptimizationPass = TRUE ;
    }

  // restore the old longjmp target
  // memcpy (&Buf, &prev_jmp_target, sizeof(jmp_buf)) ;
  

  if (CURRSTMT_OPTGLOBALS->countExpr)
  {
     CURRSTMT_OPTGLOBALS->memo->print_all_trees(root, TRUE, FALSE);
     CURRSTMT_OPTGLOBALS->memo->print_all_trees(root, FALSE, TRUE);
  }


  // ---------------------------------------------------------------------
  // extract optimal plan
  // ---------------------------------------------------------------------

  // if we aborted the above loop due to an internal optimizer
  // assertion, then we want to use the solution we discovered in a
  // previous pass of the optimizer.
  // if there was no previous optimization pass because we skipped
  // the first optimization pass, then we will try to generate a plan
  // by recalling this optimize method again using the exception recovery
  // mode which will optimize with first pass only.
  if ( didNotCompleteFinalOptimizationPass )
  {
    // check if there was no previous plan, this would happen if we were in
    // the second pass and the first pass was skipped
    if (skipShortOptimizationPass &&
		(Int32)GlobalRuleSet->getCurrentPassNumber() == 2)
    {
      delete CURRSTMT_OPTGLOBALS->task_list;
      CURRSTMT_OPTGLOBALS->task_list = NULL;
      // do not delete memo at this point yet
      CascadesMemo* secondaryMemo = CURRSTMT_OPTGLOBALS->memo;
      CURRSTMT_OPTGLOBALS->memo = NULL;
      plan = treeCopy->optimize(TRUE);
    }
    else // there was a previous plan
    {
      plan = context->bindSolutionTree(TRUE) ; // TRUE == use prevSolution_
    }
  }
  else
  {
    plan = context->bindSolutionTree(FALSE) ; // FALSE == use solution_
    // If pass2 failed to produce a plan for some reason, then we want
    // to fallback to the pass1 plan. This shouldn't happen.
    if (plan == NULL)
    {
      // A previous plan will not be possible if we skipped the first
      // optimization pass and the current pass is the second.
      if (NOT (skipShortOptimizationPass &&
		       GlobalRuleSet->getCurrentPassNumber() == 2))
	  {
        plan = context->bindSolutionTree(TRUE) ; // TRUE == use prevSolution_
	  }
    }
  }

  // clean up (don't delete memo yet, the extracted plan is still
  // a part of it; memo must be deleted after the generator phase)
  //delete CURRSTMT_OPTGLOBALS->task_list;
  CURRSTMT_OPTGLOBALS->task_list = NULL;

  // If the query contains an AntiSemiJoin, then we added the
  // JoinToTSJ rule to the pass 1 rule set. Need to remove it
  // now so it won't be enabled in pass1 for the next query.
  if (CURRSTMT_OPTDEFAULTS->isJoinToTSJRuleNeededOnPass1())
  {
    GlobalRuleSet->disable(JoinToTSJRuleNumber,
                           GlobalRuleSet->getFirstPassNumber(),
                           GlobalRuleSet->getFirstPassNumber());
  }

  // Now we print the appropriate error message if no plan was generated
  // and no earlier error was reported
  const NABoolean compilation_was_not_successful = (plan == NULL) ;
  if (compilation_was_not_successful &&
      !CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) )
  {
    // QSTUFF
    if (getOperatorType() == REL_ROOT){
      RelRoot * root = (RelRoot *) this;
      if (root->isTrueRoot() && (!(root->reqdOrder().isEmpty()))){
	if (context->getGroupAttr()->isStream()){
	  *CmpCommon::diags() << DgSqlCode(4153);
	}
	if (context->getGroupAttr()->isEmbeddedUpdateOrDelete()){
	  *CmpCommon::diags() << DgSqlCode(4154)
			      << (getGroupAttr()->isEmbeddedUpdate() ?
				  DgString0("update"): DgString0("delete"));
	}
      }
    }
    // QSTUFF
    // First we look to see if there is a CQS in effect
    if (ActiveControlDB()->getRequiredShape() &&
        ActiveControlDB()->getRequiredShape()->getShape() &&
        NOT ActiveControlDB()->getRequiredShape()->getShape()->isCutOp())
    {
      // Check the optimization level
      if (CURRSTMT_OPTDEFAULTS->optLevel() == OptDefaults::MINIMUM)
      {
        *CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileWithMinimumAndCQS);
      }
      else if (CURRSTMT_OPTDEFAULTS->randomPruningOccured())
      {
        *CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileWithMediumAndCQS);
      }
      else
      {
        *CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileWithCQS);
      }
    }
    else if (CmpCommon::statement()->getTMUDFRefusedRequirements())
    {
      // a TMUDF refused to satisfy some required properties during
      // compilation, altert the user to that possible cause
      const LIST(const NAString *) *reasonList =
        CmpCommon::statement()->getDetailsOnRefusedRequirements();

      // add up to 5 possible reasons to the diags area
      for (CollIndex i=0; i<reasonList->entries() && i < 5; i++)
        *CmpCommon::diags() << DgSqlCode(-11153)
                            << DgString0((*reasonList)[i]->data());
    }
    else // there is no pub/sub, CQS or TMUDF involved
    {
      // Check the optimization level
      if (CURRSTMT_OPTDEFAULTS->optLevel() == OptDefaults::MINIMUM)
      {
        *CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileWithMinimum);
      }
      else if (CmpCommon::diags()->getNumber(DgSqlCode::WARNING_) != 0)
      {
        // Direct the user to the warnings in the diags area.
        *CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileSeeWarning);
      }
      else
      {
        // Give up.  Internal error.
        *CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileQuery);
      }
    }
  }
  // QSTUFF
  // we want to remove potential warnings which we create while
  // searching for an acceptable plan, e.g. generated when searching
  // for a suitable index  to satisfy an order by clause on a stream
if (plan && context->getGroupAttr()->isStream())
    clearFailedPlanWarningsForStream();
  // QSTUFF

  return ( plan );
} // optimize

//<pb>
/* ============================================================ */

/*
  Task context
  ============
*/

Context::Context(CascadesGroupId groupId,
		 const ReqdPhysicalProperty * const reqdPhys,
                 const InputPhysicalProperty * const inputPhys,
		 const EstLogPropSharedPtr& inputLogProp)
       : reqdPhys_(reqdPhys), inputPhys_(inputPhys), costLimit_(NULL),
         groupId_(groupId), doneInPass_(-1), outstanding_(0),
         currentAncestor_(NULL), duplicateOf_(NULL), predecessorTask_(NULL),
	 costLimitExceeded_(FALSE), currentPlanExceededCostLimit_(FALSE),
	 pruningEnabled_(CURRSTMT_OPTDEFAULTS->optimizerPruning()),
         niceContext_(FALSE),
         inputEstLogProp_(inputLogProp),
         triedEnforcerRules_(CmpCommon::statementHeap()),
         ignoreTheseRules_(CmpCommon::statementHeap()),
         currentPlan_(NULL), solution_(NULL),prevSolution_(NULL),
         myPws_(NULL),
	 candidates_(CmpCommon::statementHeap())
{  
  CURRSTMT_OPTGLOBALS->contextCounter->incrementCounter();
  CURRENTSTMT->getCompilationStats()->incrOptContexts();
} // Context::Context

Context::~Context()
{
  delete costLimit_;

  // the expressions pointed to by the candidates_ , solution_ , and
  // reqdPhys_ pointers aren't owned by the context, so don't delete them.

  CURRSTMT_OPTGLOBALS->contextCounter->decrementCounter();

} // Context::~Context
//<pb>

void Context::print(FILE * f, const char * prefix, const char * suffix) const
{
#ifdef _DEBUG
  fprintf(f, "%sContext %p\n", prefix, this);

  fprintf(f, " niceContext: %d ", niceContext_);
  if (reqdPhys_ != NULL)
    reqdPhys_->print(f, CONCAT(prefix," reqd phys: "));
  if (costLimit_)
    fprintf(f, CONCAT(prefix," cost limit: %g"),
            costLimit_->getValue(getReqdPhysicalProperty()));

  fprintf(f, "\n%s", suffix);
#endif  // _DEBUG
} // Context::print
//<pb>

NAString Context::getRequirementsString() const
{
  NAString IPPString("", CmpCommon::statementHeap());

  // will receive the string "ptr=%p, ", where %p is a hex address
  // example:  "ptr=0x7fffffff23db, "
  // so we need 4 bytes for "ptr=", 2 bytes for the "0x", up to
  // 2 * sizeof(void *) bytes for the hex (each nibble goes to
  // one ASCII character), 2 bytes for the ", " and 1 byte for
  // the trailing null 
  char thisptr[4 + 2 + 2*sizeof(void *) + 2 + 1];

  if ( CmpCommon::getDefault( NSK_DBG_PRINT_CONTEXT_POINTER ) == DF_ON )
  {
    sprintf(thisptr, "ptr=%p, ",this);
    thisptr[sizeof(thisptr)-1]='\0';  // shouldn't be needed, but be safe
  }
  else
    thisptr[0]='\0';

  if (inputPhys_ != NULL)
  {
    IPPString = ", IPP: " + getIPPString();
  }

  return NAString(thisptr) + NAString("Cost limit: ") + getCostString() +
    ", RPP: " + getRPPString() + IPPString + ", ILP: " + getILPString();
}

//GTOOL
NAString Context::getCostString() const
{
 //------------------------------------------------------------------
 // GSH : Return a one-line description of the cost limit.
 //------------------------------------------------------------------
  NAString costString; // may be empty or cost limit xxx,

  if (getCostLimit())
    {
      char costLimitStr[50];
      sprintf(costLimitStr, "%g",
	     getCostLimit()->getValue(reqdPhys_));
      costString += costLimitStr;
    }
  else
    {
      costString = "none";
    }

  return costString;
}

//GTOOL
NAString Context::getRPPString() const
{
  // Return a one-line description of the optimization goal. Examples:
  // no required props
  // required sort order by (a,b,c)
  // required arrangement by (d,e,f)
  // required sort order by (x,y,z), partitioning on (value,w), executed in ESP

  NAString propString; // non-empty, "requires ..." or "no required props"

  if (reqdPhys_ != NULL)
    {
      // strings get added in the form ", description", the prefix gets added
      // later and the first comma gets taken out

      if (reqdPhys_->getArrangedCols() != NULL)
	{
	  propString += ", arranged cols (";
	  ((ValueIdSet *)(reqdPhys_->getArrangedCols()))->unparse(
	       propString,DEFAULT_PHASE,EXPLAIN_FORMAT);
	  propString += ")";
	}
      if (reqdPhys_->getSortKey() != NULL)
	{
	  propString += ", sort order (";
	  ((ValueIdList *)(reqdPhys_->getSortKey()))->unparse(
	       propString,DEFAULT_PHASE,EXPLAIN_FORMAT);
	  propString += ")";
	}
      // Only display sort order type requirement and dp2 sort
      // order partitioning requirement if there was a required
      // arrangement or order.
      if ((reqdPhys_->getArrangedCols() != NULL) OR
          (reqdPhys_->getSortKey() != NULL))
      {
        propString += ", sort order type req ";
        switch (reqdPhys_->getSortOrderTypeReq())
        {
          case NO_SOT:
            propString += "NONE";
            break;
          case ESP_NO_SORT_SOT:
            propString += "ESP_NO_SORT";
            break;
          case ESP_VIA_SORT_SOT:
            propString += "ESP_VIA_SORT";
            break;
          case DP2_SOT:
            propString += "DP2";
            break;
          case ESP_SOT:
            propString += "ESP";
            break;
          case DP2_OR_ESP_NO_SORT_SOT:
            propString += "DP2_OR_ESP_NO_SORT";
            break;
          default:
            // should never happen
            propString += "";
            break;
        }

        if (reqdPhys_->getDp2SortOrderPartReq() != NULL)
        {
          propString += ", dp2 sort order part req ";
          propString += reqdPhys_->getDp2SortOrderPartReq()->getText();
        }
      }
      if (reqdPhys_->getLogicalOrderOrArrangementFlag() != FALSE)
      {
        propString += ", order or arrangement req is logical ";
      }

      if (reqdPhys_->getPartitioningRequirement() != NULL)
	{
          propString += ", ";
          propString += reqdPhys_->getPartitioningRequirement()->getText();
        }

      if (reqdPhys_->getLogicalPartRequirement())
        {
          propString += ", log part req ";
	  switch (reqdPhys_->getLogicalPartRequirement()->getLogPartTypeReq())
	    {
	    case LogPhysPartitioningFunction::PA_PARTITION_GROUPING:
	      propString += "PA_PART_GROUPING ";
	      break;
	    case LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING:
	      propString += "LOG_SUBPART ";
	      break;
	    case LogPhysPartitioningFunction::HORIZONTAL_PARTITION_SLICING:
	      propString += "HORIZ_PART_SLICING ";
	      break;
	    default:
	      break;
	    }
	  if (reqdPhys_->getLogicalPartRequirement()->getNumClientsReq() !=
	      ANY_NUMBER_OF_PARTITIONS)
	    {
	      char ncr[20];
	      sprintf(ncr, "%d client(s) ",
		      reqdPhys_->getLogicalPartRequirement()->
		      getNumClientsReq());
	      propString += ncr;
	    }
	  if (reqdPhys_->getLogicalPartRequirement()->getMustUsePapa())
	    propString += "must use PAPA ";
	  if (reqdPhys_->getLogicalPartRequirement()->getLogReq())
	    propString += reqdPhys_->getLogicalPartRequirement()->
	      getLogReq()->getText();
        }

      if (reqdPhys_->getPlanExecutionLocation() == EXECUTE_IN_DP2)
	{
	  propString += ", in DP2";
	}
      else if (reqdPhys_->getPlanExecutionLocation() == EXECUTE_IN_ESP)
	{
	  propString += ", in ESP";
	}
      else if (reqdPhys_->getPlanExecutionLocation() == EXECUTE_IN_MASTER)
	{
	  propString += ", in master";
	}
      else if (reqdPhys_->getPlanExecutionLocation() == EXECUTE_IN_MASTER_OR_ESP)
	{
	  propString += ", in master/ESP";
	}

      if (reqdPhys_->getMustMatch() != NULL)
	{
	  propString += ", must match ";
	  propString += reqdPhys_->getMustMatch()->getText();
	}


      if (reqdPhys_->getPartitioningRequirement() != NULL)
      {
         PartitioningRequirement* partReq =
             reqdPhys_->getPartitioningRequirement();

         if ( partReq->isRequirementFuzzy() )
           propString += (partReq ->
                            castToFuzzyPartitioningRequirement() ->
                                getSkewProperty()).getText();
         else
           if ( partReq->isRequirementSkewed() )
             propString += (partReq ->
                              castToRequireSkewed() ->
                                  getSkewProperty()).getText();
      }

      const PushDownRequirement* pdr = reqdPhys_ -> getPushDownRequirement();

      if ( pdr != NULL ) {
        if ( pdr->castToPushDownCSRequirement() ) {
           propString +=  " ,CSOnePartition req";
        } else
        if ( pdr->castToPushDownColocationRequirement() ) {
           propString +=  " ,colocation req";
        }
      }

    }

  if (propString.length() > 0)
    {
      propString.remove(0,2); // remove leading comma and blank
    }
  else
    {
      propString = "none";
    }

  return propString;
}

//GTOOL
NAString Context::getIPPString() const
{
  // Return a one-line description of the input physical properties.
  const Int32 MAX_STR_LEN = 1001;

  NAString propString("", CmpCommon::statementHeap());

  if (inputPhys_ != NULL)
  {
    // Don't print out the outer char outputs for now - there's
    // way too much output and it clogs up the display.
    // if (NOT inputPhys_->getNjOuterCharOutputs().isEmpty() )
    // {
    //   propString += ", outer table char outputs ";
    //   char validascii[MAX_STR_LEN];
    //   NAString propText;
    //   for (ValueId x = inputPhys_->getNjOuterCharOutputs().init();
    //                    inputPhys_->getNjOuterCharOutputs().next(x);
    //                    inputPhys_->getNjOuterCharOutputs().advance(x))
    //   {
    //     sprintf(validascii,"ValId #%d: ",(CollIndex) x);
    //     propText = validascii;
    //     x.getItemExpr()->unparse(propText);
    //     propString += propText;
    //   } //end for
    // }

    if (inputPhys_->getNjOuterOrder() != NULL)
    {
      propString += ", outer table sort order (";
        ((ValueIdList *)(inputPhys_->getNjOuterOrder()))->unparse(
          propString,DEFAULT_PHASE,EXPLAIN_FORMAT);
      propString += ")";
    }
    if (inputPhys_->getNjOuterOrderPartFunc() != NULL)
    {
      propString += ", outer table log part func ";
      propString += inputPhys_->getNjOuterOrderPartFunc()->getText();
    }
    if (inputPhys_->getNjDp2OuterOrderPartFunc() != NULL)
    {
      propString += ", outer table phys part func ";
      propString += inputPhys_->getNjDp2OuterOrderPartFunc()->getText();
    }
  } // end if input physical properties exist


  if (propString.length() > 0)
  {
    propString.remove(0,2); // remove leading comma and blank
  }

  return propString;
}

//GTOOL
NAString Context::getILPString() const
{
  //------------------------------------------------------------------
  // GSH : Return a one-line description of Input Logical Property.
  //------------------------------------------------------------------

  NAString ILPString; // may be empty
  if (inputEstLogProp_ != NULL)
    {
	  ILPString += "rows: ";
      char numRows[30];
      sprintf(numRows, "%g",inputEstLogProp_->getResultCardinality().value());
      ILPString += numRows;
    }
  else
    {
      ILPString = "none";
    }

  return ILPString;
}

//GTOOL
NAString Context::getStatusString() const
{
  // This method is used in the GUI, which statically links with the
  // optimizer library. Unfortunately, this means that the GUI has its
  // own copy of global variables like GlobalRuleSet and therefore
  // can't access the current optimization pass :-(

  NAString result(CmpCommon::statementHeap());

  if (duplicateOf_)
    result = "Dupl: ";

  if (outstanding_ > 0)
    {
      char outst[50];
      sprintf(outst, "in progress, %d tasks", outstanding_);
      result += outst;
    }
  else if (costLimitExceeded_)
    result += "limit. exc.";
  else if (solution_)
    result += "optimal";
  else if (doneInPass_ >= 0)
    result += "failed";
  else
    result += "new";

  if (doneInPass_ >= 0 AND outstanding_ == 0)
    {
      char ppn[20];
      sprintf(ppn, " in pass %d",doneInPass_);
      result += ppn;
    }

  return result;
}

//<pb>
// ----------------------------------------------------------------------
// See if we can steal an optimal solution for this context from another
// context. If pruning is enabled, also see if we can steal candidate
// plans from other contexts and possibly make one of them my best
// solution so far.
// ----------------------------------------------------------------------
NABoolean Context::findBestSolution()
{

  // should only call this before any optimization is done or after it has
  // completed (this restriction may be relaxed if necessary)
  CMPASSERT(getOutstanding() == 0);

  // ---------------------------------------------------------------------
  // has this context already been optimized during the current pass?
  // ---------------------------------------------------------------------
  if (optimizedInCurrentPass())
    return(TRUE);

  // If plan stealing is disabled, then we cannot allow this method to
  // do its job. Return FALSE now.
  if (CmpCommon::getDefault(PLAN_STEALING) == DF_OFF)
    return(FALSE);

  // ---------------------------------------------------------------------
  // Scan all other contexts in this group to find optimal plans to steal,
  // or, if pruning is enabled, candidate plans to steal.
  // ---------------------------------------------------------------------
  Int32 maxc = (Int32)((*CURRSTMT_OPTGLOBALS->memo)[groupId_]->getCountOfContexts()); //# of contxts in grp
  Context        *otherContext;                        // another contxt in grp
  CascadesPlan   *plan;                                // a potential solution
  COMPARE_RESULT comp;                                 // this ?? otherContext
  Int32 i, j;

  for (i = 0; i < maxc; i++)
  {
    otherContext = (*CURRSTMT_OPTGLOBALS->memo)[groupId_]->getContext(i);

    // We will only be able to steal an optimal plan from the other context
    // if the other context has an optimal plan from this pass.
    // We will only be able to steal candidate plans if pruning is enabled,
    // because if pruning is disabled we don't store candidate plans.
    if (otherContext->hasOptimalSolution() OR
        otherContext->isPruningEnabled())
    {
      Lng32 numOtherPlans = otherContext->getCountOfCandidatePlans();

      comp = compareContexts(*otherContext);

      switch (comp)
      {
        case LESS:

        // ---------------------------------------------------------
        // Any candidate in the other context is also a candidate
        // for this context, since this context requires less.
        // Note that the candidate plans will be empty if pruning is
        // disabled.
        // ---------------------------------------------------------
        for (j = 0; j < numOtherPlans; j++)
        {
          addCandidatePlan(otherContext->getCandidatePlan(j));
        }

        break;

        case SAME:

        // ---------------------------------------------------------
        // The two contexts are the same.
        // ---------------------------------------------------------
        if (this != otherContext) // if they are two distinct Contexts
        {
          // this cannot be a duplicate because duplicates cannot be optimized
          CMPASSERT(NOT this->isADuplicate());
          // and the other must be, recursively, a duplicate of this:
          Context *myDuplicate = otherContext->getDuplicateOf();
          while ((myDuplicate != NULL) && myDuplicate != this)
          {
            myDuplicate = myDuplicate->getDuplicateOf();
          }
          CMPASSERT(myDuplicate != NULL);
        }

        // nothing to do here, we already looked at this context
        break;

        case MORE:

        // ---------------------------------------------------------
        // My required physical properties dominate the required
        // physical properties of otherContext.
        // ---------------------------------------------------------

        // ---------------------------------------------------------
        // otherContext has an optimal solution that was created in
        // the current pass.
        // ---------------------------------------------------------
        if (otherContext->hasOptimalSolution())
        {
          DBGLOGPLAN(otherContext->getSolution());
          // -----------------------------------------------------
          // If my cost limit is less than the cost for the
          // solution of a Context whose required physical
          // properties are dominated by mine, then I cannot
          // produce a solution. (I want "more" but I am
          // prepared to pay "less" for it!)
          // -----------------------------------------------------
          if (costLimit_ AND
              (costLimit_->compareWithPlanCost
               ((CascadesPlan *)otherContext->getSolution(),reqdPhys_) == LESS) )
          {
            costLimitExceeded_ = TRUE;
            DBGLOGMSGCNTXT(" *** CL exceeded by exist.soln cost ",otherContext);

            // we can still reuse thah solution when OPH_REUSE_FAILED_PLAN_COST
            // is ON and we don't have a solution yet or other context solution
            // is better than ours
            if ( CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() AND
                 satisfied(otherContext->solution_) AND
                 ( solution_ == NULL OR
                   NOT solution_->succeededInCurrentPass()
                 )
               )
            {
              setSolution(otherContext->solution_, FALSE);
              doneInPass_ = GlobalRuleSet->getCurrentPassNumber();
              DBGLOGMSGCNTXT(" *** Stolen plan exceeding my CL ",otherContext);
            }

            return(TRUE);
          }
          // -----------------------------------------------------
          // If the solution for otherContext satisfies my
          // required physical properties, then make it my
          // solution also.
          // -----------------------------------------------------
          else if (satisfied(otherContext->solution_))
          {
            if ( solution_ == NULL OR
                 NOT solution_->succeededInCurrentPass()
               )
            {
              DBGLOGMSGCNTXT(" *** Stolen plan *** ",otherContext);
              setSolution(otherContext->solution_, FALSE);
              doneInPass_ = GlobalRuleSet->getCurrentPassNumber();
            }

            return(TRUE);
	  }
        }
        // ---------------------------------------------------------
        // otherContext does not have an optimal solution yet.
        // ---------------------------------------------------------
        else
        {
          // -----------------------------------------------------
          // If otherContext was processed in the same pass and
          // does not have an optimal solution, then check
          // whether my cost limit is greater than the cost limit
          // that failed to create a solution.
          // (In order to create an optimal solution in the
          //  current pass, I should be prepared to pay "more"
          //  to get "more"!)
          // -----------------------------------------------------
          if (costLimit_ AND
              otherContext->costLimit_ AND
              (otherContext->doneInPass_ ==
               GlobalRuleSet->getCurrentPassNumber()) AND
              (costLimit_->compareCostLimits
               (otherContext->costLimit_,
                getReqdPhysicalProperty()) != MORE) )
          {
            costLimitExceeded_ = TRUE;
            DBGLOGMSGCNTXT(" *** CL exc-d one of failed contxt",otherContext);
            return(TRUE);
          }
        }

        // fall through to case UNDEFINED and go hunt for potential
        // candidates.

        case UNDEFINED:

        // ---------------------------------------------------------
        // We may or may not find a candidate in this context, try
        // all of them and see. Note that this case also handles the
        // case comp == MORE which is described in the comment above.
        // ---------------------------------------------------------
        // In the UNDEFINED case we can't steal plan for the new context
        // unless it has no mustMatch. If it has a mustMatch then the
        // satsfied(plan) function check only the pattern match of the
        // top operator which doesnot gurantee the correct force
        // Another way to fix this is by declaring two reqdPhys_ with
        // two different mustMatch ptrs as INCOMPATIBLE

        if ((NOT reqdPhys_->getMustMatch()) OR
            (reqdPhys_->getMustMatch()->getArity()==0)) // did little
                           // optimization here by checking mustMatch
                           // arity and allow stealing if its zero)
        {
          for (j = 0; j < numOtherPlans; j++)
          {
            plan = otherContext->getCandidatePlan(j);

            // check whether "plan" satisfies the requirements
            if (satisfied(plan))
              addCandidatePlan(plan);
          }
        }

        break;

        case INCOMPATIBLE:

        // ---------------------------------------------------------
        // We won't find any solution in this context, continue
        // ---------------------------------------------------------
        break;

        default:
          ABORT("internal error");
      } // end case
    } // end if other context has opt. solution or pruning is enabled
  } // end for

  // if we have reached here, no optimal solution has been found
  return(FALSE);

} // Context::findBestSolution()

//<pb>
// The following function will be used by parallelHeuristic2.
// The heuristics will check if the current group is small (less than
// ROWS_PARALLEL_THRESHOLD) and has only one base table to skip
// re-optimization of logical expressions and existing plans for this
// group if the context has partitioning requirements other than
// "exactly one partition" or replication.
NABoolean Context::reoptimizeExprAndPlans()
{
  GroupAttributes *grAttr = (GroupAttributes *)getGroupAttr();
  if ( grAttr->getNumBaseTables() == 1)
  {
     EstLogPropSharedPtr outputLogProp =
       grAttr->outputLogProp(getInputLogProp());

     // check if the group is "small" enough
     CostScalar groupRowCount =
       MIN_ONE(outputLogProp->getResultCardinality().value());
     if ( groupRowCount.value() < CURRSTMT_OPTDEFAULTS->numberOfRowsParallelThreshold() )
     {
       const ReqdPhysicalProperty* rpp = getReqdPhysicalProperty();
       if ( rpp )
       {
         const PartitioningRequirement*
	   pr = rpp->getPartitioningRequirement();
         // if part.requirements require more than one partitions
         // (cannot be casted to one partition) we dont want to
         // reoptimize this small group and return FALSE
         if ( pr )
         {
           if ( (pr->getCountOfPartitions()>1) AND
                NOT pr->isRequirementReplicateViaBroadcast() AND
                NOT pr->isRequirementReplicateNoBroadcast()
              )
           {
             return FALSE;
           }
         }
         else
         {
           const LogicalPartitioningRequirement*
	     lpr = rpp->getLogicalPartRequirement();
           if ( lpr )
           {
             const PartitioningRequirement* logreq = lpr->getLogReq();
             if ( (logreq->getCountOfPartitions()>1) AND
                  NOT logreq->isRequirementReplicateViaBroadcast() AND
                  NOT logreq->isRequirementReplicateNoBroadcast()
                )
             {
                return FALSE;
             }
           }
         } // phys. or log. partreq check

       } // rpp != NULL
     } // threshold check
  } // numBaseTables == 1

  return TRUE;
} //Context::reoptimizeExprAndPlans()

//<pb>
NABoolean Context::satisfied(const CascadesPlan * const plan) const
{
  if (reqdPhys_ == NULL)
    // -------------------------------------------------------------------
    // If there are no required props, any plan will do.
    // -------------------------------------------------------------------
    return plan->getPhysicalProperty() != NULL;
  else
    // -------------------------------------------------------------------
    // Check the required and actual physical properties.
    // -------------------------------------------------------------------
  {

    NABoolean satisfied = ( reqdPhys_->satisfied(getInputLogProp(),
                   plan->getPhysicalExpr(),plan->getPhysicalProperty()));

    if (satisfied &&
        plan->getPhysicalExpr()->getOperatorType() == REL_EXCHANGE)
    {
      const PhysicalProperty* sppForChild =
              getPhysicalPropertyOfSolutionForChild(0);

      // Prohibit ESP-Exchange Under Right side of a nested join
      // first two conditions check for ESP-Exchanges; Dp2 exchanges are
      // ignored. We also want to ignore type-2 nested joins
      // We care about repartitioned exchanges
      if ( sppForChild &&
            ! sppForChild->executeInDP2() &&
            reqdPhys_->getNoEspExchangeRequirement() )
            //plan->getPhysicalProperty()->isPartitioned() &&
            //plan->getPhysicalProperty()->getPartitioningKey().entries() >= 1)
         satisfied = FALSE;
    }
    return satisfied;
  }

} // Context::satisfied

COMPARE_RESULT Context::compareContexts(const Context & other) const
{
  COMPARE_RESULT  result;

  // Input estimated logical properties must be determined the
  // 'same' to determine the contexts compatible.
  if (getInputLogProp()->compareEstLogProp(other.getInputLogProp())==SAME)
  {
    // compare required properties
    if (reqdPhys_ == NULL  AND  other.reqdPhys_ == NULL)
      result = SAME;
    else if (reqdPhys_ == NULL)
      result = LESS;  // this context requires less than the other
    else if (other.reqdPhys_ == NULL)
      result = MORE;
    else // neither of the reqd prop's is NULL
      result = reqdPhys_->compareRequirements(*(other.reqdPhys_));

    // compare input physical properties
    // No need to check if they both point to the exact same object
    // or to NULL.
    if (inputPhys_ != other.inputPhys_)
    {
      if ((inputPhys_ != NULL) AND (other.inputPhys_ != NULL))
      {
        result = combine_compare_results(result,
          inputPhys_->compareInputPhysicalProperties(*(other.inputPhys_)));
      }
      else if ((inputPhys_ == NULL) OR (other.inputPhys_ == NULL))
      {
        // One context has input physical properties, the other one doesn't.
        // They are INCOMPATIBLE.
        result = INCOMPATIBLE;
      }
    }
  }
  else
  {
    result = INCOMPATIBLE;
  }

  return result;

} // Context::compareContexts

//<pb>
// materialize (bind) a complete and optimal tree of physical nodes from a
// given context
RelExpr * Context::bindSolutionTree(NABoolean getPrevSolution) const
{
  // If there was a problem during optimization (for example,
  // an assertion failure), we still want to return a plan.
  // Our strategy is to cache the best plan from the previous
  // optimization pass, so if the current pass fails to produce
  // a plan, then we at least still have the previous pass plan
  // to give to the generator. The getPrevSolution parameter
  // is set to TRUE if pass2 failed to produce a plan, otherwise it
  // is FALSE.

  const CascadesPlan *plan =
    ( getPrevSolution ) ? getPreviousSolution() : getSolution() ;

  RelExpr * result = NULL;

  if (plan != NULL)
  {
    // If we haven't given up on the current pass plan, then the
    // plan MUST have succeeded in this pass. WE CANNOT MIX PLANS
    // FROM PASS1 and PASS2. If this plan or any child plan was
    // not optimized in the current pass, then we can't use it.
    // We need a pure pass2 plan from pass2, otherwise we'll just
    // have to fall back to the pure pass1 plan.
    // NOTE: This should never happen.
    if (NOT getPrevSolution AND
        NOT plan->succeededInCurrentPass())
      return NULL;

    result = plan->getPhysicalExpr();

    // Add a reference to those fields needed by code generation
    // from the CascadesPlan structure to the RelExpr structure.
    result->setOperatorCost(plan->getOperatorCost());
    result->setRollUpCost(plan->getRollUpCost());
    result->setPhysicalProperty(plan->getPhysicalProperty());

    EstLogPropSharedPtr iLP = getInputLogProp();
    CostScalar numProbes = ((iLP->getResultCardinality() > (CostScalar)1) ?
                            iLP->getResultCardinality() : (CostScalar)1);
    result->setInputCardinality(numProbes);
    result->setEstRowsUsed
      ((*CURRSTMT_OPTGLOBALS->memo)[getGroupId()]->getGroupAttr()->outputLogProp(iLP)->
       getResultCardinality() / numProbes);

    result->setMaxCardEst
      ((*CURRSTMT_OPTGLOBALS->memo)[getGroupId()]->getGroupAttr()->outputLogProp(iLP)->
       getMaxCardEst());

    Int32 nc = result->getArity();

    // bind the child nodes to the optimal solutions for the child contexts
    for (Lng32 i = 0; i < nc; i++)
    {
      // get to the child context i and retrieve its optimal physical tree
      if (plan->getContextForChild(i))
      {
        // Check if we already have set the pointer to the optimal
        // child physical expression. This will only be true if we
        // failed to produce a pass2 plan and are rolling back to
        // the pass1 plan. The "GroupIdMode" in this case will be
        // "BINDING". If this is true, then "release" the binding
        // so the GroupIdMode will be reset to "MEMOIZED". If we
        // don't do this, the assignment immediately below will
        // fail because you are not supposed to overwrite the
        // child pointer if you have already set it.
        if (result->child(i).getMode() == ExprGroupId::BINDING)
          result->child(i).releaseBinding();

        result->child(i) =
          plan->getContextForChild(i)->bindSolutionTree(getPrevSolution);
        // If a child failed to produce a plan, then we can't either
        if (result->child(i) == NULL)
        {
          return NULL;
        }
      }
      else
      {
        return NULL;
      }
    } // end for all children
  } // end if there is a plan for the current operator

  // return the optimal solution or NULL, if no such solution exists
  return result;
}


// return TRUE if there is a solution for the context and
// for all the children's contexts,
// otherwise return FALSE

NABoolean Context::setPreviousSolution()
{
  // If there was a problem during optimization (e.g., an assertion failure),
  // we're trying to still return a plan.  Our strategy is to cache the
  // best plan from the previous optimization pass, so if there is an
  // assertion failure in the current pass, then we at least still have
  // a plan to give to the generator.
  //
  // In this function, we go through the Context tree and set each context's
  // prevSolution_ value.

  if(solution_ == NULL) {
    // give up
    return FALSE;
  }

  prevSolution_ = solution_ ;

  // getPreviousSolution() returns prevSolution_;
  const CascadesPlan *plan = getPreviousSolution() ;

  RelExpr * result = plan->getPhysicalExpr() ;
  Int32 nc = result->getArity();

  for (Lng32 i = 0; i < nc; i++)
    {
      if (plan->getContextForChild(i)) {
	NABoolean childHasSolution =
	  plan->getContextForChild(i)->setPreviousSolution();
	if(NOT childHasSolution) {
	  // give up
	  return FALSE;
	}
      }
      else
	CMPASSERT(FALSE) ; // what else can we do?
    }

  return TRUE;
}

//<pb>
void Context::clearFailedStatus()
{
  // Simply reset the costLimitExceeded_ flag.
  // Do not clear the cost for the failed plan because it
  // is a lower bound.
  if (costLimitExceeded_ )
  {
    DBGLOGMSGCNTXT("*** CostLimExceeded status cleaned ",this);
  }
  costLimitExceeded_ = FALSE;
}
//<pb>
void Context::setCostLimit(CostLimit* cl)
{
  // If pruning is disabled, then do not set costlimit
  if (NOT isPruningEnabled())
    {
      if (cl)
        delete cl;
      return;
    }

  if (costLimit_ != cl)
    {
      if (costLimit_ != NULL)
        delete costLimit_;

      // save new cost limit
      costLimit_ = cl;

      if (cl)
	{
          const ReqdPhysicalProperty* const rpp = getReqdPhysicalProperty();

	  if (NOT hasOptimalSolution())
	    {
	      // If the cost limit is negative, it is no longer feasible to
	      // generate any plans using this Context.
              // this is the pruning step : the "bounding" of branch-and-bound
              if ( cl->getValue(rpp) < 0E0 )
              {
                costLimitExceeded_ = TRUE;
                DBGLOGMSGCNTXT("*** CostLimExceeded status set, CL<0 ",this);
              }
              else
                // if context has an optimal solution this flag was not used
                // in original Cascades implementation. After fixing Cost
                // Based Pruning it is used for communication between
                // createContextForAChild() and createPlan() methods in the
                // sense that when reusing a solution with roll-up cost
                // exceeding cost limit, this flag will be set to TRUE to be
                // checked in createPlan(), will affect the proceccing logic
                // and set back to FALSE to allow reusing this solution up
                // in the plan tree. See check for this flag in createPlan()
                costLimitExceeded_ = FALSE;
	    }

	} // endif (cl != NULL)

    } // endif (costLimit_ != cl)

} // Context::setCostLimit
//<pb>
//==============================================================================
//  Make a specified plan the latest solution for this context.  Revise this
// context's cost limit to reflect this latest plan.
//
// Input:
//  plan -- pointer to specified plan.
//
// Output:
//  none
//
// Return:
//  none.
//
//==============================================================================
void
Context::setSolution(CascadesPlan* plan,
                     NABoolean checkAndAdjustCostLimit)
{
  if (solution_ == plan)
    {
      return;
    }

  // Support for resetting of the solution_ field to NULL.
  if (plan == NULL)
  {
    solution_ = NULL;
    return;
  }

  // We prevent duplicate contexts in a plan top-down in
  // CascadesGroup::shareContext. But that still could allow duplicate
  // physical RelExprs on the way up, if two contexts of the same
  // group are used and if plans are stolen from other contexts.
  // Check for this second condition here, on the way up.
  // See Genesis case 10-080707-3925.
  if (plan->exprOccursInChildTree(plan->getPhysicalExpr(),
                                  CURRSTMT_OPTDEFAULTS->maxDepthToCheckForCyclicPlan()))
    {
      // Not a valid plan, it's using the same RelExpr twice in
      // the tree. Return without setting the solution.
      return;
    }

  if (NOT checkAndAdjustCostLimit)
    {
      // short version of this method without the checks below
      solution_ = plan;
      return;
    }

  //--------------------------------------------------------------------
  //  Verify that plan contains a physical operator and that plan's cost
  // does indeed fall below this context's specified cost limit.
  // When reusing failed plan cost limit we allow plan cost on the way up
  // to exceed this context cost limit, but set costLimitExceeded_ flag
  // not only when cost limit becomes negative but earlier (this will
  // help pruning)
  //--------------------------------------------------------------------
  CMPASSERT(    plan->getPhysicalExpr()->isPhysical()
	    AND plan->getPhysicalProperty() != NULL);

  CostLimit* newCostLimit = NULL;

  if ( CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
  {
    if ( costLimit_ )
    {
      if (costLimit_->compareWithPlanCost(plan,reqdPhys_) == LESS)
      {
        // when existing solution exceeded costLimit this flag is used
        // for communication. It will be used in createPlan to mark
        // latestContext as violation costLimit and cleared. See the
        // comment in createPlan() after calling createContextForAChild.
        // Previously, createContextForACHild would just return NULL
        // when costLimit was exceeded without possibility to reuse
        // already existing solution.
        costLimitExceeded_ = TRUE;
        DBGLOGMSGCNTXT("*** CLExceeded_ flag set in setSolution ",this);
      }
      else
      {
        newCostLimit = costLimit_->copy();
        newCostLimit->tryToReduce(*plan->getRollUpCost(),reqdPhys_);
        setCostLimit( newCostLimit );
      }
    }
    else
    {
      //-----------------------------------------------------------
      //  Old solution had no cost limit.  Convert specified plan's
      // cost into the new cost limit.
      //-----------------------------------------------------------
      newCostLimit = plan->getRollUpCost()->convertToCostLimit(reqdPhys_);
      setCostLimit( newCostLimit );
    }

    solution_ = plan;
    return;
  }

  // and here is the old logic used when OPH_USE_FAILED_PLAN_COST
  // is OFF or OPTIMIZER_PRUNING is OFF.

  CMPASSERT(   costLimit_ == NULL
            OR costLimit_->compareWithPlanCost(plan,reqdPhys_) != LESS );
  //---------------------------------------
  //  Store specified plan as new solution.
  //---------------------------------------
  solution_ = plan;

  //---------------------------------------------
  //  Determine if old solution had a cost limit.
  //---------------------------------------------

  if (costLimit_ == NULL)
  {
    //-----------------------------------------------------------
    //  Old solution had no cost limit.  Convert specified plan's
    // cost into the new cost limit.
    //-----------------------------------------------------------
    newCostLimit = plan->getRollUpCost()->convertToCostLimit(reqdPhys_);
  }
  else
  {
    //---------------------------------------------------------
    //  Old solution had a cost limit.  Try to reduce this cost
    // limit based on the latest plan's cost.
    //---------------------------------------------------------
    newCostLimit = costLimit_->copy();
    newCostLimit->tryToReduce(*plan->getRollUpCost(),reqdPhys_);
  }

  //---------------------------------------
  //  Store new cost limit in this context.
  //---------------------------------------
  // this would also set costLimitExceeded_ flag if newCostLimit i
  // is "negative".
  setCostLimit( newCostLimit );

} // Context::setSolution()

//<pb>
NABoolean Context::requiresOrder() const
{
  return (reqdPhys_ AND
	  ( (reqdPhys_->getSortKey() AND
	     reqdPhys_->getSortKey()->entries() > 0) OR
	    (reqdPhys_->getArrangedCols() AND
	     reqdPhys_->getArrangedCols()->entries() > 0) ) );
}

NABoolean Context::requiresPartitioning() const
{
  return (reqdPhys_ AND
	  reqdPhys_->getPartitioningRequirement());
}

NABoolean Context::requiresSpecificLocation() const
{
  // right now every context always specifies a location,
  // (at least it specifies whether it wants to be
  // inside or outside of DP2)
  return (reqdPhys_ != NULL);
}

void Context::addCandidatePlan(CascadesPlan * plan)
{
  // insert the expression into the candidates list
  // when OPH_USE_CANDIDATE_PLANS is ON. Otherwise we don't
  // keep the list of candidate plans and avoid overhead
  // of prcessing this list which can grow considerably.
  // So far skipping candidate plan list processing didn't
  // cause any plan quality regression.
  if ( CURRSTMT_OPTDEFAULTS->OPHuseCandidatePlans() )
    candidates_.insert(plan);

  if (plan->getPhysicalExpr()->isPhysical() AND
      plan->getPhysicalProperty() != NULL AND
      plan->getRollUpCost() != NULL AND
      (solution_ == NULL OR
       solution_->getRollUpCost()->
         compareCosts(*plan->getRollUpCost(),reqdPhys_) == MORE)
     )
  {
    // even when a candidate is from the previous pass we can
    // use its information about solution cost and try to reduce
    // cost limit to give pruning more chances.

    if ( costLimit_ AND CURRSTMT_OPTDEFAULTS->OPHreduceCLfromCandidates() )
      costLimit_->tryToReduce(*(plan->getRollUpCost()),reqdPhys_);

    // Plan is from the current pass and is cheaper than the current solution.
    if ( plan->getCreationPassNumber() ==
         GlobalRuleSet->getCurrentPassNumber()
       )
    {
      setSolution(plan, FALSE);
      DBGLOGPLAN(plan);
      DBGLOGMSGCNTXT(" *** New soln assgnd as above plan ",plan->getContext());
    }
  }
}
//<pb>

void Context::decrOutstanding()
{
  if (--outstanding_ == 0)
    {
      // this optimization task is complete, mark it
      doneInPass_ = (Int32)GlobalRuleSet->getCurrentPassNumber();
    }
}

const RelExpr *
Context::getPhysicalExprOfSolutionForChild(Lng32 childIndex) const
{
  if (getPlan() AND
      getPlan()->getContextForChild(childIndex) AND
      getPlan()->getContextForChild(childIndex)->getSolution())
    return getPlan()->getContextForChild(childIndex)
              ->getSolution()->getPhysicalExpr();
  else
    return NULL;
} // Context::getPhysicalExprOfSolutionForChild()

const PhysicalProperty *
Context::getPhysicalPropertyOfSolutionForChild(Lng32 childIndex) const
{
  if (getPlan() AND
      getPlan()->getContextForChild(childIndex) AND
      getPlan()->getContextForChild(childIndex)->getSolution())
    return getPlan()->getContextForChild(childIndex)
              ->getSolution()->getPhysicalProperty();
  else
    return NULL;
} // Context::getPhysicalPropertyOfSolutionForChild()


const Cost * Context::getCostOfSolutionForChild(Lng32 childIndex) const
{
  if (getPlan() AND
      getPlan()->getContextForChild(childIndex) AND
      getPlan()->getContextForChild(childIndex)->getSolution())
    return getPlan()->getContextForChild(childIndex)
              ->getSolution()->getRollUpCost();
  else
    return NULL;
} // Context::getCostOfSolutionForChild()

const RelExpr *
Context::getPhysicalExprOfSolutionForGrandChild(Lng32 childIndex,
                                                Lng32 grandChildIndex) const
{
  if (getPlan() AND
      getPlan()->getContextForChild(childIndex) AND
      getPlan()->getContextForChild(childIndex)->getSolution())
  {
    const Context * grandChildContext =
      getPlan()->
        getContextForChild(childIndex)->
          getSolution()->
            getContextForChild(grandChildIndex);

    if(grandChildContext &&
       grandChildContext->getSolution())
      return
        grandChildContext->
	  getSolution()->
	    getPhysicalExpr();
    else
      return NULL;
  }
  else
    return NULL;
} // Context::getPhysicalPropertyOfSolutionForGrandChild()

const PhysicalProperty *
Context::getPhysicalPropertyOfSolutionForGrandChild(Lng32 childIndex,
                                                    Lng32 grandChildIndex) const
{
  if (getPlan() AND
      getPlan()->getContextForChild(childIndex) AND
      getPlan()->getContextForChild(childIndex)->getSolution())
  {
    const Context * grandChildContext =
      getPlan()->
        getContextForChild(childIndex)->
          getSolution()->
            getContextForChild(grandChildIndex);

    if(grandChildContext &&
       grandChildContext->getSolution())
      return
        grandChildContext->
	  getSolution()->
	    getPhysicalProperty();
    else
      return NULL;

  }
  else
    return NULL;
} // Context::getPhysicalPropertyOfSolutionForGrandChild()

const GroupAttributes * Context::getGroupAttr() const
{
  return (*CURRSTMT_OPTGLOBALS->memo)[groupId_]->getGroupAttr();
}

//<pb>
/* ============================================================ */

CascadesPlan::CascadesPlan(RelExpr * physExpr, Context * context)
  : physExpr_(physExpr)
  ,operatorCost_(NULL)
  ,rollUpCost_(NULL)
  ,planElapsedTime_(csZero)
  ,planExceededCostLimit_(FALSE)
  ,context_(context)
  ,passNoWhenCreated_(GlobalRuleSet->getCurrentPassNumber())
  ,succeededInPassNo_(-1)
  ,failedInPassNo_(-1)
  ,physProp_(NULL)
  ,childContexts_(CmpCommon::statementHeap())
  ,isBMO_(FALSE)
  {
     CURRSTMT_OPTGLOBALS->cascadesPlanCounter->incrementCounter();
  }

CascadesPlan::~CascadesPlan()
{
  // context_->decrReferenceCount();

  // The physExpr_  is not deallocated here

  delete operatorCost_;
  delete rollUpCost_;
  delete physProp_;
  for (CollIndex index = 0; index < childContexts_.entries(); index++)
    delete childContexts_[index];
  CURRSTMT_OPTGLOBALS->cascadesPlanCounter->decrementCounter();

}  // CascadesPlan::~CascadesPlan

void CascadesPlan::setOperatorCost(Cost *cost)
{
  if (operatorCost_)
    delete operatorCost_;
  operatorCost_ = cost;
}

void CascadesPlan::setRollUpCost(Cost *cost)
{
  if (rollUpCost_)
    delete rollUpCost_;
  rollUpCost_ = cost;
}

const CascadesPlan * CascadesPlan::getSolutionForChild (Lng32 childIndex) const
{
  return (IFX getContextForChild(childIndex)
	  THENX childContexts_[childIndex]->getSolution()
	  ELSEX NULL);
}

const PhysicalProperty * CascadesPlan::getPhysicalPropertyForChild (Lng32 childIndex) const
{
   return (IFX getSolutionForChild(childIndex)
	   THENX getSolutionForChild(childIndex)->getPhysicalProperty()
	   ELSEX NULL);
}
//<pb>
const Cost * CascadesPlan::getCostForChild (Lng32 childIndex) const
{
  return (IFX getSolutionForChild(childIndex)
	  THENX getSolutionForChild(childIndex)->getRollUpCost()
	  ELSEX NULL);
}

NABoolean CascadesPlan::exprOccursInChildTree(RelExpr *newExpr,
                                              Int32 maxDepth) const
{
  if (maxDepth <=0)
    return FALSE;
  // walk the plan tree and check for occurrences of newExpr in the
  // solutions of the child contexts, up to a depth of "maxDepth"
  // groups
  if (newExpr)
    {
      Int32 arity = newExpr->getArity();
      for (Int32 i=0; i < arity; i++)
        {
          Context *cc = getContextForChild(i);
          if (cc AND cc->getSolution())
            {
              // Is newExpr the top node of the this child's solution?
              // (Note: We don't check this in the recursive call
              //  because we don't want to do this check for the
              //  topmost context - it's ok to replace the solution
              //  of the topmost context with another one that uses
              //  the same expression)
              if (cc->getSolution()->getPhysicalExpr() == newExpr)
                return TRUE;

              // Recursively check the children, up to the maximum
              // depth or as long as the child's group is the same as
              // our group (compare groups instead of group ids to
              // detect merged groups). We assume that if the child
              // tree visits the same group again then there won't be
              // more than "maxDepth" intervening groups.  That's
              // certainly true for enforcers. It is also true today
              // for unnecessary logical expressions, such as
              // groupbys.
              NABoolean childIsInSameGroup =
                (*CURRSTMT_OPTGLOBALS->memo)[getContext()->getGroupId()] == (*CURRSTMT_OPTGLOBALS->memo)[cc->getGroupId()];

              if (maxDepth > 0 OR childIsInSameGroup)
                {
                  if (cc->getSolution()->exprOccursInChildTree(
                           newExpr,
                           (childIsInSameGroup ? maxDepth : maxDepth-1)))
                    return TRUE;
                }
            }
        }
    }
  return FALSE;
}

//<pb>
/* ============================================================ */

#ifdef DEBUG
void PlanWorkSpace::resetPwsCount()
{
   CURRSTMT_OPTGLOBALS->planWorkSpaceCount = 0;
}
#endif /* DEBUG */

PlanWorkSpace::PlanWorkSpace(Lng32 numberOfChildren)
    :  childContexts_(CmpCommon::statementHeap()),
       contextCount_(0),
       latestChild_(-1),
       prevPlan_(0),
       latestPlan_(-1),
       latestContext_(NULL),
       planChildCount_(0),
       withinCostLimitFlag_(TRUE),
       operatorCost_(NULL),
       partialPlanCost_(NULL),
       knownChildrenCost_(NULL),
       bestOperatorCostSoFar_(NULL),
       bestRollUpCostSoFar_(NULL),
       bestPlanSoFar_(-1),
       bestPlanSoFarIsBMO_(FALSE),
       bestSynthPhysPropSoFar_(NULL),
       isBMO_(FALSE),
       allChildrenContextsConsidered_(TRUE),
       parallelismIsOK_(TRUE),
       myContext_(NULL)
{
  CMPASSERT(numberOfChildren >= 0);
  for (Lng32 index = 0; index < numberOfChildren; index++)
    childContexts_.insertAt(index,
			    new(CmpCommon::statementHeap())
			    ContextArray(CmpCommon::statementHeap()));

#ifdef DEBUG
  // Increment count of PlanWorkSpace objects created and save that
  // value in this PlanWorkSpace object as a debugging id.
  CURRSTMT_OPTGLOBALS->planWorkSpaceCount++;
  pwsID_ = CURRSTMT_OPTGLOBALS->planWorkSpaceCount;
#endif /* DEBUG */

}
//<pb>
// -----------------------------------------------------------------------
// Destructor for the PlanWorkSpace.
// -----------------------------------------------------------------------
PlanWorkSpace::~PlanWorkSpace()
{
  for (Lng32 index = 0; index < (Lng32)childContexts_.entries(); index++)
    {
      delete childContexts_[index];
    }

  if (operatorCost_ != 0)
    {
      delete operatorCost_;
    }

  if (partialPlanCost_ != 0)
    {
      delete partialPlanCost_;
    }

  if (knownChildrenCost_ != 0)
    {
      delete knownChildrenCost_;
    }

  if (bestRollUpCostSoFar_ != 0)
    {
      delete bestRollUpCostSoFar_;
    }

  if (bestOperatorCostSoFar_ != 0)
    {
      delete bestOperatorCostSoFar_;
    }

}  // PlanWorkSpace::~PlanWorkSpace()

// -----------------------------------------------------------------------
// Store a new child Context in the PlanWorkSpace.
// -----------------------------------------------------------------------
void PlanWorkSpace::storeChildContext(Lng32 childIndex, Lng32 planNumber,
				      Context* childContext)
{
  CMPASSERT((childIndex >= 0) AND
            (childIndex < (Lng32)childContexts_.entries()) AND
	    (planNumber >= (Lng32)childContexts_[childIndex]->entries()) );

  childContexts_[childIndex]->insertAt(planNumber, childContext);
  contextCount_++;
  latestChild_ = childIndex;
  if (latestPlan_ != -1)
    prevPlan_ = latestPlan_;
  latestPlan_ = planNumber;
  latestContext_ = childContext;
  withinCostLimitFlag_ = TRUE;

} // PlanWorkSpace::storeChildContext()

// -----------------------------------------------------------------------
// Get a specific child Context.
// -----------------------------------------------------------------------
Context* PlanWorkSpace::getChildContext(Lng32 childIndex, Lng32 planNumber) const
{
  if (NOT ((childIndex >= 0) AND
           (childIndex < (Lng32)childContexts_.entries()) AND
	   (planNumber >= 0) AND
	   (planNumber < (Lng32)childContexts_[childIndex]->entries())))
    return NULL;
  else
    return childContexts_[childIndex]->at(planNumber);
} // PlanWorkSpace::getChildContext()

// Is this plan's n-th child a scan?
NABoolean PlanWorkSpace::getScanLeaf(int childNumber,
                                     int planNumber,
                                     FileScan *&scanLeaf) const
{
  Context* childContext = getChildContext(childNumber,planNumber);
  scanLeaf = NULL;
  int i = 0;

  while (++i < 20 /*guard against loops*/) {

    const CascadesPlan *plan = 
      childContext ? childContext->getSolution() : NULL;
    RelExpr *child = plan ? plan->getPhysicalExpr() : NULL;

    if (!child)
      return FALSE;

    switch (child->getOperatorType()) {
    case REL_FILE_SCAN:
      // found the scan we were looking for
      scanLeaf = static_cast<FileScan*>(child);
      return TRUE;
    case REL_EXCHANGE:
      // exchange, try its child
      childContext = plan->getContextForChild(0);
      break;
    case REL_NESTED_JOIN:
      // is it a TSJ to an index scan?
      if (static_cast<Join*>(child)->isIndexJoin())
        // get its right leaf scan node
        childContext = plan->getContextForChild(1);
      else
        return FALSE;
      break;
    default:
      return FALSE;
    } // switch
  } // while
  return FALSE;
} // PlanWorkSpace::getRightLeaf()

//<pb>
// -----------------------------------------------------------------------
// Erase the latest context.
// -----------------------------------------------------------------------
void PlanWorkSpace::eraseLatestContextFromWorkSpace()
{
  CMPASSERT((latestChild_ >= 0) AND (latestPlan_ >= 0));
  childContexts_[latestChild_]->insertAt(latestPlan_, NULL);
  latestChild_ = -1;
  prevPlan_ = 0;
  latestPlan_ = -1;
  latestContext_ = NULL;
  withinCostLimitFlag_ = TRUE;
} // PlanWorkSpace::eraseLatestContextFromWorkSpace()

// -----------------------------------------------------------------------
// Delete the specified child Context from the PlanWorkSpace.
// -----------------------------------------------------------------------
void PlanWorkSpace::deleteChildContext(Lng32 childIndex, Lng32 planNumber)
{
  CMPASSERT((childIndex >= 0) AND
            (childIndex < (Lng32)childContexts_.entries()) AND
	    (planNumber >= 0) AND
	    (planNumber < (Lng32)childContexts_[childIndex]->entries()));

  // Get rid of old context entry
  childContexts_[childIndex]->remove(planNumber);
  // get rid of the latest entries that refer to this context too
  latestChild_ = -1;
  prevPlan_ = 0;
  latestPlan_ = -1;
  latestContext_ = NULL;
  // Decrement count of contexts
  contextCount_--;

} // PlanWorkSpace::deleteChildContext()

// -----------------------------------------------------------------------
// Initialize the cost that is stored in the PlanWorkSpace.
// -----------------------------------------------------------------------
void PlanWorkSpace::initializeCost(Cost* operatorCost)
{
  if (operatorCost_)
    delete operatorCost_;
  if (partialPlanCost_)
    delete partialPlanCost_;

  operatorCost_ = operatorCost->duplicate();
  partialPlanCost_      = operatorCost->duplicate();
} // PlanWorkSpace::initializeCost()


// -----------------------------------------------------------------------
// Set the operator cost in this PlanWorkSpace.
// -----------------------------------------------------------------------
void PlanWorkSpace::setOperatorCost(Cost * cost)
{
  if (operatorCost_)
    delete operatorCost_;
  operatorCost_ = cost;
}

// -----------------------------------------------------------------------
// Restore partialPlan cost to the operator cost.
// -----------------------------------------------------------------------
void PlanWorkSpace::setPartialPlanCostToOperatorCost()
{
  *partialPlanCost_ = *operatorCost_;

  if (knownChildrenCost_ != 0)
    {
      delete knownChildrenCost_;
      knownChildrenCost_ = 0;
    }

} // PlanWorkSpace::setPartialPlanCostToOperatorCost()
//<pb>
//==============================================================================
//  Get operator's cost.  Recompute this cost for operators who change
// their mind about whether they are a big memory operator or not,
// for any operator whose degree of parallelism that costing would
// use for this plan is different from what we estimated when we computed
// the preliminary cost or from the previous plan for this operator,
// for Exchange operators, and for Hash Joins where the current plan is
// a type-1 hash join and the previous plan was a type-2 hash join.
// In all other cases, just return the operator cost computed initially.
//
// Input:
//  planNumber -- number of specified plan.
//
// Output:
//  none
//
// Return:
//  Operator's cost independent of its children.
//
//==============================================================================
Cost*
PlanWorkSpace::getFinalOperatorCost(Lng32 planNumber)
{
  CascadesPlan* myPlan = myContext_->getPlan();
  CMPASSERT(myPlan);
  RelExpr* op = myPlan->getPhysicalExpr();
  CMPASSERT(op);
  CostMethod* cm = op->costMethod();
  CMPASSERT(cm);

  // RelRoot is never a BMO and it's degree of parallelism is always 1,
  // so there is no need to recost. Just return the cost computed at
  // preliminary cost time.
  if (op->getOperatorType() == REL_ROOT)
  {
    return operatorCost_->duplicate();
  }

  //---------------------------------------------------------------
  // Determine if we're costing a BMO. Set this in the pws and the
  // plan so costing will know if the current plan is a BMO. Also
  // determine if the operator was not a BMO the last time we
  // computed a cost for this operator but now is, or vice-versa.
  //---------------------------------------------------------------
  NABoolean opChangedMindAboutBeingABMO = FALSE;

  if (op->isBigMemoryOperator(this,planNumber))
  {
    if (isBMO_ == FALSE)
    {
      opChangedMindAboutBeingABMO = TRUE;
      isBMO_ = TRUE;
      myPlan->setBigMemoryOperator(TRUE);
    }
  }
  else
  {
    if (isBMO_ == TRUE)
    {
      opChangedMindAboutBeingABMO = TRUE;
      isBMO_ = FALSE;
      myPlan->setBigMemoryOperator(FALSE);
    }
  }

  //---------------------------------------------------------------------------
  //  Set some other things up for the cost to be recomputed. These include the
  // plan's pointers to the child contexts and the plan's physical properties.
  //---------------------------------------------------------------------------
  Lng32 childIndex = 0;
  for(; childIndex < op->getArity(); childIndex++)
  {
    //-----------------------------------
    //  Set up child contexts in my plan.
    //-----------------------------------
    Context* childContext = getChildContext(childIndex,planNumber);
    if (childContext != NULL) {
      myPlan->setContextForChild(childIndex,childContext);

      //------------------------------------------------------------
      //  All children should have a well-formed plan at this point.
      //------------------------------------------------------------
      const CascadesPlan* childPlan = childContext->getSolution();
      CMPASSERT(childPlan);
    }
  }
  //-----------------------------------------------
  //  Synthesize physical properties for this plan.
  //-----------------------------------------------
  PhysicalProperty* sppForMe = op->synthPhysicalProperty(myContext_,planNumber,this);
  myPlan->setPhysicalProperty(sppForMe);
  PartitioningFunction* synthPartFunc = sppForMe->getPartitioningFunction();
  Lng32 synthNumOfParts = synthPartFunc->getCountOfPartitions();


  // Get the number of probes
  CostScalar numOfProbes =
    myContext_->getInputLogProp()->getResultCardinality();

  // Determine the actual degree of parallelism that costing would
  // use if we were to recost.
  Lng32 actualDegreeOfParallelismForCosting;
  // If the synthesized partitioning function is replicateNoBroadcast,
  // and the number of probes is less than the number of partitions,
  // then the actual degree of parallelism used for costing will be
  // the number of probes.
  if (synthPartFunc->isAReplicateNoBroadcastPartitioningFunction() AND
      (numOfProbes < synthNumOfParts) AND
      (numOfProbes == getCountOfStreams()))
    actualDegreeOfParallelismForCosting = (Lng32)numOfProbes.value();
  else
    actualDegreeOfParallelismForCosting = synthNumOfParts;

  Cost* operatorCost;

  //------------------------------------------------------------------------
  //  Recost if necessary. We must always recost an exchange operator because
  // the cost that is computed at preliminary cost time and final cost time
  // are drastically different, and the cost for the DP2 exchange plan of
  // the exchange (plan 0) is drastically different from the cost for the
  // ESP exchange plan (plan 1). Note that for hash join, plan 0 is always a
  // type-2 hash join and plan 1 is always a type-1 hash join, if parallelism
  // is allowed.  If parallelism is not allowed, then the only plan generated
  // is plan0 with the RequireExactlyOnePartition requirement.
  //------------------------------------------------------------------------
//  if (opChangedMindAboutBeingABMO OR
//      (op->getOperatorType() == REL_EXCHANGE) OR
//      (op->getOperatorType() == REL_MERGE_JOIN) OR
//      (op->getOperator().match(REL_FORCE_HASH_JOIN) AND
//       (planNumber == 1)) //current plan is type-1, previous was type-2
//     )
  {
    //----------------------------------------------------------------------
    //  Recompute operator cost. Save the new operator cost and number of
    // streams used for costing in the pws.  Note that the number of streams
    // used for costing will not be equal to the number of partitions from the
    // synthesized partitioning function if the partitioning function was
    // replicateNoBroadcast and the number of probes was less than then the
    // number of partitions, or if the number of active partitions was less
    // than the number of partitions.
    // Also, save the new operator cost.
    //-----------------------------------------------------------------------
    Lng32 countOfStreams;
    if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
    {
      operatorCost = cm->scmComputeOperatorCost(op,this,countOfStreams);
    }
    else
    {
      operatorCost = cm->computeOperatorCost(op,myContext_,countOfStreams);
    }
    initializeCost(operatorCost); // store new cost in the pws
    setCountOfStreams(countOfStreams); //store new # of streams used for costing
  }
//  else
//  {
    //-------------------------------------------------------------------------
    //  Operator cost is correct, so no need to recompute it.
    //-------------------------------------------------------------------------
//    operatorCost = operatorCost_->duplicate();
//  }

  //-------------------------------------------------------------------------
  // Reset the child context pointers. We probably don't need to do this,
  // but it does not hurt, so we will do it here, just to be safe.
  //-------------------------------------------------------------------------
  for(childIndex = 0; childIndex < op->getArity(); childIndex++)
  {
    myPlan->setContextForChild(childIndex,NULL);
  }

  return operatorCost;

} // PlanWorkSpace::getFinalOperatorCost()
//<pb>
//==============================================================================
//  Set partialPlan cost to a newly specified cost.
//
// Input:
//  newCost  -- newly specified cost.
//
// Output:
//  none
//
// Return:
//  none
//
//==============================================================================
void
PlanWorkSpace::setPartialPlanCost(Cost* newCost)
{
  if (partialPlanCost_ != 0)
    {
      delete partialPlanCost_;
    }

  partialPlanCost_ = newCost;

} // PlanWorkSpace::setPartialPlanCost()

//<pb>
//==============================================================================
//  Set known children cost to a newly specified cost.
//
// Input:
//  newCost  -- newly specified cost.
//
// Output:
//  none
//
// Return:
//  none
//
//==============================================================================
void
PlanWorkSpace::setKnownChildrenCost(Cost* newCost)
{
  if (knownChildrenCost_ != 0)
    {
      delete knownChildrenCost_;
    }

  knownChildrenCost_ = newCost;

} // PlanWorkSpace::setKnownChildrenCost()

//<pb>
//==============================================================================
//  If specified cost is lower than best cost so far, make specified cost the
// new best cost.  If the cost is not the best cost, it is deleted.
//
// Input:
//  cost       -- pointer to specified cost.
//  planNumber -- plan associated with specified cost.
//
// Output:
//  none
//
// Return:
//  none
//
//==============================================================================
void
PlanWorkSpace::updateBestCost(Cost* cost, Lng32 planNumber)
{

  //------------------------------------------------------------------------
  //  If no cost is specified, we have nothing to do, so return immediately.
  //------------------------------------------------------------------------
  if (cost == NULL)
    {
      return;
    }

  const ReqdPhysicalProperty* const rpp =
                                    myContext_->getReqdPhysicalProperty();
  const CascadesPlan* myPlan = myContext_->getPlan();

  //----------------------------------------------------------
  //  See if we have a well formed plan with which to compare.
  //----------------------------------------------------------
  if(bestRollUpCostSoFar_ == NULL)
    {

      //-------------------------------------
      //  First well formed plan encountered,
      // so set it to best cost so far.
      //-------------------------------------
      bestRollUpCostSoFar_    = cost;
      bestOperatorCostSoFar_  = operatorCost_->duplicate();
      bestPlanSoFar_          = planNumber;
      bestPlanSoFarIsBMO_     = isBMO_;
      bestSynthPhysPropSoFar_ = myPlan->getPhysicalProperty();

    }
  else
    {

      //--------------------------------------------------------------------
      //  See if specified cost is cheaper than best plan considered so far.
      //--------------------------------------------------------------------
      if(cost->compareCosts(*bestRollUpCostSoFar_,rpp) == LESS)
        {
          delete bestRollUpCostSoFar_;
          delete bestOperatorCostSoFar_;
          if (bestSynthPhysPropSoFar_ != NULL)
            delete bestSynthPhysPropSoFar_;
          bestRollUpCostSoFar_          = cost;
          bestOperatorCostSoFar_        = operatorCost_->duplicate();
          bestPlanSoFar_          = planNumber;
          bestPlanSoFarIsBMO_     = isBMO_;
          bestSynthPhysPropSoFar_ = myPlan->getPhysicalProperty();
        }
      else
        {
          delete cost;
        }
     }

} // PlanWorkSpace::updateBestCost()

//<pb>
//==============================================================================
//  Determine cost of the best plan.  Store this cost with the
// associated plan.
//
// Input:
//  None.
//
// Output:
//  planNo -- The best plan so far.
//
// Return:
//  TRUE if optimal solution was found; FALSE otherwise
//
//==============================================================================
NABoolean
PlanWorkSpace::findOptimalSolution(Lng32& planNo)
{

  CascadesPlan* myPlan  = myContext_->getPlan();
  RelExpr*      op      = myPlan->getPhysicalExpr();
  Lng32          opArity = op->getArity();

  //--------------------------------------------------
  //  Assume no optimal plan exists until we find one.
  //--------------------------------------------------
  Cost* bestRollUpCost   = NULL;
  Cost* bestOperatorCost = NULL;

  //---------------------------------------------------------------
  // Choose the best plan if one exists.
  //
  //  Note, a best plan may not exist if the user tried to force an
  // impossible plan or if the parent required physical properties
  // were not compatible with the required properties of this
  // operator.
  //
  //  Note also that we make duplicates of the best plan's cost
  // objects because the originally allocated cost objects are
  // deallocated in the plan workspace destructor.
  //---------------------------------------------------------------
  if (bestRollUpCostSoFar_ != NULL)
  {
    bestRollUpCost   = bestRollUpCostSoFar_->duplicate();
    bestOperatorCost = bestOperatorCostSoFar_->duplicate();
  }
  else // No qualifying plan exists.
  {
    planNo = -1;
    return FALSE;
  }

  // Save the synthesized physical properties of the best plan in the
  // spp slot of this plan.
  myPlan->setPhysicalProperty(bestSynthPhysPropSoFar_);

  //-------------------------------------------------------------------------
  //  Store best cost and associated big memory operator values in operator's
  // plan.
  //-------------------------------------------------------------------------
  myPlan->setOperatorCost(bestOperatorCost);
  myPlan->setRollUpCost(bestRollUpCost);
  myPlan->setBigMemoryOperator(bestPlanSoFarIsBMO_);
  for(Lng32 i = 0; i < opArity; i++)
  {
    myPlan->setContextForChild(i,getChildContext(i,bestPlanSoFar_));
  }

  planNo = bestPlanSoFar_;

  // Pruning heristic allows to use failed plan cost but needs to
  // set a flag to pass to context if plan exceeds cost limit.

  if ( CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() )
  {
    CostLimit*  costLimit = myContext_->getCostLimit();
    const ReqdPhysicalProperty* const rpp =
		myContext_->getReqdPhysicalProperty();
    if( costLimit AND
        costLimit->compareWithCost(*bestRollUpCost,rpp) == LESS )
      myPlan->setExceededCostLimit();
  }

  return TRUE;

} // PlanWorkSpace::findOptimalSolution()

//<pb>
//==============================================================================
//  Determine cost of a specified plan.  If no such plan exists, or if at least
// one child has no optimal solution, return null.  Also if plan's cost exceeds
// the cost limit for the plan workspace, return null.
//
// Input:
//  planNumber -- number of specified plan.
//
// Output:
//  none
//
// Return:
//  Cost of specified plan
//  NULL if cost can't be calculated or exceeds the cost limit.
//
//==============================================================================
Cost*
PlanWorkSpace::getCostOfPlan(Lng32 planNumber)
{

  RelExpr*      op      = myContext_->getPlan()->getPhysicalExpr();
  Lng32          opArity = op->getArity();
  Cost*         cost    = NULL;

  //----------------------------------------------------------------------
  //  For leaf operators, there is only one implicit plan, and its cost is
  // simply the operator's preliminary cost, so ignore the specified plan
  // number and simply return the Operator cost.
  //----------------------------------------------------------------------
  if (opArity == 0)
  {
    cost = operatorCost_->duplicate();
    cost->computePlanPriority(op, myContext_);
  }
  else
  {
    //-------------------------------------------
    //  No plan specified, so return immediately.
    //-------------------------------------------
    if (planNumber < 0)
      return NULL;

    //-------------------------------------------------------------------
    // Verify that each child for specified plan has an optimal solution.
    //-------------------------------------------------------------------
    for (Lng32 childIndex = 0; childIndex < opArity; childIndex++)
    {
      Context* childContext = getChildContext(childIndex,planNumber);

      if(NOT childContext)
        return NULL;

      if ( NOT childContext->hasOptimalSolution() )
        return NULL;
    }

    //----------------------------------
    //  Compute cost for specified plan.
    //----------------------------------
    CostMethod* cm = op->costMethod();
    if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
      cost = cm->scmComputePlanCost(op,this,planNumber);
    else
      cost = cm->computePlanCost(op,myContext_,this,planNumber);

    //----------------------------------
    //  Compute Priority for the plan
    //----------------------------------

    if (opArity == 1)
    {
      Cost * childCost;
      cm->getChildCostForUnaryOp(op,
                                myContext_,
                                this,
                                planNumber,
                                childCost);

      cost->computePlanPriority(op, myContext_, childCost);
      delete childCost;
    }

    if (opArity == 2)
    {
      Cost * leftChildCost;
      Cost * rightChildCost;
      cm->getChildCostsForBinaryOp(op,
                                  myContext_,
                                  this,
                                  planNumber,
                                  leftChildCost,
                                  rightChildCost);

      cost->computePlanPriority(op, myContext_, leftChildCost, rightChildCost,
                                this, planNumber);
      if (leftChildCost)
        delete leftChildCost;
      if (rightChildCost)
        delete rightChildCost;
    }

  }

  //-----------------------------------------
  //  Ensure cost does not exceed cost limit.
  //-----------------------------------------
  CostLimit*  costLimit = myContext_->getCostLimit();
  if( NOT CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost() AND costLimit AND
      costLimit->compareWithCost(*cost,
                   myContext_->getReqdPhysicalProperty()) == LESS )
  {
    DBGLOGMSGCNTXT(" *** CL exceeded in getCostOfPlan() ",myContext_);
    // when pruning is OFF or failed plan cost is not used
    // we would remove the cost object and return NULL loosing
    // all the information about this plan. Then later we might
    // need to cost this plan again. When OPH_USE_FAILED_PLAN_COST
    // is ON we try to reuse this cost and use later.
    delete cost;
    return NULL;
  }

  return cost;

} // PlanWorkSpace::getCostOfPlan()

//<pb>
/* ============================================================ */

void NestedJoinPlanWorkSpace::transferParallelismReqsToRG(RequirementGenerator &rg)
{
  Lng32     tempChildNumPartsRequirement      = childNumPartsRequirement_;
  float     tempChildNumPartsAllowedDeviation = childNumPartsAllowedDeviation_;

  if (NOT numOfESPsForced_)
    rg.makeNumOfPartsFeasible(tempChildNumPartsRequirement,
                              &tempChildNumPartsAllowedDeviation);
  rg.addNumOfPartitions(tempChildNumPartsRequirement,
                        tempChildNumPartsAllowedDeviation);

}

//<pb>
/* ============================================================ */

/*
  List of un-done tasks
  =====================
  */

CascadesTaskList::CascadesTaskList()
{
  first_ = NULL;
} // CascadesTaskList::CascadesTaskList

CascadesTaskList::~CascadesTaskList()
{
  while(NOT empty())
    delete pop();
} // CascadesTaskList::~CascadesTaskList

void CascadesTaskList::print(FILE * f, const char * prefix, const char * suffix) const
{
#ifdef _DEBUG
  if (first_ == NULL) // task list is empty
    fprintf(f, "%s%s OPEN is empty %s\n%s",
	     prefix, LINE_STRING, LINE_STRING, suffix);

  else // task list with some tasks in it
    {
      fprintf(f, "%s%s OPEN %s\n", prefix, LINE_STRING, LINE_STRING);

      // loop over all tasks to be done
      Int32 count = 0;
      for (CascadesTask * task = first_;  task != NULL;  task = task->next_)
	fprintf(f, "%d -- ", ++ count), task->print(f, "", "");

      fprintf(f, "%s OPEN %s (end)\n%s", LINE_STRING, LINE_STRING,
	       suffix);
    } // task list with some tasks in it
#endif  // _DEBUG
} // CascadesTaskList::print
//<pb>
CascadesTask * CascadesTaskList::pop()
{
  if (empty()) return( NULL );
  CascadesTask * task = first_;
  first_ = task->next_;
  return( task );
} // CascadesTaskList::pop

// Insert the new task such that it will be performed just before the provided
// beforeTask.  If beforeTask is empty, then just add this task to the end of the
// list.
void CascadesTaskList::insertTask (CascadesTask * newTask, CascadesTask * beforeTask)
{
  CascadesTask * t = first_;
  while (t != NULL)
  {
    if (t->next_ == beforeTask)
    {
      t->next_ = newTask;
      newTask->next_ = beforeTask;
      return;
    }
    t = t->next_;
  }
  if (beforeTask == NULL)
    push (newTask);
  else
    cout << " Internal error in insertTask " << endl;

}

// Insert the new task such that it will be performed just before the provided
// beforeTask.  If beforeTask is empty, then just add this task to the end of the
// list.
void CascadesTaskList::insertOptimizeExprTask (CascadesTask * newTask, CascadesTask * beforeTask)
{
  CascadesTask * t = first_;
  Context * myContext = newTask->getContext();

  while (t != NULL)
  {
    Context * nextContext = NULL;

    if (t->next_)
      nextContext = t->next_->getContext();

    // We find the beforeTask
    // OR
    // next task has:
    // * same context
    // * is an OET
    // * has higher potential
    // then insert before it.
    if ((t->next_ == beforeTask) ||
        ((myContext == nextContext) &&
         (t->next_) &&
         (t->next_->taskType() == CascadesTask::OPTIMIZE_EXPR_TASK)&&
         (t->next_->getExpr()) &&
         (t->next_->getExpr()->getPotential() > newTask->getExpr()->getPotential())))
    {
      CascadesTask * nextTask = t->next_;
      t->next_ = newTask;
      newTask->next_ = nextTask;
      return;
    }
    t = t->next_;
  }
  if (beforeTask == NULL)
    push (newTask);
  else
    cout << " Internal error in insertTask " << endl;

}

//<pb>
/* ============================================================ */

CascadesBinding::CascadesBinding(CascadesGroupId group_no,
				 RelExpr * pattern,
				 CascadesBinding * parent,
				 NABoolean incl_log,
				 NABoolean incl_phys):
				 children_(CmpCommon::statementHeap())
{
  state_ = START_GROUP;
  this->groupId_ = group_no;
  curExpr_ = NULL;
  copiedExpr_ = NULL;
  fixed_ = FALSE;              // try all matches within this group

  pattern_ = pattern;
  parent_ = parent;
  inclLog_ = incl_log;
  inclPhys_ = incl_phys;

} // CascadesBinding::CascadesBinding
//<pb>
CascadesBinding::CascadesBinding(RelExpr * expr,
				 RelExpr * pattern,
				 CascadesBinding * parent,
				 NABoolean incl_log,
                                 NABoolean incl_phys):
                                 children_(CmpCommon::statementHeap())
{
  state_ = START_EXPR;

  groupId_ = expr->getGroupId();

  curExpr_ = expr;
  copiedExpr_ = NULL;
  fixed_ = TRUE;               // restricted to this log expr

  pattern_ = pattern;
  parent_ = parent;
  inclLog_ = incl_log;
  inclPhys_ = incl_phys;

} // CascadesBinding::CascadesBinding

CascadesBinding::~CascadesBinding()
{

  // if an expression exists, delete it and its children
  if (state_ == VALID_BINDING  OR  state_ == ALMOST_EXHAUSTED)
    {
      CascadesBinding *childBinding;
      for (Lng32 childIndex = (Int32)(children_.entries());  -- childIndex >= 0;  )
	{
	  childBinding = children_[childIndex];
	  delete childBinding;
	}

      if (copiedExpr_ != NULL)
	delete copiedExpr_;

    } // if an expression exists, delete it and its childs

} // CascadesBinding::~CascadesBinding
//<pb>
void CascadesBinding::print(FILE * f, const char * prefix, const char * suffix) const
{
#ifdef _DEBUG
  fprintf(f, "%sBinding state \"%s\", Group %d\n",
	  prefix, binding_state_name(state_), groupId_);

  if (curExpr_ == NULL)
    fprintf(f, "%s -- no expression bound\n", prefix);
  else // expression
    {
      fprintf(f, "%s -- operator ", prefix);
      curExpr_->print(f,CONCAT(prefix,"curr expr: "));
    } // expression
  fprintf(f, "%s", suffix);
#endif  // _DEBUG
} // CascadesBinding::print
//<pb>
// -----------------------------------------------------------------------
// Materialize a binding inside the CascadesMemo
// structure (just like Volcano does) and gives that copy
// out to the user. The cut nodes of the extracted expression
// are shared with the cut nodes of the pattern, from where they
// are available for CascadesMemo::include. The caller of extract_expr
// has to call release_expr().
// -----------------------------------------------------------------------

RelExpr * CascadesBinding::extract_expr()
{

  RelExpr * result;

  // ensure that binding is in a suitable state
  if (state_ != VALID_BINDING  AND  state_ != ALMOST_EXHAUSTED)
    ABORT("no binding available");

  if (pattern_ != NULL  AND  pattern_->isCutOp())
    {
      // extracting a node that matches a cut node in the pattern:
      // in this case, return the cut node itself, marked with group index

      CutOp * acut = (CutOp *) pattern_;

      // bind the cut node to the group of this binding
      acut->setGroupIdAndAttr(groupId_);

      // if this binding is fixed to a particular expression, bind the
      // cut operator to that expression instead of binding it to a group
      if (fixed_)
	acut->setExpr(curExpr_);

      result = pattern_;

    } // create cut marked with group index

  else // general invocation of extract_expr
    {
      const Int32 arity = curExpr_->getArity();

      result = curExpr_;

      // extract child expressions
      for (Lng32 childIndex = 0;  childIndex < arity;  ++ childIndex)
	{
	  RelExpr *newChild = children_[childIndex]->extract_expr();

	  // Check for common subexpressions: if the children of this
	  // expression are already bound, then either someone forgot
	  // to call release_expr() or this expression occurs more than
	  // once in the binding. We assume the latter case and return
	  // a copy of the original node.
	  if (curExpr_->child(childIndex).getMode() == ExprGroupId::BINDING)
	    {
	      // Use the fact that extract_expr doesn't look at the
	      // node's children. Mark the binding that it uses a copy
	      // and not an expression residing in CascadesMemo
	      copiedExpr_ = result =
		curExpr_->copyTopNode(0, CmpCommon::statementHeap());
	      copiedExpr_->setGroupAttr(curExpr_->getGroupAttr());

	      // this should not happen after the first child
	      CMPASSERT(childIndex == 0);
	    }

	  // let the child of curExpr_ point to an actual expression
	  result->child(childIndex) = newChild;
	}

      // bind wildcards to the actually matching expression
      if (pattern_ != NULL AND pattern_->isWildcard())
	{
	  ((WildCardOp *) pattern_)->setCorrespondingNode(curExpr_);
	}
    } // general invocation of extract_expr

  // pass along groupId_'s isinCS attribute to result
  RelExpr *logExpr_ = (* CURRSTMT_OPTGLOBALS->memo) [groupId_]->getFirstLogExpr();
  if (logExpr_)
    result->setBlockStmt(logExpr_->isinBlockStmt());

  return( result );
} // CascadesBinding::extract_expr
//<pb>

// release the expression that was given
// to the caller of CascadesBinding::extract_expr()
void CascadesBinding::release_expr()
{

  // ensure that binding is in a suitable state
  if (state_ != VALID_BINDING  AND  state_ != ALMOST_EXHAUSTED)
    ABORT("trying to release a nonexistent binding");

  if (pattern_ != NULL  AND  pattern_->isCutOp())
    {
      CutOp * acut = (CutOp *) pattern_;

      acut->setGroupIdAndAttr(INVALID_GROUP_ID);

    } // create cut marked with group index

  else // non-cut node
    {
      Int32 arity = curExpr_->getArity();

      // release child expressions
      for (Lng32 childIndex = 0; childIndex < arity; ++childIndex)
	{
	  // release the child binding
	  children_[childIndex]->release_expr();

	  if (copiedExpr_ == NULL)
	    // switch the pointer back to a group number
	    (*curExpr_)[childIndex].releaseBinding();
	  else
	    // reset the pointer, the prior call to
	    // release_expr() has deleted the child
	    copiedExpr_->child(childIndex) = (RelExpr *)NULL;
	}

      // if this binding was using a copied expression, delete it
      if (copiedExpr_ != NULL)
	{
	  delete copiedExpr_;
	  copiedExpr_ = NULL;
	}

    } // non-cut node

}
//<pb>
/*
  Function CascadesBinding::advance() walks the many trees embedded in the
  CascadesMemo structure in order to find possible bindings.  Bindings may be
  restricted to conform to a pattern or to include logical or
  physical expressions only.  Each invocation produces one new
  binding.  There are three initial cases:
  (1) The binding object has just been created for a group.
  ==> Starting with the first logical expression in the group,
  search for matches
  (2) The binding object has just been created for a specific
  logical expression.
  ==> bind that expression's children
  (3) A previous complete binding exists
  ==> (a) advance child bindings in right-to-left order
  ==> if (a) fails, delete the previous binding and go on
  to the next logical expression within the group
  Advancing a binding is modelled by a finite state machine for
  each node in an expression tree.
  */
NABoolean CascadesBinding::advance()
{
  CascadesBinding *childBinding;

  // loop until either failure or success
  for (;;)
    {
      // to cache some function results
      Int32 arity = 0, childIndex;

      // state analysis and transitions
      switch (state_)
        {
        case START_GROUP :

	  // look for cycles in the binding (does an ancestor
	  // refer to the same group as this one)? A cycle in a binding
	  // without a pattern would cause an infinite number of
	  // matches!!!!
	  if (pattern_ == NULL)
	    {
	      CascadesBinding *ancestor = parent_;

	      while (ancestor != NULL)
		{
		  if (ancestor->groupId_ == groupId_)
		    {
		      state_ = FINISHED;
		      break; // exit the while loop
		    }
		  ancestor = ancestor->getParent();
		}
	      if (state_ != START_GROUP)
		break; // something happened
	    }

	  // otherwise, advance to the group's first expr
	  curExpr_ = (* CURRSTMT_OPTGLOBALS->memo) [groupId_]->getFirstLogExpr();
	  if (curExpr_ == NULL)
	    {
	      // a group with no logical expressions in it does match
	      // a cut operator, but no other patterns
	      if (pattern_ != NULL  AND pattern_->isCutOp() OR
		  inclPhys_)
		{
		  curExpr_ = (* CURRSTMT_OPTGLOBALS->memo) [groupId_]->getFirstPhysExpr();
		}
	      else
		{
		  state_ = FINISHED;
		  break;
		}
	    }
	  state_ = START_EXPR;

	  // fall through to "case start_expr"

        case START_EXPR :

	  // short-circuit for leaves: succeed exactly once
	  if (pattern_ != NULL  AND
	      pattern_->isCutOp())
            {
	      state_ = ALMOST_EXHAUSTED; // success now, but ...
	      return( TRUE );            // ... failure next time
            } // short-circuit for leaves: success exactly once

	  // cache some function results
	  arity = curExpr_->getArity();

	  // is this expression unusable?
	  if ( NOT ( // first, verify suitable log/phys property
		     (inclLog_  AND  curExpr_->isLogical())  OR
		     (inclPhys_  AND  curExpr_->isPhysical()) )
	       OR  // second, verify pattern conformance
	       // not usable if there is a pattern that
	       // isn't a SubtreeOp and doesn't match
	       (pattern_ != NULL  AND
		NOT pattern_->isSubtreeOp()  AND
		NOT curExpr_->patternMatch(*pattern_)))
            {
	      state_ = EXPR_FINISHED;  // try next expression
	      break;
            } // is this expression unusable?

	  // try to create bindings for the children
	  for (childIndex = 0; childIndex < arity;  childIndex++)
	    {
	      CascadesGroupId childGroupId = (*curExpr_)[childIndex].getGroupId();
	      CascadesBinding *childBinding;

	      if (childGroupId != INVALID_GROUP_ID)
		{
                  // create a new binding for the child group

                  if (pattern_)
                    {
		      childBinding = new(CmpCommon::statementHeap())
		        CascadesBinding(
				      (*curExpr_)[childIndex].getGroupId(),
				      IFX pattern_ == NULL OR pattern_->isSubtreeOp()
				      THENX NULL
				      ELSEX (*pattern_)[childIndex].getPtr(),
				      this,
				      inclLog_,
				      inclPhys_);
                    }
                  else
                    {
                      // if pattern_ is NULL this mean this is a rule that
                      // is not using the standard binding substitutes mechanism
                      // and rely on its own. For such rule the curExpr_ is effectively
                      // the pattern. Note this will work only for 1 level expr, usually
                      // a multijoin. This is fine for R2.0 large scope rules because
                      // the object of the rule is a single MultiJoin expression.
                      // This is however a limitation that will have to be addressed
                      // if we want to do multi-level large MultiJoin Rules; for example
                      // Group by on top of MultiJoin.
                      // For post R2.0 we will add a special cut operator to handle
                      // expressions with variable num of children like MultiJoins
                      // or preferrably to handle any expression that does not want to
                      // traverse its children for binding. (Note regular cuts can't be
                      // used for that since they are used to represent cascades group).

                      CutOp* newPattern = new (CmpCommon::statementHeap())
                        CutOp((*curExpr_)[childIndex].getGroupId(), CmpCommon::statementHeap());
                      newPattern->setGroupIdAndAttr((*curExpr_)[childIndex].getGroupId());
		      childBinding = new(CmpCommon::statementHeap())
		        CascadesBinding(
				      (*curExpr_)[childIndex].getGroupId(),
				      newPattern,
				      this,
				      inclLog_,
				      inclPhys_);

                    }
                }
	      else
		{
		  // create a new binding for the fixed child expression
		  // if the child of this expression isn't part of CascadesMemo
		  childBinding = new(CmpCommon::statementHeap()) CascadesBinding(
		       (*curExpr_)[childIndex].getPtr(),
		       IFX pattern_ == NULL OR pattern_->isSubtreeOp()
		         THENX NULL
		         ELSEX (*pattern_)[childIndex].getPtr(),
		       this,
		       inclLog_,
		       inclPhys_);
		}

	      children_.insertAt(childIndex,childBinding);
	    }

	  // try to bind the children; a failure is failure for the expr
	  for (childIndex = 0;  childIndex < arity;  ++ childIndex)
	    if (NOT children_[childIndex]->advance())
	      break; // terminate this loop

	  // check whether all children found a binding
	  if (childIndex == arity) // successful!
            {

	      // ensure proper bindings for SubtreeOp
	      if (pattern_ != NULL  AND
		  pattern_->isSubtreeOp() AND
		  NOT ((SubtreeOp *) pattern_)->all_bindings())
		{
		  // tree with only one binding, next advance() will fail
		  state_ = ALMOST_EXHAUSTED;
		}
	      else
		{
		  // allow multiple bindings
		  state_ = VALID_BINDING;
		}

	      return( TRUE );
            } // successful bindings for new expression

	  // otherwise, failure!

	  state_ = EXPR_FINISHED;
	  break;

        case VALID_BINDING :

	  // cache some function results
	  arity = curExpr_->getArity();

	  // try existing children in right-to-left order
	  // first success is overall success
	  for (childIndex = arity;  -- childIndex >= 0;  )
            {
	      if(children_[childIndex]->advance())
		// found one more binding
                {
		  // reset all siblings to the right
		  for (Lng32 other_childIndex = childIndex;
		       ++ other_childIndex < arity;  )
                    {
		      // this is very inefficient, but left as is --
		      // matters only for extremely complex patterns
		      childBinding = children_[other_childIndex];
		      delete childBinding;
		      children_[other_childIndex] =
			new(CmpCommon::statementHeap())
			  CascadesBinding((*curExpr_)[other_childIndex].getGroupId(),
				    IFX pattern_ == NULL OR
				    pattern_->isSubtreeOp()
				    THENX NULL
				    ELSEX (*pattern_)[other_childIndex].getPtr(),
				    this,
				    inclLog_,
				    inclPhys_);
		      if (NOT children_[other_childIndex]->advance())
			ABORT("internal error"); // earlier bindings!
		    } // reset all siblings to the right

		  // return overall success
		  state_ = VALID_BINDING;
		  return( TRUE );
                } // found one more binding
            } // try existing children in right-to-left order

	  state_ = EXPR_FINISHED;

	  // fall through to "case expr_finished"

        case EXPR_FINISHED :

	  if (children_.entries() > 0)
	    {
	      // existing log expr is finished; dealloc children
	      for (Lng32 childIndex = children_.entries(); -- childIndex >= 0; )
		{
		  childBinding = children_[childIndex];
		  delete childBinding;
		}
	      children_.clear();
	    }

	  // if possible, move to the next expression
	  if (NOT fixed_  AND
	      (curExpr_ = curExpr_->getNextInGroup()) != NULL)
            {
	      state_ = START_EXPR;
	      break;
            } // if possible, move to the next expression

	  // otherwise, fall through to "case almost_exhausted"

        case ALMOST_EXHAUSTED :

	  // if required, delete child bindings
	  if (children_.entries() > 0)
            {
	      if (curExpr_ != NULL) arity = curExpr_->getArity();
	      for (childIndex = 0; childIndex < arity;  childIndex++ )
		{
		  childBinding = children_[childIndex];
		  delete childBinding;
		}
	      children_.clear();
            } // if required, delete child bindings

	  // new state
	  state_ = FINISHED;

	  // fall through to "case finished"

        case FINISHED :

	  return( FALSE );

        default:
	  ABORT("internal error");

        } // state analysis and transitions
    } // loop until either failure or success

  ABORT("should never terminate this loop");
  return( FALSE );
} // CascadesBinding::advance

OptDefaults::OptDefaults(CollHeap* h) : heap_(h)
{
   // initialization  of members of OptDefaults
   // will be re-initialized by OptDefaults::initialize(..)
   optLevel_ = OptDefaults::MEDIUM;
   indexEliminationLevel_ = OptDefaults::MAXIMUM;
   mdamSelectionDefault_ = 0.5;
   readAheadMaxBlocks_ =16.0;
   acceptableInputEstLogPropError_ = 0.5;
   taskCount_ = 0;
   optTaskLimit_ = INT_MAX;
   enumPotentialThreshold_ = INT_MAX;
   level1Constant1_ = 100;
   level1Constant2_ = 100;
   level1ImmunityLimit_ = 5000;
   level1MJEnumLimit_ = 20;
   level1Threshold_ = 180;
   level1SafetyNet_ = 30000;
   level1SafetyNetMultiple_ = 3.0;
   optInitialMemory_= 0;
   originalOptimizationBudget_ = -1;
   optimizationBudget_ = -1;
   maxDepthToCheckForCyclicPlan_ = 1;
   memUsageTaskThreshold_ = 30000;
   memUsageTaskInterval_ = 1000;
   memUsageSafetyNet_ = 500;	     // 500 MB
   memUsageOptPassFactor_ = 1.5;    // i.e. 1.5*memUsageSafetyNet_
   memUsageNiceContextFactor_ = 1;  // i.e. 1*memUsageSafetyNet_
   shortOptPassThreshold_ = 12;
   numOfBlocksPerAccess_ = 0;
   optRulesGuidance_ =0;
   numTables_ = 4;
   queryComplexity_ = 0;
   queryMJComplexity_ = 0;
   isComplexMJQuery_ = FALSE;
   requiredMemoryResourceEstimate_ = 0;
   requiredCpuResourceEstimate_ = 0;
   totalDataAccessCost_ = 0;
   maxOperatorMemoryEstimate_ = 0;
   maxOperatorCpuEstimate_ = 0;
   maxOperatorDataAccessCost_ = 0;
   memoryPerCPU_ = 3E5;
   workPerCPU_ = 1E7;
   adjustedDegreeOfParallelism_ = 1;
   defaultDegreeOfParallelism_ = 1;
   maximumDegreeOfParallelism_ = 1;
   minimumESPParallelism_ = 0;
   totalNumberOfCPUs_ = 2;

   enableJoinToTSJRuleOnPass1_ = FALSE;
   triggersPresent_ = FALSE;

   reduction_to_push_gb_past_tsj_ = 0 ;
   enableCrossProductControl_ = TRUE;
   enableNestedJoinControl_ = TRUE;
   enableZigZagControl_ = FALSE;
   enableMergeJoinControl_ = TRUE;
   enableOrderedHashJoinControl_ = TRUE;
   considerNestedJoin_ = TRUE;
   considerHashJoin_ = TRUE;
   considerOrderedHashJoin_ = TRUE;
   considerHybridHashJoin_ = TRUE;
   considerMergeJoin_ = TRUE;
   considerZigZagTree_ = DF_OFF;
   considerMinMaxOpt_  = TRUE;
   preferredProbingOrderForNJ_ = FALSE;
   orderedWritesForNJ_ = FALSE;
   nestedJoinForCrossProducts_ = FALSE;
   joinOrderByUser_ = FALSE;
   ignoreExchangesInCQS_ = FALSE;
   ignoreSortsInCQS_ = FALSE;
   optimizerHeuristic1_ = FALSE;
   optimizerHeuristic2_ = FALSE;
   optimizerHeuristic3_ = FALSE;
   optimizerHeuristic4_ = FALSE;
   optimizerHeuristic5_ = FALSE;

   attemptESPParallelism_ = DF_SYSTEM;
   maxParallelismIsFeasible_ = FALSE;
   enableOrOptimization_ = TRUE;
   numberOfPartitionsDeviation_ = 0.25;
   updatedBytesPerESP_ = 400000.0;
   numberOfRowsParallelThreshold_ = 5000.0;
   deviationType2JoinsSystem_ = TRUE;
   numOfPartsDeviationType2Joins_ = 0.0;
   parallelHeuristic1_ = TRUE;
   parallelHeuristic2_ = TRUE;
   parallelHeuristic3_ = TRUE;
   parallelHeuristic4_ = TRUE;

   optimizerPruning_ = FALSE;
   OPHpruneWhenCLExceeded_ = FALSE;
   OPHreduceCLfromCandidates_ = FALSE;
   OPHreduceCLfromPass1Solution_ = FALSE;
   OPHreuseFailedPlan_ = FALSE;
   OPHreuseOperatorCost_ = FALSE;
   OPHskipOGTforSharedGCfailedCL_ = FALSE;
   OPHuseCandidatePlans_ = FALSE;
   OPHuseCompCostThreshold_ = FALSE;
   OPHuseConservativeCL_ = FALSE;
   OPHuseEnforcerPlanPromotion_ = FALSE;
   OPHuseFailedPlanCost_ = FALSE;
   OPHuseNiceContext_ = FALSE;
   OPHusePWSflagForContext_ = FALSE;
   OPHuseCachedElapsedTime_ = FALSE;
   OPHexitNJcrContChiLoop_ = FALSE;
   OPHexitMJcrContChiLoop_ = FALSE;
   OPHexitHJcrContChiLoop_ = FALSE;

   dataFlowOptimization_ = TRUE;
   compileTimeMonitor_ = FALSE;
   reuseBasicCost_ = TRUE;
   randomPruningOccured_ = FALSE;
   optimizerGracefulTermination_ = 1;
   pushDownDP2Requested_ = FALSE;
   fakeHardware_ = FALSE;
   additiveResourceCosting_ = FALSE;
   reduceBaseHistograms_ = TRUE;
   reduceIntermediateHistograms_ = TRUE;

   joinCardLowBound_ = 0.5;
   ustatAutomation_ = FALSE;
   preFetchHistograms_ = TRUE;
   siKeyGCinterval_ = (Int64)24 * 60 * 60; // 24 hours
   histMCStatsNeeded_ = TRUE;
   histSkipMCUecForNonKeyCols_ = TRUE;
   histMissingStatsWarningLevel_ = 4;
   histOptimisticCardOpt_ = 0;
   incorporateSkewInCosting_ = TRUE;
   histAssumeIndependentReduction_ = TRUE;
   histUseSampleForCardEst_ = TRUE;
   maxSkewValuesDetected_ = 10000;
   skewSensitivityThreshold_ = 0.1;
   useHighFreqInfo_ = TRUE; 
   histDefaultSampleSize_ = 10000;
   histTupleFreqValListThreshold_ = 40;
   histNumOfAddDaysToExtrapolate_ = 4;

   defNoStatsUec_ = 2;
   defNoStatsRowCount_ = 100;

   partitioningSchemeSharing_ = 1;
   riskPremiumNJ_ = 1.2;
   riskPremiumMJ_ = 1.2;
   riskPremiumSerial_ = 1.2;
   robustHjToNjFudgeFactor_ = 10.0;
   robustSortGroupBy_ = 1;
   robustQueryOptimization_ = DF_SYSTEM;

   maxMaxCardinality_ = 0;

   defSelForRangePred_ = 0.3333;
   defSelForWildCard_ = 0.20;
   defSelForNoWildCard_ = 0.90;

   baseHistogramReductionFF_ = 0.1;
   intermediateHistogramReductionFF_ = 0.25;
   histogramReductionConstantAlpha_ = 0.5;

   defs_ = &(ActiveSchemaDB()->getDefaults());
   calibratedMSCF_ET_CPU_ = 0.;
   calibratedMSCF_ET_IO_TRANSFER_= 0.;
   calibratedMSCF_ET_NUM_IO_SEEKS_= 0.;
   calibratedMSCF_ET_LOCAL_MSG_TRANSFER_= 0.;
   calibratedMSCF_ET_NUM_LOCAL_MSGS_= 0.;
   calibratedMSCF_ET_REMOTE_MSG_TRANSFER_= 0.;
   calibratedMSCF_ET_NUM_REMOTE_MSGS_= 0.;
   calculatedMEMORY_LIMIT_PER_CPU_ = 0;

   ranSeq_ = NULL;

   useNewMdam_ = TRUE;
   currentTask_ = NULL;
   memoExprCount_ = 0;

   // Query Strategizer Params
   // used for explain
   // BEGIN
   useStrategizer_ = FALSE;
   cpuCost_ = -1;
   scanCost_ = -1;
   budgetFactor_ = -1;
   pass1Tasks_= -1;
   taskFactor_= -1;
   nComplexity_= -1;
   n_2Complexity_= -1;
   n2Complexity_= -1;
   n3Complexity_= -1;
   exhaustiveComplexity_= -1; 
   // END

   requiredESPs_ = -1; 
   requiredScanDescForFastDelete_ = NULL; 

   isSideTreeInsert_ = FALSE; 

   defaultCostWeight_ = NULL;
   defaultPerformanceGoal_ = NULL;
   resourcePerformanceGoal_ = NULL;
}

OptDefaults::~OptDefaults()
{
   CleanupCostVariables();
}

// Static methods of OptDefaults

NABoolean OptDefaults::useNewMdam()
{ return useNewMdam_; }

NABoolean OptDefaults::isRuleDisabled(ULng32 ruleBitPosition)
{
  if(ruleBitPosition >= 32)
    return FALSE;

  ULng32 bitMask = SingleBitArray[ruleBitPosition];

  if(bitMask & optRulesGuidance_)
    return TRUE;

  return FALSE;
}

NABoolean OptDefaults::pruneByOptDefaults(Rule* rule, RelExpr* relExpr)
{
  // Enable nice context if memory usage exceeds predfined threshold.
  if (!OPHuseNiceContext_ &&
      (optLevel_ != OptDefaults::MAXIMUM) &&
      (getMemUsed() >
       ((Lng32)(getMemUsageSafetyNet() *
         getMemUsageNiceContextFactor()))))
  {
    OPHuseNiceContext_ = TRUE;
    OPHuseEnforcerPlanPromotion_ = OPHuseNiceContext_ AND
          (CmpCommon::getDefault(OPH_USE_ENFORCER_PLAN_PROMOTION) == DF_ON);
  }

  // No pruning is allowed in the first optimization pass
  if (GlobalRuleSet->getCurrentPassNumber() == 1)
    return FALSE;

  // Apply task-limit pruning
  if (taskCount_ > optTaskLimit_) return TRUE;

  // Some rule-expr pairs are protected from pruning
  if (NOT rule->canBePruned(relExpr)) return FALSE;

  // Apply optimization level pruning

  // optimization level 0 means stop after pass 1
  // if optimization level is set to 0 we wont actually reach this
  // point. However, this condition is set for completeness
  if (optLevel_ == OptDefaults::MINIMUM)
    return TRUE;

  if (optLevel_ != OptDefaults::MAXIMUM)
    return pruneByOptLevel(rule, relExpr);

  return FALSE;
}


Int32 OptDefaults::getRulePriority(Rule* rule)
{
  // Pass 1 rules have the priority = 1 (least chance of pruning)
  // with exception of JoinToTSJ which has priority = 2
  // All Pass 2 transformation rules have priority = 2
  // All Remaining rules have priority = 3
  // This needs revisiting, can be optimized further
  // Consider centeralization of join implementation rules

  if (rule->getNumber() == JoinToTSJRuleNumber)
    return 2;
  if (GlobalRuleSet->getPassNRules(1)->contains(rule->getNumber()))
    return 1;
  if (rule->isTransformationRule())
    return 2;
  return 3;
}

NABoolean OptDefaults::pruneByOptLevel(Rule* rule, RelExpr* relExpr)
{
  // No pruning is allowed in the first optimization pass
  // This should have been checked by the caller of this function

  // Dont apply level1 prunings on simple queries (it does not worth it)
  if (numTables_ < 5)
    return FALSE;

  // get memUsed in MBs
  Lng32 memUsed = getMemUsed();

  // The queryComplexity_ is another factor used for the logical
  // complexity of the query. However physical complexities may
  // takeover (like in right trees with NJ and HJ) hence we need
  // to check taskCount_ is within acceptable limit
  if (queryComplexity_ < level1Threshold_ AND
      taskCount_ < level1SafetyNet_ AND
      memUsed < memUsageSafetyNet_)
    return FALSE; // need to adjust this later
                  // this will take care of subquery problem

  // lets not prune anything before the level1ImmunityLimit_ tasks.
  if (taskCount_ <= level1ImmunityLimit_ AND
      memUsed < memUsageSafetyNet_)
    return FALSE;

  NABoolean prune = FALSE;
    
  if(optimizerGracefulTermination() == 1)
  {
    prune = pruneByRProbability(rule, relExpr);
  }
  else{
    prune = pruneByPotential(rule,relExpr);
  }
  
  // if we ever pruned, the Global randomPruningOccured_ is set to TRUE
  if (prune) randomPruningOccured_ = TRUE;

  return prune;
}

NABoolean OptDefaults::pruneByRProbability(Rule* rule, RelExpr* relExpr)
{
  NABoolean prune = FALSE;
  
  // get memUsed in MBs
  Lng32 memUsed = getMemUsed();

  Int32 rulePriority = getRulePriority(rule);
  double pruneRate = 0;

  // rules with priority 1 have highest survival chance
  if (rulePriority == 1)
    pruneRate = 0;

  // rules with priority 2 are pruned based
  // on their location in the tree
  if (rulePriority == 2)
  {
    pruneRate = double(level1Constant1_)/100;
    // calculate heightFactor
    Lng32 heightFactor =
      relExpr->getGroupAttr()->getNumBaseTables();
    if (heightFactor <= 3)
      pruneRate = 0;
    if (heightFactor > numTables_-2)
      pruneRate = 0;

    // We add here that if this is a join that resulted from the
    // application of the PrimeTableRule, then we give it 0 initial
    // pruning rate.
    // should move this to a Join method
    if (relExpr->getOperator().match(REL_ANY_JOIN) &&
        ((Join*)relExpr)->isJoinFromPTRule())
      pruneRate = 0;
  }

  if (rulePriority == 3)
    pruneRate = double(level1Constant2_)/100;

  // Compile time Safety net is our last adjustment to pruneRate:
  // If the taskCount_ is above the level1SafetyNet_. Then increase
  // the prune rate even further. We increase pruneRate linearly
  // (wrt taskCount_) such that we have full pruning when taskCount
  // is twice the level1SafetyNet_

  // Prune rate adjustment as determined by amount of memory used
  double pruneMemAdj = 0;

  // Prune rate adjustment as determined by number of tasks done
  double pruneTaskAdj = 0;

  if (memUsed > memUsageSafetyNet_)
    pruneMemAdj = (double(2*memUsed)/memUsageSafetyNet_ - 2);

  if (taskCount_ > level1SafetyNet_)
    pruneTaskAdj = (double(taskCount_)/level1SafetyNet_ - 1);

  pruneRate += MAXOF(pruneMemAdj, pruneTaskAdj) * (1 - pruneRate);

  // Apply pruning with probability
  prune = ((*ranSeq_).random() < pruneRate);
  
  return prune;
}

NABoolean OptDefaults::pruneByPotential(Rule* rule, RelExpr* relExpr)
{
  NABoolean prune = FALSE;
  
  // get memUsed in MBs
  Lng32 memUsed = getMemUsed();

  // Prune rate adjustment as determined by amount of memory used
  double pruneMemFactor = 0;

    // Prune rate adjustment as determined by number of tasks done
  double pruneTaskFactor = 0;

  if (memUsed > memUsageSafetyNet_)
    pruneMemFactor = (double(2*memUsed)/memUsageSafetyNet_ - 2);

  if (taskCount_ > level1SafetyNet_)
    pruneTaskFactor = (double(taskCount_)/level1SafetyNet_ - 1);

  double pruneFactor = MAXOF(pruneMemFactor, pruneTaskFactor);
    
  double survivorFactor = MAXOF(0, (1 - pruneFactor));
    
  // determine adjustment
  Int32 adjustment = 0;
      
  if ((survivorFactor >= 0.7) && (survivorFactor < 1))
    adjustment = 1;
  else if ((survivorFactor >= 0.4) && (survivorFactor < 0.7))
    adjustment = 2;
  else if ((survivorFactor > 0) && (survivorFactor < 1))
    adjustment = 3;
  else if (survivorFactor <= 0)
    return TRUE;

  // get threshold
  Int32 combinedPotentialThreshold =
    getEnumPotentialThreshold();

  // adjust potential threshold
  if (adjustment)
  {
    if(combinedPotentialThreshold > 2)
      combinedPotentialThreshold = 3;

    combinedPotentialThreshold = MAXOF(0, (combinedPotentialThreshold - adjustment));
  }
    
  Int32 groupPotential = relExpr->getGroupAttr()->getPotential();

  Int32 exprPotential = relExpr->getPotential();
    
  Int32 combinedPotential = groupPotential + exprPotential;
    
  if (exprPotential < 0 )
    combinedPotential = -1;
      
  if (combinedPotential > combinedPotentialThreshold)
    prune = TRUE;
    
  return prune;
}

// -----------------------------------------------------------------------
// Initialize ALL cost related members here!
// -----------------------------------------------------------------------

void OptDefaults::initializeCostInfo()
{

  defs_ = &(ActiveSchemaDB()->getDefaults());
  // Temporarily set Calibrated CQD's ONLY for NEO using COMP_BOOL_155.
  // It only needs to be tested with Release Binary when Reference and
  // Target systems both are the same.
  // control query default REFERENCE_CPU_FREQUENCY '1600';
  // control query default TARGET_CPU_FREQUENCY '1600';
  // To effect MSCF_DEBUG_TO_RELEASE_MULTIPLIER this code can be moved
  // up in the begining of this method;
  // Ideally this values shl'd be placed in nadefaults.cpp once regressions are resolved.

  // on SQ, CB_155 is off and we mask the following code from code coverage.
  if(CmpCommon::getDefault(COMP_BOOL_155) == DF_ON )
  {
    NAString v="1600.0";
    defs_->validateAndInsert("REFERENCE_CPU_FREQUENCY", v, FALSE);
    defs_->validateAndInsert("TARGET_CPU_FREQUENCY", v, FALSE);
	v="0.000037";
    defs_->validateAndInsert("MSCF_ET_CPU", v, FALSE);
    v="0.0000088";
    defs_->validateAndInsert("MSCF_ET_IO_TRANSFER", v, FALSE);
    v="0.000013";
    defs_->validateAndInsert("MSCF_ET_NUM_LOCAL_MSGS", v, FALSE);
    defs_->validateAndInsert("MSCF_ET_NUM_REMOTE_MSGS", v, FALSE);
    v="0.00000467";
    defs_->validateAndInsert("MSCF_ET_LOCAL_MSG_TRANSFER", v, FALSE);
    defs_->validateAndInsert("MSCF_ET_REMOTE_MSG_TRANSFER", v, FALSE);
    v="113.0";
    defs_->validateAndInsert("REFERENCE_IO_SEQ_READ_RATE", v, FALSE);
    defs_->validateAndInsert("TARGET_IO_SEQ_READ_RATE", v, FALSE);
	v="0.000013";
    defs_->validateAndInsert("REFERENCE_MSG_LOCAL_TIME", v, FALSE);
	defs_->validateAndInsert("TARGET_MSG_LOCAL_TIME", v, FALSE);
    defs_->validateAndInsert("REFERENCE_MSG_REMOTE_TIME", v, FALSE);
	defs_->validateAndInsert("TARGET_MSG_REMOTE_TIME", v, FALSE);
    v="208.98";
    defs_->validateAndInsert("REFERENCE_MSG_LOCAL_RATE", v, FALSE);
    defs_->validateAndInsert("REFERENCE_MSG_REMOTE_RATE", v, FALSE);
    defs_->validateAndInsert("TARGET_MSG_LOCAL_RATE", v, FALSE);
    defs_->validateAndInsert("TARGET_MSG_REMOTE_RATE", v, FALSE);
	v="0.000178";
    defs_->validateAndInsert("CPUCOST_HASH_PER_BYTE", v, FALSE);
	v="0.0";
    defs_->validateAndInsert("CPUCOST_HASH_PER_KEY", v, FALSE);
	v="7.57";
    defs_->validateAndInsert("HJ_CPUCOST_INITIALIZE", v, FALSE);
	v="5.44";
    defs_->validateAndInsert("MJ_CPUCOST_INITIALIZE", v, FALSE);
	v="4.655";
    defs_->validateAndInsert("NJ_CPUCOST_INITIALIZE", v, FALSE);
	v="42.31";
    defs_->validateAndInsert("HGB_CPUCOST_INITIALIZE", v, FALSE);
	v="1.8169";
    defs_->validateAndInsert("SGB_CPUCOST_INITIALIZE", v, FALSE);
	v="44.43";
    defs_->validateAndInsert("SORT_CPUCOST_INITIALIZE", v, FALSE);
	v="10.0";
    defs_->validateAndInsert("SORT_QS_FACTOR", v, FALSE);
	v="5.0";
    defs_->validateAndInsert("SORT_RW_FACTOR", v, FALSE);
    defs_->validateAndInsert("SORT_RS_FACTOR", v, FALSE);
	v="1.95";
    defs_->validateAndInsert("HH_OP_ALLOCATE_HASH_TABLE", v, FALSE);
	v="0.1351";
    defs_->validateAndInsert("HH_OP_INSERT_ROW_TO_CHAIN", v, FALSE);
	v="0.5";
    defs_->validateAndInsert("MJ_CPUCOST_INSERT_ROW_TO_LIST", v, FALSE);
	v="0.00339";
    defs_->validateAndInsert("HH_OP_PROBE_HASH_TABLE", v, FALSE);
	v="0.000";
    defs_->validateAndInsert("CPUCOST_ENCODE_PER_BYTE", v, FALSE);
	v="0.002109";
    defs_->validateAndInsert("CPUCOST_COMPARE_SIMPLE_DATA_TYPE", v, FALSE);
	v="0.0031";
    defs_->validateAndInsert("CPUCOST_COPY_SIMPLE_DATA_TYPE", v, FALSE);
	v="0.03";
    defs_->validateAndInsert("CPUCOST_EVAL_ARITH_OP", v, FALSE);
	v="0.0000212";
    defs_->validateAndInsert("CPUCOST_EXCHANGE_COST_PER_BYTE", v, FALSE);
    defs_->validateAndInsert("CPUCOST_EXCHANGE_INTERNODE_COST_PER_BYTE", v, FALSE);
    defs_->validateAndInsert("CPUCOST_EXCHANGE_REMOTENODE_COST_PER_BYTE", v, FALSE);
	v="0.00006787";
    defs_->validateAndInsert("CPUCOST_COPY_ROW_PER_BYTE", v, FALSE);
	v="0.001";
    defs_->validateAndInsert("CPUCOST_PREDICATE_COMPARISON", v, FALSE);
    v="0.084848";
    defs_->validateAndInsert("CPUCOST_SCAN_DSK_TO_DP2_PER_KB", v, FALSE);
    v="52.0";
    defs_->validateAndInsert("CPUCOST_SUBSET_OPEN_AFTER_FIRST", v, FALSE);
	v="690.27";
    defs_->validateAndInsert("CPUCOST_SUBSET_OPEN", v, FALSE); // Curently, it is High (Not much Effect, since Scan)
	                                                           // Needs change later in code for
	                                                           // 0.00009 * [ Number of Hash2 Partitions in the table] + 0.0025 }/ MSCF_ET_CPU
	v="0.0008144";
    defs_->validateAndInsert("EX_OP_COPY_ATP", v, FALSE);
	v="0.00000000000000000000000000001"; // Ideally, Needs to be set to zero.
    defs_->validateAndInsert("EX_OP_ALLOCATE_BUFFER", v, FALSE);
	defs_->validateAndInsert("EX_OP_ALLOCATE_BUFFER_POOL", v, FALSE);
	defs_->validateAndInsert("EX_OP_ALLOCATE_TUPLE", v, FALSE);
	defs_->validateAndInsert("CPUCOST_COMPARE_COMPLEX_DATA_TYPE_OVERHEAD", v, FALSE);
	defs_->validateAndInsert("CPUCOST_COPY_ROW_OVERHEAD", v, FALSE);
	defs_->validateAndInsert("CPUCOST_DATARQST_OVHD", v, FALSE);
	defs_->validateAndInsert("CPUCOST_LIKE_COMPARE_OVERHEAD", v, FALSE);
	defs_->validateAndInsert("CPUCOST_SCAN_OVH_PER_KB", v, FALSE);
	defs_->validateAndInsert("CPUCOST_SCAN_OVH_PER_ROW", v, FALSE);
  }
  // -----------------------------------------------------------------------
  // All resource-to-time multipliers need to be re-calibrated in
  // a per statement basis since they may have been changed by the
  // user through a CQS statement.
  // -----------------------------------------------------------------------

  recalibrateCPU();

  recalibrateIOSeqTransfer();

  recalibrateIOSeeks();

  recalibrateLocalMsgTransfer();

  recalibrateLocalMsg();

  recalibrateRemoteMsgTransfer();

  recalibrateRemoteMsg();

}

// Compute the recommended # of CPUs. The argument cpuResourceRequired specifies the
// original CPU resource consumption. If the value is less than a threshold <n>,
// then the final number of CPU is cpuResourceRequired / workPerCPU. 
// If the value is largar than <n>, then the actual number of CPUs allocated 
// beyond <n> is reduced.  
//
// The ratio of reduction is specified by CQD MDOP_CPUS_PENALTY.
//
// Variable <n> is specified in CQD MDOP_CPUS_SOFT_LIMIT (default to 64).
//
double OptDefaults::computeRecommendedNumCPUs(double cpuResourceRequired)
{
  double workPerCPU = getWorkPerCPU();

  Lng32 n =
            (ActiveSchemaDB()->getDefaults()).getAsLong(MDOP_CPUS_SOFT_LIMIT);

  n /= 2; // Reduce <n> by half so that the mDoPc returned is suitable as a
          // a cap value to compute the final DoP.

  double cpuResourceFor_n_CPUs = n * workPerCPU;


  double mDoPc = 0;

  if (cpuResourceRequired > cpuResourceFor_n_CPUs)   {

    double penalty =
            (ActiveSchemaDB()->getDefaults()).getAsDouble(MDOP_CPUS_PENALTY);

    double additionalCPUs = (cpuResourceRequired - cpuResourceFor_n_CPUs) /
                           (workPerCPU * penalty);

    mDoPc =  n + additionalCPUs;
  } else
    // mDOPc = ECR / Work_Unit
    mDoPc = cpuResourceRequired / workPerCPU;

  return mDoPc;
}

// This routine adjusts the memory DoP number using the similar idea for CPU.
double 
OptDefaults::computeRecommendedNumCPUsForMemory(double memoryResourceRequired)
{
  double memoryPerCPU = getMemoryPerCPU();

  Lng32 n =
            (ActiveSchemaDB()->getDefaults()).getAsLong(MDOP_CPUS_SOFT_LIMIT);

  n /= 2; // Reduce <n> by half so that the mDoPc returned is suitable as a
          // a cap value to compute the final DoP.

  double totalMemoryFor_N_CPU = n * memoryPerCPU;


  double mDoPm = 0;

  if (memoryResourceRequired > totalMemoryFor_N_CPU)   {

    double penalty =
            (ActiveSchemaDB()->getDefaults()).getAsDouble(MDOP_MEMORY_PENALTY);

    double additionalCPUs = (memoryResourceRequired - totalMemoryFor_N_CPU) /
                           (memoryPerCPU * penalty);

    mDoPm =  n + additionalCPUs;
  } else
    mDoPm = memoryResourceRequired / memoryPerCPU;

  return mDoPm;
}

RequiredResources * OptDefaults::estimateRequiredResources(RelExpr* rootExpr)
{
  RequiredResources * requiredResources = NULL;
  if (CmpCommon::getDefault(ASG_FEATURE) == DF_ON)
  {
    requiredResources = new (CmpCommon::statementHeap()) RequiredResources();
    // compute and set the resources required for this query
    rootExpr->computeRequiredResources(*requiredResources);

    maxMaxCardinality_ = requiredResources->getMaxMaxCardinality().getValue();

    requiredMemoryResourceEstimate_ = requiredResources->getMemoryResources().getValue();
    requiredCpuResourceEstimate_ = requiredResources->getCpuResources().getValue();
    totalDataAccessCost_ = requiredResources->getDataAccessCost().getValue();
    maxOperatorMemoryEstimate_ = requiredResources->getMaxOperMemoryResources().getValue();
    maxOperatorCpuEstimate_ = requiredResources->getMaxOperCpuReq().getValue();
    maxOperatorDataAccessCost_ = requiredResources->getMaxOperDataAccessCost().getValue();
    memoryPerCPU_ = getDefaultAsDouble(MEMORY_UNIT_ESP)*1000000;
    workPerCPU_ = getDefaultAsDouble(WORK_UNIT_ESP)*1000000;
    defaultDegreeOfParallelism_ = getDefaultAsLong
      (DEFAULT_DEGREE_OF_PARALLELISM);

    // take into account the number of ESPs per node. The total number of CPUs set below will be
    // aka virtual # of CPUs_ to use for this query. Do not be confused with the actual # of
    // physical CPUs available.
    NADefaults &defs = ActiveSchemaDB()->getDefaults();
    NABoolean fakeEnv = FALSE;
    totalNumberOfCPUs_ = defs.getTotalNumOfESPsInCluster(fakeEnv);

    if ( !fakeEnv ) {
      
      // guard against any strange CQD(DEFAULT_DEGREE_OF_PARALLELISM), cqdDDoP.
      // if cqdDDoP were 33 then dDoP should be 32
      Lng32 cqdDDoP = getDefaultDegreeOfParallelism();
  
      if (cqdDDoP > totalNumberOfCPUs_) cqdDDoP = totalNumberOfCPUs_;
      // We should also floor it by totalNumberOfCPUs_/16
      // start dDoP with all available CPUs
      Lng32 dDoP = totalNumberOfCPUs_;
  
      // keep halving dDoP until dDoP <= cqdDDoP
      while (cqdDDoP < dDoP) dDoP /= 2;
  
      // dDDoP should be positive, since the minimal value for both cqdDDoP 
      // and totalNumberOfCPUs_  is 1.
      adjustedDegreeOfParallelism_ = dDoP;
  
      // set the memory and cpu resources to be use in computation of degree
      // of parallelism below. Just for this method.
      double memoryToConsider = requiredMemoryResourceEstimate_;
      double cpuToConsider = requiredCpuResourceEstimate_;
  
      ULng32 useOperatorMaxForComputations =
        getDefaultAsLong(USE_OPERATOR_MAX_FOR_DOP);
  
      ULng32 operatorResourceFactor =
        useOperatorMaxForComputations;
  
      if (useOperatorMaxForComputations)
      {
        memoryToConsider = maxOperatorMemoryEstimate_;
        cpuToConsider = MINOF(cpuToConsider,
                              (operatorResourceFactor*maxOperatorCpuEstimate_));
      }
  
      double mDoPm = computeRecommendedNumCPUsForMemory(memoryToConsider);
      double mDoPc = computeRecommendedNumCPUs(cpuToConsider);
  
      double resourceDoP = MAXOF(mDoPm, mDoPc);
  
      // numCPUs = total number of CPUs (including any down CPUs)
      Lng32 numCPUs = totalNumberOfCPUs_;
  
      // maxDoP = maximum degree of parallelism
      Lng32 maxDoP;
        // if  adaptive load balancing is OFF (-2), then don't increase Dop by power of 2
      if (getDefaultAsLong(AFFINITY_VALUE) == -2)
      {
        // take the resouceDoP if it is within the range [dDop, maxDoP], 
        // otherwise take either 1 (when no CQS in effect) or the high value
        if (resourceDoP < dDoP) {
            if (ActiveControlDB()->getRequiredShape() &&
                ActiveControlDB()->getRequiredShape()->getShape())
               maxDoP = dDoP;
            else
               maxDoP = 1;
        } else if (resourceDoP > numCPUs)
          maxDoP = numCPUs;
        else
          maxDoP = resourceDoP;
       }
      else {

        // Adjust the resourceDoP so that it is either equal to or a factor 
        // of numCPUs. 

        if (resourceDoP < dDoP) 
           maxDoP = 1;
        else if (resourceDoP > numCPUs) 
          maxDoP = numCPUs;
        else
          for ( maxDoP=resourceDoP; maxDoP<=numCPUs; maxDoP++ ) {
             if ( numCPUs % maxDoP == 0 )
                break;
          }

        //maxDoP = dDoP;
        //if (resourceDoP > dDoP)
        //  while ( (maxDoP < resourceDoP) && (maxDoP * 2 <= numCPUs) ) maxDoP *= 2;
      }
  
      // Adjust max degree of parallelism to make sure that no plan will have
      // higher degree of parallelism than the largest table in the query.
      // It is meant to avoid expensive exchanges for the purpose of matching
      // the high degree of parallelism only and to have a high degree of
      // parallelism when needed for large tables while not risking the huge
      // DoP for smaller table queries.
      if (CmpCommon::getDefault(COMP_BOOL_24) == DF_ON)
      {
        QueryAnalysis *qa = QueryAnalysis::Instance();
        Lng32 highestNumOfPartns = qa->getHighestNumOfPartns();
        Lng32 adjustedMaxDoP = MAXOF(highestNumOfPartns, dDoP);
  
        // Cap maxDoP to adjustedMaxDoP
        while (adjustedMaxDoP < maxDoP) maxDoP /= 2;
      }
  
      maximumDegreeOfParallelism_ = maxDoP;
  
  #ifdef _DEBUG
      if ((CmpCommon::getDefault( NSK_DBG ) == DF_ON) &&
          (CmpCommon::getDefault( NSK_DBG_GENERIC ) == DF_ON )) {
            CURRCONTEXT_OPTDEBUG->stream()
          << endl
          << "EMR = " << requiredMemoryResourceEstimate_ << endl
          << "MaxOperMemory = " << maxOperatorMemoryEstimate_ << endl
          << "Memory_Unit = " << memoryPerCPU_ << endl
          << "MDOPm = " << mDoPm << endl
          << "ECR = " << requiredCpuResourceEstimate_ << endl
          << "MaxOperCpu = " << maxOperatorCpuEstimate_ << endl
          << "Work_Unit = " << workPerCPU_ << endl
          << "MDOPc = " << mDoPc << endl
          << "MAX(MDOPm, MDOPc) = " << resourceDoP << endl
          << "numCPUs = " << numCPUs << endl
          << "DDoP = " << dDoP << endl
          << "MDoP = " << maximumDegreeOfParallelism_ << endl
          << "Data Access Cost = " << totalDataAccessCost_ << endl
          << "Max Oper Data Access Cost = " << maxOperatorDataAccessCost_ << endl;
      }
  #endif
    } else  // end of !fakeEnv
       maximumDegreeOfParallelism_ = totalNumberOfCPUs_;

  }
  return requiredResources;
}



void OptDefaults::initialize(RelExpr* rootExpr)
{
   compileTimeMonitor_	= (CmpCommon::getDefault(COMPILE_TIME_MONITOR) == DF_ON);
  incorporateSkewInCosting_ = (CmpCommon::getDefault(INCORPORATE_SKEW_IN_COSTING) == DF_ON);
  fakeHardware_ = (CmpCommon::getDefault(ARKCMP_FAKE_HW) == DF_ON);

  defs_ = &(ActiveSchemaDB()->getDefaults());

  partitioningSchemeSharing_ = defs_->getAsLong(PARTITIONING_SCHEME_SHARING);
  riskPremiumNJ_ = defs_->getAsDouble(RISK_PREMIUM_NJ);
  riskPremiumMJ_ = defs_->getAsDouble(RISK_PREMIUM_MJ);
  riskPremiumSerial_ = defs_->getAsDouble(RISK_PREMIUM_SERIAL);
  robustHjToNjFudgeFactor_ = defs_->getAsDouble(ROBUST_HJ_TO_NJ_FUDGE_FACTOR);
  robustSortGroupBy_ = defs_->getAsLong(ROBUST_SORTGROUPBY);
  robustQueryOptimization_ = CmpCommon::getDefault(ROBUST_QUERY_OPTIMIZATION);

  DefaultToken tok = CmpCommon::getDefault(OPTIMIZATION_LEVEL);
  if (tok == DF_MINIMUM)
    optLevel_ = OptDefaults::MINIMUM;
  else if (tok == DF_MEDIUM_LOW)
    optLevel_ = OptDefaults::MEDIUM_LOW;
  else if (tok == DF_MEDIUM)
    optLevel_ = OptDefaults::MEDIUM;
  else if (tok == DF_MAXIMUM)
    optLevel_ = OptDefaults::MAXIMUM;
  else
    optLevel_ = OptDefaults::MEDIUM; // The default, here & in NADefaults.cpp

  // reset optimization budget
  originalOptimizationBudget_ = -1;
  optimizationBudget_ = -1;

  // if using optimization level 2 and analysis is OFF
  // use optimization_level 3 i.e. medium optimization
  if((!QueryAnalysis::Instance()->isAnalysisON()) &&
     (optLevel_ == OptDefaults::MEDIUM_LOW))
     optLevel_ = OptDefaults::MEDIUM;

  tok = CmpCommon::getDefault(INDEX_ELIMINATION_LEVEL);
  if(tok == DF_MINIMUM OR (ActiveControlDB()->getRequiredShape() AND
        ActiveControlDB()->getRequiredShape()->getShape() AND
        NOT ActiveControlDB()->getRequiredShape()->getShape()->isCutOp())
    )
    indexEliminationLevel_ = OptDefaults::MINIMUM;
  else if(tok == DF_MEDIUM)
    indexEliminationLevel_ = OptDefaults::MEDIUM;
  else if(tok == DF_MAXIMUM)
    indexEliminationLevel_ = OptDefaults::MAXIMUM;
  else if(tok == DF_AGGRESSIVE)
    indexEliminationLevel_ = OptDefaults::AGGRESSIVE;

  optRulesGuidance_ = defs_->getAsULong(COMP_INT_77);

  if (optLevel_ == OptDefaults::MEDIUM)
  {
    // in the case of the MEDIUM optimization level which is
    // supposed to be default as of now, take the pruning constants
    // from the defaults table
    level1Constant1_ = defs_->getAsLong(OPTIMIZATION_LEVEL_1_CONSTANT_1);
    level1Constant2_ = defs_->getAsLong(OPTIMIZATION_LEVEL_1_CONSTANT_2);
  }
  // Just a temp replacement for old optimization level HIGH
  if (CmpCommon::getDefault(COMP_BOOL_80) != DF_OFF)
  {
    level1Constant1_ = 30;  // 30% pruning rate at HIGH
    level1Constant2_ = 0;   // no pruning for implementation rules
  }

  if (level1Constant1_ < 0) level1Constant1_ = 0;
  if (level1Constant1_ >100) level1Constant1_ = 100;
  if (level1Constant2_ < 0) level1Constant2_ = 0;
  if (level1Constant2_ >100) level1Constant2_ = 100;

  level1ImmunityLimit_ = defs_->getAsLong(OPTIMIZATION_LEVEL_1_IMMUNITY_LIMIT);
  level1MJEnumLimit_   = defs_->getAsLong(OPTIMIZATION_LEVEL_1_MJENUM_LIMIT);

  level1Threshold_ = defs_->getAsLong(OPTIMIZATION_LEVEL_1_THRESHOLD);
  // If MultiJoins was used then we uplift threshold to that of 10 way join.
  if (QueryAnalysis::Instance() &&
      QueryAnalysis::Instance()->multiJoinsUsed() &&
      level1Threshold_ < 5120)
    level1Threshold_ = 5120;

  level1SafetyNet_ = defs_->getAsLong(OPTIMIZATION_LEVEL_1_SAFETY_NET);
  maxDepthToCheckForCyclicPlan_ = defs_->getAsLong
    (MAX_DEPTH_TO_CHECK_FOR_CYCLIC_PLAN);
  memUsageTaskThreshold_ = defs_->getAsLong(MEMORY_MONITOR_AFTER_TASKS);
  memUsageTaskInterval_ = defs_->getAsLong(MEMORY_MONITOR_TASK_INTERVAL);
  memUsageSafetyNet_ = defs_->getAsLong(MEMORY_USAGE_SAFETY_NET);
  memUsageOptPassFactor_ = defs_->getAsDouble(MEMORY_USAGE_OPT_PASS_FACTOR);
  memUsageNiceContextFactor_ = defs_->getAsDouble(MEMORY_USAGE_NICE_CONTEXT_FACTOR);
  shortOptPassThreshold_ = defs_->getAsLong(SHORT_OPTIMIZATION_PASS_THRESHOLD);

  additiveResourceCosting_ = (CmpCommon::getDefault(TOTAL_RESOURCE_COSTING)                                == DF_ON);

  if (((RelRoot *)rootExpr)->getTriggersList() != NULL)
  {
     triggersPresent_ = TRUE;
  }

  // optimization goal initialization
  NABoolean succ=InitCostVariables();

  enableCrossProductControl_	= (CmpCommon::getDefault(CROSS_PRODUCT_CONTROL) != DF_OFF);

  enableNestedJoinControl_	= (CmpCommon::getDefault(NESTED_JOIN_CONTROL) != DF_OFF);

  enableZigZagControl_		= (CmpCommon::getDefault(ZIG_ZAG_TREES_CONTROL) == DF_ON);

  enableMergeJoinControl_	= (CmpCommon::getDefault(MERGE_JOIN_CONTROL) != DF_OFF);

  enableOrderedHashJoinControl_	= (CmpCommon::getDefault(ORDERED_HASH_JOIN_CONTROL) != DF_OFF);

  considerNestedJoin_		= (CmpCommon::getDefault(NESTED_JOINS) != DF_OFF);

  considerHashJoin_		= (CmpCommon::getDefault(HASH_JOINS) != DF_OFF);

  considerHybridHashJoin_	= considerHashJoin_ AND (CmpCommon::getDefault(HJ_TYPE) != DF_ORDERED);

  considerOrderedHashJoin_	= considerHashJoin_ AND (CmpCommon::getDefault(HJ_TYPE) != DF_HYBRID);

  considerMergeJoin_		= (CmpCommon::getDefault(MERGE_JOINS) != DF_OFF);

  considerMinMaxOpt_		= (CmpCommon::getDefault(MIN_MAX_OPTIMIZATION) != DF_OFF);
  considerZigZagTree_		= CmpCommon::getDefault(ZIG_ZAG_TREES);

  optimizerHeuristic1_		= (CmpCommon::getDefault(OPTIMIZER_HEURISTIC_1) == DF_ON);
  optimizerHeuristic2_		= (CmpCommon::getDefault(OPTIMIZER_HEURISTIC_2) == DF_ON);
  optimizerHeuristic3_		= (CmpCommon::getDefault(OPTIMIZER_HEURISTIC_3) == DF_ON);
  optimizerHeuristic4_		= (CmpCommon::getDefault(OPTIMIZER_HEURISTIC_4) == DF_ON);
  optimizerHeuristic5_		= (CmpCommon::getDefault(OPTIMIZER_HEURISTIC_5) == DF_ON);

  pushDownDP2Requested_ = defs_->getAsLong(OPTS_PUSH_DOWN_DAM);

  if ((rootExpr->getInliningInfo()).isUsedForMvLogging() == TRUE AND
      CmpCommon::getDefault(COMP_BOOL_93) == DF_ON)
     pushDownDP2Requested_ = TRUE;

  enableJoinToTSJRuleOnPass1_ = FALSE;

  // If the query contains an ASJ or embedded delete, the call below

  // will set enableJoinToTSJRuleOnPass1_ to TRUE.

  queryComplexity_ =
  rootExpr->calculateSubTreeComplexity(enableJoinToTSJRuleOnPass1_);

  numTables_ = rootExpr->getGroupAttr()->getNumBaseTables();

  // if analysis is OFF and query complexity is very high
  // then reduce opt level to MINIMUM
  Int32 tbase = defs_->getAsLong(COMP_INT_23) ;
  if((!QueryAnalysis::Instance()->isAnalysisON()) &&
     queryComplexity_ > tbase * pow(2,tbase-1))
     optLevel_ = OptDefaults::MINIMUM;

  // -----------------------------------------------------------------------
  // Optimizer pruning heuristics.
  // -----------------------------------------------------------------------
  optimizerPruning_ = (CmpCommon::getDefault(OPTIMIZER_PRUNING) == DF_ON) AND
        NOT pushDownDP2Requested_ AND (numTables_ > 0) AND CURRSTMT_OPTGLOBALS->pruningIsFeasible AND
        ( queryComplexity_ > defs_->getAsDouble(OPH_PRUNING_COMPLEXITY_THRESHOLD));

  level1SafetyNetMultiple_ = !optimizerPruning_ ? 0.0 :
    CmpCommon::getDefaultNumeric(OPTIMIZATION_LEVEL_1_SAFETY_NET_MULTIPLE);

  if (additiveResourceCosting_ AND
      optimizerPruning_ AND
      queryComplexity_ < 5120)
    optimizerPruning_ = FALSE;

  OPHpruneWhenCLExceeded_ = optimizerPruning_ AND
	(CmpCommon::getDefault(OPH_PRUNE_WHEN_COST_LIMIT_EXCEEDED) == DF_ON);
  OPHuseCandidatePlans_ = optimizerPruning_ AND
	(CmpCommon::getDefault(OPH_USE_CANDIDATE_PLANS) == DF_ON);
  OPHreduceCLfromCandidates_ = optimizerPruning_ AND OPHuseCandidatePlans_ AND
	(CmpCommon::getDefault(OPH_REDUCE_COST_LIMIT_FROM_CANDIDATES) == DF_ON);
  OPHreduceCLfromPass1Solution_ = optimizerPruning_ AND
	(CmpCommon::getDefault(OPH_REDUCE_COST_LIMIT_FROM_PASS1_SOLUTION) == DF_ON);
  OPHreuseFailedPlan_ = optimizerPruning_ AND
	(CmpCommon::getDefault(OPH_REUSE_FAILED_PLAN) == DF_ON);
  OPHskipOGTforSharedGCfailedCL_ = optimizerPruning_ AND
	(CmpCommon::getDefault(OPH_SKIP_OGT_FOR_SHARED_GC_FAILED_CL) == DF_ON);
  OPHuseCompCostThreshold_ = optimizerPruning_ AND
	(CmpCommon::getDefault(OPH_USE_COMPARE_COST_THRESHOLD) == DF_ON);

  OPHuseConservativeCL_ = (optimizerPruning_ AND
	(CmpCommon::getDefault(OPH_USE_CONSERVATIVE_COST_LIMIT) == DF_ON)) OR
         (optimizerPruning_ AND (defaultPerformanceGoal_ != NULL) AND
          defaultPerformanceGoal_->isOptimizeForResourceConsumption());

  OPHuseFailedPlanCost_ = optimizerPruning_ AND OPHreuseFailedPlan_ AND
	(CmpCommon::getDefault(OPH_USE_FAILED_PLAN_COST) == DF_ON);
  OPHuseNiceContext_ = optimizerPruning_ AND
	( (CmpCommon::getDefault(OPH_USE_NICE_CONTEXT) == DF_ON) OR
          ( (numTables_ > defs_->getAsLong(COMP_INT_0)) AND
            (optLevel_ < OptDefaults::MAXIMUM)
          )
        );
  OPHuseEnforcerPlanPromotion_ = OPHuseNiceContext_ AND
	(CmpCommon::getDefault(OPH_USE_ENFORCER_PLAN_PROMOTION) == DF_ON);

  OPHreuseOperatorCost_ = optimizerPruning_ AND OPHuseFailedPlanCost_ AND
	(CmpCommon::getDefault(OPH_REUSE_OPERATOR_COST) == DF_ON);
  OPHusePWSflagForContext_ = optimizerPruning_ AND OPHreuseFailedPlan_ AND
	(CmpCommon::getDefault(OPH_USE_PWS_FLAG_FOR_CONTEXT) == DF_ON);
  OPHuseCachedElapsedTime_ = optimizerPruning_ AND
        (CmpCommon::getDefault(OPH_USE_CACHED_ELAPSED_TIME) == DF_ON);
  OPHexitNJcrContChiLoop_ = optimizerPruning_ AND OPHreuseFailedPlan_ AND
	(CmpCommon::getDefault(OPH_EXITNJCRCONTCHILOOP) == DF_ON);
  OPHexitMJcrContChiLoop_ = optimizerPruning_ AND OPHreuseFailedPlan_ AND
	(CmpCommon::getDefault(OPH_EXITMJCRCONTCHILOOP) == DF_ON);
  OPHexitHJcrContChiLoop_ = optimizerPruning_ AND OPHreuseFailedPlan_ AND
	(CmpCommon::getDefault(OPH_EXITHJCRCONTCHILOOP) == DF_ON);

  dataFlowOptimization_         =
        (CmpCommon::getDefault(DATA_FLOW_OPTIMIZATION) == DF_ON);
  attemptESPParallelism_        =
        CmpCommon::getDefault(ATTEMPT_ESP_PARALLELISM);

  if ((rootExpr->getOperatorType() == REL_ROOT) &&
      (((RelRoot*)rootExpr)->disableESPParallelism()) &&
      (CmpCommon::getDefault(COMP_BOOL_187) == DF_OFF))
    attemptESPParallelism_ = DF_OFF;

  enableOrOptimization_		=
        (CmpCommon::getDefault(OR_OPTIMIZATION) != DF_OFF);

  // threshold for pruning based on a plans potential
  enumPotentialThreshold_ =
    (UInt32)(ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_24);

  // If the user specifies his value of cqd MINIMUM_ESP_PARALLELISM
  // then we'll take his value, otherwise will use the number of
  // segments as minimum ESP parallelism meaning that each segment
  // will have at least one ESP. We assume that each segment have
  // 16 CPUs. For Linux, minimally allow one ESP per SQL node. 
  minimumESPParallelism_ = defs_->getAsLong(MINIMUM_ESP_PARALLELISM);
  if ( minimumESPParallelism_ == 0) {
    minimumESPParallelism_ = gpClusterInfo->numOfSMPs();
  }

  if (minimumESPParallelism_ < 1)
    minimumESPParallelism_ = 1;
  if (minimumESPParallelism_ > maximumDegreeOfParallelism_)
    minimumESPParallelism_ = maximumDegreeOfParallelism_;

  // When attemptESPParallelism is MAXIMUM we'd like to increase the
  // chance to generate a parallel plan by reducing threshold used for
  // SYSTEM and ON options. We also don't want deviation from maximum.
  // At the same time we don't want maximum parallelism for triggers,
  // MVS, insert/update/delete statement because it seems that for
  // some cases parallel plans are not supported properly.
  // After some testing/tuning COMP_INT_8 will be replaced if with
  // regular CQD. March 2006
  maxParallelismIsFeasible_ = (CURRSTMT_OPTGLOBALS->forceParallelInsertSelect OR
	  CURRSTMT_OPTGLOBALS->maxParIsFeasible) AND (attemptESPParallelism_ == DF_MAXIMUM);
  numberOfPartitionsDeviation_  = ( maxParallelismIsFeasible_ ?
	     0 :
        (float)defs_->getAsDouble(NUMBER_OF_PARTITIONS_DEVIATION) );
  numberOfRowsParallelThreshold_= ( maxParallelismIsFeasible_ ?
	    (float)defs_->getAsLong(COMP_INT_8) :
        (float)defs_->getAsDouble(NUMBER_OF_ROWS_PARALLEL_THRESHOLD) );

  updatedBytesPerESP_           =  defs_->getAsDouble(UPDATED_BYTES_PER_ESP);
  deviationType2JoinsSystem_    =
    (CmpCommon::getDefault(NUM_OF_PARTS_DEVIATION_TYPE2_JOINS) == DF_SYSTEM);
  if ( NOT deviationType2JoinsSystem_ )
    numOfPartsDeviationType2Joins_ =
	(float)defs_->getAsDouble(NUM_OF_PARTS_DEVIATION_TYPE2_JOINS);
  parallelHeuristic1_	= (CmpCommon::getDefault(PARALLEL_HEURISTIC_1) == DF_ON)
                                  AND (attemptESPParallelism_ == DF_SYSTEM);
  parallelHeuristic2_	= (CmpCommon::getDefault(PARALLEL_HEURISTIC_2) == DF_ON)
                                  AND (attemptESPParallelism_ == DF_SYSTEM);
  parallelHeuristic3_   = (CmpCommon::getDefault(PARALLEL_HEURISTIC_3) == DF_ON)
                                  AND (attemptESPParallelism_ == DF_SYSTEM);
  parallelHeuristic4_	= (CmpCommon::getDefault(PARALLEL_HEURISTIC_4) == DF_ON)
                                  AND (attemptESPParallelism_ == DF_SYSTEM);

  // dataFlowOptimization_ overwrites heuristics 4 & 5 if its value is ON
  optimizerHeuristic4_          =
    optimizerHeuristic4_ OR dataFlowOptimization_;
  optimizerHeuristic5_          =
    optimizerHeuristic5_ OR dataFlowOptimization_;

  preferredProbingOrderForNJ_   =
    (CmpCommon::getDefault(PREFERRED_PROBING_ORDER_FOR_NESTED_JOIN) == DF_ON);

  orderedWritesForNJ_           =
    (CmpCommon::getDefault(UPD_ORDERED) == DF_ON);

  nestedJoinForCrossProducts_	=
    (CmpCommon::getDefault(NESTED_JOIN_FOR_CROSS_PRODUCTS) == DF_ON);

  joinOrderByUser_		= QueryAnalysis::Instance()->joinOrderByUser();
                                  // xxx no need to check for (QueryAnalysis::Instance()) any more

  ignoreExchangesInCQS_         = FALSE; // also see below for when this is set
  ignoreSortsInCQS_             = FALSE; // also see below for when this is set

  // Can't use CmpCommon::getDefault for # of blocks per access because we would
  // get an error that any numerical value that a user would set
  // is incompatible with the default token value DF_SYSTEM. So, must
  // set the last parameter explicitly to FALSE (ignore errors).
  if ( CmpCommon::getDefault(NUM_OF_BLOCKS_PER_ACCESS,0) != DF_SYSTEM)
  {
    numOfBlocksPerAccess_ = defs_->getAsULong(NUM_OF_BLOCKS_PER_ACCESS);
  }
  else
  {
    numOfBlocksPerAccess_ = 0;
  }

  reduction_to_push_gb_past_tsj_ =
    defs_->getAsDouble(REDUCTION_TO_PUSH_GB_PAST_TSJ);

  taskCount_ = 0;

  randomPruningOccured_ = FALSE; // not yet
  optimizerGracefulTermination_ =
    defs_->getAsULong(OPTIMIZER_GRACEFUL_TERMINATION);

  // initialize random sequence;
  if (ranSeq_ == NULL)
    ranSeq_ = new RandomSequence();  // this also does initialize
  else
    (*ranSeq_).initialize();


  reuseBasicCost_		= (CmpCommon::getDefault(REUSE_BASIC_COST) == DF_ON);

  useNewMdam_		= (CmpCommon::getDefault(NEW_MDAM) == DF_ON);
  // -----------------------------------------------------------------------
  // if there is a Control Query Shape in effect, we turn all compile
  // time heuristics OFF.
  // -----------------------------------------------------------------------
  if (ActiveControlDB()->getRequiredShape() &&
      ActiveControlDB()->getRequiredShape()->getShape() &&
      NOT ActiveControlDB()->getRequiredShape()->getShape()->isCutOp())
  {
    enableCrossProductControl_ = FALSE;
    enableNestedJoinControl_ = FALSE;
    enableZigZagControl_ = FALSE;
    enableMergeJoinControl_ = FALSE;
    //attemptESPParallelism_ = DF_ON;
    maxParallelismIsFeasible_ = FALSE;
    parallelHeuristic1_ = FALSE;
    parallelHeuristic2_ = FALSE;
    parallelHeuristic3_ = FALSE;
    parallelHeuristic4_ = FALSE;
    optimizerHeuristic1_ = FALSE;
    optimizerHeuristic2_ = FALSE;
    optimizerHeuristic3_ = FALSE;
    optimizerHeuristic4_ = FALSE;
    optimizerHeuristic5_ = FALSE;
    dataFlowOptimization_ = FALSE;
    reduction_to_push_gb_past_tsj_ = 0.0;

    optimizerPruning_ = FALSE;
    OPHpruneWhenCLExceeded_ = FALSE;
    OPHreduceCLfromCandidates_ = FALSE;
    OPHreduceCLfromPass1Solution_ = FALSE;
    OPHreuseFailedPlan_ = FALSE;
    OPHreuseOperatorCost_ = FALSE;
    level1SafetyNetMultiple_ = 3.0;
    OPHskipOGTforSharedGCfailedCL_ = FALSE;
    OPHuseCandidatePlans_ = FALSE;
    OPHuseCompCostThreshold_ = FALSE;
    OPHuseConservativeCL_ = FALSE;
    OPHuseEnforcerPlanPromotion_ = FALSE;
    OPHuseFailedPlanCost_ = FALSE;
    OPHuseNiceContext_ = FALSE;
    OPHusePWSflagForContext_ = FALSE;
    OPHuseCachedElapsedTime_ = FALSE;

    ignoreExchangesInCQS_ =
      ActiveControlDB()->getRequiredShape()->getIgnoreExchange();
    ignoreSortsInCQS_ =
      ActiveControlDB()->getRequiredShape()->getIgnoreSort();

    // The following are plan quality directives so I'm not changing them
    // U can argue against that but I think for those at least the user
    // should know what he is doing
    //considerNestedJoin_ = TRUE;
    //considerHashJoin_ = TRUE;
    //considerMergeJoin_ = TRUE;
    //considerZigZagTree_ = TRUE;
    //nestedJoinForCrossProducts_ = TRUE;
    //joinOrderByUser_ = FALSE;
  }

  mdamSelectionDefault_ = defs_->getAsDouble(MDAM_SELECTION_DEFAULT);
  readAheadMaxBlocks_ = defs_->getAsDouble(READ_AHEAD_MAX_BLOCKS);
  acceptableInputEstLogPropError_ = defs_->getAsDouble(ACCEPTABLE_INPUTESTLOGPROP_ERROR);

  // -----------------------------------------------------------------------
  // Initialize histogram optdefaults from the defaults table
  // -----------------------------------------------------------------------
  reduceBaseHistograms_		        = (CmpCommon::getDefault(DYNAMIC_HISTOGRAM_COMPRESSION) == DF_ON);
  reduceIntermediateHistograms_		= (CmpCommon::getDefault(HIST_INTERMEDIATE_REDUCTION) == DF_ON);

  joinCardLowBound_			= (defs_->getAsDouble(HIST_JOIN_CARD_LOWBOUND));
  ustatAutomation_		        = (defs_->getAsLong(USTAT_AUTOMATION_INTERVAL) > 0);

  defNoStatsUec_      			= defs_->getAsDouble(HIST_NO_STATS_UEC);
  defNoStatsRowCount_                   = defs_->getAsDouble(HIST_NO_STATS_ROWCOUNT);

  defSelForWildCard_		        = defs_->getAsDouble(HIST_DEFAULT_BASE_SEL_FOR_LIKE_WILDCARD);
  defSelForNoWildCard_			= defs_->getAsDouble(HIST_DEFAULT_SEL_FOR_LIKE_NO_WILDCARD);
  defSelForRangePred_			= defs_->getAsDouble(HIST_DEFAULT_SEL_FOR_PRED_RANGE);

  baseHistogramReductionFF_         = defs_->getAsDouble(HIST_BASE_REDUCTION_FUDGE_FACTOR);
  intermediateHistogramReductionFF_ = defs_->getAsDouble(HIST_INTERMEDIATE_REDUCTION_FUDGE_FACTOR);
  histogramReductionConstantAlpha_  = defs_->getAsDouble(HIST_CONSTANT_ALPHA);
  histMCStatsNeeded_		    = (CmpCommon::getDefault(HIST_MC_STATS_NEEDED) == DF_ON) ;
  histSkipMCUecForNonKeyCols_	    = (CmpCommon::getDefault(HIST_SKIP_MC_FOR_NONKEY_JOIN_COLUMNS) == DF_ON);
  histMissingStatsWarningLevel_	    = defs_->getAsLong(HIST_MISSING_STATS_WARNING_LEVEL);
  histOptimisticCardOpt_	    = defs_->getAsLong(HIST_OPTIMISTIC_CARD_OPTIMIZATION);
  histAssumeIndependentReduction_    = (CmpCommon::getDefault(HIST_ASSUME_INDEPENDENT_REDUCTION) == DF_ON);
  histUseSampleForCardEst_              = (CmpCommon::getDefault(HIST_USE_SAMPLE_FOR_CARDINALITY_ESTIMATION) == DF_ON);

  maxSkewValuesDetected_             = defs_->getAsULong(MAX_SKEW_VALUES_DETECTED);
  skewSensitivityThreshold_          = defs_->getAsDouble(SKEW_SENSITIVITY_THRESHOLD);

  useHighFreqInfo_                   = (CmpCommon::getDefault(HIST_USE_HIGH_FREQUENCY_INFO) == DF_ON);
  histDefaultSampleSize_             = defs_->getAsDouble(HIST_ON_DEMAND_STATS_SIZE);
  histTupleFreqValListThreshold_     = defs_->getAsULong(HIST_TUPLE_FREQVAL_LIST_THRESHOLD);
  histNumOfAddDaysToExtrapolate_ = defs_->getAsULong(HIST_NUM_ADDITIONAL_DAYS_TO_EXTRAPOLATE);

  //Only do histograms pre-fetching if histogram caching is on
  if(OptDefaults::cacheHistograms())
  {
  	preFetchHistograms_		        = (CmpCommon::getDefault(HIST_PREFETCH) == DF_ON);
  }
  else
  {
	  preFetchHistograms_           = FALSE;
  }

  // Find out what the RMS security key invalidation garbage collection
  // interval is (this mirrors the logic in runtimestats/ssmpipc.cpp), so
  // we can fail-safe the histogram cache.

  static char *sct = getenv("RMS_SIK_GC_INTERVAL_SECONDS");  // in seconds
  if (sct)
    {
      siKeyGCinterval_ = ((Int64) str_atoi(sct, str_len(sct)));
      if (siKeyGCinterval_ < 10)
        siKeyGCinterval_ = 10;
    }
  else
    siKeyGCinterval_ = (Int64)24 * 60 * 60; // 24 hours

  // -----------------------------------------------------------------------
  // Initialize recalibration constants:
  // -----------------------------------------------------------------------
  initializeCostInfo();

  NABoolean onlyMemoryOps = FALSE;
  calculatedMEMORY_LIMIT_PER_CPU_ = defs_->getAsDouble(BMO_MEMORY_SIZE);

  // Query Strategizer Params
  // used for explain
  // BEGIN
  useStrategizer_ =  
    ((CmpCommon::getDefault(QUERY_STRATEGIZER) == DF_ON) &&
     (QueryAnalysis::Instance() && QueryAnalysis::Instance()->multiJoinsUsed()));     
  cpuCost_ = -1;
  scanCost_ = -1;
  budgetFactor_ = -1;
  pass1Tasks_= -1;
  taskFactor_= -1;
  nComplexity_= -1;
  n_2Complexity_= -1;
  n2Complexity_= -1;
  n3Complexity_= -1;
  exhaustiveComplexity_= -1; 
  // END

  requiredESPs_ = -1;
  requiredScanDescForFastDelete_ = NULL;

  isSideTreeInsert_ = FALSE;
  
  return;
}

NABoolean OptDefaults::InitCostVariables()
{
  NABoolean rtnStatus = TRUE;      // assume the best

  if (defaultCostWeight_ == NULL &&
      (defaultCostWeight_ = new (heap_) ResourceConsumptionWeight(1,
						     1,
						     1,
						     0,
						     0
						     )) == NULL)
    return(FALSE);

  NAString goalText;
  DefaultToken goalTok = CmpCommon::getDefault(OPTIMIZATION_GOAL, goalText, -1);

  if (defaultPerformanceGoal_)
    {
      // Free the previous performance goal so there won't be
      // a leak in the global heap.
      NADELETEBASIC(defaultPerformanceGoal_, heap_);
      defaultPerformanceGoal_ = NULL;
    }

  if (goalTok == DF_LASTROW)
    {
      defaultPerformanceGoal_ = new (heap_) OptimizeForLastRow();
    }
  else if (goalTok == DF_FIRSTROW)
    {
      defaultPerformanceGoal_ = new (heap_) OptimizeForFirstRow();
    }
  else if (goalTok == DF_RESOURCES)
    {
      defaultPerformanceGoal_ = new (heap_) OptimizeForResourceConsumption();
    }
  else
    {
      // Unknown token.
      // Since NADefaults OPTIMIZATION_GOAL default-default is "LASTROW",
      // use that as a fallback.
      defaultPerformanceGoal_ = new (heap_) OptimizeForLastRow();
      *CmpCommon::diags() << DgSqlCode(+2055)		// warning only
        << DgString0(goalText)
        << DgString1("OPTIMIZATION_GOAL");
    }

  if (resourcePerformanceGoal_ == NULL)
    {
      // This one is a hardcoded performance goal, no consulting defaults
      resourcePerformanceGoal_ = new (heap_) OptimizeForResourceConsumption();
    }

  return(rtnStatus);
}

void OptDefaults::CleanupCostVariables()
{
  if (defaultCostWeight_ != NULL)
    delete defaultCostWeight_;
  if (defaultPerformanceGoal_ != NULL)
    NADELETEBASIC(defaultPerformanceGoal_, heap_);
  if (resourcePerformanceGoal_ != NULL)
    NADELETEBASIC(resourcePerformanceGoal_, heap_);

  defaultCostWeight_ = NULL;
  defaultPerformanceGoal_ = NULL;
  resourcePerformanceGoal_ = NULL;
}

// -----------------------------------------------------------------------
// recalibrateCPU adjusts the speed of the CPU based on the state of
// the code of the reference and target calibration clusters as well as
// on the frequency of the cpu of the reference and target clusters.
// -----------------------------------------------------------------------

void
OptDefaults::recalibrateCPU()
{

  // -----------------------------------------------------------------------
  // Get the reference and target cpu frequencies
  // -----------------------------------------------------------------------

  float referenceFrequency, targetFrequency;

  // The reference frequency was set in calibration:
  CMPASSERT(defs_->getFloat(REFERENCE_CPU_FREQUENCY,referenceFrequency));

  // Get the target CPU frequency from the registry of the node
  // where the compiler is running
  // for the case when the user did not change the default.
  if ( defs_->getProvenance(TARGET_CPU_FREQUENCY)
       ==
       NADefaults::INIT_DEFAULT_DEFAULTS
       )
    {

      // get  it from the registry:
      targetFrequency = (float)gpClusterInfo->processorFrequency();
    }
  else
    {
      // The user changed the defaults either through the SQL
      // defaults table or by using a CQD. Thus get what the
      // user wants:
      CMPASSERT(defs_->getFloat(TARGET_CPU_FREQUENCY,targetFrequency));

    }

  // -----------------------------------------------------------------------
  // Get the reference and target code state and figure out how much
  // faster or slower the current system will be due to differences in
  // code state:
  // -----------------------------------------------------------------------
  DefaultToken
    referenceCodeToken = defs_->getToken(REFERENCE_CODE)
    ,targetCodeToken = defs_->getToken(TARGET_CODE);
  CMPASSERT(referenceCodeToken != DF_noSuchToken);
  CMPASSERT(targetCodeToken != DF_noSuchToken);

  // MSCF_ET_CPU denotes the cpu-to-time multiplier as calibrated
  // in the REFERENCE system. We may need to change this value to reflect
  // the target system configuration if:
  //  * The code of the reference and target systems are different (to
  //    reflect the difference between debug VS release code)
  //  * The CPU frequencies of the reference and target system are
  //    different
  double mscfEtCpu = CostPrimitives::getBasicCostFactor(MSCF_ET_CPU);

  // The debug to release multiplier represents how many times RELEASE code is
  // faster than DEBUG code (so it is always greater than 1).
  float debugMultiplier;
  CMPASSERT(defs_->getFloat(MSCF_DEBUG_TO_RELEASE_MULTIPLIER,debugMultiplier));

  // Adjust speed based on target code state:
  if (referenceCodeToken != targetCodeToken)
    {
      if (referenceCodeToken == DF_DEBUG)
        {
          // target cluster using faster RELEASE code
          mscfEtCpu /= debugMultiplier;
        }
      else if (referenceCodeToken == DF_RELEASE)
        {
          // target cluster using slow DEBUG code
          mscfEtCpu *= debugMultiplier;
        }
      else
        {
          // Bad value
          CMPASSERT(FALSE);
        }
    } // recalibrate because of code base differences


  // Now adjust speed based on target cpu frequency:
  if (referenceFrequency != targetFrequency)
    {
      mscfEtCpu *= referenceFrequency/targetFrequency;
    } // recalibrate because of cpu frequency differences

  // Set the re-calibrated CPU speed:
  calibratedMSCF_ET_CPU_ = mscfEtCpu;

} //  MSCF_ET_CPU


void OptDefaults::recalibrateIOSeqTransfer()
{
  // -----------------------------------------------------------------------
  // Get the reference and target IO transfer
  // -----------------------------------------------------------------------

  float referenceIORate, targetIORate;

  // The reference transfer rate set in calibration:
  CMPASSERT(defs_->getFloat(REFERENCE_IO_SEQ_READ_RATE, referenceIORate));

  // Get the target transfer rate from the global cluster information.
  // Case when the user did not change the default.
  if ( defs_->getProvenance(TARGET_IO_SEQ_READ_RATE)
       == NADefaults::INIT_DEFAULT_DEFAULTS
     )
    {
      // get  it from the registry:
      targetIORate = gpClusterInfo->ioTransferRate();
    }
  else
    {
      // The user changed the defaults either through the SQL
      // defaults table or by using a CQD. Thus get what the
      // user wants:
      CMPASSERT(defs_->getFloat(TARGET_IO_SEQ_READ_RATE, targetIORate));

    }

  // MSCF_ET_IO_TRANSFER denotes the io-to-time multiplier as calibrated
  // in the REFERENCE system. We may need to change this value to reflect
  // the target system configuration if:
  //  * The disk IO transfer rates of the reference and target system are
  //    different
  double mscfEtIoXfer = CostPrimitives::getBasicCostFactor(MSCF_ET_IO_TRANSFER);

  // Now adjust rate based on target disk transfer rate:
  if (referenceIORate != targetIORate)
    {
      mscfEtIoXfer *= referenceIORate/targetIORate;
    } // recalibrate because of IO transfer rate differences

  // Set the re-calibrated IO transfer rate:
  calibratedMSCF_ET_IO_TRANSFER_ = mscfEtIoXfer;

}


void OptDefaults::recalibrateIOSeeks()
{
  // -----------------------------------------------------------------------
  // Get the reference and target seek time.
  // -----------------------------------------------------------------------

  float refSeekTime, targetSeekTime;

  // The reference seek time set in calibration:
  CMPASSERT(defs_->getFloat(REFERENCE_IO_SEEK_TIME, refSeekTime));

  // Get the target seek time from the global cluster information.
  // Case when the user did not change the default.
  if ( defs_->getProvenance(TARGET_IO_SEEK_TIME)
       == NADefaults::INIT_DEFAULT_DEFAULTS
     )
    {
      // get  it from the registry:
      targetSeekTime = (float)gpClusterInfo->seekTime();
    }
  else
    {
      // The user changed the defaults either through the SQL
      // defaults table or by using a CQD. Thus get what the
      // user wants:
      CMPASSERT(defs_->getFloat(TARGET_IO_SEEK_TIME, targetSeekTime));

    }

  // MSCF_ET_NUM_IO_SEEKS denotes the io-to-time multiplier as calibrated
  // in the REFERENCE system. We may need to change this value to reflect
  // the target system configuration if:
  // The disk seek time of the reference and target system are different.
  double mscfEtSeek = CostPrimitives::getBasicCostFactor(MSCF_ET_NUM_IO_SEEKS);

  // Now adjust rate based on target disk seek time:
  if (refSeekTime != targetSeekTime)
    {
      mscfEtSeek *= targetSeekTime/refSeekTime;
    } // recalibrate because of IO transfer rate differences

  // Set the re-calibrated disk seek time:
  calibratedMSCF_ET_NUM_IO_SEEKS_ = mscfEtSeek;
}

void OptDefaults::recalibrateLocalMsg()
{

  calibratedMSCF_ET_NUM_LOCAL_MSGS_ =
    recalibrate(MSCF_ET_NUM_LOCAL_MSGS
                ,REFERENCE_MSG_LOCAL_TIME
                ,TARGET_MSG_LOCAL_TIME
                );

}

void OptDefaults::recalibrateLocalMsgTransfer()
{

  calibratedMSCF_ET_LOCAL_MSG_TRANSFER_ =
    recalibrate(MSCF_ET_LOCAL_MSG_TRANSFER
                ,REFERENCE_MSG_LOCAL_RATE
                ,TARGET_MSG_LOCAL_RATE
                );
}

void OptDefaults::recalibrateRemoteMsg()
{

  calibratedMSCF_ET_NUM_REMOTE_MSGS_ =
    recalibrate(MSCF_ET_NUM_REMOTE_MSGS
                ,REFERENCE_MSG_REMOTE_TIME
                ,TARGET_MSG_REMOTE_TIME
                );

}

void OptDefaults::recalibrateRemoteMsgTransfer()
{

  calibratedMSCF_ET_REMOTE_MSG_TRANSFER_ =
    recalibrate(MSCF_ET_REMOTE_MSG_TRANSFER
                ,REFERENCE_MSG_REMOTE_RATE
                ,TARGET_MSG_REMOTE_RATE
                );
}



double OptDefaults::recalibrate(Int32 calEnum
                                ,Int32 referenceEnum
                                ,Int32 targetEnum)
{

  float referenceTime, targetTime;
  float calValue;
  // These asserts are required because the get functions side-effect
  // their parameters, DO NOT EVER REMOVE THEM!!!
  CMPASSERT(defs_->getFloat(calEnum, calValue));
  CMPASSERT(defs_->getFloat(referenceEnum,referenceTime));
  CMPASSERT(defs_->getFloat(targetEnum, targetTime));
  if ( referenceTime != targetTime )

    {

      CMPASSERT(referenceTime > 0.);
      CMPASSERT(targetTime > 0.);

      calValue *= referenceTime/targetTime;

    }

  return calValue;

}

void OptDefaults::setQueryMJComplexity(double complexity)
{
  if(complexity > defs_->getAsDouble(OPH_PRUNING_COMPLEXITY_THRESHOLD))
    isComplexMJQuery_ = TRUE;
  else
    isComplexMJQuery_ = FALSE;

  queryMJComplexity_ = complexity;
};

NABoolean OptDefaults::cacheHistograms()
{
  return 
    ((CmpCommon::getDefault(CACHE_HISTOGRAMS) == DF_ON) &&
     ActiveSchemaDB()->getDefaults().getAsLong(CACHE_HISTOGRAMS_IN_KB)>0);
}


// ---------------------------------------------------------------------
// OptDebug class methods
// ---------------------------------------------------------------------

OptDebug::OptDebug()
{
  fileIsGood_     = FALSE;
  logFileName_[0] = '\0';
  file_           = NULL;
  ciClass_        = CmpContextInfo::CMPCONTEXT_TYPE_NONE;
}

OptDebug::~OptDebug()
{
  if ( fileIsGood_ )
  {
    // Close opened files.
    fclose(file_);
    fstream_.close();
  }

  file_ = NULL;
}

// ---------------------------------------------------------------------
// open log file.
// ---------------------------------------------------------------------

// OptDebug::openLog() is called only when CQD NSK_DBG_LOG_FILE is ON 
// in ControlDB.cpp.
NABoolean OptDebug::openLog( const char* filename )
{
  if (fileIsGood_)
    closeLog();
    
  if ( !filename || strcmp(filename, "") == 0 ) return FALSE;

  //if ( fileIsGood_ ) return FALSE;  // One log file per session

  // Open log file for writing in append mode.
  fstream_.open( filename, ios::out | ios::app );
  if ( !fstream_.good() )
  {
    fstream_.close();
    return FALSE;
  }
  
  // Open the same file again for FILE object.
  file_ = fopen( filename, "ac" );
  if ( !file_ ) {
    fstream_.close();
    return FALSE;
  }

  fileIsGood_ = TRUE;
  strcpy( logFileName_, filename );

  return TRUE;
}

// ---------------------------------------------------------------------
// close log file.
// ---------------------------------------------------------------------

void OptDebug::closeLog()
{
  if ( fileIsGood_ ) return;  // One log file per session

  // Close opened files.
  fclose(file_);
  fstream_.close();
  logFileName_[0] = '\0';
  fileIsGood_ = FALSE;
}

// ---------------------------------------------------------------------
// Accessor functions
// ---------------------------------------------------------------------

ostream& OptDebug::stream()
{
  if ( fileIsGood_ )
  {
    fstream_.seekp( 0, ios::end );

    if ( CmpCommon::context() &&
         ciClass_ != CmpCommon::context()->getCIClass() ) 
      fstream_.setstate(std::ios_base::badbit);
    else
      fstream_.clear();

    return fstream_;
  }
  else
    return cout;
}

FILE* OptDebug::fp()
{
  if ( fileIsGood_ )
  {
    fseek( file_, 0, SEEK_END );
    return file_;
  }
  else
   return stdout;
}

void OptDebug::setCompileInstanceClass(const char* str) 
{
   if ( strcmp(str, "NONE") == 0 || strcmp(str, "USER") == 0 )
      ciClass_ = CmpContextInfo::CMPCONTEXT_TYPE_NONE;
   else
   if ( strcmp(str, "META") == 0 )
      ciClass_ = CmpContextInfo::CMPCONTEXT_TYPE_META;
   else
   if ( strcmp(str, "USTATS") == 0 )
      ciClass_ = CmpContextInfo::CMPCONTEXT_TYPE_USTATS;
}

// ---------------------------------------------------------------------
// Functions to show tree nodes and their properties for debugging purpose.
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// OptDebug::showTree()
// For a SQL query, either show the query tree or query execution plan tree.
// Equivalent to the main window in GUI display.
// ---------------------------------------------------------------------
void OptDebug::showTree( const ExprNode *tree,
                         const CascadesPlan *plan,
                         const char *prefix,
                         NABoolean showDetail )
{
  if ( tree == NULL && plan == NULL ) return;

  NAString indent (prefix, CmpCommon::statementHeap());
  indent += "  ";

  const RelExpr *re = ( plan != NULL ) ?
                      plan->getPhysicalExpr()->castToRelExpr() :
                      tree->castToRelExpr();

  // Show myself.
  showNode( re, plan, prefix, showDetail );

  // Show my children.
  if ( plan != NULL )
  {
    CollIndex index = 0;
    while ( plan->getSolutionForChild(index) )
    {
      showTree( plan->getSolutionForChild(index)->getPhysicalExpr(),
                plan->getSolutionForChild(index),
                indent,
                showDetail );
      index++;
    }
  }
  else
  {
    Int32 nc = re->getArity();
    for (Lng32 i = 0; i < nc; i++)
    {
      showTree( re->child(i).getPtr(), plan, indent, showDetail );
    }
  }
}

// ---------------------------------------------------------------------
// OptDebug::showNode()
// Show individual query tree node.
// ---------------------------------------------------------------------

void OptDebug::showNode( const ExprNode *tree,
                         const CascadesPlan *plan,
                         const char *prefix,
                         NABoolean showDetail )
{
  if ( tree == NULL ) return;

  const RelExpr *re = tree->castToRelExpr();

  ostream &out = stream();

  // Show my name, cost, rows, groups and expression type.
  out << prefix << "SQL node(" << re->getText().data() << ")" << endl
      << prefix << "cost: "    << getCost( re, plan )
                << " rows: "   << getRows( re, plan )
                << BLANK_SPACE << getGroups( re )
                << " type: "   << getType( re, plan ) << endl;

  // Show my item expression.
  if ( showDetail )
  {
    if ( CmpCommon::getDefault( NSK_DBG_PRINT_ITEM_EXPR ) == DF_ON )
      showItemExpr( tree, plan, prefix );

    // Show my properties (if available).
    showProperties( tree, plan, prefix );
  }

}

// ---------------------------------------------------------------------
// OptDebug::showItemExpr()
// Show Item Expression of a node.
// Equivalent to the Item Expression window in GUI display.
// ---------------------------------------------------------------------

void OptDebug::showItemExpr( const ExprNode *tree,
                             const CascadesPlan *plan,
                             const char *prefix )
{
  ostream &out = stream();

  LIST(ExprNode *)  localExpList(CmpCommon::statementHeap());
  LIST(NAString)    localLabelList(CmpCommon::statementHeap());
  ExprNode*         currExpr;
  NAString currLabel(CmpCommon::statementHeap());
  CollIndex         numEntries;

  NAString indent (prefix, CmpCommon::statementHeap());
  indent += "  ";

  const RelExpr *re = ( plan != NULL ) ?
                      plan->getPhysicalExpr()->castToRelExpr() :
                      tree->castToRelExpr();

  re->addLocalExpr( localExpList, localLabelList );

  // remove NULL pointers from the end of the list
  // (just looks nicer that way)
  while ( (numEntries = localExpList.entries()) > 0 AND
          localExpList[numEntries - 1] == NULL )
    localExpList.getLast( currExpr );

  out << prefix << "<Item Expression Dialog>" << endl;

  // add the remaining list elements to the list of
  // children of this expression widget
  Int32 numIE = 0;
  while ( localExpList.getFirst(currExpr) )
  {
    localLabelList.getFirst( currLabel );
    out << indent.data() << "<IE #" << numIE << ">" << endl
        << indent.data() << currLabel.data() << endl;

    if ( currExpr )
      currExpr->print( fp(), CONCAT(indent, "  "), "" );

    out << indent.data() << "<\\IE #" << numIE << ">" << endl;
    numIE++;
  }

  out << prefix << "<\\Item Expression Dialog>" << endl << endl;
}

// ---------------------------------------------------------------------
// OptDebug::showProperties()
// Show the properties for a query tree node.
// Equivalent to the Properties window in GUI dislay.
// ---------------------------------------------------------------------

void OptDebug::showProperties( const ExprNode *tree,
                               const CascadesPlan *plan,
                               const char *prefix )
{
  const RelExpr *re = ( plan != NULL ) ?
                      plan->getPhysicalExpr() :
                      tree->castToRelExpr();

  // Print cost.
  if ( CmpCommon::getDefault( NSK_DBG_PRINT_COST ) == DF_ON )
    showCosts( re, plan, prefix );

  // Print logical properties.
  if ( CmpCommon::getDefault( NSK_DBG_PRINT_LOG_PROP ) == DF_ON )
    showLogicalProperties( re, plan, prefix );

  // Print physical properties.
  if ( CmpCommon::getDefault( NSK_DBG_PRINT_PHYS_PROP) == DF_ON )
    showPhysicalProperties( re, plan, prefix );

  // Print characteristic inputs.
  if ( CmpCommon::getDefault( NSK_DBG_PRINT_CHAR_INPUT) == DF_ON )
    showCharacteristicInputs( re, plan, prefix );

  // Print characteristic outputs.
  if ( CmpCommon::getDefault( NSK_DBG_PRINT_CHAR_OUTPUT) == DF_ON )
    showCharacteristicOutputs( re, plan, prefix );

  // Print constraints.
  if ( CmpCommon::getDefault( NSK_DBG_PRINT_CONSTRAINT) == DF_ON )
    showConstraints( re, plan, prefix );

  // Print context.
  if ( CmpCommon::getDefault( NSK_DBG_PRINT_CONTEXT) == DF_ON )
    showContext( re, plan, prefix );

}

// ---------------------------------------------------------------------
// OptDebug::getCost()
// Get the rollup cost for this operator (node).
// ---------------------------------------------------------------------

double OptDebug::getCost( const RelExpr *re, const CascadesPlan *plan )
{
  double cost = (double)-1;

  GroupAttributes *ga = re->getGroupAttr();

  if ( ga != NULL )
  {
    const Cost *planCost = ( plan != NULL ) ?
                           plan->getRollUpCost() : re->getRollUpCost();

    if ( planCost )
      cost = planCost->convertToElapsedTime().getValue();
  }

  return cost;
}

// ---------------------------------------------------------------------
// OptDebug::getRows()
// Get the number of rows to be returned from this operator (node).
// ---------------------------------------------------------------------

double OptDebug::getRows( const RelExpr *re, const CascadesPlan *plan )
{
  double rows = (double) -1;

  GroupAttributes *ga = re->getGroupAttr();

  if ( ga != NULL )
  {
    CostScalar numRows = (CostScalar)-1;

    if ( plan != NULL )
    {
      ga = plan->getPhysicalExpr()->castToRelExpr()->getGroupAttr();
      EstLogPropSharedPtr inputLP = plan->getContext()->getInputLogProp();

      if ( ga->existsLogExprForSynthesis() )
        numRows = ga->outputLogProp(inputLP)->getResultCardinality();
    }

    rows = numRows.value();
  }

  return rows;
}

// ---------------------------------------------------------------------
// OptDebug::getGroups()
// Get the group attributes for this operator (node).
// ---------------------------------------------------------------------

const NAString OptDebug::getGroups( const RelExpr *re )
{
  const GroupAttributes *ga = re->getGroupAttr();

  Lng32 groupId = (Lng32) re->getGroupId();

  if ( ga != NULL )
  {
    if ( groupId == NULL_COLL_INDEX )
      sprintf( CURRSTMT_OPTGLOBALS->returnString, 
               "in: %d, out: %d",
               ga->getCharacteristicInputs().entries(),
               ga->getCharacteristicOutputs().entries() );
    else
      sprintf( CURRSTMT_OPTGLOBALS->returnString, 
               "grp: %d, in: %d, out: %d",
               groupId,
               ga->getCharacteristicInputs().entries(),
               ga->getCharacteristicOutputs().entries() );
  }
  else
  {
    sprintf( CURRSTMT_OPTGLOBALS->returnString, "No Group Properties" );
  }

  return NAString(CURRSTMT_OPTGLOBALS->returnString);
}

// ---------------------------------------------------------------------
// OptDebug::getType()
// Get the expression type for this operator (node).
// ---------------------------------------------------------------------

const NAString OptDebug::getType( const RelExpr *re, const CascadesPlan *plan )
{
  if ( re->isPhysical() )
  {
    if ( plan != NULL )
    {
      if ( plan->getPhysicalProperty() != NULL &&
           plan->getRollUpCost() != NULL )
        sprintf( CURRSTMT_OPTGLOBALS->returnString, "plan" );
      else
        sprintf( CURRSTMT_OPTGLOBALS->returnString, "incomplete plan" );
    }
    else
    {
      if ( re->getPhysicalProperty() != NULL &&
           re->getRollUpCost() != NULL )
        sprintf( CURRSTMT_OPTGLOBALS->returnString, "plan" );
      else
        sprintf( CURRSTMT_OPTGLOBALS->returnString, "physical" );
    }
  }
  else
  {
    sprintf( CURRSTMT_OPTGLOBALS->returnString, "Logical" );
  }

  return NAString(CURRSTMT_OPTGLOBALS->returnString);
}

// ---------------------------------------------------------------------
// OptDebug::showCharacteristicInputs()
// Show characteristic inputs for this operator (node).
// ---------------------------------------------------------------------

void OptDebug::showCharacteristicInputs( const RelExpr *re,
                                         const CascadesPlan *plan,
                                         const char *prefix )
{
  const GroupAttributes *ga = re->getGroupAttr();
  if ( ga == NULL ) return;

  ostream &out = stream();

  NAString unparsed(CmpCommon::statementHeap());

  NAString indent (prefix, CmpCommon::statementHeap());
  indent += "  ";

  out << prefix << "<Characteristic Inputs>" << endl;

  const ValueIdSet &inputs = ga->getCharacteristicInputs();

  for ( ValueId x = inputs.init(); inputs.next(x); inputs.advance(x) )
  {
    unparsed = "";
    x.getItemExpr()->unparse( unparsed );

    out << indent.data()
        << "ValId #" << (CollIndex) x << ": " << unparsed.data() << endl;
  } // end for

  out << prefix << "<\\Characteristic Inputs>" << endl << endl;
}

// ---------------------------------------------------------------------
// OptDebug::showCharacteristicOutputs()
// Show characteristic outputs for this operator (node).
// ---------------------------------------------------------------------

void OptDebug::showCharacteristicOutputs( const RelExpr *re,
                                          const CascadesPlan *plan,
                                          const char *prefix )
{
  const GroupAttributes *ga = re->getGroupAttr();
  if ( ga == NULL ) return;

  ostream &out = stream();

  NAString unparsed(CmpCommon::statementHeap());

  NAString indent (prefix, CmpCommon::statementHeap());
  indent += "  ";

  out << prefix << "<Characteristic Outputs>" << endl;

  const ValueIdSet &outputs = ga->getCharacteristicOutputs();

  for ( ValueId x = outputs.init(); outputs.next(x); outputs.advance(x) )
  {
    unparsed = "";
    x.getItemExpr()->unparse( unparsed );

    out << indent.data()
        << "ValId #" << (CollIndex) x << ": " << unparsed.data() << endl;
  } //end for

  out << prefix << "<\\Characteristic Outputs>" << endl << endl;
}

// ---------------------------------------------------------------------
// OptDebug::showConstraints()
// Show constraints for this operator (node).
// ---------------------------------------------------------------------

void OptDebug::showConstraints( const RelExpr *re,
                                const CascadesPlan *plan,
                                const char *prefix )
{
  const GroupAttributes *ga = re->getGroupAttr();
  if ( ga == NULL ) return;

  ostream &out = stream();

  NAString unparsed(CmpCommon::statementHeap());

  NAString indent (prefix, CmpCommon::statementHeap());
  indent += "  ";

  out << prefix << "<Constraints>" << endl;

  const ValueIdSet &constraints = ga->getConstraints();
  for ( ValueId x = constraints.init();
        constraints.next(x);
        constraints.advance(x) )
  {
    unparsed = "";
    x.getItemExpr()->unparse( unparsed );

    out << indent.data()
        << "ValId #" << (CollIndex) x << ": " << unparsed.data() << endl;
  } // end for

  out << prefix << "<\\Constraints>" << endl << endl;
}

// ---------------------------------------------------------------------
// OptDebug::showContext()
// Show context for this operator (node).
// ---------------------------------------------------------------------

void OptDebug::showContext( const RelExpr *re,
                            const CascadesPlan *plan,
                            const char *prefix )
{
  const Context *context = ( plan != NULL ) ? plan->getContext() : NULL;
  if ( context == NULL ) return;

  ostream &out = stream();

  NAString unparsed(CmpCommon::statementHeap());

  NAString indent (prefix, CmpCommon::statementHeap());
  indent += "  ";

  out << prefix << "<Contexts>" << endl
      << indent.data() << "Context: " << context->getRequirementsString().data()
      << " niceContext: " << context->isNice()
      << endl;

  // Now display each of its child contexts.
  const RelExpr *physExpr = plan->getPhysicalExpr();
  Int32 nc = ( physExpr  != NULL ) ? physExpr->getArity() : 0;
  for (Int32 i = 0; i < nc; i++)
  {
    if ( plan->getContextForChild(i) != NULL )
    {
      out << indent.data() << "Context[" << i << "]: "
          << plan->getContextForChild(i)->getRequirementsString().data()
          << endl;
    }
  }

  out << prefix << "<\\Contexts>" << endl << endl;
}

// ---------------------------------------------------------------------
// OptDebug::showCosts()
// Show costs for this operator (node).
// ---------------------------------------------------------------------

void OptDebug::showCosts( const RelExpr *re,
                          const CascadesPlan *plan,
                          const char *prefix )
{
  ostream &out = stream();

  const Cost *rollUpCost   = ( plan != NULL ) ?
                             plan->getRollUpCost() :
                             re->getRollUpCost();

  const Cost *operatorCost = ( plan != NULL ) ?
                             plan->getOperatorCost() :
                             re->getOperatorCost();

  if ( rollUpCost == NULL || operatorCost == NULL ) return;

  NAString indent (prefix, CmpCommon::statementHeap());
  indent += "  ";

  out << prefix << "<Costs>" << endl;

  // Display scan specific data:
  OperatorTypeEnum ot = re->getOperatorType();
  switch (ot)
  {
    case REL_FILE_SCAN:
    {
      // Print the number plan fragments per cpu:
      // !!! The following function is not available at this time.
      /*
      out << prefix << "Cost:" << endl
          << indent.data() << "Blocks to read per DP2 access: "
          << ((FileScan *)(re))->getNumberOfBlocksToReadPerAccess() << endl;
      */
    }
  } // switch

  // Display cost class dependant info:
  showCostInDetail( "**** Roll-up Cost ****",  rollUpCost,   indent.data() );
  showCostInDetail( "**** Operator Cost ****", operatorCost, indent.data() );

  out << prefix << "<\\Costs>" << endl << endl;
}

// ---------------------------------------------------------------------
// OptDebug::showLogicalProperties()
// Show logical properties for this operator (node).
// ---------------------------------------------------------------------

void OptDebug::showLogicalProperties( const RelExpr *re,
                                      const CascadesPlan *plan,
                                      const char *prefix )
{
  const GroupAttributes *ga = re->getGroupAttr();
  if ( ga == NULL ) return;

  ostream &out = stream();

  NAString indent (prefix, CmpCommon::statementHeap());
  indent += "  ";

  out << prefix << "<Logical Properties>" << endl;

  CollIndex numEntries = ga->getInputLogPropList().entries();

  for (CollIndex i = 0; i < numEntries; i++)
  {
    if ( i > 0 )
    {
      out << indent.data() << "********************************" << endl;
    }

    out << indent.data()
        << "Input Est. Log. Props: #" << i
        << "; Estimated Rows = "
        << ga->getInputLogPropList()[i]->getResultCardinality().getValue()
	<< (ga->getInputLogPropList()[i]->isCardinalityEqOne() ? "Single" : "Multiple") << "invocations" //++MV
        << endl;

    showEstLogProp( ga->getInputLogPropList()[i], indent.data() );

    out << indent.data() << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << endl;

    out << indent.data() << "Output Est. Log. Props: #" << i
        << "; Estimated Rows = "
        << ga->getOutputLogPropList()[i]->getResultCardinality().getValue()
        << "; Max cardinality = "
        << ga->getOutputLogPropList()[i]->getMaxCardEst().getValue()
        << endl;

    showEstLogProp( ga->getOutputLogPropList()[i], indent.data() );

  } // end for

  out << prefix << "<\\Logical Properties>" << endl << endl;
}

// ---------------------------------------------------------------------
// OptDebug::showPhysicalProperties()
// Show physical properties for this operator (node).
// Refer to comments in RelExpr.h, physical properties is applicable
// after optimization is complete.
// ---------------------------------------------------------------------

void OptDebug::showPhysicalProperties( const RelExpr *re,
                                       const CascadesPlan *plan,
                                       const char *prefix )
{
  const PhysicalProperty *pp = ( plan != NULL ) ?
                               plan->getPhysicalProperty() :
                               re->getPhysicalProperty();
  if ( pp == NULL ) return;

  ostream &out = stream();

  NAString indent (prefix, CmpCommon::statementHeap());
  indent += "  ";

  NAString propText(CmpCommon::statementHeap());

  out << prefix << "<Physical Properties>" << endl;

  //-------------------------------------------------------
  // Physical property -> sorted by ...
  //-------------------------------------------------------
  const ValueIdList &sortKey = pp->getSortKey();
  if ( sortKey.entries() > 0 )
  {
    propText = "ordered_by(";
    for (Int32 i = 0; i < (Int32)sortKey.entries(); i++)
    {
      if (i > 0)
        propText += ", ";
      sortKey[i].getItemExpr()->unparse( propText,
                                         DEFAULT_PHASE,
                                         EXPLAIN_FORMAT );
    }
    propText += ")";

    out << indent.data() << propText.data() << endl;

    //-----------------------------------------------------
    // Physical Prop -> sort order type ...
    // Only displayed if there was a sort key
    //-----------------------------------------------------

    switch (pp->getSortOrderType())
    {
      case NO_SOT:
        propText = "undetermined sort order type";
        break;
      case ESP_NO_SORT_SOT:
        propText =
          "executor process sort order from an index";
        break;
      case ESP_VIA_SORT_SOT:
        propText =
          "executor process sort order from sorting";
        break;
      case DP2_SOT:
        propText =
          "DP2 process sort order from an index";
        break;
      default:
        // should never happen
        propText = "";
        break;
    }
    out << indent.data() << propText.data() << endl;

    if (pp->getDp2SortOrderPartFunc() != NULL)
    {
      propText = "Dp2 Sort Order Partitioning Function: ";
      propText += pp->getDp2SortOrderPartFunc()->getText();
      out << indent.data() << propText.data() << endl;
    }

  } // endif (sortKey.entries() > 0)

  //-------------------------------------------------------
  // Physical Prop -> partitioning ...
  //-------------------------------------------------------
  if ( pp->getPartitioningFunction() )
  {
    out << indent.data() << "partitioning function: "
        << pp->getPartitioningFunction()->getText().data() << endl;
  }

  //-------------------------------------------------------
  // Physical Prop -> location for plan execution ...
  //-------------------------------------------------------
  switch (pp->getPlanExecutionLocation())
  {
    case EXECUTE_IN_MASTER_AND_ESP:
      propText = "can execute in master and ESP";
      break;
    case EXECUTE_IN_MASTER:
      propText = "can execute in master only";
      break;
    case EXECUTE_IN_ESP:
      propText = "can execute in ESP only";
      break;
    case EXECUTE_IN_DP2:
      propText = "executes in DP2";
      break;
    default:
      propText = "executes in ???";
      break;
  }
  out << indent.data() << propText.data() << endl;

  //-------------------------------------------------------
  // Physical Prop -> Source for Partitioned data ...
  //-------------------------------------------------------
  switch (pp->getDataSourceEnum())
  {
    case SOURCE_PERSISTENT_TABLE:
      propText = "source: persistent table";
      break;
    case SOURCE_TEMPORARY_TABLE:
      propText = "source: temporary table";
      break;
    case SOURCE_TRANSIENT_TABLE:
      propText = "source: transient table";
      break;
    case SOURCE_VIRTUAL_TABLE:
      propText = "source: virtual table";
      break;
    case SOURCE_TUPLE:
      propText = "source: tuple";
      break;
    case SOURCE_ESP_DEPENDENT:
      propText = "source: esp, dependent";
      break;
    case SOURCE_ESP_INDEPENDENT:
      propText = "source: esp, independent";
      break;
    default:
      break;
  }
  out << indent.data() << propText.data() << endl;

  //-------------------------------------------------------
  // Physical Prop -> Index data
  //-------------------------------------------------------
  if (pp->getIndexDesc())
  {
    // Index levels
    out << indent.data() << "Index levels: "
        << pp->getIndexDesc()->getIndexLevels() << endl;

    // Record length
    out << indent.data() << "Record length: "
        << pp->getIndexDesc()->getRecordLength() << endl;

    // Record size in kb:
    out << indent.data() << "Record size in kb: "
        << pp->getIndexDesc()->getRecordSizeInKb().value() << endl;

    // block size:
    out << indent.data() << "Block size in kb: "
        << pp->getIndexDesc()->getBlockSizeInKb().value() << endl;

    // Records per blk:
    out << indent.data() << "Estimated records per blk: "
        << pp->getIndexDesc()->getEstimatedRecordsPerBlock().value() << endl;
  }

  out << prefix << "<\\Physical Properties>" << endl << endl;
}

// ---------------------------------------------------------------------
// OptDebug::showEstLogProp()
// Show estimated logical properties for this operator (node).
// ---------------------------------------------------------------------

void OptDebug::showEstLogProp( const EstLogPropSharedPtr& estLogProp,
                               const char *prefix )
{
  if (estLogProp == NULL) return;

  ostream &out = stream();

  NAString propText(CmpCommon::statementHeap());

  if ( estLogProp->getInputForSemiTSJ() )
  {
    out << prefix << "Input For Semi-TSJ" << endl;
  }

  const ColStatDescList & stats = estLogProp->getColStats();
  Int32 numStats = (Int32)stats.entries();
  for (Int32 i = 0; i < numStats; i++)
  {
    if ( i > 0 )
      out << prefix << "---------------------------------" << endl;

    ColStatsSharedPtr colStats = stats[i]->getColStats();

    out << prefix << "Table Column: ";
    if (colStats->getStatColumns().entries() > 0)
      out << colStats->getStatColumns()[0]->getFullColRefNameAsAnsiString().data();
    else
      out << "(None)";
    out << endl;

    if (colStats->isFakeHistogram())
      out << prefix << "*** FAKE HISTOGRAM ***" << endl;

    out << prefix << "Histogram ID = " << colStats->getHistogramId()
        << ": uec = " << colStats->getTotalUec().getValue()
        << "; rowcount = " << colStats->getRowcount().getValue() << endl;

    out << prefix << "MinValue = " << colStats->getMinValue().getText().data()
        << endl;

    out << prefix << "MaxValue = " << colStats->getMaxValue().getText().data()
        << endl;

    out << prefix << "RowRedFactor = " << colStats->getRedFactor().getValue()
        << "; UecRedFactor = " << colStats->getUecRedFactor().getValue()
        << endl;

    ColStatDescSharedPtr tempStatDesc = stats[i];

    if (tempStatDesc->getAppliedPreds().entries() > 0)
    {
      out << prefix << "Applied Predicates:" << endl;

      // we'd like to be able to see *ALL* of the applied predicates!
      ValueIdSet thePreds = tempStatDesc->getAppliedPreds() ;
      ValueIdList predList = thePreds ;
      for ( CollIndex i = 0 ; i < predList.entries() ; i++ )
      {
        propText = "       ";
        ValueIdSet tempSet ;
        tempSet.insert (predList[i]) ;
        tempSet.unparse(propText) ;

        // now we want to filter out any unnecessary references to
        // SYSKEY's, INDEXes, etc., in the applied-predicate list

        Int32 loop_escape = 0 ;
        while ( propText.contains ("\\NSK") )
        {
          if ( loop_escape++ > 100 ) break ; // avoid infinite loops!

          UInt32 i ;
          size_t pos = 0 ;
          size_t length = 0 ;

          // filter out anything that looks like the regular expression
          //      "\\NSK...([0-9]+)"

          pos = propText.index("\\NSK") ;
          if (propText(pos-1) == ',' ) pos-- ; // kill extra comma
          for ( i = pos ; i < propText.length() ; i++, length++ )
          {
            if ( propText(i) == ')' ) break ;
          }

          propText.replace ( pos, length+1, "" );
        }

        out << prefix << propText.data() << endl;
      }
    }  // if (tempStatDesc->getAppliedPreds().entries() > 0)

    HistogramSharedPtr hist = colStats->getHistogram();
    if ( hist != NULL )
    {
      for (CollIndex i = 0; i < hist->entries(); i++)
      {
        char *inclusiveBoundStr = (char *) (((*hist)[i].isBoundIncl()) ? " <= " : " <  ");

        out << prefix << "Bound " << i << inclusiveBoundStr
            << (*hist)[i].getBoundary().getText().data()
            << ": rows = " << (*hist)[i].getCardinality().getValue()
            << ", uec = " << (*hist)[i].getUec().getValue() << endl;
      }
    } // end if (hist)

  } // end for

}

// ---------------------------------------------------------------------
// OptDebug::showCostInDetail()
// Show cost information in detail for this operator (node).
// ---------------------------------------------------------------------

void OptDebug::showCostInDetail( const NAString& header,
                                 const Cost *cost,
                                 const char *prefix )
{
  ostream &out = stream();

  // Print the number of CPUs:
  out << prefix << "count of CPUs: " << cost->getCountOfCPUs() << endl;

  // Print the number plan fragments per cpu:
  out << prefix
      << "plan fragments per CPU: " << cost->getPlanFragmentsPerCPU() << endl;

  // Display header:
  out << prefix << "@@@@@@@@@@@@@@@@@@@@@@@@@@" << endl
      << prefix << header.data() << endl;

  // Print the elapsed time for this cost object:
  out << prefix
      << "elapsed time = " << cost->convertToElapsedTime().value() << endl;

  // Print Priority
  out << prefix
	  << "priority: level = " << cost->getPlanPriority().getLevel()
	  << " demotion = " << cost->getPlanPriority().getDemotionLevel()
          << " risk premium = " << cost->getPlanPriority().riskPremium().getValue()
	  << endl;

  // Depending on the cost model in effect display the cost details
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    // Display cost:
    // The order in which vectors are displayed reflects current usage:
    // SCMLR

    const SimpleCostVector &scm   = cost->getScmCplr();

    showSimpleCostVector("simple cost model", scm, prefix, out);
  }
  else
  {
    // Display cost:
    // The order in which vectors are displayed reflects current usage:
    // CPLR, CPTB, CPFR, CPB, OPLR, OPFR, total cost.

    const SimpleCostVector &cplr = cost->getCplr();
    const SimpleCostVector &cptb = cost->getCpbcTotal();
    const SimpleCostVector &cpfr = cost->getCpfr();
    const SimpleCostVector &cpfb = cost->getCpbc1();
    //jo const SimpleCostVector &oplr = cost->getOplr();
    //jo const SimpleCostVector &opfr = cost->getOpfr();
    const SimpleCostVector &tc   = cost->getTotalCost();

    showSimpleCostVector("c.p.last row", cplr, prefix, out);
    //showSimpleCostVectorDetail(cplr, prefix, out);

    showSimpleCostVector("c.p.total blk", cptb, prefix, out);
    //showSimpleCostVectorDetail(cptb, prefix, out);

    showSimpleCostVector("c.p.first row", cpfr, prefix, out);
    //showSimpleCostVectorDetail(cpfr, prefix, out);

    showSimpleCostVector("c.p.first blk.", cpfb, prefix, out);
    //showSimpleCostVectorDetail(cpfb, prefix, out);

    //jo showSimpleCostVector("o.p. last row.", oplr, prefix, out);
    //jo showSimpleCostVectorDetail(oplr, prefix, out);

    //jo showSimpleCostVector("o.p. first row.", opfr, prefix, out);
    //jo showSimpleCostVectorDetail(opfr, prefix, out);

    showSimpleCostVector("total cost", tc, prefix, out);
    //showSimpleCostVectorDetail(tc, prefix, out);
  }
}
void OptDebug::showSimpleCostVector( const char* header,
                                     const SimpleCostVector &scv,
                                     const char *prefix,
                                     ostream &out )
{
  // showing all the five cost scalars of the given
  // Simple Cost Vector.

  out << prefix << header
      << "("      << scv.getDetailDesc(DF_OFF)
      << ")" << endl;
}

// ---------------------------------------------------------------------
// OptDebug::showSimpleCostVectorDetail()
// Show simple cost vector detail for this operator (node).
// ---------------------------------------------------------------------

void OptDebug::showSimpleCostVectorDetail( const SimpleCostVector &scv,
                                           const char *prefix,
                                           ostream &out )
{
  out << scv.getDetailDesc(DF_ON, prefix) << endl;
}

// ---------------------------------------------------------------------
// OptDebug::showTask()
// Show current task.
// ---------------------------------------------------------------------

void OptDebug::showTask( Int32 pass, Int32 groupId, Int32 task_count,
                         const CascadesTask *task,
                         const ExprNode *tree,
                         const CascadesPlan *plan,
                         const char *prefix )
{
  ostream &out = stream();

  NAString indent (prefix, CmpCommon::statementHeap());
  indent += "  ";

  out << prefix << "Task: " << task_count
	            << " ParentTask: " << task->getParentTaskId()
	            << " Pass: " << pass << " GroupId: " << groupId ;

  out << indent.data() << task->taskText().data() << endl;

  // the full task list will be printed if corresponding CQD is ON.
  // This is to reduce output when debugging.

  if ( CmpCommon::getDefault( NSK_DBG_PRINT_TASK_STACK ) == DF_ON )
  {
    CascadesTask *currTask = task->getNextTask();
    while ( currTask != NULL )
    {
      out << indent.data() << currTask->taskText().data() << endl;
      currTask = currTask->getNextTask();
    }
  }
}

void OptDebug::showMemoStats(CascadesMemo *memo,
                             const char *prefix,
                             ostream &out)
{
	CascadesGroup *groupPtr;
	Lng32 maxg = memo->getCountOfUsedGroups();
	Lng32 logExprCnt=0, physExprCnt=0, grPlanCnt, grFailedPlanCnt,
		 planCnt=0, failedPlanCnt=0, contextCnt=0;
	NABoolean detailedStat = (getenv("MEMO_DETAILED_STAT") != NULL);
        NABoolean planMonitorStat = (getenv("PLAN_MONITOR_STAT") != NULL);
        if ( planMonitorStat )
          out << "PLMON, group,plan,task,timer,count,gdcnt,expr,cptr" << endl;
        char childrenGroup[30];
        char contextPtr[7 + sizeof(void *)];
        CascadesPlan * curPlan;
        RelExpr * curExpr;
        NAString outStr;

	for (Lng32 i=0; i<maxg; i++)
	{
          groupPtr = (*memo)[i];
	  logExprCnt += groupPtr->getCountOfLogicalExpr();
	  physExprCnt += groupPtr->getCountOfPhysicalExpr();
	  planCnt += groupPtr->getCountOfPlans();
	  contextCnt += groupPtr->getCountOfContexts();
	  if (detailedStat)
	  {
            const CascadesPlanList& grPlanList = groupPtr->getPlans();
	    grPlanCnt = grPlanList.entries();
	    grFailedPlanCnt=0;

	    for (Lng32 j=0; j<grPlanCnt; j++)
            {
              curPlan = grPlanList[j];

              if ( curPlan->getRollUpCost() == NULL )
	        grFailedPlanCnt++;
              if ( planMonitorStat)
              {
                curExpr = curPlan->getPhysicalExpr();
                outStr = curExpr->getText();
                if ( curExpr->getArity() == 1 )
                  snprintf(childrenGroup, sizeof(childrenGroup), "(%d),", (curExpr->child(0)).getGroupId() );
                else if (curExpr->getArity() > 1 )
                  snprintf(childrenGroup, sizeof(childrenGroup), "(%d,%d),", (curExpr->child(0)).getGroupId(),
                                      (curExpr->child(1)).getGroupId() );

                //childrenGroup[11]='\0';

                outStr += childrenGroup;
                sprintf(contextPtr,"cptr=%p,",curPlan->getContext());
                contextPtr[6 + sizeof(void *)]='\0';
                outStr += contextPtr;

                out << "PLMON," << i << "," << j << "," << curPlan->lastTaskId_ << ","
                    << curPlan->planMonitor_.timer() << ","
                    << curPlan->planMonitor_.count() << ","
                    << curPlan->planMonitor_.goodcount() << "," << outStr  << endl;
              }
            }
            failedPlanCnt += grFailedPlanCnt;

            out << prefix << "\tMemo Group: \t" << i
              << "\tContexts: \t" << groupPtr->getCountOfContexts()
	      << "\tPlans: \t" << grPlanCnt
	      << "\tFailed Plans: \t" << grFailedPlanCnt << endl;
	  }
	}

	out << prefix << "\tMemo Groups: \t" << maxg
	  << "\n\tMemo LogExprs: \t" << logExprCnt
	  << "\n\tMemo PhyExprs: \t" << physExprCnt
	  << "\n\tMemo Plans: \t" << planCnt
	  << "\n\tMemo Plans: \t" << planCnt;
	if (detailedStat)
          out << "\tFailed Plans: " << failedPlanCnt ;
	out << "\n\tMemo Contexts: \t" << contextCnt << endl;

}

// New optimization driver
RelExpr *RelExpr::optimize2()
{
  // The constructor intialize the common optimization environment
  // The destructor reset the common optimization environment
  QueryOptimizerDriver driver(this);

  DefaultToken level = CmpCommon::getDefault(OPTIMIZATION_LEVEL);
  if (level == DF_MINIMUM) {
    Context * context = driver.initializeOptimization(this);
    return driver.doPass1Only(this, context);
  }

  else {

    // initalizeOptimization() may modifie relExpr, so save a copy
    RelExpr * original = copyRelExprTree(CmpCommon::statementHeap());
    Context * context = driver.initializeOptimization(this);

    if ( CURRSTMT_OPTDEFAULTS->needShortPassOptimization() ||
	 (CmpCommon::getDefault(FORCE_PASS_ONE) == DF_ON) ) {
      return driver.doPass1Pass2(this, context);
    }

    else {
      return driver.doPass2PerhapsPass1(this, context, original);
    }
  }
}

// class QueryOptimizerDriver Implementation
QueryOptimizerDriver::QueryOptimizerDriver(RelExpr *relExpr) :
  fatalExceptionOccurring_(FALSE),
  isStream_(FALSE),
  taskCount_(0),
  taskCountHold_(-1)
{
  isCQS_ = (ActiveControlDB()->getRequiredShape() &&
	    ActiveControlDB()->getRequiredShape()->getShape() &&
	    NOT ActiveControlDB()->getRequiredShape()->getShape()->isCutOp());

  // prunningIsFeasible is global
  if ( (CmpCommon::getDefault(COMP_BOOL_9) == DF_OFF)  AND
       ( relExpr->child(0)->getOperatorType() == REL_DDL OR
	 ((RelRoot *)relExpr)->accessOptions().isUpdateOrDelete() OR
	 ((RelRoot *)relExpr)->isRootOfInternalRefresh()
       )
     )
    CURRSTMT_OPTGLOBALS->pruningIsFeasible = FALSE;
  else
    // pruning can be forced by setting CDQ COMP_BOOL_9 to ON
    CURRSTMT_OPTGLOBALS->pruningIsFeasible = TRUE;

  CURRSTMT_OPTGLOBALS->forceParallelInsertSelect =
    (CmpCommon::getDefault(FORCE_PARALLEL_INSERT_SELECT) == DF_ON) AND
    ((RelRoot *)relExpr)->accessOptions().isUpdateOrDelete();

  if (((CmpCommon::getDefault(COMP_BOOL_69) == DF_OFF)  AND
       ( relExpr->child(0)->getOperatorType() == REL_DDL OR
         ((RelRoot *)relExpr)->isRootOfInternalRefresh()))
      )
    CURRSTMT_OPTGLOBALS->maxParIsFeasible = FALSE;
  else
    // maximum parallelism can be forced by setting CDQ COMP_BOOL_69 to ON
    CURRSTMT_OPTGLOBALS->maxParIsFeasible = TRUE;

  if (CmpCommon::getDefault(OPTIMIZER_PRINT_INTERNAL_COUNTERS) == DF_OFF)
    printCounters_ = FALSE;
  else
    printCounters_ = NOT (CURRSTMT_OPTGLOBALS->BeSilent);
}

QueryOptimizerDriver::~QueryOptimizerDriver()
{
  resetOptimization();

  // If the query contains an AntiSemiJoin (or embedded delete),
  // then we added the
  // JoinToTSJ rule to the pass 1 rule set. Need to remove it
  // now so it won't be enabled in pass1 for the next query.
  if (CURRSTMT_OPTDEFAULTS->isJoinToTSJRuleNeededOnPass1())
    {
      GlobalRuleSet->disable(JoinToTSJRuleNumber,
			     GlobalRuleSet->getFirstPassNumber(),
			     GlobalRuleSet->getFirstPassNumber());
    }


  // QSTUFF
  // we want to remove potential warnings which we create while
  // searching for an acceptable plan, e.g. generated when searching
  // for a suitable index to satisfy an order by clause on a stream
  if (isStream_ && NOT fatalExceptionOccurring_)
    clearFailedPlanWarningsForStream();
}

void QueryOptimizerDriver::resetOptimization()
{
  // turn off any CQSWA pointers
  CURRENTSTMT->clearCqsWA();
  // Unbind context-heap rules' substitute-expressions from
  // statement-heap groupAttrs, to prevent invalid dangling pointers
  // when optimizing the next query.
  ReinitRuleSet(GlobalRuleSet);
}

Context * QueryOptimizerDriver::initializeOptimization(RelExpr *relExpr)
{
  CURRSTMT_OPTDEFAULTS->resetMemoExprCount();
  CURRSTMT_OPTDEFAULTS->resetCurrentTask();

  CURRSTMT_OPTGLOBALS->memo = new (CmpCommon::statementHeap()) CascadesMemo();
  CURRSTMT_OPTGLOBALS->task_list = new (CmpCommon::statementHeap()) CascadesTaskList();

  DEBUG_GUI_SET_POINTERS();

  // initialize global counters
  CURRSTMT_OPTGLOBALS->group_merge_count     = 0;
  CURRSTMT_OPTGLOBALS->garbage_expr_count    = 0;
  CURRSTMT_OPTGLOBALS->pruned_tasks_count    = 0;
  CURRSTMT_OPTGLOBALS->duplicate_expr_count  = 0;
  CURRSTMT_OPTGLOBALS->logical_expr_count = 0;
  CURRSTMT_OPTGLOBALS->physical_expr_count  = 0;
  CURRSTMT_OPTGLOBALS->plans_count          = 0;
  CURRSTMT_OPTGLOBALS->asm_count = 0;
  CURRSTMT_OPTGLOBALS->cascade_count = 0;
  CURRSTMT_OPTGLOBALS->warningGiven = FALSE;

  CostScalar::initOvflwCount(0);
  CostScalar::initUdflwCount(0);

  initializeMonitors();

  DEBUG_RESET_PWSCOUNT();

  // -- Triggers.
  // Collect the access set information and save it in the Trigger backbone.
  relExpr->calcAccessSets(CmpCommon::statementHeap());

  relExpr->checkAndReorderTree();

  // QSTUFF
  // in case of an embedded update we must pushdown the characteristic
  // output values of the generic update root to its descendants to ensure
  // that only those output values are checked against indexes when the
  // index in case of a stream. Since in the future we may also want to
  // allow for unions we push it into every subset and subsequently follow
  // the isEmbeddedUpdate() thread down to their respective children

  if (relExpr->getGroupAttr()->isStream() &&
      relExpr->getGroupAttr()->isEmbeddedUpdateOrDelete()){
    ValueIdSet dummy;
    dummy.clear();
    relExpr->pushDownGenericUpdateRootOutputs(dummy);
  }
  // QSTUFF

  // estimate resources required for this query
  RequiredResources * reqdRsrcs = CURRSTMT_OPTDEFAULTS->estimateRequiredResources(relExpr);

  // Copy initial query into CascadesMemo
  NABoolean duplicateExpr = FALSE;
  NABoolean groupMerge = FALSE;
  CascadesGroupId root =
    CURRSTMT_OPTGLOBALS->memo->include(relExpr,
                  duplicateExpr,
		  groupMerge)->getGroupId();

  // Initialize optimization defaults
  CURRSTMT_OPTDEFAULTS->initialize(relExpr);

  GlobalRuleSet->initializeFirstPass();

  // If the query contains an AntiSemiJoin, (or embedded delete)
  // then we need to add the JoinToTSJ rule to the pass 1 rule set to
  // ensure that we will get a pass1 plan, since Hash Join does not
  // currently work for ASJ (or embedded delete)
  if (CURRSTMT_OPTDEFAULTS->isJoinToTSJRuleNeededOnPass1())
    GlobalRuleSet->enable(JoinToTSJRuleNumber);

  // Create initial context for the optimization of the top level group
  // This was earlier being initialized in a manner similar to
  // GLOBAL_EMPTY_INPUT_LOGPROP. They are now initialized equal to
  // GLOBAL_... to maintain uniformity.
  Context *context =  (*CURRSTMT_OPTGLOBALS->memo)[root]->shareContext(NULL, /*rpp*/
						  NULL,
						  NULL, /*cost limit*/
						  NULL,
						  (*GLOBAL_EMPTY_INPUT_LOGPROP));
  isStream_ = context->getGroupAttr()->isStream();
  return context;
}


RelExpr *QueryOptimizerDriver::doPass1Only(RelExpr *relExpr, Context *context)
{

  try{
    GlobalRuleSet->setCurrentPassNumber(1);
    optimizeAPass(context);
  }
  catch(AssertException & e){
    // Pass one assertion, so give up.
    markFatalAndGenErrorNoPlan(relExpr, context);
    PassOneAssertFatalException(e.getCondition(),
				e.getFileName(),
				e.getLineNum()).throwException();
    CURRSTMT_OPTDEFAULTS->setCurrentTask(NULL);
  }

  RelExpr *plan = context->bindSolutionTree(FALSE); /* use current solution */

  if(plan == NULL){
    // Pass one produces no plan, so give up, at least for now.
    // Future extension could recover by trying different rule sets.
    markFatalAndGenErrorNoPlan(relExpr, context);
    PassOneNoPlanFatalException(__FILE__, __LINE__).throwException();
  }
  return plan;
}

RelExpr *QueryOptimizerDriver::doPass1Pass2(RelExpr *relExpr, Context *context)
{

  try{
    GlobalRuleSet->setCurrentPassNumber(1);
    optimizeAPass(context);
  }
  catch(AssertException & e){
    // Pass one assertion, so give up.
    markFatalAndGenErrorNoPlan(relExpr, context);
    PassOneAssertFatalException(e.getCondition(),
				e.getFileName(),
				e.getLineNum()).throwException();
  }

  // Some heuristics in the optimizer changes the total pass number
  // during the optimization pass one so that the pass two is not tried.
  // Check if this is true
  if(GlobalRuleSet->inLastPass()){
    RelExpr *plan = context->bindSolutionTree(FALSE); /* use current solution */

    if(plan == NULL){
      // Pass one produces no plan, so give up, at least for now.
      // Future extension could recover by trying different rule sets.
      markFatalAndGenErrorNoPlan(relExpr, context);
      PassOneNoPlanFatalException(__FILE__, __LINE__).throwException();
    }
    return plan;
  }

  // setPreviousSolution() returns TRUE if there is plan for the solution
  // returns FALSE otherwise
  NABoolean pass1HasPlan  = context->setPreviousSolution();
  NABoolean forcePass2 = (CmpCommon::getDefault(FORCE_PASS_TWO) == DF_ON);

  if(NOT forcePass2 && NOT isCQS_ && NOT pass1HasPlan) {
    // Pass one produces no plan, no force of pass 2, and no CQS, so give up
    // CQS may cause no plan produced in pass one, so need to try pass2 if CQS
    markFatalAndGenErrorNoPlan(relExpr, context);
    PassOneNoPlanFatalException(__FILE__, __LINE__).throwException();
  }

  // Start pass two

  try{  
    GlobalRuleSet->setCurrentPassNumber(2);
    setupPassTwoHeuristics(context);
    optimizeAPass(context);
  }
  catch(AssertException & e){
    // Pass two assertion, so return the saved pass one solution.
    // Two Sub AssertException could be supported, and different
    // catches can be added to recover differently.
    // PassTwoAssertException : public AssertException
    // PassTwoNoPlanException : public AssertException
    // These two exceptions condition could determined at a very low
    // level and thrown to give up earlier.
    genWarnAssertPassTwo(e);
    return bindSavedPass1Solution(relExpr, context);
  }

  // part of fix to genesis case 10-090520-0766, soln 10-090521-1772 where
  // pass2 exploration is cut short by random pruning and we end up with a
  // more expensive pass2 plan instead of a cheaper, better pass1 plan.
  if (pass1HasPlan && context->getSolution()) {
    const Cost * pass2Cost = context->getSolution()->getRollUpCost();
    const Cost * pass1Cost = context->getPreviousSolution()->getRollUpCost();

    COMPARE_RESULT costComparison = pass2Cost->compareCosts
      (*pass1Cost,context->getReqdPhysicalProperty());
    if (costComparison == MORE)
      // there is a pass1 plan and it's cheaper.
      return bindSavedPass1Solution(relExpr, context);
  }

  RelExpr *plan = context->bindSolutionTree(FALSE); /* use current solution */

  if(plan == NULL){
    // Pass two produces no plan, return the saved pass one solution
    genWarnNoPlanPassTwo();
    return bindSavedPass1Solution(relExpr, context);
  }
  return plan;
}

// Helper function, can only be called from doPass1Pass2()
RelExpr * QueryOptimizerDriver::bindSavedPass1Solution(RelExpr *relExpr,
						 Context *context)
{
  RelExpr *plan = context->bindSolutionTree(TRUE); /* use previous solution */
  // Check below is probably not needed, because we have tested
  // setPreviousSoltuion() earlier.
  // No harm to have some additional safety.
  if(plan == NULL){
    // Pass one produces no plan, so give up
    markFatalAndGenErrorNoPlan(relExpr, context);
    PassOneNoPlanFatalException(__FILE__, __LINE__).throwException();
  }
  return plan;
}

RelExpr * QueryOptimizerDriver::doPass2PerhapsPass1(RelExpr *relExpr,
					      Context *context,
					      RelExpr *original)
{
  try{
    GlobalRuleSet->setCurrentPassNumber(2);
    setupPassTwoHeuristics(context);
    optimizeAPass(context);
  }
  catch(AssertException & e){
    // Pass two assertion, so return the saved pass one solution.
    // Two Sub AssertException could be supported, and different
    // catches can be added to recover differently.
    // PassTwoAssertException : public AssertException
    // PassTwoNoPlanException : public AssertException
    // These two exceptions condition could determined at a very low
    // level and thrown to give up earlier.
    genWarnAssertPassTwo(e);
    resetOptimization();
    context = initializeOptimization(original);
    return doPass1Only(original, context);
  }

  RelExpr *plan = context->bindSolutionTree(FALSE); /* use current solution */

  if(plan == NULL){

    NABoolean tryPassOne =
      (CmpCommon::getDefault(TRY_PASS_ONE_IF_PASS_TWO_FAILS) == DF_ON);

    if(tryPassOne) {
      // Pass two produces no plan, we try pass one.
      genWarnNoPlanPassTwo();
      resetOptimization();
      context = initializeOptimization(original);
      return doPass1Only(original, context);
    }
    else {
      // temporarily disable trying pass one due to current bugs
      markFatalAndGenErrorNoPlan(relExpr, context);
      PassOneSkippedPassTwoNoPlanFatalException(__FILE__, __LINE__).
	throwException();
    }
  }

  return plan;
}

void QueryOptimizerDriver::setupPassTwoHeuristics(Context *context)
{
  if (CURRSTMT_OPTDEFAULTS->optimizerPruning()) {

    CostScalar forcedPass2costLimit = ActiveSchemaDB()->
      getDefaults().getAsDouble(OPH_PRUNING_PASS2_COST_LIMIT);

    if ( forcedPass2costLimit > csZero )
      ((ElapsedTimeCostLimit *)(context->getCostLimit()))->
	setUpperLimit(forcedPass2costLimit);
  }
}

void QueryOptimizerDriver::optimizeAPass(Context *context)
{
  try{
    CURRSTMT_OPTDEFAULTS->setCurrentTask(NULL);
    // check if we have enough memory for this optimization
    // pass, if this is a pass > 1
    NABoolean hasEnoughMemory =
      ((GlobalRuleSet->getCurrentPassNumber() < 2) ||
       (CURRSTMT_OPTDEFAULTS->getMemUsed() <
        ((Lng32)(CURRSTMT_OPTDEFAULTS->getMemUsageSafetyNet() *
         CURRSTMT_OPTDEFAULTS->getMemUsageOptPassFactor()))));

    if (!hasEnoughMemory)
    {
      // Warning: insufficient memory for pass 2.
      if(CmpCommon::getDefault(SHOWWARN_OPT) == DF_ON)
        *CmpCommon::diags() << DgSqlCode(6020);
    }
      CMPASSERT(hasEnoughMemory);
      optimizeAPassHelper(context);
    }
    catch(AssertException & e) {
      // decorate the AssertException with taskCount_ etc and pass
      OptAssertException(e, taskCount_).throwException();
      CURRSTMT_OPTDEFAULTS->setCurrentTask(NULL);
    }
  }

  void QueryOptimizerDriver::optimizeAPassHelper(Context *context)
  {
    char taskCountStr[100], newTaskCountStr[100];
    Lng32 taskThreshold = CURRSTMT_OPTDEFAULTS->memUsageTaskThreshold();
    Lng32 taskInterval  = CURRSTMT_OPTDEFAULTS->memUsageTaskInterval();
    Lng32 taskCountBegin;
    short stride = 0;

    sprintf(taskCountStr, "%d OptTasks", taskCount_);
    taskCountBegin = taskCount_;
    MonitorMemoryUsage_Enter(taskCountStr, NULL, TRUE);

    if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor()) {
      (*CURRSTMT_OPTGLOBALS->cascadesPassMonitor).enter();
    }

    // start optimization, push initial task
    CURRSTMT_OPTGLOBALS->task_list->push(new(CmpCommon::statementHeap())
                    OptimizeGroupTask(context,
                                      NULL, /*guidance*/
                                      FALSE,
                                      0, ++stride));

    while (NOT CURRSTMT_OPTGLOBALS->task_list->empty())
      {
        CURRSTMT_OPTDEFAULTS->setTaskCount(++ taskCount_);

        DEBUG_BREAK_ON_TASK();

        if((taskCount_ >= taskThreshold) &&
           ((taskCount_ - taskThreshold)%taskInterval) == 0)
        {
          sprintf(newTaskCountStr, "%d-%d OptTasks", taskCountBegin, taskCount_);
          MonitorMemoryUsage_Exit(taskCountStr, NULL, newTaskCountStr, TRUE);

          sprintf(taskCountStr, "%d OptTasks", taskCount_);
          taskCountBegin = taskCount_;
          MonitorMemoryUsage_Enter(taskCountStr, NULL, TRUE);
        }

        // A sneak preview
        CascadesTask * next_task = CURRSTMT_OPTGLOBALS->task_list->getFirst();

        DEBUG_GUI_DO_MEMO_STEP(next_task);
        DEBUG_SHOW_TASK(next_task);

        // start the next task
        next_task = CURRSTMT_OPTGLOBALS->task_list->pop();
        CascadesTask::cascadesTaskTypeEnum
          next_task_type = next_task->taskType();

        if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor())
          (*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[next_task_type]).enter();

        // remember current & previous task (for debugging & tracking)
        CascadesTask* prev = CURRSTMT_OPTDEFAULTS->getCurrentTask();
        CURRSTMT_OPTDEFAULTS->setCurrentTask(next_task);

        next_task->perform(taskCount_);  // tasks destroy themselves

        // previous task becomes current task (for debugging & tracking)
        CURRSTMT_OPTDEFAULTS->setCurrentTask(prev);

        TEST_ERROR_HANDLING();

        if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor())
          (*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[next_task_type]).exit();

      } // while task list is not empty

    if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor()) {
      (*CURRSTMT_OPTGLOBALS->cascadesPassMonitor).exit();
    }

    sprintf(newTaskCountStr, "%d-%d OptTasks", taskCountBegin, taskCount_);
    MonitorMemoryUsage_Exit(taskCountStr, NULL, newTaskCountStr, TRUE);

    DEBUG_GUI_HIDE_QUERY_TREE();
    DEBUG_GUI_DISPLAY_AFTER_OPTIMIZATION(context);
    DEBUG_PRINT_COUNTERS();
    DEBUG_SHOW_PLAN(context);
  }

  void QueryOptimizerDriver::markFatalAndGenErrorNoPlan(RelExpr *relExpr,
                                                   Context *context)
  {
    fatalExceptionOccurring_ = TRUE;
    genErrorNoPlan(relExpr, context);
  }

  void QueryOptimizerDriver::genWarnAssertPassTwo(AssertException & e)
  {
    // arkcmpErrorAfterPassOne is a bad name. The message is a warning.
    if(CmpCommon::getDefault(SHOWWARN_OPT) == DF_ON)
    *CmpCommon::diags() << DgSqlCode(arkcmpErrorAfterPassOne)
			<< DgString0(e.getCondition())
			<< DgString1(e.getFileName())
			<< DgInt0((Lng32)e.getLineNum());
}

void QueryOptimizerDriver::genWarnNoPlanPassTwo()
{
  if(CmpCommon::getDefault(SHOWWARN_OPT) == DF_ON) {
    // TBD
    // *CmpCommon::diags() << DgSqlCode(arkcmpNoPlanPassTwo);
  }
}

void QueryOptimizerDriver::genErrorNoPlan(RelExpr *relExpr, Context *context)
{
  if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
  {
    // QSTUFF
    if (relExpr->getOperatorType() == REL_ROOT) {

      RelRoot * root = (RelRoot *) relExpr;
      if (root->isTrueRoot() && (!(root->reqdOrder().isEmpty()))) {

	if (context->getGroupAttr()->isStream()){
	  *CmpCommon::diags() << DgSqlCode(4153);
	}
	if (context->getGroupAttr()->isEmbeddedUpdateOrDelete()) {
	  *CmpCommon::diags() << DgSqlCode(4154)
			      << (relExpr->getGroupAttr()->isEmbeddedUpdate() ?
				  DgString0("update"): DgString0("delete"));
	}

      }

    }
    // QSTUFF

    // First we look to see if there is a CQS in effect
    if (isCQS_)
    {
      // Check the optimization level
      if (CURRSTMT_OPTDEFAULTS->optLevel() == OptDefaults::MINIMUM)
      {
        *CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileWithMinimumAndCQS);
      }
      else if (CURRSTMT_OPTDEFAULTS->randomPruningOccured())
      {
        *CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileWithMediumAndCQS);
      }
      else
      {
        *CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileWithCQS);
      }
    }

    // check for an extract query
    if (relExpr->getOperatorType() == REL_ROOT &&
        ((RelRoot *) relExpr)->getNumExtractStreams() > 0)
      {
        // raise a general error stating that an extract plan could
        // not be produced
        *CmpCommon::diags() << DgSqlCode(-7004);
      }

    // check for TMUDFs with a compiler interface
    if (CmpCommon::statement()->getTMUDFRefusedRequirements())
      {
        // a TMUDF refused to satisfy some required properties during
        // compilation, altert the user to that possible cause
        const LIST(const NAString *) *reasonList =
          CmpCommon::statement()->getDetailsOnRefusedRequirements();

        // add up to 5 possible reasons to the diags area
        for (CollIndex i=0; i<reasonList->entries() && i < 5; i++)
          *CmpCommon::diags() << DgSqlCode(-11153)
                              << DgString0((*reasonList)[i]->data());
      }

    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
    {
      // there is no Pub/Sub, CQS, extract or TMUDF involved

      // Check the optimization level
      if (CURRSTMT_OPTDEFAULTS->optLevel() == OptDefaults::MINIMUM)
      {
        *CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileWithMinimum);
      }
      else if (CmpCommon::diags()->getNumber(DgSqlCode::WARNING_) != 0)
      {
	// Direct the user to the warnings in the diags area.
	*CmpCommon::diags() << DgSqlCode(arkcmpUnableToCompileSeeWarning);
      }
    } // if (isCQS_) else ...

  } // if there are no errors in the diags area

}

void QueryOptimizerDriver::DEBUG_BREAK_ON_TASK()
{
#ifdef _DEBUG
  // this is to facilitate debugging in Visual inspect
  // during debugging we would set taskCountHold_ to the task
  // number we want to stop at.
  if(taskCount_ == taskCountHold_) {
    DebugBreak();
    taskCountHold_++;
  }
#endif
}

void QueryOptimizerDriver::DEBUG_SHOW_TASK(CascadesTask * task)
{
  if ( ( CmpCommon::getDefault( NSK_DBG ) == DF_ON ) &&
       ( CmpCommon::getDefault( NSK_DBG_PRINT_TASK ) == DF_ON ) )
    {
      CURRCONTEXT_OPTDEBUG->showTask( (Int32)GlobalRuleSet->getCurrentPassNumber(),
		       (Int32) task->getGroupId(),
		       taskCount_,
		       task,
		       task->getExpr(),
		       task->getPlan(),
		       " " );
    }
}

void QueryOptimizerDriver::DEBUG_GUI_DO_MEMO_STEP(CascadesTask *task)
{
#ifdef NA_DEBUG_GUI
  if (CmpMain::msGui_ && CURRENTSTMT->displayGraph()) {
    CmpMain::pExpFuncs_->
      fpDoMemoStep( (Int32)GlobalRuleSet->getCurrentPassNumber(),
		    (Int32) task->getGroupId(),
		    taskCount_,
		    task,
		    task->getExpr(),
		    task->getPlan());
  }
#endif
}

void QueryOptimizerDriver::DEBUG_GUI_SET_POINTERS()
{
#ifdef NA_DEBUG_GUI
  CMPASSERT(gpClusterInfo != NULL);
  if (CmpMain::msGui_ && CURRENTSTMT->displayGraph() )
    CmpMain::pExpFuncs_->fpSqldbgSetCmpPointers(
         CURRSTMT_OPTGLOBALS->memo,
         CURRSTMT_OPTGLOBALS->task_list,
         QueryAnalysis::Instance(),
         cmpCurrentContext,
         gpClusterInfo);
#endif
}

void QueryOptimizerDriver::
DEBUG_GUI_DISPLAY_TRIGGERS_AFTER_NORMALIZATION(RelExpr *relExpr)
{
#ifdef _DEBUG
#ifdef NA_DEBUG_GUI
  // debug for trigger transformation only if there are triggers
  if (relExpr->getOperatorType() == REL_ROOT &&
      ((RelRoot *)relExpr)->getTriggersList() != NULL)
    CmpMain::guiDisplay(Sqlcmpdbg::AFTER_NORMALIZATION, relExpr);
#endif
#endif
}

void QueryOptimizerDriver::DEBUG_GUI_HIDE_QUERY_TREE()
{
#ifdef NA_DEBUG_GUI
  //---------------------------------------------------------------------
  // GSH : Hide the Sqlcmpdbg display if it is not hidden. This is possible
  // if you were steping through the memo optimization.
  //---------------------------------------------------------------------
  if (CmpMain::msGui_ && CURRENTSTMT->displayGraph()) {
    CmpMain::pExpFuncs_->fpHideQueryTree(TRUE);
  }
#endif
}

void QueryOptimizerDriver::
DEBUG_GUI_DISPLAY_AFTER_OPTIMIZATION(Context *context)
{
#ifdef NA_DEBUG_GUI
  // Display optimal plan at the end of each optimization pass
  Sqlcmpdbg::CompilationPhase optPass = Sqlcmpdbg::AFTER_OPT1;
  switch (GlobalRuleSet->getCurrentPassNumber()) {
  case 1:	optPass = Sqlcmpdbg::AFTER_OPT1; break;
  case 2:	optPass = Sqlcmpdbg::AFTER_OPT2; break;
  default:	break;
  }

  if (CmpMain::msGui_ && CURRENTSTMT->displayGraph())
    {
      CMPASSERT(gpClusterInfo != NULL);
      CmpMain::pExpFuncs_->fpSqldbgSetCmpPointers(
           CURRSTMT_OPTGLOBALS->memo,
           CURRSTMT_OPTGLOBALS->task_list,
           QueryAnalysis::Instance(),
           cmpCurrentContext,
           gpClusterInfo);

      CmpMain::pExpFuncs_->fpDisplayQueryTree( optPass, NULL,
					       (void*) context->getSolution());
    }

#endif
}

void QueryOptimizerDriver::DEBUG_PRINT_COUNTERS()
{
#ifdef _DEBUG
  if (printCounters_)
    {
      char string[200];
      printf ("\n");
      *CmpCommon::diags() << DgSqlCode(arkcmpOptimizerCountersWarning);
      sprintf(string,
	      "pass %d complete:\n",
	      GlobalRuleSet->getCurrentPassNumber());
      report(string);
      *CmpCommon::diags() << DgString0(string);
      sprintf(string,
	      " %d groups, %d tasks, %d rules,\n",
	      CURRSTMT_OPTGLOBALS->memo->getCountOfUsedGroups(),
	      taskCount_,
	      GlobalRuleSet->getRuleApplCount());
      report(string);
      *CmpCommon::diags() << DgString1(string);
      sprintf(string,
	      "%d groups merged, %d expr. cleaned up, %d tasks pruned\n",
	      CURRSTMT_OPTGLOBALS->group_merge_count,
	      CURRSTMT_OPTGLOBALS->garbage_expr_count,
	      CURRSTMT_OPTGLOBALS->pruned_tasks_count);
      report(string);
      *CmpCommon::diags() << DgString2(string);
      sprintf(string,
	      "%d/%d/%d/%d log/phys/plans/dupl expressions in CascadesMemo\n",
	      CURRSTMT_OPTGLOBALS->logical_expr_count,
	      CURRSTMT_OPTGLOBALS->physical_expr_count,
	      CURRSTMT_OPTGLOBALS->plans_count,
	      CURRSTMT_OPTGLOBALS->duplicate_expr_count);
      report(string);
      *CmpCommon::diags() << DgString3(string);
      sprintf(string,
	      "%d asm hits and %d asm misses\n",
	      CURRSTMT_OPTGLOBALS->asm_count,CURRSTMT_OPTGLOBALS->cascade_count);
      report(string);
      *CmpCommon::diags() << DgString4(string);
    }
#endif
}

void QueryOptimizerDriver::DEBUG_RESET_PWSCOUNT()
{
#ifdef DEBUG
  //  Reset count of PlanWorkSpace objects for each statement.
  PlanWorkSpace::resetPwsCount();

#endif
}

void QueryOptimizerDriver::DEBUG_SHOW_PLAN(Context *context)
{
  if ( context->getSolution() &&
       ( CmpCommon::getDefault( NSK_DBG ) == DF_ON ) )
    {
      Int32 passNumber = GlobalRuleSet->getCurrentPassNumber();
      if ( ( passNumber == 1 &&
	     CmpCommon::getDefault( NSK_DBG_SHOW_PASS1_PLAN ) == DF_ON )
	   OR
	   ( passNumber == 2 &&
	     CmpCommon::getDefault( NSK_DBG_SHOW_PASS2_PLAN ) == DF_ON ) )
	{
	  CURRCONTEXT_OPTDEBUG->stream() << endl << "Plan chosen after Pass "
			  << passNumber << endl;
	  CURRCONTEXT_OPTDEBUG->showTree( context->getSolution()->getPhysicalExpr(),
			   context->getSolution(),
			   "  " );
	  CURRCONTEXT_OPTDEBUG->stream() << endl;
	}
    }

#ifdef _DEBUG
  if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor() &&
      (CmpCommon::getDefault(COMPILE_TIME_MONITOR_LOG_ALLTIME_ONLY) == DF_OFF))
    {
      const char * fname = getCOMPILE_TIME_MONITOR_OUTPUT_FILEname();
      if (fname)
	CURRCONTEXT_OPTDEBUG->showMemoStats(CURRSTMT_OPTGLOBALS->memo, " ", CURRCONTEXT_OPTDEBUG->stream());
      else
	CURRCONTEXT_OPTDEBUG->showMemoStats(CURRSTMT_OPTGLOBALS->memo, " ", cout);
    }
#endif
}

void QueryOptimizerDriver::initializeMonitors()
{
  for (Int32 ti=0; ti<CascadesTask::NUMBER_OF_TASK_TYPES; ti++)
    (*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[ti]).init(0);
  (*CURRSTMT_OPTGLOBALS->cascadesPassMonitor).init(0);
  if (CURRSTMT_OPTDEFAULTS->optimizerHeuristic2()) {
    (*CURRSTMT_OPTGLOBALS->fileScanMonitor).init(0);
    (*CURRSTMT_OPTGLOBALS->nestedJoinMonitor).init(0);
    (*CURRSTMT_OPTGLOBALS->indexJoinMonitor).init(0);
    (*CURRSTMT_OPTGLOBALS->asynchrMonitor).init(0);
    (*CURRSTMT_OPTGLOBALS->singleSubsetCostMonitor).init(0);
    (*CURRSTMT_OPTGLOBALS->singleVectorCostMonitor).init(0);
    (*CURRSTMT_OPTGLOBALS->singleObjectCostMonitor).init(0);
    (*CURRSTMT_OPTGLOBALS->multSubsetCostMonitor).init(0);
    (*CURRSTMT_OPTGLOBALS->multVectorCostMonitor).init(0);
    (*CURRSTMT_OPTGLOBALS->multObjectCostMonitor).init(0);
  }
}


void QueryOptimizerDriver::TEST_ERROR_HANDLING()
{
#ifdef _DEBUG
  if (GlobalRuleSet->getCurrentPassNumber() == 1) {

    Lng32 assertTaskPass1 =
      CmpCommon::getDefaultLong(TEST_PASS_ONE_ASSERT_TASK_NUMBER);

    if(assertTaskPass1 == taskCount_) {

      CMPASSERT(FALSE);
    }
  }
  else if (GlobalRuleSet->getCurrentPassNumber() == 2) {

    Lng32 assertTaskPass2 =
      CmpCommon::getDefaultLong(TEST_PASS_TWO_ASSERT_TASK_NUMBER);

    if(assertTaskPass2 == taskCount_) {

      CMPASSERT(FALSE);
    }
  }
#endif
}

#define OPT_QSORT_THRESHOLD 8

static void opt_qsort_shortsort(char *lo, char *hi, UInt32 width, Int32 (*comp)(const void *, const void *));
static void opt_qsort_swap(char *p, char *q, UInt32 width);

void opt_qsort(void *base, UInt32 num, UInt32 width, Int32 (*comp)(const void *, const void *))
{
  char *lo, *hi;
  char *mid;
  char *loguy, *higuy;
  UInt32 size;
  char *lostk[30], *histk[30];
  Int32 stkptr;

  if (num < 2 || width == 0) return;
  stkptr = 0;

  lo = (char *) base;
  hi = (char *) base + width * (num - 1);

recurse:
  size = (hi - lo) / width + 1;

  if (size <= OPT_QSORT_THRESHOLD)
  {
    opt_qsort_shortsort(lo, hi, width, comp);
  }
  else
  {
    mid = lo + (size / 2) * width;
    opt_qsort_swap(mid, lo, width);

    loguy = lo;
    higuy = hi + width;

    for (;;)
    {
      do { loguy += width; } while (loguy <= hi && comp(loguy, lo) <= 0);
      do { higuy -= width; } while (higuy > lo && comp(higuy, lo) >= 0);
      if (higuy < loguy) break;
      opt_qsort_swap(loguy, higuy, width);
    }

    opt_qsort_swap(lo, higuy, width);

    if (higuy - 1 - lo >= hi - loguy)
    {
      if (lo + width < higuy)
      {
        lostk[stkptr] = lo;
        histk[stkptr] = higuy - width;
        ++stkptr;
      }

      if (loguy < hi)
      {
        lo = loguy;
        goto recurse;
      }
    }
    else
    {
      if (loguy < hi)
      {
        lostk[stkptr] = loguy;
        histk[stkptr] = hi;
        ++stkptr;
      }

      if (lo + width < higuy)
      {
        hi = higuy - width;
        goto recurse;
      }
    }
  }

  --stkptr;
  if (stkptr >= 0)
  {
    lo = lostk[stkptr];
    hi = histk[stkptr];
    goto recurse;
  }
  else
    return;
}

static void opt_qsort_shortsort(char *lo, char *hi, UInt32 width, Int32 (*comp)(const void *, const void *))
{
  char *p, *max;

  while (hi > lo)
  {
    max = lo;
    for (p = lo+width; p <= hi; p += width) if (comp(p, max) > 0) max = p;
    opt_qsort_swap(max, hi, width);
    hi -= width;
  }
}

static void opt_qsort_swap(char *a, char *b, UInt32 width)
{
  char tmp;

  if (a != b)
  {
    while (width--)
    {
      tmp = *a;
      *a++ = *b;
      *b++ = tmp;
    }
  }
}

double OptDefaults::riskPremiumSerial()
{
   ULng32 maxMaxCardThreshold = 
     (ActiveSchemaDB()->getDefaults()).
         getAsULong(RISK_PREMIUM_SERIAL_SCALEBACK_MAXCARD_THRESHOLD);

    if ( maxMaxCardinality_ <= maxMaxCardThreshold )
      return double(1);

    return riskPremiumSerial_;
}

OptGlobals::OptGlobals(NAHeap* heap)
   : heap_(heap)
{
   // -----------------------------------------------------------------------
   // counters for capturing optimizer statistics
   // -----------------------------------------------------------------------

   contextCounter = new (heap_) ObjectCounter();
   cascadesPlanCounter = new (heap_) ObjectCounter();

#ifdef DEBUG
  planWorkSpaceCount = 0;
#endif /* DEBUG */

   duplicate_expr_count = 0;

   logical_expr_count   = 0;
   physical_expr_count  = 0;
   plans_count          = 0;

   // ----------------------------------------------------------------
   // counters for evaluating ASM
   // -----------------------------------------------------------------
   asm_count = 0;
   cascade_count = 0;

 
   // TaskMonitor cascadesTasksMonitor[CascadesTask::NUMBER_OF_TASK_TYPES];
   for (Int32 i = 0; i < CascadesTask::NUMBER_OF_TASK_TYPES; i++)
     cascadesTasksMonitor[i] = new (heap_) TaskMonitor;

   cascadesPassMonitor = new (heap_) TaskMonitor;

   // These monitor were introduced to collect statistics about how
   // much time ScanOptimizer spent in most important functions
   // for costing and how many times those functions were called
   
   // First 3 Monitors collect statistics about fileScanOptimizer.optimize(...)
   // in costmethod. fileScanMonitor gives total statistics, nestedJoinMonitor
   // - inside nested joins, indexJoinMonitor - inside index joins
   
   fileScanMonitor = new (heap_) TaskMonitor;
   nestedJoinMonitor = new (heap_) TaskMonitor;
   indexJoinMonitor = new (heap_) TaskMonitor;
   
   // Next 3 Monitors collect statistics about computeCostForSingleSubset
   // in costmethod. singleSubsetMonitor gives total statistics,
   // singleVectorCostMonitor - inside computeCostVector,
   // singleObjectCostMonitor - inside computeCostObject
   
   singleObjectCostMonitor = new (heap_) TaskMonitor;
   singleSubsetCostMonitor = new (heap_) TaskMonitor;
   singleVectorCostMonitor = new (heap_) TaskMonitor;
   
   // Next 3 Monitors collect statistics about computeCostForMultipleSubset
   // in costmethod. multSubsetMonitor gives total statistics,
   // multVectorCostMonitor - inside computeCostVectorForMultipleSubset,
   // multObjectCostMonitor - inside computeCostObject (only to compute
   // final cost object, cases when computeCostObject is called to compute
   // intermediate cost to check the costBound are not counted).
   
   multObjectCostMonitor = new (heap_) TaskMonitor;
   multSubsetCostMonitor = new (heap_) TaskMonitor;
   multVectorCostMonitor = new (heap_) TaskMonitor;
   
   // This last monitor counts how many times a final cost object was
   // created for asynchronous access for both singleSubset and
   // multipleSubset. This is being counted in computeCostObject
   // function. To ignore all other calls (except final) for
   // computeCostObject synCheckFlag is used. (Inside computeCostFor
   // MultipleSubset computeCOstObject is called many times to
   // check costBound). It is set to TRUE right before the final call,
   // and - to FALSE right after it.
   
   asynchrMonitor = new (heap_) TaskMonitor;
   synCheckFlag = FALSE;
   
   // Controls DCMPASSERT delta < 0 for Linux
   warningGiven = FALSE;
   
   // -----------------------------------------------------------------------
   // command line options (initially set in sqlcomp.C)
   // -----------------------------------------------------------------------
   
   pruningIsFeasible = FALSE;
   maxParIsFeasible = FALSE;
   forceParallelInsertSelect = FALSE;

   // Setting this to FALSE makes the optimizer print out statistics
   BeSilent  = TRUE;
   
   // ----------------------------------------------------------------------
   // A  global output string for debugging
   // ----------------------------------------------------------------------
   
   returnString[0] = '\0';      // global output string for debugging
   
   // for debugging only
   group_merge_count  = 0;
   garbage_expr_count = 0;
   pruned_tasks_count = 0;
   countExpr          = FALSE;

   memo      = NULL;
   task_list = NULL;
}

// Does this Context provide an optimal solution?
// The Context provides an optimal solution, iff a solution is found
// during the current optimization pass whose cost is within the cost
// limit.
NABoolean Context::hasOptimalSolution() const
{
    return ( solution_ AND
             optimizedInCurrentPass() AND
             ( NOT costLimitExceeded_ OR
               CURRSTMT_OPTDEFAULTS->OPHuseFailedPlanCost()
             )
           );
}

NABoolean Context::isNiceContextEnabled() const
{
   return (niceContext_ AND CURRSTMT_OPTDEFAULTS->optimizerPruning())
           OR CURRSTMT_OPTDEFAULTS->OPHuseNiceContext();
}

