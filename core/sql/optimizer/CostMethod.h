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
#ifndef COSTMETHOD_H
#define COSTMETHOD_H
/* -*-C++-*-
*************************************************************************
*
* File:         CostMethod.h
* Description:  Optimizer cost estimation interface object
* Created:      3/97
* Language:     C++
*
*
*
*
*************************************************************************
*/

// -----------------------------------------------------------------------
#include "BaseTypes.h"
#include "GroupAttr.h"
#include "Cost.h"

//<pb>
// ----------------------------------------------------------------------
// contents of this file
// ----------------------------------------------------------------------

class CostMethod;
class CostMethodCompoundStmt;
class CostMethodExchange;
class CostMethodFileScan;
class CostMethodDP2Scan;
class CostMethodFixedCostPerRow;
class CostMethodSort;
class CostMethodGroupByAggs;
class CostMethodSortGroupBy;
class CostMethodHashGroupBy;
class CostMethodShortCutGroupBy;
class CostMethodJoin;
class CostMethodHashJoin;
class CostMethodMergeJoin;
class CostMethodNestedJoin;
class CostMethodNestedJoinFlow;
class CostMethodMergeUnion;
class CostMethodTuple;
class CostMethodTranspose;
class CostMethodStoredProc;
class CostMethodHbaseInsert;
class CostMethodHbaseUpdateOrDelete;
class CostMethodHbaseUpdate;
class CostMethodHbaseDelete;
class CostMethodUnPackRows;
class CostMethodTableMappingUDF;
class CostMethodFastExtract;

// ----------------------------------------------------------------------
// forward declarations
// ----------------------------------------------------------------------
class Context;
class PlanWorkSpace;
class RelExpr;
class GroupByAgg;
class SortGroupBy;
class HashGroupBy;
class Join;
class HashJoin;
class MergeJoin;
class NestedJoin;
class MergeUnion;
class IndexDescHistograms;
class IndexDesc;
class TableMappingUDF;
class FastExtract;

// ----------------------------------------------------------------------
// External objects.
// ----------------------------------------------------------------------
extern const CostScalar csZero;

//<pb>
/**********************************************************************/
/*                                                                    */
/*                             CostMethod                             */
/*                                                                    */
/**********************************************************************/
/* A base class that provides an interface to virtual functions used  */
/* by the optimizer for cost estimation as well as common cache space */
/* for some parameters used in the cost estimation process.           */
/**********************************************************************/
class CostMethod
{
public:
  // ---------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------
  CostMethod( const char* className );

  // ---------------------------------------------------------------------
  // Generate a cost object with a zero cost. Makes use of metrics on no
  // of probes and stream information cached.
  // ---------------------------------------------------------------------
  Cost* generateZeroCostObject();

  // ---------------------------------------------------------------------
  // computeOperatorCost() provides the external interface for calculating
  // the operator cost for a node. It calls the virtual
  // computeOperatorCostInternal(), to perform the actual work.  The
  // purpose of this function is to provide a wrapper to help ensure that
  // all cleaning up of the CostMethod objects occurs before execution
  // leaves this function.  The cleaning up of CostMethod objects is done
  // by calling cleanUp() after computeOperatorCostInternal() returns,
  // by catching exceptions and calling cleanUp() before an exception is
  // rethrown, and by providing an interface called 
  // CostMethod::cleanUpAllCostMethods() that can be called after a 
  // longjmp(). A longjmp() may occur during out-of-memory conditions.
  // ---------------------------------------------------------------------
  Cost* computeOperatorCost( RelExpr* op,
                             const Context* myContext,
                             Lng32& countOfStreams
                           );

  // ---------------------------------------------------------------------
  // scmComputeOperatorCost() computes the cost of the operator
  // when SIMPLE_COST_MODEL CQD is set to ON.
  // ---------------------------------------------------------------------
  Cost* scmComputeOperatorCost( RelExpr* op,
                                const PlanWorkSpace* pws,
				Lng32& countOfStreams
				); 

  // ---------------------------------------------------------------------
  // computePartialPlanCost() combines the cost of a given RelExpr node 
  // with the cost of its known children.  Both the PartialPlan cost and 
  // the combined cost of the known children get stored in a specified
  // plan workspace.
  // ---------------------------------------------------------------------
  virtual void computePartialPlanCost( const RelExpr* op,
                                       PlanWorkSpace* pws,
                                       const Context* myContext
                                     );

  // return true iff we are under a nested join
  NABoolean isUnderNestedJoin(RelExpr* op, const Context* myContext);

  // ---------------------------------------------------------------------
  // computePlanCost() finalizes the cost of a plan rooted at a RelExpr
  // after an optimal plan has been generated for EACH of its children.
  // It recomputes a more accurate local processing cost for this node by
  // taking into account the actual physical properties of the optimal
  // plans of its children as given by the specified plan in the plan
  // work space. It then combines this cost with the children's costs
  // and returns the result.
  // ---------------------------------------------------------------------
  virtual Cost* computePlanCost( RelExpr* op,
                                 const Context* myContext,
                                 const PlanWorkSpace* pws,
                                 Lng32  planNumber
                                );

  // ---------------------------------------------------------------------
  // scmComputePlanCost() finalizes the cost of a plan rooted at a RelExpr
  // after an optimal plan has been generated for EACH of its children.
  // It then combines this cost with the children's costs
  // and returns the result.
  // ---------------------------------------------------------------------
  virtual Cost* scmComputePlanCost( RelExpr* op,
				    const PlanWorkSpace* pws,
				    Lng32  planNumber
				    );

  // ---------------------------------------------------------------------
  // Roll up members.
  // ---------------------------------------------------------------------

  virtual void getChildCostForUnaryOp( RelExpr* op
                                     , const Context* myContext
                                     , const PlanWorkSpace* pws
                                     , Lng32  planNumber
                                     , CostPtr& childCost
                                     );

  virtual void getChildCostsForBinaryOp( RelExpr* op
                                       , const Context* myContext
                                       , const PlanWorkSpace* pws
                                       , Lng32  planNumber
                                       , CostPtr& leftChildCost
                                       , CostPtr& rightChildCost
                                       );

  // ---------------------------------------------------------------------
  // print
  // ---------------------------------------------------------------------
  virtual void print( FILE* ofd = stdout
                    , const char* indent = DEFAULT_INDENT
                    , const char* title = NULL
                    ) const;

  virtual void display() const;

  // ---------------------------------------------------------------------
  // This method is responsible for cleaning up if a longjmp occurs in
  // MXCMP.  This occurs when a memory allocation failure occurs and
  // may occur while computing operator cost.  This function calls
  // cleanUp() on all CostMethod objects.
  // ---------------------------------------------------------------------
  static void cleanUpAllCostMethods();

//<pb>
protected:

  const char* className_;

  // ---------------------------------------------------------------------
  // computeOperatorCostInternal() sets the node's cost to a preliminary,
  // low(!) estimate for an algorithm's local processing costs
  // ---------------------------------------------------------------------
  virtual Cost* computeOperatorCostInternal( RelExpr* op,
                                             const Context* myContext,
                                             Lng32& countOfStreams
                                           ) = 0;

  virtual Cost* scmComputeOperatorCostInternal( RelExpr* op,
                                                const PlanWorkSpace* pws,
                                                Lng32& countOfStreams
                                           ) = 0;

  //  Wrapper for SCM Cost constructor, used by SCM only.
  Cost * scmCost( CostScalar tuplesProcessed,
		  CostScalar tuplesProduced,
		  CostScalar tuplesSent,
		  CostScalar ioRand,
		  CostScalar ioSeq,
		  CostScalar noOfProbes,
		  CostScalar input1RowSize,
		  CostScalar input2RowSize,
		  CostScalar outputRowSize,
		  CostScalar probeRowSize );

  enum ncmRowSizeFactorType {TUPLES_ROWSIZE_FACTOR = 0,
			     SEQ_IO_ROWSIZE_FACTOR,
			     RAND_IO_ROWSIZE_FACTOR};

  CostScalar scmRowSizeFactor( CostScalar rowSize, ncmRowSizeFactorType rowSizeFactorType = TUPLES_ROWSIZE_FACTOR);

  virtual Cost* rollUp( Cost* const parentCost
                      , Cost* const childCost
                      , const ReqdPhysicalProperty* const rpp
                      );
  
  virtual Cost* scmRollUp( Cost* const parentCost
                         , Cost* const leftChildCost
                         , Cost* const rightChildCost
                         , const ReqdPhysicalProperty* const rpp
                      );

  virtual Cost* rollUpForBinaryOp( RelExpr* op
                                 , const Context* myContext
                                 , const PlanWorkSpace* pws
                                 , Lng32  planNumber
                                 );

