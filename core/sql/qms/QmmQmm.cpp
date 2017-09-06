// **********************************************************************
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
// **********************************************************************

#include "QmmQmm.h"
#include "QueryRewriteServer.h"
#include "Ipc.h"
#include "ComCextdecs.h"
#include "ComRtUtils.h"
#include "PortProcessCalls.h"

#ifdef NA_NSK
extern "C" 
{
  #include "cextdecs.h(NODENUMBER_TO_NODENAME_,    \
                       REMOTEPROCESSORSTATUS,      \
                       PROCESSHANDLE_DECOMPOSE_,   \
                       PROCESSSTRING_SCAN_,        \
                       PROCESS_SPAWN_,             \
                       FILENAME_RESOLVE_,          \
                       FILENAME_TO_PATHNAME_)"
}

#else
#include "cextdecs/cextdecs.h"
#endif

using namespace QR;

IpcServerClass* QmpStub::qmpServerClass_ = NULL;
IpcServerClass* QmsStub::qmsServerClass_ = NULL;

void QmmGuaReceiveControlConnection::actOnSystemMessage
                                       (short messageNum,
                                        IpcMessageBufferPtr sysMsg,
                                        IpcMessageObjSize sysMsgLen,
                                        short clientFileNumber,
                                        const GuaProcessHandle& clientPhandle,
                                        GuaConnectionToClient* connection)
{
// @ZXros -- these declns go with ifdef'ed code below
#ifdef NA_NSK
  short result;
  short cpu;
  short pin;
  Int32 segmentNumber;
#endif
  switch (messageNum)
    {
      case ZSYS_VAL_SMSG_OPEN:
        {
          NAString qmmName("qmm");
          QmmMessageStream* msgStream =
                  new QmmMessageStream(const_cast<IpcEnvironment*>
                                            (qmm_->getEnvironment()),
                                       qmmName, qmm_);
          msgStream->addRecipient(connection);
          //connection->receive(msgStream);
          msgStream->receive(FALSE);
        }
        break;

      case ZSYS_VAL_SMSG_CLOSE:
      case ZSYS_VAL_SMSG_PROCDEATH:
        qmm_->handleClientExit((short *)&(clientPhandle.phandle_), messageNum);
        if (connection)
          connection->setFatalError(NULL);  //@ZX
        break;

      case ZSYS_VAL_SMSG_CPUDOWN:
        {
          zsys_ddl_smsg_cpudown_def* msg = (zsys_ddl_smsg_cpudown_def*)sysMsg;
          QRLogger::log(CAT_QR_IPC, LL_INFO,
            "Cpu %d on qmm's segment has gone down.", msg->z_cpunumber);
          qmm_->getQmsStub(1, msg->z_cpunumber)
              ->disable(QmsStub::CPU_NOT_REACHABLE);
        }
        break;

      case ZSYS_VAL_SMSG_CPUUP:
        {
          zsys_ddl_smsg_cpuup_def* msg = (zsys_ddl_smsg_cpuup_def*)sysMsg;
          QRLogger::log(CAT_QR_IPC, LL_INFO,
            "Cpu %d on qmm's segment has come back up.", msg->z_cpunumber);
          //qmm_->getQmsStub(1, msg->z_cpunumber)->start();
          qmm_->getQmsStub(1, msg->z_cpunumber)->scheduleRestart();
        }
        break;

      case ZSYS_VAL_SMSG_REMOTECPUDOWN:
        {
          zsys_ddl_smsg_remotecpudown_def* msg = 
                  (zsys_ddl_smsg_remotecpudown_def*)sysMsg;
          QRLogger::log(CAT_QR_IPC, LL_INFO,
            "Cpu %d on segment %d has gone down.", msg->z_cpunumber, msg->z_nodenumber);

          qmm_->getQmsStub(msg->z_nodenumber, msg->z_cpunumber)
              ->disable(QmsStub::CPU_NOT_REACHABLE);
        }
        break;

      case ZSYS_VAL_SMSG_REMOTECPUUP:
        {
          zsys_ddl_smsg_remotecpuup_def* msg =
                  (zsys_ddl_smsg_remotecpuup_def*)sysMsg;
          QRLogger::log(CAT_QR_IPC, LL_INFO,
            "Cpu %d on segment %d has come back up.", msg->z_cpunumber, msg->z_nodenumber);
          //qmm_->getQmsStub(msg->z_nodenumber, msg->z_cpunumber)->start();
          qmm_->getQmsStub(msg->z_nodenumber, msg->z_cpunumber)->scheduleRestart();
        }
        break;

      case ZSYS_VAL_SMSG_NODEDOWN:
        {
          zsys_ddl_smsg_nodedown_def* msg = (zsys_ddl_smsg_nodedown_def*)sysMsg;
          QRLogger::log(CAT_QR_IPC, LL_INFO,
            "Segment %d has gone down.", msg->z_nodenumber);
          for (short cpu = 0; cpu < CPUS_PER_SEGMENT; cpu++)
            qmm_->getQmsStub(msg->z_nodenumber, cpu)
                ->disable(QmsStub::SEGMENT_NOT_REACHABLE);
        }
        break;

      case ZSYS_VAL_SMSG_NODEUP:
        {
          zsys_ddl_smsg_nodeup_def* msg = (zsys_ddl_smsg_nodeup_def*)sysMsg;
          QRLogger::log(CAT_QR_IPC, LL_INFO,
            "Segment %d has come back up.", msg->z_nodenumber);
          for (short cpu = 0; cpu < CPUS_PER_SEGMENT; cpu++)
            //qmm_->getQmsStub(msg->z_nodenumber, cpu)->start();
            qmm_->getQmsStub(msg->z_nodenumber, cpu)->scheduleRestart();
        }
        break;

      default:
        break;
    }

  // See if we have any processes due to be restarted.
  QRProcessStub::checkRestarts();
}

