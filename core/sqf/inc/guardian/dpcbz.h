//
// PREPROC: start of section: 
#if (defined(dpcbz_h_) || (!defined(dpcbz_h_including_section) && !defined(dpcbz_h_including_self)))
#undef dpcbz_h_
//

#include "rosetta/rosgen.h" /* rosetta utilities */

#pragma page "T9050 GUARDIAN  PCB Field IDs and Values"
//-----------------------------------------------------------------
//       @@@@@@@@     @@@@@@@@@@    @@@@@@@@@   @@@@@@@@@
//       @@@@@@@@@    @@@@@@@@@@@  @@@@@@@@@@@  @@@@@@@@@@
//       @@@    @@@   @@@     @@@  @@@       @  @@@     @@@
//       @@@     @@@  @@@     @@@  @@@          @@@     @@
//       @@@     @@@  @@@@@@@@@@@  @@@          @@@@@@@@@
//       @@@     @@@  @@@@@@@@@@   @@@          @@@@@@@@@@
//       @@@     @@@  @@@          @@@          @@@     @@@
//       @@@    @@@   @@@          @@@       @  @@@     @@@
//       @@@@@@@@@    @@@          @@@@@@@@@@@  @@@@@@@@@@@
//       @@@@@@@@     @@@           @@@@@@@@@   @@@@@@@@@@
//
//  T9050 GUARDIAN   Operating System - PCB Field IDs and Values
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
//-----------------------------------------------------------------

#endif
// PREPROC: end of section: 
//
// #pragma section PCB_FIELD_ID
//
// PREPROC: start of section: pcb_field_id
#if (defined(dpcbz_h_pcb_field_id) || (!defined(dpcbz_h_including_section) && !defined(dpcbz_h_including_self)))
#undef dpcbz_h_pcb_field_id
//

#pragma page "T9050 GUARDIAN  PCB FIELD IDs"
// Literals for requesting PCB fields.

enum {PCB_ID_AWAKE = 1};
enum {PCB_ID_BYPASSSFG = 7};
enum {PCB_ID_CODECOUNT = 2};
enum {PCB_ID_CODESEG = 3};
enum {PCB_ID_CRAID = 4};
enum {PCB_ID_CURPRI = 5};
enum {PCB_ID_CURRENTUC = 6};   // not valid on RISC       DFH
enum {PCB_ID_GROUPNEXT = 9};
enum {PCB_ID_HALT_ON_STOP = 19};
enum {PCB_ID_HEADPIN = 10};
enum {PCB_ID_INITPRI = 11};
enum {PCB_ID_IOMON = 12};
enum {PCB_ID_LCBUR = 13};      // actually, MQC receives
enum {PCB_ID_LCBUS = 14};      // actually, MQC sends
enum {PCB_ID_LIBCOUNT = 15};
enum {PCB_ID_LIBSEG = 16};
enum {PCB_ID_NOMONOPEN = 17};
enum {PCB_ID_PFSSEG = 20};
enum {PCB_ID_PHOENIXPROC = 21};
enum {PCB_ID_PRI = 22};
enum {PCB_ID_PRIORITYQ = 23};
enum {PCB_ID_PRIVCODEF = 24};
enum {PCB_ID_PRIVLIBF = 25};
enum {PCB_ID_PROCAID = 26};
enum {PCB_ID_PSTATE = 27};
enum {PCB_ID_QSTOPF = 28};
enum {PCB_ID_REMID = 29};
enum {PCB_ID_SIMON_AFTER = 38};
enum {PCB_ID_SIMON_BEFORE = 39};
enum {PCB_ID_SQLLICENSED = 31};
enum {PCB_ID_STACKSEG = 32};
enum {PCB_ID_STOPF = 33};
enum {PCB_ID_SYSPROC = 34};
enum {PCB_ID_WAIT = 35};
enum {PCB_ID_TSNLOGON = 36};
enum {PCB_ID_NCODECOUNT = 40}; // NATIVE CODE COUNT
enum {PCB_ID_NCODEFLAG = 43};
enum {PCB_ID_RCODECOUNT = 44}; // OCT CODE COUNT
enum {PCB_ID_RLIBCOUNT = 45};  // OCT LIB COUNT
enum {PCB_ID_COVERINDEX = 46};
enum {PCB_ID_COVERINDEX_UC = 47};
enum {PCB_ID_COVERINDEX_UL = 48};
enum {PCB_ID_XCURPAGES = 8};
enum {PCB_ID_XIOPROC = 18};
enum {PCB_ID_NOSTOPPROC = 37};
enum {PCB_ID_FRAMESLOCKED = 50};
enum {PCB_ID_UCLFRAMES = 51};
enum {PCB_ID_UCLLOCKED = 52};
enum {PCB_ID_DEFFILESEC = 53};
enum {PCB_ID_GLOBALSEG = 54};
enum {PCB_ID_PFAULTNOTALLOWED = 55};
     // highest currently used literal is 55  (30,41,42 AND 49 ARE FREE)

