/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutineC.cpp
* Description:  Base class for C routines
* Created:      05/14/2008
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
**********************************************************************/

#include "LmRoutineC.h"
#include "ComVersionDefs.h"

SQLUDR_CHAR *LmRoutineC::host_data_ = NULL;

LmRoutineC::LmRoutineC(const char   *sqlName,
                       const char   *externalName,
                       const char   *librarySqlName,
                       ComUInt32    numSqlParam,
                       char         *routineSig,
                       ComUInt32    maxResultSets,
                       ComRoutineLanguage language,
                       ComRoutineParamStyle paramStyle,
                       ComRoutineTransactionAttributes transactionAttrs,
                       ComRoutineSQLAccess sqlAccessMode,
                       ComRoutineExternalSecurity externalSecurity,
                       Int32        routineOwnerId,
                       const char   *parentQid,
                       ComUInt32    inputParamRowLen,
                       ComUInt32    outputRowLen,
                       const char   *currentUserName,
                       const char   *sessionUserName,
                       LmParameter  *parameters,
                       LmLanguageManagerC *lm,
                       LmHandle     routine,
                       LmContainer  *container,
                       ComDiagsArea *diagsArea)
  : LmRoutine(container, routine, sqlName, externalName, librarySqlName,
              numSqlParam, maxResultSets,
              language, paramStyle, transactionAttrs, sqlAccessMode,
              externalSecurity, 
	      routineOwnerId,
              parentQid, inputParamRowLen, outputRowLen, 
	          currentUserName, sessionUserName, 
	          parameters, lm),
    callType_(SQLUDR_CALLTYPE_INITIAL),
    stateArea_(NULL),
    udrInfo_(NULL),
    finalCallRequired_(FALSE)
{
  // If we ever support C stored procedures, we need to handle the
  // case of 0 inputs and 0 outputs. For functions there is always at
  // least one output.
  LM_ASSERT(numSqlParam_ > 0);
  
  // Allocate (static) host_data_ if it's not already allocated.
  // This will never be deleted and stays until the process dies.
  if (LmRoutineC::host_data_ == NULL)
  {
    LmRoutineC::host_data_ = (SQLUDR_CHAR *)
      malloc(SQLUDR_STATEAREA_BUFFER_SIZE + 128);
    LM_ASSERT(LmRoutineC::host_data_);
    memset((char *)LmRoutineC::host_data_, 0,
           SQLUDR_STATEAREA_BUFFER_SIZE + 128);
  }

  // Allocate the SQLUDR_STATEAREA structure
  stateArea_ = (SQLUDR_STATEAREA *) malloc(sizeof(SQLUDR_STATEAREA) + 32);
  LM_ASSERT(stateArea_);
  memset(stateArea_, 0, sizeof(SQLUDR_STATEAREA) + 32);
  stateArea_->version = SQLUDR_STATEAREA_CURRENT_VERSION;
  
  stateArea_->host_data.data = LmRoutineC::host_data_;
  stateArea_->host_data.length = SQLUDR_STATEAREA_BUFFER_SIZE;;

  stateArea_->stmt_data.data = (SQLUDR_CHAR *)
    malloc(SQLUDR_STATEAREA_BUFFER_SIZE + 128);
  LM_ASSERT(stateArea_->stmt_data.data);
  memset((char *)stateArea_->stmt_data.data, 0,
         SQLUDR_STATEAREA_BUFFER_SIZE + 128);
  stateArea_->stmt_data.length = SQLUDR_STATEAREA_BUFFER_SIZE;
  
  // Allocate SQLUDR_UDRINFO structure
  udrInfo_ = (SQLUDR_UDRINFO *) malloc(sizeof(SQLUDR_UDRINFO) + 32);
  LM_ASSERT(udrInfo_);
  memset(udrInfo_, 0, sizeof(SQLUDR_UDRINFO) + 32);

  udrInfo_->version = SQLUDR_UDRINFO_CURRENT_VERSION;
  udrInfo_->sql_version = 2400;
  udrInfo_->routine_name = sqlName_;
  // udrInfo_->library_name = librarySqlName_;
  
  const char *qid = getParentQid();
  if (qid)
    udrInfo_->query_id = copy_and_pad(qid, strlen(qid), 8);

  // Allocate memory for SQLUDR_PARAM structure.
  // We assume that the inputs come first, followed by outputs
  ComUInt32 num_inputs = 0, num_outputs = 0;
  SQLUDR_PARAM *sqludr_params = (SQLUDR_PARAM *)
    malloc(numSqlParam_ * sizeof(SQLUDR_PARAM) + 32);
  LM_ASSERT(sqludr_params);
  memset(sqludr_params, 0, numSqlParam_ * sizeof(SQLUDR_PARAM) + 32);

  // Initialize the SQLUDR_PARAM structures
  ComUInt32 dataBytes = 0;
  ComUInt32 i;
  for (i = 0; i < numSqlParam_; i++)
  {
    LmParameter &p = lmParams_[i];

    switch (p.direction())
    {
      case COM_INPUT_COLUMN:
	num_inputs++;
        dataBytes = p.inSize();
        break;
      case COM_OUTPUT_COLUMN:
	num_outputs++;
        dataBytes = p.outSize();
        break;
      default:
        LM_ASSERT1(0, "Invalid parameter mode");
        break;
    }
      
    SQLUDR_PARAM &udfParamInfo = sqludr_params[i];
    udfParamInfo.version = SQLUDR_PARAM_CURRENT_VERSION;
    udfParamInfo.datatype =
      (SQLUDR_INT16) getAnsiTypeFromFSType(p.fsType());
    
    udfParamInfo.data_len = (SQLUDR_UINT32) dataBytes;

    // The parameter name seen by the routine body will be a pointer
    // to the LmParameter's copy of the name.
    udfParamInfo.name = (SQLUDR_CHAR *) p.getParamName();

    // Union u1 contains:
    // * character_set
    // * datetime_code
    // * interval_code
    // * scale
    if (p.isCharacter())
    {
      udfParamInfo.u1.character_set =
        (SQLUDR_INT16) p.encodingCharSet();
    }
    else if (p.isDateTime())
    {
      // For datetime types: ANSI type will be SQLTYPECODE_DATETIME and
      // SQLUDR_PARAM stores a code to indicate date, time, or
      // timestamp.
      udfParamInfo.u1.datetime_code =
        (SQLUDR_INT16) p.getDatetimeCode();
    }
    else if (p.isInterval())
    {
      // For interval types: ANSI type will be SQLTYPECODE_INTERVAL and
      // SQLUDR_PARAM stores a code to indicate start and end fields.
      udfParamInfo.u1.interval_code =
        (SQLUDR_INT16) getIntervalCode(p.fsType());
    }
    else
    {
      udfParamInfo.u1.scale = (SQLUDR_INT16) p.scale();
    }

    // Union u2 contains:
    // * precision
    // * collation
    if (p.isTimeOrTimestamp())
    {
      udfParamInfo.u2.precision = (SQLUDR_INT16) p.getTimePrecision();
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
      udfParamInfo.u2.collation = (SQLUDR_INT16) collation;
    }
    else
    {
      udfParamInfo.u2.precision = (SQLUDR_INT16) p.prec();
    }
    
    // Set data offsets and lengths
    if((getParamStyle() == COM_STYLE_SQLROW) ||
       (getParamStyle() == COM_STYLE_SQLROW_TM))
    {
      switch (p.direction())
      {
        case COM_INPUT_COLUMN:
          udfParamInfo.data_offset = (SQLUDR_UINT32) p.inDataOffset();
          udfParamInfo.ind_offset = (SQLUDR_UINT32) p.inNullIndOffset();
          udfParamInfo.vc_ind_offset = (SQLUDR_UINT32) p.inVCLenIndOffset();
          udfParamInfo.vc_ind_len = (SQLUDR_UINT8) p.inVCLenIndSize();
          break;
        case COM_OUTPUT_COLUMN:
          udfParamInfo.data_offset = (SQLUDR_UINT32) p.outDataOffset();
          udfParamInfo.ind_offset = (SQLUDR_UINT32) p.outNullIndOffset();
          udfParamInfo.vc_ind_offset = (SQLUDR_UINT32) p.outVCLenIndOffset();
          udfParamInfo.vc_ind_len = (SQLUDR_UINT8) p.outVCLenIndSize();
          break;
        default:
          LM_ASSERT1(0, "Invalid parameter mode");
          break;
      }

    } // if (getParamStyle() == COM_STYLE_SQLROW)
    
  } // for each param
    
  // Fill in the rest of udrInfo_ structure fields
  udrInfo_->sql_version = (SQLUDR_UINT16) ComVersion_GetMXV();
  udrInfo_->row_format = SQLUDR_ROWFORMAT_EXPLODED;
  udrInfo_->num_inputs = num_inputs;
  udrInfo_->num_return_values = num_outputs;
  udrInfo_->inputs = (num_inputs > 0 ? &sqludr_params[0] : NULL);
  udrInfo_->return_values =
    (num_outputs > 0 ? &sqludr_params[num_inputs] : NULL);
  udrInfo_->in_row_length = inputParamRowLen_;
  udrInfo_->out_row_length = outputRowLen_;

  switch (getParamStyle())
  {
    case COM_STYLE_SQL:
      udrInfo_->param_style = SQLUDR_PARAMSTYLE_SQL;
      udrInfo_->param_style_version = 1;
      break;
    case COM_STYLE_SQLROW:
    case COM_STYLE_SQLROW_TM:
      udrInfo_->param_style = SQLUDR_PARAMSTYLE_SQLROW;
      udrInfo_->param_style_version = 1;
      break;
    case COM_STYLE_CPP_OBJ:
      // udrInfo_ is not used
      break;
    default:
      LM_ASSERT(0);
      break;
  }

  // Fill current user name and session user name 
  
  if (currentUserName)
    udrInfo_->current_user_name = copy_and_pad(currentUserName, strlen(currentUserName), 8);
  
  if (sessionUserName)
    udrInfo_->session_user_name = copy_and_pad(sessionUserName, strlen(sessionUserName), 8);
  
} // LmRoutineC::LmRoutineC

