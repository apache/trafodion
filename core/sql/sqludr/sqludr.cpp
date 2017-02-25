/**********************************************************************
 *
 * File:         sqludr.h
 * Description:  Interface between the SQL engine and routine bodies
 * Language:     C
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
 *********************************************************************/

#include "sqludr.h"
#include <stdio.h>
#include <cstdarg>
#include <climits>
#include <string.h>

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
//
// This file contains doxygen comments. To generate HTML documentation,
// run this command:
//
// doxygen doxygen_tmudr.1.6.config
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// Member functions for class UDRException
// ------------------------------------------------------------------------

/**
 *  Constructor with an integer value for SQLSTATE
 *
 *  @param sqlState ISO/ANSI SQLSTATE value to produce for this error.
 *                  According to the standard, this must be a value in
 *                  the range of 38000 - 38999 (note that since we use
 *                  an integer, non-numeric SQLSTATE values cannot be
 *                  generated.
 *  @param printf_format a format string like it is used in printf,
 *                  with a variable list of arguments to be substituted.
 *                  Example:
 *                  new UDRException(38001, "num %d, string %s", 1, "a");
 */
UDRException::UDRException(int sqlState, const char *printf_format, ...)
{
  va_list args;
  const int maxMsgLen = 250;
  char msg[maxMsgLen];

  va_start(args, printf_format);
  vsnprintf(msg, maxMsgLen, printf_format, args);
  va_end(args);

  text_ = msg;
  snprintf(sqlState_, sizeof(sqlState_), "%05d", sqlState);
}

/**
 *  Constructor with a string value for SQLSTATE
 *
 *  @param sqlState ISO/ANSI SQLSTATE value to produce for this error.
 *                  According to the standard, this must be a value of
 *                  the form 38xxx, with the xxx being digits or upper
 *                  case letters.
 *  @param printf_format a format string like it is used in printf,
 *                  with a variable list of arguments to be substituted.
 */
UDRException::UDRException(const char *sqlState, const char *printf_format, ...)
{
  va_list args;
  const int maxMsgLen = 250;
  char msg[maxMsgLen];

  va_start(args, printf_format);
  vsnprintf(msg, maxMsgLen, printf_format, args);
  va_end(args);

  text_ = msg;

  strncpy(sqlState_, sqlState, sizeof(sqlState_));
  // add a NUL terminator in case we overflowed
  sqlState_[sizeof(sqlState_)-1] = 0;
}

/**
 *  Get the SQSTATE value for this exception
 *
 *  @return A string, representing the SQLSTATE. Note that
 *          this is a pointer to a data member, the buffer
 *          lives only as long as the UDRException object.
 */
const char * UDRException::getSQLState() const
{
  return sqlState_;
}

/**
 *  Get the error message associated with this exception
 *
 *  @return A string, representing the error message, including
 *          any substituted text with the additional arguments
 *          in the constructor. Note that this is a reference to
 *          a data member, it lives only as long as the
 *          UDRException object.
 */
const std::string & UDRException::getMessage() const
{
  return text_;
}

/**
 *  Get the error message associated with this exception
 *
 *  @return Same as getMessage().
 *
 *  @deprecated Use getMessage() instead, in Java that is the
 *              standard method.
 */
const std::string & UDRException::getText() const
{
  return text_;
}

// ------------------------------------------------------------------------
// Member functions for class TMUDRSerializableObject
// ------------------------------------------------------------------------

TMUDRSerializableObject::TMUDRSerializableObject(TMUDRObjectType objectType,
                                                 unsigned short version,
                                                 unsigned short endianness)
{
  v_.objectType_  = static_cast<int>(objectType);
  v_.totalLength_ = -1; // will be set when we serialize the object
  v_.version_     = version;
  v_.endianness_  = endianness;
  v_.flags_       = 0;
  v_.filler_      = 0;
}

TMUDRSerializableObject::TMUDRObjectType TMUDRSerializableObject::getObjectType() const 
{
  return static_cast<TMUDRObjectType>(v_.objectType_);
}

unsigned short TMUDRSerializableObject::getVersion() const
{
  return v_.version_;
}

TMUDRSerializableObject::Endianness TMUDRSerializableObject::getEndianness() const
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
                       static_cast<int>(v_.objectType_),
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

