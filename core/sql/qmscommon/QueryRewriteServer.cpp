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
 **************************************************************************
 *
 * File:         QueryRewriteServer.cpp
 * Description:  MvQueryRewriteServer methods
 * Created:      06/01/2009
 * Language:     C++
 *
 **************************************************************************
 */

#include <ComCextdecs.h>
#include "QueryRewriteServer.h"
#include "QRMessage.h"
#include "QRIpc.h"
#include "ComRtUtils.h"
#include "PortProcessCalls.h"

#include "cextdecs/cextdecs.h"

#include "seabed/ms.h"

using namespace QR;

static const short SEGMENT_NAME_LEN = 8;

CollHeap* MvQueryRewriteServer::heap_ = NULL;
NABoolean MvQueryRewriteServer::heapHasBeenSet_ = FALSE;
NAString MvQueryRewriteServer::fileNamePrefix_("w:\\qms\\debug\\mvqr");
IpcEnvironment* MvQueryRewriteServer::ipcEnv_ = NULL;

IpcServerClass* MvQueryRewriteServer::qmsServerClass_ = NULL;
IpcServer* MvQueryRewriteServer::qmsServer_ = NULL;

IpcServerClass* MvQueryRewriteServer::qmmServerClass_ = NULL;
IpcServer* MvQueryRewriteServer::qmmServer_ = NULL;

IpcServerClass* MvQueryRewriteServer::qmpServerClass_ = NULL;
IpcServer* MvQueryRewriteServer::qmpServer_ = NULL;

IpcEnvironment* MvQueryRewriteServer::getIpcEnv()
{
  // Allocate static IpcEnvironment on first use. Can't use static object
  // instead of pointer, because on seaquest, the IpcEnvironment ctor makes
  // a system call before the seabed thread is available.
  if (!ipcEnv_)
    {
      if (!heapHasBeenSet_)
        QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_INFO,
          "Heap has not been specified to MvQueryRewriteServer, "
                   "IpcEnvironment being allocated on system heap.");
      ipcEnv_ = new(heap_) IpcEnvironment(heap_, NULL, FALSE,
                                          IPC_CLIENT_OR_UNSPECIFIED_SERVER,
                                          TRUE);
    }

  return ipcEnv_;
}

NABoolean MvQueryRewriteServer::processExists(const short* processHandle)
{
  char procName[200];
  Int32 nid = 0;
  Int32 pid = 0;
  short result = 0;

  NAProcessHandle phandle((SB_Phandle_Type *)processHandle);
  Int32 guaRetcode = phandle.decompose();
  if (!guaRetcode)
  {
    memset(procName, 0, sizeof(procName));
    memcpy(procName, phandle.getPhandleString(), phandle.getPhandleStringLen());
    result = msg_mon_get_process_info (procName, &nid, &pid);
  }
  else
    result = guaRetcode;
  return (result == 0);
}


char* MvQueryRewriteServer::getProcessName(IpcServerType serverType,
                                           char* nodeName, /* not used for LINUX */
                                           short cpu,
                                           CollHeap* heap)
{
  const char *overridingDefineName;
  const char *actualPrefix;
  char *period;

  switch (serverType)
    {
      case IPC_SQLQMS_SERVER:
        overridingDefineName = "_MX_QMS_PROCESS_PREFIX";
        break;
      case IPC_SQLQMP_SERVER:
        overridingDefineName = "_MX_QMP_PROCESS_PREFIX";
        break;
      case IPC_SQLQMM_SERVER:
        overridingDefineName = "_MX_QMM_PROCESS_PREFIX";
        break;
      default:
        assertLogAndThrow1(CAT_SQL_COMP_QR_IPC, LL_ERROR,
                           FALSE, QRLogicException,
                           "Unknown server type in getProcessName(): %d",
                           serverType);
    }

    actualPrefix = getenv(overridingDefineName);

    if (actualPrefix != NULL)
    {
      if (actualPrefix[0] != '$' || str_len(actualPrefix) > 4)
        return NULL;

      period = (char *) strchr(actualPrefix, '.');
      if (period)
        *period = '\0';
    }
    else
    {
      // No define, use standard prefix.
      switch(serverType)
        {
          case IPC_SQLQMS_SERVER:
            actualPrefix = QMS_PROCESS_PREFIX;
            break;
          case IPC_SQLQMP_SERVER:
            actualPrefix = QMP_PROCESS_PREFIX;
            break;
          case IPC_SQLQMM_SERVER:
            actualPrefix = QMM_PROCESS_PREFIX;
            break;
          default:
            assertLogAndThrow1(CAT_SQL_COMP_QR_IPC, LL_ERROR,
                               FALSE, QRLogicException,
                               "Unknown server type in getProcessName(): %d",
                               serverType);
        }
    }

  // Format required for Linux process name is $<name>
  // defaults to $ZQSNNNN
  char* procName = new(heap) char[100];

  snprintf(procName, sizeof(procName), "%s%04d", actualPrefix, cpu);

  return procName;
}


