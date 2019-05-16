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
 * File:         Ipc.C
 * Description:  Implementation for process communication services on the
 *               low (executor) level without using C++ run time library
 *
 * Created:      10/6/95
 * Language:     C++
 *
 *****************************************************************************
 */


#define AEVENT 1
#define  CLI_DLL

//#define IPC_INTEGRITY_CHECKING 1  // for debugging purposes

#include "Platform.h"
#include "ComASSERT.h"
#include "ComDiags.h"
#include "logmxevent.h"
#include "ExCollections.h"
#include "Ipc.h"
#include "ipcmsg.h"
#include "str.h"
#include "HeapLog.h"
#include "ComRtUtils.h"
#include "PortProcessCalls.h"

#include <time.h>
#include <sys/time.h>
#include "seabed/pctl.h"
#include "seabed/ms.h"
#include "seabed/int/opts.h"

#include <unistd.h>		// for getpid()

#include "Globals.h"
#include "Context.h"
#include "MXTraceDef.h"
#include "ExSMTrace.h"

#include "SMConnection.h"
#include "ExSMCommon.h"
#include "ExSMGlobals.h"
#include "ExSMReadyList.h"
#include "ExSMTask.h"

NABoolean XAWAITIOX_MINUS_ONE = TRUE;

#include "ComCextdecs.h"
#include "Ex_esp_msg.h"

#ifndef FS_MAX_NOWAIT_DEPTH
#define FS_MAX_NOWAIT_DEPTH 16
#endif

// -----------------------------------------------------------------------
// Methods for class IpcNodeName
// -----------------------------------------------------------------------

IpcNodeName::IpcNodeName(IpcNetworkDomain dom,
			 const char *name)
{
  domain_ = dom;

  if (domain_ == IPC_DOM_INTERNET)
    {
      SockIPAddress extIPAddr;
      SockErrNo lookupResult = extIPAddr.set(name);

      if (lookupResult.hasError())
	ABORT("Node name not found");
      ipAddr_ = extIPAddr.getRawAddress();
    }
  else
    {
      assert(domain_ == IPC_DOM_GUA_PHANDLE);
      assert(str_len(name) <= GuaNodeNameMaxLen);
      str_pad(guardianNode_.nodeName_,GuaNodeNameMaxLen,' ');
      str_cpy_all(guardianNode_.nodeName_,
		  name,
		  str_len(name));
    }
}

IpcNodeName::IpcNodeName(const SockIPAddress &iPNode)
{
  domain_ = IPC_DOM_INTERNET;
  ipAddr_ = iPNode.getRawAddress();
}

IpcNodeName & IpcNodeName::operator = (const IpcNodeName &other)
{
  domain_ = other.domain_;
  if (domain_ == IPC_DOM_INTERNET)
    ipAddr_ = other.ipAddr_;
  else
    guardianNode_ = other.guardianNode_;

  return *this;
}

NABoolean IpcNodeName::operator == (const IpcNodeName &other)
{
  if (domain_ != other.domain_)
    return FALSE;

  if (domain_ == IPC_DOM_INTERNET)
    {
      return (str_cmp((char *) &ipAddr_,
		      (char *) &other.ipAddr_,
		      sizeof(ipAddr_)) == 0);
    }
  else if (domain_ == IPC_DOM_GUA_PHANDLE)
    {
      return (str_cmp(guardianNode_.nodeName_,
		      other.guardianNode_.nodeName_,
		      GuaNodeNameMaxLen) == 0);
    }
  else
    return FALSE;
}

SockIPAddress IpcNodeName::getIPAddress() const
{
  assert(domain_ == IPC_DOM_INTERNET);
  return SockIPAddress(ipAddr_);
}

// -----------------------------------------------------------------------
// Methods for class GuaProcessHandle
// -----------------------------------------------------------------------

NABoolean GuaProcessHandle::operator == (const GuaProcessHandle &other) const
{
  // call a system procedure to compare
  return compare(other);
}

void GuaProcessHandle::dumpAndStop(bool doDump, bool doStop) const
{
  char coreFile[1024];
  NAProcessHandle phandle((SB_Phandle_Type *)&phandle_);
  phandle.decompose();
  if (doDump)
    msg_mon_dump_process_name(NULL, phandle.getPhandleString(), coreFile);
  if (doStop)
    msg_mon_stop_process_name(phandle.getPhandleString()); 
}

// -----------------------------------------------------------------------
// Methods for class IpcProcessId
// -----------------------------------------------------------------------

IpcProcessId::IpcProcessId() : IpcMessageObj(IPC_PROCESS_ID,
					     IpcCurrProcessIdVersion)
{
  domain_ = IPC_DOM_INVALID;
}

IpcProcessId::IpcProcessId(
     const GuaProcessHandle &phandle) : IpcMessageObj(IPC_PROCESS_ID,
						      IpcCurrProcessIdVersion)
{
  domain_ = IPC_DOM_GUA_PHANDLE;
  phandle_ = phandle;
}

IpcProcessId::IpcProcessId(
     const SockIPAddress &ipAddr,
     SockPortNumber port) : IpcMessageObj(IPC_PROCESS_ID,
					  IpcCurrProcessIdVersion)
{
  // only an internet node name goes together with an IP address and a port
  domain_ = IPC_DOM_INTERNET;
  pid_.ipAddress_ = ipAddr.getRawAddress();
  pid_.listnerPort_ = port;
}

IpcProcessId::IpcProcessId(const char *asciiRepresentation) :
     IpcMessageObj(IPC_PROCESS_ID,
		   IpcCurrProcessIdVersion)
{
  domain_ = IPC_DOM_INVALID;

  // On NSK, try to interpret the string as a PHANDLE first
  if (phandle_.fromAscii(asciiRepresentation))
    domain_ = IPC_DOM_GUA_PHANDLE;

  if (domain_ == IPC_DOM_INVALID)
    {
      // try to decode an internet address, followed by a port number,
      Int32 colonPos = 0;
      while (asciiRepresentation[colonPos] != 0 AND
	     asciiRepresentation[colonPos] != ':')
	colonPos++;

      if (asciiRepresentation[colonPos] == ':')
	{
	  char asciiIpAddr[300];
	  SockIPAddress ipAddr;

	  assert(colonPos < 300);
	  str_cpy_all(asciiIpAddr,asciiRepresentation,colonPos);
	  asciiIpAddr[colonPos] = 0;
	  SockErrNo sen = ipAddr.set(asciiIpAddr);

	  if (NOT sen.hasError())
	    {
	      pid_.ipAddress_ = ipAddr.getRawAddress();
	      // now parse the port number
	      ULng32 portNo = 0;
	      colonPos++;
	      while (asciiRepresentation[colonPos] >= '0' AND
		     asciiRepresentation[colonPos] <= '9')
		{
		  portNo = portNo * 10 + (asciiRepresentation[colonPos] - '0');
		}
	      pid_.listnerPort_ = portNo;
	      domain_ = IPC_DOM_INTERNET;
	    }
	}
    }
}

IpcProcessId::IpcProcessId(
     const IpcProcessId &other) : IpcMessageObj(other.getType(),
						other.getVersion())
{
  domain_ = other.domain_;

  if (domain_ == IPC_DOM_GUA_PHANDLE)
    phandle_ = other.phandle_;
  else if (domain_ == IPC_DOM_INTERNET)
    pid_ = other.pid_;
}

IpcProcessId & IpcProcessId::operator = (const IpcProcessId &other)
{
  domain_ = other.domain_;

  if (domain_ == IPC_DOM_GUA_PHANDLE)
    phandle_ = other.phandle_;
  else if (domain_ == IPC_DOM_INTERNET)
    pid_ = other.pid_;

  return *this;
}

NABoolean IpcProcessId::operator == (const IpcProcessId &other) const
{
  if (domain_ != other.domain_)
    return FALSE;

  return ((domain_ == IPC_DOM_GUA_PHANDLE AND
	   phandle_ == other.phandle_)
	  OR
	  (domain_ == IPC_DOM_INTERNET AND
	   str_cmp((char *) pid_.ipAddress_.ipAddress_,
		   (char *) other.pid_.ipAddress_.ipAddress_,
		   4) == 0 AND
	   pid_.listnerPort_ == other.pid_.listnerPort_));
}

NABoolean IpcProcessId::match(const IpcNodeName &name,
			      IpcCpuNum cpuNum) const
{
  if (domain_ == IPC_DOM_INTERNET)
    {
      // IP addresses don't tell the CPU, just compare a normalized
      // form of the IP addresses
      return (getNodeName() == name);
    }
  else if (domain_ == IPC_DOM_GUA_PHANDLE)
    {
      // if the caller cares about CPU number then compare this
      // first
      if (cpuNum != IPC_CPU_DONT_CARE AND cpuNum != getCpuNum())
	return FALSE;

      // compare the node names
      return (getNodeName() == name);
    }
  else
    return FALSE;
}

SockIPAddress IpcProcessId::getIPAddress() const
{
  assert(domain_ == IPC_DOM_INTERNET);

  return SockIPAddress(pid_.ipAddress_);
}

SockPortNumber IpcProcessId::getPortNumber() const
{
  assert(domain_ == IPC_DOM_INTERNET);

  return pid_.listnerPort_;
}

const GuaProcessHandle & IpcProcessId::getPhandle() const
{
  assert(domain_ == IPC_DOM_GUA_PHANDLE);

  return phandle_;
}

IpcNodeName IpcProcessId::getNodeName() const
{
  // getting to the node name is somewhat convoluted, sorry about that
  if (domain_ == IPC_DOM_INTERNET)
    {
      return IpcNodeName(SockIPAddress(pid_.ipAddress_));
    }
  else if (domain_ == IPC_DOM_GUA_PHANDLE)
    {
      return IpcNodeName(phandle_);
    }
  else 
    ABORT("Can't get node name of an invalid process id");

//    the return statement is here so this file will compile under VC++4.1
//    what is actually returned is of little consequence since the ABORT makes
//	  sure we never get to return.
//    Perhaps we should set pid_.ipAddress_ =  some meaningless ip address?;
	  
      return IpcNodeName(SockIPAddress(pid_.ipAddress_));
}

IpcCpuNum IpcProcessId::getCpuNum() const
{
  if (domain_ == IPC_DOM_GUA_PHANDLE)
    {
      // ask Guardian to get the CPU number out of the phandle
      return getCpuNumFromPhandle();
    }
  else
    {
      // for the internet we don't have control over the assignment of CPU
      // numbers, return a don't care value
      return IPC_CPU_DONT_CARE;
    }
}

std::string IpcProcessId::toString() const
{
  char outb[100];

  toAscii(outb, sizeof(outb));
  return outb;
}

Int32 IpcProcessId::toAscii(char *outBuf, Int32 outBufLen) const
{
  // process names shouldn't be longer than 300 bytes
  char outb[300] = "";	  // Initialize in case this is called
  Int32 outLen = 0;

  if (domain_ == IPC_DOM_GUA_PHANDLE)
    {
      outLen = phandle_.toAscii(outb,300);
    }

  if (domain_ == IPC_DOM_INTERNET)
    {
      sprintf(outb,"%d.%d.%d.%d:%d",
	      pid_.ipAddress_.ipAddress_[0],
	      pid_.ipAddress_.ipAddress_[1],
	      pid_.ipAddress_.ipAddress_[2],
	      pid_.ipAddress_.ipAddress_[3],
	      pid_.listnerPort_);
      outLen = str_len(outb);
    }

  // copy the result and terminate it with a NUL character
  str_cpy_all(outBuf,outb,MINOF(outLen,outBufLen-1));
  outBuf[MINOF(outLen,outBufLen-1)] = 0;

  // return the actual length or the length we would need
  return outLen;
}

void IpcProcessId::addProcIdToDiagsArea(ComDiagsArea &diags,
					Int32 stringno) const
{
  char asciiProcId[300];

  toAscii(asciiProcId,300);
  switch (stringno)
    {
    case 0:
      diags << DgString0(asciiProcId);
      break;
    case 1:
      diags << DgString1(asciiProcId);
      break;
    case 2:
      diags << DgString2(asciiProcId);
      break;
    case 3:
      diags << DgString3(asciiProcId);
      break;
    case 4:
      diags << DgString4(asciiProcId);
      break;
    default:
      ABORT("Invalid string no in IpcProcessId::addProcIdToDiagsArea");
    }
}

IpcConnection * IpcProcessId::createConnectionToServer(
     IpcEnvironment *env,
     NABoolean usesTransactions,
     Lng32 maxNowaitRequests,
     NABoolean parallelOpen,
     Int32 *openCompletionScheduled
     ,
     NABoolean dataConnectionToEsp
     ) const
{
  NABoolean useGuaIpc = TRUE;

  if (domain_ == IPC_DOM_INTERNET)
    {
      usesTransactions = usesTransactions; // make compiler happy
      return new(env->getHeap()) SockConnection(env,*this,FALSE);
    }
  else if (domain_ == IPC_DOM_GUA_PHANDLE)
    {
	return new(env->getHeap()) GuaConnectionToServer(env,
					      *this,
					      usesTransactions,
					      (unsigned short) maxNowaitRequests,
					      eye_GUA_CONNECTION_TO_SERVER,
					      parallelOpen,
					      openCompletionScheduled
                                              ,
                                              dataConnectionToEsp
                                              );
    }
  else
    {
      return NULL;
    }
}

IpcMessageObjSize IpcProcessId::packedLength()
{
  // we pack the domain type and then the phandle or socket process id
  IpcMessageObjSize result = baseClassPackedLength() + sizeof(domain_);
  result += sizeof(spare_);

  if (domain_ == IPC_DOM_GUA_PHANDLE)
    {
      result += sizeof(phandle_);
    }
  else if (domain_ == IPC_DOM_INTERNET)
    {
      result += sizeof(pid_);
    }

  return result;
}

IpcMessageObjSize IpcProcessId::packObjIntoMessage(IpcMessageBufferPtr buffer)
{
  // pack base class and domain info
  IpcMessageObjSize result = packBaseClassIntoMessage(buffer);
  str_cpy_all(buffer,(const char *) &domain_, sizeof(domain_));
  result += sizeof(domain_);
  buffer += sizeof(domain_);
  result += sizeof(spare_);
  buffer += sizeof(spare_);

  // ---------------------------------------------------------------------
  // NOTE: this code assumes that the OS dependent information (phandle
  // and socket pid) can be sent to another process as a byte string!!!!
  // ---------------------------------------------------------------------

  // pack the object of the right domain
  if (domain_ == IPC_DOM_GUA_PHANDLE)
    {
      str_cpy_all(buffer,(const char *) &phandle_,sizeof(phandle_));
      result += sizeof(phandle_);
    }
  else if (domain_ == IPC_DOM_INTERNET)
    {
      str_cpy_all(buffer,(const char *) &pid_,sizeof(pid_));
      result += sizeof(pid_);
    }
  
  return result;
}

void IpcProcessId::unpackObj(IpcMessageObjType objType,
			     IpcMessageObjVersion objVersion,
			     NABoolean sameEndianness,
			     IpcMessageObjSize objSize,
			     IpcConstMessageBufferPtr buffer)
{
  assert(objType == IPC_PROCESS_ID AND
	 objVersion == IpcCurrProcessIdVersion AND
	 sameEndianness);

  unpackBaseClass(buffer);

  str_cpy_all((char *) &domain_, buffer, sizeof(domain_));
  buffer += sizeof(domain_);
  buffer += sizeof(spare_);

  // check the supplied length
  assert(objSize == packedLength());

  if (domain_ == IPC_DOM_GUA_PHANDLE)
    {
      str_cpy_all((char *) &phandle_, buffer, sizeof(phandle_));
    }
  else if (domain_ == IPC_DOM_INTERNET)
    {
      str_cpy_all((char *) &pid_, buffer, sizeof(pid_));
    }
}

// -----------------------------------------------------------------------
// Methods for class IpcServer
// -----------------------------------------------------------------------

IpcServer::IpcServer(IpcConnection *controlConnection,
		     IpcServerClass *serverClass)
{
  controlConnection_ = controlConnection;
  serverClass_ = serverClass;
  str_pad(progFileName_,IpcMaxGuardianPathNameLength,0);
}

IpcServer::~IpcServer()
{
  logEspRelease(__FILE__, __LINE__);  
  if (controlConnection_)
    {
#ifdef IPC_INTEGRITY_CHECKING
      IpcEnvironment * ie = controlConnection_->getEnvironment();
      IpcAllConnections * allc = ie->getAllConnections();

      ie->checkIntegrity();
#endif
      stop();
      delete controlConnection_;
      controlConnection_ = NULL;

#ifdef IPC_INTEGRITY_CHECKING
      ie->checkIntegrity();
#endif
    }
}

void IpcServer::release()
{
  serverClass_->freeServerProcess(this);
}

void IpcServer::stop()
{
  // TBD $$$$ (in implementations for derived classes)
}

IpcGuardianServer *IpcServer::castToIpcGuardianServer()
{
  // IpcGuardianServer::castToIpcGuardianServer() returns a non-null value
  return NULL;
}

void IpcServer::logEspRelease(const char * filename, int lineNum, 
                              const char *msg)
{
  IpcConnection *cc = controlConnection_;
  if (cc &&
      cc->getEnvironment() &&
      cc->getEnvironment()->getLogReleaseEsp())
  {
    /*
    Coverage notes: to test this code in a dev regression requires
    changing $TRAF_VAR/ms.env.  However, it was tested in
    stress test on May 10, 2012.
    */
    char logMsg[500];

    // get the other end's name.
    char espName[32];
    Int32 pnameLen = cc->getOtherEnd().getPhandle().toAscii(
                       espName, sizeof(espName));
    espName[pnameLen] = '\0';

    // get the error #, if available.  Else, use -99.
    GuaErrorNumber guaError = -99;
    if (cc->castToGuaConnectionToServer())
      guaError = cc->castToGuaConnectionToServer()->getGuardianError();

    // get replySeqNum_ and state_
    ULng32 replySeqNum = cc->getReplySeqNum();

    char *state = (char *) "No State";
    switch (cc->getState())
    {
      case IpcConnection::INITIAL: 
        state = (char *) "INITIAL"; 
        break;
      case IpcConnection::OPENING: 
        state = (char *) "OPENING"; 
        break;
      case IpcConnection::ESTABLISHED: 
        state = (char *) "ESTABLISHED"; 
        break;
      case IpcConnection::SENDING: 
        state = (char *) "SENDING"; 
        break;
      case IpcConnection::REPLY_PENDING: 
        state = (char *) "REPLY_PENDING"; 
        break;
      case IpcConnection::RECEIVING: 
        state = (char *) "RECEIVING"; 
        break;
      case IpcConnection::CANCELLING: 
        state = (char *) "CANCELLING"; 
        break;
      case IpcConnection::ERROR_STATE: 
        state = (char *) "ERROR_STATE"; 
        break;
      case IpcConnection::CLOSED: 
        state = (char *) "CLOSED"; 
        break;
    }

    if (msg)
      str_sprintf(logMsg,
      "Releasing ESP %s , %s,"
      "guaError = %d, replySeqNum = %d, state = %s",
      espName,
      msg,
      guaError,
      replySeqNum,
      state);
    else
      str_sprintf(logMsg,
      "Releasing ESP %s ,"
      "guaError = %d, replySeqNum = %d, state = %s",
      espName,
      guaError,
      replySeqNum,
      state);
    
    SQLMXLoggingArea::logExecRtInfo(filename, lineNum, logMsg, 0);
  }
}

// -----------------------------------------------------------------------
//  Methods for class IpcConnection
// -----------------------------------------------------------------------

IpcConnection::IpcConnection(IpcEnvironment *env,
			     const IpcProcessId &pid,
                             const char *eye) :
  otherEnd_(pid),
  sendQueue_(env->getHeap()),
  receiveQueue_(env->getHeap()),
  replySeqNum_(0),
  recvStreams_(env->getHeap()),
  stopWait_(FALSE),
  trustIncomingBuffers_(TRUE),
  ipcMsgBufCheckFailed_(FALSE),
  breakReceived_(FALSE),
  lastTraceIndex_(NumIpcConnTraces-1),
  fileNumForIOCompletion_(InvalidGuaFileNumber),
  sendPersistentOpenReconnect_(FALSE)
{
  // ---------------------------------------------------------------------
  // Copy the eye catcher
  // ---------------------------------------------------------------------
  str_cpy_all((char *) &eyeCatcher_, eye, 4);

  state_ = INITIAL;
  environment_ = env;

  IpcAllConnections *allConn = environment_->getAllConnections();

  // find a free entry in the array that stores pointers to all connections
  id_ = allConn->unusedIndex();

  // insert this connection into the free slot and remember the slot number
  allConn->insertAt(id_,this);

  clearErrorInfo();
  MXTRC_2("IpcConnection::IpcConnection id=%d this=%x\n", id_, this);
#ifdef IPC_INTEGRITY_CHECKING
  cerr << "Just created IpcConnection " << (void *)this
    << " and inserted into IpcAllConnections " << (void *)allConn
    << "." << endl;
  checkIntegrity();
#endif
}

IpcConnection::~IpcConnection() 
{
  MXTRC_1("IpcConnection::~IpcConnection id=%d\n", id_);
  IpcAllConnections *allConn = environment_->getAllConnections();
  assert(!allConn->getPendingIOs().contains(id_));
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  assert(sendQueue_.entries() == 0);
  assert(receiveQueue_.entries() == 0);

  NABoolean couldRemove = allConn->remove(id_);
  assert(couldRemove);
#ifdef IPC_INTEGRITY_CHECKING
  cerr << "Just destroyed IpcConnection " << (void *)this
    << " and removed from IpcAllConnections " << (void *)allConn
    << "." << endl;
  checkIntegrity(FALSE /* suppress orphan checking */);
#endif
}

const char *IpcConnection::getConnectionStateString(IpcConnectionState s)
{
  switch (s)
  {
    case INITIAL: return "INITIAL";
    case OPENING: return "OPENING";
    case ESTABLISHED: return "ESTABLISHED";
    case SENDING: return "SENDING";
    case REPLY_PENDING: return "REPLY_PENDING";
    case RECEIVING: return "RECEIVING";
    case CANCELLING: return "CANCELLING";
    case ERROR_STATE: return "ERROR_STATE";
    case CLOSED: return "CLOSED";
    default: return ComRtGetUnknownString((Int32) s);
  }
}

void IpcConnection::setState(IpcConnectionState s)
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  if (s == SENDING OR s == RECEIVING OR s == OPENING)
  {
    environment_->getAllConnections()->IOPending(id_);
  }
  else if (s != IpcConnection::ERROR_STATE)
  {
    environment_->getAllConnections()->IOComplete(id_);
  }
  else
  {
    // 
    // Upon reaching the ERROR_STATE state, we should only announce 
    // I/O completion if all the following are true:
    // 
    //  a) there are no more send queue entries
    //  b) no recv queue entry has a registered callback that
    //     has not been delivered
    // 
    // If there are no send queue entries and no callbacks are pending
    // when the ERROR state is reached then we announce completion.
    // Otherwise, more I/O attempts will be made, and for each attempt
    // we will detect the ERROR and transition to the ERROR state.
    // Eventually we will transition to ERROR and there will be no more
    // send queue entries nor callbacks pending, and at that point we
    // can announce completion.
    //
    if (sendQueueEntries() == 0 && numReceiveCallbacksPending() == 0)
    {
      environment_->getAllConnections()->IOComplete(id_);
    }
  }

  if (state_ != s)
  {
    // Take care of tracing first.
    if (++lastTraceIndex_ >= NumIpcConnTraces)
      lastTraceIndex_ = 0;
    traceState_[lastTraceIndex_].oldState_ = state_;
    traceState_[lastTraceIndex_].mostRecentSendBuffer_ = lastSentBuffer_;
    traceState_[lastTraceIndex_].mostRecentReceiveBuffer_ = lastReceivedBuffer_;
    if (environment_->getLogTimeIpcConnectionState())
      clock_gettime(CLOCK_REALTIME, 
                  &traceState_[lastTraceIndex_].stateChangeTime_);

    // Now a state change.
    if (s == OPENING)
      getEnvironment()->incrNumOpensInProgress();
    else if (state_ == OPENING)
      getEnvironment()->decrNumOpensInProgress();
    state_ = s;
  }  
}