// Static singleton instance of the class.
Qmm* Qmm::instance_ = NULL;

Qmm::Qmm(CollHeap* heap)
  : qmsPool_(NULL),
    qmsCount_(0),
    qmp_(NULL),
    heap_(heap)
{
  // Establish the IPC heap and cache the IpcEnvironment ptr from
  // MvQueryRewriteServer.
  MvQueryRewriteServer::setHeap(heap);
  ipcEnv_ = MvQueryRewriteServer::getIpcEnv();

  // Set static IpcServerClass for QmsStub to use when instantiating qms
  // processes.
  QmsStub::setQmsServerClass(new(heap_) IpcServerClass(ipcEnv_,
                                                       IPC_SQLQMS_SERVER,
                                                       IPC_SPAWN_OSS_PROCESS));
}

void Qmm::handleClientExit(const short* phandle, short messageNum)
{
  short cpu, pin;
  Int32 segmentNumber;
  short result = 0;
  NABoolean qmpDied = FALSE;

  Int32 lc_pin;
  Int32 lc_cpu;
  Int32 lc_seg;

  result = XPROCESSHANDLE_DECOMPOSE_ ((SB_Phandle_Type *)phandle, &lc_cpu, &lc_pin, &segmentNumber);
  cpu = lc_cpu;
  pin = lc_pin;

  QRLogger::log(CAT_QR_IPC, LL_INFO,
        "dead process is on cpu <%d> and pin <%d>", cpu, pin);


  if (qmp_ && *qmp_ == *(SB_Phandle_Type *)phandle)
  {
     qmpDied = TRUE;
  }

  if (!qmpDied)
    {
      // When a qms dies we are delivered a null connection pointer and handle.
      // When this happens, we check the status of our qms processes until we find
      // one that's no longer alive. If >1 close messages arrive close together,
      // we may not get the same one referenced by the message, but that one will
      // be restarted in response to a subsequent message.
      NABoolean found = FALSE;
      QmsStub* qmsStub;
      for (Int32 i=0; i<qmsCount_ && !found; i++)
        {
          qmsStub = qmsPool_[i];
          NABoolean processDoesNotExist = FALSE;

          char procName[200];
          short result = -1;
       
          SB_Phandle_Type procHandle = qmsStub->getProcessHandle(); 
          NAProcessHandle processHandle(&procHandle);
          Int32 guaRetcode = processHandle.decompose();
          if (!guaRetcode)
          {
            memset(procName, 0, sizeof(procName));
            memcpy(procName, processHandle.getPhandleString(), processHandle.getPhandleStringLen());
            result = msg_mon_get_process_info (procName, &lc_cpu, &lc_pin);
          }

          if (result || (lc_cpu < 0) || (lc_pin < 0))
            processDoesNotExist = TRUE;

          if (qmsStub->getStatus() == QmsStub::RUNNING && processDoesNotExist)
            {
              found = TRUE;
              QRLogger::log(CAT_QR_IPC, LL_FATAL,
                "*** QMS on cpu %d of segment %d has died, "
                          "attempting to restart...",
                          qmsStub->getCpuNumber(), qmsStub->getSegmentNumber());
              qmsStub->disable(QmsStub::NOT_RUNNING);
              qmsStub->scheduleRestart();
            }
        }

      if (!found)
        {
          if (messageNum == ZSYS_VAL_SMSG_CLOSE)
            QRLogger::log(CAT_QR_IPC, LL_INFO,
              "*** Unknown client process has ended. ***");
          else if (messageNum == ZSYS_VAL_SMSG_PROCDEATH)
            QRLogger::log(CAT_QR_IPC, LL_INFO,
              "*** Unknown child process has died. ***");
          else
            QRLogger::log(CAT_QR_IPC, LL_INFO,
              "*** Unknown process has terminated, message=%d. ***", messageNum);
        }

      return;
    }

  if (messageNum == ZSYS_VAL_SMSG_CLOSE)
    QRLogger::log(CAT_QR_IPC, LL_INFO,
      "*** CLOSE message received for process %d,%d on segment %d ***",
      cpu, pin, segmentNumber);
  else if (messageNum == ZSYS_VAL_SMSG_PROCDEATH)
    QRLogger::log(CAT_QR_IPC, LL_INFO,
      "*** PROCDEATH message received for process %d,%d on segment %d ***",
       cpu, pin, segmentNumber);
  else
    QRLogger::log(CAT_QR_IPC, LL_INFO,
      "*** Process %d,%d on segment %d has terminated, message=%d. ***",
                cpu, pin, segmentNumber, messageNum);

  if (qmp_ && *qmp_ == *(SB_Phandle_Type *)phandle)
    {
      QRLogger::log(CAT_QR_IPC, LL_INFO,
        "*** QMP has died, will attempt to restart...");
      qmp_->scheduleRestart();
    }
  else if (*getQmsStub(segmentNumber, cpu) == *(SB_Phandle_Type *)phandle)
    {
      // This probably never gets executed; process handle usually delivered
      // for a qms termination is null, and is handled in loop above that looks
      // for one that is no longer running.
      QRLogger::log(CAT_QR_IPC, LL_ERROR,
        "*** QMS on cpu %d of segment %d has died, "
                  "will attempt to restart...",
                  cpu, segmentNumber);
      getQmsStub(segmentNumber, cpu)->disable(QmsStub::NOT_RUNNING);
      getQmsStub(segmentNumber, cpu)->scheduleRestart();
    }
  else
    QRLogger::log(CAT_QR_IPC, LL_WARN,
      "Could not identify terminated process as either QMP or QMS");
}

