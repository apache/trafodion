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
 * File:         ColStatDesc.cpp
 * Description:  Column Statistics Descriptor
 * Created:      June 7, 1995
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
#define   SQLPARSERGLOBALS_FLAGS        // must precede all #include's
#include "ColStatDesc.h"
#include "Sqlcomp.h"
#include "ItemColRef.h"
#include "ItemOther.h"
#include "ItemFunc.h"
#include "Cost.h"              /* for lookups in defaults table */
#include "Analyzer.h"
#include "../exp/exp_ovfl_ptal.h" //check for overflow & underflow
#include "CompException.h"
#include "ItemLog.h"	    // for like predicates
#include "hs_globals.h"   // for ustat automation setting
#include "hs_cli.h"       // ustat automation, insert empty histograms
#include "hs_log.h"       // ustat log
#include "SqlParserGlobals.h"
#include "CmpDescribe.h"

#ifdef DEBUG
#define HISTWARNING(x) fprintf(stdout, "Histogram optimizer warning: %s\n", x);
#else
#define HISTWARNING(x)
#endif


//#define MONITOR_SAMEROWCOUNT

// This is an arbitrary constant; perhaps someday it will end up in the
// defaults table.
//
// HIST_MAX_IN_LIST_MEMBERS: the largest number of IN-list members for
// which we attempt to do exact histogram-manipulation; for IN-lists of
// cardinality greater than this constant, we do some massive
// simplifications (see CSDL::estimateCardinality)
const Int32 HIST_MAX_IN_LIST_MEMBERS = 40 ;

// -----------------------------------------------------------------------
//  Methods on ColStatDesc class
// -----------------------------------------------------------------------

ColStatDesc::ColStatDesc (const ColStatsSharedPtr& stats,
                          const ValueIdList& columnList,
                          NAMemory * h) :
     nonVegEquals_(h)
{
  // Set up a reference to the ColStats structure
  colStats_ = stats;

  // ---------------------------------------------------------------------
  // Now, generate the valueidlist for this set of column statistics.
  // Also, generate the initial mergeState_ for this stats' column.
  // Initially, the state is that only the current column has been merged.
  // ---------------------------------------------------------------------
  const ValueId & id = columnList[stats->getStatColumns()[0]->getPosition()];
  column_ = id ;
  VEGcolumn_ = column_;
  mergeState_.insert(column_);

  fromInnerTable_ = FALSE;

  // use default constructor for nonVegEquals, and appliedPreds_

  // underlying histogram is not modified yet
  modified_ = FALSE;
  inputCard_ = 1.0;
}

// Construct a ColStatDesc given a single ValueId and a pointer
// to a ColStats object.  This constructor does not assume that
// the ColStats object contains a NAColumnList.  Used when constructing
// ColStats for generated columns that are not based on BASECOLS,
// such as with TRANSPOSE.
//
ColStatDesc::ColStatDesc(const ColStatsSharedPtr& stats,
                         const ValueId & column, NAMemory * h)
     : nonVegEquals_(h)
{
  // Set up a reference to the ColStats structure
  colStats_ = stats;

  // ---------------------------------------------------------------------
  // Set up the column, VEGColumn and MergeState.
  // ---------------------------------------------------------------------
  column_ = column ;
  VEGcolumn_ = column;

  mergeState_.insert(column) ;

  fromInnerTable_ = FALSE;

  // underlying histogram is not modified yet
  modified_ = FALSE;

  inputCard_ = 1.0;
}

void
ColStatDesc::copy (const ColStatDesc& other)
{
  column_         = other.column_;
  VEGcolumn_      = other.VEGcolumn_;
  mergeState_     = other.mergeState_;
  nonVegEquals_   = other.nonVegEquals_;
  fromInnerTable_ = other.fromInnerTable_;
  appliedPreds_   = other.appliedPreds_;
  colStats_       = other.colStats_;
  inputCard_	  = other.inputCard_;
  modified_       = FALSE;
}

void
ColStatDesc::mapUpAndCopy (const ColStatDesc& other, ValueIdMap &map)
{
  copy(other);
  map.mapValueIdUp(column_, other.column_);
  map.mapValueIdUp(VEGcolumn_, other.VEGcolumn_);

  // if the map only maps the column or VEGcolumn but not both, then
  // map both to the same value id
  if (column_ == other.column_ && VEGcolumn_ != other.VEGcolumn_)
    column_ = VEGcolumn_;
  if (VEGcolumn_ == other.VEGcolumn_ && column_ != other.column_)
    VEGcolumn_ = column_;

  // rewrite applied predicates
  if (!appliedPreds_.isEmpty() &&
      (column_ != other.column_ || VEGcolumn_ != other.VEGcolumn_))
    {
      appliedPreds_.clear();
      map.rewriteValueIdSetUp(appliedPreds_, other.appliedPreds_);
    }
}

void
ColStatDesc::deallocate()
{
  colStats_ = NULL;
}

// -----------------------------------------------------------------------
//  ColStatDesc::getColStatsToModify
//
//  If this is the first time that the underlying colstats is being
//  modified, then make a copy first. Otherwise, just return the reference
//  to colStats.
// -----------------------------------------------------------------------
ColStatsSharedPtr
ColStatDesc::getColStatsToModify()
{
  if (NOT isModified())
  {
    colStats_ = ColStatsSharedPtr(new (HISTHEAP)
				  ColStats (*colStats_, HISTHEAP));
    modified_ = TRUE;
  }

  return colStats_;
}

// argh!  pow(x,0.0) returns not-a-number!
//
// This version of pow has been written specially to do the right thing
// for its call in the following function ; use of it by any other method
// is probably not a good idea.
double mypow (double base, double exp)
{
  if ( exp == 0 )
    return 1 ; // duh!
  else if ( base <= 0 )
    return 0 ; // this isn't obvious ... hmmmm ...
  else
    return pow(base,exp) ;
}

// -----------------------------------------------------------------------
// Historically we've reduced UEC values by the same amount as rowcounts
// when a predicate is applied; but this is extremely foolish, as UEC
// values generally do not reduce very rapidly as rowcounts go down (e.g.,
// a "downsizing" of 50% of employees (reduction of .5) is not likely to
// reduce the UEC of the genders of employees by the same .5!)  In general,
// we expect UEC to go down only when rowcount goes down a lot.
//
// Making this a member function so that anyone else who wants to use it
// will be able to "find" it easier -- and so it doesn't pollute the
// global namespace.
// -----------------------------------------------------------------------
CostScalar
ColStatDesc::calculateCorrectResultUec( const CostScalar & baseRows,
                                        const CostScalar & newRows,
                                        const CostScalar & baseUec)
{
  // Sanity checks.
  CCMPASSERT( baseRows.isGreaterOrEqualThanZero() );
  CCMPASSERT( newRows.isGreaterOrEqualThanZero() );
  CCMPASSERT( baseUec.isGreaterOrEqualThanZero() );

  // we start with a simple sanity check
  if ( baseRows.isZero() || newRows.isZero() || baseUec.isZero() )
    return csZero; // rows(0) ==> uec(0)

  // now let's get started ... here's a good zeroth approximation
  CostScalar firstApprox = baseUec ;

  // as a first approximation, we limit uec to be (at most) the new rowcount
  if ( firstApprox > newRows )
	firstApprox = newRows ; // reduce to the new rowcount

  // be careful to not reduce uec if the rowcount is increasing (or
  // remaining constant)!
  if ( newRows >= baseRows )
  {
	return firstApprox ; // this is equal to MIN (baseUec, newRows)
  }

// ------------------------------------------------------------------------
// The UEC reduction formula is derived as follow
//
// u boxes
// N balls
//
// Balls are distributed among the u boxes.
//
// The distribution was performed randomly except for that the occupancy
// of any box should be greater than 0.  This was done by putting 1 ball
// in each of the u boxes and distributing the remaining N-u balls
// randomly (with equal probabilities for each box).
//
// Now apply an orthogonal selection with reduction factor R; i.e., remove
// (1-R)*N balls randomly (each ball has same chance of being removed,
// i.e., (1-R))
//
// The probability that a box is empty is
// = (Prob. of that first ball removed)
//    * (Prob. all other balls in the box were removed)
//
// The first term is simply (1-R)
// The second term can be shown to be (1-R/u)^(N-u)
// which can be approximated to exp(-R(N-u)/u) for N-u >> 1
//
// Consequently the result UEC (or # non empty boxes) is
// UEC = u*(1-(1-R)*(1-R/u)^(N-u))
//     = u*(1-(1-R)*exp(-R(N-u)/u))   N-u >> 1     ----(1)
//
// As for the linear count estimate it does not take into account
// the requirement that box initial occupancy > 0. Consequently
// It performs poorly when the values of N and u are close, which
// is a very common case.
// -----------------------------------------------------------------------

  //NB: if we reach here, baseRows >= newRows

  double R = ((CostScalar) newRows / baseRows).value() ;
  double N = baseRows.value() ;
  double u = baseUec.value() ;

  CostScalar secondApprox ;


  // we fudge >> to mean greater than 10; this should be fine
  if ( N-u > 10 )
	secondApprox = u * (1-(1-R) * exp(-R*(N-u)/u)) ;
  else
	secondApprox = u * (1-(1-R) * mypow(1-R/u,N-u)) ;


  // sanity check
//  if ( secondApprox.getValue() <= csZero.getValue() || secondApprox > firstApprox )
  if ( NOT secondApprox.isGreaterThanZero() OR secondApprox > firstApprox )
	secondApprox = firstApprox ;

  return secondApprox ;
}

// -----------------------------------------------------------------------
//  ColStatDesc::applySel
//
//  Apply the provided selectivity to the underlying column statistics
//  (ColStats structure).
// -----------------------------------------------------------------------

void
ColStatDesc::applySel( const CostScalar & selectivity )
{
  if ( selectivity == csOne )
    return ; // nothing to do

  ColStatsSharedPtr colStats = getColStatsToModify() ;

  // --------------------------------------------------------------------
  // when we "scale up" a histogram, it loses its uniqueness
  if ( selectivity.isGreaterThanOne() /* > 1 */)
  {
    colStats->setUnique( FALSE );
  }
  // --------------------------------------------------------------------

  const CostScalar & baseUec      = colStats->getTotalUec();
  const CostScalar & baseRowcount = colStats->getRowcount();
  const CostScalar   newRowcount  = MIN_ONE_CS(baseRowcount * selectivity);
  const CostScalar & originalBaseRowCount = colStats->getBaseRowCount();

  CostScalar baseRowCountForUec = baseRowcount;

  if ((originalBaseRowCount < baseRowcount) &&
    (originalBaseRowCount != csMinusOne))
    {
       baseRowCountForUec = originalBaseRowCount;
       colStats->setBaseRowCount(-1);
    }

  CostScalar newTotalUec, uecSelectivity;

  if ( baseUec.isZero() ) // avoid div-by-zero!
  {
// LCOV_EXCL_START - rfi
    newTotalUec = csZero;
    uecSelectivity = csZero;
// LCOV_EXCL_STOP
  }
  else if ( colStats->isUnique() )
  {
    // If this is a UNIQUE column, UEC == rowcount
    newTotalUec = newRowcount;
    uecSelectivity = selectivity;
  }
  else
  {
    newTotalUec = calculateCorrectResultUec( baseRowCountForUec,
					     newRowcount,
					     baseUec);

    uecSelectivity = newTotalUec / baseUec;
  }

  // If row count is anywhere >= one, then uplift the UEC to atleast one.
  if ((newRowcount >= csOne) && (newTotalUec < csOne))
    newTotalUec = csOne;

  colStats->setRedFactor   ( colStats->getRedFactor() * selectivity );
  colStats->setUecRedFactor( colStats->getUecRedFactor() * uecSelectivity );
  colStats->setRowsAndUec  ( newRowcount, newTotalUec );
}

// ----------------------------------------------------------------------
//  ColStatDesc::applySelIfSpecifiedViaHint
//
//  This helper method is called by various cardinality estimation
//  methods to adjust cardinality based on user-specified selectivity.
// ----------------------------------------------------------------------
void 
ColStatDesc::applySelIfSpecifiedViaHint(ItemExpr * pred, const CostScalar & oldRowcount)
{
  // If user specified selectivity for this predicate, we need to make
  // adjustment in reduction to reflect that.
  if(pred->isSelectivitySetUsingHint())
  {
    ColStatsSharedPtr colStats = getColStats();
    CostScalar selAdjustment(oldRowcount * pred->getSelectivityFactor()/getColStats()->getRowcount());
    if(selAdjustment > csZero)
    {
      applySel(selAdjustment);
      colStats->setSelectivitySetUsingHint();
    }
  }
}

// ----------------------------------------------------------------------
//  ColStatDesc::synchronizeStats
//
//  Change the rowcount of THIS to match the specified newRowcount;
//    Apply the change to histograms by changing the associated RedFactor.
//  Change the UEC of THIS in proportion to the change in UEC characterized
//    by baseUec and newUec.  Again, apply this to histograms by changing
//    the associated uecRedFactor.  Do not allow the resulting UEC to exceed
//    the resulting rowcount.
//
//  The third parameter is optional; it is used by some functions which
//  are certain that under no circumstances should the UEC be allowed to
//  be changed.  Otherwise, UEC is reduced as per the function above.
//
//  NOTE how similar this function is to CSD::applySel() above.  In most
//  cases, when both the old and new rowcounts are known (e.g., in cases
//  when we're not just applying some selectivity), this function should
//  be used --> except, of course, in cases when we need to make sure to
//  reduce the uec to be at most 1.
// ----------------------------------------------------------------------
void
ColStatDesc::synchronizeStats( const CostScalar & baseRowcount,
                               const CostScalar & newRowcount,
                               SynchSpecialFlag specialFlag )
{
  CCMPASSERT( baseRowcount.isGreaterOrEqualThanZero() );
  CCMPASSERT( newRowcount.isGreaterOrEqualThanZero() );

  if ( getColStats()->getRowcount() == newRowcount )
    return ; // nothing to do

  // First check to do is to see if RowCount can be uplifted to one, if
  // it is not zero and less than one. This will solve the problem of
  // UECs less than 1, which can reach 0 and cause overflow - RV

  CostScalar newRowCountCorrected;
  newRowCountCorrected = MIN_ONE_CS(newRowcount);

  // now use this adjusted newRowCount for all further calculations

  // it's really not clear what the rowred should be when the baseRowcount
  // starts out as zero ...
  // avoid divided by zero
  CostScalar rowSelectivity =
    (baseRowcount.isZero()) ? csOne : newRowCountCorrected / baseRowcount;

  ColStatsSharedPtr colStats = getColStatsToModify();
  // get base row count of the histogram, to see if that can be used to
  // calculate the Uec reduction RV
  const CostScalar &originalBaseRowCount = colStats->getBaseRowCount();
  // to get the maximum reduction, use the lower row count

 CostScalar baseRowCountForUec = baseRowcount;

 if ((originalBaseRowCount < baseRowcount) &&
     (originalBaseRowCount != csMinusOne))
     {
        baseRowCountForUec = originalBaseRowCount;
        colStats->setBaseRowCount(-1);
     }

  if ( colStats->getRowcount() != baseRowcount )
  {
    CostScalar cardDiff = colStats->getRowcount() - baseRowcount;
    if (_ABSOLUTE_VALUE_(cardDiff.value()) > 1.0)
    {

      // unusual case: but if it does happen, fudge the synchronization
      // results based upon how far our numbers are from what is expected.
      // The way we do round our cardinalities, difference of one row shows
      // up, which could be unintentional. Hence we would fudge the
      // synchronization results only if the difference is more than one.
      // Else we shall go by the reduction of newRowcount / oldRowcount
      // because that is what it should be
      // There is an extra check of != to avoid computing of ABSOLUTE_VALUE
      // in most cases.

      rowSelectivity =
	(baseRowcount.isZero()) ? csOne : colStats->getRowcount() / baseRowcount;
    }
  }

  // --------------------------------------------------------------------
  // when we "scale up" a histogram, it loses its uniqueness
  if ( rowSelectivity > csOne )
  {
    colStats->setUnique( FALSE );
  }
  // --------------------------------------------------------------------

  const CostScalar & baseUec = colStats->getTotalUec();

  CostScalar newTotalUec, uecSelectivity;

  NABoolean rowCountIsOne = FALSE;

  if ( baseUec.isZero() ) // avoid div-by-zero
  {
// LCOV_EXCL_START - rfi
    newTotalUec = csZero;
    uecSelectivity = csZero;
// LCOV_EXCL_STOP
  }
  else if ( specialFlag == DO_NOT_REDUCE_UEC )
  {
    // Sometimes the calling function knows that the UEC should not change.

    // Want to calculate uecSelectivity right.
    newTotalUec = MINOF( baseUec, newRowCountCorrected );
    uecSelectivity = newTotalUec / baseUec;	  // usually 1
  }
  else if ( colStats->isUnique() )
  {
    // If it's a UNIQUE column, then UEC == ROWCOUNT.
    if ( specialFlag == SET_UEC_TO_ONE )
    {
      // In this case, both uec & rowcount should be 1.
      rowCountIsOne = TRUE;
      rowSelectivity =
	(baseRowcount.isZero()) ? csOne : csOne / baseRowcount;
    }

    newTotalUec    = rowCountIsOne ? csOne : newRowCountCorrected ;
    uecSelectivity = rowSelectivity ;
  }
  else if ( specialFlag == SET_UEC_TO_ONE )
  {
    // Sometimes the calling function knows that
    // resulting UEC should be at most 1.

    newTotalUec = MINOF( baseUec, csOne );
    // Want to calculate uecSelectivity right.
    newTotalUec = MINOF( newTotalUec, newRowCountCorrected );
    uecSelectivity = newTotalUec / baseUec;
  }
  else // the usual case
  {
    newTotalUec = calculateCorrectResultUec( baseRowCountForUec,
                                 newRowCountCorrected,
                                 baseUec);
	uecSelectivity = newTotalUec / baseUec;
  }

  // If row count is one, then uplift the UEC to one also.
  if ((newRowCountCorrected >= csOne) && (newTotalUec < csOne))
    newTotalUec = csOne;

  colStats->setRedFactor   ( colStats->getRedFactor() * rowSelectivity );
  colStats->setUecRedFactor( colStats->getUecRedFactor() * uecSelectivity );
  colStats->setRowsAndUec  ( ( rowCountIsOne ? csOne : newRowCountCorrected ),
			     newTotalUec );
}

// -----------------------------------------------------------------------
//  ColStatDesc::modifyStats
//
//  Given a supported predicate, apply the effect of the predicate on the
//  ColStats structure, as well as its corresponding histogram.  This
//  method expects the unary predicates
//              IS [NOT] NULL
//              IS [NOT] UNKNOWN
//  and binary predicates of the form
//              COLUMN <op> <constant>
//              <constant> <op> COLUMN
//  This routine also checks for doable, simple, special, cases like
//              column_a <op> column_a
//
//  Calls these functions to do the work of predicate application:
//
//      ColStats::modifyStats
//      ColStats::simplestPreds
//
//  return value:  semantic =~= "everything's OK"
//  -------------
//  FALSE if one of the following:
//      1. none of the above predicate cases applies
//      2. the identified column does not appear in the column list
//      3. the CSD's histogram is NULL
//
//  TRUE if none of the three conditions for FALSE are met,
//      or if the predicate has already been applied
// -----------------------------------------------------------------------


NABoolean
ColStatDesc::modifyStats( ItemExpr *pred, CostScalar & newRowcount,
                          CostScalar *maxSelectivity )
{
  const ValueId & predValueId = pred->getValueId();
  OperatorTypeEnum op = pred->getOperatorType();
  ItemExpr * lhs = pred->child(0);
  ItemExpr * rhs = NULL;
  ConstValue * constant = NULL;
  NABoolean negate = FALSE;

  if ( pred->getArity() > 1 )
  {
    rhs = pred->child(1);
    constant = rhs->castToConstValue( negate );
    if(constant == NULL)
    {
      if (rhs->getOperatorType() == ITM_VEG_REFERENCE)
      {
        const VEG * veg = ((VEGReference *)rhs)->getVEG();
        ValueId constId = veg->getAConstant();
        if(constId != NULL_VALUE_ID)
	  constant = constId.getItemExpr()->castToConstValue( negate );
      }
      else
      {
        if ((op == ITM_EQUAL) &&
            (rhs->getOperatorType() == ITM_CACHE_PARAM) )
        {
          ItemExpr * constantExpr = ((ConstantParameter *)rhs)->getConstVal();
          if (constantExpr == NULL)
            return FALSE;
          constant =  constantExpr->castToConstValue(negate);        }// cache_param
      } // not aveg_reference
    }// constant !=null
  } //arity > 1

  // get writable copies of the ColStats structure
  ColStatsSharedPtr colStats = getColStatsToModify();
  newRowcount = colStats->getRowcount();

  // stand alone column should always be on lhs

  // if the predicate being applied is IS NULL then do not skip
  // instantiate_null expression from lhs.
  NABoolean digIntoInstantiateNull = TRUE;
  if ((op == ITM_IS_NULL) || (op == ITM_IS_NOT_NULL))
    digIntoInstantiateNull = FALSE;
  
   if ( (lhs->getOperatorType() != ITM_VEG_REFERENCE) &&
        (constant || (op == ITM_EQUAL) || (op == ITM_NOT_EQUAL) ||
	  (op == ITM_IS_NULL) || (op == ITM_IS_NOT_NULL)))
  {
    lhs = lhs->getLeafValueIfUseStats(digIntoInstantiateNull);
  }

  ValueId vegCol = getVEGColumn();
  if (lhs->getValueId() != vegCol )
  {
    // if the valueIDs match, then we have found the histograms for the
    // child, proceed with applying predicate
    // If the valueIds are not equal, then see if it is a VEG region 
    // (defined by VEG_REFERENCE). If it is, dig into the region to 
    // get VEG references. This is required especially for full outer joins, 
    // where the join expression forms a VEG region of its own with 
    // VEG references from the two children. However, the histogram for the join
    // is still identified by the VEG reference of the left child
    if (lhs->getOperatorType() == ITM_VEG_REFERENCE)
    {
      ValueIdSet cols;
      lhs->findAll(ITM_VEG_REFERENCE, cols, TRUE, TRUE);
      if (!cols.contains(vegCol))
        return FALSE;
    }
    else
     return FALSE;
  }

  // three kinds of preds that we can handle:
  // 1. constant is not NULL (i.e., of form <constant> op <col>)
  // 2. unary predicate
  // 3. binary predicate on same column
  // ==> if none of these is TRUE, then we can't evaluate this predicate
  if ( NOT (	constant != NULL
	     OR pred->getArity() == 1
	     OR (     pred->getArity() == 2
		  AND lhs->getValueId() == rhs->getValueId()
		)
	   )
     )
  {
    return FALSE;      // Correct column, but can't evaluate predicate....
  }

  // OK, all's well, now we evaluate the predicate

  CostScalar rowcount = csZero;
  CostScalar uec      = csZero;
  const CostScalar & origRowcount = colStats->getRowcount();
  const CostScalar & origUec      = colStats->getTotalUec();

  // These will be used later to check for the boundary condition for
  // range predicates
  const EncodedValue maxValue = colStats->getMaxValue();
  const EncodedValue minValue = colStats->getMinValue();

  // get the encoded value of the constant if any
  EncodedValue val (UNINIT_ENCODEDVALUE) ;

  if (constant != NULL)
    // get the encoded format for the constant
    val = EncodedValue (constant, negate);


  // check: if we've already applied this predicate, don't do it again!
  if ( isPredicateApplied( predValueId ) )
  {
    newRowcount = origRowcount;
    return TRUE;
  }

  // extrapolate histogram, if the value being looked for lies outside the histogram boundaries
  // no need to extrapolate if the predicate being applied is less than or less than equal to
  if (colStats->getStatColumns()[0]->getType()->getTypeQualifier() == NA_DATETIME_TYPE 
      && val >= maxValue
      && !colStats->isOrigFakeHist()
      && (op == ITM_EQUAL || op == ITM_GREATER || op == ITM_GREATER_EQ)
      && maxSelectivity == NULL
      && constant != NULL)
         colStats->adjustRowcountforRollingColumns(constant);

  // OK, predicate has NOT already been applied

  // remember whether or not a histogram is 'fake' prior to applying the
  // given predicate.
  NABoolean isaFakeHistogram = colStats->isFakeHistogram() ;

  if ( (pred->getArity() == 2) &&
       (lhs->getValueId() == rhs->getValueId()) )
  {
    if (maxSelectivity == NULL)
    colStats->simplestPreds( pred );
  }
  else
  {
    colStats->modifyStats( pred, maxSelectivity );
  }

  // maxSelectivity computation is done
  if (maxSelectivity) {
    newRowcount = colStats->getRowcount();
    return TRUE;
  }

  rowcount = colStats->getRowcount();
  uec      = colStats->getTotalUec();

  // ------------------------------------------------------------------
  // Lastly, if this predicate was applied to a 'fake' histogram
  //    AND the predicate did not eliminate all of the rows,
  //    AND no 'similar' predicate has already been applied.
  // ==> Alter this predicate's impact on the total RowCount & UEC to
  // match that of a 'default' predicate.  In cases of multiple
  // applications of 'similar' predicates, any shape-changing impact that
  // occurs above will continue to have its effect, but no actual
  // reduction in rowcount will occur.
  //
  // All changes are accomplished via alterations to appropriate reduction
  // factors.
  // ------------------------------------------------------------------

  // ------------------------------------------------------------------
  // When we're certain that we should end up with zero rows, we set the
  // histogram accordingly :
  //   1. max(min)-set-by-pred are TRUE; and
  //   2. histogram has one interval with bounds UNINIT_ENCODEDVALUE
  //   3. max/min bounds are set to UNINIT_ENCODEDVALUE
  // Unless these conditions are met, then if we ever end up with zero
  // rows, we want to undo what we did and apply default selectivity
  // (instead of the overly-selective predicate that we just applied).
  // ------------------------------------------------------------------

  HistogramSharedPtr hist = colStats->getHistogramToModify() ;

  if ( rowcount.isZero()
       AND NOT (     hist->entries() == 1
		 AND colStats->isMaxSetByPred()
		 AND colStats->isMinSetByPred()
		 AND colStats->getMaxValue() == UNINIT_ENCODEDVALUE
		 AND colStats->getMinValue() == UNINIT_ENCODEDVALUE
	       )
     )
  {
    // At this point we're not going to preserve any multiple-interval
    // shape of the histogram, because we're grasping at straws in our
    // attempt to prevent a zero-overall rowcount.  This is a kludge to
    // prevent undesired zero rowcounts (we operate under the assumption
    // that *all* zero rowcounts are unwanted).  The easiest/best way to
    // proceed is to populate a single-interval histogram with the
    // original rows & uec, declare this a fake histogram, and then
    // apply default selectivity (see next if-clause below) to come up with
    // a more reasonable reduction.
    hist->condenseToSingleInterval();
    colStats->setIsCompressed(TRUE);

    // first, set the aggregate values
    colStats->setRowsAndUec  ( origRowcount, origUec );
    colStats->setRedFactor   ( csOne );
    colStats->setUecRedFactor( csOne );

    // Set first interval's rowcount and uec.
    hist->getFirstInterval().setRowsAndUec( origRowcount, origUec );

    // from this point forward, we're going to consider this a fake histogram
    colStats->setFakeHistogram();
    isaFakeHistogram = TRUE;

    // finally, reset the rowcount/uec values
    rowcount = colStats->getRowcount();
    uec      = colStats->getTotalUec();
  }

  // after we have applied the predicate on the histogram (modified the
  // interval boundaries to reflect the predicate), and taken care of
  // special conditions, we check to see if the predicate applied
  // is a derivative of LIKE, or a similar predicate has been applied
  // to that histogram. If it is, then we don't want to apply the
  // reduction twice on that histogram. We shall bring the
  // rowcount and the UEC to what it was before the predicate was
  // applied. Hence, at the end of it, we shall have the boundaries
  // of the histogram intervals reflecting the two predicates,
  // and the rows and UEC reflecting one predicate. For LIKE predicate
  // this will ensure that the final selectivity after applying both range
  // predicates is equal to the default selectivity of LIKE predicates
  //


  if (derivOfLikeAndSimilarPredApp(pred) )
  {
    synchronizeStats( rowcount, origRowcount, DO_NOTHING_SPECIAL );
  }
  else
  {
	if (isaFakeHistogram 
	     AND NOT (     hist->entries() == 1
	     AND colStats->isMaxSetByPred()
	     AND colStats->isMinSetByPred()
	     AND colStats->getMaxValue() == UNINIT_ENCODEDVALUE
	     AND colStats->getMinValue() == UNINIT_ENCODEDVALUE
	       )
	   )
	{
          CostScalar defaultRowcount = origRowcount;

          if (!isSimilarPredicateApplied(pred->getOperatorType()))
          {
            // if it is a fake histogram, then for equality predicate, set the rowcount as
            // the sqrt of the original rowcount
            if (op == ITM_EQUAL)
            {
              if (colStats->isUnique())
                defaultRowcount = 1;
              else
                defaultRowcount = ceil(sqrt(origRowcount.getValue()));
            }
            else
              if (op == ITM_NOT_EQUAL)
              {
                defaultRowcount.minCsOne();
                if (colStats->isUnique())
                  defaultRowcount -= 1;
                else
                  defaultRowcount = origRowcount - ceil(sqrt(origRowcount.getValue()));
              }
              else
                defaultRowcount = origRowcount * pred->defaultSel();
          }

	  SynchSpecialFlag specialFlag = DO_NOTHING_SPECIAL;
	  // handle the operators which set UEC to one
	  if ( op == ITM_IS_NULL ||
	   op == ITM_IS_UNKNOWN ||
	   op == ITM_EQUAL )
	  {
		// the resulting UEC after this operation should be at most one
		specialFlag = SET_UEC_TO_ONE;
	  }

	  synchronizeStats( rowcount, defaultRowcount, specialFlag );
	}
  }

  addToAppliedPreds( predValueId );  // in this case: add when done

  // Now do the lower bound check for range predicates

  if ( !isaFakeHistogram && (val != UNINIT_ENCODEDVALUE) )
  {
    if ( op == ITM_LESS || op == ITM_LESS_EQ ||
       op == ITM_GREATER || op == ITM_GREATER_EQ)
    {
      double baseRowcount = colStats->getBaseRowCount().getValue();
      double baseUec = (colStats->getUecBeforePreds().getValue());
      if (baseUec < 1.0) baseUec = 1.0;

      double minRowcount = baseRowcount/baseUec;

      if (colStats->getRowcount() < minRowcount)
	synchronizeStats( rowcount, minRowcount );
    }
  }

  newRowcount = colStats->getRowcount();
  return TRUE;
} // ColStatDesc::modifyStats

// This method merges the two histograms from the same table
NABoolean
ColStatDesc::mergeColStatDescOfSameTable(ColStatDescSharedPtr &rightColStats,
					 OperatorTypeEnum opType)
{
  NABoolean checkForMergeFromScan = FALSE;

  if ((opType == REL_SCAN) || (opType == ITM_OR) || (opType == ITM_AND) )
    checkForMergeFromScan = TRUE;

  if ((!checkForMergeFromScan) ||
      (CmpCommon::getDefault(COMP_BOOL_74) == DF_OFF) )
    return FALSE;

  ColStatsSharedPtr rootColStats = getColStatsToModify();

  CostScalar leftUec = rootColStats->getTotalUec();
  CostScalar rightUec = rightColStats->getColStats()->getTotalUec();
  // In exceptional cases where one of the columns has been reduced by another predicate
  // example, upper(col1) = upper(col2) and col1 = 'B', we should to take the lower rowcount
  // Without the upper expression, we will get a VEG here, and that would be handled at 
  // the method applyVEGPred method and the control will not come here
  CostScalar newRowcount = MINOF(rootColStats->getRowcount(), rightColStats->getColStats()->getRowcount());
  CostScalar minUec = MINOF(leftUec, rightUec);

  // use HIST_NO_STATS_UEC to compute selectivity for equality prdicates 
  // from same table (T1.a = T1.b)

  double selectivityForPredEqual = (1.0/(CURRSTMT_OPTDEFAULTS->defNoStatsUec()) );
  newRowcount = (newRowcount * selectivityForPredEqual).minCsOne();
  CostScalar newUec = MINOF(minUec, newRowcount);

  HistogramSharedPtr hist = rootColStats->getHistogramToModify() ;

  hist->condenseToSingleInterval();
  rootColStats->setIsCompressed(TRUE);
  // Set first interval's rowcount and uec.
  hist->getFirstInterval().setRowsAndUec( newRowcount, newUec );

  // first, set the aggregate values
  rootColStats->setRowsAndUec  ( newRowcount, newUec );
  if(opType == REL_SCAN)
    rootColStats->setBaseUec(newUec);
  rootColStats->setRedFactor   ( csOne );
  rootColStats->setUecRedFactor( csOne );
  rootColStats->setRecentJoin(TRUE);
  appliedPreds().insert( rightColStats->getAppliedPreds() );
  mergeState().insert(rightColStats->getMergeState() );

  // from this point forward, we're going to consider this a fake histogram
  rootColStats->setFakeHistogram();
  return TRUE;
}

// -----------------------------------------------------------------------
// ColStatDesc::mergeColStatDesc
//
// merge two ColStatDesc's
//
// forceMerge overrides logic that prevents equijoins of the form
//      <col_1> = <col_1>
//
// Calls
//
//      ColStats::mergeColStats
//
// to do the low-level merge work
// -----------------------------------------------------------------------
void
ColStatDesc::mergeColStatDesc (ColStatDescSharedPtr& mergedStatDesc,
                               MergeType mergeMethod,
                               NABoolean forceMerge,
                               OperatorTypeEnum exprOpCode,
                               NABoolean mergeFVs)
{
  ColStatsSharedPtr rootColStats = getColStatsToModify();
  ColStatsSharedPtr mergedColStats = mergedStatDesc->getColStats();

  CostScalar minCard = csOne;
  // before we start manipulating histograms for join
  // let collect the minimum cardinality, that will be used
  // to do the sanity check later. Do this only if the 
  // parent is a Join
  if ((CmpCommon::getDefault(COMP_BOOL_45) == DF_ON ) &&
      (exprOpCode == REL_JOIN) && 
        (mergeMethod == INNER_JOIN_MERGE) &&
	  !rootColStats->isVirtualColForHist() &&
	  !mergedColStats->isVirtualColForHist() )
  {
    if (CmpCommon::getDefault(COMP_BOOL_46) == DF_OFF)
    {
      CostScalar leftFreq = rootColStats->getMaxFreq();
      CostScalar rightFreq = mergedColStats->getMaxFreq();
      minCard = MAXOF(leftFreq, rightFreq).minCsOne();
    }
  } // exprOpCode == REL_JOIN

  if ( NOT ( mergeMethod == UNION_MERGE ||
             mergeMethod == OR_MERGE    ||
             mergeMethod == LEFT_JOIN_OR_MERGE )
       /* not really a join */
     )
  {
    // --------------------------------------------------------------------
    // We do the usual case first :
    //        merges that are *NOT* (the unusual) UNIONs, ORs, or LEFT_JOIN_ORs
    // --------------------------------------------------------------------
    // For join-related merges, add any/all entries in the nonVegEquals_
    // ColStatDesc LIST of the mergedStatDesc to the nonVegEquals_ in
    // THIS.
    //
    // Unfortunately, this can't be a simple set insert, because members
    // of the set(s) are pointers, and we may have two different pointers
    // pointing to the 'same' ColStatDesc.
    // --------------------------------------------------------------------
    for ( CollIndex j = 0; j < mergedStatDesc->nonVegEquals_.entries(); j++ )
    {
      ColStatDescSharedPtr tmpDescJ = mergedStatDesc->nonVegEquals_[j];
      NABoolean foundFlag = FALSE;

      for ( CollIndex i = 0; i < nonVegEquals_.entries(); i++ )
      {
		ColStatDescSharedPtr tmpDescI = nonVegEquals_[i];
		if ( tmpDescI->VEGcolumn_ == tmpDescJ->VEGcolumn_ )
		{
		  foundFlag = TRUE;
		  break ;
		}
      } // for i

      if ( foundFlag == FALSE )
	  nonVegEquals_.insert( tmpDescJ );
    } // for j

	NABoolean skipJoin = FALSE;

    // --------------------------------------------------------------------
    // For any type of Join-related merge, Test for column statistics
    // with identical merge states, and don't perform that join.
    // --------------------------------------------------------------------
    if ( mergedStatDesc->getMergeState() == mergeState_ &&
	 forceMerge == FALSE )
    {
      // if the root contains more rows than the to-be-merged version,
      // overwrite the root with the other version.
      // else do nothing: rootColStats is o.k.
      if ( rootColStats->getRowcount() > mergedColStats->getRowcount() )
		rootColStats->overwrite( *mergedColStats );

      // update the applied predicates
      appliedPreds().insert( mergedStatDesc->getAppliedPreds() );
	  skipJoin = TRUE;
    } // forceMerge == FALSE, the two merge states are same
    // Test for rootState being a subset of the mergedState.
    else if ( mergedStatDesc->getMergeState().contains( mergeState() ) &&
	      forceMerge == FALSE )
	  {
		// the to-be-merged colStats shows the effects of a merge with
		// the rootColStats; overwrite the root copy
		rootColStats->overwrite( *mergedColStats );

		// update the mergeState_ of the root copy.
		mergeState().clear();
		mergeState().insert( mergedStatDesc->getMergeState() );

		// and its applied predicates
		appliedPreds().insert( mergedStatDesc->getAppliedPreds() );
		skipJoin = TRUE;
	  } // root state is a subset of merged state
        // left contains right -- join's already been done, and we're not forced
        else if ( mergeState().contains( mergedStatDesc->getMergeState() ) &&
	          forceMerge == FALSE )
        {
          return ;  // nothing left to merge
        }
	else	// join on unique columns
	if ( forceMerge == FALSE &&
        ( (mergeMethod == INNER_JOIN_MERGE) ||
          (mergeMethod == SEMI_JOIN_MERGE) ) )
        {
	  // Before starting with the checks, if the statistics exist
	  // for the columns being joined. Skip the following logic, if
	  // 1. no statistics exists for either of the columns
	  // 2. join is being performed on the histograms with virtual column

	  if ((CmpCommon::getDefault(COMP_BOOL_48) == DF_ON) ||
            (!rootColStats->isOrigFakeHist() &&
		  !mergedColStats->isOrigFakeHist() &&
		  !rootColStats->isVirtualColForHist() &&
		  !mergedColStats->isVirtualColForHist() ))
	  {
            NABoolean scaleFreq = TRUE;
            if (mergeMethod == SEMI_JOIN_MERGE)
              scaleFreq = FALSE;

            // merge freq values of the two sides, if the number of values in the freq value list for both
            // histograms is less than the threshold value and in case of tuple list, a frequent value list 
            // has been created  
            // 
            NABoolean mergeFreqValues = FALSE;

            CostScalar uecCushion ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_4));
	    // check if it is a primary_key - foreign_key join.If yes, and 
	    // No merge is required

	    ValueIdSet colSetLeft = mergeState();
	    NABoolean leftJoinUnique = FALSE;

	    ValueIdSet colSetRight = (ValueIdSet)(mergedStatDesc->getMergeState());
	    NABoolean rightJoinUnique = FALSE;
            NABoolean joinWithTupleList = FALSE;

	    // For semi_joins, only right side being unique matters
	    if ( (colSetLeft.entries() == 1) &&
                  (mergeMethod != SEMI_JOIN_MERGE) )
	    {
              ValueId colIdLeft;
              colSetLeft.getFirst(colIdLeft);

              // Check to see if it is a join with a tuple list
              // In this case the column type should be ITM_NATYPE

              if ((CmpCommon::getDefault(COMP_BOOL_48) == DF_ON) &&
                  (colIdLeft.getItemExpr()->getOperatorType() == ITM_NATYPE) )
              {
                joinWithTupleList = TRUE;
                // in case of a tuple list, do not do a regular merge of frequent values if
                // there was no frequent value list created. That will be true if the
                // number of elemenst in the IN list for which the tuple list was created
                // had elements less than or equal to CQD HIST_TUPLE_FREQVAL_LIST_THRESHOLD
                // Please refer method addColStatDescForVirtualCol for use of this CQD
                if (rootColStats->getFrequentValues().entries() > 0) 
                  mergeFreqValues = TRUE;
              }

              // join with a tuple list will always be unique as the
              // UEC = rowcount
	      if (joinWithTupleList || rootColStats->isAlmostUnique())
                leftJoinUnique = TRUE;
	      else
	      {
                BaseColumn * colExprLeft = colIdLeft.castToBaseColumn();

                if (colExprLeft != NULL)
                {
                  TableDesc * tableDescForLeftCol = colExprLeft->getTableDesc();
                  leftJoinUnique = colSetLeft.doColumnsConstituteUniqueIndex(tableDescForLeftCol);
                }
              }
            }

	    if (colSetRight.entries() == 1)
	    {
	      ValueId colIdRight;
	      colSetRight.getFirst(colIdRight);
              // Check to see if it is a join with a tuple list
              // In this case the column type should be ITM_NATYPE

              if ((CmpCommon::getDefault(COMP_BOOL_48) == DF_ON) &&
                  (colIdRight.getItemExpr()->getOperatorType() == ITM_NATYPE) )
              {
                joinWithTupleList = TRUE;
                // in case of a tuple list, do not do a regular merge of frequent values if
                // there was no frequent value list created. That will be true if the
                // number of elemenst in the IN list for which the tuple list was created
                // had elements less than or equal to CQD HIST_TUPLE_FREQVAL_LIST_THRESHOLD
                // Please refer method addColStatDescForVirtualCol for use of this CQD
                if (mergedColStats->getFrequentValues().entries() > 0) 
                  mergeFreqValues = TRUE;
              }

              // join with a tuple list will always be unique as the
              // UEC = rowcount
	      if(joinWithTupleList || mergedColStats->isAlmostUnique() )
		    rightJoinUnique = TRUE;
	      else
	      {
		BaseColumn * colExprRight = colIdRight.castToBaseColumn();

		if (colExprRight != NULL)
		{
		  TableDesc * tableDescForRightCol = colExprRight->getTableDesc();
		  rightJoinUnique = colSetRight.doColumnsConstituteUniqueIndex(tableDescForRightCol);
		}
	      }
	    }

	    CostScalar leftUec;
	    CostScalar rightUec;

	    // Fix for Sol:10-070222-2759. The join cardinalities were highly
	    // underestimated. This is because of the assumption the optimizer makes 
	    // regarding the relationship between the joining columns. The joining
	    // columns can be either orthogonal or contained within each other. 
	    // Containment determines if the joining column of the one table
	    // should take into account the reduction on the column from the other side
	    // The way the reduction should be computed is controlled by a CQD
	    // By default we assume that the columns are orthogonal. That is reduction
	    // on one side of a table, should not impact the other side.
	    // CQD used for this is HIST_ASSUME_INDEPENDENT_REDUCTION, 
	    // and is ON by default

	    if (CURRSTMT_OPTDEFAULTS->histAssumeIndependentReduction())
	    {
	      leftUec = rootColStats->getBaseUec();
	      rightUec = mergedColStats->getBaseUec();
              UInt32 upliftCardCond = CURRSTMT_OPTDEFAULTS->histOptimisticCardOpt();
              if ((CmpCommon::getDefault(COMP_BOOL_45) == DF_ON) && 
                  ( (upliftCardCond == 1)  ||
                    (upliftCardCond == 2) ) )
              {
                ValueIdSet joinedCols = this->getColumn();
                joinedCols.insert(mergedStatDesc->getColumn());

                // first get the joining column with minimum UEC
                // CostScalar minOriginalUec = joinedCols.getMinOrigUecOfJoiningCols();
                CostScalar minOriginalUec = MINOF(leftUec, rightUec);

                // next get the UEC of the left and the right joining columns
                CostScalar leftTotalUec = rootColStats->getTotalUec();
                CostScalar rightTotalUec = mergedColStats->getTotalUec();

                // max of left and right UEC to compute join cardinality
                // uses single interval concept and the containment assumption
                //leftUec = MAXOF(minOriginalUec, leftTotalUec);
                //rightUec = MAXOF(minOriginalUec, rightTotalUec);
                leftUec = MAXOF(leftTotalUec, minOriginalUec);
                rightUec = MAXOF(rightTotalUec, minOriginalUec);
              } // upliftCardCond = 1 OR 2
	    } // histAssumeIndependentReduction
	    else
	    {
	      leftUec = rootColStats->getTotalUec();
	      rightUec = mergedColStats->getTotalUec();
	    }

	    if (leftJoinUnique && (joinWithTupleList || 
                                  ((rootColStats->getBaseUec() * uecCushion) > mergedColStats->getBaseUec() ) ) ) 
	    {
	      // result is the otherColStats
	      CostScalar reduction = csOne;
              if (joinWithTupleList)
                reduction = 1 / MAXOF(rightUec, leftUec).getValue();
              else
                reduction = 1/leftUec.value();

	      CostScalar minUec = MINOF(rootColStats->getTotalUec(), mergedColStats->getTotalUec()) ;

              // make a copy of original rootColStats so we can merge frequent values properly later
              // This will be required if we mergeFreqValues flag is TRUE that is we need to do detailed
              // merge of frequent values
              ColStatsSharedPtr rootColStatsCopy;
              if (mergeFreqValues)
                rootColStatsCopy = ColStats::deepCopy(*(rootColStats),HISTHEAP);

              // Over write the rootColStats with the right histogram, which is the resultant
              // histogram now
	      rootColStats->overwrite( *mergedColStats );

              // Overwrite function does not copy frequent value list. I tried t make a copy
              // but that resulted in cardinality changes for outer joins, till we
              // figure out how to handle frequent values for outer joins, we will copy frequent
              // values separately, so that the behaviour of left joins does not change. 
              // Also did not want to change the behavior of short cut join for join on columns, 
              // hence do that only for tuple lists. 
              if (joinWithTupleList && 
                 (rootColStats->getFrequentValues().entries() == 0) && 
                 (mergedColStats->getFrequentValues().entries() > 0))
              {
                FrequentValueList * resultantFreqValList = new (STMTHEAP) 
                  FrequentValueList(mergedColStats->getFrequentValues(), STMTHEAP);
                rootColStats->setFrequentValue(*resultantFreqValList);
              }

	      CostScalar uecReduction = minUec / rootColStats->getTotalUec();

              // do a detailed merge of joins, if the tuple list has frequent values attached to it
              // else simply scale the frequency and the probability of the frequent value list
              // by the row and uec reduction computed
              if (mergeFreqValues)
                // rootColStats is now the rigthColStats, hence merge frequentvalues of right and
                // original left root colstats
                NABoolean dummy = rootColStats->mergeFrequentValues(rootColStatsCopy, scaleFreq);
              else
              {
                FrequentValueList & rootFrequentValueList = rootColStats->getModifableFrequentValues();
                rootFrequentValueList.scaleFreqAndProbOfFrequentValues(reduction, uecReduction);
              }

              // later scale the histogram keeping the frequent values unchanged, as they have already been 
              // scaled
              rootColStats->scaleHistogram(reduction, uecReduction, FALSE);

	      // update the applied predicates
	      appliedPreds().insert( mergedStatDesc->getAppliedPreds() );
	      mergeState().insert(mergedStatDesc->getMergeState() );
	      rootColStats->setRecentJoin(TRUE);
	      rootColStats->setModified (TRUE) ;
              skipJoin = TRUE;
	    }
	    else
	    {
	      if (rightJoinUnique && (joinWithTupleList || 
                                      (( mergedColStats->getBaseUec() * uecCushion) > rootColStats->getBaseUec() ) ))
	      {
		// final result is this colStats, hence not much to do
		// except scale this colstats to take care of cross product
		// that had taken place earlier
		// For semi-joins, the left and the rigth histograms are not scaled
		// at the time of merge, hence, no scaling is required here too

		CostScalar reduction = csOne;

                // if it is join with tuple list, reduction is equal to the 
                // larger of the two bases UECs
                if (joinWithTupleList)
                  reduction = 1 / MAXOF(rightUec, leftUec).getValue();
                else
                {
		  if (mergeMethod != SEMI_JOIN_MERGE)
		    reduction = 1/rightUec.value();
		  else
		    reduction = mergedColStats->getTotalUec()/mergedColStats->getBaseUec().value();
                }

		CostScalar minUec = MINOF(rootColStats->getTotalUec(), mergedColStats->getTotalUec()) ;
		CostScalar uecReduction = minUec / rootColStats->getTotalUec();

              // merge the frequent values of the two sides if the heuristic used above is TRUE else
              // just scale the probability of the frequent values, keeping frequency unchanged
              // This is similar to applying direct reduction to histogram intervals
              if (mergeFreqValues)
                NABoolean dummy = rootColStats->mergeFrequentValues(mergedColStats, scaleFreq);
              else
              {
                FrequentValueList & rootFrequentValueList = rootColStats->getModifableFrequentValues();
                rootFrequentValueList.scaleFreqAndProbOfFrequentValues(reduction, uecReduction);
              }

              // later scale the histogram keeping the frequent values unchanged, as they have already been 
              // scaled
		rootColStats->scaleHistogram(reduction, uecReduction, FALSE);

                appliedPreds().insert( mergedStatDesc->getAppliedPreds() );
		mergeState().insert(mergedStatDesc->getMergeState() );
		rootColStats->setRecentJoin(TRUE);
		rootColStats->setModified (TRUE) ;
		skipJoin = TRUE;
	      } // rightJoinUnique
	    } // end else leftJoinUnique
      } // histogram is !fake and !histForVirtualCol
    } // forceMerge = FALSE && mergeMethod == INNER_JOIN or SEMI_JOIN

    if (forceMerge == TRUE || skipJoin == FALSE)
    {
      // ----------------------------------------------------------------
      // merge the two column statistics
      // ----------------------------------------------------------------
      NABoolean isNumeric = FALSE; //variable used to indicate if datatype is numeric

      if(CURRSTMT_OPTDEFAULTS->reduceIntermediateHistograms())
	if(getVEGColumn()){
			isNumeric = getVEGColumn().getType().isNumeric();
	}
	else if (getColumn())
	{
			isNumeric = getColumn().getType().isNumeric();
		};

      rootColStats->mergeColStats( mergedColStats, mergeMethod, isNumeric, 
                                   exprOpCode, mergeFVs);

      // update the mergeState_ of the root copy.
      mergeState().insert( mergedStatDesc->getMergeState() );

      // and its applied predicates
      appliedPreds().insert( mergedStatDesc->getAppliedPreds() );
    } // if (forceMerge == TRUE || skipJoin == FALSE)
  } // not UNION or left join
  else  // It's a UNION, OR or LEFT_JOIN_OR.
  {
    // --------------------------------------------------------------------
    // We do the unusual case second :
    //        merges that are UNIONs, ORs, or LEFT_JOIN_ORs
    //        (i.e., "non-join" merges)
    // --------------------------------------------------------------------
    // For non-join merges, an entry should remain in the nonVegEquals_
    // ColStatDesc list of the result only if it appears in both of the
    // nonVegEquals_ involved in the merge.
    // --------------------------------------------------------------------
    CollIndex j = 0;

    while ( j < nonVegEquals_.entries() )
    {
      ColStatDescSharedPtr tmpDescJ = nonVegEquals_[j];
      NABoolean foundFlag = FALSE;

      for ( CollIndex i = 0;
	    i < mergedStatDesc->nonVegEquals_.entries() && !foundFlag;
	    i++ )
      {
	ColStatDescSharedPtr tmpDescI = mergedStatDesc->nonVegEquals_[i];
	if ( tmpDescI->VEGcolumn_ == tmpDescJ->VEGcolumn_ )
	  foundFlag = TRUE;
      } // for i

      if ( foundFlag == FALSE )
      {
	nonVegEquals_.removeAt( j );
      }
      else
      {
	j++;
      }
    } // while j

    // ----------------------------------------------------------------
    // merge the two column statistics
    // ----------------------------------------------------------------
    NABoolean isNumeric = FALSE; //variable used to indicate if datatype is numeric

	if(CURRSTMT_OPTDEFAULTS->reduceIntermediateHistograms())
		if(getVEGColumn()){
			isNumeric = getVEGColumn().getType().isNumeric();
		}
		else if (getColumn())
		{
			isNumeric = getColumn().getType().isNumeric();
		};

    rootColStats->mergeColStats( mergedColStats, mergeMethod, isNumeric, 
                                 exprOpCode, mergeFVs);

    // And update the mergeState_ and applied predicates of the root copy.
    if ( mergeMethod == UNION_MERGE ||
	 mergeMethod == OR_MERGE )
    {
      // -------------------------------------------------------------
      // An OR's or a UNION's result, should only indicate that a
      // predicate has been applied when it has been applied to both
      // sides of the OR or the UNION.
      //
      // This restriction impacts both the result's mergeState_ and
      // its Applied Predicate set.
      // -------------------------------------------------------------
      mergeState().intersectSet( mergedStatDesc->getMergeState() );
      appliedPreds().intersectSet( mergedStatDesc->getAppliedPreds() );
    }
    else
    {
      mergeState().insert( mergedStatDesc->getMergeState() );
      appliedPreds().insert( mergedStatDesc->getAppliedPreds() );
    }
  }
  if ((exprOpCode == REL_JOIN) && (rootColStats->getRowcount() < minCard) )
    rootColStats->setRowsAndUec(minCard, rootColStats->getTotalUec());

} // ColStatDesc::mergeColStatDesc

void
ColStatDescList::setScaleFactor(CostScalar val)
{
  if (CmpCommon::getDefault(COMP_BOOL_42) == DF_OFF)
    return;

  for ( CollIndex i = 0; i < entries(); i++ )  {
    (*this)[i]->getColStatsToModify()->setScaleFactor(val);
  }
}

// ------------------------------------------------------------------------
// ColStatDesc::derivOfLikeAndSimilarPredApp
//
// This method is used in modifyStats (during applyPred) and
// applyDefaultPred.
//
// It is used for range predicates derived from LIKE predicate. This method
// returns TRUE if the range predicate is a derivative of LIKE predicate
// and the first range predicate derived from LIKE has already been
// applied to this column's histogram
// ------------------------------------------------------------------------

NABoolean
ColStatDesc::derivOfLikeAndSimilarPredApp(const ItemExpr * predApplied )
{
  // get the operator for the predicate
  OperatorTypeEnum op = predApplied->getOperatorType();

  // check to see if this is a derivative of LIKE predicate
  NABoolean predDerivOfLike = FALSE;

  if ( (op == ITM_GREATER_EQ) OR
	   (op == ITM_GREATER) OR
	   (op == ITM_LESS) OR
	   (op == ITM_LESS_EQ))
  {
    BiRelat *br = (BiRelat *) predApplied;
    predDerivOfLike = br->derivativeOfLike();
	if (! predDerivOfLike)
	  return FALSE;
  }

  for ( ValueId id = appliedPreds_.init();
        appliedPreds_.next( id );
        appliedPreds_.advance( id ) )
    {
      const ItemExpr *pred = id.getItemExpr();
      OperatorTypeEnum appliedOp = pred->getOperatorType();

      if ( (appliedOp == ITM_GREATER_EQ) OR
		   (appliedOp == ITM_GREATER) OR
		   (appliedOp == ITM_LESS) OR
		   (appliedOp == ITM_LESS_EQ) )
      {
            BiRelat *br = (BiRelat *) pred;
            if (predDerivOfLike && br->derivativeOfLike())
			{
			  getColStatsToModify()->setFakeHistogram();
              return TRUE;
			}
      }
	}
	return FALSE;
}


// ------------------------------------------------------------------------
// ColStatDesc::isSimilarPredicateApplied
//
// Used to avoid redundant predicate application against 'fake' ColStats,
// this routine determines whether or not THIS's appliedPreds_ contains a
// predicate 'similar' to its input OperatorTypeEnum op.
//
// Currently, the only non-identical 'similar' predicates are the >, >=
// and <, <= predicate pairs.
// ------------------------------------------------------------------------
NABoolean
ColStatDesc::isSimilarPredicateApplied (const OperatorTypeEnum op) const
{
  for ( ValueId id = appliedPreds_.init();
	appliedPreds_.next( id );
	appliedPreds_.advance( id ) )
    {
      const ItemExpr *pred = id.getItemExpr();
      OperatorTypeEnum appliedOp = pred->getOperatorType();

      switch ( op )
        {
         case ITM_LESS:
         case ITM_LESS_EQ:
           if ( appliedOp == ITM_LESS || appliedOp == ITM_LESS_EQ )
             return TRUE;
           else
             break ;

         case ITM_GREATER:
         case ITM_GREATER_EQ:
           if ( appliedOp == ITM_GREATER || appliedOp == ITM_GREATER_EQ )
             return TRUE;
           else
             break;

         default:
           if ( appliedOp == op )
             return TRUE;
        }
    }
  return FALSE;
}


// ------------------------------------------------------------------------
// ColStatDesc::selForRelativeRange
//
// this routine determines whether or not THIS's appliedPreds_ contains a
// predicate opposite in range to its input OperatorTypeEnum op.
//
// Currently, the only opposite range predicates are the >, >=
// and <, <= predicate pairs.
//
// This returns the value of the constant (adjusted for >= and <=) over
//    high - low + 1
//
// A return of 1 indicates that this routine could not calculate the selectivity
// ------------------------------------------------------------------------
CostScalar
ColStatDesc::selForRelativeRange (const OperatorTypeEnum op,
                                  const ValueId & column,
                                  ItemExpr *newPred) const
{


  CostScalar sel = csOne;
  OperatorTypeEnum appliedOp = NO_OPERATOR_TYPE;
  ItemExpr *savePred = NULL;

  // Return the selctivity as csOne if it is a fake histograms. Then the
  // default selectivity will be applied.
  if ( getColStats()->isFakeHistogram() )
    return csOne;

  // This algorithm only works for numeric and datetime types.
  if ( NOT (( column.getType().getTypeQualifier() == NA_NUMERIC_TYPE ) ||
	        ( column.getType().getTypeQualifier() == NA_DATETIME_TYPE)))
    return csOne;

  //  Go through all the predicates we have already applied, looking for
  //   the second part of a between type operation.
  //  One of the predicates must be (>, or >=) and the other (<, or <=)
  for ( ValueId id = appliedPreds_.init();
        appliedPreds_.next (id);
	appliedPreds_.advance (id) )
  {
    ItemExpr *pred = id.getItemExpr();
    appliedOp = pred->getOperatorType();

    switch (op)
    {
    case ITM_LESS:
    case ITM_LESS_EQ:
      if ( appliedOp == ITM_GREATER || appliedOp == ITM_GREATER_EQ )
	savePred=pred;
      else
	break ;

    case ITM_GREATER:
    case ITM_GREATER_EQ:
      if ( appliedOp == ITM_LESS || appliedOp == ITM_LESS_EQ )
	savePred=pred;
      else
	break;

    default:
      if ( appliedOp == op )
	break;
    }
  }


  if ( savePred )  //  if we have found a between type of operation - do more
  {
    // look for common host variables and constant with +/- with a constant
    ItemExpr * saveChildPred = savePred->child(1);

    //Use the actual node hiding behind the cast or the notCovered node.
    if ((saveChildPred->getOperatorType() == ITM_CAST) ||
	(saveChildPred->getOperatorType() == ITM_NOTCOVERED) )
      saveChildPred = saveChildPred->child(0);

    CollIndex newPredArity  = newPred->getArity();
    CollIndex savePredArity = saveChildPred->getArity();
    OperatorTypeEnum newOp  = newPred->getOperatorType();
    OperatorTypeEnum saveOp = saveChildPred->getOperatorType();

    // We need to have one predicate compare the column to a stand alone
    // host variable and the other predicate to compare the column to a
    // host variable +/- a constant.
    if( NOT (	(     newPredArity == 0
		  AND ( saveOp == ITM_PLUS OR saveOp == ITM_MINUS )
		)
	     OR (     savePredArity == 0
		  AND ( newOp == ITM_PLUS OR newOp == ITM_MINUS )
		)
	    )
      )
    {
      return csOne;
    }

    // Now normalize to the two predicate side and the one predicate side
    ItemExpr * onePred;
    ItemExpr * twoPred;
    ItemExpr * hostVar = NULL;
    ConstValue  * constant = NULL;
    double cn;
    OperatorTypeEnum twoOp;
    OperatorTypeEnum origTwoOp;
    NABoolean def = FALSE;

    if ( newPredArity == 0 )
    {
      onePred = newPred;
      twoPred = saveChildPred;
      twoOp = saveOp;
      origTwoOp = appliedOp;
    }
    else
    {
      onePred = saveChildPred;
      twoPred = newPred;
      twoOp = newOp;
      origTwoOp = op;
    }

    // Identify which is the constant and which the host variable
    //  If this is a more complicated expression, (has more than 1 operator)
    //   we will not find either the constant or host variable.
    for ( Int32 arity = 0; arity < twoPred->getArity(); arity++ )
    {
      ItemExpr * operand = (*twoPred)[arity].getPtr();
      //Use the actual node hidding behind the cast / not covered node.
      if ( (operand->getOperatorType() == ITM_CAST) ||
	   (operand->getOperatorType() == ITM_NOTCOVERED) )
	operand = operand->child(0);


      //Check for all operators types that stay constant
      //for a particular execution of a statement
      if (    ( operand->getOperatorType() == ITM_HOSTVAR )
	   OR ( operand->getOperatorType() == ITM_DYN_PARAM )
	   OR (operand->getOperatorType() == ITM_CACHE_PARAM)
	   OR (operand->getOperatorType() == ITM_CURRENT_USER)
	   OR (operand->getOperatorType() == ITM_CURRENT_TIMESTAMP)
	   OR (operand->getOperatorType() == ITM_SESSION_USER)
	   OR (operand->getOperatorType() == ITM_GET_TRIGGERS_STATUS)
	   OR (operand->getOperatorType() == ITM_UNIQUE_EXECUTE_ID))
	hostVar = operand;

      //changed from
      /*
      if ( operand->getOperatorType() == ITM_CONSTANT )
		constant = operand->castToConstValue( def );
	  */
	  //changed to
	  //the two are almost equivalent,
	  //castToConstValue is virtual method
	  //on ItemExpr and class derived from it
	  //the base implementation always returns
	  //NULL
	  //The above lines of code can only cover
	  //constant literals e.g. 2. But the following
	  //lines of code also cover simple expressions
	  //like 1+3, 2-2. That is because castToConstValue
	  //is overloaded for BiArithmetic expressions.
	  //A non-null return value from castToConstValue
	  //indicates a constant or simple constant expression, and
	  //therefore we use the return value from castToConstValue
	  //to check for constants.
      ConstValue * isAConstant = operand->castToConstValue(def);
      if(isAConstant)
	constant = isAConstant;
    }

    // COLUMN <op> constant predicate?
    // if so, does column match the leading prefix of histogram?
    if ( constant != NULL )
    {
      // get the encoded format for the constant
	  //use double cn instead.
      EncodedValue encodedConst = EncodedValue( constant, def );
      cn = encodedConst.getDblValue();

      // we want to convert Interval into seconds from what its present
      // type.
      const NAType * constantType = constant->getType();
      if (constantType->getTypeQualifier() == NA_INTERVAL_TYPE)
	    cn = ((SQLInterval *)constantType)->getValueInSeconds(cn);
    }
    else
      return csOne;

    if ( onePred != hostVar)   // if both predicates don't refer to same hv.
      return csOne;

    //  If the range is impossible - we have zero selectivity
    if (    (	  twoOp == ITM_PLUS
	      AND ( origTwoOp== ITM_GREATER_EQ OR origTwoOp == ITM_GREATER )
	    )
	 OR (	  twoOp == ITM_MINUS
	      AND ( origTwoOp == ITM_LESS_EQ   OR origTwoOp == ITM_LESS )
	    )
       )
      return csZero;

    EncodedValue minVal = getColStats()->getMinValue();
    EncodedValue maxVal = getColStats()->getMaxValue();


    double hi = maxVal.getDblValue();
    double lo = minVal.getDblValue();

    // calculate the multiplier we will use for scaling some constants we want to add
    //   to the hi/lo/cn.
    double scaleMult = 1;
    Lng32 scale = column.getType().getScale();
    for ( Lng32 i = 0; i < scale; i++ )
    {
      if ( scale > 0 )
	scaleMult = scaleMult / 10.0;
      else
	scaleMult = scaleMult * 10.0;
    }

    // Since the ration function always calculates  (cn-lo)/(hi-lo).
    //    it doesn't add one to hi-lo to get the correct range.
    // Ensure that the calculation is accurate for small ranges - avoiding overflow.

    if ( lo > 0 )
      lo = lo - scaleMult;
    else if ( hi < ( scaleMult * 1000000 ) )
      hi = hi + scaleMult;
    minVal = EncodedValue( lo );
    maxVal = EncodedValue( hi );

    // Adjust the constant value for >= and <=, each of which add 1 to range
    double rangeInc = -1;  //(-1) since we will add lo+cn, subtract out extra value.
    if ( op == ITM_GREATER_EQ || op == ITM_LESS_EQ )
      rangeInc += 1;
    if ( appliedOp == ITM_GREATER_EQ || appliedOp == ITM_LESS_EQ )
      rangeInc += 1;

    rangeInc = rangeInc * scaleMult;

    if ( lo + cn + rangeInc  > hi )
      return csOne;

    // the ratio function uses (val-lo) to get the numerator
    //   and (hi-lo) for the denominator while avoiding overflow
    EncodedValue testVal = EncodedValue( lo + cn + rangeInc );
    sel = testVal.ratio( minVal, maxVal );
    if ( sel.getValue() < 0.0 || sel.getValue() > csOne.getValue() )
      return csOne;
    CostScalar defaultSel =
      CURRSTMT_OPTDEFAULTS->defSelForRangePred();

    // For window predicates like col between ? and ?+const,
    // (it gets converted as col >= ? and col <= ?+const).
    // We come to this point for the second predicate. i.e for col <= ?+const.
    // The selectivity for the first predicate col >= ? is already applied,
    // which is the default selectivity.
    // So here make sure the selectivity will not go beyond defaultSel.

    // The caller of this method (applyDefaultPred) will call applySel to apply
    // the selectivity. Which in a way multiplies the selectivity for the
    // predicates. In this particular case, since it is a window predicate and
    // the selectivity should be the selectivity of the window we just
    // calculated, we should not multiply the selectivities.
    // Since the multiplication happens unconditionally in the caller, divide
    // it by the defaultSel.
    // If the selectivity is equal to defaultSel return 0.999999 (if we
    // return 1 (csOne) then it is considered as fakeHistogram and again
    // the defalutSel will be applied.
    if (sel >= defaultSel)
      return 0.999999;
    else
      sel = sel / defaultSel;

    // Sanity check to make sure the cardinality did not go below average rowcount.
    CostScalar selForAvgRowcount = 1/getColStats()->getUecBeforePreds().getValue();
    if(sel < selForAvgRowcount)
      sel = selForAvgRowcount;
  }
  return sel;
}

void
ColStatDesc::print (FILE *ofd,
                    const char * prefix,
                    const char * suffix,
                    CollHeap *c, char *buf,
                    NABoolean hideDetail) const
{
  Space * space = (Space *)c;

  char mybuf[1000];

  if (!hideDetail)
  {
    snprintf(mybuf, sizeof(mybuf), "%scolumn:", prefix);
    PRINTIT(ofd, c, space, buf, mybuf);
    ValueIdList columns; 
    columns.insert(column_); 
    columns.print(ofd, prefix, suffix, c, buf);
  }
  snprintf(mybuf, sizeof(mybuf), "%sVEGcolumn:", prefix) ;
  PRINTIT(ofd, c, space, buf, mybuf);
  ValueIdList VEGcolumns; 
  VEGcolumns.insert(VEGcolumn_); 
  VEGcolumns.print(ofd, prefix, suffix, c, buf) ;
  
  snprintf(mybuf, sizeof(mybuf), "%sMerge state:", prefix) ;
  PRINTIT(ofd, c, space, buf, mybuf);
  mergeState_.print(ofd, prefix, suffix, c, buf) ;

  snprintf(mybuf, sizeof(mybuf), "%sApplied preds:", prefix) ;
  PRINTIT(ofd, c, space, buf, mybuf);
  appliedPreds_.print(ofd, prefix, suffix, c, buf) ;

  if (getColStats() == NULL)
  {
    snprintf(mybuf, sizeof(mybuf), "NULL colStats_!\n");
    PRINTIT(ofd, c, space, buf, mybuf);
  }
  else
  {
    getColStats()->print(ofd,prefix,suffix, c, buf, hideDetail);
  }
}

void
ColStatDesc::display() const
{
  print();
}

// ------------------------------------------------------------------------
//  Methods for ColStatDescList Class
// ------------------------------------------------------------------------

// some users of the CSDL class want to be able to destroy the temporary
// objects they create

void ColStatDescList::destroy()
{
  while ( entries() > 0 )
    removeDeepCopyAt( 0 );
}

// Returns TRUE if the given ColStatDesc is contained in this
// ColStatDescList. The ColStatDesc could have also been merged
// with the other colStatDesc

NABoolean
ColStatDescList::contains(const ValueId & column) const
{
  for ( CollIndex i = 0; i < entries(); i++ )
  {
    // The "merge state" of a ColStatDesc indicates all the
    // columns that have been merged into this ColStatDesc.
    // Initially, the merge state consists of the original
    // base table column, therefore, this will work even for
    // ColStatDesc's that have not been merged
    const ValueIdSet & msSet = (*this)[i]->getMergeState();
    if ( msSet.contains( column ) )
      return TRUE;
  }

  return FALSE;
}

// Returns TRUE if the given ColStatDesc is contained in this
// ColStatDescList. The ColStatDesc could have also been merged
// with the other colStatDesc

NABoolean
ColStatDescList::contains(const ValueIdList & colList) const
{
  ColStatsSharedPtr colStats;
  for(CollIndex i =0; i<colList.entries(); i++)
  {
    colStats = this->getColStatsPtrForColumn(colList[i]);

    // get the histogram pointer for the column in list 
    if (colStats == NULL)
    {
      // if the histogram is missing for this column
      return FALSE;
    }
  }
  return TRUE;
}

NABoolean
ColStatDescList::containsAtLeastOneFake() const
{

  // -----------------------------------------------------------------------
  // Go through the list of ColStatDesc and if you find
  // a fake ColStats set the valid flag to FALSE and return:
  // -----------------------------------------------------------------------

  NABoolean thereIsAFake = FALSE;
  for ( CollIndex i = 0; i < entries(); i++ )
  {
    if ( (*this)[i]->getColStats()->isFakeHistogram() )
    {
      thereIsAFake = TRUE;
      break;
    }
  }

  return thereIsAFake;
} // ColStatDescList::containsAtLeastOneFake() const

NABoolean
ColStatDescList::selectivityHintApplied() const
{
  for ( CollIndex i = 0; i < entries(); i++ )
  {
    if ( (*this)[i]->getColStats()->isSelectivitySetUsingHint() )
      return TRUE;
  }
  return FALSE;
}

// ---------------------------------------------------------------------
// Methods for doing Deep Copies of the ColStatDescs whose pointers are
// inserted into a ColStatDescList.
// In the various routines,
// 'firstN' specifies that only the first N entries in the source are to
//    be inserted.
// 'scale' specifies the factor by which the RowCounts (not UECs) should
//    be multiplied.
// 'shapeChangedMask' is AND'd with the current setting of the shape-
//    changed flag, allowing it to be either left alone (the default) or
//    cleared.
// ---------------------------------------------------------------------

// Macro to make a CSD copy and set it up correctly.
#define ALLOCATE_COL_STAT_DESC_AND_SET_IT_UP( x ) \
  ColStatDescSharedPtr tmpColStatDescPtr(new (HISTHEAP) ColStatDesc( x ), HISTHEAP); \
  ColStatsSharedPtr tmpColStatsPtr = tmpColStatDescPtr->getColStatsToModify(); \
  tmpColStatsPtr->copyAndScaleHistogram( scale ); \
  tmpColStatsPtr->setShapeChanged( \
    shapeChangedMask && tmpColStatsPtr->isShapeChanged() ); 

void
ColStatDescList::makeDeepCopy (const ColStatDescList & source,
                               const CostScalar & scale,
                               const NABoolean shapeChangedMask)
{
  for ( CollIndex i = 0; i < source.entries(); i++ )
  {
    ALLOCATE_COL_STAT_DESC_AND_SET_IT_UP( *source[i] );
    insertAt( i, tmpColStatDescPtr );
  }

  setUecList( source.getUecList() );
} // makeDeepCopy

void
ColStatDescList::appendDeepCopy (const ColStatDescList & source,
                                 const CollIndex firstN,
                                 const CostScalar & scale,
                                 const NABoolean shapeChangedMask)
{
  CollIndex thisEntries = entries();

  for ( CollIndex i = 0; i < source.entries() && i < firstN; i++ )
  {
    ALLOCATE_COL_STAT_DESC_AND_SET_IT_UP( *source[i] );
    insertAt( thisEntries, tmpColStatDescPtr );
    thisEntries++;
  }

  // add the other CSDL's uec list to this one
  insertIntoUecList( source.getUecList() );
} // appendDeepCopy

void
ColStatDescList::prependDeepCopy (const ColStatDescList & source,
                                  const CollIndex firstN,
                                  const CostScalar & scale,
                                  const NABoolean shapeChangedMask)
{
  for ( CollIndex i = 0; i < source.entries() && i < firstN; i++ )
  {
    ALLOCATE_COL_STAT_DESC_AND_SET_IT_UP( *source[i] );
    insertAt( i, tmpColStatDescPtr );
  }

  // add the other CSDL's uec list to this one
  insertIntoUecList( source.getUecList() );
} // prependDeepCopy

void
ColStatDescList::insertDeepCopy (const ColStatDescSharedPtr& source,
                                 const CostScalar & scale,
                                 const NABoolean shapeChangedMask)
{
  ALLOCATE_COL_STAT_DESC_AND_SET_IT_UP( *source );
  insert( tmpColStatDescPtr );
} // insertDeepCopy

void
ColStatDescList::insertDeepCopyAt (const CollIndex entry,
                                   const ColStatDescSharedPtr& source,
                                   const CostScalar & scale,
                                   const NABoolean shapeChangedMask)
{
  ALLOCATE_COL_STAT_DESC_AND_SET_IT_UP( *source );
  insertAt( entry, tmpColStatDescPtr );
} // insertDeepCopyAt

void
ColStatDescList::makeMappedDeepCopy (const ColStatDescList & source,
                                     ValueIdMap &map,
                                     NABoolean includeUnmappedColumns)
{
  // similar to makeDeepCopy, but this method maps all the ValueIds in
  // the source, using the provided map, in the "up" direction
  for ( CollIndex i = 0; i < source.entries(); i++ )
    {
      ValueId topVid;
      ValueId bottomVid(source[i]->getVEGColumn());

      map.mapValueIdUp(topVid, bottomVid);

      if (includeUnmappedColumns || topVid != bottomVid)
        {
          ColStatDescSharedPtr tmpColStatDescPtr(
               new (HISTHEAP) ColStatDesc(), HISTHEAP);
          tmpColStatDescPtr->mapUpAndCopy(*source[i], map);
          ColStatsSharedPtr tmpColStatsPtr = tmpColStatDescPtr->getColStatsToModify();
          tmpColStatsPtr->copyAndScaleHistogram(1.0);
          tmpColStatsPtr->setShapeChanged(tmpColStatsPtr->isShapeChanged()); 

          insert(tmpColStatDescPtr);
        }
    }

  // map the multi-column UECs
  MultiColumnUecList * mappedMultiColUECs = new (CmpCommon::statementHeap())
      MultiColumnUecList();

  mappedMultiColUECs->insertMappedList(source.getUecList(), map);
  setUecList(mappedMultiColUECs);
} // makeMappedDeepCopy

void
ColStatDescList::removeDeepCopyAt (const CollIndex entry)
{
  removeAt( entry );
} // removeDeepCopyAt

// -------------------------------------------------------------------
// copyAndScaleHistogram
// This method goes thru each colStatDesc in the list and scales the
// underlying histogram based on the scale and the row reduction factor
// and uec reduction factors specified in the colStat header.
// --------------------------------------------------------------------

void
ColStatDescList::copyAndScaleHistograms(CostScalar scale)
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    ColStatDescSharedPtr columnStatDesc = (*this)[i];
    ColStatsSharedPtr columnStats = columnStatDesc->getColStatsToModify();
    columnStats->scaleHistogram(scale);
    columnStats->computeMaxFreqOfCol(!(CmpCommon::getDefault(COMP_BOOL_42)));
  }
}

void 
ColStatDescList::setBaseUecToTotalUec()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    ColStatDescSharedPtr columnStatDesc = (*this)[i];
    ColStatsSharedPtr columnStats = columnStatDesc->getColStatsToModify();
    columnStats->setBaseUec(columnStats->getTotalUec());
  }
}

void
ColStatDescList::computeMaxFreq(NABoolean forced)
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    ColStatDescSharedPtr columnStatDesc = (*this)[i];
    ColStatsSharedPtr columnStats = columnStatDesc->getColStatsToModify();
    columnStats->computeMaxFreqOfCol(forced);
  }
}

// add colStatDesc for a constant in this colStatDescList.
// This will be used for cases like inserts, transpose or rowsets.
// Since there are no columns associated with these histograms
// they are treated in a special manner. These are distinguished
// from regular histograms by means of a flag virtualColForHist_

void ColStatDescList::addColStatDescForVirtualCol(const CostScalar & uec,
                                                  const CostScalar & rowCount,
                                                  const ValueId  colId,
                                                  const ValueId vegCol,
                                                  const ValueId mergeState,
                                                  const RelExpr * expr,
                                                  NABoolean defineVirtual)
{
  // create a frequent value list
  FrequentValueList * frequentValueList = new (STMTHEAP) FrequentValueList(STMTHEAP);
  NABoolean fakeHist = TRUE;
  EncodedValue minValue = UNINIT_ENCODEDVALUE;
  EncodedValue maxValue = UNINIT_ENCODEDVALUE;

  // If the histogram is being created for a tuple expr create a frequent value list 
  // with tuple expressions.
  if ((defineVirtual == FALSE) && 
       expr && 
       (expr->getOperatorType() == REL_TUPLE_LIST) )
  {
    fakeHist = FALSE;
    ItemExprList tList(((Tuple *) expr)->tupleExprTree(), STMTHEAP);
    
    if (tList.entries() <= CURRSTMT_OPTDEFAULTS->histTupleFreqValListThreshold())
    {
      for (CollIndex i = 0; (i < (CollIndex) tList.entries());i++)
      {
	ItemExpr * tupleVal = 
	  ((ItemExpr *) tList[i])->castToItemExpr();
        if (tupleVal == NULL)
          continue;

        ValueIdSet leafs;
        tupleVal->getLeafValueIds(leafs);

        if (leafs.entries() == 0)
          continue;

        for (ValueId val = leafs.init();
            leafs.next(val);
            leafs.advance(val))
        {
          if (val == NULL_VALUE_ID)
            continue;

          ItemExpr * leafVal = val.getItemExpr();

	  if (leafVal->getOperatorType() == ITM_CONSTANT)
	  {
            // add the constant to the frequent value list
            NABoolean neg = FALSE;
            ConstValue * cv = leafVal->castToConstValue(neg);
            if (cv == NULL) continue;
            EncodedValue ev(cv, neg);
            UInt32 hashValue = 0;
            if (cv->getType()->useHashRepresentation() &&
                cv->getType()->useHashInFrequentValue())
              hashValue = cv->computeHashValue(*(cv->getType()));
            FrequentValue newV(hashValue, csOne, csOne, ev);
            frequentValueList->insertFrequentValue(newV);
	  } // if leafVal is a constant
        } // for all leaf values. 
      } // for all elements in the IN list
    } // if number of elements in IN list is less than the threshold
  } // if the histogram is being created for tuple_list

    // finally get the in and the max values from the frequent value list
    // these will be used to set the min and the max values of the histograms
  // if frequentValueList is empty, maxValue = number of values in the tuple list
    if (frequentValueList->entries() > 0)
    {
      minValue = (*frequentValueList)[0].getEncodedValue();
      maxValue = (*frequentValueList)[frequentValueList->entries() - 1].getEncodedValue();
    }
    else
    {
      maxValue = uec.getValue();
    }

  HistogramSharedPtr emptyHist(new (HISTHEAP) Histogram(HISTHEAP));
  HistInt newFirstHistInt(minValue, FALSE);
  newFirstHistInt.setCardAndUec(0, 0);

  HistInt newSecondHistInt(maxValue, FALSE);
  newSecondHistInt.setCardAndUec(rowCount, uec);

  emptyHist->insert(newFirstHistInt);
  emptyHist->insert(newSecondHistInt);

  ComUID id(ColStats::nextFakeHistogramID());
  ColStatsSharedPtr fakeColStats(
                        new (HISTHEAP) ColStats(id,
                        uec,
                        rowCount,
                        rowCount,
                        FALSE,
                        FALSE,
                        emptyHist,
                        FALSE,
                        1.0,
                        1.0,
                        -1,         // default avg VarChar size
                        HISTHEAP));

    // This histogram is not an actual histogram
  fakeColStats->setFakeHistogram( fakeHist);
  fakeColStats->setFrequentValue(*frequentValueList);

  // Setting this flag will ensure that the compiler does not start
  // to look for this column name in the NAColumn. As there does not
  // exist a column for a constant
  fakeColStats->setVirtualColForHist ( defineVirtual );

  fakeColStats->setIsCompressed (TRUE);

  fakeColStats->setRowsAndUec(rowCount, uec);
  if (rowCount == uec)
  {
    fakeColStats->setAlmostUnique(TRUE);
    fakeColStats->setUnique(TRUE);
  }

  ColStatDescSharedPtr fakeStatDesc(new (HISTHEAP)
                 ColStatDesc (fakeColStats,colId), HISTHEAP);

  fakeStatDesc->VEGColumn() = vegCol;

  fakeStatDesc->mergeState().clear() ;
  fakeStatDesc->mergeState().insert(mergeState);

  this->insert(fakeStatDesc);
} // ColStatDescList::addColStatDescForVirtualCol

// ---------------------------------------------------------------------
// isPredTransformedInList
//
// subroutine of CSDL::estimateCardinality()
//
// NB: It is extremely important that this routine be tail-recursive;
// otherwise it is sure to overflow the stack for large IN-lists, which is
// the motivation for writing this code in the first place!
// ---------------------------------------------------------------------

NABoolean isPredTransformedInList( const ValueId & predId,
				   const ValueId & column,
				   Int32 & leaves )
{
  const ItemExpr * pred = predId.getItemExpr();
  OperatorTypeEnum op = pred->getOperatorType();

  if ( op != ITM_OR )
    return FALSE;

  // first make sure the right child conforms

  const ValueId & Rid = pred->child(1)->getValueId();
  const ItemExpr * Rpred = Rid.getItemExpr();
  const OperatorTypeEnum Rop = Rpred->getOperatorType();

  if ( Rop != ITM_EQUAL )
    return FALSE;

  const ValueId & RidLeftChild = Rpred->child(0)->getValueId();

  if ( RidLeftChild != column )
    return FALSE;

  // now make sure the left child conforms

  const ValueId & Lid = pred->child(0)->getValueId();
  const ItemExpr * Lpred = Lid.getItemExpr();
  const OperatorTypeEnum Lop = Lpred->getOperatorType();

  // base case : an ITM_OR between two ITM_EQUALs
  if ( Lop == ITM_EQUAL )
  {
    const ValueId & LidLeftChild = Lpred->child(0)->getValueId();

    leaves += 2;

    if ( LidLeftChild == column )
      return TRUE;
    else
      return FALSE;
  }
  else if ( Lop == ITM_OR ) // recursive case -- right child OK, now recurse down left child
  {
    leaves++;
    return isPredTransformedInList( Lid, column, leaves );
  }
  else
    return FALSE;
}

ULng32 ValueIdHashFn (const ValueId & key) { return (CollIndex) key ; }

// ------------------------------------------------------------------------
// $$$$ NOTE:
// The method ColStatDescList::estimateCardinality() can, as a side effect,
// update the unique entry count in the supplied ColStatsDescList.
//
// Also, use of ColStatDescList::applyVEGPred, or ColStatDescList::applyPred
// can cause the number of column stats for outer references to be decreased,
// altering the in/out parameter numOuterColStats.
// ------------------------------------------------------------------------

CostScalar
ColStatDescList::estimateCardinality (const CostScalar & initialRowCount,
                                      const ValueIdSet & setOfPredicates,
                                      const ValueIdSet & outerReferences,
                                      const Join * expr,
				      const SelectivityHint * selHint,
				      const CardinalityHint * cardHint,
                                      CollIndex & numOuterColStats,
                                      ValueIdSet & unresolvedPreds,
                                      MergeType mergeMethod,
                                      OperatorTypeEnum exprOpCode,
                                      CostScalar *maxSelectivity)
{
  // -----------------------------------------------------------------------
  //   1) estimateCardinality is computing expected cardinality by 
  //      implicitly computing the composite selectivity of a conjunct of 
  //      predicate terms: 
  //        p1 AND p2 AND ... pn 
  //      as the product of the individual selectivities: 
  //        selectivity(p1 AND p2 AND ... pn) = 
  //        selectivity(p1) * selectivity(p2) * ... selectivity(pn)
  //   2) this computation is diffused across multiple methods which 
  //      routinely modify histograms and/or sometimes directly manipulate 
  //      a rowcount argument.
  //   3) estimateCardinality assumes it is computing expected cardinality
  //      and expected cardinality alone.
  // Therefore, to avoid problems such as those reported in genesis cases
  // 10-080530-0291, 10-080530-0305, solution 10-080530-3538, we have to:
  //   a) logically separate estimateMaxSelectivity from estimateCardinality, 
  //   b) have them work on separate copies of the same ColStatDescList.
  // But, to minimize code duplication and maintenance, we have to physically
  // keep only one method. So, estimateCardinality has both the expected
  // cardinality and the max selectivity code.
  //
  // For computing maximum cardinality estimate, we need max selectivity to
  // be computed as:
  //   maxSelectivity(p1 AND p2 and ... pn) =
  //   MIN(maxSelectivity(p1), maxSelectivity(p2), ... maxSelectivity(pn))
  // 
  // The predicate conjuncts are spread across various predicate categories
  // which are evaluated at different points in the code. Therefore, this MIN
  // will often be seen in the code as
  //   maxSelectivity = MINOF( <someSelectivityExpr>, maxSelectivity )
  // -----------------------------------------------------------------------
  // NB: we are here to compute either:
  //       1) expected cardinality, or
  //       2) max selectivity
  // When we are doing 1) we should avoid doing 2) and vice versa. 
  // We are doing 1) when maxSelectivity==NULL.
  // We are doing 2) when maxSelectivity!=NULL.
  // -----------------------------------------------------------------------
  
  // If there is any cardinality hint is given return that, even
  // if there are no predicates to be applied. This would give more
  // control on the user to play around with the base cardinality
  // indirectly

  CollIndex i;

  CostScalar newRowCount  = MIN_ONE_CS( initialRowCount );
  ValueIdSet * predsWithNoHints = new (STMTHEAP) ValueIdSet(setOfPredicates);

    // Note: Can apply Default selectivity, even if no column list....
 if (maxSelectivity == NULL) {
  if ( setOfPredicates.entries() == 0 OR this->entries() == 0 )
  {
      // This is the rowcount without using hints. Save it
      setScanRowCountWithoutHint(initialRowCount);

    // Normally if there is no cardinalityHint given, there is nothing much
    // we can do in the absence of predicates or the histograms. So we return
    // from there. But if there is a hint given, then we can apply the hint
    // on the initial row count and return. This would be useful for cases
    // where we want to uplift the base cardinality of the histograms
    if (cardHint == NULL)
      return initialRowCount;
    else
    {
      newRowCount = cardHint->getScanCardinality();

      if((newRowCount.getValue() - floor(newRowCount.getValue())) > 0.00001)
      newRowCount = MIN_ONE_CS(ceil(newRowCount.getValue()) );

      return newRowCount;
    }
  }
 } // maxSelectivity == NULL

  // Now apply all the predicates on the histograms

  CostScalar tempRowcount = MIN_ONE_CS( initialRowCount );

  // all CSD's should have the same rowcount!
  if (maxSelectivity == NULL)
  {
    // (this code in its own scope so it can easily be cut-and-pasted anywhere)
    CollIndex limit = ( ( mergeMethod == SEMI_JOIN_MERGE ||
                          mergeMethod == ANTI_SEMI_JOIN_MERGE ) ?
                        numOuterColStats : entries() );
    // first, check from 0..limit-1
    enforceInternalConsistency( 0, limit );
    // next, check from limit..entries()
    enforceInternalConsistency( limit, entries() );

    const CostScalar & matchRowcount =
      (*this)[0]->getColStats()->getRowcount();
    // try this out : if the current rowcount isn't what it's "supposed" to be,
    // set it as requested and see how things go
    if ( matchRowcount != newRowCount )
      synchronizeStats( matchRowcount, newRowCount, limit );
  } // maxSelectivity == NULL

  ValueIdSet EqLocalPreds, OtherLocalPreds, EqNonLocalPreds,
             OtherNonLocalPreds, BiLogicPreds, DefaultPreds;

  setOfPredicates.categorizePredicates (outerReferences, EqLocalPreds,
                                        OtherLocalPreds, EqNonLocalPreds,
                                        OtherNonLocalPreds, BiLogicPreds,
                                        DefaultPreds);

  NAHashDictionary<ValueId, CostScalar> biLogicPredReductions // <ValueId, rowred> pairs
    (&(ValueIdHashFn),       11, TRUE, HISTHEAP) ;
  //          hash fn, initsize, uniq, heap


  ValueIdSet subDefPreds;
  if (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON )
  {
    for( ValueId subVid = DefaultPreds.init();
	 DefaultPreds.next( subVid );
	 DefaultPreds.advance( subVid ) )
    {
      ValueIdSet vs;
      ItemExpr * pred = subVid.getItemExpr();
      if ( pred->getOperatorType() == ITM_RANGE_SPEC_FUNC )
      {
	((RangeSpecRef *)pred)->getValueIdSetForReconsItemExpr(vs);
	vs.categorizePredicates (outerReferences, EqLocalPreds,
				 OtherLocalPreds, EqNonLocalPreds,
				 OtherNonLocalPreds, BiLogicPreds,
				 subDefPreds);
      
	DefaultPreds.remove(subVid);
	if (subDefPreds.entries() > 0)
	{
	  DefaultPreds += subDefPreds;
	  subDefPreds.clear();
	}
      }
    } // for
  } //if

  // -----------------------------------------------------------------------
  //  Apply AND / OR logical predicates first
  // -----------------------------------------------------------------------
      newRowCount = applyBiLogicPred(
	tempRowcount,
	BiLogicPreds,
	outerReferences,
        expr,
	selHint,
	cardHint,
	numOuterColStats,
	unresolvedPreds,
	mergeMethod,
	biLogicPredReductions,
	exprOpCode,
	maxSelectivity
	);

	// If the histograms have been modified while applying OR / AND predicates,
	// then they should be scaled, else we end up computing selectivity based on
	// the old rowcount and uec. This would happen only when there were some
	// OR / AND predicates in the query since these predicates synchronize
	// all histograms to have the same rowcount but do not modify the histogram
	// intervals. Rest all other predicates do not synchronize histograms till
	// all the predicates have been applied, hence their aggregate rowcount is
	// equal to the total rowcount from all histogram intervals. TPCH Q07 is a
	// good example, where if the histograms are not scaled, it results in high
	// join cardinalities
      if (maxSelectivity == NULL) {
	for (i = 0; i < entries(); i++)
	{
	  if ((*this)[i]->getColStats()->getRedFactor() != csOne)
	  {
		ColStatsSharedPtr thisColStats = (*this)[i]->getColStatsToModify();
		thisColStats->scaleHistogram(1);
	  }
	}
      } // maxSelectivity == NULL

	ValueId id;
	ValueIdSet nonVEGEqualPredSet;

	// do all non-VEG preds together
	OtherLocalPreds.insert( OtherNonLocalPreds );

	for( id = OtherLocalPreds.init();
		 OtherLocalPreds.next( id );
		 OtherLocalPreds.advance( id ) )
	{
	  ItemExpr * pred = id.getItemExpr();

	  if(pred->getOperatorType() == ITM_EQUAL)
	  {
	    nonVEGEqualPredSet.insert(id);
	    OtherLocalPreds.remove(id);
	  }
	}

	// predicate application : we do it "all at once" (previously, we used
	// to re-synchronize histograms after every predicate application)

	CostScalar rowcountBeforePreds = newRowCount;

	// in the case of a [ANTI_]SEMI_JOIN, the outer reference columns have a
	// different rowcount-before-preds :
	CostScalar semiJoinRowcountBeforePreds = csZero;
	if (  ( mergeMethod == SEMI_JOIN_MERGE ||
			mergeMethod == ANTI_SEMI_JOIN_MERGE )
		 AND
		  ( numOuterColStats < this->entries() )
              AND maxSelectivity == NULL
	   )
	{
	  semiJoinRowcountBeforePreds =
		(*this)[numOuterColStats]->getColStats()->getRowcount();
	}

	// -----------------------------------------------------------------------
	// Apply VEG predicates: Local and non-local
	//   Column stats for outer references are included in THIS.
	// -----------------------------------------------------------------------

	// do all VEG preds together
	EqLocalPreds.insert( EqNonLocalPreds );

	for( id = EqLocalPreds.init();
		 EqLocalPreds.next( id );
		 EqLocalPreds.advance( id ) )
	{
	  tempRowcount = rowcountBeforePreds;

	  ItemExpr * pred = id.getItemExpr();

	  if ( !applyVEGPred( pred, tempRowcount, numOuterColStats, 
                              mergeMethod, exprOpCode, maxSelectivity ) )
		DefaultPreds.insert( id );

          if (maxSelectivity == NULL)
	  newRowCount = tempRowcount;
	}
  // -----------------------------------------------------------------------
  // Apply non-VEG equality predicates: Local and non-local
  // -----------------------------------------------------------------------

  for( id = nonVEGEqualPredSet.init();
       nonVEGEqualPredSet.next( id );
       nonVEGEqualPredSet.advance( id ) )
  {

    ItemExpr * pred = id.getItemExpr();

    tempRowcount = rowcountBeforePreds;

    if ( !applyPred( pred,
		     tempRowcount,
		     numOuterColStats,
		     mergeMethod,
             exprOpCode,
             maxSelectivity
		   )
	)
    {
      DefaultPreds.insert( id );
    }
    else if (maxSelectivity == NULL)
      newRowCount = tempRowcount;
  }

  // predCountSC is used to collect the number of histograms reduced after applying predicates on single
  // column histograms
  CollIndex predCountSC = 0;
  if(exprOpCode == REL_SCAN && !maxSelectivity && (CmpCommon::getDefault(COMP_BOOL_67) == DF_ON))
    predCountSC += biLogicPredReductions.entries();

  // add another variable to count the number of columns reduced that should be considered for MC adjustment
  CollIndex predCountMC = predCountSC;

  CostScalar rowRedProduct = csOne;
  CostScalar rowRedFromEquiPreds = csOne;
  CostScalar rowRedAfterMCUECAdj = csOne;

  CollIndex loopLimit = ( ( mergeMethod == SEMI_JOIN_MERGE ||
                            mergeMethod == ANTI_SEMI_JOIN_MERGE ) ?
                          numOuterColStats : entries() );

  // ---------------------------------------------------------------------
  // now we see whether we can apply multi-column uec info, in the case
  // of a multi-column join between two tables
  // ---------------------------------------------------------------------
  // we can if:
  //   1. multiple joins have been performed (isResultOfJoin() is true for >1)
  //   2. we have multicolumn-uec information that matches those columns
  // --> this is all handled by the subroutine
  //     CSDL::useMultiUecIfMultipleJoins()
  //
  // The following call to useMultiUecIfMultipleJoins() will adjust the
  // value of newRowCount _iff_ there has been a multi-column join and
  // the necessary multi-column uec information exists.
  //
  // NB: we may need to undo some of the adjustments done in the call to
  // useMultiUecIfCorrelatedPreds() above.
  // ---------------------------------------------------------------------
  CollIndexList joinHistograms(STMTHEAP); // the CSDL-indices of the join histograms

  if (maxSelectivity == NULL)
  {
    computeRowRedFactor(mergeMethod, numOuterColStats, rowcountBeforePreds, predCountSC, predCountMC, rowRedProduct);

    // ---------------------------------------------------------------------
    // how to calculate the resulting rowcount ...?
    // ---------------------------------------------------------------------
    // first a first approximation, we use the simplest formula, ignoring
    // multi-column uec info
    // ---------------------------------------------------------------------
    rowRedFromEquiPreds = rowRedProduct;
    newRowCount = rowRedProduct * rowcountBeforePreds;

    this->addRecentlyJoinedCols(0, loopLimit);

    // Use comp_int_40 to compute the reduction beyond which mc adjustment should be done
    // Default value for the CQD is 10, which means we use multi-columns for cardinality 
    // adjustment, only if the reduction from single column histograms is more than 90%

    NABoolean doMCAdjust;
    double mcThreshold = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_40);
    if (mcThreshold >= 1 && mcThreshold <= 100)
    {
      double adjustment = 1.0 - (mcThreshold/100);
      if (newRowCount.getValue() < (rowcountBeforePreds.getValue()* adjustment))
        doMCAdjust = TRUE;
      else 
        doMCAdjust = FALSE;
    }
    else
    {
      if (newRowCount < rowcountBeforePreds)
        doMCAdjust = TRUE;
      else
        doMCAdjust = FALSE;
    }

    // For semi_joins, the newRowCount cannot be greater than the left child's
    // row count, and if we have already reached that row count, then we don't
    // need to uplift it further using multi_column stats
    if ( ( (mergeMethod != SEMI_JOIN_MERGE) &&
	  (mergeMethod != ANTI_SEMI_JOIN_MERGE) ) ||
	  doMCAdjust )
    {
      this->useMultiUecIfMultipleJoins(
	  newRowCount,         /* in/out */
	  rowcountBeforePreds, /* in */
	  0,                   /* in: start idx */
	  loopLimit,           /* in: end idx+1 */
	  joinHistograms,      /* out */
          expr,
          mergeMethod
	  );

    // ---------------------------------------------------------------------
    // now we see whether we can apply multi-column uec info, in the case of
    // predicates on 2+ columns in the table
    // ---------------------------------------------------------------------
    // idea: if those columns are correlated exactly or almost exactly, then
    // the resulting reduction should be less than the product of the 2+
    // reductions -- that is, by multiplying these two reductions together
    // we'll end up removing more rows in our estimate than we should.
    //
    // The underlying assumption here is that the two predicates being applied
    // to the two correlated columns are somehow "redundant" -- that is, both
    // predicates remove some of the "same rows".
    // ---------------------------------------------------------------------
    // At this point we need to take a look at the predicates that have been
    // applied to the various table columns.  If we have applied two
    // predicates to the same table, and those two predicates are correlated,
    // then we need to adjust the overall reduction.
    //
    // The following call to useMultiUecIfCorrelatedPreds() will
    // adjust the value of newRowCount _iff_ this is appropriate, as
    // outlined above.
    // ---------------------------------------------------------------------
    this->useMultiUecIfCorrelatedPreds(
      newRowCount,         /* in/out */
      rowcountBeforePreds, /* in */
      predCountMC,           /* in: # preds */
      joinHistograms,      /* in: hists in join */
      0,                   /* in: start idx */
      loopLimit,		 /* in: end idx+1 */
      biLogicPredReductions
      );
     }
    rowRedAfterMCUECAdj = newRowCount/rowcountBeforePreds;
  } // maxSelectivity == NULL

 // -----------------------------------------------------------------------
  // Apply other supported predicates: Local and non-local
  // -----------------------------------------------------------------------

  for( id = OtherLocalPreds.init();
       OtherLocalPreds.next( id );
       OtherLocalPreds.advance( id ) )
  {

    ItemExpr * pred = id.getItemExpr();

    tempRowcount = rowcountBeforePreds;

    if ( !applyPred( pred,
		     tempRowcount,
		     numOuterColStats,
		     mergeMethod,
             exprOpCode,
             maxSelectivity
		   )
	)
    {
      DefaultPreds.insert( id );
    }
    else if (maxSelectivity == NULL)
      newRowCount = tempRowcount;
  }

  // -----------------------------------------------------------------------
  // Apply predicates with default selectivity
  // -----------------------------------------------------------------------
  CostScalar allPredsGlobalReduction = csOne;

  for( id = DefaultPreds.init();
       DefaultPreds.next( id );
       DefaultPreds.advance( id ) )
  {
    tempRowcount = rowcountBeforePreds;

    ItemExpr * pred = id.getItemExpr();

    // don't repeat the application of any default predicate.
    if ( NOT unresolvedPreds.contains( id ) )
    {
      CostScalar thisPredGlobalReduction = csOne; // initialize it just in case
      applyDefaultPred( pred, thisPredGlobalReduction, exprOpCode,
                        maxSelectivity );

     if (maxSelectivity == NULL) {
      if ( thisPredGlobalReduction.getValue() > 0.0)
      {
	if (CostScalar(DBL_MIN)/thisPredGlobalReduction < allPredsGlobalReduction)
	  allPredsGlobalReduction *= thisPredGlobalReduction;
      }
      else
	allPredsGlobalReduction = csZero;

      // keep note of all predicates for which we've applied default sel
      unresolvedPreds.insert( id );
     } // maxSelectivity == NULL
    }
  }

  // maxSelectivity computation is done
  if (maxSelectivity) return newRowCount;

  // -----------------------------------------------------------------------
  // Have now applied all predicates; now figure out the correct resulting
  // rowcount and normalize all of the histograms to have that rowcount.
  // -----------------------------------------------------------------------

  rowRedProduct = csOne;
  computeRowRedFactor(mergeMethod, numOuterColStats, rowcountBeforePreds, predCountSC, predCountMC, rowRedProduct);

  // -----------------------------------------------------------------------
  // Reduction from equality predicates; Avoid applying reduction from equality
  // predicates twice and also account for MC UEC adjustment
  // -----------------------------------------------------------------------
  rowRedProduct *= (rowRedAfterMCUECAdj/rowRedFromEquiPreds);

  // -----------------------------------------------------------------------
  // don't forget to apply the effects of global (default) predicates!
  // -----------------------------------------------------------------------
  rowRedProduct *= allPredsGlobalReduction;

  // ---------------------------------------------------------------------
  // how to calculate the resulting rowcount ...?
  // ---------------------------------------------------------------------
  // first a first approximation, we use the simplest formula, ignoring
  // multi-column uec info
  // ---------------------------------------------------------------------
  newRowCount = rowRedProduct * rowcountBeforePreds;

  //ensure that we include the largest int.
  if((newRowCount.getValue() - floor(newRowCount.getValue())) > 0.00001)
    newRowCount = ceil(newRowCount.getValue());

  newRowCount = MIN_ONE_CS(newRowCount);

  // after applying MC stats, ensure that the newRowCount has not exceeded
  // the left rowcount, which is the oldRowCount for [anti_]semi_join
  if (mergeMethod == SEMI_JOIN_MERGE ||
                            mergeMethod == ANTI_SEMI_JOIN_MERGE)
  {
    // if after applying MC stats, the new rowcount exceeded left row count
    // then max it out to left row count
    if (newRowCount > rowcountBeforePreds)
      newRowCount = rowcountBeforePreds;
  }

  // ---------------------------------------------------------------------
  // We've now computed the resulting rowcount; now normalize all
  // histograms to have that rowcount.
  // ---------------------------------------------------------------------
  for( i = 0; i < loopLimit; i++ )
  {
    // if this histogram already has a reduction factor applied to it,
    // apply only the remaining delta (not the entire reduction again)
    const CostScalar & oldCount = (*this)[i]->getColStats()->getRowcount();
    if ( oldCount != rowcountBeforePreds )  // only apply the delta
      (*this)[i]->synchronizeStats( oldCount, newRowCount );
    else
      (*this)[i]->synchronizeStats( rowcountBeforePreds, newRowCount );
  }

  tempRowcount = newRowCount;


  // -----------------------------------------------------------------------
  //
  // ALMOST DONE!
  //
  // we now need to handle the other columns in the [ANTI_]SEMI_JOIN case
  //
  // "initial rowcount" == semiJoinRowcountBeforePreds
  //
  // -----------------------------------------------------------------------
  // All the following code should be done only if we have a [anti_]semi_join
  CostScalar rowred = 0;
  if (mergeMethod == SEMI_JOIN_MERGE ||
      mergeMethod == ANTI_SEMI_JOIN_MERGE)
  {
    rowRedProduct = csOne;

    for( i = loopLimit; i < entries(); i++ )
    {
      if ( semiJoinRowcountBeforePreds.isGreaterThanZero() /* > csZero */)
	rowred =
	  (*this)[i]->getColStats()->getRowcount() / semiJoinRowcountBeforePreds;
      else
	rowred = csZero;

      rowRedProduct *= rowred;
    }

    // don't forget to apply the effects of global (default) predicates!
    rowRedProduct *= allPredsGlobalReduction;

    // ---------------------------------------------------------------------
    // how to calculate the resulting rowcount ...?
    // ---------------------------------------------------------------------
    // 1. first, we use the simplest formula, ignoring multi-column uec info;
    // 2. then, we take the results of that formula, and see if we can apply
    // mc-uec info
    // ---------------------------------------------------------------------
    CostScalar newSemiJoinRowCount = rowRedProduct * semiJoinRowcountBeforePreds;
    CollIndexList joinHists(STMTHEAP); //the CSDL-indices of the join histograms

    this->addRecentlyJoinedCols(loopLimit, entries());

    this->useMultiUecIfMultipleJoins(
      newSemiJoinRowCount,         /* in/out */
      semiJoinRowcountBeforePreds, /* in */
      loopLimit,                   /* in: start idx */
      entries(),                   /* in: end idx+1 */
      joinHists,                   /* out */
      expr,
      mergeMethod
      );

    // Similarly for columns from the outer, after applying all changes,
    // we do not want the new row count to exceed the
    // initial row count, which for semi_joins is actually the row count
    // of the left child.
    if (newSemiJoinRowCount > rowcountBeforePreds)
      newSemiJoinRowCount = rowcountBeforePreds;

    for( i = loopLimit; i < entries(); i++ )
    {
      const CostScalar & oldCount = (*this)[i]->getColStats()->getRowcount();
      if ( oldCount != semiJoinRowcountBeforePreds ) // only apply the delta
	(*this)[i]->synchronizeStats( oldCount, newSemiJoinRowCount );
      else
	(*this)[i]->synchronizeStats( semiJoinRowcountBeforePreds,
				      newSemiJoinRowCount );
    }

#ifdef _DEBUG
    // ---------------------------------------------------------------------
    // After all this predicate application, the CSD's should all still have
    // the same rowcount
    // ---------------------------------------------------------------------
    {
      // (this code in its own scope so it can easily be cut-and-pasted anywhere)
      CollIndex limit = ( ( mergeMethod == SEMI_JOIN_MERGE ||
			    mergeMethod == ANTI_SEMI_JOIN_MERGE ) ?
			  numOuterColStats : entries() );
      // verifyInternalConsistency is quite redundant, as we do not do anything
      // in case of inconsistency between the CSDs. We should be much better of
      // enforcingInternalConsistency. replace verifyInternalConsistency
      // with enforceInternalConsistency
      // first, check from 0..limit-1
      enforceInternalConsistency( 0, limit );
      // next, check from limit..entries()
      enforceInternalConsistency( limit, entries() );

      if ( limit > 0 )
       {
         const CostScalar & matchRowcount =
           (*this)[0]->getColStats()->getRowcount();
         // try this out : if the current rowcount isn't what it's "supposed" to be
         // set it as requested and see how things go
         if ( matchRowcount != newRowCount )
           synchronizeStats( matchRowcount, newRowCount, limit );
       }
    }
#endif

  }
  // done with predicate application

  // --------------------------------------------------------------------
  // Lastly, IF we're *NOT* the immediate child of a boolean _AND_
  // operator, undo any of the ColStatDescList length reduction resulting
  // from application of non-VEG-equality preds appearing in the
  // expression tree.
  // --------------------------------------------------------------------
  if ( exprOpCode != ITM_AND )
  {
    NABoolean redo = FALSE;
    CollIndex i = 0;

    while( i < entries() )
    {
      CollIndex moveI = 0;

      if( (*this)[i]->nonVegEquals().entries() != 0 )
      {
	ColStatsSharedPtr rootColStats = (*this)[i]->getColStatsToModify();

	while( (*this)[i]->nonVegEquals().entries() != 0 )
	{
	  // ----------------------------------------------------------
	  // This ColStatDesc has been equated to other
	  // ColStatDescs by a non-VEG Equality predicate.
	  //
	  // Replace the previously-removed ColStatDesc entry.
	  //
	  // One minor complication results from the possible
	  // presence of an Outer Join, which as can be seen in
	  // the above logic, doesn't remove entries from the
	  // inner table's ColStatDesc List.
	  // ----------------------------------------------------------
	  ColStatDescSharedPtr tmpDesc = (*this)[i]->nonVegEquals()[0];
	  ((*this)[i]->nonVegEquals()).removeAt( 0 );

          ValueId column = tmpDesc->getColumn();
          NABoolean found = FALSE;

          // -----------------------------------------------------------
          // Don't restore a ColStatDesc that already has an existing
          //  entry.
          // -----------------------------------------------------------
          for ( CollIndex j = 0; j < entries(); j++ )
           {
	     if ( NOT found AND column == (*this)[j]->getColumn() )
                found = TRUE;
           }
	  if ( NOT found  &&  NOT ( tmpDesc->isFromInnerTable() &&
		     mergeMethod == OUTER_JOIN_MERGE ) )
	  {
	    // update the copy of the removed entry, and re-insert
	    ColStatsSharedPtr upColStats = tmpDesc->getColStatsToModify();
	    upColStats->overwrite( *rootColStats );

	    if( tmpDesc->isFromInnerTable() )
	    {
	      insertDeepCopyAt( numOuterColStats, tmpDesc );

	      if ( i > numOuterColStats )
		moveI++;
	    }
	    else
	    {
	      insertDeepCopyAt( 0, tmpDesc );

	      numOuterColStats++;
	      moveI++;
	    }

	    // Gen Sol:10-090218-9369:  Make sure any ColStatDescs that 
	    // are in nonVegEquals of 'tmpDesc' are correctly put back.
	    if(tmpDesc->nonVegEquals().entries() != 0 )
	      redo = TRUE;

	  }
	  else // it's from the inner table or it's an OUTER_JOIN
	  {
	    // ----------------------------------------------------------
	    // In BETA when the line below wasn't commented out,
	    // we got a memory violation
	    //
	    // NB: if we don't remove it, optimization will use
	    // more memory overall, but there isn't a memory
	    // leak per se.
	    //
	    // Basically, it's safest to comment out the line
	    // below (this fixed Genesis case 10-980121-2284).
	    // ----------------------------------------------------------

	    //delete tmpDesc;
	  }
	}

	if ( moveI == 0 )
	  moveI++;
      }
      else
	moveI++;

      i += moveI;

      if(redo)
      {
	i = 0;
	redo = FALSE;
      }
    }
  }

  // Update MC Skew values for joins
  if(expr)
  {
    ValueIdList joinColsGroup;
    const MCSkewedValueList * joinedMCSkewedValueList = ((Join *)expr)->getMCSkewedValueListForJoinPreds(joinColsGroup);    
    if(joinedMCSkewedValueList)
    {
      if(!mcSkewedValueLists_)
        mcSkewedValueLists_ = new (HISTHEAP) MultiColumnSkewedValueLists();
      ValueIdList * key  = new (HISTHEAP) ValueIdList(joinColsGroup);
      MCSkewedValueList * value = new (HISTHEAP) MCSkewedValueList (*joinedMCSkewedValueList, HISTHEAP);      
      mcSkewedValueLists_->insert( key, value );
    }
    else if(mcSkewedValueLists_)
    {
      mcSkewedValueLists_->clear(TRUE);
      mcSkewedValueLists_ = NULL;
    }
  }

  // The estimated cardinality should never go below zero
  if (newRowCount.getValue() < 0)
  {
// LCOV_EXCL_START - rfi
    CCMPASSERT( newRowCount.isGreaterOrEqualThanZero() );
    newRowCount.minCsZero();
// LCOV_EXCL_STOP
  }

  // This is the rowcount without using hints. Save it
  setScanRowCountWithoutHint(newRowCount);

  // Now after applying all the predicates, make any adjustments based on the
  // hints provided by the user. see if the expression is of the
  // index, and that if we can partially apply cardinality or selectivity hint

  if (cardHint || selHint)
    newRowCount = adjustRowcountWithHint(cardHint, selHint,
      setOfPredicates,
      newRowCount,
      initialRowCount);

  return newRowCount;

} // ColStatDescList::estimateCardinality()

// Adjust the rowcount based on the cardinality / selectivity / count(*) hint
CostScalar 
ColStatDescList::adjustRowcountWithHint(const CardinalityHint * cardHint, 
                                        const SelectivityHint * selHint, 
                                        const ValueIdSet & setOfPredicates,
                                        CostScalar & newRowCount, 
                                        const CostScalar & initialRowCount)
{
  double selectivityHint = -1.0;
  ValueIdSet localPredsFromHint;
  double baseSelectivity = 1.0;
  CostScalar selWithoutHint = newRowCount / initialRowCount;
  if (cardHint!= NULL)
  {
    selectivityHint = cardHint->getScanSelectivity().getValue();
    localPredsFromHint = cardHint->localPreds();
    baseSelectivity = cardHint->getBaseScanSelectivityFactor();
  }
  else
  {
    selectivityHint = selHint->getScanSelectivityFactor();
    localPredsFromHint = selHint->localPreds();
    baseSelectivity = selHint->getBaseScanSelectivityFactor();
  }

  if (localPredsFromHint.entries() > 0)
  {
    ValueIdSet copySetOfPreds(setOfPredicates);

	// Intersect the localPreds with localPredsFromHint to see
	// if all predicate for which the Hint was calculated are covered
	// More localPreds can come because of join predicates that showup
	// as selection predicates. Less number of local predicates
	// can be due to indexes. For lesser number of predicates, adjust
	// selectivity from Hint accordingly

	// get common predicates
	copySetOfPreds.intersect(localPredsFromHint);

    if (copySetOfPreds.entries() < localPredsFromHint.entries()) 
    {
      // The control will come here only for indexes, and by that time
      // the selectivityHint would have been set, either from selectivityHint
      // directly or from CardinalityHint
      selectivityHint = pow(selWithoutHint.getValue(), baseSelectivity);
    }

    if (cardHint && (selectivityHint < 0) )
    {
      // this is the case when estimateCardinality has come from synthLogProp
      // and is still trying to compute the selectivityHint from cardinalityHint
      newRowCount = cardHint->getScanCardinality();
    }
    else
    {
      if (selectivityHint > 0.0)
      {
	  // compute newRowCount from SelectivityHint
	  newRowCount = initialRowCount * selectivityHint;
      }
      else
	newRowCount = csOne;
    }

    if((newRowCount.getValue() - floor(newRowCount.getValue())) > 0.00001)
	newRowCount = ceil(newRowCount.getValue());

    newRowCount = MIN_ONE_CS(newRowCount);

    // Now, normalize all histograms to the same resultant rowcount.
    for( CollIndex i = 0; i < entries(); i++ )
    {
      const CostScalar & oldCount =
	  (*this)[i]->getColStats()->getRowcount();
      if( oldCount != newRowCount ) // && oldCount != 0
	  (*this)[i]->synchronizeStats( oldCount, newRowCount );
    }
  }

  return newRowCount;
}

// -------------------------------------------------------------------
// ColStatDescList::getCardOfBusiestStream
// method returns the cardinality of the busiest stream for the given
// partitioning key
// -------------------------------------------------------------------

CostScalar
ColStatDescList::getCardOfBusiestStream(const PartitioningFunction* partFunc,
								   Lng32 numOfParts,
								   GroupAttributes * groupAttr,
								   Lng32 countOfCPUs)
{
  // get the partitioning key and number of partitions

  ValueIdSet partKey = partFunc->getPartitioningKey();

  // get the total rows in the histogram
  CostScalar rowCount = (*this)[0]->getColStats()->getRowcount();

  // if number of partitions is 1, return rowcount

  if ( ( numOfParts == 1) ||
	   ( partFunc->isASinglePartitionPartitioningFunction() ) )
  {
	return (rowCount).minCsOne();
  }

  // The cardinality is based on the number of CPUs or the number of
  // partitions (whichever is fewer) for a few situations:
  //   1) if partitioning key is empty
  //   2) the round robin partitioning scheme is used.
  //   3) the skew buster partitioning scheme is used.
  if ( (partKey.isEmpty()) ||
       (partFunc->isASkewedDataPartitioningFunction()) ||
       (partFunc->isARoundRobinPartitioningFunction()) )
  {
	 Lng32 availableCpus = MINOF( numOfParts , countOfCPUs );
	 return (rowCount / availableCpus).minCsOne();
  }

  // In the following loop, get the min UEC from amongst the partitioning
  // key to compute the number of streams. In the same loop also compute
  // the accumulated frequency of the partitioning key to compute cardinality
  // per stream for hash partitions

  // min UEC for partitioning key
  CostScalar uecForPartKey = csOne;

  // accumulated freq of the partitioning key
  CostScalar  accFreq = csOne;

  for (ValueId partKeyElement = partKey.init();
	      partKey.next(partKeyElement);
	      partKey.advance(partKeyElement) )
  {

	// extract all base columns from the partitioning key column before
	// looking for statistics

	ValueIdSet baseColSet;

	// this is because findAllReferencedBaseCols, is defined on ValueIdSet
	ValueIdSet partKeySet(partKeyElement);

	partKeySet.findAllReferencedBaseCols(baseColSet);

	if (groupAttr)
	{
	  GroupAnalysis * grpAnalysis = groupAttr->getGroupAnalysis();

	  if (grpAnalysis)
	  {
		CANodeIdSet treeSet = grpAnalysis->getAllSubtreeTables();

		ValueIdSet myColumns;

		if (NOT treeSet.isEmpty() )
		  myColumns = treeSet.getUsedCols();

		// from all the base columns for this partitioning key, get the
		// one that belong to me. In case of joins, base column set would
		// also contain the column I am joining to

		if (NOT myColumns.isEmpty())
		  baseColSet.intersectSet(myColumns);
	  }
	}

	// get minimum UEC from the given column set
	CostScalar colUec = csMinusOne;
	CostScalar freq = csOne;
	
	if (CmpCommon::getDefault(COMP_BOOL_47) == DF_OFF) 
	{
	  colUec = getMinUec(baseColSet);
	  // get the max frequency of any column from the given column set
	  freq = getMaxOfMaxFreqOfCol(baseColSet);
	}
	else
	{
	  ItemExpr * partKeyExpr = partKeyElement.getItemExpr();

	  // if partitioning key is a Case expression, then use all leaf values
	  // including the constants to compute max frequencies. For all 
	  // other expressions use just the base column set
	  if (partKeyExpr->getOperatorType() == ITM_CASE)
	  {
	    ValueIdSet partKeyLeafValueSet;
	    partKeyExpr->getLeafValueIdsForCaseExpr(partKeyLeafValueSet);
	    colUec = getMaxUecForCaseExpr(partKeyLeafValueSet);
	    freq = getMaxFreqForCaseExpr(partKeyLeafValueSet);
	  }
	  else
	  {
             colUec = getMaxUec(baseColSet);
           freq = getMinOfMaxFreqOfCol(baseColSet);
	  }
	}

	if (colUec == csMinusOne)
	  uecForPartKey = rowCount;
	else
	  uecForPartKey *= colUec;

	accFreq *= (freq / rowCount);
  } // for all partitioning key columns

  // uec cannot be greater than the row count
  uecForPartKey = MINOF(uecForPartKey, rowCount);

  // compute the number of streams
  CostScalar noOfStreams = MINOF((CostScalar)numOfParts, uecForPartKey);

  // If partitioning key column is a Random number, and activeStreams_ is less
  // than the current value of noOfStreams, then noOfStreams = activeStreams_;
  CostScalar activeStreams = partFunc->getActiveStreams();
  long randomFix = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_26);
  if ( (partKey.entries() == 1) AND (randomFix != 0) AND (activeStreams != 0) )
  {
    // Get first key column.
    ValueId myPartKeyCol;
    partKey.getFirst(myPartKeyCol);
    // is it a random number?
    if (myPartKeyCol.getItemExpr()->getOperatorType() == ITM_RANDOMNUM)
    {
      noOfStreams = MINOF(activeStreams, noOfStreams);
    }
  }

  // based on the partitioning type, compute the cardinality of the stream
  // For all but hash type partitions, cardinality = number of rows / # of streams
  // such that # of streams = MINOF(# of partitions, UEC of partitioning key)

  if (partFunc->isAHashPartitioningFunction() ||
	  partFunc->isATableHashPartitioningFunction() )
  {
    // CostScalar cardOfFreqValue = (rowCount * accFreq).minCsOne();
    CostScalar cardOfFreqValue = (rowCount * accFreq);
    CostScalar maxCardPerStream = (((rowCount - cardOfFreqValue)/noOfStreams) + cardOfFreqValue).round();

    // cardinality per stream cannot be greater than the total row count
    maxCardPerStream = MINOF(maxCardPerStream, rowCount);

    // some plans are over-penalized by incorporate_skew_in_costing.
    // provide a cqd HIST_SKEW_COST_ADJUSTMENT to soften effect
    // of incorporate_skew_in_costing. 
    // take weighted average of uniform distribution and skewed data
    // based on the CQD. 
    // 0 -> get RC as if uniformly distributed
    // 1-> get RC as if skewed
    // anything in between is the linear average
    CostScalar uniformDistRowCountPerStream = rowCount / noOfStreams;
    CostScalar histSkewAdjustment = 
      (ActiveSchemaDB()->getDefaults()).getAsDouble(HIST_SKEW_COST_ADJUSTMENT);
    maxCardPerStream = 
      (maxCardPerStream * histSkewAdjustment) + 
      (uniformDistRowCountPerStream * (csOne - histSkewAdjustment));

    return maxCardPerStream.minCsOne();
  }
  else
  {
    // for all other partitions
    // return rows per stream
    return (rowCount / noOfStreams).minCsOne();
  } // for non hash partitioning functions
} // ColStatDescList::getCardOfBusiestStream

// ColStatDescList::identifyMergeCandidates
//
// A utility routine to identify which of the entries in the given
// ColStatDescList (containing statistics associated with columns that
// are accessible in the current operator) are related to the given
// predicate.
// -----------------------------------------------------------------------

NABoolean
ColStatDescList::identifyMergeCandidates (ItemExpr *VEGpred,
                                          CollIndex & rootStatIndex,
                                          CollIndexList & statsToMerge) const
{
  NABoolean foundRoot = FALSE;

  // First, get all members of the VEG group
  const VEG * predVEG = ((VEGPredicate *)VEGpred)->getVEG();
  const ValueIdSet & VEGGroup = predVEG->getAllValues();

  for ( CollIndex i = 0; i < entries(); i++ )
  {
    // 1. See if this histogram references the VEGReference id.
    // 2. Keep track of all histograms that need to be merged.
    // 3. Among those to merge, remember the first as the 'root' histogram.
    //
    // The contents of (*this)[i]->getVEGColumn() may be a VEGReference
    // or an instantiate_null expression.
    const ItemExpr * columnsExpr = (*this)[i]->getVEGColumn().getItemExpr();
    OperatorTypeEnum colType = columnsExpr->getOperatorType() ;



    // QSTUFF
    // In addition to checking for the following colTypes, we shall also
    // see if the histogram for this column is a for a virtual column
    // This could be for cases where a virtual column was created for a
    // constant expression. This is something that is expected. If there are
    // other columns in this colStatDescList, then we might be able to
    // find the root. If not, then the method will return FALSE, which is
    // acceptable.
    if ( NOT (	  colType == ITM_VEG_REFERENCE
	       OR colType == ITM_INSTANTIATE_NULL
	       OR colType == ITM_VALUEIDUNION
	       OR colType == ITM_UNPACKCOL
	       OR colType == ITM_ROWSETARRAY_SCAN
	       OR (*this)[i]->getColStats()->isVirtualColForHist()
	     )
	)
    {
      HISTWARNING("Unexpected column type");
    }
    // QSTUFF

    // There are 4 conditions where the current histogram is of interest:
    //  (a) its identifying column's VEG matches the current predicate
    //      or is contained by a VEG_REFERENCE that is contained by
    //      the current predicate.
    //  (b) its identifying column's VEG is an instantiate_null, the
    //      current predicate also contains an instantiate_null, and
    //      the valueIds of those instantiate_nulls are identical;
    //  (c) its identifying column's VEG is a VALUEIDUNION
    //  (d) its identifying column's VEG is a ITM_ROWSETARRAY_SCAN implying
    //	    it is a histogram created for a rowset

    NABoolean foundCandidate = FALSE;

    // case (a) : exact match
    if ( colType == ITM_VEG_REFERENCE )
    {
      if ( VEGpred->containsTheGivenValue( columnsExpr->getValueId() ) )
      {
	foundCandidate = TRUE;
      }
    }
    else if ( colType == ITM_INSTANTIATE_NULL )
    {
      for ( ValueId id = VEGGroup.init();
            VEGGroup.next( id );
	    VEGGroup.advance( id ) )
      {
	// case (b) : instantiate_null
	if (	 id.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL
	     AND colType == ITM_INSTANTIATE_NULL )
	{
	  const ValueId & nulledVid = columnsExpr->getValueId();
	  if ( nulledVid == id )
	  {
	    foundCandidate = TRUE;
	    break;
	  }
	}
	else
	{
	  // ITM_INDEXCOLUMN (on insert, select, drop table)
	  // ITM_INDEXCOLUMN, ITM_DYN_PARAM (on create table, drop table)
	  // ITM_BASECOLUMN (during init_sql)
	  // ITM_CONSTANT (on select, drop table)
	  continue;
	}
      }
    }
    else if ( colType == ITM_VALUEIDUNION || colType == ITM_UNPACKCOL ||
	      colType == ITM_ROWSETARRAY_SCAN )
    {
      if ( VEGpred->containsTheGivenValue( columnsExpr->getValueId() ) )
      {
	foundCandidate = TRUE;
      }
    }

    if ( foundCandidate )
    {
      statsToMerge.insert( i );

      if ( NOT foundRoot )
      {
	foundRoot = TRUE;
	rootStatIndex = i;
      }
    }
  }
  return foundRoot;
} // ColStatDescList::identifyMergeCandidates  (#1)

// -----------------------------------------------------------------------
// ColStatDescList::identifyMergeCandidates
//
// A variant routine to identify which of the entries in the given
// ColStatDescList match the given operand, which appears in a non-VEG
// equality predicate.
//
// NOTE : the return type from this function (for some reason) is always FALSE.
// -----------------------------------------------------------------------
NABoolean
ColStatDescList::identifyMergeCandidates (ItemExpr * operand,
                                          CollIndexList & statsToMerge) const
{
  OperatorTypeEnum exprType = operand->getOperatorType();
  ValueId nulledVId;
  ValueId id;

  // walk the ColStatDescList
  for ( CollIndex i = 0; i < entries(); i++ )
  {
    NABoolean foundCandidate = FALSE;
    CollIndex nonVEGCount = ((*this)[i]->getNonVegEquals()).entries();

    ItemExpr * columnsExpr = (*this)[i]->getVEGColumn().getItemExpr();
    OperatorTypeEnum colType = columnsExpr->getOperatorType();

    do
    {
      // Case 1: a VegReference that might contain an instantiate null
      if ( colType  == ITM_INSTANTIATE_NULL &&
	   exprType == ITM_VEG_REFERENCE )
      {
	nulledVId = columnsExpr->getValueId();
	const VEG * exprVEG = ((VEGReference *)operand)->getVEG();
	const ValueIdSet & VEGGroup  = exprVEG->getAllValues();

	for ( id = VEGGroup.init();
	      VEGGroup.next( id );
	      VEGGroup.advance( id ) )
	{
	  if ( id.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL &&
	       nulledVId == id.getItemExpr()->getValueId() )
	  {
	    foundCandidate = TRUE;
	    break; // jump to outer do-while
	  }
	}
      }
      // Case 2: inverse of case 1
      else if ( colType  == ITM_VEG_REFERENCE &&
	        exprType == ITM_INSTANTIATE_NULL )
      {
	nulledVId = operand->getValueId();
	const VEG * statVEG = ((VEGReference *)columnsExpr)->getVEG();
	const ValueIdSet & VEGGroup = statVEG->getAllValues();

	for ( id = VEGGroup.init();
	      VEGGroup.next( id );
	      VEGGroup.advance( id ) )
	{
	  if ( id.getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL &&
	       nulledVId == id.getItemExpr()->getValueId() )
	  {
	    foundCandidate = TRUE;
	    break; // jump to outer do-while
	  }
	}
      }
      // Case 3: two VEG References
      else if ( colType  == ITM_VEG_REFERENCE &&
		exprType == ITM_VEG_REFERENCE )
      {
	const VEG * statVEG = ((VEGReference *)columnsExpr)->getVEG();
	const VEG * exprVEG = ((VEGReference *)operand)->getVEG();

	if ( statVEG == exprVEG )
	{
	  foundCandidate = TRUE;
	}
	else // not a direct match
	{
	  // First, get all members of the various VEG groups
	  const ValueIdSet & VEGGroup  = exprVEG->getAllValues();
	  const ValueIdSet & VEGGroup2 = statVEG->getAllValues();

	  // look for column's valueId in a VEGREF in the operand's
	  // valueId set.
	  for ( id = VEGGroup.init();
		VEGGroup.next( id );
		VEGGroup.advance( id ) )
	  {
	    if ( id.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE )
	    {
	      const VEG* nestedVEG =
		((VEGReference *)(id.getItemExpr()))->getVEG();

	      if ( statVEG == nestedVEG )
	      {
		foundCandidate = TRUE;
		break; // jump to outer do-while
	      }
	    }
	  }

	  if ( !foundCandidate )
	  {
	    // look for the operand's valueId in a VEGREF in the
	    // column's valueId set.
	    for ( id = VEGGroup2.init();
		  VEGGroup2.next( id );
		  VEGGroup2.advance( id ) )
	    {
	      if ( id.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE )
	      {
		const VEG* nestedVEG =
		  ((VEGReference *)(id.getItemExpr()))->getVEG();

		if ( exprVEG == nestedVEG )
		{
		  foundCandidate = TRUE;
		  break; // jump to outer do-while
		}
	      }
	    }
	  }
	}
      }
      else if (
	// Case 4: two Instantiate NULLs.
	//     (could look inside their operands???
	   ( colType  == ITM_INSTANTIATE_NULL &&
	     exprType == ITM_INSTANTIATE_NULL )
	// Case 5 : ValueIdUnions
	OR ( colType  == ITM_VALUEIDUNION &&
	     exprType == ITM_VALUEIDUNION )
	// Case 6 : special case for packed col
	OR ( colType  == ITM_UNPACKCOL &&
	     exprType == ITM_UNPACKCOL )
	// Case 7 : VEG_PREDs
	OR ( colType  == ITM_VEG_PREDICATE &&
	     exprType == ITM_VEG_PREDICATE ) 
        OR ( ( CmpCommon::getDefault(COMP_BOOL_48) == DF_ON) &&
           ( colType == ITM_VEG_REFERENCE &&
             exprType == ITM_NATYPE) ) )
      {
	const ValueId & statVid = columnsExpr->getValueId();
	const ValueId & exprVid = operand->getValueId();
	if ( statVid == exprVid )
	{
	  foundCandidate = TRUE;
	}
      }
      else // VEG_REF + VEG_PRED, others?
      {
	// $$$ this is safe, but probably not complete!
	if ( columnsExpr->getValueId() == operand->getValueId() )
	{
	  foundCandidate = TRUE;
	}
      }

      if ( foundCandidate )
      {
	statsToMerge.insert( i );
      }

      // look for a transitive equality relationship between this column
      // and any other columns, where that equality relationship is not
      // represented in a VEGPred.
      if ( nonVEGCount > 0 )
      {
	ColStatDescSharedPtr tmpColStatDesc =
	  ((*this)[i]->getNonVegEquals())[--nonVEGCount];

	columnsExpr = tmpColStatDesc->getVEGColumn().getItemExpr();
	colType = columnsExpr->getOperatorType();
      }
      else
	columnsExpr = NULL;
     }
     while ( columnsExpr != NULL AND foundCandidate == FALSE );

  }  // for

  return FALSE;
} // ColStatDescList::identifyMergeCandidates  (#2)

// -----------------------------------------------------------------------
// ColStatDescList::applyVEGPred
//
// A CSDL::estimateCardinality() subroutine
//
// Given a VEG Predicate, merge all histograms that belong to the same
// equivalence group.
//
//  EX 1:  VEG = (T1.A, T1.B, T1.C)
//             synthesize the effect of T1.A = T1.B = T1.C,
//                  resulting in one consolidated histogram for the VEG
//
//  EX 2:  VEG = (T1.A, T1.B, 10)
//             synthesize the effect of T1.A = T1.B = 10,
//                  resulting in a degenerate histogram with a single
//                  interval representing the value 10.
//
// There is an assumption that all outer histograms in the ColStatDescList
// show the same number of rows and that all inner histograms in the list
// show the same number of rows.  But it is not guaranteed that the number
// of rows in the outer 'table' equals the number of rows in the inner
// 'table.'
//
// ColStatDescList::mergeStats has a return value to tell the caller
// whether or not Default Selectivity should be applied (e.g., because
// mergeStats couldn't process the predicate).
// The test for setting the return value is non-obvious:
//  - If there is a predicate and it can be completely applied, then don't
//    tell the caller to apply default;
//  - If there is a predicate and it can't be completely applied, then do
//    apply default.
//  - If there is no predicate that can be evaluated at run time by this
//    node, then don't apply default.
//
// This is the new version of this function which facilitates applying all
// predicates at once.
// -----------------------------------------------------------------------
#pragma nowarn(770)   // warning elimination
NABoolean
ColStatDescList::applyVEGPred (ItemExpr *VEGpred,
                               CostScalar & rowcount,
                               CollIndex & numOuterColStats,
                               MergeType mergeMethod,
			       OperatorTypeEnum opType,
                               CostScalar *maxSelectivity)
{
  VEG * predVEG = NULL;

  if ( VEGpred->getOperatorType() == ITM_VEG_PREDICATE )
    predVEG = ((VEGPredicate *)VEGpred)->getVEG();
  else
    return FALSE ; // we did not apply the predicate

  NABoolean selHintSpecified = VEGpred->isSelectivitySetUsingHint();

  const ValueId & predValueId = VEGpred->getValueId();
  // Get all members of the VEG group
  const ValueIdSet & VEGGroup = predVEG->getAllValues();

  // This is used by selectivity adjustment code.
  ValueIdSet mergeStatePriorToJoin;

  // retain initial guess-timated rowcount and uec, in case we can't
  // improve on this...
  CostScalar newRowcount  = rowcount; // guesstimate of outer rows
  CostScalar newUec	  = csOne;
  CostScalar saveRowcount = rowcount;
  CostScalar saveUec	  = csOne;
  CostScalar oldRowcountForSelAdj = rowcount;

  CollIndexList statsToMerge(CmpCommon::statementHeap());

  CollIndex i;
  CollIndex rootStatIndex = NULL_COLL_INDEX ;
  ColStatDescSharedPtr rootStatDesc;
  ColStatsSharedPtr rootColStats;
  HistogramSharedPtr hist;
  NABoolean appliedPredicateFlag = FALSE; // return flag: Success Indicator


  // See if the VEG group contains a constant.
  // NOTE:  A VEG group should not contain more than 1 constant; but if
  //        that should happen, referencesAConstValue will return the first
  //        constant found.

  ItemExpr *constant = NULL;
  NABoolean containsConstant  = VEGGroup.referencesAConstValue( &constant );
  ItemExpr* constExprPtr = NULL;

  NABoolean containsConstExpr = VEGGroup.referencesAConstExpr(&constExprPtr);


  // We want the selectivity param to contribute in the same way as its
  // substituted literal, in the context of apply VEG predicate. Note
  // simply treating the param as a host variable does not work well
  // because the selectivity that it contributes is computed from the
  // interval of the base histogram. Here the ::applyXXXPred() methods
  // can be called on behalf of a set of computed histograms. In that case,
  // contributing "original" selectivity info can lead to incorrect
  // result as demonstrated by Query 02 failure in opt/optdml03 test. Here
  // the cardinality estimate for REGION table in the left part of the join
  // tree (the right part is for the subquery) is way too small.
  NABoolean containsSelectivityParam = (containsConstExpr AND
            constExprPtr AND
            constExprPtr->getOperatorType() == ITM_CACHE_PARAM AND
            constExprPtr->castToSelParameter()
                                       );

  // Turn on "containsConstant" when a selectivity param is found in the
  // predicate, so that the param can be used in place of the literal.
  if ( containsSelectivityParam )
     containsConstant = TRUE;

  NABoolean containsHostvar = !containsConstant && containsConstExpr;


  // Explanation of the above: there is currently no "contains a hostvar
  // or dynamic parameter" function.  So, we use what's available.
  //
  // referencesAConstValue() returns TRUE if the VEGGroup contains a constant
  //
  // referencesAConstExpr() returns TRUE if the VEGGroup contains a constant
  // or a hostvar/dynamic parameter
  //
  // Thus, if referencesAConstExpr returns TRUE, but referencesAConstValue
  // returns FALSE, then there's a hostvar, not a constant.


  // In the case of a join, the left child's histograms are at the
  // beginning of the input ColStatDescList, and that fact is used
  // when mergeColStats is dealing with a [anti-]semi-Join.
  //
  // Also note: in the case of a [anti-]semi-join, this logic depends upon
  //      there being no outer-table-only VEGPreds remaining in the
  //      input predicate (they should have been dealt with in the
  //      scan of the outer table).
  //
  // locate the entries in this ColStatDescList that are associated with
  // the current VEG predicate.
  NABoolean foundRoot =
    identifyMergeCandidates( VEGpred, rootStatIndex, statsToMerge );

  CollIndex candidateHistograms = statsToMerge.entries();

  // There is no merge to perform if we don't find a root histogram.
  // BUT, failure to find a root doesn't mean that there is nothing to
  // do.  There is only nothing to do if we didn't find any histogram.
  if ( candidateHistograms == 0 )
    return FALSE; // we did not apply the predicate


  // If there's a hostvar, we want to apply some selectivity!  So return
  // FALSE, and later on, in applyDefaultPred, apply the selectivity we
  // want.
  if ( foundRoot && candidateHistograms == 1 && containsHostvar )
    return FALSE; // we didn't apply the predicate


  if ( foundRoot)
  {
    // Get modifiable copy of ColStats
    rootStatDesc = (*this)[rootStatIndex];
    rootColStats = rootStatDesc->getColStatsToModify();

    if(VEGpred->isSelectivitySetUsingHint())
      mergeStatePriorToJoin = rootStatDesc->getMergeState();

   if (maxSelectivity == NULL) {
    // Get a modifiable copy of Histogram
    hist = rootColStats->getHistogramToModify();

    if ( containsConstant || statsToMerge.entries() > 1 )
    {
      // Initialize resultant rowcount and uec to the actual
      // (non-estimated) initial values.
      newRowcount = rootColStats->getRowcount();
      newUec      = rootColStats->getTotalUec();

      // save the initial aggregate information for later processing.
      saveRowcount = newRowcount;
      saveUec      = newUec;

      // Indicate that we have applied this predicate.  Of course we
      // haven't already applied this predicate, but we expect to,
      // soon.
      //
      // Note that unlike for other predicates, the appliedPreds set
      // is not used to prevent duplicate application of a VEG
      // predicate.
      //
      // Different VEG predicates look identical!
      rootStatDesc->addToAppliedPreds( predValueId );
    }
   } // maxSelectivity == NULL
  } // foundRoot

  // Temp fix for solution 10-100607-0915, off by default.
  NABoolean tupleVirtColFix = FALSE;
  if ( foundRoot && ((rootColStats->getStatColumns()).entries() == 0) &&
       (CmpCommon::getDefault(COMP_BOOL_165) == DF_ON) )
    tupleVirtColFix = TRUE;
  
  // check first for " = constant " case
  if ( foundRoot && containsConstant && (tupleVirtColFix == FALSE) ) 
  {
    // If reference a constant or a selectivity param, then first reduce the histogram
    if ( containsSelectivityParam == TRUE )
       constant = ((SelParameter*)constExprPtr)->getConstVal();

    EncodedValue normValue( constant, FALSE );
    NABoolean neg = FALSE;
    ConstValue* constExpr = constant->castToConstValue(neg);

    if (maxSelectivity == NULL)
       rootColStats->setToSingleValue( normValue, constExpr, &rowcount);
    else
       rootColStats->adjustMaxSelectivity(normValue, constExpr, &rowcount, maxSelectivity);

    appliedPredicateFlag = TRUE;

   if (maxSelectivity == NULL) 
   {
     if (rootColStats->isOrigFakeHist())
     {
       // set the final rowcount as the square root of the baserowcount
       CostScalar newRC = csOne;
       if (!rootColStats->isUnique())
         newRC = ceil(sqrt(rowcount.getValue()));
       rootStatDesc->synchronizeStats(rootColStats->getRowcount(), newRC);
     }

    // update the uec and rowcount
    newRowcount = rootColStats->getRowcount();
    newUec      = rootColStats->getTotalUec();

    // $$$ WE NO LONGER WANT TO NORMALIZE HISTOGRAMS AFTER EVERY
    // $$$ PREDICATE!
    //
    // Instead, we will normalize all of the histograms' rowcounts after
    // we have applied all of the predicates.

    // update the 'saved' information
    saveRowcount = newRowcount;
    saveUec      = newUec;
   } // maxSelectivity == NULL
  }

  // Next, check for non-root histograms to merge.
  if ( foundRoot && statsToMerge.entries() > 1 ) // make sure there is work
  {
    // Typically, this means: For each matching histogram interval,
    //     # rows = (#rows A) * (#rows B) * (1/MAX(uecA, uecB))
    //     # uec  = MIN (uecA, uec B)
    //   where A represents one histogram and B the other histogram
    //
    // However, there are non-typical cases to worry about.
    // If the predicate being done is t1.a=t1.a, then the result of
    //   that merge is the histogram that has the smaller rowcount.
    // If the predicate is one where the result of the merge t1.a=
    //   t1.b is now again being merged with t1.a, then the result
    //   is the result of the earlier merge of t1.a=t1.b
    //
    // In response to a possible protest that the above can't happen,
    //   you might be correct.  But what is it that would prevent it??
    // NOTE: this code assumes that all predicates that can be pushed
    //       down to 'identical' columns are pushed down to all copies
    //       of those columns.
    for ( i = 0; i < statsToMerge.entries(); i++ )
    {
      if ( statsToMerge[i] != rootStatIndex )
      {
	appliedPredicateFlag = TRUE;

	// If the statistics to be merged are from opposite sides of
	// the numOuterColStats boundary, then we are doing a
	//      left_table_column = right_table_column
	// merge that should be done as the caller requested.
	// Otherwise, use HIST_NO_STATS_UEC to compute selectivity
	MergeType localMergeMethod = mergeMethod;
	NABoolean joinOnOneTable = FALSE;

	if ( NOT ( rootStatIndex < numOuterColStats &&
		   statsToMerge[i] >= numOuterColStats ) ) 
	{
	  if(!containsConstant && maxSelectivity == NULL) 
	  {
	    // even though the mergeMethod may never be used, but we are
	    // initializing this for cases when COMP_BOOL_74 is OFF
	    localMergeMethod = INNER_JOIN_MERGE;
	    joinOnOneTable = rootStatDesc->mergeColStatDescOfSameTable((*this)[statsToMerge[i]], opType);
	  }
	  else if (containsConstant)
	  {
	    // This is the case of query with selection predicates (A = 1 and B = 1)
            // part of fix for genesis cases 10-080530-0291, 10-080530-0305
            // this block of code is needed to correctly compute 
            //   maxcardinality(a=1 and b=1) 
            // Otherwise, it is incorrectly computed as
            //   maxcardinality(a=b and b=1)
	    joinOnOneTable = TRUE;
	  }
	}

	if (containsConstant)
	{
          // Do not apply the predicate if it has already been applied
          ValueIdSet appliedPreds = (*this)[statsToMerge[i]]->getAppliedPreds();
          if (!appliedPreds.contains(predValueId))
          {
            EncodedValue normValue( constant, FALSE );
            NABoolean neg = FALSE;
            ConstValue* constExpr = constant->castToConstValue(neg);

            if (maxSelectivity == NULL)
            {
               if(!selHintSpecified)
                 (*this)[statsToMerge[i]]->getColStatsToModify()->setToSingleValue
                                             (normValue, constExpr, &rowcount);
            }
            else
              (*this)[statsToMerge[i]]->getColStatsToModify()->
              adjustMaxSelectivity (normValue, constExpr, &rowcount, maxSelectivity );
	   }
        }
	
       if (maxSelectivity == NULL) 
       {
        // for outer joins, if the join VEG pred contains constant, then
        // that has already been applied, so do not do any merge with the right side now
        // Example for queries like T1 left jon T2 on T1.a = T2.a and T1.a = 1
        // the VEG pred (T2.a = 1 = VegRef(T1.a)) will appear both at the Scan (T2.a) and 
        // left join node. The predicate has already been applied on rootStatDesc (join result)
        // by the time the control comes here, so skip the merge
        if (((mergeMethod == OUTER_JOIN_MERGE) && containsConstant)
            || joinOnOneTable)
        {
          // The merge has already been done, so skip mergeColStatDesc
        }
        else
        {
          // else merge the left and the right histograms
          rootStatDesc->mergeColStatDesc ((*this)[statsToMerge[i]],
            localMergeMethod,
            FALSE, // don't force merge
            opType
          );
        }
	// get the aggregate results following the latest merge
	newRowcount = rootColStats->getRowcount();
	newUec      = rootColStats->getTotalUec();

	// $$$ WE NO LONGER WANT TO NORMALIZE HISTOGRAMS AFTER EVERY
	// $$$ PREDICATE!
	//
	// Instead, we will normalize all of the histograms' rowcounts after
	// we have applied all of the predicates.

	// update the 'saved' information
	saveRowcount = newRowcount;
	saveUec      = newUec;
       } // maxSelectivity == NULL
      }
    }
  }

 if (maxSelectivity == NULL) {
  // For histograms that belong to the same equivalence class:
  //   IF this is a not an Outer Join keep only the root
  //   IF this is an Outer Join, the original histograms need to be
  //      replaced by copies of the root; they will be null-augmented.
  if ( !containsConstant && statsToMerge.entries() > 1 )
  {
    // Walk the list backwards to avoid invalidating entries in the list
    // statsToMerge

    //NB: i is unsigned, so this loop as-is should never terminate
#pragma nowarn(270)   // warning elimination
    for ( i = statsToMerge.entries() - 1; i >= 0; i-- )
#pragma warn(270)  // warning elimination
    {
      if (statsToMerge[i] != rootStatIndex)
      {
	if ( mergeMethod == OUTER_JOIN_MERGE &&
	     statsToMerge[i] >= numOuterColStats )
	{
	  ColStatsSharedPtr updateColStats =
	    (*this)[statsToMerge[i]]->getColStatsToModify();
	  updateColStats->overwrite( *rootColStats );
	}
	else
	{
	  this->remove( (*this)[statsToMerge[i]] );

	  // maintain the numOuterColStats value when removing
	  // column statistics from the outer table's list.
	  if ( statsToMerge[i] < numOuterColStats )
	    numOuterColStats--;
	}
      }
      // Since i is unsigned, it'll never become negative, so the loop
      // above (should) never terminate!  Note that for many months, the
      // loop above DID terminate just fine ...  dunno why, though
      if ( i == 0 )
	break ;
    } // for
  } // if

  // return the [altered] rowcount
  rowcount = newRowcount;

  // If no merge could be performed (due to unavailable columns: NOTE
  // we cannot distinguish between run-time unavailability of columns
  // and optimizer unavailability of column statistics), instruct the
  // caller to not apply default selectivity.
  //
  // If the number of candidate histograms is greater than the number
  // actually merged (which should never happen, because we removed
  // multi-column histograms ... oh well), then tell the caller to apply
  // default selectivity ==> even if we *did* do some merging.
  //
  // ** special case: a VEG containing a constant: we'll do enough
  // ** reduction here that default selectivity shouldn't also be applied
  if ( candidateHistograms <= 1 )
    appliedPredicateFlag = TRUE;
  else if ( candidateHistograms > statsToMerge.entries() &&
            !containsConstant )
    appliedPredicateFlag = FALSE;
  } // maxSelectivity == NULL

  // If user specified selectivity for this predicate, we need to make
  // adjustment in reduction to reflect that.
  if ((containsConstant || statsToMerge.entries() > 1) 
    && selHintSpecified)
  {
    ItemExpr * tempPred = NULL;
    ItemExpr * selPred = NULL;
    NABoolean neg = FALSE;
    ValueIdSet baseCols;

    // If VEG predicate already has selectivity specified, use it directly.
    if(VEGpred->getSelectivityFactor() != -1)
      selPred = VEGpred;

    ValueIdSet mergeState = rootStatDesc->getMergeState();
    ValueIdSet predSet = ((VEGPredicate *)VEGpred)->getPredsWithSelectivities();

    for ( ValueId predId = predSet.init();
          predSet.next( predId );
          predSet.advance( predId ) )
    {
      if(selPred)
        break;

      tempPred = predId.getItemExpr();
    
      if(containsConstant)
      {
        // This is for scenario where we have just applied a local predicate 
	    // (eg. 't1.a = 5') and have a hint on that predicate.
        if((mergeState == ValueIdSet(tempPred->child(0))) &&
          (tempPred->child(1)->castToConstValue(neg)))
          selPred = tempPred;
        else
          continue;
      }
      else
      {
        baseCols.clear();
        tempPred->findAll(ITM_BASECOLUMN, baseCols, TRUE, TRUE);
	    // This is the scenario where we are dealing with the first join
        if(mergeState == baseCols)
          selPred = tempPred;
        else if(mergeState.contains(baseCols))
        {
	      // If the query has multiple tables joined on the same column resulting in VEG.
          // Eg, t1.a = t2.b and t1.a = t3.c and t1.a = t4.d => VEGPred(t1.a, t2.b, t3.c,t4.d)
	      // We need to get the correct predicate from the set of predicates in the VEG predicate.
	      ValueIdSet newColumnMerged = mergeState;
          newColumnMerged.subtractSet(mergeStatePriorToJoin);
          if(baseCols.contains(newColumnMerged))
          {	    
            ValueIdSet secondColumnInTheMerge = baseCols;
            secondColumnInTheMerge.subtractSet(newColumnMerged);
            if(mergeStatePriorToJoin.contains(secondColumnInTheMerge))
              selPred = tempPred;
          }
          else
            continue;
        }
        else
          continue;
      }
    }

    if(selPred)
      rootStatDesc->applySelIfSpecifiedViaHint(selPred, oldRowcountForSelAdj);
  }
  return appliedPredicateFlag;

} // applyVEGPred
#pragma warn(770)  // warning elimination

// -----------------------------------------------------------------------
//  ColStatDescList::applyBiLogicPred
//
//  A CSDL::estimateCardinality() subroutine
//
// -----------------------------------------------------------------------
CostScalar
ColStatDescList::applyBiLogicPred(CostScalar & tempRowcount,
				  ValueIdSet & BiLogicPreds,
				  const ValueIdSet & outerReferences,
                                                       const Join * expr,
				  const SelectivityHint * selHint,
				  const CardinalityHint * cardHint,
				  CollIndex & numOuterColStats,
				  ValueIdSet & unresolvedPreds,
				  MergeType mergeMethod,
				  NAHashDictionary<ValueId, CostScalar> & biLogicPredReductions, // in/mod
				  OperatorTypeEnum exprOpCode,
				  CostScalar *maxSelectivity)
{
  ValueId id;

  if (BiLogicPreds.entries() == 0) return tempRowcount;

  CostScalar newRowCount  = MIN_ONE_CS( tempRowcount );

  // maxSelectivity(p1 AND p2) == MIN(maxSelectivity(p1), maxSelectivity(p2))
  // maxSelectivity(p1 OR p2) == MIN(1, maxSelectivity(p1)+maxSelectivity(p2))
  CostScalar origRowCount = tempRowcount;

  for( id = BiLogicPreds.init();
       BiLogicPreds.next( id );
       BiLogicPreds.advance( id ) )
  {
    ItemExpr * pred = id.getItemExpr();
    ValueIdSet leftChildSet ( pred->child(0)->getValueId() );
    ValueIdSet rightChildSet( pred->child(1)->getValueId() );
    OperatorTypeEnum op = pred->getOperatorType();

    NABoolean isLike = ((BiLogic*)pred)->isLike();
    if(isLike && maxSelectivity)
    {
      *maxSelectivity = 1.0;
      break;
    }

    NABoolean doEstCard = (maxSelectivity==NULL);

    origRowCount = tempRowcount;
    
    if ( op == ITM_AND )
    {
      CostScalar maxSel1 = 1.0, maxSel2 = 1.0;

      // Apply both left and right predicates to the given ColStatDescList
      newRowCount = estimateCardinality(tempRowcount,
					leftChildSet,
					outerReferences,
                                        expr,
					selHint,
					cardHint,
					numOuterColStats,
					unresolvedPreds,
					mergeMethod,
                                        ITM_AND,
                                        doEstCard ? NULL : &maxSel1
					);
      tempRowcount = newRowCount;

      newRowCount = estimateCardinality(tempRowcount,
					rightChildSet,
					outerReferences,
                                        expr,
					selHint,
					cardHint,
					numOuterColStats,
					unresolvedPreds,
					mergeMethod,
                                        ITM_AND,
                                        doEstCard ? NULL : &maxSel2
					);
      tempRowcount = newRowCount;
      // maxSelectivity(p1 AND p2)==MIN(maxSelectivity(p1),maxSelectivity(p2))
      if (!doEstCard)
        {
          *maxSelectivity = MINOF(*maxSelectivity, MINOF(maxSel1, maxSel2));
        }
    }
    else if ( op == ITM_OR )
    {
      // special case: we'd like to detect OR-trees which result from
      // large IN-lists
      //
      // we will try to detect such an IN-list below; if this predicate is
      // such an IN-list, then we'll handle the histogram-transformation
      // a little simpler than what we usually do ...
      NABoolean large_in_list = FALSE;

      const ValueId & Lid = pred->child(0)->getValueId();
      const ItemExpr * Lpred = Lid.getItemExpr();
      const OperatorTypeEnum Lop = Lpred->getOperatorType();

      const ValueId & Rid = pred->child(1)->getValueId();
      const ItemExpr * Rpred = Rid.getItemExpr();
      const OperatorTypeEnum Rop = Rpred->getOperatorType();

      ValueId RidLeft;
      NABoolean inListCase = TRUE;
      if ( Rop == ITM_EQUAL) // don't look at child if not correct operator!
      {
	RidLeft = Rpred->child(0)->getValueId();
      }
      else
      {
        inListCase = FALSE;
      }

      Int32 in_list_members = 1; // count the first right child (above)
      if ( (  
            Lop == ITM_OR
	AND Rop == ITM_EQUAL
	AND
	    // at this point we're reasonably sure that we're inside an OR-tree
	    // that was transformed from a (large) IN-list; we now call a
	    // function to check this carefully ...
	    isPredTransformedInList ( Lid, RidLeft, in_list_members )
	AND
	    // $$$ NB: we ARBITRARILY set the number of IN-list members
	    // $$$ for which we try to carefully perform *EXACT*
	    // $$$ histogram manipulation
	    in_list_members > HIST_MAX_IN_LIST_MEMBERS
	AND
	    // just in case the constant/default has a dumb value
	    in_list_members > 5
	    ) ||
            ((Lop == ITM_OR )
            AND
            (inListCase == FALSE)
            AND
            (((BiLogic*)pred)->getNumLeaves() > HIST_MAX_IN_LIST_MEMBERS))
	)
      {
	large_in_list = TRUE;
       if (maxSelectivity == NULL) 
       {
	// now do the necessary histogram-manipulation ...
	//
	// first we find the histogram which matches the IN-list;
	// then we do some manipulation

	if (RidLeft == NULL_VALUE_ID)
	  return newRowCount;
	ItemExpr * RidLeftExpr = RidLeft.getItemExpr();
	OperatorTypeEnum RidLeftOp = RidLeftExpr->getOperatorType();

	while ((RidLeftOp !=  ITM_INDEXCOLUMN) &&
	       (RidLeftOp !=  ITM_INSTANTIATE_NULL) &&
	       (RidLeftOp !=  ITM_BASECOLUMN) &&
	       (RidLeftOp !=  ITM_VEG_REFERENCE) )
	{

	  // Do not assume that the left child of EQUAL will always be a column
	  // it can be a SUBSTRING too, with the column CASTed.

	  if (  (RidLeftExpr->child(0)) &&
			(RidLeftExpr->child(0)->getOperatorType() != ITM_CONSTANT) )
		RidLeft = RidLeftExpr->child(0)->getValueId();
	  else
	  {
		// for TRIM like functions, the left child is a constant while
		// the right child is a column.
		if (RidLeftExpr->child(1) )
		  RidLeft = RidLeftExpr->child(1)->getValueId();
		else
		{
		  // There is no column to apply predicate, hence return newRowCount
		  return newRowCount;
		}
	  }
	  RidLeftExpr = RidLeft.getItemExpr();
	  RidLeftOp = RidLeftExpr->getOperatorType();
	}

	if (RidLeft == NULL_VALUE_ID)
	  return newRowCount;

	CollIndex histIdx;
	if ( NOT this->getColStatDescIndexForColumn( histIdx,RidLeft ) )
	{
	  //10/15/2004
	  //
	  //Initially we used to assert here, the following if was missing
	  //The assertion was because even though RidLeft was covered
	  //the node to which this ColStatDescList was attached, the node
	  //did not produce any column in the RidLeft VEG (i.e. if RidLeft
	  //represented a VEGREF). This is possible if the VEG contains a
	  //constant, in such a case the VEGREG is covered by any node, since
	  //any node can produce a constant. To cover such a VEG the node
	  //does not need to produce any columns in the VEG.
	  //This assertion was being hit for a valid query
	  //Consider the scenario
	  //create table t1(x integer);
	  //create table t2(y integer);
	  //create table t3(z integer);
	  //display
	  //select *
	  //from t3, t2, t1
	  //where t1.x in (1, 2, 3, 4, 5, 6, 7, 8, 10 ,
	  //               11 ,12 ,13, 14, 15 ,16, 17 ,18, 19, 20,
	  //               21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	  //               31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	  //               41, 42)
	  //   and
	  //      t1.x = t2.y
	  //   and
	  //t2.y = 0;
	  //
	  //The code we are in is called when we have a large in
	  //list (>42 items)
	  //
	  //this could happen if we get a VEGREF (i.e. RidLeft is a VEGREF)
	  //that contains a constant. In such a case, the VEG will be considered
	  //covered as it contains a constant and could be pushed down to a
	  //scan that does not produce any columns in the VEG.
	  // Keep a check, this ASSERTION could be not relevant now. In that case
	  // remove the following code.

	  //if RidLeft is a VEGREF
	  if(RidLeft.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
	  {
	    //Get the VEG referred by RidLeft
	    VEG * RidLeftVEG = ((VEGReference*) RidLeft.getItemExpr())\
	                          ->getVEG();
	    //Get the set of valueIds in RidLeftVEG
	    ValueIdSet RidLeftSet;
	    RidLeftVEG->getAndExpandAllValues(RidLeftSet);

	    //check if the VEG contains a constant
	    NABoolean containsConstant = RidLeftSet.referencesAConstExpr();

	    //If VEG contains a constant, then RidLeft is covered
	    //by this node (i.e. node to which this ColStatDescList is attached)
	    //even if it does not produce any of the columns in the VEG.
	    if(containsConstant)
	      return newRowCount;
	  }

	  //This should not happen and so assert in a
	  //debug build
	  // could not find a matching histogram ... huh?!
	  // CCMPASSERT( FALSE ); // this should not happen!

    //CCMPASSERT is only compiled for debug builds
    //Therefore exit gracefully for a release build
    //Since this is a non-fatal error
    return newRowCount;
	}

	else
	{
	  ColStatDescSharedPtr predColumn = (*this)[histIdx];
	  ColStatsSharedPtr predStats = predColumn->getColStatsToModify();

	  if (inListCase && (predStats->getTotalUec() <= in_list_members ) )
	  {
	    // do nothing : assume no uec/rows are removed by
	    // application of this large in-list
	  }
	  else
	  {
            CostScalar redFactor = csOne;
            if (inListCase)
            {
	    // Set the histogram's new totalUec to be the
	    // number of in-list members, adjusting the
	    // rowcount by the same proportion
	      redFactor = (CostScalar( in_list_members ) / predStats->getTotalUec()).maxCsOne();
            }
            else
            {
              // it is a large OR predicate, but not derived from IN
              // list. Instead it could be a set of AND and ORs
              redFactor = (CostScalar (((BiLogic*)pred)->getNumLeaves()) / predStats->getTotalUec()).maxCsOne();
            }

	    newRowCount =
	      predStats->getRowcount() * redFactor;
	    predStats->scaleHistogram(redFactor, redFactor);
	    predStats->setBaseUec(predStats->getTotalUec());

	    // after this approximation, we no longer completely "trust"
	    // this histogram ...
	    predStats->setFakeHistogram( TRUE );
	    // Now, normalize all histograms to the same resultant rowcount.
	    for( CollIndex i = 0; i < entries(); i++ )
	    {
	      const CostScalar & oldCount =
	        (*this)[i]->getColStats()->getRowcount();
	      if( oldCount != newRowCount ) // && oldCount != 0
	        (*this)[i]->synchronizeStats( oldCount, newRowCount );
	    }
            // update tempRowcount which is baing used later as the starting
            // cardinality for the remaining bi-logic predicate
            tempRowcount = newRowCount;
	  }
	}
       } // maxSelectivity == NULL
      } // large_in_list case handled

      // ----------------------------------------------------------------------
      // If this predicate is simply a large OR-tree that resulted
      // from an IN-list transformation, then we've already handled
      // that case above, so we're done applying this predicate
      // ----------------------------------------------------------------------

      if ( NOT large_in_list )
      {
	// the following two lists, one of ValueId's, one of char *'s, are used
	// to keep track of the TRUE shape-changed flag's of this OR-node's
	// parents -- assuming the parent is not also an OR
	//
	// this is needed because when we look at the children of the OR,
	// we set the shape-changed flags for all columns to be FALSE
	//
	// if we're to do the right thing, we will need to know later on
	// whether a column was shape-changed before it was passed to this OR node
	ValueIdList shapeChangedItemExprs;
        NAList<ComUID> shapeChangedHistIds(CmpCommon::statementHeap());

        CollIndex dictSize = MIN_ONE(this->entries());

	NAHashDictionary<ValueId, ValueIdSet> priorAppliedPredsSet (
	  &(ValueIdHashFn), dictSize, TRUE, HISTHEAP );

	NAHashDictionary<ValueId, ValueIdSet> priorMergeStateSet (
	  &(ValueIdHashFn), dictSize, TRUE, HISTHEAP );

	// Make a copy of the current input ColStatDescList THIS;
	// Apply the left predicate to THIS;
	// Apply the right predicate to the manufactured copy;
	// OR the two lists together, into THIS...
	//    carefully, because the two lists no longer have to be the
	//    same length;
	// Discard the copy (it'll go away since it's a stack variable)
	ColStatDescList copyStatsList( HISTHEAP );
	CollIndex i;
	copyStatsList.makeDeepCopy (
	  *this,
	  1,        // scale remains unchanged
	  FALSE	    // clear shapeChanged flag
	  );

	// for OR's, clear THIS's shapeChanged_ flag (copyStatsList cleared above)
       if (maxSelectivity == NULL) {
	for ( i = 0; i < entries(); i++ )
	{
	  ColStatDescSharedPtr copyStatDesc = (*this)[i];
	  ColStatsSharedPtr copyStats = copyStatDesc->getColStatsToModify();

	  // need to store these shape-changed flags in order to keep
	  // track of all the information that our parent gave us!
	  // --> we don't do this if our parent is an OR
	  if ( exprOpCode != ITM_OR )
	  {
	    const ValueId & saveExpr = copyStatDesc->getVEGColumn().getItemExpr()->getValueId();

	    if ( copyStats->isShapeChanged() )
	    {
	      shapeChangedItemExprs.insert( saveExpr );
	      shapeChangedHistIds.insert( copyStats->getHistogramId() );
	    }

	    if (!copyStatDesc->getAppliedPreds().isEmpty())
	    {
	      ValueId    * key   = new (HISTHEAP) ValueId( saveExpr );
	      ValueIdSet * keyValue  = new (HISTHEAP) ValueIdSet( copyStatDesc->getAppliedPreds() );
	      priorAppliedPredsSet.insert(key, keyValue);
	    }

	    if (copyStatDesc->getMergeState() != copyStatDesc->getColumn())
	    {
	      ValueId    * key   = new (HISTHEAP) ValueId( saveExpr );
	      ValueIdSet * keyValue  = new (HISTHEAP) ValueIdSet( copyStatDesc->getMergeState() );
	      priorMergeStateSet.insert(key, keyValue);
	    }
	  } // now we've saved the ItemExpr / Histogram Id for every
	  // column that was shape-changed
	  copyStats->setShapeChanged( FALSE );  // for OR'ing, init clear.
	}
       } // maxSelectivity == NULL

	// track information for probabilistic newRowCount determination
	CostScalar saveTempRowCount = tempRowcount;

	tempRowcount = MIN_ONE_CS( tempRowcount );  // prevent division by Zero!

	// Note, if we use numOuterColStats to the two
	// estimateCardinality() function calls below, it may
	// get changed to a value greater than the number of entries
	// in copyStatsList after the first function call.
	// When that happens and we pass it to the second function call
	// below, it will cause an assertion failure.

	// Copies of numOuterColStats to be applied below.
	CollIndex leftNumOuterColStats  = numOuterColStats;
	CollIndex rightNumOuterColStats = numOuterColStats;

	ValueIdSet originalUnResolved = unresolvedPreds;

    CostScalar maxSel1 = 1.0, maxSel2 = 1.0;
	CostScalar leftRowCount = estimateCardinality(
						      tempRowcount,
						      leftChildSet,
						      outerReferences,
                                                      expr,
						      selHint,
						      cardHint,
						      leftNumOuterColStats,
						      unresolvedPreds,
						      mergeMethod,
						      ITM_OR,
                                                      doEstCard ? NULL : 
                                                      &maxSel1
						      );
	CostScalar prob1, prob2;
	prob1 = leftRowCount / tempRowcount;

	CostScalar rightRowCount = copyStatsList.estimateCardinality(
						      tempRowcount,
						      rightChildSet,
						      outerReferences,
                                                      expr,
						      selHint,
						      cardHint,
						      rightNumOuterColStats,
						      originalUnResolved,
						      mergeMethod,
						      ITM_OR,
                                                      doEstCard ? NULL : 
                                                      &maxSel2
						      );
	prob2 = rightRowCount / tempRowcount;
    // maxSelectivity(p1 OR p2)==MIN(1,maxSelectivity(p1)+maxSelectivity(p2))
    if (!doEstCard)
      {
        *maxSelectivity = MINOF(*maxSelectivity, MINOF(maxSel1+maxSel2, 1.0));
      }

    if (maxSelectivity == NULL) 
    {
      // since both left and right colStats have been initialized by the
      // same list of histograms, these should have equal number of entries. 
      // if not, something when wrong, so ignore this predicate
      // at the customer. 
      if (leftNumOuterColStats != rightNumOuterColStats )
      {
// LCOV_EXCL_START - rfi
        CCMPASSERT( leftNumOuterColStats == rightNumOuterColStats );
        return newRowCount;
// LCOV_EXCL_STOP
      }
	numOuterColStats = leftNumOuterColStats;

	// Under unusual conditions, prob1 and prob2 can be greater than
	// one!  (i.e., if we have a long in-list predicate applied to a
	// histogram that only has a few rows).  So, in other words, the
	// application of some predicates can result in an increase in
	// estimated rowcount!
	//
	// However, the calculation for newRowCount below (involving
	// p1+p2-p1*p2) will not produce a reasonable result if we're
	// not using probabilities!  It makes absolutely no sense to
	// talk about a "probability" that's >1!
	//
	// So, in the case where prob1 or prob2 is >1, we need to
	// scale down the corresponding CSDL to have the original
	// rowcount, then set prob1/prob2 to be 1.  We don't want
	// predicate application to ever increase rowcounts!

	if ( prob1 > csOne )
	{
	  // scale down the histograms
	  for ( CollIndex i = 0; i < this->entries(); i++ )
	    (*this)[i]->synchronizeStats( leftRowCount, tempRowcount );

	  // Don't forget to reset these two values!
	  leftRowCount = tempRowcount;
	  prob1 = csOne;
	}
	if ( prob2 > csOne )
	{
	  for ( CollIndex i = 0; i < copyStatsList.entries(); i++ )
	    (copyStatsList)[i]->synchronizeStats( rightRowCount, tempRowcount );

	  rightRowCount = tempRowcount;
	  prob2 = csOne;
	}

        double prob1prob2 = prob1.value() * prob2.value();

	// estimate for case where histogram merges are not possible.
	newRowCount = saveTempRowCount *
            ( prob1 + prob2 - CostScalar(prob1prob2) );

	// overLappedRowcount first was saveTempRowCount;
	// then, it was leftRowCount + rightRowCount
	// But, the real maximum amount of overlap between two histograms
	// is determined by the smaller side's rowcount.
	CostScalar overLappedRowcount = MINOF( leftRowCount, rightRowCount );

	// predicates are only completely un-resolved when they are
	// un-resolved on Both sides of the OR.
	unresolvedPreds.intersectSet( originalUnResolved );

	// ---------------------------------------------------------------
	// walk over every entry in the updated THIS.
	// Find its match in the corresponding list.
	// An entry matches when the first columns match and the histogram ID
	// matches.
	// -----------------------------------------------------------------

	// Did they change consistently?
	NABoolean bothChangedWheneverOneChanged = TRUE;
	// Did at least one change?
	NABoolean atLeastOneChanged = FALSE;
	ValueIdSet matchedRight;
	CollIndex currentL, currentR;

#pragma nowarn(262)   // warning elimination
	if( this->entries() != copyStatsList.entries() )
	{
	  // copyStatsList is a copy of THIS statsList, during
	  // the process of applying predicates to THIS and
	  // the copy, we should not have dropped any columns.
	  Int32 stophere = 0; // LCOV_EXCL_LINE - rfi
	}
#pragma warn(262)  // warning elimination

	for( currentL = 0; currentL < entries(); currentL++ )
	{
	  const ItemExpr * leftItem =
	    (*this)[currentL]->getVEGColumn().getItemExpr();
	  ComUID leftHistId =
	    (*this)[currentL]->getColStats()->getHistogramId();

	  NABoolean found = FALSE;
	  currentR = 0;
	  while( NOT found && currentR < copyStatsList.entries() )
	  {
	    const ItemExpr * rightItem =
	      (copyStatsList)[currentR]->getVEGColumn().getItemExpr();
	    ComUID rightHistId =
	      (copyStatsList)[currentR]->getColStats()->getHistogramId();

	    if( leftItem->getValueId() == rightItem->getValueId() &&
		(leftHistId == rightHistId) )
	      found = TRUE;
	    else
	      currentR++;
	  }

      if (found == FALSE)
      {
        // if could not find the histogram to be merged
        // return for release mode. Side effect could be 
        // increased cardinality
// LCOV_EXCL_START - rfi
        CCMPASSERT( found == TRUE );
        return newRowCount;
// LCOV_EXCL_STOP
      }
	  ColStatsSharedPtr leftColStats  =
	    (*this)[currentL]->getColStatsToModify();
	  ColStatsSharedPtr rightColStats =
	    (copyStatsList)[currentR]->getColStatsToModify();

	  // If either side has changed, perform an OR merge.
	  // If both sides have shape changed, find how much rowcount
	  //   was overlapped between the two.
	  //  Save the smallest amount of overlap and use it to
	  //   subtract from the sum of the two rowcounts when all
	  //   shape changes come in pairs.

	  // neither changed
	  if( NOT leftColStats->isShapeChanged()  &&
	      NOT rightColStats->isShapeChanged() )
	  {
	    copyStatsList.removeAt( currentR );
	  }
	  else  // at least one side shape-changed.
	  {
	    atLeastOneChanged = TRUE;

	    CostScalar saveRowcount =
	      leftColStats->getRowcount() + rightColStats->getRowcount();
	    (*this)[currentL]->mergeColStatDesc(
	      (copyStatsList)[currentR],
                            OR_MERGE,
                            FALSE,
                            exprOpCode
	      );

	    // for debugging informational value, track the
	    // histograms impacted by an OR.
	    (*this)[currentL]->appliedPreds().insert( id );

            if(rightColStats->isSelectivitySetUsingHint())
              leftColStats->setSelectivitySetUsingHint();

	    // both sides changed?
	    if ( leftColStats->isShapeChanged() &&
		 rightColStats->isShapeChanged() )
	    {
	      CostScalar curDif = saveRowcount -
		(*this)[currentL]->getColStats()->getRowcount();
	      if (curDif < overLappedRowcount)
		overLappedRowcount = curDif;
	    }
	    else // only one side changed!
	    {
	      bothChangedWheneverOneChanged = FALSE;
	    }

	    copyStatsList.removeAt( currentR );
	  }
	}  // for     (currentL)

#pragma nowarn(262)   // warning elimination
	if( copyStatsList.entries() != 0 )
	{
// LCOV_EXCL_START - rfi
      CCMPASSERT (copyStatsList.entries() == 0);
	  // The OR merge between THIS and the copy did not go properly
	  return newRowCount;
// LCOV_EXCL_STOP
	}
#pragma warn(262)  // warning elimination

	// Determine the resultant rowcount.  If not all columns overlapped,
	// use previously determined rowcount
	// (i.e. rowcount * (prob1 + prob2) - (prob1 * prob2))
	//  otherwise just use leftRows + rightRows - overlappedRows.

	if( atLeastOneChanged && bothChangedWheneverOneChanged )
	  newRowCount = leftRowCount + rightRowCount - overLappedRowcount;

	newRowCount = MINOF(newRowCount, origRowCount);

	// If user specified selectivity for this predicate, we need to make
	// adjustment in reduction to reflect that.
	if((exprOpCode == REL_SCAN) &&
	   (pred->isSelectivitySetUsingHint()))
	{
	  ColStatDescSharedPtr firstColStatDesc = (*this)[0];
	  firstColStatDesc->applySelIfSpecifiedViaHint(pred, origRowCount);
	  newRowCount = firstColStatDesc->getColStats()->getRowcount();
	}

	tempRowcount = newRowCount;

	// Now, normalize all histograms to the same resultant rowcount.
	for( i = 0; i < entries(); i++ )
	{
	  const CostScalar & oldCount =
	    (*this)[i]->getColStats()->getRowcount();
	  if( oldCount != newRowCount ) // && oldCount != 0
	    (*this)[i]->synchronizeStats( oldCount, newRowCount );
	}


	if( exprOpCode != ITM_OR )
	  // now, restore our parents' TRUE shape-changed flags after we come out
	  // of the OR -- we lost this information when we started the OR and
	  // set all shape-changed flags to be FALSE
	  // ** this has to be so complicated because the number of columns
	  // ** in THIS may have been reduced by the right child
	  // ** --> or does this not have to be as cautious as it is?
	{
	  for( i = 0; i < shapeChangedItemExprs.entries(); i++ )
	  {
	    NABoolean found = FALSE;
	    for( CollIndex j = 0; j < entries(); j++ )
	    {
	      const ValueId & checkExpr =
		        (*this)[j]->getVEGColumn().getItemExpr()->getValueId();
	      ComUID checkHistId =
		        (*this)[j]->getColStats()->getHistogramId();

	      NABoolean equalExprs   = ( shapeChangedItemExprs[i] == checkExpr );
	      NABoolean equalHistIds = ( shapeChangedHistIds[i] == checkHistId );

	      if ( equalExprs && equalHistIds )
	      {
		      found = TRUE;
		      (*this)[j]->getColStatsToModify()->setShapeChanged(TRUE);
		      break;
	      }
	    }
	    CCMPASSERT( found == TRUE );
	  }

	  // Restore the original appliedPred and mergeState sets which were removed 
	  // before applying OR-related join predicates
	  for( CollIndex j = 0; j < entries(); j++ )
	  {
	    (*this)[j]->appliedPreds().clear();
	    (*this)[j]->mergeState().clear();
	    const ValueId & checkExpr = (*this)[j]->getVEGColumn().getItemExpr()->getValueId();
	    if(priorAppliedPredsSet.contains(&checkExpr))
	      (*this)[j]->appliedPreds().insert(*(priorAppliedPredsSet.getFirstValue(&checkExpr)));
	    if(priorMergeStateSet.contains(&checkExpr))
	    {
	      // At Join-level, we will have histograms from both sides with the same VEG column.
	      // We need to make sure that the valueId of the column is part of saved mergeState 
	      // before copying the mergeState. 
	      ValueIdSet mergeState = *(priorMergeStateSet.getFirstValue(&checkExpr));
	      if(mergeState.contains((*this)[j]->getColumn()))
                (*this)[j]->mergeState().insert(mergeState);
	      else
		(*this)[j]->mergeState().insert((*this)[j]->getColumn());
	    }
	    else
	      (*this)[j]->mergeState().insert((*this)[j]->getColumn());
	  }
	}
       } // maxSelectivity == NULL
      } // NOT large_in_list
    } // ITM_OR
    else  // error: It isn't ITM_AND or ITM_OR ... oops
    {
// LCOV_EXCL_START - rfi
      CCMPASSERT("Unsupported binary logic operator" );
      return newRowCount;
// LCOV_EXCL_STOP
    }

    if(exprOpCode == REL_SCAN && !maxSelectivity && (CmpCommon::getDefault(COMP_BOOL_67) == DF_ON))
    {
      ValueIdSet predSet;
      pred->getLeafPredicates(predSet);

      NABoolean skip = FALSE;
      ItemExpr * column = NULL;
      ItemExpr *lhs = NULL;

      for( ValueId tempId = predSet.init();
	  predSet.next( tempId );
	  predSet.advance( tempId ) )
      {
        ItemExpr *tempExpr = tempId.getItemExpr();
	if (tempExpr->getArity() > 0)
	  lhs = tempExpr->child(0);
	else
	  lhs = NULL;

        if (!tempExpr->equatesToAConstant() ||
	    (column && (lhs != column)) ||
	    (!lhs))
        {
          skip = TRUE;
          break;
        }
        else if(!column)
          column = lhs;
      }

      if(skip)
        continue;

      CollIndex i;
      column = column->getLeafValueIfUseStats();
      ValueId colId = column->getValueId();

      const BaseColumn * baseCol = colId.castToBaseColumn();
      if(!baseCol)
        continue;
      else
        colId = baseCol->getValueId();
      
      if(!getColStatDescIndexForColumn(i, colId))
        continue;
      
      //calculate the reduction from this predicate
      CostScalar rowCnt = (*this)[i]->getColStats()->getRowcount();
      if ( rowCnt < origRowCount )
      {
        ValueId    * key   = new (HISTHEAP) ValueId(colId);
        CostScalar * value = new (HISTHEAP) CostScalar( rowCnt / origRowCount );
        ValueId * result = biLogicPredReductions.insert( key, value );
      }
    }
  }
  return newRowCount;
} // ColStatDescList::applyBiLogicPred

// -----------------------------------------------------------------------
//  ColStatDescList::applyPred
//
//  A CSDL::estimateCardinality() subroutine
//
//  This method applies the effect of a predicate on the set of column
//  statistics.  This method supports the unary predicates IS [NOT] NULL
//  and IS [NOT] UNKNOWN, as well as binary predicates involving the
//  operators {>,>=,<,<=,=,<>}, where one of the operands is a literal
//  constant.
//
//  NOTE: Support for nonVEG <col A> <eqOp> <col B> is provided in this
//  routine; ColStatDesc::modifyStats takes effect only for inequality
//  predicates involving constants.  Support of <col A> <ineqOp> <col B>
//  will (eventually) need to be added.
//
//  NOTE: The code is written as if the same predicate can apply to more
//  than one ColStatDesc in the given ColStatDescList.  This may never
//  happen, but it doesn't hurt to code it this way.
//
// This is the new version of this function which facilitates applying all
// predicates at once.
// -----------------------------------------------------------------------
NABoolean
ColStatDescList::applyPred (ItemExpr *pred,
                            CostScalar & newRowcount,
                            CollIndex & numOuterColStats,
                            MergeType mergeMethod,
                            OperatorTypeEnum exprOpCode,
                            CostScalar *maxSelectivity)
{
  // sanity check: operator understood by synthesis?
  if ( NOT pred->synthSupportedOp() ) return FALSE;

  CostScalar rowCountBeforePreds = newRowcount;
  CostScalar tempRowcount = csZero;
  CostScalar newUec;
  CollIndex i;
  NABoolean appliedPredicateFlag = FALSE;
  NABoolean negate = FALSE;

  // Check to see if the predicate is a derivative from Like predicate
  // If it is then it can only be either >= or <
  NABoolean derivedFromLike = FALSE;
  if ( (pred->getOperatorType() == ITM_GREATER_EQ) OR
      (pred->getOperatorType() == ITM_LESS) )
  {
    BiRelat *br = (BiRelat *) pred;
    derivedFromLike = br->derivativeOfLike();
  }

  // ---------------------------------------------------------------------
  // first check for ITM_IS_NULL, ITM_IS_NOT_NULL, ITM_IS_UNKNOWN, and
  // ITM_IS_NOT_UNKNOWN or the more complex case where the predicate
  // is NOT an equality predicate which does NOT involve a <constant>.
  // ---------------------------------------------------------------------
  if (	      (	   pred->getOperatorType() == ITM_IS_NULL
		OR pred->getOperatorType() == ITM_IS_NOT_NULL
		OR pred->getOperatorType() == ITM_IS_UNKNOWN
		OR pred->getOperatorType() == ITM_IS_NOT_UNKNOWN
	      )
       OR NOT (	    pred->getOperatorType() == ITM_EQUAL
		AND pred->child(0)->castToConstValue( negate ) == NULL
		AND pred->child(1)->castToConstValue( negate ) == NULL
	      )
       OR     (pred->equatesToAConstant())
     )
  {
    CostScalar oldRowcountForSelAdj;

    // Simple Case: look for <op> <col>,
    //                       <col> <op> <constant> or
    //                       <constant> <op> <col> predicates
    for ( i = numOuterColStats /*fix to ALM5128*/; i < entries(); i++ )
    {
      ColStatDescSharedPtr thisColStatDesc = (*this)[i];

      ColStatsSharedPtr colStats = thisColStatDesc->getColStats();
      if ((colStats->isVirtualColForHist() ||
          colStats->isMCforHbasePartitioning ())) // ignore MC stats intervals for now, only used by the splitting logic
	continue;

      if(pred->isSelectivitySetUsingHint())
        oldRowcountForSelAdj = colStats->getRowcount();

      CostScalar totalUecBeforePreds = colStats->getTotalUec();
      CostScalar baseUecBeforePreds = colStats->getBaseUec();
      CostScalar predMaxSel = 1.0;

      if ( thisColStatDesc->modifyStats
           ( pred, tempRowcount, maxSelectivity ? &predMaxSel : NULL) )
      {
		appliedPredicateFlag = TRUE;
		if(derivedFromLike)
		{
                  // for LIKE predicates, maxSelectivity will stay 1.0

		  // This range predicate is a derivative of Like predicate.
		  // Hence we cannot use the usual selectivity obtained after
		  // applying range predicates. Here we use a portion of the
		  // default selectivity for like predicates to obtain the
		  // resultant rowcount. We still continue to use the histograms
		  // obtained after applying range predicates the usual way
		  // The selectivity would be applied to the initial rowcount
		  // before any predicates were applied.
                 if (maxSelectivity == NULL) {
		  BiRelat *br = (BiRelat *) pred;

		  br->adjustRowcountAndUecForLike((*this)[i], 
		                                   rowCountBeforePreds, 
						   totalUecBeforePreds,
						   baseUecBeforePreds);
                 } // maxSelectivity == NULL
		}
        else // !derivedFromLike
        {
          // modifyStats has just computed selectivity(p) by side effecting
          // the histogram. For cases where maxSelectivity(p)=selectivity(p)
          // we want to reflect that in maxSelectivity.
          if (pred->maxSelectivitySameAsSelectivity() && maxSelectivity
              && newRowcount > csZero)
            {
              predMaxSel = MINOF(predMaxSel, tempRowcount/newRowcount);
              *maxSelectivity = MINOF(predMaxSel, *maxSelectivity);
            }
        }

        // If user specified selectivity for this predicate, we need to make
        // adjustment in reduction to reflect that.
        thisColStatDesc->applySelIfSpecifiedViaHint(pred, oldRowcountForSelAdj);
      }
    }

    // $$$ WE NO LONGER WANT TO NORMALIZE HISTOGRAMS AFTER EVERY
    // $$$ PREDICATE!
    //
    // Instead, we will normalize all of the histograms' rowcounts after
    // we have applied all of the predicates.

    return appliedPredicateFlag;
  } // simple case

  // maxSelectivity computation is done
  if (maxSelectivity) return appliedPredicateFlag;

  // ---------------------------------------------------------------------
  // Complex Case:  The input is an Equality Predicate not of the form
  //   <col> = <constant>.
  // Equality predicates can only be performed amongst:
  //
  //   ITM_INSTANTIATE_NULL
  //   ITM_VALUEIDUNION
  //   ITM_VEG_REFERENCE
  //   ITM_UNPACKCOL
  //
  // So, look for that case.  Actions performed depend upon the type of the
  // join, and the positions of the usable entries in the ColStatDescList.
  //
  // But before checking anything further, make sure that this predicate
  // is not a no-op of the form <expr_1> = <expr_1>.
  // ---------------------------------------------------------------------
  ItemExpr * lhs = pred->child(0);
  ItemExpr * rhs = pred->child(1);

  OperatorTypeEnum predExprType = pred->getOperatorType();

  if ( (*lhs) == (*rhs) )
    return TRUE;  // indicate that this no-op predicate was applied

  if (predExprType == ITM_EQUAL) 
  {
    lhs = lhs->getLeafValueIfUseStats();
    rhs = rhs->getLeafValueIfUseStats();
  }

  OperatorTypeEnum leftExprType = lhs->getOperatorType();
  OperatorTypeEnum rightExprType = rhs->getOperatorType();

  // Special handling for predicates of the following form:
  // T1.A = max(T2.B);
  // 
  // If there were no local predicates on T2 prior to join,
  // then we can replace max(T2.B) with the histogram maximum
  // boundary value of T2.B and apply that predicate instead.
  // This is because boundary values are no longer reliable if
  // any local predicates have been applied to the same table.

  if ((CmpCommon::getDefault(HIST_MIN_MAX_OPTIMIZATION) == DF_ON) &&
      (predExprType == ITM_EQUAL) &&
      (leftExprType == ITM_VEG_REFERENCE) && 
      (rightExprType == ITM_MIN || rightExprType == ITM_MAX))
  {
    // Sometimes, aggregate is transformed to max(max(col)), 
    // need to do the following to extract column.
    while(rhs->getOperatorType() == ITM_MIN || rhs->getOperatorType() == ITM_MAX)
      rhs = rhs->child(0);

    CollIndex i = 0;

    // check that the right hand histogram exists
    if ( getColStatDescIndexForColumn(i, rhs->getValueId()) )
    {
      ColStatDescSharedPtr rhsStatDesc = (*this)[i];      
      ColStatsSharedPtr rhsColStats = rhsStatDesc->getColStats();
      ValueId colId = rhsStatDesc->getColumn();
      BaseColumn * colExpr = colId.castToBaseColumn();

      // Need to make sure there is no local preds on the right hand side
      // (where aggregate exists). 
      if( colExpr && colExpr->getTableDesc()->getLocalPreds().entries() == 0)
      {
        // check that the left hand histogram exists
        if (getColStatDescIndexForColumn(i, lhs->getValueId()))
        {
          ColStatDescSharedPtr lhsStatDesc = (*this)[i];
          ColStatsSharedPtr currLhsColStats = lhsStatDesc->getColStats();
          CostScalar currLhsScaleFactor = currLhsColStats->getScaleFactor();
          CostScalar currLhsRC = (currLhsColStats->getRowcount()/currLhsScaleFactor);

          // Need to get original full histogram
          const NAColumnArray &columnList = currLhsColStats->getStatColumns();
          NATable * lhsTable = ((NATable *)columnList[0]->getNATable());
          StatsList &lhsStats = lhsTable->getStatistics();
          Lng32 colPosition = columnList[0]->getPosition();


          ColStatsSharedPtr origLhsColStats = lhsStats.getSingleColumnColStats(colPosition);

          // Make a deep copy
          ColStatsSharedPtr copyOfOrigLeftColStats = 
                             ColStats::deepCopy(*origLhsColStats, HISTHEAP);

          CostScalar origLhsRC = origLhsColStats->getRowcount();

          //Apply the reduction to the orig histogram if any from local preds
          CostScalar localPredReduction = currLhsRC/origLhsRC;
          if(localPredReduction != csOne)
            copyOfOrigLeftColStats->scaleHistogram(localPredReduction);

          // Before we can perform the optimization, we need to make sure we have
          // a good hash value to use to search via MIN/MAX value for an interval.
          // See ColStats::setToSingleValue(const EncodedValue&, ConstValue*, ...)
          // in the block of code where the constant is in MFV. 
          // Here since it is not easy to construct a ConstValue object out of the MIN/MAX
          // value of EncodedValue, we will restrict ourselves to MIN/MAX value ==
          // one of the frequent value from the rhs.
          const FrequentValueList& fvList = rhsColStats->getFrequentValues();

          if ( fvList.entries() > 0 ) {

             EncodedValue normValue;
             FrequentValue fv;

             if(rightExprType == ITM_MIN) {
               normValue = rhsColStats->getMinValue();
               fv = fvList[0];
             } else {
               normValue = rhsColStats->getMaxValue();
               fv = fvList[fvList.entries()-1];
             }

             if ( fv.getEncodedValue() == normValue ) {

                // Reduce to single interval via a key. The key value is
                // normValue and the hash stored in fv. The interface is 
                // a little bit of awkward. But we will live with it for now. 
                copyOfOrigLeftColStats->setToSingleValue(normValue, NULL, NULL, &fv);

                // Need to scale it back by original scaling factor
                copyOfOrigLeftColStats->scaleHistogram(currLhsScaleFactor);

                // Finally overlay the original histogram onto existing ColStatDesc
                lhsStatDesc->setColStats(copyOfOrigLeftColStats);

                return TRUE;
            }
          }
        }
      }
    }
  }

  // For 'Sample' inserts 'notCovered' over the expression, in order to avoid
  // its pushing down. But this should not have any effect on the selectivity
  // hence we shall remove any not_covered from the expressions. And estimate
  // cardinality like a regular predicate.

  NABoolean notCovered = FALSE;

  if (leftExprType == ITM_NOTCOVERED)
  {
    lhs = lhs->child(0);
    if (predExprType == ITM_EQUAL)
      notCovered = TRUE;
  }

  if (rightExprType == ITM_NOTCOVERED)
  {
    rhs = rhs->child(0);
    if (predExprType == ITM_EQUAL)
      notCovered = TRUE;
  }

  if (CmpCommon::getDefault(COMP_BOOL_48) == DF_ON)
  {
    if ( (rightExprType == ITM_NATYPE ) AND
	  (leftExprType == ITM_VEG_REFERENCE ) ) 
      notCovered = TRUE;

  }

  if (!notCovered)
  {
    if (  (rightExprType == ITM_ROWSETARRAY_SCAN ) OR
	  (leftExprType == ITM_ROWSETARRAY_SCAN ) ) 
    {
    }
    else
    {
      if ( NOT (      ( leftExprType == rightExprType )
		  AND ( leftExprType == ITM_INSTANTIATE_NULL
			OR leftExprType == ITM_VALUEIDUNION
			OR leftExprType == ITM_VEG_REFERENCE
			OR leftExprType == ITM_UNPACKCOL
		      )
		)
	  )
      {
	return FALSE;
      }
    }
  }
  // ---------------------------------------------------------------------
  // IF the referenced ColStatDesc's are provided, this equality predicate
  // *can* be performed.
  //
  // Note that Left and Right are relative to the operands of the equality
  // predicate.  Later they need to be oriented to the inner/outer sides
  // of the input ColStatDescList.
  // ---------------------------------------------------------------------
  CollIndexList leftStatsToMerge(STMTHEAP);
  CollIndexList rightStatsToMerge(STMTHEAP);

  identifyMergeCandidates( lhs, leftStatsToMerge );
  if ( leftStatsToMerge.entries() == 0 )
    return FALSE;

  identifyMergeCandidates( rhs, rightStatsToMerge );
  if ( rightStatsToMerge.entries() == 0 )
    return FALSE;

  // Take care of the possibility that one, or more, of the given column
  // references referenced more than one column in the ColStatDescList.
  // If this happens : perform an equijoin amongst the multiple columns.
  CollIndex leftRootIndex  = leftStatsToMerge[0];
  ColStatDescSharedPtr leftRootDesc = (*this)[leftRootIndex];

  CollIndex rightRootIndex = rightStatsToMerge[0];
  ColStatDescSharedPtr rightRootDesc = (*this)[rightRootIndex];

  CollIndex rootIndex;
  ColStatDescSharedPtr rootStatDesc;

  if ( leftRootIndex < rightRootIndex )
  {
    rootIndex    = leftRootIndex;
    rootStatDesc = leftRootDesc;
  }
  else
  {
    rootIndex    = rightRootIndex;
    rootStatDesc = rightRootDesc;
  }

  ColStatsSharedPtr rootColStats = rootStatDesc->getColStatsToModify();

  ColStatsSharedPtr leftColStats = leftRootDesc->getColStatsToModify();
  CostScalar leftRowcount = leftColStats->getRowcount();

  ColStatsSharedPtr rightColStats = rightRootDesc->getColStatsToModify();
  CostScalar rightRowcount = rightColStats->getRowcount();

  // Finally, given a 'root' ColStatDesc,
  //   determine whether or not we have applied this predicate
  const ValueId & predValueId = pred->getValueId();

  if ( rootStatDesc->isPredicateApplied( predValueId ) ||  // Already applied?
       leftRootIndex == rightRootIndex )                   // or no-op?
    return TRUE;

  rootStatDesc->addToAppliedPreds( predValueId );

  CostScalar saveRowcount = rootColStats->getRowcount();
  CostScalar saveUec      = rootColStats->getTotalUec();

  //Under a OR we do not want to handle vegRefs because
  // 1. for predicates a=b and ( c =b or c = d)
  // if we do perform a=b when we are computing c =b we will
  // applying the same predicates twice as a=b later gets applied
  // also.
  // 2. Application of vegref under or gives rise to an unexpected
  // ColStats as we unpack the histograms to get all the merged
  // ColStats back

  if( exprOpCode != ITM_OR )
  {
    mergeSpecifiedStatDescs (
      leftStatsToMerge, leftRootIndex,
      mergeMethod, numOuterColStats,
      saveRowcount, saveUec,
      FALSE,
      exprOpCode); // not for VEGPred

    // repeat for the right equivalence group.
    mergeSpecifiedStatDescs (
      rightStatsToMerge, rightRootIndex,
      mergeMethod, numOuterColStats,
      saveRowcount, saveUec,
      FALSE,
      exprOpCode); // not for VEGPred
  }

  // Gen Sol:10-090728-3382:    
  // In some cases, the same column is participating in more than one join
  // predicates. When subsequent predicates (second onwards) are applied, 
  // the effect of the first predicate application is lost. We will capture
  // the existing reduction here before additional predicates are applied.

  CostScalar leftReduction  = csOne, rightReduction = csOne, scaleFactor = csMinusOne;
  NABoolean isASemiJoin = ((mergeMethod == SEMI_JOIN_MERGE) || (mergeMethod == ANTI_SEMI_JOIN_MERGE));

  if(!isASemiJoin && leftRowcount < rowCountBeforePreds)
  {
    // Need to set the original scale factor before the prior predicates were applied
    if(leftRootDesc->getVEGColumn() == lhs->getValueId())
      scaleFactor = rowCountBeforePreds/leftColStats->getBaseRowCount();
    else
    {
      CollIndex nonVegEqualEntries = leftRootDesc->nonVegEquals().entries();
      for (i = 0; i < nonVegEqualEntries; i++)
      {
        ColStatDescSharedPtr tmpDesc = leftRootDesc->nonVegEquals()[i];
        if(tmpDesc->getVEGColumn() == lhs->getValueId())
        {
          scaleFactor = rowCountBeforePreds / tmpDesc->getColStats()->getBaseRowCount();
          break;
        }
      }
    }

    if(scaleFactor != csMinusOne)
    {
      leftReduction  = leftRowcount / rowCountBeforePreds;
      leftColStats->scaleHistogram(csOne/leftReduction);
      leftColStats->setScaleFactor(scaleFactor);
    }
  }

  scaleFactor = csMinusOne;

  if(!isASemiJoin && rightRowcount < rowCountBeforePreds)
  {
    // Need to set the original scale factor before the prior predicates were applied
    if(rightRootDesc->getVEGColumn() == rhs->getValueId())
      scaleFactor = rowCountBeforePreds/rightColStats->getBaseRowCount();
    else
    {
      CollIndex nonVegEqualEntries = rightRootDesc->nonVegEquals().entries();
      for (i = 0; i < nonVegEqualEntries; i++)
      {
        ColStatDescSharedPtr tmpDesc = rightRootDesc->nonVegEquals()[i];
        if(tmpDesc->getVEGColumn() == rhs->getValueId())
        {
          scaleFactor = rowCountBeforePreds / tmpDesc->getColStats()->getBaseRowCount();
          break;
        }
      }
    }

    if(scaleFactor != csMinusOne)
    {
      rightReduction  = rightRowcount / rowCountBeforePreds;
      rightColStats->scaleHistogram(csOne/rightReduction);
      rightColStats->setScaleFactor(scaleFactor);
    }
  }

  // --------------------------------------------------------------------
  // Now, safely past that unlikely situation, perform the join between
  // the left and right sides of the equality predicate.
  // For merging columns from the same table,  
  // selectivity is HIST_NO_STATS_UEC.
  // --------------------------------------------------------------------
  MergeType localMergeMethod = mergeMethod;
  NABoolean joinFromSameTable = FALSE;

  if ( leftRootIndex < rightRootIndex )
  {
    ColStatDescSharedPtr tmpDesc (new (HISTHEAP)
	   ColStatDesc( *rightRootDesc ), HISTHEAP);
    tmpDesc->setFromInnerTable( rightRootIndex >= numOuterColStats ?
				TRUE : FALSE );

    if ( NOT ( leftRootIndex < numOuterColStats &&
	       rightRootIndex >= numOuterColStats ) )
    {
      // even though the mergeMethod may never be used, but we are
      // initializing this for cases when COMP_BOOL_74 is OFF
      localMergeMethod = INNER_JOIN_MERGE;
      joinFromSameTable = leftRootDesc->mergeColStatDescOfSameTable(tmpDesc, exprOpCode);
    }

    if (!joinFromSameTable)
    {
      leftRootDesc->mergeColStatDesc(
	tmpDesc,
	localMergeMethod,
                        FALSE,  // don't force merge
                        exprOpCode
	);
    }

    leftRootDesc->nonVegEquals().insert( tmpDesc );
  }
  else
  {
    ColStatDescSharedPtr  tmpDesc(new (HISTHEAP)
	  ColStatDesc( *leftRootDesc ), HISTHEAP);
    tmpDesc->setFromInnerTable( leftRootIndex >= numOuterColStats ?
				  TRUE : FALSE );
    if ( NOT ( rightRootIndex < numOuterColStats &&
	       leftRootIndex >= numOuterColStats ) )
    {
      // even though the mergeMethod may never be used, but we are
      // initializing this for cases when COMP_BOOL_74 is OFF
      localMergeMethod = INNER_JOIN_MERGE;
      joinFromSameTable = rightRootDesc->mergeColStatDescOfSameTable(tmpDesc, exprOpCode);
    }

    if(!joinFromSameTable)
    {

      rightRootDesc->mergeColStatDesc(
	tmpDesc,
	localMergeMethod,
                               FALSE,  // don't force merge
                               exprOpCode 
	);
    }

    rightRootDesc->nonVegEquals().insert( tmpDesc );
  }

  if(leftReduction != csOne)
    rootColStats->scaleHistogram(leftReduction);

  if(rightReduction != csOne)
    rootColStats->scaleHistogram(rightReduction);

  // get the aggregate results following the latest merge
  newRowcount = rootColStats->getRowcount();
  newUec      = rootColStats->getTotalUec();

  // If user specified selectivity for this predicate, we need to make
  // adjustment in reduction to reflect that.
  rootStatDesc->applySelIfSpecifiedViaHint(pred, rowCountBeforePreds);

  // --------------------------------------------------------------------
  // For histograms that belong to the same equivalence class:
  //   IF this is NOT an Outer Join keep only the root
  //   IF this is an Outer Join, the original histograms need to be
  //      replaced by copies of the root; they will be null-augmented.
  //
  // This process is complicated by our prior separation of left-right.
  // --------------------------------------------------------------------
  //If under a or we did not merge so should not remove.
  CollIndex rightCount = 0;
  CollIndex leftCount = 0;
  if ( exprOpCode != ITM_OR )
  {
    rightCount = rightStatsToMerge.entries();
    leftCount  = leftStatsToMerge.entries();
  }

  CollIndex nextToMerge;

  // Walk the list backwards to avoid invalidating entries in the
  // statsToMerge sets
  while ( rightCount > 0 || leftCount > 0 )
  {
    if ( rightCount > 0 && leftCount > 0 )
    {
      if ( rightStatsToMerge[rightCount-1] > leftStatsToMerge[leftCount-1] )
      {
	nextToMerge = rightStatsToMerge[rightCount-1];
	rightCount--;
      }
      else
      {
	nextToMerge = leftStatsToMerge[leftCount-1];
	leftCount--;
      }
    }
    else if ( rightCount > 0 )
    {
      nextToMerge = rightStatsToMerge[rightCount-1];
      rightCount--;
    }
    else // leftCount > 0
    {
      nextToMerge = leftStatsToMerge[leftCount-1];
      leftCount--;
    }

    if ( nextToMerge != rootIndex )
    {
      if ( mergeMethod == OUTER_JOIN_MERGE &&
	   nextToMerge >= numOuterColStats )
      {
	ColStatsSharedPtr updateColStats =
	  (*this)[nextToMerge]->getColStatsToModify();
	updateColStats->overwrite( *rootColStats );

	// Avoid having duplicate entries in the nonVegEquals_ of
	// the root and the updated ColStatDesc.
	if ( (*this)[nextToMerge]->nonVegEquals().entries() > 0 )
	{
	  (*this)[nextToMerge]->nonVegEquals().clear();
	}
      }
      else
      {
	removeAt( nextToMerge );

	// maintain the numOuterColStats value when removing
	// column statistics from the outer table's list.
	if ( nextToMerge < numOuterColStats )
	  numOuterColStats--;
      }
    }
  } // while

  // $$$ WE NO LONGER WANT TO NORMALIZE HISTOGRAMS AFTER EVERY
  // $$$ PREDICATE!
  //
  // Instead, we will normalize all of the histograms' rowcounts after
  // we have applied all of the predicates.

  return TRUE;

} // ColStatDescList::applyPred

// -----------------------------------------------------------------------
// ColStatDescList::applyDefaultPred
// A CSDL::estimateCardinality() subroutine
//
// This routine deals with all those predicates the two earlier routines
// don't know how to deal with.
//
// NOTE: It is Presumed that this routine is only called with predicates
// for which a default selectivity should be applied.
//
// NOTE: as elsewhere, it is assumed that if a predicate can be pushed
// down to one instance of a column's histogram it has been pushed down to
// all 'identical' instances of that column's histograms.
//
// I.e., if default selectivity was applied to 1 instance of some
// histogram it has been applied to all same-group instances of that
// histogram to which it can be applied.
//
// NOTE: One oddity of this routine is that if the histogram impacted by a
// default predicate is 'real', default selectivity is applied once for
// each distinct predicate.  However, if the histogram is 'fake', default
// selectivity is only applied once for each Type of predicate.
//
// FUTURE WORK: Eventually, it ought to be possible to associate different
// default selectivity with different joins.  Right now, all default joins
// apply the same default selectivity.
//
// This is the new version of this function which facilitates applying all
// predicates at once.
// -----------------------------------------------------------------------

// $$$ difference between this routine and the original :
//
// 1. When the predicate in question has not been previously applied, only
// apply it to the histogram it affects, and then DO NOT normalize all
// histograms to have that resulting rowcount
//
// 2. In the case of a global predicate (one that does not apply to any
// particular histogram), we use the 'globalPredicateReduction' out
// parameter to communicate with the calling routine that we need to apply
// a global reduction to all histograms --> but LATER

void
ColStatDescList::applyDefaultPred (ItemExpr *pred,
                                   CostScalar & globalPredicateReduction,
                                   OperatorTypeEnum exprOpCode,
                                   CostScalar *maxSelectivity)
{
  // -------------------------------------------------------------------
  // If any operand of this default predicate is an ITM_HOSTVAR *and* if
  // that ITM_HOSTVAR isSystemGenerated(), or it is a selectivity param,
  // then this predicate is to be treated (for purposes of estimating
  // selectivity) as a no-op.
  //
  // Note Selectivity Params are processed in
  // ColStatDescList::applyVEGPred() and ColStatDescList::applyPred().
  //
  // In ColStatDescList::applyPred(), predicates containing selectivity
  // params are identified through the virtual method
  // SelParameter::castToConstValue().
  // -------------------------------------------------------------------
  for ( Int32 arity = 0; arity < pred->getArity(); arity++ )
  {
    const ItemExpr * operand = (*pred)[arity].getPtr();
    if ( (operand->getOperatorType() == ITM_HOSTVAR) &&
	 ((HostVar *) operand)->isSystemGenerated() ||
         (
         operand->getOperatorType() == ITM_CACHE_PARAM &&
         operand->castToSelParameter()
         )
       )
    {
      return;   // no further processing needed.
    }
  }

  const ValueId & origPredValueId = pred->getValueId();

  CostScalar defaultSel     = csOne;  // starting assumptions
  NABoolean alreadyApplied  = FALSE;
  NABoolean globalPredicate = TRUE;
  NABoolean statsExist = FALSE;

  OperatorTypeEnum op = pred->getOperatorType();

  if  (op == ITM_ONE_ROW || op == ITM_ONE_TRUE || op == ITM_ANY_TRUE)
  {
    pred = pred->child(0);
    op = pred->getOperatorType();
  }

  NABoolean isAFalsePred = FALSE;
  if(op == ITM_CONSTANT)
  {
    const ConstValue * constPred = (ConstValue *)pred->castToItemExpr();
    NAString constPredStr(constPred->getRawText()->data());
    constPredStr = constPredStr.strip(NAString::both);
    if(constPredStr == "0.")
      isAFalsePred = TRUE;
  }


  // fix soln 10-080605-3680: maxSelectivity(false) should be 0
  if (maxSelectivity && (op == ITM_RETURN_FALSE || isAFalsePred))
    {
      *maxSelectivity = MINOF(csZero, *maxSelectivity);
    }

  if (CmpCommon::getDefault(COMP_BOOL_107) == DF_ON)
  {
    if ( pred->synthSupportedOp() )
      alreadyApplied = pred->applyDefaultPred
        (*this, exprOpCode, origPredValueId, globalPredicate,
         maxSelectivity);
    else
      {
        // no need to pass maxSelectivity here because
        // maxSel(unsupported default predicate) == 1.0
        alreadyApplied = pred->applyUnSuppDefaultPred
          (*this, origPredValueId, globalPredicate);
      }
  }
// LCOV_EXCL_START :cnu
  else
  {
  //
  // CASE 1 : predicate supports synthesis
  //
  if ( pred->synthSupportedOp() ) // does the operator support synthesis??
  {
	// get the histogram on which the predicate is to be applied
	ValueId column;

	// leftColIndex contains the position of the left histogram whose statistics 
	// will be used for computing selectivity. 
	// In case the left child contains more than one columns, 
	// it would be the position of histogram with max UEC amongst left child
	CollIndex leftColIndex;

	// following two sets contain the leaf values of the respective children
	ValueIdSet leftLeafValues;
	ValueIdSet rightLeafValues;

	// This boolean will be set to TRUE if it is an equality predicate with more
	// than one column involved and COMP_BOOL_40 is ON.
	// When counting the number of columns, it takes a combined set of the
	// columns from the left and the right children
	NABoolean equiJoinWithExpr = FALSE;

	if (op == ITM_VEG_PREDICATE)
	{
	  // could be a VEG predicate with no children
	  CollIndexList statsToMerge(STMTHEAP);

	  // locate entries in this ColStatDescList that are associated
	  // with the current VEG predicate.
	  statsExist =
		identifyMergeCandidates( pred, leftColIndex, statsToMerge );
	}
	else
	{
	  if ( (op == ITM_EQUAL) &&
		   (CmpCommon::getDefault(COMP_BOOL_40) == DF_ON))
	  {
		// if COMP_BOOL_40 == ON; for equality expressions involving 
		// more than one columns, we pick the column with Max UEC
		
		ItemExpr *rightChild = pred->child(1);
		ItemExpr *leftChild = pred->child(0);

		rightChild->findAll(ITM_VEG_REFERENCE, rightLeafValues, TRUE, TRUE);

		leftChild->findAll(ITM_VEG_REFERENCE, leftLeafValues, TRUE, TRUE);

		// get leftColIndex for column with Max UEC. In case of predicates
		// like a + 4 = 10, it will be the index for col 'a'

		if ( ((rightLeafValues.entries() + leftLeafValues.entries()) > 1) && 
		   (rightLeafValues != leftLeafValues))
		{
		  statsExist = getColStatDescIndexForColWithMaxUec(leftColIndex, leftLeafValues);

		  // if there are more than one columns involved in the expression, consider 
		  // histogram with MaxUEC to compute selectivities. This would include cases 
		  // where leftChild has two columns while right child is a constant.
		  // Example (col1 + col2) = 10; and col1 = col2 + col3; and Fn(col1) = Fn(col2).
		  
		  // For predicates like col1 = 10 or col1 + 4 = 10
		  // use Max frequency as the selectivity

		  if ( (statsExist) && 
		       ( (exprOpCode != REL_SCAN) ||
		         (CmpCommon::getDefault(COMP_BOOL_74) == DF_OFF) ) )
			equiJoinWithExpr = TRUE;
		} // equi-join
		else
		{
		  // Is a local predicate with column only on the left child
		  leftChild = leftChild->getLeafValueIfUseStats();
		  leftLeafValues.clear();

		  leftChild->findAll(ITM_BASECOLUMN, leftLeafValues, TRUE, TRUE);
		  column = leftChild->getValueId();

		  if (getColStatDescIndexForColumn(leftColIndex, column) )
			  statsExist = TRUE;
		}
	  } // equal predicate and comp_bool_40 == DF_ON
	  else
	  {
		// for all other predicates with arity > 0
		if (pred->getArity() > 0)
		{
		  ItemExpr * leftChild = pred->child(0);

		  leftChild = leftChild->getLeafValueIfUseStats();
		  column = leftChild->getValueId();

		  leftChild->findAll(ITM_BASECOLUMN, leftLeafValues, TRUE, TRUE);

		  if (getColStatDescIndexForColumn(leftColIndex, column) )
			  statsExist = TRUE;
		} // end if predArity > 0
	  } // if non-equality-predicates or COMP_BOOL_40 == OFF 
	} // !ITM_VEG_PREDICATE

	if (statsExist)
	{
	  // Get the first column for which the histogram is found. 
	  // multi-column histograms no longer are running around the system, but
	  // there are still predicates that reach here
	  //
	  //   e.g., predicates involving aggregates (count(*) = 10)
	  //   e.g., predicates on host variables (?p = 10)
	  //
	  // ... probably many more

	  // CASE 1a : VEG pred
	  if ( op == ITM_VEG_PREDICATE ) // Transitive closure predicate
	  {
		// ----------------------------------------------------------
		// This logic replicates a portion to the logic used in
		// ColStatDescList::applyVEGPred to determine the parts of this
		// VEGPred for which 'real' histogram manipulation was NOT done.
		//
		// Default selectivity should be applied *once* for *each* valid
		// join that couldn't be executed because of its multi-column
		// nature, but which would have otherwise been a valid join.
		// (So long as an involved column hasn't already appeared in a
		// default EQ-join.)
		// ----------------------------------------------------------
		// $$$ FCS_ONLY kludge -- this code should be reviewed and
		// $$$ reconsidered post-FCS
		//
		// We reach this point when we're applying a predicate that looks
		// like <col> = hostvar.
		//
		// We may reach this point in other instances, too.  For now, we're
		// not going to worry about that.
		//
		// For this situation, we set the rowcount to be rc/uec, and set
		// uec to be 1.  This reduction is exactly what you'd expect
		// when applying an eqpred with a hostvar.
		//
		ColStatDescSharedPtr rootStatDesc = (*this)[leftColIndex];
		ColStatsSharedPtr rootColStats = rootStatDesc->getColStatsToModify();

		CostScalar oldUec  = rootColStats->getTotalUec();
		CostScalar oldRows = rootColStats->getRowcount();

		CostScalar newRows = oldRows / oldUec;

                // compute maxSelectivity before histograms get modified
                if (maxSelectivity)
                  {
                    VEG* veg = ((VEGPredicate*)pred)->getVEG();
                    const ValueIdSet & values = veg->getAllValues();
                    // we must distinguish between "X=?" and "X=Y".
                    ItemExpr *cExpr = NULL;
                    if (!values.referencesAConstExpr(&cExpr))
                      { // veg is an "X=Y" predicate
                        // maxSelectivity("X=Y") == 1.0
                      }
                    else // veg is an "X=?" predicate
                      {
                        // maxFreq = maxFrequency(X) for VEGPred "X=?"
                        // NB: "maxFreq = histograms.getMaxFreq(v);" may look
                        // attractive here. But, beware: it causes many maxCard
                        // tests to fail in compGeneral/test015 -- they get 0.
                        CostScalar maxFreq = csMinusOne;
                        for (ValueId v = values.init();    
                             values.next(v); 
                             values.advance(v))
                          {
                            if (v.getItemExpr()->getOperatorType() == 
                                ITM_BASECOLUMN)
                              {
                                maxFreq = getMaxFreq(v);
                                if (maxFreq > csMinusOne)
                                  break;
                              }
                          }
                        // maxSelectivity("X=?") == max frequency(X) / total rows
                        if (maxFreq > csMinusOne && 
                            rootColStats->getRowcount() > csZero)
                          *maxSelectivity = 
                            MINOF(maxFreq / rootColStats->getRowcount(),
                                  *maxSelectivity);
                      } // veg is an "X=?" predicate
                  } // maxSelectivity != NULL

		rootStatDesc->synchronizeStats(
		  oldRows,
		  newRows,
		  ColStatDesc::SET_UEC_TO_ONE
		  );
		alreadyApplied = TRUE;
	  } // 1a: veg pred

	  // CASE 1b : equal, not equal
	  else if ( ( op == ITM_EQUAL ) || ( op == ITM_NOT_EQUAL ) )
	  {
		ColStatDescSharedPtr leftStatDesc = (*this)[leftColIndex];

		if (equiJoinWithExpr)
		{
		  // Control comes here for equality predicates with more 
		  // than one column involved. The columns could be from 
		  // both children or from only left child

		  globalPredicate = FALSE; // not a global predicate
		  if (NOT(leftStatDesc->isPredicateApplied(origPredValueId) ) )
		  {
			CostScalar fudgeFactorForAggFn ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_6));

			// rightLeafValues > 0 and leaftLeafValues > 0 or <col1> = <col2>
			// col1 = Fn(col2, col3) OR Fn(col1, col2) = col3
			// selectivity is equal to 1/MAXUEC of the columns 
			// participating in the query
			// get histograms from right child. In case there are
			// more than one columns in the right child, get the histogram 
			// with max UEC

			ColStatsSharedPtr rightColStat;
			CollIndex rightColIndex;

			ColStatsSharedPtr leftColStat = leftStatDesc->getColStats();
			NABoolean leftColStatReal = !leftColStat->isOrigFakeHist();
			NABoolean rightColStatReal = FALSE;

			// UEC of the children child and maxUEC
			CostScalar rightUec = csMinusOne;
			CostScalar leftUec = leftColStat->getTotalUec();;

			// If we reached here, it is guaranteed that we have histograms for the
			// left child. But, we cannot say anything for sure for the right child.
			// Hence check if there exists any column in the right side of the equality
			// predicates.

			// if rightside has any column, get the histograms for the right child
			if ((rightLeafValues.entries() > 0) && 
			    (getColStatDescIndexForColWithMaxUec(rightColIndex, rightLeafValues)) )
			{
			  // out of all these columns, pick the one with Max UEC. While doing
			  // that it also makes sure that it is comparing the default UEC to
			  // default UEC and actual UEC to actual one of both children
			  rightColStat = (*this)[rightColIndex]->getColStats();
			  rightUec = rightColStat->getTotalUec();
			  rightColStatReal = !rightColStat->isOrigFakeHist();
			} // if getColIndex for rightLeafValues

			// check for aggregate function for both children 
			// and adjust UECs accordingly
			// We would have ideally like to compute selectivity for
			// aggregate functions by considering UEC for only those 
			// columns which are a children of Aggregate functions.
			// Because of time constraint and not being able to find
			// an inexpensive way to handle that, we are now taking the 
			// cardinality of the column with MAX UEC from the child
			// and in case of an aggregate, multiply with a fudge factor
			// Hence there could be cases where for predicates like 
			// "a + max(b) = c", we might pickup col 'a' as that has the
			// highest UEC and multiple cardinality of that by the fudge 
			// factor. If columns 'a' and 'b' belog to the same table, 
			// or have already been joined, it would not matter which
			// column we pickup, but in some cases it might poor estimates

			if (pred->child(0)->containsAnAggregate())
			{
			  leftUec = (leftUec * fudgeFactorForAggFn).minCsOne();
			}

			if (rightColStat && pred->child(1)->containsAnAggregate())
			{
			  rightUec = (rightUec * fudgeFactorForAggFn).minCsOne();
			}

			// To get the maxUec for selectivity, we don't want to compare
			// real UEC with the fake one. Hence, also check for fakeness
			// before comparing

			if (leftColStatReal && !rightColStatReal)
			  defaultSel = csOne/leftUec;
			else
			  if (rightColStatReal && !leftColStatReal)
				defaultSel = csOne/rightUec;
			  else
				defaultSel = csOne/MAXOF(leftUec, rightUec);

			if( (pred->child(0)->getOperatorType() == ITM_DAYOFWEEK) ||
			    (pred->child(1)->getOperatorType() == ITM_DAYOFWEEK) )
			    defaultSel = csOne/7;

			// Since the histograms have not been merged, we don't know which
			// one will be finally picked up later for parent node, based on
			// the characteristics output. Hence set the aggregate information
			// of all correctly. Rowcount and UEC for all will be done automatically
			// during synchronizeStats. appliedPreds is the only that needs to be
			// correctly set

			ValueIdSet columnWithPreds(leftLeafValues);
			columnWithPreds.insert(rightLeafValues);

			addToAppliedPredsOfAllCSDs( columnWithPreds, origPredValueId );

			// This will modify the rowcount, which should be done for only
			// one histogram. Remaining histograms will be synchronized later
			leftStatDesc->applySel( defaultSel );
		  } // NOT(isPerdicateApplied)
		  else
		  {
			// predicate has already been applied
			alreadyApplied = TRUE;
		  }
		} // equiJoinWithExpr
		else
		{
		  // go the usual way. Control comes here for equality predicates, if 
		  // COMP_BOOL_40 is OFF or for cases where we have a local predicate
		  // wih one column in the left child.

		  ////////////////////////////////////////////////////////////////////
		  // do not need to worry about selectivity constant parameters here
		  // because this branch is for predicates with ORs: a=? OR b=?. Each 
		  // predicate factor such as a=? is processed once by this function.  
		  // Since OR predicate is cacheable but not parameterizable, it is 
		  // impossible to have a selectivity constant parameter situation in
		  // the item expression tree. That is, there is no need to search
		  // for a selectivity constant parameter and to use its selectivity 
		  // value instead.
		  ////////////////////////////////////////////////////////////////////
		  if (leftLeafValues.entries() == 1)
		  {
			// First Question: is the predicate of the form
			//   "<col> <op> <expression>"  or  "<expression> <op> <col>"?
			//
			// Earlier logic places stand-alone columns on the left, so look
			// for a histogram associated with the left-hand valueId.
			//
			// Of course, the trick here is that the following works even if
			// the left-hand ValueId isn't for a column.

			globalPredicate = FALSE; // not a global predicate
		
			if ( NOT ( leftStatDesc->isPredicateApplied( origPredValueId ) ) )
			{ // first time for this histogram

			  rightLeafValues.clear();
			  ItemExpr * rhs = pred->child(1);

			  rhs = rhs->getLeafValueIfUseStats();
			  rhs->findAll( ITM_BASECOLUMN, rightLeafValues, TRUE, TRUE );

			  // There is only one column in the predicate or there is one 
			  // column on the right hand side of the predicate and
			  // this column is same as the left side column, use Uec
			  // of the column instead of using default statistics

			  if  (( rightLeafValues.isEmpty() )  ||  
			   ( (rightLeafValues.entries() == 1) && (leftLeafValues == rightLeafValues ) ) )
			  { 
				// <col> <op> <constant> or <col> <op> <col>, 
				// where the left and the right col are same
				// If this is a 'fake' histogram, the selectivity
				// assumed is the default selectivity associated
				// with the current predicate.  (But, don't apply
				// the same type of predicate multiple times.)
				if ( leftStatDesc->getColStats()->isFakeHistogram() )
				{
				  defaultSel =
					(leftStatDesc->isSimilarPredicateApplied( op ) ?
					csOne : pred->defaultSel() );
				}
				else
				{ // not a 'fake histogram'
				  // Determine the rowcount of the non-NULL value
				  // with the greatest/least rowcount.
				  ColStatsSharedPtr colStatsP = leftStatDesc->getColStats();
				  HistogramSharedPtr histP    = colStatsP->getHistogram();
				  const CostScalar & rowRedF = colStatsP->getRedFactor();

				  const CostScalar & rowCountBeforePred = (colStatsP->getRowcount()).minCsOne();

				  CostScalar maxRowCount = csOne;
				  CostScalar minRowCount = rowCountBeforePred;
				  CostScalar tmpRowCount;
				  CostScalar uec;

				  Interval iter = histP->getFirstInterval();

				  while ( iter.isValid() && !iter.isNull() )
				  {
					// uec must be at least 1 for these
					// calculations since we don't want to get
					// a huge blowup in rowcount
					if ( iter.getUec().isZero() )
					{
					  iter.next();
					  continue; // avoid divide-by-zero!
					}

					CostScalar iRows = rowRedF * iter.getRowcount();

					uec = (MINOF(iRows, iter.getUec())).minCsOne();

					tmpRowCount =
					  iRows / uec;

					if ( tmpRowCount > maxRowCount )
					  maxRowCount = tmpRowCount;

					if ( tmpRowCount < minRowCount )
					  minRowCount = tmpRowCount;

                                        iter.next();

				  } // end while iter() is valid

				  if ( op == ITM_EQUAL )
				  {
					defaultSel = maxRowCount / rowCountBeforePred;
                                        if (maxSelectivity) 
                                          {
                                            // maxSelectivity(x=?) == max frequency / total rows
                                            *maxSelectivity = 
                                              MINOF(defaultSel,
                                                    *maxSelectivity);
                                          } // maxSelectivity != NULL
				  }
				  else
				  {
					// With ITM_NOT_EQUAL, and no Histogram, avoid
					// setting defaultSel to zero:
					CostScalar numer;
					CostScalar denom;

					if ( minRowCount == rowCountBeforePred )
					{
					  numer = MAXOF( rowCountBeforePred, csTwo ) - csOne;
					  denom = MAXOF( rowCountBeforePred, csTwo );

					  defaultSel = numer / denom;
					}
					else
					{
					  numer = minRowCount;
					  denom = rowCountBeforePred;

					  defaultSel = csOne - ( numer / denom );

                                          // maxSelectivity(x<>?) == 1.0
                                          // which means do nothing here because 1.0 has
                                          // already been set as the default maxSelectivity
                                          // just before the estimateCardinality() call.
					}
				  } // op == ITM_NOT_EQUAL
				}  // not a 'fake histogram'
			  } // The Operand is a constant expression.
			  else // i.e., NOT leafValues.isEmpty()
			  {
				// <col1> <op> <col2>, or <col1 + col2> <op> <col3>
				// or <col1> <op> <col2 + col3>
				// The operand involves more than one column, which
				// makes this an equality join that we are not now
				// able to evaluate.
				// Note that the current predicate-based defaultSel
				// routine is not used in this case.....
				if ( op == ITM_EQUAL )
				{
				  if (leftStatDesc->isSimilarPredicateApplied( op ) )
					defaultSel = csOne;
				  else
				  {
					if ( (rightLeafValues.entries() == 1) &&
					     (exprOpCode != REL_SCAN) )
					{
					  // <col1> = <col2>
					  // we already know that left side has one column. This is the case
					  // col1 Join col2

					  CollIndex rightColIndex;

					  if (getColStatDescIndexForColumn(rightColIndex, rhs->getValueId()) )
					  {
						ColStatDescSharedPtr rightStatDesc = (*this)[rightColIndex];

						ColStatsSharedPtr leftColStats = leftStatDesc->getColStats();
						ColStatsSharedPtr rightColStats = rightStatDesc->getColStats();

						CostScalar maxUec = (MAXOF(leftColStats->getTotalUec(), 
												  rightColStats->getTotalUec())).minCsOne();

						defaultSel = csOne/maxUec;
					  } // colStat for column found
					  else
					  {
						// histogram does not exist use default selectivity for Join equal
						defaultSel = CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_EQUAL );
					  }
					} // rightLeafValueEntries = 1
					else
					{
					  // right side has more than one columns, or it is a scan. 
					  // Use default join equal selectivity for <col1> = <col2, col3>
					  // and hist_no_stats_uec for scan
					  if (exprOpCode == REL_SCAN)
					    defaultSel = (1.0/CURRSTMT_OPTDEFAULTS->defNoStatsUec());
					  else
					    defaultSel = CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_EQUAL );
					}
				  }
				}
				else
				{ // op == ITM_NOT_EQUAL. Apply default selectivity
				  if (leftStatDesc->isSimilarPredicateApplied( op ) )
				    defaultSel = csOne;
				  else
				  {
				    if (exprOpCode == REL_SCAN)
				      defaultSel = 1 - (1.0/CURRSTMT_OPTDEFAULTS->defNoStatsUec() );
				    else
                                      defaultSel = (1 - CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_EQUAL ) );
				  }
				}
			  } // NOT leafValue.isEmpty()
			  leftStatDesc->addToAppliedPreds( origPredValueId );
			  leftStatDesc->applySel( defaultSel );
			}
			else
			{ // predicate has already been applied
			  alreadyApplied = TRUE;
			}
		  } // column is leading prefix of histogram
		} // 1b: equal, not equal
	  } // !equiJoinWithExpr

	  // CASE 1c : less, less_eq, greater, greater_eq
	  else if (	 op == ITM_LESS
			OR op == ITM_LESS_EQ
			OR op == ITM_GREATER
			OR op == ITM_GREATER_EQ
		  )
	  {
		ItemExpr * rhs = pred->child(1);
		rhs = rhs->getLeafValueIfUseStats();

		// First Question: is the predicate of the form
		//   "<col> <op> <expression>"  or  "<expression> <op> <col>"?

		globalPredicate = FALSE;   // not a 'global' predicate
		ColStatDescSharedPtr statDesc = (*this)[leftColIndex];

		if ( NOT ( statDesc->isPredicateApplied( origPredValueId ) ) )
		{ // first time for this histogram
		  defaultSel = statDesc->selForRelativeRange (op, column, rhs);
		  
		  // defaultSel is one for range predicates on char or varchar 
		  // column types.
		  // if a similar predicate has already been applied to this 
		  // histogram, then we don't want to reduce the rowcount and
		  // uec further. Therefore we return the selectivity equal to
		  // one. Following two predicates are said to be similar:
		  // 1. < and <=
		  // 2. > and >=
		  // 3. If the two range predicates are derived from LIKE predicate

		  rightLeafValues.clear();

		  rhs->findAll( ITM_BASECOLUMN, rightLeafValues, TRUE, TRUE );

		  // There is only one column in the predicate or there is one 
		  // column on the right hand side of the predicate and
		  // this column is same as the left side column, use Uec
		  // of the column instead of using default statistics

		  if  (( rightLeafValues.isEmpty() )  ||  
		   ( (rightLeafValues.entries() == 1) && (leftLeafValues == rightLeafValues ) ) )
		  {
			// pred <col> operatort <expression>
			if ( defaultSel == csOne )
			{
			  if (statDesc->derivOfLikeAndSimilarPredApp(pred) ||
				  statDesc->isSimilarPredicateApplied( op ) )
				defaultSel = csOne;
			  else
			  {
				BiRelat *br = (BiRelat *) pred;
				if (br->derivativeOfLike())
				  defaultSel = br->getLikeSelectivity();
				else
				  defaultSel = pred->defaultSel();
			  } // else statDesc->similarPredApplied
			} // if defaultSel == csOne
		  } // if rightLeafValues.entries > 1 or leftLeafValues != rightLeafValues
		  else
		  {
			// not a leaf value. Predicate is <col> operator <col>
			defaultSel =
			  ( statDesc->isSimilarPredicateApplied( op ) ?
			  csOne : CostPrimitives::getBasicCostFactor( HIST_DEFAULT_SEL_FOR_JOIN_RANGE ) );
		  } // end of join range

		  statDesc->addToAppliedPreds( origPredValueId );
		  statDesc->applySel( defaultSel );
		}
		else
		{
		  alreadyApplied = TRUE;
		}
	  } // 1c: less, less_eq, greater, greater_eq

	  // CASE 1d : null, not null, unknown, not unknown
	  else if (	 op == ITM_IS_NULL
			OR op == ITM_IS_NOT_NULL
			OR op == ITM_IS_UNKNOWN
			OR op == ITM_IS_NOT_UNKNOWN
		  )
	  {
		globalPredicate = FALSE;   // not a 'global' predicate
		ColStatDescSharedPtr statDesc = (*this)[leftColIndex];

		if ( NOT ( statDesc->isPredicateApplied( origPredValueId ) ) )
		{ // first time for this histogram
		  defaultSel = ( statDesc->isSimilarPredicateApplied( op ) ?
				 csOne : pred->defaultSel() );

		  statDesc->addToAppliedPreds( origPredValueId );
		  statDesc->applySel( defaultSel );
		}
		else
		{
		  alreadyApplied = TRUE;
		}
	  } // 1d: null, not null, unknown, not unknown

	  // CASE 1e : or, and
	  else if ( ( op == ITM_OR) || ( op == ITM_AND ) )
	  {
		// Don't do anything with this predicate right here, right now.
		alreadyApplied = TRUE;
	  }  // op is AND, or OR

	  // CASE 1f : should never occur!
	  else
	  {
		DCMPASSERT( FALSE ); // unexpected condition!
		alreadyApplied = TRUE;
	  }
	} // if !alreadyApplied
  } //  if statsExist
  //
  // CASE 2 : predicate does not support synthesis
  //
  else
  {
    if ( pred->getArity() > 0 )
    {
	  NABoolean isOpTypeNot = FALSE;  
	  ItemExpr * tempPred;
	  OperatorTypeEnum tempOp;
	  
	  ValueIdSet leftLeafValues;

	  ItemExpr * leftChild = pred->child(0);

	  if(pred->getOperatorType() == ITM_NOT)
	  {
	    isOpTypeNot = TRUE;
	    tempOp = leftChild->getOperatorType();
	    tempPred = leftChild;
	    leftChild = leftChild->child(0);
	  }
	  else
	  {
	    tempOp = op;
	    tempPred = pred;
	  }

	  leftChild = leftChild->getLeafValueIfUseStats();

	  leftChild->findAll(ITM_BASECOLUMN, leftLeafValues, TRUE, TRUE);

	  if (!leftLeafValues.isEmpty())
	  {
		// First Question: is the predicate of the form
		//   "<col> <op> <expression>"  or  "<expression> <op> <col>"?
		// Earlier logic places stand-alone columns on the left, so look
		// for a histogram associated with the left-hand valueId.
		//

		CollIndex leftColIndex;

		if (getColStatDescIndexForColumn(leftColIndex, leftChild->getValueId()) )
		{
		  globalPredicate = FALSE;   // not a 'global' predicate
		  ColStatDescSharedPtr statDesc = (*this)[leftColIndex];

		  if ( NOT ( statDesc->isPredicateApplied( origPredValueId ) ) )
		  { 
			// first time for this histogram
			// if the predicate is a LIKE predicate with no wild cards
			// in the pattern. And for some reason could not be transformed
			// into an equality predicate, then set its selectivity equal to
			// 1/UEC, else go the usual way
		  
			if ( (tempOp == ITM_LIKE) && 
			   ((Like *)tempPred)->isPatternAStringLiteral())
			{
			  ColStatsSharedPtr colStat = statDesc->getColStats();

			  if (colStat->isFakeHistogram())
				defaultSel = 1.0/(CURRSTMT_OPTDEFAULTS->defNoStatsUec()) ;
			  else
			  {
				CostScalar tempUec = colStat->getTotalUec();

				//To guard against div-by-zero assertion
				if(tempUec == csZero)
				  tempUec = 1;
				
				defaultSel = 1.0/tempUec.value();
				defaultSel.maxCsOne();
			  }


			  if(isOpTypeNot)
			    defaultSel = 1 - defaultSel.value();
			}
			else
			  defaultSel = pred->defaultSel();

			statDesc->addToAppliedPreds( origPredValueId );
			statDesc->applySel( defaultSel );
		  } // NOT isPredicateApplied
		  else
		  {
			alreadyApplied = TRUE;
		  }
		} // column is leading prefix of histogram
	  }
    } // if ( pred->getArity() > 0 )
    else
    {
      defaultSel = pred->defaultSel();
    }
  }
  }
// LCOV_EXCL_STOP

  // maxSelectivity computation is done
  if (maxSelectivity) return;

  //
  // we do the following whether the predicate supports synthesis or not
  //
  if ( globalPredicate )
  {
    if(pred->isSelectivitySetUsingHint())
    {
      defaultSel = pred->getSelectivityFactor();
      (*this)[0]->getColStats()->setSelectivitySetUsingHint();
    }
    else
      // not a local predicate, yet still a default predicate
      // ==> should mean: No statistics.
      defaultSel = pred->defaultSel();

    // *******************************************************************
    // $$$ this code should go away sooner or later (when the normalizer /
    //     constant-folding do everything they should ...)
    //
    // until then, handle a few simple braindead cases here that should
    // already have been taken care of
    if ( pred->getArity() == 2 )
    {
      NABoolean negate = FALSE;
      ConstValue * lhs = pred->child(0)->castToConstValue( negate );
      ConstValue * rhs = pred->child(1)->castToConstValue( negate );
      if ( lhs != NULL && rhs != NULL )
      {
	EncodedValue left ( lhs, negate );
	EncodedValue right( rhs, negate );

	if ( left == right )
	{
// LCOV_EXCL_START - cnu
	  OperatorTypeEnum op = pred->getOperatorType();
	  switch ( op )
	  {
	  case ITM_EQUAL:         // 1 == 1
	  case ITM_LESS_EQ:       // 1 <= 1
	  case ITM_GREATER_EQ:    // 1 >= 1
	    return; // pred has no effect on selectivity!

	  case ITM_NOT_EQUAL:     // 1 <> 1
	  case ITM_GREATER:       // 1 >  1
	  case ITM_LESS:          // 1 <  1
	    defaultSel = csZero ; // pred removes all rows!
	    break;

	  default:
	    break;
	  }
// LCOV_EXCL_STOP
	}
      }
    } // $$$ end of stuff that should someday be removed ...
    // *******************************************************************

  } // globalPredicate

  // $$$ WE NO LONGER WANT TO NORMALIZE HISTOGRAMS AFTER EVERY
  // $$$ PREDICATE!
  //
  // Instead, we will normalize all of the histograms' rowcounts after
  // we have applied all of the predicates.
  //
  // However, in the case of a global predicate (read: one that does not
  // apply to any particular histogram or histograms), we apply its
  // selectivity to all ColStatDesc's.

  if ( NOT alreadyApplied && globalPredicate )  // don't redo what's already been done.
  {
    if(isAFalsePred)
      defaultSel = csZero;

    // Don't apply the selectivity here!  Instead, we return it to the
    // calling function (via the out-parameter globalPredicateReduction) ;
    // later on, we will go through the histograms and apply it to all
    // of them, at the same time that we apply all of the predicate
    // selectivities to all of them (end of estimateCardinality).
    //
    // NOTE: if we apply this selectivity here, to all histograms, the
    // end result is that it appears that each histogram had a separate
    // _x_ reduction applied to it, so the total result of the predicate
    // will be _x_^n, where n is the number of histograms!
    globalPredicateReduction = defaultSel;
  }
  else
  {
    globalPredicateReduction = csOne;
  }
} //ColStatDescList::applyDefaultPred

// -----------------------------------------------------------------------
// ColStatDescList::getMaxFreq
// get maximum frequency for the given col
//
// -----------------------------------------------------------------------

CostScalar
ColStatDescList::getMaxFreq(ValueId col)
{
  CollIndex index;

  NABoolean found = getColStatDescIndexForColumn( index, col );

  // histogram not found, return
  if ( found == FALSE )
	return -1.0;

  ColStatsSharedPtr colStats = (*this)[index]->getColStats();

  CostScalar freq = colStats->getMaxFreq();
  if (freq <= csZero)
  {
	ColStatsSharedPtr colStatsModifiable = (*this)[index]->getColStatsToModify();
	colStatsModifiable->computeMaxFreqOfCol(TRUE);
	freq = colStatsModifiable->getMaxFreq();
  }
  return freq;
}

CostScalar ColStatDescList::getUEC(ValueId col)
{
  CollIndex index;
  NABoolean found = getColStatDescIndexForColumn( index, col );

  // histogram not found, return 1
  if ( found == FALSE )
    return 1.0;

  ColStatsSharedPtr colStats = (*this)[index]->getColStats();
  CostScalar uec = colStats->getTotalUec();
  if (uec < 1.0)
    uec = 1.0;
  return uec;
}

// -----------------------------------------------------------------------
// ColStatDescList::getMaxOfMaxFreqOfCol
// get maximum frequency for the given column set
//
// -----------------------------------------------------------------------

CostScalar ColStatDescList::getMaxOfMaxFreqOfCol(const ValueIdSet & baseColSet)
{
  CostScalar maxFreq = csZero;
  CostScalar freq = csZero;

  for (ValueId col = baseColSet.init();
			baseColSet.next(col);
			baseColSet.advance(col))
  {
    if (col == NULL_VALUE_ID)
      continue; // LCOV_EXCL_LINE - rfi

	  ColStatsSharedPtr colStat = this->getColStatsPtrForColumn (col);
	  ColAnalysis * colAnalysis = col.colAnalysis();

	  // for higher way joins, if the base column does not appear as
	  // characteristic output, its histogram will not be cached. In that
	  // case we shall use the total frequency when that histogram last appeared
	  // in the list

	  if (colStat == NULL)
	  {
	    if (colAnalysis &&
		   (colAnalysis->getMaxFreq() != csMinusOne) )
	    {
	      freq = colAnalysis->getMaxFreq();
	      if (freq > maxFreq)
		maxFreq = freq;
	    }
	    else
	    {
	      // if there is no way we can get the UEC for partitioning
	      // column, set it equal to row count, and don't bother to
	      // look at other columns
	      maxFreq = csZero;
	      break;
	    }
	  }
	  else
	  {
            if (colStat->isOrigFakeHist())
            {
              maxFreq = csZero;
              break;
            }
	    freq = this->getMaxFreq(col);
	    if (freq > maxFreq)
		maxFreq = freq;
	  }
  }
  return maxFreq;
}

// -----------------------------------------------------------------------
// ColStatDescList::getMinOfMaxFreqOfCol
// get maximum frequency for the given column set
//
// -----------------------------------------------------------------------

CostScalar ColStatDescList::getMinOfMaxFreqOfCol(const ValueIdSet & baseColSet)
{
  CostScalar minFreq = COSTSCALAR_MAX;
  CostScalar freq = csOne;

  if (baseColSet.entries() == 0)
    return csZero;

  for (ValueId col = baseColSet.init();
			baseColSet.next(col);
			baseColSet.advance(col))
  {
    if (col == NULL_VALUE_ID)
      continue; // LCOV_EXCL_LINE

	  ColStatsSharedPtr colStat = this->getColStatsPtrForColumn (col);
	  ColAnalysis * colAnalysis = col.colAnalysis();

	  // for higher way joins, if the base column does not appear as
	  // characteristic output, its histogram will not be cached. In that
	  // case we shall use the total frequency when that histogram last appeared
	  // in the list

	  if (colStat == NULL)
	  {
	    if (colAnalysis &&
		   (colAnalysis->getMaxFreq() > csZero) )
	    {
		    freq = colAnalysis->getMaxFreq();
		    if (freq < minFreq)
		     minFreq = freq;
	    }
	    else
	    {
		    // if there is no way we can get the UEC for partitioning
		    // column, set it equal to row count, and don't bother to
		    // look at other columns
		    minFreq = csZero;
		    break;
	    }
	  }
	  else
	  {
            if (colStat->isOrigFakeHist())
            {
              minFreq = csZero;
              break;
            }

        freq = this->getMaxFreq(col);
        if (freq < minFreq)
		  minFreq = freq;
	  }
  }

  return minFreq;
}

// Max frequencies are computed slightly differently for Case expressions
// here if one of the leaves happens to be a constant, then we shall 
// assume the frequency to be one. A leaf value is the result of if-then-else
// Example: if <condition> then <leaf1> else <leaf 2>

CostScalar
ColStatDescList::getMaxFreqForCaseExpr(const ValueIdSet & leafValues)
{
  if(leafValues.entries() == 0)
  {
// LCOV_EXCL_START - rfi
    CCMPASSERT ( leafValues.entries() > 0 );

    // In absence of leaf values, it is not possible to calculate max freq.
    // Return zero to indicate uniform distribution.
    return csZero;
// LCOV_EXCL_STOP
  }

  CostScalar maxFreq = csZero;
  CostScalar freq = csZero;

  for (ValueId id = leafValues.init(); leafValues.next(id); leafValues.advance(id))
  {
    if (id.getItemExpr()->doesExprEvaluateToConstant(FALSE))
    {
       maxFreq = csOne;
       break;
    }
    else
    {
       freq = getMaxOfMaxFreqOfCol(id);
       if (freq > maxFreq)
          maxFreq = freq;
    }
  }

  // Max out the number of leaves to 5, to avoid getting very low frequencies.
  CostScalar maxFreqOfCaseExpr = csOne;
  double numOfLeaves = (double)(MINOF(leafValues.entries(), 5));

  // To avoid div-by-zero exception
  numOfLeaves = MIN_ONE(numOfLeaves);

  maxFreqOfCaseExpr = maxFreq * (1/numOfLeaves);

  return maxFreqOfCaseExpr;
}

// -----------------------------------------------------------------------
// ColStatDescList::getMinUec
//
// Returns the minimum UEC from the given column set
// -----------------------------------------------------------------------
CostScalar ColStatDescList::getMinUec(const ValueIdSet & baseColSet) const
{
  CostScalar uec = csMinusOne;
  CostScalar minUec = COSTSCALAR_MAX;

  // This is an error condition, which would later assume uniform distribution
  if (baseColSet.entries() == 0)
	return csMinusOne;

  for (ValueId col = baseColSet.init();
			baseColSet.next(col);
			baseColSet.advance(col))
  {
	ColStatsSharedPtr colStat = this->getColStatsPtrForColumn (col);

	// for higher way joins, if the base column does not appear as
	// characteristic output, its histogram will not be cached. In that
	// case we shall use the total UEC when that histogram last appeared
	// in the list

	if (colStat == NULL)
	{
	  ColAnalysis * colAnalysis = col.colAnalysis();
	  if (colAnalysis &&
		 (colAnalysis->getFinalUec() != csZero) )
	  {
		uec = colAnalysis->getFinalUec();
		if (uec < minUec)
		   minUec = uec;
	  }
	  else
	  {
		// if there is no way we can get the UEC for partitioning
		// column, set it equal to row count, and don't bother to
		// look at other columns
		minUec = csMinusOne;
		break;
	  }
	}
	else
	{
	  uec = colStat->getTotalUec();
	  if (uec < minUec)
	  {
	    minUec = uec;
	  }
	}
  }

  return minUec;
}

// -----------------------------------------------------------------------
// ColStatDescList::getMaxUec
//
// Returns the maximum UEC from the given column set. If histogram does
// not exist for any column, then return -1, as error condition.
// -----------------------------------------------------------------------
CostScalar ColStatDescList::getMaxUec(const ValueIdSet & baseColSet) const
{
  CostScalar uec = csMinusOne;
  CostScalar maxUec = csMinusOne;

  // This is an error condition, which would later assume uniform distribution
  if (baseColSet.entries() == 0)
	return csMinusOne;

  for (ValueId col = baseColSet.init();
			baseColSet.next(col);
			baseColSet.advance(col))
  {
	ColStatsSharedPtr colStat = this->getColStatsPtrForColumn (col);

	// for higher way joins, if the base column does not appear as
	// characteristic output, its histogram will not be cached. In that
	// case we shall use the total UEC when that histogram last appeared
	// in the list

	if (colStat == NULL)
	{
	  ColAnalysis * colAnalysis = col.colAnalysis();
	  if (colAnalysis &&
		 (colAnalysis->getFinalUec() > csZero) )
	  {
		uec = colAnalysis->getFinalUec();
		if (uec > maxUec)
		   maxUec = uec;
	  }
	  else
	  {
		// if there is no way we can get the UEC for partitioning
		// column, set it equal to row count, and don't bother to
		// look at other columns. This would mean that if the partition
	    // function contains a constant, we will have maxUec = rowcount
		maxUec = csMinusOne;
		break;
	  }
	}
	else
	{
      if (colStat->isOrigFakeHist())
      {
        maxUec = csMinusOne;
        break;
      }

	  uec = colStat->getTotalUec();
	  if (uec > maxUec)
	  {
	    maxUec = uec;
	  }
	}
  }
  return maxUec;
}

// -----------------------------------------------------------------------
// ColStatDescList::getMaxUecForCaseExpr
//
// Compute Max UEC based on the number of leaf expressions. Max
// UEC will be equal to max of ((number of leaf expressions)
// and (the max UEC of any columns in the leaf expressions)).
// -----------------------------------------------------------------------
CostScalar ColStatDescList::getMaxUecForCaseExpr(const ValueIdSet & leafValueSet) const
{
  CostScalar maxUec = getMaxUec(leafValueSet);

  // limit the maximum number of constants as a leaf in the expression to 5
  maxUec = MAXOF(maxUec.getValue(), leafValueSet.entries());
  return maxUec;
}

// -----------------------------------------------------------------------
// ColStatDescList::getAggregateUec
//
// Hopefully a useful method for various users of histograms, both
// internal and external.
// -----------------------------------------------------------------------

CostScalar ColStatDescList::getAggregateUec (const ValueIdSet & columns) const
{
  CostScalar retval = csMinusOne;

  // We first see if there's any multi-column information that exactly
  // matches the column-list in question.

  if ( getUecList() != NULL )
  {
    // The lookup method returns -1 if there is no multi-column uec
    // information for the ValueIdSet parameter; otherwise, it returns
    // those columns' multi-column uec.
    retval = getUecList()->lookup( columns );
    if ( retval.isGreaterThanZero() /*> csZero*/ )
    {
      // We know how many aggregate uec these columns had initially at
      // the scan nodes, before any predicates were applied.  Now we
      // need to take those predicates into account.
      CollIndex index;
      CostScalar origUec, currUec;

      for ( ValueId column = columns.init();
	    columns.next( column );
	    columns.advance( column ) )
      {
	origUec = getUecList()->lookup( column ); // get original single-column uec
	NABoolean found = getColStatDescIndexForColumn( index, column );

	// either orig or current single-column uec not found
	if ( found == FALSE || origUec.isLessThanZero() /* <= csZero*/ )
	  continue; // we continue looping

	currUec = (*this)[index]->getColStats()->getTotalUec();

	if ( currUec > origUec ) // sanity check: should not increase the UEC!
	  continue;

	retval *= currUec / origUec; // apply the reduction
      }

      return retval;
    }
  }

  // If we reach this point in the code, then we weren't able to use any
  // "true" multi-column information to answer the question.  So we simply
  // multiply the single-columns together to get a fudged aggregate
  // multi-column uec number.

  retval = csOne;

  CollIndex index;
  for ( ValueId column = columns.init();
	columns.next( column );
	columns.advance( column ) )
  {
    NABoolean found = getColStatDescIndexForColumn( index, column );

    // if any of the columns can't be found, abort
    if ( found == FALSE ) return csMinusOne;

    // multiply the totaluec of the histogram that matches each column
    retval *= (*this)[index]->getColStats()->getTotalUec();
  }

  return retval;

}

// ----------------------------------------------------------------------
// getColStatDescIndexForColWithMaxUec(leftColIndex, leftLeafValues)
// From the given ValueIdSet, the method returns the index of the histogram
// with max UEC
// ----------------------------------------------------------------------

NABoolean ColStatDescList::getColStatDescIndexForColWithMaxUec(
  CollIndex & indexWithMaxUec, /* out */
  const ValueIdSet & inputColumns /* in */
  ) const
{
  if (inputColumns.entries() == 1)
  {
	ValueId vid;
	inputColumns.getFirst(vid);

    return getColStatDescIndexForColumn(indexWithMaxUec, vid);
  }

  // This flag is used to indicate that histogram exists for at least 
  // one of the columns on which the predicate is being applied
  // It does not differentiate between the real and the default
  // histogram

  NABoolean statsExist = FALSE;

  // This flag is set to TRUE if even one of the histogram for the
  // column on which the predicate is being applied has real stats
  
  NABoolean atleastOneRealHist = FALSE;

  // This CostScalar will contain maximum UEC amongst all fake histograms
  // if atleastOneRealHist flag is FALSE, or amongst all real histograms
  // if the flag atleastOneRealHist flag is TRUE

  CostScalar maxUec = csOne;

  for (ValueId id = inputColumns.init();
  inputColumns.next(id);
  inputColumns.advance(id))
  {
    CollIndex index;
    if (!(getColStatDescIndexForColumn(index, id) ) )
      continue;

	statsExist = TRUE;
	ColStatsSharedPtr colStats = (*this)[index]->getColStats();

	// UECs of fake histograms are not real, 
	// We have already found at least one histogram for which actual stats
	// exist, hence skip the fake histograms for UEC comparison
	if (colStats->isOrigFakeHist() && atleastOneRealHist)
		continue;

	// Does real stats exist for this histogram? If this is the first
	// real histogram being encountered, then flush out whatever maxUec
	// has been computed so far. 
	if (!atleastOneRealHist && !colStats->isOrigFakeHist())
	{
	  maxUec = csOne;
	  atleastOneRealHist = TRUE;
	}

	// This point onwards, we are comparing
	// either all default UECs or all real UECs
    CostScalar uec = colStats->getTotalUec();
    if (uec >= maxUec)
    {
      maxUec = uec;
      indexWithMaxUec = index;
    }  
  }

  // if stats exist for even one column, return TRUE;
  return statsExist;
}

// ------------------------------------------------------------------
// addToAppliedPredsOfAllCSDs
// Update appliedPred attribute for all histograms whose column 
// information is passed in the ValueIdSet
// -------------------------------------------------------------------
void 
ColStatDescList::addToAppliedPredsOfAllCSDs(const ValueIdSet & inputColumns, 
	const ValueId & newPredicate)
{
  for (ValueId id = inputColumns.init();
  inputColumns.next(id);
  inputColumns.advance(id))
  {
    CollIndex index;
    if (!(getColStatDescIndexForColumn(index, id) ) )
      continue;

    ColStatDescSharedPtr colStatDesc = (*this)[index];
    colStatDesc->addToAppliedPreds(newPredicate);
    // also set the shape changed flag to TRUE
    colStatDesc->getColStats()->setShapeChanged(TRUE);
  }
}


// -----------------------------------------------------------------------
// ColStatDescList::useMultiUecIfCorrelatedPreds
//
// A CSDL::estimateCardinality() subroutine
//
// Use multi-column uec to find the resulting rowcount if we are
// applying multiple predicates to the same table on columns which
// are highly correlated (i.e., functional dependencies, ...).
//
// helper fn : ValueIdHashFn(), used to create an associated list
// for <ValueId, CostScalar>, which proves very useful.
// -----------------------------------------------------------------------

void
ColStatDescList::useMultiUecIfCorrelatedPreds (
     CostScalar & newRowcount,		  // in/out
     const CostScalar & oldRowcount,	  // in
     CollIndex predCount,		  // in : quick check : proceed if >=2
     const CollIndexList &joinHistograms, // in : histograms used in MC Join
     CollIndex startIndex,		  // in : 1st idx of CSDL to look at
     CollIndex stopIndex,		  // in : idx of CSDL+1 to look at
     NAHashDictionary<ValueId, CostScalar> & biLogicPredReductions)
{
  if ( getUecList() == NULL )
    return ;

  if ( startIndex >= stopIndex ) // misuse of function, oh well
    return ;

  if ( predCount < 2 ) // fewer than 2 histograms changed, nothing to do
    return ;

  NABoolean largeTableNeedsStats = FALSE;
  CostScalar adjRCBeforePreds = floor(oldRowcount.getValue());
  CollIndexList predList(STMTHEAP); // the CSDL-indices of the predicate-applied histograms
  CollIndex i;
  for ( i = startIndex; i < stopIndex; i++ )
  {
    ColStatsSharedPtr thisColStats = (*this)[i]->getColStats();
    // use ceil for the row count after preds on single column histograms
    // and floor of the original rowcount
    // to take care of costscalar rounding issues before doing the comparison
    // to see if MCs should be used to uplift the cardinalities.
    // use MC adjustment only if the adjusted rowcount from histogram after applying
    // predicates is less than the adjusted row count before applying predicates
    CostScalar adjHistRC = ceil(thisColStats->getRowcount().getValue());
    if ( NOT joinHistograms.contains( i )
	 AND (adjHistRC < adjRCBeforePreds))
    {
      // Skip any histograms created for constants.

      if ((NOT largeTableNeedsStats) AND
      (thisColStats->isUpStatsNeeded() ) AND
      !(thisColStats->isVirtualColForHist() ) )
      largeTableNeedsStats = TRUE;

      predList.insert( i ); // store the index, not the histogram
    }
  }

  // should have already checked for this, but just to be sure ...
  if ( (predList.entries() + biLogicPredReductions.entries()) < 2 )
    return ;

  NAHashDictionary<ValueId, CostScalar> predReductions // <ValueId, rowred> pairs
    (&(ValueIdHashFn),       11, TRUE, HISTHEAP) ;

  CostScalar highestReductionFromPreds = csOne;

  for ( i = 0; i < predList.entries(); i++ )
  {
    ColStatDescSharedPtr tmpCSD = (*this)[ predList[i] ] ;
    CostScalar scCard = tmpCSD->getColStats()->getRowcount();

    const ValueIdSet & preds = tmpCSD->getMergeState();

    // There could be cases when the null-instantiated column has participated
    // in the join. This could be a case of left joins or in Union. In those
    // cases, the mergeState is updated by the nulledExpression (don't know why?).
    // But this should not have an impact on adjusting cardinalities for Scans
    // So, in case of null_instantiated column, retrieve its child and use
    // that to compute multi-col uec adjustment. Sol: 10-060609-7077 and 10-060607-7010

    ValueIdSet baseColSet;
	preds.findAllReferencedBaseCols(baseColSet);

    const CostScalar rowRed = scCard / oldRowcount;

    if(rowRed < highestReductionFromPreds)
      highestReductionFromPreds = rowRed;

    for ( ValueId id = baseColSet.init(); baseColSet.next( id ); baseColSet.advance( id ) )
    {
      ValueId    * key   = new (HISTHEAP) ValueId( id );
      CostScalar * value = new (HISTHEAP) CostScalar( rowRed );
      ValueId * result = predReductions.insert( key, value );
      // all inserts should be successful; should not have the same
      // ValueId in multiple histograms
      //
      // --> NOT TRUE!  Outer joins can produce CSDL's which have 2
      // histograms for a given column; however, in this case, it's
      // generally (always?) the case that the 2nd instance of a
      // histogram is the null-instantiated one, so we should be able
      // to ignore it without any bad effects.
      //
    }
  }

  CostScalar highestReductionFromEqPreds = highestReductionFromPreds;

  // Append the columns and the respective reductions from the bilogic
  // predicates to the overall list of columns and reductions.
  NAHashDictionaryIterator<ValueId, CostScalar> biLogicPredIter( biLogicPredReductions );
  ValueId    * biLogicPredColumn = NULL;
  CostScalar * reduction  = NULL;
  CostScalar reductionFromBiLogicPreds = csOne;

  for ( biLogicPredIter.getNext( biLogicPredColumn, reduction );
	biLogicPredColumn != NULL && reduction != NULL;
	biLogicPredIter.getNext( biLogicPredColumn, reduction ) )
  {
    reductionFromBiLogicPreds *= *(reduction);

    // If the column already exists, multiply the bilogic reduction 
    // to the overall reduction of the column
    if(predReductions.contains(biLogicPredColumn))
    {
      *(reduction) *= *(predReductions.getFirstValue(biLogicPredColumn));
      predReductions.remove(biLogicPredColumn);
    }

    if(*(reduction) < highestReductionFromPreds)
      highestReductionFromPreds = *(reduction);

    ValueId    * key   = new (HISTHEAP) ValueId( *biLogicPredColumn );
    CostScalar * value = new (HISTHEAP) CostScalar( *reduction );
    ValueId * result = predReductions.insert( key, value );
  }

  // The following is the  row count of the histogram encapsulating the most 
  // reducing predicate. This serves as the upper bound for the uplifted rowcount.
  CostScalar minSingleColPredRC = csOne;

  // updatedOldRowcount stores row count before any of the predicates
  // being considered for correlation were applied.
  CostScalar updatedOldRowcount = oldRowcount;
  if(reductionFromBiLogicPreds != csZero)
  {
    updatedOldRowcount /= reductionFromBiLogicPreds;
    minSingleColPredRC = highestReductionFromPreds * updatedOldRowcount;
  }
  else
    minSingleColPredRC = highestReductionFromEqPreds * oldRowcount;  

  // Now we've got an association list of
  //   <columns we've applied predicates to, associated rowcount reduction>
  // pairs.
  //
  // The question now becomes : do we have sufficient multi-column uec
  // information to determine an adjustment to the rowcount (due to multiple
  // predicates on highly-correlated columns)?  If so, then the value
  // of 'reductionAdjustment' should be applied to newRowcount; otherwise,
  // we're done.

  // When the newRowcount is zero, we give up the promotion
  if(newRowcount.isExactlyZero())
    return;

  CostScalar reductionAdjustment;

  NABoolean sufficientInformation =
    uecList()->useMCUecForCorrPreds (predReductions,        /* in/mod  */
                                        predCount,             /* in : #param */
                                        updatedOldRowcount,           /* in */
                                        newRowcount,           /* in */
					largeTableNeedsStats,
                                        *this,                /* in. Pass list of histograms to be used for displayMissingStatsWarnings */
                                        reductionAdjustment);  /* out */

  // if we don't have sufficient MC-info, or there aren't any
  // "highly-correlated" columns, then never mind

  if(NOT sufficientInformation)
    return;

  CostScalar result = newRowcount * reductionAdjustment;

  // sanity check : if our calculated result actually *reduced* the
  // resulting rowcount (we are trying to increase it!), or if our
  // calculated result is more than the rowcount before we applied these
  // predicates (applying predicates should not increase rowcount!), then
  // we screwed up somewhere : ergo, ignore these results and return.
  if ( result < newRowcount)
    return;

  // High bound sanity check. The cardinality cannot be higher than, minimum
  // single column predicate.
  result = MINOF(result, minSingleColPredRC);
  newRowcount = result; // #retval
}

// -----------------------------------------------------------------------
// ColStatDescList::addRecentlyJoinedCols
// Traverse the histogram list and collect the recently joined columns
// -----------------------------------------------------------------------
void
ColStatDescList::addRecentlyJoinedCols(CollIndex startIndex,
                                       CollIndex stopIndex)
{
  for (CollIndex i = startIndex ; i < stopIndex ; i++ )
  {
    ColStatsSharedPtr thisColStats = (*this)[i]->getColStats();

    // skip any histograms created for constants
    if ( !(thisColStats->isVirtualColForHist() ) &&
          thisColStats->isRecentJoin() )
    {
      // first Join set the joined cols in joinStatDescList, which will be
      // used later to set the min cardinality for join
      // for left joins and Unions, the columns in the merged state could be
      // hidden by another expression. Hence extract the base column from it
      // Statistics is not affected by the extra expression.

      ValueIdSet mergedState = (*this)[i]->getMergeState();
      ValueIdSet baseColSet;
      mergedState.findAllReferencedBaseCols(baseColSet);

      this->addToJoinedCols(baseColSet);
    }
  }
}

// -----------------------------------------------------------------------
// ColStatDescList::useMultiUecIfMultipleJoins
//
// A CSDL::estimateCardinality() subroutine
//
// Use multi-column uec to find the resulting rowcount from a
// multi-column join between two tables, if possible.
//
// We need the "oldRowcount" parameter in order to determine the
// row reduction for each join Histogram.  The rowRedFactor_ data
// member is not set until we do synchronizeStats() later on.
//
// E.g., for "join T1 and T2 on T1.a=T2.b and T1.c=T2.d", if we have
// multi-column uec information on (T1.a,T1.c) and on (T2.b,T2.d), then we
// can improve our rowcount estimate for this join.
// -----------------------------------------------------------------------

void
ColStatDescList::useMultiUecIfMultipleJoins (
     CostScalar & newRowcount,       /* in/out */
     const CostScalar & oldRowcount, /* in */
     CollIndex startIndex,           /* in : first index of CSDL */
     CollIndex stopIndex,            /* in : last index of CSDL+1 */
     CollIndexList & joinHistograms, /* out */
     const Join * expr,
     MergeType mergeMethod)
{
  // compute reduction from single column histograms uptill this point
  CostScalar redFromSC = newRowcount / oldRowcount;
  // We start by saying that are no joins represented by this ColStatDescList
  // a very typical case of cross product. As we see any joining column, we
  // will set this to TRUE. If we see more than joining column, we will
  // set it back to FALSE

  joinOnSingleCol_ = FALSE;

  // this should probably never happen ... but if it does: when we don't
  // have any multi-column information whatsoever,
  // don't cause a memory exception!
  if ( getUecList() == NULL )
    return;

  if ( startIndex >= stopIndex ) // misuse of function, oh well
    return;

  // first, we need to determine if more than one join was performed -- and
  // reset all of the "isRecentJoin" flags

  CollIndex i, j;

  for ( i = startIndex ; i < stopIndex ; i++ )
  {
    ColStatsSharedPtr thisColStats = (*this)[i]->getColStats();

    // skip any histograms created for constants
     if ( !(thisColStats->isVirtualColForHist() ) &&
           thisColStats->isRecentJoin() )
    {
      joinOnSingleCol_ = TRUE;

      (*this)[i]->getColStatsToModify()->setRecentJoin( FALSE ); // unset flag
      joinHistograms.insert( i ); // store the index, not the histogram
      // first Join set the joined cols in joinStatDescList, which will be
      // used later to set the min cardinality for join
      // for left joins and Unions, the columns in the merged state could be
      // hidden by another expression. Hence extract the base column from it
      // Statistics is not affected by the extra expression.

      ValueIdSet mergedState = (*this)[i]->getMergeState();
      ValueIdSet baseColSet;
      mergedState.findAllReferencedBaseCols(baseColSet);

      this->addToJoinedCols(baseColSet);
    }
  }

  // if fewer than 2 joins were performed, never mind
  if ( joinHistograms.entries() < 2 )
    return;

  // Join is on more than one column, so set the flag to FALSE

  joinOnSingleCol_ = FALSE;

  LIST(ValueIdList) joinValueIdPairs(STMTHEAP); // the ValueId's pairwise (per join)

  // look inside the mergeStates of each join histogram, and grab the
  // ValueIds associated with each join
  for ( i = 0 ; i < joinHistograms.entries() ; i++ )
  {
    ValueIdList tmp =
      (*this)[ joinHistograms[i] ]->getMergeState(); // set->list

    ValueIdList * tmp2 = new STMTHEAP ValueIdList();

    CollIndex indx = 0;

    // There could be cases when the null-instantiated column has participated
    // in the join. This could be a case of left joins or in Union. In those
    // cases, the mergeState is updated by the nulledExpression (don't know why?).
    // But this should not have an impact on adjusting cardinalities for join
    // So, in case of null_instantiated column, retrieve its child and use
    // that to compute multi-col uec adjustment. Sol: 10-060609-7077 and 10-060607-7010

    for (CollIndex k = 0; k < tmp.entries(); k++)
    {
      if (tmp[k].getItemExpr()->getOperatorType() != ITM_BASECOLUMN)
      {
        ValueIdSet result;
        tmp[k].getItemExpr()->findAll(ITM_BASECOLUMN, result, TRUE, TRUE);
        tmp2->insertSet(result);
      }
      else
        tmp2->insert(tmp[k]);
    }
    
    // although we have not considered the MC-UEC for the columns which have been reduced
    // by non-equality predicates, we still need to take into their selectivity from
    // join on single columns but by assuming that these are not correlated to other
    // columns
    joinValueIdPairs.insert( *tmp2 );
  }

  // now see how many of these match our multicolumn-uec information

  // the ValueId's for the columns of the two tables involved in the
  // multi-column join
  CostScalar prodInitUec = csMinusOne, // #docvars
	     multiColUec = csMinusOne, // #retvals
             leftMCUec   = csOne;
  LIST(ValueIdList) joinValueIdPairsRemaining = joinValueIdPairs; // #retval
  const NABoolean largeTableNeedsStats = // should stats exist for this table?
    (*this)[0]->getColStats()->isUpStatsNeeded();

  NABoolean checkForLowBound = FALSE;

  // baseRCForMaxMCUEC contains the baseRowCount of the table which
  // has maximum multi-column UEC for the joining columns
  CostScalar baseRCForMCUEC = csOne;
  NABoolean joinOnUnique = FALSE;

  // If the join is between unique/non-unique sides, then the output parameter 
  // 'multiColUec' will return row count of non-unique side.
  NABoolean sufficientInformation =
    uecList()->getUecForMCJoin (joinValueIdPairsRemaining, /* in/out */
                                largeTableNeedsStats,      /* in     */
                                expr,
                                prodInitUec,            /* out    */
                                multiColUec,            /* out */
                                baseRCForMCUEC,
                                leftMCUec,
                                checkForLowBound,          /* out */
                                joinOnUnique,
                                *this,
                                redFromSC);         

  // minimum cardinality from join should be the minimum cardinality of group
  // for empty input logical properties.
  if (checkForLowBound)
    setCapForLowBound();

  if ( NOT sufficientInformation )
      return;

  // error! value not set, or simply avoiding div-by-zero! ignore MC-info in this case
  if ( multiColUec.isLessThanZero() )
    return;

  // Since we've reached this point, we know that the required
  // multi-column uec information exists to improve on the single-column
  // selectivity rowcount estimate!

  // -------------------------------------------------------------------------
  // What to do if joinValueIdPairsRemaining still has entries in it?
  //
  // If there are any joins that haven't been accounted for, that should
  // be alright (I think?), since their reductions have already been
  // applied to produce the single-column selectivity rowcount estimate.
  //
  // However, we don't want those entries in joinValueIdPairsRemaining to
  // go through the loop below.  So, remove these joins from
  // joinHistograms.
  // -------------------------------------------------------------------------
  for ( i = 0; i < joinValueIdPairsRemaining.entries(); i++ )
  {
    const ValueIdSet ithPair =
      joinValueIdPairsRemaining[i]; // convenience: list->set

    for ( j = 0; j < joinHistograms.entries(); j++ )
    {
      const ValueIdSet & mergeStateSet =
	(*this)[ joinHistograms[j] ]->getMergeState();

      // for left joins and Unions, the columns in the merged state could be
      // hidden by another expression. Hence extract the base column from it
      // Statistics is not affected by the extra expression.

      ValueIdSet mergedState = (*this)[i]->getMergeState();
      ValueIdSet baseColSet;
      mergedState.findAllReferencedBaseCols(baseColSet);

      // the code in getUecForMCJoin sometimes removes a table
      // reference from an entry in joinValueIdPairsRemaining --> so
      // we can't use simple equality for the comparison below
      if ( baseColSet.contains( ithPair ) )
      {
	joinHistograms.removeAt( j );
	break ;
      }
    }
  }

  // the following lines contain variables defined/used in the MC-document
  const CostScalar SC_cardinality = newRowcount;    // #docvar ("single-col value")
  CostScalar       MC_cardinality = csOne;	      // #docvar ("multi-col value")

    CostScalar       minSingleColJoinRC = COSTSCALAR_MAX;

    // first, apply the sumOfMaxUec values
   for ( i = 0; i < joinHistograms.entries(); i++ )
    {
      CostScalar singleColJoinRC;
      singleColJoinRC = (*this)[ joinHistograms[i] ]->getColStats()->getRowcount();

      if (singleColJoinRC < minSingleColJoinRC)
        minSingleColJoinRC = singleColJoinRC;
    }

   NABoolean aSemiJoin = (expr && (expr->isSemiJoin() || expr->isAntiSemiJoin())) ? TRUE : FALSE;
   if (!expr && mergeMethod == SEMI_JOIN_MERGE)
     aSemiJoin = TRUE;

   if ( joinOnUnique && !aSemiJoin)
   {
     CostScalar leftRC = ((Join *)expr)->child(0).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->getResultCardinality();
     CostScalar rightRC = ((Join *)expr)->child(1).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->getResultCardinality();

     CostScalar baseRowcount = leftRC * rightRC;

     // If joining columns are unique, rowcount from equality 
     // predicates is equal to that of non unique side
     //
     // multiColUec is a misnomer here as  it will be storing row count 
     // of non-unique side instead of MC UEC.

     MC_cardinality = multiColUec * oldRowcount / baseRowcount;
   }
   else
   {
    // for non-unique joining column set, compute reduction as follows
    if (CmpCommon::getDefault(COMP_BOOL_145) == DF_OFF)
      MC_cardinality = SC_cardinality * prodInitUec/multiColUec;
    else
    {
      CostScalar sel = MAXOF((SC_cardinality / oldRowcount), COSTSCALAR_EPSILON);

      if (baseRCForMCUEC < multiColUec)
	    multiColUec = baseRCForMCUEC;

      if (!aSemiJoin)
      {
        CostScalar selAdj = sel * prodInitUec / multiColUec;
        CostScalar adj = (csOne - sel * prodInitUec) / baseRCForMCUEC;
        selAdj = selAdj + adj;
        MC_cardinality = selAdj * oldRowcount;
      }
      else
      {
        MC_cardinality = oldRowcount * multiColUec / leftMCUec;
      }
    }
   }

    // Low bound sanity check for MC UEC. The cardinality should not go below
    // MINOF (join from single column histograms, 1/max multi col UEC)
    // The second factor (1/max MCUEC), takes into account the anti-correlation
    // where the join from single column histograms might be over estimated
    // For semi and anti semi join, we set the low bound as MIN of SC_cardinality
    // as oldRowcount is equal to leftrowcount for semi_joins which if outer is 
    // unique results as lowBoundFromMCUec = 1
    CostScalar lowBoundFromMCUec = SC_cardinality;
    if (!aSemiJoin)
      lowBoundFromMCUec = MINOF(lowBoundFromMCUec, oldRowcount/multiColUec);
    newRowcount = MAXOF(lowBoundFromMCUec, MC_cardinality);

    // High bound sanity check. The cardinality cannot be higher than, minimum
    // single column join.
    newRowcount = MINOF(newRowcount, minSingleColJoinRC);
}

// ----------------------------------------------------------------------
// this method will set the inputCard to all the colStats in this list.
// This input cardinality is what comes from outer. This is used to
// compute UECs in CalculateCorrectResultUec
// -----------------------------------------------------------------------

void ColStatDescList::setInputCard(CostScalar rows)
{
  for ( CollIndex i = 0; i < entries(); i++ )
    (*this)[i]->setInputCard(rows);
}
// ----------------------------------------------------------------------
//  ColStatDescList::synchronizeStats
//  A CSDL utility routine used to call ColStatDesc::synchronizeStats()
// ----------------------------------------------------------------------
void
ColStatDescList::synchronizeStats (const CostScalar & baseRowcount,
                                   const CostScalar & newRowcount,
                                   CollIndex loopLimit)
{
  for ( CollIndex i = 0; i < loopLimit; i++ )
  {
    const CostScalar & oldCount = (*this)[i]->getColStats()->getRowcount();

    if ( oldCount != newRowcount )
      (*this)[i]->synchronizeStats( baseRowcount, newRowcount );
  }
}

void
ColStatDescList::synchronizeStats (const CostScalar & newRowcount,
                                   CollIndex loopLimit)
{
  for ( CollIndex i = 0; i < loopLimit; i++ )
  {
    const CostScalar & oldCount = (*this)[i]->getColStats()->getRowcount();
    (*this)[i]->synchronizeStats( oldCount, newRowcount );
  }

}  // ColStatDescList::synchronizeStats

// ---------------------------------------------------------------------
//  ColStatDescList::mergeListPairwise
//
//  A routine used solely (today) for performing the implicit inner-equi-
//  join between columns appearing as outer references in both children of
//  the current join.
//
//  It presumes a certain structure to the given THIS ColStatDescList, and
//  also presumes what type of join is to be done.  (e.g., we assume that
//  there is an even number of ColStatDescSharedPtr's in the list.)

// ---------------------------------------------------------------------
CostScalar
ColStatDescList::mergeListPairwise ()
{
  CollIndex i = 0;
  CostScalar newRowcount = csZero;
  CostScalar newUec	 = csZero;

  ColStatDescSharedPtr rootStatDesc = (*this)[i];
  ColStatsSharedPtr rootColStats = rootStatDesc->getColStatsToModify();

  if ( rootColStats->getRowcount().isZero() )
  {
    // can't do much here....
    while ( i < this->entries() )
    {
      removeAt( i+1 );
      i++;
    }

    return csZero;
  }

  // sanity check
  if ((entries() % 2) != 0)
  {
// LCOV_EXCL_START - rfi
    CCMPASSERT( entries() % 2 == 0 ); // should be an even number!
    //if not return without merging. Don't want to land up with an 
    // unreferenced object in  the collections class
    return newRowcount;
// LCOV_EXCL_STOP
  }

  // NB: we avoid having the resulting rowcount blow up by making sure
  // we don't divide by something that's less than 1

  const CostScalar saveRowcount = rootColStats->getRowcount();
  CostScalar totalReduct  = csOne;
  CostScalar maxReduct    = csOne;


  while ( i < this->entries() )
  {
    rootStatDesc = (*this)[i];
    rootColStats = rootStatDesc->getColStatsToModify();
    if( NOT rootColStats->isShapeChanged() &&
	NOT (*this)[i+1]->getColStats()->isShapeChanged() )
    {
      rootStatDesc->mergeColStatDesc(
	(*this)[i+1],
	AND_MERGE,
	TRUE // force merge
	);
      // get the aggregate results following the latest merge
      newRowcount = rootColStats->getRowcount();
      newUec      = rootColStats->getTotalUec();

      CostScalar reduct = ( saveRowcount < csOne ?
			  csOne : newRowcount / saveRowcount );
      totalReduct *= reduct;

      // remove the i+1'th entry.
      removeAt( i+1 );
    }
    else
      if ( rootColStats->isShapeChanged() )
	removeAt( i+1 );
      else
	removeAt( i );

    i++; // skip over just merged ColStats to next pair
  }

  newRowcount = saveRowcount * totalReduct;

  // Ensure that all histograms report the same new rowcount.
  synchronizeStats( newRowcount, entries() );

  return newRowcount;
}  // ColStatDescList::mergeListPairwise



// ----------------------------------------------------------------------
// ColStatDescList::divideHistogramAtPartitionBoundaries
// An external routine used to take a CDSL and identify, via the rows for
// one of the range-partitioning table columns, which partitions are active
// (i.e., > 0 rows returned) in a query.
// ----------------------------------------------------------------------

NABoolean ColStatDescList::divideHistogramAtPartitionBoundaries
   (const ValueIdList            & listOfPartKeys,      /*in*/
    const ValueIdList            & listOfPartKeyOrders, /*in*/
    const LIST(EncodedValueList *) & listOfPartBounds,    /*in*/
    ValueId       & keyCorrespondingToOutputRows,       /*out*/
    NABoolean     & isKeyAscending,                     /*out*/
    ColStats      & outputRows,                         /*out*/
    CollIndexList & outputFactors) const                /*out*/
{
  // $$$ first effort, we assume that the first partititioning key is the
  // $$$ one we want
  //
  // $$$ this next stmt should be replaced by code that looks
  // $$$ and makes sure the first column doesn't have only one uec!!!
  CollIndex partKeyIndex = 0; // $$$ change this later !!!


  // let the caller know the ValueId of the partitiong key we'll be using
  keyCorrespondingToOutputRows = listOfPartKeys[partKeyIndex];
  const ValueId & keyOrder = listOfPartKeyOrders[partKeyIndex];

  const ItemExpr * ie = keyOrder.getItemExpr();
  OperatorTypeEnum ote = ie->getOperatorType();

  if ( keyOrder.getItemExpr()->getOperatorType() == ITM_INVERSE )
    isKeyAscending = FALSE; // #retval
  else
    isKeyAscending = TRUE;

  // now build the Histogram corresponding to this part. key; we will use
  // this to create the outputRows array that we return from this function
  HistogramSharedPtr targetHist(new Histogram(HISTHEAP));
  CollIndex i, numBounds = listOfPartBounds.entries();

  if ( isKeyAscending ) // insert the boundary values in their current order
  {
    for ( i = 0; i < numBounds; i++ )
      targetHist->insertAt(
	i,
	HistInt( listOfPartBounds[i]->at(partKeyIndex) )
	);
  }
  else // insert the intervals in the reverse of their current order
  {
    for ( i = 0; i < numBounds; i++ )
      targetHist->insertAt(
	i,
	HistInt(listOfPartBounds[numBounds-i-1]->at(partKeyIndex))
	);
  }

  // make sure the "MAX-valued" HistInt doesn't contain "NULL" -- recall
  // (see EncodedValue.[cpp h], NULL is encoded as MAX_DBL, which is what
  // the last HistInt in targetHist will have as a value if the partitioning
  // key is over a key of type double.
  if ( (*targetHist)[numBounds-1].getBoundary().isNullValue() )
  {
    (*targetHist)[numBounds-1].setBoundary(
      EncodedValue( _ENCODEDVALUE_CLOSE_TO_NULL_ )
      );
  }

  // Note that since we don't have any boundary-inclusiveness information,
  // we'll just pretend that none exists; arbitrarily, we decide to make
  // the flag be FALSE.
  // The effect of this :
  //
  //     <  <  <  <
  //     |  |  |  |
  //     |  |  |  |
  //     1  2  3  4
  //
  // This histogram represents a table with 3 partitions.  The SQL
  // partitions are specified at values 2 and 3.  By having the flags be
  // NOT-BOUNDARY-INCLUSIVE (FALSE), we are specifying that the first
  // partition has values from 1 (including 1) to 2 (not including 2); the
  // second partition has values from 2 (including 2) to 3 (not including
  // 3); similarly, the the third partition goes from 3 (including 3) to 4
  // (not including 4).
  //
  // If this interpretation of the SQL semantics is not correct,
  // please let me know!

  // now get the Histogram for the CSD matching the part. key we're using
  ColStatsSharedPtr sourceColStats =
    getColStatsPtrForColumn( keyCorrespondingToOutputRows );

  if ( sourceColStats == NULL ||
       sourceColStats->getHistogram()->entries() == 0 )
    return FALSE; // couldn't find it! quit!

  // at this point, we've found the colstats corresponding to the
  // partitioning key that we'll be using; and we have the corresponding
  // list of boundary values for that key; now we can generate the desired
  // information

  // ----------------------------------------------------------------------
  // Before we proceed, however, we need to make sure that there aren't
  // any contiguous partition boundaries' values which are equal.
  // Histogram semantics does not allow for this, so we have to fudge this
  // (possible, legitimate partition boundary value) case with a second
  // variable, which keeps track of the number of partitions per histogram
  // interval.  Before we code this, here's an example which should help
  // explain what we're trying to accomplish:
  //
  // Consider the following partition boundary intervals
  //
  //   |  |  |  |  |  |  |  |  |  |  |
  //   M  3  3  3  3  5  5  7  7  7  M
  //   I                             A
  //   N                             X
  //
  // We want to compress this to :
  //
  //   0  1  4  2  3
  //   -  -  -  -  -
  //   |  |  |  |  |
  //   M  3  5  7  M
  //   I           A
  //   N           X
  //
  // (where the number across the top indicate the # of partitions that
  // have values for that interval)
  //
  // Interpreting the condensed, internal histogram:
  //
  //   interval 1: [MIN,3) : 1 partition
  //   interval 2: [3,5)   : 4 partitions
  //   interval 3: [5,7)   : 2 partitions
  //   interval 4: [7,MAX) : 3 partitions
  //
  // To create the mapping (the numbers indicating how many partitions
  // have a particular boundary value), we simply count all of the
  // HistInts which have a particular boundary value, then add a "0" to
  // the beginning of our list of integers.  Note that we don't count for
  // the last partition boundary value (MAX).
  //
  // For the example above, our list-of-ints looks like :
  //
  // step1:   1 4 2 3
  // step2:   0 1 4 2 3
  //
  outputFactors.clear();
  CollIndex countDuplicates = 0;

  // we're counting the # of intervals that have the same boundary value
  for ( i = 0; i < targetHist->entries()-1; /* no automatic increment */ )
  {
    if ( (*targetHist)[i].getBoundary() == (*targetHist)[i+1].getBoundary() )
    {
      countDuplicates++;
      targetHist->removeAt( i+1 ); // in-list removal : no increment of 'i'
    }
    else
    {
      outputFactors.insertAt( i, 1 + countDuplicates );
      countDuplicates = 0; // reset
      i++; // go to next interval
    }
  }
  outputFactors.insertAt( 0, 0 ); // first HistInt always contains garbage info

  // Are we done?  Have we inserted the last number?  This is needed to
  // indicate the number of partitions which have the 2nd-to-last
  // partition boundary value (we don't count for MAX)
  if ( outputFactors.entries() < targetHist->entries() )
    outputFactors.insertAt( targetHist->entries()-1, 1 ); // last boundary should be "MAX"

  if (outputFactors.entries() != targetHist->entries() )
  {
    CCMPASSERT( outputFactors.entries() == targetHist->entries() ); // sanity check
    // histogram-to-partition-boundary-list mapping failed. Partition is unusable
    return FALSE;
  }

  if (outputFactors.entries() <= 0 )
  {
    CCMPASSERT( outputFactors.entries() > 0 );
    // histogram-to-partition-boundary-list mapping failed. Partition is unusable
    return FALSE;
  }

  if ( targetHist->entries() == 1 )
  {
    // unusual, but possible (is it?!) case : all partition boundary values are equal, ugh
    // --> if this happens, then make it a 2-HistInt histogram so we can at least
    // call the routines below
    const EncodedValue & bound = (*targetHist)[0].getBoundary();
    targetHist->clear();
    targetHist->insertZeroInterval( bound, bound, TRUE );

    outputFactors.insert( countDuplicates + 1 );
  }
  // ----------------------------------------------------------------------

  // ----------------------------------------------------------------------
  // less work for this special case: zero rows in sourceColStats
  if ( sourceColStats->getRowcount().isZero() )
  {
    outputRows.setHistogram( new HISTHEAP Histogram( *targetHist, HISTHEAP ) );
    outputRows.setMaxMinValuesFromHistogram();
    outputRows.setRowsAndUec( csZero, csZero );
    return TRUE ;
  }
  // ----------------------------------------------------------------------

  // this histogram is the source of the rows that we'll be placing in targetHist
  HistogramSharedPtr sourceHist = sourceColStats->getHistogram();

  // create the template histogram, then populate it
  HistogramSharedPtr templateHist =
    sourceHist->createMergeTemplate( targetHist, FALSE );
  ColStats templateStats( templateHist, HISTHEAP );

  CollIndex templateEntries = templateHist->entries();
  templateStats.populateTemplate( sourceColStats );
  if ( templateHist->entries() != templateEntries )
    return FALSE; // something went very wrong!

  // now, "squeeze" the resulting histogram so that it only keeps the
  // interval boundaries from the targetHistogram
  NABoolean result =
    templateHist->condenseToPartitionBoundaries( targetHist );

  if ( result != TRUE )
    return result; // something went wrong!

  // now, return the resulting Histogram in a format that we can use
  outputRows.setHistogram( templateHist );
  outputRows.setMaxMinValuesFromHistogram();
  outputRows.setRowsAndUecFromHistogram();

  return TRUE;
  // that's all folks!
}

// compress all histograms in the list to a single interval histogram
void
ColStatDescList::compressColStatsToSingleInt()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    ColStatsSharedPtr colStat = (*this)[i]->getColStatsToModify();
    if (!colStat->isVirtualColForHist() && !colStat->isOrigFakeHist())
      colStat->compressToSingleInt();
  }
}

// ----------------------------------------------------------------------
//  ColStatDescList::mergeSpecifiedStatDescs
//  A utility routine used to merge specific ColStatDesc's together.
// ----------------------------------------------------------------------
void
ColStatDescList::mergeSpecifiedStatDescs (const CollIndexList & statsToMerge,
                                          CollIndex rootIndex,
                                          MergeType mergeMethod,
                                          CollIndex numOuterColStats,
                                          CostScalar & newRowcount,
                                          CostScalar & newUec,
                                          NABoolean forVEGPred,
					  OperatorTypeEnum opType)
{
  ColStatDescSharedPtr rootDesc = (*this)[rootIndex];

  CostScalar saveRowcount = newRowcount;
  CostScalar saveUec      = newUec;

  for ( CollIndex i = 0; i < statsToMerge.entries(); i++ )
  {
    if ( statsToMerge[i] != rootIndex )
    {
      // If the statistics to be merged are from opposite sides of the
      // numOuterColStats boundary, then we are doing a
      //      left_table_column = right_table_column
      // merge that should be done as the caller requested.
      // Otherwise, we're merging columns from the same table, and 
      // selectivity for that is HIST_NO_STATS_UEC.
      MergeType localMergeMethod = mergeMethod;
      NABoolean joinOnOneTable = FALSE;
      ColStatDescSharedPtr tmpDesc(new (HISTHEAP)
	     ColStatDesc( *((*this)[statsToMerge[i]]) ), HISTHEAP);

      if ( NOT ( rootIndex < numOuterColStats &&
		 statsToMerge[i] >= numOuterColStats ) )
      {
        // even though the mergeMethod may never be used, but we are
        // initializing this for cases when COMP_BOOL_74 is OFF
	localMergeMethod = INNER_JOIN_MERGE;
	joinOnOneTable = rootDesc->mergeColStatDescOfSameTable(tmpDesc, opType);
      }

      if (!joinOnOneTable)
      {
	rootDesc->mergeColStatDesc(tmpDesc,
                                   localMergeMethod,
                                   FALSE, // don't force merge
                                   opType
                                   );
      }

      // get the aggregate results following the latest merge
      ColStatsSharedPtr colStats = rootDesc->getColStats();
      newRowcount = colStats->getRowcount();
      newUec      = colStats->getTotalUec();

      // update the 'saved' information
      saveRowcount = newRowcount;
      saveUec      = newUec;

      // if this isn't a VEG, then it's an equality predicate.
      // update the info used to support transitive closure for
      // non-VEG equality predicates.
      if ( NOT forVEGPred )
      {
	tmpDesc->setFromInnerTable(
	  statsToMerge[i] >= numOuterColStats ? TRUE : FALSE
	  );

	rootDesc->nonVegEquals().insert( tmpDesc );
      }
    }
  }
}  // ColStatDescList::mergeSpecifiedStatDescs

// -----------------------------------------------------------------------
// Input: inputColumn, the column for which we need the ColStats
// output: TRUE if there was a ColStatDesc for "inputColumn" in this
//         ColStatDescList, in which case its index is returned
//         in "index". FALSE otherwise.
// -----------------------------------------------------------------------
//
// Logic:
//
// A ColStatDescList contains a list of ColStatDesc in no particular
// order.  Every ColStatDesc represents a facade to a ColStats, i.e. to a
// histogram.  Sometimes, the histogram is fake but the ColStatDesc
// encapsulates this fact.  Every ColStatDesc has important fields that
// describe to which column it applies to:
//
// mergeState_: a ValueIdSet, describing which histograms have been merged
// with it.  Each element in the set contains the base column of the
// histogram that has been merged with the current one.
//
// -----------------------------------------------------------------------
NABoolean
ColStatDescList::getColStatDescIndexForColumn (
  CollIndex& index, /* out */
  const ValueId& inputColumn /* in */
  ) const
{

  // ----------------------------------------------------------------
  // Find the base column for the input column:
  // ----------------------------------------------------------------

  const ItemExpr *inputColumnIEPtr = inputColumn.getItemExpr();

  ValueId baseColumnForInputColumn;
  ValueId vegColumnForInputColumn;
  switch ( inputColumnIEPtr->getOperatorType() )
  {
  case ITM_VEG_REFERENCE:
    {
      // -------------------------------------------------
      //  The inputColumn is a VEG reference
      //  Loop through the columns until you find it:
      // -------------------------------------------------
      const VEG * exprVEG = ((VEGReference *)inputColumnIEPtr)->getVEG();
      const ValueIdSet & VEGGroup  = exprVEG->getAllValues();

      if(exprVEG->seenBefore())
	return FALSE;
      else
      {
	exprVEG->markAsSeenBefore();
	for ( ValueId id = VEGGroup.init();
	      VEGGroup.next( id );
	      VEGGroup.advance( id ) )
	{
	  if ( getColStatDescIndexForColumn( index, id ) )
	  {
	    exprVEG->markAsNotSeenBefore();
	    return TRUE;
	  }
	}
      // If we are here, then we did not find the column
      exprVEG->markAsNotSeenBefore();
      return FALSE;
      }
    }
    break;

  case ITM_INSTANTIATE_NULL:
  case ITM_BASECOLUMN:
    // -------------------------------------------------
    //  The inputColumn is a base column.
    // -------------------------------------------------
    baseColumnForInputColumn = inputColumn;
    break;

  case ITM_INDEXCOLUMN:
    // -------------------------------------------------
    //  Get the base column for the index column:
    // -------------------------------------------------
    {
      const BaseColumn *bcIEPtrForIndexColumn =
	(BaseColumn *) ((IndexColumn *) inputColumnIEPtr)->
	getDefinition().getItemExpr();

      baseColumnForInputColumn = bcIEPtrForIndexColumn->getValueId();
      break;
    }

  default:
     // give a last shot to see if a histogram exists for the expression
    // that we might be looking. Example we could have an aggregate
    // in a VEG, and we may also have a histogram for that. Sol:10-090110-9758
    vegColumnForInputColumn = inputColumn;
  } // switch on type of input column


  // -----------------------------------------------------------------------
  // Now, traverse this list and check whether there is a ColStatDesc
  // for the given inputColumn, if so save its index in "index"
  // -----------------------------------------------------------------------
  NABoolean foundInBT = FALSE; // assume no ColStatDesc for this column
  for ( CollIndex i = 0; i < entries(); i++ )
  {
    // Obtain the info for the current ColStatDesc:
    ColStatDescSharedPtr currentColStatDesc = (*this)[i];
    // The "merge state" of a ColStatDesc indicates all the
    // columns that have been merged into this ColStatDesc.
    // Initially, the merge state consists of the original
    // base table column, therefore, this will work even for
    // ColStatDesc's that have not been merged
    const ValueIdSet & msSet = currentColStatDesc->getMergeState();
    if ( ( msSet.contains( baseColumnForInputColumn ) ) ||
         ( currentColStatDesc->VEGColumn() == vegColumnForInputColumn) ||
         ( currentColStatDesc->getColumn() == baseColumnForInputColumn ) )
    {  // found!
      index = i;
      foundInBT = TRUE;
      break;
    }
  }

  return foundInBT;
} // getColStatDescIndexForColumn(CollIndex & index, const ValueId column) const

// -----------------------------------------------------------------------
// Input: inputColumn, the column for which we need the ColStats
//        partKeyColArray, columns for which we need the ColStats
// output: TRUE if there was a ColStatDesc for "partKeyColArray or inputColumn" 
//         in this ColStatDescList, in which case its index is returned
//         in "index". FALSE otherwise.
// -----------------------------------------------------------------------
NABoolean
ColStatDescList::getColStatDescIndexForColumn (
  CollIndex& index, /* out */
  const ValueId& inputColumn, /* in */
  NAColumnArray& partKeyColArray /* in */
  ) const
{
  
  // single column partitioned table
  if (partKeyColArray.entries() <= 1)
     return (getColStatDescIndexForColumn(index, inputColumn));

  // if multi-column partition key, find an MC with columns that are prefix to the 
  // partition column list
  for ( CollIndex i = 0; i < entries(); i++ )
  {
     if (((*this)[i]->getColStats()->isMCforHbasePartitioning()) &&
         (partKeyColArray.entries() >= (*this)[i]->getColStats()->getStatColumns().entries()))
     {
         index = i;
         return TRUE;
     }
  }

  return FALSE;
  
}


// Get the index of the ColStatDesc for a particular valueId.
// The match is based on the VEGColumn field.
NABoolean
ColStatDescList::getColStatDescIndex (CollIndex& index,           /* out */
                                      const ValueId& value) const /* in */
{
  for ( CollIndex i = 0; i < entries(); i++ )
  {
    // Obtain the info for the current ColStatDesc:
    ColStatDescSharedPtr currentColStatDesc = (*this)[i];
    const ValueId & veg = currentColStatDesc->getVEGColumn();

    if ( veg == value )
    {  // found!
      index = i;
      return TRUE;
    }
  }
  return FALSE;
}

// -----------------------------------------------------------------------
// This routine returns the ColStats (i.e. the histogram) that corresponds
// to the given inputColumn.
//
// Input:
// ======
//  const ValueId& inputColumn : a ValueId denoting the wanted histogram
//
// Output:
// =======
//  A NON-NULL ColStatsSharedPtr for the histogram that describes the given
//  inputColumn (if such histogram exists)
//
//  NULL if a histogram for the given inputColumn does not exist in the
//  ColStatDescList
// -----------------------------------------------------------------------

#pragma nowarn(262)   // warning elimination
ColStatsSharedPtr
ColStatDescList::getColStatsPtrForColumn (const ValueId& inputColumn) const
{

  ColStatsSharedPtr colStatsPtr;
  CollIndex index = 0;
  NABoolean found = getColStatDescIndexForColumn( index, inputColumn );
  if ( NOT found )
  {
    return NULL;
  }
  else
  {
#pragma nowarn(270)   // warning elimination
    if ((index < 0) || (index >= entries()) )
    {
// LCOV_EXCL_START - rfi
      // if the index is out side the range of histogram list, return
      // NULL pointer indicating that the histogram is not found in the 
      // list
      CCMPASSERT( (index >= 0) AND (index < entries() ));
      return NULL;
// LCOV_EXCL_STOP
    }

#pragma warn(270)  // warning elimination
    return (*this)[index]->getColStats();
  }
}
#pragma warn(262)  // warning elimination

// -----------------------------------------------------------------------
// This method returns the ColStatsSharedPtr for the ColStats that references
// the given predicate if it exists, otherwise it returns NULL
// -----------------------------------------------------------------------
ColStatsSharedPtr
ColStatDescList::getColStatsPtrForPredicate (const ValueId& predicate) const
{
  ColStatsSharedPtr colStatsPtr;

  const ItemExpr *predIE = predicate.getItemExpr();
  if (!predIE->isAPredicate())
    return NULL;

  const Int32 arity = predIE->getArity(); // for debugging
  switch ( arity )
  {
    case 3:
    case 2:
    {
      // a join predicate of the form col1 op col2 or similar
      ItemExpr *leftExpr  = predIE->child(0);
      ItemExpr *rightExpr = predIE->child(1);

      const ValueId &leftChildVid  = leftExpr->getValueId();
      const ValueId &rightChildVid = rightExpr->getValueId();

      // ------------------------------------------------------------------
      // Process left child:
      // ------------------------------------------------------------------
      if ( leftExpr->isAPredicate() )
      {
	      colStatsPtr = getColStatsPtrForPredicate( leftChildVid );
      }
      else if ( leftExpr->getOperatorType() == ITM_VEG_REFERENCE )
      {
	      ValueIdSet vidSet;
	      vidSet.insert(
	        ((VEGPredicate *)leftExpr)->getVEG()->getAllValues() );
	      colStatsPtr = getColStatsPtrForVEGGroup( vidSet );
      }
      else
      {
	      // It is not a predicate NOR a reference, then it
	      // must be an expression, get the columns it refers
	      // to
	      leftExpr = leftExpr->getLeafValueIfUseStats();
              const ValueId &leftVid  = leftExpr->getValueId();
	      colStatsPtr = getColStatsPtrForColumn( leftVid );
      }


      if ( colStatsPtr == NULL )
      {
	      // ---------------------------------------------------------------
	      // ColStats not found in left child, try to find it in right:
	      // ---------------------------------------------------------------
	      if ( rightExpr->isAPredicate() )
	      {
	        colStatsPtr = getColStatsPtrForPredicate( rightChildVid );
	      }
	      else if ( rightExpr->getOperatorType() == ITM_VEG_REFERENCE )
	      {
	        ValueIdSet vidSet;
	        vidSet.insert(
	          ((VEGPredicate *)rightExpr)->getVEG()->getAllValues() );
	        colStatsPtr = getColStatsPtrForVEGGroup( vidSet );
	      }
	      else
	      {
	        // It is not a predicate NOR a reference, then it
	        // must be an expression, get the columns it refers
	        // to
	        rightExpr = rightExpr->getLeafValueIfUseStats();
	        const ValueId &rightVid  = rightExpr->getValueId();
	        colStatsPtr = getColStatsPtrForColumn( rightVid );
	      }
      } // if colstats not found in left child
    } // if arity is 2
    break;

  case 0:
    {
      if ( predIE->getOperatorType() == ITM_VEG_PREDICATE )
      {
	      const ValueIdSet & vegGroup =
	        ((VEGPredicate *)predIE)->getVEG()->getAllValues();
	      colStatsPtr = getColStatsPtrForVEGGroup( vegGroup );
      }
      else
      {
        CCMPASSERT( predIE->getOperatorType() == ITM_VEG_PREDICATE );
        // add code to handle case here...
        return NULL;
      }

    }
    break;

  default:
    {
      return NULL; // For unary logic predicates, there is nothing to return
    }

  } // case getArity()


  return colStatsPtr;
}

ColStatsSharedPtr
ColStatDescList::getColStatsPtrForVEGGroup(const ValueIdSet& VEGGroup) const
{
  ColStatsSharedPtr colStatsPtr = NULL;
  // Get the first ColStats for any value that is a column:

  ValueIdSet leafValuesForExpr;

  for ( ValueId vid = VEGGroup.init();
	VEGGroup.next( vid );
	VEGGroup.advance( vid ) )
  {
    ItemExpr * vidIePtr = vid.getItemExpr();
    switch ( vidIePtr->getOperatorType() )
    {
    case ITM_BASECOLUMN:
    case ITM_INDEXCOLUMN:
      {
	colStatsPtr = getColStatsPtrForColumn( vid );
	if ( colStatsPtr != NULL ) return colStatsPtr;
	break;
      }

    case ITM_INSTANTIATE_NULL:
      {
	InstantiateNull *inst = (InstantiateNull *)vidIePtr->castToItemExpr();
	if ( NOT inst->NoCheckforLeftToInnerJoin )
	{
	  // if not a left join transformation
	  colStatsPtr = getColStatsPtrForColumn( vid );
	  if ( colStatsPtr != NULL ) return colStatsPtr;
	  break;
	}
	else
	{
	  const ValueId & childVid = vidIePtr->child(0).getValueId();
	  const ItemExpr * childVidIePtr = childVid.getItemExpr();
	  if ( childVidIePtr->getOperatorType() == ITM_VEG_REFERENCE )
	  {
            VEG *veg = ((VEGReference *)childVidIePtr)->getVEG();

            if (veg->seenBefore())
              break;

            veg->markAsSeenBefore();
            const ValueIdSet& vegGroup = veg->getAllValues();
            colStatsPtr = getColStatsPtrForVEGGroup( vegGroup );

            veg->markAsNotSeenBefore();

            if ( colStatsPtr != NULL ) return colStatsPtr;
            break;
	  }
	  else if (    childVidIePtr->getOperatorType() == ITM_BASECOLUMN
		    OR childVidIePtr->getOperatorType() == ITM_INDEXCOLUMN
		  )
	  {
	    colStatsPtr = getColStatsPtrForColumn( childVid );
	    if ( colStatsPtr != NULL ) return colStatsPtr;
	    break;
	  }
	}
	break;
      }

    case ITM_VEG_REFERENCE:
      {
        VEG *veg = ((VEGReference *)vidIePtr)->getVEG();

        if (veg->seenBefore())
          break;

        veg->markAsSeenBefore();

        // Get all members of VEGRef:
        const ValueIdSet& vegGroup = veg->getAllValues();

        colStatsPtr = getColStatsPtrForVEGGroup( vegGroup );

        veg->markAsNotSeenBefore();

	if ( colStatsPtr != NULL ) return colStatsPtr;
	break;
      }

    case ITM_PI:
    case ITM_CACHE_PARAM:
	case ITM_CONSTANT:
	case ITM_HOSTVAR:
	case ITM_DYN_PARAM:
	case ITM_CURRENT_USER:
	case ITM_SESSION_USER:
        case ITM_CURRENT_TIMESTAMP:
	case ITM_GET_TRIGGERS_STATUS:
	case ITM_UNIQUE_EXECUTE_ID:
	  continue;
    default:
      // couldn't find the histogram, continue to look for other 
      // values in a VEG group. Also collect the leaf values if
      // it is an expression  for which we can use histograms. 
      // These may be used later if we are unable to find any histograms
      ItemExpr * leafValue = vidIePtr->getLeafValueIfUseStats();
      if (leafValue != vidIePtr)
      {
	ValueIdSet lvSet;
	if (leafValue->getOperatorType() == ITM_CASE)
	{
	  leafValue->getLeafValueIdsForCaseExpr(lvSet);
	}
	else
	  leafValue->findAll(ITM_BASECOLUMN, lvSet, TRUE, TRUE);
        leafValuesForExpr.addSet(lvSet);
      }
	continue;
    } // end case

  } // end for

  // if we did not find any histograms till now, and there were some
  // expressions in the VEG for which we can use stats, 
  if ((colStatsPtr == NULL) && (leafValuesForExpr.entries() > 0))
  {
    CollIndex idx;
    if (getColStatDescIndexForColWithMaxUec(idx, leafValuesForExpr))
      colStatsPtr = (*this)[idx]->getColStats();
  }

  return colStatsPtr; // if we get here, this should be NULL

} // ColStatDescList::getColStatsPtrForVEGGroup(const ValueIdSet& VEGGroup)

CostScalar
ColStatDescList::getUecOfJoiningCols(ValueIdSet & joinedColSet) const
{
  CostScalar minUec = COSTSCALAR_MAX;
  CostScalar currUec = csOne;

  for (ValueId vid = joinedColSet.init();
       joinedColSet.next(vid);
       joinedColSet.advance(vid) )
       {
	  ColStatsSharedPtr colStats = this->getColStatsPtrForColumn(vid);
	  if (colStats)
	  {
	    currUec = colStats->getTotalUec();
	    if (currUec < minUec)
	      minUec = currUec;
	  } // if ColStats
       } // end for joinedColSet

       return minUec;

} // ColStatDescList::getUecOfJoiningCols

// LCOV_EXCL_START - dpm

void
ColStatDescList::print (ValueIdList selectListCols,
                        FILE *ofd,
                        const char * prefix,
                        const char * suffix,
                        CollHeap *c, char *buf,
                        NABoolean hideDetail) const
{
  NABoolean atLeastOnePrinted = FALSE;
  NABoolean runningShowQueryStatsCmd = (selectListCols.entries() > 0);
  Space * space = (Space *)c;
  char mybuf[1000];

  sprintf(mybuf, 
          "**************************************************************\n");
  PRINTIT(ofd, c, space, buf, mybuf);

  for (CollIndex colStatDescIndex=0;
       colStatDescIndex < entries();
       colStatDescIndex++)
  {
    ColStatDescSharedPtr statDesc = (*this)[colStatDescIndex];
    if(runningShowQueryStatsCmd)
    {
      if(!selectListCols.contains(statDesc->getVEGColumn()) &&
        (atLeastOnePrinted || colStatDescIndex < (entries()-1)))
        continue;
    }
	  
    if (atLeastOnePrinted)
    {
      sprintf(mybuf, 
              "-------------------------------------------------------\n") ;
      PRINTIT(ofd, c, space, buf, mybuf);
    }
    else
      atLeastOnePrinted = TRUE;    

    statDesc->print(ofd,prefix,suffix,c,buf,hideDetail);
  }
    
  sprintf(mybuf, 
          "**************************************************************\n");
  PRINTIT(ofd, c, space, buf, mybuf);
}
// LCOV_EXCL_STOP

void
ColStatDescList::display() const
{
  ValueIdList emptySelectList;
  print(emptySelectList);
}

void
ColStatDescList::verifyInternalConsistency(CollIndex start, CollIndex end) const
{
  CCMPASSERT ( start <= end ); // misuse of function!
  if ( start >= entries() ) return;
  if ( end   >  entries() ) return;

  const CostScalar & matchRowcount =
    (*this)[start]->getColStats()->getRowcount();
  CollIndex i = start + 1;
  for ( ; i < end; i++ )
  {
    const CostScalar & rc = (*this)[i]->getColStats()->getRowcount();
    if ( NOT ( rc == matchRowcount ) )
    {
#ifdef MONITOR_SAMEROWCOUNT
      this->display() ;
      CCMPASSERT ( (*this)[i]->getColStats()->getRowcount() == matchRowcount );
#endif
      break ;
    }
  }

  // now handle the case of histograms with zero entries
  for ( i = start; i < end; i++ )
  {
    if ( (*this)[i]->getColStats()->getHistogram()->entries() == 0 )
    {
      ColStatsSharedPtr stats = (*this)[i]->getColStatsToModify();
      stats->setToSingleInterval (
	            stats->getMinValue(),
	            stats->getMaxValue(),
	            stats->getRowcount(),
	            stats->getTotalUec()
	            );
      stats->setRedFactor    ( csOne );
      stats->setUecRedFactor ( csOne );
      stats->setFakeHistogram();
    }
  }
}

// this function has a very simple algorithm:
// (1) if the histograms all have the same rowcount, done
// (2) otherwise, find the first that's not a fake histogram; set all to
//     have its rowcount
// (3) otherwise, find average of all rowcounts, set all to this value

void
ColStatDescList::enforceInternalConsistency(CollIndex start,
                                            CollIndex end,
                                            NABoolean printNoStatsWarning)
{
  CCMPASSERT ( start <= end ); // misuse of function!
  if ( start >= entries() ) return;
  if ( end   >  entries() ) return;

  // first handle the case of histograms with zero entries
  //
  // --> At the same time, if param "printNoStatsWarning" is TRUE, then
  // fire off a warning message for every fake Colstats that has the
  // isUpStatsNeeded() flag set.  Note that we leave this flag set in
  // order to enable other we-need-stats code in other routines.
  CollIndex i = start;
  for (; i < end; i++ )
  {
    ColStatDescSharedPtr cdesc = (*this)[i];
    ColStatsSharedPtr cs = cdesc->getColStats();

    if (cs == NULL) 
    {
      CCMPASSERT(cs != NULL);
      continue;
    }

    if (cs->getHistogram() == NULL)
    {
// LCOV_EXCL_START - rfi
       CCMPASSERT("Histogram is NULL");
       cs->insertZeroInterval();
// LCOV_EXCL_STOP
    }

    if ( cs->getHistogram()->entries() == 0 )
    {
      cs = (*this)[i]->getColStatsToModify() ; // why we did the const-cast
      cs->setToSingleInterval (
	        cs->getMinValue(),
	        cs->getMaxValue(),
	        cs->getRowcount(),
	        cs->getTotalUec()
	        ) ;
      cs->setRedFactor    ( csOne );
      cs->setUecRedFactor ( csOne );
      cs->setFakeHistogram();
    }

    // Warning 6008 was earlier given for all columns, even if they did
    // not participate in the query. It is now given for only those columns whose
    // histograms are needed.
    // All missing Stats warning are controlled by the CQD HIST_MISSING_STATS_WARNING_LEVEL
    // Warnings are displayed only if the value of the CQD is greater than 0
    if(cs->getStatColumns().entries())
    {
      if (  printNoStatsWarning && cs->isUpStatsNeeded() )
      {
        if ( ( cs->isFakeHistogram() ) || (cs->isSmallSampleHistogram()) ) // if fake or small sample histogram, fire off the warning!
        {
          ValueId colId = cdesc->getColumn();
          BaseColumn * colExpr = colId.castToBaseColumn();

          if (colExpr != NULL)
          {
            // By this time we have ensured that it is a base column
            TableDesc * tableDescForCol = colExpr->getTableDesc();
            const MultiColumnUecList * ueclist =  getUecList() ;  
            NABoolean quickStats = FALSE;
            if (cs->isSmallSampleHistogram())
              quickStats = TRUE;
            ueclist->displayMissingStatsWarning(tableDescForCol,
				                  colId,
				                  TRUE,
                                                  TRUE,
                                                  *this,
                                                  csMinusOne, // displaying missing single column warnings
                                                  quickStats);
          }
        }
      }
    }
  }

  NABoolean allHaveSameRowcount = TRUE;

  const CostScalar & matchRowcount =
    (*this)[start]->getColStats()->getRowcount();

  for ( i = start+1; i < end; i++ )
  {
    const CostScalar & rc = (*this)[i]->getColStats()->getRowcount();
    if ( NOT ( rc == matchRowcount ) )
    {
      allHaveSameRowcount = FALSE;
      break ;
    }
  }

  if ( allHaveSameRowcount ) return; // CASE (1), done

  // OK, they're inconsistent ... rectifying ...

  CollIndex firstNonFake = NULL_COLL_INDEX;
  CostScalar firstNonFakeRowcount = csMinusOne;

  for ( i = start; i < end; i++ )
  {
    if ( NOT (*this)[i]->getColStats()->isFakeHistogram() )
    {
      firstNonFake = i;
      firstNonFakeRowcount = (*this)[i]->getColStats()->getRowcount();
      break;
    }
  }

  if ( firstNonFake != NULL_COLL_INDEX ) // CASE (2)
  {
    if (firstNonFakeRowcount < 0)
    {
// LCOV_EXCL_START - rfi
      CCMPASSERT( firstNonFakeRowcount.isGreaterOrEqualThanZero() );
      firstNonFakeRowcount = 0;
// LCOV_EXCL_STOP
    }

    for ( i = start; i < end; i++ )
    {
      const CostScalar & rc = (*this)[i]->getColStats()->getRowcount();
      (*this)[i]->synchronizeStats( rc, firstNonFakeRowcount );
    }
    return ; // done
  }

  // OK, they're all fake histograms ... can't do much better than averaging them ... sigh

  // CASE (3)
  CostScalar sumRC = csZero;
  for ( i = start; i < end; i++ )
  {
    sumRC += (*this)[i]->getColStats()->getRowcount();
  }

  CostScalar avgRC = sumRC / ( end - start );
  for ( i = start; i < end; i++ )
  {
    const CostScalar & rc = (*this)[i]->getColStats()->getRowcount();
    (*this)[i]->synchronizeStats( rc, avgRC );
  }
  // done, finally
}

// LCOV_EXCL_START - cnu
ValueIdSet ColStatDescList::appliedPreds () const
{
  ValueIdSet result;
  for ( CollIndex i = 0; i < entries(); i++ )
  {
    result += (*this)[i]->appliedPreds();
  }
  return result;
}

ValueIdSet ColStatDescList::VEGColumns () const
{
  ValueIdSet result;
  for ( CollIndex i = 0; i < entries(); i++ )
  {
    result += (*this)[i]->VEGColumn();
  }
  return result;
}
// LCOV_EXCL_STOP


// -----------------------------------------------------------------------
//  methods on MultiColumnUecList class
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// MultiColumnUecList::HashFunction
//
// we need some sort of hashing function in order to use NAHASHDICTIONARY;
// this is a first effort, obviously fairly naive and unsophisticated.
// -----------------------------------------------------------------------


ULng32
MultiColumnUecList::HashFunction (const ValueIdSet & input)
{
  ULng32 retval = 1 + input.entries();

  for ( ValueId id = input.init(); input.next(id); input.advance(id) )
    retval += (CollIndex) id;  // add up the ValueId's

  return retval ;
}

// default constructor

MultiColumnUecList::MultiColumnUecList () :	
     HASHDICTIONARY(ValueIdSet,CostScalar) (&(MultiColumnUecList::HashFunction),
                                            17, // original hash size ... why not?
                                            TRUE, // uniqueness constraint
                                            HISTHEAP )
     { };

// -----------------------------------------------------------------------
// MultiColumnUecList :: ctor
//
// builds the MultiColumnUecList from the initial StatsList object
//
// does the NAColumn -> ValueId conversion found in the ColStatDesc ctor
//
// this function is only called from TableDesc::getTableColStats()
// -----------------------------------------------------------------------
MultiColumnUecList::MultiColumnUecList (const StatsList   & initStats,
                                        const ValueIdList & tableColumns) :
     HASHDICTIONARY(ValueIdSet,CostScalar) (&(MultiColumnUecList::HashFunction),
                                            17, // original hash size ... why not?
                                            TRUE, // uniqueness constraint
                                            HISTHEAP )
{
  // the StatsList has two lists which it uses to store the information we
  // need to fill the MultiColumnUecList with <table-col-list,uec value> pairs:
  //
  // LIST(NAColumnArray) groupUecColumns_
  // LIST(CostScalar)    groupUecValues_

  CostScalar rowCount = initStats[0]->getRowcount();

  // loop through the list of NAColumnArray's
  for ( CollIndex i = 0; i < initStats.groupUecColumns_.entries(); i++ )
  {
    const NAColumnArray & uecCols = initStats.groupUecColumns_[i];

    ValueIdSet insertCols;

	// multiColUec
	CostScalar multiColUec(initStats.groupUecValues_[i]);

	// Upper limit to the multi column UEC
	CostScalar maxMCUec = csOne;

	// lower limit to the multi-col UEC
	CostScalar minMCUec = csOne;

	// would be set to TRUE if any histogram is missing statistics
	NABoolean statsMissing = FALSE;

    // for each NAColumnArray, map each NAColumn to a ValueId
    for ( CollIndex j = 0; j < uecCols.entries(); j++ )
    {
	  Lng32 position = uecCols[j]->getPosition();
      const ValueId & id = tableColumns[position];
	  CostScalar singleColUec = initStats.getSingleColumnUECCount(position);

	  if (singleColUec < 1)
		statsMissing = TRUE;

	  //MCUec cannot be less than the UEC count of any of the columns that
	  //are included in the MCHistogram.

      minMCUec = MAXOF(minMCUec, singleColUec);

	  maxMCUec *= singleColUec;

      insertCols.insert( id );
    }

	// The prodUec could be negative if it involves a column that is not
	// being referenced in the query. In such cases, we shall use rowcount
	// as the upper limit

	if (statsMissing)
	  maxMCUec = rowCount;
	else
	  maxMCUec = MINOF(maxMCUec, rowCount);

	// multi-col UEC should not exceed the product of single col UEC
	// or row count which ever is smaller
	// And multi-col UEC should be at least equal to max single column
	// UEC of columns participating in multi-col

	if (multiColUec < minMCUec)
	  multiColUec = minMCUec;

	if (multiColUec > maxMCUec)
	  multiColUec = maxMCUec;

	// now we've converted each NAColumn->ValueId in a particular
    // NAColumnArray; now insert this list and its corresponding uec

    insertPair( insertCols, multiColUec );
  }
}

// -----------------------------------------------------------------------
// MultiColumnUecList::insertList
//
// inserts all entries from OTHER into THIS (unless a particular entry
// already exists in THIS)
// -----------------------------------------------------------------------
void
MultiColumnUecList::insertList (const MultiColumnUecList * other)
{
  if ( other == NULL ) return;
  if ( other == this ) return;
  if ( other->entries() == 0 ) return;

  ValueIdSet * keyEntry = NULL;
  CostScalar * uecEntry = NULL;

  MultiColumnUecListIterator iter( *other );

  iter.getNext( keyEntry, uecEntry );

  while ( keyEntry != NULL && uecEntry != NULL )
  {
    if ( NOT contains( keyEntry ) )
      insertPair( *keyEntry, *uecEntry );

    iter.getNext( keyEntry, uecEntry );
  }
}

// -----------------------------------------------------------------------
// MultiColumnUecList::insertMappedList
//
// inserts all entries from OTHER into THIS, after mapping the ValueIds
// in the list, using MAP. Note that we do the mapping in the "up"
// direction.
// -----------------------------------------------------------------------
void
MultiColumnUecList::insertMappedList(const MultiColumnUecList *other,
                                     const ValueIdMap &map)
{
  if ( other == NULL ) return;
  if ( other->entries() == 0 ) return;

  ValueIdSet * keyEntry = NULL;
  CostScalar * uecEntry = NULL;

  MultiColumnUecListIterator iter( *other );

  iter.getNext( keyEntry, uecEntry );

  while ( keyEntry != NULL && uecEntry != NULL )
    {
      ValueIdSet *mappedSet = new(HISTHEAP) ValueIdSet;

      map.mapValueIdSetUp(*mappedSet, *keyEntry);
      // Todo: CSE: This is unlikely to work, since the stats will be
      // expressed in BaseColumns, while the map contains VEGRefs.
      // Uncomment the assert below and run compGeneral/TEST045
      // to see the problem.
      // DCMPASSERT(*mappedSet != *keyEntry);
      if ( NOT contains( mappedSet ) )
        insertPair( *mappedSet, *uecEntry );

      iter.getNext( keyEntry, uecEntry );
    }
}

// -----------------------------------------------------------------------
// MultiColumnUecList::insertPair
//
// inserts a <table-column-valueidset, groupUec> pair
// -----------------------------------------------------------------------
NABoolean
MultiColumnUecList::insertPair (const ValueIdSet & columns,
				const CostScalar & groupUec)
{
  ValueIdSet * columnsCopy  = new (HISTHEAP) ValueIdSet( columns );
  CostScalar * groupUecCopy = new (HISTHEAP) CostScalar( groupUec );
  if ( columnsCopy ==
       (NAHashDictionary<ValueIdSet,CostScalar>::insert( columnsCopy, groupUecCopy )) )
    return TRUE;  // insert successful
  else
    return FALSE; // insert failed
}

// -----------------------------------------------------------------------
// MultiColumnUecList::updatePair
//
// updates the groupUec of <table-column-valueidset, groupUec> pair
// -----------------------------------------------------------------------
NABoolean
MultiColumnUecList::updatePair (const ValueIdSet & columns,
				const CostScalar & groupUec)
{
  ValueIdSet * columnsCopy  = new (HISTHEAP) ValueIdSet( columns );
  if ( columnsCopy ==
       (NAHashDictionary<ValueIdSet,CostScalar>::remove( columnsCopy)) )
  {
    if ( insertPair( columns, groupUec ) )
    return TRUE;
  }// update successful
  return FALSE; // update failed
}


// -----------------------------------------------------------------------
// MultiColumnUecList::lookup
//
// given a <table-column-valueidset>, returns the corresponding group uec
// value
// -----------------------------------------------------------------------
CostScalar
MultiColumnUecList::lookup (const ValueIdSet & key) const
{
  CostScalar groupUec = csMinusOne;
  if ( contains( &key ) )
    groupUec = *(getFirstValue( &key ) );
  return groupUec;
}

// ------------------------------------------------------------------------------
// This method is used for setting the multi-column UEC for unique indexes equal
// to the row count. If the multi_column statistics for unique indexes, does not exist
// it is created by setting the column list equal to the column list from
// the index, and uec equal to the base row count of the table.
// ------------------------------------------------------------------------------
void MultiColumnUecList::initializeMCUecForUniqueIndxes(TableDesc &table,
							const CostScalar & tableRowcount)
{

  const NAFileSetList indexList = table.getNATable()->getIndexList();
  const ValueIdList &tableColumns = table.getColumnList();

  for (CollIndex listi = 0; listi < indexList.entries(); listi++)
  {
    if (indexList[listi]->uniqueIndex())
    {
      const NAColumnArray & uecCols = indexList[listi]->getIndexKeyColumns();
      ValueIdSet insertCols;
      for (CollIndex j = 0; j < uecCols.entries(); j++)
      {
	Lng32 position = uecCols[j]->getPosition();
	const ValueId & id = tableColumns[position];
	insertCols.insert(id);
      } // for all columns in the index
      // see if the multi_column statistics exists for the given set of column
      CostScalar multUecRowCount = -1;

      if ((multUecRowCount = lookup(insertCols)) > 0)
      {
	// if the row count from statistics equal to the table rowcount
	if (multUecRowCount != tableRowcount)
	  updatePair(insertCols,tableRowcount);
      }
      else
	insertPair(insertCols,tableRowcount);
    } // if the index is unique
  } // for all indexes of the table
  return;
} // initializeMCUecForUniqueIndexes

//---------------------------------------------------------------------
//MultiColumnUecList::getListOfSubsetsContainsColumns
//
//Input: list of columns
//Output: List of ValueIdSet that contains the last column in the list
//and other columns from the columnList only
//Constraints: ColumnIds that are passed in can be VegRef that contains
//             the base id for that column at the first level or it can
//             be a the id corresponding to a index on the table.
//---------------------------------------------------------------------

LIST(ValueIdSet) *
MultiColumnUecList::getListOfSubsetsContainsColumns(
  const ValueIdList & columns/*in*/,
  LIST(CostScalar)& uecCount/*out*/
  ) const
{
  LIST(ValueIdSet) * result = new (HISTHEAP) LIST( ValueIdSet )(HISTHEAP);

  ValueIdSet allValueIds;
  ValueIdSet colValueIds;
  CollIndex members = columns.entries();
  ValueId column;
  //Following we try to get all the columnids that the column ids can
  //be associated with so that when we cross reference
  //multi-column uec list we don't miss out. We create two list
  //colValueIds -- all value Ids of the last/main column
  //allValueIds -- all value Ids of the rest of the columns

  for(CollIndex i=0; i<members;i++)
  {
    column = columns[i];
    const ItemExpr * itemExprForCol = column.getItemExpr();

    if( itemExprForCol->getOperatorType() == ITM_VEG_REFERENCE )
    {
      if(i==members-1)
        colValueIds = ((VEGReference *)itemExprForCol)->getVEG()->getAllValues();
      else
        allValueIds += ((VEGReference *)itemExprForCol)->getVEG()->getAllValues();
    }
    else if ( itemExprForCol->getOperatorType() == ITM_INDEXCOLUMN )
    {
      const BaseColumn * bc;
      bc = (BaseColumn *)((IndexColumn *)itemExprForCol)->getDefinition().getItemExpr();
      if(i==members-1)
      {
        colValueIds.insert( bc->getEIC() );
        //inserts the id this column has on base table
        colValueIds.insert( bc->getValueId() );
      }
      else
      {
        //inserts other id's this column might have on other indexes
        allValueIds.insert( bc->getEIC() );
        //inserts the id this column has on base table
        allValueIds.insert( bc->getValueId() );
      }
    }
    //for the case when id is the id in the base table
    //and for extra precaution
    if(i==members-1)
      colValueIds.insert(column);
    else
      allValueIds.insert( column );
  }

#ifndef NDEBUG
  if(getenv("MDAM_MCUEC"))
  {
// LCOV_EXCL_START - dpm
    fprintf(stdout, " \n\n-----List to be considered----\n");
    allValueIds.print();
// LCOV_EXCL_STOP
  }
#endif

  MultiColumnUecListIterator iter(*this);

  ValueIdSet * keyEntry = NULL;
  CostScalar * uecEntry = NULL;

  CollIndex position = 0;   // position to enter in the list

  // following we traverse the mulcolueclist and see if
  //keyEntry contains the column, then check if it contains
  //any other column other than the columns passed in, if
  //it doesn't then it is a viable entry.
  //Ex. if abcd are the columns passed in, we first look for
  // d if the multi-columnUEC entry and if contains that then
  // we look for abc and if it contains any or all of them but
  // not any other column like 'e' or 'f' then it is a viable
  // multi-column histogram.
  for ( iter.getNext( keyEntry, uecEntry );
        keyEntry != NULL && uecEntry != NULL;
	iter.getNext( keyEntry, uecEntry ) )
  {
    ValueIdSet tempSet = *keyEntry;
    tempSet.removeCoveredVIdSet(colValueIds);
    if(tempSet.entries() < keyEntry->entries())
    {
      tempSet.removeCoveredVIdSet(allValueIds);

      if(tempSet.entries()==0)
      {
      result->insertAt( position, *keyEntry );
      uecCount.insertAt( position, *uecEntry );
      position++;
      }
    }
  }

  return result;
}

// -----------------------------------------------------------------------
// MultiColumnUecList::largestSubset
//
//Input:ValueIdSet(columns) of column valueIds
//Output:  Returns the a valueIdSet that matches most valueIds from ValueIdSet
//(columns) but does not contain any other valueIds.
//Ex: for valueIdSet (1, 4, 6)
// we had mulcolUec for (1,4,6,8) and (1,4,7) and (1)
// we would return (1) because the other two contains unmentioned columns
// In case of tie between two list we select the one with largest correlation
//Constraints: ValueIdSet passed in has to be base table Ids because
//multicol is stored using base table ids.
// -----------------------------------------------------------------------
ValueIdSet
MultiColumnUecList::largestSubset (const ValueIdSet & columns) const
{
  ValueIdSet largestSet;        // the largest subset found thus far

  // This is the smallest independence factor, It determines the largest
  // correlation between the columns. Smaller the IP factor, larger is
  // the correlation between columns
  double smallestIPFactor = 1.0;

  if ( entries() == 0 OR columns.entries() == 0 )
    return largestSet;   // no subsets at all!

  ValueIdSet * keyEntry = NULL;
  CostScalar * uecEntry = NULL;

  // we need to iterate through all entries in this list
  MultiColumnUecListIterator iter( *this );

  for ( iter.getNext( keyEntry, uecEntry );
	keyEntry != NULL && uecEntry != NULL;
	iter.getNext( keyEntry, uecEntry ) )
  {
    ValueIdSet tempSet = *keyEntry;
    //we do not want extra columns so after we remove the
    //columns asked for we should have 0 entries left.
    tempSet.removeCoveredVIdSet( columns );
    if( tempSet.entries() > 0 )
      continue;

    // want to find the entry in the multicolumnueclist which contains
    // group uec information about the most columns

    //We want to select this entry if it has more matching columns
    //Or if it has same number of columns then it has to have smaller
    // correlation factor

    if ( keyEntry->entries() >= largestSet.entries() )
    {
      double independenceFactor = 0.0;
      double SCproductUec = 1.0;

      for (ValueId keyCol = keyEntry->init();
			    keyEntry->next(keyCol);
			    keyEntry->advance(keyCol) )
	  SCproductUec *= lookup(keyCol).value();

      independenceFactor = uecEntry->value()/SCproductUec;

      if ( ( keyEntry->entries() > largestSet.entries() )
	  OR
	  ( (keyEntry->entries() == largestSet.entries())
	    AND (keyEntry->entries() > 0 )
	    AND ( independenceFactor < smallestIPFactor ) ) )
      {
	largestSet = *keyEntry;
	smallestIPFactor = independenceFactor;
      }
    }
  } // end for
  return largestSet;
}

//---------------------------------------------------------------------
//MultiColumnUecList::findDenom
//
//Input: list of columns
//Output: Boolean. if there is a multi-column histogram exactly matching
//the input then return true or return false
//Constraints: ValueIds for the columns need to be base valueIds.
//---------------------------------------------------------------------
NABoolean
MultiColumnUecList::findDenom (const ValueIdSet & columns) const
{
  if ( entries() == 0 OR columns.entries() == 0 )
    return FALSE;   // no subsets at all!

  ValueIdSet * keyEntry = NULL;
  CostScalar * uecEntry = NULL;

  // we need to iterate through all entries in this list
  MultiColumnUecListIterator iter( *this );

  for ( iter.getNext( keyEntry, uecEntry );
	keyEntry != NULL && uecEntry != NULL;
	iter.getNext( keyEntry, uecEntry ) )
  {
    if(columns == *keyEntry)
    {
      return TRUE;
    }
  }
  return FALSE;
}


// -----------------------------------------------------------------------
// MultiColumnUecList::useMCUecforCorrPreds
//
// used to calculate an adjustment in the case of multiple predicates being
// applied to highly correlated table columns
//   (fn useMultiUecIfCorrelatedPreds(), subr of
//    fn estimateCardinality() )
//
// given a list of <ValueId, CostScalar> pairs representing all of the
// histograms which have been reduced, and the amount (reduction factor)
// they've been reduced, return TRUE/FALSE if, in the list of these
// predicates, there are 2+ from the same table for which we have
// multi-column uec information and which are "highly correlated"
// (defined below).
//
// If both of these conditions are met, then we supply a factor
// "reductionAdjustment" which should be applied to the current rowcount
// estimate in order to increase it beyond its current value, to take
// into account the fact that we are applying multiple predicates to
// highly correlated columns, which we assume means that beyond the
// most selective predicate, the additional predicates are redundant
// in part or whole (i.e., they remove the "same rows" as the other
// predicates).
//
// We basically check if the row reduction has taken the number of rows
// below the number of rows per multicolumnUec. If so reduction adjustment
// will readjust the number of rows appropriately. Any call to this function
// does not return a reduction adjustment that will let the number of rows be
// less than one.
// Constraints: The valueIds in NAHashDictionary should be base table valueIds
// for those columns



// all tablePtrs should be 4-byte aligned, so divide by 4
// to get a better hash value
ULng32 TableDescHashFn (const TableDesc & tablePtr)
{ return (ULng32)((Long)&tablePtr/8) ; }

#pragma nowarn(262)   // warning elimination
NABoolean
MultiColumnUecList::useMCUecForCorrPreds (
     NAHashDictionary<ValueId, CostScalar> & predReductions, /* in/mod */
     const CollIndex numPredicates,                          /* in */
     const CostScalar& oldRowCount,                          /* in */
     const CostScalar& newRowCount,                          /* in */
     NABoolean largeTableNeedsStats,
     const ColStatDescList & scHists,
     CostScalar & reductionAdjustment)                  /* out */
{
#define USE_MULTI_COL_UEC_INFO

  if ( numPredicates < 2 )
    return FALSE;

// -----------------------------------------------------------------------
// First, we need to identify the table which has the most column
// references in our list of <pred column, rowcount reduction> pairs.
//
// We use a hash-dictionary to implement an association list of
// <TableDesc*, ValueIdList> pairs -- each ValueIdList indicates
// which ValueId's we have that touch the TableDesc* in question.
// -----------------------------------------------------------------------

  NAHashDictionary<TableDesc,ValueIdList> tableColumns (
       &TableDescHashFn,        7, TRUE, HISTHEAP );

  // we need to iterate over the <ValueId, CostScalar> pairs in order
  // to fill the NAHashDictionary above (tableColumns) with values
  NAHashDictionaryIterator<ValueId, CostScalar> predIter( predReductions );
  ValueId    * predColumn = NULL;
  CostScalar * reduction  = NULL;

  for ( predIter.getNext( predColumn, reduction );
	predColumn != NULL && reduction != NULL;
	predIter.getNext( predColumn, reduction ) )
  {
    const ValueId & iterId = *predColumn;
    // initially assume ITM_BASETABLE
    BaseColumn * iterExpr = iterId.castToBaseColumn();
    if ( iterExpr == NULL ) return FALSE; // unexpected condition
    TableDesc  * iterDesc = iterExpr->getTableDesc();

    if (iterDesc == NULL)
    {
// LCOV_EXCL_START - rfi
      CCMPASSERT( iterDesc != NULL );
      return FALSE;
// LCOV_EXCL_STOP
    }

    // do a lookup in the hash dictionary we're currently populating
    ValueIdList * colList = tableColumns.getFirstValue( iterDesc );

    if ( colList != NULL )
    {
      colList->insert( iterId ); // the joinValueIdPair which refs this tableDesc
    }
    else
    {
      colList = new (HISTHEAP) ValueIdList;
      colList->insert( iterId );
      tableColumns.insert( iterDesc, colList );
    }
  }

  // -----------------------------------------------------------------------
  // We have lists of references to tableDesc's; now, we iterate through them
  // to find the one with the most table columns.
  //
  // If there isn't a table with two column references, we quit.
  //
  // If there is a tie between two tables for most column refs, we
  // pick the one with higher multi-col UEC.
  // Table iterator to traverse the tables whose columns have been reduced
  NAHashDictionaryIterator<TableDesc,ValueIdList> tableIter( tableColumns );

  // This would contain the running descriptor of the table that we are
  // iterating from the list of tables with predicates.
  TableDesc   * iterDesc = NULL;

  // This would contain the columns on which we have local predicates for
  // the table described in iterDesc. If iterDesc contains T1, then iterList
  // would contain (a,b,c). If iterDesc is T2, iterList would be (a,b,c,d)
  ValueIdList * iterList = NULL;

  // This would contain the ValuedIdSet of the largest set of columns which
  // have multi-col stats available. The columns are from the amongst the
  // ones which have most predicates on them. Example, we have predicates
  // on columns, (a,b,c) of table T1 and (a,b,c,d) of table T2. At the end of the
  // loop, mostRefs would be 4, and if there is multi_col stats available for
  // T2 (a, b) and T2(a,b,c), then largestSubsetAmongSubsets would contain
  // (a,b,c), and mostRefdTable would contain T2

  // Table with multi-column stats on most columns. It would be T2 for us.
  TableDesc * mostRefdTable = NULL;

  // This would contain the list of columns which have most predicates on them
  // At the end of the loop it should contain T2 (a,b,c,d)
  ValueIdList * superList;

  // This would contain the count of columns of a table which has most local
  // predicates. For the above example it would be 4, at the end of the loop
  CollIndex mostSupersetRefs = 0;

  // largest set of columns which have multi-col UECs for any table.
  ValueIdSet largestSubsetAmongLargestSubsets;

  // contains the maximum number of columns which have multi-col UECs available
  CollIndex mostRefs = 0;

  CollIndex count = 1;

  // This will contain the multi-col UEC for largestSubsetAmongLargestSubsets.
  // It will be used to pick the final multi-col UEC in case there is a tie
  // between the number of columns of two tables, pick the ones with higher
  // multi-col UEC
  CostScalar multiColumnUec = csMinusOne;

    for ( tableIter.getNext( iterDesc, iterList );
	  iterDesc != NULL && iterList != NULL;
	  tableIter.getNext( iterDesc, iterList ) )
    {
      // in this loop we go through all the columns of this table that were
      // reduced collecting partial muli-col UEC lists. These partial multi-col
      // UEC lists could be overlapping or disjoint. Preference is given to
      // overlapping multi-col UECs over disjoint multi-col UECs. Partial
      // multi-col UECs are combined as follows:MC UEC needed (a, b, c, d)
      // For overlapping: MC-UEC available - (a, b, c) (c, d).
      // MC (a, b, c, d) = MC (a, b, c) * MC (c, d) / MC (c)
      // For disjoint: MC-UEC available (a, b) (c, d)
      // MC (a, b, c, d) = MC(a, b) * MC(c, d)

      if ( iterList->entries() > mostSupersetRefs )
      {
		mostSupersetRefs = iterList->entries();
		superList = iterList;
      }

      // Less than two columns of this table have been reduced.
      // Hence cannot use multi-col UEC
      if (iterList->entries() < 2) continue;

      // Contains the set of columns with highest UEC from amongst all tables
      // It contains the cumulative set of all columns which have multi-col UEC
      // available. For example, if we have predicates on column (a, b, c, d, e)
      // and multi-col UEC available for (a, b), (c, d). Then this would contain
      // (a, b, c, d). Later
      ValueIdSet cumulativeColSetWithMCUEC;

      // contains the multi col UEC for cumulativeColSetWithMCUEC, In case of partial
      // multi-col UEC, it is a function of all partial multi-col UECs for columns
      // in cumulativeColSetWithMCUEC, as described above.
      // In case there are two tables with same number of columns competing for
      // multi-colUEC this variable would contain the higher MC-UEC.
      // Correspondingly cumulativeColSetWithMCUEC would contain the column
      // set with higher multi-col UEC. For example, T1 (a, b, c, d) has
      // multi-col UEC = 1000 and T2 (a, b, c, d) has multi-col UEC = 1200
      // cumulativeColSetWithMCUEC would contain T2 (a, b, c, d) and
      // maxMultiColUec = 1200

      CostScalar maxMultiColUec = csOne;

      // colsWithReductions contain the columns remaining to be checked for
      // multi-col UEC
      ValueIdSet colsWithReductions(*iterList);

      // See is multi-column UEC exists for all the columns of the table which
      // have local predicates on them. That is the best case.

      CostScalar mcUec = lookup(colsWithReductions);
      if (mcUec.isGreaterThanZero() )
      {
        maxMultiColUec = mcUec;
        cumulativeColSetWithMCUEC.addSet(colsWithReductions);
      }
      else
      {
        // we had more than one column with reduction for this table
        // but there is no multi-col UEC.
        // If this iterList forms a unique index, we shall fake its multi-col UEC
        // with the rowcount, and continue. Else we shall display missing stats
        // warning for this.
        CostScalar baseRowCount = iterDesc->getNATable()->getOriginalRowCount();

        if (colsWithReductions.doColumnsConstituteUniqueIndex(iterDesc))
        {
          // get base rowcount of the table from original colStats
          // and set the multi-col UEC equal to this rowcount.
          insertPair(colsWithReductions, baseRowCount);

          maxMultiColUec = baseRowCount;
          cumulativeColSetWithMCUEC.addSet(colsWithReductions);
        }
        else
        {
          // we still have a possibility of finding multi-col UEC for this table
          // columns
          // Combine MC UEC of subset of columns from columns with reduction to get 
          // MC UEC of larger set
          // statsCreated will return TRUE only if the MC stats for larger
          // set of columns were created using overlapping subset of columns

          ValueIdSet colsWithPreds = colsWithReductions;
          NABoolean statsCreated = createMCStatsForColumnSet(colsWithReductions, 
                                                              cumulativeColSetWithMCUEC,
                                                              maxMultiColUec,
                                                              baseRowCount
                                                              );

          NABoolean displayWarning;
          if (isMCStatsUseful(colsWithPreds, iterDesc))
            displayWarning = TRUE;
          else
            displayWarning = FALSE;

          // Do not display the warning if we were able to create MC Stats for the
          // full set of columns from subset of overlapping column sets or
          // if the subset of columns requiring MC stats are orthogonal
          if (isMCStatsUseful(colsWithPreds, iterDesc))
          {
            // log selectivity from single column histograms in ustat log
            CostScalar sel = newRowCount /oldRowCount;
            displayMissingStatsWarning(iterDesc, (ValueIdSet)*iterList, largeTableNeedsStats, displayWarning, scHists, sel, FALSE, REL_SCAN);
          }
        } // end else (colsWithReductions.doColumnConstituteUniqueIndex(iterDesc))
      } // end else (mcUec.isGreaterThanZero() )

      // no more multi-col UECs for this table columns
      // Compare with the earlier computed MC UEC for tables
      // to see if this should be picked up. The table, which has
      // MC-UEC available for most columns is chosen. In case of a
      // tie between the two tables, we pick the one with higher MC-UEC.

      if ( (cumulativeColSetWithMCUEC.entries() > largestSubsetAmongLargestSubsets.entries() ) ||
	( (cumulativeColSetWithMCUEC.entries() == largestSubsetAmongLargestSubsets.entries() ) &&
	  (maxMultiColUec > multiColumnUec) ) )
      {
	// This table has multi-col UEC on more number of columns than
	// any of the tables we have iterated till now

	// largestSubsetAmongLargestSubsets now contains the largest
	// column set with multi-col UEC
	largestSubsetAmongLargestSubsets = cumulativeColSetWithMCUEC;

	// multi-col UEC for this set
	multiColumnUec = maxMultiColUec;

	// Table with most columns with multi-col UEC
	mostRefdTable = iterDesc;
	mostRefs = cumulativeColSetWithMCUEC.entries();;
	}
    } // for (tableIter .... )

    // no table with reduction on more than one column
    if (mostSupersetRefs <= 1)
      return FALSE;

    // There are tables with more than one column, but none have multi-col UEC.
    if ( mostRefs <= 1 ) 
    return FALSE;


    // After having gotten the largest multi-col UEC for this table, get the
    // product of predReductions from single column histograms for those columns
    CostScalar uecReduction = csOne;

    if (largestSubsetAmongLargestSubsets.entries() >= 2)
    {
     // accumulate single column reductions of all columns with multi-col UEC
      for ( ValueId i = largestSubsetAmongLargestSubsets.init();
                largestSubsetAmongLargestSubsets.next( i );
                largestSubsetAmongLargestSubsets.advance( i) )
          {
            uecReduction *= (*(predReductions.getFirstValue( &i )));
          }
    }

    //Initial row count of the table before any predicates were applied.
    //Need to use this because multiColumnUec is for that table only so
    //need to stay consistent.
    const ColStatDescList & statsList = mostRefdTable->getTableColStats();

    const CostScalar & initialRowCount = statsList[0]->getColStats()->getRowcount();

    // Following set of conditions are used to check for correlation:
    // and subsequently decide what reduction adjustment to pick
    if(   ( oldRowCount * uecReduction ) < ( initialRowCount / multiColumnUec )
      AND ( initialRowCount / multiColumnUec ) > csOne
      AND (uecReduction.getValue() > 0.0 ) )
    {
      // correlation between the columns exist, and therefore we will be able
      // to benefit from the multi-col UEC
      reductionAdjustment = ( csOne / uecReduction ) * ( csOne / multiColumnUec );
    }
    else if ( (oldRowCount * uecReduction).isLessThanOne()
      AND ( oldRowCount * uecReduction ) < ( initialRowCount / multiColumnUec )
      AND newRowCount.isGreaterThanZero() /* != csZero */)
    {
      // New rowcount obtained from single-column histograms is less than zero.
      // set the reductionAdjustment, so that the final rowcount is set to one
      reductionAdjustment = csOne / newRowCount;
    }
    else
    {
      // Columns are not correlated, so no reduction adjustment is needed
      reductionAdjustment = csOne;
    }

    // contains either the largest number of columns for which the
    // multi-col uec exists or the number of columns which were reduced
    // which ever is smaller.

    count = MINOF( largestSubsetAmongLargestSubsets.entries(),
			     numPredicates );
    return TRUE;
}
#pragma warn(262)  // warning elimination

NABoolean 
MultiColumnUecList::createMCStatsForColumnSet(ValueIdSet colsWithReductions, 
                                              ValueIdSet & cumulativeColSetWithMCUEC,
                                              CostScalar & maxMultiColUec,
                                              CostScalar baseRowCount
                                              )
{
  // accumulate all partial multi-col UEC for columns with predicates
  // for this set. In case of a tie send the one with larger correlation

  cumulativeColSetWithMCUEC = largestSubset(colsWithReductions);

  // If largestSubset.entries() == 1, then that means there are no
  // multi-col UECs available for this table, so there is nothing
  // that we can do. If any multi-col UEC was returned, then we have
  // a chance of availability of partial multi-col UECs, that we can
  // use

  if (cumulativeColSetWithMCUEC.entries() <= 1)
    return FALSE;

  // combine this MC UEC with any partial overlapping multi-col UECs
  maxMultiColUec = lookup(cumulativeColSetWithMCUEC);

  // get the remaining columns for which multi-col UEC could not be found
  colsWithReductions.subtractSet(cumulativeColSetWithMCUEC);

  // In the following two method we try to create multi-column UEC list
  // for larger column set using multi-column UEC for smaller set of columns
  // For this we need to traverse the MC list a number of times.
  // In the following method, we shall create a new MC list.
  // This list contains MC-UEC for only those column set, which include
  // all remaining columns

  MultiColumnUecList * mcListForRemainingCols = createMCListForRemainingCols(
					    colsWithReductions,
					    cumulativeColSetWithMCUEC);

  // We will have two flags to indicate whether the MC stats for a larger set of
  // columns was created using overlap subset of columns or disjoins set of columns
  // If the stats were created using overlap set of columns, we would have been able 
  // to capture correlation pretty well and we need not give a warning. But if the 
  // stats were created using disjoint set of columns, then we should give a warning
  NABoolean statsCreatedWithOverlap = FALSE;
  NABoolean statsCreatedWithDisjoint = FALSE;

  // Use the MC list created above to collect all relevant partial
  // overlapping multi-col UECs first to get the correlation for
  // larger set of columns
  statsCreatedWithOverlap = mcListForRemainingCols->createMCUECWithOverlappingColSets(
					    colsWithReductions,
					    cumulativeColSetWithMCUEC,
					    maxMultiColUec,
					    baseRowCount);

  // If any stats were created from partial multi-col UEC, save them in the
  // multi-col UEC list for future use.
  if (statsCreatedWithOverlap)
  {
    // After all the calculations, the MC UEC should not exceed the
    // original rowcount of the table
    maxMultiColUec = MINOF(maxMultiColUec, baseRowCount);

    this->insertPair(cumulativeColSetWithMCUEC, maxMultiColUec);

    // if MC UEC has reached the limit of rowcount, or we have been
    //  able to generate stats for the complete set, return TRUE
    // else continue to build MC stats from smaller subset of columns
    if ( (maxMultiColUec == baseRowCount) ||
         (colsWithReductions.entries() <= 1) )
      return TRUE;
  }

  // for the remaining columns, use relevant partial disjoint multi-col UECs
  // if computed multi-col UEC has already reached the limit of rowcount
  // then there is no need to proceed further, as we will not get any benefit

  if ( ( maxMultiColUec < baseRowCount) AND
      (colsWithReductions.entries() > 1) )
    statsCreatedWithDisjoint = mcListForRemainingCols->createMCUECWithDisjointColSets(
						  colsWithReductions,
						  cumulativeColSetWithMCUEC,
						  maxMultiColUec,
						  baseRowCount);

  // After all the calculations, the MC UEC should not exceed the
  // original rowcount of the table
  maxMultiColUec = MINOF(maxMultiColUec, baseRowCount);

  // If any stats were created from partial multi-col UEC, save them in the
  // multi-col UEC list for future use.
  if (statsCreatedWithDisjoint) 
    this->insertPair(cumulativeColSetWithMCUEC, maxMultiColUec);

  if (statsCreatedWithOverlap || statsCreatedWithDisjoint)
    return TRUE;
  else
    return FALSE;
} // createMCStatsForColumnSet

// -----------------------------------------------------------------------
// MultiColumnUecList::getUecForMCJoin
//
// used by multi-column join code (fn CSDL::useMultiUecIfMultipleJoins(), subr of
//                                 fn CSDL::estimateCardinality() )
//
// first, we list the struct used internally and the subroutine

// -----------------------
// struct MCJoinPairStruct
//
// this struct is used in the ::getUecForMCJoin() routine to keep track of
// the correspondence between :
//
// (1) a pair of ValueIdSets (representing the columns from two tables);
//
// (2) the larger of these two sets' multi-column uecs (needed by the
// calling function) ;
//
// (3) the join predicates that are not covered by the two ValueIdSets
// (i.e., the preds that we need to address later in order to correctly estimate
// the entire multi-column join's rowcount)
// -----------------------

struct MCJoinPairStruct
{
  ValueIdSet       tableOneCols_ ;
  ValueIdSet       tableTwoCols_ ;
  CostScalar       prodInitUec_ ;
  CostScalar       multiColUec_ ;
  CostScalar       leftMCUec_;
  CostScalar	   baseRowCount_ ;
  NAList<ValueIdSet> * remainingJoinPairs_;

  // stupid Collections classes require this fn! argh!
  NABoolean operator == (const MCJoinPairStruct & rhs)
  { return tableOneCols_ == rhs.tableOneCols_ && tableTwoCols_ == rhs.tableTwoCols_ ; }

public:
  MCJoinPairStruct():remainingJoinPairs_(new (CmpCommon::statementHeap())
                      NAList<ValueIdSet>(CmpCommon::statementHeap()) ) {}

  ~MCJoinPairStruct(){ delete remainingJoinPairs_;}

} ;

// -----------------------
// global function findMatchingColumns()
//
// subroutine of MultiColumnUecList::getUecForMCJoin()
//
// Given a ValueIdSet (t1Cols) representing the columns from one table,
// figure out the corresponding columns from the second table
// (t2Cols==retval) so that a subset of the join predicates (joinPairs)
// are covered by the two sets of table columns.  Any joinPairs not
// covered are placed into (remainingPairs).
//
// This is an ugly little subroutine, not placed in the main code in order
// to (attempt to) keep things a bit clearer.
// -----------------------

ValueIdSet
MultiColumnUecList::findMatchingColumns (
  const ValueIdSet        & t1Cols,     /* in  */
  const LIST(ValueIdList) & joinPairs,  /* in  */
  LIST(ValueIdList) & remainingPairs,   /* out */
  CostScalar        & maxInitUecProduct,/* out */
  CostScalar        & minInitUecProduct,/* out */
  NABoolean	    & insuffMCInfo	/* out */
  ) const
{
  ValueIdSet t2Cols; // #retval

  for ( CollIndex i = 0 ; i < joinPairs.entries(); i++ )
  {
    NABoolean inserted = FALSE ;
    if ( joinPairs[i].entries() > 2 )
    {
// LCOV_EXCL_START - rfi
      // we cannot handle a join between many (>2) columns -- do not
      // use this join predicate
      remainingPairs.insert( joinPairs[i] );
      continue ;
// LCOV_EXCL_STOP
    }

    const ValueId & firstId = joinPairs[i][0];
    const ValueId & secondId = joinPairs[i][1];

    for ( ValueId id = t1Cols.init();
	  t1Cols.next( id );
	  t1Cols.advance( id ) )
    {
      if ( id == firstId || id == secondId )
      {
	// keep a running total of the product of the columns'
	// initial uec. If single column stats do not exist
	// for any column, then its multi-column uec should be ignored
	// For example: if there is single column statistics for column I1
	// then the MC UEC for any set containing I1 (eg: (I1, I2) ) cannot
	// be considered for Join

	CostScalar uec1;
	CostScalar uec2;

	uec1 = this->lookup( firstId );
	if (uec1.isLessThanZero() )
	{
	  insuffMCInfo = TRUE;
	  // eventhough we are returning t2Cols, it shall not be used
	  return t2Cols;
	}
	else
	{
	  uec2 = this->lookup( secondId );
	  if (uec2.isLessThanZero() )
	  {
	    insuffMCInfo = TRUE;
	    // eventhough we are returning t2Cols, it shall not be used
	    return t2Cols;
	  }
	}

	if ( id == firstId )
	  t2Cols.insert( secondId );
	else
	  t2Cols.insert( firstId );
	inserted = TRUE ;

	maxInitUecProduct *= MAXOF( uec1, uec2 );
	minInitUecProduct *= MINOF( uec1, uec2 );
	break;
      }
    }

    if ( NOT inserted )
      remainingPairs.insert( joinPairs[i] );
  }

  CCMPASSERT( t2Cols.entries() == t1Cols.entries() ); // sanity check

  return t2Cols ;
}

// A help function to break the tie of two table descs. The side with 
// smaller ipFactor, smaller mcUec or larger valueId value of the 
// first column wins.
Int32 tieBreaker(
                 CostScalar ipFactor1, CostScalar mcUec1, TableDesc* tabDesc1,
                 CostScalar ipFactor2, CostScalar mcUec2, TableDesc* tabDesc2
                )
{
  if (ipFactor1 < ipFactor2 )
      return 1;

  if (ipFactor1 > ipFactor2 )
      return -1;

  if (mcUec1 < mcUec2 )
      return 1;

  if (mcUec1 > mcUec2 )
      return  -1;

  if ( tabDesc1 == tabDesc2 )
     return 0;

  if ((CollIndex) ((tabDesc1->getColumnList())[0]) >
      (CollIndex) ((tabDesc2->getColumnList())[0])
     )
    return 1;
  else
    return -1;
}

// -----------------------------------------------------------------------
// MultiColumnUecList::getUecForMCJoin
//
// Given a list of ValueIdLists representing the two (or more) join
// predicates between (hopefully) two tables, return TRUE/FALSE if we
// have the necessary multi-column uec information about some of the
// columns involved in this join.
//
// Return this multi-column uec number (the larger of the two tables's
// multi-column uec for the columns in question), and the join columns
// which weren't used to generate this number.
//
// i.e., if we do
//    "sel * from T1,T2 where T1.a=T2.b AND T1.c=T2.d",
// we need MC-info on (T1.a,T1.c) and (T2.b,T2.d) -- if this exists,
// then return TRUE and set maxMultiColUec to be the larger of the two
// corresponding multi-column uec values for (T1.a,T1.c) & (T2.b, T2.d)
//
// as a more general case, if we do

//    "sel * from T1,T2 where T1.c1=T2.c1 AND T1.c2=T2.c2 AND
// ... T1.cn=T2.cn", then we want to return the largest ValueIdSets
// (t1.1,...,t1.m) (t2.1,...t2.m) such that there is exactly one t2.1
// for every t1.1 -- any remaining ValueIdSets in joinValueIdPairs are
// returned to the calling function, which must then apply
// single-column selectivity for them.
//
// retval :
//
//   TRUE: If we can return MC-info for at least two columns from each
//   table, return TRUE and set the 2nd and 3rd parameters to be the
//   column subsets from the two tables in question.
//
//   FALSE: If we don't have MC-info for at least two columns from each
//   table, then simply return FALSE.
//
// NB: this routine recurses if necessary.  Overall, we're using a greedy
// algorithm to try to cover the join column ValueId's with multi-column
// uec information.  This greedy algorithm might not find the best overall
// answer, but a more general solution will have nasty time-complexity.
// -----------------------------------------------------------------------
NABoolean
MultiColumnUecList::getUecForMCJoin (
  LIST(ValueIdList) & joinValueIdPairs,	    /* in/out */
  const NABoolean     largeTableNeedsStats, /* in  */
  const Join        * expr,
  CostScalar        & prodInitUec,       /* out */
  CostScalar        & multiColUec,	    /* out */
  CostScalar        & baseRCForMCUEC,	 /* out */
  CostScalar        & leftMCUec,	 /* out */
  NABoolean         & checkForLowBound,  /* out */
  NABoolean         & joinOnUnique,      /* out */
  const ColStatDescList & colStats,      
  CostScalar        redFromSC
  )
{

  if ( joinValueIdPairs.entries() < 2 )
    return FALSE;

  // first, we need to divide the ValueId's in joinValueIdPairs into two list
  // of table columns, corresponding to the two tables that are being
  // joined together
  // We use a list, as we want to know exactly which column is being joined
  // to which column of the other table, so we can compare their base UECs 
  // correctly while looking for containment.
  // For other purposes where we can do with a set, we will keep a copy
  // of the set too
  ValueIdList tableOneList, tableTwoList;
  ValueIdSet tableOneSet, tableTwoSet;
  CollIndex i, j;

  // -----------------------------------------------------------------------
  // First, we need to identify whether we have two tables T1, T2
  // such that we're joining two columns (minimum) between these
  // two tables
  //
  // We use an hash-dictionary to implement an association list of
  // <TableDesc*, ValueIdList> pairs -- each ValueIdList indicates
  // the list of columns from this table participating in join
  // -----------------------------------------------------------------------

  NAHashDictionary<TableDesc,ValueIdList> tableColumns (
       &TableDescHashFn,        7, TRUE, HISTHEAP );
  //            hash fn, initsize, uniq, heap)

  // joinValueIdPairs indicate the list of joining pairs
  // example, T1.a = T2.a and T1.b = T2.b. The pairs would be
  // T1.a, T2.a and T1.b, T2.b
  for ( i = 0; i < joinValueIdPairs.entries(); i++ )
  {
    // get the list of joining columns
    const ValueIdList & joinList = joinValueIdPairs[i];

    // for each joining column, get its bast table and 
    // see if this column belongs to the current table we are looking at
    for ( j = 0; j < joinList.entries(); j++ )
    {
      const ValueId & iterId = joinList[j];
      // initially assume ITM_BASETABLE
      const BaseColumn * iterExpr = iterId.castToBaseColumn();
      if ( iterExpr == NULL )
	      return FALSE; // unexpected condition

      TableDesc  * iterDesc = iterExpr->getTableDesc();
      if (iterDesc == NULL)
      {
// LCOV_EXCL_START - rfi
        CCMPASSERT( iterDesc != NULL );
        return FALSE;
// LCOV_EXCL_STOP
      }

      // do a lookup in the hash dictionary we're currently populating
      // to see if an entry for that table already exists
      ValueIdList * colList = tableColumns.getFirstValue( iterDesc );

      if ( colList != NULL )
      {
        // if the entry for that table already exists, add the column to the columnList
        // of this table. This would be true while traversing the second and beyond
        // joining columns of this table
	colList->insert( iterId ); 
      }
      else
      {
        // if this is the first entry for this table in hash dictionary
        // create an entry with the tableDesc and the columnId
	colList = new (HISTHEAP) ValueIdList( );
	colList->insert( iterId );
	tableColumns.insert( iterDesc, colList );
      }
    }
  }

  // -----------------------------------------------------------------------
  // We have lists of references to tableDesc's; now, we iterate through them
  // and make sure that we have two tableDesc's that are referenced a minimum
  // of twice each.
  //
  // If we don't find two tables with two references each, we quit.
  //
  // If there were >2 tables like this, we take the two tables which were
  // referenced the most.  If there are ties, we pick the one with most correlation
  // If the correlation is identical we pick the one with smaller MC UECs
  // -----------------------------------------------------------------------

  NAHashDictionaryIterator<TableDesc,ValueIdList> tableIter( tableColumns );
  TableDesc     * tableDesc = NULL;
  ValueIdList * colList = NULL;
  ValueIdSet colSet;

  CollIndex mostRefs = 0,
	    secondMost = 0;

  ValueIdSet colSetForMostRefs, colSetForSecondMost;
  CostScalar mcUecCurrent = COSTSCALAR_MAX;
  CostScalar mcUecMostRefd = COSTSCALAR_MAX;
  CostScalar mcUecSecondMost = COSTSCALAR_MAX;

  CostScalar ipFactorCurrent = csOne;
  CostScalar ipFactorMostRefd = csOne;
  CostScalar ipFactorSecondMost = csOne;
  // Keep a flag to use later to differentiate the conditions that MC stats
  // cannot be used because they are not needed or because the stats are missing
  NABoolean mcStatsPresent = TRUE;

  // the table with the most refs is tableOne; the table with the 2nd most
  // refs is tableTwo (not that this matters ... :-)

  TableDesc * tableOneDesc = NULL;
  TableDesc * tableTwoDesc = NULL;

  for ( tableIter.getNext( tableDesc, colList );
        tableDesc != NULL && colList != NULL;
	tableIter.getNext( tableDesc, colList ) )
  {
    colSet = (ValueIdSet) *colList;
    if (colSet.entries() < 2 ) 
    {
      // This table joined on only one column, hence cannot be
      // used for MC adjustment
      continue;
    }

    mcStatsPresent = TRUE; // assume mc stats is available for this colSet

    mcUecCurrent = lookup(colSet);
    // if the columns are unique use basetable rowcount as the MC UEC
    NABoolean colsUnique = FALSE;
    if (mcUecCurrent == csMinusOne)
    {
      // MC stats do not exist for the joining columns
      // check to see if it is unique and if we can use rowcount as the MCUEC
      colsUnique = colSet.doColumnsConstituteUniqueIndex(tableDesc);
      if (colsUnique)
      {
        // if colSet == unique index, the MC stats for that would have been added
        // much earlier at the time of fetching stats. This is for cases when
        // colSet is a super set of unique index.
	// get base rowcount of the table from original colStats
	// and set the multi-col UEC equal to this rowcount
        // save it for later use too
	mcUecCurrent = tableDesc->getNATable()->getOriginalRowCount();
	this->insertPair(colSet, mcUecCurrent);
      }
      else
      {
        // Since the stats are missing, use just the colSet entries to make a decision
        // We will give the warning regarding missing stats later, based on other
        // heuristics
        mcStatsPresent = FALSE;
      }
    }

    CostScalar SCproductUec = csOne;

    // compute correlation factor for this set. Smaller the ipFactor
    // larger the correlation. ipFactor =1 means there
    // is no correlation, or the columns are unique. 
    // If there is a conflict we pick the one with larger correlation
    if (!colsUnique && mcStatsPresent)
    {
      for (ValueId keyCol = colSet.init();
			    colSet.next(keyCol);
			    colSet.advance(keyCol) )
	  SCproductUec *= lookup(keyCol).value();

      ipFactorCurrent = mcUecCurrent/SCproductUec;
    }
    else
      ipFactorCurrent = csOne;

    // For choosing the most referenced tables, pick the one joining on larger number
    // of columns. If there are more than one tables joining on same number of columns
    // pick the one with larger correlation (smaller ipFactor). If there are more than
    // one table with same ipFactor and same number of joining columns, pick the one with
    // smaller MCUEC. 
    // if MC Stats are missing just use number of columns as the guiding factor in
    // deciding which tables to choose for MC adjustment.
    // We are doing this here as later we use heuristics to decide whether MC stats 
    // warning is needed or not. We also use heuristic later to construct MC stats
    // using smaller subset of columns. Hence missing stats at this point does not mean
    // that we are done.
    if (CmpCommon::getDefault(COMP_BOOL_41) == DF_ON)
       mcStatsPresent = FALSE;

    if ( ( colSet.entries() > mostRefs )
       ||
         (
          (mcStatsPresent && colSet.entries() == mostRefs) &&
          (tableOneDesc == NULL ||
           tieBreaker(ipFactorCurrent, mcUecCurrent,tableDesc,
                      ipFactorMostRefd, mcUecMostRefd, tableOneDesc) == 1)
         )
       )
    {
      secondMost = mostRefs;
      colSetForSecondMost = colSetForMostRefs;
      mcUecSecondMost = mcUecMostRefd;
      tableTwoDesc = tableOneDesc;
      ipFactorSecondMost = ipFactorMostRefd;

      mostRefs = colSet.entries();
      colSetForMostRefs = colSet;
      mcUecMostRefd = mcUecCurrent;
      tableOneDesc = tableDesc;
      ipFactorMostRefd = ipFactorCurrent;
    }
    else if ( ( colSet.entries() > secondMost )
            ||
            ((mcStatsPresent && colSet.entries() == secondMost) &&
             ( tableTwoDesc == NULL ||
               tieBreaker(ipFactorCurrent, mcUecCurrent,tableDesc,
                          ipFactorSecondMost, mcUecSecondMost, tableTwoDesc) ==
1)
             )
            )
    {
      secondMost = colSet.entries();
      colSetForSecondMost = colSet;
      mcUecSecondMost = mcUecCurrent;
      tableTwoDesc = tableDesc;
      ipFactorSecondMost = ipFactorCurrent;
    }
  }

  // If there was only one table with MC stats, then there is nothing we can do
  // return. 
  if ( secondMost == 0 )
    return FALSE; 

  // did that go alright? check to make sure:
  // It would not do any major harm
  // even if the tableDescs are null, because later these are used for
  // for finding the columns which belong to the tableDescs, and if these
  // are NULL, then that means that no columns can be used for MC estimates
  // which should not be fatal enough to cause the crash.
  if ((tableOneDesc == NULL) || (tableTwoDesc == NULL ))
  {
    CCMPASSERT( tableOneDesc != NULL && tableTwoDesc != NULL );
    return FALSE;
  }

  // now iterate again and grab the tables which had the mostRefs & secondMost
  // most and second most referenced tables have their table descriptors saved
  // in tableOneDesc and tableTwoDesc
  tableIter.reset();

  ValueIdSet joinsUsed;

  for ( tableIter.getNext( tableDesc, colList );
        tableDesc != NULL && colList != NULL;
	tableIter.getNext( tableDesc, colList ) )
  {
    if ( ( tableDesc == tableOneDesc) || ( tableDesc == tableTwoDesc) )
    {
	joinsUsed.insert((ValueIdSet) * colList);
    }
  }

  if ( joinsUsed.entries() < 2 ) // something weird, but legal (e.g., T1.a=T1.b=T2.a=T2.b)
    return FALSE; // cannot handle this case, punt!

  // -----------------------------------------------------------------------
  // Now we need to create a "clean" list of joinValueIdPairs (to simplify
  // future processing), removing any column references that do not refer
  // to our two TableDesc*'s.  Initially, we just grab the subset of the
  // joinValueIdPairs that referenced the two tables.
  //
  // Any unusable entries in joinValueIdPairs are stored in a new list
  // (joinPairsNotUsed), which we'll add to our "unused join preds" retval
  // before returning from the function.
  // -----------------------------------------------------------------------

  // joinPairsUsed + joinPairsNotUsed = joinValueIdPairs
  NAList<ValueIdList> joinPairsUsed(CmpCommon::statementHeap()),
                      joinPairsNotUsed(CmpCommon::statementHeap());

  for ( i = 0; i < joinValueIdPairs.entries(); i++ )
  {
    ValueIdSet joiningCols = joinsUsed;
    ValueIdSet joinColPair = joinValueIdPairs[i];

    if ( joiningCols.intersectSet( joinColPair ).entries() >= 2 )
      joinPairsUsed.insert( joinValueIdPairs[i] );
    else
      joinPairsNotUsed.insert( joinValueIdPairs[i] );
  }

  // -----------------------------------------------------------------------
  // Now remove table references from joinPairsUsed that don't match
  // either tableOneDesc or tableTwoDesc.  Note that this step is
  // potentially buggy; if we're doing something like
  //
  //    {T1.a=T2.a=T3.a,T1.b=T2.b=T3.b, T1.c=T2.c}
  //
  // then calculating the resulting rowcount by just looking at
  //
  //    {T1.a=T2.a,T1.b=T2.b,T1.c=T2.c}
  //
  // is inexact -- if we were being really careful (which would require a
  // *ton* more checking, which isn't warranted due to the little we'd
  // gain), then we wouldn't throw away the reference to T3.  But that's
  // what we're doing right now.  So there.
  //
  // We are trying to handle the case where we do the following
  // "transformation":
  //
  //   {T1.a=T2.a=T3.a, T1.b=T4.b, T1.c=T2.c=T5.c} ==>
  //   {T1.a=T2.a, T1.c=T2.c}
  //
  // This happens in TPC-D, for example.
  //
  // NB: If we have a join of the form :
  //
  //   {T1.a=T1.b=T2.a}
  //
  // We punt on that one -- put it into joinPairsNotUsed.
  // -----------------------------------------------------------------------

  ValueIdList empty;

  // Go thru all pairs of joining columns
  for ( i = 0; i < joinPairsUsed.entries(); i++ )
  {
    NABoolean tableOneRefd = FALSE,
	      tableTwoRefd = FALSE;
    const ValueIdList & joinList = joinPairsUsed[i];
    ValueIdSet joinSet( joinList ); // convenience : list->set

    // For each pair get all columns. Example if T1.a = T2.a = T3.a
    // then the joinlist will contain, T1.a, T2.a, T3.a
    // From this list keep only those columns which have been chosen
    // to do MC adjustment. These columns are referred by tableOneDesc
    // and tableTwoDesc
    for ( j = 0; j < joinList.entries(); j++ )
    {
      const ValueId & iterId = joinList[j];
      // initially assume ITM_BASETABLE
      const BaseColumn * iterExpr = iterId.castToBaseColumn();
      if ( iterExpr == NULL )
	      return FALSE ; // unexpected condition

      const TableDesc  * iterDesc = iterExpr->getTableDesc();
      if (iterDesc == NULL)
      {
// LCOV_EXCL_START - rfi
        CCMPASSERT( iterDesc != NULL );
        return FALSE;
// LCOV_EXCL_STOP
      }

      if ( iterDesc == tableOneDesc )
	      tableOneRefd = TRUE;
      else if ( iterDesc == tableTwoDesc )
	      tableTwoRefd = TRUE;
      else
	      joinSet.remove( iterId );
    }
    // make sure that each table was ref'd exactly once (i.e., both
    // bools are TRUE and there are only two entries left)
    if ( NOT ( tableOneRefd AND tableTwoRefd AND joinSet.entries() == 2) )
    {
      joinPairsNotUsed.insert( joinPairsUsed[i] );
      joinPairsUsed[i] = empty;
    }
    else
    {
      ValueIdList tmpPair = joinSet;
      joinPairsUsed[i] = tmpPair;
    }
  }

  // -----------------------------------------------------------------------
  // Now filter out all of the empty lists inside joinPairsUsed -- to make
  // the previous loop's logic simpler, we put off dealing with the "no
  // auto increment" until now.
  // -----------------------------------------------------------------------
  for ( i = 0; i < joinPairsUsed.entries(); /* no auto increment */ )
    if ( joinPairsUsed[i].entries() == 0 )
      joinPairsUsed.removeAt( i );
    else
      i++ ;

  // -----------------------------------------------------------------------
  // Now we iterate through the "cleaned-up" list of joinPairs, and collect
  // the ValueId's of joining columns from each table for which MC will be used
  //
  // First, we check again : if joinPairsUsed doesn't have at least two
  // entries, then we quit.
  // -----------------------------------------------------------------------

  if ( joinPairsUsed.entries() < 2 )
    return FALSE;

  for ( i = 0; i < joinPairsUsed.entries(); i++ )
  {
    const ValueIdList & joinList = joinPairsUsed[i];

    // if any column has !=2 columns, then something is very wrong in
    // the previous logic, and we want to know about it!
    // Not from the customer though. If the entries are not equal
    // to 2, then that means we shall not be able to use MC UEC
    if (joinList.entries() != 2 )
    {
// LCOV_EXCL_START - rfi
      CCMPASSERT( joinList.entries() == 2 );
      return FALSE;
// LCOV_EXCL_STOP
    }

    for ( j = 0; j < joinList.entries(); j++ )
    {
      const ValueId & iterId = joinList[j];
      // initially assume ITM_BASETABLE
      const BaseColumn * iterExpr = iterId.castToBaseColumn();
      if ( iterExpr == NULL )
	      return FALSE ; // unexpected condition

      const TableDesc  * iterDesc = iterExpr->getTableDesc();
      if (iterDesc == NULL)
      {
// LCOV_EXCL_START - rfi
        CCMPASSERT( iterDesc != NULL );
        return FALSE;
// LCOV_EXCL_STOP
      }

      if ( iterDesc == tableOneDesc )
      {
	      tableOneSet.insert( iterId );
              tableOneList.insert(iterId);
      }
      else if ( iterDesc == tableTwoDesc )
      {
	      tableTwoSet.insert( iterId );
              tableTwoList.insert(iterId);
      }
      else
      {
// LCOV_EXCL_START - rfi
	      // it's a table column not associated with either of
	      // the two tables we're joining -- however, we already
	      // tried to remove all of these references!  abort!
	      // If there is a table column not associated with any of the two tables
	      // then in the worst case we shall not be able to use MC stats, but
	      // it should not be a work stoppage
	      CCMPASSERT( FALSE );
	      return FALSE ;
// LCOV_EXCL_STOP
      }
    } // j-loop
  } // i-loop

  // for whatever reason, we don't have the situation where there are
  // at least two candidate columns from each of two tables; abort
  if ( tableOneSet.entries() < 2 OR tableTwoSet.entries() < 2)
    return FALSE;

  // See if the join is being done on non-key columns. If it is then
  // we don't want to use MC uec. This is because through MC stats we are
  // not able to correctly estimate the containment of non-unique columns
  // into the other. For Example:
  // T1(a,b,c,d,e ...) key is a,b,c rowcount = 1Mil
  // T2(a,b,c,d,e ) key is a,b,c rowcount = 1Mil

  // Uec(1.a,1.b) = 1000
  // Uec(2.a,2.b) = 10000

  // Using MC we force the join estimate to be 1M x 1M / 10000
  // Here we assumed that the 1000 values of 1.a,1.b are subset
  // of the 10000 set of 2.a,2.b. This is usually not correct
  // and results in overestimation of the result of the cardinality.
  // This would have been most likely correct if 2.a,2.b was the
  // key for T2 as this would be foreign key join, so we allow that exception.

  // CQD HIST_SKIP_MC_FOR_NONKEY_JOIN_COLUMNS is used to control
  // this behaviour. 'OFF' for this CQD means we use MC stats even
  // for joins on non-key columns.  08/24/04

  NABoolean tableOneSetUnique = tableOneSet.doColumnsConstituteUniqueIndex(tableOneDesc);
  NABoolean tableTwoSetUnique = tableTwoSet.doColumnsConstituteUniqueIndex(tableTwoDesc);

  if (CURRSTMT_OPTDEFAULTS->histSkipMCUecForNonKeyCols()  )
  {
    // Check if the primary key or a unique index of either table is fully covered
    // by the columns on which this table is being joined. If none of the tables
    // are being joined on primary key, return
    if ( !( tableOneSetUnique || tableTwoSetUnique ) )
      return FALSE;
  }

  if (tableOneSet.entries() != tableTwoSet.entries() )
  {
    // The number of columns being joined from two tables
    // should be equal, if not, we cannot use multi-column UEC
    CCMPASSERT( tableOneSet.entries() == tableTwoSet.entries() );
    return FALSE;
  }

  NABoolean displayWarning = TRUE;

  if ( (CmpCommon::getDefault(HIST_MC_STATS_NEEDED) == DF_ON)
	AND largeTableNeedsStats )
  {
    NAString tableOneName(CmpCommon::statementHeap()),
             tableTwoName(CmpCommon::statementHeap()); // table names

    NAString tableOneCols(CmpCommon::statementHeap()),
             tableTwoCols(CmpCommon::statementHeap()); // column names

    if ((this->lookup( tableOneSet )).isLessThanZero() )
    {
      // There is no multi-col uec list for tableOneSet. If tableOneSet is unique
      // fake multi-col uec, by setting its UEC equal to rowcount
      if (tableOneSetUnique)
      {
	// get base rowcount of the table from original colStats
	// and set the multi-col UEC equal to this rowcount
	CostScalar baseRowCount = tableOneDesc->getNATable()->getOriginalRowCount();
	this->insertPair(tableOneSet, baseRowCount);
      }
      else
      {
	if (( tableTwoSetUnique ) || (!CURRSTMT_OPTDEFAULTS->histSkipMCUecForNonKeyCols()) )
	{
	  // But it is being joined to unique columns so display missing stats warning
	  // if the warning could be useful. The warning level should be 
          // greate than or equal to 3 for the MC warnings from Join to be displayed
          if ( (CURRSTMT_OPTDEFAULTS->histMissingStatsWarningLevel() > 3) &&
               (isMCStatsUseful(tableOneSet, tableOneDesc)))
            displayWarning = TRUE;
          else 
            displayWarning = FALSE;
          
          displayMissingStatsWarning(tableOneDesc,
                                    tableOneSet,
                                    largeTableNeedsStats,
                                    displayWarning,
                                    colStats,
                                    redFromSC,
                                   FALSE,
                                    REL_JOIN);
	}
      }
    }

    //cs if ( this->lookup( tableTwoSet ) < csZero ) // indicating it's not present
    if ((this->lookup( tableTwoSet )).isLessThanZero() )
    {
      // There is no multi-col uec list for tableTwoSet. If it is unique
      // so we fake it with the rowcount, else display missing stats warning.
      if (tableTwoSetUnique )
      {
	// get base rowcount of the table from original colStats
	// and set the multi-col UEC equal to this rowcount
	CostScalar baseRowCount = tableTwoDesc->getNATable()->getOriginalRowCount();
	this->insertPair(tableTwoSet, baseRowCount);
      }
      else
      {
	if (( tableOneSetUnique ) || (!CURRSTMT_OPTDEFAULTS->histSkipMCUecForNonKeyCols()) )
	{
          // But it is being joined to unique columns so display missing stats warning
          // if the warning could be useful. The warning level should be greater
          // than or equal to 3 for the MC warnings from Join to be displayed
          if ( (CURRSTMT_OPTDEFAULTS->histMissingStatsWarningLevel() > 3) &&
                (isMCStatsUseful(tableTwoSet, tableTwoDesc)))
            displayWarning = TRUE;
          else 
            displayWarning = FALSE;

          displayMissingStatsWarning(tableTwoDesc,
		              tableTwoSet,
		              largeTableNeedsStats,
                              displayWarning,
                              colStats,
                              redFromSC,
                              FALSE,
                              REL_JOIN);

	}
      }
    }

  } // no WARNING MULTI_COLUMN_STATS_NEEDED's currently in diags

  // MC adjustment for semi_joins is done little differently that inner joins.
  // For semi_joins we use MIN of MC_UEC instead of MAX MC_UEC. 
  // We will not do any short cut MC on unique column set
  NABoolean isNotASemiJoin = ((expr && !expr->isSemiJoin() && !expr->isAntiSemiJoin()) ? TRUE : FALSE);

  // if any of the sides was unique, and the MCUEC of the second is contained in the 
  // unique side, use row count of non-unique side as the join cardinality. 
  // Else determine the largest MCUEC and use that to compute adjustment
  // for MC UEC
  CostScalar rowcountOfNonUniqSide = csMinusOne;
  if ((CmpCommon::getDefault(COMP_BOOL_149) == DF_ON) && 
      isNotASemiJoin &&
      (tableOneSetUnique || tableTwoSetUnique) )
  {

    rowcountOfNonUniqSide = getRowcountOfNonUniqueColSet(expr, tableOneList, tableTwoList, 
                                                      tableOneSetUnique, tableTwoSetUnique);

    if (rowcountOfNonUniqSide > 0)
    {
      if (CmpCommon::getDefault(COMP_BOOL_147) == DF_OFF)
        checkForLowBound = FALSE;
      else
        checkForLowBound = TRUE;

      // This is a misnomer - multiColUec will be storing row count of non-unique side
      // instead of MC UEC.
      multiColUec = rowcountOfNonUniqSide;
      joinOnUnique = TRUE;
      // joinValueIdPairs as an output parameter contains the join pairs that were
      // not used for MCUEC. Since we used all columns for uniqueness, we will clear
      // this parameter.
      joinValueIdPairs.clear();
      return TRUE;
    }
  }

  // The joining set is not unique. Compute adjustment for MC UECs

  // Now the join column ValueIds are partitioned into two sets.  Let's
  // see whether we can find MC-info corresponding to (the largest
  // possible) subsets of these sets such that one subset is being joined
  // to the other (i.e., both sets together add up to a subset of the
  // entries in joinPairsUsed)

  // Notice, this is a fairly complicated operation!  It seems that the
  // best way to do this is iterate through what MC-info we have -- note
  // that if we don't have actual multi-column information, then we
  // shouldn't have gotten this far.

  ValueIdSet * keyEntry = NULL;
  CostScalar * uecEntry = NULL;
  MultiColumnUecListIterator iter( *this );

  LIST(ValueIdSet) tableOnePossibles(STMTHEAP); // we'll iter over these
  SET(ValueIdSet)  tableTwoPossibles(STMTHEAP); // we'll do lookups over these

  for ( iter.getNext( keyEntry, uecEntry );
        keyEntry != NULL && uecEntry != NULL;
        iter.getNext( keyEntry, uecEntry ) )
  {
    // want to consider only those entries containing MC-info
    if ( keyEntry->entries() < 2 ) continue;

    // we could check to make sure they're referencing the same table
    // desc, as above, but the check below should be faster and
    // more-than-sufficient for our needs
    if ( tableOneSet.contains( *keyEntry ) )
      tableOnePossibles.insert( *keyEntry );
    else if ( tableTwoSet.contains( *keyEntry ) )
      tableTwoPossibles.insert( *keyEntry );
  }

  if ( tableOnePossibles.entries() == 0 OR
       tableTwoPossibles.entries() == 0 )
  {
    // we are returning because of insufficient multi-column
    // information for this table
    checkForLowBound = TRUE;
    return FALSE;
  }

  // OK -- now we have two lists of possibly useful MC-info that we might
  // be able to use; now we need to check to see whether any pair (x,y)
  // between these two lists actually satisfies the requirement that the
  // columns in x are being joined to the columns in y

  // Plan : iterate over the entries in tableOnePossibles
  //
  // For each entry in tableOnePossibles, we determine, from the joinPairsUsed,
  // what multi-column uec information we need to have in tableTwoPossibles.
  //
  // If that entry does not exist in tableTwoPossibles, we remove the current entry
  // in tableOnePossibles and continue.
  //
  // When we're done, whatever remains in tableOnePossibles is usable.
  // Then it's a matter of determining which entry in tableOnePossibles is
  // best; after we do this, we need to remove some entries from
  // joinValueIdPairs (the ones that correspond to the best entry in
  // tableOnePossibles) and set the value of maxMultiColUec to be the
  // larger of the two tables' multi-column uec.

  NAList<MCJoinPairStruct *> corrList(CmpCommon::statementHeap()); // for "correspondence list"
  NAList<ValueIdList> remainingPairs(CmpCommon::statementHeap()); // tmp var
  CostScalar maxInitUecProduct = csOne;
  CostScalar minInitUecProduct = csOne;

  for ( i = 0; i < tableOnePossibles.entries(); i++ )
  {
    maxInitUecProduct = csOne;
    minInitUecProduct = csOne;

    // need to clear the entries for this call to findMatchingColumns()
    remainingPairs.clear();

    ValueIdSet needInTableTwo =
      this->findMatchingColumns (
	tableOnePossibles[i],
	joinPairsUsed,
	remainingPairs,
	maxInitUecProduct,
        minInitUecProduct,
	checkForLowBound
	);

    // if checkForLowBound was returned from findMatchingColumns as
    // no statistics existed for some column which was a part of Multi-column
    // set, then return FALSE

    if (checkForLowBound )
      return FALSE;

    // if the table 2 MC-info doesn't exist, continue
    if ( NOT tableTwoPossibles.contains( needInTableTwo ) )
      continue;

    MCJoinPairStruct *entry = new (CmpCommon::statementHeap()) MCJoinPairStruct();
    // Start setting the entries for MC computation. 
    // Set the columns from the two tables for which MC UEC exist
    entry->tableOneCols_       = tableOnePossibles[i];
    entry->tableTwoCols_       = needInTableTwo;
    if (isNotASemiJoin)
      entry->prodInitUec_        = maxInitUecProduct;
    else
      entry->prodInitUec_        = minInitUecProduct;

    CostScalar mcUEC1 = this->lookup (entry->tableOneCols_);
    CostScalar mcUEC2 = this->lookup (entry->tableTwoCols_);
 
    if (expr && !isNotASemiJoin)
    {
      // if it is a semi_join, save the left MCUEC
      ColStatDescList leftColStatsList = ((Join *)expr)->child(0).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->colStats();
      if (leftColStatsList.contains(entry->tableOneCols_))
        entry->leftMCUec_ = mcUEC1;
      else
        entry->leftMCUec_ = mcUEC2;
    }

    // now set the max MC UEC.
    if(CmpCommon::getDefault(COMP_BOOL_141) == DF_OFF)
    {
      // If COMP_BOOL_141 is OFF, set the maxMCUEC from the 
      // base table, and the product of max init UEC as it exists in the base table
      // if it is a semi_join, use min UECs instead of max
      // This is the original behaviour from denali days
      if ( mcUEC1 > mcUEC2)
      {
        if (isNotASemiJoin)
        {
	  entry->multiColUec_ = mcUEC1;
	  entry->baseRowCount_ = tableOneDesc->getNATable()->getOriginalRowCount();
        }
        else
        {
	  entry->multiColUec_ = mcUEC2;
	  entry->baseRowCount_ = tableTwoDesc->getNATable()->getOriginalRowCount();
        }
      }
      else
      {
        if (isNotASemiJoin)
        {
	  entry->multiColUec_ = mcUEC2;
	  entry->baseRowCount_ = tableTwoDesc->getNATable()->getOriginalRowCount();
        }
        else
        {
	  entry->multiColUec_ = mcUEC1;
	  entry->baseRowCount_ = tableOneDesc->getNATable()->getOriginalRowCount();
        }
      }
    }
    else
    {
      // If COMP_BOOL_141 is ON, set the maxMCUEC after applying reduction from local predicates,
      // and the product of max init UEC maxed out to the base row count of the table for 
      // which the MC UEC is being picked up for all except semi joins
      // for semi joins take the minimum uec
      CostScalar baseRowCountForTableOne = tableOneDesc->getNATable()->getOriginalRowCount();
      CostScalar baseRowCountForTableTwo = tableTwoDesc->getNATable()->getOriginalRowCount();
      
      // get the left and the right children of Join and compute highest reduction from local predicates 
      // for the set of joining columns
      if (expr && expr->isAnyJoin())
      {
        CostScalar highestUecRedByLocalPreds1 = ((Join *)expr)->highestReductionForCols(entry->tableOneCols_);
        CostScalar highestUecRedByLocalPreds2 = ((Join *)expr)->highestReductionForCols(entry->tableTwoCols_);

        // There is no way, the reduction should go beyond 1
        if (highestUecRedByLocalPreds1 > csOne)
        {
// LCOV_EXCL_START - rfi
          CCMPASSERT ("Reduction from local predicates is greater than 1");
          highestUecRedByLocalPreds1 = csOne;
// LCOV_EXCL_STOP
        }
        if (highestUecRedByLocalPreds2 > csOne)
        {
// LCOV_EXCL_START - rfi
          CCMPASSERT ("Reduction from local predicates is greater than 1");
          highestUecRedByLocalPreds2 = csOne;
// LCOV_EXCL_STOP
        }
        mcUEC1 = (mcUEC1 * highestUecRedByLocalPreds1).minCsOne();
        mcUEC2 = (mcUEC2 * highestUecRedByLocalPreds2).minCsOne();
      }

      // Now use the larger of the reduced MC UEC to do cardinality adjustments
      if ( ( mcUEC1 > mcUEC2) || ( (mcUEC1 == mcUEC2) &&
	(baseRowCountForTableOne > baseRowCountForTableTwo) ) )
      {
        if (isNotASemiJoin)
        {
	  entry->multiColUec_ = mcUEC1;
	  entry->baseRowCount_ = baseRowCountForTableOne;
        }
        else
        {
	  entry->multiColUec_ = mcUEC2;
	  entry->baseRowCount_ = baseRowCountForTableTwo;
        }
      }
      else
      {
        if (isNotASemiJoin)
        {
	  entry->multiColUec_ = mcUEC2;
	  entry->baseRowCount_ = baseRowCountForTableTwo;
        }
        else
        {
	  entry->multiColUec_ = mcUEC1;
	  entry->baseRowCount_ = baseRowCountForTableOne;
        }
      }
   }// end COMP_BOOL_141 = ON

    // -----------------------------------------------------------
    // want to write: entry.remainingJoinPairs_ = remainingPairs ;
    entry->remainingJoinPairs_->clear() ; // probably unnecessary
    for ( j = 0; j < remainingPairs.entries(); j++ )
      entry->remainingJoinPairs_->insert( remainingPairs[j] );
    // -----------------------------------------------------------

    if ( entry->multiColUec_.isGreaterThanZero() ) // sanity check -- uec<=0 is bad ...
      corrList.insert( entry );

    if ( entry->remainingJoinPairs_->entries() == 0 )
      break; // these columns cover all joins columns -- best we can do -- stop now
  }

  // if the corresponding multi-column uec information does not exist in table
  // two, then we cannot do anything, abort
  if ( corrList.entries() == 0 )
  {
    // we are returning because of insufficient multi-column
    // information for table 2
    checkForLowBound = TRUE;
    return FALSE;
  }

  // Now we have a list of possible return information; find out which entry is the best

  CollIndex bestIdx = 0; // index of best entry so far
  for ( i = 1; i < corrList.entries(); i++ )
  {
    if ( corrList[   i   ]->remainingJoinPairs_->entries() <
	 corrList[bestIdx]->remainingJoinPairs_->entries() )
      bestIdx = i;
  }

  // -----------------------------------------------------------
  // Until now, we have not changed the values of the function parameters --
  // now we set these values and prepare to return
  //
  // Don't forget to add back those joinPairs that didn't reference either
  // tableOne or tableTwo!
  // -----------------------------------------------------------

  // -----------------------------------------------------------
  // if we had a better C++ interface, we'd write the following five lines as:
  //    joinValueIdPairs = *(corrList[bestIdx].remainingJoinPairs_) + joinPairsUnused ;
  // ----------------------------------------------------------
  LIST(ValueIdList) oldJoinValueIdPairs(joinValueIdPairs,
                                        CmpCommon::statementHeap());
  joinValueIdPairs.clear();

  SET(ValueIdSet) joinValueIdPairSet(CmpCommon::statementHeap());
  ValueIdSet vidset;

  for ( i = 0; i < corrList[bestIdx]->remainingJoinPairs_->entries(); i++ )
  {
    vidset = corrList[bestIdx]->remainingJoinPairs_->at(i);
    joinValueIdPairSet.insert( vidset );
  }

  for ( i = 0; i < joinPairsNotUsed.entries(); i++ )
  {
    vidset = joinPairsNotUsed[i];
    joinValueIdPairSet.insert( vidset );
  }

  for (i=0;
       i<joinValueIdPairSet.entries();
       i++)
  {
     vidset = joinValueIdPairSet.at(i);
     ValueIdList vlist = vidset;
     joinValueIdPairs.insert(vlist);
  }

  prodInitUec    = corrList[bestIdx]->prodInitUec_; // #retvar
  multiColUec    = corrList[bestIdx]->multiColUec_; // #retvar
  baseRCForMCUEC = corrList[bestIdx]->baseRowCount_; // #retvar
  leftMCUec      = corrList[bestIdx]->leftMCUec_;

  // ------------------------------------------------------------------
  // if there are two or more join predicates that are uncovered by our
  // work above, then it might be possible to find another MC-uec value
  // for these remaining values -- recurse and see ...
  // only make the recursive call if there is a change in
  // joinValueIdPairs
  // ------------------------------------------------------------------
  if ( oldJoinValueIdPairs == joinValueIdPairs )
  {
  }
  else if ( joinValueIdPairs.entries() >= 2 )
  {
    // $$$ for the short-term, we'll recurse; later on, if this
    // $$$ appears to be a performance hit, we can avoid redoing
    // $$$ a lot of the same work

    CostScalar moreMultiColUec = csMinusOne,
	       moreInitUec  = csMinusOne,
               moreLeftMCUec = csOne;
    NABoolean insuffMCInfo = FALSE;
    CostScalar newBaseRowCount = csOne;

    if ( this->getUecForMCJoin( joinValueIdPairs,
				largeTableNeedsStats,
				expr,
				moreInitUec,
				moreMultiColUec,
				newBaseRowCount,
                                moreLeftMCUec,
				insuffMCInfo,
                                joinOnUnique,
                                colStats,
                                csMinusOne) )
    {
      CCMPASSERT( moreMultiColUec.isGreaterThanZero() );	// sanity check
      prodInitUec = (prodInitUec * moreInitUec).minCsOne();	  // multiply the maxInit-uec's together
      multiColUec = (moreMultiColUec * multiColUec).minCsOne();  // multiply the MC-uec's together
      leftMCUec = (moreLeftMCUec * leftMCUec).minCsOne();
      baseRCForMCUEC = newBaseRowCount.minCsOne();
    }
  }

  // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
  // $$$ One definite problem with the above code (among many, perhaps) :
  // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
  // $$$ if we do something like
  // $$$
  // $$$   {T1.a=T2.a=T3.a, T1.b=T2.b, T1.c=T2.c}
  // $$$
  // $$$ reducing it to
  // $$$
  // $$$   {T1.a=T2.a, T1.b=T2.b, T1.c=T2.c}
  // $$$
  // $$$ but only have multicolumn uec information for
  // $$$
  // $$$   <T1.b,T1.c>, <T2.b,T2.c>
  // $$$
  // $$$ then the first join, as returned from this function
  // $$$ to the calling fn, is <T1.a,T2.a> (we've removed the
  // $$$ reference to table T3!).  This won't cause real
  // $$$ problems, but it's a loss of information.
  // $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
  if (CmpCommon::getDefault(COMP_BOOL_147) == DF_OFF)
    checkForLowBound = FALSE;
  else
    checkForLowBound = TRUE;

  CCMPASSERT( prodInitUec.isGreaterThanZero() );        // sanity check

  return TRUE;
}

// -----------------------------------------------------------------------
// MultiColumnUecList::containsMCinfo
//
// It is very possible for the MultiColumnUecList to contain only
// single-column entries.  Most of the usage of this class is predicated
// on it actually containing multi-column information in this list -- if
// there isn't, then very often there's no point in continuing the current
// operation.
//
// This routine answers the question of whether any TRUE multi-column
// information exists in the MultiColumnUecList.
// -----------------------------------------------------------------------
NABoolean
MultiColumnUecList::containsMCinfo () const
{
  ValueIdSet * keyEntry = NULL;
  CostScalar * uecEntry = NULL;
  MultiColumnUecListIterator iter( *this );

  for ( iter.getNext( keyEntry, uecEntry );
        keyEntry != NULL && uecEntry != NULL;
        iter.getNext( keyEntry, uecEntry ) )
    {
      if ( keyEntry->entries() > 1 )
        return TRUE;
    }
  return FALSE;
}

// Following method creates multi-col UEC for larger set of columns
// using partial overlapping multi-col UECs.
// For example, if MC-UEC available - (a, b, c) (c, d).
// Then MC (a, b, c, d) = MC (a, b, c) * MC (c, d) / MC (c)
// returns TRUE or FALSE depending on whether it was able to create
// any statistics from partial multi-col UECs
NABoolean MultiColumnUecList::createMCUECWithOverlappingColSets(
					      ValueIdSet & remainingCols, /* in / out */
					      ValueIdSet & cumulativeColSetWithMCUEC, /* in / out */
					      CostScalar & mcUec, /* in / out */
					      CostScalar oldRowCount)
{
  NABoolean statsCreated = FALSE;

  // There can be several levels of overlapping. Example there can be one
  // column overlapping, or two columns overlapping and so on. Process of
  // trying all overlapping combinations can be very expensive, and might
  // not be very cost effective. Hence we shall look for only first level
  // of overlapping, in other words look for only those partial multi-col
  // UECs in which we have one column overlapping
  CollIndex noOfColsInterested = remainingCols.entries() + 1;

  while (noOfColsInterested > 1)
  {

    // traverse through all the entries of the multiColUecList for this table
    ValueIdSet * keyEntry = NULL ;
    CostScalar * uecEntry = NULL ;
    ValueIdSet overlappingCol;
    MultiColumnUecListIterator iter (*this) ;

    Int32 i ;
    double smallestIPFactor = 1;
    ValueIdSet * setWithMinIPFactor = NULL;

    for ( iter.getNext (keyEntry, uecEntry), i = 1 ;
	  keyEntry != NULL && uecEntry != NULL ;
	  iter.getNext (keyEntry, uecEntry), i++ )
    {
      ValueIdSet cols;
      cols = *keyEntry;
      if (keyEntry->entries() != noOfColsInterested)
	continue;

      // there is no point in looking forward if the number of entries are different

      // it could be interesting set, if it contains all remaining columns, and
      // one column from the largestSubset

      cols.subtractSet(remainingCols);
      if (cols.entries() != 1)
      {
	// contains no or more than one overlapping columns
	// so not interested
	continue;
      }
      if (!(cumulativeColSetWithMCUEC.contains(cols)) )
      {
	// does not contain overlapping columns, so continue to look for more
	continue;
      }
      CostScalar keyUec = lookup(*keyEntry);

      double ipFactor;
      double SCproductUec = 1.0;

      for (ValueId keyCol = keyEntry->init();
			    keyEntry->next(keyCol);
			    keyEntry->advance(keyCol))
	  SCproductUec *= lookup(keyCol).value();

      ipFactor = keyUec.value()/SCproductUec;

      if (ipFactor <= smallestIPFactor)
      {
	// found the overlapping column.
	// There could be more entries with overlapping columns, so pick the
	// one with minimum correlation factor. Example, MC UEC available -
	// (x, y), (x, z), (y, z). To create (x, y, z), we could have
	// (x, y) + (x, z) OR (x, y) + (y, z) OR (x, z) + (y, z)
	// From the three, we shall pick the one which for which
	// (multi-column UEC / product of single column UECs) is smallest
	setWithMinIPFactor = keyEntry;
	overlappingCol = cols;
	smallestIPFactor = ipFactor;
      }
    }// for all entries()
    // see if we can get any more smaller partial multi-col UECs

    // For example, we were looking for a UEC with three columns, and we could
    // not find any (setWithMinIPFactor == NULL), then look for two
    // columns and continue. If we were able to find UEC for three columns,
    // then check for any other remaining columns
    if (setWithMinIPFactor)
    {
      statsCreated = TRUE;
      mcUec = mcUec * lookup(*setWithMinIPFactor) / lookup(overlappingCol);
      cumulativeColSetWithMCUEC.addSet(*setWithMinIPFactor);
      remainingCols.subtractSet(*setWithMinIPFactor);
      noOfColsInterested = remainingCols.entries() + 1;

      // while processing, if the cumulative row count becomes greater than
      // the rowcount, break out of the loop
      if (mcUec >= oldRowCount)
      {
	mcUec = oldRowCount;
	break;
      }
    }
    else
      noOfColsInterested--;
  } // while (noOfColsInterested > 1)
  return statsCreated;
} // MultiColumnUecList::createMCUECWithOverlappingColSets

// Following method creates multi-col UEC for larger set of columns
// using partial disjoint multi-col UECs.
// For example, if MC-UEC available - (a, b) (c, d).
// Then MC (a, b, c, d) = MC (a, b) * MC (c, d)
NABoolean MultiColumnUecList::createMCUECWithDisjointColSets(
					ValueIdSet & remainingCols, /* in / out */
					ValueIdSet & cumulativeColSetWithMCUEC, /* in / out */
					CostScalar & mcUec /* in / out*/,
					CostScalar oldRowCount)
{

  NABoolean statsCreated = FALSE;

  while (remainingCols.entries() > 1)
  {
    // collect all disjoint multi-col UEC column sets. In case of a conflict, get the
    // one with largest correlation
    const ValueIdSet & largestSubset = this->largestSubset(remainingCols);

    if (largestSubset.entries() < 2)
    {
      // return no more multi-col UECs for this table
      return statsCreated;
    }

    mcUec *= lookup(largestSubset);
    cumulativeColSetWithMCUEC.addSet(largestSubset);
    remainingCols.subtractSet(largestSubset);
    statsCreated = TRUE;

    // if UEC has exceeded rowcount, no need to go further
    if (mcUec >= oldRowCount)
    {
      mcUec = oldRowCount;
      break;
    }
  } // end while (remainingCols.entries() > 1)

  return statsCreated;

} // MultiColumnUecList::createMCUECWithDisjointColSets

// In the following method, we shall create a new MC list.
// This list contains MC-UEC for only those column set, which include
// all columns of colsWithReductions and at most one column from
// cumulativeColSetWithMCUEC

MultiColumnUecList * MultiColumnUecList::createMCListForRemainingCols(
			      ValueIdSet remainingCols,
			      ValueIdSet cumulativeColSetWithMCUEC)
{

  MultiColumnUecList * groupUec = new (STMTHEAP) MultiColumnUecList ();
  // There can be several levels of overlapping. Example there can be one
  // column overlapping, or two columns overlapping and so on. Process of
  // trying all overlapping combinations can be very expensive, and might
  // not be very cost effective. Hence we shall look for only first level
  // of overlapping, in other words look for only those partial multi-col
  // UECs in which we have one column overlapping
  CollIndex noOfColsInterested = remainingCols.entries();

  ValueIdSet * keyEntry = NULL ;
  CostScalar * uecEntry = NULL ;
  ValueIdSet overlappingCol;
  MultiColumnUecListIterator iter (*this) ;

  // traverse through the multi-column uec list to get the relevant mc entries
  Int32 i;

  for ( iter.getNext (keyEntry, uecEntry), i = 1 ;
	keyEntry != NULL && uecEntry != NULL ;
	iter.getNext (keyEntry, uecEntry), i++ )
  {
    // We identify the interesting entries in the MC list as follows:
    // Example, remainingCols = (a, b, c), and cumulativeColSetWithMCUEC is (d, e, f)
    // so the only entries that can interest are (a, b), (a, b, c), (a, b, d),
    // (a, b, c, d) etc. The entries that are of no interest are
    // (a, b, c, g) - extra column 'g'
    // (a, b, d, e) - more than one overlapping columns
    // (a, b, c, d, e) - more than one overlapping column. This case can be identified
    //		just by counting the number of entries in the MC set. Number
    //		of entries should not be more than number of columns remaining + 1
    // so not interested

    // if the number of entries in the MC is greater than (no. of cols
    // remaining + 1) then we are not interested in this.
    // interested (a, b, c). Available is (d, e, f)
    // Current MC entry could be
    // (a, b, c, d, e) - two overlapping
    // or (a, b, c, d, g) - one overlapping but an extra column
    // or (a, b, g, h, i) - no overlapping but extra columns
    if (keyEntry->entries() > (noOfColsInterested + 1) )
	continue;

    // MC entry has
    // less than columns interested - could be (a, b) or (a, d) or (a, g) or (g, h)..
    // equal to columns interested - could be (a, b, c), (a, b, d), (a, g, d),
    //				      (a, g, e) ..
    // equal to columns interested + 1 - could be (a, b, c, d), (a, b, d, e),
    //				      (a, b, c, e), (a, b, d, g), ....
    ValueIdSet cols;
    cols = *keyEntry;

    // it could be intereting set, if it contains all remaining columns, and
    // one column from the largestSubset

    cols.subtractSet(remainingCols);
    if (cols.entries() > 1)
    {
      // contains no or more than one overlapping columns. Or any other extra columns
      // Entries like (g, h), (a, g, d), (a, g, e), (a, b, d, e), (a, b, d, g) will
      // be eliminated. Since all have more than one column overlapping or some
      // extra columns
      continue;
    }

    // Entries remaining are (a, b), (a, d), (a, g), (a, b, c), (a, b, d),
    // (a, b, c, d), (a, b, c, e)
    if ((cols.entries() == 1) AND !(cumulativeColSetWithMCUEC.contains(cols)) )
    {
      // does not contain overlapping columns, so not interested. (a, g)
      continue;
    }

    // entries that will be added are like
    // (a, b), (a, b, c) - cols.entries() == 0, no columns overlapping
    // (a, d), (a, b, d), (a, b, c, d), (a, b, c, e) - cols.entries() == 1
    // and cumulativeColSetWithMCUEC.contains(cols)
    //
    groupUec->insertPair(*keyEntry, *uecEntry);
  }
  return groupUec;
} // MultiColumnUecList::createMCListForRemainingCols


//
// useful debugging routines
//

// LCOV_EXCL_START - dpm
void
MultiColumnUecList::print (FILE *ofd,
                           const char * prefix,
                           const char * suffix) const

{
#ifndef NDEBUG
  ValueIdSet * keyEntry = NULL ;
  CostScalar * uecEntry = NULL ;
  MultiColumnUecListIterator iter (*this) ;

  fprintf (ofd, "================================================\n") ;
  fprintf (ofd, "%sEntries: %i\n", prefix, this->entries() ) ;

  Int32 i ;
  for ( iter.getNext (keyEntry, uecEntry), i = 1 ;
        keyEntry != NULL && uecEntry != NULL ;
        iter.getNext (keyEntry, uecEntry), i++ )
    {
      fprintf (ofd, "%s  (** entry %i**) ",prefix,i) ;
      keyEntry->display() ;
      fprintf (ofd, " ===> uec: %g\n", uecEntry->value()) ;
    }
  fprintf (ofd, "================================================\n") ;
#endif
}

void
MultiColumnUecList::display() const
{
  print();
}
// LCOV_EXCL_STOP

void
MultiColumnUecList::displayMissingStatsWarning(TableDesc * mostRefdTable,
                                              ValueIdSet predCols,
                                              NABoolean largeTableNeedsStats,
                                              NABoolean displayWarning,
                                              const ColStatDescList &colStats,
                                              CostScalar redFromSC,
                                              NABoolean quickStats,
                                              OperatorTypeEnum op) const
{
  HSLogMan *LM = HSLogMan::Instance();
  LM->LogTimeDiff("START MISSING HISTOGRAM WARNING MESSAGES", TRUE);
  
  // Do not display warning if user does not want to use multi-column
  if ((predCols.entries() > 1) &&
    (CmpCommon::getDefault(HIST_MC_STATS_NEEDED) == DF_OFF) )
    return;

  // Do not display warning if the query is an internal query from
  // the executor

  if (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
    return;

  const NATable * mostRefdNATable = mostRefdTable->getNATable();

  // create a set of column positions for predCols. These are all base tables
  // so we should be able to cast each one of them to BaseCols. This is necessary
  // to take care of cases where we have the same columns appearing in both the
  // query and its sub-query. If we go by the columns ValueIds, then, since the
  // ValueIds of the column in the two places would be different, the warning
  // would be inserted twice. A good example of this is TPCD query 2, where
  // columns such as ps_partkey, ps_suppkey appear in both the main query and
  // its sub-query. By caching with its column position we can avoid that.
  // This should not be very expensive, since column numbers are cached with the
  // column expression

  CollIndexSet setOfColsWithMissingStats(NULL,STMTHEAP);

  // define ValueId outside of for loop, to avoid c++ compiler error.
  ValueId col;
  for (col = predCols.init();
       predCols.next(col);
       predCols.advance(col) )
  {
    BaseColumn * baseCol = col.castToBaseColumn();

    if (baseCol == NULL)
      return;

    CollIndex colNumber = baseCol->getColNumber();

    NAColumn *column = baseCol->getTableDesc()->getNATable()->
                                getNAColumnArray()[colNumber];

    // Don't give a warning if it is not marked for histogram
    // or it is not a user column. Exception is salt column, for
    // which we suppress warning, but create empty histogram if
    // automation is enabled.
    if ( !column->isReferencedForHistogram() ||
         (!column->isUserColumn() && !column->isSaltColumn()))
      return;
    setOfColsWithMissingStats.insert(colNumber );
  }

  // Now check to see, if the warning for missing MC stats has already been
  // inserted in the diags area. If it does then return
  if (mostRefdNATable->doesMissingStatsWarningExist(setOfColsWithMissingStats) )
    return;

  HSGlobalsClass::schemaVersion = mostRefdNATable->getObjectSchemaVersion();
  Int32 warningNumber;
  Lng32 ustatAuto  = CmpCommon::getDefaultLong(USTAT_AUTOMATION_INTERVAL);
  Lng32 ustatLevel = CmpCommon::getDefaultLong(USTAT_AUTO_MISSING_STATS_LEVEL);
  // Determine if the missing histogram should be inserted by the compiler into the HISTOGRAMS table
  // for update statistics automation.  The following must ALL be true:
  //  1. NOT an MP table
  //  2. Schema version of table >= 2300
  //  3. Table is not volatile or automation is ON for volatile tables.
  //  4. Auto missing stats CQD level is set high enough for this single or multi-col hist.
  if (  ustatAuto > 0 && 
        HSGlobalsClass::schemaVersion >= COM_VERS_2300 &&
        (!mostRefdNATable->isVolatileTable() || 
         CmpCommon::getDefault(USTAT_AUTO_FOR_VOLATILE_TABLES) == DF_ON) &&
        ((ustatLevel >= 1 && predCols.entries() == 1) ||
         (ustatLevel >= 2 && op == REL_SCAN)          ||
         (ustatLevel >= 3 && op == REL_JOIN)          ||
         (ustatLevel >= 4 && op == REL_GROUPBY)          )
     )
    {
       // Ustat automation is on for this object.
       // Insert empty entry to HISTOGRAMS table.

       // Determine if this is a synonym and if so, get name of table instead.
       Int64 tableUID;
       NAString tableName;
       NAString histogramSchemaName;

       if (mostRefdNATable->getIsSynonymTranslationDone()) {
         tableName = mostRefdNATable->getSynonymReferenceName();
         tableUID = mostRefdNATable->getSynonymReferenceObjectUid().get_value();
         // Get catalog and schema name.  Find end of schema name.
         QualifiedName qualName(tableName, 3);
         histogramSchemaName = qualName.getSchemaNameAsAnsiString();
       }
       else {
         tableName = mostRefdNATable->getExtendedQualName().getText();
         tableUID = mostRefdNATable->objectUid().get_value();
         histogramSchemaName = mostRefdNATable->getTableName().getCatalogName() + "."
                            + mostRefdNATable->getTableName().getSchemaName();
       }
       NAString histogramTableName = getHistogramsTableLocation(histogramSchemaName,FALSE)
                                     + "." + HBASE_HIST_NAME;

       Lng32 retcode = -1;
       HSinsertEmptyHist hist(tableUID, tableName.data(), histogramTableName.data());
       Lng32 colCount = 0;
       // get col numbers
       for ( col = predCols.init(); 
             predCols.next(col); 
             predCols.advance(col))
         {
            BaseColumn * baseCol = col.castToBaseColumn();
            if (baseCol)
              {
                 Lng32 colNumber = baseCol->getColNumber();
                 retcode = hist.addColumn(colNumber);
                 if (retcode == 0) 
                   colCount++;
              }
         }
       if ( retcode == 0 && predCols.entries() == (CollIndex)colCount ) // got every col number?
         {
         NABoolean switched = FALSE;
         CmpContext* prevContext = CmpCommon::context();
         // switch to another context to avoid spawning an arkcmp process when compiling
         // the user metadata queries on the histograms tables
         if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
            if (SQL_EXEC_SWITCH_TO_COMPILER_TYPE(CmpContextInfo::CMPCONTEXT_TYPE_USTATS))  //CMPCONTEXT_TYPE_META))
            {
               //failed to switch/create metadata CmpContext,  continue using current compiler context
            }
            else
            {
               switched = TRUE;
               // send controls to the context we are switching to
               sendAllControls(FALSE, FALSE, FALSE, COM_VERS_COMPILER_VERSION, TRUE, prevContext);
            }

         retcode = hist.insert();

         // switch back to previous compiler context if we switched above
         if (switched == TRUE)
           SQL_EXEC_SWITCH_BACK_COMPILER();
         }

       if (predCols.entries() == 1) 
       {
         if (quickStats)
           warningNumber = SINGLE_COLUMN_SMALL_STATS_AUTO;
         else
           warningNumber = SINGLE_COLUMN_STATS_NEEDED_AUTO;
       }
       else
         warningNumber = MULTI_COLUMN_STATS_NEEDED_AUTO;
      }
      // Ustat automation is NOT on.
      else
      if (predCols.entries() == 1)  
      {
        if (quickStats)
          warningNumber = SINGLE_COLUMN_SMALL_STATS;
        else
          warningNumber = SINGLE_COLUMN_STATS_NEEDED;
      }
      else
        warningNumber = MULTI_COLUMN_STATS_NEEDED;
  // Do not display warning if rows < default CQD (which could be true if 
  // ustat automation is on).
  if (CURRSTMT_OPTDEFAULTS->ustatAutomation() &&
      (mostRefdTable->getTableColStats()[0]->getColStats()->getRowcount() <
       CostPrimitives::getBasicCostFactor(HIST_ROWCOUNT_REQUIRING_STATS)))
    return;

  // Do not display warning for salt column, although an empty histogram may
  // have been added to histograms table above. Use new ValueIdSet nonSaltPredCols
  // for remainder of function, which is predCols with any salt column removed.
  ValueIdSet nonSaltPredCols(predCols);
  ValueIdSet saltCols;
  for (col = predCols.init();
             predCols.next(col);
             predCols.advance(col))
  {
    if (col.isSaltColumn())
      saltCols.addElement(col.toUInt32());
  }
  nonSaltPredCols.remove(saltCols);
  if (nonSaltPredCols.isEmpty())
    return;

  // Now see, if we should issue a missing single column or a missing multi-col
  // stats warning. This would depend on the number of columns on which the
  // statistics is missing.

  // check the warning level
  // Warning level set to 0, or largeTableNeeds Stats is false, 
  // do not display any warning
  if ((CURRSTMT_OPTDEFAULTS->histMissingStatsWarningLevel() == 0) ||
      (largeTableNeedsStats == FALSE) )
    return;

  // If warning level is one, display only single column stats warning.
  if ((CURRSTMT_OPTDEFAULTS->histMissingStatsWarningLevel() == 1) &&
      (nonSaltPredCols.entries() > 1) )
    return;

  // If this is a multi-column (MC) stat, check to see if it should be displayed as
  // a warning based on the operation and the warning level.
  if ((nonSaltPredCols.entries() > 1) &&
      ((CURRSTMT_OPTDEFAULTS->histMissingStatsWarningLevel() < 2 && op == REL_SCAN)    ||
       (CURRSTMT_OPTDEFAULTS->histMissingStatsWarningLevel() < 3 && op == REL_JOIN)    ||
       (CURRSTMT_OPTDEFAULTS->histMissingStatsWarningLevel() < 4 && op == REL_GROUPBY)) )
    return;

  // dump the warning in the diags area

  NAString tableName(CmpCommon::statementHeap()); // table name

  NAString tableCols(CmpCommon::statementHeap()); // column names
  NAString predicates(CmpCommon::statementHeap()); // predicates

  ValueIdSet thePreds;
  thePreds.clear();

  if (mostRefdNATable->getIsSynonymTranslationDone())
    tableName = mostRefdNATable->getSynonymReferenceName();
  else
    tableName = mostRefdNATable->
      getExtendedQualName().getQualifiedNameObj().getQualifiedNameAsAnsiString();

  // collect all column names into a string

  NABoolean first = TRUE;
  NAString connectorText(", ");
  tableCols += "(";

  NAString opName (CmpCommon::statementHeap());
  switch (op)
  {
  case REL_SCAN:
    opName = "Scan";
    break;
  case REL_JOIN:
    opName = "Join";
    break;
  case REL_GROUPBY:
    opName = "GroupBy";
    break;
  }

  for (col = nonSaltPredCols.init();
       nonSaltPredCols.next(col);
       nonSaltPredCols.advance(col))
  {
      if (first)
	first = FALSE;
      else
	tableCols += connectorText;

    BaseColumn * baseCol = col.castToBaseColumn();

	if (baseCol == NULL)
	  return;

	NAString colName(baseCol->getTableDesc()->getNATable()->
					getNAColumnArray()[baseCol->getColNumber()]->
					getColName(), STMTHEAP);

	tableCols += ToAnsiIdentifier(colName);

        // if ustat logging is ON, log the predicates for which the warning is
        // being issued
        if (LM->LogNeeded() && colStats.entries() > 0) 
        {
          // also collect the predicates applied on this column
          CollIndex i;
          // get the histogram for the column
          NABoolean found = colStats.getColStatDescIndexForColumn(i, col);
          if (found)
          {
            // histogram for which the warning is being issued is found
            LM->Log("histogram for column with missing stats found");
            ColStatDescSharedPtr tempStatDesc = colStats[i];
            if (tempStatDesc->getAppliedPreds().entries() > 0)
            {
              LM->Log("column for missing stats has been reduced by a predicate");
              thePreds = tempStatDesc->getAppliedPreds();
              thePreds.unparse(predicates, DEFAULT_PHASE, EXPLAIN_FORMAT, mostRefdTable);
              sprintf(LM->msg, "%s",predicates.data());
              LM->Log(LM->msg);
            }
            else
              // This condition will happen either for group bys 
              // if that happens for scans or joins then there is a bug in the code
              LM->Log("column for missing stats did not have any predicate applied");
          }
          else
            // This would mean a bug in the code as this should never happen
            LM->Log("histogram for column with missing stats NOT found");
        }
  }

  tableCols += ")";

  *CmpCommon::diags() << DgSqlCode( warningNumber )
    << DgString0( tableCols )
    << DgString1( tableName )
    << DgString2(opName);

  if (LM->LogNeeded() && nonSaltPredCols.entries() > 1)
  {
    LM->Log("MC Warning for table, col set, operator, selectivity: ");
    sprintf(LM->msg, "%s, %s, %s, %e", tableName.data(), tableCols.data(), opName.data(), redFromSC.getValue());
    LM->Log(LM->msg);
  }

  LM->LogTimeDiff("END MISSING HISTOGRAM WARNING MESSAGES");

  // insert the columns with missing stats in the table Desc, so it is not displayed again
  // insert the columns with missing stats in the table Desc
  mostRefdNATable->insertMissingStatsWarning(setOfColsWithMissingStats);
}

// -----------------------------------------------------------------
// isMCStatsUseful is used to determine if there is any possibility
// of optimizer benefiting from multi-column stats. The MC stats 
// are said to be not helpful, if any subset of given column set
// is orthogonal. More heuristics can be added later to determine
// usefulness of MC stats
// -----------------------------------------------------------------
NABoolean MultiColumnUecList::isMCStatsUseful(ValueIdSet columnSet,
                                              TableDesc * tableDesc) const
{
  // This is used for only multi column stats, hence if columnSet
  // consists of single column, return FALSE
  if (columnSet.entries() == 1)
    return FALSE;

  // check to see if columnSet is unique, if it is, return FALSE.
  if (columnSet.doColumnsConstituteUniqueIndex(tableDesc))
    return FALSE;

  // get the largest subset of columns for which MC UEC exist
  // if MC stats exist for the given column set, then return
  // column set will be identical to column set passed.
  // Since the method is currently being called only when stats
  // for columnSet is missing, we will get a subset of columns
  ValueIdSet colSetWithMCStats = largestSubset(columnSet);

  // if MC stats do not exist for any columns, the colSetWithMCStats will 
  // be empty. We return TRUE then to indicate that stats will be useful
  if (colSetWithMCStats.entries() == 0)
    return TRUE;

  // if mcUec is almost equal to the rowcount, that means that columns are almost
  // orthogonal. MC stats for larger set may not give enough benefit then. 
  // This is TRUE even if the largest subset is a single column. However
  // if columnSet is equal to colSetWithMCStats then we cannot use this assumption
  if (colSetWithMCStats.entries() <= columnSet.entries())
  {
    // get the MC UEC of the columnSet
    CostScalar mcUec = lookup(colSetWithMCStats);
    // get rowcount from the base table
    CostScalar tableRowCount = tableDesc->tableColStats()[0]->getColStats()->getRowcount();

    // if MC UEc is almost equal to the row count, we may not benefit much from the MC stats
    // return FALSE in that case
    CostScalar uecCushion((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_8));
    if (mcUec >= uecCushion * tableRowCount)
      return FALSE;
  }
  return TRUE;
} // isMCStatsUseful

CostScalar
ColStatDescList::getCardOfBusiestStreamForUnderNJ(
                                        CANodeIdSet * outerNodeSet,
                                        const PartitioningFunction* partFunc,
                                        Lng32 numOfParts,
                                        GroupAttributes * groupAttr,
                                        Lng32 countOfCPUs)

{
  // get the partitioning key and number of partitions

  ValueIdSet partKey = partFunc->getPartitioningKey(); 

  // get the total rows in the histogram
  CostScalar rowCount = (*this)[0]->getColStats()->getRowcount();

  // if number of partitions is 1, return rowcount

  if ( ( numOfParts == 1) ||
       (partFunc->isASkewedDataPartitioningFunction()) ||
	   ( partFunc->isASinglePartitionPartitioningFunction() ) )
  {
	return (rowCount).minCsOne();
  }

  // The cardinality is based on the number of CPUs or the number of
  // partitions (whichever is fewer) for a few situations:
  //   1) if partitioning key is empty
  //   2) the round robin partitioning scheme is used.
  //   3) the skew buster partitioning scheme is used.
  if ( (partKey.isEmpty()) ||
       (partFunc->isASkewedDataPartitioningFunction()) ||
       (partFunc->isARoundRobinPartitioningFunction()) )
  {
	 Lng32 availableCpus = MINOF( numOfParts , countOfCPUs );
	 return (rowCount / availableCpus).minCsOne();
  }

  // In the following loop, get the min UEC from amongst the partitioning
  // key to compute the number of streams. In the same loop also compute 
  // the accumulated frequency of the partitioning key to compute cardinality
  // per stream for hash partitions

  // min UEC for partitioning key
  CostScalar uecForPartKey = csOne;

  // accumulated freq of the partitioning key
  CostScalar  accFreq = csOne;

  ValueIdSet myColumns;
  NABoolean outerReferencesExist = FALSE;
  ValueIdSet usedCols;
  ValueIdSet referencedPartKeyColSet;

  if (groupAttr)
  {
    ValueIdSet baseCols;
    ValueIdSet myInputs = groupAttr->getCharacteristicInputs();

    if (NOT myInputs.isEmpty())
    {
       outerReferencesExist = TRUE;
       myInputs.findAllReferencedBaseCols(myColumns);
    }

    if (!outerNodeSet)
    {
      rowCount = rowCount.minCsOne();
      return (rowCount/(MINOF((CostScalar)numOfParts, rowCount))).minCsOne();
    }

    ValueIdSet outerBaseCols = outerNodeSet->getUsedTableCols();
    myColumns.intersectSet(outerBaseCols);

    GroupAnalysis * grpAnalysis = groupAttr->getGroupAnalysis();

    if (grpAnalysis)
    {
       CANodeIdSet treeSet = grpAnalysis->getAllSubtreeTables();

       if (NOT treeSet.isEmpty() )
       {
         // note that UsedCols do not contain any columns
         // from outer references. They are "local"
         usedCols = treeSet.getUsedCols();
         myColumns.addSet(usedCols);
       }
     }
  }
  
  for (ValueId partKeyElement = partKey.init();
                                partKey.next(partKeyElement);
                                partKey.advance(partKeyElement) )
  {

     // extract all base columns from the partitioning key column before  
     // looking for statistics

     ValueIdSet baseColSet;

     // this is because findAllReferencedBaseCols, is defined on ValueIdSet
     ValueIdSet partKeySet(partKeyElement);

     partKeySet.findAllReferencedBaseCols(baseColSet);

	// from all the base columns for this partitioning key, get the
	// one that belong to me. In case of joins, base column set would
	// also contain the column I am joining to

      if (NOT myColumns.isEmpty())
         baseColSet.intersectSet(myColumns);

      // if outer references are present, then it is under a nested join
      // want to compute columns from the partitioning key VEGRef
      // that are from the outer references

      ValueIdSet partKeySourceSide;
      if (outerReferencesExist)
      {      
        //want to do this; note usedCols has no outer references
        // partKeySourceSide=baseColSet - usedCols

        ValueIdSet origSet = baseColSet;
        origSet.subtractSet(usedCols);
        partKeySourceSide = origSet;
      }

      CostScalar freq;
      // get min of max frequency of any column from the given column set
      // If outer references exist compute from amongst the columns coming
      // from the outer, else do it from the inner side
      // In most cases, there would be only one column in the column
      // set for which the frequency is needed. In case of expressions
      // there could be more than one column in the column set. 
      if (outerReferencesExist)
      {
        freq = getMinOfMaxFreqOfCol(partKeySourceSide).minCsOne();
      }
      else
      {
        freq = getMinOfMaxFreqOfCol(baseColSet).minCsOne();
      }

      // get minimum UEC from the given column set;
      //  this needs columns from outer references as well
      CostScalar colUec;
	  
      if (CmpCommon::getDefault(COMP_BOOL_47) == DF_OFF)
      {
        if (outerReferencesExist)
          colUec = getMinUec(partKeySourceSide);
        else
          colUec = getMinUec(baseColSet);
      }
      else
      {
        if (outerReferencesExist)
          colUec = getMaxUec(partKeySourceSide);
        else
          colUec = getMaxUec(baseColSet);
      }
	      
      if (colUec == csMinusOne)
         uecForPartKey = rowCount;
      else
	uecForPartKey *= colUec;

      accFreq *= (freq / rowCount);
    } // for all partitioning key columns

  // uec cannot be greater than the row count
  uecForPartKey = MINOF(uecForPartKey, rowCount);

  // compute the number of streams 
  CostScalar noOfStreams = MINOF((CostScalar)numOfParts, uecForPartKey);

  // If partitioning key column is a Random number, and activeStreams_ is less
  // than the current value of noOfStreams, then noOfStreams = activeStreams_;
  CostScalar activeStreams = partFunc->getActiveStreams();
  long randomFix = ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_26);
  if ( (partKey.entries() == 1) AND (randomFix != 0) AND (activeStreams != 0) )
  {
    // Get first key column.
    ValueId myPartKeyCol;
    partKey.getFirst(myPartKeyCol);
    // is it a random number?
    if (myPartKeyCol.getItemExpr()->getOperatorType() == ITM_RANDOMNUM)
    {
      noOfStreams = MINOF(activeStreams, noOfStreams);
    }
  }

  // based on the partitioning type, compute the cardinality of the stream
  // For all but hash type partitions, cardinality = number of rows / # of streams
  // such that # of streams = MINOF(# of partitions, UEC of partitioning key)
  
  if (partFunc->isAHashPartitioningFunction() ||
	  partFunc->isATableHashPartitioningFunction() )
  {
    CostScalar cardOfFreqValue = (rowCount * accFreq).minCsOne();

    CostScalar maxCardPerStream = (((rowCount - cardOfFreqValue)/noOfStreams) + cardOfFreqValue).round();

    // cardinality per stream cannot be greater than the total row count
    maxCardPerStream = MINOF(maxCardPerStream, rowCount);

    // some of NJ plans were over-penalized by incorporate_skew_in_costing.
    // provide a cqd HIST_SKEW_COST_ADJUSTMENT to soften effect
    // of incorporate_skew_in_costing. 
    // take weighted average of uniform distribution and skewed data 
    // based on the CQD. 
    // 0 -> get RC as if uniformly distributed
    // 1-> get RC as if skewed
    // anything in between is the linear average
    CostScalar uniformDistRowCountPerStream = rowCount / noOfStreams;
    CostScalar histSkewAdjustment = 
      (ActiveSchemaDB()->getDefaults()).getAsDouble(HIST_SKEW_COST_ADJUSTMENT);
    maxCardPerStream = 
      (maxCardPerStream * histSkewAdjustment) + 
      (uniformDistRowCountPerStream * (csOne - histSkewAdjustment));

    return maxCardPerStream.minCsOne();
  }
  else
  {
    // for all other partitions
    // return rows per stream
    return (rowCount / noOfStreams).minCsOne();
  } // for non hash partitioning functions
} // ColStatDescList::getCardOfBusiestStream

//------------------------------------------------------------------
// Determine if the join predicates consist of column sets where one 
// of the sides is unique and shares relationship "similar" to PK/FK 
// with the other side. If so, cardinality adjustment will be done 
// while merging ColStatDescs in ColStatDesc::mergeColStatDesc method.
// This feature is OFF by default if there are more than one joining
// columns. It'll be controlled by COMP_BOOL_149
//------------------------------------------------------------------

CostScalar MultiColumnUecList::getRowcountOfNonUniqueColSet( const Join *expr, 
                                                        ValueIdList colList1, 
                                                        ValueIdList colList2, 
                                                        NABoolean list1Unique, 
                                                        NABoolean list2Unique)
{
  CostScalar rowcountOfNonUniqSide = csMinusOne;

  // We are using unique characteristic for Joins only, atleast till R2.5 (5/29/09)
  if (!expr || !expr->isAnyJoin())
    return rowcountOfNonUniqSide;

  // We will keep a copy of the set too, as these are more efficient 
  // implementations for lookup etc.
  // all variables suffixed by "1" correspond to column list 1  (colList1) and
  // all suffixed by "2" correspond to colList2. There is no concept of "left"
  // and "right" here since we don't know which child of Join these belong to
  ValueIdSet colSet1 = colList1;
  ValueIdSet colSet2 = colList2;

  // indexes into the colStats list
  CollIndex i;
  // multi column UECs
  CostScalar MCUec1 = csMinusOne, MCUec2 = csMinusOne;

  // Take a cushion to take into account the fizziness that could have been introduced
  // using sampling
  CostScalar uecCushion ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_4));

  MCUec1 = this->lookup(colSet1);
  MCUec2 = this->lookup(colSet2);

  // if multiColUEC does not exist, then return, we cannot use to look for containment,
  // hence cannot use shortcut MC UEC calculations
  // Since we have already added the MCUEC of the unique side, that should always be there
  // We could still have MCUEC for non-unique side missing
  if ((MCUec1 <= 0) || (MCUec2 <= 0))
    return rowcountOfNonUniqSide;

  // the uniqueness and containment are checked on the histograms prior to this join, hence get
  // output logical properties of the children
  // Since we don't know which column list belongs to which child of join. We will look for
  // histogram of the columns in histogram list from both children
  // Create a completeList of histograms from the histograms of the left and the right children
  ColStatDescList completeList = ((Join *)expr)->child(0).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->colStats();
  ColStatDescList rightColStatList = ((Join *)expr)->child(1).outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP))->colStats();

  // form a complete list of histograms from both sides
  completeList.makeDeepCopy(rightColStatList);

  // histograms for the joining columns of col list 1 and 2
  ColStatsSharedPtr colStats1 = NULL;
  ColStatsSharedPtr colStats2 = NULL;

  // if colList1 is unique, then we will look for containment, only if 
  // columnList2 or the non-unique side is contained in the unique side
  if(list1Unique)
  {
    if( (MCUec1 * uecCushion) < MCUec2)
    {
      // although column set 1 is unique, the non-unique set is not contained in the
      // unique side, Check to see if individually each non-unique column is contained
      // in the unique side
      for(i =0; i<colList1.entries(); i++)
      {
        colStats1 = completeList.getColStatsPtrForColumn(colList1[i]);

        // get the histogram pointer for the column in list 1
        if (colStats1 == NULL)
        {
          // if the histogram is missing in both children, then we cannot use 
          // uniqueness property, so return. Setting the flags to FALSE instead of
          // returning -1, just so we have one point of exit.
          list1Unique = FALSE;
          list2Unique = FALSE;
          break;
        }

        // found histogram for column1, now look for column 2 histogram
        colStats2 = completeList.getColStatsPtrForColumn(colList2[i]);
        if (colStats2 == NULL)
        {
          // histogram is not present with either of the children of Join, so return -1
          // Setting the flags to FALSE instead of
          // returning -1, just so we have one point of exit.
          list1Unique = FALSE;
          list2Unique = FALSE;
          break;
        }

        if ((colStats1->getBaseUec() *uecCushion) <= colStats2->getBaseUec())
        {
          // For the uniqueness condition to be satisfied, non-unique side should be
          // contained in the unique side. In this case, the non-unique is larger than
          // the unique, hence we cannot use this containment. Look for the column 2
          // uniqueness and containment feature
          list1Unique = FALSE;
          break;
        }
      }
    }
  }

 if(list1Unique)
 {
   ColStatsSharedPtr colStatsOfNonUniqSide = completeList.getColStatsPtrForColumn(colList2[0]);
   if(colStatsOfNonUniqSide)
     rowcountOfNonUniqSide = colStatsOfNonUniqSide->getRowcount();
   else
     rowcountOfNonUniqSide = csMinusOne;
 }

  // columns from side1 are non-unique check if the columns from other side 
  // constitute a unique set
  if(!list1Unique && list2Unique)
  {
    // They are use by semantics, check for containment
    if( (MCUec2 * uecCushion) < MCUec1)
    {
      // although column set 2 is unique, the non-unique set is not contained in the
      // unique side. Check to see if individually each non-unique column is contained
      // in the unique side
      for(i =0; i<colList2.entries(); i++)
      {
        colStats2 = completeList.getColStatsPtrForColumn(colList2[i]);
        if (colStats2 == NULL)
        {
          // histogram is missing, so don't bother going forward, just return 
          list2Unique = FALSE;
          break;
        }

        // histogram for list 1 found, now look for the corresponding histogram in list 2
        colStats1 = completeList.getColStatsPtrForColumn(colList1[i]);
        if (colStats1 == NULL)
        {
          list2Unique = FALSE;
          break;
        }
 
        if ((colStats2->getBaseUec() * uecCushion) <= colStats1->getBaseUec())
        {
          // if the non-unique side is not contained in the unique side, we cannot use
          // the uniqueness feature
          list2Unique = FALSE;
          break;
        }
      }
   } // if left side is contained in the right side

    if(list2Unique)
    {
      ColStatsSharedPtr colStatsOfNonUniqSide = completeList.getColStatsPtrForColumn(colList1[0]);
      if(colStatsOfNonUniqSide)
        rowcountOfNonUniqSide = colStatsOfNonUniqSide->getRowcount();
      else
        rowcountOfNonUniqSide = csMinusOne;
    }
  }
  return rowcountOfNonUniqSide;
} 

void
ColStatDescList::computeRowRedFactor(MergeType mergeMethod, // input
				     CollIndex numOuterColStats, // input
				     CostScalar rowcountBeforePreds, // input
				     CollIndex & predCountSC, // output
                                     CollIndex & predCountMC, // output
				     CostScalar & rowRedProduct) // output
{
  // If this is a semi-join, it should be the case that only columns
  // in the outer table need to be scaled to the proper cardinality.
  CollIndex loopLimit = ( ( mergeMethod == SEMI_JOIN_MERGE ||
                            mergeMethod == ANTI_SEMI_JOIN_MERGE ) ?
                          numOuterColStats : entries() );

  // need to calculate the product of the row reduction factors
  CollIndex i;
  CostScalar rowCnt;
  // do floor of rowcount before preds to take into account small rounding errorrs that
  // may creep up due to cost scalar arithmetic. Use this adjusted value for computing
  // predCount for MC adjustments only
  CostScalar tempRCBeforePreds = floor(rowcountBeforePreds.getValue());

  for ( i = 0; i < loopLimit; i++ )
  {
    ColStatsSharedPtr tempColStats = (*this)[i]->getColStats();
    rowCnt = tempColStats->getRowcount();
    if (( rowCnt < rowcountBeforePreds ) || (tempColStats->isARollingColumn()))
    {
      predCountSC++;
      rowRedProduct *= rowCnt / rowcountBeforePreds;
    }
    // do a ceil of row count from histograms to take into account
    // any rounding issues that may creep up due to cost scalar arithmetic
    // example:
    // if rowCnt = 1.04 and rowcountBeforePreds = 1 then tempRC = 2 and tempRCBeforePreds = 1 -> do not increment predCountMC
    // if rowCnt = 1 and rowcountBeforePreds = 1 then tempRC = 1 and tempRCBeforePreds = 1 -> do not increment predCountMC
    // if rowCnt = 1 and rowcountBeforePreds = 1.04 then tempRC = 1 and tempRCBeforePreds = 1 -> do not increment predCountMC
    // if rowCnt = 0.99 and rowcountBeforePreds = 1 then tempRC = 1 and tempRCBeforePreds = 1 -> do not increment predCountMC
    // if rowCnt = 1 and rowcountBeforePreds = 0.99 then tempRC = 1 and tempRCBeforePreds = 0 -> do not increment predCountMC
    // if rowCnt = 1.04 and rowcountBeforePreds = 0.99 then tempRC = 2 and tempRCBeforePreds = 0 -> do not increment predCountMC
    // if rowCnt = 0.99 and rowcountBeforePreds = 1.04 then tempRC = 1 and tempRCBeforePreds = 1 -> do not increment predCountMC
    // if rowCnt = 1 and rowcountBeforePreds = 2 then tempRC = 1 and tempRCBeforePreds = 2 -> increment predCountMC
    // if rowCnt = 1.04 and rowcountBeforePreds = 1.9 then tempRC = 2 and tempRCBeforePreds = 1 -> do not increment predCountMC

    CostScalar tempRC = ceil(rowCnt.getValue());
    if (tempRC < tempRCBeforePreds)
      predCountMC++;
  }


  // -----------------------------------------------------------------------
  // in the case of an OUTER_JOIN_MERGE, we need to now undo part of that
  // reduction
  // -----------------------------------------------------------------------

  if ( mergeMethod == OUTER_JOIN_MERGE )
  {
    CostScalar rowred;
    ValueIdSet alreadyMergedHistograms;
    for( i = 0; i < numOuterColStats; i++ )
    {
      alreadyMergedHistograms.insert( (*this)[i]->getMergeState() );
    }
    for( i = numOuterColStats; i < loopLimit; i++ )
    {
      if( alreadyMergedHistograms.contains( (*this)[i]->getColumn() ) )
      {
	rowred = csOne;
	if ( rowcountBeforePreds.isGreaterThanZero() /* > csZero */ )
	{
	  rowred =
	    (*this)[i]->getColStats()->getRowcount() / rowcountBeforePreds;
	}
	rowRedProduct /= rowred ; // remove this reduction, it's a duplicate
      }
    }
  }
}

void ColStatDescList::insertByPosition(const StatsList & other,
                                       const NAColumnArray &columnList, 
                                       const ValueIdList &tableColList)
{
  for (CollIndex i = 0; i < columnList.entries(); i++)
  {
    NAColumn * column = columnList[i];
    short position = (short) column->getPosition();

    for(UInt32 i = 0; i < other.entries(); i++)
    {
      ColStatsSharedPtr otherStats(other[i]);
      const NAColumnArray &otherColumns = otherStats->getStatColumns();

      // Skip to the next ColStats if these stats don't contain
      // this column position.
      // position is checked for only single column histograms
      if ( (otherColumns.entries() == 1) && 
          (!otherColumns.getColumnByPos(position)) )
	      continue;

      ColStatDescSharedPtr tmpStatDesc(new (CmpCommon::statementHeap())
        ColStatDesc (other[i], tableColList), CmpCommon::statementHeap());

      if (otherColumns.entries() == 1)
      {
        // if the histogram requires only a single interval, compress it before inserting
        // it into the colStats list
        NAColumnArray tempArray = other[i]->getStatColumns();

        if ((tempArray.entries() == 1) && (tempArray[0]->isReferencedForSingleIntHist() ) )
        {
          // only for single column histograms. If only single histogram
          // is needed insert the compressed copy of the histogram, else
          // insert the full histogram
          tmpStatDesc->getColStatsToModify()->compressToSingleInt();
        }
      }
      this->insertAt(this->entries(), tmpStatDesc);
      break;
    }
  } // for each column in the column array
}

// This method traverses the histograms and returns the highest UEC
// reduction applied by local predicates among the columns in the
// input valueid set. This information is used during multi-col UEC
// adjustment for join cardinality estimation

CostScalar
ColStatDescList::getHighestUecReductionByLocalPreds(ValueIdSet &cols) const
{
  CostScalar highestUecReductionByLocalPreds = csOne;
  ColStatsSharedPtr tempColStat;

  for (ValueId vid = cols.init(); cols.next(vid); cols.advance(vid))
  {
    // Reduction from columns for which histogram is not present can be ignored
    tempColStat = this->getColStatsPtrForColumn(vid);
    if (!tempColStat)
      continue;

    CostScalar uecReductionFromLocalPreds = (tempColStat->getBaseUec()/tempColStat->getUecBeforePreds()).maxCsOne();;

    // Take the highest reduction, that is the one giving least number of rows out
    if(uecReductionFromLocalPreds < highestUecReductionByLocalPreds)
      highestUecReductionByLocalPreds = uecReductionFromLocalPreds;
  }
  return highestUecReductionByLocalPreds;
}

// ---------------------------------------------------------------------------
// It is a helper method used while computing left joins. The method locates
// histograms that have been joined to right child using inner join and
// now need to be null augmented to simulate the left join
// ---------------------------------------------------------------------------
NABoolean
ColStatDescList::locateHistogramToNULLAugment(ValueIdSet EqLocalPreds /*in*/, 
                                              NAList<CollIndex> &statsToMerge /*out*/, 
                                              CollIndex &rootStatIndex /*out*/, 
                                              CollIndex outerRefCount /*in*/)
{
  // Contains the maximum of number of intervals from amongst the left 
  // histograms that participated in equijoins and would be null instantiated
  // Example for query like T1 left join T2 on T1.a = T2.a and T1.b = T2.b
  // If the resultant histograms after join on column 'a' contains 50 intervals
  // and the resultant histograms after join on column 'b' contains 60
  // intervals, the optimizer will choose index for column 'b' and return that
  // in rootStatIndex. saveNumBuckets will be used to contain the number of 
  // largest count of intervals so far in the iteration.
  CollIndex saveNumBuckets = 0;
  // contains a flag to indicate whether the histogram that participated
  // in the equijoin has been found
  NABoolean foundFlag = FALSE;

  // --------------------------------------------------------------
  // Equijoins allow us to best determine the number of null-
  // augmented rows to be added back into the histogram(s) that
  // describe the join results.
  // First, locate histograms that capture the results of the
  // one-or-more equijoins between the inner and outer tables.
  // --------------------------------------------------------------

  for (ValueId id = EqLocalPreds.init();
                    EqLocalPreds.next (id);
                    EqLocalPreds.advance (id))
  {
    ItemExpr *pred = id.getItemExpr();
    
    // We are interested only in equi join predicates
    // skip any other predicates
    if (pred->getOperatorType() != ITM_VEG_PREDICATE)
      continue;
 
    // look thru all left histograms to find the one that
    // participated in this join
    // outerRefCount contains the count of left histograms,  
    // some of which may be the result of inner join
    for (CollIndex i = 0; i < (CollIndex)outerRefCount; i++)
    {
      // We are not interested in considering histograms created for virtual 
      // columns, such as tuples, for joins so skip to the next histogram
      if ((*this)[i]->getColStats()->isVirtualColForHist() )
      continue;

      // See if the leading column of any histograms reference
      // the VEGReference id.  Among those that do, remember
      // the one with the most intervals.
      // That one will be used to calculate how many rows from
      // the original outer (left) table did not meet the join
      // condition.

      NABoolean foundCandidate = FALSE;
      ItemExpr * columnFromJoinList = (*this)[i]->getVEGColumn().getItemExpr();

      if ( columnFromJoinList->getOperatorType() != ITM_VEG_REFERENCE )
      {
        // the left columns appear as VEG_REFERENCES in the predicate
        // If it is not a VEG_REFERENCE, skip this ColStatDesc and 
        // continue looping
        continue;
      }

      // Check to see if the column of this histogram participated 
      // in this equi-join predicate
      foundCandidate = pred->containsTheGivenValue(columnFromJoinList->getValueId());

      // if it did not, skip to next histogram
      if (!foundCandidate)
        continue;

      ColStatDescSharedPtr statDesc = (*this)[i];
      ColStatsSharedPtr colStats = statDesc->getColStats();

      // save the index of this histogram, to be merged later
      statsToMerge.insert(i);
      foundFlag = TRUE;

      // Save the histogram with most intervals. 
      if (( saveNumBuckets == 0 ) || // anything is better than this
          (colStats->getHistogram()->entries() > saveNumBuckets )) // for VEG Ref
      { //mar: need this because of TEST014, q14, where
        //     an assertion failed because rootStatIndex didn't
        //     change from its init value of 1000
        saveNumBuckets = colStats->getHistogram()->entries() ;
        rootStatIndex = i ;
      }
    }   // for outerRefCount
  }  // for EqLocalPreds

  if ( !foundFlag )
  {
    // --------------------------------------------------------------
    // No usable equality predicates found.
    // Use alternative approach to guess-timating the outer join's
    // impact:
    // Examine the histograms whose shape was changed, if any, to
    // determine the number of rows from the left that need to be
    // null augmented.
    // Again, find the one with the greatest number of intervals.
    // --------------------------------------------------------------
    statsToMerge.clear();  // reset.

    for (CollIndex i = 0; i < (CollIndex)outerRefCount; i++)
    {
      ColStatDescSharedPtr statDesc = (*this)[i];
      ColStatsSharedPtr colStats = statDesc->getColStats();

      // We are not interested in considering virtual columns for joins
      // so skip to the next histogram
      if (colStats->isVirtualColForHist() )
        continue;

      if (colStats->isShapeChanged())
      {
        statsToMerge.insert (i);
        foundFlag = TRUE;

        if ((saveNumBuckets == 0) // we should accept any value for
            || (colStats->getHistogram()->entries() > saveNumBuckets))
        {                      // rootStatIndex if we have none thus far!
          saveNumBuckets = colStats->getHistogram()->entries() ;
          rootStatIndex = i ;
        }
      }
    }  // for outerRefCount
  }  // if !foundFlag

  // if foundFlag is TRUE, 
  // statsToMerge will contain a list of indexes of the histogram to be merged
  // rootStatIndex will contain the index of the histogram of the joining
  // column with largest number of intervals
  return foundFlag;
}

// --------------------------------------------------------------------
// It is a helper method used by left joins. It merges the rows
// from the left side that did not match the right child back into 
// the joined histograms. 
// -------------------------------------------------------------------
CostScalar 
ColStatDescList::computeLeftOuterJoinRC(NABoolean &foundFlag /*out*/,
                                        const ColStatDescList &leftColStatsList /*in*/,
                                        CollIndex rootStatIndex /*in*/)
{
  ColStatDescSharedPtr joinStatDesc;
  ColStatDescSharedPtr origStatDesc;

  // ------------------------------------------------------------
  // Either an equality-predicate or some other shape-changing
  // predicate was involved in this join.  Determine the number
  // of rows that result from merging the original left table
  // with the join result.
  // ------------------------------------------------------------

  // rootStatIndex contains the index of the histogram that participated
  // in Join and amongst more than one joining histograms had most number
  // of intervals
  joinStatDesc = (*this)[rootStatIndex];
  ColStatsSharedPtr joinColStats = joinStatDesc->getColStatsToModify();
  CostScalar innerJoinRC = joinColStats->getRowcount();

  ItemExpr * statExpr = joinStatDesc->getVEGColumn().getItemExpr() ;
  OperatorTypeEnum statOper = statExpr->getOperatorType() ;
  // sanity check
  // We shall not assert for virtual columns, even though later we
  // shall not consider them for join
  // only the following histograms can participate in left joins
  // These could be 
  // 1. VEG_REFERENCE, 
  // 2. result of another left or semi-join (ITM_INSTANTIATE_NULL)
  // 3. result of UNION (ITM_VALUEIDUNION)
  // 4. result of rowset (ITM_UNPACKCOL or ITM_ROWSETARRAY_SCAN)
  // 5. IN list of more than COMP_INT_22 elements (ITM_NATYPE)
  // For any other column type, return LEFT rowcount as the final
  // left join RC
  if (NOT ( statOper == ITM_VEG_REFERENCE ||
            statOper == ITM_INSTANTIATE_NULL ||
            statOper == ITM_VALUEIDUNION ||
            statOper == ITM_UNPACKCOL ||
            statOper == ITM_ROWSETARRAY_SCAN ||
            joinColStats->isVirtualColForHist() ||
            statOper == ITM_NATYPE) )
  {
    CCMPASSERT ( "Incorrect expression participating in Join") ;
    // join result is not reliable any more
    joinColStats->setFakeHistogram(TRUE);
    // set the foundFlag to FALSE to indicate some error happened
    // so the row counts can be set appropriately
    foundFlag = FALSE;
    // in case of an error return the inner join row count as the final rowcount
    // from join.
    return (innerJoinRC);
  }

  // ------------------------------------------------------------
  // Once we have verified the correctness of the histogram
  // find the matching entry in the leftColStatsList....
  // It doesn't Have to be there, but it should be.
  // ------------------------------------------------------------
  NABoolean goodMatch = FALSE;
  for (CollIndex i = 0; i < leftColStatsList.entries(); i++)
  {
    // Skip any histograms created for virtual columns
    if (leftColStatsList[i]->getColStats()->isVirtualColForHist())
      continue;
    origStatDesc = leftColStatsList[i];

    ItemExpr * originalExpr = origStatDesc->getVEGColumn().getItemExpr() ;
    OperatorTypeEnum originalOper = originalExpr->getOperatorType() ;

    if (originalOper == ITM_ROWSETARRAY_SCAN)
      continue;

    // sanity check for the original left histogram
    if (NOT ( originalOper == ITM_VEG_REFERENCE ||
              originalOper == ITM_INSTANTIATE_NULL ||
              originalOper == ITM_VALUEIDUNION ||
              originalOper == ITM_UNPACKCOL ||
              originalOper == ITM_NATYPE) )
    {
      CCMPASSERT ( "Incorrect expression participating in Join") ;
      // join result is not reliable any more
      joinColStats->setFakeHistogram(TRUE);
      // set the foundFlag to FALSE to indicate some error happened
      // so the row counts can be set appropriately
      foundFlag = FALSE;
      // in case of an error return the inner join row count as the final rowcount
      // from join.
      return (innerJoinRC);
    }

    if ( originalExpr == statExpr ) // this is a ValueId comparison
    {
      if (originalOper != statOper)
      {
        // sanity check to ensure that the histogram type was not modified during inner join
        CCMPASSERT( "Two mismatched expressions being joined" ) ; // no reason this should fail
        // join result is not reliable any more
        joinColStats->setFakeHistogram(TRUE);
        // set the foundFlag to FALSE to indicate some error happened
        // so the row counts can be set appropriately
        foundFlag = FALSE;
        // in case of an error return the inner join row count as the final rowcount
        // from join.
        return (innerJoinRC);
      }
      goodMatch = TRUE ;
      break ;
    }
  } // for loop over leftColStatsList

  if (goodMatch)
  {
    // if all conditions satisfied, do an OR merge of the join result
    // to the original left histograms. This will add the rows from
    // the left histograms that did not match the rigth side
    joinStatDesc->mergeColStatDesc( origStatDesc,
                                    LEFT_JOIN_OR_MERGE,
                                    FALSE,
                                    REL_JOIN,
                                    FALSE/*dontMergeFVs*/);

    // In estimateCardinality, we ensure that the result of
    // Join is at least one. There is a possibility for cases
    // when the result is actually zero, that the mergeColStatDesc
    // reverts the row count back to 0. But because we want the minimum
    // cardinality to be one, we shall, once again uplift it to one.

    CostScalar oJoinResultRows = joinColStats->getRowcount();

    if (oJoinResultRows < csOne)
    {
      joinColStats->setRowsAndUec (csOne, csOne);
      oJoinResultRows = csOne;
    }
    // return the resultant rowcount. That would be result
    // of inner join + number of unmatched rows from the left side
    foundFlag = TRUE;
    return oJoinResultRows;
  }
  else
  {
    // Original histogram for the joined histogram not found.
    foundFlag = FALSE;
    // return leftRowCount as the final rowcount
    return innerJoinRC;
  }
}

// --------------------------------------------------------------------
// It is a helper method used by full outer joins. It merges the rows
// from the right side that did not match the left child back into 
// the joined histograms. 
// -------------------------------------------------------------------
CostScalar 
ColStatDescList::computeFullOuterJoinRC(NABoolean &foundFlag /*out*/,
                                    const ColStatDescList &origColStatsList /*in*/,
                                    CollIndex rootStatIndex /*in*/)
{
  ColStatDescSharedPtr joinStatDesc;
  ColStatDescSharedPtr origStatDesc;

  // ------------------------------------------------------------
  // Either an equality-predicate or some other shape-changing
  // predicate was involved in this join.  Determine the number
  // of rows that result from merging the original left table
  // with the join result.
  // ------------------------------------------------------------

  // rootStatIndex contains the index of the histogram that participated
  // in Join and amongst more than one joining histograms had most number
  // of intervals
  joinStatDesc = (*this)[rootStatIndex];
  ColStatsSharedPtr joinColStats = joinStatDesc->getColStatsToModify();
  CostScalar innerJoinRC = joinColStats->getRowcount();

  // since the right side would have been merged into the left, the right
  // column reference should appear in the mergedState of the joined result
  ValueIdSet mergedSet = joinStatDesc->getMergeState();

  // ------------------------------------------------------------
  // Once we have verified the correctness of the histogram
  // find the matching entry in the leftColStatsList....
  // It doesn't Have to be there, but it should be.
  // ------------------------------------------------------------
  NABoolean goodMatch = FALSE;
  for (CollIndex i = 0; i < origColStatsList.entries(); i++)
  {
    // Skip any histograms created for virtual columns
    if (origColStatsList[i]->getColStats()->isVirtualColForHist())
      continue;

    origStatDesc = origColStatsList[i];
    ValueId originalCol = origStatDesc->getColumn();
    OperatorTypeEnum originalOper = originalCol.getItemExpr()->getOperatorType() ;

    if (originalOper == ITM_ROWSETARRAY_SCAN)
      continue;

    if ( mergedSet.containsTheGivenValue(originalCol)) // this is a ValueId comparison
    {
      goodMatch = TRUE ;
      break ;
    }
  } // for loop over leftColStatsList

  if (goodMatch)
  {
    // if all conditions satisfied, do an OR merge of the join result
    // to the original left histograms. This will add the rows from
    // the left histograms that did not match the right side
    joinStatDesc->mergeColStatDesc( origStatDesc,
                                    LEFT_JOIN_OR_MERGE,
                                    FALSE,
                                    REL_JOIN,
                                    FALSE/*dontMergeFVs*/);

    // In estimateCardinality, we ensure that the result of
    // Join is at least one. There is a possibility for cases
    // when the result is actually zero, that the mergeColStatDesc
    // reverts the row count back to 0. But because we want the minimum
    // cardinality to be one, we shall, once again uplift it to one.

    CostScalar oJoinResultRows = joinColStats->getRowcount();

    if (oJoinResultRows < csOne)
    {
      joinColStats->setRowsAndUec (csOne, csOne);
      oJoinResultRows = csOne;
    }
    // return the resultant rowcount. That would be result
    // of inner join + number of unmatched rows from the left side
    foundFlag = TRUE;
    return oJoinResultRows;
  }
  else
  {
    CCMPASSERT("Original histogram for the joined histogram in full outer join not found");
    // Original histogram for the joined histogram not found.
    foundFlag = FALSE;
    // return inner join RC as the final rowcount
    return innerJoinRC;
  }
}

// ----------------------------------------------------------------------------------
// This is a helper method used by left joins. 
// The method is called after the the rows from the left histograms of the joining column,
// that did not match the right side are merged back into the join result. 
// In the following method, the histograms from the remaining columns are synchronized
// to have the same row count
// --------------------------------------------------------------------------------
void
ColStatDescList::synchronizeHistsWithOJRC(NAList<CollIndex> &statsToMerge /*in */, 
                                          CollIndex startIndex /*in*/,
                                          CollIndex stopIndex /*in*/,
                                          CollIndex rootStatIndex /*in*/, 
                                          const ColStatDescList &origColStatsList /*in*/,
                                          CostScalar &oJoinResultRows /*out*/, 
                                          CostScalar &baseRows /*out*/)
{
  // ---------------------------------------------------------------
  // Force All histograms to have the rowCount predicted previously:
  // either by mergeColStatDesc, or by the sWAG.
  //
  // Examine each histogram from the Left table.  If it had a shape-
  // changing predicate applied, OR the join result with the original
  // histogram for that column, as in the root case.
  // Otherwise, just changing their reduction factor to scale them to
  // the sWAG'ed result size.
  // ---------------------------------------------------------------
  CollIndex i = 0;
  for (i = startIndex; i < stopIndex; i++)
  {
    // indicates whether this histogram had a predicate applied to it
    NABoolean inPredicate = FALSE;
    // indicates whether we were able to find a match for the joined
    // histogram from the list of original histograms 
    NABoolean goodMatch = FALSE;

    // The histogram with rootStatIndex has already been merged
    // so skip that
    if ( rootStatIndex == i )
      continue ;

    // See if this column was involved in a predicate.
    // statsToMerge contain all the histogram indexes
    // that participated in join
    // Break out of the loop that 
    CollIndex j = 0;
    for (j = 0; j < statsToMerge.entries() && !inPredicate; j++)
    {
      if ( i == statsToMerge[j] )
        inPredicate = TRUE;
    }

    // ------------------------------------------------------
    // If involved in a predicate: do merge based calculation.
    // And, in either case, Tweek the resulting row count to
    // match that predicted above.
    // ------------------------------------------------------
    ColStatDescSharedPtr jStatDesc = (*this)[i];
    ColStatsSharedPtr jColStats = jStatDesc->getColStatsToModify();

    // if the histogram is for virtual columns, or from rowsets, 
    // skip mergebased calculations, simply synchronize the histogram
    // with the rowcount computed earlier. 
    if (jColStats->isVirtualColForHist() )
    {
      CostScalar oldCount = jColStats->getRowcount();
      if (oldCount != oJoinResultRows)
        jStatDesc->synchronizeStats (oldCount, oJoinResultRows);
      continue;
    }

    ItemExpr * statExpr = jStatDesc->getVEGColumn().getItemExpr() ;
    OperatorTypeEnum statOper = statExpr->getOperatorType() ;

    if (statOper == ITM_ROWSETARRAY_SCAN)
    {
      CostScalar oldCount = jColStats->getRowcount();
      if (oldCount != oJoinResultRows)
        jStatDesc->synchronizeStats (oldCount, oJoinResultRows);
      continue;
    }
    
    // sanity check to see if we have correct column type
    // participating in left join. If not, then set the rowcount
    // of all histograms equal to the rowcount obtained so far
    // that is from an inner join + merging original left histogram
    // to the left histogram after inner join and use that to synchronize 
    // all histograms and return
    if (NOT(statOper == ITM_VEG_REFERENCE ||
            statOper == ITM_INSTANTIATE_NULL ||
            statOper == ITM_VALUEIDUNION ||
            statOper == ITM_UNPACKCOL ||
            statOper == ITM_NATYPE) )
    {
      CCMPASSERT ( "Incorrect column type participating in Join") ;
      // join result is not reliable any more
      jColStats->setFakeHistogram(TRUE);
      // don't assert for customers, synchronize histograms with
      // left rowcount and continue
      CostScalar oldCount = jColStats->getRowcount();
      if (oldCount != oJoinResultRows)
        jStatDesc->synchronizeStats (oldCount, oJoinResultRows);
      continue;
    }

    // if there is any issue with the joining histograms 
    // do not bother to 
    // --------------------------------------------------------
    // find the matching entry in the leftColStatsList....
    // It doesn't Have to be there, but it should be.
    // --------------------------------------------------------
    CollIndex matchPoint = 0;
    for (j = 0; j < origColStatsList.entries() ; j++)
    {
      ColStatDescSharedPtr oStatDesc = origColStatsList[j];
      ColStatsSharedPtr oColStats = oStatDesc->getColStats();
      
      // skip any histograms created for virtual columns, We do not
      // want to join on these. continue, so that the joined histogram
      // can be synchronized with the previous joined result
      if (oColStats->isVirtualColForHist() )
        continue;

      ItemExpr * originalExpr = oStatDesc->getVEGColumn().getItemExpr() ;
      OperatorTypeEnum originalOper = originalExpr->getOperatorType() ;

      // skip any histograms created for rowsets, We do not
      // want to join on these. continue, so that the joined histogram
      // can be synchronized with the previous joined result
      if (originalOper == ITM_ROWSETARRAY_SCAN) 
        continue;

      // sanity check
      if (NOT( originalOper == ITM_VEG_REFERENCE ||
              originalOper == ITM_INSTANTIATE_NULL ||
              originalOper == ITM_VALUEIDUNION ||
              originalOper == ITM_UNPACKCOL ||
              originalOper == ITM_NATYPE ))
      {
        CCMPASSERT ( "Incorrect expression participating in Join") ;
        // We found an incorrect column type in the list of histograms
        // assert for debug, and skip for release compiler
        continue;
      }
      if (statExpr == originalExpr) // ValueId comparison
      {
        if (statOper != originalOper)
        {
          CCMPASSERT( "Mismatched expressions being joined" ) ; // no reason this should fail
          // We found an incorrect column type in the list of histograms
          // assert for debug, and skip for release compiler
          continue;
        }
        goodMatch = TRUE;
        matchPoint = j;
        break ;
      }
    } // all entries of leftColStatsList

    if (goodMatch)
    {
      ColStatDescSharedPtr oStatDesc = origColStatsList[matchPoint];
      ColStatsSharedPtr oColStats = oStatDesc->getColStats();

      if ( inPredicate )
      {
        jStatDesc->mergeColStatDesc (oStatDesc, 
                                    LEFT_JOIN_OR_MERGE,
                                    FALSE, 
                                    REL_JOIN);
        CostScalar oldCount = jColStats->getRowcount();
        if (oldCount != oJoinResultRows)
        jStatDesc->synchronizeStats (oldCount, oJoinResultRows);
      }
      else
      {
        // apply change in selectivity from the predicate to
        // all histograms not in the predicate.  Only the
        // rowcount reduction factor is changed
        jStatDesc->synchronizeStats (baseRows,
        oJoinResultRows,
        ColStatDesc::DO_NOT_REDUCE_UEC) ;
      }
    }
    else
    {
      CCMPASSERT( "Can't find the left histogram to associate in outer join" ) ;
      // somehow can't find a left entry to associate... should
      // not happen, but if it does....
      CostScalar oldCount = jColStats->getRowcount();
      if (oldCount != oJoinResultRows)
      jStatDesc->synchronizeStats (oldCount, oJoinResultRows);
    }
  }  // for outerRefCount
}

// -----------------------------------------------------------------------
// This is a helper method for left joins. It is used to 
// NULL instantiate the right histogram rows from the other side with NULLs
// ------------------------------------------------------------------------
void
ColStatDescList::nullInstantiateHists(CollIndex startIndex, 
                                      CollIndex stopIndex,
                                      CostScalar &oJoinResultRows,
                                      ValueIdList &nulledVIds)
{
  // ------------------------------------------------------------
  // Next, Instantiate Nulls in histograms from the right table.
  // Also, alter the descriptions the histograms have of themselves
  // such that they indicate that they are null-instantiated.
  // ------------------------------------------------------------

  // Histograms from right table start from the outerRefCount index
  for (CollIndex i = startIndex; i < stopIndex; i++)
  {
    ColStatDescSharedPtr jStatDesc = (*this)[i];
    ColStatsSharedPtr jColStats = jStatDesc->getColStatsToModify();
    // before computing the number of NULLs to add to the histogram
    // apply any reduction factor to the intervals that is remaining
    jColStats->scaleHistogram(1);

    // Skip any histograms created for virtual columns
    if (jColStats->isVirtualColForHist())
      continue;

    // $$$ BETA_NOT_FCS
    // $$$ kludge to fix genesis case 10-980224-5150
    // $$$ (this should never happen!)
    if ( oJoinResultRows < jColStats->getRowcount() )
    {
      jStatDesc->synchronizeStats (jColStats->getRowcount(), // baseRows
                                    oJoinResultRows,              // newRows
                                    ColStatDesc::DO_NOT_REDUCE_UEC) ;
      jColStats = jStatDesc->getColStatsToModify() ;
    }
    // $$$ BETA_NOT_FCS

    jColStats->nullAugmentHistogram(oJoinResultRows);

    ItemExpr *pred = jStatDesc->VEGColumn().getItemExpr();

    for (CollIndex j=0; j < nulledVIds.entries() ; j++)
    {
      ItemExpr *nulledExpr = nulledVIds[j].getItemExpr();

      if (nulledExpr->getOperatorType() != ITM_INSTANTIATE_NULL)
      {
        CCMPASSERT(nulledExpr->getOperatorType() == ITM_INSTANTIATE_NULL);
        continue;
      }

      // NULL instantiate only those histograms which have not participated
      // in any join. The reason is that we do not keep original left histograms
      ValueId nulledValueId = nulledExpr->child(0).getValueId();
      if (pred->getValueId() == nulledValueId) 
      {
        // overwrite column-stats VEGColumn()'s valueId
        // with the valueId of the Instantiate Null
        jStatDesc->VEGColumn() = nulledVIds[j];

        // we need to update the merge state of these InstantiateNulls
        // because otherwise we will get into a rut when we hit the
        // ColStatDesc::mergeColStatDesc() -- similar to the situation
        // with ValueIdUnions, handled in Union::synthEstLogProp()
        // replace the base column from the merge state by null
        // instantiated column to reflect the histogram is now null
        // instantiated
        if (jStatDesc->mergeState().entries() ==1)
          jStatDesc->mergeState().clear();
        else
        {
          // remove the corresponding column from merge state and replace that
          // with the null instantiated one. Rest of the merge state is not impacted
          ValueIdSet mergeState = jStatDesc->mergeState();
          // merge state contains either base columns or null instantiated column
          // references. Hence if we don't find the nulledvalueid directly 
          // get the base column from it and compare that
          if ((!mergeState.contains(nulledValueId)) &&
              (nulledValueId.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE))
          {
            mergeState.removeCoveredVIdSet((ValueIdSet) nulledValueId); 
          }
          else
            mergeState.remove(nulledValueId);
          jStatDesc->mergeState().clear();
          jStatDesc->mergeState().insert(mergeState);
        }
        jStatDesc->mergeState().insert( nulledExpr->getValueId() ) ;

        break ;
        // what, if anything should be done with the raw column
        // list associated with this col stat desc??  cmf
      }
    } // for j - nullVIds
  } // for i - outerRefCount -> joinStatDescList
}

ULng32
MultiColumnSkewedValueLists::HashFunction (const ValueIdList & input)
{
  ULng32 retval = 1 + input.entries();

  for (CollIndex i=0; i < input.entries(); i++)
  {
    retval += (CollIndex) input[i];  // add up the ValueId's
  }

  return retval ;
}

MultiColumnSkewedValueLists::MultiColumnSkewedValueLists () :	
     HASHDICTIONARY(ValueIdList,MCSkewedValueList) (&(MultiColumnSkewedValueLists::HashFunction),
                                            5,
                                            TRUE,
                                            HISTHEAP )
     { };

// -----------------------------------------------------------------------
// MultiColumnSkewedValueLists :: constructor
//
// This class is modeled after  MultiColumnUecList class and mirrors its 
// interfaces and usage
// -----------------------------------------------------------------------
MultiColumnSkewedValueLists::MultiColumnSkewedValueLists (const StatsList   & initStats,
                                        const ValueIdList & tableColumns) :
     HASHDICTIONARY(ValueIdList,MCSkewedValueList) (&(MultiColumnSkewedValueLists::HashFunction),
                                            5,
                                            TRUE,
                                            HISTHEAP )
{
  // loop through the list of NAColumnArray's
  for ( CollIndex i = 0; i < initStats.groupUecColumns_.entries(); i++ )
  {
    const NAColumnArray & cols = initStats.groupUecColumns_[i];

    if(cols.entries() == 1)
      continue;

    ValueIdList groupCols;
    for ( CollIndex j = 0; j < cols.entries(); j++ )
    {
	  Lng32 position = cols[j]->getPosition();
	  const ValueId & id = tableColumns[position];
	  groupCols.insert( id );
    }

    ValueIdList * key  = new (HISTHEAP) ValueIdList( groupCols );
    MCSkewedValueList * value = new (HISTHEAP) MCSkewedValueList (initStats.groupMCSkewedValueLists_[i], HISTHEAP);
    insert( key, value );
  }
}

const MCSkewedValueList * MultiColumnSkewedValueLists::getMCSkewedValueList(ValueIdSet columns, ValueIdList &colGroup)
{
  ValueIdList * keyEntry = NULL;
  MCSkewedValueList * valueEntry = NULL;

  CollIndex noOfCols = columns.entries();

  // we need to iterate through all entries in this list
  MultiColumnSkewedValueListsIterator iter( *this );

  for ( iter.getNext( keyEntry, valueEntry );
	keyEntry != NULL && valueEntry != NULL;
	iter.getNext( keyEntry, valueEntry ) )
  {
    if(noOfCols != (*keyEntry).entries())
      continue;

    if(columns == ValueIdSet(*keyEntry))
    {
      colGroup = *keyEntry;     
      return valueEntry;
    }
  }
  return NULL;
}

const MCSkewedValueList * ColStatDescList::getMCSkewedValueListForCols(ValueIdSet inputCols, ValueIdList &colGroup)
{
  if(mcSkewedValueLists_)
    return mcSkewedValueLists()->getMCSkewedValueList(inputCols, colGroup);
  else
    return NULL;
}

CostScalar ColStatDescList::getAvgRowcountForNonSkewedMCValues(ValueIdSet cols, MCSkewedValueList* mCSkewedValueList)
{
  CostScalar avgRowcountForNonSkewedMCValues = 1;  
  if(mCSkewedValueList && getUecList())
  { 
    CostScalar mcUecForCols = getUecList()->lookup(cols);
    CollIndex noOfMCSkewValues = mCSkewedValueList->entries();
    if(mcUecForCols - noOfMCSkewValues > csZero)
    {
      CostScalar totalRowcountOfMCSkewedValues;
      for(CollIndex i=0; i<noOfMCSkewValues; i++)
        totalRowcountOfMCSkewedValues += mCSkewedValueList->at(i)->getFrequency();

      CostScalar totalRowcount = (*this)[0]->getColStats()->getRowcount();    
      avgRowcountForNonSkewedMCValues = (totalRowcount - totalRowcountOfMCSkewedValues)/(mcUecForCols - noOfMCSkewValues);
    }
  }
  return avgRowcountForNonSkewedMCValues;
}

// eof
