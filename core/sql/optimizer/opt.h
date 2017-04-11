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
#ifndef OPT_HDR
#define OPT_HDR
/* -*-C++-*-
******************************************************************************
*
* File:         opt.h
* Description:  from Cascades Optimizer search engine:
*               This file defines classes for all classes used in the
*               optimizer search engine, beyond those defined in the
*               interface classes.
* Created:      7/22/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------
#include "RelExpr.h"
#include "CmpCommon.h"
#include "CmpContext.h"
//<pb>
// -----------------------------------------------------------------------
// Classes defined in this file
// ============================
// The search memory is organized as a single structure called "memo"
// of class "CascadesMemo," which consists of an array of equivalence
// classes, implemented by "CascadesGroups",  plus a hash table for
// finding duplicate (logical) expressions.
//
// An equivalence class' primary content is a set of equivalent logical
// and physical expressions organized as a linked list. (Since an operator
// can be both logical and physical, a separation into multiple lists
// would be quite cumbersome.) In addition, an equivalence class contains
// a list of patterns that have been explored previously for that
// equivalence class.
//
// The search yet to be done is organized as tasks; there are six
// types of tasks that schedule each other.  Tasks yet to be done are
// organized in a LIFO stack, although alternative organizations and
// executions, including randomized and parallel executions, could be
// implemented.  A context class captures constraints for a task, in
// particular cost limits and required and excluded logical and
// physical properties.
//
// When applying a rule, multiple expressions might match the pattern
// given by the rule.  If so, they are bound one at a time; a specific
// binding in captured in a tree of "CascadesBinding" objects.
// -----------------------------------------------------------------------

class CascadesGroup;	     // an equivalence class of log/phys. expressions
class CascadesMemo;          // collection of equivalence classs, search space memory

class Context;               // optimization goal
class ContextList;           // a list of context pointers

class CascadesPlan;          // a plan
class CascadesPlanList;

class PlanWorkSpace;         // a class that is used for recording plan related info.

class CascadesTask;	     // task

class OptimizeGroupTask;     // specific tasks
class OptimizeExprTask;
class ExploreGroupTask;
class ExploreExprTask;
class ApplyRuleTask;
class CreatePlanTask;
class GarbageCollectionTask;

class CascadesTaskList;	     // LIFO stack of tasks to be done

class CascadesBinding;	     // for pattern matching
class OptDefaults;

class IndexDesc;
class RequirementGenerator;

class CmpStatement;

#include <fstream>

class SimpleCostVector;
class OptDebug;              // for debugging purposes

class PerformanceGoal;
class CostWeight;

// forward declaration
class QueryComplexityVector;
namespace tmudr {
  class UDRPlanInfo;
}

#ifdef _DEBUG
  #define DBGLOGMSG(str) \
  { if (CmpCommon::getDefault(NSK_DBG_SHOW_PLAN_LOG) == DF_ON ) \
     (*CURRCONTEXT_OPTDEBUG).stream() << str << endl; }
  #define DBGLOGMSGCNTXT(str,cntxt) \
  { if (CmpCommon::getDefault(NSK_DBG_SHOW_PLAN_LOG) == DF_ON ) \
     (*CURRCONTEXT_OPTDEBUG).stream() << str << (cntxt)->getRequirementsString().data() \
                     << "group " << (cntxt)->getGroupId() << endl; }
  #define DBGLOGPLAN(plan) \
  { if (CmpCommon::getDefault(NSK_DBG_SHOW_PLAN_LOG) == DF_ON ) \
      (*CURRCONTEXT_OPTDEBUG).showNode((plan)->getPhysicalExpr()->castToRelExpr(), \
                      (plan)," *** ", FALSE ); }
  #define DBGMJENUMRULE(message) \
  { if (CmpCommon::getDefault(NSK_DBG_SHOW_MJ_ENUM_RULE_LOG) == DF_ON ) \
      (*CURRCONTEXT_OPTDEBUG).showNode((plan)->getPhysicalExpr()->castToRelExpr(), \
                      (plan)," *** ", FALSE ); }
#else
  #define DBGLOGMSG(str)
  #define DBGLOGMSGCNTXT(str,cntxt)
  #define DBGLOGPLAN(plan)
#endif

// This is a double epsilon being defined to make sure the selectivity
// always has some minimum. We did not want to do it in DefaultValidator.h,
// as that would have been too general.
#define MIN_SELECTIVITY       (1e-16)

// ----------------------------------------------------------------------
// Will put this class in other place later
// A very fast random number sequence generator
// ----------------------------------------------------------------------
class RandomSequence
{
  private:
    Lng32 m_;
    Lng32 a_;
    Lng32 c_;
    Lng32 x_;

  public:

    RandomSequence()
    { initialize();}

    double random()
    {
      x_ = (x_*a_ + c_)%m_;
      return double(x_)/double(m_);
    }

    void initialize(Lng32 i=616)
    {
      m_=259200;       // magic numbers
      a_=7141;         // magic numbers
      c_=54773;        // magic numbers
      x_= i % m_;
    }
};

// ---------------------------------------------------------------
// This class is for caching any parameters initialized in a
// per-sql statement basis. This class gets initialized
// every time a sql statement is compiled (in
// RelExpr * RelExpr::optimize(Guidance * guidance,
//			    ReqdPhysicalProperty * rpp,
//			    CostLimit* costLimit)
// located in opt.cpp).
//
// It contains the information about the optimization
// level and heuristics cached from the Defaults table.
// It also contains the methods for optimization level 1 and tasks limit.
// Lastly, it contains cached recalibration defaults.
// ---------------------------------------------------------------

class OptDefaults
{
public:

  OptDefaults (CollHeap* heap);
  ~OptDefaults ();

  enum optLevelEnum
    {
      MINIMUM,
      LOW,
      MEDIUM_LOW,
      MEDIUM,
      MEDIUM_MAX,
      MAXIMUM,
      AGGRESSIVE
    };

  optLevelEnum optLevel() { return optLevel_;}

  optLevelEnum indexEliminationLevel()
  { return indexEliminationLevel_; }
  double mdamSelectionDefault()
  { return mdamSelectionDefault_; }
  double readAheadMaxBlocks()
  { return readAheadMaxBlocks_; }
  double acceptableInputEstLogPropError()
  { return acceptableInputEstLogPropError_; }

  inline void setTaskCount(Lng32 id){taskCount_ = id;};

  inline Lng32 getTaskCount(){ return taskCount_;};

  inline Lng32 level1SafetyNet() { return level1SafetyNet_; }

  inline double level1SafetyNetMultiple()
  {
    return level1SafetyNetMultiple_;
  };
  inline NABoolean taskLimitExceeded(Lng32 id)
  {
    return (id > optTaskLimit_);
  };

  inline void setEnumPotentialThreshold(Lng32 threshold)
  {
    enumPotentialThreshold_ = threshold;
  }

  inline Lng32 getEnumPotentialThreshold()
  {
    return enumPotentialThreshold_;
  }

  inline NABoolean needShortPassOptimization()
  {
    return ((queryComplexity_ >= shortOptPassThreshold_) OR
            (numTables_ >= 5));
  };
  inline ULng32 getNumOfBlocksPerAccess()
  {
    return numOfBlocksPerAccess_;
  };
  inline Int32 getNumTables()
  {
    return numTables_;
  };
  inline NABoolean isFakeHardware()
  {
    return fakeHardware_;
  };
  inline NABoolean isAdditiveResourceCosting()
  {
    return additiveResourceCosting_;
  };
  inline double getQueryComplexity()
  {
    return queryComplexity_;
  };
  void setQueryMJComplexity(double complexity);
  inline double getQueryMJComplexity()
  {
    return queryMJComplexity_;
  };
  inline NABoolean isComplexMJQuery()
  {
    return isComplexMJQuery_;
  }
  inline double getRequiredMemoryResourceEstimate()
  {
    return requiredMemoryResourceEstimate_;
  };
  inline double getRequiredCpuResourceEstimate()
  {
    return requiredCpuResourceEstimate_;
  };
  inline double getTotalDataAccessCost()
  {
    return totalDataAccessCost_;
  };  
  inline double getMaxOperatorMemoryEstimate()
  {
    return maxOperatorMemoryEstimate_;
  };
  inline double getMaxOperatorCpuEstimate()
  {
    return maxOperatorCpuEstimate_;
  };
  inline double getMaxOperatorDataAccessCost()
  {
    return maxOperatorDataAccessCost_;
  };
  inline double getMemoryPerCPU()
  {
    return memoryPerCPU_;
  };
  inline double getWorkPerCPU()
  {
    return workPerCPU_;
  };
  inline Lng32 getAdjustedDegreeOfParallelism()
  {
    return adjustedDegreeOfParallelism_;
  };
  inline Lng32 getDefaultDegreeOfParallelism()
  {
    return defaultDegreeOfParallelism_;
  };
  inline Lng32 getMaximumDegreeOfParallelism()
  {
    return maximumDegreeOfParallelism_;
  };
  inline Lng32 getMinimumESPParallelism()
  {
    return minimumESPParallelism_;
  };
  inline Lng32 getTotalNumberOfCPUs()
  {
    return totalNumberOfCPUs_;
  };
  inline NABoolean isJoinToTSJRuleNeededOnPass1()
  {
    if (CmpCommon::getDefault(COMP_BOOL_71) != DF_ON)
      return TRUE;
    return enableJoinToTSJRuleOnPass1_;
  };
  inline NABoolean areTriggersPresent()
  {
    return triggersPresent_;
  };
  inline NABoolean isCrossProductControlEnabled()
  {
    return enableCrossProductControl_;
  };
  inline NABoolean isNestedJoinControlEnabled()
  {
    return enableNestedJoinControl_;
  };
  inline NABoolean isZigZagControlEnabled()
  {
    return enableZigZagControl_;
  };
  inline NABoolean isMergeJoinControlEnabled()
  {
    return enableMergeJoinControl_;
  };
  inline NABoolean isOrderedHashJoinControlEnabled()
  {
    return enableOrderedHashJoinControl_;
  };
  inline NABoolean isNestedJoinConsidered()
  {
    return considerNestedJoin_;
  };
  inline NABoolean isHashJoinConsidered()
  {
    return considerHashJoin_;
  };
  inline NABoolean isHybridHashJoinConsidered()
  {
    return considerHybridHashJoin_;
  };
  inline NABoolean isOrderedHashJoinConsidered()
  {
    return considerOrderedHashJoin_;
  };
  inline NABoolean isMergeJoinConsidered()
  {
    return considerMergeJoin_;
  };
  inline DefaultToken isZigZagTreeConsidered()
  {
    return considerZigZagTree_;
  };
  inline NABoolean isMinMaxConsidered()
  {
	return considerMinMaxOpt_;
  };
  inline NABoolean nestedJoinForCrossProducts()
  {
    return nestedJoinForCrossProducts_;
  };

  inline NABoolean preferredProbingOrderForNJ()
  {
    return preferredProbingOrderForNJ_;
  };

  inline NABoolean orderedWritesForNJ()
  {
    return orderedWritesForNJ_;
  };

  inline NABoolean joinOrderByUser()
  {
    return joinOrderByUser_;
  };

  inline NABoolean ignoreExchangesInCQS()
  {
    return ignoreExchangesInCQS_;
  };

  inline NABoolean ignoreSortsInCQS()
  {
    return ignoreSortsInCQS_;
  };

  inline NABoolean dataFlowOptimization()
  {
    return dataFlowOptimization_;
  };

  inline NABoolean optimizerHeuristic1()
  {
    return optimizerHeuristic1_;
  };
  inline NABoolean optimizerHeuristic2()
  {
    return optimizerHeuristic2_;
  };
  inline NABoolean optimizerHeuristic3()
  {
    return optimizerHeuristic3_;
  };
  inline NABoolean optimizerHeuristic4()
  {
    return optimizerHeuristic4_;
  };
  inline NABoolean optimizerHeuristic5()
  {
    return optimizerHeuristic5_;
  };
  inline Lng32 getMJEnumLimit()
  {
    return level1MJEnumLimit_;
  };
  inline void setInitialMemory()
  {
    optInitialMemory_ = 0;

    if (CmpCommon::getDefault(COMP_BOOL_160) == DF_OFF)
    {
      optInitialMemory_ = (CmpCommon::statementHeap())->getAllocSize();
    }
  }
  inline Lng32 getMemUsed()
  {
    UInt32 currentMem = (CmpCommon::statementHeap())->getAllocSize();
    UInt32 memUsed = currentMem - MINOF(currentMem, (UInt32)optInitialMemory_);
    memUsed /= (1024*1024);
    return memUsed;
  }
  inline void setOriginalOptimizationBudget(double budget)
  {
    originalOptimizationBudget_ = budget;
  }
  inline double getOriginalOptimizationBudget()
  {
    return originalOptimizationBudget_;
  }
  inline void setOptimizationBudget(double budget)
  {
    optimizationBudget_ = budget;
  }
  inline double getOptimizationBudget()
  {
    return optimizationBudget_;
  }
  inline Lng32 maxDepthToCheckForCyclicPlan()
  {
    return maxDepthToCheckForCyclicPlan_;
  }
  inline Lng32 memUsageTaskThreshold()
  {
    return memUsageTaskThreshold_;
  }
  inline Lng32 memUsageTaskInterval()
  {
    return memUsageTaskInterval_;
  }
  inline Lng32 getMemUsageSafetyNet()
  {
    return memUsageSafetyNet_;
  }
  inline double getMemUsageOptPassFactor()
  {
    return memUsageOptPassFactor_;
  }
  inline double getMemUsageNiceContextFactor()
  {
    return memUsageNiceContextFactor_;
  }
  inline DefaultToken attemptESPParallelism()
  {
    return attemptESPParallelism_;
  };

  inline NABoolean maxParallelismIsFeasible()
  {
    return maxParallelismIsFeasible_;
  };

  inline NABoolean isOrOptimizationEnabled()
  {
    return enableOrOptimization_;
  };

  inline float numberOfPartitionsDeviation()
  {
    return numberOfPartitionsDeviation_;
  };
  inline double updatedBytesPerESP()
  {
    return updatedBytesPerESP_;
  };
  inline float numberOfRowsParallelThreshold()
  {
    return numberOfRowsParallelThreshold_;
  };
  inline NABoolean deviationType2JoinsSystem()
  {
    return deviationType2JoinsSystem_;
  };
  inline float numOfPartsDeviationType2Joins()
  {
    return numOfPartsDeviationType2Joins_;
  };
  inline NABoolean parallelHeuristic1()
  {
    return parallelHeuristic1_;
  };
  inline NABoolean parallelHeuristic2()
  {
    return parallelHeuristic2_;
  };
  inline NABoolean parallelHeuristic3()
  {
    return parallelHeuristic3_;
  };
  inline NABoolean parallelHeuristic4()
  {
    return parallelHeuristic4_;
  };
  inline NABoolean optimizerPruning()
  {
    return optimizerPruning_;
  };
  inline NABoolean OPHpruneWhenCLExceeded()
  {
    return OPHpruneWhenCLExceeded_;
  };
  inline NABoolean OPHreduceCLfromCandidates()
  {
    return OPHreduceCLfromCandidates_;
  };
  inline NABoolean OPHreduceCLfromPass1Solution()
  {
    return OPHreduceCLfromPass1Solution_;
  };
  inline NABoolean OPHreuseFailedPlan()
  {
    return OPHreuseFailedPlan_;
  };
  inline NABoolean OPHreuseOperatorCost()
  {
    return OPHreuseOperatorCost_;
  };
  inline NABoolean OPHskipOGTforSharedGCfailedCL()
  {
    return OPHskipOGTforSharedGCfailedCL_;
  };
  inline NABoolean OPHuseCandidatePlans()
  {
    return OPHuseCandidatePlans_;
  };
  inline NABoolean OPHuseCompCostThreshold()
  {
    return OPHuseCompCostThreshold_;
  };
  inline NABoolean OPHuseConservativeCL()
  {
    return OPHuseConservativeCL_;
  };
  inline NABoolean OPHuseEnforcerPlanPromotion()
  {
    return OPHuseEnforcerPlanPromotion_;
  };
  inline NABoolean OPHuseFailedPlanCost()
  {
    return OPHuseFailedPlanCost_;
  };
  inline NABoolean OPHuseNiceContext()
  {
    return OPHuseNiceContext_;
  };
  inline NABoolean OPHusePWSflagForContext()
  {
    return OPHusePWSflagForContext_;
  };
  inline NABoolean OPHuseCachedElapsedTime()
  {
    return OPHuseCachedElapsedTime_;
  };
  inline NABoolean OPHexitNJcrContChiLoop()
  {
    return OPHexitNJcrContChiLoop_;
  };
  inline NABoolean OPHexitMJcrContChiLoop()
  {
    return OPHexitMJcrContChiLoop_;
  };
  inline NABoolean OPHexitHJcrContChiLoop()
  {
    return OPHexitHJcrContChiLoop_;
  };
  inline NABoolean compileTimeMonitor()
  {
    return compileTimeMonitor_;
  };
  inline NABoolean reuseBasicCost()
  {
    return reuseBasicCost_;
  };
  inline void setReuseBasicCost(NABoolean v)
  {
    reuseBasicCost_ = v;
  };
  inline NABoolean randomPruningOccured()
  {
    return randomPruningOccured_;
  };
  inline NABoolean pushDownDP2Requested()
  {
    return pushDownDP2Requested_;
  };
  inline NABoolean reduceBaseHistograms()
  {
  	return reduceBaseHistograms_;
  };
  inline NABoolean reduceIntermediateHistograms()
  {
   	return reduceIntermediateHistograms_;
  };
  inline NABoolean preFetchHistograms()
  {
   	return preFetchHistograms_;
  };
  inline Int64 siKeyGCinterval()
  {
    return siKeyGCinterval_;
  };

  static NABoolean cacheHistograms();

  inline double joinCardLowBound()
  {
    return joinCardLowBound_;
  };
  inline NABoolean ustatAutomation()
  {
    return ustatAutomation_;
  };
  inline NABoolean histMCStatsNeeded()
  {
  	return histMCStatsNeeded_;
  };
  inline double histDefaultSampleSize()
  {
    return histDefaultSampleSize_;
  }
  inline void setHistDefaultSampleSize(double val)
  {
    histDefaultSampleSize_ = val;
  }
  inline NABoolean histSkipMCUecForNonKeyCols()
  {
  	return histSkipMCUecForNonKeyCols_;
  };
  inline Lng32 histMissingStatsWarningLevel()
  {
      return histMissingStatsWarningLevel_;
  }
  inline NABoolean histOptimisticCardOpt()
  {
    return histOptimisticCardOpt_;
  }
  inline ULng32 histTupleFreqValListThreshold()
  {
    return histTupleFreqValListThreshold_;
  }
  inline ULng32 histNumOfAddDaysToExtrapolate()
  {
    return histNumOfAddDaysToExtrapolate_;
  }
  inline NABoolean incorporateSkewInCosting()
  {
      return incorporateSkewInCosting_;
  }
  inline ULng32 maxSkewValuesDetected()
  {
	return maxSkewValuesDetected_;
  }
  inline double skewSensitivityThreshold()
  {
	return skewSensitivityThreshold_;
  }
  inline NABoolean histAssumeIndependentReduction()
  {
      return histAssumeIndependentReduction_;
  }
  inline NABoolean histUseSampleForCardEst()
  {
    return histUseSampleForCardEst_;
  }

  void setHistUseSampleForCardEst(NABoolean v)
  {
    histUseSampleForCardEst_ = v;
  }

  inline Lng32 partitioningSchemeSharing()
  {
    return partitioningSchemeSharing_;
  };
  inline double riskPremiumNJ()
  {
    return riskPremiumNJ_;
  };
  inline double riskPremiumMJ()
  {
    return riskPremiumMJ_;
  };
  double riskPremiumSerial() ;

  inline double maxMaxCardinality() { return maxMaxCardinality_; }

  inline double robustHjToNjFudgeFactor()
  {
    return robustHjToNjFudgeFactor_;
  };
  inline Lng32 robustSortGroupBy()
  {
    return robustSortGroupBy_;
  };
  inline DefaultToken robustQueryOptimization()
  {
    return robustQueryOptimization_;
  };

  inline double defSelForRangePred()
  {
    if (defSelForRangePred_ < MIN_SELECTIVITY)
      return MIN_SELECTIVITY ;
    else
      return defSelForRangePred_;
  };

  inline double defSelForWildCard()
  {
    if (defSelForWildCard_ < MIN_SELECTIVITY)
      return MIN_SELECTIVITY ;
    else
      return defSelForWildCard_;
  };

    inline double defSelForNoWildCard()
  {
    if (defSelForNoWildCard_ < MIN_SELECTIVITY)
      return MIN_SELECTIVITY ;
    else
      return defSelForNoWildCard_;
  };

  inline double defNoStatsUec()
  {
    return defNoStatsUec_;
  };

  inline void setNoStatsUec(double uec)
  {
    defNoStatsUec_ = uec;
  };

  inline double defNoStatsRowCount()
  {
    return defNoStatsRowCount_;
  };

  inline double baseHistogramReductionFF()
  {
    return baseHistogramReductionFF_;
  };
  inline double intermediateHistogramReductionFF()
  {
    return intermediateHistogramReductionFF_;
  };
  inline double histogramReductionConstantAlpha()
  {
    return histogramReductionConstantAlpha_;
  };

  NABoolean pruneByOptDefaults(Rule* rule, RelExpr* relExpr);
  NABoolean pruneByOptLevel(Rule* rule, RelExpr* relExpr);
  NABoolean pruneByRProbability(Rule* rule, RelExpr* relExpr);
  NABoolean pruneByPotential(Rule* rule, RelExpr* relExpr);
  Int32 optimizerGracefulTermination()
  {
    return optimizerGracefulTermination_;
  };

  RequiredResources * estimateRequiredResources(RelExpr* rootExpr);
  // initialize portion of the cache based on ActiveSchemaDB's optDefaults 
  // and the pass-in argument rootExpr
  void initialize(RelExpr* rootExpr);

  Int32 getRulePriority(Rule* rule);


  double getReductionToPushGBPastTSJ()
  {
    return reduction_to_push_gb_past_tsj_;
  };
  // -----------------------------------------------------------------------
  // Cost-related methods:
  // -----------------------------------------------------------------------

  // This method is needed by the GUI to initialize its
  // own copy of OptDefaults:
  // initialize all cost related members here!!
  void initializeCostInfo();

  double getTimePerCPUInstructions()
  { return calibratedMSCF_ET_CPU_;}

  double getTimePerSeqKb()
  { return calibratedMSCF_ET_IO_TRANSFER_;}

  double getTimePerSeek()
  { return calibratedMSCF_ET_NUM_IO_SEEKS_;}

  double getTimePerKbOfLocalMsg()
  { return calibratedMSCF_ET_LOCAL_MSG_TRANSFER_;}

  double getTimePerLocalMsg()
  { return calibratedMSCF_ET_NUM_LOCAL_MSGS_;}

  double getTimePerKbOfRemoteMsg()
  { return calibratedMSCF_ET_REMOTE_MSG_TRANSFER_;}

  double getTimePerRemoteMsg()
  { return calibratedMSCF_ET_NUM_REMOTE_MSGS_;}

  double getMemoryLimitPerCPU()
  { return calculatedMEMORY_LIMIT_PER_CPU_; }

  NABoolean useHighFreqInfo()
  { return useHighFreqInfo_; }

  inline void setMemoryLimitPerCPU(double memLimit)
  {calculatedMEMORY_LIMIT_PER_CPU_ = memLimit;}

  NABoolean useNewMdam();

  CascadesTask *getCurrentTask() { return currentTask_; }
  ULng32 getMemoExprCount() { return memoExprCount_; }

  void resetMemoExprCount() { memoExprCount_ = 0; }
  void resetCurrentTask() { currentTask_ = NULL; }

  // --------------------------------------
  // Mutators:
  // --------------------------------------
  void recalibrateCPU();
  void recalibrateIOSeqTransfer();
  void recalibrateIOSeeks();
  void recalibrateLocalMsg();
  void recalibrateLocalMsgTransfer();
  void recalibrateRemoteMsg();
  void recalibrateRemoteMsgTransfer();
  double recalibrate(Int32 calEnum
                            ,Int32 baseEnum, Int32 endEnum);


  NABoolean newMemoryLimit(RelExpr * rootExpr,
                                  NABoolean recompute );

  NABoolean isRuleDisabled(ULng32 ruleBitPosition);

  void setCurrentTask( CascadesTask *t ) { currentTask_ = t; }
  ULng32 updateGetMemoExprCount() { return ++memoExprCount_; }

  // Query Strategizer Params
  // used for explain
  // BEGIN
  NABoolean useStrategizer() {return useStrategizer_; }
  double getCpuCost() { return cpuCost_; };
  double getScanCost() { return scanCost_; };
  double getTotalCost() { return (cpuCost_ + scanCost_); };
  double getBudgetFactor() { return budgetFactor_; };
  double getPass1Tasks() { return pass1Tasks_; };
  double getTaskFactor() { return taskFactor_; };
  double getNComplexity() {return nComplexity_; };
  double get2NComplexity() { return n_2Complexity_; };
  double getN2Complexity() { return n2Complexity_; };
  double getN3Complexity() { return n3Complexity_; };
  double getExhaustiveComplexity() { return exhaustiveComplexity_; };
  
  void setCpuCost(double cpuCost) { cpuCost_ = cpuCost; };
  void setScanCost(double scanCost) { scanCost_ = scanCost; };
  void setBudgetFactor(double budgetFactor) { budgetFactor_ = budgetFactor; };
  void setPass1Tasks(double pass1Tasks) { pass1Tasks_ = pass1Tasks; };
  void setTaskFactor(double taskFactor) { taskFactor_ = taskFactor; };
  void setNComplexity(double nComplexity) { nComplexity_ = nComplexity; };
  void set2NComplexity(double n_2Complexity) { n_2Complexity_ = n_2Complexity; };
  void setN2Complexity(double n2Complexity) { n2Complexity_ = n2Complexity; };
  void setN3Complexity(double n3Complexity) { n3Complexity_ = n3Complexity; };
  void setExhaustiveComplexity(double exhaustiveComplexity) { exhaustiveComplexity_ = exhaustiveComplexity; };
  // END

  Lng32 getRequiredESPs() { return requiredESPs_; }
  void setRequiredESPs(Lng32 x) { requiredESPs_ = x; }

  const IndexDesc* getRequiredScanDescForFastDelete() 
              { return requiredScanDescForFastDelete_; }
  void setRequiredScanDescForFastDelete(const IndexDesc* x) 
              { requiredScanDescForFastDelete_ = x; }

  NABoolean isSideTreeInsert() { return isSideTreeInsert_; }
  void setSideTreeInsert(NABoolean x) { isSideTreeInsert_ = x; }

  double computeRecommendedNumCPUs(double cpuResourceRequired);
  double computeRecommendedNumCPUsForMemory(double memoryResourceRequired);

  // -----------------------------------------------------------------------
  // default weighing factors for cost: make the total value a multiple
  // of random disk I/Os (normalize all values to 20 ms and add them up).
  // Space consumption (memory and temp disk files) doesn't count.
  // -----------------------------------------------------------------------
  CostWeight* getDefaultCostWeight() const { return defaultCostWeight_; }
  PerformanceGoal* getDefaultPerformanceGoal() const { return defaultPerformanceGoal_; }
  PerformanceGoal* getResourcePerformanceGoal() const { return resourcePerformanceGoal_; }


protected:
   NABoolean InitCostVariables();
   void CleanupCostVariables();

private:
  optLevelEnum optLevel_;
  optLevelEnum indexEliminationLevel_;
  double mdamSelectionDefault_;
  double readAheadMaxBlocks_;
  double acceptableInputEstLogPropError_;

  Lng32 taskCount_;
  Lng32 optTaskLimit_;
  Lng32 enumPotentialThreshold_;

  // following is a bit map that controls
  // which rules should fire and which rules should
  // not fire, the meaning of each bit is as below
  // 1 - join commutativity
  // 2 - join left shift
  // 3 - push gb below join
  // 4 - push partial gb below tsj
  // 5 - split gb
  // 6 - hash gb
  // 7 - sort gb
  // The numbers above have to be passed in as parameter
  // to isRuleDisabled()
  ULng32 optRulesGuidance_;
  Lng32 level1Constant1_;
  Lng32 level1Constant2_;
  Lng32 level1ImmunityLimit_;
  Lng32 level1MJEnumLimit_;
  ULng32 numOfBlocksPerAccess_;
  Int32  numTables_;
  double queryComplexity_;
  double queryMJComplexity_;
  NABoolean isComplexMJQuery_;
  double requiredMemoryResourceEstimate_;
  double requiredCpuResourceEstimate_;
  double totalDataAccessCost_;
  double maxOperatorMemoryEstimate_;
  double maxOperatorCpuEstimate_;
  double maxOperatorDataAccessCost_;
  double memoryPerCPU_;
  double workPerCPU_;
  Lng32 adjustedDegreeOfParallelism_;
  Lng32 defaultDegreeOfParallelism_;
  Lng32 maximumDegreeOfParallelism_;
  Lng32 minimumESPParallelism_;
  Lng32 totalNumberOfCPUs_;
  NABoolean enableJoinToTSJRuleOnPass1_;
  NABoolean triggersPresent_;
  double reduction_to_push_gb_past_tsj_;
  NABoolean enableCrossProductControl_;
  NABoolean enableNestedJoinControl_;
  NABoolean enableZigZagControl_;
  NABoolean enableMergeJoinControl_;
  NABoolean enableOrderedHashJoinControl_;
  NABoolean considerNestedJoin_;
  NABoolean considerHashJoin_;
  NABoolean considerOrderedHashJoin_;
  NABoolean considerHybridHashJoin_;
  NABoolean considerMergeJoin_;
  DefaultToken considerZigZagTree_;
  NABoolean considerMinMaxOpt_;
  NABoolean preferredProbingOrderForNJ_;
  NABoolean orderedWritesForNJ_;
  NABoolean nestedJoinForCrossProducts_;
  NABoolean joinOrderByUser_;
  NABoolean ignoreExchangesInCQS_;
  NABoolean ignoreSortsInCQS_;
  NABoolean dataFlowOptimization_;
  NABoolean optimizerGracefulTermination_;
  NABoolean optimizerHeuristic1_;
  NABoolean optimizerHeuristic2_;
  NABoolean optimizerHeuristic3_;
  NABoolean optimizerHeuristic4_;
  NABoolean optimizerHeuristic5_;
  DefaultToken attemptESPParallelism_;
  NABoolean maxParallelismIsFeasible_;
  NABoolean enableOrOptimization_;
  float numberOfPartitionsDeviation_;
  double updatedBytesPerESP_;
  float numberOfRowsParallelThreshold_;
  float numOfPartsDeviationType2Joins_;
  NABoolean deviationType2JoinsSystem_;
  NABoolean parallelHeuristic1_;
  NABoolean parallelHeuristic2_;
  NABoolean parallelHeuristic3_;
  NABoolean parallelHeuristic4_;
  NABoolean compileTimeMonitor_;
  NABoolean reuseBasicCost_;
  NABoolean randomPruningOccured_;

  NABoolean optimizerPruning_;
  NABoolean OPHpruneWhenCLExceeded_;
  NABoolean OPHreduceCLfromCandidates_;
  NABoolean OPHreduceCLfromPass1Solution_;
  NABoolean OPHreuseFailedPlan_;
  NABoolean OPHreuseOperatorCost_;
  NABoolean OPHskipOGTforSharedGCfailedCL_;
  NABoolean OPHuseCandidatePlans_;
  NABoolean OPHuseCompCostThreshold_;
  NABoolean OPHuseConservativeCL_;
  NABoolean OPHuseEnforcerPlanPromotion_;
  NABoolean OPHuseFailedPlanCost_;
  NABoolean OPHuseNiceContext_;
  NABoolean OPHusePWSflagForContext_;
  NABoolean OPHuseCachedElapsedTime_;
  NABoolean OPHexitNJcrContChiLoop_;
  NABoolean OPHexitMJcrContChiLoop_;
  NABoolean OPHexitHJcrContChiLoop_;

  RandomSequence *ranSeq_;
  NABoolean pushDownDP2Requested_;
  Lng32 shortOptPassThreshold_;
  Lng32 level1Threshold_;
  Lng32 level1SafetyNet_;
  double level1SafetyNetMultiple_;
  Lng32 optInitialMemory_;
  double originalOptimizationBudget_;
  double optimizationBudget_;
  Lng32 maxDepthToCheckForCyclicPlan_;
  Lng32 memUsageTaskThreshold_;
  Lng32 memUsageTaskInterval_;
  Lng32 memUsageSafetyNet_;
  double memUsageOptPassFactor_;
  double memUsageNiceContextFactor_;
  NABoolean fakeHardware_;
  NABoolean additiveResourceCosting_;
  // ------------------------------------------------------------------------
  // opt defaults for histogram intervals reduction, histogram caching and
  // histogram preFetching
  // ------------------------------------------------------------------------
  NABoolean reduceBaseHistograms_;
  NABoolean reduceIntermediateHistograms_;
  NABoolean preFetchHistograms_;
  Int64 siKeyGCinterval_;  // query/security invalidation key garbage collection interval, in seconds

  NABoolean ustatAutomation_;
  double histDefaultSampleSize_;
  double baseHistogramReductionFF_;
  double intermediateHistogramReductionFF_;
  double histogramReductionConstantAlpha_;
  NABoolean histMCStatsNeeded_;
  NABoolean histSkipMCUecForNonKeyCols_;
  Lng32 histMissingStatsWarningLevel_;
  Lng32 histOptimisticCardOpt_;
  NABoolean incorporateSkewInCosting_;
  NABoolean histAssumeIndependentReduction_;
  NABoolean histUseSampleForCardEst_;
  ULng32 maxSkewValuesDetected_;
  double skewSensitivityThreshold_;
  NABoolean useHighFreqInfo_;
  ULng32 histTupleFreqValListThreshold_;
  ULng32 histNumOfAddDaysToExtrapolate_;

  double joinCardLowBound_;
  double defNoStatsUec_;
  double defNoStatsRowCount_;
  double defSelForWildCard_;
  double defSelForNoWildCard_;
  double defSelForRangePred_;

  Lng32 partitioningSchemeSharing_;
  double riskPremiumNJ_;
  double riskPremiumMJ_;
  double riskPremiumSerial_;
  double robustHjToNjFudgeFactor_;
  Lng32 robustSortGroupBy_;
  DefaultToken robustQueryOptimization_;

  double maxMaxCardinality_;

  // -----------------------------------------------------------------------
  // The following are the resource-to-time multipliers that were
  // recalibrated for the current SQL statement
  // -----------------------------------------------------------------------
  NADefaults *defs_;

  double calibratedMSCF_ET_CPU_;

  double calibratedMSCF_ET_IO_TRANSFER_;

  double calibratedMSCF_ET_NUM_IO_SEEKS_;

  double calibratedMSCF_ET_LOCAL_MSG_TRANSFER_;

  double calibratedMSCF_ET_NUM_LOCAL_MSGS_;

  double calibratedMSCF_ET_REMOTE_MSG_TRANSFER_;

  double calibratedMSCF_ET_NUM_REMOTE_MSGS_;

  // ------------------------------------------------------------------------
  // The memory_limit is calculated for each query based on the table sizes
  //  after local predicates have been applied, unless the user entered
  //  a specific memory limit defualt.
  // ------------------------------------------------------------------------
  double calculatedMEMORY_LIMIT_PER_CPU_;
  NABoolean useNewMdam_;

  CascadesTask *currentTask_;
  ULng32 memoExprCount_;
  
  // Query Strategizer Params
  // used for explain
  // BEGIN
  NABoolean useStrategizer_;
  double cpuCost_;
  double scanCost_;
  double budgetFactor_;
  double pass1Tasks_;
  double taskFactor_;
  double nComplexity_;
  double n_2Complexity_;
  double n2Complexity_;
  double n3Complexity_;
  double exhaustiveComplexity_; 
  // END

  // requred ESPs and scan indexDesc involved - for fast delete used in parallel
  // purge data
  Lng32 requiredESPs_;
              
  const IndexDesc* requiredScanDescForFastDelete_;

  NABoolean isSideTreeInsert_;

  CostWeight* defaultCostWeight_;
  PerformanceGoal* defaultPerformanceGoal_;
  PerformanceGoal* resourcePerformanceGoal_;

  CollHeap* heap_;
}; // class OptDefaults

//<pb>
// -----------------------------------------------------------------------
// Optimization procedure
// ======================
// This is the only run-time interface from the DBMS to the optimizer,
// although there are many interfaces from the optimizer to
// DBI-supplied methods.  All operators in the input tree, called the
// "query," must be logical operators.  All operators in the final
// output, called the "plan", will be physical operators.  If the
// optimizer is not able to produce a plan, typically because there
// aren't sufficient physical operators or implementation rules, this
// procedure will return NULL.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Plan
// ====
// A CascadesPlan identifies a physical expression that represents the
// query execution plan for a given Context, for a given optimization
// pass.
// It contains the synthesized Physical Property for the physical
// expression as well as its Cost. It also contains a RuleSubset
// that indicates the set of Rules that have been applied.
// -----------------------------------------------------------------------
class CascadesPlan : public ReferenceCounter
{
public:
  // ---------------------------------------------------------------------
  // Constructor and Destructor.
  // ---------------------------------------------------------------------
// warning elimination (removed "inline")
  CascadesPlan(RelExpr * physExpr, Context * context = NULL);

  virtual ~CascadesPlan();

  // ---------------------------------------------------------------------
  // Accessor functions.
  // ---------------------------------------------------------------------
  inline RelExpr * getPhysicalExpr() const           { return physExpr_; }

  inline PhysicalProperty * getPhysicalProperty() const
                                                     { return physProp_; }
  inline const Cost * getOperatorCost() const
  { return operatorCost_; }

  inline const Cost * getRollUpCost() const
  { return rollUpCost_; }

  inline CostScalar getPlanElapsedTime() { return planElapsedTime_; }
  inline void setPlanElapsedTime(ElapsedTime et) { planElapsedTime_ = et; }

  inline NABoolean exceededCostLimit() { return planExceededCostLimit_; }
  inline void setExceededCostLimit() { planExceededCostLimit_ = TRUE; }

  inline Context * getContext() const                 { return context_; }

  // Get the pass numberwhen the plan was created
  inline Lng32 getCreationPassNumber() const
  { return passNoWhenCreated_; }

  // returns TRUE if the plan succeeded in the current pass, else
  // it returns FALSE
  inline NABoolean succeededInCurrentPass() const
  { return (succeededInPassNo_ == GlobalRuleSet->getCurrentPassNumber()); }

  // returns TRUE if the plan failed in the current pass, else
  // it returns FALSE
  inline NABoolean failedInCurrentPass() const
  { return (failedInPassNo_ == GlobalRuleSet->getCurrentPassNumber()); }

  inline Context * getContextForChild(Lng32 childIndex) const
         { if ( childIndex >= Lng32 (childContexts_.entries()) )
             return NULL;
           else
             return childContexts_[childIndex]; }

  inline NABoolean isBigMemoryOperator() const          { return isBMO_; }

  // ---------------------------------------------------------------------
  // Mutator functions.
  // ---------------------------------------------------------------------
  inline void setPhysicalProperty(PhysicalProperty *ppv)
  { physProp_ = ppv; }

  void setOperatorCost(Cost *cost);
  void setRollUpCost(Cost *cost);

  inline void setContextForChild(Lng32 childIndex, Context *context)
  { childContexts_.insertAt(childIndex, context); }

  inline void setSuccessInCurrentPass()
  { succeededInPassNo_ = GlobalRuleSet->getCurrentPassNumber(); }

  inline void setFailedInCurrentPass()
  { failedInPassNo_ = GlobalRuleSet->getCurrentPassNumber(); }

  inline void setBigMemoryOperator(NABoolean isBMO)     { isBMO_ = isBMO; }

  // ---------------------------------------------------------------------
  // Methods for accessing solutions from the child contexts
  // ---------------------------------------------------------------------
  const CascadesPlan * getSolutionForChild (Lng32 childIndex) const;

  const PhysicalProperty * getPhysicalPropertyForChild (Lng32 childIndex) const;

  const Cost * getCostForChild(Lng32 childIndex) const;

  NABoolean exprOccursInChildTree(RelExpr *newExpr,
                                  Int32 maxDepth=1) const;

  // ---------------------------------------------------------------------
  // Utility methods
  // ---------------------------------------------------------------------
  // for tracking the time of plan creation
  TaskMonitor planMonitor_;
  Lng32 lastTaskId_;

  // GUI display method (defined in DisplayTree.C)
  void displayTree();
//<pb>
private:

  // ---------------------------------------------------------------------
  // The physical expression to be optimized.
  // ---------------------------------------------------------------------
  RelExpr * physExpr_;

  // ---------------------------------------------------------------------
  // The physical properties for this plan.
  // ---------------------------------------------------------------------
  PhysicalProperty * physProp_;

  // ---------------------------------------------------------------------
  // Does this plan belong to a big memory operator?
  // ---------------------------------------------------------------------
  NABoolean isBMO_;

  // ---------------------------------------------------------------------
  // Cost for this operator independent of its children.  Based initially
  // on an estimated degree of parallelism and possibly revised if actual
  // degree of parallelism differs from estimate.
  // ---------------------------------------------------------------------
  Cost * operatorCost_;

  // ---------------------------------------------------------------------
  // Cost for the sub-plan rooted at this operator (operator cost combined
  // with the operator's children cost)
  // ---------------------------------------------------------------------
  Cost * rollUpCost_;

  // This is cached value of plan elapsed time. It is used to speed up
  // comparison with costLimit when  OPH_CONSERVATIVE_COST_LIMIT is OFF
  // the value is set as a side effect of the first compareWithPlanCost
  // call with this plan as an argument.
  ElapsedTime planElapsedTime_;

  // Used by pruning heuristic to reuse failed plan cost. Flag is set to
  // TRUE when failed because its partial or rollUp cost exceeded cost
  // limit. Flag is passed to context to keep track about plans exceeding
  // cost limit in context local flag currentPlanExceededCostLimit_
  NABoolean planExceededCostLimit_;

  // ---------------------------------------------------------------------
  // The context for which this plan was created.
  // ---------------------------------------------------------------------
  Context * context_;

  // ---------------------------------------------------------------------
  // The optimization pass number for which this plan was created
  // ---------------------------------------------------------------------
  const Lng32 passNoWhenCreated_;

  // ---------------------------------------------------------------------
  // The optimization pass number for which this plan last succeeded:
  // ---------------------------------------------------------------------
  Lng32 succeededInPassNo_;

  // ---------------------------------------------------------------------
  // The optimization pass number for which this plan last failed:
  // ---------------------------------------------------------------------
  Lng32 failedInPassNo_;

  // ---------------------------------------------------------------------
  // The optimization goal for each child of the given physical expression
  // that is created using the given context.
  // ---------------------------------------------------------------------
  ARRAY(Context *) childContexts_; // for phys. Operators

}; // CascadesPlan

// -----------------------------------------------------------------------
// Just for convenience: a list collection of CascadesPlan pointers
// -----------------------------------------------------------------------

class CascadesPlanList : public LIST(CascadesPlan *)
{
public:
  CascadesPlanList(CollHeap* h/*=0*/) : LIST(CascadesPlan *)(h)
    {  }
};
//<pb>
// -----------------------------------------------------------------------
// Optimization context
// ====================
// A context describes an optimization goal: required and excluded
// physical properties and a cost limit for the optimization of a
// certain equivalence class.
//
// The context also contains information about the potential solutions
// (candidates) that have been found during optimization. After the
// optimization is complete, the context contains either a solution or a
// NULL pointer to indicate failure.
//
// A context is typically created by an OptimizeGroupTask task and then
// shared by multiple follow-up tasks that deal with optimizing the
// equivalence class:
// - OptimizeExprTask tasks for all logical expressions in the
//   equivalence class at the time the OptimizeGroupTask task performs
//   (unless OptimizeGroupTask finds the optimal solution without
//    creating other tasks)
// - ApplyRuleTask tasks created by the OptimizeExprTask tasks
// - CreatePlanTask tasks created by the ApplyRuleTask tasks when
//   applying implementation rules
// - OptimizeExprTask tasks created by the ApplyRuleTask tasks when
//   applying transformation rules
// -----------------------------------------------------------------------

