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
#ifndef COLSTATDESC_H
#define COLSTATDESC_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ColStatDesc.h
 * Description:  This file contains the declaration for ColStatDesc -
 *               the descriptor for the ColStats (column statistics)
 *               structure.
 *
 * Created:      June 1, 1995
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Stats.h"  /* includes CostScalar.h, Collections.h, ValueDesc.h ... */
#include "NATable.h"
#include "SharedPtr.h"
#include "SharedPtrCollections.h"

// -----------------------------------------------------------------------
//  Contents of this file
// -----------------------------------------------------------------------
class MultiColumnUecList;
class ColStatDesc ;
class ColStatDescList;

// needed as a forward reference -- not contained in this file!
class TableDesc;
class SelectivityHint;
class CardinalityHint;
class Join;

// useful defn we use here & in PartKeyDist.[h cpp]
typedef LIST(EncodedValue) EncodedValueList ;
typedef SharedPtr<ColStatDesc> ColStatDescSharedPtr;



// -----------------------------------------------------------------------
// Multi-column uec list
//
// An association list of <list of table column,uec-count>.
//
// We maintain multi-column uec information in order to accurately
// estimate rowcounts for joins involving multiple predicates, the output
// of a groupby aggregate, and possibly other cases I'm not thinking of
// right now.  This class is a datamember of StatsList, and then the
// ColStatDescList's created from the StatsList have read-only access to
// the one copy that's maintained for all ColStatDesc's.
// -----------------------------------------------------------------------

#define MultiColumnUecListIterator NAHashDictionaryIterator<ValueIdSet,CostScalar>

class MultiColumnUecList : public HASHDICTIONARY(ValueIdSet,CostScalar)
{
public:
  static ULng32 HashFunction (const ValueIdSet & input) ;

  MultiColumnUecList (const StatsList   & initStats,
                      const ValueIdList & tableColumns ) ;

  virtual ~MultiColumnUecList() {} ;

  // given a ValueIdSet of table columns, returns the stored groupUec (if
  // it exists), else returns csMinusOne
  CostScalar lookup (const ValueIdSet & key) const ;

  void initializeMCUecForUniqueIndxes(TableDesc &table,
				      const CostScalar & tableRowcount);

  // -----------------------------------------------------------------------
  // useMCUecforCorrPreds
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
  // "numPredicates", another parameter, is specified by the calling
  // routine to let this routine know how many histograms total have been
  // altered inside of the estimateCardinality() routine.  This value is
  // used as an upper bound for the computation of how many columns in a
  // single table have had predicates applied to them (in an attempt to
  // avoid creating too large of an estimate in cases where we join
  // together columns of the same table).
  //
  NABoolean useMCUecForCorrPreds (
       NAHashDictionary<ValueId, CostScalar> & predReductions, /* in/mod */
       const CollIndex numPredicates,                          /* in */
       const CostScalar& oldRowCount,                          /* in */
       const CostScalar& newRowCount,                          /* in */
       NABoolean largeTabStatsNeeded,
       const ColStatDescList & source,
       CostScalar & reductionAdjustment)  ;               /* out */

  // -----------------------------------------------------------------------
  // getUecForMCJoin
  //
  // used by multi-column join code (fn useMultiUecIfMultipleJoins(), subr of
  //                                 fn estimateCardinality() )
  //
  // given a list of ValueIdLists representing the two (or more) join
  // predicates between (hopefully) two tables, return TRUE/FALSE if we
  // have the necessary multi-column uec information about some of the
  // columns involved in this join; return the columns we don't have MC
  // info for, and return this multi-column uec number.
  // we also supply a boolean flag "largeTableNeedsStats" which is the
  // ColStats flag "isUpStatsNeeded" -- this helps decide whether to fire
  // off a 6007-warning in the case where the best possible multi-column
  // stats don't exist
  //
  // we also supply a boolean flag "largeTableNeedsStats" which is the
  // ColStats flag "isUpStatsNeeded" -- this helps decide whether to fire
  // off a 6007-warning in the case where the best possible multi-column
  // stats don't exist
  //
  // i.e., if we do
  //    "sel * from T1,T2 where T1.a=T2.b AND T1.c=T2.d",
  // we need MC-info on (T1.a,T1.c) and (T2.b,T2.d) -- if this exists,
  // then return TRUE and set maxMultiColUec to be the larger of the two
  // corresponding multi-column uec values for (T1.a,T1.c) & (T2.b, T2.d)
  //
  // as a more general case, if we do
  //    "sel * from T1,T2 where T1.c1=T2.c1 AND T1.c2=T2.c2 AND ... T1.cn=T2.cn",
  // then we want to return the largest ValueIdSets (t1.1,...,t1.m) (t2.1,...t2.m)
  // such that there is exactly one t2.1 for every t1.1 -- any remaining ValueIdSets
  // in joinValueIdPairs are returned to the calling function, which will have to
  // apply single-column selectivity for them.
  //
  NABoolean getUecForMCJoin (LIST(ValueIdList) & joinValueIdPairs,       /* in/out */
                             const NABoolean     largeTableNeedsStats,   /*   in   */
                             const Join * expr,
                             CostScalar        & prodMaxInitUec,         /*   out  */
                             CostScalar        & maxMultiColUec,	 /* out */
			     CostScalar        & baseRCForMaxMCUEC,	 /* out */
			     CostScalar        & leftMCUec,	 /* out */
			     NABoolean	       & checkForLowBound,       /* out   */
                             NABoolean         & joinOnUnique,
                             const ColStatDescList & colStats,
                             CostScalar        redFromSC=csMinusOne);       

  //------------------------------------------------------------------
  // Determine if the join predicates consist of column sets where one 
  // of the sides is unique and shares relationship "similar" to PK/FK 
  // with the other side. If so, return the cardinality of non-unique 
  // side. This feature is OFF by default if there are more than one 
  // joining columns. It'll be controlled by COMP_BOOL_149.
  //------------------------------------------------------------------
  CostScalar getRowcountOfNonUniqueColSet(const Join *expr, 
                                    ValueIdList lhsColList, 
                                    ValueIdList rhsColList, 
                                    NABoolean leftUnique, 
                                    NABoolean rightUnique);

