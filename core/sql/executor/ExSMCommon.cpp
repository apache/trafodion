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

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include "Platform.h"
#include "ExSMCommon.h"
#include "ExSMGlobals.h"
#include "ExSMEvent.h"
#include "ExSMTask.h"
#include "ExSMTrace.h"
#include "Ipc.h"
#include "Ex_esp_msg.h"
#include "ComQueue.h"

//-------------------------------------------------------------------------
// Message text buffer for SeaMonster reader thread assertion failures
//-------------------------------------------------------------------------
__thread char ExSM_AssertBuf[32];

//-------------------------------------------------------------------------
// Compare two SeaMonster targets
//-------------------------------------------------------------------------
bool ExSM_TargetsEqual(const sm_target_t &a, const sm_target_t &b)
{
  bool result = false;
  if (a.node == b.node && a.pid == b.pid && a.tag == b.tag && a.id == b.id)
    result = true;
  return result;
}

//-------------------------------------------------------------------------
// Return the thread identifier
//-------------------------------------------------------------------------
int ExSM_GetThreadID()
{
  return (int) syscall(__NR_gettid);
}

//-------------------------------------------------------------------------
// Report the largest chunk size we can transmit using SeaMonster. We
// call SeaMonster to retrieve the absolute maximum and then use 50%
// of that value as our effective maximum.
// -------------------------------------------------------------------------
uint32_t EXSM_INTRA_MAX_CHUNK = 0;
uint32_t EXSM_INTER_MAX_CHUNK = 0;
uint32_t ExSM_GetMaxChunkSize(NABoolean intra)
{
  if ( (intra & EXSM_INTRA_MAX_CHUNK == 0) ||
       (!intra & EXSM_INTER_MAX_CHUNK == 0) )
  {
    int actualMax = 0;
    int rc = 0;

    if (intra)
      rc = SM_ctl(0, SM_GET_MAXINTRABUFFSIZE, &actualMax);
    else
      rc = SM_ctl(0, SM_GET_BUFFSIZE, &actualMax);

    exsm_assert_rc(rc, "SM_ctl");
    exsm_assert(actualMax > 0,
                "SeaMonster buffer size must be greater than 0");

    if (intra)
      EXSM_INTRA_MAX_CHUNK = (uint32_t) (actualMax / 2);
    else
      EXSM_INTER_MAX_CHUNK = (uint32_t) (actualMax / 2);

    EXSM_TRACE(EXSM_TRACE_SEND | EXSM_TRACE_SM_CALLS,
               "SM_ctl sm max %d sql max %u", (int) actualMax, 
                 intra? (unsigned int) EXSM_INTRA_MAX_CHUNK : (unsigned int) EXSM_INTER_MAX_CHUNK);
  }

  if (intra)
    return EXSM_INTRA_MAX_CHUNK;
  else
    return EXSM_INTER_MAX_CHUNK;
}

//-------------------------------------------------------------------------
// SM initialize
//-------------------------------------------------------------------------
int32_t ExSM_Initialize(ExSMGlobals *smGlobals,
                        ExExeStmtGlobals *stmtGlobals)
{
  // Add an INIT event to the in-memory trace
  ExSMEvent::add(ExSMEvent::Init);
      
  // Initialize SeaMonster
  int32_t rc = SM_init(0, smGlobals->getSQNodeNum(), 0);
  char buf[30];
  SM_strerror_r(rc, buf, sizeof(buf));
  EXSM_TRACE(EXSM_TRACE_INIT|EXSM_TRACE_SM_CALLS, "SM_init(%d) returned %s",
             (int) smGlobals->getSQNodeNum(), buf);
  if (rc != 0)
  {
    ExSMEvent::add(ExSMEvent::SMError, NULL, rc);
    ExSMGlobals::addDiags("SM_init", rc, stmtGlobals);
  }
  else
  {
    // After SM is initialized but before the reader thread is
    // created, register a special SM ID for executor internal
    // communication
    int64_t exeInternalSMID = smGlobals->getExeInternalSMID();
    int32_t rc2 = ExSM_Register(exeInternalSMID);
    exsm_assert_rc(rc2, "ExSM_Register");
  }
  
  return rc;
}

