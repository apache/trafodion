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
#ifndef IPC_H
#define IPC_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Ipc.h
 * Description:  Classes to establish and perform data exchange between
 *               processes. Supports sockets and GUARDIAN file system.
 *               Ipc objects do not use the C++ runtime library.
 * Created:      11/6/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
#include	<stdlib.h>

#include "Platform.h"
#include "PortProcessCalls.h"
#include "Int64.h"
#include "ComVersionDefs.h"
#include "IpcSockets.h"
//#include "ex_sql_table.h"
#include "Collections.h"

#include "fs/feerrors.h"
// 64-bit
// We must use literal "int" here, which violates our rule not use int
// directly in our source code. Otherwise, we'd have macro redefinition
// because _cc_status is defined as int in a header file not in our source.
#define _cc_status int  /* MUST use "int" here, not Int32. See comment above.*/

#define SSCP_PROCESS_PREFIX "$ZSC"
#define SSMP_PROCESS_PREFIX "$ZSM"
#define QMM_PROCESS_PREFIX "$ZQM"
#define QMP_PROCESS_PREFIX "$ZQP"
#define QMS_PROCESS_PREFIX "$ZQS"
#define LSIG       0200
#define LREQ       0400
#define LDONE     02000
#define LRABBIT     010
#define PERSISTENT_OPEN_RECONNECT_CODE 0x4ACE
#include "ComExeTrace.h"
#include <time.h>

#include "seabed/fs.h"

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------
const unsigned char closeTraceEntries = 64;
const unsigned char bawaitioxTraceEntries = 64;
#ifdef _DEBUG
#define NUM_IPC_MSG_TRACE_ENTRIES 32
#else
#define NUM_IPC_MSG_TRACE_ENTRIES 512
#endif
#define MAX_IPC_MSG_TRACE_ENTRIES 2049

struct GuaNodeName;
struct GuaReceiveInfo;
struct CloseTraceEntry;
struct PersistentOpenEntry;
struct BawaitioxTraceEntry;
class GuaReceiveFastStart;
class SockConnection;
class IpcNodeName;
struct GuaProcessHandle;
class MyGuaProcessHandle;
struct SockProcessId;
class IpcProcessId;
class IpcConnection;
class IpcSetOfConnections;
class SockPairConnection;
class GuaConnectionToServer;
class GuaMsgConnectionToServer;
class GuaConnectionToClient;
class IpcControlConnection;
class SockControlConnection;
class GuaReceiveControlConnection;
class IpcMessageBuffer;
class IpcMessageStreamBase;
class IpcMessageStream;
class IpcBufferedMsgStream;
class IpcClientMsgStream;
class IpcServerMsgStream;
class IpcServer;
class IpcGuardianServer;
class IpcServerClass;
class IpcAllConnections;
class IpcEnvironment;
class DefaultIpcHeap;
class ComDiagsArea;
class IpcConnectionTrace;
struct NowaitedEspStartup;
struct NowaitedEspServer;

// Macros
  #define DELAY_CSEC(n) DELAY(n)

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------
class CliGlobals;
class ExRtFragTable;  // needed for IPC-related data structure integrity check
class ExMasterEspMessage;  // needed for IPC-related data structure integrity check

// -----------------------------------------------------------------------
// A GUARDIAN style node name
// -----------------------------------------------------------------------
const Int32 GuaNodeNameMaxLen = 8;
struct GuaNodeName
{
  char nodeName_[GuaNodeNameMaxLen]; // we don't store the leading backslash
};

struct IpcEyeCatcher
{
  char nameForObjIdentity[4];    // should 8-byte have better performance
};

class IpcAwaitiox
{
public:
  IpcAwaitiox() { fileNum_ = -1; completed_ = FALSE; retryCount_ = 0; }
  void DoAwaitiox(NABoolean ignoreLrec);
  Int32 ActOnAwaitiox(void **bufAddr, Int32 *count, SB_Tag_Type *tag);
  inline short getFileNum() { return fileNum_; }
  inline NABoolean getCompleted() { return completed_; }
private:
  Int32 condCode_;
  short fileNum_;
  void * bufAddr_;
  Int32 count_;
  SB_Tag_Type tag_;
  short retCode_;
  short lastError_;
  NABoolean completed_;
  short retryCount_;
};

// -----------------------------------------------------------------------
// A generic, normalized node name (either IP address or GUARDIAN node name)
// -----------------------------------------------------------------------
class IpcNodeName
{
public:

  inline IpcNodeName()                        { domain_ = IPC_DOM_INVALID; }
  // resolve a node name from a domain identifier and a character string
  IpcNodeName(IpcNetworkDomain dom,
	      const char *name);
  // extract a node name from a process id
  // IpcNodeName(const IpcProcessId &proc);
  // make a node name from an Internet address or from a phandle
  IpcNodeName(const SockIPAddress &iPNode);
  IpcNodeName(const GuaProcessHandle &phandle);
  // Return the Guardian node name as a character string, without trailing spaces.
  inline void getNodeNameAsString (char * nodeName) const
  {
    nodeName[0] = '\\'; 
    for (Int32 i=0;i<GuaNodeNameMaxLen;i++)
      if (guardianNode_.nodeName_[i] > ' ')
        nodeName[i+1] = guardianNode_.nodeName_[i];
      else
        nodeName[i+1] = 0;
  };
  IpcNodeName & operator = (const IpcNodeName &other);
  NABoolean operator == (const IpcNodeName &other);

  inline IpcNetworkDomain getDomain() const      { return domain_; }
  SockIPAddress getIPAddress() const;

private:

  // the domain under which this node is addressable
  IpcNetworkDomain domain_;

  union
    {
      SockRawIPAddress ipAddr_;
      GuaNodeName guardianNode_;
    };
};

// -----------------------------------------------------------------------
// maximum buffer size sent through WRITEREADX (for GuaConnectionToServer)
// or MSG_LINK_ (for GuaMsgConnectionToServer).  Attempts to use buffers
// larger than this will result in the message being broken up and sent in
// chunks in those class's tryToStartNewIO methods.  This value is also 
// used by GuaReceiveControlConnection::initiateReceive and by
// GuaReceiveControlConnection::recycleReceiveBuffer.
// -----------------------------------------------------------------------
#define IOSIZEMAX 32000 // 131016 128K - 48 (buffer header) - 8 (memory fragment overhead)
const IpcMessageObjSize GuaMaxMsgIOSize = IOSIZEMAX;

// -----------------------------------------------------------------------
// max length of node names in ASCII format (Guardian actually has a
// lower limit, see GuaNodeNameMaxLen above)
// -----------------------------------------------------------------------
const Int32 IpcNodeNameMaxLength = 100;

// -----------------------------------------------------------------------
// Minimum Priv Stack size.
// -----------------------------------------------------------------------
const Int32 minPrivStackSize = 65536;

// -----------------------------------------------------------------------
// max length of Unix/OSS file names and Guardian file names
// -----------------------------------------------------------------------
const Int32 IpcMaxUnixPathNameLength = 512;
const Int32 IpcMaxGuardianPathNameLength = NA_MAX_PATH;

// -----------------------------------------------------------------------
// A Guardian file number and a Guardian error code
// -----------------------------------------------------------------------
typedef short GuaFileNumber;
const GuaFileNumber InvalidGuaFileNumber = -1;

typedef short GuaErrorNumber;
// some error numbers that are handled in this code
const GuaErrorNumber GuaOK                = FEOK;
const GuaErrorNumber GuaTimeoutErr        = FETIMEDOUT;
const GuaErrorNumber GuaSysmsgReceived    = FESYSMESS;
const GuaErrorNumber GuaInvalidFileType   = FEINVALOP;
const GuaErrorNumber GuaClientCpuDown     = 509; // received "cpu down" sysmsg
const GuaErrorNumber GuaClientNodeDown    = 510; // received "node down" sysmsg
const GuaErrorNumber GuaIpcApplicationErr = 511; // generated by this Ipc layer

// indicate an invalid Guardian reply tag in a message buffer
const short GuaInvalidReplyTag = -1;

// Helps with tracing state changes in IpcConnection 
const Int32 NumIpcConnTraces = 8;

// -----------------------------------------------------------------------
// A network process id, uniquely identifying a process on a node
// (actually, two different implementations for this object exist)
// -----------------------------------------------------------------------
struct GuaProcessHandle
{
  SB_Phandle_Type phandle_;

  NABoolean operator == (const GuaProcessHandle &other) const;
  NABoolean compare(const GuaProcessHandle &other) const;
  NABoolean fromAscii(const char *ascii);
  Int32 toAscii(char *ascii, Int32 asciiLen) const;
  Lng32 decompose(Int32 &cpu, Int32 &pin, Int32 &nodeNumber
                  , SB_Int64_Type &seqNum
                  ) const;

  Int32 decompose2(Int32 &cpu, Int32 &pin, Int32 &node
                  , SB_Int64_Type &seqNum
                  ) const;

  void dumpAndStop(bool doDump, bool doStop) const;
};

class MyGuaProcessHandle : public GuaProcessHandle
{
public:
  // default constructor initializes object with my own process handle
  MyGuaProcessHandle();
};

struct SockProcessId
{
  // with TCP/IP socket-based communication, a process is identified by an
  // IP address (a node identifier) and its listner port
  // (the port to send new connect() requests to)
  SockRawIPAddress ipAddress_;
  SockPortNumber listnerPort_;
};

struct GuaReceiveInfo
{
  // type of I/O 0=sys msg, 1=WRITE, 2=READ, 3=WRITEREAD
  short            ioType_;
  // max possible reply length (reply buffer len of READ/WRITEREAD)
#ifndef USE_SB_NEW_RI
  short            maxReplyLen_;
#else
  Int32            maxReplyLen_;
#endif
  // system-assigned message tag, to be specified in REPLYX
  short            replyTag_;
  // open file number used by client
  short            clientFileNumber_;
  // sync id (mainly for NonStop process pairs
  short            syncId_[2];
  // phandle of the client
  GuaProcessHandle phandle_;
  // open label, assigned by the server upon reply to the open message
  // (used here to store the connection id)
  short            openLabel_;
  SB_Uid_Type      userId_;
};

struct CloseTraceEntry
{
  unsigned short count_; // Number of times traceClose was called
  unsigned short line_; // Line number where traceClose was called
  short clientFileNumber_;
  Int32 cpu_;
  Int32 pin_;
  SB_Int64_Type seqNum_;
};
struct PersistentOpenEntry
{
  GuaProcessHandle persistentOpenPhandle_;
  short persistentOpenFileNum_;
  NABoolean persistentOpenExists_;
};
struct BawaitioxTraceEntry
{
  unsigned short count_;
  ULng32 recursionCount_;
  IpcSetOfConnections *ipcSetOfConnections_;
  CollIndex firstConnectionIndex_;
  IpcConnection *firstConnection_;
  IpcAwaitiox ipcAwaitiox_;
};

class GuaReceiveFastStart
{
public:
  GuaReceiveFastStart();
  NABoolean open_;
  GuaFileNumber receiveFile_;
  GuaErrorNumber openError_;
  NABoolean readUpdate_;
  unsigned char readBuffer_[80];
  _bcc_status readUpdateStatus_;
  unsigned char *bufferData_;
  NABoolean awaitiox_;
  _bcc_status awaitioxStatus_;
  Int32 awaitioxCountTransferred_;
  SB_Tag_Type ioTag_;
  GuaErrorNumber awaitioxError_;
  GuaErrorNumber fileGetInfoError_;
  NABoolean fileGetReceiveInfo_;
  GuaReceiveInfo receiveInfo_;
  GuaErrorNumber fileGetReceiveInfoError_;
  NABoolean replyx_;
  Int32 replyxCountWritten_;
  GuaErrorNumber replyxstatus_;
};

struct NowaitedEspStartup
{
  NowaitedEspServer *nowaitedEspServer_;
  Int32 *procCreateError_;
  void **newPhandle_;
  NABoolean *nowaitedStartupCompleted_;
};
struct NowaitedEspServer
{
  pthread_mutex_t cond_mutex_;
  pthread_cond_t cond_cond_;
  Int64 startTag_;
  Int64 callbackCount_;
  Int64 completionCount_;
  NABoolean waiting_;
  char waitedStartupArg_;
};

class IpcProcessId : public IpcMessageObj
{
public:

  // create a NULL process id
  IpcProcessId();

  // create a process id from a phandle
  IpcProcessId(const GuaProcessHandle &phandle);

  // create a process id from an IP address and a port number
  IpcProcessId(const SockIPAddress &ipAddr, SockPortNumber port);

  // create a process id from an ASCII string (IP addr:port for internet,
  // output of PROCESSHANDLE_TO_FILENAME_ for NSK)
  IpcProcessId(const char *asciiRepresentation);

  // copy constructor
  IpcProcessId(const IpcProcessId &other);

  // Destructor (needed for Tandem compiler for some strange reason)
  ~IpcProcessId() {}

  IpcProcessId & operator = (const IpcProcessId &other);
  inline IpcNetworkDomain getDomain() const             { return domain_; }

  NABoolean operator == (const IpcProcessId &other) const;
  NABoolean match(const IpcNodeName &name,
		  IpcCpuNum cpuNum = IPC_CPU_DONT_CARE) const;

  SockIPAddress getIPAddress() const;
  SockPortNumber getPortNumber() const;
  const GuaProcessHandle &getPhandle() const;
  IpcNodeName getNodeName() const;
  IpcCpuNum getCpuNum() const;
  std::string toString() const;
  Int32 toAscii(char *outBuf, Int32 outBufLen) const;
  void addProcIdToDiagsArea(ComDiagsArea &diags, Int32 stringno = 0) const;

  // make a connection to the process
  IpcConnection *createConnectionToServer(IpcEnvironment *env,
					  NABoolean usesTransactions,
					  Lng32 maxNowaitRequests,
					  NABoolean parallelOpen = FALSE,
					  Int32 *openCompletionScheduled = NULL
                                          ,
                                          NABoolean dataConnectionToEsp = FALSE
                                          ) const;

  // methods needed to pack and unpack the object
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer);

private:

  // the domain under which this process is addressable
  IpcNetworkDomain domain_;
  // to make phandle_ 8-byte aligned
  Int32 spare_;

  union
    {
      GuaProcessHandle phandle_;
      SockProcessId pid_;
    };

  // private methods

  IpcCpuNum getCpuNumFromPhandle() const;
};

typedef enum WaitReturnStatusEnum
  {
    WAIT_OK = 0,    // the wait() terminated with no error
    WAIT_INTERRUPT  // interrupt received during wait()
  } WaitReturnStatus;

// -----------------------------------------------------------------------
// A message queue is a queue of message buffers waiting to be sent
// or to be read
// -----------------------------------------------------------------------
class IpcMessageQueue : public LIST(IpcMessageBuffer *)
{
public:
  IpcMessageQueue(CollHeap *hp=0) : LIST(IpcMessageBuffer *)(hp) {}
};

// -----------------------------------------------------------------------
// An IpcConnection represents a point-to-point connection from one
// process to another.
// -----------------------------------------------------------------------
typedef CollIndex IpcConnectionId;

// -----------------------------------------------------------------------
// IpcConnection eyecatchers, copy only 4 chars and not the '\0'
// indentations show class hierachy
// -----------------------------------------------------------------------
#define eye_IPC_CONNECTION                                          "IPCC"
#define  eye_SQL_TABLE_CONNECTION                                   "STBL"
#define  eye_SCRATCH_FILE_CONNECTION                                "SFIL"
#define  eye_GUA_CONNECTION_TO_SERVER                               "GCTS"
#define  eye_GUA_CONNECTION_TO_CLIENT                               "GCTC"
#define  eye_GUA_MSG_CONNECTION_TO_SERVER                           "GMCS"
#define  eye_SOCKET_CONNECTION                                      "SKTC"
#define   eye_SOCKET_PAIR_CONNECTION                                "SKPC"
#define   eye_SOCKET_LISTNER_PORT_PAIR                              "SKPP"
#define   eye_SOCKET_LISTNER_PORT                                   "SKLP"

