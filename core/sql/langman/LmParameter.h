#ifndef LMPARAMETER_H
#define LMPARAMETER_H
/* -*-C++-*-
**********************************************************************
*
* File:         LmParameter.h
* Description:  LmParameter class
* Created:      07/01/1999
* Language:     C++
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
**********************************************************************/

#include <sys/types.h>
#include <stdlib.h>
#include "ComSmallDefs.h"
#include "LmCommon.h"
#include "LmError.h"

//////////////////////////////////////////////////////////////////////
//
// LmParameter
//
// The LmParameter is used to communicate routine parameter and return
// type attributes between the clients of the LM and the LM. Each routine
// parameter and/or return type is represented by its own LmParameter
// object. The LmParameter class is similiar to some extent to the Exp
// Attributes class.
//
// The SQL type attribute of an LmParameter must reflect the real SQL
// type of a parameter and not its normalized type as discussed in the
// LM Internal Specification. E.g., clients of the LM pass a DATE
// parameter to the LM normalized to a character string; however, the
// type attribute of the LmParameter must be COM_DATETIME_FSDT and not
// a character type.
//
// The LM assumes the in/out buffer address of an LmParameter are
// properly aligned such that casting/dereferencing of their contents
// is appropriate within the LM.
//
//////////////////////////////////////////////////////////////////////

class SQLLM_LIB_FUNC LmParameter
{
public:

  LmParameter(ComFSDataType fsType,
              ComUInt16 prec = 0,
              ComUInt16 scale = 0)
  {
    paramName_ = NULL;
    init(fsType,
         prec,
         scale,
         CharInfo::UnknownCharSet,
         CharInfo::UNKNOWN_COLLATION,
         COM_INPUT_COLUMN,
         FALSE, // objMap
         RS_NONE,
         0, 0, 0, 0, 0, 0, // input offsets and lengths
         0, 0, 0, 0, 0, 0, // output offsets and lengths
         NULL); // name
  }

  LmParameter(ComFSDataType       fsType,
              ComUInt16           prec,
              ComUInt16           scale,
              CharInfo::CharSet   encodingCharSet,
              CharInfo::Collation collation,
              ComColumnDirection  direction,
              ComBoolean          objMap,
              LmResultSetMode     resultSet,
              ComUInt32           inDataOffset,
              ComUInt32           inSize,
              ComSInt32           inNullIndOffset,
              ComSInt16           inNullIndSize,
              ComSInt32           inVCLenIndOffset,
              ComSInt16           inVCLenIndSize,
              ComUInt32           outDataOffset,
              ComUInt32           outSize,
              ComSInt32           outNullIndOffset,
              ComSInt16           outNullIndSize,
              ComSInt32           outVCLenIndOffset,
              ComSInt16           outVCLenIndSize,
              const char         *paramName)
  {
    paramName_ = NULL;
    init(fsType, prec, scale, encodingCharSet, collation,
         direction, objMap, resultSet,
         inDataOffset, inSize, inNullIndOffset, inNullIndSize,
         inVCLenIndOffset, inVCLenIndSize,
         outDataOffset, outSize, outNullIndOffset, outNullIndSize,
         outVCLenIndOffset, outVCLenIndSize, paramName);
  }

  virtual ~LmParameter()
  {
    freeResources();
  }

  void freeResources()
  {
    if (paramName_)
    {
      free(paramName_);
      paramName_ = NULL;
    }
  }
  
  void init(ComFSDataType       fsType,
            ComUInt16           prec,
            ComUInt16           scale,
            CharInfo::CharSet   encodingCharSet,
            CharInfo::Collation collation,
            ComColumnDirection  direction,
            ComBoolean          objMap,
            LmResultSetMode     resultSet,
            ComUInt32           inDataOffset,
            ComUInt32           inSize,
            ComSInt32           inNullIndOffset,
            ComSInt16           inNullIndSize,
            ComSInt32           inVCLenIndOffset,
            ComSInt16           inVCLenIndSize,
            ComUInt32           outDataOffset,
            ComUInt32           outSize,
            ComSInt32           outNullIndOffset,
            ComSInt16           outNullIndSize,
            ComSInt32           outVCLenIndOffset,
            ComSInt16           outVCLenIndSize,
            const char         *paramName)
  {
    setParamName(paramName);

    fsType_ = fsType;
    prec_ = prec;
    scale_ = scale;
    direction_ = direction;
    encodingCharSet_ = encodingCharSet;
    collation_ = collation;
    objMap_ = objMap;
    resultSet_ = resultSet;
    inDataOffset_ = inDataOffset;
    inSize_ = inSize;
    inNullIndOffset_ = inNullIndOffset;
    inNullIndSize_ = inNullIndSize; 
    inVCLenIndOffset_ = inVCLenIndOffset;
    inVCLenIndSize_ = inVCLenIndSize;
    outDataOffset_ = outDataOffset;
    outSize_ = outSize;
    outNullIndOffset_ = outNullIndOffset;
    outNullIndSize_ = outNullIndSize; 
    outVCLenIndOffset_ = outVCLenIndOffset;
    outVCLenIndSize_ = outVCLenIndSize;
  }