  virtual Cost* mergeNoLegsBlocking( const CostPtr leftChildCost,
                                     const CostPtr rightChildCost,
                                     const ReqdPhysicalProperty* const rpp);

  virtual Cost* convertToBlocking( const CostPtr nonBlockingCost );

  virtual Cost* mergeBothLegsBlocking( const CostPtr leftChildCost,
                                       const CostPtr rightChildCost,
                                       const ReqdPhysicalProperty* const rpp);

  inline void setClassName (const char* className) {className_ = className;}

  // ---------------------------------------------------------------------
  // The method caches the parameters typically needed for costing as
  // protected members of this class. The idea is to prevent the same
  // piece of code of retrieving these parameters to be duplicated in
  // every costing methods which need them.
  // ---------------------------------------------------------------------
  virtual void cacheParameters(RelExpr* op, const Context* myContext);

  // ---------------------------------------------------------------------
  // This method estimates the degree of parallelism the operator is
  // exhibiting by setting parameters of the class which is related to
  // parallelism. Derived classes should refine generic implementation if
  // it's not sufficient. This method assumes cacheParameters() has been
  // already called.
  // ---------------------------------------------------------------------
  virtual void estimateDegreeOfParallelism();

  // ---------------------------------------------------------------------
  // This method is responsible for determining the number of cpus and
  // the number of fragments per cpu.
  // ---------------------------------------------------------------------
   void determineCpuCountAndFragmentsPerCpu( 
    Lng32 & cpuCount, 
    Lng32 & fragmentsPerCput 
    );

  // ---------------------------------------------------------------------
  // This method is responsible for cleaning up after cost computation.
  // For example, some costing method might allocate temporary objects
  // dynamically and store them in the private space of the cost method
  // object. This defines an interface for cleaning up those objects
  // allocated after a costing session.
  // ---------------------------------------------------------------------
  virtual void cleanUp();

  // ---------------------------------------------------------------------
  // These are the parameters cached by cacheParameters().
  // ---------------------------------------------------------------------

  // The operator itself.
  RelExpr* op_;

  // The current optimization context.
  const Context* context_;

  // The rpp as in context_.
  const ReqdPhysicalProperty* rpp_;

  // Partitioning requirement or function. This could come from rpp_
  // when we're going down the tree (partReq_); or from the actual
  // physical properties when we're going up (partFunc_).
  const PartitioningRequirement* partReq_ ;
  const PartitioningFunction* partFunc_ ;

  // The no of CPUs available on the system.
  Lng32 countOfAvailableCPUs_;

  // Maximum count of pipelines allowed per CPU.
  Lng32 countOfPipelinesPerCPU_;

  // The attributes of the group the operator belongs to.
  GroupAttributes* ga_;

  // A reference to value ids this operator produces as in ga_.
  const ValueIdSet & myVis()  { return ga_->getCharacteristicOutputs(); }

  // Whether the operator has outer col references.
  NABoolean hasOuterReferences_;

  // Whether the operator sits on the right leg of a NestedJoin.
  NABoolean isUnderNestedJoin_;

  // The input estimated logical properties as in context_.
  EstLogPropSharedPtr inLogProp_;

  // The "no of probes" as in inLogProp_.
  CostScalar noOfProbes_;

  // The output estimated logical properties of this operator.
  EstLogPropSharedPtr myLogProp_;

  // The count of statements that have been compiled since the last
  // time inLogProp_ or myLogProp_ were set.  This is used to indicate
  // whether those sharePtr objects must be reset.
  ULng32 lastStatementCount_;
 
  // The output row count of the total result set of this operator.
  CostScalar myRowCount_;

  // Is the operator being costed considered a big memory operator?
  NABoolean isBMO_;

  // Memory limit in kbytes on a per stream basis. No limit if zero.
  double memoryLimit_;

  // ---------------------------------------------------------------------
  // These are parameters produced by estimateDegreeOfParallelism().
  // ---------------------------------------------------------------------

  // The degree of parallelism of this operator is assumed to exhibit.
  Lng32 countOfStreams_;

  // The average no of probes per parallel instance of this operator.
  CostScalar noOfProbesPerStream_;

  // ---------------------------------------------------------------------
  // Other class members.
  // ---------------------------------------------------------------------

  // Is the memory limit above exceeded somewhere during cost computation?
  NABoolean isMemoryLimitExceeded_;

  // enum for plan numbers
  enum planNum_ {PLAN0 = 0, PLAN1, PLAN2, PLAN3, PLAN4, PLAN5};

  // nextCostMethod and head are used to build a linked-list of all the
  // CostMethod classes that are declared statically in the optimizer
  // directory.  The linked-list is used to call cleanUp on those objects
  // if a longjmp occurs.
  CostMethod *nextCostMethod_;
  //static THREAD_P CostMethod* head_;
}; // class CostMethod

/**********************************************************************/
/*                                                                    */
/*              CostMethodCompoundStmt                                */
/*                                                                    */
/*                                                                    */
/**********************************************************************/

class CostMethodCompoundStmt : public CostMethod
{
public:
  // Constructor
  CostMethodCompoundStmt() : CostMethod( "CostMethodCompoundStmt" ) {}

  virtual ~CostMethodCompoundStmt(){};

protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& );
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

  virtual void cacheParameters(RelExpr* op, const Context* myContext);

  CostScalar cpuCostToProduceAllRows_;

}; // class CostMethodCompoundStmt

