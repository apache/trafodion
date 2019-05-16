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
 * File:         IpcGuardian.C
 * Description:  OS related code for Guardian-based IPC (see Ipc.h)
 *
 * Created:      2/11/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#define AEVENT 1

#include "Platform.h"
#include <fcntl.h>

#include "ExCollections.h"
#include "Int64.h"
#include "Ipc.h"
#include "str.h"
#include "ComDiags.h"
#include "NAExit.h"
#include "ComRtUtils.h"
#include "PortProcessCalls.h"
#include "logmxevent.h"

#include "MXTraceDef.h"
#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/int/opts.h"
#include <sys/time.h>
#include <errno.h>
extern "C" {
int_16 TMF_SETTXHANDLE_(short *);
}
#include "rosetta/rosgen.h"
#include "nsk/nskprocess.h"
extern "C" {
#include "cextdecs/cextdecs.h"
#include "zsysc.h"
}
#include "fs/feerrors.h"

#include "trafconf/trafconfig.h"  // to get TC_PROCESSOR_NAME_MAX

// Uncomment the next line to debug IPC problems (log of client's I/O)
// #define LOG_IPC

// Uncomment the next line to see a trace of $RECEIVE processing. This
// does not require the define LOG_IPC to be turned on.
// #define LOG_RECEIVE

// Uncomment the next line to see a trace of timeouts. This needs the
// define LOG_IPC to be turned on.
//#define LOG_WAIT_TIMEOUT

#if defined(LOG_IPC) || defined(LOG_RECEIVE)
void IpcGuaLogTimestamp(IpcConnection *conn); // see bottom of file
void allocateConsole(); // see bottom of file
#endif

// -----------------------------------------------------------------------
// The real thing starts here
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Guardian startup message (is sent to a process after startup)
// -----------------------------------------------------------------------

/* Guardian STARTUP message format, copied from stdlib.h
   (couldn't include stdlib.h here because of possible global data) */
struct IpcStartupMsg
{
  short msg_code;
  union
  { char   whole[16];
    struct
    { char volume[8];
      char subvolume[8];
    }    parts;
  } defaults;
  union
  { char   whole[24];
    struct
    { char volume[8];
      char subvolume[8];
      char file[8];
    }    parts;
  } infile;
  union
  { char   whole[24];
    struct
    { char volume[8];
      char subvolume[8];
      char file[8];
    }    parts;
  } outfile;
  char  param[530];

  // methods

  IpcStartupMsg();

}; // IpcStartupMsg

const short GuaIpcStartupMsgCode = -1;

//NGG

static bool sv_checked=false;
static bool sv_trace=false;

#include "seabed/trace.h"

inline static void openTraceFile()
{
  if (sv_checked) {
    return;
  }
  sv_checked = true;

  char *lv_env= getenv("ESP_TRACE_STARTUP");
  if ((lv_env) && (*lv_env=='1')) {
    trace_init((char *)"esptrace", true, (char *)"", true);
    sv_trace = true;
  }
}

#define ESP_TRACE1(s1) if (sv_trace) { trace_printf(s1); }
#define ESP_TRACE2(s1,s2) if (sv_trace) { trace_printf(s1,s2);}


IpcStartupMsg::IpcStartupMsg()
{
  // set the correct message code and blank out all other fields
  // except the param field which is NULL-terminated
  msg_code = GuaIpcStartupMsgCode;
  str_pad(defaults.whole,sizeof(defaults));
  str_pad(infile.whole,sizeof(infile));
  str_pad(outfile.whole,sizeof(outfile));
  param[0] = 0;
}

// -----------------------------------------------------------------------
// Methods for class GuaProcessHandle
// -----------------------------------------------------------------------

Lng32 GuaProcessHandle::decompose(Int32 &cpu, Int32 &pin,
                                  Int32 &nodeNumber
                                  , SB_Int64_Type &seqNum
                                  ) const
{
  // Phandle wrapper in porting layer
  NAProcessHandle phandle((SB_Phandle_Type *)&phandle_);
  
  Lng32 result = phandle.decompose();
  
  if (!result) {
    cpu = phandle.getCpu();
    pin = phandle.getPin();
    nodeNumber = phandle.getNodeNumber();
    seqNum = phandle.getSeqNum();
  }
  
  return result;
}

Int32 GuaProcessHandle::decompose2(Int32 &cpu, Int32 &pin, Int32 &node
                  , SB_Int64_Type &seqNum
                  ) const
{
  return decompose(cpu, pin, node
                  , seqNum
                  );
}

NABoolean GuaProcessHandle::compare(const GuaProcessHandle &other) const
{
  Int32 guaRetcode = XPROCESSHANDLE_COMPARE_((SB_Phandle_Type *)&phandle_,
					   (SB_Phandle_Type *)&(other.phandle_));

  // 0 means different, 1 means two procs of a process pair (different)
  // 2 means the process handles are the same
  return (guaRetcode == 2);
}

NABoolean GuaProcessHandle::fromAscii(const char *ascii)
{

  int retcode = get_phandle_with_retry((char *)ascii, &phandle_);
  if (retcode != FEOK)
    return FALSE;
  return TRUE; 
}

Int32 GuaProcessHandle::toAscii(char *ascii, Int32 asciiLen) const
{
  short result;
  Int32 guaRetcode =0;

  //Phandle wrapper in porting layer
  NAProcessHandle phandle((SB_Phandle_Type *)&phandle_);
  guaRetcode = phandle.decompose();
  memcpy(ascii, phandle.getPhandleString(), phandle.getPhandleStringLen());
  result =  phandle.getPhandleStringLen();

  if (guaRetcode)
  {
     // This went wrong, return an error message in the string
     // NOTE: we often use this method for error processing,
     // so this is probably better than raising an exception.
     // Ok, so it is just an excuse for the missing error handling.
     str_cpy_all(ascii,"Err",3);
     str_itoa(guaRetcode,&ascii[3]);
     return str_len(ascii);
  }

  // return the length of the result string (will be 0 if Guardian
  // couldn't convert the name)
  return result;
}

// -----------------------------------------------------------------------
// Methods for class IpcNodeName
// -----------------------------------------------------------------------

IpcNodeName::IpcNodeName(const GuaProcessHandle &phandle)
{
  Lng32        nodeNumber;
  short       nodeNameLen;
  char        nodeNameWithBackslash[GuaNodeNameMaxLen+1];

  domain_ = IPC_DOM_GUA_PHANDLE;

  //Phandle wrapper in porting layer
  NAProcessHandle procHandle((SB_Phandle_Type *)&phandle.phandle_);

  Int32 err = procHandle.decompose();
  assert(err == 0);
  nodeNumber = procHandle.getNodeNumber();
  nodeNameLen = procHandle.getNodeNameLen();
  memcpy(nodeNameWithBackslash, procHandle.getNodeName(), nodeNameLen); 

  // add the string terminator to the retrieved node name
  nodeNameWithBackslash[nodeNameLen] = 0;

  // copy the name without the leading backslash to the result
  str_cpy(guardianNode_.nodeName_,
	  &nodeNameWithBackslash[1],
	  GuaNodeNameMaxLen,
	  ' ');
}

// -----------------------------------------------------------------------
// Methods for class IpcProcessId
// -----------------------------------------------------------------------

IpcCpuNum IpcProcessId::getCpuNumFromPhandle() const
{

  //Phandle wrapper in porting layer
  NAProcessHandle phandle((SB_Phandle_Type *)&phandle_.phandle_);

  Int32 err = phandle.decompose();
  assert(err == 0);

  return phandle.getCpu();

}

// -----------------------------------------------------------------------
// Methods for class MyGuaProcessHandle
// -----------------------------------------------------------------------
MyGuaProcessHandle::MyGuaProcessHandle()
{

  // set the phandle with my own one

  //Phandle wrapper in porting layer
  NAProcessHandle phandle;

  Int32 err = phandle.getmine((SB_Phandle_Type *)&phandle_);
  assert(err==0); // only error is bounds error (3)
}

// -----------------------------------------------------------------------
// Methods for class GuaConnectionToServer
// -----------------------------------------------------------------------

GuaConnectionToServer::GuaConnectionToServer(
     IpcEnvironment *env,
     const IpcProcessId &procId,
     NABoolean usesTransactions,
     unsigned short nowaitDepth,
     const char *eye,
     NABoolean parallelOpen,
     Int32 *openCompletionScheduled
     ,
     NABoolean dataConnectionToEsp
     )
     : IpcConnection(env,procId,eye)
{
  openFile_                 = InvalidGuaFileNumber;
  openCompletionScheduled_  = openCompletionScheduled;
  nowaitDepth_              = nowaitDepth;
  maxIOSize_                = env->getGuaMaxMsgIOSize();

  activeIOs_ = new(env) ActiveIOQueueEntry[nowaitDepth_];
  for (unsigned short i = 0; i < nowaitDepth_; i++)
    {
      activeIOs_[i].inUse_ = false;
      activeIOs_[i].buffer_ = activeIOs_[i].readBuffer_ = NULL;
      activeIOs_[i].ioTag_ = -1;
    }

  lastAllocatedEntry_       = nowaitDepth_-1;

  numOutstandingIOs_        = 0;
  partiallySentBuffer_      = NULL;
  chunkBytesSent_           = 0;
  partiallyReceivedBuffer_  = NULL;
  chunkBytesRequested_      = 0;
  chunkBytesReceived_       = 0;
  usesTransactions_         = usesTransactions;
  guaErrorInfo_             = GuaOK;
  dataConnectionToEsp_      = dataConnectionToEsp;
  self_                     = FALSE; // Set at openPhandle time
  openRetries_              = 0;
  beginOpenTime_.tv_sec     = 0;
  beginOpenTime_.tv_nsec    = 0;
  completeOpenTime_.tv_sec  = 0;
  completeOpenTime_.tv_nsec = 0;
#if 0
  sentMsgHdr_ = (char *)env->getHeap()->allocateMemory(sizeof(MsgTraceEntry) * 8);
  memset(sentMsgHdr_, 0, sizeof(MsgTraceEntry) * 8);
  sentMsgHdrInd_ = 7;
#endif
  
  // We need a nowait depth of at least 2, one for a message and another
  // one for out-of-band messages (not really implemented yet).
  //assert(nowaitDepth_ >= 2);

  sendCallbackBufferList_ = new(env) IpcMessageBuffer*[nowaitDepth_];
  for (unsigned short i = 0; i < nowaitDepth_; i++)
    sendCallbackBufferList_[i] = NULL;

  tscoOpen_ = FALSE;
  // now open the server process
  openPhandle(NULL, parallelOpen);
}

GuaConnectionToServer::~GuaConnectionToServer()
{
  closePhandle();

  CollHeap *heap = getEnvironment()->getHeap();
  heap->deallocateMemory(activeIOs_);
  heap->deallocateMemory(sendCallbackBufferList_);
#if 0
  heap->deallocateMemory((void *)sentMsgHdr_);
#endif
}

void GuaConnectionToServer::send(IpcMessageBuffer *buffer)
{
  // simply add the new buffer to the send queue and try to start
  // as many new I/O operations as possible
  queueSendMessage(buffer);
  while (tryToStartNewIO())
    ;
}

void GuaConnectionToServer::receive(IpcMessageStreamBase *msg)
{
  // Receiving from a Guardian server is implicit, since the WRITEREADX
  // call performs both a send and a receive operation together. However,
  // we still need to add the callback and, if the I/O has already
  // completed, call the callback.

  addReceiveCallback(msg);
  
  // maybe the Guardian I/O has already completed and the buffer is
  // waiting in the base class' receive queue
  IpcMessageBuffer *receiveBuf;
  while (receiveBuf = getNextReceiveQueueEntry())
    {
      // yes, so just call its callback
      receiveBuf->callReceiveCallback(this);
    }
}

NABoolean GuaConnectionToServer::moreWaitsAllowed()
{
  return !stopWait_;
}