//-------------------------------------------------------------------------
// SM finalize
//-------------------------------------------------------------------------
int32_t ExSM_Finalize(ExSMGlobals *smGlobals)
{
  int32_t rc = 0;

  // Cancel the special SM ID for executor internal communication
  int64_t exeInternalSMID = smGlobals->getExeInternalSMID();
  ExSM_Cancel(exeInternalSMID);

  // Call SM_finalize
  ExSMEvent::add(ExSMEvent::Finalize);
  rc = SM_finalize(0);
  char buf[30];
  SM_strerror_r(rc, buf, sizeof(buf));
  EXSM_TRACE(EXSM_TRACE_EXIT, "SM_finalize returned %s", buf);

  return rc;
}

//-------------------------------------------------------------------------
// Send a SeaMonster message
//-------------------------------------------------------------------------
int32_t ExSM_Send(ExSMGlobals *smGlobals,      // IN
                  const sm_target_t &target,   // IN
                  const void *data,            // IN
                  size_t numBytes,             // IN
                  ExSM_MessageType msgType,    // IN
                  bool isPrepostRequired,      // IN
                  bool &messageWasSent,        // OUT
                  int maxRetries,              // IN (-1=try forever)
                  int64_t sendCount,           // IN (for tracing only)
                  const void *firstChunk)       // IN (useful for queued buffers)
{
  // The sendCount value is provided by the caller and is used in
  // tracing calls only. For example a connection object can pass in a
  // value representing the count of sends so far on that connection.

  exsm_assert(smGlobals, "Invalid SM globals pointer");

  Int32 errorRetrys = 0;
  const Int32 NumErrorRetries = 10;
  timespec retryintervals[NumErrorRetries] = {
                               {  0, 10*1000*1000 }  // 10 ms
                             , {  0, 100*1000*1000 } // 100 ms
                             , {  1, 0 } // 1 second
                             , {  3, 0 } // 3 seconds
                             , {  6, 0 } // 6 seconds
                             , { 10, 0 } // 10 seconds
                             , { 15, 0 } // 15 seconds
                             , { 15, 0 } // 15 seconds
                             , { 15, 0 } // 15 seconds
                             , { 15, 0 } // 15 seconds
                           } ;

  int32_t rc = 0;
  int32_t retcode = 0;

  const char *tagForTracing = (msgType == EXSM_MSG_REQUEST ? "DN" : 
                               msgType == EXSM_MSG_REPLY ? "UP" :
                               msgType == EXSM_MSG_SHORT ? "SD" :
                               "??");

  // The count variable is for tracing only and is based on a global
  // message counter in SM globals
  uint32_t count =
    (msgType == EXSM_MSG_REQUEST ? (1 + smGlobals->getSendRequestCount()) :
     msgType == EXSM_MSG_REPLY   ? (1 + smGlobals->getSendReplyCount()) :
     msgType == EXSM_MSG_SHORT   ? (1 + smGlobals->getSendShortCount()) :
     0);

  bool done = false;
  int numRetries = 0;

  // This function has the ability to retry SM_send repeatedly in a
  // retry loop. We sleep after each attempt so the retry loop doesn't
  // turn into a busy spin. The default sleep is currently 1000
  // nano000seconds = 1 millisecond.
  UInt32 nanosecondsToSleep = 1000000;

  // Loop until one of the following is encountered
  // * Successful send
  // * Non-retryable error
  // * Retry limit is reached

  while (!done)
  {
    EXSM_TRACE(EXSM_TRACE_SEND | EXSM_TRACE_SM_CALLS,
               "SEND %d %s %d:%d:%" PRId64 ":%d:0x%c bytes %d "
               "prepost %d count %d",
               (int) count, tagForTracing,
               (int) target.node, (int) target.pid, target.id,
               (int) ExSMTag_GetTagWithoutQualifier(target.tag),
               (char) ExSMTag_GetQualifierDisplay(target.tag),
               (int) numBytes, (int) isPrepostRequired, sendCount+1);

    sm_chunk_t chunk;
    char *originalBuf  = NULL;

    memset(&chunk, 0, sizeof(sm_chunk_t));
    chunk.tgt = target;
    chunk.buff = (char *) data;
    chunk.size = numBytes;
    if (isPrepostRequired)
      chunk.flags |= SM_FLAG_SEND_TO_PREPOST;

    // firstChunk is only true for preposted buffer and null for short sends
    // only for preposted buffers we need to set the chunk handle to the starting buffer address
    // during the firstChunk is different from the chunk that we are sending
    // Also add the firstChunk to in memeory trace only when sending second chunk and beyond.
    if (firstChunk)
    {
      chunk.handle = (char *) firstChunk;
      if (chunk.buff != chunk.handle)
        originalBuf = (char *)firstChunk;
    }

    // Add a SEND event to the in-memory trace. Do not add the event
    // if this is a retry.
    ExSMEvent *event = NULL;
    if (numRetries == 0)
      event = ExSMEvent::add(ExSMEvent::Send, &target,
                 0, chunk.size, chunk.flags, (int64_t) chunk.buff, (int64_t)originalBuf);
    
    // SEND
    rc = SM_put(0, 1, &chunk);
    retcode = (rc == SM_ERR_INCHUNK ? chunk.errcode : rc);

    // Add retcode to the trace event (this does not generate a
    // new event)
    if (event)
      event->setOptional1(retcode);

    if (retcode != 0)
    {
      char buf1[30];
      char buf2[30];
      SM_strerror_r(rc, buf1, sizeof(buf1));
      SM_strerror_r(chunk.errcode, buf2, sizeof(buf2));
      EXSM_TRACE(EXSM_TRACE_SEND, "SEND %d ERROR %s %s data %p",
                 (int) count, buf1, buf2, data);
    }

    // Cases to consider
    //
    // (a) Successful send. Break from the while loop.
    // 
    // (b) The buffer is queued. Treat this the same as successful
    //     send. Break from the while loop.
    // 
    // (c) Return code is EAGAIN. Loop again unless the timeout has
    //     expired.
    //
    // (d) Return code is SM_ERR_REMOTE_TARGET.  We get this error when the other end (target)
    //     cancelled the query id.  In most cases this could happen in the middle of execution
    //     of a query and there is node down and the other end is ahead in processing the
    //     clean up and cancled the query id already but did not go away yet. 
    //     There is no way to know if the other end cancelled because it is going away
    //     or it is a real error. So retry the send for a minute or so and if the other end
    //     went away it will get SM_ERR_NO_PEER eventually for which we just ignore  the error
    //     in ESP and exit and in master it will be propagated to the user.
    //
    // (e)  Other errors. Break from the while loop and return the
    //     error code.

    if (retcode == 0 || retcode == SM_ERR_QUEUED)
    {
      // Cases (a) and (b)
      messageWasSent = true;
      done = true;
    }
    else if (retcode == SM_ERR_EAGAIN)
    {
      // Case (c)
      // Loop again if the timeout has not expired.
      // Retry if maxRetries is less than zero (the caller wants to
      // retry forever) or we have not yet reached maxRetries
      if (maxRetries < 0 || numRetries < maxRetries)
      {
        timespec tspec = { 0, (long) nanosecondsToSleep };
        nanosleep(&tspec, NULL);
        numRetries++;
      }
      else
      {
        done = true;
      }
     }
    else if (retcode == SM_ERR_REMOTE_TARGET)
    {
      // Case (d)
      // Loop till the number of error retries are not expired.
      // The sleep for EGAIN is different than error retries for the node down scenarios
      // as EAGAIN's should not wait that long to retry.
      if (errorRetrys < NumErrorRetries)
        nanosleep(&retryintervals[errorRetrys++], NULL);
      else
        done = true;
    }
    else
    {
      // Case (e)
      done = true;
    }

    // If we are about to exit the loop and an event was not generated
    // in this iteration, generate an event now
    if (done && event == NULL)
      ExSMEvent::add(ExSMEvent::SendRetry, &target, retcode, numRetries);

  } // while (!done)

  if (messageWasSent)
  {
    if (msgType == EXSM_MSG_REQUEST)
      smGlobals->incrSendRequestCount();
    else if (msgType == EXSM_MSG_REPLY)
      smGlobals->incrSendReplyCount();
    else if (msgType == EXSM_MSG_SHORT)
      smGlobals->incrSendShortCount();
  }

  return retcode;
}

