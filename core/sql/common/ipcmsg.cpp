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
 * File:         IpcMsg.cpp
 * Description:  Implementation for the IPC classes using the NSK messaging
 *               API.
 *
 * Created:      6/25/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define AEVENT 1

#define set_extern_data

#include "Platform.h"

#include "rosgen.h"
#include "fs_rosetta_dml.h"
#include "dpxnsdp2"
#include "yfsiopen"
//#include "dfsiopn.h"
enum {FS_SMS_VERSION_MAY94 = 1};
#include "wdialect"
#include "ppctlc(WAIT, SETSTOP)"
#include "dmsghi.h"
#include "psignalc.h(PK_SIG_SYSTEMCALL_ABORTINQUIRE_, PK_SUSPEND_DISALLOW_SET_)"
#include "pmallocc(ADDRESS_WIRE_, ADDRESS_UNWIRE_)"
#include "hpfs2f(fs2_transid_to_buffer)"
#include "ffilcpp(FS_SQL_SETUPREQUESTINFO, FS_SQL_PUTMSGIDINACB, \
                  FS_SQL_RESETAFTERREPLY)"
#include "Int64.h"

#define _resident
#define _priv
#include "ExCollections.h"
#include "Ipc.h"
#include "str.h"
#include "ComDiags.h"
#include "NAExit.h"
#include "ipcmsg.h"
#include <fcntl.h>
#include "logmxevent.h"

#if (defined(NA_GUARDIAN_IPC))
// all of these files are OK in the executor environment (PRIV, no globals)
extern "C" {
//#include <cextdecs.h>
#include "cextdecs.h(PROCESSHANDLE_TO_FILENAME_,PROCESSHANDLE_DECOMPOSE_,FILE_OPEN_,SETMODE,FILE_GETINFO_,FILE_CLOSE_, AWAITIOX,PROCESS_DELAY_)"
#include <tal.h>
// should be #include <zsysc.h>
#include "zsysc.h"
}
#endif

//Function used to get a pointer to the pfs
_callable void fs2_get_pfsaddr(Long *);

//Function used to get transid from the filesystem
extern "C" _priv _resident int_16 FS_GETTRANSID_
(
 extaddr           tubaddr,      // input
                                 // the address of the Trans Usage Block
                                 // (having the tcbref as the 1st
                                 // field), or 0D or ptmfnocurtransid
 phandle_template *destination,  // input
                                 // where it will be sent
 int_16           *buffer);      // output
                                 // the tcbref is placed here

extern "C" _priv int_16 TMFLIBFS_ABORTTRANS_
(
  int_16 *tcbref_ptr, 
  short disposition 
);


// -----------------------------------------------------------------------
// Methods for class GuaMsgConnectionToServer
// -----------------------------------------------------------------------

GuaMsgConnectionToServer::GuaMsgConnectionToServer(
     IpcEnvironment *env,
     const IpcProcessId &procId,
     NABoolean usesTransactions,
     unsigned short nowaitDepth,
     const char *eye) : IpcConnection(env,procId,eye)
{
  openFile_                 = InvalidGuaFileNumber;
  nowaitDepth_              = nowaitDepth;
  maxIOSize_                = env->getGuaMaxMsgIOSize();

  activeIOs_                = new(env) ActiveIOQueueEntry[nowaitDepth_];
  //get the length of the request/reply control structure
//  int controllen = sizeof(fs_fs_template) + sizeof(fs_fs_template::__writeread);
  Int32 controllen = sizeof(fs_fs_writeread);

  //initialize each entry in the Active IOs queue
  for (unsigned short i = 0; i < nowaitDepth_; i++)
    {
      activeIOs_[i].writeDataCBAPtr_ = (void*) new(env) char[sizeof(NSK_CBA)];
      activeIOs_[i].readDataCBAPtr_ = (void*) new(env) char[sizeof(NSK_CBA)];
      activeIOs_[i].controlCBAPtr_ = (void*) new(env) char[sizeof(NSK_CBA)];  
      activeIOs_[i].controlBuf_ = (void*) new(env) char[controllen];
      activeIOs_[i].inUse_ = FALSE;
      activeIOs_[i].expectReply_ = FALSE;
      activeIOs_[i].msgid_ = 0;
      activeIOs_[i].transid_ = -1;
      activeIOs_[i].buffer_ = activeIOs_[i].readBuffer_ = NULL;
    }
#pragma nowarn(1506)   // warning elimination 
  lastAllocatedEntry_       = nowaitDepth_-1;
#pragma warn(1506)  // warning elimination 

  numOutstandingIOs_        = 0;
  partiallySentBuffer_      = NULL;
  chunkBytesSent_           = 0;
  partiallyReceivedBuffer_  = NULL;
  chunkBytesRequested_      = 0;
  chunkBytesReceived_       = 0;
  usesTransactions_         = usesTransactions;
  abortXnOnPathErrors_      = FALSE;
  guaErrorInfo_             = GuaOK;
  currentEntry_             = 0;

  // We need a nowait dept of at least 2, one for a message and another
  // one for out-of-band messages (not really implemented yet).
  //  assert(nowaitDepth_ >= 2);
  
  sendCallbackBufferList_ = new(env) IpcMessageBuffer*[nowaitDepth_];
  for (unsigned short i = 0; i < nowaitDepth_; i++)
    sendCallbackBufferList_[i] = NULL;

  // now open the server process
  openPhandle(NULL);
}

GuaMsgConnectionToServer::~GuaMsgConnectionToServer()
{
  closePhandle();

  CollHeap *heap = getEnvironment()->getHeap();
  for (Int32 i = 0; i < nowaitDepth_; i++)
    {
      ActiveIOQueueEntry &entry = activeIOs_[i];
      heap->deallocateMemory(entry.writeDataCBAPtr_);
      heap->deallocateMemory(entry.readDataCBAPtr_);
      heap->deallocateMemory(entry.controlCBAPtr_);
      heap->deallocateMemory(entry.controlBuf_);
    }
  heap->deallocateMemory(activeIOs_);
  heap->deallocateMemory(sendCallbackBufferList_);
}

void GuaMsgConnectionToServer::send(IpcMessageBuffer *buffer)
{
  // simply add the new buffer to the send queue and try to start
  // as many new I/O operations as possible
  queueSendMessage(buffer);
  while (tryToStartNewIO())
    ;
}

void GuaMsgConnectionToServer::receive(IpcMessageStreamBase *msg)
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

NABoolean GuaMsgConnectionToServer::moreWaitsAllowed()
{
  return !stopWait_;
}

