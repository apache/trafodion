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
 * File:         ExUdrServer.cpp
 * Description:  Client-side process management for UDR servers
 *
 * Created:      08/16/2000
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "ex_stdh.h"
#include "ExUdrServer.h"
#include "ExUdrClientIpc.h"
#include "ExpError.h"
#include "ExCextdecs.h"
#include "ComRtUtils.h"
#include "PortProcessCalls.h"

#include "seabed/fs.h"
#include "seabed/ms.h"

#ifdef UDR_DEBUG
extern const char *GetWorkRetcodeString(ExWorkProcRetcode r);
/*
static const char *GetStatusString(ExUdrServer::ExUdrServerStatus s)
{
  switch (s)
  {
  case ExUdrServer::EX_UDR_SUCCESS:
    return "Success";
    break;
  case ExUdrServer::EX_UDR_WARNING:
    return "Warning";
    break;
  case ExUdrServer::EX_UDR_ERROR:
    return "Error";
    break;
  }
  return "***UNKNOWN***";
} */

#define UdrDebug0(s) \
  ( UdrPrintf(traceFile_,(s)) )
#define UdrDebug1(s,a1) \
  ( UdrPrintf(traceFile_,(s),(a1)) )
#define UdrDebug2(s,a1,a2) \
  ( UdrPrintf(traceFile_,(s),(a1),(a2)) )
#define UdrDebug3(s,a1,a2,a3) \
  ( UdrPrintf(traceFile_,(s),(a1),(a2),(a3)) )
#define UdrDebug4(s,a1,a2,a3,a4) \
  ( UdrPrintf(traceFile_,(s),(a1),(a2),(a3),(a4)) )
#define UdrDebug5(s,a1,a2,a3,a4,a5) \
  ( UdrPrintf(traceFile_,(s),(a1),(a2),(a3),(a4),(a5)) )

#else
//
// Debug macros are no-ops in the release build
//
#define UdrDebug0(s)
#define UdrDebug1(s,a1)
#define UdrDebug2(s,a1,a2)
#define UdrDebug3(s,a1,a2,a3)
#define UdrDebug4(s,a1,a2,a3,a4)
#define UdrDebug5(s,a1,a2,a3,a4,a5)

#endif // UDR_DEBUG

//
// Helper functions allowing the ExUdrServer class and its callers to
// determine if a process ID is NULL and to nullify a process
// ID. Internally in our IpcProcessId objects we consider a valid
// process ID to be anything with a domain other than IPC_DOM_INVALID.
// The default IpcProcessId constructor sets the domain to
// IPC_DOM_INVALID so this constructor can be used to instantiate a
// NULL process ID.
//
NABoolean ProcessIdIsNull(const IpcProcessId &id)
{
  NABoolean result = FALSE;
  if (id.getDomain() == IPC_DOM_INVALID)
  {
    result = TRUE;
  }
  return result;
}
void InvalidateProcessId(IpcProcessId &id)
{
  //
  // The default IpcProcessId constructor is used to instantiate
  // a process ID with domain IPC_DOM_INVALID.
  //
  IpcProcessId nullPid;
  //
  // Invoke the IpcProcessId assignment operator to invalidate
  // id. Arguments to this operator are passed by reference, not on
  // the stack.
  //
  id = nullPid;
}

// -----------------------------------------------------------------------
// ExUdrServer
// -----------------------------------------------------------------------
ExUdrServer::ExUdrServer(IpcEnvironment *env,
                         const Int32 &userId,
                         const char *options,
                         const char *optionDelimiters,
                         const char *userName,
                         const char *userPassword,
                         IpcServerClass *serverClass)
  : state_(EX_UDR_NOT_STARTED),
    ipcEnvironment_(env),
    udrServerClass_(serverClass),
    ipcServer_(NULL),
    serverProcessId_(),
    startAttempts_(0),
    userId_(userId),
    userName_(NULL),
    userPassword_(NULL),
    options_(NULL),
    optionDelimiters_(NULL),
    refCount_(0),
    dedicated_(FALSE),
    inUse_(FALSE),
    inUseConns_(NULL),
    freeConns_(NULL)
#ifdef UDR_DEBUG
    , traceFile_(NULL)
