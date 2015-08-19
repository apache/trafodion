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
#ifndef __EX_UDR_SERVER_H
#define __EX_UDR_SERVER_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExUdrServer.h
 * Description:  Client-side process management for UDR servers
 *               
 * Created:      08/16/2000
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "ComSmallDefs.h"
#include "UdrExeIpc.h"
#include "ExCollections.h"
#include "NAUserId.h"

// Default nowait depth of 3 is used for connections (except control
// connection) to UDR Server
#define DEFAULT_NOWAIT_DEPTH 3

// -----------------------------------------------------------------------
// Forward class references
// -----------------------------------------------------------------------
class UdrControlMsg;
class UdrClientControlStream;
class ExUdrTcb;
class IpcProcessId;

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExUdrServer;
class ExUdrServerManager;

extern NABoolean ProcessIdIsNull(const IpcProcessId &);
extern void InvalidateProcessId(IpcProcessId &);

// -----------------------------------------------------------------------
// ExUdrServer
// -----------------------------------------------------------------------
class ExUdrServer : public NABasicObject
{
public:
  enum ExUdrServerStatus
  {
    EX_UDR_SUCCESS = 0,
    EX_UDR_WARNING,
    EX_UDR_ERROR
  };

  enum ExUdrServerState
  {
    EX_UDR_NOT_STARTED = 0,
    EX_UDR_READY,
    EX_UDR_BROKEN
  };

  ExUdrServer(IpcEnvironment *env,
              const Int32 &userId,
              const char *options,
              const char *optionDelimiters,
              const char *userName,
              const char *userPassword,
              IpcServerClass *serverClass);

  ~ExUdrServer();

  void setState(ExUdrServerState state) { state_ = state; }
  ExUdrServerState getState() const { return state_; }

  void setDedicated(NABoolean dedicated) { dedicated_ = dedicated; }
  NABoolean isDedicated(void) { return dedicated_;}
  void setInUse(NABoolean inUse) { inUse_ = inUse; }
  NABoolean inUse(void){ return inUse_;}

  ExUdrServerStatus start(ComDiagsArea **diags,
                          CollHeap *diagsHeap,
                          Int64 transId,
                          IpcProcessId &newId,
                          NABoolean usesTransactions);
  ExUdrServerStatus stop();
  ExUdrServerStatus kill(ComDiagsArea *diags);

  const IpcProcessId getServerProcessId() const { return serverProcessId_; }

  const char *getOptions() const { return options_; }
  const char *getOptionDelimiters() const { return optionDelimiters_; }

  // Helper function to send down server-side runtime options. Must be
  // called only after successful startup of the server.
  void sendStartupOptions(ComDiagsArea **diags, CollHeap *diagsHeap,
                          Int64 transId);

  // Matchmaking logic to determine if this server has the requested
  // attributes
  NABoolean match(const Int32 &userId,
                  const char *options,
                  const char *optionDelimiters) const;

  //
  // Functions to quiesce the UDR server
  // - isIOPending() returns TRUE if any replies are outstanding.
  // - completeUdrRequests(TRUE) polls for I/O completion until
  //   isIOPending() returns FALSE. completeUdrRequests(FALSE)
  //   waits for one I/O to complete then returns.
  //
  NABoolean isIOPending(IpcConnection *conn) const;
  void completeUdrRequests(IpcConnection *conn, NABoolean waitForAllIO) const;

  IpcConnection *getUdrControlConnection() const;

  IpcConnection *getAnIpcConnection() const;
  void releaseConnection(IpcConnection *conn);

  IpcEnvironment *myIpcEnv() const { return ipcEnvironment_; }
  CollHeap *myIpcHeap() const;

  void incrRefCount() { ++refCount_; }
  void decrRefCount() { --refCount_; }
  void setRefCount(ComUInt32 refCount) { refCount_ = refCount; }
  ComUInt32 getRefCount() const { return refCount_; }

  const Int32 &getUserId() const { return userId_; };

#ifdef UDR_DEBUG
  void setTraceFile(FILE *f)
  {
    traceFile_ = f;
  }
#endif

protected:

  inline NABoolean ready() const { return (state_ == EX_UDR_READY); }

  ExUdrServerState state_;
  IpcEnvironment *ipcEnvironment_;
  IpcServerClass *udrServerClass_;
  IpcServer *ipcServer_;
  IpcProcessId serverProcessId_;
  ULng32 startAttempts_;

  // The user identity for this UDR server
  Int32 userId_;
  char *userName_;
  char *userPassword_;

  // Server-side runtime options. Currently the options_ string stores
  // JVM startup options only.
  char *options_;
  char *optionDelimiters_;

