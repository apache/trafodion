#ifndef _KEYCOLUMNS_H
#define _KEYCOLUMNS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         keycolumns.h
 * Description:  A KeyColumns object contain the key columns in
 *               a disjunct (along with their predicates)
 *
 * Code location: mdam.C
 * Created:      //96
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------



#include "ValueDesc.h"
#include "NABasicObject.h" 

// -----------------------------------------------------------------------
// Class KeyColumns
// A KeyColumns objects is esentially a list of
// <ValueId, ValueIdSet> pairs representing a column entry
// and its predicates. This semantic must be enforced by
// the caller.
// The main use, currently, for this class is to serve as
// a base class for class ColumnOrderList below.
// The only reason that I have left class KeyColumns in is that
// I see enough difference between them that class KeyColumns
// may prove useful in other places in ARK code. If enough time
// has passed and nobody is still using KeyColumns then both
// classes should be unified and class KeyColumns should be
// deleted (04/02/97).
// -----------------------------------------------------------------------

class KeyColumns : public NABasicObject {
public:
  // the keyColumns are needed to set up the list,

  // column entries will be added later:
  KeyColumns();

  // initialize column entries
  KeyColumns(const ValueIdList& orderList);

  ~KeyColumns();

  // ------Accessors

  NABoolean isEmpty() const
  { return keyColumnPtrCache_.isEmpty(); }
  const ValueIdSet& getPredicatesForColumn(const ValueId& column) const;

  const ValueIdSet& getColumnSet() const
  { return columnSet_; }

  // Get all predicates attached to all keycolumns to set
  // allPredicates (side-effects allPredicates):
  void getAllPredicates(ValueIdSet& allPredicates) const;

  

  virtual void print( FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
	     const char* title = "") const;

  virtual void display() const
  { print(); }

  // -------Mutators:
  // Remove columns and their predicates:
  virtual void clear();
  // remove predicates hanging from each column, but leave columns in:
  virtual void clearPredicates();
  // append a set of key predicates:
  void append(const ValueIdSet& andPredicateExpression);



  // insert a column into the cache:
  void insertColumn(const ValueId& column);
  // insert a predicate, let the class decide the appropiate(s)
  // column(s)
  void insertPredicate(const ValueId &predicate);
  // insert a predicate into a given column:
  void insertPredicateInColumn(const ValueId& column
                               ,const ValueId &predicate
                               );


  // private:  oss build fails when this is uncommented
  // This class holds a set of predicates that refer to a given
  // key column
  class KeyColumn : public NABasicObject {
  public:
    enum KeyColumnType {EMPTY, // initial state, no preds
			EQUAL, // A=20
			RANGE, // A>10, A<30
			INLIST,   // A IN (...)
			CONFLICT, // There is a mix. of preds (A>3, A>20)
			CONFLICT_EQUALS, // there is an = among the preds
			IS_NULL, // A IS NULL
			ASSIGN}; 

    KeyColumn(const ValueId& column)
	 :column_(column),
	  keyColumnType_(EMPTY)
    {}

    // ----------------------------------------------------------------
    // Accessors
    // ----------------------------------------------------------------

    const ValueIdSet& getPredicates() const
    {return predicatesForColumn_;}

    const ValueId& getColumnId() const
    { return column_;}

    // returns the type of this column (it may be EMPTY)
    KeyColumnType getType() const
    { return keyColumnType_;}

    // returns the type of the predicate
    KeyColumnType getType(const ValueId& predId) const;

    
    // -----------------------------------------------------------------------
    // mutators:
    // -----------------------------------------------------------------------

    void setType(KeyColumnType type) 
    { keyColumnType_ = type;}

    void insert(const ValueId &predicate);

    // remove all predicates and reset type to EMPTY:
    void clearPredicates();


    // Eliminate conflicting predicates 
    void resolveConflict();

    
    void print( FILE* ofd = stdout,
		const char* indent = DEFAULT_INDENT,
		const char* title = "") const;
 
    
  private:
    const ValueId& column_;
    ValueIdSet predicatesForColumn_;
    KeyColumnType keyColumnType_;
  };

protected:
  // utility for insertPredicateForColumn
  void insertPredicate(const ValueId &predicate,
                       const ValueId* columnPtr);

