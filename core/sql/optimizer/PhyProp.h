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
#ifndef PHYPROP_HDR
#define PHYPROP_HDR
/* -*-C++-*-
**************************************************************************
*
* File:         PhyProp.h
* Description:  Physical Properties and Cost
* Created:      4/10/94
* Language:     C++
*
*
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------
#include "SchemaDB.h"
#include "PartFunc.h"
#include "RelEnforcer.h"
#include "PartReq.h"
#include "RelEnforcer.h"
#include "opt.h"

#include "LookupTable.h"

// -----------------------------------------------------------------------
//  The following classes are defined in this file.
// -----------------------------------------------------------------------
class PhysicalProperty;
class ReqdPhysicalProperty;
class InputPhysicalProperty;
class CurrentFragmentBigMemoryProperty;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class GroupAttributes;
class CorrName;
class DP2CostDataThatDependsOnSPP;
class PushDownProperty;
class PushDownRequirement;
namespace tmudr {
  class UDRPlanInfo;
}

// -----------------------------------------------------------------------
// global defines
// -----------------------------------------------------------------------

// see class ReqdPhysicalProperty

#define DEFAULT_LOCATION     EXECUTE_IN_ESP
#define DEFAULT_SINGLETON    1
//#define DEFAULT_DATA_SOURCE  SOURCE_PERSISTENT_TABLE /* not used */


// -----------------------------------------------------------------------
// Physical Properties
// ===================
//
//    Physical properties are those that can be derived from physical
//    expressions.  Thus, these are properties that have a dependency
//    on the physical strategy/algorithm chosen to implement the 
//    query.  
// -----------------------------------------------------------------------

class PhysicalProperty : public NABasicObject
{

public:

  // ---------------------------------------------------------------------
  //  Constructors and destructor
  // ---------------------------------------------------------------------
  PhysicalProperty() 
    : sortOrderType_(NO_SOT),
      dp2SortOrderPartFunc_(NULL),
      actualPartFunc_(NULL), 
      location_(DEFAULT_LOCATION),  
      sourceOfData_(SOURCE_PERSISTENT_TABLE),
      indexDesc_(NULL),
      partSearchKey_(NULL),
      currentCountOfCPUs_(1),
      dp2CostInfo_(NULL),
      udrPlanInfo_(NULL),
      pushDownProperty_(NULL),
      memoryNeedsinCurrentFragment_(NULL),
      explodedOcbJoin_(FALSE),
      accessOnePartition_(FALSE),
      insertedDataIsSorted_(FALSE)
  {}
  
  PhysicalProperty(const ValueIdList &sortKey, 
                   SortOrderTypeEnum sortOrderType,
                   PartitioningFunction* dp2SortOrderPartFunc = NULL,
		   PartitioningFunction * actualPartFunc = NULL,
		   const PlanExecutionEnum plenum = DEFAULT_LOCATION, 
                   const DataSourceEnum dsenum = SOURCE_PERSISTENT_TABLE,
                   const IndexDesc * indexDesc = NULL,
		   const SearchKey * partSearchKey = NULL,
                   const PushDownProperty* pdp = NULL,
				   NABoolean explOcbJoin = FALSE
		   )
    : sortKey_(sortKey),
      sortOrderType_(sortOrderType),
      dp2SortOrderPartFunc_(dp2SortOrderPartFunc),
      actualPartFunc_(actualPartFunc), 
      location_(plenum),  
      sourceOfData_(dsenum),
      indexDesc_(indexDesc),
      partSearchKey_(partSearchKey),
      currentCountOfCPUs_(1),
      dp2CostInfo_(NULL),
      udrPlanInfo_(NULL),
      pushDownProperty_(pdp),
      memoryNeedsinCurrentFragment_(NULL),
      explodedOcbJoin_(explOcbJoin),
      accessOnePartition_(FALSE),
      insertedDataIsSorted_(FALSE)
  {}

  PhysicalProperty(PartitioningFunction * actualPartFunc,
		   const PlanExecutionEnum plenum = DEFAULT_LOCATION, 
		   const DataSourceEnum dsenum = SOURCE_PERSISTENT_TABLE
		   )
    : sortOrderType_(NO_SOT),
      dp2SortOrderPartFunc_(NULL),
      actualPartFunc_(actualPartFunc), 
      location_(plenum),  
      sourceOfData_(dsenum),
      indexDesc_(NULL),
      partSearchKey_(NULL),
      currentCountOfCPUs_(1),
      dp2CostInfo_(NULL),
      udrPlanInfo_(NULL),
      pushDownProperty_(NULL),
      memoryNeedsinCurrentFragment_(NULL),
      explodedOcbJoin_(FALSE),
      accessOnePartition_(FALSE),
      insertedDataIsSorted_(FALSE)
	{}

  PhysicalProperty(const PhysicalProperty &other)
    : sortKey_(other.sortKey_), 
      sortOrderType_(other.sortOrderType_),
      dp2SortOrderPartFunc_(other.dp2SortOrderPartFunc_),
      actualPartFunc_(other.actualPartFunc_), 
      location_(other.location_),  
      sourceOfData_(other.sourceOfData_),
      indexDesc_(other.indexDesc_),
      partSearchKey_(other.partSearchKey_),
      currentCountOfCPUs_(other.currentCountOfCPUs_),
      dp2CostInfo_(other.dp2CostInfo_),
      udrPlanInfo_(other.udrPlanInfo_),
      pushDownProperty_(other.pushDownProperty_),
      memoryNeedsinCurrentFragment_(other.memoryNeedsinCurrentFragment_),
      explodedOcbJoin_(other.explodedOcbJoin_),
      accessOnePartition_(other.accessOnePartition_),
      insertedDataIsSorted_(other.insertedDataIsSorted_)
  {}

  PhysicalProperty(const PhysicalProperty &other,
		   const ValueIdList &sortKey,
                   SortOrderTypeEnum sortOrderType,
                   PartitioningFunction* dp2SortOrderPartFunc 
                  )
    : sortKey_(sortKey), 
      sortOrderType_(sortOrderType),
      dp2SortOrderPartFunc_(dp2SortOrderPartFunc),
      actualPartFunc_(other.actualPartFunc_), 
      location_(other.location_),  
      sourceOfData_(other.sourceOfData_),
      indexDesc_(other.indexDesc_),
      partSearchKey_(other.partSearchKey_),
      currentCountOfCPUs_(other.currentCountOfCPUs_),
      dp2CostInfo_(other.dp2CostInfo_),
      udrPlanInfo_(other.udrPlanInfo_),
      pushDownProperty_(other.pushDownProperty_),
      memoryNeedsinCurrentFragment_(other.memoryNeedsinCurrentFragment_),
      explodedOcbJoin_(other.explodedOcbJoin_),
      accessOnePartition_(other.accessOnePartition_),
      insertedDataIsSorted_(other.insertedDataIsSorted_)
   {}

   PhysicalProperty(const PhysicalProperty &other,
		    const ValueIdList &sortKey,
                    SortOrderTypeEnum sortOrderType,
                    PartitioningFunction* dp2SortOrderPartFunc,
                    DataSourceEnum sourceOfData
                   )
    : sortKey_(sortKey), 
      sortOrderType_(sortOrderType),
      dp2SortOrderPartFunc_(dp2SortOrderPartFunc),
      actualPartFunc_(other.actualPartFunc_), 
      location_(other.location_),  
      sourceOfData_(sourceOfData),
      indexDesc_(other.indexDesc_),
      partSearchKey_(other.partSearchKey_),
      currentCountOfCPUs_(other.currentCountOfCPUs_),
      dp2CostInfo_(other.dp2CostInfo_),
      udrPlanInfo_(other.udrPlanInfo_),
      pushDownProperty_(other.pushDownProperty_),
      memoryNeedsinCurrentFragment_(other.memoryNeedsinCurrentFragment_),
      explodedOcbJoin_(other.explodedOcbJoin_),
      accessOnePartition_(other.accessOnePartition_),
      insertedDataIsSorted_(other.insertedDataIsSorted_)
   {}