#define eye_CONTROL_CONNECTION                                      "CNTL"
#define  eye_GUA_RECEIVE_CONTROL_CONNECTION                         "GRCC"
#define   eye_ESP_GUA_CONTROL_CONNECTION                            "EGCC"
#define  eye_SOCKET_CONTROL_CONNECTION                              "SKCC"
#define   eye_ESP_SOCKET_CONTROL_CONNECTION                         "ESCT"
#define   eye_SEAMONSTER_CONNECTION_TO_CLIENT	                    "SMCC"
#define   eye_SEAMONSTER_CONNECTION_TO_SERVER	                    "SMCS"

// IpcConnectionStateEnum strings, must match the enum below
static const char * IpcConnStateName[] = {
    "INITIAL      ",  // no connection
    "OPENING      ",  // waiting for completion of nowaited open
    "ESTABLISHED  ",  // connection established, we may send
    "SENDING      ",  // we have sent a message, IO isn't complete yet
    "REPLY_PENDING",  // our message is sent, other party may now send
    "RECEIVING    ",  // we are waiting for a reply, pending I/O
    "CANCELLING   ",  // we are trying to cancel a message
    "ERROR_STATE  ",  // the connection is in an error state
    "CLOSED       "   // the connection has been closed
};

class IpcConnection : public NABasicObject
{
public:

  enum IpcConnectionState
  {
    INITIAL = 0,    // no connection
    OPENING,        // waiting for completion of nowaited open
    ESTABLISHED,    // connection established, we may send
    SENDING,        // we have sent a message, IO isn't complete yet
    REPLY_PENDING,  // our message is sent, other party may now send
    RECEIVING,      // we are waiting for a reply, pending I/O
    CANCELLING,     // we are trying to cancel a message
    ERROR_STATE,    // the connection is in an error state
    CLOSED          // the connection has been closed
  };

  static const char *getConnectionStateString(IpcConnectionState s);

 IpcConnection() 
   : fileNumForIOCompletion_(InvalidGuaFileNumber),
    recvStreams_(NULL) {} // recvStream on C++ heap

  virtual ~IpcConnection();

  // send or receive a message through the connection,
  // call the callback when the I/O completes
  virtual void send(IpcMessageBuffer *buffer) = 0;
  virtual void receive(IpcMessageStreamBase *msg) = 0;

  // TRUE if repeated wait calls are allowed on this connection.  
  // Always return TRUE in this base class.
  // Flag set in other connection methods to prevent possible looping 
  // caused by repeated wait calls.
  virtual NABoolean moreWaitsAllowed();

  // wait until a send or receive operation completed
  virtual WaitReturnStatus wait(IpcTimeout timeout, UInt32 *eventConsumed = NULL, IpcAwaitiox *ipcAwaitiox = NULL) = 0;

  // get the state of the connection
  inline IpcConnectionState getState() const            { return state_; }

  // used to display connection type
  const char * getEyeCatcher() const { return eyeCatcher_.nameForObjIdentity; }

  // return the id used to insert a connection into an IpcSetOfConnections
  inline IpcConnectionId getId() const                     { return id_; }

  // get error info
  inline Lng32 getErrorInfo() const                  { return errorInfo_; }
  virtual void populateDiagsArea(ComDiagsArea *&d, CollHeap *diagsHeap) = 0;

  // get environment info
  inline IpcEnvironment *getEnvironment() const   { return environment_; }

  // who are we connected to
  inline const IpcProcessId &getOtherEnd() const     { return otherEnd_; }

  ULng32 getReplySeqNum() { return replySeqNum_; }

  virtual bool isServerSide();

  // safe type cast down the class hierarchy
  virtual SockConnection        *castToSockConnection();
  virtual GuaConnectionToServer *castToGuaConnectionToServer();
  virtual GuaMsgConnectionToServer *castToGuaMsgConnectionToServer();
  virtual GuaConnectionToClient *castToGuaConnectionToClient();

  // Methods to do further status checking of connections: see whether
  // there are I/O operations active at the time and whether unsent or
  // unread message buffers are queued up.
  inline NABoolean isConnected() const
                        { return (state_ != INITIAL && state_ != ERROR_STATE); }
  inline NABoolean sendIOPending() const   { return (state_ == SENDING); }
  inline NABoolean receiveIOPending() const
                                         { return (state_ == RECEIVING); }
  virtual Int32 numQueuedSendMessages() = 0;
  virtual Int32 numQueuedReceiveMessages() = 0;
  virtual Int64 getSqlTableTransid();

#ifdef IPC_INTEGRITY_CHECKING
  // methods used for data structure integrity checking
  void checkIntegrity(NABoolean checkIfOrphan = TRUE);  // traverses to the "top" to begin integrity check
  void checkLocalIntegrity(void);  // checks integrity of this object  
#endif

  // Manage the flag to indicate whether IpcMessageBuffer integrity
  // check should be performed on all incoming buffers.
  NABoolean getTrustIncomingBuffers() const { return trustIncomingBuffers_; }
  void setTrustIncomingBuffers(NABoolean b) { trustIncomingBuffers_ = b; }

  NABoolean breakReceived() const { return breakReceived_; }
  void setBreakReceived(NABoolean b) { breakReceived_ = b; }

  // Number of streams that are waiting for replies
  Int32 numReceiveCallbacksPending() { return (Int32) recvStreams_.entries(); }

  // set the connection state (also manages the pending IOs list in the
  // global set of connections)
  virtual void setState(IpcConnectionState s);

  // Used after fatal error to avoid deadlock.
  virtual void setFatalError(IpcMessageStreamBase *msgStream);

  virtual short getFileNumForIOCompletion()
  {
    return fileNumForIOCompletion_;
  }

  virtual void openPhandle(char * processName, NABoolean parallelOpen);

  IpcMessageBuffer *getLastSentMsg() { return lastSentBuffer_; }

  IpcMessageBuffer *getLastReceivedMsg() { return lastReceivedBuffer_; }


  NABoolean newClientConnection(IpcMessageBuffer *receivedBuffer);

  inline void setSendPersistentOpenReconnect(NABoolean v) { sendPersistentOpenReconnect_ = v; }

  void reportBadMessage();
  
  virtual IpcConnection  *castToSMConnection() { return NULL; }

  virtual NABoolean hasActiveIOs() { return FALSE; }

  virtual void dumpAndStopOtherEnd(bool doDump, bool doStop) const {return ;}

protected:

  // TRUE if connection error occurs.
  // used to avoid wait looping.
  // Checked by the virtual method moreWaitsAllowed().
  NABoolean stopWait_;

  // The Guardian file number that corresponds to this connection for
  // handling of AWAITIOX completion. -1 indicates one of the
  // following:
  // * SQL table
  // * scratch file
  // * seamonster connection
  short fileNumForIOCompletion_;

  NABoolean sendPersistentOpenReconnect_;

  // the constructor is protected since this is a pure virtual object
  IpcConnection(IpcEnvironment *env, const IpcProcessId &pid, const char *eye);
  // set the stopWait_ flag.
  inline void stopWait(NABoolean b)                     { stopWait_ = b; }

  inline void setErrorInfo(Lng32 x)                     { errorInfo_ = x; }
  inline void clearErrorInfo()                         { errorInfo_ = 0; }
  inline void setOtherEnd(const IpcProcessId &pid)    { otherEnd_ = pid; }

  void IOPending();
  void IOComplete();
   
  // ---------------------------------------------------------------------
  // Manage the message queues in front of each connection. The queues
  // may contain two kinds of message buffers: those that are waiting
  // to be sent or read, and those that are currently being sent or
  // received. Use the public methods send, receive, wait, sendIOPending,
  // receiveIOPending, numQueuedSendMessages, and numQueuedReceiveMessages
  // to put buffers into the send queue, retrieve completely received
  // buffers from the receive queue, complete pending I/Os, and check the
  // status of the send and receive queues.
  // ---------------------------------------------------------------------
  inline const IpcMessageQueue &sendQueue() const   { return sendQueue_; }
  inline const IpcMessageQueue &receiveQueue() const {return receiveQueue_;}
  inline CollIndex sendQueueEntries()     { return sendQueue_.entries(); }
  inline CollIndex receiveQueueEntries() {return receiveQueue_.entries();}
  inline void queueSendMessage(IpcMessageBuffer *msg)
                                               { sendQueue_.insert(msg); }
  inline void queueReceiveMessage(IpcMessageBuffer *msg)
                                            { receiveQueue_.insert(msg); 
                                              lastReceivedBuffer_ = msg;
                                            }
  IpcMessageBuffer *getNextSendQueueEntry();
  IpcMessageBuffer *getNextReceiveQueueEntry();
  
  // register a messageStream callback to receive incoming message
  void addReceiveCallback(IpcMessageStreamBase* msgStream)
    { recvStreams_.insert(msgStream); }

  // Manage the flag to indicate whether an IpcMessageBuffer integrity
  // check has failed.
  NABoolean getIpcMsgBufCheckFailed() const { return ipcMsgBufCheckFailed_; }
  void setIpcMsgBufCheckFailed(NABoolean b) { ipcMsgBufCheckFailed_ = b; }

  inline IpcEnvironment *env() const {return environment_; }

  // For tracing state_ changes.
  class IpcConnectionTrace
  {
    friend class IpcConnection;

    IpcConnectionState oldState_;
    IpcMessageBuffer * mostRecentSendBuffer_;
    IpcMessageBuffer * mostRecentReceiveBuffer_;
    struct timespec stateChangeTime_;

    IpcConnectionTrace(void) : 
      oldState_(INITIAL),
      mostRecentSendBuffer_(NULL),
      mostRecentReceiveBuffer_(NULL)
      {
        stateChangeTime_.tv_sec =  0;
        stateChangeTime_.tv_nsec = 0;
      }
  };

  IpcConnectionTrace traceState_ [NumIpcConnTraces];
  Int32 lastTraceIndex_;
  IpcMessageBuffer *lastSentBuffer_;
  IpcMessageBuffer *lastReceivedBuffer_;

protected:

  // Subclasses may want to separate the operations of removing from
  // the send queue and preparing a buffer to be sent (which involves
  // assigning the buffer a sequence number among other things). These
  // two operations together perform the equivalent of
  // getNextSendQueueEntry().
  IpcMessageBuffer *removeNextSendBuffer();
  IpcMessageBuffer *removeNextReceiveBuffer();
  void prepareSendBuffer(IpcMessageBuffer *);
  void removeReceiveStreams();


private:

  // eye catcher
  IpcEyeCatcher eyeCatcher_;

  // the state of the connection (who may send next, etc.)
  IpcConnectionState state_;

  // an index into the global connection table
  IpcConnectionId id_;

  // error information associated with the connection
  Lng32 errorInfo_;

  // environment information
  IpcEnvironment *environment_;

  // which process is this connection connected to on the other end
  IpcProcessId otherEnd_;

  // messages queues for sending/receiving
  IpcMessageQueue sendQueue_;
  IpcMessageQueue receiveQueue_;

  // sequence number of the next expected msg reply. the server side puts
  // seq number in reply buffer before msg send. the client side verifies
  // reply seq number upon msg receive.
  ULng32 replySeqNum_;

  // lists used to match message stream callbacks with incoming messages
  LIST(IpcMessageStreamBase*) recvStreams_;

  // for data structure integrity checking -- used to detect "orphaned" objects
  NABoolean isOrphaned_;  // set to TRUE at beginning of check, set
                          // to FALSE when traversed to, checked at end of check

  //
  // All connections carry a flag indicating whether incoming buffers
  // should be trusted. When buffers are not trusted, a sanity check
  // is performed on each incoming IpcMessageBuffer as it arrives to
  // make sure the chain of IpcMessageObj instances in that buffer
  // does not extend beyond the buffer.
  //
  // When one of these sanity checks fails, the IpcConnection subclass
  // should set ipcMsgBufCheckFailed_ to TRUE and transition to the
  // ERROR_STATE state.
  //
  // Currently the only connection object that does not trust incoming
  // buffers is the client-side connection to the UDR server.
  //
  NABoolean trustIncomingBuffers_;
  NABoolean ipcMsgBufCheckFailed_;

  // A flag indicating whether a break was received while waiting on the
  // connection.
  NABoolean breakReceived_;
}; // class IpcConnection

// -----------------------------------------------------------------------
// A bunch of connections (used to broadcast/multicast messages and
// to wait for any one of the message transfers to complete)
// -----------------------------------------------------------------------

class IpcSetOfConnections : public SUBARRAY(IpcConnection *)
{
public:

  IpcSetOfConnections(IpcAllConnections *superset, CollHeap* hp=0,
                      NABoolean eventDriven=FALSE, NABoolean esp=FALSE);

  // copy ctor
  IpcSetOfConnections (const IpcSetOfConnections & orig, CollHeap * h=0) ;

  // TRUE if repeated wait calls are allowed on ANY connection,
  // FALSE otherwise.
  // used to prevent possible looping caused by repeated wait calls.
  NABoolean moreWaitsAnyConnection(); 

  // used by asynchronous CLI cancel.
  inline void cancelWait(NABoolean b)               { cancelWait_ = b; }

  // Wait on the specified connections (or on a superset of them)
  // until some I/O completes on some connection (not necessarily
  // one of the specified set) or until the timeout expires.
  // If calledByESP then also deletes closed connections and checks that
  // the master process is still alive (if not alive then the ESP stops).
  WaitReturnStatus waitOnSet(IpcTimeout timeout = IpcInfiniteTimeout,
		      NABoolean calledByESP = FALSE,
		      NABoolean *timedout = NULL);
  void infoPendingConnections(char *buffer, int max_len, int *rsp_len);
  void waitOnSMConnections(IpcTimeout timeout);

  // allow iteration over the members of the set, by writing the 
  // following for-loop:
  //
  // IpcSetOfConnections x;
  //
  // for (CollIndex i = 0; x.setToNext(i); i++)
  //   x.element(i)->wait();
  //
  // setToNext returns TRUE if entry at index i is used, else it sets the 
  // value of the index i to the next used connection and returns TRUE as well.
  // FALSE is returned if there are no more connections (beyond i).
  inline NABoolean setToNext(CollIndex &i) const     { return nextUsed(i); }

  inline NABoolean isEsp() const { return esp_; }

#ifdef IPC_INTEGRITY_CHECKING
  // methods used for data structure integrity checking
  void checkIntegrity(void);  // traverses to the "top" to begin integrity check
  void checkLocalIntegrity(void);  // checks integrity of this object  
#endif

private:

  NABoolean cancelWait_;

  // for data structure integrity checking -- used to detect "orphaned" objects
  NABoolean isOrphaned_;  // set to TRUE at beginning of check, set
                          // to FALSE when traversed to, checked at end of check
  IpcAllConnections * allc_;  // so we can traverse there for integrity check
  NABoolean eventDriven_;  // Drive polling by waiting on LDONE
  NABoolean esp_;
  Int64 callCount_;	  // Number of times wait method was called
  Int64 pollCount_;	  // Number of times connections were polled
  Int64 waitCount_;	  // Number of times WAIT was called
  Int64 ldoneCount_;	  // Number of LDONE completions
  Int64 lreqCount_;	  // Number of LREQ completions
  Int64 lsigCount_;	  // Number of LSIG completions
  Int64 smCompletionCount_; // Number of seamonster (LRABBIT) completions
  Int64 timeoutCount_;	  // Number of timeout completions
  Int64 activityPollCount_; // Number of times connections were polled due to activity
  short lastWaitStatus_;  // Last status returned by WAIT
  NABoolean ipcAwaitioxEnabled_; // IPC AWAITIOX(-1) is enabled where applicable
  IpcAwaitiox ipcAwaitiox_; // IocAwaitiox object
};

// -----------------------------------------------------------------------
// A socket-based connection
// -----------------------------------------------------------------------
class SockConnection : public IpcConnection
{
public:

  SockConnection(IpcEnvironment *env,
		 const IpcProcessId &pid,
		 NABoolean thisIsTheControlConnection,
                 const char *eye = eye_SOCKET_CONNECTION);

  // a constructor to use an existing operating system socket in this object
  SockConnection(IpcEnvironment *env, SockFdesc fdesc, NABoolean isClient,
                 const char *eye = eye_SOCKET_CONNECTION);

  virtual ~SockConnection();

  // get the listner port from the client
  // (use only if this is the control connection)
  void receiveClientProcId(IpcProcessId &pid);

