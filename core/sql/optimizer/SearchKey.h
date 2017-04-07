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
#ifndef SEARCHKEY_H
#define SEARCHKEY_H
/* -*-C++-*-
******************************************************************************
*
* File:         SearchKey.h
* Description:  A search key
* Created:      07/31/95
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------
#include "ValueDesc.h"
#include "NABasicObject.h"
#include "mdamkey.h"

// -----------------------------------------------------------------------
// contents of this file
//
// A search key is a pair of expressions that together define a range
// of values. Its purpose is to localize the search for specific
// values within a b-tree index.
//
// The methods on SearchKey are the only ones that are "exported"
// interfaces.
//
// -----------------------------------------------------------------------
class SearchKey;
class SearchKeyBounds;
class SearchKeyWorkSpace;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class ItemExpr;
class RangePartitioningFunction;
class TableHashPartitioningFunction;
class RoundRobinPartitioningFunction;
class HHDFSTableStats;
class HHDFSListPartitionStats;
class HHDFSBucketStats;
class HHDFSFileStats;
class HHDFSStatsBase;

// -----------------------------------------------------------------------
// class SearchKey
//
// A SearchKey contains a description of the index on which the
// search key will be used, a list of the index columns, an
// expression that specifies the value to begin the search from
// and another expression that specifies the values to end the
// search at.
//
// Each search expression has a directive associated with it that
// provides a guidance on how the search should proceed.
// -----------------------------------------------------------------------
class SearchKey : public ScanKey
{
public:
  // ---------------------------------------------------------------------
  // SearchKey() Constructor
  //
  // A search key is a pair of expressions that together define a range
  // of values. Its purpose is to localize the search for specific
  // values within a b-tree index.
  //
  // This method is used for building a search key for an operator that
  // will be used for accessing data from the given access path. It
  // receives a set of predicates as an input. It analyzes them and
  // uses eligible ones for forming the search key. It removes the
  // ValueIds of those predicates that it can use for forming the
  // search key form the given set. (This constructor is therefore a
  // function with the side-effect that the setOfPredicates can be
  // modified.)
  //
  // It returns a SearchKey regardless of whether eligible key
  // predicates are found. In such a case, the range to be search
  // is logically equal to the interval (-infinity, +infinity).
  //
  // Parameters:
  //
  // const ValueIdList &  keyColumns
  //    IN : A read-only reference to the list of key columns
  //
  // const ValueIdList &  orderOfKeyValues
  //    IN : A list of expressions that define the ASC/DESC
  //         order of key values. It can be empty.
  //
  // const ValueIdSet & externalInputs
  //    IN : A read-only reference to the external inputs that are
  //         available to the operator that will be used for accessing
  //         data from the access path.
  //
  // const NABoolean forwardSearch
  //    IN : TRUE  => The search in the index will use a top-down
  //                  left-to-right tree walk.
  //         FALSE => right-to-left tree walk
  //
  // ValueIdSet & setOfPredicates
  //    IN : The set of predicates that are to be analyzed for
  //         determining whether some of them can be used for
  //         forming the search key. The set may also contain
  //         assignment operators for columns, but predicates
  //         and assignment operators must not be mixed.
  //    OUT: The set of predicates or assigment operators that are not
  //         used for forming the search key,
  //         i.e., key predicates/operators = IN - OUT.
  //conts  ValueIdSet & nonKeyColumnSet
  //    IN : The set of of non-key columns in the index.
  //         These are used by replicateNonKeyVEGPredicates(...)
  //         to figure out whether a key predicate needs to be
  //         replicated in the executor predicates.
  //
  // Return value
  //    OUT: A SearchKey
  //
  // -----------------------------------------------------------------------
  SearchKey(const ValueIdList & keyColumns,
	    const ValueIdList & orderOfKeyValues,
	    const ValueIdSet & externalInputs,
	    const NABoolean forwardSearch,
	    ValueIdSet & setOfPredicates,
            const ValueIdSet& nonKeyColumnSet,
            const IndexDesc *indexDesc,
            const IndexDesc *UDindexDesc = NULL);


  // -----------------------------------------------------------------------
  //  The class hierarchy for the SearchKey and MdamKey is as
  //follows:
  //
  //                ScanKey
  //                  /\
  //                 /  \
  //         SearchKey  MdamKey
  //
  //Thus a SearchKey IS-A ScanKey and a MdamKey IS-A ScanKey.
  //I designed this class hierarchy early in 97 to reflect the
  //fact that both SearchKey and MdamKey are keys. Thus, I
  //put common behaviour in the abstract class ScanKey.
  // One thing that both SearchKey and MdamKey need is
  // access to the selection predicates in MDNF (Mdam
  // disjunctive normal form). Up to today, the SearchKey
  // was transforming the selection predicates to MDNF
  //  "under the covers" in its constructor (see the
  // construction of "MaterialDisjuncts" below):
  //
  // SearchKey::SearchKey(const ValueIdList & keyColumns,
  // 		     const ValueIdList & orderOfKeyValues,
  // 		     const ValueIdSet & externalInputs,
  // 		     const NABoolean forwardSearch,
  // 		     ValueIdSet & setOfPredicates)
  //      :ScanKey(keyColumns,
  // 	      externalInputs,
  // 	      *(new HEAP MaterialDisjuncts(setOfPredicates))),
  //       keyColumns_(keyColumns),
  //       beginKeyValues_(keyColumns.entries()),
  //       endKeyValues_(keyColumns.entries())
  //
  // Since construction of these disjuncts is expensive, I
  // added a new constructor that passes the Disjuncts
  // when one is available, also I added a method "init()"
  // to SearchKey to contain the code common to both
  // constructors. Note that ScanKey keeps a reference
  // to the Disjunct instance that it is passed and it does
  // not copy it by value, thus the instance of the Disjunct's
  // passed must be obtained from the HEAP.
  // -----------------------------------------------------------------------
  SearchKey(const ValueIdList & keyColumns,
	    const ValueIdList & orderOfKeyValues,
	    const ValueIdSet & externalInputs,
	    const NABoolean forwardSearch,
	    ValueIdSet & setOfPredicates,
            const Disjuncts& associatedDisjuncts,
            const ValueIdSet& nonKeyColumnSet, 
            const IndexDesc *indexDesc,
            const IndexDesc *UDindexDesc = NULL);

  // -----------------------------------------------------------------------
  // In certain cases we would like to use a single subset search key
  // to filter data from a particular partition of a range partitioning
  // scheme. The partitioning key predicate may be too complicated for
  // a single subset scheme, however (it's a multi-valued predicate).
  // To make this work, despite the complex predicate, a new constructor
  // is used that simply sets up begin and end key without going through
  // predicate analysis.
  // -----------------------------------------------------------------------
  SearchKey(const ValueIdList & keyColumns,
	    const ValueIdList & orderOfKeyValues,
	    const ValueIdSet & externalInputs,
	    const NABoolean forwardSearch,
	    ValueIdSet & setOfPredicates,
	    const RangePartitioningFunction *rpf,
            const ValueIdSet& nonKeyColumnSet, 
            const IndexDesc *indexDesc,
            const IndexDesc *UDindexDesc = NULL);

  // -----------------------------------------------------------------------
  // In certain cases we would like to use a single subset search key
  // to filter data from a particular partition of a range partitioning
  // scheme. The partitioning key predicate may be too complicated for
  // a single subset scheme, however (it's a multi-valued predicate).
  // To make this work, despite the complex predicate, a new constructor
  // is used that simply sets up begin and end key without going through
  // predicate analysis.
  // -----------------------------------------------------------------------
  SearchKey(const ValueIdList & keyColumns,
	    const ValueIdList & orderOfKeyValues,
	    const ValueIdSet & externalInputs,
	    ValueIdSet & setOfPredicates,
	    const TableHashPartitioningFunction *hpf,
            const ValueIdSet& nonKeyColumnSet, 
            const IndexDesc *indexDesc,
            const IndexDesc *UDindexDesc = NULL);

  // -----------------------------------------------------------------------
  // In certain cases we would like to use a single subset search key
  // to filter data from a particular partition of a range partitioning
  // scheme. The partitioning key predicate may be too complicated for
  // a single subset scheme, however (it's a multi-valued predicate).
  // To make this work, despite the complex predicate, a new constructor
  // is used that simply sets up begin and end key without going through
  // predicate analysis.
  // -----------------------------------------------------------------------
  SearchKey(const ValueIdList & keyColumns,
	    const ValueIdList & orderOfKeyValues,
	    const ValueIdSet & externalInputs,
	    ValueIdSet & setOfPredicates,
	    const RoundRobinPartitioningFunction *hpf,
            const ValueIdSet& nonKeyColumnSet, 
            const IndexDesc *indexDesc,
            const IndexDesc *UDindexDesc = NULL);

  // -----------------------------------------------------------------------
  // Code common to SearchKey constructors is put in the init method:
  // -----------------------------------------------------------------------
  void init(const ValueIdList & keyColumns,
		     const ValueIdList & orderOfKeyValues,
		     const ValueIdSet & externalInputs,
		     const NABoolean forwardSearch,
		     ValueIdSet & setOfPredicates);


  // -----------------------------------------------------------------------
  // Method for accessing the lower and upper bound values.
  // -----------------------------------------------------------------------
  const ValueIdList & getBeginKeyValues() const
                                               { return beginKeyValues_; }
  const ValueIdList & getEndKeyValues() const
                                                 { return endKeyValues_; }

  // -----------------------------------------------------------------------
  // Method for checking whether two key values are the same and both point
  // at the same partition.
  // The second argument determines the number of key value components
  // (0 to n-1)  to check.
  // -----------------------------------------------------------------------
  NABoolean areKeyValuesIdentical(const SearchKey* other, CollIndex n) const;

  // Methods to check whether the key range is inclusive or exclusive.
  // Note that if there is an expression to determine exclusivity, then the
  // corresponding method isXxxxKeyExclusive() returns an undefined value.
  // -----------------------------------------------------------------------
  inline NABoolean isBeginKeyExclusive() const { return beginKeyIsExclusive_; }
  inline NABoolean isEndKeyExclusive() const     { return endKeyIsExclusive_; }
  ItemExpr *getBeginKeyExclusionExpr() const;
  ItemExpr *getEndKeyExclusionExpr() const;
  // -----------------------------------------------------------------------
  // Method for computing the default lower and upper bound for this key col
  // Used when a SearchKeyBounds object is not needed. For example,
  // when adding the partition search keys.
  // -----------------------------------------------------------------------
  ValueId computeMissingKeyValue(ValueId keyColumn,
                                 NABoolean wantMinValue);

  // -----------------------------------------------------------------------
  // Method for accessing the key predicates.
  // -----------------------------------------------------------------------
  const ValueIdSet & getKeyPredicates() const
                                                { return keyPredicates_; }
  ValueIdSet & keyPredicates()                  { return keyPredicates_; }

  const ValueIdSet & getFullKeyPredicates() const  { return fullKeyPredicates_; }

  // ---------------------------------------------------------------------
  // Method for determining whether the key predicates are unique.
  // ---------------------------------------------------------------------
  NABoolean isUnique() const { return isUnique_; }

  NABoolean areAllBeginKeysMissing() const {return allBeginKeysMissing_;};
  NABoolean areAllEndKeysMissing() const {return allEndKeysMissing_;};

  NABoolean areAllChosenPredsEqualPreds() const {return allChosenPredsAreEqualPreds_;};
  NABoolean areAllKeysConstants(NABoolean convCheck) const;
  NABoolean isNarrowNeeded() const;

  // -----------------------------------------------------------------------
  // Methods inherited from ScanKey to support histogram costing
  // -----------------------------------------------------------------------

  // There is only ONE key disjunct in a SearchKey:
  CollIndex getKeyDisjunctEntries() const;

  // This method returns the key predicates present in the
  // common predicates of the disjuncts obtained from the
  // scan's selection predicates:
  void getKeyPredicatesByColumn(ColumnOrderList& colOrdLis,
				CollIndex disjunctNumber = 0) const;

  // Obtain the set of key predicates in the disjunct:
  //for SearchKey allKeyPredicates and disjunctNumber are
  //insignificant, but the signature of the function had to
  //be changed because the signature was also changed in
  //the parent class ScanKey
  void getKeyPredicates(ValueIdSet &keyPredicates, /* out */
			NABoolean  *allKeyPredicates = NULL,
                        CollIndex disjunctNumber = 0) const;

  // Generate the data structures needed by the generator and
  // rewrite VEG preds and predicates:
  virtual void preCodeGen(ValueIdSet& executorPredicates,
			  const ValueIdSet& selectionPredicates,
			  const ValueIdSet & availableValues,
			  const ValueIdSet & inputValues,
			  VEGRewritePairs * vegPairsPtr,
			  NABoolean replicateExpression = FALSE,
			  NABoolean partKeyPredsAdded = FALSE);

  // Replace the hostvariables that have changed during optimization.
  //  More than one set of host variables may have been created for
  //  logical subpartitioning under different circumstances.  Make
  //  sure all nodes are updated to use the right ones.
  virtual void replaceBegEndPivs(ValueIdSet & oldPivs,
                                  ValueIdSet & newPivs);


  inline NABoolean valuesArePartitionRange() const
  { return valuesArePartitionRange_; }

  // -----------------------------------------------------------------------
  // Methods for debugging
  // -----------------------------------------------------------------------
  void display() const;

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "SearchKey") const;

  bool isFullScanPresent()
  {		//if we retrieved values twice for each entry, this is a full scan
	  return (_countTimesBoundaryValReq == 2 * beginKeyValues_.entries());
  }

  UInt32 _countTimesBoundaryValReq; //initially 0, counts the number of times we attempted
  //to retrieve a boundary value

  Int32 getCoveredLeadingKeys() const { return coveredLeadingKeys_; }