#endif
{
  ex_assert(options && optionDelimiters,
            "No runtime options specified for UDR server startup");

  CollHeap *h = myIpcHeap();

  Int32 len = str_len(options);
  options_ = new (h) char[len + 1];
  str_cpy_all(options_, options, len + 1);

  len = str_len(optionDelimiters);
  optionDelimiters_ = new (h) char[len + 1];
  str_cpy_all(optionDelimiters_, optionDelimiters, len + 1);

  if (userName)
  {
    len = str_len(userName);
    userName_ = new (h) char[len + 1];
    str_cpy_all(userName_, userName, len + 1);
  }

  if (userPassword)
  {
    len = str_len(userPassword);
    userPassword_ = new (h) char[len + 1];
    str_cpy_all(userPassword_, userPassword, len + 1);
  }

  inUseConns_ = new (h) NAList<IpcConnection *>(h);
  freeConns_ = new (h) NAList<IpcConnection *>(h);
}

ExUdrServer::~ExUdrServer()
{
  UdrDebug1("[BEGIN ExUdrServer destructor] %p", this);

  stop();

  CollHeap *h = myIpcHeap();
  NADELETEBASIC(options_, h);
  NADELETEBASIC(optionDelimiters_, h);
  NADELETEBASIC(userName_, h);
  NADELETEBASIC(userPassword_, h);

  UdrDebug1("[END ExUdrServer destructor] %p", this);
}

CollHeap *ExUdrServer::myIpcHeap() const
{
  return myIpcEnv()->getHeap();
}

//
// Bring a UDR Server process to life if one hasn't been
// started already
//
ExUdrServer::ExUdrServerStatus ExUdrServer::start(ComDiagsArea **diags,
                                                  CollHeap *diagsHeap,
                                                  Int64 transId,
                                                  IpcProcessId &newId,
                                                  NABoolean usesTransactions)
{
#ifdef UDR_DEBUG
  UdrDebug1("[BEGIN ExUdrServer::start()] %p", this);
  UdrDebug1("  Startup options '%s'", options_);
  UdrDebug1("  Startup option delimiters '%s'", optionDelimiters_);
  if (diags && *diags)
  {
    Lng32 numDiags = (*diags)->getNumber();
    UdrDebug1("  The diagnostics area initially contains %d entries",
              numDiags);
  }
  else
  {
    UdrDebug0("  No diagnostics area exists yet");
  }
#endif // UDR_DEBUG

  // The newId and result variables hold our return values. We will
  // assume failure initially and set the values to something else
  // once we are sure we have succeeded.
  InvalidateProcessId(newId);
  ExUdrServerStatus result = EX_UDR_ERROR;

  if (ready())
  {
#ifdef UDR_DEBUG
    char buf[300];
    serverProcessId_.toAscii(buf, 300);
    UdrDebug1("  A server is already running. Process ID %s", buf);
#endif // UDR_DEBUG
    newId = serverProcessId_;
    result = EX_UDR_SUCCESS;
  }
  else
  {
    stop();

    UdrDebug0("  About to start the UDR server...");

    // Notes on UDR Server startup
    // - We are using a nowait depth of 2 for first connection. SPJs
    //   that cannot return RS use first connection for IPC.
    //   We use nowait depth of 3 for all other connections used by SPJs
    //   that can return RS. Look at getAnIpcConnection() for code details.
    //
    // - By specifying IPC_CPU_DONT_CARE as the CPU number in this
    //   call to allocateServerProcess() we get the default behavior
    //   from Guardian which is to start the process on the same CPU as
    //   the caller.

    Lng32 nowaitDepth = 2;

#ifdef _DEBUG
    char *e = getenv("UDR_NOWAIT_DEPTH");
    if (e && e[0])
      nowaitDepth = atol(e);
#endif
    UdrDebug1("  Using a nowait depth of %d", nowaitDepth);

    NABoolean waitedCreation = TRUE;
    // co-locate the tdm_udrserv with the executor process (master or ESP)
    // This is done for a couple of reasons: One is that since the ESPs
    // are evenly balanced across the CPUs, this ensures an even distribution
    // of the tdm_udrservs as well, probably better than a random distribution.
    // The second reason is that for certain maintenance UDFs (only example
    // so far is udf(event_log_reader())), we must ensure that we run one
    // tdm_udrserv on each node of the cluster, and we do that by starting
    // one ESP per node.
    IpcCpuNum collocatedCPU =
      myIpcEnv()->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).getCpuNum();

    ipcServer_ =
      udrServerClass_->allocateServerProcess(diags,
                                             diagsHeap,
                                             NULL,
                                             collocatedCPU,
                                             IPC_PRIORITY_DONT_CARE,
                                             1,   // espLevel (not relevant for UDR servers) 
                                             usesTransactions,
                                             waitedCreation,
                                             nowaitDepth);
    
#ifdef UDR_DEBUG
    UdrDebug1("  allocateServerProcess() returned %p", ipcServer_);
    if (diags && *diags)
    {
      Lng32 numDiags = (*diags)->getNumber();
      UdrDebug1("  The diagnostics area contains %d entries",
                numDiags);
    }
    else
    {
      UdrDebug0("  No diagnostics area exists");
    }
#endif // UDR_DEBUG

    startAttempts_++;

    if (diags && *diags)
    {
      Lng32 sqlcode = ((*diags)->mainSQLCODE());
      if (sqlcode < 0)
      {
        //
        // An error occurrred
        //
        // $$$$
        // Need to verify whether we can release the server class
        // instance Looks like allocateServerProcess() can return
        // non-NULL and also generate diagnostics. If that happens and
        // we call ipcServer_->release() an assertion fails because
        // ipcServer_ does not yet have a valid process handle.  The
        // assertion is in IpcProcessId::getPhandle().
        //
        UdrDebug0("  ***");
        UdrDebug1("  *** WARNING: Errors occurred. Main SQLCODE is %d",
                  sqlcode);
        UdrDebug0("  ***");
        ipcServer_ = NULL;
      }
    }

    if (ipcServer_ && ipcServer_->getControlConnection())
    {
      // We enter this block once a server process has successfully
      // been started.

      setState(EX_UDR_READY);

      // Record the process ID
      serverProcessId_ = ipcServer_->getControlConnection()->getOtherEnd();

#ifdef UDR_DEBUG
      char buf[300];
      serverProcessId_.toAscii(buf, 300);
      UdrDebug1("  A new server was started. Process ID %s", buf);
#endif // UDR_DEBUG

      // Set a flag in the control connection indicating whether or
      // not to perform integrity checks on incoming buffers.
      NABoolean trust = FALSE;
#ifdef UDR_DEBUG
      if (getenv("UDR_TRUST_REPLIES"))
      {
        trust = TRUE;
      }
#endif
      ipcServer_->getControlConnection()->setTrustIncomingBuffers(trust);


      // Send down any requested runtime options. Right now the only
      // options we support are JVM startup options.
      sendStartupOptions(diags, diagsHeap, transId);

    } // if (ipcServer_ && ipcServer_->getControlConnection())

    if (ready())
    {
      newId = serverProcessId_;
      result = EX_UDR_SUCCESS;
    }
    else
    {
      UdrDebug0("  Unable to start the UDR server");
      
      //
      // If ready() is not TRUE and no diagnostics have been created yet,
      // create the generic "Unable to receive reply from MXUDR" diagnostic
      // here.
      //
      Lng32 numDiags = 0;
      if (diags && *diags)
      {
        numDiags = (*diags)->getNumber();
      }
      if (numDiags == 0)
      {
        UdrDebug0("  ***");
        UdrDebug0("  *** WARNING: Errors occurred but no diagnostics created");
        UdrDebug0("  ***");
        if (diags)
        {
          if (!(*diags))
          {
            *diags = ComDiagsArea::allocate(diagsHeap);
          }
          **diags << DgSqlCode(-EXE_UDR_REPLY_ERROR);
        }
      }
      
      stop();
      result = EX_UDR_ERROR;
    
    } // if (!ready())
    
  } // if (ready()) ... else ...
  
  UdrDebug1("[END ExUdrServer::start()] %p", this);
  return result;
}