  // send and receive (when a callback function is specified, then
  // the operation is non-blocking/nowait)
  virtual void send(IpcMessageBuffer *buffer);
  virtual void receive(IpcMessageStreamBase *msg);

  // wait until some send or receive operation completed
  virtual WaitReturnStatus wait(IpcTimeout timeout, UInt32 *eventConsumed = NULL, IpcAwaitiox *ipcAwaitiox = NULL);

  virtual SockConnection *castToSockConnection();

  // check how many messages are currently queued (for flow control)
  virtual Int32 numQueuedSendMessages();
  virtual Int32 numQueuedReceiveMessages();

  // set error info
  virtual void populateDiagsArea(ComDiagsArea *&diags, CollHeap *diagsHeap);

  // assign this socket to standard input/output and close the existing
  // file descriptor (this assignment stays beyond fork() and exec() calls)
  inline void assignToStdInOut()             { sock_.assignToStdInOut(); }

  //   creates a duplicate handle for fdesc_
  inline SockFdesc getDuplicateFdesc_()  { return sock_.getDuplicateFdesc_(); };

  // public to make the compiler happy
  // A struct to store information about a buffer that is not yet sent
  // or that is currently being received.
  //
  // Possible states of an IO queue in the client:
  //
  // + send() has been called, not sent yet
  //   -> sent_ = FALSE, msg_, sendBuffer_, replyTag_ set, receiving_ = FALSE
  //
  // + sent, no reply received yet and receive() not called yet
  //   -> sent_ = TRUE, msg_, replyTag_ set, sendBuffer_ = NULL,
  //      recvBuffer_ = NULL, receiving_ = FALSE
  //
  // + sent, reply data received but receive() not called yet
  //   -> sent_ = TRUE, msg_, recvBuffer_ set, sendBuffer_ = NULL,
  //      receiving_ = FALSE
  //
  // + sent, no reply data received but receive() has been called
  //   -> sent_ = TRUE, msg_, replyTag_, whenDone_ set, sendBuffer_ = NULL,
  //      recvBuffer_ = NULL, receiving_ = TRUE
  //
  // + received (callback has been called)
  //   -> entry is gone from the list
  //
  // Possible states of an IO queue in the server:
  //
  // + receive() has been called, no data received yet
  //   -> sent_ = FALSE, sendBuffer_ = NULL, recvBuffer_ = NULL,
  //      receiving_ = TRUE,
  //
  // + data has been received and the receive callback has been called
  //   -> sent_ = FALSE, replyTag_ set, recvBuffer_ = NULL,
  //      sendBuffer_ = NULL, receiving_ = FALSE,
  //
  // + send() has been called to reply, data has not been sent yet
  //   -> sent_ = FALSE, sendBuffer_, replyTag_ set, receiving_ = FALSE,
  //
  // + reply has been sent
  //   -> entry is gone from the list
  //
  struct socketIOQueueEntry
  {
    NABoolean              sent_;         // true if sent
    IpcMessageStreamBase   * msg_;        // msg stream associated with this IO
    IpcMessageBuffer       * sendBuffer_; // buffer to send or NULL
    IpcMessageBuffer       * recvBuffer_; // received buffer or NULL
    short                  replyTag_;     // to match reply from server
    NABoolean              receiving_;    // has receive() been called yet?
  };

protected:

  inline void setFdesc(SockFdesc fdesc, NABoolean isClient);
  inline SockSocket & socket()                           { return sock_; }
    SockPortNumber connect(const SockIPAddress &ipAddr, SockPortNumber port);

private:

  SockSocket sock_;

  // the actual port number used for the connection (the server id has
  // the port number of the listener port which is used to initiate new
  // connections)
  SockPortNumber port_;

  // Is this the client or the server part of the connection? Needed because
  // client supports multiple sends, server supports multiple receives
  // at the same time.
  NABoolean isClient_;

  // the last reply tag assigned
  short lastReplyTag_;

  LIST(socketIOQueueEntry *) ioq_;

  // private methods

  // try to start another send operation
  void tryToSendMore();
};

// -----------------------------------------------------------------------
// A socket pair connection to a server that is started with fork()
// and exec(). A server that uses a socket pair connection can only
// have a single connection back to its client. No other clients can
// talk to the server, but the client can have other servers.
// -----------------------------------------------------------------------

class SockPairConnection : public SockConnection
{
public:

  // create both ends of a socket pair connection ("this" is the client)
  SockPairConnection(IpcEnvironment *env,
                     const char *eye = eye_SOCKET_PAIR_CONNECTION);
  SockPairConnection(IpcEnvironment *env,SockIPAddress ipAddr,SockPortNumber port);

  // create the server end of a socket pair connection, pass in the already
  // created socket's file descriptor
  SockPairConnection(IpcEnvironment *env,SockFdesc fd);

  virtual ~SockPairConnection();

  // return the other end of the connection (does this only once)
  SockPairConnection *otherEnd();

  void doConnectNow();

private:

  SockPairConnection *otherEnd_;
  SockPortNumber port_;
  SockIPAddress  ipAddr_;
};

// -----------------------------------------------------------------------
// A Guardian connection on the client side that connects to a server
// by opening its process file
// -----------------------------------------------------------------------
class GuaConnectionToServer : public IpcConnection
{
  friend class IpcGuardianServer;

public:

  GuaConnectionToServer(  IpcEnvironment *env
			, const IpcProcessId &procId
			, NABoolean usesTransactions
			, unsigned short nowaitDepth
			, const char *eye = eye_GUA_CONNECTION_TO_SERVER
			, NABoolean parallelOpen = FALSE
			, Int32 *openCompletionScheduled = NULL
                        , NABoolean dataConnectionToEsp = FALSE
                        );

  virtual ~GuaConnectionToServer();

  // send or receive a message through the connection,
  // call the callback when the I/O completes
  virtual void send(IpcMessageBuffer *buffer);
  virtual void receive(IpcMessageStreamBase *msg);

  virtual NABoolean moreWaitsAllowed();

  // wait until a send or receive operation completed
  virtual WaitReturnStatus wait(IpcTimeout timeout, UInt32 *eventConsumed = NULL, IpcAwaitiox *ipcAwaitiox = NULL);

  virtual GuaConnectionToServer *castToGuaConnectionToServer();

  virtual Int32 numQueuedSendMessages();
  virtual Int32 numQueuedReceiveMessages();

  inline GuaErrorNumber getGuardianError() const { return guaErrorInfo_; }

  inline short getFileNumForLogging() const          { return openFile_; }

  // set error info
  virtual void populateDiagsArea(ComDiagsArea *&diags, CollHeap *diagsHeap);

  // Used after fatal error to avoid deadlock.
  virtual void setFatalError(IpcMessageStreamBase *msgStream);

  virtual short getFileNumForIOCompletion()
  {
    if (getState() == OPENING)
      return openFile_;
    else
      return fileNumForIOCompletion_;
  }

  virtual void openPhandle(char * processName = NULL, NABoolean parallelOpen = FALSE);

 inline Int32 getOpenRetries() const { return openRetries_; }
 inline void setOpenRetries(Int32 num) { openRetries_ =  num; }
 inline unsigned short getNowaitDepth() { return nowaitDepth_; }
 void openRetryCleanup();

  // struct is public only to make the compiler happy
  struct ActiveIOQueueEntry
  {
    // how many bytes have been sent in this operation (0 for a READX)
    IpcMessageObjSize bytesSent_;

    // what's the size of the receive buffer (0 for WRITEX)
    IpcMessageObjSize receiveBufferSizeLeft_;

    // what's the offset in buffer_ where the I/O buffer started
    IpcMessageObjSize offset_;

    // the message buffer to be sent
    IpcMessageBuffer *buffer_;
    
    // the message buffer to be received
    IpcMessageBuffer *readBuffer_;

    // I/O tag = -1 means no oustanding I/O or I/O already completed.
    // I/O tag >= 0 means entry has a no-wait I/O outstanding.
    // I/O tag value cannot exceed nowaitDepth_-1.
    short ioTag_;

    // TRUE if an I/O is in progress for this entry, FALSE otherwise
    bool inUse_;
   };

   virtual NABoolean hasActiveIOs();

   virtual void dumpAndStopOtherEnd(bool doDump, bool doStop) const ;
private:

  // Try to issue one nowait WRITEREADX call and return
  // TRUE if one of these operations was successfully started.
  NABoolean tryToStartNewIO();

  // close the connected server process
  void closePhandle();

  void addSendCallbackBuffer(IpcMessageBuffer *buffer);
  NABoolean removeSendCallbackBuffer(IpcMessageBuffer *buffer);

  void handleIOError();
  void handleIOErrorForStream(IpcMessageStreamBase *msgStream);
  void handleIOErrorForEntry(ActiveIOQueueEntry &entry);
  void cleanUpActiveIOEntry(ActiveIOQueueEntry &entry);

  // ---------------------------------------------------------------------
  // The send and receive queues of a Guardian connection to a server are
  // managed like this:
  // 
  // - Guardian Send operations are started in the order they are
  //   they are called, but may complete in any order.
  // - The send() method places the new message at the end of the send
  //   queue. Buffers in the send queue are not physically sent yet.
  // - If less than <nowait depth> operations are active and if buffers
  //   are in the send queue, send as many as possible, leaving one
  //   possible I/O operation open for out-of-band data.
  // - If a buffer is longer than the max. length for WRITEREADX, then
  //   send it in chunks. The server MUST NOT reply with data to any
  //   chunk. After all chunks are sent, issue a read on
  //   the beginning of the buffer. This completes the send part.
  // - If a buffer is completely sent (immediately if this is a single
  //   chunk message), the send callback is called.
  // - The receive() call by the user looks for a buffer in the receive
  //   queue first. If such a buffer exists, the receive callback is
  //   called and the buffer is removed from the receive queue.
  //   Otherwise, the oldest outstanding receive operation is found
  //   and branded with the receive callback specified in receive().
  // - If an AWAITIOX operation completes, we check whether a partial
  //   buffer has come back. If this is the case, a new READX request
  //   is started immediately to redrive the I/O and read another chunk.
  //   Otherwise, call the receive callback if it has been assigned
  //   already or add the buffer to the receive queue if it doesn't have
  //   a receive callback assigned yet. This means that the receive
  //   queue contains buffers whose I/Os have completed but for which
  //   no receive() call has been issued yet.
  // ---------------------------------------------------------------------

  // open file number to the server
  GuaFileNumber openFile_;
  Int32 *openCompletionScheduled_;

  // how many WRITEREADX operations can be active at a time, also the
  // number of entries in the circular array activeIOs_
  unsigned short    nowaitDepth_;

  // Max size of a raw message sent through this connection (this value
  // MUST NOT be larger than the max message size of the server's control
  // connection).
  IpcMessageObjSize maxIOSize_;

  // A dynamically allocated circular array of nowaitDepth_ entries,
  // one for each outstanding I/O. See srEntry() method on how
  // this circular array is managed. Add entries by incrementing
  // numOutstandingIOs_ and remove entries by calling removeHead().
  ActiveIOQueueEntry *activeIOs_;

  // the index of the last entry allocated (initially, set to nowaitDepth_ - 1)
  unsigned short    lastAllocatedEntry_;

  // Number of outstanding WRITEREADX operations.
  // Must be less than nowaitDepth_ if no out-of-band data is sent and
  // may be less or equal to nowaitDepth_ if out-of-band data has been sent.
  unsigned short    numOutstandingIOs_;

  // pointer to a buffer that is currently being sent in chunks and total
  // number of bytes sent for that buffer
  IpcMessageBuffer  *partiallySentBuffer_;
  IpcMessageObjSize chunkBytesSent_;

  // pointer to a buffer that is currently being received in chunks and total
  // number of bytes requested/actually received for that buffer; also
  // remember whether that buffer had its receive callback added yet
  IpcMessageBuffer  *partiallyReceivedBuffer_;
  IpcMessageObjSize chunkBytesRequested_;
  IpcMessageObjSize chunkBytesReceived_;

  // a list of send callback buffers. for each message stream that uses this
  // connection, there is a send callback buffer on the list corresponding
  // to that stream. the send callback buffer is added to the list before
  // the first chunk is sent. after the last chunk is sent, we remove the
  // send callback buffer from the list and invoke the send callback.
  IpcMessageBuffer **sendCallbackBufferList_;

  // does the connection propagate transaction ids to the server?
  NABoolean         usesTransactions_;

  // information about the error returned from Guardian in case the
  // connection is in the ERROR state
  GuaErrorNumber    guaErrorInfo_;
  NABoolean         dataConnectionToEsp_;
  NABoolean         self_;
  Int32             openRetries_;
  struct timespec   beginOpenTime_;
  struct timespec   completeOpenTime_;
  NABoolean         tscoOpen_;
};

// -----------------------------------------------------------------------
// A Guardian connection from the server to the client via $RECEIVE
// -----------------------------------------------------------------------
class GuaConnectionToClient : public IpcConnection
{
  friend class GuaReceiveControlConnection;

public:

  virtual ~GuaConnectionToClient();

  // send or receive a message through the connection,
  // call the callback when the I/O completes
  virtual void send(IpcMessageBuffer *buffer);
  virtual void receive(IpcMessageStreamBase *msg);

  // wait until any send or receive operation on $RECEIVE completed
  virtual WaitReturnStatus wait(IpcTimeout timeout, UInt32 *eventConsumed = NULL, IpcAwaitiox *ipcAwaitiox = NULL);

  virtual GuaConnectionToClient *castToGuaConnectionToClient();

  virtual Int32 numQueuedSendMessages();
  virtual Int32 numQueuedReceiveMessages();

  inline GuaErrorNumber getGuardianError() const { return guaErrorInfo_; }

  inline short getFileNumForLogging() const  { return clientFileNumber_; }

  // set error info
  virtual void populateDiagsArea(ComDiagsArea *&diags, CollHeap *diagsHeap);

  NABoolean thisIsMyClient(const GuaProcessHandle &phandle,
			   GuaFileNumber fileNo) const;

  // client has gone away
  void close(NABoolean withError = FALSE,
	     GuaErrorNumber gerr = GuaOK);

  virtual bool isServerSide();

  virtual void dumpAndStopOtherEnd(bool doDump, bool doStop) const; 

#if 0
  inline char * receivedMsgHdr() const { return receivedMsgHdr_; }
  inline void incrReceivedMsgHdrInd()
  {
    if (receivedMsgHdrInd_ == 7)
      receivedMsgHdrInd_ = 0;
    else
      receivedMsgHdrInd_ += 1;
  }
  inline short receivedMsgHdrInd() const { return receivedMsgHdrInd_; }
#endif

private:

  // ---------------------------------------------------------------------
  // A Guardian connection to a client doesn't directly manipulate
  // $RECEIVE, it uses the control connection instead.
  //
  // Whenever a message arrives in $RECEIVE, the control connection
  // determines which GuaConnectionToClient object is the receiver.
  // Remember that there may only be one GuaConnectionToClient object
  // per client OPEN (a combination of client phandle, client file #).
  // Incoming messages are given to the connection with the acceptBuffer()
  // method.
  //
  // - The send queue maintained by the IpcConnection base class contains
  //   messages that have been passed in through a send() call and that
  //   don't have a reply tag attached to them. Those messages have no
  //   corresponding request yet. As more requests come in, the replies
  //   will get matched up with them
  // ---------------------------------------------------------------------

  // client's open file number
  // (incoming messages can be identified by client phandle/client file #)
  GuaFileNumber clientFileNumber_;

  // Error info
  GuaErrorNumber    guaErrorInfo_;

  // this connection shares $RECEIVE with the control connection and with
  // all other connections to Guardian clients
  GuaReceiveControlConnection *controlConnection_;

  // pointer to a buffer that is currently being sent in chunks and total
  // number of bytes sent for that buffer
  IpcMessageBuffer  *partiallyRepliedBuffer_;
  IpcMessageObjSize chunkBytesReplied_;

  // pointer to a buffer that is currently being received in chunks and total
  // number of bytes received for that buffer
  IpcMessageBuffer  *partiallyReceivedBuffer_;
  IpcMessageObjSize chunkBytesReceived_;
  Lng32 numOutstandingRequests_;
#if 0
  char *receivedMsgHdr_;
  short receivedMsgHdrInd_;
#endif

  // private methods

