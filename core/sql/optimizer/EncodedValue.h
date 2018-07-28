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
#ifndef ENCODEDVALUE_H
#define ENCODEDVALUE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         EncodedValue.h
 * Description:  Encoded Value class
 *
 *
 * Created:      5/31/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include <float.h>
#include "ItemExpr.h"
#include "NAColumn.h"

// -----------------------------------------------------------------------
// For now, we allocate histograms on the statement heap; eventually, we
// plan to put some of them (the read-only copies that are read in from
// the system catalogs, before any synthesis occurs) in the context heap
// -----------------------------------------------------------------------
#ifndef HISTHEAP
#define HISTHEAP (CmpCommon::statementHeap())
#endif

const double _ENCODEDVALUE_NULL_VALUE_    =  DBL_MAX ;
const double _ENCODEDVALUE_UNINIT_VALUE_  =  -7.7777777777777e-67 ; /* "arbitrarily weird" */
const double _ENCODEDVALUE_CLOSE_TO_NULL_ = (DBL_MAX*0.9999999999) ;

// -----------------------------------------------------------------------
//  Forward Declarations
// -----------------------------------------------------------------------
class ConstValue;
class SQLNumeric;

// -----------------------------------------------------------------------
//  Normalized Value
// -----------------------------------------------------------------------
class NormValue // NOT a NABasicObject (memreduction)
{
private:
  inline void copy (const NormValue& other)
  { theValue_ = other.theValue_; isNullFlag_ = other.isNullFlag_; }

public:
  // constructors
  NormValue() { theValue_ = 0; isNullFlag_ = FALSE; }

  NormValue(double value) { theValue_ = value; isNullFlag_ = FALSE; }

  // construct a NormValue from a constant
  NormValue(const ConstValue *constant, NABoolean negate = FALSE);

  // copy constructor
  NormValue (const NormValue& other)
  { copy(other) ; }

  // assignment operator
  inline NormValue & operator= (const NormValue& other)
  { copy(other) ; return *this; }

  // comparison operators
  COMPARE_RESULT compare (const NormValue &other) const;
  inline NABoolean operator>  (const NormValue &other) const
    {return (theValue_ > other.theValue_); }
  inline NABoolean operator>= (const NormValue &other) const
    {return (theValue_ >= other.theValue_); }
  inline NABoolean operator<  (const NormValue &other) const
    {return (theValue_ < other.theValue_); }
  inline NABoolean operator<= (const NormValue &other) const
    {return (theValue_ <= other.theValue_); }
  inline NABoolean operator== (const NormValue &other) const
    {return (theValue_ == other.theValue_); }
  inline NABoolean operator!= (const NormValue &other) const
    {return (theValue_ != other.theValue_); }

  // accessor function
  inline const double & getValue() const { return theValue_; }
  inline double getValue() { return theValue_; }
  inline NABoolean isNull() const { return isNullFlag_; }

  UInt32 computeHashForNumeric(SQLNumeric* typ);

  // manipulation methods
  inline void setValue(double value){ theValue_ = value; isNullFlag_ = FALSE; }
  inline void setNull() { theValue_ = _ENCODEDVALUE_NULL_VALUE_; isNullFlag_ = TRUE; }

  // utility methods
  void display (FILE *f = stdout, const char * prefix = DEFAULT_INDENT,
		const char * suffix = "",
                CollHeap *c=NULL, char *buf=NULL) const;

private:
  double theValue_;
  NABoolean isNullFlag_;
};

class NormValueList : public NAArray<NormValue>
{
public:
  // constructor 
  NormValueList(Lng32 numberOfElements = 0, NAMemory *h = 0) :
      NAArray<NormValue>(h ? h : CmpCommon::statementHeap(),numberOfElements), heap_(h) { };

  NormValueList(const NormValueList & mcsvl, NAMemory *h=0);
  NormValueList & operator=(const NormValueList& other);
  NormValueList & operator+(const NormValueList& other);
  NormValueList & operator-(const NormValueList& other);
  NormValueList & operator*(const double factor);
  void round ();

  ~NormValueList() {};

  COMPARE_RESULT compare (const NormValueList * other) const;

private:  
  NAMemory * heap_;
};

// -----------------------------------------------------------------------
//  Encoded Attribute Value
//
//  The following class stores the encoded representation for a
//  possibly multi-attribute value.
// -----------------------------------------------------------------------
class EncodedValue : public NABasicObject
{

public:
  // ---------------------------------------------------------------------
  //  Constructor
  // ---------------------------------------------------------------------
  EncodedValue (double val = 0.0);
  EncodedValue (const NormValueList& nvl, NAMemory * h = 0);

  // construct a multi-attribute value given a string representation
  // of the multi-attribute value, and a description of the columns

  // Note: This constructor is used to construct global objects, so
  // it cannot report errors.
  EncodedValue (const NAWchar *theValue)
   : valueList_(NULL), heap_(HISTHEAP)
  {
    const NAColumnArray empty ((CollHeap*) 0 /* NULL CollHeap* */) ;
    constructorFunction (theValue, empty, FALSE) ;
  }

