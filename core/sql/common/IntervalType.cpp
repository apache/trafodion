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
* File:         IntervalType.C
* Description:  Interval type
* Created:      2/27/96
* Language:     C++
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------

#include <ctype.h>
#include <limits.h>
#include "DatetimeType.h"
#include "IntervalType.h"
#include "NumericType.h"
#include "str.h"
#include "exp_clause_derived.h"

// ***********************************************************************
// Static data
// ***********************************************************************

static const Int32 IntervalFieldStringSize = 3;	// punc plus two digits
static const UInt32 maxITVal[]  = { 0, 11, 0, 23, 59, 59 };
static const char precedingITPunc[] = "^-^ ::";
				  // ^ is placeholder, as no punc precedes YEAR
				  // and no punc precedes DAY (no interval spans
				  // month and day).

// ***********************************************************************
//
// getIntervalFields returns the start and end fields for a given interval
// FS datatype.  If the given FS datatype is invalid, the routine returns
// a status of -1.  Otherwise, it returns a status of 0.
//
// ***********************************************************************

short getIntervalFields(Lng32 fsDatatype,
                        rec_datetime_field &startField,
                        rec_datetime_field &endField)
{
  switch (fsDatatype) {
  case REC_INT_YEAR:
    startField = REC_DATE_YEAR;
    endField = REC_DATE_YEAR;
    break;
  case REC_INT_MONTH:
    startField = REC_DATE_MONTH;
    endField = REC_DATE_MONTH;
    break;
  case REC_INT_YEAR_MONTH:
    startField = REC_DATE_YEAR;
    endField = REC_DATE_MONTH;
    break;
  case REC_INT_DAY:
    startField = REC_DATE_DAY;
    endField = REC_DATE_DAY;
    break;
  case REC_INT_HOUR:
    startField = REC_DATE_HOUR;
    endField = REC_DATE_HOUR;
    break;
  case REC_INT_DAY_HOUR:
    startField = REC_DATE_DAY;
    endField = REC_DATE_HOUR;
    break;
  case REC_INT_MINUTE:
    startField = REC_DATE_MINUTE;
    endField = REC_DATE_MINUTE;
    break;
  case REC_INT_HOUR_MINUTE:
    startField = REC_DATE_HOUR;
    endField = REC_DATE_MINUTE;
    break;
  case REC_INT_DAY_MINUTE:
    startField = REC_DATE_DAY;
    endField = REC_DATE_MINUTE;
    break;
  case REC_INT_SECOND:
    startField = REC_DATE_SECOND;
    endField = REC_DATE_SECOND;
    break;
  case REC_INT_MINUTE_SECOND:
    startField = REC_DATE_MINUTE;
    endField = REC_DATE_SECOND;
    break;
  case REC_INT_HOUR_SECOND:
    startField = REC_DATE_HOUR;
    endField = REC_DATE_SECOND;
    break;
  case REC_INT_DAY_SECOND:
    startField = REC_DATE_DAY;
    endField = REC_DATE_SECOND;
    break;
  case REC_INT_FRACTION:
    startField = REC_DATE_FRACTION_MP;
    endField = REC_DATE_FRACTION_MP;
    break;
  default:
    return -1;
  }
  return 0;
} // getIntervalFields

// ***********************************************************************
//
// IntervalType : An ancillary static function, and the static class methods
//
// ***********************************************************************

