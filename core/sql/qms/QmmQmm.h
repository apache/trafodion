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

#ifndef _QMS_H_
#define _QMS_H_

#include "QRIpc.h"
#include "QmsRequest.h"
#include "QRLogger.h"
#include "Collections.h"

#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/int/opts.h"
#include <sys/time.h>

#include "nsk/nskprocess.h"
extern "C" {
#include "cextdecs/cextdecs.h"
#include "zsysc.h"
}


// Classes defined in this file.
class QmmException;
class QmmGuaReceiveControlConnection;
class Qmm;
class QRProcessStub;
  class QmsStub;
  class QmpStub;
class QmmMessageStream;

// This is an extension of the QR namespace.
namespace QR
{
  const short MAX_SEGMENTS = 32;
  const short CPUS_PER_SEGMENT = 16;
  const short SEGMENT_NAME_LEN = 8;  // leading \, 5 for sys name, 2 for seg#

  // Command-line option determines how we start QMP.
  enum StartOpt
    {
      SPAWN,   // use PROCESS_SPAWN_ directly
      SERVER,  // use allocateServerProcess()
      NONE     // don't start QMP; will be done separately
    };

  // Command-line option determines how we listen for incoming messages.
  enum ListenOpt
    {
      RECEIVE, 
      WAITONALL,
      WAITCC
    };
};

using namespace QR;

/**
 * Exception thrown for an error in QMM processing.
 */
class QmmException : public QRException
{
  public:
    /**
     * Creates an exception with text consisting of the passed template filled in
     * with the values of the other arguments.
     *
     * @param[in] msgTemplate Template for construction of the full message;
     *                        contains printf-style placeholders for arguments,
     *                        passed as part of a variable argument list.
     * @param[in] ... Variable argument list, consisting of a value for each
     *                placeholder in the message template.
     */
    QmmException(const char *msgTemplate ...)
      : QRException()
    {
      qrBuildMessage(msgTemplate, msgBuffer_);
    }

    virtual ~QmmException()
    {}

}; //QmmException

class QmmGuaReceiveControlConnection : public GuaReceiveControlConnection
{
  public:
    QmmGuaReceiveControlConnection(IpcEnvironment* env, Qmm* qmm)
      : GuaReceiveControlConnection(env),
        qmm_(qmm)
      {};

    virtual ~QmmGuaReceiveControlConnection()
      {}

    virtual void actOnSystemMessage(short messageNum,
                                    IpcMessageBufferPtr sysMsg,
                                    IpcMessageObjSize sysMsgLen,
                                    short clientFileNumber,
                                    const GuaProcessHandle& clientPhandle,
                                    GuaConnectionToClient* connection);

  private:
    Qmm* qmm_;
};  // QmmGuaReceiveControlConnection

class Qmm : public NABasicObject
{
  public:
    static Qmm* getInstance(CollHeap* heap = NULL)
      {
        if (!instance_)
          instance_ = new Qmm(heap);
        return instance_;
      }
    
    const IpcEnvironment* getEnvironment() const
      {
        return ipcEnv_;
      }

    void allocateQmsPool();
    void checkAndRetryQms(Int16 maxRetries = 3, Int16 delaySeconds = 20);
    void allocateQms();        // for Windows testing
    void startQmp(short cpu);
    void startQms();
    void executeMessageLoop();
    //QRRequestResult handlePublishRequest(QRMessageRequest& request);
    QRRequestResult handlePublishRequest(QRMessageStream* msgStream);
    QRRequestResult handleAllocateRequest(QRMessageStream* msgStream);

    /**
    * Processes a request originating from the message interface. Currently, the
    * only supported request for QMM is a Publish message from QMP. A message
    * object for the response is created and returned as the function value.
    *
    * @param request The message-based request.
    * @return Message object to be returned as the response to this request.
    */
    //QRMessageObj* processRequestMessage(QRMessageRequest& request);
    QRMessageObj* processRequestMessage(QRMessageStream* msgStream);

    void handleClientExit(const short* phandle, short messageNum);

    void setListenOpt(ListenOpt opt)
      {
        listenOpt_ = opt;
      }

    void setQmpStartOpt(StartOpt startOpt)
      {
        qmpStartOpt_ = startOpt;
      }

    QmsStub* getQmsStub(Int32 segNum, short cpuNum)
      {
        return qmsPool_[(segNum-1) * CPUS_PER_SEGMENT + cpuNum];
      }

    void relayPendingPubsToQms();

  protected:
    Qmm(CollHeap* heap);

    virtual ~Qmm() //@ZX -- call freeServerProcess for qmp, qms's
      {
        delete qmsServerClass_;
        //delete qmsMsgStream_;
      }

    IpcTimeout getWaitTimeout();

    static Qmm* instance_;
    QmsStub** qmsPool_;
    short qmsCount_;
    QmpStub* qmp_;
    IpcEnvironment* ipcEnv_;
    IpcServerClass* qmsServerClass_;
    //QRMessageStream* qmsMsgStream_;
    NAList<QRXmlMessageObj*> pendingPubs_;
    ListenOpt listenOpt_;
    StartOpt qmpStartOpt_;
    CollHeap* heap_;
};

//class HeadQmm : public Qmm
//{
//};
class QRProcessStub : public NABasicObject
{
  public:
    static void checkRestarts();

    static const NAList<QRProcessStub*> getRestartList()
      {
        return restartList_;
      }