//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodExchange                         */
/*                                                                    */
/**********************************************************************/
class CostMethodExchange : public CostMethod
{
public:
  // Constructor
  CostMethodExchange() : CostMethod( "CostMethodExchange" ),
  numOfContinueDownMessages_(0) {}

private:
  NABoolean isMergeNeeded_;
  CostScalar upRowsPerConsumer_;
  CostScalar numOfContinueDownMessages_;
  PhysicalProperty *sppForMe_;
  PhysicalProperty *sppForChild_;
  CostScalar numOfProbes_;
  CostScalar downMessageLength_;
  CostScalar upMessageBufferLength_;
  CostScalar downMessageBufferLength_;
  NABoolean isOpBelowRoot_;

protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32&    countOfStreams);
  // SCM cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32&    countOfStreams);

  // SCM method
  CostScalar scmComputeUpTuplesSent( const Context*  myContext,
                                     Exchange*  exch,
                                     const PartitioningFunction* parentPartFunc,
                                     const PartitioningFunction* childPartFunc,
                                     const CostScalar& numOfConsumers,
                                     const CostScalar& numOfProducers) const;

  CostScalar computeESPCost( const NABoolean executeInESP,
                             const CostScalar& noOfProbes) const;

  Cost* computeExchangeCostGoingDown( const ReqdPhysicalProperty* rpp,
                                      const CostScalar& noOfProbes,
                                      Lng32& countOfStreams);

  void getDefaultValues( const NABoolean   executeInDP2,
                         const Exchange*   exch,
                               CostScalar& messageSpacePerRecordInKb,
                               CostScalar& messageHeaderInKb,
                               CostScalar& messageBufferSizeInKb) const;

  CostScalar computeDownMessages( const CostScalar& numOfProbes,
                                  const NABoolean  executeInDP2,
                                  const CostScalar& messageHeaderInKb,
                                  const CostScalar& messageBufferSizeInKb,
                                  const CostScalar& numOfPartitions,
                                  const CostScalar& numOfConsumers,
                                   const Context* myContext,
                                   CostScalar &msgLength
                                   ) const;

  CostScalar computeDownDataAndControlMessages(
                                         const CostScalar& numOfProbes,
                                         const NABoolean executeInDP2,
                                         const CostScalar& messageHeaderInKb,
                                        const CostScalar& messageBufferSizeInKb,
                                         const CostScalar& numOfPartitions,
                                         const CostScalar& numOfConsumers,
                                         const Context* myContext,
                                         CostScalar &downMessageLength) const;

  CostScalar computeUpMessages( 
                          const Context*              myContext,
				Exchange*             exch,
                          const PartitioningFunction* parentPartFunc,
                          const PartitioningFunction* childPartFunc,
                          const PhysicalProperty*     sppForChild,
                          const CostScalar&           messageSpacePerRecordInKb,
                          const CostScalar&           messageHeaderInKb,
                          const CostScalar&           messageBufferSizeInKb,
                          const CostScalar&           numOfConsumers,
                          const NABoolean             executeInDP2,
                                CostScalar&           upRowsPerConsumer,
                                CostScalar& numOfContinueMessages) const;
                                 
  void categorizeMessages( const PartitioningFunction* parentPartFunc,
                           const PartitioningFunction* childPartFunc,
                           const NABoolean             executeInDP2,
                           const CostScalar&           downMessages,
                           const CostScalar&           upMessages,
                                 CostScalar&           downIntraNodeMessages,
                                 CostScalar&           downInterNodeMessages,
                                 CostScalar&           downRemoteNodeMessages,
                                 CostScalar&           upIntraNodeMessages,
                                 CostScalar&           upInterNodeMessages,
                                 CostScalar&           upRemoteNodeMessages
                                 )const;
  void produceCostVectors(
                          const CostScalar&           numOfProbes,
                          const CostScalar&           numOfConsumers,
                          const CostScalar&           numOfProducers,
                          const NABoolean             executeInDP2,
			  const PartitioningFunction* myPartFunc,
                          const PartitioningFunction* childPartFunc,
                          const CostScalar&            messageSpacePerRecordInKb,
                          const CostScalar&            messageHeaderInKb,
                          const CostScalar&            messageBufferSizeInKb,
                          const CostScalar&            upRowsPerConsumer,
                          const CostScalar&            downIntraNodeMessages,
                          const CostScalar&            downInterNodeMessages,
                          const CostScalar&            downRemoteNodeMessages,
                          const CostScalar&            upIntraNodeMessages,
                          const CostScalar&            upInterNodeMessages,
                          const CostScalar&            upRemoteNodeMessages,
                                CostVecPtr&           parentFR,
                                CostVecPtr&           parentLR,
                                CostVecPtr&           childFR,
                                CostVecPtr&           childLR) const;

  void
  produceCostVectorsWithControlDataMessages(
                         const CostScalar &          numOfProbes,
                         const CostScalar &          numOfConsumers,
                         const CostScalar &          numOfProducers,
                         const NABoolean             childExecutesInDP2,
                         const PartitioningFunction* myPartFunc,
                         const PartitioningFunction* childPartFunc,
                        const CostScalar &           messageSpacePerRecordInKb,
                        const CostScalar &           messageHeaderInKb,
                          const CostScalar &           messageBufferSizeInKb,
                          const CostScalar &           upRowsPerConsumer,
                          const CostScalar &           downIntraCpuMessages,
                          const CostScalar &           downIntraSegmentMessages,
                         const CostScalar &           downRemoteSegmentMessages,
                          const CostScalar &           upIntraCPUMessages,
                          const CostScalar &           upIntraSegmentMessages,
                          const CostScalar &           upRemoteSegmentMessages,
                                CostVecPtr&           parentFR,
                                CostVecPtr&           parentLR,
                                CostVecPtr&           childFR,
                                CostVecPtr&           childLR) const;

  Cost* computeExchangeCost( const CostVecPtr  parentFR,
                             const CostVecPtr  parentLR,
                             const CostVecPtr  childFR,
                             const CostVecPtr  childLR,
                             const CostScalar& numOfConsumers,
                             const CostScalar& numOfProducers) const;

  void categorizeMessagesForDP2(
     const PartitioningFunction* parentPartFunc,
     const PartitioningFunction* childPartFunc,
     const CostScalar &downMessages,
     const CostScalar & upMessages,
     CostScalar & downIntraNodeMessages,
     CostScalar & downInterNodeMessages,
     CostScalar & downRemoteNodeMessages,
     CostScalar & upIntraNodeMessages,
     CostScalar & upInterNodeMessages,
     CostScalar & upRemoteNodeMessages) const;

  void categorizeMessagesForESP(
     const PartitioningFunction* parentPartFunc,
     const PartitioningFunction* childPartFunc,
     const CostScalar &downMessages,
     const CostScalar & upMessages,
     CostScalar & downIntraNodeMessages,
     CostScalar & downInterNodeMessages,
     CostScalar & downRemoteNodeMessages,
     CostScalar & upIntraNodeMessages,
     CostScalar & upInterNodeMessages,
     CostScalar & upRemoteNodeMessages) const;

  NABoolean  isGroupedRepartitioning(
              const PartitioningFunction* parentPartFunc,
              const PartitioningFunction* childPartFunc,
              NABoolean &parentGroupsChild,
              CostScalar &partsPerGroup)  const;
 
  CostScalar computeLocalMessageWeight(const NodeMap*, const NodeMap*) const;
  CostScalar getNumberofSegments(const NodeMap*) const;

}; // class CostMethodExchange

//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodFileScan                         */
/*                                                                    */
/**********************************************************************/
class CostMethodFileScan : public CostMethod
{
public:
  // Constructor
  CostMethodFileScan( const char * className = "CostMethodFileScan" ) 
    : CostMethod( className ) {}

protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal( RelExpr* op,
                                                const PlanWorkSpace* pws,
						Lng32& countOfStreams
						);
}; // class CostMethodFileScan

//<pb>
/**********************************************************************/
/*                                                                    */
/*                          CostMethodDP2Scan                         */
/*                                                                    */
/**********************************************************************/
class CostMethodDP2Scan : public CostMethodFileScan
{
public:
  // Constructor
  CostMethodDP2Scan() : CostMethodFileScan( "CostMethodDP2Scan" )  {}

protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal( RelExpr* op,
                                                const PlanWorkSpace* pws,
						Lng32& countOfStreams
						);
}; // class CostMethodDP2Scan

//<pb>

// ----QUICKSEARCH FOR FIXEDROW...........................................

/**********************************************************************/
/*                                                                    */
/*                     CostMethodFixedCostPerRow                      */
/*                                                                    */
/**********************************************************************/
class CostMethodFixedCostPerRow : public CostMethod
{
public:
  // Constructor
  CostMethodFixedCostPerRow(double baseCpuCost,
                            double cpuCostPerChildRow,
                            double cpuCostPerOutputRow)
    : CostMethod("CostMethodFixedCostPerRow"),
      baseCpuCost_(baseCpuCost),
      cpuCostPerChildRow_(cpuCostPerChildRow),
      cpuCostPerOutputRow_(cpuCostPerOutputRow)
    {}

  virtual void print(FILE* ofd = stdout,
                     const char* indent = DEFAULT_INDENT,
                     const char* title = NULL) const;

protected:
  // Redefine inherited virtual functions...
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

  // ---------------------------------------------------------------------
  // For SCM.
  // ---------------------------------------------------------------------
  virtual Cost* scmComputeOperatorCostInternal( RelExpr* op,
                                                const PlanWorkSpace* pws,
						Lng32& countOfStreams
						);
  const CostScalar baseCpuCost_;
  const CostScalar cpuCostPerChildRow_;
  const CostScalar cpuCostPerOutputRow_;

}; // class CostMethodFixedCostPerRow

//<pb>
// ----QUICKSEARCH FOR SORT...............................................

/**********************************************************************/
/*                                                                    */
/*                           CostMethodSort                           */
/*                                                                    */
/**********************************************************************/
class CostMethodSort : public CostMethod
{
public:
  // Constructor
  CostMethodSort()
    : CostMethod("CostMethodSort"),
      ioBufferSize_(CostPrimitives::getBasicCostFactor(SORT_IO_BUFFER_SIZE)),
      treeNodeSize_(CostPrimitives::getBasicCostFactor(SORT_TREE_NODE_SIZE)),
      exBufferSize_(CostPrimitives::getBasicCostFactor(SORT_EX_BUFFER_SIZE)),
      cpuCostPerProbeInit_(
	CostPrimitives::getBasicCostFactor(SORT_CPUCOST_INITIALIZE)),
      qsFludgeFactor_(CostPrimitives::getBasicCostFactor(SORT_QS_FACTOR)),
      rsFludgeFactor_(CostPrimitives::getBasicCostFactor(SORT_RS_FACTOR)),
      rwFudgeFactor_(CostPrimitives::getBasicCostFactor(SORT_RW_FACTOR)),
      cpuCostAllocateBuffer_(
	CostPrimitives::getBasicCostFactor(EX_OP_ALLOCATE_BUFFER)),
      cpuCostAllocateTuple_(
	CostPrimitives::getBasicCostFactor(EX_OP_ALLOCATE_TUPLE))
  {}  // CostMethodSort constructor.

  virtual void cacheParameters(RelExpr* op, const Context* myContext);

protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32&    countOfStreams);

  CostScalar scmComputeOverflowCost( CostScalar numInputTuples, CostScalar inputRowSize );