static short getIntervalFSDatatype(rec_datetime_field startField,
				   rec_datetime_field endField)
{
  short fsDatatype = -1;
  if (startField > endField)
    return fsDatatype;
  switch (startField) {
  case REC_DATE_YEAR:
    if (endField == REC_DATE_YEAR)
      fsDatatype = REC_INT_YEAR;
    else if (endField == REC_DATE_MONTH)
      fsDatatype = REC_INT_YEAR_MONTH;
    break;
  case REC_DATE_MONTH:
    if (endField == REC_DATE_MONTH)
      fsDatatype = REC_INT_MONTH;
    break;
  case REC_DATE_DAY:
    if (endField == REC_DATE_DAY)
      fsDatatype = REC_INT_DAY;
    else if (endField == REC_DATE_HOUR)
      fsDatatype = REC_INT_DAY_HOUR;
    else if (endField == REC_DATE_MINUTE)
      fsDatatype = REC_INT_DAY_MINUTE;
    else if (endField == REC_DATE_SECOND)
      fsDatatype = REC_INT_DAY_SECOND;
    break;
  case REC_DATE_HOUR:
    if (endField == REC_DATE_HOUR)
      fsDatatype = REC_INT_HOUR;
    else if (endField == REC_DATE_MINUTE)
      fsDatatype = REC_INT_HOUR_MINUTE;
    else if (endField == REC_DATE_SECOND)
      fsDatatype = REC_INT_HOUR_SECOND;
    break;
  case REC_DATE_MINUTE:
    if (endField == REC_DATE_MINUTE)
      fsDatatype = REC_INT_MINUTE;
    else if (endField == REC_DATE_SECOND)
      fsDatatype = REC_INT_MINUTE_SECOND;
    break;
  case REC_DATE_SECOND:
    if (endField == REC_DATE_SECOND)
      fsDatatype = REC_INT_SECOND;
    break;
  case REC_DATE_FRACTION_MP:
     fsDatatype = REC_INT_FRACTION;
    break;

  default:
    break;
  }
  return fsDatatype;
} // getIntervalFSDatatype ancillary function

NABoolean IntervalType::validate(rec_datetime_field startField,
				 UInt32 leadingPrecision,
				 rec_datetime_field endField,
				 UInt32 fractionPrecision)
{
  NABoolean v = TRUE;
  if (startField > endField) v = FALSE;		// < and == are ok
  if (leadingPrecision == 0 && startField < REC_DATE_SECOND) v = FALSE;         // ANSI 10.1 SR 5
  if (fractionPrecision != 0 && endField < REC_DATE_SECOND) v = FALSE;
  if (leadingPrecision  > SQLInterval::MAX_LEADING_PRECISION) v = FALSE;
  if (fractionPrecision > SQLInterval::MAX_FRACTION_PRECISION) v = FALSE;
  if (startField <= REC_DATE_MONTH && endField > REC_DATE_MONTH) v = FALSE;
  return v;
} // IntervalType::validate

UInt32 IntervalType::getPrecision(rec_datetime_field startField,
				    UInt32 leadingPrecision,
				    rec_datetime_field endField,
				    UInt32 fractionPrecision)
{
  // See also file ../sqludr/sqludr.cpp, method TypeInfo::TypeInfo,
  // which implements this algorithm as well.
  if (! IntervalType::validate(startField, leadingPrecision,
			       endField,   fractionPrecision))
    return 0;
  UInt32 precision;
  switch (getIntervalFSDatatype(startField, endField)) {
  case REC_INT_YEAR:
  case REC_INT_MONTH:
  case REC_INT_DAY:
  case REC_INT_HOUR:
  case REC_INT_MINUTE:
  case REC_INT_FRACTION:
    precision = leadingPrecision;
    break;
  case REC_INT_YEAR_MONTH:
  case REC_INT_DAY_HOUR:
  case REC_INT_HOUR_MINUTE:
    precision = leadingPrecision + 2;
    break;
  case REC_INT_DAY_MINUTE:
    precision = leadingPrecision + 4;
    break;
  case REC_INT_SECOND:
    precision = leadingPrecision + fractionPrecision;
    break;
  case REC_INT_MINUTE_SECOND:
    precision = leadingPrecision + 2 + fractionPrecision;
    break;
  case REC_INT_HOUR_SECOND:
    precision = leadingPrecision + 4 + fractionPrecision;
    break;
  case REC_INT_DAY_SECOND:
    precision = leadingPrecision + 5 + fractionPrecision;
    break;
  default:
    return 0;
  }
  return precision;
} // IntervalType::getPrecision