int32_t ExSM_Post(const sm_target_t &target,   
                  size_t dataSize,              
                  const void *data,           
                  ExSMTask *smTask,
                  NABoolean isServer)  // IN (for tracing only)
{
  sm_chunk_t chunk;
  memset(&chunk, 0, sizeof(sm_chunk_t));
  chunk.tgt = target;
  chunk.size = dataSize;
  chunk.buff = (char *)data;
  chunk.handle = (char *)smTask;
  chunk.flags |= SM_FLAG_PREPOST;
  
  int32_t rc = SM_put(0, 1, &chunk);
  
  int32_t result = (rc == SM_ERR_INCHUNK ? chunk.errcode : rc);

  // Add a POST event to the in-memory trace
  ExSMEvent::add(ExSMEvent::Post, &target,
               result, chunk.size, chunk.flags, (int64_t) chunk.buff);
 
  EXSM_TRACE(EXSM_TRACE_BUFFER | EXSM_TRACE_SM_CALLS,
             "PREPOST %s %d:%d:%" PRId64 ":%d:0x%c %p %d rc %d %d",
             (isServer ? "S" : "C"),
             (int) target.node, (int) target.pid, target.id,
             (int) ExSMTag_GetTagWithoutQualifier(target.tag),
             (char) ExSMTag_GetQualifierDisplay(target.tag),
             chunk.buff, (int) dataSize,
             (int) rc, (int) chunk.errcode);

  return result;
}


