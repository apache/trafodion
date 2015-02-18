/****************************************************************************
* File:         LmRoutineCSqlRow.cpp
* Description:  LM routine for C routines using parameter style SQLROW
* Created:      08/02/2009
* Language:     C++
*
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
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
****************************************************************************
*/
#include "LmRoutineCSqlRowTM.h"
#include "LmParameter.h"
#include "LmLangManagerC.h"




// SQLROWTM function declaration
typedef Int32 (*sqlrow_func) (SQLUDR_CHAR *in_data,
                            SQLUDR_CHAR *rowDataSpace1,
                            SQLUDR_CHAR *rowDataSpace2,
                            SQLUDR_EmitRow  getRow,
                            SQLUDR_EmitRow  emitRow,
                            SQLUDR_CHAR sqlstate[6],
                            SQLUDR_CHAR msgtext[256],
                            SQLUDR_INT32 calltype,
                            SQLUDR_STATEAREA *statearea,
                            SQLUDR_UDRINFO *udrinfo);

SQLUDR_INT32 SQLUDR_INVOKE_SQLROWTM(sqlrow_func func_ptr,
                                    SQLUDR_CHAR *in_data,
                                    SQLUDR_CHAR *rowDataSpace1,
                                    SQLUDR_CHAR *rowDataSpace2,
                                    SQLUDR_GetNextRow getRow,
                                    SQLUDR_EmitRow    emitRow,
                                    SQLUDR_CHAR sqlstate[6],
                                    SQLUDR_CHAR msgtext[256],
                                    SQLUDR_INT32 calltype,
                                    SQLUDR_STATEAREA *statearea,
                                    SQLUDR_UDRINFO *udrinfo)
{
  return func_ptr(in_data, rowDataSpace1, rowDataSpace2, getRow,
                  emitRow, sqlstate, msgtext, calltype, statearea, udrinfo);
}



//////////////////////////////////////////////////////////////////////
//
// Class LmRoutineCSqlRow
// If we get errors in creating this object, fill diagsArea and return
// error. Caller is responsbile to cleanup by calling destructor.
//
//////////////////////////////////////////////////////////////////////
LmRoutineCSqlRowTM::LmRoutineCSqlRowTM(
  const char            *sqlName,
  const char            *externalName,
  const char            *librarySqlName,
  ComUInt32             numSqlParam,
  ComUInt32             numTableInfo,
  LmTableInfo           *tableInfo,
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
  LmHandle              getNextRowPtr,
  LmHandle              emitRowPtr,
  LmContainer           *container,
  ComDiagsArea          *diags)
  : LmRoutineC(sqlName, externalName, librarySqlName, numSqlParam, routineSig,
               maxResultSets, COM_STYLE_SQLROW_TM, transactionAttrs, sqlAccessMode,
               externalSecurity, 
	       routineOwnerId,
               parentQid, inputRowLen, outputRowLen, 
               currentUserName, sessionUserName,  
	       parameters, lm, routine, container, diags)
{
  // Maximum length of getNexTRow row across several input tables.
  // This length is used to instantiate input data row as part of
  // rowDataSpace1_ in invoke rountine.
  ComUInt32 maxInputRowLength = 0;

  //Initialize the function pointers for getNextRow and emitRow
  getNextRowPtr_ = (SQLUDR_GetNextRow)getNextRowPtr;
  emitRowptr_ = (SQLUDR_EmitRow)emitRowPtr;

  //The base class LmRoutineC sets up udrInfo details. We need to 
  //update table info in udrinfo.
  udrInfo_->num_table_inputs = numTableInfo;
  
  udrInfo_->table_inputs = (SQLUDR_TABLE_PARAM *) 
                         malloc(numTableInfo * sizeof(SQLUDR_TABLE_PARAM) + 32);
  LM_ASSERT(udrInfo_->table_inputs);
  memset(udrInfo_->table_inputs, 0, numTableInfo * sizeof(SQLUDR_TABLE_PARAM) + 32);

  //Iterate through each table and its params.
  for(ComUInt32 i = 0; i < udrInfo_->num_table_inputs; i++)
  {
    LmTableInfo  &lmTInfo = tableInfo[i];
    SQLUDR_TABLE_PARAM *udrTInfo = &(udrInfo_->table_inputs[lmTInfo.tabIndex_]);
    
    udrTInfo->table_name = lmTInfo.tableName_;
    udrTInfo->num_params = lmTInfo.numInColumns_;

    // Allocate space for params
    udrTInfo->params = (SQLUDR_PARAM *) 
                         malloc(lmTInfo.numInColumns_ * sizeof(SQLUDR_PARAM) + 32);
    LM_ASSERT(udrTInfo->params);
    memset(udrTInfo->params, 0, lmTInfo.numInColumns_ * sizeof(SQLUDR_PARAM) + 32);

    for(ComUInt32 j = 0; j < udrTInfo->num_params; j++)
    {
      LmParameter &p = lmTInfo.inColumnParam_[j];
      SQLUDR_PARAM &udrColumnInfo = udrTInfo->params[j];
    
      udrColumnInfo.version = SQLUDR_PARAM_CURRENT_VERSION;
      udrColumnInfo.datatype =
      (SQLUDR_INT16) getAnsiTypeFromFSType(p.fsType());
    
      udrColumnInfo.data_len = (SQLUDR_UINT32) p.inSize();

      // The parameter name seen by the routine body will be a pointer
      // to the LmParameter's copy of the name.
      udrColumnInfo.name = (SQLUDR_CHAR *) p.getParamName();

      // Union u1 contains:
      // * character_set
      // * datetime_code
      // * interval_code
      // * scale
      if (p.isCharacter())
      {
        udrColumnInfo.u1.character_set =
          (SQLUDR_INT16) p.encodingCharSet();
      }
      else if (p.isDateTime())
      {
        // For datetime types: ANSI type will be SQLTYPECODE_DATETIME and
        // SQLUDR_PARAM stores a code to indicate date, time, or
        // timestamp.
        udrColumnInfo.u1.datetime_code =
        (SQLUDR_INT16) p.getDatetimeCode();
      }
      else if (p.isInterval())
      {
        // For interval types: ANSI type will be SQLTYPECODE_INTERVAL and
        // SQLUDR_PARAM stores a code to indicate start and end fields.
        udrColumnInfo.u1.interval_code =
        (SQLUDR_INT16) getIntervalCode(p.fsType());
      }
      else
      {
        udrColumnInfo.u1.scale = (SQLUDR_INT16) p.scale();
      }

      // Union u2 contains:
      // * precision
      // * collation
      if (p.isTimeOrTimestamp())
      {
        udrColumnInfo.u2.precision = (SQLUDR_INT16) p.getTimePrecision();
      }
      else if (p.isCharacter())
      {
        SQLUDR_COLLATION collation = SQLUDR_COLLATION_UNKNOWN;
        switch (p.collation())
        {
          case CharInfo::DefaultCollation:
            collation = SQLUDR_COLLATION_DEFAULT;
            break;
          case CharInfo::CZECH_COLLATION:
            collation = SQLUDR_COLLATION_CZECH;
            break;
          case CharInfo::CZECH_COLLATION_CI:
            collation = SQLUDR_COLLATION_CZECH_CI;
            break;
          case CharInfo::SJIS_COLLATION:
            collation = SQLUDR_COLLATION_SJIS;
            break;
        }
        udrColumnInfo.u2.collation = (SQLUDR_INT16) collation;
      }
      else
      {
        udrColumnInfo.u2.precision = (SQLUDR_INT16) p.prec();
      }
    
      udrColumnInfo.data_offset = (SQLUDR_UINT32) p.inDataOffset();
      udrColumnInfo.ind_offset = (SQLUDR_UINT32) p.inNullIndOffset();
      udrColumnInfo.vc_ind_offset = (SQLUDR_UINT32) p.inVCLenIndOffset();
      udrColumnInfo.vc_ind_len = (SQLUDR_UINT8) p.inVCLenIndSize();

    }

    if(lmTInfo.rowLength_ > maxInputRowLength)
        maxInputRowLength = lmTInfo.rowLength_;
  }
  rowDataSpace1_ = new (collHeap()) char[maxInputRowLength + 1];
  memset(rowDataSpace1_, 0, maxInputRowLength + 1);

  rowDataSpace2_ = new (collHeap()) char[outputRowLen + 1];
  memset(rowDataSpace2_, 0, outputRowLen + 1);
}