UInt32 IntervalType::computeLeadingPrecision(rec_datetime_field startField,
                                             UInt32 precision,
                                             rec_datetime_field endField,
                                             UInt32 fractionPrecision)
{
  UInt32 leadingPrecision;
  switch (getIntervalFSDatatype(startField, endField)) {
  case REC_INT_YEAR:
  case REC_INT_MONTH:
  case REC_INT_DAY:
  case REC_INT_HOUR:
  case REC_INT_MINUTE:
    leadingPrecision = precision;
    break;
  case REC_INT_YEAR_MONTH:
  case REC_INT_DAY_HOUR:
  case REC_INT_HOUR_MINUTE:
    leadingPrecision = precision - 2;
    break;
  case REC_INT_DAY_MINUTE:
    leadingPrecision = precision - 4;
    break;
  case REC_INT_SECOND:
    leadingPrecision = precision - fractionPrecision;
    break;
  case REC_INT_MINUTE_SECOND:
    leadingPrecision = precision - 2 - fractionPrecision;
    break;
  case REC_INT_HOUR_SECOND:
    leadingPrecision = precision - 4 - fractionPrecision;
    break;
  case REC_INT_DAY_SECOND:
    leadingPrecision = precision - 5 - fractionPrecision;
    break;
  case REC_INT_FRACTION:
    leadingPrecision = precision - fractionPrecision;
    break;
  default:
    return 0;
  }
  return leadingPrecision;
} // IntervalType::getLeadingPrecision

Lng32 IntervalType::getStorageSize(rec_datetime_field startField,
				  UInt32 leadingPrecision,
				  rec_datetime_field endField,
				  UInt32 fractionPrecision)
{
  Lng32 size = getBinaryStorageSize(getPrecision(startField, 
                            leadingPrecision,
                            endField,
                            fractionPrecision));
  

  // interval datatypes are stored as 2(smallint),4(int) or 8(largeint) bytes.
  // If size is tinyint size based on precision, change it to smallint size.
  if (size == SQL_TINY_SIZE)
    size = SQL_SMALL_SIZE;
  
  return size;
} // IntervalType::getStorageSize

size_t IntervalType::getStringSize(rec_datetime_field startField,
				   UInt32 leadingPrecision,
				   rec_datetime_field endField,
				   UInt32 fractionPrecision)
{
  // 1 for sign, 1 for terminating '\0'
  size_t result;
  if (startField == REC_DATE_FRACTION_MP)
  {
   result = 1 + 1 + leadingPrecision;
  }
  else
  {
    result = 1 + 1 +
             leadingPrecision +
             IntervalFieldStringSize * (endField - startField);
    if (fractionPrecision)
       result += fractionPrecision + 1;       // 1 for "."
  }
  return result;
} // IntervalType::getStringSize



// ***********************************************************************
//
//  IntervalType : regular member functions
//
// ***********************************************************************

short IntervalType::getFSDatatype() const
{
  return getIntervalFSDatatype(getStartField(), getEndField());
}

NAString IntervalType::getIntervalQualifierAsString() const
{
  NAString fields;

  fields += getFieldName(getStartField());
  char precision[10];
  if (getStartField() >= REC_DATE_SECOND) {
    sprintf(precision, "(%u,%u)", getLeadingPrecision(),getFractionPrecision());
    fields += precision;
  } else if (getStartField() == getEndField()) {
    sprintf(precision, "(%u)", getLeadingPrecision());
    fields += precision;
  } else {
    sprintf(precision, "(%u) TO ", getLeadingPrecision());
    fields += precision;
    fields += getFieldName(getEndField());
    if (getEndField() >= REC_DATE_SECOND) {
      sprintf(precision, "(%u)", getFractionPrecision());
      fields += precision;
    }
  }

  return fields;
}

NAString IntervalType::getTypeSQLname(NABoolean terse) const
{
  NAString name("INTERVAL ");

  name += getIntervalQualifierAsString();

  getTypeSQLnull(name, terse);
  return name;
} // IntervalType::getTypeSQLname