void MvQueryRewriteServer::getSegmentName(Int32 segmentNumber, char* segmentName)
{
  short segmentNameLen;
  short result = NODENUMBER_TO_NODENAME_(segmentNumber, segmentName,
                                         SEGMENT_NAME_LEN, &segmentNameLen);
  assertLogAndThrow1(CAT_SQL_COMP_QR_IPC, LL_ERROR,
                     result == 0, QRLogicException,
                     "Failed to get name for segment number %d", segmentNumber);
  segmentName[segmentNameLen] = '\0';
}

IpcServer* MvQueryRewriteServer::createServerProcess(IpcServerClass* serverClass,
                                                     Int32 segmentNumber,
                                                     short cpu,
                                                     NABoolean usesTran)
{
  IpcServerType serverType = serverClass->getServerType();
  IpcServer* server = NULL;
  ComDiagsArea* diagsArea = NULL;
  IpcAllocateDiagsArea(diagsArea, heap_);

  char segmentName[] = "NSK";


  char* baseProcName = MvQueryRewriteServer::getProcessName(serverType, NULL, cpu, heap_);


  server = serverClass->allocateServerProcess(&diagsArea, heap_, segmentName, cpu,
                                              IPC_PRIORITY_DONT_CARE, 1, usesTran,
                                              TRUE, 2, NULL, baseProcName);

  if (!server)
    QRLogger::logDiags(diagsArea, CAT_SQL_COMP_QR_IPC);
  return server;
}

IpcServer* MvQueryRewriteServer::getQmsServer(DefaultToken publishDest,
                                              NABoolean checkQms)
{
  if (qmsServer_ && checkQms)
  {
     checkQmsServer();
  }

  if (!qmsServer_)
    {
    try
    {
      getIpcEnv();  // make sure ipcEnv_ has been set
      const short SEGMENT_NAME_LEN = 50; // s/b 8 except for noncompliant names
      delete qmsServerClass_;            // If exists, may be wrong alloc type

      // Can't specify IPC_USE_PROCESS for NT, or it will create a server
      // process, but with a null control connection. Omitting the argument for
      // NT here allows uniform code to handle both platforms below.
      qmsServerClass_ = new IpcServerClass(ipcEnv_, IPC_SQLQMS_SERVER, IPC_USE_PROCESS);
      // Look for QMS on the same cpu we are running on.
      SB_Phandle_Type procHandle;
      Int32 lc_cpu;
      XPROCESSHANDLE_GETMINE_(&procHandle);
      XPROCESSHANDLE_DECOMPOSE_ (&procHandle, &lc_cpu);
      short myCpu = lc_cpu;

      qmsServer_ = createServerProcess(qmsServerClass_, -1, myCpu, TRUE);
      if (qmsServer_)
        {
          QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG,
            "QMS process found on cpu #%d", qmsServer_->getServerId().getCpuNum());
          initQms(qmsServer_, heap_);
        }
      else if (publishDest == DF_PRIVATE || publishDest == DF_BOTH)
        {
          QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_INFO,
            "Could not find local QMS, trying to start one...");
          // Can't change allocation method of existing IpcServerClass, need a
          // new one to spawn process.
          delete qmsServerClass_;
          qmsServerClass_ = new IpcServerClass(ipcEnv_, IPC_SQLQMS_SERVER, 
                                               IPC_SPAWN_OSS_PROCESS);
          ComDiagsArea* diagsArea = NULL;
          IpcAllocateDiagsArea(diagsArea, heap_);
          char segmentName[SEGMENT_NAME_LEN + 1];
          getSegmentName(-1, segmentName);
          qmsServer_ = qmsServerClass_->allocateServerProcess(&diagsArea, heap_,
                                                              segmentName, -1);
          if (qmsServer_)
            {
              QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_INFO,
                "QMS process started on cpu #%d", qmsServer_->getServerId().getCpuNum());
              initQms(qmsServer_, heap_);
            }
          else
            {
              QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
                "Failed to allocate server process for QMS");
              QRLogger::logDiags(diagsArea, CAT_SQL_COMP_QR_IPC);
            }
        }
      else
        {
          QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
            "Could not find local QMS.");
        }
    }
    catch(...)
    {
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
        "Exception when allocating server process for QMS");
      return NULL;
    }
    }
  return qmsServer_;
}  // End of getQmsServer