// Only used for Windows.
void Qmm::allocateQms()
{
  char segmentName[SEGMENT_NAME_LEN + 1];
  short segmentNameLen = 0;
  short result = NODENUMBER_TO_NODENAME_(-1, segmentName, SEGMENT_NAME_LEN,
                                         &segmentNameLen);
  segmentName[segmentNameLen] = '\0';
  QRLogger::log(CAT_QR_IPC, LL_DEBUG,
    "Result for caller's segment is %d, segment name is %s",
              result, segmentName);
  qmsCount_ = 1;
  qmsPool_ = new(heap_) QmsStub*[qmsCount_];
  qmsPool_[0] = new(heap_) QmsStub(1, segmentName, 0, 0, TRUE, TRUE,
                                   /*qmsMsgStream_,*/ ipcEnv_, heap_);
}

void Qmm::allocateQmsPool()
{
  short qmsInx = 0;
  Int32 maxCpuNum = 0;
  Int32 lv_ret = 0;
  Int32 nodeCount = 0;
  Int32 nodeMax = 0;
  MS_Mon_Node_Info_Entry_Type *nodeInfo = NULL;

  // Get the number of nodes to know how much info space to allocate
  lv_ret = msg_mon_get_node_info(&nodeCount, 0, NULL);
  if ((lv_ret == 0) && (nodeCount > 0))
  {
     // Allocate the space for node info entries
     nodeInfo = (MS_Mon_Node_Info_Entry_Type *) new(heap_)
                  char[nodeCount * sizeof(MS_Mon_Node_Info_Entry_Type)];
     if (nodeInfo)
     {
        // Get the node info
        memset(nodeInfo, 0, sizeof(nodeInfo));
        nodeMax = nodeCount;
        lv_ret = msg_mon_get_node_info(&nodeCount, nodeMax, nodeInfo);
        if (lv_ret == 0)
        {
          // Find number of storage nodes by checking the storage bit.
          // The computed value is used as node Id where QMSs will be created.
          // QMSs should be running on the storage nodes only.
          for ( Int32 i=0; i<nodeCount; i++ ) {
            if (( nodeInfo[i].type & MS_Mon_ZoneType_Storage ) &&
                ( !nodeInfo[i].spare_node ))
              maxCpuNum++;
          }
        }
        NADELETEBASIC(nodeInfo,heap_);
     }
  }

  qmsCount_ = maxCpuNum;
  qmsPool_ = new(heap_) QmsStub*[qmsCount_];

  // Create a qms stub for each possible cpu in the cluster
  for (short cpu = 0; cpu < maxCpuNum; cpu++)
  {
     qmsPool_[qmsInx++] = new(heap_) QmsStub(1, (char *)"\\NSK", cpu, 0, TRUE, TRUE,
                                             /*qmsMsgStream_,*/ ipcEnv_, heap_);
  }

}  // allocateQmsPool()

