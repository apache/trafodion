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
 * File:         ExSqlComp.h
 * Description:  Declaration of the object ExSqlComp for executor to create
 *               compiler process, build up the connection and process the 
 *               requests.
 *               
 *               
 * Created:      07/15/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#ifndef EX_SQLCOMP_H
#define EX_SQLCOMP_H

#include "Ipc.h"
#include "Int64.h"
#include "ex_god.h"
#include "CmpMessage.h"
class ComDiagsArea;
class CmpMessageObj;
class CmpMessageStream;
class ExStoredProcTcb;
class CliGlobals;
class ContextCli;
class ExControlArea;

class ExSqlComp : public ExGod
{
private:
  NABoolean isShared_;
  ContextCli * lastContext_;
  NABoolean resendingControls_;
  short compilerVersion_;
  char *nodeName_;
public:
  NABoolean isShared() { return isShared_; }
  void setShared(NABoolean isShared) { isShared_ = isShared; }
  ContextCli * getLastContext() { return lastContext_; }
  void setLastContext(ContextCli *context) { lastContext_ = context; }
  static void appendControls(ExControlArea *dest, ExControlArea *src);
  void completeRequests();
  
friend class CmpMessageStream;

public:

  enum ReturnStatus { SUCCESS, WARNING, ERROR, MOREDATA };

  enum OperationStatus
  { INIT,	// request just being created
    PENDING,	// the request has been sent correctly
    FINISHED,	// the reply has arrived from arkcmp, ready to be fetched
    FETCHED	// the reply has been fetched already by the caller
  };

  #define EXSQLCOMP	CmpMessageObj
  typedef CmpMessageObj::MessageTypeEnum Operator;
  
  // In the constructor, the server (arkcmp) will be created, 
  // the connection will also be established.
  ExSqlComp
    (void* ex_environment,
     CollHeap *h,
     CliGlobals *cliGlobals,
     ExStoredProcTcb *storedProcTcb = NULL,
     short compilerVersion = COM_VERS_COMPILER_VERSION,
     char *nodeName_ = NULL,
     IpcEnvironment *env = NULL);

  // requests processing, return a serial number of the request
  ReturnStatus sendRequest
    (Operator, const char* input_data=0, ULng32 size=0, 
     NABoolean waited=TRUE, Int64* id=0, Lng32 charset=SQLCHARSETCODE_UNKNOWN,
     NABoolean resendFlg = FALSE,
     const char *parentQid = NULL,
     Lng32 parentQidLen = 0);

  // requests for processing Internal SP
  // for InternalSP execution, executor should do the following: 
  // sendRequest(CmpMessageISPRequest, , id );
  // while (getReply(reply, , id ) == MOREDATA)
  //   getNext(id);
  //
  // sendRequest sends the CmpMessageISPRequest to arkcmp for execution.
  ReturnStatus sendRequest
    (const char* procName = 0, // ISP name, null terminated
     void* inputExpr = 0, ULng32 inputExprSize = 0, // input expr
     void* outputExpr = 0, ULng32 outputExprSize = 0, // output expr
     void* keyExpr = 0, ULng32 keyExprSize = 0, // keys expr
     void* inputData = 0, ULng32 inputDataSize = 0, // input data
     ULng32 outputRecSize = 0, ULng32 outputTotalSize = 0, // output data
     NABoolean waited=TRUE, Int64* id=0,
     const char *parentQid = NULL,
     Lng32 parentQidLen = 0);
  // send a CmpMessageISPGetNext request with the outstanding request (id)
  // if id is 0 , use the current outstanding one.
  ReturnStatus getNext ( ULng32 bufSize, Int64 id=0, NABoolean waited= TRUE, 
  const char *parentQid = NULL, Lng32 parentQidLen = 0);
  
  // status of the request with id asspecified. If id=0, the current outstanding
  // request is checked for the status.
  OperationStatus status(Int64 id=0);
  
  // Method to get the reply for the request with id as specified. If id=0, it is to get
  // the reply for current outstanding request.
  // reply is the place to hold the contents of reply code ( or result ). It could be
  // 1. preallocated by the caller and passed in. maxSize is the size allocated. 
  // 2. if 0 ( a null pointer ) is passed in, getReply method will allocate the space.
  //    reply will points to the sotrage allocated, size will be the size allocated.
  // 3. if the maxSize is not big enough to hold the reply, getReply method will allocate
  //    the storage to hold the reply, in this case reply will be overwritten with the space
  //    allocated, size will be the the size of the actual storage allocated.
  // The reply will be allocated from the heap passed in ExSqlComp::ExSqlComp constructor.
  // returns SUCCESS if this is the last reply
  // returns ERROR if error.
  // returns MOREDATA if there is more data to be retrieved for this request
  // in the case of MOREDATA, getNext(reqId) should be 
  // called again to retrieve data.

  ReturnStatus getReply(char*& reply, ULng32& size,
			ULng32 maxSize=0, Int64 id=0 /* the request id returned previously */,
			NABoolean getDataWithErrReply = FALSE);
   
  // get the diagnostics area, this area will be clean up in the next
  // sendRequest() call.
  ComDiagsArea* getDiags(Int64 id=0);

