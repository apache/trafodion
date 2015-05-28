/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1996-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef _MDAMKEY_H
#define _MDAMKEY_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         mdamkey.h
 * Description:  Handling of disjuncts and costing for mdam
 * Code location: mdam.C
 *
 * Created:      //96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "mdam.h"
#include "disjuncts.h"
#include "keycolumns.h"
#include "NABasicObject.h"
#include "IndexDesc.h"

// -----------------------------------------------------------------------
// Class ScanKey
// A ScanKey is an abstract class that represents a set of
// of subsets to be read from an index file.
// A ScanKey is logically an array of "key disjuncts",
// each key disjunct containing a set of key predicates.
// These key predicates may be accessed through a data structure
// called OrderColumnList (see method getKeyPredicatesByColumn).
//
// The predicates of the key disjuncts, when applied to the index
// table may give raise to several subsets. It is the responsability
// of the derived class to enforce the semantics of the key disjunct
// generation (for example, the key disjunct generation for
// the SearchKey guarantees that the key disjunct represents a single
// subset)
//
// Classes SearchKey (a single subset) and MdamKey (multiple subsets)
// inherit from ScanKey.
// -----------------------------------------------------------------------

// class Key contains common procedures and declarations
// for MdamKey and SearchKey
class ScanKey : public NABasicObject
{
public:
  // -----------------------------------------------------------------------
  // the nonKeyColumnSet is used by the SearchKey when it is
  // computing the key for an index desc and it is trying to decide
  // whether to replicate the keys in the executor predicates.
  // It should be an empty set for the case when search key is used
  // to compute the partitioning key.
  // -----------------------------------------------------------------------
  
  ScanKey(
       const ValueIdList& keyColumnIdList
       ,const ValueIdSet& operatorInputs
       ,const Disjuncts& associatedDisjuncts
       ,const ValueIdSet& nonKeyColumnSet
       ,const IndexDesc * indexDesc
       ,const IndexDesc * UDindexDesc = NULL
       ) :
       keyColumnIdList_(keyColumnIdList)
    ,associatedDisjuncts_(associatedDisjuncts)
    ,operatorInputs_(operatorInputs)
    ,nonKeyColumnSet_(nonKeyColumnSet)
    ,iDesc_(indexDesc)
    ,udiDesc_(UDindexDesc)
  {}


  // -----------------------------------------------------------------------
  // Accessors:
  // -----------------------------------------------------------------------

  const IndexDesc * getIndexDesc() const
  { return iDesc_; }

  const IndexDesc * getUDIndexDesc() const
  { return udiDesc_; }

  const ValueIdSet& getExecutorPredicates() const
  { return executorPredicates_; }

  void setExecutorPredicates(const ValueIdSet& executorPredicates)
          { executorPredicates_ = executorPredicates; }                                                       

  // get the number of key disjuncts for this key (the key must
  // have at least one disjunct)
  virtual CollIndex getKeyDisjunctEntries() const = 0;

  // Obtain the key predicates (hashed by column order)
  // in the key disjunct:
  virtual void getKeyPredicatesByColumn(
       ColumnOrderList& keyPredsByCol, /*out*/
       CollIndex disjunctNumber = 0) const = 0;

  // Obtain the set of key predicates in the disjunct:
  virtual void getKeyPredicates(ValueIdSet &keyPredicates, /* out */
			NABoolean * allKeyPredicates = NULL,/*out*/
                        CollIndex disjunctNumber = 0 /*in*/) const;


  const ValueIdSet& getNonKeyColumnSet() const
  { return nonKeyColumnSet_; }

  const ValueIdSet getAllColumnsReferenced() const;

  // -----------------------------------------------------------------------
  // isAKeyPredicate()
  // 
  // A predicate is eligible to be used for searching an index when it
  // satisfies the following requirements:
  // 1) It should be a unary, binary, or ternary comparison predicate.
  // 2) One of the key columns must be a child of the root of the
  //    predicate tree.
  // 3) If the predicate performs binary or ternary comparison, the operand
  //    that is compared with the key column must be a value 
  //    belonging to the external inputs.
  // 4) If the predicate performs a ternary comparison, in addition to
  //    3), the extra inclusion indicator must also be an external input.
  //  
  // Parameters:
  //
  // const ValueId &  predId
  //    IN : A read-only reference to the ValueId of the predicate
  //         to be analyzed.
  //
  //
  // ValueId & referencedInput
  //    OUT: The ValueId of the expression that belongs to externalInputs
  //         and is compared with the keyColumn in the predicate.
  //         It may be set to the NULL_VALUE_ID for an IS NULL predicate.
  //
  // ValueId & intervalExclusionExpr
  //    OUT: The value id of a boolean expression that indicates that
  //         the decision whether the predicate is an exclusive or inclusive
  //         key predicate is made at run time. It is set to NULL_VALUE_ID
  //         if this decision can be made at compile time.
  //
  // Return value
  //    OUT: TRUE  If the given predicate can be used as a key on
  //               a key column
  //         FALSE Otherwise
  //
  // -----------------------------------------------------------------------