NABoolean IpcConnection::moreWaitsAllowed()
{
  return TRUE;
}

SockConnection *IpcConnection::castToSockConnection()
{
  return NULL;
}

GuaConnectionToServer *IpcConnection::castToGuaConnectionToServer()
{
  return NULL;
}

GuaMsgConnectionToServer *IpcConnection::castToGuaMsgConnectionToServer()
{
  return NULL;
}

GuaConnectionToClient *IpcConnection::castToGuaConnectionToClient()
{
  return NULL;
}

Int64 IpcConnection::getSqlTableTransid()
{
  return -1;
}

void IpcConnection::openPhandle(char * processName, NABoolean parallelOpen)
{
  assert(FALSE);
}

IpcMessageBuffer * IpcConnection::getNextSendQueueEntry()
{
  IpcMessageBuffer *msgBuf = removeNextSendBuffer();
  if (msgBuf)
    prepareSendBuffer(msgBuf);
  return msgBuf;
}

void IpcConnection::IOPending()
{
  environment_->getAllConnections()->IOPending(id_);
}

void IpcConnection::IOComplete()
{
  environment_->getAllConnections()->IOComplete(id_);
}

bool IpcConnection::isServerSide()
{
  return false;
}

IpcMessageBuffer *IpcConnection::removeNextSendBuffer()
{
  IpcMessageBuffer *msgBuf = NULL;
  if (sendQueue_.getFirst(msgBuf))
    return msgBuf;
  return NULL;
}

IpcMessageBuffer *IpcConnection::removeNextReceiveBuffer()
{
  IpcMessageBuffer *msgBuf = NULL;
  if (receiveQueue_.getFirst(msgBuf))
    return msgBuf;
  return NULL;
}

void IpcConnection::removeReceiveStreams()
{
  for (CollIndex j = 0; j < recvStreams_.entries(); j++)
      recvStreams_.removeAt(j);
}



void IpcConnection::prepareSendBuffer(IpcMessageBuffer *msgBuf)
{
  assert(msgBuf);

  // the client side does not send sequence number as we rely on the msg
  // system to maintain the send order.
  //
  // the server side needs to send sequence number. this is because if the
  // client side uses GuaMsgConnectionToServer, it picks up arrival reply
  // msgs in random order, even though the replies are delivered in correct
  // order by the msg system.
  InternalMsgHdrInfoStruct *msgHdr =
      (InternalMsgHdrInfoStruct*)(msgBuf->data(0)); 

  if (isServerSide())
    {
      msgHdr->setSeqNum(replySeqNum_);
      // increment reply seq number for the server side
      replySeqNum_++;
    }
  if (sendPersistentOpenReconnect_)
  {
    msgHdr->setSockReplyTag(PERSISTENT_OPEN_RECONNECT_CODE);
    setSendPersistentOpenReconnect(FALSE);

  }
  else
    msgHdr->setSockReplyTag(0);

}

IpcMessageBuffer * IpcConnection::getNextReceiveQueueEntry()
{
  if (receiveQueue_.entries() == 0)
    return NULL;

  // here's the design of message sequence number:
  //
  // - the client side does not send sequence number. so the server side does
  //   not need to verify sequence number upon request arrival. the msg system
  //   ensures the msgs are delivered in their original send order.
  // - the server sends the sequence number. so the client side needs to
  //   verify sequence number upon reply arrival. this is because if the
  //   client side uses GuaMsgConnectionToServer, it picks up arrival reply
  //   msgs in random order, even though the replies are delivered in correct
  //   order by the msg system.
  // - in case of IPC error, we don't know if the server was able to properly
  //   assign a sequence number to the buffer, thus we bypass the sequence
  //   number check. 
  //
  // note: the msg system guarantees that msgs are delivered in their send
  // order, with the exception of msgs being delivered through different
  // physical wires. for example, in case of servernet error, msgs could get
  // re-routed through the expand network.
  //
  if (isServerSide() || getState() == ERROR_STATE)
    {
      // this is on the server side, or we got error.
      // no need to verify reply seq number.
      IpcMessageBuffer *msgBuf = receiveQueue_[0];
      for (CollIndex j = 0; j < recvStreams_.entries(); j++)
        {
          if ((msgBuf->getMessageStream() == NULL)    ||        
              (msgBuf->getMessageStream() == recvStreams_[j]) )
            {
              // found a valid msg on receive queue
              receiveQueue_.removeAt(0);
              msgBuf->addCallback(recvStreams_[j]);
              recvStreams_.removeAt(j);
              return msgBuf;
            }
        }

      // if we reach here it means the next msg on receive queue does not
      // match any receiving streams. we must wait for message stream to
      // ask for it.
    }
  else
    {
      // this is on the client side and we did not get error.
      // we need to verify the reply seq number.
      for (CollIndex i = 0; i < receiveQueue_.entries(); i++)
        {
          IpcMessageBuffer *msgBuf = receiveQueue_[i];

          // unpack message header which contains the sequence number
          InternalMsgHdrInfoStruct* msgHdr =
            (InternalMsgHdrInfoStruct*)(msgBuf->data(0)); 
          if (msgHdr->getSeqNum() == replySeqNum_)
            {
              // the next msg on receive queue is the expected reply
              for (CollIndex j = 0; j < recvStreams_.entries(); j++)
                {
                  if ((msgBuf->getMessageStream() == NULL)    ||        
                      (msgBuf->getMessageStream() == recvStreams_[j]) )
                    {
                      // found a valid msg on receive queue
                      receiveQueue_.removeAt(i);
                      msgBuf->addCallback(recvStreams_[j]);

                      // for the SeaMonster continue protocol we cannot
                      // delete the stream from recvStreams_ till
                      // end of the batch reply is sent.
                      if (this->castToSMConnection())
                      {
                        InternalMsgHdrInfoStruct *hdr =
                             (InternalMsgHdrInfoStruct *) msgBuf->data(0);
                        ESPMessageTypeEnum streamType =
                             (ESPMessageTypeEnum) hdr->getType();

                        IpcBufferedMsgStream *str = NULL;
                        if (msgBuf->getMessageStream())
                          str = msgBuf->getMessageStream()->
                            castToIpcBufferedMsgStream();

                        if (str && str->getSMContinueProtocol())
                        {
                          if ( streamType == IPC_MSG_SQLESP_DATA_REPLY &&
                               hdr->getSMLastInBatch() )
                            recvStreams_.removeAt(j);
                        }
                         else
                           recvStreams_.removeAt(j);
                      }
                      else
                        recvStreams_.removeAt(j);

                      // increment reply seq number for the client side
                      replySeqNum_++;
                      return msgBuf;
                    }
                } // for j

              // if we reach here it means the next msg on receive queue does
              // not match any receiving streams. we must wait for message
              // stream to ask for it.
              break;
            }
          else
            {
              // the next msg on receive queue is not the expected reply.
              // continue to the subsequent msg on receive queue.
              continue;
            }
        } // for i
    }

  return NULL;
}

void IpcConnection::setFatalError(IpcMessageStreamBase *msgStream)
{ 
  if (getState() != ERROR_STATE)
    setState(ERROR_STATE);
  
  if (getErrorInfo() == 0)
    setErrorInfo(-1);

  // receive queue may not be empty. if so invoke receive callback.
  // example:
  // GuaMsgConnectionToServer::setFatalError() invokes handleIOErrorForStream()
  // that may have put message buffer on receive queue.
  IpcMessageBuffer *receiveBuf;
  while (receiveBuf = getNextReceiveQueueEntry())
    receiveBuf->callReceiveCallback(this);

  // - what if recvStreams_ is not empty? should we clean up I/Os on them?
}

#ifdef IPC_INTEGRITY_CHECKING

// methods that perform integrity checking on Ipc-related data structures

void IpcConnection::checkIntegrity(NABoolean checkIfOrphan)
  {
  // If the parameter "checkIfOrphan" is true, we will do the orphan check;
  // otherwise we will not. (The caller passes FALSE in contexts where it
  // is valid to be an orphan, e.g. at the end of the IpcConnection destructor.)

  // if checking, assume the worst: this object is an orphan
  isOrphaned_ = checkIfOrphan;  

  environment_->checkIntegrity(); // traverse up to IpcEnvironment, do check
  if (isOrphaned_)
    {
    cerr << "Found orphaned IpcConnection object " << (void *)this << "." << endl;
    assert(!isOrphaned_);
    }
  }

void IpcConnection::checkLocalIntegrity(void)
  {
  isOrphaned_ = FALSE;  // ah, this object isn't an orphan after all
  }

#endif

NABoolean IpcConnection::newClientConnection(IpcMessageBuffer *receivedBuffer)
{
  InternalMsgHdrInfoStruct* msgHdr =
         (InternalMsgHdrInfoStruct*)(receivedBuffer->data(0));
  if (msgHdr->getSeqNum() == 0 && msgHdr->getSockReplyTag() == PERSISTENT_OPEN_RECONNECT_CODE)
  {
    return TRUE;
  }
  else
    return FALSE;
}


void IpcConnection::reportBadMessage()
{
  const char option2 = '2';
  const char *envvar = getenv("ESP_PROPAGATE_ASSERT");
  if (envvar == NULL)
    envvar = &option2;
  if (getEnvironment()->getIpcServerType() == IPC_SQLESP_SERVER &&
      ((*envvar == '1' || *envvar == '2')))
  {
    if (*envvar == '2')
    {
      NAProcessHandle phandle;
      phandle.getmine();
      phandle.decompose();
      UInt32 seconds = (phandle.getPin() % 100) * 5;
      if (seconds >= 250)
        seconds -= 250;
      sleep(seconds);

      genLinuxCorefile(NULL);
    }
    dumpAndStopOtherEnd(true, (*envvar == '2'));
    if (*envvar == '2')
      NAExit(0);  // Already generated core file of myself
  }
}

// -----------------------------------------------------------------------
//  Methods for class IpcAllConnections
// -----------------------------------------------------------------------


// wait for something to happen on any of the connections like awaitio(-1)
WaitReturnStatus IpcAllConnections::waitOnAll(
     IpcTimeout timeout,
     NABoolean calledByESP,
     NABoolean *timedout,
     Int64 *waitTime)
{
  WaitReturnStatus retcode;
  struct timespec startts;
  struct timespec endts;
  clock_gettime(CLOCK_MONOTONIC, &startts);

  if (timeout != IpcImmediately && timeout != IpcInfiniteTimeout) {
     short mask;
     if (ipcEnv_->getControlConnection() != NULL && (! GetCliGlobals()->isEspProcess())) {
        mask = XWAIT(LREQ | LDONE, timeout);
        if (mask & LREQ) 
           retcode = ipcEnv_->getControlConnection()->castToGuaReceiveControlConnection()->wait(IpcImmediately);
        else if (mask & LDONE)
           retcode = pendingIOs_->waitOnSet(IpcImmediately, calledByESP, timedout); 
        if (timedout != NULL) {
           if (mask != 0)
              *timedout = FALSE;
           else
              *timedout = TRUE;
        }
     }
     else
        retcode = pendingIOs_->waitOnSet(timeout, calledByESP, timedout); 
  }
  else
     retcode = pendingIOs_->waitOnSet(timeout, calledByESP, timedout); 
  clock_gettime(CLOCK_MONOTONIC, &endts);
  if (startts.tv_nsec > endts.tv_nsec)
    {
      // borrow 1 from tv_sec, convert to nanosec and add to tv_nsec.
      endts.tv_nsec += 1 * 1000 * 1000 * 1000;
      endts.tv_sec -= 1;
    }
  if (waitTime != NULL) 
    *waitTime = ((endts.tv_sec - startts.tv_sec) * 1000LL * 1000LL * 1000LL)
      +  (endts.tv_nsec - startts.tv_nsec);
  return retcode;
}

#ifdef IPC_INTEGRITY_CHECKING

void IpcAllConnections::checkIntegrity(void)
  {
  // try to traverse to IpcEnvironment... to get there, we need to go
  // via IpcConnection...  
  CollIndex firstConn = 0;
  
  // find first IpcConnection * (if any)
  while ((!used(firstConn)) && (firstConn < entries()))
    firstConn++;

  if (firstConn < entries())  
    {
    // we have a connection
    IpcConnection * c = usedEntry(firstConn);
    
    // traverse up to IpcEnvironment, do check
    c->checkIntegrity(); 
    }
  // else ... can't get there; just do nothing
  }

void IpcAllConnections::checkLocalIntegrity(void)
  {
  // check integrity of subarray
  pendingIOs_->checkLocalIntegrity();

  // check integrity of the set of connections
  CollIndex conn = 0;
  ULng32 entriesChecked = 0;
  
  // find first IpcConnection * (if any)
  while (entriesChecked < entries())
    {
    if (used(conn))
      {
      at(conn)->checkLocalIntegrity();
      entriesChecked++;
      }
    conn++;
    }
  }

#endif

CollIndex IpcAllConnections::fillInListOfPendingPins(char *buff,
                                                     ULng32 buffSize,
                                                     CollIndex numOfPins)
{
  CollIndex i;
  CollIndex firstConnInd = 0;
  CollIndex numOfPendings = MINOF(numOfPins, pendingIOs_->entries());
  char tempB[300];
  Int32 len;

  buff[0] = '\0';   // null terminate
  for (i = 0; i < numOfPendings; i++)
    {
      if (NOT pendingIOs_->setToNext(firstConnInd))
        break;   // should not happen
      len = pendingIOs_->element(firstConnInd)->
                         getOtherEnd().toAscii(tempB, 300);
      if (len > 0 && (ULng32) len + 2 < buffSize)
        {
          if (buff[0] != '\0')  // not the first entry
            {
              str_cat(buff, ", ", buff);
              buffSize -= 2;
            }
          str_cat(buff, tempB, buff);
          buffSize -= len;
        }
      else if (len > 0)
        break;   // no room left
      firstConnInd++;
    }

  return i;
}

void IpcAllConnections::fillInListOfPendingPhandles(GuaProcessHandle *phandles,
						    CollIndex& numOfPhandles)
{
  CollIndex firstConnInd = 0;
  numOfPhandles = MINOF(numOfPhandles, pendingIOs_->entries());
  for (CollIndex i = 0; i < numOfPhandles; i++)
    {
      if (NOT pendingIOs_->setToNext(firstConnInd))
	{
	  // should not happen
	  numOfPhandles = i;
	  return;
	}

      memcpy((char *)&phandles[i],
	     (char *)&pendingIOs_->element(firstConnInd)->getOtherEnd().getPhandle().phandle_,
	     sizeof(GuaProcessHandle));

      firstConnInd++;
    }
}

// Methods for connection tracing
void IpcAllConnections::print()
{
  char buf[10000];
  Int32 lineno = 0;

  while (printConnTrace(lineno, buf))
    {
      printf("%s", buf);
      lineno++;
    }
}

const char *ConnTraceDesc = "All IpcConnections and their states";

void IpcAllConnections::registTraceInfo(IpcEnvironment *env, ExeTraceInfo *ti)
{
  if (env)
    {
      if (ti)
        {
          Int32 lineWidth = 50; // temp
          void *regdTrace;
          Int32 ret = ti->addTrace("IpcConnectionState", this, -1, 4,
                               this, getAnEntry,
                               NULL,
                               lineWidth, ConnTraceDesc, &regdTrace);
          if (ret == 0)
          {
            // trace info added successfully, now add entry fields
            ti->addTraceField(regdTrace, "Connection ", 0,
                              ExeTrace::TR_POINTER32);
            ti->addTraceField(regdTrace, "Type ", 1, ExeTrace::TR_STRING);
            ti->addTraceField(regdTrace, "OtherEnd     ", 2,
                              ExeTrace::TR_STRING);
            ti->addTraceField(regdTrace, "State ", 3, ExeTrace::TR_STRING);
            traceRef_ = regdTrace;
          }
        }
    }
}

Int32 IpcAllConnections::printConnTrace(Int32 lineno, char *buf)
{
  if (lineno == 0)
    printEntry_ = 0;  // first time to print all entries

  // find first IpcConnection * (if any)
  while ((!used(printEntry_)) && (printEntry_ < entries()))
    printEntry_++;

  if (printEntry_ >= entries())
    return 0;  // no more connections

  IpcConnection *c = usedEntry(printEntry_++); // then advance to next
  Int32 rv = 0;
  if (c)
    {
      const char * stateName = "UNKNOWN";
      const char * eyeCatcher = "UNKN";
      Int32 cpu = 0, node, pin = 0;
      SB_Int64_Type seqNum = -1;

      if ((IpcConnection::INITIAL <=  c->getState()) && (c->getState() <= IpcConnection::CLOSED))
          stateName = IpcConnStateName[c->getState()];
      if ((char *)NULL != c->getEyeCatcher())
         eyeCatcher = c->getEyeCatcher();
      IpcNetworkDomain domain = c->getOtherEnd().getDomain();
      if (domain == IPC_DOM_GUA_PHANDLE)
        {
          GuaProcessHandle *otherEnd = (GuaProcessHandle *)&(c->getOtherEnd().getPhandle().phandle_);
          if (otherEnd)
            otherEnd->decompose(cpu, pin, node
                               , seqNum
                               );
        }
      rv = sprintf(buf, "%.4d  %8p  %.4s  %.3d,%.8d %" PRId64 " %s\n",
                   lineno, c, eyeCatcher, cpu, pin, seqNum,
                   stateName);
    }
  return rv;
}

// -----------------------------------------------------------------------
//  Methods for class IpcSetOfConnections
// -----------------------------------------------------------------------

IpcSetOfConnections::IpcSetOfConnections(IpcAllConnections *superset,
					 CollHeap* hp,
					 NABoolean eventDriven, NABoolean esp) :
        SUBARRAY(IpcConnection *)( (ARRAY(IpcConnection *) *) superset, hp),
        cancelWait_(FALSE), allc_(superset),
	eventDriven_(eventDriven), callCount_(0), pollCount_(0),esp_(esp),
	waitCount_(0), ldoneCount_(0), lreqCount_(0), lsigCount_(0),
        smCompletionCount_(0), timeoutCount_(0), activityPollCount_(0),
        lastWaitStatus_(0), ipcAwaitioxEnabled_(TRUE)
{}

IpcSetOfConnections::IpcSetOfConnections (const IpcSetOfConnections & orig, 
                                          CollHeap * h)
     : SUBARRAY(IpcConnection *)(orig, h)
{
  cancelWait_ = FALSE;
  allc_ = orig.allc_;
  eventDriven_ = orig.eventDriven_;
  callCount_ = 0;
  pollCount_ = 0;
  esp_ = orig.esp_;
  waitCount_ = 0;
  ldoneCount_ = 0;
  lreqCount_ = 0;
  lsigCount_ = 0;
  smCompletionCount_ = 0;
  timeoutCount_ = 0;
  activityPollCount_ = 0;
  lastWaitStatus_ = 0;
  ipcAwaitioxEnabled_ = FALSE;
  memset((void *)&ipcAwaitiox_, 0, sizeof(IpcAwaitiox));
}

NABoolean IpcSetOfConnections::moreWaitsAnyConnection()
{
  if (cancelWait_)
    { // Break the wait loop for an asynchronous cancel request.
      cancelWait_ = FALSE;
      return FALSE;
    }
  
  for (CollIndex i = 0; setToNext(i); i++)
    {
      if (element(i)->moreWaitsAllowed())
	return TRUE;
    }
  return FALSE;
}

void IpcSetOfConnections::waitOnSMConnections(IpcTimeout timeout)
{
  for (CollIndex i = 0; setToNext(i); i++)
  {
    IpcConnection *conn = element(i)->castToSMConnection();
    if (conn)
      conn->wait(timeout);
  }
}