  // A small summary of how reference count, dedicated and inUse flags
  // are used. Note that there are three layers of maintaining the 
  // usage of mxudr servers. First layer being the top layer.
  // First layer: Server Manager is an instance in CliGlobals. Server
  // manager either creates a new server process or returns a existing
  // server in its pool for reuse. Server manager maintains a single
  // list of servers.
  //
  // Second layer: ContextCli obtains servers from server manager or
  // the first layer on a need basis. ContextCli reuses servers in its
  // pool to service requests from various statements in its scope. 
  // contextCli can request a shared or a dedicated server from server
  // manager. A dedicated server request to server manager will always
  // return a server whose reference count is 1.   

  // Note that server manager upon request from other contextCli requests for
  // a shared mode server will not hand over a server from its pool that has
  // been already obtained as dedicated by another contextCli.
  
  // Referece count of server is always incremented by one (by the server
  // manager) when ContextCli obtaines a server from server manager. Once
  // the server is in ContextCli scope, its reference count is not altered 
  // in contextCli scope. Reference count is decremented once contectCli 
  // returns the server back to server manager. Once the server is returned 
  // back to server manager, it is free to be reassined to other contextCli
  // requests as a shared or dedicated server.

  // ContextCli uses the list of servers to complete Io or return back servers
  // when under certain scenarios. search for udrServerList_ in contextCli scope.

  // Third layer: ex_exe_statement_globals obtains servers from contextcli on 
  // a demand basis. Multiple tcbs in a statment may use a server that is obtained
  // from contextcli in shared mode. For TMUDF tcbs, a dedicated server from
  // contextCli would be obtained in dedicated mode. ex_exe_statement_globals
  // maintains a single pointer to server that is obtained in shared mode and
  // a separate list of pointers to servers that have been obtained in dedicated mode.
  // Ex_exe_statement_global sets the inUse_ flag for dedicated servers so that
  // contextCli does not hand the same dedicated server to other statements.



  // Number of contexts and TCBs using this ExUdrServer object.  A
  // refCount of 0 means no other objects are using this UDR server
  // and it can be returned to the pool of idle servers, or released.
  ComUInt32 refCount_;

  // This flag indicates that the server is obtained as dedicated and
  // cannot be shared with other contextCli requests. Usually this flag
  // is set dedicated by contextCli attempting to get this server in dedicated
  // mode, usuually to service a TMUDF client. A dedicated server is 
  // usually used by one client and never shared until it is not in use. 
  // see inUse_ flag.
  NABoolean dedicated_;

  // Ex_exe_statement_global sets the inUse_ flag for dedicated servers so that
  // contextCli does not hand the same dedicated server to other statements. 
  // inUse_ flag is reset once a statement is deallocated.
  NABoolean inUse_;


  // Lists to maintain IPC Connections to the UDR Server process
  NAList<IpcConnection *> *inUseConns_;
  NAList<IpcConnection *> *freeConns_;

#ifdef UDR_DEBUG
  FILE *traceFile_;
#endif

private:
  ExUdrServer(); // do not implement a default constructor

};


// -----------------------------------------------------------------------
// ExUdrServerManager
// -----------------------------------------------------------------------
class ExUdrServerManager : public NABasicObject
{
public:

  ExUdrServerManager(
    IpcEnvironment *env,
    ComUInt32      maxServersPerGroup = 1);

  ~ExUdrServerManager();

  // This method provides access to a UDR server with the requested
  // attributes. Successful completion of this method does not
  // guarantee that the process is actually started. Currently the
  // options parameter is for JVM startup options only, and simple
  // string comparision is our test to determine if two option sets
  // are equivalent.
  ExUdrServer *acquireUdrServer(const Int32 &userId,
                                const char *options,
                                const char *optionDelimiters,
                                const char *userName,
                                const char *userPassword,
                                NABoolean dedicated = FALSE);

  // Decrement the reference count for a given ExUdrServer instance
  void releaseUdrServer(ExUdrServer *udrServer);

  // This is the heap that will be used for all dynamic memory
  // allocations
  inline CollHeap *myIpcHeap() const
  {
    return ipcEnvironment_->getHeap();
  }

  inline ComUInt32 getMaxServersPerGroup() const
  { return maxServersPerGroup_; }

protected:

  // We need an IpcEnvironment pointer to create ExUdrServer objects.
  // That's why we store this here. All ExUdrServer objects share
  // this pointer.
  IpcEnvironment *ipcEnvironment_;

  // Server class used to create a server process. This pointer will
  // be copied to all ExUdrServer objects.
  IpcServerClass *udrServerClass_;

  // Pool of ExUdrServers
  LIST(ExUdrServer*) serverPool_;

  // Maximum number of servers an executor can create with a given set
  // of attributes. Currently this value is set to 1. In the future if
  // we want to manage multiple servers with the same attributes we
  // can set this value higher.
  ComUInt32 maxServersPerGroup_;

  // The following boolean helps us decide when it is OK to retain an
  // unused server in the pool for an application that only requires a
  // single server.
  NABoolean okToRetainOneServer_;

#ifdef UDR_DEBUG
  FILE *traceFile_;
#endif

};

#endif // __EX_UDR_SERVER_H

