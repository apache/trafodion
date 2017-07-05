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
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      10/30/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "ex_stdh.h"
#include "key_range.h"
#include "key_mdam.h"
#include "ex_mdam.h"
// #include "exp_clause_derived.h"


MdamPredIterator::MdamPredIterator(MdamColumn * first,Lng32 maxDisjunctNumber)
	: currentDisjunctNumber_(-1),
	  maxDisjunctNumber_(maxDisjunctNumber),
	  returnedAPred_(FALSE)
{
for (MdamColumn * c = first; c != 0; c = c->nextColumn())
  {
    // initialize state in MdamColumn objects used by MdamPredIterator
    c->initCurrentPred();
  }
}; 
 

Lng32 MdamPredIterator::getNextDisjunctNumber()
{
  Lng32 rc = -1;  // assume no more disjuncts

  if (currentDisjunctNumber_ < maxDisjunctNumber_) 
    {
      // increment disjunct number and return
      currentDisjunctNumber_++;
      rc = currentDisjunctNumber_;
    }
  
  return rc;
}

NABoolean MdamPredIterator::positionToNextOr(MdamPred **currentPred)
{
  NABoolean rc = FALSE;  // assume no more Or groups
  MdamPred *p = *currentPred;

  if (p)
    {
      while ((p) && 
	     (p->getDisjunctNumber() < currentDisjunctNumber_) &&
	     (!(p->firstInOrGroup())))
	{
	  p = p->getNext();
	}
      *currentPred = p;
      if ((p) && (p->getDisjunctNumber() == currentDisjunctNumber_))
	rc = TRUE;
    }

  returnedAPred_ = FALSE;

  return rc;
}

MdamPred * MdamPredIterator::getNextPred(MdamPred **currentPred)
{
  MdamPred *rc = *currentPred;

  if (rc)
    {
      // if the current predicate belongs to the current disjunct,
      // and it is in the current OR group, return it
      if ((rc->getDisjunctNumber() == currentDisjunctNumber_) &&
	  (!(returnedAPred_ && rc->firstInOrGroup())))
	{
	  *currentPred = rc->getNext();
	  returnedAPred_ = TRUE;
	}
      else  // otherwise indicate no more predicates in this OR group
	{
	  rc = 0;
	}
    }
  
  return rc;
}

// Position the predicate list to the current disjunct.
void MdamPredIterator::positionToCurrentDisjunct(MdamPred **currentPred)
{
  MdamPred *p = *currentPred;
  while((p) && 
        (p->getDisjunctNumber() < currentDisjunctNumber_))
    {
      p = p->getNext();
    }
  *currentPred = p;
}
  
MdamColumn::MdamColumn(MdamColumn * previous,
		       MdamColumnGen *columnGenInfo,
		       ex_globals *glob,
		       sql_buffer_pool *pool,
		       atp_struct *atp0,
		       atp_struct *workAtp,
		       unsigned short valueAtpIndex,
                       const ex_tcb *tcb) : 

     #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
     intervals_(5),
     tentative_intervals_(6),
     #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */
     next_(0),previous_(previous),
     interval_iterator_(intervals_),
     current_interval_(0),
     current_is_subset_interval_(FALSE),
     currentValue_(0),
     traversal_in_progress_(FALSE),
     useSparseProbes_(columnGenInfo->useSparseProbes()),
     lastProductiveFetchRangeCounter_(0),
     sparseProbeNeeded_(FALSE),
     sparseProbeSucceeded_(FALSE),
     sparseProbeFailed_(FALSE),
     columnGenInfo_(columnGenInfo),
     currentPred_(0),
     glob_(glob)
{

  // link this column to the one before it
  if (previous)
    {
      previous->setNextColumn(this);
    }

  // allocate storage to hold current, hi, lo, nonNullHi, nonNullLo values
#pragma nowarn(1506)   // warning elimination 
  pool->get_free_tuple(currentValue_,columnGenInfo_->getLength());
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
  pool->get_free_tuple(hi_,columnGenInfo_->getLength());
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
  pool->get_free_tuple(lo_,columnGenInfo_->getLength());
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
  pool->get_free_tuple(nonNullHi_,columnGenInfo_->getLength());
#pragma warn(1506)  // warning elimination 
#pragma nowarn(1506)   // warning elimination 
  pool->get_free_tuple(nonNullLo_,columnGenInfo_->getLength());
#pragma warn(1506)  // warning elimination 

  // go tell MdamColumnGen to fix up its expressions
  // (aside:  too bad we have to do it here -- if there were fixup
  // methods on tdb's we would only have to fix up once.  The disadvantage
  // of doing it here is that there may be many MdamColumn's per
  // MdamColumnGen -- just as there may be many tcb's per tdb.  So,
  // we call fixup for each MdamColumn, more times than necessary.)
  columnGenInfo_->fixup(0,0,glob->getSpace(),glob->getDefaultHeap(), tcb);

  // evaluate expressions to compute hi, lo, nonNullHi, nonNullLo values
  workAtp->getTupp(valueAtpIndex) = hi_;
  (void)(columnGenInfo_->getHiExpr()->eval(atp0,workAtp));
  workAtp->getTupp(valueAtpIndex) = lo_;
  (void)(columnGenInfo_->getLoExpr()->eval(atp0,workAtp));
  workAtp->getTupp(valueAtpIndex) = nonNullHi_;
  (void)(columnGenInfo_->getNonNullHiExpr()->eval(atp0,workAtp));
  workAtp->getTupp(valueAtpIndex) = nonNullLo_;
  (void)(columnGenInfo_->getNonNullLoExpr()->eval(atp0,workAtp));
}

