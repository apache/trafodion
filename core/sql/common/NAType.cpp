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
******************************************************************************
*
* File:         NAType.C
* Description:  Novel Abstraction for a Type
* Created:      11/16/1994
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "NAType.h"
#include "DatetimeType.h"
#include "ComASSERT.h"
#include "NumericType.h"
#include "CharType.h"
#include "MiscType.h"
#include "CmpCommon.h"     /* want to put NAType obj's on statement heap ... */
#include "str.h"

// extern declaration
extern short
convertTypeToText_basic(char * text,
                        Lng32 fs_datatype,
                        Lng32 length,
                        Lng32 precision,
                        Lng32 scale,
                        rec_datetime_field datetimestart,
                        rec_datetime_field datetimeend,
                        short datetimefractprec,
                        short intervalleadingprec,
                        short upshift,
			short caseinsensitive,
                        CharInfo::CharSet charSet,
                        const char * collation_name,
                        const char * displaydatatype,
			short displayCaseSpecific);

// -----------------------------------------------------------------------
// Methods for class NAType
// -----------------------------------------------------------------------

NAType::NAType (const NAType & rhs, NAMemory * h)
     : dataStorageSize_ (rhs.dataStorageSize_),
       SQLnullFlag_     (rhs.SQLnullFlag_),
       SQLnullHdrSize_  (rhs.SQLnullHdrSize_),
       varLenFlag_      (rhs.varLenFlag_),
       lengthHdrSize_   (rhs.lengthHdrSize_),
       dataAlignment_   (rhs.dataAlignment_),
       totalAlignment_  (rhs.totalAlignment_),
       typeName_        (rhs.typeName_, h),
       qualifier_       (rhs.qualifier_),
       displayDataType_ (rhs.displayDataType_, h)
{}

NAType::NAType( NAMemory *h, 
                const NAString&    adtName,
                NABuiltInTypeEnum  ev,
                Lng32               dataStorageSize,
                NABoolean          nullable,
                Lng32               SQLnullHdrSize,
                NABoolean          varLenFlag,
                Lng32               lengthHdrSize,
                Lng32               dataAlignment
                ) : typeName_ (h) // memleak fix
                  , displayDataType_ (h)
{
  // The following assertion is commented out so that zero-length
  // strings can be supported.
  // ComASSERT(dataStorageSize > 0);

  // for now, both null indicator has to be a short and the
  // var len size can be a short or a long??
  // The following assertion is commented out.
  // Records can contain nullable fields and not have a
  // null Header. Same for arrays.
  // ComASSERT(SQLnullHdrSize == 2 || !nullable);

  ComASSERT(lengthHdrSize == 2 || lengthHdrSize == 4 || !varLenFlag);

  // supported alignments are none, 2, 4, and 8 bytes
  if (dataAlignment == 0) dataAlignment = 1;
  ComASSERT((dataAlignment == 1) || (dataAlignment == 2) ||
            (dataAlignment == 4) || (dataAlignment == 8));
  // Do NOT assert(dataStorageSize>0); that is the NAType::isValid() method.

  typeName_        = adtName;
  qualifier_       = ev;
  dataStorageSize_ = dataStorageSize;
  SQLnullFlag_     = nullable ? ALLOWS_NULLS : NOT_NULL_NOT_DROPPABLE;
  SQLnullHdrSize_  = nullable ? SQLnullHdrSize : 0;
  varLenFlag_      = varLenFlag;
  lengthHdrSize_   = varLenFlag_ ? lengthHdrSize : 0;
  dataAlignment_   = dataAlignment;

  // the total alignment of the type is the max of the alignments of
  // the null indicator, the var len field, and the data itself,
  // where the former two are aligned as numbers
  totalAlignment_ = MAXOF(MAXOF(dataAlignment_,SQLnullHdrSize_),lengthHdrSize_);
} // NAType()

// -- set the nullable flag and recompute the alignment of the type
//
// If physical nulls are not supported, then logical nulls are not either
// (if physicalNulls is False, then logicalNulls is ignored).
//
void NAType::setNullable(NABoolean physicalNulls, NABoolean logicalNulls)
{
  if (physicalNulls && !logicalNulls)
    SQLnullFlag_  = NOT_NULL_DROPPABLE;
  else
    SQLnullFlag_  = physicalNulls ? ALLOWS_NULLS : NOT_NULL_NOT_DROPPABLE;

  SQLnullHdrSize_ = physicalNulls ? SQL_NULL_HDR_SIZE : 0;

  totalAlignment_ = MAXOF(MAXOF(dataAlignment_,SQLnullHdrSize_),lengthHdrSize_);
}

