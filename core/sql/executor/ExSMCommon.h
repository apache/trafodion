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
#ifndef EXSM_COMMON_H
#define EXSM_COMMON_H

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include "Platform.h"
#include "sm.h"
#include "NAAssert.h"

// Forward declarations
class ExSMGlobals;
class NAMemory;
class ExSMTask;
class ExExeStmtGlobals;

// This "main thread PIN" identifies the SQL main thread when the SM
// reader thread raises an LRABBIT event. The main thread associates
// itself with this PIN when SM is initialized (see
// ExSMGlobals::InitSMGlobals()) and the reader thread uses this PIN
// in calls to XAWAKE.
const int EXSM_MAIN_THREAD_PIN = 1;

//-------------------------------------------------------------------------
// Report the largest chunk size we can transmit using SeaMonster. We
// call SM to retrieve the absolute maximum and then use 50% of that
// value as our effective maximum.
//-------------------------------------------------------------------------
uint32_t ExSM_GetMaxChunkSize(NABoolean intranode = FALSE);

//-------------------------------------------------------------------------
// Map between virtual and real SQ node numbers. Note that we need to
// be aware if the size of the node number id changes
//-------------------------------------------------------------------------
int32_t ExSM_GetNodeID(int32_t nodeNum);

//-------------------------------------------------------------------------
// Compare two SM targets
//-------------------------------------------------------------------------
bool ExSM_TargetsEqual(const sm_target_t &a, const sm_target_t &b);

//-------------------------------------------------------------------------
// Assert macros for SeaMonster reader thread
//-------------------------------------------------------------------------
#define exsm_assert(expr, msg)                                       \
  { if (!(expr)) assert_botch_abend(__FILE__, __LINE__, msg, "" # expr ""); }

extern __thread char ExSM_AssertBuf[];

#define exsm_assert_rc(rc, func) {                                 \
  if (rc != 0) {                                                     \
    sprintf(ExSM_AssertBuf, "rc = %d", (int) rc);                  \
    assert_botch_abend(__FILE__, __LINE__, func, ExSM_AssertBuf);  \
  }                                                                  \
}

//-------------------------------------------------------------------------
// SeaMonster message types used in the executor
//-------------------------------------------------------------------------
enum ExSM_MessageType
  {
    // Data reqeust
    EXSM_MSG_REQUEST    = 1,

    // Data reply
    EXSM_MSG_REPLY      = 2, 

    // Short message
    EXSM_MSG_SHORT      = 3
  };

//-------------------------------------------------------------------------
// SM initialize
//-------------------------------------------------------------------------
int32_t ExSM_Initialize(ExSMGlobals *smGlobals,
                        ExExeStmtGlobals *stmtGlobals);

//-------------------------------------------------------------------------
// SM finalize
//-------------------------------------------------------------------------
int32_t ExSM_Finalize(ExSMGlobals *smGlobals);

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
                  int maxRetries,              // IN (-1 = retry forever)
                  int64_t sendCount,           // IN (for tracing only)
                  const void *startBuf);

//-------------------------------------------------------------------------
// Post a SeaMonster receive buffer
//-------------------------------------------------------------------------
int32_t ExSM_Post(const sm_target_t &target,  // IN
                  size_t dataSize,            // IN
                  const void *data,           // IN
                  ExSMTask *smTask,           // IN
                  NABoolean isServer);        // IN (for tracing only)

//-------------------------------------------------------------------------
// Send a SeaMonster short message
//-------------------------------------------------------------------------
int32_t ExSM_SendShortMessage(ExSMGlobals *smGlobals,
                              const sm_target_t &target,
                              const void *data,
                              size_t bytesToSend);

//-------------------------------------------------------------------------
// Wrapper function used by the reader thread to set the length of an
// IPC message buffer. Eliminates the need for the reader thread
// function to include or be aware of our IPC classes.
//-------------------------------------------------------------------------
void ExSM_SetMessageLength(void *buf, size_t len);

//-------------------------------------------------------------------------
// Wrapper function used by the reader thread to allocate an
// IpcMessageBuffer on the C++ heap. The size of the data region in the
// new buffer will be dataBytes. Eliminates the need for the reader
// thread function to include or be aware of our IPC classes.
//-------------------------------------------------------------------------
void *ExSM_AllocateMessageBuffer(size_t dataBytes,
                                 NAMemory *threadSafeHeap);

//-------------------------------------------------------------------------
// Return the SeaMonster prepost address of a message buffer. The
// prepost address is the first byte of data that gets transmitted. The
// IpcMessageBuffer header is not transmitted.
//-------------------------------------------------------------------------
void *ExSM_GetMessageBufferPrepostAddr(void *data);