MdamColumn::~MdamColumn()
{
  // Note that we assume that keyMdamEx::~keyMdamEx() is the only
  // procedure that calls this procedure and that it deleted the
  // intervals.  Hence there is no call to MdamColumn::release() here.
}

void MdamColumn::setNextColumn(MdamColumn *next)
{
next_ = next;
}

// LCOV_EXCL_START
MdamIntervalList & MdamColumn::getIntervalList()
{
return intervals_;
}
// LCOV_EXCL_STOP

NABoolean MdamColumn::initNextValue()
{
traversal_in_progress_ = TRUE;
current_interval_ = 0;

sparseProbeNeeded_ = FALSE;
sparseProbeSucceeded_ = FALSE;
sparseProbeFailed_ = FALSE;

return TRUE;
}

//  Note:  In MdamColumn::getNextValue(), the beginValue and endValue
//  parameters are output only. This method doesn't make any assumptions
//  about their input values (and should not, since we may have done
//  nasty things such as complement them in place).

MdamColumn::getNextValueReturnType MdamColumn::getNextValue(
     ULng32 productiveFetchRangeCounter,
     char *beginValue,  // out only!
     char *endValue,    // out only!
     short &beginExclFlag,
     short &endExclFlag,
     FixedSizeHeapManager & mdamRefListEntryHeap)
{
  getNextValueReturnType rc = TRAVERSE_UP; // assume failure
  NABoolean found_value = FALSE;

  beginValue = beginValue + columnGenInfo_->getOffset();
  endValue = endValue + columnGenInfo_->getOffset();

  while ((traversal_in_progress_) && (!found_value))
    {
      // if we have a current interval and it is not a subset interval
      if ((current_interval_) && (!current_is_subset_interval_))
	{
	  // look for the next value in this interval (if any)

	  char * cv = currentValue_.getDataPointer();
	  ULng32 len = columnGenInfo_->getLength();
	 	
	  // if the Optimizer told us to do sparse probes, or if the
	  // last dense probe didn't result in any fetch ranges with
	  // rows, do a sparse probe

	  if ((useSparseProbes_) ||
	      (lastProductiveFetchRangeCounter_ == 
	       productiveFetchRangeCounter))
	    {
	      ex_assert(sparseProbeNeeded_ == FALSE,
			"MdamColumn::getNextvalue called at wrong time.");
	      if (sparseProbeSucceeded_)
		{
		  // a successful sparse probe completed, and has set
		  // currentValue_ -- use that value now
		  sparseProbeSucceeded_ = FALSE;

		  found_value = TRUE;
		  str_cpy_all(beginValue,cv,(Int32)len);
		  str_cpy_all(endValue,cv,(Int32)len);
		  rc = TRAVERSE_DOWN;

		  ex_assert(sparseProbeFailed_ == FALSE,
			    "MdamColumn::getNextValue probe flags bad.");
		}
	      else if (sparseProbeFailed_)
		{
		  // a sparse probe was done, but it returned no data --
		  // that means there are no more values in this interval
		  sparseProbeFailed_ = FALSE;
		}
	      else // need to do a sparse probe now
		{
		  found_value = TRUE;
		  str_cpy_all(beginValue,cv,(Int32)len);

		  MdamPoint *endpt = 
		    current_interval_->getPointPtr(MdamEnums::MDAM_END);

		  str_cpy_all(endValue,endpt->getDataPointer(),(Int32)len);

		  // set exclusion flags
		  beginExclFlag = 1;  // exclude the current value
		  endExclFlag = 0;
		  if (endpt->getInclusion() == MdamEnums::MDAM_EXCLUDED)
		    endExclFlag = 1;

		  rc = PROBE;		  
		  sparseProbeNeeded_ = TRUE;
		}
	    }
	  else // use dense probes
	    {
	      if (current_interval_->getNextValue(len,cv))
		{
		  // we found the next value in this interval
		  lastProductiveFetchRangeCounter_ = 
		    productiveFetchRangeCounter;
		  found_value = TRUE;
		  str_cpy_all(beginValue,cv,(Int32)len);
		  str_cpy_all(endValue,cv,(Int32)len);
		  rc = TRAVERSE_DOWN;
		}
	    }
	}

      if (!found_value)
	{
	  // either there is no current interval, or the current interval
	  // is a subset interval, or there are no more values in the 
	  // current interval -- in all three of these cases, the next
	  // thing to do is to find the next interval

	  current_is_subset_interval_ = FALSE;  
	  if (current_interval_ == 0)
	    {
	      // must be we are looking for the first such interval
	      interval_iterator_.init();
	    }
	  current_interval_ = interval_iterator_();

	  if (previous_)
	    {
	      // find the next interval whose ref list has a non-empty
	      // intersection with the current_context_ of the previous
	      // column
	      while ((current_interval_ != 0) && 
		     (previous_->current_context_.intersectEmpty(
			  *(current_interval_->getRefListPtr()))))
		 {
		   current_interval_ = interval_iterator_();
		 }	  
	    }

	  if (current_interval_ == 0)
	    {
	      // out of intervals -- traverse up
	      traversal_in_progress_ = FALSE;
	      rc = TRAVERSE_UP;
	      current_context_.deleteEntries(mdamRefListEntryHeap);
	    }
	  // notice that the next test sets current_is_subset_interval_
	  // as a side effect...

          // The test in the following "if" was changed to fix Genesis
          // case CR #10-010204-0980. Formerly, we just intersected the
          // stop list with the current interval ref list; but we also
          // need to intersect the stop list with the previous column's
          // current context as well (if there is a previous column).

	  // When a previous column exists we should intersect all three
	  // lists - (1) stop list with the current interval refList
	  //         (2) stop list with the previous column's current context.
	  //         (3) previous column's current context and current interval refList.
	  // (3) was not covered as part of the fix for case CR #10-010204-098.
	  // This is a fix for Solution ID: 10-040108-2266.

          // else if this is a subset interval...
	  else if (current_is_subset_interval_ = 
		   (previous_ ? 
		    !stop_list_.intersectEmpty(previous_->current_context_, 
						*(current_interval_->getRefListPtr()))
		    : (!stop_list_.intersectEmpty(*(current_interval_->getRefListPtr())))
		    )
		   )
	      {
	      // we are at a subset interval
	      MdamPoint *beginpt = 
		current_interval_->getPointPtr(MdamEnums::MDAM_BEGIN);
	      MdamPoint *endpt = 
		current_interval_->getPointPtr(MdamEnums::MDAM_END);
	      ULng32 len = columnGenInfo_->getLength();

	      str_cpy_all(beginValue,beginpt->getDataPointer(),(Int32)len);
	      str_cpy_all(endValue,endpt->getDataPointer(),(Int32)len);

	      beginExclFlag = 0;
	      if (beginpt->getInclusion() == MdamEnums::MDAM_EXCLUDED)
		beginExclFlag = 1;
	      endExclFlag = 0;
	      if (endpt->getInclusion() == MdamEnums::MDAM_EXCLUDED)
		endExclFlag = 1;

	      found_value = TRUE;
	      rc = SUBSET;
	    }
	  else // not a subset interval
	    {
	      // find the first value in the interval (note that it 
	      // is possible that there might not be one, in which
	      // case we will go around the while loop again and 
	      // look for another interval)
	      char * cv = currentValue_.getDataPointer();
	      ULng32 len = columnGenInfo_->getLength();

	      if (current_interval_->getFirstValue(len,cv))
		{
		  // the interval has a first value -- return it
		  found_value = TRUE;
		  str_cpy_all(beginValue,cv,(Int32)len);

		  // set context so we traverse only to appropriate
		  // intervals in the next column to the right
		  if (previous_)
		    {
		      MdamRefList & intervalRefList = 
			*(current_interval_->getRefListPtr());
		      
		      current_context_.intersect(intervalRefList,
						 previous_->current_context_,
						 mdamRefListEntryHeap);
		    }
		  else
		    {
		      MdamRefList & intervalRefList = 
			*(current_interval_->getRefListPtr());

		      current_context_.deleteEntries(mdamRefListEntryHeap);
		      current_context_.copyEntries(intervalRefList,
						   mdamRefListEntryHeap);
		    }

		  if (useSparseProbes_)
		    {
		      // do a sparse probe to find first value in interval
		      MdamPoint *endpt = 
			current_interval_->getPointPtr(MdamEnums::MDAM_END);

		      str_cpy_all(endValue,endpt->getDataPointer(),(Int32)len);

		      // set exclusion flags
		      beginExclFlag = 0;  // include the current value
		      endExclFlag = 0;
		      if (endpt->getInclusion() == MdamEnums::MDAM_EXCLUDED)
			endExclFlag = 1;
		      
		      rc = PROBE;		  
		      sparseProbeNeeded_ = TRUE;		      
		    }
		  else  // dense probes
		    {
		      // use this value and traverse down now
		      str_cpy_all(endValue,cv,(Int32)len);	
		      rc = TRAVERSE_DOWN;
		    }

		}
	    }
	}
    }
  return rc;
}