private:

  // ---------------------------------------------------------------------
  // Parameters obtained from constructor.
  // ---------------------------------------------------------------------

  // Sort I/O buffer size. Used in the Sort Utility.
  double ioBufferSize_;

  // Tree node size.
  double treeNodeSize_;

  // Sort executor buffer size. Used on the executor side.
  double exBufferSize_;

  // ---------------------------------------------------------------------
  // Per probe setup costs. This captures the cost to call the method
  // SortUtil::SortInitialize() to allocate the SortAlgo objects (such as
  // Tree, TreeNode, Record and ScratchSpace), etc.
  // ---------------------------------------------------------------------
  CostScalar cpuCostPerProbeInit_;

  // Fludge factor for QuickSort in Sort Util.
  CostScalar qsFludgeFactor_;

  // Fludge factor for Replacement Selection in Sort Util.
  CostScalar rsFludgeFactor_;

  // Fludge factor for Read Write operation in Sort Util.
  CostScalar rwFudgeFactor_;

  // Cost to allocate a buffer.
  CostScalar cpuCostAllocateBuffer_;

  // Cost to allocate a tuple.
  CostScalar cpuCostAllocateTuple_;

  // ---------------------------------------------------------------------
  // Parameters to cache for a Sort.
  // ---------------------------------------------------------------------

  // The sort operator itself.
  Sort* sort_;

  // Length of the sort keys in bytes.
  Lng32 sortKeyLength_;

  // Length of a sort record (including the key) in bytes.
  Lng32 sortRecLength_;

  // To send a row from the executor to the Sort Util.
  CostScalar cpuCostSendRow_;

  // To compare a pair of rows' encoded key.
  CostScalar cpuCostCompareKeys_;

  // To copy the result row to the result buffer.
  CostScalar cpuCostCopyResultRow_;

  // Max no of rows that can stay in memory at a time at run generation.
  CostScalar maxRunGenOrder_;

  // Max no of runs that can be merged in a merge pass.
  CostScalar maxMergeOrder_;


}; // class CostMethodSort

//<pb>
// ----QUICKSEARCH FOR GB.................................................

/**********************************************************************/
/*                                                                    */
/*                       CostMethodGroupByAgg                         */
/*                                                                    */
/**********************************************************************/
/* Base class for GroupBy operators which mainly provides cache space */
/* for parameters in the two children's estimated logical properties. */
/**********************************************************************/
class CostMethodGroupByAgg : public CostMethod
{
public:
  CostMethodGroupByAgg(const char* className) : CostMethod(className) {}

protected:

  // ---------------------------------------------------------------------
  // The method caches the parameters typically needed for costing as
  // protected members of this class. The idea is to prevent the same
  // piece of code of retrieving these parameters to be duplicated in
  // every costing methods which need them.
  // ---------------------------------------------------------------------
  virtual void cacheParameters(RelExpr* op, const Context* myContext);

  // ---------------------------------------------------------------------
  // Whether we could improve the estimate of the local cost previously
  // computed by computeOperatorCost() after PhyProp of the operator
  // become available. Right now, that is so only if the operator is req
  // to execute in DP2.
  // ---------------------------------------------------------------------
  virtual void estimateDegreeOfParallelism();

  // ---------------------------------------------------------------------
  // Clean up SharedPtr objects
  // ---------------------------------------------------------------------
  virtual void cleanUp();

  // ---------------------------------------------------------------------
  // Parameters cached by cacheParameters().
  // ---------------------------------------------------------------------

  // The group by operator being costed.
  GroupByAgg* gb_;

  // Estimated logical properties of the join's left child.
  EstLogPropSharedPtr child0LogProp_;

  // Row count of the left child as in child0LogProp_.
  CostScalar child0RowCount_;

  // My intermediate estimated logical properties.
  EstLogPropSharedPtr myIntLogProp_;

  // Group count as in myIntLogProp_.
  CostScalar groupCount_;

  // Storage in bytes needed for the group key.
  Lng32 groupKeyLength_;

  // Storage in bytes needed for the aggregates.
  Lng32 aggregateLength_;

  // Per probe init cpu cost, should be determined by derived classes.
  CostScalar cpuCostPerProbeInit_;

  // Cost to initialize a new group in the buffer.
  CostScalar cpuCostInitNewGroup_;

  // Cost to compare the group keys.
  CostScalar cpuCostCompareGroupKeys_;

  // Cost to aggregate a row to a group.
  CostScalar cpuCostAggrRowToGroup_;

  // Cost to evaluate the having predicate.
  CostScalar cpuCostEvalHavingPred_;

  // Cost to return a row to the parent.
  CostScalar cpuCostReturnRow_;

  // ---------------------------------------------------------------------
  // Parameters computed by estimateDegreeOfParallelism().
  // ---------------------------------------------------------------------

  // Estimated no of input rows to group per stream.
  CostScalar rowCountPerStream_;

  // Estimated no of group the input rows can be grouped into per stream.
  CostScalar groupCountPerStream_;

  // Estimated no of output rows per stream.
  CostScalar myRowCountPerStream_;

}; // class CostMethodGroupByAgg

//<pb>
// ----QUICKSEARCH FOR SGB................................................

/**********************************************************************/
/*                                                                    */
/*                       CostMethodSortGroupBy                        */
/*                                                                    */
/**********************************************************************/
class CostMethodSortGroupBy : public CostMethodGroupByAgg
{
public:
  // constructor
  CostMethodSortGroupBy()
    : CostMethodGroupByAgg("CostMethodSortGroupBy"),
      bufferCount_(
	CostPrimitives::getBasicCostFactor(SGB_INITIAL_BUFFER_COUNT)),
      bufferSize_(CostPrimitives::getBasicCostFactor(SGB_INITIAL_BUFFER_SIZE))
  {
    cpuCostPerProbeInit_ = 
      CostPrimitives::getBasicCostFactor(SGB_CPUCOST_INITIALIZE);
  }

protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32&    countOfStreams);
  const double bufferCount_;

  const double bufferSize_;

}; // class CostMethodSortGroupBy

//<pb>
// ----QUICKSEARCH FOR HGB................................................

/**********************************************************************/
/*                                                                    */
/*                       CostMethodHashGroupBy                        */
/*                                                                    */
/**********************************************************************/
class CostMethodHashGroupBy : public CostMethodGroupByAgg
{
public:
  // constructor
  CostMethodHashGroupBy()
    : CostMethodGroupByAgg("CostMethodHashGroupBy"),
      bufferSize_((CostPrimitives::getBasicCostFactor(GEN_HGBY_BUFFER_SIZE) - 24)/1024),
      hashedRowOverhead_(
	(Lng32)CostPrimitives::getBasicCostFactor(HH_OP_HASHED_ROW_OVERHEAD)),
      cpuCostAllocateHashTable_(
	CostPrimitives::getBasicCostFactor(HH_OP_ALLOCATE_HASH_TABLE)),
      cpuCostInsertRowToChain_(
	CostPrimitives::getBasicCostFactor(HH_OP_INSERT_ROW_TO_CHAIN)),
      cpuCostPositionHashTableCursor_(
	CostPrimitives::getBasicCostFactor(HH_OP_PROBE_HASH_TABLE)),
      memoryLimitInDP2_(
	CostPrimitives::getBasicCostFactor(HGB_DP2_MEMORY_LIMIT)),
      groupingFactorForSpilledClusters_(
	CostPrimitives::getBasicCostFactor(HGB_GROUPING_FACTOR_FOR_SPILLED_CLUSTERS))
  {
    cpuCostPerProbeInit_ = 
      CostPrimitives::getBasicCostFactor(HGB_CPUCOST_INITIALIZE);
  }

  virtual Cost* computePlanCost(       RelExpr*       hashGroupByOp,
                                 const Context*       myContext,
                                 const PlanWorkSpace* pws,
                                       Lng32           planNumber);

protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

  CostScalar scmComputeOverflowCost( CostScalar numInputTuples, CostScalar inputRowSize, CostScalar numOutputTuples, CostScalar outputRowSize );

  virtual void cacheParameters(RelExpr* op, const Context* myContext);

  void deriveParameters();

  CostScalar calculateCostToInsertIntoChain
                                  (CostScalar &);

  // ---------------------------------------------------------------------
  // Some helper methods.
  // ---------------------------------------------------------------------

  Lng32 computeCountOfClusters( const CostScalar& memoryLimit,
                               const CostScalar& tableSize );

  void computePartialGroupByLeafCost(CostScalar& cpuFR,
                              CostScalar& cpuLR,
                              CostScalar& cpuBK,
                              CostScalar& groupingFactor);

  NABoolean computePassCost(NABoolean         isFirstPass,
                            SimpleCostVector& cvPassCurr,
                            NABoolean&        isRowProduced);

  // ---------------------------------------------------------------------
  // Constructor cached constant parameters.
  // ---------------------------------------------------------------------

  const double bufferSize_;

  const Lng32 hashedRowOverhead_;

  const CostScalar cpuCostAllocateHashTable_;

  const CostScalar cpuCostInsertRowToChain_;

  const CostScalar cpuCostPositionHashTableCursor_;

  const CostScalar memoryLimitInDP2_;

  const CostScalar groupingFactorForSpilledClusters_;

  // ---------------------------------------------------------------------
  // Parameters cached by cacheParameters().
  // ---------------------------------------------------------------------

  // Cost to compute the hash value for a row.
  CostScalar cpuCostHashRow_;