void ExUdrServer::sendStartupOptions(ComDiagsArea **diags,
                                     CollHeap *diagsHeap,
                                     Int64 transId)
{
  // Send down any requested runtime options. Right now the only
  // options we support are JVM startup options. To do the work we
  // will allocate a stream and a message on the IPC heap. The
  // message gets cleaned up eventually when its reference count
  // reaches zero. The stream takes care of its own cleanup by
  // putting itself on the IPC environment's list of "completed"
  // streams, and cleanup of that list is always guaranteed to be
  // done at a safe time.

  // This method should only be called after startup was successful
  ex_assert(ipcServer_ && ipcServer_->getControlConnection(),
            "Do not call this method without a running server");

  UdrDebug0("  About to send startup options to the server");
  NAMemory *ipcHeap = myIpcHeap();

  // If the options_ is set to OFF or ANYTHING and there is
  // no userName_ then we don't have anything to do here
  if (userName_ == NULL &&
      (str_cmp_ne(options_, "OFF") == 0 || str_cmp_ne(options_, "ANYTHING") == 0))
  {
    return ;
  }

  // Send the user name also in startup options as
  // "-Dsqlmx.udr.username=userName_"
  char *userNameOption = NULL;
  Int32 userNameOptionLen = 0;

  if (userName_)
  {
    const char *userNamePrefix = "-Dsqlmx.udr.username=";
    Int32 userNamePrefixLen = str_len(userNamePrefix);
    Int32 userNameLen = str_len(userName_);
    userNameOption = new (ipcHeap) char[userNamePrefixLen + userNameLen + 1];
    str_sprintf(userNameOption, "%s%s", userNamePrefix, userName_);
    userNameOptionLen = str_len(userNameOption);
  }

  // Send the user password also in startup options as
  // "-Dsqlmx.udr.password=userPassword_"
  char *passwordOption = NULL;
  Int32 passwordOptionLen = 0;

  if (userPassword_)
  {
    const char *passwordPrefix = "-Dsqlmx.udr.password=";
    Int32 passwordPrefixLen = str_len(passwordPrefix);
    Int32 passwordLen = str_len(userPassword_);
    passwordOption = new (ipcHeap) char[passwordPrefixLen + passwordLen + 1];
    str_sprintf(passwordOption, "%s%s", passwordPrefix, userPassword_);
    passwordOptionLen = str_len(passwordOption);
  }

  char *optionsToSend = NULL;
  Int32 delimiterLen = 1;
  Int32 tmpLen = 0;
  if (str_cmp_ne(options_, "OFF") != 0 &&
      str_cmp_ne(options_, "ANYTHING") != 0)
  {
    if (userName_)
    {
      Int32 optionsLen = str_len(options_);
      Int32 len = optionsLen + delimiterLen + userNameOptionLen 
                             + delimiterLen + passwordOptionLen;

      optionsToSend = new (ipcHeap) char[len + 1];
      str_cpy_all(optionsToSend, options_, optionsLen);
      optionsToSend[optionsLen] = optionDelimiters_[0];

      // Copy the user name
      str_cpy_all(optionsToSend + optionsLen + delimiterLen,
                  userNameOption,
                  userNameOptionLen);
      tmpLen = optionsLen + delimiterLen + userNameOptionLen;
      optionsToSend[tmpLen] = optionDelimiters_[0];

      // Copy the user password
      str_cpy_all(optionsToSend + tmpLen + delimiterLen,
                  passwordOption,
                  passwordOptionLen);
      optionsToSend[len] = '\0';
    }
    else
    {
      Int32 optionsLen = str_len(options_);
      optionsToSend = new (ipcHeap) char[optionsLen + 1];
      str_cpy_all(optionsToSend, options_, optionsLen + 1);
    }
  }
  else
  {
    if (userName_)
    {
      if (userPassword_)
      {
        Int32 len = userNameOptionLen + delimiterLen + passwordOptionLen;

        optionsToSend = new (ipcHeap) char[len + 1];

        // Copy the user name
        str_cpy_all(optionsToSend, userNameOption, userNameOptionLen);
        optionsToSend[userNameOptionLen] = ' ';

        // Copy the user password
        str_cpy_all(optionsToSend + userNameOptionLen + delimiterLen,
                    passwordOption, passwordOptionLen);
        optionsToSend[len] = '\0';
      }
      else
      {
        optionsToSend = new (ipcHeap) char[userNameOptionLen + 1];

        // Copy the user name 
        str_cpy_all(optionsToSend, userNameOption, userNameOptionLen + 1);
      }
    }
    else
    {
      // No need to send any options. We never come here because this
      // case is already checked above.
      return;
    }
  }

  NABoolean isTransactional = (transId == -1 ? FALSE : TRUE);
  
  UdrClientControlStream *stream = new (ipcHeap)
    UdrClientControlStream(myIpcEnv(),
                           NULL, // tcb
                           NULL, // stmt globals
                           TRUE, // keep diags for caller
                           isTransactional);

#ifdef UDR_DEBUG
  if (traceFile_)
  {
    stream->setTraceFile(traceFile_);
  }
#endif
    
  UdrSessionMsg *msg = new (ipcHeap)
    UdrSessionMsg(UdrSessionMsg::UDR_SESSION_TYPE_JAVA_OPTIONS,
                    0, ipcHeap);
  msg->addString(optionsToSend);
  msg->addString(optionDelimiters_);
    
  IpcConnection *conn = ipcServer_->getControlConnection();
  stream->addRecipient(conn);
  *stream << *msg;
  stream->send(TRUE,  // TRUE indicates a waited send
               transId);
    
  msg->decrRefCount();
  NADELETEBASIC(userNameOption, ipcHeap);
  NADELETEBASIC(passwordOption, ipcHeap);
  NADELETEBASIC(optionsToSend, ipcHeap);
    
  //--------------------------------------------------------------------
  // We just completed a waited send of the startup options. There
  // are a couple of error checks we need to perform now.
  // a) The stream may have encountered IPC errors and put itself into
  //    an error state.
  // b) The IPC could have been successful but the server may have
  //    returned SQL diags in its reply. Our UDR-specific message 
  //    stream subclass caches these diags for us.
  //--------------------------------------------------------------------
    
  // This boolean will track whether we need to add a generic
  // "Unable to receive reply from MXUDR" condition to the diags
  // area.
  NABoolean addUdrCondition = FALSE;

  // a) Did the stream put itself into an error state?
  if (stream->getErrorInfo() != 0)
  {
    UdrDebug0("    The message stream encountered errors");
    addUdrCondition = TRUE;
    if (diags)
    {
      if (*diags == NULL)
      {
        *diags = ComDiagsArea::allocate(diagsHeap);
      }
      conn->populateDiagsArea(*diags, diagsHeap);
    }
    stop();
  }
  else
  {
    // b) Did the server return diags?
    ComDiagsArea *diagsFromServer = stream->extractUdrDiags();
    if (diagsFromServer)
    {
      UdrDebug0("    The server returned diagnostics in its reply");
      if (diags)
      {
        addUdrCondition = TRUE;
        if (*diags == NULL)
        {
          *diags = ComDiagsArea::allocate(diagsHeap);
        }
        (*diags)->mergeAfter(*diagsFromServer);
      }
        
      diagsFromServer->decrRefCount();
      stop();
    }
  }
    
  // If either a) or b) was true, we have diags to return but they
  // may not be UDR-specific so we add a generic "Unable to
  // receive reply from MXUDR" condition here.
  if (diags && addUdrCondition)
  {
    if (*diags == NULL)
    {
      *diags = ComDiagsArea::allocate(diagsHeap);
    }
    **diags << DgSqlCode(-EXE_UDR_REPLY_ERROR);
  }

}

