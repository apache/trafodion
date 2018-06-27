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
#ifndef DATETIMETYPE_H
#define DATETIMETYPE_H
/* -*-C++-*-
**************************************************************************
*
* File:         DatetimeType.h
* Description:  Datetime Type
* Created:      2/1/96
* Language:     C++
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------

#include "NAType.h"
#include "DTICommonType.h"
#include "IntervalType.h"
#include "SQLCLIdev.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class DatetimeType;
class SQLDate;
class SQLTime;
class SQLTimestamp;
class DatetimeValue;

extern NAString LiteralDate;
extern NAString LiteralTime;
extern NAString LiteralTimestamp;
extern NAString LiteralDateTime;
extern NAString DEFAULT_DATE_DISPLAY_FORMAT;

// static NAString DEFAULT_TIME_DISPLAY_FORMAT("HH:MM:SS");
// static NAString DEFAULT_TIMESTAMP_DISPLAY_FORMAT("YYYY-MM-DD HH:MM:SS.FFFFF");

// ***********************************************************************
//
//  DatetimeType : The datetime data type
//
// ***********************************************************************
class DatetimeType : public DatetimeIntervalCommonType
{
public:

  enum { DEFAULT_FRACTION_PRECISION = 0 };      // See ANSI 6.1 SR 25: zero
  enum { MAX_FRACTION_PRECISION_USEC = 6 }; // max microseconds fract precision
  enum { MAX_FRACTION_PRECISION = 9 };  // See ANSI 6.1 SR 26: max is at least 6.
                                        // Changed to 9 to support nano secs 
                                        // fractional precision

  enum Subtype { SUBTYPE_ILLEGAL,
                 SUBTYPE_SQLDate, 
                 SUBTYPE_SQLTime, 
                 SUBTYPE_SQLTimestamp, 
                 SUBTYPE_SQLMPDatetime };

  static Subtype validate(rec_datetime_field startField,
			  rec_datetime_field endField,
			  UInt32 fractionPrecision);

  static DatetimeType* constructSubtype(NABoolean allowSQLnull,
					rec_datetime_field startField,
			    		rec_datetime_field endField,
					UInt32 fractionPrecision,
					NAMemory* h=0);

  static Lng32 getStorageSize(rec_datetime_field startField,
			     rec_datetime_field endField,
			     UInt32 fractionPrecision = 0);

  // Used by DatetimeValue
  static Int32 getExtendedEndField(rec_datetime_field endField,
				 UInt32 fractionPrecision);

  void datetimeToLong(void *bufPtr,
                      ULng32 values[]) const;
  

  Lng32 getRecDateTimeCode() const  // does this need to be virtual ??
   {
    return getRecDateTimeCode (getStartField(), getEndField());
   }

  Lng32 getRecDateTimeCode(rec_datetime_field startField, rec_datetime_field endField) const;

 NABoolean checkValid(ComDiagsArea *diags)
  {
    if ((getFractionPrecision()) > MAX_FRACTION_PRECISION)
      {
	// ## Possible future work:
	// We COULD add an optional Diags parameter onto our validate() method
	// and call it now; and it would emit more precise diagnostics
	// for why the type is invalid.  It is a complicated type after all!
	// The more precise diags could explain valid ranges of values, etc.
	//
	// Here we just emit "3134 Invalid TimeStamp $int0."
	*diags << DgSqlCode(-3134) << DgInt0(getFractionPrecision());
	return FALSE;
      }
    return TRUE;
  }

  static NABoolean leapYear(ULng32 year) 
  {
    return ((year % 4 == 0) &&
            ((year % 100 != 0) ||
             (year % 400 == 0)));
  }

  static Lng32 gregorianDays(const ULng32 values[]);
  
  static Lng32 secondsInTime(const ULng32 values[]);
  
  // returns the juliantimestamp value (64 bit integer) based on the
  // input sql timestamp value.
  // Input format is the same as what is created in 'class DatetimeValue'
  // Input fractionPrec is the fractional seconds precision, needed to scale
  //                    the fractional seconds part of the timestamp to a
  //                    number of microseconds.
  static Int64 julianTimestampValue(const char * value, const short valueLen,
                                    UInt32 fractionPrec);

  // ---------------------------------------------------------------------
  // Constructors
  // ---------------------------------------------------------------------
  DatetimeType (NAMemory *h, const NAString & adtName,
                NABoolean allowSQLnull,
                rec_datetime_field startField,
                rec_datetime_field endField,
                UInt32 fractionPrecision = 0);

  // copy ctor
  DatetimeType (const DatetimeType & rhs, NAMemory * h=0) :
       DatetimeIntervalCommonType (rhs, h)
       , displayFormat_(rhs.displayFormat_, h)
  {}

  DatetimeType & operator= (const DatetimeType &) ; // not written 

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  Subtype getSubtype() const
  {
    return (validate(getStartField(), getEndField(), getFractionPrecision()));
  }

  virtual Lng32 getDisplayLength() const;

  virtual Lng32 getDisplayLength(Lng32 datatype,
 		        Lng32 length,
		        Lng32 precision,
		        Lng32 scale,
                        Lng32 heading_len) const;

  const NAString &getDisplayFormat() const { return displayFormat_; }

  // ---------------------------------------------------------------------
  // Mutator functions
  // ---------------------------------------------------------------------

  void setDisplayFormat(const char* format) { displayFormat_ = format; }
  void setDisplayFormat(const NAString& format) { displayFormat_ = format; }

  // ---------------------------------------------------------------------
  // Virtual functions that are given an implementation for this class.
  // ---------------------------------------------------------------------
  virtual void print(FILE* ofd = stdout, const char* indent = DEFAULT_INDENT) /*const*/;

  virtual short getFSDatatype() const;

  virtual NAString getSimpleTypeName() const;

  virtual NAString getTypeSQLname(NABoolean terse = FALSE) const;

  virtual const NAString getDatetimeQualifierAsString(NABoolean includeFractionalPrec) const;

  virtual NABoolean isCompatible(const NAType& other, UInt32 * flags = NULL) const;