  // Cost to combine the aggregates of two partial groups.
  CostScalar cpuCostAggrGroupToGroup_;

  // Length of a buffered group entry with the hash key and chain pointer.
  Lng32 extGroupLength_;

  // ---------------------------------------------------------------------
  // Below is the working set of parameters used by computePassCost().
  // They are to be initialized externally before the first call is made
  // to computePassCost() to get the cost for the first pass. They are
  // then manipulated directly by computePassCost() to propagate state
  // information from the first call to the second call, and so on.
  // ---------------------------------------------------------------------

  // No of clusters the executor decides to allocate for hash grouping.
  Lng32 noOfClustersToBeAllocated_;

  // ---------------------------------------------------------------------
  // These three parameters together describe the inputs to be processed
  // in a pass. noOfClustersToBeProcessed_ is just the no of entries that
  // pass is going to process with each entry having a number of rows and
  // groups given by rowCountPerCluster_ and groupCountPerCluster_.
  // ---------------------------------------------------------------------
  Lng32 noOfClustersToBeProcessed_;
  CostScalar rowCountPerCluster_;
  CostScalar groupCountPerCluster_;

  // Peak memory that the operator is consuming.
  CostScalar mem_;

}; // class CostMethodHashGroupBy

//<pb>
// ----QUICKSEARCH FOR SHORTCUTGROUPBY.....................................

/**********************************************************************/
/*                                                                    */
/*                       CostMethodShortCutGroupBy                     */
/*                                                                    */
/**********************************************************************/
class CostMethodShortCutGroupBy : public CostMethodGroupByAgg
{
public:
  // Constructor
  CostMethodShortCutGroupBy()
    : CostMethodGroupByAgg("CostMethodShortCutGroupBy"),
      cpuCostPassRow_(CostPrimitives::getBasicCostFactor(EX_OP_COPY_ATP))
  {}

  // Cost functions
  virtual Cost * computePlanCost( RelExpr* op,
                                  const Context* myContext,
                                  const PlanWorkSpace* pws,
                                  Lng32 planNumber
                                );

protected:
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal( RelExpr* op,
                                                const PlanWorkSpace* pws,
						Lng32& countOfStreams
						);
private:
  // Cost to forward a row from child to parent.
  const CostScalar cpuCostPassRow_;

}; // class CostMethodShortCutGroupBy

//<pb>
// ----QUICKSEARCH FOR JOIN...............................................

/**********************************************************************/
/*                                                                    */
/*                           CostMethodJoin                           */
/*                                                                    */
/**********************************************************************/
/* A base class for Join operators which mainly provides cache space  */
/* for parameters in the two children's estimated logical properties. */
/**********************************************************************/
class CostMethodJoin : public CostMethod
{

public:
  // -----------------------------------------------------------------------
  // CostMethodJoin constructor.
  // -----------------------------------------------------------------------
  CostMethodJoin( const char* className = "CostMethodJoin" )
    : CostMethod(className),
      isColStatsMeaningful_(FALSE),
      child0EquiJoinColStats_(NULL),
      child1EquiJoinColStats_(NULL),
      mergedEquiJoinColStats_(NULL),
      maxDegreeOfParallelism_(csZero),
      hasEquiJoinPred_(FALSE)
  {}

protected:

  // ---------------------------------------------------------------------
  // The method caches the parameters typically needed for costing as
  // protected members of this class. The idea is to prevent the same
  // piece of code of retrieving these parameters to be duplicated in
  // every costing methods which need them.
  // ---------------------------------------------------------------------
  virtual void cacheParameters(RelExpr* op, const Context* myContext);

  // ---------------------------------------------------------------------
  // Makes use of the generic implementation. Adds logic for dealing with
  // the children's per stream row counts.
  // ---------------------------------------------------------------------
  virtual void estimateDegreeOfParallelism();

  // ---------------------------------------------------------------------
  // A more serious effect to estimate the size of a representative
  // stream. Helper method of estimateDegreeOfParallelism()
  // ---------------------------------------------------------------------
  NABoolean computeRepresentativeStream();

  // ---------------------------------------------------------------------
  // Retrieve and merge the histogram on the equi join columns.
  // ---------------------------------------------------------------------
  NABoolean mergeHistogramsOnEquiJoinPred();

  // ---------------------------------------------------------------------
  // Find predicates really evaluated at the MJ/HJ, and classify them.
  // ---------------------------------------------------------------------
  void classifyPredicates(ValueIdSet& innerEquiJoinKeys,
                          ValueIdSet& outerEquiJoinKeys,
                          ValueIdSet& otherJoinPreds,
                          ValueIdSet& otherSelPreds);

  // ---------------------------------------------------------------------
  // This method prepares column statistics information on the equi-join
  // cols of the children and estimate the row count and uec in the result
  // of doing an inner-join on those cols.
  // ---------------------------------------------------------------------
  void estimateEquiJoinStats();

  // ---------------------------------------------------------------------
  // Clean up cached ColStats and SharedPtr objects
  // ---------------------------------------------------------------------
  virtual void cleanUp();

  // ---------------------------------------------------------------------
  // Cached parameters follow.
  // ---------------------------------------------------------------------

  // The operator being costed.
  Join* jn_;

  // Estimated logical properties of the join's left child.
  EstLogPropSharedPtr child0LogProp_;

  // Row count of the left child as in child0LogProp_.
  CostScalar child0RowCount_;

  // Estimated logical properties of the join's right child.
  EstLogPropSharedPtr child1LogProp_;

  // Row count of the right child as in child1LogProp_.
  CostScalar child1RowCount_;

  // This join has equi-join predicates? Applicable to HJ and MJ only.
  NABoolean hasEquiJoinPred_;

  // Whether the ColStats following has been computed.
  NABoolean isColStatsMeaningful_;

  // ---------------------------------------------------------------------
  // Parameters set by estimateEquiJoinStats().
  // ---------------------------------------------------------------------

  // uec of the equi-join columns of left child.
  CostScalar child0EquiJoinColUec_;

  // uec of the equi-join columns of right child.
  CostScalar child1EquiJoinColUec_;

  // uec of the equi-join columns in the result of the inner equi-join.
  CostScalar resultEquiJoinColUec_;

  // Row count in the result of the inner equi-join.
  CostScalar resultEquiJoinRowCount_;

  // ---------------------------------------------------------------------
  // The following parameters are set only if isColStatsMeaningful_.
  // ---------------------------------------------------------------------

  // ColStat on equi-join column(s) of left child.
  ColStatsSharedPtr child0EquiJoinColStats_;

  // ColStat on equi-join column(s) of right child.
  ColStatsSharedPtr child1EquiJoinColStats_;

  // ColStat on equi-join column(s) after an inner-join merge.
  ColStatsSharedPtr mergedEquiJoinColStats_;
  ColStatDescSharedPtr mergedEquiJoinColStatDesc_;

  // Max degree of parallelism as estimated from the uec's of children.
  CostScalar maxDegreeOfParallelism_;

  // ---------------------------------------------------------------------
  // These are parameters produced by estimateDegreeOfParallelism().
  // Note that the uec's are only meaningful if isColStatsMeaningful_.
  // ---------------------------------------------------------------------

  // Per stream row count of left child. UEC is on join keys.
  CostScalar child0RowCountPerStream_;
  CostScalar child0UecPerStream_;

  // Per stream row count of right child. UEC is on join keys.
  CostScalar child1RowCountPerStream_;
  CostScalar child1UecPerStream_;

  // Row count after applying equi-join predicates. UEC is on join keys.
  CostScalar equiJnRowCountPerStream_;
  CostScalar equiJnUecPerStream_;

  // ---------------------------------------------------------------------
  // Parameters to store the partitioning functions of the join children.
  // ---------------------------------------------------------------------

  // Child0 partitioning function
  PartitioningFunction* child0PartFunc_;

  // Child1 partitioning function
  PartitioningFunction* child1PartFunc_;

}; // class CostMethodJoin

//<pb>
// ----QUICKSEARCH FOR HJ.................................................

/**********************************************************************/
/*                                                                    */
/*                         CostMethodHashJoin                         */
/*                                                                    */
/**********************************************************************/
class CostMethodHashJoin : public CostMethodJoin
{
public:
  // Constructor
  CostMethodHashJoin()
    : CostMethodJoin("CostMethodHashJoin"),
      bufferSize_((CostPrimitives::getBasicCostFactor(GEN_HSHJ_BUFFER_SIZE) - 24)/1024),
      hashedRowOverhead_(
	(Lng32)CostPrimitives::getBasicCostFactor(HH_OP_HASHED_ROW_OVERHEAD)),
      initialBucketCountPerCluster_(
	(Lng32)CostPrimitives::getBasicCostFactor(HJ_INITIAL_BUCKETS_PER_CLUSTER)),
      cpuCostAllocateHashTable_(
	CostPrimitives::getBasicCostFactor(HH_OP_ALLOCATE_HASH_TABLE)),
      cpuCostInsertRowToChain_(
	CostPrimitives::getBasicCostFactor(HH_OP_INSERT_ROW_TO_CHAIN)),
      cpuCostPositionHashTableCursor_(
	CostPrimitives::getBasicCostFactor(HH_OP_PROBE_HASH_TABLE)),
      stage1cvBK_(NULL),
      stage2cvBK_(NULL),
      stage2cvFR_(NULL),
      stage2cvLR_(NULL),
      stage3cvFR_(NULL),
      stage3cvLR_(NULL),
      stage3cvBK_(NULL)
  {}
  