WaitReturnStatus IpcSetOfConnections::waitOnSet(IpcTimeout timeout,
						NABoolean calledByESP,
						NABoolean *timedout)
{
  #define  MAX_TOTALWAITTIME   2147483000     //soln 10-061230-1405	
  if (timedout != NULL)
    *timedout = FALSE;
  NABoolean interruptRecvd = FALSE;
	MXTRC_FUNC("IpcSetOfConnections::wait");
	MXTRC_1("timeout=%d\n", timeout);
  // could use select UNIX call or AWAITIOX(-1) later $$$$

  // for now, do one round with zero timeout, then let more rounds follow
  // with exponentially increasing timeouts. Stop when an I/O completes.
  // Make sure to check all other connections before returning after
  // an unsucessful wait on a single connection with no timeout.

  // timeout increment (in 10 msec units), this value is the duration
  // of the first wait cycle we do
  const IpcTimeout  toInc = 1;

  // number of times we just give up the time slice before we actually
  // start to wait (with a timeout of toInc)
  const Lng32        timeSlices = 3;

  IpcTimeout        tout = 0;
  IpcTimeout        totalWaitTime = -1;
  CollIndex         firstConnInd, currentFirstConnInd;
  IpcConnection     *firstConnection = NULL; // For ESP it's conn to the Master
  NABoolean         somethingCompleted = FALSE;
  ULng32     seqNo = 0;
  IpcAllConnections *allc = NULL;
  Lng32              currTimeSlices = timeSlices;
  IpcEnvironment *env = NULL;
  NABoolean ldoneConsumed = FALSE, activity = FALSE;
  short waitFlag;
  short status;
  Int64 currentTime;
  callCount_ += 1;
  if (timeout == -2)
    timeout = -1;
  NABoolean isWaited;
  Int32 isWaitedFactor = 20;
  IpcTimeout totalOfTimeouts = 0;
  NABoolean freeUnusedMemory = TRUE;
  CliGlobals *cliGlobals = NULL;
  NABoolean soloClient = FALSE;
  NABoolean receivedSMEvent = FALSE;
  ExSMGlobals *smGlobals = ExSMGlobals::GetExSMGlobals();;
  ExSMReadyList *smReadyList = (smGlobals ? smGlobals->getReadyList() : NULL);

  // loop, exponentially increasing the timeout used to wait
  // on the first connection of the set
  while (NOT somethingCompleted)
  {
    if (totalWaitTime >= timeout AND timeout != IpcInfiniteTimeout)
    {
      if (timedout != NULL)
      {
	*timedout = TRUE;
      }
      break;
    }
    else
    {
      // get a hold of the first connection in the set, return if there is
      // none (note that the set may change during the while loop)
      firstConnInd = 0;
      if (NOT setToNext(firstConnInd))
	{
	  if ( calledByESP ) 
	    {
	      NAExit(0); // Stop this ESP! No point executing w/o connections.
	    }
	  // set is empty, return right away
	  assert(!ipcAwaitiox_.getCompleted()); // Shouldn't happen--return with AWAITIOX(-1) completion outstanding
	  return WAIT_OK;
	}
      else
	// for an ESP, the first connection is always the master
	firstConnection = element(firstConnInd);

      env = firstConnection->getEnvironment();
      cliGlobals = env->getCliGlobals();

      assert(env->getAllConnections()->getNumPendingIOs() > 0 ||
             timeout != IpcInfiniteTimeout);

      static bool sv_solo_client_completion = false;
      static char escc = '1';
      if (!sv_solo_client_completion)
      {
        sv_solo_client_completion = true;
        char *sccEnvvar = getenv("IPC_SOLO_CLIENT_COMPLETION");
        if (sccEnvvar)
          escc = *sccEnvvar;
      }
      if (escc == '1' && firstConnection->getFileNumForIOCompletion() == 1 &&
          env->getAllConnections()->entries() == 1 && !calledByESP
          && timeout == -1)
        soloClient = TRUE;
      isWaited = eventDriven_ && timeout == -1 && env->getMaxPollingInterval() != 301;
      pollCount_ += 1; // Increment the connections polled count
      waitFlag = LDONE;
      if (esp_)
	waitFlag |= LREQ;
      else if (env->breakEnabled() && !env->lsigConsumed())
	waitFlag |= LSIG;
      status = -1;

      // On Linux if SeaMonster is enabled, set the LRABBIT bit in
      // waitFlag
      Int32 numSMConnections = env->getAllConnections()->getNumSMConnections();
      if (numSMConnections > 0)
        waitFlag |= LRABBIT;

      // if this is the first time, remember the sequence number of
      // completed I/Os so far, if that number changes this method will
      // return
      if (allc == NULL)
	{
	  // get the completion sequence number so far (get the IPC
	  // environment from one of the connections in the set)
	  ldoneConsumed = env->ldoneConsumed();
	  activity = env->isEvent(AEVENT);
	  allc = env->getAllConnections();
	  allc->incrRecursionCount();
          // The sequence number at entry is captured so that it can later
          // be used to determine if any messages have completed. If so,
          // there is work for the scheduler to do and somethingCompleted
          // is set to TRUE causing a timed or infinite waitOnSet to
          // return.
	  seqNo = allc->getCompletionSeqenceNo();
	  totalOfTimeouts = 0;
	}
      else if (isWaited && !env->ldoneConsumed() && !soloClient)
      {
	if (env->isEvent(AEVENT))
	{
	  activityPollCount_ += 1;
	  totalOfTimeouts = 0;
	}
	else
	{
	  IpcTimeout waitInterval = (tout + 1) * isWaitedFactor;
	  if (waitInterval == 0)
	    lastWaitStatus_ = status = XWAIT0(waitFlag, waitInterval);
	  else
	    lastWaitStatus_ = status = XWAITNO0(waitFlag, waitInterval);
	  waitCount_ += 1;
          
	  if (status & LDONE)
	  {
	    ldoneCount_ += 1;
	    ldoneConsumed = TRUE;
	    totalOfTimeouts = 0;
            freeUnusedMemory = TRUE;
	  }
	  else if (status & LREQ)
	  {
	    lreqCount_ += 1;
	    totalOfTimeouts = 0;
            freeUnusedMemory = TRUE;
	  }
	  else if (status & LSIG)
	  {
	    lsigCount_ += 1;
	    env->setLsigConsumed(TRUE);
	    totalOfTimeouts = 0;
            freeUnusedMemory = TRUE;
	  }
	  else if (status & LRABBIT)
	  {
	    smCompletionCount_++;
            totalOfTimeouts = 0;
            freeUnusedMemory = TRUE;

            EXSM_TRACE(EXSM_TRACE_PROTOCOL,
                       "RECV LRABBIT wait count %" PRId64
                       " sm count %" PRId64 " timeout %d",
                       (Int64) waitCount_, (Int64) smCompletionCount_,
                       (int) timeout);
	  }
	  else
	  {
	    totalOfTimeouts += waitInterval;
	    if (calledByESP)
	      {
		if (allc->entries() == 1 ||
                    // Persistent opens AND all connections are (obsolete)
                    // client connections AND ESP is confirmed to be idle 
                    (env->getPersistentOpens() &&
                     allc->entries() == env->getControlConnection()->castToGuaReceiveControlConnection()->getClientConnections()->entries() &&
                     env->getIdleTimestamp() > 0))
		  {
		    // master has released this esp fragment
		    if (env->getStopAfter() > 0)
		      {
			assert(env->getIdleTimestamp() > 0);
			currentTime = NA_JulianTimestamp();
			Int64 timeDiff = currentTime - env->getIdleTimestamp();
			if (timeDiff > ((Int64)env->getStopAfter() * 1000000))
                        {
                          if (env->getLogEspIdleTimeout())
                          {
                            /*
                            Coverage notes: to test this code in a dev 
                            regression requires changing 
                            $TRAF_VAR/ms.env.  However, it was 
                            tested in stress test on 
                            May 10, 2012.
                            */
                            char myName[20];
                            memset(myName, '\0', sizeof(myName));
                            char buf[500];
                            msg_mon_get_my_info(NULL, NULL, 
                              myName, sizeof(myName),NULL,NULL,NULL,NULL);
                            str_sprintf(buf, 
                              "ESP_IDLE_TIMEOUT causes %s to exit.",
                              myName);
                            SQLMXLoggingArea::logExecRtInfo(__FILE__, 
                                                  __LINE__, buf, 0);
                          }
			  // stop esp if it has become idle and timed out
			  NAExit(0);
                        }

		      }
		  }
		else
		  {
		    if (env->getInactiveTimeout() > 0 &&
			env->getInactiveTimestamp() > 0)
		      {
			currentTime = NA_JulianTimestamp();
			Int64 timeDiff = currentTime - env->getInactiveTimestamp();
			if (timeDiff > ((Int64)env->getInactiveTimeout() * 1000000))
			  // stop esp if it has become inactive and timed out
			  NAExit(0);
		      }
		  }
	      }
            timeoutCount_ += 1;

	  } // XWAIT timed out
	} // if (env->isEvent(AEVENT)) else
      } // else if (isWaited && !env->ldoneConsumed() && !soloClient)


      // -----------------------------------------------------------------
      // wait with timeout to on the first (and maybe only) connection
      // -----------------------------------------------------------------

      if (soloClient)
      {
        interruptRecvd = firstConnection->wait(timeout);
        if (interruptRecvd)
        {
	  allc->decrRecursionCount();
	  return WAIT_INTERRUPT;
        }
        else if (seqNo != allc->getCompletionSeqenceNo())
	  somethingCompleted = TRUE;
        continue;
      }

      if (currTimeSlices > 0 AND tout > 0 AND !isWaited)
	{
	  // give up the time slice and reset the timeout to 0 instead
	  // of waiting with a timeout that is greater than 10 milliseconds
	  // (10 msec is a long time on a 300 MHz machine)
	  currTimeSlices--;

	  // platform-dependent code to give up the time slice
	  // the Sleep() method on NT can be used to give up the processor
	  Sleep(0);
	  tout = 0;
      }

      env->setLdoneConsumed(FALSE); // So that we know if it changed if we loop again
      env->setEvent(FALSE, AEVENT); // So that we know if there was any "activity"
                                    // which means to: a) an AWAITIOX completed, b)
                                    // a MSG_ISDONE_ returned true, or c) something was
                                    // started such as tryToStartNewIO called MSG_LINK_.
      Int64 waitCalled = 0, waitReturned;
      if (!isWaited && tout > 0)
	waitCalled = NA_JulianTimestamp();


      NABoolean cycleThruConnections = TRUE;
      NABoolean doIpcAwaitiox;
      
      if (env->getMaxPollingInterval() == 302 || XAWAITIOX_MINUS_ONE == FALSE)
      {
        doIpcAwaitiox = FALSE;
      }
      else
      {
        doIpcAwaitiox =
          ipcAwaitioxEnabled_ &&
          (esp_ || env->getMasterFastCompletion()) &&
          eventDriven_ &&
          (isWaited || tout == 0);
      }
      
      while (cycleThruConnections)
      {
        cycleThruConnections = FALSE;
        interruptRecvd = FALSE;
        currentFirstConnInd = firstConnInd;
        
        if (doIpcAwaitiox)
        {
          if (esp_)
          {
            currentFirstConnInd = 0;
            if (setToNext(currentFirstConnInd) &&
                currentFirstConnInd != firstConnInd)
              // Can happen for SSMP but should never happen for ESP
              firstConnection = element(currentFirstConnInd);
          }
          
          ipcAwaitiox_.DoAwaitiox(esp_ ? FALSE : TRUE);
          
          if (ipcAwaitiox_.getFileNum() != -1)
            env->bawaitioxTrace(this, allc->getRecursionCount(),
                                currentFirstConnInd, firstConnection,
                                &ipcAwaitiox_);
          
          if (ipcAwaitiox_.getFileNum() ==
              firstConnection->getFileNumForIOCompletion())
          {
            interruptRecvd =
              firstConnection->wait(isWaited ? IpcImmediately : tout,
                                    env->getEventConsumed(),
                                    (ipcAwaitiox_.getFileNum() == -1
                                     ? NULL
                                     : &ipcAwaitiox_));
          }
        } // doIpcAwaitiox
        
        else
        {
          interruptRecvd =
            firstConnection->wait(isWaited ? IpcImmediately : tout,
                                  env->getEventConsumed());
        }
        
        if (env->ldoneConsumed())
          ldoneConsumed = TRUE;
        if (env->isEvent(AEVENT))
          activity = TRUE;
        
        if (interruptRecvd)
        {
          env->setLdoneConsumed(ldoneConsumed);
          env->setEvent(activity, AEVENT);
          allc->decrRecursionCount();
          return WAIT_INTERRUPT;
        }
        
        else if (!isWaited && !env->isEvent(AEVENT) && tout > 0 &&
                 seqNo == allc->getCompletionSeqenceNo())
        {
          // waitOnSet timeout assumes that the first connection will
          // wait the specified interval if an interupt is not received
          // and there was not a completion. The following code will
          // delay if the first connection did not wait the specified
          // interval.
          if (firstConnection->getFileNumForIOCompletion() != 1) // Fix bug 2903
          {
            // Don't delay for GuaConnectionToClient
            // (fileNumForIOCompletion equals 1 ($RECEIVE))
            waitReturned = NA_JulianTimestamp();
            IpcTimeout delayTime =
              tout - ((IpcTimeout)((waitReturned - waitCalled) / 10000));
            if (delayTime > 0)
            {
              timespec nanoDelayTime;
              nanoDelayTime.tv_sec = delayTime / 100;
              nanoDelayTime.tv_nsec = (delayTime % 100) * 10000000;
              Int32 retVal = nanosleep(&nanoDelayTime, NULL);
            }
          }
        }
        
        if (doIpcAwaitiox && ipcAwaitiox_.getFileNum() != -1 &&
            ipcAwaitiox_.getCompleted() == FALSE)
        {
          cycleThruConnections = TRUE;
          continue;
        }
        
        
        // add up the time spent waiting
        // (after about 280 days of waiting this would cause an overflow trap)
        if (totalWaitTime == -1 || isWaited)
          totalWaitTime = tout;
        else
        {
          if ( totalWaitTime < MAX_TOTALWAITTIME)  // soln 10-061230-1405 begin
            totalWaitTime += tout;
          else
            totalWaitTime = -1;                    // soln 10-061230-1405  end
        }
        
        // -----------------------------------------------------------------
        // wait with 0 timeout on all the other connections
        // -----------------------------------------------------------------
        for (CollIndex i = currentFirstConnInd+1; setToNext(i); i++)
        {
          IpcConnection *element_i = element(i);
          assert(element_i);
          
          if (doIpcAwaitiox)
          {
            if (ipcAwaitiox_.getFileNum() == element_i->getFileNumForIOCompletion())
              interruptRecvd = element_i->wait(IpcImmediately, env->getEventConsumed(),
                                               (ipcAwaitiox_.getFileNum() == -1 ?
                                                NULL : &ipcAwaitiox_));
            else
              continue;
          }
          else
          {
            interruptRecvd = element_i->wait(IpcImmediately, env->getEventConsumed());
          }
          
          if (env->ldoneConsumed())
            ldoneConsumed =TRUE;
          
          if (interruptRecvd)
          {
            env->setLdoneConsumed(ldoneConsumed);
            env->setEvent(activity, AEVENT);
            allc->decrRecursionCount();
            return WAIT_INTERRUPT;
          }
          
          if (doIpcAwaitiox && ipcAwaitiox_.getFileNum() != -1 &&
              ipcAwaitiox_.getCompleted() == FALSE)
          {
            cycleThruConnections = TRUE;
            break;
          }

        } // for (CollIndex i = currentFirstConnInd+1; setToNext(i); i++)

        if (ipcAwaitiox_.getCompleted())
        {
          void *bufAddr;
          Int32 count;
          SB_Tag_Type tag;
          CollIndex usedLength = allc->getUsedLength();
          for (CollIndex i = 0; i < usedLength ; i++)
          {
            if (allc->getUsage(i) != UNUSED_COLL_ENTRY)
            {
              IpcConnection *conn = allc->usedEntry(i);
              if (conn->getState() == IpcConnection::ERROR_STATE &&
                  conn->getFileNumForIOCompletion() ==
                  ipcAwaitiox_.getFileNum())
              {
                // Clear completed_
                ipcAwaitiox_.ActOnAwaitiox(&bufAddr, &count, &tag);
              }
            }
          }
        }
        
      } // while (cycleThruConnections)
      
      if ( allc_->getRecursionCount() == 1 ) // delete closed connections
      {
        CollIndex usedLength = allc->getUsedLength();
        for (CollIndex i = 0;
             i < usedLength && allc->getDeleteCount() > 0; i++)
        {
          if (allc->getUsage(i) != UNUSED_COLL_ENTRY)
          {
            IpcConnection *conn = allc->usedEntry(i);
            if (conn->getState() == IpcConnection::CLOSED)
            {
              delete conn;
              allc->decrDeleteCount();
            }
          }
        }
      }
      
      // Compare the completion sequence numbers to see whether any
      // connections have completed.
      if (seqNo != allc->getCompletionSeqenceNo())
      {
        somethingCompleted = TRUE;
      }
      else
      {
        // increase the used timeout kind of exponentially by multiplying 
        // with 1.5 and then adding an increment (in 10 ms units).
        // Max out at 3 seconds of waiting time.
        tout = MINOF(env->getMaxPollingInterval(), ( tout + tout/2 + toInc ));
      }
      
      // if we have received a partial message, reset the timeout
      if (allc->getReceivedPartialMessage())
      {
        tout = 0;
        currTimeSlices = timeSlices;
        allc->setReceivedPartialMessage(FALSE);
      }
      if (!moreWaitsAnyConnection())
      {
        env->setLdoneConsumed(ldoneConsumed);
        env->setEvent(activity, AEVENT);
        allc->decrRecursionCount();
        return WAIT_OK;
      }
      
      // Check the SeaMonster completed task queue if there is one,
      // whether or not wait was called, and whether or not LRABBIT
      // was received if wait was called.
      // 
      // It pays to always check it because it's cheaper than figuring
      // out whether or not to.
      //
      // Notes about the "while (firstTask)" loop below
      // 
      // * getFirst() does not modify the ready list
      //
      // * The while loop is guaranteed to terminate because items in
      //   output queues are guaranteed to be removed until all are
      //   emptied and the completed task queue is emptied.
      // 
      // * In the common case, firstTask will be different on each
      //   successive call to getFirst(). It could be the same on
      //   successive calls if the reader thread is dispatched between
      //   successive calls, and the task is placed back at the head
      //   of the queue when it is empty.

      if (smReadyList)
      {
        ExSMTask *firstTask = NULL;
        while ((firstTask = smReadyList->getFirst()) != NULL)
          firstTask->getSMConnection()->wait(0);

        if (seqNo != allc->getCompletionSeqenceNo())
        {
          receivedSMEvent = TRUE;
          somethingCompleted = TRUE;
          EXSM_TRACE(EXSM_TRACE_PROTOCOL, "Done processing SM connections");
          continue;
        }
      }

    } // if (totalWaitTime >= timeout AND timeout != IpcInfiniteTimeout) else
  } // while (NOT somethingCompleted)
  
  if (!receivedSMEvent)
  {
    env->setLdoneConsumed(ldoneConsumed);
    env->setEvent(activity, AEVENT);
  }
  
  allc->decrRecursionCount();
  return WAIT_OK;
}

#ifdef IPC_INTEGRITY_CHECKING

void IpcSetOfConnections::checkIntegrity(void)
  {
  isOrphaned_ = TRUE;  // assume the worst: this object is an orphan

  // try to traverse to IpcEnvironment via IpcAllConnections and do check... 
  allc_->checkIntegrity();

  if ((isOrphaned_) && 
      (entries() > 0))  // entries > 0 ==> traverse to IpcEnvironment should have succeeded
    {
    cerr << "Found orphaned IpcSetOfConnections object." << endl;
    // assert(!isOrphaned_);  well, it might not really be orphaned;
    // it may be we can't get to its associated ExRtFragTable...

    // check this object's integrity now, since we didn't traverse to it
    checkLocalIntegrity(); 
    }
  }

void IpcSetOfConnections::checkLocalIntegrity(void)
  {
  isOrphaned_ = FALSE;

  CollIndex conn = 0;

  while (setToNext(conn))
    {

    // check to see if the corresponding element in the superset is used
    if (!allc_->used(conn))
      {
      cerr << "Found IpcConnection in subarray that is not in IpcAllConnections." << endl;
      assert(allc_->used(conn));
      }
    conn++;
    }
  }

#endif

// -----------------------------------------------------------------------
// Methods for class IpcControlConnection
// -----------------------------------------------------------------------

IpcControlConnection::~IpcControlConnection() {}

SockControlConnection * IpcControlConnection::castToSockControlConnection()
{
  return NULL;
}

GuaReceiveControlConnection *
IpcControlConnection::castToGuaReceiveControlConnection()
{
  return NULL;
}

// -----------------------------------------------------------------------
// Methods for class InternalMsgHdrInfoStruct
// -----------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// general constructor
InternalMsgHdrInfoStruct::InternalMsgHdrInfoStruct(
       IpcMessageObjType msgType,
       IpcMessageObjVersion version)
  : IpcMessageObj(msgType,version),
    totalLength_(0),
    alignment_(IpcMyAlignment),
    flags_(0),
    format_(0),
    sockReplyTag_(0),
    eyeCatcher_(Release1MessageEyeCatcher),
    seqNum_(0),
    msgStreamId_(0)
  { }

///////////////////////////////////////////////////////////////////////////////
// constructor used to perform copyless receive. unpacks objects in place.
InternalMsgHdrInfoStruct::InternalMsgHdrInfoStruct(
       IpcBufferedMsgStream* msgStream)
: IpcMessageObj(msgStream)
  { 
  // IpcBufferedMsgStream parm used to differentiate from default constructor
  if (getEndianness() != IpcMyEndianness)
    {
    swapFourBytes(totalLength_);
    swapTwoBytes(alignment_);
    swapTwoBytes(format_);
    swapTwoBytes(sockReplyTag_);
    swapFourBytes(eyeCatcher_);
    swapFourBytes(seqNum_);
    assert(0); // Need swapEightBytes() to swap msgStreamId_!
    setEndianness(IpcMyEndianness);
    }
  }

IpcMessageObjSize InternalMsgHdrInfoStruct::packedLength()
{
  return (IpcMessageObjSize) sizeof(*this);
}

IpcMessageObjSize InternalMsgHdrInfoStruct::packObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  IpcMessageObjSize result = IpcMessageObj::packObjIntoMessage(buffer);

  // we know that the packed representation has the same layout as the unpacked
  // representation, but it needs to be in big-endian format

  return result;
}

void InternalMsgHdrInfoStruct::unpackObj(IpcMessageObjType objType,
					 IpcMessageObjVersion objVersion,
					 NABoolean sameEndianness,
					 IpcMessageObjSize objSize,
					 IpcConstMessageBufferPtr buffer)
{
  assert(objSize == sizeof(*this));

  // the header always arrives in big endian format, so we should
  // see the same endianness if and only if this is a big-endian machine

  IpcMessageObj::unpackObj(objType,
			   objVersion,
			   sameEndianness,
			   objSize,
			   buffer);
}

// -----------------------------------------------------------------------
// Methods for class IpcMessageBuffer
// -----------------------------------------------------------------------

// Private constructor. Only called by public methods such as
// allocate(), createBuffer(), copy(), copyFromOffset().
IpcMessageBuffer::IpcMessageBuffer(CollHeap *heap,
                                   IpcMessageObjSize maxLen,
                                   IpcMessageObjSize msgLen,
                                   IpcMessageStreamBase *msg,
                                   short flags,
                                   short replyTag,
                                   IpcMessageObjSize maxReplyLength,
                                   Int64 transid)
  : InternalMessageBufferHeader(heap,
                                maxLen,
                                msgLen,
                                msg,
                                replyTag,
                                maxReplyLength,
                                transid,
                                flags)
{
}

IpcMessageBuffer *IpcMessageBuffer::allocate(IpcMessageObjSize maxLen,
					     IpcMessageStreamBase *msg,
					     CollHeap *heap,
                                             short flags)
{
  return new (maxLen, heap, TRUE)
    IpcMessageBuffer(heap,
                     maxLen,
                     0,
                     msg,
                     flags,
                     GuaInvalidReplyTag,
                     0,    // maxReplyLength
                     -1);  // transid
}

IpcMessageBuffer *IpcMessageBuffer::createBuffer(IpcEnvironment *env,
                                           IpcMessageObjSize newMaxLen,
                                           NABoolean failureIsFatal)
{
  // default is to make a copy of the same length
  if (newMaxLen == 0)
    newMaxLen = maxLength_;

  CollHeap *heap = (env ? env->getHeap() : NULL);

  IpcMessageBuffer *result = new (newMaxLen, heap, failureIsFatal)
    IpcMessageBuffer(heap,
                     newMaxLen,
                     newMaxLen,
                     message_,
                     flags_,
                     replyTag_,
                     maxReplyLength_, 
                     transid_);
  return result;
}

IpcMessageBuffer *IpcMessageBuffer::copy(IpcEnvironment *env,
					 IpcMessageObjSize newMaxLen,
                                         NABoolean failureIsFatal)
{
  // default is to make a copy of the same length
  if (newMaxLen == 0)
    newMaxLen = maxLength_;
  else
    assert(newMaxLen >= msgLength_); // data must fit in new copy

  CollHeap *heap = (env ? env->getHeap() : NULL);

  IpcMessageBuffer *result = new (newMaxLen, heap, failureIsFatal)
    IpcMessageBuffer(heap,
                     newMaxLen,
                     msgLength_,
                     message_,
                     flags_,
                     replyTag_,
                     maxReplyLength_, 
                     transid_);
  
  if (result)
  {
    str_cpy_all((char *) result->data(0),
                (const char *) data(0),
                (Lng32) MINOF(msgLength_,newMaxLen));

    // the copy gets the reply tag, if there is any
    setReplyTag(GuaInvalidReplyTag);
    setMaxReplyLength(0);
  }

  return result;
}

IpcMessageBuffer *IpcMessageBuffer::copyFromOffset(IpcEnvironment *env,
                                               IpcMessageObjSize newMaxLen,
                                               IpcMessageObjSize offset,
                                               NABoolean failureIsFatal)
{
  assert(msgLength_ > offset); // offset must be before the end

  CollHeap *heap = (env ? env->getHeap() : NULL);

  IpcMessageBuffer *result = new (newMaxLen, heap, failureIsFatal)
    IpcMessageBuffer(heap,
                     newMaxLen,
                     (Lng32) MINOF(msgLength_
                                   ,newMaxLen),
                     message_,
                     flags_,
                     replyTag_,
                     maxReplyLength_, 
                     transid_);
  if (result)
  {
    str_cpy_all((char *) result->data(0),
                (const char *) data(offset),
                (Lng32) MINOF(msgLength_,newMaxLen));

    // the copy gets the reply tag, if there is any
    setReplyTag(GuaInvalidReplyTag);
    setMaxReplyLength(0);
  }

  return result;
}

IpcMessageBuffer *IpcMessageBuffer::resize(IpcEnvironment *env,
					   IpcMessageObjSize newMaxLen)
{
  // NOTE: other users of the buffer will retain access to the old buffer
  IpcMessageBuffer *result = copy(env,newMaxLen);

  // this will delete this buffer, unless there are other references to it
  decrRefCount();
  return result;
}