void MvQueryRewriteServer::checkQmsServer()
{
  char procName[200];
  Int32 nid = 0;
  Int32 pid = 0;
  short result = 0;

  NAProcessHandle phandle((SB_Phandle_Type *)&(qmsServer_->getServerId().getPhandle().phandle_));
  Int32 guaRetcode = phandle.decompose();
  if (!guaRetcode)
  {
    memset(procName, 0, sizeof(procName));
    memcpy(procName, phandle.getPhandleString(), phandle.getPhandleStringLen());
    result = msg_mon_get_process_info (procName, &nid, &pid);
  }
  else
   result = guaRetcode;

  if (result)
    {
      qmsServer_ = NULL;
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_INFO, "A QMS process has died.");
    }
}


IpcServer* MvQueryRewriteServer::getQmmServer()
{
  if (qmmServer_)
    return qmmServer_;

  getIpcEnv();  // make sure ipcEnv_ has been set
  IpcServerClass* qmmServerClass = new(heap_) IpcServerClass(ipcEnv_,
                                                            IPC_SQLQMM_SERVER,
                                                            IPC_USE_PROCESS);
  short cpu;


      char segmentName[] = "NSK";
      SB_Phandle_Type procHandle;
      Int32 lc_cpu;
      XPROCESSHANDLE_GETMINE_(&procHandle);
      XPROCESSHANDLE_DECOMPOSE_ (&procHandle, &lc_cpu);
      cpu = lc_cpu;


  qmmServer_ = createServerProcess(qmmServerClass, 1, cpu, FALSE);
  if (qmmServer_)
    QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG, "QMM process found");
  else
    QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
      "Failed to locate server process for QMM on cpu %d ", cpu);

  return qmmServer_;

}  // End of getQmmServer

void MvQueryRewriteServer::resetQmmServer()
{
  qmmServer_ = NULL;
}

IpcServer* MvQueryRewriteServer::getQmpServer()
{
  if (!qmpServer_)
    {
      short cpu = IPC_CPU_DONT_CARE;
      getIpcEnv();  // make sure ipcEnv_ has been set


        //Phandle wrapper in porting layer
        NAProcessHandle phandle((SB_Phandle_Type *)&(ipcEnv_->getMyOwnProcessId()
                                                     .getPhandle().phandle_));
        short error = phandle.decompose();
        if ( !error)
        {
           cpu = phandle.getCpu();
        }
        else
        {
          QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
            "PROCESSHANDLE_DECOMPOSE_ returned error %d", error);
          cpu = IPC_CPU_DONT_CARE;  // just in case (shouldn't have changed)
        }

      qmpServerClass_ = new(heap_) IpcServerClass(ipcEnv_, IPC_SQLQMP_SERVER);
      qmpServer_ = qmpServerClass_->allocateServerProcess(NULL, NULL, NULL, cpu);
      if (qmpServer_)
        QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_INFO,
          "QMP process started on cpu #%d", qmpServer_->getServerId().getCpuNum());
      else
        QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
          "Failed to allocate server process for QMP");
    }
  return qmpServer_;
}  // End of getQmpServer

