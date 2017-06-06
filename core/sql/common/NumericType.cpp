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
/* -*-C++-*-
**************************************************************************
*
* File:         NumericType.cpp
* Description:  Numeric Type Implementation
* Created:      4/27/94
* Modified:     $Date: 2006/11/21 05:53:06 $
* Language:     C++
* Status:       Experimental
*
*
**************************************************************************
*/

#include "Platform.h"
#include "NumericType.h"
#include "Int64.h"
#include "float.h"
#include "str.h"
#include "exp_clause_derived.h"


#define NAME_BUF_LEN 100

NAString LiteralInteger("INTEGER");
NAString LiteralTinyInt("TINYINT");
NAString LiteralSmallInt("SMALLINT");
NAString LiteralBPInt("BIT PRECISION INTEGER");
NAString LiteralLargeInt("LARGEINT");
NAString LiteralNumeric("NUMERIC");
NAString LiteralDecimal("DECIMAL");
NAString LiteralBigNum("BIG NUM");
NAString LiteralLSDecimal("LSDECIMAL");
NAString LiteralFloat("FLOAT");
NAString LiteralReal("REAL");
NAString LiteralDoublePrecision("DOUBLE PRECISION");


static void unsignedLongToAscii(ULng32 number, char * asciiString,
				NABoolean prefixWithAMinus = FALSE)
{
  Lng32 index = 0;

  do                  // generate digits in the reverse order
    {
      asciiString[index++] = (char)(number % 10 + (Lng32)'0'); // get next digit
    } while ( (number /= 10) > 0);

  if (prefixWithAMinus)
    asciiString[index++] = '-';

  asciiString[index] = '\0';

  // reverse the string in place
  char temp;
  Int32 i,j;

#pragma nowarn(1506)   // warning elimination
  for (i = 0, j = strlen(asciiString)-1; i < j; i++, j--)
#pragma warn(1506)  // warning elimination
    {
      temp = asciiString[i];
      asciiString[i] = asciiString[j];
      asciiString[j] = temp;
    }

} // unsignedLongToAscii()

static void signedLongToAscii(Lng32 number, char * asciiString)
{
  Lng32 sign = number;
  if (sign < 0)       // record sign
    number = -number; // make it a positive number

  unsignedLongToAscii((ULng32)number, asciiString, (sign < 0));
}  // signedLongToAscii()

// inserts the scale indicator dot ('.') in str
static void insertScaleIndicator(NAString * str, Int32 scale)
{
  if (scale > 0)
    str->insert(str->length() - scale, ".");
}

// -----------------------------------------------------------------------
// utility functions
// -----------------------------------------------------------------------

unsigned short getBinaryStorageSize(Lng32 precision)
{
  if (precision < 1 || precision > 19) {
    return 0;
  }
  if (precision < 3)
    return SQL_TINY_SIZE;
  if (precision < 5)
    return SQL_SMALL_SIZE;
  if (precision < 10)
    return SQL_INT_SIZE;
  return SQL_LARGE_SIZE;
}

// -----------------------------------------------------------------------
// Data type name to internal type qualifier mapping function
// -----------------------------------------------------------------------

enum NumericType::NumericTypeEnum
     NumericType::tokenizeTypeName(const NAString& adtName) const
{
  NumericTypeEnum  token = MIN_NUMERIC_TYPE;

  if (adtName == "NUMERIC")
    {
      token = SQLNumeric_TYPE;
    }
  else if (adtName == "BIT PRECISION INTEGER")
    {
      token = SQLBPInt_TYPE;
    }
  else if (adtName == "DECIMAL")
    {
      token = SQLDecimal_TYPE;
    }
  else if (adtName == "BIG NUM")
    {
      token = SQLBigNum_TYPE;
    }
  else if (adtName == "LSDECIMAL")
    {
      token = LSDecimal_TYPE;
    }
  else if (adtName == "LARGEINT")
    {
      token = SQLLarge_TYPE;
    }
  else if (adtName == "INTEGER")
    {
      token = SQLInt_TYPE;
    }
  else if (adtName == "SMALLINT")
    {
      token = SQLSmall_TYPE;
    }
  else if (adtName == "TINYINT")
    {
      token = SQLTiny_TYPE;
    }
  else if (adtName == "FLOAT")
    {
      token = SQLFloat_TYPE;
    }
  else if (adtName == "REAL")
    {
      token = SQLReal_TYPE;
    }
  else if (adtName ==  "DOUBLE PRECISION")
    {
      token = SQLDoublePrecision_TYPE;
    }
  else // issue an error
  {
    NAString temp = "INTEGER";
    assert(adtName == temp);
  }

  return token;

} // tokenizeTypeName()

// ***********************************************************************
// Ugly data type conversion stuff
// ***********************************************************************
// ***********************************************************************
// NumericType
// ***********************************************************************
// -----------------------------------------------------------------------
// The constructor
// -----------------------------------------------------------------------

NumericType::NumericType( const NAString&  adtName,
			 Lng32             dataStorageSize,
			 Lng32             precision,
			 Lng32             scale,
			 Lng32             alignment,
			 NABoolean        allowNegValues,
			 NABoolean        allowSQLnull,
			 NABoolean        varLenFlag,
			 CollHeap * heap
			)
		  : NAType(adtName, NA_NUMERIC_TYPE, dataStorageSize,
			   allowSQLnull, SQL_NULL_HDR_SIZE
			   , varLenFlag
			   , ( varLenFlag ? SQL_VARCHAR_HDR_SIZE : 0 )
			   , alignment
			   , heap
			   )
{
  assert (scale <= precision);
  qualifier_ = tokenizeTypeName(adtName);
  precision_ = precision;
  scale_ = scale;
  unsigned_ = !(allowNegValues);
} // NumericType() default

NumericType::NumericType(const NumericType & numeric, CollHeap * heap)
: NAType(numeric,heap)
{

  qualifier_ = numeric.qualifier_;
  precision_ = numeric.precision_;
  scale_ = numeric.scale_;
  unsigned_ = numeric.unsigned_;
  assert (scale_ <= precision_);
}

// -----------------------------------------------------------------------
// Equality comparison
// -----------------------------------------------------------------------

NABoolean NumericType::operator==(const NAType& other) const
{
  if (NAType::operator==(other))
    {
      if (qualifier_ == ((NumericType&)other).qualifier_ &&
	  precision_ == ((NumericType&)other).precision_ &&
	  scale_ == ((NumericType&)other).scale_ &&
	  unsigned_ == ((NumericType&)other).unsigned_)
	return TRUE;
      else
	return FALSE;
    }
  else
    return FALSE;

} // operator==()

NABoolean NumericType::equalIgnoreNull(const NAType& other) const
{
  if (NAType::equalIgnoreNull(other))
    {
      if (qualifier_ == ((NumericType&)other).qualifier_ &&
	  precision_ == ((NumericType&)other).precision_ &&
	  scale_ == ((NumericType&)other).scale_ &&
	  unsigned_ == ((NumericType&)other).unsigned_)
	return TRUE;
      else
	return FALSE;
    }
  else
    return FALSE;
}

// -----------------------------------------------------------------------
// A method which tells if a conversion error can occur when converting
// a value of this type to the target type.
// -----------------------------------------------------------------------

NABoolean NumericType::errorsCanOccur (const NAType& target,
				       NABoolean lax) const
{
  NABoolean rc = TRUE;              //  assume the worst

  if (target.getTypeQualifier() == NA_NUMERIC_TYPE) {
    const NumericType &numericTarget = (const NumericType &)target;

    if (*this == numericTarget) {
      rc = FALSE;   //  no error can occur if datatypes are the same
    }
    else {
      if (!NAType::errorsCanOccur(target)) {
        if (isExact()) {
          if ((unsigned_) || (!numericTarget.unsigned_)) {
            if (numericTarget.isExact()) {
              // Source and target are exact.
              // If the magnitude and scale of the target are greater
              // than or equal to the source, then no conversion
              // error can occur
              if (getMagnitude() <= numericTarget.getMagnitude()) {
                if (scale_ == numericTarget.scale_) {
                  rc = FALSE;
                } 
                else if (lax==FALSE && scale_<=numericTarget.scale_) {
                  rc = FALSE;
                }
              }
            }
            else { // Source is exact, target is approximate.
              // if both are binary precision or decimal precision
              // and the precision of the target is greater than
              // or equal to the source, then no conversion error
              // can occur
              if (getTrueBinaryPrecision() <=
                  numericTarget.getTrueBinaryPrecision())
                rc = FALSE;
            }
          }
        }
        else { // Source is approximate.
          if (!numericTarget.isExact()) { // Both are approximate.
            //  both are approximate; if they are both binary precision
            //  (which today always happens to be true) and if the
            //  target precision is greater than or equal to the source
            //  then no conversion error can occur
            if (getTrueBinaryPrecision() <=
                numericTarget.getTrueBinaryPrecision())
              rc = FALSE;
          }
        }
      }
    }
  }
  return rc;
}

// -----------------------------------------------------------------------
// Internal type qualifier to data type name mapping function
// This is needed for supporting the SQL DESCRIBE command.
// -----------------------------------------------------------------------

NAString NumericType::getTypeName(NumericTypeEnum ntev) const
{
  NAString adtName;

  switch (ntev)
    {
    case SQLBPInt_TYPE :
      adtName = "BIT PRECISION INTEGER";
      break;
    case SQLTiny_TYPE :
      adtName = "TINYINT";
      break;
    case SQLSmall_TYPE :
      adtName = "SMALLINT";
      break;
    case SQLInt_TYPE :
      adtName = "INTEGER";
      break;
    case SQLLarge_TYPE :
      adtName = "LARGEINT";
      break;
    case SQLNumeric_TYPE :
      adtName = "NUMERIC";
      break;
    case SQLDecimal_TYPE :
      adtName = "DECIMAL";
      break;
    case SQLBigNum_TYPE :
      adtName = "BIG NUM";
      break;
    case LSDecimal_TYPE :
      adtName = "LSDECIMAL";
      break;
    case SQLFloat_TYPE :
      adtName = "FLOAT";
      break;
    case SQLReal_TYPE :
      adtName = "REAL";
      break;
    case SQLDoublePrecision_TYPE :
      adtName = "DOUBLE PRECISION";
      break;
    default :
      assert(0 == 1);  // ****ERROR: data type not supported
      break;
    } // switch (ntevev)

  return adtName;

} // getTypeName()