IpcMessageRefCount IpcMessageBuffer::decrRefCount()
{
  IpcMessageRefCount result = refCount_--;
  
  if (refCount_ == 0)
  {
    if (chunkLockCount_)
    {
      chunkLockCount_->deallocate();
      NADELETEBASIC(chunkLockCount_, heap_);
    }

    // The ref count has dropped to zero. Use the correct method to
    // deallocate space for this buffer.
    if (heap_)
    {
      // This object was allocated on an NAMemory heap
      heap_->deallocateMemory(this);
    }
    else
    {
      // This object was allocated on the C++ heap
      delete this;
    }
  }
  else if (refCount_ < 0)
  {
    // negative refcounts aren't allowed
    assert(refCount_ > 0);
  }
  
  return result;
}

CollIndex IpcMessageBuffer::initLockCount(IpcMessageObjSize maxIOSize)
{
  if (!chunkLockCount_)
    {
      chunkLockCount_ = new(heap_) NAArray<CollIndex>(heap_,2);
      chunkLockCount_->setHeap(heap_);

      if (!chunkLockCount_) 
        {
          return(0);
        }
      maxIOSize_ = maxIOSize;
  
      // determine if last chunk is less than the max IO transmission size
      bool partialChunk = msgLength_ && (msgLength_ % maxIOSize_) ? 1 : 0;
      CollIndex chunkCount = partialChunk ? 1 : 0;
      chunkCount += (msgLength_ >= maxIOSize_) ? (msgLength_ / maxIOSize_) : 0;
      for ( CollIndex i = 0; i < chunkCount ; i++ )
        {
          chunkLockCount_->insertAt(i, 0);
        }
      return(chunkCount);
    }
  
  return(chunkLockCount_->entries());
}

CollIndex IpcMessageBuffer::incrLockCount(IpcMessageObjSize offset)
{
  assert(chunkLockCount_ != NULL);
  CollIndex chunkIndex = (CollIndex) (offset ? offset/maxIOSize_ : 0);
  CollIndex lockCount = chunkLockCount_->at(chunkIndex);
  lockCount += 1;
  chunkLockCount_->insertAt(chunkIndex, lockCount);
  return(lockCount);
}

CollIndex IpcMessageBuffer::decrLockCount(IpcMessageObjSize offset)
{
  assert(chunkLockCount_ != NULL);
  CollIndex chunkIndex = (CollIndex) (offset ? offset/maxIOSize_ : 0);
  CollIndex lockCount = chunkLockCount_->at(chunkIndex);
  lockCount -= 1;
  chunkLockCount_->insertAt(chunkIndex, lockCount);
  return(lockCount);
}

CollIndex IpcMessageBuffer::getLockCount(IpcMessageObjSize offset)
{
  assert(chunkLockCount_ != NULL);
  CollIndex chunkIndex = (CollIndex) (offset ? offset/maxIOSize_ : 0);
  CollIndex lockCount = chunkLockCount_->at(chunkIndex);
  return(lockCount);
}

void IpcMessageBuffer::alignOffset(IpcMessageObjSize &offset)
{
  ULng32 offs = (ULng32) offset; // just for safety

  // clear the last 3 bits of the address to round it down to
  // the next address that is divisible by 8
  ULng32 roundedDown = offs LAND 0xFFFFFFF8;

  // if that didn't change anything we're done, the offset was
  // aligned already
  if (offs != roundedDown)
    {
      // else we have to round up and add the filler
      offset = roundedDown + 8;
    }
}

void IpcMessageBuffer::callSendCallback(IpcConnection *conn)
{
  // increment the wraparound counter for completed high-level I/Os
  conn->getEnvironment()->getAllConnections()->bumpCompletionCount();
  // set connection indicating source of IO 
  connection_ = conn;
  // call the callback without passing a message buffer to it
  message_->internalActOnSend(conn);
}

void IpcMessageBuffer::callReceiveCallback(IpcConnection *conn)
{
  // increment the wraparound counter for completed high-level I/Os
  conn->getEnvironment()->getAllConnections()->bumpCompletionCount();
  // set connection indicating source of IO 
  connection_ = conn;
  // call the callback
  message_->internalActOnReceive(this, conn);
}

void * IpcMessageBuffer::operator new(size_t headerSize,
				      IpcMessageObjSize bufferLength,
				      CollHeap *heap,
                                      NABoolean failureIsFatal)
{
  // If heap is not NULL, allocate the object on that heap. Otherwise
  // allocate on the C++ heap.
  if (heap)
    return heap->allocateMemory(headerSize + (size_t) bufferLength,
                                failureIsFatal);
  else
    return ::operator new(headerSize + (size_t) bufferLength);
}

NABoolean IpcMessageBuffer::verifyBackbone()
{
  return verifyIpcMessageBufferBackbone(*this);
}

//
// Global function to verify IpcMessageBuffer backbone. This function
// needs to look at private data inside IpcMessageObj
// instances. Rather than making the entire IpcMessageBuffer class a
// "friend" of IpcMessageObj, we just make this function a friend of
// the IpcMessageObj and IpcMessageBuffer classes.
//
NABoolean verifyIpcMessageBufferBackbone(IpcMessageBuffer &b)
{
  // Make sure buffer is at least as large as the header object
  if (b.msgLength_ < sizeof(InternalMsgHdrInfoStruct))
  {
    ipcIntegrityCheckEpilogue(FALSE);
    return FALSE;
  }

  // Now we can reference fields in the header
  InternalMsgHdrInfoStruct *header = (InternalMsgHdrInfoStruct *) b.data(0);
  IpcMessageObjSize maxChainLen = MINOF(b.maxLength_, header->totalLength_);
  
  // Loop over all objects in the message buffer, including the header
  // object, and do some sanity checks on them. We will look at all
  // length and next fields to make sure the chain of objects does not
  // extend beyond maxChainLen bytes.
  IpcMessageObj *obj = header;
  IpcMessageObjSize currOffset = 0;
  while (obj != NULL)
  {
    // Make sure buffer is at least as large as an IpcMessageObj
    if (currOffset + sizeof(IpcMessageObj) > maxChainLen)
    {
      ipcIntegrityCheckEpilogue(FALSE);
      return FALSE;
    }
    
    // Now we can reference IpcMessageObj fields
    if (obj->s_.refCount_ != 1)
    {
      ipcIntegrityCheckEpilogue(FALSE);
      return FALSE;
    }

    // All objects except the header must have a NULL virtual function
    // pointer. The header may not because the receiving connection
    // may have already done an in-place unpack of the header and that
    // operation has the side-effect of setting the vptr.
    if (obj != header && obj->getMyVPtr() != NULL)
    {
      ipcIntegrityCheckEpilogue(FALSE);
      return FALSE;
    }

    IpcMessageObjSize next = (IpcMessageObjSize)((Long) obj->s_.next_);

    if (obj->s_.objLength_ < sizeof(IpcMessageObj) ||
        currOffset + obj->s_.objLength_ > maxChainLen ||
        (next != 0 && next < obj->s_.objLength_))
    {
      ipcIntegrityCheckEpilogue(FALSE);
      return FALSE;
    }
    
    // Advance to next object in the chain
    currOffset += next;
    obj = obj->getNextFromOffset();
  }

  return TRUE;
}

// -----------------------------------------------------------------------
// Methods for class IpcMessageStreamBase
// -----------------------------------------------------------------------
IpcMessageStream * IpcMessageStreamBase::castToIpcMessageStream()
{
  return NULL;
}

IpcBufferedMsgStream *
IpcMessageStreamBase::castToIpcBufferedMsgStream()
{
  return NULL;
}

void IpcMessageStreamBase::addToCompletedList()
{    
  environment_->addToCompletedMessages(this);
}

// -----------------------------------------------------------------------
// Methods for class IpcMessageStream
// -----------------------------------------------------------------------

IpcMessageStream::IpcMessageStream(
     IpcEnvironment *env,
     IpcMessageObjType msgType,
     IpcMessageObjVersion version,
     IpcMessageObjSize fixedMsgBufferLength,
     NABoolean shareMessageObjects) :
        IpcMessageStreamBase(env),
        h_(msgType,version),
	recipients_(env->getAllConnections(),env->getHeap()),
	activeIOs_(env->getAllConnections(),env->getHeap())
{
#ifndef USE_SB_NEW_RI
  assert(fixedMsgBufferLength <= IOSIZEMAX);
#else
  assert(fixedMsgBufferLength <= env->getGuaMaxMsgIOSize());
#endif

  msgBuffer_       = NULL;
  fixedBufLen_     = fixedMsgBufferLength;
  maxReplyLength_  = 0;
  shareObjects_    = shareMessageObjects;
  objectsInBuffer_ = FALSE;
  state_           = EMPTY;
  tail_            = first();
  current_         = NULL;
  errorInfo_       = 0;
  numOfSendCallbacks_ = 0;
  corruptMessage_  = false;
  isOrphaned_      = FALSE;
}

IpcMessageStream::~IpcMessageStream()
{
  // the destructor for the message header, which will
  // perform a check of the reference count, is called implicitly

  // deallocate the existing message buffer (despite the name of the proc)
  allocateMessageBuffer(0);
}

IpcMessageStream *IpcMessageStream::castToIpcMessageStream()
{
  return this;
}

IpcMessageStream & IpcMessageStream::operator << (IpcMessageObj & toAppend)
{
#ifdef IPC_INTEGRITY_CHECKING
//  checkIntegrity();  message may be legitimately orphaned here
#endif

  // check message state, this is the process of composing the message
  assert(state_ == EMPTY OR state_ == COMPOSING);
  state_ = COMPOSING;

  if (shareObjects_)
    {
      // if we add a new object to the message, it ought not be in use
      // by some other message (only the owner can have a refcount)
      // (this may be extended later)
      assert(toAppend.s_.next_ == NULL AND toAppend.s_.refCount_ == 1);

      // increment the reference count of the appended shared object
      toAppend.incrRefCount();

      // add the object to the linked list of IpcMessageObj objects hanging
      // off the IpcMessageStream, don't pack the object into the
      // message buffer yet!!
      tail_->s_.next_ = &toAppend;
      tail_ = &toAppend;

    }
  else
    {
      // copy the object into the message buffer without altering its
      // reference count (toAppend can go away after this method returns)

      // size of toAppend when packed into the message buffer
      IpcMessageObjSize thisObjSize = toAppend.packedLength();

      // start offset of the packed version of toAppend in the message buffer
      IpcMessageObjSize startOffset;

      // we need a message buffer of at least this size
      IpcMessageObjSize neededMsgSize;

      // number of bytes used during the pack process
      IpcMessageObjSize packedBytes;

      // a pointer to the packed object cast as an IpcMessageObj
      IpcMessageObj     *bufObj;

      // we know that the object's header uses at least some space
      assert(thisObjSize >= sizeof(IpcMessageObj));
      
      // check whether there are previous objects in the buffer
      if (tail_ == first())
	{
	  // no, this is the first object to be added to the message
	  // (after the header, which gets packed during send(), but
	  // whose space gets allocated right here)
	  h_.totalLength_ = h_.packedLength();
	  IpcMessageBuffer::alignOffset(h_.totalLength_);
	  startOffset = h_.totalLength_;
	}
      else
	{
	  startOffset = h_.totalLength_;
	}

      neededMsgSize = startOffset + thisObjSize;

      // check whether we need to allocate a new or bigger message buffer
      if (msgBuffer_ == NULL OR
	  msgBuffer_->getBufferLength() < neededMsgSize)
	{
	  // current buffer is too small, make room and maybe add a little
	  // reserve so this doesn't happen again next time

	  if (fixedBufLen_ > 0)
	    neededMsgSize = MAXOF(neededMsgSize,fixedBufLen_);
	  else
	    neededMsgSize = MAXOF(neededMsgSize,DefaultInitialMessageBufSize);

	  resizeMessageBuffer(neededMsgSize);
	}

      // now pack the object into the message buffer

      packedBytes = toAppend.packObjIntoMessage(msgBuffer_->data(startOffset));
      bufObj = (IpcMessageObj *) (msgBuffer_->data(startOffset));
      
      // Do some sanity checks, did the object really use the length it
      // promised to use? Did the length field in the message get set
      // correctly?
      assert(thisObjSize == packedBytes);
      assert(bufObj->s_.objLength_ == packedBytes);

      // advance the buffer pointer to the next place where we could
      // put an object (the next object wants to sit on an 8 byte boundary)
      // NOTE: we don't increase bufObj->s_.objLength_ when we align.
      h_.totalLength_ += packedBytes;
      IpcMessageBuffer::alignOffset(h_.totalLength_);

      // prepare the packed object to be shipped off and set the link from
      // the previous object
      bufObj->setMyVPtr(NULL);
      bufObj->s_.refCount_ = 1;
      bufObj->s_.next_ = NULL;
      tail_->s_.next_ = (IpcMessageObj *) ((long)startOffset);
      tail_ = bufObj;
    }
  return *this;
}

NABoolean IpcMessageStream::extractNextObj(IpcMessageObj &toRetrieve,
                                           NABoolean checkObjects)
{
#ifdef IPC_INTEGRITY_CHECKING
//  checkIntegrity();  message may be legitimately orphaned here
#endif

  // check whether it is ok to extract data now
  assert(state_ == RECEIVED OR state_ == EXTRACTING);
  state_ = EXTRACTING;

  // we assume that the user does something like the following to ensure
  // that "toRetrieve" is actually an object of the correct (derived) class:
  //
  //    IpcMessageStream m;
  //    ...
  //    m.receive();
  //    while (m.moreObjects())
  //      {
  //        IpcMessageObjType t;
  //
  //        t = m.getNextObjType();
  //        // if different versions and lengths are possible, the user
  //        // would have to retrieve those as well
  //        switch (t)
  //          {
  //          case xyz: // case for object type of class MyOwnMessageObject
  //            {
  //              MyOwnMessageObject myOwnObj;
  //
  //              // myOwnObj should have the correct type and version
  //              // (if not automatically set by constructor, do it manually)
  //              m >> myOwnObj;
  //              ...
  //            }
  //          }
  //      }
  //

  // the user should have predicted the object type and version correctly
  assert(current_ AND
         toRetrieve.s_.objType_ == current_->s_.objType_ AND
         toRetrieve.isActualVersionOK(current_->s_.objVersion_));

  // current_ points to the next object to be retrieved
  IpcMessageObj *packedObject = current_;
  
  // need to turn around bytes if this object is in the other endianness
  if (h_.getEndianness() != IpcMyEndianness)
    packedObject->turnByteOrder();
  
  // The object pointed to by <current> will get copied onto <toRetrieve>.
  // To avoid trouble, set the refcount of <current> to the refcount of
  // toRetrieve while unpacking the object. This way, even if the user's
  // unpackObj function simply copies the refcount, we still haven't
  // destroyed it. Also, this does not alter <toRetrieve>'s refcount
  // without the user seeing it. Copying a new content onto <toRetrieve>
  // is transparent to those people who have a pointer to it, therefore
  // it shouldn't change it's refcount.
  IpcMessageRefCount saveRefCount = packedObject->s_.refCount_;
  
  // advance the current object pointer
  // (check whether the object uses an offset or a real pointer)
  if (objectsInBuffer_)
    current_ = current_->getNextFromOffset();
  else
    current_ = current_->s_.next_;
  
  // for now, all refcounts of objects in messages should be 1
  assert(saveRefCount == 1);
  
  packedObject->s_.refCount_ = toRetrieve.s_.refCount_;

  // If the caller requested an integrity check on the packed object
  // then we call a checkObj() method before unpackObj().
  NABoolean result = TRUE;
  if (checkObjects)
  {
    result = toRetrieve.checkObj(packedObject->s_.objType_,
                                 packedObject->s_.objVersion_,
                                 h_.getEndianness() == IpcMyEndianness,
                                 packedObject->s_.objLength_,
                                 (IpcConstMessageBufferPtr) packedObject);
  }
  
  // now call the user-defined unpack function
  // NOTE: this may call this method recursively!!!
  if (result)
  {
    toRetrieve.unpackObj(packedObject->s_.objType_,
                         packedObject->s_.objVersion_,
                         h_.getEndianness() == IpcMyEndianness,
                         packedObject->s_.objLength_,
                         (IpcConstMessageBufferPtr) packedObject);
  }
  
  // restore the refcount in the message
  packedObject->s_.refCount_ = saveRefCount;
  
  return result;
}

IpcMessageStream & IpcMessageStream::operator >> (IpcMessageObj * &toRetrieve)
{
#ifdef IPC_INTEGRITY_CHECKING
//  checkIntegrity(); message may be legitimately orphaned here
#endif

  // check whether it is ok to extract data now
  assert(state_ == RECEIVED OR state_ == EXTRACTING);
  state_ = EXTRACTING;

  // current_ points to the next object to be retrieved
  if (current_ != NULL)
    {
      // return the pointer to the current object
      toRetrieve = current_;

      // indicate that the current object is now also referenced
      // by the caller of this method (caller has to release the
      // object later)
      toRetrieve->incrRefCount();

      // advance the current object pointer
      // (check whether the object uses an offset or a real pointer)
      if (objectsInBuffer_)
	current_ = current_->getNextFromOffset();
      else
	current_ = current_->s_.next_;
    }
  else
    {
      // error, no more objects are available
      toRetrieve = NULL;
    }

  return *this;
}

void IpcMessageStream::clearAllObjects()
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  IpcMessageObj *obj;
  IpcMessageObj *next;

  assert(state_ != SENDING AND state_ != RECEIVING);
  assert(activeIOs_.entries() == 0);

  if (objectsInBuffer_)
    {
      clearMessageBufferContents();
    }
  else
    {
      // if the message contains a linked list of shared objects then
      // release all objects except the header which is hardwired in
      // and unlink the objects from the list as you go
      obj = first()->s_.next_;
      first()->s_.next_ = NULL;
      while (obj != NULL)
	{
	  next = obj->s_.next_;
	  obj->s_.next_ = NULL;
	  obj->decrRefCount();
	  obj = next;
	}
    }

  objectsInBuffer_ = FALSE;
  current_         = NULL;
  tail_            = first();

  state_ = EMPTY;
}

void IpcMessageStream::addRecipient(IpcConnection *recipient)
{
#ifdef IPC_INTEGRITY_CHECKING
//  checkIntegrity();  don't check here since msg. will be legitimately orphaned
#endif

  recipients_ += recipient->getId();

#ifdef IPC_INTEGRITY_CHECKING
//  checkIntegrity(); don't check here since msg. will be legitimately orphaned
#endif
}

void IpcMessageStream::addRecipients(const IpcSetOfConnections &/*recipients*/)
{
  ABORT("not implemented yet");
  // recipients_ += recipients;
}

void IpcMessageStream::deleteRecipient(IpcConnection *recipient)
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  recipients_ -= recipient->getId();

#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif
}

void IpcMessageStream::deleteAllRecipients()
{
  recipients_.clear();
}

void IpcMessageStream::giveMessageTo(IpcMessageStream &other,
				     IpcConnection *connection)
{
  // ---------------------------------------------------------------------
  // This method takes another message stream, implicitly performs a
  // receive() call on it, connects it to the connection, and passes
  // the current message of "this" message stream on to the "other"
  // message stream, as if it had been received by the connection.
  // IpcMessageStream::giveMessageTo() can therefore be used by
  // "router" message streams that listen to a connection that is shared
  // among multiple message streams, check the contents of the message,
  // and then dispatch the message to the appropriate message stream.
  // Only the "router" message stream should call receive() explicitly.
  // ---------------------------------------------------------------------
  assert(connection);

  // the other message must be in a state that allows receiving
  assert(other.getState() == EMPTY OR
	 other.getState() == SENT);

  // get the message buffer that we want to give to the other message stream
  IpcMessageBuffer *bufferToMove = msgBuffer_;

  // detach myself from the message buffer (NOTE: somebody may have already
  // looked at the message contents, so my pointers may point to the
  // message buffer)
  msgBuffer_       = NULL;
  objectsInBuffer_ = FALSE;
  current_         = NULL;
  tail_            = first();

  // this is now an empty message, ready for either send or receive
  state_ = EMPTY;

  // set the other's recipients and active IOs to the connection
  IpcConnectionId id = connection->getId();
  other.recipients_ += id;
  other.activeIOs_ += id;

  // call the callback function for the other message stream,
  // just as a connection would do
  bufferToMove->addCallback(&other);
  bufferToMove->callReceiveCallback(connection);
}


///////////////////////////////////////////////////////////////////////////////
// give receive message to class IpcBufferedMsgStream
void IpcMessageStream::giveReceiveMsgTo(IpcBufferedMsgStream& msgStream)
  {
  msgStream.addInputBuffer(msgBuffer_);
    
  msgBuffer_       = NULL;
  objectsInBuffer_ = FALSE;
  current_         = NULL;
  tail_            = first();
  state_ = EMPTY;
  
  msgStream.actOnReceive(NULL);   // trigger receiving message stream
  }

// Uncomment the next line to trace IpcMessageObj.  Works better if LOG_IPC
// is also defined in IpcGuardian.cpp
// #define LOG_IPC_MSG_OBJ