  NABoolean isAKeyPredicate(const ValueId & predId,
                            ValueId & referencedInput,
                            ValueId & intervalExclusionExpr) const;
  
  // same semantics as above, but the caller does not need the
  // referenced input nor the interval exclusion ptr
  NABoolean isAKeyPredicate(const ValueId& predId) const;

  // same semantics as above for detecting key predicates
  // but we need to make sure that the predicate is a key pred
  // for the column
  // for instance, for key columns A,B and the pred:
  // VegRef{A,2} = VegRef{C}
  // the pred is a key pred. However, it is not a key pred for A.
  NABoolean
  isAKeyPredicateForColumn(
       const ValueId & predId
       ,ValueId & referencedInput
       ,ValueId & intervalExclusionExpr
       ,const ValueId& keyColumn
       ) const;

  static NABoolean
  isAKeyPredicateForColumn(
       const ValueId & predId
       ,ValueId & referencedInput
       ,ValueId & intervalExclusionExpr
       ,const ValueId& keyColum
       ,const ValueIdSet& inputValues);

  NABoolean
  isAKeyPredicateForColumn(
       const ValueId& predId
       ,const ValueId& keyColumn) const;

  static void createComputedColumnPredicates(
       ValueIdSet &predicates,           /* in/out */
       const ValueIdSet &keyColumns,     /* in */
       const ValueIdSet &operatorInputs, /* in */
       ValueIdSet &generatedPredicates   /* out */);
  
  // -----------------------------------------------------------------------
  // Put those key predicates in keyPredicates that need to be
  // replicated in replicatedKeyPredicates:
  // -----------------------------------------------------------------------
  void
  replicateNonKeyVEGPredicates(
     const ValueIdSet& keyPredicates,
     ValueIdSet& replicatedKeyPredicates /* out */
     ) const;

  // -----------------------------------------------------------------------
  // Mutators:
  // -----------------------------------------------------------------------
  
  // Generate the data structures needed by the generator and
  // rewrite VEG preds and predicates:
  virtual void preCodeGen(ValueIdSet& executorPredicates,
			  const ValueIdSet& selectionPredicates,
			  const ValueIdSet & availableValues,
			  const ValueIdSet & inputValues,
			  VEGRewritePairs * vegPairsPtr,
			  NABoolean replicateExpression = FALSE,
                          NABoolean partKeyPredsAdded = FALSE) = 0;

  

  const ValueIdList& getKeyColumns() const
  { return keyColumnIdList_; }

  const Disjuncts& getDisjuncts() const
  { return associatedDisjuncts_ ; } 


  const ValueIdSet& getOperatorInputs() const
  { return operatorInputs_; }

protected:

  // -----------------------------------------------------------------------
  // Accessors:
  // -----------------------------------------------------------------------

  // get the selection predicates in MDNF for this key:

  // construct a key column order from the given set:
  void getKeyColumnOrderList(ColumnOrderList& keyPredsByCol, /* out*/
			     const ValueIdSet& predSet) const;


  static NABoolean expressionContainsColumn(
       const ItemExpr& iePtr
       ,const ValueId& keyColumn
       );

  // This is a quick fix:
  void splitRangeSpecRef(ColumnOrderList& keyPredsByCol,
			 const ValueId& predId,
			 const ValueIdList& columns,
			 NABoolean firstElemInRangeSpec,
			 const ValueId& predIdRange) const;    

  const ValueIdSet& getOperatorInputs() { return operatorInputs_;}


  // -----------------------------------------------------------------------
  // Mutators:
  // -----------------------------------------------------------------------

  
private:
  const IndexDesc  *iDesc_;
  const IndexDesc  *udiDesc_;
  const ValueIdList keyColumnIdList_;
  const ValueIdSet nonKeyColumnSet_;
  const ValueIdSet operatorInputs_;
  const Disjuncts& associatedDisjuncts_;
  ValueIdSet executorPredicates_;

}; // class ScanKey

//-------------------------------------------------------------
// An MdamKey contains different states at different
// times in the optimizer.
//
// Before optimization it contains data to generate
// state for a mdam key with all columns sparse and
// all stop columns set to the maximum order.
//
// After optimization, if the MdamKey survives (i.e.
// if a SingleSubset key was not chosen) the
// MdamKey may contain a mix. of sparse/dense
// columns and not all of the stop columns will be
// set to the max (this is chosen by costing)
//
// After pre-code generation, a data structure
// called OrderListPtrArray is created. It contains
// An array of key predicates ordered by key column.
// There is one entry of this array for each disjunct
// present if the MdamKey was generated from the associated
// disjuncts. There will be a single array entry, corresponding
// to the common predicates of the associated disjuncts, if the
// MdamKey was generated from the common predicates.
// Also in this phase VEG predicates and references are
// rewritten and executor predicates are generated.
//
// After code generation an Mdam Network is generated from
// the OrderListPtrArray.
//
//--------------------------------------------------------------