  //Constructor 2
  
  CostMethodHashJoin(HashJoin* hj)
    : hj_(hj),
      CostMethodJoin("CostMethodHashJoin"),
      bufferSize_((CostPrimitives::getBasicCostFactor(GEN_HSHJ_BUFFER_SIZE) - 24)/1024),
      hashedRowOverhead_(
	(Lng32)CostPrimitives::getBasicCostFactor(HH_OP_HASHED_ROW_OVERHEAD)),
      initialBucketCountPerCluster_(
	(Lng32)CostPrimitives::getBasicCostFactor(
	  HJ_INITIAL_BUCKETS_PER_CLUSTER)),
      cpuCostAllocateHashTable_(
	CostPrimitives::getBasicCostFactor(HH_OP_ALLOCATE_HASH_TABLE)),
      cpuCostInsertRowToChain_(
	CostPrimitives::getBasicCostFactor(HH_OP_INSERT_ROW_TO_CHAIN)),
      cpuCostPositionHashTableCursor_(
	CostPrimitives::getBasicCostFactor(HH_OP_PROBE_HASH_TABLE)),
      stage1cvBK_(NULL),
      stage2cvBK_(NULL),
      stage2cvFR_(NULL),
      stage2cvLR_(NULL),
      stage3cvFR_(NULL),
      stage3cvLR_(NULL),
      stage3cvBK_(NULL)
  {}

  // ---------------------------------------------------------------------
  // Some helper methods.
  // ---------------------------------------------------------------------
  Lng32 computeInitialCountOfClusters( const CostScalar& memoryLimit,
                                      const CostScalar& tableSize);

  Lng32 computeIdealCountOfClusters( const CostScalar& memoryLimit,
                                    const CostScalar& tableSize);

  inline CostScalar computeCreateHashTableCost(
    const CostScalar& rowCount
    ) const;

  CostScalar computeTotalProbingCost();

  CostScalar computeIntervalProbingCost( const CostScalar& outerRowCount,
					       CostScalar  outerUec,
                                         const CostScalar& innerRowCount,
					       CostScalar  innerUec);

  // ---------------------------------------------------------------------
  // cacheParameters().
  // ---------------------------------------------------------------------
  virtual void cacheParameters(RelExpr* op, const Context* myContext);

  // ---------------------------------------------------------------------
  // This method computes derived parameters associated with a HJ's three
  // stages of operation. It assumes both cacheParameters() as well as
  // estimateDegreeOfParallelism() have been called.
  // ---------------------------------------------------------------------
  void deriveParameters();

  // ---------------------------------------------------------------------
  // Costing methods.
  // ---------------------------------------------------------------------
  
  virtual Cost* computePlanCost( RelExpr*             hashJoinOp,
                                 const Context*       myContext,
                                 const PlanWorkSpace* pws,
                                 Lng32                 planNumber);

  SimpleCostVector computeNewBlockingCost
                                          (HashJoinCost*,
                                           CostPtr,
                                           CostPtr,
                                           const ReqdPhysicalProperty *);

  // ---------------------------------------------------------------------
  // Method to compute costs at various stages of Hash Join. Results are
  // stored in the corresponding private members of simple cost vectors.
  // ---------------------------------------------------------------------
  void computeStage1Cost();
  void computeStage2Cost();
  void computeStage3Cost();

protected:
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

  // SCM costing method
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

  CostScalar scmComputeOverflowCost( CostScalar numBuildTuples, CostScalar buildRowSize, CostScalar numProbeTuples, CostScalar probeRowSize);

  // ---------------------------------------------------------------------
  // Clean up the cost vectors at various stages.
  // ---------------------------------------------------------------------
  virtual void cleanUp();

//<pb>
private:

  // The hash join operator being costed.
  HashJoin* hj_;

  // ---------------------------------------------------------------------
  // Constant cached parameters follow.
  // ---------------------------------------------------------------------

  // The size of a local or result buffer used by HJ.
  const double bufferSize_;

  // The additional storage (in bytes) to store a row in a hash chain.
  const Lng32 hashedRowOverhead_;

  // The no of buckets allocated initially for a cluster.
  const Lng32 initialBucketCountPerCluster_;

  const CostScalar cpuCostAllocateHashTable_;

  const CostScalar cpuCostInsertRowToChain_;

  const CostScalar cpuCostPositionHashTableCursor_;

  // ---------------------------------------------------------------------
  // Dynamically cached parameters follow.
  // ---------------------------------------------------------------------

  // Cost to compute the hash value of a row.
  CostScalar cpuCostHashRow_;

  // Length of a row from the inner table in bytes.
  Lng32 child0RowLength_;

  // Extented Length of a row from the inner table in bytes.
  Lng32 extChild0RowLength_;

  // Length of a row from the outer table in bytes.
  Lng32 child1RowLength_;

  // Extented Length of a row from the outer table in bytes.
  Lng32 extChild1RowLength_;

  // Cost to copy a row from the inner table to buffer.
  CostScalar cpuCostCopyChild0Row_;

  // Cost to copy a row from the outer table to buffer.
  CostScalar cpuCostCopyChild1Row_;

  // Cost to compare a pair of hash keys.
  CostScalar cpuCostCompareHashKeys_;

  // Cost to evaluate other join preds.
  CostScalar cpuCostEvalOtherJoinPreds_;

  // Cost to evaluate other selection preds.
  CostScalar cpuCostEvalOtherSelPreds_;

  // Cost to null instantiated a row for some left joins.
  CostScalar cpuCostNullInst_;

  // Cost of probing the hash table by the complete set of outer rows.
  CostScalar cpuCostTotalProbing_;

  // ---------------------------------------------------------------------
  // Derived parameters follow.
  // ---------------------------------------------------------------------

  // Estimated final no of clusters allocated after splitting occurred.
  CostScalar noOfClusters_;

  // No of clusters occupied by inner table.
  CostScalar noOfInnerClustersOccupied_;

  // No of clusters occupied by outer table.
  CostScalar noOfOuterClustersOccupied_;

  // No of inner table clusters which remain in memory after Stage 1.
  CostScalar noOfInnerClustersInMemory_;

  // No of outer table clusters which are flushed to disk after Stage 2.
  CostScalar noOfOuterClustersFlushed_;

  // Average size of an inner cluster before overflow
  CostScalar innerClusterSize_;

  // Average size of an inner cluster taking into account overflow
  CostScalar clusterSizeAfterSplitsOverFlow_;

  // Average size of an outer cluster.
  CostScalar outerClusterSize_;

  // Estimated major memory needs of the whole hash join operation.
  CostScalar mem_;

  CostScalar estimatedNumberOfOverflowClusters_;

  // Estimated fraction of work done in Stage 2 before FR is produced.
  CostScalar stage2WorkFractionForFR_;

  // Estimated fraction of work done in Stage 3 before FR is produced.
  CostScalar stage3WorkFractionForFR_;

  // ---------------------------------------------------------------------
  // Cost vectors computed at various stages of HJ.
  // ---------------------------------------------------------------------
  SimpleCostVector* stage1cvBK_;
  SimpleCostVector* stage2cvBK_;
  SimpleCostVector* stage3cvBK_;
  SimpleCostVector* stage2cvFR_;
  SimpleCostVector* stage2cvLR_;
  SimpleCostVector* stage3cvFR_;
  SimpleCostVector* stage3cvLR_;

}; // class CostMethodHashJoin

//<pb>
// ----QUICKSEARCH FOR MJ.................................................