  EncodedValue (const wchar_t *theValue, const NAColumnArray &columns, 
                ConstValue* cvPtrs[] = NULL)
    : valueList_(NULL), heap_(HISTHEAP)
    { constructorFunction (theValue, columns, TRUE, cvPtrs) ; }

  EncodedValue (const NAWchar *theValue, const NAColumn * column)
    : valueList_(NULL), heap_(HISTHEAP)
    {
      NAColumnArray columns ;
      columns.insertAt (0,(NAColumn*)(column)) ;
      constructorFunction (theValue, columns, TRUE) ;
    }

  EncodedValue (const EncodedValue & other, NAMemory * h = 0);

private:
  // In order to have both of the above constructors do the same
  // thing, I've created a non-ctor function that both of them
  // call.  Sooner or later we can stop using the ctor that takes
  // a NAColumnArray as its parameter ...
  void constructorFunction (const wchar_t * theValue, const NAColumnArray & columns, NABoolean okToReportErrors, ConstValue** cvPtr = NULL) ;
public:

  // construct a single-attribute value from a constant or list of constants
  EncodedValue(ItemExpr * expr, NABoolean negate);

  //  Given an upper and lower bound, return a ratio stating where the
  //  value lies within the range.  If the value lies:
  //    1) closer to lowBound, then return a ratio between [0,0.5)
  //    2) at the midpoint, then return 0.5
  //    3) closer to upperBound, then return a ratio between (0.5, 1.0]
  // -----------------------------------------------------------------------
  double ratio (const EncodedValue &lowBound, const EncodedValue &upperBound) const;

  //helper method to get Hash value
  UInt32 computeRunTimeHashValue(const NAColumnArray &columns, const NAWchar * boundary, ConstValue* csPtrs[]);

  // ---------------------------------------------------------------------
  //  Comparison operators
  // ---------------------------------------------------------------------
  COMPARE_RESULT compare (const EncodedValue &other) const;
  inline NABoolean operator>  (const EncodedValue &other) const
    {return (compare(other) == MORE);}
  inline NABoolean operator>= (const EncodedValue &other) const
    {return ((compare(other) == MORE) || (compare(other) == SAME));}
  inline NABoolean operator<  (const EncodedValue &other) const
    {return (compare(other) == LESS);}
  inline NABoolean operator<= (const EncodedValue &other) const
    {return ((compare(other) == LESS) || (compare(other) == SAME));}
  inline NABoolean operator== (const EncodedValue &other) const
    {return (compare(other) == SAME);}
  inline NABoolean operator!= (const EncodedValue &other) const
    {return (compare(other) != SAME);}

  // ---------------------------------------------------------------------
  //  Utility methods
  // ---------------------------------------------------------------------
  void display (FILE *f = stdout, const char * prefix = DEFAULT_INDENT,
		const char * suffix = "",
                CollHeap *c=NULL, char *buf=NULL) const;

  // get a printable string identifying the encoded value
  const NAString getText(NABoolean parenthesized = TRUE, NABoolean showFractionalPart = TRUE) const;

  inline const NormValueList * getValueList () const { return valueList_ ; }

  inline NormValue getValue () const { return value_ ; }
  inline double getDblValue () const { return value_.getValue() ; }

  // ---------------------------------------------------------------------
  // dealing with NULL values
  // ---------------------------------------------------------------------

  // am I null?
  inline NABoolean isNullValue() const { return ( value_.isNull() ) ; }

  // sets the current value to be NULL
  inline void setValueToNull() { value_.setNull() ; }

  // if it's not a *REAL* NULL value (the null flag isn't set), but the
  // stored double value is equal to our encoded NULL value, then
  // decrease the stored double value by a smidgen
  // (i.e., $$$ KLUDGE)
  void enforceNullMechanism() ;

  // NB: we enforce the NULL mechanism every time we create an EncodedValue object
  inline void setValue (const NormValue & val)
    {
      value_ = val ;
      enforceNullMechanism() ;
    }

  double minMaxValue(const NAType *pType, const NABoolean wantMin);

  UInt32 computeHashForNumeric(SQLNumeric* typ);

  // dump the content to a data buffer for computing run-time hash 
  // used by skew buster
  void outputToBufferToComputeRTHash(const NAType* naType, 
                                     char* data,  Int32& len, UInt32& flags)
                                     const;

private:
  // ---------------------------------------------------------------------
  //  Helper function for the ctor
  // ---------------------------------------------------------------------
  void addANormValue(EncodedValue * thisPtr, ItemExpr * itemPtr,
		     NABoolean negate);

  NormValue value_ ;
  NormValueList * valueList_;
  NAMemory * heap_;
};


// Two EncodedValue's that we use fairly regularly
// (initialized in EncodedValue.cpp)


extern const EncodedValue NULL_ENCODEDVALUE ;
extern const EncodedValue UNINIT_ENCODEDVALUE ;


#endif /* ENCODEDVALUE_H */


