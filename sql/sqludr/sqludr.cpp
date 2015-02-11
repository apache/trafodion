/**********************************************************************
 *
 * File:         sqludr.h
 * Description:  Interface between the SQL engine and routine bodies
 * Language:     C
 *
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
 *
 *********************************************************************/

#include "sqludr.h"
#include <stdio.h>
#include <cstdarg>
#include <climits>

using namespace tmudr;

// ------------------------------------------------------------------------
// This file includes implementations of methods defined in sqludr.h that
// are of interest to UDR writers.
// 
// For example, the default action for the C++ compiler interface for
// TMUDFs are shown here. These can be called by TMUDFs, to provide
// additional features and this source code can be used to decide whether
// the default actions are sufficient for a given TMUDF.UDF developer
// can inspect this code, copy and modify it for their own use or
// call it from derived classes.
//
// This file gets compiled in Trafodion as part of 
// trafodion/core/sql/optimizer/UdfDllInteraction.cpp.
// It does not need to be included into the DLL of the UDF.
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// Member functions for class UDRException
// ------------------------------------------------------------------------

UDRException::UDRException(int sqlState, const char *printf_format, ...)
{
  va_list args;
  const int maxMsgLen = 250;
  char msg[maxMsgLen];

  va_start(args, printf_format);
  vsnprintf(msg, maxMsgLen, printf_format, args);
  va_end(args);

  sqlState_ = sqlState;
  text_ = msg;
}

int UDRException::getSQLState() const   { return sqlState_; }
const std::string &UDRException::getText() const     { return text_; }

// ------------------------------------------------------------------------
// Member functions for class TMUDRSerializableObject
// ------------------------------------------------------------------------

TMUDRSerializableObject::TMUDRSerializableObject(int objectType,
                                                 unsigned short version,
                                                 unsigned short endianness)
{
  v_.objectType_  = objectType;
  v_.totalLength_ = -1; // will be set when we serialize the object
  v_.version_     = version;
  v_.endianness_  = endianness;
  v_.flags_       = 0;
  v_.filler_      = 0;
}

int TMUDRSerializableObject::getObjectType() const 
{
  return v_.objectType_;
}

unsigned short TMUDRSerializableObject::getVersion() const
{
  return v_.version_;
}

Endianness TMUDRSerializableObject::getEndianness() const
{
  return (Endianness) v_.endianness_;
}

int TMUDRSerializableObject::serializedLength()
{
  return sizeof(v_);
}

int TMUDRSerializableObject::serialize(Bytes &outputBuffer,
                                       int &outputBufferLength)
{
  // as a sanity check, also serialize the total length of the object
  v_.totalLength_ = serializedLength();

  if (outputBufferLength < v_.totalLength_)
    throw UDRException(38900,"need %d bytes to serialize object of type %d, have %d bytes",
                       v_.totalLength_,
                       v_.objectType_,
                       outputBufferLength);
  memcpy(outputBuffer, (void *) &v_, sizeof(v_));
  outputBuffer += sizeof(v_);
  outputBufferLength -= sizeof(v_);
  // Checks to be done by the caller:
  //    Once the entire object is serialized,
  //    call validateSerializedLength() to make sure
  //    the number of bytes produced matches v_.totalLength_

  return sizeof(v_);
}

int TMUDRSerializableObject::deserialize(Bytes &inputBuffer,
                                         int &inputBufferLength)
{
  if (inputBufferLength < sizeof(v_))
    throw UDRException(38900,"not enough data to deserialize object header, need %d, got %d bytes",
                       sizeof(v_),
                       inputBufferLength);
  memcpy((void *) &v_, inputBuffer, sizeof(v_));

  if (inputBufferLength < v_.totalLength_)
    throw UDRException(38900,"not enough data to deserialize object of type %d, need %d, got %d bytes",
                       v_.objectType_,
                       v_.totalLength_,
                       inputBufferLength);

  inputBuffer += sizeof(v_);
  inputBufferLength -= sizeof(v_);

  // Checks to be done by the caller:
  // 1. validateObjectType() right after this call
  // 2. Once the entire object is deserialized,
  //    call validateDeserializedLength() to make sure
  //    the number of bytes consumed matches v_.totalLength_

  return sizeof(v_);
}

void TMUDRSerializableObject::validateObjectType(int o)
{
  if (v_.objectType_ != o)
    throw UDRException(38900,"Object type of expected object (%d) does not match the type (%d) in the serialized buffer",
                       o,
                       v_.objectType_);
}

void TMUDRSerializableObject::validateSerializedLength(int l)
{
  if (l != v_.totalLength_)
    throw UDRException(38900,"Expected %d bytes to serialize object of type %d, actually produced %d bytes",
                       v_.totalLength_,
                       v_.objectType_,
                       l);
}

void TMUDRSerializableObject::validateDeserializedLength(int l)
{
  if (l != v_.totalLength_)
    throw UDRException(38900,"Expected %d bytes to deserialize object of type %d, actually consumed %d bytes",
                       v_.totalLength_,
                       v_.objectType_,
                       l);
}

int TMUDRSerializableObject::serializedLengthOfInt()
{
  return sizeof(int);
}

int TMUDRSerializableObject::serializedLengthOfLong()
{
  return sizeof(long);
}

int TMUDRSerializableObject::serializedLengthOfString(const char *s)
{
  return sizeof(int) + strlen(s);
}

int TMUDRSerializableObject::serializedLengthOfString(int stringLength)
{
  return sizeof(int) + stringLength;
}

int TMUDRSerializableObject::serializedLengthOfString(const std::string &s)
{
  return serializedLengthOfString(s.size());
}

int TMUDRSerializableObject::serializedLengthOfBinary(int binaryLength)
{
  return serializedLengthOfString(binaryLength);
}

int TMUDRSerializableObject::serializeInt(
     int i,
     Bytes &outputBuffer,
     int &outputBufferLength)
{
  if (outputBufferLength < sizeof(int))
    throw UDRException(38900,"insufficient space to serialize an int");

  memcpy(outputBuffer, &i, sizeof(int));

  outputBuffer += sizeof(int);
  outputBufferLength -= sizeof(int);

  return sizeof(int);
}

int TMUDRSerializableObject::serializeLong(
     long i,
     Bytes &outputBuffer,
     int &outputBufferLength)
{
  if (outputBufferLength < sizeof(long))
    throw UDRException(38900,"insufficient space to serialize an int");

  memcpy(outputBuffer, &i, sizeof(long));

  outputBuffer += sizeof(long);
  outputBufferLength -= sizeof(long);

  return sizeof(long);
}

int TMUDRSerializableObject::serializeString(
     const char *s,
     Bytes &outputBuffer,
     int &outputBufferLength)
{
  int result = 0;
  int strLen = strlen(s);
  if (outputBufferLength < sizeof(int) + strLen)
    throw UDRException(38900,"buffer to serialize string has %d bytes, needs %d",
                       outputBufferLength, strLen);

  memcpy(outputBuffer, &strLen, sizeof(int));

  outputBuffer += sizeof(int);
  outputBufferLength -= sizeof(int);

  memcpy(outputBuffer, s, strLen);

  outputBuffer += strLen;
  outputBufferLength -= strLen;

  return sizeof(int) + strLen;
}

int TMUDRSerializableObject::serializeString(
     const char *s,
     int len,
     Bytes &outputBuffer,
     int &outputBufferLength)
{
  if (outputBufferLength < sizeof(int) + len)
    throw UDRException(38900,"buffer to serialize string has %d bytes, needs %d",
                       outputBufferLength, len);

  memcpy(outputBuffer, &len, sizeof(int));

  outputBuffer += sizeof(int);
  outputBufferLength -= sizeof(int);

  if (len > 0)
    {
      memcpy(outputBuffer, s, len);

      outputBuffer += len;
      outputBufferLength -= len;
    }

  return sizeof(int) + len;
}

int TMUDRSerializableObject::serializeString(
     const std::string &s,
     Bytes &outputBuffer,
     int &outputBufferLength)
{
  return serializeString(s.data(),
                         s.size(),
                         outputBuffer,
                         outputBufferLength);
}

int TMUDRSerializableObject::serializeBinary(
     const void *b,
     int len,
     Bytes &outputBuffer,
     int &outputBufferLength)
{
  return serializeString(static_cast<const char *>(b),
                         len,
                         outputBuffer,
                         outputBufferLength);
}

int TMUDRSerializableObject::deserializeInt(
     int &i,
     Bytes &inputBuffer,
     int &inputBufferLength)
{
  if (inputBufferLength < sizeof(int))
    UDRException(38900,"insufficient space to deserialize an int");

  memcpy(&i, inputBuffer, sizeof(int));

  inputBuffer += sizeof(int);
  inputBufferLength -= sizeof(int);

  return sizeof(int);
}

int TMUDRSerializableObject::deserializeLong(
     long &i,
     Bytes &inputBuffer,
     int &inputBufferLength)
{
  if (inputBufferLength < sizeof(long))
    UDRException(38900,"insufficient space to deserialize an int");

  memcpy(&i, inputBuffer, sizeof(long));

  inputBuffer += sizeof(long);
  inputBufferLength -= sizeof(long);

  return sizeof(long);
}

int TMUDRSerializableObject::deserializeString(
     const char *&s,
     int &stringLength,
     bool makeACopy,
     Bytes &inputBuffer,
     int &inputBufferLength)
{
  if (inputBufferLength < sizeof(int))
    throw UDRException(38900,"insufficient space to deserialize length field of a string");

  int len;

  memcpy(&len, inputBuffer, sizeof(int));
  inputBuffer +=  sizeof(int);
  inputBufferLength -= sizeof(int);

  if (inputBufferLength < len)
    throw UDRException(38900,"string length indicator value %d exceeds size %d of serialized buffer",
                       len, inputBufferLength);

  if (len <= 0)
    s = NULL;
  else if (makeACopy)
    {
      char *tempBuf = new char[len];
      memcpy(tempBuf, inputBuffer, len);
      s = tempBuf;
    }
  else
    {
      // return a pointer to the string - needs to be copied immediately and is not null-terminated
      s = inputBuffer;
    }

  // this is the length of the string in bytes
  stringLength = len;

  inputBuffer += len;
  inputBufferLength -= len;

  // this is the number of bytes consumed from the buffer
  return sizeof(int) + len;
}

int TMUDRSerializableObject::deserializeString(
     std::string &s,
     Bytes &inputBuffer,
     int &inputBufferLength)
{
  const char *temp = NULL;
  int strLen = 0;
  int result = deserializeString(temp,
                                 strLen,
                                 false,
                                 inputBuffer,
                                 inputBufferLength);
  s.assign(temp, strLen);

  return result;
}

int TMUDRSerializableObject::deserializeBinary(
     const void **b,
     int &binaryLength,
     bool makeACopy,
     Bytes &inputBuffer,
     int &inputBufferLength)
{
  const char *temp;
  int result = deserializeString(temp,
                                 binaryLength,
                                 makeACopy,
                                 inputBuffer,
                                 inputBufferLength);

  *b = const_cast<char *>(temp);

  return result;
}

// ------------------------------------------------------------------------
// Member functions for class TypeInfo
// ------------------------------------------------------------------------

TypeInfo::TypeInfo(const TypeInfo &type) :
     TMUDRSerializableObject(TYPE_INFO_OBJ,
                             getCurrentVersion())
{
  d_.cType_ = type.d_.cType_;
  d_.sqlType_ = type.d_.sqlType_;
  d_.nullable_ = type.d_.nullable_;
  d_.scale_ = type.d_.scale_;
  d_.charset_ = type.d_.charset_;
  d_.intervalCode_ = type.d_.intervalCode_;
  d_.precision_ = type.d_.precision_;
  d_.collation_ = type.d_.collation_;
  d_.length_ = type.d_.length_;
  d_.dataOffset_ = type.d_.dataOffset_;
  d_.nullIndOffset_ = type.d_.nullIndOffset_;
  d_.vcLenIndOffset_ = type.d_.vcLenIndOffset_;
  d_.flags_ = type.d_.flags_;
  d_.fillers_[0] =
    d_.fillers_[1] =
    d_.fillers_[2] =
    d_.fillers_[3] = 0;
}

TypeInfo::TypeInfo(CTYPE_CODE cType,
                   int length,
                   bool nullable) :
     TMUDRSerializableObject(TYPE_INFO_OBJ,
                             getCurrentVersion())
{
  d_.cType_ = cType;
  d_.sqlType_ = UNDEFINED_SQL_TYPE;
  d_.nullable_ = nullable;
  d_.scale_ = 0;
  d_.charset_ = UNDEFINED_CHARSET;
  d_.intervalCode_ = UNDEFINED_INTERVAL_CODE;
  d_.precision_ = 0;
  d_.collation_ = SYSTEM_COLLATION;
  d_.length_ = 0;
  d_.dataOffset_ = -1;
  d_.nullIndOffset_ = -1;
  d_.vcLenIndOffset_ = -1;
  d_.flags_ = 0;
  d_.fillers_[0] =
  d_.fillers_[1] =
  d_.fillers_[2] =
  d_.fillers_[3] = 0;

  // derive the SQL type + length from the C type
  switch (cType)
    {
    case INT16:
      d_.sqlType_ = SMALLINT;
      d_.length_  = 2;
      break;

    case UINT16:
      d_.sqlType_ = SMALLINT_UNSIGNED;
      d_.length_  = 2;
      break;

    case INT32:
      d_.sqlType_ = INT;
      d_.length_  = 4;
      break;

    case UINT32:
      d_.sqlType_ = INT_UNSIGNED;
      d_.length_  = 4;
      break;

    case INT64:
      d_.sqlType_ = LARGEINT;
      d_.length_  = 8;
      break;

    case FLOAT:
      d_.sqlType_ = REAL;
      d_.length_  = 4;
      break;

    case DOUBLE:
      d_.sqlType_ = DOUBLE_PRECISION;
      d_.length_  = 8;
      break;

    case CHAR_ARRAY:
      d_.sqlType_ = CHAR;
      d_.length_ = length;
      d_.charset_ = CHARSET_UTF8;
      break;

    case VARCHAR_ARRAY:
      d_.sqlType_ = VARCHAR;
      d_.length_ = length;
      d_.charset_ = CHARSET_UTF8;
      if (length > 32767)
        d_.flags_ |= TYPE_FLAG_4_BYTE_VC_LEN;
      break;

    case STRING:
      d_.sqlType_ = VARCHAR;
      d_.length_  = length;
      // assume this is a
      // [VAR]CHAR (<length> BYTES) CHARACTER SET UTF8
      d_.charset_ = CHARSET_UTF8;
      if (length > 32767)
        d_.flags_ |= TYPE_FLAG_4_BYTE_VC_LEN;
      break;

      //case BYTES:
      //sqlType_ = VARCHAR;
      //length_  = length;
      //break;

      //case TIME:
      //sqlType_ = SQL_TIME;
      // Todo: length_  = ?;
      //break;

    case UNDEFINED_C_TYPE:
      d_.sqlType_ = UNDEFINED_SQL_TYPE;
      break;

    default:
      throw UDRException(38900,"Invalid C Type code for the short TypeInfo constructor: %d", cType);
      break;

    }
}