void IpcMessageStream::send(NABoolean waited, Int64 transid)
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  MXTRC_FUNC("IpcMessageStream::send");
  IpcMessageObjSize   thisObjLength;
  IpcMessageObj *obj;

  // check message state
  assert(state_ == COMPOSING);

  if (shareObjects_)
    {
      // -----------------------------------------------------------------
      // Objects are shared between caller and IPC layer. Therefore they
      // were not copied by operator <<. Now that we know all of them,
      // copy them into the message buffer.
      // -----------------------------------------------------------------

      // -----------------------------------------------------------------
      // Compute the total length of the message.
      // ---------------------------------------------------------------------

      // loop through all objects and ask them how much space they need

      h_.totalLength_ = 0;
      obj = first();
      while (obj != NULL)
	{
	  // ask the object how long it will be when packed into the buffer
	  thisObjLength = obj->packedLength();
	  
	  // we know that the object's header uses at least some space
	  assert(thisObjLength >= sizeof(IpcMessageObj));
	  
	  // store that info in the object's length field (we check later
	  // when the packing is done whether the method lied or not)
	  obj->s_.objLength_ = thisObjLength;
	  
	  // find the offset of the next potential object by aligning the
	  // address and add the needed filler space to the object length
	  // NOTE: we even align the end of the message
	  h_.totalLength_ += thisObjLength;
	  IpcMessageBuffer::alignOffset(h_.totalLength_);
	  
	  // next, please
	  obj = obj->s_.next_;
	}

      // -----------------------------------------------------------------
      // allocate an empty message buffer, if the user specified a fixed
      // length buffer size then use that (allows replies via REPLYX to
      // have maxBufferLen_ bytes without switching to the multi-chunk
      // protocol)
      // -----------------------------------------------------------------
      allocateMessageBuffer(MAXOF(h_.totalLength_,fixedBufLen_));

      // -----------------------------------------------------------------
      // pack the message object into a buffer
      // -----------------------------------------------------------------
      IpcMessageObjSize msgDataLen = 0;
      IpcMessageObj *bufObj = (IpcMessageObj *) msgBuffer_->data();
      IpcMessageObj *nextBufObj = NULL;

      obj = first();
      while (obj != NULL)
	{
#ifdef LOG_IPC_MSG_OBJ
          cerr << obj->getType() << ", ";
#endif          
	  MXTRC_1("type=%d\n", obj->getType());
	  // let the user code pack the actual object into the buffer
	  thisObjLength = obj->packObjIntoMessage(
	       (IpcMessageBufferPtr) bufObj);
	  msgDataLen += thisObjLength;
	  
	  // now massage some of the header fields of the object in
	  // the buffer:
	  //
	  // - make sure the object length is the one we previously
	  //   calculated by calling obj->packedLength();
	  // - make sure we correctly set the obj. length in the message
	  // - wipe out the virtual function pointer
	  // - set the refcount of the object in the message to 1
	  // - find the end of the object by aligning the end pointer
	  // - set the next_ pointer to be a relative offset to the
	  //   next message
	  //
	  assert(obj->s_.objLength_ == thisObjLength);
#ifdef NA_BIG_ENDIAN
      // NOTE: byte length *may* have been converted to big-endian format
	  //       which means that s_.objLength_ is in the wrong format
      assert(obj->s_.objLength_ == bufObj->s_.objLength_);
#endif
	  bufObj->setMyVPtr(NULL);
	  bufObj->s_.refCount_ = 1;

	  // NOTE: see also IpcMessageObj::packDependentObjIntoMessage()
	  // when making changes to the above code

	  // advance the buffer pointer to the next place where we could
	  // put an object (the next object wants to sit on an 8 byte boundary)
	  // NOTE: we don't increase bufObj->s_.objLength_ when we align.
	  IpcMessageBuffer::alignOffset(msgDataLen);

	  // take care of the object's next_ pointer that is now embedded
	  // in the message buffer
	  if (obj->s_.next_ != NULL)
	    {
	      nextBufObj = (IpcMessageObj *) msgBuffer_->data(msgDataLen);
	      // store the place where the next object will be located
	      bufObj->s_.next_ = nextBufObj;
	      // now convert it to an offset relative to the start of obj
	      bufObj->convertNextToOffset();
	    }
	  else
	    {
	      nextBufObj = NULL;
	    }

	  // dissolve the linked list that hooked the objects together
	  IpcMessageObj *next = obj->s_.next_;
	  obj->s_.next_ = NULL;

	  // the object got copied into the message, decrement its
	  // reference count except for the first object, which is
	  // embedded in the message object
	  if (obj != first())
	    obj->decrRefCount();
 
          // Allow testing of corrupt message handling on the other side.
	  if (corruptMessage_ && (next == NULL))
            bufObj->s_.refCount_ = 666;

          // advance to the next object
	  obj = next;
	  bufObj = nextBufObj;
	}
  
      assert(h_.totalLength_ == msgDataLen);
      msgBuffer_->setMessageLength(msgDataLen);

#ifdef LOG_IPC_MSG_OBJ
      if (msgDataLen)   // i.e., if any objs
        cerr << endl;
#endif

    } // shared objects
  else
    {
      // -----------------------------------------------------------------
      // Objects were not shared between caller and IPC layer and are
      // therefore already copied into the message buffer.
      // -----------------------------------------------------------------

      // the header is not packed into the buffer yet but there is
      // space allocated for it at the beginning of the buffer
      first()->packObjIntoMessage(msgBuffer_->data());
      msgBuffer_->setMessageLength(h_.totalLength_);
      msgBuffer_->addCallback(this);
    } // objects are not shared

  // ---------------------------------------------------------------------
  // now send the message buffer to each of the connections specified
  // setup transid in message buffer.
  // ---------------------------------------------------------------------
  state_ = SENDING;

  msgBuffer_->setTransid (transid);

  // can't have a previous operation uncompleted, since that would
  // confuse the activeIOs_ set.
  assert(activeIOs_.entries() == 0);
  assert(numOfSendCallbacks_ == 0);

  // Determine the total number of IOs first, since some of the sends may
  // complete immediately and we don't want a situation where a callback
  // is called with no outstanding IOs while we aren't done yet with
  // our send loop below. Also cache the pointer to the message buffer,
  // since it may be modified by callbacks that initiate a receive operation.
  IpcSetOfConnections sendConnections(recipients_);
  IpcMessageBuffer *msgBuffer = msgBuffer_;
  CollIndex numRecipients = sendConnections.entries();

  activeIOs_ += sendConnections;
  msgBuffer_ = NULL;

  // By calling IpcConnection::send() we give up the message buffer.
  // So, in order to do multiple sends, get enough ref counts before
  // starting - i.e., one extra ref count for every connection beyond the first
  // one. Note that this helps the connections to find out whether
  // they have exclusive access to the message buffer or not. If we would
  // let each connection increment its own refcount this wouldn't work.
  for (CollIndex extraRecipients = 1;
       extraRecipients < numRecipients;
       extraRecipients++)
    msgBuffer->incrRefCount();

  // ---------------------------------------------------------------------
  // The actual IpcConnection::send() call(s)
  // ---------------------------------------------------------------------
  for (IpcConnectionId i = 0; sendConnections.setToNext(i); i++)
    sendConnections.element(i)->send(msgBuffer);

  // if the send is in wait mode, then we must wait until the send completes
  // on all connections. if the send is no-wait, then don't wait here but
  // simply return, and the caller shall issue wait on all connections.
  // otherwise we could see the stack piles up quickly due to recursive
  // send/receive calls.
  if (waited)
    {
      NABoolean interruptRecvd = waitOnMsgStream(IpcInfiniteTimeout);
      if (interruptRecvd)
        state_ = BREAK_RECEIVED;
    }

#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif
}

void IpcMessageStream::receive(NABoolean waited)
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  MXTRC_FUNC("IpcMessageStream::receive");

  // check state
  assert(state_ == EMPTY OR state_ == SENT
         OR state_ == ERROR_STATE);  // After error, succesfully sent messages 
                               // must have their receive callback registered.
                               // See code in ExMasterEspMessage::actOnSend
                               // which ensures that recipients_ contains only
                               // connections of successfully sent messages
                               // when this method is called.

  // reset errors
  errorInfo_ = 0;

  // for now, can't call receive when there are I/Os outstanding
  assert(activeIOs_.entries() == 0);

  // ---------------------------------------------------------------------
  // initiate a receive operation for connection <from> or for all
  // associated connections if <from> is NULL
  // ---------------------------------------------------------------------

  // determine outstanding IOs first, since some of the receives may
  // complete immediately and we don't want a situation where a callback
  // is called with no outstanding IOs while we aren't done yet with
  // our for loop below
  IpcSetOfConnections recConnections(recipients_);
  activeIOs_ = recConnections;

  for (IpcConnectionId i = 0; recConnections.setToNext(i); i++)
    {
      state_ = RECEIVING;

      // Be careful: some I/Os may complete immediately which may change
      // the state of this message stream!!!
      // NOTE: the receive callbacks are not supposed to send any data
      // until the last receive has completed, since this would otherwise
      // interfere with this loop.
      recConnections.element(i)->receive(this);
    }

  // if the receive is in wait mode, then we must wait until the receive
  // completes on all connections. if the receive is no-wait, then don't wait
  // here but simply return, and the caller shall issue wait on all
  // connections. otherwise we could see the stack piles up quickly due to
  // recursive send/receive calls.
  if (waited)
    {
      NABoolean interruptRecvd = waitOnMsgStream(IpcInfiniteTimeout);
      if (interruptRecvd)
        state_ = BREAK_RECEIVED;
    }

#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif
}

WaitReturnStatus IpcMessageStream::waitOnMsgStream(IpcTimeout timeout)
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  NABoolean interruptRecvd = FALSE;

  interruptRecvd = activeIOs_.waitOnSet(timeout);
 MXTRC_FUNC("IpcMessageStream::wait");

  if (interruptRecvd)
  {
     state_ = BREAK_RECEIVED; 
     return WAIT_INTERRUPT;
  }

  if (timeout == IpcInfiniteTimeout)
    {
      // This means the user wants to wait until all messages in this
      // stream have been sent or have been received, so loop until
      // outstandingIOs_ is empty.
      while (activeIOs_.entries() > 0 &&
	     activeIOs_.moreWaitsAnyConnection())
      {
	interruptRecvd = activeIOs_.waitOnSet(timeout);
        if (interruptRecvd)
        {
           state_ = BREAK_RECEIVED;
           return WAIT_INTERRUPT;
        }
      }
    }

#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif
  return WAIT_OK;
}
 
void IpcMessageStream::actOnSend(IpcConnection *)
{
  // the default callback implementation does nothing
}

void IpcMessageStream::actOnSendAllComplete()
{
  // the default callback implementation does nothing
}

void IpcMessageStream::actOnReceive(IpcConnection *)
{
  // the default callback implementation does nothing
}

void IpcMessageStream::actOnReceiveAllComplete()
{
  // the default callback implementation does nothing
}

void IpcMessageStream::clearMessageBufferContents()
{
  // if a message buffer exists and has data in it,
  // it must contain objects with a reference count of 1 only
  // (the "1" count is the use of the object in the message buffer
  // and means that we can delete the buffer)
  if (objectsInBuffer_)
    {
      IpcMessageObj *obj = first()->s_.next_;

      first()->s_.next_ = NULL;
      while (obj != NULL)
	{
	  assert(obj->getRefCount() == 1);
	  // objects in the message buffer are always using offsets
	  obj = obj->getNextFromOffset();
	}

      // give up on the objects in the message buffer
      objectsInBuffer_ = FALSE;
      tail_            = first();
      current_         = NULL;
      h_.totalLength_  = 0;
    }
}

void IpcMessageStream::allocateMessageBuffer(IpcMessageObjSize len)
{
  IpcMessageBuffer *toDelete = NULL;

  // do we need to delete an old or unusable message buffer
  if (msgBuffer_ != NULL AND
      (len == 0 OR len > msgBuffer_->getBufferLength()))
    {
      clearMessageBufferContents();
      toDelete   = msgBuffer_;
      msgBuffer_ = NULL;
    }

  if (msgBuffer_ == NULL AND len > 0)
    {
      CollHeap *heap = (environment_ ? environment_->getHeap() : NULL);

      // we need to allocate a new message buffer
      msgBuffer_ = IpcMessageBuffer::allocate(len,
					      this,
					      heap,
                                              0);
      if (toDelete != NULL)
	{
	  // move reply tag and max reply len from the old one
	  msgBuffer_->setReplyTag(toDelete->getReplyTag());
	  msgBuffer_->setMaxReplyLength(toDelete->getMaxReplyLength());
	}
    }
  else if (msgBuffer_ != NULL)
    {
      // reuse same message buffer, but change the callback
      msgBuffer_->addCallback(this);
      msgBuffer_->setMessageLength(0);
    }

  if (toDelete != NULL)
    toDelete->decrRefCount();
}

void IpcMessageStream::resizeMessageBuffer(IpcMessageObjSize newMaxLen)
{
  // make sure we have a message buffer of the desired length, but
  // don't mess with the contents
  if (msgBuffer_ != NULL)
    msgBuffer_ = msgBuffer_->resize(environment_,newMaxLen);
  else
    allocateMessageBuffer(newMaxLen);
}

void IpcMessageStream::internalActOnSend(IpcConnection *connection)
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  assert(activeIOs_.contains(connection->getId()));

  // stream can only remember the first reported error from connection
  if (state_ != ERROR_STATE && connection->getErrorInfo() != 0)
    {
      state_ = IpcMessageStream::ERROR_STATE;
      errorInfo_ = connection->getErrorInfo();
    }

  actOnSend(connection);

  numOfSendCallbacks_++;
  if (numOfSendCallbacks_ == activeIOs_.entries())
    {
      state_ = SENT;

      activeIOs_.clear();
      numOfSendCallbacks_ = 0;

      actOnSendAllComplete();
    }

#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif
}

void IpcMessageStream::internalActOnReceive(IpcMessageBuffer *buffer,
					    IpcConnection *connection)
{
#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif

  // deallocate the existing message buffer (despite the name of the proc)
  allocateMessageBuffer(0);

  assert(buffer);
  msgBuffer_ = buffer;

  assert(connection);
  IpcConnectionId id = connection->getId();
  assert(activeIOs_.contains(id));
  activeIOs_ -= id;

  if (connection->getErrorInfo() != 0)
    {
      // stream can only remember the first reported error from connection
      if (state_ != ERROR_STATE)
        {
          if (connection->breakReceived())
            state_ = BREAK_RECEIVED;
          else 
            state_ = ERROR_STATE;
          errorInfo_ = connection->getErrorInfo();
        }
    }
  else
    {
      // Set the state to RECEIVED. Even if there are more active IOs,
      // allow the user to retrieve results. Be careful at the next state
      // change: set it to EMPTY if no more receives are expected, set it back
      // to RECEIVING if there are more outstanding IOs.
      state_ = RECEIVED;

      // unpack the message header right away
      InternalMsgHdrInfoStruct *justReceived =
        (InternalMsgHdrInfoStruct *) msgBuffer_->data();

      // make sure the OS gave us a message with the correct length
      if ((msgBuffer_->getMessageLength() >= 
          sizeof(InternalMsgHdrInfoStruct)) &&
          (msgBuffer_->getMessageLength() == 
           justReceived->totalLength_))
        {
          h_.unpackObj(justReceived->getType(),
                   justReceived->getVersion(),
                   TRUE, // we've already taken care of endianness
                   justReceived->s_.objLength_,
                   msgBuffer_->data());
        }
      else 
        {
          connection->dumpAndStopOtherEnd(true, false);
          assert(msgBuffer_->getMessageLength() >= 
                 sizeof(InternalMsgHdrInfoStruct) AND
                 msgBuffer_->getMessageLength() == 
                 justReceived->totalLength_);
        }

      // for now...
      if ((h_.alignment_ != IpcMyAlignment) ||
          (h_.getEndianness() != IpcMyEndianness))
        connection->dumpAndStopOtherEnd(true, false);

      assert(h_.alignment_ == IpcMyAlignment AND
             h_.getEndianness() == IpcMyEndianness);

      // the next pointer in the object's copy of the header is an
      // actual pointer
      h_.s_.next_ = justReceived->getNextFromOffset();

      // indicate that we now have a linked list of objects in
      // the message that are all stored in the message buffer and
      // that all use offsets rather than pointers
      // (Exception: a copy of the first object is outside the buffer and it
      // uses a real pointer to the next object)
      objectsInBuffer_ = TRUE;

      // check refcount that came in the buffer
      assert(h_.s_.refCount_ == 1);
  
      IpcMessageObj *obj = first();
  
      obj = obj->s_.next_;
  
      current_ = obj; // the first non-header object in the message
      tail_ = NULL;   // don't use tail in received messages
  
      // loop over all objects in the message buffer and do
      // some sanity checks on them
      while (obj != NULL)
        {
          bool badMessage = false;
          if (obj->getRefCount() != 1)
            badMessage = true;
          if ((obj->s_.next_ != NULL) &&
	      ((ULong)obj->s_.next_ < obj->s_.objLength_))
            badMessage = true;
          if (obj->getMyVPtr() != NULL)
            badMessage = true;

          if (badMessage)
            connection->reportBadMessage();

          assert(obj->getRefCount() == 1);

          assert(obj->s_.next_ == NULL OR
	          (IpcMessageObjSize )((Long) obj->s_.next_) >= obj->s_.objLength_);

          assert(obj->getMyVPtr() == NULL);
      
          obj = obj->getNextFromOffset();
        }
    }

  // call user's callback
  actOnReceive(connection);

  if (activeIOs_.entries() == 0)
    actOnReceiveAllComplete();

#ifdef IPC_INTEGRITY_CHECKING
  checkIntegrity();
#endif
}

ExMasterEspMessage * IpcMessageStream::castToExMasterEspMessage(void)
  {
  // virtual method giving safe cast to ExMasterEspMessage
  return NULL;
  }

// abort any outstanding I/Os on this stream
void IpcMessageStream::abandonPendingIOs()
  {
    // Part of the fix for soln 10-070108-1544.  The statement that 
    // owns this stream has encountered a fatal error and is prone
    // to deadlock if Statement::releaseTransaction waits forever 
    // for the release trans/work message to complete.  
    // One problem with this fix is that it puts the IpcConnections
    // into the error state, so that the ESPs will be stopped.  And 
    // if multiple statements share the same IpcConnection then the 
    // second statement can inherit the error.

    //
    // soln 10-080625-4107. below is an example:
    // the message stream contains the fixup msg from master to 4 esps. all 4
    // esps have gone away. the fixup msg is multi-chunk (more than 4 chunks
    // in my test). 2 connections (GMCS) finished send successfully, without
    // receiving the ipc error yet due to the timing of the subsequent nowait
    // calls. the send call backs were called on those 2 connections and as a
    // result they were removed from stream's activeIOs_ list.
    //
    // the other 2 connections are still in the middle of multi-chunk send
    // and thus send callbacks have not been invoked on them. note in this
    // case, the stream has only two active I/Os but still has 4 recipient
    // connections.
    //
    // now we come here because the upper layer found dead esps and decides
    // to clean up all I/Os on stream (see Statement::releaseTransaction()).
    // the for loop below iterates thru stream's activeIOs_ list and invokes
    // setFatalError() on the latter 2 connections. as a result all I/Os on
    // those 2 connections will be cleared and they will be removed from the
    // stream's activeIOs_ list. however, because send never finished on those
    // 2 connections, we still need to invoke send callbacks on them during
    // cleanup process (see GuaMsgConnectionToServer::handleIOErrorForEntry()).
    // after we invoke the send callback on the last active connection, there
    // will be no active I/O on the stream and IpcMessageStream::receive()
    // will be invoked (see ExMasterEspMessage::actOnSend()). the receive
    // will add the 2 former connections, who finished send successfully, back
    // to stream's activeIOs_ list. so after the first invocation of the for
    // loop, the stream still has 2 recipient connections and 2 active I/Os.
    //
    // because of the above use case, we must go thru the for loop again and
    // invoke setFatalError() on the 2 former connections that were inactive
    // before the for loop but became active after the first execution of the
    // for loop. therefore I'm adding a while loop outside the for loop to
    // make sure all I/Os on the stream are cleaned up properly.
    //
    while (hasIOPending())
      {
        for (CollIndex i = 0; activeIOs_.setToNext(i); i++)
          {
            IpcConnection * c = activeIOs_.element(i);
            NABoolean useGuaIpc = TRUE;
            if (useGuaIpc)
              {
                if (c->castToGuaConnectionToServer())
                  c->castToGuaConnectionToServer()->setFatalError(this);
              }
          } // for
      } // while
  }

#ifdef IPC_INTEGRITY_CHECKING

// methods that perform integrity checking on Ipc-related data structures

void IpcMessageStream::checkIntegrity(void)
  {
  isOrphaned_ = TRUE;  // assume the worst: this object is an orphan
  environment_->checkIntegrity(); // traverse up to IpcEnvironment, do check
  if ((isOrphaned_) && (castToExMasterEspMessage()))
    {
    cerr << "Found orphaned ExMasterEspMessage object " << (void *)this << "." <<
      endl;
    checkLocalIntegrity();  // didn't get here; check our integrity now
    }
  }

void IpcMessageStream::checkLocalIntegrity(void)
  {
  isOrphaned_ = FALSE;
  activeIOs_.checkLocalIntegrity();
  }

#endif

// ----------------------------------------------------------------------------
// IpcBufferedMsgStream
// ----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// constructor
IpcBufferedMsgStream::IpcBufferedMsgStream(IpcEnvironment *env,
                                           IpcMessageType msgType,
                                           IpcMessageObjVersion version,
                                           Lng32 inUseBufferLimit,
                                           IpcMessageObjSize bufferSize)
  : IpcMessageStreamBase(env),
    msgType_(msgType), 
    msgVersion_(version), 
    bufferSize_(bufferSize),
    inUseBufferLimit_(inUseBufferLimit),
    garbageCollectLimit_(0),
    errorInfo_(0),
    receiveMsgComplete_(FALSE),
    sendMsgBuf_(NULL),
    sendMsgHdr_(NULL),
    sendMsgObj_(NULL),
    receiveMsgBufI_(0),
    receiveMsgBuf_(NULL),
    receiveMsgHdr_(NULL),
    receiveMsgObj_(NULL),
    sendBufList_(env->getHeap()),
    receiveBufList_(env->getHeap()),
    inBufList_(env->getHeap()),
    outBufList_(env->getHeap()),
    inUseBufList_(env->getHeap()),
    replyTagBufList_(env->getHeap()),
    smContinueProtocol_(FALSE)
{
}

IpcBufferedMsgStream::~IpcBufferedMsgStream()
{
  releaseBuffers();

  IpcMessageBuffer *msgBuf = NULL;
  CollIndex i = 0;

  for (i = 0; i < inBufList_.entries(); i++)
  {
    msgBuf = inBufList_[i];
    msgBuf->decrRefCount();
  }

  for (i = 0; i < receiveBufList_.entries(); i++)
  {
    msgBuf = receiveBufList_[i];
    msgBuf->decrRefCount();
  }
}

IpcBufferedMsgStream *
IpcBufferedMsgStream::castToIpcBufferedMsgStream()
{
  return this;
}

///////////////////////////////////////////////////////////////////////////////
// get next receive message from input queue.
NABoolean IpcBufferedMsgStream::getNextReceiveMsg(IpcMessageObjType& msgType)
  {
  if (receiveMsgComplete_)
    {
    if (receiveMsgBufI_ < receiveBufList_.entries())
      { // current receive message not completely unpacked, return type

      msgType = receiveMsgHdr_->getType();
      return (TRUE);
      }
    // release current receive message to inuse pool and advance to next.
    // If a user msg object was unpacked inplace and did not implement the
    // virtual method msgObjIsFree() then its persistence will be guaranteed
    // until now!
    IpcMessageBuffer* msgBuf;
    while (receiveBufList_.getFirst(msgBuf))
      { // move all buffers for current receive message to inuse pool
      inUseBufList_.insert(msgBuf);
      }
    receiveMsgBuf_ = NULL;
    receiveMsgObj_ = receiveMsgHdr_ = NULL;
    receiveMsgComplete_ = FALSE;
    }
  
  if (inBufList_.entries())
    {
    if (receiveBufList_.entries() == 0)
      { // get first buffer of message
      inBufList_.getFirst(receiveMsgBuf_);
      receiveBufList_.insert(receiveMsgBuf_);
      receiveMsgBufI_ = 0;

      // get message header (already unpacked by Guardian IpcConnection)
      receiveMsgHdr_ = (InternalMsgHdrInfoStruct*)(receiveMsgBuf_->data(0)); 
      if (receiveMsgHdr_->isLastMsgBuf())
        { // single buffer message, proceed
        receiveMsgComplete_ = TRUE;
        // unpack 1st user msg object's base class (new should handle NULL)
	IpcMessageObj *msgHeap = receiveMsgHdr_->getNextFromOffset();
	if (!msgHeap)
	  receiveMsgObj_ = NULL;
	else
	  receiveMsgObj_ = new(msgHeap) IpcMessageObj(this);
        if (receiveMsgObj_ == NULL)
        { // recursively call to advance past empty message buffers
          receiveMsgBufI_++;
          
          NABoolean result = getNextReceiveMsg(msgType);
          return result;
        }
        
        msgType = receiveMsgHdr_->getType();
        return (TRUE);
        }
      }

    // Try and extract remaining buffers associated with the same message from
    // the in queue. The buffers must originate from the same connection OR the
    // same local message stream and must have matching message types. A TRUE 
    // return code will be returned only after all buffers comprising the 
    // message have arrived.
    
    for (CollIndex i = 0; i < inBufList_.entries(); )
      { // get additional buffers of multi-buffer message  
      IpcMessageBuffer* msgBuf = inBufList_[i];
      if ((msgBuf->getConnection() == receiveMsgBuf_->getConnection())   &&
          (msgBuf->getMessageStream() == receiveMsgBuf_->getMessageStream()) )
        {
        // get message header (already unpacked by Guardian IpcConnection)
        InternalMsgHdrInfoStruct* msgHdr = (InternalMsgHdrInfoStruct*)(msgBuf->data(0));
        if  (msgHdr->getMsgStreamId() == receiveMsgHdr_->getMsgStreamId())
          {
          assert(msgHdr->getType() == receiveMsgHdr_->getType());
          inBufList_.removeAt(i);
          receiveBufList_.insert(msgBuf);
          if (msgHdr->isLastMsgBuf())
            { // all buffers for this message received, proceed
            // unpack next message object's base class 
            receiveMsgObj_ = 
              new(receiveMsgHdr_->getNextFromOffset()) IpcMessageObj(this);
            assert(receiveMsgObj_);  // should not be NULL if multiple bufs
            receiveMsgComplete_ = TRUE;
            
            msgType = receiveMsgHdr_->getType();
            return (TRUE);
            }
          continue;
          }
        }
      i++;
      }
    }

  return (FALSE);
  }

