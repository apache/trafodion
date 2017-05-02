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
**************************************************************************
*
* = File:         ScmCostMethod.C
* Description:  Cost estimation interface object for Simple Cost Model
* Language:     C++
*
* Last Modified: 
* Modified by:  
* Purpose:       Simple Cost Vector Reduction
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
#include "SimpleScanOptimizer.h"
#include "NAFileSet.h"
#include "SchemaDB.h"
#include "CostMethod.h"
#include "Cost.h"
#include "NodeMap.h"
#include "HDFSHook.h"
#include "CmpStatement.h"
#include "sqludr.h"
#include <math.h>

#ifndef NDEBUG
static THREAD_P FILE* pfp = NULL;
#endif // NDEBUG

// -----------------------------------------------------------------------
// CostMethod::scmComputeOperatorCost()
// -----------------------------------------------------------------------
Cost*
CostMethod::scmComputeOperatorCost(RelExpr* op,
                                   const PlanWorkSpace* pws,
				   Lng32& countOfStreams)
{
  Cost* cost;
  try {
    cost = scmComputeOperatorCostInternal(op, pws, countOfStreams);
  } catch(...) {
    // cleanUp() must be called before this function is called again
    // because wrong results may occur the next time scmComputeOperatorCost()
    // is called and because the SharedPtr objects must be set to zero.
    // Failure to call cleanUp() will very likely cause problems.
    cleanUp();
    throw;  // rethrow the exception
  }

  cleanUp();
  return cost;
}                     // CostMethod::scmComputeOperatorCost()

// -----------------------------------------------------------------------
// CostMethod::scmComputeOperatorCostInternal()      (derived class must redefine)
// -----------------------------------------------------------------------
Cost*
CostMethod::scmComputeOperatorCostInternal(RelExpr* op,
                                           const PlanWorkSpace* pws,
					   Lng32& countOfStreams )
{
  countOfStreams = 1;

#ifndef NDEBUG
  if ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON )
    fprintf(stdout," %s : is not yet implemented, plan may not be optimal\n", className_);
#endif // NDEBUG
  Cost* costPtr = 
    scmCost(1e32 /*tcProc */, csZero, csZero, csZero, csZero, csOne,
	    csZero, csZero, csZero, csZero);
  return costPtr;

} // CostMethod::scmComputeOperatorCostInternal()


//<pb>
//==============================================================================
// scmComputePlanCost() produces a final cumulative cost for an entire
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
CostMethod::scmComputePlanCost( RelExpr* op,
                                const PlanWorkSpace* pws,
                                Lng32 planNumber
                           )
{
  const Context* myContext = pws->getContext();

  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( op        != NULL );
  CMPASSERT( myContext != NULL );
  CMPASSERT( pws       != NULL );

  //--------------------------------------------------------------------------
  //  Grab parent's cost (independent of its children) directly from the plan
  // work space.  This cost should contain result of scmComputeOperatorCost().
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

  // The following code assumes that each operator will have maximum
  // of two children. And requires modification if that is not true.
  // For example UNION operator.

  CostPtr leftChildCost = NULL;
  CostPtr rightChildCost = NULL;
  if (op->getArity() == 1)
  {
    getChildCostForUnaryOp(op,
                           myContext,
                           pws,
                           planNumber,
                           leftChildCost);
  }
  else if (op->getArity() == 2)
  {
    getChildCostsForBinaryOp( op
                          , myContext
                          , pws
                          , planNumber
                          , leftChildCost
                          , rightChildCost);
  }
  else
    ABORT("CostMethod::scmComputePlanCost(): More than two children");


  Cost* planCost = scmRollUp( parentCost
                            , leftChildCost
                            , rightChildCost
                            , myContext->getReqdPhysicalProperty()
                            );
  if (leftChildCost)
    delete leftChildCost;
  if (rightChildCost)
    delete rightChildCost;
  delete parentCost;

  return planCost;

} // CostMethod::computePlanCost()

//==============================================================================
//  Roll up children cost and parent cost into a cumulative cost.
//  This is the default method and may be overridden for specific operators.
//
// Input:
//  parentCost -- Cost of parent independent of its child.
//
//  leftChildCost  -- cumulative cost of the left child.
//
//  rightChildCost  -- cumulative cost of the right child.
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
CostMethod::scmRollUp( Cost* const parentCost
                     , Cost* const leftChildCost
                     , Cost* const rightChildCost
                     , const ReqdPhysicalProperty* const rpp
                  )
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( parentCost  != NULL );
 
  SimpleCostVector parentVector = parentCost->getScmCplr();
  SimpleCostVector leftChildVector, rightChildVector;

  if (leftChildCost != NULL)
    leftChildVector = leftChildCost->getScmCplr();
  if (rightChildCost != NULL)
    rightChildVector = rightChildCost->getScmCplr();

  SimpleCostVector cumCostVector;

  if (leftChildCost == NULL)
    cumCostVector = parentVector;

  else 
  {
    if (rightChildCost == NULL)
      cumCostVector = parentVector + leftChildVector;
    else 
      cumCostVector = parentVector + (leftChildVector + rightChildVector);
  }

 return new STMTHEAP Cost(&cumCostVector);

} // CostMethod::scmRollUp()

//==============================================================================
//  Wrapper for SCM Cost constructor.
//  Used by SCM only.
//
// Return:
//  Cost
//
//==============================================================================
Cost *
CostMethod::scmCost(CostScalar tuplesProcessed,
		    CostScalar tuplesProduced,
		    CostScalar tuplesSent,
		    CostScalar ioRand,
		    CostScalar ioSeq,
		    CostScalar noOfProbes,
		    CostScalar input1RowSize,
		    CostScalar input2RowSize,
		    CostScalar outputRowSize,
		    CostScalar probeRowSize)
{
  // assert if called by OCM .
  DCMPASSERT(CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON);

  SimpleCostVector scmLR ( csZero,    		/* CPUTime */
			   csZero,    		/* IOTime */
			   csZero,    		/* MSGTime */
			   csZero,    		/* idle time */
			   tuplesProcessed,	/* tcProc */
			   tuplesProduced,	/* tcProd */
			   tuplesSent,   	/* tcSent */
			   ioRand,     		/* ioRand */
			   ioSeq,    		/* ioSeq */
			   noOfProbes ); 	/* num probes */

  // This is ok for now, as cpuCount will almost always be >= countOfStreams.
  // The reason this is commented out for now is because exchange costing
  // does not set countOfStreams_ and this leads to problems. Doing this in 
  // each operator separately will circumvent this problem, but make the code
  // ugly, so it is best to comment it out for now. Does not have any negative
  // impact on plans.
  /*  if (cpuCount < countOfStreams_)
  {
    scmLR = scmLR.scaleByValue(countOfStreams_/cpuCount);
  }
  */

  NABoolean scmDebugOn = (CmpCommon::getDefault(NCM_PRINT_ROWSIZE) == DF_ON);
  if (scmDebugOn == TRUE)
  {
    // debug mode, build another SimpleCostVector with rowsize information
    // for debugging purposes only
    // FOR INTERNAL USE ONLY.
    const SimpleCostVector scmDebug ( csZero,    		
				      csZero,    		
				      csZero,    		
				      csZero,    		
				      input1RowSize,	/* input1 rowsize */
				      input2RowSize,	/* input2 rowsize */
				      outputRowSize,	/* output rowsize */
				      probeRowSize,	/* probe rowsize */
				      csZero,   		
				      csZero );    	

    return new STMTHEAP Cost(&scmLR, &scmDebug);
  }
  else
  {
    // Normal mode
    return new STMTHEAP Cost(&scmLR);
  }
} // CostMethod::scmCost

//==============================================================================
//  Wrapper for SCM Cost constructor.
//  Used by SCM only.
//
// Return:
//  Cost
//
//==============================================================================
Cost *
ScanOptimizer::scmCost(CostScalar tuplesProcessed,
		       CostScalar tuplesProduced,
		       CostScalar tuplesSent,
		       CostScalar ioRand,
		       CostScalar ioSeq,
		       CostScalar noOfProbes,
		       CostScalar input1RowSize,
		       CostScalar input2RowSize,
		       CostScalar outputRowSize,
		       CostScalar probeRowSize)
{
  // assert if called by OCM .
  DCMPASSERT(CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON);

  SimpleCostVector scmLR ( csZero,    		/* CPUTime */
			   csZero,    		/* IOTime */
			   csZero,    		/* MSGTime */
			   csZero,    		/* idle time */
			   tuplesProcessed,	/* tcProc */
			   tuplesProduced,	/* tcProd */
			   tuplesSent,  	/* tcSent */
			   ioRand,     		/* ioRand */
			   ioSeq,    		/* ioSeq */
			   noOfProbes );	/* num probes */


  NABoolean scmDebugOn = (CmpCommon::getDefault(NCM_PRINT_ROWSIZE) == DF_ON);
  if (scmDebugOn == TRUE)
  {
    // debug mode, build another SimpleCostVector with rowsize information
    // for debugging purposes only
    // FOR INTERNAL USE ONLY.
    const SimpleCostVector scmDebug ( csZero,    		
				      csZero,    		
				      csZero,    		
				      csZero,    		
				      input1RowSize,	/* input1 rowsize */
				      input2RowSize,	/* input2 rowsize */
				      outputRowSize,	/* output rowsize */
				      probeRowSize,	/* probe rowsize */
				      csZero,   		
				      csZero );    	

    return new STMTHEAP Cost(&scmLR, &scmDebug);
  }
  else
  {
    // Normal mode
    return new STMTHEAP Cost(&scmLR);
  }
} // ScanOptimizer::scmCost

//==============================================================================
//  Scale the cost to account for rowsizes. In case of large rowsizes,
//  tuple counts alone may not be enough to capture the true costs.
//
// Inputs:
//  rowSize -- size of the row.
// Output:
//  none.
//
// Return:
//  RowSize factor
//
//==============================================================================
CostScalar
CostMethod::scmRowSizeFactor( CostScalar rowSize, ncmRowSizeFactorType rowSizeFactorType )
{
  CostScalar rowSizeFactor;

  switch(rowSizeFactorType)
  {
     case TUPLES_ROWSIZE_FACTOR:
       rowSizeFactor = 
	 ActiveSchemaDB()->getDefaults().getAsDouble(NCM_TUPLES_ROWSIZE_FACTOR);
       break;

     case SEQ_IO_ROWSIZE_FACTOR:
       rowSizeFactor = 
	 ActiveSchemaDB()->getDefaults().getAsDouble(NCM_SEQ_IO_ROWSIZE_FACTOR);
       break;

     case RAND_IO_ROWSIZE_FACTOR:
       rowSizeFactor = 
	 ActiveSchemaDB()->getDefaults().getAsDouble(NCM_RAND_IO_ROWSIZE_FACTOR);
       break;

     default:
       rowSizeFactor = 0.0;
       break;
  }

  // Defensive programming
  if (rowSize <= 1)
    return 1.0;

  return MAXOF(pow(rowSize.getValue(), rowSizeFactor.getValue()), 1.0);

} // CostMethod::scmRowSizeFactor

//==============================================================================
//  Scale the cost to account for rowsizes. In case of large rowsizes,
//  tuple counts alone may not be enough to capture the true costs.
//
// Inputs:
//  rowSize -- size of the row.
// Output:
//  none.
//
// Return:
//  RowSize factor
//
//==============================================================================
CostScalar
ScanOptimizer::scmRowSizeFactor( CostScalar rowSize, ncmRowSizeFactorType rowSizeFactorType )
{
  CostScalar rowSizeFactor;

  switch(rowSizeFactorType)
  {
     case TUPLES_ROWSIZE_FACTOR:
       rowSizeFactor = 
	 ActiveSchemaDB()->getDefaults().getAsDouble(NCM_TUPLES_ROWSIZE_FACTOR);
       break;

     case SEQ_IO_ROWSIZE_FACTOR:
       rowSizeFactor = 
	 ActiveSchemaDB()->getDefaults().getAsDouble(NCM_SEQ_IO_ROWSIZE_FACTOR);
       break;

     case RAND_IO_ROWSIZE_FACTOR:
       rowSizeFactor = 
	 ActiveSchemaDB()->getDefaults().getAsDouble(NCM_RAND_IO_ROWSIZE_FACTOR);
       break;

     default:
       rowSizeFactor = 0.0;
       break;
  }

  // Defensive programming
  if (rowSize <= 1)
    return 1.0;

  return MAXOF(pow(rowSize.getValue(), rowSizeFactor.getValue()), 1.0);

} // ScanOptimizer::scmRowSizeFactor

//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodRelRoot                         */
/*                                                                    */
/**********************************************************************/
Cost*
CostMethodRelRoot::scmComputeOperatorCostInternal(RelExpr* op,
                                                  const PlanWorkSpace* pws,
						  Lng32& countOfStreams)
{
  // -------------------------------------------------------------
  // Save off estimated degree of parallelism.  Always 1 for root.
  // -------------------------------------------------------------
  countOfStreams = 1;
  // ---------------------------------------------------------------------
  // In SCM Root operator cost is ignored. so, we just return an empty
  // Cost object.
  return new HEAP Cost();

} // CostMethodRelRoot::scmComputeOperatorCostInternal()

//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodFixedCostPerRow                  */
/*                                                                    */
/**********************************************************************/
Cost*
CostMethodFixedCostPerRow::scmComputeOperatorCostInternal(RelExpr* op,
                                                          const PlanWorkSpace* pws,
							  Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op, pws->getContext());
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // ---------------------------------------------------------------------
  // In SCM Fixed cost per row is ignored as this cost doesn't affect plan.
  // So, we just return an empty
  // Cost object.
  return new HEAP Cost();

} // CostMethodFixedCostPerRow::scmComputeOperatorCostInternal()



//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodFileScan                         */
/*                                                                    */
/**********************************************************************/
Cost*
CostMethodFileScan::scmComputeOperatorCostInternal(RelExpr* op,
                                                   const PlanWorkSpace* pws,
                                                   Lng32& countOfStreams)
{
  // call old computeOperatorCostInternal method.
  return computeOperatorCostInternal( op, pws->getContext(), countOfStreams );

} // CostMethodFileScan::scmComputeOperatorCostInternal()

//<pb>
/**********************************************************************/
/*                                                                    */
/*                         CostMethodDP2Scan                          */
/*                                                                    */
/**********************************************************************/
Cost*
CostMethodDP2Scan::scmComputeOperatorCostInternal(RelExpr* op,
                                                  const PlanWorkSpace* pws,
						  Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();
  FileScan *fs = (FileScan *) op;

  if (!fs->isHiveTable())
    // call old computeOperatorCostInternalx method.
    return computeOperatorCostInternal( op, myContext, countOfStreams );

  // ---------------------------------------------------------------------
  // Try to do a very vanilla cost computation for Hive scans for now
  // ---------------------------------------------------------------------
  
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;
  Cost* hiveScanCost = NULL;

  CostScalar tuplesProcessed = csZero;
  CostScalar tuplesProduced = csZero;

  // Hbase table case
  if (fs->isHbaseTable()) {

     CostScalar outputRowSize = 100 /* getEstimatedRecordLength*/;
     CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

     hiveScanCost =  scmCost(100,
                                100,
                                csZero,
                                csZero,
                                csZero,
                                noOfProbesPerStream_,
                                csZero,
                                csZero,
                                outputRowSize,
                                csZero);
  } else {

     // Hive table case
     HHDFSStatsBase hdfsStats;
   
     fs->getHiveSearchKey()->accumulateSelectedStats(hdfsStats);
    
     CostScalar tuplesProcessed = (CostScalar)hdfsStats.getEstimatedRowCount();
     CostScalar tuplesProduced =  myRowCount_ / partFunc_->getCountOfPartitions();
   
     // ---------------------------------------------------------------------
     // tupleProduced can never be more than tupleProcessed.
     // This thing can happen sometimes especially for partial group bys since 
     // cardinality estimates do not distinguish between partial 
     // and full group bys.
     // ---------------------------------------------------------------------
     tuplesProduced = MINOF(tuplesProcessed, tuplesProduced);
     
     CostScalar outputRowSize = (CostScalar)hdfsStats.getEstimatedRecordLength();
     CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);
   
     tuplesProcessed *= outputRowSizeFactor;
     tuplesProduced *= outputRowSizeFactor;
   
     // ---------------------------------------------------------------------
     // Synthesize and return the cost object.
     // ---------------------------------------------------------------------
     hiveScanCost =  scmCost(tuplesProcessed,
                                   tuplesProduced,
                                   csZero,
                                   csZero,
                                   csZero,
                                   noOfProbesPerStream_,
                                   csZero,
                                   csZero,
                                   outputRowSize,
                                   csZero);
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
    fprintf(pfp,"HIVESCAN::scmComputeOperatorCostInternal()\n");
    fprintf(pfp,"tuplesProcessed=%g,myRowCount=%g\n",
            tuplesProcessed.toDouble(),myRowCount_.toDouble());
    hiveScanCost->getScmCplr().print(pfp);
    fprintf(pfp, "Elapsed time: ");
    fprintf(pfp,"%f", hiveScanCost->
	    convertToElapsedTime(myContext->getReqdPhysicalProperty()).
	    value());
    fprintf(pfp,"\n");
  }
#endif

  return hiveScanCost;
} // CostMethodDP2Scan::scmComputeOperatorCostInternal()

// SimpleFileScanOptimizer::scmComputeCostVectors()
// Computes the cost vectors for this scan using the simple costing model.
// Computes:
//    - number of tuples processed
//    - number of tuples produced
//    - sequential IOs
//
// OUTPUTS: lastRow SimpleCostVectors of a new Cost object are populated.
//
Cost *
SimpleFileScanOptimizer::scmComputeCostVectors()
{
  // if the table is Hbase, then call scmComputeCostVectorsForHbase()
  if (getIndexDesc()->getPrimaryTableDesc()->getNATable()->isHbaseTable())
    return scmComputeCostVectorsForHbase();

  const LogPhysPartitioningFunction *logPhysPartFunc =
    getContext().getPlan()->getPhysicalProperty()->getPartitioningFunction()->
    castToLogPhysPartitioningFunction();

  NABoolean syncAccess = FALSE; 
  CostScalar numActivePartitions;
  CostScalar tuplesProcessed, tuplesProduced, tuplesSent = csZero;

  numActivePartitions = getEstNumActivePartitionsAtRuntime();
  if (logPhysPartFunc != NULL)
    syncAccess = logPhysPartFunc->getSynchronousAccess(); 

  if (syncAccess)
  {
    tuplesProcessed = getSingleSubsetSize();
    tuplesProduced = getResultSetCardinality();
  }
  else
  {
    tuplesProcessed = (getSingleSubsetSize()/numActivePartitions).getCeiling();
    tuplesProduced = getResultSetCardinalityPerScan();
  }

  setProbes(1);
  setTuplesProcessed(getSingleSubsetSize());

  CostScalar numBlocks = getNumBlocksForRows(tuplesProcessed);
  //CostScalar numBlocks = estimateSeqKBReadPerScan()/getBlockSizeInKb();
  // Store in ScanOptimizer object.  FileScan will later grab this
  // value.
  setNumberOfBlocksToReadPerAccess(numBlocks);

  // Factor in row sizes.
  CostScalar rowSize = recordSizeInKb_ * csOneKiloBytes;
  CostScalar outputRowSize = getRelExpr().getGroupAttr()->getRecordLength();
  CostScalar rowSizeFactor = scmRowSizeFactor(rowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);
  tuplesProcessed *= rowSizeFactor;
  tuplesProduced *= outputRowSizeFactor;

  CostScalar seqIORowSizeFactor = scmRowSizeFactor(rowSize, SEQ_IO_ROWSIZE_FACTOR);
  numBlocks *= seqIORowSizeFactor;

  // fix Bugzilla #1110.
  setEstRowsAccessed(getSingleSubsetSize());

  Cost* scanCost = 
    scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, numBlocks, csOne,
	    rowSize, csZero, outputRowSize, csZero);

  return scanCost;
} // scmComputeCostVectors