//
// Wait for an I/O reply. After receives a reply, the I/O entry looks like:
//
//   - entry.buffer_=entry.readBuffer_=reply buffer
//
WaitReturnStatus GuaConnectionToServer::wait(IpcTimeout timeout, UInt32 *eventConsumed, IpcAwaitiox *ipcAwaitiox)
{
  Int32 cpu, pin, nodeNumber;
  SB_Int64_Type seqNum = -1;

  GuaProcessHandle *otherEnd;
  if (getState() != OPENING)
  {
    // Shouldn't have ipcAwaitiox completion if numOutStandingIOs_ equals zero
    assert(numOutstandingIOs_ > 0 || ipcAwaitiox == NULL); 
    NABoolean retry = TRUE;
    MXTRC_FUNC("GCTS::wait");
    MXTRC_1("timeout=%d\n", timeout);
    // don't do anything if the connection is in an error state and
    // there are no more pending requests to work on.
    if (getState() == ERROR_STATE AND numOutstandingIOs_ <= 0 
        AND numQueuedSendMessages() <= 0)
      { // no more waits on this connection
        stopWait(TRUE);
        return WAIT_OK;
      }
    stopWait(FALSE);  

    // try to send or receive first if there is a timeout specified,
    // or if we don't have any IOs outstanding.  Also, try more
    // I/O to cover the special case that we've posted only the 
    // first chunk of an multi-chunk msg.  

    // This latter scenario is possible because of the logic on 
    // tryToStartNewIO that gives up if the per-process limit on 
    // MQCs would be exceeded.  It required this special handling
    // for multichunk messages, as explained in the next paragraph.

    // The connection may have to give up either 1) before the 
    // first chunk is posted, or 2) before the second chunk is 
    // posted or 3) after the second chunk is posted.  
    // The tryToStartNewIO method is called from this class's send
    // method and then also from two places in this wait method.   
    // In case 1), the test of numOutStandingIOs_ will be 
    // sufficient to cause tryToStartNewIO to be called directly 
    // below this comment.  For case 3), the other call to 
    // tryToStartNewIO will be taken, because the a_message_is_done 
    // will be set to true after the server replies to the chunk(s) 
    // already posted.  But for case 2), we need special handling, 
    // because the server does not reply to the first chunk until 
    // all chunks are sent.  And this class's send method will not 
    // be called again for the multichunk message.  So the additional
    // test below to detect that the second chunk has not yet been
    // sent, is needed to force the call to tryToStartNewIO.

    if (timeout != IpcImmediately || 
        numOutstandingIOs_ == 0   || 
        chunkBytesSent_ == maxIOSize_)
      while (tryToStartNewIO())
        ;

    // try to complete I/Os within the given time limit,
    // if there are outstanding I/Os
    if (numOutstandingIOs_ == 0)
      return WAIT_OK;

    IpcMessageBufferPtr bufferAddr = NULL;
    _bcc_status stat;
    Int32                 countRead;
    SB_Tag_Type         ioTag = -1;

    while (retry)
      {
  NABoolean ipcAwaitioxCompleted = ipcAwaitiox != NULL;
      if (ipcAwaitioxCompleted)
        ipcAwaitioxCompleted = ipcAwaitiox->getCompleted();

      if (!ipcAwaitioxCompleted)
      {
        if (tscoOpen_)
           stat = BAWAITIOXTS(&openFile_,
                               (void **) &bufferAddr,
                               &countRead,
                               &ioTag,
                               timeout,
                               OMIT);
        else
           stat = BAWAITIOX(&openFile_,
                               (void **) &bufferAddr,
                               &countRead,
                               &ioTag,
                               timeout,
                               OMIT);
      }
      else
        stat = ipcAwaitiox->ActOnAwaitiox((void **)&bufferAddr,
                                   &countRead,
                                   &ioTag);
          MXTRC_3("GCTS::wait awake filenum=%d bufferAddr=%x ioTag=%d\n", openFile_, bufferAddr, ioTag);
#ifdef LOG_WAIT_TIMEOUT
        IpcGuaLogTimestamp(this);
        cerr << "GCTS:timeout = " << timeout << "  ioTag = " << ioTag << endl;
#endif
        // Only retry if FE_EINTR error and breakEnabled is FALSE.
        retry = FALSE;
        if (_status_ne(stat))
          {
                // get a Guardian error code
            Int32 retcode = BFILE_GETINFO_(openFile_,&guaErrorInfo_);

            if (retcode != 0)
              guaErrorInfo_ = retcode; // not even FILE_GETINFO_ worked
            
            // timeout does not set the connection into an error state
            // but it causes a return
            if (guaErrorInfo_ == GuaTimeoutErr)
              {
                guaErrorInfo_ = GuaOK;
                return WAIT_OK;
              }

            // Are there any cases where we need to retry? $$$$
            // Retry if error is FE_EINTR (4004) and not breakEnabled
            if ((guaErrorInfo_ == 4004) && !getEnvironment()->breakEnabled())
              {
                guaErrorInfo_ = GuaOK;
                retry = TRUE;
              }
            // assert if a server asserted because it received more than
            // total message length within the multi-chunk protocol
            assert(guaErrorInfo_ != FEASSERTNUMBASE);
          }
      } // while (retry)

    // An I/O completed at the address bufferAddr with the returned I/O tag.
    // now try to find the matching message buffer for it.

    getEnvironment()->setEvent(TRUE, AEVENT);
   
    if (!guaErrorInfo_)
      {
	assert(ioTag >= 0 && ioTag < (Lng32)nowaitDepth_);
	ActiveIOQueueEntry &entry = activeIOs_[ioTag];
	assert(entry.inUse_ && ioTag == (Lng32)entry.ioTag_);

	// make sure we actually received the buffer that we expected
	IpcMessageBuffer *writeReadBuffer = entry.readBuffer_;
	if (writeReadBuffer == NULL)
	  // only needed for sending 2nd or later chunks
	  writeReadBuffer = entry.buffer_;
	assert(bufferAddr == writeReadBuffer->data(entry.offset_));

	// we have got the reply for this I/O entry. so set the io tag to -1.
	entry.ioTag_ = -1;
      }
    // If we got an error,the tag may or may  not be a valid one. 
    // So check for both cases below
    if (guaErrorInfo_)
      {	
        setErrorInfo(-1);
	if ((ioTag >= 0) && (ioTag < (Lng32(nowaitDepth_)))) 
	  {
	  // valid tag returned from BAWAITIOX
	    ActiveIOQueueEntry &entry = activeIOs_[ioTag];
	    assert(entry.inUse_ && ioTag == (Lng32)entry.ioTag_);
	    handleIOErrorForEntry(entry);
	  }
	else
	  
	  // We didn't get back any valid iotag from BAWAITIOX so we don't have
	  // any specific entry to handle the error for . This happens for 
	  //cases where no I/O completes - eg error 26,160,22,16. 
	  // So put all active IO entries in error state. This only happens for 
	  // what we consider are very fatal errors so this is ok. 
          handleIOError();

      }
    else
      {
	ActiveIOQueueEntry &entry = activeIOs_[ioTag];
        cleanUpActiveIOEntry(entry);

	//
	// after receiving the reply, we need figure out what the use case is
	// in order to decide what to do next:
	//
	// a) we are in the multi-chunk send mode and we just received an
	//    empty reply. in this case we ignore the empty reply and go to
	//    tryToStartNewIO(), which will send the next chunk or become a
	//    no-op if there is no more chunk to send.
	//
	// b) if the reply buffer contains the first (maybe only) chunk of the
	//    reply then:
	//
	//    b.1) - if this is a single chunk reply, then put the reply buffer
	//           on the receive queue.
	//    b.2) - if this is a multi-chunk reply, switch to the multi-chunk
	//           receive protocol. go to tryToStartNewIO() and send an
	//           empty request to receive the next chunk of reply.
	//
	// c) if the reply buffer contains a subsequent chunk of a multi-chunk
	//    reply, then:
	//
	//    c.1) - if this is the large chunk of the reply, then put the
	//           reply buffer on the receive queue.
	//    c.2) - otherwise, this is a middle chunk of the reply. go to
	//           tryToStartNewIO() and send an empty request to receive
	//           the next chunk of reply.
	//
        if (entry.receiveBufferSizeLeft_ == 0)
          {
            // case a) - received empty reply during multi-chunk send mode
            assert(countRead == 0);

            // Note that this does NOT count as a completion, we don't
            // let the upper layers know that we are using multiple
            // Guardian I/Os for this.
          }
        else
          {
            // we did expect data back, case b) or c)

            // If this is the first (maybe only) chunk of an IpcMessageBuffer
            // then determine the length of the total message by looking into
            // the message header.
            if (entry.offset_ == 0)
              {
                if (entry.buffer_ != entry.readBuffer_)
                  {
                    // no longer need the shared send buffer. release it.
                    entry.buffer_->decrRefCount();
                    // now use only the reply buffer 
                    entry.buffer_ = entry.readBuffer_;
                  }

                // since this is the first (maybe only) chunk of the reply,
                // it has a message header that contains the total length of
		// the reply.

                // Get the size of the message sent (or the reply buffer if shared)
                IpcMessageObjSize bytesSent = entry.buffer_->getMessageLength();

                // unpack message header which contains total message length
                InternalMsgHdrInfoStruct *msgHdr = 
                  new( (IpcMessageObj*)(entry.buffer_->data(0)) )
                  InternalMsgHdrInfoStruct(NULL);
                IpcMessageObjSize msgLen = msgHdr->getMsgLengthFromData();

                // remember the real length of the message coming back
                entry.buffer_->setMessageLength(msgLen);

                // check whether this is case b) or c)
                if (msgLen == (IpcMessageObjSize) countRead)
                  {
                    // case b.1) - this is a single-chunk reply.
            
                    // If we were sending a large buffer (more than one chunk)
                    // and just received a small buffer (one chunk) then
                    // release the large buffer to conserve space on the IPC
                    // heap.
                    if (bytesSent > maxIOSize_)
		      entry.buffer_ = entry.buffer_->resize(getEnvironment(), msgLen);

// jdu 01/24/12 - need more work to get the message info right
//                     env()->addIpcMsgTrace(this, IpcEnvironment::RECEIVE,
//                                           (void *)entry.buffer_, msgLen,
//                                           (msgHdr->isLastMsgBuf()? 1: 0),
//                                           msgHdr->getSeqNum());
		    queueReceiveMessage(entry.buffer_);
                  }
                else
                  {
                    // Case b.2) - this is the first chunk of a multi-chunk
		    // reply. Switch to the multi-chunk receive protocol.
		    // we just received countRead bytes of reply from server.
                    if (msgLen > entry.buffer_->getBufferLength() ||
                        bytesSent > maxIOSize_)
                      {
                        // We want to resize the reply buffer if either
                        // - The server has a reply message that is larger than
                        //   what our buffer can hold
                        // - The request buffer was large (more than one chunk)
                        //   and may now be consuming space unnecessarily on the
                        //   IPC heap
                        entry.buffer_->setMessageLength(countRead);
                        entry.buffer_ = entry.buffer_->resize(getEnvironment(), msgLen);
                        entry.buffer_->setMessageLength(msgLen);
                      }

                    // do some sanity checks, make sure we don't have two
                    // partial buffers at a time
                    if (partiallyReceivedBuffer_ != NULL)
                      reportBadMessage();
                    assert(partiallyReceivedBuffer_ == NULL);
                    if (msgLen <= (IpcMessageObjSize) countRead)
                      reportBadMessage();
                    assert(msgLen > (IpcMessageObjSize) countRead);
            
                    // move some information from the entry to data members
                    // in the connection while the chunky protocol is going on
                    partiallyReceivedBuffer_ = entry.buffer_;
                    chunkBytesRequested_ = countRead;
                    chunkBytesReceived_ = countRead;
                    getEnvironment()->getAllConnections()->
                      setReceivedPartialMessage(TRUE);    
                  }
              } // case b) - first (maybe only) chunk
            else
              {
                // case c) - this is not the first chunk
                    if (partiallyReceivedBuffer_ != entry.buffer_)
                      reportBadMessage();
                assert (partiallyReceivedBuffer_ == entry.buffer_);
                chunkBytesReceived_ += countRead;
        
                if (chunkBytesReceived_ == entry.buffer_->getMessageLength())
                  {
                    // case c.1) - this is the last chunk
// jdu 01/24/12 - need more work to get the message info right
//                     env()->addIpcMsgTrace(this, IpcEnvironment::RECEIVE,
//                                           (void *)entry.buffer_,
//                                           chunkBytesReceived_, 1, 0);
                    queueReceiveMessage(partiallyReceivedBuffer_);
                    partiallyReceivedBuffer_ = NULL;
                    chunkBytesRequested_ = 0;
                    chunkBytesReceived_ = 0;
                  }
                else
                  {
                    // case c.2) - this is a middle chunk with more chunks to
		    // follow
                    getEnvironment()->getAllConnections()->
                      setReceivedPartialMessage(TRUE);    
                  }
              } // case c) - not the first chunk
          } // case b) or c)

	// this I/O completed
	if (getState() != ERROR_STATE && numOutstandingIOs_ == 0)
	  setState(ESTABLISHED);

        // after waiting, try (again) to start as many new I/O operations as
        // possible
        while (tryToStartNewIO())
          ;
      } // if (guaErrorInfo_) else ..

    // check the message buffers on the receive queue and invoke callbacks
    // for any matching message streams
    IpcMessageBuffer *receiveBuf;
    NABoolean aCallbackIsCalled = FALSE;
    while (receiveBuf = getNextReceiveQueueEntry())
      {
        // When the user of this connection sets trustIncomingBuffers_ to
        // FALSE then we perform an integrity check on all incoming
        // message buffers. A failure causes the connection to transition
        // to the ERROR_STATE state.
        if (!getTrustIncomingBuffers() && getState() != ERROR_STATE)
          {
            if (!receiveBuf->verifyBackbone())
              {
                setIpcMsgBufCheckFailed(TRUE);
                guaErrorInfo_ = 0;
                setErrorInfo(-1);
                setState(ERROR_STATE);
              }
          }

        receiveBuf->callReceiveCallback(this);
        aCallbackIsCalled = TRUE;
      }

      // In the ERROR_STATE state we may have to announce I/O completion after 
      // callbacks are issued. The setState() method has the job of
      // detecting when I/O is complete and informing the IpcEnviroment at
      // the appropriate time. Even though it's not intuitive to call
      // setState(ERROR_STATE) here (because we are already in the ERROR_STATE state),
      // we make the call anyway to trigger any necessary bookkeeping.
    if (aCallbackIsCalled && getState() == ERROR_STATE)
      setState(ERROR_STATE);
 
    return WAIT_OK;
  } //getState() == OPENING
  else
  {
    IpcMessageBufferPtr bufferAddr;
    Int32                 countRead;
    SB_Tag_Type         ioTag = -1;
  NABoolean ipcAwaitioxCompleted = ipcAwaitiox != NULL;
      if (ipcAwaitioxCompleted)
        ipcAwaitioxCompleted = ipcAwaitiox->getCompleted();

      _bcc_status stat;
      if (!ipcAwaitioxCompleted)
      {
        if (tscoOpen_)
           stat = BAWAITIOXTS(&openFile_,
                               (void **) &bufferAddr,
                               &countRead,
                               &ioTag,
                               timeout,
                               OMIT);
        else
           stat = BAWAITIOX(&openFile_,
                               (void **) &bufferAddr,
                               &countRead,
                               &ioTag,
                               timeout,
                               OMIT);
      }
      else
        stat = ipcAwaitiox->ActOnAwaitiox((void **)&bufferAddr,
                                   &countRead,
                                   &ioTag);
          MXTRC_3("GCTS::wait awake filenum=%d bufferAddr=%x ioTag=%d\n", openFile_, bufferAddr, ioTag);
#ifdef LOG_WAIT_TIMEOUT
        IpcGuaLogTimestamp(this);
        cerr << "GCTS:timeout = " << timeout << "  ioTag = " << ioTag << endl;
#endif
    if (_status_ne(stat))
      {
        // get a Seabed error code
        GuaErrorNumber getinfoError = BFILE_GETINFO_(openFile_,&guaErrorInfo_);

        if (getinfoError != 0)
          guaErrorInfo_ = getinfoError; // not even FILE_GETINFO_ worked
        if (guaErrorInfo_ == GuaTimeoutErr)
        {
          guaErrorInfo_ = XZFIL_ERR_OK;
          return WAIT_OK;
        }
        short fsError = BFILE_CLOSE_(openFile_); // Don't retain unopened ACB
        otherEnd = (GuaProcessHandle *)&getOtherEnd().getPhandle().phandle_;
        otherEnd->decompose(cpu, pin, nodeNumber
                           , seqNum
                           );
        getEnvironment()->closeTrace(__LINE__, openFile_, cpu, pin, seqNum);
        openFile_ = -1; // Don't leave valid file number in object!
        if (guaErrorInfo_ == XZFIL_ERR_NOSUCHDEV && getState() == OPENING && getOpenRetries() < 8 && dataConnectionToEsp_)
        {
          guaErrorInfo_ = XZFIL_ERR_OK;
          setState(INITIAL);
          setOpenRetries(getOpenRetries() + 1);
          usleep(250000);
          openPhandle(NULL, TRUE);
          return WAIT_OK;
        }
        openRetryCleanup();
        setErrorInfo(-1);
        setState(ERROR_STATE);
        getEnvironment()->getAllConnections()->bumpCompletionCount();
        if (openCompletionScheduled_ != NULL)
          *openCompletionScheduled_ = 1;
        return WAIT_OK;
    }

//  Successful completion
    openRetryCleanup();
    getEnvironment()->getAllConnections()->bumpCompletionCount();
    if (openCompletionScheduled_ != NULL)
      *openCompletionScheduled_ = 1;
    fileNumForIOCompletion_ = openFile_;

  // use setmode 74 to turn off the automatic CANCEL upon AWAITIOX timeout
    stat = BSETMODE(openFile_,74,-1);
    if (_status_ne(stat))
      {
        // get a Guardian error code
        Int32 errcode2 = BFILE_GETINFO_(openFile_,&guaErrorInfo_);

        if (errcode2 != 0)
          guaErrorInfo_ = errcode2; // not even FILE_GETINFO_ worked
        setErrorInfo(-1);
        setState(ERROR_STATE);
        return WAIT_OK;
      }

    // use setmode 30 to allow I/O operations to finish in any order
    stat = BSETMODE(openFile_,30,3);
    if (_status_ne(stat))
      {
        // get a Guardian error code
        Int32 errcode2 = BFILE_GETINFO_(openFile_,&guaErrorInfo_);

        if (errcode2 != 0)
          guaErrorInfo_ = errcode2; // not even FILE_GETINFO_ worked
        setErrorInfo(-1);
        setState(ERROR_STATE);
        return WAIT_OK;
      }

    // use setmode 117 if no transactions should be propagated to the server
    if (NOT usesTransactions_)
      {
        _cc_status stat = BSETMODE(openFile_,117,1);
        if (_status_ne(stat))
          {
            // get a Guardian error code
            Int32 errcode2 = BFILE_GETINFO_(openFile_,&guaErrorInfo_);

            if (errcode2 != 0)
              guaErrorInfo_ = errcode2; // not even FILE_GETINFO_ worked
            setErrorInfo(-1);
            setState(ERROR_STATE);
            return WAIT_OK;;
          }
      }

    // the connection is established now
    setState(ESTABLISHED);
    clock_gettime(CLOCK_REALTIME, &completeOpenTime_);
    return WAIT_OK;
  }
}

void GuaConnectionToServer::openRetryCleanup()
{
  char msgBuf[100];
  Int32 cpu, pin, nodeNumber;
  SB_Int64_Type seqNum = -1;
  if (getOpenRetries())
  {
    ((GuaProcessHandle *)&getOtherEnd().getPhandle().phandle_)->
       decompose(cpu, pin, nodeNumber
       , seqNum
       );
    str_sprintf(msgBuf,
      "GuaConnectionToServer: BFILE_OPEN %d,%d,%ld "
      "error 14 retry count = %d\n", cpu, pin, seqNum, getOpenRetries());
    SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msgBuf, 0);
     setOpenRetries(0);
  }
}

GuaConnectionToServer * GuaConnectionToServer::castToGuaConnectionToServer()
{
  return this;
}

Int32 GuaConnectionToServer::numQueuedSendMessages()
{
  return sendQueueEntries();
}

Int32 GuaConnectionToServer::numQueuedReceiveMessages()
{
  return receiveQueueEntries();
}

void GuaConnectionToServer::populateDiagsArea(ComDiagsArea *&diags,
                                              CollHeap *diagsHeap)
{
  if (guaErrorInfo_ != GuaOK)
  {
    IpcAllocateDiagsArea(diags,diagsHeap);

    *diags << DgSqlCode(-2034) << DgInt0(guaErrorInfo_);
    *diags << DgNskCode(guaErrorInfo_);
    getEnvironment()->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).
      addProcIdToDiagsArea(*diags,0);
    getOtherEnd().addProcIdToDiagsArea(*diags,1);
    if (guaErrorInfo_ == FETIMEDOUT)
    {
      static __thread bool bugcatcherInitialized = false;
      static __thread bool doBugCatcher = true;
      if (!bugcatcherInitialized)
       {
         bugcatcherInitialized = true;
         char *dbc = getenv("ESP_TIMEOUT_BUGCATCHER");
         if (dbc && (*dbc != '1'))
           doBugCatcher = false;
       }
      if (doBugCatcher)
      {
        getOtherEnd().getPhandle().dumpAndStop(TRUE, FALSE);
        genLinuxCorefile("Timeout on ESP.");
      }
     }
  }

  if (getIpcMsgBufCheckFailed())
  {
    IpcAllocateDiagsArea(diags, diagsHeap);

    *diags << DgSqlCode(-2037);
    getEnvironment()->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).
      addProcIdToDiagsArea(*diags, 0);
    getOtherEnd().addProcIdToDiagsArea(*diags, 1);
  }
}

