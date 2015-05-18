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
  ComUInt32              maxResultSets,
  ComRoutineTransactionAttributes transactionAttrs,
  ComRoutineSQLAccess    sqlAccessMode,
  ComRoutineExternalSecurity externalSecurity,
  Int32                  routineOwnerId,
  LmLanguageManagerC    *lm,
  LmContainer           *container,
  ComDiagsArea          *diags)
  : LmRoutine(container,
              (LmHandle) interfaceObj,
              sqlName,
              externalName,
              librarySqlName,
              0,
              maxResultSets,
              COM_LANGUAGE_CPP,
              COM_STYLE_CPP_OBJ,
              transactionAttrs,
              sqlAccessMode,
              externalSecurity, 
              routineOwnerId,
              invocationInfo->getQueryId().c_str(),
              invocationInfo->par().getRecordLength(),
              invocationInfo->out().getRecordLength(),
              invocationInfo->getCurrentUser().c_str(),
              invocationInfo->getSessionUser().c_str(),
              NULL,
              lm),
    planInfos_(collHeap()),
    invocationInfo_(invocationInfo),
    interfaceObj_(interfaceObj),
    paramRow_(NULL),
    inputRows_(NULL),
    outputRow_(NULL)
{
  if (planInfo)
    planInfos_.insertAt(0, planInfo);
  interfaceObj_->getNextRowPtr_ = NULL; // set these with setFunctionPtrs()
  interfaceObj_->emitRowPtr_    = NULL;
  numInputTables_   = invocationInfo->getNumTableInputs();

  if (inputParamRowLen_ > 0)
    {
      paramRow_ = new(collHeap()) char[inputParamRowLen_];
      memset(paramRow_,0,inputParamRowLen_);
      const_cast<tmudr::ParameterListInfo &>(invocationInfo->par()).setRowPtr(paramRow_);
    }

  // inputRows_ and outputRow_ will be allocated in setRuntimeInfo() 
}

LmRoutineCppObj::~LmRoutineCppObj()
{
  delete invocationInfo_;
  for (CollIndex i=0; i<planInfos_.getUsedLength(); i++)
    if (planInfos_.used(i))
      delete planInfos_[i];

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
  catch (...)
    {
      // for now, just treat this as a fatal error, since we don't have a diags area
      throw;
    }

  if (paramRow_)
    NADELETEBASIC(paramRow_, collHeap());

  if (inputRows_)
    {
      for (int i=0; i<numInputTables_; i++)
        if (inputRows_[i])
          NADELETEBASIC((inputRows_[i]), collHeap());
      NADELETEBASIC(inputRows_, collHeap());
    }
  if (outputRow_)
    // actually allocated buffer started where the wall starts
    NADELETEBASIC((outputRow_ - WALL_STRING_LEN), collHeap());
}

const char *LmRoutineCppObj::getParentQid()
{
  return invocationInfo_->getQueryId().c_str();
}