TypeInfo::TypeInfo(SQLTYPE_CODE sqlType,
                   int length,
                   bool nullable,
                   int scale,
                   SQLCHARSET_CODE charset,
                   SQLINTERVAL_CODE intervalCode,
                   int precision,
                   SQLCOLLATION_CODE collation) :
     TMUDRSerializableObject(TYPE_INFO_OBJ,
                             getCurrentVersion())
{
  d_.cType_ = UNDEFINED_C_TYPE;
  d_.sqlType_ = sqlType;
  d_.nullable_ = nullable;
  d_.scale_ = scale;
  d_.charset_ = charset;
  d_.intervalCode_ = intervalCode;
  d_.precision_ = precision;
  d_.collation_ = collation;
  d_.length_ = length;
  d_.dataOffset_ = -1;
  d_.nullIndOffset_ = -1;
  d_.vcLenIndOffset_ = -1;
  d_.flags_ = 0;
  d_.fillers_[0] =
  d_.fillers_[1] =
  d_.fillers_[2] =
  d_.fillers_[3] = 0;

  switch (sqlType)
    {
    case SMALLINT:
      d_.cType_ = INT16;
      d_.length_ = 2;
      break;

    case INT:
      d_.cType_ = INT32;
      d_.length_ = 4;
      break;

    case LARGEINT:
      d_.cType_ = INT64;
      d_.length_ = 8;
      break;

    case NUMERIC:
      d_.cType_ = INT32;
      d_.length_ = 4;
      if (d_.scale_ < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a numeric in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.scale_);
      if (d_.precision_ < 0 || d_.precision_ > 18)
        throw UDRException(38900,"Precision %d of a numeric in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.precision_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a numeric in TypeInfo::TypeInfo is greater than precision %d", d_.scale_, d_.precision_);
      break;

    case DECIMAL_LSE:
      d_.cType_ = CHAR_ARRAY;
      if (d_.scale_ < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a decimal in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.scale_);
      if (d_.precision_ < 0 || d_.precision_ > 18)
        throw UDRException(38900,"Precision %d of a decimal in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.precision_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a decimal in TypeInfo::TypeInfo is greater than precision %d", d_.scale_, d_.precision_);
      break;

    case SMALLINT_UNSIGNED:
      d_.cType_ = UINT16;
      d_.length_ = 2;
      break;

    case INT_UNSIGNED:
      d_.cType_ = UINT32;
      d_.length_ = 4;
      break;

    case NUMERIC_UNSIGNED:
      d_.cType_ = INT32;
      d_.length_ = 4;
      if (d_.scale_ < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a numeric unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.scale_);
      if (d_.precision_ < 0 || d_.precision_ > 18)
        throw UDRException(38900,"Precision %d of a numeric unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.precision_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a numeric unsigned in TypeInfo::TypeInfo is greater than precision %d", d_.scale_, d_.precision_);
      break;

    case DECIMAL_UNSIGNED:
      d_.cType_ = CHAR_ARRAY;
      if (scale < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a decimal unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.scale_);
      if (d_.precision_ < 0 || d_.precision_ > 18)
        throw UDRException(38900,"Precision %d of a decimal unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.precision_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a decimal unsigned in TypeInfo::TypeInfo is greater than precision %d", d_.scale_, d_.precision_);
      break;

    case REAL:
      d_.cType_ = FLOAT;
      d_.length_ = 4;
      break;

    case DOUBLE_PRECISION:
      d_.cType_ = DOUBLE;
      d_.length_ = 8;
      break;

    case CHAR:
      d_.cType_ = CHAR_ARRAY;
      if (d_.charset_ == UNDEFINED_CHARSET)
        throw UDRException(38900,"Charset must be specified for CHAR type in TypeInfo::TypeInfo");
      break;

    case VARCHAR:
      d_.cType_ = VARCHAR_ARRAY;
      if (d_.charset_ == UNDEFINED_CHARSET)
        throw UDRException(38900,"Charset must be specified for CHAR type in TypeInfo::TypeInfo");
      if (d_.collation_ == UNDEFINED_COLLATION)
        throw UDRException(38900,"Collation must be specified for CHAR type in TypeInfo::TypeInfo");
      if (d_.length_ > 32767)
        d_.flags_ |= TYPE_FLAG_4_BYTE_VC_LEN;
      break;

    case DATE:
      // string yyyy-mm-dd
      d_.cType_ = CHAR_ARRAY;
      d_.length_ = 10;
      d_.scale_ = 0;
      break;

    case TIME:
      // string hh:mm:ss
      d_.cType_ = CHAR_ARRAY;
      d_.length_ = 8;
      d_.scale_ = 0;
      break;

    case TIMESTAMP:
      // string yyyy-mm-dd hh:mm:ss.ffffff
      //        12345678901234567890123456
      d_.cType_ = CHAR_ARRAY;
      d_.length_ = 19;
      if (scale > 0)
        d_.length_ += scale+1;
      if (scale < 0 || scale > 6)
        throw UDRException(38900,"Scale %d of timestamp in TypeInfo::TypeInfo is outside the allowed range of 0-6", sqlType);
      break;

    case INTERVAL:
      // all intervals are treated like numbers
      d_.cType_ = INT32;
      d_.length_ = 4;
      if (d_.intervalCode_ == UNDEFINED_INTERVAL_CODE)
        throw UDRException(38900,"Interval code in TypeInfo::TypeInfo is undefined");
      if (scale < 0 || scale > 6)
        throw UDRException(38900,"Scale %d of interval in TypeInfo::TypeInfo is outside the allowed range of 0-6", sqlType);
      // todo: Check scale and precision in more detail
      break;

    default:
      throw UDRException(38900,"Invalid SQL Type code for the short TypeInfo constructor with an SQL code: %d", sqlType);
      break;
    }
}

TypeInfo::TypeInfo(CTYPE_CODE cType,
                   SQLTYPE_CODE sqlType,
                   bool nullable,
                   int scale,
                   SQLCHARSET_CODE charset,
                   SQLINTERVAL_CODE intervalCode,
                   int precision,
                   SQLCOLLATION_CODE collation,
                   int length) :
     TMUDRSerializableObject(TYPE_INFO_OBJ,
                             getCurrentVersion())
{
  d_.cType_ = cType;
  d_.sqlType_ = sqlType;
  d_.nullable_ = (nullable ? 1 : 0);
  d_.scale_ = scale;
  d_.charset_ = charset;
  d_.intervalCode_ = intervalCode;
  d_.precision_ = precision;
  d_.collation_ = collation;
  d_.length_ = length;
  d_.dataOffset_ = -1;
  d_.nullIndOffset_ = -1;
  d_.vcLenIndOffset_ = -1;
  d_.flags_ = 0;
  d_.fillers_[0] =
    d_.fillers_[1] =
    d_.fillers_[2] =
    d_.fillers_[3] = 0;
}

TypeInfo::CTYPE_CODE TypeInfo::getCType() const
{
  return (TypeInfo::CTYPE_CODE) d_.cType_;
}

TypeInfo::SQLTYPE_CODE TypeInfo::getSQLType() const
{
  return (TypeInfo::SQLTYPE_CODE) d_.sqlType_;
}

TypeInfo::SQLTYPE_CLASS_CODE TypeInfo::getSQLTypeClass() const
{
  switch (d_.sqlType_)
    {
    case SMALLINT:
    case INT:
    case LARGEINT:
    case NUMERIC:
    case DECIMAL_LSE:
    case SMALLINT_UNSIGNED:
    case INT_UNSIGNED:
    case NUMERIC_UNSIGNED:
    case DECIMAL_UNSIGNED:
    case REAL:
    case DOUBLE_PRECISION:
      return NUMERIC_TYPE;

    case CHAR:
    case VARCHAR:
      return CHARACTER_TYPE;

    case DATE:
    case TIME:
    case TIMESTAMP:
      return DATETIME_TYPE;

    case INTERVAL:
      return INTERVAL_TYPE;

    default:
      break;
    }

  return UNDEFINED_TYPE_CLASS;
}

TypeInfo::SQLTYPE_SUB_CLASS_CODE TypeInfo::getSQLTypeSubClass() const
{
  switch (d_.sqlType_)
    {
    case SMALLINT:
    case INT:
    case LARGEINT:
    case NUMERIC:
    case DECIMAL_LSE:
    case SMALLINT_UNSIGNED:
    case INT_UNSIGNED:
    case NUMERIC_UNSIGNED:
    case DECIMAL_UNSIGNED:
      return EXACT_NUMERIC_TYPE;
    case REAL:
    case DOUBLE_PRECISION:
      return APPROXIMATE_NUMERIC_TYPE;

    case CHAR:
      return FIXED_CHAR_TYPE;
    case VARCHAR:
      return VAR_CHAR_TYPE;

    case DATE:
      return DATE_TYPE;
    case TIME:
      return TIME_TYPE;
    case TIMESTAMP:
      return TIMESTAMP_TYPE;

    case INTERVAL:
      switch (d_.intervalCode_)
        {
        case INTERVAL_YEAR:
        case INTERVAL_MONTH:
        case INTERVAL_YEAR_MONTH:
          return YEAR_MONTH_INTERVAL_TYPE;

        case INTERVAL_DAY:
        case INTERVAL_HOUR:
        case INTERVAL_MINUTE:
        case INTERVAL_SECOND:
        case INTERVAL_DAY_HOUR:
        case INTERVAL_DAY_MINUTE:
        case INTERVAL_DAY_SECOND:
        case INTERVAL_HOUR_MINUTE:
        case INTERVAL_HOUR_SECOND:
        case INTERVAL_MINUTE_SECOND:
          return DAY_SECOND_INTERVAL_TYPE;

        default:
          break;
        }

    default:
      break;
    }

  return UNDEFINED_TYPE_SUB_CLASS;
}

bool TypeInfo::getIsNullable() const
{
  return (d_.nullable_ != 0);
}

int TypeInfo::getScale() const
{
  return d_.scale_;
}

TypeInfo::SQLCHARSET_CODE TypeInfo::getCharset() const
{
  return (TypeInfo::SQLCHARSET_CODE) d_.charset_;
}

TypeInfo::SQLINTERVAL_CODE TypeInfo::getIntervalCode() const
{
  return (TypeInfo::SQLINTERVAL_CODE) d_.intervalCode_;
}

int TypeInfo::getPrecision() const
{
  return d_.precision_;
}

TypeInfo::SQLCOLLATION_CODE TypeInfo::getCollation() const
{
  return (TypeInfo::SQLCOLLATION_CODE) d_.collation_;
}

int TypeInfo::getLength() const
{
  return d_.length_;
}

bool TypeInfo::canGetInt() const
{
  return canGetLong();
}

int TypeInfo::getInt(const char *row, bool &wasNull) const
{
  long result = getLong(row, wasNull);

  if (result < INT_MIN || result > INT_MAX)
    throw UDRException(
         38900, 
         "Under or overflow in getInt(), %ld does not fit in an int",
         result);

  return static_cast<int>(result);
}

bool TypeInfo::canGetLong() const
{
  return (d_.dataOffset_ >= 0 &&
          getSQLTypeSubClass() == EXACT_NUMERIC_TYPE);
}

long TypeInfo::getLong(const char *row, bool &wasNull) const
{
  if (row == NULL)
    throw UDRException(
         38900,
         "Row not available for getLong() or related method");

  if (d_.dataOffset_ < 0)
    throw UDRException(
         38900,
         "Offset for column not set, getLong() or related method not available");

  if (d_.nullIndOffset_ >= 0 &&
      (*((short *) (row + d_.nullIndOffset_)) != 0))
    {
      wasNull = true;
      return 0;
    }

  long result = 0;
  int tempSQLType = d_.sqlType_;
  const char *data = row + d_.dataOffset_;

  wasNull = false;

  // convert NUMERIC to the corresponding type with binary precision
  if (d_.sqlType_ == NUMERIC || d_.sqlType_ == NUMERIC_UNSIGNED)
    {
      if (d_.precision_ <= 4)
        if (d_.sqlType_ == NUMERIC)
          tempSQLType = SMALLINT;
        else
          tempSQLType = SMALLINT_UNSIGNED;
      else if (d_.precision_ <= 9)
        if (d_.sqlType_ == NUMERIC)
          tempSQLType = INT;
        else
          tempSQLType = INT_UNSIGNED;
      else if (d_.precision_ <= 18)
        if (d_.sqlType_ == NUMERIC)
          tempSQLType = LARGEINT;
        // else unsigned 8 byte integer is not supported
    }

  switch (tempSQLType)
    {
    case SMALLINT:
      result = *((short *) data);
      break;

    case INT:
      result = *((int *) data);
      break;

    case LARGEINT:
      result = *((long *) data);
      break;

    case SMALLINT_UNSIGNED:
      result = *((unsigned short *) data);
      break;

    case INT_UNSIGNED:
      result = *((int *) data);
      break;

    default:
      throw UDRException(38902,
                         "TypeInfo::getNumericValue() not supported for SQL type %d",
                         d_.sqlType_);
      break;
    }

  return result;
}