///////////////////////////////////////////////////////////////////////////////
// get next message object type from current receive message.
NABoolean IpcBufferedMsgStream::getNextObjType(IpcMessageObjType& msgType)
  {
  if (receiveMsgObj_) 
    {
    msgType = receiveMsgObj_->getType();

    return (TRUE);
    }
  
  return (FALSE);
  }

///////////////////////////////////////////////////////////////////////////////
// get next message object size from current receive message.
IpcMessageObjSize IpcBufferedMsgStream::getNextObjSize() const
{
  return (receiveMsgObj_ ? receiveMsgObj_->s_.objLength_ : 0);
}

///////////////////////////////////////////////////////////////////////////////
// get a pointer to the next packed object in the current receive message.
IpcMessageObj* IpcBufferedMsgStream::receiveMsgObj()
  {
  if (receiveMsgObj_ == NULL)
     return (NULL);   // no more receive message objects
 
  IpcMessageObj* returnMsgObj_ = receiveMsgObj_;
  
  // the base class portion of the next packed object (IpcMessageObj) is
  // unpacked inplace now, before the derived class constructor is called
  // to allow it to be used internally by IpcBufferedMsgStream. The message
  // object will behave as a base class object until the derived class
  // constructor completes (new should handle NULL).
  IpcMessageObj *msgHeap = returnMsgObj_->getNextFromOffset();
  if (!msgHeap)
    receiveMsgObj_ = NULL;
  else 
    receiveMsgObj_ = new(msgHeap) IpcMessageObj(this);
  
  while (receiveMsgObj_ == NULL)
    { // check next receive message buffer
    receiveMsgBufI_++;
    if (receiveMsgBufI_ >= receiveBufList_.entries())
      { // done with this message
      receiveMsgBuf_ = NULL;
      receiveMsgObj_ = receiveMsgHdr_ = NULL;

      break;
      }

    receiveMsgBuf_ = receiveBufList_[receiveMsgBufI_];
    // msg header already unpacked by Guardian IpcConnection
    receiveMsgHdr_ = (InternalMsgHdrInfoStruct*)(receiveMsgBuf_->data(0));
    // unpack next message object's base class (new should handle NULL).
    IpcMessageObj *msgHeap = receiveMsgHdr_->getNextFromOffset();
    if (!msgHeap)
      receiveMsgObj_ = NULL;
    else
      receiveMsgObj_ = new(msgHeap) IpcMessageObj(this);
    }
  
  return (returnMsgObj_);
  }

///////////////////////////////////////////////////////////////////////////////
// give current receive message to a peer message stream for processing.
void IpcBufferedMsgStream::giveReceiveMsgTo(IpcBufferedMsgStream& msgStream)
{
  assert(receiveMsgComplete_);
  
  IpcMessageBuffer* msgBuf;
  while (receiveBufList_.getFirst(msgBuf))
  {
    if (msgBuf->getReplyTag() != GuaInvalidReplyTag)
    {
      replyTagBufList_.remove(msgBuf);
      msgBuf->decrRefCount(); // no longer in two lists!
    }
    msgStream.addInputBuffer(msgBuf);
  }

  receiveMsgBuf_ = NULL;
  receiveMsgObj_ = receiveMsgHdr_ = NULL;
  receiveMsgComplete_ = FALSE;
  
  msgStream.actOnReceive(NULL);   // trigger receiving message stream
}

///////////////////////////////////////////////////////////////////////////////
// pack an object in the current send message
IpcBufferedMsgStream& IpcBufferedMsgStream::operator << (IpcMessageObj& obj)
  {
  IpcMessageObjSize packedLength = obj.packedLength();
  IpcMessageObj* packedObj = 
    new(*this, packedLength) IpcMessageObj(obj.getType(), obj.getVersion());

  // IpcBufferedMsgStream does NOT allow the object to pack the base class.
  // This is because we want to ensure that the packed object behaves as a
  // generic IpcMessageObj with NO virtual behavior. The basic IpcMessageObj
  // is constructed in the buffer via the new operator above.
  IpcMessageObjSize packedBytes = 
    obj.packObjIntoMessage((IpcMessageBufferPtr)(&packedObj[1]));
      
  // Did the object really use the length it promised to use?
  assert(packedLength == packedBytes);
  
  return *this;
  }

///////////////////////////////////////////////////////////////////////////////
// unpack the next object in the current receive message
NABoolean IpcBufferedMsgStream::extractNextObj(IpcMessageObj &obj,
                                               NABoolean checkObjects)
{
  IpcMessageObj* packedObj = receiveMsgObj();

  // the user should have predicted the object type and version correctly
  assert(packedObj  AND 
         packedObj->getType() == obj.getType()  AND
         packedObj->getVersion() == obj.getVersion());

  // IpcBufferedMsgStream does NOT allow the object to unpack the base class.
  // This is because the packed object behaves as a generic IpcMessageObj and
  // the destination object wishes to retain its virtual behavior.
  IpcMessageObjSize objLen = packedObj->s_.objLength_ - sizeof(IpcMessageObj);

  // If the caller requested an integrity check on the packed object
  // then we call a checkObj() method before unpackObj().
  NABoolean result = TRUE;
  if (checkObjects)
  {
    result = obj.checkObj(packedObj->getType(),
                          packedObj->getVersion(),
                          TRUE,
                          objLen,
                          (IpcConstMessageBufferPtr)(&packedObj[1]));
  }
  
  if (result)
  {
    obj.unpackObj(packedObj->getType(),
                  packedObj->getVersion(),
                  TRUE,
                  objLen,
                  (IpcConstMessageBufferPtr)(&packedObj[1]));
  }

  return result;
}

///////////////////////////////////////////////////////////////////////////////
// allocate space for a packed object in the current send message.
IpcMessageObj* IpcBufferedMsgStream::sendMsgObj(IpcMessageObjSize packedObjLen)
{
  IpcMessageBuffer::alignOffset(packedObjLen);

  if (sendMsgBuf_ == NULL)
    {
    IpcMessageObjSize length = 
      (packedObjLen > bufferSize_ ? packedObjLen : bufferSize_); 
    length += sizeof(InternalMsgHdrInfoStruct);

    CollHeap *heap = (environment_ ? environment_->getHeap() : NULL);
    
    sendMsgBuf_ = IpcMessageBuffer::allocate(length,
                                             this,
                                             heap,
                                             0);
    
    if (sendMsgBuf_ == NULL)
      return NULL;   // no buffers available

    sendBufList_.insert(sendMsgBuf_);
    // Create message header
    // Recursively calls sendMsgObj() via operator new
    sendMsgHdr_ = new(*this, 0) InternalMsgHdrInfoStruct(this->msgType_,
                                                        this->msgVersion_);
    sendMsgHdr_->totalLength_ = sizeof(InternalMsgHdrInfoStruct);
    }

  if (packedObjLen == 0)
    return (NULL); // internal call to create an empty buffer, hdr only

  if (sendMsgHdr_ == NULL)
    { // first object must be header
    sendMsgObj_ = sendMsgHdr_ = 
                       (InternalMsgHdrInfoStruct*)(sendMsgBuf_->data(0));
    }
  else
  {
    IpcMessageObjSize newLen = sendMsgHdr_->totalLength_ + packedObjLen;
    if (newLen > sendMsgBuf_->getBufferLength())
    {
      sendMsgBuf_ = NULL;
      sendMsgObj_ = sendMsgHdr_ = NULL;

      return (sendMsgObj(packedObjLen)); //recursively call for new buffer
    }

    // link to previous last object (must coexist with IpcMessageStream!)
    // IpcBufferedMsgStream only uses "next_" as an offset although it is 
    // defined as a pointer. IpcMessageStream uses next as both pointer and
    // offset. The objLength_ field gets set later, in prepSendMsgForOutput()
    sendMsgObj_->s_.next_ =
      (IpcMessageObj*)(sendMsgBuf_->data(sendMsgHdr_->totalLength_));
    sendMsgObj_->convertNextToOffset();
    sendMsgObj_ =
      (IpcMessageObj*)(sendMsgBuf_->data(sendMsgHdr_->totalLength_));
    sendMsgHdr_->totalLength_ = newLen;
    
  }
  
  return (sendMsgObj_);
}