//
// Bring down the server process. Note that this is not a forceful
// "kill" method. If the server is busy or hung then this call may not
// actually stop the process. Under normal circumstances the call to
// release() in this method will close the control connection to the
// server, the server will detect that its only client has gone away,
// and the server will exit.
//
ExUdrServer::ExUdrServerStatus ExUdrServer::stop()
{
  UdrDebug1("[BEGIN ExUdrServer::stop()] %p", this);

  // Release all connections that were opened for this
  // Server instance. We destruct the connections that are not being
  // used. The in-use connections will be freed in releaseConnection()
  // when they are tried to use next time.
  for ( ; freeConns_->entries(); )
  {
    IpcConnection *conn = freeConns_->at(0);
    freeConns_->removeAt(0);
    delete conn;
  }

  if (ipcServer_)
  {
    UdrDebug0("  About to release the server class instance");
    //
    // This call will remove the IpcServer instance from the server 
    // class and deallocate the IpcServer instance
    //
    ipcServer_->release();
    ipcServer_ = NULL;
    InvalidateProcessId(serverProcessId_);
  }

  setState(EX_UDR_NOT_STARTED);

  UdrDebug1("[END ExUdrServer::stop()] %p", this);
  return EX_UDR_SUCCESS;
}

ExUdrServer::ExUdrServerStatus ExUdrServer::kill(ComDiagsArea *diags)
{
  UdrDebug1("[BEGIN ExUdrServer::kill()] %p", this);
  short result = 0;

  char asciiPhandle[300];
  serverProcessId_.toAscii(asciiPhandle, 300);
  UdrDebug1("  UDR Server process handle is %s", asciiPhandle);

  if (!ProcessIdIsNull(serverProcessId_))
  {
    if (serverProcessId_.getDomain() == IPC_DOM_GUA_PHANDLE)
    {
    NAProcessHandle serverPhandle(
        (SB_Phandle_Type *) &(serverProcessId_.getPhandle().phandle_));
    Int32 guaRetcode = serverPhandle.decompose();
    if (XZFIL_ERR_OK == guaRetcode)
      msg_mon_stop_process_name(serverPhandle.getPhandleString());
    UdrDebug1("  PROCESS_STOP_ returned %d", (Int32) result);
    if (diags != NULL)
      {
        *diags << DgSqlCode(EXE_UDR_ATTEMPT_TO_KILL)
               << DgString0(asciiPhandle)
               << DgInt0((Int32) result);
      }
    }
    else
    {
      UdrDebug0("  *** ERROR: UDR Server is not a Guardian process");
    }
  }
  else
  {
    UdrDebug0("  Process handle is not valid");
  }

  UdrDebug1("[END ExUdrServer::kill()] %p", this);
  return EX_UDR_SUCCESS;
}

