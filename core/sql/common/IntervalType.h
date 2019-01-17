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
#ifndef INTERVALTYPE_H
#define INTERVALTYPE_H
/* -*-C++-*-
**************************************************************************
*
* File:         IntervalType.h
* Description:  Interval Type
* Created:      2/15/96
* Language:     C++
*
*
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------

#include "Platform.h"
#include "ComASSERT.h"
#include "ComDiags.h"
#include "NAType.h"
#include "DTICommonType.h"
#include "Int64.h"
#ifdef _DEBUG
#include <iostream>
#endif

static NAString LiteralInterval("INTERVAL");

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class IntervalType;
class SQLInterval;
class IntervalQualifier;
class IntervalValue;

short getIntervalFields(Lng32 fsDatatype,
                        rec_datetime_field &startField,
                        rec_datetime_field &endField);

// ***********************************************************************
//
//  IntervalType : The interval data type
//
// ***********************************************************************
class IntervalType : public DatetimeIntervalCommonType
{
public:

  static NABoolean validate(rec_datetime_field startField,
			    UInt32 leadingPrecision,
			    rec_datetime_field endField,
			    UInt32 fractionPrecision);

  static UInt32 getPrecision(rec_datetime_field startField,
			       UInt32 leadingPrecision,
			       rec_datetime_field endField,
			       UInt32 fractionPrecision = 0);

  static UInt32 computeLeadingPrecision(rec_datetime_field startField,
					  UInt32 precision,
					  rec_datetime_field endField,
					  UInt32 fractionPrecision = 0);

  static Lng32 getStorageSize(rec_datetime_field startField,
                             UInt32 leadingPrecision,
                             rec_datetime_field endField,
                             UInt32 fractionPrecision = 0);

  static size_t getStringSize(rec_datetime_field startField,
                              UInt32 leadingPrecision,
                              rec_datetime_field endField,
                              UInt32 fractionPrecision = 0);

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  IntervalType
  ( NAMemory *heap, NABoolean allowSQLnull
  , rec_datetime_field startField
  , UInt32 leadingPrec
  , rec_datetime_field endField
  , UInt32 fractionPrec = 0
  )
  : DatetimeIntervalCommonType
  ( heap, LiteralInterval //"INTERVAL"
  , NA_INTERVAL_TYPE
  , getStorageSize(startField, leadingPrec, endField, fractionPrec)
  , allowSQLnull
  , startField
  , endField
  , fractionPrec
  , getStorageSize(startField, leadingPrec, endField, fractionPrec)
  )
  , leadingPrecision_(leadingPrec)
  {                                         // this could be a valid interval if we change endField to SECOND
    if (endField == REC_DATE_FRACTION_MP && startField != REC_DATE_FRACTION_MP)
       endField = REC_DATE_SECOND;

    if (!validate(startField, leadingPrec, endField, fractionPrec) ||
        !isValid())
      {
	makeInvalid();

#ifdef _DEBUG
	// All callers *should be* immediately calling the checkValid() method
	// and so this debugging info *should be* unnecessary.  Delete it!
	cerr << "Invalid interval specification " <<
		getTypeSQLname(TRUE /*terse*/) << endl;
#endif // _DEBUG
      }
  }


