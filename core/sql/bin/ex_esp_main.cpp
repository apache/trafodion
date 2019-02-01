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
 * File:         ex_esp_main.cpp
 * Description:  ESP main program and related methods
 *
 *
 * Created:      1/22/96
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"

#include "ex_stdh.h"
#include "memorymonitor.h"
#include "ex_exe_stmt_globals.h"
#include "ex_esp_frag_dir.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_split_bottom.h"
#include "ex_send_bottom.h"
#include "NAExit.h"
#include "ExSqlComp.h"
#include "Globals.h"
#include "Int64.h"
#include "SqlStats.h"
#include "ComUser.h"
#include "ExpError.h"
#include "ComSqlId.h"
#include "PortProcessCalls.h"
#include "cextdecs/cextdecs.h"
#include "security/dsecure.h"
#define psecure_h_including_section
#define psecure_h_security_psb_get_
#include "security/psecure.h"

#include "seabed/ms.h"
#include "seabed/fs.h"
extern void my_mpi_fclose();
#include "SCMVersHelp.h"
DEFINE_DOVERS(tdm_arkesp)

#include "NAStdlib.h"

#include "rosetta/rosgen.h"

#include "Context.h"
#include "StmtCompilationMode.h"

// -----------------------------------------------------------------------
// ESP control connection, handle system messages
// -----------------------------------------------------------------------

#include "rosetta/rosgen.h"
#include "nsk/nskprocess.h"
#include "zsysc.h"
#include "QRLogger.h"

class EspGuaControlConnection : public GuaReceiveControlConnection
{
public:

   EspGuaControlConnection(
       IpcEnvironment *env,
       ExEspFragInstanceDir *espFragInstanceDir,
       short receiveDepth = 4000,
       GuaReceiveFastStart *guaReceiveFastStart = NULL) :
       GuaReceiveControlConnection(env,
				   receiveDepth,
                                   eye_ESP_GUA_CONTROL_CONNECTION,
				   guaReceiveFastStart)
  { espFragInstanceDir_ = espFragInstanceDir; } 

  virtual void actOnSystemMessage(
       short                  messageNum,
       IpcMessageBufferPtr    sysMsg,
       IpcMessageObjSize      sysMsgLen,
       short                  clientFileNumber,
       const GuaProcessHandle &clientPhandle,
       GuaConnectionToClient  *connection);

private:

  ExEspFragInstanceDir *espFragInstanceDir_;

  virtual NABoolean fakeErrorFromNSK(short errorFromNSK, 
                          GuaProcessHandle *clientPhandle);

  NABoolean getErrorDefine( char * defineName, 
                            ExFragId &targetFragId,
                            short &targetCpu,
                            Lng32 &targetSegment );

  // Cannot do in-place initialization of static const integral 
  // member data in Visual C++.  See MS Knowledge base 
  // article 241569.  No need to workaround this, because 
  // we only need these members in non-WINNT builds.

};

class EspSockControlConnection : public SockControlConnection
{
public:

   EspSockControlConnection(IpcEnvironment *env,
				  ExEspFragInstanceDir *espFragInstanceDir) :
       SockControlConnection(env, eye_ESP_SOCKET_CONTROL_CONNECTION), env_(env),
       espFragInstanceDir_(espFragInstanceDir) {}

  inline EspSockControlConnection(IpcEnvironment *env,
				  ExEspFragInstanceDir *espFragInstanceDir,
				  Int32 socketArg,
				  Int32 portArg):
    SockControlConnection(env), env_(env),
       espFragInstanceDir_(espFragInstanceDir) 
  {}

  virtual void acceptNewConnectionRequest(SockConnection *conn);

private:

  IpcEnvironment *env_;
  ExEspFragInstanceDir *espFragInstanceDir_;
};

// -----------------------------------------------------------------------
// An object that holds a new connection, created by a Guardian open
// system message, until the first application message comes in
// -----------------------------------------------------------------------

class EspNewIncomingConnectionStream : public IpcMessageStream
{
public:

  EspNewIncomingConnectionStream(IpcEnvironment *env,
				 ExEspFragInstanceDir *espFragInstanceDir_);
  virtual ~EspNewIncomingConnectionStream();
  virtual void actOnSend(IpcConnection *connection);
  virtual void actOnReceive(IpcConnection *connection);

private:

  ExEspFragInstanceDir *espFragInstanceDir_;

};

// forward declaration
void DoEspStartup(Int32 argc,
		  char **argv,
		  IpcEnvironment &env,
		  ExEspFragInstanceDir &fragInstanceDir,
		  GuaReceiveFastStart *guaReceiveFastStart);

Int32 runESP(Int32 argc, char** argv, GuaReceiveFastStart *guaReceiveFastStart = NULL);


typedef void* stopCatchHandle;
typedef void* stopCatchContext;
typedef void (*stopCatchFunction) (stopCatchContext);
extern "C" _priv _resident
  stopCatchHandle STOP_CATCH_REGISTER_(stopCatchFunction,
				       stopCatchContext);

_priv _resident void stopCatcher( stopCatchContext scContext);


// -----------------------------------------------------------------------
// -----------  ESP main program for NT or NSK with C runtime ------------
// -----------------------------------------------------------------------

  
Int32 main(Int32 argc, char **argv)
{
  dovers(argc, argv);

  IdentifyMyself::SetMyName(I_AM_ESP);
  msg_debug_hook("arkesp", "esp.hook");

  try {
    file_init(&argc, &argv);
  }
  catch (SB_Fatal_Excep &e) {
    exit(1);
  }
  try {
    file_mon_process_startup(true);

    // Initialize log4cxx 
    QRLogger::initLog4cxx(QRLogger::QRL_ESP);
  }
  catch (SB_Fatal_Excep &e) {
    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, e.what(), 0);
    exit(1);
  }

  atexit(my_mpi_fclose);
  // Leave this commented out unless you need to debug the argument
  // cracking code below and can't rely on the -debug option.  This
  // allows the esp to put up a dialog box and then you can manually
  // force the esp into debug.
  if (getenv("SQL_MSGBOX_PROCESS") != NULL)
	 { MessageBox( NULL, "Server: Process Launched",
		       "tdm_arkesp", MB_OK|MB_ICONINFORMATION );};

  NABoolean fastStart = TRUE;
  Int32 currArg = 1;
  while (currArg < argc && fastStart == TRUE)
  {
    if (strcmp("-noespfaststart", argv[currArg]) == 0)
      fastStart = FALSE;
    currArg++;
  }
  short retCode;
  if (fastStart)
  {
    GuaReceiveFastStart *guaReceiveFastStart = new GuaReceiveFastStart();
    retCode = runESP(argc,argv,guaReceiveFastStart);
  }
  else
    retCode = runESP(argc,argv);
  ENDTRANSACTION();

  return retCode;

}


