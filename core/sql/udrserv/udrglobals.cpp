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
 *****************************************************************************
 *
 * File:         udrglobals.cpp
 * Description:  Class declaration for UDR globals
 *
 *
 * Created:      4/10/2001
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "spinfo.h"
#include "udrglobals.h"
#include "LmLangManagerJava.h"
#include "LmLangManagerC.h"
#include "udrdecs.h"
#include "Measure.h"
#include "Platform.h"
#include "UdrFFDC.h"
#include "LmJavaOptions.h"
#include "ComRtUtils.h"
#include "OperTypeEnum.h"
#include "UdrDebug.h"
#include "exp_function.h"
#include "JavaObjectInterface.h"
#include "Globals.h"

#include "udrdefs.h"

#ifndef MAXOF
#define MAXOF(X,Y) (X >= Y ? X : Y)
#endif

extern void ServerDebug(const char *, ...);

extern NABoolean getDefineSetting(const char *, char *, short *);

UdrGlobals::UdrGlobals(NAHeap *udrheap, NAHeap *ipcheap)
     :  udrHeap_(udrheap)
       ,ipcHeap_(ipcheap)
       ,ipcEnv_(NULL)
       ,ctrlConn_(NULL)
       ,replyStreams_(ipcheap)
       ,commandLineMode_(FALSE)
       ,cliSqlViolation_(FALSE)
       ,cliXactViolation_(FALSE)
       ,cliSqlError_(FALSE)
       ,cliXactWasAborted_(FALSE)
       ,currentMsgSize_(0)
       ,exitPrintPid_(TRUE)
       ,gNumOpeners_(0)
       ,logFileProvided_(FALSE)
       ,nextUniqueIdentifier_(1)
       ,objectCount_(0)
       ,replyCount_(0)
       ,requestCount_(0)
       ,showInvoke_(FALSE)
       ,showLoad_(FALSE)
       ,showMain_(FALSE)
       ,showSPInfo_(FALSE)
       ,showUnload_(FALSE)
       ,showRSLoad_(FALSE)
       ,showRSFetch_(FALSE)
       ,showRSContinue_(FALSE)
       ,showRSClose_(FALSE)
       ,showRSUnload_(FALSE)
       ,traceLevel_(0)
       ,javaLanguageManager_(NULL)
       ,cLanguageManager_(NULL)
       ,javaOptions_(NULL)
       ,verbose_(FALSE)
       ,isoMapping_(CharInfo::DefaultCharSet)

       // statistics
       ,numReqUDR_(0)
       ,numErrUDR_(0)
       ,numReqSP_(0)
       ,numErrSP_(0)
       ,numReqDataSP_(0)
       ,numErrDataSP_(0)

       ,numReqLoadSP_(0)
       ,numReqInvokeSP_(0)
       ,numReqContinueSP_(0)
       ,numReqUnloadSP_(0)

       ,numErrLoadSP_(0)
       ,numErrInvokeSP_(0)
       ,numErrContinueSP_(0)
       ,numErrUnloadSP_(0)

       ,numErrLMCall_(0)
       ,numErrCLICall_(0)

       ,numTotalSPs_(0)
       ,numCurrSPs_(0)
       ,numTotalRSets_(0)
       ,numCurrRSets_(0)
       ,currSP_(NULL)
{
  Int32 i;
  Int32 j;
  for (i=0; i<UDRMAXOPENERS_V100; i++)
    XPROCESSHANDLE_NULLIT_(&(gOpenerPhandle_[i].phandle_));

  str_cpy_all(serverName_, "tdm_udrserv", str_len("tdm_udrserv"));
  serverName_[str_len("tdm_udrserv")] = '\0';

  // Establish UDR Server process environment
  ipcEnv_ = new (ipcHeap_) IpcEnvironment(ipcHeap_);

  ctrlConn_ = new (ipcEnv_) UdrGuaControlConnection(ipcEnv_, this);
  ipcEnv_->setControlConnection(ctrlConn_);

  spList_ = new (udrHeap_) SPList(this);

  // save the =_SQL_MX_ISO_MAPPING value in Udr globals.
  isoMapping_ = (CharInfo::CharSet) ComRtGetIsoMappingEnum();
  UDR_DEBUG1("Charset : %s", CharInfo::getCharSetName(isoMapping_)); 

  // Retrieve the current role name and user name. We tolerate errors
  // in these steps for the following reasons:
  // * SPJ does not require these lookups.
  // * UDF does require them but is not yet externalized.
  // * On NSK development systems the user name lookup can fail
  //   when an LDAP server is not available.
  // * We don't want MXUDR to abend simply because the LDAP lookup
  //   fails. That might break SPJ tests.

  Lng32 username_len = 0;
  Lng32 rolename_len = 0;
  short status = 0;

  // Retrieve the CURRENT_USER name
  status = exp_function_get_user (ITM_CURRENT_USER,
                                  currentUserName_,
                                  MAX_USER_NAME_LENGTH + 1,
                                  &username_len
                                  );
  if (status != FEOK) {
    UDR_DEBUG1("*** WARNING: exp_function_get_user for CURRENT_USER returned error %d",
               (Int32) status);
    sprintf(currentUserName_, "UNKNOWN");
  }
  strcpy(currentRoleName_, currentUserName_);

  // Retrieve the SESSION_USER name
  username_len = 0;
  status = exp_function_get_user (ITM_SESSION_USER,
                                  sessionUserName_,
                                  MAX_USER_NAME_LENGTH + 1,
                                  &username_len
                                  );
  if (status != FEOK) {
    UDR_DEBUG1("*** WARNING: exp_function_get_user for SESSION_USER returned error %d",
               (Int32) status);
    sprintf(sessionUserName_, "UNKNOWN");
  }

  UDR_DEBUG1("Current Role : %s", currentRoleName_); 
  UDR_DEBUG1("Current User : %s", currentUserName_); 
  UDR_DEBUG1("Session User : %s", sessionUserName_); 
}