//
// Send a new I/O request.
//
// In Guardian each msg I/O requires a write buffer to send the request, and
// a read buffer used to receive the reply. A same buffer can be used as both
// the write buffer and the read buffer. here we will refer the write buffer
// as the "send buffer", and the read buffer as the "reply buffer".
//
// In our implementation we use ActiveIOQueueEntry to represent a msg I/O.
// entry.buffer_ and entry.readBuffer_ can be used as the send buffer and the
// reply buffer. however, the values of entry.buffer_ and entry.readBuffer_
// depend on the type of the message buffer to be sent. There are four
// possible scenarios:
//
// 1. multi-chunk send buffer, shared by multiple connections:
//
//   - first chunk: entry.buffer_=send buffer
//                  entry.readBuffer_=reply buffer (copied from send buffer's
//                                                  first chunk)
//   - subsequent chunks: entry.buffer_=send buffer, entry.readBuffer_=NULL
//
// note: for the first chunk, both entry.buffer_ and entry.readBuffer_ have
//       identical contents as the send buffer. however, entry.readBuffer_
//       must be used as the write buffer during the actual message send,
//       because it contains the message header that includes the send
//       sequence number that must not be shared between connections.
//       entry.buffer_, in this case, is not used at all during message send.
//       we have to save it only because after we receive the reply, we need
//       to call entry.buffer_->decrRefCount() to free the shared send buffer.
//
// 2. multi-chunk send buffer, single connection (not shared):
//
//   - first chunk: entry.buffer_=entry.readBuffer_=send buffer
//   - subsequent chunks: entry.buffer_=send buffer, entry.readBuffer_=NULL
//
// 3. single-chunk send buffer, shared by multiple connections
//
//   - entry.buffer_=entry.readBuffer_=reply buffer
//
// note: send buffer is released immediately after the send.
//
// 4. single-chunk send buffer, single connection (not shared):
//
//   - entry.buffer_=entry.readBuffer_=send buffer
//
NABoolean GuaConnectionToServer::tryToStartNewIO()
{

  if (getState() == OPENING)
    openPhandle(NULL); // Complete open on control connection and
                       // temporarily ignore errors

  // Any more messages or parts of messages to send?

  // There is nothing to do if there are neither new messages nor
  // incompleted partial messages.
  if (sendQueueEntries() == 0 && !partiallySentBuffer_ &&
      !partiallyReceivedBuffer_)
    return FALSE;

  // do not allow new send if a partial message is being received and we
  // already have requested all the reply data for it.
  if (partiallyReceivedBuffer_ &&
      partiallyReceivedBuffer_->getMessageLength() == chunkBytesRequested_)
    return FALSE;

  // Can't have more than nowaitDepth_ - 1 I/Os outstanding, except
  // when there is an out-of-band message for which we make an exception.
  // If there is an out-of-Band message then assume that it was placed
  // in front of the send queue.
  if (numOutstandingIOs_ >= 
      (IFX (partiallySentBuffer_ OR partiallyReceivedBuffer_)
       THENX nowaitDepth_ 
       ELSEX (nowaitDepth_-1)))
    {
      MXTRC_1("GCTS::tryToStartNewIO false numOutstandingIOs_=%d\n", numOutstandingIOs_);
      return FALSE;
    }

  // Check if the per-process limit on MQCs is exceeded.
  // Note that this should be reconsidered when this code is 
  // multithreaded.

  short numMsgsActual;
  if (XMESSAGESYSTEMINFO(5, &numMsgsActual))
    assert(0);
  if (numMsgsActual+1 >= getEnvironment()->getMaxPerProcessMQCs()) 
    return FALSE;

  // If we reach here this means we can start another nowait I/O;
  // get to the outstanding I/O entry that is to be filled next.
#ifndef NDEBUG
  NABoolean wrapAroundCheck    = FALSE;
#endif

  // We may have to return early from this method if we cannot find
  // space on the IPC heap for an outgoing buffer. If that happens
  // we'll want to restore the original value of lastAllocatedEntry_.
  unsigned short originalLastAllocated = lastAllocatedEntry_;

  // find an entry that is not in use
  while (activeIOs_[lastAllocatedEntry_].inUse_)
    {
      // increment lastAllocatedEntry_ modulo nowaitDepth_
      lastAllocatedEntry_++;
      if (lastAllocatedEntry_ == nowaitDepth_)
        {
          lastAllocatedEntry_ = 0;
#ifndef NDEBUG
          assert(!wrapAroundCheck);  // to detect infinite loop (shouldn't happen)
          wrapAroundCheck = TRUE;
#endif
        }
    }
  // we have found an entry that is not in use
  ActiveIOQueueEntry &entry = activeIOs_[lastAllocatedEntry_];
  assert(!entry.inUse_ && entry.ioTag_ == -1);

  // ---------------------------------------------------------------------
  // set up a new outstanding IO entry, depending on what to do next
  // but don't start the corresponding I/O quite yet
  // ---------------------------------------------------------------------

  // initialize all fields of the entry (there is no constructor)
  entry.bytesSent_             = 0;
  entry.receiveBufferSizeLeft_ = 0;
  entry.offset_                = 0;

  // These help keep track of the need to callSendCallback.
  NABoolean isFirstChunk = FALSE;
  NABoolean isLastChunk = FALSE;

  // ---------------------------------------------------------------------
  // Decide what to do, depending on the currently pending buffers and
  // IOs:
  // 
  // a) send another chunk of a large message down to the server without
  //    asking for data back
  // b) request some more data from the server, if the server replied
  //    with a partial message and we haven't asked for all of the
  //    rest of the data yet (never interleave this with I/Os of
  //    step a), so the server won't get confused)
  // c) get another message from the send queue and find out that it
  //    is too long for a single chunk, so send the first piece
  // d) should be the normal case, get the next message from the send
  //    queue and send it in a single chunk
  // ---------------------------------------------------------------------

  if (partiallySentBuffer_)
    {
      // case a) continue sending more chunks for this buffer
      // but don't ask for reply data, since we want the reply to
      // come back at entry.buffer_->data(0)
      entry.buffer_ = partiallySentBuffer_;
      // for multi-chunk buffer, whether shared or not, the read buffer for
      // any chunk after first chunk is NULL.
      entry.readBuffer_ = NULL;

      assert(chunkBytesSent_ < entry.buffer_->getMessageLength());

      entry.bytesSent_ = MINOF(maxIOSize_,
                               entry.buffer_->getMessageLength() -
			       chunkBytesSent_);
      entry.offset_ = chunkBytesSent_;
      chunkBytesSent_ += entry.bytesSent_;
      // if this is the last chunk ...
      if (chunkBytesSent_ >= entry.buffer_->getMessageLength())
	{
	  // we're done sending chunks
	  partiallySentBuffer_ = NULL;
          chunkBytesSent_ = 0;
	  // can call the send callback now
          isLastChunk = TRUE;
	}

      lastSentBuffer_ = entry.buffer_;
    }
  else if (partiallyReceivedBuffer_)
    {
      // b) next thing to do is to receive another chunk from the server
      entry.buffer_ = entry.readBuffer_ = partiallyReceivedBuffer_;
      entry.offset_ = chunkBytesRequested_;
      entry.receiveBufferSizeLeft_ =
	MINOF(maxIOSize_,
	      entry.buffer_->getMessageLength() - chunkBytesRequested_);
      chunkBytesRequested_ += entry.receiveBufferSizeLeft_;

      lastSentBuffer_ = entry.buffer_;
    }
  else
    {
      // get the next buffer to send from the send queue and check
      // whether it can be sent in one piece

      assert(sendQueueEntries() > 0);
      IpcMessageBuffer *nextToSend = sendQueue()[0];
      assert(nextToSend);

      // assume request and reply use same buffer
      entry.buffer_ = entry.readBuffer_ = nextToSend;
      isFirstChunk = TRUE;

      if (entry.buffer_->getRefCount() > 1)
        {
          // The send buffer is shared by multiple connections. Therefore,
          // allocate a different buffer for the reply.
          entry.readBuffer_ = entry.buffer_->
            copyFromOffset(getEnvironment(), maxIOSize_, 0, FALSE);
          if (entry.readBuffer_ == NULL)
            {
              // We ran out of space on the IPC heap ...
              getEnvironment()->setHeapFullFlag(TRUE);
              lastAllocatedEntry_ = originalLastAllocated;
              return FALSE;
            }
        }

      if (entry.buffer_->getMessageLength() > maxIOSize_)
        {
          // case c), the message we just got from the send queue is too large
          // to be sent in a single chunk :-(
          assert(partiallySentBuffer_ == NULL);

          // indicate multi-chunk protocol
          partiallySentBuffer_ = entry.buffer_;

          entry.bytesSent_ = maxIOSize_;
          chunkBytesSent_ = entry.bytesSent_;
        }
      else
        {
          // case d) can send in single chunk
          entry.bytesSent_ = entry.buffer_->getMessageLength();
          // can call the send callback now
          isLastChunk = TRUE;
        }

      // we always use the reply buffer to send the first (maybe only) chunk,
      // for following reasons:
      //
      //  - for multi-chunk shared send buffer, we MUST use the reply buffer 
      //    (entry.readBuffer_) to send its first chunk because
      //    prepareSendBuffer() sets the sequence number in the message header.
      //
      //  - for other types of send buffers, entry.buffer_=entry.readBuffer_
      //    is always true for the first chunk.
      //
      prepareSendBuffer(entry.readBuffer_);

      // got this far so de-queue buffer from this connection's queue
      removeNextSendBuffer();
      entry.receiveBufferSizeLeft_ =
        MINOF(maxIOSize_,entry.readBuffer_->getBufferLength());

      lastSentBuffer_ = entry.readBuffer_;
    }

  // ---------------------------------------------------------------------
  // Next, start the I/O operation
  // ---------------------------------------------------------------------

  // WRITEREADX requires we use the same buffer for both write and read
  IpcMessageBuffer *writeReadBuffer = entry.readBuffer_;
  if (writeReadBuffer == NULL)
    // only needed for sending 2nd or later chunks
    writeReadBuffer = entry.buffer_;

  short retryCount = 0;
  NABoolean needToRetry;  // reset on each iteration of do loop.
  short fsError = 0;
  do {

  Int32 dummyCountRead; // (gps 6/3/09 changed from unsigned short to int on Linux)
  _bcc_status stat = BWRITEREADX(
       openFile_,
       (char *)writeReadBuffer->data(entry.offset_),
       entry.bytesSent_,
       entry.receiveBufferSizeLeft_,
       &dummyCountRead,
       lastAllocatedEntry_);

      needToRetry = FALSE;
      if (_status_ne(stat))
        {
          // get a Guardian error code
          short retcode = BFILE_GETINFO_(openFile_,&fsError);
          if (retcode != 0)
            fsError = retcode; // not even FILE_GETINFO_ worked
          if ((fsError == FENOLCB) && 
              (retryCount < 100*60)) // after 60 seconds (and 6000 retries),
                                     // just give up.
            {
              // Since the per-process limit was checked above,
              // assume that it is the per-cpu limit, so let us
              // retry.
              retryCount++;
              getEnvironment()->incrRetriedMessages();
              needToRetry = TRUE;
              DELAY(1);           // 1 centisecond 
            }
        }
      else
      {
        if (entry.bytesSent_ >= sizeof(InternalMsgHdrInfoStruct))
        {
            InternalMsgHdrInfoStruct *imhis = (InternalMsgHdrInfoStruct *)
                                        writeReadBuffer->data(entry.offset_);
            env()->addIpcMsgTrace(this, IpcEnvironment::SEND,
                                  (void *)writeReadBuffer->data(entry.offset_),
                                  entry.bytesSent_,
                                  (imhis->isLastMsgBuf()? 1: 0),
                                  imhis->getSeqNum());
#if 0
          if (sentMsgHdrInd_ == 7)
            sentMsgHdrInd_ = 0;
          else
            sentMsgHdrInd_ += 1;
          MsgTraceEntry *msgTraceEntry = (MsgTraceEntry *)(sentMsgHdr_ + sizeof(MsgTraceEntry) * sentMsgHdrInd_);
          memcpy((void *)&msgTraceEntry->internalMsgHdrInfoStruct_, (void *)writeReadBuffer->data(entry.offset_), sizeof(InternalMsgHdrInfoStruct));
          msgTraceEntry->bufAddr_ = (void *)writeReadBuffer->data(entry.offset_);
          msgTraceEntry->sentReceivedLength_ = (unsigned int)entry.bytesSent_;
#endif
        }
        fsError = 0;
      }
    
  } while (needToRetry);

  if (isFirstChunk)
    addSendCallbackBuffer(entry.buffer_);

  if (fsError)
    {
      // an error happened somewhere along the way and we must
      // a) record the Guardian error number,
      guaErrorInfo_ = fsError;
      // b) set the connection to the error state.  If we have not invoked
      // the send callback, then handleIOErrorForEntry() will invoke the
      // send callback.
      setErrorInfo(-1);
      handleIOErrorForEntry(entry);

      // if the design is to disallow any future i/o after connection become
      // error state, should we return false to prevent from calling
      // tryToStartNewIO() again?
      //return FALSE;
    }
  else
    {
      // buffer has been sent successfully
      if (numOutstandingIOs_ == 0)
	setState(SENDING);
      numOutstandingIOs_++;
      entry.inUse_ = true;  // this entry now has an I/O in progress
      entry.ioTag_ = (short)lastAllocatedEntry_;

      // --------------------------------------------------------------
      // If we started the I/O for all chunks of an IpcMessageBuffer
      // (or if the IpcMessageBuffer was sent in a single message), then 
      // call its send callback.
      // --------------------------------------------------------------
      if (isLastChunk)
        {
          // removeSendCallbackBuffer() should always return TRUE here because
          // send callback has not been invoked yet. If error occurred prior to
          // this method call then handleIOErrorForEntry() should have cleared
          // all I/Os on the same message stream and we would not have come
          // here.
          NABoolean sendCallbackFlag = removeSendCallbackBuffer(entry.buffer_);
          if (sendCallbackFlag)
            // the send callback doesn't give away entry.buffer_,  and this is
            // good since the same buffer may be still used for the receive
            // operation.
            entry.buffer_->callSendCallback(this);
          else // - for debugging only
            assert(sendCallbackFlag);
        }
    }

  return TRUE;
}

void GuaConnectionToServer::openPhandle(char * processName, NABoolean parallelOpen)
{
    IpcEnvironment *env = getEnvironment();
  short openFlags;
  openFlags = nowaitDepth_ == 0 ? 0x0 : 0x4000;
  IpcConnectionState stateOnEntry = getState();
  if (stateOnEntry == INITIAL)
  {
    char  procFileName[IpcMaxGuardianPathNameLength];
    short procFileNameLen;

    short i, lastError;
    Int32 countRead;
    // If there are any and it's a data connection, use them even if
    // ssd turned persistent opens off
    if (env->getPersistentOpenAssigned() > 0 && dataConnectionToEsp_)
    {
      NABoolean success = FALSE;
      short fileNum, persistentIndex;
      GuaProcessHandle *otherEnd = (GuaProcessHandle *)&getOtherEnd().getPhandle().phandle_;
      fileNum = env->getPersistentOpenInfo(otherEnd, &persistentIndex);
      if (persistentIndex > -1)
      {
        openFile_ = fileNum;
        {
          setSendPersistentOpenReconnect(TRUE); 
          success = true;
        }
        if (success)
        {
          env->resetPersistentOpen(persistentIndex);
          setState(ESTABLISHED);
          fileNumForIOCompletion_ = openFile_;
          return;
        }
      }
    }


      phandle_template* lp_phandle = (phandle_template *) &(getOtherEnd().getPhandle().phandle_);
      memset(procFileName, 0, IpcMaxGuardianPathNameLength);
      char *srcName = (char *) lp_phandle;
      //    strncpy(procFileName, (char *) lp_phandle->verifierF(), 8);
      NAProcessHandle phandle((SB_Phandle_Type *)
                              &(getOtherEnd().getPhandle().phandle_));
      phandle.decompose();
      procFileNameLen = phandle.getPhandleStringLen();
      strncpy(procFileName, phandle.getPhandleString(), procFileNameLen);
    MXTRC_1("GCTS::openPhandle procFileName=%s\n", procFileName);
   NABoolean isEsp = getEnvironment()->getAllConnections()->getPendingIOs().isEsp();
   getEnvironment()->setLdoneConsumed(TRUE);
    // multi fragment esp 
   ESP_TRACE2("GCTS: OpenPhandle: %s ", procFileName);
   clock_gettime(CLOCK_REALTIME, &beginOpenTime_);

   if (strcmp(getEnvironment()->myProcessName(), 
              procFileName) == 0) {
     ESP_TRACE1("SELF");
     guaErrorInfo_ = BFILE_OPEN_SELF_(&openFile_,
				      0, // open for read/write access
				      0, // shared access
				      nowaitDepth_,
				      0, // sync depth 0 (target proc is not NonStop)
				      openFlags); // options           
     self_ = TRUE;
   }
   else 			     
   {
    if (! isEsp)
    {
       openFlags = openFlags | 0x400; // Thread specific completion TSCO
       tscoOpen_ = TRUE;
    }
   // multi fragment esp
    guaErrorInfo_ = BFILE_OPEN_(procFileName,
                                 procFileNameLen,
                                 &openFile_,
                                 0, // open for read/write access
                                 0, // shared access
                                 nowaitDepth_,
                                 0, // sync depth 0 (target proc is not NonStop)
                                 openFlags); // options
  //  retcode = gettimeofday(&tv2, 0);
  //  elapsedTime = (tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec;
    }
    if (guaErrorInfo_ != GuaOK)
      {
        setErrorInfo(-1);
        setState(ERROR_STATE);
        return;
      }
#ifdef NA_NO_SUCH_PLATFORM // Change to platform such a NA_WINNT to enable
    char messageBuffer[80];
    memcpy(&messageBuffer[0], procFileName, procFileNameLen);
    memset((void *)&messageBuffer[procFileNameLen], '\0', 1);
    cout << messageBuffer << endl;
    cout.flush();
#endif

    if (parallelOpen && (openFlags & 0x4000))
    {
      setState(OPENING);
      return;
    }
  } //getState() == INITIAL
  else
    assert(stateOnEntry == OPENING);



  if (openFlags & 0x4000) // Nowaited FILE_OPEN_
  {
    NABoolean completed;
    do
    {
    completed = TRUE;
    _bcc_status condCode;
    if (getState() == INITIAL ||
        env->getAllConnections()->getPendingIOs().isEsp() == FALSE)
    {
      if (tscoOpen_)
         condCode = BAWAITIOXTS(&openFile_);
      else
         condCode = BAWAITIOX(&openFile_);
    }
    else
    {
      if (tscoOpen_)
          condCode = BAWAITIOXTS(&openFile_, NULL, NULL, NULL, 10); // Wait a tenth of a second
      else
          condCode = BAWAITIOX(&openFile_, NULL, NULL, NULL, 10); // Wait a tenth of a second
    }
    if (_status_ne(condCode))
      {
        NABoolean openFailed = TRUE;
	// get a Guardian error code
        GuaErrorNumber getinfoError = BFILE_GETINFO_(openFile_,&guaErrorInfo_);

	if (getinfoError != 0)
	  guaErrorInfo_ = getinfoError; // not even FILE_GETINFO_ worked

        if (guaErrorInfo_ == GuaTimeoutErr  && env->getAllConnections()->getPendingIOs().isEsp())
        {
          guaErrorInfo_ = XZFIL_ERR_OK;
          ((GuaReceiveControlConnection *)env->getControlConnection())->wait(IpcImmediately, env->getEventConsumed());
          completed = FALSE;
          continue;
        }

        if (openFailed)
        {
          short fsError = BFILE_CLOSE_(openFile_);
          Int32 cpu, pin, nodeNumber;
          SB_Int64_Type seqNum = -1;
          GuaProcessHandle *otherEnd = (GuaProcessHandle *)&getOtherEnd().getPhandle().phandle_;
          otherEnd->decompose(cpu, pin, nodeNumber
                             , seqNum
                             );
          env->closeTrace(__LINE__, openFile_, cpu, pin, seqNum);
          openFile_ = -1; // Don't leave valid file number in object!
          setErrorInfo(-1);
          setState(ERROR_STATE);
          return;
        }
    }
  }
  while (completed == FALSE);
  }
  fileNumForIOCompletion_ = openFile_;
MXTRC_2("connection=%x, filenum=%d\n", this, openFile_);

  // some day we may want to perform nowaited FILE_OPEN_ calls and add
  // a method to "work" on the open.

  // use setmode 74 to turn off the automatic CANCEL upon AWAITIOX timeout
  _bcc_status stat = BSETMODE(openFile_,74,-1);
  if (_status_ne(stat))
    {
      // get a Guardian error code
      Int32 errcode2 = BFILE_GETINFO_(openFile_,&guaErrorInfo_);

      if (errcode2 != 0)
	guaErrorInfo_ = errcode2; // not even FILE_GETINFO_ worked
      setErrorInfo(-1);
      setState(ERROR_STATE);
      return;
    }

  // use setmode 30 to allow I/O operations to finish in any order
  stat = BSETMODE(openFile_,30,3);
  if (_status_ne(stat))
    {
      // get a Guardian error code
      Int32 errcode2 = BFILE_GETINFO_(openFile_,&guaErrorInfo_);

      if (errcode2 != 0)
	guaErrorInfo_ = errcode2; // not even FILE_GETINFO_ worked
      setErrorInfo(-1);
      setState(ERROR_STATE);
      return;
    }

  // use setmode 117 if no transactions should be propagated to the server
  if (NOT usesTransactions_)
    {
      _bcc_status stat = BSETMODE(openFile_,117,1);
      if (_status_ne(stat))
	{
	  // get a Guardian error code
	  Int32 errcode2 = BFILE_GETINFO_(openFile_,&guaErrorInfo_);

	  if (errcode2 != 0)
	    guaErrorInfo_ = errcode2; // not even FILE_GETINFO_ worked
	  setErrorInfo(-1);
	  setState(ERROR_STATE);
	  return;
	}
#     ifdef LOG_IPC
      IpcGuaLogTimestamp(this);
      cerr << "No transaction forwarding (control 117)" << endl;
#     endif

    }

#     ifdef LOG_IPC
      IpcGuaLogTimestamp(this);
      cerr << "Open succeeded" << endl;
#     endif

  // the connection is established now
  setState(ESTABLISHED);
  clock_gettime(CLOCK_REALTIME, &completeOpenTime_);
}