NAString NumericType::getTypeSQLname(NABoolean terse) const
{
  NAString rName = getTypeName(qualifier_);

  switch (qualifier_) {
  case SQLBigNum_TYPE :
    rName = "NUMERIC";
    if (getScale() > 0) {
      char precision[20];
      sprintf(precision, "(%d,%d)", getPrecision(), getScale());
      rName += precision;
    } else {
      char precision[20];
      sprintf(precision, "(%d)", getPrecision());
      rName += precision;
    }
    break;
  case SQLNumeric_TYPE :
  case SQLDecimal_TYPE :
  case LSDecimal_TYPE :
  case SQLFloat_TYPE :
  case SQLBPInt_TYPE :
    if (getScale() > 0) {
      char precision[20];
      sprintf(precision, "(%d,%d)", getPrecision(), getScale());
      rName += precision;
    } else {
      char precision[20];
      sprintf(precision, "(%d)", getPrecision());
      rName += precision;
    }
    break;
  }

  if ( supportsSign ())
  {
    if (unsigned_)
      rName += " UNSIGNED";
    else
      rName += " SIGNED";
  }

  getTypeSQLnull(rName, terse);

  return rName;

} // getTypeSQLname()

// -----------------------------------------------------------------------
// Table of standard precisions
// NOTE: The following table implements the binary precision for the
//       NonStop SQL data types
// -----------------------------------------------------------------------

Lng32 NumericType::getBinaryPrecision() const
{
  //
  // If the numeric type has binary precision, return the precision.
  // If the numeric type has decimal precision, compute the binary precision.
  //
  if (binaryPrecision())
    return getPrecision();
  assert(decimalPrecision());
  Lng32 decimalPrec = getPrecision();
  if (decimalPrec > 18)
    return BINARY64_PRECISION;
  assert (decimalPrec > 0);
  static const Lng32 binaryPrec[18] = {
     4,  7, 10, 14, 17, 20, 24, 27, 30,
    34, 37, 40, 44, 47, 50, 54, 57, 60
  };
  return binaryPrec[decimalPrec - 1] + 1 /*for sign*/;
} // getBinaryPrecision()

Lng32 NumericType::getTrueBinaryPrecision() const
{
  // We're assuming binaryPrecision() returns true for the anomalous cases
  // that return zero-precision described in NumericType.h
  if (binaryPrecision())
    return getTruePrecision();
  else
    return getBinaryPrecision();
} // getTrueBinaryPrecision()

// -----------------------------------------------------------------------
// Type synthesis for binary operators
// -----------------------------------------------------------------------

const NAType* NumericType::synthesizeType(enum NATypeSynthRuleEnum synthRule,
					  const NAType& operand1,
					  const NAType& operand2,
					  CollHeap* h,
					  UInt32 *flags) const
{
  //
  // If the second operand's type synthesis rules have higher precedence than
  // this operand's rules, use the second operand's rules.
  //
  if (operand2.getSynthesisPrecedence() > getSynthesisPrecedence())
    return operand2.synthesizeType(synthRule, operand1, operand2, h, flags);
  //
  // If either operand is not numeric, the expression is invalid.
  //
  if ((operand1.getTypeQualifier() != NA_NUMERIC_TYPE) ||
      (operand2.getTypeQualifier() != NA_NUMERIC_TYPE))
    return NULL;
  const NumericType& op1 = (NumericType &) operand1;
  const NumericType& op2 = (NumericType &) operand2;
  //
  // If either operand is signed, the result is signed.
  //
  NABoolean isSigned = op1.isSigned() OR op2.isSigned();
  //
  // If either operand is nullable, the result is nullable.
  //
  NABoolean isNullable = op1.supportsSQLnull() OR op2.supportsSQLnull();

  NABoolean isRealBigNum = 
    ((op1.isBigNum() && ((SQLBigNum&)op1).isARealBigNum()) ||
     (op2.isBigNum() && ((SQLBigNum&)op2).isARealBigNum()));

  //
  // By default, the result is not decimal.
  //
  NABoolean isDecimal = FALSE;
  //
  // Compute the scale and precision of the result.
  //
  Lng32 scale;
  Lng32 precision;

  NABoolean modeSpecial1 = 
    ((flags) && ((*flags & NAType::MODE_SPECIAL_1) != 0));
  NABoolean limitPrecision =
    ((flags) && ((*flags & NAType::LIMIT_MAX_NUMERIC_PRECISION) != 0));
  NABoolean makeUnionResultBinary =
    ((flags) && ((*flags & NAType::MAKE_UNION_RESULT_BINARY) != 0));

  NABoolean makeLargeint = FALSE;

  switch (synthRule) {
  case SYNTH_RULE_UNION: {
    //
    // Compute the magnitude of the result.  Magnitude is scaled up by a factor
    // of 10, because integers do not have exact decimal precision.  For
    // example, a SMALLINT has a precision of between 4 and 5 digits, so its
    // magnitude is 45.  A NUMERIC(5) has a precision of 5 digits so its
    // magnitude is 50.
    //
    Lng32 magnitude = MAXOF(op1.getMagnitude(), op2.getMagnitude());
    //
    // If one of the operands is an unsigned integer (e.g. SMALLINT UNSIGNED or
    // INTEGER UNSIGNED) and the other is a signed number that has a smaller or
    // equal magnitude, then the magnitude of the result must be rounded up to
    // hold the range of both operands.  For example, if the operands are
    // SMALLINT and SMALLINT UNSIGNED, the result will be NUMERIC(5).
    //
    if ((op1.isAnyUnsignedInt() AND op2.isSigned() AND
         (op1.getMagnitude() >= op2.getMagnitude())) OR
        (op2.isAnyUnsignedInt() AND op1.isSigned() AND
         (op2.getMagnitude() >= op1.getMagnitude())))
      magnitude += 10 - (magnitude % 10);
    //
    // Compute the scale of the result.
    //
    scale = MAXOF(op1.getScale(), op2.getScale());
    //
    // If the result has a scale and the magnitude is approximate, round up the
    // magnitude.
    //
    if ((scale > 0) AND ((magnitude % 10) > 0))
      magnitude += 10 - (magnitude % 10);
    //
    // Compute the precision of the result.
    //
    precision = magnitude / 10 + scale;
    //
    // If the magnitude is approximate, the result is an integer.
    //
    if (((magnitude % 10) > 0) ||
	(makeUnionResultBinary)) {
      Lng32 size = getBinaryStorageSize(precision);

      // convert tinyint to smallint.
      if (size == SQL_TINY_SIZE)
        size = SQL_SMALL_SIZE;

      // for now, make result to be an int32 or int64.
      if ((makeUnionResultBinary) &&
	  (size == SQL_SMALL_SIZE))
	size = SQL_INT_SIZE;

      switch (size) {
      case SQL_SMALL_SIZE:
        return new(h) SQLSmall(isSigned, isNullable);
      case SQL_INT_SIZE:
        return new(h) SQLInt(isSigned, isNullable);
      case SQL_LARGE_SIZE:
        return new(h) SQLLargeInt(isSigned, isNullable);
      default:
        return NULL;
      }
    }
    else if (limitPrecision)
      {
	if (precision > MAX_NUMERIC_PRECISION)
	  {
	    precision = MAX_NUMERIC_PRECISION;
	    makeLargeint = TRUE;
	  }
      }

    //
    // If both operands are DECIMAL, the result is DECIMAL.
    //
    isDecimal = op1.isDecimal() AND op2.isDecimal();
  
    break;
  }
  case SYNTH_RULE_ADD:
  case SYNTH_RULE_SUB: {
    Lng32 magnitude = MAXOF(op1.getMagnitude(), op2.getMagnitude());
    scale = MAXOF(op1.getScale(), op2.getScale());
    precision = (magnitude + 9) / 10 + scale + 1;
    if (limitPrecision)
      {
	if (precision > MAX_NUMERIC_PRECISION)
	  {
	    precision = MAX_NUMERIC_PRECISION;
	    makeLargeint = TRUE;
	  }
      }

    // result of a subtraction can be negative even if the operands
    // are unsigned. Make the result signed.
    if (synthRule == SYNTH_RULE_SUB)
      isSigned = TRUE;
    break;
  }
  case SYNTH_RULE_MUL:
    scale = op1.getScale() + op2.getScale();
    precision = (op1.getMagnitude() + 9) / 10 +
                (op2.getMagnitude() + 9) / 10 +
                scale;
    if (limitPrecision)
      {
	if ((modeSpecial1) &&
	    (scale > MAX_NUMERIC_PRECISION))
	  // scale overflow, return error.
	  return NULL;

	if ((scale <= MAX_NUMERIC_PRECISION) &&
	    (precision > MAX_NUMERIC_PRECISION))
	  {
	    precision = MAX_NUMERIC_PRECISION;
	    makeLargeint = TRUE;
	  }
      }
    break;
  case SYNTH_RULE_DIV:
    {
      NABoolean roundingRequested = 
	((flags) && ((*flags & NAType::ROUND_RESULT) != 0));
      NABoolean roundingDone = FALSE;

      if (limitPrecision)
	{
	  scale = MAXOF(op1.getScale(), op2.getScale());
	  precision = (op1.getMagnitude() + 9) / 10 +
	    (op2.getMagnitude() + 9) / 10 +
	    scale;
	}
      else
	{
	  scale = (op2.getMagnitude() + 9) / 10 + op1.getScale();
	  precision = (op1.getMagnitude() + 9) / 10 + op2.getScale() + scale;
	}
      if (limitPrecision)
	{
	  //if (precision > MAX_NUMERIC_PRECISION)
	  // {
	      precision = MAX_NUMERIC_PRECISION ;
	      makeLargeint = TRUE;
	  //  }
	}

      if (roundingRequested && 
	  (precision <= MAX_NUMERIC_PRECISION))
	{
	  roundingDone = TRUE;
	}
      
      if ((flags) && (roundingRequested))
	{
	  if (NOT roundingDone)
	    {
	      // rounding was requested but not done.
	      // Reset the ROUND bit in flags so caller could get this
	      // information.
	      *flags &= ~NAType::RESULT_ROUNDED;
	    }
	  else
	    {
	      *flags |= NAType::RESULT_ROUNDED;
	    }
	}
    }
    break;
  case SYNTH_RULE_EXP:
    precision = MAX_NUMERIC_PRECISION;
    scale = 6;
    isSigned = op1.isSigned();
    break;
  default:
    return NULL;
  }

  if (precision > MAX_NUMERIC_PRECISION) {
    //
    // If the hardware doesn't support a binary numeric of the result's
    // precision, make the result a Big Num.
    //
    return new(h) SQLBigNum(precision,
			    scale,
			    isRealBigNum,
			    isSigned,
			    isNullable,
			    NULL);
  }
  //
  // If the result is DECIMAL, return a DECIMAL.
  //
  if (isDecimal)
    return new(h) SQLDecimal(precision, scale, isSigned, isNullable);
  //
  // If the precision is more than 9, it must be signed.
  //
  if (precision > 9)
    isSigned = TRUE;
  //
  // Compute the storage size of the binary result.
  //
  Lng32 size = getBinaryStorageSize(precision);

  // convert tinyint to smallint for arithmetic results.
  if (size == SQL_TINY_SIZE)
    size = SQL_SMALL_SIZE;

  //
  // The result is NUMERIC.
  //
  if (makeLargeint)
    {
      NumericType * nat = new(h) SQLLargeInt(isSigned, isNullable);
      nat->setScale(scale);
      return nat;
    }
  else
    return new(h) SQLNumeric(size, precision, scale, isSigned, isNullable);
}