// SimpleFileScanOptimizer::scmComputeCostVectorsMultiProbes()
// Computes the cost vectors for this multi-probe scan using the simple costing model.
// Computes:
//    - number of tuples processed for all probes
//    - number of tuples produced for all probes
//    - sequential IOs for all probes
//    - number of probes for all probes
// currently assumes that probes are not ordered. Need to implement
// ordered probes case later.
//
// OUTPUTS: lastRow SimpleCostVectors of a new Cost object are populated.
//
Cost *
SimpleFileScanOptimizer::scmComputeCostVectorsMultiProbes()
{
  if ( getIndexDesc()->getPrimaryTableDesc()->getNATable()->isHbaseTable() AND
       (CmpCommon::getDefault(NCM_HBASE_COSTING) == DF_ON) )
    return scmComputeCostVectorsMultiProbesForHbase();

  CostScalar numOuterProbes = (getContext().getInputLogProp())->getResultCardinality();
  CostScalar numActivePartitions =  getNumActivePartitions();
  CostScalar ioSeq, ioRand, numRandIOs;
  NABoolean isUnique = getSearchKey()->isUnique();
  NABoolean isAnIndexJoin;
  ValueIdSet charInputs = getRelExpr().getGroupAttr()->getCharacteristicInputs();
  const ReqdPhysicalProperty* rpp = getContext().getReqdPhysicalProperty();
  NABoolean ocbJoin = FALSE;
  if (rpp != NULL && rpp->getOcbEnabledCostingRequirement())
    ocbJoin = TRUE;

  // Effective Total Row Count is the size of the bounding subset of
  // all probes.  Typically this will be all the rows of the table,
  // but if all probes are restricted to a subset of rows (e.g. the
  // key predicate contains leading constants) then the effective row
  // count will be less than the total row count.
  estimateEffTotalRowCount(totalRowCount_, effectiveTotalRowCount_);
  CostScalar effectiveTotalRowCount = (effectiveTotalRowCount_/numActivePartitions).getCeiling();
  CostScalar effectiveTotalBlocks = getNumBlocksForRows(effectiveTotalRowCount).getCeiling();
  CostScalar cacheSize = ActiveSchemaDB()->getDefaults().getAsDouble(NCM_CACHE_SIZE_IN_BLOCKS); //getDP2CacheSizeInBlocks(getBlockSizeInKb());

  categorizeMultiProbes(&isAnIndexJoin);

  CostScalar numProbes = (probes_/numActivePartitions).getCeiling();
  CostScalar numUniqueProbes = (uniqueProbes_/numActivePartitions).getCeiling();
  CostScalar numSuccessfulProbes = (successfulProbes_/numActivePartitions).getCeiling();
  CostScalar numUniqueSuccessfulProbes = ((successfulProbes_ - duplicateSuccProbes_)/numActivePartitions).getCeiling();

  NABoolean njSeqIoFix = (CmpCommon::getDefault(NCM_NJ_SEQIO_FIX) == DF_ON);
  CostScalar numBlocksPerSuccessfulProbe = csZero;
  if (njSeqIoFix)
    numBlocksPerSuccessfulProbe = blksPerSuccProbe_;
  else
    numBlocksPerSuccessfulProbe = (blksPerSuccProbe_/numActivePartitions).getCeiling();

  CostScalar numBlocksAccessedByUniqueProbes = MINOF(numUniqueProbes, effectiveTotalBlocks);

  if (ocbJoin)
  {
    // In OCB, the probe side is broadcast to all partitions of the right side.
    numProbes = numOuterProbes;
    numUniqueProbes = numOuterProbes;
  }

  // The probes are pushed down from the nested join to the right child.
  // Initialize tuplesProcessed to the number of probes.
  CostScalar tuplesProcessed = numProbes;

  // These values are for all probes.
  CostScalar accessedRows = (getDataRows()/numActivePartitions).getCeiling();
  CostScalar selectedRows = getResultSetCardinalityPerScan();



  CostScalar tuplesProduced = MINOF(selectedRows, accessedRows);

  CostScalar outputJoinCard = tuplesProduced;
  
  const IndexDesc *iDesc = getIndexDesc();
  CostScalar indexLevels = MAXOF(iDesc->getIndexLevels() - 1, 1);

  CostScalar a = numProbes; 

  if (effectiveTotalBlocks <= cacheSize)
  {
    numRandIOs = MINOF(a, effectiveTotalBlocks);
  }
  else
  {
    if (a <= cacheSize)
    {
      numRandIOs = a;
    }
    else
    {
      numRandIOs = cacheSize + (a - cacheSize) * (effectiveTotalBlocks - cacheSize) / effectiveTotalBlocks;
    }
  }

  if (isAnIndexJoin)
  {
    // Join between Index (left child) and Table (right child),
    // involves random IOs.
    tuplesProcessed += accessedRows; 
    ioRand = numRandIOs;
  }
  else
  {
    // The right child is the base table or the covering index being probed.
    
    if (getInOrderProbesFlag() OR
        ocbJoin                OR
        (effectiveTotalBlocks <= cacheSize) OR  // Whole table fits in cache
        ((numUniqueSuccessfulProbes * blksPerSuccProbe_)
         + getFailedProbes() <= cacheSize) // all blocks accessed fits cache
       )
    {
      if (isUnique)
	tuplesProcessed += numSuccessfulProbes;
      else
	tuplesProcessed += accessedRows;

      // Incoming probes are in the same order as the clustering key, 
      // no full table scan. For every probe, a subset of the table 
      // (or covered index) is processed.
      if (isUnique && numBlocksAccessedByUniqueProbes <= cacheSize)
      {
	ioRand = numBlocksAccessedByUniqueProbes;
      }
      else if (numBlocksPerSuccessfulProbe <= cacheSize)
      { 
	ioRand = numRandIOs;
	ioSeq = numUniqueSuccessfulProbes * (numBlocksPerSuccessfulProbe - 1);
      }
      else
      {
	ioRand = numRandIOs;
	ioSeq = numSuccessfulProbes * (numBlocksPerSuccessfulProbe - 1);   
      }
    }
    else 
    {
      // Begin Temp code
      // My changes to 5450/5425 exposed a bug where RandomIo NJ preferred
      // because Ocr plan was not tried in ETL workload. I am partially 
      // rolling back my changes 5425, but will fix the root cause asap.
      // get all predicates
      ValueIdSet allPreds;
      allPreds = getSingleSubsetPreds();

      // get all predicates' base columns
      ValueIdSet allReferencedBaseCols;
      allPreds.findAllReferencedBaseCols(allReferencedBaseCols);

      // try to find a predicate that matches
      // 1st nonconstant key prefix column
      NABoolean foundKey = FALSE;
      CollIndex x = 0;
      ColAnalysis *colA = NULL;
      const ValueIdList *currentIndexSortKey = &(iDesc->getOrderOfKeyValues());

      for (x = 0; x < (*currentIndexSortKey).entries() && !foundKey; x++)
      {
        ValueId firstkey = (*currentIndexSortKey)[x];
        // firstkey with a constant predicate does not count in
        // making this NJ better than a HJ. keep going.
        ItemExpr *cv;
        NABoolean isaConstant = FALSE;
        ValueId firstkeyCol;
        colA = firstkey.baseColAnalysis(&isaConstant, firstkeyCol);
        if (isaConstant)
          continue; // try next prefix column
        if (!colA) // no column analysis
          break; // we can't go further, break out of the loop.
        if (colA->getConstValue(cv,FALSE/*useRefAConstExpr*/))
          continue; // try next prefix column
        // any predicate on first nonconstant prefix key column?
        if (allReferencedBaseCols.containsTheGivenValue(firstkeyCol))
          // nonconstant prefix key matches predicate
          foundKey = TRUE;
        else
         break; // predicate is not a key predicate, cost it high
      }

      if ((NOT (iDesc->isClusteringIndex()) &&
          (getSearchKey()->getKeyPredicates().entries() > 0)))
        foundKey = TRUE;

      // end Temp code

      // re-check risky NJ Heuristics from JoinToTSJRule::topMatch()
      NABoolean allowNJ = TRUE;

      Lng32 ratio = (ActiveSchemaDB()->getDefaults()).getAsLong
      (HJ_SCAN_TO_NJ_PROBE_SPEED_RATIO);

      // innerS is size of data scanned for inner table in HJ plan
      CostScalar innerS = effectiveTotalRowCount_ * getRecordSizeInKb() * 1024;

      // We must allow NJ if #outerRows <= 1 because
      // HashJoinRule::topMatch disables HJ for that case.
      // if innerS < #outerRows * ratio, prefer HJ over NJ.
      if (ratio > 0 && numOuterProbes > 1 &&
          (innerS < numOuterProbes * ratio) )
          allowNJ = FALSE;
      // if probes are partially ordered and max cardinality of probes
      // < 10000 * probes, then cost this NJ cheaper than random order probes.
      // Otherwise we suspect card estimation of probes and accessedRows,
      // so do not want take chance with NJ plan.
      // This check is controlled by CQD NCM_NJ_PROBES_MAXCARD_FACTOR(10000).

      // get max card factor and probes estimated max cardinality
      CostScalar maxCardFactor = (ActiveSchemaDB()->getDefaults()).getAsDouble(NCM_NJ_PROBES_MAXCARD_FACTOR);
      CostScalar maxCardOfProbes = (getContext().getInputLogProp())->getMaxCardEst();
      if ((getPartialOrderProbesFlag() &&
          maxCardOfProbes != -1 &&
          maxCardOfProbes / numOuterProbes < maxCardFactor &&
          allowNJ) || foundKey)
      {

	if (isUnique)
	  tuplesProcessed += numSuccessfulProbes;
	else
	  tuplesProcessed += accessedRows;
	if (isUnique && numBlocksAccessedByUniqueProbes <= cacheSize)
	{
	  ioRand = numBlocksAccessedByUniqueProbes;
	}
	else
	{
	  ioRand = numRandIOs;
	  ioSeq = numSuccessfulProbes * (numBlocksPerSuccessfulProbe - 1);  
	}
      }
      else
      {
        // No appropriate index, full table (or covering index) scan,
	// involves sequential IOs.  For every probe, the whole right side 
	// is accessed. For covering indexes, the row size will be typically 
	// smaller than the row size of the base table, so the number of blocks
	// will less, making it the cheaper alternative. 
        tuplesProcessed += (numProbes * effectiveTotalRowCount);
        ioSeq = numProbes * effectiveTotalBlocks;
        ioRand = numRandIOs;
      }
    }
  }

  // set the field before it is being mutiplied by the row size factor
  setTuplesProcessed(tuplesProcessed*numActivePartitions);
 
  // Store in ScanOptimizer object.  FileScan will later grab this value.
  ioSeq = MAXOF(ioSeq, 0);
  setNumberOfBlocksToReadPerAccess(ioSeq.minCsOne());

  // Factor in row sizes.
  CostScalar rowSize = recordSizeInKb_ * csOneKiloBytes;
  CostScalar probeRowSize = charInputs.getRowLength();
  CostScalar outputRowSize = getRelExpr().getGroupAttr()->getRecordLength();

  CostScalar rowSizeFactor = scmRowSizeFactor(rowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(probeRowSize + outputRowSize);
  tuplesProcessed *= rowSizeFactor;
  tuplesProduced *= outputRowSizeFactor;

  CostScalar seqIORowSizeFactor = scmRowSizeFactor(rowSize, SEQ_IO_ROWSIZE_FACTOR);
  CostScalar randIORowSizeFactor = scmRowSizeFactor(rowSize, RAND_IO_ROWSIZE_FACTOR);

  ioSeq *= seqIORowSizeFactor;
  ioRand *= randIORowSizeFactor;

  LogPhysPartitioningFunction *logPhysPartFunc =
    (LogPhysPartitioningFunction *)  // cast away const
    getContext().getPlan()->getPhysicalProperty()->getPartitioningFunction()->
    castToLogPhysPartitioningFunction();

  if (logPhysPartFunc != NULL)
  {
    PartitioningFunction* logPartFunc = logPhysPartFunc->getLogPartitioningFunction();
    CostScalar numParts = logPartFunc->getCountOfPartitions();
    CostScalar serialNJFactor = ActiveSchemaDB()->getDefaults().getAsDouble(NCM_SERIAL_NJ_FACTOR);
    if (isAnIndexJoin && numParts == 1)
    {
      tuplesProcessed *= serialNJFactor;
      tuplesProduced *= serialNJFactor;
      ioRand *= serialNJFactor;
      ioSeq *= serialNJFactor;
    }
  }

  if (isProbeCacheApplicable())
  {
    CostScalar pcCostAdjFactor = getProbeCacheCostAdjFactor();
    tuplesProduced *= pcCostAdjFactor;
    ioRand *= pcCostAdjFactor;
    ioSeq *= pcCostAdjFactor;
  }

  Cost* scanCostMultiProbes = 
    scmCost(tuplesProcessed, tuplesProduced, csZero, ioRand, ioSeq, numProbes,
	    rowSize, csZero, outputRowSize, probeRowSize);

  // temporary fix to cost index join cheaper than base table scan.
  // when we allow salted indexes, then this can be removed
  if (isAnIndexJoin &&
      getIndexDesc()->getPrimaryTableDesc()->getNATable()->isHbaseTable())
  {
    CostScalar redFactor = CostScalar(1.0) /
               ActiveSchemaDB()->getDefaults().getAsDouble(NCM_IND_JOIN_COST_ADJ_FACTOR);
    scanCostMultiProbes->cpScmlr().scaleByValue(redFactor);
  }

  // temporary fix to cost index scan cheaper than base table scan.
  // when we allow salted indexes, then this can be removed
  if ( !(getIndexDesc()->isClusteringIndex()) &&
       getIndexDesc()->getPrimaryTableDesc()->getNATable()->isHbaseTable()) 
  {
    CostScalar redFactor = CostScalar(1.0) /
               ActiveSchemaDB()->getDefaults().getAsDouble(NCM_IND_SCAN_COST_ADJ_FACTOR);
    scanCostMultiProbes->cpScmlr().scaleByValue(redFactor);
  }

  // fix Bugzilla #1110.
  setEstRowsAccessed(getDataRows());

  return scanCostMultiProbes;
 
} // SimpleFileScanOptimizer::scmComputeCostVectorsMultiProbes(...)

// Compute the Cost for this single subset Scan using the simple cossting model.
//
// Attempts to find an existing basic cost object which can be reused.
//
// Computes or reuses the last row cost vector. 
//
// OUTPUTS:
// Return - A NON-NULL Cost* representing the Cost for this scan node
//
// Side-Affects - computes and sets the numberOfBlocksToReadPerAccess_
// data member of ScanOptimizer.  This value will be captured by the
// FileScan node and passed to DP2 by the executor, DP2 uses it to
// decide whether it will do read ahead or not.
//
Cost*
SimpleFileScanOptimizer::scmComputeCostForSingleSubset()
{
  // Determine if this scan is receiving multiple probes.  If so,
  // use the MultiProbe Scan Optimizer methods.  Other wise use the
  // single probe methods.
  //

  Cost *scanCost;

  CostScalar repeatCount =  
      getContext().getPlan()->getPhysicalProperty()->
      getDP2CostThatDependsOnSPP()->getRepeatCountForOperatorsInDP2();

  NABoolean multiProbeScan =  repeatCount.isGreaterThanOne() OR
      (getContext().getInputLogProp()->getColStats().entries() > 0);

  //Bugzilla 1110: either method called below will call setEstRowsAccessed()
  //to set the estimated rows access for this object (SimpleFileScanOptimizer)
  if ( multiProbeScan ) 
  {
    scanCost = scmComputeCostVectorsMultiProbes();
  } 
  else 
  {
    scanCost = scmComputeCostVectors();
  }

  CostScalar skewFactor = 1.0;
  if (CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) 
  {
     // ompute multiplicative factor = probesAtBusiestStream/ProbesPerScan_
     // Multiply the last row cost by the factor
     // Note that the last row cost is per Partition
     CostScalar probesAtBusyStream
                           =  getContext().getPlan()->getPhysicalProperty()->
                               getDP2CostThatDependsOnSPP()->
                               getProbesAtBusiestStream();

     CostScalar probesPerScan = scanCost->getScmCplr().getNumProbes();
     // probes must be > 0  if not assert.
     DCMPASSERT(probesPerScan.isGreaterThanZero());
    
     probesPerScan = MAXOF(probesPerScan, 1);
    
     skewFactor = (probesAtBusyStream/probesPerScan).minCsOne();
     if (CmpCommon::getDefault(NCM_SKEW_COST_ADJ_FOR_PROBES) == DF_ON)
       scanCost->cpScmlr().scaleByValue(skewFactor);
  }

  return scanCost;
  
} // SimpleFileScanOptimizer::SCMComputeCostForSingleSubset()

Cost*
FileScanOptimizer::scmComputeCostForSingleSubset()
{
    SimpleFileScanOptimizer *sfso = 
      new (STMTHEAP) SimpleFileScanOptimizer(getFileScan(),
					     getResultSetCardinality(),
					     getContext(),
					     getExternalInputs());
    SearchKey *searchKey = NULL;
    MdamKey *mdamKey = NULL;
    Cost* c = 
      sfso->optimize(searchKey, mdamKey);//scmComputeCostForSingleSubset();
			      
  // transfer probe counters from sfso to this (the FileScan optimizer)
  setProbes(sfso->getProbes());
  setSuccessfulProbes(sfso->getSuccessfulProbes());
  setUniqueProbes(sfso->getUniqueProbes());
  setDuplicateSuccProbes(sfso->getDuplicateSuccProbes());
  setTuplesProcessed(sfso->getTuplesProcessed());
  setEstRowsAccessed(sfso->getEstRowsAccessed());

  return c;
} // FileScanOptimizer::ScmComputeCostForSingleSubset()

// FileScanOptimizer::scmComputeMDAMForHbase()
// Compute MDAM cost for Hbase Scan
// Computes:
//    -- number of tuples processed by Region Server
//    -- number of tuples produced by Region Server
//    -- sequential IOs by Region Server
//    -- random IOs by Region Server
//
//    -- number of tuples processed by Hbase client
//    -- number of tuples produced by Hbase client
//    -- number of tuples received by Hbase client
//
//    OUTPUTS: SimpleCostVectors of a new Cost object are populated.
Cost*
FileScanOptimizer::scmComputeMDAMCostForHbase(
                     CostScalar &totalRows,
                     CostScalar & seeks,
                     CostScalar & seq_kb_read,
                     CostScalar& incomingProbes)
{
  // Hbase Scan cost :
  //    Cost incurred at client side (Master or ESP)
  //                   +
  //    Cost incurred at Server side (Hbase Region Server a.k.a HRS)
  //
  // Server side cost : CPU cost + IO cost
  //   CPU cost = Tuples processed +
  //              Tuples produced (same as processed for now, will be different
  //              when predicates pushed down to HRS
  //   IO cost = Seq IO cost + random IO cost
  //      Seq IO cost = number of blocks need to be read to retrive total rows 
  //                    qualified by all disjuncts
  //      random IO cost = seeks incurred by all disjuncts of all key columns
  //                       (Random IO is a function of UEC of key cols with preds missing)
  //
  //  Client side cost : CPU cost + Msg Cost
  //    CPU cost = Tuples processed for all disjuncts +
  //               Tuples produced after applying executor predicates for all disjuncts
  //    Msg cost = Tuples received from HRS for all disjuncts
  
  // estimate HRS cost
  CostScalar tcProcInHRS, tcProdInHRS, seqIOsInHRS, randomIOsInHRS = csZero;
  tcProcInHRS = totalRows;
  tcProdInHRS = totalRows; // will be different when predicates pushed down to HRS
  seqIOsInHRS = seq_kb_read / getIndexDesc()->getBlockSizeInKb();
  randomIOsInHRS = seeks;
  
  // estimate Hbase Client Side (HCS)cost
  CostScalar tcProcInHCS, tcProdInHCS, tcRcvdByHCS = csZero;
  tcProcInHCS = tcProdInHRS;
  tcProdInHCS = MINOF(getResultSetCardinality(), totalRows);
  tcRcvdByHCS = tcProdInHRS;
  
  // factor in row sizes;
  CostScalar rowSize = getIndexDesc()->getRecordSizeInKb() * csOneKiloBytes;
  CostScalar outputRowSize = getRelExpr().getGroupAttr()->getRecordLength();
  CostScalar rowSizeFactor = scmRowSizeFactor(rowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);
  CostScalar seqIORowSizeFactor = scmRowSizeFactor(rowSize, SEQ_IO_ROWSIZE_FACTOR);
  CostScalar randIORowSizeFactor = scmRowSizeFactor(rowSize, RAND_IO_ROWSIZE_FACTOR);

  // for HRS
  tcProcInHRS *= rowSizeFactor;
  tcProdInHRS *= rowSizeFactor;
  seqIOsInHRS *= seqIORowSizeFactor; 
  randomIOsInHRS *= randIORowSizeFactor;

  // for HCS
  tcProcInHCS *= rowSizeFactor;
  tcProdInHCS *= outputRowSizeFactor;
  tcRcvdByHCS *= rowSizeFactor;

  // normalize it by #region servers for HRS
  CollIndex HRSPartitions = getEstNumActivePartitionsAtRuntimeForHbaseRegions();
  tcProcInHRS = (tcProcInHRS / HRSPartitions).getCeiling();
  tcProdInHRS = (tcProdInHRS / HRSPartitions).getCeiling();
  seqIOsInHRS = (seqIOsInHRS / HRSPartitions).getCeiling();
  randomIOsInHRS = (randomIOsInHRS / HRSPartitions).getCeiling();

  // normalize it by DoP for HCS
  CollIndex HCSPartitions = getEstNumActivePartitionsAtRuntime();
  tcProcInHCS = (tcProcInHCS / HCSPartitions).getCeiling();
  tcProdInHCS = (tcProdInHCS / HCSPartitions).getCeiling();
  tcRcvdByHCS = (tcRcvdByHCS / HCSPartitions).getCeiling();

  CostScalar tuplesProcessed = tcProcInHRS + tcProcInHCS;
  CostScalar tuplesProduced = tcProdInHRS + tcProdInHCS;

  CostScalar probesPerPartition = (incomingProbes/HRSPartitions).getCeiling();

  Cost* hbaseMdamCost =
    scmCost(tuplesProcessed, tuplesProduced, tcRcvdByHCS, randomIOsInHRS,
            seqIOsInHRS, probesPerPartition, rowSize, csZero, outputRowSize, csZero);

  return hbaseMdamCost;

}

// SimpleFileScanOptimizer::scmComputeCostVectorsForHbase()
// Compute operator cost for Hbase Scan.
// Computes:
//   -- number of tuples processed by Region Server
//   -- number of tuples produced by Region Server
//   -- sequential IOs by Region Server
//
//  -- number of tuples processed by Hbase client
//  -- number of tuples produced by Hbase client
//  -- number of tuples received by Hbase client
//
// OUTPUTS: SimpleCostVectors of a new Cost object are populated.
Cost *
SimpleFileScanOptimizer::scmComputeCostVectorsForHbase()
{
  // Hbase Scan cost : 
  //    Cost incurred at client side (Master or ESP)
  //                   +
  //    Cost incurred at Server side (Hbase Region Server a.k.a HRS)
  // 
  // Server side cost : CPU cost + IO cost
  //   CPU cost = Tuples processed + 
  //              Tuples produced (same as processed for now, will be different
  //              when predicates pushed down to HRS
  //   Seq IO cost = number of blocks need to be read get "Tuples processed"
  //
  // Client side cost : CPU cost + Msg Cost
  //   CPU cost = Tuples processed + 
  //              Tuples produced after applying executor predicates 
  //   Msg cost = Tuples received from HRS

  // estimate HRS cost
  CostScalar tcProcInHRS, tcProdInHRS, seqIOsInHRS = csZero;
  tcProcInHRS = getSingleSubsetSize();
  tcProdInHRS = tcProcInHRS; // will be different when predicates pushed down to HRS
  seqIOsInHRS = getNumBlocksForRows(tcProcInHRS);

  // estimate Hbase Client Side (HCS)cost
  CostScalar tcProcInHCS, tcProdInHCS, tcRcvdByHCS = csZero;
  tcProcInHCS = tcProdInHRS;
  tcProdInHCS = getResultSetCardinality();
  tcRcvdByHCS = tcProdInHRS;

  // heuristics to favor serial plans for small queries
  NABoolean costParPlanSameAsSer = FALSE;
  CostScalar parPlanRcLowerLimit = 2.0 * 
    ActiveSchemaDB()->getDefaults().getAsDouble(NUMBER_OF_ROWS_PARALLEL_THRESHOLD);
  
  if ( tcRcvdByHCS <= parPlanRcLowerLimit AND 
       (CmpCommon::getDefault(NCM_HBASE_COSTING) == DF_ON) )
    costParPlanSameAsSer = TRUE;

  // factor in row sizes;
  CostScalar rowSize = recordSizeInKb_ * csOneKiloBytes;
  CostScalar outputRowSize = getRelExpr().getGroupAttr()->getRecordLength();
  CostScalar rowSizeFactor = scmRowSizeFactor(rowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);
  CostScalar seqIORowSizeFactor = scmRowSizeFactor(rowSize, SEQ_IO_ROWSIZE_FACTOR);
  
  // for HRS
  tcProcInHRS *= rowSizeFactor;
  tcProdInHRS *= rowSizeFactor;
  seqIOsInHRS *= seqIORowSizeFactor;

  // for HCS
  tcProcInHCS *= rowSizeFactor;
  tcProdInHCS *= outputRowSizeFactor;
  tcRcvdByHCS *= rowSizeFactor;

  // some book keeping
  setProbes(1);
  setTuplesProcessed(getSingleSubsetSize());
  setEstRowsAccessed(getSingleSubsetSize());
  setNumberOfBlocksToReadPerAccess(seqIOsInHRS);

  // normalize it by #region servers for HRS
  CollIndex HRSPartitions = getEstNumActivePartitionsAtRuntimeForHbaseRegions();
  tcProcInHRS = (tcProcInHRS / HRSPartitions).getCeiling();
  tcProdInHRS = (tcProdInHRS / HRSPartitions).getCeiling();
  seqIOsInHRS = (seqIOsInHRS / HRSPartitions).getCeiling();

  // normalize it by DoP for HCS
  CollIndex HCSPartitions = getEstNumActivePartitionsAtRuntime();
  if (costParPlanSameAsSer)
    HCSPartitions = 1;
  tcProcInHCS = (tcProcInHCS / HCSPartitions).getCeiling();
  tcProdInHCS = (tcProdInHCS / HCSPartitions).getCeiling();
  tcRcvdByHCS = (tcRcvdByHCS / HCSPartitions).getCeiling();

  // compute HRS cost + HCS cost
  // Total CPU cost
  CostScalar tuplesProcessed = tcProcInHRS + tcProcInHCS;
  CostScalar tuplesProduced = tcProdInHRS + tcProdInHCS;

  // Total IO cost = seqIOsInHRS
  // Total Msg cost = tcRcvdByHCS

  Cost* hbaseScanCost =
    scmCost(tuplesProcessed, tuplesProduced, tcRcvdByHCS, csZero,
            seqIOsInHRS, csOne, rowSize, csZero, outputRowSize, csZero);

  // temporary fix to cost index scan cheaper than base table scan.
  // when we allow salted indexes, then this can be removed
  if ( !(getIndexDesc()->isClusteringIndex()) )
  {
    CostScalar redFactor = CostScalar(1.0) /
               ActiveSchemaDB()->getDefaults().getAsDouble(NCM_IND_SCAN_COST_ADJ_FACTOR);
    hbaseScanCost->cpScmlr().scaleByValue(redFactor);
  }

  return hbaseScanCost;
}

// SimpleFileScanOptimizer::scmComputeCostVectorsMultiProbesForHbase()
// Computes the cost  for multi-probe scan using NCM for Hbase Scan.
// Computes:
//   -- number of tuples processed by Region Server for all probes
//   -- number of tuples produced by Region Server for all probes
//   -- sequential IOs by Region Server for all probes
//   -- random IOs by Region Server for all probes
//
//  -- number of tuples processed by Hbase client
//  -- number of tuples produced by Hbase client
//  -- number of tuples received by Hbase client
//
// OUTPUTS: SimpleCostVectors of a new Cost object are populated.
Cost *
SimpleFileScanOptimizer::scmComputeCostVectorsMultiProbesForHbase()
{
  // Hbase Scan cost :
  //    Cost incurred at client side (Master or ESP)
  //                   +
  //    Cost incurred at Server side (Hbase Region Server a.k.a HRS)
  //
  // Server side cost : CPU cost + IO cost
  //   CPU cost = Tuples processed +
  //              Tuples produced (same as processed for now, will be different
  //              when predicates pushed down to HRS
  //   IO cost = Seq IO cost + Random IO cost
  //   Seq IO cost = number of blocks need to be read get "Tuples processed"
  //   Random IO cost = number of seeks needed to get "Tuples processed"
  //
  // Client side cost : CPU cost + Msg Cost
  //   CPU cost = Tuples processed +
  //              Tuples produced after applying executor predicates
  //   Msg cost = Tuples received from HRS

  // estimate HRS cost
  CostScalar tcProcInHRS, tcProdInHRS, seqIOsInHRS, randIOsInHRS = csZero;
  NABoolean isUnique = getSearchKey()->isUnique();
  NABoolean isAnIndexJoin;

  // helping data members needed to compute cost vectors
  estimateEffTotalRowCount(totalRowCount_, effectiveTotalRowCount_);
  CostScalar effectiveTotalBlocks =
    getNumBlocksForRows(effectiveTotalRowCount_).getCeiling();

  categorizeMultiProbes(&isAnIndexJoin);

  CostScalar numProbes = probes_;
  CostScalar numUniqueProbes = uniqueProbes_;
  CostScalar numSuccessfulProbes = successfulProbes_;
  CostScalar numUniqueSuccessfulProbes = (successfulProbes_ - duplicateSuccProbes_);

  CostScalar estBolcksByUniqProbes = MINOF(uniqueProbes_, effectiveTotalBlocks);
  const CostScalar uniqBlocks = numUniqueSuccessfulProbes * blksPerSuccProbe_;
  CostScalar lowerBoundBlockCount = uniqBlocks + getFailedProbes();

  tcProcInHRS = numProbes;
  CostScalar accessedRows = getDataRows();
  tcProdInHRS = getResultSetCardinality();

  CollIndex HRSPartitions = getEstNumActivePartitionsAtRuntimeForHbaseRegions();
  // 52 blocks cache per region
  // Assumption : heap size of RS = 8GB
  // data block cache = 20% * 8GB => 1.6GB
  // cache in blocks = 1.6GB / 64KB (hbase block size) => 26214 blocks
  // Assume on avg RS services 500 regions = 26214/500 ~= 52 blocks per region
  CostScalar cacheSize = ActiveSchemaDB()->getDefaults().getAsDouble(NCM_CACHE_SIZE_IN_BLOCKS);
  const CostScalar cacheSizeForAll(cacheSize * HRSPartitions);

  CostScalar cacheHitProbability = (cacheSizeForAll/uniqBlocks).maxCsOne();

  // logic for estimating randomIOs
  if (isUnique)
  {
    // each probe brings no more than one data block
    if (estBolcksByUniqProbes <= cacheSizeForAll)
      randIOsInHRS = estBolcksByUniqProbes;
    else
    {
      CostScalar cacheMissProbabilityForUniqueProbe =
        ((estBolcksByUniqProbes - cacheSizeForAll)
          /estBolcksByUniqProbes).minCsZero();
      randIOsInHRS = cacheSizeForAll + // First probes to fill the cache
                  (getProbes() - cacheSizeForAll) * // rest could miss
                  cacheMissProbabilityForUniqueProbe;
    }
  }
  else
  {
    // SearchKey is not unique. In this case each successful probe could bring several 
    // (or even all) // blocks of inner table.
    if (uniqBlocks <= cacheSizeForAll)
      randIOsInHRS = estBolcksByUniqProbes;
    else
      // table does not fit in cache, extra seeks needed.
      randIOsInHRS = getUniqueProbes() +
                  getDuplicateSuccProbes() * (csOne - cacheHitProbability);
  }

  // logic for estimating seqIOs
  // we also modify tcProcInHRS
  if ( (effectiveTotalBlocks <= cacheSizeForAll) OR  // Whole table fits in cache
       (lowerBoundBlockCount <= cacheSizeForAll) OR  // all blocks accessed fits cache
          // Duplicate probe finds data in cache
        ((getBlksPerSuccProbe() <= cacheSizeForAll) AND getInOrderProbesFlag())
     )
  {
    // Full cache benefit
    seqIOsInHRS = MINOF(effectiveTotalBlocks, lowerBoundBlockCount);
    if (isUnique)
      tcProcInHRS += numSuccessfulProbes;
    else
      tcProcInHRS += accessedRows;
  }
  else if ((getBlksPerSuccProbe() > cacheSize) &&
            getInOrderProbesFlag())
  {
    seqIOsInHRS = lowerBoundBlockCount;
    tcProcInHRS += accessedRows;
  }
  else if (isLeadingKeyColCovered())
  {
    const CostScalar extraDataBlocks =
      (getDuplicateSuccProbes() * getBlksPerSuccProbe()) * (csOne - cacheHitProbability);
    seqIOsInHRS = lowerBoundBlockCount + extraDataBlocks;
    tcProcInHRS += accessedRows;
  }
  else
  {
    // worst case scenario
    tcProcInHRS += (numProbes * effectiveTotalRowCount_);
    seqIOsInHRS = numProbes * effectiveTotalBlocks;
  }

  tcProdInHRS = accessedRows; // will be different when predicates pushed down to HRS

  // estimate Hbase Client Side (HCS)cost
  CostScalar tcProcInHCS, tcProdInHCS, tcRcvdByHCS = csZero;
  tcProcInHCS = tcProdInHRS;
  tcProdInHCS = getResultSetCardinality();
  tcRcvdByHCS = tcProdInHRS;

  // heuristics to favor serial plans for small queries
  NABoolean costParPlanSameAsSer = FALSE;
  CostScalar parPlanRcLowerLimit = 2.0 *
    ActiveSchemaDB()->getDefaults().getAsDouble(NUMBER_OF_ROWS_PARALLEL_THRESHOLD);

  if ( tcRcvdByHCS <= parPlanRcLowerLimit )
    costParPlanSameAsSer = TRUE;

  // factor in row sizes;
  CostScalar rowSize = recordSizeInKb_ * csOneKiloBytes;
  CostScalar outputRowSize = getRelExpr().getGroupAttr()->getRecordLength();
  CostScalar rowSizeFactor = scmRowSizeFactor(rowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);
  CostScalar seqIORowSizeFactor = scmRowSizeFactor(rowSize, SEQ_IO_ROWSIZE_FACTOR);
  CostScalar randIORowSizeFactor = scmRowSizeFactor(rowSize, RAND_IO_ROWSIZE_FACTOR);

  // some book keeping
  // set the field before it is being mutiplied by the row size factor
  setTuplesProcessed(tcProcInHRS+tcProcInHCS);
  setEstRowsAccessed(getDataRows());
  setNumberOfBlocksToReadPerAccess(seqIOsInHRS);

  // for HRS
  tcProcInHRS *= rowSizeFactor;
  tcProdInHRS *= rowSizeFactor;
  seqIOsInHRS *= seqIORowSizeFactor;
  randIOsInHRS *= randIORowSizeFactor;

  // for HCS
  tcProcInHCS *= rowSizeFactor;
  tcProdInHCS *= outputRowSizeFactor;
  tcRcvdByHCS *= rowSizeFactor;

  // normalize it by #region servers for HRS
  tcProcInHRS = (tcProcInHRS / HRSPartitions).getCeiling();
  tcProdInHRS = (tcProdInHRS / HRSPartitions).getCeiling();
  seqIOsInHRS = (seqIOsInHRS / HRSPartitions).getCeiling();
  randIOsInHRS = (randIOsInHRS / HRSPartitions).getCeiling();

  // normalize it by DoP for HCS
  CollIndex HCSPartitions = getEstNumActivePartitionsAtRuntime();
  if (costParPlanSameAsSer)
    HCSPartitions = 1;

  tcProcInHCS = (tcProcInHCS / HCSPartitions).getCeiling();
  tcProdInHCS = (tcProdInHCS / HCSPartitions).getCeiling();

  // compute HRS cost + HCS cost
  // Total CPU cost
  CostScalar tuplesProcessed = tcProcInHRS + tcProcInHCS;
  CostScalar tuplesProduced = tcProdInHRS + tcProdInHCS;

  // Total IO cost = seqIOsInHRS + randIOsInHRS
  // Total Msg cost = tcRcvdByHCS

  Cost* hbaseMultiProbeScanCost =
    scmCost(tuplesProcessed, tuplesProduced, tcRcvdByHCS, randIOsInHRS,
            seqIOsInHRS, csOne, rowSize, csZero, outputRowSize, csZero);

  if (isProbeCacheApplicable())
  {
    CostScalar pcCostAdjFactor = getProbeCacheCostAdjFactor();
    hbaseMultiProbeScanCost->cpScmlr().scaleByValue(pcCostAdjFactor);
  }

  // cost un-split index_scan cheaper than base table scan cost if index_scan
  // selectivity < NCM_IND_SCAN_SELECTIVITY
  if ( !(getIndexDesc()->isClusteringIndex()) && HRSPartitions == 1)
  {
    CostScalar sel = accessedRows / totalRowCount_;
    CostScalar indScanSelThreshold = 
      ActiveSchemaDB()->getDefaults().getAsDouble(NCM_IND_SCAN_SELECTIVITY);
    if ( indScanSelThreshold != -1 AND sel < indScanSelThreshold )
      hbaseMultiProbeScanCost->cpScmlr().scaleByValue(sel);
  }

  // to do : get number of index partitions:
  /*
  if (isAnIndexJoin && num_index_partitions == 1)
  {
    CostScalar sel = probes_ / totalRowCount_;
    CostScalar indJoinSelThreshold = 
      ActiveSchemaDB()->getDefaults().getAsDouble(NCM_IND_JOIN_SELECTIVITY);
    if ( sel < indJoinSelThreshold )
      hbaseMultiProbeScanCost->cpScmlr().scaleByValue(sel);
  }
  */

  return hbaseMultiProbeScanCost;
}


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
Cost*
CostMethodExchange::scmComputeOperatorCostInternal(RelExpr* op,
                                                   const PlanWorkSpace* pws,
						   Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( op	       != NULL );
  CMPASSERT( myContext != NULL );

  //-----------------------------------
  //  Downcast to an Exchange operator.
  //-----------------------------------
  Exchange* exch = (Exchange*)op;

  isOpBelowRoot_ = (*CURRSTMT_OPTGLOBALS->memo)[myContext->getGroupId()]->isBelowRoot();

  const CostScalar & numOfProbes =
    ( myContext->getInputLogProp()->getResultCardinality() ).minCsOne();

  const ReqdPhysicalProperty* rpp = myContext->getReqdPhysicalProperty();

  sppForMe_ = (PhysicalProperty *) myContext->getPlan()->getPhysicalProperty();

  //------------------------------------------------------------------------
  //  If we have not synthesized physical properties, we are going down
  // the tree, return an empty cost for now. Is this OK??
  //------------------------------------------------------------------------
  if ( NOT sppForMe_ )
    return  new STMTHEAP Cost();

  //------------------------------------------------------------
  //  Get the physical properties for the child of the Exchange.
  //------------------------------------------------------------
  const PhysicalProperty* sppForChild =
    myContext->getPhysicalPropertyOfSolutionForChild(0);

  sppForChild_ = (PhysicalProperty*) sppForChild;
  numOfProbes_ = (CostScalar )numOfProbes;

  NABoolean executeInDP2 = sppForChild->executeInDP2();

  const PartitioningFunction* const childPartFunc =
    sppForChild->getPartitioningFunction();

  PartitioningFunction* const myPartFunc =
    myContext->getPlan()->getPhysicalProperty()->getPartitioningFunction();

  const PartitioningFunction* const bottomPartFunc = exch->getBottomPartitioningFunction();  

  //--------------------------------------------------------------------------
  //  Compute number of consumer processes associated with this Exchange.
  //--------------------------------------------------------------------------
  const CostScalar& numOfConsumers  = ((NodeMap *)(myPartFunc->getNodeMap()))->
                                    getNumActivePartitions();
  const CostScalar& numOfPartitions = ((NodeMap *)(childPartFunc->getNodeMap()))->
                                                getEstNumActivePartitionsAtRuntime();
  const CostScalar& numOfProducers  = MINOF( numOfPartitions ,sppForChild->getCurrentCountOfCPUs()  );

  //---------------------------------------------
  //  Determine number of rows produced by child.
  //---------------------------------------------
  EstLogPropSharedPtr inputLP       = myContext->getInputLogProp();
  CMPASSERT( inputLP ); 
  EstLogPropSharedPtr childOutputLP = exch->child(0).outputLogProp(inputLP);
  const CostScalar& totalRowCount = (childOutputLP->getResultCardinality()).getCeiling();
  //---------------------------------------------------------------

  //---------------------------------------------------------------
  //  Exchange operator's number of streams is parent's degree of
  // parallelism (i.e. number of concurrently executing consumers).
  //---------------------------------------------------------------
  countOfStreams = Lng32(numOfConsumers.getValue());

  //---------------------------------------------
  // set output resultset cardinality  as active streams, this value
  // will be used in ColStatDescList::getCardOfBusiestStream() if
  // partitioning key is a random number.
  //---------------------------------------------
  long randomFix = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_26);
  if (NOT executeInDP2 && (randomFix != 0))
  {
    CostScalar estDop = totalRowCount;
    CostScalar maxCard = childOutputLP->getMaxCardEst();
    // use the max cardinality, somewhat corrected for risk by taking
    // geometric mean of card and maxCard.
    if (maxCard != -1)
    {
      estDop = sqrt((estDop * maxCard).getValue());
      estDop.getFloor();
    }
    // if estDop > countOfStreams, then take countOfStreams
    estDop = MINOF((CostScalar)countOfStreams, estDop);
    myPartFunc->setActiveStreams(estDop);
  }

  //---------------------------------------------------------------------
  //  Compute the number of "up" tuples sent.
  // "up" tuples sent are the number of tuples sent from the child to the parent.
  //---------------------------------------------------------------------
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

  CostScalar espFixUpWeight = ActiveSchemaDB()->getDefaults().getAsDouble(NCM_ESP_FIXUP_WEIGHT);
  NABoolean espStartFix = (CmpCommon::getDefault(NCM_ESP_STARTUP_FIX) == DF_ON);
  CostScalar espFixupCost = csZero;
  if (espStartFix)
    espFixupCost = espFixUpWeight * numOfConsumers;
  else
    espFixupCost = espFixUpWeight * (numOfProducers + numOfConsumers);
  CostScalar tuplesProcessed = csZero;
  CostScalar tuplesProduced = csZero;
  CostScalar upTuplesSent = csZero;
  CostScalar origUpTuplesSent = csZero;
  NADefaults &defs1 = ActiveSchemaDB()->getDefaults();

  if (NOT executeInDP2)
  {
    exch->computeBufferLength(myContext,
                            numOfConsumers,
                            numOfProducers,
                            upMessageBufferLength_,
                            downMessageBufferLength_);
    upTuplesSent = scmComputeUpTuplesSent(myContext,
                                          exch,
                                          myPartFunc,
                                          childPartFunc,
                                          numOfConsumers,
					  numOfProducers);
    origUpTuplesSent = upTuplesSent;
  }
  else
  {
    upMessageBufferLength_= messageBufferSizeInKb;
    downMessageBufferLength_= csOne;

    if (childPartFunc)
    {
      const LogPhysPartitioningFunction *lpf = childPartFunc->castToLogPhysPartitioningFunction();
      if (lpf != NULL AND lpf->getUsePapa())
      {
	//PAPA (SplitTop)
	upTuplesSent = totalRowCount/numOfConsumers;

	// Is merging of sorted streams possibly needed? 
	// Add in tuplesProcessed to account for the merging of sorted streams.
	isMergeNeeded_ = (sppForMe_->getSortOrderType() != DP2_SOT) &&
	  (NOT sppForMe_->getSortKey().isEmpty()); 
        // get weight for merging of sorted streams by each ESP
        CostScalar mergeFactor =
        ActiveSchemaDB()->getDefaults().getAsDouble(NCM_EXCH_MERGE_FACTOR);
	if ( isMergeNeeded_ AND numOfProducers > numOfConsumers )
	{
          tuplesProcessed = upTuplesSent * mergeFactor;
	}
	// Fix for the serial plan issue with split tops.
	// This was an issue seen with TPCH Q1, the serial plan looks 
	// like a very good plan, but for some reason performs poorly. Bob W. 
	// thinks it may be the "wave pattern" issue. Adjust costs by
	// adding "fixup" costs to compensate.
	if ( (CmpCommon::getDefault(COMP_BOOL_202) == DF_OFF) AND
             (numOfConsumers == csOne) AND 
	     (numOfProducers > numOfConsumers) )
	  //	     (rpp->getPlanExecutionLocation() == EXECUTE_IN_MASTER) ) //isOpBelowRoot_)
	  upTuplesSent += espFixupCost;
      }
    }
  }

  if (NOT executeInDP2)  
  {
    // Fixup Costs for esp_exchanges.
    upTuplesSent += espFixupCost;
  }

  CostScalar inputRowSize = exch->child(0).getGroupAttr()->getRecordLength();
  CostScalar rowSizeFactor = scmRowSizeFactor(inputRowSize);
  tuplesProcessed *= rowSizeFactor;
  upTuplesSent *= rowSizeFactor;

  NABoolean ndcsFixOff = (CmpCommon::getDefault(NCM_EXCH_NDCS_FIX) == DF_OFF);
  if (ndcsFixOff)
    origUpTuplesSent = upTuplesSent;

  CostScalar adj1 = defs1.getAsLong(COMP_INT_62);
  CostScalar parAdj = defs1.getAsDouble(NCM_PAR_ADJ_FACTOR);
  if (adj1 == csZero)
    adj1=1000000;
   if ( (numOfConsumers == csOne) AND
        (numOfProducers > numOfConsumers) AND
      isOpBelowRoot_  AND
      origUpTuplesSent > adj1 &&
      parAdj > 0)
   {
      upTuplesSent = parAdj ;
   }

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

    if (!childMatchesPartitioning &&
	(CmpCommon::getDefault(NCM_PAR_GRPBY_ADJ) == DF_ON))
    {
      CostScalar grpByAdjFactor = 
	ActiveSchemaDB()->getDefaults().getAsDouble(ROBUST_PAR_GRPBY_EXCHANGE_FCTR);
      tuplesProcessed *= grpByAdjFactor;
      upTuplesSent *= grpByAdjFactor;
    }
  }

  Cost* exchangeCost =  
    scmCost(tuplesProcessed, tuplesProduced, upTuplesSent, csZero, csZero, numOfProbes,
	    inputRowSize, csZero, inputRowSize, csZero);

  return exchangeCost;

} // CostMethodExchange::scmComputeOperatorCostInternal()
//<pb>

