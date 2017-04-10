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
 * File:         OptLogRelExpr.C
 * Description:  Optimizer methods related to Logical Expressions
 *
 * Created:      5/17/94
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
#include "AllRelExpr.h"
#include "AllItemExpr.h"
#include "Cost.h"
#include "opt.h"
#include "PhyProp.h"
#include "OptTrigger.h"
#include "hs_read.h"
#include "Analyzer.h"
#include "AppliedStatMan.h"
#include "NormWA.h"
#include "CmpStatement.h"

#include <math.h>

// -----------------------------------------------------------------------
// member functions for class RelExpr
// -----------------------------------------------------------------------

RelExpr *
RelExpr::optimizeNode()
{
  RelExpr *result;
  NABoolean printStats = CURRSTMT_OPTGLOBALS->BeSilent;

  //if (getenv("PRINT_OPTIMIZER_STATISTICS"))
  CURRSTMT_OPTGLOBALS->BeSilent = FALSE;

  // ---------------------------------------------------------------------
  // optimize
  // ---------------------------------------------------------------------
  NABoolean useNewOptDriver =
    (CmpCommon::getDefault(NEW_OPT_DRIVER) == DF_ON);

  if(useNewOptDriver)
    result = optimize2();
  else
    result = optimize();

  CURRSTMT_OPTGLOBALS->BeSilent = printStats;

  // release the bindings so the following phases can modify the tree
  if (result != NULL)
    result->releaseBindingTree();

  // The search space can now be deleted.
  // Since the group attributes have a reference count, they will still be
  // available for the result tree. Contexts will be gone, however.
  //delete memo;
  CURRSTMT_OPTGLOBALS->memo = NULL;

  return result;
} // RelExpr::optimizeNode()

void
RelExpr::cleanupAfterCompilation()
{
  // CascadesMemo now gets deleted before the generator phase, no need for this
  // method anymore
  // delete memo;
  // memo = NULL;
  // delete secondaryMemo;
  // secondaryMemo = NULL;
} // RelExpr::cleanupAfterCompilation()

// how many moves to pursue during exploration?
Int32
RelExpr::computeExploreExprCutoff(RuleWithPromise [],
                                  Int32 numberOfMoves,
                                  RelExpr *,
                                  Guidance *)
{
  // default implementation: use all promising moves
  return numberOfMoves;
} // RelExpr::computeExploreExprCutoff()

// given an array of moves (rule applications with a given
// promise), now many of the moves should actually be performed?
Int32
RelExpr::computeOptimizeExprCutoff(RuleWithPromise [],
                                   Int32 numberOfMoves,
                                   Guidance *,
                                   Context *)
{
  // default implementation: use all promising moves
  return numberOfMoves;
} // RelExpr::computeOptimizeExprCutoff()

// -----------------------------------------------------------------------
// This is a helper method for the purpose of synthesizing estimated
// logical properties for unary and leaf operators.  This method is
// invoked by various synthEstLogProp() methods.
// -----------------------------------------------------------------------
EstLogPropSharedPtr
RelExpr::synthEstLogPropForUnaryLeafOp (const EstLogPropSharedPtr& inputEstLogProp,
					const ColStatDescList & childStats,
					CostScalar initialRowcount,
                                        CostScalar *childMaxCardEst) const
{
  CollIndex outerRefCount = 0;
  OperatorTypeEnum opType = getOperatorType();

  // ------------------------------------------------------------------------
  // Special case for estimated rowcounts of SQL update operations (e.g.,
  // "update t1 set a = 3 where a < 2;") for columns which have an index.
  //
  // If this is a REL_LEAF_[DELETE | UPDATE | INSERT] node, and there are
  // not any selection predicates, then the outputestlogprop is exactly the
  // same as the inputestlogprop (because we're deleting / updating /
  // inserting rows that are exactly specified by the input key predicates).
  switch (opType) {
    case REL_LEAF_DELETE:
    case REL_LEAF_UPDATE:
    case REL_LEAF_INSERT: // For insert, we should take into account the initial
      // tableColStats of the Insert table too
      if ( getSelectionPred().entries() == 0 )
      {
        EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(*inputEstLogProp));
	GenericUpdate *updateExpr = (GenericUpdate *) this;
	// do it only if the update expression is on a normal table
	if (updateExpr->getTableName().getSpecialType() == ExtendedQualName::NORMAL_TABLE)
	{
	  // create myColStats after mapping my child's columns to my columns.
	  ValueIdMap & updateSelectValueIdMap = updateExpr->updateToSelectMap();

	  myEstProps->mapOutputsForUpdate(*updateExpr, updateSelectValueIdMap);
	}
	return myEstProps;
      }
    default:
      break ;
  }
  // ------------------------------------------------------------------------

  // output est log prop -- allocate it
  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(1));

  // Start with the provided initial rowcount.
  CostScalar newRowCount = initialRowcount;

  // If input column statistics have been provided, reset the initial new
  // rowcount.
  if (childStats.entries() > 0)
    newRowCount = childStats[0]->getColStats()->getRowcount();

  //++MV
  // remember the initial cardinality for later use
  CostScalar initialCardinality = newRowCount;
  //--MV

  // Part of fix to genesis cases 10-080530-0291, 10-080530-0305,
  // solution 10-080530-3538 "R2.4NF:RQ:AND predicate MIN sel not always
  // chosen for Max Card calc". 
  // Computation of expected cardinality estimates routinely SIDE EFFECT 
  // histograms. For example, given a compound predicate 
  //   "col = ? and col like pattern"
  // we used to make an estimateCardinality() call that computes 
  // maxSelectivity of "col = ?" but that SIDE EFFECTs the histogram. 
  // So, when a 2nd estimateCardinality() call computes maxSelectivity of 
  // "col like pattern", that maxSelectivity reflects the modified histogram 
  // which may be good for expected cardinality estimates but bad for max 
  // cardinality estimates. 
  // One way of fixing this problem is to split estimateMaxSelectivity and
  // estimateCardinality into separate methods. But, that duplicates code 
  // and causes code maintenance problems.
  // We tried to make these two methods operate on the same ColStatDescList
  // instance. But, that does not work. It breaks estimated cardinality or
  // it breaks max cardinality. Sigh :-(
  // So, the solution is to have two ColStatDescList instances and have one
  // method, estimateCardinality, compute both max selectivity & expected
  // cardinality using 2 calls with different arguments.
 
  // maxRows is our initial max cardinality estimate.
  // For some queries, eg, "select * from t10 where d not like 'one%'"
  // the max cardinality estimate for the root node can be below actual
  // because RelRoot::synthEstLogProp() calls
  //   synthEstLogPropForUnaryLeafOp(inputEstLogProp, 
  //     child(0).outputLogProp(inputEstLogProp)->getColStats(),
  //     child(0).outputLogProp(inputEstLogProp)->getResultCardinality())
  // Since max cardinality should never be below actual, we correct this by
  // also passing the child's max cardinality estimate
  //   synthEstLogPropForUnaryLeafOp(inputEstLogProp, 
  //     child(0).outputLogProp(inputEstLogProp)->getColStats(),
  //     child(0).outputLogProp(inputEstLogProp)->getResultCardinality(),
  //     child(0).outputLogProp(inputEstLogProp)->getMaxCardEst())
  // which shows up here as a non-null childMaxCardEst.
  CostScalar maxRows = 
    (childMaxCardEst ? (*childMaxCardEst) : initialCardinality);

  // unless overridden by downstream code, maxSelectivity is 1.0
  // this is a diffused & obscure way of computing maxCardinality and
  // maxSelectivity.
  // but we have to fit in without breaking existing expected cardinality
  // estimation code.
  CostScalar maxSelectivity = csOne;

  NABoolean indexNestedJoin = FALSE;
  CollIndex outerInd;
  MergeType mergeMethod = INNER_JOIN_MERGE; // starting assumption

  ColStatDescList columnStats(CmpCommon::statementHeap());

  if (opType != REL_SCAN)
  {
    // make the local copy of the information provided to this 'leaf' operator
    columnStats.makeDeepCopy (childStats) ;

    // We want to be sure that all colStats have the same row count.
    // It should assert if these are different

#define MONITOR_SAMEROWCOUNT
        columnStats.verifyInternalConsistency( 0, columnStats.entries() );
#undef MONITOR_SAMEROWCOUNT

  }
  else

    // ----------------------------------------------------------------------
    // If the RelExpr being processed is a scan (tbd: other operators too?)
    // there are numerous scan-specific actions to perform.
    // The inputEstLogProp's associated with true outer references (i.e., not
    // a host variable, or parameter), have been passed down from their source
    // largely unchanged.  They are now effectively cross-producted with the
    // child's statistics.
    // This allows parents of the scan to largely ignore the inputEstLogProps
    // they are given as input; the still-interesting descriptions of outer
    // references will be part of the output of their children.
    //
    // NOTE: This is not what run-time does; run-time doesn't provide output
    // that is identical with supplied input.  This treatment of input data
    // (outer references) is done to simplify logical property maintenance,
    // primarily in the area of histograms.
    //
    // The cross-producting of outer references' and child's statistics is
    // not done in either of two situations:
    // - The outer reference contains data from an index of the current table.
    // - The input description of the outer references indicate that they are
    //   the result of a nested [anti-]semi-join.
    //
    // In the case of an index-driven nested join, the estimated RowCount of
    // the inner table should be scaled down to match that reported in the
    // outer table prior to executing any additional join.
    //
    // Further, for index-driven nested joins, the base table columns'
    // statistics -- those associated with columns provided by the index
    // table as outer references -- are removed.  Their equivalent index
    // column stats provide the result stats for the join between the index
    // table and the base table.
    //
    // NOTE: this code assumes that all predicates that can be pushed down
    //       to 'identical' columns are pushed down to all copies of those
    //       columns.
    // ----------------------------------------------------------------------
    {
      // -----------------------------------------------------------------
      // First, determine if we seem to be in an index-driven nested join.
      // And simultaneously, make a copy of only those base table column statistics
      // that are not provided as outer references from the index table.
      // -----------------------------------------------------------------
      const ColStatDescList &outerColStatsList = inputEstLogProp->getColStats();
      outerRefCount = outerColStatsList.entries();
      CostScalar inputCard = inputEstLogProp->getResultCardinality();

      if (!outerRefCount)
      {
	// no outer references, copy all base columns
	columnStats.makeDeepCopy (childStats) ;
      }
      else
      {
	// copy only those columns which are not provided as outer references
	CollIndex i;
	for (i = 0; i < childStats.entries(); i++)
	{
	  ColStatDescSharedPtr columnStatDesc = childStats[i];
	  for (outerInd = 0; (Int32)outerInd < outerRefCount; outerInd++)
	    {
	      ColStatDescSharedPtr outerStatDesc = outerColStatsList[outerInd];

	      if (outerStatDesc->getMergeState().contains(
	              columnStatDesc->getMergeState()) )
	      {
		indexNestedJoin = TRUE;
		break;
	      }

	  }
	  if((Int32)outerInd == outerRefCount)
	  {
	    ColStatDescSharedPtr tmpColStatDescPtr(new (HISTHEAP)
                     ColStatDesc(*childStats[i]), HISTHEAP);
	    ColStatsSharedPtr tmpColStatsPtr = tmpColStatDescPtr->getColStatsToModify();
	    tmpColStatsPtr->copyAndScaleHistogram(1);
	    columnStats.insert(tmpColStatDescPtr );
	   }
	}
	// now set the Uec list of the copy from the original colStats
        columnStats.setUecList( childStats.getUecList() );

      }

      columnStats.setInputCard(inputCard);
      // After doing all these copies, we want to be sure that all
      // colStats have the same row count. It should assert if these are
      // different

#define MONITOR_SAMEROWCOUNT
        columnStats.verifyInternalConsistency( 0, columnStats.entries() );
#undef MONITOR_SAMEROWCOUNT

      // -----------------------------------------------------------------
      // Second, pre-pend the outer reference column statistics onto the
      // [remaining] child's column statistics.
      // -----------------------------------------------------------------
      CostScalar innerScale =
        (indexNestedJoin ||
         inputEstLogProp->getInputForSemiTSJ() != EstLogProp::NOT_SEMI_TSJ ?
         1 : newRowCount);

      // There can be cases where the outerColStatsList is empty, but there are
      // still rows coming into the right child. See Sol: 10-030327-5157.
      // For such cases the columnStats cardinality does not get adjusted even though
      // the input cardinality > 1. Sscale histograms with the input cardinality.
      if (outerRefCount > 0)
	columnStats.prependDeepCopy (outerColStatsList, // source ColStatDescList
                                   outerRefCount,     // prepend limit
                                   innerScale,        // scale prepended Rowcounts
                                   FALSE) ;           // clear shapeChanged flag
      else
	if (inputEstLogProp->getResultCardinality() > csOne) // no colStats to prepend, still the rowcount needs to be adjusted
	{
	  CostScalar inputRowcount = inputEstLogProp->getResultCardinality();
	  CollIndex colEntries = columnStats.entries();
	  for (CollIndex i = 0;  i < colEntries; i++)
	  {
	    ColStatDescSharedPtr columnStatDesc = columnStats[i];
	    ColStatsSharedPtr tempColumnStats = columnStatDesc->getColStatsToModify();
	    tempColumnStats->copyAndScaleHistogram ( inputRowcount );
	  }
	}


      if (columnStats.entries() > 0)
	newRowCount = columnStats[0]->getColStats()->getRowcount();

      // -----------------------------------------------------------------
      // Third, scale the original entries in the child's column stats. The
      // actual determination of how this is done is based on knowing
      // whether or not this is an index-driven nested join.....
      //
      // I.e., if it's an index-based NJ, then both RowCount and UEC of
      // the 'base' table are scaled; if it's not index-based, then the
      // scaling depends upon whether or not this scan is the right child
      // of a [anti-]semi-TSJ.  If not, scaling is done as with join
      // cross-products; otherwise (it *is* the R child of a [anti-]semiTSJ), the
      // right side isn't scaled.
      // For case where outerrefCount = 0, but innerScale is greater than 1, the
      // scaling of histograms has already been done.
      // -----------------------------------------------------------------
      CostScalar outerScale = 1;

      if (indexNestedJoin)
	{
	  for (CollIndex i = outerRefCount;  i < columnStats.entries(); i++)
	    {
	      ColStatDescSharedPtr columnStatDesc = columnStats[i];
              CostScalar oldCount = columnStatDesc->getColStats()->getRowcount();

	      if (oldCount != newRowCount)
		columnStatDesc->synchronizeStats (oldCount, newRowCount);
	    }
	}
      else // not an indexNestedJoin
	{
	  if (outerRefCount > 0 )
            {
	      // Either the method for doing the join, or the scale factor of
              // the remaining columns in the StatDescList, must be updated.
              if ( inputEstLogProp->getInputForSemiTSJ() == EstLogProp::SEMI_TSJ )
                mergeMethod = SEMI_JOIN_MERGE;
              else if ( inputEstLogProp->getInputForSemiTSJ() == EstLogProp::ANTI_SEMI_TSJ )
                mergeMethod = ANTI_SEMI_JOIN_MERGE;
              else
                outerScale = outerColStatsList[0]->getColStats()->getRowcount();
            }

	  for (CollIndex i = outerRefCount; i < columnStats.entries(); i++)
	    {
	      ColStatDescSharedPtr columnStatDesc = columnStats[i];
              ColStatsSharedPtr colStats = columnStatDesc->getColStatsToModify();
              colStats->copyAndScaleHistogram( outerScale );
	    }
	}
    } // If REL_SCAN


  const ValueIdSet & inputValues = getGroupAttr()->getCharacteristicInputs();
  ValueIdSet outerReferences;

  // Get the set of outer references.  If input value is a VEGReference,then
  // include this VEGReference only if the group consists of no constants.
  if (inputValues.entries() > 0)
    inputValues.getOuterReferences (outerReferences);

  // Apply the effect of my selection predicates on columnStats,
  // returning the resultant rowcount.
  const SelectivityHint * selHint = NULL;
  const CardinalityHint * cardHint = NULL;
  NABoolean sampleUsed = FALSE;

  if (opType == REL_SCAN)
  {
    Scan * scanExpr = (Scan *)this;
    selHint = scanExpr->getSelectivityHint();
    cardHint = scanExpr->getCardinalityHint();

    ColStatDescList columnStatsForMaxSel(CmpCommon::statementHeap());
    columnStatsForMaxSel.makeDeepCopy (columnStats) ;

    // Set MC Skewed values
    columnStats.setMCSkewedValueLists(childStats.getMCSkewedValueLists());

    // compute maxSelectivity. toss rowcount return value.
    (void) columnStatsForMaxSel.estimateCardinality
      (newRowCount, getSelectionPred(), outerReferences, NULL, selHint,
       cardHint, outerRefCount, myEstProps->unresolvedPreds(), 
       mergeMethod, opType, &maxSelectivity);

    newRowCount =
    columnStats.estimateCardinality (newRowCount,       /*in*/
                                      getSelectionPred(),/*in*/
                                      outerReferences,   /*in*/
                                      NULL,
                                      selHint,		/*in*/
                                      cardHint,		/*in*/
                                      outerRefCount,     /*in/out*/
                                      myEstProps->unresolvedPreds(), /*in/out*/
                                      mergeMethod,
                                      opType,      /*in*/
                                      NULL);

    // if cardinality or selectivity hint is not given, then 
    // check to see if the computed cardinality is within
    // the cardinality range obtained from executing the same
    // query on a sample
    if (scanExpr && !cardHint && !selHint && !columnStats.selectivityHintApplied())
    {
      sampleUsed = scanExpr->checkForCardRange(getSelectionPred(), newRowCount);
    }
  }
  else
  {
  newRowCount =
    columnStats.estimateCardinality (newRowCount,       /*in*/
		                      getSelectionPred(),/*in*/
		                      outerReferences,   /*in*/
                                      NULL,
		                      NULL,		/*in*/
		                      NULL,
		                      outerRefCount,     /*in/out*/
		                      myEstProps->unresolvedPreds(), /*in/out*/
		                      mergeMethod,      /*in*/
                                      ITM_FIRST_ITEM_OP); /*no-op*/
  }

  // Cardinality after applying predicates should not be more than
  // the product of cardinality before applying predicate multiplied
  // by the input cardinality. Sol:10-070502-4521. We do this, only
  // if there are no hints defined for this table. In case of hints
  // this sanity check is ignored

  if (!sampleUsed && !cardHint && !selHint)
  {
    CostScalar inputCard = inputEstLogProp->getResultCardinality();
    NABoolean rollingColumnPresent = FALSE;
    if(newRowCount > (inputCard * initialCardinality))
    {
      for ( CollIndex i = 0; i < columnStats.entries(); i++ )
      {
	if((columnStats)[i]->getColStats()->isARollingColumn())
	{
	  rollingColumnPresent = TRUE;
	  break;
	}
      }
    }
    
    if(!rollingColumnPresent)
      newRowCount = MINOF(newRowCount, inputCard * initialCardinality);
  }

  // ++MV
  if (getInliningInfo().isForceCardinality())
  {
    CostScalar oldCount = newRowCount;
    // Override the estimated cardinality using the inlining information.
    newRowCount = getInliningInfo().getNewCardinality(initialCardinality);
    if (oldCount != newRowCount)
    {
      columnStats.synchronizeStats(newRowCount, columnStats.entries());
    }
  }
  else
    columnStats.synchronizeStats(newRowCount, columnStats.entries());
  // --MV

  myEstProps->setResultCardinality( newRowCount );
  if (cardHint != NULL)
    myEstProps->setMaxCardEst( newRowCount );
  else if (selHint != NULL && selHint->getScanSelectivityFactor() > 0.0)
    myEstProps->setMaxCardEst(maxRows * selHint->getScanSelectivityFactor());
  else 
    myEstProps->setMaxCardEst(MAXOF(maxRows * maxSelectivity, newRowCount));
  // -------------------------------------------------------------------
  // Now, determine which of these histograms are useful based on the
  // characteristic outputs for the group, and finish up.
  // We need to handle some special cases:
  // 1) if the expression is update then we need to generate colStats for
  //    the characteristics outputs of the UpdateExpression. Each of these
  //    should map to the corresponding colStats which are picked up based
  //    on the characteristic outputs of the child of Update
  // 2) If there are some VEG predicates on this operator, which contain a
  //    reference to a column of another table, and also a constant
  //    then the histograms for these should be passed too, even though
  //    the column does not showup as a characteristics output. The reason
  //    for doing this is because we might need it later while doing a
  //    nested join between the two tables.
  // -------------------------------------------------------------------

  OperatorTypeEnum statOper = getOperatorType();
  ValueIdSet charOutputs = getGroupAttr()->getCharacteristicOutputs();

  if ( (statOper == REL_UNARY_INSERT) || (statOper == REL_LEAF_INSERT) ||
	(statOper == REL_UNARY_DELETE ) || (statOper == REL_LEAF_DELETE ) ||
	 (statOper == REL_UNARY_UPDATE) || (statOper == REL_LEAF_UPDATE))
  {
    GenericUpdate *updateExpr = (GenericUpdate *) this;

    // do it only if the update expression is on a normal table
    if (updateExpr->getTableName().getSpecialType() == ExtendedQualName::NORMAL_TABLE)
    {
      myEstProps->pickOutputsForUpdate(columnStats, inputEstLogProp, (*this), charOutputs, getSelectionPred());
    }
    else
     myEstProps->pickOutputs( columnStats, inputEstLogProp, charOutputs, getSelectionPred());
  }
  else
    myEstProps->pickOutputs( columnStats, inputEstLogProp, charOutputs, getSelectionPred());

  return (myEstProps);

} // RelExpr::synthEstLogPropForUnaryLeafOp()

void
RelExpr::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  // create a default OLP with rowcount of 1.
  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(1));

  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstProps);
}

void
RelExpr::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent logical expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  GroupAttributes * GA = getGroupAttr();

  Int32 numChildren = getArity();
  Int32 numBaseTables = 0;   // add up all base tables from children
  Int32 numTMUDFs = 0;       // ditto for TMUDFs
  // synthesize the log props for all children
  for (Lng32 i = 0; i < numChildren; i++)
  {
    // only if the child is not a Cascadesgroup or NULL
    if (child(i).getPtr() != NULL)
    {
      child(i)->synthLogProp(normWAPtr);
      numBaseTables += child(i).getGroupAttr()->getNumBaseTables();
      numTMUDFs += child(i).getGroupAttr()->getNumTMUDFs();

      // QSTUFF
      // synthesize logical properties STREAM and DELETE from
      // children. The properties are synthesized by the BINDER for
      // semantic checking and by the OPTIMIZER for rule enforcement
      if (child(i).getGroupAttr()->isStream())
      {
        GA->setStream(TRUE);
        if (child(i).getGroupAttr()->isSkipInitialScan())
            getGroupAttr()->setSkipInitialScan(TRUE);
      }
      if (child(i).getGroupAttr()->isEmbeddedUpdateOrDelete())
        GA->setEmbeddedIUD(child(i).getGroupAttr()->getEmbeddedIUD());
      // QSTUFF ^^

      if (child(i).getGroupAttr()->getHasNonDeterministicUDRs())
        getGroupAttr()->setHasNonDeterministicUDRs(TRUE);

      if (child(i).getGroupAttr()->isSeabaseMDTable())
        getGroupAttr()->setSeabaseMDTable(TRUE);
    }
  }

  // remember the number of base tables

  // remember the number of base tables and TMUDFs
  GA->setNumBaseTables(numBaseTables);
  GA->setNumTMUDFs(numTMUDFs);
  // calculate the row size from the characteristic outputs
  GA->setRecordLength(GA->getCharacteristicOutputs().getRowLength());
  GA->setInputVarLength(GA->getCharacteristicInputs().getRowLength());

  // Associate this GA with this log expr for any further synthesis
  GA->setLogExprForSynthesis (this);

  //++MV
  // Used by inlining mechanism for adding optimizer constraints
  if (!getUniqueColumns().isEmpty())
  {
    GA->addConstraint(new(CmpCommon::statementHeap())
	       UniqueOptConstraint(getUniqueColumns()));
  }

  if (getCardConstraint())
  {
    GA->addConstraint(new(CmpCommon::statementHeap())
			 CardConstraint(getCardConstraint()->getLowerBound(),
					getCardConstraint()->getUpperBound()));
  }
  //--MV
} // RelExpr::synthLogProp()

void
RelExpr::finishSynthEstLogProp()
{
  GroupAttributes * GA = getGroupAttr();

  Int32 numChildren = getArity();
  CostScalar minRowCount = COSTSCALAR_MAX;

  // finish synthesis of log props for all children
  for (Lng32 i = 0; i < numChildren; i++)
  {
    // only if the child is not a CascadesGroup or NULL
    if (child(i).getPtr() != NULL)
    {
      child(i)->finishSynthEstLogProp();

      // set the minimum rowCount after applying local predicates for this group
      CostScalar thisChildsRowCount = child(i)->getGroupAttr()->getMinChildEstRowCount();

      if (thisChildsRowCount < minRowCount)
	    minRowCount = thisChildsRowCount;
    }
  }

  GA->setMinChildEstRowCount(minRowCount);

} // RelExpr::finishSynthEstLogProp()


// clear logical properties of me and my children
void
RelExpr::clearAllEstLogProp()
{
  Int32 numChildren = getArity();

  // clear the log props for all children
  for (Lng32 i = 0; i < numChildren; i++)
  {
    // only if the child is not a CascadesGroup or NULL
    if (child(i).getPtr() != NULL)
    {
      child(i)->clearAllEstLogProp();
	}
  }

  // finally clear my logical properties
  getGroupAttr()->clearAllEstLogProp();

  // for scan nodes clear the colStats_ from tableDesc, in order to use
  // compressed histograms in future

  if (getOperatorType() == REL_SCAN)
  {
	// for scan nodes clear the histograms compressed flag to FALSE
	// so that the histograms can be compressed later
	Scan * scanExpr = (Scan *)this;

	scanExpr->getTableDesc()->histsCompressed(FALSE);
  }
}

RelExpr *
RelExpr::checkAndReorderTree()
{
  // ---------------------------------------------------------------------
  //  Reorder any join orderings based on estimated rowcount.
  //  We do not re-order if the control JOIN_ORDER_BY_USER is ON since
  //  this specify that we want the join order that was output from
  //  the normalizer, unchanged.
  // ---------------------------------------------------------------------
  //  Here am using direct call to NADefaults instead of the much more
  //  efficient way of using the CURRSTMT_OPTDEFAULTS->joinOrderByUser() function.
  //  The reason is that OptDefaults has not been initialized yet.
  //  It's not a big deal since this is happening only once per statement.
  //
  //  The path of reorderTree is used also to perform the trigger

  //  is not required.

  NABoolean treeModified = FALSE;

  NABoolean areTriggersPresent = getOperatorType() == REL_ROOT &&
    ((RelRoot *)this)->getTriggersList() != NULL;
  NABoolean containsOnStatementMV = getOperatorType() == REL_ROOT &&
    ((RelRoot *)this)->containsOnStatementMV();

  NABoolean doReorderJoin = 
        ((ActiveSchemaDB()->getDefaults()).getAsULong(COMP_INT_90)!=0) ||
        getGroupAttr()->reorderNeeded() ||
        areTriggersPresent || containsOnStatementMV;

  if (!(QueryAnalysis::Instance()->joinOrderByUser())) 
  {
     if (doReorderJoin && 
         ((CURRSTMT_CQSWA == NULL) ||
          CURRSTMT_CQSWA && !CURRSTMT_CQSWA->reArrangementSuccessful_)
        )
    {
       this->reorderTree(treeModified, doReorderJoin);
       this->clearAllEstLogProp();

       if (treeModified)
          synthLogProp();
    }
  }
  else{
    if (getGroupAttr()->reorderNeeded())
      *CmpCommon::diags() << DgSqlCode(-4174);
  }
  // QSTUFF

/*
  NABoolean doReorderJoin = FALSE;

  NABoolean areTriggersPresent = getOperatorType() == REL_ROOT &&
    ((RelRoot *)this)->getTriggersList() != NULL;
  NABoolean containsOnStatementMV = getOperatorType() == REL_ROOT &&
    ((RelRoot *)this)->containsOnStatementMV();

  if (!(QueryAnalysis::Instance()->joinOrderByUser()) &&
      ((ActiveSchemaDB()->getDefaults()).getAsULong(COMP_INT_90)!=0 
       || getGroupAttr()->isStream() 
       || areTriggersPresent
       || containsOnStatementMV))
	doReorderJoin = TRUE;
  // QSTUFF
  else{
    if (getGroupAttr()->reorderNeeded())
      *CmpCommon::diags() << DgSqlCode(-4174);
  }
  // QSTUFF

  if ( (doReorderJoin && (CURRSTMT_CQSWA == NULL)) ||
       (doReorderJoin &&
        CURRSTMT_CQSWA &&
        !CURRSTMT_CQSWA->reArrangementSuccessful_)
     )
	{
	  this->reorderTree(treeModified, doReorderJoin);
	  this->clearAllEstLogProp();

	  if (treeModified)
		synthLogProp();
	}
*/

  return(this);

}
// The reorderTree() is used also for pre-optimizer pass
RelExpr *
RelExpr::reorderTree(NABoolean & treeModified, NABoolean doReorderJoin)
{
  Int32 nc = getArity();
  NABoolean childModified = FALSE;
  treeModified = FALSE;

  // reorder any join backbones in the children
  for (Lng32 i = 0; i < nc; i++)
  {
    child(i) = child(i)->reorderTree(childModified, doReorderJoin);
    if (childModified)
      treeModified = TRUE;
  }

  // if any of my children were reordered, then we need to
  // clear my logical properties for resynthesis.
  if (treeModified)
    getGroupAttr()->clearLogProperties();

  return (this);
} // RelExpr::reorderTree()