LmRoutineCSqlRowTM::~LmRoutineCSqlRowTM()
{
  NADELETEBASIC(rowDataSpace1_, collHeap());
  NADELETEBASIC(rowDataSpace2_, collHeap());
}

LmResult LmRoutineCSqlRowTM::invokeRoutine(void *inputRow,
                                           void *outputRow,
                                           ComDiagsArea *da)
{
  // We can return early if the caller is requesting a FINAL call but
  // not FINAL is necessary because the INITIAL call was never made
  if (callType_ == SQLUDR_CALLTYPE_FINAL && !finalCallRequired_)
    return LM_OK;

  void *param_in_data = NULL;
  
  // If this is not a FINAL call, we need to pass the address of
  // the IN and OUT row data pointers to UDF
  //
  // For the FINAL call (invokeRoutine() during putRoutine()), there
  // are no data pointers to be passed down to routine. Only
  // statearea_ & udrInfo_ are sent.
  if (callType_ != SQLUDR_CALLTYPE_FINAL)
  {
    param_in_data = inputRow;
  }

  // Initialize SQLSTATE to all '0' characters and add a null terminator
  str_pad(sqlState_, SQLUDR_SQLSTATE_SIZE - 1, '0');
  sqlState_[SQLUDR_SQLSTATE_SIZE - 1] = 0;
  
  // Initialize SQL text to all zero bytes
  str_pad(msgText_, SQLUDR_MSGTEXT_SIZE, '\0');

  // Call the function
  Int32 retValue = SQLUDR_INVOKE_SQLROWTM((sqlrow_func) routine_,
                                      (SQLUDR_CHAR *) param_in_data,
                                      (SQLUDR_CHAR *)rowDataSpace1_,
                                      (SQLUDR_CHAR *)rowDataSpace2_,
                                      getNextRowPtr_,
                                      emitRowptr_,
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


void LmTableInfo::reset(NAHeap *udrHeap)
{
  if(reqSqlBuffer_)
  {
    udrHeap->deallocateMemory(reqSqlBuffer_);
    reqSqlBuffer_ = NULL;
  }
  if(emitSqlBuffer_)
  {
    udrHeap->deallocateMemory(emitSqlBuffer_);
    emitSqlBuffer_ = NULL;
  }

  lastReqBuffer_ = FALSE;
}