//-------------------------------------------------------------------------
// Return the address of a message buffer given the SeaMonster prepost
// address. The prepost address immediately follows an IpcMessageBuffer
// instance.
//-------------------------------------------------------------------------
void *ExSM_GetMessageBufferAddr(void *prepostAddr);

//-------------------------------------------------------------------------
// Return the IPC message object type of the first object that follows
// the header (the header is an InternalMsgHdrInfoStruct) in the
// IpcMessageBuffer pointed to by the data argument.
//
// If the object type cannot be determined "UNKNOWN" is returned.
//-------------------------------------------------------------------------
const char *ExSM_GetMessageBufferType(void *data, size_t dataBytes);

//-------------------------------------------------------------------------
// Function to return the threadId of the calling thread
//-------------------------------------------------------------------------
int ExSM_GetThreadID();

//-------------------------------------------------------------------------
// Functions and values to manipulate and test SeaMonster tag qualifiers
//
// Each SM message travels with:
// * The ID associated with the query
// * The tag associated with the TCB
// * Zero or more tag qualifier bits
//
// Tag qualifiers occupy the four high-order bits of the tag. We
// currently have the following qualifiers:
//
//   REPLY - Prepost message or short message sent from server to
//           client
//
//   SQL_CHUNK - Prepost message sent using the SQL chunking protocol
//
// Note:
//
//   Chunk protocol is sometimes used for messages that travel as a
//   single chunk. This happens when message size exceeds the max
//   expected size on the receiving end but does not exceed the
//   SeaMonster max buffer size. Such messages will have the CHUNK
//   qualifier set.
//
//   The SHUTDOWN message is a special short message. It travels with
//   a SeaMonster ID of 0, tag of 0, and no tag qualifiers.
//
//-------------------------------------------------------------------------

// The tag qualifier bits
const int32_t EXSM_TAG_REPLY      = 0x10000000;
const int32_t EXSM_TAG_SQL_CHUNK  = 0x20000000;

// A filter for masking out the qualifier bits from a 32-bit tag
const int32_t EXSM_TAG_FILTER     = 0x0fffffff;

// Get, set and toggle the REPLY bit in the tag. The set function
// modifies the caller's tag. The toggle function returns a new tag
// and does not modify the caller's tag.
inline bool ExSMTag_GetReplyFlag(int t)   { return t & EXSM_TAG_REPLY; }
inline void ExSMTag_SetReplyFlag(int *t)  { *t |= EXSM_TAG_REPLY; }
inline int ExSMTag_ToggleReplyFlag(int t) { return t ^ EXSM_TAG_REPLY; }

// Get, set and toggle the SQL CHUNK bit in the tag. The set function
// modifies the caller's tag. The clear function also modifies the
// caller's tag.
inline bool ExSMTag_GetSQLChunkFlag(int t)    { return t & EXSM_TAG_SQL_CHUNK; }
inline void ExSMTag_SetSQLChunkFlag(int *t)   { *t |= EXSM_TAG_SQL_CHUNK; }
inline void ExSMTag_ClearSQLChunkFlag(int *t) { *t &= ~EXSM_TAG_SQL_CHUNK; }

// Return the tag value with all qualifier bits cleared
inline int ExSMTag_GetTagWithoutQualifier(int t) { return t & EXSM_TAG_FILTER; }

// Return a single hex character representing the 4-bit tag qualifier
inline char ExSMTag_GetQualifierDisplay(int t)
{
  char qualifierDisplay[17] = "0123456789abcdef";
  return qualifierDisplay[((unsigned int) t) >> 28];
}

//-------------------------------------------------------------------------
// We maintain a global list of completed send buffers. The following
// functions are used to add and remove elements. For more detail see
// the implementation of these functions in ExSMCommon.cpp
//-------------------------------------------------------------------------
void ExSM_AddCompletedSendBuffer(void *buf);
bool ExSM_RemoveCompletedSendBuffer(void *&buf);

//-------------------------------------------------------------------------
// We maintain a global collection of active IDs and functions to
// register, cancel, and find an ID. Access to the collection is
// protected by a mutex. The register function is a no-op if the ID
// is already in the collection, and the cancel function is a no-op
// if the ID is not in the collection.
//-------------------------------------------------------------------------
int32_t ExSM_Register(const int64_t &id);
int32_t ExSM_Cancel(const int64_t &id);
bool ExSM_FindID(const int64_t &id, bool lock = true);

#endif // EXSM_COMMON_H