RelExpr *
RelExpr::fixupTriggers(NABoolean & treeModified)
{
  Int32 nc = getArity();
  NABoolean childModified = FALSE;
  NABoolean myTreeModified = FALSE;

  // reorder any join backbones in the children
  for (Lng32 i = 0; i < nc; i++)
  {
    child(i) = child(i)->fixupTriggers(childModified);
    if (childModified)
      myTreeModified = TRUE;
  }

  // if any of my children were reordered, then we need to
  // clear my logical properties for resynthesis.
  if (myTreeModified)
    getGroupAttr()->clearLogProperties();

  treeModified = treeModified || myTreeModified;

  return (this);
} // RelExpr::fixupTriggers()

// The fixupTriggers() is used for pre-optimizer pass
// for triggers
// After trigger backbone transformation
RelExpr *
Union::fixupTriggers(NABoolean & treeModified)
{
  RelExpr *retNode = this;
  NABoolean myTreeModified = FALSE;

  #ifndef NDEBUG
  if (getenv("TRIGGER_TRANSFORMATION_OFF") != NULL)
    return (RelExpr::fixupTriggers(treeModified));
  #endif

  InliningInfo &InlineInfo = getInliningInfo();

  if (InlineInfo.isDrivingBeforeTriggers() ||
    (InlineInfo.isDrivingTempDelete() && !InlineInfo.isBeforeTriggersExist()))
  {
    OptTriggersBackbone *backbone = new(CmpCommon::statementHeap())
      OptTriggersBackbone(this);
    backbone->init();
    RelExpr *topNode = backbone->getTransformedTree();
    NADELETE(backbone, OptTriggersBackbone, (CmpCommon::statementHeap()));

    if (topNode != this) // top node has been removed
    {
      myTreeModified = TRUE;
      retNode = topNode;
    }
  }

  RelExpr * retVal = retNode->RelExpr::fixupTriggers(myTreeModified);
  
  if(myTreeModified)
    getGroupAttr()->clearLogProperties();

  treeModified = treeModified || myTreeModified;

  return retVal;
} // Union::fixupTriggers()
// end - for triggers

// -----------------------------------------------------------------------
// member functions for class CutOp
// -----------------------------------------------------------------------

void
CutOp::synthLogProp(NormWA * normWAPtr)
{
}

// ----------------------------------------------------------------------
// Helper method for left and full outer joins
// The method simulates the behavior of left outer joins
// ----------------------------------------------------------------------
void
Join::instantiateLeftHistsWithNULLs(ValueIdSet EqLocalPreds, /*in*/
                                    CollIndex outerRefCount, /*in*/
                                    ColStatDescList &joinStatDescList, /*in*/
                                    const ColStatDescList &leftColStatsList, /*in*/
                                    CostScalar &oJoinResultRows /*in, out*/,
                                    CostScalar initialRowcount /*in*/,
                                    CollIndex &rootStatIndex /*out*/,
                                    NAList<CollIndex>& statsToMerge /*out*/)
{
  // contains the rowcount of the left child before join
  CostScalar baseRows;
  // contains the innerJoinCardinality
  CostScalar innerJoinCard = oJoinResultRows;
  // Flag to indicate that the histogram from the left side that needs to
  // be NULL Instantiated is found. 
  NABoolean foundFlag = FALSE;

  // locate Histogram that participated in equiJoin to determine the number of rows
  // to NULL augment. If no histogram participated in equiJoin, look for a histogram 
  // whose shape changed during join. This could be like join on expressions
  // rootIndex contains the index of the histogram to be NULL augmented first
  foundFlag = joinStatDescList.locateHistogramToNULLAugment(EqLocalPreds, 
                                                          statsToMerge,
                                                          rootStatIndex,
                                                          outerRefCount);

  // Use the above histogram to estimate the outer join result. This is done by 
  // determining the number of rows from the left / right side that did 
  // not participate in the inner join
  // found flag indicates if an error occured while trying to compute the number
  // of rows. It is an input as well as an output parameter
  // if there were no histogram with join predicate found to start with, then
  // there is nothing to do
  if (foundFlag)
  {
    baseRows = joinStatDescList[rootStatIndex]->getColStats()->getRowcount();

    oJoinResultRows = joinStatDescList.computeLeftOuterJoinRC(foundFlag, 
                                                              leftColStatsList, 
                                                              rootStatIndex);
  }

  // if any hist with equality predicate or any shape changing predicate was
  // found, set baseRowCount as the rowcount of the histogram. This would be 
  // the result after an inner join.
  // if foundFlag was false, that is we did not find any histogram with
  // predicate, then set the baseRows equal to original left rowcount
  
  if (!foundFlag)
  {
    // Exception condition, set condition so no histograms could be merged
    // ------------------------------------------------------------
    // If this code is hit, no histogram shape-changing predicates
    // (or none that we currently know how to apply) were found.
    // In this [hopefully] unusual case, set the baseRowcount as 
    // the initialRowCount, which is the cross product of the two
    // children of Join
    // Since it is an exception, we shall set the oJoinResultRows as 
    // the max of inner join cardinality and original leftRowCount
    //
    // Set up a situation where the null-augmentation logic does
    // the desired thing:
    //    clear the statsToMerge set, and
    //    pick a rootStatIndex to indicate that no special root
    //      histogram was found.
    // ------------------------------------------------------------
    CCMPASSERT ("Histograms for left table in outer join missing");
    baseRows        = initialRowcount;
    statsToMerge.clear();  // reset.
    CostScalar lrc = csOne;
    if (leftColStatsList.entries() > 0)
      lrc = leftColStatsList[0]->getColStats()->getRowcount();
    oJoinResultRows = MAXOF(innerJoinCard, lrc);
    rootStatIndex = leftColStatsList.entries() + 1;  // out of range; +1 overkill
  }

  // NULL augment the histograms that did not match the other side
  // synchronize histograms from left table to all have the same rowcount
  joinStatDescList.synchronizeHistsWithOJRC(statsToMerge, 
                                          0,
                                          outerRefCount,
                                          rootStatIndex, 
                                          leftColStatsList,
                                          oJoinResultRows, 
                                          baseRows);

  // ------------------------------------------------------------
  // Next, Instantiate Nulls in histograms from the right table.
  // Also, alter the discriptions the histograms have of themselves
  // such that they indicate that they are null-instantiated.
  // ------------------------------------------------------------
  ValueIdList nulledVIds = nullInstantiatedOutput();

  // At this point all joined histograms have the intervals from the 
  // left side have merged. Now NULL Instantiate histograms
  joinStatDescList.nullInstantiateHists(outerRefCount, 
                                        joinStatDescList.entries(), 
                                        oJoinResultRows, 
                                        nulledVIds);

  return;
}  // instantiateLeftHistsWithNULLs

// ----------------------------------------------------------------------
// Helper method for left and full outer joins
// The method simulates the behavior of left and full outer joins
// depending on the fullOuterJoinMethod. The flag = TRUE means that
// it is the second part of full outer join, where right histograms
// are being NULL instantiated
// ----------------------------------------------------------------------
void
Join::instantiateRightHistsWithNULLs(CollIndex outerRefCount, /*in*/
                                    ColStatDescList &innerJoinCopy, /*in/out*/
                                    ColStatDescList &joinStatDescList, /*in/out*/
                                    const ColStatDescList &rightColStatsList, /*in*/
                                    CostScalar &oJoinResultRows /*in, out*/,
                                    CostScalar initialRowcount /*in*/,
                                    CollIndex rootStatIndex /*in*/,
                                    NAList<CollIndex> statsToMerge /*out*/)
{
  // contains the leftJoin cardinality
  CostScalar leftJoinCard = oJoinResultRows;
  // contains the number of right rows that did not match the left
  CostScalar rightJoinCard = csMinusOne;
  // contains inner join cardinality
  CostScalar innerJoinCard = csMinusOne;
  // Flag to indicate that the histogram from the left side that needs to
  // be NULL Instantiated is found. 
  NABoolean foundFlag = FALSE;
  // baseRowCount contains the rowcount with which we started. For full outerjoin
  // we have already computed left join cardinality
  CostScalar baseRows = leftJoinCard;

  // Use the histogram to estimate the outer join result. This is done by 
  // determining the number of rows from the right side that did 
  // not participate in the inner join
  if (statsToMerge.entries() > 0)
  {
    innerJoinCard = innerJoinCopy[rootStatIndex]->getColStats()->getRowcount();
    rightJoinCard = innerJoinCopy.computeFullOuterJoinRC(foundFlag, 
                                                         rightColStatsList, 
                                                         rootStatIndex);
    // Final full outer join cardinality 
    // = inner join card + left rows that did not match inner + right rows that did not match inner
    // Since inner got accounted twice, we shall reduce one inner
    // Hence fullOuterJoinCard = leftJoinCard + rightJoinCard - InnerJoinCard
    oJoinResultRows = leftJoinCard + rightJoinCard - innerJoinCard;
  }

  // In case of an error set the baseRows equal to original left rowcount
  
  // if there were no histogram with join predicate found to start with, then
  // there is nothing to do
  if (!foundFlag)
  {
    // Exception condition, set condition so no histograms could be merged
    // ------------------------------------------------------------
    // If this code is hit, no histogram shape-changing predicates
    // (or none that we currently know how to apply) were found.
    // In this [hopefully] unusual case, set the baseRowcount as 
    // the initialRowCount, which is the cross product of the two
    // children of Join
    // Since it is an exception, we shall set the oJoinResultRows as 
    // the max of left join cardinality and original rightRowCount
    //
    // Set up a situation where the null-augmentation logic does
    // the desired thing:
    //    clear the statsToMerge set, and
    //    pick a rootStatIndex to indicate that no special root
    //      histogram was found.
    // ------------------------------------------------------------
    CCMPASSERT ("Histograms from right table in full outer join missing");
    baseRows        = initialRowcount;
    statsToMerge.clear();  // reset.
    rootStatIndex = joinStatDescList.entries() + 1;  // out of range; +1 overkill
    CostScalar rrc = csOne;
    if (rightColStatsList.entries() > 0)
      rrc = rightColStatsList[0]->getColStats()->getRowcount();
    oJoinResultRows = MAXOF(leftJoinCard, rrc);
  }

  // NULL augment the histograms that did not match the other side
  // synchronize histograms from left / right table to all have the same rowcount
  // computed so far for full outer joins
  joinStatDescList.synchronizeHistsWithOJRC(statsToMerge, 
                                            outerRefCount,
                                            joinStatDescList.entries(),
                                            rootStatIndex, 
                                            rightColStatsList,
                                            oJoinResultRows, 
                                            baseRows);

  // ------------------------------------------------------------
  // Next, Instantiate Nulls in histograms from the left table.
  // Also, alter the descriptions the histograms have of themselves
  // such that they indicate that they are null-instantiated.
  // ------------------------------------------------------------
  ValueIdList nulledVIds = nullInstantiatedForRightJoinOutput();
  
  joinStatDescList.nullInstantiateHists(0, outerRefCount, oJoinResultRows, nulledVIds);

  return;
}  // instantiateRightHistsWithNULLs

// -----------------------------------------------------------------------
// Join::commonSynthEst is shared logic for Join::synthEstLogPropForTSJ
// and Join::synthEstLogProp.
// -----------------------------------------------------------------------
CostScalar
Join::commonSynthEst (const EstLogPropSharedPtr& myEstProps,
                      const EstLogPropSharedPtr& intermedEstProps,
                      CollIndex & outerRefCount,
                      const ColStatDescList &leftColStatsList,
                      const ColStatDescList &rightColStatsList,
                      ColStatDescList & joinStatDescList,
                      CostScalar initialRowcount)
{
  // CMPASSERT (outerRefCount == leftColStatsList.entries() ) ; // should always be true!

  // Get the set of outer references.  If input value is a VEGReference,
  // include this VEGReference only if the group consists of no constants.
  const ValueIdSet & inputValues = getGroupAttr()->getCharacteristicInputs();
  ValueIdSet outerReferences;

  if (inputValues.entries() > 0)
    inputValues.getOuterReferences (outerReferences);

  // if the join is an outer join, gather some information (to be used
  // later) regarding the original join predicates.
  ValueIdSet EqLocalPreds, OtherLocalPreds, EqNonLocalPreds,
             OtherNonLocalPreds, BiLogicPreds, DefaultPreds;

  if ( isLeftJoin() || isFullOuterJoin() )
    getJoinPred().categorizePredicates (outerReferences, EqLocalPreds,
					OtherLocalPreds, EqNonLocalPreds,
					OtherNonLocalPreds, BiLogicPreds,
					DefaultPreds);

  MergeType mergeMethod = getMergeTypeToBeUsedForSynthLogProperties();

  // approximate the cardinality of the join
  CostScalar newRowcount = initialRowcount;

  //---------------------------------------------------------------------
  // Apply the local join predicate; return the resultant rowcount.
  // In the case of Outer joins, this join is done as an Inner join.
  //---------------------------------------------------------------------
  newRowcount =
    joinStatDescList.estimateCardinality (newRowcount,       /*in*/
                                          getJoinPred(),     /*in*/
                                          outerReferences,   /*in*/
                                          this,
                                          NULL,              /*for selectivityHint, which can only be given for Scan*/
                                          NULL,              /* for CardinalityHint, which is NULL for ! Scan operators */
                                          outerRefCount,     /*in/out*/
                                          myEstProps->unresolvedPreds(), /*in/out*/
                                          mergeMethod,      /*in*/
                                          REL_JOIN);


  // make a copy of innerJoinCard result which will be used later to compute
  // full outerjoin cardinality;
  ColStatDescList innerJoinCopy;
  if(isFullOuterJoin())
    innerJoinCopy.makeDeepCopy( joinStatDescList );
  //---------------------------------------------------------------------
  // Save an intermediate EstLogProp for use by Physical Costing.
  // As usual, this is a Deep copy to prevent inadvertent modifications
  // of the data, by subsequent changes to the Histograms.
  //---------------------------------------------------------------------
  ColStatDescList & intermedStatsList = intermedEstProps->colStats();

  intermedStatsList.makeDeepCopy (joinStatDescList) ;

  intermedEstProps->unresolvedPreds() += myEstProps->getUnresolvedPreds();
  intermedEstProps->setResultCardinality( newRowcount );

  //---------------------------------------------------------------------
  // IF this is an outer join, whether left or full, fix-up the result of
  // the inner join to reflect its outer join nature:
  //  - recreate what left rows didn't meet the join predicate.
  //  - 'OR' those rows, null-augmented on right, into the join results
  // newRowcount contains the left join rowcount. 
  //---------------------------------------------------------------------
  // contains the list of indices of histograms that should be
  // NULL instantiated
  NAList<CollIndex> statsToMerge(CmpCommon::statementHeap());
  // contains the index of the left histogram to be merged
  CollIndex rootStatIndex = NULL_COLL_INDEX;

  // save the inner join cardinality
  CostScalar innerJoinCard = newRowcount;

  if ( mergeMethod == OUTER_JOIN_MERGE )
  {
    instantiateLeftHistsWithNULLs(EqLocalPreds, /*in*/
                                  outerRefCount, /*in*/
                                  joinStatDescList, /*in*/
                                  leftColStatsList, /*in*/
                                  newRowcount /*in, out*/,
                                  initialRowcount /*in*/,
                                  rootStatIndex,
                                  statsToMerge);

  }  // if (left or full outer join)

  // ---------------------------------------------------------------------
  // if this is a full outer join,
  //  - recreate what right rows didn't meet the join predicate.
  // start with the inner join and find the number of rows from the right
  // that did not match the left side (innerJoinCopy)
  //  - 'OR' those rows, null-augmented on the left, into the join results
  // ---------------------------------------------------------------------
  if ( isFullOuterJoin() )
  {
    instantiateRightHistsWithNULLs(outerRefCount, /*in*/
                                    innerJoinCopy, /*in*/
                                    joinStatDescList, /*in */
                                    rightColStatsList, /*in*/
                                    newRowcount /*in, out*/,
                                    initialRowcount /*in*/,
                                    rootStatIndex,
                                    statsToMerge);
  }

  // We want to recompute the max frequency as the NULL interval added 
  // may become the most frequent value
  if ( mergeMethod == OUTER_JOIN_MERGE &&
       CmpCommon::getDefault(HIST_FREQ_VALS_NULL_FIX) == DF_ON )
    joinStatDescList.computeMaxFreq(TRUE);

  // ---------------------------------------------------------------------
  // Lastly, apply the effect of my selection predicates on joinStatDescList,
  // returning the resultant rowcount.
  //
  // If we had been doing an outer join, this where-clause predicate is done
  // as an inner join.
  // ---------------------------------------------------------------------
  if (mergeMethod == OUTER_JOIN_MERGE)
    mergeMethod = INNER_JOIN_MERGE;

  newRowcount =
    joinStatDescList.estimateCardinality (newRowcount,       /*in*/
                                          getSelectionPred(),/*in*/
                                          outerReferences,   /*in*/
                                          this,
                                          NULL,              /*for selectivityHint, which can only be given for Scan*/
                                          NULL,              /* for Cardinality hint, which can only be given for Scan */
                                          outerRefCount,     /*in/out*/
                                          myEstProps->unresolvedPreds(), /*in/out*/
                                          mergeMethod,
                                          REL_JOIN);      /*in*/

  return (newRowcount);

} // commonSynthEst

// -----------------------------------------------------------------------
// Join::synthEstLogPropForTSJ
// -----------------------------------------------------------------------
void
Join::synthEstLogPropForTSJ(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp)) return;

  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(1));
  EstLogPropSharedPtr intermedEstProps(new (HISTHEAP) EstLogProp(1));

  EstLogPropSharedPtr leftEstProps;
  EstLogPropSharedPtr rightEstProps;
  EstLogPropSharedPtr copyInputEstProp;

  // if inputForSemiTSJ is set for inputEstLogProp it cannot be passed to child(0)
  //   however it must be passed to child(1)
  //   so create a new EstLogProp with the flag set off to pass to child(0)
  if (inputEstLogProp->getInputForSemiTSJ() != EstLogProp::NOT_SEMI_TSJ)
    {
      copyInputEstProp = EstLogPropSharedPtr(new (HISTHEAP) EstLogProp(*inputEstLogProp));
      copyInputEstProp->setInputForSemiTSJ(EstLogProp::NOT_SEMI_TSJ);
    }
  else
    {
      copyInputEstProp = inputEstLogProp;
    }

  leftEstProps = child(0).outputLogProp(copyInputEstProp);

  EstLogPropSharedPtr copyLeftEstProps(new (HISTHEAP) EstLogProp (*leftEstProps));

  // from this point on, it is important to utilize the left's copy.
  if ( isSemiJoin() ||
       inputEstLogProp->getInputForSemiTSJ() == EstLogProp::SEMI_TSJ)
    copyLeftEstProps->setInputForSemiTSJ( EstLogProp::SEMI_TSJ );
  else if ( isAntiSemiJoin() ||
            inputEstLogProp->getInputForSemiTSJ() == EstLogProp::ANTI_SEMI_TSJ)
    copyLeftEstProps->setInputForSemiTSJ( EstLogProp::ANTI_SEMI_TSJ );

  //Compute the right child output estLogProps for the given left child EstLogProp
  rightEstProps = child(1).outputLogProp(copyLeftEstProps);

  const ColStatDescList &leftColStatsList  = copyLeftEstProps->getColStats();
  const ColStatDescList &rightColStatsList = rightEstProps->getColStats();

  // Initially, the unresolved predicates of a join are those that are
  // unresolved by either child.
  myEstProps->unresolvedPreds() += copyLeftEstProps->getUnresolvedPreds();
  myEstProps->unresolvedPreds() += rightEstProps->getUnresolvedPreds();

  //------------------------------------------------------------------
  // Copy the Right child's ColStatDescs
  //------------------------------------------------------------------
  ColStatDescList joinStatDescList;
  joinStatDescList.makeDeepCopy (rightColStatsList) ;

  CollIndex i;
  CollIndex outerRefCount = 0;

  for (i = 0; i < rightColStatsList.entries(); i++)
    {
      ColStatDescSharedPtr joinStatDesc = joinStatDescList[i] ;

      ItemExpr * statExpr = joinStatDesc->getVEGColumn().getItemExpr() ;

      // -----------------------------------------------------------------
      // Determine the number of entries in rightEstProps's column
      // stats that match entries in the copyLeftEstProps columns.
      //
      // The first of these matching stats originated from the given
      // inputEstLogProp column stats, and the remaining are colstats from
      // the left table that are modified and returned by the right.  That
      // determined count will be used by the right child as the count of
      // Outer reference columns
      // -----------------------------------------------------------------
      NABoolean goodMatch = FALSE;
      for (CollIndex j = 0; j < leftColStatsList.entries() ; j++)
	{
	  ColStatDescSharedPtr origStatDesc = leftColStatsList[j];

	  ItemExpr * originalExpr = origStatDesc->getVEGColumn().getItemExpr() ;

	  if ( statExpr == originalExpr )
          {
            goodMatch = TRUE;
            break ; // break out of inner for-loop
          }
	}

      if (goodMatch)
	      outerRefCount++;
      else
	      break ;
    }

  CostScalar originalLeftRowcount = copyLeftEstProps->getResultCardinality();
  CostScalar newRowcount = rightEstProps->getResultCardinality();

  newRowcount = this->commonSynthEst (myEstProps,
                                      intermedEstProps,
                                      outerRefCount,
                                      leftColStatsList,
                                      rightColStatsList,
                                      joinStatDescList,
                                      newRowcount) ;

  // synchronizeStat only if needed
  NABoolean synchronizeStatNeeded = FALSE;

  CostScalar oldCount = newRowcount;
  // ++MV
  if (getInliningInfo().isForceCardinality())
  {
    // Override the estimated cardinality using the inlining information.
    newRowcount = getInliningInfo().getNewCardinality(newRowcount);
  }

  // since the histograms have been merged together. Set the scale factor of remaining 
  // histograms to one.
  joinStatDescList.setScaleFactor(csOne);

  newRowcount = doCardSanityChecks(copyInputEstProp,
                                   joinStatDescList,
                                   newRowcount);
  CostScalar maxCardEst = estimateMaxCardinality
    (copyInputEstProp, joinStatDescList);

  if (oldCount != newRowcount)
    joinStatDescList.synchronizeStats (newRowcount, joinStatDescList.entries()) ;

  myEstProps->setResultCardinality(newRowcount);
  myEstProps->setMaxCardEst(MAXOF(maxCardEst, newRowcount));

  //------------------------------------------------------------------
  // Now, determine which of these histograms are useful based on
  // the characteristic outputs for the group
  // -----------------------------------------------------------------

  const ValueIdSet charOutputs = getGroupAttr()->getCharacteristicOutputs();

  myEstProps->pickOutputs( joinStatDescList, inputEstLogProp, charOutputs, getSelectionPred()) ;

  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstProps,
					 intermedEstProps);

} // Join::synthEstLogPropForTSJ()