  // --------------------------------------------------------------
  // display missing stats warning. The warning is displayed based 
  // on the CQDs:
  // HIST_MISSING_STATS_WARNING_LEVEL - The CQD has 5 values
  // It is used to control the number of missing stats warnings
  // that should be generated. 
  // 0: Display no warnings.
  // 1: Display only missing single column stats warnings. These include 6008 and 6011 
  // 2: Display all single column missing stats warnings and 
  //    multi-column missing stats warnings for Scans only. 
  // 3: Display all missing single column stats warnings and missing 
  //    multi-column stats warnings for Scans and GroupBy operators only..
  // 4: Display all missing single column stats and missing multi-column 
  //    stats warnings for all operators including Scans, Joins and groupBys.
  // THE CQD also does not have an impact on the auto update stats behavior. The stats will
  // still be automatically generated even if the warnings have been suppressed.
  // Default behavior is to generate all warnings
  // --------------------------------------------------------------
  void
   displayMissingStatsWarning(TableDesc * mostRefdTable,
			      ValueIdSet predCols,
			      NABoolean largeTableNeedsStats,
                              NABoolean displayWarning,
                              const ColStatDescList & colStats, 
                              CostScalar redFromSC = csMinusOne,
                              NABoolean quickStats = FALSE,
                              OperatorTypeEnum op = REL_SCAN) const;

  // -----------------------------------------------------------------
  // isMCStatsUseful is used to determine if there is any possibility
  // of optimizer benefiting from multi-column stats. The MC stats 
  // are said to be not helpful, if any subset of given column set
  // is orthogonal. More heuristics can be added later to determine
  // usefulness of MC stats
  // -----------------------------------------------------------------
  NABoolean
    isMCStatsUseful(ValueIdSet columnSet,
                    TableDesc * tableDesc) const;

  // ------------------------------------------------------------------
  // Combine MC UEC of subset of columns from columns with reduction to get 
  // MC UEC of larger set
  // ------------------------------------------------------------------

  NABoolean createMCStatsForColumnSet(ValueIdSet colsWithReduction, 
                                      ValueIdSet & cumulativeColSetWithMCUEC,
                                      CostScalar & maxMultiColUec,
                                      CostScalar baseRowCount
                                      );
  // -----------------------------------------------------------------------
  // findMatchingColumns
  //
  // subroutine of getUecForMCJoin; used to find correspondences between
  // lists of table columns w.r.t. the join predicates
  // -----------------------------------------------------------------------
  ValueIdSet findMatchingColumns (const ValueIdSet        & t1Cols,     /* in  */
                                  const LIST(ValueIdList) & joinPairs,  /* in  */
                                  LIST(ValueIdList) & remainingPairs,   /* out */
                                  CostScalar        & maxInitUecProduct, /* out */
                                  CostScalar        & minInitUecProduct, /* out */
                                  NABoolean	    & checkForLowBound	/* out */
				  ) const ;

  // -----------------------------------------------------------------------
  // largestSubset
  //
  // used by multi-column group by code (GroupbyAgg::synthEstLogProp)
  //
  // given a list of table columns we're interested in, returns a list
  // representing the largest subset of the input list for which we have
  // multicolumn uec information
  //
  // if there are ties, return the one with largest correlation
  ValueIdSet largestSubset (const ValueIdSet & columns) const ;



  //---------------------------------------------------------------------
  //MultiColumnUecList::getListOfSubsetsContainsColumn
  //
  //Input: columnList
  //Output: List of ValueIdSet that contains the last column in the list
  //and other columns from the columnList only
  //Constraints: ColumnId that is passed in can be VegRef that contains
  //             the base id for that column at the first level or it can
  //             be a the id corresponding to a index on the table.
  //---------------------------------------------------------------------
  LIST(ValueIdSet) * getListOfSubsetsContainsColumns(
    const ValueIdList & columns,
    LIST(CostScalar)& uecCount
    ) const;

  //---------------------------------------------------------------------
  //MultiColumnUecList::findDenom
  //
  //Input: list of columns
  //Output: Boolean. if there is a multi-column histogram exactly matching
  //the input then return true or return false
  //Constraints: ValueIds for the columns need to be base valueIds.
  //---------------------------------------------------------------------
  NABoolean findDenom(const ValueIdSet & columns)const;

  // add the <table-column-valueidset, uec-value> pairs from OTHER into
  // THIS (the ones that aren't already there)
  void insertList (const MultiColumnUecList * other) ;

  void insertMappedList(const MultiColumnUecList *other,
                        const ValueIdMap &map); // map is used in "up" direction

  // this routine answers whether there's any bona fide multi-column
  // information contained in this list -- i.e., any
  // <valueidset,costscalar> pairs where the valueidset has more than one
  // entry
  NABoolean containsMCinfo() const ;

  // display the contents of the MultiColumnUecList
  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const ;
  void display () const ;
  // inserts a <table-column-valueidset, uec-value> pair
  // returns TRUE if successful, FALSE if not successful (e.g., already exists)
  // ==> private because no one should ever change this object!
  // made public, to handle updateMCUecForUniqueIndexes -

  NABoolean insertPair (const ValueIdSet & key, const CostScalar & groupUec) ;


  // updates groupUec for columns in the Multi-column Uec list
  // This will be used only if no multi-col uec exists for the unique
  // index or if the uec for unique index is not equal to the row count
  // as it should be

  NABoolean updatePair (const ValueIdSet & columns,
				const CostScalar & groupUec);

