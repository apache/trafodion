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
* File:         LmExpr.cpp
* Description:  Code to generate ItemExpr trees that convert UDR parameters
*               to/from formats acceptable as input to the Language Manager
* Created:      10/28/2000
* Language:     C++
*
******************************************************************************
*/

/*
  About this code:
    
  The Language Manager does not accept all SQL datatypes. When a SQL
  datatype is not acceptable as input/output to LM, the value must be
  converted to a "normalized form" before/after LM sees it. The functions
  in this file create ItemExpr trees that convert values to/from LM-normal
  form. 

  The two main functions are CreateLmInputExpr() and CreateLmOutputExpr().
  Both can be called during codegen for a UDR node to create ItemExpr 
  trees that convert to/from LM-normal form. See comments at the beginning
  of those functions for more detail. 

  This file begins with many static helper functions.

*/

#include "LmExpr.h"
#include "ItemExpr.h"
#include "ItemFunc.h"
#include "NAType.h"
#include "ItemNAType.h"
#include "CharType.h"
#include "NumericType.h"
#include "parser.h"
#include "CmpContext.h"
#include "DatetimeType.h"

// Fix for bug 3137.
// The following global is defined in SqlParserGlobals.h file and is set
// when parsing result set proxy statement. It is used in allowing a
// result set column type of VARCHAR(0) in proxy statement(resulting from
// an empty string in SELECT statement).
// It gets reset after initial parsing of the proxy statement but we need to
// set it again during codegen time to avoid error 3003.
extern THREAD_P NABoolean inRSProxyStmt;

// Helper function to create an ItemExpr tree from a scalar
// expression string.
static ItemExpr *ParseExpr(NAString &s, CmpContext &c, ItemExpr &ie)
{
  ItemExpr *result = NULL;
  Parser parser(&c);
  result = parser.getItemExprTree(s.data(), s.length(),
                                  CharInfo::UTF8
                                  , 1, &ie);
  return result;
}

//
// Helper function to get the maximum number of characters required
// to represent a value of a given NAType.
//
static Lng32 GetDisplayLength(const NAType &t)
{
  Lng32 result = t.getDisplayLength(
    t.getFSDatatype(),
    t.getNominalSize(),
    t.getPrecision(),
    t.getScale(),
    0);
  return result;
}

//
// Helper function to create an ItemExpr tree that converts a SQL
// value to a SQL string without null terminator.
//
static ItemExpr *CreateLmString(ItemExpr &source,
                                const NAType &sourceType,
                                ComRoutineLanguage language,
                                ComRoutineParamStyle style,
                                CmpContext *cmpContext)
{
  //
  // We want an ItemExpr that converts any value X to a 
  // string. We will use this SQL syntax:
  //
  //   cast(X as {CHAR|VARCHAR}(N) CHARACTER SET ISO88591)
  //
  // where N is the max display length of X. The datatype of the
  // result will be CHAR(N) or VARCHAR(N) depending on the language.
  //
  ItemExpr *result = NULL;
  NAMemory *h = cmpContext->statementHeap();
  
  Lng32 maxLength = GetDisplayLength(sourceType);
  char buf[100];
  sprintf(buf, "%d", maxLength);

  NAString *s = new (h) NAString("cast (@A1 as ", h);
  if (style == COM_STYLE_JAVA_CALL)
    (*s) += "VARCHAR(";
  else
    (*s) += "CHAR(";
  (*s) += buf;
  (*s) += ") CHARACTER SET ISO88591);";
  
  result = ParseExpr(*s, *cmpContext, source);

  return result;
}

//
// Helper function to create an ItemExpr tree that converts
// a SQL value, represented by the source ItemExpr, to the
// target type.
//
static ItemExpr *CreateCastExpr(ItemExpr &source, const NAType &target,
                                CmpContext *cmpContext)
{
  ItemExpr *result = NULL;
  NAMemory *h = cmpContext->statementHeap();
  
  NAString *s;
  s = new (h) NAString("cast(@A1 as ", h);

  (*s) += target.getTypeSQLname(TRUE);

  if (!target.supportsSQLnull())
    (*s) += " NOT NULL";

  (*s) += ");";
  
  result = ParseExpr(*s, *cmpContext, source);

  return result;
}

