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
/*++

Module name:

NSKprocess.h

Abstract:

     This module contains the prototypes and other declarations and
     defines needed to specify the process creation API.
Revision History:

--*/
#ifndef _NSK_process_
#define _NSK_process_

#include "nsk/nskcommon.h"
#include "nsk/nskport.h"


// This procedure is used for creating NT processes with special initialization
// to make them run on top of the NSK-NT layer. Most parameters are paraphrased
// below. There are 2 option-flag parameters. 'Nmopt' can have the value NAMED_AUTO,
// NAMED_MANUAL, NAMED_NOTHING or NAMED_BACKUP as possible values. In the case of
// NAMED_MANUAL or NAMED_BACKUP the pr_name parameter should be a non-null name
// string. The other option parameter is create_options. It can have the values
// USE_CALLER_ENV, USE_ARG_ENV or USE_BOTH_ENV. In the case of the latter two, the
// 'env_block' parameter points to a valid environment block. Please see the
// NSK-NT process creation ES for details.

DllImport
LONG NSKProcessCreate (
                  PCHAR cmdline,//object file followed by <optional> command line args
                  DWORD priority,   // NT priority class
                  SHORT pe,     // system number within cluster to create on
                  PNSK_PORT_HANDLE lphDestPort, // handle to the new port
                  SHORT portclass,    // if you don't care supply -1
                  SHORT portsubclass,  // if you don't care supply -1
                  DWORD nmopt,  // look at comments above
                  PCHAR pr_name,  // name to be given to the newly created port
                  PCHAR process_descr, // ptr to a process descriptor string
                  DWORD nowait_tag, // returned in nowait creation
                  DWORD create_options, // look at comments above
                  LPVOID env_block,  // ptr to env block supplied or NULL
                  DWORD env_block_size, // size of the supplied env_block
                  DWORD pfssize = 0,
                  PNSK_PORT_HANDLE lphMyPort = NULL// caller's port for getting notified
                                            // at the conclusion of process create
                 );




DllImport
void  NSKProcessInitializer(DWORD* pindex);

DllImport
void  NSKProcessStopper (PVOID);

DllImport
BOOL  NSKIsProcessAlive (PNSK_PCB pTargetPcb);
 
// NSKGetSecurityBlk returns NULL if the current PCB is not
// initialized
DllImport
PVOID  NSKGetPCBSecurityBlk ();


#define USE_CALLER_ENV  0
#define USE_ARG_ENV     1
#define USE_BOTH_ENV    2
#define CALLER_ENV_TMF  3      // Reserved specially for TMFINIT process.
#define STARTUP_USE_CALLER_ENV  4   // Same as USE_CALLER_ENV, but usable
                                    // before the CPU up message is sent.
                                    // For NSK-Lite internal use only.

#define NAMED_NOTHING   0
#define NAMED_AUTO      1
#define NAMED_MANUAL    2
#define NAMED_BACKUP    3



// The following struct is used to pass to the remote thread created
// in NSKIsProcessAlive() on service's context. The purpose is to determine
// the live or dead status of the ppcb in this struct

typedef struct NSK_PROCESS_INFO_
{
    PNSK_PCB ppcb;      // the poiner to the process we are interedted in
    BOOL     isAlive;   // the answer return from remote thread
} NSK_PROCESS_INFO, *PNSK_PROCESS_INFO;


//
// The following defines the structure of Resource type "TANDEM-NSK", id "#1".
//
#define NSK_ATTRS_VERSION_MAY97 1
#define NSK_ATTRS_VERSION_JUL97 2
#define NSK_ATTRS_CURRENT_VERSION NSK_ATTRS_VERSION_JUL97

#define SHADOWDEATH_IGNORE 1
#define SHADOWDEATH_FATAL  2

#define SHADOWTYPE_FS      0
#define SHADOWTYPE_SQLC    1
#define SHADOWTYPE_ARKCMP  2
#define SHADOWTYPE_SQLCO   3   // New define for the Cobol compiler

typedef struct NSK_PROCESS_ATTRS_
{
  short        version;
  short        nameOption;
  union
  {
    int        pfssize;
    struct {
      unsigned size:31;
      unsigned nopfs:1;
    };
  };
  short     priv;
  short     shadowDeath;
  short     sql;
  char      portname[ 10 ];
  short     service;
  short     shadowType;
  int       sharedMemSize;
} NSK_PROCESS_ATTRS, *PNSK_PROCESS_ATTRS;
//
// Initialize the passed structure with null values
//
#define INIT_PROCESS_ATTRS_MAY97( attrs ) \
{\
  (attrs)->version     = NSK_ATTRS_CURRENT_VERSION; \
  (attrs)->nameOption  = NAMED_NOTHING; \
  (attrs)->pfssize     = 0; \
  (attrs)->sharedMemSize = 0;\
  (attrs)->priv        = FALSE; \
  (attrs)->shadowDeath = 0; \
  (attrs)->sql         = FALSE; \
  (attrs)->portname[0] = '\000'; \
}

#define INIT_PROCESS_ATTRS( attrs ) \
{\
  (attrs)->version     = NSK_ATTRS_CURRENT_VERSION; \
  (attrs)->nameOption  = NAMED_NOTHING; \
  (attrs)->pfssize     = 0; \
  (attrs)->sharedMemSize = 0;\
  (attrs)->priv        = FALSE; \
  (attrs)->shadowDeath = 0; \
  (attrs)->sql         = FALSE; \
  (attrs)->portname[0] = '\000'; \
  (attrs)->service     = FALSE; \
  (attrs)->shadowType  = 0; \
}

DllExport
void NSKInitProcessAttrs( PNSK_PROCESS_ATTRS attrs , int version );

DllExport
int NSKGetProcessAttrs( int version, PNSK_PROCESS_ATTRS attrs );

#endif