#pragma nowarn(1506)   // warning elimination
class Context : public NABasicObject
{

public:

  // constructor (do NOT use this constructor, use CascadesGroup::shareContext()
  // or ExprGroupId::shareContext()!!!!!)
  Context (CascadesGroupId equivClassId,
	   const ReqdPhysicalProperty * const reqdPhys,
           const InputPhysicalProperty* const inputPhys,
	   const EstLogPropSharedPtr& inputLogProp);

  virtual ~Context ();

  // ---------------------------------------------------------------------
  // Accessor methods used by the optimizer outside the Cascades
  // search engine.
  // ---------------------------------------------------------------------

  inline const ReqdPhysicalProperty * getReqdPhysicalProperty() const
                                                     { return reqdPhys_; }

  inline const InputPhysicalProperty* getInputPhysicalProperty() const
  { return inputPhys_; }

  NABoolean requiresOrder() const;
  NABoolean requiresPartitioning() const;
  NABoolean requiresSpecificLocation() const;

  inline CostLimit* getCostLimit() const            { return costLimit_; }

  inline CascadesPlan * getPlan() const           { return currentPlan_; }

  inline const PhysicalProperty *getPhysicalPropertyForSolution() const
  {
    if (solution_)
      return solution_->getPhysicalProperty();
    else
      return NULL;
  }

