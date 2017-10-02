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
#ifndef NUMERICTYPE_H
#define NUMERICTYPE_H
/* -*-C++-*-
**************************************************************************
*
* File:         NumericType.h
* Description:  Numeric Type
* Created:      4/27/94
* Language:     C++
*
*
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------

#include <limits.h>
#include <math.h>
#include "BaseTypes.h"
#include "NAType.h"
#include "ComDiags.h"
#include "BigNumHelper.h"
// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class NumericType;
class SQLBPInt;
class SQLTiny;
class SQLSmall;
class SQLInt;
class SQLLargeInt;
class SQLNumeric;
class SQLDecimal;
class LSDecimal;
class SQLBigNum;
class SQLFloat;
class SQLReal;
class SQLDoublePrecision;

extern  NAString LiteralInteger;
extern  NAString LiteralTinyInt;
extern  NAString LiteralSmallInt;
extern  NAString LiteralBPInt;
extern  NAString LiteralLargeInt;
extern  NAString LiteralNumeric;
extern  NAString LiteralDecimal;
extern  NAString LiteralBigNum;
extern  NAString LiteralLSDecimal;
extern  NAString LiteralFloat;
extern  NAString LiteralReal;
extern  NAString LiteralDoublePrecision;

// -----------------------------------------------------------------------
// utility functions
// -----------------------------------------------------------------------

unsigned short getBinaryStorageSize(Lng32 precision);

// ***********************************************************************
//
//  NumericType : The numeric data type
//
// ***********************************************************************
class NumericType : public NAType
{
public:

  // ---------------------------------------------------------------------
  // A method which tells if a conversion error can occur when converting
  // a value of this type to the target type.
  // ---------------------------------------------------------------------
  NABoolean errorsCanOccur (const NAType& target, NABoolean lax=TRUE) const;

  // ---------------------------------------------------------------------
  // A name that expresses the characteristics of the type.
  // ---------------------------------------------------------------------
  virtual NAString getTypeSQLname(NABoolean terse = FALSE) const;

  virtual NAString getSimpleTypeName() const { return getTypeName(qualifier_); }

  virtual short getFSDatatype() const;

  NABoolean isExact() const
  {
    return qualifier_ >= MIN_EXACT_NUMERIC_TYPE &&
           qualifier_ <= MAX_EXACT_NUMERIC_TYPE;
  }

  NABoolean isAnyUnsignedInt() const
  {
    return isExact() AND isUnsigned() AND binaryPrecision();
  }

  NABoolean isInteger() const;

  NABoolean isDecimal() const      { return qualifier_ == SQLDecimal_TYPE ||
				  	    qualifier_ == LSDecimal_TYPE; }
  NABoolean isBigNum() const { return qualifier_ == SQLBigNum_TYPE;}
  NABoolean isInternalType() const { return (isBigNum()); }
 
  NABoolean supportsSign () const  {return isExact(); }

  // ---------------------------------------------------------------------
  // Accessor functions for the precision, magnitude, scale and unsigned
  // indicator.  Magnitude is scaled up by a factor of 10, because integers
  // do not have exact decimal precision.  For example, a NUMERIC(5) has a
  // precision of 5 digits so its magnitude is 50.  A signed SMALLINT has a
  // precision of between 4 and 5 digits, so its magnitude is 45.
  // For SMALLINT, INTEGER and LARGEINT, magnitude is 10 * log m
  // where m is the maximum representable value.  For example, signed 
  // SMALLINT is 45 and unsigned SMALLINT is 48.
  // ---------------------------------------------------------------------
  virtual Lng32 getPrecision() const { return precision_;}  

  Lng32 getBinaryPrecision() const;
  
  // ALERT: ANTI-KLUDGE! ALERT: ANTI-KLUDGE! ALERT: ANTI-KLUDGE!
  // The above functions: getPrecision() and getBinaryPrecision() may fool
  // us into thinking they always return their type's precision. No!
  // For many floating-point types, they actually return zero! Some developer
  // coded into sqlparser.y, NAType.cpp, CatDataType.cpp the assumption that
  // "zero precision distinguishes REAL vs FLOAT, DOUBLE PRECISION vs FLOAT".
  // This seemingly innocent assumption misled us into thinking we can use
  // the above functions in NumericType::errorsCanOccur to establish the
  // safety of caching predicates like "floating-point-key-column = 1.23".
  // That was true in R1.8.5. But, the IEEE floating-point related changes
  // interacted with this misleading assumption to make this false. The result
  // is genesis case 10-040407-8352. "double-precision-key-column = 1.23 is
  // no longer a cacheable predicate" because getBinaryPrecision() returns
  // zero. If we try to fix this case by making getBinaryPrecision() return
  // the true result, we will break other tests such as compGeneral/test028
  // because 
  //   "create table t(d double precision)" 
  // followed by "showddl t" would unexpectedly return 
  //   "create table t(d float(54))"
  // So, to fix this case without breaking other tests, we are forced to
  // introduce the following two new functions that return the true precision
  // for numeric types including all floating-point types.
  virtual Lng32 getTruePrecision() const { return getPrecision(); }  
  Lng32 getTrueBinaryPrecision() const;
  
  virtual Lng32 getMagnitude() const { return (precision_ - scale_) * 10; }

  // should this numeric type be checked for value type fitting in query cache?
  virtual NABoolean shouldCheckValueFitInType() const { return FALSE; }
  // get normalized value from the buffer
  virtual double getNormalizedValue(void*) const { return -1; }

  virtual Lng32 getScale() const { return scale_; }

  virtual Lng32 getColFlags() const { return colFlags_; }

  virtual NABoolean isUnsigned() const { return unsigned_; }
  NABoolean isSigned() const { return NOT isUnsigned(); }
  virtual void makeUnsigned() { unsigned_ = TRUE; }
  virtual void makeSigned() { unsigned_ = FALSE; }
  
  // ---------------------------------------------------------------------
  // Does this data type use decimal or binary precision?
  // ---------------------------------------------------------------------
  virtual NABoolean decimalPrecision() const { return FALSE; }
  virtual NABoolean binaryPrecision() const { return TRUE; }
  
  void setScale(Lng32 scale){scale_ = scale;};
  void setColFlags(Lng32 colFlags){colFlags_ = colFlags;};
  void setPrecision(Lng32 precision){precision_ = precision;};
      
  // ---------------------------------------------------------------------
  // Methods for comparing if two ADT definitions are equal.
  // ---------------------------------------------------------------------
  virtual NABoolean operator==(const NAType& other) const;

  // This method is identical to operator==() except the null attribute is not
  // compared.
  virtual NABoolean equalIgnoreNull(const NAType& other) const;

  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  virtual void minRepresentableValue(void*, Lng32*, 
				     NAString ** stringLiteral = NULL,
				     CollHeap* h=0) const;
  virtual void maxRepresentableValue(void*, Lng32*, 
				     NAString ** stringLiteral = NULL,
				     CollHeap* h=0) const;

  virtual NABoolean createSQLLiteral(const char * buf,       // in
                                     NAString *&sqlLiteral,  // out
                                     NABoolean &isNull,      // out
                                     CollHeap *h) const;     // in/out

  // ---------------------------------------------------------------------
  //  Method that returns the encoded form (in floating point) for a 
  //  given value.  This value will then be used by the optimizer
  //  for estimations and selectivity computation.
  // ---------------------------------------------------------------------
  virtual double encode (void*) const;

  virtual NABoolean isEncodingNeeded() const;

  // ---------------------------------------------------------------------
  // A virtual function for synthesizing the type of a binary operator.
  // ---------------------------------------------------------------------
  virtual const NAType* synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                       const NAType& operand1,
                                       const NAType& operand2,
				       CollHeap* h,
				       UInt32 *flags = NULL) const;

  // ---------------------------------------------------------------------
  // Print function for debugging
  // ---------------------------------------------------------------------
  virtual void print(FILE* ofd = stdout, const char *indent = DEFAULT_INDENT);
   
  // ---------------------------------------------------------------------
  // A method for generating the hash key.
  // SQL builtin types should return getTypeSQLName() 
  // ---------------------------------------------------------------------
  virtual NAString *getKey(CollHeap* h=0) const;
 
  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const 
    { return new(h) NumericType(*this, h); }

protected:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  NumericType ( NAMemory *heap,
                       const NAString&  adtName, 
	    	       Lng32             dataStorageSize,
		       Lng32             precision,
		       Lng32             scale,
		       Lng32             alignment,
		       NABoolean        allowNegValues = TRUE,
		       NABoolean        allowSQLnull = TRUE,
 		       NABoolean        varLenFlag = FALSE
                     );
   NumericType ( const NumericType& numeric, CollHeap * heap =0);

private:

  // ---------------------------------------------------------------------
  // Declaration for different data type names that are used internally.
  // ---------------------------------------------------------------------
  enum NumericTypeEnum 
  {
    MIN_NUMERIC_TYPE,
    MIN_EXACT_NUMERIC_TYPE,
    SQLBPInt_TYPE,
    SQLTiny_TYPE,
    SQLSmall_TYPE,
    SQLInt_TYPE,
    SQLNumeric_TYPE,
    SQLDecimal_TYPE,
    LSDecimal_TYPE,
    SQLBigNum_TYPE,
    SQLLarge_TYPE,
    MAX_EXACT_NUMERIC_TYPE,
    MIN_APPROX_NUMERIC_TYPE,
    SQLReal_TYPE,
    SQLFloat_TYPE,
    SQLDoublePrecision_TYPE,
    MAX_APPROX_NUMERIC_TYPE,
    MAX_NUMERIC_TYPE
  };

  // ---------------------------------------------------------------------
  // Method for mapping a data type name string to its internal form.
  // ---------------------------------------------------------------------
  NumericTypeEnum tokenizeTypeName(const NAString& adtName) const;

  NAString getTypeName(NumericTypeEnum ntev) const;

  // ---------------------------------------------------------------------
  // Definition of variable for storing internal form names.
  // ---------------------------------------------------------------------
  NumericTypeEnum  qualifier_;  

  // ---------------------------------------------------------------------
  // The precision_ is an overloaded value. It contains either
  // 1) the decimal precision
  //    - the total number of digits that are representable in
  //      storage of size size_. Some exact numeric data types use 
  //      the decimal precision.
  // 2) the binary precision
  //    - the total number of bits that are used for representing
  //      the mantissa. Some exact and all approximate numeric data 
  //      types use the binary precision.
  // ---------------------------------------------------------------------
  Lng32  precision_;
  
  // ---------------------------------------------------------------------
  // The scale_ is another overloaded value. It contains either
  // 1) a positive integer when it is used in conjunction with
  //    a decimal precision. The scale indicates the number of 
  //    digits that are in the fractional part of an exact
  //    numeric data type.
  //       numeric value = exact numeric value * 10**(-scale_)
  // 2) a signed integer when it is used in conjunction with a
  //    binary precision. The scale_ contains the signed exponent
  //       numeric value = mantissa * 10**(scale_)
  // ---------------------------------------------------------------------
  Lng32  scale_;

  // ---------------------------------------------------------------------
  // If true, then 2**(n-1) more values can be represented in the 
  // given size_, where n = binary precision.
  // ---------------------------------------------------------------------
  NABoolean unsigned_;

  //----------------------------------------------------------------------
  // colFlags_ is a 32 bit general purpose flag. 
  // BIT 0 is reserved for ZNS format, where its value indicates
  // whether this column is subjected to ZNS format or not.
  // Other bits are available for future work. 
  // ---------------------------------------------------------------------
  Lng32 colFlags_;
  
}; // class NumericType




// ***********************************************************************
//
//  SQLBPInt : Bit Precision Integer, SQL/ARK specific. Allows integers 
//             smaller than SMALLINTs. Used for packing  vertical partition
//             (VP) column values to reduce queue and storage overheads. 
//             Defined as: BIT PRECISION (size) UNSIGNED
//             Restrictions: (1) only UNSIGNED allowed; (2) 1 <= size <= 15
//
// ***********************************************************************
class SQLBPInt : public NumericType
{
public: 
  // Constructor function
   SQLBPInt (NAMemory *heap, UInt32 declared, 
                   NABoolean allowSQLnull = TRUE, 
                   NABoolean allowNegValues = FALSE
                  );

  short getFSDatatype () const
  {
	  return REC_BPINT_UNSIGNED;
  }

  inline UInt32 getDeclaredLength () const
  {
	  return declaredSize_;	  // in number of bits
  }

  virtual Lng32 getMagnitude () const 
  {
    // Magnitude = 10 * log (2**declaredSize)
    // Must be greater than 10 to make precision >= 1
    return MAXOF ((Lng32) (10 * declaredSize_ * 0.30103) , 10);
  }
	 
  virtual double getMaxValue () const 
  {
    assert((declaredSize_>=1) && (declaredSize_<=15));
    static const unsigned short limits[] = {   1,    3,    7,    15,    31,   
	                              63,  127,  255,   511,  1023, 
				    2047, 4095, 8191, 16383, 32767 };
    return limits[declaredSize_-1];
  }
	 
  virtual NABoolean shouldCheckValueFitInType() const { return TRUE; }

  NABoolean decimalPrecision () const { return FALSE; }

  NABoolean binaryPrecision () const { return TRUE; } 

  virtual NABoolean operator==(const NAType& other) const;

  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;
  void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void* bufPtr) const;

  NAString* convertToString(double v, CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // No special type synthesis necessary. When two BPInts are involved
  // in a binary operation, the result will be a smallint. Otherwise, type 
  // will be determined by the other operand.  
  // ---------------------------------------------------------------------  
  
  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLBPInt(h, declaredSize_, supportsSQLnull(), !isUnsigned());
  }

private:
  UInt32 declaredSize_;   // declared size in bits 
  
}; // class SQLBPInt

// ***********************************************************************
//
//  SQLTiny : SQL TINY
//
// ***********************************************************************
class SQLTiny : public NumericType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLTiny (NAMemory *heap, NABoolean allowNegValues = TRUE, 
           NABoolean allowSQLnull = TRUE);
   short getFSDatatype() const
    {
      if (isUnsigned())
	return REC_BIN8_UNSIGNED;
      else
	return REC_BIN8_SIGNED;
    }

  NABoolean roundTripConversionToDouble() const { return TRUE; };

  virtual Lng32 getMagnitude() const { return isUnsigned() ? 28 : 25; }

  virtual double getMaxValue() const { return isUnsigned() ?  255 : 127; }

  virtual double getMinValue() const { return isUnsigned() ?  0 : -127; }

  virtual NABoolean shouldCheckValueFitInType() const { return TRUE; }

  // ---------------------------------------------------------------------
  // Does this data type use decimal or binary precision?
  // ---------------------------------------------------------------------
   NABoolean decimalPrecision() const { return FALSE; }

   NABoolean binaryPrecision() const { return TRUE; }
  
  virtual void makeUnsigned() 
  { 
    setPrecision(SQL_UTINY_PRECISION);
    NumericType::makeUnsigned();
  }

  virtual void makeSigned() 
  { 
    setPrecision(SQL_TINY_PRECISION);
    NumericType::makeSigned();
  }

  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;
  void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  NAString* convertToString(double v, CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void* bufPtr) const;

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLTiny(h, !isUnsigned()
                          ,supportsSQLnull()
                          );
  }

  virtual double getNormalizedValue(void* buf) const 
  {
    if (isUnsigned())
      return  (double)(*(unsigned char *)buf);
    else
      return  (double)(*(char *)buf);
  };

private:
  
}; // class SQLTiny

// ***********************************************************************
//
//  SQLSmall : SQL SMALLINT
//
// ***********************************************************************
class SQLSmall : public NumericType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLSmall (NAMemory *heap, NABoolean allowNegValues = TRUE, 
                   NABoolean allowSQLnull = TRUE);

   short getFSDatatype() const
    {
      if (isUnsigned())
	return REC_BIN16_UNSIGNED;
      else
	return REC_BIN16_SIGNED;
    }

  NABoolean roundTripConversionToDouble() const { return TRUE; };

  virtual Lng32 getMagnitude() const { return isUnsigned() ? 48 : 45; }

  virtual double getMaxValue() const { return isUnsigned() ?  65535 : 32767; }

  virtual double getMinValue() const { return isUnsigned() ?  0 : -32766; }

  virtual NABoolean shouldCheckValueFitInType() const { return TRUE; }

  // ---------------------------------------------------------------------
  // Does this data type use decimal or binary precision?
  // ---------------------------------------------------------------------
   NABoolean decimalPrecision() const { return FALSE; }

   NABoolean binaryPrecision() const { return TRUE; }
  
  virtual void makeUnsigned() 
  { 
    setPrecision(SQL_USMALL_PRECISION);
    NumericType::makeUnsigned();
  }

  virtual void makeSigned() 
  { 
    setPrecision(SQL_SMALL_PRECISION);
    NumericType::makeSigned();
  }

  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;
  void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  NAString* convertToString(double v, CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void* bufPtr) const;

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLSmall(h, !isUnsigned()
			   ,supportsSQLnull()
			   );
  }

  virtual double getNormalizedValue(void* buf) const 
  {
    if (isUnsigned())
      return  (double)(*(unsigned short *)buf);
    else
      return  (double)(*(short *)buf);
  };

private:
  
}; // class SQLSmall

// ***********************************************************************
//
//  SQLInt : SQL INTEGER
//
// ***********************************************************************
class SQLInt : public NumericType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLInt (NAMemory *heap, NABoolean allowNegValues = TRUE, 
		 NABoolean allowSQLnull = TRUE);

  short getFSDatatype() const
    {
      if (isUnsigned())
	return REC_BIN32_UNSIGNED;
      else
	return REC_BIN32_SIGNED;
    }

  NABoolean roundTripConversionToDouble() const { return TRUE; };

  virtual Lng32 getMagnitude() const { return isUnsigned() ? 96 : 93; }

  virtual double getMaxValue() const { return isUnsigned() ?  4294967295U : 2147483647; }

  virtual double getMinValue() const { return isUnsigned() ?  0 : -2147483646; }

  virtual NABoolean shouldCheckValueFitInType() const { return TRUE; }

  // ---------------------------------------------------------------------
  // Does this data type use decimal or binary precision?
  // --------------------------------------------------------------------- 
   NABoolean decimalPrecision() const { return FALSE; }

   NABoolean binaryPrecision() const { return TRUE; }
  
  virtual void makeUnsigned() 
  { 
    setPrecision(SQL_UINT_PRECISION);
    NumericType::makeUnsigned();
  }

  virtual void makeSigned() 
  { 
    setPrecision(SQL_INT_PRECISION);
    NumericType::makeSigned();
  }

  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;
  void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  NAString* convertToString(double v, CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void* bufPtr) const;

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLInt(h, !isUnsigned()
			 ,supportsSQLnull()
			 );
  }

  virtual double getNormalizedValue(void* buf) const 
  {
    if (isUnsigned())
      return  (double)(*(ULng32 *)buf);
    else
      return  (double)(*(Lng32 *)buf);
  };

private:
  
}; // class SQLInt



// ***********************************************************************
//
//  SQLLargeInt : SQL INTEGER
//
// ***********************************************************************
class SQLLargeInt : public NumericType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLLargeInt (NAMemory *heap, NABoolean allowNegValues = TRUE, 
	       NABoolean allowSQLnull = TRUE);

  SQLLargeInt (NAMemory *heap, Lng32 scale,
               UInt16 disAmbiguate,  // 64bit
	       NABoolean allowNegValues = TRUE, 
	       NABoolean allowSQLnull = TRUE);

  short getFSDatatype() const
    {
      if (isUnsigned())
        return REC_BIN64_UNSIGNED;
      else
        return REC_BIN64_SIGNED;
    }

  virtual Lng32 getMagnitude() const 
  { return (isUnsigned() ? 195 : 189); }

  virtual double getMaxValue() const 
  { 
    return
      (isUnsigned() ? 18446744073709551615ULL : 9223372036854775807ULL); 
  } 

  virtual double getMinValue() const 
  { 
    return (isUnsigned() ? 0 : -9.2233720368547e+18);
  }

  virtual NABoolean shouldCheckValueFitInType() const { return TRUE; }

  // ---------------------------------------------------------------------
  // Does this data type use decimal or binary precision?
  // ---------------------------------------------------------------------
   NABoolean decimalPrecision() const { return FALSE; }

   NABoolean binaryPrecision() const { return TRUE; }
  
  // --------------------------------------------------------------------
  // Accessor. To provide access to the private ClientDataType_ of the 
  // derived class.
  // -------------------------------------------------------------------
  inline NAString getClientDataTypeName() const ; 

  // --------------------------------------------------------------------
  // Mutator. To allow the derived class to initialize the private 
  // data member.
  //---------------------------------------------------------------------
  inline void setClientDataType(NAString clientDataTypeName) ;

  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;
  void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void* bufPtr) const;

  NAString* convertToString(double v, CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLLargeInt(h, getScale(),
                              (UInt16) 0,
			      !isUnsigned()
			      ,supportsSQLnull()
			      );
  }

  virtual double getNormalizedValue(void* buf) const 
  { return (double) *(Int64 *)buf; }

private:

  NAString clientDataType_; // added the protected string to distinguish 
                            // a bigint from a largeint for ODBC group.
  
}; // class SQLLargeInt

// ***********************************************************************
// Implementation for inline functions
// ***********************************************************************


NAString SQLLargeInt::getClientDataTypeName() const 
{
 
  if (clientDataType_.isNull())
     return getSimpleTypeName();
  return clientDataType_ ;

}     
 
void SQLLargeInt::setClientDataType(NAString clientDataTypeName)
{

  clientDataType_ = clientDataTypeName;

}
  

// **********************************************************************
// 
// SQLBigInt: SQLBigInt. Basically it is the same as SQLLargeInt.
//
// **********************************************************************

class SQLBigInt : public SQLLargeInt
{
    
public: 
      
  SQLBigInt (NAMemory *heap, NABoolean allowNegValues = TRUE, 
		      NABoolean allowSQLnull = TRUE);

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLBigInt(h, !isUnsigned()
			    ,supportsSQLnull()
			    );
  }

private:


}; // SQLBigInt()

 
// ***********************************************************************
//
//  SQLNumeric : SQL NUMERIC. Binary numbers with decimal precision.
//                 defined as NUMERIC(12,2), etc.
//
// ***********************************************************************
class SQLNumeric : public NumericType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLNumeric (NAMemory *heap, Lng32 length, Lng32 precision, Lng32 scale,
		     NABoolean allowNegValues = TRUE, 
		     NABoolean allowSQLnull = TRUE);

   // Note: DisAmbiguate arg added so Compiler can distinguish between
   //       this constructor and the one above....for 64bit project.
   SQLNumeric (NAMemory *heap, NABoolean allowNegValues,
                     Lng32 precision,
                     Lng32 scale,
                     const Int16 DisAmbiguate,
                     NABoolean allowSQLnull = TRUE);

   short getFSDatatype() const
    {
      if (isUnsigned())
	{
	  if (getNominalSize() == sizeof(UInt8))
	    return REC_BIN8_UNSIGNED;
	  else
            if (getNominalSize() == sizeof(short))
              return REC_BIN16_UNSIGNED;
            else
              if (getNominalSize() == sizeof(Lng32))
                return REC_BIN32_UNSIGNED;
              else 
                return REC_BIN64_UNSIGNED;
	}
      else
	{
	  if (getNominalSize() == sizeof(Int8))
	    return REC_BIN8_SIGNED;
	  else
            if (getNominalSize() == sizeof(short))
              return REC_BIN16_SIGNED;
            else
              if (getNominalSize() == sizeof(Lng32))
                return REC_BIN32_SIGNED;
              else
                return REC_BIN64_SIGNED;
	}
      
    }
  
   NABoolean roundTripConversionToDouble() const
   {  return getNominalSize() <= sizeof(Lng32); };

  // ---------------------------------------------------------------------
  // Does this data type use decimal or binary precision?
  // ---------------------------------------------------------------------
   NABoolean decimalPrecision() const { return TRUE; }

   NABoolean binaryPrecision() const { return FALSE; }
  
  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;
  void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  NAString* convertToString(double v, CollHeap* h=0) const;

  virtual double getMinValue() const;
  virtual double getMaxValue() const;

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void* bufPtr) const;

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLNumeric(h, getNominalSize()
			     ,getPrecision()
			     ,getScale()
			     ,!isUnsigned()
			     ,supportsSQLnull()
			     );
  }

  virtual double getNormalizedValue(void*) const;

private:

}; // class SQLNumeric


// -- Min and max permissible values

// ***********************************************************************
//
//  SQLDecimal : SQL DECIMALINT
//
// ***********************************************************************
class SQLDecimal : public NumericType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLDecimal (NAMemory *heap, Lng32 length, Lng32 scale,
		     NABoolean allowNegValues = TRUE, 
		     NABoolean allowSQLnull = TRUE);

   short getFSDatatype() const
    {
      if (isUnsigned())
	return REC_DECIMAL_UNSIGNED;
      else
	return REC_DECIMAL_LSE;
    }

  // ---------------------------------------------------------------------
  // Does this data type use decimal or binary precision?
  // ---------------------------------------------------------------------
   NABoolean decimalPrecision() const { return TRUE; }

   NABoolean binaryPrecision() const { return FALSE; }
  
  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;
  void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  NAString* convertToString(double v, CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void * bufPtr) const;

  virtual double getMinValue() const;
  virtual double getMaxValue() const;

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLDecimal(h, getNominalSize()
			     ,getScale()
			     ,!isUnsigned()
			     ,supportsSQLnull()
			     );
  }

private:
  
}; // class SQLDecimal


// ***********************************************************************
//
//  SQLBigNum :       Operations on this type are not
//                    supported by hardware (plus, minus, etc) nor can
//                    they be converted to a binary type which can hold
//                    this values (ex. precision > 18).
//
// ***********************************************************************
class SQLBigNum : public NumericType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
   SQLBigNum ( NAMemory *heap, Lng32 precision, Lng32 scale,
	       NABoolean isARealBigNum, // = TRUE,
	       NABoolean allowNegValues, // = TRUE, 
	       NABoolean allowSQLnull ); // = TRUE);

   short getFSDatatype() const
    {
      if (isUnsigned())
	return REC_NUM_BIG_UNSIGNED;
      else
	return REC_NUM_BIG_SIGNED;
    }

  // ---------------------------------------------------------------------
  // For NAType::getDisplayLength().
  // ---------------------------------------------------------------------
 
  virtual Lng32 getDisplayLength() const
                   { return getPrecision() + (getScale() > 0 ? 2 : 1); }

  // ---------------------------------------------------------------------
  // Does this data type use decimal or binary precision?
  // ---------------------------------------------------------------------
  NABoolean decimalPrecision() const { return TRUE; }
  
  NABoolean binaryPrecision() const { return FALSE; }
  
  // operations on Big Nums are 'complex'.  
  
  NABoolean isSimpleType() const  { return FALSE; }
  
  NABoolean isComplexType() const { return TRUE;  }
  
  NABoolean isARealBigNum() const { return isARealBigNum_; }

  void resetRealBigNum()  { isARealBigNum_ = FALSE; } 

  // ---------------------------------------------------------------------
  // Return the closest Equivalent External Type for Large Dec and Big Num
  // ---------------------------------------------------------------------
  virtual NAType * closestEquivalentExternalType(CollHeap* heap=0) const; 

  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void * bufPtr) const;

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
    return SYNTH_PREC_BIG_NUM;
  }

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLBigNum(h, getPrecision()
			    ,getScale()
			    ,isARealBigNum()
			    ,!isUnsigned()
			    ,supportsSQLnull()
			    );
  }

  virtual double getNormalizedValue(void*) const;

private:
  // true, if this bignum was created by user or was synthesized
  // based on a user bignum.
  // false, if this was derived based on non-bignums.
  //  (example:  col1(largeint) * col2(largeint)
  //             will be a BigNum but not isARealBigNum since both operands
  //             are non-bignums)
  NABoolean isARealBigNum_;
}; // class SQLBigNum

// ***********************************************************************
//
//  LSDecimal : Decimal, leading sign separate. Not supported internally.
//              Valid for application host variables only!
//
// ***********************************************************************
class LSDecimal : public NumericType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
   LSDecimal (NAMemory *heap, Lng32 length,
		    Lng32 scale, 
		    NABoolean allowSQLnull = TRUE);

   short getFSDatatype() const
    {
      return REC_DECIMAL_LS;
    }

   NABoolean isExternalType() const
    {
      return TRUE;
    }

   NAType * equivalentType(CollHeap* h=0) const
    {
      return new(h) SQLDecimal(h, getNominalSize() - 1, getScale(),
			       TRUE, supportsSQLnull());
    }

  // ---------------------------------------------------------------------
  // Does this data type use decimal or binary precision?
  // ---------------------------------------------------------------------
   NABoolean decimalPrecision() const { return TRUE; }
  
   NABoolean binaryPrecision() const { return FALSE; }
  
  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;
  void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void * bufPtr) const;

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) LSDecimal(h, getNominalSize()
			    ,getScale()
			    ,supportsSQLnull()
			    );
  }

private:
  
}; // class LSDecimal


// ***********************************************************************
//
//  SQLFloat : SQL FLOAT
//
// ***********************************************************************
class SQLFloat : public NumericType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLFloat
  ( NAMemory *heap, NABoolean allowSQLnull
  , Lng32 dataStorageSize
  , Lng32 precision = SQL_FLOAT_PRECISION
  , const NAString& adtName = LiteralFloat
  )
  : NumericType
  ( heap, adtName
  , dataStorageSize
  , precision
  , 0
  , dataStorageSize
  , TRUE
  , allowSQLnull
  ,FALSE
  )
  { 
    // Should not assert precision <= SQL_FLOAT_PRECISION. 
    // This will caused an assertion failure if the user
    // enter float (x) where x > 54. Instead the parser
    // should call the checkValid method to make sure that
    // the user does not enter the precision greater than the
    // SQL_FLOAT_PRECISION. 

    // assert((precision > 0) && (precision <= SQL_FLOAT_PRECISION));
    assert (precision >= 0);
  }

  // If the above constructor is called with zero precision, that can
  // mislead callers of getPrecision() into thinking type FLOAT
  // has zero precision. The true default precision of type FLOAT is
  // SQL_FLOAT_PRECISION.
  virtual Lng32 getTruePrecision() const 
    { return getPrecision() ? getPrecision() : SQL_DOUBLE_PRECISION; }  

  NABoolean checkValid(ComDiagsArea *diags)
  {
    if ((getPrecision()) > SQL_FLOAT_PRECISION)
      {
	*diags << DgSqlCode(-3135) << DgInt0(getPrecision());
	return FALSE;
      }
    return TRUE;
  }
  // ---------------------------------------------------------------------
  // Does this data type use decimal or binary precision?
  // ---------------------------------------------------------------------
  NABoolean decimalPrecision() const { return FALSE; }
  
  NABoolean binaryPrecision() const { return TRUE; }
  
  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  void minRepresentableValue(void* bufPtr, Lng32* bufLen, 
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;
  void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
			     NAString ** stringLiteral = NULL,
			     CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // Method that returns the encoded form for a given value to be
  // used by the optimizer for estimations.
  // ---------------------------------------------------------------------
  virtual double encode (void * bufPtr) const;

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
    return SYNTH_PREC_FLOAT;
  }

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLFloat(h,
			   supportsSQLnull(), 
			   getNominalSize(),
			   getPrecision(), 
			   getSimpleTypeName());
  }

  virtual double getMaxValue() const { return 1.7976931348623157e+308; }

  virtual NABoolean shouldCheckValueFitInType() const { return TRUE; }

  virtual double getNormalizedValue(void* buf) const { return *(double *)buf; }

private:
  
}; // class SQLFloat

// ***********************************************************************
//
//  SQLReal : SQL REAL
//
// ***********************************************************************
class SQLReal : public SQLFloat
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
   SQLReal (NAMemory *heap, NABoolean allowSQLnull = TRUE,
		  Lng32 precision = 0)
  : SQLFloat(heap, allowSQLnull, SQL_REAL_SIZE, 0, LiteralReal)
  {}
 
  // The above constructor can mislead callers of getPrecision() into
  // thinking type REAL has zero precision. The true precision of type
  // REAL is SQL_REAL_PRECISION.
  virtual Lng32 getTruePrecision() const { return SQL_REAL_PRECISION; }  

   short getFSDatatype() const { return REC_FLOAT32; }

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const 
    { return new(h) SQLReal(h, supportsSQLnull()); }

  virtual double getMaxValue() const { return 3.40282347e+38; }

  virtual double getNormalizedValue(void* buf) const { return *(float *)buf; }

private:
  
}; // class SQLReal

// ***********************************************************************
//
//  SQLDoublePrecision : SQL DOUBLE PRECISION
//
// ***********************************************************************
class SQLDoublePrecision : public SQLFloat
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
   SQLDoublePrecision (NAMemory *heap, NABoolean allowSQLnull = TRUE,
                       Lng32 precision = SQL_DOUBLE_PRECISION,
                       NABoolean fromFloat = FALSE)
  : SQLFloat(heap, allowSQLnull, SQL_DOUBLE_PRECISION_SIZE, 
	     precision, LiteralDoublePrecision),
    fromFloat_(fromFloat),
    origPrecision_(precision)
  {}

  // sqlparser.y calls the above constructor with zero precision. This can
  // mislead callers of getPrecision() into thinking type DOUBLE PRECISION
  // has zero precision. The true precision of type DOUBLE PRECISION is
  // SQL_DOUBLE_PRECISION.
  virtual Lng32 getTruePrecision() const { return SQL_DOUBLE_PRECISION; }  

   short getFSDatatype() const { return REC_FLOAT64; }

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const
  {
    return new(h) SQLDoublePrecision(h, supportsSQLnull(),getPrecision(), fromFloat());
  }

  // ---------------------------------------------------------------------
  // Method for comparing if two ADT definitions are equal.
  // ---------------------------------------------------------------------
  virtual NABoolean operator==(const NAType& other) const
  {
  if (NAType::operator==(other))
    return TRUE;
  else
    return FALSE;
  } // operator==()

  const NABoolean fromFloat() const { return fromFloat_;}
  const Lng32 origPrecision() const { return origPrecision_;}
private:
  // if this type was created through the use of FLOAT keyword and not
  // DOUBLE or DOUBLE PRECISION.
  NABoolean fromFloat_;
  Lng32 origPrecision_;
}; // class SQLDoublePrecision


#endif /* NUMERICTYPE_H */
