//-------------------------------------------------------------------------
// Function to send a SeaMonster short message
//-------------------------------------------------------------------------
int32_t ExSM_SendShortMessage(ExSMGlobals *smGlobals,
                              const sm_target_t &target,
                              const void *data,
                              size_t bytesToSend)
{
  bool messageWasSent = false;
  int32_t result =
    ExSM_Send(smGlobals,
              target,
              data,
              bytesToSend,
              EXSM_MSG_SHORT,
              false,           // isPrepostRequired
              messageWasSent,  // OUT
              -1,              // retry forever
              0, NULL);        // send count (for tracing only)
  return result;
}

//-------------------------------------------------------------------------
// Wrapper function used by the reader thread to set the length of an
// IPC message buffer. Eliminates the need for the reader thread
// function to include or be aware of our IPC classes.
//-------------------------------------------------------------------------
void ExSM_SetMessageLength(void *buf, size_t len)
{                               
  IpcMessageBuffer *msgBuf = (IpcMessageBuffer *) buf;
  msgBuf->setMessageLength(len);
}

//-------------------------------------------------------------------------
// Wrapper function used by the reader thread to allocate an
// IpcMessageBuffer. The size of the data region in the new buffer
// will be dataBytes. Eliminates the need for the reader thread
// function to include or be aware of our IPC classes.
// -------------------------------------------------------------------------
void *ExSM_AllocateMessageBuffer(size_t dataBytes,
                                 NAMemory *threadSafeHeap)
{
  IpcMessageBuffer *msgBuf = 
    IpcMessageBuffer::allocate(dataBytes,
                               NULL,            // IpcMessageStreamBase *
                               threadSafeHeap,  // NAMemory *
                               0);
  return msgBuf;
}

//-------------------------------------------------------------------------
// Function to return the SeaMonster prepost address of a message
// buffer. The prepost address is the first byte of data that gets
// transmitted. The IpcMessageBuffer header is not transmitted.
//-------------------------------------------------------------------------
void *ExSM_GetMessageBufferPrepostAddr(void *data)
{
  void *result = NULL;
  if (data)
  {
    IpcMessageBuffer *msgBuf = (IpcMessageBuffer *) data;
    result = msgBuf->data();
  }
  return result;
}