  inline const Cost *getCostForSolution() const
  {
    if (solution_)
      return solution_->getRollUpCost();
    else
      return NULL;
  }

  const RelExpr *
  getPhysicalExprOfSolutionForChild(Lng32 childIndex) const;

  const PhysicalProperty * getPhysicalPropertyOfSolutionForChild
                              (Lng32 childIndex) const;

  const Cost * getCostOfSolutionForChild(Lng32 childIndex) const;

  const RelExpr *
  getPhysicalExprOfSolutionForGrandChild(Lng32 childIndex,
                                         Lng32 grandChildIndex) const;

  const PhysicalProperty * getPhysicalPropertyOfSolutionForGrandChild
                              (Lng32 childIndex,
                               Lng32 grandChildIndex) const;

  // ---------------------------------------------------------------------
  // Mutator methods used by the optimizer outside the Cascades
  // search engine.
  // ---------------------------------------------------------------------

  // --- Methods on Cost Limit

  void setCostLimit(CostLimit* c);

  void setCostLimitExceeded() { costLimitExceeded_ = TRUE; }

  // is the current cost limit a feasible one for creating a plan?
  NABoolean isCostLimitFeasible() const
                                      { return (NOT costLimitExceeded_); }
  NABoolean exceededCostLimit() const
                                      { return NOT costLimitExceeded_; }


