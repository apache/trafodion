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
/**********************************************************************/
/* -*-C++-*-
**************************************************************************
*
* File:         CostMethod.C
* Description:  Optimizer cost estimation interface object
* Created:      3/97
* Language:     C++
*
* Purpose:	 Simple Cost Vector Reduction
*
*
*
**************************************************************************
*/

#include "GroupAttr.h"
#include "AllRelExpr.h"
#include "RelPackedRows.h"
#include "RelSequence.h"
#include "RelSample.h"
#include "AllItemExpr.h"
#include "ItemSample.h"
#include "opt.h"
#include "EstLogProp.h"
#include "DefaultConstants.h"
#include "ItemOther.h"
#include "ScanOptimizer.h"
#include "NAFileSet.h"
#include "SchemaDB.h"
#include "opt_error.h"

#include "CostMethod.h"
#include "Cost.h"
#include "NodeMap.h"
#include <math.h>
#include "OptimizerSimulator.h"
#include "CmpStatement.h"

//THREAD_P CostMethod* CostMethod::head_ = NULL;

#ifndef NDEBUG
static THREAD_P FILE* pfp = NULL;
#endif // NDEBUG

//<pb>

// A function having an external linkage to allow display() to
// be called. This is a workaround for bugs/missing
// functionality in ObjectCenter that cause display() to become
// an undefined symbol.
// LCOV_EXCL_START :dpm
void displayCostMethod(const CostMethod& pf)
  {
  pf.display();
  }

void displayCostMethod(const CostMethod* pf)
  {
  if (pf)
    pf->display();
  }
// LCOV_EXCL_STOP

//<pb>
//************************************************
// Global functions related to unary cost roll-up.
//************************************************

//==============================================================================
//  Combine and roll up cost of a non-blocking parent and its single child.
//
// Input:
//  parentOnly  -- Cost of parent independent of its child.
//
//  childRollUp -- Combined cost of child and all its dependents.
//
//  rpp         -- Parent's required physical properties necessary for
//                  performing a blocking additon.
//
// Output:
//  none
//
// Return:
//  Rolled up cost.
//
//==============================================================================
// LCOV_EXCL_START :cnu -- OCM code
Cost*
rollUpUnaryNonBlocking(const Cost& parentOnly,
                       const Cost& childRollUp,
                       const ReqdPhysicalProperty* const rpp)
{

  //-----------------------
  //  Create an empty cost.
  //-----------------------
  Cost* rollUp = new STMTHEAP Cost();

  //-------------------------------------------------------------------------
  //  For total cost, simply accumulate all resource usage with simple vector
  // addition.
  //-------------------------------------------------------------------------
  rollUp->totalCost() = parentOnly.getTotalCost() + childRollUp.getTotalCost();

  //------------------------------------------------------------------------
  //  For current process first row cost, use blocking addition since the
  // parent can't proceed until it receives at least one row from its child.
  //------------------------------------------------------------------------
  rollUp->cpfr() = blockingAdd(parentOnly.getCpfr(),
                               childRollUp.getCpfr(),
                               rpp);


  //------------------------------------------------------------------------
  //  For a non-blocking roll-up, all of the child's efforts except its work
  // producing its first row overlap with the work of the parent.  Thus, the
  // formula below involves an overlapped addition of the parents last row
  // cost and the child's cumulative last row cost less its first row cost.
  // The child's first row cost is added back using simple vector addition.
  //------------------------------------------------------------------------

  rollUp->cplr() = overlapAddUnary(parentOnly.getCplr(),
                              childRollUp.getCplr() - childRollUp.getCpfr())
                   + childRollUp.getCpfr();


  //--------------------------------------------------------------------
  //  Ensure that no component of rolled up first row vector exceeds the
  // corresponding component of rolled up last row vector.
  //--------------------------------------------------------------------
  rollUp->cpfr().enforceUpperBound(rollUp->cplr());

  //-----------------------------------------------------------------------
  //  For a non-blocking roll-up, a parent simply reports whatever blocking
  // costs and overlapped process costs its child reported.  Blocking costs
  // are normalized to the parent's number of probes.
  //-----------------------------------------------------------------------
  const CostScalar & parentNumProbes = parentOnly.getCplr().getNumProbes();
  rollUp->cpbc1()     =
               childRollUp.getCpbc1().getNormalizedVersion(parentNumProbes);
  rollUp->cpbcTotal() =
               childRollUp.getCpbcTotal().getNormalizedVersion(parentNumProbes);
//jo  rollUp->opfr()      = childRollUp.getOpfr();
//jo  rollUp->oplr()      = childRollUp.getOplr();

  return rollUp;

} // rollUpUnaryNonBlocking

//<pb>
//==============================================================================
//  Combine and roll up cost of a blocking parent and its single child.
//
// Input:
//  parentOnly  -- Cost of parent independent of its child.
//
//  childRollUp -- Combined cost of child and all its dependents.
//
//  rpp         -- Parent's required physical properties necessary for
//                  performing a blocking additon.
//
// Output:
//  none
//
// Return:
//  Rolled up cost.
//
//==============================================================================
Cost*
rollUpUnaryBlocking(const Cost& parentOnly,
                    const Cost& childRollUp,
                    const ReqdPhysicalProperty* const rpp)
{

  //-----------------------
  //  Create an empty cost.
  //-----------------------
  Cost* rollUp = new STMTHEAP Cost();

  //-------------------------------------------------------------------------
  //  For total cost, simply accumulate all resource usage with simple vector
  // addition.
  //-------------------------------------------------------------------------
  rollUp->totalCost() = parentOnly.getTotalCost() + childRollUp.getTotalCost();

  //----------------------------------------------------------------------
  //  For a blocking parent, the rolled-up first row and last row costs
  // become the parent's first row and last row costs respectively.
  //----------------------------------------------------------------------
  rollUp->cpfr() = parentOnly.getCpfr();
  rollUp->cplr() = parentOnly.getCplr();

  //---------------------------------------------------------------------
  //  Compute number of probes associated with parent's preliminary cost.
  //---------------------------------------------------------------------
  const CostScalar & parentNumProbes = parentOnly.getCpbc1().getNumProbes();

  //------------------------------------------------------------------------
  //  If parent is first blocking operator encountered, set the roll-up
  // first blocking cost to the sum of the parent's first blocking cost and
  // the child roll-up last row cost.  (Remember to convert the child's last
  // row cost from an total cost to an average cost by dividing it by the
  // parent's number of probes.)
  //
  //------------------------------------------------------------------------
  if ( childRollUp.getCpbc1().isZeroVectorWithProbes() )
    {
      rollUp->cpbc1() = overlapAddUnary(parentOnly.getCpbc1(),
                                   childRollUp.getCplr() / parentNumProbes);
    }

  else
    {
      //--------------------------------------------------------------------
      //  Parent not first blocking operator.  Roll up first blocking
      // cost reported by child but normalized to parent's number of probes.
      //--------------------------------------------------------------------
      rollUp->cpbc1()            =
              childRollUp.getCpbc1().getNormalizedVersion(parentNumProbes);
    }

  //----------------------------------------------------------------------
  //  Roll up total blocking cost as the sum of the parents blocking
  // cost, the child's last row cost and the child's total blocking cost.
  // Remember to convert the child's last row cost from an total cost to
  // an average cost by dividing it by the number of probes.
  //
  //  The parent's blocking activity overlaps with the child's last row
  // activity, so these are added using overlapped addition.  The parent's
  // blocking activity, however, must wait for the child's blocking
  // activity, so the child's total blocking cost is added using
  // blocking addition.
  //
  //----------------------------------------------------------------------
  rollUp->cpbcTotal()       =
    blockingAdd(
      overlapAddUnary(parentOnly.getCpbc1(),
                      childRollUp.getCplr() / parentNumProbes),
      childRollUp.getCpbcTotal().getNormalizedVersion(parentNumProbes),
      rpp);


  //-----------------------------------------------------------------
  //  For standard blocking roll-up, a parent simply reports whatever
  // overlapped process costs its child reported.
  //-----------------------------------------------------------------
//jo  rollUp->opfr() = childRollUp.getOpfr();
//jo  rollUp->oplr() = childRollUp.getOplr();

  return rollUp;

} // rollUpUnaryBlocking
//<pb>
/**********************************************************************/
/*                                                                    */
/*                             CostMethod                             */
/*                                                                    */
/**********************************************************************/
/*               Base class for all CostMethod objects                */
/**********************************************************************/

CostMethod::CostMethod( const char* className ) : className_( className )
{
    if ( CURRENTSTMT ) {
       nextCostMethod_ = CURRENTSTMT->getCostMethodHead();
       CURRENTSTMT->setCostMethodHead(this);
    } else {
       nextCostMethod_ = NULL;
    }
}

// Print
void
CostMethod::print( FILE* ofd
                 , const char* indent
                 , const char* title
                 ) const
  {
#pragma nowarn(1506)   // warning elimination
  BUMP_INDENT(indent);
#pragma warn(1506)  // warning elimination
  fprintf(ofd,"%s ",NEW_INDENT);
  if (title)
    fprintf(ofd,"%s ",title);
  if (className_)
    fprintf(ofd,"%s ",className_);
  fprintf(ofd,"\n ");
  }                                    // CostMethod::print()

void
CostMethod::display() const  { print(); }
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// CostMethod::cleanUpAllCostMethods() is used to reset the SharedPtrs in
// the CostMethod derived classes when a longjmp occurs. This is necessary
// because the CostMethod objects may be pointing to an old statement
// heap after the longjmp(). The next statement will cause problems if
// the SharedPtrs are still pointing to the old statement heap.  This
// function may also clean up other problems that may exist in the
// CostMethod objects when a longjmp occurs.
// -----------------------------------------------------------------------
// LCOV_EXCL_START :rfi
void
CostMethod::cleanUpAllCostMethods()
{
   if ( !CURRENTSTMT )
     return;

  for (CostMethod *cm = CURRENTSTMT->getCostMethodHead(); 
       cm != NULL; cm = cm->nextCostMethod_)
    cm->cleanUp();
}
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// CostMethod::generateZeroCostObject()
// Generate a zero cost object out of the information already cached.
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu -- OCM code
Cost* CostMethod::generateZeroCostObject()
{
  // A zero cost vector.
  SimpleCostVector cv(
    csZero,		    // CPU TIME
    csZero,		    // IO TIME
    csZero,		    // MSG TIME
    csZero,		    // idle time
    noOfProbesPerStream_    // num probes
    );

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  // Synthesize the "zero" cost object.
  return new STMTHEAP Cost( &cv, &cv, NULL, cpuCount, fragmentsPerCPU );

}
// LCOV_EXCL_STOP

// return true iff we are under a nested join
NABoolean
CostMethod::isUnderNestedJoin( RelExpr* op, const Context* myContext )
{
  // begin code extracted from CostMethod::cacheParameters ---------------
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( op	       != NULL );
  CMPASSERT( myContext != NULL );

  op_ = op;
  context_ = myContext;

  inLogProp_ = context_->getInputLogProp();
  CMPASSERT(inLogProp_ != NULL);
  noOfProbes_ = ( inLogProp_->getResultCardinality() ).minCsOne();

  ga_ = op_->getGroupAttr();
  CMPASSERT(ga_ != NULL);

  // Check if the operator has outer col references (under a NestedJoin).
  const ValueIdSet& inVis = ga_->getCharacteristicInputs();
  ValueIdSet outerRef;
  inVis.getOuterReferences(outerRef);
  hasOuterReferences_ = (NOT outerRef.isEmpty());

  // ---------------------------------------------------------------------
  // Is this operator on the right leg of a NJ? Note that we couldn't tell
  // for sure. This is just a most-of-the-time-true heuristic. In a very
  // rare case, an operator on the right leg of a NJ can have no outer
  // references and takes just one probe. One example is:
  //
  //     NJ                     Table A can produce exactly one row and
  //    / \                     Table B can have no outer references if
  //   A  Mat-(A.x=B.x)         all the predicates are evaluated at Mat.
  //       |
  //       B
  // ---------------------------------------------------------------------

  isUnderNestedJoin_ = ( hasOuterReferences_ OR
      noOfProbes_.isGreaterThanOne() /* > csOne */ );
  // end code extracted from CostMethod::cacheParameters -----------------

  return isUnderNestedJoin_;
}

//<pb>
// -----------------------------------------------------------------------
// CostMethod::cacheParameters()
// -----------------------------------------------------------------------
void CostMethod::cacheParameters(RelExpr* op, const Context* myContext)
{
  (void) isUnderNestedJoin(op, myContext);

  rpp_ = context_->getReqdPhysicalProperty();

  if(rpp_)
    partReq_ = rpp_->getPartitioningRequirement();
  else
  {
    // Only a RELROOT may have a NULL rpp_.
    CMPASSERT(op->getOperatorType() == REL_ROOT);
    partReq_ = NULL;
  }
  partFunc_ = NULL;

  myLogProp_ = ga_->outputLogProp(inLogProp_);
  CMPASSERT(myLogProp_ != NULL);
  myRowCount_ = ( myLogProp_->getResultCardinality() ).minCsOne();

  // Determine if the operator is a big memory operator
  if (context_->getPlan()->isBigMemoryOperator())
  {
    isBMO_ = TRUE;
    memoryLimit_ =  CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU();
  }
  else
  {
    isBMO_ = FALSE;
    memoryLimit_ = 0.0;
  }


  isMemoryLimitExceeded_ = FALSE;

  // No of CPUs available on this system.
  countOfAvailableCPUs_ = (rpp_ ? rpp_->getCountOfAvailableCPUs() : 1);

  // Maximum count of pipelines allowed per CPU.
  countOfPipelinesPerCPU_ = (rpp_ ? rpp_->getCountOfPipelines() : 1) /
                                                    countOfAvailableCPUs_;

  // Initialization, just in case operator doesn't call estDegOfParallism().
  countOfStreams_ = 1;
  noOfProbesPerStream_ = ( noOfProbes_ ).minCsOne();

  // ---------------------------------------------------------------------
  // Support for recosting with the operator's synthesized physical prop
  // when it becomes available when we're going up the tree.
  // ---------------------------------------------------------------------
  const PhysicalProperty* spp = context_->getPlan()->getPhysicalProperty();
  if( spp != NULL )
  {
    // Set partFunc_ to the actual partitioning function.
    partFunc_ = spp->getPartitioningFunction();
    CMPASSERT( partFunc_ );

    // This is the operator's actual degree of parallelism.
    countOfAvailableCPUs_ = spp->getCurrentCountOfCPUs();
  }

} // CostMethod::cacheParameters()
//<pb>
// -----------------------------------------------------------------------
// CostMethod::estimateDegreeOfParallelism().
//
// Generic code for deciding on the degree of parallelism this operator
// exhibits for the purpose of costing. Different operators should refine
// this implementation if it's not sufficient. It assumes parameters have
// been cached.
//
// This implementation computes two parameters:
// countOfStreams_ and noOfProbesPerStream_.
// -----------------------------------------------------------------------
void CostMethod::estimateDegreeOfParallelism()
{
  if (partFunc_ != NULL)
  {
    countOfStreams_ = partFunc_->getCountOfPartitions();
    ValueIdSet partKey = partFunc_->getPartitioningKey();
    long randomFix = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_26);
    if ( (partKey.entries() == 1) AND (randomFix != 0) )
    {
      // Get first key column.
      ValueId myPartKeyCol;
      partKey.getFirst(myPartKeyCol);
      // is it a random number?
      if (myPartKeyCol.getItemExpr()->getOperatorType() == ITM_RANDOMNUM)
      {
        CostScalar activeStreams = partFunc_->getActiveStreams();
        (CostScalar)countOfStreams_ =
          MINOF(activeStreams, (CostScalar)countOfStreams_);
      }
    }
  }
  // ---------------------------------------------------------------------
  // Estimates are based on what's specified in rpp_ of an operator which
  // is the only hint available when we are going down the query tree at
  // optimization.
  // ---------------------------------------------------------------------
  else if(rpp_ != NULL)
  {
    if((partReq_ != NULL) AND
       (partReq_->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS))
    {
      countOfStreams_ = partReq_->getCountOfPartitions();
    }
    else
    {
      // must underestimate, since we are on our way down.
      countOfStreams_ = rpp_->getCountOfPipelines();
    }
  }
  else
  // ---------------------------------------------------------------------
  // No rpp specified. True only for RelRoot which runs in one stream.
  // ---------------------------------------------------------------------
  {
    CMPASSERT(op_->getOperatorType() == REL_ROOT);
    countOfStreams_ = 1;
  }

  // If this operator is on the right leg of a parallel nested join,
  // then limit the countOfStreams_ by the number of probes, because
  // if we have fewer probes than the number of streams, then some
  // streams will be inactive.
  if ((partReq_ != NULL) AND
       partReq_->isRequirementReplicateNoBroadcast() )
  {
    CostScalar tempCountOfStreams= MINOF(CostScalar(countOfStreams_),
                                             noOfProbes_.getCeiling());
    countOfStreams_ = Lng32(tempCountOfStreams.value());
  }

  CMPASSERT(countOfStreams_ > 0);

  // ---------------------------------------------------------------------
  // The following code determines the no of probes an instance of this
  // operator receives.
  // ---------------------------------------------------------------------
  // ---------------------------------------------------------------------
  // We have two possible cases:
  // 1. I am asked to execute serially, in which case countOfStreams = 1.
  // 2. I am trying to execute in parallel and the probes are distributed
  //    over each parallel instance of mine.
  //
  // In both cases, we expect noOfProbes_ is just scaled down by
  // a factor of countOfStreams_ to produce noOfProbesPerStream_.
  //
  // ---------------------------------------------------------------------
  // Using the CQD INCORPORATE_SKEW_IN_COSTING we control the enhancement done for
  // estimating number of probes per stream. If the code is activated
  // number of probes per stream will be taken as cardinality of the 
  // busiest stream
  // ---------------------------------------------------------------------

  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
	  (partFunc_ != NULL) )
  {
	noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                        countOfStreams_,
                        NULL,
                        countOfAvailableCPUs_ );
  }
  else
	noOfProbesPerStream_ = ( noOfProbes_ / countOfStreams_ ).minCsOne();

} // CostMethod::estimateDegreeOfParallelism().
//<pb>

void CostMethod::determineCpuCountAndFragmentsPerCpu(
  Lng32 & cpuCount,
  Lng32 & fragmentsPerCpu )
{

  //------------------------------------------------------------------
  //  Count of streams limits the no of cpu this operator executes on.
  //------------------------------------------------------------------
  cpuCount = ( countOfStreams_ < countOfAvailableCPUs_ ?
			 countOfStreams_ : countOfAvailableCPUs_ );
  CMPASSERT( cpuCount >= 1 );	    // sanity test

  //-------------------------------------------------------------------
  //  Compute maximum number of fragments per cpu by taking the ceiling
  // of the number of streams divided by the number of cpus.
  //-------------------------------------------------------------------
  fragmentsPerCpu = ( ( countOfStreams_ + ( cpuCount - 1 ) ) / cpuCount );
  CMPASSERT( fragmentsPerCpu >= 1 );  // sanity test

}

// -----------------------------------------------------------------------
// CostMethod::cleanUp().
// -----------------------------------------------------------------------
inline void CostMethod::cleanUp()
{
  // The EstLogPropSharedPtr values must be cleaned up.  If they are left
  // around at the end of the statement, the next statement that executes
  // will probably cause memory corruption or crashes because the pointers
  // will be pointing to the old statement heap.
  inLogProp_ = 0;
  myLogProp_ = 0;
}

// -----------------------------------------------------------------------
// CostMethod::computeOperatorCost()
// -----------------------------------------------------------------------
Cost*
CostMethod::computeOperatorCost(RelExpr* op,
                                const Context* myContext,
                                Lng32& countOfStreams)
{
  Cost* cost;
  try {
    cost = computeOperatorCostInternal(op, myContext, countOfStreams);
  } catch(...) { // LCOV_EXCL_LINE :rfi
    // cleanUp() must be called before this function is called again
    // because wrong results may occur the next time computeOperatorCost()
    // is called and because the SharedPtr objects must be set to zero.
    // Failure to call cleanUp() will very likely cause problems.
    // LCOV_EXCL_START :rfi
    cleanUp();
    throw;  // rethrow the exception
    // LCOV_EXCL_STOP
  }

  cleanUp();
  return cost;
}                     // CostMethod::computeOperatorCost()

//<pb>
//==============================================================================
//  Default implementation to produce an PartialPlan cost for a specified
// physical operator and all its known children.  Both the PartialPlan cost and
// the known children cost get stored in a specified plan workspace
//
// Input:
//  op         -- specified physical operator.
//
//  myContext  -- context associated with specified physical operator
//
//  pws        -- plan workspace associated with specified physical operator.
//
// Output:
//  none
//
// Return:
//  none
//
//==============================================================================
void
CostMethod::computePartialPlanCost(const RelExpr* op,
                                   PlanWorkSpace* pws,
                                   const Context* myContext)
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( op	       != NULL );
  CMPASSERT( pws       != NULL );
  CMPASSERT( myContext != NULL );

  //----------------------------------------------------------------------
  //  PartialPlan costing only appropriate for operators which are neither
  // leaves nor unary.
  //----------------------------------------------------------------------
  CMPASSERT( op->getArity() > 1 );

  //------------------------------------------
  //  Extract latest plan from plan workspace.
  //------------------------------------------
  Lng32 planNumber = pws->getLatestPlan();

  //---------------------------------------------------------------
  //  Extract parent operator's local cost from the plan workspace.
  // This cost should contain result of computeOperatorCost().
  //---------------------------------------------------------------
  Cost* parentCost = pws->getOperatorCost();

  //----------------------------------------
  //  Accumulate cost of all known children.
  //----------------------------------------
  Cost* knownChildrenCost = new STMTHEAP Cost();
  for ( Lng32 childIdx = 0; childIdx < op->getArity(); childIdx++ )
  {
    //-----------------------------------------------------
    //  Get current child's context via the plan workspace.
    //-----------------------------------------------------
    Context* childContext = pws->getChildContext( childIdx, planNumber );

    //----------------------------------------------
    // See if current child has an optimal solution.
    //----------------------------------------------
    if ( childContext != NULL && childContext->hasOptimalSolution() )
    {
      //--------------------------------------------------------------------
      //  Current child has an optimal solution, so accumulate its cost into
      // a cumulative cost of known children.
      //--------------------------------------------------------------------
      const Cost* childCost = childContext->getSolution()->getRollUpCost();
      knownChildrenCost->mergeOtherChildCost( *childCost );
    }
  }

  Cost* partialPlanCost = NULL;
  //-----------------------------------------------------------------
  //  Produce PartialPlan cost by rolling up known children cost with
  // parent's preliminary cost.
  //-----------------------------------------------------------------
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
    partialPlanCost = scmRollUp( parentCost,
                                 knownChildrenCost,
                                 NULL,
                                 myContext->getReqdPhysicalProperty() );
  else
    partialPlanCost = rollUp( parentCost,
                              knownChildrenCost,
                              myContext->getReqdPhysicalProperty() );

  //---------------------------------------------------------------------------
  //  Save off both PartialPlan cost and known children cost in plan workspace.
  //---------------------------------------------------------------------------
  pws->setPartialPlanCost( partialPlanCost );
  pws->setKnownChildrenCost( knownChildrenCost );

} // CostMethod::computePartialPlanCost()

//<pb>
//==============================================================================
//  Default implementation to produce a final cumulative cost for an entire
// subtree rooted at a specified physical operator.
//
// Input:
//  op         -- specified physical operator.
//
//  myContext  -- context associated with specified physical operator
//
//  pws        -- plan workspace associated with specified physical operator.
//
//  planNumber -- used to get appropriate child contexts.
//
// Output:
//  none
//
// Return:
//  Pointer to cumulative final cost.
//
//==============================================================================
Cost*
CostMethod::computePlanCost( RelExpr* op,
                             const Context* myContext,
                             const PlanWorkSpace* pws,
                             Lng32 planNumber
                           )
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( op	       != NULL );
  CMPASSERT( myContext != NULL );
  CMPASSERT( pws       != NULL );

  //--------------------------------------------------------------------------
  //  Grab parent's cost (independent of its children) directly from the plan
  // work space.  This cost should contain result of computeOperatorCost().
  //--------------------------------------------------------------------------
  // Need to cast constness away since getFinalOperatorCost cannot
  // be made const
  Cost* parentCost = ((PlanWorkSpace *)pws)->getFinalOperatorCost( planNumber );

  //----------------------------------------------------------------------------
  //  For leaf nodes (i.e. those having no children), return a copy of parent's
  // cost as the final cost.
  //----------------------------------------------------------------------------
  if ( op->getArity() == 0 )
  {
    return parentCost;
  }

  //--------------------------------------------------------
  //  Merge all children's costs in preparation for roll-up.
  //--------------------------------------------------------
  Cost mergedChildCost;
  for ( Lng32 childIdx = 0; childIdx < op->getArity(); childIdx++ )
  {
    //------------------------------------------------------
    //  Get current child's context via our plan work space.
    //------------------------------------------------------
    Context* childContext = pws->getChildContext( childIdx, planNumber );

    //------------------------------------------------------------------
    // Make sure plans are already generated by the operator's children.
    //------------------------------------------------------------------
    if ( childContext == NULL )
    {
      ABORT("CostMethod::computePlanCost(): A child has a NULL context"); // LCOV_EXCL_LINE :rfi
    }

    // Coverity flags this dereferencing null pointer childContext.
    // This is a false positive, we fix it using annotation.
    // coverity[var_deref_model]
    if ( NOT childContext->hasOptimalSolution() )
    {
      ABORT("CostMethod::computePlanCost(): A child has no solution"); // LCOV_EXCL_LINE :rfi
    }

    //---------------------------------------------
    // Accumulate this child's cost into PlanCost.
    //---------------------------------------------
    mergedChildCost.mergeOtherChildCost(
                         *childContext->getSolution()->getRollUpCost() );

  }

  if(op->getOperatorType()==REL_SHORTCUT_GROUPBY && op->getFirstNRows() == 1)
  {
    mergedChildCost.cpfr() = mergedChildCost.getCpfr() * 0.8;
    mergedChildCost.cplr() = mergedChildCost.getCpfr();
  }
  Cost* planCost = rollUp( parentCost
                          , &mergedChildCost
                          , myContext->getReqdPhysicalProperty()
                          );

  delete parentCost;

  return planCost;

} // CostMethod::computePlanCost()
//<pb>
//==============================================================================
//  Obtain copies of costs for a physical binary operator's two children.
//
// Input:
//  op             -- specified physical binary operator.
//
//  myContext      -- context associated with specified physical binary
//                     operator.
//
//  pws            -- plan work space associated with specified physical
//                     binary operator.
//
//  planNumber     -- used to get appropriate child contexts.
//
// Output:
//  leftChildCost  -- pointer to a copy of left child's cost.
//
//  rightChildCost -- pointer to a copy of right child's cost.
//
// Return:
//  none.
//
//==============================================================================
// LCOV_EXCL_START :cnu -- OCM code
void
CostMethod::getChildCostsForBinaryOp( RelExpr* op
                                    , const Context* myContext
                                    , const PlanWorkSpace* pws
                                    , Lng32  planNumber
                                    , CostPtr& leftChildCost
                                    , CostPtr& rightChildCost
                                    )
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( op	       != NULL );
  CMPASSERT( myContext != NULL );
  CMPASSERT( pws       != NULL );

  //--------------------------------------------------------------------
  //  By definition, a binary operator should have exactly two children.
  //--------------------------------------------------------------------
  CMPASSERT( op->getArity() == 2 );

  //-----------------------------------------------------------------
  //  Obtain left child's cost after verifying that left child has an
  // associated context and optimal soloution.
  //-----------------------------------------------------------------
  Context* childContext = pws->getChildContext( 0, planNumber );

  if ( childContext == NULL )
  {
    ABORT(
      "CostMethod::getChildCostsForBinaryOp(): Left child has a NULL context"
      );
  }

  // Coverity flags this dereferencing null pointer childContext.
  // This is a false positive, we fix it using annotation.
  // coverity[var_deref_model]
  if ( NOT childContext->hasOptimalSolution() )
  {
    ABORT(
      "CostMethod::getChildCostsForBinaryOp(): Left child has no solution"
      );
  }

  leftChildCost =
    new STMTHEAP Cost( *childContext->getSolution()->getRollUpCost() );

  //-------------------------------------------------------------------
  //  Obtain right child's cost after verifying that right child has an
  // associated context and optimal soloution.
  //-------------------------------------------------------------------
  childContext = pws->getChildContext( 1, planNumber );

  if ( childContext == NULL )
  {
    ABORT(
      "CostMethod::getChildCostsForBinaryOp(): Right child has a NULL context"
      );
  }

  // Coverity flags this dereferencing null pointer childContext.
  // This is a false positive, we fix it using annotation.
  // coverity[var_deref_model]
  if ( NOT childContext->hasOptimalSolution() )
  {
    ABORT(
      "CostMethod::getChildCostsForBinaryOp(): Right child has no solution"
      );
  }

  rightChildCost =
    new STMTHEAP Cost( *childContext->getSolution()->getRollUpCost() );

}


void
CostMethod::getChildCostForUnaryOp( RelExpr* op
                                  , const Context* myContext
                                  , const PlanWorkSpace* pws
                                  , Lng32  planNumber
                                  , CostPtr& childCost
                                   )
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( op	       != NULL );
  CMPASSERT( myContext != NULL );
  CMPASSERT( pws       != NULL );

  //--------------------------------------------------------------------
  //  By definition, a binary operator should have exactly two children.
  //--------------------------------------------------------------------
  CMPASSERT( op->getArity() == 1 );

  //-----------------------------------------------------------------
  //  Obtain left child's cost after verifying that left child has an
  // associated context and optimal soloution.
  //-----------------------------------------------------------------
  Context* childContext = pws->getChildContext( 0, planNumber );

  if ( childContext == NULL )
  {
    ABORT(
      "CostMethod::getChildCostForUnaryOp():  child has a NULL context"
      );
  }

  // Coverity flags this dereferencing null pointer childContext.
  // This is a false positive, we fix it using annotation.
  // coverity[var_deref_model]
  if ( NOT childContext->hasOptimalSolution() )
  {
    ABORT(
      "CostMethod::getChildCostForUnaryOp():  child has no solution"
      );
  }

  childCost =
    new STMTHEAP Cost( *childContext->getSolution()->getRollUpCost() );
}

//<pb>
//==============================================================================
//  Roll up a child's cumulative cost into its parent's cost.
//
// Input:
//  parentCost -- Cost of parent independent of its child.
//
//  childCost  -- Combined cost of child and all its dependents.
//
//  rpp        -- Parent's required physical properties needed by lower level
//                  roll-up routines.
//
// Output:
//  none
//
// Return:
//  Rolled up cost.
//
//==============================================================================
Cost*
CostMethod::rollUp( Cost* const parentCost
                  , Cost* const childCost
                  , const ReqdPhysicalProperty* const rpp
                  )
{

  //------------------------------------------------------------------
  //  If current operator is non-blocking, use a non-blocking roll up.
  //------------------------------------------------------------------
  if (parentCost->getCpbc1().isZeroVectorWithProbes())
    {

      return rollUpUnaryNonBlocking(*parentCost, *childCost, rpp);
    }
  else
    {

      //-------------------------------------------------------
      //  Current operator is blocking; use a blocking roll up.
      //-------------------------------------------------------
      return rollUpUnaryBlocking(*parentCost, *childCost, rpp);
    }

} // CostMethod::rollUp()
//<pb>
//==============================================================================
//  Default implementation to produce a final cumulative cost for an entire
// subtree rooted at a specified binary physical operator.
//
// Input:
//  op         -- specified binary physical operator.
//
//  myContext  -- context associated with specified physical operator
//
//  pws        -- plan work space associated with specified physical operator.
//
//  planNumber -- used to get appropriate child contexts.
//
// Output:
//  none
//
// Return:
//  Pointer to cumulative final cost.
//
//==============================================================================
Cost*
CostMethod::rollUpForBinaryOp( RelExpr* op
                             , const Context* myContext
                             , const PlanWorkSpace* pws
                             , Lng32  planNumber
                             )
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( op	       != NULL );
  CMPASSERT( myContext != NULL );
  CMPASSERT( pws       != NULL );

  //--------------------------------------------------------------------------
  //  Get cumulative costs associated with each child of this binary operator.
  //--------------------------------------------------------------------------
  CostPtr leftChildCost;
  CostPtr rightChildCost;
  getChildCostsForBinaryOp( op
                          , myContext
                          , pws
                          , planNumber
                          , leftChildCost
                          , rightChildCost);

  //--------------------------------------------------------------------------
  //  Merging of children's costs depend on whether or not any of the children
  // have blocking operators.
  //--------------------------------------------------------------------------
  Cost* mergedChildCost;
  const ReqdPhysicalProperty* rpp = myContext->getReqdPhysicalProperty();
  if ( leftChildCost->getCpbcTotal().isZeroVectorWithProbes() )
  {
    if ( rightChildCost->getCpbcTotal().isZeroVectorWithProbes() )
    {

      //-------------------------------------------------------
      //  Neither child has a blocking operator in its subtree.
      //-------------------------------------------------------
      mergedChildCost = mergeNoLegsBlocking( leftChildCost,
					     rightChildCost,
					     rpp
					   );
    }
    else
    {

      //----------------------------------------------------------
      //  Only right child has a blocking operator in its subtree.
      // Convert left child to blocking and proceed as if both
      // children were blocking.
      //----------------------------------------------------------
      Cost* blockingLeftChildCost = convertToBlocking( leftChildCost );
      mergedChildCost = mergeBothLegsBlocking( blockingLeftChildCost,
					       rightChildCost,
					       rpp
					     );
      delete blockingLeftChildCost;
    }
  }
  else
  {
    if ( rightChildCost->getCpbcTotal().isZeroVectorWithProbes() )
    {

      //---------------------------------------------------------
      //  Only left child has a blocking operator in its subtree.
      // Convert right child to blocking and proceed as if both
      // children were blocking.
      //---------------------------------------------------------
      Cost* blockingRightChildCost = convertToBlocking( rightChildCost );
      mergedChildCost = mergeBothLegsBlocking( leftChildCost,
					       blockingRightChildCost,
					       rpp
					     );
      delete blockingRightChildCost;
    }
    else
    {

      //----------------------------------------------------------
      //  Both children have blocking operators in their subtrees.
      //----------------------------------------------------------
      mergedChildCost = mergeBothLegsBlocking( leftChildCost,
					       rightChildCost,
					       rpp
					     );
    }
  }

  //-----------------------------------------------------------------------
  //  Child costs have been merged at this point, so delete local copies of
  // those costs.
  //-----------------------------------------------------------------------
  delete leftChildCost;
  delete rightChildCost;

  //----------------------------------------------------------------------
  //  Get addressability to parent cost in plan workspace and roll this up
  // with the recently calculated merged children cost.
  //----------------------------------------------------------------------
  Cost* parentCost = ((PlanWorkSpace *)pws)->getFinalOperatorCost( planNumber );
  Cost* rollUpCost = rollUp( parentCost, mergedChildCost, rpp );

  //----------------------------------------------------------------------
  // Parent cost and local copy of merged child cost have been rolled up
  // at this point, so delete them.
  //----------------------------------------------------------------------
  delete parentCost;
  delete mergedChildCost;

  //--------------------------------------------
  //  Return previously calculated roll-up cost.
  //--------------------------------------------
  return rollUpCost;

} // CostMethod::rollUpForBinaryOp
//<pb>
//==============================================================================
//  This merge routine should never be called for CostMethod base class.
//
// Input:
//  leftChildCost  -- pointer to cumulative cost of left child.
//
//  rightChildCost -- pointer to cumulative cost of right child.
//
//  rpp            -- Parent's required physical properties.
//
// Output:
//  none
//
// Return:
//  Always NULL.
//
//==============================================================================
Cost*
CostMethod::mergeNoLegsBlocking( const CostPtr leftChildCost,
                                 const CostPtr rightChildCost,
                                 const ReqdPhysicalProperty* const rpp)
{

  ABORT("CostMethod::mergeNoLegsBlocking");
  return NULL;

} // CostMethod::mergeNoLegsBlocking
//<pb>
//==============================================================================
//  Convert a non-blocking cost into a blocking cost using a canonical
// transformation which moves the first row vector to both blocking vectors and
// then resets the first row vector.
//
//  Note:  callers must delete returned cost object.
//
// Input:
//  nonBlockingCost -- pointer to cumulative non-blocking cost.
//
// Output:
//  none
//
// Return:
//  Pointer to converted blocking cost.
//
//==============================================================================
Cost*
CostMethod::convertToBlocking( const CostPtr nonBlockingCost )
{

  //------------------------------------------------------
  // Verify that non-blocking cost is indeed non-blocking.
  //------------------------------------------------------
  CMPASSERT( nonBlockingCost != NULL );
  CMPASSERT( nonBlockingCost->getCpbc1().isZeroVectorWithProbes() );
  CMPASSERT( nonBlockingCost->getCpbcTotal().isZeroVectorWithProbes() );

  //----------------------------------------------------------------------------
  //  Convert cost vectors of non-blocking child to look like a blocking vector.
  // Move first row cost to both blocking vectors and subtract first row cost
  // from last row cost.
  //----------------------------------------------------------------------------
  Cost* blockingCost	       = new STMTHEAP Cost( *nonBlockingCost );

  blockingCost->cplr()	      -= blockingCost->cpfr();

  const CostScalar & numProbes = blockingCost->getCplr().getNumProbes();

  blockingCost->cpbcTotal()    = blockingCost->cpfr();
  blockingCost->cpbcTotal().setNumProbes( numProbes );

  blockingCost->cpbc1()	       = blockingCost->cpfr();
  blockingCost->cpbc1().setNumProbes( numProbes );

  //---------------------------------------------------------------------
  //  Zero out first row cost vector yet preserving its number of probes.
  //---------------------------------------------------------------------
  blockingCost->cpfr().reset();
  blockingCost->cpfr().setNumProbes( csOne );

  return blockingCost;

} // CostMethod::convertToBlocking
//<pb>
//==============================================================================
//  This merge routine should never be called for CostMethod base class.
//
// Input:
//  leftChildCost  -- pointer to cumulative cost of left child.
//
//  rightChildCost -- pointer to cumulative cost of right child.
//
//  rpp            -- Parent's required physical properties.
//
// Output:
//  none
//
// Return:
//  Always NULL.
//
//==============================================================================
Cost*
CostMethod::mergeBothLegsBlocking( const CostPtr leftChildCost,
                                   const CostPtr rightChildCost,
                                   const ReqdPhysicalProperty* const rpp)
{

  ABORT("CostMethod::mergeBothLegsBlocking");
  return NULL;

} // CostMethod::mergeBothLegsBlocking
//<pb>
// LCOV_EXCL_STOP

/**********************************************************************/
/*                                                                    */
/*                         CostMethodExchange                         */
/*                                                                    */
/**********************************************************************/
//<pb>
//==============================================================================
//  Compute operator cost for a specified Exchange operator.
//
// Input:
//  op             -- pointer to specified Exchange operator.
//
//  myContext      -- pointer to optimization context for this Exchange
//                     operator.
//
// Output:
//  countOfStreams -- degree of parallelism for this Exchange (i.e. number of
//                     consumers for this Exchange.)
//
// Return:
//  Pointer to computed cost object for this exchange operator.
//
//==============================================================================
// LCOV_EXCL_START :cnu -- OCM code
Cost*
CostMethodExchange::computeOperatorCostInternal(RelExpr* op,
                                                const Context* myContext,
                                                Lng32& countOfStreams)
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( op	       != NULL );
  CMPASSERT( myContext != NULL );

  //-----------------------------------
  //  Downcast to an Exchange operator.
  //-----------------------------------
  Exchange* exch = (Exchange*)op;
  op_ = op;

  //--------------------------------------------------------------------------
  // Exchange cost is calculated twice.
  //   First:  When going down the tree.  This cost is minimal and doesn't
  //           come close to reflecting the real cost.  The reason is that we
  //           often don't know how many ESPs and DP2s we are dealing with.
  //           This information is supplied by the FileScan Costing.
  //
  //   Second: When going up the tree.  At this point we know everything we
  //           will know about filescan suppied information.  Going up the
  //           tree we do a complete costing of the Exchange.
  //--------------------------------------------------------------------------
  const CostScalar numOfProbes =
    ( myContext->getInputLogProp()->getResultCardinality() ).minCsOne();

  const ReqdPhysicalProperty* rpp = myContext->getReqdPhysicalProperty();

  //------------------------------------------------------------------------
  //  If we have not synthesized physical properties, we are going down
  // the tree, so do a fast, pessimistic computation with minimal knowledge.
  //------------------------------------------------------------------------

  sppForMe_ = (PhysicalProperty *) myContext->
                                     getPlan()->getPhysicalProperty();

  if ( NOT sppForMe_) 
    return  computeExchangeCostGoingDown( rpp, numOfProbes, countOfStreams );

  //============================================================================
  //           * * *   O N   O U R   W A Y   B A C K   U P   * * *
  //
  //             THIS IS WHERE THE REAL EXCHANGE COSTING IS DONE
  //
  //  An Exchange operator acts as a broker of sorts between a set of "consumer"
  // processes (known collectively as the "parent") and a set of "producer"
  // processes (known collectively as the "child").  The parent sends requests
  // down to the child, and the child returns rows up to the parent.  The parent
  // can bundle multiple requests in a single message buffer, and the child can
  // bundle multiple rows in a single message buffer.  Computing the cost of an
  // exchange involves the following steps:
  //
  //    1. Compute number of "down" messages from parent to child.
  //    2. Compute number of "up" messages from child to parent.
  //    3. Determine how many "down" messages cross node boundaries and how many
  //        "up" messages cross node boundaries.
  //    4. Produce first row and last row cost vectors for both the parent and
  //        the child using the message counts from step 3.
  //    5. Compute the final Exchange operator cost from the vectors produced in
  //        step four.
  //
  //  There are four basic Exchange situations that we need to cost:
  //
  //    1.  dp2(s) -> master
  //    2.  esps   -> master
  //    3.  dp2s   -> esps
  //    4.  esps   -> esps (this involves either repartitioning or replication)
  //
  //  Fortunately, we don't need special code for each different case.  We need
  // to distinguish between DP2 Exchanges (cases 1 and 3) and ESP Exchanges
  // (cases 2 and 4) at certain points in the code, but for the most part the
  // exchange costing code applies to all cases.  Nevertheless, anyone who plans
  // on changing the code in the future should keep these cases in mind.
  //
  //  A final point.  We amortize the cost of an Exchange over the number of
  // consumers that will execute concurrently (i.e. the parent's degree of
  // parallelism).
  //============================================================================

  //------------------------------------------------------------
  //  Get the physical properties for the child of the Exchange.
  //------------------------------------------------------------
  const PhysicalProperty* sppForChild =
      myContext->getPhysicalPropertyOfSolutionForChild(0);

  sppForChild_ = (PhysicalProperty*) sppForChild;
  numOfProbes_ = (CostScalar )numOfProbes; 

  isOpBelowRoot_ = (*CURRSTMT_OPTGLOBALS->memo)[myContext->getGroupId()]->isBelowRoot();
  const PartitioningFunction* const childPartFunc =
    sppForChild->getPartitioningFunction();

  const PartitioningFunction* const myPartFunc =
    myContext->getPlan()->getPhysicalProperty()->getPartitioningFunction();

  NABoolean executeInDP2 = sppForChild->executeInDP2();

  //--------------------------------------------------------------------------
  //  Compute number of producer processes and consumer processes associated
  // with this Exchange.
  //
  //  Also compute number of physical partitions.  This may exceed the number
  // of producers in the case of a logical partitioning strategy known as
  // "partition grouping".  We need number of active partitions to compute the number
  // of "down" messages.
  // Note: Taking into account the "current count of CPUs" is OK for now
  // because we currently use one (reader) ESP per CPU. This may change in the
  // future as the amount of data per CPU increases. - Sunil
  //--------------------------------------------------------------------------
  const CostScalar& numOfConsumers  = ((NodeMap *)(myPartFunc->getNodeMap()))->
                                    getNumActivePartitions();
  const CostScalar& numOfPartitions = ((NodeMap *)(childPartFunc->getNodeMap()))->
                                                getEstNumActivePartitionsAtRuntime();
#pragma warning (disable : 4018)   //warning elimination
  const CostScalar& numOfProducers  = MINOF( numOfPartitions ,sppForChild->getCurrentCountOfCPUs()  );
#pragma warning (default : 4018)   //warning elimination

  //---------------------------------------------------------------
  //  Exchange operator's number of streams is parent's degree of
  // parallelism (i.e. number of concurrently executing consumers).
  //---------------------------------------------------------------
  countOfStreams = Lng32(numOfConsumers.getValue());

  //------------------------------------------------------------
  //  Get default values needed for subsequent Exchange costing.
  //------------------------------------------------------------
  CostScalar messageSpacePerRecordInKb;
  CostScalar messageHeaderInKb;
  CostScalar messageBufferSizeInKb;
  getDefaultValues(executeInDP2,
                   exch,
                   messageSpacePerRecordInKb,
                   messageHeaderInKb,
                   messageBufferSizeInKb);

  if (NOT executeInDP2)
    exch->computeBufferLength(myContext,
                            numOfConsumers,
                            numOfProducers,
                            upMessageBufferLength_,
                            downMessageBufferLength_);
  else
  {
    upMessageBufferLength_=messageBufferSizeInKb;
    downMessageBufferLength_=csOne;
  }

  //------------------------------------------------------------------
  //  Compute the number of "down" messages.
  //
  //  DownMessages are the messages flowing from the PA to root in DP2
  // or from a parent ESP to a child ESP (i.e. from a Send-top 
  // operator to a Send-bottom operator).
  //------------------------------------------------------------------
  CostScalar downMessages = computeDownMessages(numOfProbes,
                                                executeInDP2,
                                                messageHeaderInKb,
                                                messageBufferSizeInKb,
                                                numOfPartitions,
                                                numOfConsumers,
                                                myContext,
                                                downMessageLength_);

  //---------------------------------------------------------------------
  //  Compute the number of "up" messages.
  //
  // UpMessages for executeInDP2 are those flowing from root in DP2 to PA.
  //   We make pushing groupby's attractive even if without them we would
  //   send back only one buffer.  So for up messages we do not round the
  //   last buffer up to a full buffer.
  //
  // UpMessages for ESP to ESP exchanges are the number of
  //   messages needed to do the repartition or replication
  //---------------------------------------------------------------------
  
  CostScalar upMessages = computeUpMessages(myContext,
                                            exch,
                                            myPartFunc,
                                            childPartFunc,
                                            sppForChild,
                                            messageSpacePerRecordInKb,
                                            messageHeaderInKb,
                                            messageBufferSizeInKb,
                                            numOfConsumers,
                                            executeInDP2,
                                            upRowsPerConsumer_,
                                            numOfContinueDownMessages_);

   // each continue message is about 60 bytes
   if (CmpCommon::getDefault(COMP_BOOL_60) == DF_ON)
     downMessages = downMessages + numOfContinueDownMessages_; 

     downMessageLength_ = downMessageLength_ + 
                          CostScalar(60) * numOfContinueDownMessages_;

  //----------------------------------------------------------------------
  // Is merging of streams possibly needed? 
  //----------------------------------------------------------------------
  isMergeNeeded_ = (sppForMe_->getSortOrderType() != DP2_SOT) &&
                   (NOT sppForMe_->getSortKey().isEmpty()); 

  //-------------------------------------------------------------------------
  //  Given the number of "up" and "down" messages, now determine how many of
  // each type cross node boundaries.
  //-------------------------------------------------------------------------
  CostScalar downIntraNodeMessages,
             downInterNodeMessages,
             downRemoteNodeMessages,
             upIntraNodeMessages,
             upInterNodeMessages,
             upRemoteNodeMessages;
  categorizeMessages(myPartFunc,
                     childPartFunc,
                     executeInDP2,
                     downMessages,
                     upMessages,
                     downIntraNodeMessages,
                     downInterNodeMessages,
                     downRemoteNodeMessages,
                     upIntraNodeMessages,
                     upInterNodeMessages,
                     upRemoteNodeMessages);

  //-------------------------------------------------------------------------
  //  Produce first row and last row cost vectors for both the parent and the
  // child using the message information above.
  //-------------------------------------------------------------------------
  CostVecPtr myFR    = NULL;
  CostVecPtr myLR    = NULL;
  CostVecPtr childFR = NULL;
  CostVecPtr childLR = NULL;
  produceCostVectors(numOfProbes,
                     numOfConsumers,
                     numOfProducers,
                     executeInDP2,
		     myPartFunc,
                     childPartFunc,
                     messageSpacePerRecordInKb,
                     messageHeaderInKb,
                     messageBufferSizeInKb,
                     upRowsPerConsumer_,
                     downIntraNodeMessages,
                     downInterNodeMessages,
                     downRemoteNodeMessages,
                     upIntraNodeMessages,
                     upInterNodeMessages,
                     upRemoteNodeMessages,
                     myFR,
                     myLR,
                     childFR,
                     childLR);

  // adjust the Last Row / First Row Cost for an exchange
  // operator on top of a Partial GroupBy Leaf node
  const RelExpr * myImmediateChild = myContext->
    getPhysicalExprOfSolutionForChild(0);

  const RelExpr * myGrandChild = myContext->
    getPhysicalExprOfSolutionForGrandChild(0,0);

  ValueIdSet immediateChildPartKey =
    childPartFunc->getPartitioningKey();

  const PhysicalProperty* sppForGrandChild =
    myContext->
      getPhysicalPropertyOfSolutionForGrandChild(0,0);

  PartitioningFunction * grandChildPartFunc =
    (sppForGrandChild?
      sppForGrandChild->getPartitioningFunction():
        NULL);

  ValueIdSet grandChildPartKey;

  if(grandChildPartFunc)
    grandChildPartKey =
      grandChildPartFunc->
        getPartitioningKey();

  NABoolean myChildIsExchange = FALSE;
  NABoolean myChildIsSortOnTopOfHashPartGbyLeaf = FALSE;

  if (myImmediateChild)
  {
    if ((myImmediateChild->getOperatorType() == REL_SORT) &&
        myGrandChild &&
        (myGrandChild->getOperatorType() == REL_HASHED_GROUPBY) &&
        (((GroupByAgg*)myGrandChild)->isAPartialGroupByLeaf()) &&
        (CmpCommon::getDefault(COMP_BOOL_103) == DF_OFF))
      myChildIsSortOnTopOfHashPartGbyLeaf = TRUE;

    if ((myImmediateChild->getOperatorType() == REL_EXCHANGE) &&
      (CmpCommon::getDefault(COMP_BOOL_186) == DF_ON))
    myChildIsExchange = TRUE;
  }


  // childToConsider will be either the immediate child of this
  // exchange node, or the grand child. It will be the grand child
  // in case there is another exchange below this exchange i.e.
  // this exchange is on top of a PA. The grand child is only used
  // in case we want to influence the cost for partial grouping in
  // dp2. By default we don't adjust the cost for partial grouping
  // in dp2, but if COMP_BOOL_186 is ON then we allow cost adjustments
  // for exchange on top partial grouping in DP2.
  const RelExpr * childToConsider =
    ((myChildIsExchange || myChildIsSortOnTopOfHashPartGbyLeaf)?
      myGrandChild:myImmediateChild);

  ValueIdSet bottomPartKey =
    ((myChildIsExchange)?grandChildPartKey:immediateChildPartKey);

  if (childToConsider &&
      (!executeInDP2) &&
      ((childToConsider->getOperatorType() == REL_HASHED_GROUPBY)||
       (childToConsider->getOperatorType() == REL_ORDERED_GROUPBY))&&
      (((GroupByAgg*)childToConsider)->isAPartialGroupByLeaf()))
  {
    ValueIdSet childGroupingColumns =
      ((GroupByAgg*)childToConsider)->groupExpr();

    NABoolean childMatchesPartitioning = FALSE;

    if (childGroupingColumns.contains(bottomPartKey))
      childMatchesPartitioning = TRUE;

    if (!childMatchesPartitioning)
    {
      CostScalar grpByAdjFactor = (ActiveSchemaDB()->getDefaults())\
                                  .getAsDouble(ROBUST_PAR_GRPBY_EXCHANGE_FCTR);
      (*myLR) *= grpByAdjFactor;
      (*myFR) *= grpByAdjFactor;
      (*childLR) *= grpByAdjFactor;
      (*childFR) *= grpByAdjFactor;
    }
  }
CostScalar ocbAdjustFactor_2 = (ActiveSchemaDB()->getDefaults())\
                                  .getAsDouble(COMP_FLOAT_5);
CostScalar ocbAdjustFactor_3 = (ActiveSchemaDB()->getDefaults())\
                                  .getAsDouble(COMP_INT_34);
CostScalar ocbAdjustFactor_4 = (ActiveSchemaDB()->getDefaults())\
                                  .getAsDouble(COMP_INT_35);
CostScalar ocbAdjustFactor_5 = (ActiveSchemaDB()->getDefaults())\
                                  .getAsDouble(COMP_INT_36);
const InputPhysicalProperty* ippForMe =
    myContext->getInputPhysicalProperty();
  const ValueIdSet& inVis = op_->getGroupAttr()->getCharacteristicInputs();
  ValueIdSet outerRef;
  inVis.getOuterReferences(outerRef);
  NABoolean hasOuterReferences = (NOT outerRef.isEmpty());

  NABoolean isUnderNestedJoin=
       ( hasOuterReferences OR numOfProbes.isGreaterThanOne() );
  const PartitioningFunction* const parentPartFunc =
     myContext->getPlan()->getPhysicalProperty()->getPartitioningFunction();

  //--------------------------------------------------------------
  //  Compute Exchange cost using the cost vectors produced above.
  //--------------------------------------------------------------
  Cost* exchangeCost = computeExchangeCost(myFR,
                                           myLR,
                                           childFR,
                                           childLR,
                                           numOfConsumers,
                                           numOfProducers);

  //-----------------------------------------------
  //  As good citizens we clean up after ourselves.
  //-----------------------------------------------
  delete myFR;
  delete myLR;
  delete childFR;
  delete childLR;

  return exchangeCost;

} // CostMethodExchange::computeOperatorCostInternal()
//<pb>
// -----------------------------------------------------------------------
// CostMethodExchange::computeEspCost(NABoolean executeInEsp)
//------------------------------------------------------------------------
CostScalar 
CostMethodExchange::computeESPCost ( const NABoolean executeInESP
                                   , const CostScalar & numOfProbes) const
{
  CostScalar espCPUTime ( csZero );
  CostScalar espIOTime( csZero );

  if ( executeInESP )
  {
    espCPUTime  = CostPrimitives::getBasicCostFactor( CPUCOST_ESP_INITIALIZATION )
					   *  CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();
    // Assuming 2 seeks and 4k of virtual memory, i.e. 4Kbytes IO transfer
    // espIOTime =  CostScalar( 2.0 * CURRSTMT_OPTDEFAULTS->getTimePerSeek() +
    //             4.0 *CURRSTMT_OPTDEFAULTS->getTimePerSeqKb() );
    // IO component was commented out after WM bencmark. It was not clear why it was
    // put here in the first place. Nov.2005. SP.
  }

 // return new STMTHEAP SimpleCostVector( espCPUTime,       /* CPUTime */
					//espIOTime,      /* IOTime */
					//csZero,         /* MSGTime */
					//csZero,	       /* idle time */
					//numOfProbes ); /* num probes */
  return espCPUTime;

} // CostMethodExchange::computeESPCost()
//<pb>
// ------------------------------------------------------------------------------
// CostMethodExchange::computeExchangeCostGoingDown
// ------------------------------------------------------------------------------
Cost*
CostMethodExchange::computeExchangeCostGoingDown( const ReqdPhysicalProperty* rpp,
                                                  const CostScalar & numOfProbes,
                                                  Lng32& countOfStreams)
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( rpp != NULL );

  // The cost of initializing an Exchange is equivalent to 1000 cpu instructions
  // and the 1000 instructions converted into CPUTime.
  const CostScalar & cpuTime = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions() ;


  // The execution model for the Exchange is such that the result that
  // is produced by its child, i.e., a set of producer processes, is
  // distributed to one or more consumer processes. The preliminary
  // cost takes into account the initialization of communication for
  // the consumers of data. If ESPs are not persistent, this cost
  // can include a process startup cost.
  // The cost of data transfer cannot be established until a plan for
  // the child is available.
  // Note: We should use the numOfProbes to compute the MSGTime in 
  // object cv. Doing so would lead to better pruning of plans.- Sunil
  SimpleCostVector cv(
    cpuTime,
    csZero,         /* no IOTime */
    csZero,         /* no MSGTime */
    csZero,         /* idle time */
    numOfProbes	    /* num probes */
    );


  CostScalar espCount;
  PartitioningRequirement* myPartReq = rpp->getPartitioningRequirement();

  if (	   ( myPartReq != NULL )
       AND ( myPartReq->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS )
     )
  {
    espCount = myPartReq->getCountOfPartitions();
  }
  else
  {
    espCount = rpp->getCountOfPipelines();
  }

  //---------------------------------------------------------------
  //  Exchange operator's number of streams is parent's degree of
  // parallelism (i.e. number of concurrently executing consumers).
  // Note: Use of ceiling function causes over-estimation of plan 
  // fragments per cpu (when espPerCpu is multiplied by the 
  // # of active cpus). -- Sunil
  //---------------------------------------------------------------
  countOfStreams = (Lng32)espCount.value();

  CostScalar activeCPUs = MINOF( espCount,
				 CostScalar(rpp->getCountOfAvailableCPUs()) );

  CostScalar espsPerCPU = (espCount / activeCPUs).getCeiling();

  return new STMTHEAP Cost( &cv,
			    &cv,
			    NULL,
			    Lng32(activeCPUs.getValue()),
			    Lng32(espsPerCPU.getValue())
			  );

} // CostMethodExchange::computeExchangeCostGoingDown
// LCOV_EXCL_STOP
//<pb>
//==============================================================================
//  Compute default values needed for costing a specified exchange operator.
//
// Input:
//  executeInDP2              -- TRUE if child executes in DP2; FALSE otherwise.
//
//  exch                      -- pointer to specified Exchange operator.
//
// Output:
//  messageSpacePerRecordInKb -- size of a record (including any header
//                                overhead) in KB.
//
//  messageHeaderInKb         -- size of message buffer header in KB.
//
//  messageBufferSizeInKb     -- size of entire message buffer in KB.
//
// Return:
//  none
//
//==============================================================================
void
CostMethodExchange::getDefaultValues(
                                const NABoolean   executeInDP2,
                                const Exchange*   exch,
                                      CostScalar& messageSpacePerRecordInKb,
                                      CostScalar& messageHeaderInKb,
                                      CostScalar& messageBufferSizeInKb) const
{

  //-------------------------------------------------------------------------
  //  Determine message buffer size.  Messages to DP2 have a different buffer
  // size than messages sent to another ESP.
  // Note: The generator (Exchange::codeGenForESP) might choose a different
  // buffer sizes. Need to get both code routines in sync. -- Sunil
  //-------------------------------------------------------------------------
  if (executeInDP2)
    messageBufferSizeInKb =
                    CostPrimitives::getBasicCostFactor(DP2_MESSAGE_BUFFER_SIZE);
  else
    messageBufferSizeInKb =
                    CostPrimitives::getBasicCostFactor(OS_MESSAGE_BUFFER_SIZE);

  //-------------------------------------------
  //  Get addressability to the defaults table.
  //-------------------------------------------
  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  //-----------------------------------------------------------------
  //  DP2 adds a row header to each row placed in the message buffer.
  //
  //  Note: it's not clear to me why DP2 is involved in data transfer
  //  or why DP2 needs to add a row header to each row.
  //  It's the EID that moves data from the DP2 cache into its buffer
  //  and then sends it to the parent fragment (master or an ESP).
  //  The buffer containing the data is a SQLBuffer object, with its
  //  own header, format, etc. In the case of DP2-exchange, the file
  //  system places its own header (IPC protocol overhead) on top of
  //  the SQLBuffer. In the case of ESP-exchange, the send-top/send-bottom
  //  protocol adds its own header. These additional overhead need
  //  to be incorporated into the code below. -- Sunil
  //-----------------------------------------------------------------
  const CostScalar recordHeaderInBytes =
    defs.getAsLong( DP2_MESSAGE_HEADER_SIZE_BYTES ); // 18

  //--------------------------------------------------------------------
  //  Compute size (in KB) of a row including its associated row header.
  //--------------------------------------------------------------------
  const GroupAttributes* childGA = exch->child(0).getGroupAttr();
  messageSpacePerRecordInKb =
    ( recordHeaderInBytes + childGA->getRecordLength() ) / csOneKiloBytes;

  //------------------------------------------------------------------------
  //  Determine message header size (in KB).  Ensure that the size of
  // this header and the size of one row (including the row header) does
  // not exceed the size of the message buffer.  In other words, we need to
  // ensure that we have enough message buffer space to send back at least
  // one row.
  //------------------------------------------------------------------------
  messageHeaderInKb = defs.getAsDouble( DP2_MESSAGE_HEADER_SIZE );
                                                           // 18/1024. == 0.0176
  //Throw an assertion if the Message Record length is greater than that
  //of buffer size on a local node which is currently 51Kb and if
  //the size is more than 51 then the execution will fail, it is not compared
  //with DP2_MESSAGE_BUFFER_SIZE as the size is different on local and remote
  //node 51KB on local, 32KB on remote as of 03/15/2002, so compared with the
  //larger value 51KB.
  //Note: At some point in the near future, the maximum remote message size will
  // be the same as the max. local message size

  // Meantime COMP_BOOL_140 can be used to avoid this check and allow for
  // longer rows; e.g. for testing (for Hash-Join the row length is checked
  // in the generator )
  if ( CmpCommon::getDefault(COMP_BOOL_140) == DF_OFF ) {
    
    CMPASSERT( (CostScalar)
	       CostPrimitives::getBasicCostFactor(LOCAL_MESSAGE_BUFFER_SIZE)
	       >= (messageSpacePerRecordInKb + messageHeaderInKb) );
  }

} // CostMethodExchange::getDefaultValues()

//<pb>
//==============================================================================
//  Compute number of messages sent from parent of an exchange operator down to
// its child.
//
// Input:
//  numOfProbes               -- number of requests sent from parent to child.
//
//  messageHeaderInKb         -- size of message buffer header in KB.
//
//  messageBufferSizeInKb     -- size of entire message buffer in KB.
//
//  numOfPartitions           -- number child processes actually receiving
//                                messages.
//
// Output:
//  none
//
// Return:
//  Number of messages sent down to child.
//
//==============================================================================
// LCOV_EXCL_START :cnu -- OCM code
CostScalar
CostMethodExchange::computeDownMessages(
                                         const CostScalar& numOfProbes,
                                         const NABoolean executeInDP2,
                                         const CostScalar& messageHeaderInKb,
                                         const CostScalar& messageBufferSizeInKb,
                                         const CostScalar& numOfPartitions,
                                         const CostScalar& numOfConsumers,
                                         const Context* myContext,
                                         CostScalar &downMessageLength) const
{

   if (CmpCommon::getDefault(COMP_BOOL_60) == DF_ON)
   {
      return  computeDownDataAndControlMessages(numOfProbes,
                                        executeInDP2,
                                        messageHeaderInKb,
                                        messageBufferSizeInKb,
                                        numOfPartitions,
                                        numOfConsumers,
                                        myContext,
                                        downMessageLength);
   }

  //--------------------------------------------------------
  // If this is an ESP exchange (not executeInDP2)
  //  the number of messags is the number of probes times 3;
  //
  // Note: The factor of 3 comes from a start message, a message
  //  containing one probe or request, and a stop message.
  //  The formula below does not work for a TSJ operator where
  //  multiple probes are sent to a child. These probes are
  //  buffered; hence, the start/stop overhead is incurred not
  //  on a probe-basis but on a buffer-basis. The formula needs
  //  to take buffering into account. (A TSJ operator is used
  //  for inserts and for nested joins.) - Sunil
  // 
  // Note: Why multiply by the numOfConsumers (upper ESPs) ???????? 
  // The numOfProbes is already cumulative for the entire CascadesGroup!! 
  // We multiply by numOfPartitions because all probes 
  // always go to all lower ESPs. - Sunil
  //--------------------------------------------------------

    // This logic is definitely wrong. sending
    // probes down from ESP to DP2 is very similar to sending probes
    // dow from ESP to ESP and could/should have similar cost.
    // So I'll use comp_bool_29 to bypass the difference between 
    // these 2 cases if it is OFF - default value.
  if (NOT executeInDP2 AND (CmpCommon::getDefault(COMP_BOOL_29) == DF_OFF))
  {
    downMessageLength=numOfProbes * 3 * numOfPartitions*numOfConsumers;
    return  (numOfProbes * 3 * numOfPartitions*numOfConsumers);
  }

  //-------------------------------------------
  //  Get addressability to the defaults table.
  //-------------------------------------------
  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  //----------------------------------------------------
  //  Determine size of end-of-buffer indicator (in KB).
  //  Determine size of a request (in KB).
  //----------------------------------------------------
  const CostScalar endOfBufferHeaderInKb =
           defs.getAsDouble(DP2_END_OF_BUFFER_HEADER_SIZE); //32./1024 == 0.0313

  const CostScalar requestInKb =
           defs.getAsDouble(DP2_EXCHANGE_REQUEST_SIZE);     // 48/1024 == 0.0469

  //------------------------------------------------------
  // Assume uniform distribution of numOfProbes over PA's.
  // Note: In the case of DP2-exchange, for hash-partitioned 
  // tables, a probe will go to all partitions unless constant
  // values are available for all partitioning-key columns.
  // In order to represent this notion, we need code
  // here that's similar to code used to compute 
  // "repeat count" in the costing of a scan operator. - Sunil
  //------------------------------------------------------

  //---------------------------------------------------------------------
  // we change number of probes if skewness is to be considered
  // this is true for type-1 nested joins
  // for type-2 nested joins: the one with skew in dp2 would be very busy
  // so that is ok
  //---------------------------------------------------------------------
  CostScalar probesToBeCosted = ( numOfProbes / numOfPartitions ).minCsOne();
  const PhysicalProperty* sppForChild =
    myContext->getPhysicalPropertyOfSolutionForChild(0);

  const PartitioningFunction* const childPartFunc =
    sppForChild->getPartitioningFunction();
 
  PartitioningFunction* phys= NULL;
 
  if (childPartFunc->isALogPhysPartitioningFunction())
     phys = childPartFunc->
             castToLogPhysPartitioningFunction()->
             getPhysPartitioningFunction();

  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) && (phys != NULL))
  {
    CostScalar probesFromBusStream = csMinusOne;
    // check if this exchange is over a scan node
    if(sppForChild->getDP2CostThatDependsOnSPP() )
       probesFromBusStream = sppForChild->
                          getDP2CostThatDependsOnSPP()->
                          getProbesAtBusiestStream();
    probesToBeCosted = MAXOF(probesFromBusStream, probesToBeCosted);
  }

  const CostScalar numOfProbesPerPartitionInKb =
            probesToBeCosted  * requestInKb;

    DP2CostDataThatDependsOnSPP *dp2CostInfo =
     (DP2CostDataThatDependsOnSPP *) sppForChild->getDP2CostThatDependsOnSPP();


  //------------------------------------------------------------
  // Down Messages:
  //   Figure out the number of messages to dp2 (downMessages)
  //   A down message is sent to every active partition that
  //   will return rows for a request.
  //   (numOfProbes are packed in a down message; a down message
  //   will in general have as many numOfProbes as there are
  //   entries in the PA down queue). We need to take into
  //   account that there may be several partitions;
  //
  //  Down Messages for ESP exchanges use the same fields as
  //    dp2 down messages.
  //
  //  Assume that all probes will be packed contiguously into
  // the down message buffer as if all of them were immediately
  // available (this is the best case. In the worst case
  // every request is sent down in its own buffer):
  //------------------------------------------------------------
  CostScalar downMessagesPerPartition =  numOfProbesPerPartitionInKb
                                       / (  messageBufferSizeInKb
                                          - endOfBufferHeaderInKb);

  downMessagesPerPartition   = ( downMessagesPerPartition ).minCsOne();
  
  downMessageLength=downMessagesPerPartition * numOfPartitions;
  return downMessagesPerPartition * numOfPartitions;


} // CostMethodExchange::computeDownMessages()

//****************************************************************************
// computeDownDataAndControlMessages()
//****************************************************************************
CostScalar
CostMethodExchange::computeDownDataAndControlMessages( 
                                         const CostScalar& numOfProbes,
                                         const NABoolean executeInDP2,
                                         const CostScalar& messageHeaderInKb,
                                        const CostScalar& messageBufferSizeInKb,
                                         const CostScalar& numOfPartitions,
                                         const CostScalar& numOfConsumers,
                                         const Context* myContext,
                                         CostScalar &downMessageLength) const
{


  CostScalar inputRowSize = op_->getGroupAttr()->
                                 getCharacteristicInputs().getRowLength();
  if (NOT executeInDP2 )
  {

     //if (NOT inputRowSize.isLessThanOne())
     // Assume minimum of 28 bytes with each probe
     inputRowSize = inputRowSize + CostScalar(28);

     CostScalar numConnections = MINOF(
                          ActiveSchemaDB()->getDefaults().getAsLong
                                          (GEN_SNDT_NUM_BUFFERS),
                          ActiveSchemaDB()->getDefaults().getAsLong   
                                          (GEN_SNDB_NUM_BUFFERS) );

     CostScalar numOfMessages = CostScalar(2) * numOfConsumers * 
                                 numOfPartitions * numConnections;


     downMessageLength = CostScalar(64) * numOfPartitions*
                                                      numOfConsumers + // A
                          inputRowSize * numOfPartitions* 
                                                      numOfConsumers + // B
                          CostScalar(10000) * numOfPartitions;  // D
                                
    // we are charging startup overhead of exchange operator here. 
    // (A) File Open
    // (B) Open and sending input with request
    // (C) Continue message to be calculated after upMessages are calculated
    // Note that these are really short messages unlike upward messages which 
    // tend to be buffered messages
    // (D) Fixed overhead: 10k * number of producers to compete with Dp2
    // exchanges
     return numOfMessages;
  }

  downMessageLength = csZero;
  //-------------------------------------------
  //  Get addressability to the defaults table.
  //-------------------------------------------
  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  //----------------------------------------------------
  //  Determine size of end-of-buffer indicator (in KB).
  //  Determine size of a request (in KB).
  //----------------------------------------------------
  const CostScalar endOfBufferHeaderInKb =
           defs.getAsDouble(DP2_END_OF_BUFFER_HEADER_SIZE); //32./1024 == 0.0313

  const CostScalar requestInKb =
           defs.getAsDouble(DP2_EXCHANGE_REQUEST_SIZE);     // 48/1024 == 0.0469

  // this is true for type-1 nested joins
  // for type-2 nested joins: the one with skew in dp2 would be very busy
  // so that is ok
  //---------------------------------------------------------------------
  CostScalar probesToBeCosted = ( numOfProbes / numOfPartitions ).minCsOne();
  const PhysicalProperty* sppForChild =
    myContext->getPhysicalPropertyOfSolutionForChild(0);

  const PartitioningFunction* const childPartFunc =
    sppForChild->getPartitioningFunction();
 
  PartitioningFunction* phys= NULL;
 
  if (childPartFunc->isALogPhysPartitioningFunction())
     phys = childPartFunc->
             castToLogPhysPartitioningFunction()->
             getPhysPartitioningFunction();

  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) && (phys != NULL))
  {
    CostScalar probesFromBusStream = csMinusOne;
    // check if this exchange is over a scan node
    if (sppForChild->getDP2CostThatDependsOnSPP()) 
       probesFromBusStream = sppForChild->
                          getDP2CostThatDependsOnSPP()->
                          getProbesAtBusiestStream();
    probesToBeCosted = MAXOF(probesFromBusStream, probesToBeCosted);
  }

  CostScalar numOfProbes1=probesToBeCosted*numOfPartitions;

  CostScalar inputSizeInKb =
            numOfProbes1  * requestInKb + numOfProbes1 * inputRowSize/1024 ;

  //assume data messages are buffered when computing number of messages
  CostScalar downDataMessages = (inputSizeInKb / (  messageBufferSizeInKb
                                        - endOfBufferHeaderInKb)).minCsOne(); 

  const PartitioningFunction* const parentPartFunc =
     myContext->getPlan()->getPhysicalProperty()->getPartitioningFunction();

  CostScalar paPartsPerGroup;
  NABoolean parentGroupsChildren;

  // Check if the operator has outer col references (under a NestedJoin).
  const ValueIdSet& inVis = op_->getGroupAttr()->getCharacteristicInputs();
  ValueIdSet outerRef;
  inVis.getOuterReferences(outerRef);
  NABoolean hasOuterReferences = (NOT outerRef.isEmpty());

  NABoolean isUnderNestedJoin=
       ( hasOuterReferences OR numOfProbes.isGreaterThanOne() );

  // are we doing grouping; if so increase number of down messages
  // by number of PAs in a group
  NABoolean isGroupingDone = isGroupedRepartitioning(childPartFunc,
              parentPartFunc,
              parentGroupsChildren,
              paPartsPerGroup);

  NABoolean inAType2Join = FALSE;
  if (parentPartFunc->isAReplicateNoBroadcastPartitioningFunction() &&
      myContext->getInputPhysicalProperty())
  {
     const PartitioningFunction* const njParentPartFunc = 
       myContext->getInputPhysicalProperty()->
           getNjOuterOrderPartFuncForNonUpdates();
     
     if (njParentPartFunc != NULL)
     {
        isGroupingDone = isGroupedRepartitioning(childPartFunc,
            njParentPartFunc,
            parentGroupsChildren,
            paPartsPerGroup);
        if (isGroupingDone)
          inAType2Join = TRUE;
     }
  
  }


  CostScalar downControlMessageLength(csZero), downControlMessages(csZero);
  CostScalar numOfPartitions1=numOfPartitions;
  if (NOT isUnderNestedJoin)
  {
    downControlMessageLength= CostScalar(10000) * numOfPartitions;     
    downControlMessages = numOfPartitions;
    downDataMessages = downDataMessages*numOfPartitions;
    // what should we do for grouping here?
  }
  else
  {
    // this is how probes are compressd;
    CostScalar compressionFactor = defs.getAsLong(COMP_INT_60);
   const InputPhysicalProperty* ippForMe =
    myContext->getInputPhysicalProperty();
    // no compression if it is zero
    if (compressionFactor == csZero)
       compressionFactor = messageBufferSizeInKb; 

    DP2CostDataThatDependsOnSPP *dp2CostInfo =
     (DP2CostDataThatDependsOnSPP *) sppForChild->getDP2CostThatDependsOnSPP();

    /*if (isGroupingDone)
      downDataMessages = downDataMessages * paPartsPerGroup;*/

    if (dp2CostInfo !=NULL)
    {
       switch(dp2CostInfo->getRepeatCountState())
      {
        case DP2CostDataThatDependsOnSPP::KEYCOLS_COVERED_BY_CONST:
        {
           downControlMessageLength= CostScalar(10000);
           downControlMessages = csOne;
         
           // Assumption about how tightly probes are packed.
           downDataMessages = downDataMessages *
                (messageBufferSizeInKb/compressionFactor.minCsOne()).minCsOne();
           break;   
        }
        case DP2CostDataThatDependsOnSPP::KEYCOLS_COVERED_BY_PROBE_COLS_CONST:
        {
         // is this a type-2 join?

         // assumption about how tightly probes are packed
         downDataMessages = downDataMessages * 
              (messageBufferSizeInKb/compressionFactor.minCsOne()).minCsOne();


          if (parentPartFunc->isAReplicateNoBroadcastPartitioningFunction())
          {
             CostScalar actualMessages = MINOF( numOfPartitions *numOfConsumers,
                                             numOfProbes); 


             if (inAType2Join )
             {
               actualMessages = MINOF(actualMessages, 
                                      numOfConsumers * paPartsPerGroup);

               CostScalar adjFactor = defs.getAsLong(COMP_INT_61);
               if (adjFactor.isGreaterThanZero())
               {
                  if (adjFactor == csOne)
                     actualMessages = MINOF(actualMessages, paPartsPerGroup);
                  else
                     actualMessages = MINOF(actualMessages, adjFactor);
               }
             }

             downControlMessageLength= CostScalar(10000) * actualMessages; 
                                     
             downControlMessages = actualMessages;
          }
          else
          {
             CostScalar actualMessages = MINOF(numOfPartitions, numOfProbes);
             downControlMessageLength= CostScalar(10000) * actualMessages;
             downControlMessages = actualMessages;
          }
                                   
          break;
        }
        case DP2CostDataThatDependsOnSPP::KEYCOLS_NOT_COVERED:
        case DP2CostDataThatDependsOnSPP::UNKNOWN:
        {
           downControlMessageLength=
                CostScalar(10000)* numOfPartitions * numOfConsumers;
           downControlMessages = numOfPartitions * numOfConsumers ;

          if( myContext->getReqdPhysicalProperty()->getOcbEnabledCostingRequirement() )
           {
               downControlMessages = MINOF(
                                   numOfPartitions *numOfConsumers,
                                   numOfProbes);
               CostScalar interMedLength = CostScalar(10000)*numOfProbes;
               downControlMessageLength=MINOF(downControlMessageLength,
                                              interMedLength);
           }
         
           if ( myContext->getReqdPhysicalProperty()->getOcbEnabledCostingRequirement() ) 
           {
             downControlMessages = MINOF(numOfPartitions * numOfConsumers,
                                         numOfProbes);
             CostScalar interMedLength = CostScalar(10000) * numOfProbes;
             downControlMessageLength = MINOF(downControlMessageLength,
                                              interMedLength);
           }

           // Assumption about how tightly probes are packed.
           // if COMP_INT_60 = 1 then 1 k buffers are sent
           // if it is 2 then 2k buffers sent
           // if it is 56 then buffers of 56k are sent (default)
           downDataMessages = downDataMessages *
               (messageBufferSizeInKb/compressionFactor.minCsOne()).minCsOne();

           // All data goes to all partitions
           downDataMessages = downDataMessages * numOfPartitions ;
           inputSizeInKb = inputSizeInKb * numOfPartitions ;
           break;
         }
         case DP2CostDataThatDependsOnSPP::UPDATE_OPERATION:
         {
           // make serial updates more expensive than parallel updates
           downDataMessages = downDataMessages / numOfConsumers ;
           if (parentPartFunc->isAReplicateNoBroadcastPartitioningFunction())
           {
              CostScalar actualMessages = MINOF( 
                                   numOfPartitions *numOfConsumers,
                                   numOfProbes);

              downControlMessageLength= CostScalar(10000) * actualMessages;

              downControlMessages = actualMessages;
           }
           else
           {
              CostScalar actualMessages = MINOF(numOfPartitions, numOfProbes);
              downControlMessageLength= CostScalar(10000) * actualMessages;
              downControlMessages = actualMessages; 
           }
           break;
         }
      }
   }
     
  }  // Under Nested Join
  //------------------------------------------------------------
  // Down Messages:
  //   Figure out the number of messages to dp2 (downMessages)
  //   A down message is sent to every active partition that
  //   will return rows for a request.
  //   (numOfProbes are packed in a down message; a down message
  //   will in general have as many numOfProbes as there are
  //   entries in the PA down queue). We need to take into
  //   account that there may be several partitions;
  //
  //  Down Messages for ESP exchanges use the same fields as
  //    dp2 down messages.
  //
  //  Assume that all probes will be packed contiguously into
  // the down message buffer as if all of them were immediately
  // available (this is the best case. In the worst case
  // every request is sent down in its own buffer):
  //------------------------------------------------------------
  downMessageLength = downControlMessageLength + inputSizeInKb *1024; 
 
  // continue messages are computed elsewhere 
  return downDataMessages+downControlMessages;

} // CostMethodExchange::computeDownMessages()
//<pb>
//==============================================================================
//  Compute number of messages sent from child of a specified exchange operator
// up to its parent.
//
// Input:
//  exch                      -- pointer to specified Exchange operator.
//
//  parentContext             -- pointer to optimization context for specified
//                                Exchange operator.
//
//  parentPartFunc            -- pointer to parent's partitioning function.
//
//  childPartFunc             -- pointer to child's partitioning function.
//
//  sppForChild               -- pointer to child's physical properties.
//
//  messageSpacePerRecordInKb -- size of a record (including any header
//                                overhead) in KB.
//
//  messageHeaderInKb         -- size of message buffer header in KB.
//
//  messageBufferSizeInKb     -- size of entire message buffer in KB.
//
//  numOfConsumers            -- number parent processes actually receiving
//                                messages.
//
//  executeInDP2              -- TRUE if child executes in DP2; FALSE otherwise.
//
// Output:
//  upRowsPerConsumer         -- number of output rows coming up to parent.
//
// Return:
//  Number of messages sent from child up to parent.
//
//==============================================================================
CostScalar
CostMethodExchange::computeUpMessages(
                          const Context*              parentContext,
				Exchange*             exch,
                          const PartitioningFunction* parentPartFunc,
                          const PartitioningFunction* childPartFunc,
                          const PhysicalProperty*     sppForChild,
                          const CostScalar &          messageSpacePerRecordInKb,
                          const CostScalar &          messageHeaderInKb,
                          const CostScalar &          messageBufferSizeInKb,
                          const CostScalar &          numOfConsumers,
                          const NABoolean             executeInDP2,
                                CostScalar&           upRowsPerConsumer,
                                CostScalar&           numOfContinueDownMessages) const
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( parentContext  != NULL );
  CMPASSERT( exch	    != NULL );
  CMPASSERT( parentPartFunc != NULL );
  CMPASSERT( childPartFunc  != NULL );

  //---------------------------------------------------------------------
  // Up messages:
  // For DP2 access at least one up message is sent from every
  //   (active?) partition to the master. In general,
  //   a request can generate more than one up message
  //   but it must generate at least one (if only notifying that there
  //   were no matching records).  In order to promote aggregates in DP2
  //   we do not cost full buffers for the last buffer.
  //
  // For ESP to ESP access the up messages are the messages that are
  //   either repartitioned or replicated.
  //------------------------------------------------------------------

  //---------------------------------------------
  //  Determine number of rows produced by child.
  //---------------------------------------------
  EstLogPropSharedPtr inputLP       = parentContext->getInputLogProp();
  CMPASSERT( inputLP );

  EstLogPropSharedPtr childOutputLP = exch->child(0).outputLogProp(inputLP);

  // Determine number of probes and whether there are any outer references
  // (i.e. probe values)
  const CostScalar& noOfProbes = ( inputLP->getResultCardinality() ).minCsOne();
  ValueIdSet externalInputs( exch->getGroupAttr()->getCharacteristicInputs() );
  ValueIdSet outerRefs;
  externalInputs.getOuterReferences( outerRefs );

  // Calculate number of rows each consumer will receive and then calculate
  // the number of messages based on message overhead for each row and the
  // size of the message buffer.
  //------------------------------------------------------------------------
  if (CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting())
  {
    upRowsPerConsumer = childOutputLP->
                          getCardOfBusiestStream(
                              parentPartFunc,
                              (Lng32)numOfConsumers.getValue(),
                              exch->getGroupAttr(),
                              (Lng32)numOfConsumers.getValue());
    // assume number of consumers = number of CPUs;
    // it is only used for round robin pf. where skew is not an issue

  }
  else
    upRowsPerConsumer = childOutputLP->getResultCardinality() /
                        numOfConsumers;


  //------------------------------------------------------------------------
  // For broadcast replication, each child will send all rows to all consumers.
  // For no broadcast replication underneath a materialize that is not
  // passing the probes through, each consumer will read the rows of all
  // the children. This is similar to broadcast replication, so what we
  // want to do here is the same - multiply the number of rows by the
  // by the number of consumers.
  //------------------------------------------------------------------------
  if (	  parentPartFunc->isAReplicateViaBroadcastPartitioningFunction()
       OR (	parentPartFunc->isAReplicateNoBroadcastPartitioningFunction()
	    AND noOfProbes == 1	    // 1 probe, but
	    AND outerRefs.isEmpty() // no probe values - must be under a materialize
	  )
     )
  {
    //---------------------------------------------------------------------
    // All producers send all rows to all consumers.
    //---------------------------------------------------------------------
    upRowsPerConsumer = upRowsPerConsumer * numOfConsumers ;

  }

  CostScalar bufferLength;
  if (CmpCommon::getDefault(COMP_BOOL_60) == DF_ON)
    bufferLength = upMessageBufferLength_;
  else
    bufferLength = messageBufferSizeInKb;

  CostScalar  upMessagesPerConsumer =  
    ( upRowsPerConsumer  /
      ((bufferLength-messageHeaderInKb)/messageSpacePerRecordInKb ).getFloor()).getCeiling() ;

    numOfContinueDownMessages = csZero;
   
    if ( NOT upMessagesPerConsumer.isLessThanOne())
      numOfContinueDownMessages = (upMessagesPerConsumer - csOne).
                                    getCeiling() * numOfConsumers;
  if ( NOT executeInDP2 )
  {

    //----------------------------------------------------------
    //  Producer processes are ESPs.  Make sure we have at least
    // one up message per consumer.
    //----------------------------------------------------------
    return MIN_ONE_CS( upMessagesPerConsumer ) * numOfConsumers;
  }
  else
  {

    //-----------------------------------------------------------
    //  Producer processes are DP2s.  To give aggregates in DP2 a
    // slight advantage, calculate a smaller number of messages
    // when each consumer sends less than one message on average.
    //-----------------------------------------------------------
    if ( upMessagesPerConsumer.isLessThanOne() /* < csOne */ )
    {
      return MIN_ONE_CS((numOfConsumers - csOne) * upMessagesPerConsumer );
    }
    else
    {
      return upMessagesPerConsumer * numOfConsumers;
    }
  }

} // CostMethodExchange::computeUpMessages()

//**************************************************************************
// CostMethodExchange::isGroupedRepartitioning()
//
//**************************************************************************
NABoolean CostMethodExchange::isGroupedRepartitioning(
                        const PartitioningFunction* childPartFunc,
                        const PartitioningFunction* parentPartFunc,
                        NABoolean &parentGroupsChildren,
                        CostScalar& partsPerGroup) const
{
   parentGroupsChildren = FALSE;
   NABoolean childGroupsParent = FALSE;
   Lng32 myPartsPerGroup=0;

   NABoolean isParentSinglePF = (parentPartFunc->
                      castToSinglePartitionPartitioningFunction() != NULL);
   NABoolean isChildSinglePF = (childPartFunc->
                      castToSinglePartitionPartitioningFunction() != NULL);

   if (isParentSinglePF && isChildSinglePF)
      return FALSE;

   LogPhysPartitioningFunction *logPhys = (LogPhysPartitioningFunction *)
                           childPartFunc->
                              castToLogPhysPartitioningFunction();

   if (logPhys != NULL)
   {
      childPartFunc = logPhys->getPhysPartitioningFunction();
   }
       
   if (parentPartFunc->isAGroupingOf(*childPartFunc, &myPartsPerGroup) 
       //AND !isParentSinglePF
      )
    {
      parentGroupsChildren = TRUE; 
    }
    else if (childPartFunc->isAGroupingOf(*parentPartFunc, &myPartsPerGroup))
    {
      childGroupsParent = TRUE;
    }
   
    partsPerGroup=myPartsPerGroup; 
    return ( (childGroupsParent OR parentGroupsChildren)
             AND partsPerGroup >= 1); 
}

void CostMethodExchange::categorizeMessagesForDP2(
                             const PartitioningFunction* parentPartFunc,
                             const PartitioningFunction* childPartFunc,
                             const CostScalar &downMessages,
                             const CostScalar & upMessages,
                             CostScalar & downIntraCpuMessages,
                             CostScalar & downIntraSegmentMessages,
                             CostScalar & downRemoteSegmentMessages,
                             CostScalar & upIntraCpuMessages,
                             CostScalar & upIntraSegmentMessages,
                             CostScalar & upRemoteSegmentMessages) const

{
  // are there multiple segments? (or clusters in NodeMap notation)
  const NodeMap* childNodeMap  = childPartFunc->getNodeMap();
  const NodeMap * parentNodeMap = parentPartFunc->getNodeMap();

  NABoolean areThereMultiSegments = childNodeMap->isMultiCluster
                                    (0,childNodeMap->getNumEntries(), TRUE);
  if (!areThereMultiSegments)
     areThereMultiSegments = parentNodeMap->isMultiCluster
                                    (0,parentNodeMap->getNumEntries(), TRUE);

  // number of producers, number of consumers
  // can't use nodemaps here
  const CostScalar& numOfConsumers  = ((NodeMap *)(parentPartFunc->getNodeMap())
                                      )->getNumActivePartitions();
  const CostScalar& numOfPartitions = ((NodeMap *)(childPartFunc->getNodeMap()))
                                      ->getEstNumActivePartitionsAtRuntime();

  const CostScalar& numOfProducers  = MINOF( numOfPartitions,
                                         sppForChild_->getCurrentCountOfCPUs());

  const LogPhysPartitioningFunction* childLppf =
      childPartFunc->castToLogPhysPartitioningFunction();

  CostScalar intraSegmentFactor = CostScalar(15)/CostScalar(16);

  CostScalar maxDegree = MAXOF(numOfProducers, numOfConsumers);

  NABoolean isPAPartGroup = 
       (childLppf != NULL
         && (childLppf->getLogPartType() ==
               LogPhysPartitioningFunction::PA_PARTITION_GROUPING));

  NABoolean parentGroupsChildren = FALSE;
  CostScalar paPartsPerGroup;

  if ( (numOfConsumers == csOne) && 
       NOT areThereMultiSegments && 
       (numOfProducers >= csOne) )
     // case 1: single segment: serial plan; only dp2 parallelism
  {
     downIntraCpuMessages=downMessages/numOfProducers;
     downIntraSegmentMessages= downMessages - downIntraCpuMessages;
     downRemoteSegmentMessages=csZero;
     upRemoteSegmentMessages=csZero;
     upIntraCpuMessages=upMessages/numOfProducers;
     upIntraSegmentMessages=upMessages-upIntraCpuMessages;
  }
  else if ( (numOfConsumers == csOne) && 
             areThereMultiSegments
          )
  {
    // case 2: serial plan, multi-segment; same as Case 1
     CostScalar numChildSegments = getNumberofSegments(childNodeMap);
     downIntraCpuMessages=downMessages/numOfProducers;
     downIntraSegmentMessages=downMessages/numChildSegments -
                              downIntraCpuMessages;

     downRemoteSegmentMessages=downMessages - 
                               downIntraSegmentMessages-
                               downIntraCpuMessages;

     upIntraCpuMessages=upMessages/numOfProducers;
     upIntraSegmentMessages=upMessages/numChildSegments -
                            upIntraCpuMessages;
     upRemoteSegmentMessages=upMessages-upIntraSegmentMessages -
                            upIntraCpuMessages;

  }
  else if ((  areThereMultiSegments &&
              parentPartFunc->isAReplicateNoBroadcastPartitioningFunction()
            ) ||
            ( !areThereMultiSegments &&
              parentPartFunc->isAReplicateNoBroadcastPartitioningFunction()
            )
          )
  // case 3: type 2 nested join: similar to ESP communication pattern
  // assumption: number of producers, consumers is a power of 2.
  // IntraNode % (1/16) * interSegmentWeight
  // InterSegment % (15/16) * interSegmentWeight  
  {
     CostScalar localMessageWeight = computeLocalMessageWeight(childNodeMap,
                                                             parentNodeMap); 
     CostScalar intraNodeWeightFactor = csOne/maxDegree;

     if (areThereMultiSegments)
     {
      
        downIntraCpuMessages= downMessages * localMessageWeight *
                               (csOne - intraSegmentFactor);

        downIntraSegmentMessages= downMessages * localMessageWeight * 
                                  intraSegmentFactor;

        downRemoteSegmentMessages= downMessages-downIntraSegmentMessages -
                                   downIntraCpuMessages;

        upIntraCpuMessages= upMessages * localMessageWeight * 
                            (csOne - intraSegmentFactor);

        upIntraSegmentMessages=upMessages* localMessageWeight *
                               intraSegmentFactor;

        upRemoteSegmentMessages=upMessages - upIntraSegmentMessages -
                                upIntraCpuMessages;
     }
     else
     {
       // this is ok even if have 16 x 1; in that case down messages
       // could be one. We are charging a fraction to intracpu and another
       // fraction to intrasegment. But in fact there is just one message
       // which is either intracpu or intrasegment, but not both.
       upRemoteSegmentMessages=csZero;
       downRemoteSegmentMessages=csZero;
       downIntraCpuMessages=downMessages / maxDegree;
       downIntraSegmentMessages=downMessages - downIntraCpuMessages;
       upIntraCpuMessages=upMessages /maxDegree;
       upIntraSegmentMessages=upMessages - upIntraCpuMessages;
       
     }
  }
  else if ( areThereMultiSegments &&
            numOfConsumers <= numOfProducers &&
            isGroupedRepartitioning(childPartFunc,
                                    parentPartFunc,
                                    parentGroupsChildren,
                                    paPartsPerGroup) 
          )
  {
  // remote messages decrease as we increase degree of parallelism:
  // 1 x 128: all (almost all) are remote
  // 128 x 128: all are local, that is, within segment

    CostScalar intraCpuMessageWeight=csOne/paPartsPerGroup;
    CostScalar remoteSegmentWeight, intraSegmentWeight;

    if (paPartsPerGroup <= 16)
    {
       remoteSegmentWeight = csZero; 
       intraSegmentWeight = csOne - intraCpuMessageWeight;
    }
    else
    {
       remoteSegmentWeight = (paPartsPerGroup - 16)/paPartsPerGroup;
       intraSegmentWeight = CostScalar(15) / paPartsPerGroup;
    }

    downIntraCpuMessages=downMessages * intraCpuMessageWeight;    
    downIntraSegmentMessages= downMessages * intraSegmentWeight;
                              
    downRemoteSegmentMessages=downMessages * remoteSegmentWeight;

    upIntraCpuMessages=upMessages*intraCpuMessageWeight;
    upIntraSegmentMessages=upMessages * intraSegmentWeight; 
    upRemoteSegmentMessages=upMessages * remoteSegmentWeight;
  } 
  else if ( !areThereMultiSegments &&
            (numOfConsumers <= numOfProducers) &&
             isGroupedRepartitioning(childPartFunc,
                        parentPartFunc,
                        parentGroupsChildren,
                        paPartsPerGroup)
          ) 
       // case 6:  single segment; number of consumers > 1.
       // 2 x 16, 4x16, 8 x 16, 16x16 etc.
  {
   
    CostScalar intraCpuMessageWeight=csOne/paPartsPerGroup; 
    downIntraCpuMessages=downMessages * intraCpuMessageWeight;
    downIntraSegmentMessages=downMessages - downIntraCpuMessages; 
    downRemoteSegmentMessages=csZero;
    upIntraCpuMessages=upMessages * intraCpuMessageWeight;
    upIntraSegmentMessages=upMessages-upIntraCpuMessages;
    upRemoteSegmentMessages= csZero; 
  }
  else
  {
    // default: when do we end up here? Sometimes isGroupingOf may not
    // work
     CostScalar intraSegmentWeight = 
                 computeLocalMessageWeight(childNodeMap,parentNodeMap); 

    downIntraCpuMessages= downMessages * intraSegmentWeight * 
                          (csOne-intraSegmentFactor); 

    downIntraSegmentMessages=downMessages * intraSegmentWeight * 
                              intraSegmentFactor ;

    downRemoteSegmentMessages=downMessages - downIntraSegmentMessages
                             - downIntraCpuMessages;

    upIntraCpuMessages=upMessages * intraSegmentWeight * 
                       (csOne-intraSegmentFactor);
    upIntraSegmentMessages=upMessages * intraSegmentWeight * 
                           intraSegmentFactor;

    upRemoteSegmentMessages=upMessages - upIntraCpuMessages -
                             upIntraSegmentMessages;
  }
   DCMPASSERT(!upRemoteSegmentMessages.isLessThanZero());
   DCMPASSERT(!downRemoteSegmentMessages.isLessThanZero());
} // CostMethodExchange::categorizeMessagesForDP2()

//****************************************************************************
//CostMethodExchange::getNumberofSegments()
//****************************************************************************
CostScalar CostMethodExchange::getNumberofSegments(const NodeMap* childNodeMap)
const
{
   return CostScalar(childNodeMap->getNumEntries()/16).minCsOne();   
}// CostMethodExchange::getNumberofSegments()

//**************************************************************************
//CostScalar CostMethodExchange::computeLocalMessageWeight()
//**************************************************************************
CostScalar CostMethodExchange::computeLocalMessageWeight(
                                         const NodeMap *childNodeMap, 
                                         const NodeMap *parentNodeMap) const

{
   CostScalar numSegments;
   if (childNodeMap->getNumEntries() > parentNodeMap->getNumEntries())
   {
      numSegments = getNumberofSegments(childNodeMap);
   }
   else
   {
      numSegments = getNumberofSegments(parentNodeMap);
   }

   CostScalar localMessageWeight = csOne/numSegments;
   return localMessageWeight;
} // CostMethodExchange::computeLocalMessageWeight()

//**************************************************************************
// CostMethodExchange::categorizeMessagesForESP()
//**************************************************************************
void CostMethodExchange::categorizeMessagesForESP(
                              const PartitioningFunction* parentPartFunc,
                              const PartitioningFunction* childPartFunc,
                              const CostScalar &downMessages,
                              const CostScalar & upMessages,
                              CostScalar & downIntraCpuMessages,
                              CostScalar & downIntraSegmentMessages,
                              CostScalar & downRemoteSegmentMessages,
                              CostScalar & upIntraCpuMessages,
                              CostScalar & upIntraSegmentMessages,
                              CostScalar & upRemoteSegmentMessages) const
{

  // check if grouped repartitioning is being attempted
  // partitioning keys, types are same
  // number of producers/ number of consumers is a power of 2 or
  // number of consumers/number of producers is a power of 2
  // if that is case, no remote messages

  // downward messages get sent every where for an ESP exchange: each lower 
  // layer ESP process receives same number of requests


  const NodeMap* childNodeMap  = childPartFunc->getNodeMap();
  NABoolean multiSegmentsChild = childNodeMap->isMultiCluster
                                  (0,childNodeMap->getNumEntries(), TRUE);

  const NodeMap* parentNodeMap = parentPartFunc->getNodeMap();
  NABoolean multiSegmentsParent = parentNodeMap->isMultiCluster
                                   (0, parentNodeMap->getNumEntries(), TRUE);
  NABoolean areThereMultiSegments = (multiSegmentsParent || multiSegmentsChild);

  // compute local, remote message weight if necessary
  CostScalar intraSegmentWeight;

  CostScalar intraSegmentFactor = CostScalar(15)/CostScalar(16);

  if (areThereMultiSegments)
  {
     intraSegmentWeight = computeLocalMessageWeight(childNodeMap, 
                                                    parentNodeMap);
  }

  NABoolean parentGroupsChildren;
  CostScalar paPartsPerGroup;
  NABoolean isPAGrouping = isGroupedRepartitioning(
                               childPartFunc, parentPartFunc,
                               parentGroupsChildren,
                               paPartsPerGroup);

  // we are not interested in groups of 1 for ESP Exchange
  if (paPartsPerGroup == csOne)
     isPAGrouping = FALSE;

  if ( areThereMultiSegments  &&
       !isPAGrouping
     )
  {
    downIntraCpuMessages=downMessages * intraSegmentWeight * 
                         (csOne-intraSegmentFactor);

    downIntraSegmentMessages=downMessages * intraSegmentWeight* 
                             intraSegmentFactor;

    downRemoteSegmentMessages=downMessages-downIntraSegmentMessages -
                              downIntraCpuMessages;
 
    upIntraCpuMessages = upMessages * intraSegmentWeight * 
                          (csOne-intraSegmentFactor);

    upIntraSegmentMessages= upMessages * intraSegmentWeight * intraSegmentFactor;
    upRemoteSegmentMessages= upMessages-
                             upIntraCpuMessages-upIntraSegmentMessages;
  }
  else if (!areThereMultiSegments && !isPAGrouping)
  {
    const CostScalar& numOfConsumers  = ((NodeMap *)
                                          (parentPartFunc->getNodeMap())
                                            )->getNumActivePartitions();

    const CostScalar& numOfPartitions = ((NodeMap *)
                                           (childPartFunc->getNodeMap()))
                                        ->getNumActivePartitions();

    const CostScalar& numOfProducers  = MINOF( numOfPartitions,
                                         sppForChild_->getCurrentCountOfCPUs());
    CostScalar outDegree = MAXOF(numOfConsumers, numOfProducers);


    upRemoteSegmentMessages=downRemoteSegmentMessages=csZero;
    upIntraCpuMessages =upMessages/outDegree;

    upIntraSegmentMessages=upMessages -upIntraSegmentMessages;
    downIntraCpuMessages = downMessages/outDegree  ;
    downIntraSegmentMessages=downMessages - downIntraCpuMessages;
  }
  else // PAGrouping...
  {
    CostScalar intraCpuMessageWeight=csOne/paPartsPerGroup;
    CostScalar remoteSegmentWeight, intraSegmentWeight;

    if (paPartsPerGroup <= 16)
    {
      remoteSegmentWeight = csZero;
      intraSegmentWeight = csOne - intraCpuMessageWeight;
    }
    else
    {
      remoteSegmentWeight = (paPartsPerGroup - 16)/paPartsPerGroup;
      intraSegmentWeight = CostScalar(15) / paPartsPerGroup;
    }
    downIntraSegmentMessages=downMessages *intraSegmentWeight;
    downIntraCpuMessages = downMessages * intraCpuMessageWeight;
    downRemoteSegmentMessages=downMessages * remoteSegmentWeight;
    upIntraCpuMessages = upMessages * intraCpuMessageWeight;
    upIntraSegmentMessages=upMessages * intraSegmentWeight;
    upRemoteSegmentMessages= upMessages * remoteSegmentWeight;
  }

} // CostMethodExchange::categorizeMessagesForESP()
//<pb>
//==============================================================================
//  Determine how many of the specified "down" and "up" messages are internode
// (i.e. cross node boundaries) and how many are intranode (i.e. don't cross
// node boundaries.
//
// Input:
//  parentPartFunc        -- pointer to parent's partitioning function.
//
//  childPartFunc         -- pointer to child's partitioning function.
//
//  executeInDP2          -- TRUE if child executes in DP2; FALSE otherwise.
//
//  downMessages          -- number of messages sent from parent down to child.
//
//  upMessages            -- number of messages sent from child up to parent.
//
// Output:
//  downIntraNodeMessages -- number of down messages which do not cross node
//                            boundaries.
//
//  downInterNodeMessages -- number of down messages which cross node
//                            boundaries.
//
//  downRemoteNodeMessages -- number of down messages which cross system boundary
//
//  upIntraNodeMessages   -- number of up messages which do not cross node
//                            boundaries.
//
//  upInterNodeMessages   -- number of up messages which cross node boundaries.
//
//  upRemoteNodeMessages  -- number of up messages which cross system boundaries
// Return:
//  none
//
//==============================================================================
void
CostMethodExchange::categorizeMessages(
                         const PartitioningFunction* parentPartFunc,
                         const PartitioningFunction* childPartFunc,
                         const NABoolean             executeInDP2,
                         const CostScalar &          downMessages,
                         const CostScalar &          upMessages,
                               CostScalar&           downIntraNodeMessages,
                               CostScalar&           downInterNodeMessages,
                               CostScalar&           downRemoteNodeMessages,
                               CostScalar&           upIntraNodeMessages,
                               CostScalar&           upInterNodeMessages,
                               CostScalar&           upRemoteNodeMessages) const
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( parentPartFunc != NULL );
  CMPASSERT( childPartFunc  != NULL );

  //----------------------------------------------------------------
  //  Extract node maps from parent and child partitioning functions
  // respectively.  Ensure both node maps exist and have the same
  // number of entries as their respective partitioning functions.
  //----------------------------------------------------------------
  const NodeMap* parentNodeMap = parentPartFunc->getNodeMap();
  const NodeMap* childNodeMap  = childPartFunc->getNodeMap();
  CMPASSERT(   parentNodeMap != NULL
            && parentPartFunc->getCountOfPartitions()
                                        == (Lng32) parentNodeMap->getNumEntries()
            && childNodeMap != NULL);

  //-------------------------------------------------------------------------
  //  Get addressability to the defaults table.
  //-------------------------------------------------------------------------
  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  //-------------------------------------------------------------------------
  // If faked hardware, then set the number of nodes per cluster based on a
  // CQD, otherwise get the number of nodes per cluster from gpClusterInfo.
  //-------------------------------------------------------------------------

  CollIndex numOfNodesInActiveClusters =
      ( CURRSTMT_OPTDEFAULTS->isFakeHardware() ?
            defs.getAsLong(DEF_NUM_NODES_IN_ACTIVE_CLUSTERS)
          : gpClusterInfo->numOfSMPs()
      );

  //---------------------------------------------------------------------------
  // The rest of this function may either assume either that grouping is being
  // done or that a broadcast is being done. If a grouping is being done, then
  // nodeMaps may be used to categorize the types of messages. Broadcasts
  // assume that messages may be sent to all nodes and use simple calculations
  // based on the system configuration for categorizing the messages.
  // Alternatively, if COMP_BOOL_60 is on, then the old behavior of setting
  // useNodeMaps based on the "executeInDp2" check will be used.
  //
  // NOTE: For this build of CQD4, COMP_BOOL_59 must be turned on to to
  // use some new code that determines whether grouping is being done.  If
  // testing shows better plans, then the code that checks COMP_BOOL_59 will
  // be modified.  If this code is improved to an acceptible point in the
  // future, both of these CQDs may be recycled.
  //
  // There are a few times when grouping is evident.  First, when the child
  // partitioning function is a LogPhysPartitioningFunction and the logPartType
  // is PA_PARTITION_GROUPING or LOGICAL_SUBPARTITIONING.  The next is when
  // the parent is a grouping of the child.  The final case is a reverse
  // grouping where the data is being repartitioned from fewer ESPs to more
  // ESPs and the child is a logical grouping of the parent. In this case,
  // the roles of parent and child are reversed in order to categorize the
  // messages in an easy way.
  //---------------------------------------------------------------------------

 NABoolean useNodeMaps = FALSE;
 if (CmpCommon::getDefault(COMP_BOOL_60) == DF_ON)
 {
    if (executeInDP2)
      categorizeMessagesForDP2(parentPartFunc,
                               childPartFunc,
                               downMessages,
                               upMessages,
                               downIntraNodeMessages,
                               downInterNodeMessages,
                               downRemoteNodeMessages,
                               upIntraNodeMessages,
                               upInterNodeMessages,
                               upRemoteNodeMessages) ;
    else
      categorizeMessagesForESP(parentPartFunc,
                              childPartFunc,
                              downMessages,
                              upMessages,
                              downIntraNodeMessages,
                              downInterNodeMessages,
                              downRemoteNodeMessages,
                              upIntraNodeMessages,
                              upInterNodeMessages,
                              upRemoteNodeMessages) ;
    return;
 }

// The following code is retained for history reason. The new logic 
// (when CB_60 on) is fully tested and will be used all the time. 

  else if (CmpCommon::getDefault(COMP_BOOL_59) == DF_OFF)
  {
    useNodeMaps = TRUE;
  }
  else
  {
    const LogPhysPartitioningFunction* childLppf =
      childPartFunc->castToLogPhysPartitioningFunction();

    if (childLppf != NULL
        && (childLppf->getLogPartType() ==
                 LogPhysPartitioningFunction::PA_PARTITION_GROUPING
            || childLppf->getLogPartType() ==
                 LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING))
    {
      useNodeMaps = TRUE;
    }
    else if (parentPartFunc->isAGroupingOf(*childPartFunc))
    {
      useNodeMaps = TRUE;
    }
    else if (childPartFunc->isAGroupingOf(*parentPartFunc))
    {
      // This involves repartitioning the data where the child is a grouping
      // of the parent.  NodeMaps can be used here, but the logic in this
      // function must reverse the roles of the parent and child.
      parentNodeMap = childPartFunc->getNodeMap();
      childNodeMap  = parentPartFunc->getNodeMap();
      useNodeMaps = TRUE;
    }
  }

  if (useNodeMaps && parentNodeMap->allNodesSpecified()
      && childNodeMap->allNodesSpecified()
      && NOT CURRSTMT_OPTDEFAULTS->isFakeHardware())
    {


      NABoolean fakeEnv = FALSE;
      CollIndex totalEsps = defs.getTotalNumOfESPsInCluster(fakeEnv);
      CMPASSERT(parentNodeMap->getNumEntries() <= totalEsps ); //here it needs to be all clusters

      //--------------------------------------------------------------------       
      // Derive the implicit grouping for the parent node map.
      //--------------------------------------------------------------------
      CollIndexPointer groupStart;
      CollIndexPointer groupSize;
      ((NodeMap*) childNodeMap)->deriveGrouping(parentNodeMap->getNumEntries(),
                                                groupStart,
                                                groupSize);

      //----------------------------------------------------------------
      //  Using the derived grouping, we can now determine the number of
      // internode and intranode communication links.
      //----------------------------------------------------------------
      CostScalar interNode = csZero;
      CostScalar intraNode = csZero;
      CostScalar remoteNode = csZero;
      for (CollIndex parentIdx = 0;
                     parentIdx < parentNodeMap->getNumEntries();
                     parentIdx++)
        {

          //------------------------------------------------------------
          //  Only count messages that pertain to active parent entries.
          //------------------------------------------------------------
          if ( parentNodeMap->isActive(parentIdx) )
            {
              for (CollIndex childIdx = groupStart[parentIdx];
                             childIdx <   groupStart[parentIdx]
                                        + groupSize[parentIdx];
                             childIdx++)
                {

                  //-----------------------------------------------------------
                  //  Only count messages that pertain to active child entries.
                  //-----------------------------------------------------------
                  if ( childNodeMap->isActive(childIdx) )
                    {

                      //------------------------------------------------------
                      //  If parent and child entries are on the same system, we
                      // have an intrasystem communication link.  Otherwise we
                      // have an  intersytem communication link.
                      //------------------------------------------------------
                      if(parentNodeMap->getClusterNumber(parentIdx)
                                      != childNodeMap->getClusterNumber(childIdx)  )
                      {
                        remoteNode++;
                      }
                      //------------------------------------------------------
                      //  If parent and child entries are on the same node, we
                      // have an intranode communication link.  Otherwise we
                      // have an internode communication link.
                      //------------------------------------------------------

                      else if (parentNodeMap->getNodeNumber(parentIdx)
                                      == childNodeMap->getNodeNumber(childIdx) )
                        {
                          intraNode++;
                        }
                      else
                        {
                          interNode++;
                        }
                    }
                }
            }
        }

      //---------------------------------------------------------------------
      //  Given the number of intersystem ,internode and intranode communication links,
      // we can calculate the percentage of internode and intranode messages.
      //---------------------------------------------------------------------
      CostScalar interNodePercentage;
      CostScalar intraNodePercentage;
      CostScalar remoteNodePercentage;
      if ((interNode + intraNode + remoteNode).isGreaterThanZero() /*>csZero*/)
        {
          remoteNodePercentage = remoteNode / (remoteNode+interNode+intraNode);
          interNodePercentage = interNode / (remoteNode + interNode + intraNode);
          intraNodePercentage = csOne - (interNodePercentage+remoteNodePercentage);
        }
      else
        {
          remoteNodePercentage = csZero;
          interNodePercentage = csZero;
          intraNodePercentage = csZero;
        }

      //-------------------------------------------------------------------
      //  Calcualte the number of internode and intranode messages based on
      // the internode and intranode percentages calculated above.
      //-------------------------------------------------------------------
      upRemoteNodeMessages  = upMessages   * remoteNodePercentage;
      upInterNodeMessages   = upMessages   * interNodePercentage;
      upIntraNodeMessages   = upMessages   * intraNodePercentage;
      downRemoteNodeMessages= downMessages * remoteNodePercentage;
      downInterNodeMessages = downMessages * interNodePercentage;
      downIntraNodeMessages = downMessages * intraNodePercentage;

      return;

    }

  //----------------------------------------------------------------------
  // The following code fragment is active if fake hardware is in use or if 
  // all nodes and clusters are not specified in the parent and child
  // node maps. 
  //  Estimate number of active nodes in cluster as the maximum of the
  // number of active child nodes and the number of active parent nodes.
  // Of course, we can't have more active nodes than actually exist in the
  // active clusters.
  //----------------------------------------------------------------------
  const CollIndex activeChildNodes  =
	  ((NodeMap *)childNodeMap)->getEstNumActivePartitionsAtRuntime();
  const CollIndex activeParentNodes =
	  ((NodeMap *)parentNodeMap)->getNumActivePartitions();
  const CostScalar & activeNodesInClusters =
  MINOF( MAXOF( activeChildNodes, activeParentNodes ), numOfNodesInActiveClusters );
  CostScalar downInterIntraNodeMessages = csZero;
  CostScalar upInterIntraNodeMessages = csZero;
  CostScalar activeClusterInNetwork = gpClusterInfo->getNumActiveCluster();
  if(CURRSTMT_OPTDEFAULTS->isFakeHardware())
	  activeClusterInNetwork = csOne;
  //-------------------------------------------------------------------------
  //  Assume all messages are uniformly distributed among all active nodes in
  // the cluster.
  //-------------------------------------------------------------------------
  downInterIntraNodeMessages= downMessages/ activeClusterInNetwork;
  downRemoteNodeMessages = downMessages - downInterIntraNodeMessages;
  downIntraNodeMessages = downInterIntraNodeMessages / (activeNodesInClusters/activeClusterInNetwork);
  downInterNodeMessages = downInterIntraNodeMessages - downIntraNodeMessages;
  upInterIntraNodeMessages = upMessages/activeClusterInNetwork;
  upRemoteNodeMessages = upMessages - upInterIntraNodeMessages;
  upIntraNodeMessages   = upInterIntraNodeMessages / (activeNodesInClusters/activeClusterInNetwork);
  upInterNodeMessages   = upInterIntraNodeMessages - upIntraNodeMessages;

} // CostMethodExchange::categorizeMessages()
//<pb>
//==============================================================================
//  Produce cost vectors representing resources used by parent and child of
// Exchange operator to produce their first and last rows.
//
// Input:
//  numOfProbes               -- number of requests sent from parent to child.
//
//  numOfConsumers            -- number of parent processes receiving up
//                                messages and sending down messages.
//
//  numOfProducers            -- number of child processes receiving down
//                                messages and sending up messages.
//
//  executeInDP2              -- TRUE if child executes in DP2; FALSE otherwise.
//
//  myPartFunc                -- pointer to my partitioning function.
//
//  childPartFunc             -- pointer to child's partitioning function.
//
//  messageSpacePerRecordInKb -- size of a record (including any header
//                                overhead) in KB.
//
//  messageHeaderInKb         -- size of message buffer header in KB.
//
//  messageBufferSizeInKb     -- size of entire message buffer in KB.
//
//  downIntraNodeMessages     -- number of down messages which do not cross node
//                                boundaries.
//
//  downInterNodeMessages     -- number of down messages which cross node
//                                boundaries.
//
//  downRemoteNodeMessages    -- number of down messages which cross system
//.                              messages
//  upIntraNodeMessages       -- number of up messages which do not cross node
//                                boundaries.
//
//  upInterNodeMessages       -- number of up messages which cross node
//                                boundaries.
//  upRemoteNodeMessages      -- number of up messages which cross system boundaries
//
// Output:
//  parentFR                  -- resources used by parent to produce first row.
//
//  parentFR                  -- resources used by parent to produce last row.
//
//  childFR                   -- resources used by child to produce first row.
//
//  childFR                   -- resources used by child to produce last row.
//
// Return:
//  none
//
//==============================================================================

void
CostMethodExchange::produceCostVectors(
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
                          const CostScalar &           downIntraNodeMessages,
                          const CostScalar &           downInterNodeMessages,
                          const CostScalar &           downRemoteNodeMessages,
                          const CostScalar &           upIntraNodeMessages,
                          const CostScalar &           upInterNodeMessages,
                          const CostScalar &           upRemoteNodeMessages,
                                CostVecPtr&           parentFR,
                                CostVecPtr&           parentLR,
                                CostVecPtr&           childFR,
                                CostVecPtr&           childLR) const
{

  if (CmpCommon::getDefault(COMP_BOOL_60) == DF_ON)
  {
     return produceCostVectorsWithControlDataMessages(
                             numOfProbes,
                             numOfConsumers,
                             numOfProducers,
                             childExecutesInDP2,
                             myPartFunc,
                             childPartFunc,
                             messageSpacePerRecordInKb,
                             messageHeaderInKb,
                             messageBufferSizeInKb,
                             upRowsPerConsumer_,
                             downIntraNodeMessages,
                             downInterNodeMessages,
                             downRemoteNodeMessages,
                             upIntraNodeMessages,
                             upInterNodeMessages,
                             upRemoteNodeMessages,
                             parentFR,
                             parentLR,
                             childFR,
                             childLR);
  } 

  //-------------------------------------------------------------------------
  //  Calculate CPU cost of copying a byte for an Exchange and the additional
  // cost of copying that byte across a node or system boundary.
  //-------------------------------------------------------------------------
  const CostScalar instrPerByte =
             CostPrimitives::getBasicCostFactor(CPUCOST_COPY_ROW_PER_BYTE)
           + CostPrimitives::getBasicCostFactor(CPUCOST_EXCHANGE_COST_PER_BYTE);

  const CostScalar interNodeInstrPerByte =
   CostPrimitives::getBasicCostFactor(CPUCOST_EXCHANGE_INTERNODE_COST_PER_BYTE);
  const CostScalar remoteNodeInstrPerByte =
   CostPrimitives::getBasicCostFactor(CPUCOST_EXCHANGE_REMOTENODE_COST_PER_BYTE);

  //---------------------------------------------------------------------------
  //  Calculate CPU cost of copying an entire message buffer and the additional
  // cost of copying that buffer across a node or system boundary.
  //---------------------------------------------------------------------------
  const CostScalar instrToCopyAMessage =
    ( instrPerByte * messageBufferSizeInKb * csOneKiloBytes ).getCeiling();
  const CostScalar interNodeInstrToCopyAMessage =
    ( interNodeInstrPerByte * messageBufferSizeInKb * csOneKiloBytes ).getCeiling();
  const CostScalar remoteNodeInstrToCopyAMessage =
    ( remoteNodeInstrPerByte * messageBufferSizeInKb * csOneKiloBytes ).getCeiling();

  //-----------------------------------------------------------------
  //  There are always two copies made to transfer data from DP2 root
  // to the master:
  //
  // 1.- From DP2InExe buffer to set up message buffer
  //     (done by root in dp2)
  // 2.- From the messaging system to the memory space of the
  //    master
  //
  // ESP to ESP communication works in the same way (for now)
  //-----------------------------------------------------------------
  const CostScalar senderIntraNodeCopies     = csOne;
  const CostScalar receiverIntraNodeCopies   = csOne;
  const CostScalar senderInterNodeCopies     = csOne;
  const CostScalar receiverInterNodeCopies   = csOne;
  const CostScalar senderRemoteNodeCopies    = csOne;
  const CostScalar receiverRemoteNodeCopies  = csOne;
  const CostScalar intraNodeCopiesPerMessage =
                                senderIntraNodeCopies + receiverIntraNodeCopies;
  const CostScalar interNodeCopiesPerMessage =
                                senderInterNodeCopies + receiverInterNodeCopies;
  const CostScalar remoteNodeCopiesPerMessage=
                                senderRemoteNodeCopies+ receiverRemoteNodeCopies;

  //-------------------------------------------------------------
  //  Distribute the load of messages.
  //  All messages affect the CPU component.
  //  Only internode messages affect the LOCAL message component.
  //  There are no intercluster (i.e. REMOTE) messages on NT.
  //  Note: Intra-node (i.e. intra-cpu on NSK) messages are 
  //  ignored when computing the LOCAL message component. Such
  //  messages merely involve a memory-to-memory copy.- Sunil
  //--------------------------------------------------------

  //------------------------------------------------------------------
  //  Compute number of copies for intra node, internode and remote
  // messages from the parent perspective.
  //------------------------------------------------------------------
  CostScalar parentIntraNodeCopies =
          (downIntraNodeMessages * senderIntraNodeCopies)
        + (upIntraNodeMessages   * receiverIntraNodeCopies);

  CostScalar parentInterNodeCopies =
          (downInterNodeMessages * senderInterNodeCopies)
        + (upInterNodeMessages   * receiverInterNodeCopies);
  CostScalar parentRemoteNodeCopies =
          (downRemoteNodeMessages * senderRemoteNodeCopies)
        + (upRemoteNodeMessages   * receiverRemoteNodeCopies);

  // Divide total IntraNode messages by number of intra nodes.
  // Divide total InterNode messages by number of inter nodes.
  // Divide total RemoteNode messages by number of remote nodes.
  CostScalar intraNode = csZero;
  CostScalar interNode = csZero;
  CostScalar remoteNode = csZero;
  CostScalar downMessages =  downIntraNodeMessages + downInterNodeMessages
                           + downRemoteNodeMessages;
  if ( CmpCommon::getDefault(COMP_BOOL_97) == DF_OFF )
  {
    intraNode = (downIntraNodeMessages/downMessages) * numOfConsumers;
    interNode = (downInterNodeMessages/downMessages) * numOfConsumers;
    remoteNode = (downRemoteNodeMessages/downMessages) * numOfConsumers;
  }
  else
  {
    intraNode = interNode = remoteNode = numOfConsumers;
  }

  //----------------------------------------------------------------
  //  We have calculated the total number of messages, now normalize
  // these to the number of parent's CPUs (numOfConsumers).
  // Note: What if we have 2 consuming ESPs per CPU (i.e. MAX_ESPS_PER_CPU_PER_OP=2)?
  // If so, then numOfConsumers <> no. of CPUs.-- Sunil
  //----------------------------------------------------------------
  CostScalar parentIntraNodeCopiesPerCPU   =   parentIntraNodeCopies
                                             / MIN_ONE_CS(intraNode);
  CostScalar parentInterNodeCopiesPerCPU   =   parentInterNodeCopies
                                             / MIN_ONE_CS(interNode);
  CostScalar parentRemoteNodeCopiesPerCPU  =   parentRemoteNodeCopies
                                             / MIN_ONE_CS(remoteNode);
  CostScalar downInterNodeMessagesPerCPU   =   downInterNodeMessages
                                             / MIN_ONE_CS(interNode);
  CostScalar downRemoteNodeMessagesPerCPU   =   downRemoteNodeMessages
                                             / MIN_ONE_CS(remoteNode);

  //-------------------------------------------------------
  //  Calculate number of bytes of down internode and intersystem messages.
  //-------------------------------------------------------
  CostScalar parentInterNodeMessagesInKbPerCPU =  downInterNodeMessagesPerCPU
                                                * messageBufferSizeInKb;
  CostScalar parentRemoteNodeMessagesInKbPerCPU=  downRemoteNodeMessagesPerCPU
                                                * messageBufferSizeInKb;

  //----------------------------------------------------------------
  //  Calculate number of total message copies and internode message
  // copies for producing the first row.
  //----------------------------------------------------------------
  const CostScalar & firstRowParentIntraNodeCopiesPerCPU =
    MINOF( parentIntraNodeCopiesPerCPU, intraNodeCopiesPerMessage );

  const CostScalar & firstRowParentInterNodeCopiesPerCPU =
    MINOF( parentInterNodeCopiesPerCPU, interNodeCopiesPerMessage );
  const CostScalar & firstRowParentRemoteNodeCopiesPerCPU =
                                              MINOF(parentRemoteNodeCopiesPerCPU,
                                                    remoteNodeCopiesPerMessage);
  //----------------------------------------------------------------------------
  //  Calculate memory usage.  We need enough memory for each copy of a message.
  // When the child does not executes in DP2, each producer needs enough memory
  // for each copy of a message.
  //----------------------------------------------------------------------------
  CostScalar parentMemory = messageBufferSizeInKb * (MINOF(parentInterNodeCopiesPerCPU,
                                                    interNodeCopiesPerMessage)+
						     MINOF(parentIntraNodeCopiesPerCPU,
                                                     intraNodeCopiesPerMessage)+ 
                                                    MINOF(parentRemoteNodeCopiesPerCPU,
                                                    remoteNodeCopiesPerMessage));
  if (NOT childExecutesInDP2)
  {
    parentMemory = parentMemory * numOfProducers;
  }

  //-----------------
  // Parent First Row
  //-----------------
  parentFR = new STMTHEAP SimpleCostVector;

  parentFR->setInstrToCPUTime(  firstRowParentIntraNodeCopiesPerCPU
                       * instrToCopyAMessage );
  parentFR->addNumLocalToMSGTime(
                           MINOF( csOne, downInterNodeMessagesPerCPU ) );
  parentFR->addKBLocalToMSGTime(
                           MINOF( messageBufferSizeInKb,
                                  parentInterNodeMessagesInKbPerCPU ) );
  parentFR->addNumRemoteToMSGTime(
                           MINOF(csOne,downRemoteNodeMessagesPerCPU));
  parentFR->addKBRemoteToMSGTime(
                           MINOF(messageBufferSizeInKb,
                                 parentRemoteNodeMessagesInKbPerCPU));
  //parentFR->setNormalMemory(parentMemory);
  parentFR->setNumProbes(numOfProbes);

  //-----------------
  //  Parent Last Row
  //-----------------
  parentLR = new STMTHEAP SimpleCostVector;

  parentLR->setInstrToCPUTime(  (  parentIntraNodeCopiesPerCPU
                          * instrToCopyAMessage)
	               + (  parentInterNodeCopiesPerCPU
                          * interNodeInstrToCopyAMessage)
			+  (  parentRemoteNodeCopiesPerCPU
                          * remoteNodeInstrToCopyAMessage));
  parentLR->addNumLocalToMSGTime( downInterNodeMessagesPerCPU) ;
  parentLR->addKBLocalToMSGTime( parentInterNodeMessagesInKbPerCPU );
  parentLR->addNumRemoteToMSGTime( downRemoteNodeMessagesPerCPU );
  parentLR->addKBRemoteToMSGTime( parentRemoteNodeMessagesInKbPerCPU );
  // parentLR->setNormalMemory( parentMemory );
  parentLR->setNumProbes( numOfProbes );

  // give some weight for parallel plans compared to serial plan when
  // a large number of rows are being returned to application

  NADefaults &defs1 = ActiveSchemaDB()->getDefaults();
  CostScalar adj1 = defs1.getAsLong(COMP_INT_62);
  if (adj1 == csZero)
    adj1=1000000;
   NABoolean adjustmentForParallelPlans = FALSE;
   if ( (numOfConsumers == csOne) AND
        (numOfProducers > numOfConsumers) AND
      isOpBelowRoot_  AND
     upRowsPerConsumer_  > adj1 )
   {
      parentLR->setToValue(0.0001)  ;
      parentFR->setToValue(0.0001)  ;
      adjustmentForParallelPlans = TRUE;
   }

  //------------------------------------------------------------------
  //  Compute number of copies for intra node and for just internode
  // messages from child's perspective.
  //------------------------------------------------------------------
  CostScalar childIntraNodeCopies =
          (downIntraNodeMessages * receiverIntraNodeCopies)
        + (upIntraNodeMessages   * senderIntraNodeCopies);

  CostScalar childInterNodeCopies =
          (downInterNodeMessages * receiverInterNodeCopies)
        + (upInterNodeMessages   * senderInterNodeCopies);
  CostScalar childRemoteNodeCopies =
          (downRemoteNodeMessages* receiverInterNodeCopies)
        + (upRemoteNodeMessages  * senderRemoteNodeCopies);

  // Divide total IntraNode messages by number of intra nodes.
  // Divide total InterNode messages by number of inter nodes.
  // Divide total RemoteNode messages by number of remote nodes.
  CostScalar upMessages =  upIntraNodeMessages + upInterNodeMessages
                         + upRemoteNodeMessages;
  if ( CmpCommon::getDefault(COMP_BOOL_97) == DF_OFF )
  {
    intraNode = (upIntraNodeMessages/upMessages) * numOfProducers;
    interNode = (upInterNodeMessages/upMessages) * numOfProducers;
    remoteNode = (upRemoteNodeMessages/upMessages) * numOfProducers;
  }
  else
  {
    intraNode = interNode = remoteNode = numOfProducers;
  }

  //-------------------------------------------------------------------------
  //  The messages used in calculating childIntraNodeCopiesPerCPU and
  // upInterNodeMessagesPerCPU,upRemoteNodeMessagesPerCPU were based on the
  // MAXOF producers and senders. Normalize this to the number of CPUs in the child.
  //-------------------------------------------------------------------------
  CostScalar childIntraNodeCopiesPerCPU       =   childIntraNodeCopies
                                                / MIN_ONE_CS(intraNode);

  CostScalar childInterNodeCopiesPerCPU       =   childInterNodeCopies
                                                / MIN_ONE_CS(interNode);
  CostScalar childRemoteNodeCopiesPerCPU      =   childRemoteNodeCopies
                                                / MIN_ONE_CS(remoteNode);

  CostScalar upInterNodeMessagesPerCPU        =   upInterNodeMessages
                                                / MIN_ONE_CS(interNode);
  CostScalar upRemoteNodeMessagesPerCPU       =   upRemoteNodeMessages
                                                / MIN_ONE_CS(remoteNode);

  CostScalar childInterNodeMessagesInKbPerCPU =   upInterNodeMessagesPerCPU
                                                * messageBufferSizeInKb;
  CostScalar childRemoteNodeMessagesInKbPerCPU=   upRemoteNodeMessagesPerCPU
                                                * messageBufferSizeInKb;



  const CostScalar & firstRowChildIntraNodeCopiesPerCPU =
    MINOF( childIntraNodeCopiesPerCPU, intraNodeCopiesPerMessage );

  //----------------------------------------------------------------------------
  //  Calculate memory usage.  We need enough memory for each copy of a message.
  // When the child does not execute in DP2, each consumer needs enough memory
  // for each copy of a message.
  //----------------------------------------------------------------------------
  CostScalar  childMemory = messageBufferSizeInKb * (interNodeCopiesPerMessage
                                                     + remoteNodeCopiesPerMessage);
  if ( NOT childExecutesInDP2 )
  {
    childMemory = childMemory * numOfConsumers;
  }

  //------------------------------------------------------------------------------
  // Cost to compute the hash value for a row.
  //
  // I have made 2 changes: The hash value needs to be computed only if range or
  // hash repartitioning is taking place. And this determination needs to be made 
  // based on the Exchange's (and not child's) partitioning function. Note that we
  // ignore the cpu costs associated with round-robin and random repartitioning;
  // these costs are trivial. Range-repartitioning involves evaluating the range
  // partitioning function for each row. This requires a binary search through
  // the partition key arrary, using an encoded key (derived from the row) as the
  // search key. In lieu of a cost estimate for this binary search, we'll use the
  // CPU cost for computing a hash partitioning function. - Sunil
  //-------------------------------------------------------------------------------
  CostScalar cpuCostHashRow = csZero;
  if ( NOT childExecutesInDP2 AND numOfConsumers.isGreaterThanOne() /*> csOne*/ 
       AND ( myPartFunc->isAHashPartitioningFunction() 
             OR myPartFunc->isAHash2PartitioningFunction()
             OR myPartFunc->isAHashDistPartitioningFunction()
	     OR myPartFunc->isARangePartitioningFunction() 
	     OR myPartFunc->isASkewedDataPartitioningFunction() 
             ))
  {
    if ( CmpCommon::getDefault(COMP_BOOL_97) == DF_OFF )
      cpuCostHashRow =
        CostPrimitives::cpuCostForHash( myPartFunc->getPartitioningKey() );
    else
      cpuCostHashRow =
        CostPrimitives::cpuCostForHash( childPartFunc->getPartitioningKey() );
  }

  // if the exchange is PAPA and for Hash2 and Hash1 pfs, we need to
  // cost hashing of incoming probes... 
  if ( (CmpCommon::getDefault(COMP_BOOL_57) == DF_OFF) AND
        childExecutesInDP2 AND 
       (numOfProducers> numOfConsumers) AND
        childPartFunc->isALogPhysPartitioningFunction() AND
       numOfProbes.isGreaterThanOne() 
     )
  {
     PartitioningFunction* phys  = childPartFunc->
                                   castToLogPhysPartitioningFunction()->
                                   getPhysPartitioningFunction();

     // Hash2 and Hash1 partitioning functions have different functions
     // to compute partition numbers and hash1 is more expensive
     // that should be reflected in the cost computation

     if (phys->isAHash2PartitioningFunction() OR
         phys->isAHashDistPartitioningFunction() OR
         phys->isARangePartitioningFunction())
     {
        Exchange *exch= (Exchange *) op_;
        NABoolean areProbesHashed=
            exch->areProbesHashed(childPartFunc->getPartitioningKey());
       if (areProbesHashed)
       {
         CostScalar cpuCostHashProbes =
          CostPrimitives::cpuCostForHash( childPartFunc->getPartitioningKey() );
         parentLR->addInstrToCPUTime( cpuCostHashProbes * 
                                      (numOfProbes/numOfConsumers).minCsOne());

       }
     }
    
  }
  
  if ( (CmpCommon::getDefault(COMP_BOOL_57) == DF_OFF) AND
       isMergeNeeded_ AND
       numOfProducers> numOfConsumers )
  {
     CostScalar cpuCostCompareKeys =
          CostPrimitives::cpuCostForCompare(sppForMe_->getSortKey());
     parentLR->addInstrToCPUTime( cpuCostCompareKeys * 
                                     upRowsPerConsumer_);
  }

  //----------------------------------------------------------------------------
  //  Compute maximum number of rows that can fit in a single message buffer and
  // make this the upper bound for the number of messages associated with the
  // child's first row cost.
  //----------------------------------------------------------------------------
  const CostScalar maxRowsPerMessage =
    ( ( messageBufferSizeInKb - messageHeaderInKb ) / messageSpacePerRecordInKb
    ).getFloor();

  const CostScalar & firstRowNumOfRows =
    MINOF( upRowsPerConsumer, maxRowsPerMessage );

  //----------------
  // Child First Row
  //----------------
  childFR = new STMTHEAP SimpleCostVector;

  childFR->setInstrToCPUTime(  (  firstRowChildIntraNodeCopiesPerCPU
                         * instrToCopyAMessage)
                      + (firstRowNumOfRows * cpuCostHashRow));

  childFR->addNumLocalToMSGTime( MINOF( csOne, upInterNodeMessagesPerCPU ) );

  childFR->addKBLocalToMSGTime(
                            MINOF(messageBufferSizeInKb,
                                  childInterNodeMessagesInKbPerCPU));
  childFR->addNumRemoteToMSGTime(
                            MINOF(csOne,upRemoteNodeMessagesPerCPU));

  childFR->addKBRemoteToMSGTime(
                            MINOF(messageBufferSizeInKb,
                                  childRemoteNodeMessagesInKbPerCPU));

  // childFR->setNormalMemory( childMemory );
  childFR->setNumProbes( numOfProbes );

  //---------------
  // Child Last Row
  //---------------
  childLR = new STMTHEAP SimpleCostVector;

  childLR->setInstrToCPUTime(  (childIntraNodeCopiesPerCPU
                              * instrToCopyAMessage)
	                  + (  childInterNodeCopiesPerCPU
                             * interNodeInstrToCopyAMessage)
                          + (  childRemoteNodeCopiesPerCPU
                             * remoteNodeInstrToCopyAMessage)
                          + (upRowsPerConsumer * cpuCostHashRow));

  childLR->addNumLocalToMSGTime( upInterNodeMessagesPerCPU );
  childLR->addKBLocalToMSGTime( childInterNodeMessagesInKbPerCPU );
  childLR->addNumRemoteToMSGTime( upRemoteNodeMessagesPerCPU );
  childLR->addKBRemoteToMSGTime( childRemoteNodeMessagesInKbPerCPU );
  // childLR->setNormalMemory( childMemory );
  childLR->setNumProbes( numOfProbes );

  if (adjustmentForParallelPlans)
  {
    childFR->scaleByValue(csZero);
    childLR->scaleByValue(csZero);
  }
  //----------------------------------------------------------------
  //  Compute the cost of starting ESPs as an idle time cost.  Store
  // this idle time in both the first row and last row vectors.
  //----------------------------------------------------------------
  //SimpleCostVector* espStartupCost = computeESPCost(NOT executeInDP2,
  //                                                  numOfProbes);
  //CostScalar espElapsedTime =  espStartupCost->getCPUTime();
  //                             + espStartupCost->getIOTime();
  // computeEXPCost has been simplified by removing IO component. Nov.2005

  CostScalar espElapsedTime = computeESPCost(NOT childExecutesInDP2,numOfProbes);
  // ESP startup cost is waited, so must pay the price of ESP startup
  // for each consumer, according to Hans (4/20/99).
  espElapsedTime *= numOfConsumers;
  CostScalar startupAdj = (ActiveSchemaDB()->getDefaults())\
                                  .getAsDouble(COMP_INT_63);
  if( isOpBelowRoot_ && (NOT childExecutesInDP2) && ( numOfProbes < 10 ) && (CmpCommon::getDefault(COMP_BOOL_122) == DF_ON) ){
  parentFR->addToCpuTime( (espElapsedTime * startupAdj) / numOfProbes );
  parentLR->addToCpuTime(  (espElapsedTime * startupAdj) / numOfProbes );
  }

} // CostMethodExchange::produceCostVectors()

void
CostMethodExchange::produceCostVectorsWithControlDataMessages(
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
                                CostVecPtr&           childLR) const
{

  //-------------------------------------------------------------------------
  //  Calculate CPU cost of copying a byte for an Exchange and the additional
  // cost of copying that byte across a node or system boundary.
  //-------------------------------------------------------------------------
  const CostScalar instrPerByte =
             CostPrimitives::getBasicCostFactor(CPUCOST_COPY_ROW_PER_BYTE)
           + CostPrimitives::getBasicCostFactor(CPUCOST_EXCHANGE_COST_PER_BYTE);

  const CostScalar intraSegmentInstrPerByte =
   CostPrimitives::getBasicCostFactor(CPUCOST_EXCHANGE_INTERNODE_COST_PER_BYTE);
  const CostScalar remoteSegmentInstrPerByte =
   CostPrimitives::getBasicCostFactor(CPUCOST_EXCHANGE_REMOTENODE_COST_PER_BYTE);

  //---------------------------------------------------------------------------
  //  Calculate CPU cost of copying an entire message buffer and the additional
  // cost of copying that buffer across a node or system boundary.
  //---------------------------------------------------------------------------
  const CostScalar instrToCopyA1KBMessage =
    ( instrPerByte *  csOneKiloBytes ).getCeiling();
  const CostScalar intraSegmentInstrToCopyA1KBMessage =
    ( intraSegmentInstrPerByte *  csOneKiloBytes ).getCeiling();
  const CostScalar remoteSegmentInstrToCopyA1KBMessage =
    ( remoteSegmentInstrPerByte * csOneKiloBytes ).getCeiling();

  CostScalar downMessages = downIntraCpuMessages + 
                            downRemoteSegmentMessages +
                            downIntraSegmentMessages;

  // these message lengths are in kilo bytes 
  CostScalar downIntraSegmentMessagesLength;
  CostScalar downRemoteSegmentMessagesLength;
  CostScalar downIntraCpuMessagesLength;

    // downMessageLength_ is in bytes; so we divide by csOneKiloBytes
    CostScalar normalizeFactor = downMessages * csOneKiloBytes; 
                                 
    downIntraSegmentMessagesLength = downMessageLength_ * downIntraSegmentMessages   
                        / normalizeFactor;

    downRemoteSegmentMessagesLength = downMessageLength_ * downRemoteSegmentMessages 
                                  / normalizeFactor ;
   
    downIntraCpuMessagesLength = downMessageLength_ * downIntraCpuMessages 
                                 / normalizeFactor; 
  /*else
  {
    // downMessageLength_ is already in kilo-bytes
    CostScalar normalizeFactor = downMessageLength_/downMessages;
    downIntraSegmentMessagesLength = downIntraSegmentMessages*normalizeFactor; 
    downRemoteSegmentMessagesLength = downRemoteSegmentMessages*normalizeFactor;
    downIntraCpuMessagesLength = downIntraCpuMessages*normalizeFactor;
  }*/


  //-----------------------------------------------------------------
  //  There are always two copies made to transfer data from DP2 root
  // to the master:
  //
  // 1.- From DP2InExe buffer to set up message buffer
  //     (done by root in dp2)
  // 2.- From the messaging system to the memory space of the
  //    master
  //
  // ESP to ESP communication works in the same way (for now)
  //-----------------------------------------------------------------
  const CostScalar senderIntraCPUCopies     = csOne;
  const CostScalar receiverIntraCPUCopies   = csOne;
  const CostScalar senderIntraSegmentCopies     = csOne;
  const CostScalar receiverIntraSegmentCopies   = csOne;
  const CostScalar senderRemoteSegmentCopies    = csOne;
  const CostScalar receiverRemoteSegmentCopies  = csOne;
  const CostScalar intraCPUCopiesPerMessage =
                                senderIntraCPUCopies + receiverIntraCPUCopies;
  const CostScalar interSegmentCopiesPerMessage =
                                senderIntraSegmentCopies + receiverIntraSegmentCopies;
  const CostScalar remoteSegmentCopiesPerMessage=
                                senderRemoteSegmentCopies+ receiverRemoteSegmentCopies;

  //-------------------------------------------------------------
  //  Distribute the load of messages.
  //  All messages affect the CPU component.
  //  Only internode messages affect the LOCAL message component.
  //  There are no intercluster (i.e. REMOTE) messages on NT.
  //  Note: Intra-node (i.e. intra-cpu on NSK) messages are 
  //  ignored when computing the LOCAL message component. Such
  //  messages merely involve a memory-to-memory copy.- Sunil
  //--------------------------------------------------------

  //------------------------------------------------------------------
  //  Compute number of copies for intra node, internode and remote
  // messages from the parent perspective. all messages are normaized to
  // 1 Kb even though up and down buffer sizes could be different
  // This is used in computing COPY cost only. 
  //------------------------------------------------------------------
  CostScalar parentIntraCPUCopyMessageLength =  
          (downIntraCpuMessagesLength * senderIntraCPUCopies)
        + (upIntraCPUMessages * upMessageBufferLength_ *
           receiverIntraCPUCopies);

  CostScalar parentIntraSegmentCopyMessageLength =
          (downIntraSegmentMessagesLength * senderIntraSegmentCopies)
        + (upIntraSegmentMessages * upMessageBufferLength_ * 
           receiverIntraSegmentCopies);

  CostScalar parentRemoteSegmentCopyMessageLength =
          (downRemoteSegmentMessagesLength * senderRemoteSegmentCopies)
        + (upRemoteSegmentMessages * upMessageBufferLength_  * 
           receiverRemoteSegmentCopies);

  // Divide total IntraNode messages by number of intra nodes.
  // Divide total InterNode messages by number of inter nodes.
  // Divide total RemoteNode messages by number of remote nodes.
  CostScalar intraNode = csZero;
  CostScalar interNode = csZero;
  CostScalar remoteNode = csZero;
  if ( CmpCommon::getDefault(COMP_BOOL_97) == DF_ON )
  {
    intraNode = (downIntraCpuMessages/downMessages) * numOfConsumers;
    interNode = (downIntraSegmentMessages/downMessages) * numOfConsumers;
    remoteNode = (downRemoteSegmentMessages/downMessages) * numOfConsumers;
  }
  else
  {
    intraNode = interNode = remoteNode = numOfConsumers;
  }

  //----------------------------------------------------------------
  //  We have calculated the total number of messages, now normalize
  // these to the number of parent's CPUs (numOfConsumers).
  // Note: What if we have 2 consuming ESPs per CPU (i.e. MAX_ESPS_PER_CPU_PER_OP=2)?
  // If so, then numOfConsumers <> no. of CPUs.-- Sunil
  //----------------------------------------------------------------
  CostScalar parentIntraCPUCopyMessageLengthPerConsumer   =  
                parentIntraCPUCopyMessageLength / intraNode.minCsOne();

  CostScalar parentIntraSegmentCopyLengthPerConsumer   =
             parentIntraSegmentCopyMessageLength / interNode.minCsOne();

  CostScalar parentRemoteSegmentCopyLengthPerConsumer  =  
          parentRemoteSegmentCopyMessageLength / remoteNode.minCsOne();

  CostScalar downIntraSegmentMessagesPerConsumer   =   downIntraSegmentMessages
                                             / interNode.minCsOne();
  CostScalar downRemoteSegmentMessagesPerConsumer   =   downRemoteSegmentMessages
                                             / remoteNode.minCsOne();

  //-------------------------------------------------------
  //  Calculate number of kilobytes of down internode and intersegment
  //  messages. They are normalized
  //-------------------------------------------------------
  CostScalar parentIntraSegmentMessagesInKbPerConsumer = 
             downIntraSegmentMessagesLength / interNode.minCsOne();

  CostScalar parentRemoteSegmentMessagesInKbPerConsumer=
            downRemoteSegmentMessagesLength / remoteNode.minCsOne(); 

  //----------------------------------------------------------------
  //  Calculate number of total message copies and internode message
  // copies for producing the first row.
  //----------------------------------------------------------------
  const CostScalar & firstRowParentIntraNodeCopiesPerConsumer =
    MINOF( parentIntraCPUCopyMessageLengthPerConsumer, intraCPUCopiesPerMessage );

  const CostScalar & firstRowParentInterNodeCopiesPerConsumer =
    MINOF( parentIntraSegmentCopyLengthPerConsumer, interSegmentCopiesPerMessage );

  const CostScalar & firstRowParentRemoteNodeCopiesPerConsumer =
    MINOF(parentRemoteSegmentCopyLengthPerConsumer, remoteSegmentCopiesPerMessage);

  //----------------------------------------------------------------------------
  //  Calculate memory usage.  We need enough memory for each copy of a message.
  // When the child does not executes in DP2, each producer needs enough memory
  // for each copy of a message.
  //----------------------------------------------------------------------------
  /*CostScalar parentMemory = messageBufferSizeInKb * (MINOF(parentInterNodeCopiesPerCPU,
                                                    interSegmentCopiesPerMessage)+
						     MINOF(parentIntraNodeCopiesPerCPU,
                                                     intraCPUCopiesPerMessage)+ 
                                                    MINOF(parentRemoteNodeCopiesPerCPU,
                                                    remoteSegmentCopiesPerMessage));
  if (NOT childExecutesInDP2)
  {
    parentMemory = parentMemory * numOfProducers;
  }*/

  //-----------------
  // Parent First Row
  //-----------------
  parentFR = new STMTHEAP SimpleCostVector;

  parentFR->setInstrToCPUTime(  firstRowParentIntraNodeCopiesPerConsumer
                       * instrToCopyA1KBMessage );

  parentFR->addNumLocalToMSGTime(
                           MINOF( csOne, downIntraSegmentMessagesPerConsumer ) );

  parentFR->addKBLocalToMSGTime(
                           MINOF( downMessageBufferLength_,
                                  parentIntraSegmentMessagesInKbPerConsumer ) );

  parentFR->addNumRemoteToMSGTime(
                           MINOF(csOne,downRemoteSegmentMessagesPerConsumer));

  parentFR->addKBRemoteToMSGTime(
                           MINOF(downMessageBufferLength_,
                                 parentRemoteSegmentMessagesInKbPerConsumer));

  //parentFR->setNormalMemory(parentMemory);
  parentFR->setNumProbes(numOfProbes);

  //-----------------
  //  Parent Last Row
  //-----------------
  parentLR = new STMTHEAP SimpleCostVector;

  CostScalar copyCost = 
      parentIntraCPUCopyMessageLengthPerConsumer * instrToCopyA1KBMessage +
      parentIntraSegmentCopyLengthPerConsumer * intraSegmentInstrToCopyA1KBMessage +
      parentRemoteSegmentCopyLengthPerConsumer * intraSegmentInstrToCopyA1KBMessage;
     // this used to be remoteSegmentInstrToCopyA1KBMessage;
     // may need some calibration work


  parentLR->setInstrToCPUTime(copyCost);

  parentLR->addNumLocalToMSGTime( downIntraSegmentMessagesPerConsumer) ;
  parentLR->addKBLocalToMSGTime( parentIntraSegmentMessagesInKbPerConsumer );
  parentLR->addNumRemoteToMSGTime( downRemoteSegmentMessagesPerConsumer );
  parentLR->addKBRemoteToMSGTime(parentRemoteSegmentMessagesInKbPerConsumer);
  // parentLR->setNormalMemory( parentMemory );
  parentLR->setNumProbes( numOfProbes );

  // give some weight for parallel plans compared to serial plan when
  // a large number of rows are being returned to application

  NADefaults &defs1 = ActiveSchemaDB()->getDefaults();
  CostScalar adj1 = defs1.getAsLong(COMP_INT_62);
  if (adj1 == csZero)
     adj1=1000000;

  NABoolean adjustmentForParallelPlans = FALSE;
  if ( (numOfConsumers == csOne) AND 
       (numOfProducers > numOfConsumers) AND
        isOpBelowRoot_  AND
       upRowsPerConsumer_  > adj1 )
  {
     parentLR->setToValue(0.0001)  ;
     parentFR->setToValue(0.0001)  ;
     adjustmentForParallelPlans = TRUE;
  }

  //------------------------------------------------------------------
  // Messages Length in bytes at the child layer
  // 
  //------------------------------------------------------------------
  CostScalar childIntraCPUCopyMessageLength =
          (downIntraCpuMessagesLength * receiverIntraCPUCopies)
        + (upIntraCPUMessages   * upMessageBufferLength_ *
           senderIntraCPUCopies);

  CostScalar childIntraSegmentCopyMessageLength =
          (downIntraSegmentMessagesLength * receiverIntraSegmentCopies)
        + (upIntraSegmentMessages * upMessageBufferLength_   *
           senderIntraSegmentCopies);

  CostScalar childRemoteSegmentCopyMessageLength =
          (downRemoteSegmentMessagesLength * receiverIntraSegmentCopies)
        + (upRemoteSegmentMessages  * upMessageBufferLength_ *
           senderRemoteSegmentCopies);

  // Divide total IntraNode messages by number of intra nodes.
  // Divide total InterNode messages by number of inter nodes.
  // Divide total RemoteNode messages by number of remote nodes.
  CostScalar upMessages =  upIntraCPUMessages + upIntraSegmentMessages
                         + upRemoteSegmentMessages;
  if ( CmpCommon::getDefault(COMP_BOOL_97) == DF_ON )
  {
    intraNode = (upIntraCPUMessages/upMessages) * numOfProducers;
    interNode = (upIntraSegmentMessages/upMessages) * numOfProducers;
    remoteNode = (upRemoteSegmentMessages/upMessages) * numOfProducers;
  }
  else
  {
    intraNode = interNode = remoteNode = numOfProducers;
  }


  //-------------------------------------------------------------------------
  //  The messages used in calculating childIntraNodeCopiesPerCPU and
  // upIntraSegmentMessagesPerCPU,upRemoteSegmentMessagesPerCPU were based on the
  // MAXOF producers and senders. Normalize this to the number of CPUs in the child.
  //-------------------------------------------------------------------------
  CostScalar childIntraCPUCopyLengthPerProducer =  
           childIntraCPUCopyMessageLength / intraNode.minCsOne();

  CostScalar childIntraSegmentCopyLengthPerProducer =
           childIntraSegmentCopyMessageLength / interNode.minCsOne();

  CostScalar childRemoteSegmentCopyLengthPerProducer      =  
           childRemoteSegmentCopyMessageLength / remoteNode.minCsOne();

  CostScalar childUpIntraSegmentMessagesPerProducer        =  
           upIntraSegmentMessages / interNode.minCsOne();

  CostScalar childUpRemoteSegmentMessagesPerProducer       =   
            upRemoteSegmentMessages / remoteNode.minCsOne();

  CostScalar childIntraSegmentMessagesInKbPerProducer =
        upIntraSegmentMessages * upMessageBufferLength_/interNode.minCsOne();

  CostScalar childRemoteSegmentMessagesInKbPerProducer=
    upRemoteSegmentMessages * upMessageBufferLength_ / remoteNode.minCsOne();

  const CostScalar & firstRowChildIntraNodeCopiesPerProducer =
   MINOF( childIntraCPUCopyLengthPerProducer, childIntraCPUCopyMessageLength);

  //----------------------------------------------------------------------------
  //  Calculate memory usage.  We need enough memory for each copy of a message.
  // When the child does not execute in DP2, each consumer needs enough memory
  // for each copy of a message.
  //----------------------------------------------------------------------------
  CostScalar  childMemory = messageBufferSizeInKb * (interSegmentCopiesPerMessage
                                                     + remoteSegmentCopiesPerMessage);
  if ( NOT childExecutesInDP2 )
  {
    childMemory = childMemory * numOfConsumers;
  }

  //------------------------------------------------------------------------------
  // Cost to compute the hash value for a row.
  //
  // I have made 2 changes: The hash value needs to be computed only if range or
  // hash repartitioning is taking place. And this determination needs to be made 
  // based on the Exchange's (and not child's) partitioning function. Note that we
  // ignore the cpu costs associated with round-robin and random repartitioning;
  // these costs are trivial. Range-repartitioning involves evaluating the range
  // partitioning function for each row. This requires a binary search through
  // the partition key arrary, using an encoded key (derived from the row) as the
  // search key. In lieu of a cost estimate for this binary search, we'll use the
  // CPU cost for computing a hash partitioning function. - Sunil
  //-------------------------------------------------------------------------------
  CostScalar cpuCostHashRow = csZero;
  if ( NOT childExecutesInDP2 AND numOfConsumers.isGreaterThanOne() /*> csOne*/ 
       AND ( myPartFunc->isAHashPartitioningFunction() 
             OR myPartFunc->isAHash2PartitioningFunction()
             OR myPartFunc->isAHashDistPartitioningFunction()
	     OR myPartFunc->isARangePartitioningFunction() 
             OR myPartFunc->isASkewedDataPartitioningFunction()
             ))
  {
    if ( CmpCommon::getDefault(COMP_BOOL_97) == DF_OFF )
      cpuCostHashRow =
        CostPrimitives::cpuCostForHash( myPartFunc->getPartitioningKey() );
    else
      cpuCostHashRow =
        CostPrimitives::cpuCostForHash( childPartFunc->getPartitioningKey() );
  }

  // if the exchange is PAPA and for Hash2 and Hash1 pfs, we need to
  // cost hashing of incoming probes... 
  if ( (CmpCommon::getDefault(COMP_BOOL_57) == DF_OFF) AND
        childExecutesInDP2 AND 
       (numOfProducers> numOfConsumers) AND
        childPartFunc->isALogPhysPartitioningFunction() AND
       numOfProbes.isGreaterThanOne() 
     )
  {
     PartitioningFunction* phys  = childPartFunc->
                                   castToLogPhysPartitioningFunction()->
                                   getPhysPartitioningFunction();

     // Hash2 and Hash1 partitioning functions have different functions
     // to compute partition numbers and hash1 is more expensive
     // that should be reflected in the cost computation

     if (phys->isAHash2PartitioningFunction() OR
         phys->isAHashDistPartitioningFunction() OR
         phys->isARangePartitioningFunction())
     {
        Exchange *exch= (Exchange *) op_;
        NABoolean areProbesHashed=
            exch->areProbesHashed(childPartFunc->getPartitioningKey());
       if (areProbesHashed)
       {
         CostScalar cpuCostHashProbes =
          CostPrimitives::cpuCostForHash( childPartFunc->getPartitioningKey() );
         parentLR->addInstrToCPUTime( cpuCostHashProbes * 
                                      (numOfProbes/numOfConsumers).minCsOne());

       }
     }
    
  }
  
  if ( (CmpCommon::getDefault(COMP_BOOL_57) == DF_OFF) AND
       isMergeNeeded_ AND
       numOfProducers> numOfConsumers )
  {
     CostScalar cpuCostCompareKeys =
          CostPrimitives::cpuCostForCompare(sppForMe_->getSortKey());
     parentLR->addInstrToCPUTime( cpuCostCompareKeys * 
                                     upRowsPerConsumer_);
  }

  //----------------------------------------------------------------------------
  //  Compute maximum number of rows that can fit in a single message buffer and
  // make this the upper bound for the number of messages associated with the
  // child's first row cost.
  //----------------------------------------------------------------------------
  const CostScalar maxRowsPerMessage =
    ( ( upMessageBufferLength_ - messageHeaderInKb ) 
         / messageSpacePerRecordInKb).getFloor();

  const CostScalar & firstRowNumOfRows =
    MINOF( upRowsPerConsumer, maxRowsPerMessage );

  //----------------
  // Child First Row
  //----------------

  childFR = new STMTHEAP SimpleCostVector;

  childFR->setInstrToCPUTime(  
                        firstRowChildIntraNodeCopiesPerProducer
                        * instrToCopyA1KBMessage
                      + (firstRowNumOfRows * cpuCostHashRow));

  childFR->addNumLocalToMSGTime( 
        MINOF( csOne, childUpIntraSegmentMessagesPerProducer )
       );

  childFR->addKBLocalToMSGTime(
                            MINOF(messageBufferSizeInKb,
                                  childIntraSegmentMessagesInKbPerProducer));
  childFR->addNumRemoteToMSGTime(
                            MINOF(csOne,childUpRemoteSegmentMessagesPerProducer));

  childFR->addKBRemoteToMSGTime(
                            MINOF(messageBufferSizeInKb,
                                  childRemoteSegmentMessagesInKbPerProducer));

  // childFR->setNormalMemory( childMemory );
  childFR->setNumProbes( numOfProbes );

  //---------------
  // Child Last Row
  //---------------


  childLR = new STMTHEAP SimpleCostVector;

  copyCost = childIntraCPUCopyLengthPerProducer * instrToCopyA1KBMessage +
       childIntraSegmentCopyLengthPerProducer * intraSegmentInstrToCopyA1KBMessage +
       childRemoteSegmentCopyLengthPerProducer * intraSegmentInstrToCopyA1KBMessage;
   // this used to be remoteSegmentInstrToCopyA1KBMessage; calibration?

  CostScalar adjustment=upRowsPerConsumer;
  NADefaults &defs = ActiveSchemaDB()->getDefaults();
  CostScalar adjFact = defs.getAsLong(COMP_INT_61);


  // If this exchange is partitioning too few rows is it really needed?
  // Adjust rows such that it is atleast numOfConsumers * numOfProducers
  // do not do this if it is in Dp2 or if number of consumers is one
  // this is primarily because partial group-by cardinality is 
  // under-estimated
  /*if ((upRowsPerConsumer * numOfConsumers  < numOfConsumers * numOfProducers ||
      upRowsPerConsumer * numOfConsumers  < adjFact ) &&
     numOfConsumers > csOne &&
     NOT childExecutesInDP2)
  { 
     adjustment = numOfConsumers * numOfProducers;
     if (adjustment < adjFact)
        adjustment = adjFact;
   
  }*/

  childLR->setInstrToCPUTime(  copyCost + 
                              adjustment * cpuCostHashRow
                            );

  childLR->addNumLocalToMSGTime( childUpIntraSegmentMessagesPerProducer );
  childLR->addKBLocalToMSGTime( childIntraSegmentMessagesInKbPerProducer );
  childLR->addNumRemoteToMSGTime(childUpRemoteSegmentMessagesPerProducer );
  childLR->addKBRemoteToMSGTime( childRemoteSegmentMessagesInKbPerProducer );
  // childLR->setNormalMemory( childMemory );
  childLR->setNumProbes( numOfProbes );

  
  if (adjustmentForParallelPlans)
  {
     childFR->scaleByValue(csZero);
     childLR->scaleByValue(csZero);
  }

  //----------------------------------------------------------------
  //  Compute the cost of starting ESPs as an idle time cost.  Store
  // this idle time in both the first row and last row vectors.
  //----------------------------------------------------------------
  //SimpleCostVector* espStartupCost = computeESPCost(NOT executeInDP2,
  //                                                  numOfProbes);
  //CostScalar espElapsedTime =  espStartupCost->getCPUTime();
  //                             + espStartupCost->getIOTime();
  // computeEXPCost has been simplified by removing IO component. Nov.2005

  CostScalar espElapsedTime = computeESPCost(NOT childExecutesInDP2,numOfProbes);
  // ESP startup cost is waited, so must pay the price of ESP startup
  // for each consumer, according to Hans (4/20/99).
  espElapsedTime *= numOfConsumers;
  CostScalar startupAdj = (ActiveSchemaDB()->getDefaults())\
                                  .getAsDouble(COMP_INT_63);
if(isOpBelowRoot_ && (NOT childExecutesInDP2) && ( numOfProbes < 10 ) && (CmpCommon::getDefault(COMP_BOOL_122) == DF_ON) ){
  parentFR->addToCpuTime( (espElapsedTime * startupAdj) / numOfProbes );
  parentLR->addToCpuTime(  (espElapsedTime * startupAdj) / numOfProbes );
  }

} // CostMethodExchange::produceCostVectors()
//<pb>
//==============================================================================
//  Compute cost object for Exchange operator given the parent and child first
// row and last row cost vectors.
//
// Input:
//  parentFR        -- resources used by parent to produce first row.
//
//  parentFR        -- resources used by parent to produce last row.
//
//  childFR         -- resources used by child to produce first row.
//
//  childFR         -- resources used by child to produce last row.
//
//  numOfProducers  -- number of child processes receiving down messages and
//                      sending up messages.
//
//  numOfConsumers  -- number parent processes receiving up messages and sending
//                      down messages.
//
// Output:
//  none
//
// Return:
//  Pointer to cost object for exchange operator.
//
//==============================================================================
Cost*
CostMethodExchange::computeExchangeCost( const CostVecPtr   parentFR,
                                         const CostVecPtr   parentLR,
                                         const CostVecPtr   childFR,
                                         const CostVecPtr   childLR,
                                         const CostScalar & numOfConsumers,
                                         const CostScalar & numOfProducers ) const
{

  //----------------------------------------------------------------------------
  //  When computing an Exchange's cost object, we must construct the parent and
  // child cost objects separately because they could possibly use a different
  // number of CPUs and have a different number of streams per CPU.  By
  // constructing separate costs objects for the parent and child we ensure that
  // the first row and last row costs are normalized appropriately per CPU and
  // per stream.  Once both the parent and child costs have been normalized, we
  // can safely combine them into a single cost object.
  //----------------------------------------------------------------------------

  //---------------------
  //  Compute Child cost.
  //---------------------
  Cost * childExchangeCost =
    new STMTHEAP Cost( childFR,
		       childLR,
		       NULL,
		       Lng32(numOfProducers.getValue()),
		       1 );

  //---------------------
  // Compute Parent cost.
  //---------------------
  Cost * parentExchangeCost =
    new STMTHEAP Cost( parentFR,
		       parentLR,
		       NULL,
		       Lng32(numOfConsumers.getValue()),
		       1 );

  //-------------------------------------------------------------------
  //  When we get a CPU map we can possibly overlay the addition of the
  // send and receive costs if they don't both occur on common CPUs.
  //-------------------------------------------------------------------

  //------------------------------------------------------------------
  //  Finally, combine normalized parent and child costs into a single
  // Exchange cost object
  //------------------------------------------------------------------
  SimpleCostVector cvFR =   childExchangeCost->getCpfr()
                          + parentExchangeCost->getCpfr();

  SimpleCostVector cvLR =   childExchangeCost->getCplr()
                          + parentExchangeCost->getCplr();

  Cost* exchangeCost =
    new STMTHEAP Cost( &cvFR,
                       &cvLR,
                       NULL,  // No blocking vector for exchange.
                       1,     // vectors already normalized to number of CPUs
                       1 );   // and plan fragments per CPU


  //----------------------------------------------------------------------
  //  Now set number of streams per CPU and count of CPUs correctly for
  // Exchange.  It won't ever be used (today) - we are just doing this for
  // consistency.
  //----------------------------------------------------------------------
  exchangeCost->planFragmentsPerCPU() = 1;
  exchangeCost->countOfCPUs()         = Lng32(numOfConsumers.getValue());

  //-----------------------------------------------
  //  As good citizens we clean up after ourselves.
  //-----------------------------------------------
  delete childExchangeCost;
  delete parentExchangeCost;

  return exchangeCost;

} // CostMethodExchange::computeExchangeCost()
// LCOV_EXCL_STOP
//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodFileScan                         */
/*                                                                    */
/**********************************************************************/
Cost*
CostMethodFileScan::computeOperatorCostInternal(RelExpr* op,
                                                const Context* myContext,
                                                Lng32& countOfStreams)
{

  FileScan* p = (FileScan*)op;         // downcast
  Cost *costPtr = NULL;

  // short cut for hbase for now
//  if (p->isHbaseTable()) {
//
//     CostScalar outputRowSize = 100 /* getEstimatedRecordLength*/;
//     CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);
//
//     Cost* hbaseSCanCost=  scmCost(100,
//                                100,
//                                csZero,
//                                csZero,
//                                csZero,
//                                noOfProbesPerStream_,
//                                csZero,
//                                csZero,
//                                outputRowSize,
//                                csZero);
//     return hbaseSCanCost;
//  }


  // The fileScanOptimizer performs several actions:
  // 1. Decides the access method for the scan
  // 2.- Computes the cost for the access method
  // 3.- Builds a key for the access method and attaches
  //     that key to the scan.
  // 4.- The FileScanOptimizer is side-effect free (i.e. it
  //     won't side-effect its paraemters)

  // This cannot be computed in the filescan optimizer since
  // outputLogProp is not const (and due to lazy evaluation of
  // log. properties) cannot be made into a const method:


  CostScalar resultSetCardinality =
   p->getGroupAttr()->
    outputLogProp(myContext->getInputLogProp())->getResultCardinality();
  // $$$ Due to a bug in histograms (up to tag A091197_1)
  // $$$ sometimes the cardinality is negative, if so, fix it
  // $$$ to pass regressions:
  if ( resultSetCardinality.isLessThanZero() /* < csZero */ )
    resultSetCardinality = CostScalar(100.0);


  ValueIdSet
    externalInputs(p->getGroupAttr()->getCharacteristicInputs());

  // ---------------------------------------------------------------------
  // Apply partitioning key predicates if necessary
  // ---------------------------------------------------------------------
  // Scan is a leaf so it has its physical properties available
  LogPhysPartitioningFunction *logPhysPartFunc =
    (LogPhysPartitioningFunction *)  // cast away const
    myContext->getPlan()->getPhysicalProperty()->getPartitioningFunction()->
    castToLogPhysPartitioningFunction();

  if ( logPhysPartFunc != NULL )
  {
    LogPhysPartitioningFunction::logPartType logPartType =
      logPhysPartFunc->getLogPartType();

    if (    logPartType == LogPhysPartitioningFunction::LOGICAL_SUBPARTITIONING
	 OR logPartType == LogPhysPartitioningFunction::HORIZONTAL_PARTITION_SLICING
       )
    {
      // We need to add the partition input values so that
      // they are considered when putting together the keys
      externalInputs +=
        logPhysPartFunc->getLogPartitioningFunction()->getPartitionInputValues();
    }
  }

  ScanOptimizer *scanOptimizer =
    ScanOptimizer::getScanOptimizer(*p /*in, not side-effected */
                                    ,resultSetCardinality /* in */
                                    ,*myContext
                                    ,externalInputs
                                    ,STMTHEAP
                                    );

  // -----------------------------------------------------------------------
  // $$$ Below we would like to use the virtual function
  // mechanism to get back a single object of type ScanKey&,
  // however, in order to do this I would have to add a
  // preCodeGen implementation to class SearchKey. This takes
  // some work and I'd rather do other things for now. But
  // in the future the SearchKey and Mdamkey generic
  // behaviour should be completely unified by their ancestor
  // ScanKey.
  // -----------------------------------------------------------------------

  SearchKey *searchKeyPtr = NULL;
  MdamKey *mdamKeyPtr = NULL;
  NABoolean replicateKeyPredsBecauseOfKeyKludge = FALSE;

  NABoolean
    isNestedJoin = ( myContext->getInputLogProp()->getColStats().entries() > 0 );

// LCOV_EXCL_START
// excluded for coverage because DEBUG only code
if (CURRSTMT_OPTDEFAULTS->optimizerHeuristic2()) {//#ifndef NDEBUG
  if (isNestedJoin)
    (*CURRSTMT_OPTGLOBALS->nestedJoinMonitor).enter();
  (*CURRSTMT_OPTGLOBALS->fileScanMonitor).enter();
}//#endif
// LCOV_EXCL_STOP
  costPtr = scanOptimizer->optimize(searchKeyPtr,  /* out */
                                    mdamKeyPtr     /* out */);

// LCOV_EXCL_START
// excluded for coverage because DEBUG only code
if (CURRSTMT_OPTDEFAULTS->optimizerHeuristic2()) {//#ifndef NDEBUG
  (*CURRSTMT_OPTGLOBALS->fileScanMonitor).exit();
  if (isNestedJoin)
    (*CURRSTMT_OPTGLOBALS->nestedJoinMonitor).exit();
// LCOV_EXCL_STOP
}//#endif

  // Set blocks per access estimate. Use value from the defaults
  // table if set by the user. If not set, compute it.
  Lng32 blocksPerAccess = (Lng32)CURRSTMT_OPTDEFAULTS->getNumOfBlocksPerAccess();
  if (blocksPerAccess == 0)
  {
    // Defaults table must have specified SYSTEM.
    blocksPerAccess = scanOptimizer->getNumberOfBlocksToReadPerAccess();
  }

  // Check if user overrode the blocks per access estimate via CQ Shape.
  // If so, override the value from the defaults table or our computation.
  const ReqdPhysicalProperty* propertyPtr =
    myContext->getReqdPhysicalProperty();
  if ( propertyPtr
       && propertyPtr->getMustMatch()
       && (propertyPtr->getMustMatch()->getOperatorType()
	   == REL_FORCE_ANY_SCAN))
    {
      ScanForceWildCard* scanForcePtr =
        (ScanForceWildCard*)propertyPtr->getMustMatch();
      if (scanForcePtr->getNumberOfBlocksToReadPerAccess() > -1)
        blocksPerAccess = scanForcePtr->getNumberOfBlocksToReadPerAccess();
    }
  // Get estimated Dp2 rows accessed from ScanOptimizer.
  CostScalar estRowsAccessed = scanOptimizer->getEstRowsAccessed();
  // Set blocks read (hint to DP2)
  p->setNumberOfBlocksToReadPerAccess(blocksPerAccess);
  p->setEstRowsAccessed(estRowsAccessed);
  if (searchKeyPtr)
    {
      // Single subset was chosen, set the key:
      p->setSearchKey(searchKeyPtr);
    }
  else
    {
      // Mdam was chosen, set the key:
      p->setMdamKey(mdamKeyPtr);
    }

  // For the scan, the streams are given by the number of
  // active partitions (when cost is going down this number
  // may be wrong because of logical partitioning may actually
  // increase (or decrease) the number of *real* partitions.
  // When cost is going up we get the streams from the
  // *real* part. func. in the phys. props
  // The file scan optimizer figures this out in the method below
  if (CmpCommon::getDefault(NCM_HBASE_COSTING) == DF_ON)
    countOfStreams = 
      (Lng32)scanOptimizer->getEstNumActivePartitionsAtRuntimeForHbaseRegions();
  else
    countOfStreams = (Lng32)scanOptimizer->getNumActivePartitions();

  // ------------------------------------------------------------------------
  // If we are on the right leg of a parallel nested join, the
  // number of probes is 1, and we are underneath a materialize
  // that does not pass the probes through, then we are going to
  // have to scan the entire table N times, where N is the required
  // number of logical partitions.
  // ------------------------------------------------------------------------
  const ReqdPhysicalProperty* rpp = myContext->getReqdPhysicalProperty();
  const LogicalPartitioningRequirement *lpr =
    rpp->getLogicalPartRequirement();
  PartitioningRequirement * logPartReq = NULL;
  if (lpr != NULL)
    logPartReq  = lpr->getLogReq();
  const CostScalar & noOfProbes =
    ( myContext->getInputLogProp()->getResultCardinality() ).minCsOne();
  ValueIdSet outerRefs;
  externalInputs.getOuterReferences(outerRefs);

  if (logPartReq
      AND logPartReq ->isRequirementReplicateNoBroadcast() // parallel n.j.
      AND (noOfProbes == 1)    // 1 probe, but
      AND outerRefs.isEmpty()) //no probe values
  {
    // We have replicate no broadcast underneath a materialize that
    // does not pass the probes through. This means we are on the
    // right leg of a parallel nested join and each ESP is going to
    // have to read the entire table, but the scan costing was
    // unaware of this. Multiply the last row and total cost
    // by the degree of parallelism.
    const CostScalar & numParts = logPartReq->getCountOfPartitions();
    costPtr->cplr() = costPtr->getCplr() * numParts;
    costPtr->totalCost() = costPtr->getTotalCost() * numParts;
  }

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
// LCOV_EXCL_START :dpm
// excluded for coverage because DEBUG only code
#ifndef NDEBUG
  if ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON )
  {
    pfp = stdout;
    fprintf(pfp,"FILESCAN::computeOperatorCost()\n");
    costPtr->print(pfp);
    fprintf(pfp, "Elapsed time: ");
    fprintf(pfp,"%f", costPtr->
            convertToElapsedTime(
                 myContext->getReqdPhysicalProperty()).
            value());
    fprintf(pfp,"\n");
  }
#endif
// LCOV_EXCL_STOP

  // transfer probe counters to p (the FileScan)
  p->setProbes(scanOptimizer->getProbes());
  p->setSuccessfulProbes(scanOptimizer->getSuccessfulProbes());
  p->setUniqueProbes(scanOptimizer->getUniqueProbes());
  p->setDuplicatedSuccProbes(scanOptimizer->getDuplicateSuccProbes());
  p->setTuplesProcessed(scanOptimizer->getTuplesProcessed());

  delete scanOptimizer;

  return costPtr;

} // CostMethodFileScan::computeOperatorCostInternal()
//<pb>

/**********************************************************************/
/*                                                                    */
/*                         CostMethodDP2Scan                          */
/*                                                                    */
/**********************************************************************/
Cost*
CostMethodDP2Scan::computeOperatorCostInternal(RelExpr* op,
				               const Context* myContext,
                                               Lng32& countOfStreams)
{

  DP2Scan* p = (DP2Scan*)op;         // downcast

  // The DP2 costing is exactly as the file scan, thus use
  // the filescan's costing here:
  Cost *costPtr = CostMethodFileScan::computeOperatorCostInternal( op,
                     myContext, countOfStreams );

  return costPtr;

} // CostMethodDP2Scan::computeOperatorCostInternal()
//<pb>


// ----QUICKSEARCH FOR FIXEDROW...........................................

/**********************************************************************/
/*                                                                    */
/*                     CostMethodFixedCostPerRow                      */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodFixedCostPerRow::computeOperatorCostInternal().
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu -- OCM code
Cost*
CostMethodFixedCostPerRow::computeOperatorCostInternal(RelExpr* op,
                                                       const Context* myContext,
                                                       Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // This CostMethod basically computes a CPU cost for operators whose
  // costs are not that important. It applies the formula:
  // baseCpuCost_ +
  // cpuCostPerChildRow_ * child0RowCount_ +
  // cpuCostPerOutputRow_ * myRowCount_, where the row counts are those
  // of the total result set, and then amortize the cost across streams.
  // It takes the first row count to be just its last row count amortized
  // across the no of probes.
  // ---------------------------------------------------------------------
  CostScalar cpu = baseCpuCost_;

  if(op->getArity() > 0)
  {
    EstLogPropSharedPtr child0LogProp = op->child(0).outputLogProp(inLogProp_);
    cpu += cpuCostPerChildRow_ * child0LogProp->getResultCardinality();
  }
  cpu += cpuCostPerOutputRow_ * myRowCount_;

  // ---------------------------------------------------------------------
  // Synthesize the cost vectors.
  // ---------------------------------------------------------------------
  SimpleCostVector cvLR (
    cpu/countOfStreams_ * CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions(),
    csZero,csZero,csZero,
    noOfProbesPerStream_);

  SimpleCostVector cvFR (
    cpu/countOfStreams_/noOfProbesPerStream_	  // converting CPU instr
     * CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions(), //into time
    csZero,csZero,csZero,
    noOfProbesPerStream_);

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"FIXEDCOST::computeOperatorCost()\n");
    cvFR.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return the cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  Cost *costPtr = new STMTHEAP Cost( &cvFR,
				     &cvLR,
				     NULL,
				     cpuCount,
				     fragmentsPerCPU
				   );

#ifndef NDEBUG
  if ( printCost )
    {
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodFixedCostPerRow::computeOperatorCostInternal().
//<pb>
// -----------------------------------------------------------------------
// CostMethodFixedCostPerRow::print().
// -----------------------------------------------------------------------
void CostMethodFixedCostPerRow::print(FILE* ofd,
                                      const char* indent,
                                      const char* title) const
{
#pragma nowarn(1506)   // warning elimination
  BUMP_INDENT(indent);
#pragma warn(1506)  // warning elimination
  CostMethod::print(ofd,indent,title);
  fprintf(ofd,"%s ",NEW_INDENT);
  fprintf(ofd,
          "baseCpuCost=%g cpuCostPerChildRow=%g cpuCostPerOutputRow=%g",
          baseCpuCost_.value(),
          cpuCostPerChildRow_.value(),
          cpuCostPerOutputRow_.value());

  fprintf(ofd,"\n ");

} // CostMethodFixedCostPerRow::print()
// LCOV_EXCL_STOP
//<pb>

// ----QUICKSEARCH FOR SORT...............................................

/**********************************************************************/
/*                                                                    */
/*                           CostMethodSort                           */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodSort cacheParameters().
// -----------------------------------------------------------------------
void CostMethodSort::cacheParameters(RelExpr* op,
                                     const Context* myContext)
{
  CostMethod::cacheParameters(op,myContext);

  // Get addressability to the defaults table and extract default memory.
  // CacheParameters() can set memoryLimit_ to > 20MB in some cases, for
  // example if the query tree contains REL_GROUPBY operator. This will
  // result in the assumption that even BMO plans can be sorted in memory,
  // , but executor uses internal sorts only if the sort table  size < 20MB.
  NADefaults &defs               = ActiveSchemaDB()->getDefaults();
  memoryLimit_ = defs.getAsDouble(MEMORY_UNITS_SIZE);

  sort_ = (Sort*) op_;

  ValueIdSet sortKeyVis;

  if(sort_->getSortKey().isEmpty())
  {
    CMPASSERT(NOT sort_->getArrangedCols().isEmpty());
    sortKeyVis = sort_->getArrangedCols();
  }
  else
  {
    sortKeyVis.insertList(sort_->getSortKey());
  }

  Lng32 myRowLength = myVis().getRowLength();

  // allocate space for atleast 2 rows
  memoryLimit_ = MAXOF(memoryLimit_, 2 * myRowLength);

  sortKeyLength_ = sortKeyVis.getRowLength();
  sortRecLength_ = sortKeyLength_ + myRowLength;


  // ---------------------------------------------------------------------
  // The key of a row is encoded. A copy of the row (together with the
  // encoded key) is then made to a buffer the Sort session allocates.
  // ---------------------------------------------------------------------
  cpuCostSendRow_ = CostPrimitives::cpuCostForEncode(sortKeyVis) +
                           CostPrimitives::cpuCostForCopyRow(myRowLength);

  // ---------------------------------------------------------------------
  // Cost to compare the keys of two rows.
  // ---------------------------------------------------------------------
  cpuCostCompareKeys_ = CostPrimitives::cpuCostForCompare(sortKeyVis);

  // ---------------------------------------------------------------------
  // Executor allocates result buffer to hold the row it receives. This
  // is the cost to make a copy of the row to that buffer.
  // ---------------------------------------------------------------------
  cpuCostCopyResultRow_ = CostPrimitives::cpuCostForCopyRow(myRowLength);

  // ---------------------------------------------------------------------
  // Determine the max run generation order and merge order if there is
  // a memory limit.
  // ---------------------------------------------------------------------
  if(isBMO_)
  {
    // Save two buffers' memory for the double output buffering.
    double memory = memoryLimit_ - ioBufferSize_ * 2.;

    // Memory allocated to a BMO should not be that small.
    CMPASSERT(memory > 0.)

    // -------------------------------------------------------------------
    // Memory needs for each row during run generation. We need storage
    // for the row itself and its associated tree node.
    // -------------------------------------------------------------------
    double memoryForEachRowAtRunGenPhase =
                                 (sortRecLength_ / 1024. + treeNodeSize_);
    maxRunGenOrder_ = (memory / memoryForEachRowAtRunGenPhase);

    // Memory allocated to a BMO should not be that small.
    CMPASSERT(maxRunGenOrder_ >= 2.)

    // -------------------------------------------------------------------
    // Memory needs for each run during a merge phase. We need a buffer
    // and an internal node for each run.
    // -------------------------------------------------------------------
    double memoryForEachRunAtMergePhase = (ioBufferSize_ + treeNodeSize_);
    maxMergeOrder_ = (memory / memoryForEachRunAtMergePhase);

    // Memory allocated to a BMO should not be that small.
    CMPASSERT(maxMergeOrder_ >= 2.);
  }
  else
  // ---------------------------------------------------------------------
  // No memory limit. Just do internal sort.
  // ---------------------------------------------------------------------
  {
    maxRunGenOrder_ = maxMergeOrder_ = 0.;
  }
}  // CostMethodSort::cacheParameters().
//<pb>
// -----------------------------------------------------------------------
// CostMethodSort computeOperatorCost().
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu -- OCM code
Cost*
CostMethodSort::computeOperatorCostInternal(RelExpr* op,
                                            const Context* myContext,
                                            Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------

  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // Cost scalars to be computed.
  // ---------------------------------------------------------------------
  CostScalar cpuBK(csZero), ioSeekBK(csZero), ioByteBK(csZero); //j memBK(csZero), diskBK(csZero);
  CostScalar cpuLR(csZero), ioSeekLR(csZero), ioByteLR(csZero); //j memLR(csZero), diskLR(csZero);

  CostScalar rowCount(csZero);
  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
        (partFunc_ != NULL) )
  {
    CostScalar myRowCountPerStream =
    myLogProp_->getCardOfBusiestStream(partFunc_,
                                       countOfStreams_,
                                       ga_,
                                       countOfAvailableCPUs_);
    rowCount = ( myRowCountPerStream / noOfProbesPerStream_ ).minCsOne();
  }
  else
  {
  // Row count a single probe on one instance of this sort is processing.
    rowCount =
    ( myRowCount_ / countOfStreams_ / noOfProbesPerStream_ ).minCsOne();
  }

  // Memory requirements of the whole table to be sorted.
  // Add 12 bytes for executor's tuple descriptor size.
  CostScalar tableSize = (rowCount * (sortRecLength_ + 12)) / csOneKiloBytes;

  // ---------------------------------------------------------------------
  // Per probe setup costs. This captures the cost to call the method
  // SortUtil::SortInitialize() to allocate the SortAlgo objects (such as
  // Tree, TreeNode, Record and ScratchSpace), etc.
  // ---------------------------------------------------------------------
  cpuBK += cpuCostPerProbeInit_;

  // ---------------------------------------------------------------------
  // The key of a row is encoded. A copy of the row (together with the
  // encoded key) is then made to a buffer the Sort session allocates.
  // ---------------------------------------------------------------------
  cpuBK += cpuCostSendRow_ * rowCount;

  // ---------------------------------------------------------------------
  // If this is not a BMO, we can use as much memory as we wish. Assume
  // an internal quick sort session will be done, which takes O(n*logn)
  // comparisons. Assume the same case if the whole file can fit into
  // memory.
  // ---------------------------------------------------------------------
  if( ((NOT isBMO_) OR (tableSize < memoryLimit_)) )
  {
    //j memBK += tableSize;
    cpuBK += cpuCostCompareKeys_ * qsFludgeFactor_ *
                                         rowCount * log(rowCount.value());
  }
  else
  {
    CMPASSERT(rowCount >= maxRunGenOrder_);

    // -------------------------------------------------------------------
    // We have to do run generation. Each row is put into a tournament
    // tree and the new winner of the tree is determined in O(h) time,
    // where h is the height of the tree.
    // -------------------------------------------------------------------
    CostScalar h = log(maxRunGenOrder_.value()) / log(2.);
    cpuBK += cpuCostCompareKeys_ * rsFludgeFactor_ * h * rowCount;

    // -------------------------------------------------------------------
    // The whole table is flushed to disk.
    // -------------------------------------------------------------------
    ioSeekBK += tableSize / ioBufferSize_;
    ioByteBK += tableSize;
    //j diskBK += tableSize;

    // -------------------------------------------------------------------
    // The expected no of rows in each run generated from Replacement
    // Selection is twice the number of nodes in the tree.
    // -------------------------------------------------------------------
    CostScalar rowCountPerRun = MINOF( maxRunGenOrder_ * csTwo, rowCount );
    CostScalar noOfRuns = (rowCount / rowCountPerRun).getCeiling();

    // -------------------------------------------------------------------
    // Intermediate merge passes are needed if we have generated more
    // runs than the max merge order.
    // -------------------------------------------------------------------
    if(maxMergeOrder_ < noOfRuns)
    {
      // -----------------------------------------------------------------
      // Should intermediate pass ever occur, we need twice the disk
      // space since both the input and output files of the intermediate
      // pass have to reside on the disk at the same time.
      // -----------------------------------------------------------------
      //j diskBK += tableSize;

      // -----------------------------------------------------------------
      // During each intermediate pass, (noOfRuns/maxMergeOrder_) merge
      // sessions are executed. Each merge session merges maxMergeOrder_
      // runs into a single run. If the number of runs left after a pass
      // remains to be larger than maxMergeOrder_, a second pass has to
      // be done. Eventually, [log(runCount) / log(maxMergeOrder) - 1]
      // intermediate passes are resulted.
      // -----------------------------------------------------------------
      CostScalar noOfIntPasses =
	log(noOfRuns.value()) / log(maxMergeOrder_.value()) - 1.;

      // -----------------------------------------------------------------
      // Sum of CPU costs for all merge sessions.
      // -----------------------------------------------------------------
      CostScalar h = log(maxMergeOrder_.value()) / log(2.);
      CostScalar cpuCostPerIntPass =
                     cpuCostCompareKeys_ * rsFludgeFactor_ * h * rowCount;
      cpuBK += cpuCostPerIntPass * noOfIntPasses;

      // -----------------------------------------------------------------
      // Summing up the effect of all its sessions, each pass involves
      // reading the whole table and writing the whole table once.
      // -----------------------------------------------------------------
      ioSeekBK += tableSize / ioBufferSize_ * csTwo * noOfIntPasses;
      ioByteBK += tableSize * csTwo * noOfIntPasses;

      // We have maxMergeOrder_ runs left after these intermediate passes.
      noOfRuns = maxMergeOrder_;
    }

    // -------------------------------------------------------------------
    // Since the tree node and buffers are pre-allocated to use the max
    // run generation and merge orders, we use up all memory available.
    // -------------------------------------------------------------------
    //j memBK = memoryLimit_;

    // -------------------------------------------------------------------
    // We could start producing rows in the final merge phase after at
    // least a buffer from each run is read into memory.
    // -------------------------------------------------------------------
    ioSeekBK += noOfRuns;
    ioByteBK += MINOF(noOfRuns * ioBufferSize_,tableSize);

    // -------------------------------------------------------------------
    // From this point onwards, sort can start producing rows, while it's
    // doing a final merge of its runs. We decide to continue computing
    // a per-probe LR cost and scale it up by the no of probes at the end.
    // -------------------------------------------------------------------

    // -------------------------------------------------------------------
    // CPU cost for the final merge phase. We will allocate a tree only
    // for the runs we have, which can be fewer than maxMergeOrder_.
    // -------------------------------------------------------------------
    CMPASSERT(noOfRuns <= maxMergeOrder_);
    h = MIN_ONE(log(noOfRuns.value()) / log(2.));
    cpuLR += cpuCostCompareKeys_ * rsFludgeFactor_ * h * rowCount;

    // -------------------------------------------------------------------
    // IO cost. The whole table is read exactly once. NB. We needn't read
    // the first buffer for each run since the cost has been charged to
    // BK cost already.
    // -------------------------------------------------------------------
    ioSeekLR += MAXOF( tableSize / ioBufferSize_ - noOfRuns, csZero );
    ioByteLR += MAXOF( tableSize - noOfRuns * ioBufferSize_, csZero );

    // -------------------------------------------------------------------
    // Memory used in the final merge phase.
    // -------------------------------------------------------------------
    //j memLR =
      //j noOfRuns * (ioBufferSize_ + treeNodeSize_) + ( csTwo * ioBufferSize_ );
  }

  // ---------------------------------------------------------------------
  // Sort receive part of the executor. Executor allocates result buffer
  // to hold the row it receives.
  // ---------------------------------------------------------------------
  cpuLR += (cpuCostAllocateBuffer_ * tableSize / exBufferSize_);
  cpuLR += (cpuCostAllocateTuple_ + cpuCostCopyResultRow_) * rowCount;

  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();
  const CostScalar ff_seeks = CURRSTMT_OPTDEFAULTS->getTimePerSeek();
  // BMO (external sort) involves both read and write, getTimePerSeqKb returns
  // time taken to read 1KB of data sequentially, since write takes more time
  // than read, we need some adjustment (read-write fudge factor) here.
  const CostScalar ff_seqIO =
          CostScalar(rwFudgeFactor_ * CURRSTMT_OPTDEFAULTS->getTimePerSeqKb());
  const CostScalar ff_local_msgs = CURRSTMT_OPTDEFAULTS->getTimePerLocalMsg();
  const CostScalar ff_kblocal_msgs = CURRSTMT_OPTDEFAULTS->getTimePerKbOfLocalMsg();


   SimpleCostVector cvBK (
    cpuBK * ff_cpu,
    ioSeekBK * ff_seeks + ioByteBK * ff_seqIO,
    ioSeekBK * ff_local_msgs + ioByteBK * ff_kblocal_msgs,
    csZero,
    noOfProbesPerStream_
    );

   SimpleCostVector cvFR (
    csZero,csZero,csZero,csZero,
    noOfProbesPerStream_
    );

  // ---------------------------------------------------------------------
  // Scale last row cost scalars up by the no of probes. Disk and memory
  // are not scaled up since they are reusable across probes.
  // ---------------------------------------------------------------------
  cpuLR *= noOfProbesPerStream_;
  ioSeekLR *= noOfProbesPerStream_;
  ioByteLR *= noOfProbesPerStream_;
//j  diskLR = diskBK;
   SimpleCostVector cvLR (
    cpuLR * ff_cpu,
    ioSeekLR * ff_seeks + ioByteLR * ff_seqIO,
    ioSeekLR * ff_local_msgs + ioByteLR * ff_kblocal_msgs,
    csZero,
    noOfProbesPerStream_
    );

  //----------------------------------------------------------------------
  // check if this is a partial cost; we reduce the cost by a factor 
  // influenced by maximum window size of the partial sort
  //----------------------------------------------------------------------
  Sort *sOp = (Sort *)op;
  if (! sOp->getPrefixSortKey().isEmpty())
  {
     ColStatDescList& myPartSortColStats = myLogProp_->colStats(); 
     ValueIdSet partSort = sOp->getPrefixSortKey();
  
     CollIndex length = sOp->getPrefixSortKey().entries(); 
     CostScalar maxWindowSize(myRowCount_)
                ,temp;

     for (CollIndex i = 0; i < length;  i++)
     {
        temp = 
         myPartSortColStats.getMaxOfMaxFreqOfCol(
            sOp->getPrefixSortKey().at(i).getItemExpr()->
                             removeInverseOrder()->getValueId()
                                                );

        if ((temp.isGreaterThanOne()) &&
           (maxWindowSize > temp))
          maxWindowSize = temp;
          
     }

     CostScalar sortAdjstFactor(csZero);
     CostScalar sortFactor = (ActiveSchemaDB()->getDefaults())\
                              .getAsDouble(PARTIAL_SORT_ADJST_FCTR);

     if (maxWindowSize.isGreaterThanOne()) 
        sortAdjstFactor = (maxWindowSize/myRowCount_) * sortFactor;

     if ( sortAdjstFactor == csZero)
        sortAdjstFactor = sortFactor;

      cvLR.scaleByValue(sortAdjstFactor);
      cvBK.scaleByValue(sortAdjstFactor);
      cvFR.scaleByValue(sortAdjstFactor); 
  }
  
  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"SORT::computeOperatorCost()\n");
    fprintf(pfp,"myRowCount=%g\n",myRowCount_.value());
    cvBK.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  Cost *costPtr = new STMTHEAP Cost( &cvFR,
				     &cvLR,
				     &cvBK,
				     cpuCount,
				     fragmentsPerCPU
				   );

#ifndef NDEBUG
  if ( printCost )
    {
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodSort::computeOperatorCostInternal()
// LCOV_EXCL_STOP
//<pb>

// ----QUICKSEARCH FOR GROUPBY............................................

/**********************************************************************/
/*                                                                    */
/*                       CostMethodGroupByAgg                         */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodGroupByAgg::cacheParameters().
// -----------------------------------------------------------------------
void CostMethodGroupByAgg::cacheParameters(RelExpr* op,
                                           const Context* myContext)
{
  // ---------------------------------------------------------------------
  // Comments on GroupBy estimated logical properties: Estimated Logical
  // Properties synthesis for GroupBy operators do not distinguish among
  // partial or full aggregation. As a result, if we split GroupBy's into
  // several levels of execution (right now, we can have at most three),
  // Estimated Logical Properties will always report that the GroupBy at
  // the lowest level does all the work (ie. a full grouping) so that all
  // the remaining GroupBy's above report the same input and output row
  // counts. This problem needs to be addressed, but it looks like some
  // work on the infrastructure needs to be done. Right now, the tentative
  // plan is to have physical costing do something to "estimate" actual
  // row counts in these cases. (See code in estimateParallelism()), but
  // too little can be done about it at this level.
  //
  // Added on 1/27/98: This is no longer true. Some recent code written
  // estimates the output row count of a partial group by
  // leaf by treating as if the node executes in ONE single DP2. Although
  // this sounds like an improvement, if the no of instances of DP2 where
  // this partial group by executes on is not ONE, the number of partial
  // groups still need to be re-estimated.
  // ---------------------------------------------------------------------

  CostMethod::cacheParameters(op,myContext);

  gb_ = (GroupByAgg *) op;

  // We will now get the Estimated Logical Properties of the child of
  // Materialize; also multipleCalls will tell us whether or not such
  // child gets called only once.
  Int32 multipleCalls;
  EstLogPropSharedPtr modInputLP;
  if (inLogProp_->getResultCardinality().isGreaterThanOne() &&
      CmpCommon::getDefault(COSTING_SHORTCUT_GROUPBY_FIX) != DF_ON)
  {
     // Executor doesn't materialize grouby operator, but optimizer assumes 
     // it does. Because of this we under estimate groupby cost when it 
     // is right side of NJ. This CQD is being used since it is already 
     // available and fix is generic, not specific to short cut grby.
     modInputLP = gb_->child(0).getGroupAttr()
         ->materializeInputLogProp(inLogProp_, &multipleCalls);
  }
  else
     modInputLP = inLogProp_;

  // if inputForSemiTSJ is set for inputEstLogProp it cannot be passed
  // below the filter so create a new EstLogProp with the flag set off to
  // pass to my children
  //
  EstLogPropSharedPtr copyInputEstProp;

  if (modInputLP->getInputForSemiTSJ() != EstLogProp::NOT_SEMI_TSJ)
    {
      copyInputEstProp = EstLogPropSharedPtr(new (HISTHEAP)
              EstLogProp(*modInputLP));
      copyInputEstProp->setInputForSemiTSJ(EstLogProp::NOT_SEMI_TSJ);
    }
  else
    {
      copyInputEstProp = modInputLP;
    }

  child0LogProp_ = gb_->child(0).outputLogProp(copyInputEstProp);
  myIntLogProp_ = gb_->getGroupAttr()->intermedOutputLogProp(copyInputEstProp);

  CMPASSERT(child0LogProp_ != NULL);
  child0RowCount_ = ( child0LogProp_->getResultCardinality() ).minCsOne();

  CMPASSERT(myIntLogProp_ != NULL);
  groupCount_ = ( myIntLogProp_->getResultCardinality() ).minCsOne();
  groupCount_ = MINOF( child0RowCount_, groupCount_ );

  const ValueIdSet& grbyVis = gb_->groupExpr();
  const ValueIdSet& aggrVis = gb_->aggregateExpr();
  const ValueIdSet& predVis = gb_->selectionPred();

  // Length in bytes of the group key and the aggregates.
  groupKeyLength_ = (grbyVis.isEmpty() ? 0 : grbyVis.getRowLength());
  aggregateLength_ = (aggrVis.isEmpty() ? 0 : aggrVis.getRowLength());

  // ---------------------------------------------------------------------
  // Per probe init cost. Subclasses should refine it.
  // ---------------------------------------------------------------------
  cpuCostPerProbeInit_ = csZero;

  // ---------------------------------------------------------------------
  // Cost to initialize a new group.
  // ---------------------------------------------------------------------
  cpuCostInitNewGroup_ =
                CostPrimitives::getBasicCostFactor(EX_OP_ALLOCATE_TUPLE) +
                CostPrimitives::cpuCostForCopySet(grbyVis) +
                CostPrimitives::cpuCostForCopyRow(aggregateLength_);

  // ---------------------------------------------------------------------
  // CPU cost for comparing the group keys.
  // ---------------------------------------------------------------------
  cpuCostCompareGroupKeys_ = CostPrimitives::cpuCostForCompare(grbyVis);

  // ---------------------------------------------------------------------
  // CPU cost for aggregating a row with an existing group.
  // ---------------------------------------------------------------------
  cpuCostAggrRowToGroup_ = CostPrimitives::cpuCostForAggrRow(aggrVis);

  // ---------------------------------------------------------------------
  // CPU cost for evaluating the having predicate on a group.
  // ---------------------------------------------------------------------
  cpuCostEvalHavingPred_ = CostPrimitives::cpuCostForEvalPred(predVis);

  // ---------------------------------------------------------------------
  // CPU cost to return a qualified row.
  // ---------------------------------------------------------------------
  cpuCostReturnRow_ = CostPrimitives::getBasicCostFactor(EX_OP_COPY_ATP);

}  // CostMethodGroupByAgg::cacheParameters().

//<pb>
// -----------------------------------------------------------------------
// CostMethodGroupByAgg::estimateDegreeOfParallelism().
//
// This method computes five parameters:
// CostMethod::countOfStreams_,
// CostMethod::noOfProbesPerStream_,
// CostMethodGroupByAgg::rowCountPerStream_,
// CostMethodGroupByAgg::groupCountPerStream_,
// CostMethodGroupByAgg::myRowCountPerStream_.
//
// Note that it doesn't make use of the base class's implementation of
// estimateDegreeOfParallelism(). It refines that implementation.
// -----------------------------------------------------------------------
void CostMethodGroupByAgg::estimateDegreeOfParallelism()
{
  const ValueIdSet& grbyVis = gb_->groupExpr();

  // rpp_ should not be NULL for a GroupBy operator.
  CMPASSERT(rpp_);
  const PartitioningFunction* pf = partFunc_;
  const PartitioningRequirement* pr = partReq_;

  EstLogPropSharedPtr groupEstLogProp;

  if (myIntLogProp_->getResultCardinality() < child0RowCount_)
  {
	groupEstLogProp = myIntLogProp_;
  }
  else
  {
	groupEstLogProp = child0LogProp_;
  }

  // We are asked to compute a scalar aggregate with no grouping columns.
  if(grbyVis.isEmpty())
  {
    // -------------------------------------------------------------------
    // The topmost scalar aggregate has to act as the final consolidator
    // which must therefore consume and produce only one stream, unless
    // it is doing a replicateNoBroadcast.
    // -------------------------------------------------------------------
    if(gb_->isNotAPartialGroupBy() OR gb_->isAPartialGroupByRoot())
    {
      // Compute the number of streams.
      if (pf != NULL)
      // ---------------------------------------------------------------
      // pf can exist when we're going up the tree. Grab the count of
      // partitions from there. This is an accurate count.
      // ---------------------------------------------------------------
        countOfStreams_ = pf->getCountOfPartitions();
      else if ((pr != NULL) AND
               (pr->isRequirementReplicateNoBroadcast()))
        countOfStreams_ = pr->getCountOfPartitions();
      else
        countOfStreams_ = 1;

      // If this operator is on the right leg of a parallel nested join,
      // then limit the countOfStreams_ by the number of probes, because
      // if we have fewer probes than the number of streams, then some
      // streams will be inactive.
      if ((partReq_ != NULL) AND
           partReq_->isRequirementReplicateNoBroadcast())
      {
        CostScalar tempCountOfStreams= MINOF(CostScalar(countOfStreams_),
                                             noOfProbes_.getCeiling());
        countOfStreams_ = Lng32(tempCountOfStreams.value());
      }

      // Compute the number of rows per stream.
      // Added on 1/27/98: The partial grouping effect has finally
      // been reflected in the child row count, so no need to
      // special case a partial group by root.
	  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
		  (partFunc_ != NULL) )
	  {
		// groupAttr are passed to compute the columns of partitioning key
		// which belong to this operator. For group by, the subtree of this 
		// node is same as that of the child
		// so we shall pass this nodes group attributes for the child too
		rowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                           countOfStreams_,
                           ga_,
                           countOfAvailableCPUs_);
		// For input logical properties, we shall use all columns of the partitioning
		// key. Passing NULL will ensure that
		noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                           countOfStreams_,
                           NULL,
                           countOfAvailableCPUs_);
	  }
	  else
	  {
		rowCountPerStream_ =  ( child0RowCount_ / countOfStreams_ ).minCsOne();
		noOfProbesPerStream_ = ( noOfProbes_/countOfStreams_ ).minCsOne();
	  }

      groupCountPerStream_ = noOfProbesPerStream_;
      myRowCountPerStream_ = noOfProbesPerStream_;

      // Just in case. We want to keep (rowCount >= groupCount).
      rowCountPerStream_ = MAXOF(rowCountPerStream_,groupCountPerStream_);
    }
    else if(gb_->isAPartialGroupByNonLeaf())
    {
      CMPASSERT(NOT rpp_->executeInDP2());

      if (pf != NULL)
        // ---------------------------------------------------------------
        // pf can exist when we're going up the tree. Grab the count of
        // partition from there. This is an accurate count.
        // ---------------------------------------------------------------
        countOfStreams_ = pf->getCountOfPartitions();
      else if ((pr != NULL) AND
               (pr->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS))
        countOfStreams_ = pr->getCountOfPartitions();
      else
        countOfStreams_ = rpp_->getCountOfPipelines();

      // If this operator is on the right leg of a parallel nested join,
      // then limit the countOfStreams_ by the number of probes, because
      // if we have fewer probes than the number of streams, then some
      // streams will be inactive.
      if ((partReq_ != NULL) AND
           partReq_->isRequirementReplicateNoBroadcast())
      {
        CostScalar tempCountOfStreams= MINOF(CostScalar(countOfStreams_),
                                             noOfProbes_.getCeiling());
        countOfStreams_ = Lng32(tempCountOfStreams.value());
      }

      // -----------------------------------------------------------------
      // Each probe generates a request for each instance of PartialGroup
      // ByNonLeaf.  This number should be at least one!!!
      // -----------------------------------------------------------------
	  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
		  (partFunc_ != NULL) )
	  {
		noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                        countOfStreams_,
                        NULL,
                        countOfAvailableCPUs_);
		// groupAttr are passed to compute the columns of partitioning key
		// which belong to this operator. For group by, the subtree of this 
		// node is same as that of the child
		// so we shall pass this nodes group attributes for the child too
		rowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                        countOfStreams_,
                        ga_,
                        countOfAvailableCPUs_);
	  }
	  else
	  {
		noOfProbesPerStream_ = ( noOfProbes_ / countOfStreams_ ).minCsOne();
		rowCountPerStream_ =  ( child0RowCount_ / countOfStreams_ ).minCsOne();
	  }

      // -----------------------------------------------------------------
      // At for group and row count estimates, each stream of a Partial
      // GroupByNonLeaf produces one group on one probe. Also, there
      // must be a PartialGroupByLeaf below it in DP2 to help with the
      // grouping. Each instance of these PartialGroupByLeaf is going to
      // produce one row on each probe. Thus there should be as many rows
      // to group as the no of partitions we have for the base table, and
      // we don't know how many partitions there are at this point.
      // -----------------------------------------------------------------
      groupCountPerStream_ = noOfProbesPerStream_;

      // -----------------------------------------------------------------
      // This is a definitely unknown. As everywhere else, assume the base
      // table is just partitioned into the same no of streams as we have
      // in the ESP, so that each instance of this PartialGBNonLeaf takes
      // one row only. It actually makes this PartialGBNonLeaf redundant,
      // which is actually what we might want to do since two levels of
      // groupby's seem to have introduced enough parallelism to do scalar
      // aggregation.
      //
      // Added on 1/27/98: The partial grouping effect has finally
      // been reflected in the child row count.
      // -----------------------------------------------------------------
      // rowCountPerStream_ = noOfProbesPerStream_;

      rowCountPerStream_ = MAXOF(rowCountPerStream_, groupCountPerStream_);

      // -----------------------------------------------------------------
      // Shouldn't have any having predicates since this is not the final
      // consolidator. In some rare cases it is possible to a having pred here
      // If there are two nested joins above this partial groupby and there is 
      // a pred between columns from the outer tables of both NJs then that
      // pred can end being pushed into this partial_gb_leaf. E.g. core/test002
      // Genesis_10_000222_6892_r3
      // -----------------------------------------------------------------
      myRowCountPerStream_ = groupCountPerStream_;
    }
    else // a partial group by leaf.
    {

      if (pf != NULL)
        // ---------------------------------------------------------------
        // pf can exist when we're going up the tree. Grab the count of
        // partition from there. This is an accurate count.
        // ---------------------------------------------------------------
        countOfStreams_ = pf->getCountOfPartitions();
      else if ((pr != NULL) AND
               (pr->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS))
        countOfStreams_ = pr->getCountOfPartitions();
      else
        // must underestimate, since we are on our way down.
        countOfStreams_ = rpp_->getCountOfPipelines();

      // If this operator is on the right leg of a parallel nested join,
      // then limit the countOfStreams_ by the number of probes, because
      // if we have fewer probes than the number of streams, then some
      // streams will be inactive.
      if ((partReq_ != NULL) AND
           partReq_->isRequirementReplicateNoBroadcast())
      {
        CostScalar tempCountOfStreams= MINOF(CostScalar(countOfStreams_),
                                             noOfProbes_.getCeiling());
        countOfStreams_ = Lng32(tempCountOfStreams.value());
      }

      // -----------------------------------------------------------------
      // When the leaf is pushed down to DP2. Each leaf is responsible
      // for grouping in one partition of the table.
      // -----------------------------------------------------------------
	  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
		  (partFunc_ != NULL) )
	  {
		noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                              countOfStreams_,
                              NULL,
                              countOfAvailableCPUs_);
		// groupAttr are passed to compute the columns of partitioning key
		// which belong to this operator. For group by, the subtree of this 
		// node is same as that of the child
		// so we shall pass this nodes group attributes for the child too
		rowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                              countOfStreams_,
                              ga_,
                              countOfAvailableCPUs_);
	  }
	  else
	  {
		noOfProbesPerStream_ = ( noOfProbes_ / countOfStreams_ ).minCsOne();
		rowCountPerStream_ = ( child0RowCount_ / countOfStreams_ ).minCsOne();
	  }

      // -----------------------------------------------------------------
      // For scalar aggregation, the leaf can do
      // full-grouping and give just one row for each table partition on
      // each probe, since not much memory is involved. Assume row counts
      // are distributed evenly across the all partitions.
      // -----------------------------------------------------------------
      groupCountPerStream_ = noOfProbesPerStream_;

      // Just in case. We want to keep (rowCount >= groupCount).
      rowCountPerStream_ = MAXOF(rowCountPerStream_,groupCountPerStream_);

      // -----------------------------------------------------------------
      // Shouldn't have any having predicates since this is not the final
      // consolidator. In some rare cases it is possible to a having pred here
      // If there are two nested joins above this partial groupby and there is 
      // a pred between columns from the outer tables of both NJs then that
      // pred can end being pushed into this partial_gb_leaf. E.g. core/test002
      // Genesis_10_000222_6892_r3
      // -----------------------------------------------------------------
      //CMPASSERT(predVis.isEmpty());
      myRowCountPerStream_ = groupCountPerStream_;

    } // endif(gb_->isNotAPartialGroupBy() OR gb_->isAPartialGroupByRoot())
  }
  // DIFFICULT case: We have some grouping columns.
  else
  {
    if (pf != NULL)
      // ---------------------------------------------------------------
      // pf can exist when we're going up the tree. Grab the count of
      // partition from there. This is an accurate count.
      // ---------------------------------------------------------------
      countOfStreams_ = pf->getCountOfPartitions();
    else if ((pr != NULL) AND
             (pr->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS))
      countOfStreams_ = pr->getCountOfPartitions();
    else
      countOfStreams_ = -1;

    // If this operator is on the right leg of a parallel nested join,
    // then limit the countOfStreams_ by the number of probes, because
    // if we have fewer probes than the number of streams, then some
    // streams will be inactive.
    if ((partReq_ != NULL) AND
         partReq_->isRequirementReplicateNoBroadcast())
    {
      CostScalar tempCountOfStreams= MINOF(CostScalar(countOfStreams_),
                                           noOfProbes_.getCeiling());
      countOfStreams_ = Lng32(tempCountOfStreams.value());
    }

    // -----------------------------------------------------------------
    // Required to produce exactly one partition. No choice.
    // -----------------------------------------------------------------
    if(countOfStreams_ == 1)
    {
      noOfProbesPerStream_ = noOfProbes_;
      groupCountPerStream_ = groupCount_;

      // ---------------------------------------------------------------
      // When we're going down the tree, the execution requirements
      // ensure that this requirement to produce one stream is not
      // compatible with a PartialGroupByNonLeaf.
      // ---------------------------------------------------------------
      if(context_->getPlan()->getPhysicalProperty() == NULL)
      {
        CMPASSERT(NOT gb_->isAPartialGroupByNonLeaf());
      }

      // For full group by, just take all the rows and groups.
      if(gb_->isNotAPartialGroupBy())
      {
        rowCountPerStream_ = child0RowCount_;
        myRowCountPerStream_ = myRowCount_;
      }
      else if(gb_->isAPartialGroupByRoot())
      // ---------------------------------------------------------------
      // For PartialGroupByRoot, some grouping have been done by other
      // partial group by operator(s) down the tree (but again, we don't
      // know how much).
      // There is an upper bound on the no of rows this root gets. Each
      // group can be present in the stream produced by each PartialGB
      // below, resulting in a total no of groupCount_ * countOfStreams_
      // rows. Also, we couldn't have more rows than before any partial
      // grouping is done. But we might have as little as groupCount_
      // rows if my child is partitioned by my grouping columns.
      // ---------------------------------------------------------------
      {
        // -------------------------------------------------------------
        // Added on 1/27/98: Now we know that the row count we directly
        // get from my child has been estimated with the partial grouping
        // effect.
        // -------------------------------------------------------------
        // rowCountPerStream_ = groupCount_ * countOfStreams_;
        rowCountPerStream_ = child0RowCount_;
        myRowCountPerStream_ = myRowCount_;
      }
      else // a leaf executing in one stream.
      {
        rowCountPerStream_ = child0RowCount_;
        myRowCountPerStream_ = myRowCount_;
      }
    }
    else if (countOfStreams_ >= 2)
    // I'm required to produce partitioned streams.
    {
      if(NOT rpp_->executeInDP2())
      {
        // -------------------------------------------------------------
        // The rowcount for a full groupby is correct. So we are going
        // to assume even split across the streams.
        // -------------------------------------------------------------
        if(gb_->isNotAPartialGroupBy())
        {
		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   NULL,
                                   countOfAvailableCPUs_);
			groupCountPerStream_ = groupEstLogProp->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   ga_,
                                   countOfAvailableCPUs_);
			myRowCountPerStream_ = myLogProp_->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   ga_,
                                   countOfAvailableCPUs_);
			// groupAttr are passed to compute the columns of partitioning key
			// which belong to this operator. For group by, the subtree of this 
			// node is same as that of the child
			// so we shall pass this nodes group attributes for the child too
			rowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   ga_,
                                   countOfAvailableCPUs_);
		  }
		  else
		  {
			noOfProbesPerStream_ = (noOfProbes_/countOfStreams_).minCsOne();
			groupCountPerStream_ = (groupCount_/countOfStreams_).minCsOne();
			myRowCountPerStream_ = (myRowCount_/countOfStreams_).minCsOne();
			rowCountPerStream_   = (child0RowCount_/countOfStreams_).minCsOne();
		  }
        }
        else
        // -------------------------------------------------------------
        // We can have a PartialGroupByRoot, PartialGroupByNonLeaf, or
        // a PartialGroupByLeaf. In all of these cases, the rowcount
        // we have is not a good estimate. They are just the same as
        // groupCount, and we can't really do much about it at this point.
        // -------------------------------------------------------------
        if(gb_->isAPartialGroupByRoot())
        {
		  CostScalar rowCount;

		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                                 countOfStreams_,
                                 NULL,
                                 countOfAvailableCPUs_);
			groupCountPerStream_ = groupEstLogProp->getCardOfBusiestStream(partFunc_,
                                 countOfStreams_,
                                 ga_,
                                 countOfAvailableCPUs_);
			myRowCountPerStream_ = myLogProp_->getCardOfBusiestStream(partFunc_,
                                 countOfStreams_,
                                 ga_,
                                 countOfAvailableCPUs_);
			// groupAttr are passed to compute the columns of partitioning key
			// which belong to this operator. For group by, the subtree of this 
			// node is same as that of the child
			// so we shall pass this nodes group attributes for the child too
			rowCount = child0LogProp_->getCardOfBusiestStream(partFunc_,
                                 countOfStreams_,
                                 ga_,
                                 countOfAvailableCPUs_);
		  }
		  else
		  {
			noOfProbesPerStream_ = (noOfProbes_/countOfStreams_).minCsOne();
			groupCountPerStream_ = (groupCount_/countOfStreams_).minCsOne();
			myRowCountPerStream_ = (myRowCount_/countOfStreams_).minCsOne();
			rowCount   = (child0RowCount_/countOfStreams_).minCsOne();
		  }
          
          // -----------------------------------------------------------
          // Assume each of the partial grouping operator has all the
          // groups present at each stream.
          // -----------------------------------------------------------
          // rowCountPerStream_ = MIN_ONE_CS(groupCount_);

          rowCountPerStream_ = MAXOF(rowCount, groupCountPerStream_);
        }
        else
        {
		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                               countOfStreams_,
                               NULL,
                               countOfAvailableCPUs_);
			// groupAttr are passed to compute the columns of partitioning key
			// which belong to this operator. For group by, the subtree of this 
			// node is same as that of the child
			// so we shall pass this nodes group attributes for the child too
			rowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                               countOfStreams_,
                               ga_,
                               countOfAvailableCPUs_);
		  }
		  else
		  {
			noOfProbesPerStream_ = (noOfProbes_/countOfStreams_).minCsOne();
			rowCountPerStream_   = (child0RowCount_/countOfStreams_).minCsOne();
		  }
          
		  groupCountPerStream_ = (groupCount_).minCsOne();
          myRowCountPerStream_ = (myRowCount_).minCsOne();

          // groupCountPerStream_ should be less or equal to rowCountPerStream_
          groupCountPerStream_ = MINOF(groupCountPerStream_, rowCountPerStream_) ;
        }
      }
      else // required to execute in DP2.
      {
        // -------------------------------------------------------------
        // Update on 11/19/97: Check whether we have information on the
        // partitioning columns, which is the case when we are going up
        // the tree. This information could help us improve our group
        // count estimate a lot.
        // -------------------------------------------------------------
        ValueIdSet partKey;
        if (pf != NULL)
          partKey = pf->getPartitioningKey();
        else
          partKey = pr->getPartitioningKey();

        if(NOT partKey.isEmpty())
        {
          // -----------------------------------------------------------
          // If partitioning key is a subset of the grouping columns,
          // the rows belonging to the same group must be assigned to
          // the same partition. Thus, estimate of groupCountPerStream_
          // is more like groupCount_ / countOfStream_. In the other
          // case, it's more like just groupCount_ if the same group
          // can be present in each stream.
          // -----------------------------------------------------------
          partKey.remove(gb_->groupExpr());
          if(partKey.isEmpty())
          {
            noOfProbesPerStream_ = (noOfProbes_/countOfStreams_).minCsOne();
            rowCountPerStream_   = (child0RowCount_/countOfStreams_).minCsOne();
            groupCountPerStream_ = (groupCount_/countOfStreams_).minCsOne();
            myRowCountPerStream_ = (myRowCount_/countOfStreams_).minCsOne();
          }
          else
          {
			if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
				(partFunc_ != NULL) )
			{
			  noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                                  countOfStreams_,
                                  NULL,
                                  countOfAvailableCPUs_);
			  // groupAttr are passed to compute the columns of partitioning key
			  // which belong to this operator. For group by, the subtree of this 
			  // node is same as that of the child
			  // so we shall pass this nodes group attributes for the child too
			  rowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                                  countOfStreams_,
                                  ga_,
                                  countOfAvailableCPUs_);
			}
			else
			{
			  noOfProbesPerStream_ = (noOfProbes_/countOfStreams_).minCsOne();
			  rowCountPerStream_   = (child0RowCount_/countOfStreams_).minCsOne();
			}

            groupCountPerStream_ = MINOF(groupCount_,rowCountPerStream_);
            myRowCountPerStream_ = MINOF(myRowCount_,rowCountPerStream_);
          }
        }
        else
        {
          // -------------------------------------------------------------
          // The rowcounts at the leaf are actually right if full grouping
          // can really be done there. Specific implementations are going
          // to interpret the group count as such when the operator runs
          // in DP2 and then considers whether it has the memory to do a
          // full grouping.
          // -------------------------------------------------------------
          
		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                                  countOfStreams_,
                                  NULL,
                                  countOfAvailableCPUs_);
			// groupAttr are passed to compute the columns of partitioning key
			// which belong to this operator. For group by, the subtree of this 
			// node is same as that of the child
			// so we shall pass this nodes group attributes for the child too
			rowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                                  countOfStreams_,
                                  ga_,
                                  countOfAvailableCPUs_);
			groupCountPerStream_ = groupEstLogProp->getCardOfBusiestStream(partFunc_,
                                  countOfStreams_,
                                  ga_,
                                  countOfAvailableCPUs_);
			myRowCountPerStream_ = myLogProp_->getCardOfBusiestStream(partFunc_,
                                  countOfStreams_,
                                  ga_,
                                  countOfAvailableCPUs_);
		  }
		  else
		  {
			noOfProbesPerStream_ = (noOfProbes_/countOfStreams_).minCsOne();
			rowCountPerStream_   = (child0RowCount_/countOfStreams_).minCsOne();
			groupCountPerStream_ = (groupCount_/countOfStreams_).minCsOne();
			myRowCountPerStream_ = (myRowCount_/countOfStreams_).minCsOne();
		  }
        }
      }
    } // endif(countOfStreams_ >= 2)
    else
    // -------------------------------------------------------------------
    // On the way down and no partitioning requirement specified, or the
    // partitioning requirement did not specify a number of partitions.
    // The operator can choose its own degree of parallelism.
    // -------------------------------------------------------------------
    {
      if(NOT rpp_->executeInDP2())
      {
        // must underestimate, since we are on our way down
        countOfStreams_ = rpp_->getCountOfPipelines();

        // ---------------------------------------------------------------
        // The rowcount for a full groupby is correct. So we are going to
        // assume even split across the streams.
        // ---------------------------------------------------------------
        if(gb_->isNotAPartialGroupBy())
        {
		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   NULL,
                                   countOfAvailableCPUs_);
			groupCountPerStream_ = groupEstLogProp->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   ga_,
                                   countOfAvailableCPUs_);
			myRowCountPerStream_ = myLogProp_->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   ga_,
                                   countOfAvailableCPUs_);
			// groupAttr are passed to compute the columns of partitioning key
			// which belong to this operator. For group by, the subtree of this 
			// node is same as that of the child
			// so we shall pass this nodes group attributes for the child too
			rowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   ga_,
                                   countOfAvailableCPUs_);
		  }
		  else
		  {
			noOfProbesPerStream_ = (noOfProbes_/countOfStreams_).minCsOne();
			groupCountPerStream_ = (groupCount_/countOfStreams_).minCsOne();
			myRowCountPerStream_ = (myRowCount_/countOfStreams_).minCsOne();
			rowCountPerStream_   = (child0RowCount_/countOfStreams_).minCsOne();
		  }
        }
        else
        // ---------------------------------------------------------------
        // We can have a PartialGroupByRoot, PartialGroupByNonLeaf, or
        // a PartialGroupByLeaf. In all of these cases, the rowcount
        // we have is not a good estimate. They are just the same as
        // groupCount, and we can't really do much about it at this point.
        // ---------------------------------------------------------------
        if(gb_->isAPartialGroupByRoot())
        {
		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                                countOfStreams_,
                                NULL,
                                countOfAvailableCPUs_);
			groupCountPerStream_ = groupEstLogProp->getCardOfBusiestStream(partFunc_,
                                countOfStreams_,
                                ga_,
                                countOfAvailableCPUs_);
			myRowCountPerStream_ = myLogProp_->getCardOfBusiestStream(partFunc_,
                                countOfStreams_,
                                ga_,
                                countOfAvailableCPUs_);
		  }
		  else
		  {
			noOfProbesPerStream_ = (noOfProbes_/countOfStreams_).minCsOne();
			groupCountPerStream_ = (groupCount_/countOfStreams_).minCsOne();
			myRowCountPerStream_ = (myRowCount_/countOfStreams_).minCsOne();
		  }

          // -------------------------------------------------------------
          // Assume each of the partial grouping operator has all the
          // groups present at each stream.
          // -------------------------------------------------------------
          rowCountPerStream_ = (groupCount_).minCsOne();
        } // gb_->isAPartialGroupByRoot
        else
        {
		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                                  countOfStreams_,
                                  NULL,
                                  countOfAvailableCPUs_);
			// groupAttr are passed to compute the columns of partitioning key
			// which belong to this operator. For group by, the subtree of this 
			// node is same as that of the child
			// so we shall pass this nodes group attributes for the child too
			rowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                                  countOfStreams_,
                                  ga_,
                                  countOfAvailableCPUs_);
		  }
		  else
		  {
			noOfProbesPerStream_ = (noOfProbes_/countOfStreams_).minCsOne();
			rowCountPerStream_   = (child0RowCount_/countOfStreams_).minCsOne();
		  }
          
		  groupCountPerStream_ = (groupCount_).minCsOne();
          
		  myRowCountPerStream_ = (myRowCount_).minCsOne();
		  
          // groupCountPerStream_ should be less or equal to rowCountPerStream_
          groupCountPerStream_ = MINOF(groupCountPerStream_, rowCountPerStream_) ;
        }
      }
      else
      {
        // ---------------------------------------------------------------
        // We're going down, we were not given a part. requirement,
        // and we are in DP2. Any group by operator pushed
        // down to DP2 is just going to follow whichever partitioning
        // is inherent in the physical table. So, just guess that the
        // table is partitioned max pipelines ways. On the way
        // back up we will have the synthesized partitioning function
        // and so we will not come here.
        //
        // ---------------------------------------------------------------
        countOfStreams_ = rpp_->getCountOfPipelines();

        // ---------------------------------------------------------------
        // The rowcounts at the leaf are actually right if full grouping
        // can really be done there. Specific implementations are going
        // to interpret the group count as such when the operator runs
        // in DP2 and then considers whether it has the memory to do a
        // full grouping.
        // ---------------------------------------------------------------
		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			noOfProbesPerStream_ = inLogProp_->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   NULL,
                                   countOfAvailableCPUs_);
			groupCountPerStream_ = groupEstLogProp->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   ga_,
                                   countOfAvailableCPUs_);
			myRowCountPerStream_ = myLogProp_->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   ga_,
                                   countOfAvailableCPUs_);
			// groupAttr are passed to compute the columns of partitioning key
			// which belong to this operator. For group by, the subtree of this 
			// node is same as that of the child
			// so we shall pass this nodes group attributes for the child too
			rowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                                   countOfStreams_,
                                   ga_,
                                   countOfAvailableCPUs_);
		  }
		  else
		  {
			noOfProbesPerStream_ = (noOfProbes_/countOfStreams_).minCsOne();
			groupCountPerStream_ = (groupCount_/countOfStreams_).minCsOne();
			myRowCountPerStream_ = (myRowCount_/countOfStreams_).minCsOne();
			rowCountPerStream_   = (child0RowCount_/countOfStreams_).minCsOne();
		  }
      }
    } // end if on way down and no number of partitions requirement
  } // endif(grbyVis.isEmpty())


#ifndef NDEBUG
  // debug
  CostScalar delta = rowCountPerStream_ - groupCountPerStream_;
  if ((delta < 0) && (CURRSTMT_OPTGLOBALS->warningGiven == FALSE))
  {
   *CmpCommon::diags()
    << DgSqlCode(CMP_OPT_WARN_FROM_DCMPASSERT)
    << DgString0("delta < 0");
   CURRSTMT_OPTGLOBALS->warningGiven = TRUE;
  }
#endif
  // recover in release
  if (groupCountPerStream_ > rowCountPerStream_)
    groupCountPerStream_ = rowCountPerStream_;

}  // CostMethodGroupByAgg::estimateDegOfParallelism().

void CostMethodGroupByAgg::cleanUp()
{
  child0LogProp_ = 0;
  myIntLogProp_ = 0;

  CostMethod::cleanUp();
}

//<pb>

// ----QUICKSEARCH FOR SGB................................................

/**********************************************************************/
/*                                                                    */
/*                       CostMethodSortGroupBy                        */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodSortGroupBy::computeOperatorCostInternal().
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu -- OCM code
Cost*
CostMethodSortGroupBy::computeOperatorCostInternal(RelExpr* op,
                                                   const Context* myContext,
                                                   Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // CostScalars to be computed.
  // ---------------------------------------------------------------------
  CostScalar cpuFR(csZero), cpuLR(csZero), mem(csZero);

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

 
  if (CmpCommon::getDefault(COMP_BOOL_86) == DF_ON)
  {
     // reset cpuCopyCostRow similar to hash grpby
     cpuCostReturnRow_ = CostPrimitives::cpuCostForCopyRow(
                                      groupKeyLength_ + aggregateLength_);
  }

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // Added on 7/16/97: If we're on our way down the tree and this group
  // by is being considered for execution in DP2, generate a zero cost
  // object first and come back to cost it later when we're on our way up.
  // Set the count of streams to an invalid value (0) to force us to
  // recost on the way back up.
  // ---------------------------------------------------------------------
  if(rpp_->executeInDP2() AND
    (NOT context_->getPlan()->getPhysicalProperty()))
  {
    countOfStreams = 0;
    return generateZeroCostObject();
  }

  // ---------------------------------------------------------------------
  // Make sure rowcount is at least group count to prevent absurdity in
  // results.
  // ---------------------------------------------------------------------
  rowCountPerStream_ = MAXOF(rowCountPerStream_,groupCountPerStream_);

  // ---------------------------------------------------------------------
  // Per probe initialization.
  // ---------------------------------------------------------------------
  cpuFR += cpuCostPerProbeInit_;
  cpuLR += cpuCostPerProbeInit_ * noOfProbesPerStream_;

  // ---------------------------------------------------------------------
  // Cost to initialize a new group.
  // ---------------------------------------------------------------------
  cpuFR += cpuCostInitNewGroup_;
  cpuLR += cpuCostInitNewGroup_ * groupCountPerStream_;

  // ---------------------------------------------------------------------
  // CPU cost for comparing the group keys.
  // ---------------------------------------------------------------------
  cpuFR += cpuCostCompareGroupKeys_ *
                  MINOF(child0RowCount_ / groupCount_,rowCountPerStream_);
  cpuLR += cpuCostCompareGroupKeys_ * rowCountPerStream_;

  // ---------------------------------------------------------------------
  // CPU cost for aggregating a row with an existing group.
  // ---------------------------------------------------------------------
  cpuFR += cpuCostAggrRowToGroup_ * MINOF(child0RowCount_ / groupCount_,
                               rowCountPerStream_ - groupCountPerStream_);
  cpuLR +=
     cpuCostAggrRowToGroup_ * (rowCountPerStream_ - groupCountPerStream_);

  // ---------------------------------------------------------------------
  // CPU cost for evaluating the having predicate on a group.
  // ---------------------------------------------------------------------
  cpuFR += cpuCostEvalHavingPred_ * MINOF(groupCount_ / myRowCount_,
                                                    groupCountPerStream_);
  cpuLR += cpuCostEvalHavingPred_ * groupCountPerStream_;

  // ---------------------------------------------------------------------
  // CPU cost to return a qualified row.
  // ---------------------------------------------------------------------
  cpuFR += cpuCostReturnRow_;
  cpuLR += cpuCostReturnRow_ * myRowCountPerStream_;

  // ---------------------------------------------------------------------
  // Buffers initially allocated to keep the result.
  // ---------------------------------------------------------------------
  mem = CostScalar(bufferCount_ * bufferSize_);

  // ---------------------------------------------------------------------
  // Synthesize the simple cost vectors.
  // ---------------------------------------------------------------------

  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();
	CostScalar cpuForcvFR;

  cpuForcvFR = cpuFR/noOfProbesPerStream_ * ff_cpu;

  SimpleCostVector cvFR (
    cpuForcvFR,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  SimpleCostVector cvLR (
    cpuLR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"SORTGROUPBY::computeOperatorCost()\n");
    fprintf(pfp,"child0RowCount=%g,groupCount=%g,myRowCount=%g\n",
            child0RowCount_.value(),groupCount_.value(),myRowCount_.value());
    cvFR.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return the cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  // If this is a partial Group By leaf in an ESP adjust it's cost
  GroupByAgg * groupByNode = (GroupByAgg *) op;

  PhysicalProperty * sppForMe = (PhysicalProperty *) myContext->
                                  getPlan()->getPhysicalProperty();

  if(groupByNode->isAPartialGroupByLeaf() &&
     ((sppForMe && sppForMe->executeInESPOnly()) ||
      (CmpCommon::getDefault(COMP_BOOL_186) == DF_ON)))
  {
    // don't adjust if Group Columns contain partition columns.
    const PartitioningFunction* const myPartFunc =
      sppForMe->getPartitioningFunction();

    ValueIdSet myPartKey = myPartFunc->getPartitioningKey();

    ValueIdSet myGroupingColumns = groupByNode->groupExpr();

    NABoolean myGroupingMatchesPartitioning = FALSE;

    if (myPartKey.entries() &&
        myGroupingColumns.contains(myPartKey))
      myGroupingMatchesPartitioning = TRUE;

    if (!myGroupingMatchesPartitioning)
    {
      CostScalar grpByAdjFactor = (ActiveSchemaDB()->getDefaults())\
                                    .getAsDouble(ROBUST_PAR_GRPBY_LEAF_FCTR);
      cvLR *= grpByAdjFactor;
      cvFR *= grpByAdjFactor;
    }
  }

  Cost *costPtr = new STMTHEAP Cost( &cvFR,
				     &cvLR,
				     NULL,
				     cpuCount,
				     fragmentsPerCPU
				   );

#ifndef NDEBUG

  if ( printCost )
    {
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodSortGroupBy::computeOperatorCostInternal().
// LCOV_EXCL_STOP
//<pb>

// ----QUICKSEARCH FOR HGB................................................

/**********************************************************************/
/*                                                                    */
/*                       CostMethodHashGroupBy                        */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodHashGroupBy::cacheParameters().
// -----------------------------------------------------------------------
void CostMethodHashGroupBy::cacheParameters(RelExpr* op,
                                            const Context* myContext)
{
  CostMethodGroupByAgg::cacheParameters(op,myContext);

  // Cost to compute the hash value for a row.
  cpuCostHashRow_ = CostPrimitives::cpuCostForHash(gb_->groupExpr());

  // HGB needs to copy the row from local buffer to result buffer.
  cpuCostReturnRow_ = CostPrimitives::cpuCostForCopyRow(
                                      groupKeyLength_ + aggregateLength_);

  // Besides regular stuffs, we also have to insert the row to the chain.
  cpuCostInitNewGroup_ += cpuCostInsertRowToChain_;

  // ---------------------------------------------------------------------
  // Since partial groups may result from spilling, we might have to aggr
  // a partial group into another in subsequent passes. The cost to aggr
  // a row to a group and a partial group to another may be different. We
  // charge the same cost for the time being.
  // ---------------------------------------------------------------------
  cpuCostAggrGroupToGroup_ = cpuCostAggrRowToGroup_;

  // Length of the group with aggregates and the hash table overhead.
  extGroupLength_ =
                  groupKeyLength_ + aggregateLength_ + hashedRowOverhead_;

  // ---------------------------------------------------------------------
  // Added on 1/27/98: Now groupCount_ has been estimated with some actual
  // partial grouping effect. However, it has been estimated assuming the
  // table has only one partition and the partial grouping is done in a
  // single DP2. Here, we need to turn this groupCount_ back to what it
  // was before, so that the original logic in computePartialGroupByLeafCost()
  // can correctly handle it. The formulae for doing so are given below:
  //
  //  Gfull = no of full groups. also final value of groupCount_.
  //
  //  Gpart = no of partial groups as estimated by new EstLogProp
  //          code. ie. assuming partial grouping in ONE DP2.
  //          also the initial value of groupCount_.
  //
  //  R     = total no of rows in the table (all partitions).
  //
  //  Mgp   = memory need to store a group.
  //
  //  Mdp2  = max memory DP2 can allow this partial groupby to use.
  //
  //  Gdp2  = Mdp2 / Mgp (no of groups accommodated in one DP2)
  //
  //  Gpart = Gdp2 + (R - Gdp2 * (R / Gfull)).
  //
  //  ===>  Gfull = (R * Gdp2) / (R + Gdp2 - Gpart)
  // ---------------------------------------------------------------------

  if( myContext->getReqdPhysicalProperty()->executeInDP2() &&
      (CmpCommon::getDefault(COMP_BOOL_52) == DF_ON)
    )
  {
    CostScalar Gdp2 =
        (memoryLimitInDP2_ * csOneKiloBytes * csOneKiloBytes / extGroupLength_);

    // case where DP2 cannot accommodate all groups.
    if ( Gdp2 < groupCount_ )
    {
      // Changed the order of arithmetic operators in the following expression
      // Initially we were first adding Gdp2 to child0RowCount_ and then
      // subtracting groupCount_ from the result. If the difference
      // was smaller than COSTSCALAR_EPSILON, then the result returned would be zero.
      // This was causing an assertion later while estimating parallelism.
      // This is not the required behaviour in this case. As a temporary fix
      // I first subtract the groupCount_ and then add Gdp2 to it. For a better
      // fix we should revisit, costScalar subtraction to see, why is there a
      // need to return zero, if the difference is smaller than epsilon.
      CostScalar denominator =
          (child0RowCount_ - groupCount_ + Gdp2 ).minCsOne();
      //    MIN_ONE_CS(child0RowCount_ + Gdp2 - groupCount_);

      groupCount_ = child0RowCount_ / denominator * Gdp2;
    }
  }

}  // CostMethodHashGroupBy::cacheParameters().
//<pb>
// -----------------------------------------------------------------------
// CostMethodHashGroupBy::deriveParameters().
//
// Assume that both cacheParameters() and estimateDegreeOfParallelism()
// have been called. Derive three parameters: rowCount_,groupCount_ and
// noOfClusters_, which together forms an initial working set of
// parameters for computePassCost().
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu -- OCM code
void CostMethodHashGroupBy::deriveParameters()
{
  // ---------------------------------------------------------------------
  // These metrics are computed on a per-stream per-probe basis.
  // ---------------------------------------------------------------------
  CostScalar rowCount =
    (rowCountPerStream_ / noOfProbesPerStream_).minCsOne();
  CostScalar groupCount =
    (groupCountPerStream_ / noOfProbesPerStream_).minCsOne();
  CostScalar groupedTableSize = groupCount / csOneKiloBytes * extGroupLength_;

  // No memory limit or whole grouped table enough to fit in main memory.
  // Old behaviour
  if ( (CmpCommon::getDefault(COMP_BOOL_52) == DF_ON) &&
       (NOT isBMO_  || groupedTableSize <= memoryLimit_)
     )
  {
    noOfClustersToBeAllocated_ = 1;
  }
  else
  {
    // Use estimates to compute how many clusters we allocate.
    noOfClustersToBeAllocated_ =
                    computeCountOfClusters(memoryLimit_,groupedTableSize);

   // We should have allocated > 1 clusters if table doesn't fit.
   if ( CmpCommon::getDefault(COMP_BOOL_52) == DF_ON)
     CMPASSERT(noOfClustersToBeAllocated_ > 1);
  }


  // To begin with, there is one cluster with all rows in the table.
  noOfClustersToBeProcessed_ = 1;
  groupCountPerCluster_ = groupCount;
  rowCountPerCluster_ = rowCount;

}  // CostMethodHashGroupBy::deriveParameters().
//<pb>
//==============================================================================
//  Compute cost for doing hash grouping without overflow.  This is the
// algorithm currently used for the executor Hash Group By operator when it
// executes in DP2.
// Executor behavior from R2.2 has changed. Now Partial_Hash_Groupby_leaf can 
// happen in ESP. Partial Groupby Leaf never overflows, its job is to reduce
// the input going to the root. It uses fixed size memory:
// 1. 100 MB if in an ESP, controlled by EXE_MEMORY_FOR_PARTIALHGB_IN_MB.  
// 2. 1000 groups if in DP2, controlled by MAX_DP2_HASHBY_GROUP.
//
//  We can keep on allocating buffers to accommodate new groups as far as
// all the memory available has not been used up. When we see a new group
// but have no more space for it, the row is simply returned. Thus, only
// partial groups may result.
//
// Input:
//  none
//
// Output:
//  cpuFR           -- CPU usage to produce first row after blocking phase
//                      completes.

//  cpuLR           -- CPU usage to produce all rows after blocking phase
//                      completes.

//  cpuBK           -- CPU usage during blocking phase.

//  groupingFactor  -- Percentage of groups which fit completely in memory.
//
// Return:
//  none
//
//==============================================================================
void
CostMethodHashGroupBy::computePartialGroupByLeafCost(
                                              CostScalar& cpuFR,
                                              CostScalar& cpuLR,
                                              CostScalar& cpuBK,
                                              CostScalar& groupingFactor)
{
  CostScalar rowCount   = rowCountPerStream_;
  CostScalar groupCount = groupCountPerStream_;

  //--------------------------------------
  //  Guard against potential abnormalies.
  //--------------------------------------
  rowCount   = (rowCount).minCsOne();
  groupCount = (groupCount).minCsOne();

  //-----------------------------------------------------------------
  //  Number of groups the dp2 groupby can accomodate is at most 1000;
  // The CQD MAX_DP2_HASHBY_GROUPS has this as the default value.
  //
  // how many rows are consumed by the group by operator depends on UEC,
  // skew etc.
  // For now we assume uniform distribution among the distinct values given
  // by the UEC
  //-----------------------------------------------------------------

  CostScalar groupCountInMemory = csZero;

  if(rpp_->executeInDP2())
  {
    groupCountInMemory = 
      ActiveSchemaDB()->getDefaults().getAsLong(MAX_DP2_HASHBY_GROUPS);
  }
  else
  {
    // get Esp groupby memory limit (default 100MB).
    CostScalar memorySizeForEsp = ActiveSchemaDB()->
               getDefaults().getAsLong(EXE_MEMORY_FOR_PARTIALHGB_IN_MB);
    // convert MB into bytes, divide by groupLength to get partial groupCount
    groupCountInMemory = (memorySizeForEsp * 1048576) / extGroupLength_;
  }

  groupCountInMemory            =
      MINOF(groupCountInMemory,groupCount);
  mem_                          =
      groupCountInMemory * extGroupLength_ / csOneKiloBytes;

  //fudge factor for CPU
  const CostScalar ff_cpu =
      CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();


  //-----------------------------------------------
  //  The hash table is probed for every input row.
  //-----------------------------------------------
  CostScalar cpu = cpuCostPositionHashTableCursor_ * rowCount * ff_cpu;

  //---------------------------------------------------------------
  //  Charge an initialize new group cost for each group in memory.
  //---------------------------------------------------------------
  cpu += cpuCostInitNewGroup_ * groupCountInMemory * ff_cpu;

  //-----------------------------------------
  //  Can't have more groups than input rows.
  //-----------------------------------------
  CMPASSERT(rowCount >= groupCount);

  //----------------------------------
  // Average number of rows per group.
  //----------------------------------
  CostScalar rowsPerGroup = (rowCount / groupCount);

  //---------------------------------------------------------------------------
  //  Aggregation is done only for rows whose group is in memory.
  //
  //  Note that the cost for the first row of a group is reflected in the group
  // initialization cost while all subsequent rows for a group are actually
  // aggregated into the group.  Hence the term (rowsPerGroup - 1.).
  //---------------------------------------------------------------------------
  cpu += cpuCostAggrRowToGroup_ * (rowsPerGroup - csOne) * groupCountInMemory
		* ff_cpu;

  //------------------------------------------------
  //  Cost to compute hash value for each input row.
  //------------------------------------------------
  cpu += cpuCostHashRow_ * rowCount * ff_cpu;

  //---------------------------------------------
  //  Fraction of groups which are fully grouped.
  //---------------------------------------------
  groupingFactor = (groupCountInMemory / groupCount);
  CostScalar rowsConsumedByPartialGB = (groupingFactor * rowCount);

  //-------------------------------------------------------------------------
  //  Since we are executing in DP2, once overflow occurs, we start returning
  //  rows belonging to not yet formed groups.
  //
  //  Thus, all work done up to the first overflow belongs to
  //  blocking usage.  All subsequent work belongs to last row usage.
  //-------------------------------------------------------------------------

  cpuFR = groupingFactor * cpu;

  //------------------------------------------------------------------------
  // we compute the blocking cost in the following way:
  // number of rows preBlocking that need to be processed before the
  // first row is
  // returned is at least groupCountInMemory. that is groupCountIMemory < NR.
  // Also it must be numRowsPreBlocking < rowsConsumedByPartialGB.
  // We take the mid-point as a
  // heuristic.
  // We compute the following costs for each row
  // 1. probing cost
  // 2. group initialization cost
  // 3. aggregate evaluation cost
  //
  // Old behaviour - blocking cost is zero - could be obtained by turning on
  // the CQD
  //-------------------------------------------------------------------------

  CostScalar numRowsPreBlocking;
  if (NOT (groupingFactor.isLessThanOne()))
  {
    numRowsPreBlocking = rowsConsumedByPartialGB;
  }
  else
  {

    numRowsPreBlocking = groupCountInMemory +
                         (rowsConsumedByPartialGB - groupCountInMemory) /2;
    // this is roughly equal to rowsConsumedByPartialGB/2 + 500
  }

  //------------------------------------------------------------------
  // blocking cost is zero, if the old behaviour is desired OR
  // aggregate expressions do not exist
  //------------------------------------------------------------------
  if ( CmpCommon::getDefault(COMP_BOOL_52) == DF_ON ||
       gb_->aggregateExpr().isEmpty())
  {
    cpuBK = csZero;
    cpuLR = cpu;
  }
  else
  {
    // blocking cost computation depends on the order of rows of the input
    // that is how quickly we fill up 1000 groups in the worst case.
    // this is not a concern if the grouping factor is 1
    //
    // if number of groups is more than 1000, we assume that we need to
    // hash rowsConsumedByPartialGB/2 times before the groups are full.

    // it follows that the last row cost needs to get hashing cost for the
    // remaining hash cost of rowsConsumedByPartialGB/2 rows.
    cpuBK = ((cpuCostInitNewGroup_ * groupCountInMemory)+
            (cpuCostHashRow_ +
             cpuCostAggrRowToGroup_ +
             cpuCostPositionHashTableCursor_) *
             numRowsPreBlocking )
             * ff_cpu ;

    // give more weight to plans where grouping Factor is 1 or is more
    // than 70%.
    if (NOT(groupingFactor.isLessThanOne())) // Is it one?
       cpuBK = cpuBK/2;
    else if (groupingFactor >= CostScalar(0.7))
       cpuBK = cpuBK/1.5;

  }


  if ( CmpCommon::getDefault(COMP_BOOL_52) == DF_OFF &&
       (NOT (gb_->aggregateExpr().isEmpty()) ))
  {
     //-------------------------------------------------------------------
     // Cost to return all output rows. Some of these are grouped rows and
     // some are un-grouped.
     //-------------------------------------------------------------------
     cpuLR += cpuCostReturnRow_ * ff_cpu *
                 (groupCountInMemory + rowCount - rowsConsumedByPartialGB);

     if ( CmpCommon::getDefault(COMP_BOOL_90) == DF_OFF )
     {
     if (groupingFactor < CostScalar(1) )
       cpuLR += (cpuCostHashRow_ +
               cpuCostAggrRowToGroup_ +
               cpuCostPositionHashTableCursor_) *
               rowsConsumedByPartialGB/2
               * ff_cpu ;
     }
     else
     {
       if (groupingFactor.isLessThanOne())
       {
         cpuLR += (csOne - groupingFactor) * cpuCostHashRow_
                                  * rowCount * ff_cpu;
         cpuLR +=  cpuCostAggrRowToGroup_ * rowsConsumedByPartialGB/2 * ff_cpu;
       }
       
     }
     //---------------------------------------------------------------------
     //  First row cost involves simply the cost to return one row after all
     // blocking activity has occurred.
     //---------------------------------------------------------------------
  }
  cpuFR = cpuCostReturnRow_ * ff_cpu;

  if (cpuFR > cpuLR)
    cpuFR = cpuLR;
}  // CostMethodHashGroupBy::computePartialGroupByLeafCost().
//<pb>
//==============================================================================
//  The classic Hash GroupBy algorithm may take several passes.  In each pass,
// if memory fills up, a cluster is selected for spilling to disk.  The
// spilled cluster is only partially grouped, so it must be grouped again in a
// subsequent pass.
//
//  This member function computes the costs associated with a single pass of the
// Hash GroupBy algorithm including the cost (if any) to read previously spilled
// clusters from disk and to write spilled clusters to disk.
//
// NOTE:  If this function returns TRUE, it will change the private data members
//         noOfClustersToProcessed_, groupCountPerClusters_ and
//         rowCountPerClusters_ to reflect the new numbers for the next pass.
//
// Input:
//  isFirstPass    -- TRUE for first call to this routine; FALSE otherwise.
//                     When TRUE, assume input is un-grouped rows.  Otherwise,
//                     assume input rows are partially grouped already.
//
// Output:
//  cvPassCurr     -- Cost vector representing CPU and IO resources used for
//                     current pass.
//
//  isRowProduced  -- TRUE if this pass produces an output row; FALSE otherwise.
//
// Return:
//  TRUE if current pass had to do overflow spilling (thus requiring a
//   subsequent call to this routine); FALSE otherwise.
//
//==============================================================================
NABoolean
CostMethodHashGroupBy::computePassCost(NABoolean         isFirstPass,
                                       SimpleCostVector& cvPassCurr,
                                       NABoolean&        isRowProduced)
{
  //---------------------------------------------------------------------
  //  CostScalars to be computed. We first compute the cost of processing
  // one previously spilled cluster and scale the cost up to arrive at
  // the total cost for all clusters.
  //---------------------------------------------------------------------
  CostScalar cpu(csZero), ioSeek(csZero), ioByte(csZero); //j mem(csZero), disk(csZero);
  cvPassCurr.reset();
  cvPassCurr.setNumProbes(csOne);

  //--------------------------------------------------------
  // Common calculations.
  //--------------------------------------------------------
  CostScalar extGroupLengthInKB = CostScalar(extGroupLength_) / csOneKiloBytes;

  //--------------------------------------------------------
  //  A group must be accommodated in a buffer. Otherwise...
  //--------------------------------------------------------
  CMPASSERT( extGroupLengthInKB <= bufferSize_ );

  //---------------------------------------------------------------------
  //  The contents of EACH cluster spilled over from a previous pass are
  // are going to be divided into noOfClustersAllocated_ and processed.
  // In order to avoid confusion, we name the metrics of each previously
  // spilled cluster without the "PerCluster" suffix. The suffix is saved
  // to mean those metrics of a cluster at this pass.
  //---------------------------------------------------------------------

  //----------------------------------------------------
  //  "PerCluster" metrics of previous pass get renamed.
  //----------------------------------------------------
  CostScalar rowCount   = rowCountPerCluster_;
  CostScalar groupCount = groupCountPerCluster_;

  //-------------------------------------
  //  Size of the table if fully grouped.
  //-------------------------------------
  CostScalar groupedTableSize = groupCount * extGroupLengthInKB;

  //--------------------------------------------------------------------
  //  Unless we are working on the first pass, we need to read back from
  // disk each previously-spilled cluster.
  //--------------------------------------------------------------------
  if(NOT isFirstPass)
  {
    ioByte = rowCount * extGroupLengthInKB;
    ioSeek = ioByte / bufferSize_;
  }

  //--------------------------------------------------------------------
  //  CPU costs involved in groupby processing, considering rows in all
  // clusters of this pass.
  //
  // 1. The hash value of a row is used to determine the cluster a row
  //    belongs to.
  // 2. The hash table at that cluster is probed to see whether the row
  //    is part of an existing group in the buffer.
  // 3. If yes, the row is aggregated into the group. Otherwise, we copy
  //    the grouping columns of the row to the buffer, initialize the
  //    aggregates of that new group, and insert it into the hash table.
  //--------------------------------------------------------------------

  //-----------------------------------------------
  //  The hash table is probed for every input row.
  //-----------------------------------------------
  cpu += cpuCostPositionHashTableCursor_ * rowCount;

  //----------------------------------------------------------------
  //  Spilling causes partial groups. Thus, this is under-estimated.
  //----------------------------------------------------------------
  cpu += cpuCostInitNewGroup_ * groupCount;

  //-----------------------------------------------
  //  Cost to aggregate a row to an existing group.
  //-----------------------------------------------
  const CostScalar & cpuCostAggregation =
        (isFirstPass ? cpuCostAggrRowToGroup_ : cpuCostAggrGroupToGroup_);

  //-----------------------------------------------------------
  //  This is over-estimated as spilling causes partial groups.
  //-----------------------------------------------------------
  CMPASSERT(rowCount >= groupCount);
  cpu += cpuCostAggregation * (rowCount - groupCount);

  //-----------------------------------------------------------------------
  //  Determine whether spilling has occurred.
  //
  //  No more spilling. We have a quick way out. Just scale up the CPU cost
  // by the number of clusters processed at this pass and return FALSE.
  //-----------------------------------------------------------------------
  if((NOT isBMO_) OR (groupedTableSize <= memoryLimit_))
  {

    //----------------------------------------------------
    //  Since no spilling occurs, we begin returning rows.
    //----------------------------------------------------
    isRowProduced = TRUE;

    //------------------------------------------------------------------------
    //  Set resources used for this pass scaled up for the number of clusters.
    //------------------------------------------------------------------------
    cvPassCurr.setInstrToCPUTime(cpu * noOfClustersToBeProcessed_);
    cvPassCurr.addKBytesToIOTime(ioByte * noOfClustersToBeProcessed_);
    cvPassCurr.addSeeksToIOTime(ioSeek * noOfClustersToBeProcessed_);
    cvPassCurr.addNumLocalToMSGTime(ioSeek * noOfClustersToBeProcessed_);
    cvPassCurr.addKBLocalToMSGTime(ioByte * noOfClustersToBeProcessed_);
    //j cvPassCurr.setNormalMemory(groupedTableSize);
    //j cvPassCurr.setPersistentMemory(groupedTableSize);

    //--------------------------------
    //  No more spilling in this pass.
    //--------------------------------
    return FALSE;
  }

  //------------------------------------
  //  "PerCluster" metrics of this pass.
  //------------------------------------
  CostScalar rowCountPerCluster =
                          (rowCount / noOfClustersToBeAllocated_).minCsOne();
  CostScalar groupCountPerCluster =
                        (groupCount / noOfClustersToBeAllocated_).minCsOne();

  //----------------------------------------------------
  //  Size of a cluster if grouping has been fully done.
  //----------------------------------------------------
  CostScalar clusterSize = groupCountPerCluster * extGroupLengthInKB;

  //--------------------------------------------------------------------
  //  Must have allocated more than one cluster if spilling is expected.
  //--------------------------------------------------------------------
  CMPASSERT(noOfClustersToBeAllocated_ > 1);

  //----------------------------------------------------------------------
  //  The 1st buffer of each cluster has to remain in memory all the time.
  //----------------------------------------------------------------------
  CostScalar memoryFor1stBufferOfClusters =
                                 bufferSize_ * noOfClustersToBeAllocated_;

  //-------------------------------------------------------
  //  Memory must accommodate one buffer from each cluster.
  //-------------------------------------------------------
  CMPASSERT(memoryFor1stBufferOfClusters <= memoryLimit_);

  //----------------------------------------------------------------------
  //  Memory left for the first cluster. Each cluster takes up one buffer.
  //----------------------------------------------------------------------
  CostScalar memoryLeft =
                  CostScalar(memoryLimit_) - memoryFor1stBufferOfClusters;

  //---------------------------------------------------------------------
  //  Actually, (clusterSize > bufferSize_) is always true because:
  //
  // (clusterSize * noOfClustersAllocated_) == groupedTableSize >
  // memoryLimit_ >= memoryFor1stBufferOfClusters ==
  // (bufferSize_ * noOfClustersAllocated_).
  //
  //  But in case some kind of truncations or floating point manipulation
  // produces an error big enough to make this untrue...
  //---------------------------------------------------------------------
  clusterSize = MAXOF(clusterSize,bufferSize_+.1);

  //----------------------------------------------
  //  Number of clusters which can stay in memory.
  //----------------------------------------------
  Lng32 noOfClustersInMemory = (Lng32)
            (memoryLeft / (clusterSize - bufferSize_)).getFloor().value();

  //----------------------------------------------------------------------
  // for the clusters that stay in memory, calculate average chain length
  // in the event of collisions; if the chain is long, cpuCostInsertRowToChain_
  // needs to be adjusted appropriately takig into account datatype of
  // grouping columns, especially for char and varchar types.
  //
  // We ignore memory pressure, executor assumes a 10MB available memory and
  // 10% of this is used for constructing hash entries in the hash table;
  // Each entry takes 4 bytes; consequently, there are about 250,000 entries per
  // cluster; then average chain length is groupcountPerCluster/ 250000
  // this is not quite accurate, if we do not have good stats or good
  // cardinality estimates (UEC of grouping columns).
  // We ignore these scenarios for now
  //------------------------------------------------------------------------

  if ( CmpCommon::getDefault(COMP_BOOL_52) == DF_OFF)
  {
    CostScalar averageChainLength =  (groupCountPerCluster/
                               ActiveSchemaDB()->getDefaults().getAsLong(
                                MAX_HEADER_ENTREIS_PER_HASH_TABLE)).getFloor();

    CostScalar costToInsert = calculateCostToInsertIntoChain
                                         (averageChainLength);

    cpu += costToInsert * noOfClustersInMemory;
  }
  //------------------------------------------------------------
  //  Number of clusters which spills over to disk in this pass.
  //------------------------------------------------------------
  Lng32 noOfClustersSpilled =
                        noOfClustersToBeAllocated_ - noOfClustersInMemory;

  // -----------------------------------------------------------------------
  //  Groups in spilled clusters are only partial groups. Thus, they take
  // up more disk space and incur more disk I/O than that estimated from
  // their sizes when fully grouped. In the worst though unlikely case,
  // all the rows in the spilled cluster could remain ungrouped. It might
  // also happen that the cluster could have been fully grouped if the
  // rows are sorted by the grouping columns (in which case we should be
  // doing sort group by instead). So the number of rows written to disk for
  // each of these spilled clusters could be anywhere between the extremes
  // of groupCountPerCluster and rowCountPerCluster.
  //
  // In order to account for this, we introduce a fludge factor we call
  // groupingFactorForSpilledClusters_ which can range from 0 to 1. And
  // the number of rows in a spilled cluster is determined by the formula:
  //
  // rowCountPerCluster +
  // groupingFactorForSpilledClusters_ *
  // (groupCountPerCluster - rowCountPerCluster).
  //
  // which can lie between groupCountPerCluster and rowCountPerCluster.
  // -----------------------------------------------------------------------
  CostScalar rowCountPerSpilledCluster =
        rowCountPerCluster + (groupCountPerCluster - rowCountPerCluster) *
	                                groupingFactorForSpilledClusters_;

  //-----------------------------------------------------------------------
  //  Re-estimate spilled cluster size after adjusted for partial grouping.
  //-----------------------------------------------------------------------
  CostScalar spilledClusterSize =
                      rowCountPerSpilledCluster * extGroupLengthInKB;

  //---------------------------------------
  //  I/O for writing the spilled clusters.
  //---------------------------------------
  //j disk   = spilledClusterSize * noOfClustersSpilled;
  ioByte = spilledClusterSize * noOfClustersSpilled;
  ioSeek = ioByte / bufferSize_;

  //----------------------------------------------------------------------------
  //  Now, the whole thing is done for *each* of the previously spilled clusters
  // which is stored as noOfClustersToBeProcessed_ before this function has been
  // called. We need to scale up the cost we have computed so far by that
  // factor.
  //----------------------------------------------------------------------------
  cvPassCurr.setInstrToCPUTime(cpu * noOfClustersToBeProcessed_);
  cvPassCurr.addKBytesToIOTime(ioByte * noOfClustersToBeProcessed_);
  cvPassCurr.addSeeksToIOTime(ioSeek * noOfClustersToBeProcessed_);
  cvPassCurr.addNumLocalToMSGTime(ioSeek * noOfClustersToBeProcessed_);
  cvPassCurr.addKBLocalToMSGTime(ioByte * noOfClustersToBeProcessed_);
  //j cvPassCurr.setDiskUsage(disk);
  //j cvPassCurr.setNormalMemory(memoryLimit_);
  //j cvPassCurr.setPersistentMemory(memoryLimit_);

  //---------------------------------------------------------------------
  //  If there is a un-spilled cluster in this pass, assume the first row
  // is produced. But the row is produced only after we finish all the
  // processing for the first cluster processed.
  //---------------------------------------------------------------------
  isRowProduced = (noOfClustersInMemory > 0);

  //------------------------------------------------------------------------
  //  Finally, we need to prepare new values of noOfClustersToBeProcessed_,
  // rowCountPerCluster_ and groupCountPerCluster_ for the next pass if
  // we have spilling in this pass.
  //
  //  This needs explanation. *Each* previously-spilled cluster which is
  // processed in this pass generates a number of spilled clusters stored
  // in noOfClustersSpilled.
  //
  //  Since we process a total of noOfClustersToBeProcessed_ previously-
  // spilled clusters in this pass, total number of spilled clusters created
  // for the next pass = noOfClustersToBeProcessed_ * noOfClustersSpilled,
  // and this number becomes noOfClustersToBeProcessed_ for the next pass.
  //------------------------------------------------------------------------
  groupCountPerCluster_      =  groupCountPerCluster;
  rowCountPerCluster_        =  rowCountPerSpilledCluster;

  // this check was added to prevent overflowing.  11/06/00
  if ( noOfClustersToBeProcessed_ > INT_MAX/MIN_ONE(noOfClustersSpilled) )
    noOfClustersToBeProcessed_ = INT_MAX;
  else
    noOfClustersToBeProcessed_ *= noOfClustersSpilled;

  return TRUE;

} // CostMethodHashGroupBy::computePassCost()

//------------------------------------------------------------------
// This method computes cost of chains; assume on an average we
// check half of the chain
//------------------------------------------------------------------
CostScalar CostMethodHashGroupBy::calculateCostToInsertIntoChain
                                             (CostScalar &averageChainLength)
{
  if (averageChainLength < 2 )
     return CostScalar (0);

  return  averageChainLength * groupKeyLength_/2 * cpuCostCompareGroupKeys_;
} // CostMethodHashGroupBy::calculateCostToInsertIntoChain()

//<pb>
//==============================================================================
//  Compute number of clusters used by a Hash GroupBy.  This is based on the
// memory limit, the estimated grouped-table size and the buffer size (as stored
// in the private section of the class).
//
// Input:
//  memoryLimit  -- Amount of main memory available to Hash GroupBy.
//
//  tableSize    -- Size of input table for Hash GroupBy.
//
// Output:
//  none
//
// Return:
//  Number of clusters used by Hash GroupBy algorithm.
//
//==============================================================================
Lng32
CostMethodHashGroupBy::computeCountOfClusters( const CostScalar & memoryLimit,
                                               const CostScalar & tableSize )
{

  CostScalar clusters;
  Lng32 maxClusters;

  if ( CmpCommon::getDefault(COMP_BOOL_52) == DF_ON)
  {
    clusters = (tableSize / memoryLimit);

    maxClusters = (Lng32)((memoryLimit / bufferSize_).getFloor().value());

    if (clusters > double(maxClusters))
      return maxClusters;
  }
  else
  {
    // if in dp2, the number of clusters is 1
    if (rpp_->executeInDP2())
    {
      return 1;
    }

    //---------------------------------------------------------
    //  the requested location is either ESP or Master
    // the maximum number of clusters is 40 in the executor. Each
    // cluster maintains a hash table; minimum is 1.
    //---------------------------------------------------------

    // grouped Table Size is already in Kilobytes
    CostScalar  groupedTableSize = tableSize;
    double maxTableSizeForNumberOfClusters = ActiveSchemaDB()->getDefaults().
               getAsDouble(HGB_MAX_TABLE_SIZE_FOR_CLUSTERS);

    if (groupedTableSize.value() > maxTableSizeForNumberOfClusters)
    {
      groupedTableSize=maxTableSizeForNumberOfClusters;
    }
    else if (groupedTableSize < 100)
    {
      groupedTableSize=100;
    }

    double exeMemoryLimit=ActiveSchemaDB()->getDefaults().
                          getAsDouble(HGB_MEMORY_AVAILABLE_FOR_CLUSTERS) *
                          csOneKiloBytes.getFloor().value();

    clusters = (groupedTableSize / exeMemoryLimit);

    //---------------------------------------------
    //  We must retain one buffer for each cluster.
    //---------------------------------------------
    maxClusters = (Lng32)((memoryLimit / bufferSize_).getFloor().value());

    if(maxClusters != 0 &&
     clusters > double(maxClusters))
    {
      return maxClusters;
    }
  }

  return Lng32(clusters.getCeiling().value());

}  // CostMethodHashGroupBy::computeCountOfClusters().
//<pb>
//==============================================================================
//  Compute operator cost for a specified Hash GroupBy operator.
//
// Input:
//  op             -- pointer to specified Hash GroupBy operator.
//
//  myContext      -- pointer to optimization context for this Hash GroupBy
//                     operator.
//
// Output:
//  countOfStreams -- degree of parallelism for this Hash GroupBy operator.
//
// Return:
//  Pointer to computed cost object for this Hash GroupBy operator.
//
//==============================================================================
Cost*
CostMethodHashGroupBy::computeOperatorCostInternal(RelExpr*       op,
                                                   const Context* myContext,
                                                   Lng32&          countOfStreams)
{

  //-----------------------------
  //  CostScalars to be computed.
  //-----------------------------
  CostScalar cpuLR(csZero), ioLR(csZero), idleLR(csZero);
  CostScalar cpuFR(csZero), ioFR(csZero);
  CostScalar cpuBK(csZero), ioBK(csZero);
  //j CostScalar mem(csZero),   disk(csZero);

  //fudge factor for CPUTIME
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();


  //-------------------
  //  Preparatory work.
  //-------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  //-------------------------------------------
  //  Save off estimated degree of parallelism.
  //-------------------------------------------
  countOfStreams = countOfStreams_;

  //----------------------------------------------------------------------
  //  Added on 7/16/97: If we're on our way down the tree and this group
  // by is being considered for execution in DP2, generate a zero cost
  // object first and come back to cost it later when we're on our way up.
  // Set the count of streams to an invalid value (-1) to force us to
  // recost on the way back up.
  //----------------------------------------------------------------------
  if(rpp_->executeInDP2() AND
    (NOT context_->getPlan()->getPhysicalProperty()))
  {
    countOfStreams = -1;
    return generateZeroCostObject();
  }

  deriveParameters();

  //===============
  //  Real work. ==
  //===============
  GroupByAgg * groupByNode = (GroupByAgg *) op;

  PhysicalProperty * sppForMe = (PhysicalProperty *) myContext->
                                  getPlan()->getPhysicalProperty();
  NABoolean executeInEsp = FALSE;
  if ((sppForMe != NULL) && sppForMe->executeInESPOnly())
    executeInEsp = TRUE;

  //--------------------------------------------------------------------
  //  Percentage of groups that fit in memory.  Assume all do initially.
  //--------------------------------------------------------------------
  CostScalar groupingFactor = csOne;

  //--------------------------------------------------------------------
  // A hash groupby performed in DP2 and partial leaf groupby performed
  // in Esp don't do overflow handling. Once the memory allocated
  // to it has been used up, rows coming up which
  // belong to new groups are just returned to parent grby op as they are.
  //--------------------------------------------------------------------
  if(rpp_->executeInDP2() ||
     (groupByNode->isAPartialGroupByLeaf() &&
      executeInEsp))
  {
    computePartialGroupByLeafCost(cpuFR,cpuLR,cpuBK,groupingFactor);
  }
  else
  {
    //----------------------------------------------------------------------
    //  Compute the hash value of each row.  Since we can't return an output
    // row until all input rows are hashed, this cost goes into blocking.
    //----------------------------------------------------------------------

    // -------------------------------------------------------------------
    // Distinct is considered non-blocking; add the cost to cpuLR
    // -------------------------------------------------------------------
    NABoolean considerBlockingCost;
    if ((CmpCommon::getDefault(COMP_BOOL_52) == DF_OFF) &&
        (gb_->aggregateExpr().isEmpty())
       )
      considerBlockingCost = FALSE;
    else 
      considerBlockingCost = TRUE;

    CostScalar hashCost = cpuCostHashRow_
                      * rowCountPerStream_ / noOfProbesPerStream_
                      * ff_cpu;

    if (considerBlockingCost)
       cpuBK += hashCost;
    else 
       cpuLR += hashCost;
    
    SimpleCostVector cvPassPrev, cvPassCurr;
    NABoolean isFirstPass  = TRUE;
    NABoolean isFRproduced = FALSE;

    //--------------------------------------------------------------------
    //  This loop captures the recursive spilling of clusters. A cluster
    // is chosen to spill to disk when the whole input table doesn't fit
    // into memory. Once spilling has occurred, the cluster is not fully
    // grouped. Its disk image contains only partial groups. Therefore,
    // we need to perform a new pass of grouping on the cluster again. In
    // that second pass, the image is treated as an input file, and we
    // allocate a number of clusters to handle it, just as we did before.
    // Again, some clusters in the second round may also spill, leading
    // to processing in a third round and so on. This continues until we
    // have no more spilling.
    //--------------------------------------------------------------------
    NABoolean isSpilled;
    do
    {

      //-------------------------------------------------------------------
      //  Find resource usage for current pass and determine whether or not
      // this pass produced an output row.
      //-------------------------------------------------------------------
      NABoolean isRowProducedInThisPass;
      isSpilled = computePassCost(isFirstPass,
                                  cvPassCurr,
                                  isRowProducedInThisPass);


      //------------------------------------------------------------------
      //  All first pass resource usage goes into blocking.  Subsequent
      // passes go into blocking until an output row is actually produced.
      //
      //------------------------------------------------------------------

      if ( (isFirstPass OR (NOT isFRproduced)) AND considerBlockingCost
         )
      {
        //--------------------------------------------------------------------
        //  All resource usage goes into blocking until first row is produced.
        //--------------------------------------------------------------------
        cpuBK	+= cvPassCurr.getCPUTime();
        ioBK    += cvPassCurr.getIOTime();
      }
      else
      {
        //------------------------------------------------------------------
        //  After first row is produced, subsequent resource usage goes into
        // last row.
        //
        // Note that we must convert last row cost to a cost for all probes.
        //------------------------------------------------------------------
        cpuLR    += cvPassCurr.getCPUTime() * noOfProbesPerStream_;
        ioLR	 += cvPassCurr.getIOTime()  * noOfProbesPerStream_;

        //------------------------------------------------------------------
        //  No pass can begin until the previous pass has completed in its
        // entirety, so we need to ensure that an appropriate amount of idle
        // time is reflected for each transition between passes.
        //------------------------------------------------------------------
        SimpleCostVector cvBlockingSum = blockingAdd(cvPassPrev,
                                                     cvPassCurr,
                                                     rpp_);
        idleLR   += cvBlockingSum.getIdleTime() * noOfProbesPerStream_;
      }

      //--------------------------------------------------------------------
      //  If no rows have been produced in a previous pass, see if this pass
      // produced the first row.
      //--------------------------------------------------------------------
      if(NOT isFRproduced)
      {
        isFRproduced = isRowProducedInThisPass;
      }

      //-----------------------------------------------
      //  All subsequent passes are not the first pass.
      //-----------------------------------------------
      isFirstPass = FALSE;

      //------------------------------------------------------------
      //  Current pass becomes previous pass in next loop iteration.
      //------------------------------------------------------------
      cvPassPrev = cvPassCurr;

    }
    while (isSpilled); // Continue until no more spilling.

    //-----------------------------------------------------------------
    //  Costs to evaluate the having predicates and to copy the rows to
    // the result buffer. Rather simplistic FR computation.
    //-----------------------------------------------------------------


    cpuFR += cpuCostEvalHavingPred_ * ff_cpu;
    cpuLR += cpuCostEvalHavingPred_ * groupCountPerStream_ * ff_cpu;

    cpuFR += cpuCostReturnRow_ * ff_cpu;
    cpuLR += cpuCostReturnRow_ * myRowCountPerStream_ * ff_cpu;

  }

  //-------------------------------------
  //  Synthesize the simple cost vectors.
  //-------------------------------------
  SimpleCostVector cvFR (
    cpuFR,
    ioFR,
    ioFR,
    csZero,
    noOfProbesPerStream_
    );

  SimpleCostVector cvLR (
    cpuLR,
    ioLR,
    ioLR,
    idleLR,
    noOfProbesPerStream_
    );

  SimpleCostVector cvBK (
    cpuBK,
    ioBK,
    ioBK,
    csZero,
    noOfProbesPerStream_
    );

#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"HASHGROUPBY::computeOperatorCost()\n");
    fprintf(pfp,"child0RowCount=%g,groupCount=%g,myRowCount=%g\n",
            child0RowCount_.value(),groupCount_.value(),myRowCount_.value());
    cvFR.print(pfp);
    cvLR.print(pfp);
    cvBK.print(pfp);
  }
#endif

  //----------------------------------------
  //  Synthesize and return the cost object.
  //----------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  // If this is a partial Group By leaf in an ESP adjust it's cost
  if(groupByNode->isAPartialGroupByLeaf() &&
     ((sppForMe && sppForMe->executeInESPOnly()) ||
      (CmpCommon::getDefault(COMP_BOOL_186) == DF_ON)))
  {
    // don't adjust if Group Columns contain partition columns.
    const PartitioningFunction* const myPartFunc =
      sppForMe->getPartitioningFunction();

    ValueIdSet myPartKey = myPartFunc->getPartitioningKey();

    ValueIdSet myGroupingColumns = groupByNode->groupExpr();

    NABoolean myGroupingMatchesPartitioning = FALSE;

    if (myPartKey.entries() &&
        myGroupingColumns.contains(myPartKey))
      myGroupingMatchesPartitioning = TRUE;

    if (!myGroupingMatchesPartitioning)
    {
      CostScalar grpByAdjFactor = (ActiveSchemaDB()->getDefaults())\
                                    .getAsDouble(ROBUST_PAR_GRPBY_LEAF_FCTR);
      cvLR *= grpByAdjFactor;
      cvFR *= grpByAdjFactor;
      cvBK *= grpByAdjFactor;
    }
  }

  Cost *costPtr = new STMTHEAP HashGroupByCost( &cvFR,
                                                &cvLR,
                                                &cvBK,
                                                cpuCount,
                                                fragmentsPerCPU,
                                                groupingFactor
					      );

#ifndef NDEBUG
  if ( printCost )
    {
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

} // CostMethodHashGroupBy::computeOperatorCostInternal()
//<pb>
//==============================================================================
//  Produce a final cumulative cost for an entire subtree rooted at a specified
// physical Hash GroupBy operator.
//
// Input:
//  hashGroupbyOp -- specified physical Hash GroupBy operator.
//
//  myContext     -- context associated with specified Hash GroupBy operator.
//
//  pws           -- plan work space associated with specified Hash GroupBy
//                    operator.
//
//  planNumber    -- used to get appropriate child contexts.
//
// Output:
//  none
//
// Return:
//  Pointer to cumulative final cost.
//
//==============================================================================
Cost*
CostMethodHashGroupBy::computePlanCost(      RelExpr*       hashGroupByOp,
                                       const Context*       myContext,
                                       const PlanWorkSpace* pws,
                                             Lng32           planNumber)
{

  //-----------------------------------------------------------------------
  //  Get a local copy of the required physical properties for use in later
  // roll-up computations.
  //-----------------------------------------------------------------------
  const ReqdPhysicalProperty* rpp = myContext->getReqdPhysicalProperty();

  //------------------------------------------------------------------------
  // if this executes in dp2 call other roll up, so operator is not
  //   considered blocking
  //------------------------------------------------------------------------
  if (rpp->executeInDP2() &&
      (CmpCommon::getDefault(COMP_BOOL_52) == DF_ON) )
  {
    return CostMethod::computePlanCost(hashGroupByOp,
                                       myContext,
                                       pws,
                                       planNumber);
  }

 if ((CmpCommon::getDefault(COMP_BOOL_52) == DF_OFF) &&
     ((HashGroupBy *)hashGroupByOp)->aggregateExpr().isEmpty())
 {
   return CostMethod::computePlanCost(hashGroupByOp,
                                   myContext,
                                   pws,
                                   planNumber);
 }

  //------------------------------------------------------------------------
  // If the group-by is because of DISTINCT, then it is not a blocking
  // operator; for a distinct, no aggregate expressions exist
  //------------------------------------------------------------------------

  //------------------------------------------------------------------------
  //  Get parent's operator cost (independent of its children) from the plan
  // workspace.
  //
  // NOTE:  We need to cast constness away since getFinalOperatorCost cannot
  //       be made const.
  //------------------------------------------------------------------------
    HashGroupByCost* parentCost =
      (HashGroupByCost*)
       ((PlanWorkSpace *)pws)->getFinalOperatorCost(planNumber);

  //------------------------------------------------------------------
  //  Get child's roll-up cost from a child context stored in the plan
  // workspace.
  //------------------------------------------------------------------
  Context*    childContext    = pws->getChildContext(0,planNumber);
  CMPASSERT(childContext && childContext->hasOptimalSolution());

  const Cost* childRollUpCost = childContext->getSolution()->getRollUpCost();

  Cost* planCost;

  //============================================================================
  //  When a Hash GroupBy operator executes in DP2, it may encounter a situation
  // whereby it can no longer fit a new group into its memory buffers.  We call
  // this an overflow situation.  During an overflow situation, the Hash GroupBy
  // operator simply returns single rows to its parent, and the resources
  // necessary for the child of the Hash GroupBy operator to produce these rows
  // overlap with the Hash GroupBy operator's own last row activity.
  //
  //  Thus, when a Hash GroupBy operator executes in DP2, we need to adjust
  // the roll-up formulas to take into account the inherant overlap of an
  // overflow situation.  If the Hash GroupBy operator does not execute in DP2,
  // then we can use the traditional blocking unary roll-up formulas.
  //============================================================================

  //----------------------------------------------------------------------------
  //  If Hash GroupBy operator does not execute in DP2, use traditional blocking
  // unary roll-up formulas.
  //----------------------------------------------------------------------------
  if (NOT rpp->executeInDP2())
    {
      planCost = rollUpUnaryBlocking(*parentCost, *childRollUpCost, rpp);
      delete parentCost;
      return planCost;
    }

  //=========================================================================
  //  At this point we know the Hash GroupBy operator executes in DP2, so use
  // the traditional blocking unary roll-up formulas modified to take into
  // account potential overlap during an overflow situation.
  //=========================================================================

  //-----------------------
  //  Create an empty cost.
  //-----------------------
  planCost = new STMTHEAP Cost();

  //-------------------------------------------------------------------------
  //  Total cost roll-up and first row roll-up are the same as in traditional
  // unary blocking.
  //-------------------------------------------------------------------------
  planCost->totalCost() = parentCost->getTotalCost()
                                              + childRollUpCost->getTotalCost();
  planCost->cpfr()      = parentCost->getCpfr();

  //----------------------------------------------------------------------
  //  A percentage of the child's last row activity overlaps with the Hash
  // GroupBy operator's last row activity.  The term (1 - groupingFactor)
  // represents this percentage.
  //----------------------------------------------------------------------
  const CostScalar & groupingFactor = parentCost->getGroupingFactor();
  planCost->cplr() =
     overlapAddUnary(parentCost->getCplr(),
                childRollUpCost->getCplr() * (csOne - groupingFactor));

  //---------------------------------------------------------------------
  //  Compute number of probes associated with parent's preliminary cost.
  //---------------------------------------------------------------------
  const CostScalar & parentNumProbes = parentCost->getCpbc1().getNumProbes();

  //-------------------------------------------------
  //  See if Hash GroupBy is first blocking operator.
  //-------------------------------------------------
  if ( childRollUpCost->getCpbc1().isZeroVectorWithProbes() )
    {

      //----------------------------------------------------------------------
      //  Hash GroupBy is first blocking operator.  Use same formula as in
      // traditional unary blocking roll-up except that only a percentage of
      // the child's last row activity is accumulated into blocking activity.
      // The term (groupingFactor) represents this percentage.
      //----------------------------------------------------------------------
      planCost->cpbc1() =
          overlapAddUnary(parentCost->getCpbc1(),
                     childRollUpCost->getCplr()*groupingFactor/parentNumProbes);
    }

  else
    {
      //-------------------------------------------------------------
      //  Parent not first blocking operator.  Roll up first blocking
      // cost just as in traditional unary blocking roll-up.
      //-------------------------------------------------------------
      planCost->cpbc1() =
              childRollUpCost->getCpbc1().getNormalizedVersion(parentNumProbes);
    }

  //-------------------------------------------------------------------------
  //  The total blocking formula is the same as in traditional unary blocking
  // roll-up except that only a percentage of the child's last row activity
  // is accumulated into blocking activity.  The term (groupingFactor)
  // represents this percentage.
  //-------------------------------------------------------------------------
  planCost->cpbcTotal() =
    blockingAdd(
      overlapAddUnary(parentCost->getCpbc1(),
                 childRollUpCost->getCplr() * groupingFactor / parentNumProbes),
      childRollUpCost->getCpbcTotal().getNormalizedVersion(parentNumProbes),
      rpp);

  //------------------------------------------------------------------------
  //  Overlapped process costs are the same as in traditional unary blocking
  // roll-up.
  //------------------------------------------------------------------------
//jo  planCost->opfr() = childRollUpCost->getOpfr();
//jo  planCost->oplr() = childRollUpCost->getOplr();

  delete parentCost;
  return planCost;

} // CostMethodHashGroupBy::computePlanCost()
//<pb>

// ----QUICKSEARCH FOR ShortCutGroupBy.....................................

/**********************************************************************/
/*                                                                    */
/*                       CostMethodShortCutGroupBy                     */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodShortCutGroupBy::computeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodShortCutGroupBy::computeOperatorCostInternal(RelExpr* op,
                                                       const Context* myContext,
                                                       Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // CostScalars to be computed.
  // ---------------------------------------------------------------------
  CostScalar cpu(csZero);

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // Added on 7/16/97: If we're on our way down the tree and this group
  // by is being considered for execution in DP2, generate a zero cost
  // object first and come back to cost it later when we're on our way up.
  // Set the count of streams to an invalid value (0) to force us to
  // recost on the way back up.
  // ---------------------------------------------------------------------
  if(rpp_->executeInDP2() AND
    (NOT context_->getPlan()->getPhysicalProperty()))
  {
    countOfStreams = 0;
    return generateZeroCostObject();
  }

  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();


  // ---------------------------------------------------------------------
  // It's hard to cost an ShortCutGroupBy accurately since its execution
  // can usually be short-circuited after say, we found one row to have
  // satisfied the any-true aggregate expression. This short circuit can
  // even lead to the cancellation of the execution of the operator's
  // child. Since we cost the full execution of the operator's child, we
  // also cost the full execution of the ShortCutGroupBy assuming there
  // isn't a short circuit.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // aggrVis should contain only one expr which is rooted by ANYTRUE op.
  // ---------------------------------------------------------------------
  const ValueIdSet& aggrVis = gb_->aggregateExpr();
  CMPASSERT(NOT aggrVis.isEmpty());
  ValueId ExprVid = aggrVis.init();
  // coverity[check_return] 
  aggrVis.next(ExprVid);
  const ItemExpr * itemExpr = ExprVid.getItemExpr();
  OperatorTypeEnum optype = itemExpr->getOperatorType();
  CMPASSERT(optype==ITM_MIN OR
            optype==ITM_MAX OR
            optype==ITM_ANY_TRUE OR
            optype==ITM_ANY_TRUE_MAX);

  if((optype==ITM_ANY_TRUE) OR (optype==ITM_ANY_TRUE_MAX))
  {
    const ValueId & anyTruePredVid = itemExpr->child(0).getValueId();
    ValueIdSet anyTruePredVis;
    anyTruePredVis.insert(anyTruePredVid);

    CostScalar cpuCostEvalAnyTruePred =
                 CostPrimitives::cpuCostForEvalPred(anyTruePredVis);
    cpu = (cpuCostPassRow_ + cpuCostEvalAnyTruePred) * child0RowCount_;

    // ---------------------------------------------------------------------
    // Synthesize the cost vectors.
    // ---------------------------------------------------------------------


    SimpleCostVector cv (
      cpu/countOfStreams_ * ff_cpu,
      csZero,
      csZero,
      csZero,
      noOfProbesPerStream_
      );

    // ---------------------------------------------------------------------
    // For debugging.
    // ---------------------------------------------------------------------
#ifndef NDEBUG
    NABoolean printCost =
      ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
    if ( printCost )
    {
      pfp = stdout;
      fprintf(pfp,"ShortCutGroupBy::computeOperatorCost()\n");
      fprintf(pfp,"childRowCount=%g",child0RowCount_.value());
      cv.print(pfp);
    }
#endif

    // ---------------------------------------------------------------------
    // Synthesize and return the cost object.
    // ---------------------------------------------------------------------

    // Find out the number of cpus and number of fragments per cpu.
    Lng32 cpuCount, fragmentsPerCPU;
    determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

    Cost *costPtr = new STMTHEAP Cost(&cv,
                                      &cv,
                                      NULL,
                                      cpuCount,
                                      fragmentsPerCPU
				     );

#ifndef NDEBUG
    if ( printCost )
    {
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

    return costPtr;
  }
  else  // MIN/MAX optimization
  {
    //tentative costing code
    CMPASSERT(op->getFirstNRows()==1);
    cpu = cpuCostPassRow_;//it only passes along a single row
    SimpleCostVector cv (cpu/countOfStreams_ * ff_cpu,
                      csZero,
		      csZero,
		      csZero,
		      noOfProbesPerStream_);
    Lng32 cpuCount, fragmentsPerCPU;
    determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );
    Cost *costPtr = new STMTHEAP Cost (&cv,&cv,NULL,cpuCount,fragmentsPerCPU);
    return costPtr;

  }
}  // ShortCutGroupBy::computeOperatorCostInternal().


Cost*
CostMethodShortCutGroupBy::computePlanCost( RelExpr* op,
                                            const Context* myContext,
                                            const PlanWorkSpace* pws,
                                            Lng32 planNumber
                                          )
{
  if (CmpCommon::getDefault(COSTING_SHORTCUT_GROUPBY_FIX) == DF_ON &&
      isUnderNestedJoin(op, myContext) == FALSE)
    return CostMethod::computePlanCost(op, myContext, pws, planNumber);

  //--------------------------------------------------------------------------
  //  Grab parent's cost (independent of its children) directly from the plan
  // work space.  This cost should contain result of computeOperatorCost().
  //--------------------------------------------------------------------------
  // Need to cast constness away since getFinalOperatorCost cannot
  // be made const
  Cost* parentCost = ((PlanWorkSpace *)pws)->getFinalOperatorCost( planNumber );

  //------------------------------------------------------
  //  Get current child's context via our plan work space.
  //------------------------------------------------------
  Context* childContext = pws->getChildContext( 0, planNumber );

  //------------------------------------------------------------------
  // Make sure plans are already generated by the operator's children.
  //------------------------------------------------------------------
  if ( childContext == NULL )
  {
     ABORT("CostMethod::computePlanCost(): A child has a NULL context");
  }

  // Coverity flags this dereferencing null pointer childContext.
  // This is a false positive, we fix it using annotation.
  // coverity[var_deref_model]
  if ( NOT childContext->hasOptimalSolution() )
  {
     ABORT("CostMethod::computePlanCost(): A child has no solution");
  }

  //---------------------------------------------
  // Accumulate this child's cost into PlanCost.
  //---------------------------------------------
  Cost mergedChildCost(
                 *childContext->getSolution()->getRollUpCost() );

  /*
 if(op->getFirstNRows() == 1)
 {
   mergedChildCost.cpfr() = mergedChildCost.getCpfr()*0.8 ;
   mergedChildCost.cplr() = mergedChildCost.getCpfr() ;
 }
 */

 Cost* planCost = rollUp( parentCost
                         , &mergedChildCost
                         , myContext->getReqdPhysicalProperty()
                        );

 delete parentCost;

 return planCost;

} // CostMethodShortCutGroupBy::computePlanCost()
// LCOV_EXCL_STOP
//<pb>

// ----QUICKSEARCH FOR JOIN...............................................

/**********************************************************************/
/*                                                                    */
/*                           CostMethodJoin                           */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodJoin::cacheParameters().
// -----------------------------------------------------------------------
void CostMethodJoin::cacheParameters(RelExpr* op, const Context* myContext)
{
  CostMethod::cacheParameters(op,myContext);

  jn_ = (Join *) op;

  // if inputForSemiTSJ is set for inputLogProp_ it cannot be passed below the join
  //   so create a new EstLogProp with the flag set off to pass to my children
  EstLogPropSharedPtr copyInputEstProp;
  if (inLogProp_->getInputForSemiTSJ() != EstLogProp::NOT_SEMI_TSJ)
    {
      copyInputEstProp = EstLogPropSharedPtr(new (HISTHEAP)
              EstLogProp(*inLogProp_));
      copyInputEstProp->setInputForSemiTSJ(EstLogProp::NOT_SEMI_TSJ);
    }
  else
    {
      copyInputEstProp = inLogProp_;
    }
  child0LogProp_ = jn_->child(0).outputLogProp(copyInputEstProp);

  CMPASSERT(child0LogProp_ != NULL);
  child0RowCount_ = ( child0LogProp_->getResultCardinality() ).minCsOne();


  // ---------------------------------------------------------------------
  // For a TSJ, the input est log prop for the right child is the output
  // est log prop of its left child.
  // EXCEPT for the case where the TSJ is a semi-Join, in which case that
  // extra piece of information has to be added to the input est log prop
  // given to the right child.
  // ---------------------------------------------------------------------
  if ( jn_->isTSJ() )
  {
    if ( jn_->isSemiJoin() ||
         inLogProp_->getInputForSemiTSJ() == EstLogProp::SEMI_TSJ )
      {
        EstLogPropSharedPtr copyChild0LogProp(new STMTHEAP
             EstLogProp(*child0LogProp_));
        copyChild0LogProp->setInputForSemiTSJ( EstLogProp::SEMI_TSJ );
        child1LogProp_ = jn_->child(1).outputLogProp( copyChild0LogProp );
      }
    else if ( jn_->isAntiSemiJoin() ||
              inLogProp_->getInputForSemiTSJ() == EstLogProp::ANTI_SEMI_TSJ )
      {
        EstLogPropSharedPtr copyChild0LogProp(new STMTHEAP
             EstLogProp(*child0LogProp_));
        copyChild0LogProp->setInputForSemiTSJ( EstLogProp::ANTI_SEMI_TSJ );
        child1LogProp_ = jn_->child(1).outputLogProp( copyChild0LogProp );
      }
    else
      {
        child1LogProp_ = jn_->child(1).outputLogProp( child0LogProp_ );
      }
  }
  else  // if semiTSJ set use copy with it set off
  {
    child1LogProp_ = jn_->child(1).outputLogProp( copyInputEstProp );
  }

  CMPASSERT(child1LogProp_ != NULL);
  child1RowCount_ = ( child1LogProp_->getResultCardinality() ).minCsOne();

  //---------------------------------------------------------------------------
  // The reuse issue is accounted for by setting
  // number of probes to ONE in the final rollup cost in the method
  // CostMethodHashJoin::computePlanCost. The rationale for the whole
  // Reuse costing is also explained there.
  //
  // In case of Hash Join Reuse, get outputlogprop from
  // materializeOutputLogProp. -OA Mar02
  //---------------------------------------------------------------------------

  if ( jn_->isHashJoin() AND myContext->getInputLogProp()
    ->getResultCardinality().isGreaterThanOne() /* > 1 */ )
  {
    HashJoin * hj = (HashJoin *)jn_;
    if(hj->isNoOverflow() AND hj->isReuse())
    {
      Int32 multipleCalls;

      // The right(inner) child's output log prop are set equal to materializeoutputLP
      // This essentially means that the number of parent probes for the right
      // child is set to ONE IF the right child needs to be materialized only once.
      // Right child's output cardinality ==(num parent probes)*(actual number
      //   of tuples returned by the child). When numParentProbes==1, it just returns
      // the actual number of tuples.
      child1LogProp_ = jn_->child(1).getGroupAttr()
	 ->materializeOutputLogProp(copyInputEstProp, &multipleCalls);
      child1RowCount_ = ( child1LogProp_->getResultCardinality() ).minCsOne();

      // It is not clear why changing the number of probes for the join itself
      // even if we materialize the right child to have only one probe.
      if ( CmpCommon::getDefault(COMP_BOOL_37) == DF_ON AND multipleCalls == 0 )
      {
		// setting numberOfprobes_ = one, implies, that we are computing this
		// for empty input logical properties

		noOfProbes_ = csOne;

		EstLogPropSharedPtr emptyLogProp = (*GLOBAL_EMPTY_INPUT_LOGPROP);

		if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			(partFunc_ != NULL) )
		{
		  
		  noOfProbesPerStream_ = emptyLogProp->getCardOfBusiestStream(partFunc_,
															  countOfStreams_,
															  NULL,
															  countOfAvailableCPUs_ );
		}
		else
		  noOfProbesPerStream_ = ( noOfProbes_ / countOfStreams_ ).minCsOne();
      }
    }
  }


  // Make sure clean up is done in the last costing session.
  if(isColStatsMeaningful_)
  {
    ABORT("CostMethodJoin: Didn't clean up ColStats after last session.");
  }

  // Whether a HJ or MJ has equi-join predicates.
  hasEquiJoinPred_ =
      (NOT jn_->isTSJ()) AND (NOT jn_->getEquiJoinPredicates().isEmpty());

  // ---------------------------------------------------------------------
  // If I am not a NJ, estimate result statistics of doing inner join on
  // equi-join cols. Such statistics information are useful for MJ and HJ
  // costing.
  // ---------------------------------------------------------------------
  if(NOT jn_->isTSJ()) estimateEquiJoinStats();

  // ---------------------------------------------------------------------
  // Try to retrieve the ColStats information on the equi join columns
  // from the children's EstLogProp and prepare histograms for further
  // processing if we can. If operation is successful, results are stored
  // in {child0,child1,merged}EquiJoinColStats_.
  //
  // if(hasEquiJoinPred_)
  //   isColStatsMeaningful_ = mergeColStatsOnEquiJoinPred();
  // else
  //   isColStatsMeaningful_ = FALSE;
  // ---------------------------------------------------------------------

  // Store the partitioning functions of the children, if physical
  // properties are available.
  const PhysicalProperty* sppOfChild0 =
          myContext->getPhysicalPropertyOfSolutionForChild(0);

  if (sppOfChild0 != NULL)
    child0PartFunc_ = sppOfChild0->getPartitioningFunction();
  else
    child0PartFunc_ = NULL;

  const PhysicalProperty* sppOfChild1 =
          myContext->getPhysicalPropertyOfSolutionForChild(1);
  if (sppOfChild1 != NULL)
    child1PartFunc_ = sppOfChild1->getPartitioningFunction();
  else
    child1PartFunc_ = NULL;

}
//<pb>
// -----------------------------------------------------------------------
// CostMethodJoin::estimateEquiJoinStats()
//
// This method prepares column statistics information on the equi-join
// cols of the children and estimate the row count and uec in the result
// of doing an inner-join on those cols.
//
// Parameters computed by this method include:
// child0EquiJoinColUec_, child1EquiJoinColUec_, resultEquiJoinColUec_,
// and resultEquiJoinRowCount_.
// -----------------------------------------------------------------------
void CostMethodJoin::estimateEquiJoinStats()
{
  // ---------------------------------------------------------------------
  // $$$ In cases where noOfProbes_!=1, there is a pending problem on the
  // $$$ accuracy of the estimates of child0RowCount_ and child1RowCount_
  // $$$ and the way the result of the join is computed by Join::synthEst
  // $$$ LogProp().
  // ---------------------------------------------------------------------

  // This method is for MJ and HJ only.
  CMPASSERT(NOT jn_->isTSJ());

  // The cross product case.
  if(NOT hasEquiJoinPred_)
  {
    child0EquiJoinColUec_ = csOne;
    child1EquiJoinColUec_ = csOne;
    resultEquiJoinColUec_ = csOne;

    // $$$ Read comments at beginning of this method.

    CostScalar prodChild0Child1;

    //if (child1RowCount_.getValue() < 1.000001)
    //     child1RowCount_ = CostScalar(1.0);
    child1RowCount_.minCsOne();

    prodChild0Child1 = child0RowCount_ * child1RowCount_;

    resultEquiJoinRowCount_ = (prodChild0Child1 / noOfProbes_ ).minCsOne();
    //  MIN_ONE_CS(prodChild0Child1 / noOfProbes_ );

    return;
  }

  // ---------------------------------------------------------------------
  // First, estimate child0EquiJoinColUec_ and child1EquiJoinUec_.
  // ---------------------------------------------------------------------
  ValueIdSet equiJoinPreds;
  if ( jn_->isMergeJoin() AND
       ( NOT CURRSTMT_OPTDEFAULTS->optimizerPruning() OR
         CmpCommon::getDefault(OPH_USE_ORDERED_MJ_PRED) == DF_ON )
     )
     {
      MergeJoin * mj = (MergeJoin *)jn_;
      equiJoinPreds = mj->getOrderedMJPreds();
     }
  else
    equiJoinPreds = jn_->getEquiJoinPredicates();

  // The children's list of column statistics.
  ColStatDescList& child0ColStatDescList = child0LogProp_->colStats();
  ColStatDescList& child1ColStatDescList = child1LogProp_->colStats();

  // No of equijoin predicates which are not VEGPreds. Ex (x.a + 1 = y.b).
  Lng32 noOfNonVEGPreds = 0;

  // ---------------------------------------------------------------------
  // Products of the uec's from all the single column statistics of those
  // columns present in the VEG predicates.
  // ---------------------------------------------------------------------
  CostScalar child0UecProduct = csOne;
  CostScalar child1UecProduct = csOne;

  // Are we missing statistics for some of our VEG predicates ?
  CollIndex noOfVEGPredsWithMissingStats = 0;

  ValueIdSet equiJoinVEGPreds;
  for(ValueId pred = equiJoinPreds.init();
      equiJoinPreds.next(pred);
      equiJoinPreds.advance(pred))
  {
    const ItemExpr* predItemExpr = pred.getItemExpr();
    CMPASSERT(predItemExpr->isAPredicate());
    if(predItemExpr->getOperatorType() != ITM_VEG_PREDICATE)
    {
      noOfNonVEGPreds++;
    }
    else
    {
      equiJoinVEGPreds.insert(pred);

      // -----------------------------------------------------------------
      // Locate relevant statistics from child0/child1 for the VEG pred.
      // -----------------------------------------------------------------
      ColStatsSharedPtr child0ColStats =
        child0ColStatDescList.getColStatsPtrForPredicate(pred);

      ColStatsSharedPtr child1ColStats =
        child1ColStatDescList.getColStatsPtrForPredicate(pred);

      // -----------------------------------------------------------------
      // Retrieve uec information from the ColStats.
      // $$$ They shouldn't be NULL after prototype code get discarded.
      // -----------------------------------------------------------------
      if(child0ColStats != NULL AND child1ColStats != NULL)
      {
        const CostScalar & child0Uec = child0ColStats->getTotalUec();
        const CostScalar & child1Uec = child1ColStats->getTotalUec();

        child0UecProduct *= child0Uec;
        child1UecProduct *= child1Uec;
      }
      else noOfVEGPredsWithMissingStats++;
    }
  }

  // ---------------------------------------------------------------------
  // Use product of uec's of all join columns (up to a limit of the given
  // row count) to approximate uec of multiple join columns, if only one
  // of them is missing.
  //
  // $$$ In the future, we might want to try finding a multiple column
  // $$$ statistics describing all the join columns in the VEG predicates
  // $$$ instead, but multiple column histograms are not yet considered
  // $$$ in phase 1....
  // 
  // ---------------------------------------------------------------------

  if ( (equiJoinVEGPreds.entries() > noOfVEGPredsWithMissingStats) &&
       (noOfVEGPredsWithMissingStats < 2))
  {
    child0EquiJoinColUec_ = MINOF(child0UecProduct,child0RowCount_);
    child1EquiJoinColUec_ = MINOF(child1UecProduct,child1RowCount_);
  }
  else  // Too many VEG stats are missing.
  {
    child0EquiJoinColUec_ = child0RowCount_;
    child1EquiJoinColUec_ = child1RowCount_;
  }

  child0EquiJoinColUec_ = ( child0EquiJoinColUec_ ).minCsOne();
  child1EquiJoinColUec_ = ( child1EquiJoinColUec_ ).minCsOne();

  // ---------------------------------------------------------------------
  // Check if the equi-join predicates are just all predicates we have
  // and I am an inner-join. In such case, my output statistics are just
  // the result equi-join stats.
  // ---------------------------------------------------------------------
  if(jn_->isInnerNonSemiJoin())
  {
    // First three are just place holders. We need the forth one.
    ValueIdSet vs1;
    ValueIdSet vs2;
    ValueIdSet vs3;
    ValueIdSet otherPreds;
    classifyPredicates(vs1,vs2,vs3,otherPreds);

    // Case: 10-030611-2757 - BEGIN
    // If there are no other predicates evaluated at this join,
    // use the myRowCount_ as resultEquiJoinRowCount_ and return.
    if(otherPreds.isEmpty())
    {
      resultEquiJoinRowCount_ = myRowCount_;
      return;
    }
    // Case: 10-030611-2757 - END
  }

  // ---------------------------------------------------------------------
  // Build an inner Join node, include only equi-join predicates and call
  // Join::estimateCardinality() to synthesize result statistics.
  // ---------------------------------------------------------------------

  // Constructor join node and point it to same children as jn_.
  OperatorTypeEnum joinType = (jn_->isSemiJoin() || jn_->isAntiSemiJoin()? REL_SEMIJOIN: REL_JOIN);
  Join join(NULL,NULL,joinType,NULL,FALSE,TRUE);
  join[0] = (*jn_)[0];
  join[1] = (*jn_)[1];

  // Set its predicates to contain only the equi-join predicates of jn_.
  join.selectionPred() = equiJoinPreds;

  // Set up group attributes for join to store result in.
  GroupAttributes* ga = new STMTHEAP GroupAttributes;
  ga->setCharacteristicInputs(ga_->getCharacteristicInputs());
  ga->setCharacteristicOutputs(ga_->getCharacteristicOutputs());
  ga->addConstraints(ga_->getConstraints());
  join.setGroupAttr(ga);

  NABoolean inputCacheable = inLogProp_->isCacheable();
  
  // we don't want these stats to be cached in the ASM
  if (inputCacheable)
    inLogProp_->setCacheableFlag(FALSE);
    
  // Tell the join to synthesize its estimated logical properties.
  join.synthEstLogProp(inLogProp_);

  // Store result of equi join.
  resultEquiJoinRowCount_ =
    ( ga->outputLogProp(inLogProp_)->getResultCardinality() ).minCsOne();

  if (inputCacheable)
    inLogProp_->setCacheableFlag(TRUE);

  ga->clearAllEstLogProp();

  // cardinality after applying to a subset of predicates should not
  // go below the cardinality after applying all predicates
  // myRowCount_ is the cardinality after applying all predicates
  resultEquiJoinRowCount_ = MAXOF(resultEquiJoinRowCount_, myRowCount_);


}  // CostMethodJoin::estimateEquiJoinStats().
//<pb>
// -----------------------------------------------------------------------
// CostMethodJoin::estimateDegreeOfParallelism().
//
// Makes use of the generic implementation. Adds logic for dealing with
// the children's per stream row counts. Assumed cacheParameters() has
// been called.
//
// Parameters computed by this method include:
// child0RowCountPerStream_, child1RowCountPerStream_,
// child0UecPerStream_, child1UecPerStream_,
// equiJnRowCountPerStream_ and equiJnUecPerStream_.
//
// The latter 4 are not computed if the join is a NestedJoin, since they
// are currently not used in the cost estimation of NestedJoin.
// -----------------------------------------------------------------------
void CostMethodJoin::estimateDegreeOfParallelism()
{
  GroupAttributes * child0GA = jn_->child(0).getGroupAttr();
  GroupAttributes * child1GA = jn_->child(1).getGroupAttr();

  CostMethod::estimateDegreeOfParallelism();
  NABoolean doNotPenalizeSkew = FALSE;
 
  // Operator is a NestedJoin.
  if(jn_->isTSJ())
  {
    const ReqdPhysicalProperty* rppForMe = context_->getReqdPhysicalProperty();
    if (((NestedJoin*)jn_)->isProbeCacheApplicable(rppForMe->getPlanExecutionLocation()))
      doNotPenalizeSkew = TRUE;

    // The uec's are not currently being used in NJ cost analysis.
	  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
		  (partFunc_ != NULL) &&
                  (doNotPenalizeSkew == FALSE))
	  {
		child0RowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                          countOfStreams_,
                          child0GA,
                          countOfAvailableCPUs_);
		child1RowCountPerStream_ = child1LogProp_->getCardOfBusiestStream(partFunc_,
                          countOfStreams_,
                          child1GA,
                          countOfAvailableCPUs_);
	  }
	  else
	  {
		child0RowCountPerStream_ =  ( child0RowCount_ / countOfStreams_ ).minCsOne();
		child1RowCountPerStream_ =  ( child1RowCount_ / countOfStreams_ ).minCsOne();
	  }
  }
  else
  {
    // -----------------------------------------------------------------
    // For HJ and MJ, we are not going to try a SQL/MP style PLAN 1 if
    // there are no equi-join predicates. The only thing we consider is
    // a PLAN 2, which replicates the right child.
    // -----------------------------------------------------------------
    if(NOT hasEquiJoinPred_)
    {
      // ---------------------------------------------------------------
      // If a HJ or MJ has no equi-join predicates, at plan generation,
      // a predicate of (1==1) which is always TRUE is generated for the
      // join so that a cross product is resulted. That's why the uec's
      // on the join column is 1, since its value is same for all rows.
      // ---------------------------------------------------------------
	  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
		  (partFunc_ != NULL) )
	  {
		child0RowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                            countOfStreams_,
                            child0GA,
                            countOfAvailableCPUs_);
	  }
	  else
		child0RowCountPerStream_ =  ( child0RowCount_ / countOfStreams_ ).minCsOne();
      child0UecPerStream_ = csOne;
      child1RowCountPerStream_ = child1RowCount_;
      child1UecPerStream_ = csOne;

      // ---------------------------------------------------------------
      // $$$ Same note as in estimateEquiJoinStats() applies.
      // ---------------------------------------------------------------
      CostScalar prodCh0Ch1;

      //if (child1RowCountPerStream_.getValue() < 1.000001)
      //   child1RowCountPerStream_ = CostScalar(1.0);
      child1RowCountPerStream_.minCsOne();
      prodCh0Ch1 = child0RowCountPerStream_ * child1RowCountPerStream_;

      equiJnRowCountPerStream_ = prodCh0Ch1 / noOfProbes_;
      equiJnUecPerStream_ = csOne;
    }
    else
    // -----------------------------------------------------------------
    // Otherwise, we try both PLAN 1 and PLAN 2. If we are on our
    // way down, then we don't know for sure, since the partitioning
    // function of the right child is not available yet and we don't
    // have access to the plan number. So in this case we will
    // underestimate, i.e. assume plan1. If the partitioning function
    // of the right child is available then we are on our way back
    // up and we can tell for sure.
    // -----------------------------------------------------------------
    {
      // ---------------------------------------------------------------
      // Try to use colstats to estimate row counts for a representative
      // stream. If that fails, just assume even distribution.
      // ---------------------------------------------------------------
      // LCOV_EXCL_START :cnu
      // excluded for coverage because below code is disabled
      if(isColStatsMeaningful_)
      {
        // -------------------------------------------------------------
        // $$$ This should never be the code path taken in Phase 1 !!
        // -------------------------------------------------------------
        CMPASSERT(FALSE);

        if(NOT computeRepresentativeStream())
        {
          // -----------------------------------------------------------
          // computeRepresentativeStream() fails, just assume even
          // distribution of row counts across all available streams.
          // -----------------------------------------------------------
		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			child0RowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                                 countOfStreams_,
                                 child0GA,
                                 countOfAvailableCPUs_);
		  }
		  else
			child0RowCountPerStream_ =  ( child0RowCount_ / countOfStreams_ ).minCsOne();
          child0UecPerStream_ =
	    (	child0EquiJoinColStats_->getTotalUec()
			/ countOfStreams_ ).minCsOne();
          // For replicate broadcast, each stream must process all the
          // child1 rows, i.e. this is a Type-2 join.  Also assume a
          // Type-2 join if we are on the way down the tree (i.e. if
          // child1PartFunc is NULL).
          if ((child1PartFunc_ == NULL) OR
              child1PartFunc_->isAReplicateViaBroadcastPartitioningFunction())
          {
            child1RowCountPerStream_ = child1RowCount_;
            child1UecPerStream_ = child1EquiJoinColStats_->getTotalUec();
          }
          else
          {
            // No replication, or the right child partitioning function
            // is not available. Assume each stream only processes a portion
            // of the child1 rows, i.e. a Type1 join.
			if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
				(partFunc_ != NULL) )
			{
			  child1RowCountPerStream_ = child1LogProp_->getCardOfBusiestStream(partFunc_,
                                 countOfStreams_,
                                 child1GA,
                                 countOfAvailableCPUs_);
			}
			else
			  child1RowCountPerStream_ =  ( child1RowCount_ / countOfStreams_ ).minCsOne();
            child1UecPerStream_ =
	      ( child1EquiJoinColStats_->getTotalUec()
			  / countOfStreams_ ).minCsOne();
          }

          // -----------------------------------------------------------
          // This might give an illogical result, but since compute
          // RepresentativeStream() fails, we have no other better ways.
          // -----------------------------------------------------------
		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			ColStatDescList equiJoinColStatDescList;

			equiJoinColStatDescList.insert(mergedEquiJoinColStatDesc_);

			equiJnRowCountPerStream_ = equiJoinColStatDescList.getCardOfBusiestStream(partFunc_,
														  countOfStreams_,
														  child0GA,
														  countOfAvailableCPUs_);
		  }
		  else
			equiJnRowCountPerStream_ =
			  (mergedEquiJoinColStats_->getRowcount() / countOfStreams_).minCsOne();

          equiJnUecPerStream_ =
            mergedEquiJoinColStats_->getTotalUec() / countOfStreams_;
        }
        else
        {
          // -----------------------------------------------------------
          // The stream row counts and uec's are already set by
          // computeRepresentativeStream. Nothing needs to be done.
          // -----------------------------------------------------------
        }
      }
      // LCOV_EXCL_STOP
      else
      // ---------------------------------------------------------------
      // $$$ This is always the code path taken right now, since the
      // $$$ code for merging histogram stats is being reconsidered. As
      // $$$ a result, isColStatsMeaningful_ is always FALSE.
      // ---------------------------------------------------------------
      {
        // -------------------------------------------------------------
        // Use the values obtained from estimateEquiJoinStats().
        // -------------------------------------------------------------
		if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			(partFunc_ != NULL) )
		{
		  child0RowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                            countOfStreams_,
                            child0GA,
                            countOfAvailableCPUs_);
		}
		else
		  child0RowCountPerStream_ =  ( child0RowCount_ / countOfStreams_ ).minCsOne();
        child0UecPerStream_ =
	  ( child0EquiJoinColUec_ / countOfStreams_ ).minCsOne();
        // For replicate broadcast, each stream must process all the
        // child1 rows, i.e. this is a Type-2 join.
        // If we are on the way down the tree (child1PartFunc is NULL) and
        // if planNumber is 0, then it's a Type-2 join.
        Lng32 planNumber = -1;
        PlanWorkSpace* myPws = context_->getPlanWorkSpace();
        if (myPws != NULL && (myPws->getCountOfChildContexts() <= 2))
          planNumber = PLAN0;

        NABoolean type2Plan;
        if (ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_95) == 0)
        {
          if ( jn_->isHashJoin()   AND
               planNumber == PLAN0 AND
               (child1PartFunc_ == NULL OR
                child1PartFunc_->isAReplicateViaBroadcastPartitioningFunction())
             )
            type2Plan = TRUE;
          else
            type2Plan = FALSE;
        }
        else
        {
          if ((child1PartFunc_ == NULL) OR
              child1PartFunc_->isAReplicateViaBroadcastPartitioningFunction())
            type2Plan = TRUE;
          else
            type2Plan = FALSE;
        }

        if (type2Plan)
        {
          child1RowCountPerStream_ = child1RowCount_;
          child1UecPerStream_ = child1EquiJoinColUec_;
        }
        else
        {
          // No replication, or the right child partitioning function
          // is not available. Assume each stream only processes a portion
          // of the child1 rows, i.e. a Type1 join.
		  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
			  (partFunc_ != NULL) )
		  {
			child1RowCountPerStream_ = child1LogProp_->getCardOfBusiestStream(partFunc_,
                               countOfStreams_,
                               child1GA,
                               countOfAvailableCPUs_);
		  }
		  else
			child1RowCountPerStream_ =  ( child1RowCount_ / countOfStreams_ ).minCsOne();
          child1UecPerStream_ =
	    ( child1EquiJoinColUec_ / countOfStreams_ ).minCsOne();
        }

        // -------------------------------------------------------------
        // $$$ This, together with the above statistics, may give an
        // $$$ illogical picture. A good solution to deal with this is
        // $$$ yet to be devised.
        // -------------------------------------------------------------
        equiJnRowCountPerStream_ =
          (resultEquiJoinRowCount_ / countOfStreams_);

      } // endif(NOT computeRepresentativeStream())
    } // endif(NOT hasEquiJoinPred_)
  } // endif(jn_->isTSJ())

} // CostMethodJoin::estimateDegreeOfParallelism().
//<pb>
// -----------------------------------------------------------------------
// CostMethodJoin::computeRepresentativeStream().
//
// This method tries to use more sophisticated techniques to come up with
// what is called "a representative stream" for costing. Essentially it
// is an imagined stream with an effective cost which best represents the
// actual stream cost of the operation in the cases where there maybe an
// inherent uneven distribution of workload across different streams.
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu
NABoolean CostMethodJoin::computeRepresentativeStream()
{
  // This method needs more refinement and thoughts...
  return FALSE;

#if 0
  GroupAttributes * child0GA = jn_->child(0).getGroupAttr();
  GroupAttributes * child1GA = jn_->child(1).getGroupAttr();

  // This is essentially an implementation shared by MJ and HJ but not NJ.
#pragma nowarn(203)   // warning elimination
  if(jn_->isTSJ()) return FALSE;
#pragma warn(203)  // warning elimination

  // Cannot do better if no colstats are available for analysis.
  if(NOT isColStatsMeaningful_) return FALSE;

  // Simple case.
  if(countOfStreams_ == 1)
  {
    child0RowCountPerStream_ = child0RowCount_;
    child0UecPerStream_ = child0EquiJoinColStats_->getTotalUec();
    child1RowCountPerStream_ = child1RowCount_;
    child1UecPerStream_ = child1EquiJoinColStats_->getTotalUec();
    /*
    equiJnRowCountPerStream_ = mergedEquiJoinColStats_->getRowcount();
    equiJnUecPerStream_ = mergedEquiJoinColStats_->getTotalUec();
    */
    return TRUE;
  }

  // ---------------------------------------------------------------------
  // Check if the no of uec's present limits our degree of parallelism.
  // If yes, we could use at most a number of streams equal to the total
  // no of uec's could be active. In such a case, we consider assigning
  // one uec to each stream, and cost the stream with the largest row
  // count per uec.
  //
  // More analysis could be done in Phase 2 to pick a better stream...
  // For example,
  //
  // 1. Compute average row count per uec from the merged statistics.
  // 2. Compute average row connt per uec for each merged histogram int.
  // 3. If (2>1) there exists one unique value with many rows. For the
  //    join to be done correctly, all of such rows must come from the
  //    same stream. Now, the cost of such a stream is much higher than
  //    the rest due to data skew. For LR cost elapsed time optimization,
  //    we should pick such a stream to cost. On the other hand, for FR
  //    cost, we should in fact just pick the average stream.
  // 4. Otherwise, we could believe in having a good hash partitioning
  //    function or partitioning boundaries such that the workload are
  //    spread evenly across.
  // ---------------------------------------------------------------------

  // countOfStreams_ is designed to be smaller than INT_MAX.
  if (maxDegreeOfParallelism_.value() < double(INT_MAX))
  {
    if (countOfStreams_ > Lng32(maxDegreeOfParallelism_.value()))
    {
      // Not all streams could be active.
      countOfStreams_ = Lng32(maxDegreeOfParallelism_.value());

      CollIndex intIndex;
      // Pick biggest merged result set to be our representative stream.
      intIndex = mergedEquiJoinColStats_->getHistogram()->
                               pickIntervalWithLargestRowCountPerUec();
      const HistInt& histInt0 =
                    child0EquiJoinColStats_->getHistogram()->at(intIndex);
      child0RowCountPerStream_ =
                    histInt0.getCardinality() / histInt0.getFudgedUec();
      const HistInt& histInt1 =
	            child1EquiJoinColStats_->getHistogram()->at(intIndex);
      child1RowCountPerStream_ =
                    histInt1.getCardinality() / histInt1.getFudgedUec();
      child0UecPerStream_ = csOne;
      child1UecPerStream_ = csOne;
      const HistInt& histIntM =
	            mergedEquiJoinColStats_->getHistogram()->at(intIndex);
      equiJnRowCountPerStream_ =
                    histIntM.getCardinality() / histIntM.getFudgedUec();
      equiJnUecPerStream_ = csOne;
      return TRUE;
    }
  }

  // ---------------------------------------------------------------------
  // For now, just come up with some averages in the other case where
  // parallelism is not limited by uec.
  // ---------------------------------------------------------------------
  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
	  (partFunc_ != NULL) )
  {
	child0RowCountPerStream_ = child0LogProp_->getCardOfBusiestStream(partFunc_,
                           countOfStreams_,
                           child0GA,
                           countOfAvailableCPUs_);
  }
  else
	child0RowCountPerStream_ =  ( child0RowCount_ / countOfStreams_ ).minCsOne();
  child0RowCountPerStream_ = (child0RowCountPerStream_).minCsOne();
  child0UecPerStream_ =
                 child0EquiJoinColStats_->getTotalUec() / countOfStreams_;
  child0UecPerStream_ = (child0UecPerStream_).minCsOne();
  // For replicate broadcast, each stream must process all the
  // child1 rows, i.e. this is a Type-2 join.  Also assume a
  // Type-2 join if we are on the way down the tree (i.e. if
  // child1PartFunc is NULL).
  if ((child1PartFunc_ == NULL) OR
      child1PartFunc_->isAReplicateViaBroadcastPartitioningFunction())
  {
    child1RowCountPerStream_ = child1RowCount_;
    child1UecPerStream_ = child1EquiJoinColStats_->getTotalUec();
  }
  else
  {
    // No replication, or the right child partitioning function
    // is not available. Assume each stream only processes a portion
    // of the child1 rows, i.e. a Type1 join.
	if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
		(partFunc_ != NULL) )
	{
	  child1RowCountPerStream_ = child1LogProp_->getCardOfBusiestStream(partFunc_,
                   countOfStreams_,
                   child1GA,
                   countOfAvailableCPUs_);
	}
	else
	  child1RowCountPerStream_ =  ( child1RowCount_ / countOfStreams_ ).minCsOne();
    child1UecPerStream_ =
      child1EquiJoinColStats_->getTotalUec() / countOfStreams_;
    child1UecPerStream_ = (child1UecPerStream_).minCsOne();
  }

  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
	  (partFunc_ != NULL) )
  {
	ColStatDescList equiJoinColStatDescList;

	equiJoinColStatDescList.insert(mergedEquiJoinColStatDesc_);

	equiJnRowCountPerStream_ = equiJoinColStatDescList.getCardOfBusiestStream(partFunc_,
												  countOfStreams_,
												  child0GA,
												  countOfAvailableCPUs_);
  }
  else
	equiJnRowCountPerStream_ = (mergedEquiJoinColStats_->getRowcount() /
															countOfStreams_).minCsOne();

  equiJnUecPerStream_ = mergedEquiJoinColStats_->getTotalUec() /
                                                          countOfStreams_;
  if(equiJnUecPerStream_ > MINOF(child0UecPerStream_,child1UecPerStream_))
     equiJnUecPerStream_ = MINOF(child0UecPerStream_,child1UecPerStream_);
  CostScalar maxEquiJnRowCountPerStream =
                    (child0RowCountPerStream_ * child1RowCountPerStream_)/
                           MAXOF(child0UecPerStream_,child1UecPerStream_);
  equiJnRowCountPerStream_ =
               MINOF(equiJnRowCountPerStream_,maxEquiJnRowCountPerStream);

  return TRUE;
#endif // 0

}  // CostMethodJoin::computeRepresentativeStream().
//<pb>
// -----------------------------------------------------------------------
// CostMethodJoin::mergeHistogramsOnEquiJoinPred().
//
// This method merges the column statistics on the left child of the col
// referenced in the equi-join predicate with the column statistics on
// the right child of the col referenced in the same predicate.
// -----------------------------------------------------------------------
NABoolean CostMethodJoin::mergeHistogramsOnEquiJoinPred()
{
  const ValueIdSet& equiJoinPred = jn_->getEquiJoinPredicates();

  // Don't do multi-column predicates for now.
  if(equiJoinPred.isEmpty() OR equiJoinPred.entries() != 1) return FALSE;

  const ValueIdList& child0ColList = jn_->getEquiJoinExprFromChild0();
  CMPASSERT(child0ColList.entries() == 1);
  const ValueIdList& child1ColList = jn_->getEquiJoinExprFromChild1();
  CMPASSERT(child1ColList.entries() == 1);

  // The objects under the ValueId's are VEGRefs.
  const ValueId & child0Col = child0ColList[0];
  const ValueId & child1Col = child1ColList[0];

  // Fix for coverity cid #1512 : pointers child1ColItemExpr
  // child0ColItemExpr are unused.
  // const ItemExpr* child0ColItemExpr = child0Col.getItemExpr();
  // const ItemExpr* child1ColItemExpr = child1Col.getItemExpr();

  const ColStatDescList& child0ColStats = child0LogProp_->colStats();
  const ColStatDescList& child1ColStats = child1LogProp_->colStats();

  // List of indices of child0ColStats which reference child0Col.
  NAList<CollIndex> child0RefColStatsIndices(CmpCommon::statementHeap());
  NAList<CollIndex> placeHolder(CmpCommon::statementHeap());

  // $$$ Needed to change method to public.
  // child0ColStats.identifyMergeCandidates(child0ColItemExpr,
  //                                        child0RefColStatsIndices,
  //                                        placeHolder);
  if(child0RefColStatsIndices.isEmpty()) return FALSE;

  // List of indices of child1ColStats which reference child1Col.
  NAList<CollIndex> child1RefColStatsIndices(CmpCommon::statementHeap());

  // $$$ Needed to change method to public.
  // child1ColStats.identifyMergeCandidates(child1ColItemExpr,
  //                                        child1RefColStatsIndices,
  //                                        placeHolder);
  if(child1RefColStatsIndices.isEmpty()) return FALSE;

  // Should there be only one histogram from one side which can be merged?
  if(child0RefColStatsIndices.entries() != 1 OR
                    child1RefColStatsIndices.entries() != 1) return FALSE;

  // ---------------------------------------------------------------------
  // Get the column statistics to merge. If histograms are just faked,
  // normal even distribution assumption will do the job.
  // ---------------------------------------------------------------------
  CollIndex child0RootIndex = child0RefColStatsIndices[0];
  ColStatDescSharedPtr child0RootDesc = child0ColStats[child0RootIndex];
  CollIndex child1RootIndex = child1RefColStatsIndices[0];
  ColStatDescSharedPtr child1RootDesc = child1ColStats[child1RootIndex];

  ColStatDescList myColStatDescList(CmpCommon::statementHeap());
  myColStatDescList.insertDeepCopyAt(0,child0RootDesc);
  myColStatDescList.insertDeepCopyAt(1,child1RootDesc);
  myColStatDescList.insertIntoUecList (child0ColStats.getUecList()) ;
  myColStatDescList.insertIntoUecList (child1ColStats.getUecList()) ;

  // Get the real working copy of column statistics objects from the desc.
  ColStatsSharedPtr child0ColStat = myColStatDescList[0]->getColStatsToModify();
  if(child0ColStat->isFakeHistogram()) return FALSE;

  ColStatsSharedPtr child1ColStat = myColStatDescList[1]->getColStatsToModify();
  if(child1ColStat->isFakeHistogram()) return FALSE;

  // Store them in cache.
  child0EquiJoinColStats_ = child0ColStat;
  child1EquiJoinColStats_ = child1ColStat;

  // Original histograms of children.
  HistogramSharedPtr child0Histogram = child0ColStat->getHistogramToModify();
  HistogramSharedPtr child1Histogram = child1ColStat->getHistogramToModify();

  // A template will be made out of the histograms in children's ColStats.
  HistogramSharedPtr child0Template = child0Histogram->
                                createMergeTemplate(child1Histogram,TRUE /*equiMerge*/);

  // Make a template for child1 as well.
  HistogramSharedPtr child1Template(new HISTHEAP Histogram(*child0Template, HISTHEAP));

  // Make a template for the merged histogram.
  HistogramSharedPtr mergedTemplate(new HISTHEAP Histogram(*child0Template, HISTHEAP));

  // Set the template's row counts and uec's.

  ColStats child0tmp ( child1Template, STMTHEAP );
  ColStats child1tmp ( child0Template, STMTHEAP );
  child0tmp.populateTemplate (child0ColStat);
  child1tmp.populateTemplate (child1ColStat);
  // child0tmp, child1tmp not used after this point

  // be careful! populateTemplate may have compressed the intervals if
  // the resulting rowcount was too low!
  if ( child0Template->entries() != child1Template->entries() OR
       child0Template->entries() != mergedTemplate->entries() )
    {
      child0Template->condenseToSingleInterval() ; // one of these
      child1Template->condenseToSingleInterval() ; // is redundant
      mergedTemplate->condenseToSingleInterval() ;
    }
  CMPASSERT ( child0Template->entries() == child1Template->entries() ) ;
  CMPASSERT ( child0Template->entries() == mergedTemplate->entries() ) ;

  // ---------------------------------------------------------------------
  // Replace the histograms in the children's ColStats with the templates
  // so that the interval numbers match with the merged histogram. The
  // original histograms needn't be deallocated since the original
  // children's ColStats are pointing to them.
  // ---------------------------------------------------------------------
  child0ColStat->setHistogram(child0Template);
  child1ColStat->setHistogram(child1Template);
  child0ColStat->setRedFactor	(csOne);
  child1ColStat->setRedFactor   (csOne);
  child0ColStat->setUecRedFactor(csOne);
  child1ColStat->setUecRedFactor(csOne);

  CostScalar rowCount, uec, totalRowCount, totalUec;
  CostScalar child0Uec, child0RowCount, child1Uec, child1RowCount;
  CollIndex i(1);

  // To be recomputed.
  maxDegreeOfParallelism_ = csZero;

  // Perform the merge.
  while(i < mergedTemplate->entries())
  {
    child0RowCount = ((*child0Template)[i].getCardinality());
    child0Uec      = ((*child0Template)[i].getUec());
    child1RowCount = ((*child1Template)[i].getCardinality());
    child1Uec      = ((*child1Template)[i].getUec());

    maxDegreeOfParallelism_ += MAXOF(child0Uec,child1Uec);

    uec = MINOF(child0Uec,child1Uec);
    if(uec.isGreaterThanZero() /* > csZero */)
      rowCount = (child0RowCount * child1RowCount) /
                                               MAXOF(child0Uec,child1Uec);
    else
      rowCount = csZero;

    (*mergedTemplate)[i].setCardAndUec(rowCount, uec);
    totalRowCount += rowCount;
    totalUec += uec;
    i++;
  }
  maxDegreeOfParallelism_ = (maxDegreeOfParallelism_).minCsOne();

  // ---------------------------------------------------------------------
  // Synthesize the merged ColsStat. Note that we don't set the min/max
  // value of this ColStats since they are not used in later computation.
  // ---------------------------------------------------------------------
  ComUID id(ColStats::nextFakeHistogramID());
  mergedEquiJoinColStats_ = ColStatsSharedPtr(
    new HISTHEAP ColStats( id,
			   totalUec,
			   totalRowCount,
// added baseRowCount for testing initialized baseRowCount with totalRowCount
// 11/30 RV
			   totalRowCount,
			   FALSE,
			   FALSE,
			   mergedTemplate,
			   FALSE,
			   csOne,     // default row reduction factor
			   csOne,     // default uec reduction factor
                           -1,        // default avg VarChar size
			   HISTHEAP));

  ColStatDescSharedPtr equiJoinStatDesc(new (HISTHEAP)
                 ColStatDesc (mergedEquiJoinColStats_,child0Col), HISTHEAP);

  
  equiJoinStatDesc->VEGColumn() = child0Col;

  equiJoinStatDesc->mergeState().clear() ;
  equiJoinStatDesc->mergeState().insert(child0Col);
  equiJoinStatDesc->mergeState().insert(child1Col);
  mergedEquiJoinColStatDesc_ = equiJoinStatDesc;

  return TRUE;
}
// LCOV_EXCL_STOP
//<pb>
// -----------------------------------------------------------------------
// CostMethodJoin::classifyPredicates().
//
// The method classifies the predicates this Hash Join or Merge Join is
// evaluating into three types - Equi Join preds, other join preds and
// selection preds. The result is a set of inner equi-join keys and outer
// equi-join keys with the other predicates.
// -----------------------------------------------------------------------
void CostMethodJoin::classifyPredicates(ValueIdSet& innerEquiJoinKeys,
                                        ValueIdSet& outerEquiJoinKeys,
                                        ValueIdSet& otherJoinPreds,
                                        ValueIdSet& otherSelPreds)
{
  CMPASSERT(NOT jn_->isTSJ());

  // ---------------------------------------------------------------------
  // Hash join predicates are equi join predicates which have already been
  // chosen by the Hash Join Implementation Rule.
  // ---------------------------------------------------------------------
  innerEquiJoinKeys.insertList(jn_->getEquiJoinExprFromChild1());
  outerEquiJoinKeys.insertList(jn_->getEquiJoinExprFromChild0());

  // ---------------------------------------------------------------------
  // For left joins, we might have predicates both in joinPred() as well
  // as selectionPred(). joinPred() of left joins have to be handled
  // specially, since we could not push down even covered predicates like
  // VEGPred(VEG{T.x,5}) to the left child T. Those rows not with
  // (T.x != 5) have to be null-instantiated by the left join itself
  // rather than thrown away by the left child directly.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Consider joinPred() first for left-joins and semi-joins. Inner non-
  // semi join has selectionPred() only.
  // ---------------------------------------------------------------------
  ValueIdSet predSet = jn_->joinPred();
  NABoolean isConsideringJoinPred = TRUE;
  if(jn_->isInnerNonSemiJoin())
  {
    // -------------------------------------------------------------------
    // This is sometimes not true when the join is originally an outer
    // join transformed into an inner join. In that case, the joinPred()
    // is not cleared, but nevertheless the predicates are copied to
    // selectionPred(), thus, they needn't be considered anymore.
    // CMPASSERT(predSet.isEmpty());
    // -------------------------------------------------------------------
    predSet = jn_->selectionPred();
    isConsideringJoinPred = FALSE;
  }

  // Use break to get out of loop.
  while(1)
  {
    ValueIdSet predsToKeep;
    for( ValueId pred = predSet.init();
	 predSet.next( pred );
	 predSet.advance( pred ) )
    {
      const ItemExpr* itemPred = pred.getItemExpr();

      // -----------------------------------------------------------------
      // Normally we don't keep VEG predicates. VEG without constants in
      // them and covered by the both children of this join node have
      // already been moved away as equi-join preds. The ones left behind
      // are either those uncovered (which are most probably to evaluated
      // at a parent join node rather than this one), or with constants in
      // them, which are not true join predicates and will eventually get
      // pushed down. An exception to this are the joinPred() of a left
      // join, at which a VEG predicate with a reference to the output of
      // its left child will not get pushed down despite it has constants
      // in it. For example, we could not push down VEGPred(VEG{T.x,5})
      // to the left child T. Those rows with (T.x != 5) have to be null-
      // instantiated by the left join itself rather than get thrown away
      // by the left child directly.
      // -----------------------------------------------------------------
      if(itemPred->getOperatorType() == ITM_VEG_PREDICATE)
      {
        if(jn_->isLeftJoin() AND isConsideringJoinPred)
        {
          const VEG* predVEG = ((VEGPredicate *) itemPred)->getVEG();
          const ValueIdSet& VEGGroup = predVEG->getAllValues();

          ItemExpr* constant = NULL;
          if(VEGGroup.referencesAConstValue(&constant))
          {
            const ValueId & VEGGroupId =
	      predVEG->getVEGReference()->getValueId();

            if(jn_->child(0).getGroupAttr()->
                                       isCharacteristicOutput(VEGGroupId))
	    {
	      // -----------------------------------------------------------
	      // VEG with a constant and a reference to an output from the
	      // left child. It is to be evaluated at this join node because
	      // it can not be pushed down to the left. (See comments above)
	      // -----------------------------------------------------------
	      predsToKeep.insert(pred);
	    }
          }
        }
      }
      else
      {
	// -----------------------------------------------------------------
	// Other cases are OR predicates, inequality join predicates which
	// are to be kept. Inequality non-join predicates are already pushed
	// down during normalization and shouldn't appear here.
	// -----------------------------------------------------------------
	predsToKeep.insert(pred);
      }

    } // end of for-loop iterating the predicate set.

    if(isConsideringJoinPred)
    {
      otherJoinPreds = predsToKeep;
      isConsideringJoinPred = FALSE;
      predsToKeep.clear();
      predSet = jn_->selectionPred();
      if(jn_->isSemiJoin() || jn_->isAntiSemiJoin())
      {
        CMPASSERT(predSet.isEmpty());
        break;
      }
    }
    else
    {
      otherSelPreds = predsToKeep;
      break;
    }
  } // end of while-loop.
}
//<pb>
// -----------------------------------------------------------------------
// CostMethodJoin::cleanUp()
//
// The method cleans up cached parameters which need deallocation and
// should be called after a costing session is done.
// -----------------------------------------------------------------------
void CostMethodJoin::cleanUp()
{
  // Make sure we deallocate ColStats stored during the last invocation.
  if(isColStatsMeaningful_)
  {
    CMPASSERT(child0EquiJoinColStats_ != NULL);
    CMPASSERT(child1EquiJoinColStats_ != NULL);
    CMPASSERT(mergedEquiJoinColStats_ != NULL);
    isColStatsMeaningful_ = FALSE;
    child0EquiJoinColStats_ = NULL;
    child1EquiJoinColStats_= NULL;
    mergedEquiJoinColStats_= NULL;
    mergedEquiJoinColStatDesc_ = NULL;
  }
  // Case: 10-030611-2757 - BEGIN
  // Cleaning the variable resultEquiJoinRowCount_ here to take care
  // of the problem in reusing it without initializing.
  // Similarly all the other variables also need to be cleaned here.
  resultEquiJoinRowCount_ = csZero;
  // Case: 10-030611-2757 - END

  // Reset the EstLogPropSharedPtr objects
  child0LogProp_ = 0;
  child1LogProp_ = 0;

  // Clean up fields in base class
  CostMethod::cleanUp();
  
}  // CostMethodJoin::cleanUp().
//<pb>

// ----QUICKSEARCH FOR HJ.................................................

/**********************************************************************/
/*                                                                    */
/*                         CostMethodHashJoin                         */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodHashJoin::cacheParameters().
// -----------------------------------------------------------------------
void CostMethodHashJoin::cacheParameters(RelExpr* op,
                                         const Context* myContext)
{
  CostMethodJoin::cacheParameters(op,myContext);

  // if child1PartFunc_ is NULL, which means going down the tree,
  // get partition function from child context that has been already optimized.
  
  if (child1PartFunc_ == NULL)
  {
    Lng32 planNumber = 0;
    PlanWorkSpace* myPws = myContext->getPlanWorkSpace();
    if (myPws != NULL)
      planNumber = myPws->getLatestPlan();

    if (planNumber > PLAN0)
    {
      Context* childContext;

      if (planNumber == PLAN1)
        // get child0 context to access child0 partFunc
        childContext = myPws->getChildContext(0, planNumber);
      else
        childContext = myPws->getChildContext(1, planNumber);

      if ( childContext != NULL && childContext->hasOptimalSolution() )
      {
        const PhysicalProperty* sppOfChild = 
          childContext->getPhysicalPropertyForSolution();
        if (sppOfChild != NULL)
          // Though we assign child0 partFunc to child1 partFunc, it's 
          // really not going to harm in this case, because it's being used
          // only for the purpose of preliminary cost estimation
          child1PartFunc_ = sppOfChild->getPartitioningFunction();
      }
    }
  }
  

  hj_ = (HashJoin*) op;

  // ---------------------------------------------------------------------
  // Find out what the predicates are to be evaluated at this HJ node,
  // and compute cost primitives related to them.
  // ---------------------------------------------------------------------
  ValueIdSet innerHashKeys;
  ValueIdSet outerHashKeys;
  ValueIdSet otherJoinPreds;
  ValueIdSet otherSelPreds;
  classifyPredicates(
                innerHashKeys,outerHashKeys,otherJoinPreds,otherSelPreds);

  if(innerHashKeys.isEmpty())
  {
    cpuCostHashRow_ = csZero;
    cpuCostCompareHashKeys_ = csZero;
  }
  else
  {
    cpuCostHashRow_ = CostPrimitives::cpuCostForHash(innerHashKeys);
    cpuCostCompareHashKeys_ =
                         CostPrimitives::cpuCostForCompare(innerHashKeys);
  }

  if(otherJoinPreds.isEmpty())
    cpuCostEvalOtherJoinPreds_ = csZero;
  else
    cpuCostEvalOtherJoinPreds_ = CostPrimitives::
                                       cpuCostForEvalPred(otherJoinPreds);
  if(otherSelPreds.isEmpty())
    cpuCostEvalOtherSelPreds_ = csZero;
  else
    cpuCostEvalOtherSelPreds_ = CostPrimitives::
                                        cpuCostForEvalPred(otherSelPreds);

  if(hj_->isLeftJoin())
  {
    if(hj_->nullInstantiatedOutput().isEmpty())
      cpuCostNullInst_ = csZero;
    else
      cpuCostNullInst_ = CostPrimitives::
          cpuCostForCopyRow(hj_->nullInstantiatedOutput().getRowLength());
  }

  // Length of a row from the left table.
  GroupAttributes* child0GA = hj_->child(0).getGroupAttr();
#pragma nowarn(1506)   // warning elimination
  child0RowLength_ = child0GA->getRecordLength();
#pragma warn(1506)  // warning elimination
  extChild0RowLength_ = child0RowLength_ + hashedRowOverhead_;

  // Length of a row from the right table.
  GroupAttributes* child1GA = hj_->child(1).getGroupAttr();
#pragma nowarn(1506)   // warning elimination
  child1RowLength_ = child1GA->getRecordLength();
#pragma warn(1506)  // warning elimination
  extChild1RowLength_ = child1RowLength_ + hashedRowOverhead_;

  // Cost for making a copy of those rows to the local buffer.
  cpuCostCopyChild0Row_ = CostPrimitives::
                                      cpuCostForCopyRow(child0RowLength_);
  cpuCostCopyChild1Row_ = CostPrimitives::
                                      cpuCostForCopyRow(child1RowLength_);

  // Cost for the whole set of outer rows to probe the hash table.
  cpuCostTotalProbing_ = computeTotalProbingCost();

  // temporary vectors must be NULL:
  DCMPASSERT(stage1cvBK_ == NULL);
  DCMPASSERT(stage2cvBK_ == NULL);
  DCMPASSERT(stage2cvFR_== NULL);
  DCMPASSERT(stage2cvLR_ == NULL);
  DCMPASSERT(stage3cvFR_ == NULL);
  DCMPASSERT(stage3cvLR_ == NULL);
  DCMPASSERT(stage3cvBK_ == NULL);

}
//<pb>
// -----------------------------------------------------------------------
// CostMethodHashJoin::deriveParameters().
//
// This method computes derived parameters associated with a HJ's three
// stages of operation. It assumes both cacheParameters() as well as
// estimateDegreeOfParallelism() have been called.
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu -- OCM code
void CostMethodHashJoin::deriveParameters()
{
  // ---------------------------------------------------------------------
  // The metrics which follow are all on a per-stream-per-probe basis.
  // ---------------------------------------------------------------------
  CostScalar innerRowCount = csZero;
  // Adjustments for OHJ when Reuse is set. This change is required since
  // we don't set noOfProbes to One.
  if ( (CmpCommon::getDefault(COMP_BOOL_37) == DF_OFF) AND
       hj_->isReuse() AND (hj_->multipleCalls() == 0) )
    innerRowCount = child1RowCountPerStream_;
  else
    innerRowCount = (child1RowCountPerStream_ / noOfProbesPerStream_).minCsOne();
  CostScalar innerTableSize =
    innerRowCount / csOneKiloBytes * extChild1RowLength_;
  CostScalar outerRowCount =
    (child0RowCountPerStream_ / noOfProbesPerStream_).minCsOne();
  CostScalar outerTableSize =
    outerRowCount / csOneKiloBytes * extChild0RowLength_;

  // ---------------------------------------------------------------------
  // Simple case. Quick way out.
  // ---------------------------------------------------------------------
  if((NOT isBMO_) OR (innerTableSize <= memoryLimit_)
        OR (hj_->isNoOverflow()))
  {
    noOfClusters_ = csOne;
    noOfInnerClustersOccupied_ = noOfOuterClustersOccupied_ = csOne;
    noOfInnerClustersInMemory_ = csOne;
    noOfOuterClustersFlushed_ = csZero;
    innerClusterSize_ = innerTableSize;
    clusterSizeAfterSplitsOverFlow_ = innerTableSize;
    estimatedNumberOfOverflowClusters_ = csZero;
    outerClusterSize_ = outerTableSize;
    mem_ = innerTableSize;

    // -------------------------------------------------------------------
    // For more details on this first row analysis, read comments on code
    // close to the end of this method.
    // -------------------------------------------------------------------
    CostScalar outerRowCountForFR = (child0RowCount_ / myRowCount_).minCsOne();
    outerRowCountForFR = MINOF(outerRowCountForFR,outerRowCount);
    stage2WorkFractionForFR_ = (outerRowCountForFR / outerRowCount);
    stage3WorkFractionForFR_ = csZero;

    return;
  }
  else mem_ = memoryLimit_;

  // ---------------------------------------------------------------------
  // We don't average the uec over the probes. We want to assume, if we
  // can, the outer references and the equi-join column are non-corelated.
  // However, row count is always a logical upper limit of the uec.
  // ---------------------------------------------------------------------
  CostScalar innerUec = MINOF(child1UecPerStream_,innerRowCount);
  CostScalar outerUec = MINOF(child0UecPerStream_,outerRowCount);

  // Just in case.
  innerUec = MIN_ONE(innerUec);
  outerUec = MIN_ONE(outerUec);

  // ---------------------------------------------------------------------
  // Decide number of clusters to allocate and also decide number of overflow
  // clusters
  // ---------------------------------------------------------------------
  Lng32 idealNoOfClusters;

  if ( CmpCommon::getDefault(COMP_BOOL_54) == DF_ON ) 
  {
    // -------------------------------------------------------------------
    // Now assume that we have an even distribution of rows among a no of
    // clusters. Compute that no of clusters, such that the size of one
    // cluster together with one buffer from each remaining clusters could
    // just fit into the memory limit. This may be a good estimate for the
    // final no of clusters after splitting, on condition that the value
    // of uec (of the hash keys) doesn't limit the maximum no of clusters
    // the rows could possibly be hashed to.
    // -------------------------------------------------------------------
    idealNoOfClusters = computeIdealCountOfClusters(
                                               memoryLimit_,innerTableSize);
  }
  else
  {
    // -------------------------------------------------------------------
    // Calculate number of clusters (at most four) and number of overflow
    // clusters
    // -------------------------------------------------------------------
    idealNoOfClusters = computeInitialCountOfClusters(
                                               memoryLimit_,innerTableSize);
  }

  if (innerUec.value() < (double) idealNoOfClusters)
  {
    // Splitting occurs until each uec takes up one cluster.
    noOfClusters_ = innerUec.getCeiling();
  }
  else
  {
    // Splitting occurs until the ideal no of clusters is reached.
    noOfClusters_ = idealNoOfClusters;
  }

  // if cross product, noOfInnerClustersOccupied_ is one
  if (NOT hasEquiJoinPred_)
    noOfInnerClustersOccupied_ = noOfClusters_ ;
  else
    noOfInnerClustersOccupied_ = noOfClusters_ + 
                                 estimatedNumberOfOverflowClusters_;

  // ---------------------------------------------------------------------
  // Now compute the average cluster size across the clusters occupied.
  // Also, find out what fraction of the inner table will get flushed to
  // disk.
  //
  // innerClusterSize_ refers to clustersize as if no overflow has taken place
  // clusterSizeAfterSplitsOverFlow_ is close to memory limit (that is after
  // overflow has occured)
  // ---------------------------------------------------------------------
  innerClusterSize_ = innerTableSize / noOfClusters_ ;

  clusterSizeAfterSplitsOverFlow_ = innerTableSize / (noOfClusters_ +
                                         estimatedNumberOfOverflowClusters_);

  if ( CmpCommon::getDefault(COMP_BOOL_54) == DF_ON )
  {
     noOfInnerClustersInMemory_ = (innerClusterSize_ > memoryLimit_ ? 0 : 1);
  }
  else
  {
    noOfInnerClustersInMemory_ = memoryLimit_ / innerClusterSize_.getValue();
    noOfInnerClustersInMemory_ = noOfInnerClustersInMemory_.getFloor();
    // noOfInnerClustersInMemory_ could be zero 
  }


  //------------------------------------------------------------------------
  // adjust number of overflow clusters if number of inner clusters does not
  // match number of clusters fit in memory
  //------------------------------------------------------------------------
  NABoolean adjustClusters = FALSE;
  if ( noOfInnerClustersInMemory_ != noOfInnerClustersOccupied_ &&
       estimatedNumberOfOverflowClusters_.isZero()
     )
  {
     estimatedNumberOfOverflowClusters_ = noOfInnerClustersOccupied_ 
                                         - noOfInnerClustersInMemory_;

     noOfInnerClustersOccupied_ = noOfInnerClustersInMemory_;
     adjustClusters = TRUE;
  }
    
  // ---------------------------------------------------------------------
  // Now consider the outer table. If it has more uec's than there are
  // no of clusters, assume even distribution of its rows across these
  // clusters. Otherwise, assume each uec takes up one cluster.
  // ---------------------------------------------------------------------
  if (NOT hasEquiJoinPred_)
    noOfOuterClustersOccupied_ = noOfClusters_;//this is one for a cross product
  else if (noOfClusters_ < outerUec.getCeiling() && adjustClusters)
    noOfOuterClustersOccupied_ = (noOfInnerClustersOccupied_ + 
                                  estimatedNumberOfOverflowClusters_);
  else if (noOfClusters_ < outerUec.getCeiling())
    noOfOuterClustersOccupied_ = noOfInnerClustersOccupied_;
  else
    noOfOuterClustersOccupied_ = outerUec.getCeiling();

  outerClusterSize_ = outerTableSize / noOfOuterClustersOccupied_;

  // **************************************************************************
  // I am commenting the code related to left joins;this needs to be looked
  // at latter. Assumption: rows are uniformly distributed across all clusters
  // this is not true for example if there is a skew. Then we need to collect
  // the interval information. Since that code is currently commented out, this
  // is ok.
  //*************************************************************************
  /*if(noOfOuterClustersOccupied_ > noOfInnerClustersOccupied_)
  {
    // -------------------------------------------------------------------
    // We don't have to flush an outer cluster in one of the two cases:
    // 1. its corresponding inner cluster is empty (for non left joins).
    // 2. its corresponding inner cluster is in-memory.
    // -------------------------------------------------------------------
    if(NOT hj_->isLeftJoin())
    {
      // -----------------------------------------------------------------
      // We assume the maximum no of outer clusters with a corresponding
      // non-empty inner cluster.
      // -----------------------------------------------------------------
      noOfOuterClustersFlushed_ =
         MAXOF(noOfInnerClustersOccupied_ - noOfInnerClustersInMemory_, csZero);
    }
    else
    {
      // -----------------------------------------------------------------
      // $$$ Here is an indication we might be able to produce the first
      // $$$ row for a left join fast (which is a null-instantiated row).
      // $$$ Further work needed to account for this ??
      // For the time being, we assume rows can only be produced at Stage
      // 2 or 3, so we still need to flush them.
      // -----------------------------------------------------------------
      noOfOuterClustersFlushed_ =
        MAXOF(noOfOuterClustersOccupied_ - noOfInnerClustersInMemory_, csZero) ;
    }
  }
  else
  {*/

  noOfOuterClustersFlushed_ =
       MAXOF(noOfOuterClustersOccupied_ - noOfInnerClustersInMemory_,
             csZero);

  // ---------------------------------------------------------------------
  // Finally, some first row cost analysis. But first, some foreword.
  //
  // Due to all kind of uncertainties involved in a hash operation, first
  // row costs are very difficult to predict. The strategy here is to
  // charge a fraction out of the LR costs as FR costs. Heuristics are
  // applied to produce such fractions. Such analysis are very crude but
  // I really doubt if we could come up with some thing better.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Result rows are produced when rows from the outer table probe the
  // hash table of an inner cluster and find matches in the chain. The
  // following ratio is useful as an estimate of how many rows from the
  // outer table have probed the hash table before the first row is ever
  // produced.
  // ---------------------------------------------------------------------
  CostScalar outerRowCountForFR = (child0RowCount_ / myRowCount_);

  // ---------------------------------------------------------------------
  // At least one row from the outer table is needed to produce a result
  // row. Also, if this HJ sits on the right leg of a NJ, always assume
  // the first row is produced in the first probe by that NJ. Otherwise,
  // things get just a bit too complicated. NB: "outerRowCount" here is
  // already a per-stream-per-probe (NJ probe) metric.
  // ---------------------------------------------------------------------
  outerRowCountForFR = (outerRowCountForFR).minCsOne();
  outerRowCountForFR = MINOF(outerRowCountForFR,outerRowCount);

  // ---------------------------------------------------------------------
  // The fate of an outer table row can be one of these three:
  //
  // 1. It is thrown away at Stage 2 if it is hashed to a cluster whose
  //    corresponding inner cluster is empty.
  // 2. It probes a hash table at Stage 2 if it is hashed to a cluster
  //    whose corresponding inner cluster in in-memory.
  // 3. It is eventually flushed to disk if it is hashed to a cluster
  //    whose corresponding inner cluster has also been flushed. It is
  //    going to probe a hash table in Stage 3.
  //
  // Here, we estimate the fractions of outer table rows which fall into
  // each of the above three categories.
  // ---------------------------------------------------------------------
  double fractionOfOuterRowProbingInStage2 =
          (noOfInnerClustersInMemory_ / noOfOuterClustersOccupied_).value();
  double fractionOfOuterRowProbingInStage3 =
          (noOfOuterClustersFlushed_ / noOfOuterClustersOccupied_).value();
  double fractionOfOuterRowsThrownAway = (1. -
   fractionOfOuterRowProbingInStage2 - fractionOfOuterRowProbingInStage3);

  // No of outer rows which finish their processing in Stage 2.
  CostScalar outerRowsDoneInStage2 = outerRowCount *
      (fractionOfOuterRowProbingInStage2 + fractionOfOuterRowsThrownAway);

  // No of outer rows which finish their processing in Stage 3.
  CostScalar outerRowsDoneInStage3 = outerRowCount *
                                        fractionOfOuterRowProbingInStage3;

  // Check if the first row can be produced in Stage 2.
  if(outerRowCountForFR <= outerRowsDoneInStage2)
  {
    stage2WorkFractionForFR_ = outerRowCountForFR / outerRowsDoneInStage2;
    stage3WorkFractionForFR_ = 0.;
  }
  else // we got to wait till Stage 3 to get our first row.
  {
    // No rows can be produced in Stage2.
    stage2WorkFractionForFR_ = 1.;

    // No of outer table rows need to be processed in Stage 3 to get FR.
    outerRowCountForFR -= outerRowsDoneInStage2;

    // Since all probing are done in Stage3.
    stage3WorkFractionForFR_ = outerRowCountForFR / outerRowsDoneInStage3;
  }

}  // CostMethodHashJoin::deriveParameters().
//<pb>
// -----------------------------------------------------------------------
// CostMethodHashJoin::computeIdealCountOfClusters().
//
// This method computes the ideal no of clusters that the inner table can
// be evenly divided into such that we have the maximum cluster size that
// could satisfy the condition that:
//
// . The first cluster and a buffer from the rest of the clusters could
//   stay within the memory limit during the first stage of hash join.
//
// The cluster size computed this way gives us a fair estimate of how big
// is the part of the inner table which can stay in memory in Stage 1 of
// the hash join. But there are always other considerations like whether
// the uec (of the hash keys) of the table restricts the maximum number of
// clusters the rows can be hashed to.
// -----------------------------------------------------------------------
Lng32 CostMethodHashJoin::computeIdealCountOfClusters(
  const CostScalar & memoryLimit,
  const CostScalar & tableSize
  )
{
  // We couldn't even have one cluster this way. Memory is too limited to
  // do a HJ.
  CMPASSERT(memoryLimit >= bufferSize_);

  // If whole table fits into memory limit, just one cluster does the job.
  if(memoryLimit >= tableSize) return 1;

  // ---------------------------------------------------------------------
  // Otherwise, the estimate is based on the solution of the set:
  //
  // 1. s * c = T
  // 2. s + (c-1) b = M
  //
  // where T is the table size, M is the memory limit, c is the number of
  // clusters, s is the cluster size and b is the buffer size. Formula 1
  // says: the table is evenly spread among the clusters. Formula 2 says:
  // one cluster together with one buffer from each remaining clusters
  // just fits into the memory limits.
  //
  // Solving for c, we have c = ((b+M)-sqrt(sqr(b+M)-4*b*T))/(2*b), which
  // doesn't have a solution if (sqr(b+M)-4*b*T)<0. In that case, we must
  // overflow the whole file.
  //
  // Steven: There should be another solution for c, i.e.
  //	c = ((b+M)+sqrt(sqr(b+M)-4*b*T))/(2*b)
  // But we choose the first solution, which provides a smaller answer,
  // i.e. smaller number of clusters
  // ---------------------------------------------------------------------
  CostScalar c;
  Lng32 cLong;
  CostScalar bM = (memoryLimit + bufferSize_);
  CostScalar bM2 = (bM * bM);
  CostScalar bM2Minus4bT = bM2 - tableSize * bufferSize_ * 4.;

  // Max no of clusters that could be used.
  Lng32 cLongMax = (Lng32) (memoryLimit / bufferSize_).getFloor().value();

  if( bM2Minus4bT.isLessThanZero() /* < csZero */)
  {
    // -------------------------------------------------------------------
    // We don't have a solution. That means the table is so large that no
    // matter how many clusters we allocate, the size of a cluster added
    // to the sum of sizes of one buffer from the remaining clusters is
    // always bigger than what the memory can accommodate. In that case,
    // we would end up using the maximum no of clusters which can be used
    // and each cluster will be flushed to disk.
    // -------------------------------------------------------------------
    cLong = cLongMax;
    // CMPASSERT(tableSize / double(cLong) > memoryLimit);
  }
  else
  {
    c = (bM - sqrt(bM2Minus4bT.value())) / bufferSize_ / csTwo;

    // This is a result of the fact that (tableSize > memoryLimit) here.
    CMPASSERT(NOT c.isLessThanOne() /* >= csOne*/);

    cLong = (Lng32) c.getCeiling().value();
    cLong = MINOF(cLong,cLongMax);
  }
  return cLong;

}  // CostMethodHashJoin::computeIdealCountOfClusters().

//<pb>
// -----------------------------------------------------------------------
// CostMethodHashJoin::computeInitialCountOfClusters().
//
// This method computes the no of clusters the HJ is going to use based
// on the memory limit and the estimated table size given as well as the
// initial no of buckets per cluster and buffer size.
//
// The initial number of clusters is at most 4; the method computes the value
// estimatedNumberOfOverflowClusters_. This gets used in calculating blocking
// costs of various stages of the hash join.
//
// Special case: the number of clusters is set to 1 if the join is a cross
// product; estimatedNumberOfOverflowClusters_ would reflect the memory needs
// of the cross product operation.
// -----------------------------------------------------------------------
Lng32 CostMethodHashJoin::computeInitialCountOfClusters(
                                      const CostScalar & memoryLimit,
                                      const CostScalar & tableSize)
{

  CostScalar clusters = ( tableSize / memoryLimit);

  Lng32 clustersL = (Lng32) clusters.getCeiling().value();

  // there are at most 16 buckets and the initial number of clusters
  // is at most 4
  if (clusters > 4) 
    clusters = 4;

  // calculate likelihood of overflow to disk; assume memoryLimit is at most
  // 200 MB; memorryLimit is already in kilobytes. Note that this is only done
  // for calculation of initial clusters. The number of clusters may be higher
  // due to splitting

  float myMemoryLimit = (float) memoryLimit.getValue();
  if (memoryLimit > 2E6)
    myMemoryLimit =  2E6;

  estimatedNumberOfOverflowClusters_ = CostScalar ( tableSize.getValue() / 
                                                    myMemoryLimit);

  if (NOT hasEquiJoinPred_ && estimatedNumberOfOverflowClusters_ > 0)
  { 
    estimatedNumberOfOverflowClusters_.operator--();
  }
  else if (estimatedNumberOfOverflowClusters_ > 4 ) 
    estimatedNumberOfOverflowClusters_ = estimatedNumberOfOverflowClusters_-4;
  else 
    estimatedNumberOfOverflowClusters_ = 0;

  // At least one buffer from each bucket must fit into main memory.
  CostScalar basicMemoryReqdPerCluster =
                              bufferSize_ * initialBucketCountPerCluster_;

  CostScalar maxClusters = (memoryLimit / basicMemoryReqdPerCluster);
  //long maxClustersL = (long) maxClusters.getFloor().value();

  if (NOT hasEquiJoinPred_)
  {
    // there is only one cluster if this is a cross product
    clusters=1;
  }
  
  Lng32 initialClusters = (Lng32) MINOF(clusters,maxClusters).getCeiling().value();
  return initialClusters;
}  // CostMethodHashJoin::computeInitialCountOfClusters().

// -----------------------------------------------------------------------
// CostMethodHashJoin::computeCreateHashTableCost().
//
// This method computes the cost of chaining up rows to form a hash table.
// The cost for computing the hash value of a row is not included in this
// computation.
// -----------------------------------------------------------------------
CostScalar CostMethodHashJoin::computeCreateHashTableCost(
                                         const CostScalar& rowCount) const
{
  return cpuCostAllocateHashTable_ + cpuCostInsertRowToChain_ * rowCount;
}  // CostMethodHashJoin::computeCreateHashTableCost().
//<pb>
// -----------------------------------------------------------------------
// CostMethodHashJoin::computeTotalProbingCost().
//
// This method estimates the total CPU cost for producing the whole result
// set. Costs included here are:
//
// 1. Cost of probing a hash table by the outer rows, which includes the
//    cost to traverse the chain, compare the hash keys. Also note that
//    we underestimate by not considering collision here if we do find a
//    match.
// 2. Cost of evaluating predicates other than the hash join predicates
//    after a match has been found in 1.
// -----------------------------------------------------------------------
CostScalar CostMethodHashJoin::computeTotalProbingCost()
{
  CostScalar cpu(csZero);
  CostScalar compareHashKeyOpCount(csZero);
  CostScalar evalOtherJoinPredsOpCount(csZero);
  CostScalar evalOtherSelPredsOpCount(csZero);

  // $$$ This is always the code path to take for phase 1.
  if(NOT isColStatsMeaningful_)
  {
    // Doing cross product.
    if(NOT hasEquiJoinPred_)
    {
      compareHashKeyOpCount = csZero;

      if(hj_->isInnerNonSemiJoin())
      {
        evalOtherJoinPredsOpCount = csZero;
        evalOtherSelPredsOpCount = resultEquiJoinRowCount_;
      }
      else if(hj_->isSemiJoin() OR hj_->isAntiSemiJoin())
      {
        evalOtherJoinPredsOpCount = resultEquiJoinRowCount_;
        evalOtherSelPredsOpCount = csZero;
      }
      else if(hj_->isLeftJoin())
      {
        evalOtherJoinPredsOpCount = resultEquiJoinRowCount_;
        evalOtherSelPredsOpCount = myRowCount_;
      }
    }
    else
    {
      if(hj_->isInnerNonSemiJoin())
      {
        // ---------------------------------------------------------------
        // This is the optimal case providing no collisions occur, the
        // no of comparison operation necessary in total should just be
        // equal to the no of rows generated by the equi-join, each row
        // present in the equi-join result is a result of a successful
        // key comparison operation.
        // ---------------------------------------------------------------
        compareHashKeyOpCount = resultEquiJoinRowCount_;

        // ---------------------------------------------------------------
        // An inner non-semi join has no predicates stored as joinPred().
        // This is sometimes untrue when an outer join is converted to an
        // inner join and its predicates copied to selectionPred() but not
        // cleared.
        // CMPASSERT(hj_->joinPred().isEmpty());
        // ---------------------------------------------------------------
        evalOtherJoinPredsOpCount = csZero;

        // ---------------------------------------------------------------
        // The remaining selection predicates are then evaluated on each
        // row in the result of the equi-join.
        // ---------------------------------------------------------------
        evalOtherSelPredsOpCount = resultEquiJoinRowCount_;
      }

      // Semijoin and anti semijoin are costed the same.
      // OA mar-02
      else if(hj_->isSemiJoin() OR hj_->isAntiSemiJoin())
      {
        // ---------------------------------------------------------------
        // No of comparisons for a semi-join is harder to estimate. It's
        // because when we are probing the hash table, we may ignore the
        // rest of the chain once all the remaining predicates are also
        // satisfied in the match.
        // ---------------------------------------------------------------

        // This is a measure of selectivity of the remaining predicates.
        CostScalar selOfOtherPreds =
                                    myRowCount_ / resultEquiJoinRowCount_;

        // The number of rows selected by all predicates should not
        // exceed the number of rows selected only by the equijoin
        // predicates.  Thus, selOfOtherPreds should be a true percentage
        // between zero and 1 inclusive.  We force this, just in case.
        //selOfOtherPreds = MAXOF(MINOF(selOfOtherPreds, csOne), csZero);
        (selOfOtherPreds.maxCsOne()).minCsZero();

        // ---------------------------------------------------------------
        // Estimate the chain length from statistics of the inner table.
        // ---------------------------------------------------------------
        CostScalar averageChainLength =
                                  child1RowCount_ / child1EquiJoinColUec_;

        // Use selectivity to estimate part of chain compared.
        CostScalar chainLengthTraversed = averageChainLength *
                                       (csOne - selOfOtherPreds);

        compareHashKeyOpCount = evalOtherJoinPredsOpCount =
                           resultEquiJoinRowCount_ * chainLengthTraversed;

        // A semijoin has no predicates stored as selectionPred().
        CMPASSERT(hj_->selectionPred().isEmpty());
        evalOtherSelPredsOpCount = csZero;
      }
      else if(hj_->isLeftJoin())
      {
        // ---------------------------------------------------------------
        // This is the optimal case providing no collisions occur, the
        // no of comparison operation necessary in total should just be
        // equal to the no of rows generated by the equi-join, each row
        // present in the equi-join result is a result of a successful
        // key comparison operation.
        // ---------------------------------------------------------------
        compareHashKeyOpCount = resultEquiJoinRowCount_;

        // ---------------------------------------------------------------
        // The rest of the join predicates are evaluated on rows produced
        // by probing the hash join. A failed probe outer row go against
        // the selection predicates directly.
        // ---------------------------------------------------------------
        evalOtherJoinPredsOpCount = resultEquiJoinRowCount_;

        // ---------------------------------------------------------------
        // This is just some sort of an average to estimate the no of
        // rows which evaluates to TRUE on all the join predicates and
        // those which get null-instantiated and go against the selection
        // predicates.
        // ---------------------------------------------------------------
        evalOtherSelPredsOpCount =
                             (resultEquiJoinRowCount_ + myRowCount_) * .5;
      }
    }

    cpu += cpuCostPositionHashTableCursor_ * child0RowCount_;
    cpu += cpuCostCompareHashKeys_ * compareHashKeyOpCount;
    cpu += cpuCostEvalOtherJoinPreds_ * evalOtherJoinPredsOpCount;
    cpu += cpuCostEvalOtherSelPreds_ * evalOtherSelPredsOpCount;
  }
  return cpu;

}  // CostMethodHashJoin::computeTotalProbingCost().
//<pb>

// -----------------------------------------------------------------------
// CostMethodHashJoin::computeStage1Cost().
// -----------------------------------------------------------------------
void CostMethodHashJoin::computeStage1Cost()
{
  // ---------------------------------------------------------------------
  // Cost scalars to be computed.
  // ---------------------------------------------------------------------
  CostScalar cpu(csZero), ioSeek(csZero), ioByte(csZero), ioTimeForFirstRow(csZero),
    ioTimeForBlocking(csZero), ioFlushing(csZero) ; //j disk(csZero);

  // ---------------------------------------------------------------------
  // Steps done in Stage1.
  //
  // 1. Read the inner table rows and compute their hash values.
  // 2. Assign the rows to clusters by their hash values.
  // 3. Copy each row to a buffer associated with its cluster.
  // 4. When the available memory is used up, pick a cluster and flush
  //    its buffers to disk.
  // 5. If there are buffers left in memory after the whole inner table
  //    has been read, build a hash table for them by chaining the rows
  //    up.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Average per probe cost of computing hash value of and copying each
  // row to the buffer.
  // ---------------------------------------------------------------------
  cpu = (cpuCostHashRow_ + cpuCostCopyChild1Row_) *
                        (child1RowCountPerStream_ / noOfProbesPerStream_);

  // ---------------------------------------------------------------------
  // Average per probe cost of flushing those clusters which cannot be
  // accommodated in memory.
  // ---------------------------------------------------------------------

  if(noOfInnerClustersInMemory_ != noOfInnerClustersOccupied_ )
  {
    ioByte = clusterSizeAfterSplitsOverFlow_ *
                estimatedNumberOfOverflowClusters_;

    ioSeek = ioByte / bufferSize_;

    ioFlushing = ioSeek * CURRSTMT_OPTDEFAULTS->getTimePerSeek() +
      ioByte * CURRSTMT_OPTDEFAULTS->getTimePerSeqKb();
  }

  // ---------------------------------------------------------------------
  // Average per probe cost to create hash table for in memory clusters.
  // ---------------------------------------------------------------------
  if(noOfInnerClustersInMemory_ != 0)
  {
    CostScalar rowsInMemory = clusterSizeAfterSplitsOverFlow_ *
      noOfInnerClustersInMemory_ / extChild1RowLength_ * csOneKiloBytes;
    rowsInMemory = (rowsInMemory).minCsOne();
    cpu += (computeCreateHashTableCost(csZero) * noOfInnerClustersInMemory_) +
      computeCreateHashTableCost(rowsInMemory);
  }

  // Optimizer shouldn't choose OHJ for BMOs.
  // no IO cost for OHJ.
  // ---------------------------------------------------------------------
  // PAGE FAULT cost in case of an Ordered Hash Join that is a BMO.-OA
  // ---------------------------------------------------------------------

  if(CmpCommon::getDefault(COMP_BOOL_37) == DF_ON AND 
     hj_->isNoOverflow() AND isBMO_)
  {
    // Page faults possible because overflow logic is turned off.
    // Since number of Clusters == 1, we have
    // innerTableSize = innerClusterSize_;

    double innerTableSize = innerClusterSize_.getValue();

    // memoryLimit_ = Memory limit in kbytes on a per stream basis.
    // No limit if zero
    double percentageOfPageFaults =
      (innerTableSize - memoryLimit_) /
      innerTableSize;
    double numPagesToScan = 1.0;
    double pageSize =
      CostPrimitives::getBasicCostFactor(DEF_PAGE_SIZE);

    const double seekCostFR = numPagesToScan * percentageOfPageFaults;

    ioTimeForFirstRow = seekCostFR * (CURRSTMT_OPTDEFAULTS->getTimePerSeek() +
			    pageSize * CURRSTMT_OPTDEFAULTS->getTimePerSeqKb());

    // ioTimeForLastRow = ioTimeForFirstRow * noOfProbesPerStream_;

    // record size of the inner table
    // (add overhead per rec as the executor does, this also helps prevent
    // zero divide problems)
    double recordSize =
      (hj_->child(1).getGroupAttr()->getRecordLength() +
       CostPrimitives::getBasicCostFactor(HH_OP_HASHED_ROW_OVERHEAD)) / 1024.0;
    // Find out how many rows would fit in memory
    double numRowsThatFitInMemory = memoryLimit_ / recordSize;
    double innerRowCount = innerTableSize / recordSize;

    // ioTimeforBlocking is the no. of rows which can't fit in memory
    // (waiting), times the time taken for each row, which is ioTimefor
    //  first row.

    ioTimeForBlocking =
      CostScalar((innerRowCount - numRowsThatFitInMemory)) *
		ioTimeForFirstRow;

  } // pagefault costing for ordered hash joins OA feb02


  CostScalar cpuTimeForBlocking =
    cpu * CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  // If the inner table is reused, the blocking cost is divided by numprobes.
  if( hj_->isReuse() AND hj_->multipleCalls() == 0)
  {

    CostScalar numOriginalProbes = inLogProp_->getResultCardinality();
    cpuTimeForBlocking /= numOriginalProbes;
    ioTimeForBlocking  /= numOriginalProbes;
  }


  CostScalar io = ioFlushing + ioTimeForBlocking;


  // ---------------------------------------------------------------------
  // Synthesize the cost vectors and objects.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // the first stage
  // of a HJ by a blocking cost instead of a FR and LR costs. The FR cost
  // cost is changed to the BK cost, while the LR cost computation has
  // been commented out below
  // ---------------------------------------------------------------------

  if ( CmpCommon::getDefault(COMP_BOOL_39) == DF_ON )
    stage1cvBK_ =
    new STMTHEAP SimpleCostVector (
      cpu * CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions(),
      io,
      csZero,			// msg time
      csZero,			// idle time
      noOfProbesPerStream_
      );
  else
    stage1cvBK_ =
    new STMTHEAP SimpleCostVector (
      cpuTimeForBlocking,
      io,
      csZero,			// msg time
      csZero,			// idle time
      inLogProp_->getResultCardinality()
      );

}  // CostMethodHashJoin::computeStage1Cost().
//<pb>
// -----------------------------------------------------------------------
// CostMethodHashJoin::computeStage2Cost().
//
// Estimates depend on overflowFraction_ computed by computeStage1Cost().
// -----------------------------------------------------------------------
void CostMethodHashJoin::computeStage2Cost()
{
  // ---------------------------------------------------------------------
  // Cost scalars to be computed.
  // ---------------------------------------------------------------------
  CostScalar cpuFR(csZero), ioSeekFR(csZero), ioByteFR(csZero);
  //j, diskFR(csZero);
  CostScalar cpuLR(csZero), ioSeekLR(csZero), ioByteLR(csZero);
  //j diskLR(csZero);
  CostScalar cpuBK(csZero);
  const CostScalar numberOfProbesForHJ = inLogProp_->getResultCardinality();

  // ---------------------------------------------------------------------
  // Steps done in Stage2.
  //
  // 1. Read the outer table rows and compute their hash values.
  // 2. Assign the rows to clusters by their hash values.
  // 3. If a row is assigned to a cluster whose corresponding cluster for
  //    the inner table is in memory, a probe on the hash table occurs.
  // 4. If a row is assigned to a cluster whose corresponding cluster for
  //    the inner table is on disk, copy the row to a buffer associated
  //    with this cluster.
  // 5. When a buffer of a overflow cluster is full, write the buffer to
  //    disk and free up the space.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Average per probe cost of computing hash value of each row.
  // ---------------------------------------------------------------------

  CostScalar probingFractionForStage2 = ( noOfInnerClustersInMemory_ /
      (noOfInnerClustersInMemory_ + noOfOuterClustersFlushed_));

  if ( CmpCommon::getDefault(COMP_BOOL_54) == DF_OFF )  
  {
    cpuLR = (cpuCostHashRow_ *
                         child0RowCountPerStream_ / noOfProbesPerStream_) *
          probingFractionForStage2;

    cpuBK = (cpuCostHashRow_ *
               child0RowCountPerStream_ / noOfProbesPerStream_) *
          (1 - probingFractionForStage2.getValue());
  }
  else
    cpuLR = (cpuCostHashRow_ *
                       child0RowCountPerStream_ / noOfProbesPerStream_);

  // ---------------------------------------------------------------------
  // Outer rows that are hashed to an in-memory cluster needn't be stored
  // in buffers. They could probe the hash table directly.
  // ---------------------------------------------------------------------
  CostScalar rowsCopied = outerClusterSize_ *
    noOfOuterClustersFlushed_ / extChild0RowLength_ * csOneKiloBytes;
  cpuBK += (cpuCostCopyChild0Row_ * rowsCopied);

  // ---------------------------------------------------------------------
  // We expect some probing activities on the hash table during Stage 2
  // if there is an in-memory inner cluster.
  // ---------------------------------------------------------------------
  if(noOfInnerClustersInMemory_ > 0)
  {
    // -------------------------------------------------------------------
    // The cost of such probing activities is estimated as follows:
    //
    // 1. We have already computed the total cost of hash table probing
    //    using the histograms for the *complete* set of rows and stored
    //    it in cpuCostTotalProbing_. See computeTotalProbingCost().
    // 2. Now, we average this cost across all streams and NJ probes.
    // 3. Then, we split the average between Stage 2 and Stage 3 in the
    //    ratio (No of clusters joined in Stage 2) to (No of clusters to
    //    be joined in Stage 3).
    // -------------------------------------------------------------------
    CostScalar cpuCostAverageProbing =
          (cpuCostTotalProbing_ / countOfStreams_ / noOfProbesPerStream_);
    cpuLR += (cpuCostAverageProbing * probingFractionForStage2);
  }

  // ---------------------------------------------------------------------
  // Average per probe cost of flushing those outer clusters whose
  // corresponding inner clusters are also flushed.
  // ---------------------------------------------------------------------
  ioByteLR = outerClusterSize_ * noOfOuterClustersFlushed_;
  ioSeekLR = ioByteLR / bufferSize_;
  //j diskLR = ioByteLR;

  // ---------------------------------------------------------------------
  // Now estimate FR cost from the LR cost and the precomputed fraction
  // of LR cost to be charged as FR cost.
  // ---------------------------------------------------------------------
  cpuFR = cpuLR * stage2WorkFractionForFR_;
  //  ioByteFR = ioByteLR * stage2WorkFractionForFR_;
  // ioSeekFR = ioSeekLR * stage2WorkFractionForFR_;
  // diskFR = ioByteFR;


  // fudge factors for cpuTime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  // ---------------------------------------------------------------------
  // Synthesize the cost vectors and objects.
  //
  // Note that memory is 0. because Stage2 didn't use additional memory.
  // It reuses the memory already used by Stage1.
  // ---------------------------------------------------------------------
  if ( CmpCommon::getDefault(COMP_BOOL_39) == DF_ON )
    stage2cvFR_ =
    new STMTHEAP SimpleCostVector(
      cpuFR * ff_cpu,
      csZero,
      csZero,
      csZero,
      noOfProbesPerStream_
      );
  else
    stage2cvFR_ =
    new STMTHEAP SimpleCostVector(
      cpuFR * ff_cpu,
      csZero,
      csZero,
      csZero,
      numberOfProbesForHJ
      );

  // ---------------------------------------------------------------------
  // Last row cost is a sum over all the probes. Note that disk space is
  // reusable over probes and needn't be scaled.
  // ---------------------------------------------------------------------
  //j diskLR = ioByteLR;
  cpuLR *= noOfProbesPerStream_;
  ioSeekLR *= noOfProbesPerStream_;
  ioByteLR *= noOfProbesPerStream_;
  // no messages:
  if ( CmpCommon::getDefault(COMP_BOOL_39) == DF_ON )
    stage2cvLR_ =
    new STMTHEAP SimpleCostVector (
      cpuLR * ff_cpu,
      csZero,	// iotime
      csZero,	// msgtime
      csZero,	// idletime
      noOfProbesPerStream_
      );
  else
    stage2cvLR_ =
    new STMTHEAP SimpleCostVector (
      cpuLR * ff_cpu,
      csZero,	// iotime
      csZero,	// msgtime
      csZero,	// idletime
      numberOfProbesForHJ
      );
  // -----------------------------------------------------------------------
  // Writing the outer is considered blocking:
  // -----------------------------------------------------------------------
  // ---------------------------------------------------------------------
  if ( CmpCommon::getDefault(COMP_BOOL_39) == DF_ON )
    stage2cvBK_ =
    new STMTHEAP SimpleCostVector (
      cpuBK * ff_cpu,
      ioSeekLR * CURRSTMT_OPTDEFAULTS->getTimePerSeek()+	  // combined time for
      ioByteLR * CURRSTMT_OPTDEFAULTS->getTimePerSeqKb(),  //seeks & transfer=IO
      csZero,
      csZero,
      noOfProbesPerStream_
      );
  else
    stage2cvBK_ =
    new STMTHEAP SimpleCostVector (
      cpuBK * ff_cpu,
      (ioSeekLR * CURRSTMT_OPTDEFAULTS->getTimePerSeek()+
      ioByteLR * CURRSTMT_OPTDEFAULTS->getTimePerSeqKb())/numberOfProbesForHJ,
      csZero,
      csZero,
      numberOfProbesForHJ
      );
}  // CostMethodHashJoin::computeStage2Cost().
//<pb>
// -----------------------------------------------------------------------
// CostMethodHashJoin::computeStage3Cost().
// -----------------------------------------------------------------------
void CostMethodHashJoin::computeStage3Cost()
{
  // There is no Stage 3 if no outer clusters are flushed to disk.
  if(noOfOuterClustersFlushed_ == 0)
  {
    stage3cvFR_ =
      new STMTHEAP SimpleCostVector(
	csZero,
	csZero,
	csZero,
	csZero,
	noOfProbesPerStream_
	);

    stage3cvLR_ =
      new STMTHEAP SimpleCostVector(
	csZero,
	csZero,
	csZero,
	csZero,
	noOfProbesPerStream_
	);

    return;
  }

  // ---------------------------------------------------------------------
  // Cost scalars to be computed.
  // ---------------------------------------------------------------------
  CostScalar
    cpuBK(csZero)
    ,cpuLR(csZero)
    ,cpuFR(csZero)
    ,ioSeekBK(csZero)
    ,ioByteBK(csZero)
    ;

  // ---------------------------------------------------------------------
  // In Stage 3, the remaining part of the tables are read in and joined,
  // in cluster pairs. The steps are as follows:
  //
  // 1. Read a cluster from the inner table.
  // 2. Build a hash table for the cluster read.
  // 3. Read a buffer from the matching cluster in the outer table.
  // 4. Probe the hash table using rows from the buffer and do the join.
  // 5. Do 3 and 4 again until all buffers from that cluster are read.
  // 6. Begin the cycle with another cluster.
  //
  // So the total cost consists of:
  // A. Total I/O of reading the remaining part of both tables once.
  // B. CPU cost for building the hash table for each cluster.
  // C. The part of cpuCostTotalProbing_ which is not charged in Stage2.
  //
  // Note that the above steps assume that the smaller cluster can fit
  // into memory. Otherwise, we need to read only a part of the smaller
  // cluster which fits, build its hash table, probe it using the larger
  // cluster, and repeat the same step for another part of the smaller
  // cluster until the whole smaller cluster is exhausted. Thus, the
  // larger cluster is actually read a number of times equal to ceiling(
  // smallerClusterSize/memoryLimit_).
  // ---------------------------------------------------------------------

  CostScalar buildClusterSize = clusterSizeAfterSplitsOverFlow_;
  CostScalar probeClusterSize = outerClusterSize_;
 
  CostScalar hashLoopPasses; 
  // when is this more than one? 
  if (NOT hasEquiJoinPred_)
     hashLoopPasses = estimatedNumberOfOverflowClusters_;
  else 
     // this is always one; need to be looked into again -
     hashLoopPasses = (buildClusterSize / memoryLimit_).getCeiling().value();

  hashLoopPasses=hashLoopPasses.minCsOne();

  // ---------------------------------------------------------------------
  // Total I/O spent in reading a pair of clusters back. Hash loops mean
  // the probing clusters need to be read multiple times. Cost of Item A,
  // push effect of hash loops. noOfOuterClusterFlushed_ is just no of
  // cluster pairs left to be joined.
  // ---------------------------------------------------------------------
  CostScalar probeKb,probeSeeks,buildKb,buildSeeks;

  probeKb = probeClusterSize * hashLoopPasses * noOfOuterClustersFlushed_;
  probeSeeks = probeKb / bufferSize_;
//  probeSeeks = MINOF(CostScalar(noOfOuterClustersFlushed_), probeSeeks);

  if (NOT hasEquiJoinPred_)
    buildKb = buildClusterSize*estimatedNumberOfOverflowClusters_;
  else
    buildKb = buildClusterSize*noOfOuterClustersFlushed_;

  buildSeeks = buildKb / bufferSize_;
//  buildSeeks = MINOF(CostScalar(noOfOuterClustersFlushed_), buildSeeks);

  // reads of probes are blocking because reads of
  // I/O buffers are not using double buffering
  ioByteBK = probeKb+buildKb;
  ioSeekBK = probeSeeks+buildSeeks;

  // ---------------------------------------------------------------------
  // No of rows needed to be chained into a hash table per cluster. Note
  // that we always chain rows in the inner cluster. Cost of Item B.
  // ---------------------------------------------------------------------
  CostScalar rowsChained = clusterSizeAfterSplitsOverFlow_ / 
                           extChild1RowLength_ * 1024.;
 
  cpuBK =
    computeCreateHashTableCost(rowsChained)
    *
    estimatedNumberOfOverflowClusters_;

  // ---------------------------------------------------------------------
  // Remaining part of probing cost. Cost of item C.
  // ---------------------------------------------------------------------
  CostScalar cpuCostAverageProbing =
          (cpuCostTotalProbing_ / countOfStreams_ / noOfProbesPerStream_);
  double probingFractionForStage3 = ( noOfOuterClustersFlushed_ /
          (noOfInnerClustersInMemory_ + noOfOuterClustersFlushed_)).value();
  cpuLR += (cpuCostAverageProbing * probingFractionForStage3);

  // ---------------------------------------------------------------------
  // Again take the precomputed fraction of cost out of the LR cost to
  // give our FR cost.
  // ---------------------------------------------------------------------
  cpuFR = cpuLR * stage3WorkFractionForFR_;

  // fudge factor for cputime
    const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  // ---------------------------------------------------------------------
  // Again all memory has been accounted for in Stage 1. Also no more
  // disk space is needed.
  // Also note that all I/O is blocking in stage 3, thus LR and FR
  // have no I/O.
  // ---------------------------------------------------------------------
  // no messages:
  stage3cvFR_ =
    new STMTHEAP SimpleCostVector (
      cpuFR * ff_cpu,
      csZero,
      csZero,
      csZero,
      noOfProbesPerStream_
      );

  // ---------------------------------------------------------------------
  // So far, LR costs are per probe cost. Scale them up to represent all
  // cost for all the probes.
  // ---------------------------------------------------------------------
  cpuLR *= noOfProbesPerStream_;

  stage3cvLR_ =
    new STMTHEAP SimpleCostVector (
      cpuLR * ff_cpu,
      csZero,
      csZero,
      csZero,
      noOfProbesPerStream_
      );

  stage3cvBK_ =
    new STMTHEAP SimpleCostVector (
      cpuBK * ff_cpu,
      ioSeekBK * CURRSTMT_OPTDEFAULTS->getTimePerSeek()+	  // adding seektime &
      ioByteBK * CURRSTMT_OPTDEFAULTS->getTimePerSeqKb(),  // Kbtime to IOtime
      csZero,
      csZero,
      noOfProbesPerStream_
      );

}  // CostMethodHashJoin::computeStage3Cost().
//<pb>
// -----------------------------------------------------------------------
// CostMethodHashJoin::computeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodHashJoin::computeOperatorCostInternal(RelExpr* op,
                                                const Context* myContext,
                                                Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();
  deriveParameters();


  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // Compute costs at various stages of the Join.
  // ---------------------------------------------------------------------
  computeStage1Cost();
  computeStage2Cost();
  computeStage3Cost();

  // -----------------------------------------------------------------------
  // Compute first row, last row and blocking costs using temporary cost
  // vectors for each of the hash join stages.  Stage 1 involves building
  // a hash table from the inner table and writing overflow clusters to
  // disk.  Stage 2 involves taking rows from the outer table, probing the
  // hash table built in stage 1 and writing overflow clusters to disk.
  // Stage 3 involves joining the overflow clusters from stages 1 and 2.
  // Since no ancestor activity can proceed until stage 1 finishes, we put
  // its resource usage in a blocking vector.  Since stage 3 can not proceed
  // until stage 2 completes we add the corresponding resource vectors using
  // blocking addition.
  // -----------------------------------------------------------------------
  SimpleCostVector cvFR( blockingAdd(*stage2cvFR_, *stage3cvFR_, rpp_) );
  SimpleCostVector cvLR( blockingAdd(*stage2cvLR_, *stage3cvLR_, rpp_) );
  SimpleCostVector cvBK;
  if (stage1cvBK_ != NULL)
    {
      cvBK = *stage1cvBK_;
      if (stage2cvBK_ != NULL)
        {
          cvBK = blockingAdd(cvBK, *stage2cvBK_, rpp_);
        }
      if (stage3cvBK_ != NULL)
        {
          cvBK = blockingAdd(cvBK, *stage3cvBK_, rpp_);
        }
    }


  // ---------------------------------------------------------------------
  // Set each cost vector's number of probes.
  // ---------------------------------------------------------------------
  if ( CmpCommon::getDefault( COMP_BOOL_39 ) == DF_ON )
  {
  cvFR.setNumProbes(noOfProbesPerStream_);
  cvLR.setNumProbes(noOfProbesPerStream_);
  cvBK.setNumProbes(noOfProbesPerStream_);
  }
  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"HASHJOIN::computeOperatorCost()\n");
    fprintf(pfp,"myRowCount=%g;child0RowCount=%g;child1RowCount=%g\n",
     myRowCount_.value(),child0RowCount_.value(),child1RowCount_.value());
    cvFR.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return the cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  // ---------------------------------------------------------------------
  // Create new HashJoin Cost object.  Store not only the traditional Cost
  // vectors and variables but also intermediate values and vectors of use
  // for final costing of hash joins.
  // ---------------------------------------------------------------------
  Cost *costPtr = new STMTHEAP
                      HashJoinCost (&cvFR,
                                    &cvLR,
                                    &cvBK,
                                    cpuCount,
                                    fragmentsPerCPU,
                                    stage2WorkFractionForFR_,
                                    stage3WorkFractionForFR_,
                                    stage2cvLR_,
                                    stage3cvLR_,
                                    stage1cvBK_,
                                    stage2cvBK_,
                                    stage3cvBK_
                                    );

#ifndef NDEBUG
  if ( printCost )
    {
      pfp = stdout;
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodHashJoin::computeOperatorCostInternal().
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// Clean up the cost vectors at various stages.
// -----------------------------------------------------------------------
void CostMethodHashJoin::cleanUp()
{
  if (stage1cvBK_ != NULL)
    {
      delete stage1cvBK_;
      stage1cvBK_ = NULL;
    }

  if (stage2cvBK_ != NULL)
    {
      delete stage2cvBK_;
      stage2cvBK_ = NULL;
    }

  if (stage3cvBK_ != NULL)
    {
      delete stage3cvBK_;
      stage3cvBK_ = NULL;
    }

  if (stage2cvFR_ != NULL)
    {
      delete stage2cvFR_;
      stage2cvFR_ = NULL;
    }

  if (stage2cvLR_ != NULL)
    {
      delete stage2cvLR_;
      stage2cvLR_ = NULL;
    }

  if (stage3cvFR_ != NULL)
    {
      delete stage3cvFR_;
      stage3cvFR_ = NULL;
    }

  if (stage3cvLR_ != NULL)
    {
      delete stage3cvLR_;
      stage3cvLR_ = NULL;
    }

  CostMethodJoin::cleanUp();

}  // CostMethodHashJoin::cleanUp().
//<pb>
//==============================================================================
//  Produce a final cumulative cost for an entire subtree rooted at a specified
// physical HASH JOIN operator.
//
// Input:
//  hashJoinOp -- specified physical hash join operator.
//
//  myContext  -- context associated with specified physical join operator
//
//  pws        -- plan work space associated with specified physical hash join
//                    operator.
//
//  planNumber -- used to get appropriate child contexts.
//
// Output:
//  none
//
// Return:
//  Pointer to cumulative final cost.
//
//==============================================================================
// LCOV_EXCL_START :cnu -- OCM code
Cost*
CostMethodHashJoin::computePlanCost( RelExpr*             hashJoinOp,
                                     const Context*       myContext,
                                     const PlanWorkSpace* pws,
                                     Lng32                 planNumber)
{

  //-----------------------------------------------------------------------
  //  Get a local copy of the required physical properties for use in later
  // roll-up computations.
  //-----------------------------------------------------------------------
  const ReqdPhysicalProperty* rpp = myContext->getReqdPhysicalProperty();

  //-------------------------------------------------------------------
  //  Get cumulative costs associated with each child of this hash join
  // operator.
  //-------------------------------------------------------------------
  CostPtr leftChildCost;
  CostPtr rightChildCost;
  getChildCostsForBinaryOp(hashJoinOp,
                           myContext,
                           pws,
                           planNumber,
                           leftChildCost,
                           rightChildCost);

  //---------------------------------------------------------------------------
  //  The vector idleT represents the amount of time during which exactly one
  // of the two child processes has provided a rows to this HASH JOIN operator.
  // It is a vector whose idle time component is the elapsed time of the slower
  // child minus the elapsed time of the faster child.
  //---------------------------------------------------------------------------
  /*jo SimpleCostVector idleT;
  CostScalar leftChildOpfrET  =  leftChildCost->getOpfr().getElapsedTime(rpp);
  CostScalar rightChildOpfrET = rightChildCost->getOpfr().getElapsedTime(rpp);
  idleT.setIdleTime(  MAXOF( leftChildOpfrET, rightChildOpfrET )
		    - MINOF( leftChildOpfrET, rightChildOpfrET ) );

  //-------------------------------------------------
  //  Add idle time calculated above to slower child.
  //-------------------------------------------------
  if ( leftChildOpfrET > rightChildOpfrET )
    {

      //---------------------------------------------------------------------
      //  Left child is slower.  See if it or any of its descendants have any
      // blocking activity.
      //---------------------------------------------------------------------
      if ( leftChildCost->getCpbcTotal().isZeroVector() )
        {

          //-----------------------------------------------------------------
          //  Neither the left child nor  any of its descendants has any
          // blocking activity, so add idle time to both the first and last
          // row vectors.
          //
          //  Remember that since the last row vector represents a cumulative
          // cost per probe and the idle time represents the cost of an
          // average single probe, we must repeatedly add the idle time to
          // the last row vector--once for each probe.  We add the first
          // probe's worth of idle time using simple vector addition.  Since
          // all subsequent probes overlap with the previous probe, we can use
          // overlapped addition for the subsequent probes.
          //-----------------------------------------------------------------
          leftChildCost->cpfr() += idleT;
          CostScalar overlappedProbes
                                = leftChildCost->getCplr().getNumProbes() - 1;
          leftChildCost->cplr() = overlapAdd(leftChildCost->getCplr() + idleT,
                                             (idleT * overlappedProbes)
                                            );
        }
      else
        {

          //------------------------------------------------------------------
          //  Left child or its descendents has blocking activity, so add idle
          // time to the blocking vectors.
          //------------------------------------------------------------------
          leftChildCost->cpbc1()     += idleT;
          leftChildCost->cpbcTotal() += idleT;
        }
    }
  else
    {

      //----------------------------------------------------------------------
      //  Right child is slower.  See if it or any of its descendants have any
      // blocking activity.
      //----------------------------------------------------------------------
      if ( rightChildCost->getCpbcTotal().isZeroVector() )
        {

          //------------------------------------------------------------------
          //  Neither the right child nor  any of its descendants has any
          // blocking activity, so add idle time to both the first and last
          // row vectors.
          //
          //  Remember that since the last row vector represents a cumulative
          // cost per probe and the idle time represents the cost of an
          // average single probe, we must repeatedly add the idle time to
          // the last row vector--once for each probe.  We add the first
          // probe's worth of idle time using simple vector addition.  Since
          // all subsequent probes overlap with the previous probe, we can use
          // overlapped addition for the subsequent probes.
          //------------------------------------------------------------------
          rightChildCost->cpfr() += idleT;
          CostScalar overlappedProbes
                                 = rightChildCost->getCplr().getNumProbes() - 1;
          rightChildCost->cplr() = overlapAddUnary(rightChildCost->getCplr() + idleT,
                                              (idleT * overlappedProbes)
                                             );
        }
      else
        {

          //-------------------------------------------------------------------
          //  Right child or its descendents has blocking activity, so add idle
          // time to the blocking vectors.
          //-------------------------------------------------------------------
          rightChildCost->cpbc1()     += idleT;
          rightChildCost->cpbcTotal() += idleT;
        }
    }

  jo*/

  //----------------------------------------------------------------------
  //  Get addressability to parent cost in plan workspace and roll this up
  // with children costs retrieved above.
  //----------------------------------------------------------------------
  HashJoinCost* parentCost =
    (HashJoinCost*) ((PlanWorkSpace *)pws)->getFinalOperatorCost(planNumber);

  Cost* rollUpCost = new STMTHEAP Cost();

  //------------------------------------------------------------------------
  //  For total cost, simply accumulate all resoure usage with simple vector
  // addition.
  //------------------------------------------------------------------------
  rollUpCost->totalCost() =   leftChildCost->getTotalCost()
                            + rightChildCost->getTotalCost()
                            + parentCost->getTotalCost();

  //////////////////////////////////////////////////////////////////////////////
  //  The following background information will help in understanding the roll
  // up formulas for first row, last row and total blocking resource usage
  // vectors.
  //
  //  A HASH JOIN consists of three stages.  Stage 1 involves building a hash
  // table with rows from the inner (right) table and writing overflow clusters
  // to disk.  Stage 2 involves taking rows from the outer (left) table, probing
  // the hash table built in stage 1 and writing overflow clusters to disk.
  // Stage 3 involves joining the overflow clusters from stages 1 and 2.
  //
  //  Stage 1 resource usage comes from the total blocking vector in the hash
  // join's preliminary cost object since no ancestor activity can proceed until
  // stage 1 finishes.
  //////////////////////////////////////////////////////////////////////////////

  //----------------------------------------------------------------------------
  //  No ancestor activity can begin until the left child has produced at least
  // one row, so the left child's first row cost goes into the total blocking
  // roll-up cost.  The remaining portion of the left child's last row activity
  // overlaps with stage 2, so use overlapped addition when combining the
  // respective resource usage.  Stage 3, however, can not proceed until stage 2
  // finishes, so use blocking addition when adding resource usage from stage 3.
  //
  //  To determine the appropriate first row activity for stages 2 and 3, use
  // the same fractions calculated in preliminary costing.
  //----------------------------------------------------------------------------
  const SimpleCostVector & leftCplr   = leftChildCost->getCplr();
  const SimpleCostVector & leftCpfr   = leftChildCost->getCpfr();
  const SimpleCostVector & stage2Cost = parentCost->getStage2Cost();
  const SimpleCostVector & stage3Cost = parentCost->getStage3Cost();

  const CostScalar & stage2Fraction = parentCost->getStage2WorkFractionForFR();
  const CostScalar & stage3Fraction = parentCost->getStage3WorkFractionForFR();

  // *************************************************************************
  // this formula assumes that leftCpfr is at most the rolled up blocking cost
  // We find that this is not true. So we further adjust the cpfr() cost below
  // *************************************************************************
  rollUpCost->cpfr() =
    blockingAdd( overlapAddUnary( leftCplr - leftCpfr, stage2Cost ) * stage2Fraction,
		 stage3Cost * stage3Fraction,
		 rpp
	       );

  //----------------------------------------------------------------------------
  //  The same basic formula for first row roll-up applies for last row as well.
  // Of course, we no longer need to worry about first row fractions for either
  // of the two stages, so the formula is simplified accordingly.
  //----------------------------------------------------------------------------

  SimpleCostVector leftChildlrCostWithOutfrCost = leftCplr - leftCpfr;
  if ( CmpCommon::getDefault(COMP_BOOL_56) == DF_OFF)
    leftChildlrCostWithOutfrCost.setIdleTime(leftCplr.getIdleTime());
  rollUpCost->cplr() = blockingAdd(overlapAddUnary(leftChildlrCostWithOutfrCost,
                                           stage2Cost),
                                stage3Cost,
                                rpp
                                );

   if ( CmpCommon::getDefault(COMP_BOOL_56) == DF_OFF)
  {
    // rolled-up last row cost should be at least the rolled up left child cost
    rollUpCost->cplr() = etMAXOF(rollUpCost->cplr(), leftCplr, rpp);
  }

 if ( CmpCommon::getDefault(COMP_BOOL_55) == DF_OFF)
 {
   rollUpCost->cpbcTotal() = computeNewBlockingCost(parentCost,
                                                    leftChildCost,
                                                    rightChildCost, 
                                                    rpp); 

   // ************************************************************
   // we subtract leftchild first row roll-up cost from
   // leftchild last row cost, when calculating parent's
   // roll-up last row cost. If the child's first row roll-up
   // cost is high, last row cost is decreasing and it is not
   // our intent
   // ************************************************************
   if ( CmpCommon::getDefault(COMP_BOOL_56) == DF_OFF)
   {
      rollUpCost->cpfr() = etMINOF(rollUpCost->cpfr(),
                                  rollUpCost->cpbcTotal(),
                                  rpp);
   }

 }
 else
 {

  //----------------------------------------------------------------------------
  //  Producing all rows but the first row from the right table overlaps with
  // building the hash table, hence the term
  //
  //  overlapAdd( (rightCplr - rightCpfr) / numProbes,
  //               parentCost->getCpbcTotal()         )
  //
  //  Note that since blocking activity is on a per probe basis, we must convert
  // the last row resource usage from a cumulative cost for all probes to an
  // average cost per probe.
  //
  //  We also add in any blocking cost from the right child using blocking
  // addition.
  //
  //  Finally, the blocking activity from the left child (including the left
  // child's first row cost) overlaps with blocking activity of the right
  // child, so we add in the left child's blocking activity with overlapped
  // addition.
  //----------------------------------------------------------------------------
  const SimpleCostVector & rightCplr = rightChildCost->getCplr();
  const SimpleCostVector & rightCpfr = rightChildCost->getCpfr();
  const CostScalar	 & numProbes = rightCplr.getNumProbes();
  const SimpleCostVector & parentTotBl = parentCost->cpbcTotal();
  const SimpleCostVector & leftChildTotBl = leftChildCost->cpbcTotal();
  if ( CmpCommon::getDefault(COMP_BOOL_39) == DF_OFF )
  {
    // get total blocking for all probes
    rollUpCost->cpbcTotal() =
      overlapAddUnary(
        leftCpfr + leftChildTotBl*(leftChildTotBl.getNumProbes()),
        blockingAdd(
          overlapAddUnary( rightCplr - rightCpfr,
                           parentTotBl*(parentTotBl.getNumProbes())
          ) + rightCpfr,
          rightChildCost->getCpbcTotal()*numProbes,rpp
        )
      );
    // scaling down blocking cost by dividing the operator blocking
    // cost by the number of probes coming from HashJoin parent.
    CostScalar scaleFactor = csOne/inLogProp_->getResultCardinality();
    rollUpCost->cpbcTotal().scaleByValue(scaleFactor);
  }
  else
    rollUpCost->cpbcTotal() =
           overlapAddUnary(leftCpfr + leftChildCost->getCpbcTotal(),
                      blockingAdd(  overlapAddUnary( ( rightCplr - rightCpfr)
                                               / numProbes,
                                              parentCost->getCpbcTotal()
                                              ) + rightCpfr,
                                  rightChildCost->getCpbcTotal(),
                                  rpp
                                 )
                     );
  }
  //----------------------------------------------------------------------------
  //  A compromise solution for first blocking is to simply set it equal to
  // total blocking.  Incorporating the first blocking cost from either leg
  // unduly penalizes the activity from the other leg, so absent a more complex
  // model, this compromise solution seemed the most reasonable.  Since
  // overlapped vectors OPFR and OPLR are not currently used, the compromise
  // does no harm at this point.
  //----------------------------------------------------------------------------
  rollUpCost->cpbc1() = rollUpCost->getCpbcTotal();

  //----------------------------------------------------------------------------
  //  Assuming that this HASH JOIN operator's process and its child processes
  // all run in separate CPUs, no interferance occurs, so the first row produced
  // by the HASH JOIN depends on the fastest of the two children and the last
  // row produced by the HASH JOIN depends on the slowest of the two children.
  //----------------------------------------------------------------------------
  /*jo rollUpCost->opfr()  = etMINOF(leftChildCost->getOpfr(),
                                rightChildCost->getOpfr(),
                                rpp);


     rollUpCost->oplr()  = etMAXOF(leftChildCost->getOplr(),
                                rightChildCost->getOplr(),
                                rpp);
  jo*/

  //-----------------------------------------------------------------------
  //  Child costs have been merged at this point, so delete local copies of
  // those costs.  The parent cost can be deleted too.
  //-----------------------------------------------------------------------
  delete leftChildCost;
  delete rightChildCost;
  delete parentCost;

    //-----------------------------------------------------------------------
  // In order to accommodate introduction of REUSE in hash join, the following
  // changes have been made to the hash join costing:
  // 1. In HashJoin::createContextForAChild, in the case of reuse, the LP returned
  // take into account the reuse and use materializeInputLogProp() method. This
  // ensures that the right child has the right number of probes (only ONE) and
  // so it returns the right number of rows scanned.
  // 2. In this method CostMethodHashJoin::computePlanCost(), the number of
  // probes is set properly to reflect on the final cost. If the inner table is
  // read only once, the numProbes is ONE, and in the case the inner table is read
  // more than once but still reused, like in the case of ordered CharacteristicInputs,
  // the numProbes is the UEC of the characteristic input fields. -OA
  // ------------------------------------------------------------------------


  HashJoin* hj = (HashJoin*) jn_;

  if( hj->isNoOverflow() AND hj->isReuse() AND hj->multipleCalls()==0 )
  {
    CostScalar newNumProbes = csOne;
     //Unique entry Count for the parent probes
    if( NOT hj->valuesGivenToChild().isEmpty() )
    {
      PhysicalProperty* spp = NULL;
      if ( myContext->getPlan())
	spp = myContext->getPlan()->getPhysicalProperty();

      CostScalar uniqueProbeCount =
	inLogProp_->getAggregateUec (hj_->valuesGivenToChild());
      if (uniqueProbeCount < noOfProbes_)
	uniqueProbeCount = noOfProbes_;

      newNumProbes = uniqueProbeCount;
      CMPASSERT(spp);
      if (!spp->isSorted())
	{
	  // assume random probes with uniqueProbeCount distinct values
	  // hash table is rebuilt once for the first probe +
	  // (1 - 1/uniqueProbeCount) times for each successive probe
	  newNumProbes =
	    csOne + (noOfProbes_ - csOne) * (csOne - csOne / uniqueProbeCount);
	}
    }

      if ( CmpCommon::getDefault(COMP_BOOL_39) == DF_ON )
      {
        rollUpCost->totalCost().setNumProbes(newNumProbes);
        rollUpCost->cpfr().setNumProbes(newNumProbes);
        rollUpCost->cplr().setNumProbes(newNumProbes);
        rollUpCost->cpbcTotal().setNumProbes(newNumProbes);
        rollUpCost->cpbc1().setNumProbes(newNumProbes); // this was missing
      }
  }


  //--------------------------------------------
  //  Return previously calculated roll-up cost.
  //--------------------------------------------
  return rollUpCost;

 } // CostMethodHashJoin::computePlanCost()

//*************************************************************************
// CostMethodHashJoin::computeNewBlockingCost()
// A simplified and more accurate calculation of the roll-up blocking cost.
// The previous rollup calculation has wrong assumptions when overlapping 
// the blocking cost with the right child last row costs. The overlap occurs
// only for the first phase, but not for subsequent phases.
//
// The right child blocking cost and the left child blocking cost overlap;
// the blocking costs of the phase 2 and the phase 3 are added using the
// blocking addition.
//
// Also the method is rewritten to make it easier to understand.
//
//*************************************************************************
SimpleCostVector CostMethodHashJoin::computeNewBlockingCost(
                                    HashJoinCost* parentCost,
                                    CostPtr leftChildCost,
                                    CostPtr rightChildCost,
                                    const ReqdPhysicalProperty *rpp)
{
  const SimpleCostVector & rightCplr = rightChildCost->getCplr();
  const SimpleCostVector & leftCplr = leftChildCost->getCplr();
  const SimpleCostVector & rightCpfr = rightChildCost->getCpfr();
  const CostScalar       & RCnumProbes = rightCplr.getNumProbes();
  const CostScalar       & LCnumProbes = leftCplr.getNumProbes();
  const CostScalar       & parentNumProbes = parentCost->
                                             getCplr().getNumProbes();

  const SimpleCostVector & stage1BK = parentCost->getStage1BKCost();
  const SimpleCostVector & stage2BK = parentCost->getStage2BKCost();
  const SimpleCostVector & stage3BK = parentCost->getStage3BKCost();
  const SimpleCostVector & leftChildTotBl = leftChildCost->cpbcTotal(); 
  const SimpleCostVector & rightChildTotBl=rightChildCost->cpbcTotal();

  SimpleCostVector  A=overlapAddUnary
                             (rightCplr - rightCpfr,
                             stage1BK * parentNumProbes) +
                            rightCpfr;

  SimpleCostVector  B=overlapAddUnary(rightChildTotBl * RCnumProbes,
                                       leftChildTotBl*LCnumProbes);

  SimpleCostVector  C= blockingAdd(B, A, rpp);

  SimpleCostVector  D=blockingAdd(C, stage2BK*parentNumProbes, rpp);
  SimpleCostVector E = blockingAdd(D, stage3BK*parentNumProbes, rpp)/parentNumProbes;

  E.setNumProbes(parentNumProbes);
  return E;
} // CostMethodHashJoin::computeNewBlockingCost()
// LCOV_EXCL_STOP
//<pb>

// ----QUICKSEARCH FOR MJ.................................................
/**********************************************************************/
/*                                                                    */
/*                        CostMethodMergeJoin                         */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodMergeJoin::cacheParameters().
// -----------------------------------------------------------------------
void CostMethodMergeJoin::cacheParameters(RelExpr* op,
                                          const Context* myContext)
{
  CostMethodJoin::cacheParameters(op,myContext);

  mj_ = (MergeJoin*) op;

  const ValueIdSet& equiJoinPreds = mj_->getEquiJoinPredicates();

  cpuCostCompareKeys_ = CostPrimitives::cpuCostForCompare(equiJoinPreds);

}  // CostMethodMergeJoin::cacheParameters().
//<pb>
// -----------------------------------------------------------------------
// CostMethodMergeJoin::computeIntervalMergingCost().
//
// This method computes the merging cost for a histogram interval. It
// does so by estimating how many times each of the following expressions
// are evaluated in the MJ executor:
//
// . mergeExpr, which compares a row from the left with a row from the
//   right. It evaluates to TRUE if they have the same merge join key.
//
// . compExpr, which is evaluated when mergeExpr evalutes to FALSE. It
//   also compares a row from the left with a row from the right and it
//   evaluates to TRUE if left row has a smaller merge join key than the
//   right row.
//
// . leftCheckDupExpr, which compares the join keys of the next row and
//   the current row of the left child.
//
// . rightCheckDupExpr, which acts like leftCheckDupExpr for the right
//   child.
//
// . preJoinExpr, which checks if a merged row matches those join preds
//   other than equi-join predicates.
//
// . postJoinExpr, which checks if a joined row satisfies the selection
//   remaining preds.
//
// There are the assumptions that within the interval, the rows are evenly
// distributed among the uec's and the join occurs for the maximum number
// of possible uec's (which is the smaller of the left and right uec's).
//
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu -- OCM code
CostScalar CostMethodMergeJoin::computeIntervalMergingCost(
  CostScalar  child0RowCount,
  CostScalar  child0Uec,
  CostScalar  child1RowCount,
  CostScalar  child1Uec,
  CostScalar& mem
  )
{
  // Just in case. uec could never be bigger than row count.
  child0Uec = MINOF(child0Uec,child0RowCount);
  child1Uec = MINOF(child1Uec,child1RowCount);

  // ---------------------------------------------------------------------
  // The counts of no of evaluations of each expression present.
  // ---------------------------------------------------------------------
  CostScalar mergeExprEvalCount(csZero);
  CostScalar compExprEvalCount(csZero);
  CostScalar leftCheckDupExprEvalCount(csZero);
  CostScalar rightCheckDupExprEvalCount(csZero);
  CostScalar preJoinExprEvalCount(csZero);
  CostScalar postJoinExprEvalCount(csZero);
  CostScalar nullInstantiateCount(csZero);

  CostScalar cpu(csZero);

  // ---------------------------------------------------------------------
  // For reasons that I will explain to you if you really wanna know them,
  // assume that there is at least one row in each interval.
  // Question from Steven: Who are you?!
  // ---------------------------------------------------------------------
  child0Uec      = (child0Uec).minCsOne();
  child0RowCount = (child0RowCount).minCsOne();
  child1Uec      = (child1Uec).minCsOne();
  child1RowCount = (child1RowCount).minCsOne();

  // ---------------------------------------------------------------------
  // Find out what the predicates are to be evaluated at this HJ node,
  // and compute cost primitives related to them.
  // First two are just place holders. We need the last two.
  // ---------------------------------------------------------------------
  ValueIdSet vs1;
  ValueIdSet vs2;
  ValueIdSet otherJoinPreds;
  ValueIdSet otherSelPreds;
  classifyPredicates(vs1, vs2, otherJoinPreds, otherSelPreds);

    if(otherJoinPreds.isEmpty())
      cpuCostEvalOtherJoinPreds_ = csZero;
    else
      cpuCostEvalOtherJoinPreds_ = CostPrimitives::
                                   cpuCostForEvalPred(otherJoinPreds);
    if(otherSelPreds.isEmpty())
      cpuCostEvalOtherSelPreds_ = csZero;
    else
      cpuCostEvalOtherSelPreds_ = CostPrimitives::
                                  cpuCostForEvalPred(otherSelPreds);

  // ---------------------------------------------------------------------
  // As in Histogram analysis, assume maximum possible potential matches.
  // ---------------------------------------------------------------------
  CostScalar child0RowCountPerUec = (child0RowCount / child0Uec);
  CostScalar child1RowCountPerUec = (child1RowCount / child1Uec);
  const CostScalar & uecMatched = MINOF(child0Uec,child1Uec);
  CostScalar uecUnmatched = (MAXOF(child0Uec,child1Uec)- uecMatched);

  // ---------------------------------------------------------------------
  // Row count assuming inner non-semi Join on the equi-join predicates.
  // ---------------------------------------------------------------------
  CostScalar mergeJoinRowCount =
                 uecMatched * child0RowCountPerUec * child1RowCountPerUec;
  CostScalar unmatchedChild0RowCount =
                                      uecUnmatched * child0RowCountPerUec;

  // ---------------------------------------------------------------------
  // Imagine there is a pointer on both the left and right. In all cases,
  // whenever we move the pointer one row ahead, either a leftCheckDupExpr
  // or a rightCheckDupExpr is then evaluated. Thus, they are evaluated a
  // number of times equal to the number of rows on each side.
  // ---------------------------------------------------------------------
  leftCheckDupExprEvalCount  = (child0RowCount - csOne);
  rightCheckDupExprEvalCount = (child1RowCount - csOne);

  // ---------------------------------------------------------------------
  // The mergeExpr is evaluated to compare a row from the left and a row
  // the right to see whether they are equal.
  //
  // Consider the case when mergeExpr evaluates to TRUE, that is, the key
  // on the left equals that on the right, then, we produce a row and uses
  // the CheckDupExpr's to move the pointers forward, producing rows along
  // the way if the key values keep unchanged on both sides. (In fact, we
  // first store the duplicate right rows in a list and then join all
  // duplicates on the left with each row in the list. At the next time we
  // apply mergeExpr again, we are comparing a different pairs of keys.
  // Thus, the mergeExpr, when evaluated to TRUE, achieves the effect of
  // pushing the pointers on the two sides to point to their next unique
  // entry.
  // ---------------------------------------------------------------------
  mergeExprEvalCount = uecMatched;

  // ---------------------------------------------------------------------
  // When mergeExpr evaluates to FALSE, compExpr is then evaluated to
  // decide which side should be moved to the next unique entry. Then,
  // CheckDupExpr's on that side is evaluated to find the row with the
  // next unique key. Thus, the mergeExpr, when evaluated to FALSE, has
  // the effect of pushing the pointer on one side to point to its next
  // unique entry together with an evaluation of compExpr. Sometimes,
  // evaluation stops once one side has been exhausted. However, if the
  // last unique key is a match, evaluation will occur till the very end.
  // For example,
  //
  //  +-----+   +-----+    When these two tables are merged, the sequence
  //  |  1  |   |  1  |    of expression evaluations are: 1-1M,1-3RD,
  //  |  2  |   |  3  |    1-2LD,2-3M,2-3C,2-6LD,6-3M,6-3C,3-6RD,6-6M.
  //  |  6  |   |  6  |    mergeExpr(M) is evaluated twice to T and twice
  //  +-----+   +-----+    to F and Expr(C) is evaluated twice, etc.
  //
  // ---------------------------------------------------------------------
  compExprEvalCount = uecUnmatched;
  mergeExprEvalCount += uecUnmatched;

  if(mj_->isLeftJoin())
  {
    // preJoinExpr is evaluated for each matched row from the merge join.
    preJoinExprEvalCount = mergeJoinRowCount;
  }
  else if(mj_->isSemiJoin() OR mj_->isAntiSemiJoin())
  {
    // -------------------------------------------------------------------
    // The pre-join predicates are evaluated on each merge-joined row we
    // get until we get a TRUE. We can then skip joining this row with
    // the remaining right rows having the same key value. Assume this
    // point occurs on the first match.
    // -------------------------------------------------------------------
    preJoinExprEvalCount = (uecMatched * child0RowCountPerUec);
  }
  else
  {
    // Inner non-semi join shouldn't have any pre-join predicates.
    preJoinExprEvalCount = csZero;
  }

  // ---------------------------------------------------------------------
  // postJoinExpr is evaluated for each row output from the merge join
  // and the other join predicates.
  // ---------------------------------------------------------------------
  if(mj_->isSemiJoin() OR mj_->isAntiSemiJoin())
  {
    // Semi Join should not have any post-join predicates.
    CMPASSERT(mj_->selectionPred().isEmpty());
    postJoinExprEvalCount = 0.;
  }
  else if(mj_->isInnerNonSemiJoin())
  {
    // -------------------------------------------------------------------
    // Since its joinPred() is empty, the post join predicates are
    // evaluated on every row through the merge join.
    // -------------------------------------------------------------------
    postJoinExprEvalCount = mergeJoinRowCount;
  }
  else
  {
    // -------------------------------------------------------------------
    // Even null-instantiated rows need to be tested by post join pred.
    // -------------------------------------------------------------------
    postJoinExprEvalCount = mergeJoinRowCount + unmatchedChild0RowCount;

    // -------------------------------------------------------------------
    // Null instantiation is done on only those rows without a match in
    // the hash table, and on those rows which are eliminated by other
    // join preds (which are not estimated here).
    // -------------------------------------------------------------------
    nullInstantiateCount = unmatchedChild0RowCount;
      if(mj_->nullInstantiatedOutput().isEmpty())
        cpuCostNullInst_ = csZero;
      else
        cpuCostNullInst_ = CostPrimitives::
          cpuCostForCopyRow(mj_->nullInstantiatedOutput().getRowLength());
  }




  // ---------------------------------------------------------------------
  // All 4 expressions (mergeExpr,compExpr,leftCheckDupExpr,rightCheckDup
  // Expr) involve comparing a pair of the merge key. They should cost
  // more or less the same.
  // ---------------------------------------------------------------------
  cpu += cpuCostCompareKeys_ * (mergeExprEvalCount + compExprEvalCount +
                  leftCheckDupExprEvalCount + rightCheckDupExprEvalCount);
  cpu += cpuCostEvalOtherJoinPreds_ * preJoinExprEvalCount;
  cpu += cpuCostEvalOtherSelPreds_ * postJoinExprEvalCount;
  cpu += cpuCostNullInst_ * nullInstantiateCount;

  // ---------------------------------------------------------------------
  // There is a cost assoicated with storing duplicate right rows in a
  // list whenever there is a key match with a left row.
  // ---------------------------------------------------------------------
  CostScalar rowsStored = uecMatched * child1RowCountPerUec;
  cpu += cpuCostInsertRowToList_ * rowsStored;
  cpu += cpuCostRewindList_ * uecMatched * (child0RowCountPerUec - csOne);
  cpu += cpuCostClearList_ * (uecMatched - csOne);
  mem = child1RowCountPerUec * listNodeSize_;

  // ---------------------------------------------------------------------
  // Assume optimiscally for semi-join that the first row satisfies other
  // preds.
  // ---------------------------------------------------------------------
  if(NOT (mj_->isSemiJoin() OR mj_->isAntiSemiJoin()))
                    cpu += cpuCostGetNextRowFromList_ * mergeJoinRowCount;

  return cpu;

}  // CostMethodMergeJoin::computeIntervalMergingCost().
//<pb>
// -----------------------------------------------------------------------
// CostMethodMergeJoin::computeTotalMergingCost()
//
// Returns a CPU cost and stores the memory cost in mem.
// -----------------------------------------------------------------------
CostScalar CostMethodMergeJoin::computeTotalMergingCost(CostScalar& mem)
{
  CostScalar cpu(csZero);

  // $$$ This is always the code path taken for Phase 1.
  if(NOT isColStatsMeaningful_)
  {
    // Merge Join does not do cross product.
    CMPASSERT(hasEquiJoinPred_);

    // -------------------------------------------------------------------
    // Do a one interval merge with the average uec.
    // -------------------------------------------------------------------
    // compute on per stream basis which uses skew adjusted cardinalities.
    cpu = computeIntervalMergingCost(child0RowCountPerStream_, 
                                     child0UecPerStream_,
                                     child1RowCountPerStream_,
                                     child1UecPerStream_, mem);
  }
  else
  {
    // $$$ This code path shouldn't be taken in Phase 1.
    CMPASSERT(FALSE);

    // -------------------------------------------------------------------
    // Traverse the histogram intervals. Compute the cost for each and
    // sum up.
    // -------------------------------------------------------------------
    HistogramSharedPtr child0Histogram =
                                  child0EquiJoinColStats_->getHistogram();
    HistogramSharedPtr child1Histogram =
                                  child1EquiJoinColStats_->getHistogram();
    HistogramSharedPtr mergedHistogram =
                                  mergedEquiJoinColStats_->getHistogram();

    CostScalar child0Uec, child0RowCount, child1Uec, child1RowCount;
    CollIndex i(1);

    // Just caching some parameters needed later.
    const CostScalar & child0RedFactor =
				  child0EquiJoinColStats_->getRedFactor();
    const CostScalar & child1RedFactor =
				  child1EquiJoinColStats_->getRedFactor();

    CostScalar memInt;

    while(i < mergedHistogram->entries())
    {
      child0RowCount =
               ((*child0Histogram)[i].getCardinality() * child0RedFactor);
      child1RowCount =
               ((*child1Histogram)[i].getCardinality() * child1RedFactor);

	  child0Uec = MINOF((*child0Histogram)[i].getUec(), child0RowCount);
	  child1Uec = MINOF((*child1Histogram)[i].getUec(), child1RowCount);

      cpu += computeIntervalMergingCost(
                child0RowCount,child0Uec,child1RowCount,child1Uec,memInt);
      i++;
      mem = MAXOF(mem,memInt);
    }
  }
  return cpu;

}  // CostMethodMergeJoin::computeTotalMergingCost().
//<pb>
// -----------------------------------------------------------------------
// CostMethodMergeJoin::computeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodMergeJoin::computeOperatorCostInternal(RelExpr* op,
                                                 const Context* myContext,
                                                 Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // Compute the cost for merging the whole set, divide the cost across
  // all the streams to give the average stream cost.
  // ---------------------------------------------------------------------
  CostScalar memLR;
  CostScalar cpuMerge = computeTotalMergingCost(memLR);
  CostScalar cpuLR = cpuCostPerProbeInit_ * noOfProbesPerStream_;
  // ---------------------------------------------------------------------
  // Just charge a fraction of the LR cost as FR cost. We cannot do any
  // better in general since we don't have an idea of how rows are divided
  // across streams and probes.
  // $$$ Actually, first row cost can better predicted by histogram in the
  // $$$ special case of a one probe serial plan.
  // ---------------------------------------------------------------------
  CostScalar cpuFR = cpuCostPerProbeInit_;
  // per stream basis estimation
  cpuLR += cpuMerge;
  cpuFR += (cpuMerge / equiJnRowCountPerStream_ / noOfProbesPerStream_);
  cpuLR += (cpuCostCopyAtp_ * myRowCount_ / countOfStreams_);
  cpuFR += cpuCostCopyAtp_;
  // Make sure the FR cost doesn't exceed per probe average of LR cost.
  cpuFR = MINOF(cpuFR,cpuLR/noOfProbesPerStream_);

  // fudge factor for cputime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  // ---------------------------------------------------------------------
  // Synthesize the cost vectors.
  // ---------------------------------------------------------------------
  const SimpleCostVector cvFR(
    cpuFR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  const SimpleCostVector cvLR(
    cpuLR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"MERGEJOIN::computeOperatorCost()\n");
    fprintf(pfp,"myRowCount=%g\n",myRowCount_.value());
    cvFR.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  Cost *costPtr = new STMTHEAP
                         Cost (&cvFR,&cvLR,NULL,cpuCount,fragmentsPerCPU);

#ifndef NDEBUG
  if( printCost )
    {
      pfp = stdout;
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodMergeJoin::computeOperatorCostInternal().
//<pb>
//==============================================================================
//  Produce a final cumulative cost for an entire subtree rooted at a specified
// Merge JOIN operator.
//
// Input:
//  mergeJoinOp -- specified Merge JOIN operator.
//
//  myContext   -- context associated with specified physical join operator
//
//  pws         -- plan work space associated with specified physical join
//                  operator.
//
//  planNumber  -- used to get appropriate child contexts.
//
// Output:
//  none
//
// Return:
//  Pointer to cumulative final cost.
//
//==============================================================================
Cost*
CostMethodMergeJoin::computePlanCost( RelExpr*             mergeJoinOp,
                                      const Context*       myContext,
                                      const PlanWorkSpace* pws,
                                      Lng32  planNumber
                                    )
{

  //----------------------------------------------------------------------
  //  Merge JOINs and Merge UNIONs share a general strategy with differing
  // details specified in specialized virtual functions.
  //----------------------------------------------------------------------
  return rollUpForBinaryOp(mergeJoinOp, myContext, pws, planNumber);

} // CostMethodMergeJoin::computePlanCost()
//<pb>
//==============================================================================
//  Merge cumulative costs of both children of JOIN operator when neither child
// has a blocking operator anywhere within its subtree.
//
// Input:
//  leftChildCost  -- pointer to cumulative cost of left child.
//
//  rightChildCost -- pointer to cumulative cost of right child.
//
// Output:
//  none
//
// Return:
//  Pointer to merged cost of both children.
//
//==============================================================================
Cost*
CostMethodMergeJoin::mergeNoLegsBlocking( const CostPtr leftChildCost,
                                          const CostPtr rightChildCost,
                                          const ReqdPhysicalProperty* const rpp)
{

  //-------------------------------------------------------------------------
  //  The vector idleT represents the amount of time during which exactly one
  // of the two child processes has provided a rows to this JOIN operator.
  // It is a vector whose idle time component is the elapsed time of the
  // slower child minus the elapsed time of the faster child.
  //-------------------------------------------------------------------------
  /*jo SimpleCostVector idleT;
  CostScalar leftChildOpfrET  =  leftChildCost->getOpfr().getElapsedTime(rpp);
  CostScalar rightChildOpfrET = rightChildCost->getOpfr().getElapsedTime(rpp);
  idleT.setIdleTime(  MAXOF( leftChildOpfrET, rightChildOpfrET )
                    - MINOF( leftChildOpfrET, rightChildOpfrET ) );
  jo*/

  //-------------------------------------
  //  Create merged cost initially empty.
  //-------------------------------------
  Cost *mergedCost = new STMTHEAP Cost();


  //-------------------------------------------------------------------------
  //  For total cost, simply accumulate all resource usage with simple vector
  // addition.
  //-------------------------------------------------------------------------
  mergedCost->totalCost() =  leftChildCost->getTotalCost()
                           + rightChildCost->getTotalCost();

  //--------------------------------------------------------------------------
  //  First and last row computations depend on knowing which child is slower.
  //--------------------------------------------------------------------------
  //jo if ( leftChildCost->getOpfr().getElapsedTime(rpp)
  //jo     <= rightChildCost->getOpfr().getElapsedTime(rpp) )
  //jo   {

      //------------------------
      //  Right child is slower.
      //------------------------

      //--------------------------------------------------------------------
      //  Since both children must produce their first row before a JION can
      // produce its first row, the merged first row cost is simple the
      // overlapped sum of the children's first row costs plus the idle time
      // waiting for the slower of the two children (i.e. the right child in
      // this case).
      //--------------------------------------------------------------------
      mergedCost->cpfr() = overlapAdd( leftChildCost->getCpfr(),
                                       rightChildCost->getCpfr() ); //jo + idleT) );

      //------------------------------------------------------------------
      //  All probes after the first probe overlap with the previous probe
      // so overlappedProbes equals (number-of-probes - 1).
      //------------------------------------------------------------------
      //jo CostScalar overlappedProbes =
      //jo                  (rightChildCost->getCplr().getNumProbes() - csOne);

      //------------------------------------------------------------------
      //  Before adding idleT to the last row cost, we must convert it
      // from an average cost to a total cost.  The term
      // idleT * overlappedProbes represents the total idle cost for
      // all probes except the last probe.  The term
      // rightChildCost->getCplr() + idleT represents the last row cost
      // of the right child plus the idle cost for the first probe.  Since
      // all but the first probe overlap with the previous probe, these
      // two terms are added together with overlapped addition.
      //------------------------------------------------------------------
       mergedCost->cplr()
        = overlapAdd(leftChildCost->getCplr(),rightChildCost->getCplr());
   //jo                  overlapAdd( rightChildCost->getCplr() + idleT,
   //jo                              idleT * overlappedProbes ) );
  /*jo   }
  else
    {

      //-----------------------
      //  Left child is slower.
      //-----------------------

      //------------------------------------------------------------------------
      //  Since both children must produce their first row before a JION can
      // produce its first row, the merged first row cost is simple the
      // overlapped sum of the children's first row costs plus the idle time
      // waiting for the slower of the two children (i.e. the left child in this
      // case).
      //------------------------------------------------------------------------
     mergedCost->cpfr() = overlapAdd( (leftChildCost->getCpfr() + idleT),
                                       rightChildCost->getCpfr()          );

      //------------------------------------------------------------------
      //  All probes after the first probe overlap with the previous probe
      // so overlappedProbes equals (number-of-probes - 1).
      //------------------------------------------------------------------
      CostScalar overlappedProbes =
                        (leftChildCost->getCplr().getNumProbes() - csOne);

      //------------------------------------------------------------------
      //  Before adding idleT to the last row cost, we must convert it
      // from an average cost to a total cost.  The term
      // idleT * overlappedProbes represents the total idle cost for
      // all probes except the last probe.  The term
      // leftChildCost->getCplr() + idleT represents the last row cost
      // of the right child plus the idle cost for the first probe.  Since
      // all but the first probe overlap with the previous probe, these
      // two terms are added together with overlapped addition.
      //------------------------------------------------------------------
      mergedCost->cplr()
        = overlapAdd(rightChildCost->getCplr(),
                     overlapAdd( leftChildCost->getCplr() + idleT,
                                 idleT * overlappedProbes ) );
    }



  //------------------------------------------------------------------------
  //  Assuming that this JOIN operator's process and its child processes all
  // run in separate CPUs, no interferance occurs, so the first row produced
  // by the JOIN depends on the fastest of the two children and the last
  // row produced by the JOIN depends on the slowest of the two children.
  //------------------------------------------------------------------------
   mergedCost->opfr() = etMINOF( leftChildCost->getOpfr(),
                                rightChildCost->getOpfr(),
                                rpp );

   mergedCost->oplr() = etMAXOF( leftChildCost->getOplr(),
                                rightChildCost->getOplr(),
                              rpp );
  jo*/

  return mergedCost;

} // CostMethodMergeJoin::mergeNoLegsBlocking
//<pb>
//==============================================================================
//  Merge cumulative costs of both children of JOIN operator when both children
// have a blocking operator somewhere within their respective subtrees.
//
// Note:  As a side effect, the right Child's blocking vectors will be
//       normalized to the number of probes for the left child.
//
// Input:
//  leftChildCost  -- pointer to cumulative cost of left child.
//
//  rightChildCost -- pointer to cumulative cost of right child.
//
//  rpp            -- Parent's required physical properties needed by lower
//                     level routines.
//
// Output:
//  none
//
// Return:
//  Pointer to merged cost of both children.
//
//==============================================================================
Cost*
CostMethodMergeJoin::mergeBothLegsBlocking(const CostPtr leftChildCost,
                                           const CostPtr rightChildCost,
                                           const ReqdPhysicalProperty* const rpp
                                          )
{

  //-------------------------------------
  //  Create merged cost initially empty.
  //-------------------------------------
  Cost *mergedCost = new STMTHEAP Cost();

  //-------------------------------------------------------------------------
  //  For total cost, simply accumulate all resource usage with simple vector
  // addition.
  //-------------------------------------------------------------------------
  mergedCost->totalCost() =  leftChildCost->getTotalCost()
                           + rightChildCost->getTotalCost();

  //----------------------------------------------------------------------------
  //  Since both children must produce their first row before a JOIN can produce
  // its first row, the merged first row cost is simple the overlapped sum of
  // the children's first row costs.
  //----------------------------------------------------------------------------
  mergedCost->cpfr() = overlapAdd( leftChildCost->getCpfr(),
                                   rightChildCost->getCpfr() );

  //-----------------------------------------------------------------
  //  Since both children act independently, merge last row vector as
  // overlapped sum of children's last row vectors.
  //-----------------------------------------------------------------
  mergedCost->cplr() = overlapAdd( leftChildCost->getCplr(),
                                   rightChildCost->getCplr() );

  //--------------------------------------------------------------------
  //  Normalize right child's blocking vectors to left child's number of
  // probes.
  //--------------------------------------------------------------------
  rightChildCost->cpbcTotal().normalize(
                                    leftChildCost->getCpbcTotal().getNumProbes()
                                       );
  rightChildCost->cpbc1().normalize( leftChildCost->getCpbc1().getNumProbes() );


  //-------------------------------------------------------------------------
  //  The vector idleT represents the amount of time during which exactly one
  // of the two child processes has provided a rows to this JOIN operator.
  // It is a vector whose idle time component is the elapsed time of the
  // slower child minus the elapsed time of the faster child.
  //-------------------------------------------------------------------------
  /*jo SimpleCostVector idleT;
  CostScalar leftChildOpfrET  =  leftChildCost->getOpfr().getElapsedTime(rpp);
  CostScalar rightChildOpfrET = rightChildCost->getOpfr().getElapsedTime(rpp);
  idleT.setIdleTime(  MAXOF( leftChildOpfrET, rightChildOpfrET )
                    - MINOF( leftChildOpfrET, rightChildOpfrET ) );

  jo*/

  //-------------------------------------------------------------------------
  //  The total blocking vector formula resembles that of the last row vector
  // formula with the addition of any idle time.
  //-------------------------------------------------------------------------
  mergedCost->cpbcTotal()
              = overlapAdd( leftChildCost->getCpbcTotal(),
                            rightChildCost->getCpbcTotal() ); //jo + idleT;

  //----------------------------------------------------------------------------
  //  In the formula for the current process first blocking vector, the quantity
  // etMINOF(...) represents the cost of the faster child. The quantity
  // vecMINOF(...) represents the interferance factor.
  //----------------------------------------------------------------------------
  mergedCost->cpbc1()
                  = overlapAdd( etMINOF( leftChildCost->getCpbc1(),
                                         rightChildCost->getCpbc1(),
                                         rpp ),
                                vecMINOF( leftChildCost->getCpbc1(),
                                          rightChildCost->getCpbc1() ) ); //jo + idleT;

  //------------------------------------------------------------------------
  //  Assuming that this JOIN operator's process and its child processes all
  // run in separate CPUs, no interferance occurs, so the first row produced
  // by the JOIN depends on the fastest of the two children and the last
  // row produced by the JOIN depends on the slowest of the two children.
  //------------------------------------------------------------------------
  //jo mergedCost->opfr() = etMINOF( leftChildCost->getOpfr(),
  //jo                              rightChildCost->getOpfr(),
  //jo                              rpp );

  //jo mergedCost->oplr() = etMAXOF( leftChildCost->getOplr(),
  //jo                              rightChildCost->getOplr(),
  //jo                              rpp );

  return mergedCost;

} // CostMethodMergeJoin::mergeBothLegsBlocking
//<pb>

// ----QUICKSEARCH FOR NJ.................................................


/**********************************************************************/
/*                                                                    */
/*                        CostMethodNestedJoin                        */
/*                                                                    */
/**********************************************************************/

//==============================================================================
//  Merge cumulative costs of both children of NESTED JOIN operator when neither
// child has a blocking operator anywhere within its subtree.
//
// Input:
//  leftChildCost  -- pointer to cumulative cost of left child.
//
//  rightChildCost -- pointer to cumulative cost of right child.
//
// Output:
//  none
//
// Return:
//  Pointer to merged cost of both children.
//
//==============================================================================
Cost*
CostMethodNestedJoin::mergeNoLegsBlocking(const CostPtr leftChildCost,
                                          const CostPtr rightChildCost,
                                          const ReqdPhysicalProperty* const rpp)
{
  //-------------------------------------
  //  Create merged cost initially empty.
  //-------------------------------------
  Cost *mergedCost = new STMTHEAP Cost();

  //-------------------------------------------------------------------------
  //  For a nested join, view left child as a unary child of the right child
  // and use non-blocking unary roll-up formulas for the child merge.  Since
  // neither child has blocking costs, we do not merge blocking vectors and
  // thus implicitly leave them as zero vectors.
  //-------------------------------------------------------------------------

  //------------------------------------------------
  //  Same formula as in non-blocking unary roll-up.
  //------------------------------------------------
  mergedCost->totalCost() =  leftChildCost->getTotalCost()
                           + rightChildCost->getTotalCost();

  //------------------------------------------------------------------------
  //  Same formula as in non-blocking unary roll-up.
  //
  //  The formula for CPFR relies on the assumption that the first probe in
  // the right child produces the actual first row from the NLJ operator.
  // When producing the first row from the NLJ operator requires multiple
  // probes, all probes after the first probe overlap with the immediately
  // previous probe.  This implies usage of overlapped addition instead of
  // blocking addition.  We could also have chosen to use simple vector
  // addition as a compromise, but it seemd most likely that the first probe
  // would produce the first output row, so we kept blocking addition.
  //------------------------------------------------------------------------
  mergedCost->cpfr() = blockingAdd(leftChildCost->getCpfr(),
                                   rightChildCost->getCpfr(),
                                   rpp );

  //------------------------------------------------
  //  Same formula as in non-blocking unary roll-up.
  //------------------------------------------------
  mergedCost->cplr() = overlapAdd(rightChildCost->getCplr(),
                                  ( leftChildCost->getCplr()
                                     - leftChildCost->getCpfr() ) )
                        + leftChildCost->getCpfr();

  //-----------------------------------------------------------------
  //  Ensure that no component of merged first row vector exceeds the
  // corresponding component of merged last row vector.
  //-----------------------------------------------------------------
  mergedCost->cpfr().enforceUpperBound(mergedCost->cplr());

  //------------------------------------------------------------------------
  //  Unilke the formula for CPFR, in the formula for OPFR the compromise
  // solution of simple vector addition seemed more appropriate.  If the NLJ
  // operator's immediate left child was an exchange node, then we would use
  // blocking addition as in CPFR, but in general there could be many
  // non-blocking operators between the exchange node and the left child, so
  // blocking addition seemed too pessimistic.
  //------------------------------------------------------------------------
  //jo mergedCost->opfr() = leftChildCost->getOpfr() + rightChildCost->getOpfr();

  //--------------------------------------------------------------------------
  //  In the formula for OPLR, the term leftChildCost->getOpfr() represents
  // the initial activity needed to prime the pump.  The term
  //
  //       vecMAXOF(rightChildCost->getOplr(),
  //                (leftChildCost->getOplr() - leftChildCost->getOpfr()) )
  //
  //  represents the largest resource usage between the right leg activity and
  // the remaining left leg activity.
  //--------------------------------------------------------------------------
  //jo mergedCost->oplr() = vecMAXOF(rightChildCost->getOplr(),
  //jo                              ( leftChildCost->getOplr()
  //jo                                 - leftChildCost->getOpfr() )
  //jo                             )
  //jo                      + leftChildCost->getOpfr();

  return mergedCost;

}  // CostMethodNestedJoin::mergeNoLegsBlocking
//<pb>
//==============================================================================
//  Merge cumulative costs of both children of NESTED JOIN operator when only
// the left child has a blocking operator anywhere within its subtree.
//
// Input:
//  leftChildCost  -- pointer to cumulative cost of left child.
//
//  rightChildCost -- pointer to cumulative cost of right child.
//
// Output:
//  none
//
// Return:
//  Pointer to merged cost of both children.
//
//==============================================================================
Cost*
CostMethodNestedJoin::mergeLeftLegBlocking
                                         ( const CostPtr leftChildCost,
                                           const CostPtr rightChildCost,
                                           const ReqdPhysicalProperty* const rpp
                                         )
{

  //-------------------------------------
  //  Create merged cost initially empty.
  //-------------------------------------
  Cost *mergedCost = new STMTHEAP Cost();

  //--------------------------------------------------------------------------
  //  For a nested join, view left child as a unary child of the right child.
  // Since the right leg is non-blocking, use non-blocking unary roll-up
  // formulas for the child merge.
  //--------------------------------------------------------------------------

  //--------------------------------------------------------------
  //  The formulas for totalCost, CPFR and CPLR are the same as in
  // non-blocking unary roll-up.
  //--------------------------------------------------------------
  mergedCost->totalCost() =  leftChildCost->getTotalCost()
                           + rightChildCost->getTotalCost();

  mergedCost->cpfr() = blockingAdd(leftChildCost->getCpfr(),
                                   rightChildCost->getCpfr(),
                                   rpp );

  mergedCost->cplr() = overlapAdd(rightChildCost->getCplr(),
                                  ( leftChildCost->getCplr()
                                     - leftChildCost->getCpfr() ) )
                        + leftChildCost->getCpfr();

  //-----------------------------------------------------------------
  //  Ensure that no component of merged first row vector exceeds the
  // corresponding component of merged last row vector.
  //-----------------------------------------------------------------
  mergedCost->cpfr().enforceUpperBound(mergedCost->cplr());

  //--------------------------------------------------------------------------
  //  Since we view the left leg as logically coming underneath the right leg,
  // and since by assumption only the left leg has blocking activity, CPBC1
  // and CPBCTotal come from the left child after normalizing to the right
  // child's number of probes.
  //--------------------------------------------------------------------------
  const CostScalar & rightNumProbes = rightChildCost->getCplr().getNumProbes();
  mergedCost->cpbc1() =
    leftChildCost->getCpbc1().getNormalizedVersion(rightNumProbes);
  mergedCost->cpbcTotal() =
    leftChildCost->getCpbcTotal().getNormalizedVersion(rightNumProbes);

  //--------------------------------------------------------------
  //  The formula for OPFR is the same as when neither leg blocks.
  //--------------------------------------------------------------
  //jo mergedCost->opfr() = blockingAdd(leftChildCost->getOpfr(),
  //jo                                  rightChildCost->getOpfr(),
  //jo                                  rpp );

  //--------------------------------------------------------------------------
  //  By assumption, the left leg blocks, so the right leg can not begin until
  // the left leg has produced all its rows from its first probe.  Thus, in
  // the formula for OPLR, the term
  //
  //       leftChildCost->getOplr() / leftOvNumProbes
  //
  // represents the initial activity needed to prime the pump.  The term
  //
  //       vecMAXOF(rightChildCost->getOplr(),
  //                (leftChildCost->getOplr()
  //                  - (leftChildCost->getOplr() / leftOvNumProbes ) ) )
  //
  //  represents the largest resource usage between the right leg activity and
  // the remaining left leg activity.
  //--------------------------------------------------------------------------
  //jo const CostScalar & leftOvNumProbes = leftChildCost->getOplr().getNumProbes();
  //jo mergedCost->oplr() = vecMAXOF(rightChildCost->getOplr(),
  //jo                               ( leftChildCost->getOplr()
  //jo                                  - (leftChildCost->getOplr()
  //jo                                      / leftOvNumProbes )     )
  //jo                              )
  //jo                       + ( leftChildCost->getOplr() / leftOvNumProbes);

  return mergedCost;

}  // CostMethodNestedJoin::mergeLeftLegBlocking
//<pb>
//==============================================================================
//  Merge cumulative costs of both children of NESTED JOIN operator when only
// the right child has a blocking operator anywhere within its subtree.
//
// Input:
//  leftChildCost  -- pointer to cumulative cost of left child.
//
//  rightChildCost -- pointer to cumulative cost of right child.
//
// Output:
//  none
//
// Return:
//  Pointer to merged cost of both children.
//
//==============================================================================
Cost*
CostMethodNestedJoin::mergeRightLegBlocking
                                         ( const CostPtr leftChildCost,
                                           const CostPtr rightChildCost,
                                           const ReqdPhysicalProperty* const rpp
                                         )
{

  //-------------------------------------
  //  Create merged cost initially empty.
  //-------------------------------------
  Cost *mergedCost = new STMTHEAP Cost();

  //-------------------------------------------------------------------------
  //  For total cost, simply accumulate all resource usage with simple vector
  // addition.
  //-------------------------------------------------------------------------
  mergedCost->totalCost() =  leftChildCost->getTotalCost()
                           + rightChildCost->getTotalCost();

  //-------------------------------------------------------------------
  //  Since right child blocks, the merged first row cost is simply the
  // right child's first row cost.
  //-------------------------------------------------------------------
  mergedCost->cpfr() = rightChildCost->getCpfr();

  //-------------------------------------------------------------------------
  //  All of the left child's last row activity except the portion devoted to
  // first row activity overlaps with the right child's last row activity.
  //-------------------------------------------------------------------------
  mergedCost->cplr() = overlapAdd(rightChildCost->getCplr(),
                                  (leftChildCost->getCplr()
                                    - leftChildCost->getCpfr()) );

  //-----------------------------------------------------------------
  //  Ensure that no component of merged first row vector exceeds the
  // corresponding component of merged last row vector.
  //-----------------------------------------------------------------
  mergedCost->cpfr().enforceUpperBound(mergedCost->cplr());

  //-------------------------------------------------------------------------
  //  Since the right child's blocking activity can't begin until it receives
  // one row from the left child, we add the left child's first row activity
  // to both of the right child's blocking vectors.
  //-------------------------------------------------------------------------
  const CostScalar & rightNumProbes =
    rightChildCost->getCpbcTotal().getNumProbes();

  SimpleCostVector blkLeftCpfr = leftChildCost->getCpfr() / rightNumProbes;

  mergedCost->cpbc1()          = blockingAdd(rightChildCost->getCpbc1(),
                                             blkLeftCpfr,
                                             rpp);

  mergedCost->cpbcTotal()      = blockingAdd(rightChildCost->getCpbcTotal(),
                                             blkLeftCpfr,
                                             rpp);

  //---------------------------------------------------------------------
  //  The formulas for OPFR and OPLR are the same as when neither leg has
  // any blocking activity.
  //---------------------------------------------------------------------
  //jo mergedCost->opfr() = leftChildCost->getOpfr() + rightChildCost->getOpfr();

  //jo mergedCost->oplr() = vecMAXOF(rightChildCost->getOplr(),
  //jo                               ( leftChildCost->getOplr()
  //jo                                  - leftChildCost->getOpfr() )
  //jo                              )
  //jo                       + leftChildCost->getOpfr();

  return mergedCost;

}  // CostMethodNestedJoin::mergeRightLegBlocking
//<pb>
//==============================================================================
//  Merge cumulative costs of both children of NESTED JOIN operator when both
// children have a blocking operator somewhere within their respective subtrees.
//
// Input:
//  leftChildCost  -- pointer to cumulative cost of left child.
//
//  rightChildCost -- pointer to cumulative cost of right child.
//
//  rpp            -- Parent's required physical properties needed by lower
//                     level routines.
//
// Output:
//  none
//
// Return:
//  Pointer to merged cost of both children.
//
//==============================================================================
Cost*
CostMethodNestedJoin::mergeBothLegsBlocking(
                                           const CostPtr leftChildCost,
                                           const CostPtr rightChildCost,
                                           const ReqdPhysicalProperty* const rpp
                                           )
{

  //-------------------------------------
  //  Create merged cost initially empty.
  //-------------------------------------
  Cost *mergedCost = new STMTHEAP Cost();

  //-------------------------------------------------------------------------
  //  For total cost, simply accumulate all resource usage with simple vector
  // addition.
  //-------------------------------------------------------------------------
  mergedCost->totalCost() =  leftChildCost->getTotalCost()
                           + rightChildCost->getTotalCost();

  //-------------------------------------------------------------------
  //  Since right child blocks, the merged first row cost is simply the
  // right child's first row cost.
  //-------------------------------------------------------------------
  mergedCost->cpfr() = rightChildCost->getCpfr();

  //-------------------------------------------------------------------------
  //  All of the left child's last row activity except the portion devoted to
  // first row activity overlaps with the right child's last row activity.
  //-------------------------------------------------------------------------
  mergedCost->cplr() = overlapAdd(rightChildCost->getCplr(),
                                  (leftChildCost->getCplr()
                                    - leftChildCost->getCpfr()) );

  //-----------------------------------------------------------------
  //  Ensure that no component of merged first row vector exceeds the
  // corresponding component of merged last row vector.
  //-----------------------------------------------------------------
  mergedCost->cpfr().enforceUpperBound(mergedCost->cplr());

  //-----------------------------------------------------------------------
  //  Determine right child's number of probes for normalizing left child's
  // blocking vectors appropriately.
  //-----------------------------------------------------------------------
  const CostScalar & rightNumProbes =
    rightChildCost->getCpbcTotal().getNumProbes();

  //-----------------------------------------------------------------------
  //  Since left child has blocking activity, it's lowest blocking operator
  // becomes the merged lowest blocking operator.
  //-----------------------------------------------------------------------
  mergedCost->cpbc1() =
                 leftChildCost->getCpbc1().getNormalizedVersion(rightNumProbes);

  //--------------------------------------------------------------------------
  //  The right child's blocking activity can't begin until the left child has
  // completed all of its blocking activity and returned at least one row.
  // Thus, we combine the left child's total blocking activity, the left
  // child's first row activity and the right child's total blocking activity
  // to produced merged total blocking activity.
  //--------------------------------------------------------------------------
  SimpleCostVector normLeftCpbcTotal =
             leftChildCost->getCpbcTotal().getNormalizedVersion(rightNumProbes);

  SimpleCostVector blkLeftCpfr       = leftChildCost->getCpfr() / rightNumProbes;

  mergedCost->cpbcTotal()            =
             blockingAdd(blockingAdd(rightChildCost->getCpbcTotal(),
                                     blkLeftCpfr,
                                     rpp),
                          normLeftCpbcTotal,
                          rpp);

  //---------------------------------------------------------------------
  //  The formulas for OPFR and OPLR are the same as when only the left
  // leg has any blocking activity.
  //---------------------------------------------------------------------
  //jo mergedCost->opfr() = blockingAdd(leftChildCost->getOpfr(),
  //jo                                  rightChildCost->getOpfr(),
  //jo                                  rpp );

  //jo const CostScalar & leftOvNumProbes = leftChildCost->getOplr().getNumProbes();
  //jo mergedCost->oplr() = vecMAXOF(rightChildCost->getOplr(),
  //jo                               ( leftChildCost->getOplr()
  //jo                                  - (leftChildCost->getOplr()
  //jo                                      / leftOvNumProbes )     )
  //jo                              )
  //jo                       + ( leftChildCost->getOplr() / leftOvNumProbes);

  return mergedCost;

} //CostMethodNestedJoin::mergeBothLegsBlocking
// LCOV_EXCL_STOP
//<pb>
// -----------------------------------------------------------------------
// CostMethodNestedJoin::cacheParameters()
// -----------------------------------------------------------------------
void CostMethodNestedJoin::cacheParameters(RelExpr* op,
                                           const Context* myContext)
{
  CostMethodJoin::cacheParameters(op,myContext);
  nj_ = (NestedJoin*) op;

  // ---------------------------------------------------------------------
  // All predicates on a NJ are pushable to the right leg, except in the
  // case of a left join. Some of its selectionPred() may be needed to be
  // evaluated at the NJ node itself after null-instantiation. In that
  // case, it's ok to push down the part of selectionPred() which can be
  // pushed down to the left, but not so for the right. Otherwise, we can
  // end up returning a null-instantiated row, which should be eliminated
  // by the selectionPred().
  // ---------------------------------------------------------------------
  if(nj_->isLeftJoin() AND (NOT nj_->selectionPred().isEmpty()))
  {
    cpuCostEvalPred_ = CostPrimitives::cpuCostForEvalPred(
                                                    nj_->selectionPred());
  }
  else
  {
    cpuCostEvalPred_ = csZero;
  }

  // ---------------------------------------------------------------------
  // If it's a left join, there might be a possibility that we need to
  // null-instantiate a row "explicitly". In that case, we need to account
  // for the CPU cost and extra buffers.
  // ---------------------------------------------------------------------
  if(nj_->isLeftJoin() AND (NOT nj_->nullInstantiatedOutput().isEmpty()))
  {
    cpuCostNullInst_ = CostPrimitives::cpuCostForCopyRow(
                            nj_->nullInstantiatedOutput().getRowLength());
  }
  else
  {
    cpuCostNullInst_ = csZero;
  }

}
//<pb>
// -----------------------------------------------------------------------
// CostMethodNestedJoin::computeOperatorCostInternal()
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu -- OCM code
Cost*
CostMethodNestedJoin::computeOperatorCostInternal(RelExpr* op,
                                                  const Context* myContext,
                                                  Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // CostScalars to be computed.
  // ---------------------------------------------------------------------
  CostScalar cpuFR(csZero), cpuLR(csZero), mem(csZero);

  // ---------------------------------------------------------------------
  // Work in NJ operator is divided into three phases.
  //
  // In the first phase, a minimal cost is incurred in passing request
  // from its parent to its left child plus some book-keeping cost for
  // handling multiple requests.
  //
  // Phase two involves passing the atp's from its the left queue to its
  // right queue, which is done for each row returned from the left.
  //
  // In the last phase, the predicates are evaluated, and the ATPs of the
  // satisfying rows are copied to the parent queue.
  //
  // Below, we are considering the total cost for all probes on all
  // streams, and then amortize the resulting cost over the streams.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Phase 1 cost.
  // ---------------------------------------------------------------------
  CostScalar noOfProbesNeededToGetFirstRow =
                            (noOfProbes_/myRowCount_).minCsOne();
  if(nj_->isLeftJoin() AND nj_->selectionPred().isEmpty())
                           noOfProbesNeededToGetFirstRow = csOne;
  noOfProbesNeededToGetFirstRow =
                         MINOF(noOfProbesNeededToGetFirstRow,noOfProbes_);
  cpuFR += cpuCostPerProbeInit_ * noOfProbesNeededToGetFirstRow;
  cpuLR += cpuCostPerProbeInit_ * noOfProbes_;

  // ---------------------------------------------------------------------
  // Phase 2 cost.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // This is an estimate of the no of rows passed to the right to get the
  // first row returned.
  // ---------------------------------------------------------------------
  CostScalar leftRowsNeededToGetFirstRow =
                        MIN_ONE(child0RowCount_/myRowCount_);
  if(nj_->isLeftJoin() AND nj_->selectionPred().isEmpty())
                             leftRowsNeededToGetFirstRow = csOne;
  leftRowsNeededToGetFirstRow =
                       MINOF(leftRowsNeededToGetFirstRow,child0RowCount_);
  cpuFR += cpuCostPassRow_ * leftRowsNeededToGetFirstRow;
  cpuLR += cpuCostPassRow_ * child0RowCount_;

  // ---------------------------------------------------------------------
  // Phase 3 cost.
  // ---------------------------------------------------------------------

  if( NOT cpuCostEvalPred_.isZero() )
  {
    CostScalar rightRowsNeededToGetFirstRow =
                        (child1RowCount_/myRowCount_).minCsOne();
    rightRowsNeededToGetFirstRow =
                      MINOF(rightRowsNeededToGetFirstRow,child1RowCount_);
    cpuFR += (cpuCostEvalPred_ * rightRowsNeededToGetFirstRow);
    cpuLR += (cpuCostEvalPred_ * child1RowCount_);
  }

  // ---------------------------------------------------------------------
  // If it's a left join, there might be a possibility that we need to
  // null-instantiate a row "explicitly". In that case, we need to account
  // for the CPU cost and extra buffers.
  // $$$ This is very crude estimate. Can interface better with Logical
  // $$$ property synthesis to get better estimate using Left Join stats.
  // ---------------------------------------------------------------------
  if( NOT cpuCostNullInst_.isZero() )
  {
    if(child1RowCount_ < child0RowCount_)
    {
      cpuFR += cpuCostNullInst_ * countOfStreams_;
      cpuLR += cpuCostNullInst_ *
                (child0RowCount_ - child1RowCount_).minCsOne();
    }

    // -------------------------------------------------------------------
    // Buffers are only allocated to hold the rows which are "explicitly"
    // null-instantiated. Otherwise, NLJ needs negligible memory to run.
    // -------------------------------------------------------------------
    mem = CostScalar(bufferCount_ * bufferSize_);
  }

  // ---------------------------------------------------------------------
  // Finally, account for the cost of passing the row to the parent.
  // ---------------------------------------------------------------------
  cpuFR += cpuCostPassRow_ * countOfStreams_;
  cpuLR += cpuCostPassRow_ * myRowCount_;

  // ---------------------------------------------------------------------
  // Amortize the cpu costs across multiple streams of execution.
  // ---------------------------------------------------------------------
  cpuFR /= countOfStreams_;
  cpuLR /= countOfStreams_;

  // ---------------------------------------------------------------------
  // Compute the per-probe average FR cost. Make sure it doesn't exceed
  // the per-probe average of the LR cost.
  // ---------------------------------------------------------------------
  cpuFR /= noOfProbesPerStream_;
  cpuFR = MINOF(cpuFR,cpuLR/noOfProbesPerStream_);

  //fudge factor for cputime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();


  // ---------------------------------------------------------------------
  // Synthesize the cost vectors.
  // ---------------------------------------------------------------------
  const SimpleCostVector cvFR(
    cpuFR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  const SimpleCostVector cvLR(
    cpuLR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"NESTEDJOIN::computeOperatorCost()\n");
    fprintf(pfp,"myRowCount=%g;child0RowCount=%g;child1RowCount=%g\n",
     myRowCount_.value(),child0RowCount_.value(),child1RowCount_.value());
    cvFR.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  Cost *costPtr = new STMTHEAP
                         Cost (&cvFR,&cvLR,NULL,cpuCount,fragmentsPerCPU);

#ifndef NDEBUG
  if ( printCost )
    {
      pfp = stdout;
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodNestedJoin::computeOperatorCostInternal().
//<pb>
//==============================================================================
//  Produce a final cumulative cost for an entire subtree rooted at a specified
// physical NESTED LOOPS JOIN operator.
//
// Input:
//  nestedJoinOp -- specified physical nested loops join operator.
//
//  myContext    -- context associated with specified physical nested join
//                    operator.
//
//  pws          -- plan work space associated with specified physical join
//                    operator.
//
//  planNumber   -- used to get appropriate child contexts.
//
// Output:
//  none
//
// Return:
//  Pointer to cumulative final cost.
//
//==============================================================================
Cost*
CostMethodNestedJoin::computePlanCost( RelExpr*             nestedJoinOp,
                                       const Context*       myContext,
                                       const PlanWorkSpace* pws,
                                       Lng32                 planNumber)
{

  //--------------------------------------------------------------------------
  //  Get cumulative costs associated with each child of this nested loops
  // join operator.
  //--------------------------------------------------------------------------
  CostPtr leftChildCost;
  CostPtr rightChildCost;
  getChildCostsForBinaryOp( nestedJoinOp
                          , myContext
                          , pws
                          , planNumber
                          , leftChildCost
                          , rightChildCost);

  //--------------------------------------------------------------------------
  // For index joins we set the priority of the right child to normal
  // priority in order to to double count the scan priority when we combine
  // the children priorities.
  //--------------------------------------------------------------------------
  if((nestedJoinOp->getGroupAttr()->getNumBaseTables() == 1) AND
     (nestedJoinOp->child(0).getGroupAttr()->getNumBaseTables() == 1))
  {
    // We right on an Index_Join. Do the correction.
    rightChildCost->planPriority().resetToNormal();
    // Reset also original copy of cost object
    // Fix for coverity cid 1096: NULL_RETURNS
    Context * childContext = pws->getChildContext( 1, planNumber);
    if (childContext != NULL)
    ((Cost*) (childContext->getSolution()->getRollUpCost()))
           ->planPriority().resetToNormal();
  }

  //--------------------------------------------------------------------------
  //  Merging of children's costs depend on which (if any) children of this
  // nested loops join operator have blocking costs.
  //--------------------------------------------------------------------------
  Cost* mergedChildCost;
  const ReqdPhysicalProperty* rpp = myContext->getReqdPhysicalProperty();
  if ( leftChildCost->getCpbcTotal().isZeroVectorWithProbes() )
    {
      if ( rightChildCost->getCpbcTotal().isZeroVectorWithProbes() )
        {

          //-------------------------------------------------------
          //  Neither child has a blocking operator in its subtree.
          //-------------------------------------------------------
          mergedChildCost = mergeNoLegsBlocking(leftChildCost,
                                                rightChildCost,
                                                rpp);
        }
      else
        {

          //----------------------------------------------------------
          //  Only right child has a blocking operator in its subtree.
          //----------------------------------------------------------
          mergedChildCost = mergeRightLegBlocking(leftChildCost,
                                                  rightChildCost,
                                                  rpp);
        }
    }
  else
    {
      if ( rightChildCost->getCpbcTotal().isZeroVectorWithProbes() )
        {

          //---------------------------------------------------------
          //  Only left child has a blocking operator in its subtree.
          //---------------------------------------------------------
          mergedChildCost = mergeLeftLegBlocking(leftChildCost,
                                                 rightChildCost,
                                                 rpp);
        }
      else
        {

          //----------------------------------------------------------
          //  Both children have blocking operators in their subtrees.
          //----------------------------------------------------------
          mergedChildCost = mergeBothLegsBlocking(leftChildCost,
                                                  rightChildCost,
                                                  rpp);
        }
    }

  //-----------------------------------------------------------------------
  //  Child costs have been merged at this point, so delete local copies of
  // those costs.
  //-----------------------------------------------------------------------
  delete leftChildCost;
  delete rightChildCost;

  //----------------------------------------------------------------------
  //  Get addressability to parent cost in plan workspace and roll this up
  // with the recently calculated merged children cost.
  //----------------------------------------------------------------------
  Cost* parentCost = ((PlanWorkSpace *)pws)->getFinalOperatorCost(planNumber);
  Cost* rollUpCost = rollUp(parentCost, mergedChildCost, rpp);

  //----------------------------------------------------------------------
  // The parent cost and the local copy of merged child cost have been
  // rolled up at this point, so delete them.
  //----------------------------------------------------------------------
  delete mergedChildCost;
  delete parentCost;

  //--------------------------------------------
  //  Return previously calculated roll-up cost.
  //--------------------------------------------
  return rollUpCost;

 } // CostMethodNestedJoin::computePlanCost()
//<pb>
// ----QUICKSEARCH FOR NJF................................................

/**********************************************************************/
/*                                                                    */
/*                      CostMethodNestedJoinFlow                      */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodNestedJoinFlow::computeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodNestedJoinFlow::computeOperatorCostInternal(RelExpr* op,
                                                      const Context* myContext,
                                                      Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // The NestedJoinFlow operator doesn't produce a row, it is just used
  // to pass row from the left to the right. It doesn't do anything to
  // any row produced by the right.
  // ---------------------------------------------------------------------
  CostScalar cpuLR = (cpuCostPassRow_ * child0RowCountPerStream_);

  // ---------------------------------------------------------------------
  // First row cost is just the cost for the complete probe, since NJF
  // doesn't produce any rows.
  // ---------------------------------------------------------------------
  CostScalar cpuFR = cpuLR / noOfProbesPerStream_;

  //fudge factor for cputime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();


  // ---------------------------------------------------------------------
  // Synthesize the cost vectors.
  // ---------------------------------------------------------------------
  const SimpleCostVector cvFR(
    cpuFR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  const SimpleCostVector cvLR(
    cpuLR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"NESTEDJOINFLOW::computeOperatorCost()\n");
    fprintf(pfp,"childRowCount=%g",child0RowCount_.value());
    cvFR.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  Cost *costPtr = new STMTHEAP
                         Cost (&cvFR,&cvLR,NULL,cpuCount,fragmentsPerCPU);

#ifndef NDEBUG
  if ( printCost )
    {
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodNestedJoinFlow::computeOperatorCostInternal().
// LCOV_EXCL_STOP
//<pb>

// ----QUICKSEARCH FOR MU.................................................

/**********************************************************************/
/*                                                                    */
/*                        CostMethodMergeUnion                        */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodMergeUnion::cacheParameters().
// -----------------------------------------------------------------------
void CostMethodMergeUnion::cacheParameters(
                                    RelExpr* op, const Context* myContext)
{
  CostMethod::cacheParameters(op,myContext);

  mu_ = (MergeUnion*) op;

  ValueIdSet sortKeyVis;
  sortKeyVis.insertList(mu_->getSortOrder());

  cpuCostCopyRow_ = CostPrimitives::cpuCostForCopySet(myVis());
  cpuCostCompareKeys_ = (sortKeyVis.isEmpty() ?
                      csZero : CostPrimitives::cpuCostForCompare(sortKeyVis));
}
//<pb>
// -----------------------------------------------------------------------
// CostMethodMergeUnion::computeOperatorCostInternal().
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu -- OCM code
Cost*
CostMethodMergeUnion::computeOperatorCostInternal(RelExpr* op,
                                                  const Context* myContext,
                                                  Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // CostScalars to be computed.
  // ---------------------------------------------------------------------
  CostScalar cpuFR(csZero), cpuLR(csZero), mem(csZero);

  // ---------------------------------------------------------------------
  // Some start up cost.
  // ---------------------------------------------------------------------
  cpuFR += cpuCostPerProbeInit_;
  cpuLR += cpuCostPerProbeInit_ * noOfProbes_;

  // ---------------------------------------------------------------------
  // Assume left and right children progresses as same rate for FR cost.
  // When a sort order is required, we need a row from both left and right
  // to produce one row.
  // ---------------------------------------------------------------------
  if( cpuCostCompareKeys_.isZero() )
    cpuFR += cpuCostCopyAtp_ + cpuCostCopyRow_;
  else
    cpuFR += (cpuCostCopyAtp_ + cpuCostCopyRow_) * 2 + cpuCostCompareKeys_;

  // ---------------------------------------------------------------------
  // To pass the result back to up queue to parent.
  // ---------------------------------------------------------------------
  cpuFR += cpuCostCopyAtp_ * csTwo;

  // ---------------------------------------------------------------------
  // Processing cost includes copying each row from left and right to the
  // buffer, evaluating the merge expression if there is one, and copy the
  // ATP to its up queue.
  // ---------------------------------------------------------------------
  cpuLR += (cpuCostCopyRow_ + cpuCostCompareKeys_ + cpuCostCopyAtp_) *
                                                              myRowCount_;

  // ---------------------------------------------------------------------
  // The merge union operator allocates a buffer pool of five buffers,
  // each of size 10024 bytes (yes, 10024 bytes, ie. 1033.7891 kbytes) at
  // the beginning. It sticks with using only these buffers thereafter.
  // ---------------------------------------------------------------------
  mem = CostScalar(bufferSize_ * bufferCount_);

  // ---------------------------------------------------------------------
  // Average the LR cost across all the streams available. The FR cost
  // has been computed based on on a probe in a single stream and needn't
  // been averaged out. However, we don't want the per probe average of
  // the FR cost to be higher than that of the last row cost and which may
  // happen when we average the LR cost out.
  // ---------------------------------------------------------------------
  cpuLR /= countOfStreams_;
  cpuFR = MINOF(cpuFR,cpuLR/noOfProbesPerStream_);

  //fudge factor for cputime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  // ---------------------------------------------------------------------
  // Synthesize the cost vectors.
  // ---------------------------------------------------------------------
  const SimpleCostVector cvFR (
    cpuFR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  const SimpleCostVector cvLR (
    cpuLR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"MERGEUNION::computeOperatorCost()\n");
    fprintf(pfp,"myRowCount=%g\n",myRowCount_.value());
    cvFR.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  Cost *costPtr = new STMTHEAP
                         Cost (&cvFR,&cvLR,NULL,cpuCount,fragmentsPerCPU);

#ifndef NDEBUG
  if ( printCost )
    {
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodMergeUnion::computeOperatorCostInternal().
//<pb>
//==============================================================================
//  Produce a final cumulative cost for an entire subtree rooted at a specified
// physical UNION operator.
//
// Input:
//  unionOp    -- specified physical union operator.
//
//  myContext  -- context associated with specified physical union operator
//
//  pws        -- plan work space associated with specified physical union
//                  operator.
//
//  planNumber -- used to get appropriate child contexts.
//
// Output:
//  none
//
// Return:
//  Pointer to cumulative final cost.
//
//==============================================================================
Cost*
CostMethodMergeUnion::computePlanCost( RelExpr* unionOp,
                                       const Context* myContext,
                                       const PlanWorkSpace* pws,
                                       Lng32 planNumber
                                     )
{

  //----------------------------------------------------------------------
  //  For now, UNIONs use a generic roll-up strategy for binary operators.
  //----------------------------------------------------------------------
  return rollUpForBinaryOp(unionOp, myContext, pws, planNumber);

} // CostMethodMergeUnion::computePlanCost()
//<pb>
//==============================================================================
//  Merge cumulative costs of both children of UNION operator when neither child
// has a blocking operator anywhere within its subtree.
//
// Input:
//  leftChildCost  -- pointer to cumulative cost of left child.
//
//  rightChildCost -- pointer to cumulative cost of right child.
//
//  rpp            -- Parent's required physical properties needed by lower
//                     level routines.
//
// Output:
//  none
//
// Return:
//  Pointer to merged cost of both children.
//
//==============================================================================
Cost*
CostMethodMergeUnion::mergeNoLegsBlocking( const CostPtr leftChildCost,
                                           const CostPtr rightChildCost,
                                           const ReqdPhysicalProperty* const rpp
                                         )
{

  //-------------------------------------
  //  Create merged cost initially empty.
  //-------------------------------------
  Cost* mergedCost = new STMTHEAP Cost();

  //-------------------------------------------------------------------------
  //  For total cost, simply accumulate all resource usage with simple vector
  // addition.
  //-------------------------------------------------------------------------
  mergedCost->totalCost() =  leftChildCost->getTotalCost()
                           + rightChildCost->getTotalCost();

  //---------------------------------------------------------------------------
  //  The first row cost is the cost of the faster child plus a measure of
  // interferance between the two children.  The quantity etMINOF(...)
  // represents the cost of the faster child. The quantity vecMINOF(...)
  // represents the interferance factor.
  //---------------------------------------------------------------------------
  mergedCost->cpfr() =  overlapAdd( etMINOF( leftChildCost->getCpfr(),
                                             rightChildCost->getCpfr(),
                                             rpp ),
                                    vecMINOF( leftChildCost->getCpfr(),
                                              rightChildCost->getCpfr() ) );

  //------------------------------------------------------------------------
  //  Last row cost requires both children to finish, so add both children's
  // cost using overlapped addition.
  //------------------------------------------------------------------------
  mergedCost->cplr() = overlapAdd( leftChildCost->getCplr(),
                                   rightChildCost->getCplr() );

  //-----------------------------------------------------------------
  //  Ensure that no component of merged first row vector exceeds the
  // corresponding component of merged last row vector.
  //-----------------------------------------------------------------
  mergedCost->cpfr().enforceUpperBound(mergedCost->cplr());

  //-------------------------------------------------------------------------
  //  Assuming that this UNION operator's process and its child processes all
  // run in separate CPUs, no interferance occurs, so the first row produced
  // by the UNION depends on the fastest of the two children and the last
  // row produced by the UNION depends on the slowest of the two children.
  //-------------------------------------------------------------------------
  //jo mergedCost->opfr() = etMINOF( leftChildCost->getOpfr(),
  //jo                               rightChildCost->getOpfr(),
  //jo                               rpp );

  //jo mergedCost->oplr() = etMAXOF( leftChildCost->getOplr(),
  //jo                               rightChildCost->getOplr(),
  //jo                              rpp );

  return mergedCost;

} // CostMethodMergeUnion::mergeNoLegsBlocking
//<pb>
//==============================================================================
//  Merge cumulative costs of both children of JOIN operator when both children
// have a blocking operator somewhere within their respective subtrees.
//
// Note:  As a side effect, the right Child's blocking vectors will be
//       normalized to the number of probes for the left child.
//
// Input:
//  leftChildCost  -- pointer to cumulative cost of left child.
//
//  rightChildCost -- pointer to cumulative cost of right child.
//
//  rpp            -- Parent's required physical properties needed by lower
//                     level routines.
//
// Output:
//  none
//
// Return:
//  Pointer to merged cost of both children.
//
//==============================================================================
Cost*
CostMethodMergeUnion::mergeBothLegsBlocking(
                                          const CostPtr leftChildCost,
                                          const CostPtr rightChildCost,
                                          const ReqdPhysicalProperty* const rpp
                                           )
{

  Cost *mergedCost = new STMTHEAP Cost();

  //-------------------------------------------------------------------------
  //  For total cost, simply accumulate all resource usage with simple vector
  // addition.
  //-------------------------------------------------------------------------
  mergedCost->totalCost() =  leftChildCost->getTotalCost()
                           + rightChildCost->getTotalCost();

  //---------------------------------------------------------------------------
  //  The first row cost is the cost of the faster child plus a measure of
  // interferance between the two children.  The quantity etMINOF(...)
  // represents the cost of the faster child. The quantity vecMINOF(...)
  // represents the interferance factor.
  //---------------------------------------------------------------------------
  mergedCost->cpfr() =  overlapAdd( etMINOF( leftChildCost->getCpfr(),
                                             rightChildCost->getCpfr(),
                                             rpp ),
                                    vecMINOF( leftChildCost->getCpfr(),
                                              rightChildCost->getCpfr() ) );

  //---------------------------------------------------------------------
  //  Normalize right child's blocking vector's to left child's number of
  // probes.
  //---------------------------------------------------------------------
  rightChildCost->cpbcTotal().normalize(
                                    leftChildCost->getCpbcTotal().getNumProbes()
                                       );
  rightChildCost->cpbc1().normalize( leftChildCost->getCpbc1().getNumProbes() );

  //-------------------------------------------------------------------------
  //  Last row cost calculation changes slightly depending on which child has
  // slower blocking activity.
  //-------------------------------------------------------------------------
  CostScalar leftChildCpbcTotalET =
    leftChildCost->getCpbcTotal().getElapsedTime(rpp);
  CostScalar rightChildCpbcTotalET =
    rightChildCost->getCpbcTotal().getElapsedTime(rpp);

  const CostScalar & leftChildCpbcTotalNumProbes =
    leftChildCost->getCpbcTotal().getNumProbes();
  const CostScalar & rightChildCpbcTotalNumProbes =
    rightChildCost->getCpbcTotal().getNumProbes();

  if ( leftChildCpbcTotalET <= rightChildCpbcTotalET )
    {

      //---------------------------------------------------------------------
      //  Right child has slower blocking activity.  Calculate a ratio which
      // represents the percentage of the left child's last row activity that
      // overlaps with the right child's blocking activity.
      //---------------------------------------------------------------------
      const CostScalar leftChildLastRowElapsedTime =
        leftChildCost->getCplr().getElapsedTime(rpp); // div-by-zero fix

      // if leftChildLastRowElapsedTime is zero (for whatever reason ... this is
      // suspicious ... ), set the ratio to be one -- don't divide by zero!
      const CostScalar overlapRatio =
	(leftChildLastRowElapsedTime.isZero())
	? csOne
        : MINOF( csOne,
		 (   ( rightChildCpbcTotalET * rightChildCpbcTotalNumProbes )
                   - ( leftChildCpbcTotalET * leftChildCpbcTotalNumProbes )
                 ) / leftChildLastRowElapsedTime
               );

      //-------------------------------------------------------------------
      //  Produce a vector which represents the portion of the left child's
      // last row activity that does not overlap with the right child's
      // blocking activity.
      //-------------------------------------------------------------------
      SimpleCostVector nonOverlapLeft = leftChildCost->getCplr();
      nonOverlapLeft.scaleByValue( csOne - overlapRatio );

      //----------------------------------------------------------------------
      //  Multiply right child's blocking vector by its number of probes so it
      // represents cumulative activity over all probes and is thus
      // commensurate with last row activity.
      //----------------------------------------------------------------------
      SimpleCostVector rightBlockingAllProbes =
	rightChildCost->getCpbcTotal() * rightChildCpbcTotalNumProbes;

      //------------------------------------------------------------------------
      //  Produce a vector which represents the portion of the left child's last
      // row activiity that overlaps with the right child's blocking activity.
      // Reduce this vector to the extent by which it actually overlaps.
      //------------------------------------------------------------------------
      SimpleCostVector overlapLeft = leftChildCost->getCplr();
      overlapLeft = overlapAdd(overlapLeft.scaleByValue(overlapRatio),
                               rightBlockingAllProbes)
                     - rightBlockingAllProbes;

      //-----------------------------------------------------------------------
      //  Since both children of a UNION act independently, merge the last row
      // vectors of both children using overlapped addition.  A portion of the
      // left child's last row activity has been reduced to the extent that it
      // can overlap with the right child's blocking activity.
      //-----------------------------------------------------------------------
      mergedCost->cplr() = overlapAdd(rightChildCost->getCplr(),
                                      nonOverlapLeft + overlapLeft);
    }
  else
    {

      //----------------------------------------------------------------------
      //  Left child has slower blocking activity.  Calculate a ratio which
      // represents the percentage of the right child's last row activity that
      // overlaps with the left child's blocking activity.
      //----------------------------------------------------------------------
      const CostScalar rightChildLastRowElapsedTime =
        rightChildCost->getCplr().getElapsedTime(rpp); // div-by-zero fix

      // if rightChildLastRowElapsedTime is zero (for whatever reason ... this is
      // suspicious ... ), set the ratio to be one -- don't divide by zero!
      const CostScalar overlapRatio =
	( rightChildLastRowElapsedTime.isZero() )
	? csOne
	: MINOF( csOne,
		 (   ( leftChildCpbcTotalET * leftChildCpbcTotalNumProbes )
		   - ( rightChildCpbcTotalET * rightChildCpbcTotalNumProbes )
                 ) / rightChildLastRowElapsedTime
               );

      //--------------------------------------------------------------
      //  Produce a vector which represents the portion of the right
      // child's last row activity that does not overlap with the left
      // child's blocking activity.
      //--------------------------------------------------------------
      SimpleCostVector nonOverlapRight = rightChildCost->getCplr();
      nonOverlapRight.scaleByValue( csOne - overlapRatio );

      //---------------------------------------------------------------------
      //  Multiply left child's blocking vector by its number of probes so it
      // represents cumulative activity over all probes and is thus
      // commensurate with last row activity.
      //---------------------------------------------------------------------
      SimpleCostVector leftBlockingAllProbes =
	leftChildCost->getCpbcTotal() * leftChildCpbcTotalNumProbes;

      //--------------------------------------------------------------------
      //  Produce a vector which represents the portion of the right child's
      // last row activiity that overlaps with the left child's blocking
      // activity.  Reduce this vector to the extent by which it actually
      // overlaps.
      //--------------------------------------------------------------------
      SimpleCostVector overlapRight = rightChildCost->getCplr();
      overlapRight = overlapAdd(overlapRight.scaleByValue(overlapRatio),
                                leftBlockingAllProbes)
                      - leftBlockingAllProbes;

      //-----------------------------------------------------------------------
      //  Since both children of a UNION act independently, merge the last row
      // vectors of both children using overlapped addition.  A portion of the
      // right child's last row activity has been reduced to the extent that it
      // can overlap with the left child's blocking activity.
      //-----------------------------------------------------------------------
      mergedCost->cplr() = overlapAdd(leftChildCost->getCplr(),
                                      nonOverlapRight + overlapRight);

    }

  //-----------------------------------------------------------------
  //  Ensure that no component of merged first row vector exceeds the
  // corresponding component of merged last row vector.
  //-----------------------------------------------------------------
  mergedCost->cpfr().enforceUpperBound(mergedCost->cplr());

  //--------------------------------------------------------------------------
  //  Since both children of a UNION act independently, merge children's total
  // blocking vectors using overlapped addition.
  //--------------------------------------------------------------------------
  mergedCost->cpbcTotal()
                      = overlapAdd( leftChildCost->getCpbcTotal(),
                                    rightChildCost->getCpbcTotal() );

  //---------------------------------------------------------------------------
  //  For blocking vectors, we use the same basic formula:  the cost of the
  // faster child plus an interferance factor.  The quantity etMINOF(...)
  // represents the cost of the faster child. The quantity vecMINOF(...)
  // represents the interferance factor.
  //---------------------------------------------------------------------------
  mergedCost->cpbc1() = overlapAdd( etMINOF( leftChildCost->getCpbc1(),
                                             rightChildCost->getCpbc1(),
                                             rpp ),
                                    vecMINOF( leftChildCost->getCpbc1(),
                                              rightChildCost->getCpbc1() ) );

  //-------------------------------------------------------------------------
  //  Assuming that this UNION operator's process and its child processes all
  // run in separate CPUs, no interferance occurs, so the first row produced
  // by the UNION depends on the fastest of the two children and the last
  // row produced by the UNION depends on the slowest of the two children.
  //-------------------------------------------------------------------------
  //jo mergedCost->opfr() = etMINOF( leftChildCost->getOpfr(),
  //jo                               rightChildCost->getOpfr(),
  //jo                               rpp );

  //jo mergedCost->oplr() = etMAXOF( leftChildCost->getOplr(),
  //jo                               rightChildCost->getOplr(),
  //jo                               rpp );

  return mergedCost;

} // CostMethodMergeUnion::mergeBothLegsBlocking
//<pb>

// ----QUICKSEARCH FOR ROOT...............................................

/**********************************************************************/
/*                                                                    */
/*                          CostMethodRelRoot                         */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodRelRoot::computeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodRelRoot::computeOperatorCostInternal(RelExpr* op,
                                               const Context* myContext,
                                               Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);

  // -------------------------------------------------------------
  // Save off estimated degree of parallelism.  Always 1 for root.
  // -------------------------------------------------------------
  countOfStreams = 1;

  // ---------------------------------------------------------------------
  // A RelRoot performs no actual functions on the rows it get than just
  // copying them to the application. The operator receives one stream of
  // rows. In parallel plans, the Exchange below RelRoot collects rows
  // from all streams, and send them to one single instance of RelRoot.
  // ---------------------------------------------------------------------

  CostScalar cpuCopyRow =
                CostPrimitives::cpuCostForCopyRow(myVis().getRowLength());
  CostScalar cpuFR = cpuCopyRow;
  CostScalar cpuLR = cpuCopyRow * myRowCount_;

  CostScalar readMetadataOpenFirstPartitionCpu=
    CostPrimitives::getBasicCostFactor(CPUCOST_SUBSET_OPEN)*
    op->getGroupAttr()->getNumBaseTables()*
    CostPrimitives::getBasicCostFactor(MSCF_ET_CPU);

  //fudge factor for cputime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  // ---------------------------------------------------------------------
  // Synthesize the cost vectors.
  // ---------------------------------------------------------------------
  SimpleCostVector cvFR (
    cpuFR * ff_cpu,
    csZero,
    csZero,
    readMetadataOpenFirstPartitionCpu,
    csOne
    );

  SimpleCostVector cvLR (
    cpuLR * ff_cpu,
    csZero,
    csZero,
    readMetadataOpenFirstPartitionCpu,
    csOne
    );

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"RELROOT::computeOperatorCost()\n");
    fprintf(pfp,"childRowCount=%g\n",myRowCount_.value());
    cvFR.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return the cost object.
  // ---------------------------------------------------------------------

  Cost *costPtr = new STMTHEAP Cost (&cvFR,&cvLR,NULL,1,1);

#ifndef NDEBUG
  if ( printCost )
    {
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodRelRoot::computeOperatorCostInternal().
//<pb>

// ----QUICKSEARCH FOR TUPLE..............................................

/**********************************************************************/
/*                                                                    */
/*                          CostMethodTuple                           */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodTuple::computeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodTuple::computeOperatorCostInternal(RelExpr* op,
                                             const Context* myContext,
                                             Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  Tuple* tp = (Tuple*) op;

  // ---------------------------------------------------------------------
  // Cost includes allocating the tuple and then evaluating the column
  // expressions in the tuple.
  // ---------------------------------------------------------------------
  CostScalar cpuCostEvalExpr = csZero;
  for(Lng32 i = 0; i < Lng32(tp->tupleExpr().entries()); i++)
     cpuCostEvalExpr +=
                   CostPrimitives::cpuCostForEvalExpr(tp->tupleExpr()[i]);

  CostScalar cpuFR = cpuCostAllocateTuple_ + cpuCostEvalExpr;

  // ---------------------------------------------------------------------
  // The Tuple operator returns exactly one row for each probe it gets.
  // Thus, its total row count should just be probeCount.
  // ---------------------------------------------------------------------
  CostScalar cpuLR = cpuFR * noOfProbesPerStream_;

  // Tuple operator process number of tuples in the IN list, and selectivity
  // is 50% if there is a predicate otherwise it is a cross product.
  // So, the cost should reflect this work otherwise we get many NJ plans
  // with tupleList on the RHS. If TupleList is under NJ and COMP_INT_80 = 1,
  // include total rowcount in the cost. Part of the fix is also in HGBY cost.
  // Current value of myRowCount_ = numTuples * probes, so we
  // divide this value by countOfStreams to compute the cost per ESP.
  // If COMP_INT_80  = 0 means fix is OFF.
  //                 > 0 means fix is ON. Default value is 3, so fix is ON.
  Lng32 compInt80 = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_80);
  if ( (compInt80 > 0 ) AND isUnderNestedJoin_)
  {
    cpuLR = cpuFR * ((myRowCount_ / countOfStreams).minCsOne());
    // scale up by CPUCOST_NJ_TUPLST_FF to avoid TupList under NJ.
    cpuLR = cpuLR * CostPrimitives::getBasicCostFactor(CPUCOST_NJ_TUPLST_FF);
  }

  //fudge factor for cputime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  // ---------------------------------------------------------------------
  // Synthesize the cost vectors.
  // ---------------------------------------------------------------------
  const SimpleCostVector cvFR (
    cpuFR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  const SimpleCostVector cvLR (
    cpuLR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"TUPLE::computeOperatorCost()\n");
    cvFR.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  Cost *costPtr = new STMTHEAP Cost (&cvFR,&cvLR,NULL,cpuCount,fragmentsPerCPU);

#ifndef NDEBUG
  if ( printCost )
    {
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodTuple::computeOperatorCostInternal().
// LCOV_EXCL_STOP
//<pb>

/**********************************************************************/
/*                                                                    */
/*                          CostMethodTranspose                       */
/*                                                                    */
/**********************************************************************/

// Compute common costing parameters.
//
void
CostMethodTranspose::cacheParameters(RelExpr *op,
                                     const Context *myContext)
{
  // Just to make sure things are working as expected
  //
  CMPASSERT(op->getOperatorType() == REL_TRANSPOSE);

  CostMethod::cacheParameters(op,myContext);

  // We know at this point that the op is a Physical Transpose node.
  //
  PhysTranspose *transpose = (PhysTranspose *)op;

  // The set of values that the transpose operator
  // will move for to produce one row.
  //
  ValueIdSet moveValues;

  for(CollIndex v = 0; v < transpose->transUnionVectorSize(); v++) {
    const ValueIdList &valIdList = transpose->transUnionVector()[v];

    for(CollIndex vidu = 0; vidu < valIdList.entries(); vidu++) {

      const ValueIdUnion *valIdUnion =
        (ValueIdUnion *)valIdList[vidu].getValueDesc()->getItemExpr();

      moveValues += valIdUnion->getSource(0);
    }
  }


  // The estimated cost to produce one row.
  //
  CostScalar cpuCostToProduceOneRow =
    CostPrimitives::cpuCostForCopySet(moveValues) +
    CostPrimitives::getBasicCostFactor(EX_OP_ALLOCATE_TUPLE);

  // Estimated cost to produce all rows.
  //
  cpuCostToProduceAllRows_ = myRowCount_ * cpuCostToProduceOneRow;

}

// CostMethodTranspose::computeOperatorCostInternal() -------------------------
// Compute the cost of this Transpose node given the optimization context.
//
// Parameters
//
// RelExpr *op
//  IN - The PhysTranpose node which is being costed.
//
// Context *myContext
//  IN - The optimization context within which to cost this node.
//
// long& countOfStreams
//  OUT - Estimated degree of parallelism for returned preliminary cost.
//
// LCOV_EXCL_START :cnu -- OCM code
Cost *
CostMethodTranspose::computeOperatorCostInternal(RelExpr *op,
					         const Context *myContext,
                                                 Lng32& countOfStreams)
{
  // Just to make sure things are working as expected
  //
  CMPASSERT(op->getOperatorType() == REL_TRANSPOSE);

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  CostScalar cpuCostToProduceLastRow =
    cpuCostToProduceAllRows_ / countOfStreams_;

  CostScalar cpuCostToProduceFirstRow =
    cpuCostToProduceLastRow / myRowCount_ / noOfProbes_;

  //fudge factor for cputime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  const SimpleCostVector cvFirstRow(
    cpuCostToProduceFirstRow * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  const SimpleCostVector cvLastRow(
    cpuCostToProduceLastRow * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  return new STMTHEAP
    Cost(&cvFirstRow, &cvLastRow, NULL, cpuCount, fragmentsPerCPU);


}      // CostMethodTranspose::computeOperatorCostInternal()
// LCOV_EXCL_STOP

/**********************************************************************/
/*                                                                    */
/*                     CostMethodCompoundStmt                         */
/*                                                                    */
/**********************************************************************/
// Compute common costing parameters.
//
void
CostMethodCompoundStmt::cacheParameters(RelExpr *op,
                                     const Context *myContext)
{
  // Just to make sure things are working as expected
  //
  CMPASSERT(op->getOperatorType() == REL_COMPOUND_STMT);

  CostMethod::cacheParameters(op,myContext);

  // The estimated cost to produce one row.
  // It is treated as a constant.
  CostScalar cpuCostToProduceOneRow =
    CostPrimitives::getBasicCostFactor(EX_OP_ALLOCATE_TUPLE);

  cpuCostToProduceAllRows_ = myRowCount_ * cpuCostToProduceOneRow;

} // CostMethodCompoundStmt::cacheParameters()

//-------------------------------------------------------------------------
// CostMethodCompoundStmt::computeOperatorCostInternal()
// Compute the cost of this Compound Statement node, given the optimization context.
//
// Parameters
//
// RelExpr *op
//  IN - The PhysTranpose node which is being costed.
//
// Context *myContext
//  IN - The optimization context within which to cost this node.
//-------------------------------------------------------------------------
// LCOV_EXCL_START :cnu -- OCM code
Cost *
CostMethodCompoundStmt::computeOperatorCostInternal(RelExpr *op,
                                                    const Context *myContext,
                                                    Lng32& countOfStreams)
{
  // Just to make sure things are working as expected
  //
  CMPASSERT(op->getOperatorType() == REL_COMPOUND_STMT);

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  countOfStreams = countOfStreams_;

  CostScalar cpuLR =
    cpuCostToProduceAllRows_ / countOfStreams_;

  CostScalar cpuFR =
    cpuLR / myRowCount_ / noOfProbes_;

  //fudge factor for cputime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  const SimpleCostVector cvFirstRow(
    cpuFR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  const SimpleCostVector cvLastRow(
    cpuLR * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  return new STMTHEAP
    Cost(&cvFirstRow, &cvLastRow, NULL, cpuCount, fragmentsPerCPU);

}      // CostMethodTranspose::computePreliminaryCost()

/**********************************************************************/
/*                                                                    */
/*                          CostMethodStoredProc                      */
/*                                                                    */
/**********************************************************************/

// CostMethodStoredProc::computeOperatorCostInternal() -----------------------
// Compute the cost of this Stored Procedure node given the optimization
// context.
//
//
// Parameters
//
// RelExpr *op
//  IN - The PhysTranpose node which is being costed.
//
// Context *myContext
//  IN - The optimization context within which to cost this node.
//
// long& countOfStreams
//  OUT - Estimated degree of parallelism for returned preliminary cost.
//
Cost *
CostMethodStoredProc::computeOperatorCostInternal(RelExpr *op,
					          const Context *myContext,
                                                  Lng32& countOfStreams)
{
  // Completely ficticious cost for now of 1000 CPU instructions and
  // no I/O or message cost.
  //
  const SimpleCostVector cv (
    csOne * CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions(),
    csZero,
    csZero,
    csZero,
    csOne
    );

  //  Stored procedures never run in parallel.
  //
  countOfStreams = 1;

  return new STMTHEAP Cost( &cv, &cv, NULL, 1, 1 );

}      // CostMethodStoredProc::computeOperatorCostInternal()
/**********************************************************************/
/*                                                                    */
/*                          CostMethodTableMappingUDF                 */
/*                                                                    */
/**********************************************************************/

// CostMethodTableMappingUDF::computeOperatorCostInternal() ----------
// Compute the cost of this Stored Procedure node given the optimization
// context.
//
//
// Parameters
//
// RelExpr *op
//  IN - The TableMappingUDF node which is being costed.
//
// Context *myContext
//  IN - The optimization context within which to cost this node.
//
// long& countOfStreams
//  OUT - Estimated degree of parallelism for returned preliminary cost.
//
Cost *
CostMethodTableMappingUDF::computeOperatorCostInternal(RelExpr* op,
                                                const Context* myContext,
                                                Lng32& countOfStreams)
{
  CostScalar cpuTimeForFirstRow;
  CostScalar cpuTimeForLastRow;

  EstLogPropSharedPtr inputLP  = myContext->getInputLogProp();
  EstLogPropSharedPtr outputLP = op->getGroupAttr()->outputLogProp(inputLP);
  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // Save off estimated degree of parallelism.
  countOfStreams = countOfStreams_;

  
  /**** May not be needed for now *****************

  // can mapReduceOp be BMO?
  // add CQD which tells complexity of UDF.
  // MAPREDUCE_UDR_COMPLEXITY : 1 means Small, 2 means Medium, 3 means Large
  CostScalar mapRedUdfComplxity = defs.getAsLong(MAPREDUCE_UDR_COMPLEXITY);
  CostScalar costAdj = 1;
  if (mapRedUdfComplxity == 2)
    costAdj = 5; // multiply total cost 5 times for medium UDF
  else if (mapRedUdfComplxity == 3)
    costAdj = 10; // multiply total cost 10 times for Large UDF


 **************************************************/

  // Size of a record in kilobytes. Let's not use this one for now. Enable it if required.
  //double recordSize = op->child(0).getGroupAttr()->getRecordLength() / 1024.0;
  // per stream Rows from child 
  CostScalar  rowsFromChildPerStream ;
  if (op->getArity() == 1)
  {
    EstLogPropSharedPtr childOutputLP = op->child(0).outputLogProp( inputLP );
    rowsFromChildPerStream = childOutputLP->getResultCardinality() / countOfStreams;
  }
  else
  { 
    // when TMUDF is the leaf node we do not know its cardinality
    // (till a discovery method is added). Till then use a value
    // that will encourage parallel plan creation.
    rowsFromChildPerStream = 100000/ countOfStreams;
  }
  
 
  // get cpu FF
  const double ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();
  const double udfCpuCost = defs.getAsDouble(COMP_FLOAT_9) ; // assumption, will get overridden by dll
  
  
  // Add default cost: cost_per_byte * num_rows * cpu_ff.

  // First row cost includes cost for 1 row.
  cpuTimeForFirstRow = udfCpuCost * ff_cpu;
  // cpuTimeForFirstRow *= costAdj;

  // For last row, all probes coming from above must be included as well:
  cpuTimeForLastRow  = udfCpuCost*ff_cpu; 
  cpuTimeForLastRow *= rowsFromChildPerStream*noOfProbesPerStream_ ;
  // cpuTimeForLastRow *= costAdj;

  

  SimpleCostVector cvFR(
    cpuTimeForFirstRow,
    csZero,                      // no IO time
    csZero,                      // no message time
    csZero,                      // no idle time
    noOfProbesPerStream_);       // num. of probes

  SimpleCostVector cvLR(
    cpuTimeForLastRow,
    csZero,                      // no IO time
    csZero,                      // no message time
    csZero,                      // no idle usage
    noOfProbesPerStream_);

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  return new STMTHEAP Cost( &cvFR,
                            &cvLR,
                            NULL,
                            countOfStreams_,
                            fragmentsPerCPU
                          );
}
// LCOV_EXCL_STOP

/**********************************************************************/
/*                                                                    */
/*                          CostMethodFastExtract                     */
/*                                                                    */
/**********************************************************************/

// CostMethodFastExtract::computeOperatorCostInternal() ----------
// Compute the cost of this Fast Extract node given the optimization
// context.
//
//
// Parameters
//
// RelExpr *op
//  IN - The FastExtract node which is being costed.
//
// Context *myContext
//  IN - The optimization context within which to cost this node.
//
// long& countOfStreams
//  OUT - Estimated degree of parallelism for returned preliminary cost.
//
Cost *
CostMethodFastExtract::computeOperatorCostInternal(RelExpr* op,
                                                const Context* myContext,
                                                Lng32& countOfStreams)
{
  CostScalar cpuTimeForFirstRow;
  CostScalar cpuTimeForLastRow;

  EstLogPropSharedPtr inputLP  = myContext->getInputLogProp();
  EstLogPropSharedPtr outputLP = op->getGroupAttr()->outputLogProp(inputLP);
  NADefaults &defs = ActiveSchemaDB()->getDefaults();

  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // Save off estimated degree of parallelism.
  countOfStreams = countOfStreams_;

  // per stream Rows from child
  CostScalar  rowsFromChildPerStream ;
  EstLogPropSharedPtr childOutputLP = op->child(0).outputLogProp( inputLP );
  rowsFromChildPerStream = childOutputLP->getResultCardinality() / countOfStreams;



  // get cpu FF
  const double ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();


  // First row cost includes cost for 1 row.
  cpuTimeForFirstRow = csOne * ff_cpu;

  // For last row, all probes coming from above must be included as well:
  cpuTimeForLastRow  = csOne*ff_cpu;
  cpuTimeForLastRow *= rowsFromChildPerStream*noOfProbesPerStream_ ;



  SimpleCostVector cvFR(
    cpuTimeForFirstRow,
    csZero,                      // no IO time
    csZero,                      // no message time
    csZero,                      // no idle time
    noOfProbesPerStream_);       // num. of probes

  SimpleCostVector cvLR(
    cpuTimeForLastRow,
    csZero,                      // no IO time
    csZero,                      // no message time
    csZero,                      // no idle usage
    noOfProbesPerStream_);




  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  return new STMTHEAP Cost( &cvFR,
                            &cvLR,
                            NULL,
                            countOfStreams_,
                            fragmentsPerCPU
                          );
}

/**********************************************************************/
// Begin cost methods for WRITE operations
/**********************************************************************/

//<pb>
// ----QUICKSEARCH FOR HbaseInsert........................................

/**********************************************************************/
/*                                                                    */
/*                      CostMethodHbaseInsert                         */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodHbaseInsert::cacheParameters()
// -----------------------------------------------------------------------
void CostMethodHbaseInsert::cacheParameters(RelExpr* op, const Context * myContext)
{
  CostMethod::cacheParameters(op, myContext);

  HbaseInsert* insOp = (HbaseInsert *)op;

  CMPASSERT(partFunc_ != NULL);
  NodeMap * nodeMap = (NodeMap *)partFunc_->getNodeMap();
  if (nodeMap)
    activePartitions_ = (CostScalar)nodeMap->getNumActivePartitions();
  else
    // Occasionally (e.g., regress/fullstack2/test023, the insert/select
    // from t023t1 into t023t2 using a transpose operator), we get
    // a ReplicateNoBroadcastPartitioningFunction lacking a node map.
    // In this case we'll just use the number of partitions from the
    // partitioning function itself -- which is probably an ESP count.
    activePartitions_ = (CostScalar)partFunc_->getCountOfPartitions();

  // The number of asynchronous streams is USUALLY the # of active parts.
  countOfAsynchronousStreams_ = activePartitions_;
} // CostMethodHbaseInsert::cacheParameters()



// -----------------------------------------------------------------------
// CostMethodHbaseInsert::computeOperatorCostInternal()
// -----------------------------------------------------------------------
Cost* CostMethodHbaseInsert::computeOperatorCostInternal(RelExpr* op,
  const Context* myContext,
  Lng32& countOfStreams)
{
  cacheParameters(op, myContext);
  estimateDegreeOfParallelism();

  // ------------------------------------------------------
  // Save off our current estimated degree of parallelism.
  // in the 'out' parameter; we might revise it below
  // ------------------------------------------------------
  countOfStreams = countOfStreams_;

  CostScalar currentCpus =
    (CostScalar)myContext->getPlan()->getPhysicalProperty()->getCurrentCountOfCPUs();
  activeCpus_ = MINOF(countOfAsynchronousStreams_, currentCpus);

  // update count of streams; the caller of the method uses this value
  if ((countOfAsynchronousStreams_ > 0) &&
      (countOfAsynchronousStreams_ < countOfStreams)
      )
    countOfStreams = (Lng32)countOfAsynchronousStreams_.getValue();


  streamsPerCpu_ =
    (countOfAsynchronousStreams_ / activeCpus_).getCeiling();

  CostScalar noOfProbesPerStream(csOne);

  // Determine the number of probes per stream. Use this number as
  // the number of rows to insert (this is "per-stream" costing).

  noOfProbesPerStream =
    (noOfProbes_ / countOfAsynchronousStreams_).minCsOne();

  // ************************************************************
  // Compute the write/read cost for the insert
  //
  // ************************************************************

  // ---------------------------------------------------------------------
  // Synthesize the cost vectors.
  // ---------------------------------------------------------------------
  SimpleCostVector cvFR;
  SimpleCostVector cvLR;

  // For now, we don't bother to estimate CPU time, I/O time, transfer 
  // time or idle time, since we really are only supporting the new 
  // cost model.
  //
  // Future possible improvements:
  //
  // 1. Take into account HBase memstore insertion cost. The memstore
  // uses a Red-Black tree which has o(n * log(n)) insertion cost. To
  // model this correctly, we'd need to take into account the number
  // of HBase regions rather than the number of ESPs, that is, to
  // divide the number of probes by the number of HBase regions to find
  // n. This cost will be paid no matter how many inserting streams
  // there are so by itself this may not be interesting. It would only
  // be interesting if there were a choice in the plan between inserting
  // and not inserting (e.g. if we were considering bypassing the
  // memstore, or if we were considering storing an intermediate result,
  // neither of which are choices we examine today).
  //
  // 2. Take into account whether the probes are in key order. There
  // is anecdotal evidence that if the probes are in key order, then
  // memstore insertion cost is less. Possibly this is true only if
  // inserting at the end or the beginning of the key range in a 
  // partition; intuitively inserting in the middle would seem to incur
  // the full insertion cost. This is worthwhile taking into 
  // consideration as it opens the possibility of choosing between a
  // plan that sorts rows in Trafodion before passing them to HBase
  // vs. a plan that does not. To make this calculation we must know
  // the memstore insertion cost (point 1 above), the order of the 
  // probes, whether the ESPs are aligned to the Regions of the 
  // target table, and whether we are inserting at the beginning or
  // end of the key range. A first approximation to the last item
  // would be whether the target table is empty. This is interesting
  // because the case that we are doing an INSERT/SELECT into a new
  // table is likely to be common.
  //
  // 3. Take into account memstore flush cost. We could add I/O time
  // for flushes. For example, we could compare the number of probes
  // per HBase Region with the number of rows that would cause a
  // flush (the latter can be obtained from 
  // HbaseClient::estimateMemStoreRows() and is a function of the
  // HBase parameter hbase.hregion.memstore.flush.size). Again, this
  // cost will be paid no matter the plan choice so this is not
  // interesting today. As with point 1, it becomes interesting only
  // if there is a plan choice between inserting via memstore or not.
  //
  // In the interest of time, we move forward without these 
  // improvements for now.

  cvFR.setNumProbes(noOfProbesPerStream);
  cvLR.setNumProbes(noOfProbesPerStream);

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  Cost *costPtr = new STMTHEAP
    Cost(&cvFR
         , &cvLR
         , NULL
         , Lng32(activeCpus_.getValue())
         , Lng32(streamsPerCpu_.getValue())
         );

#ifndef NDEBUG
  if (CmpCommon::getDefault(OPTIMIZER_PRINT_COST) == DF_ON)
    {
      pfp = stdout;
      fprintf(pfp, "HbaseInsert elapsed time: ");
      fprintf(pfp, "%f", costPtr->
              convertToElapsedTime(
                                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp, "\n");
    }
#endif

  return costPtr;
} // CostMethodHbaseInsert::computeOperatorCostInternal

// -----------------------------------------------------------------------
// CostMethodHbaseInsert::cleanUp()
//
// The method cleans up cached parameters which need deallocation and
// should be called after a costing session is done.
// -----------------------------------------------------------------------
void CostMethodHbaseInsert::cleanUp()
{
  activePartitions_ = csOne;
  activeCpus_ = csOne;
  streamsPerCpu_ = csOne;
  countOfAsynchronousStreams_ = csOne;

  // Clean up fields in base class
  CostMethod::cleanUp();

}  // CostMethodHbaseInsert::cleanUp().

//<pb>



/**********************************************************************/
/*                                                                    */
/*                          CostMethodUnPackRows                      */
/*                                                                    */
/**********************************************************************/

// Compute common costing parameters.
//
void
CostMethodUnPackRows::cacheParameters(RelExpr *op,
                                      const Context *myContext)
{
  // Just to make sure things are working as expected
  //
  CMPASSERT(op->getOperatorType() == REL_UNPACKROWS);

  CostMethod::cacheParameters(op,myContext);

  // We know at this point that the op is a Physical UnPackRows node.
  //
  PhysUnPackRows *unPackRows = (PhysUnPackRows *)op;

  // The set of values that the unPackRows operator
  // will move for to produce one row.
  //
  ValueIdSet moveValues;

  moveValues.insertList(unPackRows->unPackExpr());

  // The estimated cost to produce one row.
  //
  CostScalar cpuCostToProduceOneRow =
    CostPrimitives::cpuCostForCopySet(moveValues) +
    CostPrimitives::getBasicCostFactor(EX_OP_ALLOCATE_TUPLE);

  // Estimated cost to produce all rows.
  //
  cpuCostToProduceAllRows_ = myRowCount_ * cpuCostToProduceOneRow;

}

// CostMethodUnPackRows::computeOperatorCostInternal() -------------------------
// Compute the cost of this UnPackRows node given the optimization context.
//
// Parameters
//
// RelExpr *op
//  IN - The PhysUnPackRows node which is being costed.
//
// Context *myContext
//  IN - The optimization context within which to cost this node.
//
// long& countOfStreams
//  OUT - Estimated degree of parallelism for returned preliminary cost.
//
// LCOV_EXCL_START :cnu -- OCM code
Cost *
CostMethodUnPackRows::computeOperatorCostInternal(RelExpr *op,
                                                  const Context *myContext,
                                                  Lng32& countOfStreams)
{
  // Just to make sure things are working as expected
  //
  CMPASSERT(op->getOperatorType() == REL_UNPACKROWS);

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  CostScalar cpuCostToProduceLastRow =
    cpuCostToProduceAllRows_ / countOfStreams_;

  CostScalar cpuCostToProduceFirstRow =
    cpuCostToProduceLastRow / myRowCount_ / noOfProbes_;

  //fudge factor for cpuTime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  const SimpleCostVector cvFirstRow(
    cpuCostToProduceFirstRow * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  const SimpleCostVector cvLastRow(
    cpuCostToProduceLastRow * ff_cpu,
    csZero,
    csZero,
    csZero,
    noOfProbesPerStream_
    );

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  return new STMTHEAP
    Cost(&cvFirstRow, &cvLastRow, NULL, cpuCount, fragmentsPerCPU);

}      // CostMethodUnPackRows::computeOperatorCostInternal()
// LCOV_EXCL_STOP

/**********************************************************************/
/*                                                                    */
/*                          CostMethodRelSequence                      */
/*                                                                    */
/**********************************************************************/
// Compute common costing parameters.
//
void
CostMethodRelSequence::cacheParameters(RelExpr *op,
                                       const Context *myContext)
{
  // Just to make sure things are working as expected
  //
  DCMPASSERT(op->getOperatorType() == REL_SEQUENCE);

  CostMethod::cacheParameters(op,myContext);

  // We know at this point that the op is a Physical RelSequence node.
  //
  const PhysSequence *relSequence = (PhysSequence *)op;

  // The set of values that the RelSequence operator
  // will move to produce one row.
  
  historyBufferWidthInBytes_ = relSequence->getEstHistoryRowLength(); //historyIds.getRowLength();

  const Lng32 numHistoryRows = MIN_ONE(relSequence->numHistoryRows());

  historyBufferSizeInBytes_ = numHistoryRows * historyBufferWidthInBytes_;

  // The estimated cost to produce one row.
  //
  CostScalar cpuCostToProduceOneRow =

    // The cost to compute the sequence functions.
    CostPrimitives::cpuCostForCopyRow(relSequence->
                                      sequenceFunctions().getRowLength()) +

    // The cost to copy history buffer row to result.
    CostPrimitives::cpuCostForCopyRow(historyBufferWidthInBytes_) +

    // The cost to allocate one tuple per row.
    CostPrimitives::getBasicCostFactor(EX_OP_ALLOCATE_TUPLE);

  // Estimated cost to produce all rows.
  //
  cpuCostToProduceAllRows_ = myRowCount_ * cpuCostToProduceOneRow;

}

// CostMethodRelSequence::computeOperatorCostInternal() -------------------------
// Compute the cost of this RelSequence node given the optimization context.
//
// Parameters
//
// RelExpr *op
//  IN - The PhysSequence node which is being costed.
//
// Context *myContext
//  IN - The optimization context within which to cost this node.
//
// long& countOfStreams
//  OUT - Estimated degree of parallelism for returned preliminary cost.
//
// LCOV_EXCL_START :cnu -- OCM code
Cost *
CostMethodRelSequence::computeOperatorCostInternal(RelExpr *op,
                                                   const Context *myContext,
                                                   Lng32& countOfStreams)
{
  // Just to make sure things are working as expected
  //
  DCMPASSERT(op->getOperatorType() == REL_SEQUENCE);

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // Cost scalars to be computed.
  // ---------------------------------------------------------------------

  CostScalar memFRInKB = historyBufferSizeInBytes_ / 1024.;
  CostScalar memLRInKB = memFRInKB;

  CostScalar cpuLR = cpuCostToProduceAllRows_ / countOfStreams_;

  CostScalar cpuFR = (cpuLR / myRowCount_) / noOfProbes_;

  CostScalar seekFR;
  CostScalar transferFRInKB;
  CostScalar seekLR;
  CostScalar transferLRInKB;

  // Now, consider the possibility of page faults.
  if (isBMO_ AND memFRInKB > csZero AND memFRInKB > memoryLimit_) {

    double pageSizeInKB =
      CostPrimitives::getBasicCostFactor(DEF_PAGE_SIZE);

    // Assume there is always one page fault to get first row.
    //
    seekFR = csOne;
    transferFRInKB = seekFR * pageSizeInKB;

    Lng32 numHistoryRowsPerPage =
      (Lng32)((pageSizeInKB * 1024)) / historyBufferWidthInBytes_;

    // Since the history buffer does not fit in memory and since it is
    // accessed in a circular fashion, there will be a page fault for
    // every page of rows added to the history buffer.
    //
    seekLR = myRowCount_ / numHistoryRowsPerPage;

    // Also, there is a chance that the evaluation of the sequence
    // functions will access a row of the history buffer which is not
    // in memory causing more page faults.
    //
    // This is the probability of page faults assuming random access to
    // the history buffer.
    //
    CostScalar probOfPageFaults = (memFRInKB - memoryLimit_) / memFRInKB;

    // But the access will tend to be local, not random, so adjust the
    // probability.
    //
    probOfPageFaults = probOfPageFaults * probOfPageFaults;

    seekLR += myRowCount_ * probOfPageFaults;
    transferLRInKB = seekLR * pageSizeInKB;

    seekLR = seekLR / countOfStreams_;
    transferLRInKB = transferLRInKB / countOfStreams_;

  }

    //fudge factor for cpuTime, ioSeeks & ioTransfer
    const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();
    const CostScalar ff_seeks = CURRSTMT_OPTDEFAULTS->getTimePerSeek();
    const CostScalar ff_seqIO = CURRSTMT_OPTDEFAULTS->getTimePerSeqKb();

  // CPUTime, IOTime= SeekTime + Transfer Time, no messages, never idle
  // num of probes are the five parameters passed.

  const SimpleCostVector
    cvFR(cpuFR * ff_cpu,				// CPU Time.
         seekFR * ff_seeks + transferFRInKB * ff_seqIO,	// IOTime
         csZero,					// no messages
         csZero,					// never Idle
         noOfProbesPerStream_);				// num probes

  const SimpleCostVector
    cvLR(cpuLR	* ff_cpu,				// CPU Time.
         seekLR * ff_seeks + transferLRInKB * ff_seqIO,	// IOTime.
         csZero,					// no messages
         csZero,					// never Idle
         noOfProbesPerStream_);				// num probes

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  return new STMTHEAP Cost(&cvFR, &cvLR, NULL, cpuCount, fragmentsPerCPU);

}      // CostMethodRelSequence::computeOperatorCostInternal()


/**********************************************************************/
/*                                                                    */
/*                          CostMethodSample                          */
/*                                                                    */
/**********************************************************************/
// Compute common costing parameters.
//
// CostMethodSample::computeOperatorCostInternal() ---------------------
// Compute the cost of this Sample node given the optimization context.
//
// Parameters
//
// RelExpr *op
//  IN - The PhysSample node which is being costed.
//
// Context *myContext
//  IN - The optimization context within which to cost this node.
//
// long& countOfStreams
//  OUT - Estimated degree of parallelism for returned preliminary cost.
//
Cost *
CostMethodSample::computeOperatorCostInternal(RelExpr *op,
                                              const Context *myContext,
                                              Lng32& countOfStreams)
{
  // Just to make sure things are working as expected
  //
  DCMPASSERT(op->getOperatorType() == REL_SAMPLE);

  // We know at this point that the op is a Physical RelSequence node.
  //
  PhysSample *relSample = (PhysSample *)op;

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  Lng32 numBalanceExpr = 0;

  DCMPASSERT(relSample->balanceExpr().entries() == 1);

  ValueId balanceRoot;

  relSample->balanceExpr().getFirst(balanceRoot);

  ItemExpr *balExpr = (ItmBalance *)balanceRoot.getItemExpr();

  while(balExpr) {
    numBalanceExpr++;
    balExpr = balExpr->child(2);
  }

  CostScalar cpuCostToProcessOneRow = numBalanceExpr *
    (CostPrimitives::getBasicCostFactor(CPUCOST_EVAL_SIMPLE_PREDICATE) +
     (2 * CostPrimitives::getBasicCostFactor(CPUCOST_EVAL_ARITH_OP)));

  EstLogPropSharedPtr inputLP = myContext->getInputLogProp();
  EstLogPropSharedPtr childOutputLP = op->child(0).outputLogProp( inputLP );
  const CostScalar & childNumRows = childOutputLP->getResultCardinality();

  cpuCostToProduceAllRows_ = cpuCostToProcessOneRow * childNumRows;

  CostScalar cpuLR = cpuCostToProduceAllRows_ / countOfStreams_;

  CostScalar cpuFR = (cpuLR / myRowCount_) / noOfProbes_;

  //fudge factor for cpuTime
  const CostScalar ff_cpu = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();

  const SimpleCostVector
    cvFR(cpuFR * ff_cpu,		// CPU Time
         csZero,			// IO Time
         csZero,			// no messages
         csZero,			// never idle
         noOfProbesPerStream_);		// num probes

  const SimpleCostVector
    cvLR(cpuLR * ff_cpu,		// CPU Time
         csZero,			// IO Time
         csZero,			// no messages
         csZero,			// never idle
         noOfProbesPerStream_);		// num probes

  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  return new STMTHEAP
    Cost(&cvFR, &cvLR, NULL, cpuCount, fragmentsPerCPU);

}      // CostMethodSample::computeOperatorCostInternal()

// -----------------------------------------------------------------------
// CostMethodIsolatedScalarUDF::computeOperatorCostInternal().
// -----------------------------------------------------------------------

Cost*
CostMethodIsolatedScalarUDF::computeOperatorCostInternal(RelExpr* op,
                                                       const Context* myContext,
                                                       Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // -----------------------------------------
  // Determine the number of input Rows/Probes
  // -----------------------------------------
  CostScalar noOfProbes =
    ( myContext->getInputLogProp()->getResultCardinality() ).minCsOne();

  // The noOfProbes is used to scale up the cost of the UDF.
  // However since the UDF assumes the cost may be different for the first
  // time it is called due to initialization of the UDF's data structures,
  // perhaps loading of DLLs etc, we will subtract the first probe out.

  noOfProbes -= csOne;
 
  IsolatedScalarUDF *udf = (IsolatedScalarUDF *) op;

  // Make sure we actually are a UDF.
  CMPASSERT( udf != NULL );
  CMPASSERT( op->getOperatorType() == REL_ISOLATED_SCALAR_UDF  );
  

  // ---------------------------------------------------------------------
  // This CostMethod basically computes a CPU cost for IsolatedScalarUDF 
  // operators. 
  // 
  // It uses the intialCost numbers for the first probe as it is assumed
  // that it may require initialization of the routine's data structures,
  // may include loading of DLLs
  // It applies the formula:
  //
  // cpu  = initialCpuCost * fanOut +
  //        normalCpuCost * noOfProbes * fanOut+  
  //
  // where the row counts are those
  // of the total result set, and then amortize the cost across streams.
  // It takes the first row count to be just its last row count amortized
  // across the no of probes.

  // We factor in the cost of sending messages to and from the UDR
  // server in a similar fashion:
  //
  // msgs = initialMsgCost * 2 * fanOut +
  //        normalMsgCost * noOfProbes * 2 * fanOut (2 messages per row)
  // ---------------------------------------------------------------------


  // Make sure we have a RoutineDesc.
  CMPASSERT( udf->getRoutineDesc() != NULL );


  // Get a reference to the routine Cost Vectors.
  SimpleCostVector &initialCostV = udf->getRoutineDesc()->getEffInitialRowCostVector();
  SimpleCostVector &normalCostV = udf->getRoutineDesc()->getEffNormalRowCostVector();

  // Gather the different cost numbers
  CostScalar initialCpuCost = initialCostV.getCPUTime();
  CostScalar initialMsgCost = initialCostV.getMessageTime();
  CostScalar initialIOCost = initialCostV.getIOTime();

  CostScalar normalCpuCost = normalCostV.getCPUTime();
  CostScalar normalMsgCost = normalCostV.getMessageTime();
  CostScalar normalIOCost = normalCostV.getIOTime();
  CostScalar fanOut = udf->getRoutineDesc()->getEffFanOut();


  // Following code copied from FileScan
  CostScalar resultSetCardinality =
   udf->getGroupAttr()->
    outputLogProp(myContext->getInputLogProp())->getResultCardinality();
  // $$$ Due to a bug in histograms (up to tag A091197_1)
  // $$$ sometimes the cardinality is negative, if so, fix it
  // $$$ to pass regressions:
  if ( resultSetCardinality.isLessThanZero() /* < csZero */ )
    resultSetCardinality = CostScalar(fanOut);

  CostScalar cpu = initialCpuCost + (normalCpuCost * (fanOut-1));
  CostScalar msgs = initialMsgCost * 2 + (normalMsgCost * (fanOut-1));
  CostScalar io = initialIOCost + (normalIOCost * (fanOut-1));


  // ---------------------------------------------------------------------
  // Synthesize the First Row cost vector.
  // This is used for [Fist N] type queries..
  // The number we are computing here is actually only accurate for 
  // [First 1], but it is consistent with what we do for FIXEDCOST nodes.
  // ---------------------------------------------------------------------
  SimpleCostVector cvFR (
    cpu/countOfStreams_                            // converting CPU instr
     * CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions(), //into time
    io/countOfStreams_,msgs/countOfStreams_,csZero,
    noOfProbesPerStream_);


  cpu += normalCpuCost * noOfProbes * fanOut;
  msgs += normalMsgCost * noOfProbes * 2 * fanOut;
              // double it to account for two messages per row.
              // we are making a rough assumption here that we only require
              // one message to the UDR server and one return message for the
              // result per row.


  io += normalIOCost * noOfProbes * fanOut;

  // ---------------------------------------------------------------------
  // Synthesize the stead state cost vector.
  // This is used for [Last N] type queries..
  // ---------------------------------------------------------------------
  SimpleCostVector cvLR (
    cpu/countOfStreams_ * CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions(),
    io/countOfStreams_,msgs/countOfStreams_,csZero,
    noOfProbesPerStream_);


  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"IsolatedScalarUDF::computeOperatorCost()\n");
    cvFR.print(pfp);
    cvLR.print(pfp);
  }
#endif

  // ---------------------------------------------------------------------
  // Synthesize and return the cost object.
  // ---------------------------------------------------------------------

  // Find out the number of cpus and number of fragments per cpu.
  Lng32 cpuCount, fragmentsPerCPU;
  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  Cost *costPtr = new STMTHEAP Cost( &cvFR,
                                     &cvLR,
                                     NULL,
                                     cpuCount,
                                     fragmentsPerCPU
                                   );

#ifndef NDEBUG
  if ( printCost )
    {
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", costPtr->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return costPtr;

}  // CostMethodIsolatedScalarUDF::computeOperatorCostInternal().
// LCOV_EXCL_STOP

//<pb>