// -- Compute the size of the storage required for this ADT
Lng32 NAType::getTotalAlignedSize() const
{
  Lng32 size = SQLnullHdrSize_;
  Lng32 align  = getDataAlignment();  // must be divisible by data alignment

  if (size > 0)
    {
      size = (((size - 1)/align) + 1) * align;
    }
  //  size = ADJUST(size, getDataAlignment());
  size += lengthHdrSize_;
  size += dataStorageSize_;

  return size;
} // getTotalSize()

Lng32 NAType::getTotalSize() const
{
  return dataStorageSize_ + getPrefixSize();
} // getTotalSize()

// -- Compute the total size of null indicator, variable length
//    indicator, and fillers between them and the data field

Lng32 NAType::getPrefixSize() const
{
  // the result must be the smallest number that is greater or equal
  // to SQLnullHdrSize_ + lengthHdrSize_ and that is divisible by
  // the data alignment.
  Lng32 align  = getDataAlignment();  // must be divisible by data alignment
 
  //long prefixLen = SQLnullHdrSize_ + lengthHdrSize_;

  // if the var length field has an alignment greater than that of the
  // null indicator, there is an extra filler between those two fields
 // if (lengthHdrSize_ > 2 AND SQLnullHdrSize_ == 2)
   // prefixLen = 2 * lengthHdrSize_;

  // return the result
  // return (SQLnullHdrSize_ + lengthHdrSize_ + align - 1) / align * align;

  // for now (FS2 simulator) there is no alignment:
  return (SQLnullHdrSize_ + lengthHdrSize_);
}

Lng32 NAType::getPrefixSizeWithAlignment() const
{
  // Previous method does not consider alignment and is used by getTotalSize()
  // the result must be the smallest number that is greater or equal
  // to SQLnullHdrSize_ + lengthHdrSize_ and that is divisible by
  // the data alignment.
  Lng32 align  = getDataAlignment();  // must be divisible by data alignment
 
  
  // if the var length field has an alignment greater than that of the
  // null indicator, there is an extra filler between those two fields
  // for now this method does not handle length indicator > 2 bytes
 // if (lengthHdrSize_ > 2 AND SQLnullHdrSize_ == 2)
   // prefixLen = 2 * lengthHdrSize_;

  // return the result
  return (SQLnullHdrSize_ + lengthHdrSize_ + align - 1) / align * align;
}

// -- return the total size of this ADT just like NAType::getTotalSize,
//    except add filler bytes such that the size is a multiple of its
//    alignment (as returned by getByteAlignment())


// -- Equality comparison

NABoolean NAType::operator==(const NAType& other) const
{
  if (typeName_ == other.typeName_ &&
      qualifier_ == other.qualifier_ &&
      dataStorageSize_ == other.dataStorageSize_ &&
      SQLnullFlag_ == other.SQLnullFlag_ &&
      varLenFlag_ == other.varLenFlag_)
    return TRUE;
  else
    return FALSE;
} // operator==()

NABoolean NAType::equalIgnoreLength(const NAType& other) const
{
  if (typeName_ == other.typeName_ &&
      qualifier_ == other.qualifier_ &&
      SQLnullFlag_ == other.SQLnullFlag_ &&
      varLenFlag_ == other.varLenFlag_)
    return TRUE;
  else
    return FALSE;
}

NABoolean NAType::equalIgnoreNull(const NAType& other) const
{
  if (typeName_ == other.typeName_ &&
      qualifier_ == other.qualifier_ &&
      dataStorageSize_ == other.dataStorageSize_ &&
      varLenFlag_ == other.varLenFlag_)
    return TRUE;
  else
    return FALSE;
}

NABoolean NAType::equalPhysical(const NAType& other) const
{
  if (typeName_ == other.typeName_ &&
      qualifier_ == other.qualifier_ &&
      dataStorageSize_ == other.dataStorageSize_ &&
      supportsSQLnullPhysical() == other.supportsSQLnullPhysical() &&
      varLenFlag_ == other.varLenFlag_)
    return TRUE;
  else
    return FALSE;
}

Lng32 NAType::getEncodedKeyLength() const
{
  // by default we assume that a NULL indicator gets prepended to the
  // encoded form and that any variable indicators get eliminated and
  // the data field is extended up to its maximum length.
  // There are no fillers in encoded keys (neither between NULL indicators
  // and the data nor between different key columns).
  return getSQLnullHdrSize() + getNominalSize();
}