// -----------------------------------------------------------------------
//  Join::synthEstLogProp
// -----------------------------------------------------------------------
void
Join::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{

  if (isTSJ())
  {
    synthEstLogPropForTSJ(inputEstLogProp);
    return;
  }

  if (getGroupAttr()->isPropSynthesized(inputEstLogProp))
      return;

  const NABoolean isASemiJoin = isSemiJoin() || isAntiSemiJoin();

  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp());
  EstLogPropSharedPtr intermedEstProps(new (HISTHEAP) EstLogProp());

  EstLogPropSharedPtr leftEstProps;
  EstLogPropSharedPtr rightEstProps;
  EstLogPropSharedPtr copyInputEstProp;

  // if inputForSemiTSJ is set for inputEstLogProp it cannot be passed below the join
  //   so create a new EstLogProp with the flag set off to pass to my children
  if (inputEstLogProp->getInputForSemiTSJ() != EstLogProp::NOT_SEMI_TSJ)
    {
      copyInputEstProp = EstLogPropSharedPtr(new (HISTHEAP)
                EstLogProp(*inputEstLogProp));
      copyInputEstProp->setInputForSemiTSJ(EstLogProp::NOT_SEMI_TSJ);
    }
  else
    {
      copyInputEstProp = inputEstLogProp;
    }

  leftEstProps  = child(0).outputLogProp(copyInputEstProp);
  rightEstProps = child(1).outputLogProp(copyInputEstProp);

  // Initiallly, the unresolved predicates of a join are those that are
  // unresolved by either child.
  myEstProps->unresolvedPreds() += leftEstProps->getUnresolvedPreds();
  myEstProps->unresolvedPreds() += rightEstProps->getUnresolvedPreds();

  //------------------------------------------------------------------
  // access the column statistics descriptor list from the left and
  // right child's estimated logical properties.
  //------------------------------------------------------------------
  const ColStatDescList &leftColStatsList = leftEstProps->getColStats();
  const ColStatDescList &rightColStatsList = rightEstProps->getColStats();

  ColStatDescList outerColStatsList = inputEstLogProp->getColStats();

  ColStatDescList joinStatDescList;
  // set up the ueclist data member!
  joinStatDescList.insertIntoUecList (leftColStatsList.getUecList()) ;
  joinStatDescList.insertIntoUecList (rightColStatsList.getUecList()) ;

  //--------------------------------------------------------------------
  // Note, when gathering rowcounts, avoid problems caused by the
  // possibility of subsequent division by zero.
  //--------------------------------------------------------------------
  CostScalar leftRowCount     = leftEstProps->getResultCardinality().minCsOne() ;
  CostScalar rightRowCount    = rightEstProps->getResultCardinality().minCsOne() ;
  CostScalar outerRefRowCount = inputEstLogProp->getResultCardinality().minCsOne() ;

  CostScalar newRowCount = 1;
  Int32 outerMatchCount = 0;
  CollIndex i = 0;
  CollIndex j = 0;

  //--------------------------------------------------------------------
  // Stage 1: If there are outer references involved (I.e., this join is
  // under the 2nd child of a TSJ), determine the effect of the fact
  // that a join only produces rows when, for a given probe, it produces
  // rows from both its left and right child.
  //
  // First cross-product the matching outer references returned by both
  // children.
  //
  // Note that getColStatsToModify's copy of the ColStats causes two
  // pointers to point to the same histogram.
  // i.e., copyAndScaleHistogram does the real copy.
  // ( copyAndScaleHistogram also applies the current reduction factor
  //   to all histogram intervals )
  //--------------------------------------------------------------------
  // Note:
  //   If this join is under the second child of a Left/Anti TSJ
  //   the outer colRef VEG may be a different VEGRef in that region
  //   that it is in this region.
  //   E.g    T1 LEFT JOIN (T2 JOIN T3 ON (T1.x = T2.x AND T2.x = T3.x)
  //
  //   There will be a VEG for {VegRef(T1.x), T2.x, T3.x}
  //   The outerColRef will be VegRef(T1.x)
  //--------------------------------------------------------------------
  for (i=0; i < outerColStatsList.entries(); i++)
    {
      NABoolean foundMatch = FALSE;
      CollIndex leftCol, rightCol;

      for (j=0; j < leftColStatsList.entries() ; j++)
        if (leftColStatsList[j]->getVEGColumn().getItemExpr()->
            containsTheGivenValue(outerColStatsList[i]->getVEGColumn()) )
          {
            leftCol = j;
            foundMatch = TRUE;
            break ;
          }

      if (foundMatch)
	{
	  foundMatch = FALSE;

	  for (j=0; j < rightColStatsList.entries() ; j++)
            if (rightColStatsList[j]->getVEGColumn().getItemExpr()->
                containsTheGivenValue(outerColStatsList[i]->getVEGColumn()) )
	      {
                rightCol = j;
                foundMatch = TRUE;
                break ;
	      }
	}

      if (foundMatch)
	{
	  // Normal Case: Found current def'n of outer ref for both children
	  // Cross-Product those two def'ns and insert both into the under-
	  // construction ColStatDescList for this Join.
	  // *** Future: if we had multi-column outer references it would
	  //             be nice to bring them along......
          joinStatDescList.insertDeepCopyAt (outerMatchCount,
                                             leftColStatsList[leftCol],
                                             rightRowCount); // scale
          outerMatchCount++;

          joinStatDescList.insertDeepCopyAt (outerMatchCount,
                                             rightColStatsList[rightCol],
                                             leftRowCount); // scale
          outerMatchCount++;
	}
    } // for i

  //--------------------------------------------------------------------
  // Stage 2: If there are outer reference involved, and we were able to
  // match the returned outer references from the left and right child,
  // Inner-equi-join pairs of entries (i and i+1) in the ColStatDescList
  // that represents matching outer references.
  //--------------------------------------------------------------------

  //++MV
  // remember the initial cardinality for later use
  CostScalar initialCardinality = newRowCount;
  //--MV

  if ( outerMatchCount > 0 )
    newRowCount = joinStatDescList.mergeListPairwise();
  else
  {
    // before doing the cross product, make sure there is no overflow
    if (rightRowCount.getValue() < 1.000001)
         rightRowCount = CostScalar(1.0);

    if ( DBL_MAX /rightRowCount.getValue() > leftRowCount.getValue())
      newRowCount = rightRowCount * leftRowCount;

    else
      newRowCount = CostScalar(DBL_MAX)/CostScalar(100.0);
  }

  //--------------------------------------------------------------------
  // Stage 3: At this point, there are 3 cardinalities to deal with:
  //   newRowCount, if applicable, a normalized view of outer references;
  //   leftRowCount, rows from left child; and
  //   rightRowCount, rows from right child.
  // Now, set up the rest of the ColStatDesc entries for other columns
  // returned by children of the join, and make a first approximation
  // of the cardinality of the join, prior to applying local predicates.
  //
  // This is complicated by differences in treatment of non-semi- and
  // semi- joins.  It is also tricky to estimate the output cardinality
  // of the current join when it is itself under the right child of a
  // TSJ.
  // When under the right child of a TSJ, both this join's children
  // describe themselves as producing the result of all probes done.
  // To determine the rowcount involved in the crossproduct done prior
  // to applying local join predicates, involves application of the
  // implicit equi-join between returned outer references (done above),
  // and in the case of non-semi-joins also involves down-scaling the
  // newRowCount produced above by the outer reference's cardinality.
  //
  // If we had outer references,
  //   If this is NOT a semijoin, then
  //     all histograms are made to have newRowCount/outerRefCard rows.
  //   Else
  //     all histograms on outer leg have newRowCount/outerRefCard rows
  //     all histograms on inner leg keep their current cardinality
  // Else
  //   If this is NOT a semijoin, then
  //     all histograms need to have leftRowCount*RightRowCount rows
  //   Else
  //     leave histograms with their current cardinality.
  //
  // reminder: Subsequent logic will use the fact that the Left child's
  //           column statistics appear first.  It is significant for
  //           semi- and outer- joins.
  //--------------------------------------------------------------------
  CostScalar leftScale = 1;
  CostScalar rightScale = 1;
  CostScalar outerRefScale = ((CostScalar) 1) / outerRefRowCount;

  // Multiply the rowCount after cross-product with the outerRefScale to
  // to ensure that the input cardinality is not counted twice, once each from
  // the left and the right child.
  // This would be true for both cases when outerMatchCount > 0 (predicates like
  // T1.a =T2.a = T3.a, where T1.a is passed down to both T2 and T3.
  // Hence is counted twice.), and when outerMatchCount = 0 (predicate T1.a = T2.a + T3.a
  // where T1.a is not passed down to T2 and T3, but T2 and T3 are evaluated for each probe
  // coming down from T1. Once again the input Cardinality is counted twice in this case)
  // While reducing the cardinality, make sure it does not go below 1

  newRowCount = MIN_ONE_CS(outerRefScale * newRowCount);

  // The right scale is equal to the left row count divided by
  // the outer row count. This is because the outer row count
  // has already been included in both the children.
  rightScale  = isASemiJoin ? (CostScalar) 1 : leftRowCount * outerRefScale;
  leftScale   = isASemiJoin ? (CostScalar) 1 : rightRowCount * outerRefScale;

  // update newRowCount if it is a semi-join
  if (isASemiJoin)
    newRowCount = leftRowCount;

  CollIndex joinEntries = joinStatDescList.entries();

  if ( outerRefScale != 1 )
  {
    for (i = 0; i < joinEntries; i++ )
    {
      joinStatDescList[i]->applySel (outerRefScale);
    }
  }

  for (i = 0; i < leftColStatsList.entries(); i++)
    {
      NABoolean alreadyAdded = FALSE;
      for (j=0 ; j < outerColStatsList.entries() ; j++)
        if (leftColStatsList[i]->getVEGColumn().getItemExpr()->
            containsTheGivenValue(outerColStatsList[j]->getVEGColumn()) )
	  {
	    alreadyAdded = TRUE;
            break ;
	  }

      if (NOT alreadyAdded)
        {
          joinStatDescList.insertDeepCopy(leftColStatsList[i],
                                          leftScale, // scale
                                          FALSE);    // clear shapeChanged
        }
    }

  // remember the number of columns, and current stats of the 'outer' table
  CollIndex outerRefCount = joinStatDescList.entries();
  ColStatDescList leftCopy;
  leftCopy.makeDeepCopy( leftColStatsList );

  for (i = 0; i < rightColStatsList.entries(); i++)
    {
      NABoolean alreadyAdded = FALSE;
      for (j=0 ; j < outerColStatsList.entries() ; j++)
         if (rightColStatsList[i]->getVEGColumn().getItemExpr()->
             containsTheGivenValue(outerColStatsList[j]->getVEGColumn()) )
          {
            alreadyAdded = TRUE;
            break ;
          }

      if (NOT alreadyAdded)
        joinStatDescList.insertDeepCopy(rightColStatsList[i],
                                        rightScale, // scale
                                        FALSE);     // clear shapeChanged
    }

  // Finally, after all the setup, do the real work in the join.
  newRowCount = this->commonSynthEst (myEstProps,
                                      intermedEstProps,
                                      outerRefCount,
                                      leftCopy,
                                      rightColStatsList,
                                      joinStatDescList,
                                      newRowCount) ;

  // If the input to this join is for semijoinTSJ then force a semijoin
  //  merge between the result of this join and the input to this join.

  if (inputEstLogProp->getInputForSemiTSJ() != EstLogProp::NOT_SEMI_TSJ)
    {
      ColStatDescList newStatsList;
      EstLogPropSharedPtr newIntermedEstProps(new (HISTHEAP) EstLogProp());
      // To maintain uniformity, use GLOBAL_EMPTY_INPUT_LOGPROP
      EstLogPropSharedPtr newLeftEstProp  = child(0).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP));
      EstLogPropSharedPtr newRightEstProp = child(1).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP));

      leftRowCount  = MIN_ONE_CS(newLeftEstProp->getResultCardinality());
      rightRowCount = MIN_ONE_CS(newRightEstProp->getResultCardinality());
      rightScale = isASemiJoin ? 1 : leftRowCount;
      leftScale  = isASemiJoin ? 1 : rightRowCount;

      const ColStatDescList &leftColStatsList  = newLeftEstProp->getColStats();
      const ColStatDescList &rightColStatsList = newRightEstProp->getColStats();

      j = 0;
      newStatsList.makeDeepCopy( leftColStatsList,
                                 leftScale,  // scale RowCount,
                                 FALSE );    // clear shapeChanged

      newStatsList.appendDeepCopy( rightColStatsList,
                                   rightColStatsList.entries(),  // all
                                   rightScale, // scale Rowcount
                                   FALSE );    // clear shapeChanged

      outerRefCount = 0 ; // at this point, there *are* no outer refs; we add them below
      ColStatDescList leftCopy;
      leftCopy.makeDeepCopy(leftColStatsList);

      // Finally, after all the settup, do the real work in the join.
      newRowCount = this->commonSynthEst (myEstProps,
                                          newIntermedEstProps,
                                          outerRefCount,
                                          leftCopy,
                                          rightColStatsList,
                                          newStatsList,
                                          newRowCount) ;

      // now do a semi join with the input

      newStatsList.prependDeepCopy( outerColStatsList, // source
                                    outerColStatsList.entries(), // all
                                    1,                 // no scale
                                    FALSE );           // clear shapeChanged

      ValueIdSet outerReferences;
      outerRefCount = outerColStatsList.entries();

      MergeType mergeType ;
      if ( inputEstLogProp->getInputForSemiTSJ() == EstLogProp::SEMI_TSJ )
        {
          mergeType = SEMI_JOIN_MERGE ;
        }
      else
        {
          // We got here after checking that the merge type is not NOT_SEMI_TSJ, implying that it is
          // one of the TSJs. If it is not a SEMI_TSJ, then we assume it to be ANTI_SEMI_TSJ
          CCMPASSERT (inputEstLogProp->getInputForSemiTSJ() == EstLogProp::ANTI_SEMI_TSJ) ;
          mergeType = ANTI_SEMI_JOIN_MERGE ;
        }

      newRowCount =
        newStatsList.estimateCardinality (newRowCount,                /*in*/
                                          getSelectionPred(),         /*in*/
                                          outerReferences,            /*in*/
                                          this,
                                          NULL,	       /*for selectivityHint, which can only be given for Scan*/
                                          NULL,	       /* for Cardinality hint, which can only be given for Scan */
                                          outerRefCount,              /*in*/
                                          myEstProps->unresolvedPreds(), /*in/out*/
                                          mergeType,                 /*in*/
                                          REL_JOIN);

      joinStatDescList.synchronizeStats (newRowCount, joinStatDescList.entries()) ;

      // deallocate the entries used by the 'if-local' leftCopy.
      for ( i = 0; i < leftCopy.entries(); i++ )
        leftCopy.removeDeepCopyAt(0);
    }

  // deallocate the entries used by the leftCopy
  for ( i = 0 ; i < leftCopy.entries(); i++ )
    leftCopy.removeDeepCopyAt(0) ;

  // synchronizeStat only if needed
  NABoolean synchronizeStatNeeded = FALSE;

    CostScalar oldCount = newRowCount;

  // ++MV
  if (getInliningInfo().isForceCardinality())
  {
    // Override the estimated cardinality using the inlining information.
    newRowCount = getInliningInfo().getNewCardinality(initialCardinality);
  }
  // --MV

  // since the histograms have been merged together. Set the scale factor of remaining 
  // histograms to one.
  joinStatDescList.setScaleFactor(csOne);

  newRowCount = doCardSanityChecks(copyInputEstProp,
                                   joinStatDescList,
                                   newRowCount);
  CostScalar maxCardEst = estimateMaxCardinality
    (copyInputEstProp, joinStatDescList);
								   
  if (oldCount != newRowCount)
    joinStatDescList.synchronizeStats (newRowCount, joinStatDescList.entries()) ;

  myEstProps->setResultCardinality(newRowCount);
  myEstProps->setMaxCardEst(MAXOF(maxCardEst, newRowCount));

  //---------------------------------------------------------------------------
  // Now, determine which of these histograms are useful based on
  // the characteristic outputs and the selection predicates for the group.
  // --------------------------------------------------------------------------

  myEstProps->pickOutputs( joinStatDescList, inputEstLogProp, getGroupAttr()->getCharacteristicOutputs(), getSelectionPred());

  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstProps,
      intermedEstProps);

} // Join::synthEstLogProp()

// ------------------------------------------------------------------------
// Join::doCardSanityChecks
// Is a helper method, which does sanity checks for Join cardinality
// at the end. The following checks are used to slightly
// improve the results. These are in no way perfect, and we intend
// to come back and have a look at the whole estimateCardinality
// some day.
// (1) For joins with fake histograms, rowcount should be atleast equal
// the max of left and the right child
// (2) For join on single column, the join cardinality should be
// a minimum of maximum frequency of the two children. Example
// T1 has 10,000 rows and 1 UEC and T2 has 30,000 rows equally
// distributed amongst 2 values (UECs). Max frequency of T1 for a
// value would be 10,000 / 1 and for T2 it would be 30,000/2 = 15,000
// Join cardinality should be a minimum of 15,000
// Ofcourse since we do not assume the rows to be equally distributed
// hence frequency is calculated for each interval of the histogram
// (3) Fourth check is an optimistic sanity check also used to
// uplift the join cardinality for highly skewed data, such that values
// in one column span over say 200,000 days, while the values on other
// column span, say over 100 days. The problem happens when the joining
// column is indirectly reduced because of a local predicate on some other
// column. The fix is controlled by CQD
// HIST_OPTIMISTIC_CARD_OPTIMIZATION, and is OFF by default
// (4) For join on more than one columns, and in the absence of
// multi-column statistics, newRowcount should not be less than some
// percentage of smaller table
// (5) New row count should never be greater than the inputCardinality
// multiplied by the cardinalitiy fo the leftchild for empty input
// log prop multipled by the cardinality of the right child for empty
// inputlogprop. This will ensure that the input cardinality is not
// considered twice, thereby increasing the row counts.
//------------------------------------------------------------------

CostScalar
Join::doCardSanityChecks(const EstLogPropSharedPtr& inLP,
			 ColStatDescList & joinStatDescList,
                         CostScalar newRowCount)
{
  if(joinStatDescList.selectivityHintApplied())
    return newRowCount;

  CostScalar adjustedRowcount = newRowCount;

  CostScalar inCard = inLP->getResultCardinality();
  EstLogPropSharedPtr leftEstLogProp = child(0).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP));
  EstLogPropSharedPtr rightEstLogProp = child(1).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP));
  CostScalar leftCard = leftEstLogProp->getResultCardinality();
  CostScalar rightCard = rightEstLogProp->getResultCardinality();

  if (isLeftJoin())
  {
    ColStatDescList &rightColStatsList = rightEstLogProp->colStats();
    if(!rightColStatsList.isEmpty())
    {
      CostScalar nullRows = rightColStatsList[0]->getColStats()->getNullCount();
      rightCard -= nullRows;
      rightCard.minCsOne();
    }
  }
  CostScalar maxCard = leftCard * rightCard * inCard;
  CostScalar minCard = csOne;

  ValueIdSet joinPredicates = getSelectionPred();
  joinPredicates.addSet(getJoinPred());

  ValueIdSet joinedCols = joinStatDescList.getJoinedCols();

  NABoolean joinOnSingleCol = FALSE;

  if ( (joinPredicates.entries() == 1) &&
       (joinedCols.entries() > 0 ) ) 
    joinOnSingleCol = TRUE;

  NABoolean skipMinCardTest = FALSE;
  NABoolean SemiOrAntiSemiJoin = ((isSemiJoin() || isAntiSemiJoin()) ? TRUE : FALSE);

  OperatorTypeEnum oper = getOperatorType();

  if (CmpCommon::getDefault(COMP_BOOL_44) == DF_OFF)
  {
    for ( CollIndex i = 0; i < joinStatDescList.entries(); i++ )
    {
      ColStatDescSharedPtr thisColStatDesc = joinStatDescList[i];
      ValueIdSet mergeCols;
      thisColStatDesc->getMergeState().findAllReferencedBaseCols(mergeCols);
        
      // The second argument of FALSE for the method membersCoveredInSet
      // indicates that we do NOT need to look below InstantiateNulls
      // to check for coverage.
      if (( mergeCols.membersCoveredInSet(joinedCols,FALSE) > 0) &&
	  (thisColStatDesc->getColStats()->isOrigFakeHist() ) )
      {
        if (SemiOrAntiSemiJoin)
          minCard = leftCard;
        else
          minCard = MAXOF(leftCard, rightCard);

	skipMinCardTest = TRUE;
	break;
      }
    }
  }

  if ( joinOnSingleCol &&
       (skipMinCardTest == FALSE) )
  {
    // single column join, set the min cardinality

    // 1. Based on the maximum frequency
    if (CmpCommon::getDefault(COMP_BOOL_46) == DF_OFF)
    {
      for (ValueId col = joinedCols.init();
                        joinedCols.next(col);
                        joinedCols.advance(col) )
      {
        CostScalar currMax = csOne;

        CostScalar leftFreq =  leftEstLogProp->colStats().getMaxFreq(col);

        CostScalar rightFreq = csOne;
        
        if (!SemiOrAntiSemiJoin)
          // for semi_join, the cardinality is only dependent on the outer
          // frequency, hence inner frequency should not be applied to 
          // semi-joins        
          rightFreq = rightEstLogProp->colStats().getMaxFreq(col);

        if (leftFreq > rightFreq)
          currMax = leftFreq;
        else 
          currMax = rightFreq;

        if (minCard < currMax)
        {
          minCard = currMax;
          skipMinCardTest = TRUE;
        }
      }
    } // comp_bool_46 == off

    // The code is controlled by CQD HIST_OPTIMISTIC_CARD_OPTIMIZATION
    // with following values
    // 0 - disable code. .
    // 1 - uplift the join cardinality only if the join cardinality is
    // lower than the minimum cardinality
    // 2 - overwrite the join cardinality irrespective of its original
    // value. This
    // could even lower the join cardinality

    UInt32 upliftCardCond = CURRSTMT_OPTDEFAULTS->histOptimisticCardOpt();
    if (( (upliftCardCond == 1)  ||
          (upliftCardCond == 2) ) &&
           joinOnSingleCol &&
           !SemiOrAntiSemiJoin)
    {
      CostScalar currMin = csOne;

      // first get the joining column with minimum UEC
      CostScalar minOriginalUec = joinedCols.getMinOrigUecOfJoiningCols();

      // next get the UEC of the left and the right joining columns
      CostScalar leftUec = leftEstLogProp->getColStats().
			      getUecOfJoiningCols(joinedCols);
      CostScalar rightUec = rightEstLogProp->getColStats().
			      getUecOfJoiningCols(joinedCols);

      // max of left and right UEC to compute join cardinality
      // uses single interval concept and the containment assumption
      CostScalar baseUec = MAXOF(rightUec, leftUec);
      // Using maximum of the left and right UEC and minimum original
      // UEC makes the estimate more conservative
      baseUec = MAXOF(baseUec, minOriginalUec);

      currMin = (rightCard * leftCard)/baseUec;
      MIN_ONE_CS(currMin.round());

      if ( ( (upliftCardCond == 1) && (minCard < currMin))
	   ||
	   (upliftCardCond > 1) )
      {
	    minCard = currMin;
	    skipMinCardTest = TRUE;
      }
    } // histOptimisticCardOpt > 0
  } // single column join

  if ( (skipMinCardTest == FALSE) &&
	   (joinStatDescList.isCapForLowBound()) )
  {
	// multi-column join
	const JBBSubset * localJBBView = NULL;

	if ((getGroupAnalysis()) &&
	  (localJBBView = getGroupAnalysis()->getLocalJBBView()))
	  minCard = localJBBView->getMinimumJBBCRowCount();
	else
	{
	  // we estimate cardinality for GroupAttr using empty_input_logprop
	  // but at this point we might have some input est logprop. So take
	  // inputEstLogProp into account while determining low bound
	  CostScalar minEstRowCount = getGroupAttr()->getMinChildEstRowCount();

	  if (minEstRowCount <= csZero)
	    minEstRowCount = computeMinEstRCForGroup();
	  // we estimate cardinality for GroupAttr using empty_input_logprop
	  // but at this point we might have some input est logprop. So take
	  // inputEstLogProp into account while determining low bound

	  minCard = inCard * minEstRowCount;
	}

	minCard *= (CURRSTMT_OPTDEFAULTS->joinCardLowBound());
	skipMinCardTest = TRUE;
  }

  if (adjustedRowcount < minCard)
  {
	adjustedRowcount = minCard.round();
  }

  if (adjustedRowcount > maxCard)
  {
    // Synchronize all histograms of the joinStatDescList based on this
    // new row count
    adjustedRowcount = maxCard.round();
  }

  adjustedRowcount.minCsOne();
  return adjustedRowcount;
}


NABoolean Join::matchRIConstraint(GroupAttributes& leftGA, 
				  GroupAttributes& rightGA,
				  NormWA * normWAPtr)
{
  // there can only be at most one RefOpt CompRefOpt match at a given join
  // once we find that match we terminate all loops.
  NABoolean riMatched = FALSE;
  ValueIdSet matchingPreds;
  if ((leftGA.hasRefOptConstraint() || leftGA.hasCompRefOptConstraint() ) &&
     (rightGA.hasRefOptConstraint() || rightGA.hasCompRefOptConstraint()))
  {
    const ValueIdSet& leftConstraints = leftGA.getConstraints();
    const ValueIdSet& rightConstraints = rightGA.getConstraints();
    for (ValueId leftId = leftConstraints.init();
	      leftConstraints.next(leftId) && NOT riMatched;
	      leftConstraints.advance(leftId) )
    {
      if (leftId.getItemExpr()->getOperatorType() == ITM_REF_OPT_CONSTRAINT)
      {
	if (rightGA.hasCompRefOptConstraint())
	{
	  for (ValueId ukId = rightConstraints.init();
		rightConstraints.next(ukId) && NOT riMatched;
		rightConstraints.advance(ukId) )
	  {
	    if (ukId.getItemExpr()->getOperatorType() == ITM_COMP_REF_OPT_CONSTRAINT)
	    {
	      riMatched = hasMatchingRIConstraint(leftId, ukId, normWAPtr);
	    }
	  } // end of loop over compRefOptConstraints
	} // if rightGA has CompRefOptConstraints
      } // if leftId is a RefOptConstraint
      else if (leftId.getItemExpr()->getOperatorType() == ITM_COMP_REF_OPT_CONSTRAINT)
      {
	if (rightGA.hasRefOptConstraint())
	{
	  for (ValueId fkId = rightConstraints.init();
		rightConstraints.next(fkId) && NOT riMatched;
		rightConstraints.advance(fkId) )
	  {
	    if (fkId.getItemExpr()->getOperatorType() == ITM_REF_OPT_CONSTRAINT)
	    {
	      riMatched = hasMatchingRIConstraint(fkId, leftId, normWAPtr);
	    }
	  } // end of loop over RefOptConstraints
	} // if rightGA has RefOptConstraints
      } // if leftId is a CompRefOptConstraint
    } // end of loop over leftGAConstraints
  } // leftGA has RefOpt or CompRefOptConstraints and rightGA has RefOpt or CompRefOpt
  return riMatched;
}

NABoolean Join::hasMatchingRIConstraint(ValueId fkConstraintId, 
					ValueId ukConstraintId,
					NormWA * normWAPtr)
{
    RefOptConstraint * riConstraint = 
      (RefOptConstraint *) fkConstraintId.getItemExpr();
    ComplementaryRefOptConstraint * compRIConstraint = 
      (ComplementaryRefOptConstraint *) ukConstraintId.getItemExpr();

    ValueIdSet matchingPreds;
    NABoolean riMatched = FALSE;
    

    if (riConstraint->uniqueConstraintName() == 
	  compRIConstraint->constraintName())
    {
      if (hasRIMatchingPredicates(riConstraint->foreignKeyCols(), 
				  compRIConstraint->uniqueKeyCols(),
				  compRIConstraint->getTableDesc(),
				  matchingPreds))
      {
	ValueIdSet uniqKeyNonPredOutputs, globalNonEssOutputs;
	ValueIdSet myNonPredOutputs ;

	riMatched = TRUE;
	riConstraint->setIsMatched(TRUE);

	RelExpr * referencedTable = 
	  (RelExpr *) compRIConstraint->getReferencedTable();
	uniqKeyNonPredOutputs = referencedTable->getGroupAttr()->
	  getCharacteristicOutputs();
	
	ValueIdSet ucCols = compRIConstraint->uniqueKeyCols();
	uniqKeyNonPredOutputs -= ucCols;

	myNonPredOutputs = getGroupAttr()->getCharacteristicOutputs();
	myNonPredOutputs -= ucCols;

	if (NOT (myNonPredOutputs.referencesOneValueFromTheSet(uniqKeyNonPredOutputs)))
	{
	  referencedTable->setMarkedForElimination(TRUE);
          compRIConstraint->setIsMatchedForElimination(TRUE);
	  normWAPtr->setContainsJoinsToBeEliminated(TRUE);
	  setPredicatesToBeRemoved(matchingPreds);
	}
	else 
	{
          if (normWAPtr->getExtraHubVertex() &&
              CmpCommon::getDefault(MVQR_USE_EXTRA_HUB_TABLES)        == DF_ON &&
              CmpCommon::getDefault(MVQR_USE_RI_FOR_EXTRA_HUB_TABLES) == DF_ON )
          {
	    normWAPtr->getExtraHubVertex()->getGroupAttr()->
	      getNonEssentialCharacteristicOutputs(globalNonEssOutputs);

	    if (uniqKeyNonPredOutputs.referencesAllValuesFromMe
                                                       (globalNonEssOutputs))
	    {
	      referencedTable->setIsExtraHub(TRUE);
	      setIsExtraHub(TRUE);
              normWAPtr->setCheckForExtraHubTables(TRUE);

              addExtraHubNonEssentialOutputs(
                                  referencedTable->getGroupAttr()->
                                     getEssentialCharacteristicOutputs());
            }
	  }
	} // eliminate or extra-hub or do nothing
      } // hasMatchingRIPredicates
    }  // refOpt.name == compRefOpt.name
    return riMatched;
}

CostScalar
Join::estimateMaxCardinality(const EstLogPropSharedPtr& inLP,
                             ColStatDescList & joinStatDescList)
{
  EstLogPropSharedPtr leftEstLogProp = child(0).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP));
  EstLogPropSharedPtr rightEstLogProp = child(1).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP));

  CostScalar leftMax = leftEstLogProp->getMaxCardEst();
  CostScalar rightMax = rightEstLogProp->getMaxCardEst();
  CostScalar inCard = inLP->getResultCardinality();
  // maxCardEst0 is max card estimate based on semantics only
  CostScalar maxCardEst0 = leftMax * rightMax * inCard;

  ValueIdSet joinedCols, joinPredicates = getSelectionPred();
  joinPredicates.addSet(getJoinPred());

  // max cardinality estimate for non-equi join is
  //   maxCardEst = maxCardEst(left) * maxCardEst(right)
  if (joinPredicates.hasNoEquiPredicates(joinedCols))
    {
       return maxCardEst0;
    }

  // this is an equi-join (it has at least one equality join predicate)

  // At this point the logical properties of a join have been synthesized, 
  // so we can check if each row from the first or second child is 
  // guaranteed to have a single match from the other child.
  if (rowsFromLeftHaveUniqueMatch())
    { 
      maxCardEst0 = MINOF(leftMax * inCard, maxCardEst0);
    }
  if (rowsFromRightHaveUniqueMatch())
    {
      maxCardEst0 = MINOF(rightMax * inCard, maxCardEst0);
    }

  // max cardinality for an equi-join
  // T1 |X| T2 where T1.x1 = T2.y1 and T1.x2 = T2.y2 and ... T1.xn = T2.yn 
  //   is MIN(maxCard(T1) * maxFreq(T2.ymin), maxCard(T2) * maxFreq(T1.xmin))
  // T1 |X| T2 where T1.x1+1 = T2.y1 and ... T1.xn-9 = T2.yn 
  //   is maxCard(T1) * maxFreq(T2.ymin)
  // T1 |X| T2 where T1.x1 = T2.y1+6 and ... T1.xn = T2.yn*3
  //   is maxCard(T2) * maxFreq(T1.xmin)
  // we do this by the way joinedCols is computed by hasNoEquiPredicates above
  // and by the code below

  CostScalar leftFreqMin = leftMax;
  CostScalar rightFreqMin = rightMax;
  CostScalar leftCard = leftEstLogProp->getResultCardinality();
  CostScalar rightCard = rightEstLogProp->getResultCardinality();
  CostScalar leftAvgFreq = leftMax;
  CostScalar rightAvgFreq = rightMax;
  for (ValueId col = joinedCols.init();
       joinedCols.next(col);
       joinedCols.advance(col) )
    {
      CostScalar leftFreq =  leftEstLogProp->colStats().getMaxFreq(col);
      if (leftFreq >= 0)
        {
          // leftFreq is based on expected cardinality histograms.
          // to approximate maxFreq based on max cardinality histograms,
          // scale up leftFreq by maxCard/estCard and make sure it's >= 1
          leftFreq *= (leftMax / leftCard);
          leftFreqMin = MINOF(leftFreqMin, leftFreq.minCsOne());
        }
      CostScalar rightFreq = rightEstLogProp->colStats().getMaxFreq(col);
      if (rightFreq >= 0)
        {
          // rightFreq is based on expected cardinality histograms.
          // to approximate maxFreq based on max cardinality histograms,
          // scale up rightFreq by maxCard/estCard and make sure it's >= 1
          rightFreq *= (rightMax / rightCard);
          rightFreqMin = MINOF(rightFreqMin, rightFreq.minCsOne());
        }
      // avgFreq(T.z) = maxCard(T) / totalUec(T.z)
      leftAvgFreq = 
        MINOF(leftAvgFreq, 
              CostScalar(leftMax/leftEstLogProp->colStats().getUEC(col))
              .minCsOne());
      rightAvgFreq = 
        MINOF(rightAvgFreq, 
              CostScalar(rightMax/rightEstLogProp->colStats().getUEC(col))
              .minCsOne());
      // avgFreq(T.z) may sometime exceed maxFreq(T.z)
    }

  // maxCardEst1 is maxCard based on maxFreq
  CostScalar maxCardEst1 = 
    inCard * MINOF(leftMax * rightFreqMin, rightMax * leftFreqMin);

  // combining the 2 estimates
  maxCardEst1 = MINOF(maxCardEst1, maxCardEst0);

  // maxCard based on maxFreq is often too high. 
  // Sometimes we can do better using Zipf's law.
  // Many types of data (including join results) can be approximated with a
  // Zipfian distribution. The most popular join value occurs about twice as
  // often as the 2nd most popular one, which occurs twice as often as the 
  // 4th most popular one, etc. 
  // Formula for computing an equi-join's maxCard based on Zipf's law 
  // is:
  //   1.6 * maxFreq(T2.ymin) * maxFreq(T1.xmin)
  //   + MIN( maxCard(T1)*avgFreq(T2.ymin), maxCard(T2)*avgFreq(T1.xmin)
  // where
  //   avgFreq(T.z) = maxCard(T) / totalUec(T.z)
  CostScalar maxCardEstz = CostScalar(1.6) * rightFreqMin * leftFreqMin +
    MINOF(leftMax * rightAvgFreq, rightMax * leftAvgFreq);

  Int32 cqd = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_47);
  CostScalar maxCardEst;
  switch (cqd) {
  case 1:
    maxCardEst = maxCardEst1;
    break;
  case 2:
    maxCardEst = MINOF(maxCardEstz, maxCardEst0);
    break;
  default:
    maxCardEst = MINOF(maxCardEst1, maxCardEstz);
    break;
  }
  return maxCardEst;
}