void GuaConnectionToServer::closePhandle()
{
  MXTRC_1("GCTS::closePhandle connection=%x", this);

  //
  // it's possible that some pending I/Os still remain on this connection.
  // if IPC error occurs during query execution, there are three possible
  // scenarios for a connection when it comes here:
  //
  //   1. this connection received ipc error and handled the error on the spot
  //      by calling one of the handleIOError() methods.
  //   2. this connection did not receive error, but some other connection(s)
  //      on the same message stream received error. in this case we should
  //      invoke IpcMessageStream::abandonPendingIOs() to abort pending I/Os
  //      on all of stream's connections and invoke all necessary callbacks.
  //   3. this connection did not receive error, neither did any other
  //      connections on the same message stream. in this case it's possible
  //      that nothing happens on this connection until the destructor is
  //      invoked. thus we need to clean up any pending I/Os on the connection.
  //      we should invoke delinkConnection() on the stream to trigger any
  //      book keepings needed, as delinkConnection() and receive callback
  //      should have the same book keeping logic.
  //
  // example for #3: we have a test case that kills esps while executing a
  // long running query. the master has multiple send tops, with each send top
  // having a message stream that includes only one data connection to a top
  // level esp. if IPC error occurs on other connections (but not on this
  // connection) as the result of dead esps, all sql operations are aborted
  // from higher level and the master may never get a chance to call
  // handleIOError() to clean up any pending I/Os on this connection. thus we
  // need to release any msg buffers from the pending I/Os. but note that
  // in case of multi-chunk message buffer, there may be multiple I/O entries
  // pointing to the same message buffer. in that case each buffer should be
  // released only once.
  //
  handleIOError();

  // receive queue may not be empty. if so invoke receive callback.
  // example:
  // GuaConnectionToServer::setFatalError() invokes handleIOErrorForStream()
  // that may have put message buffer on receive queue.
  IpcMessageBuffer *receiveBuf;
  while (receiveBuf = getNextReceiveQueueEntry())
    receiveBuf->callReceiveCallback(this);

  // Note that after closing, the connection is always in the initial
  // state. This is the way to fix a connection in the error state.
  // The close is always considered successful.
  guaErrorInfo_ = GuaOK;
  clearErrorInfo();
  setState(INITIAL);

  if (openFile_ != InvalidGuaFileNumber)
    {
      IpcEnvironment *env = getEnvironment();
      NABoolean closeFile;
      if (env->getPersistentOpens() && dataConnectionToEsp_ && self_ == FALSE && numOutstandingIOs_ == 0)
      {
        closeFile = FALSE;
        short persistentIndex = env->getNewPersistentOpenIndex();
        if (persistentIndex > -1)
        {
          GuaProcessHandle *otherEnd = (GuaProcessHandle *)&getOtherEnd().getPhandle().phandle_;
          env->setPersistentOpenInfo(persistentIndex, otherEnd, openFile_);
          Int32 cpu, pin, nodeNumber;
          SB_Int64_Type seqNum = -1;
          otherEnd = (GuaProcessHandle *)&getOtherEnd().getPhandle().phandle_;
          otherEnd->decompose(cpu, pin, nodeNumber
                             , seqNum
                             );
          env->closeTrace(__LINE__, openFile_, cpu, pin, 
                           seqNum); // Persistent open simulated close
        }
      }
      else
        closeFile = TRUE;
      if (closeFile)
      {
        _bcc_status status;
        short lastError;
        for (Int32 numOut = 0; numOut < numOutstandingIOs_; numOut++)
        {
          status = BCANCELREQ(openFile_);
          if (_status_ne(status))
            short retCode = BFILE_GETINFO_(openFile_, &lastError);
        }
        BFILE_CLOSE_(openFile_);
        Int32 cpu, pin, nodeNumber;
        SB_Int64_Type seqNum = -1;
        GuaProcessHandle *otherEnd = (GuaProcessHandle *)&getOtherEnd().getPhandle().phandle_;
        otherEnd->decompose(cpu, pin, nodeNumber
                           , seqNum
        );
        env->closeTrace(__LINE__, openFile_, cpu, pin, nodeNumber);
      }

      openFile_ = fileNumForIOCompletion_ = InvalidGuaFileNumber;
    }
}

NABoolean GuaConnectionToServer::hasActiveIOs()
{
  for (unsigned short i = 0; i < nowaitDepth_; i++)
  {
     ActiveIOQueueEntry &entry = activeIOs_[i];
     if (entry.inUse_)
        return TRUE;
  }
  return FALSE;
}

void GuaConnectionToServer::setFatalError(IpcMessageStreamBase *msgStream)
{ 
  if (guaErrorInfo_ == GuaOK)  // if error hasn't been set yet
    guaErrorInfo_ = GuaIpcApplicationErr;

  setState(ERROR_STATE);

  // we must set error info to -1 so receive callback knows not to
  // parse the potentially corrupted receive queue buffer.
  setErrorInfo(-1);

  // handleIOErrorForStream() may put the message buffer on receive queue.
  // and that buffer may have the following content:
  //
  //   - the actual full reply from server, or
  //   - the send buffer if send failed, or
  //   - partial send or reply if send/reply was multi-chunk and did not
  //     complete
  //
  handleIOErrorForStream(msgStream);

  IpcConnection::setFatalError(msgStream);
}

void GuaConnectionToServer::addSendCallbackBuffer(IpcMessageBuffer *buffer)
{
  for (unsigned short i = 0; i < nowaitDepth_; i++)
    {
      if (!sendCallbackBufferList_[i])
        {
          sendCallbackBufferList_[i] = buffer;
          return;
        }
    }
}

NABoolean GuaConnectionToServer::removeSendCallbackBuffer(IpcMessageBuffer *buffer)
{
  for (unsigned short i = 0; i < nowaitDepth_; i++)
    {
      if (sendCallbackBufferList_[i] == buffer)
        {
          sendCallbackBufferList_[i] = NULL;
          return TRUE;
        }
    }

  return FALSE;  
}

void GuaConnectionToServer::handleIOError()
{
  // connection no longer usable due to I/O error. abort all existing I/Os
  // on this connection.
  for (unsigned short i = 0; i < nowaitDepth_; i++)
    {
      ActiveIOQueueEntry &entry = activeIOs_[i];
      if (entry.inUse_)
        handleIOErrorForEntry(entry);
    }

  numOutstandingIOs_ = 0;
}

void GuaConnectionToServer::handleIOErrorForStream(IpcMessageStreamBase *msgStream)
{
  // abort all existing I/Os on this connection that are from the given stream
  for (unsigned short i = 0; i < nowaitDepth_; i++)
    {
      ActiveIOQueueEntry &entry = activeIOs_[i];
      if (entry.inUse_ && entry.buffer_->getMessageStream() == msgStream)
        handleIOErrorForEntry(entry);
    }
}

//
// The I/O entry has a write buffer (entry.buffer_) and a read buffer
// (entry.readBuffer_). For entry description after message send, see comments
// at the top of tryToStartNewIO(). For entry description after receive reply,
// see comments at the top of wait().
//
void GuaConnectionToServer::handleIOErrorForEntry(ActiveIOQueueEntry &entry)
{
  // I/O error occurred on given entry during send or receive

  if (getState() != ERROR_STATE)
    setState(ERROR_STATE);

  if (getErrorInfo() == 0)
      setErrorInfo(-1);

  // abort all existing I/Os on the same message stream
  // - what about I/Os on other streams?
  for (unsigned short i = 0; i < nowaitDepth_; i++)
    {
      ActiveIOQueueEntry &nextEntry = activeIOs_[i];

      if (nextEntry.inUse_ &&
          nextEntry.buffer_->getMessageStream() ==
          entry.buffer_->getMessageStream())
        {
          cleanUpActiveIOEntry(nextEntry);

          // special case: for multi-chunk shared buffer, if nextEntry is
          // first chunk and entry is any chunk after first chunk, we need
          // to free the read buffer of the first chunk since all other chunks
          // have their read buffers set to NULL.
          if (nextEntry.readBuffer_ &&
              nextEntry.readBuffer_ != nextEntry.buffer_)
            // nextEntry is the first chunk of a multi-chunk shared buffer
            nextEntry.readBuffer_->decrRefCount();
        }
    }  // for i

  // clear partial send/receive buffer on the same message stream so to
  // prevent further I/Os.
  if (partiallySentBuffer_ &&
      partiallySentBuffer_->getMessageStream() ==
      entry.buffer_->getMessageStream())
    partiallySentBuffer_ = NULL;
  else if (partiallyReceivedBuffer_ &&
           partiallyReceivedBuffer_->getMessageStream() ==
           entry.buffer_->getMessageStream())
    partiallyReceivedBuffer_ = NULL;

  // put the buffer on receive queue, regardless of send succeeded or not.
  queueReceiveMessage(entry.buffer_);

  // if entry.buffer_ is still on the sendCallbackBufferList_, then send
  // callback has not been invoked for this connection and thus send failed.
  NABoolean sendSuccess = !removeSendCallbackBuffer(entry.buffer_);
  // in case of send failure invoke send callback
  if (!sendSuccess)
    entry.buffer_->callSendCallback(this);

  // the design is to disallow any future i/o after connection become error
  // state. so we should cleanup send queue by invoking send callback for
  // all send buffers.
  IpcMessageBuffer *sendBuffer;
  while (sendBuffer = removeNextSendBuffer())
    {
      queueReceiveMessage(sendBuffer);
      sendBuffer->callSendCallback(this);
    }
}

void GuaConnectionToServer::cleanUpActiveIOEntry(ActiveIOQueueEntry &entry)
{
  // abort if we are still waiting for message reply
  if (entry.ioTag_ >= 0)
    {
      // if we come here with entry's I/O still outstanding, it means we're
      // on the error handling path. so it's ok to abort/cancel the I/O.
      //
      // note that when CANCELREQ is called to cancel a request on a process
      // file, the file system aborts the transaction associated with the
      // process.
      assert(getErrorInfo() != 0);
      BCANCELREQ(openFile_, entry.ioTag_);
      entry.ioTag_ = -1;
    }

  entry.inUse_ = false;
  numOutstandingIOs_--;
}

void GuaConnectionToServer::dumpAndStopOtherEnd(bool dump, bool stop) const
{
  getOtherEnd().getPhandle().dumpAndStop(dump, stop);
}

// -----------------------------------------------------------------------
// Methods for class GuaConnectionToClient
// -----------------------------------------------------------------------

GuaConnectionToClient::GuaConnectionToClient(
     IpcEnvironment *env,
     const IpcProcessId &clientProcId,
     GuaFileNumber clientFileNumber,
     GuaReceiveControlConnection *controlConnection,
     const char *eye)
     : IpcConnection(env,clientProcId,eye)
{
  clientFileNumber_        = clientFileNumber;
  guaErrorInfo_            = GuaOK;
  controlConnection_       = controlConnection;

  partiallyRepliedBuffer_  = NULL;
  chunkBytesReplied_       = 0;
  partiallyReceivedBuffer_ = NULL;
  chunkBytesReceived_      = 0,
  fileNumForIOCompletion_  = controlConnection->receiveFile_;
  numOutstandingRequests_ = 0;
#if 0
  receivedMsgHdr_ = (char *)env->getHeap()->allocateMemory(sizeof(MsgTraceEntry) * 8);
  memset(receivedMsgHdr_, 0, sizeof(MsgTraceEntry) * 8);
  receivedMsgHdrInd_ = 7;
#endif
}

GuaConnectionToClient::~GuaConnectionToClient()
{
#if 0
  CollHeap *heap = getEnvironment()->getHeap();
  heap->deallocateMemory((void *)receivedMsgHdr_);
#endif
}

bool GuaConnectionToClient::isServerSide()
{
  return true;
}

void GuaConnectionToClient::send(IpcMessageBuffer *buffer)
{

  if (buffer->getReplyTag() == GuaInvalidReplyTag)
    ABORT("Need to wait for a request before replying for now");

  queueSendMessage(buffer);

  while (startReplyingToNextRequest())
    ;

}

void GuaConnectionToClient::receive(IpcMessageStreamBase *msg)
{
  setState(RECEIVING);
  // tell the control connection that we are ready to receive
  controlConnection_->initiateReceive();

  addReceiveCallback(msg);

  // maybe the Guardian I/O has already completed and the buffer is
  // waiting in the base class' receive queue
  IpcMessageBuffer *receiveBuf;
  while (receiveBuf = getNextReceiveQueueEntry())
    {
      // yes, so just call its callback
#     ifdef LOG_RECEIVE
      cerr << "Calling receive callback for queued request during receive()"
	   << endl;
#     endif

      receiveBuf->callReceiveCallback(this);
    }

}


WaitReturnStatus GuaConnectionToClient::wait(IpcTimeout timeout, UInt32 *eventConsumed, IpcAwaitiox *ipcAwaitiox)
{
  MXTRC_FUNC("GCTC::wait");
  MXTRC_1("timeout=%d\n", timeout);
  // wait on the control connection for the specified timeout
  WaitReturnStatus result = controlConnection_->wait(timeout, eventConsumed, ipcAwaitiox);

  if (result)
    {
      // if an I/O completed, retry until no more I/Os can be completed
      // without waiting
      while (controlConnection_->wait(IpcImmediately, eventConsumed, ipcAwaitiox))
	;
    }
  return WAIT_OK;
}

GuaConnectionToClient * GuaConnectionToClient::castToGuaConnectionToClient()
{
  return this;
}

Int32 GuaConnectionToClient::numQueuedSendMessages()
{
  return sendQueueEntries();
}

Int32 GuaConnectionToClient::numQueuedReceiveMessages()
{
  return receiveQueueEntries();
}

void GuaConnectionToClient::populateDiagsArea(ComDiagsArea *&diags,
                                              CollHeap *diagsHeap)
{
  if (guaErrorInfo_ != GuaOK)
    {
      IpcAllocateDiagsArea(diags,diagsHeap);

      *diags << DgSqlCode(-2033) << DgInt0(guaErrorInfo_)
             << DgNskCode(guaErrorInfo_);
      getEnvironment()->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).
	addProcIdToDiagsArea(*diags,0);
      getOtherEnd().addProcIdToDiagsArea(*diags,1);
    }
}

NABoolean GuaConnectionToClient::thisIsMyClient(
     const GuaProcessHandle &phandle,
     GuaFileNumber fileNo) const
{
  return (clientFileNumber_ == fileNo AND
	  getOtherEnd().getPhandle() == phandle);
}

void GuaConnectionToClient::close(NABoolean withError,
                                  GuaErrorNumber gerr)
{
  if (numOutstandingRequests_ != 0)
  {
    withError = TRUE;;
  }
  if (getState() == RECEIVING)
    {
      controlConnection_->numReceivingConnections_--;
    }

  // set the state to CLOSED or ERROR_STATE, meaning that we are not connected
  // CLOSED causes the collection to be deleted when it is safe (no recursion)
  // ERROR_STATE causes it to be left around to provide debugging evidence
  if (withError)
    setState(ERROR_STATE);
  else
    setState(CLOSED);
  guaErrorInfo_ = gerr;

  // check if there are outstanding I/Os and raise an error if there are
  IpcMessageBuffer *lostBuffer;

  lostBuffer = partiallyRepliedBuffer_;
  if (lostBuffer != NULL)
    {
      // clean up
      partiallyRepliedBuffer_ = NULL;
      chunkBytesReplied_ = 0;

      // couldn't reply with all of the buffer, this is an error
      setState(ERROR_STATE);
      lostBuffer->callSendCallback(this);
      lostBuffer->decrRefCount();
    }

  while ((lostBuffer = getNextSendQueueEntry()) != NULL)
    {
      // if a connection with outstanding I/Os gets closed then this
      // is an error
      setState(ERROR_STATE);
      lostBuffer->callSendCallback(this);
      lostBuffer->decrRefCount();
    }

  if (partiallyReceivedBuffer_)
    {
      // clean up, client must have known what it did when it stopped
      // half way sending the buffer
      partiallyReceivedBuffer_->decrRefCount();
      partiallyReceivedBuffer_ = NULL;
      chunkBytesReceived_ = 0;
    }

  // it's ok to have buffers in the receive queue, those are still
  // available for their recipients to be read

  // numRequestors counts the number of file opens on $RECEIVE and
  // there is a 1:1 correlation between open connections and file opens
  controlConnection_->decrNumRequestors();

  // Indicates that a closed connect exists that should be found and deleted
  // when there is no recursion
  if (getState() == CLOSED)
    getEnvironment()->getAllConnections()->incrDeleteCount();

  // if there are outstanding I/Os on $RECEIVE, tell the control
  // connection about them $$$$
}

NABoolean GuaConnectionToClient::startReplyingToNextRequest()
{
  IpcMessageBuffer *buffer;

  if (partiallyRepliedBuffer_)
    return FALSE;

  buffer = getNextSendQueueEntry();
  if (buffer == NULL)
    return FALSE;

  // In Guardian, a send from the client to the server is called reply
  // and it is done without locking the server... unless the reply is
  // larger than the max. reply length, in which case we need to switch
  // to the multiple chunk method

  IpcMessageObjSize bytesToSend = buffer->getMessageLength();
  if (bytesToSend > buffer->getMaxReplyLength())
    {
      // message has to be transported back in multiple chunks
      assert(partiallyRepliedBuffer_ == NULL);
      partiallyRepliedBuffer_ = buffer;
      chunkBytesReplied_      = 0;
      bytesToSend             = buffer->getMaxReplyLength();
      IOPending();
    }

  lastSentBuffer_ = buffer;
  // send it off
  controlConnection_->sendReplyData(buffer->data(0),
				    bytesToSend,
				    buffer->getReplyTag(),
				    this,
				    GuaOK);
  decrNumOutstandingRequests();
  if (partiallyRepliedBuffer_ AND
      NOT (getState() == ERROR_STATE))
    {
      chunkBytesReplied_ += bytesToSend;
      // we need to get another request from the client for the next chunk
      controlConnection_->initiateReceive();
    }
  else
    {
      // The send operation has completed
// jdu 01/24/12 - need more work to get the message info right
//       env()->addIpcMsgTrace(this, IpcEnvironment::RESPOND,
//                             (void *)buffer, buffer->getMessageLength(),
//                             1, (UInt32) buffer->getReplyTag());
      // Call the send callback (which does not take the buffer away)
      buffer->callSendCallback(this);

      // try to reuse this buffer another time
      if (buffer->getRefCount() == 1)
	{
	  controlConnection_->recycleReceiveBuffer(buffer);
	}
      else
	{
	  ABORT("No other reuse of reply message buffers for now");
	  // buffer->decrRefCount(getEnvironment());  would be another option
	}
    }

  return TRUE;
}