const NAType* NAType::synthesizeNullableType(NAMemory * h) const
{
  if (this->supportsSQLnull())
    return this;
  NAType *result = this->newCopy(h);
  result->setNullable(TRUE);
  return result;
}

const NAType* NAType::synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                     const NAType& operand1,
                                     const NAType& operand2,
				     NAMemory * h,
				     UInt32 *flags) const
{
  //
  // No type synthesis rule was found for this operand.  If this is the first
  // operand, try the second operand's rules.  Otherwise, the expression is
  // invalid. Make sure no endless loop can occur if &operand1 == &operand2.
  //
  if (this == &operand1 AND this != &operand2)
    return operand2.synthesizeType(synthRule, operand1, operand2, h, flags);
  return NULL;
}

const NAType* NAType::synthesizeTernary(enum NATypeSynthRuleEnum synthRule,
                                        const NAType& operand1,
                                        const NAType& operand2,
                                        const NAType& operand3,
					NAMemory * h) const
{
  //
  // No type synthesis rule was found for this operand.  If this is the first
  // operand, try the second operand's rules.  If this is the second operand,
  // try the third operand's rules.  Otherwise, the expression is invalid.
  //
  if (this == &operand1)
    return operand2.synthesizeTernary(synthRule, operand1, operand2, operand3,
				      h);
  if (this == &operand2)
    return operand3.synthesizeTernary(synthRule, operand1, operand2, operand3,
				      h);
  return NULL;
}

//Tells us if this is a numeric type
NABoolean NAType::isNumeric() const
{
	//Any of the following types imply numeric data
	switch(getTypeQualifier())
	{
		case NA_NUMERIC_TYPE:
			return TRUE;
		case NA_INTERVAL_TYPE:
			return TRUE;
		case NA_DATETIME_TYPE:
			return TRUE;
		default:
			break;
	}
	return FALSE;
}
// ---------------------------------------------------------------------
// Methods that return the binary form of the minimum and the maximum
// representable values.
// ---------------------------------------------------------------------
void NAType::minRepresentableValue(void*, Lng32*, NAString**,
				   NAMemory * h) const {}

void NAType::maxRepresentableValue(void*, Lng32*, NAString**,
				   NAMemory * h) const {}

NAString* NAType::convertToString(double v, NAMemory * h) const
{
  return NULL;
}

NABoolean NAType::createSQLLiteral(const char * buf,
                                   NAString *&stringLiteral,
                                   NABoolean &isNull,
                                   CollHeap *h) const
{
  // the base class can handle the case of a NULL value and
  // generate a NULL value, otherwise let the derived class
  // generate a literal
  if (supportsSQLnull())
    {
      Int32 nullValue = 0;

      switch (getSQLnullHdrSize())
        {
        case 2:
          {
            Int16 tmp = *((Int16*) buf);
            nullValue = tmp;
          }
          break;

        default:
          ComASSERT(FALSE);
        }

      if (nullValue)
        {
          stringLiteral = new(h) NAString("NULL", h);
          isNull = TRUE;
          return TRUE;
        }
    }

  isNull = FALSE;
  return FALSE;
}

// keyValue INPUT is the string representation of the current value, then
// keyValue is OUTPUT as the string rep of the very NEXT value, and RETURN TRUE.
// If we're already at the maximum value, keyValue returns empty, RETURN FALSE.
NABoolean NAType::computeNextKeyValue(NAString &keyValue) const
{
  keyValue = "";
  NAString temp =
   "Derived class of NAType needs to define virtual method computeNextKeyValue";
  ComASSERT(keyValue == temp);
  return FALSE;
}

void NAType::print(FILE* ofd, const char* indent)
{
#ifdef TRACING_ENABLED 
  fprintf(ofd,"%s nominal size %d, total %d\n",
          indent,getNominalSize(),getTotalSize());
#endif
}

void NAType::display()
{
  std::cout << getTypeSQLname().data() << std::endl;
}

// -- The external name for the type (text representation)

NAString NAType::getTypeSQLname(NABoolean) const
{
  return "UNSUPPORTED TYPE";
}

void NAType::getTypeSQLnull(NAString& ns, NABoolean ignore) const
{
  if (! ignore)
    if (supportsSQLnullPhysical())
      if (supportsSQLnullLogical())
	ns += " ALLOWS NULLS";
      else
	ns += " NO NULLS DROPPABLE";
    else
      ns += " NO NULLS";
}

// -- Method for returning the hash key

NAString* NAType::getKey(NAMemory * h) const
{
  return new (h) NAString(getTypeSQLname(), h);
}

short NAType::getFSDatatype() const
{
  return -1;
}