class MdamKey : public ScanKey
{
public:
  MdamKey(
       const ValueIdList& keyColumnIdList
       ,const ValueIdSet& operatorInputs
       ,const Disjuncts& associatedDisjuncts
       ,const ValueIdSet& nonKeyColumnSet
       ,const IndexDesc * indexDesc
       );
  ~MdamKey();



  // Return the number of disjuncts that were created
  // out of the selection predicates:
  virtual CollIndex getKeyDisjunctEntries() const;

  // returns the key predicates in those disjuncts
  virtual void getKeyPredicatesByColumn(
       ColumnOrderList& keyPredsByCol, /*out*/
       CollIndex disjunctNumber = 0) const;

  
  void setNoExePred(NABoolean v = TRUE){ noExePred_= v; }

  NABoolean getNoExePred(){ return noExePred_;}
    
  
  CollIndex getStopColumn(CollIndex disjunctNumber) const;

  // TRUE means that column is sparse, else it is dense
  NABoolean getSparseFlag(CollIndex columnOrder) const;

  NABoolean isColumnSparse(CollIndex columnOrder) const
  { return getSparseFlag(columnOrder); }
  NABoolean isColumnDense(CollIndex columnOrder) const
  { return NOT isColumnSparse(columnOrder); }
  

  void setAllStopColumnsToMax(); // all stop columns are the highest
  void setAllColumnsToSparse(); // all columns are sparse
  
  void setStopColumn(CollIndex disjunctNumber,
		     CollIndex columnOrder);
  void setSparseFlag(CollIndex columnOrder, NABoolean isSparse);

  void setColumnToDense(CollIndex columnOrder)
  { setSparseFlag(columnOrder,FALSE); }

  void setColumnToSparse(CollIndex columnOrder)
  { setSparseFlag(columnOrder,TRUE); }
  
  // This function copies information from 'other' MDAM key
  // to the current MDAM key: stop columns, starse keys and
  // noExePred. It is used to share this information when
  // saving and reusing BasicCost for MDAM key.
  void reuseMdamKeyInfo(MdamKey * other);

  // ---------------------------------
  // ---- Pre-code generation interface:
  // ---------------------------------
  // Pre-code gen materializes the columnorderlist
  // and rewrites the predicates.
  // It also computes the executor predicates:
  void preCodeGen(ValueIdSet& executorPredicates,
		  const ValueIdSet& selectionPredicates,
		  const ValueIdSet & availableValues,
		  const ValueIdSet & inputValues,
		  VEGRewritePairs * vegPairsPtr,
		  NABoolean replicateExpression = FALSE,
                  NABoolean partKeyPredsAdded = FALSE);
  

  // ---------------------------------
  // ---- Generator interface:
  // ---------------------------------
  const ColumnOrderListPtrArray& getColumnOrderListPtrArray() const; 

  
  // -----------------------------------------------------------------------
  // Utility functions:
  // -----------------------------------------------------------------------
  void display() { print(); }
  void print( FILE* ofd = stdout,
		    const char* indent = DEFAULT_INDENT,
		    const char* title = "") const;


private:

  // extract key predicates from predicates and append them to
  // keyPredicates (side-effects keyPredicates):
  void appendKeyPredicates(ValueIdSet& keyPredicates,
			   const ValueIdSet& predicates,
                           const ValueIdSet& inputValues) const;

  NABoolean isAPartKeyPredicateForMdam(const ValueId& predId,
                                       const ValueIdSet& inputValues) const;



  // -----------------------------------------------------------------------
  // This members are created by the costing. They contain the data
  // needed to generate the ColumnOrderListArray in preCodeGen.
  // Also, ScanKey::associatedDisjuncts is used in this generation.
  // -----------------------------------------------------------------------
  NABoolean *sparseFlagArray_;
  CollIndex *stopColumnArray_;
  //Boolean that tells us if executor predicates can be empty or not.
  //It can be empty if all predicates are applied at the dp2.
  NABoolean noExePred_;

  // -----------------------------------------------------------------------
  //  The ColumnOrderListArray contains the data from which the generator
  // builds the mdam network.
  // This array is NULL in the optimizer and it is generated by preCodeGen
  // -----------------------------------------------------------------------
  ColumnOrderListPtrArray *columnOrderListPtrArrayPtr_;
}; // class MdamKey
       




#endif
// eof 