  // These two method used by pruning heuristic to use failed plan cost
  inline void setCurrentPlanExceededCostLimit()
  {
	currentPlanExceededCostLimit_ = TRUE;
  }
  inline NABoolean currentPlanExceededCostLimit()
  {
	return currentPlanExceededCostLimit_;
  }
  // ---------------------------------------------------------------------
  // Methods that are used within the Cascades search engine.
  // ---------------------------------------------------------------------

  // Compare one context with another;
  // result == MORE <==> this context is more restricive
  // in both reqd and excl phys prop; similar for LESS;
  // result == INCOMPATIBLE if contexts have conflicting
  // requested properties (like sorted by A vs. sorted by B),
  // otherwise, result = UNDEFINED.
  COMPARE_RESULT compareContexts (const Context & other) const;

  // Mark this Context to indicate that it is *done* for the current pass.
  // The marking is done for a Context that cannot yield an optimal plan,
  // e.g., because the plan is infeasible given the cost limit.
  void markAsDoneForCurrentPass()
                  { doneInPass_ = GlobalRuleSet->getCurrentPassNumber(); }

  // Use this to re-optimize the current context:
  void resetCurrentPassNumber()
  { doneInPass_ = Lng32(-1); }

  // Has this Context been optimized during the current optimization pass?
  NABoolean optimizedInCurrentPass() const
        { return (doneInPass_ == GlobalRuleSet->getCurrentPassNumber()); }

  // Does this Context provide an optimal solution?
  // The Context provides an optimal solution, iff a solution is found
  // during the current optimization pass whose cost is within the cost
  // limit.
  NABoolean hasOptimalSolution() const;

  // The Context provides possible solution, iff a solution is found
  // during the current optimization pass. Cost limit needs to be checked
  // separately, when necessary.
  NABoolean hasSolution() const
  {
    return ( solution_ AND optimizedInCurrentPass() );
  }

  void clearFailedStatus();

  // Test whether the given plan satisfies a context.
  // (no consideration of the cost limit)
  NABoolean satisfied(const CascadesPlan * const plan) const;

  // Find the best existing plan for a context and indicate in the return
  // value whether the optimal plan or a definite failure has been found.
  NABoolean findBestSolution();

  // The following function will be used by parallelHeuristic2.
  // The heuristics will check if the current group is small (less than
  // ROWS_PARALLEL_THRESHOLD) and has only one base table to skip
  // re-optimization of logical expressions and existing plans for this
  // group if the context has partitioning requirements other than
  // "exactly one partition".
  NABoolean reoptimizeExprAndPlans();

  // return a complete solution (a binding of a tree of physical nodes)
  //
  // see function body for description of the parameter
  RelExpr * bindSolutionTree(NABoolean getPrevSolution=FALSE) const;

  NABoolean isPruningEnabled() const           { return pruningEnabled_; }
  void setPruningEnabled(NABoolean v)             { pruningEnabled_ = v; }

  // store pws information in context, used for preliminary cost estimation
  void setPlanWorkSpace(PlanWorkSpace *myPws) { myPws_ = myPws; }
  inline PlanWorkSpace* getPlanWorkSpace() const { return myPws_; }

  NABoolean isNice() const
  {
    return niceContext_;
  }

  NABoolean isNiceContextEnabled() const;

  void setNice(NABoolean v)                          { niceContext_ = v; }

  CascadesGroupId getGroupId() const                  { return groupId_; }
  void setGroupId(CascadesGroupId g)                     { groupId_ = g; }

  // methods on the associated input est. logical properties
  EstLogPropSharedPtr getInputLogProp() const  { return inputEstLogProp_; }
  void setInputLogProp(const EstLogPropSharedPtr& elp) { inputEstLogProp_ = elp; }

  // --- Methods on the current plan

  // Whenever this Context is reused (CascadesGroup::shareContext()),
  // the current plan is cleared.
  void clearCurrentPlan()                         { currentPlan_ = NULL; }

  // Whenever this Context is resued for creating a new plan
  // (CreatePlanTask::CreatePlanTask), the new plan is assigned
  // with this Context.
  void assignCurrentPlan(CascadesPlan * plan)     { currentPlan_ = plan; }

