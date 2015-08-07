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
#ifndef LMJAVATYPE_H
#define LMJAVATYPE_H
/* -*-C++-*-
******************************************************************************
*
* File:         LmJavaType.h
* Description:  Java Type Encapsulation
*
* Created:      08/22/2003
* Language:     C++
*
*
******************************************************************************
*/

#include "LmAssert.h"
#include "ComSmallDefs.h"

//////////////////////////////////////////////////////////////////////
//
// Forward class references
//
//////////////////////////////////////////////////////////////////////
class LmParameter;

//////////////////////////////////////////////////////////////////////
//
// LmJavaType
//
// This class is common to Language Manager classes. It is used by
// these classes as an aid in mapping SQL/MX types (defined in
// ComFSDataType in ComSmallDefs.h) to their Java peers in the
// context of the LM for Java. If new types are added to this class,
// they should be reflected in isJavaTypeObject() method.
//
// The Type enum is based upon the following properties of ComFSDataType:
// (1) COM_UNKNOWN_FSDT is the first value of the enum.
// (2) COM_LAST_FSDT is the last value of the enum.
// (3) COM_DATETIME_FSDT through COM_DATETIME_FSDT + 2 are implicitly
//     reserved for data/time types.
//////////////////////////////////////////////////////////////////////
class LmJavaType
{
public:
  enum Type {
    JT_NONE          = COM_UNKNOWN_FSDT -1,
    JT_VOID          = COM_UNKNOWN_FSDT,
    JT_SHORT         = COM_SIGNED_BIN16_FSDT,
    JT_INT           = COM_SIGNED_BIN32_FSDT,
    JT_LONG          = COM_SIGNED_BIN64_FSDT,
    JT_FLOAT         = COM_FLOAT32_FSDT,
    JT_DOUBLE        = COM_FLOAT64_FSDT,
    JT_LANG_STRING   = COM_VCHAR_DBL_FSDT,
    JT_MATH_BIGDEC   = COM_SIGNED_DECIMAL_FSDT,
    JT_SQL_DATE      = COM_DATETIME_FSDT + 0,
    JT_SQL_TIME      = COM_DATETIME_FSDT + 1,
    JT_SQL_TIMESTAMP = COM_DATETIME_FSDT + 2,
    JT_LANG_INTEGER  = JT_INT + COM_LAST_FSDT,
    JT_LANG_LONG     = JT_LONG + COM_LAST_FSDT,
    JT_LANG_FLOAT    = JT_FLOAT + COM_LAST_FSDT,
    JT_LANG_DOUBLE   = JT_DOUBLE + COM_LAST_FSDT,
    JT_SQL_RESULTSET = COM_LAST_FSDT + COM_LAST_FSDT + 1,
    JT_LAST = JT_SQL_RESULTSET
  };

  struct TypeElement
  {
    Type  javaIdx;                // Java type index. This is LM type.
    const char  *javaTypeName;    // Java encoded type.
    Lng32  javaTypeNameLen;       // Length of encoded Java type.
    const char  *javaText;        // Java type in normal text.
    Lng32  javaTextLen;           // Length of javaText.
  };

public:

  LmJavaType(LmParameter *param);
  ~LmJavaType() {}

  // Accessors
  LmJavaType::Type getType() const
  {
    return type_;
  }

  LmParameter *getLmParameter() const
  {
    return lmParam_;
  }

  const char* getJavaTypeName(ComUInt32 &len) const;

  ComBoolean isJavaTypeObject() const;

  // TypeElement search based on index into JavaTypeTable
  TypeElement* getTypeElement() const;

  // TypeElement search based on TypeName
  static TypeElement* getTypeElement(char* javaTypeName);

private:
  Type        type_;            // Type
  LmParameter *lmParam_;        // Corresponding parameter
  Int32         index_;           // index into JavaTypeTable

  static TypeElement javaTypeTable[];

};


#endif