protected:
  void computeCoveredLeadingKeys();
  void computeFullKeyPredicates(ValueIdSet& predicates);

private:

  // ***********************************************************************
  // SearchKey - Private data.
  // ***********************************************************************

  // -----------------------------------------------------------------------
  // Values to start the search with.
  // -----------------------------------------------------------------------
  ValueIdList beginKeyValues_;

  // -----------------------------------------------------------------------
  // Values to terminate the search with.
  // -----------------------------------------------------------------------
  ValueIdList endKeyValues_;

  // -----------------------------------------------------------------------
  // The set of key predicates that are derived from the common predicate,
  // computed by computeCommonPredicates() and is an intersection of all 
  // predicates in DisjunctArray). During the computation, the range spec is 
  // examined and only those NOT in OR form are included in the common 
  // predicate. For example, the IN list in the following query is an OR 
  // predicate,
  //    select * from t010t2 where a in (1,4) and b='a' and c = 1;
  // the predicate "a in (1,4)" will not be part of the common predicate.
  // -----------------------------------------------------------------------
  ValueIdSet keyPredicates_;

  // -----------------------------------------------------------------------
  // The set of full key predicates that are derived from the selection
  // predicates.  This set includes every predicate that refers to any key
  // columns. 
  // -----------------------------------------------------------------------
  ValueIdSet fullKeyPredicates_;

  // ---------------------------------------------------------------------
  // Boolean indicating whether key predicates formulate a unique key.
  // ---------------------------------------------------------------------
  NABoolean  isUnique_;

  // ---------------------------------------------------------------------
  // Is the key range inclusive or exclusive on the begin/end key side?
  // (NOTE: if this is not known at compile time, expressions to compute
  // the values can be specified below)
  // ---------------------------------------------------------------------
  NABoolean beginKeyIsExclusive_;
  NABoolean endKeyIsExclusive_;

  // ---------------------------------------------------------------------
  // Optional dynamic indicators for inclusive/exclusive begin/end keys
  // ---------------------------------------------------------------------
  ValueId beginKeyExclusionExpr_;
  ValueId endKeyExclusionExpr_;

  NABoolean valuesArePartitionRange_;

  // TRUE, if no key predicates were used to construct begin keys.
  //       Min(asc) or Max(desc) values were generated.
  NABoolean allBeginKeysMissing_;
  // TRUE, if no key predicates were used to construct end keys.
  //       Max(asc) or Min(desc) values were generated.
  NABoolean allEndKeysMissing_;
  
  // set to true if all explicitely specified key preds are equality
  // predicates.
  NABoolean allChosenPredsAreEqualPreds_;

  Int32 coveredLeadingKeys_;
  //  NABoolean searchOnKeyColumnsOnly_;

}; // class SearchKey