//==============================================================================
//  Compute number of tuples sent from child of a specified exchange operator
// up to its parent.
//
// Input:
//  parentContext             -- pointer to optimization context for specified
//                                Exchange operator.
//
//  exch                      -- pointer to specified Exchange operator.
//
//
//  parentPartFunc            -- pointer to parent's partitioning function.
//
//  childPartFunc             -- pointer to child's partitioning function.
//
//  numOfConsumers            -- number parent processes actually receiving
//                                messages.
// Return:
//  Number of tuples sent from child up to parent.
//
//==============================================================================
CostScalar
CostMethodExchange::scmComputeUpTuplesSent(
                          const Context*              parentContext,
		          Exchange*                   exch,
                          const PartitioningFunction* parentPartFunc,
                          const PartitioningFunction* childPartFunc,
                          const CostScalar &          numOfConsumers,
                          const CostScalar &          numOfProducers) const
{
  //----------------------------------------------------------------------
  // Defensive programming.
  //----------------------------------------------------------------------
  CMPASSERT( parentContext  != NULL );
  CMPASSERT( exch	    != NULL );
  CMPASSERT( parentPartFunc != NULL );
  CMPASSERT( childPartFunc  != NULL );

  //---------------------------------------------
  //  Determine number of rows produced by child.
  //---------------------------------------------
  EstLogPropSharedPtr inputLP       = parentContext->getInputLogProp();
  CMPASSERT( inputLP );

  EstLogPropSharedPtr childOutputLP = exch->child(0).outputLogProp(inputLP);
  const CostScalar& totalRowCount = (childOutputLP->getResultCardinality()).getCeiling();
  const CostScalar& rowCountPerConsumer = (totalRowCount/numOfConsumers).getCeiling(); 
  const CostScalar& rowCountPerProducer = (totalRowCount/numOfProducers).getCeiling(); 

  // Determine number of probes and whether there are any outer references
  // (i.e. probe values)
  const CostScalar& noOfProbes = ( inputLP->getResultCardinality() ).minCsOne();
  ValueIdSet externalInputs( exch->getGroupAttr()->getCharacteristicInputs() );
  ValueIdSet outerRefs;
  externalInputs.getOuterReferences( outerRefs );

  CostScalar upRowsPerConsumer, upRowsPerProducer;

  // Calculate number of rows each consumer will receive.
  //------------------------------------------------------------------------
  if (CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting())
  {
    upRowsPerConsumer = childOutputLP->
      getCardOfBusiestStream(parentPartFunc,
			     (Lng32)numOfConsumers.getValue(),
			     exch->getGroupAttr(),
			     (Lng32)numOfConsumers.getValue());
    // assume number of consumers = number of CPUs;
    // it is only used for round robin pf. where skew is not an issue

    upRowsPerProducer = childOutputLP->
      getCardOfBusiestStream(childPartFunc,
			     (Lng32)numOfProducers.getValue(),
			     exch->getGroupAttr(),
			     (Lng32)numOfProducers.getValue());
  }
  else
  {
    upRowsPerConsumer = rowCountPerConsumer;
    upRowsPerProducer = rowCountPerProducer;
  }

  CostScalar upRowsPerBusiestStream = MAXOF(upRowsPerConsumer, upRowsPerProducer);
    
  //------------------------------------------------------------------------
  // For broadcast replication, each child will send all rows to all consumers.
  // For no broadcast replication underneath a materialize that is not
  // passing the probes through, each consumer will read the rows of all
  // the children. This is similar to broadcast replication, so what we
  // want to do here is the same - multiply the number of rows by the
  // by the number of consumers.
  //------------------------------------------------------------------------
  if (parentPartFunc->isAReplicateViaBroadcastPartitioningFunction()
      OR (parentPartFunc->isAReplicateNoBroadcastPartitioningFunction()
	  AND noOfProbes == 1	    // 1 probe, but
	  AND outerRefs.isEmpty() // no probe values - must be under a materialize
	  )
      )
  {
    //---------------------------------------------------------------------
    // All producers send all rows to all consumers.
    //---------------------------------------------------------------------
    upRowsPerBusiestStream *= numOfConsumers;
  }

  return upRowsPerBusiestStream;

} // CostMethodExchange::scmComputeUpTuplesSent()