  PhysicalProperty(const PhysicalProperty &other,
		   const ValueIdList &sortKey,
                   SortOrderTypeEnum sortOrderType,
                   PartitioningFunction* dp2SortOrderPartFunc,
		   PartitioningFunction* actualPartFunc
                  )
    : sortKey_(sortKey), 
      sortOrderType_(sortOrderType),
      dp2SortOrderPartFunc_(dp2SortOrderPartFunc),
      actualPartFunc_(actualPartFunc), 
      location_(other.location_),  
      sourceOfData_(other.sourceOfData_),
      indexDesc_(other.indexDesc_),
      partSearchKey_(other.partSearchKey_),
      currentCountOfCPUs_(other.currentCountOfCPUs_),
      dp2CostInfo_(other.dp2CostInfo_),
      udrPlanInfo_(other.udrPlanInfo_),
      pushDownProperty_(other.pushDownProperty_),
      memoryNeedsinCurrentFragment_(other.memoryNeedsinCurrentFragment_),
      explodedOcbJoin_(other.explodedOcbJoin_),
      accessOnePartition_(other.accessOnePartition_),
      insertedDataIsSorted_(other.insertedDataIsSorted_)
 
  {}


  virtual ~PhysicalProperty();
  
  // ---------------------------------------------------------------------
  // Accessor/Mutator functions
  // ---------------------------------------------------------------------

   const NABoolean getexplodedOcbJoinProperty() const
      { return explodedOcbJoin_; }

  // For determining if the partitioning key is a prefix of the
  // sort key (i.e. the clustering key) 
  NABoolean isPartKeyPrefixOfSortKey() const; // LCOV_EXCL_LINE

  // --- For sort key
  inline NABoolean isSorted() const   { return (sortKey_.entries() > 0); }
  inline const ValueIdList & getSortKey() const       { return sortKey_; }

  // accessors 
  inline SortOrderTypeEnum getSortOrderType() const
  { return sortOrderType_; }
  inline PartitioningFunction* getDp2SortOrderPartFunc() const
  { return dp2SortOrderPartFunc_; }

  // mutators
  void setSortOrderType(SortOrderTypeEnum sortOrderType)
  { 
    // Check that the synthesized sort order type is within the
    // allowable range for a synthesized sort order type.
    DCMPASSERT((sortOrderType >= NO_SOT) AND
               (sortOrderType <= DP2_SOT))
    sortOrderType_ = sortOrderType;
  }
  void setDp2SortOrderPartFunc(
                     PartitioningFunction* dp2SortOrderPartFunc)
  { dp2SortOrderPartFunc_ = dp2SortOrderPartFunc; }

  void setLocation(PlanExecutionEnum location)
  {location_ = location;}

  // --- For partitioning function
  inline NABoolean isPartitioned() const
  { return (actualPartFunc_ AND actualPartFunc_->getCountOfPartitions() > 1); }

  inline void scaleNumberOfPartitions(Lng32 npartns)
   { 
     if ( actualPartFunc_ )
        actualPartFunc_ = actualPartFunc_->scaleNumberOfPartitions(npartns); 
   }; 

  inline PartitioningFunction * getPartitioningFunction() const 
                                               { return actualPartFunc_; }
  inline Lng32 getCountOfPartitions() const
  {
    if (actualPartFunc_)
      return actualPartFunc_->getCountOfPartitions();
    else
      return 1;
  }

  // --- Read-only access to the partitioning key.
  inline const ValueIdSet & getPartitioningKey() const
                        { return  actualPartFunc_->getPartitioningKey(); }

  // --- For plan execution
  inline NABoolean executeInMasterOrESP() const 
                       { return (location_ == EXECUTE_IN_MASTER_OR_ESP); }
  inline NABoolean executeInMasterOnly() const 
                              { return (location_ == EXECUTE_IN_MASTER); }
  inline NABoolean executeInESPOnly() const 
                                 { return (location_ == EXECUTE_IN_ESP); }
  inline NABoolean executeInDP2() const 
                                 { return (location_ == EXECUTE_IN_DP2); }
  inline PlanExecutionEnum getPlanExecutionLocation() const
                                                     { return location_; }

  // -- For the source of the data
  inline DataSourceEnum getDataSourceEnum() const
                                                 { return sourceOfData_; }
  inline void setDataSourceEnum(DataSourceEnum s)
                                                    { sourceOfData_ = s; }

  // -- Accessor for the table descriptor
  inline const IndexDesc * getIndexDesc() const     { return indexDesc_; }
  inline void  setIndexDesc(const IndexDesc * val)   { indexDesc_ = val; }

  // -- Accessor and Mutator for the partition search key (part. key predicates)
  inline const SearchKey * getPartSearchKey() const
                                                { return partSearchKey_; }
  inline void setPartSearchKey(const SearchKey* partSearchKey)
                                       { partSearchKey_ = partSearchKey; }

  // -- Accessor and Mutator for cpuCountInDP2_.
  inline Lng32 getCurrentCountOfCPUs() const    { return currentCountOfCPUs_; }
  inline void setCurrentCountOfCPUs(const Lng32 value)
                                            { currentCountOfCPUs_ = value; }
				      
  // ---------------------------------------------------------------------
  // comparison functions (A > B means that A delivers all the
  // physical properties that B has plus some more).
  // ---------------------------------------------------------------------
  inline NABoolean operator > (const PhysicalProperty &other) const
                              { return compareProperties(other) == MORE; }
  inline NABoolean operator < (const PhysicalProperty &other) const
                              { return compareProperties(other) == LESS; }
  inline NABoolean operator == (const PhysicalProperty &other) const
                              { return compareProperties(other) == SAME; }

  // -----------------------------------------------------------------------
  // For costing data that gets computed at synthesis time:
  // -----------------------------------------------------------------------
  const DP2CostDataThatDependsOnSPP * getDP2CostThatDependsOnSPP() const
  { return dp2CostInfo_; }
  
  void setDP2CostThatDependsOnSPP(DP2CostDataThatDependsOnSPP * dp2Cost)
  { dp2CostInfo_ = dp2Cost; }

  // for UDRs (only to transfer this info to the code generator)
  tmudr::UDRPlanInfo *getUDRPlanInfo() const      { return udrPlanInfo_; }
  void setUDRPlanInfo(tmudr::UDRPlanInfo *pi)       { udrPlanInfo_ = pi; }
  
  // -----------------------------------------------------------------------
  // Make sure that all members are covered by the group attributes!
  //
  // Currently, we only check the sort key.  Someday we may check other
  // fields, such as the partitioning function.
  // -----------------------------------------------------------------------
  void enforceCoverageByGroupAttributes (const GroupAttributes * groupAttr) ;

  // ---------------------------------------------------------------------
  //  Utility functions
  // ---------------------------------------------------------------------
  void print(FILE* ofd = stdout,
	     const char * prefix = DEFAULT_INDENT,
             const char * suffix = "") const;

 
  void setPushDownProperty(const PushDownProperty* pdp) 
 	{ pushDownProperty_ = pdp; };
  const PushDownProperty* getPushDownProperty() const
	{ return pushDownProperty_; } ;