// *************************************************************************
// Classes that are used internally by the key builder.
// *************************************************************************

// -------------------------------------------------------------------------
// class SearchKeyBounds
// This class is used by the key builder for maintaining intermediate
// status of processing per key column. The intention is to encapsulate
// different policies for choosing key predicates within the implementation
// that is provided for this class. For example, if a measure such as the
// "selectivity" (or uec) of the key predicate is computed, one could
// implement the policy of choosing the most selective inequality predicates
// for defining the lower and upper bounds, respectively.
//
// -------------------------------------------------------------------------
class SearchKeyBounds : public NABasicObject
{
public:

  // -----------------------------------------------------------------------
  // Default constructor
  // -----------------------------------------------------------------------
  SearchKeyBounds(const ValueId & keyId,
                  NABoolean ascDescFlag,
                  NABoolean forwardReverseFlag,
                  const SearchKey& searchKey)
    : keyColumnId_(keyId),
      ascendingOrder_(ascDescFlag), forwardSearch_(forwardReverseFlag),
      beginKeyBoundType_(BOUND_TYPE_NOT_SET),
      beginKeyPredicate_(NULL_VALUE_ID), beginKeyValue_(NULL_VALUE_ID),
      beginValueIsExclusive_(FALSE),
      endKeyBoundType_(BOUND_TYPE_NOT_SET),
      endKeyPredicate_(NULL_VALUE_ID), endKeyValue_(NULL_VALUE_ID),
      endValueIsExclusive_(FALSE),
      searchKey_(searchKey)
  {}