void LmRoutineCppObj::setRuntimeInfo(
     const char   *parentQid,
     int           totalNumInstances,
     int           myInstanceNum)
{
  invocationInfo_->setQueryId(parentQid);
  invocationInfo_->setTotalNumInstances(totalNumInstances);
  invocationInfo_->setMyInstanceNum(myInstanceNum);

  // allocate row buffers
  if (numInputTables_ > 0)
    {
      inputRows_ = new (collHeap()) char *[numInputTables_];
      for (int i=0; i<numInputTables_; i++)
        {
          int inputRowLength = invocationInfo_->in(i).recordLength_;

          if (inputRowLength > 0)
            {
              inputRows_[i] = new(collHeap()) char[inputRowLength];
              memset(inputRows_[i], 0, inputRowLength);
              const_cast<tmudr::TableInfo &>(
                   invocationInfo_->in(i)).setRowPtr(inputRows_[i]);
            }
          else
            // at compile time, input rows are not specified
            inputRows_[i] = NULL;
        }
    }
  else
    {
      inputRows_ = NULL;
    }

  if (outputRowLen_ > 0)
    {
      outputRow_ = new (collHeap()) char[outputRowLen_ + 2*WALL_STRING_LEN];
      // remember the pointer to the actually usable
      // buffer, not where the wall starts
      outputRow_ += WALL_STRING_LEN;
      memset(outputRow_, 0, outputRowLen_);
      setUpWall(outputRow_, outputRowLen_);
      invocationInfo_->out().setRowPtr(outputRow_);
    }
  else
    outputRow_ = NULL;
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
     /* IN */     tmudr::UDRInvocationInfo::CallPhase phase,
     /* IN */     const char   *serializedInvocationInfo,
     /* IN */     Int32         invocationInfoLen,
     /* OUT */    Int32        *invocationInfoLenOut,
     /* IN */     const char   *serializedPlanInfo,
     /* IN */     Int32         planInfoLen,
     /* IN */     Int32         planNum,
     /* OUT */    Int32        *planInfoLenOut,
     /* IN */     char         *inputParamRow,
     /* IN */     Int32         inputParamRowLen,
     /* OUT */    char         *outputRow,
     /* IN */     Int32         outputRowLen,
     /* IN/OUT */ ComDiagsArea *da)
{
  LmResult result = LM_OK;

  // initialize out parameters
  *invocationInfoLenOut = 0;
  *planInfoLenOut = 0;

  // some parameter checks
  if (invocationInfoLen <= 0 &&
      invocationInfo_ == NULL)
    {
      // we need to have an Invocation info
      *da << DgSqlCode(-LME_COMPILER_INTERFACE_ERROR)
          << DgString0(getNameForDiags())
          << DgString1(tmudr::UDRInvocationInfo::callPhaseToString(
                            tmudr::UDRInvocationInfo::COMPILER_INITIAL_CALL))
          << DgString2("LmRoutineCppObj::invokeRoutineMethod()")
          << DgString3("Missing UDRInvocationInfo");
    }

  try
    {
      if (invocationInfoLen > 0)
        {
          if (invocationInfo_ == NULL)
            invocationInfo_ = new tmudr::UDRInvocationInfo;

          // unpack the invocation info
          invocationInfo_->deserializeObj(serializedInvocationInfo,
                                          invocationInfoLen);
        }

      if (planInfoLen > 0)
        {
          if (!planInfos_.used(planNum))
            planInfos_.insertAt(planNum, new tmudr::UDRPlanInfo(invocationInfo_,
                                                                planNum));

          // unpack the invocation info
          planInfos_[planNum]->deserializeObj(serializedPlanInfo,
                                              planInfoLen);
        }

      // some parameter checks
      if (inputParamRowLen < inputParamRowLen_)
        return LM_ERR;
      // test to do for scalar UDFs
      // if (outputRowLen < invocationInfo_->out().getRecordLength())
      //   return LM_ERR;

      if (inputParamRow && inputParamRowLen_ > 0)
        // copy parameter row
        memcpy(invocationInfo_->par().getRowPtr(), inputParamRow, inputParamRowLen_);

      invocationInfo_->callPhase_ = phase;
      switch (phase)
        {
        case tmudr::UDRInvocationInfo::COMPILER_INITIAL_CALL:
          if (invocationInfo_->getDebugFlags() &
              tmudr::UDRInvocationInfo::PRINT_INVOCATION_INFO_INITIAL)
            invocationInfo_->print();

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
               *planInfos_[planNum]);
          break;

        case tmudr::UDRInvocationInfo::COMPILER_PLAN_CALL:
          interfaceObj_->describePlanProperties(*invocationInfo_,
                                                *planInfos_[planNum]);
          break;

        case tmudr::UDRInvocationInfo::COMPILER_COMPLETION_CALL:
          interfaceObj_->completeDescription(*invocationInfo_,
                                             *planInfos_[planNum]);
          if (invocationInfo_->getDebugFlags() &
              tmudr::UDRInvocationInfo::PRINT_INVOCATION_INFO_END_COMPILE)
            {
              invocationInfo_->print();
              printf("\n");
              for (CollIndex i=0; i<planInfos_.getUsedLength(); i++)
                if (planInfos_.used(i))
                  {
                    if (i == planNum)
                      printf("++++++++++ Chosen plan: ++++++++++\n");
                    else
                      printf("-------- Plan not chosen: --------\n");
                    planInfos_[i]->print();
                  }
            }

          break;

        case tmudr::UDRInvocationInfo::RUNTIME_WORK_CALL:
          {
            if (invocationInfo_->getDebugFlags() &
                tmudr::UDRInvocationInfo::PRINT_INVOCATION_INFO_AT_RUN_TIME)
              {
                invocationInfo_->print();
                planInfos_[planNum]->print();
              }

#ifndef NDEBUG
            if ((invocationInfo_->getDebugFlags() &
                 tmudr::UDRInvocationInfo::DEBUG_INITIAL_RUN_TIME_LOOP_ALL) ||
                (invocationInfo_->getDebugFlags() &
                 tmudr::UDRInvocationInfo::DEBUG_INITIAL_RUN_TIME_LOOP_ONE &&
                 invocationInfo_->getMyInstanceNum() == 0))
              interfaceObj_->debugLoop();
#endif

            interfaceObj_->processData(*invocationInfo_,
                                       *planInfos_[planNum]);

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

      // return length of updated invocation and plan info for
      // compile-time phases
      if (invocationInfo_ &&
          phase < tmudr::UDRInvocationInfo::RUNTIME_WORK_CALL)
        {
          *invocationInfoLenOut = invocationInfo_->serializedLength();
          if (planInfos_.used(planNum))
            *planInfoLenOut = planInfos_[planNum]->serializedLength();
        }
    }
  catch (tmudr::UDRException e)
    {
      // Check the returned SQLSTATE value and raise appropriate
      // SQL code. Valid SQLSTATE values begin with "38" except "38000"
      const char *sqlState = e.getSQLState();

      if ((strncmp(sqlState, "38", 2) == 0) &&
          (strncmp(sqlState, "38000", 5) != 0))
        {
          *da << DgSqlCode(-LME_CUSTOM_ERROR)
              << DgString0(e.getText().c_str())
              << DgString1(sqlState);
          *da << DgCustomSQLState(sqlState);
        }
      else
        {
          *da << DgSqlCode(-LME_UDF_ERROR)
              << DgString0(invocationInfo_->getUDRName().c_str())
              << DgString1(sqlState)
              << DgString2(e.getText().c_str());
        }
      result = LM_ERR;
    }
  catch (...)
    {
      *da << DgSqlCode(-LME_COMPILER_INTERFACE_ERROR)
          << DgString0(getNameForDiags())
          << DgString1(invocationInfo_->callPhaseToString(phase))
          << DgString2("LmRoutineCppObj::invokeRoutineMethod()")
          << DgString3("general exception");
      result = LM_ERR;
    }

  invocationInfo_->callPhase_ =
    tmudr::UDRInvocationInfo::UNKNOWN_CALL_PHASE;

  return result;
}

LmResult LmRoutineCppObj::getRoutineInvocationInfo(
     /* IN/OUT */ char         *serializedInvocationInfo,
     /* IN */     Int32         invocationInfoMaxLen,
     /* OUT */    Int32        *invocationInfoLenOut,
     /* IN/OUT */ char         *serializedPlanInfo,
     /* IN */     Int32         planInfoMaxLen,
     /* IN */     Int32         planNum,
     /* OUT */    Int32        *planInfoLenOut,
     /* IN/OUT */ ComDiagsArea *da)
{
  LmResult result = LM_OK;

  // Retrieve updated invocation and plan info.
  // The invokeRoutineMethod provided the required buffer
  // space, so there is no excuse for having insufficient
  // buffer when calling this, and doing so will raise
  // an exception in the try block below.
  try
    {
      if (invocationInfo_ && invocationInfoMaxLen > 0)
        {
          char *tempiibuf = serializedInvocationInfo;
          int tempiilen   = invocationInfoMaxLen;
          *invocationInfoLenOut = invocationInfo_->serialize(
               tempiibuf, tempiilen);
        }
      else
        *invocationInfoLenOut = 0;

      if (planInfos_.used(planNum) && planInfoMaxLen > 0)
        {
          char *temppibuf = serializedPlanInfo;
          int temppilen   = planInfoMaxLen;
          *planInfoLenOut = planInfos_[planNum]->serialize(
               temppibuf, temppilen);
        }
      else
        *planInfoLenOut = 0;
    }
  catch (tmudr::UDRException e)
    {
      // this UDRException is generated by Trafodion code and
      // it is an internal error to fail serializing the structs,
      // even though, the user might have caused it by corrupting
      // these structs
      *da << DgSqlCode(-LME_COMPILER_INTERFACE_ERROR)
          << DgString0(getNameForDiags())
          << DgString1(invocationInfo_->callPhaseToString(invocationInfo_->getCallPhase()))
          << DgString2("LmRoutineCppObj::getRoutineInvocationInfo()")
          << DgString3("e.getText().c_str()");
      result = LM_ERR;
    }
  catch (...)
    {
      *da << DgSqlCode(-LME_COMPILER_INTERFACE_ERROR)
          << DgString0(getNameForDiags())
          << DgString1(invocationInfo_->callPhaseToString(invocationInfo_->getCallPhase()))
          << DgString2("LmRoutineCppObj::getRoutineInvocationInfo()")
          << DgString3("general exception");
      result = LM_ERR;
    }

  return result;
}

void LmRoutineCppObj::setFunctionPtrs(SQLUDR_GetNextRow getNextRowPtr,
                                      SQLUDR_EmitRow emitRowPtr)
{
  interfaceObj_->getNextRowPtr_ = getNextRowPtr;
  interfaceObj_->emitRowPtr_    = emitRowPtr;
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
