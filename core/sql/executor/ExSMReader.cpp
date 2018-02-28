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

#include <unistd.h>
#include <time.h>
#include "Platform.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "ExSMReader.h"
#include "ExSMCommon.h"
#include "ExSMTrace.h"
#include "ExSMGlobals.h"
#include "ExSMTask.h"
#include "ExSMTaskList.h"
#include "ExSMReadyList.h"
#include "ExSMQueue.h"
#include "ExSMEvent.h"
#include "ExSMShortMessage.h"
#include "NAAssert.h"

void *ExSM_ReaderFunction(void *arg)
{
  int32_t rc = NAAssertMutexCreate();
  if (rc < 2) // Main executor thread must precede this thread
    abort();

  ExSMGlobals *glob = (ExSMGlobals *) arg;
  exsm_assert(glob, "Invalid SM globals pointer");

  // Initialize the in-memory trace
  ExSMEvent::initReaderThread();

  // Settings for tracing to a file
  UInt32 tLevel = glob->getTraceLevel();
  const char * tPrefix = glob->getTraceFilePrefix();
  ExSM_SetTraceLevel(tLevel);
  ExSM_SetTraceEnabled(glob->getTraceEnabled(), glob);

  pid_t mainThreadPID = glob->getMainThreadPID();
  bool isMaster = glob->isMasterExecutor();

  char readerPrefix[20];
  sprintf(readerPrefix, "[R%d]", (int) mainThreadPID);

  EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_SM_CALLS,
             "%s BEGIN READER (%s %d)", readerPrefix,
             isMaster ? "MASTER" : "ESP", (int) mainThreadPID);

  ExSMTaskList *smTaskList = glob->getSMTaskList();
  ExSMReadyList *readyList = glob->getReadyList();
  int64_t loopCount = 0;

  // retryCount is the number of times SM_get has been called and
  // returned zero chunks. retryCount is reset to 0 every time SM_get
  // returns one or more chunks.
  int32_t retryCount = 0;

  glob->setReaderThreadState(ExSMGlobals::STARTED,
                             true,  // do nocking
                             true); // signal

  while (glob->getReaderThreadState() == ExSMGlobals::STARTED)
  {
    loopCount++;

    // If tracing got turned on via set session default, the main
    // thread learns about it at fixup time and the reader thread will
    // learn about it the second time through this loop after fixup,
    // since the trace level changed.
    if ((tLevel != glob->getTraceLevel()) || 
        (strcmp(tPrefix, glob->getTraceFilePrefix()))) 
    {
      tLevel = glob->getTraceLevel();
      tPrefix = glob->getTraceFilePrefix();
      ExSM_SetTraceLevel(tLevel);
      ExSM_SetTraceEnabled(glob->getTraceEnabled(), glob);
    }

    EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
               "%s LOOP %ld", readerPrefix, (long) loopCount);

    sm_chunk_t *chunks = NULL;
    int32_t nchunks = 0;
    sm_handle_t dataHandle = 0;

#ifndef NDEBUG
    // In the debug build we can delay the first call to
    // SM_get. This simulates "slow reap" of the server-side
    // prepost. In response to the slow reap, the main thread should
    // see ERR_PREPOST errors when it sends and retry the send.
    if (loopCount == 1)
    {
      const char *envvar = getenv("EXSM_WAIT_DELAY");
      if (envvar && *envvar)
      {
        Int32 seconds = atoi(envvar);
        if (seconds > 0)
        {
          EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
                     "EXSM_WAIT_DELAY %d\n", (int) seconds);
          sleep(seconds);
        }
      }
    }