  // -----------------------------------------------------------------------
  // A member of a Collection class must have a const == operator
  // defined for it.
  // -----------------------------------------------------------------------
  NABoolean operator == (const SearchKeyBounds &other) const;

  // -----------------------------------------------------------------------
  // An enumeration of the types of bounds that can be specified by a
  // predicate. For example,
  //   BOUND_TYPE_LOWER : is specified by a >, >= predicate
  //   BOUND_TYPE_EQUAL : is specified by an = predicate
  //   BOUND_TYPE_UPPER : is specified by a <, <= predicates
  // -----------------------------------------------------------------------
  enum BoundType
    {
      BOUND_TYPE_NOT_SET,
      BOUND_TYPE_LOWER,
      BOUND_TYPE_EQUAL,
      BOUND_TYPE_UPPER
    };

  // -----------------------------------------------------------------------
  // Methods for storing and accessing the ValueId for the key column.
  // -----------------------------------------------------------------------
  void setKeyColumnId(const ValueId & exprId)     { keyColumnId_ = exprId; }

  const ValueId & getKeyColumnId() const            { return keyColumnId_; }

  void setIndexOrderToAscending(const NABoolean indexOrder)
                                           { ascendingOrder_ = indexOrder; }

  void setSearchDirection(const NABoolean forwardOrReverse)
                                      { forwardSearch_ = forwardOrReverse; }