/**********************************************************************/
/*                                                                    */
/*                        CostMethodMergeJoin                         */
/*                                                                    */
/**********************************************************************/
class CostMethodMergeJoin : public CostMethodJoin
{
public:
  // Constructor
  CostMethodMergeJoin()
    : CostMethodJoin("CostMethodMergeJoin"),
      cpuCostPerProbeInit_(
	CostPrimitives::getBasicCostFactor(MJ_CPUCOST_INITIALIZE)),
      cpuCostInsertRowToList_(
	CostPrimitives::getBasicCostFactor(MJ_CPUCOST_INSERT_ROW_TO_LIST)),
      cpuCostRewindList_(
	CostPrimitives::getBasicCostFactor(MJ_CPUCOST_REWIND_LIST)),
      cpuCostGetNextRowFromList_(
	CostPrimitives::getBasicCostFactor(MJ_CPUCOST_GET_NEXT_ROW_FROM_LIST)),
      cpuCostClearList_(
	CostPrimitives::getBasicCostFactor(MJ_CPUCOST_CLEAR_LIST)),
      cpuCostCopyAtp_(CostPrimitives::getBasicCostFactor(EX_OP_COPY_ATP)),
      listNodeSize_(CostPrimitives::getBasicCostFactor(MJ_LIST_NODE_SIZE))
  {}

  // ---------------------------------------------------------------------
  // cacheParameters().
  // ---------------------------------------------------------------------
  virtual void cacheParameters(RelExpr* op, const Context* myContext);

  // ---------------------------------------------------------------------
  // Some helper methods.
  // ---------------------------------------------------------------------
  CostScalar computeTotalMergingCost(CostScalar& mem);

  CostScalar computeIntervalMergingCost( CostScalar  child0RowCount,
					 CostScalar  child0Uec,
					 CostScalar  child1RowCount,
					 CostScalar  child1Uec,
					 CostScalar& mem );


  // ---------------------------------------------------------------------
  // Costing methods.
  // ---------------------------------------------------------------------
  virtual Cost* computePlanCost( RelExpr*             mergeJoinOp,
                                 const Context*       myContext,
                                 const PlanWorkSpace* pws,
                                 Lng32                 planNumber);

protected:
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

  virtual Cost* mergeNoLegsBlocking( const CostPtr leftChildCost,
                                     const CostPtr rightChildCost,
                                     const ReqdPhysicalProperty* const rpp);

  virtual Cost* mergeBothLegsBlocking( const CostPtr leftChildCost,
                                       const CostPtr rightChildCost,
                                       const ReqdPhysicalProperty* const rpp);

//<pb>
private:

  // The operator being costed.
  MergeJoin* mj_;

  // ---------------------------------------------------------------------
  // Parameters cached by constructor.
  // ---------------------------------------------------------------------

  // CPU cost for initialization.
  const CostScalar cpuCostPerProbeInit_;

  // CPU cost to copy an ATP.
  const CostScalar cpuCostCopyAtp_;

  // ---------------------------------------------------------------------
  // Scalars dealing with the maintainence of the list of duplicate right
  // rows. (also cached by constructor.)
  // ---------------------------------------------------------------------

  const CostScalar cpuCostInsertRowToList_;

  const CostScalar cpuCostRewindList_;

  const CostScalar cpuCostGetNextRowFromList_;

  const CostScalar cpuCostClearList_;

  const CostScalar listNodeSize_;

  // ---------------------------------------------------------------------
  // Dynamic cached parameters follow.
  // ---------------------------------------------------------------------

  // CPU cost to compare the merge join keys.
  CostScalar cpuCostCompareKeys_;

  // CPU cost to evaluate join predicates other than merge join preds.
  CostScalar cpuCostEvalOtherJoinPreds_;

  // CPU cost to evaluate the remaining selection predicates.
  CostScalar cpuCostEvalOtherSelPreds_;

  // CPU cost to null-instantiated a row.
  CostScalar cpuCostNullInst_;

}; // class CostMethodMergeJoin

//<pb>
// ----QUICKSEARCH FOR NJ.................................................

/**********************************************************************/
/*                                                                    */
/*                        CostMethodNestedJoin                        */
/*                                                                    */
/**********************************************************************/
class CostMethodNestedJoin : public CostMethodJoin
{
public:
  // Constructor
  CostMethodNestedJoin( const char * className = "CostMethodNestedJoin" )
    : CostMethodJoin( className ),
      cpuCostPerProbeInit_(
	CostPrimitives::getBasicCostFactor(NJ_CPUCOST_INITIALIZE)),
      cpuCostPassRow_(
	CostPrimitives::getBasicCostFactor(NJ_CPUCOST_PASS_ROW)),
      bufferCount_(
	CostPrimitives::getBasicCostFactor(NJ_INITIAL_BUFFER_COUNT)),
      bufferSize_(CostPrimitives::getBasicCostFactor(NJ_INITIAL_BUFFER_SIZE))
  {}

  // Cost functions
  virtual Cost* computePlanCost( RelExpr*             nestedJoinOp,
                                 const Context*       myContext,
                                 const PlanWorkSpace* pws,
                                 Lng32                 planNumber);
protected:

  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

  // SCM costing method
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

  virtual Cost* mergeNoLegsBlocking( const CostPtr leftChildCost,
                                     const CostPtr rightChildCost,
                                     const ReqdPhysicalProperty* const rpp);

  virtual Cost* mergeLeftLegBlocking( const CostPtr leftChildCost,
                                      const CostPtr rightChildCost,
                                      const ReqdPhysicalProperty* const rpp);

  virtual Cost* mergeRightLegBlocking( const CostPtr leftChildCost,
                                       const CostPtr rightChildCost,
                                       const ReqdPhysicalProperty* const rpp);

  virtual Cost* mergeBothLegsBlocking( const CostPtr leftChildCost,
                                       const CostPtr rightChildCost,
                                       const ReqdPhysicalProperty* const rpp);

  // To cache dynamic parameters.
  virtual void cacheParameters(RelExpr* op, const Context *myContext);

  // The NestedJoin operator being costed.
  NestedJoin* nj_;

  // Dynamic cost factors which are cached by cacheParameters().

  // Cost to evaluate the predicates which need to be evaluated at this NJ.
  CostScalar cpuCostEvalPred_;

  // Cost to null-instantiate a row in the case of left join.
  CostScalar cpuCostNullInst_;

  // ---------------------------------------------------------------------
  // Constant cost factors cached while the object is constructed.
  // ---------------------------------------------------------------------

  // Per probe initialization CPU cost for a NJ operator.
  const CostScalar cpuCostPerProbeInit_;

  // CPU cost to pass the row from the left to the right.
  const CostScalar cpuCostPassRow_;

  // No of buffers allocated to hold null instantiated rows
  const double bufferCount_;

  // Size of a buffer allocated to hold null instantiated rows.
  const double bufferSize_;

}; // class CostMethodNestedJoin

//<pb>
// ----QUICKSEARCH FOR NJF................................................

/**********************************************************************/
/*                                                                    */
/*                      CostMethodNestedJoinFlow                      */
/*                                                                    */
/**********************************************************************/
class CostMethodNestedJoinFlow : public CostMethodNestedJoin
{
public:
  // Constructor
  CostMethodNestedJoinFlow() 
    : CostMethodNestedJoin( "CostMethodNestedJoinFlow" ) {}


protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

}; // class CostMethodNestedJoinFlow

//<pb>

// ----QUICKSEARCH FOR MU.................................................

/**********************************************************************/
/*                                                                    */
/*                        CostMethodMergeUnion                        */
/*                                                                    */
/**********************************************************************/
class CostMethodMergeUnion : public CostMethod
{
public:
  // Constructor
  CostMethodMergeUnion()
    : CostMethod("CostMethodMergeUnion"),
      cpuCostPerProbeInit_(
	CostPrimitives::getBasicCostFactor(MU_CPUCOST_INITIALIZE)),
      cpuCostCopyAtp_(CostPrimitives::getBasicCostFactor(EX_OP_COPY_ATP)),
      bufferCount_(
	CostPrimitives::getBasicCostFactor(MU_INITIAL_BUFFER_COUNT)),
      bufferSize_(CostPrimitives::getBasicCostFactor(MU_INITIAL_BUFFER_SIZE))
  {}

  // Cost functions
  virtual Cost* computePlanCost( RelExpr*             op,
                                 const Context*       myContext,
                                 const PlanWorkSpace* pws,
                                 Lng32                 planNumber);

protected:

  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

  virtual Cost* mergeNoLegsBlocking( const CostPtr leftChildCost,
                                     const CostPtr rightChildCost,
                                     const ReqdPhysicalProperty* const rpp);

  virtual Cost* mergeBothLegsBlocking( const CostPtr leftChildCost,
                                       const CostPtr rightChildCost,
                                       const ReqdPhysicalProperty* const rpp);

  virtual void cacheParameters(RelExpr* op, const Context* myContext);
//<pb>

private:

  // The MergeUnion operator being costed.
  MergeUnion* mu_;

  // ---------------------------------------------------------------------
  // Dynamic cost factors which are cached by cacheParameters().
  // ---------------------------------------------------------------------

  // Cost to copy a row from the child to parent.
  CostScalar cpuCostCopyRow_;

  // Cost to copy the sort key if there is any.
  CostScalar cpuCostCompareKeys_;

