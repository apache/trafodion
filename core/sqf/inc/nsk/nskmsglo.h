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
#if _MSC_VER >= 1100
#pragma once
#endif

#ifndef _NSK_msglo_
#define _NSK_msglo_

// mqc type literals

#define mqcstate_nullidle     0x0
#define mqcstate_omsg         0x1
#define mqcstate_imsg         0x2
#define mqcstate_opio         0x3
#define mqcstate_glup         0x4
#define mqcstate_whole        0x5
#define mqcstate_oabandon     0x6
#define mqcstate_status       0x7
#define mqcstate_odope        0x8
#define mqcstate_idope        0x9
#define mqcstate_iabandon     0xa
#define mqcstate_odone        0xb
#define mqcstate_dopereply    0xc
#define mqcstate_glupdone     0xd


// message command codes.  These are contained in the mqc
// image received by a destination pe.

#define HMSG         0     // incoming message
#define HDATA        1     // here's data ( response to sdata )
#define HCTRL        2     // here's control (response to sctrl )
#define BABANDON     3     // abandon a message
#define FABANDON     4     // abandon complete (response to babandon)
#define HREPLY       5     // here's reply
#define SDATA        6     // send data
#define SCTRL        7     // send control
#define NPIO         8     // normal pio - completed with fpio
#define OPIO         9     // orphan pio - send and forget
#define NGLUP       10     // normal glup - completed with fglup
#define OGLUP       11     // orphan glup - sent on locker resend
#define FPIO        12     // pio complete
#define FGLUP       13     // glup complete
#define HMSG_NACK   14     // target is dead.
#define MESSAGE_INVALID 255 // Bad news to see this

// pio request literals

#define PIO_GREETING             1
#define PIO_GET_RELOADLOCK       2
#define PIO_RELEASE_RELOADLOCK   3
#define PIO_SENDWHOLE            4
#define PIO_SENDVERIFIER         5
#define PIO_IAMALIVE             6
#define PIO_SENDPORTNAMECOUNTER  7


// glup request literals

#define GLUP_PORT_ADD            32  // adds a port
#define GLUP_PORT_ACTION         33  // changes or deletes a port
#define GLUP_RELOAD              34  // merges two clusters
#define GLUP_GETVERIFIERS        35  // allocates a block of verifiers
#define GLUP_RELOAD_END          36  // reload end glup after TMF init.
#define GLUP_NAMECOUNTER_INCREMENT  37  // glup to avoid name collision.
#define GLUP_DCT_CLEANUP         38  // clean DCT entries of certain nodes
#define GLUP_NODE_DOWN           39  // propogate node down events to clients
#define GLUP_CONNECTED           40  // test if cluster can talk to a new node

#define MAX_PIODATASIZE     0x4000

#define MAX_GLUPDATASIZE 1024

// there literals define the state of the reloadee as he is coming up
#define STATE_RELOADEE             0
#define STATE_RELOAD_IN_PROGRESS   1
#define STATE_RELOAD_COMPLETED     2

// winsock stuff

// version = 2.0

#define socketversion MAKEWORD( 2, 0 )

// socket states

#define socket_connected   0
#define socket_none        1
#define socket_waiting     2
#define socket_letHimGoFirst 3      // used for Lti connections

// pe connection states

#define connect_up              0
#define connect_known           1
#define connect_down            2
#define connect_unconfigured    7

// pio sent to other pe at discovery time

typedef struct _NSK_PIO_xx{


} NSK_PIO_xx, *PNSK_PIO_xx;





#ifndef NSKMsglo_C

// forward references

typedef class  _NSK_PCB *PNSK_PCB;
typedef class  _NSK_QCB *PNSK_QCB;
typedef class  _NSK_NRL *PNSK_NRL;
typedef class  _NSK_MQC *PNSK_MQC;
typedef class  _NSK_BSD *PNSK_BSD;
typedef struct _NSK_SG  *PNSK_SG;
typedef struct _NSKT_OVERLAPPED *PNSKT_OVERLAPPED;
typedef struct NSKT_IOMSG_t     *PNSKT_IOMSG;
typedef class  _NSK_SENDINFO *PNSK_SENDINFO;
typedef class _NSK_CACHE *PNSK_CACHE;
typedef class _NSK_CACHE_CTL *PNSK_CACHE_CTL;



DllImport
PNSK_CACHE AllocateCacheEntry( PNSK_CACHE_CTL pctl );

DllImport
void FreeCacheEntry (PNSK_MQC pmqc );

DllImport
VOID CopyUserBuf( PNSK_MQC    pmqc,
                  PVOID       source_buf,
                  PVOID       target_buf,
                  INT         buf_size );

DllImport
VOID NSKMQCAbandonImsg( PNSK_MQC pmqc,
                        PNSK_QCB pqcb );

DllExport
VOID NSKMQCFakeAbandonImsg( PNSK_MQC pmqc, 
						    PNSK_QCB pqcb );

DllImport
VOID NSKSetupSend( PNSK_SENDINFO psendinfo,
                   PVOID      psendbuf,
                   PDWORD     bufcount );

DllImport
VOID NSKTSetupSend( PNSK_SENDINFO psendinfo,
                    PVOID       psendbuf
                  );
DllImport
VOID NSKSendCompletion( PNSK_SG        pnsk,
                        PNSK_BSD       pbsd,
                        PNSKT_OVERLAPPED  poverlapped );

DllImport
LONG NSKSend(  PNSK_SG           pnsk,
               PNSK_PCB          ppcb,
               PNSK_MQC          pmqc );


DllImport
VOID NSKProcessPIO( PNSK_SG  pnsk,
                    PNSK_MQC pmqc,
                    DWORD    sendingpe,
                    PVOID    precvbuf );

DllImport
LONG NSKSendPIO(
                      USHORT     requestcode,
                      PCHAR      pdata,
                      DWORD      length,
                      DWORD      destpe,
                      PDWORD     p0,
                      PDWORD     p1,
                      PDWORD     p2,
                      PNSK_QCB   pqcb,
                      BOOL       orphan );

DllImport
VOID NSKProcessGLUP( PNSK_SG  pnsk,
                     PNSK_MQC pmqc,
                     DWORD    sendingpe,
                     PVOID    precvbuf );

DllImport
LONG NSKSendGLUP(
                      USHORT     requestcode,
                      DWORD      glupseq,
                      PCHAR      pdata,
                      DWORD      length,
                      DWORD      p0,
                      DWORD      p1,
                      DWORD      p2,
                      HANDLE     wakeup,
                      DWORD      lockerpe,
                      DWORD      cpumask,
                      BOOL       orphan );

DllImport
VOID NSKsetup_error_reply ( PNSK_MQC   pmqc,    // outgoing MQC to reply to
                            DWORD      error,   // error code of the reply
                            DWORD      errclass,// additional error classification
                                                // bits to set (currently this is
                                                // always zero)
                            BOOL       iamservice  // True if called in context of
                                                // a thread in the service process
                          );

DllExport
VOID NSKdope_reply ( PNSK_MQC       pmqc,
                     DWORD			exit_code,
                     PNSK_SG        pnsk);

DllImport
BOOL NSKContinueGLUPBroadcast( PNSK_SG  pnsk,
                               PNSK_PCB ppcb,
                               PNSK_MQC pmqc );

DllImport
void NSKGLUPResend( void );

DllImport
VOID NSKMqcInit (PNSK_MQC pmqc);


#endif

#endif
