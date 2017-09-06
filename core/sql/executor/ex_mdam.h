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
#ifndef EX_MDAM_H
#define EX_MDAM_H

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

#include "ex_error.h"
#include "NABoolean.h"
#include "key_range.h"
#include "key_mdam.h"
#include "MdamEnums.h"
#include "MdamEndPoint.h"
#include "MdamPoint.h"
#include "MdamRefList.h"
#include "MdamRefListEntry.h"
#include "MdamInterval.h"
#include "MdamIntervalList.h"
#include "MdamIntervalListIterator.h"
#include "FixedSizeHeapManager.h"

// forward references
class MdamColumn;


///////////////////////////////////////////////////////////
// Class MdamPredIterator
//
// This class is used to build an Mdam network.  It keeps
// state associated with traversing MdamPreds hung off of
// the keyRangeGen and MdamColumnGen objects.  We can't keep
// this state in the keyRangeGen and MdamColumnGen objects
// themselves since they are read-only (created by the
// Generator).  Some of the needed state is managed by
// this class but stored in MdamColumn objects. 
///////////////////////////////////////////////////////////
class MdamPredIterator : public ExGod
 {

   Lng32 currentDisjunctNumber_;
   Lng32 maxDisjunctNumber_;

   // The next variable is used to parse the MdamPred's into OR
   // groups.  It is initialized to FALSE by positionToNextOr();
   // getNextPred() sets it to TRUE whenever it returns a predicate.
   // If getNextPred() is called and this variable is already true,
   // getNextPred() returns 0 if the firstInOrGroup() method on the
   // next MdamPred is TRUE.
   NABoolean returnedAPred_;

public:

   MdamPredIterator(MdamColumn * first,Lng32 maxDisjunctNumber);

   ~MdamPredIterator()
       {  };

   Lng32 getNextDisjunctNumber();  // -1 means no more disjuncts

   // In the next three methods, currentPred is state stored in MdamColumn
   // objects, but updated by these methods only.

   // FALSE means no more predicates
   NABoolean positionToNextOr(MdamPred **currentPred);

   // 0 (NULL) means no more predicates
   // for this key column within this predicate  
   MdamPred * getNextPred(MdamPred **currentPred); 
   
   // Position the predicate list to the current disjunct.
   void positionToCurrentDisjunct(MdamPred **currentPred);

};


/////////////////////////////////////////////////////
// class MdamColumn
/////////////////////////////////////////////////////
class MdamColumn : public ExGod
{
  MdamColumn * next_;
  MdamColumn * previous_;
  MdamIntervalList intervals_;
  MdamIntervalList tentative_intervals_;  // used in building; holds
                                          // intervals for current disjunct
  MdamIntervalListIterator interval_iterator_;  // needed for traversal
  MdamInterval * current_interval_;             // needed for traversal
  NABoolean current_is_subset_interval_;
  NABoolean traversal_in_progress_;

  // The next field determines whether we use dense or sparse probes
  // to materialize values in a non-subset interval.  (See below for a
  // description of what a subset interval is.) 
  NABoolean useSparseProbes_;

  // The next field is used only for dense probes.  If we do a dense
  // probe, and the scan operator doesn't obtain any rows from the
  // resulting fetch ranges, we switch to sparse probes until the
  // situation changes.  This keeps us from generating lots of
  // non-productive fetch ranges when there are gaps between values in
  // some interval.
  ULng32 lastProductiveFetchRangeCounter_;

  // The next three fields are always FALSE when dense probing is
  // used.  When sparse probing is used, sparseProbeNeeded_ is set
  // whenever getNextValue() returns "PROBE".  It is reset by
  // reportProbeResult().  The latter method sets exactly one of
  // sparseProbeSucceeded_ or sparseProbeFailed_.  These variables
  // enforce the protocol that after getNextValue() returns "PROBE",
  // we must call reportProbeResult() (or initNextValue()) before calling
  // getNextValue() again.
  NABoolean sparseProbeNeeded_;
  NABoolean sparseProbeSucceeded_;
  NABoolean sparseProbeFailed_;