  // --- Methods on the plans that are candidate solutions.

  void addCandidatePlan(CascadesPlan * plan);

  inline Lng32 getCountOfCandidatePlans() const
                                         { return candidates_.entries(); }
  inline CascadesPlan * getCandidatePlan(Lng32 i)
                                                { return candidates_[i]; }
  inline NABoolean isACandidate(CascadesPlan *plan) const
                                    { return candidates_.contains(plan); }

  // --- Methods on the plan that is the best solution so far

  inline const CascadesPlan * getSolution() const    { return solution_; }


  void setSolution(CascadesPlan *plan,
                   NABoolean checkAndAdjustCostLimit = TRUE);

  void setPhysicalPropertyForSolution(PhysicalProperty* spp)
                                  { solution_->setPhysicalProperty(spp); }

  // manage the number of outstanding operations/tasks for this context
  Lng32 getOutstanding() const                     { return outstanding_; }
  inline void incrOutstanding()                        { outstanding_++; }
  void decrOutstanding();

  Context * getCurrentAncestor() const        { return currentAncestor_; }
  void setCurrentAncestor(Context *c)            { currentAncestor_ = c; }

  NABoolean isADuplicate() const          { return duplicateOf_ != NULL; }
  Context * getDuplicateOf() const                { return duplicateOf_; }
  void setDuplicateOf(Context *c)                    { duplicateOf_ = c; }

  void setPredecessorTask(CascadesTask *t)       { predecessorTask_ = t; }
  CascadesTask * getPredecessorTask() const   { return predecessorTask_; }

  inline const RuleSubset &getTriedEnforcerRules() const
                                           { return triedEnforcerRules_; }
  inline RuleSubset &triedEnforcerRules() { return triedEnforcerRules_; }
  inline RuleSubset &ignoredRules() { return ignoreTheseRules_; }

  virtual void print (FILE * f = stdout,
		      const char * prefix = "",
		      const char * suffix = "") const;

  NAString getRequirementsString() const;
  //GTOOL
  NAString getCostString() const;
  NAString getRPPString() const;
  NAString getIPPString() const;
  NAString getILPString() const;
  NAString getStatusString() const;

  // -- Methods to enable the optimizer "recovery" mechanism
  // -- basically, if the optimizer hits an assertion in pass N,
  // -- we return the best plan from pass N-1

  inline const CascadesPlan * getPreviousSolution() const
                                                 { return prevSolution_; }
  NABoolean setPreviousSolution() ;

  const GroupAttributes *getGroupAttr() const;

//<pb>
private:

  // ---------------------------------------------------------------------
  // information that is passed top-down during the optimization process
  // ---------------------------------------------------------------------

  const ReqdPhysicalProperty* const reqdPhys_;  // required physical properties

  const InputPhysicalProperty* const inputPhys_; // input physical properties

  CostLimit*                costLimit_;       // cost limit

  // ---------------------------------------------------------------------
  // administrative information, used by the optimizer search engine
  // ---------------------------------------------------------------------
  CascadesGroupId  groupId_;           // where is this context used
  Lng32             doneInPass_;        // OptimizeGroupTask done in this pass
  Lng32             outstanding_;       // # plans outstanding in
                                       // OptimizeGroupTask task
  Context*         currentAncestor_;   // current parent context
  Context*         duplicateOf_;       // duplicate resulting from
                                       // equivalence class merge

  // pointer to the predecessor task for this context, i.e.
  // the task scheduled just prior to the OptimizeGroupTask for this context
  CascadesTask*    predecessorTask_;

  NABoolean        costLimitExceeded_; // solution exceeds cost limit
  NABoolean        currentPlanExceededCostLimit_; // flag passed from plan
  NABoolean        pruningEnabled_;    // allowed to perform branch and bound?

  // "Nice context". If true, parent will not imlicitly require properties
  // from grandchildren (could be satisfied by child naturally or forced
  // by enforcers above immediate child). Usually, parent requirement will
  // be always tried first before firing enforcers. This causes, for
  // example, partitioning requirement created by groupBy to be tried by
  // any relational operator below that groupBy, as weel as arrangement
  // requirement created by merge join. "Nice context" reduces the number
  // of contexts to be optimized, therefore - compile time and memory
  // required to optimize the plan. The plan created with "nice context"
  // could be a littel worse than default plan.
  NABoolean        niceContext_;

  // ---------------------------------------------------------------------
  // Each context is associated with one set of input estimated logical
  // properties from this group (or NULL).
  // ---------------------------------------------------------------------
  EstLogPropSharedPtr inputEstLogProp_;  // input est. log. properties used
                                         // during optimization

  // ---------------------------------------------------------------------
  // Bitmap specifying which enforcer rules have already been applied
  // for this context (to avoid applying them again).
  // ---------------------------------------------------------------------
  RuleSubset triedEnforcerRules_;

  // ---------------------------------------------------------------------
  // Bitmap specifying All the rules that this context may consider.
  // This is done for performance reason and not correctness.
  // ---------------------------------------------------------------------
  RuleSubset ignoreTheseRules_;

  // ---------------------------------------------------------------------
  // Plans that are generated bottom up.
  // ---------------------------------------------------------------------
  CascadesPlan*    currentPlan_;  // plan that is currently being optimized
				  // using  this Context
  CascadesPlan*    solution_;     // optimal solution for this context
  CascadesPlan*    prevSolution_ ;// pointer to the solution of the previous
                                  // optimizer pass (needed for recovery from
                                  // assertion failures)
  CascadesPlanList candidates_;   // plans that satisfy the context

  PlanWorkSpace*       myPws_;    // my PlanWorkSpace

}; // Context
#pragma warn(1506)  // warning elimination

// -----------------------------------------------------------------------
// A list and an array of context pointers
// -----------------------------------------------------------------------
class ContextList : public LIST(Context *)
{
public:
  ContextList(CollHeap* h/*=0*/) : LIST(Context *)(h) //
    {  }
};

class ContextArray : public ARRAY(Context *)
{
public:
  ContextArray(CollHeap* h/*=0*/) : ARRAY(Context *)(h)
    {}

  // copy ctor
  ContextArray (const ContextArray & orig, CollHeap * h=0) ;
};
//<pb>
// -----------------------------------------------------------------------
// class PlanWorkSpace
//
// This object is created and destroyed together with the CreatePlanTask.
// It provides a memory for the implementation of the plan generator,
// createPlan(). The latter method can create multiple
// Contexts for considering different plans for one or more of its
// children. It saves each Context that is created for its child in
// a PlanWorkSpace object. Finally, it selects one of the candidates
// per child to create its own execution plan.
// -----------------------------------------------------------------------

class PlanWorkSpace : public NABasicObject
{
public:

  // ---------------------------------------------------------------------
  // Constructor and destructor.
  // ---------------------------------------------------------------------
  PlanWorkSpace(Lng32 numberOfChildren);

  ~PlanWorkSpace();

  const Context *getContext() const { return myContext_; }

  // ---------------------------------------------------------------------
  // Set the context under which this operator is optimized for. A plan
  // must have already been created for this context by CreatePlanTask.
  // ---------------------------------------------------------------------
  void storeContext(Context* myContext)
  {
    myContext_ = myContext;
    CMPASSERT(myContext_->getPlan() != NULL);
  }

  // ---------------------------------------------------------------------
  // Is the workspace empty?
  // ---------------------------------------------------------------------
  NABoolean isEmpty() const               { return (contextCount_ == 0); }

  Lng32 getCountOfChildContexts() const           { return contextCount_; }

  // ---------------------------------------------------------------------
  // Store a new child Context in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  void storeChildContext(Lng32 childIndex, Lng32 planNumber,
			 Context* childContext);

  // ---------------------------------------------------------------------
  // Get a specific child Context.
  // ---------------------------------------------------------------------
  Context* getChildContext(Lng32 childIndex, Lng32 planNumber = 0) const;

  // Is this plan's n-th child a scan?
  NABoolean getScanLeaf(int childNumber, int planNumber, FileScan *&scanLeaf) const;

  // -----------------------------------------------------------------------
  // Erase the latest context.
  // -----------------------------------------------------------------------
  void eraseLatestContextFromWorkSpace();

  // -----------------------------------------------------------------------
  // Delete the specified child Context from the PlanWorkSpace.
  // -----------------------------------------------------------------------
  void deleteChildContext(Lng32 childIndex, Lng32 planNumber);

  // ---------------------------------------------------------------------
  // Initialize the cost and set it to the initial local cost.
  // ---------------------------------------------------------------------
  void initializeCost(Cost* operatorCost);

  void setOperatorCost(Cost * cost);

  Cost* getOperatorCost() const          { return operatorCost_; }

  void setCountOfStreams(Lng32 countOfStreams)
                                     { countOfStreams_ = countOfStreams; }

  Lng32 getCountOfStreams() const               { return countOfStreams_; }

  // ---------------------------------------------------------------------
  // Return initial operator cost if the guess we made about the
  // physical properties that costing is based on was accurate.
  // Otherwise, recompute the operator cost using the synthesized
  // physical property stored in myContext_->getPlan() and return it.
  // ---------------------------------------------------------------------
  Cost* getFinalOperatorCost(Lng32 planNumber);

  // ---------------------------------------------------------------------
  // Set/get a computed PartialPlan cost in/from the PlanWorkSpace.
  // ---------------------------------------------------------------------
  void setPartialPlanCost(Cost* newCost);

  Cost* getPartialPlanCost() const                { return partialPlanCost_; }

  // ---------------------------------------------------------------------
  // Set/get a computed known children cost in/from the PlanWorkSpace.
  // ---------------------------------------------------------------------
  void setKnownChildrenCost(Cost* newCost);

  Cost* getKnownChildrenCost() const          { return knownChildrenCost_; }

  void updateBestCost(Cost* cost, Lng32 planNumber);

  Cost*             getBestRollUpCostSoFar() const
                                               { return bestRollUpCostSoFar_; }

  Cost*             getBestOperatorCostSoFar() const
                                             { return bestOperatorCostSoFar_; }

  Lng32              getBestPlanSoFar() const { return bestPlanSoFar_; }

  NABoolean isBestPlanSoFarBMO() const       { return bestPlanSoFarIsBMO_; }

  PhysicalProperty* getBestSynthPhysPropSoFar() const
                                            { return bestSynthPhysPropSoFar_; }

  void setPartialPlanCostToOperatorCost();

  // ---------------------------------------------------------------------
  // Get the latest Context that was considered.
  // ---------------------------------------------------------------------
  Context* getLatestChildContext() const        { return latestContext_; }

  Lng32 getLatestChildIndex() const                { return latestChild_; }

  Lng32 getLatestPlan() const                       { return latestPlan_; }
  Lng32 getPrevPlan() const                     { return prevPlan_; }


  // ---------------------------------------------------------------------
  // Keep track of number of children costed for latest plan.
  // ---------------------------------------------------------------------
  Lng32 getPlanChildCount() const               { return planChildCount_; }
  void incPlanChildCount()                          { ++planChildCount_; }
  void resetPlanChildCount()                      { planChildCount_ = 0; }

  // ---------------------------------------------------------------------
  // If the latest Context cannot be used because the cost for its plan
  // exceeds the cost limit, mark it as failed,
  // ---------------------------------------------------------------------
  void markLatestContextAsViolatesCostLimit()
                                         { withinCostLimitFlag_ = FALSE; }
  NABoolean isLatestContextWithinCostLimit() const
                                          { return withinCostLimitFlag_; }

  // ---------------------------------------------------------------------
  // Iterates over the well-formed plans stored in this pws, cost them
  // and pick the one with the lowest cost if planNumber is specified to
  // be -1. Otherwise, just pick the plan with planNumber if it's valid,
  // pick no plan otherwise. Return TRUE and set planNumber to the plan
  // picked if one is picked. Return FALSE and set planNumber to -1 other
  // wise.
  // ---------------------------------------------------------------------
  NABoolean findOptimalSolution(Lng32& planNumber);

  Cost* getCostOfPlan(Lng32 planNumber);

  // ---------------------------------------------------------------------
  // Does this pws belong to a big memory operator?
  // ---------------------------------------------------------------------
  void setBigMemoryOperator(NABoolean isBMO)           { isBMO_ = isBMO; }
  NABoolean isBigMemoryOperator() const                 { return isBMO_; }

  // ---------------------------------------------------------------------
  // Does this pws wants parallelism
  // ---------------------------------------------------------------------
  void setParallelismIsOK(NABoolean isOK)   { parallelismIsOK_ = isOK; }
  NABoolean getParallelismIsOK() const      { return parallelismIsOK_; }

  NABoolean allChildrenContextsConsidered()
    { return allChildrenContextsConsidered_;}
  void setAllChildrenContextsConsidered()
    { allChildrenContextsConsidered_ = TRUE;}
  void resetAllChildrenContextsConsidered()
    { allChildrenContextsConsidered_ = FALSE;}