enum {PCB_ID32_CURSEG = 106};
enum {PCB_ID32_MEM = 100};
enum {PCB_ID32_MQ = 101};
enum {PCB_ID32_PDST = 102};
enum {PCB_ID32_RECVHEAD = 103};
enum {PCB_ID32_SEG = 107};
enum {PCB_ID32_SIMON_FLAGS = 115};
enum {PCB_ID32_TO = 104};
enum {PCB_ID32_XPTR = 105};
enum {PCB_ID32_FPST = 108};
enum {PCB_ID32_MAINSTACKSIZE = 109};
enum {PCB_ID32_PRIVSTACKSIZE = 110};
enum {PCB_ID32_FSGPVAL = 111};
enum {PCB_ID32_PSBADDR = 112};
enum {PCB_ID32_GLOBALSEGSIZE = 113};
enum {PCB_ID32_MEASCLIENTCBADDR = 114};
     // highest currently used literal  is 115

enum {PCB_IDF_CRT = 200};

enum {PCB_IDS_GMOMS_PHANDLE = 301};
enum {PCB_IDS_PHANDLE = 300};
enum {PCB_IDS_MEAS_FENIXPRISTATE = 310};
enum {PCB_IDS_MEAS_PRICAID = 311};
enum {PCB_IDS_MEAS_PRISTATEPFS = 312};
enum {PCB_IDS_MEAS_PROGFILE = 313};
enum {PCB_IDS_MEAS_SEGLCBS = 314};
enum {PCB_IDS_MOMS_PHANDLE = 302};
enum {PCB_IDS_PATHWAYSCNAME = 315}; // highest currently used literal is 315

//
#endif
// PREPROC: end of section: pcb_field_id
//
// #pragma section PCB_MODE_SETTING
//
// PREPROC: start of section: pcb_mode_setting
#if (defined(dpcbz_h_pcb_mode_setting) || (!defined(dpcbz_h_including_section) && !defined(dpcbz_h_including_self)))
#undef dpcbz_h_pcb_mode_setting
//

#pragma page "T9050 GUARDIAN  PCB MODE SETTING"
// Literals for setting PCB mode fields.

enum {PCB_MODE_OFF = 0};
enum {PCB_MODE_ON = 1};

enum {PCB_MODE_FIFO = 0};
enum {PCB_MODE_PRI = 1};

//
#endif
// PREPROC: end of section: pcb_mode_setting
//
// #pragma section PCB_FIELD_VALUES
//
// PREPROC: start of section: pcb_field_values
#if (defined(dpcbz_h_pcb_field_values) || (!defined(dpcbz_h_including_section) && !defined(dpcbz_h_including_self)))
#undef dpcbz_h_pcb_field_values
//

#pragma page "T9050 GUARDIAN  PCB FIELD VALUES"
// Literals for various PCB fields.

// Special Timeout Values for calls to WAIT