//---------------------------------------------------------------------------
// Join::synthConstraints
//---------------------------------------------------------------------------
void
Join::synthConstraints(NormWA * normWAPtr)
{

  // QSTUFF
  NAString fmtdList1(CmpCommon::statementHeap());
  LIST(TableNameMap*) xtnmList1(CmpCommon::statementHeap());
  NAString fmtdList2(CmpCommon::statementHeap());
  LIST(TableNameMap*) xtnmList2(CmpCommon::statementHeap());
  // QSTUFF

  GroupAttributes &myGA = *getGroupAttr();
  GroupAttributes &leftGA = *child(0).getGroupAttr();
  GroupAttributes &rightGA = *child(1).getGroupAttr();

  const ValueIdSet &leftConstraints = leftGA.getConstraints();
  const ValueIdSet &rightConstraints = rightGA.getConstraints();

  // ---------------------------------------------------------------------
  // Find out if each row from the left/right can have a
  // single matching row from the right/left
  // ---------------------------------------------------------------------
  if (NOT getEquiJoinExprFromChild1().isEmpty())
    leftHasUniqueMatches_ = rightGA.isUnique(getEquiJoinExprFromChild1());
  else
    leftHasUniqueMatches_ = FALSE;

  if (NOT getEquiJoinExprFromChild0().isEmpty())
    rightHasUniqueMatches_ = leftGA.isUnique(getEquiJoinExprFromChild0());
  else
    rightHasUniqueMatches_ = FALSE;

  if (isSemiJoin() OR isAntiSemiJoin())
    leftHasUniqueMatches_ = TRUE;

  // QSTUFF
  // this is a cursor update, delete or insert and therefore
  // the constraint is guaranteed to be true
  if (isCursorUpdate())
    leftHasUniqueMatches_ = TRUE;
  // QSTUFF

  Cardinality minLeft, minRight, maxLeft, maxRight;
  Cardinality minRows, maxRows;

  // ---------------------------------------------------------------------
  // synthesize cardinality constraints
  // ---------------------------------------------------------------------

  (void) leftGA.hasCardConstraint(minLeft,maxLeft);
  (void) rightGA.hasCardConstraint(minRight,maxRight);

  if (maxLeft == 1)
    rightHasUniqueMatches_ = TRUE;

  if (maxRight == 1)
    leftHasUniqueMatches_ = TRUE;

  // If this GroupAttr() has already been synthesised return now
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  // if both the tables produced by the children have a maximum cardinality,
  // the result of the join can't be larger than the product of their
  // cardinalities
  minRows = (Cardinality)0;

  if (leftHasUniqueMatches_)
    if (rightHasUniqueMatches_)
      maxRows = maxLeft < maxRight ? maxLeft : maxRight;
    else
      maxRows =  maxLeft;
  else if (rightHasUniqueMatches_)
    maxRows = maxRight;
  else if (maxLeft < INFINITE_CARDINALITY AND
           maxRight < INFINITE_CARDINALITY)
    maxRows = maxLeft * maxRight;
  else
    maxRows = (Cardinality)INFINITE_CARDINALITY;

  // synthesize minimum cardinality constraints, if the join has no
  // selection predicate and (it has no join predicate or it is an
  // outer join) and the left child table has a minimum cardinality
  // constraint and (the right child table has a minimum cardinality
  // constraint or this is an outer join)

  // $$$$ TBC

  // add the cardinality constraint, if there is any
  if (minRows > 0 OR maxRows < INFINITE_CARDINALITY)
    {
      myGA.addConstraint(new(CmpCommon::statementHeap())
			 CardConstraint(minRows,maxRows));
    }

  // ---------------------------------------------------------------------
  // synthesize functional dependencies
  // ---------------------------------------------------------------------

  if (CmpCommon::getDefault(OPTIMIZER_SYNTH_FUNC_DEPENDENCIES) != DF_OFF)
    {
      // Generate functional dependencies. Functional dependencies say
      // that some columns depend on a set of other columns, where
      // that set may be different or smaller than the set of unique
      // columns.  With "depend on" we mean that if column c depends
      // on columns (a,b) then we will never have two result rows of
      // our subtree with the same (a,b) values but different c
      // values.

      // Example: Take a join of NATION and SUPPLIER in TPC-D on the
      // NATIONKEY column. The result has a uniqueness constraint on
      // S_SUPPKEY. In addition we can record that N_REGIONKEY is
      // functionally dependent on N_NATIONKEY. The join result will
      // never have two rows with the same N_NATIONKEY (e.g. for
      // China) and two different N_REGIONKEY values (e.g. for Asia
      // and America), it will always be the same N_REGIONKEY (the one
      // for Asia in this case).

      // Generate functional dependencies only if the original
      // uniqueness constraints of a child are lost. This is because
      // all columns of a result are always functionally dependent on
      // any set of unique columns.
      FuncDependencyConstraint::synthFunctionalDependenciesFromChild(
	   myGA,
	   child(0),
	   NOT leftHasUniqueMatches_);
      FuncDependencyConstraint::synthFunctionalDependenciesFromChild(
	   myGA,
	   child(1),
	   NOT rightHasUniqueMatches_);
    }

  // ---------------------------------------------------------------------
  // synthesize uniqueness constraints
  // ---------------------------------------------------------------------

  if (rightHasUniqueMatches_)
    {
      // if the left table has just one row, propagate all "interesting"
      // uniqueness constraints from the right table to the top, since
      // the join will never duplicate a row from the right table

      for (ValueId rx = rightConstraints.init();
	   rightConstraints.next(rx);
	   rightConstraints.advance(rx))
	{
	  ItemExpr *rc = rx.getItemExpr();
	  OperatorTypeEnum roptype = rc->getOperatorType();

	  // is this an "interesting" uniqueness constraint
	  if (roptype == ITM_UNIQUE_OPT_CONSTRAINT)
	      // would have to use a coverage method because of VEGies
	      // AND
	      // myGA.getCharacteristicOutputs().contains(
	      //   ((UniqueOptConstraint *)rc)->uniqueCols()))
	    {
	      myGA.addConstraint(rx);
	    }
	}
    }

  if (leftHasUniqueMatches_)
    {
      // walk the constraints of the left child table
      for (ValueId lx = leftConstraints.init();
	   leftConstraints.next(lx);
	   leftConstraints.advance(lx))
	{
	  ItemExpr *lc = lx.getItemExpr();
	  OperatorTypeEnum loptype = lc->getOperatorType();

	  // is this a uniqueness constraint and are the unique columns in
	  // it "interesting" (are they part of the characteristic outputs)
	  if (loptype == ITM_UNIQUE_OPT_CONSTRAINT)
	      // would have to use a coverage method because of VEGies
	      // AND
	      // myGA.getCharacteristicOutputs().contains(
	      //   ((UniqueOptConstraint *)lc)->uniqueCols()))
	    {
	      // found an "interesting" unique constraint
	      // on the left child table
              myGA.addConstraint(lx);
	    } // interesting uniqueness constraint on left table
	} // for loop over
    }
  else if(rightHasUniqueMatches_ == FALSE)
    {
      // Neither side has a unique match from the other.
      // walk the constraints of the left child table
      for (ValueId lx = leftConstraints.init();
	   leftConstraints.next(lx);
	   leftConstraints.advance(lx))
	{
	  ItemExpr *lc = lx.getItemExpr();
	  OperatorTypeEnum loptype = lc->getOperatorType();

	  // is this a uniqueness constraint and are the unique columns in
	  // it "interesting" (are they part of the characteristic outputs)
	  if (loptype == ITM_UNIQUE_OPT_CONSTRAINT)
	      // would have to use a coverage method because of VEGies
	      // AND
	      // myGA.getCharacteristicOutputs().contains(
	      //   ((UniqueOptConstraint *)lc)->uniqueCols()))
	    {
	      // found an "interesting" unique constraint
	      // on the left child table
              // for each unique (candidate) key of the right table, form
              // a new uniqueness constraint for the join with the
              // combination of the unique columns

              for (ValueId rx = rightConstraints.init();
                   rightConstraints.next(rx);
                   rightConstraints.advance(rx))
                {
                  ItemExpr *rc = rx.getItemExpr();
                  OperatorTypeEnum roptype = rc->getOperatorType();

                  // is this an "interesting" uniqueness constraint
                  if (roptype == ITM_UNIQUE_OPT_CONSTRAINT)
	            // would have to use coverage because of VEGies
	            // AND
	            //   myGA.getCharacteristicOutputs().contains(
	            //        ((UniqueOptConstraint *)rc)->uniqueCols()))
	            {
	              // make a new uniqueness constraint with
	              // the unique columns from the left child table...
	              UniqueOptConstraint *newConstr =
	                new(CmpCommon::statementHeap())
	                  UniqueOptConstraint(((UniqueOptConstraint *)lc)->
			                   uniqueCols());

	              // ...and add the unique columns from the right table
	              newConstr->uniqueCols() +=
	                ((UniqueOptConstraint *)rc)->uniqueCols();

		      // now add the new uniqueness constraint
		      myGA.addConstraint(newConstr);
	            } // interesting right uniqueness constraint
		} // for loop over right child constraints
	    } // interesting uniqueness constraint on left table
	} // for loop over
    } // neither table had a unique match


    // QSTUFF VV
    // make sure we never multiply left tuples when doing joins on nested
    // update/delete result sets

    if (leftGA.isEmbeddedUpdateOrDelete() &&  !leftHasUniqueMatches_)
	{
		if( child(1)->getRETDesc() && getRETDesc() )  // soln:10_040806_8608, in the case of JoinLeftShiftRule the RETDesc_ may be NULL
		{

			child(1)->getRETDesc()->getTableList(xtnmList1, &fmtdList1);
			getRETDesc()->getTableList(xtnmList2, &fmtdList2);
			*CmpCommon::diags() << DgSqlCode(-4150)
				<< DgString0(fmtdList1)
				<< (leftGA.isEmbeddedUpdate() ?
						DgString1("update"):DgString1("delete"))
								 << DgString2(fmtdList2);
		}
    }

    if (rightGA.isEmbeddedUpdateOrDelete() && !rightHasUniqueMatches_)
    {
		if( child(0)->getRETDesc() && getRETDesc() ) // soln:10_040806_8608, in the case of JoinLeftShiftRule the RETDesc_ may be NULL
		{
			child(0)->getRETDesc()->getTableList(xtnmList1, &fmtdList1);
			getRETDesc()->getTableList(xtnmList2, &fmtdList2);
			*CmpCommon::diags() << DgSqlCode(-4150)
				<< DgString0(fmtdList1)
				<< (rightGA.isEmbeddedUpdate() ?
						 DgString1("update"):DgString1("delete"))
									 << DgString2(fmtdList2) ;
		}
    }

    // QSTUFF ^^
    processCompRefOptConstraints(normWAPtr);
    myGA.addSuitableRefOptConstraints(leftConstraints);
    myGA.addSuitableRefOptConstraints(rightConstraints);
    

} // Join::synthConstraints()

void
Join::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (NOT getGroupAttr()->existsLogExprForSynthesis())
    {

      RelExpr::synthLogProp(normWAPtr);

      // set the number of tables in the join (that form the join backbone).
      getGroupAttr()->setNumJoinedTables (
           child(0).getGroupAttr()->getNumJoinedTables() + 1);

      // Let's clear the logExprForSynthesis just set in
      // RelExpr::synthLogProp() so that synthConstraints() can
      // do its job.

      getGroupAttr()->setLogExprForSynthesis(NULL);
    }

  //-----------------------------------------------------
  // Find out our equijoin predicates and save them in
  // equiJoinPredicates_.
  // They remain also in selectionPred() or joinPred().
  //-----------------------------------------------------
  findEquiJoinPredicates();

  //-----------------------------------------------------
  // synthesize my cardinality and uniqueness constraints
  //-----------------------------------------------------
  synthConstraints(normWAPtr);


   // If we are synthesising constraints for the first time
   // for the group rmember the exprssion we used.
  if (NOT getGroupAttr()->existsLogExprForSynthesis())
    getGroupAttr()->setLogExprForSynthesis(this);

  // for left joins we can eliminate the whole join if 
  // (a) outputs of right child are unique in the equijoin cols and
  // (b) outputs from right child are not passed up from the join
  // this join elimination is not based on RI constraints
  if (isLeftJoin() && getSelectionPred().isEmpty() &&
      (CmpCommon::getDefault(ELIMINATE_REDUNDANT_JOINS) != DF_OFF) &&
      (NOT isFullOuterJoin()) && normWAPtr)
  {
    ValueIdSet equiJoinCols1  = getEquiJoinExprFromChild1();
    if ((NOT equiJoinCols1.isEmpty()) && 
	   child(1)->getGroupAttr()->isUnique(equiJoinCols1))
      {
	ValueIdSet emptySet, emptySet1, coveredExpr, coveredSubExpr;
	ValueIdSet myOutputs = getGroupAttr()->getCharacteristicOutputs();
	child(1)->getGroupAttr()->coverTest(
	  myOutputs,
	  emptySet,
	  coveredExpr,
	  emptySet1,
	  &coveredSubExpr);
	if(coveredExpr.isEmpty() && coveredSubExpr.isEmpty())
	{
          NABoolean usesInstNullInOutput = 
            myOutputs.referencesOneValueFromTheSet(nullInstantiatedOutput());
          if(!usesInstNullInOutput)
          {
            setMarkedForElimination(TRUE);
            normWAPtr->setContainsJoinsToBeEliminated(TRUE);
          }
	}
        else 
	{
          ValueIdSet globalNonEssOutputs;
          if (normWAPtr->getExtraHubVertex() &&
              CmpCommon::getDefault(MVQR_USE_EXTRA_HUB_TABLES) == DF_ON )
          {
	    normWAPtr->getExtraHubVertex()->getGroupAttr()->
	      getNonEssentialCharacteristicOutputs(globalNonEssOutputs);

            ValueIdSet nullOutputs(nullInstantiatedOutput());
            nullOutputs.intersectSet(myOutputs); // affects nullOutputs
            nullOutputs -= child(0)->getGroupAttr()->getCharacteristicOutputs();
            // removes join preds from nullOutputs, since those can be considered to
            // be provided by the child(0) and therefore should not affect whether
            // child(1) is considered an extra-hub table.

	    if ((!nullOutputs.isEmpty()) && nullOutputs.referencesAllValuesFromMe(globalNonEssOutputs))
	    {
              RelExpr * descendant = child(1)->castToRelExpr();
              while (descendant->getArity() == 1)
                descendant = descendant->child(0)->castToRelExpr();
              if (descendant->getArity() == 0)
              {
                descendant->setIsExtraHub(TRUE);
                setIsExtraHub(TRUE);
                /*normWAPtr->setCheckForExtraHubTables(TRUE);
                addExtraHubNonEssentialOutputs(
                                  referencedTable->getGroupAttr()->
                                  getEssentialCharacteristicOutputs()); */
              }
            }
	  }

        }
      }
  }


}// Join::synthLogProp()

// -----------------------------------------------------------------------
//  The following method is called as a first step during optimization
//  to reorder the join query by increasing estimated rowcount. The
//  purpose of reordering the join tree is to come up with a reasonable
//  cost limit during the first optimization pass.
//
// The reorderTree() is used also for pre-optimizer pass
// -----------------------------------------------------------------------
RelExpr *
Join::reorderTree(NABoolean & treeModified, NABoolean doReorderJoin)
{
  treeModified = FALSE;    // Indicates whether this tree is modified.

  NABoolean childModified; // Indicates whether any child of any of
                           // the join nodes are modified.

  // If this is not an inner join (i.e. semi-join, tsj, or outer join),
  // then we can't reorder this join node.  Just reorder the children
  // of this join node.
  if (getOperatorType() != REL_JOIN)
  {
    for (Lng32 i = 0; i < getArity(); i++)
    {
      child(i) = child(i)->reorderTree (childModified, doReorderJoin);
      if (childModified)
		treeModified = TRUE;
    }

    // If any of my children were reordered, then we need to
    // clear my logical properties for resynthesis.
    if (treeModified)
      getGroupAttr()->clearLogProperties();

    return this;
  }

  if (!doReorderJoin)
	return(this);

  RelExprList * OBRowcountList   = new(CmpCommon::statementHeap())
    RelExprList(CmpCommon::statementHeap());
  RelExprList * orderedList      = new(CmpCommon::statementHeap())
    RelExprList(CmpCommon::statementHeap());
  RelExprList * joinBackBoneList = new(CmpCommon::statementHeap())
    RelExprList(CmpCommon::statementHeap());

  // Walk down the Join tree, inserting children of the join nodes
  // in an ordered list (ordered by increasing estimated rowcount).
  RelExpr * expr       = this;
  while ((expr != NULL) AND (expr->getOperatorType() == REL_JOIN)
         // QSTUFF
         // we must prevent that accidentially a join is pushed into
         // a generic update subtree
         AND (NOT expr->getGroupAttr()->isGenericUpdateRoot())
         // QSTUFF
        )
  {
    // Remember the list of join backbones so we can do
    // reverse tree walks.
    joinBackBoneList->insertAt (0, expr);

    // Keep an ordered list of the join children (ordered by rowcount).
    OBRowcountList->insertOrderByRowcount (expr->child(1));

    // Keep an original ordering of join children.
    orderedList->insertAt (0, expr->child(1));

    if (expr->child(0)->getOperatorType() != REL_JOIN)
    {
      OBRowcountList->insertOrderByRowcount (expr->child(0));
      orderedList->insertAt (0, expr->child(0));
    }
    expr = expr->child(0);
  }

  // something went wrong, while inserting the expressions
  CMPASSERT (joinBackBoneList->entries() == OBRowcountList->entries()-1);

  if (*OBRowcountList != *orderedList)
    treeModified = TRUE;

  // **************************************************************
  // If none of the children of the join nodes visited here needed to be
  // reordered, then all that is left to be done in this call is to call
  // this method recursively on the children to see if they need to be
  // reordered.
  // **************************************************************
  if (NOT treeModified)
  {
    // First, reorder the left child of the innermost join node.
    (*joinBackBoneList)[0]->child(0) =
      (*OBRowcountList)[0]->reorderTree (childModified, doReorderJoin);
    if (childModified) treeModified = TRUE;

    // Second, reorder all the right children.
    for (Lng32 i = 0; i < (Lng32)joinBackBoneList->entries(); i++)
    {
      // set up the right child of the current join node.
      (*joinBackBoneList)[i]->child(1) =
        (*OBRowcountList)[i+1]->reorderTree (childModified, doReorderJoin);
      if (childModified)
        treeModified = TRUE;
      // If any of the children were reordered, then we need to clear
      // the logical properties of the current join expr for resynthesis.
      if (treeModified)
        (*joinBackBoneList)[i]->getGroupAttr()->clearLogProperties();
    }


    // cleanup
    delete OBRowcountList;
    delete orderedList;
    delete joinBackBoneList;

    return this;
  }

  // **************************************************************
  // SECOND: ACTUALLY REORDER the join tree.
  // **************************************************************

  // Now, perform a reverse tree walk of the join nodes.
  // Pull up join predicates to the top most join node, recomputing
  // outer references along the way.

  // First, set up the left child of the innermost join node.
  (*joinBackBoneList)[0]->child(0) =
    (*OBRowcountList)[0]->reorderTree (childModified, doReorderJoin);
  if (childModified) treeModified = TRUE;

  for (Lng32 i = 0; i < (Lng32)joinBackBoneList->entries(); i++)
  {
    // set up the right child of the current join node.
    (*joinBackBoneList)[i]->child(1) =
      (*OBRowcountList)[i+1]->reorderTree (childModified, doReorderJoin);
    if (childModified) treeModified = TRUE;

    // Pull up join predicates from its child join node, if any.
    // Also, recompute outer references for this child node.
    RelExpr * joinExpr = (*joinBackBoneList)[i];
    if (i > 0)
    {
      joinExpr->selectionPred() += joinExpr->child(0)->selectionPred();
      joinExpr->child(0)->selectionPred().clear();
    }

    // Make the inputs minimal and the outputs maximal before calling
    // pushdownCoveredExpressions
    if (i < (Lng32)joinBackBoneList->entries() -1)
    {
      ValueIdSet inputValues;
      inputValues =
        joinExpr->child(0).getGroupAttr()->getCharacteristicInputs();
      inputValues +=
        joinExpr->child(1).getGroupAttr()->getCharacteristicInputs();
      joinExpr->getGroupAttr()->setCharacteristicInputs(inputValues);
      joinExpr->primeGroupAttributes();
    }
  }

  // If any of my children were reordered, then we need to
  // clear my logical properties for resynthesis.
  if (treeModified)
    getGroupAttr()->clearLogProperties();

  // Now, walk down this newly transformed tree of join nodes,
  // pushing down join predicates that were pulled up.
  expr = this;

  // Is the left child of this inner join a inner join itself? We would
  // have pulled up join predicates only if this is true.
  if ((expr->child(0)->getOperatorType() == REL_JOIN)
      // QSTUFF
      // We didn't touch a generic update root, so no need to consider
      // pushing any predicates down to it.
      AND (NOT expr->child(0)->getGroupAttr()->isGenericUpdateRoot())
      // QSTUFF
     )
  {
    NABoolean moreJoinNodesWhosePredsWerePulledUp = TRUE;

    while (moreJoinNodesWhosePredsWerePulledUp)
    {
      // Push predicates down to the join so the poor guy gets the join
      // predicates he deserves.

      ValueIdSet originalPred = expr->getSelectionPred();
      RelExpr *exprRChild = expr->child(1);
      RelExpr *exprLChild = expr->child(0);
      ValueIdSet originalRCPred = exprRChild->getSelectionPred();
      ValueIdSet originalLCPred = exprLChild->getSelectionPred();
      ValueIdSet rcOutputs = exprRChild->getGroupAttr()->
                            getCharacteristicOutputs();

      expr->pushdownCoveredExpr(
        expr->getGroupAttr()->getCharacteristicOutputs(),
        expr->getGroupAttr()->getCharacteristicInputs(),
        expr->selectionPred());

      // remove predicates that are not covered at the right child
      // why we remove pushed down predicates from right child of join:
      // earlier in the method, predicates are pulled up only from the child(0)
      // but pushdown covered expression pushes down predicates to both
      // children. Now we attempt to undo work done for the right child
      // We need to remove uncovered predicates from child(1)
      // Solution: 10-070620-5692
      // restore original Right Child Selection predicate
      // restore original right child outputs
      // restore selection predicate of join expression: add predicates
      // pushed down to right child, but not to left

      // compute selection predicates passed down to right child,
      // but not to left child
      ValueIdSet updatedRChildPred = exprRChild->getSelectionPred();
      updatedRChildPred.subtractSet (originalRCPred);

      ValueIdSet updatedLChildPred = exprLChild->getSelectionPred();
      updatedLChildPred.subtractSet (originalLCPred);

      exprRChild->selectionPred() = originalRCPred;
      exprRChild->getGroupAttr()->setCharacteristicOutputs(rcOutputs);

      updatedRChildPred.subtractSet(updatedLChildPred);
      expr->selectionPred().addSet(
             updatedRChildPred);

      // Go to the next join node
      expr = expr->child(0);

      // The join may involve different tables then it did before, so we
      // will have to resynthesize the logical properties. Clearing them
      // now will force us to do this later.
      expr->getGroupAttr()->clearLogProperties();

      // If the next join node's left child isn't another inner join, then
      // we've pushed down all the join predicates that we pulled up.
      // So we are done.
      if ((expr->child(0)->getOperatorType() != REL_JOIN)
          // QSTUFF
          // We didn't touch a generic update root, so no need to consider
          // pushing any predicates down to it.
          OR expr->child(0)->getGroupAttr()->isGenericUpdateRoot()
          // QSTUFF
         )
      {
        moreJoinNodesWhosePredsWerePulledUp = FALSE;
      }

    } // end while more join nodes who need preds pushed to them

    // Finally, we need to remove from the lowest level join any veg
    // predicates we pushed down that are not true join predicates for
    // this join. The pushdown logic pushes all veg preds down even if
    // they are only covered by one of the children so that the veg pred
    // can be translated into a "IS NOT NULL" pred when it gets to a
    // leaf. But we don't keep pushing down all the way to the leaves,
    // so these veg preds can end up stranded at a join node where they
    // don't belong.  So, we need to get rid of them. We don't have to
    // worry about actually pushing them to the leaves because the
    // normalizer already did that. This code is stolen directly
    // from the end of Join::pushdownCoveredExpr().
    ValueIdSet VEGEqPreds;
    expr->selectionPred().lookForVEGPredicates(VEGEqPreds);

    // ---------------------------------------------------------------------
    // Remove those VEGPreds which are not covered at first child. For
    // example VEGPred(VEG{T2.a,T3.a}) in JOIN1 of (T1 JOIN1 (T2 JOIN2 T3))
    // is not covered at the first child. The predicate could be pushed
    // down to the second child without being retained at JOIN1.
    // ---------------------------------------------------------------------
    VEGEqPreds.removeUnCoveredExprs(
      expr->child(0).getGroupAttr()->getCharacteristicOutputs());

    // ---------------------------------------------------------------------
    // Remove those VEGPreds which are not covered at second child. For
    // example VEGPred(VEG{T1.a,T2.a}) in JOIN2 of ((T1 JOIN1 T2) JOIN2 T3)
    // is not covered at the second child. The predicate could be pushed
    // down to the first child without being retained at JOIN2.
    // ---------------------------------------------------------------------
    VEGEqPreds.removeUnCoveredExprs(
      expr->child(1).getGroupAttr()->getCharacteristicOutputs());

  } // end if we need to push down join preds we pulled up

  // cleanup
  delete OBRowcountList;
  delete orderedList;
  delete joinBackBoneList;

  return this;

} // Join::reorderTree

const MergeType Join::getMergeTypeToBeUsedForSynthLogProperties()
{
  MergeType mergeMethod;
  if (isSemiJoin())
    mergeMethod = SEMI_JOIN_MERGE;
  else if (isAntiSemiJoin())
    mergeMethod = ANTI_SEMI_JOIN_MERGE ;
  else if (isInnerNonSemiJoin() || getOperatorType() == REL_TSJ_FLOW)
    mergeMethod = INNER_JOIN_MERGE;
  else if (isLeftJoin() || isFullOuterJoin())
    mergeMethod = OUTER_JOIN_MERGE;
  else
  {
    CCMPASSERT("Incorrect join type");
    mergeMethod = INNER_JOIN_MERGE;
  }
  return mergeMethod;
}

const MCSkewedValueList * Join::getMCSkewedValueListForJoinPreds(ValueIdList & colGroup)
{
  ColStatDescList leftColStatDescList = child(0).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->colStats();
  ColStatDescList rightColStatDescList = child(1).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->colStats();

  ValueId col;
  ValueIdSet lhsCols, rhsCols;

  CollIndex index = NULL_COLL_INDEX;
  ValueIdSet joiningCols = getEquiJoinExprFromChild0();
  for (col = joiningCols.init();
             joiningCols.next(col);
             joiningCols.advance(col) )
  {
    leftColStatDescList.getColStatDescIndex(index, col);
    if (index == NULL_COLL_INDEX)
      continue;
    lhsCols.insert(leftColStatDescList[index]->getColumn());
  }

  index = NULL_COLL_INDEX;
  joiningCols.clear();
  joiningCols = getEquiJoinExprFromChild1();
  for (col = joiningCols.init();
             joiningCols.next(col);
             joiningCols.advance(col) )
  {
    rightColStatDescList.getColStatDescIndex(index, col);
    if (index == NULL_COLL_INDEX)
      continue;
    rhsCols.insert(rightColStatDescList[index]->getColumn());
  }

  if((lhsCols.entries() == 0) || 
     (rhsCols.entries() == 0) || 
     (lhsCols.entries() != rhsCols.entries()))
    return NULL;

  //Extract MC Skew values for column group that are part of join predicates
  ValueIdList lhsColGrp, rhsColGrp;
  MCSkewedValueList* leftMCSkewedValueList = (MCSkewedValueList *)leftColStatDescList.getMCSkewedValueListForCols(lhsCols, lhsColGrp);
  MCSkewedValueList* rightMCSkewedValueList = (MCSkewedValueList *)rightColStatDescList.getMCSkewedValueListForCols(rhsCols, rhsColGrp);

  CostScalar avgRowcountForNonSkewValuesOnLeftSide = leftColStatDescList.getAvgRowcountForNonSkewedMCValues(lhsCols, leftMCSkewedValueList);
  CostScalar avgRowcountForNonSkewValuesOnRightSide = rightColStatDescList.getAvgRowcountForNonSkewedMCValues(rhsCols, rightMCSkewedValueList);

  MCSkewedValueList * result = new (STMTHEAP) MCSkewedValueList(STMTHEAP);
  result->mergeMCSkewedValueList(leftMCSkewedValueList, 
                                 rightMCSkewedValueList, 
                                 avgRowcountForNonSkewValuesOnLeftSide, 
                                 avgRowcountForNonSkewValuesOnRightSide, 
                                 getMergeTypeToBeUsedForSynthLogProperties());
  colGroup = lhsColGrp;

  if ( result->entries() == 0 ) {
    NADELETE(result, MCSkewedValueList, STMTHEAP);
    result = NULL;
  }

  return result;
}

