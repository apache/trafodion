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
* File:         UdrRSProcess.cpp
* Description:  This module contains all the main methods for
*               processing Result Set related messages.
*
* Created:      10/12/2005
* Language:     C++
*
*
*****************************************************************************
*/

#include "udrextrn.h"
#include "udrglobals.h"
#include "UdrExeIpc.h"
#include "UdrStreams.h"
#include "udrutil.h"
#include "UdrResultSet.h"


void processAnRSLoadMessage(UdrGlobals *udrGlob,
                            UdrServerReplyStream &msgStream,
                            UdrRSLoadMsg &request)
{
  const char* moduleName = "processAnRSLoadMessage";

  ComDiagsArea *diags = ComDiagsArea::allocate(udrGlob->getIpcHeap());

  doMessageBox(udrGlob, TRACE_SHOW_DIALOGS,
               udrGlob->showRSLoad_, moduleName);

  NABoolean showRSLoadLogic = (udrGlob->verbose_ &&
                               udrGlob->traceLevel_ >= TRACE_IPMS &&
                               udrGlob->showRSLoad_);

  if (showRSLoadLogic)
  {
    ServerDebug("[UdrServ (%s)]  Receive RS Load Request", moduleName);
  }

  // process RS Load request
  SPInfo *sp = udrGlob->getSPList()->spFind(request.getHandle());
  if (sp == NULL)
  {
    controlErrorReply(udrGlob, msgStream, UDR_ERR_MISSING_UDRHANDLE,
                      0, "RS Load Message");
    return;
  }

  // Check if RS index in the request is valid. The RS index above UDR
  // server is 1-based.
  if (request.getRsIndex() < 1 ||
      request.getRsIndex() > sp->getNumResultSets())
  {
    controlErrorReply(udrGlob, msgStream, UDR_ERR_INVALID_RS_INDEX,
                      request.getRsIndex(), "Load");
    return;
  }

  // Activate SP's transaction
  sp->activateTransaction();

  sp->loadUdrResultSet(request.getRsIndex() - 1,
                       request.getRSHandle(),
		       request.getNumRSColumns(),
		       request.getRowSize(),
		       request.getBufferSize(),
		       request.getColumnDesc(),
                       *diags);

  // Prepare reply and send it
  msgStream.clearAllObjects();

  UdrRSLoadReply *reply = new (udrGlob->getIpcHeap())
     UdrRSLoadReply(udrGlob->getIpcHeap());

  if (reply == NULL)
  { // no reply msg built...
    controlErrorReply(udrGlob, msgStream, UDR_ERR_MESSAGE_PROCESSING,
                      RS_ERR_NO_REPLY_MSG, NULL);
    return;
  }

  // If diags contains errors, set UDR and RS handles to invalid values
  if (diags && diags->getNumber(DgSqlCode::ERROR_) > 0)
  {
    reply->setHandle(INVALID_UDR_HANDLE);
    reply->setRSHandle(INVALID_RS_HANDLE);
  }
  else
  {
    reply->setHandle(request.getHandle());
    reply->setRSHandle(request.getRSHandle());
  }

  msgStream << *reply;

  if (diags && diags->getNumber() > 0)
  {
    if (diags->getNumber(DgSqlCode::ERROR_) > 0)
    {
      udrGlob->numErrUDR_++;
      udrGlob->numErrSP_++;
      udrGlob->numErrRSLoad_++;
    }

    msgStream << *diags;
    if (showRSLoadLogic)
      dumpDiagnostics(diags, 2);
  }

  if (showRSLoadLogic)
  {
    ServerDebug("[UdrServ (%s)] About to send RS LOAD reply", moduleName);
  }

  sendControlReply(udrGlob, msgStream, sp);

  if (diags)
  {
    diags->decrRefCount();
  }

  reply->decrRefCount();

} // processAnRSLoadMessage()