  // The following ref list is the intersection of the ref lists
  // of the current intervals of this and all preceeding columns.
  // It is meaningful only when we have traversed to some column
  // beyond this column (i.e. when we traverse down).
  MdamRefList current_context_;

  // The following ref list is used to determine whether a given
  // interval is a "subset interval", i.e. whether we can use this
  // interval to define a single key range.  We can do this if and
  // only if all values of any key columns to the right of this
  // column are included.  (Note that this implies that any interval
  // in the right-most column that can be traversed to is a subset
  // interval.)  We call this ref list a "stop list" because it 
  // stops traversal to key columns to the right.
  MdamRefList stop_list_;

  // The hi, lo, etc. values here are all encoded, so they already
  // take into account whether the column is ASC vs. DESC (but not
  // whether this is a reverse scan).
  tupp currentValue_;      // current value of column
  tupp hi_;                // highest possible value for column
  tupp lo_;                // lowest possible value for column
  tupp nonNullHi_;         // highest possible non-NULL value for column
  tupp nonNullLo_;         // lowest possible non-NULL value for column

  // the next field links the run-time column state to compile-time
  // column state
  MdamColumnGen * columnGenInfo_;

  // the next field is used by MdamPredIterator to maintain state
  // when iterating over predicates for a given column
  MdamPred * currentPred_;

  // globals.
  ex_globals * glob_;

public:

  enum getNextValueReturnType { TRAVERSE_DOWN, TRAVERSE_UP, SUBSET, PROBE };

  MdamColumn(MdamColumn * previous,
			MdamColumnGen *columnGenInfo,
			ex_globals *glob,
			sql_buffer_pool *pool,
			atp_struct *atp0,
			atp_struct *workAtp,
			unsigned short valueAtpIndex,
                        const ex_tcb *tcb);

  ~MdamColumn();

  // methods used to build an Mdam network

  void initCurrentPred()
  { currentPred_ = columnGenInfo_->getFirstPred(); };
  
  // returns TRUE if the disjunct number is in some stop list
  NABoolean buildDisjunct(MdamPredIterator & predIterator,
				     sql_buffer_pool *pool,
				     atp_struct *atp0,
				     atp_struct *atp1,
				     unsigned short valueAtpIndex,
				     Lng32 disjunct_number,
				     NABoolean disjunct_number_in_stop_list,
				     FixedSizeHeapManager & mdamIntervalHeap,
				     FixedSizeHeapManager & 
                                      mdamRefListEntryHeap,
				     FixedSizeHeapManager & 
                                      mdamRefListEntrysForStopListsHeap,
                                     Lng32 & dataConvErrorFlag);

  void tossDisjunct(FixedSizeHeapManager & mdamIntervalHeap,
                               FixedSizeHeapManager & mdamRefListEntryHeap);

  void mergeDisjunct(Lng32 disjunct_number,
				FixedSizeHeapManager & mdamIntervalHeap,
				FixedSizeHeapManager & mdamRefListEntryHeap);

  NABoolean disjunctIsEmpty();

  // Method used to destroy an Mdam network.
  void releaseNetwork(FixedSizeHeapManager & mdamIntervalHeap,
                                 FixedSizeHeapManager & mdamRefListEntryHeap);

  // Member function used when we are done with the plan.
  void release(FixedSizeHeapManager & mdamRefListEntrysForStopListsHeap);

  // methods used to traverse an Mdam network
  MdamIntervalList & getIntervalList();

  MdamColumn * nextColumn() { return next_; };
  MdamColumn * previousColumn() { return previous_; };

  NABoolean initNextValue();

  // returns TRAVERSE_DOWN if a value was obtained, and the current
  // interval is not a subset interval; SUBSET if values were obtained
  // and it *is* a subset interval; TRAVERSE_UP if no values were obtained.