// -----------------------------------------------------------------------
// member functions for class Union
//
// First: Union::synthEstLogProp
// -----------------------------------------------------------------------

void
Union::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp)) return;

  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp());

  const ColStatDescList &outerColStatsList = inputEstLogProp->getColStats();

  EstLogPropSharedPtr leftEstProps;
  EstLogPropSharedPtr rightEstProps;
  EstLogPropSharedPtr copyInputEstProp;

  // if inputForSemiTSJ is set for inputEstLogProp it cannot be passed below the union
  //   so create a new EstLogProp with the flag set off to pass to my children
  if (inputEstLogProp->getInputForSemiTSJ() != EstLogProp::NOT_SEMI_TSJ)
    {
      copyInputEstProp = EstLogPropSharedPtr(new (HISTHEAP)
          EstLogProp(*inputEstLogProp));
      copyInputEstProp->setInputForSemiTSJ(EstLogProp::NOT_SEMI_TSJ);
    }
  else
    {
      copyInputEstProp = inputEstLogProp;
    }

  leftEstProps  = child(0).outputLogProp(copyInputEstProp);
  rightEstProps = child(1).outputLogProp(copyInputEstProp);

  CostScalar rowCount = leftEstProps->getResultCardinality() +
  rightEstProps->getResultCardinality();

  // The unresolved predicates of a union are those that are unresolved
  // by both children.
  myEstProps->unresolvedPreds() += leftEstProps->getUnresolvedPreds();
  myEstProps->unresolvedPreds().intersectSet(
       rightEstProps->getUnresolvedPreds());


  // -----------------------------------------------------------------
  // access the column statistics descriptor list from the left and
  // right child's estimated logical properties.
  // -----------------------------------------------------------------
  const ColStatDescList &leftColStatsList  = leftEstProps->getColStats();
  const ColStatDescList &rightColStatsList = rightEstProps->getColStats();

  ColStatDescList unionStatDescList;       // Result Column Stats.
  unionStatDescList.insertIntoUecList (leftColStatsList.getUecList()) ;
  unionStatDescList.insertIntoUecList (rightColStatsList.getUecList()) ;

  // Get the information needed for associating the children's columns
  // with the union's result columns
  const ValueIdList & unionMapTable = colMapTable();

  // -------------------------------------------------------------------
  // Because of the way that outer references are dealt with, there is
  // an implicit unioning operation between the copy of the outer ref'
  // column descriptors that were sent to the left child of the union and
  // those sent to the right.
  // First, produce the result of that implicit union so that it may
  // become part of the output logical properties of this operator.
  // -------------------------------------------------------------------
  CollIndex i;
  CollIndex j;
  CollIndex leftMatch = NULL_COLL_INDEX, rightMatch = NULL_COLL_INDEX;

  NABoolean foundLeftMatch  = FALSE;
  NABoolean foundRightMatch = FALSE;

  for (i = 0; i < outerColStatsList.entries(); i++)
    {
      // ------------------------------------------------------------------
      // For each outer reference.
      //   Look for the matching column statistics from the left child.
      //   if found:
      //      Look for the matching column statistics from the right child
      //      if found:
      //         if left and right column statistics include a histogram
      //            Produce a histogram for the result of the Union
      // ------------------------------------------------------------------
      ColStatDescSharedPtr outerStatDesc = outerColStatsList[i];
      const ItemExpr * outerItem =
	outerStatDesc->getVEGColumn().getItemExpr();

      foundLeftMatch  = FALSE;
      foundRightMatch = FALSE;

      // look for matching column statistics from the left side.
      for (j = 0; j < leftColStatsList.entries() && !foundLeftMatch; j++)
	{
          ColStatDescSharedPtr lStatDesc = leftColStatsList[j];
          const ItemExpr * leftItem =
	    lStatDesc->getVEGColumn().getItemExpr();

	  if (leftItem->getValueId() == outerItem->getValueId())
	    {
	      foundLeftMatch = TRUE;
	      leftMatch = j;
	    }
	}

      // look for matching column statistics from the right side.
      for (j = 0; j < rightColStatsList.entries() && !foundRightMatch; j++)
	{
	  ColStatDescSharedPtr rStatDesc = rightColStatsList[j];
	  const ItemExpr * rightItem =
	    rStatDesc->getVEGColumn().getItemExpr();

	  if (rightItem->getValueId() == outerItem->getValueId())
	    {
	      foundRightMatch = TRUE;
	      rightMatch = j;
	    }
	}

      if (foundLeftMatch && foundRightMatch)
	{
	  // Union together left & right column statistics' histograms
          ColStatDescSharedPtr lStatDesc = leftColStatsList[leftMatch];
          ColStatsSharedPtr lColStats    = lStatDesc->getColStats();

          ColStatDescSharedPtr rStatDesc = rightColStatsList[rightMatch];
          ColStatsSharedPtr rColStats    = rStatDesc->getColStats();

	  // perform the 'union' operation into the new col stats,
	  // and insert the new column statistics into the results list
	  CollIndex entry = unionStatDescList.entries();
	  unionStatDescList.insertDeepCopyAt(entry, lStatDesc);

	  unionStatDescList[entry]->mergeColStatDesc(rStatDesc, 
                                                 UNION_MERGE, 
                                                 FALSE, 
                                                 REL_UNION);
          }  // if foundLeftMatch && foundRightMatch
    } // for i

  // Now Union the actual result columns of the union operation, as
  // opposed to the returned input columns that are unioned above.
  for ( i = 0; i < unionMapTable.entries(); i++ )
  {
    // ------------------------------------------------------------------
    // For each entry in the union's colMapTable,
    //   Look for the matching column statistics from the left child.
    //   if found:
    //      Look for the matching column statistics from the right child
    //      if found:
    //         if left and right column statistics include a histogram
    //            Produce a histogram for the result of the Union
    // ------------------------------------------------------------------
    ItemExpr * idUnion = unionMapTable[i].getItemExpr();
    if (idUnion->getOperatorType() != ITM_VALUEIDUNION)
    {
      CCMPASSERT("Invalid expression in UNION");
      continue;
    }

    // cast the source columns of UNION to base columns

	  BaseColumn * leftSource = ((ValueIdUnion *)idUnion)->getLeftSource().castToBaseColumn();
	  BaseColumn * rightSource = ((ValueIdUnion *)idUnion)->getRightSource().castToBaseColumn();

	  foundLeftMatch  = FALSE;
    foundRightMatch = FALSE;

    // look for matching column statistics from the left side.
    for (j = 0; j < leftColStatsList.entries() && !foundLeftMatch; j++)
	  {
      ColStatDescSharedPtr lStatDesc = leftColStatsList[j];
      const BaseColumn * leftItem =
	    lStatDesc->getVEGColumn().castToBaseColumn();

		  if (leftItem == leftSource)
	    {
	      foundLeftMatch = TRUE;
	      leftMatch = j;
	    }
	  }

    // look for matching column statistics from the right side.
    for (j = 0; j < rightColStatsList.entries() && !foundRightMatch; j++)
	  {
	    ColStatDescSharedPtr rStatDesc = rightColStatsList[j];
	    const BaseColumn * rightItem =
	      rStatDesc->getVEGColumn().castToBaseColumn();

	    if (rightItem == rightSource)
	    {
	      foundRightMatch = TRUE;
	      rightMatch = j;
	    }
	  }

    if (foundLeftMatch && foundRightMatch)
	  {
	      // Union together left & right column statistics' histograms
        ColStatDescSharedPtr lStatDesc = leftColStatsList[leftMatch];
        ColStatsSharedPtr lColStats    = lStatDesc->getColStats();

        ColStatDescSharedPtr rStatDesc = rightColStatsList[rightMatch];
        ColStatsSharedPtr rColStats    = rStatDesc->getColStats();

        // perform the 'union' operation into the new col stats, and
	      // insert the new column statistics into the results col stat list
	      CollIndex entry = unionStatDescList.entries();
	      unionStatDescList.insertDeepCopyAt(entry, lStatDesc);

	      unionStatDescList[entry]->mergeColStatDesc(rStatDesc, 
                                                     UNION_MERGE,
                                                     FALSE,
                                                     REL_UNION);

	      // update the result's description of itself
	      //  Note that it is proper that this isn't done in the previous
	      //  body of code that looks (as is pointed out above), similar to
	      //  this code.
	      unionStatDescList[entry]->VEGColumn() = unionMapTable[i];

        // need to do the following, adding "my" ValueId to the list of
        // columns I've joined to, because in ColStatDesc::mergeColStatDesc()
        // we can get into trouble when we have a mergeState_ that is empty
        // --> we set to empty first (via clear()) because we do not want
        // the left table's mergeState information
        unionStatDescList[entry]->mergeState().clear() ;
        unionStatDescList[entry]->mergeState().insert(idUnion->getValueId()) ;
	    }  // if foundLeftMatch && foundRightMatch
    } // for i

  // the cardinality of a union is the sum of its child cardinalities
  myEstProps->setResultCardinality(rowCount);

  // maxCard(s1 UNION s2) == maxCard(s1) + maxCard(s2)
  myEstProps->setMaxCardEst
    (MAXOF(leftEstProps->getMaxCardEst() +               
           rightEstProps->getMaxCardEst(),
           rowCount));

  // -------------------------------------------------------------------
  // Lastly, determine which of these histograms are useful based on
  // the characteristic outputs and selection predicates for the group.
  // -------------------------------------------------------------------
  myEstProps->pickOutputs( unionStatDescList, inputEstLogProp, getGroupAttr()->getCharacteristicOutputs(), getSelectionPred());

  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstProps);

} // Union::synthEstLogProp

// ---------------------------------------------------------------------
// Union::synthLogProp
// ---------------------------------------------------------------------

void
Union::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  RelExpr::synthLogProp(normWAPtr);

  // if both left and right child have cardinality constraint, then it
  // is possible to create a cardinality constraint for the result
  Cardinality minLeft, minRight, maxLeft, maxRight;

  // ---------------------------------------------------------------------
  // synthesize uniqueness and cardinality constraints
  // ---------------------------------------------------------------------

  (void) child(0).getGroupAttr()->hasCardConstraint(minLeft,maxLeft);
  (void) child(1).getGroupAttr()->hasCardConstraint(minRight,maxRight);

  if (minLeft > 0 OR minRight > 0
      OR
      (maxLeft < INFINITE_CARDINALITY AND maxRight < INFINITE_CARDINALITY))
    {
      // if one of the upper bounds is infinity, then make the total upper
      // bound infinity, otherwise add the lower and upper bound together
      if (maxLeft == INFINITE_CARDINALITY OR maxRight == INFINITE_CARDINALITY)
	{
	  getGroupAttr()->addConstraint
	    (new(CmpCommon::statementHeap())
	     CardConstraint(minLeft+minRight,
			    (Cardinality)INFINITE_CARDINALITY));
	}
      else
	{
	  getGroupAttr()->addConstraint
	    (new(CmpCommon::statementHeap())
	     CardConstraint(minLeft+minRight,
			    maxLeft+maxRight));
	}
    }

  // do uniqueness constraints someday (find uniqueness constraints on both
  // sides and then find check constraints that make sure the values from
  // the left side don't overlap the values of the right side) $$$$

} // Union::synthLogProp()