void processAnRSFetchOrContinueMessage(UdrGlobals *udrGlob,
                                       UdrServerDataStream &msgStream,
				       UdrHandle udrHandle,
				       RSHandle rsHandle,
				       UdrDataBuffer *request,
                                       UdrIpcObjectType requestType)
{
  const char* moduleName = "processAnRSFetchOrContinueMessage";

  // This method should only be called for RS invoke or RS continue
  // message types.
  NABoolean show = FALSE;
  if (requestType == UDR_MSG_RS_DATA_HEADER)
    show = udrGlob->showRSFetch_;
  else if (requestType == UDR_MSG_RS_CONTINUE)
    show = udrGlob->showRSContinue_;
  else
    UDR_ASSERT(0, "Request Type can only be Udr RS Data Header or Continue");

  ComDiagsArea *diags = ComDiagsArea::allocate(udrGlob->getIpcHeap());
  UDR_ASSERT(diags, "Unable to allocate memory for SQL diagnostics area");
  doMessageBox(udrGlob, TRACE_SHOW_DIALOGS, show, moduleName);

  NABoolean doTrace = (udrGlob->verbose_ &&
                       udrGlob->traceLevel_ >= TRACE_IPMS &&
                       show) ? TRUE : FALSE;

  if (doTrace)
    ServerDebug("[UdrServ (%s)]  Receive RS Invoke or Continue Request",
                moduleName);

  // Free up any receive buffers no longer in use
  msgStream.cleanupBuffers();

  SPInfo *sp = udrGlob->getSPList()->spFind(udrHandle);
  if (sp == NULL)
  {
    if (requestType == UDR_MSG_RS_DATA_HEADER)
      dataErrorReply(udrGlob, msgStream, UDR_ERR_MISSING_UDRHANDLE,
                     0, "RS Fetch Message");
    else
      dataErrorReply(udrGlob, msgStream, UDR_ERR_MISSING_UDRHANDLE,
                     0, "RS Continue Message");

    sendDataReply(udrGlob, msgStream, NULL);

    if (diags)
      diags->decrRefCount();

    return;
  }

  UdrResultSet *udrRS = sp->getUdrResultSetByHandle(rsHandle);
  if (udrRS == NULL)
  {
    if (requestType == UDR_MSG_RS_DATA_HEADER)
      dataErrorReply(udrGlob, msgStream, UDR_ERR_MISSING_RSHANDLE,
                     (Lng32)rsHandle, "Fetch");
    else
      dataErrorReply(udrGlob, msgStream, UDR_ERR_MISSING_RSHANDLE,
                     (Lng32)rsHandle, "Continue");

    sendDataReply(udrGlob, msgStream, NULL);

    if (diags)
      diags->decrRefCount();

    return;
  }

  // We do nothing if this is an RS_CONTINUE and RS is already closed
  // or fetch is complete
  if (requestType == UDR_MSG_RS_CONTINUE &&
      (udrRS->isClosed() || udrRS->isFetchComplete()))
  {
    if (diags)
      diags->decrRefCount();

    return;
  }

  // For FETCH: RS state should be LOADED or REINITIATED
  // For CONTINUE: RS state should be FETCH or EARLY_CLOSE
  if ((requestType == UDR_MSG_RS_DATA_HEADER &&
       (! udrRS->isLoaded() && ! udrRS->isReInited()))  ||
      (requestType == UDR_MSG_RS_CONTINUE &&
       (! udrRS->isFetchStarted() && ! udrRS->isEarlyClose())))
  {
    // TBD: This does not work correctly. The error takes two char params
    // but we are sending only one. Need to fix this.
    dataErrorReply(udrGlob,
                   msgStream,
                   UDR_ERR_INVALID_RS_STATE,
                   0,
                   udrRS->stateString());

    sendDataReply(udrGlob, msgStream, NULL);

    if (diags)
      diags->decrRefCount();

    return;
  }

  // Allocate reply message object
  UdrDataBuffer *reply = new (msgStream, udrRS->getBufferSize())
    UdrDataBuffer(udrRS->getBufferSize(),
		  UdrDataBuffer::UDR_DATA_OUT,
		  NULL);
  if (reply == NULL || reply->getSqlBuffer() == NULL)
  {  // could not allocate UdrDataBuffer
    dataErrorReply(udrGlob, msgStream, UDR_ERR_MESSAGE_PROCESSING,
                   RS_ERR_NO_REPLY_DATA_BUFFER, NULL);

    sendDataReply(udrGlob, msgStream, NULL);

    if (reply)
      reply->decrRefCount();
      
    if (diags)
      diags->decrRefCount();

    return;
  }

  // Activate SP's transaction
  sp->activateTransaction();

  // Allocate a diags list where row warning diags will be stored.
  NAList<ComDiagsArea *> *rowDiagsList =
    new (udrGlob->getIpcHeap()) NAList<ComDiagsArea *>(udrGlob->getIpcHeap());

  Lng32 numRows = udrRS->fetchRows(udrGlob,
                                  request ? request->getSqlBuffer() : NULL,
                                  reply->getSqlBuffer(),
                                  *diags,
                                  rowDiagsList);
  if (numRows == -1)
  {
    // $$$$ TBD:
    // The following is temporary. We should copy ERROR/EOD data rows
    // for any error during fetching.
    dataErrorReply(udrGlob,
                   msgStream,
                   UDR_ERR_MESSAGE_PROCESSING,
                   RS_ERR_NO_REPLY_DATA_BUFFER,
                   NULL);

    sendDataReply(udrGlob, msgStream, NULL);

    if (reply)
      reply->decrRefCount();

    // $$$$ TBD. Somehow we should send diags to our caller. For now
    // we simply release them.
    if (diags)
      diags->decrRefCount();

    if (rowDiagsList)
    {
      for (ComUInt32 i = 0; i < rowDiagsList->entries(); i++)
        (*rowDiagsList)[i]->decrRefCount();

      delete rowDiagsList;
      rowDiagsList = NULL;
    }

    return;
  }

  if (rowDiagsList)
  {
    for (ComUInt32 i = 0; i < rowDiagsList->entries(); i++)
    {
      msgStream << *(*rowDiagsList)[i];
      (*rowDiagsList)[i]->decrRefCount();
    }

    delete rowDiagsList;
    rowDiagsList = NULL;
  }

  if (diags && diags->getNumber() > 0)
  {
    msgStream << *diags;
    if (doTrace)
      dumpDiagnostics(diags, 2);

    if (diags->getNumber(DgSqlCode::ERROR_) > 0)
    {
      udrGlob->numErrUDR_++;
      udrGlob->numErrSP_++;
      if (requestType == UDR_MSG_RS_DATA_HEADER)
        udrGlob->numErrRSFetch_++;
      if (requestType == UDR_MSG_RS_CONTINUE)
        udrGlob->numErrRSContinue_++;
    }
  }

  if (doTrace)
  {
    ComUInt32 replyRowLen = udrRS->getExeRowSize();
    ServerDebug("");
    ServerDebug("[UdrServ (%s)] RS reply SqlBuffer", moduleName);
    ServerDebug("[UdrServ (%s)] Exe row size %u", moduleName, replyRowLen);
    displaySqlBuffer(reply->getSqlBuffer(),
                     (Lng32) reply->getSqlBufferLength());
  }

  // Finally set the flag if this is the last buffer.
  if (udrRS->isFetchComplete())
    reply->setLastBuffer(TRUE);

  sendDataReply(udrGlob, msgStream, sp);

  if (diags)
  {
    diags->decrRefCount();
  }
} // processAnRSInvokeOrContinueMessage()

