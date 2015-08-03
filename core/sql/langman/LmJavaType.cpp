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
* File:         LmJavaType.cpp
* Description:  Java Type Encapsulation
*
* Created:      08/22/2003
* Language:     C++
*
*
******************************************************************************
*/

#include "LmParameter.h"
#include "LmJavaType.h"

#define JAVATYPETABLE_SIZE  16
#define LM_JAVATYPE_INVALID_INDEX -1

LmJavaType::TypeElement LmJavaType::javaTypeTable[] =
{
  {JT_VOID,         "V",                       1, "void",                  4},
  {JT_SHORT,        "S",                       1, "short",                 5},
  {JT_INT,          "I",                       1, "int",                   3},
  {JT_LONG,         "J",                       1, "long",                  4},
  {JT_FLOAT,        "F",                       1, "float",                 5},
  {JT_DOUBLE,       "D",                       1, "double",                6},
  {JT_LANG_STRING,  "Ljava/lang/String;",     18, "java.lang.String",     16},
  {JT_MATH_BIGDEC,  "Ljava/math/BigDecimal;", 22, "java.math.BigDecimal", 20},
  {JT_SQL_DATE,     "Ljava/sql/Date;",        15, "java.sql.Date",        13},
  {JT_SQL_TIME,     "Ljava/sql/Time;",        15, "java.sql.Time",        13},
  {JT_SQL_TIMESTAMP, "Ljava/sql/Timestamp;",  20, "java.sql.Timestamp",   18},
  {JT_LANG_INTEGER, "Ljava/lang/Integer;",    19, "java.lang.Integer",    17},
  {JT_LANG_LONG,    "Ljava/lang/Long;",       16, "java.lang.Long",       14},
  {JT_LANG_FLOAT,   "Ljava/lang/Float;",      17, "java.lang.Float",      15},
  {JT_LANG_DOUBLE,  "Ljava/lang/Double;",     18, "java.lang.Double",     16},
  {JT_SQL_RESULTSET,"Ljava/sql/ResultSet;",   20, "java.sql.ResultSet",   18}
};


//
// LmJavaType constructor sets index_ to be an index into
// javaTypeTable depending on the given LmParameter.
// If the LmParameter is not recognized type by LM, index_ will be
// set to LM_JAVATYPE_INVALID_INDEX(-1).
//
LmJavaType::LmJavaType(LmParameter *lmParam)
 : lmParam_(lmParam),
   type_(JT_NONE),
   index_(LM_JAVATYPE_INVALID_INDEX)
{
  if (lmParam_ == NULL)
  {
    type_ = JT_VOID;
  }
  else if (lmParam_->resultSet())
  {
    type_ = JT_SQL_RESULTSET;
  }
  else
  {
    ComFSDataType data_type = lmParam_->fsType();
    switch(data_type)
    {
      case COM_SIGNED_BIN16_FSDT:
      case COM_UNSIGNED_BIN16_FSDT:
      {
         if (lmParam_->prec() > 0)
         {
            type_ = JT_MATH_BIGDEC;
         }
         else
         {
           if (lmParam_->objMapping())
              type_ = (Type) (lmParam_->fsType() + COM_LAST_FSDT);
           else
              type_ = JT_SHORT;
         }
         break;
      }

      case COM_SIGNED_BIN32_FSDT:
      case COM_UNSIGNED_BIN32_FSDT:
      {
         if (lmParam_->prec() > 0)
         {
            type_ = JT_MATH_BIGDEC;
         }
         else
         {
           if (lmParam_->objMapping())
              type_ = (Type) (lmParam_->fsType() + COM_LAST_FSDT);
           else
              type_ = JT_INT;
         }
         break;
      }

      case COM_SIGNED_BIN64_FSDT:
      {
        if (lmParam_->prec() > 0)
        {
           type_ = JT_MATH_BIGDEC;
        }
        else
        {
          if (lmParam_->objMapping())
             type_ = (Type) (lmParam_->fsType() + COM_LAST_FSDT);
          else
             type_ = (Type) (lmParam_->fsType());
        }
        break;
      }
      case COM_SIGNED_DECIMAL_FSDT:
      case COM_UNSIGNED_DECIMAL_FSDT:
        type_ = JT_MATH_BIGDEC;
        break;

      case COM_SIGNED_NUM_BIG_FSDT:
      case COM_UNSIGNED_NUM_BIG_FSDT:
        type_ = JT_MATH_BIGDEC; // or should this be JT_LANG_STRING?
        break;

      // The SQL CHAR strings and SQL INTERVAL types are set as
      // java string types since JDBC doesn't have type INTERVAL.
      // JDBC handles SQL INTERVAL types as Types.OTHER.
      case COM_FCHAR_FSDT:
      case COM_FCHAR_DBL_FSDT:
      case COM_VCHAR_FSDT:
      case COM_VCHAR_DBL_FSDT:
      case COM_VCHAR_LONG_FSDT:
      case COM_INTERVAL_YEAR_YEAR_FSDT:
      case COM_INTERVAL_MON_MON_FSDT:
      case COM_INTERVAL_DAY_DAY_FSDT:
      case COM_INTERVAL_HOUR_HOUR_FSDT:
      case COM_INTERVAL_MIN_MIN_FSDT:
      case COM_INTERVAL_SEC_SEC_FSDT:
      case COM_INTERVAL_YEAR_MON_FSDT:
      case COM_INTERVAL_DAY_HOUR_FSDT:
      case COM_INTERVAL_DAY_MIN_FSDT:
      case COM_INTERVAL_DAY_SEC_FSDT:
      case COM_INTERVAL_HOUR_MIN_FSDT:
      case COM_INTERVAL_HOUR_SEC_FSDT:
      case COM_INTERVAL_MIN_SEC_FSDT:
        type_ = JT_LANG_STRING;
        break;

      case COM_FLOAT32_FSDT:
      case COM_FLOAT64_FSDT:
      {
        if (lmParam_->objMapping())
          type_ = (Type) (lmParam_->fsType() + COM_LAST_FSDT);
        else
          type_ = (Type) (lmParam_->fsType());

        break;
      }

      default:
        type_ = (Type) (lmParam_->fsType() +
                       (lmParam_->prec() == 0? 0: lmParam_->prec()-1));
        break;
    }
  }

  for (Int32 index = 0; index <= JAVATYPETABLE_SIZE; ++index)
  {
    if (type_ == javaTypeTable[index].javaIdx)
    {
      index_ = index;
      return;
    }
  }

}