IpcConnection* IpcBufferedMsgStream::getConnection()
{
  ABORT("IpcMessageStreamBase::getConnection() should not be called!");
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// cleanup unpacked message buffers with objects no longer inuse
void IpcBufferedMsgStream::cleanupBuffers()
{
  if (inUseBufList_.entries() <= (CollIndex) garbageCollectLimit_) 
    return;
  
  CollIndex i = 0;
  while (i < inUseBufList_.entries())
  {  
    IpcMessageBuffer* msgBuf = inUseBufList_[i];
    InternalMsgHdrInfoStruct* msgHdr = 
      (InternalMsgHdrInfoStruct*)(msgBuf->data(0));
    IpcMessageObj* msgObj = msgHdr;
    IpcMessageObj* nextMsgObj;
    while (nextMsgObj = msgObj->getNextFromOffset())
    { 
      if (!(nextMsgObj->msgObjIsFree()))
        break;  // quit, no need to check remaining objects in buffer
      
      msgObj->mergeNextPackedObj(); // unlink msg objects no longer in use
    }
    
    if (msgHdr->getNextFromOffset() == NULL)
    { // all objects are no longer inuse, free buffer
      inUseBufList_.removeAt(i);
      msgBuf->decrRefCount();
    }
    else
    { // msg objects in buffer still in use
      i++;
      while (!(msgHdr->isLastMsgBuf()))
      { // skip over remaining buffers for this message.
        // must guarantee that a complex object consisting of multiple message
        // objects or a set of objects within a message can be held inuse by 
        // the root(first allocated) object!
        assert(i < inUseBufList_.entries());
        msgBuf = inUseBufList_[i];
        msgHdr = (InternalMsgHdrInfoStruct*)(msgBuf->data(0));
        i++;
      }
    }
  }
  if (inUseBufList_.entries() < (CollIndex) inUseBufferLimit_)
  { // optimize garbage collection limit until inuse limit reached
    garbageCollectLimit_ = inUseBufList_.entries(); 
  }
  
}

////////////////////////////////////////////////////////////////////////////
// prepare send message objects for output and put buffers in output queue.
void IpcBufferedMsgStream::prepSendMsgForOutput()
{
  while (sendBufList_.getFirst(sendMsgBuf_))
  {
    sendMsgHdr_ = (InternalMsgHdrInfoStruct*)(sendMsgBuf_->data(0));
    sendMsgBuf_->setMessageLength(sendMsgHdr_->totalLength_);
    MXTRC_1("totalLength_=%d \n", sendMsgHdr_->totalLength_);
    if (sendBufList_.entries() == 0)
      sendMsgHdr_->setLastMsgBuf();
    sendMsgHdr_->setMsgStreamId((Long)this);
    sendMsgObj_ = sendMsgHdr_;
    do
      {
      // calculate object length, note that this is done only to be
      // compatible with IpcMessageStream and that this object length
      // is rounded up to the alignment that was done internally
      if (sendMsgObj_->s_.next_)
        sendMsgObj_->s_.objLength_ =
	  (IpcMessageObjSize)(Long)((sendMsgObj_->s_.next_));
      else
        sendMsgObj_->s_.objLength_ =
          (IpcMessageObjSize)(sendMsgBuf_->data(0)
			      + sendMsgHdr_->totalLength_
			      - (IpcMessageBufferPtr) sendMsgObj_);
      MXTRC_3("objType_=%d objLength_=%d next_=%d\n", sendMsgObj_->s_.objType_, sendMsgObj_->s_.objLength_, sendMsgObj_->s_.next_);
      sendMsgObj_->prepMsgObjForSend();
      sendMsgObj_->setMyVPtr(NULL);
      }
    while ((sendMsgObj_ = sendMsgObj_->getNextFromOffset()) != NULL);

    outBufList_.insert(sendMsgBuf_);
  }
  
  sendMsgBuf_ = NULL;
  sendMsgObj_ = sendMsgHdr_ = NULL;
}

////////////////////////////////////////////////////////////////////////////
// add a message buffer to the input queue.
void IpcBufferedMsgStream::addInputBuffer(IpcMessageBuffer* inputBuf)
{
  inBufList_.insert(inputBuf); 
  if (inputBuf->getReplyTag() != GuaInvalidReplyTag)
  {
    if (getSMContinueProtocol())
      EXSM_TRACE(EXSM_TRACE_PROTOCOL, "STREAM %p add input buf %p", this,
                 inputBuf);

    inputBuf->incrRefCount(); // buffer in 2 lists, incr to hold!

    // save reply tag, needed for response
    replyTagBufList_.insert(inputBuf);
  }
}

///////////////////////////////////////////////////////////////////////////////
// get next message buffer from output queue matched with next reply tag.
IpcMessageBuffer* IpcServerMsgStream::getReplyTagOutputBuffer(
                                            IpcConnection*& connection,
                                            IpcBufferedMsgStream*& msgStream)
{
  MXTRC_FUNC("IpcServerMsgStream::getReplyTagOutputBuffer");

  if (numOfReplyTagBuffers())
  {
    IpcMessageBuffer* outputBuf = getOutputBuffer();
    if (outputBuf)
    {
      IpcMessageBuffer* replyTagBuf = replyTagBufList_[0];
      
      outputBuf->setReplyTag(replyTagBuf->getReplyTag());
      outputBuf->setMaxReplyLength(replyTagBuf->getMaxReplyLength());
      connection = replyTagBuf->getConnection();
      MXTRC_2("replyTag=%d connection=%x", replyTagBuf->getReplyTag(), connection);
      msgStream = (IpcBufferedMsgStream*)(replyTagBuf->getMessageStream());

      NABoolean deleteReplyTag = FALSE;

      // For the SeaMonster continue protocol the communication
      // between client and server is not one to one. That is client
      // does not send a continue message for every reply from the
      // server and the server does not send only one reply for every
      // request.  For every request the server receives, the server
      // replies with the number of buffers set by
      // sendBufferLimit_. The server sets the LAST IN BATCH flag for
      // the last buffer in a batch and waits for a continue message
      // from the client before it sends more replies.
      //
      // The way a stream controls if more buffers can be sent to the
      // client is through replyTagBufList_. If there are any entries
      // in replyTagBufList_ then the server stream can send messages
      // to the client and if there are no entries in replyTagBufList_
      // then the server stream cannot send any more messages until
      // the client sends a continue request and that gets added to
      // replyTagBufList_.
      //
      // So for SeaMonster continue protocol just after sending one
      // reply server cannot delete the request from replyTagBufList_
      // and needs to wait until the LAST IN BATCH reply or the EOD
      // reply is sent. The following changes make sure that for
      // SeaMonster continue protocol we do not delete the request
      // from replyTagBufList_ except for the LAST IN BATCH or the EOD
      // reply.
      if (getSMContinueProtocol())
      {
        buffersSentInBatch_++;

        InternalMsgHdrInfoStruct *hdr = 
                        (InternalMsgHdrInfoStruct *)outputBuf->data(0);

        // If this buffer is the last in batch to be sent then mark the buffer
        // as LAST IN BATCH and remember to delete the buffer from reply tag
        // If this buffer is not last in batch then check if LAST IN BATCH 
        // was already set by the TCB since it reached EOD, if so remember
        // to delete buffer from reply tag
        if (buffersSentInBatch_ == sendBufferLimit_)
        {
          EXSM_TRACE(EXSM_TRACE_PROTOCOL, 
                     "STREAM %p sent %d limit %d sending last in batch",
                     this, buffersSentInBatch_, sendBufferLimit_);
          hdr->setSMLastInBatch();
   
          deleteReplyTag = TRUE;
        }
        else if (hdr->getSMLastInBatch())
          deleteReplyTag = TRUE;
      }
      else // regular one to one continue protocol case
        deleteReplyTag = TRUE;
   
      if (deleteReplyTag)
      {
        buffersSentInBatch_ = 0;
        replyTagBuf->setReplyTag(GuaInvalidReplyTag); // invalidate reply tag
        replyTagBuf->decrRefCount(); // free if not in another list
        replyTagBufList_.remove(replyTagBuf);
      }

      return outputBuf;

    } // if (outputBuf)
  } // if (numOfReplyTagBuffers())

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////  
// internal send call back may be redefined by derived classes.
void IpcBufferedMsgStream::internalActOnSend(IpcConnection* connection)
  {
  if (connection && !errorInfo_)
    {
    errorInfo_ = connection->getErrorInfo();
    }
  actOnSend(connection);
  }
                 
///////////////////////////////////////////////////////////////////////////////  
// internal receive call back may be redefined by derived classes.
void IpcBufferedMsgStream::internalActOnReceive(IpcMessageBuffer* buffer,
                                                IpcConnection* connection)
  {
  if (connection && !errorInfo_)
    {
    errorInfo_ = connection->getErrorInfo();
    }
  if (buffer)
    {
    addInputBuffer(buffer);
    }
  actOnReceive(connection);
  }

void IpcBufferedMsgStream::actOnSendAllComplete()
{
  // the default callback implementation does nothing
}

void IpcBufferedMsgStream::actOnReceiveAllComplete()
{
  // the default callback implementation does nothing
}

// ----------------------------------------------------------------------------
// IpcClientMsgStream
// ----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// constructor
IpcClientMsgStream::IpcClientMsgStream(IpcEnvironment *env,
                                       IpcMessageType msgType,
                                       IpcMessageObjVersion version,
                                       Lng32 sendBufferLimit,
                                       Lng32 inUseBufferLimit,
                                       IpcMessageObjSize bufferSize)
  : IpcBufferedMsgStream(env, msgType, version, inUseBufferLimit, bufferSize),
    sendBufferLimit_(sendBufferLimit),
    responsesPending_(0),
    recipients_(env->getAllConnections(),env->getHeap()),
    localRecipients_(env->getHeap()),
    localReplyTag_(0),
    smBatchIsComplete_(FALSE)
{ }

///////////////////////////////////////////////////////////////////////////////
// broadcast the current send message to all recipients
void IpcClientMsgStream::sendRequest(Int64 transid)
  {
  MXTRC_FUNC("IpcClientMsgStream::sendRequest");
  prepSendMsgForOutput();

  while (numOfOutputBuffers())
    {
    IpcMessageBuffer* msgBuf;
    CollIndex numRecipients = 
                          recipients_.entries() + localRecipients_.entries();

    for (IpcConnectionId i = 0; recipients_.setToNext(i); i++)
      { // send output message to all remote connections
      numRecipients--;
      msgBuf = numRecipients ? copyOutputBuffer() : getOutputBuffer(); 

      // store transid in msgbuf
      msgBuf->setTransid (transid);

      responsesPending_++;

      if (getSMContinueProtocol())
        EXSM_TRACE(EXSM_TRACE_PROTOCOL, "STREAM %p rp is now %d", this,
                   (int) responsesPending_);
      
      IpcConnection *conn = recipients_.element(i);
      conn->send(msgBuf);
    }

    assert(numRecipients == localRecipients_.entries()); 

    for (CollIndex j = 0; j < localRecipients_.entries(); j++)
      { // send output message to all local server msg streams
      IpcBufferedMsgStream* msgStream = localRecipients_[j];
      numRecipients--;
      msgBuf = numRecipients ? copyOutputBuffer() : getOutputBuffer(); 
      responsesPending_++;
      msgBuf->setReplyTag(getLocalReplyTag());
      msgStream->internalActOnReceive(msgBuf, NULL);
      }
    assert(numRecipients == 0);
    }

  // commenting out the following code as this is causing problems for SM,
  // removing this code does not cause problems for Seabed messages as this
  // is mostly noop for seabed messages
  //recipients_.waitOnSet(IpcImmediately);
  }

// abort any outstanding I/Os on this stream
void IpcClientMsgStream::abandonPendingIOs()
{
  while (numOfResponsesPending() > 0)
    {
      for (IpcConnectionId i = 0; recipients_.setToNext(i); i++)
        {
          IpcConnection * c = recipients_.element(i);
          NABoolean useGuaIpc = TRUE;
          if (useGuaIpc)
            {
              if (c->castToGuaConnectionToServer())
                c->castToGuaConnectionToServer()->setFatalError(this);
            }
        } // for
    } // while
}

///////////////////////////////////////////////////////////////////////////////
// internal receive call back 
void IpcClientMsgStream::internalActOnReceive(IpcMessageBuffer* buffer,
                                              IpcConnection* connection)
{
  MXTRC_FUNC("IpcClientMsgStream::internalActOnReceive");

  // Note: It is possible for this function to be called with a NULL
  // buffer pointer under certain error conditions.

  if (buffer)
    buffer->setReplyTag(GuaInvalidReplyTag); // invalidate so buff can cleanup

  // Cases to consider
  // (a) buffer is NULL (which indicates an error condition)
  // (b) stream is NOT following seamonster continue protocol
  // (c) stream IS following seamonster continue protocol

  if (!getSMContinueProtocol() || !buffer)
  {
    // Cases (a) and (b)
    if (responsesPending_ > 0)
      responsesPending_--;
    else
    {
      connection->dumpAndStopOtherEnd(true, false);
      assert(responsesPending_ > 0);
    }
  }
  else
  {
    // Case (c)
    InternalMsgHdrInfoStruct *hdr =
      (InternalMsgHdrInfoStruct *) buffer->data(0);
    assert(hdr);

    if (hdr->getSMLastInBatch())
    {
      // Remember that the LAST IN BATCH buffer is received so later
      // in actOnReceive() we can decrement the statement globals
      // message counter. This saved this information in a stream
      // data member since the actOnReceive() method does not have a
      // buffer pointer to check the actual LAST IN BATCH flag.
      smBatchIsComplete_ = TRUE;
      
      responsesPending_--;
      EXSM_TRACE(EXSM_TRACE_PROTOCOL, "STREAM %p rp is now %d", this,
                 (int) responsesPending_);

      IpcConnection *conn = connection->castToSMConnection();
      assert(conn);
      ((SMConnection *)conn)->decrOutstandingSMRequests();
    }
  }

  // Call the parent class internalActOnReceive() function. This will
  // invoke the child class actOnReceive() virtual method.
  IpcBufferedMsgStream::internalActOnReceive(buffer, connection);

  if (getSMContinueProtocol())
  {
    // After receive callback processing is complete in the parent and
    // child classes, we can reset the batch complete flag
    smBatchIsComplete_ = FALSE;
  }
}

///////////////////////////////////////////////////////////////////////////////
// internal send call back
void IpcClientMsgStream::internalActOnSend(IpcConnection* connection)
{
  // We'll have to initiate receive even if connection is in error. This
  // is because the stream needs to do bookkeeping and other work only
  // when the receive callback is called. As result, decrementing
  // responsesPending_ becomes unnecessary. Please ignore next comment

  // If an error occurred, we will decrement responsesPending_ before
  // calling the superclass implementation of this method. This way
  // when the superclass method issues send callbacks, those callbacks
  // can correctly determine whether responsesPending_ has dropped to
  // zero following an IPC error.

  // But before making any judgements about whether an error occurred,
  // first make sure this stream's errorInfo_ field is updated with
  // any error information from the connection.
  if (connection && !errorInfo_)
    errorInfo_ = connection->getErrorInfo();

  IpcBufferedMsgStream::internalActOnSend(connection);
  if (connection)
  {
    connection->receive(this);
  }
}

// ----------------------------------------------------------------------------
// IpcServerMsgStream
// ----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// constructor
IpcServerMsgStream::IpcServerMsgStream(IpcEnvironment *env,
                                       IpcMessageType msgType,
                                       IpcMessageObjVersion version,
                                       Lng32 sendBufferLimit,
                                       Lng32 inUseBufferLimit,
                                       IpcMessageObjSize bufferSize)
  : IpcBufferedMsgStream(env, msgType, version, inUseBufferLimit, bufferSize),
    sendBufferLimit_(sendBufferLimit),
    client_(NULL),
    buffersSentInBatch_(0)
  { };

//////////////////////////////////////////////////////////////////////////////
// send the current response message back to the client
void IpcServerMsgStream::sendResponse()
  {
  MXTRC_FUNC("IpcServerMsgStream::sendResponse");
  prepSendMsgForOutput();
  tickleOutputIo();
  }

//////////////////////////////////////////////////////////////////////////////
// server is done replying to all requests
void IpcServerMsgStream::responseDone()
{
  while (numOfOutputBuffers() < numOfReplyTagBuffers())
  { // create empty responses for excess reply tags
    sendMsgObj(0);  
    prepSendMsgForOutput();
  }
  tickleOutputIo();
}

///////////////////////////////////////////////////////////////////////////////
// reply to outstanding requests from the output queue
void IpcServerMsgStream::tickleOutputIo()
{
  // This early return is done without any SeaMonster tracing so that
  // SeaMonster tracing only happens when there is actually something
  // to send
  if (numOfOutputBuffers() == 0)
    return;

  if (getSMContinueProtocol())
    EXSM_TRACE(EXSM_TRACE_PROTOCOL,
               "STREAM %p BEGIN tickleOutputIo bufs %d", this,
               (int) numOfOutputBuffers());
  
  IpcMessageBuffer* msgBuf;
  IpcConnection* connection; 
  IpcBufferedMsgStream* msgStream;
  MXTRC_FUNC("IpcServerMsgStream::tickleOutputIo");
  while (msgBuf = getReplyTagOutputBuffer(connection, msgStream))
  {
    if (connection)
    {
      // respond to remote client message stream via connection
      connection->send(msgBuf);
      
      // comment out the following wait to eliminate the cause of bug 2473
      // in an open/close cursor test that recreates the problem
      //connection->wait(IpcImmediately);
    }
    else
    {  // respond to local client message stream
      msgStream->internalActOnReceive(msgBuf, NULL);
    }
  }

  if (getSMContinueProtocol())
    EXSM_TRACE(EXSM_TRACE_PROTOCOL,
               "STREAM %p END tickleOutputIo bufs %d", this,
               (int) numOfOutputBuffers());
}

void IpcBufferedMsgStream::setSMLastInBatch()
{
  Int32 entries = (Int32) numOfSendBuffers();
  assert(entries > 0);

  IpcMessageBuffer *msgBuf = sendBufList_[entries - 1];
  InternalMsgHdrInfoStruct *hdr = (InternalMsgHdrInfoStruct *) msgBuf->data(0);
  hdr->setSMLastInBatch();
}

///////////////////////////////////////////////////////////////////////////////
// internal receive call back 
void IpcServerMsgStream::internalActOnReceive(IpcMessageBuffer* buffer,
                                              IpcConnection* connection)
{
  if (buffer)
  {
    assert(buffer->getReplyTag() != GuaInvalidReplyTag); // vaild reply tag ?

    if (client_ != NULL)
    {
      // cannot be recipient of local msg stream if receiving from connection
      assert(client_ == buffer->getConnection());
      client_->receive(this);
    }
  }
  IpcBufferedMsgStream::internalActOnReceive(buffer, connection);
}

// -----------------------------------------------------------------------
// Methods for class IpcServerClass
// -----------------------------------------------------------------------

IpcServerClass::IpcServerClass(IpcEnvironment *env,
			       IpcServerType serverType,
			       IpcServerAllocationMethod allocationMethod,
                               short serverVersion,
                               char *nodeName) :
     allocatedServers_(env->getHeap())
{
  environment_ = env;
  serverType_ = serverType;
  allocationMethod_ = allocationMethod;
  serverVersion_ = serverVersion;
  nodeName_ = nodeName;
  char *parallelOpens = getenv("ESP_PARALLEL_CC_OPENS");
  if (parallelOpens != NULL && *parallelOpens == '0')
    parallelOpens_ = FALSE;
  else
    parallelOpens_ = TRUE;
  Int32 retVal;
  retVal = pthread_mutex_init(&nowaitedEspServer_.cond_mutex_, NULL);
  assert(retVal == 0);
  retVal = pthread_cond_init(&nowaitedEspServer_.cond_cond_, NULL);
  assert(retVal == 0);
  nowaitedEspServer_.startTag_ = 0;
  nowaitedEspServer_.callbackCount_ = 0;
  nowaitedEspServer_.completionCount_ = 0;
  nowaitedEspServer_.waiting_ = FALSE;
  char *waitedStartupArg = getenv("ESP_PARALLEL_STARTUP");
  if (waitedStartupArg == NULL)
    nowaitedEspServer_.waitedStartupArg_ = '0';
  else
  {
    switch (*waitedStartupArg)
    {
    case '0':
      nowaitedEspServer_.waitedStartupArg_ = '1';
      break;
    case '1':
      nowaitedEspServer_.waitedStartupArg_ = '0';
      break;
    default:
      nowaitedEspServer_.waitedStartupArg_ = '0';
    }
  }
  if (allocationMethod_ == IPC_ALLOC_DONT_CARE)
    {
      // NA_WINNT is set and NA_GUARDIAN_IPC is set
      // The standard method on NT is to create a Guardian process
      // in order to run in an NT only or simulated environment we can set an environment
      // variable to override that mechanism.
      if (getenv("SQL_NO_NSK_LITE") == NULL)
        {
          allocationMethod_ = IPC_LAUNCH_GUARDIAN_PROCESS;
        }
      else 
        {
          allocationMethod_ = IPC_LAUNCH_NT_PROCESS;
          time_t tp;
          time(&tp);
	  nextPort_ = IPC_SQLESP_PORTNUMBER + tp % 10000; // arbitrary
        };
    }
}

IpcServerClass::~IpcServerClass() 
{
   NAHeap *heap = (NAHeap *)environment_->getHeap();
   CollIndex entryCount;
   entryCount  = allocatedServers_.entries();
   for (CollIndex i = 0 ; i < entryCount; i++) {
       NADELETE(allocatedServers_[i], IpcServer, heap);
   }
   allocatedServers_.clear();
}

IpcServer * IpcServerClass::allocateServerProcess(ComDiagsArea **diags,
						  CollHeap   *diagsHeap,
						  const char *nodeName,
						  IpcCpuNum cpuNum,
						  IpcPriority priority,
						  Lng32 espLevel,
						  NABoolean usesTransactions,
						  NABoolean waitedCreation,
						  Lng32 maxNowaitRequests,
						  const char* progFileName,
						  const char* processName,
						  NABoolean parallelOpens,
						  IpcGuardianServer **creatingProcess)
{
  IpcServer *result = NULL;
  short retcode = 0;
  if (creatingProcess != NULL)
  {
    result = *creatingProcess;
    if ((*creatingProcess)->isCreatingProcess())
    {
      assert(waitedCreation == FALSE);
      retcode = (*creatingProcess)->workOnStartup(IpcInfiniteTimeout,diags,diagsHeap);
      if ((*creatingProcess)->isCreatingProcess())
      {
	result = NULL;
	return result; // Launch didn't complete
      }
    }
    else
      assert(FALSE); // Existing process in not in creating state
    if (retcode)
      {
        char buf[20];
        str_sprintf(buf, "retcode = %d", retcode);
        const char *retcodeText = buf;
        (*creatingProcess)->logEspRelease(__FILE__, __LINE__, 
                      retcodeText);
        (*creatingProcess)->release();
        *creatingProcess = NULL; // Launch completed
        result = NULL;
      }
    // remember this server
    if (result != NULL)
      allocatedServers_.insert(result);
    return result;
  }
  NABoolean lv_usesTransactions = usesTransactions;
  Lng32 lv_maxNowaitRequests = maxNowaitRequests;
 
  IpcConnection *serverConn = NULL;
  const char *className = NULL;
  IpcServerPortNumber defaultPortNumber = IPC_INVALID_SERVER_PORTNUMBER;
  NABoolean debugServer = FALSE;
  const char *overridingDefineName = NULL;

  debugServer = (getenv("DEBUG_SERVER") != NULL);

  // to avoid compiler warning for Unix build
  waitedCreation = waitedCreation;

  switch (serverType_)
    {
    case IPC_SQLUSTAT_SERVER:
      if (debugServer)
	{
	  className = "arkustatdbg";
	  defaultPortNumber = IPC_SQLUSTAT_DEBUG_PORTNUMBER;
	}
      else
	{
	  className = "arkustat";
	  defaultPortNumber = IPC_SQLUSTAT_PORTNUMBER;
	}
	  overridingDefineName = "ARK_STA_PROG_FILE_NAME";
      break;
    case IPC_SQLCAT_SERVER:
      if (debugServer)
	{
	  className = "arkcatdbg";
	  defaultPortNumber = IPC_SQLCAT_DEBUG_PORTNUMBER;
	}
      else
	{
	  className = "arkcat";
	  defaultPortNumber = IPC_SQLCAT_PORTNUMBER;
	}
	  overridingDefineName = "ARK_CAT_PROG_FILE_NAME";
      break;
    case IPC_SQLCOMP_SERVER:
      if (debugServer)
	{
	  className = "arkcmpdbg";
	  defaultPortNumber = IPC_SQLCOMP_DEBUG_PORTNUMBER;
	}
      else
	{
	  className = "arkcmp";
	  defaultPortNumber = IPC_SQLCOMP_PORTNUMBER;
	}
	  overridingDefineName = "_ARK_CMP_PROG_FILE_NAME";
      break;
    case IPC_SQLESP_SERVER:
      if (debugServer)
	{
	  className = "arkespdbg";
	  defaultPortNumber = IPC_SQLESP_DEBUG_PORTNUMBER;
	}
      else
	{
	  className = "arkesp";
	  defaultPortNumber = IPC_SQLESP_PORTNUMBER;
	}
      overridingDefineName = "_ARK_ESP_PROG_FILE_NAME";
      break;
   case IPC_SQLBDRR_SERVER:
      className = "bdrr";
      overridingDefineName = "=_MX_BDRR_PROG_FILE_NAME";
      break;
      //
      // UDR Servers
      //
      
    case IPC_SQLUDR_SERVER:
      {
        if (debugServer)
        {
          className = "udrservdbg";
          defaultPortNumber = IPC_SQLUDR_DEBUG_PORTNUMBER;
        }
        else
        {
          className = "udrserv";
          defaultPortNumber = IPC_SQLUDR_PORTNUMBER;
        }
        overridingDefineName = "_ARK_UDR_PROG_FILE_NAME";
      }
      break;
     
      //
      // Query Matching Server
      //
    case IPC_SQLQMS_SERVER:
      {
        if (debugServer)
        {
          className = "qmsdbg";
          defaultPortNumber = IPC_SQLQMS_DEBUG_PORTNUMBER;
        }
        else
        {
          className = "qms";
          defaultPortNumber = IPC_SQLQMS_PORTNUMBER;
        }
        overridingDefineName = "_ARK_QMS_PROG_FILE_NAME";
      }
      break;

      //
      // Query Matching Publisher
      //
    case IPC_SQLQMP_SERVER:
      {
        if (debugServer)
        {
          className = "qmpdbg";
          defaultPortNumber = IPC_SQLQMP_DEBUG_PORTNUMBER;
        }
        else
        {
          className = "qmp";
          defaultPortNumber = IPC_SQLQMP_PORTNUMBER;
        }
        overridingDefineName = "_ARK_QMP_PROG_FILE_NAME";
      }
      break;  

      //
      // Query Matching Monitor
      //
    case IPC_SQLQMM_SERVER:
      {
        if (debugServer)
        {
          className = "qmmdbg";
          defaultPortNumber = IPC_SQLQMM_DEBUG_PORTNUMBER;
        }
        else
        {
          className = "qmm";
          defaultPortNumber = IPC_SQLQMM_PORTNUMBER;
        }
        overridingDefineName = "_ARK_QMM_PROG_FILE_NAME";
      }
      break; 

      // generic servers passed in as progFileName
    case IPC_GENERIC_SERVER:
    case IPC_SQLBDRS_SERVER:
      if ((allocationMethod_ != IPC_USE_PROCESS) &&
         (! progFileName))
        ABORT("Invalid server type specified in IpcServer::IpcServer()");

      if (allocationMethod_ == IPC_USE_PROCESS)
	className = processName;
      else
        className = progFileName;
      defaultPortNumber = IPC_GENERIC_PORTNUMBER;
      //      overridingDefineName = "_ARK_GENERIC_PROG_FILE_NAME";
      break;
    case IPC_SQLSSCP_SERVER:
      className = "sscp";
      lv_usesTransactions = FALSE;
      lv_maxNowaitRequests =  FS_MAX_NOWAIT_DEPTH;   
      overridingDefineName = "=_MX_SSCP_PROCESS_PREFIX"; 
      break;
    case IPC_SQLSSMP_SERVER:
      className = "ssmp";
      lv_usesTransactions = FALSE;
      lv_maxNowaitRequests =  FS_MAX_NOWAIT_DEPTH;   
      overridingDefineName = "=_MX_SSMP_PROCESS_PREFIX";
      break;
    default:
      ABORT("Invalid server type specified in IpcServer::IpcServer()");
      break;
    }

  switch (allocationMethod_)
    {

    case IPC_LAUNCH_GUARDIAN_PROCESS:
    case IPC_SPAWN_OSS_PROCESS:
    case IPC_USE_PROCESS:
      {
	IpcGuardianServer *result2 =
	  new(environment_) IpcGuardianServer(
	       this,
	       diags,
	       diagsHeap,
	       nodeName,
	       className,
	       cpuNum,
	       priority, //IPC_PRIORITY_DONT_CARE,
	       allocationMethod_,
	       (short) allocatedServers_.entries(),
	       lv_usesTransactions,
	       FALSE,
               waitedCreation,
	       lv_maxNowaitRequests,
	       overridingDefineName,
	       processName,
	       parallelOpens);
	result = result2;
	  retcode = result2->workOnStartup(IpcInfiniteTimeout,diags,diagsHeap);
	if (result2->isCreatingProcess() && retcode == 0)
          return result2;
        if (retcode)
          {
             char buf[20];
             str_sprintf(buf, "retcode = %d", retcode);
             const char *retcodeText = buf;
             result2->logEspRelease(__FILE__, __LINE__,
                      retcodeText);
             result2->release();
             result = NULL;
          }
      }
      break;

    case IPC_INETD:
      {
	serverConn = createInternetProcess(diags,
					   diagsHeap,
					   nodeName,
					   className,
					   cpuNum,
					   usesTransactions,
					   defaultPortNumber);
	// make an IpcServer object
	if (serverConn != NULL)
	  {
	    result = new(environment_) IpcServer(serverConn,this);
	    // $$$$ add errors from creating the connection
	  }
      }
      break;

    case IPC_POSIX_FORK_EXEC:
      {
	serverConn = forkProcess(diags,
				 diagsHeap,
				 nodeName,
				 className,
				 cpuNum,
				 usesTransactions);
	// make an IpcServer object
	if (serverConn != NULL)
	  result = new(environment_) IpcServer(serverConn,this);
      }
      break;


    case IPC_LAUNCH_NT_PROCESS:
      {
	defaultPortNumber = (IpcServerPortNumber)nextPort_;
	nextPort_++;
	
	// make an IpcServer object
	if (serverConn != NULL)
	  result = new(environment_) IpcServer(serverConn,this);
      }
      break;
    default:
      ABORT("Invalid server class allocation method");
      break;

    } // switch

  // remember this server
  if (result != NULL)
    allocatedServers_.insert(result);

  return result;
}

void IpcServerClass::freeServerProcess(IpcServer *s)
{
  // assume caller already killed the server
  allocatedServers_.remove(s);
  NADELETE(s, IpcServer, environment_->getHeap());
}

char *IpcServerClass::getProcessName(short cpuNum, char *processName)
{
  return getServerProcessName(serverType_, cpuNum, processName);
}
// -----------------------------------------------------------------------
// methods for class IpcEnvironment
// -----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
// Helper method getDefineShort
//  Accepts a define name as an argument.  Be sure to make this 24 bytes
//  long, blank padded if necessary, not including the null terminator --
//  see the manual for DEFINEINFO if there are any questions.
//  
//  Return -1 if define not resolved, else it returns an integer parsed 
//  from the class MAP's "file name".
//
// For example:
// add_define =_SQLMX_MAX_OUTGOING_MSG class=MAP file=\$SYSTEM.#128
//
/////////////////////////////////////////////////////////////////////
short getDefineShort( char * defineName )
{
  Lng32 retVal = -1;

  return (short) retVal;
  return (short) retVal;
}


// -----------------------------------------------------------------------
// Methods for IpcEnvironment
// -----------------------------------------------------------------------

 IpcEnvironment::IpcEnvironment(CollHeap *heap, UInt32 *eventConsumed,
                               NABoolean breakEnabled, IpcServerType serverType, NABoolean useGuaIpcAtRuntime
                               , NABoolean persistentProcess
                              ) :
     breakEnabled_(breakEnabled),
     heap_(heap),
     eventConsumedAddr_(eventConsumed),
     completedMessages_(heap),
     envvars_(NULL),
     envvarsLen_(0),
     heapFull_(FALSE),
     safetyBuffer_(NULL),
     stopAfter_(0),
     inactiveTimeout_(0),
     inactiveTimestamp_(0),
     espPrivStackSize_(64 * 1024), // Neo default priv stack size
     espFreeMemTimeout_(0), // initial value, will be overwritten by fixup message. 
     useGuaIpcAtRuntime_(useGuaIpcAtRuntime),
     serverType_(serverType),
     guaMaxMsgIOSize_(IOSIZEMAX),
     maxCCNowaitDepthLow_(InitialNowaitRequestsPerEsp), 
     maxCCNowaitDepthHigh_(HighLoadNowaitRequestsPerEsp),
     maxPerProcessMQCs_(XMAX_SETTABLE_SENDLIMIT_H), // Seaquest "H-Series"/Seaquest limit
     retriedMessageCount_(0),
     cliGlobals_(NULL),
     numOpensInProgress_(0)
     , persistentProcess_(persistentProcess)
     , corruptDownloadMsg_(false)
     , logReleaseEsp_(false)
     , logEspIdleTimeout_(false)
     , logEspGotCloseMsg_(false)
     , logTimeIpcConnectionState_(false)
     , seamonsterEnabled_(false)
{
  if (heap_ == NULL)
    heap_ = new DefaultIpcHeap; // here it's ok to use global operator new
  
  allConnections_ = new(heap_) IpcAllConnections(this, heap_, 
    (serverType == IPC_SQLESP_SERVER
     || serverType == IPC_SQLSSCP_SERVER
     || serverType == IPC_SQLSSMP_SERVER));
  controlConnection_ = NULL;
  for (Lng32 i = 0; i < 4; i++)
    currentExRtFragTable_[i] = NULL; // for integrity checking

  idleTimestamp_ = NA_JulianTimestamp();
#ifdef USE_SB_NEW_RI
  const char *maxenvvar = getenv("IPC_IOSIZEMAX");
  if (maxenvvar)
    guaMaxMsgIOSize_ = MAXOF(3000, MINOF(atoi(maxenvvar), 1048576));
#endif //USE_SB_NEW_RI

  maxPollingInterval_ = 300;
  const char *envvar;
  envvar = getenv("IPC_MAX_POLLING_INTERVAL");
  if (envvar)
    maxPollingInterval_ = atoi(envvar);

  persistentOpenAssigned_ = 0;
  char *perOpensEnvvar = getenv("ESP_PERSISTENT_OPENS");
  Int32 perOpensEnvvarVal;
  if (perOpensEnvvar != NULL )
    perOpensEnvvarVal = atoi(perOpensEnvvar);
  else
    perOpensEnvvarVal = 0;
  if (perOpensEnvvarVal < 1)
  {
    persistentOpens_ = FALSE;
    persistentOpenEntries_ = 64;
  }
  else
  {
    persistentOpens_ = TRUE;
    if (perOpensEnvvarVal < 3)
      persistentOpenEntries_ = 64;
    else if (perOpensEnvvarVal < 256)
      persistentOpenEntries_ = perOpensEnvvarVal;
    else
      persistentOpenEntries_ = 256;
  }
  persistentOpenArray_ = (PersistentOpenEntry (*) [1])heap_->allocateMemory(sizeof(PersistentOpenEntry) * persistentOpenEntries_);
  for (Int32 i = 0; i < persistentOpenEntries_; i++)
  {
    (*persistentOpenArray_)[i].persistentOpenExists_ = FALSE;
  }

  char *masterFastCompletion = getenv("MASTER_FAST_COMPLETION");
  if (masterFastCompletion != NULL && *masterFastCompletion == '0')
    masterFastCompletion_ = FALSE;
  else
    masterFastCompletion_ = TRUE; 
  char *nowaitDepthEnvvar = getenv("ESP_NOWAIT_DEPTH");
  if (nowaitDepthEnvvar != NULL)
    maxCCNowaitDepthLow_ = maxCCNowaitDepthHigh_ = atoi(nowaitDepthEnvvar);
  XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETSENDLIMIT, XMAX_SETTABLE_SENDLIMIT_H);

 if (getenv("ESP_CORRUPT_MESSAGE_TEST"))
   corruptDownloadMsg_ = true;

  const char *lre = getenv("LOG_ESP_RELEASE");
  if (lre && *lre == '1')
    logReleaseEsp_ = true;

  const char *liet = getenv("LOG_IDLE_ESP_TIMEOUT");
  if (liet && *liet == '1')
    logEspIdleTimeout_ = true;

  const char *legcm = getenv("LOG_ESP_GOT_CLOSE_MSG");
  if (legcm && *legcm == '1')
    logEspGotCloseMsg_ = true;

  const char *etis = getenv("ESP_TIME_IPCCONNECTION_STATES");
  if (etis && *etis == '1')
    logTimeIpcConnectionState_ = true;
  
  const char *smEnv = getenv("SQ_SEAMONSTER");
  if (smEnv && *smEnv == '1')
    seamonsterEnabled_ = true;

  char *espAssignByLevel = getenv("ESP_ASSIGN_BY_LEVEL");
  if (espAssignByLevel == NULL)
    espAssignByLevel_ = '0';
  else
  {
    switch (*espAssignByLevel)
    {
    case '1':
      espAssignByLevel_ = '1';
      break;
    default:
      espAssignByLevel_ = '0';
    }
  }

  memset(myProcessName_, 0, sizeof(myProcessName_));

  closeTraceArray_ = (CloseTraceEntry (*) [closeTraceEntries])heap_->allocateMemory(sizeof(CloseTraceEntry) * closeTraceEntries);
  for (Int32 i = 0; i < closeTraceEntries; i++)
  {
    (*closeTraceArray_)[i].count_ = 0;
    (*closeTraceArray_)[i].line_ = 0;
    (*closeTraceArray_)[i].clientFileNumber_ = 0;
    (*closeTraceArray_)[i].cpu_ = 0;
    (*closeTraceArray_)[i].pin_ = 0;
    (*closeTraceArray_)[i].seqNum_ = -1;
  }
  closeTraceIndex_ = closeTraceEntries - 1;
  bawaitioxTraceArray_ = (BawaitioxTraceEntry (*) [bawaitioxTraceEntries])heap_->allocateMemory(sizeof(BawaitioxTraceEntry) * bawaitioxTraceEntries);
  for (Int32 i = 0; i < bawaitioxTraceEntries; i++)
  {
    (*bawaitioxTraceArray_)[i].count_ = 0;
    (*bawaitioxTraceArray_)[i].recursionCount_ = 0;
    (*bawaitioxTraceArray_)[i].firstConnectionIndex_ = 0;
    (*bawaitioxTraceArray_)[i].firstConnection_ = NULL;
    char *ipcAwaitiox = (char *)&((*bawaitioxTraceArray_)[i].ipcAwaitiox_);
    memset((char *)&((*bawaitioxTraceArray_)[i].ipcAwaitiox_), 0, sizeof(IpcAwaitiox));
  }
  bawaitioxTraceIndex_ = bawaitioxTraceEntries - 1;

  // Ipc data message trace area initialization
  maxIpcMsgTraceIndex_ = NUM_IPC_MSG_TRACE_ENTRIES;
  const char *ipcMsgEnv = getenv("NUM_EXE_IPC_MSG_TRACE_ENTRIES");
  if (ipcMsgEnv != NULL)
  {
    Int32 nums = atoi(ipcMsgEnv);
    if (nums >= 0 && nums < MAX_IPC_MSG_TRACE_ENTRIES)
      maxIpcMsgTraceIndex_ = nums;  //ignore any other value
  }
  ipcMsgTraceArea_ = new (heap_) IpcMsgTrace[maxIpcMsgTraceIndex_];
  memset(ipcMsgTraceArea_, 0, sizeof(IpcMsgTrace) * maxIpcMsgTraceIndex_);
  lastIpcMsgTraceIndex_ = maxIpcMsgTraceIndex_;
  ipcMsgTraceRef_ = NULL;
}

  void IpcEnvironment::closeTrace(unsigned short line,
                                  short clientFileNumber,
                                  Int32 cpu,
                                  Int32 pin,
                                  SB_Int64_Type seqNum)
{
  unsigned short i = closeTraceIndex_ == closeTraceEntries - 1 ? 0 : closeTraceIndex_ + 1;
  (*closeTraceArray_)[i].count_ = (*closeTraceArray_)[closeTraceIndex_].count_ + 1;
  (*closeTraceArray_)[i].line_ = line;
  (*closeTraceArray_)[i].clientFileNumber_ = clientFileNumber;
  (*closeTraceArray_)[i].cpu_ = cpu;
  (*closeTraceArray_)[i].pin_ = pin;
  (*closeTraceArray_)[i].seqNum_ = seqNum;
  closeTraceIndex_ = i;
}
  void IpcEnvironment::bawaitioxTrace(IpcSetOfConnections *ipcSetOfConnections,
                                  ULng32 recursionCount,
                                  CollIndex firstConnectionIndex,
                                  IpcConnection *firstConnection,
                                  IpcAwaitiox *ipcAwaitiox)
{
  unsigned short i = bawaitioxTraceIndex_ == bawaitioxTraceEntries - 1 ? 0 : bawaitioxTraceIndex_ + 1;
  (*bawaitioxTraceArray_)[i].count_ = (*bawaitioxTraceArray_)[bawaitioxTraceIndex_].count_ + 1;
  (*bawaitioxTraceArray_)[i].recursionCount_ = recursionCount;
  (*bawaitioxTraceArray_)[i].ipcSetOfConnections_ = ipcSetOfConnections;
  (*bawaitioxTraceArray_)[i].firstConnectionIndex_ = firstConnectionIndex;
  (*bawaitioxTraceArray_)[i].firstConnection_ = firstConnection;
  bawaitioxTraceIndex_ = i;
}

IpcEnvironment::~IpcEnvironment()
{
  if (ipcMsgTraceRef_)
  {
    ExeTraceInfo *ti = cliGlobals_->getExeTraceInfo();
    if (ti)
    {
      ti->removeTrace(ipcMsgTraceRef_);
    }
  }
  NADELETEBASIC(ipcMsgTraceArea_, heap_);

  delete allConnections_;
  delete controlConnection_;
  releaseSafetyBuffer();
}

void IpcEnvironment::stopIpcEnvironment()
{

  NAExit(0);
}

void IpcEnvironment::setIdleTimestamp()
{
  idleTimestamp_ = NA_JulianTimestamp();
}

void IpcEnvironment::setInactiveTimestamp()
{
  inactiveTimestamp_ = NA_JulianTimestamp();
}

void IpcEnvironment::deleteCompletedMessages()
{
  while (completedMessages_.entries())
  {
    IpcMessageStreamBase * mm = completedMessages_[0];
    completedMessages_.remove(mm);
    delete mm;
  }

  // Clean up any SeaMonster send buffers that had been queued but are
  // now completed
  void *buf = NULL;
  while (ExSM_RemoveCompletedSendBuffer(buf))
  {
    IpcMessageBuffer *msgBuf = (IpcMessageBuffer *) buf;
    assert(msgBuf);
    msgBuf->decrRefCount();
  }
}

void IpcEnvironment::setControlConnection(IpcControlConnection *cc)
{
  if (controlConnection_ == NULL)
    controlConnection_ = cc;
  else
  {
    controlConnection_->getConnection()->dumpAndStopOtherEnd(true, false);
    if (cc->getConnection()->getOtherEnd() ==
        controlConnection_->getConnection()->getOtherEnd())
      ; // Already have a core-file.
    else
      cc->getConnection()->dumpAndStopOtherEnd(true, false);
    assert(controlConnection_ == NULL);
  }
}

IpcProcessId IpcEnvironment::getMyOwnProcessId(IpcNetworkDomain dom)
{
  if (dom == IPC_DOM_INVALID)
    {
      // if not specified, the default domains are the "native" domains
      dom = IPC_DOM_INTERNET;
    }

  if (dom == IPC_DOM_INTERNET)
    {
      SockIPAddress sockIpAddr;
      SockPortNumber portNo;

      // get the port number (listner port for server, some number for master)
      if (controlConnection_)
	{
	  // can't have an internet proc id when reading from $RECEIVE
	  assert(controlConnection_->castToSockControlConnection());

	  // get the listner port number from the control connection
	  portNo = controlConnection_->castToSockControlConnection()->
	    getListnerPortNum();
	}
      else
	{
	  portNo = 0;

	}

      // get the IP address of the local node
      sockIpAddr.set();
      
      // make a process id from the IP address and the port number
      return IpcProcessId(sockIpAddr,portNo);
    }
  else if (dom == IPC_DOM_GUA_PHANDLE)
    {
      // for Guardian, just get the phandle from the operating system
      return IpcProcessId(MyGuaProcessHandle());
    }
  else
    {
      ABORT("Invalid domain in IpcEnvironment::getMyOwnProcessId()");
    }
  // make the compiler happy
  return IpcProcessId();
}

IpcPriority IpcEnvironment::getMyProcessPriority()
{
  IpcPriority priority;
  priority = -1;

  return priority;
}

void IpcEnvironment::setEnvVars(char ** envvars)
{
  envvars_ = envvars;
}

void IpcEnvironment::setEnvVarsLen(Lng32 envvarsLen)
{
  envvarsLen_ = envvarsLen;
}

void IpcEnvironment::releaseSafetyBuffer()
{
// Don't mmap and munmap a 512K block on Linux for every download
// and fixup. 256K is not a lot to just leave allocated.
}

void IpcEnvironment::setHeapFullFlag(NABoolean b)
{
  heapFull_ = b;

  if (heapFull_)
  {
    releaseSafetyBuffer();
  }
  else
  {
    // Allocate a safety buffer if we don't have one already. The size
    // chosen here (256K) is arbitrary. It is expected to be "enough"
    // space on the heap to complete some I/Os after the heap became
    // full. The main scanario we know of when the heap becomes full
    // is during broadcast of large ESP fragments.
    if (safetyBuffer_ == NULL)
      safetyBuffer_ = (char *) heap_->allocateMemory(256 * 1024);
  }
}

void IpcEnvironment::notifyNoOpens()
{
  if (serverType_ != IPC_SQLSSCP_SERVER &&
      serverType_ != IPC_SQLSSMP_SERVER)
    stopIpcEnvironment();
}

void IpcEnvironment::logRetriedMessages()
{
  if (retriedMessageCount_ > 0)
  {
  }
}

#ifdef IPC_INTEGRITY_CHECKING

// methods that perform integrity checking on Ipc-related data structures

void IpcEnvironment::setCurrentExRtFragTable(ExRtFragTable *ft)
  {
  NABoolean alreadyIn = FALSE;
  Lng32 i;
  Lng32 firstOpen = -1;

  for (i = 0; i < 4; i++)
    {
    if (ft == currentExRtFragTable_[i])
      alreadyIn = TRUE;
    else if ((currentExRtFragTable_[i] == 0) && (firstOpen < 0))
      firstOpen = i;
    }

  if ((!alreadyIn) && (firstOpen >= 0))
    {
    cerr << "Adding ExRtFragTable integrity check pointer in IpcEnvironment to "
      << (void *)ft << "." << endl;
    // to give debugger a chance to come up
    /*for (long i = 0; i < 10000000; i++)
      {
      long j = i + 1;
      while (j > 1)
        {
        if (j & 1)
          j = 3 * j + 1;
        else
          j = j / 2;
        }
      } */
    currentExRtFragTable_[firstOpen] = ft;
    }
  }

void IpcEnvironment::removeCurrentExRtFragTable(ExRtFragTable *ft)
  {
  Lng32 i;
  
  for (i = 0; i < 4; i++)
    {
    if (ft == currentExRtFragTable_[i])
      {
      currentExRtFragTable_[i] = 0;
      cerr << "Removing ExRtFragTable integrity check pointer in IpcEnvironment to "
        << (void *)ft << "." << endl;
      }
    }
  }

void IpcEnvironment::setExRtFragTableIntegrityCheckPtr
(void (*fnptr) (ExRtFragTable *ft))
  {
  integrityCheckExRtFragTablePtr_ = fnptr;
  }


ExRtFragTable * IpcEnvironment::getCurrentExRtFragTable(Lng32 i) 
  { 
  if ((i >= 0) && (i < 4))
    return currentExRtFragTable_[i];
  
  return 0;
  }

void IpcEnvironment::checkIntegrity(void)
  {
  // IpcEnvironment, being the top of the network of IPC-related data structures,
  // is the place where we begin integrity checking
  checkLocalIntegrity();
  }

void IpcEnvironment::checkLocalIntegrity(void)
  {
  for (Lng32 i = 0; i < 4; i++) 
    {
    if ((currentExRtFragTable_[i]) && (integrityCheckExRtFragTablePtr_))
      {
      // this file doesn't know about the class ExRtFragTable, so we call
      // a C-style function instead in file /executor/ex_frag_rt.cpp...
      //currentExRtFragTable_[i]->checkLocalIntegrity(); 
      (*integrityCheckExRtFragTablePtr_)(currentExRtFragTable_[i]);
      }
    }
  
  allConnections_->checkLocalIntegrity();  // check IpcAllConnections
  }

#endif

short IpcEnvironment::getNewPersistentOpenIndex()
{
  if (persistentOpenAssigned_ < persistentOpenEntries_)
  {
    for (unsigned short i = 0; i < persistentOpenEntries_; i++)
    {
      if ((*persistentOpenArray_)[i].persistentOpenExists_ == FALSE)
        return i;
    }
  }
  return -1;
}

void IpcEnvironment::setPersistentOpenInfo(short index, GuaProcessHandle *otherEnd, short fileNum)
{
  memcpy((void *)&(*persistentOpenArray_)[index].persistentOpenPhandle_, (void *)otherEnd, sizeof(GuaProcessHandle));
  (*persistentOpenArray_)[index].persistentOpenFileNum_ = fileNum;
  (*persistentOpenArray_)[index].persistentOpenExists_ = TRUE;
  persistentOpenAssigned_ += 1;
}

short IpcEnvironment::getPersistentOpenInfo(GuaProcessHandle *otherEnd, short *index)
{
  Int32 guaRetCode;
  for (short i = 0; i < persistentOpenEntries_; i++)
  {
    if ((*persistentOpenArray_)[i].persistentOpenExists_)
    {
      guaRetCode = XPROCESSHANDLE_COMPARE_((SB_Phandle_Type *)otherEnd,
                                           (SB_Phandle_Type *)&(*persistentOpenArray_)[i].persistentOpenPhandle_);
      if (guaRetCode == 2) // Phandles are the same
      {
        *index = i;
        return (*persistentOpenArray_)[i].persistentOpenFileNum_;
      }
    }
  }
  // Matching phandle was not found
  *index = -1;
  return -1;
}

void IpcEnvironment::resetPersistentOpen(short index)
{
  (*persistentOpenArray_)[index].persistentOpenExists_ = FALSE;
  persistentOpenAssigned_ -= 1;
}

const char *IpcMsgTraceDesc =
           "SQL Ipc Message exchanged between consumer and producer processes.\n Can use env NUM_EXE_IPC_MSG_TRACE_ENTRIES to config more or less entries";

void IpcEnvironment::registTraceInfo(ExeTraceInfo *ti)
{
  if (cliGlobals_ && !ipcMsgTraceRef_)
  {
    // register IPC message trace and IPC connection trace
    if (ti)
    {
      Int32 lineWidth = 66;
      void *regdTrace;
      Int32 ret = ti->addTrace("IpcMessages", this, maxIpcMsgTraceIndex_, 6,
                               this, getALine,
                               &lastIpcMsgTraceIndex_,
                               lineWidth, IpcMsgTraceDesc, &regdTrace);
      if (ret == 0)
      {
        // trace info added successfully, now add entry fields
        ti->addTraceField(regdTrace, "Connection ", 0,
                          ExeTrace::TR_POINTER32);
        ti->addTraceField(regdTrace, "BufferAddr   ", 1, ExeTrace::TR_POINTER32);
        ti->addTraceField(regdTrace, "Length ", 2, ExeTrace::TR_INT32);
        ti->addTraceField(regdTrace, "Type", 3, ExeTrace::TR_INT32);
        ti->addTraceField(regdTrace, "Last   ", 4, ExeTrace::TR_CHAR);
        ti->addTraceField(regdTrace, "SeqNum", 5, ExeTrace::TR_INT32);
        ipcMsgTraceRef_ = regdTrace;
      }

      // IPC connection trace
      allConnections_->registTraceInfo(this, ti);
    }
  }
}

Int32 IpcEnvironment::printAnIpcEntry(Int32 lineno, char *buf)
{
  if (lineno >= maxIpcMsgTraceIndex_)
    return 0;
  Int32 rv;
  rv = sprintf(buf, "%.4d  %8p  %8p  %8d  %.4s  %3d %10d\n",
               lineno,
               ipcMsgTraceArea_[lineno].conn_,
               ipcMsgTraceArea_[lineno].bufAddr_,
               ipcMsgTraceArea_[lineno].length_,
               IpcMsgOperName[ipcMsgTraceArea_[lineno].sendOrReceive_],
               ipcMsgTraceArea_[lineno].isLast_,
               ipcMsgTraceArea_[lineno].seqNum_);
  return rv;
}

char const *IpcEnvironment::myProcessName()
{
  if (myProcessName_[0] == '\0')
    if (getCliGlobals())
      strcpy(myProcessName_, getCliGlobals()->myProcessNameString());
    else {
      NAProcessHandle myPhandle;
      myPhandle.getmine();
      myPhandle.decompose();
      strcpy(myProcessName_, myPhandle.getPhandleString());
      return myProcessName_;
    }
  return myProcessName_;
 
}

void IpcAllocateDiagsArea(ComDiagsArea *&diags, CollHeap *diagsHeap)
{
  if ( NOT diags)
    {
      // diags does not point to an allocated diags area yet, allocate one
      diags = ComDiagsArea::allocate(diagsHeap);

      // catch the case where we can't even allocate the diags area
      if (NOT diags)
	ABORT("unable to allocate diagnostics area for IPC error");
    }
}

// -----------------------------------------------------------------------
// Global operator new with the placement form where an IPC environment
// is specified
// -----------------------------------------------------------------------
void * operator new(size_t size, IpcEnvironment *env)
{
  return env->getHeap()->allocateMemory(size);
}

void * operator new[](size_t size, IpcEnvironment *env)
{
  return env->getHeap()->allocateMemory(size);
}

char *getServerProcessName(IpcServerType serverType,
                           short cpuNum, char *processName, short *envType)
{
  const char *processPrefix = NULL;
  char serverNodeName[MAX_SEGMENT_NAME_LEN+1];
  short len;

   if (processPrefix == NULL)
   {
       switch (serverType)
       {
         case IPC_SQLSSCP_SERVER:
           processPrefix = SSCP_PROCESS_PREFIX;
           break;
         case IPC_SQLSSMP_SERVER:
           processPrefix = SSMP_PROCESS_PREFIX;
           break;
         case IPC_SQLQMS_SERVER:
           processPrefix = QMS_PROCESS_PREFIX;
           break;
         case IPC_SQLQMP_SERVER:
           processPrefix = QMP_PROCESS_PREFIX;
           break;
         case IPC_SQLQMM_SERVER:
           processPrefix = QMM_PROCESS_PREFIX;
           break;
         default:
           return NULL;
       }
   }
   str_sprintf(processName, "%s%d", processPrefix, cpuNum);
  return processName;
}

void IpcAwaitiox::DoAwaitiox(NABoolean ignoreLrec)
{
  if (!completed_)
  {
    fileNum_ = ignoreLrec ? -2 : -1; // Reminder: change to -1
    bufAddr_ = 0;
    count_ = retCode_ = lastError_ = 0;
    tag_ = 0;
    if (ignoreLrec)
       condCode_ = BAWAITIOXTS(&fileNum_, &bufAddr_, &count_, &tag_, 0);
    else
       condCode_ = BAWAITIOX(&fileNum_, &bufAddr_, &count_, &tag_, 0);
    if (fileNum_ == -2)
      fileNum_ = -1;
    completed_ = TRUE;
    if (condCode_ != 0) // not successful completion
    {
      retCode_ = BFILE_GETINFO_(fileNum_, &lastError_);
      if (retCode_ == 0 && lastError_ == 40)
      {
	fileNum_ = -1;
	completed_ = FALSE;
      }
    }
    retryCount_ = 0;
  }
  else
  {
    retryCount_ += 1;
    bool loop = false;
    char *envvarPtr;
    if (retryCount_ >= 10)
    {
      envvarPtr = getenv("IPC_LOOP_ON_UNEXPECTED_COMPLETION");
      if (envvarPtr && atoi(envvarPtr) == 1)
        loop = true;
    }
    while (retryCount_ >= 10  && loop)
    {
      sleep(10);
    }
    assert(retryCount_ < 10);
  }
}

Int32 IpcAwaitiox::ActOnAwaitiox(void **bufAddr, Int32 *count, SB_Tag_Type *tag)
{
  *bufAddr = bufAddr_;
  *count = count_;
  *tag = tag_;
  completed_ = FALSE;
  return condCode_;
}

// These operator delete functions will be called if initialization throws an
// exception. They remove a compiler warning with the .NET 2003 compiler.
void IpcMessageBuffer::operator delete(void *p,
                                       IpcMessageObjSize bufferLength,
                                       CollHeap *heap, NABoolean bIgnore)
{
  if (heap)
    heap->deallocateMemory(p);
}

void IpcAllConnections::printConnTraceLine(char *buffer, int *rsp_len, IpcConnection *conn)
{
      Int32 lineLen;
      Int32 cpu, node, pin;
      SB_Int64_Type seqNum = -1;
      IpcNetworkDomain domain;
      cpu = pin;
      domain = conn->getOtherEnd().getDomain();

      if (domain == IPC_DOM_GUA_PHANDLE)
      {
        GuaProcessHandle *otherEnd = (GuaProcessHandle *)&(conn->getOtherEnd().getPhandle().phandle_);
        if (otherEnd)
          otherEnd->decompose(cpu, pin, node
                             , seqNum
                             );
      }

      if (!memcmp(conn->getEyeCatcher(), "STBL", 4))
      {
      }
      else if (conn->castToSMConnection())
      {
        cpu = ((SMConnection *)conn)->getSMTarget().node;
        pin = ((SMConnection *)conn)->getSMTarget().pid;
        sm_id_t smQueryId = ((SMConnection *)conn)->getSMTarget().id;
        Int32 smTag = ((SMConnection *)conn)->getSMTarget().tag;
        *rsp_len = sprintf(buffer + *rsp_len,
                           "%.4s %s %.3d,%.8d,%.8" PRId64 
                           ",%.8" PRId64 ",%.8d\n",
                           conn->getEyeCatcher(),
                           IpcConnStateName[conn->getState()],
                           cpu, pin, seqNum, smQueryId, smTag);
      }
      else
      {
        *rsp_len  = sprintf(buffer, "%.4s %s %.3d,%.8d,%.8" PRId64 "\n",
                          conn->getEyeCatcher(), 
                          IpcConnStateName[conn->getState()], 
                          cpu, pin, seqNum);

      }
}

void IpcAllConnections::infoAllConnections(char *buffer, int max_len, int *rsp_len)
{
  CollIndex i, usedLength = getUsedLength();
  IpcConnection *conn;
  enum { MAX_LINE = 250 }; //  Currently printConnTraceLine sprintf's about 80 characters
  char local_buffer[MAX_LINE];
  int  line_len = 0;
  for (i = 0; i < usedLength; i++)
  {
    if (getUsage(i) != UNUSED_COLL_ENTRY)
    {
      conn = usedEntry(i);
      local_buffer[0] = '\0';
      line_len = 0;
      printConnTraceLine(local_buffer, &line_len, conn);
      if ( (*rsp_len + line_len + 1) <= max_len)
      {
         strncpy( (buffer + *rsp_len), local_buffer, (size_t)line_len );
         *rsp_len += line_len;
      }
      else
      {
         sprintf( (buffer + *rsp_len - 16 ),"\nOUT OF BUFFER!\n");
         return;
      }
    }
  }
  *(buffer + *rsp_len) = '\n';
  *rsp_len += 1;
}

void IpcSetOfConnections::infoPendingConnections(char *buffer, int max_len, int *rsp_len)
{
  IpcAllConnections *allConns;
  IpcConnection *conn;
  enum { MAX_LINE = 250 };  // Currently printConnTraceLine sprintf's about 80 characters
  char local_buffer[MAX_LINE];
  int line_len = 0;
  CollIndex i, firstConnIndex = 0;
  if (!setToNext(firstConnIndex))
    return;
  else
  {
    conn = element(firstConnIndex);
    allConns = conn->getEnvironment()->getAllConnections();
    local_buffer[0] = '\0';
    line_len = 0;
    allConns->printConnTraceLine(local_buffer, &line_len, conn);
    if ( (*rsp_len + line_len + 1) <= max_len )
    {
       strncpy( (buffer + *rsp_len), local_buffer, line_len);
       *rsp_len += line_len;
    }
    else
    {
       sprintf( (buffer + *rsp_len - 16), "\nOUT OF BUFFER!\n");
       return;
    }
  }
  for (i = firstConnIndex + 1; setToNext(i); i++)
  {
    conn = element(i);
    local_buffer[0] = '\0';
    line_len = 0;
    allConns->printConnTraceLine(local_buffer, &line_len, conn);
    if ( (*rsp_len + line_len + 1) <= max_len )
    {
       strncpy( (buffer + *rsp_len), local_buffer, line_len);
       *rsp_len += line_len;
    }
    else
    {
       sprintf( (buffer + *rsp_len - 16), "\nOUT OF BUFFER!\n");
       return;
    }
  }
  *(buffer + *rsp_len) = '\n';
  *rsp_len += 1;
}

void operator delete(void *p, IpcEnvironment *env)
{
  env->getHeap()->deallocateMemory(p);
}