  // Following method creates multi-col UEC for larger set of columns
  // using partial overlapping multi-col UECs.
  // For example, if MC-UEC available - (a, b, c) (c, d).
  // Then MC (a, b, c, d) = MC (a, b, c) * MC (c, d) / MC (c)
  NABoolean createMCUECWithOverlappingColSets(ValueIdSet & remainingCols,
    					       ValueIdSet & cumulativeColSetWithMCUEC,
					       CostScalar & multiColUec,
					       CostScalar oldRowcount);


  // Following method creates multi-col UEC for larger set of columns
  // using partial disjoint multi-col UECs.
  // For example, if MC-UEC available - (a, b) (c, d).
  // Then MC (a, b, c, d) = MC (a, b) * MC (c, d)
  NABoolean createMCUECWithDisjointColSets(ValueIdSet & remainingCols,
    					    ValueIdSet & cumulativeColSetWithMCUEC,
					    CostScalar & multiColUec,
					    CostScalar oldRowCount);

  // In the following method, we shall create a new MC list.
  // This list contains MC-UEC for only those column set, which include
  // all columns of colsWithReductions and atmost one column from
  // cumulativeColSetWithMCUEC

  MultiColumnUecList * createMCListForRemainingCols(
				ValueIdSet colsWithReductions,
				ValueIdSet cumulativeColSetWithMCUEC);

  MultiColumnUecList () ; // added 05/23/05.

private:

  // this class should never create an uninitialized object!
  // this class should never be copied!
  // Commented because we need to create a temporary MultiColumnUecList
  // in the method createMCListForRemainingCols - 05/23/05
  // MultiColumnUecList () ;
  MultiColumnUecList (const MultiColumnUecList & other) ;
};

#define MultiColumnSkewedValueListsIterator NAHashDictionaryIterator<ValueIdList,MCSkewedValueList>

class MultiColumnSkewedValueLists : public HASHDICTIONARY(ValueIdList,MCSkewedValueList)
{
public:
  static ULng32 HashFunction (const ValueIdList & input) ;

  MultiColumnSkewedValueLists ();
  MultiColumnSkewedValueLists (const StatsList   & initStats,
			     const ValueIdList & tableColumns ) ;

  virtual ~MultiColumnSkewedValueLists() { };

  // This method will retrieve skew values if found, otherwise NULL is returned.
  const MCSkewedValueList* getMCSkewedValueList(ValueIdSet colSet, ValueIdList & colGroup);

private:
  MultiColumnSkewedValueLists (const MultiColumnSkewedValueLists & other) ;
};

// -----------------------------------------------------------------------
//  A column statistics descriptor contains a valueid for the column
//  that makes up this column statistics object, as well as a pointer
//  to the ColStats structure.
// -----------------------------------------------------------------------
class ColStatDesc : public NABasicObject
{
  friend class TableDesc;

protected:

  // copy method
  void copy (const ColStatDesc& other) ;

  // deallocate method
  void deallocate () ;

public:

  // the following enum is used only in synchronizeStats()
  enum SynchSpecialFlag { DO_NOTHING_SPECIAL, DO_NOT_REDUCE_UEC, SET_UEC_TO_ONE } ;

  // default constructor
  ColStatDesc (NAMemory * h=HISTHEAP) :
       column_(), VEGcolumn_(), nonVegEquals_(h), colStats_(NULL), modified_(FALSE), 
		 inputCard_(1.0)
  { }

  // constructor
  ColStatDesc (const ColStatsSharedPtr& stats, const ValueIdList& columnList, NAMemory * h=HISTHEAP) ;

  // constructor used to create a ColStatDesc for a generated column.
  ColStatDesc (const ColStatsSharedPtr& stats, const ValueId & column, NAMemory * h=HISTHEAP) ;

  // virtual destructor
  ~ColStatDesc() { deallocate(); }


  // copy constructor
  ColStatDesc (const ColStatDesc& other, NAMemory * h=HISTHEAP) :
       nonVegEquals_(h)
  { copy(other); }

  // assignment operator
  inline ColStatDesc & operator= (const ColStatDesc& other)
  {
    if ( &other != this )             // support a=a
      { deallocate(); copy(other); }
    return *this;
  }

  // comparison operator
  NABoolean operator== (const ColStatDesc& other) const
  { return (column_ == other.column_) ; }

  // accessor functions (all const)
  inline const ValueId & getColumn () const               { return column_; }
  inline const ValueId & getVEGColumn () const            { return VEGcolumn_; }
  inline const ColStatsSharedPtr getColStats () const            { return colStats_; }
  inline NABoolean isModified () const                    { return modified_; }
  inline NABoolean isFromInnerTable () const              { return fromInnerTable_; }

  inline const ValueIdSet & getMergeState() const         { return mergeState_; }
  inline const SHPTR_LIST(ColStatDescSharedPtr)&
               getNonVegEquals() const                    { return nonVegEquals_; }

  inline const ValueIdSet & getAppliedPreds () const      { return appliedPreds_; }
  inline NABoolean isPredicateApplied (const ValueId & newPredicate) const
                                { return appliedPreds_.contains( newPredicate ); }
  NABoolean isSimilarPredicateApplied ( const OperatorTypeEnum op ) const;

  NABoolean derivOfLikeAndSimilarPredApp(const ItemExpr * pred ) ;

  CostScalar selForRelativeRange (const OperatorTypeEnum op,
                                  const ValueId  & column,
                                  ItemExpr *newPred) const;


  // these accessor functions return access to the private data members
  ColStatsSharedPtr getColStatsToModify () ;
  inline ValueId & VEGColumn ()                           { return VEGcolumn_; }
  inline ValueIdSet & mergeState ()                       { return mergeState_; }
  inline SHPTR_LIST(ColStatDescSharedPtr)& nonVegEquals () { return nonVegEquals_; }
  inline ValueIdSet & appliedPreds ()                     { return appliedPreds_; }

  // manipulation functions
  inline void setModified (NABoolean flag=TRUE)           { modified_ = flag; }
  inline void setFromInnerTable (NABoolean flag=TRUE)     { fromInnerTable_ = flag; }
  inline void setColStats (const ColStatsSharedPtr& stats) { colStats_ = stats; }