//
// getJavaTypeName(): Returns the Java type name.
//
const char *
LmJavaType::getJavaTypeName(ComUInt32 &len) const
{
  if (index_ == LM_JAVATYPE_INVALID_INDEX)
  {
    len = 0;
    return NULL;
  }
  else
  {
    LmJavaType::TypeElement e = javaTypeTable[index_];

    len = e.javaTypeNameLen;
    return e.javaTypeName;
  }
}

//
// isJavaTypeObject() : Tells if given type is object type,
//
// Note: The types in LmJavaType are not ordered. JTINT=132
// comes before JT_LANG_STRING=66. So we can't use one single stmt
//    if (jType >= JT_LANG_STRING) return TRUE
//
// returns TRUE if type is object
//         FALSE if not
//
ComBoolean
LmJavaType::isJavaTypeObject() const
{
  switch(type_)
  {
    case JT_LANG_STRING:
    case JT_MATH_BIGDEC:
    case JT_SQL_DATE:
    case JT_SQL_TIME:
    case JT_SQL_TIMESTAMP:
    case JT_LANG_INTEGER:
    case JT_LANG_LONG:
    case JT_LANG_FLOAT:
    case JT_LANG_DOUBLE:
    case JT_SQL_RESULTSET:
      return TRUE;

    case JT_VOID:
    case JT_SHORT:
    case JT_INT:
    case JT_LONG:
    case JT_FLOAT:
    case JT_DOUBLE:
      return FALSE;
  }

  return FALSE;
}


//
// getTypeElement: return the TypeElement corresponding to 
// this instance.
//
LmJavaType::TypeElement*
LmJavaType::getTypeElement() const
{
  if (index_ != LM_JAVATYPE_INVALID_INDEX)
    return &javaTypeTable[index_];
  else
    return NULL;
}


//
// getTypeElement: TypeElement search based on Java encoded type name
// This is a static method, not an instance method.
//
LmJavaType::TypeElement*
LmJavaType::getTypeElement(char *javaTypeName)
{
  for (Int32 i = 0; i < JAVATYPETABLE_SIZE; i++)
  {
    TypeElement *e = &javaTypeTable[i];

    if (str_cmp_ne(javaTypeName, e->javaTypeName) == 0)
      return e;
  }

  return NULL;
}
