/* -*-C++-*-
**********************************************************************
*
* File:         LmLangManagerC.cpp
* Description:  Language Manager for C
*
* Created:      05/15/2008
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

#include "LmCommon.h"
#include "LmLangManagerC.h"
#include "LmContManager.h"
#include "LmRoutineC.h"
#include "LmRoutineCSql.h"
#include "LmRoutineCSqlRow.h"
#include "LmRoutineCSqlRowTM.h"
#include "LmRoutineCppObj.h"
#include "LmExtFunc.h"
#include "LmDebug.h"
#include "sqludr.h"

LmLanguageManagerC::LmLanguageManagerC(
  LmResult &result,
  NABoolean commandLineMode,
  ComDiagsArea *diagsArea)
  : LmLanguageManager(commandLineMode),
    diagsArea_(diagsArea)
{
  setRoutineIsActive(FALSE);
  contManager_ = new (collHeap()) LmContainerManagerSimple(this);
  result = LM_OK;
}

LmLanguageManagerC::~LmLanguageManagerC()
{
  delete contManager_;
}

LmResult LmLanguageManagerC::validateRoutine(
  ComUInt32     numSqlParam,
  ComFSDataType paramType[],
  ComUInt32     paramSubType[],
  ComColumnDirection direction[],
  const char    *routineName,
  const char    *containerName,
  const char    *externalPath,
  char          *sigBuf,
  ComUInt32     sigLen,
  ComFSDataType resultType,
  ComUInt32     resultSubType,
  ComUInt32     numResultSets,
  const char    *metaContainerName,
  const char    *optionalSig,
  ComDiagsArea  *diagsArea)
{
  return LM_OK;
}

LmResult LmLanguageManagerC::getRoutine(
  ComUInt32    numSqlParam,
  LmParameter  parameters[],
  ComUInt32   numTableInfo,
  LmTableInfo tableInfo[],
  LmParameter  *returnValue,
  ComRoutineParamStyle paramStyle,
  ComRoutineTransactionAttributes transactionAttrs,
  ComRoutineSQLAccess sqlAccessMode,
  tmudr::UDRInvocationInfo *invocationInfo,
  tmudr::UDRPlanInfo       *planInfo,
  const char   *parentQid,
  ComUInt32    inputRowLen,
  ComUInt32    outputRowLen,
  const char   *sqlName,
  const char   *externalName,
  const char   *routineSig,
  const char   *containerName,
  const char   *externalPath,
  const char   *librarySqlName,
  const char   *currentUserName,
  const char   *sessionUserName,
  ComRoutineExternalSecurity externalSecurity,
  Int32        routineOwnerId,
  LmRoutine    **handle,
  LmHandle     getNextRowPtr,
  LmHandle     emitRowPtr,
  ComUInt32    maxResultSets,
  ComDiagsArea *diagsArea)
{
  *handle = NULL;
  LmContainer *container = NULL;
  LmResult result = LM_OK;
  const char *operation = "dlsym";

  ComDiagsArea *da = (diagsArea != NULL) ? diagsArea : diagsArea_;

  // Get the requested container from the CM.
  result = contManager_->getContainer(containerName, externalPath,
                                      &container, da);
  if (result == LM_ERR)
    return LM_ERR;
  
  // Get a handle to the requested routine
  LmHandle routinePtr = NULL;

  if (paramStyle != COM_STYLE_CPP_OBJ)
    {
      routinePtr = getRoutinePtr(container->getHandle(), externalName);

      if (routinePtr == NULL)
        {
          char *libraryName = new (collHeap())
            char[str_len(externalPath) + str_len(containerName) + 2];
          sprintf(libraryName, "%s/%s", externalPath, containerName);

          *da << DgSqlCode(-LME_DLL_METHOD_NOT_FOUND)
              << DgString0(externalName)
              << DgString1(libraryName);

          addDllErrors(*da, operation, FALSE);

          NADELETEBASIC(libraryName, collHeap());
          return LM_ERR;
        }
    }

  // allocate an LM handle for the external method.
  LmRoutine *routineHandle = NULL;
  if (paramStyle == COM_STYLE_SQL)
  {
    routineHandle =
      new (collHeap()) LmRoutineCSql(sqlName,
                                     externalName,
                                     librarySqlName,
                                     numSqlParam,
                                     (char *)routineSig,
                                     maxResultSets,
                                     transactionAttrs,
                                     sqlAccessMode,
                                     externalSecurity,
                                     routineOwnerId,
                                     parentQid,
                                     inputRowLen,
                                     outputRowLen,
                                     currentUserName,
                                     sessionUserName,
                                     parameters,
                                     this,
                                     routinePtr,
                                     container,
                                     da);
  }
  else if (paramStyle == COM_STYLE_SQLROW)
  {
    routineHandle =
      new (collHeap()) LmRoutineCSqlRow(sqlName,
                                        externalName,
                                        librarySqlName,
                                        numSqlParam,
                                        (char *)routineSig,
                                        maxResultSets,
                                        transactionAttrs,
                                        sqlAccessMode,
                                        externalSecurity,
                                        routineOwnerId,
                                        parentQid,
                                        inputRowLen,
                                        outputRowLen,
                                        currentUserName,
                                        sessionUserName,
                                        parameters,
                                        this,
                                        routinePtr,
                                        container,
                                        da);
  }
  else if (paramStyle == COM_STYLE_SQLROW_TM)
  {
    routineHandle =
      new (collHeap()) LmRoutineCSqlRowTM(sqlName,
                                        externalName,
                                        librarySqlName,
                                        numSqlParam,
                                        numTableInfo,
                                        tableInfo,
                                        (char *)routineSig,
                                        maxResultSets,
                                        transactionAttrs,
                                        sqlAccessMode,
                                        externalSecurity,
                                        routineOwnerId,
                                        parentQid,
                                        inputRowLen,
                                        outputRowLen,
                                        currentUserName,
                                        sessionUserName,
                                        parameters,
                                        this,
                                        routinePtr,
                                        getNextRowPtr,
                                        emitRowPtr,
                                        container,
                                        da);
  }
  else if (paramStyle == COM_STYLE_CPP_OBJ)
  {
    char factoryMethodName[500];
    char libraryName[5000];

    snprintf(factoryMethodName,
             sizeof(factoryMethodName),
             "%s",
             externalName);
    snprintf(libraryName, sizeof(libraryName), "%s/%s", externalPath, containerName);

    // search the DLL for the method that creates the tmudr::UDRInterface object
    LmHandle factoryMethodPtr = getRoutinePtr(container->getHandle(), factoryMethodName);
    tmudr::UDR *interfacePtr = NULL;

    if (factoryMethodPtr == NULL)
        {
          *da << DgSqlCode(-LME_DLL_METHOD_NOT_FOUND)
              << DgString0(factoryMethodName)
              << DgString1(libraryName);

          addDllErrors(*da, operation, FALSE);

          return LM_ERR;
        }

    try
      {
        tmudr::CreateInterfaceObjectFunc fPtr =
          reinterpret_cast<tmudr::CreateInterfaceObjectFunc>(factoryMethodPtr);

        // call the factory method
        interfacePtr = (*fPtr)();
      }
    catch (tmudr::UDRException e)
      {
          *da << DgSqlCode(-LME_FACTORY_METHOD)
              << DgString0(factoryMethodName)
              << DgString1(libraryName)
              << DgString2(e.getText().data());
          return LM_ERR;
      }
    catch (...)
      {
          *da << DgSqlCode(-LME_FACTORY_METHOD)
              << DgString0(factoryMethodName)
              << DgString1(libraryName)
              << DgString2("general exception");
          return LM_ERR;
      }

    if (interfacePtr == NULL)
      {
        *da << DgSqlCode(-LME_FACTORY_METHOD)
            << DgString0(factoryMethodName)
            << DgString1(libraryName)
            << DgString2("method returned NULL");
        return LM_ERR;
      }

    // create the language manager routine, which will take
    // ownership of the interface object
    routineHandle =
      new (collHeap()) LmRoutineCppObj(invocationInfo,
                                       planInfo,
                                       interfacePtr,
                                       sqlName,
                                       externalName,
                                       librarySqlName,
                                       numSqlParam,
                                       numTableInfo,
                                       tableInfo,
                                       (char *)routineSig,
                                       maxResultSets,
                                       transactionAttrs,
                                       sqlAccessMode,
                                       externalSecurity,
                                       routineOwnerId,
                                       parentQid,
                                       inputRowLen,
                                       outputRowLen,
                                       currentUserName,
                                       sessionUserName,
                                       parameters,
                                       this,
                                       routinePtr,
                                       getNextRowPtr,
                                       emitRowPtr,
                                       container,
                                       da);
  }
  else 
  {
    // XXX LM_ASSERT(0);
    char *paramStyleMsg = new (collHeap())
      char[100];
    sprintf(paramStyleMsg, "Unknown ParameterStyle(%d)", paramStyle);

    *da << DgSqlCode(-LME_VALIDATION_FAILED)
        << DgString0(externalName)
        << DgString1(paramStyleMsg);

    addDllErrors(*da, operation, FALSE);
  }

  // Verify the handle.
  if (routineHandle == NULL)
  {
    // DiagsArea is already filled
    if (container)
      contManager_->putContainer(container);
    return LM_ERR;
  }
  else
  {
    *handle = routineHandle;
    return LM_OK;
  }
}

LmResult LmLanguageManagerC::putRoutine(
  LmRoutine    *handle,
  ComDiagsArea *diagsArea)
{
  if (handle == NULL)
    return LM_OK;

  LmResult result = LM_OK;

  // For now we assume all C routine bodies require a FINAL call. In
  // the future we can make the FINAL optional, controlled by a flag
  // in the LmRoutine instance.
  LmRoutineC *routine = (LmRoutineC *) handle;
  routine->setFinalCall();
  result = routine->invokeRoutine(NULL, NULL, diagsArea);


  // De-ref the container.
  if (routine->container())
    contManager_->putContainer(routine->container());
  
  // De-allocate the handle.
  delete routine;

  return result;
}

LmResult LmLanguageManagerC::invokeRoutine(
  LmRoutine    *handle,
  void         *inputRow,
  void         *outputRow,
  ComDiagsArea *diagsArea)
{
  LmRoutine *routine = (LmRoutine *) handle;
  LM_ASSERT(routine);
  LmResult result = LM_OK;

  if (routine->getParamStyle() == COM_STYLE_CPP_OBJ)
    result = static_cast<LmRoutineCppObj *>(routine)->invokeRoutineMethod(
         tmudr::UDRInvocationInfo::RUNTIME_WORK_CALL,
         inputRow,
         diagsArea);
  else
    result = routine->invokeRoutine(inputRow, outputRow, diagsArea);

  return result;
}

LmHandle LmLanguageManagerC::createLoader(
  const char   *externalPath,
  ComDiagsArea *da)
{
  return new (collHeap()) LmCLoader();
}

void LmLanguageManagerC::deleteLoader(LmHandle loader)
{
  // This function deletes LmCLoader object
  LmCLoader *cLoader = (LmCLoader *) loader;
  delete cLoader;
}

LmHandle LmLanguageManagerC::loadContainer(
  const char   *containerName,
  const char   *externalPath,
  LmHandle     extLoader,
  ComUInt32    *containerSize,
  ComDiagsArea *da)
{
  return loadDll(containerName, externalPath, extLoader, 
                          containerSize, da, collHeap());
}

void LmLanguageManagerC::unloadContainer(LmHandle containerHandle)
{
  unloadDll(containerHandle, diagsArea_);
}

LmResult LmLanguageManagerC::getSystemProperty(
  const char   *key,
  char         *value,
  ComUInt32    bufferLen,
  ComBoolean   &propertyIsSet,
  ComDiagsArea *diagsArea)
{
  // TBD: Need to update this with getenv() function call
  propertyIsSet = FALSE;
  return LM_OK;
}

LmResult LmLanguageManagerC::setSystemProperty(
  const char   *key,
  const char   *value,
  ComDiagsArea *diagsArea)
{
  // TBD: Need to update this with setenv() function call
  return LM_OK;
}

LmCLoader::LmCLoader()
{
}

LmCLoader::~LmCLoader()
{
}