  // take the diagnostics area, it is the user's responsibility to clean up
  // the ComDiagsArea.
  ComDiagsArea* takeDiags(Int64 id=0);

  void clearDiags();

  // In the destructor, the connection will be disconnected, then the
  // server will be destroyed.
  virtual ~ExSqlComp();

  // end connection with arkcmp. Kill it.
  void endConnection();

  ReturnStatus changePriority(Lng32 IpcPriority, NABoolean isDelta);

  inline CollHeap * getHeap() {return h_; };

  NABoolean isConnected() {return (sqlcompMessage_ != NULL);};
  short getVersion() {return compilerVersion_;}
  char *getNodeName() {return nodeName_;} 
  void setNodeName(char *nodeName) {nodeName_ = nodeName;}
  inline NABoolean badConnection() {return badConnection_; }
  inline NABoolean breakReceived() { return breakReceived_; }
  inline IpcServer *getServer() { return server_; }
  inline Int64 getRecentIpcTimestamp() { return recentIpcTimestamp_; }

  ReturnStatus setDefaultCatAndSch();

private:
  ExSqlComp(const ExSqlComp&);
  ExSqlComp& operator=(const ExSqlComp&);

  // Some helper routines for internal usage

  NABoolean getEnvironment(char* &data, ULng32 &size);

  // preSendRequest is to initialize the objects for each request.
  ReturnStatus preSendRequest(NABoolean doRefreshEnvs);

  // sendRequest is to send the CmpMessageObj to arkcmp.
  ReturnStatus sendRequest(CmpMessageObj*, NABoolean w=TRUE);

  // sendR is to actual call the IPC routine to send the CmpMessageObj.
  ReturnStatus sendR(CmpMessageObj*, NABoolean w=TRUE);
  inline ReturnStatus waitForReply();
  //inline 
    ReturnStatus refreshEnvs();

  ReturnStatus resetAllDefaults();
  ReturnStatus resetRemoteDefaults();
  ReturnStatus resendControls(NABoolean ctxSw = FALSE);

  NABoolean error(Lng32);

  // start MXCCMP
  ReturnStatus startSqlcomp();
  // server creation
  ReturnStatus createServer();
  // connection establishment
  ReturnStatus establishConnection();
  // resend the request 
  ReturnStatus resendRequest();
  // delete the internal structure for server/connection
  void deleteServerStruct();

  CollHeap * h_;
  CliGlobals *cliGlobals_;
  IpcServerAllocationMethod allocMethod_;
  IpcEnvironment *env_;
  IpcServerClass *sc_;
  IpcServer *server_;
  CmpMessageStream *sqlcompMessage_;
  ExStoredProcTcb *storedProcTcb_;
  CmpMessageConnectionType::ConnectionTypeEnum connectionType_;

  void* exEnvironment_; // unknown, to be supported by executor.
  NABoolean doRefreshEnvironment_;
 
  ComDiagsArea* diagArea_;

  // Requests is a structure to hold the current outstanding requests in 
  // ExSqlComp.
  struct Requests 
  {
    // request objects.
    CmpMessageObj* message_;
    // after the actOnReceive method, message_ is deleted, the requestId_ will be set to
    // the id of the message to be compared in getReply method.
    Int64 requestId_;  // after the actOnReceive, message_ is deleted, the id 
    NABoolean waited_; // waited_ flag for this request.
    OperationStatus ioStatus_;
    Int32 resendCount_;
  };
  
  void initRequests(Requests&);

  // This is to trace the ISP execution request, since there could be multiple
  // replys for the ISP request, the ID needs to be sent over to identify
  // the request.
  Int64 currentISPRequest_;

  // TODO, The outstandingSendBuffers should be an array of Requests
  // lists of outstanding send and receive buffers(send and receive queues)
  // in the order in which their I/O operations have been initiated. 
  // Currently only one outstanding I/O is supported in one ExSqlComp object, 
  // so there is only one element in outstandingSendBuffer_;
  Requests outstandingSendBuffers_;
  NABoolean badConnection_;
  NABoolean breakReceived_;
 
  // for the one process option
  char* replyData_;
  ULng32 replyDatalen_;
  ReturnStatus retval_;
  Int64 recentIpcTimestamp_;
}; // end of ExSqlComp
  
// -----------------------------------------------------------------------
// CmpMessageStream class, based on IpcMessageStream
// -----------------------------------------------------------------------

class CmpMessageStream : public IpcMessageStream
{
public:
  CmpMessageStream(IpcEnvironment*, ExSqlComp*);
 
  void setWaited(NABoolean t)   {  waited_ = t; }
  NABoolean waited()   {  return waited_; }

  virtual void actOnSend(IpcConnection* connection);
  virtual void actOnSendAllComplete();
  virtual void actOnReceive(IpcConnection* connection);
  
  virtual ~CmpMessageStream() {  }

private:
  CmpMessageStream(const CmpMessageStream&);
  const CmpMessageStream& operator=(const CmpMessageStream&);

  ExSqlComp *sqlcomp_;
  NABoolean waited_;

}; // end of CmpMessageStream

#include "NAExecTrans.h"

#endif