#ifdef DEBUG
  // ---------------------------------------------------------------------
  //  Reset count of PlanWorkSpace objects created.  Used for debugging
  // purpopes only. The counter is maintained by CmpStatement::planWorkSpaceCount_
  // ---------------------------------------------------------------------
  void resetPwsCount();

#endif /* DEBUG */

//<pb>
private:

  // ---------------------------------------------------------------------
  // The context under which the operator associated with this pws is
  // optimized for.
  // ---------------------------------------------------------------------
  Context* myContext_;

  // ---------------------------------------------------------------------
  // The following array has as many elements as the number of children
  // of the physical expression for which plan generation is in progress.
  // Each element is an array of pointers to Contexts that are used for
  // optimzing a specific child.
  // ---------------------------------------------------------------------
  ARRAY(ContextArray*)  childContexts_;

  // ---------------------------------------------------------------------
  // A counter for contexts created so far.
  // ---------------------------------------------------------------------
  Lng32 contextCount_;

  // ---------------------------------------------------------------------
  // Cache the latest Context that was stored in childContexts_
  // as well as the index for the child.
  // ---------------------------------------------------------------------
  Lng32     latestChild_;
  Lng32     latestPlan_;
  Context* latestContext_;

  // keep track of previous plan, this will be compared with current plan
  // (latestPlan) to decide if preliminary cost need to be recomputed
  Lng32     prevPlan_;

  // ---------------------------------------------------------------------
  // Keep track of the number of children costed for the latest plan.
  // ---------------------------------------------------------------------
  Lng32     planChildCount_;

  // ---------------------------------------------------------------------
  // If the latest Context cannot be used because the cost for its plan
  // exceeds the cost limit, mark it as failed,
  // ---------------------------------------------------------------------
  NABoolean withinCostLimitFlag_;

  // ---------------------------------------------------------------------
  // Cost for this operator independent of its children.  Based initially
  // on an estimated degree of parallelism and possibly revised if actual
  // degree of parallelism differs from estimate.
  // ---------------------------------------------------------------------
  Cost* operatorCost_;

  // ---------------------------------------------------------------------
  // Degree of parallelism assumed when computing operator cost.
  // ---------------------------------------------------------------------
  Lng32 countOfStreams_;

  // ---------------------------------------------------------------------
  // The cost for this operator together with the cost for one or more
  // (but not all) of its children. This cost is NOT the final cost.
  // ---------------------------------------------------------------------
  Cost* partialPlanCost_;

  // -------------------------------------------------------------------
  // The cost for one or more (but not all) of this operator's children.
  // -------------------------------------------------------------------
  Cost* knownChildrenCost_;

  // --------------------------------------------------------------------
  //  The best roll-up cost of any plan considered so far for this plan
  // workspace.  Also keep track of the operator cost, plan number,
  // whether it is a BMO and synthesized physical properties associated
  // with the best plan so far.
  // --------------------------------------------------------------------
  Cost*             bestRollUpCostSoFar_;
  Cost*             bestOperatorCostSoFar_;
  Lng32              bestPlanSoFar_;
  NABoolean         bestPlanSoFarIsBMO_;
  PhysicalProperty* bestSynthPhysPropSoFar_;

  // ---------------------------------------------------------------------
  // Does this pws belong to a big memory operator?
  // ---------------------------------------------------------------------
  NABoolean isBMO_;

  // Flag is set to TRUE when all children contexts have been considered

  NABoolean allChildrenContextsConsidered_;

  // This attribute will be set to FALSE for hash and merge joins
  // when parallelHeuristic is on and okToAttemtESPParallelism()
  // function retuns FALSE
  NABoolean parallelismIsOK_;

#ifdef DEBUG
  ////////////////////////////////////////////////////////////////////////
  //   THE FOLLOWING FIELD IS USED FOR DEBUGGING PURPOSES ONLY    //
  ////////////////////////////////////////////////////////////////////////

  // ---------------------------------------------------------------------
  // Value of CmpStatement::planWorkSpaceCount_ at time of PlanWorkSpace creation. 
  // Used for debugging purposes only.  Never modified once set.
  // ---------------------------------------------------------------------
  Lng32 pwsID_;

#endif /* DEBUG */

}; // class PlanWorkSpace

class NestedJoinPlanWorkSpace : public PlanWorkSpace
{
public:

  NestedJoinPlanWorkSpace(Lng32 numberOfChildren) : 
       PlanWorkSpace(numberOfChildren),
       useParallelism_(FALSE),
       childNumPartsRequirement_(0),
       childNumPartsAllowedDeviation_(0.0),
       numOfESPsForced_(FALSE),
       childPlansToConsider_(0),
       OCBJoinIsConsidered_(FALSE),
       OCRJoinIsConsidered_(FALSE),
       fastLoadIntoTrafodion_(FALSE) {}
  ~NestedJoinPlanWorkSpace() {}

  NABoolean getUseParallelism()       const       { return useParallelism_; }
  Lng32     getChildNumPartsRequirement() const
                                        { return childNumPartsRequirement_; }
  float     getChildNumPartsAllowedDeviation() const
                                   { return childNumPartsAllowedDeviation_; }
  NABoolean getNumOfESPsForced()      const      { return numOfESPsForced_; }
  Lng32     getChildPlansToConsider() const { return childPlansToConsider_; }
  NABoolean getOCBJoinIsConsidered()  const  { return OCBJoinIsConsidered_; }
  NABoolean getOCRJoinIsConsidered()  const  { return OCRJoinIsConsidered_; }
  NABoolean getFastLoadIntoTrafodion()  const  { return fastLoadIntoTrafodion_; }

  void setParallelismItems(NABoolean useParallelism,
                           Lng32     childNumPartsRequirement,
                           float     childNumPartsAllowedDeviation,
                           NABoolean numOfESPsForced)
          {                useParallelism_ =                useParallelism;
                 childNumPartsRequirement_ =      childNumPartsRequirement;
            childNumPartsAllowedDeviation_ = childNumPartsAllowedDeviation;
                          numOfESPsForced_ =               numOfESPsForced; }
  void setChildPlansToConsider(Lng32 cp)      { childPlansToConsider_ = cp; }
  void setOCBJoinIsConsidered(NABoolean o)      { OCBJoinIsConsidered_ = o; }
  void setOCRJoinIsConsidered(NABoolean o)      { OCRJoinIsConsidered_ = o; }
  void setFastLoadIntoTrafodion(NABoolean o)      { fastLoadIntoTrafodion_ = o; }

  void transferParallelismReqsToRG(RequirementGenerator &rg);

private:

  NABoolean useParallelism_;
  Lng32     childNumPartsRequirement_;
  float     childNumPartsAllowedDeviation_;
  NABoolean numOfESPsForced_;
  Lng32     childPlansToConsider_;
  NABoolean OCBJoinIsConsidered_;
  NABoolean OCRJoinIsConsidered_;
  NABoolean fastLoadIntoTrafodion_;

}; // class NestedJoinPlanWorkSpace

class TMUDFPlanWorkSpace : public PlanWorkSpace
{
public:

  TMUDFPlanWorkSpace(Lng32 numberOfChildren) : 
       PlanWorkSpace(numberOfChildren),
       udrPlanInfo_(NULL)  {}
  ~TMUDFPlanWorkSpace() {}

  // ---------------------------------------------------------------------
  // get/set TMUDF-related things
  // ---------------------------------------------------------------------
  tmudr::UDRPlanInfo *getUDRPlanInfo()           { return udrPlanInfo_; }
  const tmudr::UDRPlanInfo *getUDRPlanInfo() const
                                                 { return udrPlanInfo_; }
  void setUDRPlanInfo(tmudr::UDRPlanInfo *pi)      { udrPlanInfo_ = pi; }

private:

  tmudr::UDRPlanInfo *udrPlanInfo_;

};

//<pb>
// -----------------------------------------------------------------------
// Groups of equivalent expressions
// ================================
// The search memory consists of many groups (equivalence classes) of
// equivalent expressions. A group collects all equivalent logical
// expressions (queries) and physical expressions (plans) and provides a
// centralized place for the logical properties of all these
// expressions.  It consists of three main components
// (a) a list of logical expressions that all produce the same result,
// (b) the logical properties of that result, and
// (c) a list of plans that implement the logical expressions.
// (d) a list of pattern for which this group has been expanded
// -----------------------------------------------------------------------

#pragma nowarn(1506)   // warning elimination
class CascadesGroup : public NABasicObject
{

public:

  CascadesGroup (CascadesGroupId groupId,
		 GroupAttributes *groupAttr);
  ~CascadesGroup ();

  inline CascadesGroupId getGroupId() const           { return groupId_; }

  inline GroupAttributes * getGroupAttr() const     { return groupAttr_; }

  // add, unlink, and retrieve logical/physical expressions, and plans
  void addLogExpr (RelExpr * expr, RelExpr *src=NULL);
  NABoolean addPhysExpr (RelExpr* & expr, RelExpr *src=NULL);
  void addPlan(CascadesPlan * plan);

  RelExpr *unlinkLogExpr(RelExpr *expr);
  RelExpr *unlinkPhysExpr(RelExpr *expr);

  inline RelExpr * getFirstLogExpr() const           { return logExprs_; }
  inline RelExpr * getFirstPhysExpr() const         { return physExprs_; }
  CascadesPlan   * getFirstPlan() const;

  const CascadesPlanList& getPlans() const              { return plans_; }
  Lng32 getCountOfPlans() const                { return plans_.entries(); }

  RelExpr * getLastLogExpr() const;

  Lng32 getCountOfLogicalExpr() const;
  Lng32 getCountOfPhysicalExpr() const;
  double calculateNoOfLogPlans() const;

  HashValue hash ();

  void merge (CascadesGroup * other);

  // create or share a context with a given optimization goal for this group
  Context * shareContext(const ReqdPhysicalProperty* const reqdPhys,
                         const InputPhysicalProperty* const inputPhys,
			 CostLimit* costLimit,
			 Context* parentContext,
			 const EstLogPropSharedPtr& inputLogProp);

  // access all the optimization goals (contexts) for this group
  inline Lng32 getCountOfContexts() const
                                              { return goals_.entries(); }
  inline Context * getContext(Lng32 i)                { return goals_[i]; }

  inline Lng32 getExploredInPass() const        { return exploredInPass_; }
  inline void setExploredInPass(Lng32 e)           { exploredInPass_ = e; }

  inline RuleSubset getExploredRules () const       {return exploredRules_;}
  inline void addToExploredRules (RuleSubset & rules) {exploredRules_ += rules;}

  // temporary, for nice context prototype
  inline NABoolean isBelowRoot() const
  {
      return isBelowRoot_;
  }
  inline void setBelowRoot(NABoolean val)
  {
      isBelowRoot_ = val;
  }

  void print (FILE * f = stdout,
	      const char * prefix = "",
	      const char * suffix = "") const;
//<pb>
private:

  CascadesGroupId   groupId_;          // index into CascadesMemo
  GroupAttributes * groupAttr_;        // common attributes for this Group
  RelExpr         * logExprs_;         // linked list of logical expressions
  RelExpr         * physExprs_;        // linked list of physical expressions
  CascadesPlanList  plans_;            // linked list of plans
  ContextList       goals_;            // optimization goals tried so far
  Lng32              exploredInPass_;   // to avoid exploring twice
  RuleSubset        exploredRules_;    // Rules that the groups has been explored
                                       // with or scheduled to be explored with.
  // This is temporary and later should be moved to GroupAttr ot GroupAnalysys,
  // the flag will be TRUE for immediate child of the ROOT. This will help
  // top parallelism heristic and nice contrext for top groupBy
  NABoolean  isBelowRoot_;
}; // CascadesGroup
#pragma warn(1506)  // warning elimination
//<pb>
// -----------------------------------------------------------------------
// Search space memory
// ===================
// All search efforts' results are kept in the search space memory.
// The main components of this memory is an array of groups.  In
// addition, there is a hash table to facilitate fast detection of
// duplicate expressions.
// -----------------------------------------------------------------------

#pragma nowarn(1506)   // warning elimination
class CascadesMemo : public NABasicObject
{

public:

  CascadesMemo (CascadesGroupId groups = 100, Lng32 buckets = 1001);
  ~CascadesMemo ();
  // to include a newly derived expression in "memo"
  RelExpr * include (RelExpr * query,
		     NABoolean & duplicateExprFlag,
		     NABoolean & groupMergeFlag,
		     NABoolean GrpIdIsBinding = FALSE, //soln-10-070330-3667
		     CascadesGroupId groupId = INVALID_GROUP_ID,
		     Context * context = NULL,
                     RelExpr *before = NULL
                    );

  // deal with duplicate detection
  void addExpr (RelExpr * expr, HashValue hash_value);
  RelExpr * findDuplicate (RelExpr * expr) const;

