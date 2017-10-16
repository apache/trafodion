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
 * File:         IpcMsg.h
 * Description:  Classes to establish and perform data exchange between
 *               processes using the NSK messaging API.
 * 
 * Created:      6/25/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#ifndef IPCMSGH
#define IPCMSGH

#include "Ipc.h"

// -----------------------------------------------------------------------
// A Guardian connection on the client side that connects to a server
// by opening its process file
// -----------------------------------------------------------------------
class GuaMsgConnectionToServer : public IpcConnection
{
  friend class IpcGuardianServer;

public:

  GuaMsgConnectionToServer(IpcEnvironment *env,
			const IpcProcessId &procId,
			NABoolean usesTransactions,
			unsigned short nowaitDepth,
                        char *eye = (char *)eye_GUA_MSG_CONNECTION_TO_SERVER);

  virtual ~GuaMsgConnectionToServer();

  // send or receive a message through the connection,
  // call the callback when the I/O completes
  virtual void send(IpcMessageBuffer *buffer);
  virtual void receive(IpcMessageStreamBase *msg);

  virtual NABoolean moreWaitsAllowed();

  // wait until a send or receive operation completed
  // return TRUE only if an interrupt was received
  virtual WaitReturnStatus wait(IpcTimeout timeout, UInt32 *eventConsumed, IpcAwaitiox *ipcAwaitiox = NULL);
  
  virtual GuaMsgConnectionToServer *castToGuaMsgConnectionToServer();

  virtual Int32 numQueuedSendMessages();
  virtual Int32 numQueuedReceiveMessages();

  inline GuaErrorNumber getGuardianError() const { return guaErrorInfo_; }

  inline void setAbortTransOnPathErrorsFlag() { abortXnOnPathErrors_ = TRUE; }
  inline NABoolean getAbortTransOnPathErrorsFlag() { return abortXnOnPathErrors_; }

  // set error info
  virtual void populateDiagsArea(ComDiagsArea *&diags, CollHeap *diagsHeap);

  inline short getFileNumForLogging() const { return openFile_; }

  // struct is public only to make the compiler happy
  struct ActiveIOQueueEntry
  {
    // TRUE if an I/O is in progress for this entry, FALSE otherwise
    NABoolean         inUse_;
    
    //TRUE if we want to check for a reply to this message
    NABoolean         expectReply_;

    //the message id returned by MSG_LINK_ for this queue entry
    UInt32 msgid_;
   
    // how many bytes have been sent in this operation (0 for a READX)
    IpcMessageObjSize bytesSent_;

    // what's the size of the receive buffer (0 for WRITEX)
    IpcMessageObjSize receiveBufferSizeLeft_;

    // what's the offset in buffer_ where the I/O buffer started
    IpcMessageObjSize offset_;

    //pointer to the CBA to be used for the read data buffer
    void * readDataCBAPtr_;
    
    //pointer to the CBA to be used for the write data buffer
    void * writeDataCBAPtr_;
    
    // the message buffer to be sent
    IpcMessageBuffer  *buffer_;
    
    // the message buffer to be received
    IpcMessageBuffer *readBuffer_;
    
    //pointer to the CBA to be used for the control buffer
    void * controlCBAPtr_;
    
    //contains control info to sent to/received from the server
    void * controlBuf_;

    // This is used to keep track of the transid associated with
    // the message, in case the transaction needs to be aborted
    // if the server connection dies.
    Int64  transid_;
  };

  // Used after fatal error to avoid deadlock.
  virtual void setFatalError(IpcMessageStreamBase *msgStream);

private:

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

  //This is the entry we will check next for completion of I/O from the activeIOs_ array;
  //we will go in a round robin fashion wrapping around after nowaitDepth_-1
  //the first entry we see with entry.inUse_ ==TRUE we will check for IO completion
  unsigned short currentEntry_;
  
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

  // on certain path errors, need to stop transaction in progress. 
  // This is to fix the bug reported in solution 10-030508-6267.
  NABoolean         abortXnOnPathErrors_;

  // information about the error returned from Guardian in case the
  // connection is in the ERROR state
  GuaErrorNumber    guaErrorInfo_;

  // private methods

  // Try to issue one nowait WRITEREADX call and return
  // TRUE if one of these operations was successfully started. Out of
  // band messages are placed ahead of an already existing message queue.
  NABoolean tryToStartNewIO();

  void addSendCallbackBuffer(IpcMessageBuffer *buffer);
  NABoolean removeSendCallbackBuffer(IpcMessageBuffer *buffer);

  void handleIOError();
  void handleIOErrorForStream(IpcMessageStreamBase *msgStream);
  void handleIOErrorForEntry(ActiveIOQueueEntry &entry);
  void cleanUpActiveIOEntry(ActiveIOQueueEntry &entry);
  short addressWire(ActiveIOQueueEntry &entry, short wireOptions);
  void addressUnwire(ActiveIOQueueEntry &entry);

  // open/close the connected server process
  void openPhandle(char * processName = NULL);
  void closePhandle();

  // setup the requestheader before sending message
  // through MSG_LINK_
  short setupRequestInfo(void* controlInfo, Int64 transid);
  
  //reset acb info after message is received
  void resetAfterReply(UInt32 msgid, short error, Int64 *transid);

  //put the msgid into the acb so that on exit
  //the file system cleans up
  void putMsgIdinACB(UInt32 msgid);

};

#endif //IPCMSGH
