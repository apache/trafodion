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
#ifndef CASCADESBASIC_H
#define CASCADESBASIC_H
/* -*-C++-*-
******************************************************************************
*
* File:         CascadesBasic.h
* Description:  common definitions for the Cascades search engine
* Created:      5/4/94
* Language:     C++
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "Collections.h"

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------
class HashValue;
class ReferenceCounter;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class ValueId;
class ValueIdSet;
class CANodeIdSet;
class NAString;

// -----------------------------------------------------------------------
// max arity of an operator.
// There are two ways to add a new operator with a greater arity:
// a) increase this constant
// b) (applicable for item expressions only)
//    redefine the operator[] method for that new operator and allocate
//    space for the additional children in that particular class
// -----------------------------------------------------------------------
const Int32 MAX_REL_ARITY = 2; // join, union
const Int32 MAX_ITM_ARITY = 3; // like, between predicates have arity 3

// -----------------------------------------------------------------------
//   Group index
//   ===========
//   The number of a group serves as index into the array of groups in
//   CascadesMemo. Pointers to groups are not permitted, only these indices.
//   This level of indirection will permit merging groups as required.
// -----------------------------------------------------------------------

typedef CollIndex CascadesGroupId;
const   CollIndex INVALID_GROUP_ID = NULL_COLL_INDEX;

// -----------------------------------------------------------------------
// value of a hash function
// -----------------------------------------------------------------------

class HashValue : public NABasicObject
{
public:

  HashValue(ULng32 v = 0x0) { val_ = v; }

  inline NABoolean operator == (const HashValue &other)
  { return (val_ == other.val_); }

  // hash values are combined with other hash values or other values by
  // using the operator ^=
  inline HashValue & operator ^= (const HashValue & other)
  { val_ ^= other.val_; return *this; }
  inline HashValue & operator ^= (UInt32 other)
  { val_ ^= (ULng32) other; return *this; }
  inline HashValue & operator ^= (unsigned short other)
  { val_ ^= (ULng32) other; return *this; }
  inline HashValue & operator ^= (unsigned char other)
  { val_ ^= (ULng32) other; return *this; }
  inline HashValue & operator ^= (Int32 other)
  { val_ ^= (ULng32) other; return *this; }
  inline HashValue & operator ^= (short other)
  { val_ ^= (ULng32) other; return *this; }
  inline HashValue & operator ^= (char other)
  { val_ ^= (ULng32) other; return *this; }
  inline HashValue & operator ^= (void * other)
  { val_ ^= (ULng32)((Long) other); return *this; }
  HashValue & operator ^= (const NAString & other);
  HashValue & operator ^= (const ValueId & other);
  HashValue & operator ^= (const ValueIdSet & other);
  HashValue & operator ^= (const CANodeIdSet & other);

  inline ULng32 getValue() { return val_; }

private:

  ULng32 val_;
};

// -----------------------------------------------------------------------
//  Comparison results
//  ==================
//  Property vectors and costs permit comparisons
//  NOTE: easy to remember values are assigned for debugging only!!
// -----------------------------------------------------------------------

typedef enum COMPARE_RESULT
{
  UNDEFINED = -3, // for partial orderings
  INCOMPATIBLE = -2,
  LESS = -1,
  SAME = 0,
  MORE = 1

  // x1 -> compare (x2) == MORE  <==>  x1 > x2

} COMPARE_RESULT;

/*
inline char * compare_result_string (COMPARE_RESULT c)
{
    return (c == MORE ? "more" :
            c == LESS ? "less" :
            c == SAME ? "same" :
	    c == INCOMPATIBLE ? "incompatible" :
            "undefined");
} // compare_result_string
*/

inline COMPARE_RESULT reverse_compare_result (COMPARE_RESULT c)
{
    return (c == MORE ? LESS :
            c == LESS ? MORE :
            c); // SAME or UNDEFINED (don't use with INCOMPATIBLE)
} // reverse_compare_result

inline COMPARE_RESULT combine_compare_results (
        COMPARE_RESULT c1, COMPARE_RESULT c2)
{
  if (c1 == INCOMPATIBLE OR
      c2 == INCOMPATIBLE)       return INCOMPATIBLE;
  else if (c1 == UNDEFINED OR
	   c2 == UNDEFINED)     return UNDEFINED;
  else if (c1 == SAME)          return c2;
  else if (c2 == SAME)          return c1;
  else if (c1 == c2)            return c1;
  else                          return UNDEFINED;
} // combine_compare_results

// -----------------------------------------------------------------------
// Sort order: same, inverse, different
// -----------------------------------------------------------------------

typedef enum OrderComparison
{
  SAME_ORDER,
  INVERSE_ORDER,
  DIFFERENT_ORDER
} OrderComparison;

inline OrderComparison combineOrderComparisons(OrderComparison o1,
					       OrderComparison o2)
{
  if (o1 == o2)
    return o1;
  else
    return DIFFERENT_ORDER;
}

// -----------------------------------------------------------------------
//  Reference Count Object
//
//  The following class serves as a base class for objects for which
//  a reference count needs to be maintained.
//
//  Users of this class are responsible for invoking methods to
//  increment/decrement the reference count.
//  When the reference count is decremented to 0, the object is destroyed.
//
//  Custom new() and delete() methods can be used for faster storage
//  allocation and to gather statistics about memory usages.
//
// -----------------------------------------------------------------------

class ReferenceCounter : public NABasicObject
{
public:

  // ---------------------------------------------------------------------
  //  Constructor/Assignment/Destructor
  // ---------------------------------------------------------------------
  ReferenceCounter ();
  ReferenceCounter (const ReferenceCounter&);
  ReferenceCounter& operator= (const ReferenceCounter&);
  virtual ~ReferenceCounter ();

  // ---------------------------------------------------------------------
  //  Manipulation of reference counter
  // ---------------------------------------------------------------------
  void incrementReferenceCount(Int32 delta = 1);
  void decrementReferenceCount(Int32 delta = 1);
  Int32 getReferenceCount() const;

private:

  Int32 referenceCount_;

}; // ReferenceCounter

#endif /* CASCADESBASIC_H */