  // -----------------------------------------------------------------------
  // SearchKeyBounds::analyzeSearchPredicates()
  //
  // Parameters:
  //
  // const ValueIdSet & setOfPredicates
  //    IN : The set of predicates that are to be analyzed for
  //         determining whether some of them can be used for
  //         forming the search key.
  //
  // const ValueIdSet & externalInputs
  //    IN : A read-only reference to the external inputs that are
  //         available to the operator that will be used for accessing
  //         data from the access path.
  //
  // -----------------------------------------------------------------------
  void analyzeSearchPredicates(const ValueIdSet & setOfPredicates,
			       const ValueIdSet & externalInputs);

  // -----------------------------------------------------------------------
  // SearchKeyBounds::computeBeginKeyAndEndKeyValues()
  // -----------------------------------------------------------------------
  void computeBeginKeyAndEndKeyValues(ValueIdSet & setOfKeyPredicates,
				      ValueIdSet & setOfNonKeyPredicates,
				      NABoolean & previousPredsAreEqualPreds,
				      NABoolean & beginKeyIsExclusive,
				      NABoolean & endKeyIsExclusive,
				      ValueId & beginKeyExclusionExpr,
				      ValueId & endKeyExclusionExpr,
				      NABoolean & beginKeyMissing,
				      NABoolean & endKeyMissing,
				      NABoolean &allchosenPredsAreEqualPreds);

  // ---------------------------------------------------------------------
  // accessor methods
  // ---------------------------------------------------------------------
  const ValueId & getBeginKeyValue() const { return beginKeyValue_; }

  const ValueId & getEndKeyValue() const   { return endKeyValue_; }

  ValueId getBeginExclusionExpr() const      { return beginExclusionExpr_; }
  ValueId getEndExclusionExpr() const          { return endExclusionExpr_; }

  NABoolean isBeginValueExclusive() const { return beginValueIsExclusive_; }
  NABoolean isEndValueExclusive() const     { return endValueIsExclusive_; }

  // -----------------------------------------------------------------------
  // Accessor functions for the index order and search direction.
  // -----------------------------------------------------------------------
  const NABoolean isAscendingOrder() const       { return ascendingOrder_; }

  const NABoolean isForwardSearch() const         { return forwardSearch_; }


  // -----------------------------------------------------------------------
  // Accessor functions for the getting begin/end key predicates
  // -----------------------------------------------------------------------
  const ValueId & getBeginKeyPredicate() const { return beginKeyPredicate_; }

  const ValueId & getEndKeyPredicate() const     { return endKeyPredicate_; }

  // -----------------------------------------------------------------------
  // Methods for debugging
  // -----------------------------------------------------------------------
  void display() const;

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "SearchKeyBounds") const;