// Make sure all QMSs started, and if not, retry up to maxRetries times, each
// time invoking a delay of delaySeconds seconds prior to starting it on the
// first cpu in the array on which it isn't running. When QMM dies and comes
// back up, sometimes the old one's QMSs haven't received the notification and
// self-terminated before the new QMM tries to create mew Q<Ss, and a "duplicate
// process name" error results.
void Qmm::checkAndRetryQms(Int16 maxRetries, Int16 delaySeconds)
{
  NABoolean needRetry = TRUE;
  for (Int16 i=1; i<=maxRetries && needRetry; i++)
    {
      NABoolean allOK = TRUE;
      needRetry = FALSE;
      for (Int16 cpuInx=0; cpuInx<qmsCount_; cpuInx++)
        {
          if (qmsPool_[cpuInx]->getStatus() == QmsStub::NOT_RUNNING)
            {
              if (allOK)  // first one found that isn't running?
                {
                  allOK = FALSE;
                  DELAY(delaySeconds * 100);
                }
              if (!qmsPool_[cpuInx]->start())
                needRetry = TRUE;  // at least 1 qms didn't start
            }
        }
    }
}

void Qmm::startQmp(short cpu)
{
  if (qmp_)
    delete qmp_;
  qmp_ = new(heap_) QmpStub(*ipcEnv_, qmpStartOpt_, cpu, heap_);
}

NABoolean QmpStub::start()
{
  char* baseProcName = MvQueryRewriteServer::getProcessName(IPC_SQLQMP_SERVER, NULL, cpu_);

  SB_Phandle_Type *phandle;
  phandle = get_phandle_with_retry(baseProcName);

  if (phandle != NULL)
  { 
       QRLogger::log(CAT_QR_IPC, LL_INFO,
         "qmm found existing qmp process, not starting new one.");
    setProcessHandle(*phandle);
    return TRUE;
  }

  switch ((Int32)qmpStartOpt_)
    {
      case SPAWN:
        spawnProcess(ipcEnv_, cpu_);
        break;
      case SERVER:
        allocateProcess(ipcEnv_, cpu_);
        break;
      case NONE:
        // don't start qmp here; start manually
        break;
      default:
        assertLogAndThrow1(CAT_QR_IPC, LL_ERROR,
                           FALSE, QmmException,
                           "Unknown option for starting QMP -- %d",
                           qmpStartOpt_);
        break;
    }

  return TRUE;
}

void QmpStub::allocateProcess(IpcEnvironment& ipcEnv, short cpu)
{
  if (!qmpServerClass_)
    qmpServerClass_ = new(heap_) IpcServerClass(&ipcEnv,
                                                IPC_SQLQMP_SERVER,
                                                IPC_SPAWN_OSS_PROCESS);
  ComDiagsArea* diagsArea = NULL;
  IpcAllocateDiagsArea(diagsArea, heap_);

  // Have to supply name of segment (minus leading \, which gets prepended), or a
  // fault will occur while populating the diagnostics area if there is an error.
  char localSegmentName[SEGMENT_NAME_LEN + 1];
  short localSegmentNameLen;
  short result = NODENUMBER_TO_NODENAME_(-1,localSegmentName,
                                         SEGMENT_NAME_LEN,
                                         &localSegmentNameLen);
  assertLogAndThrow1(CAT_QR_IPC, LL_ERROR,
                     result==0, QmmException,
                     "Could not get name of local segment, error is %d", result);
  localSegmentName[localSegmentNameLen] = '\0';
  qmpServer_ = qmpServerClass_->allocateServerProcess(&diagsArea, heap_,
                                                      localSegmentName+1, cpu,
                                                      IPC_PRIORITY_DONT_CARE, 1,
                                                      TRUE, TRUE);
  if (qmpServer_)
    {
      
      QRLogger::log(CAT_QR_IPC, LL_DEBUG,
        "QMP process started on cpu #%d", qmpServer_->getServerId().getCpuNum());
      setProcessHandle(qmpServer_->getServerId().getPhandle().phandle_);
    }
  else
    {
      QRLogger::log(CAT_QR_IPC, LL_ERROR,
          "Failed to allocate server process for QMP on cpu %d ", cpu);
      QRLogger::logDiags(diagsArea, CAT_QR_IPC);
    }
}