  // deal with groups
  CascadesGroupId makeNewGroup(GroupAttributes *ga);
  void update (CascadesGroup * oldGroup, CascadesGroup * newGroup);
  inline Lng32 getCountOfUsedGroups() const
                                             { return group_.entries(); }
  inline CascadesGroup * operator[] (CascadesGroupId groupId)
                                              { return group_[groupId]; }

  // consolidate group pointers after group merge(s)
  Int32 garbageCollection();

  // check for inconsistencies (return FALSE if something wrong)
  NABoolean consistencyCheck() const;

  void print (FILE * f = stdout,
	      const char * prefix = "",
	      const char * suffix = "") const;

  void print_all_trees (CascadesGroupId root,
			NABoolean incl_log,
			NABoolean incl_phs,
			FILE * f = stdout) const;
//<pb>
private:

  ARRAY(CascadesGroup *) group_;      // groups of equivalent expressions

  ARRAY(RelExpr *)	 hash_;       // for dupl. expr. detection
  Lng32                   hashSize_;   // size of hash table

}; // CascadesMemo
#pragma warn(1506)  // warning elimination
//<pb>
// -----------------------------------------------------------------------
// Tasks
// =====
// A task is an activity within the search process.  The original task
// is to optimize the entire query.  Tasks create and schedule each
// other; when no un-done tasks remain, optimization terminates.
//  Tasks must destroy themselves when done!
// -----------------------------------------------------------------------

class CascadesTask : public NABasicObject
{
friend class CascadesTaskList;

public:

  enum cascadesTaskTypeEnum
    {
      OPTIMIZE_GROUP_TASK,
      OPTIMIZE_EXPR_TASK,
      APPLY_RULE_TASK,
      CREATE_PLAN_TASK,
      EXPLORE_GROUP_TASK,
      EXPLORE_EXPR_TASK,
      GARBAGE_COLLECTION_TASK,
      NUMBER_OF_TASK_TYPES = 7     // This should be at the end of the list
    };

  CascadesTask (Guidance * guidance, Context * context,
		Lng32 parentTaskId, short stride);
  ~CascadesTask ();

  Context * getContext() const                        { return context_; }

  inline CascadesTask * getNextTask() const {return next_; }

  inline Lng32 getParentTaskId() const { return parentTaskId_; }

  inline short getSubTaskId() const { return stride_; }

  virtual CascadesGroupId getGroupId() const;

  virtual RelExpr * getExpr();

  virtual CascadesPlan * getPlan();

  virtual void perform (Lng32 taskId) = 0;

  virtual void garbageCollection(RelExpr *oldx,
				 RelExpr *newx,
				 Int32 groupMergeCount);

  virtual NAString taskText() const;

  NABoolean taskNumberExceededLimit(Lng32 id);

  virtual void print (FILE * f = stdout,
		      const char * prefix = "",
		      const char * suffix = "") const;

  virtual cascadesTaskTypeEnum taskType() = 0;

//<pb>
private:

  CascadesTask	* next_;	  // linked list of tasks

protected:

  Guidance	* guidance_;	  // search guidance
  Context	* context_;	      // shared context
  Lng32        parentTaskId_;  // parent task, in execution order
  short       stride_;        
  // stride_ is (sub)task number assigned by parent in its parent::perform
  // call when parent was popped and it pushed this (sub)task onto the
  // cascades todo stack. 
  // {parentTaskId_,stride_} uniquely identifies this CascadesTask.

}; // CascadesTask
//<pb>
// -----------------------------------------------------------------------
// List of un-done tasks
// -----------------------------------------------------------------------

class CascadesTaskList : public NABasicObject
{

public:

  CascadesTaskList ();
  ~CascadesTaskList ();
  inline NABoolean empty () const { return first_ == NULL; }

  // push and pop
  inline void push (CascadesTask * task) { task->next_ = first_; first_ = task; }
  CascadesTask * pop ();

  // for the rare case when tasks need to be scheduled in a specific position
  // on the stack
  void insertTask (CascadesTask * task, CascadesTask * predecessorTask);

  // for the rare case when tasks need to be scheduled in a specific position
  // on the stack
  void insertOptimizeExprTask (CascadesTask * task, CascadesTask * predecessorTask);

  // non-destructive read
  CascadesTask * getFirst() { return first_; }
  CascadesTask * getNext(CascadesTask *prev) { return prev->next_; }

  void print (FILE * f = stdout,
	      const char * prefix = "",
	      const char * suffix = "") const;

private:

  CascadesTask	* first_;	// anchor of CascadesTask stack

}; // CascadesTaskList
//<pb>
// -----------------------------------------------------------------------
// Task to optimize a group
// ========================
// Find the optimal plan for any expression in a group.
// -----------------------------------------------------------------------

class OptimizeGroupTask : public CascadesTask
{

public:

  OptimizeGroupTask (Context * context,
		     Guidance * guidance,
		     NABoolean reoptimize,
		     Lng32 parentTaskId, short stride);
  virtual CascadesGroupId getGroupId() const;

  virtual void perform (Lng32 taskId);

  virtual NAString taskText() const;

  void print (FILE * f = stdout,
	      const char * prefix = "",
	      const char * suffix = "") const;

  virtual cascadesTaskTypeEnum taskType() {return CascadesTask::OPTIMIZE_GROUP_TASK;}

private:

  CascadesGroupId  groupId_;
  NABoolean        reoptimize_;  // retry optimization before taking any
                                 // existing "best" plans, if this is set to TRUE
  // helper method that returns true if scheduling a CreatePlanTask for this
  // plan's physExpr for this context_ can generate a DAG (non-tree) plan
  NABoolean canCauseCycle(const CascadesPlan *plan) const;
}; // OptimizeGroupTask
//<pb>
// -----------------------------------------------------------------------
// Task to explore a logical expression
// ====================================
// Find the optimal plan for any expression equivalent to a given one.
// -----------------------------------------------------------------------

class OptimizeExprTask : public CascadesTask
{

public:

  OptimizeExprTask (RelExpr * expr,
		    Guidance * guidance,
		    Context * context,
		    Lng32 parentTaskId, short stride);
  virtual CascadesGroupId getGroupId() const;

  virtual RelExpr * getExpr();

  virtual void perform (Lng32 taskId);

  virtual void garbageCollection(RelExpr *oldx,
				 RelExpr *newx,
				 Int32 groupMergeCount);

  void print (FILE * f = stdout,
	      const char * prefix = "",
	      const char * suffix = "") const;

  virtual NAString taskText() const;

  virtual cascadesTaskTypeEnum taskType() {return CascadesTask::OPTIMIZE_EXPR_TASK;}

private:

  RelExpr	* expr_;

}; // OptimizeExprTask
//<pb>
// -----------------------------------------------------------------------
// Task to explore a group
// =======================
// Given a pattern, create all equivalent expressions matching
// the pattern.  If the pattern is NULL, create all equivalent
// expressions.
// -----------------------------------------------------------------------

class ExploreGroupTask : public CascadesTask
{

public:

  ExploreGroupTask (CascadesGroupId groupId,
		    RelExpr * pattern,
		    Guidance * guidance,
		    Lng32 parentTaskId, short stride);
  virtual CascadesGroupId getGroupId() const;

  virtual void perform (Lng32 taskId);

  void print (FILE * f = stdout,
	      const char * prefix = "",
	      const char * suffix = "") const;

  virtual NAString taskText() const;

  virtual cascadesTaskTypeEnum taskType() {return CascadesTask::EXPLORE_GROUP_TASK;}

private:

  CascadesGroupId   groupId_;
  RelExpr	  * pattern_;

}; // ExploreGroupTask
//<pb>
// -----------------------------------------------------------------------
// Task to explore a logical expression
// ====================================
// Given a pattern, create all equivalent expressions matching
// the pattern.  If the pattern is NULL, create all equivalent
// expressions.
// -----------------------------------------------------------------------

class ExploreExprTask : public CascadesTask
{

public:

  ExploreExprTask (RelExpr * expr,
		   RelExpr * pattern,
		   Guidance * guidance,
		   Lng32 parentTaskId, short stride);
  virtual CascadesGroupId getGroupId() const;
  virtual RelExpr * getExpr();

  virtual void perform (Lng32 taskId);

  virtual void garbageCollection(RelExpr *oldx,
				 RelExpr *newx,
				 Int32 groupMergeCount);
  void print (FILE * f = stdout,
	      const char * prefix = "",
	      const char * suffix = "") const;

  virtual NAString taskText() const;

  virtual cascadesTaskTypeEnum taskType() {return CascadesTask::EXPLORE_EXPR_TASK;}

private:

  RelExpr	* expr_;
  RelExpr	* pattern_;

}; // ExploreExprTask
//<pb>
// -----------------------------------------------------------------------
// Task to apply a rule to a logical expression
// ============================================
// Given a rule and a logical expression, determine all sensible
// bind ings for the subexpressions currently available in "memo" and
// apply the rule.
// -----------------------------------------------------------------------

class ApplyRuleTask : public CascadesTask
{

public:

  ApplyRuleTask (Rule * rule,
		 RelExpr * expr,
		 RelExpr * pattern,
		 Guidance * guidance,
		 Context * context,
		 Lng32 parentTaskId, short stride);
  virtual CascadesGroupId getGroupId() const;
  virtual RelExpr * getExpr();

  virtual void perform (Lng32 taskId);

  virtual void garbageCollection(RelExpr *oldx,
				 RelExpr *newx,
				 Int32 groupMergeCount);

  void print (FILE * f = stdout,
	      const char * prefix = "",
	      const char * suffix = "") const;
  virtual NAString taskText() const;

  virtual cascadesTaskTypeEnum taskType() {return CascadesTask::APPLY_RULE_TASK;}

private:

  Rule		* rule_;	// rule to apply
  RelExpr	* expr_;	// root of expr. before rule
  RelExpr     	* pattern_;     // pattern for further exploration
  NABoolean     exploring_;     // FALSE if apply is for an optimizing task
                                // TRUE if apply is for exploring task

}; // ApplyRuleTask
//<pb>
// -----------------------------------------------------------------------
// Task to create a subplan
// ========================
// After an implementation rule has been applied during optimization,
// i.e., an implementation algorithm has been considered for one node
// in the query tree, optimization continues by optimizating each
// child to the implementation algorithm.  This task schedules further
// tasks to optimize one child at a time.  This task is different from
// other task types in the sense that this task is activated multiple
// times, i.e., each time another child has been optimized.
// -----------------------------------------------------------------------

class CreatePlanTask : public CascadesTask
{

public:

  CreatePlanTask (Rule * rule,
		  RelExpr * expr,
		  Guidance * guidance,
		  Context * context,
		  Lng32 parentTaskId, short stride,
		  CascadesPlan * failedPlan = NULL);

  ~CreatePlanTask ();
  void print (FILE * f = stdout,
	      const char * prefix = "",
	      const char * suffix = "") const;
  virtual NAString taskText() const;
  virtual CascadesGroupId getGroupId() const;
  virtual RelExpr * getExpr();
  virtual CascadesPlan * getPlan();

  virtual void perform (Lng32 taskId);

  virtual void garbageCollection(RelExpr *oldx,
				 RelExpr *newx,
				 Int32 groupMergeCount);

  virtual cascadesTaskTypeEnum taskType() {return CascadesTask::CREATE_PLAN_TASK;}

private:

  CascadesPlan * plan_;            // the plan

  PlanWorkSpace*  workSpace_;      // how many lives did this task have so far

  Rule *         rule_;            // implementation rule that was applied

}; // CreatePlanTask
//<pb>
// -----------------------------------------------------------------------
// Task to clean up after a group merge
// ====================================
// After a group merge, the merged group can be accessed with two
// different group numbers. The garbage collection task walks through
// the CascadesMemo structure and makes sure only one group number is used for
// each group, to avoid duplicate expressions that aren't detected as
// such, because they use different (but equivalent) group numbers.
// -----------------------------------------------------------------------

class GarbageCollectionTask : public CascadesTask
{

public:

  GarbageCollectionTask ();
  ~GarbageCollectionTask ();
  virtual void perform (Lng32 taskId);

  virtual void garbageCollection(RelExpr *oldx,
				 RelExpr *newx,
				 Int32 groupMergeCount);

  void print (FILE * f = stdout,
	      const char * prefix = "",
	      const char * suffix = "") const;

  virtual NAString taskText() const;

  virtual cascadesTaskTypeEnum taskType() {return CascadesTask::GARBAGE_COLLECTION_TASK;}

private:

  NABoolean alreadyDone_;
}; // GarbageCollectionTask
//<pb>
// -----------------------------------------------------------------------
// Bindings
// ========
// for pattern matching while applying a rule.  The CascadesBinding-tree is
// structured like the pattern; each CascadesBinding points to a logical
// expression (within a group).  The "advance" method permits to
// iterator over all possible bindings for a pattern.
// -----------------------------------------------------------------------

class CascadesBinding : public ReferenceCounter
{

public:

