#ifndef _DISJUNCTS_H
#define _DISJUNCTS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         disjuncts.h
 * Description:  Handling of local disjuncts and optimizer/generator
 *               mdam interface
 * Code location: mdam.C
 *               
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

#include "mdam.h"
#include "disjunct.h"
#include "NABasicObject.h"

// -----------------------------------------------------------------------
// Class Disjuncts
// This class encapsulates the handling of the disjunct
// array class. It always contains at least one element.
// -----------------------------------------------------------------------


typedef NABoolean (*funcPtrT)(ItemExpr*);

class Disjuncts : public NABasicObject // Abstract class
{
public:

  virtual CollIndex entries() const = 0;


  // INPUT: i the position of the disjunct (zeroth is firts)
  // OUTPUT: disjunct the i-th disjunct 
  // returns FALSE if no more disjuncts,
  virtual NABoolean get(Disjunct& disjunct, CollIndex i) const;

  virtual void print( FILE* ofd,
		    const char* indent = DEFAULT_INDENT,
		    const char* title = "disjuncts") const;

  void print() const;
  
  const ValueIdSet& getCommonPredicates() const
  { return commonPredicates_; }

  NABoolean containsOrPredsInRanges() const;
  NABoolean containsAndorOrPredsInRanges() const;

protected:
  // The intersection of all the disjuncts in the *local* disjunct array:
  virtual void computeCommonPredicates();
  // The empty disjunct array is a disjunct array with
  // one entry: the empty disjunct
  DisjunctArray * createEmptyDisjunctArray() const;

  NABoolean containsSomePredsInRanges(funcPtrT funcP) const;

private:
  // the intersection of all the predicates in the disjunct
  ValueIdSet commonPredicates_;

}; // class Disjuncts

class MaterialDisjuncts : public Disjuncts // "Normal" disjuncts
{
public:
  // create a disjunct array out of a selection predicate expression:
  MaterialDisjuncts(const ValueIdSet& selectionPredicates);

  // wrap a MaterialDisjuncts around a single disjunct 
  MaterialDisjuncts(Disjunct*);

  ~MaterialDisjuncts();

  virtual CollIndex entries() const
  { return disjunctArrayPtr_->entries(); }


  virtual NABoolean get(Disjunct& disjunct, CollIndex i) const;


private:
  void createDisjunctArray(const ValueIdSet& selectionPredicates);

  // An array of disjuncts created by calling createDisjunctArray(). 
  // The main functionality of that method is to call mdamTreeWalk() on
  // ItemExpr and convert the predicates into disjunctive normal form. 
  // Note that for a RANGE SPEC itemExpr, only predicates NOT in OR
  // forms can be processed. Also, the mdam tree walk has a 
  // threshold of 256 * 8 = 2000 disjuncts that are allowed in the array. 
  // Beyond that, a NULL is returned. 
  DisjunctArray *disjunctArrayPtr_;

}; // class MaterialDisjuncts



#endif
// eof 