  // the constructor is private because a client connection gets created
  // only as part of an incoming open message; users get pointers to
  // client connections via IpcControlConnection::getConnection() and
  // GuaReceiveControlConnection::actOnSystemMessage().
  GuaConnectionToClient(IpcEnvironment *env,
			const IpcProcessId &clientProcId,
			GuaFileNumber clientFileNumber,
			GuaReceiveControlConnection *controlConnection,
                        const char *eye = eye_GUA_CONNECTION_TO_CLIENT);

  // send next buffer through the control connection 
  NABoolean startReplyingToNextRequest();

  // accept an incoming client request from the control connection,
  void acceptBuffer(IpcMessageBuffer  *buffer,
		    IpcMessageObjSize receivedDataLength);
  void incrNumOutstandingRequests() { numOutstandingRequests_++; }
  void decrNumOutstandingRequests() { numOutstandingRequests_--; }

};

// -----------------------------------------------------------------------
// The connection through which a server gets controlled by its owner.
// Exactly one object of this type exists in every server. This is
// an abstract base class, derived objects exist for each domain.
// -----------------------------------------------------------------------
class IpcControlConnection
{
public:

  IpcControlConnection(IpcNetworkDomain domain,
                       const char *eye = eye_CONTROL_CONNECTION) : domain_(domain)
                      {
                        str_cpy_all((char *) &eyeCatcher_, eye, 4);
                        numRequestors_ = 0;
                      }
  virtual ~IpcControlConnection();

  // get the control connection (there may be alternate control connections,
  // too)
  virtual IpcConnection *getConnection() const = 0;

  // through which domain is this control connection addressable
  inline IpcNetworkDomain getDomain() const            { return domain_; }

  // safe casting down the class hierarchy
  virtual SockControlConnection *castToSockControlConnection();
  virtual GuaReceiveControlConnection * castToGuaReceiveControlConnection();

  // get number of requestors
  inline Lng32 getNumRequestors() const          { return numRequestors_; }

protected:

  inline void incrNumRequestors()                    { numRequestors_++; }
  inline void decrNumRequestors()                    { numRequestors_--; }

private:

  IpcNetworkDomain  domain_;        // which domain is the control
                                    // connection in
  Lng32              numRequestors_; // how many processes are requestors
  IpcEyeCatcher     eyeCatcher_;    // eye catcher

};

// -----------------------------------------------------------------------
  // a special connection that listens for connect requests from
  // other processes to a given port and accepts them
// -----------------------------------------------------------------------
class SockListnerPort : public SockConnection
{
public:

  // constructor
  SockListnerPort(IpcEnvironment *env,
		  SockFdesc fdesc,
		  SockControlConnection *cc,
                  const char *eye = eye_SOCKET_LISTNER_PORT);

  // it is not allowed to send or receive on a listner port, it accepts
  // connection requests only and is always in the RECEIVING state
  virtual void send(IpcMessageBuffer *buffer);
  virtual void receive(IpcMessageStreamBase *msg);

  // wait until a connect request is made and initiate a new accept() call
  virtual WaitReturnStatus wait(IpcTimeout timeout, UInt32 *eventConsumed = NULL, IpcAwaitiox *ipcAwaitiox = NULL);

  SockPortNumber getListnerPortNum() const     { return listnerPortNum_; }

private:

  // pointer back to the control connection to call acceptNewConnectionRequest
  SockControlConnection *cc_;
  SockPortNumber listnerPortNum_;
};

// -----------------------------------------------------------------------
// A control connection for a process that was forked by inetd,
// by its client, or by another process. It communicates to its client
// via stdin and stdout which are bound to sockets. New clients are
// accepted from a special listner port which is created.
// -----------------------------------------------------------------------
class SockControlConnection : public IpcControlConnection
{
public:

  SockControlConnection(IpcEnvironment *env,
                        const char *eye = eye_SOCKET_CONTROL_CONNECTION);

  SockControlConnection(IpcEnvironment *env,
                        Int32 inheritedSocket,
                        Int32 passedPort,
                        const char *eye = eye_SOCKET_CONTROL_CONNECTION);

  IpcConnection *getConnection() const;

  SockControlConnection *castToSockControlConnection();

  SockPortNumber getListnerPortNum() const     { return listnerPortNum_; }

  // this method is called when a new process tries to connect
  virtual void acceptNewConnectionRequest(SockConnection *conn);

private:

  // the control connection to the client (use polymorphism)
  SockConnection *controlConnection_;

  // the port through which new connections to this server can be created
  SockSocket      listnerSocket_;
  SockPortNumber  listnerPortNum_;
  SockListnerPort *listnerPort_;
};

// -----------------------------------------------------------------------
// A control connection for a Guardian server using $RECEIVE
// -----------------------------------------------------------------------

class GuaReceiveControlConnection : public IpcControlConnection
{
  friend class GuaConnectionToClient;

public:

  GuaReceiveControlConnection(IpcEnvironment *env,
			      short receiveDepth = 4000,
                              const char *eye = eye_GUA_RECEIVE_CONTROL_CONNECTION,
			      GuaReceiveFastStart *guaReceiveFastStart = NULL);

  virtual IpcConnection *getConnection() const;

  GuaReceiveControlConnection * castToGuaReceiveControlConnection();

  // ---------------------------------------------------------------------
  // The Guardian $RECEIVE control connection manages the OS file $RECEIVE
  // and dispatches incoming messages to the correct connections. To do
  // this, the object mainatains a set of all Guardian-based connections
  // to a client (which share $RECEIVE). There is one GuaConnectionToClient
  // object for each file open that a client does on our process' $RECEIVE.
  // Multiple IpcMessageStream objects can share the same client
  // connection, though.
  // 
  // Receive buffer management: note that READUPDATEX operations complete
  // in arbitrary order, so it is not possible to use specific receive
  // buffers for specific requests. So, we use a one-size-fits-all
  // approach for the buffers. There is no limit on the number of
  // buffers, as the protocol is already limited by the nowait depth.
  // 
  // ---------------------------------------------------------------------

  // wait for an event to happen on $RECEIVE, return whether it did
  WaitReturnStatus wait(IpcTimeout timeout = IpcInfiniteTimeout, UInt32 *eventConsumed = NULL, IpcAwaitiox *ipcAwaitiox = NULL);

  // Handle an incoming system message. For open messages only, a new
  // connection is created and passed in newConnection. The default
  // implementation ignores all system messages except that it refuses
  // all open messages except the first one.
  virtual void actOnSystemMessage(
       short                  messageNum,
       IpcMessageBufferPtr    sysMsg,
       IpcMessageObjSize      sysMsgLen,
       short                  clientFileNumber,
       const GuaProcessHandle &clientPhandle,
       GuaConnectionToClient  *connection);

  inline void setUserTransReplyTag(short urt)
                      { userTransReplyTag_ = urt; switchToUserTransid(); }

  // needed for LRU when esp initiates its own transaction
  inline void setBeginTransTag(Lng32 transTag)
    { beginTransTag_ = transTag; }
  inline Lng32 getBeginTransTag()
    { return beginTransTag_; }
  inline void clearBeginTransTag()
    { beginTransTag_ = -1; }

  // On Linux, if an ESP starts a transaction, before replying to the master,
  // it has to restore the original transaction (since the TM and Seabed do
  // not handle this on Linux). The original transaction handle is saved here,
  // to allow this to be done easily.
  void setOriginalTransaction(short *txHandle);
  short *getOriginalTransaction();
  void clearOriginalTransaction();
  inline void setTxHandleValid()
    { txHandleValid_ = TRUE; }
  inline void clearTxHandleValid()
    { txHandleValid_ = FALSE; }
  inline NABoolean isTxHandleValid()
    { return txHandleValid_; }
  inline IpcSetOfConnections *getClientConnections()
    { return &clientConnections_; }

  inline IpcEnvironment * getEnv() const                  { return env_; }
  inline GuaErrorNumber getGuaErrorInfo() const  { return guaErrorInfo_; }
  inline short getActiveTransReplyTag() const  { return activeTransReplyTag_; }
  void waitForMaster();
  // open file number if $RECEIVE (always 0)
  GuaFileNumber receiveFile_;

protected:

  // did the first client open this process yet?
  NABoolean initialized_;

private:

  // remember the environment
  IpcEnvironment *env_;
  
  // the first control connection (the connection to the client that
  // sent the first open message)
  IpcConnection *firstClientConnection_;

  // a set of all GuaConnectionToClient objects that share $RECEIVE
  IpcSetOfConnections clientConnections_;

  // set of those connections whose clients failed or are unreachable
  IpcSetOfConnections failedConnections_;

  // a pool of free receive buffers that can be used
  IpcMessageQueue receiveBufferPool_;

  // a count on how many connections are currently in the receiving
  // state and waiting for data from $RECEIVE
  Lng32 numReceivingConnections_;

  // a pool of receive buffers that are in use by an outstanding
  // READUPDATEX (search for buffer here when AWAITIOX completes)
  IpcMessageQueue activeReceiveBuffers_;

  // how many READUPDATEX operations can be active at a time
  short receiveDepth_;

  // Max size of an incoming raw message (receive buffers must be this
  // long to avoid losing data on incoming long request messages)
  IpcMessageObjSize maxIOSize_;

  // how many nowaited READUPDATEX calls can be active at a time?
  // A number between 1 and receiveDepth_
  Lng32 maxOutstandingIOs_;

  // a count of how many active READUPDATEX calls we have
  // (minimum of (maxOutstandingIOs_,numActiveReceiveCalls_,1))
  Lng32 numOutstandingIOs_;

  // a count of how many outstanding REPLYX calls we have
  // (numOutstandingIOs_ + numOutstandingRequests_ <= receiveDepth_)
  Lng32 numOutstandingRequests_;

  // For certain operations such as LRU, the master does not initiate
  // a transaction. instead each of the esps initiates their own transactions.
  // if the transaction is initiated by an ESP, then remember the trans tag
  // returned from BEGINTRANSACTION. later the trans tag can be used by ESP
  // to invoke RESUMETRANSACTION and resume the transaction that was initiated
  // by BEGINTRANSACTION.
  Lng32 beginTransTag_;

  // Transaction handle of transaction received by an ESP via the message
  // system on Linux.
  // On Linux, if an ESP starts a transaction, before replying to the master,
  // it has to restore the original transaction (since the TM and Seabed do
  // not handle this on Linux). The original transaction handle is saved here,
  // to allow this to be done easily.
  NABoolean txHandleValid_;
  SB_Transid_Type txHandle_;

  // the last IO reply tag that determines the current transid.
  // the last non-chunk IO reply tag that determines the implicit transid.
  // the explicit user transaction reply tag in use at this time (if any).
  short activeTransReplyTag_;
  short implicitTransReplyTag_;
  short userTransReplyTag_;

  // the Guardian error code in case something went wrong
  // (many errors in $RECEIEVE handling are fatal and abort the process)
  GuaErrorNumber guaErrorInfo_;


  GuaReceiveFastStart *guaReceiveFastStart_;

  // private methods

  // reply to a given request, using the REPLYX system call
  void sendReplyData(IpcMessageBufferPtr data,
		     IpcMessageObjSize   size,
		     short               replyTag,
		     IpcConnection       *conn, // for debugging only
		     GuaErrorNumber      retcodeToClient);

  // initiate a READUPDATEX operation on $RECEIVE
  void initiateReceive(NABoolean newReceive = TRUE);

  // make sure the user-specified transid is the current one
  void switchToUserTransid();

  // find a client connection with the given description of the client
  GuaConnectionToClient *findConnection(
       short                  openLabel);
  GuaConnectionToClient *findConnection(
       short                  clientFileNumber,
       const GuaProcessHandle &clientPhandle);

  // recycle a message buffer that has become free (decrements its refcount)
  void recycleReceiveBuffer(IpcMessageBuffer *b);

  // mark a connection whose client has died or is unreachable
  void markAsDead(GuaConnectionToClient *c,
		  GuaErrorNumber gerr);

public:

  // for error injection testing.
  virtual NABoolean fakeErrorFromNSK(short errorFromNSK, 
                             GuaProcessHandle *clientPhandle)  {return FALSE; }
 
};

// -----------------------------------------------------------------------
// A message buffer (the container in which the message travels)
// It consists of a header struct which is not transmitted across the
// wire and of a space for the actual message. The reference count
// indicates how many other objects (message streams, connections) have
// a pointer to this object. Message buffers are often passed from one
// object to another without changing the reference count (e.g. via
// IpcConnection::send, IpcMessageBuffer::callReceiveCallback,
// IpcMessageStream::giveMessageTo, IpcMessageStream::internalActOnReceive).
// The reply tag and reply length are only used by Guardian connections
// to clients and help matching a reply with its corresponding client
// request. Callbacks get deposited by message streams during send and
// receive operations and get called when those operations complete.
// -----------------------------------------------------------------------
struct InternalMessageBufferHeader
{
protected:

  // ---------------------------------------------------------------------
  // Make the length of this struct a multiple of 8. This is to avoid
  // any alignment padding between this header structure and the data
  // region of the IpcMessageBuffer.
  //
  // Currently we compile with 32-bit pointers. The struct has a size
  // of 48 bytes and the sizes of datatypes in individual fields are:
  //
  //   IpcMessageObjSize   4 bytes
  //   short               2 bytes
  //   IpcMessageRefCount  4 bytes
  //   Int64               8 bytes
  //   pointers            4 bytes
  //
  // ---------------------------------------------------------------------

  IpcMessageObjSize      maxLength_;// length of buffer following this struct
  IpcMessageObjSize      msgLength_;// length of msg to send / received msg

  short                  flags_;
  short                  replyTag_; // Guardian receive message tag for REPLYX
  IpcMessageObjSize      maxReplyLength_; // max len of a reply to recvd msg

  IpcMessageRefCount     refCount_; // how many msges & connections use it
  IpcMessageObjSize      maxIOSize_;// IO transmission chunk size

  Int64                  transid_;  // save context transid

  IpcMessageStreamBase  *message_;  // what is the message (used for callbacks)
  CollHeap              *heap_;     // can point to NAMemory or is NULL if 
                                    // object was allocated on C++ heap

  IpcConnection         *connection_; // connection, source of IO callback
  ARRAY(CollIndex)      *chunkLockCount_; // if buffer shared, controls
                                          // (un)lock of memory chunk

  InternalMessageBufferHeader(CollHeap *heap,
                              IpcMessageObjSize maxLen,
                              IpcMessageObjSize msgLen,
                              IpcMessageStreamBase *msg,
                              short replyTag,
                              IpcMessageObjSize maxReplyLength,
                              Int64 transid,
                              short flags)
  {
    maxLength_ = maxLen;
    msgLength_ = msgLen;
    flags_ = flags;
    replyTag_ = replyTag;
    maxReplyLength_ = maxReplyLength;
    refCount_ = 1;
    maxIOSize_ = 0;
    transid_ = transid; 
    message_ = msg;
    heap_ = heap;
    connection_ = NULL;
    chunkLockCount_ = NULL;

#ifdef NA_DEBUG
    assert((sizeof(InternalMessageBufferHeader) % 8) == 0);
#endif
  }

};


// -----------------------------------------------------------------------
// same as above, but with <length_> bytes of extra space appended
// -----------------------------------------------------------------------
class IpcMessageBuffer : private InternalMessageBufferHeader
{
public:

  // A friend function to enable internal integrity checks in
  // IpcMessageBuffer objects.
  friend NABoolean verifyIpcMessageBufferBackbone(IpcMessageBuffer &);

  // all destroyed buffers should have a refcount of 1
  // (similar to the IpcMessageObj class)
  ~IpcMessageBuffer()
  {
    // Earlier code would assert refCount_ == 1 and we now assert
    // refCount_ is 0. It does not make sense that the destructor
    // would run with a ref count of 1 because decrRefCount() calls
    // the destructor only when the ref count is 0. It is possible
    // that this destructor actually never runs because we always call
    // heap->deallocateMemory() instead of "delete".
    assert(refCount_ == 0);
  }

  inline Int64 getTransid() { return transid_; }
  inline void setTransid(Int64 transid) { transid_ = transid; }

  // There is no constructor to make this object, the static member
  // function allocate needs to be used instead, so the object can
  // only be created on the heap
  static IpcMessageBuffer *allocate(IpcMessageObjSize maxLen,
				    IpcMessageStreamBase *msg,
				    CollHeap *heap,
                                    short flags);