void MdamColumn::reportProbeResult(char *keyData)
{
ex_assert(sparseProbeNeeded_,
	  "MdamColumn::reportProbeResult called unexpectedly.");

sparseProbeNeeded_ = FALSE;
if (keyData)
  {
    // the sparse probe was successful -- save the key value
    Int32 len = Int32(columnGenInfo_->getLength());
#pragma nowarn(1506)   // warning elimination 
    Lng32 offset = columnGenInfo_->getOffset();
#pragma warn(1506)  // warning elimination 
    char * cv = currentValue_.getDataPointer();

    str_cpy_all(cv,keyData + offset,len);

    sparseProbeSucceeded_ = TRUE;
  }
else
  {
    // the sparse probe returned no data
    sparseProbeFailed_ = TRUE;
  }

}

void MdamColumn::completeKey(char *bktarget,
			     char *ektarget,
			     short bkexcl,
			     short ekexcl)
{
  Int32 len = Int32(columnGenInfo_->getLength());
  char *extremalValue;

  bktarget = bktarget + columnGenInfo_->getOffset();
  ektarget = ektarget + columnGenInfo_->getOffset();

  if (bkexcl)
    // use hi value if begin key is excluded
    extremalValue = hi_.getDataPointer();  
  else
    // use lo value if begin key is included
    extremalValue = lo_.getDataPointer(); 
  
  str_cpy_all(bktarget,extremalValue,len);
  
  if (ekexcl)
    // use lo value if end key is excluded
    extremalValue = lo_.getDataPointer(); 
  else
    // use hi value if end key is included
    extremalValue = hi_.getDataPointer(); 
  
  str_cpy_all(ektarget,extremalValue,len);
};