//#pragma nowarn(770)   // warning elimination 
void QmpStub::spawnProcess(IpcEnvironment& ipcEnv, short cpu) //, ComDiagsArea **diags, CollHeap *diagsHeap)
{
   SB_Phandle_Type p_handle;
   const char* progFile = "tdm_arkqmp";

#define MAX_PROC_ARGS   10
#define SET_ARGV(argv,argc,argval) {argv[argc] = (char *) calloc(strlen(argval), 1); \
    strcpy(argv[argc++], argval); }

  Int32                   largc = 0;
  char                  *largv[MAX_PROC_ARGS];

  Int32 server_nid = cpu;
  Int32 server_pid = 0;
  Int32 server_oid = 0;
  //char process_name[100];
  char prog[MS_MON_MAX_PROCESS_PATH];

  SET_ARGV(largv, largc, progFile);
  SET_ARGV(largv, largc, "-oss");
  //SET_ARGV(largv, largc, NULL);

  strcpy(prog, getenv("TRAF_HOME"));
  strcat(prog, "/export/bin32/");
  strcat(prog, progFile);

  char* tmpprocess_name = MvQueryRewriteServer::getProcessName(IPC_SQLQMP_SERVER, NULL, cpu, heap_);

  char process_name[100];
  strcpy(process_name, tmpprocess_name);

  msg_mon_start_process(
                        prog,           /* prog */
                        process_name,   /* name */
                        process_name,   /* output process name */
                        largc,          /* args */
                        largv,
                        &p_handle,
                        0,              /* open */
                        &server_oid,    /* oid */
                        MS_ProcessType_Generic, /* process type */
                        0,              /* priority */
                        0,              /* debug */
                        0,              /* backup */
                        &server_nid,    /* nid */
                        &server_pid,
                        NULL,
                        NULL);   /* pid */

  setProcessHandle(p_handle);
}

void Qmm::relayPendingPubsToQms()
{
  QRXmlMessageObj* xmlMsgObj;
  for (CollIndex i=0; i<pendingPubs_.entries(); i++)
    {
      xmlMsgObj = pendingPubs_[i];
      QRLogger::log(CAT_QR_IPC, LL_DEBUG,
        "XML of publish message is:\n%s", xmlMsgObj->getData());
      for (Int32 qmsInx=0; qmsInx<qmsCount_; qmsInx++)
        {
          qmsPool_[qmsInx]->publish(xmlMsgObj);
        }
    }

  QRLogger::log(CAT_QR_IPC, LL_DEBUG,
    "Sent %d Publish requests to QMS list", pendingPubs_.entries());
  for (CollIndex i=0; i<pendingPubs_.entries(); i++)
    pendingPubs_[i]->decrRefCount();
  pendingPubs_.clear();
}

void Qmm::executeMessageLoop()
{
  // Have to allocate this from heap, because ~IpcEnvironment deletes it.
  QmmGuaReceiveControlConnection* conn = 
            new(heap_) QmmGuaReceiveControlConnection(ipcEnv_, this);
  ipcEnv_->setControlConnection(conn);
  NAString qmmName("qmm");
  QRMessageStream msgStream(ipcEnv_, qmmName);
  //QRMessageRequest request(msgStream);
  QRMessageObj* responseObj = NULL;

  while (!conn->getConnection())
    conn->wait(IpcInfiniteTimeout);
  IpcConnection* firstClient =  NULL;

  if (listenOpt_ == WAITONALL)
    {
      firstClient = conn->getConnection();
      msgStream.addRecipient(firstClient);
      while (TRUE)
        {
          IpcAllConnections* allConns = ipcEnv_->getAllConnections();
          allConns->waitOnAll();
        }
    }
  else if (listenOpt_ == WAITCC)
    {
      WaitReturnStatus waitStatus;
      while (TRUE)
        {
          //debugMessage1("IPC heap usage = %d", ipcEnv_.getHeap()->getAllocSize());
          waitStatus = conn->wait(getWaitTimeout());
          if (waitStatus == WAIT_OK)
            {
              QRProcessStub::checkRestarts();
            }
        }
    }
  else
    {
      assertLogAndThrow1(CAT_QR_IPC, LL_ERROR,
                         listenOpt_ == RECEIVE, QmmException,
                         "Unknown listen opt -- %d", listenOpt_);
      firstClient = conn->getConnection();
      msgStream.addRecipient(firstClient);

      while (TRUE)
        {
          msgStream.receive();
          //IpcAllConnections* allConns = ipcEnv_->getAllConnections();
          //allConns->waitOnAll();
          try
            {
              //responseObj = processRequestMessage(request);
              responseObj = processRequestMessage(&msgStream);
            }
          catch(...)
            {}

          // Either an exception was caught above, or a response was not generated
          // for some other reason (shouldn't happen). Either way, create a status
          // message indicating an internal error.
          if (!responseObj)
            responseObj = new QRStatusMessageObj(QR::InternalError);

          msgStream.clearAllObjects();
          msgStream.setType(responseObj->getType()); // so correct msg type logged
          msgStream << *responseObj;
          msgStream.send();
          responseObj->decrRefCount();
          relayPendingPubsToQms();
        }
    }
}