// -----------------------------------------------------------------------
// member functions for class GroupByAgg
// -----------------------------------------------------------------------
#pragma nowarn(262)   // warning elimination
void
GroupByAgg::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp)) return;

  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp());
  EstLogPropSharedPtr intermedEstProps(new (HISTHEAP) EstLogProp());

  CostScalar maxCard = COSTSCALAR_MAX;

  //--------------------------------------------------------------------
  // access the column statistics descriptor lists passed in as outer
  // references and those generated by the child.
  //--------------------------------------------------------------------
  const ColStatDescList &outerColStatsList = inputEstLogProp->getColStats();
  CollIndex outerRefCount =  outerColStatsList.entries();

  // We will now get the Estimated Logical Properties of the child of
  // Materialize; also multipleCalls will tell us whether or not such
  // child gets called only once.
  Int32 multipleCalls;
  EstLogPropSharedPtr modInputLP;
  if (inputEstLogProp->getResultCardinality() > CostScalar (1) &&
      CmpCommon::getDefault(COSTING_SHORTCUT_GROUPBY_FIX) != DF_ON)
  {
    // Executor doesn't materialize grouby operator, but optimizer assumes
    // it does. Because of this we under estimate groupby cost when it
    // is right side of NJ. This CQD is being used since it is already
    // available and fix is generic, not specific to short cut grby.
    modInputLP =
    child(0).getGroupAttr()
            ->materializeInputLogProp(inputEstLogProp, &multipleCalls);
  }
  else
     modInputLP = inputEstLogProp;

  EstLogPropSharedPtr childEstProps;
  EstLogPropSharedPtr copyInputEstProp;

  // if inputForSemiTSJ is set for inputEstLogProp it cannot be passed
  // below the group-by ==> so create a new EstLogProp with the flag set
  // off to pass to my children
  if (inputEstLogProp->getInputForSemiTSJ() != EstLogProp::NOT_SEMI_TSJ)
    {
      copyInputEstProp = EstLogPropSharedPtr(new (HISTHEAP) EstLogProp(*modInputLP));
      copyInputEstProp->setInputForSemiTSJ(EstLogProp::NOT_SEMI_TSJ);
    }
  else
    {
      copyInputEstProp =  modInputLP;
    }

  childEstProps = child(0).outputLogProp(copyInputEstProp);

  const ColStatDescList &childColStatsList = childEstProps->getColStats();

  // Initially, the unresolved predicates of a GROUPBY are those that are
  // unresolved by the child.
  myEstProps->unresolvedPreds() += childEstProps->getUnresolvedPreds();

  ColStatDescList groupStatDescList;

  CollIndex i;
  CostScalar resultCardinality;
  NABoolean foundFlag;

  // make the group-by local copy of the information returned by the child
  groupStatDescList.makeDeepCopy( childColStatsList );

  //--------------------------------------------------------------------
  // The GROUP BY, implicitly, contains any COLUMNS appearing as Outer
  // References.  Add any such columns to the set of GROUPing columns.
  // Note: it is not necessary to add these columns to groupStatDescList,
  // as they should already be there.
  //
  // This modeling of the impact of outer references is not perfect, but
  // it is (in many cases) close to the truth, and it is about as good
  // as the group-by can do at this time.

  // For cardinality estimations we shall use only the selected columns
  //--------------------------------------------------------------------

  ValueIdSet workGroup = groupExpr_;
  const ValueIdSet requiredInput = getGroupAttr()->getCharacteristicInputs();

  workGroup.removeCoveredExprs(requiredInput);

  NABoolean trueScalAgg = (!aggregateExpr_.isEmpty() && groupExpr_.isEmpty());
  if(!trueScalAgg)
  {
  for ( i = 0; i < outerColStatsList.entries(); i++ )
    {
      const ItemExpr * colExpr = outerColStatsList[i]->getVEGColumn().getItemExpr() ;
      foundFlag = FALSE;  // assume invalid; then disprove assumption

      for ( ValueId idOut = requiredInput.init() ;
            requiredInput.next (idOut) AND NOT foundFlag ;
            requiredInput.advance (idOut) )
        {
          if ( colExpr->getValueId() == idOut )
            {
              foundFlag = TRUE;
              break ; // jump out to outer for-loop
            }
          else if ( idOut.getItemExpr()->getOperatorType() ==
                    ITM_VEG_REFERENCE )
            {
              VEG * nestedVEG = ((VEGReference *) (idOut.getItemExpr()))->getVEG() ;
              const ValueIdSet & VEGGroup = nestedVEG->getAllValues() ;

              for ( ValueId id = VEGGroup.init() ;
                    VEGGroup.next(id) ;
                    VEGGroup.advance(id) )
                {
                  if ( colExpr->getValueId() == id )
                    {
                      foundFlag = TRUE ;
                      break ; // break out to 2nd loop -- wish we could jump out more!
                    }
                }
            }
        }

      if ( foundFlag )
	{
          workGroup.insert (colExpr->getValueId()) ;
	}
    }
  }

  ValueIdSet workGroupCopy = workGroup; // save the GROUP-BY specification

  //--------------------------------------------------------------------
  // Determine something of a worst-case result cardinality for this
  // grouping operation. This is the cardinality of the child of GroupBy
  // for the given inputEstLogProp - nothing to group.
  //--------------------------------------------------------------------
  CostScalar oldCount = childEstProps->getResultCardinality();

  if (oldCount < 0)
  {
    CCMPASSERT (oldCount >= 0) ;
    oldCount = 0;
  }

  //--------------------------------------------------------------------
  // Then attempt to improve on that estimate.
  // is this a simple aggregate (no groupby columns) or a "real" groupby
  //--------------------------------------------------------------------

  CostScalar uecTotal = 1;
  CostScalar maxUECofGrpBy = 0;

  if(trueScalAgg)
    {
      // set the estimated cardinality to 1 also
      resultCardinality = 1 ;
      // For groupbys below nested join multiply by input cardinality.
      // This fix generic to scalar aggregate but is being controlled by
      // existing CQD COSTING_SHORTCUT_GROUPBY_FIX.
      if (CmpCommon::getDefault(COSTING_SHORTCUT_GROUPBY_FIX) == DF_ON)
      {
        CostScalar inputCard = copyInputEstProp->getResultCardinality();
        if (inputCard > csOne)
          resultCardinality = MIN_ONE(inputCard < oldCount ? inputCard : oldCount);
      }
    }
  else
    {
      resultCardinality = 1 ; // make sure this variable is assigned to!

      NAList<CollIndex> singleStats(CmpCommon::statementHeap());

      CollIndex lastStats = NULL_COLL_INDEX;

      CostScalar uecTotalFromCompExp = csOne;
      ValueIdSet vegWorkGroup, compExpWorkGroup;

      if (CmpCommon::getDefault(COMP_BOOL_148) == DF_ON)
      {
	workGroup.lookForVEGReferences(vegWorkGroup);
	workGroup.lookForVEGPredicates(vegWorkGroup);

	compExpWorkGroup.insert(workGroup.subtractSet(vegWorkGroup));	

	for ( ValueId compExpWorkGroupItem = compExpWorkGroup.init();
		      compExpWorkGroup.next(compExpWorkGroupItem);
		      compExpWorkGroup.advance(compExpWorkGroupItem))
	{
	  ItemExpr *colExpr = compExpWorkGroupItem.getItemExpr() ;
	  CostScalar minUec = csOne, maxUec = csOne;
	  if(colExpr->calculateMinMaxUecs(groupStatDescList, minUec, maxUec))
	    uecTotalFromCompExp *= minUec;
	  else
	    vegWorkGroup.insert(compExpWorkGroupItem);
	}

	workGroup = vegWorkGroup;
      }

      // -----------------------------------------------------------------
      // Determine how many groups there are.
      // The trick is to avoid having a single grouping column counted
      // multiple times because it appears in multiple statistics.
      //
      // Walk the group-by list multiple times, where every pass
      //  - locates the relevant multi-column histogram with the most
      //    columns;
      //  - factors the uec of that histogram into the running uecTotal;
      //  - removes the columns associated with that histogram from the
      //    group-by column set 'workGroup.'
      //
      //
      // First: Attempt to locate statistics for
      //   - individual columns currently appearing in the GROUP BY;
      //   - multi-column histograms, ALL of whose columns appear in the
      //     GROUP BY (eventually, test for Leading columns).
      //
      // While doing this, remember the multi-column histogram with the
      // most columns.
      // -----------------------------------------------------------------
      // -----------------------------------------------------------------
      // mar : at this point we need to check : do any of the
      //       grouping columns uniquely determine any of the others?
      //
      //       -> if so, this is the point (right before the loop below)
      //          that we need to remove them from workGroup
      // -----------------------------------------------------------------
      // look at base table information, use primary key information
      // from the NABaseColumn's
      //
      // we iterate through all of the columns in the
      // workGroup list
      //
      // . for each one, we look for information about
      // it in the groupStatDescList
      //
      // MAKE SURE THE COLUMN IS A ITM_BASECOLUMN
      // if (colExpr->getOperatorType() == ITM_BASECOLUMN) ...
      //
      // . if we find a column 'c' that is a primary key in its
      // base table, then we remove all columns from 'workGroup'
      // that are in the same table as 'c'

      const ValueIdList workGroupList = workGroup ;

      // first, look at all entries from the VEG's contained
      // in 'workGroup', saving the ones that are base columns
      // into 'baseColumnSet'.
      // Definitions:
      // baseColumnSet   - set of base columns which constitute the work group.
      // columnOfThisVEG - it is a list of ValueIdSets, where each ValueIdASet
      //                   is a set of baseColumns for each workGroup item.
      //                   Please note, that the workGroup is composed of VEG
      //                   columns and not the base columns
      // groupByTables   - Set of all tables whose columns are participating
      //                   in the groupBy.
      // columnToRemove  - From all the tables which participate in the groupBy
      //                   collect all non-primary key columns. These columns
      //		   are probable redundant columns.
      // probableRedundantColSet - are the columnToRemove that are also present
      //                   in the baseColumnSet.
      // interestingColSet - are the set of columns from the baseColumnSet which
      //                   are primary keys of the tables, these will be used later
      //                   to check for dependencies

      ValueIdSet baseColumnSet ;
      LIST(ValueIdSet ) columnsOfThisVEG(STMTHEAP);
      ValueIdSet probableRedundantColSet;
      SET(TableDesc *) * groupByTables = NULL;

      for ( i = 0 ; i < workGroupList.entries() ; i++ )
      {
	    ItemExpr *colExpr = workGroupList[i].getItemExpr() ;
	    ValueIdSet columns;

	    colExpr->findAll( ITM_BASECOLUMN, columns, TRUE, TRUE );
	    baseColumnSet.insert(columns);
	    columnsOfThisVEG.insertAt(i, columns);
      }

      // Now get all the base tables involved in GroupBy

      groupByTables = baseColumnSet.getAllTables();

      // for all tables, collect the columns that can removed. These should
      // not be non-primary key column

      ValueIdSet columnsToRemove;

      // Save columns that are interesting, and should stay if they do not have
      // any dependencies

      ValueIdSet interestingColSet;

      for (i = 0; i < groupByTables->entries(); i++ )
      {
	    TableDesc * thisTable = (*groupByTables)[i];
	    // get all primary key column and non-primary keys columns for this table
	    ValueIdSet primaryKeyColumns = (ValueIdSet)(thisTable->getPrimaryKeyColumns() );

	    ValueIdList userColumns;

	    // get All user columns for this table;
	    thisTable->getUserColumnList(userColumns);

	    // Non primary key columns will be user columns minus the primary key columns
	    ValueIdSet nonPrimaryKeyColumns(userColumns);

	    // subtract primary key columns from here to get non-primary key columns
	    nonPrimaryKeyColumns.subtractSet(primaryKeyColumns);
	    probableRedundantColSet.insert(nonPrimaryKeyColumns.intersect(baseColumnSet));

	    // use this nonprimarykey columns set to get all the columns that can
	    // be removed from grouping columns set. These can be removed only if the
	    // entire key is covered by the grouping column set

	    if ( !(primaryKeyColumns.isEmpty()) &&
	          (primaryKeyColumns.intersect(baseColumnSet) == primaryKeyColumns ) )
	    {
	      columnsToRemove.insert(nonPrimaryKeyColumns.intersect(baseColumnSet) );
	      interestingColSet.insert(primaryKeyColumns);
	    }
      }

      // If there are no columns to remove, or if there are no interesting columns
      // then we don't want to remove any columns from the workGroup.
      // Else, we shall remove the VEGes from the work group which contain
      // these columns, but do not contain any columns from the interesting
      // columns set. We will also check for dependency information if there
      // are primary keys from more than one tables.

      if (!columnsToRemove.isEmpty() && !interestingColSet.isEmpty() )
      {
	    ValueIdSet updatedColumnsToRemove = columnsToRemove;
    	
	    // go to each VEG in the work group to see if it can be removed
	    // can we use dependency information?
	    NABoolean considerDependency;

	    SET(TableDesc *) * interestingColTables = interestingColSet.getAllTables();

	    if ( (!probableRedundantColSet.isEmpty()) && (interestingColTables->entries() > 1))
	      considerDependency = TRUE;
	    else
	      considerDependency = FALSE;

	    //First remove the redundant columns that have direct dependency
	    for ( i = 0 ; i < workGroupList.entries() ; i++ )
	    {
	      ValueIdSet interestingColsFromThisVeg = columnsOfThisVEG[i].intersect(interestingColSet);

	      // sanity check to ensure that there is atleast one item in the workGroup.
	      if (workGroup.entries() == 1) break;

	      ValueIdSet redColCandidates = columnsOfThisVEG[i].intersect(columnsToRemove);

	      if ( (!redColCandidates.isEmpty() ) &&
	          (interestingColsFromThisVeg.isEmpty()) )
	      {
	        workGroup.remove(workGroupList[i]);
	        updatedColumnsToRemove.subtractSet(redColCandidates);
	      }
	    }

	    // Now, consider indirect dependencies among remaining grouping columns
	    if (considerDependency)
	      handleIndirectDepInGroupingcols(workGroup, interestingColSet, updatedColumnsToRemove, 
					  probableRedundantColSet, groupStatDescList);
      }

	  // Now that we have a final list of grouping cols, lets figure out
	  // the maximum limit for group by cardinality

	  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

	  if (appStatMan && (workGroup.entries() > 0))
	  {
		NABoolean allTablesConsidered = TRUE;

		// currMax is used to keep track of the running max card
		// used to set the upper limit to the cardinality of GroupBy
		CostScalar currentMax = csOne;

                SET(TableDesc *) * workGroupTables = new STMTHEAP SET(TableDesc *)(STMTHEAP);
                SET(TableDesc *) * relevantGroupTables = new STMTHEAP SET(TableDesc *)(STMTHEAP);

                // get all subtree tables of this group
                if (getGroupAttr()->getGroupAnalysis())
                {
                  CANodeIdSet subtreeset = getGroupAttr()->getGroupAnalysis()->getAllSubtreeTables();
                  for (CANodeId id =  subtreeset.init();
		    subtreeset.next(id);
		    subtreeset.advance(id))
                  {
                    TableDesc * tdesc = id.getNodeAnalysis()->getTableAnalysis()->getTableDesc();
                    workGroupTables->insert(tdesc);
                  }
                }
                
                // Now go thru all sub tree tables to see which one of them
                // satisfy all grouping expressions
                for (CollIndex j = 0; j < workGroupTables->entries(); j ++)
                {
                  TableDesc * thisTable = (*workGroupTables)[j];
                  const ValueIdSet allColsOfThisTable = thisTable->getColumnList();
                  NABoolean useTableForMaxCard = TRUE;

	          for ( ValueId workGroupItem = workGroup.init();
			        workGroup.next(workGroupItem);
			        workGroup.advance(workGroupItem))
	          {
	            ItemExpr *colExpr = workGroupItem.getItemExpr() ;
                    colExpr = colExpr->getLeafValueIfUseStats();
		    ValueIdSet columns;

		    colExpr->findAll( ITM_BASECOLUMN, columns, TRUE, TRUE );

		    if ( (colExpr->getOperatorType() != ITM_VEG_REFERENCE) ||
			    (columns.isEmpty()) )
		    {
			  // This is a case where the grouping expression contains
			  // ValueIdUnion of constants. This could be a typical case
			  // of Transpose. In this case we do not want to set any
			  // limit to the max cardinality
		          // if there are no grouping columns, there is nothing to do
			  useTableForMaxCard = FALSE;
			  break;
		    }

		    if (!columns.isEmpty())
		    {
                      // is any column in this workGroup contained in the subtree table?
                      // If not, then we cannot use this table to compute maxCard of GroupBy
                      // skip to next table
                      columns.intersectSet(allColsOfThisTable);
                      if (columns.isEmpty())
                      {
                        useTableForMaxCard = FALSE;
                        break;
                      }
		    }
		  }
                  if (useTableForMaxCard)
                    relevantGroupTables->insert(thisTable);
                }

		if (relevantGroupTables->entries() !=  workGroupTables->entries())
		  allTablesConsidered = FALSE;
                {
		  for (i = 0; i < relevantGroupTables->entries(); i++ )
		  {
		    TableDesc * thisTable = (*relevantGroupTables)[i];

		    const TableAnalysis * thisTableAnalysis = thisTable->getTableAnalysis() ;
		    if (thisTableAnalysis &&
			    thisTableAnalysis->getNodeAnalysis() )
		    {
			  CANodeId localNodeId = thisTableAnalysis->getNodeAnalysis()->getId();
                          NABoolean flag = (inputEstLogProp->isCacheable() ? TRUE : FALSE);
                          if (inputEstLogProp != (*GLOBAL_EMPTY_INPUT_LOGPROP))
                            inputEstLogProp->setCacheableFlag(FALSE);
			  currentMax *= appStatMan->getStatsForCANodeId(localNodeId, inputEstLogProp)->getResultCardinality();
                          inputEstLogProp->setCacheableFlag(flag);
		    }
		    else
		    {
			  allTablesConsidered = FALSE;
			  break;
		    }
		  } // for all relevantGroupTables
                }

		if ((allTablesConsidered) && (workGroupTables->entries()))
                  {
                    maxCard = currentMax;
                    maxUECofGrpBy = currentMax;
                  }
	  } // if AppStatMan

	  // ------------------------------------------------------------------
      // the first thing we want to do is take advantage of any
      // multi-column uec information that's available to us;
      // if we have a multi-column uec value for columns that all
      // appear in the grouping list, then remove those columns from the
      // grouping list and use this uec value in their place.
      // ------------------------------------------------------------------

      const MultiColumnUecList * ueclist = groupStatDescList.getUecList() ;
      ValueIdSet subset, prevSubset ;

      NABoolean largeTableNeedsStats = FALSE;
      // should stats exist for this table?

      if (groupStatDescList.entries() > 0)
	largeTableNeedsStats = groupStatDescList[0]->getColStats()->isUpStatsNeeded();

      // we need baseColumnSet, if we need to display missing stats warning.
      // This will be needed only if there are more than one grouping columns
      // remaining.

      SET(TableDesc *) * tablesWithMissingStats = new STMTHEAP SET(TableDesc *)(STMTHEAP);

      // can't do anything if the groupby list only contains 1 column
      while ( workGroup.entries() > 1 && ueclist != NULL )
        {
	  baseColumnSet.clear();
	  for ( ValueId workGroupItem = workGroup.init();
			workGroup.next(workGroupItem);
			workGroup.advance(workGroupItem))
	  {
	    ItemExpr *colExpr = workGroupItem.getItemExpr() ;
	    ValueIdSet columns;

	    if (colExpr == NULL) continue;

	    colExpr->findAll( ITM_BASECOLUMN, columns, TRUE, TRUE );
	    baseColumnSet.insert(columns);
	  }

	  prevSubset = subset ; // used to avoid infinite loops!
          subset.clear() ;

	  // There could be cases when the null-istantiated column has participated
	  // in the join. This could be a case of left joins or in Union. In those
	  // cases, the mergeState is updated by the nulledExpression (don't know why?).
	  // But this should not have an impact on adjusting cardinalities for groupBy
	  // So, in case of null_instantiated column, retrieve its child and use
	  // that to compute multi-col uec adjustment. Sol: 10-060609-7077 and 10-060607-7010

	  subset = ueclist->largestSubset (baseColumnSet) ;
          NABoolean displayWarning = TRUE;

          if ( subset.entries() < 2 )
	  {
	    // there are no multi-column UECs left, hence display
	    // the missing stats warning for all the grouping columns
	    // of the remaining tables
	    for (i = 0; i < groupByTables->entries(); i++ )
	    {
	      TableDesc * thisTable = (*groupByTables)[i];

	      if (thisTable == NULL) continue;

	      ValueIdSet userColumnsSet(thisTable->getColumnList());
	      userColumnsSet.intersectSet(baseColumnSet);

	      // if warning has not been inserted for this table
	      // and number of columns to display the warning is greater than 1
	      if ( tablesWithMissingStats
		  && !tablesWithMissingStats->contains(thisTable)
		  && (userColumnsSet.entries() >= 2) )
	      {
		// Check if the MC stats dont exist in the private copy of MC UEC list of the table
		const MultiColumnUecList * ueclistOfThisTable = thisTable->getTableColStats().getUecList();
		if (!ueclistOfThisTable 
                    || (ueclistOfThisTable->lookup(userColumnsSet) == csMinusOne))
		{
                  // display missing stats warning, only if the warning_level
                  // is appropriately set and MC stats could be useful
                  if ((CURRSTMT_OPTDEFAULTS->histMissingStatsWarningLevel() > 2) &&
                      (ueclist->isMCStatsUseful(userColumnsSet, thisTable) ) )
                    displayWarning = TRUE;
                  else
                    displayWarning = FALSE;

                  ueclist->displayMissingStatsWarning(thisTable,
                                                      userColumnsSet,
                                                      largeTableNeedsStats,
                                                      displayWarning,
                                                      groupStatDescList,
                                                      csMinusOne,
                                                      FALSE,
                                                      REL_GROUPBY);


		  tablesWithMissingStats->insert(thisTable);
		}
	      }

	      baseColumnSet.subtractSet(userColumnsSet);
	      if (baseColumnSet.entries() ==0)
		break;
	    }

            break ; // no multi-column-uec's of interest are available
	  }

	  // We do have multi-column UEC available. Check to see if it exists for
	  // all the grouping columns of the table. If not, display missing
	  // stats warning for those columns, and then continue to look for
	  // partial multi-col UECs. This would take care of case, where say
	  // we have group by on (a, b, c, d) and multi-col UEC for (a, b)
	  // and (c, d). The optimizer would use these multi-col statistics
	  // on the same hand it will also give a missing stats warning for
	  // (a, b, c, d)

	  ValueId firstElement;

	  if (subset.isEmpty() ) break;

	  subset.getFirst(firstElement);

	  ItemExpr * thisExpr = firstElement.getItemExpr();

	  if (thisExpr == NULL) break;

	  TableDesc * thisTable = ((BaseColumn *)thisExpr)->getTableDesc();

	  if (thisTable == NULL) break;

	  ValueIdSet userColumnsSet(thisTable->getColumnList());
	  userColumnsSet.intersectSet(baseColumnSet);

	      // if warning has not been inserted for this table
	      // and number of columns to display the warning is greater than 1
	      if ( tablesWithMissingStats
		  && !tablesWithMissingStats->contains(thisTable)
		  && (subset.entries() != userColumnsSet.entries() ) )
	  {
	    // display missing stats warning, only if the warning_level
            // is appropriately set and MC stats could be useful
            if ((CURRSTMT_OPTDEFAULTS->histMissingStatsWarningLevel() > 2) &&
                (ueclist->isMCStatsUseful(userColumnsSet, thisTable)) )
              displayWarning = TRUE;
            else
              displayWarning = FALSE;

            ueclist->displayMissingStatsWarning(thisTable,
						userColumnsSet,
						largeTableNeedsStats,
                                                displayWarning,
                                                  groupStatDescList,
                                                  csMinusOne,
                                                  FALSE,
                                                  REL_GROUPBY);

	    tablesWithMissingStats->insert(thisTable);

	  }

	  if ( subset == prevSubset )
	    break; // to avoid inf loop

	  // what's the group uec associated with this subset
          CostScalar subsetUec = ueclist->lookup (subset) ;

	  if ( subsetUec <= 0 )
	    break; // some problem with statistics!

          // if any single-column uec info or histograms are missing,
          // then we can't use the multi-uec-list information
          NABoolean cannotContinue = FALSE ;

          // now figure out how much each of the columns in subset has
          // had its uec reduced
          CollIndex i ;

          ValueIdList subsetList = subset ; // convenience : set->list

	  // product fo single column UECs will be used to set the subsetUec

	  CostScalar SC_Uec = csOne;

		  // loop over all subset entries
          for ( i = 0 ; i < subsetList.entries() ; i++ )
            {
              // first, find out what each single-column's uec was initially
              ValueIdSet wrapper ;
              wrapper.insert (subsetList[i]) ;
              CostScalar initUec = ueclist->lookup (wrapper) ;
              if ( initUec == -1 ) // error flag indicating it wasn't found
                {
                  cannotContinue = TRUE ;
                  break ;
                }
              // second, find out what each single-column's uec is now
              CostScalar currUec = 0 ;
              CollIndex index ;

              // do a lookup in the CSDL for this ValueId
              if ( groupStatDescList.getColStatDescIndexForColumn (index, subsetList[i]) )
                {
                  currUec = groupStatDescList[index]->getColStats()->getTotalUec() ;
                }
              else
                {
                  cannotContinue = TRUE ;
                  break ;
                }

	      // groupBy card should be a MIN
	      // of multiColUec and the product of single column UEC of
	      // participating columns
	      SC_Uec *= currUec;
            } // done processing this subset

          if ( cannotContinue ) // some stats were missing
            break ;

          subsetUec = MINOF(subsetUec, SC_Uec);

          uecTotal *= subsetUec ;

          // Now remove this subset and continue
          //
          // want to write : workGroup.remove (subset) ;
          //
          // But: workGroup contains VEGRef's mostly (entirely?), so we
          // need to use a slightly more complicated function to perform
          // this operation.
          //
          for ( i = 0 ; i < subsetList.entries() ; i++ )
          {
            UInt32 initialGrpCnt = workGroup.entries();
            ValueId referencingExpr ;
            while (( initialGrpCnt > 0) && (workGroup.entries() > 0) )
            {
              if (workGroup.referencesTheGivenValue( subsetList[i], referencingExpr) )
                workGroup.remove (referencingExpr) ;
              initialGrpCnt --;
            }
	      }
        } // while ( workGroup.entries() > 0 )

      // ------------------------------------------------------------------
      // now that we've handled all multi-column uec information, now
      // see what information we have for single column stats
      // ------------------------------------------------------------------

      singleStats.clear();

      for ( i = 0; i < groupStatDescList.entries(); i++ )
        {
          const ItemExpr * colExpr =
            groupStatDescList[i]->getVEGColumn().getItemExpr() ;

          ValueId tmp ;
          if ( workGroup.contains( colExpr->getValueId() ) )
            singleStats.insert(i);
          //
          // $$$ The following 2nd check is part of a fix for a problem which came
          // $$$ up in TPC-D -- we're grouping by an EXTRACT value, and in this
          // $$$ case the UEC should at the very most stay the same (if not go down --
          // $$$ that's a change we can leave for another day).  To wit, we need
          // $$$ this column's uec value!
          // $$$
          // $$$ The current solution may be too general -- maybe we should limit
          // $$$ this to entries in workGroup which are for EXTRACT ops ...?
          //
          else if ( workGroup.referencesTheGivenValue ( colExpr->getValueId(), tmp ) )
            singleStats.insert(i) ;
        }  // for i

      // -----------------------------------------------------------------
      // Next, walk the list of remaining single-column statistics.
      // -----------------------------------------------------------------
      CollIndex loopCount = 0 ;

  for (i = 0; (i < singleStats.entries()) && (uecTotal < oldCount); i++ )
	{
	  ColStatsSharedPtr groupStats =
	    groupStatDescList[ singleStats[i] ]->getColStats();
	  const ValueId columnVEG =
	    groupStatDescList[ singleStats[i] ]->getVEGColumn();

	  const ItemExpr * colExpr = columnVEG.getItemExpr();
      const ValueId columnId = colExpr->getValueId() ;

      // $$$ this is the second part to take care of the
      // $$$ "EXTRACT-groupby-TPCD" problem
      //
      // 1. first, we see whether this column is referenced by the expression
      // 2. if so, then we find the item in workGroup that ref's this column
      // 3. remove that item from workGroup
      // 4. There could be more than one expressions referencing that column
      // remove all of them
      UInt32 initialGrpCnt = workGroup.entries();
      while(( initialGrpCnt > 0) && (workGroup.entries() > 0) )
      {
        ValueId referencingExpr ;
        if (workGroup.referencesTheGivenValue // not always true, if expr refs 2 cols
            ( columnId, referencingExpr) )
          workGroup.remove (referencingExpr) ;
        initialGrpCnt--;
      }
      // else ... it's probably correct to multiply by this column's uec
      // ... I think ...

      uecTotal *= groupStats->getTotalUec();

      loopCount++;
	  lastStats = singleStats[i];
	}

	uecTotal *= uecTotalFromCompExp;

      // -----------------------------------------------------------------
      // Finally, determine the reported result Cardinality based upon
      // the above uecTotal, combined with knowing whether or not we've
      // found statistics for all the grouping columns.
      // -----------------------------------------------------------------
      if ( workGroup.entries() == 0 )
	{
	  // In the special case where one single ColStats totally covers
	  // the grouping list, and only in that case, the rowcounts of
	  // that column's histogram should be set to match its UECs.
	  if ( loopCount == 1 )
	    {
	      ColStatsSharedPtr groupStats =
		groupStatDescList[ lastStats ]->getColStatsToModify();

	      groupStats->makeGrouped(); // apply impact of grouping
	    }
	  // For groupbys below nested join we do some extra adjustments
	  CostScalar inputCard = copyInputEstProp->getResultCardinality();
	  if (inputCard > csOne)
	  {
	    double rInput = inputCard.getValue();
	    double uChild = uecTotal.getValue();
	    double rChild = oldCount.getValue();
	    double finalCard;

	    finalCard = rInput * uChild * ( 1 - ((rInput - 1) / rInput) * exp( - rChild / (rInput*uChild)) );

	    resultCardinality = MIN_ONE(finalCard < rChild ? finalCard : rChild );
	  }
	  else
	    resultCardinality = MIN_ONE_CS(uecTotal < oldCount ? uecTotal : oldCount);
	}
      else
	{
	  // for now, do worst-case based model
	  resultCardinality = oldCount;
	}
    } // a "real" groupby

  if (resultCardinality > maxCard)
	resultCardinality = maxCard;

  // Ensure that all histograms report the same rowcount.
  CollIndex loopLimit = groupStatDescList.entries();

  for (i = 0; i < loopLimit; i++)
    {
      oldCount = groupStatDescList[i]->getColStats()->getRowcount();

      if (oldCount != resultCardinality)
	{
	  // IF the identified column is part of the GROUP BY, then its
	  // UEC shouldn't change due to Grouping.  Otherwise, its uec
	  // should be reduced (as elsewhere) to the same degree that its
	  // rowcount has been reduced.
	  ValueId columnVEG = groupStatDescList[i]->getVEGColumn() ;

          const ItemExpr * colExpr = columnVEG.getItemExpr();

          if ( workGroupCopy.contains(colExpr->getValueId()) )
            {
              const NABoolean doNotReduceUEC = TRUE ;
              groupStatDescList[i]->synchronizeStats (oldCount,
                                                      resultCardinality,
                                                      ColStatDesc::DO_NOT_REDUCE_UEC) ;
            }
	  else
            {
              groupStatDescList[i]->synchronizeStats (oldCount,
                                                      resultCardinality) ;
            }
	}
    }

  //---------------------------------------------------------------------
  // Save an intermediate EstLogProp for use by Physical Costing.
  // As usual, this is a Deep copy to prevent inadvertent modifications
  // of the data, by subsequent changes to the Histograms.
  //---------------------------------------------------------------------
  ColStatDescList & intermedStatsList = intermedEstProps->colStats();

  intermedStatsList.makeDeepCopy( groupStatDescList );

  if (isAPartialGroupByLeaf())
  {
    // DP2 will group as many rows as it can; the rows it cannot group it
    // will simply pass through to its parent to complete the group-by ==>
    // so that's why this code below involves physical costing stuff ...

     // No of groups the memory accommodates.
     // Length of the group with aggregates and the hash table overhead.
     const ValueIdSet& grbyVis = groupExpr();
     const ValueIdSet& aggrVis = aggregateExpr();


     // Length in bytes of the group key and the aggregates.
     CostScalar groupKeyLength = (grbyVis.isEmpty() ? 0 : grbyVis.getRowLength());
     CostScalar aggregateLength = (aggrVis.isEmpty() ? 0 : aggrVis.getRowLength());
     CostScalar hashedRowOverhead = (CostPrimitives::getBasicCostFactor(HH_OP_HASHED_ROW_OVERHEAD));
     CostScalar extGroupLength =
                  groupKeyLength + aggregateLength + hashedRowOverhead;
     CostScalar memoryLimitInDP2 = (CostPrimitives::getBasicCostFactor(HGB_DP2_MEMORY_LIMIT));
     CostScalar groupCountInMemory =
                              memoryLimitInDP2 * 1024. / extGroupLength;
     groupCountInMemory = MINOF(groupCountInMemory, resultCardinality);
     CostScalar mem = groupCountInMemory * extGroupLength / 1024.;

     CostScalar ipRows = ( groupExpr_.isEmpty() ? inputEstLogProp->getResultCardinality() :
			   childEstProps->getResultCardinality());

     CostScalar rowsEliminated = groupCountInMemory / resultCardinality * ipRows;
     resultCardinality = MAXOF (groupCountInMemory,
                                ipRows - rowsEliminated + groupCountInMemory);

     // WaveFix Begin
     // This is part of the fix for the count(*) wave
     // if there is a scalar agregate query on a multi-partitioned table,
     // something like Select count(*) from fact;
     // In such a case we would like to get a layer of esps,
     // doing so causes the plan to fixup in parallel avoiding the serial
     // fixup if the plan is just the master executor on top of dp2. The
     // serial fixup causes the query to execute in wave pattern, since
     // each dp2 is fixed up and then starts execution. Due to serial
     // fixup a dp2 is fixed up, and then we move to the next dp2 causing
     // the wave pattern.
     if (QueryAnalysis::Instance() &&
         QueryAnalysis::Instance()->dontSurfTheWave() &&
         isAPartialGroupByLeaf())
     {
       resultCardinality = childEstProps->getResultCardinality()/2;
     }
     // WaveFix End
  }

  intermedEstProps->unresolvedPreds() += myEstProps->getUnresolvedPreds();
  intermedEstProps->setResultCardinality( resultCardinality.minCsOne() );

  // maxCard(GrpBy(X)) == 
  //   MIN(card(GrpBy(x))*(maxCard(X)/card(X)), max UEC of grouping columns)
  CostScalar maxRows = resultCardinality.minCsOne() * 
    (childEstProps->getMaxCardEst() / 
     childEstProps->getResultCardinality().minCsOne());
  if (maxUECofGrpBy > 0 // it is credible
      && maxUECofGrpBy < maxRows) // max UEC of grouping columns < maxCard
    {
      // maxCard cannot exceed max UEC of grouping columns
      maxRows = maxUECofGrpBy; 
    }
  CostScalar maxSelectivity = csOne;

  //------------------------------------------------------------------------
  // Next, apply any HAVING Clause
  //------------------------------------------------------------------------
  const ValueIdSet & inputValues = getGroupAttr()->getCharacteristicInputs();
  ValueIdSet outerReferences;

  // Get the set of outer references.  If input value is a VEGReference,then
  // include this VEGReference only if the group consists of no constants.
  if (inputValues.entries() > 0)
    inputValues.getOuterReferences (outerReferences);

  ColStatDescList groupStatDescListForMaxSel(CmpCommon::statementHeap());
  groupStatDescListForMaxSel.makeDeepCopy(groupStatDescList);
  // compute maxSelectivity. toss rowcount return value.
  (void) groupStatDescListForMaxSel.estimateCardinality
    (resultCardinality, getSelectionPred(), outerReferences, NULL, NULL, NULL,
     outerRefCount, myEstProps->unresolvedPreds(), INNER_JOIN_MERGE,
     ITM_FIRST_ITEM_OP, &maxSelectivity);

  CostScalar newRowCount =
    groupStatDescList.estimateCardinality (resultCardinality,  /*in*/
                                          getSelectionPred(), /*in*/
                                          outerReferences,    /*in*/
                                          NULL,
                                          NULL,               /*for selectivityHint, which can only be given for Scan*/
                                          NULL,               /* for Cardinality hint, which can only be given for Scan */
                                          outerRefCount,      /*in/out*/
                                          myEstProps->unresolvedPreds(), /*in/out*/
                                          INNER_JOIN_MERGE, /*in*/
                                          ITM_FIRST_ITEM_OP, /*no-op*/
                                          NULL);

  groupStatDescList.computeMaxFreq(TRUE);
  myEstProps->setResultCardinality( newRowCount );
  myEstProps->setMaxCardEst(MAXOF(maxRows * maxSelectivity, newRowCount));
  groupStatDescList.synchronizeStats(newRowCount, groupStatDescList.entries());

  // -------------------------------------------------------------------
  // Now, determine which of these histograms are useful based on the
  // characteristic outputs for the group.
  // -------------------------------------------------------------------

  ValueIdSet predSet;

  myEstProps->pickOutputs( groupStatDescList, inputEstLogProp, getGroupAttr()->getCharacteristicOutputs(), predSet);

  //---------------------------------------------------------------------------
  // Save two sets of output estimated logical properties.
  // 1) Intermediate OLP = rowcount estimates after applying grouping
  // 2) Final OLP = rowcount estimates after applying grouping and having preds
  //----------------------------------------------------------------------------
  getGroupAttr()->addInputOutputLogProp (inputEstLogProp,
		 myEstProps /* final OLP - after applying grouping and HAVING preds */,
		 intermedEstProps /* intermediate OLP - after grouping */);
} // GroupByAgg::synthEstLogProp
#pragma warn(262)  // warning elimination

void GroupByAgg::handleIndirectDepInGroupingcols(ValueIdSet& workGroup,
				      ValueIdSet& interestingColSet,
				      ValueIdSet& updatedColumnsToRemove,
				      ValueIdSet& probableRedundantColSet,
				      const ColStatDescList & groupStatDescList)
{
  const ValueIdList newWorkGroupList = workGroup ;

  // The following list stores the list of VEG'es that share circular
  // dependencies among the columns in grouping expression
  CollIndexList cirDepCandidates(CmpCommon::statementHeap());
  
  SET(TableDesc *) * interestingColTables = interestingColSet.getAllTables();

  for ( CollIndex i = 0 ; i < newWorkGroupList.entries() ; i++ )
  {
    // sanity check to ensure that there is atleast one item in the workGroup.
    if (workGroup.entries() == 1) break;
    
    ItemExpr *colExpr = newWorkGroupList[i].getItemExpr() ;
    ValueIdSet colsFromThisVeg;
    colExpr->findAll( ITM_BASECOLUMN, colsFromThisVeg, TRUE, TRUE );

    ValueIdSet interestingColsFromThisVeg = colsFromThisVeg.intersect(interestingColSet);

    // now see if any primary keys columns have dependencies with non-key columns
    // and should be removed. This would be necessary only if there are more than one
    // primary keys and also there are some redundant columns
    if (!interestingColsFromThisVeg.isEmpty() )
    {
      ValueIdSet nonInterestingColSetFromVeg = colsFromThisVeg;
      nonInterestingColSetFromVeg.subtractSet(interestingColSet);
      nonInterestingColSetFromVeg.intersectSet(probableRedundantColSet);
      if( !(nonInterestingColSetFromVeg.isEmpty()) )
      {
	NABoolean itemCanBeRemoved = FALSE;

	// The following logic tests for indirect dependency.
	// Only the VEG from the grouping expression that satisfies 
	// the following conditions will be removed;
	//
	// 1. Non-primary key columns in the VEG should have the primary key column of the 
	//    respective table in the grouping expression;
	// 2. Primary key column in the VEG should be the only column from that particular
	//    table in the grouping expression;  
      
	SET(TableDesc *) * nonInterestingColTablesFromVeg = nonInterestingColSetFromVeg.getAllTables();

	// For the first condition
	for (CollIndex j = 0; j < nonInterestingColTablesFromVeg->entries(); j++ )
	{
	  if(interestingColTables->contains(nonInterestingColTablesFromVeg->at(j)))
	  {
	    itemCanBeRemoved = TRUE;
	    break;
	  }
	}
      
	if(itemCanBeRemoved)
	{
	  for (ValueId col = interestingColsFromThisVeg.init();
				interestingColsFromThisVeg.next(col);
				interestingColsFromThisVeg.advance(col))
	  {

	    for ( CollIndex l = 0; l < groupStatDescList.entries(); l++ )
	    {
	      if(newWorkGroupList[i] == groupStatDescList[l]->getVEGColumn())
	      {
		ValueIdSet colSetOfThisTbl = col.castToBaseColumn()->getTableDesc()->getColumnList();
		// For the second condition
		if(!colSetOfThisTbl.intersectSet(updatedColumnsToRemove).isEmpty())
		{
		  itemCanBeRemoved = FALSE;
		  // Found a column that has a circular dependency, add it to the list
		  cirDepCandidates.insert(i);
		}
		break;
	      }
	      if(l == groupStatDescList.entries()-1)
		itemCanBeRemoved = FALSE;
	    }
	    if(!itemCanBeRemoved)
	      break;
	  }
	}

	if(itemCanBeRemoved)
	  workGroup.remove(newWorkGroupList[i]);
      }
    }
  }

  // Retain only the column with minimum UEC from the list of columns that have circular dependencies
  CollIndex count = cirDepCandidates.entries();
  if(count > 1)
  {
    CostScalar minUec = csZero, uec = csOne;
    for ( CollIndex k = 0; k < count; k++ )
    {
      // At least one of the columns from the circular dependency list should be retained.
      if( (k == count-1) && (minUec == csZero) )
	break;

      for ( CollIndex l = 0; l < groupStatDescList.entries(); l++ )
      {
	if(newWorkGroupList[cirDepCandidates[k]] == groupStatDescList[l]->getVEGColumn())
	{
	  uec = groupStatDescList[l]->getColStats()->getTotalUec();

	  // Remove all the columns except the one with lowest UEC
	  if (uec > minUec)
	    workGroup.remove(newWorkGroupList[cirDepCandidates[k]]);
	  else
	    minUec = uec;
  	
	  break;
	}

	// Remove the VEG if the UEC couldnt be calculated
	if(l == groupStatDescList.entries()-1)
	  workGroup.remove(newWorkGroupList[cirDepCandidates[k]]);
      }
    }
  }
}

void
GroupByAgg::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  // Synthesize log. properties for me and all my children.
  RelExpr::synthLogProp(normWAPtr);

  if (NOT isAPartialGroupByLeaf())
  {
    // is this a simple aggregate (no groupby columns) or a "real" groupby
    if (groupExpr_.isEmpty())
    {
      // if the grouping expression is empty (simple aggregate), add
      // a cardinality constraint for at most one row or exactly one
      // row if there is no HAVING clause that could remove the single
      // output row
      if (selectionPred().isEmpty())
	      getGroupAttr()->addConstraint
	        (new(CmpCommon::statementHeap()) CardConstraint((Cardinality)1,(Cardinality)1));
      else
	      getGroupAttr()->addConstraint
	       (new(CmpCommon::statementHeap()) CardConstraint((Cardinality)0,(Cardinality)1));
    }
    else
    {
      // create a uniqueness constraint for the grouping columns
      ValueIdSet uniqueCols = groupExpr_;

      // remove from the grouping columns any expressions covered
      // by the inputs
      uniqueCols.removeCoveredExprs(getGroupAttr()->getCharacteristicInputs());
      if (uniqueCols.isEmpty())
        {
          // If nothing left then at most a single row will be returned
	  getGroupAttr()->addConstraint(new(CmpCommon::statementHeap())
             CardConstraint((Cardinality)0,(Cardinality)1));
        }
      else
        {
	  getGroupAttr()->addConstraint(new(CmpCommon::statementHeap())
             UniqueOptConstraint(uniqueCols));
          // if grpby child has cardinality constraint, create one for grpby
          Cardinality minrows, maxrows;
          if (child(0).getGroupAttr()->hasCardConstraint(minrows, maxrows)) {
            getGroupAttr()->addConstraint
              (new(CmpCommon::statementHeap()) 
               CardConstraint((Cardinality)1, // grpby cannot go below 1 grp
                              maxrows));
          }
        }
    getGroupAttr()->addSuitableRefOptConstraints
      (child(0).getGroupAttr()->getConstraints());
    processCompRefOptConstraints(normWAPtr);
    }
  }
} // GroupByAgg::synthLogProp()

// -----------------------------------------------------------------------
// member functions for class RelRoot
// -----------------------------------------------------------------------
void
RelRoot::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp)) return;

  // Synthesize estimated logical properties from my child's.
  CostScalar childMaxCardEst = 
    child(0).outputLogProp(inputEstLogProp)->getMaxCardEst();
  EstLogPropSharedPtr myEstLogProp =
    synthEstLogPropForUnaryLeafOp(inputEstLogProp,
	   child(0).outputLogProp(inputEstLogProp)->getColStats(),
           child(0).outputLogProp(inputEstLogProp)->getResultCardinality(),
                                  &childMaxCardEst);

  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstLogProp);

} // RelRoot::synthEstLogProp

void
RelRoot::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  // Synthesize log. properties for me and my children.
  RelExpr::synthLogProp(normWAPtr);

  // just copy the constraints from my child.
  getGroupAttr()->addConstraints(child(0).getGroupAttr()->getConstraints());

} // RelRoot::synthLogProp()


// -----------------------------------------------------------------------
// member functions for class Filter
// -----------------------------------------------------------------------

