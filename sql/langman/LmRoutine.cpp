/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutine.cpp
* Description:  LmRoutine class
* Created:      09/02/2009
* Language:     C++
*
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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

#include "LmRoutine.h"
#include "LmLangManager.h"
#include "LmCommon.h"
#include "ComObjectName.h"

LmRoutine::LmRoutine(LmHandle container,
                     LmHandle routine,
                     const char *sqlName,
                     const char *externalName,
                     const char *librarySqlName,
                     ComUInt32 numParam,
                     ComUInt32 maxResultSets,
                     ComRoutineLanguage language,
                     ComRoutineParamStyle paramStyle,
                     ComRoutineTransactionAttributes transactionAttrs,
                     ComRoutineSQLAccess sqlAccessMode,
                     ComRoutineExternalSecurity externalSecurity,
                     Int32 routineOwnerId,
                     const char *parentQid,
                     Int32 inputParamRowLen,
                     Int32 outputRowLen,
                     const char   *currentUserName,
                     const char   *sessionUserName,
                     LmParameter *lmParams,
                     LmLanguageManager *lm)
  : container_(container),
    routine_(routine),
    externalName_(NULL),
    numSqlParam_(numParam),
    maxResultSets_(maxResultSets),
    lm_(lm),
    sqlName_(NULL),
    librarySqlName_(NULL),
    numParamsInSig_(0),
    resultSetList_(collHeap()),
    parentQid_(NULL),
    language_(language),
    paramStyle_(paramStyle),
    transactionAttrs_(transactionAttrs),
    sqlAccessMode_(sqlAccessMode),
    externalSecurity_ (externalSecurity),
    routineOwnerId_ (routineOwnerId),
    inputParamRowLen_(inputParamRowLen),
    outputRowLen_(outputRowLen),
    lmParams_(lmParams),
    udrCatalog_(NULL),
    udrSchema_(NULL)
{
  // Make copies of some of the strings passed in
  if (externalName)
    externalName_ = copy_and_pad(externalName, strlen(externalName), 8);
  if (sqlName)
    sqlName_ = copy_and_pad(sqlName, strlen(sqlName), 8);
  if (librarySqlName)
    librarySqlName_ = copy_and_pad(librarySqlName, strlen(librarySqlName), 8);
  if (parentQid)
    parentQid_ = copy_and_pad(parentQid, strlen(parentQid), 8);

  // Use a ComObjectName object to parse the SQL name and store copies
  // of the catalog and schema names
  NAString sqlNameStr(sqlName);
  ComObjectName objName(sqlNameStr);
  const NAString &cat = objName.getCatalogNamePartAsAnsiString();
  const NAString &sch = objName.getSchemaNamePartAsAnsiString();
  if (!cat.isNull())
    udrCatalog_ = copy_string(collHeap(), cat.data());
  if (!sch.isNull())
    udrSchema_ = copy_string(collHeap(), sch.data());
}

LmRoutine::~LmRoutine()
{
  if (externalName_)
    free(externalName_);
  if (sqlName_)
    free(sqlName_);
  if (librarySqlName_)
    free(librarySqlName_);
  if (parentQid_)
    free((char *) parentQid_);

  if (udrCatalog_)
    NADELETEBASIC(udrCatalog_, collHeap());
  if (udrSchema_)
    NADELETEBASIC(udrSchema_, collHeap());
}

LmResult LmRoutine::invokeRoutineMethod(
     /* IN */     tmudr::UDRInvocationInfo::CallPhase phase,
     /* IN */     const char   *serializedInvocationInfo,
     /* IN */     Int32         invocationInfoLen,
     /* OUT */    Int32        *invocationInfoLenOut,
     /* IN */     const char   *serializedPlanInfo,
     /* IN */     Int32         planInfoLen,
     /* IN */     Int32         planNum,
     /* OUT */    Int32        *planInfoLenOut,
     /* IN */     char         *inputRow,
     /* IN */     Int32         inputRowLen,
     /* OUT */    char         *outputRow,
     /* IN */     Int32         outputRowLen,
     /* IN/OUT */ ComDiagsArea *da)
{
  LM_ASSERT(0); // should not call this method on the base class
  return LM_ERR;
}

void LmRoutine::setRuntimeInfo(
       const char   *parentQid,
       int           totalNumInstances,
       int           instanceNum)
{
  LM_ASSERT(0); // should not call this method on the base class
}

LmResult LmRoutine::getRoutineInvocationInfo
(
     /* IN/OUT */ char         *serializedInvocationInfo,
     /* IN */     Int32         invocationInfoMaxLen,
     /* OUT */    Int32        *invocationInfoLenOut,
     /* IN/OUT */ char         *serializedPlanInfo,
     /* IN */     Int32         planInfoMaxLen,
     /* IN */     Int32         planNum,
     /* OUT */    Int32        *planInfoLenOut,
     /* IN/OUT */ ComDiagsArea *da)
{
  LM_ASSERT(0); // should not call this method on the base class
  return LM_ERR;
}

const char *LmRoutine::getNameForDiags()
{
  return (lm_->getCommandLineMode() ? externalName_ : sqlName_);
}
