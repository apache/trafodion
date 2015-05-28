/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2015 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/
#ifndef LM_EXPR_H
#define LM_EXPR_H
/* -*-C++-*-
******************************************************************************
*
* File:         LmExpr.h
* Description:  Code to create ItemExpr trees that convert UDR parameters
*               to/from formats acceptable as input to the Language Manager
*               
* Created:      10/28/2000
* Language:     C++
*
*
*
******************************************************************************
*/

/*
    This file defines global functions used during UDR codegen to
    manage expressions that convert SQL values to/from formats
    acceptable to the Language Manager (LM). See comments in 
    LmExpr.cpp for details.
*/

#include "NABoolean.h"
#include "ComSmallDefs.h"

//
// Forward class references
//
class NAType;
class ItemExpr;
class CmpContext;

enum LmExprResult
{
  LmExprOK,
  LmExprError
};

NABoolean LmTypeSupportsPrecision(const NAType &t);
NABoolean LmTypeSupportsScale(const NAType &t);
NABoolean LmTypeIsString(const NAType &t,
                         ComRoutineLanguage language,
                         ComRoutineParamStyle style,
                         NABoolean isResultSet);
NABoolean LmTypeIsObject(const NAType &t);

LmExprResult CreateLmInputExpr(const NAType &formalType,
                               ItemExpr &actualValue,
                               ComRoutineLanguage language,
                               ComRoutineParamStyle style,
                               CmpContext *cmpContext,
                               ItemExpr *&newExpr);

LmExprResult CreateLmOutputExpr(const NAType &formalType,
                                ComRoutineLanguage language,
                                ComRoutineParamStyle style,
                                CmpContext *cmpContext,
                                ItemExpr *&newSourceExpr,
                                ItemExpr *&newTargetExpr,
                                NABoolean isResultSet);

#endif // LM_EXPR_H

