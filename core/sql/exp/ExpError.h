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
****************************************************************************
*
* File:         ExpError.h (previously part of /executor/ex_error.h)
* Description:
*
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#ifndef EXP_ERROR_H
#define EXP_ERROR_H

#include <stdio.h>

class ComDiagsArea;
class NAMemory;
class ex_clause;

#include "OperTypeEnum.h"
#include "NAVersionedObject.h"

#include "ExpErrorEnums.h"

// -----------------------------------------------------------------------
// List of all errors generated in the SQL executor code 
// -----------------------------------------------------------------------

// NOTE: All the enum values are moved to "ExpErrorEnums.h"


class ComCondition;

#define MAX_OFFENDING_SOURCE_DATA_DISPLAY_LEN  256

// This version of ExRaiseSqlError is used by the expressions code.  In
// addition to having slightly different parameters, it differs from the
// above version in that it puts an error condition in the supplied
// ComDiagsArea rather than in a copy.

ComDiagsArea *ExAddCondition(NAMemory * heap, ComDiagsArea** diagsArea,
			     Lng32 err, ComCondition** cond=NULL,
			     Lng32 * intParam1 = NULL,
			     Lng32 * intParam2 = NULL,
			     Lng32 * intParam3 = NULL,
			     const char * stringParam1 = NULL,
			     const char * stringParam2 = NULL,
			     const char * stringParam3 = NULL);

ComDiagsArea *ExRaiseSqlError(NAMemory * heap, ComDiagsArea** diagsArea,
			      ExeErrorCode err, ComCondition** cond=NULL,
			      Lng32 * intParam1 = NULL,
			      Lng32 * intParam2 = NULL,
			      Lng32 * intParam3 = NULL,
			      const char * stringParam1 = NULL,
			      const char * stringParam2 = NULL,
			      const char * stringParam3 = NULL);

ComDiagsArea *ExRaiseSqlError(NAMemory * heap, ComDiagsArea** diagsArea,
                              Lng32 err, 
                              Lng32 * intParam1 = NULL,
                              Lng32 * intParam2 = NULL,
                              Lng32 * intParam3 = NULL,
                              const char * stringParam1 = NULL,
                              const char * stringParam2 = NULL,
                              const char * stringParam3 = NULL);

ComDiagsArea *ExRaiseSqlWarning(NAMemory * heap, ComDiagsArea** diagsArea,
				ExeErrorCode err, ComCondition** cond=NULL);
ComDiagsArea *ExRaiseSqlWarning(NAMemory * heap, ComDiagsArea** diagsArea,
                              ExeErrorCode err, ComCondition** cond,
                              Lng32 * intParam1 = NULL,
                              Lng32 * intParam2 = NULL,
                              Lng32 * intParam3 = NULL,
                              const char * stringParam1 = NULL,
                              const char * stringParam2 = NULL,
                              const char * stringParam3 = NULL);
ComDiagsArea *ExRaiseFunctionSqlError(NAMemory * heap, 
				      ComDiagsArea** diagsArea,
				      ExeErrorCode err, 
				      NABoolean derivedFunction = FALSE,
				      OperatorTypeEnum origOperType = ITM_FIRST_ITEM_OP,
				      ComCondition** cond=NULL);
ComDiagsArea *ExRaiseDetailSqlError(CollHeap* heap, 
				    ComDiagsArea** diagsArea,
				    ExeErrorCode err, 
				    Int32 pciInst,
                                    char *op1,
                                    char *op2 = NULL,
                                    char *op3 = NULL);

ComDiagsArea *ExRaiseDetailSqlError(CollHeap* heap, 
				    ComDiagsArea** diagsArea,
				    ExeErrorCode err, 
				    ex_clause *clause,
                                    char *op_data[]);

ComDiagsArea *ExRaiseDetailSqlError(CollHeap* heap, 
				    ComDiagsArea** diagsArea,
				    ExeErrorCode err, 
				    char *src,
                                    Int32 srcLength,
                                    Int16 srcType,
                                    Int32 srcScale,
                                    Int16 tgtType,
                                    UInt32 flags,
                                    Int32 tgtLength = -1,
                                    Int32 tgtScale = -1,
                                    Int32 tgtPrecision = 0,
                                    Int32 srcPrecision = -1);
char *stringToHex(char * out, Int32 outLen, char * in, Int32 inLen);
                                                                
#endif /* EXP_ERROR_H */