  inline void addToAppliedPreds (const ValueId & newPredicate)
    { appliedPreds_.insert( newPredicate ); }
  inline void removeFromAppliedPreds (const ValueId & newPredicate)
    { appliedPreds_.remove( newPredicate ); }

  // apply the following selectivity to the column statistics
  void applySel (const CostScalar & selectivity) ;

  void applySelIfSpecifiedViaHint(ItemExpr * pred, const CostScalar & oldRowcount);

  void setInputCard (CostScalar rows) {inputCard_ = rows; }

  CostScalar getInputCard()  { return inputCard_; }

  void mapUpAndCopy (const ColStatDesc& other, ValueIdMap &map) ;

  // synchronize/map the RowCount and UEC change of one set of aggregate
  // statistics with the current set of aggregate statistics
  void synchronizeStats (const CostScalar & baseRowcount,
                         const CostScalar & newRowcount,
                         SynchSpecialFlag=DO_NOTHING_SPECIAL) ;

  // modify statistics by applying the effect of the provided predicate
  NABoolean modifyStats (ItemExpr *pred, CostScalar &newRowcount,
                         CostScalar *maxSelectivity=NULL);

  // merge twoColStatDescs from the same table
 NABoolean mergeColStatDescOfSameTable(ColStatDescSharedPtr &rightColStats,
                                       OperatorTypeEnum opType = ITM_FIRST_ITEM_OP);

 // merge two ColStatDesc's
  void mergeColStatDesc(ColStatDescSharedPtr& mergedStatDesc,
			MergeType mergeMethod,
			NABoolean forceMerge = FALSE,
                        OperatorTypeEnum opType = ITM_FIRST_ITEM_OP,
                        NABoolean mergeFVs=TRUE) ;

  // -----------------------------------------------------------------------
  // Reduce uec by correct amount, instead of what we've done in the past.
  // -----------------------------------------------------------------------
  static CostScalar
  calculateCorrectResultUec (const CostScalar & baseRows,
                             const CostScalar & newRows,
                             const CostScalar & baseUec) ;

  // display the colStats_ inside the colstatdesc:
  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "",
              CollHeap *c=NULL, char *buf=NULL,
              NABoolean hideDetail=FALSE) const ;
  void display () const ;

  // ------------------------------------------------------------------------
  // The first four private members are worthy of a little discussion and
  // clarification.
  //
  // 'column' is the column for which these stats apply.
  //
  // 'VEGcolumn_' is the VEGRef id corresponding with column_ e.g.,
  // column_ is T2.a; If IT2.a is an index on T2.a, this VEG includes
  // (IT2.a, T2.a), further if there exists a predicate T2.a=T2.b and
  // IT3.b is an index on T2.b, the VEG includes (IT2.a, T2.a, IT3.b,
  // T2.b).  VEGcolumn_ may, over time, contain an instantiate_null
  // operator when the associated column has become the output of an outer
  // join, as well as an value_id_union map when the column is the output
  // of a union.
  //
  // 'mergeState_' is a set associated with column_.  Each set indicates
  // which of the statistics from the matching VEGcolumn_ entry have been
  // merged.  Each set starts with only the ValueId from column_, but as
  // VEG preds are applied, this set grows.  This entry is necessary to
  // support nested index joins, and as a side benefit prevent a VEGPred
  // of the form a=a from doing anything to the statistics for 'a'.
  //
  // 'nonVegEquals_' tracks information regarding EQ-Joins applied to columns
  // outside the 'normal' realm of VEG predicates.
  //
  // This, primarily, applies to equality predicates underneith an OR,
  // which are not placed in a VEG.  The information is used 2 ways:
  // - For AND's beneath an OR, this list is used to provide transitivity;
  // - Directly beneath OR's, this list indicates what ColStatDesc to
  //   update/recreate and add back into the containing ColStatDescList.
  // ------------------------------------------------------------------------

private:

  // compress the ColStats for local predicates on a column
  // The local predicates should involve a constant
  // e.g.
  // * t1.col1 = 3
  // * t1.col1 < 3
  // * t1.col1 > 1
  // * t1.col1 > 1 and t1.col1 < 3
  void compressColStatsForQueryPreds(ItemExpr * lowerBound,
                                     ItemExpr * upperBound,
                                     NABoolean  hasJoinPred = FALSE)
  { colStats_->compressColStatsForQueryPreds(lowerBound, upperBound, hasJoinPred); };

  ValueId column_;              // identify the base table column(s) of which
  //                            // these statistics is/are comprised
  ValueId VEGcolumn_;           // identify the equivalent VEG (corresponding
  //                            // to base table columns) of which these stats
  //                            // are comprised
  ValueIdSet mergeState_;       // indicate which histograms have been merged
  //                            // (so far)
  SHPTR_LIST(ColStatDescSharedPtr) // pointers to ColStatDescs that were EQ-merged
               nonVegEquals_;      // to create this ColStatDesc, even though they
  //                               // were not both contained in a single VEG.
  NABoolean    fromInnerTable_; // used in nonVegEquals_ related OR processing
  ValueIdSet   appliedPreds_;   // All Predicates applied to this ColStats
  ColStatsSharedPtr colStats_;  // reference to ColStats structure
  NABoolean    modified_;       // FALSE => the Colstats structure has not yet
  //                            // been modified
  CostScalar  inputCard_;	// any input cardinality which could be reflected
				// in this colStat
};

// -----------------------------------------------------------------------
//
// class ColStatDescList : a LIST of ColStatDescSharedPtr's
//
// Since a ColStatDesc is the histogram for a single table column, a CSDL
// is the histograms modelling the columns for an entire table (or node in
// a query plan, i.e., a "virtual table", one produced from multiple
// tables joined together)
//
// -----------------------------------------------------------------------