enum {WAIT_FOREVER             = -1, // wait up to 248 days for event.
      WAIT_NOWAIT              = -2, // check for event, but do not wait.
      WAIT_RESUME_OLD_TIMEOUT  = 0
     };                              // wait for event or the remaining
                                     // time on the timeout value used
                                     // on the last call to WAIT.

// WAIT and AWAKE values:
// Currently assigned events and corresponding bits.  Lowest bit # is highest
// priority:
//       15: LSEM - extended semaphore
//       14: PWU  - private wakeup
//       12: LRABBIT - Server class event.
//       12: LQIO - queued I/O (for shared memory LAN)
//       10: LCHLD - POSIX process' child exited or suspended.
//        9: LPIPE (LSOCK) - POSIX pipe server/socket event.
//        8: LSIG - POSIX signal wakeup.
//        7: LREQ - requesting a link
//        6: LTMF - TMF request
//        5: LDONE - link done
//        4: LCAN - link cancel
//        3: LINSP - INSPECT event
//        2: INTR - interrupt
//        1: IOPON - I/O power on
//        0: PON - power on

// Please Note:  LSIG is sometimes reset by the signaling code
//               without going thru either Wait or Awake.

enum {LSEM     = 01 ,
      PWU      = 02 ,
      LRABBIT  = 010 ,
      LQIO     = 010 ,
      LCHLD    = 040 ,
      LPIPE    = 0100 ,
      LSIG     = 0200 ,
      LREQ     = 0400 ,
      LTMF     = 01000 ,
      LDONE    = 02000 ,
      LCAN     = 04000 ,
      LINSP    = 010000 ,
      INTR     = 020000 ,
      IOPON    = 040000 ,
      PON      = 0100000
     };

enum {LSOCK = LPIPE};

// PROCESS STATE values: (note pun: PSTATEINSPECT* = PSTATEDEBUG* + 4)
enum {PSTATEUNALLOCATED  = 0,  // PCB IS UNALLOCATED
      PSTATESTARTING     = 1,  // PROCESS IS STARTING
      PSTATERUNNABLE     = 2,  // PROCESS IS READY TO RUN
      PSTATESUSPENDED    = 3,  // PROCESS IS SUSPENDED
      PSTATEDEBUGMAB     = 4,  // PROCESS IS IN MAB DEBUG
      PSTATEDEBUGBRK     = 5,  // PROCESS IS IN BREAKPOINT DEBUG
      PSTATEDEBUGTRAP    = 6,  // PROCESS IS IN TRAP DEBUG
      PSTATEDEBUGREQ     = 7,  // PROCESS IS IN REQUEST DEBUG
      PSTATEINSPECTMAB   = 8,  // PROCESS IS IN MAB INSPECT
      PSTATEINSPECTBRK   = 9,  // PROCESS IS IN BREAKPOINT INSPECT
      PSTATEINSPECTTRAP  = 10, // PROCESS IS IN TRAP INSPECT
      PSTATEINSPECTREQ   = 11, // PROCESS IS IN REQUEST INSPECT
      PSTATESAVEABEND    = 12, // PROCESS IS IN SAVE ABEND MODE
      PSTATETERMINATING  = 13, // PROCESS IS STOPPING
      PSTATEWAITXIOINIT  = 14
     };                        // PROCESS IS awaiting XIO initialization

#define PSTATEDEBUG( pstate )                                                 \
   ((pstate) >= PSTATEDEBUGMAB && (pstate) <= PSTATEDEBUGREQ)
#define PSTATEDEBUGINSPECT( pstate )                                          \
   ((pstate) >= PSTATEDEBUGMAB && (pstate) <= PSTATEINSPECTREQ)
#define PSTATEINSPECT( pstate ) $$$ Define Not Used; No Rosetta Translation $$$
#define PSTATEDEBUGTOINSPECT( x ) $$$ Define Not Used; No Rosetta Translation $$$// maps debug* -> inspect*

// PCB_STOPF values:
enum {PCB_STOPF_BY_ANYONE    = 0, // stoppable by anyone
      PCB_STOPF_NORMAL       = 1, // normally stoppable
      PCB_STOPF_UNSTOPPABLE  = 2
     };                           // unstoppable