  // create new empty buffer (defaults to same size buffer)
  IpcMessageBuffer *createBuffer(IpcEnvironment *env,
                                 IpcMessageObjSize newMaxLen = 0,
                                 NABoolean failureIsFatal = TRUE);

  // make a 1:1 copy of a buffer, allow to specify a new length
  // (if there is a reply tag in the buffer, then the copy gets it)
  IpcMessageBuffer *copy(IpcEnvironment *env,
			 IpcMessageObjSize newMaxLen = 0,
                         NABoolean failureIsFatal = TRUE);

  // make a copy of a buffer starting from the offset location for newMaxLen
  // (if there is a reply tag in the buffer, then the copy gets it)
  IpcMessageBuffer *copyFromOffset(IpcEnvironment *env,
                                   IpcMessageObjSize newMaxLen,
                                   IpcMessageObjSize offset,
                                   NABoolean failureIsFatal = TRUE);
                                   
  // resize a message buffer (if refcount > 1 caller gets a copy)
  IpcMessageBuffer *resize(IpcEnvironment *env,
			   IpcMessageObjSize newMaxLen);

  // to make the compiler happy, write a matching delete operator for the
  // placement new
  void operator delete(void *ptr)       { ::operator delete(ptr); }

  // get the buffer length of this struct (the buffer follows the header
  // fields and gets allocated by the allocate() method)
  inline IpcMessageObjSize getBufferLength() const  { return maxLength_; }

  // get the total length of this struct
  inline IpcMessageObjSize getTotalLength() const
                                    { return maxLength_ + sizeof(*this); }

  // get message stream (where it makes sense)
  inline IpcMessageStreamBase *getMessageStream() const {return message_;}

  // get connection (where it makes sense)
  inline IpcConnection* getConnection() const      { return connection_; }

  // get and set the actual message length (the part that gets transferred)
  inline IpcMessageObjSize getMessageLength() const { return msgLength_; }
  inline void setMessageLength(IpcMessageObjSize l)    { msgLength_ = l; }

  // remember the message tag and reply length for Guardian REPLYX
  inline short getReplyTag() const                   { return replyTag_; }
  inline void setReplyTag(short tag)                  { replyTag_ = tag; }
  inline IpcMessageObjSize getMaxReplyLength() const
                                               { return maxReplyLength_; }
  inline void setMaxReplyLength(IpcMessageObjSize l)
                                                  { maxReplyLength_ = l; }

  // get a pointer to the data byte number <i>
  inline IpcMessageBufferPtr data(CollIndex i = 0)
                        { return &((IpcMessageBufferPtr) (this + 1))[i]; }

  // deal with reference counts, delete buffer once its count drops to 0
  inline IpcMessageRefCount incrRefCount()         { return ++refCount_; }
  IpcMessageRefCount decrRefCount();
  inline IpcMessageRefCount getRefCount() const      { return refCount_; }
  
  CollIndex initLockCount(IpcMessageObjSize maxIOSize);
  CollIndex incrLockCount(IpcMessageObjSize offset);
  CollIndex decrLockCount(IpcMessageObjSize offset);
  CollIndex getLockCount(IpcMessageObjSize offset);

  // returns TRUE if this is a multi-chunk buffer shared by multi-connections
  inline NABoolean isShared() { return (chunkLockCount_ != NULL); }

  // Handle alignment, increment offset to the next 8 byte boundary
  // Message objects always start on 8 byte boundaries and the header
  // objects all have a size that is a multiple of 8, so that all user
  // objects should always be aligned for all currently existing
  // hardware platforms. This makes it possible to read the user object
  // out of the message buffer directly. Note: this alignment algorithm
  // is fixed and independent of the actual alignment of the target
  // platform, since we want to exchange messages between platforms (some day).
  static void alignOffset(IpcMessageObjSize &offset);

  // method to change and call the callback specified by I/O operations
  inline void addCallback(IpcMessageStreamBase *msg) { message_ = msg; }
  void callSendCallback(IpcConnection *conn);
  void callReceiveCallback(IpcConnection *conn);

  // Verify that the chain of message objects in this buffer does not
  // extend beyond the buffer.
  NABoolean verifyBackbone();

private:

  IpcMessageBuffer(); // Do not implement a default constructor

  // Private constructor. Only called by public methods such as
  // allocate(), createBuffer(), copy(), copyFromOffset().
  IpcMessageBuffer(CollHeap *heap,
                   IpcMessageObjSize maxLen,
                   IpcMessageObjSize msgLen,
                   IpcMessageStreamBase *msg,
                   short flags,
                   short replyTag,
                   IpcMessageObjSize maxReplyLength,
                   Int64 transid);

  // "placement new" to allocate the right size at the right place
  void * operator new(size_t)
                         { ABORT("must use placement new"); return (void *)0xDEADBEEF; }

  // Private operator new for class IpcMessageBuffer. Only called by
  // public methods such as allocate(), createBuffer(), copy(),
  // copyFromOffset().
  //
  // If env is NULL or if env->getHeap() is NULL the new object is
  // allocated by global new. Otherwise the object is allocated by
  // env->getHeap()->allocateMemory().
  void * operator new(size_t headerSize,
		      IpcMessageObjSize bufferLength,
		      CollHeap *heap,
                      NABoolean failureIsFatal);

  // The following operator delete will be called if initialization throws
  // an exception.  It is needed to remove a warning from the .NET 2003
  // compiler
  void operator delete(void *ptr, IpcMessageObjSize bufferLength,
                       CollHeap *heap, NABoolean bIgnore);

};

// -----------------------------------------------------------------------
// Message header struct used in class IpcMessageStream (defined below)
// -----------------------------------------------------------------------

struct InternalMsgHdrInfoStruct : public IpcMessageObj
{
public:
  // constructor
  InternalMsgHdrInfoStruct(
       IpcMessageType msgType,
       IpcMessageObjVersion version);

  // constructor used to perform copyless receive. maps packed objects in place.
  InternalMsgHdrInfoStruct(IpcBufferedMsgStream* msgStream);
  
  // get header sequence number (used to preserve send order)
  // 32-bit seq number
  UInt32  getSeqNum() const
    { return(seqNum_); }

  // set header sequence number
  // 32-bit seq number
  void setSeqNum(UInt32  seqNum)
    { seqNum_ = seqNum; }

  short getSockReplyTag() const
    { return(sockReplyTag_); }

  void setSockReplyTag(short sockReplyTag)
    { sockReplyTag_ = sockReplyTag; }

  // is last buffer in multi-buffer message?
  NABoolean isLastMsgBuf() const
  { return (flags_  & IPCMSG_LAST_BUF ? TRUE : FALSE); }

  // set last buffer in multi-buffer message
  void setLastMsgBuf()
  { flags_ |= IPCMSG_LAST_BUF; }

  // Flag for last buffer in a batch of SeaMonster replies
  NABoolean getSMLastInBatch() const
  { return (flags_  & IPCMSG_SM_LAST_IN_BATCH ? TRUE : FALSE); }
  void setSMLastInBatch()
  { flags_ |= IPCMSG_SM_LAST_IN_BATCH; }

  // get sending message stream id
  // stream id is actually the pointer to the stream
  Long getMsgStreamId() const
    { return(msgStreamId_); }

  // set message stream id
  // stream id is actually the pointer to the stream
  void setMsgStreamId(Long id)
    { msgStreamId_ = id; }

  // get the message length from the received message header
  IpcMessageObjSize getMsgLengthFromData() { return totalLength_; }

  // override method to get length information (packed length = actual length)
  IpcMessageObjSize packedLength();

  // override pack method
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);

  // override unpack method to do some error checking
  void unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer);

  enum IpcMessageObjFlags
    {
      // Last buffer in a multi-buffer message
      IPCMSG_LAST_BUF            = 0x0001,

      // Last buffer in a SeaMonster reply batch
      IPCMSG_SM_LAST_IN_BATCH    = 0x0002
    };

  // data fields (all public, to be used by IpcMessageStream only)
  // total length of these fields is 24 bytes (divisible by 8)
  // Note that the OS related code peeks at incoming messages to determine
  // the message length and that it uses this struct for this purpose!!!

  IpcMessageObjSize totalLength_;  // total length of the message incl. header
  short             alignment_;    // 1,2,4,8
  short             flags_;        // enum IpcMessageObjFlags
  short             format_;       // compressed, ...
  short             sockReplyTag_; // spare for Guardian, reply tag for sock.
  Int32               eyeCatcher_;   // bit pattern to detect junk messages
  UInt32      seqNum_;       // sequence number to preserve send order 
  // stream id is actually the pointer to the stream
  Long		msgStreamId_;  // stream id for coalescing multi-buf msg

};

#if 0
struct MsgTraceEntry
{
  InternalMsgHdrInfoStruct internalMsgHdrInfoStruct_;
  void * bufAddr_;
  unsigned int sentReceivedLength_;
};
#endif

// eye catcher for SQL/ARK messages (NOAH in big-endian HAON in little-endian)
const Lng32 Release1MessageEyeCatcher = 1313816904;

// default buffer size value for non-shared message streams
const IpcMessageObjSize DefaultInitialMessageBufSize = 2048;

// -----------------------------------------------------------------------
// IpcMessageStream eyecatchers, copy only 4 chars and not the '\0'
// indentations show class hierachy
// -----------------------------------------------------------------------
#define eye_IPC_MESSAGE_STREAM_BASE                                 "MBAS"
#define  eye_IPC_MESSAGE_STREAM                                     "STRM"
#define   eye_ESP_CONTROL_MESSAGE                                   "ECNS"
#define   eye_MASTER_ESP_MESSAGE                                    "MEMS"
#define   eye_SPLIT_BOTTOM_REQUEST_MESSAGE                          "SPBR"
#define   eye_SPLIT_BOTTOM_SAVED_MESSAGE                            "SPBS"
#define   eye_UDR_CONTROL_STREAM                                    "UCNS"
#define    eye_UDR_CLIENT_CONTROL_STREAM                            "UCCS"
#define   eye_NEW_INCOMING_CONNECTION_STREAM                        "NICS"
#define  eye_IPC_BUFFERED_MESSAGE_STREAM                            "BSTM"
#define   eye_IPC_CLIENT_MSG_STREAM                                 "CLSM"
#define    eye_SEND_TOP_MSG_STREAM                                  "SDTM"
#define    eye_SEND_TOP_CANCEL_MESSAGE_STREAM                       "SDTC"
#define    eye_UDR_CLIENT_DATA_STREAM                               "UCDS"
#define   eye_IPC_SERVER_MSG_STREAM                                 "SRSM"
#define    eye_SEND_BOTTOM_WORK_MESSAGE_STREAM                      "SDBM"
#define    eye_SEND_BOTTOM_CANCEL_MESSAGE_STREAM                    "SDBC"

// -----------------------------------------------------------------------
// Very simple base class for IpcMessageStream and BufferedMessageStream.
// This base class is used in IpcMessageBuffer objects when they point
// to a message stream.
// -----------------------------------------------------------------------

class IpcMessageStreamBase : public NABasicObject
{
public:

  IpcMessageStreamBase(IpcEnvironment *env)
    : environment_(env)
  {}
  virtual ~IpcMessageStreamBase()
  {}

  virtual IpcMessageStream *     castToIpcMessageStream();
  virtual IpcBufferedMsgStream * castToIpcBufferedMsgStream();

  // internal send call back must be redefined by derived classes.
  virtual void internalActOnSend(IpcConnection* connection) = 0;
                 
  // internal receive call back mustbe redefined by derived classes.
  virtual void internalActOnReceive(IpcMessageBuffer* buffer,
                                    IpcConnection* connection) = 0;

  // A callback cannot directly delete its own message stream object, since
  // upon return from the callback some more administrative tasks may happen.
  // To achieve the same effect as a "delete this", the callback can call
  // the following method instead, which will put "this" on a list of
  // message streams to delete. The delete is then performed at
  // a point in time when it is safe to do.
  void addToCompletedList();

protected:
  IpcEnvironment *environment_;

};

// -----------------------------------------------------------------------
// An object of class IpcMessageStream is a collection of other objects in
// some process that the process wants to share with others. The
// IpcMessageStream object therefore acts as a container. When creating a
// message object, the creator specifies whether he is willing to share
// the objects directly with the message system (or with other processes)
// or whether copies of all objects should be made immediately when they
// get added to the message.
// -----------------------------------------------------------------------
class IpcMessageStream : public IpcMessageStreamBase
{
friend class IpcMessageStreamBase;
public:

  enum MessageStateEnum
    {
      EMPTY,        // no data in the message, ready for send or receive
      COMPOSING,    // user is adding objects to the (yet unsent) message
      SENDING,      // message is being sent
      SENT,         // send operation has completed (msg is empty)
      RECEIVING,    // outstanding receive operation on the message
      RECEIVED,     // message was received
      EXTRACTING,   // user is extracting received objects (next state: empty)
      ERROR_STATE,  // an error occurred
      BREAK_RECEIVED
    };

  IpcMessageStream(IpcEnvironment *env,
		   IpcMessageType msgType = IPC_MSG_INVALID,
		   IpcMessageObjVersion version = 1,
		   IpcMessageObjSize fixedMsgBufferLength = 0,
		   NABoolean shareMessageObjects = FALSE);

  virtual ~IpcMessageStream();

  virtual IpcMessageStream *castToIpcMessageStream();

  // accessor methods
  inline IpcMessageType getType() const               { return h_.getType(); }
  inline IpcMessageObjVersion getVersion() const   { return h_.getVersion(); }
  inline void setType(IpcMessageType t)                     { h_.setType(t); }
  inline void setVersion(IpcMessageObjVersion v)         { h_.setVersion(v); }
  enum MessageStateEnum getState()                          { return state_; }
  inline Lng32 getErrorInfo() const                    { return errorInfo_; }
  IpcMessageObjSize getFixedMsgBufferLength() const   { return fixedBufLen_; }

  // Include an object into a message
  IpcMessageStream & operator << (IpcMessageObj & toAppend);

  // Extract an object from a message into an existing object of the same
  // type (this will call the virtual(!) method toRetrieve.unpackObj())
  inline IpcMessageStream & operator >> (IpcMessageObj & toRetrieve)
  {
    extractNextObj(toRetrieve, FALSE);
    return *this;
  }

  // Extract a pointer to an object from a message
  // (not recommended for objects that have virtual functions)
  // The object has to be released by calling toRetrieve->decrRefCount()
  IpcMessageStream & operator >> (IpcMessageObj * &toRetrieve);

  inline IpcMessageObjSize getMaxReplyLength() const
    { return maxReplyLength_; }
  inline void setMaxReplyLength(IpcMessageObjSize l)
    { maxReplyLength_ = l; }

  // Extract an object from a message into an existing object of the
  // same type and optionally perform an integrity check on the packed
  // object before extracting the data. This will call the virtual
  // methods toRetrieve.checkObj() and toRetrieve.unpackObj().
  //
  // "stream >> obj" is equivalent to "stream.extractNextObj(obj, FALSE)"
  NABoolean extractNextObj(IpcMessageObj &toRetrieve, NABoolean checkObjects);

  // check whether there are more objects to extract
  NABoolean moreObjects()                         { return current_ != NULL; }

  // get information about the next object to be retrieved
  IpcMessageObjType getNextObjType()           { return current_->getType(); }
  IpcMessageObjVersion getNextObjVersion()  { return current_->getVersion(); }
  IpcMessageObjSize getNextObjSize()       { return current_->s_.objLength_; }

  // Check whether the message is in use by the send/receive methods
  inline NABoolean hasIOPending() const  { return (activeIOs_.entries() > 0); }

  // remove all objects from the message (discards unread or unsent ones)
  void clearAllObjects();

  // get/set the recipients of a message to send
  inline const IpcSetOfConnections & getRecipients() const
                                                   { return recipients_; }
  void addRecipient(IpcConnection *recipient);
  void addRecipients(const IpcSetOfConnections &recipients);
  void deleteRecipient(IpcConnection *recipient);
  void deleteAllRecipients();

  // pass a received message on to another message stream object
  // (allows to dispatch messages from a central receiver to multiple
  // dependent message stream objects)
  void giveMessageTo(IpcMessageStream &other, IpcConnection *connection);