// Send a MATCH message to the given qms process, and receive a result
// descriptor in response.
QRXmlMessageObj* MvQueryRewriteServer::sendMatchMessage(IpcServer* qms,
                                                        XMLString* qryDescText,
                                                        CollHeap * heap)
{
  NAString optName("optimizer");
  QRMessageStream msgStream(ipcEnv_, optName, heap, QR::MATCH_REQUEST);
  QRXmlMessageObj* msgPtr = new QRXmlMessageObj(qryDescText, QR::MATCH_REQUEST);
    
  // Add QMS process as a recipient of the message.
  assertLogAndThrow(CAT_SQL_COMP_QR_IPC, LL_ERROR,
                    qms, QRLogicException,
                    "Null qms passed to sendMatchMessage");
  IpcConnection* conn = qms->getControlConnection();
  msgStream.addRecipient(conn);
  msgStream.clearAllObjects();
    
  // Insert message object into the message stream
  msgStream << *msgPtr;

  // Send the message stream to the server
  msgStream.send();
  msgPtr->decrRefCount();

  // Read the reply from the server
  msgStream.setType(QR::MATCH_RESPONSE);
  msgStream.receive();

  QRXmlMessageObj* xmlResponse = NULL;
  if (msgStream.moreObjects())
        {
	  Lng32 t = msgStream.getNextObjType();
	  switch (t)
	  {
	  case QR::MATCH_RESPONSE:
          // Get the message object containing the result descriptor. This
          // is returned as the function result, and the caller must
          // decrement the reference count when through with it.
          xmlResponse = new QRXmlMessageObj(NULL, QR::MATCH_RESPONSE);
	      msgStream >> *xmlResponse;
	    break;

	  case QR::STATUS_RESPONSE:
	    {
            QRStatusMessageObj* statusResponse = new QRStatusMessageObj();
              msgStream >> *statusResponse;
            QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG,
              "STATUS RESPONSE received: %d\n", (Int32)statusResponse->getStatusCode());
            statusResponse->decrRefCount();
          }
          break;

        default:
          QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR, "Unexpected response type: %d.", t);
          break;
      }

      if (msgStream.moreObjects())
        {
          QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_WARN,
            "Match request received one or more extraneous response "
                     "objects, which were discarded");
          msgStream.clearAllObjects();
        }
    }
  else
    {
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_WARN,
        "No response object in message stream from QMS");
      checkQmsServer();
    }

  return xmlResponse;
}  // sendMatchMessage

QRRequestResult MvQueryRewriteServer::initQms(IpcServer* qmsServer,
                                              CollHeap* heap)
{
  QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG, "INITIALIZE request sent to QMS...");
  QRRequestResult response = sendInitializeMessage(qmsServer, heap);
  if (response == Success)
    QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG, "...Initialization succeeded");
  else
    QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR, "INITIALIZATION FAILED, result = %d",
                  response);
  return response;
}

