/****************************************************************************
* File:         LmRoutineCSqlRow.cpp
* Description:  LM routine for C routines using parameter style SQLROW
* Created:      08/02/2009
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
****************************************************************************
*/
#include "LmRoutineCSqlRow.h"
#include "LmParameter.h"
#include "LmLangManagerC.h"

// SQLROW function declaration
typedef Int32 (*sqlrow_func) (SQLUDR_CHAR *input_row,
                            SQLUDR_CHAR *output_row,
                            SQLUDR_CHAR sqlstate[6],
                            SQLUDR_CHAR msgtext[256],
                            SQLUDR_INT32 calltype,
                            SQLUDR_STATEAREA *statearea,
                            SQLUDR_UDRINFO *udrinfo);

SQLUDR_INT32 SQLUDR_INVOKE_SQLROW(sqlrow_func func_ptr,
                                  SQLUDR_CHAR *input_row,
                                  SQLUDR_CHAR *output_row,
                                  SQLUDR_CHAR sqlstate[6],
                                  SQLUDR_CHAR msgtext[256],
                                  SQLUDR_INT32 calltype,
                                  SQLUDR_STATEAREA *statearea,
                                  SQLUDR_UDRINFO *udrinfo)
{
  return func_ptr(input_row, output_row, sqlstate, msgtext,
                  calltype, statearea, udrinfo);
}

//////////////////////////////////////////////////////////////////////
//
// Class LmRoutineCSqlRow
// If we get errors in creating this object, fill diagsArea and return
// error. Caller is responsbile to cleanup by calling destructor.
//
//////////////////////////////////////////////////////////////////////
LmRoutineCSqlRow::LmRoutineCSqlRow(
  const char            *sqlName,
  const char            *externalName,
  const char            *librarySqlName,
  ComUInt32             numSqlParam,
  char                  *routineSig,
  ComUInt32             maxResultSets,
  ComRoutineTransactionAttributes transactionAttrs,
  ComRoutineSQLAccess   sqlAccessMode,
  ComRoutineExternalSecurity externalSecurity,
  Int32                 routineOwnerId,
  const char            *parentQid,
  ComUInt32             inputRowLen,
  ComUInt32             outputRowLen,
  const char           *currentUserName,
  const char           *sessionUserName,
  LmParameter           *parameters,
  LmLanguageManagerC    *lm,
  LmHandle              routine,
  LmContainer           *container,
  ComDiagsArea          *diags)
  : LmRoutineC(sqlName, externalName, librarySqlName, numSqlParam, routineSig,
               maxResultSets,
               COM_LANGUAGE_C,
               COM_STYLE_SQLROW,
               transactionAttrs,
               sqlAccessMode,
               externalSecurity, 
	       routineOwnerId,
               parentQid, inputRowLen, outputRowLen, 
               currentUserName, sessionUserName,  
	       parameters, lm, routine, container, diags)
{
}

LmRoutineCSqlRow::~LmRoutineCSqlRow()
{
}

LmResult LmRoutineCSqlRow::invokeRoutine(void *inputRow,
                                         void *outputRow,
                                         ComDiagsArea *da)
{
  // We can return early if the caller is requesting a FINAL call but
  // not FINAL is necessary because the INITIAL call was never made
  if (callType_ == SQLUDR_CALLTYPE_FINAL && !finalCallRequired_)
    return LM_OK;

  void *param_in_data = NULL, *param_out_data = NULL;

  // If this is not a FINAL call, we need to pass the address of
  // the IN and OUT row data pointers to UDF
  //
  // For the FINAL call (invokeRoutine() during putRoutine()), there
  // are no data pointers to be passed down to routine. Only
  // statearea_ & udrInfo_ are sent.
  if (callType_ != SQLUDR_CALLTYPE_FINAL)
  {
    param_in_data = inputRow;
    param_out_data = outputRow;
  }

  // Initialize SQLSTATE to all '0' characters and add a null terminator
  str_pad(sqlState_, SQLUDR_SQLSTATE_SIZE - 1, '0');
  sqlState_[SQLUDR_SQLSTATE_SIZE - 1] = 0;
  
  // Initialize SQL text to all zero bytes
  str_pad(msgText_, SQLUDR_MSGTEXT_SIZE, '\0');

  // Call the function
  Int32 retValue = SQLUDR_INVOKE_SQLROW((sqlrow_func) routine_,
                                      (SQLUDR_CHAR *) param_in_data,
                                      (SQLUDR_CHAR *) param_out_data,
                                      sqlState_,
                                      msgText_,
                                      callType_,
                                      stateArea_,
                                      udrInfo_);
  
  if (callType_ == SQLUDR_CALLTYPE_INITIAL)
  {
    // Set the call type for next invocation to NORMAL if this is an
    // INITIAL call
    callType_ = SQLUDR_CALLTYPE_NORMAL;
    finalCallRequired_ = TRUE;
  }
  else if (callType_ == SQLUDR_CALLTYPE_FINAL)
  {
    // We are done if this is a FINAL call
    finalCallRequired_ = FALSE;
    return LM_OK;
  }

  // Check the return value from routine execution
  if (retValue != SQLUDR_SUCCESS)
  {
    sqlState_[SQLUDR_SQLSTATE_SIZE - 1] = 0;
    return processReturnStatus(retValue, da);
  }

  return LM_OK;
}