QRRequestResult Qmm::handlePublishRequest(QRMessageStream* msgStream)
{
  // Have to use the global heap because IpcMessageObj is not derived from
  // NABasicObject.
  QRXmlMessageObj* xmlMsgObj = new QRXmlMessageObj(NULL, PUBLISH_REQUEST);
  *msgStream >> *xmlMsgObj;
  pendingPubs_.insert(xmlMsgObj);
  return QR::Success;
}

QRRequestResult Qmm::handleAllocateRequest(QRMessageStream* msgStream)
{
  return QR::Unable;
}

//QRMessageObj* Qmm::processRequestMessage(QRMessageRequest& request)
QRMessageObj* Qmm::processRequestMessage(QRMessageStream* msgStream)
{
  QRRequestResult result;
  XMLFormattedString resultXML;
  QRMessageObj* responseMsgPtr = NULL;
  //QRMessageTypeEnum requestType;

  if (!msgStream->moreObjects())
    {
      QRLogger::log(CAT_QR_IPC, LL_ERROR,
        "QMM received an empty message stream.");
      return new QRStatusMessageObj(ProtocolError);
    }

  // @ZX: Although we allow the possibility of multiple request objects per
  //      message, the returned response object will indicate the status of
  //      only the last one. Should probably return a status for each request
  //      item (or at least delete the overwritten ones so they don't leak).
  // @ZX: Note that the message stream is cleared when we encounter an unknown
  //      or unhandled message type. How can an object be extracted from a stream
  //      if you don't understand its type?
  while (msgStream->moreObjects())
    {
      switch (msgStream->getNextObjType())
        {
          // Can't use heap_ for the allocation of the response message because
          // IpcMessageObj  is not derived from NABasicObject.

          case PUBLISH_REQUEST:
            result = handlePublishRequest(msgStream);
            if (result == QR::Success)
              QRLogger::log(CAT_QR_IPC, LL_INFO, "PUBLISH was successful.");
            else
              QRLogger::log(CAT_QR_IPC, LL_ERROR,
                "PUBLISH failed, status is %d", result);
            responseMsgPtr = new QRStatusMessageObj(result);
            break;

          case ALLOCATE_REQUEST:
            result = handleAllocateRequest(msgStream);
            if (result == QR::Success)
            QRLogger::log(CAT_QR_IPC, LL_INFO, "ALLOCATE was successful.");
            else
            QRLogger::log(CAT_QR_IPC, LL_ERROR,
              "ALLOCATE failed, status is %d", result);
            responseMsgPtr = new QRStatusMessageObj(result);
            break;

          case DEFAULTS_REQUEST:
            QRLogger::log(CAT_QR_IPC, LL_ERROR, 
              "'DEFAULTS' request not yet handled by QMM");
            responseMsgPtr = new QRStatusMessageObj(QR::InvalidRequest);
            msgStream->clearAllObjects();
            break;

          default:
            QRLogger::log(CAT_QR_IPC, LL_ERROR,
              "QMM received unexpected message type: %d", msgStream->getNextObjType());
            responseMsgPtr = new QRStatusMessageObj(QR::InvalidRequest);
            msgStream->clearAllObjects();
            break;
        }
    }

  return responseMsgPtr;
} // processRequestMessage