// Helper function to create an ItemExpr tree that converts a string
// to an INTERVAL value. This expression tree is necessary because
// SQL/MX will only convert arbitrary strings to INTERVALs when moving
// values into the CLI via an input expression.
static ItemExpr *CreateIntervalExpr(ItemExpr &source, const NAType &target,
                                    CmpContext *cmpContext)
{
  // Our goal is to create the following expression tree. Assume "@A1"
  // is the input string
  //
  // case substring(@A1 from 1 for 1)
  // when '-' then
  //   cast(-cast(substring(@A1 from 2) as interval year) as interval year)
  // else
  //   cast(@A1 as interval year)
  // end

  ItemExpr *result = NULL;
  NAMemory *h = cmpContext->statementHeap();

  NAString T = target.getTypeSQLname(TRUE);
  if (!target.supportsSQLnull())
    T += " NOT NULL";
  
  NAString *s = new (h) NAString("case substring(@A1 from 1 for 1) ", h);
  (*s) += "when '-' then cast(-cast(substring(@A1 from 2) as ";
  (*s) += T;
  (*s) += ") as ";
  (*s) += T;
  (*s) += ") else cast(@A1 as ";
  (*s) += T;
  (*s) += ") end;";

  result = ParseExpr(*s, *cmpContext, source);

  return result;
}

//---------------------------------------------------------------------------
// CreateLmInputExpr
// 
// Creates an ItemExpr tree to convert a SQL value to LM-normal form.
// Returns the tree in newExpr. Right now LM-normal form is:
// - SQL native format for
//       * binary precision integers
//       * floating point and
//       * Character types(char and varchar)
// - non-null terminated C string(CHAR(N) or VARCHAR(N)) for everything else
//---------------------------------------------------------------------------
LmExprResult CreateLmInputExpr(const NAType &formalType,
                               ItemExpr &actualValue,
                               ComRoutineLanguage language,
                               ComRoutineParamStyle style,
                               CmpContext *cmpContext,
                               ItemExpr *&newExpr)
{
  LmExprResult result = LmExprOK;
  NABoolean isResultSet = FALSE;

  if (LmTypeIsString(formalType, language, style, isResultSet))
  {
    if (formalType.getTypeQualifier() == NA_CHARACTER_TYPE)
    {
      newExpr = &actualValue;
    }
    else
    {
      newExpr = CreateLmString(actualValue, formalType, language, style, cmpContext);
    }
  }
  else
  {
    newExpr = CreateCastExpr(actualValue, formalType, cmpContext);
  }

  if (newExpr == NULL)
  {
    result = LmExprError;
  }

  return result;
}