Lng32 NAType::getPrecision() const
{
  return -1;
}

Lng32 NAType::getMagnitude() const
{
  return -1;
}

Lng32 NAType::getScale() const
{
  return -1;
}

Lng32 NAType::getPrecisionOrMaxNumChars() const
{
  return getPrecision();
}

Lng32 NAType::getScaleOrCharset() const
{
  return getScale();
}

// Implementation of a pure virtual function.
// Check if a conversion error can occur because of nulls.

NABoolean NAType::errorsCanOccur (const NAType& target, NABoolean lax) const
{
  if ((!supportsSQLnull()) || (target.supportsSQLnull()))
    return FALSE;
  return TRUE;
}

Lng32 NAType::getDisplayLength() const
{
  return getDisplayLength(getFSDatatype(),
                          getNominalSize(),
                          getPrecision(),
                          getScale(),
                          0);
}

// Gets the length that a given data type would use in the display tool
Lng32 NAType::getDisplayLength(Lng32 datatype,
		    Lng32 length,
		    Lng32 precision,
		    Lng32 scale,
                    Lng32 heading_len) const
{
  Lng32 d_len = 0;

  Int32 scale_len = 0;
  if (scale > 0)
    scale_len = 1;

  switch (datatype)
    {
    case REC_BPINT_UNSIGNED:
      // Can set the display size based on precision. For now treat it as
      // unsigned smallint
      d_len = SQL_USMALL_DISPLAY_SIZE;
      break;

    case REC_BIN8_SIGNED:
      d_len = SQL_TINY_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN8_UNSIGNED:
      d_len = SQL_UTINY_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN16_SIGNED:
      d_len = SQL_SMALL_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN16_UNSIGNED:
      d_len = SQL_USMALL_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN32_SIGNED:
      d_len = SQL_INT_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN32_UNSIGNED:
      d_len = SQL_UINT_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN64_SIGNED:
      d_len = SQL_LARGE_DISPLAY_SIZE + scale_len;
      break;

    case REC_BIN64_UNSIGNED:
      d_len = SQL_ULARGE_DISPLAY_SIZE + scale_len;
      break;

    case REC_BYTE_F_ASCII:
      d_len = length;
      break;

   case REC_BYTE_V_ASCII:
   case REC_BYTE_V_ASCII_LONG:
      d_len = length;
      break;

    case REC_DECIMAL_UNSIGNED:
      d_len = length + scale_len;
      break;

    case REC_DECIMAL_LSE:
      d_len = length + 1 + scale_len;
      break;

    case REC_NUM_BIG_SIGNED:
      {
	SQLBigNum tmp(NULL, precision,scale,FALSE,TRUE,FALSE);
	d_len = tmp.getDisplayLength();
      }
      break;

    case REC_NUM_BIG_UNSIGNED:
      {
	SQLBigNum tmp(NULL, precision,scale,FALSE,FALSE,FALSE);
	d_len = tmp.getDisplayLength();
      }
      break;

    case REC_FLOAT32:
      d_len = SQL_REAL_DISPLAY_SIZE;
      break;

    case REC_FLOAT64:
      d_len = SQL_DOUBLE_PRECISION_DISPLAY_SIZE;
      break;

    case REC_DATETIME:
     /*
      switch (precision) {
      case SQLDTCODE_DATE:
	{
	  SQLDate tmp(FALSE);
	  d_len = tmp.getDisplayLength();
	}
        break;
      case SQLDTCODE_TIME:
	{
	  SQLTime tmp(FALSE, (unsigned) scale);
	  d_len = tmp.getDisplayLength();
	}
        break;
      case SQLDTCODE_TIMESTAMP:
	{
	  SQLTimestamp tmp(FALSE, (unsigned) scale);
	  d_len = tmp.getDisplayLength();
	}
        break;
      default:
        d_len = length;
        break;
      }
      */
      ComASSERT(FALSE);
      break;

    case REC_INT_YEAR:
    case REC_INT_MONTH:
    case REC_INT_YEAR_MONTH:
    case REC_INT_DAY:
    case REC_INT_HOUR:
    case REC_INT_DAY_HOUR:
    case REC_INT_MINUTE:
    case REC_INT_HOUR_MINUTE:
    case REC_INT_DAY_MINUTE:
    case REC_INT_SECOND:
    case REC_INT_MINUTE_SECOND:
    case REC_INT_HOUR_SECOND:
    case REC_INT_DAY_SECOND:
    case REC_INT_FRACTION: {
        rec_datetime_field startField;
        rec_datetime_field endField;
        getIntervalFields(datatype, startField, endField);
        SQLInterval interval(NULL, FALSE,
                             startField,
                             (UInt32) precision,
                             endField,
                             (UInt32) scale);
        d_len = interval.getDisplayLength();
      }
      break;

    case REC_BLOB:
    case REC_CLOB:
      d_len = length;
      break;

    case REC_BOOLEAN:
      d_len = SQL_BOOLEAN_DISPLAY_SIZE;
      break;

    case REC_BINARY_STRING:
    case REC_VARBINARY_STRING:
      d_len = length;
      break;

    default:
      d_len = length;
      break;
    }

  if (d_len >= heading_len)
    return d_len;
  else
    return heading_len;
}