  void setBigMemoryEstimationProperty( CurrentFragmentBigMemoryProperty *c)
       { memoryNeedsinCurrentFragment_ = c;}

  const CurrentFragmentBigMemoryProperty * getBigMemoryEstimationProperty()
       { return memoryNeedsinCurrentFragment_; }

  void setAccessOnePartition(NABoolean x) { accessOnePartition_ = x; } ;
  NABoolean getAccessOnePartition() const { return accessOnePartition_; } ;

  void setInsertedDataIsSorted(NABoolean x) { insertedDataIsSorted_ = x; } ;
  NABoolean getInsertedDataIsSorted() const { return insertedDataIsSorted_; } ;

private:
  // ---------------------------------------------------------------------
  // Method for comparing two Physical property vectors.
  // ---------------------------------------------------------------------
  COMPARE_RESULT compareProperties(const PhysicalProperty & other) const;

  // ---------------------------------------------------------------------
  // Information about the order of the result rows
  // ---------------------------------------------------------------------
  ValueIdList  sortKey_;       // sorted by the specified cols/expressions

  // ---------------------------------------------------------------------
  // The type of sort order: NO_SOT, ESP_NO_SORT_SOT,
  // ESP_VIA_SORT_SOT, DP2_SOT
  // ---------------------------------------------------------------------
  SortOrderTypeEnum sortOrderType_;
  
  // ---------------------------------------------------------------------
  // The physical partitioning function of the access path that generated
  // the sort order. NULL if no sort order or if the sort order type is
  // not DP2.
  // ---------------------------------------------------------------------
  PartitioningFunction* dp2SortOrderPartFunc_;

  // ---------------------------------------------------------------------
  // The partitioning function encapsulates data parallelism.
  // When a plan fragment executes within DP2 the degree of operator
  // parallelism is equal to the number of partitions, i.e., the degree 
  // of data parallelism. In contrast, when a plan fragement executes
  // in ESP, the number of data streams, i.e., partitions, are expected
  // to be equal to the number of operator instances that are expected
  // to execute in parallel.
  // ---------------------------------------------------------------------
  PartitioningFunction * actualPartFunc_; 
  
  // ---------------------------------------------------------------------
  // Information about where the plan, or plan fragment, should be 
  // executed.
  // ---------------------------------------------------------------------
  PlanExecutionEnum  location_;
  
  // ---------------------------------------------------------------------
  // The source for the data, persistent table, temporary table or esp.
  // ---------------------------------------------------------------------
  DataSourceEnum  sourceOfData_;
  
  // ---------------------------------------------------------------------
  // The index which is the source of data. It points
  // to a table name only when sourceIsAPersistentTable() is TRUE.
  // Otherwise it is NULL.
  // This information is preserved until the first Exchange operator 
  // is reached.
  // ---------------------------------------------------------------------
  const IndexDesc * indexDesc_;

  // ---------------------------------------------------------------------
  // The search key to be used to determine partitions for indexDesc_.
  // This search key contains artificial predicates, designed to split
  // data into different streams when an operator is executed in parallel,
  // and any other predicates that may restrict the partititions used.
  // All predicates in partSearchKey_ have to be covered by the
  // characteristic inputs of the RelExpr that carries this object.
  // ---------------------------------------------------------------------
  const SearchKey * partSearchKey_;

  // ---------------------------------------------------------------------
  // No of CPU used if the operator executes in DP2. Meaningless if the
  // operator doesn't executes in DP2. This is for informational purpose
  // only. Should not be used to determine whether some ReqdPhysProperty
  // are satisfied.
  // ---------------------------------------------------------------------
  // $$$ This member should be moved to class
  // $$$ DP2CostDataThatDependsOnSPP!!!!
  Lng32 currentCountOfCPUs_;

  // -----------------------------------------------------------------------
  // The following class should contain all costing data for dp2
  // operators that gets computed at synthesis time:
  // -----------------------------------------------------------------------

  DP2CostDataThatDependsOnSPP * dp2CostInfo_;
  tmudr::UDRPlanInfo *udrPlanInfo_;

  const PushDownProperty* pushDownProperty_;

  CurrentFragmentBigMemoryProperty *memoryNeedsinCurrentFragment_;
  NABoolean explodedOcbJoin_;

  NABoolean accessOnePartition_;

  // set in DP2Insert::synthPhysicalProperty.
  // Indicates if the data to be inserted is sorted.
  NABoolean insertedDataIsSorted_;
}; // PhysicalProperty

// -----------------------------------------------------------------------
// Performance goals
//
// Optimize for N rows
//   Choose a plan that offers the lowest cost for returning the 
//   first N rows.
//
// Optimize for first row
//   Choose a plan that offers that offers the lowest first row cost
//
// Optimize for last row
//   Choose a plan that offers that offers the lowest last row cost
//
// Optimize for resource consumption
//   Choose a plan that offers that offers the lowest total cost
//
// -----------------------------------------------------------------------
class PerformanceGoal
{
public:
  virtual ~PerformanceGoal();
  
  virtual NABoolean isOptimizeForFirstRow() const;

  virtual NABoolean isOptimizeForLastRow() const;

  virtual NABoolean isOptimizeForResourceConsumption() const;

private:
}; // class PerformanceGoal

// -- Optimize for N rows
class OptimizeForNRows : public PerformanceGoal
{
public:
  OptimizeForNRows(Lng32 numberOfRows) : rowCount_(numberOfRows) 
                                            { assert(numberOfRows > 0); }
  virtual ~OptimizeForNRows() {}

  virtual NABoolean isOptimizeForFirstRow() const;
  
private:  
  Lng32 rowCount_;
}; // class OptimizeForNRows

// -- Optimize for first row - not used in SQ
// LCOV_EXCL_START
class OptimizeForFirstRow : public OptimizeForNRows
{
public:
  OptimizeForFirstRow() : OptimizeForNRows(1)                          { }

  virtual ~OptimizeForFirstRow() {}

}; // class OptimizeForFirstRow
// LCOV_EXCL_STOP

// -- Optimize for last row
class OptimizeForLastRow : public OptimizeForNRows
{
public:
  OptimizeForLastRow() : OptimizeForNRows(INT_MAX)                    { }
  
  virtual ~OptimizeForLastRow() {}

  virtual NABoolean isOptimizeForLastRow() const;
  virtual NABoolean isOptimizeForFirstRow() const;

}; // class OptimizeForLastRow

// -- Optimize for resource consumption - not used in SQ
// LCOV_EXCL_START
class OptimizeForResourceConsumption : public PerformanceGoal
{
public:
  virtual ~OptimizeForResourceConsumption() {}

  virtual NABoolean isOptimizeForResourceConsumption() const;

}; // class OptimizeForResourceConsumption
// LCOV_EXCL_STOP


// -----------------------------------------------------------------------
// Required physical properties
// -----------------------------------------------------------------------

class ReqdPhysicalProperty  : public NABasicObject
{

public:

  // ---------------------------------------------------------------------
  //  Constructor, copy constructor, destructor, and assignment operator
  // ---------------------------------------------------------------------

// warning elimination (removed "inline")
  ReqdPhysicalProperty(
           const ValueIdSet* const arrangedBy = NULL,
           const ValueIdList* const orderedBy = NULL,
           SortOrderTypeEnum sortOrderTypeReq = NO_SOT,
           PartitioningRequirement* dp2SortOrderPartReq = NULL,
           const NABoolean logicalOrderOrArrangement = FALSE,
           PartitioningRequirement* partReq = NULL,
           LogicalPartitioningRequirement* logicalPartReq = NULL,
           const PlanExecutionEnum location = DEFAULT_LOCATION,
           const Lng32 availableCPUs = DEFAULT_SINGLETON,
           const Lng32 pipelinesPerCPU = DEFAULT_SINGLETON,
           const CostWeight* const costWeight = CURRSTMT_OPTDEFAULTS->getDefaultCostWeight(),
           const PerformanceGoal* const perfGoal = CURRSTMT_OPTDEFAULTS->getDefaultPerformanceGoal(),
           RelExpr* mustMatch = NULL,
	   PushDownRequirement* pdp = NULL
  )
    : arrangedBy_(arrangedBy), orderedBy_(orderedBy),
      sortOrderTypeReq_(sortOrderTypeReq),
      dp2SortOrderPartReq_(dp2SortOrderPartReq),
      logicalOrderOrArrangement_(logicalOrderOrArrangement),
      partReq_(partReq),
      logicalPartReq_(logicalPartReq),
      location_(location),
      availableCPUs_(availableCPUs),
      pipelinesPerCPU_(pipelinesPerCPU),
      costWeight_(costWeight),
      perfGoal_(perfGoal),
      mustMatch_(mustMatch),
      pushDownRequirement_(pdp),
      noEspExchangeRequirement_(FALSE),
	  ocbEnabledCostingRequirement_(FALSE)
  {
#ifndef NDEBUG
    performConsistencyCheck();
#endif
  }

  // ---------------------------------------------------------------------
  // NOTE: mustMatch_ is not propagated from one reqd prop to another
  // ---------------------------------------------------------------------
  inline ReqdPhysicalProperty(const ReqdPhysicalProperty& other)
    : arrangedBy_(other.arrangedBy_), orderedBy_(other.orderedBy_), 
      sortOrderTypeReq_(other.sortOrderTypeReq_),
      dp2SortOrderPartReq_(other.dp2SortOrderPartReq_),
      logicalOrderOrArrangement_(other.logicalOrderOrArrangement_),
      partReq_(other.partReq_), 
      logicalPartReq_(other.logicalPartReq_),
      location_(other.location_),
      availableCPUs_(other.availableCPUs_), 
      pipelinesPerCPU_(other.pipelinesPerCPU_),
      costWeight_(other.costWeight_),
      perfGoal_(other.perfGoal_),
      mustMatch_(NULL),
      pushDownRequirement_(other.pushDownRequirement_),
      noEspExchangeRequirement_(other.noEspExchangeRequirement_),
	  ocbEnabledCostingRequirement_(other.ocbEnabledCostingRequirement_)
  {
#ifndef NDEBUG
    performConsistencyCheck();
#endif
  }

  // ---------------------------------------------------------------------
  // NOTE: mustMatch_ is not propagated from one reqd prop to another
  // ---------------------------------------------------------------------
  inline ReqdPhysicalProperty(const ReqdPhysicalProperty& other,
                  const ValueIdSet* const arrangedBy,
                  const ValueIdList* const orderedBy,
                  SortOrderTypeEnum sortOrderTypeReq,
                  PartitioningRequirement* dp2SortOrderPartReq
                             )
    : arrangedBy_(arrangedBy), orderedBy_(orderedBy), 
      sortOrderTypeReq_(sortOrderTypeReq),
      dp2SortOrderPartReq_(dp2SortOrderPartReq),
      logicalOrderOrArrangement_(other.logicalOrderOrArrangement_),
      partReq_(other.partReq_), 
      logicalPartReq_(other.logicalPartReq_),
      location_(other.location_),
      availableCPUs_(other.availableCPUs_), 
      pipelinesPerCPU_(other.pipelinesPerCPU_),
      costWeight_(other.costWeight_),
      perfGoal_(other.perfGoal_),
      mustMatch_(NULL),
      pushDownRequirement_(other.pushDownRequirement_),
      noEspExchangeRequirement_(other.noEspExchangeRequirement_),
	  ocbEnabledCostingRequirement_(other.ocbEnabledCostingRequirement_)
  {
#ifndef NDEBUG
    performConsistencyCheck();
#endif
  }

  // ---------------------------------------------------------------------
  // NOTE: mustMatch_ is not propagated from one reqd prop to another
  // NOTE: The logicalPartitionBoundariesFlag_ is not propagated from one
  // NOTE: required property to another when the partitioning requirement
  // NOTE: is changed
  // ---------------------------------------------------------------------
// warning elimination (removed "inline")
  ReqdPhysicalProperty(const ReqdPhysicalProperty& other,
                  const ValueIdSet* const arrangedBy,
                  const ValueIdList* const orderedBy,
                  SortOrderTypeEnum sortOrderTypeReq,
                  PartitioningRequirement* dp2SortOrderPartReq,
                  PartitioningRequirement* partReq
			     )
    : arrangedBy_(arrangedBy), orderedBy_(orderedBy), 
      sortOrderTypeReq_(sortOrderTypeReq),
      dp2SortOrderPartReq_(dp2SortOrderPartReq),
      logicalOrderOrArrangement_(other.logicalOrderOrArrangement_),
      partReq_(partReq), 
      logicalPartReq_(other.logicalPartReq_),
      location_(other.location_),
      availableCPUs_(other.availableCPUs_), 
      pipelinesPerCPU_(other.pipelinesPerCPU_),
      costWeight_(other.costWeight_),
      perfGoal_(other.perfGoal_),
      mustMatch_(NULL),
      pushDownRequirement_(other.pushDownRequirement_),
      noEspExchangeRequirement_(other.noEspExchangeRequirement_),
	  ocbEnabledCostingRequirement_(other.ocbEnabledCostingRequirement_)
  {
#ifndef NDEBUG
    performConsistencyCheck();
#endif
  }

  // ---------------------------------------------------------------------
  // NOTE: mustMatch_ is not propagated from one reqd prop to another
  // NOTE: The logicalPartitionBoundariesFlag_ is not propagated from one
  // NOTE: required property to another when the partitioning requirement
  // NOTE: is changed
  // ---------------------------------------------------------------------
// warning elimination (removed "inline")
  ReqdPhysicalProperty(const ReqdPhysicalProperty &other,
                  const ValueIdSet* const arrangedBy,
                  const ValueIdList* const orderedBy,
                  SortOrderTypeEnum sortOrderTypeReq,
                  PartitioningRequirement* dp2SortOrderPartReq,
                  PartitioningRequirement* partReq,
                  const PlanExecutionEnum location
			     )
    : arrangedBy_(arrangedBy), orderedBy_(orderedBy), 
      sortOrderTypeReq_(sortOrderTypeReq),
      dp2SortOrderPartReq_(dp2SortOrderPartReq),
      logicalOrderOrArrangement_(other.logicalOrderOrArrangement_),
      partReq_(partReq), 
      logicalPartReq_(other.logicalPartReq_),
      location_(location),
      availableCPUs_(other.availableCPUs_), 
      pipelinesPerCPU_(other.pipelinesPerCPU_),
      costWeight_(other.costWeight_),
      perfGoal_(other.perfGoal_),
      mustMatch_(NULL),
      pushDownRequirement_(other.pushDownRequirement_),
      noEspExchangeRequirement_(other.noEspExchangeRequirement_),
	  ocbEnabledCostingRequirement_(other.ocbEnabledCostingRequirement_)
  {
#ifndef NDEBUG
    performConsistencyCheck();
#endif
  }