IpcTimeout Qmm::getWaitTimeout()
{
  const NAList<QRProcessStub*> restartList = QRProcessStub::getRestartList();
  CollIndex restartListEntries = restartList.entries();
  if (restartListEntries == 0)
    return IpcInfiniteTimeout;

  // Find the time of the next scheduled restart. Init to first entry in list;
  // we exited above is list was empty.
  Int64 earliestTimestamp = restartList[0]->getLockoutEndTS();
  for (CollIndex i=1; i<restartListEntries; i++)
    {
      if (restartList[i]->getLockoutEndTS() < earliestTimestamp)
        earliestTimestamp = restartList[i]->getLockoutEndTS();
    }

  // Timestamp is microsecond resolution, IpcTimeout is in 10ms units (100 =
  // 1 second).
  Int64 microsecondsTillNext = earliestTimestamp - NA_JulianTimestamp()
                                                 + 1000000;  // 1-sec fudge factor
  if (microsecondsTillNext < 0)  // already past time somehow?
    microsecondsTillNext = 0;
  QRLogger::log(CAT_QR_IPC, LL_DEBUG,
    "Wait timeout set to %d seconds", microsecondsTillNext/1000000);
  return (IpcTimeout)(microsecondsTillNext / 1000);
}


NAList<QRProcessStub*> QRProcessStub::restartList_((CollHeap*)NULL);

QRProcessStub::QRProcessStub(CollHeap* heap)
  : lockoutEndTS_(0),
    retryNumber_(0),
    heap_(heap)
{
  // Initialize the process handle to its null representation, all -1s.
  nullProcessHandle();
}

void QRProcessStub::setLockout()
{
  static Int32 seconds[] = {0, 10, 20, 30, 60, 120};
  Int32 lockoutSeconds = (retryNumber_ >= (sizeof seconds / sizeof seconds[0]))
                            ? 180
                            : seconds[retryNumber_];
  QRLogger::log(CAT_QR_IPC, LL_DEBUG,
    "Will not attempt another restart of this process for %d seconds.",
              lockoutSeconds);
  lockoutEndTS_ = NA_JulianTimestamp() + (lockoutSeconds * 1000000);
}

void QRProcessStub::scheduleRestart()
{
  if (lockoutEndTS_ > NA_JulianTimestamp())
    {
      // Not time yet, add to restart list. When restarted and removed from
      // restart list, will enter another (longer) lockout period.
      restartList_.insert(this);
    }
  else
    {
      // Either there was no lockout or it has expired. Restart process, set
      // retry count to 1. Only when a process dies again when it is in its
      // lockout period do we queue the restart and extend the next lockout.
      // If the restart fails, increment the retry count and put the stub in
      // the restart list.
      if (start())
        retryNumber_ = 1;
      else
        {
          retryNumber_++;
          restartList_.insert(this);
        }
      setLockout();
    }
}

void QRProcessStub::checkRestarts()
{
  // Go through the list backwards so indexing is not affected by removal of
  // elements. CollIndex is an unsigned type, so have to start at entries()
  // (1 past last valid index) instead of entries()-1, so we don't try to assign
  // it a negative value in the case of an empty list.
  QRProcessStub* processStub;
  for (CollIndex i=restartList_.entries(); i>0; i--)
    {
      processStub = restartList_[i-1];
      if (processStub->lockoutEndTS_ < NA_JulianTimestamp())
        {
          // If the process starts successfully, remove it from the list, else
          // leave it there to try again next time. In either case, bump up the
          // lockout period before the next time it can be restarted.
          if (processStub->start())
            restartList_.removeAt(i-1);
          processStub->retryNumber_++;
          processStub->setLockout();
        }
    }
}

void QRProcessStub::nullProcessHandle()
{
}

QmsStub::QmsStub(short segmentNumber, char* segmentName, short cpuNumber,
                 short segmentStatus, NABoolean cpuExists,
                 NABoolean cpuReachable, // QRMessageStream* qmsMsgStream,
                 IpcEnvironment* ipcEnv,
                 CollHeap* heap)
  : QRProcessStub(heap),
    segmentNumber_(segmentNumber),
    cpuNumber_(cpuNumber),
    segmentStatus_(segmentStatus),
    qmsServer_(NULL),
    status_(UNINITIALIZED),
    //qmsMsgStream_(qmsMsgStream)
    qmsMsgStream_(new(heap_) QRMessageStream(ipcEnv, "qmm", heap, PUBLISH_REQUEST))
{
  strcpy(segmentName_, segmentName);

  if (!cpuExists)
    {
      status_ = CPU_NOT_PRESENT;
      return;
    }
  else if (!cpuReachable)
    {
      status_ = CPU_NOT_REACHABLE;
      QRLogger::log(CAT_QR_IPC, LL_DEBUG,
        "Processor %d on segment %s is not reachable.", cpuNumber, segmentName);
      return;
    }
  else if (segmentStatus != 0)
    {
      status_ = SEGMENT_NOT_REACHABLE;  // Segment status logged by caller
      return;
    }

  #ifndef NA_WINNT
     qmsProcessName_ = MvQueryRewriteServer::getProcessName(IPC_SQLQMS_SERVER, segmentName_, cpuNumber, heap_);
  #else
     qmsProcessName_ = new(heap_) char[PROCESSNAME_STRING_LEN];
     sprintf(qmsProcessName_, "%s.%s%02d", segmentName_, QMS_PROCESS_PREFIX, cpuNumber);
  #endif

  // Launch the QMS process.
  start();
}