const NAType* IntervalType::synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                           const NAType& operand1,
					   const NAType& operand2,
					   CollHeap* h,
					   UInt32 *flags) const
{
  //
  // The Interval is unsupported if the start field is FRACTION.
  //
  if (!operand1.isSupportedType() || !operand2.isSupportedType())
    return NULL;
  //
  // If the second operand's type synthesis rules have higher precedence than
  // this operand's rules, use the second operand's rules.
  //
  if (operand2.getSynthesisPrecedence() > getSynthesisPrecedence())
    return operand2.synthesizeType(synthRule, operand1, operand2, h, flags);
  //
  // Legal:  I+I, I-I, I/N, I*N, N*I.  ANSI 6.15.
  //
  switch (synthRule) {
  case SYNTH_RULE_ADD:
  case SYNTH_RULE_SUB:
  case SYNTH_RULE_UNION: {
    if (operand1.getTypeQualifier() != NA_INTERVAL_TYPE) return NULL;
    if (operand2.getTypeQualifier() != NA_INTERVAL_TYPE) return NULL;
    if (! operand1.isCompatible(operand2)) return NULL;
    const IntervalType& op1 = (IntervalType&)operand1;
    const IntervalType& op2 = (IntervalType&)operand2;
    rec_datetime_field startField = MINOF(op1.getStartField(),
                                          op2.getStartField());
    UInt32 leadingPrecision = MAXOF(op1.getLeadingPrecision(),
                                      op2.getLeadingPrecision());
    if ((synthRule == SYNTH_RULE_ADD) || (synthRule == SYNTH_RULE_SUB))
      leadingPrecision = MINOF(leadingPrecision + 1,
                               SQLInterval::MAX_LEADING_PRECISION);
    rec_datetime_field endField = MAXOF(op1.getEndField(), op2.getEndField());
    UInt32 fractionPrecision = MAXOF(op1.getFractionPrecision(),
                                       op2.getFractionPrecision());
    UInt32 totalPrecision = getPrecision(startField,
                                           leadingPrecision,
                                           endField,
                                           fractionPrecision);
    if (totalPrecision > SQLInterval::MAX_LEADING_PRECISION)
      leadingPrecision -= totalPrecision - SQLInterval::MAX_LEADING_PRECISION;
    return new(h) SQLInterval(h,
		 op1.supportsSQLnull() || op2.supportsSQLnull(),
		 startField,
		 leadingPrecision,
		 endField,
		 fractionPrecision);
    }
  case SYNTH_RULE_DIV: {
    if (operand1.getTypeQualifier() != NA_INTERVAL_TYPE) return NULL;
    if (operand2.getTypeQualifier() != NA_NUMERIC_TYPE) return NULL;
    if (! ((NumericType &) operand2).isExact()) return NULL;

    const IntervalType& op1 = (IntervalType&)operand1;
    const NumericType& op2 = (NumericType&)operand2;

    UInt32 leadingPrecision = (op2.getScale() + op1.getLeadingPrecision());
    UInt32 fractionPrecision = op1.getFractionPrecision();

    if ((op1.getLeadingPrecision() < leadingPrecision) ||
       (op2.supportsSQLnull() && !op1.supportsSQLnull()))
      return new(h) SQLInterval (h, op1.supportsSQLnull() || op2.supportsSQLnull(),
                                 op1.getStartField(),
                                 leadingPrecision,
                                 op1.getEndField(),
                                 fractionPrecision);
    else
      return &operand1;
    }
  case SYNTH_RULE_MUL: {
    const IntervalType* intervalOp;
    const NumericType* numericOp;
    switch (operand1.getTypeQualifier()) {
    case NA_INTERVAL_TYPE:
      if (operand2.getTypeQualifier() != NA_NUMERIC_TYPE) return NULL;
      intervalOp = (IntervalType*) &operand1;
      numericOp = (NumericType*) &operand2;
      break;
    case NA_NUMERIC_TYPE:
      if (operand2.getTypeQualifier() != NA_INTERVAL_TYPE) return NULL;
      numericOp = (NumericType*) &operand1;
      intervalOp = (IntervalType*) &operand2;
      break;
    default:
      return NULL;
    }
    if (! numericOp->isExact()) return NULL;
    UInt32 leadingPrecision =
      MINOF((UInt32) (intervalOp->getLeadingPrecision() +
                       (numericOp->getMagnitude() + 9) / 10),
            SQLInterval::MAX_LEADING_PRECISION);
    UInt32 totalPrecision = getPrecision(intervalOp->getStartField(),
                                           leadingPrecision,
                                           intervalOp->getEndField(),
                                           intervalOp->getFractionPrecision());
    if (totalPrecision > SQLInterval::MAX_LEADING_PRECISION)
      leadingPrecision -= totalPrecision - SQLInterval::MAX_LEADING_PRECISION;
    return new(h) SQLInterval(h,
		 intervalOp->supportsSQLnull() || numericOp->supportsSQLnull(),
		 intervalOp->getStartField(),
		 leadingPrecision,
		 intervalOp->getEndField(),
		 intervalOp->getFractionPrecision());
    }
  default:
    return NULL;
  } // switch
} // synthesizeType()

Lng32 IntervalType::getDisplayLength() const
  {
    //
    // Return the string size excluding the null terminator.
    //
    return getStringSize() - 1;
  }