void GuaConnectionToClient::acceptBuffer(IpcMessageBuffer  *buffer,
					 IpcMessageObjSize receivedDataLength)
{
  // the length of the logical message that we are going to get
  IpcMessageObjSize totalMessageLength;
  incrNumOutstandingRequests();

  // ---------------------------------------------------------------------
  // Handle case of a multi-chunk reply first. If the incoming message
  // is empty that means that the requestor is asking for additional
  // chunks of a multi-chunk reply.
  // ---------------------------------------------------------------------
  if (receivedDataLength == 0)
    {
      if (partiallyRepliedBuffer_ != NULL AND
          partiallyReceivedBuffer_ == NULL)
        ;
      else 
        dumpAndStopOtherEnd(true, false);
      assert(partiallyRepliedBuffer_ != NULL AND partiallyReceivedBuffer_ == NULL);
          
      // Requestor is asking for more of the partial reply buffer.
      // Reply with the next chunk.
      IpcMessageObjSize nextChunkSize =
	MINOF(partiallyRepliedBuffer_->getMessageLength() - chunkBytesReplied_,
	      buffer->getMaxReplyLength());

      controlConnection_->sendReplyData(
	   partiallyRepliedBuffer_->data(chunkBytesReplied_),
	   nextChunkSize,
	   buffer->getReplyTag(),
	   this,
	   GuaOK);
      decrNumOutstandingRequests();
      chunkBytesReplied_ += nextChunkSize;
      controlConnection_->recycleReceiveBuffer(buffer);

      if (chunkBytesReplied_ >= partiallyRepliedBuffer_->getMessageLength())
	{
	  // all of the message got sent, get rid of the oversized reply
	  // buffer and call the callback (as usual, save everything on
	  // the stack before calling the callback)
          IOComplete();
	  IpcMessageBuffer *b = partiallyRepliedBuffer_;
	  partiallyRepliedBuffer_ = NULL;
	  chunkBytesReplied_      = 0;
	  b->callSendCallback(this);
	  b->decrRefCount();
	}
      else
	{
	  // tell the control connection we need another one
	  controlConnection_->initiateReceive();
    
          return;
        }
    }

  // ---------------------------------------------------------------------
  // Check out the situation: is the incoming data an entire message or
  // is it just a chunk. If it's a chunk, is it the first or the last one?
  // Switch from and to the chunk protocol, if necessary.
  // ---------------------------------------------------------------------
  else if (partiallyReceivedBuffer_ == NULL)
    {
      // this is the first (maybe the only) chunk of a new message
      // unpack message header which contains total message length
     
      InternalMsgHdrInfoStruct *msgHdr = 
        new( (IpcMessageObj*)(buffer->data(0)) )
          InternalMsgHdrInfoStruct(NULL);
        
      totalMessageLength = msgHdr->getMsgLengthFromData();
      buffer->setMessageLength(totalMessageLength);

      if (totalMessageLength == receivedDataLength)
	{
	  // simplest case, single-chunk message
          queueReceiveMessage(buffer);
          setState(ESTABLISHED);
	}
      else
	{
	  // total message len should never be less than received len
          if (totalMessageLength <= receivedDataLength)
             reportBadMessage();
	  assert(totalMessageLength > receivedDataLength);

	  // we only received part of the data, go and allocate a
	  // buffer that can hold all of it and switch to the chunk protocol
	  buffer->setMessageLength(receivedDataLength);
	  partiallyReceivedBuffer_ = buffer->resize(getEnvironment(),
						    totalMessageLength);
	  chunkBytesReceived_ = receivedDataLength;
	  partiallyReceivedBuffer_->setMessageLength(totalMessageLength);
	}
    }
  else
    {
      // we're already in the chunky protocol (beyond 1st chunk),
      // copy the additional data
      totalMessageLength = partiallyReceivedBuffer_->getMessageLength();
          if (chunkBytesReceived_ + receivedDataLength > totalMessageLength)
             reportBadMessage();
      assert(chunkBytesReceived_ + receivedDataLength <= totalMessageLength);
      str_cpy_all(partiallyReceivedBuffer_->data(chunkBytesReceived_),
		  buffer->data(0),
		  receivedDataLength);
      chunkBytesReceived_ += receivedDataLength;

      // must reply with an empty message to secondary request chunks
      controlConnection_->sendReplyData(NULL,
                                        0,
                                        buffer->getReplyTag(),
                                        this,
                                        GuaOK);
      decrNumOutstandingRequests();
      controlConnection_->recycleReceiveBuffer(buffer);

      // We are done receiving the entire message if all the data has arrived.
      if (chunkBytesReceived_ == totalMessageLength)
	{
          queueReceiveMessage(partiallyReceivedBuffer_);
          setState(ESTABLISHED);
	  partiallyReceivedBuffer_ = NULL;
	  chunkBytesReceived_ = 0;
	}
      else
        {
        // tell the control connection we need another one
        controlConnection_->initiateReceive();
        }
    }
      
  // call callbacks for any matching message streams
  IpcMessageBuffer *receiveBuf;
  while (receiveBuf = getNextReceiveQueueEntry())
    receiveBuf->callReceiveCallback(this);

  // send any messages blocked by multi-chunk protocol
  while (startReplyingToNextRequest())
    ;
}

void GuaConnectionToClient::dumpAndStopOtherEnd(bool dump, bool stop) const
{
  getOtherEnd().getPhandle().dumpAndStop(dump, stop);
}

// -----------------------------------------------------------------------
// Methods for class GuaReceiveControlConnection
// -----------------------------------------------------------------------
GuaReceiveControlConnection::GuaReceiveControlConnection(
     IpcEnvironment * env,
     short receiveDepth,
     const char *eye,
     GuaReceiveFastStart *guaReceiveFastStart)
     : IpcControlConnection(IPC_DOM_GUA_PHANDLE,eye),
       clientConnections_(env->getAllConnections(),env->getHeap()),
       failedConnections_(env->getAllConnections(),env->getHeap()),
       receiveBufferPool_(env->getHeap()),
       activeReceiveBuffers_(env->getHeap()),
       initialized_(FALSE),
       guaReceiveFastStart_(guaReceiveFastStart)
{

  // This process was created by the Guardian procedure PROCESS_CREATE_
  // and needs to open $RECEIVE to get its messages. All messages arrive
  // through $RECEIVE and then get dispatched to the appropriate
  // IpcConnection objects via a lookup. Any wait operation on any
  // connection to a server may therefore accept messages for other
  // client connections.

  // initialize data members
  env_                     = env;
  firstClientConnection_   = NULL;
  numReceivingConnections_ = 0;
  receiveFile_             = InvalidGuaFileNumber;
  receiveDepth_            = receiveDepth;
  maxIOSize_               = env_->getGuaMaxMsgIOSize();
  maxOutstandingIOs_       = 1; // Guardian limit
  numOutstandingIOs_       = 0;
  numOutstandingRequests_  = 0;
  beginTransTag_           = -1;
  txHandleValid_           = FALSE;
  memset (&txHandle_, 0, sizeof(SB_Transid_Type));
  activeTransReplyTag_     = GuaInvalidReplyTag;
  implicitTransReplyTag_   = GuaInvalidReplyTag;
  userTransReplyTag_       = GuaInvalidReplyTag;
  guaErrorInfo_            = GuaOK;

  // now open $RECEIVE
  if (guaReceiveFastStart_!= NULL && guaReceiveFastStart_->open_)
  {
    guaErrorInfo_ = guaReceiveFastStart_->openError_;
    receiveFile_ = guaReceiveFastStart_->receiveFile_;
  }
  else
    guaErrorInfo_ = BFILE_OPEN_((char *)"$RECEIVE",
			       8,
			       &receiveFile_,
			       0, // read-write
			       0, // shared
			       (short) maxOutstandingIOs_,
			       receiveDepth_,
  0,0,0,0); // no options
  if (guaErrorInfo_ != 0)
    {
      // We're in serious trouble, this process has just started
      // and it can't open $RECEIVE. This means we have to die.
      ABORT("Unable to open $RECEIVE");
    }
  MXTRC_2("GRCC::GRCC connection=%x, filenum=%d\n", this,  receiveFile_);
       
  // use setmode 74 to turn off the automatic CANCEL upon AWAITIOX timeout
  if (guaReceiveFastStart_ == NULL)
  {
   _bcc_status stat = BSETMODE(receiveFile_,74,-1);
    if (_status_ne(stat))
      {
	// this is bad
	ABORT("Internal error on setmode($receive)");
      }
  }


  // MONITORNET is currently not available on NT and
  // it is not needed until there is support for multiple nsk expand nodes

  // now initiate the first READUPDATEX operation (which will complete
  // with an open message), even if we don't have a connection yet
  initiateReceive(TRUE);
}

IpcConnection * GuaReceiveControlConnection::getConnection() const
{
  return firstClientConnection_;
}

GuaReceiveControlConnection *
GuaReceiveControlConnection::castToGuaReceiveControlConnection()
{
  return this;
}