bool TypeInfo::canGetDouble() const
{
  return (d_.dataOffset_ >= 0 &&
          getSQLTypeSubClass() == APPROXIMATE_NUMERIC_TYPE);
}

double TypeInfo::getDouble(const char *row, bool &wasNull) const
{
  if (row == NULL)
    throw UDRException(
         38900,
         "Row not available for getDouble()");

  if (d_.dataOffset_ < 0)
    throw UDRException(
         38900,
         "Offset for column not set, getDouble() method not available");

  if (d_.nullIndOffset_ >= 0 &&
      *((short *) (row + d_.nullIndOffset_)) != 0)
    {
      wasNull = true;
      return 0.0;
    }
              
  double result = 0.0;
  const char *data = row + d_.dataOffset_;

  wasNull = false;

  switch (d_.sqlType_)
    {
    case REAL:
      result = *((float *) data);
      break;

    case DOUBLE_PRECISION:
      result = *((double *) data);
      break;

    default:
      throw UDRException(38900,
                         "getDouble() not supported for SQL type %d",
                         d_.sqlType_);
      break;
    }

  return result;
}

bool TypeInfo::canGetString() const
{
  SQLTYPE_CLASS_CODE typeClass = getSQLTypeClass();

  return (d_.dataOffset_ >= 0 &&
          (typeClass == CHARACTER_TYPE ||
           typeClass == INTERVAL_TYPE));
  // handle numeric and interval types later
}

const char * TypeInfo::getRaw(const char *row,
                              bool &wasNull,
                              int &byteLen) const
{
  if (row == NULL)
    throw UDRException(
         38900,
         "Row not available for getRaw()");

  if (d_.dataOffset_ < 0)
    throw UDRException(
         38900,
         "Offset for column not set, getRaw() method not available");

  if (d_.nullIndOffset_ >= 0 &&
      *((short *) (row + d_.nullIndOffset_)) != 0)
    {
      wasNull = true;
      byteLen = 0;
      return NULL;
    }
              
  const char *result = row + d_.dataOffset_;

  wasNull = false;

  switch (d_.sqlType_)
    {
    case VARCHAR:
      if (d_.flags_ & TYPE_FLAG_4_BYTE_VC_LEN)
        {
          const int32_t *vcLen4 =
            reinterpret_cast<const int32_t *>(row + d_.vcLenIndOffset_);

          byteLen = *vcLen4;
        }
      else
        {
          const int16_t *vcLen2 =
            reinterpret_cast<const int16_t *>(row + d_.vcLenIndOffset_);

          byteLen = *vcLen2;
        }
      break;

    case UNDEFINED_SQL_TYPE:
      throw UDRException(38900,
                         "getString() not supported for SQL type %d",
                         d_.sqlType_);
      break;

    default:
      byteLen = d_.length_;
      if (d_.sqlType_ == CHAR)
        // trim trailing blanks from the value
        while (byteLen > 0 && result[byteLen-1] == ' ')
          byteLen--;
      break;
    }

  return result;
}

void TypeInfo::setInt(int val, char *row) const
{
  setLong(val, row);
}

void TypeInfo::setLong(long val, char *row) const
{
  if (row == NULL ||
      d_.dataOffset_ < 0)
    throw UDRException(38900, "setInt() or setLong() on a non-existent value");

  // set NULL indicator to 0
  if (d_.nullIndOffset_ >= 0)
    *(reinterpret_cast<short *>(row + d_.nullIndOffset_)) = 0;

  int tempSQLType = d_.sqlType_;
  const char *data = row + d_.dataOffset_;

  // convert NUMERIC to the corresponding type with binary precision
  if (d_.sqlType_ == NUMERIC || d_.sqlType_ == NUMERIC_UNSIGNED)
    {
      if (d_.precision_ <= 4)
        if (d_.sqlType_ == NUMERIC)
          tempSQLType = SMALLINT;
        else
          tempSQLType = SMALLINT_UNSIGNED;
      else if (d_.precision_ <= 9)
        if (d_.sqlType_ == NUMERIC)
          tempSQLType = INT;
        else
          tempSQLType = INT_UNSIGNED;
      else if (d_.precision_ <= 18)
        if (d_.sqlType_ == NUMERIC)
          tempSQLType = LARGEINT;
        // else unsigned 8 byte integer is not supported
    }

  switch (tempSQLType)
    {
    case SMALLINT:
      *((short *) data) = val;
      break;

    case INT:
      *((int *) data) = val;
      break;

    case LARGEINT:
      *((long *) data) = val;
      break;

    case SMALLINT_UNSIGNED:
      *((unsigned short *) data) = val;
      break;

    case INT_UNSIGNED:
      *((int *) data) = val;
      break;

    case REAL:
    case DOUBLE_PRECISION:
      setDouble(val, row);
      break;

    default:
      throw UDRException(38900,
                         "setLong(), setInt() or related is not supported for data type %d",
                         d_.sqlType_);
    }
}

void TypeInfo::setDouble(double val, char *row) const
{
  if (row == NULL ||
      d_.dataOffset_ < 0)
    throw UDRException(38900, "setDouble() on a non-existent value");

  // set NULL indicator to 0
  if (d_.nullIndOffset_ >= 0)
    *(reinterpret_cast<short *>(row + d_.nullIndOffset_)) = 0;

  const char *data = row + d_.dataOffset_;

  switch (d_.sqlType_)
    {
    case REAL:
      *(reinterpret_cast<float *>(row + d_.dataOffset_)) = val;
      break;

    case DOUBLE_PRECISION:
      *(reinterpret_cast<double *>(row + d_.dataOffset_)) = val;
      break;

    default:
      throw UDRException(38900,
                         "setDouble() is not supported for data type %d",
                         d_.sqlType_);
    }
}

void TypeInfo::setString(const char *val, int stringLen, char *row) const
{
  if (row == NULL ||
      d_.dataOffset_ < 0)
    throw UDRException(38900, "setString() on a non-existent value");

  // set NULL indicator, a stringLen of 0 and a NULL val ptr is interpreted
  // as NULL, everything else as non-null
  if (d_.nullIndOffset_ >= 0)
    if (val == NULL)
      {
        if (stringLen > 0)
          throw UDRException(
               38900,
               "setString with NULL string and string len > 0");
        setNull(row);
        return;
      }
    else
      *(reinterpret_cast<short *>(row + d_.nullIndOffset_)) = 0;

  char *data = row + d_.dataOffset_;
  bool isApproxNumeric = false;

  // a string overflow will raise an exception
  if (stringLen > d_.length_)
    throw UDRException(38900,
                       "setString() with a string of length %d on a column with length %d",
                       stringLen,
                       d_.length_);

  switch (d_.sqlType_)
    {
    case CHAR:
    case VARCHAR:
    case DATE:
    case TIME:
    case TIMESTAMP:
      // for both CHAR and VARCHAR, copy the string and pad with blanks
      memcpy(data, val, stringLen);
      if (stringLen < d_.length_)
        memset(data+stringLen, ' ', d_.length_ - stringLen);

      if (d_.sqlType_ == VARCHAR)
        {
          // set the varchar length indicator
          if (d_.vcLenIndOffset_ < 0)
            throw UDRException(38900,
                               "Internal error, VARCHAR without length indicator");

          if (d_.flags_ & TYPE_FLAG_4_BYTE_VC_LEN)
            *(reinterpret_cast<int32_t *>(row + d_.vcLenIndOffset_)) =
              stringLen;
          else
            *(reinterpret_cast<int16_t *>(row + d_.vcLenIndOffset_)) =
              stringLen;
        }
      break;

    case REAL:
    case DOUBLE_PRECISION:
      isApproxNumeric = true;
      // fall through to next case

    case SMALLINT:
    case INT:
    case LARGEINT:
    case NUMERIC:
    case DECIMAL_LSE:
    case SMALLINT_UNSIGNED:
    case INT_UNSIGNED:
    case NUMERIC_UNSIGNED:
    case DECIMAL_UNSIGNED:
      {
        char buf[200];
        int valLen = (stringLen >= sizeof(buf) ? sizeof(buf)-1 : stringLen);
        long lval;
        double dval;
        int numCharsConsumed = 0;
        int rc = 0;

        // copy the value to be able to add a terminating NUL byte
        memcpy(buf, val, valLen);
        buf[valLen] = 0;

        if (isApproxNumeric)
          rc = sscanf(buf,"%lf%n", &dval, &numCharsConsumed) < 0;
        else
          rc = sscanf(buf,"%ld%n", &lval, &numCharsConsumed) < 0;

        if (rc < 0)
          throw UDRException(
               38900,
               "Error in setString(), \"%s\" is not a numeric value",
               buf);

        // check for any non-white space left after conversion
        while (numCharsConsumed < valLen)
          if (buf[numCharsConsumed] != ' ' &&
              buf[numCharsConsumed] != '\t')
            throw UDRException(
                 38900,
                 "Found non-numeric character in setString for a numeric column: %s",
                 buf);
          else
            numCharsConsumed++;

        if (isApproxNumeric)
          setDouble(dval, row);
        else
          setLong(lval, row);
      }
      break;

    case UNDEFINED_SQL_TYPE:
    case INTERVAL:
    default:
      throw UDRException(38900,
                         "setString() is not yet supported for data type %d",
                         d_.sqlType_);
    }
}

void TypeInfo::setNull(char *row) const
{
  if (row == NULL ||
      d_.dataOffset_ < 0)
    throw UDRException(38900, "setNull() on a non-existent value");

  // set NULL indicator to -1
  if (d_.nullIndOffset_ >= 0)
    *(reinterpret_cast<short *>(row + d_.nullIndOffset_)) = -1;
  else
    throw UDRException(38900,
                       "Trying to set a non-nullable value to NULL");
}

void TypeInfo::toString(std::string &s, bool longForm) const
{
  char buf[100];

  switch (d_.sqlType_)
    {
    case UNDEFINED_SQL_TYPE:
      s += "undefined_sql_type";
      break;
    case SMALLINT:
      s += "SMALLINT";
      break;
    case INT:
      s += "INT";
      break;
    case LARGEINT:
      s += "LARGEINT";
      break;
    case NUMERIC:
      snprintf(buf, sizeof(buf), "NUMERIC(%d,%d)",
               getScale(), getPrecision());
      s += buf;
      break;
    case DECIMAL_LSE:
      snprintf(buf, sizeof(buf), "DECIMAL(%d,%d)",
               getScale(), getPrecision());
      s += buf;
      break;
    case SMALLINT_UNSIGNED:
      s += "SMALLINT UNSIGNED";
      break;
    case INT_UNSIGNED:
      s += "INT UNSIGNED";
      break;
    case NUMERIC_UNSIGNED:
      snprintf(buf, sizeof(buf), "NUMERIC(%d,%d) UNSIGNED",
               getScale(), getPrecision());
      s += buf;
      break;
    case DECIMAL_UNSIGNED:
      snprintf(buf, sizeof(buf), "DECIMAL(%d,%d) UNSIGNED",
               getScale(), getPrecision());
      s += buf;
      break;
    case REAL:
      s += "REAL";
      break;
    case DOUBLE_PRECISION:
      s += "DOUBLE PRECISION";
      break;
    case CHAR:
    case VARCHAR:
      const char *csName;

      switch(getCharset())
        {
        case UNDEFINED_CHARSET:
          csName = "undefined";
          break;
        case CHARSET_ISO88591:
          csName = "ISO88591";
          break;
        case CHARSET_UTF8:
          csName = "UTF8";
          break;
        case CHARSET_UCS2:
          csName = "UCS2";
          break;
        default:
          csName = "invalid charset!";
          break;
        }

      snprintf(buf, sizeof(buf), "%s(%d%s) CHARACTER SET %s",
               (d_.sqlType_ == CHAR ? "CHAR" : "VARCHAR"),
               getLength(),
               (getCharset() == CHARSET_UTF8 ? " BYTES" : ""),
               csName);
      s += buf;
      break;
    case DATE:
      s += "DATE";
      break;
    case TIME:
      s += "TIME";
      break;
    case TIMESTAMP:
      snprintf(buf, sizeof(buf), "TIMESTAMP(%d)",
               getPrecision());
      s += buf;
      break;
    case INTERVAL:
      switch (d_.intervalCode_)
        {
        case UNDEFINED_INTERVAL_CODE:
          snprintf(buf, sizeof(buf), "INTERVAL with undefined subtype!");
          break;
        case INTERVAL_YEAR:
          snprintf(buf, sizeof(buf), "INTERVAL YEAR(%d)",
                   getPrecision());
          break;
        case INTERVAL_MONTH:
          snprintf(buf, sizeof(buf), "INTERVAL MONTH(%d)",
                   getPrecision());
          break;
        case INTERVAL_DAY:
          snprintf(buf, sizeof(buf), "INTERVAL DAY(%d)",
                   getPrecision());
          break;
        case INTERVAL_HOUR:
          snprintf(buf, sizeof(buf), "INTERVAL HOUR(%d)",
                   getPrecision());
          break;
        case INTERVAL_MINUTE:
          snprintf(buf, sizeof(buf), "INTERVAL MINUTE(%d)",
                   getPrecision());
          break;
        case INTERVAL_SECOND:
          snprintf(buf, sizeof(buf), "INTERVAL SECOND(%d,%d)",
                   getPrecision(),
                   getScale());
          break;
        case INTERVAL_YEAR_MONTH:
          snprintf(buf, sizeof(buf), "INTERVAL YEAR(%d) TO MONTH",
                   getPrecision());
          break;
        case INTERVAL_DAY_HOUR:
          snprintf(buf, sizeof(buf), "INTERVAL DAY(%d) TO HOUR",
                   getPrecision());
          break;
        case INTERVAL_DAY_MINUTE:
          snprintf(buf, sizeof(buf), "INTERVAL DAY(%d) TO MINUTE",
                   getPrecision());
          break;
        case INTERVAL_DAY_SECOND:
          snprintf(buf, sizeof(buf), "INTERVAL DAY(%d) TO SECOND(%d)",
                   getPrecision(),
                   getScale());
          break;
        case INTERVAL_HOUR_MINUTE:
          snprintf(buf, sizeof(buf), "INTERVAL HOUR(%d) TO MINUTE",
                   getPrecision());
          break;
        case INTERVAL_HOUR_SECOND:
          snprintf(buf, sizeof(buf), "INTERVAL HOUR(%d) TO SECOND(%d)",
                   getPrecision(),
                   getScale());
          break;
        case INTERVAL_MINUTE_SECOND:
          snprintf(buf, sizeof(buf), "INTERVAL MINUTE(%d) TO SECOND(%d)",
                   getPrecision(),
                   getScale());
          break;
        default:
          snprintf(buf, sizeof(buf), "invalid interval code!");
        }
      s += buf;
      break;
    default:
      s += "invalid SQL type!";
      break;
    }

  if (!d_.nullable_)
    s += " NOT NULL";

  if (longForm && d_.dataOffset_ >= 0)
    {
      snprintf(buf, sizeof(buf), " offsets: (nullInd=%d, vcLen=%d, data=%d)",
               d_.nullIndOffset_, d_.vcLenIndOffset_, d_.dataOffset_);
      s += buf;
    }
}