// -----------------------------------------------------------------------
// Min and Max permissible values.
// -----------------------------------------------------------------------
void NumericType::minRepresentableValue(void*, Lng32*, NAString**,
					CollHeap* h) const {}
void NumericType::maxRepresentableValue(void*, Lng32*, NAString**,
					CollHeap* h) const {}

NABoolean NumericType::createSQLLiteral(const char * buf,
                                        NAString *&stringLiteral,
                                        NABoolean &isNull,
                                        CollHeap *h) const
{
  if (NAType::createSQLLiteral(buf, stringLiteral, isNull, h))
    return TRUE;

  // For all the numeric types, just converting the number to a string value should
  // produce a valid SQL literal.
  const int RESULT_LEN = 100;
  char result[RESULT_LEN];
  const char *valPtr = buf + getSQLnullHdrSize();
  unsigned short resultLen = 0;
  ComDiagsArea *diags = NULL;

  ex_expr::exp_return_type ok =
    convDoIt((char *) valPtr,
             getTotalSize(),
             getFSDatatype(),
             getPrecision(),
             getScale(),
             result,
             RESULT_LEN,
             REC_BYTE_V_ASCII,
             0,
             SQLCHARSETCODE_UTF8,
             (char *) &resultLen,
             sizeof(resultLen),
             h,
             &diags,
             ConvInstruction::CONV_UNKNOWN,
             0);

   if ( ok != ex_expr::EXPR_OK || resultLen == 0)
     return FALSE;

   stringLiteral = new(h) NAString(result, resultLen, h);
   return TRUE;
}

// -----------------------------------------------------------------------
// Print function for debugging
// -----------------------------------------------------------------------
void NumericType::print(FILE* ofd, const char* indent)
{
#ifdef TRACING_ENABLED  // NT_PORT ( bd 8/4/96 )
  fprintf(ofd,"%s %s\n",indent,getTypeSQLname());
#endif
} // NumericType::print()

// -----------------------------------------------------------------------
// A method for generating the hash key.
// SQL builtin types should return getTypeSQLName()
// -----------------------------------------------------------------------
NAString* NumericType::getKey(CollHeap* h) const
{
  return new (h) NAString(getTypeSQLname(), h);
} // NumericType::getKey()

short NumericType::getFSDatatype() const
{
  return -1;
}

double NumericType::encode (void *) const
{
  return -1;
}

NABoolean NumericType::isEncodingNeeded() const
{
#if defined( NA_LITTLE_ENDIAN )
   return TRUE;
#else
   if (isBigNum()) // bignums always need to be encoded
     return TRUE;
   else if (isSigned())
     return TRUE;
   else
     return FALSE;
#endif
}

// -----------------------------------------------------------------------
//  Methods for SQLTiny
// -----------------------------------------------------------------------

SQLTiny::SQLTiny(NABoolean allowNegValues, NABoolean allowSQLnull,CollHeap * heap)
     : NumericType
       ( LiteralTinyInt
         , SQL_TINY_SIZE
         , (allowNegValues ? SQL_SMALL_PRECISION:SQL_USMALL_PRECISION)
         , 0
         , 2
         , allowNegValues
         , allowSQLnull
         ,FALSE
         ,heap
         )
{
} // SQLTiny()

double SQLTiny::encode (void* bufPtr) const
{
  Int8 tempValue;
  UInt8 usTempValue;
  char * valPtr = (char *)bufPtr;
  if (supportsSQLnull())
    valPtr += getSQLnullHdrSize();

  if(isUnsigned())
  {
    str_cpy_all ((char *)&usTempValue, valPtr, getNominalSize());
    return ((double)usTempValue * pow(10.0, -1 * getScale()));
  }
  else
  {
    str_cpy_all ((char *)&tempValue, valPtr, getNominalSize());
    return ((double)tempValue * pow(10.0, -1 * getScale()));
  }
}

// -- Min and max permissible values