//<pb>
// -----------------------------------------------------------------------
// CostMethodSort scmComputeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodSort::scmComputeOperatorCostInternal(RelExpr* op,
                                               const PlanWorkSpace* pws,
					       Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  sort_ = (Sort*) op;

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  CostScalar rowCount(csZero);
  if ((CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting()) &&
        (partFunc_ != NULL) )
  {
    rowCount =
      myLogProp_->getCardOfBusiestStream(partFunc_,
					 countOfStreams_,
					 ga_,
					 countOfAvailableCPUs_);
  }
  else
  {
  // Row count a single probe on one instance of this sort is processing.
    rowCount = myRowCount_ / countOfStreams_;
  }

  // need to do nlog(n) operations to sort n rows.
  // log function will give us the natural logarithm.
  // So, to get log to a different base, we have to compute log(n)/log(base)
  CostScalar tuplesProcessed = rowCount * log(rowCount.value());
  CostScalar logBase = ActiveSchemaDB()->getDefaults().getAsDouble(NCM_NUM_SORT_RUNS);
  tuplesProcessed /= log(logBase.getValue());
  CostScalar tuplesProduced = rowCount;

  CostScalar inputRowSize = sort_->child(0).getGroupAttr()->getRecordLength();
  CostScalar rowSizeFactor = scmRowSizeFactor(inputRowSize);

  // Compute sort overflow costs.
  //  tuplesProcessed += scmComputeOverflowCost(rowCount, inputRowSize);

  CostScalar overflowCost = scmComputeOverflowCost(rowCount, inputRowSize);

  // Factor in rowsize.
  tuplesProcessed *= rowSizeFactor;
  tuplesProduced *= rowSizeFactor;

  // Find out the number of cpus and number of fragments per cpu.
  //  long cpuCount, fragmentsPerCPU;
  //  determineCpuCountAndFragmentsPerCpu( cpuCount, fragmentsPerCPU );

  // We are using tuplesSent as a placeholder for overflow costs as it would be
  // easier for debugging by differentiating between regular tuples processed 
  // and overflow costs, rather than lumping them all togther in 
  // tuplesProcessed. tuplesSent is zero for all operators except exchange,
  // so it makes sense to use this instead of adding a new field.
  // It does not make a difference to the overall cost computation.
  Cost* sortCost = 
    scmCost(tuplesProcessed, tuplesProduced, overflowCost, csZero, csZero, noOfProbesPerStream_,
	    inputRowSize, csZero, inputRowSize, csZero);

#ifndef NDEBUG
  if ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON )
    {
      pfp = stdout;
      fprintf(pfp,"SORT::scmComputeOperatorCostInternal()\n");
      sortCost->print(pfp);
      fprintf(pfp, "Elapsed time: ");
      fprintf(pfp,"%f", sortCost->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
    }
#endif

  return sortCost;

}  // CostMethodSort::scmComputeOperatorCostInternal()

//==============================================================================
//  Compute the overflow cost of the sort.
//  tuple counts alone may not be enough to capture the true costs.
//
// Inputs:
//  numInputTuples -- input cardinality
//  inputRowSize -- size of the input row.
// Output:
//  none.
//
// Return:
//  Overflow cost
//
//==============================================================================
CostScalar
CostMethodSort::scmComputeOverflowCost( CostScalar numInputTuples, CostScalar inputRowSize )
{
  NABoolean sortOverFlowCosting = 
    (CmpCommon::getDefault(NCM_SORT_OVERFLOW_COSTING) == DF_ON);

  if (sortOverFlowCosting == FALSE)
    return 0;

  CostScalar inputRowSizeFactor = scmRowSizeFactor(inputRowSize);

  CostScalar sortBufferSizeInBytes = ActiveSchemaDB()->getDefaults().getAsDouble(GEN_SORT_MAX_BUFFER_SIZE);
  CostScalar numSortBuffers = ActiveSchemaDB()->getDefaults().getAsDouble(GEN_SORT_MAX_NUM_BUFFERS);

  CostScalar memoryAvailable = sortBufferSizeInBytes * numSortBuffers;
  CostScalar memoryUsed = numInputTuples * inputRowSize;
  
  // Pointers to the sort keys (32 bytes) are stored in an array. The array
  // size is limited by the 128MB segment allocation limit. 
  // Therefore, the number of sort key pointers is limited to 128/32=4MB
  // If this changes, maxSortKeys will need to be updated.
  CostScalar maxSortKeys = CostScalar(4.0) * csOneKiloBytes * csOneKiloBytes;

  if (memoryUsed <= memoryAvailable && numInputTuples <= maxSortKeys)
    return 0;

  // If overflow occurs during sort, the whole table is overflowed to disk.
  // The factor of 2 comes from having to write overflow tuples to disk and 
  // read back the overflow tuples from disk.
  CostScalar overflow = CostScalar(2.0) * numInputTuples * inputRowSizeFactor;

  return overflow;

} // CostMethodSort::scmComputeOverflowCost

//<pb>

/**********************************************************************/
/*                                                                    */
/*                         CostMethodSortGroupBy                      */
/*                                                                    */
/**********************************************************************/
//<pb>
//==============================================================================
//  Compute operator cost for a specified SortGroupBy operator.
//
// Input:
//  op             -- pointer to specified SortGroupBy operator.
//
//  myContext      -- pointer to optimization context for this SortGroupBy
//                     operator.
//
// Output:
//  countOfStreams -- degree of parallelism for this SortGroupBy operator.
//
// Return:
//  Pointer to computed cost object for this SortGroupBy operator.
//
//==============================================================================
Cost*
CostMethodSortGroupBy::scmComputeOperatorCostInternal(RelExpr* op,
                                                      const PlanWorkSpace* pws,
						      Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

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
  // Set the count of streams to an invalid value (-1) to force us to
  // recost on the way back up.
  // ---------------------------------------------------------------------
  if(rpp_->executeInDP2() AND
    (NOT context_->getPlan()->getPhysicalProperty()))
  {
    countOfStreams = -1;
    return generateZeroCostObject();
  }

  // ---------------------------------------------------------------------
  // Make sure rowcount is at least group count to prevent absurdity in
  // results.
  // ---------------------------------------------------------------------
  rowCountPerStream_ = MAXOF(rowCountPerStream_,groupCountPerStream_);

  // The input to the SortGroupBy is always sorted, either the input is
  // already naturally sorted or there is an explicit Sort operator below it.
  
  CostScalar tuplesProcessed = rowCountPerStream_;
  CostScalar tuplesProduced = myRowCountPerStream_ ;

  // ---------------------------------------------------------------------
  // tupleProduced can never be more than tupleProcessed.
  // This thing can happen sometimes especially for partial group bys since 
  // cardinality estimates do not distinguish between partial 
  // and full group bys.
  // ---------------------------------------------------------------------
  tuplesProduced = MINOF(tuplesProcessed, tuplesProduced);
  
  // Make the SortGroupBy cheaper than the HashGroupBy if the input is
  // naturally sorted. If there is an explicit Sort below, then the overall cost 
  // of the SortGroupBy could be more than the HashGroupBy.
  // Assume that SortGroupBy is 30% faster than HashGroupBy if the input is already sorted.
  double sortToHashGroupFactor =
    ActiveSchemaDB()->getDefaults().getAsDouble(NCM_SGB_TO_HGB_FACTOR);

  tuplesProcessed *= sortToHashGroupFactor;

  CostScalar inputRowSize = gb_->child(0).getGroupAttr()->getCharacteristicOutputs().getRowLength();
  CostScalar outputRowSize = myVis().getRowLength();
  CostScalar inputRowSizeFactor = scmRowSizeFactor(inputRowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  tuplesProcessed *= inputRowSizeFactor;
  tuplesProduced *= outputRowSizeFactor;

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

    if (!myGroupingMatchesPartitioning &&
	(CmpCommon::getDefault(NCM_PAR_GRPBY_ADJ) == DF_ON))
    {
      CostScalar grpByAdjFactor = ActiveSchemaDB()->getDefaults().getAsDouble(ROBUST_PAR_GRPBY_LEAF_FCTR);
      tuplesProcessed *= grpByAdjFactor;
      tuplesProduced *= grpByAdjFactor;
    }
  }

  // ---------------------------------------------------------------------
  // Synthesize and return the cost object.
  // ---------------------------------------------------------------------
  Cost* sortGroupByCost = 
        scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    inputRowSize, csZero, outputRowSize, csZero);

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"SORTGROUPBY::scmComputeOperatorCostInternal()\n");
    fprintf(pfp,"child0RowCount=%g,groupCount=%g,myRowCount=%g\n",
                                 child0RowCount_.toDouble(),groupCount_.toDouble(),myRowCount_.toDouble());
    sortGroupByCost->getScmCplr().print(pfp);
    fprintf(pfp, "Elapsed time: ");
    fprintf(pfp,"%f", sortGroupByCost->
	    convertToElapsedTime(myContext->getReqdPhysicalProperty()).
	    value());
    fprintf(pfp,"\n");
  }
#endif

  return sortGroupByCost;

}  // CostMethodSortGroupBy::scmComputeOperatorCostInternal().

/**********************************************************************/
/*                                                                    */
/*                         CostMethodShortCutGroupBY                  */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodShortCutGroupBy::scmComputeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodShortCutGroupBy::scmComputeOperatorCostInternal
                                             (RelExpr* op,
                                              const PlanWorkSpace* pws,
                                              Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

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

  // ---------------------------------------------------------------------
  // ShortCutGroupBy execution can be short-circuited after we found one 
  // row to have satisfied the any-true aggregate expression. 
  // This short circuit can even lead to the cancellation of the execution 
  // of the operator's child. 
  // Assume that on average you process 50% of child rows before a 
  // short_circuit. If no short_circuit it is same as regular sortGroupBy,
  // no harm either. 
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

  CostScalar tuplesProcessed = csZero;
  CostScalar tuplesProduced = csZero;

  if((optype==ITM_ANY_TRUE) OR (optype==ITM_ANY_TRUE_MAX))
  { 
    tuplesProcessed = (rowCountPerStream_ * 0.5);
    tuplesProduced = myRowCountPerStream_ ;
  }
  else  // MIN/MAX optimization
  {
    //it only passes along a single row
    CMPASSERT(op->getFirstNRows()==1);
    tuplesProcessed = tuplesProduced = 1;
  }

  CostScalar inputRowSize = gb_->child(0).getGroupAttr()->getCharacteristicOutputs().getRowLength();
  CostScalar outputRowSize = myVis().getRowLength();
  CostScalar inputRowSizeFactor = scmRowSizeFactor(inputRowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  tuplesProcessed *= inputRowSizeFactor;
  tuplesProduced *= outputRowSizeFactor;

  // ------------------------------------------------
  // Synthesize and return the cost object.
  // ------------------------------------------------
  Cost* shortCutGBCost = 
        scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    inputRowSize, csZero, outputRowSize, csZero);

  // ---------------------------------------------------------------------
  // For debugging.
  // ---------------------------------------------------------------------
#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"ShortCutGroupBy::scmComputeOperatorCostInternal()\n");
    fprintf(pfp,"childRowCount=%g",child0RowCount_.toDouble());
    shortCutGBCost->getScmCplr().print(pfp);
    fprintf(pfp, "Elapsed time: ");
    fprintf(pfp,"%f", shortCutGBCost->convertToElapsedTime(myContext->getReqdPhysicalProperty()).value());
    fprintf(pfp,"\n");
  }