WaitReturnStatus GuaReceiveControlConnection::wait(IpcTimeout timeout, UInt32 *eventConsumed, IpcAwaitiox *ipcAwaitiox)
{
  MXTRC_FUNC("GRCC::wait");
  MXTRC_1("timeout=%d\n", timeout);
  // ---------------------------------------------------------------------
  // call AWAITIOX with the specified timeout
  // ---------------------------------------------------------------------
  IpcMessageBufferPtr bufferAddr = NULL;
  short msgType = 0;
  NABoolean controlReceived = FALSE;
  Int32 countTransferred;
  SB_Tag_Type ioTag = -1;
  NABoolean systemMessageReceived;

  // don't call AWAITIOX unless there are outstanding I/Os
  if (numOutstandingIOs_ == 0)
  {
    if (timeout > 0)           // is GuaConnectionToClient and it returns too soon
      usleep(timeout * 10000); // Delay here instead before returning if
    return WAIT_OK;
  }

  GuaErrorNumber retcode = GuaOK;
  NABoolean retry = TRUE;
  NABoolean setFirstClientToNull = FALSE;

  while (retry)
    {
      if(initialized_ || timeout != IpcInfiniteTimeout){
	_cc_status stat;
	if (ipcAwaitiox == NULL || !ipcAwaitiox->getCompleted())
	{
	  stat = BAWAITIOX(&receiveFile_,
				 (void **) &bufferAddr,
				 &countTransferred,
			         &ioTag,
				 timeout,
				 OMIT);
	}
	else
	{
	  stat = ipcAwaitiox->ActOnAwaitiox((void **)&bufferAddr,
				 &countTransferred,
				 &ioTag);
	}
	if (_status_ne(stat)) 
	  retcode = BFILE_GETINFO_(receiveFile_,&guaErrorInfo_);
	else
	  retcode = guaErrorInfo_ = GuaOK;
      }
      else { // not initialized && infinite timeout
	// Set the timeout to 1 min
	Lng32 newTimeOut= 100*60*1;
	NABoolean done = FALSE; 
	while(!done){
          _cc_status stat;
	  if (guaReceiveFastStart_ != NULL && guaReceiveFastStart_->awaitiox_)
	  {
	    guaReceiveFastStart_->awaitiox_ = FALSE;
	    stat = guaReceiveFastStart_->awaitioxStatus_;
	    bufferAddr = (char *)&guaReceiveFastStart_->readBuffer_[0];
	    countTransferred = guaReceiveFastStart_->awaitioxCountTransferred_;
	    ioTag = guaReceiveFastStart_->ioTag_;
	  }
	  else
	  {
	    stat = BAWAITIOX(&receiveFile_,
				   (void **) &bufferAddr,
				   &countTransferred,
				   &ioTag,
				   newTimeOut);
	  }
	  if (guaReceiveFastStart_ != NULL && guaReceiveFastStart_->bufferData_ != NULL)
	  {
	    memcpy((char *)guaReceiveFastStart_->bufferData_, (char *)bufferAddr, countTransferred);
	    bufferAddr = (IpcMessageBufferPtr)guaReceiveFastStart_->bufferData_;
	    retcode = guaReceiveFastStart_->fileGetInfoError_;
	    guaErrorInfo_ = guaReceiveFastStart_->awaitioxError_;
	    guaReceiveFastStart_->bufferData_ = NULL;
	  }
	  else
	  {
	    if (_status_ne(stat)) 
	      retcode = BFILE_GETINFO_(receiveFile_,&guaErrorInfo_);
	    else
	      retcode = guaErrorInfo_ = GuaOK;
	  }
	  if(guaErrorInfo_ != GuaTimeoutErr) 
	    {
	      // we received something or some other error than time-out has ocurred
	      env_->setEvent(TRUE, AEVENT);
	      done = true;
	    }
	} // while
      } // else

#ifdef LOG_WAIT_TIMEOUT
      IpcGuaLogTimestamp((IpcConnection *) NULL);
      cerr << "GRCC:timeout = " << timeout << "  ioTag = " << ioTag << endl;
#endif


      if (!((guaErrorInfo_== 4004) && !env_->breakEnabled()))
	{
	  // Not to retry unless error is FE_EINTR (4004)
	  retry = FALSE;
      
	  if (retcode != GuaOK)
	    guaErrorInfo_ = retcode; // not even FILE_GETINFO_ worked

	  systemMessageReceived = (guaErrorInfo_ == GuaSysmsgReceived);
	  if (systemMessageReceived)
	    msgType = *((short *) bufferAddr);

	  if (guaErrorInfo_ == GuaTimeoutErr)
	    {
	      // ----------------------------------------------------------
	      // AWAITIOX timed out, nothing to do here
	      // ----------------------------------------------------------
	      return WAIT_OK;
	    }

	  // ----------------------------------------------------------------
	  // Check for fatal errors (if we fail here or while reading the
	  // receive info we abort, since there doesn't seem any reasonable
	  // error recovery for such errors)
	  // ---------------------------------------------------------------
	  if (guaErrorInfo_ != GuaOK AND
	      guaErrorInfo_ != GuaSysmsgReceived)
	    {
	      // Are there any cases where we need to retry? $$$$
	      // Error recovery from this?
	      ABORT("Fatal error in AWAITIOX($RECEIVE)");
	    }
	}
    } // while

  // ---------------------------------------------------------------------
  // call FILE_GETRECEIVEINFO_ to find out about the client
  // ---------------------------------------------------------------------

  GuaReceiveInfo receiveInfo;

  if (guaReceiveFastStart_ != NULL && guaReceiveFastStart_->fileGetReceiveInfo_)
  {
    guaErrorInfo_ = guaReceiveFastStart_->fileGetReceiveInfoError_;
    memcpy((char *)&receiveInfo, &guaReceiveFastStart_->receiveInfo_, sizeof(GuaReceiveInfo));
    guaReceiveFastStart_->fileGetReceiveInfo_ = FALSE;
  }
  else
  guaErrorInfo_ = BFILE_GETRECEIVEINFO_((FS_Receiveinfo_Type *)&receiveInfo);
  if (systemMessageReceived && (msgType == ZSYS_VAL_SMSG_CLOSE))
  {
    Int32 cpu, pin, nodeNumber;
    SB_Int64_Type seqNum = -1;
    receiveInfo.phandle_.decompose(cpu, pin, nodeNumber
       , seqNum
       );

    env_->closeTrace(__LINE__, receiveInfo.clientFileNumber_, cpu, pin, nodeNumber);
  }

  if (guaErrorInfo_ != GuaOK)
    {
      ABORT("Fatal error in FILE_GETRECEIVEINFO_");
    }

  // ---------------------------------------------------------------------
  // Successfully received a message and we know now where it came from
  // ---------------------------------------------------------------------

  // this nowait I/O just completed
  numOutstandingIOs_--;

  // we already assume that the message will get delivered to some connection
  numReceivingConnections_--;

  // eventually we'll have to reply to this
  numOutstandingRequests_++;

  // we got a new transaction id as a result of the completed read on
  // $RECEIVE, switch back to the explicitly selected transaction of the user.
  // Note: If either request lth or reply lth is zero then IO is for secondary
  // chunks of multi chunk msg, ignore currently received trans reply tag.
  // Save last non-chunk message reply tag to restore implicit transaction
  // context after replying because REPLYX looses current trans context.
  activeTransReplyTag_ = receiveInfo.replyTag_;
   if (countTransferred && receiveInfo.maxReplyLen_)
    { 
    implicitTransReplyTag_ = activeTransReplyTag_;
    }
  switchToUserTransid();

#ifdef LOG_RECEIVE
  Int64 jts = JULIANTIMESTAMP();
  MyGuaProcessHandle me;
  IpcProcessId other(receiveInfo.phandle_);
  char meAsAscii[200];
  char otherAsAscii[200];
  me.toAscii(meAsAscii,200);
  other.toAscii(otherAsAscii,200);

  cerr << "(" << 
    // NT has problems printing out an Int64
    (ULng32) jts
       << "): " << meAsAscii << " from " << otherAsAscii
       << "(" << receiveInfo.clientFileNumber_ << ") "
       << "Received " << countTransferred << " bytes with max reply len "
       << receiveInfo.maxReplyLen_ 
       << endl;
#endif /* LOG_RECEIVE */

  // redrive the READUPDATE process to see whether we can start another I/O
  initiateReceive(FALSE);

  // ---------------------------------------------------------------------
  // find the buffer in the list of outstanding receive buffers
  // (only one entry at this time, because of Guardian limits)
  // ---------------------------------------------------------------------
  IpcMessageBuffer *receivedBuffer = NULL;
  
  for (CollIndex i = 0;
       i < activeReceiveBuffers_.entries() AND receivedBuffer == NULL;
       i++)
    {
      if (activeReceiveBuffers_[i]->data(0) == bufferAddr)
	{
	  // found it
	  receivedBuffer = activeReceiveBuffers_[i];
	  activeReceiveBuffers_.remove(receivedBuffer);
	}
    }
  
#ifdef LOG_RECEIVE
  cerr << "Found the active receive buffer " << (Lng32) receivedBuffer << endl;
#endif

  if (receivedBuffer == NULL)
    {
      // couldn't find the buffer
      ABORT("Internal error: receive buffer not found");
      // could also reply with error but this is an indicator for a
      // grave error somewhere
    }
  
  // ---------------------------------------------------------------------
  // Find the connection (if possible)
  // ---------------------------------------------------------------------
  //TBD
  GuaConnectionToClient *conn = findConnection(receiveInfo.clientFileNumber_, receiveInfo.phandle_);
  if (countTransferred > 0 && systemMessageReceived == FALSE && conn->newClientConnection(receivedBuffer) == TRUE)
  {
    if (conn)
	{
      Int32 cpu, pin, nodeNumber;
      SB_Int64_Type seqNum = -1;
      receiveInfo.phandle_.decompose(cpu, pin, nodeNumber
       , seqNum
       );
      env_->closeTrace(__LINE__, receiveInfo.clientFileNumber_, 
                       cpu, pin, seqNum); // Persistent open simulated close
      conn->close();
      clientConnections_ -= conn->getId();
      //delete conn;
      if (conn == firstClientConnection_)
        setFirstClientToNull = TRUE;
	}
    conn = new(env_->getHeap()) GuaConnectionToClient(
	       env_,
	       IpcProcessId(receiveInfo.phandle_),
	       receiveInfo.clientFileNumber_,
	       this);
    if (firstClientConnection_ == NULL)
      firstClientConnection_ = conn;
    clientConnections_ += conn->getId();

    incrNumRequestors();

    msgType = ZSYS_VAL_SMSG_OPEN;
    actOnSystemMessage(msgType,
		       NULL, // buffer Address
		       0, // count transferred
		       receiveInfo.clientFileNumber_,
		       receiveInfo.phandle_,
		       conn);

  }

  // ---------------------------------------------------------------------
  // Now we got all the info we need: the buffer, who sent it, and what
  // connection it is for (if it is for any connection). Next thing to
  // do is to process the received data.
  // ---------------------------------------------------------------------
  if (systemMessageReceived)
    {
      // -----------------------------------------------------------------
      // received a system message
      // -----------------------------------------------------------------

      // a system message doesn't have the usual header, its type is
      // delivered in the first 2 bytes instead

#     ifdef LOG_RECEIVE
      cerr << "System message received: " << msgType << endl;
#     endif
			MXTRC_1("System message received, msgType=%d\n", msgType);
      NABoolean repliedToSystemMessage = FALSE;
      // by default, reject any system requests (like CONTROL, etc.)
      GuaErrorNumber sysMsgRetcode = GuaOK;
    
      // switch on the system message type
      switch (msgType)
	{
	case ZSYS_VAL_SMSG_CPUDOWN:
	  {
	    // a local CPU went down, mark all clients that are
	    // on that CPU as dead

	    zsys_ddl_smsg_cpudown_def *msg =
	      (zsys_ddl_smsg_cpudown_def *) bufferAddr;

	    IpcNodeName myNodeName =
	      IpcProcessId(MyGuaProcessHandle()).getNodeName();
	    for (CollIndex i = 0; clientConnections_.setToNext(i); i++)
	      {
		GuaConnectionToClient *c =
		  clientConnections_.element(i)->
		  castToGuaConnectionToClient();

		if (c != NULL AND
		    c->getOtherEnd().match(myNodeName,msg->z_cpunumber))
                {
		  markAsDead(c,GuaClientCpuDown);
                  if (c == firstClientConnection_)
                  {
                    setFirstClientToNull = TRUE;
                    // ALM CR 5373 - If we get CPU down system 
                    // message before close system message,
                    // we need to let make sure the 
                    // EspGuaControlConnection::actOnSystemMessage
                    // gets a chance to stop this process
                    // before we set the firstClientConnection_
                    // to NULL at the end of this method.
                    // Do this by pretending this system 
                    // message came from the firstClientConnection_.
                    if ( env_->getAllConnections()->
                         getPendingIOs().isEsp() )
                    {
                      conn = (GuaConnectionToClient *)firstClientConnection_;
/*                      if (firstClientConnection_)
                        SQLMXLoggingArea::logExecRtInfo(
                          __FILE__, __LINE__,
                         "Processed CPU down system message,  "
                         "firstClientConnection_ not NULL", 0);
                      else
                        SQLMXLoggingArea::logExecRtInfo(
                          __FILE__, __LINE__,
                         "Processed CPU down system message,  "
                         "but firstClientConnection_ was NULL", 0);
*/
                    }
                  }
                }
	      }
	  }
	  break;
	  
	case ZSYS_VAL_SMSG_REMOTECPUDOWN:
	  {
	    // a remote CPU went down, mark all clients that are
	    // on that CPU as dead

	    zsys_ddl_smsg_remotecpudown_def *msg =
	      (zsys_ddl_smsg_remotecpudown_def *) bufferAddr;

	    IpcCpuNum remoteCpu = msg->z_cpunumber;
	    // null-terminate the node name (this may overwrite
	    // over the struct, but what the heck, we know we
	    // have used a very long receive buffer)
	    msg->z_nodename[msg->z_nodename_len] = 0;
	    // create an IpcNodeName from it
	    IpcNodeName remoteNodeName(IPC_DOM_GUA_PHANDLE,
				       &msg->z_nodename[1]);

	    for (CollIndex i = 0; clientConnections_.setToNext(i); i++)
	      {
		GuaConnectionToClient *c =
		  clientConnections_.element(i)->
		  castToGuaConnectionToClient();

		if (c != NULL AND
		    c->getOtherEnd().match(remoteNodeName,
					   remoteCpu))
                {
		  markAsDead(c,GuaClientCpuDown);
                  if (c == firstClientConnection_)
                    setFirstClientToNull = TRUE;
                }
	      }
	  }
	  break;
	  
        case XZSYS_VAL_SMSG_SHUTDOWN:
          {
            NAExit(0);
          }
          break;
	case ZSYS_VAL_SMSG_OPEN:
	  {
	    char otherAscii[200];
	    receiveInfo.phandle_.toAscii(otherAscii, 200);	  	
	    // open message: create a new connection

#ifndef NDEBUG
            // This error injection code *could* work just fine in
            // release code, but for performance reasons, we only 
            // have it for DEBUG.
            const short injectedError = 48;
            if (fakeErrorFromNSK(injectedError, &receiveInfo.phandle_))
              {
              sendReplyData(NULL,0,receiveInfo.replyTag_,NULL,injectedError);
              return WAIT_OK;
              }
#endif
	    // create a new Guardian connection to the client who
	    // is opening us
	    if (conn)
	      {
		conn->close();
		clientConnections_ -= conn->getId();
		//delete conn;
                if (conn == firstClientConnection_)
                  setFirstClientToNull = TRUE;
	      }
	    conn = new(env_->getHeap()) GuaConnectionToClient(
		 env_,
		 IpcProcessId(receiveInfo.phandle_),
		 receiveInfo.clientFileNumber_,
		 this);
	    MXTRC_4("GRCC::wait new connection=%x id=%d info=%s.%d\n", conn, conn->getId(), otherAscii, receiveInfo.clientFileNumber_);
	    
	    // reply to the open message right here and set the
	    // open label to the id of the new connection
	    zsys_ddl_smsg_open_reply_def openReply;

	    openReply.z_msgnumber = ZSYS_VAL_SMSG_OPEN;
	    openReply.z_openid = (short) conn->getId();

	    sendReplyData((IpcMessageBufferPtr) &openReply,
	                  controlReceived ? 0 : sizeof(openReply),
			  receiveInfo.replyTag_,
			  NULL,
			  GuaOK);
	    repliedToSystemMessage = TRUE;

	    // if this is the first client then remember it
	    if (firstClientConnection_ == NULL)
	      firstClientConnection_ = conn;
	    clientConnections_ += conn->getId();

	    incrNumRequestors();
	  }
	  break;
	  
	case ZSYS_VAL_SMSG_CLOSE:
	  {
	    // close message, remove the corresponding connection

	    //zsys_ddl_smsg_close_def *msg = 
	       //(zsys_ddl_smsg_close_def *) bufferAddr;
			MXTRC_1("GRCC::wait close message: conn=%x\n", conn);

	    if (conn)
	      {
		conn->close();
		clientConnections_ -= conn->getId();
		//delete conn;
                if (conn == firstClientConnection_)
                  setFirstClientToNull = TRUE;
	      }

	    // - do we need to reply to the CLOSE request?
	    //repliedToSystemMessage = TRUE;
	  }
	  break;
	  
	case ZSYS_VAL_SMSG_NODEDOWN:
	  {
	    // Node went down, all clients from that node are dead

	    zsys_ddl_smsg_nodedown_def *msg =
	      (zsys_ddl_smsg_nodedown_def *) bufferAddr;

	    // null-terminate the node name (this may overwrite
	    // over the struct, but what the heck, we know we
	    // have used a very long receive buffer)
	    msg->z_nodename[msg->z_nodename_len] = 0;
	    // create an IpcNodeName from it
	    IpcNodeName remoteNodeName(IPC_DOM_GUA_PHANDLE,
				       &msg->z_nodename[1]);

	    for (CollIndex i = 0; clientConnections_.setToNext(i); i++)
	      {
		GuaConnectionToClient *c =
		  clientConnections_.element(i)->
		  castToGuaConnectionToClient();

		if (c != NULL AND
		    c->getOtherEnd().match(remoteNodeName))
                {
		  markAsDead(c,GuaClientNodeDown);
                  if (c == firstClientConnection_)
                    setFirstClientToNull = TRUE;
                }
	      }
	  }
	  break;
	  
	default:
	  // don't care about other messages, if they are requests
	  // to do something then make sure we reject that request
	  sysMsgRetcode = GuaInvalidFileType;
	  break;
	}

//  the tdm_service will die if we stop before we reply
//  in some cases, actOnSystemMessage calls Exit(0);
//  this is an error prone fix, if actOnSystemMessage ever wants to reply itself, it would be too late
//
      // if we haven't replied in the individual case
      // then reply with an empty message
      if (NOT repliedToSystemMessage)
	sendReplyData(NULL,0,receiveInfo.replyTag_,NULL,sysMsgRetcode);
     
      // now let any derived class do its thing with the system message
      // (treat this as a callback and return from wait() calls)
      // if it's not a close message from an orphan file
      if (!(msgType == ZSYS_VAL_SMSG_CLOSE && controlReceived == FALSE && conn == NULL ))
      {
	env_->getAllConnections()->bumpCompletionCount();
	actOnSystemMessage(msgType,
			   bufferAddr,
			   countTransferred,
			   receiveInfo.clientFileNumber_,
			   receiveInfo.phandle_,
			   conn);
      }

      
      // we're done with the received buffer
      recycleReceiveBuffer(receivedBuffer);

      // initiate a new receive operation for the next system message
      initiateReceive();
    }
  else
    {
      // -----------------------------------------------------------------
      // got a message in one of the buffers that we know, now
      // dispatch it to the connection that it belongs to
      // -----------------------------------------------------------------
      receivedBuffer->setReplyTag(receiveInfo.replyTag_);
      receivedBuffer->setMaxReplyLength(receiveInfo.maxReplyLen_);

      if (conn != NULL)
	{
	  // -------------------------------------------------------------
	  // Now we've found the connection that is supposed to
	  // receive this message (recipientConn becomes owner of buffer)
	  // -------------------------------------------------------------
          if (countTransferred >= sizeof(InternalMsgHdrInfoStruct))
          {
            InternalMsgHdrInfoStruct *imhis = (InternalMsgHdrInfoStruct *)
                                                 bufferAddr;
            conn->env()->addIpcMsgTrace(conn, IpcEnvironment::ACCEPT,
                                        (void *)bufferAddr, countTransferred,
                                        (imhis->isLastMsgBuf()? 1: 0),
                                        imhis->getSeqNum());
#if 0
            conn->incrReceivedMsgHdrInd();
            MsgTraceEntry *msgTraceEntry = (MsgTraceEntry *)(conn->receivedMsgHdr() + sizeof(MsgTraceEntry) * conn->receivedMsgHdrInd());
            memcpy((void *)&msgTraceEntry->internalMsgHdrInfoStruct_, (void *)bufferAddr, sizeof(InternalMsgHdrInfoStruct));
            msgTraceEntry->bufAddr_ = (void *)bufferAddr;
            msgTraceEntry->sentReceivedLength_ = (unsigned int)countTransferred;
#endif
          }
	  conn->acceptBuffer(receivedBuffer,countTransferred);
	}
      else if (env_->isPersistentProcess())
        {
          // A recreated persistent process can receive messages
          // from an open of a previous instance of the process
          // see bug 1997, 2468 and 2469
	  sendReplyData(NULL,0,receiveInfo.replyTag_,
			NULL,FEWRONGID);
	  recycleReceiveBuffer(receivedBuffer);
	  initiateReceive();
        }
      else
	{
	  // couldn't find the connection to the client, reply with
	  // a special error in the hope that this won't cause a deadlock
	  // $$$$ should we abort instead? This looks like a bad error.
	  // (Current reason for not just aborting is that we believe that
	  // some open connections may have been destroyed at user's request)
	  
          receiveInfo.phandle_.dumpAndStop(true, false);
	  sendReplyData(NULL,0,receiveInfo.replyTag_,
			NULL,GuaIpcApplicationErr);
	  recycleReceiveBuffer(receivedBuffer);
	  initiateReceive();
	  ABORT("Couldn't find connection to client"); // for now, debug this
	}
    }
  if (setFirstClientToNull)
    firstClientConnection_ = NULL;
  return WAIT_INTERRUPT;
}

void GuaReceiveControlConnection::actOnSystemMessage(
     short                  messageNum,
     IpcMessageBufferPtr    /*sysMsg*/,
     IpcMessageObjSize      /*sysMsgLen*/,
     short                  /*clientFileNumber*/,
     const GuaProcessHandle & /*clientPhandle*/,
     GuaConnectionToClient  *connection)
{
  // The default implementation ignores all system messages, except that
  // it makes sure that only one client opens the process.
  if (getNumRequestors() > 1)
  {
    if (firstClientConnection_)
    {
      connection->dumpAndStopOtherEnd(true, false);
      if (firstClientConnection_->getOtherEnd() == 
          connection->getOtherEnd().getPhandle())
        ;  // already have a core.
      else  
        firstClientConnection_->dumpAndStopOtherEnd(true, false);
    }
    ABORT("More than one OPEN system message received");
  }
  else if (getNumRequestors() == 0 AND initialized_)
  {
    // in the default implementation the server stops if its client
    // goes away
    
    // for debugging it is sometimes helpful to print a message for this
    // ABORT("Lost connection to client");
    
#ifdef LOG_RECEIVE
    cerr << "No requestors exist. About to call NAExit()..." << endl;
#endif
    NAExit(0);
  }
  else if (NOT initialized_ AND getNumRequestors() > 0)
  {
    // the first requestor came in
    initialized_ = TRUE;
  }
  
  // This method should be overridden in derived classes if a process
  // wants to handle more than one client. The derived class needs to
  // assign a task to each newly created connection that is passed with
  // an open message. The derived class also needs to decide what to
  // do when it loses a client process.
}

void GuaReceiveControlConnection::sendReplyData(
     IpcMessageBufferPtr data,
     IpcMessageObjSize   size,
     short               replyTag,
#ifdef LOG_RECEIVE
     IpcConnection       *conn,
#else
     IpcConnection       *, // avoid compiler warning
#endif
     GuaErrorNumber      retcodeToClient)
{
  // must call this for a single chunk
  assert(size <= maxIOSize_ AND
	 replyTag != GuaInvalidReplyTag);

  // call REPLYX
   Int32 countWritten;

  _cc_status stat;
  if (guaReceiveFastStart_ != NULL && guaReceiveFastStart_->replyx_)
  {
    stat = guaReceiveFastStart_->replyxstatus_;
    countWritten = guaReceiveFastStart_->replyxCountWritten_;
    guaReceiveFastStart_->replyx_ = FALSE;
  }
  else
  {
    // Reset the original transaction, if one exists.
    if (txHandleValid_)
       TMF_SETTXHANDLE_((short *)&txHandle_); 
    stat = BREPLYX(data,
			     size,
			     &countWritten,
			     replyTag
			     ,retcodeToClient
			     );
  }
  if (_status_ne(stat) OR (ULng32)countWritten != size)


    {
      // get a Guardian error code
      GuaErrorNumber errcode2 = BFILE_GETINFO_(receiveFile_,&guaErrorInfo_);

      if (errcode2 != GuaOK)
	guaErrorInfo_ = errcode2;

      // sorry, if something goes wrong here we have no way to let
      // the client or master know about it, all we can do is to die
      char buf[100];
      str_sprintf(buf, "REPLYX returned error %d", (Int32) guaErrorInfo_);
      ABORT(buf);
      // don't die in cases where the client caused the fault (if any)
    }

  // we have one less outstanding REPLYX
  numOutstandingRequests_--;

  // REPLYX loses the current transaction id, restore the user-defined one,
  // unless we did a reply on the user-defined current transaction
  activeTransReplyTag_ = GuaInvalidReplyTag;
  if (replyTag == implicitTransReplyTag_)
    implicitTransReplyTag_ = GuaInvalidReplyTag;
  if (replyTag == userTransReplyTag_)
    userTransReplyTag_ = GuaInvalidReplyTag;

  switchToUserTransid();

#ifdef LOG_RECEIVE
  if (conn)
    IpcGuaLogTimestamp(conn);
  else
    cerr << "Without use of a connection: ";

  cerr << "Replying with " << countWritten << " bytes, tag " << replyTag
       << ", err " << retcodeToClient
       << endl;
#endif

}

void GuaReceiveControlConnection::initiateReceive(NABoolean newReceive)
{
MXTRC_FUNC("GRCC::initiateReceive");
  Int32 count_read = 0;

  if (newReceive)
    {
      // A connection specifies TRUE when it initially calls this;
      // newReceive is set to FALSE when we simply want to start
      // a READUPDATEX call that hasn't been started earlier due to
      // the maxOutstandingIOs_ limit.
      numReceivingConnections_++;
    }

  // limit the number of outstanding IOs to the specified maximum.
  if (numOutstandingIOs_ >= maxOutstandingIOs_   OR
       numOutstandingIOs_ + numOutstandingRequests_ >= receiveDepth_)
    return;

  // get a previously used buffer or allocate a new one
  IpcMessageBuffer *buffer = NULL;

  // hunt for a free receive buffer from the pool of recycled ones
  for (CollIndex i = 0;
       buffer == NULL AND i < receiveBufferPool_.entries();
       i++)
    {
      if (receiveBufferPool_[i]->getRefCount() == 1)
	{
	  buffer = receiveBufferPool_[i];
	  receiveBufferPool_.removeAt(i);
	}
    }
 
  if (buffer == NULL)
    {
      CollHeap *heap = (env_ ? env_->getHeap() : NULL);
      buffer = IpcMessageBuffer::allocate(maxIOSize_, NULL, heap, 0);
      if (buffer == NULL)
	{
	  ABORT("Out of memory while allocating a receive buffer");
	}
    }

  // insert the buffer into the list of buffers that have outstanding
  // I/Os
  activeReceiveBuffers_.insert(buffer);

  // call READUPDATEX
  _cc_status stat;
  if (guaReceiveFastStart_ != NULL && guaReceiveFastStart_->readUpdate_)
  {
    guaReceiveFastStart_->readUpdate_ = FALSE;
    stat = guaReceiveFastStart_->readUpdateStatus_;
    guaReceiveFastStart_->bufferData_ = (unsigned char *)buffer->data(0);
  }
  else
  {
    stat = BREADUPDATEX(
         receiveFile_,
         (char *) buffer->data(0),
         (MINOF(buffer->getBufferLength(),maxIOSize_)),
         &count_read
         );
  }

  if (_status_ne(stat))
    {
      // get a Guardian error code
      short errcode2 = BFILE_GETINFO_(receiveFile_,&guaErrorInfo_);

      // sorry, if something goes wrong here we have no way to let
      // the client or master know about it, all we can do is to die
      ABORT("Error in READUPDATEX");
    }
  else
    {
      // adjust the number of outstanding READUPDATEX operations
      numOutstandingIOs_++;
    }
}

void GuaReceiveControlConnection::switchToUserTransid()
{
}

void GuaReceiveControlConnection::setOriginalTransaction(short *txHandle)
{
  memcpy(&txHandle_, txHandle, sizeof(SB_Transid_Type));
}
short * GuaReceiveControlConnection::getOriginalTransaction()
{
  return (short *)&txHandle_;
}
void GuaReceiveControlConnection::clearOriginalTransaction()
{
  memset(&txHandle_, 0, sizeof(SB_Transid_Type));
}