  // ---------------------------------------------------------------------
  // Constant cost factors cached while the object is constructed.
  // ---------------------------------------------------------------------

  // Per probe initialization CPU cost for the MU operator.
  const CostScalar cpuCostPerProbeInit_;

  // CPU cost to copy the atp of the a row.
  const CostScalar cpuCostCopyAtp_;

  // No of buffers allocated.
  const double bufferCount_;

  // Size of a buffer allocated.
  const double bufferSize_;

}; // class CostMethodMergeUnion

//<pb>
// ----QUICKSEARCH FOR ROOT...............................................

/**********************************************************************/
/*                                                                    */
/*                       CostMethodRelRoot                            */
/*                                                                    */
/**********************************************************************/
class CostMethodRelRoot : public CostMethod
{
public:
  // Constructor
  CostMethodRelRoot() : CostMethod("CostMethodRelRoot")	{}

protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);
private:

}; // class CostMethodRelRoot

//<pb>
// ----QUICKSEARCH FOR TUPLE..............................................

/**********************************************************************/
/*                                                                    */
/*                          CostMethodTuple                           */
/*                                                                    */
/**********************************************************************/
class CostMethodTuple : public CostMethod
{
public:
  // Constructor
  CostMethodTuple()
    : CostMethod("CostMethodTuple"),
      cpuCostAllocateTuple_(
	CostPrimitives::getBasicCostFactor(EX_OP_ALLOCATE_TUPLE))
  {}

protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);


private:

  // Cost to allocate a tuple.
  const CostScalar cpuCostAllocateTuple_;

}; // class CostMethodTuple


//<pb>
/**********************************************************************/
/*                                                                    */
/*                          CostMethodTranspose                       */
/*                                                                    */
/**********************************************************************/
class CostMethodTranspose : public CostMethod
{
public:
  // Constructor
  CostMethodTranspose() : CostMethod( "CostMethodTranpose" )  {}

protected:

  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);


  virtual void cacheParameters(RelExpr* op, const Context* myContext);

  CostScalar cpuCostToProduceAllRows_;

};                                     // class CostMethodTranspose
//<pb>
/**********************************************************************/
/*                                                                    */
/*                          CostMethodStoredProc                      */
/*                                                                    */
/**********************************************************************/
class CostMethodStoredProc : public CostMethod
{
public:
  // Constructor
  CostMethodStoredProc() : CostMethod( "CostMethodStoredProc" )	{}

protected:

  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

};                                     // class CostMethodStoredProc

//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodHbaseInsert                      */
/*                                                                    */
/**********************************************************************/
class CostMethodHbaseInsert : public CostMethod
{
public:
  // Constructor
  CostMethodHbaseInsert() : CostMethod( "CostMethodHbaseInsert" ) {}

protected:
  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

  virtual void cacheParameters(RelExpr*, const Context *);
  virtual void cleanUp();

protected:
  
  // storage related
  // TODO: add members to help compute whether inserts will
  // fit in memstore or not if this seems interesting

  // parallelism related
  CostScalar activePartitions_;
  CostScalar activeCpus_;
  CostScalar streamsPerCpu_;
  CostScalar countOfAsynchronousStreams_;

}; // class CostMethodHbaseInsert



//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodHbaseUpdateOrDelete              */
/*                                                                    */
/**********************************************************************/
class CostMethodHbaseUpdateOrDelete : public CostMethod
{
public:
  // Constructor
  CostMethodHbaseUpdateOrDelete( const char* className )
    : CostMethod( className ) 
  {};

protected:

  // Old model cost function (obsolete; only here because base
  // class requires an implementation)
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams) = 0;

  NABoolean allKeyColumnsHaveHistogramStatistics(
    const IndexDescHistograms & histograms,
    const IndexDesc * CIDesc
    ) const;

  CostScalar numRowsToScanWhenAllKeyColumnsHaveHistograms(
    IndexDescHistograms & histograms,
    const ColumnOrderList & keyPredsByCol,
    const CostScalar & activePartitions,
    const IndexDesc * CIDesc
    ) const;

void computeIOCostsForCursorOperation(
    CostScalar & randomIOs,        // out
    CostScalar & sequentialIOs,    // out
    const IndexDesc * CIDesc,
    const CostScalar & numRowsToScan,
    NABoolean probesInOrder
    ) const;

protected:

};
//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodHbaseUpdate                        */
/*                                                                    */
/**********************************************************************/
class CostMethodHbaseUpdate : public CostMethodHbaseUpdateOrDelete
{
public:
  // Constructor
  CostMethodHbaseUpdate()
    : CostMethodHbaseUpdateOrDelete( "CostMethodHbaseUpdate" )  {}

protected:
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);


}; // class CostMethodHbaseUpdate

//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodHbaseDelete                        */
/*                                                                    */
/**********************************************************************/
class CostMethodHbaseDelete : public CostMethodHbaseUpdateOrDelete
{
public:
  // Constructor
  CostMethodHbaseDelete()
    : CostMethodHbaseUpdateOrDelete( "CostMethodHbaseDelete" )  {}

protected:
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

}; // class CostMethodHbaseDelete


/**********************************************************************/
/*                                                                    */
/*                          CostMethodUnPackRows                      */
/*                                                                    */
/**********************************************************************/
class CostMethodUnPackRows : public CostMethod
{
public:
  // Constructor
  CostMethodUnPackRows() : CostMethod( "CostMethodUnPackRows" )	{}

protected:

  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);


  virtual void cacheParameters(RelExpr* op, const Context* myContext);

  CostScalar cpuCostToProduceAllRows_;
};                                     // class CostMethodUnPackRows

/**********************************************************************/
/*                                                                    */
/*                          CostMethodRelSequence                      */
/*                                                                    */
/**********************************************************************/
class CostMethodRelSequence : public CostMethod
{
public:
  // Constructor
  CostMethodRelSequence()
    : CostMethod( "CostMethodRelSequence" ),
      cpuCostToProduceAllRows_( csZero ),
      historyBufferWidthInBytes_(0),
      historyBufferSizeInBytes_(0)
  {}

protected:

  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams);

  virtual void cacheParameters(RelExpr* op, const Context* myContext);

  // Cached cost scalar for the cost to produce all rows (across all
  // streams) for the Sequence node
  //
  CostScalar cpuCostToProduceAllRows_;

  // The width of the history buffer to be allocated in the executor.
  //
  Lng32 historyBufferWidthInBytes_;

  // The total size of the history buffer to be allocated in the
  // executor.
  //
  Lng32 historyBufferSizeInBytes_;
};                                     // class CostMethodRelSequence

/**********************************************************************/
/*                                                                    */
/*                          CostMethodSample                          */
/*                                                                    */
/**********************************************************************/
class CostMethodSample : public CostMethod
{
public:
  // Constructor
  CostMethodSample()
    : CostMethod( "CostMethodSample" ),
      cpuCostToProduceAllRows_(csZero)
  {}

protected:

  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);
  // SCM Cost function
  virtual Cost* scmComputeOperatorCostInternal( RelExpr* op,
                                                const PlanWorkSpace* pws,
                                                Lng32& countOfStreams
                                              );
  CostScalar cpuCostToProduceAllRows_;

};                                     // class CostMethodSample


/**********************************************************************/
/*                                                                    */
/*                          CostMethodIsolatedScalarUDF               */
/*                                                                    */
/**********************************************************************/
class CostMethodIsolatedScalarUDF : public CostMethod
{
public:
   // Constructor
   CostMethodIsolatedScalarUDF() 
     : CostMethod( "CostMethodIsolatedScalarUDF" )
   {}

private:

protected:
   // Cost functions
   virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                             const Context* myContext,
                                             Lng32&    countOfStreams);
   // SCM cost function
   virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                                const PlanWorkSpace* pws,
                                                Lng32&    countOfStreams);

};
//<pb>
/**********************************************************************/
/*                                                                    */
/*                          CostMethodTableMappingUDF                 */
/*                                                                    */
/**********************************************************************/
class CostMethodTableMappingUDF : public CostMethod
{
public:
  // Constructor
  CostMethodTableMappingUDF() : CostMethod( "CostMethodTableMappingUDF" )	{}

protected:

  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

     // SCM cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
                                               Lng32&    countOfStreams);

};                              

//<pb>
/**********************************************************************/
/*                                                                    */
/*                          CostMethodFastExtract                     */
/*                                                                    */
/**********************************************************************/
class CostMethodFastExtract : public CostMethod
{
public:
  // Constructor
  CostMethodFastExtract() : CostMethod( "CostMethodFastExtract" )	{}

protected:

  // Cost functions
  virtual Cost* computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams);

     // SCM cost function
  virtual Cost* scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
                                               Lng32&    countOfStreams);

};
#endif /* COSTMETHOD_H */