#endif

  return shortCutGBCost;

}  // ShortCutGroupBy::scmComputeOperatorCostInternal().

/**********************************************************************/
/*                                                                    */
/*                         CostMethodHashGroupBy                      */
/*                                                                    */
/**********************************************************************/
//<pb>
//==============================================================================
//  Compute operator cost for a specified HashGroupBy operator.
//
// Input:
//  op             -- pointer to specified HashGroupBy operator.
//
//  myContext      -- pointer to optimization context for this HashGroupBy
//                     operator.
//
// Output:
//  countOfStreams -- degree of parallelism for this HashGroupBy operator.
//
// Return:
//  Pointer to computed cost object for this HashGroupBy operator.
//
//==============================================================================
Cost*
CostMethodHashGroupBy::scmComputeOperatorCostInternal(RelExpr*       op,
                                                      const PlanWorkSpace* pws,
                                                      Lng32&          countOfStreams)
{
  const Context* myContext = pws->getContext();

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

  // ---------------------------------------------------------------------
  // Make sure rowcount is at least group count to prevent absurdity in
  // results.
  // ---------------------------------------------------------------------
  rowCountPerStream_ = MAXOF(rowCountPerStream_,groupCountPerStream_);

  CostScalar tuplesProcessed = rowCountPerStream_; 
  CostScalar tuplesProduced = myRowCountPerStream_;
  // ---------------------------------------------------------------------
  // tupleProduced can never be more than tupleProcessed.
  // This thing can happen sometimes especially for partial group bys since
  // cardinality estimates do not distinguish between partial
  // and full group bys.
  // ---------------------------------------------------------------------
  tuplesProduced = MINOF(tuplesProcessed, tuplesProduced);

  // Special things for Dp2 (push down) hash groupby.
  // I think this is not required that is why it is commented.
  if(rpp_->executeInDP2())
  {
    CostScalar groupCountInMemory =
      ActiveSchemaDB()->getDefaults().getAsLong(MAX_DP2_HASHBY_GROUPS);
    groupCountInMemory = MINOF(groupCountInMemory, groupCountPerStream_);

    //----------------------------------
    // Average number of rows per group.
    //----------------------------------
    CostScalar rowsPerGroup = (rowCountPerStream_ / groupCountPerStream_);
    CostScalar rowsNotTouched = 
      rowCountPerStream_ - groupCountInMemory * rowsPerGroup;
    //    tuplesProduced = groupCountInMemory + rowsNotTouched;
    if (isUnderNestedJoin_)
    {
      tuplesProduced *= countOfStreams;
      tuplesProcessed *= countOfStreams;
      //    tuplesProduced = groupCountInMemory + rowsNotTouched;
      //    tuplesProcessed += groupCountInMemory * rowsPerGroup;
    }
  }

  CostScalar inputRowSize = gb_->child(0).getGroupAttr()->getCharacteristicOutputs().getRowLength();
  CostScalar outputRowSize = myVis().getRowLength();
  CostScalar inputRowSizeFactor = scmRowSizeFactor(inputRowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  // tuplesProcessed += scmComputeOverflowCost(tuplesProcessed, inputRowSize,
  //                                           tuplesProduced, outputRowSize);
  GroupByAgg * groupByNode = (GroupByAgg *) op;
  CostScalar overflowCost;

  if (rpp_->executeInDP2() || groupByNode->isAPartialGroupByLeaf())
    overflowCost = csZero;
  else
    overflowCost = scmComputeOverflowCost(tuplesProcessed, inputRowSize,
                                          tuplesProduced, outputRowSize);
  tuplesProcessed *= inputRowSizeFactor;
  tuplesProduced *= outputRowSizeFactor;

  // If this is a partial Group By leaf in an ESP adjust it's cost
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

    if (!myGroupingMatchesPartitioning &&
	(CmpCommon::getDefault(NCM_PAR_GRPBY_ADJ) == DF_ON))
    {
      CostScalar grpByAdjFactor = ActiveSchemaDB()->getDefaults().getAsDouble(ROBUST_PAR_GRPBY_LEAF_FCTR);
      tuplesProcessed *= grpByAdjFactor;
      overflowCost *= grpByAdjFactor;
      tuplesProduced *= grpByAdjFactor;
    }
  }

  //----------------------------------------
  //  Synthesize and return the cost object.
  //----------------------------------------
  
  // We are using tuplesSent as a placeholder for overflow costs as it would be
  // easier for debugging by differentiating between regular tuples processed 
  // and overflow costs, rather than lumping them all togther in 
  // tuplesProcessed. tuplesSent is zero for all operators except exchange,
  // so it makes sense to use this instead of adding a new field.
  // It does not make a difference to the overall cost computation.
  Cost* hashGroupByCost = 
            scmCost(tuplesProcessed, tuplesProduced, overflowCost, csZero, csZero, noOfProbesPerStream_,
	    inputRowSize, csZero, outputRowSize, csZero);

#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"HASHGROUPBY::scmComputeOperatorCostInternal()\n");
    fprintf(pfp,"child0RowCount=%g,groupCount=%g,myRowCount=%g\n",
                                 child0RowCount_.toDouble(),groupCount_.toDouble(),myRowCount_.toDouble());
    hashGroupByCost->getScmCplr().print(pfp);
    fprintf(pfp, "Elapsed time: ");
    fprintf(pfp,"%f", hashGroupByCost->
	    convertToElapsedTime(myContext->getReqdPhysicalProperty()).value());
      fprintf(pfp,"\n");
  }
#endif

  return hashGroupByCost;
}

 // CostMethodHashGroupBy::scmComputeOperatorCostInternal()
//<pb>

//==============================================================================
//  Compute the overflow cost of the hash group by.
//  tuple counts alone may not be enough to capture the true costs.
//
// Inputs:
//  numInputTuples -- input cardinality
//  inputRowSize -- size of the input row.
//  numOutputTuples -- output cardinality
//  outputRowSize -- size of the output row.
// Output:
//  none.
//
// Return:
//  Overflow cost
//
//==============================================================================
CostScalar
CostMethodHashGroupBy::scmComputeOverflowCost( CostScalar numInputTuples, CostScalar inputRowSize, CostScalar numOutputTuples, CostScalar outputRowSize )
{
  NABoolean hashGroupByOverFlowCosting = 
    (CmpCommon::getDefault(NCM_HGB_OVERFLOW_COSTING) == DF_ON);

  if (hashGroupByOverFlowCosting == FALSE)
    return 0;

  CostScalar inputRowSizeFactor = scmRowSizeFactor(inputRowSize);

  // Use the OCM value (200 MB) for memory limit per cpu/stream in bytes.
  // The value in the defaults table is 204800, in KB.
  CostScalar memoryAvailableInKB = ActiveSchemaDB()->getDefaults().getAsDouble(BMO_MEMORY_SIZE);

  // Convert to bytes as all computations here are in bytes.
  CostScalar memoryAvailable = memoryAvailableInKB * csOneKiloBytes;

  CostScalar memoryUsed = numOutputTuples * outputRowSize;

  if (memoryUsed <= memoryAvailable)
    return 0;

  CostScalar fractionOverflow = CostScalar(1.0) - memoryAvailable/memoryUsed;

  // The factor of 2 comes from having to write overflow tuples to disk and 
  // read back the overflow tuples from disk.
  CostScalar overflow = CostScalar(2.0) * fractionOverflow *
    numInputTuples * inputRowSizeFactor;

  return overflow;

} // CostMethodHashGroupBy::scmComputeOverflowCost

/**********************************************************************/
/*                                                                    */
/*                         CostMethodHashJoin                         */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodHashJoin::scmComputeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodHashJoin::scmComputeOperatorCostInternal(RelExpr* op,
                                                   const PlanWorkSpace* pws,
						   Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  hj_ = (HashJoin*) op;

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  CostScalar outputJoinCard, tuplesProcessed, commutativeTuplesProcessed;
  CostScalar overflowCost, commutativeOverflowCost;

  //COMP_INT_80 = 0 means fix is OFF.
  //            = 1 means only 1736 fix is ON.
  //            = 2 means only 1769 fix is ON.
  //           >= 3 means 1736 and 1769 fixes are ON.
  Lng32 compInt80 = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_80);


  // fix for 1736 
  // cost of serial HJ is less than cost of parallel one because 
  // values of equiJnRowCountPerStream_ and myRowCount_ are different.
  // Fix is to use the same value for both (equiJnRowCountPerStream_)
  if (compInt80 == 1 OR compInt80 >= 3)
    outputJoinCard = equiJnRowCountPerStream_;
  else { // will be removed after perf testing
  if (CURRSTMT_OPTDEFAULTS->incorporateSkewInCosting() && countOfStreams > 1)
  {
  /*    
	  CostScalar child0CardOfFreqValue =
	  ((child0RowCountPerStream_ * countOfStreams - child0RowCount_)/
	  (countOfStreams - 1));

	  CostScalar child1CardOfFreqValue =
	  ((child1RowCountPerStream_ * countOfStreams - child1RowCount_)/
	  (countOfStreams - 1));

	  outputJoinCard = MAXOF(child0CardOfFreqValue * child1CardOfFreqValue,
	  (myRowCount_/countOfStreams).getCeiling());
	  outputJoinCard = MINOF(outputJoinCard, myRowCount_);
   */
    outputJoinCard = equiJnRowCountPerStream_;
  }
  else
  {
    outputJoinCard = (myRowCount_/countOfStreams).getCeiling();
  } 
} // will be removed after perf testing

  CostScalar probeTuplesPerStream = child0RowCountPerStream_;
  CostScalar buildTuplesPerStream = child1RowCountPerStream_;
  CostScalar broadcastTuplesPerStream;

  if (ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_95) == 0)
    broadcastTuplesPerStream = child1RowCountPerStream_;
  else
    broadcastTuplesPerStream = child1RowCount_;

  CostScalar probeTuples = child0RowCount_;
  CostScalar buildTuples = child1RowCount_;

  CostScalar probeRowSize = child0RowLength_;
  CostScalar buildRowSize = child1RowLength_;
  CostScalar outputRowSize = hj_->getGroupAttr()->getCharacteristicOutputs().getRowLength();

  CostScalar probeSize = probeTuples * probeRowSize;
  CostScalar buildSize = buildTuples * buildRowSize;

  CostScalar probeRowSizeFactor = scmRowSizeFactor(probeRowSize);
  CostScalar buildRowSizeFactor = scmRowSizeFactor(buildRowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  // this is not true when going down the tree because child1PartFunc_
  // is always NULL even for Type1 plans.
  NABoolean type2Join = ((child1PartFunc_ == NULL) OR
			 child1PartFunc_->isAReplicateViaBroadcastPartitioningFunction());

  NABoolean buildSizeSmaller = FALSE;
  if (buildSize <= probeSize)
    buildSizeSmaller = TRUE;

  // In SCM, we always build with the smaller side if possible.
  // If the build side is larger than the probe side, we will not pick this 
  // plan.
  // In certain cases like (left or right) outer joins, there is no choice in
  // picking the left and left side of a join.
  if (type2Join)
  {
      tuplesProcessed = broadcastTuplesPerStream * buildRowSizeFactor + 
	probeTuplesPerStream * probeRowSizeFactor;
      overflowCost = 
	scmComputeOverflowCost(broadcastTuplesPerStream, buildRowSize,
			       probeTuplesPerStream, probeRowSize);
      commutativeTuplesProcessed = probeTuples * probeRowSizeFactor +
	buildTuplesPerStream * buildRowSizeFactor;
      commutativeOverflowCost = 
	scmComputeOverflowCost(probeTuples, probeRowSize,
                               buildTuplesPerStream, buildRowSize);
    if (NOT hasEquiJoinPred_)
    {
      if ( compInt80 < 2) { // will be removed after perf testing
      // cross product type2 join.
      tuplesProcessed += (probeTuplesPerStream * probeRowSizeFactor *
			  broadcastTuplesPerStream * buildRowSizeFactor);
      commutativeTuplesProcessed += (buildTuplesPerStream * buildRowSizeFactor *
				     probeTuples * probeRowSizeFactor);
      } // will be removed after perf testing
    }
  }
  else
  {
    // Regular type1 hash join.
    tuplesProcessed = buildTuplesPerStream * buildRowSizeFactor + 
      probeTuplesPerStream * probeRowSizeFactor;
    overflowCost = 
      scmComputeOverflowCost(buildTuplesPerStream, buildRowSize,
			     probeTuplesPerStream, probeRowSize);
    commutativeTuplesProcessed = probeTuplesPerStream * probeRowSizeFactor + 
      buildTuplesPerStream * buildRowSizeFactor;
    commutativeOverflowCost = 
      scmComputeOverflowCost(probeTuplesPerStream, probeRowSize,
                             buildTuplesPerStream, buildRowSize);
  }
  
  NABoolean commutativeCostSame = 
    (tuplesProcessed + overflowCost == commutativeTuplesProcessed + commutativeOverflowCost);
  NABoolean commutativeCostCheaper = 
    (tuplesProcessed + overflowCost > commutativeTuplesProcessed + commutativeOverflowCost);

  if (buildSizeSmaller)
  {
      // Ensure that this is cheaper than the commutative operation.
    if (commutativeCostSame)
    {
      tuplesProcessed = tuplesProcessed * 0.999;
      overflowCost = overflowCost * 0.999;
    }
    else if (commutativeCostCheaper)
    {
      tuplesProcessed = commutativeTuplesProcessed;
      overflowCost = commutativeOverflowCost;
    }
  }
  else
  {
      // Ensure that this is NOT cheaper than the commutative operation.
    if (commutativeCostSame)
    {
      tuplesProcessed = tuplesProcessed / 0.999;
      overflowCost = overflowCost / 0.999;
    }
    else if (NOT commutativeCostCheaper)
    {
      tuplesProcessed = commutativeTuplesProcessed;
      overflowCost = commutativeOverflowCost;
    }
  }
  
  CostScalar tuplesProduced = outputJoinCard * outputRowSizeFactor;

  //----------------------------------------
  //  Synthesize and return the cost object.
  //----------------------------------------
  
  // We are using tuplesSent as a placeholder for overflow costs as it would be
  // easier for debugging by differentiating between regular tuples processed 
  // and overflow costs, rather than lumping them all togther in 
  // tuplesProcessed. tuplesSent is zero for all operators except exchange,
  // so it makes sense to use this instead of adding a new field.
  // It does not make a difference to the overall cost computation.
  Cost* hashJoinCost = 
    scmCost(tuplesProcessed, tuplesProduced, overflowCost, csZero, csZero, noOfProbesPerStream_,
	    probeRowSize, buildRowSize, outputRowSize, csZero);

#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"HASHJOIN::scmComputeOperatorCostInternal()\n");
    fprintf(pfp,"child0RowCount=%g,child1RowCount=%g,myRowCount=%g\n",
                                 child0RowCount_.toDouble(),child1RowCount_.toDouble(),myRowCount_.toDouble());
    hashJoinCost->getScmCplr().print(pfp);
    fprintf(pfp, "Elapsed time: ");
    fprintf(pfp,"%f", hashJoinCost->
	    convertToElapsedTime(myContext->getReqdPhysicalProperty()).value());
      fprintf(pfp,"\n");
  }
#endif

  return hashJoinCost;
}

//==============================================================================
//  Compute the overflow cost of the hash join.
//  tuple counts alone may not be enough to capture the true costs.
//
// Inputs:
//  numBuildTuples -- build side cardinality
//  buildRowSize -- size of the build side row.
//  numProbeTuples -- probe side cardinality
//  probeRowSize -- size of the probe side row.
// Output:
//  none.
//
// Return:
//  Overflow cost
//
//==============================================================================
CostScalar
CostMethodHashJoin::scmComputeOverflowCost( CostScalar numBuildTuples, CostScalar buildRowSize, CostScalar numProbeTuples, CostScalar probeRowSize )
{
  NABoolean hashJoinOverFlowCosting = 
    (CmpCommon::getDefault(NCM_HJ_OVERFLOW_COSTING) == DF_ON);

  if (hashJoinOverFlowCosting == FALSE)
    return 0;

  CostScalar probeRowSizeFactor = scmRowSizeFactor(probeRowSize);
  CostScalar buildRowSizeFactor = scmRowSizeFactor(buildRowSize);

  // Use the OCM value (200 MB) for memory limit per cpu/stream in bytes.
  // The value in the defaults table is 204800, in KB.
  CostScalar memoryAvailableInKB = ActiveSchemaDB()->getDefaults().getAsDouble(BMO_MEMORY_SIZE);

  // Convert to bytes as all computations here are in bytes.
  CostScalar memoryAvailable = memoryAvailableInKB * csOneKiloBytes;

  CostScalar memoryUsed = numBuildTuples * buildRowSize;

  if (memoryUsed <= memoryAvailable)
    return 0;

  CostScalar fractionOverflow = CostScalar(1.0) - memoryAvailable/memoryUsed;

  // The factor of 2 comes from having to write overflow tuples to disk and 
  // read back the overflow tuples from disk.
  CostScalar buildOverflow = CostScalar(2.0) * fractionOverflow *
    numBuildTuples * buildRowSizeFactor;

  // The factor of 2 comes from having to write overflow tuples to disk and 
  // read back the overflow tuples from disk.
  CostScalar probeOverflow = CostScalar(2.0) * fractionOverflow *
    numProbeTuples * probeRowSizeFactor;

  return (buildOverflow + probeOverflow);

} // CostMethodHashJoin::scmComputeOverflowCost

//<pb>
//**********************************************************************/
/*                                                                    */
/*                         CostMethodMergeJoin                         */
/*                                                                    */
/**********************************************************************/

// -----------------------------------------------------------------------
// CostMethodMergeJoin::scmComputeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodMergeJoin::scmComputeOperatorCostInternal(RelExpr* op,
                                                    const PlanWorkSpace* pws,
						    Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  mj_ = (MergeJoin*) op;

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  CostScalar outputJoinCard = equiJnRowCountPerStream_;
  
  CostScalar leftTuples = child0RowCountPerStream_;
  CostScalar rightTuples = child1RowCountPerStream_;
  CostScalar tuplesProcessed, tuplesProduced;
    
  // Length of a row from the left table.
  GroupAttributes* child0GA = mj_->child(0).getGroupAttr();
  CostScalar leftRowSize = child0GA->getRecordLength();

  // Length of a row from the right table.
  GroupAttributes* child1GA = mj_->child(1).getGroupAttr();
  CostScalar rightRowSize = child1GA->getRecordLength();

  CostScalar outputRowSize = mj_->getGroupAttr()->getCharacteristicOutputs().getRowLength();

  CostScalar leftRowSizeFactor = scmRowSizeFactor(leftRowSize);
  CostScalar rightRowSizeFactor = scmRowSizeFactor(rightRowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  // The inputs to the MergeJoin are always sorted, either the inputs are
  // already naturally sorted or there are explicit Sort operators below.
  // MergeJoins are 40% faster than HashJoins if the inputs are already sorted.
  // This was based on lots of experiments and analysis of costs and execution 
  // times.
  double mergeToHashJoinFactor =
    ActiveSchemaDB()->getDefaults().getAsDouble(NCM_MJ_TO_HJ_FACTOR);
  tuplesProcessed = (rightTuples * rightRowSizeFactor + 
		     leftTuples * leftRowSizeFactor) * mergeToHashJoinFactor;
  // COMP_BOOL_97 OFF means MJ fix is ON (default case). 
  if ( CmpCommon::getDefault(COMP_BOOL_97) == DF_OFF )
    tuplesProduced = outputJoinCard * outputRowSizeFactor;
  else // will be removed after perf testing
    tuplesProduced = outputJoinCard * outputRowSizeFactor * mergeToHashJoinFactor;

  //----------------------------------------
  //  Synthesize and return the cost object.
  //----------------------------------------
  
  Cost* mergeJoinCost =
    scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    leftRowSize, rightRowSize, outputRowSize, csZero);