//-------------------------------------------------------------------------
// Function to return the address of a message buffer given the
// SeaMonster prepost address. The prepost address immediately follows
// an IpcMessageBuffer instance.
//-------------------------------------------------------------------------
void *ExSM_GetMessageBufferAddr(void *prepostAddr)
{
  void *result = NULL;
  if (prepostAddr)
    result = ((char *) prepostAddr - sizeof(IpcMessageBuffer));

  return result;
}

//-------------------------------------------------------------------------
// Function to return the IPC message object type of the first object
// that follows the header (the header is an InternalMsgHdrInfoStruct)
// in the IpcMessageBuffer pointed to by the data argument.
//
// If the object type cannot be determined "UNKNOWN" is returned.
//-------------------------------------------------------------------------
const char *ExSM_GetMessageBufferType(void *data, size_t dataBytes)
{
  if (data && dataBytes >= sizeof(IpcMessageObj))
  {
    IpcMessageObj *msgObj = (IpcMessageObj *) data;
    IpcMessageObj *next = msgObj->getNextFromOffset();
    if (next && ((void *) next > data) &&
        (((char *) data + dataBytes) >=
         ((char *) next + sizeof(IpcMessageObj))))
    {
      Int32 type = next->getType();
      return getESPMessageObjTypeString((ESPMessageObjTypeEnum) type);
    }
  }

  return "UNKNOWN";
}

//-------------------------------------------------------------------------
// Function to map between virtual and real SQ node numbers
//-------------------------------------------------------------------------
int32_t ExSM_GetNodeID(int32_t nodeNum)
{
  // Two cases to consider:
  //
  // (a) Running on a cluster, or running on a workstation with both
  //     Seaquest and SeaMonster using virtual nodes. This is default
  //     behavior. The function will return the value of nodeNum that was
  //     passed in.
  //
  // (b) This is a workstation, Seaquest is running with virtual
  //     nodes, but SeaMonster is not running with virtual nodes. From
  //     SeaMonster's point of view there is only one node and it does
  //     not matter how many virtual nodes we have. The function will
  //     always return 0.
  // 
  //     This case is detected when:
  //       SQ_VIRTUAL_NODES env var is not NULL
  //       SM_VIRTUALNODE env var is "0"

  static bool firstTime = true;
  static bool smVirtualNodes = true;

  if (firstTime)
  {
    firstTime = false;

    if (getenv("SQ_VIRTUAL_NODES"))
    {
      // If we are running with SQ virtual nodes, but without
      // SeaMonster virtual nodes, we need to set smVirtualNodes to
      // false
      char *sm_envvar = getenv("SM_VIRTUALNODE");
      if (sm_envvar && sm_envvar[0] == '0')
        smVirtualNodes = false;
    }
  }

  if (smVirtualNodes)
    return nodeNum;
  else
    return 0; // from SeaMonster's point of view it does not matter how
              // many virtual nodes we have, we are on the same hw.
}

//-------------------------------------------------------------------------
// We maintain a global list of completed send buffers
//  * Each list node is an instance of struct ExSM_MessageList
//  * The list is protected by a global mutex
//  * The list nodes are allocated on a thread-safe heap
//  * The list is only visible in this file. Global functions are
//    provided to add and remove elements.
//-------------------------------------------------------------------------
struct ExSM_MessageList
{
  IpcMessageBuffer *elem_;
  ExSM_MessageList *next_;
};

// Global list pointer
static ExSM_MessageList *EXSM_COMPLETED_SENDS = NULL;

// Global mutex
static pthread_mutex_t COMPLETED_SEND_MUTEX = PTHREAD_MUTEX_INITIALIZER;