  // give message to new class IpcBufferedMsgStream
  // (temporary to bridge routing from old to new message stream)
  void giveReceiveMsgTo(IpcBufferedMsgStream& msgStream);

  // send a message and call the actOnSend callback
  // once for each completed send operation to a recipient
  void send(NABoolean waited = TRUE, Int64 transid = (Int64)-1);

  // Receive a message and call the actOnReceive callback when done.
  // If <waited> is set to TRUE, all receive operations on all connections
  // are completed and the receive callback is called for each of them.
  // In the case of multiple recipients (senders), the receive callback
  // has to handle the extraction of message objects from the message or
  // all but the objects from the last received message are lost. In the
  // case of receiving from a single connection, either the callback or
  // the caller of the receive method can do the extraction. The receive
  // method never reads more than one message from each of the connections.
  void receive(NABoolean waited = TRUE);

  // Wait a certain time for some I/O to complete. Use the isIOPending()
  // method to check whether all I/Os for this message have been completed,
  // if a timeout other than the default value is used.
  WaitReturnStatus waitOnMsgStream(IpcTimeout timeout = IpcInfiniteTimeout);

  // safe cast to ExMasterEspMessage class
  virtual ExMasterEspMessage * castToExMasterEspMessage(void);

  // abort any outstanding I/Os on this stream 
  void abandonPendingIOs();

#ifdef IPC_INTEGRITY_CHECKING
  // methods used for data structure integrity checking
  void checkIntegrity(void);  // traverses to the "top" to begin integrity check
  void checkLocalIntegrity(void);  // checks integrity of this object  
#endif

protected:

  // to be implemented by users of the IpcMessageStream object: application
  // code to be performed when replies or requests arrive or when
  // a message gets sent(default is to do nothing)
  virtual void actOnSend(IpcConnection *connection);
  virtual void actOnSendAllComplete();
  virtual void actOnReceive(IpcConnection *connection);
  virtual void actOnReceiveAllComplete();

  // to be used by Guardian client message streams that manipulate transactions
  inline short getReplyTag() const   { return msgBuffer_->getReplyTag(); }

  // exercise assertions on the other side of the connection.
  void corruptMessage() { corruptMessage_ = true; }

private:

  // header fields that get put into the message
  InternalMsgHdrInfoStruct h_;

  // fields that do not get sent with the message
  IpcMessageBuffer      *msgBuffer_;    // current message buffer
  IpcMessageObjSize     fixedBufLen_;   // user-specified max. len of messages
  IpcMessageObjSize     maxReplyLength_;// max len of a reply to recvd msg
  NABoolean             shareObjects_;  // share msg objects with caller?
  NABoolean             objectsInBuffer_;// objects reside inside the buffer
  IpcSetOfConnections   recipients_;    // communication partners
  IpcSetOfConnections   activeIOs_;     // active communication partners
  IpcMessageObj         *tail_;         // last object in linked object list
  IpcMessageObj         *current_;      // current object in linked obj list
  Lng32                  errorInfo_;     // fix this later to contain error info
  ULng32         numOfSendCallbacks_;
  MessageStateEnum      state_;         // state of the message (buffer)

  // private methods

  // get the first object in the message (this is the header itself)
  IpcMessageObj *first() { return &h_; }

  // remove all objcts from inside the current message buffer,
  // checking for leftover references, allocate a message buffer
  void clearMessageBufferContents();
  void allocateMessageBuffer(IpcMessageObjSize len);
  void resizeMessageBuffer(IpcMessageObjSize len);

  // the following functions implement virtual callback functions
  // by invoking the virtual methods msg->actOnSend(), msg->actOnReceive()
  // after performing some administrative work
  void internalActOnSend(IpcConnection *connection);
  void internalActOnReceive(IpcMessageBuffer *buffer,
			    IpcConnection *connection);

  // for data structure integrity checking
  NABoolean isOrphaned_;

  bool corruptMessage_;

};


// -----------------------------------------------------------------------
// IpcBufferedMsgStream
// -----------------------------------------------------------------------
class IpcBufferedMsgStream : public IpcMessageStreamBase
{
friend class IpcClientMsgStream;
friend class IpcServerMsgStream;
friend class IpcMessageStream;
friend class IpcMessageStreamBase;

public:

  // constructor
  IpcBufferedMsgStream(IpcEnvironment *env,
                       IpcMessageType msgType,
                       IpcMessageObjVersion version,
                       Lng32 inUseBufferLimit,
                       IpcMessageObjSize bufferSize);
  // destructor
  virtual ~IpcBufferedMsgStream();
               
  virtual IpcBufferedMsgStream *castToIpcBufferedMsgStream();

  // get next receive message from input queue.
  // This method must be called before any message objects can be unpacked.
  // The return value indicates whether a complete receive message exists.
  // IpcMessageObjType parameter is the type of the sending message stream.
  // This method will not advance to the next receive message until
  // all message objects in the current receive message are processed 
  // via method receiveMsgObj(), OR until the current message is given to
  // another peer message stream via method giveReceiveMsgTo().
  NABoolean getNextReceiveMsg(IpcMessageObjType& msgType);

  // get next message object type from current receive message.
  // The return value indicates whether another message objects exists. The 
  // message stream does not advance to the next message object until it is
  // actually unpacked via the method receiveMsgObj(). 
  NABoolean getNextObjType(IpcMessageObjType& msgType);

  // get next message object size from current receive message
  IpcMessageObjSize getNextObjSize() const;

  // get a pointer to the next packed object in the current receive message.
  // Use this method in conjunction with IpcMessageObj::operator new() to
  // unpack the message object. This method advances the message stream to the
  // next message object in the current receive message. NULL is returned if
  // no more messages objects exist in the current receive message.
  IpcMessageObj* receiveMsgObj();

  // give current receive message to a peer message stream for processing.
  // The current receive message is available to give away until it is
  // released implicitly when the next message is advanced via method
  // getNextReceiveMsg().
  void giveReceiveMsgTo(IpcBufferedMsgStream& msgStream);

  // pack an object in the current send message
  IpcBufferedMsgStream& operator << (IpcMessageObj& obj);

  // unpack the next object in the current receive message
  inline IpcBufferedMsgStream& operator >> (IpcMessageObj& obj)
  {
    extractNextObj(obj, FALSE);
    return *this;
  }

  // Extract an object from a message into an existing object of the
  // same type and optionally perform an integrity check on the packed
  // object before extracting the data. This will call the virtual
  // methods toRetrieve.checkObj() and toRetrieve.unpackObj().
  //
  // "stream >> obj" is equivalent to "stream.extractNextObj(obj, FALSE)"
  NABoolean extractNextObj(IpcMessageObj &toRetrieve, NABoolean checkObjects);

  // allocate space for a packed object in the current send message.
  IpcMessageObj* sendMsgObj(IpcMessageObjSize packedObjLen);

   // cleanup unpacked message buffers with objects no longer inuse
  void cleanupBuffers();
  
   // recalibrate garbage collection and release message buffers
  void releaseBuffers()
    { 
      garbageCollectLimit_ = 0;
      cleanupBuffers();
    }

  // get number of inuse buffers (unpacked messages with objects
  // still in use by the application)
  Lng32 numOfInUseBuffers() const
    { return inUseBufList_.entries(); }

  // get number of buffers in the input mesage queue
  Lng32 numOfInputBuffers() const
    { return inBufList_.entries(); }

  // get number of buffers in the output mesage queue
  Lng32 numOfOutputBuffers() const
    { return outBufList_.entries(); }
  
  // get number of buffers in the send mesage queue
  Lng32 numOfSendBuffers() const
    { return sendBufList_.entries(); }
  
  // get number of pending reply tags
  Lng32 numOfReplyTagBuffers() const
    { return replyTagBufList_.entries(); }

  // get last error information from connection 
  Lng32  getErrorInfo() const
    { return errorInfo_; }

  // check limit of in use message buffers.
  NABoolean inUseLimitReached() const
    { return((numOfInUseBuffers() + numOfInputBuffers()) >= inUseBufferLimit_); }

  IpcMessageObjSize getBufferSize() { return bufferSize_; }

  Lng32 getInUseBufferLimit() const { return inUseBufferLimit_; }

  virtual IpcConnection *getConnection();

   void setSMContinueProtocol(NABoolean b) { smContinueProtocol_ = b; }
   NABoolean getSMContinueProtocol() const { return smContinueProtocol_; }

   void setSMLastInBatch();

protected:

  // user send call back, Application code to process error handling
  virtual void actOnSend(IpcConnection* connection) = 0;
  virtual void actOnSendAllComplete();

  // user receive call back, Application code to invoke inbound msg processing
  virtual void actOnReceive(IpcConnection* connection) = 0;
  virtual void actOnReceiveAllComplete();

  // add a message buffer to the input queue.
  void addInputBuffer(IpcMessageBuffer* inputBuf);

  // get next message buffer from output queue.
  IpcMessageBuffer* getOutputBuffer()
    { IpcMessageBuffer* buf; return(outBufList_.getFirst(buf) ? buf : NULL); }

  // get a copy of the next message buffer in the output queue.
  IpcMessageBuffer* copyOutputBuffer()
    { return(outBufList_.entries() ? 
                                 outBufList_[0]->copy(environment_) : NULL); }

  // prepare send message objects for output and put buffers in output queue.
  void prepSendMsgForOutput();
 
  // call back functions

  // internal send call back may be redefined by derived classes.
  virtual void internalActOnSend(IpcConnection* connection);
                 
  // internal receive call back may be redefined by derived classes.
  virtual void internalActOnReceive(IpcMessageBuffer* buffer,
                                    IpcConnection* connection);

private:

  // copy constructor (should not be called!)
  IpcBufferedMsgStream(const IpcBufferedMsgStream&);
  
  // assignment operator (should not be called!)
  const IpcBufferedMsgStream& operator = (const IpcBufferedMsgStream&);


  IpcMessageObjType msgType_;           // message object type 
  IpcMessageObjVersion msgVersion_;     // message object version 
  IpcMessageObjSize bufferSize_;        // minimum length of message buffers
  Lng32 inUseBufferLimit_;               // inuse receive buffer limit
  Lng32 garbageCollectLimit_;            // inuse buf limit for garbage collect
  Lng32 errorInfo_;                      // error info from connection
  NABoolean receiveMsgComplete_;        // complete receive msg ready to unpack
  IpcMessageBuffer* sendMsgBuf_;        // current send message buffer
  InternalMsgHdrInfoStruct*
                        sendMsgHdr_;    // current send message header
  IpcMessageObj* sendMsgObj_;           // current send msg object being built
  CollIndex receiveMsgBufI_;            // index of receive message buffer
  IpcMessageBuffer* receiveMsgBuf_;     // current receive message buffer
  InternalMsgHdrInfoStruct*
                        receiveMsgHdr_; // current receive message header
  IpcMessageObj* receiveMsgObj_;        // next receive msg obj to be extracted
  IpcMessageQueue sendBufList_;         // current send message buffer list
  IpcMessageQueue receiveBufList_;      // current receive message buffer list
  IpcMessageQueue inBufList_;           // input message queue
  IpcMessageQueue outBufList_;          // output message queue
  IpcMessageQueue inUseBufList_;        // unpacked buffers with objects in use
  IpcMessageQueue replyTagBufList_;     // unpacked buffers with reply tags

  NABoolean smContinueProtocol_;
}; 

// ----------------------------------------------------------------------------
// IpcClientMsgStream
// ----------------------------------------------------------------------------

class IpcClientMsgStream : public IpcBufferedMsgStream
{
public:

  // constructor
  IpcClientMsgStream(IpcEnvironment *env,
                     IpcMessageType msgType,
                     IpcMessageObjVersion version,
                     Lng32 sendBufferLimit,
                     Lng32 inUseBufferLimit,
                     IpcMessageObjSize bufferSize);
                
  // get/set the recipients of a message to send
  inline const IpcSetOfConnections & getRecipients() const
                                                   { return recipients_; }

  // add a remote recipient connection to send request messages to
  void addRecipient(IpcConnection* connection)
   { recipients_ += connection->getId(); }
  
  // add a local recipient message stream to send request messages to
  void addRecipient(IpcServerMsgStream* msgStream)
   { localRecipients_.insert(msgStream); }
  
  // check limit of send message buffers.
  NABoolean sendLimitReached()
    { return (responsesPending_ >= sendBufferLimit_); }

  // get number of responses pending from IpcServerMsgStream
  Lng32 numOfResponsesPending()
    { return responsesPending_; }

  Lng32 getSendBufferLimit() const { return sendBufferLimit_; }

  // broadcast the current send message to all recipients
  void sendRequest(Int64 transid = (Int64)-1);

  // we may want to add the capability to send() to a specific connection
  // rather than broadcast 
  // void sendRequest(IpcConnection* connection, Int64 transid = (Int64)-1); ???

  // we may want to add the capability to send() to a specific local message
  // stream rather than broadcast
  // void sendRequest(IpcServerMsgStream* msgStream, Int64 transid = (Int64)-1); ???

  // abort any outstanding I/Os on this stream
  void abandonPendingIOs();

  // For seamonster
  NABoolean getSMBatchIsComplete() const { return smBatchIsComplete_; }
  void setSMBatchIsComplete(NABoolean x) { smBatchIsComplete_ = x; }

  // internal receive call back 
  void internalActOnReceive(IpcMessageBuffer* buffer,
                            IpcConnection* connection);

protected:

  // internal send call back
  void internalActOnSend(IpcConnection* connection);

private:

  // get next local reply tag
  short getLocalReplyTag()
    { while(++localReplyTag_ == GuaInvalidReplyTag); return(localReplyTag_); }

  Lng32 sendBufferLimit_;           // outstanding request buffer limit
  Lng32 responsesPending_;          // responses pending count
  IpcSetOfConnections recipients_;  // remote connections to receive broadcast
  SET(IpcServerMsgStream*)  
                 localRecipients_;  // local msg streams to receive broadcast
  short localReplyTag_;             // reply tag used for local msg streams

  // For seamonster
  NABoolean smBatchIsComplete_;     // has complete batch been received
  
};

// ----------------------------------------------------------------------------
// IpcServerMsgStream
// ----------------------------------------------------------------------------

class IpcServerMsgStream : public IpcBufferedMsgStream
{
public:

  // constructor
  IpcServerMsgStream(IpcEnvironment *env,
                     IpcMessageType msgType,
                     IpcMessageObjVersion version,
                     Lng32 sendBufferLimit,
                     Lng32 inUseBufferLimit,
                     IpcMessageObjSize bufferSize);

  // set remote client connection to receive request messages from
  void setClient(IpcConnection* connection, NABoolean receive = TRUE)
  {
    if (client_ != NULL)
    {
      client_->dumpAndStopOtherEnd(true, false);
      assert(client_ == NULL);
    }
    client_ = connection;
    if (receive)
      client_->receive(this);
  }
  
  IpcConnection* getClient() 
    { return client_; }

  // check limit of send message buffers.
  NABoolean sendLimitReached()
    { return (numOfOutputBuffers() >= sendBufferLimit_); }
  
  // prepare the current response message for output and continue responding
  // to any pending request.
  void sendResponse();

  // server is done replying to all requests. send empty responses for all
  // pending requests.
  void responseDone();

  // reply to outstanding requests from the output queue
  void tickleOutputIo();

  IpcConnection *getConnection() { return client_; }

protected:

  // internal receive call back 
  void internalActOnReceive(IpcMessageBuffer* buffer,
                            IpcConnection* connection);

  // get next message buffer from output queue matched with next reply tag.
  IpcMessageBuffer* getReplyTagOutputBuffer(IpcConnection*& connection,
                                            IpcBufferedMsgStream*& msgStream);

private:

  IpcConnection* client_;  // remote client connection
  Lng32 sendBufferLimit_;   // output queue response buffer limit

  // The SeaMonster continue protocol allows a batch of replies per
  // request. The batch size is sendBufferLimit_.
  //
  // This is different from the current one-to-one continue protocol
  // where the server sends one buffer for every request and the
  // client sends a continue request after every reply.
  //
  // The buffersSentInBatch_ counter keeps track of how many buffers
  // have been sent in the current batch.
  //
  // When the counter reaches sendBufferLimit_, the header of the last
  // buffer in the batch is marked LAST IN BATCH. This informs the
  // client to decrement stream and connection counters which enables
  // a continue message to be sent by the client.
  Lng32 buffersSentInBatch_;
  
};