//
// Wait for an I/O reply. After receives a reply, the I/O entry looks like:
//
//   - entry.buffer_=entry.readBuffer_=reply buffer
//
WaitReturnStatus GuaMsgConnectionToServer::wait(IpcTimeout timeout, UInt32 *eventConsumed = NULL, IpcAwaitiox *ipcAwaitiox)
{
  NABoolean ipcAwaitioxCompleted = ipcAwaitiox != NULL;
  if (ipcAwaitioxCompleted)
    ipcAwaitioxCompleted = ipcAwaitiox->getCompleted();
  assert(ipcAwaitioxCompleted == FALSE); 
        //Internal error: AWAITIOX should not have completed a message system based operation
  short error = 0;
  short waitField = LDONE | LSIG;
  short status = 0;
  direct_globals_template * pfsptr;
#pragma nowarn(252)   // warning elimination 
  fs2_get_pfsaddr((Long*)&pfsptr);
#pragma warn(252)  // warning elimination 

  if ((ULng32)(openFile_) >= (ULng32)(pfsptr->numftentries))
  {
     // If this connection is not open, there can be nothing to wait on.
     // Return an error in this case to indicate that this is an inappropriate
     // call. Do no wait on this connection again.
     guaErrorInfo_ = FENOTOPEN;
     setErrorInfo(-1);
     handleIOError();
     stopWait(TRUE);
     return WAIT_OK;
  }

  // don't do anything if the connection is in an error state and
  // there are no more pending requests to work on.
  if (getState() == ERROR_STATE AND numOutstandingIOs_ <= 0
      AND numQueuedSendMessages() <= 0)
  { // no more waits on this connection
    stopWait(TRUE);
    return WAIT_OK;
  }
  stopWait(FALSE);  
  setBreakReceived(FALSE);

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

  //used to mark the point in the activeIOs queue from
  //where we started checking for IO completion
  unsigned short start = currentEntry_;
  //indicates we found a completed message
  Int32 a_message_is_done = 0;
  
  //check if anyone of our messages has completed
  do {
    if ((activeIOs_[currentEntry_].inUse_) &&
        (activeIOs_[currentEntry_].expectReply_))
      {
        //check if the message for this entry is done
        if (MSG_ISDONE2_((NSK_msId2)activeIOs_[currentEntry_].msgid_))
          {
            getEnvironment()->setEvent(TRUE, AEVENT);
            a_message_is_done = 1;
            break;// found a complete message break
          }
      }

    currentEntry_++;

    /*
    ** Not that expensive since if-conversion reduces the control flow into
    ** predicated instructions 
    */
    if (currentEntry_ == nowaitDepth_)
      currentEntry_ = 0;
  } while(currentEntry_ != start );
 
  //
  // IMPORTANT: MUST NOT have any early return statement from here until
  // the end of this function. So that the loop at the end of the function
  // that issues receive callbacks will always be executed if any receive
  // entries were queued by this function.
  //

  NABoolean interrupt = FALSE;
 
  //if no message is done wait, then check again for message completion
  //for nowaited mode (timeout == 0), do not call WAIT.
  if ((!a_message_is_done && timeout != 0) || getEnvironment()->lsigConsumed())
    {
      // Only wait for LDONE if breakEnabled is not set.
      IpcTimeout waitTimeout = timeout == 0 ? -2 : timeout;
      if (getEnvironment()->lsigConsumed())
        status = LSIG;
      else
        {
          if (getEnvironment()->breakEnabled())
            status = WAIT(waitField, waitTimeout);//wait for timeout
          else
            status = WAIT(LDONE, waitTimeout);//wait for timeout
        }
      if (!status)
        {
          a_message_is_done = FALSE; // timed out
        }
      else
        {
          if (status & LSIG)
            {
              getEnvironment()->setLsigConsumed(FALSE);
              short oldsigmod = PK_SUSPEND_DISALLOW_SET_(1);
#pragma nowarn(1506)   // warning elimination 
              error = PK_SIG_SYSTEMCALL_ABORTINQUIRE_();
#pragma warn(1506)  // warning elimination 
              PK_SUSPEND_DISALLOW_SET_(oldsigmod);
              if (error > 0)
                {
                  // received a signal.
                  guaErrorInfo_ = FE_EINTR;
                  setBreakReceived(TRUE);
                  setErrorInfo(-1);
                  handleIOError();
                  interrupt = TRUE; // FE_EINTR
                }
            } // if (status & LSIG)
          else
            {
              //woken up because some message completed on the LDONE queue
              //somewhere. Check if the message completed was ours.
              getEnvironment()->setLdoneConsumed(TRUE);
              do {
                //check if this activeIOs_ entry is in Use
                if (activeIOs_[currentEntry_].inUse_ &&
                    activeIOs_[currentEntry_].expectReply_)
                  {
                    //check if the message for this entry is done
                    if (MSG_ISDONE2_((NSK_msId2)activeIOs_[currentEntry_].msgid_))
                      {
                        a_message_is_done = 1;
                        // The following line of code was added but is being
                        // removed because:
                        // a) LDONE consumed which servers the same purpose
                        //    has already been set, and
                        // b) a compiler performance regression occurred and
                        //    it's an unlikely but possible cause
                        //getEnvironment()->setEvent(TRUE, AEVENT);
                        break;
                      }
                  }

                currentEntry_++;
                if (currentEntry_ == nowaitDepth_) {
                  currentEntry_ = 0;
                }
              } while(currentEntry_ != start );
            }
        }
    }

  if (a_message_is_done)
    {
      ActiveIOQueueEntry &entry = activeIOs_[currentEntry_];

      NSK_msResult2  results;

      short oldstop = SETSTOP(2);//become unstoppable

      //Pickup reply and terminate message
      MSG_BREAK2_((NSK_msId2)entry.msgid_, &results,
                  (NSK_PHandle _ptr64 *)getOtherEnd().getPhandle().phandle_);

      //Get the error from the reply control buffer
      message_header_template * ReplyControlBuf = (message_header_template *)entry.controlBuf_;
      error = ReplyControlBuf->error();

      if (error == GuaTimeoutErr)
        {
          // timeout does not set the connection into an error state but it
          // causes a return. later we shall wait on this connection again.
          SETSTOP(oldstop);//become stoppable
          guaErrorInfo_ = error;
          return WAIT_OK;
        }

      //reset the filesystem data structures
      Int64 localTransid = entry.transid_;
      resetAfterReply(entry.msgid_, error, &localTransid);

      SETSTOP(oldstop);//become stoppable

      // we have got the reply for this I/O entry
      entry.expectReply_ = FALSE;

      if (error)
        {
          // Remember the Guardian error code
          guaErrorInfo_ = error;
          setErrorInfo(-1);
          handleIOErrorForEntry(entry);
        }
      else
        {
          cleanUpActiveIOEntry(entry);

          //get # of bytes written in reply
          ULng32 countRead = (ULng32)results.rr_dataSize;

          // Now try to figure out what the original operation was, so
          // we know what to do with the IpcMessageBuffer:
          //
          // a) If this I/O was a write operation for part of a buffer,
          //    then another operation for the same buffer is following, so
          //    just remove the outstanding I/O entry.
          // b) If this I/O returned part of a buffer, then we have to issue
          //    another I/O operation for the rest of the buffer.
          // c) If we have received all of the data, the buffer is ready to
          //    be delivered to its destination. If the buffer has its callback
          //    assigned, then call the callback, otherwise add the buffer to
          //    the receive queue that is managed by the base class,
          //    IpcConnection. "completelyReadBuffer" is set for case c)
          //
          if (entry.receiveBufferSizeLeft_ == 0)
            {
              // case a)
              assert(countRead == 0);
        
              // Note that this does NOT count as a completion, we don't
              // let the upper layers know that we are using multiple
              // Guardian I/Os for this.      
            }
          else
            {
              // we did expect data back, case b) or c)
              if (entry.offset_ == 0)
                {
                  if (numOutstandingIOs_ > 0)
                    {
                      // more pending I/Os on this connection. check if there
                      // are other chunks of entry.buffer_.
                      //
                      //  - in multi-chunk mode, when server receives any
                      // of the subsequent chunk, it sends an empty reply
                      // right away. however, when server receives the first
                      // chunk, it does not reply right away. instead, server
                      // holds the first chunk until it has received all
                      // subsequent chunks and then server replies to the
                      // first chunk. on the client side, even though the
                      // first chunk receives its reply the last, but since
                      // we are looking at a randomly selected I/O entry,
                      // "entry" could be any one of the multi-chunk I/Os.
                      //
                      for (unsigned short i = 0; i < nowaitDepth_; i++)
                        {
                          ActiveIOQueueEntry &nextEntry = activeIOs_[i];
                          if (nextEntry.inUse_ &&
                              nextEntry.buffer_ == entry.buffer_)
                            {
                              // entry is the first chunk and nextEntry
                              // is a subsequent chunk of a multi-chunk
                              // send buffer.
                              // only the first chunk receives reply.
                              // there is no reply for subsequent chunks.

                              // we must clean up I/O on nextEntry now
                              // or otherwise the shared send buffer
                              // entry.buffer_ can get deallocated
                              // (see a few lines below). after that
                              // we cannot clean up I/O on nextEntry as
                              // entry.buffer_->chunkLockCount_ is not
                              // longer accessible.
                              cleanUpActiveIOEntry(nextEntry);

                              if (numOutstandingIOs_ == 0)
                                // no more pending I/Os on this connection
                                break;
                            }
                        }
                    }

                  if (entry.buffer_->isShared())
                    {
                      // no longer need the shared send buffer. release it.
                      entry.buffer_->decrRefCount(getEnvironment());
                      // now use only the reply buffer 
                      entry.buffer_ = entry.readBuffer_;
                    }
          
                  // since this is the first (maybe only) chunk of the message
                  // buffer, we can get the length of the total message by
                  // looking into the message header.

                  // Get the size of the message sent (or the reply buffer if shared)
                  IpcMessageObjSize bytesSent = entry.buffer_->getMessageLength();

                  // unpack message header which contains reply message length
                  InternalMsgHdrInfoStruct *msgHdr = 
                    new( (IpcMessageObj*)(entry.buffer_->data(0)) )
                    InternalMsgHdrInfoStruct(NULL);
                  IpcMessageObjSize msgLen = msgHdr->getMsgLengthFromData();
          
                  // remember the real length of the message coming back
                  entry.buffer_->setMessageLength(msgLen);
          
                  // check whether this is case b) or c)
                  if (msgLen == (IpcMessageObjSize) countRead)
                    {
                      // The "normal" case c). This is a single-chunk reply.

                      // If we were sending a large buffer(more than one chunk)
                      // and just received a small buffer (one chunk) then
                      // release the large buffer to conserve space on the IPC
                      // heap.
                      if (bytesSent > maxIOSize_)
                        {
                          entry.buffer_ = entry.buffer_->resize(getEnvironment(), msgLen);
                        }

                      queueReceiveMessage(entry.buffer_);
                    }
                  else
                    {
                      // Case b). This is a multi-chunk reply. Switch to the
                      // chunk protocol, countRead bytes are already received
                      // back from the server.
                      if (msgLen > entry.buffer_->getBufferLength() ||
                          bytesSent > maxIOSize_)
                        {
                          // We want to resize the reply buffer if either:
                          // - The server has a reply message that is larger
                          //   than what our buffer can hold
                          // - The request buffer was large (more than one
                          //   chunk) and may now be consuming space
                          //   unnecessarily on the IPC heap
                          entry.buffer_->setMessageLength(countRead);
                          entry.buffer_ = entry.buffer_->resize(getEnvironment(), msgLen);
                          entry.buffer_->setMessageLength(msgLen);
                        }
            
                      // do some sanity checks, make sure we don't have two
                      // partial buffers at a time
                      assert(partiallyReceivedBuffer_ == NULL);
                      assert(msgLen > (IpcMessageObjSize) countRead);
            
                      // move some information from the entry to data members
                      // in the connection while the chunk protocol is going on
                      partiallyReceivedBuffer_ = entry.buffer_;
                      chunkBytesRequested_ = countRead;
                      chunkBytesReceived_ = countRead;
                      getEnvironment()->getAllConnections()->
                        setReceivedPartialMessage(TRUE);
                    }
                } // first (maybe only) chunk
              else
                {
                  // case b), this is not the first chunk
                  assert (partiallyReceivedBuffer_ == entry.buffer_);
                  chunkBytesReceived_ += countRead;
          
                  if (chunkBytesReceived_ == entry.buffer_->getMessageLength())
                    {
                      // this was the last chunk
                      queueReceiveMessage(partiallyReceivedBuffer_);
                      partiallyReceivedBuffer_ = NULL;
                      chunkBytesRequested_ = 0;
                      chunkBytesReceived_ = 0;
                    }
                  else
                    {
                      getEnvironment()->getAllConnections()->
                        setReceivedPartialMessage(TRUE);
                    }
                } // not the first chunk
            } // case b) or c)
      
          // this I/O completed
          if (getState() != ERROR_STATE && numOutstandingIOs_ == 0)
            setState(ESTABLISHED);

          // after waiting, try (again) to start as many new I/O operations as
          // possible
          while (tryToStartNewIO())
            ;
        } // if (error) else
    } // if (a_message_is_done)

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

  return interrupt ? WAIT_INTERRUPT : WAIT_OK ;
}