NABoolean MdamColumn::buildDisjunct(MdamPredIterator & predIterator,
                                    sql_buffer_pool *pool,
                                    atp_struct *atp0,
                                    atp_struct *workAtp,
                                    unsigned short valueAtpIndex,
                                    Lng32 disjunct_number,
                                    NABoolean disjunct_number_in_stop_list,
                                    FixedSizeHeapManager & mdamIntervalHeap,
                                    FixedSizeHeapManager & mdamRefListEntryHeap,
                                    FixedSizeHeapManager & 
                                    mdamRefListEntrysForStopListsHeap,
                                    Lng32 & dataConvErrorFlag)
{
NABoolean rc = disjunct_number_in_stop_list;
NABoolean got_a_predicate = FALSE;


// Note that we defer supplying the ref list for now

MdamInterval * accumulator = new(mdamIntervalHeap)
    MdamInterval(lo_,
                 MdamEnums::MDAM_INCLUDED,
                 hi_,
                 MdamEnums::MDAM_INCLUDED); 
tentative_intervals_.append(accumulator);


// Advance the predicate list to the current disjunct, if necessary.
// This will cause any left-over predicates that were not processed due
// to an earlier conflict to be skipped.
// Case number 10-971205-3848.
predIterator.positionToCurrentDisjunct(&currentPred_);


// Loop over the OR groups.
while ((predIterator.positionToNextOr(&currentPred_)) && 
       (!tentative_intervals_.isEmpty()))
  {
    #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
    MdamIntervalList or_result(3);
    #else
    MdamIntervalList or_result;
    #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */

    MdamPred * predPtr;

    // Loop over the predicates within an OR group.
    while (predPtr = predIterator.getNextPred(&currentPred_))
      {
        // build an interval representing this predicate, if possible

        MdamInterval * or_accumulator = 0;
    
        tupp & pv  = workAtp->getTupp(valueAtpIndex);
        tupp & pv2 = workAtp->getTupp(valueAtpIndex+1);
#pragma nowarn(1506)   // warning elimination 
        pool->get_free_tuple(pv,columnGenInfo_->getLength());
        if (predPtr->getPredType() == MdamPred::MDAM_BETWEEN)
          pool->get_free_tuple(pv2,columnGenInfo_->getLength());
#pragma warn(1506)  // warning elimination 
    
        got_a_predicate = TRUE;
        dataConvErrorFlag = 0;  // The zero hard-coded here should be
                                // ex_conv_clause::CONV_RESULT_OK in
                                // file exp/exp_clause_derived.h.
        // The call to getValue sets dataConvErrorFlag. We use it twice in the
        // case of MDAM_BETWEEN, copying the values to other local variables.
        ex_expr::exp_return_type errorCode = predPtr->getValue(atp0,workAtp);

        Int32 dcErrFlag1 = dataConvErrorFlag;
        Int32 dcErrFlag2 = 0;
        if (errorCode == ex_expr::EXPR_OK &&
            predPtr->getPredType() == MdamPred::MDAM_BETWEEN)
          {
            dataConvErrorFlag = 0;
            errorCode = predPtr->getValue2(atp0,workAtp);
            dcErrFlag2 = dataConvErrorFlag;
          }

        MdamPred::MdamPredType predType = MdamPred::MDAM_RETURN_FALSE;
        // Next 2 used only for MDAM_BETWEEN.
        MdamEnums::MdamInclusion startInclusion = predPtr->getStartInclusion();
        MdamEnums::MdamInclusion endInclusion   = predPtr->getEndInclusion();
        if (errorCode == ex_expr::EXPR_OK)
          predType = predPtr->getTransformedPredType(dcErrFlag1, dcErrFlag2,
                                                     startInclusion, endInclusion);

        // The switch statement below implements the following mapping...
        //   MDAM_EQ -> [pv,pv]
        //   MDAM_LE -> [nonNullLo, pv]
        //   MDAM_LT -> [nonNullLo, pv)
        //   MDAM_GE -> [pv, nonNullHi]
        //   MDAM_GT -> (pv,nonNullHi]
        //   MDAM_BETWEEN -> [pv,pv2]
        //   MDAM_ISNULL -> [hi,hi]
        //   MDAM_ISNULL_DESC -> [lo,lo]
        //   MDAM_ISNOTNULL -> [nonNullLo,nonNullHi]
        //   MDAM_RETURN_FALSE -> no interval
        switch (predType)
          {
          case MdamPred::MDAM_EQ:
            {
              or_accumulator = new(mdamIntervalHeap)
                      MdamInterval(pv,
                                   MdamEnums::MDAM_INCLUDED,
                                   pv,
                                   MdamEnums::MDAM_INCLUDED);
              break;
            }
          case MdamPred::MDAM_LE:
            {
              or_accumulator = new(mdamIntervalHeap)
                      MdamInterval(nonNullLo_,
                                 MdamEnums::MDAM_INCLUDED,
                                 pv,
                                 MdamEnums::MDAM_INCLUDED);
              break;
            }
          case MdamPred::MDAM_LT:
            {
              or_accumulator = new(mdamIntervalHeap)
                      MdamInterval(nonNullLo_,
                                   MdamEnums::MDAM_INCLUDED,
                                   pv,
                                   MdamEnums::MDAM_EXCLUDED);
              break;
            }
          case MdamPred::MDAM_GE:
            {
              or_accumulator = new(mdamIntervalHeap)
                      MdamInterval(pv,
                                   MdamEnums::MDAM_INCLUDED,
                                   nonNullHi_,
                                   MdamEnums::MDAM_INCLUDED);
              break;
            }
          case MdamPred::MDAM_GT:
            {
              or_accumulator = new(mdamIntervalHeap)
                      MdamInterval(pv,
                                   MdamEnums::MDAM_EXCLUDED,
                                   nonNullHi_,
                                   MdamEnums::MDAM_INCLUDED);
              break;
            }
          case MdamPred::MDAM_BETWEEN:
            {
              // If the predicate is on a descending key column, switch the
              // endpoints.
              if (predPtr->reverseEndpoints())
                or_accumulator = new(mdamIntervalHeap)
                      MdamInterval(pv2,
                                   endInclusion,
                                   pv,
                                   startInclusion);
              else
                or_accumulator = new(mdamIntervalHeap)
                      MdamInterval(pv,
                                   startInclusion,
                                   pv2,
                                   endInclusion);
              break;
            }
          case MdamPred::MDAM_ISNULL:  // IS NULL predicate on ASC column
            {
              // if the column is nullable and ASC, we know that hi_
              // is the NULL value
              or_accumulator = new(mdamIntervalHeap)
                      MdamInterval(hi_,
                                   MdamEnums::MDAM_INCLUDED,
                                   hi_,
                                   MdamEnums::MDAM_INCLUDED);
              break;
            }
          case MdamPred::MDAM_ISNULL_DESC:  // IS NULL predicate on DESC column
            {
              // if the column is nullable and DESC, we know that lo_
              // is the NULL value
              or_accumulator = new(mdamIntervalHeap)
                      MdamInterval(lo_,
                                   MdamEnums::MDAM_INCLUDED,
                                   lo_,
                                   MdamEnums::MDAM_INCLUDED);
              break;
            }
          case MdamPred::MDAM_ISNOTNULL:
            {
              or_accumulator = new(mdamIntervalHeap)
                      MdamInterval(nonNullLo_,
                                   MdamEnums::MDAM_INCLUDED,
                                   nonNullHi_,
                                   MdamEnums::MDAM_INCLUDED);
              break;
            }
          case MdamPred::MDAM_RETURN_FALSE:
            {
              // The predicate cannot be satisfied so no interval is created.
              break;
            }
          default:
            {
              ex_assert(0,"Invalid predicate type"); //LCOV_EXCL_LINE
              break;
            }
          }

        // release tupp_descriptor assigned to pv now that we are done
        // with pv
        pv.release();  
        
        // OR in the interval just obtained into or_result
        if (or_accumulator)
          {
            if (or_result.isEmpty())
              {
                or_result.append(or_accumulator);
              }
            else
              {
                #if defined ( NA_MDAM_EXECUTOR_DEBUG_ILTF )
                MdamIntervalList or_temp1(4);
                #else
                MdamIntervalList or_temp1;
                #endif /* NA_MDAM_EXECUTOR_DEBUG_ILTF */
                or_temp1.append(or_accumulator);
                or_result.unionSameDisjunct(or_temp1,
                                            columnGenInfo_->getLength(),
                                            mdamIntervalHeap,
                                            mdamRefListEntryHeap);
                or_temp1.deleteAllIntervals(mdamIntervalHeap,
                                            mdamRefListEntryHeap);
              }
          }
      }

    // AND the OR result to the accumulator
    tentative_intervals_.intersect(or_result,
                                   columnGenInfo_->getLength(),
                                   mdamIntervalHeap,
                                   mdamRefListEntryHeap);
    or_result.deleteAllIntervals(mdamIntervalHeap,
                                 mdamRefListEntryHeap);
  }

if (((got_a_predicate) && (!rc)) ||
    ((previous_ == 0) && (!rc)))
  {
    // this is the last key column that has a predicate
    // for this disjunct, or there is no key column having
    // a predicate for this column (in which case one wonders
    // why MDAM was picked!) -- add the disjunct number to the
    // stop list for this key column
    stop_list_.insert((Int32)disjunct_number,mdamRefListEntrysForStopListsHeap);
    rc = TRUE;
  }

return rc;
}