#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"MERGEJOIN::scmComputeOperatorCostInternal()\n");
    fprintf(pfp,"child0RowCount=%g,child1RowCount=%g,myRowCount=%g\n",
                                 child0RowCount_.toDouble(),child1RowCount_.toDouble(),myRowCount_.toDouble());
    mergeJoinCost->getScmCplr().print(pfp);
    fprintf(pfp, "Elapsed time: ");
    fprintf(pfp,"%f", mergeJoinCost->
	    convertToElapsedTime(myContext->getReqdPhysicalProperty()).value());
    fprintf(pfp,"\n");
  }
#endif

  return mergeJoinCost;
}
//<pb>

/**********************************************************************/
/*                                                                    */
/*                         CostMethodNestedJoin                         */
/*                                                                    */
/**********************************************************************/


// -----------------------------------------------------------------------
// CostMethodNestedJoin::scmComputeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodNestedJoin::scmComputeOperatorCostInternal(RelExpr* op,
                                                     const PlanWorkSpace* pws,
						     Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  CostScalar tuplesProcessed, tuplesProduced;
  CostScalar outputJoinCard = (myRowCount_/countOfStreams).getCeiling();

  CostScalar leftTuples;

  // I have chosen the left side arbitrarily, the right side will be considered 
  // by the optimizer during consideration of the commutative permutation.
  CostScalar broadcastTuples = child0RowCount_;// child0RowCountPerStream_ * countOfStreams;
  if ((child0PartFunc_ == NULL) OR
      child0PartFunc_->isAReplicateViaBroadcastPartitioningFunction())
    leftTuples = broadcastTuples;
  else
    leftTuples = child0RowCountPerStream_;

  // Length of a row from the left table.
  GroupAttributes* child0GA = nj_->child(0).getGroupAttr();
  CostScalar leftRowSize = child0GA->getRecordLength();

  // Length of a row from the right table.
  GroupAttributes* child1GA = nj_->child(1).getGroupAttr();
  CostScalar rightRowSize = child1GA->getRecordLength();

  CostScalar outputRowSize = nj_->getGroupAttr()->getCharacteristicOutputs().getRowLength();

  CostScalar leftRowSizeFactor = scmRowSizeFactor(leftRowSize);
  CostScalar rightRowSizeFactor = scmRowSizeFactor(rightRowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  // The tuples (probes) from the left side are processed by the nested join 
  // and sent down to the right side. 
  // These are sent back up from the right side to the nested join, this 
  // accounts for the factor of 2 in the formula.
  tuplesProcessed = leftTuples * leftRowSizeFactor * 2;

  // The nested join operator produces "outputJoinCard" number of rows.
  tuplesProduced = outputJoinCard * outputRowSizeFactor;

  //----------------------------------------
  //  Synthesize and return the cost object.
  //----------------------------------------
  
  Cost* nestedJoinCost = 
    scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    leftRowSize, rightRowSize, outputRowSize, csZero);

#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"NESTEDJOIN::scmComputeOperatorCostInternal()\n");
    fprintf(pfp,"child0RowCount=%g,child1RowCount=%g,myRowCount=%g\n",
                                child0RowCount_.toDouble(),child1RowCount_.toDouble(),myRowCount_.toDouble());
    nestedJoinCost->getScmCplr().print(pfp);
    fprintf(pfp, "Elapsed time: ");
    fprintf(pfp,"%f", nestedJoinCost->
	    convertToElapsedTime(myContext->getReqdPhysicalProperty()).value());
    fprintf(pfp,"\n");
  }
#endif

  return nestedJoinCost;
}

/**********************************************************************/
/*                                                                    */
/*                         CostMethodNestedJoinFlow                   */
/*                                                                    */
/**********************************************************************/


// -----------------------------------------------------------------------
// CostMethodNestedJoinFlow::scmComputeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost*
CostMethodNestedJoinFlow::scmComputeOperatorCostInternal(RelExpr* op,
                                                         const PlanWorkSpace* pws,
							 Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  // Length of a row from the left table.
  GroupAttributes* child0GA = nj_->child(0).getGroupAttr();
  CostScalar leftRowSize = child0GA->getRecordLength();

  CostScalar outputRowSize = nj_->getGroupAttr()->getCharacteristicOutputs().getRowLength();

  CostScalar leftRowSizeFactor = scmRowSizeFactor(leftRowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  // NestedJoinFlow doesn't do anything by itself except 
  // sends left child rows to right child
  CostScalar tuplesProcessed = child0RowCountPerStream_ * leftRowSizeFactor;
  // Passes rows from right child to its parent.
  CostScalar tuplesProduced = (myRowCount_/countOfStreams).getCeiling() * outputRowSizeFactor;

  //----------------------------------------
  //  Synthesize and return the cost object.
  //----------------------------------------

  Cost* nestedJoinFlowCost = 
    scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    leftRowSize, csZero, outputRowSize, csZero);

#ifndef NDEBUG
  NABoolean printCost =
    ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON );
  if ( printCost )
  {
    pfp = stdout;
    fprintf(pfp,"NESTEDJOINFLOW::scmComputeOperatorCostInternal()\n");
    fprintf(pfp,"child0RowCount=%g,child1RowCount=%g,myRowCount=%g\n",
                                child0RowCount_.toDouble(),child1RowCount_.toDouble(),myRowCount_.toDouble());
    nestedJoinFlowCost->getScmCplr().print(pfp);
    fprintf(pfp, "Elapsed time: ");
    fprintf(pfp,"%f", nestedJoinFlowCost->
	    convertToElapsedTime(myContext->getReqdPhysicalProperty()).value());
    fprintf(pfp,"\n");
  }
#endif

  return nestedJoinFlowCost;
}

// CostMethodTranspose::scmComputeOperatorCostInternal() -------------------------
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
Cost *
CostMethodTranspose::scmComputeOperatorCostInternal(RelExpr *op,
                                            const PlanWorkSpace* pws,
                                            Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

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
  EstLogPropSharedPtr inputLP = myContext->getInputLogProp();
  EstLogPropSharedPtr childOutputLP = op->child(0).outputLogProp( inputLP );
  const CostScalar & child0RowCount = childOutputLP->getResultCardinality();

  CostScalar tuplesProcessed = (child0RowCount/countOfStreams).getCeiling();
  CostScalar tuplesProduced = (myRowCount_/countOfStreams).getCeiling();

  GroupAttributes* child0GA = op->child(0).getGroupAttr();
  CostScalar inputRowSize = child0GA->getRecordLength();
  CostScalar outputRowSize = op->getGroupAttr()->getCharacteristicOutputs().getRowLength();

  CostScalar inputRowSizeFactor = scmRowSizeFactor(inputRowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  tuplesProcessed *= inputRowSizeFactor;
  tuplesProduced *= outputRowSizeFactor;

  // ------------------------------------------------
  // Synthesize and return the cost object.
  // ------------------------------------------------
  Cost* trnsPoseCost = 
        scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    inputRowSize, csZero, outputRowSize, csZero);

  return trnsPoseCost;
}      // CostMethodTranspose::scmComputeOperatorInternal()


/**********************************************************************/
/*                                                                    */
/*                          CostMethodSample                          */
/*                                                                    */
/**********************************************************************/
// Compute common costing parameters.
//
// CostMethodSample::scmComputeOperatorCostInternal() ---------------------
// Compute the cost of this Sample node given the optimization context.
//
// Parameters
//
// RelExpr *op
//  IN - The PhysSample node which is being costed.
//
// const PlanWorkSpace* pws
//  IN - Optimization context within which to cost this node.
//
// long& countOfStreams
//  OUT - Estimated degree of parallelism for returned preliminary cost.
//
Cost *
CostMethodSample::scmComputeOperatorCostInternal(RelExpr *op,
                                                 const PlanWorkSpace* pws,
						 Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  // Just to make sure things are working as expected
  //
  DCMPASSERT(op->getOperatorType() == REL_SAMPLE);

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  EstLogPropSharedPtr inputLP = myContext->getInputLogProp();
  EstLogPropSharedPtr childOutputLP = op->child(0).outputLogProp( inputLP );
  const CostScalar & child0RowCount = childOutputLP->getResultCardinality();

  CostScalar tuplesProcessed = (child0RowCount/countOfStreams).getCeiling();
  CostScalar tuplesProduced = (myRowCount_/countOfStreams).getCeiling();

  GroupAttributes* child0GA = op->child(0).getGroupAttr();
  CostScalar inputRowSize = child0GA->getRecordLength();
  CostScalar outputRowSize = op->getGroupAttr()->getCharacteristicOutputs().getRowLength();

  CostScalar inputRowSizeFactor = scmRowSizeFactor(inputRowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  tuplesProcessed *= inputRowSizeFactor;
  tuplesProduced *= outputRowSizeFactor;

  // ------------------------------------------------
  // Synthesize and return the cost object.
  // ------------------------------------------------
  Cost* sampleCost = 
        scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    inputRowSize, csZero, outputRowSize, csZero);

  return sampleCost;
}      // CostMethodSample::scmComputeOperatorCostInternal()

Cost *
CostMethodRelSequence::scmComputeOperatorCostInternal(RelExpr *op,
                                                      const PlanWorkSpace* pws,
						      Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

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

  EstLogPropSharedPtr inputLP = myContext->getInputLogProp();
  EstLogPropSharedPtr childOutputLP = op->child(0).outputLogProp( inputLP );
  const CostScalar & child0RowCount = childOutputLP->getResultCardinality();

  CostScalar tuplesProcessed = (child0RowCount/countOfStreams).getCeiling();
  CostScalar tuplesProduced = (myRowCount_/countOfStreams).getCeiling();

  GroupAttributes* child0GA = op->child(0).getGroupAttr();
  CostScalar inputRowSize = child0GA->getRecordLength();
  CostScalar outputRowSize = op->getGroupAttr()->getCharacteristicOutputs().getRowLength();

  CostScalar inputRowSizeFactor = scmRowSizeFactor(inputRowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  tuplesProcessed *= inputRowSizeFactor;
  tuplesProduced *= outputRowSizeFactor;

  // ------------------------------------------------
  // Synthesize and return the cost object.
  // ------------------------------------------------
  Cost* seqCost = 
        scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    inputRowSize, csZero, outputRowSize, csZero);

  return seqCost;
}      // CostMethodRelSequence::scmComputeOperatorInternal()

//-------------------------------------------------------------------------
// CostMethodCompoundStmt::scmComputeOperatorInternal()
// Compute the cost of this Compound Statement node, given the optimization
// context.
//
// Parameters
//
// RelExpr *op
//  IN - The PhysTranpose node which is being costed.
//
// Context *myContext
//  IN - The optimization context within which to cost this node.
//-------------------------------------------------------------------------

Cost *
CostMethodCompoundStmt::scmComputeOperatorCostInternal(RelExpr *op,
                                                       const PlanWorkSpace* pws,
						       Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  // Just to make sure things are working as expected
  //
  DCMPASSERT(op->getOperatorType() == REL_COMPOUND_STMT);

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  EstLogPropSharedPtr inputLP = myContext->getInputLogProp();
  EstLogPropSharedPtr child0OutputLP = op->child(0).outputLogProp( inputLP );
  EstLogPropSharedPtr child1OutputLP = op->child(1).outputLogProp( inputLP );
  const CostScalar & child0RowCount = child0OutputLP->getResultCardinality();
  const CostScalar & child1RowCount = child1OutputLP->getResultCardinality();

  GroupAttributes* child0GA = op->child(0).getGroupAttr();
  CostScalar input0RowSize = child0GA->getRecordLength();
  GroupAttributes* child1GA = op->child(1).getGroupAttr();
  CostScalar input1RowSize = child1GA->getRecordLength();
  CostScalar outputRowSize = op->getGroupAttr()->getCharacteristicOutputs().getRowLength();

  CostScalar input0RowSizeFactor = scmRowSizeFactor(input0RowSize);
  CostScalar input1RowSizeFactor = scmRowSizeFactor(input1RowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  CostScalar tuplesProcessed = 
    child0RowCount * input0RowSizeFactor + child1RowCount * input1RowSizeFactor;
  // per stream basis
  tuplesProcessed = (tuplesProcessed/countOfStreams).getCeiling();
  CostScalar tuplesProduced = (myRowCount_/countOfStreams).getCeiling() * outputRowSizeFactor;

  // ------------------------------------------------
  // Synthesize and return the cost object.
  // ------------------------------------------------
  Cost* csCost = 
        scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    input0RowSize, input1RowSize, outputRowSize, csZero);

  return csCost;
}      // CostMethodCompoundStmt::scmComputeOperatorInternal()

Cost *
CostMethodStoredProc::scmComputeOperatorCostInternal(RelExpr *op,
                                                     const PlanWorkSpace* pws,
						     Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  //  Stored procedures never run in parallel.
  // -----------------------------------------
  countOfStreams = 1;

  CostScalar outputRowSize = op->getGroupAttr()->getCharacteristicOutputs().getRowLength();
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  CostScalar tuplesProduced = myRowCount_ * outputRowSizeFactor;

  // ------------------------------------------------
  // Synthesize and return the cost object.
  // ------------------------------------------------
  Cost* spCost = 
    scmCost(csZero, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    csZero, csZero, outputRowSize, csZero);

  return spCost;
}      // CostMethodStoredProc::scmComputeOperatorInternal()

Cost *
CostMethodTuple::scmComputeOperatorCostInternal(RelExpr *op,
                                                const PlanWorkSpace* pws,
						Lng32& countOfStreams)
{
  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,pws->getContext());
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  CostScalar outputRowSize = op->getGroupAttr()->getCharacteristicOutputs().getRowLength();
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  // ---------------------------------------------------------------------
  // The Tuple operator returns exactly one row for each probe it gets.
  // Thus, its total row count should just be probeCount.
  // ---------------------------------------------------------------------
  CostScalar tuplesProduced = myRowCount_ * noOfProbesPerStream_ * outputRowSizeFactor;

  // ------------------------------------------------
  // Synthesize and return the cost object.
  // ------------------------------------------------
  Cost* tupCost = 
    scmCost(csZero, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    csZero, csZero, outputRowSize, csZero);

  return tupCost;
}      // CostMethodTuple::scmComputeOperatorInternal()

Cost *
CostMethodUnPackRows::scmComputeOperatorCostInternal(RelExpr *op,
                                                     const PlanWorkSpace* pws,
						     Lng32& countOfStreams)
{
  // Just to make sure things are working as expected
  //
  DCMPASSERT(op->getOperatorType() == REL_UNPACKROWS);

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,pws->getContext());
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  CostScalar outputRowSize = op->getGroupAttr()->getCharacteristicOutputs().getRowLength();
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  CostScalar tuplesProduced = (myRowCount_/countOfStreams).getCeiling() * outputRowSizeFactor;

  // ------------------------------------------------
  // Synthesize and return the cost object.
  // ------------------------------------------------
  Cost* upackCost = 
    scmCost(csZero, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    csZero, csZero, outputRowSize, csZero);

  return upackCost;
}      // CostMethodUnPackRows::scmComputeOperatorInternal()

// -----------------------------------------------------------------------
// CostMethodMergeUnion::computeOperatorCostInternal().
// -----------------------------------------------------------------------
Cost *
CostMethodMergeUnion::scmComputeOperatorCostInternal(RelExpr *op,
                                                     const PlanWorkSpace* pws,
						     Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  EstLogPropSharedPtr inputLP = myContext->getInputLogProp();
  EstLogPropSharedPtr child0OutputLP = op->child(0).outputLogProp( inputLP );
  EstLogPropSharedPtr child1OutputLP = op->child(1).outputLogProp( inputLP );
  const CostScalar & child0RowCount = child0OutputLP->getResultCardinality();
  const CostScalar & child1RowCount = child1OutputLP->getResultCardinality();

  GroupAttributes* child0GA = op->child(0).getGroupAttr();
  CostScalar input0RowSize = child0GA->getRecordLength();
  GroupAttributes* child1GA = op->child(1).getGroupAttr();
  CostScalar input1RowSize = child1GA->getRecordLength();
  CostScalar outputRowSize = op->getGroupAttr()->getCharacteristicOutputs().getRowLength();

  CostScalar input0RowSizeFactor = scmRowSizeFactor(input0RowSize);
  CostScalar input1RowSizeFactor = scmRowSizeFactor(input1RowSize);
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  CostScalar tuplesProcessed = 
    child0RowCount * input0RowSizeFactor + child1RowCount * input1RowSizeFactor;
  // per stream basis
  tuplesProcessed = (tuplesProcessed/countOfStreams).getCeiling();
  CostScalar tuplesProduced = (myRowCount_/countOfStreams).getCeiling() * outputRowSizeFactor;

  // ------------------------------------------------
  // Synthesize and return the cost object.
  // ------------------------------------------------
  Cost* muCost = 
        scmCost(tuplesProcessed, tuplesProduced, csZero, csZero, csZero, noOfProbesPerStream_,
	    input0RowSize, input1RowSize, outputRowSize, csZero);

  return muCost;
}      // CostMethodMergeUnion::scmComputeOperatorInternal()
	   
//<pb>

// -------------------------------------------------------------------
// Cost methods for write DML operations
// -------------------------------------------------------------------


// -------------------------------------------------------------------
// This method is a stub for obsolete old cost model
// -------------------------------------------------------------------
Cost*
CostMethodHbaseUpdateOrDelete::computeOperatorCostInternal(RelExpr* op,
  const Context * context,
  Lng32& countOfStreams)
{
  CMPASSERT(false);  // should never be called
  return NULL;
}

// -----------------------------------------------------------------------
// CostMethodHbaseUpdateOrDelete::allKeyColumnsHaveHistogramStatistics()
//
// Returns TRUE if all key columns have histograms, FALSE if not.
// -----------------------------------------------------------------------
NABoolean CostMethodHbaseUpdateOrDelete::allKeyColumnsHaveHistogramStatistics(
  const IndexDescHistograms & histograms,
  const IndexDesc * CIDesc
  ) const
{
  // Check if all key columns have histogram statistics
  NABoolean statsForAllKeyCols = TRUE;
  for ( CollIndex j = 0; j < CIDesc->getIndexKey().entries(); j++ )
  {
    if (histograms.isEmpty())
    {
      statsForAllKeyCols = FALSE;
      break;
    }
    else if (!histograms.getColStatsPtrForColumn((CIDesc->getIndexKey()) [j]))
    {
      // If we get a null pointer when we try to retrieve a
      // ColStats for a column of this table, then no histogram
      // data was created for that column.
      statsForAllKeyCols = FALSE;
      break;
    }
  }

  return statsForAllKeyCols;
}   // CostMethodHbaseUpdateOrDelete::allKeyColumnsHaveHistogramStatistics()

// -----------------------------------------------------------------------
// CostMethodHbaseUpdateOrDelete::numRowsToScanWhenAllKeyColumnsHaveHistograms()
//
// Returns an estimate of the number of rows that will be scanned as a
// result of applying key predicates. Assumes that histograms exist for
// all key columns.
// -----------------------------------------------------------------------
#pragma nowarn(262)   // warning elimination
CostScalar
CostMethodHbaseUpdateOrDelete::numRowsToScanWhenAllKeyColumnsHaveHistograms(
  IndexDescHistograms & histograms,
  const ColumnOrderList & keyPredsByCol,
  const CostScalar & activePartitions,
  const IndexDesc * CIDesc
  ) const
{

  // Determine if there are single subset predicates:
  CollIndex singleSubsetPrefixOrder;
  NABoolean itIsSingleSubset =
    keyPredsByCol.getSingleSubsetOrder( singleSubsetPrefixOrder );

  NABoolean thereAreSingleSubsetPreds = FALSE;
  if ( singleSubsetPrefixOrder > 0 )
  {
    thereAreSingleSubsetPreds = TRUE;
  }
  else
  {
    //  singleSubsetPrefixOrder==0  means either there
    // is an equal, an IN,  or there are no key preds in the
    // first column.
    // singleSubsetPrefixOrder==0 AND itIsSingleSubset
    // means there is an EQUAL or there are no key preds
    // in the first column, check for existance of
    // predicates in this case:
    if (     itIsSingleSubset // this FALSE for an IN predicate
	 AND keyPredsByCol[0] != NULL
       )
    {
      thereAreSingleSubsetPreds = TRUE;
    }
  }


  CMPASSERT(NOT histograms.isEmpty());

  // Apply those key predicates that reference key columns
  // before the first missing key to the histograms:
  const SelectivityHint * selHint = CIDesc->getPrimaryTableDesc()->getSelectivityHint();
  const CardinalityHint * cardHint = CIDesc->getPrimaryTableDesc()->getCardinalityHint();

  if ( thereAreSingleSubsetPreds || selHint || cardHint )
  {
    // ---------------------------------------------------------
    // There are some key predicates, so apply them
    // to the histograms and get the total rows:
    // ---------------------------------------------------------

    // Get all the key preds for the key columns up to the first
    // key column with no key preds (if any)
    ValueIdSet singleSubsetPrefixPredicates;
    for ( CollIndex i = 0; i <= singleSubsetPrefixOrder; i++ )
    {
      const ValueIdSet *predsPtr = keyPredsByCol[i];
      CMPASSERT( predsPtr != NULL ); // it must contain preds
      singleSubsetPrefixPredicates.insert( *predsPtr );

    } // for every key col in the sing. subset prefix

    RelExpr * dummyExpr = new (STMTHEAP) RelExpr(ITM_FIRST_ITEM_OP,
				                 NULL,
				                 NULL,
				                 STMTHEAP);

    histograms.applyPredicates( singleSubsetPrefixPredicates, *dummyExpr, selHint, cardHint);

  } // if there are key predicates

  // If there is no key predicates, a full table scan will be generated.
  // Otherwise, key predicates will be applied to the histograms.
  // Now, compute the number of rows after key preds are applied,
  // and accounting for asynchronous parallelism:
  CostScalar numRowsToScan =
    ((histograms.getRowCount()/activePartitions).getCeiling()).minCsOne();

  return numRowsToScan;
}   // CostMethodHbaseUpdateOrDelete::numRowsToScanWhenAllKeyColumnsHaveHistograms()
#pragma warn(262)  // warning elimination