int TypeInfo::serializedLength()
{
  // format is base class bytes + binary image of d_
  return TMUDRSerializableObject::serializedLength() +
    serializedLengthOfBinary(sizeof(d_));
}

int TypeInfo::serialize(Bytes &outputBuffer,
                        int &outputBufferLength)
{
  int result = 
    TMUDRSerializableObject::serialize(outputBuffer,
                                       outputBufferLength);

  result += serializeBinary(&d_,
                            sizeof(d_),
                            outputBuffer,
                            outputBufferLength);
  validateSerializedLength(result);

  return result;
}

int TypeInfo::deserialize(Bytes &inputBuffer,
                          int &inputBufferLength)
{
  int result =
    TMUDRSerializableObject::deserialize(inputBuffer, inputBufferLength);
  validateObjectType(TYPE_INFO_OBJ);

  int binarySize = 0;
  const void *temp = NULL;

  result += deserializeBinary(&temp,
                              binarySize,
                              false,
                              inputBuffer,
                              inputBufferLength);
  if (binarySize != sizeof(d_))
    throw UDRException(38900,"Expected %d bytes to deserialize TypeInfo struct, actually used %d bytes",
                       sizeof(d_),
                       binarySize);
  memcpy(&d_, temp, binarySize);

  validateDeserializedLength(result);

  return result;
}

void TypeInfo::setOffsets(int indOffset, int vcOffset, int dataOffset)
{
  d_.nullIndOffset_  = indOffset;
  d_.vcLenIndOffset_ = vcOffset;
  d_.dataOffset_     = dataOffset;
}


// ------------------------------------------------------------------------
// Member functions for class ProvenanceInfo
// ------------------------------------------------------------------------

ProvenanceInfo::ProvenanceInfo() :
     inputTableNum_(-1),
     inputColumnNum_(-1)
{}

ProvenanceInfo::ProvenanceInfo(int inputTableNum,
                               int inputColumnNum) :
     inputTableNum_(inputTableNum),
     inputColumnNum_(inputColumnNum)
{}

int ProvenanceInfo::getInputTableNum() const
{
  return inputTableNum_;
}

int ProvenanceInfo::getInputColumnNum() const
{
  return inputColumnNum_;
}

bool ProvenanceInfo::isFromInputTable() const
{
  return inputTableNum_ >= 0 && inputColumnNum_ >= 0;
}

// ------------------------------------------------------------------------
// Member functions for class ColumnInfo
// ------------------------------------------------------------------------

ColumnInfo::ColumnInfo() :
     TMUDRSerializableObject(COLUMN_INFO_OBJ,
                             getCurrentVersion()),
     usage_(USED),
     uniqueEntries_(-1)
{}

ColumnInfo::ColumnInfo(const char *name,
                       const TypeInfo &type,
                       COLUMN_USE usage,
                       long uniqueEntries) :
     TMUDRSerializableObject(COLUMN_INFO_OBJ,
                             getCurrentVersion()),
     name_(name),
     type_(type),
     usage_(usage),
     uniqueEntries_(uniqueEntries)
{}
  
const std::string &ColumnInfo::getColName() const
{
  return name_;
}

const TypeInfo &ColumnInfo::getType() const
{
  return type_;
}

TypeInfo & ColumnInfo::getType()
{
  return type_;
}

long ColumnInfo::getUniqueEntries() const
{
  return uniqueEntries_;
}

ColumnInfo::COLUMN_USE ColumnInfo::getUsage() const        {
  return usage_;
}

const ProvenanceInfo &ColumnInfo::getProvenance() const
{
  return provenance_;
}

void ColumnInfo::setColName(const char *name)
{
  name_ = name;
}

void ColumnInfo::setType(TypeInfo &type)
{
  type_ = type;
}

void ColumnInfo::setUniqueEntries(long uniqueEntries)
{
  uniqueEntries_ = uniqueEntries;
}

void ColumnInfo::setUsage(COLUMN_USE usage)
{
  usage_ = usage;
}

void ColumnInfo::setProvenance(const ProvenanceInfo &provenance)
{
  provenance_ = provenance;
}

void ColumnInfo::toString(std::string &s, bool longForm) const
{
  s += name_;
  if (longForm)
    {
      s += " ";
      type_.toString(s, longForm);
    }
}

int ColumnInfo::serializedLength()
{
  // format: base class + name + type + int(usage) + long(uec) +
  //         int(input table #) + int(input col #)
  return TMUDRSerializableObject::serializedLength() +
    serializedLengthOfString(name_) +
    type_.serializedLength() +
    3 * serializedLengthOfInt() +
    serializedLengthOfLong();
}

int ColumnInfo::serialize(Bytes &outputBuffer,
                          int &outputBufferLength)
{
  int result = 
    TMUDRSerializableObject::serialize(outputBuffer,
                                       outputBufferLength);

  result += serializeString(name_,
                            outputBuffer,
                            outputBufferLength);

  result += type_.serialize(outputBuffer,
                            outputBufferLength);

  result += serializeInt(static_cast<int>(usage_),
                         outputBuffer,
                         outputBufferLength);

  result += serializeLong(uniqueEntries_,
                          outputBuffer,
                          outputBufferLength);

  result += serializeInt(getProvenance().getInputTableNum(),
                         outputBuffer,
                         outputBufferLength);

  result += serializeInt(getProvenance().getInputColumnNum(),
                         outputBuffer,
                         outputBufferLength);

  validateSerializedLength(result);

  return result;
}

int ColumnInfo::deserialize(Bytes &inputBuffer,
                            int &inputBufferLength)
{
  int result =
    TMUDRSerializableObject::deserialize(inputBuffer, inputBufferLength);
  int tempInt1 = 0;
  int tempInt2 = 0;

  validateObjectType(COLUMN_INFO_OBJ);

  result += deserializeString(name_,
                      inputBuffer,
                      inputBufferLength);

  result += type_.deserialize(inputBuffer,
                              inputBufferLength);

  result += deserializeInt(tempInt1,
                   inputBuffer,
                   inputBufferLength);
  usage_ = static_cast<COLUMN_USE>(tempInt1);

  result += deserializeLong(uniqueEntries_,
                    inputBuffer,
                    inputBufferLength);

  result += deserializeInt(tempInt1,
                   inputBuffer,
                   inputBufferLength);
  result += deserializeInt(tempInt2,
                   inputBuffer,
                   inputBufferLength);
  setProvenance(ProvenanceInfo(tempInt1, tempInt2));

  validateDeserializedLength(result);

  return result;
}


// ------------------------------------------------------------------------
// Member functions for class ConstraintInfo
// ------------------------------------------------------------------------

int ConstraintInfo::serializedLength()
{
  return TMUDRSerializableObject::serializedLength();
}

int ConstraintInfo::serialize(Bytes &outputBuffer,
                              int &outputBufferLength)
{
  int result = 
    TMUDRSerializableObject::serialize(outputBuffer,
                                       outputBufferLength);
  if (getObjectType() == CONSTRAINT_INFO_OBJ)
    validateSerializedLength(result);

  return result;
}

int ConstraintInfo::deserialize(Bytes &inputBuffer,
                                int &inputBufferLength)
{
  int result =
    TMUDRSerializableObject::deserialize(inputBuffer, inputBufferLength);

  if (getObjectType() == CONSTRAINT_INFO_OBJ)
    validateDeserializedLength(result);

  return result;
}

// ------------------------------------------------------------------------
// Member functions for class PredicateInfo
// ------------------------------------------------------------------------

int PredicateInfo::serializedLength()
{
  return TMUDRSerializableObject::serializedLength();
}

int PredicateInfo::serialize(Bytes &outputBuffer,
                             int &outputBufferLength)
{
  int result = 
    TMUDRSerializableObject::serialize(outputBuffer,
                                       outputBufferLength);
  validateSerializedLength(result);

  return result;
}

int PredicateInfo::deserialize(Bytes &inputBuffer,
                               int &inputBufferLength)
{
  int result =
    TMUDRSerializableObject::deserialize(inputBuffer, inputBufferLength);

  validateObjectType(PREDICATE_INFO_OBJ);
  validateDeserializedLength(result);

  return result;
}

// ------------------------------------------------------------------------
// Member functions for class PartitionInfo
// ------------------------------------------------------------------------

PartitionInfo::PartitionInfo() : type_(UNKNOWN)
{}

PartitionInfo::PARTITION_TYPE PartitionInfo::getType() const
{
  return type_;
}


// ------------------------------------------------------------------------
// Member functions for class OrderInfo
// ------------------------------------------------------------------------

int OrderInfo::getNumEntries() const
{
  return columnNumbers_.size();
}

int OrderInfo::getColumnNum(int i) const
{
  return columnNumbers_[i];
}

ORDER_TYPE OrderInfo::getOrderType(int i) const
{
  return orderTypes_[i];
}

void OrderInfo::addEntry(int colNum, ORDER_TYPE orderType)
{
  columnNumbers_.push_back(colNum);
  orderTypes_.push_back(orderType);
}

void OrderInfo::addEntryAt(int pos,
                           int colNum,
                           ORDER_TYPE orderType)
{
  columnNumbers_.insert(columnNumbers_.begin() + pos, colNum);
  orderTypes_.insert(orderTypes_.begin() + pos, orderType);
}

// ------------------------------------------------------------------------
// Member functions for class TupleInfo
// ------------------------------------------------------------------------

TupleInfo::TupleInfo(TMUDRObjectType objType, int version) :
     TMUDRSerializableObject(objType, version),
     recordLength_(-1),
     rowPtr_(NULL),
     wasNull_(false)
{}

TupleInfo::~TupleInfo()
{
  // delete all columns
  for (std::vector<ColumnInfo *>::iterator it1 = columns_.begin();
       it1 != columns_.end();
       it1++)
    delete *it1;

  // rowPtr_ is not owned by this object
}

int TupleInfo::getNumColumns() const
{
  return columns_.size();
}

int TupleInfo::getColNum(const char *colName) const
{
  int result = 0;
  std::vector<ColumnInfo *>::const_iterator it = columns_.begin();

  for (; it != columns_.end(); it++, result++)
    if ((*it)->getColName() == colName)
      return result;

  throw UDRException(38900, "Column %s not found", colName);
}

int TupleInfo::getColNum(const std::string &colName) const
{
  return getColNum(colName.c_str());
}

const ColumnInfo &TupleInfo::getColumn(int i) const
{
  return *(columns_[i]);
}

const ColumnInfo &TupleInfo::getColumn(const std::string &name) const
{
  return getColumn(getColNum(name));
}

ColumnInfo &TupleInfo::getColumn(int i)
{
  return *(columns_[i]);
}

ColumnInfo &TupleInfo::getColumn(const std::string &name)
{
  return getColumn(getColNum(name));
}

void TupleInfo::addColumn(const ColumnInfo &column)
{
  ColumnInfo *newCol = new ColumnInfo(column);

  columns_.push_back(newCol);
}

bool TupleInfo::canGetInt(int colNum) const
{
  return (rowPtr_ != NULL &&
          columns_[colNum]->getType().canGetInt());
}

int TupleInfo::getInt(int colNum) const
{
  bool &nonConstWasNull = const_cast<TupleInfo *>(this)->wasNull_;

  nonConstWasNull = false;

  return columns_[colNum]->getType().getInt(rowPtr_, nonConstWasNull);
}

int TupleInfo::getInt(const std::string &colName) const
{
  return getInt(getColNum(colName));
}