// A helper function.
// This method returns a text representation of the datatype
// based on the datatype information input to this method.
// Returns -1 in case of error, 0 if all is ok.
/*static*/
short NAType::convertTypeToText(char * text,	   // OUTPUT
				Lng32 fs_datatype,  // all other vars: INPUT
				Lng32 length,
				Lng32 precision,
				Lng32 scale,
				rec_datetime_field datetimestart,
				rec_datetime_field datetimeend,
				short datetimefractprec,
				short intervalleadingprec,
				short upshift,
				short caseinsensitive,
                                CharInfo::CharSet charSet,
                                CharInfo::Collation collation,
                                const char * displaydatatype,
				short displayCaseSpecific)
{
  return convertTypeToText_basic(text,
                                 fs_datatype,
                                 length,
                                 precision,
                                 scale,
                                 datetimestart,
                                 datetimeend,
                                 datetimefractprec,
                                 intervalleadingprec,
                                 upshift,
				 caseinsensitive,
                                 charSet,
                                 CharInfo::getCollationName(collation),
                                 displaydatatype,
				 displayCaseSpecific);
}

short NAType::getMyTypeAsHiveText(NAString * outputStr/*out*/) const
{
  Lng32		      fs_datatype		= getFSDatatype();

  switch (fs_datatype)
    {
    case REC_MIN_F_CHAR_H ... REC_MAX_F_CHAR_H:
      {
        SQLChar * ct = (SQLChar*)this;
        char buf[20];
        Int32 size = ct->getStrCharLimit();
        str_itoa(size, buf);
        *outputStr = "char(";
        *outputStr += buf;
        *outputStr += ")";
      }
      break;

    case REC_MIN_V_CHAR_H ... REC_MAX_V_CHAR_H:
      {
        SQLVarChar * ct = (SQLVarChar*)this;
        if (ct->wasHiveString())
          *outputStr = "string";
        else if (DFS2REC::isCharacterString(fs_datatype))
          {
            char buf[20];
            // Hive doesn't have the "n bytes" notation,
            // so just take the overall char limit
            Int32 size = ct->getStrCharLimit();
            str_itoa(size, buf);
            *outputStr = "varchar(";
            *outputStr += buf;
            *outputStr += ")";
          }
        else
          *outputStr = "binary";
      }
      break;

    case REC_BIN8_SIGNED:
    case REC_BIN8_UNSIGNED:
      *outputStr = "tinyint";
      break;

    case REC_BIN16_SIGNED:
    case REC_BIN16_UNSIGNED:
      *outputStr = "smallint";
      break;

    case REC_BIN32_SIGNED:
    case REC_BIN32_UNSIGNED:
      *outputStr = "int";
      break;

    case REC_BIN64_SIGNED:
      *outputStr = "bigint";
      break;

    case REC_MIN_DECIMAL ... REC_MAX_DECIMAL:
      if (getPrecision() <= 38)
        outputStr->format("decimal(%d,%d)",
                          getPrecision(),
                          getScale());
      else
        *outputStr = "double";
      break;

    case REC_FLOAT32:
      *outputStr = "float";
      break;

    case REC_FLOAT64:
      *outputStr = "double";
      break;

    case REC_BOOLEAN:
      *outputStr = "boolean";
      break;

    case REC_DATETIME:
      {
        DatetimeIntervalCommonType & dtiCommonType =
          (DatetimeIntervalCommonType &) *this;
         
        ComDateTimeStartEnd dtEndField = 
          (ComDateTimeStartEnd)dtiCommonType.getEndField();

        if ((rec_datetime_field)dtEndField == REC_DATE_SECOND)
          *outputStr = "timestamp";
        else 
          *outputStr = "date";
      }
      break;

    default:
      *outputStr = "unknown";
      break;

    } // switch

  return 0;
}