GuaReceiveFastStart::GuaReceiveFastStart()
{
  _bcc_status status;
  Lng32 bufferAddr;
  readUpdate_ = FALSE;
  awaitiox_ = FALSE;
  replyx_ = FALSE;
  awaitioxError_ = 0;
  fileGetReceiveInfo_ = FALSE;
  zsys_ddl_smsg_open_reply_def openReply;
  //openError_ -- not altered
  openError_ = BFILE_OPEN_((char *)"$RECEIVE", 8, &receiveFile_, 0, 0, 1, 4000, 0);
  //open_ -- not altered
  open_ = TRUE;
  if (openError_ == 0)
  {
    status = BSETMODE(receiveFile_, 74, -1);

    if (_status_ne(status))
      {
	// this is bad
	ABORT("Internal error on setmode($receive)");
      }
    // readUpdateStatus_ -- not altered
    readUpdateStatus_ = BREADUPDATEX(receiveFile_, (char *)&readBuffer_[0], 80);
    // readUpdate_ -- altered
    readUpdate_ = TRUE;
    // bufferData_ -- altered
    bufferData_ = NULL;
    if (_status_eq(readUpdateStatus_)) // Did not get an error on READUPDATEX
    {
      // awaitioxStatus_ -- not altered
      awaitioxStatus_ = BAWAITIOX(&receiveFile_,
					 (void **)&bufferAddr,
					 &awaitioxCountTransferred_,
					 (SB_Tag_Type *)&ioTag_,
					 100 * 60 *10); // 10 minutes
      // fileGetInfoError_ -- not altered
      fileGetInfoError_ = BFILE_GETINFO_(receiveFile_, &awaitioxError_);
      // awaitiox_ -- altered
      awaitiox_ = TRUE;
      if (fileGetInfoError_ == 0 && awaitioxError_ == 6)
      {
	fileGetReceiveInfoError_ = BFILE_GETRECEIVEINFO_((FS_Receiveinfo_Type *)&receiveInfo_);

	// fileGetReceiveInfo_ -- altered
	fileGetReceiveInfo_ = TRUE;
	if (fileGetReceiveInfoError_ == 0)
	{
	  openReply.z_msgnumber = ZSYS_VAL_SMSG_OPEN;
	  openReply.z_openid = 0; // GuaReceiveControlConnection id_ must be zero
	  // replyxstatus_ -- not altered
	  replyxstatus_ = BREPLYX((IpcMessageBufferPtr)&openReply,
	                                4,
					&replyxCountWritten_,
					receiveInfo_.replyTag_,
					GuaOK);
	  // replyx_ == altered
	  replyx_ = TRUE;
	}
      }
    }
  }
}

// -----------------------------------------------------------------------
// Startup handling of ESP
// -----------------------------------------------------------------------

Int32 runESP(Int32 argc, char** argv, GuaReceiveFastStart *guaReceiveFastStart)
{
  // initialize ESP global data
  StatsGlobals * statsGlobals;

  XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETRECVLIMIT, XMAX_SETTABLE_RECVLIMIT_H);
  CliGlobals *cliGlobals = NULL;
  cliGlobals = CliGlobals::createCliGlobals(TRUE); // TRUE indicates a non-master process (WAIT on LREC)
  if (cliGlobals == NULL) // Sanity check
    NAExit(1); // Abend
  Int32 shmid;
  statsGlobals = shareStatsSegment(shmid);
  cliGlobals->setSharedMemId(shmid);
  //Lng32 numCliCalls = cliGlobals->incrNumOfCliCalls();
  cliGlobals->setIsESPProcess(TRUE);
  NAHeap *espExecutorHeap = cliGlobals->getExecutorMemory();
  // must create default context after set IpcEnvironment in CliGlobals first
  // because context's ExSqlComp object needs IpcEnvironment
  cliGlobals->initiateDefaultContext();
  NAHeap *espIpcHeap = cliGlobals->getIpcHeap();
  IpcEnvironment *ipcEnvPtr = cliGlobals->getEnvironment();
  if (statsGlobals != NULL)
     cliGlobals->setMemoryMonitor(statsGlobals->getMemoryMonitor());
  else 
  {
     // Start the  memory monitor for dynamic memory management
     Lng32 memMonitorWindowSize = 10;
     Lng32 memMonitorSampleInterval = 10;
     MemoryMonitor *memMonitor = new (espExecutorHeap) 
                           MemoryMonitor(memMonitorWindowSize,
                           memMonitorSampleInterval,
                           espExecutorHeap);
     cliGlobals->setMemoryMonitor(memMonitor);
  }
  // After CLI globals are initialized but before we begin ESP message
  // processing, have the CLI context set its user identity based on
  // the OS user identity.
  ContextCli *context = cliGlobals->currContext();
  ex_assert(context, "Invalid context pointer");
  context->initializeUserInfoFromOS();

  ExEspFragInstanceDir espFragInstanceDir(cliGlobals,
                                          espExecutorHeap,
                                          (StatsGlobals *)statsGlobals);

  ExEspControlMessage espIpcControlMessage(&espFragInstanceDir,
                                           ipcEnvPtr,
                                           espIpcHeap);

  // handle startup (command line args, control connection)
  DoEspStartup(argc,argv,*ipcEnvPtr,espFragInstanceDir,guaReceiveFastStart);
  // the control message stream talks through the control connection
  espIpcControlMessage.addRecipient(
       ipcEnvPtr->getControlConnection()->getConnection());

  // start the first receive operation
  espIpcControlMessage.receive(FALSE);
 
  NABoolean timeout;
  Int64 prevWaitTime = 0;

  // while there are requesters
  while (espFragInstanceDir.getNumMasters() > 0)
    {
      // -----------------------------------------------------------------
      // The ESPs most important line of code: DO THE WORK
      // -----------------------------------------------------------------

      espFragInstanceDir.work(prevWaitTime);

      // -----------------------------------------------------------------
      // After we have done work, it's necessary to wait for some I/O
      // (the frag instance dir work procedure works until it is blocked).
      // -----------------------------------------------------------------

      ipcEnvPtr->getAllConnections()->
	waitOnAll(IpcInfiniteTimeout, TRUE, &timeout, &prevWaitTime); // TRUE means: Called by ESP main
    }

  // nobody wants us anymore, right now that means that we stop
  return 0;
}

