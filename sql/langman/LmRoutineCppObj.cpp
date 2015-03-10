/****************************************************************************
* File:         LmRoutineCppObj.cpp
* Description:  LM routine for C++ routines using the object interface
* Created:      01/22/2015
* Language:     C++
*
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
#include "LmRoutineCppObj.h"
#include "LmParameter.h"
#include "LmLangManagerC.h"

// Utility method to set up a "wall" before and after a buffer.
// This wall is set to a specific pattern. If someone accidentally
// writes beyond the buffer boundaries, the wall will get clobbered
// and a subsequent validation will raise an error.


#define WALL_STRING "\xcd\xcd\xcd\xcd\xcd\xcd\xcd\xcd"
#define WALL_STRING_LEN 8

//////////////////////////////////////////////////////////////////////
//
// Class LmRoutineCppObj
// If we get errors in creating this object, fill diagsArea and return
// error. Caller is responsbile to cleanup by calling destructor.
//
//////////////////////////////////////////////////////////////////////
LmRoutineCppObj::LmRoutineCppObj(
  tmudr::UDRInvocationInfo *invocationInfo,
  tmudr::UDRPlanInfo    *planInfo,
  tmudr::UDR            *interfaceObj,
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
  const char            *queryId,
  ComUInt32             inputParamRowLen,
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
               maxResultSets, COM_STYLE_CPP_OBJ, transactionAttrs, sqlAccessMode,
               externalSecurity, 
	       routineOwnerId,
               queryId, inputParamRowLen, outputRowLen, 
               currentUserName, sessionUserName,  
	       parameters, lm, routine, container, diags)
{

  invocationInfo_ = invocationInfo;
  planInfo_       = planInfo;
  interfaceObj_   = interfaceObj;
  interfaceObj_->getNextRowPtr_ = (SQLUDR_GetNextRow)getNextRowPtr;
  interfaceObj_->emitRowPtr_    = (SQLUDR_EmitRow)emitRowPtr;
  numInputTables_ = invocationInfo->getNumTableInputs();
  paramRowLen_    = inputParamRowLen;
  outputRowLen_   = outputRowLen;

  paramRow_ = new(collHeap()) char[paramRowLen_];
  memset(paramRow_,0,paramRowLen_);
  const_cast<tmudr::ParameterListInfo &>(invocationInfo->par()).setRowPtr(paramRow_);

  if (numInputTables_ > 0)
    {
      inputRows_ = new (collHeap()) char *[numInputTables_];
      for (int i=0; i<numInputTables_; i++)
        {
          int inputRowLength = invocationInfo->in(i).recordLength_;

          inputRows_[i] = new(collHeap()) char[inputRowLength];
          memset(inputRows_[i], 0, inputRowLength);
          const_cast<tmudr::TableInfo &>(
               invocationInfo->in(i)).setRowPtr(inputRows_[i]);
        }
    }
  else
    {
      inputRows_ = NULL;
    }

  outputRow_ = new (collHeap()) char[outputRowLen + 2*WALL_STRING_LEN];
  // remember the pointer to the actually usable
  // buffer, not where the wall starts
  outputRow_ += WALL_STRING_LEN;
  memset(outputRow_, 0, outputRowLen);
  setUpWall(outputRow_, outputRowLen);
  invocationInfo->out().setRowPtr(outputRow_);
}

LmRoutineCppObj::~LmRoutineCppObj()
{
  try
    {
      // delete the interface object, the virtual destructor may call user code
      delete interfaceObj_;
    }
  catch (tmudr::UDRException e)
    {
      // for now, just treat this as a fatal error, since we don't have a diags area
      throw e;
    }

  if (numInputTables_ > 0)
    {
      for (int i=0; i<numInputTables_; i++)
        NADELETEBASIC((inputRows_[i]), collHeap());
      NADELETEBASIC(inputRows_, collHeap());
    }
  // actually allocated buffer started where the wall starts
  NADELETEBASIC((outputRow_ - WALL_STRING_LEN), collHeap());
}

LmResult LmRoutineCppObj::invokeRoutine(void *inputRow,
                                        void *outputRow,
                                        ComDiagsArea *da)
{
  // This type of routine does not support the old C interface,
  // must call invokeRoutineMethod instead
  *da << DgSqlCode(-11111)
      << DgString0("Called LmRoutineCppObj::invokeRoutine()");

  return LM_ERR;
}

LmResult LmRoutineCppObj::invokeRoutineMethod(
     tmudr::UDRInvocationInfo::CallPhase phase,
     void *parameterRow,
     ComDiagsArea *da)
{
  LmResult result = LM_OK;

  try
    {
      invocationInfo_->callPhase_ = phase;
      switch (phase)
        {
        case tmudr::UDRInvocationInfo::COMPILER_INITIAL_CALL:
#ifndef NDEBUG
          {
            if (invocationInfo_->getDebugFlags() &
                tmudr::UDRInvocationInfo::DEBUG_INITIAL_COMPILE_TIME_LOOP)
              interfaceObj_->debugLoop();
          }
#endif
          interfaceObj_->describeParamsAndColumns(*invocationInfo_);
          break;

        case tmudr::UDRInvocationInfo::COMPILER_DATAFLOW_CALL:
          interfaceObj_->describeDataflowAndPredicates(*invocationInfo_);
          break;

        case tmudr::UDRInvocationInfo::COMPILER_CONSTRAINTS_CALL:
          interfaceObj_->describeConstraints(*invocationInfo_);
          break;

        case tmudr::UDRInvocationInfo::COMPILER_STATISTICS_CALL:
          interfaceObj_->describeStatistics(*invocationInfo_);
          break;

        case tmudr::UDRInvocationInfo::COMPILER_DOP_CALL:
          interfaceObj_->describeDesiredDegreeOfParallelism(
               *invocationInfo_,
               *planInfo_);
          break;

        case tmudr::UDRInvocationInfo::COMPILER_PLAN_CALL:
          interfaceObj_->describePlanProperties(*invocationInfo_,
                                                *planInfo_);
          break;

        case tmudr::UDRInvocationInfo::COMPILER_COMPLETION_CALL:
          interfaceObj_->completeDescription(*invocationInfo_,
                                             *planInfo_);
          break;

        case tmudr::UDRInvocationInfo::RUNTIME_WORK_CALL:
          {
            // copy parameter row
            memcpy(invocationInfo_->par().getRowPtr(), parameterRow, inputParamRowLen_);
#ifndef NDEBUG
            if ((invocationInfo_->getDebugFlags() &
                 tmudr::UDRInvocationInfo::DEBUG_INITIAL_RUN_TIME_LOOP_ALL) ||
                (invocationInfo_->getDebugFlags() &
                 tmudr::UDRInvocationInfo::DEBUG_INITIAL_RUN_TIME_LOOP_ONE &&
                 invocationInfo_->getMyInstanceNum() == 0))
              interfaceObj_->debugLoop();
#endif

            interfaceObj_->processData(*invocationInfo_,
                                       *planInfo_);

            if (result == LM_OK)
              {
#ifndef NDEBUG
                if (invocationInfo_->getDebugFlags() & tmudr::UDRInvocationInfo::TRACE_ROWS)
                  printf("(%d) Emitting EOD\n",
                         invocationInfo_->getMyInstanceNum());
#endif

                // call emitRow to indicate EOD, something the
                // C interface would do inside the UDF
                SQLUDR_Q_STATE qstate = SQLUDR_Q_EOD;

                (*interfaceObj_->emitRowPtr_)(
                     invocationInfo_->out().getRowPtr(),
                     0,
                     &qstate);
              }
          }
          break;

        default:
          *da << DgSqlCode(-11111)
              << DgString0("Invalid call phase in LmRoutineCppObj::invokeRoutineMethod()");

          result = LM_ERR;
          break;
        
        }
    }
  catch (tmudr::UDRException e)
    {
      strncpy(sqlState_, e.getSQLState(), sizeof(sqlState_));
      sqlState_[SQLUDR_SQLSTATE_SIZE-1] = '\0';
      strncpy(msgText_, e.getText().data(), sizeof(msgText_));
      msgText_[SQLUDR_MSGTEXT_SIZE-1] = '\0';

      result = processReturnStatus(SQLUDR_ERROR, da);
    }

  invocationInfo_->callPhase_ =
    tmudr::UDRInvocationInfo::UNKNOWN_CALL_PHASE;

  return result;
}

void LmRoutineCppObj::setUpWall(char *userBuf, int userBufLen)
{
  memcpy(userBuf - WALL_STRING_LEN, WALL_STRING, WALL_STRING_LEN);
  memcpy(userBuf + userBufLen, WALL_STRING, WALL_STRING_LEN);
}

LmResult LmRoutineCppObj::validateWall(char *userBuf,
                                       int userBufLen,
                                       ComDiagsArea *da,
                                       const char *bufferName)
{
  if (memcmp(userBuf - WALL_STRING_LEN, WALL_STRING, WALL_STRING_LEN) != 0)
    {
      if (da)
        *da << DgSqlCode(LME_BUFFER_OVERWRITE)
            << DgString0(invocationInfo_->getUDRName().data())
            << DgString1(bufferName)
            << DgString2("beginning");
      else
        throw tmudr::UDRException(
             38900,
             "UDR %s overwrote space before its %s buffer",
             invocationInfo_->getUDRName().data(),
             bufferName);
    }

  if (memcmp(userBuf + userBufLen, WALL_STRING, WALL_STRING_LEN) != 0)
    {
      if (da)
        *da << DgSqlCode(LME_BUFFER_OVERWRITE)
            << DgString0(invocationInfo_->getUDRName().data())
            << DgString1(bufferName)
            << DgString2("end");
      else
        throw tmudr::UDRException(
             38900,
             "UDR %s overwrote space after its %s buffer",
             invocationInfo_->getUDRName().data(),
             bufferName);
    }

  return LM_OK;
}

LmResult LmRoutineCppObj::validateWalls(ComDiagsArea *da)
{
  LmResult result = LM_OK;

  /* not using walls for input tables at the moment
  if (numInputTables_ > 0)
    {
      for (int i=0; i<numInputTables_; i++)
        if (validateWall(inputRows_[i],
                         inputRowLen_,
                         da,
                         (i == 0? "first input table" :
                          (i == 1? "second input table" :
                           "third or subsequent input table"))) != LM_OK)
          result = LM_ERR;
    }
  */

  if (validateWall(outputRow_, outputRowLen_, da, "output row buffer") != LM_OK)
    result = LM_ERR;

  return result;
}