// ***********************************************************************
//  IntervalType : Min and max values, and encoding
// ***********************************************************************

void IntervalType::getRepresentableValue(char sign,
					 void* bufPtr, Lng32* bufLen,
					 NAString** stringLiteral,
					 CollHeap* h) const
{
  size_t valueStringLen = getStringSize();
  char* valueString = new char[valueStringLen];
  char* v = valueString;
  char digit;
  if (sign == '0') {
    sign = '+';
    digit = '0';
  } else {
    *v++ = sign;
    digit = '9';
  }
  Int32 i = getLeadingPrecision();
  for (; i > 0; i--) *v++ = digit;
  if (getStartField() != REC_DATE_FRACTION_MP) {
    for (Int32 field = getStartField() + 1; field <= getEndField(); field++) {
    Int32 index = field - REC_DATE_YEAR;          // zero-based array index!
    sprintf(v, "%c%02u", precedingITPunc[index],
            digit == '0' ? 0 : maxITVal[index]);
    v += IntervalFieldStringSize;
    }
    if (i = getFractionPrecision()) {
    *v++ = '.';
    for ( ; i > 0; i--) *v++ = digit;
    }
  }
  *v = '\0';

  i = (digit == '0') ? 0 : 1;
  IntervalValue value(&valueString[i], getStartField(), getLeadingPrecision(),
		      getEndField(), getFractionPrecision(), sign);
  assert(value.isValid());

  if (bufPtr) {
    assert(*bufLen >= value.getValueLen());
    *bufLen = value.getValueLen();
    str_cpy_all((char*)bufPtr, (char*)value.getValue(), value.getValueLen());
  }

  if (stringLiteral)
    {
      *stringLiteral = new (h) NAString("INTERVAL ", h);

      if (sign == '-')
	**stringLiteral += "-";

      **stringLiteral += "\'";

      if (sign == '-')
	{
	  // skip the sign
	  **stringLiteral += &valueString[1];
	}
      else
	**stringLiteral += valueString;

      **stringLiteral += "\' ";

      **stringLiteral += getIntervalQualifierAsString();
    }

  delete [] valueString;
}

void IntervalType::getZeroValue(void* bufPtr, Lng32* bufLen,
				NAString** stringLiteral,
				CollHeap* h) const
{
  getRepresentableValue('0', bufPtr, bufLen, stringLiteral, h);
}

void IntervalType::minRepresentableValue(void* bufPtr, Lng32* bufLen,
					 NAString** stringLiteral,
					 CollHeap* h) const
{
  getRepresentableValue('-', bufPtr, bufLen, stringLiteral, h);
}

void IntervalType::maxRepresentableValue(void* bufPtr, Lng32* bufLen,
					 NAString** stringLiteral,
					 CollHeap* h) const
{
  getRepresentableValue('+', bufPtr, bufLen, stringLiteral,h);
}

