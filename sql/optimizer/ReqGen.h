/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef REQGEN_H
#define REQGEN_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ReqGen.h
 * Description:  Class to generate required physical properties for a
 *               child of a RelExpr node. Something like an intelligent
 *               constructor for class ReqdPhysicalProperties.
 *
 * Created:      2/5/98
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "PhyProp.h"

// -----------------------------------------------------------------------
//  The following classes are defined in this file.
// -----------------------------------------------------------------------
class RequirementGenerator;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// class RequirementGenerator
//
// This class is used to generate a new ReqdPhysicalProperty object
// for a particular group from existing and new requirements.
// The RequirementGenerator provides intelligent methods for adding
// and removing requirements. The methods are also easy to use, making
// the code implementations of RelExpr::createContextForAChild easier. The
// RequirementGenerator performs all necessary checks that are needed
// to provide consistent and feasible requirements to a child group.
// Typically, a piece of code that needs to form a requirement will
// do this in three steps:
// - create an object of class RequirementGenerator (may be on the stack)
//   and possibly pass in some starting requirements
// - add or remove requirements
// - generate a ReqdPhysicalProperty object, if requirements are feasible
// Example: GroupByAggr::createContextForAChild
//   {
//     RequirementGenerator rg(child(0),<my Reqirements from context>);
//
//     if (<this is not a partial grby>)
//       rg.addPartitioningKey(<myGroupingCols>);
//
//     if (<this is a sort groupby>)
//       rg.addArrangement(<myGroupingCols>);
//
//     if (<this operator runs optimally in parallel>)
//       rg.addNumOfPartitions(<optimal # of parts>);
//
//     if (rg.isFeasible())
//       {
//         ReqdPhysicalProperty rppForChild = rg.produceRequirement();
//         <add cost limit and share context>
//       }
//   }
// -----------------------------------------------------------------------
class RequirementGenerator
{
public:

  // ---------------------------------------------------------------------
  // constructor: it takes a child of a RelExpr to get the child's
  // group attributes, and it takes an optional ReqdPhysicalProperty
  // object with starting requirements (which can be selectively
  // removed before adding other ones)
  // ---------------------------------------------------------------------
  RequirementGenerator(
      const ExprGroupId &groupForRequirement,
      const ReqdPhysicalProperty *startRequirements = NULL);

  // ---------------------------------------------------------------------
  // accessor methods
  // ---------------------------------------------------------------------
  inline Lng32 getCountOfPipelines() const
                             { return availableCPUs_ * pipelinesPerCPU_; }
  inline Lng32 getCountOfAvailableCPUs() const   { return availableCPUs_; }

  inline const ReqdPhysicalProperty* getStartRequirements() const
    { return startRequirements_; }

  // ---------------------------------------------------------------------
  // Methods to add new requirements to the RequirementGenerator.
  // The methods may be called multiple times, and the requirements
  // get added to the start requirements passed in the constructor
  // and the requirements. Use the checkFeasibility() method to
  // find out whether the different requirements contradict each other.
  // ---------------------------------------------------------------------
  void addSortKey(const ValueIdList &additionalSortKey,
               SortOrderTypeEnum sortOrderTypeReq = INCOMPATIBLE_SOT,
               PartitioningRequirement* dp2SortOrderPartReq = NULL);
  void addArrangement(const ValueIdSet &additionalArrangementParam,
               SortOrderTypeEnum sortOrderTypeReq = INCOMPATIBLE_SOT,
               PartitioningRequirement* dp2SortOrderPartReq = NULL);
  void addSortOrderTypeReq(SortOrderTypeEnum sortOrderTypeReq,
               PartitioningRequirement* dp2SortOrderPartReq = NULL);
  void addPartitioningKey(const ValueIdSet &partKey);
  void addNumOfPartitions(Lng32 newNumberOfPartitions,
                          float newNumOfPartsAllowedDeviation = 
                            CURRSTMT_OPTDEFAULTS->numberOfPartitionsDeviation());
  void addPartRequirement(PartitioningRequirement *pr);
  void addLocationRequirement(PlanExecutionEnum loc);
  void addLogicalPartRequirement(LogicalPartitioningRequirement *pr);
  void addPushDownRequirement(const PushDownRequirement* pdr)
   { pushDownRequirement_ = pdr; };
  void addNoEspExchangeRequirement();
  void addOcbCostingRequirement();
  // ---------------------------------------------------------------------
  // Methods to completely remove certain parts of the requirement.
  // These methods remove both the requirements that came from the
  // start requirements and those that were added.
  // ---------------------------------------------------------------------
  void removeSortKey();
  void removeExtraSortKeys();
  void removeArrangement();
  void removeSortOrderTypeReq();
  void removeAllPartitioningRequirements();
  void removePushDownRequirement() { pushDownRequirement_ = 0; };

  // ---------------------------------------------------------------------
  // Methods to replace or modify requirements.
  // ---------------------------------------------------------------------
  void replaceLocationRequirement(PlanExecutionEnum loc);
  void setLogicalOrderOrArrangementFlag(NABoolean newValue)
    { logicalOrderOrArrangement_ = newValue; }