GuaConnectionToClient * GuaReceiveControlConnection::findConnection(
     short openLabel)
{
  // we were clever enough to let Guardian remember the connection
  // id for us in the open label (alternatively, we could find
  // the connection that matches the given phandle/file#)
  if (clientConnections_.contains(openLabel))
    {
      return
	clientConnections_.element(openLabel)->castToGuaConnectionToClient();
    }
  MXTRC("GRCC::findConnection false\n");
  return NULL;
}

GuaConnectionToClient * GuaReceiveControlConnection::findConnection(
     short                  clientFileNumber,
     const GuaProcessHandle &clientPhandle)
{
  // search all connections for a match
  for (CollIndex i = 0; clientConnections_.setToNext(i); i++)
    {
      GuaConnectionToClient *c =
	clientConnections_.element(i)->castToGuaConnectionToClient();

      if (c != NULL AND c->thisIsMyClient(clientPhandle,clientFileNumber))
	return c;
    }

  return NULL;
}

void GuaReceiveControlConnection::recycleReceiveBuffer(IpcMessageBuffer *b)
{
  if (b->getBufferLength() == maxIOSize_)
    {
      // this buffer has the right length to be kept in the receive buffer pool
      b->setReplyTag(GuaInvalidReplyTag);
      b->setMaxReplyLength(0);
      b->addCallback(NULL);
      receiveBufferPool_.insert(b);
    }
  else
    {
      // good bye
      b->decrRefCount();
    }
}

void GuaReceiveControlConnection::markAsDead(GuaConnectionToClient *c,
					     GuaErrorNumber gerr)
{
  // this connection is no longer part of the set of good client
  // connections
  clientConnections_ -= c->getId();
  failedConnections_ += c->getId();

  // tell the connection, too
  c->close(TRUE,gerr);

  if (clientConnections_.isEmpty())
    getEnv()->notifyNoOpens();
}



void GuaReceiveControlConnection::waitForMaster()
{
      int openWaitSeconds = 600;
      const char* owsEnvVar = getenv("SQL_SRVR_OPEN_WAIT_SECONDS");
      if (owsEnvVar)
      {
        int o = atoi(owsEnvVar);
        if (o > 0)
          openWaitSeconds = o;
      }
      int maxWaitTime = openWaitSeconds;
      do {
        struct timespec startedOpenWaitTs;
        if (clock_gettime(CLOCK_MONOTONIC, &startedOpenWaitTs))
        {
          char buf[256];
          str_sprintf(buf, "clock_gettime failed, errno %d", errno);
          ABORT(buf);
        }
        Int64 timeStart = ComRtGetJulianFromUTC(startedOpenWaitTs);
      
	wait(100 * openWaitSeconds);
        if (getConnection() != NULL)
          break;

        struct timespec nowOpenWaitTs;
        if (clock_gettime(CLOCK_MONOTONIC, &nowOpenWaitTs))
        {
          char buf[256];
          str_sprintf(buf, "clock_gettime failed, errno %d", errno);
          ABORT(buf);
        }
        Int64 timeNow = ComRtGetJulianFromUTC(nowOpenWaitTs);
        openWaitSeconds -= ((timeNow - timeStart) / (1000 * 1000));
      } while (openWaitSeconds > 0);

      if (getConnection() == NULL)
      {
        char msg[256];
        sprintf(msg, 
                "Server exiting after waiting %d seconds for initial open.",
                maxWaitTime);
        SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
        NAExit(0);
      }
      env_->setStopAfter(maxWaitTime);

}

// -----------------------------------------------------------------------
// Methods for class IpcGuardianServer
// -----------------------------------------------------------------------

IpcGuardianServer::IpcGuardianServer(
     IpcServerClass * serverClass,
     ComDiagsArea   ** /* diags */,
     CollHeap       * /* diagsHeap */,
     const char     * nodeName,
     const char     * className,
     IpcCpuNum      cpuNum,
     IpcPriority    priority,
     IpcServerAllocationMethod allocMethod,
     short          uniqueTag,
     NABoolean      usesTransactions,
     NABoolean      debugServer,
     NABoolean      waitedStartup,
     Lng32           maxNowaitRequests,
     const char     * overridingDefineForProgFile,
     const char     * processName,
     NABoolean      parallelOpens) : IpcServer(NULL,
					       serverClass)
{
  serverState_                 = INITIAL;
  nodeName_                    = NULL;
  className_                   = className;
  cpuNum_                      = cpuNum;
  actualCpuNum_                = cpuNum;
  requestedCpuDown_            = FALSE;
  priority_                    = priority;
  allocMethod_                 = allocMethod;
  uniqueTag_                   = uniqueTag;
  usesTransactions_            = usesTransactions;
  debugServer_                 = debugServer;
  waitedStartup_               = waitedStartup;
  nowaitDepth_                 = (unsigned short) maxNowaitRequests;
  overridingDefineForProgFile_ = overridingDefineForProgFile;
  processName_                 = processName;
  parallelOpens_               = parallelOpens,
  guardianError_               = 0;
  procCreateError_             = 0;
  procCreateDetail_            = 0;
  activeMessage_               = NULL;
  nowaitedEspStartup_.nowaitedEspServer_ = &getServerClass()->nowaitedEspServer_;
  nowaitedEspStartup_.procCreateError_ = &procCreateError_;
  nowaitedEspStartup_.newPhandle_ = &newPhandle_;
  nowaitedEspStartup_.nowaitedStartupCompleted_ = &nowaitedStartupCompleted_;
  unhooked_ = false;
}

IpcGuardianServer *IpcGuardianServer::castToIpcGuardianServer()
{
  return this;
}

void IpcGuardianServer::stop()
{
  if (getServerClass()->getServerType() == IPC_SQLBDRS_SERVER)
    return;

  if (controlConnection_ && allocMethod_ != IPC_USE_PROCESS)
  {
    // Stop the server process (MXCMP or MXESP)
    if (unhooked_)
    {
       if (controlConnection_->castToGuaConnectionToServer())
       {
          controlConnection_->castToGuaConnectionToServer()-> closePhandle();
          return;
       }
    } 
    char procName[200];
    short procNameLen = 200;
    Int32 nid = 0;
    Int32 pid = 0;
    short result = 0;

    //Phandle wrapper in porting layer
    NAProcessHandle phandle((SB_Phandle_Type *)&(getServerId().getPhandle().phandle_));

    Int32 guaRetcode = phandle.decompose();
    
    if (!guaRetcode)
    { 
      msg_mon_stop_process_name(phandle.getPhandleString());
    }
  }
}

short IpcGuardianServer::workOnStartup(IpcTimeout timeout,
				       ComDiagsArea **diags,
				       CollHeap *diagsHeap)
{

  Int32 retcode = 0;
  MXTRC_FUNC("IpcGuardianServer::workonStartup");
  if (serverState_ == INITIAL)
    {
      // Check if the class name contains a slash. If this is the
      // case then change the allocation mode to SPAWN, since the
      // class name must be an OSS file name. In all other cases
      // leave the allocation method as is.
      for (Int32 i = 0; className_[i] != 0; i++)
	if (className_[i] == '/')
	  allocMethod_ = IPC_SPAWN_OSS_PROCESS;

      if (allocMethod_ == IPC_LAUNCH_GUARDIAN_PROCESS)
	{
	  // launch the process, it will be a new Guardian process
	  // and can run on any node/cpu; communication will be via
	  // Guardian WRITEREADX
	  launchProcess(diags,diagsHeap);
	  if (serverState_ == CREATING_PROCESS)
	  {
	    assert(waitedStartup_ == FALSE);
            return 0;
	  }
	}
      else if (allocMethod_ == IPC_SPAWN_OSS_PROCESS)
	{
	  // spawn a new OSS process, it is started in the local system
	  // and the communication is also via WRITEREADX
	  spawnProcess(diags,diagsHeap);
	}
      else if (allocMethod_ == IPC_USE_PROCESS)
        {
          useProcess(diags, diagsHeap);
        }
      else
	ABORT("Invalid process allocation method for Guardian Server");

    } // serverState_ == INITIAL
  else if (serverState_ == CREATING_PROCESS)
  {
    assert(waitedStartup_ == FALSE);
    launchProcess(diags,diagsHeap); // Call it a second time
  }

  if (serverState_ == ERROR_STATE)
    {
      if (diags && (allocMethod_ != IPC_USE_PROCESS))
	{
	  if ((!(**diags).contains(-2013)) && (!(**diags).contains(-2012))) // avoid generating redundant error
	    {
	      IpcAllocateDiagsArea(*diags,diagsHeap);

	      // Server process $0~string0 could not be created on $1~string1
              // - Operating system error $2~int0 on program file.
	      (**diags) << DgSqlCode(-2013) << DgString0(progFileName_) 
                        << DgInt0(guardianError_)
                        << DgNskCode(guardianError_);
              char location[100];
              getCpuLocationString(location);
              (**diags) << DgString1(location);
	    }
	}
      return guardianError_;
    }
  return 0;
} // IpcGuardianServer::workOnStartup()

void IpcGuardianServer::acceptSystemMessage(const char *sysMsg,
					    Lng32 sysMsgLength)
{
  short *msgType = (short *) sysMsg;

  // make sure we received at least the two bytes for the message type
  // or otherwise we'll read junk instead of the message type

  if (sysMsg == NULL)  // Temporary debugging aid
    return;

  assert(sysMsg != NULL AND sysMsgLength >= sizeof(msgType));

  // see include file zsysc.h
  switch (*msgType)
    {
    case ZSYS_VAL_SMSG_PROCCREATE:
      {
	zsys_ddl_smsg_proccreate_def *processCreateNowaitMsg =
	  (zsys_ddl_smsg_proccreate_def *) sysMsg;

	procCreateError_  = processCreateNowaitMsg->z_error;
	procCreateDetail_ = processCreateNowaitMsg->z_error_detail;

	if (processCreateNowaitMsg->z_error != 0 AND
	    processCreateNowaitMsg->z_error != 14 /*undef. externals*/)
	  {
	    guardianError_    = 4022;  // some generic Guardian error
	    // set the error code and set the state to ERROR_STATE, it is
	    // the responsibility of the user of the object to set the
	    // diagnostics area
	    serverState_      = ERROR_STATE;
	    return;
	  }
	else
	  {
	    // process was successfully created, now create a connection to it
	    IpcProcessId serverProcId(
		 (const GuaProcessHandle &) processCreateNowaitMsg->z_phandle);
	    NABoolean useGuaIpc = TRUE;
	    if (useGuaIpc)
              {
                controlConnection_ = new
                  (getServerClass()->getEnv()->getHeap())
                  GuaConnectionToServer(getServerClass()->getEnv(),
                                        serverProcId,
                                        usesTransactions_,
                                        nowaitDepth_,
                                        eye_GUA_CONNECTION_TO_SERVER,
                                        parallelOpens_, NULL, FALSE
                   );

                if (controlConnection_->getState() == IpcConnection::ERROR_STATE)
                  guardianError_ = controlConnection_->castToGuaConnectionToServer()->getGuardianError();
              }
	    // On NT and Linux startup message is not needed.
	    if (controlConnection_->getState() == IpcConnection::ERROR_STATE)
	      {
		serverState_ = ERROR_STATE;
		return;
	      }
	    else
	      {
		serverState_ = READY;
	      }

	  }
      }
      break;

    case ZSYS_VAL_SMSG_PROCSPAWN:
      {
	zsys_ddl_smsg_procspawn_def *processSpawnNowaitMsg =
	  (zsys_ddl_smsg_procspawn_def *) sysMsg;

	if (processSpawnNowaitMsg->z_errno != 0)
	  {
	    // set the error code and set the state to ERROR_STATE, it is
	    // the responsibility of the user of the object to set the
	    // diagnostics area
	    guardianError_     = (short) processSpawnNowaitMsg->z_errno;
	    procCreateError_   = processSpawnNowaitMsg->z_tpcerror;
	    procCreateDetail_  = processSpawnNowaitMsg->z_tpcdetail;
	    serverState_       = ERROR_STATE;
	    return;
	  }
	else
	  {
	    // process was successfully created, now create a connection to it
	    IpcProcessId serverProcId(
		 (const GuaProcessHandle &) processSpawnNowaitMsg->z_phandle);
	    
	    NABoolean useGuaIpc = TRUE;
	    if (useGuaIpc)
	      controlConnection_ = new
		(getServerClass()->getEnv()->getHeap())
       		GuaConnectionToServer(getServerClass()->getEnv(),
				      serverProcId,
				      usesTransactions_,
				      nowaitDepth_);
              
	    // created server process is immediately ready for use
	    serverState_ = READY;
	  }
      }
      break;

    default:
      ABORT("Invalid type of system message received");
    }
}


void NewProcessCallback(SB_Phandle_Type *newPhandle,
                        MS_Mon_NewProcess_Notice_def *newProcNotice)
{
   NowaitedEspStartup *nowaitedEspStartup = (NowaitedEspStartup*)newProcNotice->tag;
   NowaitedEspServer *nowaitedEspServer = nowaitedEspStartup->nowaitedEspServer_;
   Int32 *procCreateError = nowaitedEspStartup->procCreateError_;
   NABoolean *nowaitedStartupCompleted = nowaitedEspStartup->nowaitedStartupCompleted_;
   memcpy(*nowaitedEspStartup->newPhandle_,(void *)newPhandle, sizeof(SB_Phandle_Type));
   ESP_TRACE2("CB: ToAcq_m, tag: %p\n", nowaitedEspStartup);
   pthread_mutex_lock(&nowaitedEspServer->cond_mutex_);
   ESP_TRACE2("CB: Acq_m, tag: %p\n", nowaitedEspStartup);
   *procCreateError = newProcNotice->ferr;
   *nowaitedStartupCompleted = TRUE;
   nowaitedEspServer->callbackCount_ += 1;
//   if (nowaitedEspServer->startTag_ == nowaitedEspServer->callbackCount_ && nowaitedEspServer->completionCount_ == 0)
   if (nowaitedEspServer->waiting_)
     pthread_cond_signal(&nowaitedEspServer->cond_cond_);
   pthread_mutex_unlock(&nowaitedEspServer->cond_mutex_);
   ESP_TRACE2("CB: Rls_m, tag: %p\n", nowaitedEspStartup);
}