IpcConnection *ExUdrServer::getUdrControlConnection() const
{
  IpcConnection *result = NULL;
  if (ready() && ipcServer_)
  {
    result = ipcServer_->getControlConnection();

    if (result && result->getState() == IpcConnection::ERROR_STATE)
      result = NULL;
  }
  return result;
}

// A free connection from freeConns_ will be moved into inUseConns_ list.
// If there is no available free connection, a new IPC Connection will
// be created.
IpcConnection *ExUdrServer::getAnIpcConnection() const
{
  UdrDebug0("[BEGIN ExUdrServer::getAnIpcConnection()]");

  IpcConnection *conn = NULL;
  CollIndex numFreeConns = freeConns_->entries();

  if (numFreeConns > 0)
  {
    // remove from freeList_ and add it in inUseList_
    conn = freeConns_->at(numFreeConns - 1);
    freeConns_->removeAt(numFreeConns - 1);
    inUseConns_->insert(conn);

    UdrDebug1("    An existing connection %p will be reused", conn);
  }
  else
  {
    Lng32 nowaitDepth = DEFAULT_NOWAIT_DEPTH;

#ifdef _DEBUG
    char *e = getenv("UDR_NOWAIT_DEPTH");
    if (e && e[0])
      nowaitDepth = atol(e);
#endif
    UdrDebug1("  Using a nowait depth of %d", nowaitDepth);

    // create a new connection and add it in inUseList_
    conn = serverProcessId_.createConnectionToServer(myIpcEnv(),
                                                     TRUE,
                                                     nowaitDepth);

    // Set a flag in connection indicating whether or
    // not to perform integrity checks on incoming buffers.
    NABoolean trust = FALSE;
#ifdef UDR_DEBUG
      if (getenv("UDR_TRUST_REPLIES"))
      {
        trust = TRUE;
      }
#endif

    conn->setTrustIncomingBuffers(trust);


    UdrDebug1("    A new connection %p is created", conn);
    inUseConns_->insert(conn);
  }

  UdrDebug0("[END ExUdrServer::getAnIpcConnection()]");

  return conn;
}