  // ---------------------------------------------------------------------
  // NOTE: mustMatch_ is not propagated from one reqd prop to another
  // NOTE: The logicalPartitionBoundariesFlag_ is not propagated from one
  // NOTE: required property to another when the partitioning requirement
  // NOTE: is changed
  // ---------------------------------------------------------------------
// warning elimination (removed "inline")
  ReqdPhysicalProperty(const ReqdPhysicalProperty &other,
                  const ValueIdSet* const arrangedBy,
                  const ValueIdList* const orderedBy,
                  SortOrderTypeEnum sortOrderTypeReq,
                  PartitioningRequirement* dp2SortOrderPartReq,
                  PartitioningRequirement* partReq,
                  const PlanExecutionEnum location,
                  const Lng32 availableCPUs,
                  const Lng32 pipelinesPerCPU
			     )
    : arrangedBy_(arrangedBy), orderedBy_(orderedBy), 
      sortOrderTypeReq_(sortOrderTypeReq),
      dp2SortOrderPartReq_(dp2SortOrderPartReq),
      logicalOrderOrArrangement_(other.logicalOrderOrArrangement_),
      partReq_(partReq), 
      logicalPartReq_(other.logicalPartReq_),
      location_(location),
      availableCPUs_(availableCPUs), 
      pipelinesPerCPU_(pipelinesPerCPU),
      costWeight_(other.costWeight_),
      perfGoal_(other.perfGoal_),
      mustMatch_(NULL),
      pushDownRequirement_(other.pushDownRequirement_),
      noEspExchangeRequirement_(other.noEspExchangeRequirement_),
	  ocbEnabledCostingRequirement_(other.ocbEnabledCostingRequirement_)
  {
#ifndef NDEBUG
    performConsistencyCheck();
#endif
  }

  // create an empty requirement with only a must match in it
// warning elimination (removed "inline")
  ReqdPhysicalProperty(RelExpr* mustMatch)
    : arrangedBy_(NULL), orderedBy_(NULL),
      sortOrderTypeReq_(NO_SOT),
      dp2SortOrderPartReq_(NULL),
      logicalOrderOrArrangement_(FALSE),
      partReq_(NULL),
      logicalPartReq_(NULL),
      location_(DEFAULT_LOCATION),
      availableCPUs_(DEFAULT_SINGLETON),
      pipelinesPerCPU_(DEFAULT_SINGLETON),
      perfGoal_(CURRSTMT_OPTDEFAULTS->getDefaultPerformanceGoal()),
      costWeight_(CURRSTMT_OPTDEFAULTS->getDefaultCostWeight()),
      mustMatch_(mustMatch),
      pushDownRequirement_(NULL),
      noEspExchangeRequirement_(FALSE),
	  ocbEnabledCostingRequirement_(FALSE)
  {
#ifndef NDEBUG
    performConsistencyCheck();
#endif
  } 

  // ---------------------------------------------------------------------
  // NOTE: mustMatch_ is not propagated from one reqd prop to another
  // NOTE: This constructor is only used to change the "must match" 
  //       attribute of an operator's rpp. This constructor is never
  //       used when creating a new rpp for a child.
  // ---------------------------------------------------------------------
// warning elimination (removed "inline")
  ReqdPhysicalProperty(const ReqdPhysicalProperty& other,
                              RelExpr* mustMatch
                             )
    : arrangedBy_(other.arrangedBy_), orderedBy_(other.orderedBy_), 
      sortOrderTypeReq_(other.sortOrderTypeReq_),
      dp2SortOrderPartReq_(other.dp2SortOrderPartReq_),
      logicalOrderOrArrangement_(other.logicalOrderOrArrangement_),
      partReq_(other.partReq_), 
      logicalPartReq_(other.logicalPartReq_),
      location_(other.location_),
      availableCPUs_(other.availableCPUs_), 
      pipelinesPerCPU_(other.pipelinesPerCPU_),
      costWeight_(other.costWeight_),
      perfGoal_(other.perfGoal_),
      mustMatch_(mustMatch),
      pushDownRequirement_(other.pushDownRequirement_),
      noEspExchangeRequirement_(other.noEspExchangeRequirement_),
	  ocbEnabledCostingRequirement_(other.ocbEnabledCostingRequirement_)
  {
#ifndef NDEBUG
    performConsistencyCheck();
#endif
  }

  virtual ~ReqdPhysicalProperty();

  // --------------------------------------------------------------------
  // does a given plan satisfy the required property
  // ---------------------------------------------------------------------
  NABoolean satisfied(EstLogPropSharedPtr inputLogProp,
                      const RelExpr * const physExpr,
		      const PhysicalProperty * const physProp) const;

  // ---------------------------------------------------------------------
  // Comparison functions between two required phys. properties
  //
  // Return:
  //
  // LESS          if "this" required physical property requires LESS
  //               than the "other" required property
  // SAME          if both properties require the SAME props
  // MORE          if "this" required physical property requires MORE
  //               than the "other" required property
  // INCOMPATIBLE  if the properties are incompatible (A and B are incom-
  //               patible => no physical property exists that satisfies
  //               both A and B or the two contexts have incompatible
  //               optimization goals, such as optimization for n rows
  //               or statistics for input values)
  // UNDEFINED     can't tell which requires more or less (properties may
  //               in some subtle cases still be incompatible)
  // ---------------------------------------------------------------------
  COMPARE_RESULT compareRequirements(const ReqdPhysicalProperty & other) const;

  // ---------------------------------------------------------------------
  // Accessor/Mutator functions
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Check whether any requirements are specified at all.
  // ---------------------------------------------------------------------
  NABoolean isEmpty() const;

  // ---------------------------------------------------------------------
  // User-specified cost weight.
  // ---------------------------------------------------------------------
  inline const CostWeight* getCostWeight() const { return costWeight_; }

  // ---------------------------------------------------------------------
  // User-specified performance goal.
  // ---------------------------------------------------------------------
  inline const PerformanceGoal* getPerformanceGoal() const
                                                     { return perfGoal_; }
  // ---------------------------------------------------------------------
  // Requirements related to imposing an order on the result.
  // ---------------------------------------------------------------------
  inline const ValueIdSet * getArrangedCols() const { return arrangedBy_; }
  inline const ValueIdList * getSortKey() const     { return orderedBy_; }
  static void initSortOrderTypeCompTab();
  static void initSortOrderTypeContextCompTab();

  //accessors  
  SortOrderTypeEnum getSortOrderTypeReq() const
  { return sortOrderTypeReq_; }
  PartitioningRequirement* getDp2SortOrderPartReq() const
  { return dp2SortOrderPartReq_; }

  

  // mutators
  void setSortOrderTypeReq (SortOrderTypeEnum sortOrderTypeReq)
  { 
    // Check that the required sort order type is within the
    // allowable range for a required sort order type.
    DCMPASSERT((sortOrderTypeReq >= NO_SOT) AND
               (sortOrderTypeReq <= DP2_OR_ESP_NO_SORT_SOT))
    sortOrderTypeReq_ = sortOrderTypeReq;
  }
  void setDp2SortOrderPartReq 
         (PartitioningRequirement* dp2SortOrderPartReq)
  { dp2SortOrderPartReq_ = dp2SortOrderPartReq; }

  NABoolean sortOrderTypeReqAndSynthCompatible
              (const SortOrderTypeEnum synthesizedSortOrderType) const;
  COMPARE_RESULT compareSortOrderTypeReqToReq
              (const SortOrderTypeEnum otherSortOrderTypeReq) const;
  NABoolean dp2SortOrderPartReqAndSynthCompatible
              (const PhysicalProperty* const spp) const;