class ColStatDescList: public SHPTR_LIST (ColStatDescSharedPtr)
{
public:

  ColStatDescList (NAMemory* h=0) : SHPTR_LIST(ColStatDescSharedPtr)(h), uecList_(NULL),
    useCapForLowBound_ (FALSE),
    joinOnSingleCol_(FALSE),
    scanRowCountWithoutHint_ (-1.0),
    mcSkewedValueLists_(NULL)
    {}

  ColStatDescList (const ColStatDescList & other, NAMemory* h/*=0*/) :
    SHPTR_LIST(ColStatDescSharedPtr)(other,h),
    uecList_(other.uecList_),
    useCapForLowBound_ (other.useCapForLowBound_),
    joinOnSingleCol_ (other.joinOnSingleCol_),
    scanRowCountWithoutHint_(other.scanRowCountWithoutHint_),
    joinedCols_(other.joinedCols_), 
    mcSkewedValueLists_(other.mcSkewedValueLists_)
    {}

  // should the destructor do anything?  for now, no
  virtual ~ColStatDescList () {}

  // Returns TRUE if at least one of the histograms is a
  // fake histogram. A fake histogram is a histogram that
  // was synthesized by ColStats from info. other than statistics.
  // We need to test for this because we don't want to cost
  // MDAM relying on fake histograms. MDAM must not be chosen
  // when there are fake histograms.
  NABoolean containsAtLeastOneFake() const;

  NABoolean selectivityHintApplied() const;

  // Returns TRUE if the given Column is contained in this
  // ColStatDescList. The ColStatDesc could have also been merged
  // with the other colStatDesc

  NABoolean contains(const ValueId & column) const;

  // Returns TRUE if the full column set is contained in this
  // ColStatDescList. 

  NABoolean contains(const ValueIdList & colList) const;

  // Returns the (possibly multi-column) uec value for the
  // ValueId-specified column(s) in the parameter list.
  //
  // NB: in the case where we cannot find uec information for one or more
  // of 'columns' (i.e., histogram isn't available), this method returns
  // -1.  Anyone using this method should check for this value!
  CostScalar getAggregateUec (const ValueIdSet & columns) const ;

  // set base uec for all columns in their colAnalysis
  void setBaseUecForAllCols();

  void setScaleFactor(CostScalar val);

  // getColStatDescIndexForColWithMaxUec(leftColIndex, leftLeafValues)
  // From the given ValueIdSet, the method returns the index of the histogram
  // with max UEC
  NABoolean getColStatDescIndexForColWithMaxUec(CollIndex & leftColIndex,
        const ValueIdSet & leftLeafValues) const;

  void addToAppliedPredsOfAllCSDs(const ValueIdSet & colSet,
        const ValueId & newPredicate);

  // some usages of CSDL want to be able to create and then delete individual
  // CSDL's -- for these users, we have an explicit destroy function
  void destroy () ;