bool TupleInfo::canGetLong(int colNum) const
{
  return (rowPtr_ != NULL &&
          columns_[colNum]->getType().canGetLong());
}

long TupleInfo::getLong(int colNum) const
{
  bool &nonConstWasNull = const_cast<TupleInfo *>(this)->wasNull_;

  nonConstWasNull = false;

  return columns_[colNum]->getType().getLong(rowPtr_, nonConstWasNull);
}

long TupleInfo::getLong(const std::string &colName) const
{
  return getLong(getColNum(colName));
}

bool TupleInfo::canGetDouble(int colNum) const
{
  return (rowPtr_ != NULL &&
          columns_[colNum]->getType().canGetDouble());
}

double TupleInfo::getDouble(int colNum) const
{
  bool &nonConstWasNull = const_cast<TupleInfo *>(this)->wasNull_;

  nonConstWasNull = false;

  return columns_[colNum]->getType().getDouble(rowPtr_, nonConstWasNull);
}

double TupleInfo::getDouble(const std::string &colName) const
{
  return getDouble(getColNum(colName));
}

bool TupleInfo::canGetString(int colNum) const
{
  return (rowPtr_ != NULL &&
          columns_[colNum]->getType().canGetString());
}

const char * TupleInfo::getRaw(int colNum, int &byteLen) const
{
  bool &nonConstWasNull = const_cast<TupleInfo *>(this)->wasNull_;

  nonConstWasNull = false;

  return columns_[colNum]->getType().getRaw(rowPtr_,
                                            nonConstWasNull,
                                            byteLen);
}

void TupleInfo::getDelimitedRow(std::string &row,
                                char delim,
                                bool quote,
                                char quoteSymbol) const
{
  // read all columns and form a delimited text row from them

  // if quote is true, then quote any text that contains the delimiter
  // and also double any quotes appearing in the text 
  int nc = getNumColumns();

  row.erase();

  for (int i=0; i<nc; i++)
    {
      std::string val=getString(i);

      if (i>0)
        row.push_back(delim);

      if (!wasNull())
        {
          if (quote)
            {
              bool quoteTheString=false;

              // replace all quotes with two quotes
              for (std::string::iterator it = val.begin();
                   it != val.end();
                   it++)
                if (*it == quoteSymbol)
                  {
                    quoteTheString = true;
                    it++;
                    val.insert(it,quoteSymbol);
                  }
                else if (*it == delim)
                  quoteTheString = true;

              // if we found a quote or a delimiter in the
              // string, then quote it
              if (quoteTheString)
                {
                  val.insert(0,1,quoteSymbol);
                  val.push_back(quoteSymbol);
                }
            } // quote

          row += val;
        } // value is not NULL
    } // loop over columns
}

std::string TupleInfo::getString(int colNum) const
{
  int stringLen = 0;
  TypeInfo::SQLTYPE_CODE sqlType = columns_[colNum]->getType().getSQLType();

  switch (sqlType)
    {
    case TypeInfo::CHAR:
    case TypeInfo::VARCHAR:
    case TypeInfo::DATE:
    case TypeInfo::TIME:
    case TypeInfo::TIMESTAMP:
      {
        // these types are stored as strings
        const char *buf = getRaw(colNum, stringLen);
        if (buf)
          return std::string(buf, stringLen);
        else
          return std::string("");
      }


    case TypeInfo::SMALLINT:
    case TypeInfo::INT:
    case TypeInfo::LARGEINT:
    case TypeInfo::NUMERIC:
    case TypeInfo::SMALLINT_UNSIGNED:
    case TypeInfo::INT_UNSIGNED:
    case TypeInfo::NUMERIC_UNSIGNED:
      {
        char buf[32];
        long num = getLong(colNum);

        snprintf(buf, sizeof(buf), "%ld", num);

        return buf;
      }

    case TypeInfo::REAL:
    case TypeInfo::DOUBLE_PRECISION:
      {
        char buf[32];
        double num = getDouble(colNum);
        // see also constants SQL_FLOAT_FRAG_DIGITS and
        // SQL_DOUBLE_PRECISION_FRAG_DIGITS in file 
        // trafodion/core/sql/common/SQLTypeDefs.h
        int numSignificantDigits = 17;
        if (sqlType == TypeInfo::REAL)
          numSignificantDigits = 7;

        snprintf(buf, sizeof(buf), "%*g", numSignificantDigits, num);

        return buf;
      }
    case TypeInfo::DECIMAL_LSE:
    case TypeInfo::DECIMAL_UNSIGNED:
    case TypeInfo::INTERVAL:
    default:
      throw UDRException(
           38900,
           "Type %d not yet supported in getString()",
           sqlType);
    }

}

std::string TupleInfo::getString(const std::string &colName) const
{
  return getString(getColNum(colName));
}

bool TupleInfo::wasNull() const
{
  return wasNull_;
}

void TupleInfo::setInt(int colNum, int val) const
{
  columns_[colNum]->getType().setInt(val, rowPtr_);
}

void TupleInfo::setLong(int colNum, long val) const
{
  columns_[colNum]->getType().setLong(val, rowPtr_);
}

void TupleInfo::setDouble(int colNum, double val) const
{
  columns_[colNum]->getType().setDouble(val, rowPtr_);
}

void TupleInfo::setString(int colNum, const char *val) const
{
  setString(colNum, val, strlen(val));
}

void TupleInfo::setString(int colNum, const char *val, int stringLen) const
{
  columns_[colNum]->getType().setString(val, stringLen, rowPtr_);
}

void TupleInfo::setString(int colNum, const std::string &val) const
{
  setString(colNum, val.data(), val.size());
}

const char * TupleInfo::setFromDelimitedRow(const char *row,
                                            char delim,
                                            bool quote,
                                            char quoteSymbol) const
{
  int nc = getNumColumns();
  const char *c = row;

  for (int i=0; i<nc; i++)
    {
      // skip over whitespace
      while (*c == ' ' || *c == '\t') c++;

      // make sure we have a delimiter for columns other than the first
      if (i>0)
        {
          if (*c != delim)
            throw UDRException(
                 38900,
                 "Expected delimiter at position %d in string %s",
                 c-row, row);

          // skip over the delimiter and white space
          c++;
          while (*c == ' ' || *c == '\t') c++;
        }

      // find the end of the column value
      const char *endOfVal = c;

      if (quote && *c == quoteSymbol)
        {
          // read and set a quoted string
          bool embeddedQuote = false;
          bool done = false;

          endOfVal = ++c;

          // find the matching end to the quote
          while (*endOfVal != 0 && !done)
            if (*endOfVal == quoteSymbol)
              if (endOfVal[1] == quoteSymbol)
                {
                  // skip over both quotes
                  embeddedQuote = true;
                  endOfVal += 2;
                }
              else
                // found the terminating quote
                done = true;
            else
              endOfVal++;

          if (!done)
            throw UDRException(
                 38900,
                 "missing quote at the end of column %d in string %s",
                 i, row);

          if (embeddedQuote)
            {
              // need to transform the double doublequotes
              // in a separate buffer
              std::string unquotedVal(c, (endOfVal-c));
              std::string::iterator it = unquotedVal.begin();

              while (it != unquotedVal.end())
                if (*it == quoteSymbol)
                  it = unquotedVal.erase(it);
                else
                  it++;
              // set from the transformed string
              setString(i, unquotedVal);
            }
          else
            {
              // set from the value between the quotes
              setString(i, c, (endOfVal-c));
              // skip over the trailing quote
              endOfVal++;
            }

        }
      else
        {
          // c points to the beginning of the field value
          // find the next delimiter or the end of the
          // record and treat white space only as a NULL
          // value
          bool isNull = true;

          while (*endOfVal != 0 && *endOfVal != delim)
            {
              if (isNull && *endOfVal != ' ' && *endOfVal != '\t')
                isNull = false;

              endOfVal++;
            }

          if (isNull)
            setNull(i);
          else
            setString(i, c, (endOfVal-c));
        }

      // set the current character pointer to the
      // character just past of what we have consumed
      c = endOfVal;
    }

  return c;
}

void TupleInfo::setNull(int colNum) const
{
  columns_[colNum]->getType().setNull(rowPtr_);
}

void TupleInfo::addIntegerColumn(const char *colName, bool isNullable)
{
  addColumn(ColumnInfo(colName, TypeInfo(TypeInfo::INT32,0,isNullable)));
}

void TupleInfo::addLongColumn(const char *colName, bool isNullable)
{
  addColumn(ColumnInfo(colName, TypeInfo(TypeInfo::INT64,0,isNullable)));
}

void TupleInfo::addCharColumn(const char *colName,
                              int length,
                              bool isNullable,
                              TypeInfo::SQLCHARSET_CODE charset,
                              TypeInfo::SQLCOLLATION_CODE collation)
{
  addColumn(ColumnInfo(colName, TypeInfo(TypeInfo::CHAR,
                                         length,
                                         isNullable,
                                         0,
                                         charset,
                                         TypeInfo::UNDEFINED_INTERVAL_CODE,
                                         0,
                                         collation)));
}

void TupleInfo::addVarCharColumn(const char *colName,
                                 int length,
                                 bool isNullable,
                                 TypeInfo::SQLCHARSET_CODE charset,
                                 TypeInfo::SQLCOLLATION_CODE collation)
{
  addColumn(ColumnInfo(colName, TypeInfo(TypeInfo::VARCHAR,
                                         length,
                                         isNullable,
                                         0,
                                         charset,
                                         TypeInfo::UNDEFINED_INTERVAL_CODE,
                                         0,
                                         collation)));
}

void TupleInfo::addColumns(const std::vector<ColumnInfo *> &columns)
{
  for (std::vector<ColumnInfo *>::const_iterator it = columns.begin();
       it != columns.end();
       it++)
    addColumn(**it);
}

void TupleInfo::addColumnAt(const ColumnInfo &column, int position)
{
  ColumnInfo *newCol = new ColumnInfo(column);

  columns_.insert(columns_.begin() + position, newCol);
}

void TupleInfo::deleteColumn(int i)
{
  std::vector<ColumnInfo *>::iterator it = columns_.begin() + i;

  if (it != columns_.end())
    {
      delete *it;
      columns_.erase(it);
    }
  else
    throw UDRException(38906, "Column number %d not found", i);
}

void TupleInfo::deleteColumn(const std::string &name)
{
  deleteColumn(getColNum(name));
}

void TupleInfo::print()
{
  printf("    Number of columns        : %d\n", getNumColumns());
  printf("    Columns                  : \n");
  for (int c=0; c<getNumColumns(); c++)
    {
      std::string colString;

      getColumn(c).toString(colString, true);
      printf("        %s\n", colString.c_str());
    }
  if (recordLength_ >= 0)
    printf("    Record length            : %d\n", recordLength_);
}

int TupleInfo::serializedLength()
{
  // format: base class + int(#cols) + n*ColumnInfo + int(recordLength_)
  // rowPtr_ is not serialized
  int result = TMUDRSerializableObject::serializedLength() +
    2 * serializedLengthOfInt();

  for (int c=0; c<getNumColumns(); c++)
    result += getColumn(c).serializedLength();

  return result;
}

int TupleInfo::serialize(Bytes &outputBuffer,
                         int &outputBufferLength)
{
  int result = 
    TMUDRSerializableObject::serialize(outputBuffer,
                                       outputBufferLength);

  result += serializeInt(getNumColumns(),
                         outputBuffer,
                         outputBufferLength);

  for (int c=0; c<getNumColumns(); c++)
    result += getColumn(c).serialize(outputBuffer,
                                     outputBufferLength);

  result += serializeInt(recordLength_,
                         outputBuffer,
                         outputBufferLength);

  if (getObjectType() == TUPLE_INFO_OBJ)
    validateSerializedLength(result);

  return result;
}

int TupleInfo::deserialize(Bytes &inputBuffer,
                           int &inputBufferLength)
{
  int result =
    TMUDRSerializableObject::deserialize(inputBuffer, inputBufferLength);

  // Caller needs to validate the object type when they are
  // serializing this class, since derived objects exist, so we
  // can't unconditionally expect a given object type here
  // validateObjectType(TABLE_INFO_OBJ);
  int numCols = 0;

  result += deserializeInt(numCols,
                   inputBuffer,
                   inputBufferLength);

  for (int c=0; c<numCols; c++)
    {
      ColumnInfo ci;

      result += ci.deserialize(inputBuffer,
                               inputBufferLength);
      addColumn(ci);
    }

  result += deserializeInt(recordLength_,
                   inputBuffer,
                   inputBufferLength);

  rowPtr_ = NULL;
  wasNull_ = false;

  if (getObjectType() == TUPLE_INFO_OBJ)
    validateDeserializedLength(result);

  return result;
}

char * TupleInfo::getRowPtr() const
{
  return rowPtr_;
}

int TupleInfo::getRecordLength() const
{
  return recordLength_;
}

void TupleInfo::setRecordLength(int len)
{
  recordLength_ = len;
}

void TupleInfo::setRowPtr(char *ptr)
{
  rowPtr_ = ptr;
}


// ------------------------------------------------------------------------
// Member functions for class TableInfo
// ------------------------------------------------------------------------

TableInfo::TableInfo() :
     TupleInfo(TABLE_INFO_OBJ, getCurrentVersion()),
     numRows_(-1)
{}

TableInfo::~TableInfo()
{
  // delete all constraints
  for (std::vector<ConstraintInfo *>::iterator it2 = constraints_.begin();
       it2 != constraints_.end();
       it2++)
    delete *it2;
}

long TableInfo::getNumRows() const
{
  return numRows_;
}

const PartitionInfo &TableInfo::getQueryPartitioning() const
{
  return queryPartitioning_;
}

const OrderInfo &TableInfo::getQueryOrdering() const
{
  return queryOrdering_;
}