NABoolean IntervalType::createSQLLiteral(const char * buf,
                                         NAString *&stringLiteral,
                                         NABoolean &isNull,
                                         CollHeap *h) const
{
  if (NAType::createSQLLiteral(buf, stringLiteral, isNull, h))
    return TRUE;

  // First step is to convert to ASCII, then add interval syntax around it
  const int NUMVAL_LEN = 32;
  char numVal[NUMVAL_LEN];
  char *numValPtr = numVal;
  const char *valPtr = buf + getSQLnullHdrSize();
  NABoolean isNegative = FALSE;
  
  unsigned short numValLen = 0;
  ComDiagsArea *diags = NULL;

  ex_expr::exp_return_type ok =
    convDoIt((char *) valPtr,
             getNominalSize(),
             getFSDatatype(),
             getPrecision(),
             getFractionPrecision(),
             numVal,
             NUMVAL_LEN,
             REC_BYTE_V_ASCII,
             0,
             SQLCHARSETCODE_UTF8,
             (char *) &numValLen,
             sizeof(numValLen),
             h,
             &diags,
             ConvInstruction::CONV_UNKNOWN,
             0);

   if ( ok != ex_expr::EXPR_OK || numValLen == 0)
     return FALSE;

   // put the literal together like this:
   // INTERVAL [-] '<value converted to ascii w/o sign>' <start field>[<precision>] [to <end-field>][<precision>]

   stringLiteral = new(h) NAString("INTERVAL ", h);

   // skip leading blanks, converted value is right-aligned
   while (*numValPtr == ' ' && numValLen > 0)
     {
       numValPtr++;
       numValLen--;
     }

   // determine the sign and skip blanks after the sign
   if (numValLen && *numValPtr == '-')
     {
       isNegative = TRUE;
       numValPtr++;
       numValLen--;
       while (*numValPtr == ' ' && numValLen > 0)
         {
           numValPtr++;
           numValLen--;
         }
     }
       
   if (numValLen <= 0)
     return FALSE;

   // add [-] '<value converted to ascii w/o sign>'
   if (isNegative)
     *stringLiteral += "- ' ";
   else
     *stringLiteral += "'";

   stringLiteral->append(numValPtr, numValLen);
   *stringLiteral += "' ";

   // add start field
   *stringLiteral += getFieldName(getStartField());

   // add leading precision if not the default of 2
   if (getStartField() != REC_DATE_SECOND)
     {
       if (getLeadingPrecision() != 2)
         {
           char lp[30];
           snprintf(lp, 30, "(%d)", getLeadingPrecision());
           *stringLiteral += lp;
         }
     }
   else if (getLeadingPrecision() != 2 || getFractionPrecision() != 6)
     {
       // this is the form SECOND(<leading prec>, <fraction prec>)
       char p[60];
       snprintf(p, 60, "(%d, %d)", getLeadingPrecision(), getFractionPrecision());
       *stringLiteral += p;
     }     

   // add end field if different from start field
   if (getEndField() != getStartField())
     {
       *stringLiteral += " TO ";
       *stringLiteral += getFieldName(getEndField());
     }
     
   // add fraction precision for secnds if not the default of 6
   if (getEndField() == REC_DATE_SECOND && getFractionPrecision() != 6)
     {
       char fp[30];
       snprintf(fp, 30, "(%d)", getFractionPrecision());
       *stringLiteral += fp;
     }

   return TRUE;
}

double IntervalType::encode (void* bufPtr) const
{
  return -1;			// for now ...##
} // IntervalType::encode

// ***********************************************************************
// IntervalType : Display/debug
// ***********************************************************************

void IntervalType::print(FILE* ofd, const char* indent) /*const*/
{
  fprintf(ofd, "%s ", getTypeSQLname().data());
  NAType::print(ofd, indent);
}

// ***********************************************************************
//
//  IntervalValue : An interval value
//
// ***********************************************************************

IntervalValue::IntervalValue
( const char* strValue
, rec_datetime_field startField
, UInt32 leadingPrecision
, rec_datetime_field endField
, UInt32 fractionPrecision
, char sign
)
: value_(NULL)
, valueLen_(0)
{
  if (sign != '+' && sign != '-')
    return;
  //
  // Check that the qualifier is valid.
  //
  Lng32 storageSize = IntervalType::getStorageSize(startField,
						  leadingPrecision,
						  endField,
						  fractionPrecision);
  if (storageSize == 0) {
    SQLInterval triggerErrmsg(NULL, FALSE,
			      startField, leadingPrecision,
			      endField,   fractionPrecision);
    return;
  }
  //
  // Skip leading blanks.
  //
  while (*strValue == ' ' || *strValue == '\t')
    strValue++;
  //
  // Skip sign if present (e.g., strValue comes from getMin/MaxRepresentable()
  // or from print()).
  //
  if (scanChar(strValue, sign))
    while (*strValue == ' ' || *strValue == '\t')
      strValue++;
  //
  // Scan the first field.  The maximum number of digits allowed is the
  // leading precision.
  //
  Int64 intervalValue=0; //initialize to zero fix Solution ID 10-020913-1668
  if (leadingPrecision > 0)
  {
   if (! scanField(strValue, leadingPrecision, intervalValue))
     return;
  }
  //
  // Scan the remaining fields and the separators preceding them.
  //
  Lng32 fieldValue;
  for (Int32 field = startField + 1; field <= endField; field++) {
    Int32 index = field - REC_DATE_YEAR;		// zero-based array index!
    //
    // Scan the punctuation preceding this field.
    // We allow colon as well as space to separate days and hours.
    //
    if (! scanChar(strValue, precedingITPunc[index]))
      if (field != REC_DATE_HOUR || (! scanChar(strValue, ':')))
	return;
    //
    // Scan the field.  It can have no more than two digits.  Check that
    // it doesn't exceed the maximum value for that field.
    //
	if ((! scanField(strValue, 2, fieldValue)) || (UInt32)fieldValue > maxITVal[index])
      return;
    intervalValue = (intervalValue * (maxITVal[index] + 1)) + fieldValue;
  }
  //
  // If the end field is the seconds field, scan the fraction if present.
  //
  if (endField >= REC_DATE_SECOND && startField != REC_DATE_FRACTION_MP) {
    if ((scanChar(strValue, '.') || (endField == REC_DATE_FRACTION_MP))
         && isdigit(*strValue)) {
      const char* fraction = strValue;
      if (! scanField(strValue, SQLInterval::MAX_FRACTION_PRECISION, fieldValue))
        return;
      //
      // Convert the fraction to the precision specified in the qualifier.
      //
      UInt32 currentPrecision = strValue - fraction;
      if (currentPrecision < fractionPrecision)
	for ( ; currentPrecision++ < fractionPrecision; )
          fieldValue *= 10;
      else
        if (currentPrecision > fractionPrecision) { // illegal
          return;
        }
    } else
      fieldValue = 0;
    //
    // Add the fraction to the interval value.
    //
    for (UInt32 i = 0; i < fractionPrecision; i++)
      intervalValue *= 10;
    intervalValue += fieldValue;
  }
  //
  // If the sign is '-', negate the interval value.
  //
  if (sign == '-')
    intervalValue = - intervalValue;
  //
  // Skip trailing blanks.
  //
  while (*strValue == ' ' || *strValue == '\t')
    strValue++;
  //
  // If this is the end of the string, the interval value is valid.
  //
  if (*strValue == '\0')
    setValue(intervalValue, storageSize);

} // IntervalValue ctor