NABoolean QmsStub::start()
{
  // Qmm class should have called static function to set this.
  assertLogAndThrow(CAT_QR_IPC, LL_ERROR,
                    qmsServerClass_, QmmException, "qmsServerClass_ is NULL");

  // Nodeup message is accompanied by a remote cpu up message for one processor,
  // so there will be a redundant attempt to start qms on that cpu a 2nd time.
  if (qmsServer_)
    {
      QRLogger::log(CAT_QR_IPC, LL_DEBUG,
        "QmsStub::start() -- QMS process already running on cpu %d "
                    "of segment %s", cpuNumber_, segmentName_);
      if (status_ == RUNNING)
        {
          QRLogger::log(CAT_QR_IPC, LL_INFO, "-- will not restart QMS process.");
          return TRUE;
        }
      else
        {
          // Exists, but status not what we want. Get rid of it, new one will
          // be started below.
          QRLogger::log(CAT_QR_IPC, LL_DEBUG, 
            "-- QMS process has status %d, will restart.", status_);
          disable(NOT_RUNNING);
        }
    }
  
  ComDiagsArea* diagsArea = NULL;
  IpcAllocateDiagsArea(diagsArea, heap_);

  // Pass segmentName_+1 because IpcGuardianServer::spawnProcess() assumes the
  // node name does not include the leading \, and adds one.

  qmsServer_ = qmsServerClass_->allocateServerProcess
                                    (&diagsArea, heap_, segmentName_+1,
                                     cpuNumber_, IPC_PRIORITY_DONT_CARE, 1,
                                     TRUE, TRUE, 2, NULL,
                                     qmsProcessName_);

  if (qmsServer_)
    {
      qmsMsgStream_->addRecipient(qmsServer_->getControlConnection());
      status_ = RUNNING;

      setProcessHandle(qmsServer_->getServerId().getPhandle().phandle_);
      QRLogger::log(CAT_QR_IPC, LL_DEBUG,
        "QMS process started on cpu #%d", qmsServer_->getServerId().getCpuNum());
      MvQueryRewriteServer::initQms(qmsServer_, heap_);
      return TRUE;
    }
  else
    {
      status_ = NOT_RUNNING;
      QRLogger::log(CAT_QR_IPC, LL_ERROR,
          "Failed to allocate server process for QMS on cpu %d of segment %s",
          cpuNumber_, segmentName_);
      QRLogger::logDiags(diagsArea, CAT_QR_IPC);
      return FALSE;
    }
}

void QmsStub::disable(Status reason)
{
  setStatus(reason);

  // This stub may not have an active process. For example, a segment down will
  // cause this to be called for each stub, including processors not present in
  // the segment (e.g., a segment with only 8 instead of 16 processors).
  if (qmsServer_)
    {
      qmsMsgStream_->deleteRecipient(qmsServer_->getControlConnection());
      qmsServer_->release();
      qmsServer_ = NULL;
      nullProcessHandle();
    }
}

void QmsStub::publish(QRXmlMessageObj* xmlMsgObj)
{
  if (!qmsServer_)
    return;

  qmsMsgStream_->clearAllObjects();
  qmsMsgStream_->setType(PUBLISH_REQUEST);
  *qmsMsgStream_ << *xmlMsgObj;
  qmsMsgStream_->send();
}

//void QmmMessageStream::actOnSend(IpcConnection* connection)
//{
//}

void QmmMessageStream::actOnReceive(IpcConnection* connection)
{
  QRLogger::log(CAT_QR_IPC, LL_INFO, "Reached QmmMessageStream::actOnReceive()");
  QRMessageObj* responseObj = NULL;
  try
    {
      responseObj = qmm_->processRequestMessage(this);
    }
  catch(...)
    {}

  QRMessageStream::actOnReceive(connection);
  respond(responseObj);

  qmm_->relayPendingPubsToQms();

  // See if we have any processes due to be restarted.
  QRProcessStub::checkRestarts();
}