bool TableInfo::isStream() const
{
  return false;
}

int TableInfo::getNumConstraints() const
{
  return constraints_.size();
}

const ConstraintInfo &TableInfo::getConstraint(int i) const
{
  return *(constraints_[i]);
}

ConstraintInfo &TableInfo::getConstraint(int i)
{
  return *(constraints_[i]);
}

void TableInfo::setNumRows(long rows)
{
  numRows_ = rows;
}

void TableInfo::addConstraint(ConstraintInfo &constraint)
{
  ConstraintInfo *newConstr = new ConstraintInfo(constraint);

  constraints_.push_back(newConstr);
}

void TableInfo::setIsStream(bool stream)
{
  if (stream)
    throw UDRException(38908, "Stream tables not yet supported");
}

void TableInfo::print()
{
  TupleInfo::print();
  printf("    Estimated number of rows : %ld\n", getNumRows());
  printf("    Partitioning             : ");
  switch (getQueryPartitioning().getType())
    {
    case PartitionInfo::UNKNOWN:
      printf("unknown\n");
      break;
    case PartitionInfo::SERIAL:
      printf("serial\n");
      break;
    case PartitionInfo::PARTITION:
      {
        bool needsComma = false;
        printf("(");
        for (std::vector<int>::const_iterator it1 = getQueryPartitioning().begin();
             it1 != getQueryPartitioning().end();
             it1++)
          {
            if (needsComma)
              printf(", ");
            printf("%s", getColumn(*it1).getColName().c_str());
            needsComma = true;
          }
        printf(")\n");
      }
      break;
    case PartitionInfo::REPLICATE:
      printf("replicate\n");
      break;
    default:
      printf("invalid partitioning specification!\n");
      break;
    }
  printf("    Ordering                 : ");
  if (getQueryOrdering().getNumEntries() > 0)
    {
      bool needsComma = false;

      printf("(");
      for (int o=0; o<getQueryOrdering().getNumEntries(); o++)
        {
          if (needsComma)
            printf(", ");
          printf("%d", getQueryOrdering().getColumnNum(o));

          ORDER_TYPE ot = getQueryOrdering().getOrderType(o);
          if (ot == DESCENDING)
            printf(" DESC");
          else if (ot != ASCENDING)
            printf(" - invalid order type!");
        }
      printf(")\n");
    }
  else
    printf("none\n");
}

int TableInfo::serializedLength()
{
  // format: base class + long(numRows_) +
  // int(#part cols) + int(#order cols) +
  // binary array of ints:
  // p*int(partkeycol#) +
  // o*(int(ordercol#) + int(ordering))
  int result = TupleInfo::serializedLength() +
    serializedLengthOfLong() +
    2 * serializedLengthOfInt() +
    serializedLengthOfBinary(
         (getQueryPartitioning().size() +
          2 * getQueryOrdering().getNumEntries()) * sizeof(int));

  return result;
}

int TableInfo::serialize(Bytes &outputBuffer,
                         int &outputBufferLength)
{
  int result = 
    TupleInfo::serialize(outputBuffer,
                         outputBufferLength);
  int numPartCols = queryPartitioning_.size();
  int numOrderCols = queryOrdering_.getNumEntries();
  int *intArray = new int[numPartCols + 2*numOrderCols];
  int c;

  result += serializeLong(numRows_,
                          outputBuffer,
                          outputBufferLength);

  result += serializeInt(numPartCols,
                         outputBuffer,
                         outputBufferLength);

  result += serializeInt(numOrderCols,
                         outputBuffer,
                         outputBufferLength);

  for (c=0; c<numPartCols; c++)
    intArray[c] = queryPartitioning_[c];
  for (c=0; c<numOrderCols; c++)
    {
      intArray[numPartCols+2*c] = queryOrdering_.getColumnNum(c);
      intArray[numPartCols+2*c+1] =
        static_cast<int>(queryOrdering_.getOrderType(c));
    }
  result += serializeBinary(
       intArray,
       (numPartCols + 2*numOrderCols) * sizeof(int),
       outputBuffer,
       outputBufferLength);
  delete intArray;

  validateSerializedLength(result);

  return result;
}

int TableInfo::deserialize(Bytes &inputBuffer,
                           int &inputBufferLength)
{
  int result =
    TupleInfo::deserialize(inputBuffer, inputBufferLength);

  validateObjectType(TABLE_INFO_OBJ);
  int numCols = 0;
  int numPartCols = 0;
  int numOrderCols = 0;
  const int *intArray = NULL;
  int binarySize = 0;
  int c;

  result += deserializeLong(numRows_,
                            inputBuffer,
                            inputBufferLength);

  result += deserializeInt(numPartCols,
                           inputBuffer,
                           inputBufferLength);

  result += deserializeInt(numOrderCols,
                           inputBuffer,
                           inputBufferLength);

  result += deserializeBinary((const void **) &intArray,
                              binarySize,
                              false,
                              inputBuffer,
                              inputBufferLength);
  if (binarySize != (numPartCols + 2*numOrderCols) * sizeof(int))
    throw UDRException(38900, "Invalid int array size in TableInfo, got %d, expected %d",
                       binarySize,
                       (numPartCols + 2*numOrderCols) * sizeof(int));
  for (c=0; c<numPartCols; c++)
    queryPartitioning_.push_back(intArray[c]);
  for (c=0; c<numOrderCols; c++)
    queryOrdering_.addEntry(
         intArray[numPartCols+2*c],
         static_cast<ORDER_TYPE>(intArray[numPartCols+2*c+1]));

  validateDeserializedLength(result);

  return result;
}


// ------------------------------------------------------------------------
// Member functions for class ParameterListInfo
// ------------------------------------------------------------------------

ParameterListInfo::ParameterListInfo() :
     TupleInfo(PARAMETER_LIST_INFO_OBJ, getCurrentVersion()),
     constBufferLen_(0),
     constBuffer_(NULL)
{
}

ParameterListInfo::~ParameterListInfo()
{
  if (constBufferLen_)
    delete constBuffer_;
}

int ParameterListInfo::serializedLength()
{
  // format: Base class + int(constBufferLength_) + binary(constBuffer_)
  int result = TupleInfo::serializedLength() +
    serializedLengthOfInt();

  if (constBufferLen_ > 0)
    result += serializedLengthOfBinary(constBufferLen_);

  return result;
}
int ParameterListInfo::serialize(Bytes &outputBuffer,
                                 int &outputBufferLength)
{
  int result = TupleInfo::serialize(outputBuffer,
                                    outputBufferLength);

  result += serializeInt(constBufferLen_,
                         outputBuffer,
                         outputBufferLength);

  if (constBufferLen_)
    result += serializeBinary(
         constBuffer_,
         constBufferLen_,
         outputBuffer,
         outputBufferLength);

  validateSerializedLength(result);

  return result;
}

int ParameterListInfo::deserialize(Bytes &inputBuffer,
                                   int &inputBufferLength)
{
  int result =
    TupleInfo::deserialize(inputBuffer,
                           inputBufferLength);

  validateObjectType(PARAMETER_LIST_INFO_OBJ);

  result += deserializeInt(constBufferLen_,
                           inputBuffer,
                           inputBufferLength);

  if (constBufferLen_ > 0)
    {
      result += deserializeBinary((const void **) &constBuffer_,
                                  constBufferLen_,
                                  true,
                                  inputBuffer,
                                  inputBufferLength);
      // also let rowPtr_ in the base class
      // point to this buffer, so that we can use
      // the deserialize... methods
      rowPtr_ = const_cast<char *>(constBuffer_);
    }
  else
    constBuffer_ = NULL;

  validateDeserializedLength(result);

  return result;
}

void ParameterListInfo::setConstBuffer(int constBufferLen,
                                       const char *constBuffer)
{
  if (constBuffer_ && constBuffer_ != constBuffer)
    delete constBuffer_;

  constBufferLen_ = constBufferLen;
  constBuffer_ = constBuffer;
  rowPtr_ = const_cast<char *>(constBuffer_);
}


// ------------------------------------------------------------------------
// Member functions for class UDRWriterCompileTimeData
// ------------------------------------------------------------------------

UDRWriterCompileTimeData::UDRWriterCompileTimeData()
{}

UDRWriterCompileTimeData::~UDRWriterCompileTimeData()
{}

void UDRWriterCompileTimeData::print()
{
  printf("no print method provided for UDR Writer compile time data\n");
}

// ------------------------------------------------------------------------
// Member functions for class UDRWriterRunTimeData
// ------------------------------------------------------------------------

UDRWriterRunTimeData::UDRWriterRunTimeData()
{}

UDRWriterRunTimeData::~UDRWriterRunTimeData()
{}

void UDRWriterRunTimeData::print()
{
  printf("no print method provided for UDR Writer run time data\n");
}

// ------------------------------------------------------------------------
// Member functions for class UDRInvocationInfo
// ------------------------------------------------------------------------

UDRInvocationInfo::UDRInvocationInfo() :
     TMUDRSerializableObject(UDR_INVOCATION_INFO_OBJ,
                             getCurrentVersion()),
     numTableInputs_(0),
     callPhase_(UNKNOWN_CALL_PHASE),
     funcType_(SETFUNC),
     debugFlags_(0),
     udrWriterCompileTimeData_(NULL),
     udrWriterRunTimeData_(NULL),
     totalNumInstances_(0),
     myInstanceNum_(0)
{}

UDRInvocationInfo::~UDRInvocationInfo()
{
  // delete all the content of collections of pointers
  for (std::vector<PredicateInfo *>::iterator p = predicates_.begin();
       p != predicates_.end();
       p++)
    delete *p;

  // delete UDF writer's data
  if (udrWriterCompileTimeData_)
    delete udrWriterCompileTimeData_;
  if (udrWriterRunTimeData_)
    delete udrWriterRunTimeData_;
}

const std::string &UDRInvocationInfo::getUDRName() const
{
  return name_;
}

int UDRInvocationInfo::getNumTableInputs() const
{
  return numTableInputs_;
}

const TableInfo &UDRInvocationInfo::in(int childNum) const
{
  if (childNum >= numTableInputs_)
    throw UDRException(38909, "Invalid child table number %d", childNum);

  return inputTableInfo_[childNum];
}

const TableInfo &UDRInvocationInfo::out() const
{
  return outputTableInfo_;
}

TableInfo &UDRInvocationInfo::getOutputTableInfo()
{
  return outputTableInfo_;
}

CallPhase UDRInvocationInfo::getCallPhase() const
{
  return callPhase_;
}

bool UDRInvocationInfo::isCompileTime() const
{
  return (callPhase_ <= COMPILER_INITIAL_CALL &&
          callPhase_ <= COMPILER_COMPLETION_CALL);
}

bool UDRInvocationInfo::isRunTime() const
{
  return (callPhase_ >= RUNTIME_INITIAL_CALL);
}

int UDRInvocationInfo::getDebugFlags() const
{
  return debugFlags_;
}

UDRInvocationInfo::FuncType UDRInvocationInfo::getFuncType() const
{
  return funcType_;
}

const ParameterListInfo &UDRInvocationInfo::getFormalParameters() const
{
  return formalParameterInfo_;
}

const ParameterListInfo &UDRInvocationInfo::par() const
{
  return actualParameterInfo_;
}

int UDRInvocationInfo::getNumFormalParameters() const
{
  return formalParameterInfo_.getNumColumns();
}

const ColumnInfo &UDRInvocationInfo::getFormalParameterInfo(int position) const
{
  return formalParameterInfo_.getColumn(position);
}

const ColumnInfo &UDRInvocationInfo::getFormalParameterInfo(
     const std::string &name) const
{
  return formalParameterInfo_.getColumn(name);
}

int UDRInvocationInfo::getNumActualParameters() const
{
  return actualParameterInfo_.getNumColumns();
}

const ColumnInfo &UDRInvocationInfo::getActualParameterInfo(int position) const
{
  return actualParameterInfo_.getColumn(position);
}

const ColumnInfo &UDRInvocationInfo::getActualParameterInfo(
     const std::string &name) const
{
  return actualParameterInfo_.getColumn(name);
}

ParameterListInfo &UDRInvocationInfo::nonConstFormalParameters()
{
  return formalParameterInfo_;
}

ParameterListInfo &UDRInvocationInfo::nonConstActualParameters()
{
  return actualParameterInfo_;
}

int UDRInvocationInfo::getNumPredicates() const
{
  return predicates_.size();
}

const PredicateInfo &UDRInvocationInfo::getPredicate(int i) const
{
  return *(predicates_[i]);
}

void UDRInvocationInfo::addFormalParameter(const ColumnInfo &param)
{
  validateCallPhase(COMPILER_INITIAL_CALL, COMPILER_INITIAL_CALL,
                    "UDRInvocationInfo::addFormalParameter()");

  formalParameterInfo_.addColumn(param);
}

void UDRInvocationInfo::setFuncType(FuncType type)
{
  validateCallPhase(COMPILER_INITIAL_CALL, COMPILER_INITIAL_CALL,
                    "UDRInvocationInfo::setFuncType()");

  funcType_ = type;
}