short NAType::getMyTypeAsText(NAString * outputStr,  // output
			      NABoolean addNullability,
                              NABoolean addCollation) const
{
  // get the right value for all these
  Lng32		      fs_datatype		= getFSDatatype();
  ComSInt32           precision			= 0;
  ComSInt32           scale			= 0;
  ComDateTimeStartEnd dtStartField		= COM_DTSE_UNKNOWN;
  ComDateTimeStartEnd dtEndField		= COM_DTSE_UNKNOWN; 
  ComSInt32           dtTrailingPrecision	= 0;
  ComSInt32           dtLeadingPrecision	= 0;
  ComBoolean          isUpshifted		= FALSE;
  ComBoolean          isCaseinsensitive         = FALSE;
  CharInfo::CharSet   characterSet   = CharInfo::UnknownCharSet;
  CharInfo::Collation collationSequence	= CharInfo::UNKNOWN_COLLATION;

  // Prepare parameters in case of a NUMERIC type
  if ( getTypeQualifier() == NA_NUMERIC_TYPE ) 
    {
      NumericType & numericType = (NumericType &) *this;
      
      scale = getScale();
      
      if (DFS2REC::isFloat(fs_datatype) ||
	  fs_datatype == REC_BPINT_UNSIGNED  ||  // all the floats
	  ! numericType.binaryPrecision() )	// or if non binary
	{
	  precision = getPrecision();
	}
    }
  
  // Prepare parameters in case of INTERVAL / DATETIME type
  // Note: ComDateTimeStartEnd is the same enum as rec_datetime_field 
  // (the latter is defined in /common/dfs2rec.h )
  if (getTypeQualifier() == NA_DATETIME_TYPE ||
      getTypeQualifier() == NA_INTERVAL_TYPE ) 
    {
      DatetimeIntervalCommonType & dtiCommonType =
	(DatetimeIntervalCommonType &) *this;
      
      dtStartField = (ComDateTimeStartEnd)dtiCommonType.getStartField();
      dtEndField = (ComDateTimeStartEnd)dtiCommonType.getEndField();
      dtTrailingPrecision = dtiCommonType.getFractionPrecision();
      dtLeadingPrecision = dtiCommonType.getLeadingPrecision();
    }
  
  // Prepare parameters in case of a CHARACTER type
  CharType & charType = (CharType &) *this;
  if ( getTypeQualifier() == NA_CHARACTER_TYPE ) 
    {
      isUpshifted       = charType.isUpshifted();
      isCaseinsensitive = charType.isCaseinsensitive();
      characterSet      = charType.getCharSet();
      if (addCollation)
        collationSequence = charType.getCollation();
      if ( characterSet == CharInfo::UTF8 /*  || (characterSet == CharInfo::SJIS */ )
      {
         // If byte length limit is EXACTLY (maxBytesPerChar * character limit), then use character limit
         if ( charType.getNominalSize() == charType.getStrCharLimit() * charType.getBytesPerChar() )
            precision   = charType.getStrCharLimit();
         // else leave precision as 0
      }
    }

  char text[100];
  short rc = 
  NAType::convertTypeToText(text,
			    fs_datatype,
			    getNominalSize(),
			    precision,
			    scale,
			    (rec_datetime_field)dtStartField,
			    (rec_datetime_field)dtEndField,
			    (short)dtTrailingPrecision,
			    (short)dtLeadingPrecision,
			    (short)isUpshifted,
			    (short)isCaseinsensitive,
			    characterSet,
			    collationSequence,
			    getDisplayDataType().data(),
			    0);
  if (rc)
    return -1;

  outputStr->append(text);

  if (NOT addNullability)
    {
      return 0; // do not reach the append null below 
    }

  if (NOT supportsSQLnull())
    {
      outputStr->append(" NOT NULL NOT DROPPABLE");
    }
  
  return 0;
}

Lng32 NAType::getSize() const  
{
  return sizeof(*this) + typeName_.length();
}

Lng32 NAType::hashKey() const  
{
  return typeName_.hash();
}

// return true iff it is safe to call NAType::hashKey on me
NABoolean NAType::amSafeToHash() const
{
  return typeName_.data() != NULL;
}

NABoolean NAType::useHashInFrequentValue() const
{
   return (DFS2REC::isAnyCharacter(getFSDatatype()));
}

NABoolean NAType::useHashRepresentation() const
{
    if ( DFS2REC::isAnyCharacter(getFSDatatype())) 
      return TRUE;
      
    if ( getTypeQualifier() == NA_NUMERIC_TYPE &&
        getTypeName() == LiteralNumeric &&
        getNominalSize() <= 8 ) // make is consistent with 
                                // ConstValue::computeHashValue() 
                                // (see ItemExpr.cpp).
      {
	return TRUE; // we want to use hash for SQL NUMERIC
	// when it is not inside the freq value list
      }

    return FALSE;
}