QRRequestResult MvQueryRewriteServer::sendInitializeMessage(IpcServer* qms,
                                                            CollHeap* heap)
{
  NAString optName("optimizer");
  QRMessageStream msgStream(ipcEnv_, optName, heap, QR::INITIALIZE_REQUEST);
  QRSimpleMessageObj* msgPtr = new QRSimpleMessageObj(QR::INITIALIZE_REQUEST);
    
  // Add QMS process as a recipient of the message.
  assertLogAndThrow(CAT_SQL_COMP_QR_IPC, LL_ERROR,
                    qms, QRLogicException,
                    "Null qms passed to sendInitializeMessage()");
  IpcConnection* conn = qms->getControlConnection();
  msgStream.addRecipient(conn);
  msgStream.clearAllObjects();
    
  // Insert message object into the message stream
  msgStream << *msgPtr;

  // Send the message stream to the server
  msgStream.send();
  msgPtr->decrRefCount();

  // Read the reply from the server
  msgStream.setType(QR::STATUS_RESPONSE);
  msgStream.receive();

  QRRequestResult status = ProtocolError;  // unless response read successfully
  if (msgStream.moreObjects())
    {
      Lng32 t = msgStream.getNextObjType();
      if (t == QR::STATUS_RESPONSE)
        {
          QRStatusMessageObj* statusResponse = new QRStatusMessageObj();
          msgStream >> *statusResponse;
          // Must extract the status code before decrementing ref count.
          status = statusResponse->getStatusCode();
          QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_DEBUG,
                        "STATUS RESPONSE received: %d\n", (Int32)status);
          statusResponse->decrRefCount();
        }
      else
        QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR, "Unexpected response type: %d.", t);

      if (msgStream.moreObjects())
        {
          QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_WARN,
            "Initialize request received one or more extraneous response "
                     "objects, which were discarded");
          msgStream.clearAllObjects();
        }
    }
  else
    {
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_WARN,
        "No response object in message stream from QMS");

      if (qmsServer_)
         checkQmsServer();
    }

  return status;
}  // sendInitializeMessage

// This function was copied from cli/Context.cpp.
// *****************************************************************************
// *  
// *   Julian timestamp is converted to standard format:  
// *      YYYY-MM-DD HH:MM:SS.MMM.MMM 
// *
// * @Param  buffer                     | char *                | OUT       |
// *   NUll-terminated char array in which result is returned.
// *
// * @Param  GMT_Time                   | short *               | IN        |
// *   is the julian timestamp to be converted.
// *
// * @End
// *****************************************************************************
void MvQueryRewriteServer::formatTimestamp(
   char          *buffer,   // Output
   Int64          GMT_Time) // Input
{
  if (GMT_Time == 0)
  {
    buffer[0] = '\0';
    return;
  }

  short    Date_and_Time[8];
  short  & Year = *((short  *)&(Date_and_Time[0]));
  short  & Month = *((short  *)&(Date_and_Time[1]));
  short  & Day = *((short  *)&(Date_and_Time[2]));
  short  & Hour = *((short  *)&(Date_and_Time[3]));
  short  & Minute = *((short  *)&(Date_and_Time[4]));
  short  & Second = *((short  *)&(Date_and_Time[5]));
  short  & Millisecond = *((short  *)&(Date_and_Time[6]));
  short  & Microsecond = *((short  *)&(Date_and_Time[7]));
  short    errorNumber;
  bool     isGMT = false;
  Int64    julianTime;

//#if defined (NA_YOS)
// If supplied value is not a legal julian timestmap, punt
   if (GMT_Time > 274958971199999999LL || // The year 4000
       GMT_Time < 146728398400000000LL)   // The year 1
   {
      *buffer = 0;
      return; 
   }
//#endif

//Convert to local time
   julianTime = CONVERTTIMESTAMP(GMT_Time,0,-1,&errorNumber);

// If we can't convert, just show GMT
   if (errorNumber)
      isGMT = true;

// Decompose timestamp
   INTERPRETTIMESTAMP(julianTime,Date_and_Time);

// 0123456789012345678901234567
//"YYYY-MM-DD HH:MM:SS.mmm.mmm"

   NUMOUT(buffer,Year,10,4);
   buffer[4] = '-';
   NUMOUT(&buffer[5],Month,10,2);
   buffer[7] = '-';
   NUMOUT(&buffer[8],Day,10,2);
   buffer[10] = ' ';
   NUMOUT(&buffer[11],Hour,10,2);
   buffer[13] = ':';
   NUMOUT(&buffer[14],Minute,10,2);
   buffer[16] = ':';
   NUMOUT(&buffer[17],Second,10,2);
   buffer[19] = '.';
   NUMOUT(&buffer[20],Millisecond,10,3);
   buffer[23] = '.';
   NUMOUT(&buffer[24],Microsecond,10,3);

   if (isGMT)
   {
      buffer[27] = ' '; 
      buffer[28] = 'G';
      buffer[29] = 'M';
      buffer[30] = 'T';
      buffer[31] = 0;
   }
   else
      buffer[27] = 0;
} // End of formatTimestamp