void
Filter::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  // We will now get the Estimated Logical Properties of the child
  // also multipleCalls will tell us whether or not such
  // child gets called only once.
  Int32 multipleCalls;
  EstLogPropSharedPtr modInputLP;

  const CostScalar orgRowcount = inputEstLogProp->getResultCardinality();
  if (orgRowcount > 1)
  {
     modInputLP =
         child(0).getGroupAttr()->materializeInputLogProp(inputEstLogProp, &multipleCalls);
     const CostScalar newRowcount = modInputLP->getResultCardinality();
     if (orgRowcount == newRowcount)
       modInputLP = inputEstLogProp;
  }
  else
     modInputLP = inputEstLogProp;

  if ( getGroupAttr()->isPropSynthesized(modInputLP) &&
       getGroupAttr()->isPropSynthesized(inputEstLogProp) )
    {
      return ; // already done this
    }

  // Synthesize the intermediate logical properties
  EstLogPropSharedPtr myEstLogProp =
              synthEstLogPropForUnaryLeafOp (inputEstLogProp,
	                 child(0).outputLogProp(inputEstLogProp)->getColStats(),
	                 child(0).outputLogProp(inputEstLogProp)->getResultCardinality());


  // Synthesize estimated logical properties from my child's, taking
  EstLogPropSharedPtr intermedEstLogProp =
              synthEstLogPropForUnaryLeafOp (modInputLP,
	                 child(0).outputLogProp(modInputLP)->getColStats(),
	                 child(0).outputLogProp(modInputLP)->getResultCardinality());


  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstLogProp, intermedEstLogProp);

} // Filter::synthEstLogProp

void
Filter::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  // Synthesize log. properties for me and my children
  RelExpr::synthLogProp(normWAPtr);

  GroupAttributes &myGA = *getGroupAttr();

  ValueIdSet nonRIConstraints;
  for (ValueId x= child(0).getGroupAttr()->getConstraints().init();
       child(0).getGroupAttr()->getConstraints().next(x);
       child(0).getGroupAttr()->getConstraints().advance(x) )
    {
      if ((x.getItemExpr()->getOperatorType() != ITM_COMP_REF_OPT_CONSTRAINT) &&
	  (x.getItemExpr()->getOperatorType() != ITM_REF_OPT_CONSTRAINT))
	  nonRIConstraints += x;
    }
  myGA.addConstraints(nonRIConstraints);
  myGA.addSuitableRefOptConstraints
  (child(0).getGroupAttr()->getConstraints());
  processCompRefOptConstraints(normWAPtr);

  // if a cardinality constraint was copied and if it constrains the
  // minimum number of rows, remove that, since the filter predicate
  // may filter out some rows
  Cardinality minRows, maxRows;

  if (myGA.hasCardConstraint(minRows,maxRows) AND minRows > 0)
    {
      const ValueIdSet &myConstraints = myGA.getConstraints();

      for (ValueId x = myConstraints.init();
	   myConstraints.next(x);
	   myConstraints.advance(x))
	{
	  ItemExpr *xx = x.getItemExpr();
	  OperatorTypeEnum loptype = xx->getOperatorType();

	  if (loptype == ITM_CARD_CONSTRAINT)
	    {
	      CardConstraint *c = (CardConstraint *) xx;
	      if (c->getLowerBound() > 0)
		{
		  // remove this constraint from the group attrs and
		  // add a new one (if needed) with the upper bound only
		  myGA.deleteConstraint(x);
		  if (c->getUpperBound() < INFINITE_CARDINALITY)
		    myGA.addConstraint(new(CmpCommon::statementHeap())
				       CardConstraint((Cardinality)0,
						      c->getUpperBound()));
		}
	    }
	}
    }

} // Filter::synthLogProp()

// -----------------------------------------------------------------------
// member functions for class MapValueIds
// -----------------------------------------------------------------------
void
MapValueIds::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp)) return;

  // Synthesize estimated logical properties from my child's

  const ColStatDescList *colStats =
    &(child(0).outputLogProp(inputEstLogProp)->getColStats());
  CostScalar baseCardinality =
    child(0).outputLogProp(inputEstLogProp)->getResultCardinality();

  if (cseRef_) // could also do this unconditionally
    {
      ColStatDescList *mappedStats = new(CmpCommon::statementHeap())
        ColStatDescList(CmpCommon::statementHeap());
 
      mappedStats->makeMappedDeepCopy(*colStats,
                                      map_,
                                      TRUE);
      colStats = mappedStats;
    }

  EstLogPropSharedPtr myEstLogProp =
    synthEstLogPropForUnaryLeafOp (inputEstLogProp,
                                   *colStats,
                                   baseCardinality);

  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstLogProp);

} // MapValueIds::synthEstLogProp

void
MapValueIds::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  // Synthesize log. properties for me and my children.
  RelExpr::synthLogProp(normWAPtr);

  GroupAttributes &myGA = *getGroupAttr();

  ValueIdSet nonRIConstraints;
  for (ValueId x= child(0).getGroupAttr()->getConstraints().init();
       child(0).getGroupAttr()->getConstraints().next(x);
       child(0).getGroupAttr()->getConstraints().advance(x) )
  {
    if (x.getItemExpr()->getOperatorType() == ITM_UNIQUE_OPT_CONSTRAINT)
    {
      ValueIdSet uniqueCols ;
      getMap().rewriteValueIdSetUp
	(uniqueCols, ((UniqueOptConstraint *)x.getItemExpr())->uniqueCols());
      myGA.addConstraint(new(CmpCommon::statementHeap())
			 UniqueOptConstraint(uniqueCols));
    }
    else if (x.getItemExpr()->getOperatorType() == ITM_CARD_CONSTRAINT)
      myGA.addConstraint(x);
    // func. dependency and check opt constraints can be added here if needed
  }
} //  MapValueIds::synthLogProp()

// -----------------------------------------------------------------------
// member functions for class Scan
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// This constant implements heuristics on how to limit the number of plans
// generated by the file scan rule. Currently we won't consider plans for
// a logical scan node that involve more than 2 joins (3-way joins) of
// indexes.
// After walking through the code we noticed that currently 3-way joins
// never gets into the plan because there is no predicate passed to it.
// As a result the second index gets only primary key columns and has to
// be scanned completely. In this case going to the base table after the
// first index(2 way join) will always be cheaper than the same job plus
// full scan of the second index (3 way join). So we decided to save time
// and eliminate 3 way join by setting MAX_NUM_INDEX_JOIN = 1 instead of
// 2. So, a logical scan node will have no more than 1 index join.
// -----------------------------------------------------------------------
const Lng32 Scan::MAX_NUM_INDEX_JOINS = 1;

void
Scan::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp))
    return;

  // synthesize my estimated logical properties from the initial
  // table statistics or the stats of a common subexpression
  const ColStatDescList *colStats =
    &getTableDesc()->getTableColStats();
  CostScalar baseCardinality = getBaseCardinality();

  if (commonSubExpr_)
    {
      ValueIdList tempTableCols;
      ValueIdList tempTableVEGCols;
      ValueIdList cseCols;
      CSEInfo *info =
        CmpCommon::statement()->getCSEInfo(commonSubExpr_->getName());
      // The original tree is the child of the CommonSubExprRef that
      // did the analysis of whether to use CSEs
      CommonSubExprRef *analyzingCSERef =
        info->getConsumer(info->getIdOfAnalyzingConsumer());

      // this makes the ValueIdList of only those CSE columns that are
      // actually used in the temp table
      commonSubExpr_->makeValueIdListFromBitVector(
           cseCols,
           analyzingCSERef->getColumnList(),
           info->getNeededColumns());

      getTableDesc()->getUserColumnList(tempTableCols);
      getTableDesc()->getEquivVEGCols(tempTableCols, tempTableVEGCols);
      // make a ValueIdMap from the original tree for the common
      // subexpression (bottom) to the columns of our temp table (top)
      ValueIdMap cseToTempScanMap(tempTableVEGCols,
                                  cseCols);

      // get to the ColStatDesc of the common subexpression
      const ColStatDescList &origCSEColStats(
           analyzingCSERef->getEstLogProps()->colStats());

      ColStatDescList *mappedStats = new(CmpCommon::statementHeap())
        ColStatDescList(CmpCommon::statementHeap());

      // now translate the col stats of the original CSE to the temp table
      mappedStats->makeMappedDeepCopy(origCSEColStats,
                                      cseToTempScanMap,
                                      FALSE);

      colStats = mappedStats;
      baseCardinality = analyzingCSERef->getEstLogProps()->getResultCardinality();
    }

  EstLogPropSharedPtr myEstLogProp =
    synthEstLogPropForUnaryLeafOp(inputEstLogProp,
                                  *colStats,
                                  baseCardinality);

  QueryAnalysis *qa = QueryAnalysis::Instance();

  if ( (CmpCommon::getDefault(COMP_BOOL_42) == DF_ON) &&
       (qa && qa->isCompressedHistsViable()) )
  {
    // compress histograms to a single interval histogram
    ColStatDescList &myColStatsList = myEstLogProp->colStats();
    myColStatsList.compressColStatsToSingleInt();
    if(!myColStatsList.isEmpty())
      myEstLogProp->setResultCardinality(myColStatsList[0]->getColStats()->getRowcount());
  }

  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstLogProp);

} // Scan::synthEstLogProp

// Check the rowcount obtained from histogram manipulation with the
// rowcount obtained after executing the query with same set of predicates
// on a sample table. Using a confidence of 95% we have already computed
// a range within which the cardinality should lie within
NABoolean 
Scan::checkForCardRange(const ValueIdSet & setOfPredicates,
                        CostScalar & newRowCount /* in and out */)
{
  if (!CURRSTMT_OPTDEFAULTS->histUseSampleForCardEst())
  return FALSE;
  NABoolean cardChange = FALSE;

   TableDesc * td = getTableDesc();

   if (td == NULL)
     return cardChange;

   if (td->getTableAnalysis() == NULL)
     return cardChange;

  // Since the selection predicates may contain equality predicates with
  // columns from other table, get the actual local predicates from 
  // table analysis, and see if all those were applied or not. Replace any
  // RangeSpecRefs with their source predicates so they are the ones
  // used in the intersect operation.
  const ValueIdSet& localPreds = td->getTableAnalysis()->getLocalPreds();
  ValueIdSet derangedLocalPreds = localPreds.replaceRangeSpecRefs();
  ValueIdSet derangedSetOfPreds = setOfPredicates.replaceRangeSpecRefs();
  derangedLocalPreds = derangedSetOfPreds.intersect(derangedLocalPreds);

  // Now we have only the local predicates for this Scan node
  if (derangedLocalPreds.entries() == 0) 
    return cardChange;
  
  // To take into account indexes and TSJs, see if the predicates
  // for which the estimates were done is a super set or a subset
  // of the set of predicates for which the CTS was executed
  ValueIdSet predsExecuted = td->predsExecuted();
  ValueIdSet commonPreds = derangedLocalPreds.intersect(predsExecuted);

  // nothing in common, return taking cardinality estimates from histograms as correct
  if (commonPreds.entries() == 0)
    return cardChange;

  // if local preds are identical to the preds that were executed
  // cardinalities should be within the range frm CTS
  if (derangedLocalPreds == predsExecuted)
  {
    // since the set of predicates are identical, we want to ensure
    // the rowcount computed from the histogram lies within the error
    // range. 
      newRowCount = MAXOF(newRowCount, getTableDesc()->getMinRC());
      newRowCount = MINOF(newRowCount, getTableDesc()->getMaxRC());
      cardChange = TRUE;
  }
  else
    if ((commonPreds.entries() < predsExecuted.entries()) &&
        (commonPreds.entries() == derangedLocalPreds.entries()))
    {
      // It could be a case of an index which does not cover all predicates
      // Basically the local predicates is a subset of predicates for which
      // FetchCount was executed.
      // Example, index is on columns (a, b) while the predicates for which
      // FetchCount was used was (a, b, c). In this case the cardinality estimates
      // from histogram cannot be more than what we got from FetchCount
      newRowCount = MAXOF(newRowCount, getTableDesc()->getMinRC());
      cardChange = TRUE;
    }
    else
     if ((commonPreds.entries() == predsExecuted.entries()) &&
        (commonPreds.entries() < derangedLocalPreds.entries()))
     {
       // case where local predicates is a super set of predicates for which
       // FetchCount was executed. In this case the estimates from histogram cannot
       // be less than the estimates that were computed from the FetchCount
       newRowCount = MINOF(newRowCount, getTableDesc()->getMaxRC());
       cardChange = TRUE;
     }
     return cardChange;
}

void
Scan::synthLogProp(NormWA * normWAPtr)
{
  // check to see this GA has already been associated with
  // a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  ValueIdList tempClusteringKey;
  ValueIdSet clusteringKey;

  // get the VEGReferences of the clustering key into a ValueIdSet
  tabId_->getEquivVEGCols(tabId_->getClusteringIndex()->getIndexKey(),
			  tempClusteringKey);
  clusteringKey = tempClusteringKey;


  // Synthesize log. properties for me and my children.
  RelExpr::synthLogProp(normWAPtr);

  // this node reads one base table
  getGroupAttr()->setNumBaseTables(1);

  // create uniqueness constraints (for now, only the primary key)
  const LIST(IndexDesc *) & ixlist = tabId_->getIndexes();

  NABoolean addCardConstraint = TRUE;

  // check for empty scans
  for (ValueId p=getSelectionPred().init();
       getSelectionPred().next(p);
       getSelectionPred().advance(p))
    {
      NABoolean negateIt = FALSE;
      ConstValue *cv = p.getItemExpr()->castToConstValue(negateIt);

      if (cv && !negateIt && cv->isAFalseConstant())
        {
          // this scan doesn't produce any outputs at all
          getGroupAttr()->addConstraint(
               new(CmpCommon::statementHeap())
                 CardConstraint((Cardinality)0,(Cardinality)0));
          addCardConstraint = FALSE;
        }
    }

  for (CollIndex indexNo = 0; indexNo < ixlist.entries(); indexNo++)
    {
      IndexDesc *idesc = ixlist[indexNo];

      // Determining uniqueness constraints from indexes is not quite
      // straightforward, since no information is available whether
      // this index was created as a "unique" index. The method used
      // here assumes that the index key is always unique (currently
      // and probably for the future a DP2 requirement). Since we don't
      // want to add a lot of unnecessary uniqueness constraints, eliminate
      // all those who contain the clustering key (except for the clustering
      // index itself, of course).

      const ValueIdList &indexKey(idesc->getIndexKey());
      ValueIdList tempUniqueCols;
      ValueIdSet uniqueCols;

      // get the VEGReferences of the key columns into a ValueIdSet
      tabId_->getEquivVEGCols(indexKey,
			      tempUniqueCols);
      uniqueCols = tempUniqueCols;

      if (idesc->isClusteringIndex() OR
          (NOT uniqueCols.contains(clusteringKey)))
	{
          // Remove from the unique key any values that are covered
          // by the inputs. If the key ends up empty then add a
          // cardinality constraint.
          uniqueCols.removeCoveredExprs(getGroupAttr()->getCharacteristicInputs());

          // Remove any columns that are computed from the remaining
          // unique columns
          for (ValueId cc=uniqueCols.init();
               uniqueCols.next(cc);
               uniqueCols.advance(cc))
            {
              BaseColumn *bc = cc.castToBaseColumn();
              ValueId computedExpr(bc->getComputedColumnExpr());

              if (computedExpr != NULL_VALUE_ID)
                {
                  // Check whether the underlying base columns
                  // are part of the unique key. If so, then
                  // this computed column can be computed from
                  // the other columns and therefore is not
                  // part of the minimal unique key
                  ValueIdSet underlyingCols;
                  ValueIdSet underlyingVEGCols;

                  bc->getUnderlyingColumnsForCC(underlyingCols);
                  tabId_->getEquivVEGCols(underlyingCols,
                                          underlyingVEGCols);

                  if (uniqueCols.contains(underlyingVEGCols))
                    uniqueCols -= cc;
                }
            }

          if (uniqueCols.isEmpty())
            {
              if (addCardConstraint)
	        getGroupAttr()->addConstraint(new(CmpCommon::statementHeap())
                   CardConstraint((Cardinality)0,(Cardinality)1));
              addCardConstraint = FALSE; // do it once
            }
          else
	    getGroupAttr()->addConstraint(new(CmpCommon::statementHeap())
               UniqueOptConstraint(uniqueCols));
	}
    }

    // we don't need to add RefOptConstraints during analyze and optimizer phases
    // the normWAPtr check serves that purpose.
    if (normWAPtr && 
	(CmpCommon::getDefault(ELIMINATE_REDUNDANT_JOINS) != DF_OFF))
    {
	RefConstraint * riConstraint = NULL ;
	ValueIdList keyColVEGVids ;
	ValueIdSet keyColVEGVidsSet ;

	const ValueIdSet & myOutputs = getGroupAttr()->getCharacteristicOutputs();
     
	const AbstractRIConstraintList RIConstraintsList = 
	  getTableDesc()->getNATable()->getRefConstraints() ;
	for (CollIndex i = 0; i < RIConstraintsList.entries(); i++)
	{
	  riConstraint = (RefConstraint *) RIConstraintsList.at(i);
	  if ((riConstraint->getIsEnforced()) ||
		  (CmpCommon::getDefault(ELIMINATE_REDUNDANT_JOINS) != DF_MINIMUM))
	  {
	    keyColVEGVids.clear();
	    keyColVEGVidsSet.clear();
	    const Constraint::KeyColumns& keyCols = riConstraint->keyColumns();
	    for (CollIndex j = 0; j < keyCols.entries(); j++)
	    {
	      keyColVEGVids.insert(tabId_->getColumnVEGList()[keyCols.at(j)->getPosition()]);
	    }
	    keyColVEGVidsSet = keyColVEGVids ;

	    // If my outputs include the foreign key columns then there may be a
	    // matching RI join further up the query tree. This is a necessary but
	    // not a sufficient condition.
	    if (myOutputs.contains(keyColVEGVidsSet))
	    {
	      getGroupAttr()->addConstraint(new(CmpCommon::statementHeap())
		    RefOptConstraint(keyColVEGVids,
		    riConstraint->getUniqueConstraintName()));
	      // this flag is used to quickly determine if we have RefOpt constraints later.
	      getGroupAttr()->setHasRefOptConstraint(TRUE);
	    }
	  }
	}
      
      // synthesize CompRefOpt constraints now.
      processCompRefOptConstraints(normWAPtr);
    }

  // For debugging purposes only:
  //   Provides a way of faking the estimated rowcount for a table
  //   by using a funny correlation name "ROWSnnnn..." where nnnn... stands for
  //   the number of rows.
#ifndef NDEBUG
  NAString cname(getTableDesc()->getCorrNameObj().getCorrNameAsString(),
                 CmpCommon::statementHeap());

  if ((cname.length() > 3) && (cname(0,4) == NAString("ROWS")))
    setBaseCardinality((Cardinality)NAStringToReal(cname(4,cname.length()-4)));
  else
#endif
    setBaseCardinality (MIN_ONE (getTableDesc()->getNATable()->getEstRowCount())) ;

  if ((CURRSTMT_OPTDEFAULTS->histDefaultSampleSize() > 0) &&
      (RelExpr *)this->getRETDesc() &&
      (RelExpr *)this->getRETDesc()->getBindWA() &&
      (RelExpr *)this->getRETDesc()->getBindWA()->inDDL())
  {
    CURRSTMT_OPTDEFAULTS->setHistDefaultSampleSize(0);
  }

  // For each set of column statistics, convert the base column valueids
  // to the equivalent VEG cols if not done already.
  ColStatDescList & initialStats = getTableDesc()->tableColStats();
  for (CollIndex i = 0; i < initialStats.entries(); i++)
    initialStats[i]->VEGColumn() =
      getTableDesc()->getEquivVEGCol(initialStats[i]->getColumn());

  // set if this scan node is trafodion SMD table
  if ( getTableDesc()->getNATable()->isSeabaseMDTable() )
    getGroupAttr()->setSeabaseMDTable(TRUE);

} // Scan::synthLogProp()

void Scan::processCompRefOptConstraints(NormWA * normWAPtr)
{
  if (normWAPtr && 
	(CmpCommon::getDefault(ELIMINATE_REDUNDANT_JOINS) != DF_OFF) &&
	  getGroupAttr()->getCharacteristicInputs().isEmpty())
  {
      UniqueConstraint * uniqConstraint = NULL;
      ValueIdList keyColVEGVids ;
      ValueIdSet keyColVEGVidsSet;
      const ValueIdSet & myOutputs = getGroupAttr()->getCharacteristicOutputs();
      const ValueIdSet& myEssOutputs = 
	getGroupAttr()->getEssentialCharacteristicOutputs();

      const AbstractRIConstraintList UniqConstraintsList = 
	getTableDesc()->getNATable()->getUniqueConstraints() ;

      for (CollIndex i = 0; i < UniqConstraintsList.entries(); i++)
      {
	keyColVEGVids.clear();
	keyColVEGVidsSet.clear();
	uniqConstraint = (UniqueConstraint *)UniqConstraintsList.at(i);
	const Constraint::KeyColumns& keyCols = uniqConstraint->keyColumns();
	for (CollIndex j = 0; j < keyCols.entries(); j++)
	{
	  keyColVEGVids.insert
	    (getTableDesc()->getColumnVEGList()[keyCols.at(j)->getPosition()]);
	}
	keyColVEGVidsSet = keyColVEGVids;

	// The three necessary conditions that determine if this UniqueConstraint
	// can provide a match for a RefOpt constraint in some join up above is
	// (a) my outputs include the unique key columns
	// (b) my essential outputs do not include anything other than the unique key cols
	// (c) there is at least one Referential constraint defined on me
	// (d) this node does not contain a reducing predicate of the form
	//     table.col = 1 or table.col1 = table.col2
	if ((myOutputs.contains(keyColVEGVidsSet)) &&
	    (keyColVEGVidsSet.contains(myEssOutputs))&&
	    (uniqConstraint->hasRefConstraintsReferencingMe())&&
	    (NOT containsReducingPredicates(keyColVEGVidsSet)))
	{
	  getGroupAttr()->addConstraint(new(CmpCommon::statementHeap())
		ComplementaryRefOptConstraint(keyColVEGVids,
		uniqConstraint->getConstraintName(), this, getTableDesc()));
	  getGroupAttr()->setHasCompRefOptConstraint(TRUE);
	}
      }
  }
}

NABoolean Scan::containsReducingPredicates(const ValueIdSet& keyCols) const
{
  const ValueIdSet& selPreds = getSelectionPredicates(); 
  for (ValueId x = selPreds.init();
       selPreds.next(x);
       selPreds.advance(x))
  {
    ValueId childId = NULL_VALUE_ID;
    ItemExpr *ie = x.getItemExpr();

    // If the ItemExpr is a range spec operator, sub in the actual predicate,
    // which is its right subtree.
    if (ie->getOperatorType() == ITM_RANGE_SPEC_FUNC)
      ie = ie->child(1);

    if (ie->getOperatorType() == ITM_VEG_PREDICATE)
    {
      VEG* pred = ((VEGPredicate*)ie)->getVEG();
      // catches cases like T1.a = 1, T1.a = ? or T1.a = hostvar or (T1.a = T2.a and T2.a = 1)
      // Here "a" is not a primary key column of T1. If it is then we don't consider
      // T1.a = 1 a reducing predicate, because the if there is a FK-PK relation, the 
      // T1.a =1 relation will be applied on the FK table too.
      if ((pred->getCountOfUserSuppliedInputs() > 0) &&
          (NOT(pred->getAllValues().referencesOneValueFromTheSet(keyCols))))
        return TRUE;
    }
    else if (ie->isARangePredicate())
    {
      if (ie->child(0)->doesExprEvaluateToConstant(FALSE) &&
          ie->child(1)->getOperatorType() == ITM_VEG_REFERENCE) {
        childId = (ie->child(1).getPtr())->getValueId();
      }
      else if (ie->child(1)->doesExprEvaluateToConstant(FALSE) &&
               ie->child(0)->getOperatorType() == ITM_VEG_REFERENCE) {
        childId = (ie->child(0).getPtr())->getValueId();
      }
      // catches cases like T1.a > 1, T1.a < ? or T1.a > hostvar
      // where T1.a is not a key column.
      // Also catches t1.a > t1.b or T1.key < T1.a
      if (NOT(keyCols.contains(childId)))
        return TRUE;
    }
    else
      return TRUE; // something unexpected, so assume the worst
  }

  // This will catch preds like T1.a = T1.b, (T1.a = T2.b and T2.b = T1.c)
  // When we have two columns of T1 that are related by an equality pred either
  // directly or indirectly, the VEGColList has the same VEG for pth columns
  // and we use this to detect such predicates here.
  const TableDesc * tdesc = ((Scan *)this)->getTableDesc();
  ULng32 numCols = tdesc->getColumnVEGList().entries();
  const ValueIdSet colVEGSet(tdesc->getColumnVEGList());
  if (colVEGSet.entries() < numCols)
    return TRUE;

  return FALSE; // no reducing predicate detected
}

// -------------------------------------------------------------------------
// This method would set the basic cardinality and selecttivity information
// in the group attributes. This includes cardinality after aplying all local
// predicates, selectivity and cardinality hints if any
// -------------------------------------------------------------------------

void
Scan::finishSynthEstLogProp()
{
  ExtendedQualName::SpecialTableType specialType = getTableName().getSpecialType();

  // minimum row count of this group is the row count after applying local predicates
  // Set this only if it is not a triggers table

  if( specialType != ExtendedQualName::TRIGTEMP_TABLE)
  {
    TableDesc * tableDesc = getTableDescForBaseSelectivity();

    if (specialType == ExtendedQualName::NORMAL_TABLE)
    {
      CardinalityHint * cardHint = tableDesc->cardinalityHint();
      SelectivityHint * selHint = tableDesc->selectivityHint();

      if ((cardHint || selHint) && !suppressHints_)
      {
		if ((cardHint && ( cardHint->getBaseScanSelectivityFactor() == -1.0 ) ) ||
			(selHint && ( selHint->getBaseScanSelectivityFactor() == -1.0 ) ) )
		{
			// baseScanSelectivityFactor should be set only once for Scan node
			// for corresponding indexes, it would be picked up from the tableDesc
			// of Scan. Compute cardinality for emptyInputLogProp using no hints
			// also, do not cache these logical properties in any of the cache, as
			// these are not what the user wants. He wants the cardinality based
			// on the hint he has provided.

			CostScalar baseSelectivity = computeBaseSelectivity();

			if (cardHint)
			{
				tableDesc->setBaseSelectivityHintForScan(cardHint, baseSelectivity);
			}

			if (selHint)
			{
				tableDesc->setBaseSelectivityHintForScan(selHint, baseSelectivity);
			}
			setTableDesc(tableDesc);
		} // baseScanSelectivityFactor == -1.0
      } // cardHint || selHint
      // In both the cases of Hint and FetchCount, synchronize histograms with ColStats and 
    } // specialType == NORMAL_TABLE
    CostScalar minimumRowcount = getGroupAttr()->getResultCardinalityForEmptyInput();
    getGroupAttr()->setMinChildEstRowCount(minimumRowcount);
  } // specialType != TRIGTEMP_TABLE
} // Scan::finishSynthEstLogProp()


// -----------------------------------------------------------------------
// methods for class Tuple
// -----------------------------------------------------------------------
void
Tuple::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp)) return;

  // create a default OutputLogProp with cardinality of 1
  EstLogPropSharedPtr myEstLogProp(new (HISTHEAP) EstLogProp(1));

  // create a histogram for each tuple, such that the row count and
  // uec for each histogram is one. Reason for that is for cases
  // like insert, these tuples get mapped to the columns in which
  // the value is being inserted. These columns can then appear in the
  // characteristic outputs. This means that we should have colStats for
  // those columns. But since we were not generating colStats for the
  // constants, there are no colStats generated for the columns to which
  // these constants are being mapped.

  ColStatDescList & outputColStats = myEstLogProp->colStats();

  ValueIdList & tupleList = tupleExpr();

  CollIndex nTupleListEntries = (CollIndex)tupleList.entries();
  for (CollIndex i = 0; i < nTupleListEntries ; i++)
  {
    ValueId ituple = tupleList[i];
    outputColStats.addColStatDescForVirtualCol(1,
					      1,
					      ituple,
					      ituple,
					      ituple,
                                              NULL
                                              );
  }

  getGroupAttr()->addInputOutputLogProp(inputEstLogProp, myEstLogProp);
} // Tuple::synthEstLogProp

void
Tuple::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  // Synthesize log. properties for me and my children.
  RelExpr::synthLogProp(normWAPtr);

  // create the cardinality constraint
  if (selectionPred().isEmpty())
    getGroupAttr()->addConstraint(new(CmpCommon::statementHeap())
				  CardConstraint((Cardinality)1,(Cardinality)1));
  else
    getGroupAttr()->addConstraint(new(CmpCommon::statementHeap())
				  CardConstraint((Cardinality)0,(Cardinality)1));

} // Tuple::synthLogProp()