  inline NABoolean getLogicalOrderOrArrangementFlag() const
                                    { return logicalOrderOrArrangement_; }
  inline void setLogicalOrderOrArrangementFlag(const NABoolean newValue)
                                { logicalOrderOrArrangement_ = newValue; }


  // ---------------------------------------------------------------------
  // Location for plan execution
  // ---------------------------------------------------------------------
  PlanExecutionEnum getPlanExecutionLocation() const
                                                     { return location_; }
  NABoolean executeInDP2() const { return (location_ == EXECUTE_IN_DP2); }

  // ---------------------------------------------------------------------
  // The number of pipelines that can be formed for executing a
  // plan fragment in parallel without (outside of) a DP2.
  // ---------------------------------------------------------------------
  inline Lng32 getCountOfPipelines() const
                             { return availableCPUs_ * pipelinesPerCPU_; }
  inline Lng32 getCountOfAvailableCPUs() const   { return availableCPUs_; }

  // ---------------------------------------------------------------------
  // The partitions, i.e., the number of data streams that should
  // be produced. This is specifed by means of a partitioning requirement.
  // ---------------------------------------------------------------------
  inline NABoolean requiresPartitioning() const 
                                           { return (partReq_ != NULL); }

  inline PartitioningRequirement * getPartitioningRequirement() const 
                                                     { return partReq_; }
  Lng32 getCountOfPartitions() const
                             { return partReq_->getCountOfPartitions(); }

  // --- Read-only access to the partitioning key.
  inline const ValueIdSet & getPartitioningKey() const
                              { return  partReq_->getPartitioningKey(); }

  // ---------------------------------------------------------------------
  // logical partitioning requirement for DP2 fragments
  // ---------------------------------------------------------------------
  LogicalPartitioningRequirement * getLogicalPartRequirement() const
                                               { return logicalPartReq_; }
  void setLogicalPartRequirement(LogicalPartitioningRequirement *lpr)
                                                { logicalPartReq_ = lpr; }

  // ---------------------------------------------------------------------
  // A pattern supplied for enforcing a certain shape on the query
  // tree of the plan.
  // ---------------------------------------------------------------------
  inline const RelExpr * getMustMatch() const       { return mustMatch_; }

  RelExpr* getInputMustMatch(Lng32 childIndex) const; // LCOV_EXCL_LINE


  void setPushDownRequirement(const PushDownRequirement* pdp) 
 	{ pushDownRequirement_ = pdp; };
  const PushDownRequirement* getPushDownRequirement() const
	{ return pushDownRequirement_ ; } ;
  NABoolean getNoEspExchangeRequirement() const
        { return noEspExchangeRequirement_; } ;
  void setNoEspExchangeRequirement(const NABoolean b)
       { noEspExchangeRequirement_ = b; };
  NABoolean getOcbEnabledCostingRequirement() const
        { return ocbEnabledCostingRequirement_; } ;
  void setOcbEnabledCostingRequirement(const NABoolean b)
       { ocbEnabledCostingRequirement_ = b; };
  // ---------------------------------------------------------------------
  //  Utility functions
  // ---------------------------------------------------------------------
  void print(FILE* ofd = stdout,
	     const char * prefix = DEFAULT_INDENT,
             const char * suffix = "") const;

private:

  // ---------------------------------------------------------------------
  // PRIVATE METHODS
  // ---------------------------------------------------------------------
  void performConsistencyCheck() const
  { 
    DCMPASSERT((availableCPUs_ > 0) AND 
               (pipelinesPerCPU_ > 0) AND
               (costWeight_ != NULL) AND 
               (perfGoal_ != NULL) );

#ifndef NDEBUG
    performSortOrderTypeConsistencyCheck();
#endif
  }

  void performSortOrderTypeConsistencyCheck() const
  {
    if ((arrangedBy_ == NULL) AND (orderedBy_ == NULL))
    {
      DCMPASSERT(sortOrderTypeReq_ == NO_SOT);
      DCMPASSERT(dp2SortOrderPartReq_ == NULL);
    }
    else
    {
      DCMPASSERT((sortOrderTypeReq_ != NO_SOT) OR
                 (location_ == EXECUTE_IN_DP2));
    }

    if ((dp2SortOrderPartReq_ != NULL) OR
        (sortOrderTypeReq_ == DP2_OR_ESP_NO_SORT_SOT) OR
        (sortOrderTypeReq_ == DP2_SOT))
    {
      DCMPASSERT(dp2SortOrderPartReq_ != NULL);
      DCMPASSERT((location_ == EXECUTE_IN_DP2 AND sortOrderTypeReq_ == NO_SOT) OR (sortOrderTypeReq_ == DP2_OR_ESP_NO_SORT_SOT) OR
                 (sortOrderTypeReq_ == DP2_SOT));
    }
  } // performSortOrderTypeConsistencyCheck()

  // ---------------------------------------------------------------------
  // PRIVATE DATA
  // ---------------------------------------------------------------------
  static THREAD_P LookupTable<NABoolean>* sortOrderTypeCompTab_;
  static THREAD_P LookupTable<COMPARE_RESULT>* sortOrderTypeContextCompTab_;

  const CostWeight* const costWeight_;    // weight used for comparing costs

  const PerformanceGoal* const perfGoal_; // the performance goal

  // ---------------------------------------------------------------------
  // The output rows are required to be "arranged" such that they are
  // sorted and the set of "arrangedBy_" values forms a prefix of the
  // sort key.
  // ---------------------------------------------------------------------
  const ValueIdSet* const arrangedBy_;

  // ---------------------------------------------------------------------
  // The output rows are required to be ordered by the given columns.
  // ---------------------------------------------------------------------
  const ValueIdList* const orderedBy_;

  // ---------------------------------------------------------------------
  // The type of sorted order needed: NO_SOT, ESP_NO_SORT_SOT, 
  // ESP_VIA_SORT_SOT, DP2_SOT, ESP_SOT, DP2_OR_ESP_NO_SORT_SOT
  // ---------------------------------------------------------------------
  SortOrderTypeEnum sortOrderTypeReq_;

  // ---------------------------------------------------------------------
  // The required physical partitioning for a sort order type of DP2 to
  // be valid.
  // ---------------------------------------------------------------------
  PartitioningRequirement* dp2SortOrderPartReq_;

  // ---------------------------------------------------------------------
  // Indicates whether any required order or arrangement is a logical one.
  // This flag will be TRUE if the current location is DP2, and there is
  // a required order or arrangement that came from an operator that is
  // not in DP2. A logical order or arrangement requirement has 
  // different rules as to whether the spp satisfies it. See the     
  // "satisfied" method for details.
  // ---------------------------------------------------------------------
  NABoolean logicalOrderOrArrangement_;

  // ---------------------------------------------------------------------
  // The number of instances of a plan fragment that shall execute
  // concurrently, in parallel.
  // ---------------------------------------------------------------------
  const Lng32 availableCPUs_;
  const Lng32 pipelinesPerCPU_;

  // ---------------------------------------------------------------------
  // Specification of the location where the plan must execute.
  // ---------------------------------------------------------------------
  const PlanExecutionEnum location_;