void MvQueryRewriteServer::getFormattedTimestamp(char* buffer)
{
  formatTimestamp(buffer, NA_JulianTimestamp());
}

// *************************************************************** 
// *************************************************************** 
QR::QRRequestResult 
MvQueryRewriteServer::sendPublishMessage(const NAString* descriptorText,
                                         const NAString& serverName,
                                         IpcServer*& server,
					 CollHeap * heap)
{
  // String constant used in logging entries.
  const static char PUBLISH[] = "PUBLISH";
 
  if (!descriptorText)
    {
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
        "null descriptorText passed to "
                 "MvQueryRewriteServer::sendPublishMessage()");
      return InternalError;
    }

  if (!server)
    {
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
        "null server passed to MvQueryRewriteServer::sendPublishMessage()");
      return InternalError;
    }

  // Only messages we deal with here are publish.
  const char* requestName = PUBLISH;
  IpcMessageObjType msgType = QR::PUBLISH_REQUEST;

  QRRequestResult result;
  QRMessageStream msgStream(server->getServerClass()->getEnv(),
                            serverName,
                            heap,
                            msgType);
  QRXmlMessageObj* msgPtr = new QRXmlMessageObj(descriptorText, msgType);

  // Add server process as a recipient of the message.
  msgStream.addRecipient(server->getControlConnection());
  msgStream.clearAllObjects();
    
  // Insert message object into the message stream, and send it.
  msgStream << *msgPtr;
  msgStream.send();
  msgPtr->decrRefCount();

  QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_INFO,
    "Publish message sent, awaiting reply...");

  // Read the reply from the server
  msgStream.receive();
  if (!msgStream.moreObjects())
  {
    QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
      "No response object in message stream from %s", serverName.toCharStar());
    if (!processExists((short *)&(server->getServerId().getPhandle().phandle_)))
      {
        // If called from catman to publish directly to local qms, server
        // refers to that qms process. If it is not there, nothing we can do
        // here. It will be initialized with all published MVs when it comes
        // back up.
        if (server->getServerClass()->getServerType() == IPC_SQLQMS_SERVER)
          {
            QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_WARN,
              "Can't publish to qms: process does not exist");
            return Unable;
          }

        // Check periodically until QMM is restarted. If we terminate, qmm may
        // have already restarted and found us, so will not create a new qmp.
        // Also, since we were a client of the former qmm, the new one will not
        // receive a system message notifying it of our termination.
        QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_INFO,
          "QMM is gone, trying to connect to new incarnation...");
        server = NULL;
        resetQmmServer(); // so a new one will be looked for
        while (!server)
          {
            server = getQmmServer();
            if (!server)
              {
                DELAY(1000);
                QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_INFO, "Trying again...");
              }
          }

        QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_INFO,
          "Connection to new QMM established, resending message.");
        return sendPublishMessage(descriptorText, serverName, server, heap);
      }
    return ProtocolError;
  }
  else if (msgStream.getNextObjType() != QR::STATUS_RESPONSE)
  {
    QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
      "Wrong response object returned from %s request", requestName);
    return ProtocolError;
  }
  else
  {
    QRStatusMessageObj statusObj;
    msgStream >> statusObj;
    result = statusObj.getStatusCode();
    if (result == QR::Success)
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_INFO,
        "%s request succeeded", requestName);
    else
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_ERROR,
        "%s request failed with status %d", requestName, result);
    if (msgStream.moreObjects())
    {
      QRLogger::log(CAT_SQL_COMP_QR_IPC, LL_WARN,
        "%s request received one or more extraneous response "
                  "objects, which were discarded", requestName);
      msgStream.clearAllObjects();
    }
    return result;
  }
}  // sendPublishMessage

void extractDefineAndThenPutEnvIfFound(char *defineName)
{
} // static void extractDefineAndThenPutEnvIfFound()