  // ---------------------------------------------------------------------
  // Methods that help in generating "good" requirements in cases where
  // the RelExpr generating them has some freedom (e.g. by removing some
  // equi-join columns from a required arrangement or partitioning key).
  // These methods side-effect their parameters and remove some
  // parts of them that are in conflict with existing requirements.
  // ---------------------------------------------------------------------
  // makeSortKeyFeasible can be used to make an sort key feasible
  void makeSortKeyFeasible(ValueIdList &proposedSortKey);

  // makeArrangementFeasible can be used to make an arrangement feasible
  void makeArrangementFeasible(ValueIdSet &proposedArrangement);

  // modify the number of required partitions such that it doesn't conflict
  void makeNumOfPartsFeasible(Lng32 &proposedNumOfParts,
                        float *proposedNumOfPartsAllowedDeviation = NULL);


  // ---------------------------------------------------------------------
  // Methods to do consistency checking and to get information about
  // how the requirements look so far
  // ---------------------------------------------------------------------
  NABoolean checkFeasibility();

  // ---------------------------------------------------------------------
  // The final method to produce a ReqdPhysicalProperty object or a NULL
  // pointer if the requirements are not feasible (contradictory).
  // ---------------------------------------------------------------------
  ReqdPhysicalProperty *produceRequirement();

  static void initSortOrderTypeReqCompTab();

  SortOrderTypeEnum determineCompatibleSortOrderTypeReq(     
                    SortOrderTypeEnum otherSortOrderTypeReq) const;

  // Add the skew data handling requirement. If a fully specified partition
  // rquirement is to be added, it must be added earlier than the skew
  // requirement and any skewness property specified in that partition
  // requirement is overwritten by this skew requirement.
  void addSkewRequirement(skewProperty::skewDataHandlingEnum,
                          SkewedValueList* skList,
                          NABoolean broadcastOneRow);
  void removeSkewRequirement() { skewPropertyAdded_ = FALSE; };

  // Check that the added skew requirement can be made compatible with
  // what exists in the partR, and if so, modify partR so that the skew
  // requirement is properly represented in partR.
  void makeSkewReqFeasible(PartitioningRequirement* partR);

private:

  // private methods

  void processStartRequirements();
  void producePartitioningRequirements();
  void checkCompatibilityWithGroupAttributes();
  ValueIdSet combinePartKeysHeuristically(
     const ValueIdSet& partKey1,
     const ValueIdSet& partKey2);

  // private data members

  static THREAD_P LookupTable<SortOrderTypeEnum>* sortOrderTypeReqCompTab_;

  const ReqdPhysicalProperty     *startRequirements_;
  const GroupAttributes          *ga_;

  // data members that store the overall state of requirement generation
  NABoolean                      stillFeasible_;
  NABoolean                      startRequirementsHaveBeenProcessed_;
  ReqdPhysicalProperty           *generatedRequirement_;
  PartitioningRequirement        *generatedPartitioningRequirement_;
  NABoolean                      generatedPartitioningReqIsValid_;

  // Things that got added to the requirements (either through the add
  // methods above or through the start requirements in the constructor)
  // Also there are some flags that remember things still to be done,
  // such as adding start requirements and checking.

  // order
  ValueIdList                    *addedSortKey_;
  NABoolean                      sortKeyIsUnique_;
  NABoolean                      removeStartSortKey_;

  // simplified columns from required orders 
  ValueIdList                    simpleSortCols_;

  // arrangement
  ValueIdSet                     *addedArrangement_;
  NABoolean                      removeStartArrangement_;

  // simplified columns from required arrangements 
  ValueIdSet                     simpleArrangedCols_;

  // order or arrangement
  NABoolean                      logicalOrderOrArrangement_;

  // partitioning
  // "fuzzy" requirements
  ValueIdSet                     addedPartKey_;
  NABoolean                      partKeyHasBeenAdded_;
  Lng32                           addedNumberOfPartitions_;
  float                          addedNumOfPartsAllowedDeviation_;
  // fully specified partitioning requirement
  PartitioningRequirement  *addedPartitioningRequirement_;
  NABoolean                      removeStartPartitioningRequirements_;

  // location
  PlanExecutionEnum              addedLocation_;
  NABoolean                      locationHasBeenAdded_;
  NABoolean                      removeStartLocation_;
  
  // sort order type
  SortOrderTypeEnum              addedSortOrderTypeReq_;
  PartitioningRequirement*       addedDp2SortOrderPartReq_;

  // the other requirements that we don't check
  Lng32                           availableCPUs_;
  Lng32                           pipelinesPerCPU_;
  const CostWeight               *costWeight_;
  const PerformanceGoal          *perfGoal_;
  LogicalPartitioningRequirement *logicalPartReq_;
   
  const PushDownRequirement*	 pushDownRequirement_;

  skewProperty                   addedSkewProperty_;
  NABoolean                      skewPropertyAdded_;

  NABoolean                      requireNoESPExchange_;
  NABoolean                      requireOcbEnabledCosting_;
  NABoolean                      requireHash2Only_;
}; // RequirementGenerator

#endif