NABoolean MdamColumn::disjunctIsEmpty()
{
return this->tentative_intervals_.isEmpty();
}


void MdamColumn::tossDisjunct(FixedSizeHeapManager & mdamIntervalHeap,
                              FixedSizeHeapManager & mdamRefListEntryHeap)
{
  tentative_intervals_.deleteAllIntervals(mdamIntervalHeap,
                                          mdamRefListEntryHeap);
}


void MdamColumn::mergeDisjunct(Lng32 disjunct_number,
			       FixedSizeHeapManager & mdamIntervalHeap,
			       FixedSizeHeapManager & mdamRefListEntryHeap)
{
  intervals_.unionSeparateDisjuncts(tentative_intervals_,
				    (Int32)disjunct_number,
				    columnGenInfo_->getLength(),
				    mdamIntervalHeap,
				    mdamRefListEntryHeap);
  tentative_intervals_.deleteAllIntervals(mdamIntervalHeap,
                                          mdamRefListEntryHeap);
}


void MdamColumn::releaseNetwork(FixedSizeHeapManager & mdamIntervalHeap,
                                FixedSizeHeapManager & mdamRefListEntryHeap)
{
  intervals_.deleteAllIntervals(mdamIntervalHeap,
                                mdamRefListEntryHeap);

  // shouldn't need to do this on tentative_intervals_ since
  // mergeDisjunct() or tossDisjunct() should have already done it,
  // but we do this anyway just to be robust
  tentative_intervals_.deleteAllIntervals(mdamIntervalHeap,
                                          mdamRefListEntryHeap);

  // Delete the ref list entries for current_context_.
  current_context_.deleteEntries(mdamRefListEntryHeap);
}


