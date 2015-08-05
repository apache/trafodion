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
#ifndef QUERYREWRITESERVERHANDLER_H
#define QUERYREWRITESERVERHANDLER_H
/* -*-C++-*-
 **************************************************************************
 *
 * File:         QueryRewriteServer.h
 * Description:  MvQueryRewriteServer class and methods
 * Created:      06/01/2009
 * Language:     C++
 *
 **************************************************************************
 */

#include "XMLUtil.h"
#include "QRMessage.h"
#include "QRIpc.h"
#include "DefaultConstants.h"

class QmpPublish;

void extractDefineAndThenPutEnvIfFound(char *);

//============================================================================
class MvQueryRewriteServer : public NABasicObject
{
public:

  static char* getProcessName(IpcServerType serverType,
                              char* nodeName,
                              short cpu,
                              CollHeap* heap = NULL);

  static void setHeap(CollHeap* heap)
    {
      heap_ = heap;
      heapHasBeenSet_ = TRUE;
    }

  static IpcEnvironment* getIpcEnv();

  static NABoolean processExists(const short* processHandle);

  static void getSegmentName(Int32 segmentNumber, char* segmentName);

  /**
   * Creates or reuses a server process of the class specified by \c serverClass.
   * @param serverClass Type of server process to look for/create.
   * @param segmentNumber Number of segment to look for/create the process on.
   * @param cpu Cpu to look for/create the server process on.
   * @param usesTran Value of usesTransaction argument to pass to
   *        allocateServerProcess.
   * @return Pointer to the server process.
   */
  static IpcServer* createServerProcess(IpcServerClass* serverClass,
                                        Int32 segmentNumber,
                                        short cpu,
                                        NABoolean usesTran);
  /**
   * Returns a pointer to a QMS server.
   * @param publishDest Whether publishing public, private, or both.
   * @param checkQms Whether or not to check if existing qms process is alive. 
   * @return Pointer to the QMS instance.
   */
  static IpcServer* getQmsServer(DefaultToken publishDest,
                                 NABoolean checkQms = FALSE);

  /**
   * Checks to see if the QMS server in use is still alive. This is invoked
   * when there is no response to a message sent to the QMS.
   */
  static void checkQmsServer();

 /**
   * Returns a pointer to a QMM server.
   * @return Pointer to the QMM instance.
   */
  static IpcServer* getQmmServer();

  /**
   * Sets the QMM server to \c NULL. This is called when QMM is discovered to
   * have died, and allows the new one to be found when getQmmServer() is
   * called again.
   */
  static void resetQmmServer();

 /**
   * Returns a pointer to a QMP server.
   * @return Pointer to the QMP instance.
   */
  static IpcServer* getQmpServer();

  static void getFormattedTimestamp(char* buffer);

  /**
   * Sends a MATCH message to the indicated QMS process, and receives a response
   * message that contains a result descriptor, or \c NULL if an error occurred.
   * The payload of the outgoing message is a query descriptor in XML form.
   *
   * @param qms Pointer to IPCServer object representing the QMS process to
   *            send the MATCH message to.
   * @param qryDescText The XML text for the query descriptor to send to QMS.
   * @param heap Heap to use for allocations.
   * @return Pointer to message object containing result descriptor. This will
   *         be null if an error occurred.
   */
  static QRXmlMessageObj* sendMatchMessage(IpcServer* qms,
                                           XMLString* qryDescText,
                                           CollHeap* heap);

  static QR::QRRequestResult sendPublishMessage(const NAString* descriptorText,
                                                const NAString& serverName,
                                                IpcServer*& server,
                                                CollHeap * heap);

  /**
   * Requests initialization of the passed QMS server.
   *
   * @param qmsServer Instance of QMS to initialize.
   * @param heap Heap to use for any allocations.
   * @return Result returned from QMS reflecting its initialization status.
   */
  static QRRequestResult initQms(IpcServer* qmsServer, CollHeap* heap);

  /**
   * Sends an IPC message to QMS to initialize itself.
   *
   * @param server Instance of QMS to which the message is sent.
   * @param heap Heap used to allocate the message stream object.
   * @return Status responsed returned in a message from QMS.
   */
  static QRRequestResult sendInitializeMessage(IpcServer* server,
                                               CollHeap * heap);

protected:
  static void formatTimestamp(
    char          *buffer,    // Output
    Int64          GMT_Time); // Input

private:
  // Static class, no instances -- make ctor private and undefined
  MvQueryRewriteServer();

  static CollHeap* heap_;
  static NABoolean heapHasBeenSet_;
  static NAString fileNamePrefix_;

  // IPC objects.
  static IpcEnvironment* ipcEnv_;
  static IpcServerClass* qmsServerClass_;
  static IpcServer* qmsServer_;

  static IpcServerClass* qmmServerClass_;
  static IpcServer* qmmServer_;

  static IpcServerClass* qmpServerClass_;
  static IpcServer* qmpServer_;

};  // MvQueryRewriteServer

#endif // QUERYREWRITESERVERHANDLER_H