  enum CascadesBindingStateEnum
    {
      START_GROUP,	// newly created for an entire group
      START_EXPR,	// newly created for a single expression
      VALID_BINDING,	// last binding was succeeded
      ALMOST_EXHAUSTED, // last binding succeeded, but no further ones
      FINISHED,	        // iteration through bindings is exhausted
      EXPR_FINISHED	// current expr is finished; in advance() only
    };

// warning elimination (removed "inline")
  static const char * binding_state_name (CascadesBindingStateEnum state)
    {
      return  state == START_GROUP ? "start a group" :
              state == START_EXPR ? "start an expr" :
              state == VALID_BINDING ? "valid binding" :
              state == ALMOST_EXHAUSTED ? "last valid binding" :
              state == FINISHED ? "finished" : "undefined";
    } // binding_state_name

  // to find bindings for all expressions in group
  CascadesBinding(CascadesGroupId groupId,
		  RelExpr * pattern,
		  CascadesBinding * parent,
		  NABoolean incl_log,
		  NABoolean incl_phys);

  // to find bindings rooted at a specific logical expression
  CascadesBinding(RelExpr * expr,
		  RelExpr * pattern,
		  CascadesBinding * parent,
		  NABoolean incl_log,
		  NABoolean incl_phys);

  ~CascadesBinding();
  void print(FILE * f = stdout,
	     const char * prefix = "",
	     const char * suffix = "") const;

  NABoolean advance();		// produce the next binding

  RelExpr * extract_expr();        // materialize a binding
  void release_expr();          // release the materialized binding

  inline CascadesGroupId getGroupId() const { return groupId_; }
  inline CascadesBinding * getParent() { return parent_; }
//<pb>
private:

  CascadesBindingStateEnum  state_;

  NABoolean	     fixed_;	    // is advancing prohibited?
  CascadesGroupId    groupId_;      // group for which a binding is to be created
  RelExpr	   * curExpr_;	    // first or current expression
  RelExpr          * copiedExpr_;   // copied extracted expression or NULL

  RelExpr	   * pattern_;	    // pattern to search for
  CascadesBinding  * parent_;       // parent binding or NULL for top
  NABoolean	     inclLog_;	    // include log. expr's?
  NABoolean	     inclPhys_;	    // include phys. expr's?

  LIST(CascadesBinding *) children_;  // bindings for child expr's

};  // CascadesBinding

// -----------------------------------------------------------------------
// class OptDebug
//
// This class is for printing debug information to a file in the optimizer.
// On NSK, Display GUI is not available and arkcmp does not own a console
// window.  Hence, we want to log debug information to a file.
//
// -----------------------------------------------------------------------

class OptDebug
{
public:
  OptDebug();
  ~OptDebug();

  NABoolean openLog( const char* filename );
  void closeLog();

  
  void setCompileInstanceClass(const char* str);

  // ---------------------------------------------------------------------
  // Accessor functions.
  // ---------------------------------------------------------------------
  ostream& stream();
  FILE*    fp();

  // ---------------------------------------------------------------------
  // Functions to show tree nodes and their properties for debugging purpose.
  // ---------------------------------------------------------------------
  void showTree( const ExprNode *tree = NULL,
                 const CascadesPlan *plan = NULL,
                 const char *prefix = "",
                 NABoolean showDetail = TRUE );

  // ---------------------------------------------------------------------
  // Functions to show CascadeTask for debugging purpose.
  // ---------------------------------------------------------------------
  void showTask( Int32 pass = -1,
                 Int32 groupId = -1,
                 Int32 task_count = -1,
                 const CascadesTask *task = NULL,
                 const ExprNode *tree = NULL,
                 const CascadesPlan *plan = NULL,
                 const char *prefix = "" );

  void showNode( const ExprNode *tree = NULL,
                 const CascadesPlan *plan = NULL,
                 const char *prefix = "",
                 NABoolean showDetail = TRUE );

  // ---------------------------------------------------------------------
  // Functions to show memo statistics for debugging purpose.
  // ---------------------------------------------------------------------
  void showMemoStats(CascadesMemo *memo, const char *prefix, ostream &out);

private:

  // ---------------------------------------------------------------------
  // Log file related members.
  // ---------------------------------------------------------------------

  NABoolean fileIsGood_;         // true if a file has been specified
  char      logFileName_[1024];  // log file name
  fstream   fstream_;            // log file stream
  FILE     *file_;               // log file object

  CmpContextInfo::CmpContextClassType ciClass_;  // the class of compile instances
                                 // to which the compilation activities as
                                 // specified by NSK_DBG_<nnn> will be reported.
                                 // The values are defined by 
                                 // CmpContextInfo::setCompileInstanceClass.

  // ---------------------------------------------------------------------
  // Helper functions
  // ---------------------------------------------------------------------

  void showItemExpr( const ExprNode *tree = NULL,
                     const CascadesPlan *plan = NULL,
                     const char *prefix = "" );

  void showProperties( const ExprNode *tree = NULL,
                       const CascadesPlan *plan = NULL,
                       const char *prefix = "" );

  double getCost( const RelExpr *re, const CascadesPlan *plan );
  double getRows( const RelExpr *re, const CascadesPlan *plan );
  const NAString getGroups( const RelExpr *re );
  const NAString getType( const RelExpr *re, const CascadesPlan *plan );

  void showCharacteristicInputs( const RelExpr *re,
                                 const CascadesPlan *plan,
                                 const char *prefix = "" );
  void showCharacteristicOutputs( const RelExpr *re,
                                  const CascadesPlan *plan,
                                  const char *prefix = "" );
  void showConstraints( const RelExpr *re,
                        const CascadesPlan *plan,
                        const char *prefix = "" );
  void showContext( const RelExpr *re,
                    const CascadesPlan *plan,
                    const char *prefix = "" );
  void showCosts( const RelExpr *re,
                  const CascadesPlan *plan,
                  const char *prefix = "" );
  void showLogicalProperties( const RelExpr *re,
                              const CascadesPlan *plan,
                              const char *prefix = "" );
  void showPhysicalProperties( const RelExpr *re,
                               const CascadesPlan *plan,
                               const char *prefix = "" );

  void showEstLogProp( const EstLogPropSharedPtr& estLogProp,
                       const char * prefix = "" );

  void showCostInDetail( const NAString& header,
                         const Cost *cost,
                         const char *prefix = "" );
  void showSimpleCostVector( const char* header,
                             const SimpleCostVector &scv,
                             const char *prefix,
                             ostream &out );
  void showSimpleCostVectorDetail( const SimpleCostVector &scv,
                                   const char *prefix,
                                   ostream &out );

};  // class OptDebug

class AssertException;

class QueryOptimizerDriver
{
public:
  QueryOptimizerDriver(RelExpr *relExpr);
  ~QueryOptimizerDriver();
  RelExpr * doPass1Only(RelExpr *relExpr, Context *context);
  RelExpr * doPass1Pass2(RelExpr *relExpr, Context *context);
  RelExpr * doPass2PerhapsPass1(RelExpr *relExpr, Context *context,
				RelExpr *original);
  Context * initializeOptimization(RelExpr *relExpr);
  void resetOptimization();
private:
  void DEBUG_BREAK_ON_TASK();
  void DEBUG_SHOW_TASK(CascadesTask * task);
  void DEBUG_PRINT_COUNTERS();
  void DEBUG_RESET_PWSCOUNT();
  void DEBUG_SHOW_PLAN(Context *context);

  void DEBUG_GUI_DO_MEMO_STEP(CascadesTask *task);
  void DEBUG_GUI_SET_POINTERS();
  void DEBUG_GUI_DISPLAY_TRIGGERS_AFTER_NORMALIZATION(RelExpr *relExpr);
  void DEBUG_GUI_HIDE_QUERY_TREE();
  void DEBUG_GUI_DISPLAY_AFTER_OPTIMIZATION(Context *context);

  void TEST_ERROR_HANDLING();

  void initializeMonitors();
  RelExpr * bindSavedPass1Solution(RelExpr *relExpr, Context *context);
  void setupPassTwoHeuristics(Context *context);
  void optimizeAPass(Context *context);
  void optimizeAPassHelper(Context *context);
  void markFatalAndGenErrorNoPlan(RelExpr *relExpr, Context *context);
  void genWarnAssertPassTwo(AssertException & e);
  void genWarnNoPlanPassTwo();
  void genErrorNoPlan(RelExpr *relExpr, Context *context);

  NABoolean fatalExceptionOccurring_;
  NABoolean isStream_;
  NABoolean isCQS_;
  Lng32 taskCount_;
  Lng32 taskCountHold_;
  NABoolean printCounters_;
};

void opt_qsort(void *base, UInt32 num, UInt32 width, Int32 (*comp)(const void *, const void *));


// class to wrap some optimizer global variables. 
class OptGlobals
{
public:
   OptGlobals (NAHeap* heap_);

   // -----------------------------------------------------------------------
   // counters for capturing optimizer statistics
   // -----------------------------------------------------------------------

   ObjectCounter* contextCounter; 
   ObjectCounter* cascadesPlanCounter; 

   Lng32            duplicate_expr_count;

#ifdef DEBUG
  // ---------------------------------------------------------------------
  // Counter for number of PlanWorkSpace objects generated by the
  // optimizer for a particular SQL statement.
  // Used for debugging purposes only
  // ---------------------------------------------------------------------
  Lng32 planWorkSpaceCount;
#endif /* DEBUG */

   Lng32            logical_expr_count;
   Lng32            physical_expr_count;
   Lng32            plans_count;

   // ----------------------------------------------------------------
   // counters for evaluating ASM
   // -----------------------------------------------------------------
   Int32 asm_count;
   Int32 cascade_count;
   
   // TaskMonitor cascadesTasksMonitor[CascadesTask::NUMBER_OF_TASK_TYPES];
   TaskMonitor *cascadesTasksMonitor[CascadesTask::NUMBER_OF_TASK_TYPES];
   TaskMonitor *cascadesPassMonitor;
   
   // These monitor were introduced to collect statistics about how
   // much time ScanOptimizer spent in most important functions
   // for costing and how many times those functions were called
   
   // First 3 Monitors collect statistics about fileScanOptimizer.optimize(...)
   // in costmethod. fileScanMonitor gives total statistics, nestedJoinMonitor
   // - inside nested joins, indexJoinMonitor - inside index joins
   
   TaskMonitor     *fileScanMonitor;
   TaskMonitor     *nestedJoinMonitor;
   TaskMonitor     *indexJoinMonitor;
   
   // Next 3 Monitors collect statistics about computeCostForSingleSubset
   // in costmethod. singleSubsetMonitor gives total statistics,
   // singleVectorCostMonitor - inside computeCostVector,
   // singleObjectCostMonitor - inside computeCostObject
   
   TaskMonitor     *singleSubsetCostMonitor;
   TaskMonitor     *singleVectorCostMonitor;
   TaskMonitor     *singleObjectCostMonitor;
   
   // Next 3 Monitors collect statistics about computeCostForMultipleSubset
   // in costmethod. multSubsetMonitor gives total statistics,
   // multVectorCostMonitor - inside computeCostVectorForMultipleSubset,
   // multObjectCostMonitor - inside computeCostObject (only to compute
   // final cost object, cases when computeCostObject is called to compute
   // intermediate cost to check the costBound are not counted).
   
   TaskMonitor     *multSubsetCostMonitor;
   TaskMonitor     *multVectorCostMonitor;
   TaskMonitor     *multObjectCostMonitor;
   
   // This last monitor counts how many times a final cost object was
   // created for asynchronous access for both singleSubset and
   // multipleSubset. This is being counted in computeCostObject
   // function. To ignore all other calls (except final) for
   // computeCostObject synCheckFlag is used. (Inside computeCostFor
   // MultipleSubset computeCOstObject is called many times to
   // check costBound). It is set to TRUE right before the final call,
   // and - to FALSE right after it.
   
   TaskMonitor    *asynchrMonitor;
   NABoolean       synCheckFlag;
   
   // Controls DCMPASSERT delta < 0 for Linux
   NABoolean warningGiven;
   
   // -----------------------------------------------------------------------
   // command line options (initially set in sqlcomp.C)
   // -----------------------------------------------------------------------
   
   NABoolean       pruningIsFeasible;
   NABoolean       maxParIsFeasible;
   NABoolean       forceParallelInsertSelect;

   // Setting this to FALSE makes the optimizer print out statistics
   NABoolean       BeSilent;
   
   // ----------------------------------------------------------------------
   // A  global output string for debugging
   // ----------------------------------------------------------------------
   
   char returnString[500];      // global output string for debugging
   
   // for debugging only
   Int32             group_merge_count;
   Int32             garbage_expr_count;
   Int32             pruned_tasks_count;
   NABoolean         countExpr;

   CascadesMemo     * memo;
   CascadesTaskList * task_list;

private:
 NAHeap* heap_;
};
#endif // OPT_HDR