// PCB_ONLIST values:                 -- PCB_LIST [0:1] links PCB to ...
enum {PCB_ONLIST_NONE = 0};               // No list
enum {PCB_ONLIST_FREE = 1};               // Thin/Fat free list
enum {PCB_ONLIST_READY = 2};              // Ready list
enum {PCB_ONLIST_SEMAPHORE = 3};          // A priv semaphore's wait list.
enum {PCB_ONLIST_PROCSTOP = 4};           // Process stop list
                                          // This list contains PCBs waiting for
                                          // the monitor to begin stop processing.
enum {PCB_ONLIST_DMONEVENT = 5};          // DMON event list
enum {PCB_ONLIST_XSTOPWAIT = 6};          // Processes waiting to start XSTOP are
                                          // on this list while waiting for an
                                          // available phoenix shadow slot.
enum {PCB_ONLIST_SEMAPHORE2 = 7};         // A binary semaphore's wait list
enum {PCB_ONLIST_OSSSEMN = 8};            // An OSS semaphore's greater wait list
enum {PCB_ONLIST_OSSSEMZ = 9};            // An OSS semaphore's zero wait list
enum {PCB_ONLIST_PRIORITYSEMAPHORE = 10}; // An NPI semaphore's wait list
enum {FLPTUNDERFLOW_TRAP = 0};            // trapout the process
enum {FLPTUNDERFLOW_ZERO = 1};            // replace by zero and continue

enum {STARTDEBUGGER_NONPRIV = 0x1};       // only if process in "safe" place
enum {STARTDEBUGGER_PRIV = 0x3};          // DEBUGNOW or priv user

// PCB_TRAPHANDLER values
enum {TRAPHANDLER_IS_NONE = 0};           // process has no trap handler
enum {TRAPHANDLER_IS_DEBUG = 1};          // process has debug as trap handler
enum {TRAPHANDLER_IS_OWN = 3};            // process has own trap handler


// These values are used to encode various trap handlers that a process
// can have.  They are passed (or returned) in arrays that are passed to
// SETTRAP as parameters.

enum {TRS_FOR_NONE = -1};
enum {TRE_FOR_NONE = 0};
enum {TRP_FOR_NONE = 0};

enum {TRS_FOR_DEBUG = 0};
enum {TRE_FOR_DEBUG = 0};
enum {TRP_FOR_DEBUG = 1};

//
#endif
// PREPROC: end of section: pcb_field_values
//
// #pragma section CPU_PIN_FIELDS
//
// PREPROC: start of section: cpu_pin_fields
#if (defined(dpcbz_h_cpu_pin_fields) || (!defined(dpcbz_h_including_section) && !defined(dpcbz_h_including_self)))
#undef dpcbz_h_cpu_pin_fields
//

#pragma page "T9050 GUARDIAN  CPU PIN FIELDS"

#define PIDCPU 0,7
#define PIDPIN 8,15