// Releases a connection back to freeConns_ for later use.
//
// Note: An opened connection will never be closed unless there is a
// problem accessing the server. This might be okay in most cases.
// But it's waste of resources in an app where many CALL stmts are opened
// at one point and does not do much with them later.
// Executor can have a model where the number of free conns are limited and
// a conn will be closed when it is being released if the free conn
// limit is reached. There are several ways to set this limit. One way
// is by way of session defaults.
void ExUdrServer::releaseConnection(IpcConnection *conn)
{
  if (conn == NULL)
    return;

  // Don't need to do anything for control connection since
  // control connection is not added to these lists.
  if (conn == getUdrControlConnection())
    return;

  if (! inUseConns_->remove(conn))
  {
    // If UDR Server dies, we will have NULL control connection by
    // the time we come here. In that case, we don't need to assert
    // because 'conn' might be control connection
    if (getUdrControlConnection())
      ex_assert(0, "A connection that is being released is not in use.");
  }

  // Conn will be deleted in the following cases
  //  1. conn got error
  //  2. UDR Server is in error state because some other conn got error
  //  3. UDR Server is restarted after an error
  if (conn->getState() == IpcConnection::ERROR_STATE ||
      state_ == ExUdrServer::EX_UDR_BROKEN ||
      ! (conn->getOtherEnd() == serverProcessId_))
    delete conn;
  else
    freeConns_->insert(conn);

  return;
}

NABoolean ExUdrServer::isIOPending(IpcConnection *conn) const
{
  NABoolean result = FALSE;
  if (ipcServer_ && conn)
  {
    NABoolean ioPendingOnConnection =
      conn->sendIOPending() || conn->receiveIOPending();
    NABoolean anythingQueuedOnConnection =
      (conn->numQueuedSendMessages() > 0)
      || (conn->numQueuedReceiveMessages() > 0);
    if (ioPendingOnConnection || anythingQueuedOnConnection ||
        conn->numReceiveCallbacksPending() > 0)
    {
      result = TRUE;
    }
  }
  return result;
}

void ExUdrServer::completeUdrRequests(IpcConnection *conn,
                                      NABoolean waitForAllIO) const
{
#ifdef UDR_DEBUG
  NABoolean firstTime = TRUE;
#endif // UDR_DEBUG
  NABoolean done = FALSE;

  while (!done && isIOPending(conn))
  {
#ifdef UDR_DEBUG
    if (firstTime)
    {
      firstTime = FALSE;
      UdrDebug0("***");
      UdrDebug0("*** I/O is still pending on the UDR control connection");
    }
    UdrDebug0("*** Waiting for one UDR I/O to complete...");
#endif // UDR_DEBUG
    
    // Wait on 'conn'
    conn->wait(IpcInfiniteTimeout);
    UdrDebug0("*** A UDR I/O has completed.");

    if (!waitForAllIO)
    {
      done = TRUE;
    }
  }

#ifdef UDR_DEBUG
  if (!firstTime)
  {
    UdrDebug0("***");
  }
#endif // UDR_DEBUG

}