LmRoutineC::~LmRoutineC()
{
  // udrInfo_->return_values is not needed to be deleted. It's
  // all consecutively allocated memory starting from
  // udrInfo_->inputs.
  SQLUDR_PARAM *sqludr_params = udrInfo_->inputs;
  free(sqludr_params);

  if (stateArea_ && stateArea_->stmt_data.data)
    free(stateArea_->stmt_data.data);

  if (stateArea_)
    free(stateArea_);

  if (udrInfo_)
  {
    if (udrInfo_->query_id)
      free(udrInfo_->query_id);
    if (udrInfo_->current_user_name)
      free(udrInfo_->current_user_name);
    if (udrInfo_->session_user_name)
      free(udrInfo_->session_user_name);
    deletePassThroughInputs();
    free(udrInfo_);
  }
}

void
LmRoutineC::setNumPassThroughInputs(ComUInt32 numPassThroughInputs)
{
  deletePassThroughInputs();

  udrInfo_->num_pass_through = numPassThroughInputs;
  udrInfo_->pass_through = NULL;
  
  if (numPassThroughInputs > 0)
  {
    ComUInt32 numBytes = (numPassThroughInputs * sizeof(SQLUDR_BUFFER)) + 32;
    udrInfo_->pass_through = (SQLUDR_BUFFER *) malloc(numBytes);
    LM_ASSERT(udrInfo_->pass_through);
    memset(udrInfo_->pass_through, 0, numBytes);
  }
}