void DoEspStartup(Int32 argc,
		  char **argv,
		  IpcEnvironment &env,
		  ExEspFragInstanceDir &fragInstanceDir,
		  GuaReceiveFastStart *guaReceiveFastStart)
{
  // make the compiler happy by using fragInstanceDir for something
  if (fragInstanceDir.getNumEntries() < 0)
    {}

  // interpret command line arguments
  IpcServerAllocationMethod allocMethod = IPC_ALLOC_DONT_CARE;

  Int32 currArg = 1;

  Int32 socketArg = 0;
  Int32 portArg = 0;

  while (currArg < argc)
    {
      if (strcmp("-fork", argv[currArg]) == 0)
        {
	  allocMethod = IPC_POSIX_FORK_EXEC;
        }
      else if (strcmp("-service", argv[currArg]) == 0)
	{
	  // /etc/inetd.conf should be configured such that the "-service"
	  // command line option is given
	  allocMethod = IPC_INETD;
	}
      else if (strcmp("-guardian", argv[currArg]) == 0)
	{
	  allocMethod = IPC_LAUNCH_GUARDIAN_PROCESS;
	}
      else if (strcmp("-noespfaststart", argv[currArg]) == 0)
	;
      else if (strcmp("-debug", argv[currArg]) == 0)
	{
	  NADebug();
	}
      else
	{
	  // bad arguments, die
	  NAExit(-1);
	}

      currArg++;
    }


  // create control connection (open $RECEIVE in Tandemese)
  switch (allocMethod)
    {
    case IPC_LAUNCH_GUARDIAN_PROCESS:
    case IPC_SPAWN_OSS_PROCESS:
      {
      // open $RECEIVE with a receive depth of 4000

	 GuaReceiveControlConnection *cc =
				  
	 new(&env) EspGuaControlConnection(&env,
					  &fragInstanceDir,
					  4000,
					  guaReceiveFastStart);
      env.setControlConnection(cc);

      // wait for the first open message to come in
      cc->waitForMaster();
      // set initial timeout in case the master never send first plan message
      env.setIdleTimestamp();
      }
     break;
      
    case IPC_INETD:
    case IPC_POSIX_FORK_EXEC:
      env.setControlConnection(
	   new(&env) EspSockControlConnection(&env,&fragInstanceDir));

      break;
      // NEEDS PORT (12/16/96) 
      //	handle the local NT process case without NSK-like

    case IPC_LAUNCH_NT_PROCESS:

      //debugging code - figure out later
      // the name of this machine on which this process is executing
      char myMachine[IpcNodeNameMaxLength];
      char errorLine[64];
      Int32  result;

      // who am I?
      result = gethostname(myMachine,IpcNodeNameMaxLength);
      if (!result)
	{
	  sprintf(errorLine," DoEspStartup : gethostname : error %d",result);
	  ABORT(errorLine);
	};
      // end debugging code
      env.setControlConnection(
	   new(&env) EspSockControlConnection(
		&env,&fragInstanceDir,socketArg, portArg));
      break;
      
    default:
      // bad command line arguments again
      NAExit(-1);
    }
}