#if 0 /* ROSETTA INDEX: PLEASE DO NOT REMOVE */
dpcb.tal
[0.0] Sep 23 1996 17:40:05
[_cls[v96_09_23_17_44]_crl[15]_csn_enn_rsc_ext_faz]
a
Y n 0x0 0x67 0xbfb80f59 0x63df4b9a 0x64d39975 0x20ae7c80 0x5e09917a
0x0 0x0 0x85ed4390 0x8bd624b3 0x6635061c 0x528a2693 0x10177a18
_paT9050 GUARDIAN  PCB Field IDs and Values_
Y n 0x67 0x390 0x7364399a 0xa031f197 0xf9bfa008 0xb7b6e61b 0xebeb1ab4
0x2 0x0 0x9ca817ab 0x8ebcef0b 0x70b34764 0xfba33010 0x1c8a9ef1
_paT9050 GUARDIAN  PCB Field IDs and Values - CHANGE DESCRIPTIONS_
Y n 0x3f7 0x92 0xc90b4be9 0xbf66cfb5 0x3ae02cdd 0xeed5c3c9 0xde5674d5
0x13 0x0 0xa5926d1a 0xe55455c2 0xf5d144b8 0x7797068e 0x3fad4478
_if15_
N n 0x489 0xd 0x0 0x0 0x0 0x0 0x0
0x14 0x0 0x0 0x0 0x0 0x0 0x0
_in15_
N y 0x496 0x189c 0x0 0x0 0x0 0x0 0x0
0x15 0x0 0x0 0x0 0x0 0x0 0x0
_ei15_
Y n 0x1d32 0x1a 0x52ed7664 0x7768c00b 0x60ef6639 0x97058419 0x4dba53c2
0x98 0x0 0x9c86151a 0x4ee6ad8 0x396fb3d 0x88bb1021 0x66043b19
_scPCB_FIELD_ID_
Y n 0x1d4c 0x1e 0xd2315480 0x2e146f96 0x1577c073 0xb0020501 0x37f55f7a
0x99 0x0 0x1a462a46 0xc6378e39 0xe74d2555 0x155e98ae 0x674b1aa2
[_scPCB_FIELD_ID_]_paT9050 GUARDIAN  PCB FIELD IDs_
Y n 0x1d6a 0xac2 0x1fc6b535 0xe57383d0 0xd042a0e5 0x373ab490 0xf11292b3
0x9a 0x0 0xc5966a0 0x98563d8f 0x2b95a8b4 0x15f120e7 0x867b2e92 cglobal.tal 0xd0
_scPCB_MODE_SETTING_
Y n 0x282c 0x22 0x9c5af7e6 0x6381c029 0xd39b495f 0xb1d641c 0xaf742f5a
0xf1 0x0 0x2f89fe98 0x923096f8 0x26d865e7 0xad6437f3 0xf2bbbf5e
[_scPCB_MODE_SETTING_]_paT9050 GUARDIAN  PCB MODE SETTING_
Y n 0x284e 0xca 0xb76ae316 0x9e092a2d 0x397d651d 0xab02bb1d 0xaf771a7f
0xf2 0x0 0xba8bcb77 0x7c56c5e7 0x49c37be9 0xf93befff 0xb2cc3ef0 cglobal.tal 0xd0
_scPCB_FIELD_VALUES_
Y n 0x2918 0x22 0xf25ab808 0xe132c8c4 0x91af50bc 0x4b6e67f1 0xdcef670d
0xfb 0x0 0xd1a80d0 0xd3bb1c32 0x50e69045 0x2a4cb595 0x4d44b48b
[_scPCB_FIELD_VALUES_]_paT9050 GUARDIAN  PCB FIELD VALUES_
Y n 0x293a 0xafb 0x3660b995 0x65ff7398 0xa7eea0c6 0xc4a104a1 0xbe755100
0xfc 0x0 0x7e982911 0x28067496 0x2a1aff50 0xcab41703 0x2195110d cglobal.tal 0xd0
[_scPCB_FIELD_VALUES_]dPSTATEDEBUG
Y n 0x3435 0x70 0xfe2114ee 0xb04cf679 0xa0642666 0xffd6ddb7 0xc2dd8da7
0x140 0x21 0x6c0f7a2d 0xf12e9dd7 0xf00c26e 0x4e67d130 0x29c181a9 sframe.tal 0xda7
[_scPCB_FIELD_VALUES_]a
Y n 0x34a5 0x25 0xbc9399a7 0x520ff80d 0xe3170020 0x9573c243 0xc0d8f38b
0x141 0x3e 0xaaba3306 0x414b40bc 0x3f519d9b 0xad6f55b6 0x86564dc0 cglobal.tal 0xd0
[_scPCB_FIELD_VALUES_]dPSTATEDEBUGINSPECT
Y n 0x34ca 0x6b 0x8cf49edf 0x18ff88d8 0xcd69a218 0xcd623a28 0xbdfeb0c
0x142 0x28 0x28170a3d 0xfe572fcc 0xbf7bab20 0x1ea9a74d 0x819639f4 ssignal.tal 0x857
[_scPCB_FIELD_VALUES_]a
Y n 0x3535 0x20 0x20483312 0x3109f51a 0x93d73789 0x98441455 0xc653972c
0x143 0x47 0x56080700 0xc64b36ed 0xa3e4bb60 0x5797c74b 0x56d5741 cglobal.tal 0xd0
[_scPCB_FIELD_VALUES_]dPSTATEINSPECT
N n 0x3555 0x30 0x0 0x0 0x0 0x0 0x0
0x144 0x23 0x0 0x0 0x0 0x0 0x0
[_scPCB_FIELD_VALUES_]a
Y n 0x3585 0x22 0x17dfc9be 0xbb532ec7 0x776730e3 0x6b996670 0x73afc26c
0x145 0x42 0xcf1c79be 0x81ce463b 0xbadfc650 0xa8a868ea 0x66d95be4 cglobal.tal 0xd0
[_scPCB_FIELD_VALUES_]dPSTATEDEBUGTOINSPECT
N n 0x35a7 0x30 0x0 0x0 0x0 0x0 0x0
0x146 0x25 0x0 0x0 0x0 0x0 0x0
[_scPCB_FIELD_VALUES_]a
Y n 0x35d7 0x8fa 0xe379856a 0x6b30dcf0 0x8ad3c4a7 0x52ccdc1 0xa2feb727
0x146 0x2f 0x837d0b3f 0xd0364ee3 0x79ba93e8 0x43359338 0x526acbb2 cglobal.tal 0xd0
_scCPU_PIN_FIELDS_
Y n 0x3ed1 0x20 0x505f7535 0x575feb4f 0xff935e9d 0xe02bd09 0xb92f7074
0x175 0x0 0xc29c6555 0xc900b1f9 0x73924946 0xb82c887e 0xbfacbe69
[_scCPU_PIN_FIELDS_]_paT9050 GUARDIAN  CPU PIN FIELDS_
Y n 0x3ef1 0x3e 0x89a7b2e3 0xba23be4a 0x63c3a65c 0xf82329d1 0x797db4eb
0x176 0x0 0xf88032a8 0x89cbd368 0xcaa360fa 0x19373233 0xb3198c1b cglobal.tal 0xd0
[_scCPU_PIN_FIELDS_]dPIDCPU
Y n 0x3f2f 0x4 0xedf2916d 0xe9f08e62 0xc79dec4d 0x34b2bc68 0x86245488
0x178 0x12 0x24e38fec 0x70064198 0x32d85d1d 0x2f870ebc 0xb07dcbea sprcinf.tal 0xed
[_scCPU_PIN_FIELDS_]a
Y n 0x3f33 0xf 0x44912cee 0x651e140a 0x6629ee60 0xabd6bd10 0x16c36b42
0x178 0x18 0xb3c967e3 0xbf2cbaeb 0xbd43a8fc 0x6b3c2be4 0x3097a988 cglobal.tal 0xd0
[_scCPU_PIN_FIELDS_]dPIDPIN
Y n 0x3f42 0x5 0xda83207e 0xc2efcf5e 0xae1da60b 0x6925c101 0x4ef298dd
0x179 0x12 0xb1927db2 0x60c37da1 0x77efe93b 0x16dbc22c 0xf2ff4075 sprcinf.tal 0xee
[_scCPU_PIN_FIELDS_]a
Y n 0x3f47 0x1 0xca31d1f1 0x9d43446b 0x32a1326 0x8acc2e8f 0x8f6f9195
0x179 0x19 0x92b404e5 0x56588ced 0x6c1acd4e 0xbf053f68 0x9f73a93 cglobal.tal 0xd0
#endif /* ROSETTA INDEX */
//
#endif
// PREPROC: end of section: cpu_pin_fields
//
//
#if (!defined(dpcbz_h_including_self))
#undef dpcbz_h_including_section
#endif
// end of file