void MdamColumn::release(FixedSizeHeapManager & mdamRefListEntrysForStopListsHeap)
{
  stop_list_.deleteEntries(mdamRefListEntrysForStopListsHeap);
}


keyMdamEx::keyMdamEx(const keyRangeGen & keyRangeGen,
		     const short in_version,
		     sql_buffer_pool *pool,
		     ex_globals *glob,
                     const ex_tcb *tcb)
     : keyRangeEx(keyRangeGen,glob,in_version,pool),
       network_built_(FALSE),
       stop_lists_built_(FALSE),
       current_column_(0),
       current_column_index_(-1),
       productiveFetchRangeCounter_(0),
       mdamRefListEntryHeap_(sizeof(MdamRefListEntry),
			     getGenInfo().getMaxMdamRefs()),
       mdamRefListEntrysForStopListsHeap_(sizeof(MdamRefListEntry),
			     getGenInfo().getMaxMdamRefsForStopLists()),
       mdamIntervalHeap_(sizeof(MdamInterval),
                         getGenInfo().getMaxMdamIntervals()),
       complementKeysBeforeReturning_(
	    getGenInfo().complementKeysBeforeReturning())
{
  number_of_key_cols_ = 0;
  first_column_ = last_column_ = 0;

  const keyMdamGen & keyM = (const keyMdamGen &)keyRangeGen;

  for (MdamColumnGen *cg = keyM.getFirst(); 
       cg != 0;
       cg = cg->getNext())
    {
      last_column_ =
	new(glob->getSpace()) MdamColumn(last_column_,
					 cg,
					 glob,
					 pool,
					 workAtp_, // $$$ will this work?
					 workAtp_,
					 keyM.getValueAtpIndex(), tcb);
      if (first_column_ == 0)
	first_column_ = last_column_;
      number_of_key_cols_++;
    }
  NABoolean refListEntrysForStopListsHeapAcquired
    = mdamRefListEntrysForStopListsHeap_.acquireHeapMemory(
                                              globals_->getDefaultHeap());
}


void keyMdamEx::destroyNetwork()
{
  if (network_built_)
    {
      for (MdamColumn * c = first_column_; c != 0; c = c->nextColumn())
	{
	  c->releaseNetwork(mdamIntervalHeap_,
                            mdamRefListEntryHeap_);
	}
      network_built_ = FALSE;
    }

  // reset tree traversal variables so any attempt to traverse will
  // return end-of-traverse
  current_column_ = 0;
  current_column_index_ = -1;

  // Release memory for MdamInterval's and MdamRefListEntry's.
  mdamRefListEntryHeap_.releaseHeapMemory();
  mdamIntervalHeap_.releaseHeapMemory();
}