#endif

    // Wait for arrivals
    rc = SM_get(0, &chunks, &nchunks, &dataHandle);

    // Add a RECEIVE completion event to the in-memory trace
    ExSMEvent::add(ExSMEvent::Receive, NULL, rc);

    EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT|EXSM_TRACE_SM_CALLS,
               "%s  wait rc %d n %d h %p", readerPrefix, 
               (int) rc, (int) nchunks, (void *) dataHandle);

    if (rc == SM_ERR_NOSERVICE)
    {
       glob->handleReaderThreadError(rc, "SM_get", dataHandle);
       return 0;
    }

    // We want to process the chunk array if rc is zero or rc
    // indicates that error codes are in individual chunks. Otherwise
    // assert.
    if (rc != SM_ERR_INCHUNK)
      exsm_assert_rc(rc, "SM_get");

    // Go back to the top of the main loop if no chunks were returned
    if (chunks == NULL || nchunks < 1)
    {
      retryCount++;
      EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
                 "%s  wait will be retried, count %d", readerPrefix,
                 (int) retryCount);
      continue;
    }

    // If we reach here, SM_get returned a chunk array for
    // processing. Reset the retry count and process the arrivals.
    retryCount = 0;

    // Lock the task list
    smTaskList->lock();
    EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
               "%s   locked task list, tasks %d", readerPrefix,
               (int) smTaskList->getNumTasks());

    // A container for short messages
    ExSMShortMessage shortMsg;
    ExSMShortMessage::MsgType shortMsgType = ExSMShortMessage::UNKNOWN;
    
    // Process each arrival
    for (int32_t i = 0; i < nchunks; i++)
    {
      sm_chunk_t &chunk = chunks[i];

      // Add a CHUNK event to the in-memory trace to show that we
      // are processing a new chunk. 
      //
      // The fourth integer provided to ExSMEvent::add() will be one
      // of the following:
      // * Short message with length >= 4: first four data bytes
      // * Otherwise: chunk.buff
      // 
      // Short message data bytes are included in the trace event
      // because that is where we store the message type.
      // 
      // NOTE: We read the first four data bytes ONLY for short
      // messages. For prepost messages we should not access
      // chunk.buff until after we verify the task exists and the
      // query ID is valid. If the query ID is not valid, that means
      // the main thread already called SM cancel for and may have
      // already deleted the buffer pointed to by chunk.buff.
      int64_t fourthArg = (int64_t) chunk.buff;

      if ((chunk.flags & SM_FLAG_PREPOST) == 0)
      {
        int32_t firstFourBytes = 0;
        if ((chunk.buff && chunk.size >= 4))
          memcpy(&firstFourBytes, chunk.buff, 4);
        fourthArg = (int64_t) firstFourBytes;
      }

      ExSMEvent *event = ExSMEvent::add(ExSMEvent::Chunk, &chunk.tgt,
                     chunk.errcode,
                     chunk.size,
                     chunk.flags,
                     fourthArg);

      // Is the chunk a request or reply
      bool isRequest = !ExSMTag_GetReplyFlag(chunk.tgt.tag);

      // Retrieve the global request or reply counter from SM globals
      uint32_t count = 0;
      if ((chunk.flags & SM_FLAG_PREPOST) == 0)
        count = glob->incrRecvShortCount();
      else if (isRequest)
        count = glob->incrRecvRequestCount();
      else
        count = glob->incrRecvReplyCount();

      // Add chunk information to the trace file
      EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT|EXSM_TRACE_SM_CALLS,
                 "%s   %s %d src %d:%d:%" PRId64 ":%d:0x%c fl 0x%x",
                 readerPrefix,
                 (isRequest ? "REQUEST" : "REPLY"), (int) count,
                 (int) chunk.tgt.node, (int) chunk.tgt.pid, chunk.tgt.id,
                 (int) ExSMTag_GetTagWithoutQualifier(chunk.tgt.tag),
                 (char) ExSMTag_GetQualifierDisplay(chunk.tgt.tag),
                 (int) chunk.flags);

      char buf[30];
      SM_strerror_r(chunk.errcode, buf, sizeof(buf));

      EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT|EXSM_TRACE_SM_CALLS,
                 "%s   sz %d err %s h %p buf %p", readerPrefix,
                 (int) chunk.size, buf,
                 chunk.handle, chunk.buff);

      // Add chunk size to the trace file if this is one fragment of a
      // SQL data buffer
      if (ExSMTag_GetSQLChunkFlag(chunk.tgt.tag))
        EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_BUFFER,
                   "%s   RECV CHUNK %d", readerPrefix, (int) chunk.size);

      if (chunk.errcode != 0)
      {
        // Cases to consider
        //
        // If chunk.errcode is LOCAL_TARGET (meaning a chunk arrived
        // but the ID was already cancelled):
        // 
        //   (a) The ID in the chunk is not in the global collection
        //       of active IDs. Treat this as a late arrival for a 
        //       query that already ended. Discard the chunk.
        // 
        //       If a garbage ID value arrives we will most likely
        //       discard the chunk rather than reporting an
        //       error. Currently we are willing to live with this
        //       risk.
        // 
        //   (b) The ID is active. This is an internal error.
        // 
        // Otherwise
        // 
        //   (c) This is an internal error

        if (chunk.errcode == SM_ERR_LOCAL_TARGET)
        {
          int64_t smQueryID = chunk.tgt.id;
          bool found = ExSM_FindID(smQueryID);
          if (!found)
          {
            // Case (a)
            ExSMEvent::add(ExSMEvent::IDNotActive, &chunk.tgt,
                           chunk.errcode, chunk.flags);
            EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_SM_CALLS,
                       "ID is not active, LOCAL_TARGET error will be ignored");
            continue;
          }
        }
        else if (chunk.errcode == SM_ERR_NODE_DOWN  ||
                 (chunk.errcode == SM_ERR_PREPOST) ||
                 (chunk.errcode == SM_ERR_NOSERVICE))
        {
          glob->handleReaderThreadError(chunk.errcode, "SM_get", dataHandle, smTaskList);
          return 0;
        }

        // Cases (b) and (c)
        char buf[64];
        char buf2[30];
        SM_strerror_r(chunk.errcode, buf2, sizeof(buf));
        snprintf(buf, sizeof(buf), "chunk.errcode == %s", buf2);
        exsm_assert(FALSE, buf);
      }

      if (chunk.flags & SM_FLAG_SENDNOTIFICATION)
      {
        // SM is returning notification of a completed send. In
        // response the reader thread places the send buffer on a
        // global queue which the main thread cleans up periodically.
        // 
        // The cleanup happens in
        // IpcEnvironment::deleteCompletedMessages()

        exsm_assert(chunk.buff, "QUEUED event arrived but buffer is NULL");
        exsm_assert(chunk.handle, "QUEUED event arrived but handle is NULL");

        if (event)
          event->setOptional5((int64_t)chunk.handle);

        void *bufferAddr = ExSM_GetMessageBufferAddr(chunk.handle);
        ExSM_AddCompletedSendBuffer(bufferAddr);

        
        EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_SM_CALLS,
                   "COMPLETED SEND data %p msg %p\n",
                   chunk.buff, chunk.handle);

        continue;
      }

      // Verify that the chunk has a data region. Currently there is
      // always a data region even for short messages, because the
      // short message type travels as data and every short message
      // must have a type.
      exsm_assert(chunk.buff, "SM chunk arrived, invalid buff pointer");

      // The target we use for task lookup is not always the target
      // that just arrived. Today the only exception is:
      // * For a prepost message that is a "large buffer" the tag for
      //   lookup is the sender's tag with the LARGE_MSG flag cleared
      sm_target_t targetForTaskLookup = chunk.tgt;

      // Is the chunk a fragment of a SQL data buffer. This variable
      // is initially false but can become true later when we look at
      // flags inside the chunk.
      bool receivedSQLBufferFragment = false; 

      if ((chunk.flags & SM_FLAG_PREPOST) == 0)
      {
        // This is a short message

        // Initialize the short message container (a local variable)
        // from the chunk that just arrived.
        shortMsg.receive(chunk);
        shortMsgType = (ExSMShortMessage::MsgType) shortMsg.getValue(0);

        if (EXSM_TRACE_ENABLED)
          shortMsg.writeToTrace(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT|
                                EXSM_TRACE_SM_CALLS,
                                readerPrefix, "   ");
        
        // Certain short messages require no further
        // processing. Examples are the SHUTDOWN and FIXUP_REPLY
        // messages. We do not need to locate a task object and
        // execute logic specific to that task. All we need to do is
        // change state variables and return to the top of the loop
        // that processes each individual chunk.

        if (shortMsgType == ExSMShortMessage::SHUTDOWN)
        {
          // When a SHUTDOWN message is received:
          // * return to the top of the chunk processing loop
          // * following the chunk loop, return to the top of the main loop
          // * the SHUTDOWN state will be noticed and the main loop
          //   will terminate
          // * following the main loop, the reader thread will exit

          glob->setReaderThreadState(ExSMGlobals::PROCESSING_SHUTDOWN);
          continue;
        }

        else if (shortMsgType == ExSMShortMessage::FIXUP_REPLY)
        {
          // One FIXUP_REPLY is sent from ESP to master each time an
          // ESP successfully fixes up a fragment. There is no
          // interesting content in these messages. But they need to
          // flow from ESP to master as an SM "go message".
          //
          // Without these replies flowing from ESP to master, if the
          // master were to send a data request to the ESP, the
          // receiving SM service might report that ESP preposts are
          // not yet available.
          //
          // When a FIXUP_REPLY is received:
          // * Increment a global counter. The master is watching this
          //   counter and when the counter reaches a certain value,
          //   the master assumes all ESPs are ready for SM data
          //   requests.
          glob->incrFixupReplyCount();
          EXSM_TRACE(EXSM_TRACE_RDR_THR,
                     "%s   Fixup reply count %d", readerPrefix,
                     (int) glob->getFixupReplyCount());
          continue;
        }
      }
      else
      {
        // This is a prepost message

        // If this is one fragment of a SQL buffer, note that fact and
        // adjust the target for task lookup
        if (ExSMTag_GetSQLChunkFlag(chunk.tgt.tag))
        {
          ExSMTag_ClearSQLChunkFlag(&targetForTaskLookup.tag);
          receivedSQLBufferFragment = true;
        }
      }

      // Find the task
      bool doLock = false;
      bool doTrace = false;
      ExSMTask *task = smTaskList->findTask(targetForTaskLookup,
                                            doLock,
                                            doTrace);
      if (task == NULL)
      {
        EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT, 
                   "%s *** ERROR: SM TASK NOT FOUND", readerPrefix);

        // Perform the lookup again with tracing enabled. This lookup
        // is expected to fail just like the first lookup failed.
        doTrace = true;
        task = smTaskList->findTask(targetForTaskLookup,
                                    doLock,
                                    doTrace);

        exsm_assert(task == NULL,
                    "Task lookup should not succeed after first lookup failed");

        // If the ID is active, this is an unexpected arrival and we
        // will assert. If the ID is not active, we assume this is a
        // late arrival for a query that ended and silently ignore the
        // chunk.
        int64_t smQueryID = chunk.tgt.id;
        bool found = ExSM_FindID(smQueryID);
        if (!found)
        {
          ExSMEvent::add(ExSMEvent::IDNotActive, &chunk.tgt,
                         chunk.errcode, chunk.flags);
          EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_SM_CALLS,
                     "ID is not active, chunk will be ignored");
          continue;
        }
        
      } // if (task == NULL)

      exsm_assert(task, "SM task not found");
        
      if ((chunk.flags & SM_FLAG_PREPOST) != 0)
      {
        // This is a prepost message. Print the message type to the
        // trace file.
        EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT|
                   EXSM_TRACE_SM_CALLS, 
                   "%s   msg type %s", readerPrefix,
                   ExSM_GetMessageBufferType(chunk.buff, chunk.size));
      }

      task->incrReceiveCount();

      EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
                 "%s   tcb %p recv cnt %d", readerPrefix,
                 task->getTCB(), (int) task->getReceiveCount());

      if ((chunk.flags & SM_FLAG_PREPOST) == 0)
      {
        // This is a short message. The short message contents have
        // already been copied into the shortMsg variable. We now
        // check the message type and process accordingly.

        // BUFFER SIZE
        if (shortMsgType == ExSMShortMessage::SIZE)
        {
          // Message size and chunk size are the second and third
          // values in the short message. The collection of values is
          // 0-based so we need to extract slots 1 and 2.
          uint32_t msgSize = (uint32_t) shortMsg.getValue(1);
          uint32_t chunkSize = (uint32_t) shortMsg.getValue(2);
          
          void *largeBuffer =
            ExSM_AllocateMessageBuffer(msgSize, glob->getThreadSafeHeap());
          exsm_assert(largeBuffer, "Allocation failed for largeBuffer");

          task->recvChunk_Enter(largeBuffer, msgSize, chunkSize);
          
          // Next step is to post the receive buffer. The target for
          // this post is the same as the sender's target with the
          // LARGE MSG flag set in the tag.

          // Reasons for setting the LARGE MSG flag:
          // * If we did not set the flag, the new post would be
          //   queued behind other buffers already posted for the
          //   same target
          // * We don't want this post at the back of the queue
          // * We set a flag in the tag so this post is seen at the
          //   front of a different queue

          sm_target_t target = chunk.tgt;
          ExSMTag_SetSQLChunkFlag(&target.tag);
          
          char *chunkAddr = (char *) task->recvChunk_GetPrepostAddr();

          int32_t rc = ExSM_Post(target, chunkSize, chunkAddr, task, 0);
          
          if (rc == SM_ERR_NOSERVICE || 
              rc == SM_ERR_NOPEER ||  
              rc == SM_ERR_NODE_DOWN)
          {
            glob->handleReaderThreadError(rc, "ExSM_Post", dataHandle, smTaskList);
            return 0;
          }

          EXSM_TRACE(EXSM_TRACE_BUFFER|EXSM_TRACE_SM_CALLS,
                     "PREPOST %d:%d:%" PRId64 ":%d:0x%c %p sz %d",
                     (int) target.node, (int) target.pid, target.id,
                     (int) ExSMTag_GetTagWithoutQualifier(target.tag),
                     (char) ExSMTag_GetQualifierDisplay(target.tag),
                     chunkAddr, (int) chunkSize);

          exsm_assert_rc(rc, "ExSM_Post");
          
          // Send an ACK. The target is the sender's target with the
          // reply bit flipped.
          target = chunk.tgt;
          target.tag = ExSMTag_ToggleReplyFlag(chunk.tgt.tag);

          ExSMShortMessage m;
          m.setTarget(target);
          m.setNumValues(1);
          m.setValue(0, (int32_t) ExSMShortMessage::ACK);
          int32_t rc2 =  m.send();
          if (rc2 == SM_ERR_NOSERVICE ||
              rc2 == SM_ERR_NOPEER ||
              rc2 == SM_ERR_NODE_DOWN)
          {
            glob->handleReaderThreadError(rc2, "ExSM_SendShortMessage", dataHandle, smTaskList);
            return 0;
          }

        }
        
        // ACK
        else if (shortMsgType == ExSMShortMessage::ACK)
        {
          task->sendChunk_SetAckArrived(true);
          EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
                     "%s   ACK arrived, task %p", readerPrefix, task);
        }

        // UNKNOWN SHORT MESSAGE TYPE
        else
        {
          char buf[64];
          snprintf(buf, sizeof(buf), "Invalid short message type %d",
                   (int) shortMsgType);
          exsm_assert(FALSE, buf);
        }

        // We are done processing short messages. Return to the top of
        // the loop that processes each chunk.
        continue;
        
      } // if (short message)

      // Now we process prepost messages. Cases to consider:
      // (a) This is a chunk, but not the final chunk
      // (b) This is the final chunk
      // (c) This message is not chunked, the complete message arrived

      // In cases (b) and (c) we need to insert the receive buffer
      // into the task output queue. The local variable
      // bufferForOutputQueue will point to the buffer that gets
      // inserted.
      void *bufferForOutputQueue = NULL;

      // Before processing the data buffer or modifying any variables
      // in the task or its queues, first make sure the address
      // returned from SM matches one of the following:
      // 
      // * For a SQL chunk: the task's most recent prepost address
      // * For a complete message: first element of the task input
      //   queue
      //
      // An assertion will fail if there is not a match.

      ExSMQueue *outQ = task->getOutQueue();
      ExSMQueue *inQ = task->getInQueue();
      void *expectedAddr = NULL;
      void *actualAddr = chunk.buff;

      // This block determines the correct value of expectedAddr. The
      // comparison of expectedAddr versus actualAddr happens right
      // after this block.
      if (receivedSQLBufferFragment)
      {
        expectedAddr = task->recvChunk_GetPrepostAddr();
      }
      else
      {
        exsm_assert(!inQ->isEmpty(), "SM task input queue is empty");
        ExSMQueue::Entry &entry = inQ->getHeadEntry();
        void *dataPointer = entry.getData();
        exsm_assert(dataPointer,
                    "Task queue entry contains a NULL buffer pointer");
        expectedAddr = ExSM_GetMessageBufferPrepostAddr(dataPointer);

        // We can also set bufferForOutputQueue when the arrival is a
        // complete message
        bufferForOutputQueue = dataPointer;
      }

      // Make sure expectedAddr matches actualAddr
      EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
                 "%s   prepost %p, arrival %p", readerPrefix,
                 expectedAddr, actualAddr);
      exsm_assert(expectedAddr == actualAddr,
                  "Expected address does not match actual");
      
      // Now we trust the chunk.buff address and can process data

      if (receivedSQLBufferFragment)
      {
        // Cases (a) and (b)

        // Tell the task that new bytes have arrived
        task->recvChunk_Receive(chunk.size);

        // See if this is the final chunk or not and process
        // accordingly
        if (task->recvChunk_MoreExpected())
        {
          // Case (a): Not the final chunk

          // Post the next chunk
          sm_target_t target = chunk.tgt;
          ExSMTag_SetSQLChunkFlag(&target.tag);

          uint32_t chunkSize = (uint32_t) task->recvChunk_GetChunkSize();
          char *chunkAddr = (char *) task->recvChunk_GetPrepostAddr();
          
          int32_t rc = ExSM_Post(target, chunkSize, chunkAddr, task, 0);
          
	  if (rc == SM_ERR_NOSERVICE || 
	      rc == SM_ERR_NOPEER ||  
	      rc == SM_ERR_NODE_DOWN)
          {
            glob->handleReaderThreadError(rc, "ExSM_Post", dataHandle, smTaskList);
            return 0;
          }

          exsm_assert_rc(rc, "ExSM_Post");

          EXSM_TRACE(EXSM_TRACE_BUFFER|EXSM_TRACE_SM_CALLS,
                     "PREPOST %d:%d:%" PRId64 ":%d:0x%c %p",
                     (int) target.node, (int) target.pid, target.id,
                     (int) ExSMTag_GetTagWithoutQualifier(target.tag),
                     (char) ExSMTag_GetQualifierDisplay(target.tag),
                     chunkAddr, (int) chunkSize);

          // Send an ACK. The target is the sender's target with the
          // reply bit flipped and the chunk flag cleared.
          target = chunk.tgt;
          target.tag = ExSMTag_ToggleReplyFlag(chunk.tgt.tag);
          ExSMTag_ClearSQLChunkFlag(&target.tag);

          ExSMShortMessage m;
          m.setTarget(target);
          m.setNumValues(1);
          m.setValue(0, (int32_t) ExSMShortMessage::ACK);
          int32_t rc2 = m.send();
          if (rc2 == SM_ERR_NOSERVICE ||
              rc2 == SM_ERR_NOPEER ||
              rc2 == SM_ERR_NODE_DOWN)
          {
            glob->handleReaderThreadError(rc2, "ExSM_SendShortMessage", dataHandle, smTaskList);
            return 0;
          }
        }
        else
        {
          // Case (b): Final chunk

          // Store a pointer to the receive buffer. It will be used
          // later for an insert into the task output queue.
          bufferForOutputQueue = task->recvChunk_GetBuffer();
          exsm_assert(bufferForOutputQueue,
                      "Task returned a NULL buffer pointer");

          // Adjust the length of the receive buffer
          ExSM_SetMessageLength(bufferForOutputQueue, chunk.size);
          
          // The task can now release its pointer to the receive
          // buffer. The buffer will not be forgotten because it gets
          // added to the task output queue a few lines below.
          task->recvChunk_Exit();
        }

      } // received a chunk

      else
      {
        // Case (c): A complete message

        // Make sure bufferForOutputQueue was initialized
        exsm_assert(bufferForOutputQueue,
                    "bufferForOutputQueue was not initialized");

        // Set the message length in the preposted receive buffer
        ExSM_SetMessageLength(bufferForOutputQueue, chunk.size);
        
        // Remove the input queue element
        inQ->removeHead();
      
      } // received a complete message (not a chunk)

      // If bufferForOutputQueue is not NULL, this is either a
      // complete message or the final chunk. We need to place the
      // receive buffer on the task output queue. We also add the task
      // to the SM ready list if the task output queue was initially
      // empty.

      if (bufferForOutputQueue)
      {
        exsm_assert(!outQ->isFull(), "SM task output queue is full");
        bool outQueueWasEmpty = outQ->isEmpty();

        ExSMQueue::Entry &outEntry = outQ->getTailEntry();
        outEntry.setData(bufferForOutputQueue);
        outQ->insert();
        
        if (outQueueWasEmpty)
        {
          // NOTE: The SM ready list is accessed by both threads (main
          // and reader) but does not have its own lock. By
          // convention, modifications to the ready list are always
          // performed while holding a lock on the SM task list (a
          // global collection of all SM tasks in this process).
          readyList->add(task);
        }

        // Schedule the recipient of this message (the recipient is
        // currently a connection object whose wait() method will
        // execute once the main thread receives the LRABBIT event)
        int32_t *scheduledAddr = task->getScheduledAddr();
        exsm_assert(scheduledAddr,
                    "SM task scheduledAddr pointer is NULL");
        *scheduledAddr = 1;
        EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
                   "%s   scheduled task %p", readerPrefix, task);

      } // if (bufferForOutputQueue)
    } // for each chunk
    
    // Unlock the task list
    smTaskList->unlock();
    EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
               "%s   unlocked task list", readerPrefix);

    // Release SeaMonster buffer space
    rc = SM_get_done(0, dataHandle);

    // Add a WAITDONE event to the in-memory trace
    ExSMEvent::add(ExSMEvent::ReceiveDone);

    EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT, 
               "%s  SM_get_done rc %d", readerPrefix, (int) rc);

    // Generate a seabed event to wake the main thread. There is no
    // need to wake the main thread if we already received the
    // SHUTDOWN message.
    if (glob->getReaderThreadState() != ExSMGlobals::PROCESSING_SHUTDOWN)
    {
      XAWAKE(EXSM_MAIN_THREAD_PIN, LRABBIT);
      EXSM_TRACE(EXSM_TRACE_RDR_THR|EXSM_TRACE_WAIT,
                 "%s  generated LRABBIT", readerPrefix);
    }

  } // while (glob->getReaderThreadState() == ExSMGlobals::STARTED)

  // At this point the reader thread loop has exited and the thread is
  // going to exit.
  // 
  // Reader thread state is set to DONE so the main thread knows the
  // thread is no longer active.
  //
  // We also signal the main thread via a condition variable so the
  // main thread can wake up in case it was waiting for the reader
  // thread to complete (this is the case where the main thread sends
  // the reader thread a SHUTDOWN message).
  
  EXSM_TRACE(EXSM_TRACE_RDR_THR, "%s Reader thread shutting down",
             readerPrefix);
  EXSM_TRACE(EXSM_TRACE_RDR_THR, "%s Reader thread state %s", readerPrefix,
             glob->getThreadStateString(glob->getReaderThreadState()));

  // Change the reader thread state to DONE. This will signal the main
  // thread which could be waiting for the reader thread to finish its
  // shutdown processing.
  glob->setReaderThreadState(ExSMGlobals::DONE,
                             true,  // do locking
                             true); // signal

  EXSM_TRACE(EXSM_TRACE_RDR_THR,"%s END READER", readerPrefix);
  return 0;
}