// Function to add a list element
void ExSM_AddCompletedSendBuffer(void *buf)
{
  // Get a pointer to the thread-safe IPC heap
  ExSMGlobals *glob = ExSMGlobals::GetExSMGlobals();
  NAMemory *heap = glob->getThreadSafeHeap();
  exsm_assert(heap, "Invalid heap pointer");
  
  // Allocate a new list node
  ExSM_MessageList *newElem = (ExSM_MessageList *)
    heap->allocateMemory(sizeof(ExSM_MessageList));
  
  // Lock the list
  int rc = pthread_mutex_lock(&COMPLETED_SEND_MUTEX);
  exsm_assert_rc(rc, "pthread_mutex_lock");

  // Add the new node to the head of the list
  newElem->elem_ = (IpcMessageBuffer *) buf;
  newElem->next_ = EXSM_COMPLETED_SENDS;
  EXSM_COMPLETED_SENDS = newElem;

  // Unlock the list
  rc = pthread_mutex_unlock(&COMPLETED_SEND_MUTEX);
  exsm_assert_rc(rc, "pthread_mutex_unlock");
}

// Function to remove a list element. The element is returned in the
// buf output argument. The return value will be true if the list was
// non-empty and an element did get removed.
bool ExSM_RemoveCompletedSendBuffer(void *&buf)
{
  buf = NULL;
  bool removed = false;

  if (EXSM_COMPLETED_SENDS)
  {
    // Lock the list
    int rc = pthread_mutex_lock(&COMPLETED_SEND_MUTEX);
    exsm_assert_rc(rc, "pthread_mutex_lock");
    
    // Remove the head node
    ExSM_MessageList *headNode = EXSM_COMPLETED_SENDS;
    if (headNode)
      EXSM_COMPLETED_SENDS = headNode->next_;

    // Unlock the list
    rc = pthread_mutex_unlock(&COMPLETED_SEND_MUTEX);
    exsm_assert_rc(rc, "pthread_mutex_unlock");
    
    if (headNode)
    {
      // Keep a copy of the first element
      buf = headNode->elem_;
      
      // Delete the old head node
      ExSMGlobals *glob = ExSMGlobals::GetExSMGlobals();
      NAMemory *heap = glob->getThreadSafeHeap();
      exsm_assert(heap, "Invalid heap pointer");
      heap->deallocateMemory(headNode);
      
      removed = true;
    }
  }
  
  return removed;
}

//-------------------------------------------------------------------------
// We maintain a global collection of active IDs and functions to
// register, cancel, and find an ID. Access to the collection is
// protected by a mutex. The register function is a no-op if the ID
// is already in the collection, and the cancel function is a no-op
// if the ID is not in the collection.
//-------------------------------------------------------------------------
HashQueue *EXSM_IDS = NULL;
pthread_mutex_t EXSM_IDS_MUTEX = PTHREAD_MUTEX_INITIALIZER;

// Register an ID and add it to the global collection of query
// IDs. The function is a no-op if the ID is already in the
// collection. 
//
// Returns 0 if no errors were encountered, otherwise a SeaMonster
// error code.
int32_t ExSM_Register(const int64_t &smQueryID)
{
  int32_t smResult = 0;
  int rc = 0;

  // Lock the collection
  rc = pthread_mutex_lock(&EXSM_IDS_MUTEX);
  exsm_assert_rc(rc, "pthread_mutex_lock");
  
  // Create the collection if necessary
  if (EXSM_IDS == NULL)
  {
    ExSMGlobals *glob = ExSMGlobals::GetExSMGlobals();
    NAMemory *heap = glob ? glob->getThreadSafeHeap() : NULL;
    exsm_assert(heap, "Invalid heap pointer");
    
    EXSM_IDS = new (heap) HashQueue(heap);
    exsm_assert(EXSM_IDS, "Invalid hash queue pointer");
  }

  // Register the ID if it is not already in the collection
  bool found = ExSM_FindID(smQueryID, false);
  if (found)
  {
    EXSM_TRACE(EXSM_TRACE_SM_CALLS,
               "REGISTER %" PRId64 " already active, entries %d\n",
               smQueryID, (int) EXSM_IDS->numEntries());
  }
  else
  {
    // Add a REGISTER event to the in-memory trace. The ID is passed
    // in a dummy sm_target_t structure.
    sm_target_t tgt;
    memset(&tgt, 0, sizeof(tgt));
    tgt.id = smQueryID;
    ExSMEvent::add(ExSMEvent::Register, &tgt);

    // Register the ID. Store the SM return value and return it to the
    // caller. This function does not react to SM errors -- that is
    // left to the caller.
    smResult = SM_register(0, smQueryID);
    EXSM_TRACE(EXSM_TRACE_CANCEL | EXSM_TRACE_SM_CALLS,
               "SM_register %" PRId64 " rc %d entries %d",
               smQueryID, (int) smResult, (int) EXSM_IDS->numEntries());

    if (smResult == 0)
    {
      // Add the ID to the global collection of active IDs
      EXSM_IDS->insert((char *) &smQueryID, sizeof(smQueryID),
                       (void *) smQueryID);
    }
  }
  
  // Unlock the collection
  rc = pthread_mutex_unlock(&EXSM_IDS_MUTEX);
  exsm_assert_rc(rc, "pthread_mutex_lock");

  return smResult;
}

