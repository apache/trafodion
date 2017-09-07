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
* File:         udrunload.cpp
* Description:  This is the module that processes UDR Unload messages.
*               The tasks for this process are to :
*               . Extract UDR Handle attributes from message
*               . Locate UDR Handle SPInfo data structure
*               . Call Language Manager to put the SP Routine handle
*               . Destroy SPInfo context for the SP
*               . Handle errors returned from LM.
*
* Created:      01/01/2001
* Language:     C++
*
*
*****************************************************************************
*/
#undef  UDRUNLOAD_INSTANTIATE
#define UDRUNLOAD_INSTANTIATE

#include "udrextrn.h"
#include "UdrStreams.h"
#include "udrdefs.h"
#include "udrutil.h"
#include "UdrExeIpc.h"

#include "udrdecs.h"

void processAnUnLoadMessage(UdrGlobals *UdrGlob,
                            UdrServerReplyStream &msgStream,
                            UdrUnloadMsg &request)
{

  Lng32 error;
  const char *moduleName = "processAnUnLoadMessage";
  char errorText[MAXERRTEXT];

  ComDiagsArea *diags = ComDiagsArea::allocate(UdrGlob->getIpcHeap());

  doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
               UdrGlob->showLoad_, moduleName);

  if (UdrGlob->verbose_ &&
      UdrGlob->traceLevel_ >= TRACE_IPMS &&
      UdrGlob->showUnload_)
  {
    ServerDebug("[UdrServ (%s)]  Receive Unload Request", moduleName);
  }

  // UDR UNLOAD comes outside of Enter Tx and Exit Tx pair. Make sure
  // the correct tx is activated, if it had come with a tx.
  msgStream.activateCurrentMsgTransaction();

  // Find the SPInfo instance associated with the incoming UDR handle
  SPInfo *sp = UdrGlob->getSPList()->spFind(request.getHandle());

  if (sp == NULL)
  {
// LCOV_EXCL_START
    //
    // No SPInfo exists for this UDR handle. Could be because the
    // handle is invalid or because this is an out-of-sequence UNLOAD
    // request. If the handle is invalid that is an error. If the
    // UNLOAD is out-of-sequence, we generate a warning and also
    // create a dummy SPInfo instance so that when the LOAD arrives we
    // know to ignore it.
    //
    if (UdrHandleIsValid(request.getHandle()))
    {
      char buf[100];
      convertInt64ToAscii(request.getHandle(), buf);
      *diags << DgSqlCode(UDR_ERR_UNEXPECTED_UNLOAD);
      *diags << DgString0(buf);
      sp = new (UdrGlob->getUdrHeap()) SPInfo(UdrGlob,
                                              UdrGlob->getUdrHeap(),
                                              request.getHandle());
      sp->setSPInfoState(SPInfo::UNLOADING);
    }
    else
    {
      *diags << DgSqlCode(-UDR_ERR_MISSING_UDRHANDLE);
      *diags << DgString0("Unload Message");
    }
  } // if (sp == NULL)
// LCOV_EXCL_STOP

  if (sp)
  {
    LmRoutine *lmr = sp->getLMHandle();
    if (lmr == NULL && sp->isLoaded())
    {
      *diags << DgSqlCode(-UDR_ERR_MISSING_LMROUTINE);
      *diags << DgString0("Unload Message");
    }

    if (UdrGlob->verbose_ &&
        UdrGlob->traceLevel_ >= TRACE_IPMS &&
        UdrGlob->showUnload_)
    {
      ServerDebug("[UdrServ (%s)]  Call SPInfo::releaseSP",
                  moduleName);
    }
    
    //
    // releaseSP will contact Language Manager to unload the SP and
    // will also invoke the SPInfo destructor.
    //
    error = sp->releaseSP(TRUE, *diags);
    sp = NULL;

    if (error != 0)
// LCOV_EXCL_START
    {
      sprintf(errorText,
              "(%.30s) UDR Unload Error: %d",
              moduleName, (Lng32) error);
      ServerDebug("[UdrServ (%s)]  %s\n",
                  "releaseSP", errorText);
      doMessageBox(UdrGlob, TRACE_SHOW_DIALOGS,
                   UdrGlob->showLoad_,
                   errorText);
    }
// LCOV_EXCL_STOP
  }

  //
  // Build a reply and send it
  //
  msgStream.clearAllObjects();
  UdrUnloadReply *reply = new (UdrGlob->getIpcHeap())
    UdrUnloadReply(UdrGlob->getIpcHeap());

  if (reply == NULL)
  {
    controlErrorReply(UdrGlob, msgStream,
                      UDR_ERR_MESSAGE_PROCESSING,
                      INVOKE_ERR_NO_REPLY_BUFFER,
                      NULL);
    return;
  }

  reply->setHandle(request.getHandle());

  msgStream << *reply;

  if (diags && diags->getNumber() > 0)
  {
    msgStream << *diags;
    UdrGlob->numErrUDR_++;
    UdrGlob->numErrSP_++;
    UdrGlob->numErrUnloadSP_++;
  }

  if (UdrGlob->verbose_ &&
      UdrGlob->traceLevel_ >= TRACE_IPMS &&
      UdrGlob->showUnload_)
  {
    ServerDebug("[UdrServ (%s)] Send Unload Reply", moduleName);
  }

#ifdef _DEBUG
  if (UdrGlob && UdrGlob->getJavaLM())
  {
    sleepIfPropertySet(*(UdrGlob->getJavaLM()),
                       "MXUDR_UNLOAD_DELAY", diags);
  }
#endif // _DEBUG

  sendControlReply(UdrGlob, msgStream, NULL);

  if (diags)
  {
    diags->decrRefCount();
  }

  reply->decrRefCount();

} // processAnUnLoadMessage