#if defined( NA_LITTLE_ENDIAN )
  virtual NABoolean isEncodingNeeded() const;
#endif
  
  virtual NABoolean operator==(const NAType& other) const;

  // ---------------------------------------------------------------------
  // A virtual function for synthesizing the type of a binary operator.
  // ---------------------------------------------------------------------
  virtual const NAType* synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                       const NAType& operand1,
                                       const NAType& operand2,
				       NAMemory* h,
				       UInt32 *flags = NULL) const;

  // ---------------------------------------------------------------------
  // A virtual function for synthesizing the type of a ternary operator.
  // ---------------------------------------------------------------------
  virtual const NAType* synthesizeTernary(enum NATypeSynthRuleEnum synthRule,
                                          const NAType& operand1,
                                          const NAType& operand2,
                                          const NAType& operand3,
					  NAMemory* h=0) const;

  virtual NAType::SynthesisPrecedence getSynthesisPrecedence() const;
  
  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  virtual void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString** stringLiteral = NULL,
                                     NAMemory* h=0) const;
  virtual void maxRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString** stringLiteral = NULL,
                                     NAMemory* h=0) const;

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
  // *unimplemented*, so that DatetimeType, just like NAType, 
  // remains an abstract class:  only SQLDate, SQLTime, SQLTimestamp
  // can be instantiated.
  //
  //	virtual NAType *newCopy() const { return new DatetimeType(*this); }
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Generic way for finding if a DatetimeType contains a particuar Datetime 
  // field.
  // ---------------------------------------------------------------------
virtual NABoolean containsField(rec_datetime_field theField) const;

  // ---------------------------------------------------------------------
  // Generic way for finding if two DatetimeTypes contain any common Datetime 
  // fields.
  // ---------------------------------------------------------------------

virtual NABoolean fieldsOverlap(const DatetimeType& other) const;

private:

  // Auxiliary methods
  void getRepresentableValue(const char* inValueString,
			     void* bufPtr, Lng32* bufLen,
			     NAString** stringLiteral,
			     NAMemory* h=0) const;

  NAString displayFormat_;
  
}; // class DatetimeType

// ***********************************************************************
//
//  SQLDate : SQL DATE
//
// ***********************************************************************
class SQLDate : public DatetimeType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions 
  // ---------------------------------------------------------------------
  SQLDate(NAMemory *h, NABoolean allowSQLnull = TRUE)
       : DatetimeType (h, LiteralDate,
                       allowSQLnull,
                       REC_DATE_YEAR,
                       REC_DATE_DAY,
                       0)
  {}

  // copy ctor
  SQLDate (const SQLDate & rhs, NAMemory * h=0)
       : DatetimeType (rhs, h) 
  {}
  
  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(NAMemory* h=0) const 	
    { return new (h) SQLDate(*this, h); }

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void* bufPtr) const;

  // Method that returns a SQL Date literal representing 
  // the value (as #seconds) in v.
  NAString* convertToString(double v, NAMemory* h) const;
   
  virtual Lng32 getPrecision() const { return SQLDTCODE_DATE; }

  // get min and max value 
  virtual double getMinValue() const; // the encode for '0001-01-01'
  virtual double getMaxValue() const; // the encode for '9999-12-31'