void SQLTiny::minRepresentableValue(void* bufPtr, Lng32* bufLen,
                                     NAString ** stringLiteral,
				     CollHeap* h) const
{
  assert(*bufLen >= sizeof(char));
  Lng32 valueBuf;
  *bufLen = sizeof(char);
  if (NumericType::isUnsigned())
    {
      *((char*)bufPtr) = 0;
      valueBuf = 0;
    }
  else
    {
      char temp = CHAR_MIN;
      for (Lng32 i = 0; i < sizeof(char); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      valueBuf = CHAR_MIN;
    }

  if (stringLiteral != NULL)
    {
      // Generate a printable string for the minimum value
      char nameBuf[NAME_BUF_LEN]; // 2 ** 16 == 65536. Need space for 5 digits only
      signedLongToAscii(valueBuf, nameBuf);
      *stringLiteral = new (h) NAString(nameBuf, h);
    }

} // SQLTiny::minRepresentableValue()

void SQLTiny::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
                                     NAString ** stringLiteral,
				     CollHeap* h) const
{
  assert(*bufLen >= sizeof(char));
  Lng32 valueBuf;
  *bufLen = sizeof(char);
  if (NumericType::isUnsigned())
    {
      unsigned short temp = UCHAR_MAX;
      for (Lng32 i = 0; i < sizeof(char); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      valueBuf = UCHAR_MAX;
    }
  else
    {
      short temp = CHAR_MAX;
      for (Lng32 i = 0; i < sizeof(char); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      valueBuf = CHAR_MAX;
    }

  if (stringLiteral != NULL)
    {
      // Generate a printable string for the maximum value
      char nameBuf[NAME_BUF_LEN]; // 2 ** 16 == 65536. Need space for 5 digits only
      signedLongToAscii(valueBuf, nameBuf);
      *stringLiteral = new (h) NAString(nameBuf, h);
    }

} // SQLTiny::maxRepresentableValue()

NAString* SQLTiny::convertToString(double v, CollHeap* h) const
{
   Lng32 valueBuf = (Lng32)v;

   char nameBuf[NAME_BUF_LEN]; // 2 ** 16 == 65536. Need space for 5 digits only
   signedLongToAscii(valueBuf, nameBuf);
   return new (h) NAString(nameBuf, h);
}

// -----------------------------------------------------------------------
//  Methods for SQLSmall
// -----------------------------------------------------------------------

SQLSmall::SQLSmall(NABoolean allowNegValues, NABoolean allowSQLnull,CollHeap * heap)
        : NumericType
            ( LiteralSmallInt
	    , SQL_SMALL_SIZE
            , (allowNegValues ? SQL_SMALL_PRECISION:SQL_USMALL_PRECISION)
	    , 0
	    , 2
	    , allowNegValues
            , allowSQLnull
			,FALSE
			,heap
            )
{
} // SQLSmall()

double SQLSmall::encode (void* bufPtr) const
{
  short tempValue;
  unsigned short usTempValue;
  char * valPtr = (char *)bufPtr;
  if (supportsSQLnull())
    valPtr += getSQLnullHdrSize();

  if(isUnsigned())
  {
    str_cpy_all ((char *)&usTempValue, valPtr, getNominalSize());
    return ((double)usTempValue * pow(10.0, -1 * getScale()));
  }
  else
  {
    str_cpy_all ((char *)&tempValue, valPtr, getNominalSize());
    return ((double)tempValue * pow(10.0, -1 * getScale()));
  }
}

// -- Min and max permissible values

void SQLSmall::minRepresentableValue(void* bufPtr, Lng32* bufLen,
                                     NAString ** stringLiteral,
				     CollHeap* h) const
{
  assert(*bufLen >= sizeof(short));
  Lng32 valueBuf;
  *bufLen = sizeof(short);
  if (NumericType::isUnsigned())
    {
      *((short*)bufPtr) = 0;
      valueBuf = 0;
    }
  else
    {
      short temp = SHRT_MIN;
      for (Lng32 i = 0; i < sizeof(short); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      valueBuf = SHRT_MIN;
    }

  if (stringLiteral != NULL)
    {
      // Generate a printable string for the minimum value
      char nameBuf[NAME_BUF_LEN]; // 2 ** 16 == 65536. Need space for 5 digits only
      signedLongToAscii(valueBuf, nameBuf);
      *stringLiteral = new (h) NAString(nameBuf, h);
    }

} // SQLSmall::minRepresentableValue()

void SQLSmall::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
                                     NAString ** stringLiteral,
				     CollHeap* h) const
{
  assert(*bufLen >= sizeof(short));
  Lng32 valueBuf;
  *bufLen = sizeof(short);
  if (NumericType::isUnsigned())
    {
      unsigned short temp = USHRT_MAX;
      for (Lng32 i = 0; i < sizeof(short); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      valueBuf = USHRT_MAX;
    }
  else
    {
      short temp = SHRT_MAX;
      for (Lng32 i = 0; i < sizeof(short); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      valueBuf = SHRT_MAX;
    }

  if (stringLiteral != NULL)
    {
      // Generate a printable string for the maximum value
      char nameBuf[NAME_BUF_LEN]; // 2 ** 16 == 65536. Need space for 5 digits only
      signedLongToAscii(valueBuf, nameBuf);
      *stringLiteral = new (h) NAString(nameBuf, h);
    }

} // SQLSmall::maxRepresentableValue()

NAString* SQLSmall::convertToString(double v, CollHeap* h) const
{
   Lng32 valueBuf = (Lng32)v;

   char nameBuf[NAME_BUF_LEN]; // 2 ** 16 == 65536. Need space for 5 digits only
   signedLongToAscii(valueBuf, nameBuf);
   return new (h) NAString(nameBuf, h);
}

// -----------------------------------------------------------------------
//  Methods for SQLBPInt
// -----------------------------------------------------------------------

#pragma nowarn(1506)   // warning elimination
SQLBPInt::SQLBPInt(UInt32 declared,
		   NABoolean allowSQLnull,
		   NABoolean allowNegValues,
		   CollHeap * heap)
      : NumericType (LiteralBPInt	 // ADT Name
		   , SQL_SMALL_SIZE      // StorageSize
		   , declared	         // Precision
		   , 0		         // Scale
		   , 2                   // Alignment
		   , allowNegValues
		   , allowSQLnull
		   , FALSE
		   , heap
		    )
{
  assert (declared > 0 && declared < 16); // size between 1 & 15
  assert (allowNegValues == FALSE);
  declaredSize_ = declared;
} // SQLBPInt()
#pragma warn(1506)  // warning elimination

// Are two BPInt types equal if they have different storage sizes??
NABoolean SQLBPInt::operator==(const NAType& other) const
{
  if (NumericType::operator==((NumericType&)other))
  {
    if (declaredSize_ == ((SQLBPInt&)other).declaredSize_)
      return TRUE;
    else
      return FALSE;
  }
  else
    return FALSE;
} // operator==()

double SQLBPInt::encode (void *bufPtr ) const
{
  short tempValue;
  unsigned short usTempValue;
  char * valPtr = (char *)bufPtr;

  if (supportsSQLnull())
    valPtr += getSQLnullHdrSize();

  if(isUnsigned())
  {
    str_cpy_all ((char *)&usTempValue, valPtr, getNominalSize());
    return ((double)usTempValue * pow(10.0, -1 * getScale()));
  }
  else
  {
    str_cpy_all ((char *)&tempValue, valPtr, getNominalSize());
    return ((double)tempValue * pow(10.0, -1 * getScale()));
  }
}


void SQLBPInt::minRepresentableValue (void* bufPtr, Lng32* bufLen,
				      NAString ** stringLiteral,
				      CollHeap* h) const
{
  assert(*bufLen >= sizeof(short));
  Lng32 valueBuf;
  *bufLen = sizeof(short);
  // BPInt is unsigned, so minimum value is 0.
  *((short*)bufPtr) = 0;
  valueBuf = 0;

  if (stringLiteral != NULL)
  {
    // Generate a printable string for the minimum value
    char nameBuf[NAME_BUF_LEN];
    signedLongToAscii(valueBuf, nameBuf);
    *stringLiteral = new (h) NAString(nameBuf, h);
  }
}


void SQLBPInt::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
                                     NAString ** stringLiteral,
                                     CollHeap* h) const
{
  assert(*bufLen >= sizeof(short));
  Lng32 valueBuf;
  *bufLen = sizeof(short);
  static const unsigned short limits[] = {   1,    3,    7,    15,    31,
	                                    63,  127,  255,   511,  1023,
				          2047, 4095, 8191, 16383, 32767 };
  unsigned short temp = limits[declaredSize_ - 1];
  for (Lng32 i = 0; i < sizeof(short); i++)
  {
    ((char *)bufPtr)[i] = ((char *)&temp)[i];
  }
  valueBuf = temp;

  if (stringLiteral != NULL)
  {
    // Generate a printable string for the maximum value
    char nameBuf[NAME_BUF_LEN]; // 2 ** 15 = 32768. Need space for 5 digits only
    signedLongToAscii(valueBuf, nameBuf);
    *stringLiteral = new (h) NAString(nameBuf, h);
  }
}

NAString* SQLBPInt::convertToString(double v, CollHeap* h) const
{
  Lng32 temp = Lng32(v);

  char nameBuf[NAME_BUF_LEN]; 

  signedLongToAscii(temp, nameBuf);

  return new (h) NAString(nameBuf, h);
}

// -----------------------------------------------------------------------
//  Methods for SQLInt
// -----------------------------------------------------------------------

SQLInt::SQLInt(NABoolean allowNegValues, NABoolean allowSQLnull,CollHeap * heap)
      : NumericType
	  ( LiteralInteger
	  , SQL_INT_SIZE
          , (allowNegValues ? SQL_INT_PRECISION:SQL_UINT_PRECISION)
	  , 0
          , 4
	  , allowNegValues
          , allowSQLnull
		  ,FALSE
		  ,heap
          )
{
} // SQLInt()


double SQLInt::encode (void* bufPtr) const
{
  Lng32 tempValue;
  ULng32 usTempValue;
  char * valPtr = (char *)bufPtr;
  if (supportsSQLnull())
    valPtr += getSQLnullHdrSize();

  if(isUnsigned())
  {
    str_cpy_all ((char *)&usTempValue, valPtr, getNominalSize());
    return ((double)usTempValue * pow(10.0, -1 * getScale()));
  }
  else
  {
    str_cpy_all ((char *)&tempValue, valPtr, getNominalSize());
    return ((double)tempValue * pow(10.0, -1 * getScale()));
  }
}

void SQLInt::minRepresentableValue(void* bufPtr, Lng32* bufLen,
				   NAString ** stringLiteral,
				   CollHeap* h) const
{
  assert(*bufLen >= sizeof(Lng32));
  // To generate a printable string for the minimum value
  char nameBuf[NAME_BUF_LEN]; // 2 ** 32 == 4294967296. Need space for 10 digits only
  ULng32 temp;
  *bufLen = sizeof(Lng32);
  if (NumericType::isUnsigned())
    {
      temp = 0;
      for (Lng32 i = 0; i < sizeof(Lng32); i++)
	{
	  ((char *)bufPtr)[i] = 0;
	}
      if (stringLiteral != NULL) //only when need to return a string
	{
	  unsignedLongToAscii(temp, nameBuf);
	}
     }
  else
    {
      temp = INT_MIN;
      for (Lng32 i = 0; i < sizeof(Lng32); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      if (stringLiteral != NULL) //only when need to return a string
	{
           signedLongToAscii(temp, nameBuf);
	}
  }

  if (stringLiteral != NULL)
    *stringLiteral = new (h) NAString(nameBuf, h);

} // SQLInt::minRepresentableValue()

void SQLInt::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
				   NAString ** stringLiteral,
				   CollHeap* h) const
{
  assert(*bufLen >= sizeof(Lng32));
  // To generate a printable string for the minimum value
  char nameBuf[NAME_BUF_LEN]; // 2 ** 32 == 4294967296. Need space for 10 digits only
  *bufLen = sizeof(Lng32);
  if (NumericType::isUnsigned())
    {
      ULng32 temp = UINT_MAX;
      for (Lng32 i = 0; i < getNominalSize(); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      if (stringLiteral != NULL) //only when need to return a string
	{
	  unsignedLongToAscii(temp, nameBuf);
	}
    }
  else
    {
      Lng32 temp = INT_MAX;
      for (short i = 0; i < getNominalSize(); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      if (stringLiteral != NULL) //only when need to return a string
	{
          signedLongToAscii(temp, nameBuf);
	}
    }
  if (stringLiteral != NULL)
    *stringLiteral = new (h) NAString(nameBuf, h);
} // SQLInt::maxRepresentableValue()

NAString* SQLInt::convertToString(double v, CollHeap* h) const
{
  char nameBuf[NAME_BUF_LEN]; // 2 ** 32 == 4294967296. Need space for 10 digits only

  if (NumericType::isUnsigned()) {
     ULng32 temp = (ULng32)v;
     unsignedLongToAscii(temp, nameBuf);
  } else {
     Lng32 temp = (Lng32)v;
     signedLongToAscii(temp, nameBuf);
  }
  return new (h) NAString(nameBuf, h);
}

// -----------------------------------------------------------------------
//  Methods for SQLLargeInt
// -----------------------------------------------------------------------

SQLLargeInt::SQLLargeInt(NABoolean allowNegValues, NABoolean allowSQLnull,
			 CollHeap * heap)
      : NumericType
	  ( LiteralLargeInt
	  , 8/*SQL_L_INT_SIZE */
          , SQL_LARGE_PRECISION
	  , 0
          , 8
	  , allowNegValues
          , allowSQLnull
		  ,FALSE
		  ,heap
          )
{
}   // SQLLargeInt()

SQLLargeInt::SQLLargeInt(Lng32 scale,
			 UInt16 disAmbiguate,
			 NABoolean allowNegValues, NABoolean allowSQLnull,
			 CollHeap * heap)
      : NumericType
	  ( LiteralLargeInt
	  , 8/*SQL_L_INT_SIZE */
          , SQL_LARGE_PRECISION
	  , scale
          , 8
	  , allowNegValues
          , allowSQLnull
		  ,FALSE
		  ,heap
          )
{
}   // SQLLargeInt()


double SQLLargeInt::encode (void* bufPtr) const
{
  Int64 tempValue;
  UInt64 usTempValue;

  char * valPtr = (char *)bufPtr;
  if (supportsSQLnull())
    valPtr += getSQLnullHdrSize();

  if (isUnsigned())
  {
    str_cpy_all ((char *)&usTempValue, valPtr, getNominalSize());
    return (convertUInt64ToDouble(tempValue) * pow(10.0, -1 * getScale()));
  }
  else
  {
    str_cpy_all ((char *)&tempValue, valPtr, getNominalSize());
    return (convertInt64ToDouble(tempValue) * pow(10.0, -1 * getScale()));
  }
}

void SQLLargeInt::minRepresentableValue(void* bufPtr, Lng32* bufLen,
                                        NAString ** stringLiteral,
                                        CollHeap* h) const
{
  assert(*bufLen >= sizeof(Int64));
  *bufLen = sizeof(Int64);
  char nameBuf[NAME_BUF_LEN];
  if (NumericType::isUnsigned())
    {
      UInt64 temp = 0;
      for (Lng32 i = 0; i < sizeof(UInt64); i++)
	{
	  ((char *)bufPtr)[i] = 0;
	}
      if (stringLiteral != NULL) //only when need to return a string
	{
	  convertUInt64ToAscii(temp, nameBuf);
	}
     }
  else
    {
      Int64 temp = LLONG_MIN;
      for (Lng32 i = 0; i < sizeof(Int64); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      if (stringLiteral != NULL) //only when need to return a string
	{
           convertInt64ToAscii(temp, nameBuf);
	}
  }

  if (stringLiteral != NULL)
    *stringLiteral = new (h) NAString(nameBuf, h);

} // SQLLargeInt::minRepresentableValue()

void SQLLargeInt::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
                                        NAString ** stringLiteral,
                                        CollHeap* h) const
{
  assert(*bufLen >= sizeof(Int64));
  char nameBuf[NAME_BUF_LEN];
  *bufLen = sizeof(Int64);
  if (NumericType::isUnsigned())
    {
      UInt64 temp = ULLONG_MAX;
      for (Lng32 i = 0; i < getNominalSize(); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      if (stringLiteral != NULL) //only when need to return a string
	{
	  convertUInt64ToAscii(temp, nameBuf);
	}
    }
  else
    {
      Int64 temp = LLONG_MAX;
      for (short i = 0; i < getNominalSize(); i++)
	{
	  ((char *)bufPtr)[i] = ((char *)&temp)[i];
	}
      if (stringLiteral != NULL) //only when need to return a string
	{
          convertInt64ToAscii(temp, nameBuf);
	}
    }
  if (stringLiteral != NULL)
    *stringLiteral = new (h) NAString(nameBuf, h);
} // SQLLargeInt::maxRepresentableValue()

NAString* SQLLargeInt::convertToString(double v, CollHeap* h) const
{
   char nameBuf[NAME_BUF_LEN];   // a reasonably large buffer

   if (NumericType::isUnsigned()) {
     UInt64 temp = (UInt64)v;
     convertUInt64ToAscii(temp, nameBuf);
   } else {
     Int64 temp = Int64(v);
     convertInt64ToAscii(temp, nameBuf);
   }

   return new (h) NAString(nameBuf, h);
}

// -----------------------------------------------------------------------
//  Methods for SQLBigInt
// -----------------------------------------------------------------------

SQLBigInt::SQLBigInt(NABoolean allowNegValues, NABoolean allowSQLnull,CollHeap * heap)
  : SQLLargeInt(allowNegValues, allowSQLnull,heap)
{
  setClientDataType("BIGINT");
}

// -----------------------------------------------------------------------
//  Methods for SQLNumeric
// -----------------------------------------------------------------------

SQLNumeric::SQLNumeric(Lng32 length, Lng32 precision, Lng32 scale,
		       NABoolean allowNegValues, NABoolean allowSQLnull,
			   CollHeap * heap)
: NumericType
  ( LiteralNumeric
   , length
   , precision
   , scale
   , length
   , allowNegValues
   , allowSQLnull
   ,FALSE
   ,heap
   )
{
} // SQLNumeric()


// Note: DisAmbiguate arg added so Compiler can distinguish between
//       this constructor and the one above...for 64bit project.
SQLNumeric::SQLNumeric(NABoolean allowNegValues, Lng32 precision, Lng32 scale,
                       const Int16 DisAmbiguate,
                       NABoolean allowSQLnull)
: NumericType
  ( LiteralNumeric
   , getBinaryStorageSize(precision)
   , precision
   , scale
   , getBinaryStorageSize(precision)
   , allowNegValues
   , allowSQLnull
   )
{
} // SQLNumeric()

double SQLNumeric::encode (void* bufPtr) const
{
  Lng32 longTemp;
  ULng32 usLongTemp;
  short shrtTemp;
  unsigned short usShrtTemp;
  Int8 charTemp;
  UInt8 usCharTemp;

  char * valPtr = (char *)bufPtr;
  if (supportsSQLnull())
    valPtr += getSQLnullHdrSize();

  if (getNominalSize() <= 1)
  {
    if (isUnsigned())
    {
      str_cpy_all ((char *)&usCharTemp, valPtr, getNominalSize());
      return ((double)usCharTemp * pow(10.0, -1 * getScale()));
    }
    else
    {
      str_cpy_all ((char *)&charTemp, valPtr, getNominalSize());
      return ((double)charTemp * pow(10.0, -1 * getScale()));
    }
  }
  else if (getNominalSize() <= 2)
  {
    if (isUnsigned())
    {
      str_cpy_all ((char *)&usShrtTemp, valPtr, getNominalSize());
      return ((double)usShrtTemp * pow(10.0, -1 * getScale()));
    }
    else
    {
      str_cpy_all ((char *)&shrtTemp, valPtr, getNominalSize());
      return ((double)shrtTemp * pow(10.0, -1 * getScale()));
    }
  }
  else if (getNominalSize() <= 4)
  {
    if (isUnsigned())
    {
      str_cpy_all ((char *)&usLongTemp, valPtr, getNominalSize());
      return ((double)usLongTemp * pow(10.0, -1 * getScale()));
    }
    else
    {
      str_cpy_all ((char *)&longTemp, valPtr, getNominalSize());
      return ((double)longTemp * pow(10.0, -1 * getScale()));
    }
  }
  else
  {
    Int64 int64Temp;
    str_cpy_all ((char *)&int64Temp, valPtr, getNominalSize());
    return (convertInt64ToDouble(int64Temp) * pow(10.0, -1 * getScale()));
  }
}

void SQLNumeric::minRepresentableValue(void* bufPtr, Lng32* bufLen,
				       NAString ** stringLiteral,
				       CollHeap* h) const
{
  assert(*bufLen >= getNominalSize());
  // To generate a printable string for the minimum value
  char nameBuf[NAME_BUF_LEN];  // a reasonably large buffer
  *bufLen = getNominalSize();

  if (NumericType::isUnsigned())
    {
      for (Lng32 i = 0; i < getNominalSize(); i++)
	{
	  ((char *)bufPtr)[i] = 0;
	}
      Lng32 temp = 0;
      unsignedLongToAscii(temp, nameBuf);

      if (stringLiteral != NULL)
	{
	  *stringLiteral = new (h) NAString(nameBuf, h);
	}
    }
  else
    {
      // signed numeric
      switch (getNominalSize())
        {
        case sizeof(Int64):
          {
            Int64 temp = 0;
            Lng32 i=0;
            for (; i<getPrecision(); i++)
              {
                temp = temp * 10 + 9;
              }
            temp = -temp;
            
            for (i = 0; i < getNominalSize(); i++)
              ((char *)bufPtr)[i] = ((char *)&temp)[i];
            convertInt64ToAscii(temp, nameBuf);
          }
          break;
          
        case sizeof(Int32):
          {
            Lng32 temp = 0;
            Lng32 i=0;
            for (; i<getPrecision(); i++)
              {
                temp = temp * 10 + 9;
              }
            temp = -temp;
            
            for (i = 0; i < getNominalSize(); i++)
              ((char *)bufPtr)[i] = ((char *)&temp)[i];
            signedLongToAscii(temp, nameBuf);
          }
          break;
        
        case sizeof(Int16):
          {
            short temp = 0;
            Lng32 i=0;
            for (; i<getPrecision(); i++)
              {
                temp = temp * 10 + 9;
              }
            temp = -temp;
            
            for (i = 0; i < getNominalSize(); i++)
              ((char *)bufPtr)[i] = ((char *)&temp)[i];
            signedLongToAscii((Lng32)temp, nameBuf);
          }
          break;

      case sizeof(Int8):
          {
            Int8 temp = 0;
            Lng32 i=0;
            for (; i<getPrecision(); i++)
              {
                temp = temp * 10 + 9;
              }
            temp = -temp;
            
            for (i = 0; i < getNominalSize(); i++)
              ((char *)bufPtr)[i] = ((char *)&temp)[i];
            signedLongToAscii((Lng32)temp, nameBuf);
          }
          break;

        } // switch
 
    }
  
  if (stringLiteral != NULL)
    {
      *stringLiteral = new (h) NAString(nameBuf, h);
      insertScaleIndicator(*stringLiteral, getScale());
    }
} // SQLNumeric::minRepresentableValue()

void SQLNumeric::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
				       NAString ** stringLiteral,
				       CollHeap* h) const
{
  assert(*bufLen >= getNominalSize());
  // To generate a printable string for the maximum value
  char nameBuf[NAME_BUF_LEN];  // a reasonably large buffer
  *bufLen = getNominalSize();

  switch (getNominalSize())
    {
    case sizeof(Int64):
      {
        Int64 temp = 0;
        Lng32 i=0;
        for (; i<getPrecision(); i++)
          {
            temp = temp * 10 + 9;
          }
        
        for (i = 0; i < getNominalSize(); i++)
          ((char *)bufPtr)[i] = ((char *)&temp)[i];
        convertInt64ToAscii(temp, nameBuf);
      }
      break;
      
    case sizeof(Int32):
      {
        Lng32 temp = 0;
        Lng32 i=0;
        for (; i<getPrecision(); i++)
          {
            temp = temp * 10 + 9;
          }
        
        for (i = 0; i < getNominalSize(); i++)
          ((char *)bufPtr)[i] = ((char *)&temp)[i];
        signedLongToAscii(temp, nameBuf);
      }
    break;
    
    case sizeof(Int16):
      {
        short temp = 0;
        Lng32 i=0;
        for (; i<getPrecision(); i++)
          {
#pragma nowarn(1506)   // warning elimination
            temp = temp * 10 + 9;
#pragma warn(1506)  // warning elimination
          }
        
        for (i = 0; i < getNominalSize(); i++)
          ((char *)bufPtr)[i] = ((char *)&temp)[i];
        signedLongToAscii((Lng32)temp, nameBuf);
      }
      break;

    case sizeof(Int8):
      {
        Int8 temp = 0;
        Lng32 i=0;
        for (; i<getPrecision(); i++)
          {
            temp = temp * 10 + 9;
          }
        
        for (i = 0; i < getNominalSize(); i++)
          ((char *)bufPtr)[i] = ((char *)&temp)[i];
        signedLongToAscii((Lng32)temp, nameBuf);
      }
      break;

    }

  if (stringLiteral != NULL)
    {
      *stringLiteral = new (h) NAString(nameBuf, h);
      insertScaleIndicator(*stringLiteral, getScale());
    }
} // SQLNumeric::maxRepresentableValue()


NAString* SQLNumeric::convertToString(double v, CollHeap* h)  const
{
  char nameBuf[NAME_BUF_LEN];  // a reasonably large buffer
  char fractionBuf[NAME_BUF_LEN];  // a reasonably large buffer

  {
     switch (getNominalSize())
     {
        case sizeof(Int64):
        {
	   Int64 temp = (Int64)v;
	   convertInt64ToAscii(temp, nameBuf);

           Int64 fraction = Int64(v - double(temp));
	   convertInt64ToAscii(temp, fractionBuf);
           fractionBuf[getScale()] = 0; 

        }
        break;

#ifdef NA_64BIT
        // dg64 - a bit of a guess
        case sizeof(Int32):
#else
        case sizeof(Lng32):
#endif
        {
	   Lng32 temp = (Lng32)v;
	   signedLongToAscii(temp, nameBuf);

           Lng32 fraction = Lng32(v - double(temp));
	   signedLongToAscii(temp, fractionBuf);
           fractionBuf[getScale()] = 0; 
        }
        break;

        case sizeof(short):
        {
	   short temp = (short)v;
	   signedLongToAscii((Lng32)temp, nameBuf);

           short fraction = short(v - double(temp));
	   signedLongToAscii(temp, fractionBuf);
           fractionBuf[getScale()] = 0; 
        }
        break;
     }
  }

  NAString* result = new (h) NAString(nameBuf, h);

  result->append('.');
  result->append(fractionBuf);

  return result;
} 

double SQLNumeric::getNormalizedValue(void* buf) const
{
  double val;
  switch (getNominalSize())
  {
    case sizeof(short):
    {
      if (isUnsigned())
        val = (double)(*(unsigned short *)buf);
      else
	val = (double)(*(short *)buf);
    }
    break;
    case sizeof(Lng32):
    {
      if (isUnsigned())
        val = (double)(*(ULng32 *)buf);
      else
	val = (double)(*(Lng32 *)buf);
    }
    break;
    case sizeof(Int64):
    {  
      val = (double)(* (Int64 *)buf);
    }
    break;
    default:
    {
      return -1; // invalid value
    }
  }
  return val * pow(10.0, -1 * getScale());
}

double SQLNumeric::getMinValue() const
{
  if (NumericType::isUnsigned())
    {
      return double(0);
    }
  else
    {
      switch (getNominalSize()) {
	    case sizeof(Int64):
	    {
	       Int64 temp = 0;
	       Lng32 i=0;
	       for (; i<getPrecision(); i++)
	       {
		  temp = temp * 10 + 9;
	       }
	       temp = -temp;
               temp *= pow(10.0, -1 * getScale());
               return double(temp);
	    }
	    break;

#ifdef NA_64BIT
            // dg64 - a bit of a guess
	    case sizeof(Int32):
#else
	    case sizeof(Lng32):
#endif
	    {
	       Lng32 temp = 0;
	       Lng32 i=0;
	       for (; i<getPrecision(); i++)
	       {
		  temp = temp * 10 + 9;
	       }
	       temp = -temp;
               temp *= pow(10.0, -1 * getScale());
               return double(temp);

	    }
	    break;

	    case sizeof(short):
	    case sizeof(Int8):
	    {
	       short temp = 0;
	       Lng32 i=0;
	       for (; i<getPrecision(); i++)
	       {
#pragma nowarn(1506)   // warning elimination
		  temp = temp * 10 + 9;
#pragma warn(1506)  // warning elimination
	       }
#pragma nowarn(1506)   // warning elimination
	       temp = -temp;
#pragma warn(1506)  // warning elimination

               temp *= pow(10.0, -1 * getScale());
               return double(temp);
	    }
	    break;

            default: 
               return 0;
         }
    }
}

double SQLNumeric::getMaxValue() const
{
   switch (getNominalSize())
     {
        case sizeof(Int64):
        {
	   Int64 temp = 0;
	   Lng32 i=0;
	   for (; i<getPrecision(); i++)
	   {
	      temp = temp * 10 + 9;
	   }

           temp *= pow(10.0, -1 * getScale());

	   return double(temp);
        }
        break;

#ifdef NA_64BIT
        // dg64 - a bit of a guess
        case sizeof(Int32):
#else
        case sizeof(Lng32):
#endif
        {
	   Lng32 temp = 0;
	   Lng32 i=0;
	   for (; i<getPrecision(); i++)
	   {
	      temp = temp * 10 + 9;
	   }

           temp *= pow(10.0, -1 * getScale());

	   return double(temp);
        }
        break;

        case sizeof(short):
        case sizeof(Int8):
        {
	   short temp = 0;
	   Lng32 i=0;
	   for (; i<getPrecision(); i++)
	   {
#pragma nowarn(1506)   // warning elimination
	      temp = temp * 10 + 9;
#pragma warn(1506)  // warning elimination
	   }

           temp *= pow(10.0, -1 * getScale());

	   return double(temp);
        }
        break;

        default:
          return 0;
     }
}

// -----------------------------------------------------------------------
//  Methods for SQLDecimal
// -----------------------------------------------------------------------

SQLDecimal::SQLDecimal(Lng32 length, Lng32 scale,
		       NABoolean allowNegValues,
		       NABoolean allowSQLnull,
			   CollHeap * heap)
: NumericType
  ( LiteralDecimal
   , length
   , length
   , scale
   , 1
   , allowNegValues
   , allowSQLnull
   ,FALSE
   ,heap
   )
{
} // SQLDecimal()


double SQLDecimal::encode(void * input) const
{
  double temp = 0;
  char * valPtr = (char *)input;

  // skip the null indicator header, if exists
  if (supportsSQLnull()) valPtr += getSQLnullHdrSize();

  char * temp_dec = new char[getNominalSize() + 1];
  str_cpy_all(temp_dec, valPtr, getNominalSize());
  temp_dec[getNominalSize()] = 0;

  // first bit of the first byte is the sign bit.
  // if set, the decimal number is a negative number.
  // Reset the bit before converting to double.
  if (temp_dec[0] & 0200)
    {
#pragma nowarn(1506)   // warning elimination
      temp_dec[0] = temp_dec[0] & 0177;
#pragma warn(1506)  // warning elimination
      temp = - atof(temp_dec);
    }
  else
    temp = atof(temp_dec);

  // upscale it
  temp = temp * pow(10.0, -1 * getScale());

  delete temp_dec;

  return temp;
}

// -- Min and max permissible values

void SQLDecimal::minRepresentableValue(void* bufPtr, Lng32* bufLen,
				       NAString ** stringLiteral,
				       CollHeap* h) const
{
  assert(*bufLen >= getNominalSize());
  *bufLen = getNominalSize();

  for (Lng32 i=0; i<*bufLen; i++)
    {
      if (NumericType::isUnsigned())
	((char *)bufPtr)[i] = '0';
      else
	((char *)bufPtr)[i] = '9';
    }

  // Convert to a string
  if (stringLiteral != NULL)
    {
      *stringLiteral = new (h) NAString(h);

      // prefix '-' sign, if signed decimal
      if (!NumericType::isUnsigned())
	{
	  (*stringLiteral)->append("-");
	}

      // and append the value.
      (*stringLiteral)->append((char *)bufPtr, (Int32)(*bufLen));

      insertScaleIndicator(*stringLiteral, getScale());
    }

  // set the sign bit, if signed decimal
  if (!NumericType::isUnsigned())
    {
      ((char *)bufPtr)[0] |= 0200;
    }
} // SQLDecimal::minRepresentableValue()

void SQLDecimal::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
				       NAString ** stringLiteral,
				       CollHeap* h) const
{
  assert(*bufLen >= getNominalSize());
  *bufLen = getNominalSize();

  for (Lng32 i=0; i<*bufLen; i++)
    {
      ((char *)bufPtr)[i] = '9';
    }

  // Convert to a string
  if (stringLiteral != NULL)
    {
      *stringLiteral = new (h) NAString((char *)bufPtr, (Int32)(*bufLen), h);
      insertScaleIndicator(*stringLiteral, getScale());
    }
} // SQLDecimal::maxRepresentableValue()


NAString* SQLDecimal::convertToString(double v, CollHeap* h)  const
{
  char nameBuf[NAME_BUF_LEN];  // a reasonably large buffer
  char fractionBuf[NAME_BUF_LEN];  // a reasonably large buffer

  if (getNominalSize() <= sizeof(short))
  { 
     short temp = (short)v;
     signedLongToAscii((Lng32)temp, nameBuf);
               
     short fraction = short(v - double(temp));
     signedLongToAscii(fraction, fractionBuf);
     fractionBuf[getScale()] = 0; 

  } else
  if (getNominalSize() <= sizeof(Int32)) {
     Lng32 temp = (Lng32)v;
     signedLongToAscii(temp, nameBuf);

     Int32 fraction = Int32(v - double(temp));
     signedLongToAscii(fraction, fractionBuf);
     fractionBuf[getScale()] = 0; 

  } else {
     Int64 temp = (Int64)v;
     convertInt64ToAscii(temp, nameBuf);

     Int64 fraction = Int64(v - double(temp));
     convertInt64ToAscii(fraction, fractionBuf);
     fractionBuf[getScale()] = 0; 
  }

  NAString* result = new (h) NAString(nameBuf, h);
  result -> append('.');
  result -> append(fractionBuf);

  return result;
}

double SQLDecimal::getMinValue() const
{
  if (NumericType::isUnsigned())
    return double(0.0);

  double temp = 1;
  for (Lng32 i=0; i<getNominalSize(); i++)
    {
      temp = temp * 10 + 9;
    }

  temp = temp * pow(10.0, -1 * getScale());

  return -temp;

}


double SQLDecimal::getMaxValue()  const
{
  double temp = 1;
  for (Lng32 i=0; i<getNominalSize(); i++)
    {
      temp = temp * 10 + 9;
    }

  temp = temp * pow(10.0, -1 * getScale());

  return temp;
}


// ------------------------------------------------------
// methods for class SQLBigNum
// ------------------------------------------------------

SQLBigNum::SQLBigNum(Lng32 precision, Lng32 scale,
		     NABoolean isARealBigNum,
		     NABoolean allowNegValues,
		     NABoolean allowSQLnull,
		     CollHeap * heap)
: NumericType
  ( LiteralBigNum
   , BigNumHelper::ConvPrecisionToStorageLengthHelper(precision)
   , precision
   , scale
   , 2                // Big Nums should always start on 2-byte boundaries
   , allowNegValues
   , allowSQLnull
   , FALSE
   , heap
   ),
  isARealBigNum_(isARealBigNum)
{
} // SQLBigNum()


const NAType* SQLBigNum::synthesizeType(
					enum NATypeSynthRuleEnum synthRule,
					const NAType& operand1,
					const NAType& operand2,
					CollHeap* h,
					UInt32 *flags) const
{
  //
  // If the second operand's type synthesis rules have higher precedence than
  // this operand's rules, use the second operand's rules.
  //
  if (operand2.getSynthesisPrecedence() > getSynthesisPrecedence())
    return operand2.synthesizeType(synthRule, operand1, operand2, h, flags);
  //
  // If either operand is not numeric, the expression is invalid.
  //
  if ((operand1.getTypeQualifier() != NA_NUMERIC_TYPE) ||
      (operand2.getTypeQualifier() != NA_NUMERIC_TYPE))
    return NULL;
  const NumericType& op1 = (NumericType &) operand1;
  const NumericType& op2 = (NumericType &) operand2;
  //
  // If either operand is signed, the result is signed.
  //
  NABoolean isSigned = op1.isSigned() OR op2.isSigned();
  //
  // If either operand is nullable, the result is nullable.
  //
  NABoolean isNullable = op1.supportsSQLnull() OR op2.supportsSQLnull();

  NABoolean isRealBigNum = 
    ((op1.isBigNum() && ((SQLBigNum&)op1).isARealBigNum()) ||
     (op2.isBigNum() && ((SQLBigNum&)op2).isARealBigNum()));

  //
  // Compute the scale and precision of the result.
  //
  Lng32 scale;
  Lng32 precision;
  switch (synthRule) {
  case SYNTH_RULE_UNION: {
    Lng32 magnitude = MAXOF(op1.getMagnitude(), op2.getMagnitude());
    scale = MAXOF(op1.getScale(), op2.getScale());
    precision = (magnitude + 9) / 10 + scale;
    break;
  }
  case SYNTH_RULE_PASS_THRU_NUM: {
    //
    // Return a new type of type 'this' with data attributes of 'other'.
    //
    assert(this == &op2);
    const NumericType& other = op1;
    scale = other.getScale();
    precision = (other.getMagnitude() + 9) / 10 + scale;
    isSigned = other.isSigned();
    isNullable = other.supportsSQLnull();
    break;
  }
  case SYNTH_RULE_ADD:
  case SYNTH_RULE_SUB: {
    Lng32 magnitude = MAXOF(op1.getMagnitude(), op2.getMagnitude());
    scale = MAXOF(op1.getScale(), op2.getScale());
    precision = (magnitude + 9) / 10 + scale + 1;
    break;
  }
  case SYNTH_RULE_MUL:
    scale = op1.getScale() + op2.getScale();
    precision = (op1.getMagnitude() + 9) / 10 +
                (op2.getMagnitude() + 9) / 10 +
                scale;
    break;
  case SYNTH_RULE_DIV:
    //    scale = MAXOF(op1.getScale(), op2.getScale());
    //precision = (op1.getMagnitude() + 9) / 10 +
    //  (op2.getMagnitude() + 9) / 10 +
    //  scale;

    scale = (op2.getMagnitude() + 9) / 10 + op1.getScale();
    precision = (op1.getMagnitude() + 9) / 10 + op2.getScale() + scale;
    break;
  case SYNTH_RULE_EXP:
    precision = MAX_NUMERIC_PRECISION;
    scale = 6;
    isSigned = op1.isSigned();
    break;
  default:
    return NULL;
  }

  return new(h) SQLBigNum(precision,
                          scale,
                          isRealBigNum,
                          isSigned,
                          isNullable,
			  NULL);
}

double SQLBigNum::getNormalizedValue(void* buf) const
{
  double temp = 0;
  char * valPtr = (char *)buf;

  char sign = BIGN_GET_SIGN(valPtr, getNominalSize());

  // if negative, just return -1
  if (sign)
    return -1;

  // Recast input as an array of unsigned shorts
  unsigned short * valPtrInShorts = (unsigned short *) valPtr;
  // NOTE: The last bit contains the sign bit.  It must be 0 since, if not,
  //       we would have returned -1 above.
  for (Int32 i = getNominalSize()/2 - 1; i >= 0; i--)
  {
    temp = temp * 65536 + valPtrInShorts[i]; // 65536 = 2^16, base for Big Nums
    // set a limit to prevent possible overflow
    if (temp >= 2.7430620343968440e+303 ) {  // max of double / 65536
      return -1;  
    }
  }

  return temp * pow(10.0, -1 * getScale());
}

double SQLBigNum::encode (void * input) const
{
  double temp = 0;

  char * valPtr = (char *)input;
  if (supportsSQLnull()) valPtr += getSQLnullHdrSize();

  // Recast input as an array of unsigned shorts
  unsigned short * valPtrInShorts = (unsigned short *) valPtr;

  char sign = BIGN_GET_SIGN(valPtr, getNominalSize());

  // Clear sign bit
  BIGN_CLR_SIGN(valPtr, getNominalSize());

  for (Int32 i = getNominalSize()/2 - 1; i >= 0; i--)
    {
      temp = temp * (USHRT_MAX + 1) + valPtrInShorts[i]; // USHRT_MAX + 1 = 2^16, i.e
                                                         // the base in which Big Nums
                                                         // are representated
    }

  // Now, upscale it.
  temp = temp * pow(10.0, -1 * getScale());

  // If source is negative, negate result and then reset sign
  if (sign) {
    temp = -temp;
    BIGN_SET_SIGN(valPtr, getNominalSize());
  }

  return temp;
}


// -- Min and max permissible values

void SQLBigNum::minRepresentableValue(void* bufPtr, Lng32* bufLen,
					    NAString ** stringLiteral,
					    CollHeap* h) const
{
  assert(*bufLen >= getNominalSize());
  *bufLen = getNominalSize();

  // Prepare ASCII representation of min permissible value in temp array.
  char * temp = new (h) char[getPrecision() + 1]; // one extra byte for the sign.
  if (NumericType::isUnsigned()) {
    temp[0] = '+';
    for (Int32 i = 1; i <= getPrecision(); i++)
      temp[i] = '0';
  }
  else {
    temp[0] = '-';
    for (Int32 i = 1; i <= getPrecision(); i++)
      temp[i] = '9';
  }

  // Convert ASCII to Big Num (with sign) representation
  BigNumHelper::ConvAsciiToBigNumWithSignHelper(getPrecision() + 1, // extra byte for sign
	                                        *bufLen,
				                temp,
				                (char *) bufPtr);

  // Convert to a string
  if (stringLiteral != NULL)
    {
      *stringLiteral = new (h) NAString(temp, (Int32)(getPrecision() + 1), h);
      insertScaleIndicator(*stringLiteral, getScale());
    }

  NADELETEBASIC(temp, (h));

} // minRepresentableValue()



void SQLBigNum::maxRepresentableValue(void* bufPtr,
				      Lng32* bufLen,
				      NAString ** stringLiteral,
				      CollHeap* h) const
{
  assert(*bufLen >= getNominalSize());
  *bufLen = getNominalSize();

  // Prepare ASCII representation of min permissible value in temp array.
  char * temp = new (h) char[getPrecision() + 1]; // one extra byte for the sign.
  temp[0] = '+';
  for (Int32 i = 1; i <= getPrecision(); i++)
    temp[i] = '9';

  // Convert ASCII to Big Num (with sign) representation
  BigNumHelper::ConvAsciiToBigNumWithSignHelper(getPrecision() + 1, // extra byte for sign
						*bufLen,
						temp,
						(char *) bufPtr);


  // Convert to a string
  if (stringLiteral != NULL)
    {
      *stringLiteral = new (h) NAString(temp, (Int32)(getPrecision() + 1), h);
      insertScaleIndicator(*stringLiteral, getScale());
    }

  NADELETEBASIC(temp, (h));

} // maxRepresentableValue()

NAType* SQLBigNum::closestEquivalentExternalType(CollHeap* heap)const
{
  NAType *equivalentType = NULL;

  equivalentType = this->newCopy(heap);
  return equivalentType;
}


// -----------------------------------------------------------------------
//  Methods for LSDecimal
// -----------------------------------------------------------------------

LSDecimal::LSDecimal(Lng32 length, Lng32 scale,
		     NABoolean allowSQLnull,CollHeap * heap)
: NumericType
( LiteralLSDecimal
  , length + 1    // first byte is sign, i.e., stargae size is length + 1
  , length
  , scale
  , 1
  , TRUE
  , allowSQLnull
  ,FALSE
  ,heap
  )
{
} // LSDecimal()

double LSDecimal::encode(void * input) const
{
  double temp = 0;
  char * valPtr = (char *)input;

  // skip the null indicator header, if exists
  if (supportsSQLnull()) valPtr += getSQLnullHdrSize();

  char * temp_dec = new char[getNominalSize() + 1];
  str_cpy_all(temp_dec, valPtr, getNominalSize());
  temp_dec[getNominalSize()] = 0;

  temp = atof(temp_dec);

  // upscale it
  temp = temp * pow(10.0, -1 * getScale());

  delete temp_dec;

  return temp;
}

// -- Min and max permissible values

void LSDecimal::minRepresentableValue(void* bufPtr, Lng32* bufLen,
				      NAString ** stringLiteral,
				      CollHeap* h) const
{
  assert(*bufLen >= getNominalSize());
  *bufLen = getNominalSize();
  ((char *)bufPtr)[0] = '-';
  for (Lng32 i = 1; i < *bufLen; i++)
    ((char *)bufPtr)[i] = '9';

  // Convert to a string
  if (stringLiteral != NULL)
    {
      *stringLiteral = new (h) NAString((char *)bufPtr, (Int32)(*bufLen), h);
      insertScaleIndicator(*stringLiteral, getScale());
    }
} // LSDecimal::minRepresentableValue()

void LSDecimal::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
				      NAString ** stringLiteral,
				      CollHeap* h) const
{
  assert(*bufLen >= getNominalSize());
  *bufLen = getNominalSize();
  ((char *)bufPtr)[0] = ' ';
  for (Lng32 i = 1; i < *bufLen; i++)
    ((char *)bufPtr)[i] = '9';

  // Convert to a string
  if (stringLiteral != NULL)
    {
      *stringLiteral = new (h) NAString((char *)bufPtr, (Int32)(*bufLen), h);
      insertScaleIndicator(*stringLiteral, getScale());
    }
} // LSDecimal::maxRepresentableValue()

NABoolean NumericType::isInteger() const      
{ 
  return ( qualifier_ == SQLInt_TYPE || qualifier_ == SQLSmall_TYPE ||
           (getTypeQualifier() == NA_NUMERIC_TYPE && getPrecision() > 0 && getScale() == 0) ); 
}

// -----------------------------------------------------------------------
// Type synthesis for binary operators
// -----------------------------------------------------------------------

const NAType* SQLFloat::synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                       const NAType& operand1,
				       const NAType& operand2,
				       CollHeap* h,
				       UInt32 *flags) const
{
  //
  // If the second operand's type synthesis rules have higher precedence than
  // this operand's rules, use the second operand's rules.
  //
  if (operand2.getSynthesisPrecedence() > getSynthesisPrecedence())
    return operand2.synthesizeType(synthRule, operand1, operand2,h,flags);
  //
  // If either operand is not numeric, the expression is invalid.
  //
  if ((operand1.getTypeQualifier() != NA_NUMERIC_TYPE) ||
      (operand2.getTypeQualifier() != NA_NUMERIC_TYPE))
    return NULL;
  const NumericType& op1 = (NumericType &) operand1;
  const NumericType& op2 = (NumericType &) operand2;
  //
  // If either operand is nullable, the result is nullable.
  //
  NABoolean isNullable = op1.supportsSQLnull() OR op2.supportsSQLnull();
  //
  // Compute the precision of the result.
  //
  Lng32 precision;
  switch (synthRule) {
  case SYNTH_RULE_UNION:
    precision = MAXOF(op1.getBinaryPrecision(), op2.getBinaryPrecision());
    break;
  case SYNTH_RULE_ADD:
  case SYNTH_RULE_SUB:
    precision = MAXOF(op1.getBinaryPrecision(), op2.getBinaryPrecision()) + 1;
    break;
  case SYNTH_RULE_MUL:
    precision = op1.getBinaryPrecision() + op2.getBinaryPrecision();
    break;
  case SYNTH_RULE_DIV:
  case SYNTH_RULE_EXP:
    precision = SQL_DOUBLE_PRECISION;
    break;
  default:
    return NULL;
  }
  precision = MINOF(precision, SQL_DOUBLE_PRECISION);
  //  return new(h) SQLFloat(isNullable, precision);
  return new(h) SQLDoublePrecision(isNullable, h, precision);
}

double SQLFloat::encode (void* bufPtr) const
{
  char * valPtr = (char *)bufPtr;
  if (supportsSQLnull())
    valPtr += getSQLnullHdrSize();

  double double_ieee;
  switch (getFSDatatype())
    {
    case REC_FLOAT32:
      {
	float float_ieee;
	str_cpy_all ((char *)&float_ieee, valPtr, getNominalSize());
	double_ieee = float_ieee;
      }
    break;

    case REC_FLOAT64:
      {
	str_cpy_all ((char *)&double_ieee, valPtr, getNominalSize());
      }
    break;

    }

  return double_ieee;
}

#define FLT_TDM_MIN 1.7272337e-77F
#define FLT_TDM_MAX 1.1579208e77F
#define DBL_TDM_MIN 1.7272337110188889e-77
#define DBL_TDM_MAX 1.15792089237316192e77

void SQLFloat::minRepresentableValue
( void* bufPtr
, Lng32* bufLen
, NAString ** stringLiteral
, CollHeap* h
) const
{
  assert(*bufLen >= getNominalSize());
  *bufLen = getNominalSize();

  char nameBuf[NAME_BUF_LEN];   // a reasonably large buffer

  switch (getFSDatatype())
    {
    case REC_FLOAT32:
      {
	float temp = - FLT_MAX;
	str_cpy_all((char *) bufPtr, (char *) &temp, SQL_REAL_SIZE);
	sprintf(nameBuf, "%17.9E", temp);
      }
    break;

    case REC_FLOAT64:
      {
	// for some reason, DBL_MAX gives an overflow error when parser tries
	// to convert the string literal back it the original value.  So, for
	// now, use FLT_MAX.  (rsm 3/30/2000)
	//    double temp = - DBL_MAX;

	double temp = - DBL_MAX;
	str_cpy_all((char *) bufPtr, (char *) &temp, SQL_DOUBLE_PRECISION_SIZE);
	sprintf(nameBuf, "%24.17E", temp);
      }
    break;



    } // switch

  if (stringLiteral != NULL)
    {
      //
      // Generate a printable string for the minimum value.
      //
      *stringLiteral = new (h) NAString(nameBuf, h);
    }

} // SQLFloat::minRepresentableValue()

void SQLFloat::maxRepresentableValue
( void* bufPtr
, Lng32* bufLen
, NAString ** stringLiteral
, CollHeap* h
) const
{
  assert(*bufLen >= getNominalSize());
  *bufLen = getNominalSize();

  char nameBuf[NAME_BUF_LEN];   // a reasonably large buffer

  switch (getFSDatatype())
    {
    case REC_FLOAT32:
      {
	float temp = FLT_MAX;
	str_cpy_all((char *) bufPtr, (char *) &temp, SQL_REAL_SIZE);
	sprintf(nameBuf, "%17.9E", temp);
      }
    break;

    case REC_FLOAT64:
      {
	// for some reason, DBL_MAX gives an overflow error when parser tries
	// to convert the string literal back it the original value.  So, for
	// now, use FLT_MAX.  (rsm 3/30/2000)
	//    double temp = - DBL_MAX;

	double temp = DBL_MAX;
	str_cpy_all((char *) bufPtr, (char *) &temp, SQL_DOUBLE_PRECISION_SIZE);
	sprintf(nameBuf, "%24.17E", temp);
      }
    break;

    } // switch

  if (stringLiteral != NULL)
    {
      //
      // Generate a printable string for the minimum value.
      //
      *stringLiteral = new (h) NAString(nameBuf, h);
    }

} // SQLFloat::maxRepresentableValue()