// -----------------------------------------------------------------------
// methods for class TupleList
// -----------------------------------------------------------------------
void
TupleList::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp)) return;

  CostScalar numberOfTuples = numberOfTuples_;
  numberOfTuples.minCsZero();

  if (CmpCommon::getDefault(COMP_BOOL_76) == DF_ON)
    // Take into account the number of probes coming from the outer
    numberOfTuples *= inputEstLogProp->getResultCardinality();

  // create a OutputLogProp with cardinality of numberOfTuples
  EstLogPropSharedPtr myEstLogProp(new (HISTHEAP) EstLogProp(numberOfTuples));

  // create a histogram for each tuple, such that the row count and
  // uec for each histogram is one. Reason for that is for cases
  // like insert, these tuples get mapped to the columns in which
  // the value is being inserted. These columns can then appear in the
  // characteristic outputs. This means that we should have colStats for
  // those columns. But since we were not generating colStats for the
  // constants, there are no colStats generated for the columns to which
  // these constants are being mapped.

  ColStatDescList & outputColStats = myEstLogProp->colStats();

  ValueIdList  & tupleList = tupleExpr();

  CollIndex nTupleListEntries = (CollIndex)tupleList.entries();
  for (CollIndex i = 0; i < nTupleListEntries ; i++)
  {
    ValueId ituple = tupleList[i];

    NABoolean defineVirtual = TRUE;
    if (CmpCommon::getDefault(COMP_BOOL_48) == DF_ON)
    {
      ItemExpr * tupleExpr = ituple.getItemExpr();
      tupleExpr = tupleExpr->getLeafValueIfUseStats();
      ituple = tupleExpr->getValueId();
      if (createdForInList())
        defineVirtual = FALSE;
    }

    outputColStats.addColStatDescForVirtualCol(numberOfTuples,
					      numberOfTuples,
					      ituple,
					      ituple,
					      ituple,
                                              this,
                                              defineVirtual);
  }

  const ColStatDescList &outerColStatsList = inputEstLogProp->getColStats();
  CollIndex outerRefCount = outerColStatsList.entries();
  if (outerRefCount > 0)
  {
    CostScalar innerScale = numberOfTuples_;
    innerScale.minCsOne();

    outputColStats.prependDeepCopy (outerColStatsList, // source ColStatDescList
				 outerRefCount,     // prepend limit
				 innerScale,        // scale prepended Rowcounts
				 FALSE) ;           // clear shapeChanged flag
  }

  getGroupAttr()->addInputOutputLogProp(inputEstLogProp, myEstLogProp);
} // TupleList::synthEstLogProp

void
TupleList::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  // Synthesize log. properties for me and my children.
  RelExpr::synthLogProp(normWAPtr);

  // create the cardinality constraint
  if (selectionPred().isEmpty())
    getGroupAttr()->addConstraint(
	 new(CmpCommon::statementHeap()) CardConstraint(
	      (Cardinality) numberOfTuples_,
	      (Cardinality) numberOfTuples_));
  else
    getGroupAttr()->addConstraint(
	 new(CmpCommon::statementHeap()) CardConstraint(
	      (Cardinality) 0,
	      (Cardinality) numberOfTuples_));

} // TupleList::synthLogProp()


// -----------------------------------------------------------------------
// member functions for class GenericUpdate
// -----------------------------------------------------------------------
void
GenericUpdate::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp)) return;

  // Synthesize my estimated logical properties from either the table
  // statistics if this is a leaf GenericUpdate, or from my child's
  // statistics if this is a unary GenericUpdate operator.
  EstLogPropSharedPtr myEstLogProp;
  if (getArity() == 0)
  {
    myEstLogProp =
      // for the third parameter we use the default value (1) for the
      // initial rowcount
      synthEstLogPropForUnaryLeafOp (inputEstLogProp, getTableDesc()->getTableColStats());
  }
  else
  {
    EstLogPropSharedPtr childOutputLogProp = child(0).outputLogProp(inputEstLogProp);
    myEstLogProp =
      synthEstLogPropForUnaryLeafOp (inputEstLogProp,
		     childOutputLogProp->getColStats(),
		     childOutputLogProp->getResultCardinality());
  }

  getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstLogProp);
}

void
GenericUpdate::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis())
    return;

  Int32 numBaseTables;

  // synthesize logical properties for me and my child (if any).
  RelExpr::synthLogProp(normWAPtr);

  if (getArity() == 0)
  {
    // For each set of column statistics, convert the base column valueids
    // to the equivalent VEG cols.
    ColStatDescList & initialStats = getTableDesc()->tableColStats();
    for (CollIndex i = 0; i < initialStats.entries(); i++)
      initialStats[i]->VEGColumn() =
        getTableDesc()->getEquivVEGCol(initialStats[i]->getColumn());

    numBaseTables = 1; // a leaf update counts as one base table
  }
  else
  {
    // For an update with children we don't count our own table again,
    // since there is already a scan node for that table below us.
    // This makes the counting independent on whether a subset or a
    // cursor update is being used.
    numBaseTables = child(0).getGroupAttr()->getNumBaseTables();
  }

  getGroupAttr()->setNumBaseTables(numBaseTables);
} // GenericUpdate::synthLogProp()

void
GenericUpdate::finishSynthEstLogProp()
{
  CostScalar minRowCount = COSTSCALAR_MAX;
  NABoolean triggersTempTable = \
    (getTableName().getSpecialType() == ExtendedQualName::TRIGTEMP_TABLE) ?\
    TRUE : FALSE;

  if (triggersTempTable)
  {
	if (getArity() == 0)
	{
		minRowCount = getGroupAttr()->outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->\
				getResultCardinality();
	}
	else
	{
		minRowCount = child(0).getGroupAttr()->getMinChildEstRowCount();
	}
  }
   getGroupAttr()->setMinChildEstRowCount(minRowCount);
} // GenericUpdate::finishSynthEstLogProp

// -----------------------------------------------------------------------
// methods for class ExplainFunc
// -----------------------------------------------------------------------
void
ExplainFunc::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp))
    return;

  // Create a new Output Log Property with cardinality of 10 for now.
  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(10));

  getGroupAttr()->addInputOutputLogProp(inputEstLogProp, myEstProps);

} // ExplainFunc::synthEstLogProp

void
ExplainFunc::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  //  for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  RelExpr::synthLogProp(normWAPtr);

} // ExplainFunc::synthLogProp()

// -----------------------------------------------------------------------
// methods for class Transpose
// -----------------------------------------------------------------------

// Transpose::synthEstLogProp() ------------------------------------------
// synthesize estimated logical properties given a specific set of
// input log. properties.
//
// Parameters:
//
// const EstLogPropSharedPtr& inputEstLogProp
//    IN : A set of input logical properties used to estimate the logical
//         properities of this node.
//
// For Example:
//
//           SELECT *
//             FROM Table
//        TRANSPOSE A,B AS C1
//                  X,Y,Z as C2
//                  (1,'hello'),(2,'world') AS (C3, C4)
//           KEY BY K1
//
// Terminology:
// transUnionVector_:This is a vector of ValueIdLists. There is one entry
//   for each transpose set, plus one entry for the key values. Each entry
//   contains a list of ValueIdUnion Nodes. The first entry contains a list
//   with one ValueIdUnion node. This node is for the Const. Values (1 - N)
//   representing the Key Values. The other entries contain lists of
//   ValueIdUnion nodes for the Transposed Values. Each of these entries of
//   the vector represent a transpose set.  If the transpose set contains a
//   list of values, then there will be only one ValueIdUnion node in the
//   list.  If the transpose set contains a list of lists of values, then
//   there will be as many ValueIdUnion nodes as there are items in the
//   sublists. (see example below.)
//
//               TRANSPOSE
//                 transUnionVectorSize_: 4
//                 transUnionVector_:
//                     ValueIdUnion(1,2,3,4,5,6,7)
//                     ValueIdUnion(A,B)
//                     ValueIdUnion(X,Y,Z)
//                     ValueIdUnion(1,2) , ValueIdUnion('hello','world')
//
// Cardinality of Transpose - is the cardinality of the child * transpose expressions.
// In the above example, there are 4 transpose expressions. So the cardinality will be
// Cardinality of the child * 4
//
// UEC of Transpose : is the sum of UEC of columns comprising of the transposed column. There
// is also one added for each constant. Hence
// UEC of C1 = UEC of A + UEC of B
// UEC of C2 = UEC of X + UEC of Y + UEC of Z
// UEC of C3 = 2
// UEC of C4 = 2
//

#pragma nowarn(770)   // warning elimination
void
Transpose::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp) == TRUE)
    return;

  // Create a new Output Log Property
  //
  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp());

  // Get the estimated logical properties of the child. To be used
  // to estimate the logical properties of this node.
  //
  EstLogPropSharedPtr childEstProp = child(0).outputLogProp(inputEstLogProp);

  // Get the current column stats.
  //
  const ColStatDescList &outerColStatsList = inputEstLogProp->getColStats();

  const ColStatDescList &childColStatsList = childEstProp->getColStats();

  // Result Column Stats. Based on childs col stats list
  // since result of transpose can contain columns from child.
  //
  ColStatDescList transStatDescList(childColStatsList,CmpCommon::statementHeap());

  // Composite Column Stats. for all transposed columns and
  // the key column if it exists.  This is because there is
  // a corelation between the key column and the transposed
  // columns.  If the key column does not exist, no composite
  // stats will be generated.
  //
  ColStatsSharedPtr compColStats;
  ColStatDescSharedPtr compColStatDesc;

  // Does the key column exist?...
  //
  if(transUnionVector()[0].entries() > 0)
  {
    // The key values exist.
    // Add col stats for this generated column,
    // and the composite column stats for the key
    // column and the transposed columns.
    //
    ValueIdUnion *keyValIdUnion =
      (ValueIdUnion *)(transUnionVector()[0])[0].getValueDesc()->getItemExpr();

    CostScalar numKeyVals = keyValIdUnion->entries();
    CostScalar rowCount = childEstProp->getResultCardinality() * numKeyVals;

    // Create a histogram for the Key column.
    // Empty for now.
    //
    HistogramSharedPtr emptyHist(new (HISTHEAP) Histogram(HISTHEAP));

    // Create the ColStats for the key column.
    //
    ComUID id(ColStats::nextFakeHistogramID());
    ColStatsSharedPtr keyStats(
      new (HISTHEAP) ColStats(id,
                              numKeyVals,
                              rowCount,
                              rowCount,
                              FALSE,
                              FALSE,
                              emptyHist,
                              FALSE,
                              1.0,
                              1.0,
                              -1,  //average varchar size
                              HISTHEAP));

    // Setting this flag will ensure that the compiler does not start
    // to look for this column name in the NAColumn. As there does not
    // exist a column for a constant
    keyStats->setVirtualColForHist(TRUE);
    keyStats->setIsCompressed(TRUE);

      // Create a copy of the column stats to be used for the
    // composite column stats.
    //
    compColStats = ColStatsSharedPtr(new (HISTHEAP) ColStats(*keyStats, HISTHEAP));

    // Set the UEC of the composite column stats to 0.  This is
    // because the key column does not affect the total UEC of
    // the composite column.  Later, the UEC's of the transpose columns
    // will be added in.
    //
    compColStats->setRowsAndUec(0, 0);

    ColStatDescSharedPtr keyColStatDesc(new (HISTHEAP) ColStatDesc(keyStats,
                                 (transUnionVector()[0])[0]), HISTHEAP);

    compColStatDesc = ColStatDescSharedPtr(new (HISTHEAP) ColStatDesc(compColStats,
                                 (transUnionVector()[0])[0]), HISTHEAP);

    transStatDescList.insert(keyColStatDesc);

  } // (transUnionVector()[0].entries() > 0)

  Int32 haveAllColStats = 0;

  for(CollIndex vec = 1; vec < transUnionVectorSize(); vec++)
  {
    ValueIdList &valIdList = transUnionVector()[vec];

    for(CollIndex vidu = 0; vidu < valIdList.entries(); vidu++)
    {
      // Keep count of constant members in the Union.
      CostScalar constMembers = 0;

      ValueId firstSourceID;

      ValueIdUnion *valIdUnion =
        (ValueIdUnion *)valIdList[vidu].getValueDesc()->getItemExpr();

      for(CollIndex vid = 0; vid < valIdUnion->entries(); vid++)
      {
        ValueId sourceValId = valIdUnion->getSource((Lng32)vid);

	sourceValId = getSourceColFromExprForUec(sourceValId, childColStatsList);

	CollIndex index;
        if(childColStatsList.
           getColStatDescIndex(index, sourceValId ))
	{
          if (!firstSourceID)
	    firstSourceID = sourceValId;
	}
	else
	  constMembers++;
      } // for (valIdUnion->entries() )

      if (firstSourceID) haveAllColStats++;

      if (!firstSourceID)
      {
	// This column is a union of all constants, so create a fake histogram
	// with UEC equal to number of unique constants in the Union, and the
	// row count equal to the number of rows of the child
	// This fake histogram is used by all the nodes above this to compute
	// cardinalities

	// get rid of all duplicates to get number of unique constants.
	ValueIdSet sourceConstants = valIdUnion->getSources();
	constMembers = sourceConstants.entries();

	CostScalar rowCount = MINOF(constMembers, childEstProp->getResultCardinality() );

	// number of distinct constants form the UEC
	// VEGColumn and the mergeState comprise of the ValueIdUnion for this column

	transStatDescList.addColStatDescForVirtualCol(constMembers,
						      rowCount,
						      valIdUnion->getSource(0),
						      valIdList[vidu],
						      valIdUnion->getValueId(),
                                                      NULL);

      }
      else
      {
        // Have col stats for atleast 1 member.
        CollIndex entry = transStatDescList.entries();

        CollIndex firstEntry = NULL_COLL_INDEX;

        childColStatsList.
          getColStatDescIndex(firstEntry,firstSourceID);

        ColStatDescSharedPtr firstColStatDesc = childColStatsList[firstEntry];

        // transStatDescList.insertAt(entry, firstColStatDesc);

        // for safety, perform a deep copy.....
        ColStatsSharedPtr transColStats =
          firstColStatDesc->getColStatsToModify();
        transColStats->copyAndScaleHistogram( 1 );

	ValueId firstCol = valIdUnion->getSource(0);
	firstCol = getSourceColFromExprForUec(firstCol, childColStatsList);

        ColStatDescSharedPtr transStatDesc(new (HISTHEAP)
                       ColStatDesc (transColStats,firstCol), HISTHEAP);

        transStatDescList.insertAt(entry,transStatDesc);

        CollIndex index;
        for(CollIndex vid = 1; vid < valIdUnion->entries(); vid++)
	{
	  index = NULL_COLL_INDEX;

	  // The column in the Transpose could be within an expression
	  // hence first get all VEG expressions from the source, and then
	  // use that VEG expression to look for the column

	  ValueId sourceColumn = valIdUnion->getSource((Lng32)vid);

	  sourceColumn = getSourceColFromExprForUec(sourceColumn, childColStatsList);

          childColStatsList.
            getColStatDescIndex(index, sourceColumn);

	  if (index == NULL_COLL_INDEX) continue;

          ColStatDescSharedPtr colStatDesc = childColStatsList[index];

	  transStatDescList[entry]->mergeColStatDesc(colStatDesc,
                                                 UNION_MERGE,
                                                 FALSE,
                                                 REL_UNION);
        } // for (ValueIdUnion->entries() )

        // update the result's description of itself
        //  Note that it is proper that this isn't done in the previous
        //  body of code that looks (as is pointed out above), similar to
        //  this code.
        transStatDescList[entry]->VEGColumn() = valIdList[vidu];

        // need to do the following, adding "my" ValueId to the list of
        // columns I've joined to, because in ColStatDesc::mergeColStatDesc()
        // we can get into trouble when we have a mergeState_ that is empty
        // --> we set to empty first (via clear()) because we do not want
        // the left table's mergeState information
        transStatDescList[entry]->mergeState().clear() ;
        transStatDescList[entry]->mergeState().insert(valIdUnion->getValueId());

	// add 1 to the UEC for each constant member
	ColStatsSharedPtr transposedColStat = transStatDescList[entry]->getColStatsToModify();
	CostScalar unionUec = transposedColStat->getTotalUec();
	unionUec += constMembers;

	// Even while setting the UEc we want to be sure that we are maintaining
	// the sanity relationship between UEC and rowcount. That is why the UEC
	// is never set alone

	CostScalar rowCount = transposedColStat->getRowcount();
	transposedColStat->setRowsAndUec(rowCount, unionUec);
      } // (!firstSourceID)
    } // for (valIdList.entries() )
  } // for (transUnionVectorSize() )

  if(haveAllColStats && compColStats)
  {
    // There exists a key column for the transpose
    CostScalar compUEC = 0;

    for(CollIndex vec = 1; vec < transUnionVectorSize(); vec++)
    {

      ValueIdList &valIdList = transUnionVector()[vec];
      ValueIdUnion *firstValIdUnion =
        (ValueIdUnion *)valIdList[0].getValueDesc()->getItemExpr();

      for(CollIndex vid = 0; vid < firstValIdUnion->entries(); vid++)
      {

        CostScalar interUEC = 1;
        for(CollIndex vidu = 0; vidu < valIdList.entries(); vidu++)
	{

          ValueIdUnion *valIdUnion =
            (ValueIdUnion *)valIdList[vidu].getValueDesc()->getItemExpr();

          CollIndex index = NULL_COLL_INDEX;

	  ValueId sourceColumn = valIdUnion->getSource((Lng32)vid);

	  sourceColumn = getSourceColFromExprForUec(sourceColumn, childColStatsList);

          childColStatsList.
            getColStatDescIndex(index, sourceColumn);

          if (index == NULL_COLL_INDEX) continue;

	  ColStatDescSharedPtr colStatDesc = childColStatsList[index];

          interUEC = interUEC * colStatDesc->getColStats()->getTotalUec();

        } // for (valIdList.entries() )

        compUEC = compUEC + interUEC;
      } // for (firstValIdUnion->entries() )

      compColStatDesc->mergeState().clear() ;
      for(CollIndex vidu = 0; vidu < valIdList.entries(); vidu++)
      {
        compColStatDesc->mergeState().insert(valIdList[vidu]);
      }
    } // for (transUnionVectorSize() )

    const CostScalar compRow = compColStats->getRowcount() ;

    compColStats->setRowsAndUec(compRow, compUEC);
    transStatDescList.insert(compColStatDesc);
  } // if(haveAllColStats && compColStats)

  // Estimate the cardinality of this node by multipling the estimated
  // cardinality of the child by the total number of transpose expressions.
  // The total number of transpose expressions is also the number of
  // key values, or the number of entries in the first entry of
  // transUnionVector()
  //
  CollIndex numTransposeExprs = 0;

  // The first entry for the key values, which is optional.
  // Start with the second entry.  The number of key values is
  // the sum of entries of the first valIdUnion of each (1 - N)
  // ValueIdList of the transUnionVector.
  //
  for(CollIndex v = 1 ; v < transUnionVectorSize(); v++)
  {

    ValueIdUnion *valIdUnion =
      (ValueIdUnion *)(transUnionVector()[v])[0].getValueDesc()->getItemExpr();

    numTransposeExprs += valIdUnion->entries();
  }

  // Estimated rowCount.
  //
  CostScalar rowCount =
    childEstProp->getResultCardinality().minCsOne() * numTransposeExprs;

  // Set the cardinality estimate.
  //
  myEstProps->setResultCardinality(rowCount);

  // maxCard(transpose(X)) == card(transpose(X)) * (maxCard(X)/card(X))
  CostScalar maxRows = rowCount * 
    (childEstProp->getMaxCardEst() / 
     childEstProp->getResultCardinality().minCsOne());
  myEstProps->setMaxCardEst(MAXOF(maxRows, rowCount));

  // -------------------------------------------------------------------
  // Lastly, determine which of these histograms are useful based on
  // the characteristic outputs and selection predicates for the group.
  // -------------------------------------------------------------------

  myEstProps->pickOutputs( transStatDescList, inputEstLogProp, getGroupAttr()->getCharacteristicOutputs(), getSelectionPred());

  // -------------------------------------------------------------------
  // VERY IMPORTANT!  Before we register myEstProps as the OLP of the ILP
  // inputEstLogProp, it's vital that every histogram in myEstProps
  // reports the same rowcount.  Otherwise, it's very difficult for
  // resulting code to make any sense of these statistics (in particular,
  // what's the rowcount associated with them?).
  // -------------------------------------------------------------------
  ColStatDescList & columnStats = myEstProps->colStats() ; // nasty access to private member
  columnStats.synchronizeStats (rowCount, columnStats.entries()) ;

  // Set the logical properties of this node.
  //
  getGroupAttr()->addInputOutputLogProp(inputEstLogProp, myEstProps);

} // Transpose::synthEstLogProp
#pragma warn(770)  // warning elimination



// Transpose::synthLogProp ----------------------------------------------
// synthesize logical properties
//
void
Transpose::synthLogProp(NormWA * normWAPtr)
{
  // check to see whether properties are already synthesized.
  if (getGroupAttr()->existsLogExprForSynthesis())
    return;

  // Need to check for cardinality constraints and uniques constraints.
  // !!!

  RelExpr::synthLogProp(normWAPtr);
} // Transpose::synthLogProp()


// -----------------------------------------------------------------------
// methods for class Pack
// -----------------------------------------------------------------------

void
Pack::synthEstLogProp(const EstLogPropSharedPtr& inEstLogProp)
{
  if(getGroupAttr()->isPropSynthesized(inEstLogProp) == TRUE) return;

  EstLogPropSharedPtr myEstProp(new (HISTHEAP) EstLogProp());

  // ---------------------------------------------------------------------
  // $$$ No more column statistics will be available above the Pack node.
  // $$$ just set the estimate cardinality.
  // ---------------------------------------------------------------------
  EstLogPropSharedPtr childEstProp = child(0).outputLogProp(inEstLogProp);

  // Just to make sure division by zero doesn't happen in prototype code.
  ULng32 pf = MIN_ONE (packingFactorLong()) ;
  CostScalar myRowCount = childEstProp->getResultCardinality() / pf;

  myEstProp->setResultCardinality(myRowCount);

  getGroupAttr()->addInputOutputLogProp(inEstLogProp, myEstProp);
}

void
Pack::synthLogProp(NormWA * normWAPtr)
{
  // check to see whether properties are already synthesized.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  // ---------------------------------------------------------------------
  // $$$ anything else on logical properties that need to be added ??
  // ---------------------------------------------------------------------

  // Synthesize things like my record length and drive synthesis for child.
  RelExpr::synthLogProp(normWAPtr);
}

void CommonSubExprRef::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp))
    return ; // already done this

  // make the child's est log props my own
  getGroupAttr()->addInputOutputLogProp (
       inputEstLogProp,
       child(0).getGroupAttr()->outputLogProp(inputEstLogProp));
}

void CommonSubExprRef::synthLogProp(NormWA * normWAPtr)
{
  // Follow the RelExpr base class logic first
  if (getGroupAttr()->existsLogExprForSynthesis())
    return;

  RelExpr::synthLogProp(normWAPtr);

  // simply propagate the child's constraints
  getGroupAttr()->addConstraints(child(0).getGroupAttr()->getConstraints());
} // RelRoutine::synthLogProp()

void RelRoutine::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis())
    return;

  //     For now assume the lower classes will do what they need.
  RelExpr::synthLogProp(normWAPtr);

} // RelRoutine::synthLogProp()


void IsolatedScalarUDF::synthLogProp(NormWA * normWAPtr)
{
  GroupAttributes *GA = getGroupAttr();

  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (GA->existsLogExprForSynthesis()) return;

  super::synthLogProp(normWAPtr);

  if (getRoutineDesc()->getEffectiveNARoutine()->isDeterministic() == FALSE)
      GA->setHasNonDeterministicUDRs(TRUE);

  // For isolated scalar UDFs, always add a constraint of (0,1) since 
  // these funtions only return 1 row per probe (call).  
  GA->addConstraint(new(CmpCommon::statementHeap()) CardConstraint(0,1));


  // We also want to add a FuncDependencyConstraint to establish the 
  // relationship beetween the functions outputs and its inputs.

  FuncDependencyConstraint *fdc =
                          new(CmpCommon::statementHeap())
                              FuncDependencyConstraint(
                                              GA->getCharacteristicInputs(),
                                              GA->getCharacteristicOutputs());
  GA->addConstraint(fdc);

} // IsolatedScalarUDF::synthLogProp()

void IsolatedScalarUDF::synthEstLogProp(const EstLogPropSharedPtr &inputEstLogProp)
{
   if (getGroupAttr()->isPropSynthesized(inputEstLogProp)) return;


      // Make sure we have a fanOut of 1!
   CMPASSERT(getRoutineDesc()->getEffFanOut() == 1);

      // create a EstLogProp for the routine
      // Scaled by the input cardinality.
   EstLogPropSharedPtr myEstProps(new (HISTHEAP) 
                              EstLogProp(
                                 inputEstLogProp->getResultCardinality() *
                                 getRoutineDesc()->getEffFanOut() 
                                        )
                                 );
 
      // We know that at most we will return every row seen.
   myEstProps->setMaxCardEst(inputEstLogProp->getResultCardinality() *
                             getRoutineDesc()->getEffFanOut()
                            );

      // get the default setting for UEC.
   UInt32 defaultUdfUec = (ActiveSchemaDB()->getDefaults()).
                                     getAsULong(ROUTINE_DEFAULT_UEC);

      // loop through the input columns and get the Max of their Uec.
      // We will use this as the output UEC if our metadata did not 
      // specify any UEC for the output column,
   CostScalar maxInputUec = csMinusOne;
   const ColStatDescList &inputColStatsList = inputEstLogProp->getColStats();
   for (UInt32 i=0; i < inputColStatsList.entries(); i++)
   {
      // XXX need to check that the input is part of our characteristicInputs
      // as it may contain more columns than what the routine has as inputs.
      ColStatDescSharedPtr inputStatDesc = inputColStatsList[i];
      maxInputUec = MAXOF(inputStatDesc->getColStats()->getTotalUec(),
                          maxInputUec);
   }

      // Associate the UEC we stored in the RoutineDesc to this 
      // EstLogPropSharedPtr.
   ColStatDescList &colStatDescList = getRoutineDesc()->
                                         getEffOutParamColStatDescList();

      // Check the UEC of each given output parameter
   for (UInt32 k=0; k < colStatDescList.entries(); k++)
   {
      ColStatDescSharedPtr  outColStatDesc  = colStatDescList[k];
      CostScalar maxOutputUec = outColStatDesc->getColStats()->getTotalUec();

         // If we didn't get a value from Metadata, we will either use
         // a value from the CQD or, the max of the inputs.
      if (maxOutputUec == csMinusOne) 
      {
            // We only have one Column in each array
            // We access TotalUec since that is what gets set during
            // ColStats() construction ..
   
            // Check to see what our DEFAULTS is set to..
            // A value higher than 0 in the defaults table overrides the 
            // use of the input UECs as a estimate of the Output UECs.
         if (defaultUdfUec > 0)
           maxOutputUec =  MAXOF(csOne, defaultUdfUec);
   
            // If we don't have output UEC value, and we didn't set a value
            // in the defaults table, use the max of the input UEC for the 
            // output UEC.
         if (maxOutputUec == csMinusOne) 
         {
            if (maxInputUec == csMinusOne)
            {
                  // Since both our input and output column UECs are unknown
                  // Force it to be 1.
               maxOutputUec = csOne; 
            }
            else
            {
                  // Otherwise set it to the max of the inputs.
               maxOutputUec = maxInputUec;
            }
   
         }
            // Update the Uec for the Output Column
            // setRowsAndUec() updates the totalUec..
         outColStatDesc->getColStats()->setRowsAndUec( 
                     outColStatDesc->getColStats()->getRowcount(), maxOutputUec);
      }
   }


      // This is the maximum we will return.
   CostScalar newRowCount = myEstProps->getMaxCardEst();

      // Synchronize the cardinality - estimateCardinalty below assumes
      // that everything has been synchronized.
   colStatDescList.synchronizeStats(newRowCount, colStatDescList.entries());

      // These are inputs from the left
      // can be empty if all the parameters are literals or hostVars.
   ValueIdSet outerReferences = getGroupAttr()->getCharacteristicInputs();
   CollIndex outerRefCount = outerReferences.entries();
   
   MergeType mergeMethod = INNER_JOIN_MERGE; // starting assumption

   newRowCount =
    colStatDescList.estimateCardinality (newRowCount,   /*in*/
				     getSelectionPred(),/*in*/
				     outerReferences,   /*in*/
				     NULL,
                                     NULL,		
				     NULL,
				     outerRefCount,     /*in/out*/
				     myEstProps->unresolvedPreds(),/*in/out*/
				     mergeMethod,      /*in*/
                                     REL_ISOLATED_SCALAR_UDF); /*no-op*/


      // This is the number of rows that we think we will return
   myEstProps->setResultCardinality(newRowCount);
 

      // From the given ColStatDescList, populate columnStats_ with column
      // descriptors that are useful based on the characteristic outputs 
      // for the group. 
   myEstProps->pickOutputs(colStatDescList,
                          inputEstLogProp, 
                          getGroupAttr()->getCharacteristicOutputs(),
                          selectionPred());

      // -----------------------------------------------------------------------
      // This method adds a reference to the provided input estimated logical
      // property.  It also allocates a corresponding set of output estimated
      // logical properties.

   getGroupAttr()->addInputOutputLogProp (inputEstLogProp, myEstProps);

} // IsolatedScalarUDF::synthEstLogProp()

void CallSP::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis())
    return;

  super::synthLogProp(normWAPtr);

} // CallSP::synthLogProp()

void CallSP::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp) == TRUE)
    return;

  // Create a new Output Log Property with cardinality of 1
  const Int32 myCardinality = 1;
  EstLogPropSharedPtr myEstProps(new (HISTHEAP) EstLogProp(myCardinality));

  getGroupAttr()->addInputOutputLogProp(inputEstLogProp, myEstProps);

} // CallSP::synthEstLogProp()