// -----------------------------------------------------------------------
// This is an object that is held by the owner of a context-sensitive
// server. It describes the name of the server (ServerId) and its
// properties.
// -----------------------------------------------------------------------
class IpcServer
{
  friend class IpcServerClass;

public:

  void release();

  // accessor methods
  const IpcProcessId & getServerId() const
                             { return controlConnection_->getOtherEnd(); }
  IpcConnection *getControlConnection()      { return controlConnection_; }
  IpcServerClass *getServerClass()                 { return serverClass_; }
  virtual IpcGuardianServer *castToIpcGuardianServer();
  void logEspRelease(const char * filename, int lineNum, 
                     const char *msg = NULL);

protected:

  // the id of the server's service connection (also contains its process id)
  IpcConnection *controlConnection_;

  // each server belongs to a server class
  IpcServerClass *serverClass_;

  // remember the expanded program file name (mainly for error reporting)
  char           progFileName_[IpcMaxGuardianPathNameLength];

  // private methods

  // constructor, to be used by friends and derived classes only,
  // everybody else calls IpcServerClass::allocateServerProcess()
  IpcServer(IpcConnection *controlConnection,
	    IpcServerClass *serverClass);
  virtual ~IpcServer();

  // stop the server process
  virtual void stop();
};

// -----------------------------------------------------------------------
// Specialization for a Guardian server process
// -----------------------------------------------------------------------
class IpcGuardianServer : public IpcServer
{
public:

  IpcGuardianServer(
       IpcServerClass * serverClass,
       ComDiagsArea   **diags,
       CollHeap       *diagsHeap,
       const char     * nodeName,
       const char     * className,
       IpcCpuNum      cpuNum = IPC_CPU_DONT_CARE,
       IpcPriority    priority = IPC_PRIORITY_DONT_CARE,
       IpcServerAllocationMethod allocMethod = IPC_LAUNCH_GUARDIAN_PROCESS,
       short          uniqueTag = 0,
       NABoolean      usesTransactions = FALSE,
       NABoolean      debugServer = FALSE,
       NABoolean      waitedStartup = TRUE,
       Lng32           maxNowaitRequests = 2,
       const char     * overridingDefineForProgFile = "",
       const char     * processName = NULL,
       NABoolean      parallelOpens = FALSE);

  inline void setStateReady()              { serverState_ = READY; }
  inline NABoolean isReady() const       { return serverState_ == READY; }
  inline NABoolean hasError() const      { return serverState_ == ERROR_STATE; }
  inline short getUniqueTag() const                 { return uniqueTag_; }
  NABoolean isCreatingProcess() const
  {  
    serverState state = serverState_;
    NABoolean result = state == CREATING_PROCESS;
    return result;
  }
  inline IpcCpuNum getCpuNum() const           { return cpuNum_; }
  inline NABoolean getRequestedCpuDown() const { return requestedCpuDown_;}
  inline NABoolean getUsesTransactions() const      { return usesTransactions_; }
  inline unsigned short getNowaitDepth() const      { return nowaitDepth_; }
  virtual IpcGuardianServer *castToIpcGuardianServer();

  virtual void stop();

  // do work on the startup process without blocking
  // Call this either for indefinite wait or until either isReady() or
  // hasError() returns TRUE. If isReady() returns true, the connection_
  // data member of the parent class will be set. If hasError() returns
  // TRUE, then the diagnostics area will be set.
  short workOnStartup(IpcTimeout timeout,
		      ComDiagsArea **diags,
		      CollHeap *diagsHeap);

  // caller has a system message that indicates something about the
  // startup of this server (leave type of sys msg unspecified)
  void acceptSystemMessage(const char *sysMsg,
			   Lng32 sysMsgLength);

  short changePriority(Lng32 priority, NABoolean isDelta = FALSE);

  NABoolean serverDied(); // return TRUE iff server is dead

  inline const char *getProcessName() {return processName_; }

  NowaitedEspStartup nowaitedEspStartup_;
  void * newPhandle_;
  NABoolean nowaitedStartupCompleted_;

private:

  // ---------------------------------------------------------------------
  // For nowaited process creation, the state in which the server is
  // (right now this is used only for Guardian connections)
  // ---------------------------------------------------------------------
  enum serverState
  {
    INITIAL,
    CREATING_PROCESS,
    READY,
    ERROR_STATE
  } serverState_;

  // private methods

  void launchProcess(ComDiagsArea **diags, CollHeap *diagsHeap);
  void spawnProcess(ComDiagsArea **diags, CollHeap *diagsHeap);
  void useProcess(ComDiagsArea **diags, CollHeap *diagsHeap);
  void launchNSKLiteProcess(ComDiagsArea **diags, CollHeap *diagsHeap);

  void populateDiagsAreaFromTPCError(ComDiagsArea *&d, CollHeap *diagsHeap);

  // put server cpu location in the given string. e.g. \EJR0101 cpu 1
  void getCpuLocationString(char *location);

  // the node name on which the process is started (determined
  // only if needed at process start time as a function of actualCpuNum_)
  char  * nodeName_;

  // The program file name of the server; if partially qualified it gets
  // expanded with $SYSTEM.SYSTEM as the default. A DEFINE name could also
  // be given but make sure that the resulting name has either the system
  // name unspecified or has the same system as "nodeName_".
  const char  * className_;

  // the requested Trafodion node number to start the process on
  // (for historical reasons this is called a CpuNum); IPC_CPU_DONT_CARE
  // if we don't care which node 
  IpcCpuNum   cpuNum_;

  // the actual Trafodion node where the process was started
  IpcCpuNum   actualCpuNum_;

  // remember if node-down caused server to be created on a CPU 
  // different from the requested one. (tbd - could the IpcConnection's
  // phandle be compared to cpuNum_ to give the same info?)
  NABoolean requestedCpuDown_;

  // the requested priority for the server
  IpcPriority priority_;

  // allocation method, indicates whether PROCESS_LAUNCH_ or
  // PROCESS_SPAWN_ should be used to start the process. If the process file
  // name contains a slash "/", we always use PROCESS_SPAWN_, therefore
  // one method is to specify IPC_LAUNCH_GUARDIAN_PROCESS by default and
  // to pass OSS filenames with a slash in them.
  IpcServerAllocationMethod allocMethod_;

  // a unique tag for this server, used for nowaited I/O operations, or -1
  // if no nowait operations are requested
  short       uniqueTag_;

  // should the connection to the server propagate the client's transaction?
  NABoolean   usesTransactions_;

  // should the server be started in the debugging mode
  NABoolean   debugServer_;

  // should we use nowaited requests to start the server and send the startup
  // message? If yes, call workOnStartup() until either isReady() or hasError()
  // return TRUE.
  NABoolean   waitedStartup_;
  NABoolean   parallelOpens_;

  // max number of concurrent nowait I/Os to the server
  unsigned short nowaitDepth_;

  // a Guardian DEFINE can be passed here that, if it exists and points to
  // an existing file, overrides the given program file name in "className"
  const char  * overridingDefineForProgFile_;

  // processname of the process
  const char * processName_;

  // Two error codes that are set when the state is ERROR. These error
  // codes should be added to the diagnostics area.
  GuaErrorNumber guardianError_;
  Int32            procCreateError_;
  short          procCreateDetail_;

  // when sending messages, don't delete the message until the I/O completed
  char        * activeMessage_;

 NABoolean unhooked_;
};

// -----------------------------------------------------------------------
// Max. length of a server class name (see below)
// -----------------------------------------------------------------------
const Int32 IpcMaxServerClassNameLen = 100;

// -----------------------------------------------------------------------
// A server class object is used to allocate servers of a certain type
// (like sqlcomp, esp, ...).
// -----------------------------------------------------------------------
class IpcServerClass : public NABasicObject
{
public:

  // Constructor; specify a name
  IpcServerClass(IpcEnvironment *env,
		 IpcServerType serverType,
		 IpcServerAllocationMethod allocationMethod =
		 IPC_ALLOC_DONT_CARE,
                 short version = COM_VERS_MXV,
                 char *nodeName = NULL);
  ~IpcServerClass();

  inline IpcEnvironment *getEnv() const           { return environment_; }

  // allocate and free a server
  IpcServer *allocateServerProcess(ComDiagsArea **diags = NULL,
				   CollHeap *diagsHeap = NULL,
				   const char *nodeName = NULL,
				   IpcCpuNum cpuNum = IPC_CPU_DONT_CARE,
				   IpcPriority priority = IPC_PRIORITY_DONT_CARE,
				   Lng32 espLevel = 1,
				   NABoolean usesTransactions = TRUE,
				   NABoolean waitedCreation = TRUE,
				   Lng32 maxNowaitRequests = 2,
				   const char *progFileName = NULL,
				   const char *processName = NULL,
				   NABoolean parallelOpens = FALSE,
				   IpcGuardianServer **creatingProcess = NULL);
  void freeServerProcess(IpcServer *s);
  inline short getServerVersion() { return serverVersion_;}

  inline IpcServerType getServerType() { return serverType_;}

  char *getProcessName(short cpuNum, char *processName);
  NABoolean parallelOpens() { return parallelOpens_; }
  NowaitedEspServer nowaitedEspServer_;
private:

  // server type
  IpcServerType serverType_;

  // allocation method
  IpcServerAllocationMethod allocationMethod_;

  // remember all the servers of this class that are used in this process
  LIST(IpcServer *) allocatedServers_;

  IpcEnvironment *environment_;
  short serverVersion_;
  char *nodeName_;
  NABoolean parallelOpens_;
  // private methods (contain the OS-related code to start the server)
  // (Guardian processes are created by calling the constructor
  // IpcGuardianServer::IpcGuardianServer() and the work procedure
  // IpcGuardianServer::workOnStartup())
  IpcConnection *createInternetProcess(
       ComDiagsArea **diags,
       CollHeap       *diagsHeap,
       const char     *nodeName,
       const char     *className,
       IpcCpuNum      cpuNum = IPC_CPU_DONT_CARE,
       NABoolean      usesTransactions = FALSE,
       SockPortNumber defaultPortNumber = NoSockPortNumber);
// the next port number to allocate
  SockPortNumber  nextPort_;

// process creation using native win32 createprocess api
  IpcConnection *createNTProcess(
	   ComDiagsArea **diags,
       CollHeap       *diagsHeap,
       const char     *nodeName,
       const char     *className,
       IpcCpuNum      cpuNum = IPC_CPU_DONT_CARE,
       NABoolean      usesTransactions = FALSE,
       SockPortNumber defaultPortNumber = NoSockPortNumber);
  IpcConnection *forkProcess(
       ComDiagsArea **diags,
       CollHeap   *diagsHeap,
       const char *nodeName,
       const char *className,
       IpcCpuNum  cpuNum = IPC_CPU_DONT_CARE,
       NABoolean  usesTransactions = FALSE);
};

// -----------------------------------------------------------------------
// A global data structure that holds a pointer to all connections that
// exist within a process. This allows certain global operations (e.g.
// wait for any I/O). This class is part of the Ipc environment, class
// IpcEnvironment.
// -----------------------------------------------------------------------
class IpcAllConnections : public ARRAY(IpcConnection *)
{
  friend class IpcConnection;
  friend class IpcMessageBuffer;

public:

  IpcAllConnections(IpcEnvironment *env, CollHeap *hp = 0, NABoolean esp = FALSE) : ARRAY(IpcConnection*)(hp)
    {
      ipcEnv_ = env;
      pendingIOs_ = new(hp) IpcSetOfConnections(this,hp,TRUE,esp);
      completionSequenceNo_ = 0;
      deleteCount_ = 0;
      recursionCount_ = 0;
      receivedPartialMessage_ = FALSE;
      traceRef_ = NULL;
      numSMConnections_ = 0;
    }
  
  // copy ctor
  IpcAllConnections (const IpcAllConnections & orig, CollHeap * h=0) ; // not written

  // wait for something to happen on any of the connections like awaitio(-1)
  WaitReturnStatus waitOnAll(IpcTimeout timeout = IpcInfiniteTimeout,
			     NABoolean calledByESP = FALSE,
			     NABoolean *timedout = NULL,
                             Int64 *waitTime = NULL);

  // used by asynchronous CLI cancel.
  inline void cancelWait(NABoolean b) const
                                            { pendingIOs_->cancelWait(b); }

  // get those connections that are pending
  inline const IpcSetOfConnections &getPendingIOs() const
                                                   { return *pendingIOs_; }

  // find out how many connections have pending IOs
  inline CollIndex getNumPendingIOs() const
                                         { return pendingIOs_->entries(); }

  // get the sequence number of the last I/O that completed
  inline ULng32 getCompletionSeqenceNo() const
                                         { return completionSequenceNo_; }
  // indicate an I/O completion to global wait procedures
  inline void bumpCompletionCount()           { completionSequenceNo_++; }

  // indicate a connection was closed and should be deleted when no recursion
  inline void incrDeleteCount()		      { deleteCount_++; }

  // indicate a closed connection was deleted (when no recursion)
  inline void decrDeleteCount()		      { deleteCount_--; }

  // get the closed but not deleted count to determine whether to search
  inline ULng32 getDeleteCount()	      { return deleteCount_; }

  // increment on each recursive call to waitOnSet for a subset of all
  inline void incrRecursionCount()		      { recursionCount_++; }

  // decrement on each return from a recursive call to waitOnSet for a subset of all
  inline void decrRecursionCount()		      { recursionCount_--; }

   // get the count of recursive calls to waitOnSet for a subset of all
  inline ULng32 getRecursionCount()	      { return recursionCount_; }

  // indicate to global wait procedures that we received a partial message
  inline void setReceivedPartialMessage(NABoolean flag) 
                            { receivedPartialMessage_ = flag; }

  // find out if we received a partial message                             
  inline NABoolean getReceivedPartialMessage() 
                            { return receivedPartialMessage_; }

  // get the list of connections with pending I/Os and save the node names,
  // CPUs and PINs of the first <n> processes of the other ends in given buff
  CollIndex fillInListOfPendingPins(char *buff, ULng32 buffSize,
                                    CollIndex numOfPins);

  void fillInListOfPendingPhandles(GuaProcessHandle *phandles,
				   CollIndex& numOfPhandles);

#ifdef IPC_INTEGRITY_CHECKING
  // methods used for data structure integrity checking
  void checkIntegrity(void);  // traverses to the "top" to begin integrity check
  void checkLocalIntegrity(void);  // checks integrity of this object  
#endif

  // methods used for Ipc Connection tracing
  void print(); // can be called from the debugger
  void registTraceInfo(IpcEnvironment *env, ExeTraceInfo *ti);
  Int32 printConnTrace(Int32 lineno, char *buf);
  static Int32 getAnEntry(void * mine, Int32 lineno, char * buf)
           { return ((IpcAllConnections *) mine)->printConnTrace(lineno, buf); }
  void infoAllConnections(char *buffer, int max_len, int *rsp_len);
  void printConnTraceLine(char *buffer, int *rsp_len, IpcConnection *conn);

  Int32 getNumSMConnections() { return numSMConnections_; }
  void incrNumSMConnections() { numSMConnections_++; }
  void decrNumSMConnections() { numSMConnections_--; }

private:

  // used by our friends, the connection objects, to change their status
  inline void IOPending(IpcConnectionId id)         { *pendingIOs_ += id; }
  inline void IOComplete(IpcConnectionId id)        { *pendingIOs_ -= id; }

  // the subset of connections that are currently sending or receiving
  IpcSetOfConnections *pendingIOs_;

  // an ever-increasing (with wraparound) counter of completed I/Os
  // (calling a callback for a completed send or receive counts as
  // an I/O completion)
  ULng32 completionSequenceNo_;
  ULng32 deleteCount_;
  ULng32 recursionCount_;

  // A flag to indicate that we received a part of multi-chunk
  // message. This flag is used only to reset the timeout in the
  // wait method after we have received a part of multi-chunk message.
  NABoolean receivedPartialMessage_;

