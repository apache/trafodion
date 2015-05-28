#ifndef LMLANGMANAGERC_H
#define LMLANGMANAGERC_H
/* -*-C++-*-
**********************************************************************
*
* File:         LmLangManagerC.h
* Description:  Language Manager for C definitions
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

#include "LmLangManager.h"

//////////////////////////////////////////////////////////////////////
//
// Forward Reference Classes
//
//////////////////////////////////////////////////////////////////////
class ComDiagsArea;
class LmContainerManager;
class LmRoutine;
class LmRoutineC;
class LmRoutineSas;
class LmParameter;

//////////////////////////////////////////////////////////////////////
//
// Contents
//
//////////////////////////////////////////////////////////////////////
class LmLanguageManagerC;
class LmCLoader;


//////////////////////////////////////////////////////////////////////
//
// LmLanguageManagerC
//
// The LmLanguageManagerC is a concrete class implementing the LM
// for C (LMC). 
//
// The LMC constructor has the following parameters:
//   result:    Constructor result, LM_OK is returned upon success.
//   diagsArea: Diagnostics area [NULL]
//
//////////////////////////////////////////////////////////////////////
class SQLLM_LIB_FUNC LmLanguageManagerC : public LmLanguageManager
{
friend class LmRoutineC;
friend class LmRoutineSas;

public:
  LmLanguageManagerC(LmResult &result,
                     NABoolean commandLineMode = FALSE,
                     ComDiagsArea *diagsArea = NULL);
  
  ~LmLanguageManagerC();

  virtual ComRoutineLanguage getLanguage() const { return COM_LANGUAGE_C; }

  // LM service methods. All service methods take optional ComDiagsArea
  virtual LmResult validateRoutine(
    ComUInt32     numParam,
    ComFSDataType paramType[],
    ComUInt32     paramSubType[],
    ComColumnDirection direction[],
    const char    *routineName,
    const char    *containerName,
    const char    *externalPath,
    char          *sigBuf,
    ComUInt32     sigLen,
    ComFSDataType resultType = COM_UNKNOWN_FSDT,
    ComUInt32     resultSubType = 0,
    ComUInt32     numResultSets = 0,
    const char    *metaContainerName = NULL,
    const char    *optionalSig = NULL,
    ComDiagsArea  *diagsArea = NULL);

  virtual LmResult getRoutine(
    ComUInt32    numParam,
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
    Int32 routineOwnerId,
    LmRoutine    **handle,
    LmHandle    getNextRowPtr,
    LmHandle    emitRowPtr,
    ComUInt32    maxResultSets,
    ComDiagsArea *diagsArea = NULL);

  virtual LmResult putRoutine(
    LmRoutine *handle,
    ComDiagsArea *diagsArea = NULL);

  virtual LmResult invokeRoutine(
    LmRoutine    *handle,
    void         *inputRow,
    void         *outputRow,
    ComDiagsArea *diagsArea = NULL);

  // Container Manager support methods.
  virtual LmHandle createLoader(
    const char *externalPath,
    ComDiagsArea *diags);

  virtual void deleteLoader(LmHandle extLoader);

  virtual LmHandle loadContainer(
    const char   *containerName,
    const char   *externalPath,
    LmHandle     extLoader,
    ComUInt32    *containerSize,
    ComDiagsArea *diagsArea);

  virtual void unloadContainer(LmHandle cont);

  virtual const char* containerExtension()
    { return "so"; }

  virtual LmResult getSystemProperty(
    const char   *key,
    char         *value,
    ComUInt32    bufferLen,
    ComBoolean   &propertyIsSet,
    ComDiagsArea *diagsArea = NULL);

  virtual LmResult setSystemProperty(
    const char   *key,
    const char   *value,
    ComDiagsArea *diagsArea = NULL);

private:
  LmContainerManager *contManager_; // LMJ's CM.
  ComDiagsArea* diagsArea_;	    // Diagnostics Area passed from client

}; // class LmLanguageManagerC

// LmCLoader: This is an empty class. This class is only there so
// that the LmMetaContainer objects can be used for LMC & LMJava with
// minimal changes
class LmCLoader : public NABasicObject
{
public:
  LmCLoader();
  ~LmCLoader();
private:
};
#endif