  void setInDataInfo(ComUInt32 offset,
                     ComUInt32 size,
		     ComSInt32 nullIndOffset,
                     ComSInt16 nullIndSize,
                     ComSInt32 vcLenIndOffset,
                     ComSInt16 vcLenIndSize) 
  {
    inDataOffset_ = offset;
    inSize_ = size;
    inNullIndOffset_ = nullIndOffset;
    inNullIndSize_ = nullIndSize; 
    inVCLenIndOffset_ = vcLenIndOffset;
    inVCLenIndSize_ = vcLenIndSize;
  }

  void setOutDataInfo(ComUInt32 offset,
                      ComUInt32 size,
                      ComSInt32 nullIndOffset,
                      ComSInt16 nullIndSize,
                      ComSInt32 vcLenIndOffset,
                      ComSInt16 vcLenIndSize) 
  {
    outDataOffset_ = offset;
    outSize_ = size;
    outNullIndOffset_ = nullIndOffset;
    outNullIndSize_ = nullIndSize; 
    outVCLenIndOffset_ = vcLenIndOffset;
    outVCLenIndSize_ = vcLenIndSize;
  }

  // Accessors.
  ComFSDataType fsType() const          { return fsType_; }
  ComUInt16 prec() const                { return prec_; }
  ComUInt16 scale() const               { return scale_; }
  ComColumnDirection direction() const  { return direction_; }
  ComBoolean objMapping() const         { return objMap_; }

  ComBoolean resultSet() const
  { return (resultSet_ == RS_SET ? TRUE : FALSE); }
  
  CharInfo::CharSet encodingCharSet() const { return encodingCharSet_; }

  CharInfo::Collation collation() const { return collation_; }

  // The date/time code (which indicates whether the type is a DATE,
  // TIME, or TIMESTAMP) is stored in the precision field
  ComUInt16 getDatetimeCode() const { return prec_; }

  // TIME and TIMESTAMP precision is stored in the scale field
  ComUInt16 getTimePrecision() const { return scale_; }

  ComUInt32 inDataOffset() const      { return inDataOffset_; }
  ComUInt32 inSize() const            { return inSize_; }
  ComSInt32 inNullIndOffset() const   { return inNullIndOffset_; }
  ComSInt16 inNullIndSize() const     { return inNullIndSize_; }
  ComSInt32 inVCLenIndOffset() const  { return inVCLenIndOffset_; } 
  ComSInt16 inVCLenIndSize() const    { return inVCLenIndSize_; }

  ComUInt32 outDataOffset() const     { return outDataOffset_; }
  ComUInt32 outSize() const           { return outSize_; }
  ComSInt32 outNullIndOffset() const  { return outNullIndOffset_; }
  ComSInt16 outNullIndSize() const    { return outNullIndSize_; }
  ComSInt32 outVCLenIndOffset() const { return outVCLenIndOffset_; } 
  ComSInt16 outVCLenIndSize() const   { return outVCLenIndSize_; }

  const char *getParamName() const { return paramName_; }

  ComUInt32 actualInDataSize(void* data) const;
  ComUInt32 actualOutDataSize(void* data) const;

  // Mutators
  void setObjMapping(ComBoolean o = TRUE) { objMap_ = o; }

  // Utilities.
  ComBoolean isNumeric() const
  {
    if (prec_ > 0 &&
        (fsType_ == COM_SIGNED_BIN16_FSDT ||
         fsType_ == COM_UNSIGNED_BIN16_FSDT ||
         fsType_ == COM_SIGNED_BIN32_FSDT ||
         fsType_ == COM_UNSIGNED_BIN32_FSDT ||
         fsType_ == COM_SIGNED_BIN64_FSDT ||
         fsType_ == COM_SIGNED_NUM_BIG_FSDT ||
         fsType_ == COM_UNSIGNED_NUM_BIG_FSDT))
      return TRUE;
    return FALSE;
  }
  
  ComBoolean isBigNum() const
  {
    if (fsType_ == COM_SIGNED_NUM_BIG_FSDT ||
        fsType_ == COM_UNSIGNED_NUM_BIG_FSDT)
      return TRUE;
    return FALSE;
  }
  
  ComBoolean isDecimal() const
  {
    if (fsType_ == COM_UNSIGNED_DECIMAL_FSDT ||
        fsType_ == COM_SIGNED_DECIMAL_FSDT)
      return TRUE;
    return FALSE;
  }
  
  ComBoolean isCharacter() const
  {
    if (fsType_ == COM_FCHAR_FSDT ||
        fsType_ == COM_FCHAR_DBL_FSDT ||
        fsType_ == COM_VCHAR_FSDT ||
        fsType_ == COM_VCHAR_DBL_FSDT ||
        fsType_ == COM_VCHAR_LONG_FSDT)
      return TRUE;
    return FALSE;
  }
  
  ComBoolean isDateTime() const
  {
    if (fsType_ == COM_DATETIME_FSDT)
      return TRUE;
    return FALSE;
  }
  