  inline ColStatDescList & operator = (const ColStatDescList &other)
    {
      this->SHPTR_LIST(ColStatDescSharedPtr)::operator = ( other );
      uecList_ = other.uecList_ ;
      mcSkewedValueLists_ = other.mcSkewedValueLists_;
      return *this;
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
  void appendDeepCopy   (const ColStatDescList & source,
                         const CollIndex firstN,
                         const CostScalar & scale = 1,
                         const NABoolean shapeChangedMask = TRUE) ;

  void makeDeepCopy     (const ColStatDescList & source,
                         const CostScalar & scale = 1,
                         const NABoolean shapeChangedMask = TRUE) ;

  void prependDeepCopy  (const ColStatDescList & source,
                         const CollIndex firstN,
                         const CostScalar & scale = 1,
                         const NABoolean shapeChangedMask = TRUE) ;

  void insertDeepCopy   (const ColStatDescSharedPtr & source,
                         const CostScalar & scale = 1,
                         const NABoolean shapeChangedMask = TRUE) ;

  void insertDeepCopyAt (const CollIndex entry,
                         const ColStatDescSharedPtr & source,
                         const CostScalar & scale = 1,
                         const NABoolean shapeChangedMask = TRUE) ;
  void makeMappedDeepCopy(
                         const ColStatDescList & source,
                         ValueIdMap &map, // map source "up"
                         NABoolean includeUnmappedColumns);

  void removeDeepCopyAt (const CollIndex entry) ;

  void computeMaxFreq(NABoolean forced = FALSE);

  // add colStatDesc for a virtual column in this colStatDescList
  // This will be used for cases like inserts, transpose or rowsets,
  // where the column is being equated to a constant or the right child
  // of the join is a constant.

  void addColStatDescForVirtualCol(const CostScalar & uec,
				    const CostScalar & rowCount,
				    const ValueId  colId,
				    const ValueId vegCol,
				    const ValueId mergeState,
                                    const RelExpr * expr,
                                    NABoolean defineVirtual = TRUE);

  // ---------------------------------------------------------------------
  // Method for estimating the Cardinality, given a set of predicates
  // and column statistics.
  //
  // estimateCardinality takes special actions when it is invoked against
  // the children of ITM_AND and ITM_OR operators.
  // ---------------------------------------------------------------------
  CostScalar estimateCardinality (const CostScalar & initalRowCount,
				  const ValueIdSet & setOfPredicates,
				  const ValueIdSet & outerReferences,
                                  const Join * expr,
				  const SelectivityHint * selHint,
				  const CardinalityHint * cardHint,
				  CollIndex & numOuterColStats,
				  ValueIdSet & unresolvedPreds,
				  MergeType mergeMethod =
				  INNER_JOIN_MERGE,
				  OperatorTypeEnum exprOpCode =
				  ITM_FIRST_ITEM_OP , // no-op value
                                  CostScalar *maxSelectivity=NULL);


  // Adjust the rowcount based on the cardinality / selectivity / count(*) hint
  CostScalar adjustRowcountWithHint(const CardinalityHint * cardHint, 
                                    const SelectivityHint * selHint, 
				    const ValueIdSet & setOfPredicates,
                                    CostScalar & newRowCount, 
				    const CostScalar & initialRowCount);

  void copyAndScaleHistograms(CostScalar scale);

  void setBaseUecToTotalUec();

  CostScalar getMaxFreq(ValueId col);

  CostScalar getUEC(ValueId col);

  // get maximum frequency for the given column set
  CostScalar getMaxOfMaxFreqOfCol(const ValueIdSet & baseColSet) ;

  // get maximum frequency for the given column set
  CostScalar getMinOfMaxFreqOfCol(const ValueIdSet & baseColSet) ;

  // get max frequency of the leaves of Case expression
  CostScalar getMaxFreqForCaseExpr(const ValueIdSet & leafValues);

  void addToJoinedCols (const ValueIdSet & newPredCols)
  { joinedCols_.insert( newPredCols ); }

  void clearJoinedCols ()
  { joinedCols_.clear(); }

  ValueIdSet getJoinedCols() { return joinedCols_; }

  CostScalar getUecOfJoiningCols(ValueIdSet & joinedColSet) const;

// Returns the minimum UEC from the given column set
  CostScalar getMinUec(const ValueIdSet & baseColSet) const;

// Returns the maximum UEC from the given column set
  CostScalar getMaxUecForCaseExpr(const ValueIdSet & baseColSet) const;

  // Returns the maximum UEC from the given leaf value set
  CostScalar getMaxUec(const ValueIdSet & leafValueSet) const;


  // returns cardinality of busiest stream based on the given list
  // of histograms

  CostScalar getCardOfBusiestStream(const PartitioningFunction* partFunc,
						Lng32 numOfParts,
						GroupAttributes  * grpAttr,
						Lng32 countOfCPUs = 1);

 CostScalar  getCardOfBusiestStreamForUnderNJ(CANodeIdSet * outerNodeSet,
                                              const PartitioningFunction* pf,
                                              Lng32 numOfParts,
                                              GroupAttributes * gr,
                                              Lng32 countOfCpus = 1);
  
 void addRecentlyJoinedCols(CollIndex startIdx,
                            CollIndex stopIdx);

 void compressColStatsToSingleInt();

 void insertByPosition(const StatsList & other,
                       const NAColumnArray &columnList, 
                       const ValueIdList &tableColList);

private:
  // ---------------------------------------------------------------------
  // The following five routines are private subroutines of
  // estimateCardinality() :
  //
  //   the first three are used to apply different types of predicates to
  //   the ColStatDescList that calls them;
  //
  //   the fourth tries to use multi-column uec information for
  //   adjusting rowcount estimation in the case of applying multiple
  //   predicates for highly correlated columns within the same table.
  //
  //   the fifth tries to use multi-column uec information for
  //   multiple-column joins.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Given a VEG predicate, merge all histograms belonging to the same
  // equivalence class.
  NABoolean applyVEGPred (ItemExpr *VEGpred,
                          CostScalar & newRowcount,
			  CollIndex & numOuterColStats,
			  MergeType mergeMethod = INNER_JOIN_MERGE,
			  OperatorTypeEnum exprOpCode = ITM_FIRST_ITEM_OP,
                          CostScalar *maxSelectivity=NULL);
  // ---------------------------------------------------------------------
  // Apply a bi-relational predicate to the set of column statistics.
  NABoolean applyPred (ItemExpr *biRelatpred,
                       CostScalar & newRowcount,
		       CollIndex & numOuterColStats,
		       MergeType mergeMethod = INNER_JOIN_MERGE,
                       OperatorTypeEnum exprOpCode = ITM_FIRST_ITEM_OP,
                       CostScalar *maxSelectivity=NULL);

  // ---------------------------------------------------------------------
  // Apply a predicate having 'default' selectivity to the given column
  // statistics.
  void applyDefaultPred (ItemExpr * pred, CostScalar & newRowcount,
                         OperatorTypeEnum exprOpCode = ITM_FIRST_ITEM_OP,
                         CostScalar *maxSelectivity=NULL);

  // ---------------------------------------------------------------------
  // Apply BiLogicPreds ITM_OR and ITM_AND

  CostScalar applyBiLogicPred(CostScalar & tempRowCount,
			      ValueIdSet & BiLogicPreds,
			      const ValueIdSet & outerReferences,
                                              const Join * expr,
			      const SelectivityHint * selHint,
			      const CardinalityHint * cardHint,
			      CollIndex & numOuterColStats,
			      ValueIdSet & unresolvedPreds,
			      MergeType mergeMethod,
			      NAHashDictionary<ValueId, CostScalar> & biLogicPredReductions,
			      OperatorTypeEnum exprOpCode = ITM_FIRST_ITEM_OP,
			      CostScalar *maxSelectivity=NULL);

  // ---------------------------------------------------------------------
  // Use multi-column uec to find the resulting rowcount from multiple
  // predicates on correlated columns within a single table, if possible.
  void useMultiUecIfCorrelatedPreds (
     CostScalar & newRowcount,		  // in/out
     const CostScalar & oldRowcount,	  // in
     CollIndex predCount,		  // in : quick check : proceed if >=2
     const CollIndexList &joinHistograms, // in : histograms used in MC Join
     CollIndex startIndex,		  // in : 1st idx of CSDL to look at
     CollIndex stopIndex,		  // in : idx of CSDL+1 to look at
     NAHashDictionary<ValueId, CostScalar> & biLogicPredReductions);

  // ---------------------------------------------------------------------
  // Use multi-column uec to find the resulting rowcount from a
  // multi-column join between two tables, if possible.
  void useMultiUecIfMultipleJoins (
     CostScalar & newRowcount,        /* in/out */
     const CostScalar & oldRowcount,  /* in */
     CollIndex startIndex,            /* in : first index of CSDL */
     CollIndex stopIndex,             /* in : last index of CSDL+1 */
     CollIndexList & joinHistograms,  /* out */
     const Join * expr,
     MergeType mergeMethod
     );

  // ---------------------------------------------------------------------
  void computeRowRedFactor(MergeType mergeMethod, 
                            CollIndex numOuterColStats,
                            CostScalar rowcountBeforePreds,
                            CollIndex & predCountSC, 
                            CollIndex & predCountMC, 
                            CostScalar & rowRedProduct);

public:

    CostScalar getHighestUecReductionByLocalPreds(ValueIdSet &cols) const;

  // synchronize the RowCount of all the histograms in the list (those
  // with array indices from 0..loopLimit-1)
  void synchronizeStats ( const CostScalar & baseRowcount,
                          const CostScalar & newRowcount,
			  CollIndex loopLimit ) ;

  // this version doesn't care what the original rowcount was originally
  // supposed to be -- it just does the work of setting all histograms to
  // have newRowcount as the rowcount
  void synchronizeStats ( const CostScalar & newRowcount,
                          CollIndex loopLimit ) ;

  // Used only by Join::synthEstLogProp to do inner-equi-joins of any
  // columns appearing as outer references from both children of the
  // join
  CostScalar mergeListPairwise() ;

  // Used for mapping a histogram to a range-partitioned table so that we
  // can determine which partitions in a query are active
  // -- returns TRUE if everything's OK, FALSE otherwise
  NABoolean divideHistogramAtPartitionBoundaries
     (const ValueIdList            & listOfPartKeys,     /*in*/
      const ValueIdList            & listOfPartKeyOrders,/*in*/
      const LIST(EncodedValueList *) & listOfPartBounds,   /*in*/
      ValueId       & keyCorrespondingToOutputRows,      /*out*/
      NABoolean     & isKeyAscending,                    /*out*/
      ColStats      & outputRows,                        /*out*/
      CollIndexList & outputFactors) const ;             /*out*/


  // ------------------------------------------------------------------------
  // methods to access individual CSD's within the CSDL via various
  // indexing methods (e.g., via ValueId, ...)
  //
  // Currently we have six of these :
  // . getColStatDescIndexForColumn()
  // . getColStatDescIndex()
  // . getColStatsPtrForColumn()
  // . getColStatsPtrForPredicate()
  // . getColStatsPtrForVEGGroup()
  // . getSingleColStatsForVEGPred()
  // ------------------------------------------------------------------------

  NABoolean getColStatDescIndexForColumn (CollIndex& index, /* output */
                                          const ValueId& column) const ;

  NABoolean getColStatDescIndexForColumn (CollIndex& index, /* output */
                                          const ValueId& column,
                                          NAColumnArray& partKeyColArray) const ;

  // Get the Index of the ColStatDesc which has the given
  // valueID as a VEGColumn.  Does not assume that 'value'
  // must be a BASECOL, VEGREF, etc.
  //
  NABoolean getColStatDescIndex (CollIndex& index, /* output */
                                 const ValueId& value) const ;

  // This is used in the scan costing:
  // Returns the ColStats corresponding to the given column
  // This should be used to find the single column histogram
  // associated with columns. It returns NULL if the
  // histogram does not exist.
  ColStatsSharedPtr getColStatsPtrForColumn (const ValueId& column) const ;

  ColStatsSharedPtr getColStatsPtrForPredicate (const ValueId& predicate) const ;

  ColStatsSharedPtr getColStatsPtrForVEGGroup (const ValueIdSet& VEGGroup) const ;

  // This is used in join costing:
  // Given one VEG predicate, retrieve the first single column ColStats with
  // a column in the VEG predicate.
  ColStatsSharedPtr getSingleColStatsForVEGPred (const ValueId& VEGPred) const ;


  // ------------------------------------------------------------------------
  // internal consistency checking
  // ------------------------------------------------------------------------

  // Verify that the CSDL's internal semantics are being maintained!
  // --> loop from array index startIndex..endIndex-1
  void verifyInternalConsistency (CollIndex startIndex, CollIndex endIndex) const ;

  // Enforce the CSDL's internal semantics if they're not being maintained!
  // --> loop from array index startIndex..endIndex-1
  // --> if "printNoStatsWarning" is TRUE, then print the 6008's re: no stats
  // when there should be
  void enforceInternalConsistency (CollIndex startIndex,
                                   CollIndex endIndex,
                                   NABoolean printNoStatsWarning = FALSE) ;


  // ------------------------------------------------------------------------
  // looking at aggregate attributes over all ColStatDesc's in the CSDL
  // ------------------------------------------------------------------------

  // return the set of all applied preds to members
  ValueIdSet appliedPreds () const;

  // return the set of all VEGColumns of members
  ValueIdSet VEGColumns () const;


  // ------------------------------------------------------------------------
  // displaying debugging information
  // ------------------------------------------------------------------------

  // Display each colstatdesc in the list:
  void print (ValueIdList selectListCols,
              FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "",
              CollHeap *c=NULL, char *buf=NULL,
              NABoolean hideDetail=FALSE) const ;
  void display () const ;

  // used by RelExpr::showQueryStats()
  void showQueryStats(CollHeap *c, char *buf, ValueIdList selectListCols) const
    { print(selectListCols, stdout, DEFAULT_INDENT, "", c, buf,
            TRUE/*hideDetail*/); 
    }

  // a utility routine used by mergeStats and applyDefaultPred
  NABoolean identifyMergeCandidates(ItemExpr * VEGpred,
				    CollIndex & rootStatIndex,
				    CollIndexList & statsToMerge) const ;

  // It is a helper method used while computing left joins. The method locates
  // histograms that have been joined to right child using inner join and
  // now need to be null augmented to simulate the left join
  // ---------------------------------------------------------------------------
  NABoolean
  locateHistogramToNULLAugment(ValueIdSet EqLocalPreds, 
                                NAList<CollIndex> &statsToMerge, 
                                CollIndex &rootStatIndex, 
                                CollIndex outerRefCount);

  // --------------------------------------------------------------------
  // It is a helper method used by left joins. It merges the rows
  // from the left side that did not match the right child back into 
  // the joined histograms. 
  // -------------------------------------------------------------------
  CostScalar 
  computeLeftOuterJoinRC(NABoolean &foundFlag /*in and out*/,
                         const ColStatDescList &leftColStatsList,
                         CollIndex rootStatIndex);

  // --------------------------------------------------------------------
  // It is a helper method used by full outer joins. It merges the rows
  // from the right side that did not match the left child back into 
  // the joined histograms. 
  // -------------------------------------------------------------------
  CostScalar 
  computeFullOuterJoinRC(NABoolean &foundFlag /*in and out*/,
                        const ColStatDescList &origColStatsList,
                        CollIndex rootStatIndex);

  // ----------------------------------------------------------------------------------
  // This is a helper method used by left joins. 
  // The method is called after the the rows from the left histograms of the joining column,
  // that did not match the right side are merged back into the join result. 
  // In the following method, the histograms from the remaining columns are synchronized
  // to have the same row count
  // --------------------------------------------------------------------------------
  void
  synchronizeHistsWithOJRC(NAList<CollIndex> &statsToMerge, 
                          CollIndex startIndex,
                          CollIndex stopIndex,
                          CollIndex rootStatIndex, 
                          const ColStatDescList &leftColStatsList,
                          CostScalar &oJoinResultRows, 
                          CostScalar &baseRows);

  // -----------------------------------------------------------------------
  // This is a helper method for left joins. It is used to 
  // NULL instantiate the right histogram rows from the other side with NULLs
  // ------------------------------------------------------------------------
  void
  nullInstantiateHists(CollIndex startIndex, 
                      CollIndex stopIndex,
                      CostScalar &oJoinResultRows,
                      ValueIdList &nulledVIds);

  const MCSkewedValueList * getMCSkewedValueListForCols(ValueIdSet cols, ValueIdList &colGroup);
  CostScalar getAvgRowcountForNonSkewedMCValues(ValueIdSet cols, MCSkewedValueList* mCSkewedValueList);

private:
  // a utility routine used by applyPred
  NABoolean identifyMergeCandidates(ItemExpr * operand,
				    CollIndexList & statsToMerge) const ;

  // a utility routine for merging a specified set of members in the given
  // ColStatDescList
  void mergeSpecifiedStatDescs (const CollIndexList & statsToMerge,
                                CollIndex rootIndex,
                                MergeType mergeMethod,
                                CollIndex numOuterColStats,
                                CostScalar & newRowcount,
                                CostScalar & newUec,
                                NABoolean forVEGPred,
				OperatorTypeEnum opType = ITM_FIRST_ITEM_OP) ;

  // Prior to any Join's predicate analysis, a ColStatDescList is built
  // that contains the cross-product of the left and right tables.
  //
  // Knowledge of that cross-producting is incorporated in formulas that
  // evaluate VEG and Equality predicates.


  // We maintain multi-column uec information in order to accurately
  // estimate rowcounts for joins involving multiple predicates, the
  // output of a groupby aggregate, and possibly other cases I'm not
  // thinking of right now.  This data member should be used completely
  // read-only; that is, only one such list exists for all
  // ColStatDescList's, and no CSDL has the rights to modify it.

  MultiColumnUecList * uecList_ ;

  MultiColumnSkewedValueLists * mcSkewedValueLists_;

  // We also maintain a flag to indicate that we could have used multi-column
  // information, but as it is unavailable, we shall put a cap on the lower
  // bound of the join cardinality to ensure that we do not underestimate

  NABoolean useCapForLowBound_;

  // The following flag specifies if the optimizer should use frequency of
  // joining columns to uplift join cardinality. The flag is set only if the
  // join is being done on one column

  NABoolean joinOnSingleCol_;

  // We want to cache the totalRowCount after applying local predicates
  // on a table, without using hints. This will be used in computing the
  // cardinality for an index when some cardinality or selectivity hint
  // is given by the user.

  CostScalar scanRowCountWithoutHint_;

  ValueIdSet joinedCols_;

  // the following are methods for accessing ColStatDescList data members
public:
  inline void setUecList (const MultiColumnUecList * list)
    {
      if ( list != NULL )
        uecList_ = const_cast<MultiColumnUecList*>(list) ;
    }

  inline void insertIntoUecList (const MultiColumnUecList * other)
    {
      if ( uecList_ == NULL )
        setUecList (other) ;
      else
        uecList_->insertList (other) ;
    }

  inline const MultiColumnUecList * getUecList() const   { return uecList_ ; }

  inline MultiColumnUecList * uecList() { return uecList_ ; }

  inline void setMCSkewedValueLists (const MultiColumnSkewedValueLists* list)
    {
      if ( list != NULL )
        mcSkewedValueLists_ = const_cast<MultiColumnSkewedValueLists*>(list) ;
    }

  inline const MultiColumnSkewedValueLists * getMCSkewedValueLists() const   { return mcSkewedValueLists_ ; }

  inline MultiColumnSkewedValueLists * mcSkewedValueLists() { return mcSkewedValueLists_ ; }

  inline NABoolean isCapForLowBound () const       { return useCapForLowBound_; }

  inline void setCapForLowBound (NABoolean flag=TRUE)    { useCapForLowBound_ = flag; }

  inline NABoolean isJoinOnSingleCol () const  { return joinOnSingleCol_; }

  inline void setJoinOnSingleCol (NABoolean flag = TRUE) { joinOnSingleCol_ = flag; }

  void setInputCard (CostScalar rows);

  inline CostScalar getScanRowCountWithoutHint () const       { return scanRowCountWithoutHint_; }

  inline void setScanRowCountWithoutHint (CostScalar scanRowCountWithoutHint)
    { scanRowCountWithoutHint_ = scanRowCountWithoutHint; }
};


#endif /* COLSTATDESC_H */
