#ifndef LMROUTINEC_H
#define LMROUTINEC_H
/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutineC.h
* Description:  LmRoutine for C routines
* Created:      05/22/2008
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

#include "LmRoutine.h"
#include "LmLangManagerC.h"
#include "LmParameter.h"
#include "sqludr.h"

//////////////////////////////////////////////////////////////////////
//
// LmRoutineC
//
// The LmRoutineC class maintains state of a C routine. LmRoutineC is
// an abstract class. Different concrete subclasses will exist for
// different parameter passing styles.
//
//////////////////////////////////////////////////////////////////////
class SQLLM_LIB_FUNC LmRoutineC : public LmRoutine
{
  friend class LmLanguageManagerC;
  
public:

  LmRoutineC(const char   *sqlName,
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
             Int32 routineOwnerId,
             const char   *parentQid,
             ComUInt32    inputParamRowLen,
             ComUInt32    outputRowLen,
             const char   *currentUserName,
             const char   *sessionUserName,
             LmParameter  *parameters,
             LmLanguageManagerC *lm,
             LmHandle     routine,
             LmContainer  *container,
             ComDiagsArea *diagsArea);
  
  virtual ~LmRoutineC();

  // The following pure virtual methods must be implemented even
  // though we do not currently support result sets for C routines
  void cleanupLmResultSet(LmResultSet *resultSet,
                          ComDiagsArea *diagsArea = NULL) {}
  void cleanupLmResultSet(ComUInt32 index,
                          ComDiagsArea *diagsArea = NULL) {}
  void cleanupResultSets(ComDiagsArea *diagsArea = NULL) {}

  // callType_ is primarily managed by the LmRoutineC object.
  // However when the LM caller is making a FINAL call, the caller
  // needs to let LmRoutineC know by calling setFinalCall().
  void setFinalCall() { callType_ = SQLUDR_CALLTYPE_FINAL; }

  void setExternalName(const char *name, ComUInt32 nameLen);

  void setNumPassThroughInputs(ComUInt32 numPassThroughInputs);
  void setPassThroughInput(ComUInt32 index, void *data, ComUInt32 dataLen);

  virtual LmResult handleFinalCall(ComDiagsArea *diagsArea = NULL);

protected:

  LmResult processReturnStatus(ComSInt32 result, ComDiagsArea *diags);

  void deletePassThroughInputs();

  static SQLUDR_CHAR *host_data_;     // Static data member

  SQLUDR_CHAR         sqlState_[SQLUDR_SQLSTATE_SIZE];
  SQLUDR_CHAR         msgText_[SQLUDR_MSGTEXT_SIZE];
  SQLUDR_UINT16       callType_;
  SQLUDR_STATEAREA   *stateArea_;
  SQLUDR_UDRINFO     *udrInfo_;

  NABoolean finalCallRequired_;

}; // class LmRoutineC

#endif