void UDRInvocationInfo::addPassThruColumns(int inputTableNum,
                                           int startInputColumnNum,
                                           int endInputColumnNum)
{
  validateCallPhase(COMPILER_INITIAL_CALL, COMPILER_INITIAL_CALL,
                    "UDRInvocationInfo::addPassThruColumns()");

  // Adding one or more columns from an input (child) table as output columns
  // The advantage of doing this is that the query optimizer can automatically
  // apply some optimizations:
  //
  // - Push predicates on output columns down to input tables. This reduces the
  //   number of rows that have to be processed by the TMUDF.
  // - If a table-valued input is ordered, the TMUDF output is assumed to be
  //   also ordered on the corresponding columns.
  // - Similar for partitioning.
  // - If there are histogram statistics on an input column, these statistics
  //   will be used for the output columns as well, even though the TMUDF may
  //   eliminate some input rows and duplicate others, so the total row count
  //   and frequencies of values may or may not be usable.

  if (endInputColumnNum == -1)
    endInputColumnNum = in(inputTableNum).getNumColumns() - 1;

  for (int c=startInputColumnNum; c<=endInputColumnNum; c++)
    {
      // make a copy of the input column
      ColumnInfo newCol(in(inputTableNum).getColumn(c));

      // change the provenance info of the column
      newCol.setProvenance(ProvenanceInfo(inputTableNum, c));

      outputTableInfo_.addColumn(newCol);
    }
}

void UDRInvocationInfo::addPredicate(const PredicateInfo &pred)
{
  validateCallPhase(COMPILER_DATAFLOW_CALL, COMPILER_DATAFLOW_CALL,
                    "UDRInvocationInfo::addPredicate()");

  PredicateInfo *newPred = new PredicateInfo(pred);

  predicates_.push_back(newPred);
}

UDRWriterCompileTimeData *UDRInvocationInfo::getUDRWriterCompileTimeData()
{
  validateCallPhase(COMPILER_DATAFLOW_CALL, COMPILER_COMPLETION_CALL,
                    "UDRInvocationInfo::getUDRWriterCompileTimeData()");

  return udrWriterCompileTimeData_;
}

void UDRInvocationInfo::setUDRWriterCompileTimeData(
     UDRWriterCompileTimeData *compileTimeData)
{
  validateCallPhase(COMPILER_DATAFLOW_CALL, COMPILER_COMPLETION_CALL,
                    "UDRInvocationInfo::setUDRWriterCompileTimeData()");

  // for now we can't allow this, since we would call the destructor of
  // this object after we unloaded the DLL containing the code
  // Todo: Cache DLL opens, at least until after the
  // UDRInvocationInfo objects get deleted.
  throw UDRException(
       38912,
       "UDRInvocationInfo::setUDRWriterCompileTimeData() not yet supported");
  /*
  if (udrWriterCompileTimeData_)
    delete udrWriterCompileTimeData_;

  udrWriterCompileTimeData_ = compileTimeData;
  */
}

void UDRInvocationInfo::copyPassThruData(int inputTableNum,
                                         int startInputColumnNum,
                                         int endInputColumnNum)
{
  // no need to validate call phase, this will raise an exception at compile time
  // validateCallPhase(RUNTIME_INITIAL_CALL, RUNTIME_FINAL_CALL,
  //                   "UDRInvocationInfo::copyPassThruData()");

  int endColNum = endInputColumnNum;
  int numOutCols = out().getNumColumns();

  if (endInputColumnNum < 0 ||
      endInputColumnNum >= in(inputTableNum).getNumColumns())
    endColNum = in(inputTableNum).getNumColumns() - 1;

  // loop through the output columns and pick up those that
  // are passed through from the specified input columns
  for (int oc=0; oc<numOutCols; oc++)
    {
      const ProvenanceInfo &prov = out().getColumn(oc).getProvenance();
      int it = prov.getInputTableNum();
      int ic = prov.getInputColumnNum();

      if (it == inputTableNum &&
          ic >= startInputColumnNum &&
          ic <= endColNum)
        {
          // this output column is passed through from the range
          // of input columns selected, copy it
          const TypeInfo &ty = out().getColumn(oc).getType();

          switch (ty.getSQLTypeSubClass())
            {
            case TypeInfo::FIXED_CHAR_TYPE:
            case TypeInfo::VAR_CHAR_TYPE:
            case TypeInfo::DATE_TYPE:
            case TypeInfo::TIME_TYPE:
            case TypeInfo::TIMESTAMP_TYPE:
              {
                int strLen = 0;
                const char *str = in(it).getRaw(ic, strLen);

                if (in(it).wasNull())
                  out().setNull(oc);
                else
                  out().setString(oc, str, strLen);
              }
              break;
            case TypeInfo::EXACT_NUMERIC_TYPE:
              {
                long l = in(it).getLong(ic);

                if (in(it).wasNull())
                  out().setNull(oc);
                else
                  out().setLong(oc, l);
              }
              break;
            case TypeInfo::APPROXIMATE_NUMERIC_TYPE:
              {
                double d = in(it).getDouble(ic);

                if (in(it).wasNull())
                  out().setNull(oc);
                else
                  out().setDouble(oc, d);
              }
              break;

            case TypeInfo::YEAR_MONTH_INTERVAL_TYPE:
            case TypeInfo::DAY_SECOND_INTERVAL_TYPE:
              throw UDRException(
                   38900,
                   "UDRInvocationInfo::copyPassThruData not yet supported for type subclass %d",
                   (int) ty.getSQLTypeSubClass());

            case TypeInfo::UNDEFINED_TYPE_SUB_CLASS:
            default:
              throw UDRException(
                   38900,
                   "Invalid type subclass in UDRInvocationInfo::copyPassThruData: %d",
                   (int) ty.getSQLTypeSubClass());
            }
        }
    }
}

int UDRInvocationInfo::getNumParallelInstances() const
{
  validateCallPhase(RUNTIME_INITIAL_CALL, RUNTIME_FINAL_CALL,
                    "UDRInvocationInfo::getNumParallelInstances()");

  return totalNumInstances_;
}

int UDRInvocationInfo::getMyInstanceNum() const
{
  validateCallPhase(RUNTIME_INITIAL_CALL, RUNTIME_FINAL_CALL,
                    "UDRInvocationInfo::getMyInstanceNum()");

  return myInstanceNum_;
}

UDRWriterRunTimeData *UDRInvocationInfo::getUDRWriterRunTimeData()
{
  validateCallPhase(RUNTIME_INITIAL_CALL, RUNTIME_FINAL_CALL,
                    "UDRInvocationInfo::getUDRWriterRunTimeData()");

  return udrWriterRunTimeData_;
}

void UDRInvocationInfo::setUDRWriterRunTimeData(
     UDRWriterRunTimeData *runTimeData)
{
  validateCallPhase(RUNTIME_INITIAL_CALL, RUNTIME_FINAL_CALL,
                    "UDRInvocationInfo::setUDRWriterRunTimeData()");

  // for now we can't allow this, since we would call the destructor of
  // this object after we unloaded the DLL containing the code
  // Todo: Cache DLL opens, at least until after the
  // UDRInvocationInfo objects get deleted.
  throw UDRException(
       38912,
       "UDRInvocationInfo::setUDRRunTimeData() not yet supported");
  /*
  if (udrWriterRunTimeData_)
    delete udrWriterRunTimeData_;

  udrWriterRunTimeData_ = runTimeData;
  */
}

void UDRInvocationInfo::print()
{
  printf("\nUDRInvocationInfo\n-----------------\n");
  printf("UDR Name                   : %s\n", getUDRName().c_str());
  printf("Num of table-valued inputs : %d\n", getNumTableInputs());
  printf("Call phase                 : %s\n", callPhaseToString(callPhase_));
  printf("Debug flags                : 0x%x\n", getDebugFlags());
  printf("Function type              : TBD\n");

  printf("Formal parameters          : (");
  bool needsComma = false;
  for (int p=0; p<getNumFormalParameters(); p++)
    {
      std::string buf;

      if (needsComma)
        printf(", ");
      getFormalParameterInfo(p).toString(buf);
      printf("%s", buf.c_str());
      needsComma = true;
    }
  printf(")\n");

  printf("Actual parameters          : (");
  needsComma = false;
  const ParameterListInfo &pli = par();

  for (int p=0; p < pli.getNumColumns(); p++)
    {

      if (needsComma)
        printf(", ");

      if (pli.canGetString(p))
        {
          std::string strVal = pli.getString(p);

          if (pli.wasNull())
            printf("NULL");
          else
            printf("'%s'", strVal.c_str());
        }
      else if (pli.canGetLong(p))
        {
          long longVal = pli.getLong(p);

          if (pli.wasNull())
            printf("NULL");
          else
            printf("%ld", longVal);
        }
      else if (pli.canGetDouble(p))
        {
          double doubleVal = pli.getDouble(p);

          if (pli.wasNull())
            printf("NULL");
          else
            printf("%g", doubleVal);
        }
      else
        {
          // no value available, print name and type
          std::string buf;

          pli.getColumn(p).toString(buf, true);
          printf("\n        ");
          printf(buf.c_str());
        }
      needsComma = true;
    }
  printf(")\n");

  if (udrWriterCompileTimeData_)
    {
      printf("UDR Writer comp. time data : ");
      udrWriterCompileTimeData_->print();
      printf("\n");
    }
  if (udrWriterRunTimeData_)
    {
      printf("UDR Writer run time data   : ");
      udrWriterRunTimeData_->print();
      printf("\n");
    }

  if (isRunTime())
    printf("Instance number            : %d of %d\n",
           getMyInstanceNum(),
           getNumParallelInstances());

  for (int c=0; c<getNumTableInputs(); c++)
    {
      printf("\nInput TableInfo %d\n-----------------\n", c);
      const_cast<TableInfo &>(in(c)).print();
    }
  printf("\nOutput TableInfo\n----------------\n");
  outputTableInfo_.print();
}

int UDRInvocationInfo::serializedLength()
{
  // Format: base class + name + debugFlags + type + callPhase +
  // numTableInputs + n*TableInfo + TableInfo(outputTableInfo_) +
  // formal params + actual params
  int result = TMUDRSerializableObject::serializedLength() +
    serializedLengthOfString(name_) +
    4*serializedLengthOfInt();

  int i;

  for (i=0; i<numTableInputs_; i++)
    result += inputTableInfo_[i].serializedLength();

  result += outputTableInfo_.serializedLength();
  result += formalParameterInfo_.serializedLength();
  result += actualParameterInfo_.serializedLength();

  return result;
}

int UDRInvocationInfo::serialize(Bytes &outputBuffer,
                                 int &outputBufferLength)
{
  int result = 
    TMUDRSerializableObject::serialize(outputBuffer,
                                       outputBufferLength);
  int i;

  result += serializeString(name_,
                            outputBuffer,
                            outputBufferLength);

  result += serializeInt(debugFlags_,
                         outputBuffer,
                         outputBufferLength);

  result += serializeInt(static_cast<int>(funcType_),
                         outputBuffer,
                         outputBufferLength);

  result += serializeInt(static_cast<int>(callPhase_),
                         outputBuffer,
                         outputBufferLength);

  result += serializeInt(numTableInputs_,
                         outputBuffer,
                         outputBufferLength);

  for (i=0; i<numTableInputs_; i++)
    result += inputTableInfo_[i].serialize(outputBuffer,
                                           outputBufferLength);

  result += outputTableInfo_.serialize(outputBuffer,
                                       outputBufferLength);

  result += formalParameterInfo_.serialize(outputBuffer,
                                           outputBufferLength);

  result += actualParameterInfo_.serialize(outputBuffer,
                                           outputBufferLength);

  validateSerializedLength(result);

  return result;
}

int UDRInvocationInfo::deserialize(Bytes &inputBuffer,
                                   int &inputBufferLength)
{
  int tempInt = 0;
  int i;
  int result =
    TMUDRSerializableObject::deserialize(inputBuffer, inputBufferLength);

  validateObjectType(UDR_INVOCATION_INFO_OBJ);

  result += deserializeString(name_,
                              inputBuffer,
                              inputBufferLength);

  result += deserializeInt(debugFlags_,
                           inputBuffer,
                           inputBufferLength);

#ifndef NDEBUG
  int debugLoop = 2;

  if (debugFlags_ & DEBUG_LOAD_MSG_LOOP)
    debugLoop = 1;
  // go into a loop to allow the user to attach a debugger,
  // if requested, set debugLoop = 2 in the debugger to get out
  while (debugLoop < 2)
    debugLoop = 1-debugLoop;
#endif

  result += deserializeInt(tempInt,
                           inputBuffer,
                           inputBufferLength);
  funcType_ = static_cast<FuncType>(tempInt);

  result += deserializeInt(tempInt,
                           inputBuffer,
                           inputBufferLength);
  callPhase_ = static_cast<CallPhase>(tempInt);

  result += deserializeInt(numTableInputs_,
                           inputBuffer,
                           inputBufferLength);

  for (i=0; i<numTableInputs_; i++)
    result += inputTableInfo_[i].deserialize(inputBuffer,
                                             inputBufferLength);

  result += outputTableInfo_.deserialize(inputBuffer,
                                         inputBufferLength);

  result += formalParameterInfo_.deserialize(inputBuffer,
                                             inputBufferLength);

  result += actualParameterInfo_.deserialize(inputBuffer,
                                             inputBufferLength);

  validateDeserializedLength(result);

  return result;
}

void UDRInvocationInfo::validateCallPhase(CallPhase start,
                                          CallPhase end,
                                          const char *callee) const
{
  if (callPhase_ < start && callPhase_ != UNKNOWN_CALL_PHASE)
    throw UDRException(
         38900,
         "Method %s cannot be called before the %s phase",
         callee,
         callPhaseToString(start));
  if (callPhase_ > end)
    throw UDRException(
         38900,
         "Method %s cannot be called after the %s phase",
         callee,
         callPhaseToString(end));
}