ExeErrorCode keyMdamEx::buildNetwork(sql_buffer_pool *pool,atp_struct *atp0) 
{
  destroyNetwork();  // clean out any old network still hanging around

  keyMdamGen & mdamGen = (keyMdamGen &)tdbKey_;

  // Acquire memory for MdamRefListEntry's.
  ExeErrorCode refListEntryHeapErrorCode
    = mdamRefListEntryHeap_.acquireHeapMemory(globals_->getDefaultHeap());
  if (refListEntryHeapErrorCode != (ExeErrorCode)0)
    return refListEntryHeapErrorCode;

  // Acquire memory for MdamInterval's.
  ExeErrorCode intervalHeapErrorCode
    = mdamIntervalHeap_.acquireHeapMemory(globals_->getDefaultHeap());
  if (intervalHeapErrorCode != (ExeErrorCode)0)
    return intervalHeapErrorCode;

  MdamPredIterator predIterator(first_column_,
                                mdamGen.getMaxDisjunctNumber());

  Lng32 disjunct_number;
  while ((disjunct_number = predIterator.getNextDisjunctNumber()) >= 0)
    {
      NABoolean disjunct_number_in_stop_list = stop_lists_built_;
      NABoolean empty_disjunct = FALSE;
      Lng32 column_number = number_of_key_cols_-1; // $$$ remove later

      for (MdamColumn * m = last_column_;
        	        m != 0;
	                m = m->previousColumn())
        {
          ex_assert(column_number >= 0,
		    "keyMdamEx::Mdam -- bad column_number"); // $$$ remove later

          disjunct_number_in_stop_list = 
            m->buildDisjunct(predIterator,
		             pool,
			     atp0,
			     workAtp_,
			     getGenInfo().getValueAtpIndex(),
			     disjunct_number,
			     disjunct_number_in_stop_list,
			     mdamIntervalHeap_,
			     mdamRefListEntryHeap_,
			     mdamRefListEntrysForStopListsHeap_,
                             dataConvErrorFlag_);

          if (m->disjunctIsEmpty())
	    empty_disjunct = TRUE;
	  
	  column_number--; // $$$ remove later
        }  // end for

      ex_assert(column_number == -1,
                "keyMdamEx::Mdam -- wrong number of columns"); // $$$ remove later
      ex_assert(disjunct_number_in_stop_list,
	        "keyMdamEx::Mdam -- disjunct number not in any stop list");

      // if the disjunct is empty for some column (i.e. if some column
      // has contradictory predicates for this disjunct), throw the disjunct
      // away; otherwise merge it in
      if (empty_disjunct)
        {
          for (MdamColumn * c = first_column_;
	                    c != 0;
		            c = c->nextColumn())
            {
	      c->tossDisjunct(mdamIntervalHeap_,
                              mdamRefListEntryHeap_);
            }
        }
      else
	{
	  for (MdamColumn * c = first_column_;
		            c != 0;
		            c = c->nextColumn())
            {
	      c->mergeDisjunct(disjunct_number,
			       mdamIntervalHeap_,
			       mdamRefListEntryHeap_);
            }
	}
  }  // end while
  network_built_ = TRUE;
  stop_lists_built_ = TRUE;

  // for testing... display the network
  #if defined ( NA_MDAM_EXECUTOR_DEBUG )
  print();
  #endif /* NA_MDAM_EXECUTOR_DEBUG */
 
  return (ExeErrorCode)0;  // No error.
}


keyMdamEx::~keyMdamEx()
{
  destroyNetwork();

  // Delete the MdamColumn*s.
  MdamColumn * current;
  MdamColumn * next;
  for (current = first_column_;
       current != 0; 
       current = next)
    {
      current->release(mdamRefListEntrysForStopListsHeap_);
      next = current->nextColumn();
      delete current;
    }

  // Release memory for the stop list ref list entires.
  // stop_list_.deleteEntries(mdamRefListEntryHeap);
  mdamRefListEntrysForStopListsHeap_.releaseHeapMemory();
}


ExeErrorCode keyMdamEx::initNextKeyRange(sql_buffer_pool *pool,
			        	 atp_struct * atp0)
{
  ExeErrorCode rc = buildNetwork(pool,atp0);
  if (rc == (ExeErrorCode)0)    // if no error
    {
      current_column_ = first_column_;
      current_column_index_ = 0;
      current_column_->initNextValue();
    }
  return rc;
}
 