int TMUDRSerializableObject::deserialize(ConstBytes &inputBuffer,
                                         int &inputBufferLength)
{
  if (inputBufferLength < sizeof(v_))
    throw UDRException(38900,"not enough data to deserialize object header, need %d, got %d bytes",
                       sizeof(v_),
                       inputBufferLength);
  memcpy((void *) &v_, inputBuffer, sizeof(v_));

  if (inputBufferLength < v_.totalLength_)
    throw UDRException(38900,"not enough data to deserialize object of type %d, need %d, got %d bytes",
                       static_cast<int>(v_.objectType_),
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

void TMUDRSerializableObject::validateObjectType(TMUDRObjectType o)
{
  if (v_.objectType_ != o)
    throw UDRException(38900,"Object type of expected object (%d) does not match the type (%d) in the serialized buffer",
                       o,
                       static_cast<int>(v_.objectType_));
}

void TMUDRSerializableObject::validateSerializedLength(int l)
{
  if (l != v_.totalLength_)
    throw UDRException(38900,"Expected %d bytes to serialize object of type %d, actually produced %d bytes",
                       v_.totalLength_,
                       static_cast<int>(v_.objectType_),
                       l);
}

void TMUDRSerializableObject::validateDeserializedLength(int l)
{
  if (l != v_.totalLength_)
    throw UDRException(38900,"Expected %d bytes to deserialize object of type %d, actually consumed %d bytes",
                       v_.totalLength_,
                       static_cast<int>(v_.objectType_),
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
     ConstBytes &inputBuffer,
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
     ConstBytes &inputBuffer,
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
     ConstBytes &inputBuffer,
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
     ConstBytes &inputBuffer,
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
     ConstBytes &inputBuffer,
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

TMUDRSerializableObject::TMUDRObjectType TMUDRSerializableObject::getNextObjectType(
     ConstBytes inputBuffer,
     int inputBufferLength)
{
  // determine the object type of the next object in the buffer
  if (inputBufferLength < sizeof(v_))
    throw UDRException(38900,"not enough data to look at next object header, need %d, got %d bytes",
                       sizeof(v_),
                       inputBufferLength);
  const headerFields *nextObjInBuffer = reinterpret_cast<const headerFields *>(inputBuffer);

  return static_cast<TMUDRObjectType>(nextObjInBuffer->objectType_);
}

// ------------------------------------------------------------------------
// Member functions for class TypeInfo
// ------------------------------------------------------------------------

/** Copy constructor */
TypeInfo::TypeInfo(const TypeInfo &type) :
     TMUDRSerializableObject(TYPE_INFO_OBJ,
                             getCurrentVersion())
{
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

/**
 *  Default constructor, with optional arguments
 *
 *  Construct a TypeInfo object from an SQL type, with several optional
 *  arguments (including the SQL type). This is mostly used to create
 *  formal parameters or output columns in the compiler interface, if
 *  a more complex data type is required that is not covered by the
 *  TupleInfo::addXXXColumn() methods.
 *
 *  @param sqlType       SQL type enum to construct the type from.
 *  @param length        Length of CHAR/VARCHAR types, not needed for other types.
 *                       Note that the length for UTF-8 types is in bytes, not
 *                       characters, so this is equivalent to
 *                       @n [VAR]CHAR (@c @b length BYTES) CHARACTER SET UTF8
 *  @param nullable      Determines the NULL / NOT NULL attribute of the type
 *                       Default: false (that means NOT NULL)
 *  @param scale         Scale for numeric type, fraction precision for
 *                       fractional seconds, not needed for other types.
 *  @param charset       Character set enum for CHAR/VARCHAR types, not needed
 *                       for other types.
 *  @param intervalCode  Interval code enum for intervals, not needed otherwise.
 *  @param precision     Precision for numeric types and leading precision for
 *                       interval data types.
 *  @param collation     Collation enum for CHAR/VARCHAR types, not needed for
 *                       other types. Note that only one type of collation is
 *                       currently supported.
 *  @throws UDRException
 */
TypeInfo::TypeInfo(SQLTypeCode sqlType,
                   int length,
                   bool nullable,
                   int scale,
                   SQLCharsetCode charset,
                   SQLIntervalCode intervalCode,
                   int precision,
                   SQLCollationCode collation) :
     TMUDRSerializableObject(TYPE_INFO_OBJ,
                             getCurrentVersion())
{
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
    case TINYINT:
      d_.length_ = 1;
      d_.precision_ = 0;
      d_.scale_ = 0;
      break;

    case SMALLINT:
      d_.length_ = 2;
      d_.precision_ = 0;
      d_.scale_ = 0;
      break;

    case INT:
      d_.length_ = 4;
      d_.precision_ = 0;
      d_.scale_ = 0;
      break;

    case LARGEINT:
      d_.length_ = 8;
      d_.precision_ = 0;
      d_.scale_ = 0;
      break;

    case NUMERIC:
      d_.length_ = convertToBinaryPrecision(d_.precision_);
      if (d_.scale_ < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a numeric in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.scale_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a numeric in TypeInfo::TypeInfo is greater than precision %d", d_.scale_, d_.precision_);
      break;

    case DECIMAL_LSE:
      if (scale < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a decimal in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.scale_);
      if (precision < 1 || precision > 18)
        throw UDRException(38900,"Precision %d of a decimal in TypeInfo::TypeInfo is out of the allowed range of 1-18", d_.precision_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a decimal in TypeInfo::TypeInfo is greater than precision %d", d_.scale_, d_.precision_);
      // format [-]mmmm[.sss]  - total number of digits = precision
      d_.length_ = d_.precision_ + 1; // add one for the sign
      if (d_.scale_ > 0)
        d_.length_ += 1; // for the decimal point
      break;

    case TINYINT_UNSIGNED:
      d_.length_ = 1;
      d_.precision_ = 0;
      d_.scale_ = 0;
      break;

    case SMALLINT_UNSIGNED:
      d_.length_ = 2;
      d_.precision_ = 0;
      d_.scale_ = 0;
      break;

    case INT_UNSIGNED:
      d_.length_ = 4;
      d_.precision_ = 0;
      d_.scale_ = 0;
      break;

    case NUMERIC_UNSIGNED:
      d_.length_ = convertToBinaryPrecision(d_.precision_);
      if (d_.scale_ < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a numeric unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.scale_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a numeric unsigned in TypeInfo::TypeInfo is greater than precision %d", d_.scale_, d_.precision_);
      break;

    case DECIMAL_UNSIGNED:
      if (scale < 0 || scale > 18)
        throw UDRException(38900,"Scale %d of a decimal unsigned in TypeInfo::TypeInfo is out of the allowed range of 0-18", d_.scale_);
      if (d_.precision_ < 1 || d_.precision_ > 18)
        throw UDRException(38900,"Precision %d of a decimal unsigned in TypeInfo::TypeInfo is out of the allowed range of 1-18", d_.precision_);
      if (scale > precision)
        throw UDRException(38900,"Scale %d of a decimal unsigned in TypeInfo::TypeInfo is greater than precision %d", d_.scale_, d_.precision_);
      // format mmmm[.sss]  - total number of digits = precision
      d_.length_ = d_.precision_;
      if (d_.scale_ > 0)
        d_.length_ += 1; // for the decimal point
      break;

    case REAL:
      d_.length_ = 4;
      break;

    case DOUBLE_PRECISION:
      d_.length_ = 8;
      break;

    case CHAR:
      if (d_.charset_ == UNDEFINED_CHARSET)
        throw UDRException(38900,"Charset must be specified for CHAR type in TypeInfo::TypeInfo");
      // length is the length in characters, but d_.length_ is
      // the byte length, multiply by min bytes per char
      d_.length_ = length * minBytesPerChar();
      if (d_.length_ < 0)
        throw UDRException(38900,
                           "Length of a character type must not be negative, got %d",
                           d_.length_);
      if (d_.collation_ == UNDEFINED_COLLATION)
        throw UDRException(38900,"Collation must be specified for CHAR type in TypeInfo::TypeInfo");
      break;

    case VARCHAR:
      if (d_.charset_ == UNDEFINED_CHARSET)
        throw UDRException(38900,"Charset must be specified for VARCHAR type in TypeInfo::TypeInfo");
      if (d_.collation_ == UNDEFINED_COLLATION)
        throw UDRException(38900,"Collation must be specified for VARCHAR type in TypeInfo::TypeInfo");
      // length is the length in characters, but d_.length_ is
      // the byte length, multiply by min bytes per char
      d_.length_ = length * minBytesPerChar();
      if (d_.length_ > 32767)
        // see also CharType::CharType in ../common/CharType.cpp
        d_.flags_ |= TYPE_FLAG_4_BYTE_VC_LEN;
      if (d_.length_ < 0)
        throw UDRException(38900,
                           "Length of a varchar type must not be negative, got %d",
                           d_.length_);
      break;

    case CLOB:
    case BLOB:
      // BLOB and CLOB are represented by a handle that looks like a VARCHAR
      // but may contain binary data (use ISO8859-1 to be able to represent
      // binary data)
      d_.charset_ = CHARSET_ISO88591;
      // should we check the provided length if it comes from the UDR writer?
      // or just let it error out at runtime with an overflow?
      break;

    case DATE:
      // string yyyy-mm-dd
      d_.length_ = 10;
      d_.scale_ = 0;
      break;

    case TIME:
      // string hh:mm:ss
      d_.length_ = 8;
      if (scale > 0)
        d_.length_ += scale+1;
      if (scale < 0 || scale > 6)
        throw UDRException(38900,"Scale %d of time in TypeInfo::TypeInfo is outside the allowed range of 0-6", scale);
      break;

    case TIMESTAMP:
      // string yyyy-mm-dd hh:mm:ss.ffffff
      //        12345678901234567890123456
      d_.length_ = 19;
      if (scale > 0)
        d_.length_ += scale+1;
      if (scale < 0 || scale > 6)
        throw UDRException(38900,"Scale %d of timestamp in TypeInfo::TypeInfo is outside the allowed range of 0-6", scale);
      break;

    case INTERVAL:
      {
        int totalPrecision = 0;
        bool allowScale = false;

        if (d_.intervalCode_ == UNDEFINED_INTERVAL_CODE)
          throw UDRException(38900,"Interval code in TypeInfo::TypeInfo is undefined");
        if (scale < 0 || scale > 6)
          throw UDRException(38900,"Scale %d of interval in TypeInfo::TypeInfo is outside the allowed range of 0-6", sqlType);

        // all intervals are treated like signed numbers, need to compute
        // the length from the combined precision of all parts, see method
        // IntervalType::getStorageSize() in ../common/IntervalType.cpp and
        // see also the defaults for leading precision in the SQL Reference
        // Manual. Note that the default for fraction precision in this
        // constructor is 0, the default scale for other types. This is
        // different from the default fraction precision of 6 in Trafodion
        // SQL!!

        // start with the leading precision
        if (precision == 0)
          totalPrecision = 2; // default leading precision
        else
          totalPrecision = precision;

        switch (d_.intervalCode_)
          {
          case INTERVAL_YEAR:
          case INTERVAL_MONTH:
          case INTERVAL_DAY:
          case INTERVAL_HOUR:
          case INTERVAL_MINUTE:
            // we are all set
            break;
          case INTERVAL_SECOND:
            // add the fraction precision (scale)
            totalPrecision += scale;
            allowScale = true;
            break;
          case INTERVAL_YEAR_MONTH:
          case INTERVAL_DAY_HOUR:
          case INTERVAL_HOUR_MINUTE:
            // leading field + 1 more field
            totalPrecision += 2;
            break;
          case INTERVAL_DAY_MINUTE:
            // leading field + 2 more fields
            totalPrecision += 4;
            break;
          case INTERVAL_DAY_SECOND:
            totalPrecision += 6 + scale;
            allowScale = true;
            break;
          case INTERVAL_HOUR_SECOND:
            totalPrecision += 4 + scale;
            allowScale = true;
            break;
          case INTERVAL_MINUTE_SECOND:
            totalPrecision += 2 + scale;
            allowScale = true;
            break;
          default:
            throw UDRException(
                 38900,
                 "TypeInfo::TypeInfo() for interval type with invalid interval code");
          }

        if (scale > 0 && !allowScale)
          throw UDRException(
               38900,
               "TypeInfo::TypeInfo(): Scale (fraction precision) should not be specified for a type when end field is not SECOND");

        // convert decimal to binary precision, but intervals don't
        // use single byte representation (yet?)
        d_.length_ = MAXOF(2,convertToBinaryPrecision(totalPrecision));
      }
      break;

    case BOOLEAN:
      d_.length_ = 1;
      d_.precision_ = 0;
      d_.scale_ = 0;
      break;

    case UNDEFINED_SQL_TYPE:
      // this case is reached when we call the default constructor,
      // type and other fields still need to be defined
      break;

    default:
      throw UDRException(38900,"Invalid SQL Type code for the short TypeInfo constructor with an SQL code: %d", sqlType);
      break;
    }
}

TypeInfo::TypeInfo(SQLTypeCode sqlType,
                   bool nullable,
                   int scale,
                   SQLCharsetCode charset,
                   SQLIntervalCode intervalCode,
                   int precision,
                   SQLCollationCode collation,
                   int length) :
     TMUDRSerializableObject(TYPE_INFO_OBJ,
                             getCurrentVersion())
{
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

/**
 *  Get the SQL type.
 *
 *  @return SQL type enum.
 */
TypeInfo::SQLTypeCode TypeInfo::getSQLType() const
{
  return (TypeInfo::SQLTypeCode) d_.sqlType_;
}

/**
 *  Get the SQL type class.
 *
 *  Determine whether this is a numeric character, datetime or interval type.
 *  @return SQL type class enum.
 */
TypeInfo::SQLTypeClassCode TypeInfo::getSQLTypeClass() const
{
  switch (d_.sqlType_)
    {
    case TINYINT:
    case SMALLINT:
    case INT:
    case LARGEINT:
    case NUMERIC:
    case DECIMAL_LSE:
    case TINYINT_UNSIGNED:
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

    case BLOB:
    case CLOB:
      return LOB_TYPE;

    case BOOLEAN:
      return BOOLEAN_TYPE;

    default:
      break;
    }

  return UNDEFINED_TYPE_CLASS;
}

/**
 *  Get the SQL type subclass.
 *
 *  This goes to one more level of detail beyond the type class,
 *  like exact/approximate numeric, char/varchar, etc.
 *  @return SQL type subclass enum.
 */
TypeInfo::SQLTypeSubClassCode TypeInfo::getSQLTypeSubClass() const
{
  switch (d_.sqlType_)
    {
    case TINYINT:
    case SMALLINT:
    case INT:
    case LARGEINT:
    case NUMERIC:
    case DECIMAL_LSE:
    case TINYINT_UNSIGNED:
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

    case BLOB:
    case CLOB:
      return LOB_SUB_CLASS;

    case BOOLEAN:
      return BOOLEAN_SUB_CLASS;

    default:
      break;
    }

  return UNDEFINED_TYPE_SUB_CLASS;
}

/**
 *  Get whether the type is nullable.
 *
 *  @return True for nullable types, false for non-nullable types.
 */
bool TypeInfo::getIsNullable() const
{
  return (d_.nullable_ != 0);
}

/**
 *  Get the scale of the data type.
 *
 *  For integer, largeint, etc. types the scale is 0, since these are
 *  integer data types. For NUMERIC and DECIMAL types, a scale can
 *  be specified. Timestamp and some interval data types have a
 *  "fraction precision" value, which is the number of digits
 *  allowed after the decimal point for seconds. This fraction precision
 *  is returned as the scale, since can be considered the scale of
 *  the seconds part. For other data types like CHAR, the scale
 *  value is meaningless.
 *  @return Scale (digits after the decimal point) for numeric types,
 *          fraction precision (digits of fractional seconds) for intervals.
 */
int TypeInfo::getScale() const
{
  return d_.scale_;
}

/**
 *  Get the character set of the data type.
 *
 *  @return Character set enum.
 */
TypeInfo::SQLCharsetCode TypeInfo::getCharset() const
{
  return (TypeInfo::SQLCharsetCode) d_.charset_;
}

/**
 *  Get the interval code for start/end fields.
 *
 *  @return Interval code enum, indicating start and end fields of an interval type.
 */
TypeInfo::SQLIntervalCode TypeInfo::getIntervalCode() const
{
  return (TypeInfo::SQLIntervalCode) d_.intervalCode_;
}

/**
 *  Get the precision (max. number of significant digits).
 *
 *  The precision is the maximum number of digits before the decimal
 *  point a value can have. For interval types, this is the "leading
 *  precision". For example, an INTEGER value can range from
 *  -2,147,483,648 to 2,147,483,647. It's precision is 10, since the
 *  longest number has 10 digits. Note that not all 10 digit numbers
 *  can be represented in an integer. This is called binary
 *  precision. NUMERIC and DECIMAL types have decimal precision,
 *  meaning that a NUMERIC(10,0) type can represent values from
 *  -9,999,999,999 to +9,999,999,999.
 *
 *  @return Precision of numeric types or interval types.
 */
int TypeInfo::getPrecision() const
{
  return d_.precision_;
}

/**
 *  Get the collation for char/varchar data types.
 *
 *  Note that, currently, only one collation is supported.
 *  This default collation is a binary collation, except that
 *  trailing blanks are ignored.
 *
 *  @return Collation enum.
 */
TypeInfo::SQLCollationCode TypeInfo::getCollation() const
{
  return (TypeInfo::SQLCollationCode) d_.collation_;
}

/**
 *  Get the length of a value of the type.
 *
 *  Getting the length is useful for CHAR/VARCHAR data types
 *  but probably not as useful for other types that may have
 *  an internal representation unknown to a UDR writer.
 *  This returns the length in bytes, not in characters.
 *
 *  @see getCharLength()
 *
 *  @return Length in bytes.
 */
int TypeInfo::getByteLength() const
{
  return d_.length_;
}

/**
 *  Get the maximum number of characters that can be stored in this type.
 *
 *  This method should be used only for character types that
 *  have a fixed-width encoding. For variable-length encoding, like
 *  UTF-8, the method returns the highest possible number of characters
 *  (assuming single byte characters in the case of UTF-8). Right now,
 *  UTF-8 data types all have byte semantics, meaning there is no
 *  limit for the number of characters stored in a type, it is only
 *  limited by the number of bytes. The method returns 0 for numeric
 *  types. It returns the length of the string representation for
 *  types that are represented by a string, like datetime types.
 *
 *  @see getByteLength()
 *
 *  @return Length in bytes.
 *  @throws UDRException
 */
int TypeInfo::getMaxCharLength() const
{
  switch (getSQLTypeClass())
    {
    case CHARACTER_TYPE:
      return d_.length_ / minBytesPerChar();
    case NUMERIC_TYPE:
      return 0;
    case DATETIME_TYPE:
      // return the length of the string representation
      // in ISO88591/UTF-8
      return d_.length_;
    case INTERVAL_TYPE:
      return 0;
    case BOOLEAN_TYPE:
      return 0;
    default:
      throw UDRException(
           38900,
           "Called TypeInfo::getMaxCharLength() on an unsupported type: %d",
           d_.sqlType_);
    }
}

/**
 *  Set the nullable attribute of a type
 *
 *  Use this method to set types created locally in the UDF
 *  to be nullable or not nullable.
 *
 *  @param nullable true to set the type to nullable, false
 *                  to give the type the NOT NULL attibute.
 */
void TypeInfo::setNullable(bool nullable)
{
  d_.nullable_ = nullable;
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
  // see also code in LmTypeIsString() in file ../generator/LmExpr.cpp
  if (d_.sqlType_ == NUMERIC ||
      d_.sqlType_ == NUMERIC_UNSIGNED ||
      d_.sqlType_ == INTERVAL ||
      d_.sqlType_ == BOOLEAN)
    {
      if (d_.length_ == 1)
        if (d_.sqlType_ == NUMERIC_UNSIGNED)
          tempSQLType = TINYINT_UNSIGNED;
        else
          tempSQLType = TINYINT;
      else if (d_.length_ == 2)
        if (d_.sqlType_ == NUMERIC_UNSIGNED)
          tempSQLType = SMALLINT_UNSIGNED;
        else
          tempSQLType = SMALLINT;
      else if (d_.length_ == 4)
        if (d_.sqlType_ == NUMERIC_UNSIGNED)
          tempSQLType = INT_UNSIGNED;
        else
          tempSQLType = INT;
      else if (d_.length_ == 8)
        tempSQLType = LARGEINT;
        // unsigned 8 byte integer is not supported
    }

  switch (tempSQLType)
    {
    case TINYINT:
      result = *((char *) data);
      break;

    case SMALLINT:
      result = *((short *) data);
      break;

    case INT:
      result = *((int *) data);
      break;

    case LARGEINT:
      result = *((long *) data);
      break;

    case TINYINT_UNSIGNED:
      result = *((unsigned char *) data);
      break;

    case SMALLINT_UNSIGNED:
      result = *((unsigned short *) data);
      break;

    case INT_UNSIGNED:
      result = *((int *) data);
      break;

    case DECIMAL_LSE:
    case DECIMAL_UNSIGNED:
      {
        long fractionalPart = 0;
        bool isNegative = false;
        bool overflow = false;
        char buf[200];
        int dataLen = d_.length_;

        if (*data == '-')
          {
            isNegative = true;
            data++;
            dataLen--;
          }

        // copy the value to be able to add a terminating NUL byte
        memcpy(buf, data, dataLen);
        buf[dataLen] = 0;

        if (d_.scale_ == 0)
          {
            if (sscanf(buf, "%ld", &result) != 1)
              throw UDRException(
                   38900,
                   "Error converting decimal value %s to a long",
                   buf);
          }
        else
          {
            if (sscanf(buf, "%ld.%ld", &result, &fractionalPart) != 2)
              throw UDRException(
                   38900,
                   "Error converting decimal value %s (with scale) to a long",
                   buf);
            for (int s=0; s<d_.scale_; s++)
              if (result <= LONG_MAX/10)
                result *= 10;
              else
                overflow = true;
            if (result <= LONG_MAX - fractionalPart)
              result += fractionalPart;
            else
              overflow = true;
          }

        if (isNegative)
          if (result < LONG_MAX)
            result = -result;
          else
            overflow = true;

        if (overflow)
          throw UDRException(
               38900,
               "Under or overflow occurred, converting decimal to a long");
      }
      break;

    case REAL:
    case DOUBLE_PRECISION:
      {
        double dresult = getDouble(row, wasNull);

        if (dresult < LONG_MIN || dresult > LONG_MAX)
            throw UDRException(
                 38900, 
                 "Overflow in getInt() or getLong(), float value %g does not fit in a long",
                 dresult);
        result = static_cast<long>(dresult);
      }
      break;

    default:
      throw UDRException(38902,
                         "TypeInfo::getLong() and getDouble() not supported for SQL type %d",
                         d_.sqlType_);
      break;
    }

  return result;
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

    case TINYINT:
    case SMALLINT:
    case INT:
    case LARGEINT:
    case NUMERIC:
    case DECIMAL_LSE:
    case TINYINT_UNSIGNED:
    case SMALLINT_UNSIGNED:
    case INT_UNSIGNED:
    case NUMERIC_UNSIGNED:
    case DECIMAL_UNSIGNED:
    case INTERVAL:
    case BOOLEAN:
      {
        result = static_cast<double>(getLong(row, wasNull));
        // for numbers with a scale, ensure that the decimal
        // point is at the right place for floating point results
        for (int s=0; s<d_.scale_; s++)
          result /= 10;
      }
      break;

    default:
      throw UDRException(38900,
                         "getDouble() not supported for SQL type %d",
                         d_.sqlType_);
      break;
    }

  return result;
}

time_t TypeInfo::getTime(const char *row, bool &wasNull) const
{
  time_t result = 0;

  if (d_.sqlType_ == INTERVAL)
    {
      long longVal = getLong(row, wasNull);

      if (wasNull)
        return 0;

      // convert the interval value to seconds

      // NOTE: This relies on the assumption that time_t
      //       uses seconds as its unit, which is true for
      //       current Linux systems but may not always remain
      //       true
      switch (d_.intervalCode_)
        {
        case INTERVAL_DAY:
          result = longVal * 86400;
          break;
        case INTERVAL_HOUR:
        case INTERVAL_DAY_HOUR:
          result = longVal * 3600;
          break;
        case INTERVAL_MINUTE:
        case INTERVAL_DAY_MINUTE:
        case INTERVAL_HOUR_MINUTE:
          result = longVal * 60;
          break;
        case INTERVAL_SECOND:
        case INTERVAL_DAY_SECOND:
        case INTERVAL_HOUR_SECOND:
        case INTERVAL_MINUTE_SECOND:
          {
            // scale the value down and ignore fractional seconds
            for (int s=0; s<d_.scale_; s++)
              longVal /= 10;

            result = longVal;
          }
          break;
        default:
          throw UDRException(
               38900,
               "getTime() is not supported for year-month intervals");
        }
    } // intervals
  else
    {
      int stringLen = 0;
      const char *val = getRaw(row, wasNull, stringLen);
      char buf[200];
      struct tm t;
      bool ok = true;

      if (wasNull)
        return 0;


      t.tm_sec =
        t.tm_min =
        t.tm_hour =
        t.tm_mday =
        t.tm_mon =
        t.tm_year =
        t.tm_wday =
        t.tm_yday =
        t.tm_isdst = 0;

      if (stringLen+1 > sizeof(buf))
        throw UDRException(
             38900,
             "Datetime string of length %d exceeds size limit of %d for time_t conversion",
             stringLen, (int) sizeof(buf) - 1);
      memcpy(buf, val, stringLen);
      buf[stringLen] = 0;

      switch (d_.sqlType_)
        {
        case DATE:
          // yyyy-mm-dd
          ok = (sscanf(buf,"%4d-%2d-%2d", &t.tm_year, &t.tm_mon, &t.tm_mday) == 3);
          result = mktime(&t);
          break;

        case TIME:
          // hh:mm:ss
          ok = (sscanf(buf,"%2d:%2d:%2d", &t.tm_hour, &t.tm_min, &t.tm_sec) == 3);
          result = 3600 * t.tm_hour + 60 * t.tm_min + t.tm_sec;
          break;

        case TIMESTAMP:
          // yy-mm-dd hh:mm:ss
          ok = (sscanf(buf,"%4d-%2d-%2d %2d:%2d:%2d", &t.tm_year, &t.tm_mon, &t.tm_mday,
                                                      &t.tm_hour, &t.tm_min, &t.tm_sec) == 6);
          result = mktime(&t);
          break;

        default:
          throw UDRException(38900,
                             "getTime() not supported for SQL type %d",
                             d_.sqlType_);
        }

      if (!ok)
        throw UDRException(
             38900,
             "Unable to parse datetime string %s for conversion to time_t",
             buf);

      // catch errors returned by mktime
      if (result < 0)
        throw UDRException(
             38900,
             "Unable to convert datetime string %s to time_t",
             buf);
    }

  return result;
}

bool TypeInfo::getBoolean(const char *row, bool &wasNull) const
{
  switch (getSQLTypeClass())
    {
    case CHARACTER_TYPE:
      {
        int byteLen = 0;
        const char *cval = getRaw(row, wasNull, byteLen);

        while (byteLen > 0 && cval[byteLen-1] == ' ')
          byteLen--;

        // strings must have a value of "0" or "1"
        if (byteLen == 1 &&
            (cval[0] == '0' ||
             cval[0] == '1'))
          return (cval[0] == '1');
        else
          {
            std::string errval(cval, (byteLen > 10 ? 10 : byteLen));

            throw UDRException(
                 38900,
                 "getBoolean() encountered string value %s, booleans must be 0 or 1",
                 errval.c_str());
          }
      }
      break;

    case NUMERIC_TYPE:
    case BOOLEAN_TYPE:
      {
        // numerics or booleans must have a value of 0 or 1
        long lval = getLong(row, wasNull);
        if (lval <0 || lval > 1)
          throw UDRException(
             38900,
             "getBoolean() encountered value %ld, booleans must be 0 or 1",
             lval);
        return (lval != 0);
      }
      break;

    default:
      {
        std::string typeName;

        toString(typeName, false);
        throw UDRException(
             38900,
             "getBoolean() not supported for type %s",
             typeName.c_str());
      }
    }
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
    case BLOB:
    case CLOB:
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
                         "getString()/getRaw() not supported for SQL type %d",
                         d_.sqlType_);
      break;

    default:
      byteLen = d_.length_;
      if (d_.sqlType_ == CHAR)
        switch (d_.charset_)
          {
          case CHARSET_ISO88591:
          case CHARSET_UTF8:
            // trim trailing blanks from the value
            while (byteLen > 0 && result[byteLen-1] == ' ')
              byteLen--;
            break;
          case CHARSET_UCS2:
            // trim trailing little-endian UCS2 blanks
            // from the value
            while (byteLen > 1 &&
                   reinterpret_cast<const unsigned short *>(result)[byteLen/2-1] == (unsigned short) ' ')
              byteLen -= 2;
            break;
          default:
            throw UDRException(
                 38900,
                 "Unsupported character set in TupleInfo::getRaw(): %d",
                 d_.charset_);
          }
      break;
    }

  return result;
}

bool TypeInfo::isAvailable() const
{
  return (d_.dataOffset_ >= 0);
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
  char *data = row + d_.dataOffset_;

  // convert NUMERIC to the corresponding type with binary precision
  if (d_.sqlType_ == NUMERIC ||
      d_.sqlType_ == NUMERIC_UNSIGNED ||
      d_.sqlType_ == INTERVAL)
    {
      if (d_.length_ == 1)
        if (d_.sqlType_ == NUMERIC_UNSIGNED)
          tempSQLType = TINYINT_UNSIGNED;
        else
          tempSQLType = TINYINT;
      else if (d_.length_ == 2)
        if (d_.sqlType_ == NUMERIC_UNSIGNED)
          tempSQLType = SMALLINT_UNSIGNED;
        else
          tempSQLType = SMALLINT;
      else if (d_.length_ == 4)
        if (d_.sqlType_ == NUMERIC_UNSIGNED)
          tempSQLType = INT_UNSIGNED;
        else
          tempSQLType = INT;
      else if (d_.length_ == 8)
        tempSQLType = LARGEINT;
        // unsigned 8 byte integer is not supported
    }

  switch (tempSQLType)
    {
    case TINYINT:
      if (val < SCHAR_MIN || val > SCHAR_MAX)
        throw UDRException(
             38900,
             "Overflow/underflow when assigning %ld to TINYINT type",
             val);
      *((char *) data) = val;
      break;

    case SMALLINT:
      if (val < SHRT_MIN || val > SHRT_MAX)
        throw UDRException(
             38900,
             "Overflow/underflow when assigning %ld to SMALLINT type",
             val);
      *((short *) data) = val;
      break;

    case INT:
      if (val < INT_MIN || val > INT_MAX)
        throw UDRException(
             38900,
             "Overflow/underflow when assigning %ld to INTEGER type",
             val);
      *((int *) data) = val;
      break;

    case LARGEINT:
      *((long *) data) = val;
      break;

    case TINYINT_UNSIGNED:
      if (val < 0 || val > UCHAR_MAX)
        throw UDRException(
             38900,
             "Overflow/underflow when assigning %ld to TINYINT UNSIGNED type",
             val);
      *((unsigned char *) data) = val;
      break;

    case SMALLINT_UNSIGNED:
      if (val < 0 || val > USHRT_MAX)
        throw UDRException(
             38900,
             "Overflow/underflow when assigning %ld to SMALLINT UNSIGNED type",
             val);
      *((unsigned short *) data) = val;
      break;

    case INT_UNSIGNED:
      if (val < 0 || val > UINT_MAX)
        throw UDRException(
             38900,
             "Overflow/underflow when assigning %ld to INTEGER UNSIGNED type",
             val);
      *((unsigned int *) data) = val;
      break;

    case DECIMAL_LSE:
    case DECIMAL_UNSIGNED:
      {
        bool overflow = false;
        bool isNegative = false;
        int remainingLength = d_.length_;
        int neededLengthWithoutSign = d_.precision_ + (d_.scale_ > 0 ? 1 : 0);
        char buf[20];
        int remainingBufLength = sizeof(buf);
        char *bufPtr = buf;

        const long maxvals[] = {9L,
                                99L,
                                999L,
                                9999L,
                                99999L,
                                999999L,
                                9999999L,
                                99999999L,
                                999999999L,
                                9999999999L,
                                99999999999L,
                                999999999999L,
                                9999999999999L,
                                99999999999999L,
                                999999999999999L,
                                9999999999999999L,
                                99999999999999999L,
                                999999999999999999L};

        if (d_.precision_ < 1 || d_.precision_ > 18 ||
            d_.scale_ < 0 || d_.scale_ > d_.precision_)
          throw UDRException(
               38900, "Invalid precision (%d) or scale (%d) for a decimal data type",
               d_.precision_, d_.scale_);

        // right now precision is limited to 18, but may need to use this code for BigNum
        // so we add a check for this future case
        if (d_.length_ >=  sizeof(buf))
          throw UDRException(
               38900, "Decimal precision %d is not supported by setLong(), limit is %d",
               d_.precision_, sizeof(buf));

        if (val < 0)
          {
            if (tempSQLType == DECIMAL_UNSIGNED)
              throw UDRException(
                   38900,
                   "Trying to assign a negative value to a DECIMAL UNSIGNED type");
            val = -val;
            isNegative = true;
            *bufPtr = '-';
            bufPtr++;
            remainingLength--;
            remainingBufLength--;
          }

        // add enough blanks to print the number right-adjusted
        while (neededLengthWithoutSign < remainingLength)
          {
            *bufPtr = ' ';
            bufPtr++;
            remainingLength--;
            remainingBufLength--;
          }

        // sanity check, d_.length_ should have enough space for sign,
        // precision and decimal point
        if (remainingLength < neededLengthWithoutSign)
          throw UDRException(
               38900,
               "Internal error, field length too short in setLong() (%d, %d)",
               remainingLength, neededLengthWithoutSign);

        // validate limits for decimal precision
        if (val > maxvals[d_.precision_-1])
          throw UDRException(
               38900,
               "Overflow/underflow occurred while converting value %ld to a DECIMAL(%d, %d)",
               val, d_.precision_, d_.scale_);

        if (d_.scale_ == 0)
          {
            snprintf(bufPtr, remainingBufLength, "%0*ld", d_.precision_, val);
          }
        else
          {
            long fractionalValue = 0;
            long multiplier = 1;

            for (int s=0; s<d_.scale_; s++)
              {
                fractionalValue += multiplier * (val % 10);
                val /= 10;
                multiplier *= 10;
              }
            snprintf(bufPtr, remainingBufLength,
                     "%0*ld.%0*ld",
                     d_.precision_-d_.scale_,
                     val,
                     d_.scale_,
                     fractionalValue);
          }
        // snprintf put a terminating NUL byte into the string,
        // which is not allowed in the actual record, copy the
        // part without this extra byte into the record
        memcpy(data, buf, d_.length_);
      }
      break;

    case REAL:
    case DOUBLE_PRECISION:
      setDouble(val, row);
      break;

    case BOOLEAN:
      *((char *) data) = val;
      if (val != 0 && val != 1)
        throw UDRException(
             38900,
             "Got value %ld which cannot be converted to a BOOLEAN, only 0 or 1 are allowed",
             val);
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
      // we are not testing for underflow at this point
      if (val > FLT_MAX || val < -FLT_MAX)
        throw UDRException(
             38900,
             "Overflow when assigining to REAL type");
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

void TypeInfo::setTime(time_t val, char *row) const
{
  if (d_.sqlType_ == INTERVAL)
    {
      long tVal = static_cast<long>(val);
      long result = 0;

      // convert the time_t value to the base units of the interval

      // NOTE: This relies on the assumption that time_t
      //       uses seconds as its unit, which is true for
      //       current Linux systems but may not always remain
      //       true. It may also some day become bigger than long.
      switch (d_.intervalCode_)
        {
        case INTERVAL_DAY:
          result = tVal/86400;
          break;
        case INTERVAL_HOUR:
        case INTERVAL_DAY_HOUR:
          result = tVal/3600;
          break;
        case INTERVAL_MINUTE:
        case INTERVAL_DAY_MINUTE:
        case INTERVAL_HOUR_MINUTE:
          result = tVal/60;
          break;
        case INTERVAL_SECOND:
        case INTERVAL_DAY_SECOND:
        case INTERVAL_HOUR_SECOND:
        case INTERVAL_MINUTE_SECOND:
          {
            // scale the value up
            for (int s=0; s<d_.scale_; s++)
              tVal *= 10;

            result = tVal;
          }
          break;
        default:
          throw UDRException(
               38900,
               "getTime() is not supported for year-month intervals");
        }

      setLong(result, row);
    } // intervals
  else
    {
      struct tm t;
      time_t temp = val;
      char buf[64];
      const char *fraction = ".000000";
      int strLimit = sizeof(buf) - strlen(fraction);

      if (gmtime_r(&temp, &t) != &t)
        throw UDRException(
             38900,
             "Unable to interpret time_t value %ld",
             (long) val);

      switch (d_.sqlType_)
        {
        case DATE:
          // yyyy-mm-dd
          snprintf(buf, strLimit, "%04d-%02d-%02d", t.tm_year, t.tm_mon, t.tm_mday);
          break;

        case TIME:
          // hh:mm:ss
          snprintf(buf, strLimit, "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
          break;

        case TIMESTAMP:
          // yyyy-mm-d hh:mm:ss
          snprintf(buf, strLimit, "%04d-%02d-%02d %02d:%02d:%02d",
                   t.tm_year, t.tm_mon, t.tm_mday,
                   t.tm_hour, t.tm_min, t.tm_sec);
          break;

        default:
          throw UDRException(38900,
                             "setTime() not supported for SQL type %d",
                             d_.sqlType_);
        }

      // add fraction (with value 0) if needed
      if (d_.scale_ > 0)
        strncat(buf, fraction, d_.scale_+1);

      setString(buf, strlen(buf), row);
    } // types other than intervals
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

  switch (d_.sqlType_)
    {
    case CHAR:
    case VARCHAR:
    case DATE:
    case TIME:
    case TIMESTAMP:
    case BLOB:
    case CLOB:
      // a string overflow will raise an exception
      if (stringLen > d_.length_)
        // should probably check whether this is a CHAR or datetime
        // and the excess characters are all blanks
        throw UDRException(
             38900,
             "setString() with a string of length %d on a column with length %d",
             stringLen,
             d_.length_);

      // for these types, copy the string and pad with blanks
      // of the appropriate charset for fixed-length strings
      memcpy(data, val, stringLen);

      if (d_.sqlType_ == VARCHAR ||
          d_.sqlType_ == BLOB ||
          d_.sqlType_ == CLOB)
        {
          // set the varchar length indicator
          if (d_.vcLenIndOffset_ < 0)
            throw UDRException(38900,
                               "Internal error, VARCHAR/BLOB/CLOB without length indicator");

          if (d_.flags_ & TYPE_FLAG_4_BYTE_VC_LEN)
            *(reinterpret_cast<int32_t *>(row + d_.vcLenIndOffset_)) =
              stringLen;
          else
            *(reinterpret_cast<int16_t *>(row + d_.vcLenIndOffset_)) =
              stringLen;
        }
      else if (stringLen < d_.length_)
        // fill fixed character value with blanks of the appropriate
        // character set
        switch (d_.charset_)
          {
          case CHARSET_ISO88591:
          case CHARSET_UTF8:
            memset(data+stringLen, ' ', d_.length_ - stringLen);
            break;
          case CHARSET_UCS2:
            {
              int paddedLen = stringLen;

              // pad with little-endian UCS-2 blanks
              while (paddedLen+1 < d_.length_)
                {
                  reinterpret_cast<unsigned short *>(data)[paddedLen/2] = (unsigned short) ' ';
                  paddedLen += 2;
                }
            }
            break;
          default:
            throw UDRException(
                 38900,
                 "Unsupported character set in TupleInfo::setString(): %d",
                 d_.charset_);
          }

      break;

    case REAL:
    case DOUBLE_PRECISION:
      isApproxNumeric = true;
      // fall through to next case

    case TINYINT:
    case SMALLINT:
    case INT:
    case LARGEINT:
    case NUMERIC:
    case DECIMAL_LSE:
    case TINYINT_UNSIGNED:
    case SMALLINT_UNSIGNED:
    case INT_UNSIGNED:
    case NUMERIC_UNSIGNED:
    case DECIMAL_UNSIGNED:
      {
        char buf[200];
        long lval = 0;
        double dval = 0.0;
        int numCharsConsumed = 0;
        int rc = 0;
        int vScale = 0;
        bool sawMinusDot = false;

        // ignore trailing blanks
        while (val[stringLen-1] == ' ' ||
               val[stringLen-1] == '\t')
          stringLen--;

        if (stringLen+1 > sizeof(buf))
          throw UDRException(
               38900,
               "String of length %d exceeds size limit of %d for numeric conversion",
               stringLen, (int) sizeof(buf) - 1);

        // copy the value to be able to add a terminating NUL byte
        memcpy(buf, val, stringLen);
        buf[stringLen] = 0;

        if (isApproxNumeric)
          rc = sscanf(buf,"%lf%n", &dval, &numCharsConsumed);
        else
          rc = sscanf(buf,"%ld%n", &lval, &numCharsConsumed);

        if (rc <= 0)
          {
            bool isOK = false;

            // could not read a long or float value, this could be an error
            // or a number that starts with '.' or '-.' with optional
            // leading white space. Check for this special case before
            // raising an exception.
            if (!isApproxNumeric && d_.scale_ > 0 && numCharsConsumed == 0)
              {
                while (buf[numCharsConsumed] == ' ' ||
                       buf[numCharsConsumed] == '\t')
                  numCharsConsumed++;
                if (buf[numCharsConsumed] == '-' &&
                    buf[numCharsConsumed+1] == '.')
                  {
                    // the number starts with "-.", remember
                    // to negate it at the end and go on
                    sawMinusDot = true;
                    numCharsConsumed++; // skip over the '-'
                    isOK = true;
                  }
                else if (buf[numCharsConsumed] == '.')
                  {
                    // the number starts with '.', that's
                    // ok, continue on
                    isOK = true;
                  }
              }

            if (!isOK)
              throw UDRException(
                   38900,
                   "Error in setString(), \"%s\" is not a numeric value",
                   buf);
          }

        if (d_.scale_ > 0 && !isApproxNumeric)
          {
            if (buf[numCharsConsumed] == '.')
              {
                int sign = (lval < 0 ? -1 : 1);

                // skip over the decimal dot
                numCharsConsumed++;

                // process digits following the dot
                while (numCharsConsumed < stringLen &&
                       buf[numCharsConsumed] >= '0' &&
                       buf[numCharsConsumed] <= '9' &&
                       lval <= LONG_MAX/10)
                  {
                    lval = 10*lval +
                      ((int)(buf[numCharsConsumed++] - '0')) * sign;
                    vScale++;
                  }
              }

            if (sawMinusDot)
              lval = -lval;

            while (vScale < d_.scale_)
              if (lval <= LONG_MAX/10)
                {
                  lval *= 10;
                  vScale++;
                }
              else
                throw UDRException(
                     38900,
                     "Error in setString(): Value %s exceeds range for a long",
                     buf);

            if (vScale > d_.scale_)
              throw UDRException(
                     38900,
                     "Error in setString(): Value %s exceeds scale %d",
                     buf, d_.scale_);
          }

        // check for any non-white space left after conversion
        while (numCharsConsumed < stringLen)
          if (buf[numCharsConsumed] != ' ' &&
              buf[numCharsConsumed] != '\t')
            throw UDRException(
                 38900,
                 "Error in setString(): \"%s\" is not a valid, in-range numeric value",
                 buf);
          else
            numCharsConsumed++;

        if (isApproxNumeric)
          setDouble(dval, row);
        else
          setLong(lval, row);
      }
      break;

    case INTERVAL:
      {
        char buf[100];
        char *strVal;
        unsigned long years, months, days, hours, minutes, seconds, singleField;
        long result = 0;
        unsigned long fractionalVal = 0;
        int numLeaderChars   = 0;
        int numCharsConsumed = 0;
        int numFractionChars = 0;
        bool ok = true;
        bool readFraction = false;
        bool isNegative = false;

        // ignore trailing blanks
        while (val[stringLen-1] == ' ')
          stringLen--;

        if (stringLen+1 > sizeof(buf))
          throw UDRException(
               38900,
               "String of length %d exceeds size limit of %d for interval conversion",
               stringLen, (int) sizeof(buf) - 1);

        // copy the value to be able to add a terminating NUL byte
        memcpy(buf, val, stringLen);
        buf[stringLen] = 0;
        strVal = buf;

        // check for the sign
        while (*strVal == ' ')
          strVal++;
        if (*strVal == '-')
          {
            isNegative = true;
            strVal++;
          }

        numLeaderChars = strVal - buf;

        // Use sscanf to convert string representation to a number.
        // Note that this does not check for overflow, which cannot occur
        // for valid interval literals, and that it also may allow some
        // string that aren't quite legal, such as 01:120:00 (should be 03:00:00).
        // We treat such overflows like other overflows that could occur in the
        // user-written code.

        switch (d_.intervalCode_)
          {
          case INTERVAL_YEAR:
          case INTERVAL_MONTH:
          case INTERVAL_DAY:
          case INTERVAL_HOUR:
          case INTERVAL_MINUTE:
            ok = (sscanf(strVal, "%lu%n", &singleField, &numCharsConsumed) >= 1);
            result = singleField;
            break;
          case INTERVAL_SECOND:
            ok = (sscanf(strVal, "%lu%n", &singleField, &numCharsConsumed) >= 1);
            result = singleField;
            readFraction = true;
            break;
          case INTERVAL_YEAR_MONTH:
            ok = (sscanf(strVal, "%lu-%lu%n", &years, &months, &numCharsConsumed) >= 2);
            result = years * 12 + months;
            break;
          case INTERVAL_DAY_HOUR:
            ok = (sscanf(strVal, "%lu %lu%n", &days, &hours, &numCharsConsumed) >= 2);
            result = days * 24 + hours;
            break;
          case INTERVAL_DAY_MINUTE:
            ok = (sscanf(strVal, "%lu %lu:%lu%n", &days, &hours, &minutes, &numCharsConsumed) >= 3);
            result = days * 1440 + hours * 60 + minutes;
            break;
          case INTERVAL_DAY_SECOND:
            ok = (sscanf(strVal, "%lu %lu:%lu:%lu%n", &days, &hours, &minutes, &seconds, &numCharsConsumed) >= 4);
            result = days * 86400 + hours * 3600 + minutes * 60 + seconds;
            readFraction = true;
            break;
          case INTERVAL_HOUR_MINUTE:
            ok = (sscanf(strVal, "%lu:%lu%n", &hours, &minutes, &numCharsConsumed) >= 2);
            result = hours * 60 + minutes;
            break;
          case INTERVAL_HOUR_SECOND:
            ok = (sscanf(strVal, "%lu:%lu:%lu%n", &hours, &minutes, &seconds, &numCharsConsumed) >= 3);
            result = hours * 3600 + minutes * 60 + seconds;
            readFraction = true;
            break;
          case INTERVAL_MINUTE_SECOND:
            ok = (sscanf(strVal, "%lu:%lu%n", &minutes, &seconds, &numCharsConsumed) >= 2);
            result = minutes * 60 + seconds;
            readFraction = true;
            break;
          default:
            throw UDRException(
                 38900,
                 "Invalid interval code in TupleInfo::setString()");
          }

        strVal += numCharsConsumed;

        // allow fractional seconds, regardless of whether fraction precision is >0
        if (ok && readFraction && *strVal == '.')
          {
            ok = (sscanf(strVal,".%ld%n", &fractionalVal, &numFractionChars) >= 1);
            strVal += numFractionChars;

            // then, if the fractional seconds are not 0, complain if fraction
            // precision is 0.
            if (fractionalVal > 0 && d_.scale_ == 0)
              throw UDRException(
                   38900,
                   "Encountered a fractional second part in a string value for an interval type that doesn't allow fractional values: %s",
                   buf);
          }

        if (!ok)
          throw UDRException(
               38900,
               "Error in setString(), \"%s\" is not an interval value for interval code %d",
               buf, d_.intervalCode_);

        // check for any non-white space left after conversion
        while (strVal - buf < stringLen)
          if (*strVal != ' ' &&
              *strVal != '\t')
            throw UDRException(
                 38900,
                 "Found non-numeric character in setString for an interval column: %s",
                 buf);
          else
            strVal++;

        if (d_.scale_ > 0)
          {
            long fractionOverflowTest = fractionalVal;

            // scale up the result
            for (int s=0; s<d_.scale_; s++)
              {
                result *= 10;
                fractionOverflowTest /= 10;
              }

            if (fractionOverflowTest != 0)
              throw UDRException(
                   38900,
                   "Fractional value %ld exceeds allowed range for interval fraction precision %d",
                   fractionalVal, d_.scale_);

            // add whole and fractional seconds (could overflow in extreme cases)
            result += fractionalVal;
          }

        // could overflow in extreme cases
        if (isNegative)
          result = -result;

        // result could exceed allowed precision, will cause an executor error when processed further
        setDouble(result, row);
      }
      break;

    case BOOLEAN:
      {
        long bval = -1;
        // accept 0, 1, TRUE, true, FALSE, false as values
        while (stringLen > 0 && val[stringLen-1] == ' ')
          stringLen--;
        if (stringLen == 1)
          {
            if (strcmp(val, "0") == 0)
              bval = 0;
            else if (strcmp(val, "1") == 0)
              bval = 1;
          }
        if (stringLen == 4)
          {
            if (strcmp(val, "TRUE") == 0 ||
                strcmp(val, "true") == 0)
              bval = 1;
          }
        else if (stringLen == 5)
          if (strcmp(val, "FALSE") == 0 ||
              strcmp(val, "false") == 0)
            bval = 0;

        if (bval >= 0)
          setLong(bval, row);
        else
          throw UDRException(38900,
                             "Invalid value %.10s encountered in setString() for a boolean data type",
                             val);
      }
      break;

    case UNDEFINED_SQL_TYPE:
    default:
      throw UDRException(38900,
                         "setString() is not yet supported for data type %d",
                         d_.sqlType_);
    }
}

void TypeInfo::setBoolean(bool val, char *row) const
{
  switch (getSQLTypeClass())
    {
    case CHARACTER_TYPE:
      setString((val ? "1" : "0"), 1, row);
      break;

    case NUMERIC_TYPE:
    case BOOLEAN_TYPE:
      setLong((val ? 1 : 0), row);
      break;

    default:
      {
        std::string typeName;

        toString(typeName, false);
        throw UDRException(
             38900,
             "setBoolean() not supported for type %s",
             typeName.c_str());
      }

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

int TypeInfo::minBytesPerChar() const
{
  switch (d_.charset_)
    {
    case CHARSET_ISO88591:
    case CHARSET_UTF8:
      return 1;
    case CHARSET_UCS2:
      return 2;
    default:
      throw UDRException(
           38900, "Minimum bytes per char not defined for charset %d",
           d_.charset_);
    }
}

int TypeInfo::convertToBinaryPrecision(int decimalPrecision) const
{
  if (decimalPrecision < 1 || decimalPrecision > 18)
    throw UDRException(
         38900,
         "Decimal precision %d is out of the allowed range of 1-18",
         decimalPrecision);

  if (decimalPrecision < 3)
    return 1;
  else if (decimalPrecision < 5)
    return 2;
  else if (decimalPrecision < 10)
    return 4;
  else
    return 8;
}

void TypeInfo::toString(std::string &s, bool longForm) const
{
  char buf[100];

  switch (d_.sqlType_)
    {
    case UNDEFINED_SQL_TYPE:
      s += "undefined_sql_type";
      break;
    case TINYINT:
      s += "TINYINT";
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
               getPrecision(), getScale());
      s += buf;
      break;
    case DECIMAL_LSE:
      snprintf(buf, sizeof(buf), "DECIMAL(%d,%d)",
               getPrecision(), getScale());
      s += buf;
      break;
    case TINYINT_UNSIGNED:
      s += "TINYINT UNSIGNED";
      break;
    case SMALLINT_UNSIGNED:
      s += "SMALLINT UNSIGNED";
      break;
    case INT_UNSIGNED:
      s += "INT UNSIGNED";
      break;
    case NUMERIC_UNSIGNED:
      snprintf(buf, sizeof(buf), "NUMERIC(%d,%d) UNSIGNED",
               getPrecision(), getScale());
      s += buf;
      break;
    case DECIMAL_UNSIGNED:
      snprintf(buf, sizeof(buf), "DECIMAL(%d,%d) UNSIGNED",
               getPrecision(), getScale());
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
               getMaxCharLength(),
               (getCharset() == CHARSET_UTF8 ? " BYTES" : ""),
               csName);
      s += buf;
      break;
    case DATE:
      s += "DATE";
      break;
    case TIME:
      s += "TIME";
      if (d_.scale_ > 0)
        {
          snprintf(buf, sizeof(buf), "(%d)", d_.scale_);
          s += buf;
        }
      break;
    case TIMESTAMP:
      snprintf(buf, sizeof(buf), "TIMESTAMP(%d)", d_.scale_);
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
    case BLOB:
      s += "BLOB";
      break;
    case CLOB:
      s += "CLOB";
      break;
    case BOOLEAN:
      s += "BOOLEAN";
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

int TypeInfo::deserialize(ConstBytes &inputBuffer,
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

/**
 * Default constructor, generates unspecified provenance.
 */
ProvenanceInfo::ProvenanceInfo() :
     inputTableNum_(-1),
     inputColNum_(-1)
{}

/**
 *  Constructor to link an output column to a specific input column
 *
 *  This constructor can be used to produce a "passthru column". An easier
 *  way to do this is the UDRInvocationInfo::addPassThruColumns() method.
 *
 *  @param inputTableNum Input table number (0 for a TMUDF with a single
 *                       table-valued input, the most common case).
 *  @param inputColNum   Column number in intput table "inputTableNum"
 *                       that is the source of the output column to be
 *                       produced.
 */
ProvenanceInfo::ProvenanceInfo(int inputTableNum,
                               int inputColNum) :
     inputTableNum_(inputTableNum),
     inputColNum_(inputColNum)
{}

/**
 *  Get the input table number.
 *
 *  @return Input table number.
 */
int ProvenanceInfo::getInputTableNum() const
{
  return inputTableNum_;
}

/**
 *  Get the input column number.
 *
 *  @return Input column number.
 */
int ProvenanceInfo::getInputColumnNum() const
{
  return inputColNum_;
}

/**
 *  Test whether the column comes from any or from a specific table-valued input.
 *
 *  @param  inputTableNum -1 to test for any table-valued input, or a specific
 *          input table number.
 *  @return true if the provenance indicates a column that comes from the
 *          specified input table(s), false otherwise
 */
bool ProvenanceInfo::isFromInputTable(int inputTableNum) const
{
  return (inputTableNum_ >= 0 &&
          inputColNum_ >= 0 &&
          (inputTableNum > 0 ? inputTableNum == inputTableNum_ : true));
}

// ------------------------------------------------------------------------
// Member functions for class ColumnInfo
// ------------------------------------------------------------------------

/**
 *  Default constructor
 */
ColumnInfo::ColumnInfo() :
     TMUDRSerializableObject(COLUMN_INFO_OBJ,
                             getCurrentVersion()),
     usage_(UNKNOWN),
     estimatedUniqueEntries_(-1)
{}

/**
 *  Constructor, specifying a name and a type
 *
 *  @param name       Name of the column to add. Use UPPER CASE letters,
 *                    digits and underscore, otherwise you will need to
 *                    use delimited column names with matching case in
 *                    Trafodion.
 *  @param type       Type of the column to add.
 */
ColumnInfo::ColumnInfo(const char *name,
                       const TypeInfo &type) :
     TMUDRSerializableObject(COLUMN_INFO_OBJ,
                             getCurrentVersion()),
     name_(name),
     type_(type),
     usage_(UNKNOWN),
     estimatedUniqueEntries_(-1)
{}
  
/**
 *  Get the name of the column.
 *
 *  @return Name of the column in UTF-8.
 */
const std::string &ColumnInfo::getColName() const
{
  return name_;
}

/**
 *  Get the type of the column.
 *
 *  @return Type of the column.
 */
const TypeInfo &ColumnInfo::getType() const
{
  return type_;
}

/**
 *  Non-const method to get the type.
 *
 *  @return Non-const type of the column. Note that the
 *          types of parameters and output columns can only
 *          be changed from the
 *          UDR::describeParamsAndColumns() call.
 */
TypeInfo & ColumnInfo::getType()
{
  return type_;
}

/**
 *  Get the estimated number of unique entries.
 *
 *  This returns an estimate for the number of unique values
 *  for this column in the table. For example, a column containing
 *  the names of US states would have approximately 50 distinct
 *  values, assuming that most or all states are represented.
 *  This estimate can be provided by the UDR writer, through the
 *  setUniqueEntries() method, or in some cases it can also be
 *  provided by the Trafodion compiler.
 *
 *  @see ColumnInfo::setEstimatedUniqueEntries()
 *
 *  @return Estimated number of unique entries or -1 if there is no estimate.
 */
long ColumnInfo::getEstimatedUniqueEntries() const
{
  return estimatedUniqueEntries_;
}

/**
 *  Get the usage of an input or output column.
 *
 *  This usage may be set in the
 *  UDR::describeDataflowAndPredicates() method,
 *  set automatically by Trafodion for certain situations
 *  with passthru columns, or left at the default of USED.
 *
 *  @return Usage enum value for the column.
 */
ColumnInfo::ColumnUseCode ColumnInfo::getUsage() const        {
  return usage_;
}

/**
 *  Get provenance info for an output column.
 *
 *  @return Provenance of the column.
 */
const ProvenanceInfo &ColumnInfo::getProvenance() const
{
  return provenance_;
}

/**
 *  Set the name of the column.
 *
 *  @param colName Name of the column (in UTF-8). There is a length
 *         limit of 256 bytes for the column name.
 */
void ColumnInfo::setColName(const char *colName)
{
  name_ = colName;
}

/**
 *  Set the type of the column.
 *
 *  This is done by constructing a TypeInfo object and passing it to this method.
 *
 *  @param type Type of the column.
 */
void ColumnInfo::setType(TypeInfo &type)
{
  type_ = type;
}

/**
 *  Provide an estimate for the number of unique values of a column.
 *
 *  Only use this method from within the following methods:
 *  @arg UDR::describeParamsAndColumns()
 *  @arg UDR::describeDataflowAndPredicates()
 *  @arg UDR::describeConstraints()
 *  @arg UDR::describeStatistics()
 *
 *  @see ColumnInfo::getEstimatedUniqueEntries()
 *
 *  @param uniqueEntries Estimate of the number of unique entries or
 *         -1 if there is no estimate.
 */
void ColumnInfo::setEstimatedUniqueEntries(long uniqueEntries)
{
  estimatedUniqueEntries_ = uniqueEntries;
}

/**
 *  Set the usage of the column.
 *
 *  See the ColumnInfo::COLUMN_USE enum for different options.
 *
 *  Only use this method from within the following method:
 *  @arg UDR::describeParamsAndColumns()
 *
 *  @param usage Usage enum value of the column.
 */
void ColumnInfo::setUsage(ColumnUseCode usage)
{
  usage_ = usage;
}

/**
 *  Set the provenance of an output column.
 *
 *  This defines a relationship between an output column and
 *  a column of a table-valued input from which the output value
 *  is copied. Such columns are called pass-thru columns. See
 *  class ProvenanceInfo for more information.
 *
 *  Only use this method from within the following method:
 *  @arg UDR::describeParamsAndColumns()
 *
 *  @param provenance The provenance information.
 */
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
      if (provenance_.isFromInputTable())
        {
          char buf[100];

          snprintf(buf, sizeof(buf), " passthru(%d,%d)",
                   provenance_.getInputTableNum(),
                   provenance_.getInputColumnNum());
          s += buf;
        }
      switch (usage_)
        {
        case UNKNOWN:
        case USED:
          // don't show anything for these "normal" cases
          break;
        case NOT_USED:
          s+= " (not used)";
          break;
        case NOT_PRODUCED:
          s+= " (not produced)";
          break;
        default:
          s+= " (invalid usage code)";
          break;
        }
      if (estimatedUniqueEntries_ >= 0)
        {
          char buf[40];

          snprintf(buf, sizeof(buf), " uec=%ld", estimatedUniqueEntries_);
          s+= buf;
        }
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

  result += serializeLong(estimatedUniqueEntries_,
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

int ColumnInfo::deserialize(ConstBytes &inputBuffer,
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
  usage_ = static_cast<ColumnUseCode>(tempInt1);

  result += deserializeLong(estimatedUniqueEntries_,
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

ConstraintInfo::ConstraintInfo(ConstraintTypeCode constraintType,
                               unsigned short version) :
     TMUDRSerializableObject(
          (constraintType == CARDINALITY ?
              TMUDRSerializableObject::CARDINALITY_CONSTRAINT_INFO_OBJ :
           (constraintType == UNIQUE ?
              TMUDRSerializableObject::UNIQUE_CONSTRAINT_INFO_OBJ :
              TMUDRSerializableObject::UNKNOWN_OBJECT_TYPE)),
          version),
     constraintType_(constraintType)
{
  if (getObjectType() == TMUDRSerializableObject::UNKNOWN_OBJECT_TYPE)
    throw UDRException(
         38900,
         "Invalid subclass in ConstraintInfo() constructor");
}

/**
 *  Get the type of the constraint.
 *
 *  This allows safe casting to derived classes, based on the type.
 *
 *  @return Type of the constraint.
 */
ConstraintInfo::ConstraintTypeCode ConstraintInfo::getType() const
{
  return constraintType_;
}

int ConstraintInfo::serializedLength()
{
  return TMUDRSerializableObject::serializedLength() +
    serializedLengthOfInt();
}

int ConstraintInfo::serialize(Bytes &outputBuffer,
                              int &outputBufferLength)
{
  int result = 
    TMUDRSerializableObject::serialize(outputBuffer,
                                       outputBufferLength);

  result += serializeInt(static_cast<int>(constraintType_),
                         outputBuffer,
                         outputBufferLength);
  return result;
}

int ConstraintInfo::deserialize(ConstBytes &inputBuffer,
                                int &inputBufferLength)
{
  int result =
    TMUDRSerializableObject::deserialize(inputBuffer, inputBufferLength);
  int tempInt = 0;

  result += deserializeInt(tempInt,
                   inputBuffer,
                   inputBufferLength);
  constraintType_ = static_cast<ConstraintTypeCode>(tempInt);

  return result;
}

// ------------------------------------------------------------------------
// Member functions for class CardinalityConstraintInfo
// ------------------------------------------------------------------------

/**
 *  Construct a new cardinality constraint.
 *
 *  A cardinality constraint allows to specify a lower and/or an upper
 *  limit for the number of rows in a table.
 *
 *  @param minNumRows The minimum number of rows in the table, 0 or
 *                    a positive number.
 *  @param maxNumRows The maximum number of rows in the table, or -1
 *                    if there is no upper bound. If it is not -1, maxNumRows
 *                    must be greater or equal minNumRows.
 *  @throws UDRException
 */
CardinalityConstraintInfo::CardinalityConstraintInfo(long minNumRows,
                                                     long maxNumRows) :
     ConstraintInfo(CARDINALITY, getCurrentVersion()),
     minNumRows_(minNumRows),
     maxNumRows_(maxNumRows)
{
  if (minNumRows < 0 ||
      maxNumRows < -1 ||
      maxNumRows >= 0 && minNumRows > maxNumRows)
    throw UDRException(
         38900,
         "Invalid lower/upper bound for cardinality constraint: (%ld, %ld)",
         minNumRows, maxNumRows);
}

/**
 *  Return the minimum number of rows in a table.
 *
 *  @return Minimum number of rows (0 or a positive number).
 */
long CardinalityConstraintInfo::getMinNumRows() const
{
  return minNumRows_;
}

/**
 *  Return the maximum number of rows in a table.
 *
 *  @return Maximum number of rows or -1 if there is no upper bound.
 */
long CardinalityConstraintInfo::getMaxNumRows() const
{
  return maxNumRows_;
}

void CardinalityConstraintInfo::toString(const TableInfo &,
                                         std::string &s)
{
  char buf[100];

  snprintf(buf, sizeof(buf), "cardinality constraint(min=%ld, max=%ld)",
           minNumRows_, maxNumRows_);
  s += buf;
}

int CardinalityConstraintInfo::serializedLength()
{
  return ConstraintInfo::serializedLength() +
    2 * serializedLengthOfLong();
}

int CardinalityConstraintInfo::serialize(Bytes &outputBuffer,
                              int &outputBufferLength)
{
  int result = 
    ConstraintInfo::serialize(outputBuffer,
                              outputBufferLength);

  result += serializeLong(minNumRows_,
                          outputBuffer,
                          outputBufferLength);
  result += serializeLong(maxNumRows_,
                          outputBuffer,
                          outputBufferLength);

  validateSerializedLength(result);

  return result;
}

int CardinalityConstraintInfo::deserialize(ConstBytes &inputBuffer,
                                int &inputBufferLength)
{
  int result =
    ConstraintInfo::deserialize(inputBuffer, inputBufferLength);

  validateObjectType(CARDINALITY_CONSTRAINT_INFO_OBJ);

  result += deserializeLong(minNumRows_,
                            inputBuffer,
                            inputBufferLength);
  result += deserializeLong(maxNumRows_,
                            inputBuffer,
                            inputBufferLength);

  validateDeserializedLength(result);

  return result;
}


// ------------------------------------------------------------------------
// Member functions for class UniqueConstraintInfo
// ------------------------------------------------------------------------

/**
 *  Default constructor for an empty uniqueness constraint.
 *
 *  Use method addColumn() to add columns.
 */
UniqueConstraintInfo::UniqueConstraintInfo() :
     ConstraintInfo(UNIQUE,
                    getCurrentVersion())
{}

/**
 *  Get the number of columns that form the unique key.
 *
 *  @return Number of columns in the uniqueness constraint.
 */
int UniqueConstraintInfo::getNumUniqueColumns() const
{
  return uniqueColumns_.size();
}

/**
 *  Get a column of the uniqueness constraint by iterator.
 *
 *  Like in other methods, we use an integer to iterate over the
 *  columns in the set. Note that the columns form a set, so this
 *  number i is merely there to iterate over the set of columns.
 *
 *  @param i A number between 0 and getNumUniqueColumns()-1. 
 *  @return Column number/ordinal of the unique column.
 *  @throws UDRException
 */
int UniqueConstraintInfo::getUniqueColumn(int i) const
{
  if (i < 0 || i >= uniqueColumns_.size())
    throw UDRException(
         38900,
         "Invalid index in getUniqueColumn: %d, has %d columns",
         i, static_cast<int>(uniqueColumns_.size()));
  return uniqueColumns_[i];
}

/**
 *  Add a column to a uniqueness constraint.
 *
 *  @param c Column number/ordinal of one of the unique columns in the
 *           constraint.
 */
void UniqueConstraintInfo::addColumn(int c)
{
  std::vector<int>::iterator it;

  // insert columns ordered by number and ignore duplicates

  // skip over any elements < c
  for (it = uniqueColumns_.begin();
       it != uniqueColumns_.end() && *it < c;
       it++)
    ;

  // insert at the current position if c is not already in the list
  if (it == uniqueColumns_.end() || *it > c)
    uniqueColumns_.insert(it, c);
}

void UniqueConstraintInfo::toString(const TableInfo &ti, std::string &s)
{
  s += "unique(";
  for (int c=0; c<uniqueColumns_.size(); c++)
    {
      if (c>0)
        s +=  ", ";

      s += ti.getColumn(uniqueColumns_[c]).getColName();
    }
  s += ")";
}

int UniqueConstraintInfo::serializedLength()
{
  return ConstraintInfo::serializedLength() +
    serializedLengthOfBinary(uniqueColumns_.size() * sizeof(int));
}

int UniqueConstraintInfo::serialize(Bytes &outputBuffer,
                                    int &outputBufferLength)
{
  int result = 
    ConstraintInfo::serialize(outputBuffer,
                              outputBufferLength);
  int numCols = uniqueColumns_.size();
  int *cols = new int[numCols];

  for (int u=0; u<numCols; u++)
    cols[u] = uniqueColumns_[u];

  result += serializeBinary(cols,
                            numCols * sizeof(int),
                            outputBuffer,
                            outputBufferLength);
  delete cols;

  validateSerializedLength(result);

  return result;
}

int UniqueConstraintInfo::deserialize(ConstBytes &inputBuffer,
                                      int &inputBufferLength)
{
  int result =
    ConstraintInfo::deserialize(inputBuffer, inputBufferLength);
  int numCols;
  int *cols;
  int binaryLength;

  validateObjectType(UNIQUE_CONSTRAINT_INFO_OBJ);

  result += deserializeBinary((const void **)&cols,
                              binaryLength,
                              false,
                              inputBuffer,
                              inputBufferLength);
  numCols = binaryLength / sizeof(int);
  for (int u=0; u<numCols; u++)
    uniqueColumns_.push_back(cols[u]);

  validateDeserializedLength(result);

  return result;
}


// ------------------------------------------------------------------------
// Member functions for class PredicateInfo
// ------------------------------------------------------------------------

PredicateInfo::PredicateInfo(TMUDRObjectType t) :
     TMUDRSerializableObject(t, getCurrentVersion()),
     evalCode_(UNKNOWN_EVAL),
     operator_(UNKNOWN_OP)
{}

/**
 *  Get evaluation code for a predicate.
 *
 *  @return Evaluation code.
 *  @throws UDRException
 */
PredicateInfo::EvaluationCode PredicateInfo::getEvaluationCode() const
{
  return static_cast<EvaluationCode>(evalCode_);
}

/**
 *  Get operator code for a predicate.
 *
 *  @return Operator code.
 *  @throws UDRException
 */
PredicateInfo::PredOperator PredicateInfo::getOperator() const
{
  return operator_;
}

/**
 *  Check whether this predicate is a comparison predicate.
 *
 *  Use this method to determine  whether it is safe to cast the object
 *  to class ComparisonPredicateInfo.

 *  @return true if predcate i is a comparison predicate, false otherwise.
 */
bool PredicateInfo::isAComparisonPredicate() const
{
  switch (operator_)
    {
    case EQUAL:
    case NOT_EQUAL:
    case LESS:
    case LESS_EQUAL:
    case GREATER:
    case GREATER_EQUAL:
      return true;

    default:
      return false;
    }
}

void PredicateInfo::setOperator(PredicateInfo::PredOperator op)
{
  operator_ = op;
}

void PredicateInfo::setEvaluationCode(PredicateInfo::EvaluationCode c)
{
  evalCode_ = c;
}

int PredicateInfo::serializedLength()
{
  return TMUDRSerializableObject::serializedLength() +
    2 * serializedLengthOfInt();
}

int PredicateInfo::serialize(Bytes &outputBuffer,
                             int &outputBufferLength)
{
  int result = 
    TMUDRSerializableObject::serialize(outputBuffer,
                                       outputBufferLength);

  result += serializeInt(evalCode_,
                         outputBuffer,
                         outputBufferLength);
  result += serializeInt(static_cast<int>(operator_),
                         outputBuffer,
                         outputBufferLength);
  // validate length in derived classes
  return result;
}

int PredicateInfo::deserialize(ConstBytes &inputBuffer,
                               int &inputBufferLength)
{
  int result =
    TMUDRSerializableObject::deserialize(inputBuffer, inputBufferLength);

  int op = 0;

  result += deserializeInt(evalCode_,
                           inputBuffer,
                           inputBufferLength);
  
  result += deserializeInt(op,
                           inputBuffer,
                           inputBufferLength);
  operator_ = static_cast<PredOperator>(op);
  
  // validate operator type and length in derived classes
  return result;
}

// ------------------------------------------------------------------------
// Member functions for class ComparisonPredicateInfo
// ------------------------------------------------------------------------

ComparisonPredicateInfo::ComparisonPredicateInfo() :
     PredicateInfo(COMP_PREDICATE_INFO_OBJ),
     columnNumber_(-1)
{}

/**
 *  Get the column number of the column in this comparison predicate.
 *
 *  @return Column number.
 */
int ComparisonPredicateInfo::getColumnNumber() const
{
  return columnNumber_;
}

/**
 *  Return whether this comparison value involves a constant.
 *
 *  The method returns whether the comparison predicate is of the form
 *  "column" "op" "constant". If it returns false, the predicate
 *  compares the column with a parameter or some other value not
 *  available to the UDR. Predicates that do not involve a constant
 *  cannot be evaluated in the UDR itself, since the comparison value
 *  is not available to the UDR. They can be evaluated on a table-valued
 *  input, however.
 *
 *  @return true if the comparison is with a constant, false otherwise
 */
bool ComparisonPredicateInfo::hasAConstantValue() const
{
  return (value_.size() > 0);
}

/**
 *  Return the value, as a string, of the constant in this predicate.
 *
 *  This returns the value, using SQL syntax, of the constant involved
 *  in the comparison predicate. It throws an exception if method
 *  hasAConstantValue() would return false.
 *
 *  @see hasAConstantValue()
 *
 *  @return Value of the constant in this comparison predicate.
 *  @throws UDRException
 */
std::string ComparisonPredicateInfo::getConstValue() const
{
  return value_;
}

void ComparisonPredicateInfo::setColumnNumber(int columnNumber)
{
  columnNumber_ = columnNumber;
}

void ComparisonPredicateInfo::setValue(const char *value)
{
  value_.assign(value);
}

void ComparisonPredicateInfo::mapColumnNumbers(const std::vector<int> &map)
{
  if (map[columnNumber_] < 0)
    throw UDRException(
         38900,
         "Invalid column mapping for column %d in a predicate",
         columnNumber_);
  columnNumber_ = map[columnNumber_];
}

void ComparisonPredicateInfo::toString(std::string &s,
                                       const TableInfo &ti) const
{
  s += ti.getColumn(columnNumber_).getColName();

  switch (getOperator())
    {
    case UNKNOWN_OP:
      s += " unknown operator ";
      break;
    case EQUAL:
      s += " = ";
      break;
    case NOT_EQUAL:
      s += " <> ";
      break;
    case LESS:
      s += " < ";
      break;
    case LESS_EQUAL:
      s += " <= ";
      break;
    case GREATER:
      s += " > ";
      break;
    case GREATER_EQUAL:
      s += " >= ";
      break;
    case IN:
      s += " in ";
      break;
    case NOT_IN:
      s += " not in ";
      break;
    default:
      s += " invalid operator ";
      break;
    }

  s += value_;
}

int ComparisonPredicateInfo::serializedLength()
{
  return PredicateInfo::serializedLength() +
    serializedLengthOfInt() +
    serializedLengthOfString(value_);
}

int ComparisonPredicateInfo::serialize(Bytes &outputBuffer,
                                       int &outputBufferLength)
{
  int result = 
    PredicateInfo::serialize(outputBuffer,
                             outputBufferLength);

  result += serializeInt(columnNumber_,
                         outputBuffer,
                         outputBufferLength);
  result += serializeString(value_,
                            outputBuffer,
                            outputBufferLength);

  validateSerializedLength(result);

  return result;
}

int ComparisonPredicateInfo::deserialize(ConstBytes &inputBuffer,
                                         int &inputBufferLength)
{
  int result =
    PredicateInfo::deserialize(inputBuffer, inputBufferLength);

  result += deserializeInt(columnNumber_,
                           inputBuffer,
                           inputBufferLength);
  result += deserializeString(value_,
                              inputBuffer,
                              inputBufferLength);

  validateObjectType(COMP_PREDICATE_INFO_OBJ);
  validateDeserializedLength(result);

  return result;
}

// ------------------------------------------------------------------------
// Member functions for class PartitionInfo
// ------------------------------------------------------------------------

/**
 *  Default constructor
 *
 *  Use this constructor to generate an object to be passed
 *  to UDRInvocationInfo::setChildPartitioning().
 */
PartitionInfo::PartitionInfo() : type_(UNKNOWN)
{}

/**
 *  Get the partitioning type.
 *
 *  @return Partition type enum.
 */
PartitionInfo::PartitionTypeCode PartitionInfo::getType() const
{
  return type_;
}

/**
 *  Get the number of columns that form the partitioning key
 *
 *  Returns the number of columns in the list of partitioning keys
 *  or zero if there are no such columns.
 *
 *  @return Number of partitioning key columns (could be zero)
 */
int PartitionInfo::getNumEntries() const
{
  return partCols_.size();
}

/**
 *  Get the number/ordinal of the ith partitioning column.
 *
 *  @return Number/ordinal (0-based) of the ith partitioning column in
 *          the list of partitioning columns.
 *  @throws UDRException
 */
int PartitionInfo::getColumnNum(int i) const
{
  if (i < 0 || i >= partCols_.size())
    throw UDRException(
         38900,
         "Trying to access column %d of a PartitionInfo with %d partitioning columns",
         i, partCols_.size());
  return partCols_[i];
}

/**
 *  Set the partitioning type.
 *
 *  @param type Partition type enum.
 */
void PartitionInfo::setType(PartitionTypeCode type)
{
  type_ = type;
}

/**
 *  Add a new column to the list of partitioning columns
 *
 *  Add a new column to the list of column numbers that form the
 *  partitioning key. Use this only if the type of the partitioning
 *  is set to PARTITION.
 *
 *  @param colNum Number of the column (ordinal, 0-based) of the
 *                associated table.
 *  @throws UDRException
 */
void PartitionInfo::addEntry(int colNum)
{
  // don't allow duplicates
  for (std::vector<int>::iterator it = partCols_.begin();
       it != partCols_.end();
       it++)
    if (*it == colNum)
      throw UDRException(
           38900,
           "Trying to add column number %d more than once to a PartitionInfo object",
           colNum);

  partCols_.push_back(colNum);
}

/**
 *  Clear the contents of the object
 */
void PartitionInfo::clear()
{
  type_ = UNKNOWN;
  partCols_.clear();
}

void PartitionInfo::mapColumnNumbers(const std::vector<int> &map)
{
  for (int i=0; i<partCols_.size(); i++)
    {
      int colNum = partCols_[i];

      if (map[colNum] < 0)
        throw UDRException(
             38900,
             "Invalid mapping for PARTITION BY column %d",
             colNum);
      partCols_[i] = map[colNum];
    }
}


// ------------------------------------------------------------------------
// Member functions for class OrderInfo
// ------------------------------------------------------------------------

/**
 *  Get the number of entries (columns) in the ordering.
 *
 *  @return Number of entries/columns that make up the ordering.
 */
int OrderInfo::getNumEntries() const
{
  return columnNumbers_.size();
}

/**
 *  Get the column number of an entry of the ordering.
 *
 *  @param i the position (0-based) of the ordering, 0 meaning the leading position.
 *  @return The column number of the n-th entry of the ordering (both are 0-based).
 *  @throws UDRException
 */
int OrderInfo::getColumnNum(int i) const
{
  if (i < 0 || i >= columnNumbers_.size())
    throw UDRException(
         38900,
         "Trying to access colnum entry %d of an OrderInfo object with %d entries",
         i, columnNumbers_.size());

  return columnNumbers_[i];
}

/**
 *  Get the order type of an entry of the ordering.
 *
 *  @param i the position (0-based) of the ordering, 0 meaning the leading position.
 *  @return The order type of the n-th entry of the ordering (0-based).
 *  @throws UDRException
 */
OrderInfo::OrderTypeCode OrderInfo::getOrderType(int i) const
{
  if (i < 0 || i >= orderTypes_.size())
    throw UDRException(
         38900,
         "Trying to access order type entry %d of an OrderInfo object with %d entries",
         i, orderTypes_.size());

  return orderTypes_[i];
}

/**
 *  Append an entry to the ordering.
 *
 *  @param colNum Column number to append to the ordering.
 *  @param orderType Order type (ascending or descending) to use.
 */
void OrderInfo::addEntry(int colNum, OrderTypeCode orderType)
{
  columnNumbers_.push_back(colNum);
  orderTypes_.push_back(orderType);
}

/**
 *  Insert an entry at any position of the ordering.
 *
 *  A quick example to illustrate this: Let's say we have a table
 *  with columns (a,b,c). Their column numbers are 0, 1, and 2.
 *  We produce an ordering (C ASCENDING):
 *
 *  @code OrderInfo myorder;
 *  
 *  myorder.addEntryAt(0, 2); @endcode
 *
 *  Next, we want to make this into (B DESCENDING, C ASCENDING):
 *
 *  @code myorder.addEntryAt(0, 1, DESCENDING); @endcode
 *
 *  @param pos Position (0-based) at which we want to insert. The new
 *             entry will be position "pos" after the insertion, any
 *             existing entries will be moved up.
 *  @param colNum Number of the column by which we want to order
 *  @param orderType Order type (ascending or descending) to use
 *  @throws UDRException
 */
void OrderInfo::addEntryAt(int pos,
                           int colNum,
                           OrderTypeCode orderType)
{
  if (pos > columnNumbers_.size())
    throw UDRException(
         38900,
         "OrderInfo::addEntryAt at position %d with a list of %d entries",
         pos, columnNumbers_.size());
  columnNumbers_.insert(columnNumbers_.begin() + pos, colNum);
  orderTypes_.insert(orderTypes_.begin() + pos, orderType);
}

/**
 *  Clear the contents of the object
 */
void OrderInfo::clear()
{
  columnNumbers_.clear();
  orderTypes_.clear();
}

void OrderInfo::mapColumnNumbers(const std::vector<int> &map)
{
  for (int i=0; i<columnNumbers_.size(); i++)
    {
      int colNum = columnNumbers_[i];

      if (map[colNum] < 0)
        throw UDRException(
             38900,
             "Invalid mapping for ORDER BY column %d",
             colNum);
      columnNumbers_[i] = map[colNum];
    }
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

/**
 *  Get the number of columns or parameters.
 *
 *  @return Number of columns/parameters.
 */
int TupleInfo::getNumColumns() const
{
  return columns_.size();
}

/**
 *  Look up a column/parameter number by name.
 *
 *  @param colName Name of an existing column.
 *  @return Column/parameter number.
 *  @throws UDRException
 */
int TupleInfo::getColNum(const char *colName) const
{
  int result = 0;
  std::vector<ColumnInfo *>::const_iterator it = columns_.begin();

  for (; it != columns_.end(); it++, result++)
    if ((*it)->getColName() == colName)
      return result;

  throw UDRException(38900, "Column %s not found", colName);
}

/**
 *  Look up a column/parameter number by name.
 *
 *  @param colName Name of an existing column.
 *  @return Column/parameter number.
 *  @throws UDRException
 */
int TupleInfo::getColNum(const std::string &colName) const
{
  return getColNum(colName.c_str());
}

/**
 *  Get the column info for a column identified by its ordinal.
 *
 *  @param colNum Column number.
 *  @return Column info.
 *  @throws UDRException
 */
const ColumnInfo &TupleInfo::getColumn(int colNum) const
{
  if (colNum < 0 || colNum >= columns_.size())
    throw UDRException(
         38900,
         "Trying to access column number %d but column list has only %d elements",
         colNum, columns_.size());

  return *(columns_[colNum]);
}

/**
 *  Get the column info for a column identified by its name.
 *
 *  @param colName Name of an existing column.
 *  @return Column info.
 *  @throws UDRException
 */
const ColumnInfo &TupleInfo::getColumn(const std::string &colName) const
{
  return getColumn(getColNum(colName));
}

/**
 *  Get the non-const column info for a column identified by its ordinal.
 *
 *  @param colNum Column number.
 *  @return Column info.
 *  @throws UDRException
 */
ColumnInfo &TupleInfo::getColumn(int colNum)
{
  if (colNum < 0 || colNum >= columns_.size())
    throw UDRException(
         38900,
         "Trying to access column number %d but column list has only %d elements",
         colNum, columns_.size());

  return *(columns_[colNum]);
}

/**
 *  Get the non-const column info for a column identified by its name.
 *
 *  @param colName Name of an existing column.
 *  @return Column info.
 *  @throws UDRException
 */
ColumnInfo &TupleInfo::getColumn(const std::string &colName)
{
  return getColumn(getColNum(colName));
}

/**
 *  Get the type of a column.
 *
 *  @param colNum Column number.
 *  @return Type of the column.
 *  @throws UDRException
 */
const TypeInfo &TupleInfo::getType(int colNum) const
{
  return getColumn(colNum).getType();
}

/**
 *  Get the SQL type class.
 *
 *  Determine whether this is a numeric character, datetime or interval type.
 *  @param colNum Column number.
 *  @return SQL type class enum.
 *  @throws UDRException
 */
TypeInfo::SQLTypeClassCode TupleInfo::getSQLTypeClass(int colNum) const
{
  return getType(colNum).getSQLTypeClass();
}

/**
 *  Add a new column.
 *
 *  Only use this method from within the following method:
 *  @arg UDR::describeParamsAndColumns()
 *
 *  @param column Info of the new column to add.
 *  @throws UDRException
 */
void TupleInfo::addColumn(const ColumnInfo &column)
{
  ColumnInfo *newCol = new ColumnInfo(column);

  columns_.push_back(newCol);
}

/**
 *  Get an integer value of a column or parameter
 *
 *  This method is modeled after the JDBC interface.
 *
 *  Use this method at runtime. It can also be used for
 *  actual parameters that are available at compile time.
 *
 *  @param colNum Column number.
 *  @return Integer value.
 *          If the value was a NULL value, then 0 is returned.
 *          The wasNull() method can be used to determine whether
 *          a NULL value was returned.
 *  @throws UDRException
 */
int TupleInfo::getInt(int colNum) const
{
  bool &nonConstWasNull = const_cast<TupleInfo *>(this)->wasNull_;

  nonConstWasNull = false;

  return getType(colNum).getInt(rowPtr_, nonConstWasNull);
}

/**
 *  Get an integer value for a column identified by name.
 *
 *  @see TupleInfo::getInt(int) const
 *
 *  @param colName Name of an existing column.
 *  @return Integer value.
 *          If the value was a NULL value, then 0 is returned.
 *          The wasNull() method can be used to determine whether
 *          a NULL value was returned.
 *  @throws UDRException
 */
int TupleInfo::getInt(const std::string &colName) const
{
  return getInt(getColNum(colName));
}

/**
 *  Get a long value of a column or parameter
 *
 *  This method is modeled after the JDBC interface.
 *
 *  Use this method at runtime. It can also be used for
 *  actual parameters that are available at compile time.
 *
 *  @param colNum Column number.
 *  @return long value.
 *          If the value was a NULL value, then 0 is returned.
 *          The wasNull() method can be used to determine whether
 *          a NULL value was returned.
 *  @throws UDRException
 */
long TupleInfo::getLong(int colNum) const
{
  bool &nonConstWasNull = const_cast<TupleInfo *>(this)->wasNull_;

  nonConstWasNull = false;

  return getType(colNum).getLong(rowPtr_, nonConstWasNull);
}

/**
 *  Get a long value for a column identified by name.
 *
 *  @see TupleInfo::getLong(int) const
 *
 *  @param colName Name of an existing column.
 *  @return long value.
 *  @throws UDRException
 */
long TupleInfo::getLong(const std::string &colName) const
{
  return getLong(getColNum(colName));
}

/**
 *  Get a double value of a column or parameter
 *
 *  This method is modeled after the JDBC interface.
 *
 *  Use this method at runtime. It can also be used for
 *  actual parameters that are available at compile time.
 *
 *  @param colNum Column number.
 *  @return double value.
 *  @throws UDRException
 */
double TupleInfo::getDouble(int colNum) const
{
  bool &nonConstWasNull = const_cast<TupleInfo *>(this)->wasNull_;

  nonConstWasNull = false;

  return getType(colNum).getDouble(rowPtr_, nonConstWasNull);
}

/**
 *  Get double value of a column/parameter identified by name.
 *
 *  @see TupleInfo::getDouble(int colNum) const
 *
 *  @param colName Name of an existing column.
 *  @return double value.
 *  @throws UDRException
 */
double TupleInfo::getDouble(const std::string &colName) const
{
  return getDouble(getColNum(colName));
}

/**
 *  Get a pointer to the raw data value of a column.
 *
 *  Using this method requires knowledge of the data layout
 *  for the different types used in UDRs. This method can be
 *  useful for performance optimizations, when converting longer
 *  string values to std::string is undesirable. Note that the
 *  pointer to the raw value is valid only until a new row
 *  is read or the existing row is emitted.
 *
 *  Use this method at runtime. It can also be used for
 *  actual parameters that are available at compile time.
 *
 *  @param colNum Column number.
 *  @param byteLen Length, in bytes, of the value returned.
 *  @return Pointer to the raw column value in the row buffer.
 *  @throws UDRException
 */
const char * TupleInfo::getRaw(int colNum, int &byteLen) const
{
  bool &nonConstWasNull = const_cast<TupleInfo *>(this)->wasNull_;

  nonConstWasNull = false;

  return getType(colNum).getRaw(rowPtr_,
                                nonConstWasNull,
                                byteLen);
}

/**
 *  Get a datetime or interval column value as time_t
 *
 *  This method can be used to convert column values with
 *  a datetime type or a day-second interval type to the
 *  POSIX type time_t. Note that this may result in the loss
 *  of fractional seconds.
 *
 *  Use this method at runtime. It can also be used for
 *  actual parameters that are available at compile time.
 *
 *  @param colNum Column number.
 *  @throws UDRException
 */
time_t TupleInfo::getTime(int colNum) const
{
  bool &nonConstWasNull = const_cast<TupleInfo *>(this)->wasNull_;

  nonConstWasNull = false;

  return getType(colNum).getTime(rowPtr_,
                                 nonConstWasNull);
}

/**
 *  Check whether a parameter is available at compile-time.
 *
 *  Use this method to check in the compiler interfaces whether
 *  an actual parameter is a constant value that can be read
 *  at compile time. If this method returns true, the value
 *  can be accessed with the getInt(), getString() etc. methods.
 *
 *  @param colNum Column number.
 *  @return true if the parameter value is available.
 *  @throws UDRException
 */
bool TupleInfo::isAvailable(int colNum) const
{
  return (rowPtr_ != NULL &&
          colNum < columns_.size() &&
          getType(colNum).isAvailable());
}

/**
 *  Get columns of a row as a delimited string.
 *
 *  This method is useful to interface with tools that take a delimited
 *  record format. It is also useful for printing rows
 *  (see UDRInvocationInfo::TRACE_ROWS).
 *
 *  Only use this method at runtime.
 *
 *  Note: This method may return a string that contains multiple
 *        character sets, if columns with different character sets
 *        are involved. Using this method with UCS-2 columns is not
 *        recommended.
 *
 *  @param row         String reference in which the result delimited row
 *                     will be returned.
 *  @param delim       US ASCII field delimiter to use.
 *  @param quote       Whether to quote character field values that contain
 *                     the delimiter symbol or a quote symbol. Quote symbols
 *                     will be duplicated to escape them.
 *  @param quoteSymbol US ASCII quote character to use, if quote is true.
 *  @param firstColumn First column to read.
 *  @param lastColumn  Last column to read (inclusive) or -1 to read until
 *                     the last column in the row.
 *  @throws UDRException
 */
void TupleInfo::getDelimitedRow(std::string &row,
                                char delim,
                                bool quote,
                                char quoteSymbol,
                                int firstColumn,
                                int lastColumn) const
{
  // read all columns and form a delimited text row from them

  // if quote is true, then quote any text that contains the delimiter
  // and also double any quotes appearing in the text 
  int nc = getNumColumns();

  if (firstColumn >= nc ||
      firstColumn < 0 ||
      lastColumn < -1 ||
      lastColumn > 0 && lastColumn >= nc)
    throw UDRException(
         38900,
         "Invalid column range %d to %d in getDelimitedRow for a tuple with %d columns",
         firstColumn,
         lastColumn,
         nc);
  if (lastColumn == -1)
    lastColumn = nc-1;

  row.erase();

  for (int i=firstColumn; i<=lastColumn; i++)
    {
      std::string val=getString(i);

      if (i>firstColumn)
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

/**
 *  Get a string value of a column or parameter
 *
 *  This method is modeled after the JDBC interface.
 *
 *  Use this method at runtime. It can also be used for
 *  actual parameters that are available at compile time.
 *
 *  @param colNum Column number.
 *  @return String value.
 *          If the value was a NULL value, an empty string
 *          is returned. The wasNull() method can be used to
 *          determine whether a NULL value was returned.
 *  @throws UDRException
 */
std::string TupleInfo::getString(int colNum) const
{
  int stringLen = 0;
  TypeInfo::SQLTypeCode sqlType = getType(colNum).getSQLType();

  switch (sqlType)
    {
    case TypeInfo::DECIMAL_LSE:
    case TypeInfo::DECIMAL_UNSIGNED:
    case TypeInfo::CHAR:
    case TypeInfo::VARCHAR:
    case TypeInfo::DATE:
    case TypeInfo::TIME:
    case TypeInfo::TIMESTAMP:
    case TypeInfo::BLOB:
    case TypeInfo::CLOB:
      {
        // these types are stored as strings
        const char *buf = getRaw(colNum, stringLen);
        if (buf)
          return std::string(buf, stringLen);
        else
          return std::string("");
      }


    case TypeInfo::TINYINT:
    case TypeInfo::SMALLINT:
    case TypeInfo::INT:
    case TypeInfo::LARGEINT:
    case TypeInfo::NUMERIC:
    case TypeInfo::TINYINT_UNSIGNED:
    case TypeInfo::SMALLINT_UNSIGNED:
    case TypeInfo::INT_UNSIGNED:
    case TypeInfo::NUMERIC_UNSIGNED:
      {
        char buf[32];
        long num = getLong(colNum);

        if (wasNull_)
          return "";

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

        if (wasNull_)
          return "";

        snprintf(buf, sizeof(buf), "%*lf", numSignificantDigits, num);

        return buf;
      }
    case TypeInfo::INTERVAL:
      {
        char buf[32];
        long longVal       = getLong(colNum);
        long fractionalVal = 0;
        const TypeInfo typ = getType(colNum);
        TypeInfo::SQLIntervalCode intervalCode = typ.getIntervalCode();
        int precision      = typ.getPrecision();
        int scale          = typ.getScale();
        const char *sign   = "";
        const char *dot    = (scale == 0 ? "" : ".");

        if (wasNull_)
          return "";

        if (longVal < 0)
          {
            longVal = -longVal;
            sign = "-";
          }

        // split the number into integer and fractional values
        for (int d=0; d<scale; d++)
          {
            fractionalVal = 10*fractionalVal + longVal % 10;
            longVal /= 10;
          }

        switch (intervalCode)
          {
          case TypeInfo::INTERVAL_YEAR:
          case TypeInfo::INTERVAL_MONTH:
          case TypeInfo::INTERVAL_DAY:
          case TypeInfo::INTERVAL_HOUR:
          case TypeInfo::INTERVAL_MINUTE:
            // Example: "59"
            snprintf(buf, sizeof(buf), "%s%*ld", sign, precision, longVal);
            break;
          case TypeInfo::INTERVAL_SECOND:
            // Example: "99999.000001"
            snprintf(buf, sizeof(buf), "%s%*ld%s%0*ld", sign, precision, longVal,
                                                                         dot, scale, fractionalVal);
            break;
          case TypeInfo::INTERVAL_YEAR_MONTH:
            // Example: "100-01"
            snprintf(buf, sizeof(buf), "%s%*ld-%02d", sign, precision, (long) (longVal/12),
                                                                       (int)  (longVal%12));
            break;
          case TypeInfo::INTERVAL_DAY_HOUR:
            // Example: "365 06"
            snprintf(buf, sizeof(buf), "%s%*ld %02d", sign, precision, (long) (longVal/24),
                                                                       (int)  (longVal%24));
            break;
          case TypeInfo::INTERVAL_DAY_MINUTE:
            // Example: "365:05:49"
            snprintf(buf, sizeof(buf), "%s%*ld %02d:%02d", sign, precision, (long) (longVal/1440),
                                                                            (int)  (longVal%1440/60),
                                                                            (int)  (longVal%60));
            break;
          case TypeInfo::INTERVAL_DAY_SECOND:
            // Example: "365:05:49:12.00"
            snprintf(buf, sizeof(buf), "%s%*ld %02d:%02d:%02d%s%0*ld", sign, precision,
                     (long) (longVal/86400),
                     (int)  (longVal%86400/3600),
                     (int)  (longVal%3600/60),
                     (int)  (longVal%60),
                     dot, scale, fractionalVal);
            break;
          case TypeInfo::INTERVAL_HOUR_MINUTE:
            // Example: "12:00"
            snprintf(buf, sizeof(buf), "%s%*ld:%02d", sign, precision, (long) (longVal/60),
                                                                       (int)  (longVal%60));
            break;
          case TypeInfo::INTERVAL_HOUR_SECOND:
            // Example: "100:00:00"
            snprintf(buf, sizeof(buf), "%s%*ld:%02d:%02d%s%0*ld", sign, precision,
                     (long) (longVal/3600),
                     (int)  (longVal%3600/60),
                     (int)  (longVal%60),
                     dot, scale, fractionalVal);
            break;
          case TypeInfo::INTERVAL_MINUTE_SECOND:
            // Example: "3600:00.000000"
            snprintf(buf, sizeof(buf), "%s%*ld:%02d%s%0*ld", sign, precision,
                     (long) (longVal/60),
                     (int)  (longVal%60),
                     dot, scale, fractionalVal);
            break;
          default:
            throw UDRException(
                 38900,
                 "Invalid interval code in TypeInfo::getString()");
          }

        return buf;
      }
    case TypeInfo::BOOLEAN:
      {
        if (getBoolean(colNum))
          return "1";
        else
          return "0";
      }

    default:
      throw UDRException(
           38900,
           "Type %d not yet supported in getString()",
           sqlType);
    }

}

/**
 *  Get a string value of a column or parameter identified by name.
 *
 *  This method is modeled after the JDBC interface.
 *
 *  Use this method at runtime. It cannot be used for
 *  actual parameters that are available at compile time, use
 *  getString(int colNum) instead, since actual parameters are not named.
 *
 *  @param colName Name of an existing column.
 *  @return String value.
 *          If the value was a NULL value, an empty string
 *          is returned. The wasNull() method can be used to
 *          determine whether a NULL value was returned.
 *  @throws UDRException
 */
std::string TupleInfo::getString(const std::string &colName) const
{
  return getString(getColNum(colName));
}

/**
 *  Get a boolean value of a column or parameter
 *
 *  This method is modeled after the JDBC interface.
 *  It can be used on boolean, numeric and character columns.
 *  Numeric columns need to have a value of 0 (false) or 1 (true),
 *  character columns need to have a value of "0" (false) or "1" (true).
 *
 *  Use this method at runtime. It can also be used for
 *  actual parameters that are available at compile time.
 *
 *  @param colNum Column number.
 *  @return Boolean value.
 *          If the value was a NULL value, false
 *          is returned. The wasNull() method can be used to
 *          determine whether a NULL value was returned.
 *  @throws UDRException
 */
bool TupleInfo::getBoolean(int colNum) const
{
  bool &nonConstWasNull = const_cast<TupleInfo *>(this)->wasNull_;

  nonConstWasNull = false;

  return getType(colNum).getBoolean(rowPtr_, nonConstWasNull);
}

/**
 *  Get a boolean value of a column or parameter identified by name.
 *
 *  This method is modeled after the JDBC interface.
 *
 *  Use this method at runtime. It cannot be used for
 *  actual parameters that are available at compile time, use
 *  getString(int colNum) instead, since actual parameters are not named.
 *
 *  @param colName Name of an existing column.
 *  @return bool value.
 *          If the value was a NULL value, false
 *          is returned. The wasNull() method can be used to
 *          determine whether a NULL value was returned.
 *  @throws UDRException
 */
bool TupleInfo::getBoolean(const std::string &colName) const
{
  return getBoolean(getColNum(colName));
}

/**
 *  Check whether the last value returned from a getInt() etc. method was NULL.
 *
 *  This method is modeled after the JDBC interface.
 *
 *  @return true if the last value returned from a getInt(), getString()
 *               etc. method was a NULL value, false otherwise.
 */
bool TupleInfo::wasNull() const
{
  return wasNull_;
}

/**
 *  Set an output column to a specified integer value.
 *
 *  Use this method at runtime.
 *
 *  @param colNum Index/ordinal of the column to set.
 *  @param val    The new integer value for the column to set.
 *  @throws UDRException
 */
void TupleInfo::setInt(int colNum, int val) const
{
  getType(colNum).setInt(val, rowPtr_);
}

/**
 *  Set an output column to a specified long value.
 *
 *  Use this method at runtime.
 *
 *  @param colNum Index/ordinal of the column to set.
 *  @param val    The new long value for the column to set.
 *  @throws UDRException
 */
void TupleInfo::setLong(int colNum, long val) const
{
  getType(colNum).setLong(val, rowPtr_);
}

/**
 *  Set an output column to a specified double value.
 *
 *  Use this method at runtime.
 *
 *  @param colNum Index/ordinal of the column to set.
 *  @param val    The new double value for the column to set.
 *  @throws UDRException
 */
void TupleInfo::setDouble(int colNum, double val) const
{
  getType(colNum).setDouble(val, rowPtr_);
}

/**
 *  Set an output column to a specified string value.
 *
 *  Use this method at runtime. The length of the string is determined
 *  by calling strlen().
 *
 *  @param colNum Index/ordinal of the column to set.
 *  @param val    The new string value for the column to set.
 *                The length of the string is determined by calling strlen.
 *  @throws UDRException
 */
void TupleInfo::setString(int colNum, const char *val) const
{
  setString(colNum, val, strlen(val));
}

/**
 *  Set an output column to a specified string value.
 *
 *  Use this method at runtime.
 *
 *  @param colNum    Index/ordinal of the column to set.
 *  @param val       The new string value for the column to set.
 *  @param stringLen Length (in bytes) of the string value provided.
 *                   The string may contain embedded NUL bytes.
 *  @throws UDRException
 */
void TupleInfo::setString(int colNum, const char *val, int stringLen) const
{
  getType(colNum).setString(val, stringLen, rowPtr_);
}

/**
 *  Set an output column to a specified string value.
 *
 *  Use this method at runtime.
 *
 *  @param colNum    Index/ordinal of the column to set.
 *  @param val       The new string value for the column to set.
 *  @throws UDRException
 */
void TupleInfo::setString(int colNum, const std::string &val) const
{
  setString(colNum, val.data(), val.size());
}

/**
 *  Set a datetime or interval output column to a value specified as time_t
 *
 *  This method cannot be used with year-month intervals or data types that
 *  are not datetime or interval types. It is not possible to set fractional
 *  seconds with this method.
 *
 *  Use this method at runtime.
 *
 *  @param colNum    Index/ordinal of the column to set.
 *  @param val       The new time_t value for the column to set.
 *  @throws UDRException
 */
void TupleInfo::setTime(int colNum, time_t val) const
{
  getType(colNum).setTime(val, rowPtr_);
}

/**
 *  Set a column to a value specified as bool
 *
 *  This will set boolean, numeric and character columns.
 *  Numeric values will be 0 for false, 1 for true.
 *  String values will be "0" for false, "1" for true.
 *
 *  Use this method at runtime.
 *
 *  @param colNum    Index/ordinal of the column to set.
 *  @param val       The new boolean value for the column to set.
 *  @throws UDRException
 */
void TupleInfo::setBoolean(int colNum, bool val) const
{
  getType(colNum).setBoolean(val, rowPtr_);
}

/**
 *  Set the result row from a string with delimited field values.
 *
 *  This method can be used to read delimited text files and
 *  conveniently produce a result table from them. For example,
 *  if the following string is passed in as row:
 *  @code skip1|'skip|2'|3|'delim|and''Quote'|5 @endcode
 *  This call:
 *  @code setFromDelimitedRow(
 *      row,  // row
 *      '|',  // delim
 *      true, // quote
 *      '\'', // quoteSymbol (single quote)
 *      10,   // firstColumnToSet
 *      11,   // lastColumnToSet
 *      2);   // numDelimColsToSkip @endcode
 *  would set output column 10 to 3 and output column 11 to delim|and'Quote.
 *
 *  Note: The delimited row may need to contain strings of multiple
 *        character sets. Using this method with UCS2 columns is not
 *        recommended, since that might require special handling.
 *
 *  @see getDelimitedRow()
 *
 *  @param row    A string with delimited field values to read.
 *  @param delim  Delimiter between field values. Use a US ASCII symbol
 *                as the delimiter.
 *  @param quote  true if the method should assume that text fields
 *                use quotes to quote special symbols like delimiters
 *                that are embedded within fields, and that quote symbols
 *                embedded in text fields will be doubled.
 *  @param quoteSymbol US ASCII Quote symbol used to quote text. Meaningful
 *                     only if quote is set to true.
 *  @param firstColumnToSet First column in the output table to be set
 *                          from the delimited row (0-based).
 *  @param lastColumnToSet  Last column in the output table to be set
 *                          (inclusive) or -1 to indicate to set all
 *                          remaining columns of the table.
 *  @param numDelimColsToSkip Number of fields to skip in the delimited
 *                            row before using the values to set output
 *                            columns.
 *  @return                  Pointer to the first character after the
 *                           text that has been consumed by this method.
 *  @throws UDRException
 */
const char * TupleInfo::setFromDelimitedRow(const char *row,
                                            char delim,
                                            bool quote,
                                            char quoteSymbol,
                                            int firstColumnToSet,
                                            int lastColumnToSet,
                                            int numDelimColsToSkip) const
{
  int nc = getNumColumns();
  const char *c = row;
  // virtual start column number of the first column in the delimited row
  // we may need to skip some values to reach the first one to use
  int startCol = firstColumnToSet-numDelimColsToSkip;

  if (firstColumnToSet >= nc ||
      firstColumnToSet < 0 ||
      lastColumnToSet < -1 ||
      lastColumnToSet > 0 && (lastColumnToSet >= nc ||
                              firstColumnToSet > lastColumnToSet))
    throw UDRException(
         38900,
         "Invalid column range %d to %d in setFromDelimitedRow for a tuple with %d columns",
         firstColumnToSet,
         lastColumnToSet,
         nc);
  if (lastColumnToSet == -1)
    lastColumnToSet = nc-1;

  for (int i=startCol; i<=lastColumnToSet; i++)
    {
      // skip over whitespace
      while (*c == ' ' || *c == '\t') c++;

      // make sure we have a delimiter for columns other than the first
      if (i>startCol)
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
              if (i >= firstColumnToSet)
                // set from the transformed string
                setString(i, unquotedVal);
            }
          else
            {
              if (i >= firstColumnToSet)
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

          if (i >= firstColumnToSet)
            {
              if (isNull)
                setNull(i);
              else
                setString(i, c, (endOfVal-c));
            }
        }

      // set the current character pointer to the
      // character just past of what we have consumed
      c = endOfVal;
    }

  return c;
}

/**
 *  Set an output column to a NULL value.
 *
 *  Use this method at runtime.
 *
 *  @param colNum Index/ordinal of the column to set to NULL.
 *  @throws UDRException
 */
void TupleInfo::setNull(int colNum) const
{
  getType(colNum).setNull(rowPtr_);
}

/**
 *  Add an integer output column.
 *
 *  The new column is added at the end.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @param colName    Name of the column to add. Use UPPER CASE letters,
 *                    digits and underscore, otherwise you will need to
 *                    use delimited column names with matching case in
 *                    Trafodion.
 *  @param isNullable true if the added column should be nullable,
 *                    false if the added column should have the NOT NULL
 *                    constraint.
 *  @throws UDRException
 */
void TupleInfo::addIntColumn(const char *colName, bool isNullable)
{
  addColumn(ColumnInfo(colName, TypeInfo(TypeInfo::INT,0,isNullable)));
}

/**
 *  Add a long output column.
 *
 *  The new column is added at the end.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @param colName    Name of the column to add. Use UPPER CASE letters,
 *                    digits and underscore, otherwise you will need to
 *                    use delimited column names with matching case in
 *                    Trafodion.
 *  @param isNullable true if the added column should be nullable,
 *                    false if the added column should have the NOT NULL
 *                    constraint.
 *  @throws UDRException
 */
void TupleInfo::addLongColumn(const char *colName, bool isNullable)
{
  addColumn(ColumnInfo(colName, TypeInfo(TypeInfo::LARGEINT,0,isNullable)));
}

/**
 *  Add a double output column.
 *
 *  The new column is added at the end.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @param colName    Name of the column to add. Use UPPER CASE letters,
 *                    digits and underscore, otherwise you will need to
 *                    use delimited column names with matching case in
 *                    Trafodion.
 *  @param isNullable true if the added column should be nullable,
 *                    false if the added column should have the NOT NULL
 *                    constraint.
 *  @throws UDRException
 */
void TupleInfo::addDoubleColumn(const char *colName, bool isNullable)
{
  addColumn(ColumnInfo(colName, TypeInfo(TypeInfo::DOUBLE_PRECISION,0,isNullable)));
}

/**
 *  Add a fixed character output column.
 *
 *  The new column is added at the end.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @param colName    Name of the column to add. Use UPPER CASE letters,
 *                    digits and underscore, otherwise you will need to
 *                    use delimited column names with matching case in
 *                    Trafodion.
 *  @param length     Length of the new character column.
 *                    For single-byte and variable byte character sets,
 *                    the length is specified in bytes. For UTF-8, this
 *                    is equivalent to CHAR(length BYTES) in SQL. For UCS2,
 *                    the length is in UCS2 16-bit characters.
 *  @param isNullable true if the added column should be nullable,
 *                    false if the added column should have the NOT NULL
 *                    constraint.
 *  @param charset    Character set of the new column.
 *  @param collation  Collation of the new column.
 *  @throws UDRException
 */
void TupleInfo::addCharColumn(const char *colName,
                              int length,
                              bool isNullable,
                              TypeInfo::SQLCharsetCode charset,
                              TypeInfo::SQLCollationCode collation)
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

/**
 *  Add a VARCHAR output column.
 *
 *  The new column is added at the end.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @param colName    Name of the column to add. Use UPPER CASE letters,
 *                    digits and underscore, otherwise you will need to
 *                    use delimited column names with matching case in
 *                    Trafodion.
 *  @param length     Length of the new character column.
 *                    For single-byte and variable byte character sets,
 *                    the length is specified in bytes. For UTF-8, this
 *                    is equivalent to CHAR(length BYTES). For UCS2, the
 *                    length is in UCS2 16-bit characters.
 *  @param isNullable true if the added column should be nullable,
 *                    false if the added column should have the NOT NULL
 *                    constraint.
 *  @param charset    Character set of the new column.
 *  @param collation  Collation of the new column.
 *  @throws UDRException
 */
void TupleInfo::addVarCharColumn(const char *colName,
                                 int length,
                                 bool isNullable,
                                 TypeInfo::SQLCharsetCode charset,
                                 TypeInfo::SQLCollationCode collation)
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

/**
 *  Add multiple columns to the table-valued output.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @param columns Vector of ColumnInfo objects describing the columns to add.
 *  @throws UDRException
 */
void TupleInfo::addColumns(const std::vector<ColumnInfo *> &columns)
{
  for (std::vector<ColumnInfo *>::const_iterator it = columns.begin();
       it != columns.end();
       it++)
    addColumn(**it);
}

/**
 *  Add a new column at a specified position.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @param column   ColumnInfo object describing the new column.
 *  @param position Position/ordinal number of the new column.
 *                  All existing columns with ordinal numbers
 *                  greater or equal to position will be shifted by one.
 *  @throws UDRException
 */
void TupleInfo::addColumnAt(const ColumnInfo &column, int position)
{
  ColumnInfo *newCol = new ColumnInfo(column);

  columns_.insert(columns_.begin() + position, newCol);
}

/**
 *  Delete a column of the table-valued output.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @param i Position/ordinal (0-based) of column to be deleted.
 *  @throws UDRException
 */
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

/**
 *  Delete a column with a specified column name.
 *
 *  The first column that matches the specified column name
 *  will be deleted.
 *
 *  @param name Name of the column to be deleted.
 *  @throws UDRException
 */
void TupleInfo::deleteColumn(const std::string &name)
{
  deleteColumn(getColNum(name));
}

/**
 *  Print the object, for use in debugging.
 *
 *  @see UDR::debugLoop()
 *  @see UDRInvocationInfo::PRINT_INVOCATION_INFO_AT_RUN_TIME
 */
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

int TupleInfo::deserialize(ConstBytes &inputBuffer,
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

  // delete all existing columns
  for (std::vector<ColumnInfo *>::iterator it1 = columns_.begin();
       it1 != columns_.end();
       it1++)
    delete *it1;
  columns_.clear();

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

  // leave rowPtr_ intact, the row is not serialized/
  // deserialized with this object

  if (getObjectType() == TUPLE_INFO_OBJ)
    validateDeserializedLength(result);

  return result;
}

char * TupleInfo::getRowPtr() const
{
  return rowPtr_;
}

/**
 *  Get the record length of a row.
 *
 *  This method returns the approximate record length of the tuple at
 *  compile time and the actual (non-compressed) record length at
 *  runtime. This might be useful for cost estimation, otherwise it
 *  can be ignored by UDF writers.
 *
 *  @return Record length in bytes.
 */
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
     estimatedNumRows_(-1),
     estimatedNumPartitions_(-1)
{}

TableInfo::~TableInfo()
{
  // delete all constraints
  for (std::vector<ConstraintInfo *>::iterator it2 = constraints_.begin();
       it2 != constraints_.end();
       it2++)
    delete *it2;
}

/**
 *  Get the estimated number of rows of this table.
 *
 *  @see setEstimatedNumRows()
 *  @see getEstimatedNumPartitions()
 *
 *  @return Estimated number of rows or -1 if there is no estimate.
 */
long TableInfo::getEstimatedNumRows() const
{
  return estimatedNumRows_;
}

/**
 *  For tables with a PARTITION BY, get estimated number of partitions.
 *
 *  @see getEstimatedNumRows()
 *  @see setEstimatedNumRows()
 *
 *  @return Estimated number of partitions or -1 if there is no estimate or no PARTITION BY.
 */
long TableInfo::getEstimatedNumPartitions() const
{
  return estimatedNumPartitions_;
}

/**
 *  Get the PARTITION BY clause for this input table.
 *
 *  This returns either the PARTITION BY clause specified in the
 *  SQL query, or the updated partitioning information, set by
 *  UDRInvocationInfo::setChildPartitioning(), called during
 *  UDR::describeParamsAndColumns().
 *
 *  @return Partitioning clause for this input table.
 */
const PartitionInfo &TableInfo::getQueryPartitioning() const
{
  return queryPartitioning_;
}

// non-const version
PartitionInfo &TableInfo::getQueryPartitioning()
{
  return queryPartitioning_;
}

/**
 *  Get the ORDER BY clause for this input table.
 *
 *  This returns either the ORDER BY clause specified in the
 *  SQL query, or the updated ordering information, set by
 *  UDRInvocationInfo::setChildOrdering(), called during
 *  UDR::describeParamsAndColumns().
 *
 *  @return Ordering clause for this input table.
 */
const OrderInfo &TableInfo::getQueryOrdering() const
{
  return queryOrdering_;
}

// non-const version
OrderInfo &TableInfo::getQueryOrdering()
{
  return queryOrdering_;
}

/**
 *  Returns whether the UDF result is treated as a continuous stream.
 *
 *  Note: This is currently not supported. The method always returns false
 *  for now.
 *
 *  @return true if the UDF result is a stream, false otherwise.
 */
bool TableInfo::isStream() const
{
  return false;
}

/**
 *  Get the number of constraints defined on this table.
 *
 *  @return Number of constraints defined on this table.
 */
int TableInfo::getNumConstraints() const
{
  return constraints_.size();
}

/**
 *  Get a constraint by index/ordinal number.
 *
 *  @param i index/ordinal (0-based) of the constraint.
 *  @return Constraint for a given index/ordinal.
 *  @throws UDRException
 */
const ConstraintInfo &TableInfo::getConstraint(int i) const
{
  if (i < 0 || i >= constraints_.size())
    throw UDRException(
         38900,
         "Trying to access constraint %d of a ConstraintInfo object with %d constraints",
         i, constraints_.size());

  return *(constraints_[i]);
}

/**
 *  Set the estimated number of rows for a UDF table-valued result.
 *
 *  Setting this value can help the Trafodion optimizer generate a better
 *  plan for queries containing table-valued UDFs. Note that this is only
 *  an estimate, a strict correspondence to the actual number of rows
 *  returned at runtime is not required.
 *
 *  Only use this method from within the following methods:
 *  @arg UDR::describeParamsAndColumns()
 *  @arg UDR::describeDataflowAndPredicates()
 *  @arg UDR::describeConstraints()
 *  @arg UDR::describeStatistics()
 *
 *  @param rows Estimated number of rows for this table.
 */
void TableInfo::setEstimatedNumRows(long rows)
{
  estimatedNumRows_ = rows;
}

/**
 *  Add a cardinality constraint to the UDF table-valued output.
 *
 *  Only use this method from within the following methods:
 *  @arg UDR::describeParamsAndColumns()
 *  @arg UDR::describeDataflowAndPredicates()
 *  @arg UDR::describeConstraints()
 *
 *  @param constraint New constraint to add. The object needs to be
 *         deallocated by the caller after this call returns.
 *  @throws UDRException
 */
void TableInfo::addCardinalityConstraint(
     const CardinalityConstraintInfo &constraint)
{
  ConstraintInfo *newConstr = new CardinalityConstraintInfo(constraint);

  constraints_.push_back(newConstr);
}

/**
 *  Add a uniqueness constraint to the UDF table-valued output.
 *
 *  Only use this method from within the following methods:
 *  @arg UDR::describeParamsAndColumns()
 *  @arg UDR::describeDataflowAndPredicates()
 *  @arg UDR::describeConstraints()
 *
 *  @param constraint New uniqueness constraint to add. The object needs
 *         to be deallocated by the caller after this call returns.
 *  @throws UDRException
 */
void TableInfo::addUniquenessConstraint(
     const UniqueConstraintInfo &constraint)
{
  ConstraintInfo *newConstr = new UniqueConstraintInfo(constraint);

  constraints_.push_back(newConstr);
}

/**
 *  Set whether a table should be treated as a stream.
 *
 *  This method is not yet supported.
 *
 *  @param stream true if the table is a stream, false otherwise.
 *  @throws UDRException
 */
void TableInfo::setIsStream(bool stream)
{
  if (stream)
    throw UDRException(38908, "Stream tables not yet supported");
}

/**
 *  Print the object, for use in debugging.
 *
 *  @see UDR::debugLoop()
 *  @see UDRInvocationInfo::PRINT_INVOCATION_INFO_AT_RUN_TIME
 */
void TableInfo::print()
{
  TupleInfo::print();
  printf("    Estimated number of rows : %ld\n", getEstimatedNumRows());
  printf("    Partitioning             : ");
  switch (getQueryPartitioning().getType())
    {
    case PartitionInfo::UNKNOWN:
      printf("unknown\n");
      break;
    case PartitionInfo::ANY:
      printf("any\n");
      break;
    case PartitionInfo::SERIAL:
      printf("serial\n");
      break;
    case PartitionInfo::PARTITION:
      {
        bool needsComma = false;
        printf("(");
        for (int p=0; p<getQueryPartitioning().getNumEntries(); p++)
          {
            if (needsComma)
              printf(", ");
            printf("%s", getColumn(getQueryPartitioning().getColumnNum(p)).getColName().c_str());
            needsComma = true;
          }
        printf(")\n");
        printf("    Estimated # of partitions: %ld\n", getEstimatedNumPartitions());
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
      printf("(");
      for (int o=0; o<getQueryOrdering().getNumEntries(); o++)
        {
          if (o>0)
            printf(", ");
          printf("%s",
                 getColumn(
                      getQueryOrdering().getColumnNum(o)).getColName().c_str());

          OrderInfo::OrderTypeCode ot = getQueryOrdering().getOrderType(o);
          if (ot == OrderInfo::DESCENDING)
            printf(" DESC");
          else if (ot != OrderInfo::ASCENDING)
            printf(" - invalid order type!");
        }
      printf(")\n");
    }
  else
    printf("none\n");
  if (constraints_.size() > 0)
    {
      printf("    Constraints              :\n");

      for (int c=0; c<constraints_.size(); c++)
        {
          std::string s = "        ";

          constraints_[c]->toString(*this, s);
          printf("%s\n", s.c_str());
        }
    }
}

void TableInfo::setQueryPartitioning(const PartitionInfo &partInfo)
{
  queryPartitioning_ = partInfo;
}

void TableInfo::setQueryOrdering(const OrderInfo &orderInfo)
{
  queryOrdering_ = orderInfo;
}

int TableInfo::serializedLength()
{
  // format: base class + long(numRows) + long(numParts) +
  // int(#part cols) + int(#order cols) +
  // binary array of ints:
  // p*int(partkeycol#) +
  // o*(int(ordercol#) + int(ordering)) +
  // int(#constraints) + constraints
  int result = TupleInfo::serializedLength() +
    2 * serializedLengthOfLong() +
    4 * serializedLengthOfInt() +
    serializedLengthOfBinary(
         (getQueryPartitioning().getNumEntries() +
          2 * getQueryOrdering().getNumEntries()) * sizeof(int));

  for (int c=0; c<constraints_.size(); c++)
    result += constraints_[c]->serializedLength();

  return result;
}

int TableInfo::serialize(Bytes &outputBuffer,
                         int &outputBufferLength)
{
  int result = 
    TupleInfo::serialize(outputBuffer,
                         outputBufferLength);
  int numPartCols = queryPartitioning_.getNumEntries();
  int numOrderCols = queryOrdering_.getNumEntries();
  int numConstraints = constraints_.size();
  int *intArray = new int[numPartCols + 2*numOrderCols];
  int c;

  result += serializeLong(estimatedNumRows_,
                          outputBuffer,
                          outputBufferLength);

  result += serializeLong(estimatedNumPartitions_,
                          outputBuffer,
                          outputBufferLength);

  result += serializeInt(numPartCols,
                         outputBuffer,
                         outputBufferLength);

  result += serializeInt(numOrderCols,
                         outputBuffer,
                         outputBufferLength);

  result += serializeInt(static_cast<int>(queryPartitioning_.getType()),
                         outputBuffer,
                         outputBufferLength);

  for (c=0; c<numPartCols; c++)
    intArray[c] = queryPartitioning_.getColumnNum(c);
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

  result += serializeInt(numConstraints,
                         outputBuffer,
                         outputBufferLength);
  for (c=0; c<numConstraints; c++)
    result += constraints_[c]->serialize(outputBuffer,
                                         outputBufferLength);

  validateSerializedLength(result);

  return result;
}

int TableInfo::deserialize(ConstBytes &inputBuffer,
                           int &inputBufferLength)
{
  int result =
    TupleInfo::deserialize(inputBuffer, inputBufferLength);

  validateObjectType(TABLE_INFO_OBJ);
  int numCols = 0;
  int numPartCols = 0;
  int numOrderCols = 0;
  int numConstraints = 0;
  int partType = 0;
  const int *intArray = NULL;
  int binarySize = 0;
  int c;

  result += deserializeLong(estimatedNumRows_,
                            inputBuffer,
                            inputBufferLength);

  result += deserializeLong(estimatedNumPartitions_,
                            inputBuffer,
                            inputBufferLength);

  result += deserializeInt(numPartCols,
                           inputBuffer,
                           inputBufferLength);

  result += deserializeInt(numOrderCols,
                           inputBuffer,
                           inputBufferLength);

  result += deserializeInt(partType,
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
  queryPartitioning_.clear();
  queryPartitioning_.setType(
       static_cast<PartitionInfo::PartitionTypeCode>(partType));
  for (c=0; c<numPartCols; c++)
    queryPartitioning_.addEntry(intArray[c]);
  queryOrdering_.clear();
  for (c=0; c<numOrderCols; c++)
    queryOrdering_.addEntry(
         intArray[numPartCols+2*c],
         static_cast<OrderInfo::OrderTypeCode>(intArray[numPartCols+2*c+1]));

  // delete all constraints
  for (std::vector<ConstraintInfo *>::iterator it2 = constraints_.begin();
       it2 != constraints_.end();
       it2++)
    delete *it2;
  constraints_.clear();

  result += deserializeInt(numConstraints,
                           inputBuffer,
                           inputBufferLength);
  for (c=0; c<numConstraints; c++)
    {
      ConstraintInfo *constr = NULL;

      // look ahead what the next object type is and allocate
      // an empty object of the appropriate subclass
      switch (getNextObjectType(inputBuffer,
                                inputBufferLength))
        {
        case CARDINALITY_CONSTRAINT_INFO_OBJ:
          constr = new CardinalityConstraintInfo();
          break;
        case UNIQUE_CONSTRAINT_INFO_OBJ:
          constr = new UniqueConstraintInfo();
          break;
        default:
          throw UDRException(
               38900,
               "Invalid object type during constraint deserialization: %d",
               static_cast<int>(getNextObjectType(inputBuffer,
                                                  inputBufferLength)));
        }
      // deserialize the object and add it to the list of constraints
      result += constr->deserialize(inputBuffer,
                                    inputBufferLength);
      constraints_.push_back(constr);
    }

  validateDeserializedLength(result);

  return result;
}


// ------------------------------------------------------------------------
// Member functions for class ParameterListInfo
// ------------------------------------------------------------------------

ParameterListInfo::ParameterListInfo() :
     TupleInfo(PARAMETER_LIST_INFO_OBJ, getCurrentVersion())
{
}

ParameterListInfo::~ParameterListInfo()
{
}

int ParameterListInfo::serializedLength()
{
  // format: Base class
  return TupleInfo::serializedLength();
}
int ParameterListInfo::serialize(Bytes &outputBuffer,
                                 int &outputBufferLength)
{
  int result = TupleInfo::serialize(outputBuffer,
                                    outputBufferLength);

  validateSerializedLength(result);

  return result;
}

int ParameterListInfo::deserialize(ConstBytes &inputBuffer,
                                   int &inputBufferLength)
{
  int result =
    TupleInfo::deserialize(inputBuffer,
                           inputBufferLength);

  validateObjectType(PARAMETER_LIST_INFO_OBJ);
  validateDeserializedLength(result);

  return result;
}


// ------------------------------------------------------------------------
// Member functions for class UDRWriterCompileTimeData
// ------------------------------------------------------------------------

/**
 *  Default constructor.
 *
 *  UDR writers can derive from this class to store state between
 *  the calls of the compiler interface.
 */
UDRWriterCompileTimeData::UDRWriterCompileTimeData()
{}

/**
 *  Virtual destructor.
 *
 *  Override the virtual destructor in derived classes to clean up any
 *  resources owned by the UDR writer once the compile phase of a
 *  query is completed.
 */
UDRWriterCompileTimeData::~UDRWriterCompileTimeData()
{}

/**
 *  Print the object, for use in debugging.
 *
 *  @see UDR::debugLoop()
 *  @see UDRInvocationInfo::PRINT_INVOCATION_INFO_AT_RUN_TIME
 */
void UDRWriterCompileTimeData::print()
{
  printf("no print method provided for UDR Writer compile time data\n");
}

// ------------------------------------------------------------------------
// Member functions for class UDRInvocationInfo
// ------------------------------------------------------------------------

UDRInvocationInfo::UDRInvocationInfo() :
     TMUDRSerializableObject(UDR_INVOCATION_INFO_OBJ,
                             getCurrentVersion()),
     numTableInputs_(0),
     callPhase_(UNKNOWN_CALL_PHASE),
     funcType_(GENERIC),
     debugFlags_(0),
     sqlAccessType_(CONTAINS_NO_SQL),
     sqlTransactionType_(REQUIRES_NO_TRANSACTION),
     sqlRights_(INVOKERS_RIGHTS),
     isolationType_(TRUSTED),
     udrWriterCompileTimeData_(NULL),
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
}

/**
 *  Get the UDR name.
 *
 *  @return Fully qualified name (catalog.schema.name) of the UDR.
 */
const std::string &UDRInvocationInfo::getUDRName() const
{
  return name_;
}

/**
 *  Get number of table-valued inputs provided.
 *
 *  @return Number of table-valued inputs provided.
 */
int UDRInvocationInfo::getNumTableInputs() const
{
  return numTableInputs_;
}

/**
 *  Get description of a table-valued input.
 *
 *  @return TableInfo reference for the table-valued input.
 *  @throws UDRException
 */
const TableInfo &UDRInvocationInfo::in(int childNum) const
{
  if (childNum < 0 || childNum >= numTableInputs_)
    throw UDRException(38909, "Invalid child table number %d", childNum);

  return inputTableInfo_[childNum];
}

/**
 *  Get description of the table-valued result.
 *
 *  @return TableInfo reference for the table-valued output.
 */
const TableInfo &UDRInvocationInfo::out() const
{
  return outputTableInfo_;
}

/**
 *  Non-const method to get description of the table-valued result.
 *
 *  @return Non-const TableInfo reference for the table-valued output.
 */
TableInfo &UDRInvocationInfo::out()
{
  return outputTableInfo_;
}

/**
 *  Get call phase.
 *
 *  This call is not normally needed, since we know which method
 *  of UDR we are in. However, in some cases where the UDR
 *  writer wants to use code in multiple call phases this might
 *  be useful.
 *
 *  @return Enum for the call phase we are in.
 */
UDRInvocationInfo::CallPhase UDRInvocationInfo::getCallPhase() const
{
  return callPhase_;
}

/**
 *  Get current user.
 *
 *  Get the id of the current user, which is the effective
 *  user id at the time. This is usually the same as
 *  the session user, except when a view or UDR uses "definer
 *  privileges", substituting the current user with the
 *  definer of the view or UDR. In SQL, this value is
 *  called CURRENT_USER.
 *
 *  @see getSessionUser()
 *  @return Current user.
 */
const std::string &UDRInvocationInfo::getCurrentUser() const
{
  return currentUser_;
}

/**
 *  Get session user.
 *
 *  Get the id of the session user, which is the user who
 *  connected to the database. This is usually the same as
 *  the current user, except when a view or UDR uses "definer
 *  privileges", substituting the current user with the
 *  definer of the view or UDR. In SQL, this value is
 *  called SESSION_USER.
 *
 *  @see getCurrentUser()
 *  @return Session user.
 */
const std::string &UDRInvocationInfo::getSessionUser() const
{
  return sessionUser_;
}

/**
 *  Get current role.
 *
 *  @return Current role.
 */
const std::string &UDRInvocationInfo::getCurrentRole() const
{
  return currentRole_;
}

/**
 *  Get query id.
 *
 *  The query id is only available at runtime. It is an empty
 *  string at compile time.
 *
 *  @return Query id.
 */
const std::string &UDRInvocationInfo::getQueryId() const
{
  return queryId_;
}

// The next four methods are not yet documented in Doxygen,
// since there is no choice yet. Add them to the documentation
// when we support more than one choice.
UDRInvocationInfo::SQLAccessType UDRInvocationInfo::getSQLAccessType() const
{
  return sqlAccessType_;
}

UDRInvocationInfo::SQLTransactionType
UDRInvocationInfo::getSQLTransactionType() const
{
  return sqlTransactionType_;
}

UDRInvocationInfo::SQLRightsType UDRInvocationInfo::getSQLRights() const
{
  return sqlRights_;
}

UDRInvocationInfo::IsolationType UDRInvocationInfo::getIsolationType() const
{
  return isolationType_;
}

/**
 *  Check whether we are in the compile time interface.
 *
 *  @return true at compile time, false at run-time.
 */
bool UDRInvocationInfo::isCompileTime() const
{
  return (callPhase_ <= COMPILER_INITIAL_CALL &&
          callPhase_ <= COMPILER_COMPLETION_CALL);
}

/**
 *  Check whether we are in the run-time interface.
 *
 *  @return false at compile time, true at run-time.
 */
bool UDRInvocationInfo::isRunTime() const
{
  return (callPhase_ >= RUNTIME_WORK_CALL);
}

/**
 *  Get debugging flags, set via CONTROL QUERY DEFAULT.
 *
 *  Debug flags are set via the UDR_DEBUG_FLAGS CONTROL QUERY DEFAULT
 *  at compile time. This returns the value of this CQD. Usually not
 *  needed.
 *
 *  @return Value the UDR_DEBUG_FLAGS CQD has or had at compile time.
 */
int UDRInvocationInfo::getDebugFlags() const
{
  return debugFlags_;
}

/**
 *  Get the function type of this UDR invocation.
 *
 *  Returns the function type that can be set by the UDR writer
 *  with the setFuncType() method.
 *
 *  @see setFuncType()
 *
 *  @return Enum of the function type.
 */
UDRInvocationInfo::FuncType UDRInvocationInfo::getFuncType() const
{
  return funcType_;
}

/**
 *  Get the formal parameters of the UDR invocation.
 *
 *  Formal parameters are available only at compile time.
 *  They are either defined in the CREATE FUNCTION DDL or through
 *  the compile time interface. Note that number and types of formal
 *  and actual parameters must match, once we return from the
 *  describeParamsAndColumns() call, otherwise an error will be generated.
 *
 *  @return Formal parameter description.
 */
const ParameterListInfo &UDRInvocationInfo::getFormalParameters() const
{
  return formalParameterInfo_;
}

/**
 *  Get parameters of the UDR invocation.
 *
 *  These are the actual parameters. At compile time, if a constant
 *  has been used, the value of this constant is available, using
 *  getString(), getInt() etc. methods. The isAvailable() method indicates
 *  whether the parameter is indeed available at compile time. Parameters
 *  are always available at run-time.
 *
 *  @return Parameter description.
 */
const ParameterListInfo &UDRInvocationInfo::par() const
{
  return actualParameterInfo_;
}

ParameterListInfo &UDRInvocationInfo::nonConstFormalParameters()
{
  return formalParameterInfo_;
}

ParameterListInfo &UDRInvocationInfo::nonConstActualParameters()
{
  return actualParameterInfo_;
}

/**
 *  Return number of predicates to be applied in the context of this UDF.
 *
 *  Don't use this method from within UDR::describeParamsAndColumns(),
 *  since the predicates are not yet set up in that phase.
 *
 *  @return Number of predicates.
 */
int UDRInvocationInfo::getNumPredicates() const
{
  // predicates are not yet set up in the initial call
  validateCallPhase(COMPILER_DATAFLOW_CALL, RUNTIME_WORK_CALL,
                    "UDRInvocationInfo::getNumPredicates()");
  return predicates_.size();
}

/**
 *  Get the description of a predicate to be applied.
 *
 *  @return Description of the predicate.
 *
 *  @see setPredicateEvaluationCode()
 *  @throws UDRException
 */
const PredicateInfo &UDRInvocationInfo::getPredicate(int i) const
{
  if (i < 0 || i >= predicates_.size())
    throw UDRException(
         38900,
         "Trying to access predicate %d of a PredicateInfo object with %d predicates",
         i, predicates_.size());

  return *(predicates_[i]);
}

/**
 *  Check whether a given predicate is a comparison predicate.
 *
 *  This returns whether it is safe to use method getComparisonPredicate().
 *
 *  @see getComparisonPredicate()
 *
 *  @param i Number/ordinal index of the predicate.
 *  @return true if predcate i is a comparison predicate, false otherwise.
 *  @throws UDRException
 */
bool UDRInvocationInfo::isAComparisonPredicate(int i) const
{
  return getPredicate(i).isAComparisonPredicate();
}

/**
 *  Get a comparison predicate
 *
 *  Note: This will throw an exception if predicate i is not a
 *  comparison predicate. Use method isAComparisonPredicate() to
 *  make sure this is the case. Note also that the numbering
 *  scheme is the same as that for getPredicate, so if there is
 *  a mix of different predicate types, the numbers of comparison
 *  predicates are not contiguous.
 *
 *  @see getPredicate()
 *  @see isAComparisonPredicate()
 *  @param i Number/ordinal of the predicate to retrieve.
 *  @return Comparison predicate.
 *  @throws UDRException
 */
const ComparisonPredicateInfo &UDRInvocationInfo::getComparisonPredicate(
     int i) const
{
  if (!isAComparisonPredicate(i))
    throw UDRException(38900,
                       "Predicate %d is not a comparison predicate",
                       i);
  return dynamic_cast<ComparisonPredicateInfo &>(*(predicates_[i]));
}

/**
 *  Add a formal parameter to match an actual parameter.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @see describeParamsAndColumns()
 *
 *  @param param Info with name and type of the formal parameter.
 *
 *  @throws UDRException
 */
void UDRInvocationInfo::addFormalParameter(const ColumnInfo &param)
{
  validateCallPhase(COMPILER_INITIAL_CALL, COMPILER_INITIAL_CALL,
                    "UDRInvocationInfo::addFormalParameter()");

  formalParameterInfo_.addColumn(param);
}

/**
 *  Set the function type of this UDR invocation.
 *
 *  Use this simple method with some caution, since it has an effect
 *  on how predicates are pushed down through TMUDFs with table-valued
 *  inputs. See describeDataflowAndPredicates() for details. The function
 *  type also influences the default degree of parallelism for a TMUDF.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @see getFunctType()
 *  @see describeParamsAndColumns()
 *  @see describeDataflowAndPredicates()
 *  @see setDesiredDegreeOfParallelism()
 *
 *  @param type Function type of this UDR invocation.
 *  @throws UDRException
 */
void UDRInvocationInfo::setFuncType(FuncType type)
{
  validateCallPhase(COMPILER_INITIAL_CALL, COMPILER_INITIAL_CALL,
                    "UDRInvocationInfo::setFuncType()");

  funcType_ = type;

  // also set the default value for partitioning of table-valued inputs
  // to ANY, if this UDF is a mapper, to allow parallel execution
  // without restrictions
  if (type == MAPPER &&
      getNumTableInputs() == 1 &&
      in().getQueryPartitioning().getType() == PartitionInfo::UNKNOWN)
    inputTableInfo_[0].getQueryPartitioning().setType(PartitionInfo::ANY);
}

/**
 *  Add columns of table-valued inputs as output columns.
 *
 *  Many TMUDFs make the column values of their table-valued inputs available
 *  as output columns. Such columns are called "pass-thru" columns. This
 *  method is an easy interface to create such pass-thru columns. Note that
 *  if a column is marked as pass-thru column, the UDF must copy the input
 *  value to the output (e.g. with the copyPassThruData() method). If it fails
 *  to do that, incorrect results may occur, because the compiler makes
 *  the assumptions that these values are the same.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @see UDR::describeParamsAndColumns()
 *  @see ProvenanceInfo
 *  @see ColumnInfo::getProvenance()
 *
 *  @param inputTableNum    Index of table-valued input to add.
 *  @param startInputColNum First column of the table-valued input to add
 *                          as an output column.
 *  @param endInputColNum   Last column of the table-valued input to add
 *                          as an output column (note this is inclusive)
 *                          or -1 to add all remaining column.
 *  @throws UDRException
 */
void UDRInvocationInfo::addPassThruColumns(int inputTableNum,
                                           int startInputColNum,
                                           int endInputColNum)
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

  if (endInputColNum == -1)
    endInputColNum = in(inputTableNum).getNumColumns() - 1;

  for (int c=startInputColNum; c<=endInputColNum; c++)
    {
      // make a copy of the input column
      ColumnInfo newCol(in(inputTableNum).getColumn(c));

      // change the provenance info of the column
      newCol.setProvenance(ProvenanceInfo(inputTableNum, c));

      outputTableInfo_.addColumn(newCol);
    }
}

/**
 *  Set the PARTITION BY info for a table-valued input.
 *
 *  This method allows the UDR writer to override the
 *  PARTITION BY syntax specified for a table-valued input
 *  in the query. Use it to change the required partitioning.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @see getChildPartitioning()
 *  @see UDR::describeParamsAndColumns()
 *
 *  @param inputTableNum Number of table-valued input to set.
 *  @param partInfo New information on required partitioning for this input table.
 *  @throws UDRException
 */
void UDRInvocationInfo::setChildPartitioning(int inputTableNum,
                                             const PartitionInfo &partInfo)
{
  validateCallPhase(COMPILER_INITIAL_CALL, COMPILER_INITIAL_CALL,
                    "UDRInvocationInfo::setChildPartitioning()");
  if (inputTableNum < 0 || inputTableNum >= numTableInputs_)
    throw UDRException(38900, "Invalid child table number %d", inputTableNum);

  inputTableInfo_[inputTableNum].setQueryPartitioning(partInfo);
}

/**
 *  Set the ORDER BY info for a table-valued input.
 *
 *  This method allows the UDR writer to override the
 *  ORDER BY syntax specified for a table-valued input
 *  in the query. Use it to change the required order.
 *
 *  Only use this method from within the
 *  UDR::describeParamsAndColumns() method.
 *
 *  @see getChildOrdering()
 *  @see UDR::describeParamsAndColumns()
 *
 *  @param inputTableNum Number of table-valued input to set.
 *  @param orderInfo New information on required order for this input table.
 *  @throws UDRException
 */
void UDRInvocationInfo::setChildOrdering(int inputTableNum,
                                         const OrderInfo &orderInfo)
{
  validateCallPhase(COMPILER_INITIAL_CALL, COMPILER_INITIAL_CALL,
                    "UDRInvocationInfo::setChildOrder()");
  if (inputTableNum < 0 || inputTableNum >= numTableInputs_)
    throw UDRException(38900, "Invalid child table number %d", inputTableNum);
  inputTableInfo_[inputTableNum].setQueryOrdering(orderInfo);
}

/**
 *  Set the usage information for a column of a table-valued input
 *
 *  This method allows the UDR writer to specify whether a given
 *  child column is needed or not.
 *
 *  Only use this method from within the
 *  UDR::describeDataflowAndPredicates() method.
 *
 *  @see setUnusedPassthruColumns()
 *  @see UDR::describeDataflowAndPredicates()
 *
 *  @param inputTableNum  Number of table-valued input to set.
 *  @param inputColumnNum Column number for the column to set.
 *  @param usage          New usage for this column.
 *  @throws UDRException
 */
void UDRInvocationInfo::setChildColumnUsage(int inputTableNum,
                                            int inputColumnNum,
                                            ColumnInfo::ColumnUseCode usage)
{
  in(inputTableNum); // validate inputTableNum
  inputTableInfo_[inputTableNum].getColumn(inputColumnNum).setUsage(usage);
}

/**
 *  Mark any passthru columns that are not needed as unused.
 *
 *  For any passthru columns that are marked as NOT_USED or NOT_PRODUCED in
 *  the table-valued result, set the corresponding input columns
 *  to NOT_USED as well. Note that this assumes that the UDF
 *  does not need these columns, either! The usage for the passthru column
 *  itself is also set to NOT_PRODUCED, since the UDF could not produce
 *  the column without having access to the corresponding input column.
 *
 *  Only use this method from within the
 *  UDR::describeDataflowAndPredicates() method.
 *
 *  @see addPassThruColumns()
 *  @see setChildColumnUsage()
 *  @see UDR::describeDataflowAndPredicates()
 *
 *  @throws UDRException
 */
void UDRInvocationInfo::setUnusedPassthruColumns()
{
  int numOutCols = out().getNumColumns();

  // loop over output columns
  for (int oc=0; oc<numOutCols; oc++)
    {
      ColumnInfo &colInfo = out().getColumn(oc);
      ColumnInfo::ColumnUseCode usage = colInfo.getUsage();
      const ProvenanceInfo &prov = colInfo.getProvenance();
      int it = prov.getInputTableNum();
      int ic = prov.getInputColumnNum();

      // is this a pass-thru column that is not used?
      if (it >= 0 && ic >= 0 &&
          (usage == ColumnInfo::NOT_USED ||
           usage == ColumnInfo::NOT_PRODUCED))
        {
          setChildColumnUsage(it, ic, ColumnInfo::NOT_USED);
          // also make sure the output column is not produced, since
          // we could not get its value from the table-valued input
          colInfo.setUsage(ColumnInfo::NOT_PRODUCED);
        }
    }
}

/**
 *  Decide where to evaluate a predicate.
 *
 *  Only use this method from within the
 *  UDR::describeDataflowAndPredicates() method.
 *
 *  @see getPredicate()
 *  @see UDR::describeDataflowAndPredicates()
 *
 *  @param predicateNum Number/index of predicate returned by getPredicate()
 *                      method.
 *  @param c            Evaluation code for this predicate.
 *  @throws UDRException
 */
void UDRInvocationInfo::setPredicateEvaluationCode(int predicateNum,
                                                   PredicateInfo::EvaluationCode c)
{
  validateCallPhase(COMPILER_DATAFLOW_CALL, COMPILER_DATAFLOW_CALL,
                    "UDRInvocationInfo::setPredicateEvaluationCode()");

  // validate index
  const PredicateInfo &pred = getPredicate(predicateNum);

  if (c == PredicateInfo::EVALUATE_IN_UDF &&
      pred.isAComparisonPredicate() &&
      !(dynamic_cast<const ComparisonPredicateInfo &>(pred).hasAConstantValue()))
    throw UDRException(
         38900,
         "Comparison predicate %d cannot be evaluated in the UDF since it does not refer to a constant value",
         predicateNum);
  predicates_[predicateNum]->setEvaluationCode(c);
}

/**
 *  Push predicates on pass-thru columns to the table-valued input.
 *
 *  Push one or more predicates to their corresponding table-valued input,
 *  if they reference only columns from that input, otherwise leave the
 *  predicate(s) unchanged.
 *
 *  Only use this method from within the
 *  UDR::describeDataflowAndPredicates() method.
 *
 *  @see PredicateInfo::setEvaluationCode()
 *  @see UDR::describeDataflowAndPredicates()
 *
 *  @param startPredNum Number/index of first predicate to be pushed.
 *  @param lastPredNum  Number/index of last predicate to be pushed (inclusive)
 *                      or -1 to push all remaining predicates.
 *  @throws UDRException
 */
void UDRInvocationInfo::pushPredicatesOnPassthruColumns(int startPredNum,
                                                        int lastPredNum)
{
  validateCallPhase(COMPILER_DATAFLOW_CALL, COMPILER_DATAFLOW_CALL,
                    "UDRInvocationInfo::pushPredicatesOnPassthruColumns()");

  int numPreds = getNumPredicates();

  // loop over predicates in the specified range
  for (int p = startPredNum;
       p<numPreds && (p<=lastPredNum || lastPredNum == -1);
       p++)
    if (isAComparisonPredicate(p))
      {
        const ComparisonPredicateInfo &cpi = getComparisonPredicate(p);

        if (out().getColumn(cpi.getColumnNumber()).
            getProvenance().isFromInputTable())
          // Yes, this predicate is a comparison predicate on a pass-thru
          // column (note we do not allow predicates of the form
          // "col1 op col2"). Push it down.
          setPredicateEvaluationCode(p,PredicateInfo::EVALUATE_IN_CHILD);
      }
}

/**
 *  Propagate constraints for UDFs that return one result row for
 *         every input row.
 *
 *  Use this method only if the UDF returns no more than one result row for
 *  every input row it reads from its single table-valued input. Note that
 *  it is ok for the UDF to return no result rows for some input rows.
 *  Wrong results may be returned by SQL statements involving this UDF if
 *  the UDF does at runtime not conform to the 1x1 relationship of rows.
 *
 *  Only use this method from within the UDR::describeConstraints() method.
 *
 *  @param exactlyOneRowPerInput Indicates whether the UDF returns exactly
 *                               one output row (true) or at most one output
 *                               row (false) for every input row.
 */
void UDRInvocationInfo::propagateConstraintsFor1To1UDFs(
     bool exactlyOneRowPerInput)
{
  validateCallPhase(COMPILER_CONSTRAINTS_CALL, COMPILER_CONSTRAINTS_CALL,
                    "UDRInvocationInfo::propagateConstraintsFor1To1UDFs()");

  if (getNumTableInputs() == 1)
    {
      int numConstraints = in().getNumConstraints();
      int numOutputCols = out().getNumColumns();

      for (int c=0; c<numConstraints; c++)
        switch (in().getConstraint(c).getType())
          {
          case ConstraintInfo::CARDINALITY:
            {
              const CardinalityConstraintInfo &cc = 
                static_cast<const CardinalityConstraintInfo &>(
                     in().getConstraint(c));

              // add a cardinality constraint to the parent with
              // an adjusted lower bound of 0 if exactlyOneRowPerInput
              // is false
              out().addCardinalityConstraint(CardinalityConstraintInfo(
                                                  (exactlyOneRowPerInput ?
                                                   cc.getMinNumRows() :
                                                   0),
                                                  cc.getMaxNumRows()));
            }
            break;

          case ConstraintInfo::UNIQUE:
            {
              UniqueConstraintInfo ucParent;
              const UniqueConstraintInfo &ucChild = 
                static_cast<const UniqueConstraintInfo &>(
                     in().getConstraint(c));
              int numUniqueCols = ucChild.getNumUniqueColumns();

              // translate child columns into parent columns
              for (int uc=0; uc<numUniqueCols; uc++)
                for (int oc=0; oc<numOutputCols; oc++)
                  if (out().getColumn(oc).getProvenance().getInputColumnNum()
                      == ucChild.getUniqueColumn(uc))
                    {
                      ucParent.addColumn(oc);
                      break;
                    }

              if (ucParent.getNumUniqueColumns() == numUniqueCols)
                // we were able to translate all the unique columns on the
                // child into unique columns of the parent, add the constraint
                out().addUniquenessConstraint(ucParent);
            }
            break;

          default:
            // should not see this
            break;
          }
    }
}

/**
 *  Get data to persist between calls of the compile-time interface
 *
 *  The UDR writer must use a static or dynamic cast to get a pointer
 *  to the derived class.
 *
 *  Only use this method at compile time.
 *
 *  @see setUDRWriterCompileTimeData()
 *
 *  @return UDR writer-specific data that was previously attached or NULL.
 *  @throws UDRException
 */
UDRWriterCompileTimeData *UDRInvocationInfo::getUDRWriterCompileTimeData()
{
  validateCallPhase(COMPILER_INITIAL_CALL, COMPILER_COMPLETION_CALL,
                    "UDRInvocationInfo::getUDRWriterCompileTimeData()");

  return udrWriterCompileTimeData_;
}

/**
 *  Set data to persist between calls of the compile-time interface
 *
 *  This call can be used to attach an object derived from class
 *  UDRWriterCompileTimeData to the UDRInvocationInfo object. Once
 *  attached, the data will be carried between the stages of the
 *  compiler interface and can be used to keep state. Note that
 *  this data will be deleted at the end of the compiler phase and
 *  will not persist until runtime.
 *
 *  Only use this method at compile time.
 *
 *  To keep state for specific plan alternatives, use the
 *  UDRPlanInfo::setUDRWriterCompileTimeData() method.
 *
 *  @see UDRInvocationInfo::getUDRWriterCompileTimeData()
 *  @see UDRPlanInfo::setUDRWriterCompileTimeData()
 *  @see getUDRWriterCompileTimeData()
 *
 *  @param compileTimeData UDR writer-defined compile-time data to attach.
 *  @throws UDRException
 */
void UDRInvocationInfo::setUDRWriterCompileTimeData(
     UDRWriterCompileTimeData *compileTimeData)
{
  validateCallPhase(COMPILER_INITIAL_CALL, COMPILER_PLAN_CALL,
                    "UDRInvocationInfo::setUDRWriterCompileTimeData()");

  if (udrWriterCompileTimeData_)
    delete udrWriterCompileTimeData_;

  udrWriterCompileTimeData_ = compileTimeData;
}

/**
 *  Copy values of pass-thru columns from the input to the output table.
 *
 *  This method is an easy way to set the values of the table-valued result
 *  row from their corresponding values in the table-valued inputs.
 *  Note that the UDR must set all the values of the pass-thru columns to
 *  the corresponsing values of the input tables. If it fails to do that,
 *  some optimizations done by Trafodion could lead to wrong results
 *  (e.g. some predicates could be applied incorrectly). Every TMUDF with
 *  table-valued inputs and pass-thru columns should call this method for
 *  every row it emits.
 *
 *  This method can only be called from within UDR::processData().
 *
 *  @see addPassThruColumns()
 *  @see UDR::processData()
 *
 *  @param inputTableNum    Number of table-valued input to copy from.
 *  @param startInputColNum First column number in the input table to copy
 *  @param endInputColNum   Last column number in the input table to copy
 *                          (inclusive) or -1 to copy all remaining columns
 *  @throws UDRException
 */
void UDRInvocationInfo::copyPassThruData(int inputTableNum,
                                         int startInputColNum,
                                         int endInputColNum)
{
  // no need to validate call phase, this will raise an exception at compile time
  // validateCallPhase(RUNTIME_INITIAL_CALL, RUNTIME_FINAL_CALL,
  //                   "UDRInvocationInfo::copyPassThruData()");

  int endColNum = endInputColNum;
  int numOutCols = out().getNumColumns();

  if (endInputColNum < 0 ||
      endInputColNum >= in(inputTableNum).getNumColumns())
    endColNum = in(inputTableNum).getNumColumns() - 1;

  // loop through the output columns and pick up those that
  // are passed through from the specified input columns
  for (int oc=0; oc<numOutCols; oc++)
    {
      const ProvenanceInfo &prov = out().getColumn(oc).getProvenance();
      int it = prov.getInputTableNum();
      int ic = prov.getInputColumnNum();

      if (it == inputTableNum &&
          ic >= startInputColNum &&
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
            case TypeInfo::LOB_SUB_CLASS:
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
            case TypeInfo::YEAR_MONTH_INTERVAL_TYPE:
            case TypeInfo::DAY_SECOND_INTERVAL_TYPE:
            case TypeInfo::BOOLEAN_SUB_CLASS:
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

            case TypeInfo::UNDEFINED_TYPE_SUB_CLASS:
            default:
              throw UDRException(
                   38900,
                   "Invalid or unsupported type subclass in UDRInvocationInfo::copyPassThruData: %d",
                   (int) ty.getSQLTypeSubClass());
            }
        }
    }
}

/**
 *  Get the number of parallel instances working on this UDR invocation.
 *
 *  Use this method to find out how many parallel instances are
 *  executing this UDR.
 *
 *  This method can only be called from within UDR::processData().
 *
 *  @see getMyInstanceNum()
 *  @return Number of parallel instances for this UDR invocation.
 *  @throws UDRException
 */
int UDRInvocationInfo::getNumParallelInstances() const
{
  validateCallPhase(RUNTIME_WORK_CALL, RUNTIME_WORK_CALL,
                    "UDRInvocationInfo::getNumParallelInstances()");

  return totalNumInstances_;
}

/**
 *  Get the instance number of this runtime process.
 *
 *  Use this method to find out which of the parallel instances
 *  executing a UDR this process is.
 *
 *  This method can only be called from within UDR::processData().
 *
 *  @see getNumParallelInstances()
 *  @return A number between 0 and getNumParallelInstances() - 1.
 *  @throws UDRException
 */
int UDRInvocationInfo::getMyInstanceNum() const
{
  validateCallPhase(RUNTIME_WORK_CALL, RUNTIME_WORK_CALL,
                    "UDRInvocationInfo::getMyInstanceNum()");

  return myInstanceNum_;
}

/**
 *  Print the object, for use in debugging.
 *
 *  @see UDR::debugLoop()
 *  @see UDRInvocationInfo::PRINT_INVOCATION_INFO_AT_RUN_TIME
 */
void UDRInvocationInfo::print()
{
  printf("\nUDRInvocationInfo\n-----------------\n");
  printf("UDR Name                   : %s\n", getUDRName().c_str());
  printf("Num of table-valued inputs : %d\n", getNumTableInputs());
  printf("Call phase                 : %s\n", callPhaseToString(callPhase_));
  printf("Debug flags                : 0x%x\n", getDebugFlags());
  printf("Function type              : %s\n", (funcType_ == GENERIC ? "GENERIC" :
                                               (funcType_ == MAPPER ? "MAPPER" :
                                                (funcType_ == REDUCER ? "REDUCER" :
                                                 (funcType_ == REDUCER_NC ? "REDUCER_NC" :
                                                  "Invalid function type")))));
  printf("User id                    : %s\n", getCurrentUser().c_str());
  printf("Session user id            : %s\n", getSessionUser().c_str());
  printf("User role                  : %s\n", getCurrentRole().c_str());
  if (isRunTime())
    printf("Query id                   : %s\n", getQueryId().c_str());

  bool needsComma = false;

  if (!isRunTime())
    {
      printf("Formal parameters          : (");
      for (int p=0; p<getFormalParameters().getNumColumns(); p++)
        {
          std::string buf;

          if (needsComma)
            printf(", ");
          getFormalParameters().getColumn(p).toString(buf);
          printf("%s", buf.c_str());
          needsComma = true;
        }
      printf(")\n");
    }

  printf("Actual parameters          : (");
  needsComma = false;
  const ParameterListInfo &pli = par();

  for (int p=0; p < pli.getNumColumns(); p++)
    {

      if (needsComma)
        printf(", ");

      if (pli.isAvailable(p))
        {
          std::string strVal = pli.getString(p);

          if (pli.wasNull())
            printf("NULL");
          else
            printf("'%s'", strVal.c_str());
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

  if (isRunTime())
    printf("Instance number (0-based)  : %d of %d\n",
           getMyInstanceNum(),
           getNumParallelInstances());

  for (int c=0; c<getNumTableInputs(); c++)
    {
      printf("\nInput TableInfo %d\n-----------------\n", c);
      const_cast<TableInfo &>(in(c)).print();
    }
  printf("\nOutput TableInfo\n----------------\n");
  outputTableInfo_.print();

  if (predicates_.size() > 0)
    {
      printf("\nPredicates\n----------\n");

      for (int p=0; p<getNumPredicates(); p++)
        {
          std::string predString;

          getPredicate(p).toString(predString, out());
          switch (getPredicate(p).getEvaluationCode())
            {
            case PredicateInfo::UNKNOWN_EVAL:
              break;
            case PredicateInfo::EVALUATE_ON_RESULT:
              predString += " (evaluated on result)";
              break;
            case PredicateInfo::EVALUATE_IN_UDF:
              predString += " (evaluated by the UDF)";
              break;
            case PredicateInfo::EVALUATE_IN_CHILD:
              predString += " (evaluated in the child)";
              break;
            default:
              predString += " -- invalid evaluation code!";
              break;
            }
          printf("    %s\n", predString.c_str());
        }
    }
}

int UDRInvocationInfo::serializedLength()
{
  // Format: base class + name + sqlAccessType + sqlTransactionType_ +
  // sqlRights + isolationType + debugFlags + type + callPhase +
  // numTableInputs + n*TableInfo + TableInfo(outputTableInfo_) +
  // formal params + actual params + num preds + preds
  int result = TMUDRSerializableObject::serializedLength() +
    serializedLengthOfString(name_) +
    serializedLengthOfString(currentUser_) +
    serializedLengthOfString(sessionUser_) +
    serializedLengthOfString(currentRole_) +
    serializedLengthOfString(queryId_) +
    9*serializedLengthOfInt();

  int i;

  for (i=0; i<numTableInputs_; i++)
    result += inputTableInfo_[i].serializedLength();

  result += outputTableInfo_.serializedLength();
  result += formalParameterInfo_.serializedLength();
  result += actualParameterInfo_.serializedLength();

  for (std::vector<PredicateInfo *>::iterator it = predicates_.begin();
       it != predicates_.end();
       it++)
    {
      result += (*it)->serializedLength();
    }

  return result;
}

// more convenient methods for external callers,
// without side-effecting parameters
void UDRInvocationInfo::serializeObj(Bytes outputBuffer,
                                     int outputBufferLength)
{
  Bytes tempBuf = outputBuffer;
  int tempLen   = outputBufferLength;

  serialize(tempBuf, tempLen);
}

void UDRInvocationInfo::deserializeObj(ConstBytes inputBuffer,
                                       int inputBufferLength)
{
  ConstBytes tempBuf = inputBuffer;
  int tempLen   = inputBufferLength;

  deserialize(tempBuf, tempLen);
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

  result += serializeInt(static_cast<int>(sqlAccessType_),
                         outputBuffer,
                         outputBufferLength);

  result += serializeInt(static_cast<int>(sqlTransactionType_),
                         outputBuffer,
                         outputBufferLength);

  result += serializeInt(static_cast<int>(sqlRights_),
                         outputBuffer,
                         outputBufferLength);

  result += serializeInt(static_cast<int>(isolationType_),
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

  result += serializeString(currentUser_,
                            outputBuffer,
                            outputBufferLength);

  result += serializeString(sessionUser_,
                            outputBuffer,
                            outputBufferLength);

  result += serializeString(currentRole_,
                            outputBuffer,
                            outputBufferLength);

  result += serializeString(queryId_,
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

  result += serializeInt(predicates_.size(),
                         outputBuffer,
                         outputBufferLength);

  for (std::vector<PredicateInfo *>::iterator it = predicates_.begin();
       it != predicates_.end();
       it++)
    {
      result += (*it)->serialize(outputBuffer,
                                 outputBufferLength);
    }

  validateSerializedLength(result);

  return result;
}

int UDRInvocationInfo::deserialize(ConstBytes &inputBuffer,
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

  result += deserializeInt(tempInt,
                           inputBuffer,
                           inputBufferLength);
  sqlAccessType_ = static_cast<SQLAccessType>(tempInt);

  result += deserializeInt(tempInt,
                           inputBuffer,
                           inputBufferLength);
  sqlTransactionType_ = static_cast<SQLTransactionType>(tempInt);

  result += deserializeInt(tempInt,
                           inputBuffer,
                           inputBufferLength);
  sqlRights_ = static_cast<SQLRightsType>(tempInt);

  result += deserializeInt(tempInt,
                           inputBuffer,
                           inputBufferLength);
  isolationType_ = static_cast<IsolationType>(tempInt);

  result += deserializeInt(debugFlags_,
                           inputBuffer,
                           inputBufferLength);

  result += deserializeInt(tempInt,
                           inputBuffer,
                           inputBufferLength);
  funcType_ = static_cast<FuncType>(tempInt);

  result += deserializeInt(tempInt,
                           inputBuffer,
                           inputBufferLength);
  callPhase_ = static_cast<CallPhase>(tempInt);

  result += deserializeString(currentUser_,
                              inputBuffer,
                              inputBufferLength);

  result += deserializeString(sessionUser_,
                              inputBuffer,
                              inputBufferLength);

  result += deserializeString(currentRole_,
                              inputBuffer,
                              inputBufferLength);

  result += deserializeString(queryId_,
                              inputBuffer,
                              inputBufferLength);

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

  // delete all predicates
  for (std::vector<PredicateInfo *>::iterator p = predicates_.begin();
       p != predicates_.end();
       p++)
    delete *p;
  predicates_.clear();

  result += deserializeInt(tempInt,
                           inputBuffer,
                           inputBufferLength);

  for (int p=0; p<tempInt; p++)
    {
      switch (getNextObjectType(inputBuffer,inputBufferLength))
        {
        case COMP_PREDICATE_INFO_OBJ:
          {
            ComparisonPredicateInfo *p = new ComparisonPredicateInfo;

            result += p->deserialize(inputBuffer,
                                     inputBufferLength);

            predicates_.push_back(p);
          }
          break;

        default:
          throw UDRException(
               38900,
               "Found invalid predicate object of type %d",
               static_cast<int>(getNextObjectType(inputBuffer,inputBufferLength)));
        }
    }

  // The UDR writer compile time data stays in place and is not affected
  // by deserialization.

  // totalNumInstances_ and myInstanceNum_ are currently not serialized,
  // since they are set only at runtime.

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

const char *UDRInvocationInfo::callPhaseToString(CallPhase c)
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
      return "describeDataflowAndPredicates()";
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
    case RUNTIME_WORK_CALL:
      return "runtime work call";
      break;
    default:
      return "invalid call phase!";
      break;
    }
}

void UDRInvocationInfo::setQueryId(const char *qid)
{
  queryId_ = qid;
}

void UDRInvocationInfo::setTotalNumInstances(int i)
{
  totalNumInstances_ = i;
}

void UDRInvocationInfo::setMyInstanceNum(int i)
{
  myInstanceNum_ = i;
}


// ------------------------------------------------------------------------
// Member functions for class UDRPlanInfo
// ------------------------------------------------------------------------

UDRPlanInfo::UDRPlanInfo(UDRInvocationInfo *invocationInfo, int planNum) :
     TMUDRSerializableObject(UDR_PLAN_INFO_OBJ,
                             getCurrentVersion()),
     invocationInfo_(invocationInfo),
     planNum_(planNum),
     costPerRow_(-1),
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

/**
 *  Get a unique id for a given plan within a UDR invocation.
 *
 *  @return Plan number for this object, relative to the invocation.
 */
int UDRPlanInfo::getPlanNum() const
{
  return planNum_;
}

/**
 *  Get the cost of the UDR per row, approximately in nanoseconds.
 *
 *  @see setCostPerRow()
 *  @return Cost of the UDR per row, in nanoseconds, for optimization purposes.
 */
long UDRPlanInfo::getCostPerRow() const
{
  return costPerRow_;
}

/**
 *  Return the desired degree of parallelism for this plan.
 *
 *  @see setDesiredDegreeOfParallelism()
 *  @return Degree of parallelism to be used for this plan alternative
 *          (positive) or one of the enum values in
 *          UDRPlanInfo::SpecialDegreeOfParallelism (zero or negative).
 */
int UDRPlanInfo::getDesiredDegreeOfParallelism() const
{
  return degreeOfParallelism_;
}

/**
 *  Set the desired degree of parallelism.
 *
 *  Only use this method from within the
 *  UDR::describeDesiredDegreeOfParallelism() method.
 *
 *  Here are some special values that can be set, in
 *  addition to positive numbers. These are defined in
 *  class UDRPlanInfo.
 *
 *  @li @c ANY_DEGREE_OF_PARALLELISM:
 *        This will allow the optimizer to choose any degree
 *        of parallelism, including 1 (serial execution)
 *  @li @c DEFAULT_DEGREE_OF_PARALLELISM:
 *        Currently the same as ANY_DEGREE_OF_PARALLELISM.
 *        The optimizer will use a heuristic based on
 *        the estimated cardinality (which you can set in
 *        the UDR::describeStatistics() interface).
 *  @li @c MAX_DEGREE_OF_PARALLELISM:
 *        Choose the highest possible degree of parallelism.
 *  @li @c ONE_INSTANCE_PER_NODE:
 *        Start one parallel instance on every Trafodion node.
 *        This is mostly meant for internal TMUDFs, e.g. a
 *        TMUDF to read the log files on every node.
 *
 *  @see getDesiredDegreeOfParallelism()
 *  @param dop desired degree of parallelism (a positive number or
 *             one of the enum values in
 *             UDRPlanInfo::SpecialDegreeOfParallelism).
 *  @throws UDRException
 */
void UDRPlanInfo::setDesiredDegreeOfParallelism(int dop)
{
  invocationInfo_->validateCallPhase(UDRInvocationInfo::COMPILER_DOP_CALL,
                                     UDRInvocationInfo::COMPILER_DOP_CALL,
                                     "UDRPlanInfo::setDesiredDegreeOfParallelism()");

  degreeOfParallelism_ = dop;
}

/**
 *  Set the cost of the UDR per row, approximately in nanoseconds.
 *
 *  Specifying a cost can help with query plan issues. Note that the
 *  operator cost ("EST_OPER_COST") in EXPLAIN is not directly related
 *  to the nanosecond value specified here:
 *  <ul>
 *  <li>For parallel plans (those under an ESP_EXCHANGE), the cost
 *      is calculated for one parallel instance only.
 *  <li>The cost in nanoseconds is converted to internal units
 *      (see CQD NCM_UDR_NANOSEC_FACTOR).
 *  <li>The EXPLAIN cost contains additional factors, accounting
 *      for the cost to send input data to the process that executes
 *      the UDR and for sending back the result.
 *  </ul>
 *
 *  The default implementation estimates the cost to be approximately
 *  100 * sqrt(out().getRecordLength()). Therefore, a value of
 *  1000 might be a good starting point for a cost per row estimate,
 *  assuming an output row length of about 1 KB. Increase this for
 *  more complex UDFs or for wider result rows, decrease it for
 *  simpler UDFs or shorter result rows.
 *
 *  Only use this method from within the
 *  UDR::describeDesiredDegreeOfParallelism() method.
 *
 *  @see UDR::describeDesiredDegreeOfParallelism()
 *  @see getCostPerRow()
 *  @see UDR::TupleInfo::getRecordLength()
 *  @param nanoseconds Cost of the UDR per row, in nanoseconds, for
 *                     optimization purposes.
 */
void UDRPlanInfo::setCostPerRow(long nanoseconds)
{
  invocationInfo_->validateCallPhase(UDRInvocationInfo::COMPILER_DOP_CALL,
                                     UDRInvocationInfo::COMPILER_PLAN_CALL,
                                     "UDRPlanInfo::setCostPerRow()");

  costPerRow_ = nanoseconds;
}

/**
 *  Get data to persist between calls of the optimizer interface
 *
 *  @see setUDRWriterCompileTimeData()
 *  @return UDR writer-specific data that was previously attached or NULL.
 *  @throws UDRException
 */
UDRWriterCompileTimeData *UDRPlanInfo::getUDRWriterCompileTimeData()
{
  invocationInfo_->validateCallPhase(UDRInvocationInfo::COMPILER_DATAFLOW_CALL,
                                     UDRInvocationInfo::COMPILER_COMPLETION_CALL,
                                     "UDRPlanInfo::getUDRWriterCompileTimeData()");

  return udrWriterCompileTimeData_;
}

/**
 *  Set data to persist between calls of the optimizer interface
 *
 *  This call can be used to attach an object derived from class
 *  UDRWriterCompileTimeData to the UDRPlanInfo object. Once
 *  attached, the data will be carried between the stages of the
 *  optimizer interface and can be used to keep state. Note that
 *  this data will be deleted at the end of the optimizer phase and
 *  will not persist until runtime.
 *
 *  Use this method to keep data that is specific to a query plan
 *  alternative, represented by the UDRPlanInfo object. Use
 *  UDRInvocationInfo::setUDRWriterCompileTimeData() to keep data
 *  that is common for the entire UDR invocation.
 *
 *  @see UDRInvocationInfo::setUDRWriterCompileTimeData()
 *  @see getUDRWriterCompileTimeData()
 *  @param compileTimeData UDR writer-defined compile-time data to attach.
 *  @throws UDRException
 */
void UDRPlanInfo::setUDRWriterCompileTimeData(
     UDRWriterCompileTimeData *compileTimeData)
{
  invocationInfo_->validateCallPhase(UDRInvocationInfo::COMPILER_DATAFLOW_CALL,
                                     UDRInvocationInfo::COMPILER_COMPLETION_CALL,
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

/**
 *  Attach a byte array to the plan to be sent to the runtime instances.
 *
 *  Compile time and runtime interfaces of the UDR can be called from
 *  different processes, since UDRs can be executed in parallel and on
 *  different nodes. If the UDR writer would like to carry state from
 *  the compiler interface calls to runtime calls, the best way to achieve
 *  this to attach it using this call and to retrieve the state at runtime
 *  using the getPlanData() call.
 *
 *  The best place to use this method is from within
 *  UDR::completeDescription() method, since this method is
 *  called on the optimal plan that will be used at runtime. It can
 *  also be called from other methods, and the plan data will be
 *  discarded if the plan is not chosen.
 *
 *  @see getPlanData()
 *
 *  @param planData A byte array, content defined by the UDR writer, to be
 *         sent to all runtime instances executing the UDR. The buffer
 *         can and should be deleted by the caller after calling this method.
 *  @param planDataLength Length, in bytes, of the planData.
 *  @throws UDRException
 */
void UDRPlanInfo::addPlanData(const char *planData,
                              int planDataLength)
{
  invocationInfo_->validateCallPhase(UDRInvocationInfo::COMPILER_DOP_CALL,
                                     UDRInvocationInfo::COMPILER_COMPLETION_CALL,
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

/**
 *  Retrieve plan data attached to the UDR invocation and plan.
 *
 *  This method can be called at runtime to get state generated at compile time.
 *
 *  @see setPlanData()
 *
 *  @param planDataLength (out) Length of returned plan data.
 *  @return Pointer to a byte array with plan data generated by the UDR writer
 *          at compile time.
 */
const char *UDRPlanInfo::getPlanData(int &planDataLength)
{
  planDataLength = planDataLength_;
  return planData_;
}

/**
 *  Print the object, for use in debugging.
 *
 *  @see UDRInvocationInfo::PRINT_INVOCATION_INFO_AT_RUN_TIME
 */
void UDRPlanInfo::print()
{
  printf("\nUDRPlanInfo\n-----------------------\n");
  printf("Plan number                : %d\n",  planNum_);
  printf("Cost per row               : %ld\n", costPerRow_);
  printf("Degree of parallelism      : %d\n",  degreeOfParallelism_);
  if (udrWriterCompileTimeData_)
    {
      printf("UDR Writer comp. time data : ");
      udrWriterCompileTimeData_->print();
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

// more convenient methods for external callers,
// without side-effecting parameters
void UDRPlanInfo::serializeObj(Bytes outputBuffer,
                               int outputBufferLength)
{
  Bytes tempBuf = outputBuffer;
  int tempLen   = outputBufferLength;

  serialize(tempBuf, tempLen);
}

void UDRPlanInfo::deserializeObj(ConstBytes inputBuffer,
                                 int inputBufferLength)
{
  ConstBytes tempBuf = inputBuffer;
  int tempLen   = inputBufferLength;

  deserialize(tempBuf, tempLen);
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

int UDRPlanInfo::deserialize(ConstBytes &inputBuffer,
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
// Member functions for class UDR
// ------------------------------------------------------------------------

/**
 *  Default constructor.
 *
 *  Use this in the constructor of a derived class.
 */
UDR::UDR() :
     getNextRowPtr_(NULL),
     emitRowPtr_(NULL)
{}

/**
 *  Virtual Destructor.
 *
 *  Override this destructor and deallocate any resources of a derived
 *  class, if necessary. Note that a UDR object may be used
 *  for several UDR invocations, sometimes at the same time, in one
 *  or more queries. Therefore, this class is for storing resources that
 *  can be shared among multiple invocations. Note also that compile time
 *  and run time may happen in different processes, so it is not possible
 *  to carry state from compile time to run time calls for invocations
 *  with this class. See below for how to carry invocation-related information
 *  between the different phases.
 *
 *  @see UDRInvocationInfo::setUDRWriterCompileTimeData()
 *  @see UDRPlanInfo::setUDRWriterCompileTimeData()
 *  @see UDRPlanInfo::addPlanData()
 *  @throws UDRException
 */
UDR::~UDR() {}

/**
 *  First method of the compiler interface (optional).
 *
 *  Describe the output columns of a TMUDF, based on a description of
 *  its parameters (including parameter values that are specified as a
 *  constant) and the description of the table-valued input columns.

 *  When the compiler calls this, it will have set up the formal and
 *  actual parameter descriptions as well as an output column
 *  description containing all the output parameters defined in the
 *  CREATE FUNCTION DDL (if any).
 *
 *  This method should do a general check of things it expects
 *  that can be validated at this time. Things to check:
 *  @li Number, types and values of actual parameters.
 *  @li Number of table-valued inputs and columns of these inputs.
 *  @li PARTITION BY and ORDER BY clause specified for input tables.
 *  @li Other things like user ids, etc.
 *
 *  Setting the function type with the UDRInvocationInfo::setFuncType()
 *  method will help the compiler generate more efficient code,
 *
 *  The method should then generate a description of the table-valued
 *  output columns, if applicable and if the columns provided at DDL
 *  time are not sufficient. The "See also" section points to methods
 *  to set these values.
 *
 *  Columns of the table-valued output can be declard as "pass-thru"
 *  columns to make many optimizations simpler.
 *
 *  This method must also add to or alter the formal parameter list
 *  to match the list of actual parameters.
 *
 *  The default implementation does nothing.
 *
 *  @see UDRInvocationInfo::par()
 *  @see UDRInvocationInfo::getNumTableInputs()
 *  @see UDRInvocationInfo::in()
 *  @see UDRInvocationInfo::setFuncType()
 *  @see UDRInvocationInfo::addFormalParameter()
 *  @see UDRInvocationInfo::addPassThruColumns()
 *  @see TupleInfo::addColumn()
 *  @see TupleInfo::addIntegerColumn()
 *  @see TupleInfo::addLongColumn()
 *  @see TupleInfo::addCharColumn()
 *  @see TupleInfo::addVarCharColumn()
 *  @see TupleInfo::addColumns()
 *  @see TupleInfo::addColumnAt()
 *  @see TupleInfo::deleteColumn(int)
 *  @see TupleInfo::deleteColumn(const std::string &)
 *
 *  @param info A description of the UDR invocation.
 *  @throws UDRException
 */
void UDR::describeParamsAndColumns(UDRInvocationInfo &info)
{
}

/**
 *  Second method of the compiler interface (optional).
 *
 *  Eliminate unneeded columns and decide where to execute predicates.
 *
 *  This is the second call in the compiler interface, after
 *  describeParamsAndColumns(). When the compiler calls this, it will
 *  have marked the UDF result columns with a usage code, indicating
 *  any output columns that are not required for this particular query.
 *  It will also have created a list of predicates that need to be
 *  evaluated.
 *
 *  This method should do three things:
 *  @li Mark columns of the table-valued inputs as not used, based on
 *      the result column usage and internal needs of the UDF. Such
 *      input columns will later be eliminated.
 *  @li Mark output columns that are not used and that can be
 *      easily suppressed by the UDF as NOT_PRODUCED. Such columns
 *      will be eliminated as well.
 *  @li Decide where to evaluate each predicate, a) on the UDF result
 *      (default), b) inside the UDF by code written by the UDF writer,
 *      or c) in the table-valued inputs.
 *
 *  The default implementation does not mark any of the table-valued input
 *  columns as NOT_USED. It also does not mark any output columns as
 *  NOT_PRODUCED. Predicate handling in the default implementation
 *  depends on the function type:
 *  @li UDRInvocationInfo::GENERIC:
 *               No predicates are pushed down, because the compiler
 *               does not know whether any of the eliminated rows might
 *               have altered the output of the UDF. One example is the
 *               "sessionize" UDF, where eliminated rows can lead to
 *               differences in session ids.
 *  @li UDRInvocationInfo::MAPPER:
 *               All predicates on pass-thru columns are pushed down to
 *               table-valued inputs. Since the UDF carries no state between
 *               the input rows it sees, eliminating any input rows will
 *               not alter any results for other rows.
 *  @li UDRInvocationInfo::REDUCER:
 *               Only predicates on the PARTITION BY columns will be
 *               pushed to table-valued inputs. These predicates may
 *               eliminate entire groups of rows (partitions), and since
 *               no state is carried between such groups that is valid.
 *  @li UDRInvocationInfo::REDUCER_NC:
 *               Same as REDUCER.
 *
 *  NOTE: When eliminating columns from the table-valued inputs or
 *        the table-valued result, column numbers may change in the
 *        next call, as these columns are actually removed from the
 *        lists. If the UDF carries state between calls and if that
 *        state refers to column numbers, they will need to be
 *        updated. This is best done in this describeDataflowAndPredicates()
 *        call.
 *
 *  @see ColumnInfo::getUsage()
 *  @see ColumnInfo::setUsage() (to mark output columns as NOT_PRODUCED)
 *  @see UDRInvocationInfo::setFuncType()
 *  @see UDRInvocationInfo::setChildColumnUsage() (to mark unused input columns)
 *  @see UDRInvocationInfo::setUnusedPassthruColumns()
 *  @see UDRInvocationInfo::pushPredicatesOnPassthruColumns()
 *  @see UDRInvocationInfo::setPredicateEvaluationCode()
 *
 *  @param info A description of the UDR invocation.
 *  @throws UDRException
 */
void UDR::describeDataflowAndPredicates(UDRInvocationInfo &info)
{
  switch (info.getFuncType())
    {
    case UDRInvocationInfo::GENERIC:
      break;

    case UDRInvocationInfo::MAPPER:
      // push as many predicates as possible to the children
      info.pushPredicatesOnPassthruColumns();
      break;

    case UDRInvocationInfo::REDUCER:
    case UDRInvocationInfo::REDUCER_NC:
      {
        int partitionedChild = -1;

        // find a child that uses a PARTITION BY
        for (int c=0; c<info.getNumTableInputs(); c++)
          if (info.in(c).getQueryPartitioning().getType() ==
              PartitionInfo::PARTITION)
            {
              partitionedChild = c;
              break;
            }

        if (partitionedChild >= 0)
          {
            const PartitionInfo &partInfo =
              info.in(partitionedChild).getQueryPartitioning();
            int numPredicates = info.getNumPredicates();

            // walk through all comparison predicates
            for (int p=0; p<numPredicates; p++)
              if (info.isAComparisonPredicate(p))
                {
                  // a predicate on column "predCol"
                  int predCol = 
                    info.getComparisonPredicate(p).getColumnNumber();
                  const ColumnInfo &colInfo = info.out().getColumn(predCol);
                  const ProvenanceInfo &prov = colInfo.getProvenance();

                  // find the corresponding child table and child column #
                  if (prov.getInputTableNum() == partitionedChild)
                    {
                      int inputColNum = prov.getInputColumnNum();

                      // check whether inputColNum appears in the PARTITION BY clause
                      for (int pbColIx=0; pbColIx<partInfo.getNumEntries(); pbColIx++)
                        if (partInfo.getColumnNum(pbColIx) == inputColNum)
                          {
                            // yes, this is a predicate on a partitioning column,
                            // push it down if possible
                            info.pushPredicatesOnPassthruColumns(p, p);
                            break;
                          }
                    } // column is from the partitioned input table
                } // is a comparison predicate
          } // found a partitioned child table
      } // REDUCER(_NC)
      break;

    default:
      throw UDRException(
           38900,
           "Invalid UDR Function type: %d",
           static_cast<int>(info.getFuncType()));
    }
}

/**
 *  Third method of the compiler interface (optional).
 *
 *  Set up logical constraints on the UDF result table.
 *
 *  When the compiler calls this method, it will have synthesized
 *  constraints on the table-valued inputs, if any. The UDR writer
 *  can now indicate constraints on the table-valued result.
 *
 *  The default implementation does nothing.
 *
 *  @see TableInfo::getNumConstraints()
 *  @see TableInfo::getConstraint()
 *  @see TableInfo::addCardinalityConstraint()
 *  @see TableInfo::addUniquenessConstraint()
 *  @see UDRInvocationInfo::propagateConstraintsFor1To1UDFs()
 *  @param info A description of the UDR invocation.
 *  @throws UDRException
 */
void UDR::describeConstraints(UDRInvocationInfo &info)
{
}

/**
 *  Fourth method of the compiler interface (optional).
 *
 *  Set up statistics for the table-valued result.
 *
 *  When the optimizer calls this method, it will have synthesized
 *  some statistics for the table-valued inputs, if any. The UDR
 *  writer can now indicate the estimated row count for the table-valued
 *  result and estimated number of unique values for the output columns.
 *
 *  The default implementation does nothing. If no estimated cardinality
 *  is set for the output table and no estimated number of unique values
 *  is set for output columns, the optimizer will make default assumptions.
 *  Here are some of these default assumptions:
 *  <ul>
 *  <li>UDRs of type UDRInvocationInfo::MAPPER return one output row for
 *      each row in their largest input table.
 *  <li>UDRs of type UDRInvocationInfo::REDUCER and REDUCER_NC return one
 *      output row for every partition in their largest partitioned input
 *      table.
 *  <li>For output columns that are passthru columns, the estimated
 *      unique entries are the same as for the underlying column in the
 *      table-valued input.
 *  <li>Other default cardinality and unique entry counts can be influenced
 *      with defaults (CONTROL QUERY DEFAULT) in Trafodion SQL.
 *  </ul>
 *
 *  @see UDRInvocationInfo::setFuncType()
 *  @see ColumnInfo::getEstimatedUniqueEntries()
 *  @see ColumnInfo::setEstimatedUniqueEntries()
 *  @see TableInfo::getEstimatedNumRows()
 *  @see TableInfo::setEstimatedNumRows()
 *  @see TableInfo::getEstimatedNumPartitions()
 *
 *  @param info A description of the UDR invocation.
 *  @throws UDRException
 */
void UDR::describeStatistics(UDRInvocationInfo &info)
{
  // do nothing
}

/**
 *  Fifth method of the compiler interface (optional).
 *
 *  Describe the desired parallelism of a UDR.
 *
 *  This method can be used to specify a desired degree of
 *  parallelism, either in absolute or relative terms.
 *
 *  The default behavior is to allow any degree of parallelism for
 *  TMUDFs of function type UDRInvocationInfo::MAPPER or
 *  UDRInvocationInfo::REDUCER (or REDUCER_NC) that have exactly
 *  one table-valued input. The default behavior forces serial
 *  execution in all other cases. The reason is that for a single
 *  table-valued input, there is a natural way to parallelize the
 *  function by parallelizing its input a la MapReduce. In all
 *  other cases, parallel execution requires active participation
 *  by the UDF, which is why the UDF needs to signal explicitly
 *  that it can handle such flavors of parallelism.
 *
 *  Default implementation:
 *  @code
 *  if (info.getNumTableInputs() == 1 &&
 *      (info.getFuncType() == UDRInvocationInfo::MAPPER ||
 *       info.getFuncType() == UDRInvocationInfo::REDUCER ||
 *       info.getFuncType() == UDRInvocationInfo::REDUCER_NC))
 *    plan.setDesiredDegreeOfParallelism(UDRPlanInfo::ANY_DEGREE_OF_PARALLELISM);
 *  else
 *    plan.setDesiredDegreeOfParallelism(1); // serial execution
 *  @endcode
 *
 *  Note that this is NOT foolproof, and that the TMUDF might still
 *  need to validate the PARTITION BY and ORDER BY syntax used in its
 *  invocation.
 *
 *  Note also that in order to get parallel execution, you may need to
 *  implement the UDR::describeStatistics() interface and provide a
 *  cardinality estimate. Alternatively, you can set the
 *  PARALLEL_NUM_ESPS CQD.
 *
 *  @see UDRPlanInfo::setDesiredDegreeOfParallelism()
 *  @see UDRInvocationInfo::setFuncType()
 *
 *  @param info A description of the UDR invocation.
 *  @param plan Plan-related description of the UDR invocation.
 *  @throws UDRException
 */
void UDR::describeDesiredDegreeOfParallelism(UDRInvocationInfo &info,
                                                      UDRPlanInfo &plan)
{
  if (info.getNumTableInputs() == 1 &&
      (info.getFuncType() == UDRInvocationInfo::MAPPER ||
       info.getFuncType() == UDRInvocationInfo::REDUCER ||
       info.getFuncType() == UDRInvocationInfo::REDUCER_NC))
    plan.setDesiredDegreeOfParallelism(UDRPlanInfo::ANY_DEGREE_OF_PARALLELISM);
  else
    plan.setDesiredDegreeOfParallelism(1); // serial execution
}

/**
 *  Sixth method of the compiler interface (optional).
 *
 *  The query optimizer calls this method once for every plan alternative
 *  considered for a UDR invocation. It provides the required partitioning
 *  and ordering of the result. The UDR writer can decide whether these
 *  requirements are acceptable to the UDR and whether any partitioning
 *  or ordering of the table-valued inputs is required to produce the required
 *  result properties.
 *
 *  This interface is currently not used.
 *  
 *  @param info A description of the UDR invocation.
 *  @param plan Plan-related description of the UDR invocation.
 *  @throws UDRException
 */
void UDR::describePlanProperties(UDRInvocationInfo &info,
                                          UDRPlanInfo &plan)
{
  // TBD
}

/**
 *  Seventh and final method of the compiler interface for TMUDFs (optional).
 *
 *  This final compile time call gives the UDF writer the opportunity
 *  to examine the chosen query plan, to pass information on to the
 *  runtime method, using UDRPlanInfo::addPlanData(), and to clean up
 *  any resources related to the compile phase of a particular TMUDF
 *  invocation.
 *
 *  The default implementation does nothing.
 *
 *  @see UDRPlanInfo::addPlanData()
 *  @see UDRPlanInfo::getUDRWriterCompileTimeData()
 *  @see UDRInvocationInfo::getUDRWriterCompileTimeData()
 *
 *  @param info A description of the UDR invocation.
 *  @param plan Plan-related description of the UDR invocation.
 *  @throws UDRException
 */
void UDR::completeDescription(UDRInvocationInfo &info,
                                       UDRPlanInfo &plan)
{
}

/**
 *  Runtime code for UDRs (required).
 *
 *  This is the only method that is mandatory in the implementation
 *  of a UDR (in addition to the factory method).
 *
 *  This method needs to set the output column values and emit
 *  rows by calling the emitRows() method. It can read rows from
 *  table-valued inputs, using the getNextRow() method.
 *
 *  @see TupleInfo::setInt()
 *  @see TupleInfo::setString()
 *  @see emitRow()
 *  @see getNextRow()
 *  @see TupleInfo::getInt()
 *  @see TupleInfo::getString()
 *  @see UDRInvocationInfo::copyPassThruData()
 *
 *  @param info A description of the UDR invocation.
 *  @param plan Plan-related description of the UDR invocation.
 *  @throws UDRException
 */
void UDR::processData(UDRInvocationInfo &info,
                               UDRPlanInfo &plan)
{
  throw UDRException(38900,"UDR::processData() must be overridden by the UDF");
}

/**
 *  Debugging hook for UDRs.
 *
 *  This method is called in debug Trafodion builds when certain
 *  flags are set in the UDR_DEBUG_FLAGS CQD (CONTROL QUERY DEFAULT).
 *  See https://cwiki.apache.org/confluence/display/TRAFODION/Tutorial%3A+The+object-oriented+UDF+interface#Tutorial:Theobject-orientedUDFinterface-DebuggingUDFcode
 *  for details.
 *
 *  The default implementation prints out the process id and then
 *  goes into an endless loop. The UDF writer can then attach a
 *  debugger, set breakpoints and force the execution out of the loop.
 *
 *  Note that the printout of the pid may not always be displayed on
 *  a terminal, for example if the process is executing on a different node.
 */
void UDR::debugLoop()
{
  int debugLoop = 1;
  int myPid = static_cast<int>(getpid());

  printf("Process %d entered a loop to be able to debug it\n", myPid);

  // go into a loop to allow the user to attach a debugger,
  // if requested, set debugLoop = 2 in the debugger to get out
  while (debugLoop < 2)
    debugLoop = 1-debugLoop;
}

/**
 *  Read a row of a table-value input.
 *
 *  This method can only be called from within processData().
 *
 *  @param info A description of the UDR invocation.
 *  @param tableIndex Indicator for which table-valued input to read data.
 *  @return true if another row could be read, false if it reached end of data.
 *  @throws UDRException
 */
bool UDR::getNextRow(UDRInvocationInfo &info, int tableIndex)
{
  SQLUDR_Q_STATE qstate = SQLUDR_Q_MORE;

  (*getNextRowPtr_)(info.in(tableIndex).getRowPtr(),
                    tableIndex,
                    &qstate);

  if (info.getDebugFlags() & UDRInvocationInfo::TRACE_ROWS)
    switch (qstate)
      {
      case SQLUDR_Q_MORE:
        {
          std::string row;

          info.in(tableIndex).getDelimitedRow(row,'|',true);
          // replace any control characters with escape sequences
          for (int c=row.size()-1; c>=0; c--)
            if (row[c] < 32)
              {
                char buf[5];

                // print \x0a for an ASCII line feed (decimal 10)
                snprintf(buf, sizeof(buf), "\\x%02hhx", row[c]);
                row.replace(c, 1, buf);
              }
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

  return (qstate == SQLUDR_Q_MORE);
}

/**
 *  Emit a row of the table-valued result.
 *
 *  This method can only be called from within processData().
 *
 *  @param info A description of the UDR invocation.
 *  @throws UDRException
 */
void UDR::emitRow(UDRInvocationInfo &info)
{
  SQLUDR_Q_STATE qstate = SQLUDR_Q_MORE;

  if (info.getDebugFlags() & UDRInvocationInfo::TRACE_ROWS)
    {
      std::string row;

      info.out().getDelimitedRow(row,'|',true);
      // replace any control characters with escape sequences
      for (int c=row.size()-1; c>=0; c--)
        if (row[c] < 32)
          {
            char buf[5];

            // print \x0a for an ASCII line feed (decimal 10)
            snprintf(buf, sizeof(buf), "\\x%02hhx", row[c]);
            row.replace(c, 1, buf);
          }
      printf("(%d) Emitting row: %s\n",
             info.getMyInstanceNum(), row.c_str());
    }

  (*emitRowPtr_)(info.out().getRowPtr(),
                 0,
                 &qstate);
}

/**
 *  For versioning, return features supported by the UDR writer.
 *
 *  This method can be used in the future to facilitate changes in
 *  the UDR interface. UDR writers will be able to indicte through this
 *  method whether they support new features.
 *
 *  The default implementation returns 0 (no extra features are supported).
 *
 *  @return A yet to be determined set of bit flags or codes for
 *          supported features.
 */
int UDR::getFeaturesSupportedByUDF()
{
  return 0;
}