// Cancel an ID and remove it from the global collection of query
// IDs. The function is a no-op if the ID is not already in the
// collection.
//
// Returns 0 if no errors were encountered, otherwise a SeaMonster
// error code.
int32_t ExSM_Cancel(const int64_t &smQueryID)
{
  int32_t smResult = 0;

  if (EXSM_IDS)
  {
    int rc = 0;
    
    // Lock the collection
    rc = pthread_mutex_lock(&EXSM_IDS_MUTEX);
    exsm_assert_rc(rc, "pthread_mutex_lock");
    
    // Cancel the ID if it is already in the collection
    bool found = ExSM_FindID(smQueryID, false);
    if (found)
    {
      // Remove the ID from the global collection of active IDs
      EXSM_IDS->remove((char *) &smQueryID, sizeof(smQueryID),
                       (void *) smQueryID);
      
      // Add a CANCEL event to the in-memory trace. The ID is passed
      // in a dummy sm_target_t structure.
      sm_target_t tgt;
      memset(&tgt, 0, sizeof(tgt));
      tgt.id = smQueryID;
      ExSMEvent::add(ExSMEvent::Cancel, &tgt);
      
      // Cancel the ID. Store the SM return value and return it to the
      // caller. This function does not react to SM errors -- that is
      // left to the caller.
      smResult = SM_cancel(0, smQueryID);
      EXSM_TRACE(EXSM_TRACE_CANCEL | EXSM_TRACE_SM_CALLS,
                 "SM_cancel %" PRId64 " rc %d entries %d",
                 smQueryID, (int) smResult, (int) EXSM_IDS->numEntries());
    }
    else
    {
      EXSM_TRACE(EXSM_TRACE_SM_CALLS,
                 "CANCEL %" PRId64 " is not active, entries %d\n",
                 smQueryID, (int) EXSM_IDS->numEntries());
    }
    
    // Unlock the collection
    rc = pthread_mutex_unlock(&EXSM_IDS_MUTEX);
    exsm_assert_rc(rc, "pthread_mutex_lock");
  }

  return smResult;
}

// Find an ID in the global collection. If lock is true, access is
// protected by a mutex.
bool ExSM_FindID(const int64_t &smQueryID, bool lock)
{
  bool result = false;
  
  if (EXSM_IDS)
  {
    int rc = 0;
    
    // Lock the collection if the caller requested locking
    if (lock)
    {
      rc = pthread_mutex_lock(&EXSM_IDS_MUTEX);
      exsm_assert_rc(rc, "pthread_mutex_lock");
    }
    
    // Walk the hash bucket looking for a matching ID
    void *entry = NULL;
    EXSM_IDS->position((char *) &smQueryID, sizeof(smQueryID));
    while ((entry = EXSM_IDS->getNext()) != NULL)
    {
      int64_t current = (int64_t) entry;
      if (current == smQueryID)
      {
        result = true;
        break;
      }
    }
    
    // Unlock the collection if the caller requested locking
    if (lock)
    {
      rc = pthread_mutex_unlock(&EXSM_IDS_MUTEX);
      exsm_assert_rc(rc, "pthread_mutex_lock");
    }
  }

  return result;
}