keyRangeEx::getNextKeyRangeReturnType keyMdamEx::getNextKeyRange
  (atp_struct *,NABoolean fetchRangeHadRows, NABoolean detectNullRange)
{
  // This logic + MdamColumn::getNextValue are essentially a re-coding
  // of the SQL/MP procedure evac^next^traverse.  One fundamental
  // difference is that unlike SQL/MP, sparse probes are done in the
  // layer on top of Mdam, instead of below.  So, this procedure, for
  // example, might return a key range for probing or for fetching.
  // In SQL/MP, evac^next^traverse returns only key ranges for
  // fetching.  The probing is done inside the scope of the
  // evac^next^traverse procedure call.

  if (fetchRangeHadRows)
    productiveFetchRangeCounter_++;

  getNextKeyRangeReturnType rc = NO_MORE_RANGES; // assume no more ranges
  NABoolean not_done = TRUE;

  char *bktarget = bkData_.getDataPointer();
  char *ektarget = ekData_.getDataPointer();
  ULng32 keyLength = getGenInfo().getKeyLength();

  // LCOV_EXCL_START
  if (getGenInfo().getKeytag() > 0)
    {
      unsigned short keytag = getGenInfo().getKeytag();
      str_cpy_all(bktarget, (char *)&keytag, sizeof(short));
      str_cpy_all(ektarget, (char *)&keytag, sizeof(short));
      bktarget += sizeof(short);
      ektarget += sizeof(short);
      keyLength -= sizeof(short);
    }
  // LCOV_EXCL_STOP

  if (complementKeysBeforeReturning_)
    {
      // if reverse scan, need to swap where we put begin and end keys
      char *temp = bktarget;
      bktarget = ektarget;
      ektarget = temp;
    }

  //  Part of fix for Genesis case 10-971031-9814:
  //  Keep track of the offset of the left-most column traversed.
  ULng32 minOffset = 0;
  if (current_column_)
    minOffset = current_column_->getOffset();
  //  End of this part of the fix.

  while ((current_column_) && (not_done))
    {
      MdamColumn::getNextValueReturnType status = 
	current_column_->getNextValue(productiveFetchRangeCounter_,
				      bktarget,
				      ektarget,
				      bkExcludeFlag_,
				      ekExcludeFlag_,
				      mdamRefListEntryHeap_);
      switch (status)
	{
	case MdamColumn::TRAVERSE_DOWN:
	  {
	    current_column_ = current_column_->nextColumn();
	    current_column_index_++;
	    ex_assert(current_column_ != 0,
		      "keyMdamEx::getNextKeyRange() traversed off end");
	    current_column_->initNextValue();
	    break;
	  }
	case MdamColumn::TRAVERSE_UP:
	  {
	    current_column_ = current_column_->previousColumn();
	    current_column_index_--;

            //  Part of fix for Genesis case 10-971031-9814:
            //  Keep track of the offset of the left-most column traversed.
            ULng32 offset = 0;
            
            if (current_column_)  // note current_column_ is 0 at end of all traversal
              offset = current_column_->getOffset();
            if (offset < minOffset)
              minOffset = offset;
            //  End of this part of the fix.
            
	    ex_assert(current_column_index_ >= -1,
		      "keyMdamEx::getNextKeyRange() column index wrong");
	    break;
	  }
	case MdamColumn::SUBSET:
	case MdamColumn::PROBE:
	  {
	    MdamColumn * col;
	    Lng32 index = current_column_index_ + 1;

	    for (col = current_column_->nextColumn();
		 col != 0;
		 col = col->nextColumn())
	      {
		col->completeKey(bktarget,
				 ektarget,
				 bkExcludeFlag_,
				 ekExcludeFlag_);
		index++;
	      }
	    
	    ex_assert(index == number_of_key_cols_,
		      "keyMdamEx::getNextKeyRange() completeKey failure");

	    not_done = FALSE;

	    rc = FETCH_RANGE;
	    if (status == MdamColumn::PROBE) 
	      rc = PROBE_RANGE;

	    if (complementKeysBeforeReturning_)
	      {
		// On reverse scans, the keys in the Mdam network have
		// been complemented; we must undo this complementing
		// before returning the keys to the caller.

                // Part of fix for Genesis case 10-971031-9814:
                // Complement only that part of the key that was computed
                // on this call to getNextKeyRange(); the preceding parts
                // of the key have already been complemented on prior calls.

                str_complement(keyLength-minOffset,bktarget+minOffset);
                str_complement(keyLength-minOffset,ektarget+minOffset);
                // End of fix.

		// also we need to swap the exclude flags (just as we
		// did the key buffers earlier)

		short temp;

		temp = bkExcludeFlag_;
		bkExcludeFlag_ = ekExcludeFlag_;
		ekExcludeFlag_ = temp;
	      }
	    
	    break;
	  }
	default:
	  {
	    ex_assert(0,"keyMdamEx::getNextKeyRange() invalid getNextValue rc"); // LCOV_EXCL_LINE
	    break;
	  }
	}
    }

  return rc;
}


void keyMdamEx::reportProbeResult(char *keyData)
{
  ex_assert(current_column_,
	    "keyMdamEx::reportProbeResult() called outside traverse");

  if (complementKeysBeforeReturning_)
    {
      if (keyData)
	{
	  // for reverse scans, complement the encoding so it is consistent
	  // with what is stored in the MDAM network
	  str_complement(getGenInfo().getKeyLength(),keyData);
	}
    }

  current_column_->reportProbeResult(keyData);
}

// release tupp storage associated with an Mdam network
void keyMdamEx::release()
{
  destroyNetwork();
  keyRangeEx::release();  // invoke release() on base class
}


// Print functions.
#if defined ( NA_MDAM_EXECUTOR_DEBUG )
void MdamColumn::print(const char * header) const
{
  cout << header << endl;

  if (intervals_.isEmpty())
    {
      cout << "No intervals" << endl;
    }
  else
    {
      intervals_.printBrief();
    }
  cout << " Stop list: ";
  stop_list_.printBrief();
  cout << endl;
}  

void keyMdamEx::print(const char * header) const
{
  cout << header << endl;

  MdamColumn * c;
  Lng32 i = 0;

  for (c = first_column_;
       c != 0;
       c = c->nextColumn())
    {
      cout << "Column " << i;
      c->print(":");
      i++;
    }
}
#endif /* NA_MDAM_EXECUTOR_DEBUG */

