///////////////////////////////////////////////////////////////////////////////
//
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
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LOCALIO_H
#define LOCALIO_H
#ifndef NAMESERVER_PROCESS

#include <iostream>
#include <string>
#include <map>

using namespace std;

#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/un.h>

#include "msgdef.h"

#define SQ_LIO_MAX_BUFFERS      1000 // shared buffers per node in real cluster,
                                     // SQ_LIO_MAX_BUFFERS/4 in virtual cluster
#define SQ_LIO_SHM_PERMISSIONS  0660 // shared memory permissions
#define SQ_LIO_MSQ_PERMISSIONS  0660 // message queue permissions

#define SQ_LIO_NORMAL_MSG          1 // normal messages
#define SQ_LIO_SIGNAL_TIMEOUT      (5000000) // 5 milliseconds (in nano seconds)
#define SQ_LIO_MONITOR_ACQUIRE_MAX (sharedBuffersMax -(sharedBuffersMax/4))
                                     // limit monitor's usage to 75% of max buffers

#define SQ_LIO_SIGNAL_REQUEST_REPLY (SIGRTMAX - 4)

typedef enum
{
  MC_ReadySend = 1,
  MC_NoticeReady,
  MC_SReady,
  MC_AttachStartup,
  MC_NoticeClear
} MonitorCtlType;

typedef struct
{
  bool               received;
  bool               attaching;
  int                OSPid;     // Buffer owning pid (owner process)
  Verifier_t         verifier;  // pid verifier (owner process)
  int                index;     // Shared buffer relative index location 
                                // in SQ_LIO_MAX_BUFFERS array
  struct timespec    timestamp; // Last time buffer actively used
  int                bufInUse;
} SharedMsgTrailer;

typedef struct LioSharedMsg
{
  struct message_def msg;
  SharedMsgTrailer   trailer;
} SharedMsgDef;

// Structure used in message queue operations: msgsnd, msgrcv
typedef struct
{
  long mtype;
  int  index;
} ClientBufferInfo;

typedef struct LioSharedMemHdr
{
  int  mPid;
  int  wdtEnabler;
  int  lastMonRefresh; 
} SharedMemHdr;

typedef int SB_Verif_Type;
#endif
#endif