  // The input parameters are set as follows:
  // Returned value         Parameter
  // --------------         ---------
  // TRAVERSE_DOWN          beginValue,endValue are both set 
  //                        (and are set to the same value)
  //                        beginExclFlag and endExclFlag are not set
  // TRAVERSE_UP            nothing is set
  // SUBSET or PROBE        beginValue,endValue are both set 
  //                        (but might not be the same value)
  //                        beginExclFlag and endExclFlag are both set and
  //                        should be applied to the key range as a whole
  getNextValueReturnType getNextValue(ULng32 pFRCounter,
						 char *bktarget,
						 char *ektarget,
						 short &beginExclFlag,
						 short &endExclFlag,
			   FixedSizeHeapManager & mdamRefListEntryHeap);

  void reportProbeResult(char *keyData);

  void completeKey(char *bktarget,
                              char *ektarget,
                              short bkexcl,
                              short ekexcl);

  ULng32 getOffset() 
    { return columnGenInfo_->getOffset(); };

  // Print function.
  #ifdef NA_MDAM_EXECUTOR_DEBUG
  void print(const char * header = "") const;
  #endif /* NA_MDAM_EXECUTOR_DEBUG */

private:
  void setNextColumn(MdamColumn * next);



};

/////////////////////////////////////////////////////
// class keyMdamEx
/////////////////////////////////////////////////////

class keyMdamEx : public keyRangeEx
{
  Lng32 number_of_key_cols_;

  // anchors for the Mdam network
  NABoolean network_built_;
  NABoolean stop_lists_built_;  // only need to build stop lists once ever
  MdamColumn * first_column_;
  MdamColumn * last_column_;

  // state variables used during Mdam network traversal
  MdamColumn * current_column_;
  Lng32 current_column_index_;

  // the counter below is passed to MdamColumn::getNextValue, and helps
  // the MdamColumn decide whether to switch from dense probes to sparse
  ULng32 productiveFetchRangeCounter_; // unsigned to allow wrapping

  // Memory management for MdamRefListEntry*s and MdamInterval*s.
  FixedSizeHeapManager mdamRefListEntryHeap_;
  FixedSizeHeapManager mdamRefListEntrysForStopListsHeap_;
  FixedSizeHeapManager mdamIntervalHeap_;

  // For reverse scans, we complement the encoded key values in the
  // Mdam network (this is done via the encode expressions created by
  // the Generator) so that we can treat forward scans and reverse
  // scans in the same way. But the scan operators require the values
  // to be uncomplemented.  So, we need to know whether to
  // uncomplement them before returning.  The following member tells
  // us.
  
  NABoolean complementKeysBeforeReturning_;

public:
  keyMdamEx(const keyRangeGen & tdb_key,
		       const short in_version,
		       sql_buffer_pool *pool,
		       ex_globals *glob,
                       const ex_tcb *tcb);
 
  ~keyMdamEx();

  virtual ExeErrorCode initNextKeyRange(sql_buffer_pool *pool,
					           atp_struct * atp0);

  virtual getNextKeyRangeReturnType getNextKeyRange
    (atp_struct * atp0,NABoolean fetchRangeHadRows,
     NABoolean detectNullRange = TRUE); 

  virtual void reportProbeResult(char *keyData);

  // release tupp storage
  void release();

  keyMdamGen & getGenInfo()
  { return (keyMdamGen &)tdbKey_; };

private:
  // Returns zero if memory for MdamRefListEntry's and MdamInterval's
  // is successfully acquired.  Otherwise, returns ExeErrorCode.
  ExeErrorCode buildNetwork(sql_buffer_pool *pool,atp_struct *atp0);

  void destroyNetwork();

  // Print function.
  #ifdef NA_MDAM_EXECUTOR_DEBUG
  void print(const char * header = "") const;
  #endif /* NA_MDAM_EXECUTOR_DEBUG */

};


#endif