//---------------------------------------------------------------------------
// CreateLmOutputExpr
// 
// Creates an ItemExpr tree to convert a value in LM-normal form to 
// SQL native format. Returns the tree in outputValue. The expressions
// will be used by a UDR TCB to process a UDR output value in the UDR
// server's reply buffer.
// 
// One other side-effect of this function:
// This function determines the SQL type that corresponds to LM-normal
// form and creates an NATypeToItem instance of that type. This NATypeToItem
// represents a UDR output value that has just come back from the UDR server
// in a reply buffer. A UDR TCB must convert the value from LM-normal format
// to the SQL formal parameter type. To allow codegen for the UDR TCB to set
// up the map table correctly, we return the NATypeToItem instance in 
// normalizedValue.
//---------------------------------------------------------------------------
LmExprResult CreateLmOutputExpr(const NAType &formalType,
                                ComRoutineLanguage language,
                                ComRoutineParamStyle style,
                                CmpContext *cmpContext,
                                ItemExpr *&normalizedValue,
                                ItemExpr *&outputValue,
                                NABoolean isResultSet)
{
  LmExprResult result = LmExprError;
  normalizedValue = NULL;
  outputValue = NULL;
  NAMemory *h = cmpContext->statementHeap();
  NAType *replyType = NULL;
  NABoolean isString = LmTypeIsString(formalType, language, style, isResultSet);

  if (isString &&
      (formalType.getTypeQualifier() != NA_CHARACTER_TYPE))
  {
    if (isResultSet || style != COM_STYLE_JAVA_CALL)
    {
      Lng32 maxLength = GetDisplayLength(formalType);
      replyType = new (h) SQLChar(maxLength);
    }
    else
    {
      Lng32 maxLength = GetDisplayLength(formalType);
      replyType = new (h) SQLVarChar(maxLength);
    }
  }
  else
  {
    replyType = formalType.newCopy(h);
  }
  
  if (replyType)
  {
    if (style == COM_STYLE_JAVA_CALL)
    {
      // $$$$ TBD: let's assume all CALL statement parameters are
      // nullable for now, until we are sure UDR server's null
      // processing is correct for both nullable and non-nullable
      // types.
      replyType->setNullable(TRUE);
    }
    else
      // Copy the nullability attribute from the formal type
      replyType->setNullable(formalType);

    normalizedValue = new (h) NATypeToItem(replyType->newCopy(h));
    if (normalizedValue)
    {
      // Note that we didn't apply cast expr for CHAR and VARCHAR for
      // IN params. But we need to have cast expr for OUT params, even
      // though there is no actual casting of data, because we need to
      // move data from buffer to up Queue in the root node.

      if (formalType.getTypeQualifier() == NA_INTERVAL_TYPE && isString)
        outputValue = CreateIntervalExpr(*normalizedValue,
                                         formalType,
                                         cmpContext);
      else
      {
    	// Fix for bug 3137.
    	// Set the parser flag to allow VARCHAR(0) as proxy column type
    	// before entering the parser. See comment above for more detail.
    	if (isResultSet && formalType.getTypeQualifier() == NA_CHARACTER_TYPE)
    	   inRSProxyStmt = TRUE;
        outputValue = CreateCastExpr(*normalizedValue,
                                     formalType,
                                     cmpContext);
        inRSProxyStmt = FALSE;
      }

      if (outputValue)
      {
        result = LmExprOK;
      }
      else
      {
        normalizedValue = NULL;
      }
    }
  }
  
  return result;
}

//---------------------------------------------------------------------------
// LM type info functions
//
// LM-required types have certain attributes that are of interest during
// codegen for a UDR operator. The following functions all return TRUE
// or FALSE depending on whether a given NAType has one of these attributes.
//---------------------------------------------------------------------------