const char *UDRInvocationInfo::callPhaseToString(CallPhase c) const
{
  switch(c)
    {
    case UNKNOWN_CALL_PHASE:
      return "unknown";
      break;
    case COMPILER_INITIAL_CALL:
      return "describeParamsAndColumns()";
      break;
    case COMPILER_DATAFLOW_CALL:
      return "describeDataflow()";
      break;
    case COMPILER_CONSTRAINTS_CALL:
      return "describeConstraints()";
      break;
    case COMPILER_STATISTICS_CALL:
      return "describeStatistics()";
      break;
    case COMPILER_DOP_CALL:
      return "describeDesiredDegreeOfParallelism()";
      break;
    case COMPILER_PLAN_CALL:
      return "describePlanProperties()";
      break;
    case COMPILER_COMPLETION_CALL:
      return "completeDescription()";
      break;
    case RUNTIME_INITIAL_CALL:
      return "runtime initial call";
      break;
    case RUNTIME_WORK_CALL:
      return "runtime work call";
      break;
    case RUNTIME_FINAL_CALL:
      return "runtime final call";
      break;
    default:
      return "invalid call phase!";
      break;
    }
}

// ------------------------------------------------------------------------
// Member functions for class UDRPlanInfo
// ------------------------------------------------------------------------

UDRPlanInfo::UDRPlanInfo(UDRInvocationInfo *invocationInfo) :
     TMUDRSerializableObject(UDR_PLAN_INFO_OBJ,
                             getCurrentVersion()),
     invocationInfo_(invocationInfo),
     costPerRow_(0),
     degreeOfParallelism_(ANY_DEGREE_OF_PARALLELISM),
     udrWriterCompileTimeData_(NULL),
     planData_(NULL),
     planDataLength_(0)
{}

UDRPlanInfo::~UDRPlanInfo()
{
  if (udrWriterCompileTimeData_)
    delete udrWriterCompileTimeData_;
  if (planData_)
    delete planData_;
}

long UDRPlanInfo::getCostPerRow() const
{
  return costPerRow_;
}

int UDRPlanInfo::getDesiredDegreeOfParallelism() const
{
  return degreeOfParallelism_;
}

void UDRPlanInfo::setDesiredDegreeOfParallelism(int dop)
{
  invocationInfo_->validateCallPhase(COMPILER_DOP_CALL,
                                     COMPILER_DOP_CALL,
                                     "UDRPlanInfo::setDesiredDegreeOfParallelism()");

  degreeOfParallelism_ = dop;
}

void UDRPlanInfo::setCostPerRow(long microseconds)
{
  invocationInfo_->validateCallPhase(COMPILER_DOP_CALL,
                                     COMPILER_PLAN_CALL,
                                     "UDRPlanInfo::setCostPerRow()");

  costPerRow_ = microseconds;
}

UDRWriterCompileTimeData *UDRPlanInfo::getUDRWriterCompileTimeData()
{
  invocationInfo_->validateCallPhase(COMPILER_DATAFLOW_CALL,
                                     COMPILER_COMPLETION_CALL,
                                     "UDRPlanInfo::getUDRWriterCompileTimeData()");

  return udrWriterCompileTimeData_;
}

void UDRPlanInfo::setUDRWriterCompileTimeData(
     UDRWriterCompileTimeData *compileTimeData)
{
  invocationInfo_->validateCallPhase(COMPILER_DATAFLOW_CALL,
                                     COMPILER_COMPLETION_CALL,
                                     "UDRPlanInfo::setUDRWriterCompileTimeData()");

  // for now we can't allow this, since we would call the destructor of
  // this object after we unloaded the DLL containing the code
  // Todo: Cache DLL opens, at least until after the
  // UDRInvocationInfo objects get deleted.
  throw UDRException(
       38912,
       "UDRPlanInfo::setUDRWriterCompileTimeData() not yet supported");

  if (udrWriterCompileTimeData_)
    delete udrWriterCompileTimeData_;

  udrWriterCompileTimeData_ = compileTimeData;
}

void UDRPlanInfo::addPlanData(const char *planData,
                              int planDataLength)
{
  invocationInfo_->validateCallPhase(COMPILER_DOP_CALL,
                                     COMPILER_COMPLETION_CALL,
                                     "UDRPlanInfo::addPlanData()");

  if (planDataLength > 0 &&
      planData == NULL)
    throw UDRException(38900,
                       "UDRWriterCompileTimeData::addPlanData() with no plan data and length >0");

  if (planDataLength_)
    delete planData_;

  planData_ = NULL;
  planDataLength_ = 0;

  if (planDataLength)
    {
      // make a new copy of the input data
      planData_ = new char[planDataLength];
      memcpy(const_cast<char *>(planData_),
             const_cast<char *>(planData),
             planDataLength);
      planDataLength_ = planDataLength;
    }
}

const char *UDRPlanInfo::getPlanData(int &planDataLength)
{
  planDataLength = planDataLength_;
  return planData_;
}

void UDRPlanInfo::print()
{
  printf("\nUDRPlanInfo\n-----------------------\n");
  printf("Cost per row               : ");
  printf("%ld\n", costPerRow_);
  printf("Degree of parallelism      : ");
  printf("%d\n", degreeOfParallelism_);
  if (getUDRWriterCompileTimeData())
    {
      printf("UDR Writer comp. time data : ");
      getUDRWriterCompileTimeData()->print();
      printf("\n");
    }
  printf("UDF Writer plan data length: ");
  printf("%d\n", planDataLength_);
}

int UDRPlanInfo::serializedLength()
{
  // Format: base class + long(cost) + int(DoP) + UDR Writer data
  int result = TMUDRSerializableObject::serializedLength() +
    serializedLengthOfLong() +
    serializedLengthOfInt();
  int udrWriterPlanDataLen = 0;

  result += serializedLengthOfBinary(planDataLength_);

  return result;
}

int UDRPlanInfo::serialize(Bytes &outputBuffer,
                           int &outputBufferLength)
{
  int result = 
    TMUDRSerializableObject::serialize(outputBuffer,
                                       outputBufferLength);

  result += serializeLong(costPerRow_,
                          outputBuffer,
                          outputBufferLength);

  result += serializeInt(degreeOfParallelism_,
                         outputBuffer,
                         outputBufferLength);

  int udrWriterPlanDataLen = 0;
  char *udrWriterPlanData = NULL;

  result += serializeBinary(planData_,
                            planDataLength_,
                            outputBuffer,
                            outputBufferLength);

  validateSerializedLength(result);

  return result;
}

int UDRPlanInfo::deserialize(Bytes &inputBuffer,
                             int &inputBufferLength)
{
  int result =
    TMUDRSerializableObject::deserialize(inputBuffer, inputBufferLength);

  validateObjectType(UDR_PLAN_INFO_OBJ);

  result += deserializeLong(costPerRow_,
                            inputBuffer,
                            inputBufferLength);

  result += deserializeInt(degreeOfParallelism_,
                           inputBuffer,
                           inputBufferLength);

  result += deserializeBinary((const void **) &planData_,
                              planDataLength_,
                              true,
                              inputBuffer,
                              inputBufferLength);

  validateDeserializedLength(result);

  return result;
}

// ------------------------------------------------------------------------
// Member functions for class UDRInterface
// ------------------------------------------------------------------------

UDRInterface::UDRInterface() :
     getNextRowPtr_(NULL),
     emitRowPtr_(NULL)
{}

UDRInterface::~UDRInterface() {}

// Describe the output columns of a TMUDF, based on a description of
// its parameters (including parameter values that are specified as a
// constant) and the description of the table-valued input columns

// This default implementation does nothing
void UDRInterface::describeParamsAndColumns(UDRInvocationInfo &info)
{
  // First method called during compilation of a TMUDF invocation
  // ------------------------------------------------------------

  // When the optimizer calls this, it will have set up the formal and
  // actual parameter descriptions as well as an output column
  // description containing all the output parameters defined in the
  // CREATE FUNCTION DDL (if any).

  // This method should do a general check of things it expects
  // that can be validated at this time.

  // This method can add, alter, or remove input and output
  // parameters:
  // - input parameters that are constants can be altered
  //   to a different constant value
  // - Todo: additional constant input parameters can be added
  // - non-constant input parameters: ???
  // - output parameters (columns of the table-valued result)
  //   can be input columns that are passed through or
  //   they can be columns generated by the TMUDF

  // Example 1: check for a constant parameter that is expected:
  /*
     if (info.getNumParameters() != 3)
       throw UDRException(38088, 
                          "Expecting three input parameters");
     if (!info.getFormalParameters().canGetInt(0))
       throw UDRException(38099, 
                          "First argument of TMUDF %s needs to be a compile time integer constant",
                          info.getName().c_str());
  */

  // Example 2: To add a column produced by the TMUDF, use the
  // TableInfo::addColumn() method. Example: Add a char(10) output
  // column created by the UDF as the first column:

  /*
     info.getOutputTableInfo().addColumnAt(
       ColumnInfo("ColProducedByTMUDF",      // name of output column
                  TypeInfo(TypeInfo::STRING, // C type of column
                           10)),             // length of string
       0);                                   // output column position
  */

  // Example 3: Add all columns provided by the table-valued
  //            inputs as output columns

  /*
     for (int t=0; t<info.getNumTableInputs(); t++)
       info.addPassThruColumns(t);
  */

}

void UDRInterface::describeDataflow(UDRInvocationInfo &info)
{
  // TBD
}

void UDRInterface::describeConstraints(UDRInvocationInfo &info)
{
  // TBD
}

void UDRInterface::describeStatistics(UDRInvocationInfo &info)
{
  // TBD
}

void UDRInterface::describeDesiredDegreeOfParallelism(UDRInvocationInfo &info,
                                                        UDRPlanInfo &plan)
{
  // The default behavior is to allow any degree of parallelism for
  // TMUDFs with one table-valued input, and to force serial execution
  // in all other cases. The reason is that for a single table-valued
  // input, there is a natural way to parallelize the function by
  // parallelizing its input a la MapReduce. In all other cases,
  // parallel execution requires active participation by the UDF,
  // which is why the UDF needs to signal explicitly that it can
  // handle such flavors of parallelism.

  // Note that this is NOT foolproof, and that the TMUDF might still
  // need to validate the PARTITION BY and ORDER BY syntax used in its
  // invocation.

  if (info.getNumTableInputs() == 1)
    plan.setDesiredDegreeOfParallelism(UDRPlanInfo::ANY_DEGREE_OF_PARALLELISM);
  else
    plan.setDesiredDegreeOfParallelism(1); // serial execution

  // Note about using the pre-defined constants:
  // 
  // ANY_DEGREE_OF_PARALLELISM:
  //        This will allow the optimizer to choose any degree
  //        of parallelism, including 1 (serial execution)
  // DEFAULT_DEGREE_OF_PARALLELISM:
  //        Currently the same as ANY_DEGREE_OF_PARALLELISM.
  //        The optimizer will use a heuristic based on
  //        the estimated cardinality.
  // MAX_DEGREE_OF_PARALLELISM:
  //        Choose the highest possible degree of parallelism.
  // ONE_INSTANCE_PER_NODE:
  //        Start one parallel instance on every Trafodion node.
  //        This is mostly meant for internal TMUDFs, e.g. a
  //        TMUDF to read the log files on every node.
}

void UDRInterface::describePlanProperties(UDRInvocationInfo &info,
                                          UDRPlanInfo &plan)
{
  // TBD
}

void UDRInterface::completeDescription(UDRInvocationInfo &info,
                                       UDRPlanInfo &plan)
{
  // The default implementation does nothing.
}

void UDRInterface::processData(UDRInvocationInfo &info,        // runtime
                               UDRPlanInfo &plan)
{
  throw UDRException(38900,"UDRInterface::processData() must be overridden by the UDF");
}

void UDRInterface::debugLoop()
{
  int debugLoop = 1;
  int myPid = static_cast<int>(getpid());

  printf("Process %d entered a loop to be able to debug it\n", myPid);

  // go into a loop to allow the user to attach a debugger,
  // if requested, set debugLoop = 2 in the debugger to get out
  while (debugLoop < 2)
    debugLoop = 1-debugLoop;
}

bool UDRInterface::getNextRow(UDRInvocationInfo &info, int tableIndex)
{
  SQLUDR_Q_STATE qstate = SQLUDR_Q_MORE;

  (*getNextRowPtr_)(info.in(tableIndex).getRowPtr(),
                    tableIndex,
                    &qstate);

#ifndef NDEBUG
  if (info.getDebugFlags() & UDRInvocationInfo::TRACE_ROWS)
    switch (qstate)
      {
      case SQLUDR_Q_MORE:
        {
          std::string row;

          info.in(tableIndex).getDelimitedRow(row,'|',true);
          printf("(%d) Input row from table %d: %s\n",
                 info.getMyInstanceNum(), tableIndex, row.c_str());
        }
        break;

      case SQLUDR_Q_EOD:
        printf("(%d) Input table %d reached EOD\n",
               info.getMyInstanceNum(), tableIndex);
        break;

      case SQLUDR_Q_CANCEL:
        printf("(%d) Cancel request from input table %d\n",
               info.getMyInstanceNum(), tableIndex);
        break;

      default:
        printf("(%d) Invalid queue state %d from input table %d\n",
               info.getMyInstanceNum(), qstate, tableIndex);
      }
#endif

  return (qstate == SQLUDR_Q_MORE);
}

void UDRInterface::emitRow(UDRInvocationInfo &info)
{
  SQLUDR_Q_STATE qstate = SQLUDR_Q_MORE;

#ifndef NDEBUG
  if (info.getDebugFlags() & UDRInvocationInfo::TRACE_ROWS)
    {
      std::string row;

      info.out().getDelimitedRow(row,'|',true);
      printf("(%d) Emitting row: %s\n",
             info.getMyInstanceNum(), row.c_str());
    }
#endif

  (*emitRowPtr_)(info.getOutputTableInfo().getRowPtr(),
                 0,
                 &qstate);
}

int UDRInterface::getFeaturesSupportedByUDF()
{
  // the default implementation returns 0 (no extra features are supported)
  return 0;
}