  // ---------------------------------------------------------------------
  // The partition requirement describes the characteristics of the 
  // data streams that are to be produced as output by using a 
  // built-in (system-defined) partitioning requirement. The destination
  // for the data streams could either be a partitioned file set or 
  // even be a set of ESP's. The absence of a partitioning requirement
  // implies that a single output stream must be produced.
  //
  // partReq_->getCountOfPartitions() specifies the number
  // of data streams that must be produced as output. The distribution
  // of rows amongst these data streams is dependant upon information
  // contained in the partitioning requirement.
  //
  // partReq_->getPartitioningKey() yields a set of expressions that 
  // form the partitioning key. The application of the partitioning
  // function to the values contained in the partitioning key columns 
  // decides the data stream (partition number) to which a certain row
  // is assigned.
  // ---------------------------------------------------------------------
  PartitioningRequirement* partReq_;

  // ---------------------------------------------------------------------
  // The logical partitioning requirement applies to contexts that
  // execute in DP2. In this case, logicalPartReq_ describes the
  // requirements of the PA or PAPA exchange above. This helps the
  // DP2 leaf node (scan, insert, update, delete) to perform "logical
  // partitioning", meaning to deal with the fact that multiple PAs or
  // ESPs will share the work for the operation.
  // ---------------------------------------------------------------------
  LogicalPartitioningRequirement *logicalPartReq_;
       
  // ---------------------------------------------------------------------
  // Requirement for matching a given tree shape
  // ---------------------------------------------------------------------
  RelExpr* mustMatch_;

  // PushDown requirement, if any.
  const PushDownRequirement* pushDownRequirement_;

  NABoolean noEspExchangeRequirement_;
  NABoolean ocbEnabledCostingRequirement_;

}; // ReqdPhysicalProperty

// this class collects fragment-wise memory related information
class CurrentFragmentBigMemoryProperty : public NABasicObject
{
public:
   CurrentFragmentBigMemoryProperty():
    currentFileSize_(0),
    cumulativeFileSizeofFragment_(0),
    noBigMemOpsSoFar_(0),
    Op_(NO_OPERATOR_TYPE)
   {};
   virtual ~CurrentFragmentBigMemoryProperty() {};

   void setCurrentFileSize(double d)
    { currentFileSize_=d;
      cumulativeFileSizeofFragment_=d;}

   double getCumulativeFileSize() 
   { return cumulativeFileSizeofFragment_; }

   void incrementCumulativeMemSize(double d)
   { cumulativeFileSizeofFragment_+=d;}

   void setOperatorType(OperatorTypeEnum op)
    { Op_ = op;}

private:
   double currentFileSize_;
   double cumulativeFileSizeofFragment_;
   Int32    noBigMemOpsSoFar_;
   OperatorTypeEnum    Op_;
};


// required push down property

class PushDownCSProperty;
class PushDownColocationProperty;

//
// PushDown property abstract class, which defines the interface
// for producing push-down property and generating push-down 
// requirement. 
//
class PushDownProperty : public NABasicObject
{

public:
   PushDownProperty() {};
   virtual ~PushDownProperty() {};

   // is this propery fully specified?
   virtual NABoolean isEmpty() const = 0;

   // generating a requirement from a propery.
   // used by various createContextForAChild() to echo the
   // requirement to the right child, given the push-down
   // property from the left child.
   virtual const PushDownRequirement* makeRequirement() const = 0;

   // dynamic type checking help function for PushDownColocationProperty
   virtual const PushDownColocationProperty* 
      castToPushDownColocationProperty() const { return NULL; };

   // dynamic type checking help function for PushDownCSProperty
   virtual const
   PushDownCSProperty* castToPushDownCSProperty() const { return NULL; };

private:
};

//
// Push-down colocation checking property.
//
class PushDownColocationProperty : public PushDownProperty
{

public:
   PushDownColocationProperty(const NodeMap* map = 0) : map_(map) {};
   virtual ~PushDownColocationProperty() {};

   const NodeMap* getNodeMap() const { return map_; };
   NABoolean isEmpty() const { return map_ == 0; };

   // generate a PushDownColocationRequirement from the property.
   const PushDownRequirement* makeRequirement() const;

   const PushDownColocationProperty* 
      castToPushDownColocationProperty() const { return this; };

private:
   const NodeMap* map_;
};

//
// Compound statement push down property.
//
class PushDownCSProperty : public PushDownColocationProperty
{

public:
   PushDownCSProperty(const PartitioningFunction* func = 0,
                    const SearchKey* key = 0) :
      PushDownColocationProperty( (func)? func->getNodeMap() : 0),
      partFunc_(func), searchKey_(key) {};
   ~PushDownCSProperty() {};

   // get data members
   const SearchKey* getSearchKey() const { return searchKey_; };
   const PartitioningFunction* getPartFunc() const { return partFunc_; };

   // partFunc will never be NULL for a concrete CS push-down
   // property. searchKey could be empty (e.g., "select * from t" 
   // where t is a single partitioned table)
   NABoolean isEmpty() const { return partFunc_ == 0; };

   // generating a CS push-down requirement.
   const PushDownRequirement* makeRequirement() const;

   // dynamic type checking help function
   const PushDownCSProperty* castToPushDownCSProperty() const { return this; };

private:
   const PartitioningFunction* partFunc_;
   const SearchKey* searchKey_;
};


// Push down requirement (physical always)
class PushDownCSRequirement;
class PushDownColocationRequirement;

//
// PushDown requirement abstract class, which defines the interface
// for verifying whether a push-down property satisfies a push-down 
// requirement.
//
class PushDownRequirement : public NABasicObject
{
public:
   PushDownRequirement() {};
   virtual ~PushDownRequirement() {};

   // check the null state of the requirement (is the requirement fully
   // specified?).
   virtual NABoolean isEmpty() const = 0;

   // verifying a requirement is met by a property.
   virtual NABoolean satisfied(const PushDownProperty&) const = 0;

   // dynamic type checking on PushDownColocationRequirement.
   virtual const PushDownColocationRequirement* 
      castToPushDownColocationRequirement() const { return NULL; };

   // dynamic type checking on PushDownCSRequirement.
   virtual const PushDownCSRequirement* 
      castToPushDownCSRequirement() const { return NULL; };

   virtual COMPARE_RESULT compare(const PushDownRequirement &other) const = 0;

private:
};

//
// Colocation requirement class hierarchy.
//
class PushDownColocationRequirement : public PushDownRequirement
{
public:
   PushDownColocationRequirement(const NodeMap* map = NULL) : map_(map) {};
   ~PushDownColocationRequirement() {};

   // The data member access function
   const NodeMap* getNodeMap() const { return map_; };

   NABoolean isEmpty() const { return map_ == NULL; };

   NABoolean satisfied(const PushDownProperty&) const;

   const PushDownColocationRequirement* 
      castToPushDownColocationRequirement() const { return this; };

   // test whether an instance of PushDownRequirement class is
   // actually a PushDownColocationRequirement.
   static NABoolean isInstanceOf(const PushDownRequirement* x)
     { return (x AND x->castToPushDownColocationRequirement()); };

   COMPARE_RESULT compare(const PushDownRequirement &other) const;

private:
   const NodeMap* map_;
};

//
// Compound statement push down requirement class hierarchy.
//
class PushDownCSRequirement : public PushDownColocationRequirement
{
public:
   PushDownCSRequirement(const PartitioningFunction* func = NULL, 
		    const SearchKey* key = NULL) :
      PushDownColocationRequirement((func)? func->getNodeMap() : NULL),
      partFunc_(func), searchKey_(key) {};

   ~PushDownCSRequirement() {};

   // data member access functions
   const SearchKey* getSearchKey() const { return searchKey_; };
   const PartitioningFunction* getPartFunc() const { return partFunc_; };

   // partFunc will never be NULL for a concrete CS push-down 
   // requirement
   NABoolean isEmpty() const { return partFunc_ == NULL; };