//
// This function returns TRUE if LM wants values of the
// specified type t to be converted to/from C strings. The only 
// SQL types that do not need to be converted to C strings are:
// 
//  INT, TINYINT, SMALLINT, LARGEINT, FLOAT, REAL, DOUBLE PRECISION, BOOLEAN
//
// because these types map to Java primitive types:
//
//  INT -> int
//  TINYINT -> byte
//  SMALLINT -> short
//  LARGEINT -> long
//  FLOAT -> float or double
//  REAL -> float
//  DOUBLE PREC -> double
//  BOOLEAN -> byte
//
// For the object-oriented Java and C++ parameter styles, we represent
// intervals as a signed numeric of 2, 4, or 8 bytes, in the other
// parameter styles it is represented as a string.
//
//  INTERVAL -> short or int or long or string
//
// Note: When changing this, a change in file ../sqludr/sqludr.cpp
//       may be required as well (and other places, of course)
NABoolean LmTypeIsString(const NAType &t,
                         ComRoutineLanguage language,
                         ComRoutineParamStyle style,
                         NABoolean isResultSet)
{
  NABoolean result = TRUE;
  
  if (t.getTypeQualifier() == NA_NUMERIC_TYPE)
  {
    const NumericType &nt = *((const NumericType *) &t);
    if (nt.isExact())
    {
      if (nt.binaryPrecision())
      {
        // INT
        // SMALLINT
        // LARGEINT
        result = FALSE;
      }
      else if (nt.isDecimal())
      {
        // DECIMAL
        if (isResultSet)
          result = FALSE;
        else
          result = TRUE;
      }
      else
      {
        // NUMERIC
        // Cases to consider
        // * SPJ result sets: LM format is internal format
        // * Java call style: LM format is a string
        // * Bignum: LM format is a string
        // * C/C++ routines or Java object style: LM format is internal format
        if (isResultSet)
          result = FALSE;
        else if (language != COM_LANGUAGE_C &&
                 language != COM_LANGUAGE_CPP &&
                 style != COM_STYLE_JAVA_OBJ)
          result = TRUE;
        else if (nt.isBigNum())
          result = TRUE;
        else
          result = FALSE;
      }
    }
    else
    {
      // FLOAT (8-byte value)
      // REAL (4-byte value)
      // DOUBLE PRECISION = FLOAT(52)
      result = FALSE;
    }
  }
  else if (t.getTypeQualifier() == NA_INTERVAL_TYPE &&
           (style == COM_STYLE_JAVA_OBJ ||
            style == COM_STYLE_CPP_OBJ))
  {
    // in the object-oriented styles, use the native
    // interval representation as a number
    result = FALSE;
  }
  else if (t.getTypeQualifier() == NA_BOOLEAN_TYPE)
    result = FALSE;

  return result;
}

//
// This function returns TRUE if LM requires a precision setting 
// for the given type.
//
NABoolean LmTypeSupportsPrecision(const NAType &t)
{
  NABoolean result = FALSE;
  NABuiltInTypeEnum q = t.getTypeQualifier();
  
  switch (q)
  {
    case NA_NUMERIC_TYPE:
    {
      const NumericType &nt = *((const NumericType *) &t);
      if (nt.isExact() && !nt.binaryPrecision())
      {
        // NUMERIC, DECIMAL
        result = TRUE;
      }
    }
    break;

    case NA_DATETIME_TYPE:
    {
      // For DATE, TIME, and TIMESTAMP we store the datetime code as
      // precision. The datetime code indicates whether the type is
      // DATE, TIME, or TIMESTAMP.
      result = TRUE;
    }
    break;
  }
  
  return result;
}

// This function returns TRUE if LM requires a scale setting 
// for the given type.
NABoolean LmTypeSupportsScale(const NAType &t)
{
  NABoolean result = FALSE;
  NABuiltInTypeEnum q = t.getTypeQualifier();
  
  switch (q)
  {
    case NA_NUMERIC_TYPE:
    {
      const NumericType &nt = *((const NumericType *) &t);
      if (nt.isExact() && !nt.binaryPrecision())
      {
        // NUMERIC, DECIMAL
        result = TRUE;
      }
    }
    break;

    case NA_DATETIME_TYPE:
    {
      const DatetimeType &dt = *((const DatetimeType *) &t);
      DatetimeType::Subtype subtype = dt.getSubtype();
      if (subtype == DatetimeType::SUBTYPE_SQLTime ||
          subtype == DatetimeType::SUBTYPE_SQLTimestamp)
      {
        // For TIME and TIMESTAMP we store fractional precision in the
        // scale field
        result = TRUE;
      }
    }
    break;
  }
  
  return result;
}

// This function returns TRUE if LM considers the given type to
// be an "LM object type". 
//
// *** NOT IMPLEMENTED YET ***
// Semantics for LM object types are not completely defined so
// we cannot implement this function yet.
NABoolean LmTypeIsObject(const NAType &t)
{
  NABoolean result = FALSE;
  return result;
}
