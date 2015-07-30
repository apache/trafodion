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

#ifndef MDAM_H
#define MDAM_H
/* -*-C++-*-
******************************************************************************
*
* File:         MDAM.h
* Description:  MDAM Class -- DisjunctArray
* Created:      6/30/96
* Modified:
* Language:     C++
* Status:
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------
#include "Collections.h"
#include "ValueDesc.h"

#define  MDAM_MAX_NUM_DISJUNCTS 256

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// class ItemExpr;

// ***********************************************************************
// contents of this file
// ***********************************************************************
class DisjunctArray;



// ***********************************************************************
// DisjunctArray : An ordered collection of ValueIdSets
//
// A DisjunctArray is an NAArray, or an array.  It represents the
// predicates in disjunctive normal form.  Each entry of the array
// is in essence a disjunct made up of predicates ANDed together. Each
// entry has a pointer to a ValueIdSet. This ValueIdSet is the set of
// predicates that make up the disjunct.
//
// Since disjuncts are ORed together to represent the query in disjunctive
// normal form, each entry in this array can be considered as being ORed
// to the rest of the entries in the array.
//
// This representation is not truly in disjunctive normal form since a
// predicate can be an IN list -- equality predicates on the same column
// ORed together.
// ***********************************************************************

class DisjunctArray : public ARRAY(ValueIdSet * )
{
public:

  // ---------------------------------------------------------------------
  // Constructor
  // ---------------------------------------------------------------------

  DisjunctArray() :
    ARRAY(ValueIdSet *)(CmpCommon::statementHeap())
    { }

  DisjunctArray(ValueIdSet * const disjunct ) :
    ARRAY(ValueIdSet *)(CmpCommon::statementHeap())
    { insertAt(0, disjunct); }

private:
  DisjunctArray (const DisjunctArray &) ; //memleak fix
public:

  // ---------------------------------------------------------------------
  // Create a new disjunct entry by inserting a pointer to the ValueIdSet
  // to the end of the array. Finds the number of entries in the array and
  // inserts the pointer as a new entry at the end of the array.
  // ---------------------------------------------------------------------

  void insert(ValueIdSet * setOfPredicates )
  { insertAt(entries(), setOfPredicates); }

  // ---------------------------------------------------------------------
  // Destructor
  // Gets rid of all the ValueIdSets that each of the entries points to
  // ---------------------------------------------------------------------
  ~DisjunctArray();



  // ---------------------------------------------------------------------
  // This method is invoked only when there is a single entry in one of
  // the disjunct arrays being ANDed.  In this case all the predicates
  // in this single disjunct (ValueIdSet) are added to the disjuncts of
  // the disjunct array being processed.  This method loops through the
  // entries of the disjunct array being processed.  For each one it gets
  // the ValueIdSet that the entry points to.  It adds to this the
  // predicate value ids from the single disjunct (ValueIdSet) in the
  // disjunct array passed in as a parameter (otherDisjunctArray).
  // ---------------------------------------------------------------------
  void andDisjunct
    ( DisjunctArray * otherDisjunctArray );

  // ---------------------------------------------------------------------
  // Here a new disjunctArray is created as a result of the ANDing of the
  // two disjunctArrays.  The method loops through the left disjunct array
  // and for each of its entries it loops though the right disjunct array.
  // For a combination of each of the entries in the two disjunctArrays it
  // inserts an entry into the new disjunctArray.  It essentially performs
  // a cross-product of the two arrays.  So the number of entries in the
  // new array should be the number of entries in the left disjunct array
  // times the number in the right disjunct array.
  //
  // For each combination of the entries it first copies the left disjunct
  // entry creating a new ValueIdSet.  To this newly created ValueIdSet it
  // adds the predicate value ids in the right array ValueIdSet.  It uses
  // the += operator which does a logical OR of the predicates in both
  // the left and the right ValueIdSet entries.  After creating the new
  // disjunctArray, it deletes the left and right disjunct arrays, also
  // deleting all the ValueIdSets that they pointed to.
  // ---------------------------------------------------------------------
  void andDisjunctArrays
    ( DisjunctArray *  leftDisjunctArray,
      DisjunctArray * rightDisjunctArray );

  // ---------------------------------------------------------------------
  // This method ORs the disjunctArray passed in as a parameter by copying
  // all the entries from this disjunctArray and adding them to the
  // disjunctArray being processed.  So it loops through the disjunct
  // array passed and inserts each entry into the disjunct array being
  // processed.  When it is done, since the ValueIdSets that the array
  // pointed to still need to be preserved, it clears the disjunct array
  // that is not needed anymore (has been merged) and then deletes it.
  // Simply deleting it would also delete the ValueIdSets that it points
  // to.
  // ---------------------------------------------------------------------
  void orDisjunctArray
    ( DisjunctArray * otherDisjunctArray );

  // ---------------------------------------------------------------------
  // Utility functions
  // ---------------------------------------------------------------------
  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "DisjunctArray") const;

  void display() const;

private:

  NABoolean hasContradictoryPredicates_;

}; // class DisjunctArray




// ***********************************************************************
// static functions related to DisjunctArray
// ***********************************************************************

// -----------------------------------------------------------------------
// mdamANDDisjunctArrays ANDs two DisjunctArrays
// -----------------------------------------------------------------------
DisjunctArray * mdamANDDisjunctArrays(
     DisjunctArray *  leftDisjunctArray,
     DisjunctArray * rightDisjunctArray );

// -----------------------------------------------------------------------
// mdamORDisjunctArrays ORs two DisjunctArrays
// -----------------------------------------------------------------------
DisjunctArray * mdamORDisjunctArrays(
     DisjunctArray *  leftDisjunctArray,
     DisjunctArray * rightDisjunctArray );



#endif /* MDAM_H */