protected:
  double encodeSqlLiteral(char* literal) const;

private:
  
}; // class SQLDate

// ***********************************************************************
//
//  SQLTime : SQL TIME
//
// ***********************************************************************
class SQLTime : public DatetimeType
{
public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLTime(NAMemory *h, NABoolean allowSQLnull = TRUE, 
	  UInt32 fractionPrecision = DEFAULT_FRACTION_PRECISION)
       : DatetimeType (h, LiteralTime,
                       allowSQLnull,
                       REC_DATE_HOUR,
                       REC_DATE_SECOND,
                       fractionPrecision)
  {}

  // copy ctor
  SQLTime (const SQLTime & rhs, NAMemory * h=0)
       : DatetimeType (rhs, h) 
  {}

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(NAMemory* h=0) const 	
    { return new (h) SQLTime(*this, h); }

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void* bufPtr) const;

  // Method that returns a SQL Time literal representing 
  // the value (as #seconds with fractions) in v.
  NAString* convertToString(double v, NAMemory* h) const;

   
  virtual Lng32 getPrecision() const { return SQLDTCODE_TIME; }

  virtual Lng32 getScale() const { return getFractionPrecision(); }

  // get min and max value 
  virtual double getMinValue()  const; // the encode for '00:00:00'
  virtual double getMaxValue()  const; // the encode for '23:59:59.<x>'
                                       // where <x> is 99...9 with 
                                       // the number of digits '9'
                                       // specified by the fraction precision.

protected:
  double encodeSqlLiteral(char* literal) const;

private:
  
}; // class SQLTime

// ***********************************************************************
//
//  SQLTimestamp : SQL TIMESTAMP
//
// ***********************************************************************
class SQLTimestamp : public DatetimeType
{
public:

  enum { DEFAULT_FRACTION_PRECISION = 6 };	// See ANSI 6.1 SR 25: six

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------

  SQLTimestamp(NAMemory *h, NABoolean allowSQLnull = TRUE, 
               UInt32 fractionPrecision = DEFAULT_FRACTION_PRECISION)
       : DatetimeType (h, LiteralTimestamp,
                       allowSQLnull,
                       REC_DATE_YEAR,
                       REC_DATE_SECOND,
                       fractionPrecision)
  {}

  // copy ctor
  SQLTimestamp (const SQLTimestamp & rhs, NAMemory * h=0) :
       DatetimeType (rhs, h)
  {}

  SQLTimestamp & operator= (const SQLTimestamp &) ; // not written

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(NAMemory* h=0) const 	
    { return new (h) SQLTimestamp(*this, h); }

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void* bufPtr) const;

  // Method that returns a SQL timestamp literal representing 
  // the value in v.
  NAString* convertToString(double v, NAMemory* h) const;

   virtual Lng32 getPrecision() const { return SQLDTCODE_TIMESTAMP; }

   virtual Lng32 getScale() const { return getFractionPrecision(); }
 
  // get the min value, the encode for '0001-01-01 00:00:00'
  virtual double getMinValue()  const; 

  // get the max value, the encode for '9999-12-31 23:59:59.<x>',
  // where <x> is 99...9 with the number of digits '9' specified
  // by fraction precision
  virtual double getMaxValue()  const; 

protected:
  double encodeSqlLiteral(char* literal) const;

private:
  
}; // class SQLTimestamp

// ***********************************************************************
//
//  SQLMPDatetime : A Datetype for non-ANSI SQL/MP Datetimes
//
// ***********************************************************************
class SQLMPDatetime : public DatetimeType
{
public:

    SQLMPDatetime(NAMemory *h, rec_datetime_field startField,
                  rec_datetime_field endField,
                  NABoolean allowSQLnull = TRUE, 
                  UInt32 fractionPrecision = DEFAULT_FRACTION_PRECISION)
  : DatetimeType (h, LiteralDateTime,
                  allowSQLnull,
                  startField,
                  endField,
                  ((endField >= REC_DATE_SECOND) ?
                     fractionPrecision : 
                                                  0))
  {}

        SQLMPDatetime (const SQLMPDatetime & rhs, NAMemory * h=0)
        : DatetimeType (rhs, h)
        {}

        virtual NAType *newCopy(NAMemory* h=0) const  
        { return new (h) SQLMPDatetime(*this, h); }

        virtual double encode (void* bufPtr) const;
 
        virtual Lng32 getPrecision() const { return SQLDTCODE_MPDATETIME; } 

        virtual Lng32 getScale() const { return getFractionPrecision(); }

        virtual NABoolean isCompatible(const NAType& other, UInt32 * flags = NULL) const;
  