void processAnRSFetchMessage(UdrGlobals *udrGlob,
                             UdrServerDataStream &msgStream,
			     UdrHandle udrHandle,
			     RSHandle rsHandle,
                             UdrDataBuffer *request)
{
  processAnRSFetchOrContinueMessage(udrGlob,
			  	    msgStream,
				    udrHandle,
				    rsHandle,
				    request,
				    UDR_MSG_RS_DATA_HEADER);
  return;
}

void processAnRSContinueMessage(UdrGlobals *udrGlob,
                                UdrServerDataStream &msgStream,
                                UdrRSContinueMsg &request)
{
  // We don't need to provide an input databuffer in the 5th parameter
  // for the following method call for RS_CONTINUE message. This is because
  // the required information is already stored in UdrResultSet during
  // Fetch request processing.
  processAnRSFetchOrContinueMessage(udrGlob,
                                    msgStream,
			            request.getHandle(),
			            request.getRSHandle(),
				    NULL,
				    UDR_MSG_RS_CONTINUE);
  return;
}

void processAnRSCloseMessage(UdrGlobals *udrGlob,
                             UdrServerReplyStream &msgStream,
                             UdrRSCloseMsg &request)
{
  const char* moduleName = "processAnRSCloseMessage";

  ComDiagsArea *diags = ComDiagsArea::allocate(udrGlob->getIpcHeap());
  UDR_ASSERT(diags, "Unable to allocate memory for SQL diagnostics area");

  doMessageBox(udrGlob, TRACE_SHOW_DIALOGS,
               udrGlob->showRSClose_, moduleName);

  NABoolean doTrace = (udrGlob->verbose_ &&
                       udrGlob->traceLevel_ >= TRACE_IPMS &&
                       udrGlob->showRSClose_) ? TRUE : FALSE;
  if (doTrace)
  {
    ServerDebug("[UdrServ (%s)]  Receive RS Close Request", moduleName);
  }

  // Find the SPInfo instance for this request
  SPInfo *sp = udrGlob->getSPList()->spFind(request.getHandle());
  if (sp == NULL)
  {
    controlErrorReply(udrGlob, msgStream, UDR_ERR_MISSING_UDRHANDLE,
                      0, "RS Close Message");
    return;
  }

  // Find the UdrResultSet instance for this request
  UdrResultSet *udrRS = sp->getUdrResultSetByHandle(request.getRSHandle());
  if (udrRS == NULL)
  {
    controlErrorReply(udrGlob, msgStream, UDR_ERR_MISSING_RSHANDLE,
                      (Lng32)request.getRSHandle(), "RS Close Message");
    return;
  }

  // Use SP's transaction if it had come within Enter Tx and Exit Tx pair
  // Else, use msg's transaction
  if (! sp->activateTransaction())
    msgStream.activateCurrentMsgTransaction();

  // Let the UdrResultSet instance process this RS CLOSE request
  udrRS->processRSClose(diags);

  // Build a reply & send it
  msgStream.clearAllObjects();

  UdrRSCloseReply *reply = new (udrGlob->getIpcHeap())
    UdrRSCloseReply(udrGlob->getIpcHeap());
  if (reply == NULL)
  {
    controlErrorReply(udrGlob, msgStream, UDR_ERR_MESSAGE_PROCESSING,
                      RS_ERR_NO_REPLY_MSG, NULL);
    return;
  }

  // If diags contains errors, set UDR and RS handles to invalid values
  if (diags && diags->getNumber(DgSqlCode::ERROR_) > 0)
  {
    reply->setHandle(INVALID_UDR_HANDLE);
    reply->setRSHandle(INVALID_RS_HANDLE);
  }
  else
  {
    reply->setHandle(request.getHandle());
    reply->setRSHandle(request.getRSHandle());
  }

  msgStream << *reply;

  if (diags && diags->getNumber() > 0)
  {
    if (diags->getNumber(DgSqlCode::ERROR_) > 0)
    {
      udrGlob->numErrUDR_++;
      udrGlob->numErrSP_++;
      udrGlob->numErrRSClose_++;
    }

    msgStream << *diags;
    if (doTrace)
      dumpDiagnostics(diags, 2);
  }

  if (doTrace)
  {
    ServerDebug("[UdrServ (%s)] Send RS Close Reply", moduleName);
  }

  sendControlReply(udrGlob, msgStream, sp);

  if (diags)
  {
    diags->decrRefCount();
  }

  reply->decrRefCount();

} // processAnRSCloseMessage()

