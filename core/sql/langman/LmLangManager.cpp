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
* File:         LmLangManager.cpp
* Description:  Language Manager (LM) base and support classes
* Created:      07/01/1999
* Language:     C++
*
******************************************************************************
*/

#include "CharType.h"
#include "NumericType.h"
#include "LmLangManager.h"

// a stub to avoid an undefined external
#include "NAType.h"
//////////////////////////////////////////////////////////////////////
//
// Class LmLanguageManager
//
// Note: The implementation of convertIn and convertOut in the base
// class currently meets the requirements for both an LM for Java and C.
// 
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// LM service: convertIn. 
//////////////////////////////////////////////////////////////////////
// Exclude the following methods for coverage as they are not used in LM.
// LCOV_EXCL_START
LmResult LmLanguageManager::convertIn(
  NAType   *src,
  NAType   **dst,
  NAMemory *mem)
{
  *dst = NULL;

  switch (src->getTypeQualifier()) 
  {
  case NA_CHARACTER_TYPE:
  case NA_DATETIME_TYPE:
    // (VAR)CHAR and DATE/TIME.
    *dst = new(mem) ANSIChar(mem, src->getNominalSize(), FALSE);
    return LM_CONV_REQUIRED;

  case NA_NUMERIC_TYPE:
    // NUMERIC and DECIMAL.
    if (((NumericType*)src)->decimalPrecision())
    {
      *dst = new(mem) ANSIChar(mem, src->getNominalSize(), FALSE);
      return LM_CONV_REQUIRED;
    }

    // FLOAT.
    if (src->getPrecision() == SQL_FLOAT_PRECISION)
    {
      *dst = new(mem) SQLDoublePrecision(mem, src->supportsSQLnull());
      return LM_CONV_REQUIRED;
    }

    // Other numerics.
    switch (src->getPrecision())
    {
    case SQL_SMALL_PRECISION:
    case SQL_INT_PRECISION:
    case SQL_LARGE_PRECISION:
    case SQL_REAL_PRECISION:
    case SQL_DOUBLE_PRECISION:
      return LM_OK;
    }
  }

  return LM_CONV_ERROR;
}

//////////////////////////////////////////////////////////////////////
// LM service: convertOut. 
//////////////////////////////////////////////////////////////////////
LmResult LmLanguageManager::convertOut(
  NAType   *src,
  NAType   **dst,
  NAMemory *mem)
{
  *dst = NULL;

  if (src->getTypeQualifier() == NA_NUMERIC_TYPE &&
      src->getPrecision() == SQL_FLOAT_PRECISION)
    return LM_CONV_ERROR;

  return convertIn(src, dst, mem);
}
// LCOV_EXCL_STOP

//////////////////////////////////////////////////////////////////////
// skipURLProtocol.
//////////////////////////////////////////////////////////////////////
Int32 LmLanguageManager::skipURLProtocol(const char* externalPath)
{
  static const char *protocol = "file://";
  static const Int32 len = str_len(protocol);

  if (strncmp(externalPath, protocol, len) != 0)
    return 0;

  return len;
}