        virtual const NAType* synthesizeType(enum NATypeSynthRuleEnum synthRule,     // in common\DateTime.cpp
                                            const NAType& operand1,
                                            const NAType& operand2,
                                            NAMemory* h,
                                            UInt32 *flags = NULL
                                            ) const;
        virtual NABoolean isSupportedType() const;

private:                                          
};
 
// ***********************************************************************
//
//  DatetimeValue : A datetime value
//
// ***********************************************************************
class DatetimeValue
{
public:

  // ---------------------------------------------------------------------
  // Datetime fields.
  // Note that DatetimeField::YEAR == 0, 
  // while rec_datetime_field::REC_DATE_YEAR == 1, and so forth.
  //
  // These are used to index some static arrays in the .C implementation.
  // ---------------------------------------------------------------------
  enum DatetimeField { YEAR = 0, MONTH, DAY, HOUR, MINUTE, SECOND, FRACTION };

  enum { N_DATETIME_FIELDS = FRACTION + 1 };

  // ---------------------------------------------------------------------
  // Constructor functions 
  // ---------------------------------------------------------------------
  DatetimeValue (const char* strValue,
                 rec_datetime_field startField,
                 rec_datetime_field endField,
                 UInt32& fractionPrecision /* OUT */,
                 NABoolean useOldConstructor);

  DatetimeValue (const char* value,
                 Lng32 storageSize);

  void oldConstructor(const char* strValue,
                      rec_datetime_field startField,
                      rec_datetime_field endField,
                      UInt32& fractionPrecision /* OUT */);

  // ---------------------------------------------------------------------
  // Destructor functions 
  // ---------------------------------------------------------------------
  ~DatetimeValue()
  {
    delete [] value_;
  }

  // ---------------------------------------------------------------------
  // Set the value.
  // ---------------------------------------------------------------------
  void setValue(rec_datetime_field startField,
                rec_datetime_field endField,
		UInt32 fractionPrecision,
                const ULng32 values[]);

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  const unsigned char* getValue() const	{ return value_; }

  unsigned short getValueLen() const 	{ return valueLen_; }

  NABoolean isValid() const 		{ return value_ != NULL; }

  NAString getValueAsString(const DatetimeType& dt) const;

  void print(const DatetimeType& dt, 
	     FILE* ofd = stdout, const char* indent = DEFAULT_INDENT) const;

  // ---------------------------------------------------------------------
  // Static function to get the datetime fields from a Julian timestamp into
  // an array suitable for input to setValue().
  // ---------------------------------------------------------------------
  static void decodeTimestamp(Int64 julianTimestamp,
                              UInt32 fracSec,
                              UInt32* dtvFields);

private:
  
  // ---------------------------------------------------------------------
  // Datetime formats.
  // ---------------------------------------------------------------------
  enum DatetimeFormat {
    FORMAT_DEFAULT,
    FORMAT_USA,
    FORMAT_EUROPEAN,
    FORMAT_ANY
  };

  // ---------------------------------------------------------------------
  // Scan the date.
  // ---------------------------------------------------------------------
  NABoolean scanDate(const char* &strValue,
                     DatetimeFormat &format,
                     ULng32 values[]) const;

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
  // Scan the time.
  // ---------------------------------------------------------------------
  NABoolean scanTime(const char* &strValue,
                     DatetimeFormat format,
                     ULng32 values[],
                     UInt32 &fractionPrecision) const;

  // ---------------------------------------------------------------------
  // Scan the given field.
  // ---------------------------------------------------------------------
  NABoolean scanField(const char* &strValue,
                      DatetimeField field,
                      ULng32 values[]) const;

  // ---------------------------------------------------------------------
  // Determine if the type contains the indicated field.
  // ---------------------------------------------------------------------
  NABoolean containsField(DatetimeField theField, 
                          rec_datetime_field startField, 
                          rec_datetime_field endField) const
   {
    if (theField+1 >= startField && theField+1 <= endField)
     {
      return TRUE;
     }
    else
     {
      return FALSE;
     } 
   }

  // ---------------------------------------------------------------------
  // Validate the date.
  // ---------------------------------------------------------------------
  NABoolean validateDate(const ULng32 values[]) const;

  // ---------------------------------------------------------------------
  // Datetime value.
  // ---------------------------------------------------------------------
  unsigned char* value_;

  // ---------------------------------------------------------------------
  // Length of value in bytes.
  // ---------------------------------------------------------------------
  unsigned short valueLen_;

  rec_datetime_field startField_;
  rec_datetime_field endField_;


}; // class DatetimeValue

#endif /* DATETIMETYPE_H */