//copy ctor

  IntervalType(const IntervalType &interval, NAMemory * heap)
		:DatetimeIntervalCommonType(interval,heap),
  leadingPrecision_(interval.leadingPrecision_)
  {
	if (endField_ == REC_DATE_FRACTION_MP && startField_ != REC_DATE_FRACTION_MP)
       endField_ = REC_DATE_SECOND;

    if (!validate(startField_, leadingPrecision_, endField_, fractionPrecision_) ||
        !isValid())
      {
	makeInvalid();

#ifdef _DEBUG
	// All callers *should be* immediately calling the checkValid() method
	// and so this debugging info *should be* unnecessary.  Delete it!
	cerr << "Invalid interval specification " <<
		getTypeSQLname(TRUE /*terse*/) << endl;
#endif // _DEBUG
      }
  }

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------

  // ## This method should be a virtual method on NAType,
  // ## and then of course it could not be inline
  // ## (implemented inline for now just for expediency of checking in).
  NABoolean checkValid(ComDiagsArea *diags)
  {
    if (!isValid())
      {
	// ## Possible future work:
	// We COULD add an optional Diags parameter onto our validate() method
	// and call it now; and it would emit more precise diagnostics
	// for why the type is invalid.  It is a complicated type after all!
	// The more precise diags could explain valid ranges of values, etc.
	//
	// Here we just emit "3044 Invalid interval $string0."
	*diags << DgSqlCode(-3044) << DgString0(getIntervalQualifierAsString());
	return FALSE;
      }
    return TRUE;
  }

  size_t getStringSize() const
  {
    return getStringSize(getStartField(), getLeadingPrecision(),
			 getEndField(),   getFractionPrecision());
  }
  
  virtual Lng32  getDisplayLength() const;

  UInt32 getTotalPrecision() const
  {
    return getPrecision(getStartField(),
                        getLeadingPrecision(),
                        getEndField(),
                        getFractionPrecision());
  }

  virtual NABoolean isEncodingNeeded() const
  {
    return TRUE;
  }

  // ---------------------------------------------------------------------
  // Virtual functions that are given an implementation for this class.
  // ---------------------------------------------------------------------
  virtual UInt32 getLeadingPrecision() const	{ return leadingPrecision_; }
 
  virtual Lng32 getPrecision() const { return getLeadingPrecision(); } 

  virtual void print(FILE* ofd = stdout, const char* indent = DEFAULT_INDENT) /*const*/;

  virtual short getFSDatatype() const;

  virtual NAString getSimpleTypeName() const 	{ return "INTERVAL"; }
  virtual NAString getTypeSQLname(NABoolean terse = FALSE) const;
  NAString getIntervalQualifierAsString() const;

  virtual NABoolean isSupportedType() const
  {
   if ((getStartField() == REC_DATE_FRACTION_MP)||
       (getStartField()== REC_DATE_SECOND &&  getLeadingPrecision() == 0))
     return FALSE;
   else
     return TRUE;
  }
  // Both types must be year-month intervals or both must be day-time intervals
  // (y-m interval can be just y, just m, or both y-m; see ANSI 4.5.2).
  virtual NABoolean isCompatible(const NAType& other, UInt32 * flags = NULL) const
  {
    const IntervalType& o = (IntervalType&)other;
    if (!isSupportedType() || !other.isSupportedType())
     return FALSE;
    else
    {
      return NAType::isCompatible(other, flags) &&
       (getEndField() <= REC_DATE_MONTH) == (o.getEndField() <= REC_DATE_MONTH);
    }
  }

  virtual NABoolean operator==(const NAType& other) const
  {
    return NAType::operator==(other) &&
           getStartField() 	  == ((IntervalType&) other).getStartField() &&
           getEndField()   	  == ((IntervalType&) other).getEndField() &&
           getLeadingPrecision()  == ((IntervalType&) other).getLeadingPrecision() &&
           getFractionPrecision() == ((IntervalType&) other).getFractionPrecision();
  }

  // ---------------------------------------------------------------------
  // A virtual function for synthesizing the type of a binary operator.
  // ---------------------------------------------------------------------
  virtual const NAType* synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                       const NAType& operand1,
                                       const NAType& operand2,
				       CollHeap* h,
				       UInt32 *flags = NULL) const;

  virtual NAType::SynthesisPrecedence getSynthesisPrecedence() const
  {
    return SYNTH_PREC_INTERVAL;
  }

  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // And the zero value, properly formatted in string form, for column defaults.
  // ---------------------------------------------------------------------
  virtual void minRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString** stringLiteral = NULL,
			     CollHeap* h=0) const;
  virtual void maxRepresentableValue(void*, Lng32*,
			     NAString** stringLiteral = NULL,
			     CollHeap* h=0) const;
  virtual void getZeroValue(void*, Lng32*,
			    NAString** stringLiteral = NULL,
			    CollHeap* h=0) const;

  virtual NABoolean createSQLLiteral(const char * buf,       // in
                                     NAString *&sqlLiteral,  // out
                                     NABoolean &isNull,      // out
                                     CollHeap *h) const;     // in/out

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void* bufPtr) const;

  // ---------------------------------------------------------------------
  // Leave this, inherited from NAType as a pure virtual function,
  // *unimplemented*, so that IntervalType, just like NAType,
  // remains an abstract class:  only SQLInterval can be instantiated.
  //
  //	virtual NAType *newCopy() const { return new IntervalType(*this); }
  // ---------------------------------------------------------------------

private:

  // Auxiliary methods
  void getRepresentableValue(char sign,
			     void* bufPtr, Lng32* bufLen,
			     NAString** stringLiteral,
			     CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // Leading precision (1..MAX_LEADING_PRECISION).
  // ---------------------------------------------------------------------
  unsigned short leadingPrecision_;

}; // class IntervalType

// ***********************************************************************
//
//  SQLInterval : SQL INTERVAL
//
// ***********************************************************************
class SQLInterval : public IntervalType
{
public:

  enum { DEFAULT_LEADING_PRECISION  =  2,	// ANSI 10.1 SR 5: two
	 MAX_LEADING_PRECISION	    = MAX_NUMERIC_PRECISION, // 10.1 SR 3: >=2
	 DEFAULT_FRACTION_PRECISION =  6,	// ANSI 10.1 SR 6: six
	 MAX_FRACTION_PRECISION_USEC=  6,	// ANSI 10.1 SR 4: >=6
	 MAX_FRACTION_PRECISION	    =  9	// ANSI 10.1 SR 4: >=6
       };

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLInterval
  ( NAMemory *h, NABoolean allowSQLnull
  , rec_datetime_field startField
  , UInt32 leadingPrec
  , rec_datetime_field endField
  , UInt32 fractionPrec = DEFAULT_FRACTION_PRECISION
  )
  : IntervalType(h, allowSQLnull, startField, leadingPrec, endField,
                 endField >= REC_DATE_SECOND ? fractionPrec : 0)
  {}

// copy ctor
  SQLInterval(const SQLInterval & interval,NAMemory * heap=0):
  IntervalType(interval,heap)
  {}

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
    { return new(h) SQLInterval(*this,h); }

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode(void *bufPtr) const;

  // 10-031022-0617 -begin
  //----------------------------------------------------------------------
  // Special implementation of encode, which depending upon the type
  // of the interval converts the value into seconds. This should
  // be merged with the encode method.
  //----------------------------------------------------------------------

  double getValueInSeconds(double valInSecs);
  
  // 10-031022-0617 -end

private:

}; // class SQLInterval

// ***********************************************************************
//
//  IntervalQualifier : A syntactic-sugar class used by the Parser only
//
// ***********************************************************************
class IntervalQualifier : public SQLInterval
{
public:

  // Constructors
  IntervalQualifier
  ( NAMemory *h, rec_datetime_field startField
  , UInt32 leadingPrec = DEFAULT_LEADING_PRECISION
  )
  : SQLInterval(h, FALSE, startField, leadingPrec, startField, DEFAULT_FRACTION_PRECISION)
  {}

  IntervalQualifier
  ( NAMemory *h, rec_datetime_field startField
  , UInt32 leadingPrec
  , rec_datetime_field endField
  , UInt32 fractionPrec
  )
  : SQLInterval(h, FALSE, startField, leadingPrec, endField, fractionPrec)
  {}

private:

}; // class IntervalQualifier

// ***********************************************************************
//
//  IntervalValue : An interval value
//
// ***********************************************************************
class IntervalValue : public NABasicObject
{
public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  IntervalValue
  ( const char* strValue
  , rec_datetime_field startField
  , UInt32 leadingPrecision
  , rec_datetime_field endField
  , UInt32 fractionPrecision
  , char sign = '+'
  );

  IntervalValue
  ( const char* value
  , Lng32 storageSize
  );

  // ---------------------------------------------------------------------
  // Destructor functions
  // ---------------------------------------------------------------------
  ~IntervalValue()
  {
    delete [] value_;
  }

  // ---------------------------------------------------------------------
  // Set the value.
  // ---------------------------------------------------------------------
  void setValue(Int64 value, Lng32 valueLen);

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  const unsigned char* getValue() const	{ return value_; }

  unsigned short getValueLen() const 	{ return valueLen_; }

  NABoolean isValid() const 		{ return value_ != NULL; }

  NAString getValueAsString(const IntervalType& dt) const;

  void print(const IntervalType& dt,
	     FILE* ofd = stdout, const char* indent = DEFAULT_INDENT) const;

private:

  // ---------------------------------------------------------------------
  // Scan the given character.
  // ---------------------------------------------------------------------
  NABoolean scanChar(const char* &strValue, char c) const
  {
    if (*strValue == c) {
      strValue++;
      return TRUE;
    }
    return FALSE;
  }

  // ---------------------------------------------------------------------
  // Scan the given field.
  // ---------------------------------------------------------------------
  NABoolean scanField(const char* &strValue,
                      UInt32 maxFieldLen,
                      Lng32 &value) const;

  NABoolean scanField(const char* &strValue,
                      UInt32 maxFieldLen,
                      Int64 &value) const;

  // ---------------------------------------------------------------------
  // Interval value.
  // ---------------------------------------------------------------------
  unsigned char* value_;

  // ---------------------------------------------------------------------
  // Length of value in bytes.
  // ---------------------------------------------------------------------
  unsigned short valueLen_;

}; // class IntervalValue

#endif /* INTERVALTYPE_H */