NABoolean NAType::isSkewBusterSupportedType() const
{
  if ( DFS2REC::isAnyCharacter(getFSDatatype()) ) 
    return TRUE;

  if ( getTypeQualifier() == NA_NUMERIC_TYPE &&
       getTypeName() == LiteralNumeric 
     )
     return TRUE;

  switch (getFSDatatype()) {
     // SQL integer data types can be handled
     case REC_BIN16_UNSIGNED:
     case REC_BIN16_SIGNED:
     case REC_BIN32_UNSIGNED:
     case REC_BIN32_SIGNED:
     case REC_BIN64_SIGNED:
       return TRUE;

     default:
       break;
  }

  return FALSE;
}
        
CharInfo::CharSet NAType::getCharSet() const	
{ return CharInfo::UnknownCharSet; };

#define MAX_PRECISION_ALLOWED  18
#define MAX_NUM_LEN     16

NAType* NAType::getNATypeForHive(const char* hiveType, NAMemory* heap)
{
  if ( !strcmp(hiveType, "tinyint"))
    {
      if (CmpCommon::getDefault(TRAF_TINYINT_SUPPORT) == DF_OFF)
        return new (heap) SQLSmall(heap, TRUE /* neg */, TRUE /* allow NULL*/);
      else
        return new (heap) SQLTiny(heap, TRUE /* neg */, TRUE /* allow NULL*/);
    }

  if ( !strcmp(hiveType, "smallint"))
    return new (heap) SQLSmall(heap, TRUE /* neg */, TRUE /* allow NULL*/);
 
  if ( !strcmp(hiveType, "int")) 
    return new (heap) SQLInt(heap, TRUE /* neg */, TRUE /* allow NULL*/);

  if ( !strcmp(hiveType, "bigint"))
    return new (heap) SQLLargeInt(heap, TRUE /* neg */, TRUE /* allow NULL*/);

  if ( !strcmp(hiveType, "boolean"))
    return new (heap) SQLBooleanNative(heap, TRUE);
 
  if ( !strcmp(hiveType, "string"))
    {
      Int32 lenInBytes = CmpCommon::getDefaultLong(HIVE_MAX_STRING_LENGTH_IN_BYTES);
      NAString hiveCharset = CmpCommon::getDefaultString(HIVE_DEFAULT_CHARSET);
      //        ActiveSchemaDB()->getDefaults().getValue(HIVE_DEFAULT_CHARSET);
      hiveCharset.toUpper();
      CharInfo::CharSet hiveCharsetEnum = CharInfo::getCharSetEnum(hiveCharset);
      Int32 maxNumChars = 0;
      Int32 storageLen = lenInBytes;
      SQLVarChar * nat = 
        new (heap) SQLVarChar(heap, CharLenInfo(maxNumChars, storageLen),
                              TRUE, // allow NULL
                              FALSE, // not upshifted
                              FALSE, // not case-insensitive
                              CharInfo::getCharSetEnum(hiveCharset),
                              CharInfo::DefaultCollation,
                              CharInfo::IMPLICIT);
      nat->setWasHiveString(TRUE);
      return nat;
    }
  
  if ( !strcmp(hiveType, "binary"))
    {
      Int32 len = CmpCommon::getDefaultLong(HIVE_MAX_BINARY_LENGTH);
      NAType * nat = NULL;
      if (CmpCommon::getDefault(TRAF_BINARY_SUPPORT) == DF_OFF)
        nat = new (heap) SQLVarChar(heap, len);
      else
        nat = new (heap) SQLBinaryString(heap, len, TRUE, TRUE);
      return nat;
    }

  if ( !strcmp(hiveType, "float"))
    return new (heap) SQLReal(heap, TRUE /* allow NULL*/);

  if ( !strcmp(hiveType, "double"))
    return new (heap) SQLDoublePrecision(heap, TRUE /* allow NULL*/);

  if ( !strcmp(hiveType, "timestamp"))
    return new (heap) SQLTimestamp(heap, TRUE /* allow NULL */ , DatetimeType::MAX_FRACTION_PRECISION);

  if ( !strcmp(hiveType, "date"))
    return new (heap) SQLDate(heap, TRUE /* allow NULL */);

  if ( (!strncmp(hiveType, "varchar", 7)) ||
       (!strncmp(hiveType, "char", 4)))
  {
    char maxLen[32];
    memset(maxLen, 0, 32);
    int i=0,j=0;
    int copyit = 0;
    int lenStr = strlen(hiveType);
    //get length
    for(i = 0; i < lenStr ; i++)
    {
      if(hiveType[i] == '(') //start
      {
        copyit=1;
        continue;
      }
      else if(hiveType[i] == ')') //stop
        break; 
      if(copyit > 0)
      {
        maxLen[j] = hiveType[i];
        j++;
      }
    }
    Int32 len = atoi(maxLen);

    if(len == 0) return NULL;  //cannot parse correctly

    NAString hiveCharset = CmpCommon::getDefaultString(HIVE_DEFAULT_CHARSET);
    hiveCharset.toUpper();
    CharInfo::CharSet hiveCharsetEnum = CharInfo::getCharSetEnum(hiveCharset);
    Int32 maxNumChars = 0;
    Int32 storageLen = len;
    if (CharInfo::isVariableWidthMultiByteCharSet(hiveCharsetEnum))
    {
      // For Hive VARCHARs, the number specified is the max. number of characters,
      // while we count in bytes when using HIVE_MAX_STRING_LENGTH_IN_BYTES for Hive STRING
      // columns. Set the max character constraint and also adjust the required storage length.
       maxNumChars = len;
       storageLen = len * CharInfo::maxBytesPerChar(hiveCharsetEnum);
    }

    if (!strncmp(hiveType, "char", 4))
      return new (heap) SQLChar(heap, CharLenInfo(maxNumChars, storageLen),
                                TRUE, // allow NULL
                                FALSE, // not upshifted
                                FALSE, // not case-insensitive
                                FALSE, // not varchar
                                CharInfo::getCharSetEnum(hiveCharset),
                                CharInfo::DefaultCollation,
                                CharInfo::IMPLICIT);
    else
      return new (heap) SQLVarChar(heap, CharLenInfo(maxNumChars, storageLen),
                                   TRUE, // allow NULL
                                   FALSE, // not upshifted
                                   FALSE, // not case-insensitive
                                   CharInfo::getCharSetEnum(hiveCharset),
                                   CharInfo::DefaultCollation,
                                   CharInfo::IMPLICIT);
  } 

  if ( !strncmp(hiveType, "decimal", 7) )
  {
    const Int16 DisAmbiguate = 0;

    Int32 i=0, pstart=-1, pend=-1, sstart=-1, send=-1, p=-1, s = -1;
    Int32 hiveTypeLen = strlen(hiveType);
    char pstr[MAX_NUM_LEN], sstr[MAX_NUM_LEN];
    memset(pstr,0,sizeof(pstr));
    memset(sstr,0,sizeof(sstr));

    for( i = 0; i < hiveTypeLen; i++ )
    {
      if(hiveType[i] == '(' )
      {
        pstart = i+1;
      }
      else if(hiveType[i] == ',')
      {
        pend = i;
        sstart = i+1;
      }
      else if(hiveType[i] == ')')
      {
        send = i;
      }
      else
       continue;
    }
    if(pend == -1) // no comma found, so no sstart and send
    {
       pend = send;
       send = -1;
       s = 0;
    }  
    if(pend - pstart > 0)
    {
      if( (pend - pstart) >= MAX_NUM_LEN ) // too long
        return NULL;
      strncpy(pstr,hiveType+pstart, pend-pstart);
      p=atoi(pstr);
    }

    if(send - sstart > 0)
    {
      if( (send - sstart) >= MAX_NUM_LEN ) // too long
        return NULL;
      strncpy(sstr,hiveType+sstart,send-sstart);
      s=atoi(sstr);
    }

    if( (p>0) && (p <= MAX_PRECISION_ALLOWED) ) //have precision between 1 - 18
    {
      if( ( s >=0 )  &&  ( s<= p) ) //have valid scale
        return new (heap) SQLNumeric(heap, TRUE, p, s, DisAmbiguate, TRUE);
      else
        return NULL;
    }
    else if( p > MAX_PRECISION_ALLOWED)  
    {
      if ( (s>=0) && ( s<= p ) ) //have valid scale
        return new (heap) SQLBigNum(heap, p, s, TRUE, TRUE, TRUE);
      else
        return NULL;
    }
    //no p and s given, p and s are all initial value
    else if( ( p == -1 ) && ( s == -1 ) )
    {
      // hive define decimal as decimal ( 10, 0 )
      return new (heap) SQLNumeric(heap, TRUE, 10, 0, DisAmbiguate, TRUE);
    }
    else
    {
      return NULL; 
    }

  }

  return NULL;
}