// Matchmaking logic to determine if this server has the requested
// attributes
NABoolean ExUdrServer::match(const Int32 &userId,
                             const char *options,
                             const char *optionDelimiters) const
{
  // Two instances are considered a match if they have the same user
  // identity and any of the following are true:
  // - One or both instances have runtime options of "ANYTHING"
  // - Both instances have runtime options of "OFF"
  // - Both instances have matching runtime options and a matching
  //   option delimiter string

  if (userId_ != userId)
  {
    return FALSE;
  }

  if (str_cmp_ne(options, "ANYTHING") == 0 ||
      str_cmp_ne(options_, "ANYTHING") == 0)
  {
    return TRUE;
  }

  if (str_cmp_ne(options, "OFF") == 0 &&
      str_cmp_ne(options_, "OFF") == 0)
  {
    return TRUE;
  }

  if (str_cmp_ne(options_, options) == 0 &&
      str_cmp_ne(optionDelimiters_, optionDelimiters) == 0)
  {
    return TRUE;
  }

  return FALSE;
}

// -----------------------------------------------------------------------
// ExUdrServerManager
// -----------------------------------------------------------------------
ExUdrServerManager::ExUdrServerManager(IpcEnvironment* env,
                                       ComUInt32 maxServersPerGroup)
  : ipcEnvironment_(env),
    maxServersPerGroup_(maxServersPerGroup),
    serverPool_(myIpcHeap()),
    okToRetainOneServer_(FALSE)
#ifdef UDR_DEBUG
    , traceFile_(NULL)
#endif
{
  //
  // Create the UDR server class. This does not actually
  // start any processes. Use allocateServerProcess() to
  // do that.
  //
  // This object is copied into each ExUdrServer object.
  // ExUdrServer->start() method calls allocateServerProcess()
  // on this object to create a process.
  //
  udrServerClass_ = new (myIpcHeap())
    IpcServerClass(ipcEnvironment_, IPC_SQLUDR_SERVER);

#ifdef UDR_DEBUG
  if ((getenv("UDR_""DEBUG") != NULL) ||
      (getenv("UDR_SERVER_MGR_DEBUG") != NULL))
  {
    traceFile_ = stdout;
  }
#endif
}

ExUdrServerManager::~ExUdrServerManager()
{
  //
  // stop all the running ExUdrServer processes.
  //
  for (CollIndex i = 0; i < serverPool_.entries(); i++)
  {
    delete serverPool_[i];
  }

  if (udrServerClass_)
  {
    //
    // class IpcServerClass does not have a destructor so to delete
    // udrServerClass_ all we have to do is deallocate the memory from
    // the IPC heap.
    //
    NADELETEBASIC(udrServerClass_, myIpcHeap());
  }
}