    QRProcessStub(CollHeap* heap);

    virtual ~QRProcessStub()
      {}

    Int32 operator==(SB_Phandle_Type ph) const
      {
        return !memcmp((char*)&processHandle_, (char*)&ph, sizeof(SB_Phandle_Type));
      }

    SB_Phandle_Type getProcessHandle() const
      {
        return processHandle_;
      }

    void setProcessHandle(SB_Phandle_Type ph)
      {
        memcpy(&processHandle_, &ph, sizeof(SB_Phandle_Type));
      }
    Int64 getLockoutEndTS() const
      {
        return lockoutEndTS_;
      }

    /**
     * Nulls out the process handle. A null process handle consists of all -1s.
     */
    void nullProcessHandle();

    void scheduleRestart();
    virtual NABoolean start() = 0;

  protected:
    static NAList<QRProcessStub*> restartList_;

    void setLockout();

    //zsys_ddl_phandle_def processHandle_;
    SB_Phandle_Type processHandle_;
    Int64 lockoutEndTS_;
    Lng32 retryNumber_;
    CollHeap* heap_;
};

class QmsStub : public QRProcessStub
{
  public:
    // Qmm class will call this from its ctor.
    static void setQmsServerClass(IpcServerClass* serverClass)
      {
        qmsServerClass_ = serverClass;
      }

    enum Status
      {
        UNINITIALIZED = -1,
        RUNNING,
        NOT_RUNNING,
        CPU_NOT_PRESENT,
        CPU_NOT_REACHABLE,
        SEGMENT_NOT_REACHABLE
      };

    QmsStub(short segmentNumber, char* segmentName, short cpuNumber,
            short segmentStatus, NABoolean cpuExists,
            NABoolean unreachableCpu,
            IpcEnvironment* ipcEnv, //QRMessageStream* qmsMsgStream,
            CollHeap* heap);

    virtual ~QmsStub()
      {
        delete qmsProcessName_;
        delete qmsMsgStream_;
      }

    Status getStatus() const
      {
        return status_;
      }

    short getCpuNumber() const
      {
        return cpuNumber_;
      }

    short getSegmentNumber() const
      {
        return segmentNumber_;
      }

    void setStatus(Status newStatus)
      {
        status_ = newStatus;
      }

    virtual NABoolean start();

    virtual void disable(Status reason);

    void publish(QRXmlMessageObj* xmlMsgObj);

  private:
    QmsStub(const QmsStub&);
    Int32 operator=(const QmsStub&);

    static IpcServerClass* qmsServerClass_;

    short segmentNumber_;
    char segmentName_[SEGMENT_NAME_LEN + 1];
    short cpuNumber_;
    short segmentStatus_;
    char* qmsProcessName_;
    IpcServer* qmsServer_;
    Status status_;
    QRMessageStream* qmsMsgStream_;
};

class QmpStub : public QRProcessStub
{
  public:
    QmpStub(IpcEnvironment& ipcEnv, StartOpt qmpStartOpt,
            short cpu,  CollHeap* heap)
      : QRProcessStub(heap),
        ipcEnv_(ipcEnv),
        qmpStartOpt_(qmpStartOpt),
        cpu_(cpu),
        debugQmp_(FALSE)
      {
        start();
      }

    virtual ~QmpStub()
      {  //@ZX
      }

    virtual NABoolean start();

    void spawnProcess(IpcEnvironment& ipcEnv, short cpu);
    void allocateProcess(IpcEnvironment& ipcEnv, short cpu);
#ifdef NA_WINNT
    void launchNSKLiteProcess(IpcEnvironment& ipcEnv, short p_pe);
#endif

  private:
    QmpStub(const QmpStub&);
    Int32 operator=(const QmpStub&);

    static IpcServerClass* qmpServerClass_;

    IpcServer* qmpServer_;
    IpcEnvironment& ipcEnv_;
    StartOpt qmpStartOpt_;
    short cpu_;
    NABoolean debugQmp_;
};

class QmmMessageStream : public QRMessageStream
{
  public:
    /**
     * Creates a message stream used to convey Query Rewrite message objects.
     *
     * @param *env The IPC environment containing the stream.
     * @param logger Logger to use.
     * @param thisEnd Name of the program unit defining the stream (used only
     *                for logging.
     * @param heap Heap used for dynamic allocation.
     * @param msgType Type of messages carried by the stream.
     */
    QmmMessageStream(IpcEnvironment *env,
                     const NAString& thisEnd,
                     Qmm* qmm,
                     NAMemory* heap = NULL,
                     IpcMessageType msgType = UNSPECIFIED_QR_MESSAGE)
      : QRMessageStream(env, thisEnd, heap, msgType),
        qmm_(qmm)
      {}
    
    ~QmmMessageStream()
      {}

    /**
     * Callback function invoked after a message is sent through the stream.
     * @param connection The IpcConnection through which the message has been
     *                   sent.
     */
    //virtual void actOnSend(IpcConnection* connection);

    /**
     * Callback function invoked after a message is received through the stream.
     * @param connection The IpcConnection through which the message has been
     *                   received.
     */
    virtual void actOnReceive(IpcConnection* connection);

    virtual void actOnSendAllComplete()
    {
      clearAllObjects();
      receive(FALSE);
    }

  protected:
    Qmm* qmm_;
}; // QmmMessageStream

#endif  /* _QMS_H_ */