   NABoolean satisfied(const PushDownProperty&) const;

   const PushDownCSRequirement* 
      castToPushDownCSRequirement() const { return this; };

   // test whether an instance of PushDownRequirement class is
   // actually a PushDownCSRequirement.
   static NABoolean isInstanceOf(const PushDownRequirement* x)
     { return (x AND x->castToPushDownCSRequirement()); };

   COMPARE_RESULT compare(const PushDownRequirement &other) const;

private:
   const PartitioningFunction* partFunc_;
   const SearchKey* searchKey_;
};



// -----------------------------------------------------------------------
// Input physical properties
//
// Used by an operator to specify to its child any physical information
// concerning the parent, ancestors, or siblings that could be helpful
// in costing the current operator. It is currently only used by       
// nested join to pass physical information about the left child to the
// right child. With a few exceptions, if assumeSortedForCosting_ is set
// then all other fields will be NULL and if any of the nj... fields are
// set assumeSortedForCosting_ is FALSE.
// -----------------------------------------------------------------------

class InputPhysicalProperty : public NABasicObject
{
  public:
    
    // default and standard constructor
    InputPhysicalProperty(
       const ValueIdSet & outerCharOutputs,
       const ValueIdList* const outerOrder = NULL,
       const PartitioningFunction* const outerOrderPartFunc = NULL,
       const PartitioningFunction* const dp2SortOrderPartFunc =NULL,
       const NABoolean assumeSortedForCosting = FALSE,
	   NABoolean explodedOcbJoinForCosting = FALSE,
       const CardConstraint * outerCardConstraint = NULL)
      : njOuterCharOutputs_(outerCharOutputs),
        njOuterOrder_(outerOrder),
        njOuterOrderPartFunc_(outerOrderPartFunc),
        njDp2OuterOrderPartFunc_(dp2SortOrderPartFunc),
	assumeSortedForCosting_(assumeSortedForCosting),
        outerCardConstraint_(outerCardConstraint),
         njOuterPartFuncForNonUpdates_(outerOrderPartFunc),
		explodedOcbJoinForCosting_(explodedOcbJoinForCosting)
    {}
	// Constructor especially for Exploded Nested Join
	InputPhysicalProperty(
        const NABoolean assumeSortedProbesForCosting,
        NABoolean explodedOcbJoinForCosting = FALSE)
        :  assumeSortedForCosting_(assumeSortedProbesForCosting),
           explodedOcbJoinForCosting_(explodedOcbJoinForCosting),
           njOuterCharOutputs_(NULL),
           njOuterOrder_(NULL),
           njOuterOrderPartFunc_(NULL),
           njOuterPartFuncForNonUpdates_(NULL),
           njDp2OuterOrderPartFunc_(NULL),
           outerCardConstraint_(NULL)
    {}
    InputPhysicalProperty(
        const NABoolean assumeSortedProbesForCosting,
        const PartitioningFunction* const njOuterPartFunc,
		NABoolean explodedOcbJoinForCosting,
        const CardConstraint * outerCardConstraint = NULL)
        :  assumeSortedForCosting_(assumeSortedProbesForCosting),
           njOuterPartFuncForNonUpdates_(njOuterPartFunc),
		   explodedOcbJoinForCosting_(explodedOcbJoinForCosting),
           njOuterCharOutputs_(NULL),
           njOuterOrder_(NULL),
           njOuterOrderPartFunc_(NULL),
           njDp2OuterOrderPartFunc_(NULL),
           outerCardConstraint_(outerCardConstraint)
    {}


    // copy constructor
    InputPhysicalProperty(const InputPhysicalProperty& other)
      : njOuterCharOutputs_(other.njOuterCharOutputs_),
        njOuterOrder_(other.njOuterOrder_),
        njOuterOrderPartFunc_(other.njOuterOrderPartFunc_),
        njDp2OuterOrderPartFunc_(other.njDp2OuterOrderPartFunc_),
        outerCardConstraint_(other.outerCardConstraint_),
	assumeSortedForCosting_(other.assumeSortedForCosting_),
        njOuterPartFuncForNonUpdates_(other.njOuterPartFuncForNonUpdates_),
		explodedOcbJoinForCosting_(other.explodedOcbJoinForCosting_)
    {}

    // destructor
    virtual ~InputPhysicalProperty();

    // Accessors
    const ValueIdSet& getNjOuterCharOutputs() const
      { return njOuterCharOutputs_; }

    const ValueIdList* getNjOuterOrder() const
      { return njOuterOrder_; }

    const PartitioningFunction* getNjOuterOrderPartFunc() const
      { return njOuterOrderPartFunc_; }

    const PartitioningFunction* getNjDp2OuterOrderPartFunc() const
      { return njDp2OuterOrderPartFunc_; }

    const CardConstraint * getOuterCardConstraint() const
      { return outerCardConstraint_; }

    NABoolean getAssumeSortedForCosting() const
      { return assumeSortedForCosting_; }


    // Comparison method for use when comparing contexts
    COMPARE_RESULT compareInputPhysicalProperties(
                     const InputPhysicalProperty& other) const;

    const PartitioningFunction* getNjOuterOrderPartFuncForNonUpdates() const
      { return njOuterPartFuncForNonUpdates_;}
    NABoolean getExplodedOcbJoinForCosting() const
      { return explodedOcbJoinForCosting_; }

  private:

    // Used to pass the characteristic outputs from the left child
    // of a nested join to the right child. This is so the right
    // child can determine what its equijoin columns are.
    const ValueIdSet  njOuterCharOutputs_;

    // Used to pass the synthesized sort key of the left child of
    // a nested join to the right child for costing purposes.
    const ValueIdList * njOuterOrder_;

    // Used to pass the synthesized logical partitioning function 
    // of the left child of a nested join to the right child for
    // costing purposes.
    const PartitioningFunction* njOuterOrderPartFunc_;


    // Used to pass the synthesized physical partitioning function
    // of the left child access path to the right child, if the
    // sort order type of the left child's sort key was "dp2", 
    // for costing purposes.
    const PartitioningFunction* njDp2OuterOrderPartFunc_;

    // we want to find partition function even when order is not 
    // important. See case2 comment for assumeSortedForCosting_ below.
    const PartitioningFunction* njOuterPartFuncForNonUpdates_;

    // If we have a cardinality constraint from the outer child, we
    // can use this to sharpen bounds on estimated number of probes
    // in the inner child tree. In particular, if there is a small
    // upper bound, MDAM becomes a safe choice for inner child scans.
    const CardConstraint * outerCardConstraint_;

    // assumeSortedForCosting_ flag is set for two cases:
    // case 1: when input is rowset.
    // Used to pass information that the left child is an Unpack node
    // that is being used to flow input rowsets. We will assume that input rowsets 
    // are sorted with respect to whatever access path is being considered for the
    // right child. This assumption is made only for costing, and not to detrmine the
    // plan. This assumption will help us choose "optimistic" plans for rowset input,
    // assuming that rowset input is appropriately sorted, even though it may not be,
    // as we have no way to infer the sort order of the input rowset.
    // In this case njOuterPartFuncForNonUpdates_ will always NULL.

    // case 2: when left child partFunc njOuterPartFuncForNonUpdates_ is passed for 
    // NJ plan 0. This is only for cost estimation of exchange operator.
    // In this case njOuterPartFuncForNonUpdates_ will always be non-NULL.
    const NABoolean assumeSortedForCosting_ ;
    
	NABoolean explodedOcbJoinForCosting_ ;
}; // InputPhysicalProperty

#endif /* PHYPROP_HDR */