void IpcGuardianServer::launchNSKLiteProcess(ComDiagsArea **diags,
				     CollHeap *diagsHeap)
{
  NABoolean nowaitedStartupCompleted = FALSE;
  static bool sv_cmp_node_id_checked = false;
  static bool sv_cmp_node_id_mine = false;
  static bool sv_launch_unhooked_checked = false;
  static bool sv_launch_unhooked = false;
  static bool sv_launch_cmp_unhooked_checked = false;
  static bool sv_launch_cmp_unhooked = false;
  bool launch_hooked_special = false;

  bool noSeabaseDefTableRead = false;

  NSK_PORT_HANDLE	p_phandle;
  if (serverState_ == INITIAL)
  {
  // a character string with the program file name
  const Int32                    maxLengthOfCommandLineArgs = 32;
  char                         progFileName[(IpcMaxGuardianPathNameLength +
	                                         maxLengthOfCommandLineArgs)];
  char *                       environmentName= NULL;

  // if this assertion fails during testing then increase
  // the literal above.
  
  assert(strlen(" -guardian -debug") <= 32);

  // parameters to NSKProcessCreate

  short				p_pe;
  Int32				p_nowaitTag;

#define MAX_PROC_ARGS   10
#define SET_ARGV(argv,argc,argval) {argv[argc] = (char *) calloc(strlen(argval)+1, 1); \
    strcpy(argv[argc++], argval); }

  Int32                   largc = 0;
  char                  *largv[MAX_PROC_ARGS];
  MS_Mon_PROCESSTYPE processType = MS_ProcessType_Generic;

//NGG
  openTraceFile();

  // ---------------------------------------------------------------------
  // Set parameters for process_launch_
  // ---------------------------------------------------------------------

  // Pe
  
  if (cpuNum_ == IPC_CPU_DONT_CARE)
    p_pe = -1;  // use same cpu as caller
  else
    p_pe = (short)cpuNum_;

  // -----------------------------------------------------------------
  // create the program file name from the class name and the overriding
  // define name.
  //
  // for now, we form the name from an environment variable. if the
  // environment variable is not present then we form the name from
  // the class name. we look for environment variables of the form
  // =_ARK_???_PROG_FILE_NAME
  //
  // names which are formed from class names are hard coded below.
  //
  // the long term plan is to form the name from the registry while allowing
  // overrides for development and debugging purposes only
  // 
  // note we REQUIRE the name to be identical on each PE !!!
  // -----------------------------------------------------------------
  if (overridingDefineForProgFile_)
    environmentName = getenv(overridingDefineForProgFile_);
  if (environmentName == NULL)
  {
	// ---------------------------------------------------------------
	// The path of executables will be decided by NSKProcessCreate. 
	// ---------------------------------------------------------------

	if ((strcmp(className_,"arkesp")== 0) || (strcmp(className_,"arkespdbg") == 0))
	{
	  SET_ARGV(largv, largc, "tdm_arkesp");
          char *fastStartArg = getenv("ESP_FASTSTART");
	  if (fastStartArg == NULL || *fastStartArg != '1')
	    SET_ARGV(largv, largc, "-noespfaststart");
	  strcpy(progFileName,"tdm_arkesp");

	}
	else
	if ((strcmp(className_,"arkcmp")== 0) || (strcmp(className_,"arkcmpdbg") == 0))
	{
	  SET_ARGV(largv, largc, "tdm_arkcmp");
	  strcpy(progFileName,"tdm_arkcmp");
          if (!sv_launch_cmp_unhooked_checked)
          {
            char *lv_launch_unhooked = getenv("IPC_LAUNCH_CMP_UNHOOKED");
            if ((lv_launch_unhooked != NULL) &&  
                 (*lv_launch_unhooked == '1'))
	       sv_launch_cmp_unhooked = true;
            sv_launch_cmp_unhooked_checked = true;
          }
          launch_hooked_special = !sv_launch_cmp_unhooked;
	}
	else 
	if ((strcmp(className_,"arkcat")== 0) || (strcmp(className_,"arkcatdbg") == 0))
		strcpy(progFileName,"arkcat.exe");
	else
	if ((strcmp(className_,"arkustat")== 0) || (strcmp(className_,"arkustatdbg") == 0))
		strcpy(progFileName,"arkustat.exe");
	else
	if ((strcmp(className_,"udrserv")== 0) || (strcmp(className_,"udrservdbg") == 0))
	{
	  SET_ARGV(largv, largc, "tdm_udrserv");
	  strcpy(progFileName,"tdm_udrserv");
	}
	else
	if ((strcmp(className_,"qms")== 0) || (strcmp(className_,"qmsdbg") == 0))
        {
          SET_ARGV(largv, largc, "tdm_arkqms");
          strcpy(progFileName,"tdm_arkqms");
        }
        else
	if ((strcmp(className_,"qmp")== 0) || (strcmp(className_,"qmpdbg") == 0))
        {
          SET_ARGV(largv, largc, "tdm_arkqmp");
          strcpy(progFileName,"tdm_arkqmp");
        }
        else
	if ((strcmp(className_,"qmm")== 0) || (strcmp(className_,"qmmdbg") == 0))
        {
          SET_ARGV(largv, largc, "tdm_arkqmm");
          strcpy(progFileName,"tdm_arkqmm");
        }
	else 
	if (strcmp(className_,"bdrr")== 0) 
        {
            // This process should be started as hooked always
	    launch_hooked_special = true;

            SET_ARGV(largv, largc, "mxbdrdrc");
            strcpy(progFileName,"mxbdrdrc");
            if (cpuNum_ != IPC_CPU_DONT_CARE)
            {
               MS_Mon_Node_Info_Type info;
               if (msg_mon_get_node_info_detail(cpuNum_, &info) !=  XZFIL_ERR_OK)
                  p_pe = -1;
               else
               if (info.num_returned < 1 || ! (info.node[0].type & MS_Mon_ZoneType_Edge))
                  p_pe = -1; 
               else
                  p_pe = cpuNum_;
            }
            else
                p_pe = -1;
           
            
        }
        else
	{ serverState_ = ERROR_STATE;

      if (diags)
        {
          IpcAllocateDiagsArea(*diags,diagsHeap);
          (**diags) << DgSqlCode(-2011) << DgInt0(FEBADNAME)
            << DgString0(className_) << DgNskCode(FEBADNAME);
        }

      return;
	};
  }
  else if (strlen(environmentName) <= IpcMaxGuardianPathNameLength)
	strcpy(progFileName,environmentName);
  else
        strcpy(progFileName,"///invalid_env_var");

  strcpy(progFileName_, progFileName);	// for any error messages

  // nowait tag
  


    // ---------------------------------------------------------------------
    // Set the run time arguments in the command line
    // ---------------------------------------------------------------------
    SET_ARGV(largv, largc, "-guardian");

    if (debugServer_)
      SET_ARGV(largv, largc, "-debug");

    // ---------------------------------------------------------------------
    // start a new process on the specified PE with the specified
    // program file
    // ---------------------------------------------------------------------

    void * envp    = getServerClass()->getEnv()->getEnvVars();
    Lng32 envpLen = getServerClass()->getEnv()->getEnvVarsLen();
    Int32 server_nid = p_pe; /* multi fragment esp concurrent change */
    Int32 server_pid = 0;
    Int32 server_oid = 0;
    char process_name[100];
    char prog[MS_MON_MAX_PROCESS_PATH];

    const char *pwd = NULL;

    process_name[0] = 0;
    pwd = getenv("PWD");
    if (strlen(pwd) + 1 + strlen(progFileName) + 1 <
        MS_MON_MAX_PROCESS_PATH)
      {
        strcpy(prog, pwd);
        strcat(prog, "/");
        strcat(prog, progFileName);
      }
    else
      strcpy(prog, "///invalid prog file");


    if (!sv_cmp_node_id_checked)
    {
      char *lv_cmp_node_id = getenv("CMP_NODE_AFFINITY");
      if ((lv_cmp_node_id != NULL) &&  (*lv_cmp_node_id == '1'))
	  sv_cmp_node_id_mine = true;
      sv_cmp_node_id_checked = true;
    }
    if (sv_cmp_node_id_mine && ((strcmp(className_,"arkcmp")== 0) ||
	(strcmp(className_,"arkcmpdbg") == 0)))
    {
       Int32 nid;
       Int32 err = msg_mon_get_my_info (&nid,NULL,NULL,0,NULL,NULL,NULL,NULL);
       if (!err)
	  server_nid = nid;
    }
  
    if (launch_hooked_special)
       unhooked_ = false;
    else
    {
       if (!sv_launch_unhooked_checked)
       {
          char *lv_launch_unhooked = getenv("IPC_LAUNCH_UNHOOKED");
          if ((lv_launch_unhooked != NULL) &&  (*lv_launch_unhooked == '1'))
	     sv_launch_unhooked = true;
          sv_launch_unhooked_checked = true;
       }
       unhooked_ = sv_launch_unhooked;
    }

    //  strcpy(process_name, "$srv");

    if (waitedStartup_ == FALSE)
    {
      Int32 returnValue;
      nowaitedStartupCompleted_ = FALSE;

      nowaitedEspStartup_.nowaitedEspServer_->startTag_ += 1;
      // Temporarily ignore returnValue
      newPhandle_ = (void *)getServerClass()->getEnv()->getHeap()->allocateMemory(sizeof(SB_Phandle_Type));
      ESP_TRACE2("MT: Call MMSPNW, svr: %p\n", &nowaitedEspStartup_);

      bool retryStartProcess;
      do
      {
        actualCpuNum_ = server_nid;  // save requested CPU (might be IPC_CPU_DONT_CARE)
        returnValue =  msg_mon_start_process_nowait_cb2(NewProcessCallback,
			    prog,           /* prog */
			    process_name,   /* name */
			    process_name,   /* output process name */
			    largc,          /* args */
			    largv,
			    //0,              /* open */
			    //&server_oid,    /* oid */
			    processType, /* process type */
			    0,              /* priority */
			    0,              /* debug */
			    0,              /* backup */
			    (Int64)&nowaitedEspStartup_,
			    &server_nid,    /* nid */
	  		    &server_pid,
			    NULL,
			    NULL,
			    unhooked_);
        ESP_TRACE2("MT: Back MMSPNW, svr: %p\n", &nowaitedEspStartup_);
        if (actualCpuNum_ == IPC_CPU_DONT_CARE)
          actualCpuNum_ = server_nid;  // msg_mon_start_process_nowait_cb2 might have assigned server_nid
        if (returnValue == XZFIL_ERR_FSERR && server_nid != -1)
        {
          server_nid = -1;
          retryStartProcess = true;
          requestedCpuDown_ = TRUE;
        }
        else
          retryStartProcess = false;
      }
      while (retryStartProcess);
      ESP_TRACE2("MT: Back MMSPNW, svr: %p\n", &nowaitedEspStartup_);
      procCreateError_ = returnValue;
    }
   else
    {

      // should have a define for the name length
      if (processName_)
      {
         strncpy (process_name, processName_, 99);
      }

      actualCpuNum_ = server_nid;  // save requested CPU (might be IPC_CPU_DONT_CARE)
      Int32 returnValue = msg_mon_start_process2(
			    prog,           /* prog */
			    process_name,   /* name */
			    process_name,   /* output process name */
			    largc,          /* args */
			    largv,
			    &p_phandle,
			    0,              /* open */
			    &server_oid,    /* oid */
			    processType, /* process type */
			    0,              /* priority */
			    0,              /* debug */
			    0,              /* backup */
			    &server_nid,    /* nid */
	  		    &server_pid,
			    NULL,
			    NULL,
			    unhooked_);
      procCreateError_ = returnValue;
      if (actualCpuNum_ == IPC_CPU_DONT_CARE)
        actualCpuNum_ = server_nid;  // msg_mon_start_process2 might have assigned server_nid
    }
  
  
  
     serverState_ = CREATING_PROCESS;
   
     if (getenv("SQL_MSGBOX_PROCESS") != NULL)
       {
	 MessageBox( NULL, "Requester: Process Launched", (CHAR *)&progFileName, MB_OK|MB_ICONINFORMATION );
       };
   } // serverState_ == INITIAL
   else
   {
       assert(serverState_ == CREATING_PROCESS &&
            nowaitedEspStartup_.nowaitedEspServer_->waitedStartupArg_ != '1');
       ESP_TRACE2("MT: ToAcq_m, svr: %p\n" , &nowaitedEspStartup_);
       pthread_mutex_lock(&nowaitedEspStartup_.nowaitedEspServer_->cond_mutex_);
       while (nowaitedStartupCompleted_ == FALSE)
       {
	 ESP_TRACE1("MT: Acq_m - Wt_CV\n");
	 nowaitedEspStartup_.nowaitedEspServer_->waiting_ = TRUE;
	 pthread_cond_wait(&nowaitedEspStartup_.nowaitedEspServer_->cond_cond_, &nowaitedEspStartup_.nowaitedEspServer_->cond_mutex_);
	 ESP_TRACE1("MT: Acq_CV\n");
	 nowaitedEspStartup_.nowaitedEspServer_->waiting_ = FALSE;
       }
     // Callback for this ESP has occurred
     p_phandle = *(NSK_PORT_HANDLE *)newPhandle_;
     getServerClass()->getEnv()->getHeap()->deallocateMemory(newPhandle_);
     nowaitedEspStartup_.nowaitedEspServer_->completionCount_ += 1;
     nowaitedStartupCompleted = TRUE;
     if (nowaitedEspStartup_.nowaitedEspServer_->startTag_ == nowaitedEspStartup_.nowaitedEspServer_->completionCount_)
     {
       assert(nowaitedEspStartup_.nowaitedEspServer_->startTag_ == nowaitedEspStartup_.nowaitedEspServer_->callbackCount_);
       nowaitedEspStartup_.nowaitedEspServer_->startTag_ =
	 nowaitedEspStartup_.nowaitedEspServer_->callbackCount_ =
	 nowaitedEspStartup_.nowaitedEspServer_->completionCount_ = 0;
    }
    pthread_mutex_unlock(&nowaitedEspStartup_.nowaitedEspServer_->cond_mutex_);
    ESP_TRACE1("MT: Rls_m\n");
   }

  
  if (waitedStartup_  OR (procCreateError_ != NO_ERROR) OR nowaitedStartupCompleted)
    {
      // create a system message from the return info
      //

      zsys_ddl_smsg_proccreate_def sysmsg;

      str_pad((char *) &sysmsg, sizeof(sysmsg), 0);

      sysmsg.z_msgnumber = ZSYS_VAL_SMSG_PROCCREATE;
      sysmsg.z_tag       = -1;
      memcpy(&sysmsg.z_phandle, &p_phandle, sizeof(sysmsg.z_phandle));
      sysmsg.z_error     = procCreateError_;
      
      // the system message that otherwise would be sent to $RECEIVE
      // gets delivered right here in outputList
      ESP_TRACE2("MT: To call acceptSysMsg, svr: %p\n", &nowaitedEspStartup_);
      acceptSystemMessage((const char *) &sysmsg,
			  sizeof(sysmsg));
      ESP_TRACE2("MT: Back from acceptSysMsg, svr: %p\n", &nowaitedEspStartup_);
      if (serverState_ == ERROR_STATE)
	{
	  ESP_TRACE1("MT: Error in acceptSysMsg\n");
	  // something went wrong with process creation, non-parallel open
          // of the control connection, or initiation of of parallel
          // open of the control connection
	  if (diags)
           if (procCreateError_ == XZFIL_ERR_OK)
            // Diagnostics must be due to error on BFILE_OPEN_
             controlConnection_->populateDiagsArea(*diags,diagsHeap);
           else
	    populateDiagsAreaFromTPCError(*diags,diagsHeap);
	  return;
	}
   }

	return;
}

void IpcGuardianServer::launchProcess(ComDiagsArea **diags,
				      CollHeap *diagsHeap)
{
 
  launchNSKLiteProcess(diags,diagsHeap);
  return;
}

void IpcGuardianServer::spawnProcess(ComDiagsArea **diags,
				     CollHeap *diagsHeap)
{

  launchNSKLiteProcess(diags,diagsHeap);
  return;
}

void IpcGuardianServer::useProcess(ComDiagsArea **diags,
				      CollHeap *diagsHeap)
{
  SB_Phandle_Type procHandle;
  short usedlength;
  char processName[50];
  char *tmpProcessName;
  int rc;

  if (processName_ == NULL)
  {

    tmpProcessName = getServerClass()->getProcessName((short)cpuNum_, processName);
    // use diagsHeap for the time being
    Int32 len = str_len(processName);

    processName_ = new (getServerClass()->getEnv()->getHeap()) char[len+1];
    str_cpy_all((char *)processName_, (const char *)processName, len+1);
  }
  else
    tmpProcessName = (char *)processName_;

  GuaErrorNumber guaError = 0;
  short i = 0;
  while (i < 3)
  {
    rc = get_phandle_with_retry(tmpProcessName, &procHandle);
    if (rc != FEOK)
    {
      serverState_ = ERROR_STATE;
      guardianError_ = rc;
      if (diags)
      {
        IpcAllocateDiagsArea(*diags,diagsHeap);
        (**diags) << DgSqlCode(-2024) << DgString0(processName_) 
          << DgInt0(rc);
		
      }
      return;
    }
    else
    {
      //Phandle wrapper in porting layer
      NAProcessHandle phandle(&procHandle);

      rc = phandle.decompose();
      if (rc != 0)
      {
        serverState_ = ERROR_STATE;
        guardianError_ = rc;
        if (diags)
        {
          IpcAllocateDiagsArea(*diags,diagsHeap);
          (**diags) << DgSqlCode(-2024) << DgString0(processName_)
            << DgInt0(rc);
        }
        return;
      }
    }

    IpcProcessId serverProcId((const GuaProcessHandle &)procHandle);
	      
    controlConnection_ = new(getServerClass()->getEnv()->getHeap())
      GuaConnectionToServer(getServerClass()->getEnv(),
			  serverProcId,
			  usesTransactions_,
			  nowaitDepth_);
    if (controlConnection_->getState() == IpcConnection::ERROR_STATE)
    {
      i++;
      guaError = controlConnection_->
          castToGuaConnectionToServer()->getGuardianError();
      delete controlConnection_;
      controlConnection_ = NULL;
      DELAY(10);
    }
    else
      break;
  }

  if (controlConnection_ != NULL)
    // Theprocess is ready for use
    serverState_ = READY;
  else
  {
    serverState_ = ERROR_STATE;
    guardianError_ = guaError;
    if (diags)
    {
      IpcAllocateDiagsArea(*diags,diagsHeap);
      (**diags) << DgSqlCode(-2024) << DgString0(processName_)
        << DgInt0(guardianError_);
    }
  }
}


short IpcGuardianServer::changePriority(IpcPriority priority, NABoolean isDelta)
{
  return 0;
}
					
NABoolean IpcGuardianServer::serverDied()
{
  const GuaProcessHandle &ph = getServerId().getPhandle();
  char pname[PhandleStringLen];
  Int32 pnameLen = ph.toAscii(pname, PhandleStringLen);
  pname[pnameLen] = '\0';
  int nid, pid;
  SB_Verif_Type verifier;
  int rc = msg_mon_get_process_info2(pname, &nid, &pid, &verifier);
  return rc != 0 ;
}

void IpcGuardianServer::populateDiagsAreaFromTPCError(ComDiagsArea *&diags,
                                                      CollHeap *diagsHeap)
{
  IpcAllocateDiagsArea(diags,diagsHeap);

  switch (procCreateError_)
    {
    // common launch errors
    case XZFIL_ERR_NOSUCHDEV:  //  14
    case XZFIL_ERR_NOBUFSPACE: //  22
    case XZFIL_ERR_FSERR:      //  53
    case XZFIL_ERR_BADREPLY:   //  74
    case XZFIL_ERR_OVERRUN:    // 121
    case XZFIL_ERR_DEVERR:     // 190
      // common error on launch--AQR
      (*diags) << DgSqlCode(-2012) << DgInt0(guardianError_)
	<< DgInt1(procCreateError_) <<DgInt2(procCreateDetail_)
        << DgNskCode(guardianError_);
      break;

    default:
      (*diags) << DgSqlCode(-2013) << DgInt0(procCreateError_)
               << DgNskCode(procCreateError_);
      break;
    }

  char location[TC_PROCESSOR_NAME_MAX];
  getCpuLocationString(location);
  (*diags) << DgString1(location);

  // the $string0 parameter always identifies the program file name
  (*diags) <<  DgString0(progFileName_);

  const char * interpretiveText = NULL; // for 2012 errors, we add interpretive text

  switch (procCreateError_)
    {
    case XZFIL_ERR_NOSUCHDEV:  //  14
    case XZFIL_ERR_FSERR:      //  53
    case XZFIL_ERR_DEVERR:     // 190
      interpretiveText = "Could not access executable file.";
      break;

    case XZFIL_ERR_NOBUFSPACE: //  22
      interpretiveText = "Insufficient buffer space.";
      break;

    case XZFIL_ERR_BADREPLY:   //  74
      interpretiveText = "Incorrect reply received from monitor.";
      break;

    case XZFIL_ERR_OVERRUN:    // 121
      interpretiveText = "A message overrun occurred while communicating with the monitor.";
      break;
   
    default:
      interpretiveText = NULL;
      break;
    }
  
  if (interpretiveText)
    (*diags) << DgString2(interpretiveText);
}

void IpcGuardianServer::getCpuLocationString(char *location)
{
  if (!location)
    return;

  // populate the nodeName_ if it has not already been captured
  if ((nodeName_ == NULL) && (actualCpuNum_ != IPC_CPU_DONT_CARE))
    {
      // populate nodeName_ from the Trafodion node number that we actually attempted to use 
      MS_Mon_Node_Info_Type nodeInfo;
      Int32 rc = msg_mon_get_node_info_detail(actualCpuNum_, &nodeInfo);
      if (rc == 0)
        {
          nodeName_ =  new (getServerClass()->getEnv()->getHeap()) char[TC_PROCESSOR_NAME_MAX];
          strcpy(nodeName_, nodeInfo.node[0].node_name);
        }
    }

  if (nodeName_)
    {
      strcpy(location,nodeName_);
    }
  else
    {
      strcpy(location,"an unspecified node");
    }
}

#if defined(LOG_IPC) || defined(LOG_RECEIVE)


void IpcGuaLogTimestamp(IpcConnection *conn)
{
  Int64 jts = JULIANTIMESTAMP();
  MyGuaProcessHandle me;
  char meAsAscii[200];
  char otherAsAscii[200];
  char *fromto;
  short fno = 999;


  me.toAscii(meAsAscii,200);
  if (conn)
    conn->getOtherEnd().toAscii(otherAsAscii,200);
  else
  {
    otherAsAscii[0] = '?';
    otherAsAscii[1] = '?';
    otherAsAscii[2] = '\0';
  }

NABoolean useGuaIpc = TRUE;
  if (!useGuaIpc AND conn AND conn->castToGuaConnectionToServer())
    {
      fromto = " to ";
      fno = conn->castToGuaConnectionToServer()->
        getFileNumForLogging();
    }
  else if (conn AND conn->castToGuaConnectionToClient())
    {
      fromto = " from ";
      fno = conn->castToGuaConnectionToClient()->
        getFileNumForLogging();
    }
  else
    fromto = " <-> ";

  cerr << "(" << 
    // NT has problems printing out an Int64
    (ULng32) jts
       << "): " << meAsAscii << fromto << otherAsAscii
       << "(" << fno << ") ";
}
#endif /* LOG_IPC || LOG_RECEIVE */