  const KeyColumn& getKeyColumn(const ValueId& column) const;	
  // the only reason that the following does not return a pointer
  // to constant data is because the templates cannot seem
  // to accept them (I will put these into an ARRAY later,
  // see validateOrder below...)
  KeyColumn* getKeyColumnPtr(const ValueId& column);


  LIST(KeyColumn *) keyColumnPtrCache_;
private:
  // this is handy to have to be able to get at the data in the
  // key columns object
  ValueIdSet columnSet_;

}; // class KeyColumns



// -----------------------------------------------------------------------
// A ColumnOrderList is used to store key predicates.
// Its semantic is such that all predicates in the list are ANDed
// together. Thus, it can be used to represent a disjunct.
// Also, and very important, the predicates are hashed by key
// column order (order zero is the first key column,
// order one, the second, and so on). 
// -----------------------------------------------------------------------



class ColumnOrderList : public KeyColumns
{
public:
  ColumnOrderList(const ValueIdList& columnList);
  // accessor:
  // A pointer to the Predicates in the column order:
  // (NULL if the order has no predicates)
  const ValueIdSet* operator[] (CollIndex order) const;

  // Return the KeyColumn for this order (a NULL column is
  // returned if the order has no predicates)
  const KeyColumn* getPredicateExpressionPtr(CollIndex order) const;

  
  // the number of column orders in the list (i.e. ABCAB returns 5)
  CollIndex entries() const
  { return orderKeyColumnPtrList_.entries(); }
  //the columnIdList on which the columnOrderList was created
  ValueIdList getColumnList() const
  { return columnList_;}  


  // the columnId associated with the key column at location 'order'.
  ValueId getKeyColumnId(CollIndex order) const;

  // -----------------------------------------------------------------------
  // Returns the single subset order (see header of method
  // implementation for definition and examples)
  // -----------------------------------------------------------------------
  NABoolean getSingleSubsetOrder(CollIndex& sso /* out */) const;
  // Fix for MDAM in an isolated way.
  NABoolean getSingleSubsetOrderForMdam(CollIndex& sso /* out */) const;

// Returns TRUE if at least one key column
  // in the list contains predicates:
  NABoolean containsPredicates() const;

  virtual void display() const
  { print(); }

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
	      const char* title = "") const;

  //------------------
  // Mutators
  //------------------
  // No predicates returned after (NOT including) this column
  // Predicates for columnOrder ARE returned
  // Since columnOrder starts from zero, at least the
  // first column must contain predicates (otherwise
  // wgy bother to pick MDAM?)
  void setStopColumn(CollIndex stopColumn);

  // if no columns are valid use:
  void invalidateAllColumns();

  // If possible, convert predicate expression in order to a compatible
  // expression with no conflict:
  void resolveConflict(CollIndex order);  
private:

  // Call this to signal that a given order contains
  // predicates (by default, no order contains predicates)
  // order <-> predicates is a one-to-many relationships
  // (i.e. two orders may point to the same predicate set,
  //  still one od the orders "may not" contain predicates.)
  // For instance, given that the orders are for index columns
  // A B C A B (secondary index on ABC, primary key on AB)
  // the mdam optimizer may have found that we need to collect
  // predicates for the intervals only up to order 2 (i.e. column
  // C on the list). Thus, the second A and B do not contain
  // predicates and (*this)[3] and (*this)[4) would return the
  // NULL pointer (validateOrder must be called for order 0, 1, 2)
  void validateOrder(CollIndex order);
  void invalidateOrder(CollIndex order);

  // for code generation
  LIST(KeyColumn *) orderKeyColumnPtrList_;
  const ValueIdList columnList_;

}; // class ColumnOrderList


class ColumnOrderListPtrArray : public ARRAY(ColumnOrderList *){
public:
  ColumnOrderListPtrArray();
  // copy ctor
  ColumnOrderListPtrArray (const ColumnOrderListPtrArray & orig) :
    ARRAY(ColumnOrderList *)(orig, HEAP)
    {}

  // These are needed by MdamKey::preCodeGen to compute the
  // executor predicates:
  void computeCommonKeyPredicates(ValueIdSet& commonPredicates) const;

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
	      const char* title = "ColumnOrderListPtrArray") const;



  // mutators:


private:

};

#endif
// eof 