// we don't have any way of invoking these at this point.
// we should provide a way for procedure bodies to set a java property
// that the language manager can use to check to determine if it should
// display/zero stats.
void UdrGlobals::resetAllStats()
{
 numReqUDR_ = 0;
 numErrUDR_ = 0;
 numReqSP_ = 0;
 numErrSP_ = 0;
 numReqDataSP_ = 0;
 numErrDataSP_ = 0;

 numReqLoadSP_ = 0;
 numReqInvokeSP_ = 0;
 numReqContinueSP_ = 0;
 numReqUnloadSP_ = 0;

 numErrLoadSP_ = 0;
 numErrInvokeSP_ = 0;
 numErrContinueSP_ = 0;
 numErrUnloadSP_ = 0;

 numErrLMCall_ = 0;
 numErrCLICall_ = 0;

 numTotalSPs_ = 0;
 numCurrSPs_ = 0;
 numTotalRSets_ = 0;
 numCurrRSets_ = 0;

};

void UdrGlobals::displayStats(ostream& out, Lng32 indent)
{
  char ind[100];

  Lng32 indmax = (indent > 99) ? 99 : indent;

  Lng32 indIdx = 0;
  for (indIdx = 0; indIdx < indmax; indIdx++)
	  ind[indIdx] = ' ';
  ind[indIdx] = '\0';

  ServerDebug(" ");
  ServerDebug("%sUDR Server Statistics:", ind );
  ServerDebug("%s---------------------", ind );
  ServerDebug("%sNum Req UDR        : " INT64_SPEC , ind, numReqUDR_ );
  ServerDebug("%sNum Err UDR        : " INT64_SPEC , ind, numErrUDR_ );
  ServerDebug("%sNum Req SP         : " INT64_SPEC , ind, numReqSP_ );
  ServerDebug("%sNum Err SP         : " INT64_SPEC , ind, numErrSP_ );
  ServerDebug("%sNum Req Data SP    : " INT64_SPEC , ind, numReqDataSP_ );
  ServerDebug("%sNum Err Data SP    : " INT64_SPEC , ind, numErrDataSP_ );

  ServerDebug("%sNum Req Load SP    : " INT64_SPEC , ind, numReqLoadSP_ );
  ServerDebug("%sNum Req Invoke SP  : " INT64_SPEC , ind, numReqInvokeSP_ );
  ServerDebug("%sNum Req Continue SP: " INT64_SPEC , ind, numReqContinueSP_ );
  ServerDebug("%sNum Req Unload SP  : " INT64_SPEC , ind, numReqUnloadSP_ );

  ServerDebug("%sNum Err Load SP    : " INT64_SPEC , ind, numErrLoadSP_ );
  ServerDebug("%sNum Err Invoke SP  : " INT64_SPEC , ind, numErrInvokeSP_ );
  ServerDebug("%sNum Err Continue SP: " INT64_SPEC , ind, numErrContinueSP_ );
  ServerDebug("%sNum Err Unload SP  : " INT64_SPEC , ind, numErrUnloadSP_ );

  ServerDebug("%sNum Err LM Call    : " INT64_SPEC , ind, numErrLMCall_ );
  ServerDebug("%sNum Err CLI Call   : " INT64_SPEC , ind, numErrCLICall_ );
  ServerDebug("%sNum Total SPs      : " INT64_SPEC , ind, numTotalSPs_ );
  ServerDebug("%sNum Curr  SPs      : " INT64_SPEC , ind, numCurrSPs_ );
  ServerDebug("%sNum Total RSets    : " INT64_SPEC , ind, numTotalRSets_ );
  ServerDebug("%sNum Curr RSets     : " INT64_SPEC , ind, numCurrRSets_ );
}