void
LmRoutineC::setPassThroughInput(ComUInt32 index,
	                        void *data,
	                        ComUInt32 dataLen)
{
  SQLUDR_BUFFER &pass_through_buffer = udrInfo_->pass_through[index];

  pass_through_buffer.length = dataLen;

  if (dataLen > 0)
  {
    LM_ASSERT(data);
    pass_through_buffer.data = copy_and_pad((char *) data, dataLen, 32);
  }
  else
  {
    pass_through_buffer.data = copy_and_pad("", 0, 32);
  }
}

void LmRoutineC::deletePassThroughInputs()
{
  if (udrInfo_)
  {
    if (udrInfo_->num_pass_through > 0)
    {
      LM_ASSERT(udrInfo_->pass_through);

      for (ComUInt32 i = 0; i < udrInfo_->num_pass_through; i++)
      {
        SQLUDR_BUFFER &b = udrInfo_->pass_through[i];
        free(b.data);
      }

      free(udrInfo_->pass_through);
    }
    
    udrInfo_->num_pass_through = 0;
    udrInfo_->pass_through = NULL;
  }
}

LmResult LmRoutineC::handleFinalCall(ComDiagsArea *diagsArea)
{
  setFinalCall();
  return invokeRoutine(NULL, NULL, diagsArea);
}

LmResult
LmRoutineC::processReturnStatus(ComSInt32 retcode, ComDiagsArea *diags)
{
  char *returnMsgText = NULL, *noMsgText = NULL;

  if (retcode == SQLUDR_SUCCESS)
    return LM_OK;

  if (msgText_[0] != '\0')
  {
    returnMsgText = msgText_;
  }
  else
  {
    const char *text =
      "No SQL message text was provided by user-defined function ";
    ComUInt32 msgLen = str_len(text) + str_len(getNameForDiags());
    noMsgText = new (collHeap()) char[msgLen + 1];
    sprintf(noMsgText, "%s%s", text, getNameForDiags());

    returnMsgText = noMsgText;
  }

  // Check the returned SQLSTATE value and raise appropriate
  // SQL code. Valid SQLSTATE values begin with "38" except "38000"
  if ((strncmp(sqlState_, "38", 2) == 0) &&
      (strncmp(sqlState_, "38000", 5) != 0))
  {
    Int32 sqlCode = (retcode == SQLUDR_ERROR) ?
      -LME_CUSTOM_ERROR : LME_CUSTOM_WARNING;

    *diags << DgSqlCode(sqlCode)
           << DgString0(returnMsgText)
	   << DgString1(sqlState_);
    *diags << DgCustomSQLState(sqlState_);
  }
  else
  {
    Int32 sqlCode = (retcode == SQLUDR_ERROR) ? -LME_UDF_ERROR : LME_UDF_WARNING;

    *diags << DgSqlCode(sqlCode)
	   << DgString0(getNameForDiags())
           << DgString1(sqlState_)
           << DgString2(returnMsgText);
  }

  if (noMsgText)
    NADELETEBASIC(noMsgText, collHeap());

  return (retcode == SQLUDR_ERROR) ? LM_ERR: LM_OK;
}