// -----------------------------------------------------------------------
// CostMethodHbaseUpdateOrDelete::computeIOCostsForCursorOperation().
// -----------------------------------------------------------------------
void CostMethodHbaseUpdateOrDelete::computeIOCostsForCursorOperation(
  CostScalar & randomIOs,        // out
  CostScalar & sequentialIOs,    // out
  const IndexDesc * CIDesc,
  const CostScalar & numRowsToScan,
  NABoolean probesInOrder
  ) const
{
  const CostScalar & kbPerBlock = CIDesc->getBlockSizeInKb();
  // if rowsize is bigger than blocksize, rowsPerBlock will be 1.
  const CostScalar rowsPerBlock =
    ((kbPerBlock * csOneKiloBytes) /
     CIDesc->getNAFileSet()->getRecordLength()).getCeiling();
  CostScalar totalIndexBlocks(csZero);

  if (probesInOrder)
  {
    // If the probes are in order, assume that each successive
    // probe refers to the next record in the table, i.e. the
    // probes are "highly inclusive", or in other words, there
    // are no gaps in the records to be updated. So, assuming
    // this, the number of blocks that need to be read is
    // the # of probes divided by the rows per block. This is
    // guaranteed to be correct if we are updating most of the
    // rows, we can only go wrong if we are updating a small
    // number of dispersed rows. This seems unlikely, and anyway
    // even if it's true we won't be that far off. If the rows
    // are highly inclusive, we could also assume that since the
    // blocks will be contiguous that there will only by one seek.
    // But, we'd be way off in the case where the assumption
    // doesn't hold so we won't do it for now. What we need is
    // some way to determine the "inclusiveness factor".
    sequentialIOs = (numRowsToScan / rowsPerBlock).getCeiling();
    // The # of index blocks to read is based on the number of data
    // blocks that must be read
    totalIndexBlocks =
      CIDesc->getEstimatedIndexBlocksLowerBound(sequentialIOs);
    randomIOs = totalIndexBlocks;
  }
  else  // probes not in order
  {
    // Assume all IOs are random. This is a bit pessimistic
    // because at some point much of the file will be in cache,
    // so one could argue that as the number of rows updated
    // or deleted grows large the number of random IOs should
    // decrease. We'll leave that to future work.
    sequentialIOs = csZero;
    totalIndexBlocks =
      CIDesc->getEstimatedIndexBlocksLowerBound(numRowsToScan);
    randomIOs = numRowsToScan + totalIndexBlocks;
  }

} // CostMethodHbaseUpdateOrDelete::computeIOCostsForCursorOperation()



// ----QUICKSEARCH FOR HbaseUpdate........................................

/**********************************************************************/
/*                                                                    */
/*                      CostMethodHbaseUpdate                         */
/*                                                                    */
/**********************************************************************/

//*******************************************************************
// This method computes the cost vector of the HbaseUpdate operation
//*******************************************************************
Cost*
CostMethodHbaseUpdate::scmComputeOperatorCostInternal(RelExpr* op,
  const PlanWorkSpace* pws,
  Lng32& countOfStreams)
{
  // TODO: Write this method; the code below is a copy of the Delete
  // method which we'll use for the moment. This is better than just
  // a simple constant stub; we will get parallel Update plans with
  // this code, for example, that we won't get with constant cost.

  // The theory of operation of Update is somewhat different (since it
  // might, for example, do a Delete + an Insert, or might do an Hbase
  // Update -- is that decided before we get here?), so this code will
  // underestimate the cost in general.

  const Context * myContext = pws->getContext();

  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  const InputPhysicalProperty* ippForMe =
    myContext->getInputPhysicalProperty();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  HbaseUpdate* updOp = (HbaseUpdate *)op;   // downcast

  CMPASSERT(partFunc_ != NULL);

  //  Later, if and when we start using NodeMaps to track active regions for 
  //  Trafodion tables in HBase (or native HBase tables), we can use the
  //  following to get active partitions.
  //CostScalar activePartitions =
  // (CostScalar)
  //   (((NodeMap *)(partFunc_->getNodeMap()))->getNumActivePartitions());
  //  But for now, we do the following:
  CostScalar activePartitions = (CostScalar)(partFunc_->getCountOfPartitions());

  const IndexDesc* CIDesc = updOp->getIndexDesc();
  const CostScalar & recordSizeInKb = CIDesc->getRecordSizeInKb();

  CostScalar tuplesProcessed(csZero);
  CostScalar tuplesProduced(csZero);
  CostScalar tuplesSent(csZero);  // we use tuplesSent to model sending rowIDs to Hbase 
  CostScalar randomIOs(csZero);
  CostScalar sequentialIOs(csZero);

  CostScalar countOfAsynchronousStreams = activePartitions;

  // figure out if the probes are in order - if they are, then when
  // scanning, I/O will tend to be sequential

  NABoolean probesInOrder = FALSE;
  if (ippForMe != NULL)  // input physical properties exist?
  {
    // See if the probes are in order.

    // For delete, a partial order is ok.
    NABoolean partiallyInOrderOK = TRUE;
    NABoolean probesForceSynchronousAccess = FALSE;
    ValueIdList targetSortKey = CIDesc->getOrderOfKeyValues();
    ValueIdSet sourceCharInputs =
      updOp->getGroupAttr()->getCharacteristicInputs();

    ValueIdSet targetCharInputs;
    // The char inputs are still in terms of the source. Map them to the target.
    // Note: The source char outputs in the ipp have already been mapped to
    // the target. CharOutputs are a set, meaning they do not have duplicates
    // But we could have cases where two columns of the target are matched to the
    // same source column, example: Sol: 10-040416-5166, where we have
    // INSERT INTO b6table1
    //		  ( SELECT f, h_to_f, f, 8.4
    //            FROM btre211
    //            );
    // Hence we use lists here instead of sets.
    // Check to see if there are any duplicates in the source Characteristics inputs
    // if no, we shall perform set operations, as these are faster
    ValueIdList bottomValues = updOp->updateToSelectMap().getBottomValues();
    ValueIdSet bottomValuesSet(bottomValues);
    NABoolean useListInsteadOfSet = FALSE;

    CascadesGroup* group1 = (*CURRSTMT_OPTGLOBALS->memo)[updOp->getGroupId()];

    GenericUpdate* upOperator = (GenericUpdate *) group1->getFirstLogExpr();

    if (((upOperator->getTableName().getSpecialType() == ExtendedQualName::NORMAL_TABLE ) || (upOperator->getTableName().getSpecialType() == ExtendedQualName::GHOST_TABLE )) &&
     (bottomValuesSet.entries() != bottomValues.entries() ) )
    {

      ValueIdList targetInputList;
      // from here get all the bottom values that appear in the sourceCharInputs
      bottomValues.findCommonElements(sourceCharInputs );
      bottomValuesSet = bottomValues;

      // we can use the bottomValues only if these contain some duplicate columns of
      // characteristics inputs, otherwise we shall use the characteristics inputs.
      if (bottomValuesSet == sourceCharInputs)
      {
        useListInsteadOfSet = TRUE;
	updOp->updateToSelectMap().rewriteValueIdListUpWithIndex(
	  targetInputList,
	  bottomValues);
	targetCharInputs = targetInputList;
      }
    }

    if (!useListInsteadOfSet)
    {
      updOp->updateToSelectMap().rewriteValueIdSetUp(
	targetCharInputs,
	sourceCharInputs);
    }

    // If a target key column is covered by a constant on the source side,
    // then we need to remove that column from the target sort key
    removeConstantsFromTargetSortKey(&targetSortKey,
                                   &(updOp->updateToSelectMap()));
    NABoolean orderedNJ = TRUE;
    // Don't call ordersMatch if njOuterOrder_ is null.
    if (ippForMe->getAssumeSortedForCosting())
      orderedNJ = FALSE;
    else
      // if leading keys are not same then don't try ordered NJ.
      orderedNJ =
        isOrderedNJFeasible(*(ippForMe->getNjOuterOrder()), targetSortKey);

    if (orderedNJ AND 
        ordersMatch(ippForMe,
                    CIDesc,
                    &targetSortKey,
                    targetCharInputs,
                    partiallyInOrderOK,
                    probesForceSynchronousAccess))
    {
      probesInOrder = TRUE;
      if (probesForceSynchronousAccess)
      {
        // The probes form a complete order across all partitions and
        // the clustering key and partitioning key are the same. So, the
        // only asynchronous I/O we will see will be due to ESPs. So,
        // limit the count of streams in DP2 by the count of streams in ESP.

        // Get the logPhysPartitioningFunction, which we will use
        // to get the logical partitioning function. If it's NULL,
        // it means the table was not partitioned at all, so we don't
        // need to limit anything since there already is no asynch I/O.

     // TODO: lppf is always null in Trafodion; figure out what to do instead...
        const LogPhysPartitioningFunction* lppf =
            partFunc_->castToLogPhysPartitioningFunction();
        if (lppf != NULL)
        {
          PartitioningFunction* logPartFunc =
            lppf->getLogPartitioningFunction();
          // Get the number of ESPs:
          CostScalar numParts = logPartFunc->getCountOfPartitions();

          countOfAsynchronousStreams = MINOF(numParts,
                                             countOfAsynchronousStreams);
        } // lppf != NULL
      } // probesForceSynchronousAccess
    } // probes are in order
  } // if input physical properties exist

  CostScalar currentCpus = 
    (CostScalar)myContext->getPlan()->getPhysicalProperty()->getCurrentCountOfCPUs();
  CostScalar activeCpus = MINOF(countOfAsynchronousStreams, currentCpus);
  CostScalar streamsPerCpu =
    (countOfAsynchronousStreams / activeCpus).getCeiling();


  CostScalar noOfProbesPerPartition(csOne);

  CostScalar numRowsToDelete(csOne);
  CostScalar numRowsToScan(csOne);

  CostScalar commonComputation;

  // Determine # of rows to scan and to delete

  if (updOp->getSearchKey() && updOp->getSearchKey()->isUnique() && 
    (noOfProbes_ == 1))
  {
    // unique access

    activePartitions = csOne;
    countOfAsynchronousStreams = csOne;
    activeCpus = csOne;
    streamsPerCpu = csOne;
    numRowsToScan = csOne;
    // assume the 1 row always satisfies any executor predicates so
    // we'll always do the Delete
    numRowsToDelete = csOne;
  }
  else
  {
    // non-unique access

    numRowsToDelete =
      ((myRowCount_ / activePartitions).getCeiling()).minCsOne();
    noOfProbesPerPartition =
      ((noOfProbes_ / countOfAsynchronousStreams).getCeiling()).minCsOne();

    // need to compute the number of rows that satisfy the key predicates
    // to compute the I/Os that must be performed

    // need to create a new histogram, since the one from input logical
    // prop. has the histogram for the table after all executor preds are
    // applied (i.e. the result cardinality)
    IndexDescHistograms histograms(*CIDesc,CIDesc->getIndexKey().entries());

    // retrieve all of the key preds in key column order
    ColumnOrderList keyPredsByCol(CIDesc->getIndexKey());
    updOp->getSearchKey()->getKeyPredicatesByColumn(keyPredsByCol);

    if ( NOT allKeyColumnsHaveHistogramStatistics( histograms, CIDesc ) )
    {
      // All key columns do not have histogram data, the best we can
      // do is use the number of rows that satisfy all predicates
      // (i.e. the number of rows we will be updating)
      numRowsToScan = numRowsToDelete;
    }
    else
    {
      numRowsToScan = numRowsToScanWhenAllKeyColumnsHaveHistograms(
	histograms,
	keyPredsByCol,
	activePartitions,
	CIDesc
	);
      if (numRowsToScan < numRowsToDelete) // sanity test
      {
        // we will scan at least as many rows as we delete
        numRowsToScan = numRowsToDelete;
      }
    }
  }

  // Notes: At execution time, several different TCBs can be created
  // for a delete. We can class them three ways: Unique, Subset, and
  // Rowset. Representative examples of the three classes are:
  //
  //   ExHbaseUMDtrafUniqueTaskTcb
  //   ExHbaseUMDtrafSubsetTaskTcb
  //   ExHbaseAccessSQRowsetTcb
  //
  // The theory of operation of each of these differs somewhat. 
  //
  // For the Unique variant, we use an HBase "get" to obtain a row, apply
  // a predicate to it, then do an HBase "delete" to delete it if the
  // predicate is true. (If there is no predicate, we'll simply do a
  // "checkAndDelete" so there would be no "get" cost.) 
  //
  // For the Subset variant, we use an HBase "scan" to obtain a sequence
  // of rows, apply a predicate to each, then do an HBase "delete" on
  // each row that passes the predicate.
  //
  // For the Rowset variant, we simply pass all the input keys to 
  // HBase in batches in HBase "deleteRows" calls. (In Explain plans,
  // this TCB shows up as "trafodion_delete_vsbb", while the first two
  // show up as "trafodion_delete".) There is no "get" cost. In plans
  // with this TCB, there is a separate Scan TCB to obtain the keys,
  // which then flow to this Rowset TCB via a tuple flow or nested join.
  // (Such a separate Scan might exist with the first two TCBs also,
  // e.g., when an index is used to decide which rows to delete.)
  // The messaging cost to HBase is also reduced since multiple delete
  // keys are sent per HBase interaction.
  //
  // Unfortunately the decisions as to which TCB will be used are
  // currently made in the generator code and so aren't easily 
  // available to us here. For the moment then, we make no attempt 
  // to distinguish a separate "get" cost, nor do we take into account
  // possible reduced message cost in the Rowset case. Should this
  // choice be refactored in the future to push it into the Optimizer,
  // then we can do a better job here. We did attempt to distinguish
  // the unique case here from the others, but even there our criteria
  // are not quite the same as in the generator. So at best, this attempt
  // simply sharpens the cost estimate in this one particular case.


  // Compute the I/O cost

  computeIOCostsForCursorOperation(
    randomIOs /* out */,
    sequentialIOs /* out */,
    CIDesc,
    numRowsToScan,
    probesInOrder
    );

  // Compute the tuple cost

  tuplesProduced = numRowsToDelete;
  tuplesProcessed = numRowsToScan; 
  tuplesSent = numRowsToDelete;

  CostScalar rowSize = updOp->getIndexDesc()->getRecordLength();
  CostScalar rowSizeFactor = scmRowSizeFactor(rowSize); 
  CostScalar outputRowSize = updOp->getGroupAttr()->getRecordLength();
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  tuplesProcessed *= rowSizeFactor;
  tuplesSent *= rowSizeFactor;
  tuplesProduced *= outputRowSizeFactor;


  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  CostScalar probeRowSize = updOp->getIndexDesc()->getKeyLength();
  Cost * updateCost = 
    scmCost(tuplesProcessed, tuplesProduced, tuplesSent, randomIOs, sequentialIOs, noOfProbes_,
	    rowSize, csZero, outputRowSize, probeRowSize);

#ifndef NDEBUG
if ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON )
    {
      pfp = stdout;
      fprintf(pfp, "HbaseUpdate::scmComputeOperatorCostInternal()\n");
      updateCost->getScmCplr().print(pfp);
      fprintf(pfp, "HBase Update elapsed time: ");
      fprintf(pfp,"%f", updateCost->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
      fprintf(pfp,"CountOfStreams returned %d\n",countOfStreams);
    }
#endif

  return updateCost;
}

// ----QUICKSEARCH FOR HbaseDelete........................................

/**********************************************************************/
/*                                                                    */
/*                      CostMethodHbaseDelete                         */
/*                                                                    */
/**********************************************************************/