void EspGuaControlConnection::actOnSystemMessage(
       short                  messageNum,
       IpcMessageBufferPtr    sysMsg,
       IpcMessageObjSize      sysMsgLen,
       short                  clientFileNumber,
       const GuaProcessHandle &clientPhandle,
       GuaConnectionToClient  *connection)
{
  switch (messageNum)
    {
    case ZSYS_VAL_SMSG_OPEN:
      if (initialized_)
	{
	  // This an OPEN message for a connection that isn't the
	  // initial control connection. Create a new message stream and
	  // attach it to the newly created connection.
	  EspNewIncomingConnectionStream *newStream = new(getEnv()->getHeap())
	    EspNewIncomingConnectionStream(getEnv(),espFragInstanceDir_);

	  ex_assert(connection != NULL,
                    "Must create connection for open sys msg");
	  newStream->addRecipient(connection);
          newStream->receive(FALSE);

	  // now abandon the new object, it will find its way to the right
	  // send bottom TCB on its own
	  // (a memory leak would result if the client would open our $RECEIVE
	  // w/o sending corresponding ESP level open requests)
	}
      break;

    case ZSYS_VAL_SMSG_CPUDOWN:
    case ZSYS_VAL_SMSG_REMOTECPUDOWN:
    case ZSYS_VAL_SMSG_CLOSE:
    case ZSYS_VAL_SMSG_NODEDOWN:
      // Somebody closed us or went down. Was it master executor?
      // Note that GuaReceiveControlConnection::getConnection returns
      // the master executor connection.
      if (getConnection() == connection)
      {
        // Master is gone, stop this process and let the OS cleanup.
        if (getEnv()->getLogEspGotCloseMsg())
        {
          /*
          Coverage notes: to test this code in a dev regression requires
          changing $TRAF_VAR/ms.env, so I made a manual test on
          May 11, 2012 to verify this code.
          */
          char myName[20];
          memset(myName, '\0', sizeof(myName));
          getEnv()->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).toAscii(
                                                    myName, sizeof(myName));
          char buf[500];
          char *sysMsgName = NULL;
          switch (messageNum)
          {
          case ZSYS_VAL_SMSG_CPUDOWN:
            sysMsgName = (char *) "CPUDOWN";
            break;
          case ZSYS_VAL_SMSG_REMOTECPUDOWN:
            sysMsgName = (char *) "REMOTECPUDOWN";
            break;
          case ZSYS_VAL_SMSG_CLOSE:
            sysMsgName = (char *) "CLOSE";
            break;
          case ZSYS_VAL_SMSG_NODEDOWN:
            sysMsgName = (char *) "NODEDOWN";
            break;
          }
          str_sprintf(buf, 
                      "System %s message causes %s to exit.",
                            sysMsgName, myName);
          SQLMXLoggingArea::logExecRtInfo(__FILE__, 
                                          __LINE__, buf, 0);
        }
        getEnv()->stopIpcEnvironment();
      }
      // Otherwise, do a search thru all
      // downloaded fragment entries and check whether their
      // client is still using them. The IPC layer will wake
      // up the scheduler so the actual release can take place.
      espFragInstanceDir_->releaseOrphanEntries();
      break;

    default:
      // do nothing for all other kinds of system messages
      break;
    } // switch


  // The parent class already handles the job of closing all connections
  // who lost their client process by failed processes, failed CPUs and
  // failed systems or networks. Check here that we die if all our
  // requestors go away, but don't die if the first system message is
  // something other than an OPEN message.
  if (getNumRequestors() == 0 AND initialized_)
    {
      // ABORT("Lost connection to client");
      // losing the client is not a reason to panic, the client may
      // have voluntarily decided to exit without freeing its resources
      NAExit(0);
    }
  else if (NOT initialized_ AND getNumRequestors() > 0)
    {
      // the first requestor came in
      initialized_ = TRUE;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// The next two methods support error injection.  This is controlled by 
// setting either one or two defines.  
// 
// Any character on "fragment" part of the client side is ignored.  
// 
// Important limitation: this feature will not be able to selectively support 
// more than one statement per ESP.

NABoolean  EspGuaControlConnection::getErrorDefine( char * defineName, 
                                 ExFragId &targetFragId,
                                 short &targetCpu,
                                 Lng32 &targetSegment )
{
  NABoolean fakeOpenDefineIsSet = FALSE;
  return fakeOpenDefineIsSet;
}


NABoolean  EspGuaControlConnection::fakeErrorFromNSK(short errorFromNSK,
                                         GuaProcessHandle *clientPhandle)
{
  NABoolean retcode = FALSE;
  // tbd - could we use getEnv here on Windows?
  return retcode;
}

// -----------------------------------------------------------------------
// methods for class EspNewIncomingConnectionStream
// -----------------------------------------------------------------------

EspNewIncomingConnectionStream::EspNewIncomingConnectionStream(
     IpcEnvironment       *ipcEnvironment,
     ExEspFragInstanceDir *espFragInstanceDir) :
     IpcMessageStream(ipcEnvironment,
		      IPC_MSG_SQLESP_SERVER_INCOMING,
		      CurrEspReplyMessageVersion,
		      0,
		      TRUE)
{
  espFragInstanceDir_ = espFragInstanceDir;
}

EspNewIncomingConnectionStream::~EspNewIncomingConnectionStream()
{
  // nothing to do
}

void EspNewIncomingConnectionStream::actOnSend(IpcConnection *)
{
  // typically the stream will never send but we use it for a send 
  // when we reject extract consumer opens.
}

void EspNewIncomingConnectionStream::actOnReceive(IpcConnection *connection)
{
  // check for OS errors
  if (getState() == ERROR_STATE)
  {
    ex_assert(FALSE,"Error while receiving first message from client");
  }

  // check for protocol errors
  bool willPassTheAssertion = 
             (getType() == IPC_MSG_SQLESP_DATA_REQUEST OR
              getType() == IPC_MSG_SQLESP_CANCEL_REQUEST) AND
              getVersion() == CurrEspRequestMessageVersion AND
              moreObjects();
  if (!willPassTheAssertion)
  {
    char *doCatchBugCRx = getenv("ESP_BUGCATCHER_CR_NONUMBER");
    if (!doCatchBugCRx ||
        *doCatchBugCRx != '0')
    {
      connection->dumpAndStopOtherEnd(true, false);
      environment_->getControlConnection()->
        castToGuaReceiveControlConnection()->
        getConnection()->dumpAndStopOtherEnd(true, false);
    }
  }

  ex_assert((getType() == IPC_MSG_SQLESP_DATA_REQUEST OR
             getType() == IPC_MSG_SQLESP_CANCEL_REQUEST) AND
	    getVersion() == CurrEspRequestMessageVersion AND
	    moreObjects(),
	    "Invalid first message from client");

  // take a look at the type of the first object in the message
  IpcMessageObjType nextObjType = getNextObjType();
  switch (nextObjType)
  {
    case ESP_OPEN_HDR:
    case ESP_LATE_CANCEL_HDR:
      {
        ExFragKey key;
        Lng32 remoteInstNum;
        NABoolean isParallelExtract = false; 

        // peek at the message header to see for whom it is
        if (nextObjType == ESP_OPEN_HDR)
        {
          ExEspOpenReqHeader reqHdr((NAMemory *) NULL);
	   
	  *this >> reqHdr;
	  key = reqHdr.key_;
	  remoteInstNum = reqHdr.myInstanceNum_;
	  if (reqHdr.getOpenType() == ExEspOpenReqHeader::PARALLEL_EXTRACT) 
          {
            isParallelExtract = true;
	  }
	}
        else
	{
          // note that the late cancel request may or may not
	  // arrive as the first request (only in the former case
	  // will we reach here)
	  ExEspLateCancelReqHeader reqHdr((NAMemory *) NULL);
	   
	  *this >> reqHdr;
	  key = reqHdr.key_;
	  remoteInstNum = reqHdr.myInstanceNum_;
	}

        if (!isParallelExtract) 
        {
          ExFragInstanceHandle handle =
	    espFragInstanceDir_->findHandle(key);

	  if (handle != NullFragInstanceHandle)
	  {
            // the send bottom node # myInstanceNum of this downloaded fragment
            // is the true recipient of this open request
            ex_split_bottom_tcb * receivingTcb =
              espFragInstanceDir_->getTopTcb(handle);
            ex_send_bottom_tcb *receivingSendTcb =
              receivingTcb->getSendNode(remoteInstNum);

            // Check the connection for a co-located client, and if so,
            // tell the split bottom, because it may prefer this send 
            // bottom when using skew buster uniform distribution.
            if (espFragInstanceDir_->
                  getEnvironment()->
                  getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).match(
                    connection->getOtherEnd().getNodeName(),
                    connection->getOtherEnd().getCpuNum()))
              receivingTcb->setLocalSendBottom(remoteInstNum);

            // Portability note for the code above: we pass IPC_DOM_GUA_PHANDLE
            // for IpcEnvironment::getMyOwnProcessId, even though that method
            // can be called with the default param (IpcNetworkDomain 
            // IPC_DOM_INVALID).  In fact it would  probably be better 
            // to call the object without specifying the IpcNetworkDomain so
            // that it can decide for itself what domain it is using.
            // But there is a problem with the Windows implementation
            // of IpcEnvironment::getMyOwnProcessId, it seems to assume
            // that its domain is IPC_DOM_INTERNET and so this will 
            // cause the botch of an assertion that its control connection 
            // (which is type EspGuaControlConnection) can be cast to a 
            // SockControlConnection.  When this problem is fixed, the 
            // IPC_DOM_GUA_PHANDLE param above can be removed.  Also,
            // when this code is ported to run it a domain other than
            // "guardian", it will be necessary to fix this and to
            // fix IpcEnvironment::getMyOwnProcessId to work properly on
            // windows.

            receivingSendTcb->setClient(connection);
            receivingSendTcb->routeMsg(*this);
          }
	  else
	  {
            connection->dumpAndStopOtherEnd(true, false);
	    ex_assert(FALSE,"entry not found, set diagnostics area and reply");
	  }

        } // normal case, not parallel extract

        else 
        {
          // The OPEN request is from a parallel extract consumer. The
          // incoming request contains a user ID which we will compare
          // against the current user ID for this ESP.

          // NOTE: The user ID for the extract security check is
          // currently sent and compared as a C string. On Linux it is
          // possible to send and compare integers which would lead to
          // simpler code. The code to send/compare strings is still
          // used because it works on all platforms.

          char errorStr[150];

	  // check if next msg is of securityInfo type. 
          ex_assert(moreObjects(), "expected object not received");
          ex_assert(getNextObjType() == ESP_SECURITY_INFO,
	            "received message for unknown message type");
          
	  // unpack security info
	  ExMsgSecurityInfo secInfo(environment_->getHeap());
	  *this >> secInfo;

          // Get the auth ID of this ESP in text form and compare it
          // to the auth ID that arrived in the message. Skip this
          // step in the debug build if an environment variable is
          // set.
          NABoolean doAuthIdCheck = TRUE;
          Int32 status = 0;
#ifdef _DEBUG
          const char *envvar = getenv("NO_EXTRACT_AUTHID_CHECK");
          if (envvar && envvar[0])
            doAuthIdCheck = FALSE;
#endif
          if (doAuthIdCheck)
          {
            // Get user ID from ExMsgSecurityInfo -> (secUserID)
            // the user ID is the integer value made into a string
            // Convert it back into its integer value
            short userIDLen = (short) str_len(secInfo.getAuthID());
            Int32 secUserID = str_atoi(secInfo.getAuthID(), userIDLen);

            // Get the current user ID
            Int32 curUserID = ComUser::getSessionUser(); 

            // Report an error if the user ID is not valid
            if (curUserID == NA_UserIdDefault || secUserID == NA_UserIdDefault)
            {
              str_cpy_c(errorStr,
                        "Producer ESP could not authenticate the consumer, "
                        "no valid current user.");
              status = -1;
            }

            // Make sure user id passed in ExMsgSecurityInfo matches
            // the user id associated with the current session

#if defined(_DEBUG)
            NABoolean doDebug = (getenv("DBUSER_DEBUG") ? TRUE : FALSE);
            if (doDebug)
              printf("[DBUSER:%d] ESP extract user ID: "
                     "local [%d], msg [%d]\n",
                     (int) getpid(), curUserID, secUserID);
#endif

              // Compare user ID, Report an error, if comparison fails
              if (curUserID != secUserID)
              {
                str_cpy_c(errorStr,
                          "Producer ESP could not authenticate the consumer, "
                          "user named passed in ExMsgSecurityInfo is not the "
                          "current user");
                status = -1;
              }

          } // if (doAuthIdCheck)
		   
          // get the split bottom TCB that matches the securityKey
          ex_split_bottom_tcb *receivingTcb = NULL;
          if (status == 0) 
          {
            receivingTcb = espFragInstanceDir_->getExtractTop(secInfo.getSecurityKey());
            if (receivingTcb == NULL) 
            {
              str_cpy_c(errorStr, "Producer ESP could not locate extract node");
              status = -1;
            }
          }

	  // get the sendBottom TCB if not already connected to a client
	  ex_send_bottom_tcb *receivingSendTcb = NULL;
	  if (status == 0)
	  {
	    receivingSendTcb = receivingTcb->getConsumerSendBottom();
	    if (receivingSendTcb == NULL) 
	    {
	      str_cpy_c(errorStr, "Producer ESP already connected to a client");
	      status = -1;
	    }
	  }

          // send the error message to the consumer 
	  if (status != 0) 
	  {
            clearAllObjects();
            setType(IPC_MSG_SQLESP_DATA_REPLY);

            NAMemory *heap = environment_->getHeap();

	    IpcMessageObj* baseObj =
	      new(heap)IpcMessageObj(IPC_SQL_DIAG_AREA, CurrEspReplyMessageVersion);
	    *this << *baseObj;

	    // prepare proper error message
	    char phandle[100];
	    MyGuaProcessHandle myHandle;
            myHandle.toAscii(phandle, 100);

	    ComDiagsArea *diags = ComDiagsArea::allocate(heap);
            *diags << DgSqlCode(-EXE_PARALLEL_EXTRACT_OPEN_ERROR)
                   << DgString0(phandle)
                   << DgString1(errorStr);
	    *this  << *diags;

	    diags->decrRefCount();

            send(TRUE /* TRUE indicates waited */);
          }

          // if everything okay, then make the connection
	  if (status == 0) 
          {
            receivingSendTcb->setClient(connection);
            receivingSendTcb->routeMsg(*this);
          }
        } // parallel extract case
      } // open or cancel header
      break;

      default:
        ex_assert(FALSE,"Invalid request for first client message");
  } // end switch

  // self-destruct, the new connection is now handled by someone else
  addToCompletedList();
}


// -----------------------------------------------------------------------
// Methods for class EspSockControlConnection
// -----------------------------------------------------------------------
void EspSockControlConnection::acceptNewConnectionRequest(SockConnection *conn)
{
  // create a send bottom message stream without attaching it
  // yet to a TCB, since we don't know yet to which TCB it belongs
  EspNewIncomingConnectionStream *newStream = new(env_->getHeap())
    EspNewIncomingConnectionStream(env_,espFragInstanceDir_);

  ex_assert(conn != NULL,"Must create connection for open sys msg");
  newStream->addRecipient(conn);
  newStream->receive(FALSE);

  // now abandon the new object, it will find its way to the right
  // send bottom TCB on its own
  // (a memory leak would result if the client would open us
  // w/o sending corresponding ESP level open requests)
}

// -----------------------------------------------------------------------
// KSKSKS Kludge for resolving reference to unused function
// -----------------------------------------------------------------------