IntervalValue::IntervalValue
( const char* value
, Lng32 storageSize
)
: value_(new unsigned char[storageSize])
, valueLen_((unsigned short) storageSize)
{
  memcpy(value_, value, valueLen_);
} // IntervalValue ctor

NABoolean IntervalValue::scanField(const char* &strValue,
                                   UInt32 maxFieldLen,
                                   Lng32 &value) const
{
  //
  // Compute the value of the interval field.
  //
  value = 0;
  UInt32 i = 0;
  for (; i < maxFieldLen && isdigit(strValue[i]); i++)
    value = value * 10 + (strValue[i] - '0');
  //
  // If the field has no digits or too many digits, the field is invalid.
  //
  if (i == 0 || isdigit(strValue[i]))
    return FALSE;
  strValue += i;
  return TRUE;
}

NABoolean IntervalValue::scanField(const char* &strValue,
                                   UInt32 maxFieldLen,
                                   Int64 &value) const
{
  //
  // Compute the value of the interval field.
  //
  value = 0;
  UInt32 i = 0;
  for (; i < maxFieldLen && isdigit(strValue[i]); i++)
    value = value * 10 + (strValue[i] - '0');
  //
  // If the field has no digits or too many digits, the field is invalid.
  //
  if (i == 0 || isdigit(strValue[i]))
    return FALSE;
  strValue += i;
  return TRUE;
}

void IntervalValue::setValue(Int64 value, Lng32 valueLen)
{
  assert(0 <= valueLen && valueLen <= USHRT_MAX);
  valueLen_ = (unsigned short)valueLen;
  value_ = new unsigned char[valueLen_];
  //  memcpy(value_, (char *) &value + 8 - valueLen_, valueLen_);

  switch (valueLen_) {
  case SQL_SMALL_SIZE: {
    *(short *)value_ = (short)value;
    break;
  }
  case SQL_INT_SIZE: {
    *(Lng32 *)value_ = (Lng32)value;
    break;
  }
  case SQL_LARGE_SIZE:
    *(Int64 *)value_ = value;
    break;
  default:
    ;
  }
}

static void valueToField(Int64& value, NAString& result,
			 char* buffer, UInt32 prec, UInt32 factor,
			 char punc)
{
  Int64 tmpval = value;
  Int64 tmpfactor = (Lng32) factor;
  value = value / tmpfactor;
  tmpval -= value * tmpfactor;
  //  unsigned long i = *((unsigned long *) &tmpval + 1);
  ULng32 i = (ULng32)tmpval;
  sprintf(buffer, "%c%0*u", punc, prec, i);
  result.prepend(buffer);
}