  // A reference to trace registered in global trace repository
  void *traceRef_;
  CollIndex printEntry_;
  Int32 numSMConnections_; // Number of SeaMonster connections
  IpcEnvironment *ipcEnv_;
};

// Constants to indicates how many concurrent requests we allow per
// ESP. The number of concurrent requests limits the number of fragment
// instances that we can download to the ESP, since for each transaction
// we need to be able to send 2 messages simultaneously. There are two
// constants: an initial one to save resources, and a second one for
// large queries to keep the number of ESPs low.  See logic on IpcEnvironment
// constructor that allows these to be overridden with environment variables.

const Int32 InitialNowaitRequestsPerEsp = 15;
const Int32 HighLoadNowaitRequestsPerEsp = 15;

// Ipc Data message type names. must match with the enum IpcMsgOper below
static const char * IpcMsgOperName[] =
{
  "NONE",    // initial state or no action yet
  "SEND",    // for consumer to send request to a producer
  "RECV",    // for consumer to receive reply from a producer
  "ACPT",    // for producer to accept a request from consumer
  "RESP"     // for producer to respond request to a consumner
};

// -----------------------------------------------------------------------
// The environment for IpcIPC objects and procedures.
// Using a pointer to this environment in many objects avoids global
// variables which are a problem in the executor library environment.
// -----------------------------------------------------------------------
class IpcEnvironment : public NABasicObject
{
public:

  IpcEnvironment(CollHeap *heap = NULL, UInt32 *eventConsumed = NULL,
		 NABoolean breakEnabled = FALSE, 
                 IpcServerType serverType = IPC_CLIENT_OR_UNSPECIFIED_SERVER,
		 NABoolean useGuaIpcAtRuntime = FALSE,
                 NABoolean persistentProcess = FALSE);
  ~IpcEnvironment();

  Lng32 getMaxPollingInterval() { return maxPollingInterval_; }
  void setMaxPollingInterval(Lng32 arg) { maxPollingInterval_ = arg; }
  NABoolean getPersistentOpens() { return persistentOpens_; }
  void setPersistentOpens(NABoolean arg) { persistentOpens_ = arg; }
  unsigned short getPersistentOpenAssigned() { return persistentOpenAssigned_; }
  short getPersistentOpenInfo(GuaProcessHandle *otherEnd, short *index);
  void resetPersistentOpen(short index);
  short getNewPersistentOpenIndex();
  void setPersistentOpenInfo(short index, GuaProcessHandle *otherEnd, short fileNum);
  NABoolean getMasterFastCompletion() { return masterFastCompletion_; }
  NABoolean isPersistentProcess() { return persistentProcess_; }

  void stopIpcEnvironment();

  IpcAllConnections *getAllConnections() const { return allConnections_; }
  IpcControlConnection *getControlConnection() const
                                            { return controlConnection_; }

  inline void addToCompletedMessages(IpcMessageStreamBase * m)
    {
      completedMessages_.insert(m);
    };
  void deleteCompletedMessages();
  void setControlConnection(IpcControlConnection *cc);

  inline NABoolean breakEnabled() { return breakEnabled_; }
  inline void setBreakEnabled( NABoolean enabled ){ breakEnabled_ = enabled; }

  inline CollHeap *getHeap() const                       { return heap_; }

  IpcProcessId getMyOwnProcessId(IpcNetworkDomain dom = IPC_DOM_INVALID);
  inline UInt32 * getEventConsumed() { return eventConsumedAddr_; }
  inline void setEvent(NABoolean on, UInt32 event)
	    {
	      if (eventConsumedAddr_ != NULL)
	      {
		if (on)
		  *eventConsumedAddr_ |= event;
		else
		  *eventConsumedAddr_ &= 0xFFFFFFFF ^ event;
	      }
	    }
  inline NABoolean isEvent(UInt32 event)	
     { return eventConsumedAddr_ && *eventConsumedAddr_ & event; }
  inline void setLdoneConsumed(NABoolean ldoneConsumed = TRUE)
	    {
	      if (eventConsumedAddr_ != NULL)
	      {
		if (ldoneConsumed)
		  *eventConsumedAddr_ |= LDONE;
		else
		  *eventConsumedAddr_ &= 0xFFFFFFFF ^ LDONE;
	      }
	    }
  inline NABoolean ldoneConsumed()
    { return eventConsumedAddr_ && *eventConsumedAddr_ & LDONE; }
  inline void setLsigConsumed(NABoolean lsigConsumed = TRUE)
	    {
	      if (eventConsumedAddr_ != NULL)
	      {
		if (lsigConsumed)
		  *eventConsumedAddr_ |= LSIG;
		else
		  *eventConsumedAddr_ &= 0xFFFFFFFF ^ LSIG;
	      }
	    }
  inline NABoolean lsigConsumed()	
    { return eventConsumedAddr_ && *eventConsumedAddr_ & LSIG; }

  inline char ** getEnvVars()                   { return envvars_;}
  inline Lng32 getEnvVarsLen()                   { return envvarsLen_;}
  void setEnvVars(char ** envvars);
  void setEnvVarsLen(Lng32 envvarsLen);

#ifdef IPC_INTEGRITY_CHECKING
  // for debug integrity checking
  void setExRtFragTableIntegrityCheckPtr
    (void (*fnptr) (ExRtFragTable *ft));
  void setCurrentExRtFragTable(ExRtFragTable *ft);
  void removeCurrentExRtFragTable(ExRtFragTable *ft);
  ExRtFragTable * getCurrentExRtFragTable(Lng32 i); 
  void checkIntegrity(void);  // traverses to the "top" to begin integrity check
  void checkLocalIntegrity(void);  // checks integrity of this object
#endif

  IpcPriority getMyProcessPriority();

  // We have a flag to indicate that the IPC heap became full. The
  // flag is not used internally by the instance. It is only placed
  // here because the IpcEnvironment is generally visible from all
  // parts of the CLI and executor that are IPC-aware.
  NABoolean getHeapFullFlag() const { return heapFull_; }
  void setHeapFullFlag(NABoolean b);

  // One other thing we do to manage the IPC heap is keep a "safety"
  // buffer allocated in this instance and when the heap becomes full,
  // the code which detects that can release the safety buffer to
  // guarantee that there is some space left on the heap.
  void releaseSafetyBuffer();
  IpcTimeout getStopAfter() const { return stopAfter_; }
  void setStopAfter(IpcTimeout stopAfter) { stopAfter_ = stopAfter; }
  Int64 getIdleTimestamp() const { return idleTimestamp_; }
  void clearIdleTimestamp() { idleTimestamp_ = 0; }
  void setIdleTimestamp();
  IpcTimeout getInactiveTimeout() const { return inactiveTimeout_; }
  void setInactiveTimeout(IpcTimeout inactiveTimeout)
  { inactiveTimeout_ = inactiveTimeout; }
  Int64 getInactiveTimestamp() const { return inactiveTimestamp_; }
  void clearInactiveTimestamp() { inactiveTimestamp_ = 0; }
  void setInactiveTimestamp();
  Lng32 getEspPrivStackSize() const { return espPrivStackSize_; }
  void setEspPrivStackSize(Lng32 espPrivStackSize) { espPrivStackSize_ = espPrivStackSize; }
  Lng32 getEspFreeMemTimeout() const { return espFreeMemTimeout_; }
  void setEspFreeMemTimeout(Lng32 freeMemTimeout) { espFreeMemTimeout_ = freeMemTimeout; }
  NABoolean getEspCloseErrorLogging() const { return espCloseErrorLogging_; }
  void setEspCloseErrorLogging(NABoolean espCloseErrorLogging) { espCloseErrorLogging_ = espCloseErrorLogging; }
  inline NABoolean useGuaIpcAtRuntime() const { return useGuaIpcAtRuntime_; }
  inline IpcServerType getIpcServerType() { return serverType_; }
  void notifyNoOpens();
  inline IpcMessageObjSize getGuaMaxMsgIOSize() { return guaMaxMsgIOSize_; }

  // This allows setting the threshhold at which the tryToStartNewIO methods
  // switch to chunking.  See define =_SQLMX_MAX_IPC_MSG_SIZE usage in 
  // this class's ctor.
  inline IpcMessageObjSize getGuaMaxMsgIOSize()  const { return guaMaxMsgIOSize_; }

  // Allows testing of send depth limits for control connections.
  // See defines =_SQLMX_NOWAIT_DEPTH_LOW and _HI in this class's ctor.
  inline unsigned short getCCMaxWaitDepthLow() const { return maxCCNowaitDepthLow_; }
  inline unsigned short getCCMaxWaitDepthHigh() const { return maxCCNowaitDepthHigh_; }

  // Supports logging of retried MSG_LINK_ or WRITEREADX calls.
  inline void incrRetriedMessages() { retriedMessageCount_++; }
  void logRetriedMessages(void);

  // Allows testing of per-process Message Quick Cell limits.
  // See define =_SQLMX_MAX_OUTGOING_MSG in this class's ctor.
  inline short getMaxPerProcessMQCs() const { return maxPerProcessMQCs_; }

  void setCliGlobals(CliGlobals *cliGlobals) { cliGlobals_ = cliGlobals; }
  CliGlobals *getCliGlobals() const { return cliGlobals_; }
  inline char getEspAssignByLevel() const { return espAssignByLevel_; }
  inline void incrNumOpensInProgress() { numOpensInProgress_ += 1; }
  inline void decrNumOpensInProgress() { numOpensInProgress_ -= 1; }
  inline short getNumOpensInProgress() const { return numOpensInProgress_; }
  void closeTrace(unsigned short, short, Int32, Int32, SB_Int64_Type);
  void bawaitioxTrace(IpcSetOfConnections *ipcSetOfConnections,
                      ULng32 recursionCount,
                      CollIndex firstConnectionIndex,
                      IpcConnection *firstConnection,
                      IpcAwaitiox *ipcAwaitiox);
  // Methods to aid executor tracing for data send and receive
  void registTraceInfo(ExeTraceInfo *ti);
  void addIpcMsgTrace(IpcConnection *conn, const char mtype,
                      void * bufAddr, Int32 length, char isLast,
                      UInt32 seqNum)
    {
       if (++lastIpcMsgTraceIndex_ >= maxIpcMsgTraceIndex_)
         lastIpcMsgTraceIndex_ = 0;
       ipcMsgTraceArea_[lastIpcMsgTraceIndex_].conn_ = conn;
       ipcMsgTraceArea_[lastIpcMsgTraceIndex_].bufAddr_ = bufAddr;
       ipcMsgTraceArea_[lastIpcMsgTraceIndex_].length_ = length;
       ipcMsgTraceArea_[lastIpcMsgTraceIndex_].sendOrReceive_ = mtype;
       ipcMsgTraceArea_[lastIpcMsgTraceIndex_].isLast_ = isLast;
       ipcMsgTraceArea_[lastIpcMsgTraceIndex_].seqNum_ = seqNum;
    }
  Int32 printAnIpcEntry(Int32 lineno, char *buf);
  static Int32 getALine(void * mine, Int32 lineno, char * buf)
           { return ((IpcEnvironment *) mine)->printAnIpcEntry(lineno, buf); }

  bool getCorruptDownloadMsg() const {return corruptDownloadMsg_; }
  bool getLogReleaseEsp() const { return logReleaseEsp_; }
  bool getLogEspIdleTimeout() const { return logEspIdleTimeout_; }
  bool getLogEspGotCloseMsg() const { return logEspGotCloseMsg_; }
  bool getLogTimeIpcConnectionState() const 
                                    { return logTimeIpcConnectionState_; }
  bool smEnabled() { return seamonsterEnabled_; }
  char const *myProcessName();

  // trace for data send and receive
  enum IpcMsgOper
  {
    UNUSED,    // initial state or no action yet
    SEND,      // for consumer to send request to a producer
    RECEIVE,   // for consumer to receive reply from a producer
    ACCEPT,    // for producer to accept a request from consumer
    RESPOND    // for producer to respond request to a consumner
  };

  struct IpcMsgTrace
  {
    IpcConnection *conn_;   // channel used to send or receive
    void *bufAddr_;         // buffer containing the data
    Int32 length_;          // total sent/received size
    char sendOrReceive_;    // contains enum IpcMsgOper value
    char isLast_;           // indicates if it is the last chunk
    UInt32 seqNum_;         // sequence number of multi-chunk message
  };

private:
  IpcAllConnections    *allConnections_;            // all connections of this process
  IpcControlConnection *controlConnection_;         // if this is a server
  LIST(IpcMessageStreamBase *) completedMessages_;  // messages which will be cleaned up
                                                    // periodically
  CollHeap             *heap_;                      // heap for allocating space

  ExRtFragTable        *currentExRtFragTable_[4]; // for debug integrity checking

  // contains the environment (the exported env vars on oss).
  // Passed onto mxcmp when it is started on release platform.
  // See CliLayerForNsk.cpp, method SetEnviron_InternalNSK for details
  // on how this is set. See spawnProcess method in IPC on how envs
  // are passed on.
  char ** envvars_;
  Lng32 envvarsLen_;

  void (*integrityCheckExRtFragTablePtr_) (ExRtFragTable *ft) ;
  UInt32         *eventConsumedAddr_;      // address of event consumed indicator
  NABoolean            breakEnabled_;
  NABoolean            heapFull_;
  char                *safetyBuffer_;
  NABoolean            useGuaIpcAtRuntime_;
  IpcTimeout           stopAfter_;               // Exit after time interval in microseconds 
  Int64                idleTimestamp_;
  IpcTimeout           inactiveTimeout_;         // Exit after time interval in microseconds 
  Int64                inactiveTimestamp_;
  Lng32                 espPrivStackSize_;        // Maximum esp priv stack size
  NABoolean            espCloseErrorLogging_;    // Log EMS event if close is received with req outstanding
  IpcServerType        serverType_;
  Lng32                 maxPollingInterval_;
  NABoolean             persistentOpens_;
  unsigned short        persistentOpenEntries_; // Entries in array
  unsigned short        persistentOpenAssigned_; // Entries assigned
  PersistentOpenEntry  (*persistentOpenArray_)[1];
  NABoolean            masterFastCompletion_;
  NABoolean             persistentProcess_;
  CliGlobals          *cliGlobals_;              // CliGlobals
  Lng32                 espFreeMemTimeout_;       // secs after which idle ESP frees up memory.
  IpcMessageObjSize    guaMaxMsgIOSize_;
  unsigned short       maxCCNowaitDepthLow_;
  unsigned short       maxCCNowaitDepthHigh_;
  Int32                  retriedMessageCount_;
  short                maxPerProcessMQCs_;

  //  IpcPriority priority_;
  char                 espAssignByLevel_;
  short                numOpensInProgress_;
  CloseTraceEntry (*closeTraceArray_)[closeTraceEntries];
  short closeTraceIndex_;
  BawaitioxTraceEntry (*bawaitioxTraceArray_)[bawaitioxTraceEntries];
  short bawaitioxTraceIndex_;
  // Executor trace related, see ComExeTrace.h for more info
  IpcMsgTrace *ipcMsgTraceArea_; // Array of IpcMsgTrace entries
  Int32 lastIpcMsgTraceIndex_;    // points to the last used entry
  Int32 maxIpcMsgTraceIndex_;     // max index value
  void *ipcMsgTraceRef_;          // pointer to this trace in the repository
  bool corruptDownloadMsg_;
  bool logReleaseEsp_;
  bool logEspIdleTimeout_;
  bool logEspGotCloseMsg_;
  bool logTimeIpcConnectionState_;
  bool seamonsterEnabled_;
  char myProcessName_[PhandleStringLen];
};

// -----------------------------------------------------------------------
// Default heap for IPC, using global operator new and delete
// -----------------------------------------------------------------------
#include <iosfwd>
using namespace std;

// convenience function to make sure a diagnostics area is allocated
void IpcAllocateDiagsArea(ComDiagsArea *&diags, CollHeap *diagsHeap);

// -----------------------------------------------------------------------
// Overload global operator new with an IpcEnvironment
// -----------------------------------------------------------------------
void * operator new(size_t size, IpcEnvironment *env);
void * operator new[](size_t size, IpcEnvironment *env);

void operator delete(void *p, IpcEnvironment *env);

char *getServerProcessName(IpcServerType serverType,
                           short cpuNum, char *processName, short *envType = NULL);

#endif /* IPC_H */