private:
  const SearchKey& getSearchKey() const
  { return searchKey_; }

  // ***********************************************************************
  // SearchKeyBounds - Private methods.
  // ***********************************************************************

  // -----------------------------------------------------------------------
  // Depending on the type of comparison that is performed by the
  // predicate use the value that is referenced in it and apply a
  // bound for the search.
  // -----------------------------------------------------------------------
  void applyABoundForTheSearch(const ValueId & predId,
			       const ValueId & referencedInput,
			       const ValueId & intervalExclusionExpr);

  // -----------------------------------------------------------------------
  // Depending on the type of comparison that is performed by the
  // predicate, decide whether it will provide a lower bound,
  // an upper bound or both.
  // -----------------------------------------------------------------------
  BoundType getBoundType(const ValueId & predId,
			 const ValueId & referencedInput) const;

  // ---------------------------------------------------------------------
  // Find out whether the bound produced by this predicate is inclusive
  // (FALSE) or exclusive (TRUE). Return FALSE if this is an equal
  // comparison or if this information is being determined at run time.
  // ---------------------------------------------------------------------
  static NABoolean isBoundExclusive(const ValueId & predId);

  // -----------------------------------------------------------------------
  // Mutator functions used for initializing the work space.
  // -----------------------------------------------------------------------
  void setBeginKeyBoundType(BoundType boundType)
                                         { beginKeyBoundType_ = boundType; }
  void setBeginKeyPredicate(const ValueId & predId)
                                            { beginKeyPredicate_ = predId; }
  void setBeginKeyValue(const ValueId & exprId)
                                                { beginKeyValue_ = exprId; }

  void setEndKeyBoundType(BoundType boundType)
                                           { endKeyBoundType_ = boundType; }
  void setEndKeyPredicate(const ValueId & predId)
                                              { endKeyPredicate_ = predId; }
  void setEndKeyValue(const ValueId & exprId)     { endKeyValue_ = exprId; }

  // -----------------------------------------------------------------------
  // Methods for accessing the bound type.
  // -----------------------------------------------------------------------
  NABoolean isBeginKeyChosen() const
                      { return (beginKeyBoundType_ != BOUND_TYPE_NOT_SET); }
  NABoolean isEndKeyChosen() const
                        { return (endKeyBoundType_ != BOUND_TYPE_NOT_SET); }

  BoundType getBeginKeyBoundType() const      { return beginKeyBoundType_; }
  BoundType getEndKeyBoundType() const          { return endKeyBoundType_; }

  // -----------------------------------------------------------------------
  // Methods for accessing the key predicates.
  // -----------------------------------------------------------------------
  NABoolean isBeginKeyPredicateChosen() const
                           { return (beginKeyPredicate_ != NULL_VALUE_ID); }
  NABoolean isEndKeyPredicateChosen() const
                             { return (endKeyPredicate_ != NULL_VALUE_ID); }

  //const ValueId & getBeginKeyPredicate() const
  //                                            { return beginKeyPredicate_; }
  //const ValueId & getEndKeyPredicate() const
  //                                              { return endKeyPredicate_; }

  // -----------------------------------------------------------------------
  // Methods for accessing the key values
  // -----------------------------------------------------------------------
  NABoolean isBeginKeyValueChosen() const
                               { return (beginKeyValue_ != NULL_VALUE_ID); }
  NABoolean isEndKeyValueChosen() const
                                 { return (endKeyValue_ != NULL_VALUE_ID); }

  // -----------------------------------------------------------------------
  // Methods for computing the lower and upper bound for this key column.
  // -----------------------------------------------------------------------
  void computeBeginKeyValue(NABoolean prevBoundWasExclusive);
  void computeEndKeyValue(NABoolean prevBoundWasExclusive);
  ValueId computeMissingKeyValue(NABoolean wantMinValue);

  // ***********************************************************************
  // SearchKeyBounds - Private data.
  // ***********************************************************************

  // -----------------------------------------------------------------------
  // The ValueId for the key column
  // -----------------------------------------------------------------------
  ValueId keyColumnId_;

  // -----------------------------------------------------------------------
  // Invariants for the search that will be performed.
  // NABoolean ascendingOrder
  //    IN : TRUE  => Rows in the index are sorted such that the keys
  //                  are in an ASCENDING sequence.
  //         FALSE => are in an DESCENDING sequence.
  //
  // NABoolean forwardSearch
  //    IN : TRUE  => The search in the index will use a top-down
  //                  left-to-right tree walk.
  //         FALSE => right-to-left tree walk
  //
  // -----------------------------------------------------------------------
  NABoolean ascendingOrder_;  // the index order

  NABoolean forwardSearch_;    // the search direction

  // -----------------------------------------------------------------------
  // The ValueId of an
  //     =, >, >= predicate for scans in the ascending sequence
  //     =, <, <= predicate for scans in the descending sequence
  // It is set to the NULL_VALUE_ID if no such predicate is supplied.
  // -----------------------------------------------------------------------
  BoundType beginKeyBoundType_;

  ValueId beginKeyPredicate_;

  ValueId beginKeyValue_;  // cache the external input

  ValueId beginExclusionExpr_;

  NABoolean beginValueIsExclusive_; // valid only if beginExclusionExpr_ = NULL

  // -----------------------------------------------------------------------
  // The ValueId of an
  //     =, <, <= predicate for scans in the ascending sequence
  //     =, >, >= predicate for scans in the descending sequence
  // It is set to the NULL_VALUE_ID if no such predicate is supplied.
  // -----------------------------------------------------------------------
  BoundType endKeyBoundType_;

  ValueId endKeyPredicate_;

  ValueId endKeyValue_;  // cache the external input

  ValueId endExclusionExpr_;

  NABoolean endValueIsExclusive_; // valid only if endExclusionExpr_ = NULL

  const SearchKey& searchKey_;

}; // class SearchKeyBounds

// -------------------------------------------------------------------------
// class SearchKeyWorkSpace
//
// This class is used by the key builder for maintaining intermediate
// status of processing. It is passed as a parameter between utility
// methods that are used by the key builder. The interfaces for this
// class are internals for the key builder.
// -------------------------------------------------------------------------
class SearchKeyWorkSpace : public NABasicObject
{
public:

  // -----------------------------------------------------------------------
  // Method for allocating the work space.
  //
  // Parameters:
  //
  // const ValueIdList & keyColumns
  //    IN : The key columns of the index.
  //
  // const NABoolean ascendingOrder
  //    IN : TRUE  => Rows in the index are sorted such that the keys
  //                  are in an ASCENDING sequence.
  //         FALSE => are in an DESCENDING sequence.
  //
  // const NABoolean forwardSearch
  //    IN : TRUE  => The search in the index will use a top-down
  //                  left-to-right tree walk.
  //         FALSE => right-to-left tree walk
  //
  // ----------------------------------------------------------------------
  SearchKeyWorkSpace(const ValueIdList & keyColumns,
		     const ValueIdList & indexOrder,
		     const NABoolean forwardSearch,
                     const SearchKey& searchKey);