LmLanguageManager *UdrGlobals::getLM(ComRoutineLanguage language) const
{
  switch (language)
  {
    case COM_LANGUAGE_JAVA:
      return getJavaLM();

    case COM_LANGUAGE_C:
    case COM_LANGUAGE_CPP:
      return getCLM();

    default:
      UDR_ASSERT(0, "Request for LM for unknown language is encountered.");
      return NULL;
  }
}

LmLanguageManager *UdrGlobals::getOrCreateLM(LmResult &result,
                                             ComRoutineLanguage language,
                                             ComDiagsArea *diags)
{
  switch (language)
  {
    case COM_LANGUAGE_JAVA:
      return getOrCreateJavaLM(result, diags);

    case COM_LANGUAGE_C:
    case COM_LANGUAGE_CPP:
      return getOrCreateCLM(result, diags);

    default:
      UDR_ASSERT(0, "Request to create LM for unknown language is encountered.");
      result = LM_ERR;
      return NULL;
  }
}

// small helper class to use functions in JavaObjectInterface
class UdrGlobalsJavaObj : public JavaObjectInterface
{
public: 

  UdrGlobalsJavaObj(NAHeap *heap) : JavaObjectInterface(heap, (jobject) -1) {}
  JOI_RetCode initJVMForUDRServ(LmJavaOptions *options);
};

JOI_RetCode UdrGlobalsJavaObj::initJVMForUDRServ(LmJavaOptions *options)
{
  return initJVM(options);
}

LmLanguageManagerJava *UdrGlobals::getOrCreateJavaLM(LmResult &result,
                                                     ComDiagsArea *diags)
{
  result = LM_OK;

  if (javaLanguageManager_)
    return javaLanguageManager_;

  // Note: For trusted UDRs, we use a T2 driver to do SQL from
  // UDRs. This means that the UDR server contains the executor
  // code. If we do SQL and that SQL needs another JVM, we need to
  // share the JVM (and also the LmLanguageManagerJava). Therefore,
  // create these objects in the executor and CLI globals.
  TIMER_ON(initLangmanTimer);

  // create a JavaObjectInterface, just so we can call its method
  // to create or attach to a JVM
  UdrGlobalsJavaObj helper(udrHeap_);

  helper.initJVMForUDRServ(getJavaOptions());

  // now that we know that a JVM exists, create the language manager
  // in the CLI globals
  CliGlobals *cliGlobals = GetCliGlobals();

  if (!cliGlobals)
    cliGlobals = CliGlobals::createCliGlobals(TRUE);

  javaLanguageManager_ = cliGlobals->getLanguageManagerJava();

  TIMER_OFF(initLangmanTimer, "Call Langman Constructor (trusted)");
  LOG_FLUSH;

  // At a later time, we may want to use a T4 driver for isolated
  // UDRs and create a stand-alone LmLanguageManagerJava here,
  // using the code below.
  //
  // ComUInt32 lmJavaMax = 1;
  //
  // TIMER_ON(initLangmanTimer)
  // javaLanguageManager_ = new (getUdrHeap())
  //   LmLanguageManagerJava(result, commandLineMode_, lmJavaMax,
  //                         getJavaOptions(), diags);
  //
  // TIMER_OFF(initLangmanTimer, "Call Langman Constructor (isolated)")
  // LOG_FLUSH
  //
  // // Make sure LM pointer and result are consistent. Either we return
  // // a NULL pointer and an error result, or a non-NULL pointer and
  // // LM_OK.
  //
  // if (javaLanguageManager_ && result != LM_OK)
  // {
  //   delete javaLanguageManager_;
  //   javaLanguageManager_ = NULL;
  // }

  if (!javaLanguageManager_)
    result = LM_ERR;

  return javaLanguageManager_;

} // UdrGlobals::getOrCreateJavaLM()

void UdrGlobals::destroyJavaLM()
{
  if (javaLanguageManager_)
  {
    delete javaLanguageManager_;
    javaLanguageManager_ = NULL;
  }
}

void UdrGlobals::destroyJavaLMAndJVM()
{
  destroyJavaLM();
  LmLanguageManagerJava::destroyVM();
}

LmJavaOptions *UdrGlobals::getJavaOptions()
{
  if (javaOptions_ == NULL)
  {
    javaOptions_ = new (getUdrHeap()) LmJavaOptions();
  }
  return javaOptions_;
}

LmLanguageManagerC *UdrGlobals::getOrCreateCLM(LmResult &result,
                                               ComDiagsArea *diags)
{
  if (cLanguageManager_)
  {
    result = LM_OK;
    return cLanguageManager_;
  }

  cLanguageManager_ = new (getUdrHeap())
    LmLanguageManagerC(result, commandLineMode_, diags);

  if (cLanguageManager_ && result != LM_OK)
  {
    delete cLanguageManager_;
    cLanguageManager_ = NULL;
  }

  if (!cLanguageManager_)
    result = LM_ERR;

  return cLanguageManager_;
}

void UdrGlobals::destroyCLM()
{
  if (cLanguageManager_)
  {
    delete cLanguageManager_;
    cLanguageManager_ = NULL;
  }
}