  ComBoolean isTimeOrTimestamp() const
  {
    if (fsType_ == COM_DATETIME_FSDT &&
        (prec_ == REC_DTCODE_TIME || prec_ == REC_DTCODE_TIMESTAMP))
      return TRUE;
    return FALSE;
  }
  
  ComBoolean isInterval() const
  {
    if (DFS2REC::isInterval(fsType_))
      return TRUE;
    return FALSE;
  }
  
  ComBoolean isIn() const 
  {
    if (direction_ == COM_INPUT_COLUMN || direction_ == COM_INOUT_COLUMN)
      return TRUE;
    return FALSE;
  }
  
  ComBoolean isOut() const
  {
    if (direction_ == COM_OUTPUT_COLUMN || direction_ == COM_INOUT_COLUMN)
      return TRUE;
    return FALSE;
  }

  // Methods to set the contents of an output buffer to a normalized
  // SQL type
  LmResult setOutSmallInt(void *, short);
  LmResult setOutInteger(void *, Int32);
  LmResult setOutLargeInt(void *, Int64);
  LmResult setOutReal(void *, float);
  LmResult setOutFloat(void *, double);
  LmResult setOutDouble(void *, double);
  LmResult setOutNumeric(void *, const char *, ComBoolean, CollHeap *,
                         ComDiagsArea *);
  LmResult setOutDecimal(void *, const char*, CollHeap *, ComDiagsArea *);
  LmResult setOutChar(void *, const char*, ComUInt32 len);
  LmResult setOutDate(void *, const char*);
  LmResult setOutTime(void *, const char*);
  LmResult setOutTimestamp(void *, const char*);
  LmResult setOutInterval(void *, const char*, ComUInt32 len);
  
  // Methods to set and retrieve null indicators
  void setNullInput(char *dataRow, ComBoolean isNull) const
  {
    if (inNullIndSize_ > 0)
      setNullValue(dataRow, inNullIndOffset_, inNullIndSize_, isNull);
  }
  void setNullOutput(char *dataRow, ComBoolean isNull) const
  {
    if (outNullIndSize_ > 0)
      setNullValue(dataRow, outNullIndOffset_, outNullIndSize_, isNull);
  }
  ComBoolean isNullInput(char *dataRow) const
  {
    return (inNullIndSize_ > 0 ?
            isNullValue(dataRow, inNullIndOffset_) :
            FALSE);
  }
  ComBoolean isNullOutput(char *dataRow) const
  {
    return (outNullIndSize_ > 0 ?
            isNullValue(dataRow, outNullIndOffset_) :
            FALSE);
  }

private:

  ComUInt32 vcDataSize(char *data, ComSInt32 offset, ComSInt16 size) const;

  // These private methods to get and set null indicators should not
  // be called for NOT NULL parameters.
  ComBoolean isNullValue(char *dataRow, ComUInt32 indOffset) const;
  void setNullValue(char *dataRow,
                    ComUInt32 indOffset, ComUInt32 indSize,
                    ComBoolean isNull) const;
  
  ComFSDataType fsType_;  // FS data type

  ComUInt16 prec_;        // Precision for NUMERIC/DECIMAL
                          // Datetime qualifier for DATE/TIME/TIMESTAMP

  ComUInt16 scale_;       // Scale for NUMERIC/DECIMAL
                          // Precision for TIME/TIMESTAMP

  CharInfo::CharSet encodingCharSet_; // Encoded charset for CHAR/VARCHAR

  CharInfo::Collation collation_;     // Collation for CHAR/VARCHAR

  ComColumnDirection direction_;      // IN, OUT, or INOUT parameter mode

  ComBoolean objMap_;   // Object mapping flag. Used for SQL/JRT extended
                        // Java object types (e.g. java.lang.Integer)

  char resultSet_;      // Specifies the parameter is a dummy result set
                        // parameter to support SQL/JRT syntax inside SPs
                        // (see LmResultSet).

  ComUInt32 inDataOffset_;      // Input data offset for IN/INOUT parameters.
  ComUInt32 inSize_;            // Input buffer size.
  ComSInt32 inNullIndOffset_;   // Null ind offset for IN/INOUT parameters.
  ComSInt16 inNullIndSize_;     // Null ind size for IN/INOUT parameters.
  ComSInt32 inVCLenIndOffset_;  // VC len ind offset for IN/INOUT parameters
  ComSInt16 inVCLenIndSize_;    // VC len ind length for IN/INOUT parameters

  ComUInt32 outDataOffset_;     // Output data offset for OUT/INOUT parameters.
  ComUInt32 outSize_;           // Max Output buffer size.
  ComSInt32 outNullIndOffset_;  // Null ind offset for OUT/INOUT parameters.
  ComSInt16 outNullIndSize_;    // Null ind size for OUT/INOUT parameters.
  ComSInt32 outVCLenIndOffset_; // VC len ind offset for OUT/INOUT parameters
  ComSInt16 outVCLenIndSize_;   // VC len ind length for OUT/INOUT parameters

  char *paramName_;             // Optional parameter name

  void setParamName(const char *name);

  // Do not implement a default constructor
  LmParameter();

}; // class LmParameter

#endif