  ~SearchKeyWorkSpace();

  // ----------------------------------------------------------------------
  // SearchKeyWorkSpace::analyzeSearchPredicates()
  //
  // Parameters:
  //
  // const ValueIdSet & setOfPredicates
  //    IN : The set of predicates that are to be analyzed for
  //         determining whether some of them can be used for
  //         forming the search key.
  //
  // const ValueIdSet & externalInputs
  //    IN : A read-only reference to the external inputs that are
  //         available to the operator that will be used for accessing
  //         data from the access path.
  //
  // ----------------------------------------------------------------------
  void analyzeSearchPredicates(const ValueIdSet & setOfPredicates,
			       const ValueIdSet & externalInputs  );

  // ----------------------------------------------------------------------
  // SearchKeyWorkSpace::computeBeginKeyAndEndKeyValues()
  //
  // Parameters:
  //
  // const ValueIdList &  KeyColumns
  //    IN : List of key columns
  //
  // const ValueIdSet &  externalInputs
  //    IN : Set of external input values
  //
  // ValueIdList &  beginKeyValues
  //    OUT: Each element of this list contains the ValueId of a
  //         scalar expression that specifies a lower bound value.
  //
  // ValueIdList &  endKeyValues
  //    OUT: Each element of this list contains the ValueId of a
  //         scalar expression that specifies an upper bound value.
  //
  // ValueIdSet & setOfKeyPredicates
  //    OUT: The set of predicates that are used for building the key.
  //
  // ValueIdSet & setOfNonKeyPredicates
  //    OUT: The set of predicates that must be evaluated as
  //         selection expressions on rows that are qualified
  //         by the index keys.
  //
  // NABoolean &  keyPredicatesUnique
  //    OUT: Boolean indicating whether set of key predicates specify
  //         a unique row.
  //
  // NABoolean & beginKeyIsExclusive
  //    OUT: Set to FALSE, if the begin key is included in the key range or
  //         if this info is computed dynamically by beginKeyExclusionExpr,
  //         it is set to TRUE if the begin key is excluded from the range.
  //
  // NABoolean & endKeyIsExclusive
  //    OUT: Analogous to beginKeyIsExclusive
  //
  // ValueId & beginKeyExclusionExpr
  //    OUT: If not set to NULL, indicates an expression to be executed
  //         at runtime to get the value of <beginKeyIsExclusive>.
  //
  // ValueId & endKeyExclusionExpr
  //    OUT: analogous to beginKeyExclusionExpr
  //
  // NABoolean & allBeginKeysMissing
  //    OUT: TRUE, if no key predicates were used to construct begin keys.
  //               Min(asc) or Max(desc) values were generated.
  //
  // NABoolean & allEndKeysMissing
  //    OUT: TRUE, if no key predicates were used to construct end keys.
  //               Max(asc) or Min(desc) values were generated.
  //
  // ----------------------------------------------------------------------
  void computeBeginKeyAndEndKeyValues(const ValueIdList & keyColumns,
                                      const ValueIdSet & externalInputs,
                                      ValueIdList & beginKeyValues,
				      ValueIdList & endKeyValues,
				      ValueIdSet & setOfKeyPredicates,
				      ValueIdSet & setOfNonKeyPredicates,
				      NABoolean & keyPredicatesUnique,
				      NABoolean & beginKeyIsExclusive,
				      NABoolean & endKeyIsExclusive,
				      ValueId & beginKeyExclusionExpr,
				      ValueId & endKeyExclusionExpr,
				      NABoolean &allBeginKeysMissing,
				      NABoolean &allEndKeysMissing,
				      NABoolean &allChosenPredsAreEqualPreds);


  const ValueIdSet& getNonKeyColumnSet() const
  { return searchKey_.getNonKeyColumnSet(); }

  const SearchKey& getSearchKey() const
  { return searchKey_; }

private:

  void handleMultiColumnRangePred(NABoolean isBeginKey,
                                  CollIndex keyNum,
                                  ValueIdList & boundaryValues);
  // ----------------------------------------------------------------------
  // The number of key columns processed  in this workspace.
  // ----------------------------------------------------------------------
  Lng32 keyCount_;

  LIST(SearchKeyBounds *)  keyBounds_;

  const SearchKey& searchKey_;

}; // class SearchKeyWorkSpace

// -----------------------------------------------------------------------
// Class HivePartitionAndBucketKey
// This class is used to determine which of the Hive list partitions
// and Hive buckets are actually required in a file scan - based on
// the predicates on that scan. This is somewhat similar to the
// functions that a partition SearchKey provides on a regular SeaQuest
// table.
// -----------------------------------------------------------------------

class HiveFileIterator;