NAString IntervalValue::getValueAsString(const IntervalType& dt) const
{
  size_t resultlen = dt.getStringSize();
  NAString result( (NASize_T) resultlen);
  if (! isValid())
    return result;

  Int64 value;
  switch (valueLen_) {
  case SQL_SMALL_SIZE: {
    short temp;
    memcpy((char *) &temp, value_, valueLen_);
    value = temp;
    break;
  }
  case SQL_INT_SIZE: {
    Lng32 temp;
    memcpy((char *) &temp, value_, valueLen_);
    value = temp;
    break;
  }
  case SQL_LARGE_SIZE:
    memcpy((char *) &value, value_, valueLen_);
    break;
  default:
    return result;
  }

  char sign = '+';
  if (value < 0) {
    sign = '-';
    value = -value;
  }

  // make a reasonable size result string
  char buffer[25];
  if (dt.getFractionPrecision()) {
    UInt32 i, factor = 1;
    for (i = dt.getFractionPrecision(); i; i--) factor *= 10;
    valueToField(value, result, buffer, dt.getFractionPrecision(), factor, '.');
  }
  for (Int32 field = dt.getEndField(); field > dt.getStartField(); field--) {
    Int32 index = field - REC_DATE_YEAR;		// zero-based array index!
    valueToField(value, result, buffer, IntervalFieldStringSize-1,
		 maxITVal[index] + 1, precedingITPunc[index]);
  }
  char leadingField[SQLInterval::MAX_LEADING_PRECISION + 1];
  convertInt64ToAscii(value, leadingField);
  sprintf(buffer, "%*s", dt.getLeadingPrecision(), leadingField);
  result.prepend(buffer);
  if (sign == '-')
    result.prepend(&sign, 1);
  return result;
}

void IntervalValue::print(const IntervalType& dt,
			  FILE* ofd, const char* indent) const
{
  if (isValid())
    fprintf(ofd, "%s\n", getValueAsString(dt).data());
  else
    fprintf(ofd, "%sInvalid %s\n", indent, dt.getTypeSQLname().data());
}

// ***********************************************************************
//
//  SQLInterval : Encode
//
// ***********************************************************************

double SQLInterval::encode(void *bufPtr) const
{
  char * valPtr = (char *)bufPtr;
  double val = 0;

  if (supportsSQLnull()) valPtr += getSQLnullHdrSize();

  Lng32 size = getStorageSize(getStartField(),
			     getLeadingPrecision(),
			     getEndField(),
			     getFractionPrecision());

  switch (size) {
  case sizeof(short): {
    short temp;
    memcpy((char *) &temp, valPtr, sizeof(short));
    val = temp;
    break;
  }
  case sizeof(Int32): {
    Int32 temp;
    memcpy((char *) &temp, valPtr, sizeof(Int32));
    val = temp;
    break;
  }
  case sizeof(Int64): {
    Int64 temp;
    memcpy((char *) &temp, valPtr, sizeof(Int64));
    val = convertInt64ToDouble(temp);
    break;
  }
  default:
    assert(FALSE);
  }

  return val;
}

// 10-031022-0617 -begin
// --------------------------------------------------------------
// Special implementation of encode, which depending upon the type 
// of the interval. This converts the value into seconds. This should 
// finally be merged with the above encode method.
// --------------------------------------------------------------

double SQLInterval::getValueInSeconds(double valInSecs)
{
  // First get the double value using encode()
  rec_datetime_field startField = getStartField();
  unsigned short i = 0;
  switch (startField) {
  case REC_DATE_YEAR:
	valInSecs = valInSecs * 365 * 24 * 60 * 60;
	break;
  case REC_DATE_MONTH:
	valInSecs = valInSecs * 30 * 24 * 60 * 60;
	break;
  case REC_DATE_DAY:
	valInSecs = valInSecs * 24 * 60 * 60;
	break;
  case REC_DATE_HOUR:
	valInSecs = valInSecs * 60 * 60;
	break;
  case REC_DATE_MINUTE:
	valInSecs = valInSecs * 60;
	break;
  case REC_DATE_SECOND:
	for (i = 0; i < getFractionPrecision(); i++)
	  valInSecs /= 10;
	break;
  default:
	break;
  }
  return valInSecs;
}

  //10-031022-0617 -end