//
// Returns an ExUdrServer with the requested attributes
//
// Creates a new ExUdrServer object if the number of existing servers
// with matching attributes is less than maximum number of servers
// allowed per group. Otherwise, we return one of the servers we
// already have with the lowest reference count.
//
// Successful completion of this method does not guarantee that the
// server process is actually started.
//
ExUdrServer* ExUdrServerManager::acquireUdrServer(const Int32 &userId,
                                                  const char *options,
                                                  const char *optionDelimiters,
                                                  const char *userName,
                                                  const char *userPassword,
                                                  NABoolean   dedicated)
{
#ifdef UDR_DEBUG
  if ((getenv("UDR_""DEBUG") != NULL) ||
      (getenv("UDR_SERVER_MGR_DEBUG") != NULL))
  {
    traceFile_ = stdout;
  }
  else
  {
    traceFile_ = NULL;
  }
#endif

  UdrDebug0("[BEGIN ExUdrServerManager::acquireUdrServer()]");
  UdrDebug1("  options: '%s'", options);
  UdrDebug1("  delimiters: '%s'", optionDelimiters);
  UdrDebug1("  Max servers per group: %u", getMaxServersPerGroup());

  ExUdrServer *udrServer = NULL;
  ComUInt32 lowestRefCnt = 0;
  ComUInt32 numServersMatched = 0;
  CollIndex entries = serverPool_.entries();

  for (CollIndex i = 0; i < entries; i++)
  {
    ExUdrServer *s = serverPool_[i];
    if (s->match(userId, options, optionDelimiters) &&
       (!s->isDedicated()))
    {
      numServersMatched++;
      if (udrServer == NULL || s->getRefCount() < lowestRefCnt)
      {
        udrServer = serverPool_[i];
	lowestRefCnt = udrServer->getRefCount();
      }
    }
  }

  UdrDebug2("  Found %u matching server%s", numServersMatched,
            numServersMatched == 1 ? "" : "s");

  if (entries > 0 && numServersMatched == 0)
  {
    UdrDebug0("  A new group is being encountered");
    UdrDebug0("  About to release idle servers...");
  
    // We are seeing a server group for the first time, and it is not
    // the only group we are currently managing. We will no longer
    // treat this application as one that only requires a single UDR
    // server.
    okToRetainOneServer_ = FALSE;
  
    // Now release all idle servers (those with a ref count of
    // zero). The loop here traverses the list in reverse order.
    CollIndex idx = entries;
    while (idx--)
    {
      ExUdrServer *curr = serverPool_[idx];
      if (curr->getRefCount() == 0)
      {
        delete curr;
	serverPool_.removeAt(idx);
      }
    }
    UdrDebug0("  Done releasing idle servers");
  }

  NABoolean reUseExistingServer = FALSE;

  if (numServersMatched >= getMaxServersPerGroup())
  {
    //
    // We hit our limit for servers in this group. We will return the
    // one with the lowest reference count.
    //
    reUseExistingServer = TRUE;
  }

  // If the request is for a dedicated server and the decision is to
  // reuse an existing server, make sure the selected server's reference
  // count is zero, else do not reuse this server.
  if(dedicated && reUseExistingServer &&
    (udrServer->getRefCount() != 0))
  {
    reUseExistingServer = FALSE;
  }


  if (reUseExistingServer)
  {
    //
    // We hit our limit for servers in this group. We will return the
    // one with the lowest reference count.
    //
    udrServer->incrRefCount();
    UdrDebug0("  No more servers can be started in this group");
    UdrDebug1("  Server %p will be reused", udrServer);
  }
  else
  {
    // If the requested options were "ANYTHING" then we are actually
    // going to start a server as if the user specified "OFF". So
    // that future requests for "OFF" will match this new server we
    // are creating, we will change "ANYTHING" to "OFF" here. Note
    // that the "ANYTHING" and "OFF" strings were uppercased by the
    // SQL compiler when the plan was generated.
    const char *newOptions = options;
    if (str_cmp_ne(options, "ANYTHING") == 0)
    {
      newOptions = "OFF";
    }

    udrServer = new (myIpcHeap()) ExUdrServer(ipcEnvironment_,
                                              userId,
                                              newOptions,
                                              optionDelimiters,
                                              userName,
                                              userPassword,
                                              udrServerClass_);
#ifdef UDR_DEBUG
    if (traceFile_)
    {
      udrServer->setTraceFile(traceFile_);
    }
#endif

    UdrDebug1("  Created a new ExUdrServer instance %p", udrServer);

    udrServer->setRefCount(1);
    serverPool_.insert(udrServer);
  }

  UdrDebug2("  Returning ExUdrServer %p, ref count %u",
            udrServer, udrServer->getRefCount());
  UdrDebug0("[END ExUdrServerManager::acquireUdrServer()]");

  return udrServer;
}

//
// A method to decrement the reference count on an ExUdrServer
// instance. Callers need to pass in a non-NULL argument because no
// checking is done here. When a reference count reaches zero we do
// one of two things:
//
//  a) Retain the ExUdrServer instance as an idle server in our
//     pool. This will be done as a performance optimization if the
//     application has not yet made use of multiple UDR servers.
//
//  b) Stop the server and remove it from the pool. We choose this
//     option once we have detected that the application is using
//     multiple UDR servers.
//
void ExUdrServerManager::releaseUdrServer(ExUdrServer *udrServer)
{
  UdrDebug1("[BEGIN ExUdrServerManager::releaseUdrServer(%p)", udrServer);

  NABoolean found = FALSE;
  for (CollIndex i = 0; !found && i < serverPool_.entries(); i++)
  {
    if (udrServer == serverPool_[i])
    {
      found = TRUE;
      udrServer->decrRefCount();
      udrServer->setDedicated(FALSE);

      UdrDebug2("  Found server %p. Ref count decremented to %u",
                udrServer, udrServer->getRefCount());

      if (udrServer->getRefCount() == 0)
      {
        if (okToRetainOneServer_ && serverPool_.entries() == 1)
        {
          // Do nothing. So far this application appears to only
          // require a single UDR server so we will retain this one.
          UdrDebug0("  There is one server in the pool, it will be retained");
        }
        else
        {
          delete udrServer;
          serverPool_.removeAt(i);
        }
      }

    } // if (udrServer == serverPool_[i])
  } // for each server in the pool

  UdrDebug1("[END ExUdrServerManager::releaseUdrServer(%p)]", udrServer);
}