class HivePartitionAndBucketKey : public NABasicObject
{
public:
  HivePartitionAndBucketKey(const HHDFSTableStats *hdfsTableStats,// IN
                            const ValueIdList &hivePartColList,   // IN
                            const ValueIdList &hiveBucketColList, // IN
                            ValueIdSet & setOfPredicates);        // IN/OUT

  const HHDFSTableStats * getHDFSTableStats() const { return hdfsTableStats_; }
  // compute statistics for selected partitions and buckets
  void accumulateSelectedStats(HHDFSStatsBase &result);

  NABoolean isSingleBucketSelected() const
                                         { return selectedSingleBucket_ >= 0; }
  Int32 selectedSingleBucket() const          { return selectedSingleBucket_; }
  // provide (const) access to the bit mask of selected partitions
  const SUBARRAY(HHDFSListPartitionStats *) & getMask() 
                                                { return selectedPartitions_; }

  // iteratively retrieve a list of HDFS files selected by the selection preds
  NABoolean getNextFile(HiveFileIterator &i);

private:

  const HHDFSTableStats *hdfsTableStats_;
  ValueIdList hivePartColList_;
  ValueIdList hiveBucketColList_;
  // TBD: how to process the predicates

  // number of the bucket if predicates select a single bucket,
  // -1 otherwise
  Int32 selectedSingleBucket_;

  // bitmap for Hive partitions that are not eliminated by
  // predicates on Hive partitioning columns
  SUBARRAY(HHDFSListPartitionStats *) selectedPartitions_;
};

// Iterator class to retrieve a list of HDFS files that are
// selectd by the selection predicates

class HiveFileIterator
{
  friend class HivePartitionAndBucketKey;

public:

  HiveFileIterator() { reset(); }

  void reset() { p_ = b_ = f_ = 0; l_= 1;
            l1PartStats_ = NULL; l2BucketStats_ = NULL; l3FileStats_ = NULL; }

  const HHDFSListPartitionStats *getPartStats()       { return l1PartStats_; }
  const HHDFSBucketStats        *getBucketStats()   { return l2BucketStats_; }
  const HHDFSFileStats          *getFileStats()       { return l3FileStats_; }
 

private:

  // current file selected by the iterator
  const HHDFSListPartitionStats *l1PartStats_;
  const HHDFSBucketStats        *l2BucketStats_;
  const HHDFSFileStats          *l3FileStats_;

  // indexes of next file to be returned
  CollIndex p_; // index of list partition
  CollIndex b_; // index of bucket within partn
  CollIndex f_; // index of file within bucket
  CollIndex l_; // level at which to try to advance next
};


class HbaseSearchKey : public SearchKey
{

public:
   HbaseSearchKey(const ValueIdList & keyColumns,
            const ValueIdList & orderOfKeyValues,
            const ValueIdSet & externalInputs,
            const NABoolean forwardSearch,
            ValueIdSet & setOfPredicates,
            const ValueIdSet& nonKeyColumnSet,
            const IndexDesc *indexDesc,
            const IndexDesc *UDIndexDesc,
            const ValueIdSet & outputsByHbaseScan
                 );

   const ValueIdSet& getRequiredOutputColumns() { return requiredOutputColumns_; };

   const ValueIdSet& getNonKeyPreds() { return nonKeyPreds_; };

   // NABoolean searchOnKeyColumnsOnly();

   static void makeHBaseSearchKeys(
        const SearchKey * builtinSearchKey,
        const ValueIdList & keyColumns,
        const ValueIdList & orderOfKeyValues,
        const ValueIdSet & externalInputs,
        const NABoolean forwardSearch,
        ValueIdSet & setOfPredicates,
        const ValueIdSet& nonKeyColumnSet,
        const IndexDesc *indexDesc,
        const ValueIdSet & outputsByHbaseScan,
        LIST(HbaseSearchKey *) &producedKeys);

  // A method comparing two hbase search keys on the following aspects. 
  // 
  // 1. The length of the begin/end keys
  // 2. The equality of both begin and end key values
  // 3. The key type (unique or scan)
  // 4. # of covered leading keys
  // 
  // Please refer to HbaseAccess::processSQHbaseKeyPreds() for
  // elements of HbaseSearchKey used and compared in this method.
  //
  NABoolean isEqualTo(const HbaseSearchKey* other) const;

  NABoolean isFalsePred() {return isFalsePred_ ;}
  void setIsFalsePred(NABoolean val) {isFalsePred_ = val ;}

  // -----------------------------------------------------------------------

protected:

   // the set of non key columns detected during cstr();
   ValueIdSet nonKeyColumns_;

   // the set of key columns passed in cstr();
   ValueIdList keyColumns_;

   // the output columns from the hbase scan, required by all its direct or indirect
   // parents;
   ValueIdSet requiredOutputColumns_;

   ValueIdSet nonKeyPreds_;

   // is a constant folded pred that returns FALSE always. SK has 
   // min/max for begin/end reversed.
   NABoolean isFalsePred_;
};

#endif /* SEARCHKEY_H */