//*******************************************************************
// This method computes the cost vector of the HbaseDelete operation
//*******************************************************************
Cost*
CostMethodHbaseDelete::scmComputeOperatorCostInternal(RelExpr* op,
  const PlanWorkSpace* pws,
  Lng32& countOfStreams)
{
  const Context * myContext = pws->getContext();

  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  const InputPhysicalProperty* ippForMe =
    myContext->getInputPhysicalProperty();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  HbaseDelete* delOp = (HbaseDelete *)op;   // downcast

  CMPASSERT(partFunc_ != NULL);

  //  Later, if and when we start using NodeMaps to track active regions for 
  //  Trafodion tables in HBase (or native HBase tables), we can use the
  //  following to get active partitions.
  //CostScalar activePartitions =
  // (CostScalar)
  //   (((NodeMap *)(partFunc_->getNodeMap()))->getNumActivePartitions());
  //  But for now, we do the following:
  CostScalar activePartitions = (CostScalar)(partFunc_->getCountOfPartitions());

  const IndexDesc* CIDesc = delOp->getIndexDesc();
  const CostScalar & recordSizeInKb = CIDesc->getRecordSizeInKb();

  CostScalar tuplesProcessed(csZero);
  CostScalar tuplesProduced(csZero);
  CostScalar tuplesSent(csZero);  // we use tuplesSent to model sending rowIDs to Hbase 
  CostScalar randomIOs(csZero);
  CostScalar sequentialIOs(csZero);

  CostScalar countOfAsynchronousStreams = activePartitions;

  // figure out if the probes are in order - if they are, then when
  // scanning, I/O will tend to be sequential

  NABoolean probesInOrder = FALSE;
  if (ippForMe != NULL)  // input physical properties exist?
  {
    // See if the probes are in order.

    // For delete, a partial order is ok.
    NABoolean partiallyInOrderOK = TRUE;
    NABoolean probesForceSynchronousAccess = FALSE;
    ValueIdList targetSortKey = CIDesc->getOrderOfKeyValues();
    ValueIdSet sourceCharInputs =
      delOp->getGroupAttr()->getCharacteristicInputs();

    ValueIdSet targetCharInputs;
    // The char inputs are still in terms of the source. Map them to the target.
    // Note: The source char outputs in the ipp have already been mapped to
    // the target. CharOutputs are a set, meaning they do not have duplicates
    // But we could have cases where two columns of the target are matched to the
    // same source column, example: Sol: 10-040416-5166, where we have
    // INSERT INTO b6table1
    //		  ( SELECT f, h_to_f, f, 8.4
    //            FROM btre211
    //            );
    // Hence we use lists here instead of sets.
    // Check to see if there are any duplicates in the source Characteristics inputs
    // if no, we shall perform set operations, as these are faster
    ValueIdList bottomValues = delOp->updateToSelectMap().getBottomValues();
    ValueIdSet bottomValuesSet(bottomValues);
    NABoolean useListInsteadOfSet = FALSE;

    CascadesGroup* group1 = (*CURRSTMT_OPTGLOBALS->memo)[delOp->getGroupId()];

    GenericUpdate* upOperator = (GenericUpdate *) group1->getFirstLogExpr();

    if (((upOperator->getTableName().getSpecialType() == ExtendedQualName::NORMAL_TABLE ) || (upOperator->getTableName().getSpecialType() == ExtendedQualName::GHOST_TABLE )) &&
     (bottomValuesSet.entries() != bottomValues.entries() ) )
    {

      ValueIdList targetInputList;
      // from here get all the bottom values that appear in the sourceCharInputs
      bottomValues.findCommonElements(sourceCharInputs );
      bottomValuesSet = bottomValues;

      // we can use the bottomValues only if these contain some duplicate columns of
      // characteristics inputs, otherwise we shall use the characteristics inputs.
      if (bottomValuesSet == sourceCharInputs)
      {
        useListInsteadOfSet = TRUE;
	delOp->updateToSelectMap().rewriteValueIdListUpWithIndex(
	  targetInputList,
	  bottomValues);
	targetCharInputs = targetInputList;
      }
    }

    if (!useListInsteadOfSet)
    {
      delOp->updateToSelectMap().rewriteValueIdSetUp(
	targetCharInputs,
	sourceCharInputs);
    }

    // If a target key column is covered by a constant on the source side,
    // then we need to remove that column from the target sort key
    removeConstantsFromTargetSortKey(&targetSortKey,
                                   &(delOp->updateToSelectMap()));
    NABoolean orderedNJ = TRUE;
    // Don't call ordersMatch if njOuterOrder_ is null.
    if (ippForMe->getAssumeSortedForCosting())
      orderedNJ = FALSE;
    else
      // if leading keys are not same then don't try ordered NJ.
      orderedNJ =
        isOrderedNJFeasible(*(ippForMe->getNjOuterOrder()), targetSortKey);

    if (orderedNJ AND 
        ordersMatch(ippForMe,
                    CIDesc,
                    &targetSortKey,
                    targetCharInputs,
                    partiallyInOrderOK,
                    probesForceSynchronousAccess))
    {
      probesInOrder = TRUE;
      if (probesForceSynchronousAccess)
      {
        // The probes form a complete order across all partitions and
        // the clustering key and partitioning key are the same. So, the
        // only asynchronous I/O we will see will be due to ESPs. So,
        // limit the count of streams in DP2 by the count of streams in ESP.

        // Get the logPhysPartitioningFunction, which we will use
        // to get the logical partitioning function. If it's NULL,
        // it means the table was not partitioned at all, so we don't
        // need to limit anything since there already is no asynch I/O.

     // TODO: lppf is always null in Trafodion; figure out what to do instead...
        const LogPhysPartitioningFunction* lppf =
            partFunc_->castToLogPhysPartitioningFunction();
        if (lppf != NULL)
        {
          PartitioningFunction* logPartFunc =
            lppf->getLogPartitioningFunction();
          // Get the number of ESPs:
          CostScalar numParts = logPartFunc->getCountOfPartitions();

          countOfAsynchronousStreams = MINOF(numParts,
                                             countOfAsynchronousStreams);
        } // lppf != NULL
      } // probesForceSynchronousAccess
    } // probes are in order
  } // if input physical properties exist

  CostScalar currentCpus = 
    (CostScalar)myContext->getPlan()->getPhysicalProperty()->getCurrentCountOfCPUs();
  CostScalar activeCpus = MINOF(countOfAsynchronousStreams, currentCpus);
  CostScalar streamsPerCpu =
    (countOfAsynchronousStreams / activeCpus).getCeiling();


  CostScalar noOfProbesPerPartition(csOne);

  CostScalar numRowsToDelete(csOne);
  CostScalar numRowsToScan(csOne);

  CostScalar commonComputation;

  // Determine # of rows to scan and to delete

  if (delOp->getSearchKey() && delOp->getSearchKey()->isUnique() && 
    (noOfProbes_ == 1))
  {
    // unique access

    activePartitions = csOne;
    countOfAsynchronousStreams = csOne;
    activeCpus = csOne;
    streamsPerCpu = csOne;
    numRowsToScan = csOne;
    // assume the 1 row always satisfies any executor predicates so
    // we'll always do the Delete
    numRowsToDelete = csOne;
  }
  else
  {
    // non-unique access

    numRowsToDelete =
      ((myRowCount_ / activePartitions).getCeiling()).minCsOne();
    noOfProbesPerPartition =
      ((noOfProbes_ / countOfAsynchronousStreams).getCeiling()).minCsOne();

    // need to compute the number of rows that satisfy the key predicates
    // to compute the I/Os that must be performed

    // need to create a new histogram, since the one from input logical
    // prop. has the histogram for the table after all executor preds are
    // applied (i.e. the result cardinality)
    IndexDescHistograms histograms(*CIDesc,CIDesc->getIndexKey().entries());

    // retrieve all of the key preds in key column order
    ColumnOrderList keyPredsByCol(CIDesc->getIndexKey());
    delOp->getSearchKey()->getKeyPredicatesByColumn(keyPredsByCol);

    if ( NOT allKeyColumnsHaveHistogramStatistics( histograms, CIDesc ) )
    {
      // All key columns do not have histogram data, the best we can
      // do is use the number of rows that satisfy all predicates
      // (i.e. the number of rows we will be updating)
      numRowsToScan = numRowsToDelete;
    }
    else
    {
      numRowsToScan = numRowsToScanWhenAllKeyColumnsHaveHistograms(
	histograms,
	keyPredsByCol,
	activePartitions,
	CIDesc
	);
      if (numRowsToScan < numRowsToDelete) // sanity test
      {
        // we will scan at least as many rows as we delete
        numRowsToScan = numRowsToDelete;
      }
    }
  }

  // Notes: At execution time, several different TCBs can be created
  // for a delete. We can class them three ways: Unique, Subset, and
  // Rowset. Representative examples of the three classes are:
  //
  //   ExHbaseUMDtrafUniqueTaskTcb
  //   ExHbaseUMDtrafSubsetTaskTcb
  //   ExHbaseAccessSQRowsetTcb
  //
  // The theory of operation of each of these differs somewhat. 
  //
  // For the Unique variant, we use an HBase "get" to obtain a row, apply
  // a predicate to it, then do an HBase "delete" to delete it if the
  // predicate is true. (If there is no predicate, we'll simply do a
  // "checkAndDelete" so there would be no "get" cost.) 
  //
  // For the Subset variant, we use an HBase "scan" to obtain a sequence
  // of rows, apply a predicate to each, then do an HBase "delete" on
  // each row that passes the predicate.
  //
  // For the Rowset variant, we simply pass all the input keys to 
  // HBase in batches in HBase "deleteRows" calls. (In Explain plans,
  // this TCB shows up as "trafodion_delete_vsbb", while the first two
  // show up as "trafodion_delete".) There is no "get" cost. In plans
  // with this TCB, there is a separate Scan TCB to obtain the keys,
  // which then flow to this Rowset TCB via a tuple flow or nested join.
  // (Such a separate Scan might exist with the first two TCBs also,
  // e.g., when an index is used to decide which rows to delete.)
  // The messaging cost to HBase is also reduced since multiple delete
  // keys are sent per HBase interaction.
  //
  // Unfortunately the decisions as to which TCB will be used are
  // currently made in the generator code and so aren't easily 
  // available to us here. For the moment then, we make no attempt 
  // to distinguish a separate "get" cost, nor do we take into account
  // possible reduced message cost in the Rowset case. Should this
  // choice be refactored in the future to push it into the Optimizer,
  // then we can do a better job here. We did attempt to distinguish
  // the unique case here from the others, but even there our criteria
  // are not quite the same as in the generator. So at best, this attempt
  // simply sharpens the cost estimate in this one particular case.


  // Compute the I/O cost

  computeIOCostsForCursorOperation(
    randomIOs /* out */,
    sequentialIOs /* out */,
    CIDesc,
    numRowsToScan,
    probesInOrder
    );

  // Compute the tuple cost

  tuplesProduced = numRowsToDelete;
  tuplesProcessed = numRowsToScan; 
  tuplesSent = numRowsToDelete;

  CostScalar rowSize = delOp->getIndexDesc()->getRecordLength();
  CostScalar rowSizeFactor = scmRowSizeFactor(rowSize); 
  CostScalar outputRowSize = delOp->getGroupAttr()->getRecordLength();
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  tuplesProcessed *= rowSizeFactor;
  tuplesSent *= rowSizeFactor;
  tuplesProduced *= outputRowSizeFactor;


  // ---------------------------------------------------------------------
  // Synthesize and return cost object.
  // ---------------------------------------------------------------------

  CostScalar probeRowSize = delOp->getIndexDesc()->getKeyLength();
  Cost * deleteCost = 
    scmCost(tuplesProcessed, tuplesProduced, tuplesSent, randomIOs, sequentialIOs, noOfProbes_,
	    rowSize, csZero, outputRowSize, probeRowSize);

#ifndef NDEBUG
if ( CmpCommon::getDefault( OPTIMIZER_PRINT_COST ) == DF_ON )
    {
      pfp = stdout;
      fprintf(pfp, "HbaseDelete::scmComputeOperatorCostInternal()\n");
      deleteCost->getScmCplr().print(pfp);
      fprintf(pfp, "HBase Delete elapsed time: ");
      fprintf(pfp,"%f", deleteCost->
              convertToElapsedTime(
                   myContext->getReqdPhysicalProperty()).
              value());
      fprintf(pfp,"\n");
      fprintf(pfp,"CountOfStreams returned %d\n",countOfStreams);
    }
#endif

  return deleteCost;

}  // CostMethodHbaseDelete::scmComputeOperatorCostInternal()

// ----QUICKSEARCH FOR HbaseInsert ........................................

/**********************************************************************/
/*                                                                    */
/*                      CostMethodHbaseInsert                         */
/*                                                                    */
/**********************************************************************/

//**************************************************************
// This method computes the cost vector of the HbaseInsert operation
//**************************************************************
Cost*
CostMethodHbaseInsert::scmComputeOperatorCostInternal(RelExpr* op,
  const PlanWorkSpace* pws,
  Lng32& countOfStreams)
{
  // TODO: For now assume we are always doing an HBase insert.
  // Is it possible we go through this code path for Hive inserts?
  // If so, figure out what to do.

  HbaseInsert* insOp = (HbaseInsert *)op;   // downcast

  // compute some details
  const Context * myContext = pws->getContext();
  Cost *costPtr = computeOperatorCostInternal(op, myContext, countOfStreams);

  CostScalar noOfProbesPerStream(csOne);

  // the number of rows to insert (this is "per-stream" costing).
  CostScalar numOfProbesPerStream =
    (noOfProbes_ / countOfAsynchronousStreams_).minCsOne();

  CostScalar tuplesProcessed = numOfProbesPerStream;
  CostScalar tuplesProduced = numOfProbesPerStream;

  CostScalar ioRand = csZero;  // we don't bother estimating this
  CostScalar ioSeq = csZero;  // we don't bother estimating this

  // Factor in row sizes.
  CostScalar rowSize = ((IndexDesc *)insOp->getIndexDesc())->getNAFileSet()->getRecordLength();
  CostScalar rowSizeFactor = scmRowSizeFactor(rowSize);
  tuplesProcessed *= rowSizeFactor;
  tuplesProduced *= rowSizeFactor;

  // there doesn't seem to be an estRowsAccessed_ member or
  // related methods in hbaseInsert at the moment... add it
  // when the need becomes apparent
  //insOp->setEstRowsAccessed(noOfProbes_);

  //----------------------------------------
  //  Synthesize and return the cost object.
  //----------------------------------------
  Cost *hbaseInsertCost =
    scmCost(tuplesProcessed, tuplesProduced, csZero, ioRand, ioSeq, noOfProbesPerStream_,
            rowSize, csZero, rowSize, csZero);

#ifndef NDEBUG
  NABoolean printCost =
    (CmpCommon::getDefault(OPTIMIZER_PRINT_COST) == DF_ON);
  if (printCost)
    {
      pfp = stdout;
      fprintf(pfp, "HbaseInsert::scmComputeOperatorCostInternal()\n");
      hbaseInsertCost->getScmCplr().print(pfp);
      fprintf(pfp, "Hbase Insert elapsed time: ");
      fprintf(pfp, "%f", hbaseInsertCost->convertToElapsedTime(myContext->getReqdPhysicalProperty()).value());
      fprintf(pfp, "\n");
      fprintf(pfp,"CountOfStreams returned %d\n",countOfStreams);
    }
#endif

  // We use the call to computeOperatorCostInternal() to compute the various costs, but we
  // we do not need the cost object computed by this method, we generate and return
  // a different New Cost Model (NCM) cost object.
  delete costPtr;

  return hbaseInsertCost;
}

/**********************************************************************/
// End cost methods for WRITE DML operations
/**********************************************************************/

//<pb>

//**************************************************************
// This method computes the cost vector of the IsolatedScalarUDF operation
//**************************************************************
Cost*
CostMethodIsolatedScalarUDF::scmComputeOperatorCostInternal(RelExpr* op,
                                                            const PlanWorkSpace* pws,
                                                            Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // -----------------------------------------
  // Save off estimated degree of parallelism.
  // -----------------------------------------
  countOfStreams = countOfStreams_;

  IsolatedScalarUDF *udf = (IsolatedScalarUDF *) op;

  // Make sure we are an UDF.
  CMPASSERT( udf != NULL );
  CMPASSERT( op->getOperatorType() == REL_ISOLATED_SCALAR_UDF  );

  // Get the size of the inputs.
  RowSize inputRowBytes = udf->getGroupAttr()->getInputVarLength();

  // -----------------------------------------
  // Determine the number of input Rows/Probes
  // -----------------------------------------
  CostScalar noOfProbes =
    ( myContext->getInputLogProp()->getResultCardinality() ).minCsOne();

  noOfProbes -= csOne;  // subtract one to account for the first row.

  // Make sure we have a RoutineDesc.
  CMPASSERT( udf->getRoutineDesc() != NULL );

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


  CostScalar inputRowSize = inputRowBytes;
  CostScalar inputRowSizeFactor = scmRowSizeFactor(inputRowSize);

  CostScalar outputRowSize = udf->getGroupAttr()->getRecordLength();
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  // Normalize the rowCount (inputProbes) over the number of streams.
  CostScalar rowCount = (noOfProbes/countOfStreams);

  // Cost for First probe.
  CostScalar tuplesProcessed = initialCpuCost * inputRowSizeFactor;
  CostScalar tuplesProduced = initialIOCost * outputRowSizeFactor;
  CostScalar tuplesSent = initialMsgCost * inputRowSizeFactor;
  
  
  // Cost for subsequent probes
  tuplesProcessed += rowCount * normalCpuCost * inputRowSizeFactor;
  tuplesProduced += rowCount * normalCpuCost * fanOut * normalIOCost * outputRowSizeFactor;
  // Assume we produce 2 messages to the UDF server per probe.
  // per output row(fanOut).
  tuplesSent += rowCount * normalMsgCost * 2 * fanOut * inputRowSizeFactor;

  Cost* tableRoutineCost =  scmCost(tuplesProcessed, tuplesProduced, 
                                    tuplesSent, csZero, csZero, csZero, 
                                    inputRowSize, csZero, outputRowSize, csZero);
  
  return tableRoutineCost;
}
//**************************************************************
// This method computes the cost vector of the TableMappingUDF operation
//**************************************************************
Cost*
CostMethodTableMappingUDF::scmComputeOperatorCostInternal(RelExpr* op,
                                                          const PlanWorkSpace* pws,
                                                          Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();
  EstLogPropSharedPtr inputLP  = myContext->getInputLogProp();

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // Save off estimated degree of parallelism.
  countOfStreams = countOfStreams_;

  TableMappingUDF *udf = (TableMappingUDF *) op;

  // Make sure we are an UDF.
  CMPASSERT( udf != NULL );
  CMPASSERT( op->castToTableMappingUDF() );

  const TMUDFPlanWorkSpace *tmudfPWS = static_cast<const TMUDFPlanWorkSpace *>(pws);
  const tmudr::UDRPlanInfo *planInfo = tmudfPWS->getUDRPlanInfo();

  EstLogPropSharedPtr outputLP = op->getGroupAttr()->outputLogProp( inputLP );
  CostScalar outputRowsPerStream = outputLP->getResultCardinality() / countOfStreams;

  // Get the size of the scalar inputs and output row
  RowSize inputRowSize = udf->getGroupAttr()->getInputVarLength();
  RowSize outputRowSize = op->getGroupAttr()->getRecordLength();

  // -----------------------------------------
  // Determine the number of input Rows/Probes
  // -----------------------------------------
  CostScalar noOfProbes =
    ( inputLP->getResultCardinality() ).minCsOne();

  // per stream Rows from child 
  CostScalar tuplesProcessed = csZero;

  for (int i=0; i < op->getArity(); i++)
    {
      EstLogPropSharedPtr childOutputLP = op->child(i).outputLogProp( inputLP );
      CostScalar  rowsFromChildPerStream =
        childOutputLP->getResultCardinality() / countOfStreams;
      RowSize childOutputRowBytes = udf->child(i).getGroupAttr()->getInputVarLength();
      CostScalar childOutputRowSizeFactor = scmRowSizeFactor(childOutputRowBytes);

      tuplesProcessed += rowsFromChildPerStream * childOutputRowSizeFactor;
    }

  // tuples produced
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);
  CostScalar tuplesProduced = outputRowsPerStream * outputRowSizeFactor;

  // We send all child output rows to the UDR server and receive all
  // the output rows back, so all processed and produced tuples are also sent
  CostScalar tuplesSent = csZero;

  // For isolated TMUDFs, add the cost of sending all child outputs
  // to the UDR server and sending all the results back from the UDR server,
  // this is in addition to any cost estimates the UDF writer provided.
  // For now, add this unconditionally, since all TMUDFs go through tdm_udrserv.
  // if (udf->getInvocationInfo()->getIsolationType() ==
  //    tmudr::UDRInvocationInfo::ISOLATED)
  tuplesSent = tuplesProcessed + tuplesProduced;

  if (planInfo->getCostPerRow() > 0)
    {
      // The UDF writer specified a cost per row (per stream) in nanoseconds,
      // convert that to internal units and create a cost by interpreting the
      // UDF cost per row as tuples produced
      CostScalar rowNanosecFactor = 
	 ActiveSchemaDB()->getDefaults().getAsDouble(NCM_UDR_NANOSEC_FACTOR);
      tuplesProduced = outputRowsPerStream * rowNanosecFactor * planInfo->getCostPerRow();

      // in this case, set tuplesProcessed to 0, since that cost is
      // included in the per output tuple cost specified by the UDF writer
      tuplesProcessed = csZero;

      // Note that we still add cost for sending tuples back and forth,
      // if applicable, in tuplesSent.
    }

  Cost* tableRoutineCost = scmCost(tuplesProcessed, tuplesProduced, 
                                   tuplesSent, csZero, csZero, csOne, 
                                   inputRowSize, csZero, outputRowSize, csZero);

  return tableRoutineCost;
  
  
}


//**************************************************************
// This method computes the cost vector of the FastExtract operation
//**************************************************************
Cost*
CostMethodFastExtract::scmComputeOperatorCostInternal(RelExpr* op,
                                                      const PlanWorkSpace* pws,
                                                      Lng32& countOfStreams)
{
  const Context* myContext = pws->getContext();
  EstLogPropSharedPtr inputLP  = myContext->getInputLogProp();

  // ---------------------------------------------------------------------
  // Preparatory work.
  // ---------------------------------------------------------------------
  cacheParameters(op,myContext);
  estimateDegreeOfParallelism();

  // Save off estimated degree of parallelism.
  countOfStreams = countOfStreams_;

  // Get the size of the inputs.
  RowSize inputRowBytes = op->getGroupAttr()->getInputVarLength();

  // -----------------------------------------
  // Determine the number of input Rows/Probes
  // -----------------------------------------
  CostScalar noOfProbes =
    ( inputLP->getResultCardinality() ).minCsOne();

  noOfProbes -= csOne;  // subtract one to account for the first row.

  CostScalar inputRowSize = inputRowBytes;
  CostScalar inputRowSizeFactor = scmRowSizeFactor(inputRowSize);

  CostScalar outputRowSize = op->getGroupAttr()->getRecordLength();
  CostScalar outputRowSizeFactor = scmRowSizeFactor(outputRowSize);

  // per stream Rows from child
   CostScalar  rowsFromChildPerStream ;
   EstLogPropSharedPtr childOutputLP = op->child(0).outputLogProp( inputLP );
   rowsFromChildPerStream = childOutputLP->getResultCardinality() / countOfStreams;


  // Cost for subsequent probes
  CostScalar tuplesProcessed = rowsFromChildPerStream * inputRowSizeFactor;
  CostScalar tuplesProduced = rowsFromChildPerStream * outputRowSizeFactor;
  // Assume we produce 2 messages to the UDF server per probe.
  // per output row(fanOut).
  CostScalar tuplesSent = tuplesProcessed;

  Cost* fastExtractCost =  scmCost(tuplesProcessed, tuplesProduced,
                                    tuplesSent, csZero, csZero, csZero,
                                    inputRowSize, csZero, outputRowSize, csZero);

  return fastExtractCost;


}