void processAnRSUnloadMessage(UdrGlobals *udrGlob,
                              UdrServerReplyStream &msgStream,
                              UdrRSUnloadMsg &request)
{
  const char* moduleName = "processAnRSUnloadMessage";

  doMessageBox(udrGlob, TRACE_SHOW_DIALOGS,
               udrGlob->showRSUnload_, moduleName);

  if (udrGlob->verbose_ &&
      udrGlob->traceLevel_ >= TRACE_IPMS &&
      udrGlob->showRSUnload_)
  {
    ServerDebug("[UdrServ (%s)]  Receive RS Unload Request", moduleName);
  }

  // Find the SPInfo instance for this request
  SPInfo *sp = udrGlob->getSPList()->spFind(request.getHandle());
  if (sp == NULL)
  {
    controlErrorReply(udrGlob, msgStream, UDR_ERR_MISSING_UDRHANDLE,
                      0, "RS Unload Message");
    return;
  }

  // Find the UdrResultSet instance for this request
  UdrResultSet *udrRS = sp->getUdrResultSetByHandle(request.getRSHandle());
  if (udrRS == NULL)
  {
    controlErrorReply(udrGlob, msgStream, UDR_ERR_MISSING_RSHANDLE,
                      (Lng32)request.getRSHandle(), "Unload");
    return;
  }

  // Use SP's transaction if it had come within Enter Tx and Exit Tx pair
  // Else, use msg's transaction
  if (! sp->activateTransaction())
    msgStream.activateCurrentMsgTransaction();

  // Unload RS
  udrRS->unload();

  // Build a reply & send it
  msgStream.clearAllObjects();

  UdrRSUnloadReply *reply = new (udrGlob->getIpcHeap())
    UdrRSUnloadReply(udrGlob->getIpcHeap());

  if (reply == NULL)
  {
    controlErrorReply(udrGlob, msgStream, UDR_ERR_MESSAGE_PROCESSING,
                      RS_ERR_NO_REPLY_MSG, NULL);
    return;
  }

  reply->setHandle(request.getHandle());
  reply->setRSHandle(request.getRSHandle());

  msgStream << *reply;

  if (udrGlob->verbose_ &&
      udrGlob->traceLevel_ >= TRACE_IPMS &&
      udrGlob->showUnload_)
  {
    ServerDebug("[UdrServ (%s)] Send RS Unload Reply", moduleName);
  }

  sendControlReply(udrGlob, msgStream, sp);

  reply->decrRefCount();
} // processAnRSUnloadMessage()