GuaMsgConnectionToServer * GuaMsgConnectionToServer::castToGuaMsgConnectionToServer()
{
  return this;
}

Int32 GuaMsgConnectionToServer::numQueuedSendMessages()
{
#pragma nowarn(1506)   // warning elimination 
  return sendQueueEntries();
#pragma warn(1506)  // warning elimination 
}

Int32 GuaMsgConnectionToServer::numQueuedReceiveMessages()
{
#pragma nowarn(1506)   // warning elimination 
  return receiveQueueEntries();
#pragma warn(1506)  // warning elimination 
}

void GuaMsgConnectionToServer::populateDiagsArea(ComDiagsArea *&diags,
                                                 CollHeap *diagsHeap)
{
  if (guaErrorInfo_ != GuaOK)
  {
    IpcAllocateDiagsArea(diags,diagsHeap);
    
    *diags << DgSqlCode(-2034) << DgInt0(guaErrorInfo_);
    getEnvironment()->getMyOwnProcessId(IPC_DOM_GUA_PHANDLE).
      addProcIdToDiagsArea(*diags,0);
    getOtherEnd().addProcIdToDiagsArea(*diags,1);
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
//                  entry.readBuffer_=reply buffer
//   - subsequent chunks: entry.buffer_=send buffer, entry.readBuffer_=NULL
//
// 2. multi-chunk send buffer, single connection (not shared):
//
//   - first chunk: entry.buffer_=entry.readBuffer_=send buffer
//   - subsequent chunks: entry.buffer_=send buffer, entry.readBuffer_=NULL
//
// 3. single-chunk send buffer, shared by multiple connections
//
//   - entry.buffer_=send buffer
//   - entry.readBuffer_=reply buffer
//
// 4. single-chunk send buffer, single connection (not shared):
//
//   - entry.buffer_=entry.readBuffer_=send buffer
//


NABoolean GuaMsgConnectionToServer::tryToStartNewIO()
{
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
    return FALSE;
  
  // Check if the per-process limit on MQCs is exceeded.
  // Note that this should be reconsidered when this code is 
  // multithreaded.
  short numMsgsActual;
  if (MESSAGESYSTEMINFO(5, &numMsgsActual))
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
  //assert(!entry.inUse_ && !entry.expectReply_);

  // ---------------------------------------------------------------------
  // set up a new outstanding IO entry, depending on what to do next
  // but don't start the corresponding I/O quite yet
  // ---------------------------------------------------------------------

  // initialize all fields of the entry (there is no constructor)
  entry.bytesSent_             = 0;
  entry.receiveBufferSizeLeft_ = 0;
  entry.offset_                = 0;
  entry.msgid_                 = 0;

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
  //    steps a) and b), so the server won't get confused
  // c) get another message from the send queue and find out that it
  //    is too long for a single chunk, so send the first piece
  // d) should be the normal case, get the next message from the send
  //    queue and send it in a single chunk
  // ---------------------------------------------------------------------

  if (partiallySentBuffer_)
    {
      // case a) continue sending more chunks for this buffer
      // but don't ask for reply data, since we may want the reply to come
      // back at entry.buffer_->data(0) 
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

      if (entry.buffer_->getRefCount() > 1 || entry.buffer_->isShared())
        {
          if (!entry.buffer_->initLockCount(getEnvironment()->getHeap(),
                                            maxIOSize_))
            {
              // We ran out of space on the IPC heap. This is OK and we
              // can return early. Higher layers will redrive the I/O for
              // this connection.
              getEnvironment()->setHeapFullFlag(TRUE);
              lastAllocatedEntry_ = originalLastAllocated;
              return FALSE;
            }

          // The send buffer is shared by multiple connections. Therefore,
          // allocate a different buffer for the reply. the reply buffer size
          // is set as:
          //
          //   - if user has explicitly specified the reply buffer length
          //     and it is less than maxIOSize_, then use it.
          //   - otherwise, use maxIOSize_.
          IpcMessageStream *msgStream = entry.buffer_->getMessageStream()->castToIpcMessageStream();
          //assert(msgStream);
          IpcMessageObjSize maxReplyLen = msgStream->getMaxReplyLength();
          if (maxReplyLen > 0 && maxReplyLen < maxIOSize_)
            entry.readBuffer_ = entry.buffer_->createBuffer(getEnvironment(), maxReplyLen, FALSE);
          else
            entry.readBuffer_ = entry.buffer_->createBuffer(getEnvironment(), maxIOSize_, FALSE);
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

      prepareSendBuffer(entry.buffer_);

      // got this far so de-queue buffer from this connection's queue
      removeNextSendBuffer();
      entry.receiveBufferSizeLeft_ =
        MINOF(maxIOSize_, entry.readBuffer_->getBufferLength());

      lastSentBuffer_ = entry.buffer_;
    }

  // ---------------------------------------------------------------------
  // Next, start the I/O operation
  // ---------------------------------------------------------------------
  short wireOptions = 8;
  short GuardianError = 0;
 
  direct_globals_template * pfsptr;
  acb_standard_template * acb;

  // Get the pointer to the acb - needed in order to decrement the count
  // of outstanding requests in the ACB on errors.
#pragma nowarn(252)   // warning elimination 
  fs2_get_pfsaddr((Long*)&pfsptr);
#pragma warn(252)  // warning elimination 

  // check that the file is actually open, i.e., that this is a valid 
  // filenum.
  if ((ULng32)(openFile_) >= (ULng32)(pfsptr->numftentries))
  {
     GuardianError = FENOTOPEN;
  }
  else
     acb = (acb_standard_template *) pfsptr->file_table[openFile_];

  //become unstoppable
  short oldstop = SETSTOP(2);

  if (NOT GuardianError)
    {
      // lock memory used for the control info buffer
#pragma nowarn(1506)   // warning elimination 
      GuardianError = ADDRESS_WIRE_((unsigned char *)entry.controlBuf_,
                                    sizeof(fs_fs_writeread), wireOptions);
#pragma warn(1506)  // warning elimination
    }

  if (NOT GuardianError)
    {
      entry.transid_ = (Int64)entry.buffer_->getTransid();
      // setup request control buffer 
      GuardianError = setupRequestInfo((void*)entry.controlBuf_,
				       (Int64)entry.buffer_->getTransid());
      if (GuardianError)
        ADDRESS_UNWIRE_((unsigned char *)entry.controlBuf_,
                        (sizeof(fs_fs_writeread)), wireOptions);
    }

  if (NOT GuardianError)
    {
      GuardianError = addressWire(entry, wireOptions);
      if (GuardianError)
        {
          // decrement the number of outstanding requests -
          // this was incremented by the call to setupRequestInfo() above.
          acb->acb_numreqs = acb->acb_numreqs - 1;
          ADDRESS_UNWIRE_((unsigned char *)entry.controlBuf_,
                          (sizeof(fs_fs_writeread)), wireOptions);
        }
    }

  if (NOT GuardianError)
    {
      // generate Context-Based Addresses (CBAs) for the data buffer
      // and for the control buffer
      NSK_CBA  &writeDataCBA = *(NSK_CBA*) entry.writeDataCBAPtr_;
      NSK_CBA  &readDataCBA = *(NSK_CBA*) entry.readDataCBAPtr_;
      NSK_CBA  &controlCBA = *(NSK_CBA*) entry.controlCBAPtr_;

      // for all other types of send use the send buffer
      CBA_CREATE_((NSK_CBA *)entry.writeDataCBAPtr_, entry.buffer_->data(entry.offset_));

      if (!entry.readBuffer_)
        // this is a subsequent chunk of a multi-chunk send. no reply is
        // needed.
        CBA_CREATE_((NSK_CBA *)entry.readDataCBAPtr_, 0);
      else
        // all other types of reply
        CBA_CREATE_((NSK_CBA *)entry.readDataCBAPtr_, entry.readBuffer_->data(entry.offset_));

      CBA_CREATE_((NSK_CBA *)entry.controlCBAPtr_,entry.controlBuf_);

      NSK_msLinkOpts2 options = MSG_LINK_CBA;

      short retryCount = 0;
      NABoolean needToRetry;
      do {
          // send the message to the server, using CBAs, so that hide/reveal
          // operations later can't invalidate the addresses
          NSK_msId2 localMsgId = 0;

          GuardianError = MSG_LINK2_(
	       (NSK_PHandle _ptr64 *)getOtherEnd().getPhandle().phandle_,
	       (NSK_msId2 _ptr64 *)&localMsgId,
	       (int16 _ptr64 *)&controlCBA,
	       (NSK_msSize2)sizeof(fs_fs_writeread),
	       (int16 _ptr64 *)&controlCBA,
	       (NSK_msSize2)sizeof(fs_fs_writeread),
	       (char _ptr64 *)&writeDataCBA, //message to send to server
	       (NSK_msSize2)entry.bytesSent_,  //# of bytes to send 
	       (char _ptr64 *)&readDataCBA, //buffer to receive reply from server
	       (NSK_msSize2)entry.receiveBufferSizeLeft_, //reply bytes expected
               0, 0, 0, options);

          entry.msgid_ = (UInt32)localMsgId;

          if (GuardianError == FENOLCB)  // too many MQCs.
            {
              // Since the per-process limit was checked above,
              // assume that it is the per-cpu limit, so let us
              // retry.
              retryCount++;
              needToRetry = TRUE;
              getEnvironment()->incrRetriedMessages();
              PROCESS_DELAY_(10*1000);  // 10,000 microseconds
            }
          else
            needToRetry = FALSE;
      } while (needToRetry && 
               retryCount < 100*60 );  // after 60 seconds (and 6000 retries),
                                       // just give up.
      getEnvironment()->setEvent(TRUE, AEVENT);

      if (GuardianError)
        {
          // failed message link

          // decrement the number of outstanding requests -
          // this was incremented by the call to setupRequestInfo() above.
          acb->acb_numreqs = acb->acb_numreqs - 1;
          addressUnwire(entry);
        } // if (GuardianError)
      else
        {
          // Put the message id into the acb.
          putMsgIdinACB(entry.msgid_);
        }
    }

  //go back to old stop mode
  SETSTOP(oldstop);

  if (isFirstChunk)
    addSendCallbackBuffer(entry.buffer_);

  if (GuardianError)
    {
      // an error happened somewhere along the way and we must
      // a) record the Guardian error number,
      guaErrorInfo_ = GuardianError;
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
      entry.inUse_ = TRUE;  // this entry now has an I/O in progress
      entry.expectReply_ = TRUE;

      // --------------------------------------------------------------
      // If we started the I/O for all chunks of an IpcMessageBuffer
      // (or if the IpcMessageBuffer was sent in a single message), then 
      // call its send callback.
      // --------------------------------------------------------------
      if (isLastChunk)
        {
          // removeSendCallbackBuffer() should always return TRUE here because
          // send callback has not been invoked yet. If error occured prior to
          // this method call then handleIOErrorForEntry() should have cleared
          // all I/Os on the same message stream and we would not have come
          // here.
          NABoolean sendCallbackFlag = removeSendCallbackBuffer(entry.buffer_);
          if (sendCallbackFlag)
            // the send callback doesn't give away entry.buffer_,  and this is
            // good since the same buffer may be still used for the receive
            // operation.
            entry.buffer_ ->callSendCallback(this);
          else // - for debugging only
            assert(sendCallbackFlag);
        }
    }

  return TRUE;
}

void GuaMsgConnectionToServer::openPhandle(char * processName)
{
  char  procFileName[IpcMaxGuardianPathNameLength];
  short procFileNameLen;
  short openFlags = nowaitDepth_ == 0 ? 0x0 : 0x4000;

  if (! processName)
    {
      // convert the phandle to a string that can be passed to FILE_OPEN_
      guaErrorInfo_ = PROCESSHANDLE_TO_FILENAME_(
	   (short *) getOtherEnd().getPhandle().phandle_,
	   procFileName,
	   IpcMaxGuardianPathNameLength,
	   &procFileNameLen,
	   0);
      if (guaErrorInfo_ != GuaOK)
	{
	  setErrorInfo(-1);
	  setState(ERROR_STATE);
	  return;
	}
    }
  else
    {
      strcpy(procFileName, processName);
      procFileNameLen = (short)strlen(processName);
    }

  getEnvironment()->setLdoneConsumed(TRUE);

  // solution 10-081025-6810:
  // wait for esp open reply indefinitely. time out every 10 minutes, write
  // a warning msg to ems log including error code and esp pin, and then go
  // back to waiting. we allow up to 10 timeouts, or 100 minutes.
  //
  // - when AWAITIOX times out, it completes/cancels the nowait i/o
  // initiated by FILE_OPEN_. thus we cannot simply call AWAITIOX again since
  // there is no more outstanding i/o. we must try FILE_OPEN_ again and then
  // call AWAITIOX to wait another 10 minutes. also, we must save the file
  // numbers returns from each timed out FILE_OPEN_ so we can close them later.
  // and we cannot call FILE_CLOSE_ on any of the returned file numbers until
  // AWAITIOX returns successfully with esp's open reply. otherwise the close
  // msg from master will cause esp to exit.
  //
  GuaFileNumber oldOpens[10]; // allow up to 10 timeouts
  short numOldOpens = 0;
  _cc_status stat;
  while (1)
    {
#pragma nowarn(1506)   // warning elimination 
      guaErrorInfo_ = FILE_OPEN_(procFileName,
                                 procFileNameLen,
                                 &openFile_,
                                 0, // open for read/write access
                                 0, // shared access
                                 nowaitDepth_,
                                 0, // sync depth 0 (target proc is not NonStop)
                                 openFlags); // options
#pragma warn(1506)  // warning elimination 
      if (guaErrorInfo_ != GuaOK)
        break;

      if (!(openFlags &= 0x4000))
        // return if FILE_OPEN_ was waited
        break;

      // FILE_OPEN_ was no-waited

      // wait indefinitely
      stat = AWAITIOX(&openFile_);
      if (_status_eq(stat))
        // reply received with no error
        break;

      // we got error. get Guardian error code.
      GuaErrorNumber getinfoError = FILE_GETINFO_(openFile_,&guaErrorInfo_);
      if (getinfoError != 0)
        guaErrorInfo_ = getinfoError; // not even FILE_GETINFO_ worked

      // set guaErrorInfo_ to -1 if we did not get a valid error?
      //if (guaErrorInfo_ == GuaOK)
      //  guaErrorInfo_ = -1;


      // AWAITIOX returned error, or ran out allowed timeouts.
      break;
    } // while (1)

  for (short i = 0; i < numOldOpens; i++)
    FILE_CLOSE_(oldOpens[i]);

  if (guaErrorInfo_ != GuaOK)
    {
      if (openFile_ != -1)
        {
          FILE_CLOSE_(openFile_); // Don't retain unopened ACB
          openFile_ = -1; // Don't leave valid file number in object!
        }
      setErrorInfo(-1);
      setState(ERROR_STATE);
      return;
    }

  // some day we may want to perform nowaited FILE_OPEN_ calls and add
  // a method to "work" on the open.

  // use setmode 74 to turn off the automatic CANCEL upon AWAITIOX timeout
  stat = SETMODE(openFile_,74,-1);
  if (_status_ne(stat))
    {
      // get a Guardian error code
      Int32 errcode2 = FILE_GETINFO_(openFile_,&guaErrorInfo_);

      if (errcode2 != 0)
#pragma nowarn(1506)   // warning elimination 
	guaErrorInfo_ = errcode2; // not even FILE_GETINFO_ worked
#pragma warn(1506)  // warning elimination 
      setErrorInfo(-1);
      setState(ERROR_STATE);
      return;
    }

  // use setmode 30 to allow I/O operations to finish in any order
  stat = SETMODE(openFile_,30,3);
  if (_status_ne(stat))
    {
      // get a Guardian error code
      Int32 errcode2 = FILE_GETINFO_(openFile_,&guaErrorInfo_);

      if (errcode2 != 0)
#pragma nowarn(1506)   // warning elimination 
	guaErrorInfo_ = errcode2; // not even FILE_GETINFO_ worked
#pragma warn(1506)  // warning elimination 
      setErrorInfo(-1);
      setState(ERROR_STATE);
      return;
    }

  // use setmode 117 if no transactions should be propagated to the server
  if (NOT usesTransactions_)
    {
      _cc_status stat = SETMODE(openFile_,117,1);
      if (_status_ne(stat))
	{
	  // get a Guardian error code
	  Int32 errcode2 = FILE_GETINFO_(openFile_,&guaErrorInfo_);

	  if (errcode2 != 0)
#pragma nowarn(1506)   // warning elimination 
	    guaErrorInfo_ = errcode2; // not even FILE_GETINFO_ worked
#pragma warn(1506)  // warning elimination 
	  setErrorInfo(-1);
	  setState(ERROR_STATE);
	  return;
	}
    }

  // the connection is established now
  setState(ESTABLISHED);
}

void GuaMsgConnectionToServer::closePhandle()
{
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
  // GuaMsgConnectionToServer::setFatalError() invokes handleIOErrorForStream()
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
      FILE_CLOSE_(openFile_);
      openFile_ = InvalidGuaFileNumber;
    }
}

//This function sets up the control information required by
//the file system on the server side
//control is a pointer to the buffer that is supposed to
//contain the control information
short GuaMsgConnectionToServer::setupRequestInfo(void * control, Int64 transid){
  //Redirected to T9055 to insulate SQL/MX from changes in 
  //ACB_REQUEST_TEMPLATE 
  Int32 retcode = FS_SQL_SETUPREQUESTINFO(openFile_,
                                        (fs_fs_template *) control,
                                        transid);
  return retcode;

#if 0
  fs_fs_writeread* controlInfo =(fs_fs_writeread*) control;
  direct_globals_template * pfsptr;//pointer to the PFS
  acb_standard_template * acb;//pointer to the acb for this file
  short error = FEOK; //variable to catch errors
  
  //first get pointer to the PFS
#pragma nowarn(252)   // warning elimination 
  fs2_get_pfsaddr((Long*)&pfsptr);
#pragma warn(252)  // warning elimination 
 
  //then get pointer to the acb in the 
  //PFS Filetable at the index openFile_
  acb = (acb_standard_template *) pfsptr->file_table[openFile_];
 
  //set the dialect type to indicate this is a call from the file system
  //since we are trying to emulate the WRITEREADX call
  controlInfo->dialect_type = DIALECT_FS_FS;
  controlInfo->request_type = FS_FS_WRITEREAD;
  controlInfo->request_version = CURRENT_VERSION_FS_FS;
  controlInfo->minimum_interpretation_version = MINIMUM_VERSION_FS_FS;
  
  controlInfo->tcbref_valid = 0;
  controlInfo->lid_valid = 0;
  controlInfo->filler = 0;
  memset((void *)&(controlInfo->sendflags), 0, 
         sizeof(linkmon_sendflags_template));
 
  //get the number of outstanding requests
  int_16 next = acb->acb_numreqs;
 
  void ** acb_reqaddrs = (void **)((char *)&(acb->req.acb_requestbase_addr) + 
                                   sizeof(acb->req.acb_requestbase_addr) );
 
  //get a pointer to the data for this request
  acb_request_template * acb_reqptr =
      (acb_request_template *)acb_reqaddrs[next];
  
  //check if numreqs is less than the maximum number of reqs allowed
  if (( acb->acb_numreqs) < (int_16) acb->acb_maxreqs)
  {
    //update the numreqs
    acb->acb_numreqs = next + 1;
    int_16 savenum = acb_reqptr->acb_reqnum;
    //initialize the acb req to all zeros
    _fill_32(acb_reqptr, (_len(acb_request_template)/ 4), 0 );
    acb_reqptr->acb_reqnum = savenum;
  } // if req avail
  else
  {
    // we used to return FETOOMANY here, but the check in
    // tryToStartNewIO of numOutStandingIOs and nowaitDepth_ 
    // should have prevented the FETOOMANY.
     assert(0);
  }
  
  // if transid argument is invalid set flag in controlInfo.
  if (transid == (Int64)-1) 
    {
      controlInfo->tcbref_valid = 0;
    }
  else
    {
      // Here, acb_reqptr->acb_tubaddr is not initialized with the 
      // context transid (as done in FS), to prevent ENDTRANSACTION 
      // from returning error 81 if there are outstanding ipc msgs 
      // (for read only query).
      // For nowaited queries involves insert, update, delete, 
      // or nowaited prepares, CliGlobals::checkOperationsPending 
      // detects outstanding operations and returns error to
      // FS2^FLUSH^ALL^VSBB.

      // transid passed in is valid. Call FS2 to move transid into
      // control buffer (may need to "massage" transid if the destination
      // is a remote node.
     
      error = Fs2_transid_to_buffer
                  (acb->posix.acb_procsection->tmfvirtualnode, 
                   (unsigned char *)&transid, 
                   (unsigned char *)&(controlInfo->tcbref),
                   0
                  );
      if (error)
	{
	  acb->acb_numreqs = acb->acb_numreqs - 1;
	  return error;  
	}
      controlInfo->tcbref_valid = 1;
    }
  
  //hard coded since file open passes a zero
  controlInfo->sender.syncid = 1;
  
  //get the filenum for this connection
  controlInfo->sender.first_word.filenum = openFile_;
  
  //copy the phandle
  memcpy(&controlInfo->sender.phandle,&pfsptr->my_phandle,(sizeof(short)*10));
  controlInfo->sender.user_openid = acb->posix.acb_procsection->acb_useropenid;
  controlInfo->sender.id.openid = acb->posix.acb_procsection->sender.acb_procopenid;
  return FEOK;
#endif
}

//This function is used to put the msgid into the acb after a MSG_LINK_.
//This let's the filesystem cleanup outstanding IOs on PROCESS_STOP_
void GuaMsgConnectionToServer::putMsgIdinACB(UInt32 msgid){
  //Redirected to T9055 to insulate SQL/MX from changes in 
  //ACB_REQUEST_TEMPLATE 
  Int32 retcode = FS_SQL_PUTMSGIDINACB(openFile_, msgid);
#if 0
  direct_globals_template * pfsptr;//pointer to the PFS
  acb_standard_template * acb;//pointer to the acb for openFile_
  
  //first get pointer to the PFS
#pragma nowarn(252)   // warning elimination 
  fs2_get_pfsaddr((Long*)&pfsptr);
#pragma warn(252)  // warning elimination 
  
  //then get pointer to the acb in the PFS 
  //Filetable at the index openFile_
  acb = (acb_standard_template *) pfsptr->file_table[openFile_];
  
  //get the index into the array of acb requests
#pragma nowarn(1506)   // warning elimination 
  int_16 next = acb->acb_numreqs - 1;
#pragma warn(1506)  // warning elimination 
  
  void ** acb_reqaddrs = (void **)((char *)&(acb->req.acb_requestbase_addr) + 
                                   sizeof(acb->req.acb_requestbase_addr) );

  //get a pointer to the acb data for this request
  acb_request_template * acb_reqptr =
      (acb_request_template *)acb_reqaddrs[next];
  
  //put the msgid into the acb
  acb_reqptr->mid.acb_mid = msgid;
  if (acb_reqptr->tub.acb_tubaddr != 0)
     acb_reqptr->tub.acb_tub->pending_count = 
#pragma nowarn(1506)   // warning elimination 
            (ULng32)(acb_reqptr->tub.acb_tub->pending_count) + 1u;
#pragma warn(1506)  // warning elimination
#endif 
}

void  GuaMsgConnectionToServer::resetAfterReply(UInt32 msgid, short error,
                                                Int64 *transid){
  //Redirected to T9055 to insulate SQL/MX from changes in 
  //ACB_REQUEST_TEMPLATE 
  Int32 retcode = FS_SQL_RESETAFTERREPLY(openFile_, msgid, error, transid, 
                                       abortXnOnPathErrors_);

#if 0
  direct_globals_template * pfsptr;//pointer to the PFS
  acb_standard_template * acb;//pointer to the acb for openFile_
  acb_request_template * acb_reqptr;
  short reqIndex = -1; 
  short i;

 //first get pointer to the PFS
#pragma nowarn(252)   // warning elimination 
  fs2_get_pfsaddr((Long*)&pfsptr);
#pragma warn(252)  // warning elimination 
  
  //then get pointer to the acb in the 
  //PFS Filetable at the index openFile_
  acb = (acb_standard_template *) pfsptr->file_table[openFile_];
  
  void ** acb_reqaddrs = (void **)((char *)&(acb->req.acb_requestbase_addr) + 
                                   sizeof(acb->req.acb_requestbase_addr) );
  
  // get a pointer to the acb data for this request (search through array
  // of outstanding requests,, to get the request id that matches this one,
  // that was just completed). This is a fix for a bug - earlier, it was
  // just assumed that the request completed was the last request in the
  // array - this may not be the case, as requests may be completed out
  // of order. This bug lead to %4100 halts (BR #85). The fix to search
  // the array for the completed request, along with the fix to "shrink" the
  // array (see code below) solves this bug by doing what the Enscribe
  // Filesystem does in AWAITIOX.
  for (i=0; i <= acb->acb_numreqs; i++)
  {   
     acb_reqptr = (acb_request_template *)acb_reqaddrs[i];
     if (acb_reqptr->mid.acb_mid == msgid)
     {
        reqIndex = i;
        break;
     }
  }
  
  //set msgid to 0
  acb_reqptr->mid.acb_mid = 0;
  acb_reqptr->acb_reqrdy = 1;
  
  if (acb_reqptr->tub.acb_tubaddr != 0)
     acb_reqptr->tub.acb_tub->pending_count = 
#pragma nowarn(1506)   // warning elimination 
            (ULng32)(acb_reqptr->tub.acb_tub->pending_count) - 1u;
#pragma warn(1506)  // warning elimination 
 
  //decrement the numreqs
  acb->acb_numreqs = acb->acb_numreqs - 1;

  // Move all requests after this one to be before this one. This "shrinking"
  // of the request array is done to make it efficient to find a slot while
  // sending a new request (free slots are always at the end of the array)
  if ((acb->acb_numreqs > 0) && (reqIndex != acb->acb_numreqs))
  {
     i = reqIndex;
     while (i != acb->acb_numreqs)
     {
        acb_reqaddrs[i] = acb_reqaddrs[i + 1];
#pragma nowarn(1506)   // warning elimination 
        i = i + 1;
#pragma warn(1506)  // warning elimination 
     }
     acb_reqaddrs[acb->acb_numreqs] = (void *)acb_reqptr;
  }

  if (error == FECPUFAIL ||
      error == FEPATHDOWN ||
      (error >= FENETERR && error <= 255)
     )
  {
    // abort the transaction if a path error is received.
    if ((transid != NULL) && (*transid != -1))
    {
       // Want to stop the transaction on path errors. This is 
       // to fix the bug reported in solution 10-030508-6267.
       if (abortXnOnPathErrors_)
       {
          TMFLIBFS_ABORTTRANS_( (int_16 *)transid,
                                FETRANSABRTOWNERDIED );
       }
    }
  }
#endif
}

void GuaMsgConnectionToServer::setFatalError(IpcMessageStreamBase *msgStream)
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

  // This next call only works if GuaMsgConnectionToServer inherits
  // directly from IpcConnection.
  IpcConnection::setFatalError(msgStream);
}

void GuaMsgConnectionToServer::addSendCallbackBuffer(IpcMessageBuffer *buffer)
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

NABoolean GuaMsgConnectionToServer::removeSendCallbackBuffer(IpcMessageBuffer *buffer)
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

void GuaMsgConnectionToServer::handleIOError()
{
  // connection no longer usable due to I/O error. abort all existing I/Os
  // on this connection.
  for (unsigned short i = 0; i < nowaitDepth_; i++)
    {
      ActiveIOQueueEntry &entry = activeIOs_[i];
      if (entry.inUse_)
        handleIOErrorForEntry(entry);
    }

  assert(partiallySentBuffer_ == NULL);
  assert(partiallyReceivedBuffer_ == NULL);

  currentEntry_ = 0;
  numOutstandingIOs_ = 0;
}

void GuaMsgConnectionToServer::handleIOErrorForStream(IpcMessageStreamBase *msgStream)
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
void GuaMsgConnectionToServer::handleIOErrorForEntry(ActiveIOQueueEntry &entry)
{
  // I/O error occurred on given entry during send or receive

  if (getState() != ERROR_STATE)
    setState(ERROR_STATE);

  if (getErrorInfo() == 0)
      setErrorInfo(-1);

  // clean up I/O entry
  if (entry.inUse_)
    cleanUpActiveIOEntry(entry);

  // abort all existing I/Os on the same message stream
  //  - what about I/Os on other streams?
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
            nextEntry.readBuffer_->decrRefCount(getEnvironment());
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

  // if entry is the first chunk of a multi-chunk shared buffer, we need to
  // free its read buffer.
  if (entry.readBuffer_ && entry.readBuffer_ != entry.buffer_)
    entry.readBuffer_->decrRefCount(getEnvironment());

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

void GuaMsgConnectionToServer::cleanUpActiveIOEntry(ActiveIOQueueEntry &entry)
{
  short oldstop = SETSTOP(2);//become unstoppable

  // abort if we are still waiting for message reply
  if (entry.expectReply_)
    {
      MSG_ABANDON2_((NSK_msId2)entry.msgid_);
      resetAfterReply(entry.msgid_, 0, NULL);
    }

  addressUnwire(entry);

  SETSTOP(oldstop);//become stoppable 

  entry.expectReply_ = FALSE;
  entry.inUse_ = FALSE;
  entry.msgid_ = 0;
  entry.transid_ = -1;

  numOutstandingIOs_--;
}

// lock memory used for the actual message send/reply
short GuaMsgConnectionToServer::addressWire(ActiveIOQueueEntry &entry,
                                            short wireOptions)
{
  //
  // no need to lock control buffer in this method. it should have been locked
  // already by the caller.
  //

  //
  // lock memory for write/send buffer. write buffer may be shared.
  //
  short GuardianError = 0;
  if (entry.buffer_->isShared())
    {
      // write buffer is shared by multiple connections
      if (entry.buffer_->getLockCount(entry.offset_) == 0)
        {
          // if the write buffer chunk is not locked, lock it
#pragma nowarn(1506)   // warning elimination 
          GuardianError = ADDRESS_WIRE_
            ((unsigned char *)entry.buffer_->data(entry.offset_),
             entry.bytesSent_, wireOptions);
#pragma warn(1506)  // warning elimination 
          if (GuardianError)
            return GuardianError;
        }

      // increment lock count for shared write bufer
      entry.buffer_->incrLockCount(entry.offset_);
    }
  else
    {
      // write buffer is only used by a single connection
      // entry.buffer_ = entry.readBuffer_ = send buffer
#pragma nowarn(1506)   // warning elimination
      ULng32 maxDataBufferLength =
        MAXOF(entry.receiveBufferSizeLeft_, entry.bytesSent_);
      GuardianError= ADDRESS_WIRE_
        ((unsigned char *)entry.buffer_->data(entry.offset_),
         maxDataBufferLength, wireOptions);
#pragma warn(1506)  // warning elimination 
      if (GuardianError)
        return GuardianError;
    }

  //
  // lock memory for read/reply buffer. reply buffer is never shared.
  //
  if (entry.readBuffer_ && entry.readBuffer_ != entry.buffer_)
    {
      // entry.buffer_ = send buffer (shared)
      // entry.readBuffer_ = reply buffer
      assert(entry.buffer_->isShared());
#pragma nowarn(1506)   // warning elimination 
      GuardianError = ADDRESS_WIRE_
        ((unsigned char *)entry.readBuffer_->data(0),
         entry.receiveBufferSizeLeft_, wireOptions);
#pragma warn(1506)  // warning elimination 
      if (GuardianError)
        {
          // if error, unlock write buffer that must be shared
          if (entry.buffer_->decrLockCount(entry.offset_) == 0)
            // no more I/O on the write buffer. unlock its memory.
            ADDRESS_UNWIRE_((unsigned char *)entry.buffer_->data(entry.offset_),
                            entry.bytesSent_, wireOptions);

          return GuardianError;
        }
    }

  return GuardianError;
}

// unlock memory used for the actual message send/reply
void GuaMsgConnectionToServer::addressUnwire(ActiveIOQueueEntry &entry)
{
  //
  // unlock memory for control buffer
  //
  short wireOptions = 8;
  ADDRESS_UNWIRE_((unsigned char *)entry.controlBuf_, sizeof(fs_fs_writeread), wireOptions);

  //
  // unlock memory for write/send buffer. write buffer may be shared.
  //
  if (entry.buffer_->isShared())
    {
      // write buffer is shared by multiple connections
      if (entry.buffer_->decrLockCount(entry.offset_) == 0)
        // no more I/O on the write buffer. unlock its memory.
        ADDRESS_UNWIRE_((unsigned char *)entry.buffer_->data(entry.offset_),
                        entry.bytesSent_, wireOptions);
    }
  else
    {
      // write buffer is only used by a single connection
      // entry.buffer_ = entry.readBuffer_ = send buffer
      ULng32 maxDataBufferLength =
        MAXOF(entry.receiveBufferSizeLeft_, entry.bytesSent_);
      ADDRESS_UNWIRE_((unsigned char *)entry.buffer_->data(entry.offset_),
                      maxDataBufferLength, wireOptions);
    }

  //
  // unlock memory for read/reply buffer. reply buffer is never shared.
  //
  if (entry.readBuffer_ && entry.readBuffer_ != entry.buffer_)
    ADDRESS_UNWIRE_((unsigned char *)entry.readBuffer_->data(0),
                    entry.receiveBufferSizeLeft_, wireOptions);
}

#undef _resident
#undef _priv
