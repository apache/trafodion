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
// PREPROC: start of section: 
#if (defined(ppctlz_h_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_
//

#include "rosetta/rosgen.h" /* rosetta utilities */
//
#endif
// PREPROC: end of section: 
//
// #pragma section STACK_SWITCHTOTOS_
//
// PREPROC: start of skipped section: stack_switchtotos_
//

// #pragma section STACK_SWITCHTOUSER_
//
// PREPROC: start of skipped section: stack_switchtouser_
//

// #pragma section STACK_SWITCHTOUSERINT32_
//
// PREPROC: start of skipped section: stack_switchtouserint32_
//

// #pragma section ARMTRAP
//
// PREPROC: start of skipped section: armtrap
//

// #pragma section SETTRAP
//
// PREPROC: start of skipped section: settrap
//

// #pragma section TRAP_STOP_
//
// PREPROC: start of section: trap_stop_
#if (defined(ppctlz_h_trap_stop_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_trap_stop_
//
 _priv _resident _extensible void TRAP_STOP_
  (int_16       pin,         // PIN OF PROCESS TO BE STOPPED.
   NSK_regSave *regsave_ptr) // POINTS TO REGSAVE OF TRAPPED
// PROCESS IN THE INTERRUPT
// HANDLER STACK, IF FROM IH..
;
//
#endif
// PREPROC: end of section: trap_stop_
//
// #pragma section STOP_IF_UNRESUMABLE_
//
// PREPROC: start of section: stop_if_unresumable_
#if (defined(ppctlz_h_stop_if_unresumable_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_stop_if_unresumable_
//

 _priv _resident int_16 STOP_IF_UNRESUMABLE_
  (int_16   pin,     // AFFECTED PROCESS
   int_16  *regsave) // IT'S REGISTERS (EXAMINED ONLY IN TRAPPING MODE)
;
//
#endif
// PREPROC: end of section: stop_if_unresumable_
//
// #pragma section TRAPOUT
//
// PREPROC: start of section: trapout
#if (defined(ppctlz_h_trapout) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_trapout
//

 _priv _resident _extensible void TRAPOUT
  (int_16   pin,
   int_16   trap_number,
   int_16   siv_num,
   int_16  *sgqe_ptr);
//
#endif
// PREPROC: end of section: trapout
//
// #pragma section TRAPOUT_LOOPTIMER_ACTION_
//
// PREPROC: start of section: trapout_looptimer_action_
#if (defined(ppctlz_h_trapout_looptimer_action_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_trapout_looptimer_action_
//

 _priv _resident void TRAPOUT_LOOPTIMER_ACTION_
  (int_16  pin);
//
#endif
// PREPROC: end of section: trapout_looptimer_action_
//
// #pragma section SYSIN
//
// PREPROC: start of section: sysin
#if (defined(ppctlz_h_sysin) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_sysin
//
 _priv _resident int_16 SYSIN(int_16  stck);
//
#endif
// PREPROC: end of section: sysin
//
// #pragma section SETSTOP
//
// PREPROC: start of section: setstop
#if (defined(ppctlz_h_setstop) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_setstop
//
#ifndef ALREADY_SOURCE_IN_SETSTOP_
#define ALREADY_SOURCE_IN_SETSTOP_
#ifdef __cplusplus
extern "C"
#endif
DllImport
short  SETSTOP(short  stopmode);
#endif // ALREADY_SOURCE_IN_SETSTOP_
//
#endif
// PREPROC: end of section: setstop
//
// #pragma section LASTADDR
//
// PREPROC: start of skipped section: lastaddr
//

// #pragma section LASTADDRX
//
// PREPROC: start of skipped section: lastaddrx
//

// #pragma section MYPID
//
// PREPROC: start of section: mypid
#if (defined(ppctlz_h_mypid) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_mypid
//

 _resident _callable int_16 MYPID();
//
#endif
// PREPROC: end of section: mypid
//
// #pragma section SETLOOPTIMER
//
// PREPROC: start of section: setlooptimer
#if (defined(ppctlz_h_setlooptimer) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_setlooptimer
//

 _callable _resident _variable _cc_status SETLOOPTIMER
  (int_16   newtimer,
   int_16  *oldtimer);
//
#endif
// PREPROC: end of section: setlooptimer
//
// #pragma section PIE_IMPORTANT_PROCESS_
//
// PREPROC: start of section: pie_important_process_
#if (defined(ppctlz_h_pie_important_process_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_pie_important_process_
//

 _priv _resident int_16 PIE_IMPORTANT_PROCESS_
  (int_16  msgpri);
//
#endif
// PREPROC: end of section: pie_important_process_
//
// #pragma section PIE_REQ_PRIORITY_INFORM_
//
// PREPROC: start of section: pie_req_priority_inform_
#if (defined(ppctlz_h_pie_req_priority_inform_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_pie_req_priority_inform_
//

 _priv _resident void PIE_REQ_PRIORITY_INFORM_
  (int_32  newpri);
//
#endif
// PREPROC: end of section: pie_req_priority_inform_
//
// #pragma section PIE_DP2_WAIT_
//
// PREPROC: start of section: pie_dp2_wait_
#if (defined(ppctlz_h_pie_dp2_wait_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_pie_dp2_wait_
//

 _priv _resident int_16 PIE_DP2_WAIT_(int_16  event,
                                                int_32  time,
                                                int_32  in_pri);
//
#endif
// PREPROC: end of section: pie_dp2_wait_
//
// #pragma section WAIT
//
// PREPROC: start of section: wait
#if (defined(ppctlz_h_wait) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_wait
//

 _priv _resident int_16 WAIT(int_16  event,
                                       int_32  time);
//
#endif
// PREPROC: end of section: wait
//
// #pragma section WAIT_MULTIPLE_
//
// PREPROC: start of section: wait_multiple_
#if (defined(ppctlz_h_wait_multiple_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_wait_multiple_
//

 _priv _resident int_16 WAIT_MULTIPLE_(int_16   event,
                                                 int_32   time,
                                                 fixed_0
                                                          eventu,
                                                 fixed_0
                                                         *eventu_return);
//
#endif
// PREPROC: end of section: wait_multiple_
//
// #pragma section DELAY
//
// PREPROC: start of section: delay
#if (defined(ppctlz_h_delay) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_delay
//
extern "C"
DllImport
void DELAY(int_32  time);
//
#endif
// PREPROC: end of section: delay
//
// #pragma section AWAKE_A06
//
// PREPROC: start of section: awake_a06
#if (defined(ppctlz_h_awake_a06) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_awake_a06
//

_alias("AWAKE^A06") _priv _resident void AWAKE_A06
  (int_16  pin,
   int_16  event,
   int_16  func);
//
#endif
// PREPROC: end of section: awake_a06
//
// #pragma section AWAKE
//
// PREPROC: start of section: awake
#if (defined(ppctlz_h_awake) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_awake
//

 _priv _resident void AWAKE(int_16  pin,
                                      int_16  event);
//
#endif
// PREPROC: end of section: awake
//
// #pragma section AWAKE_LREQ_
//
// PREPROC: start of section: awake_lreq_
#if (defined(ppctlz_h_awake_lreq_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_awake_lreq_
//

 _priv _resident void AWAKE_LREQ_ (int_16  pin,
                                             int_16  msgpri);
//
#endif
// PREPROC: end of section: awake_lreq_
//
// #pragma section AWAKE_MULTIPLE_
//
// PREPROC: start of section: awake_multiple_
#if (defined(ppctlz_h_awake_multiple_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_awake_multiple_
//

 _priv _resident void AWAKE_MULTIPLE_(int_16   pin,
                                                int_16   event,
                                                fixed_0  eventu);
//
#endif
// PREPROC: end of section: awake_multiple_
//
// #pragma section CONTENTION_FOR_CPU_
//
// PREPROC: start of section: contention_for_cpu_
#if (defined(ppctlz_h_contention_for_cpu_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_contention_for_cpu_
//

 _priv _resident int_16 CONTENTION_FOR_CPU_
  (int_16  msgpri);
//
#endif
// PREPROC: end of section: contention_for_cpu_
//
// #pragma section CONTENTION_FOR_DP2_
//
// PREPROC: start of section: contention_for_dp2_
#if (defined(ppctlz_h_contention_for_dp2_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_contention_for_dp2_
//

 _priv _resident int_16 CONTENTION_FOR_DP2_
  (int_16  msgpri,
   int_16  hotevents);
//
#endif
// PREPROC: end of section: contention_for_dp2_
//
// #pragma section NEWPROCESSNOWAIT
//
// PREPROC: start of section: newprocessnowait
#if (defined(ppctlz_h_newprocessnowait) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_newprocessnowait
//

 _callable _extensible_n(7) void NEWPROCESSNOWAIT
  (int_16  *file,
   int_16   priority,
   int_16   datapages,
   int_16   processor,
   int_16  *crtpid,
   int_16  *error,
   int_16  *pairname,
   int_16  *hometerm,
   int_16   inspectflag,
   int_16   gmom_jobid,
   int_16  *errinfo,
   int_32   pfs_size);
//
#endif
// PREPROC: end of section: newprocessnowait
//
// #pragma section NEWPROCESS
//
// PREPROC: start of section: newprocess
#if (defined(ppctlz_h_newprocess) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_newprocess
//

 _callable _extensible_n(7) void NEWPROCESS
  (int_16  *file,
   int_16   priority,
   int_16   datapages,
   int_16   processor,
   int_16  *crtpid,
   int_16  *error,
   int_16  *pairname,
   int_16  *hometerm,
   int_16   inspectflag,
   int_16   gmom_jobid,
   int_16  *errinfo,
   int_32   pfs_size);
//
#endif
// PREPROC: end of section: newprocess
//
// #pragma section ABEND
//
// PREPROC: start of section: abend
#if (defined(ppctlz_h_abend) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_abend
//

 _extensible_n(1) _callable _resident _cc_status ABEND
  (int_16        *crtpid,
   int_16         stopbackup,
   int_16        *perror,
   int_16         complcode,
   int_16         terminfo,
   int_16        *ssid,
   int_16         length,
   unsigned_char *text);
//
#endif
// PREPROC: end of section: abend
//
// #pragma section STOP
//
// PREPROC: start of section: stop
#if (defined(ppctlz_h_stop) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_stop
//

 _extensible_n(1) _callable _resident _cc_status STOP
  (int_16        *crtpid,
   int_16         stopbackup,
   int_16        *perror,
   int_16         complcode,
   int_16         terminfo,
   int_16        *ssid,
   int_16         length,
   unsigned_char *text);
//
#endif
// PREPROC: end of section: stop
//
// #pragma section ABEND_CURPIN_OR_HALT_
//
// PREPROC: start of section: abend_curpin_or_halt_
#if (defined(ppctlz_h_abend_curpin_or_halt_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_abend_curpin_or_halt_
//

 _priv _resident void ABEND_CURPIN_OR_HALT_
  (int_16  haltcode) //  I:
;
//
#endif
// PREPROC: end of section: abend_curpin_or_halt_
//
// #pragma section PRIORITY
//
// PREPROC: start of section: priority
#if (defined(ppctlz_h_priority) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_priority
//

 _extensible_n(1) _callable _resident int_16 PRIORITY
  (int_16   newpri,
   int_16  *initpri);
//
#endif
// PREPROC: end of section: priority
//
// #pragma section MOM
//
// PREPROC: start of section: mom
#if (defined(ppctlz_h_mom) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_mom
//

 _callable void MOM(int_16 *crtpid);
//
#endif
// PREPROC: end of section: mom
//
// #pragma section MYTERM
//
// PREPROC: start of section: myterm
#if (defined(ppctlz_h_myterm) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_myterm
//

 _callable void MYTERM(int_16 *tname);
//
#endif
// PREPROC: end of section: myterm
//
// #pragma section SETMYTERM
//
// PREPROC: start of section: setmyterm
#if (defined(ppctlz_h_setmyterm) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_setmyterm
//

 _callable _cc_status SETMYTERM(int_16 *termname);
//
#endif
// PREPROC: end of section: setmyterm
//
// #pragma section GETREMOTECRTPID
//
// PREPROC: start of section: getremotecrtpid
#if (defined(ppctlz_h_getremotecrtpid) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_getremotecrtpid
//

 _callable _resident _cc_status GETREMOTECRTPID
  (int_16   pid,
   int_16  *crtpid,
   int_16   sysid);
//
#endif
// PREPROC: end of section: getremotecrtpid
//
// #pragma section GETCRTPID
//
// PREPROC: start of section: getcrtpid
#if (defined(ppctlz_h_getcrtpid) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_getcrtpid
//

 _callable _resident _cc_status GETCRTPID
  (int_16   pid,
   int_16  *crtpid);
//
#endif
// PREPROC: end of section: getcrtpid
//
// #pragma section MYGMOM
//
// PREPROC: start of section: mygmom
#if (defined(ppctlz_h_mygmom) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_mygmom
//

 _callable void MYGMOM(int_16 *crtpid);
//
#endif
// PREPROC: end of section: mygmom
//
// #pragma section STEPMOM
//
// PREPROC: start of section: stepmom
#if (defined(ppctlz_h_stepmom) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_stepmom
//

 _callable _cc_status STEPMOM(int_16 *crtpid);
//
#endif
// PREPROC: end of section: stepmom
//
// #pragma section CURRENTSPACE
//
// PREPROC: start of skipped section: currentspace
//

// #pragma section SPACEIDTOSEG
//
// PREPROC: start of skipped section: spaceidtoseg
//

// #pragma section CREATORACCESSID
//
// PREPROC: start of section: creatoraccessid
#if (defined(ppctlz_h_creatoraccessid) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_creatoraccessid
//

 _callable int_16 CREATORACCESSID();
//
#endif
// PREPROC: end of section: creatoraccessid
//
// #pragma section PROCESSACCESSID
//
// PREPROC: start of section: processaccessid
#if (defined(ppctlz_h_processaccessid) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_processaccessid
//

 _callable _resident int_16 PROCESSACCESSID();
//
#endif
// PREPROC: end of section: processaccessid
//
// #pragma section CONVERT_D00FILENAME_TO_OLD_
//
// PREPROC: start of section: convert_d00filename_to_old_
#if (defined(ppctlz_h_convert_d00filename_to_old_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_convert_d00filename_to_old_
//
 int_16 CONVERT_D00FILENAME_TO_OLD_
  (unsigned_char *new_fn, // D00 FORMAT FILE NAME
   int_16         len,    // LENGTH OF FILE NAME
   int_16        *old_fn, // PRE-D00 FORMAT FILE NAME, LENGTH 12 WORDS
   int_16         sysid);
//
#endif
// PREPROC: end of section: convert_d00filename_to_old_
//
// #pragma section SWAPFILENAME
//
// PREPROC: start of section: swapfilename
#if (defined(ppctlz_h_swapfilename) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_swapfilename
//

 _cc_status SWAPFILENAME(int_16 *name);
//
#endif
// PREPROC: end of section: swapfilename
//
// #pragma section PROGRAMFILENAME
//
// PREPROC: start of section: programfilename
#if (defined(ppctlz_h_programfilename) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_programfilename
//

 _cc_status PROGRAMFILENAME(int_16 *name);
//
#endif
// PREPROC: end of section: programfilename
//
// #pragma section SYSTEMPROCESSINFO
//
// PREPROC: start of section: systemprocessinfo
#if (defined(ppctlz_h_systemprocessinfo) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_systemprocessinfo
//

 _extensible_n(10) int_16 SYSTEMPROCESSINFO
  (int_16   pid,
   int_16  *crtpid,
   int_16  *creatorid,
   int_16  *processid,
   int_16  *priority,
   int_16  *program,
   int_16  *hometerm,
   int_16   sysid,
   int_16   searchmask,
   int_16  *privref,
   fixed_0 *processtime,
   int_16  *waitstate,
   int_16  *processstate,
   int_16  *libraryfile,
   int_16  *swapfile,
   int_16  *contextchanges,
   int_16  *flags,
   int_16  *licenses,
   int_16  *gmomcrtpidjobid);
//
#endif
// PREPROC: end of section: systemprocessinfo
//
// #pragma section PROCESSINFO
//
// PREPROC: start of section: processinfo
#if (defined(ppctlz_h_processinfo) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_processinfo
//

 _extensible_n(10) int_16 PROCESSINFO(int_16   pid,
                                                int_16  *crtpid,
                                                int_16  *creatorid,
                                                int_16  *processid,
                                                int_16  *priority,
                                                int_16  *program,
                                                int_16  *hometerm,
                                                int_16   sysid,
                                                int_16   searchmask,
                                                int_16  *privref,
                                                fixed_0 *processtime,
                                                int_16  *waitstate,
                                                int_16  *processstate,
                                                int_16  *libraryfile,
                                                int_16  *swapfile,
                                                int_16  *contextchanges,
                                                int_16  *flags,
                                                int_16  *licenses,
                                                int_16  *gmomcrtpidjobid);
//
#endif
// PREPROC: end of section: processinfo
//
// #pragma section XSUSPEND
//
// PREPROC: start of section: xsuspend
#if (defined(ppctlz_h_xsuspend) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_xsuspend
//

 _resident _priv void XSUSPEND(int_16  pin);
//
#endif
// PREPROC: end of section: xsuspend
//
// #pragma section ADDRTOPROCNAME
//
// PREPROC: start of section: addrtoprocname
#if (defined(ppctlz_h_addrtoprocname) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_addrtoprocname
//

 _callable _resident _extensible int_16 ADDRTOPROCNAME
  (int_16         pval,
   int_16         eval,
   unsigned_char *name,
   int_16         usrbuflen,
   int_16        *namelen,
   int_16        *procbase,
   int_16        *procsize,
   int_16        *procentry,
   int_16        *procattr,
   int_16         addrpin);
//
#endif
// PREPROC: end of section: addrtoprocname
//
// #pragma section ADDRTOPROCNAME_RISC_
//
// PREPROC: start of section: addrtoprocname_risc_
#if (defined(ppctlz_h_addrtoprocname_risc_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_addrtoprocname_risc_
//

 _callable _resident _extensible int_16 ADDRTOPROCNAME_RISC_
  (int_32         riscp,
   unsigned_char *name,
   int_16         usrbuflen,
   int_16        *namelen,
   int_16        *proctype,
   int_32        *procbase,
   int_32        *procsize,
   int_32        *procentry,
   int_32        *procattr,
   int_16         addrpin);
//
#endif
// PREPROC: end of section: addrtoprocname_risc_
//
// #pragma section ALTERPRIORITY
//
// PREPROC: start of section: alterpriority
#if (defined(ppctlz_h_alterpriority) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_alterpriority
//

 _callable _cc_status ALTERPRIORITY(int_16  *crtpid,
                                              int_16   newpriority);
//
#endif
// PREPROC: end of section: alterpriority
//
// #pragma section ACTIVATE_SUSPEND_PROCESS
//
// PREPROC: start of section: activate_suspend_process
#if (defined(ppctlz_h_activate_suspend_process) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_activate_suspend_process
//

 _priv _cc_status ACTIVATE_SUSPEND_PROCESS
  (int_16  *crtpid, // PROCESS TIMESTAMP/PID OR NAME/PID
   int_16   request // INPUT
                         // INPUT
            ) // ACTIVATE OR SUSPEND
;
//
#endif
// PREPROC: end of section: activate_suspend_process
//
// #pragma section ACTIVATEPROCESS
//
// PREPROC: start of section: activateprocess
#if (defined(ppctlz_h_activateprocess) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_activateprocess
//

 _callable _cc_status ACTIVATEPROCESS
  (int_16 *crtpid) // PROCESS' CRTPID
;
//
#endif
// PREPROC: end of section: activateprocess
//
// #pragma section SUSPENDPROCESS
//
// PREPROC: start of section: suspendprocess
#if (defined(ppctlz_h_suspendprocess) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_suspendprocess
//

 _callable _cc_status SUSPENDPROCESS
  (int_16 *crtpid) // PROCESS' CRTPID
;
//
#endif
// PREPROC: end of section: suspendprocess
//
// #pragma section MONITORNET
//
// PREPROC: start of section: monitornet
#if (defined(ppctlz_h_monitornet) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_monitornet
//

 _callable _resident void MONITORNET(int_16  on);
//
#endif
// PREPROC: end of section: monitornet
//
// #pragma section MONITORALL
//
// PREPROC: start of section: monitorall
#if (defined(ppctlz_h_monitorall) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_monitorall
//

 _priv _resident void MONITORALL();
//
#endif
// PREPROC: end of section: monitorall
//
// #pragma section MONITORCPUS
//
// PREPROC: start of section: monitorcpus
#if (defined(ppctlz_h_monitorcpus) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_monitorcpus
//
#ifdef __cplusplus
extern "C"
#endif
DllImport
void MONITORCPUS(int_16  mask);
//
#endif
// PREPROC: end of section: monitorcpus
//
// #pragma section MONITORNEW
//
// PREPROC: start of section: monitornew
#if (defined(ppctlz_h_monitornew) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_monitornew
//

 _callable _resident void MONITORNEW(int_16  on);
//
#endif
// PREPROC: end of section: monitornew
//
// #pragma section GETCPCBINFO
//
// PREPROC: start of section: getcpcbinfo
#if (defined(ppctlz_h_getcpcbinfo) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_getcpcbinfo
//

 _extensible _callable void GETCPCBINFO
  (int_16   request_id,
   int_16  *cpcb_info,
   int_16   in_len,
   int_16  *out_len,
   int_16  *Errno);
//
#endif
// PREPROC: end of section: getcpcbinfo
//
// #pragma section PROCESSFILESECURITY
//
// PREPROC: start of section: processfilesecurity
#if (defined(ppctlz_h_processfilesecurity) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_processfilesecurity
//

 _extensible _callable int_16 PROCESSFILESECURITY
  (int_16  new_security);
//
#endif
// PREPROC: end of section: processfilesecurity
//
// #pragma section SETLOGOFFSTATE
//
// PREPROC: start of section: setlogoffstate
#if (defined(ppctlz_h_setlogoffstate) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_setlogoffstate
//

 _callable _resident void SETLOGOFFSTATE();
//
#endif
// PREPROC: end of section: setlogoffstate
//
// #pragma section DEBUGPROCESS
//
// PREPROC: start of section: debugprocess
#if (defined(ppctlz_h_debugprocess) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_debugprocess
//

 _extensible _callable void DEBUGPROCESS
  (int_16  *processid,
   int_16  *error,
   int_16  *term,
   int_16   now);
//
#endif
// PREPROC: end of section: debugprocess
//
// #pragma section DELPTLIST
//
// PREPROC: start of section: delptlist
#if (defined(ppctlz_h_delptlist) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_delptlist
//

 _variable _resident _priv void DELPTLIST
  (int_16    pin,
   NSK_PTLE *element,
   int_16    stop);
//
#endif
// PREPROC: end of section: delptlist
//
// #pragma section ADDPTLIST
//
// PREPROC: start of section: addptlist
#if (defined(ppctlz_h_addptlist) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_addptlist
//

 _variable _resident _priv void ADDPTLIST
  (int_16    pin,
   NSK_PTLE *element,
   int_16    stop);
//
#endif
// PREPROC: end of section: addptlist
//
// #pragma section PROCESSTIME
//
// PREPROC: start of section: processtime
#if (defined(ppctlz_h_processtime) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_processtime
//

 _extensible _callable _resident fixed_0 PROCESSTIME
  (int_16  pid,
   int_16  sysid);
//
#endif
// PREPROC: end of section: processtime
//
// #pragma section MYPROCESSTIME
//
// PREPROC: start of section: myprocesstime
#if (defined(ppctlz_h_myprocesstime) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_myprocesstime
//

 _callable _resident _extensible fixed_0 MYPROCESSTIME();
//
#endif
// PREPROC: end of section: myprocesstime
//
// #pragma section SIGNALPROCESSTIMEOUT
//
// PREPROC: start of section: signalprocesstimeout
#if (defined(ppctlz_h_signalprocesstimeout) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_signalprocesstimeout
//

 _resident _extensible _callable _cc_status SIGNALPROCESSTIMEOUT
  (int_32   toval,
   int_16   parm1,
   int_32   parm2,
   int_16  *tleid);
//
#endif
// PREPROC: end of section: signalprocesstimeout
//
// #pragma section CANCELPROCESSTIMEOUT
//
// PREPROC: start of section: cancelprocesstimeout
#if (defined(ppctlz_h_cancelprocesstimeout) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_cancelprocesstimeout
//

 _resident _extensible _callable _cc_status CANCELPROCESSTIMEOUT
  (int_16  tleid);
//
#endif
// PREPROC: end of section: cancelprocesstimeout
//
// #pragma section CLEARALLPROCESSTIMERS
//
// PREPROC: start of section: clearallprocesstimers
#if (defined(ppctlz_h_clearallprocesstimers) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_clearallprocesstimers
//

 _priv _resident void CLEARALLPROCESSTIMERS
  (int_16  pin);
//
#endif
// PREPROC: end of section: clearallprocesstimers
//
// #pragma section ENLARGEPTLE
//
// PREPROC: start of section: enlargeptle
#if (defined(ppctlz_h_enlargeptle) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_enlargeptle
//

 _priv _resident void ENLARGEPTLE();
//
#endif
// PREPROC: end of section: enlargeptle
//
// #pragma section GETPTLE
//
// PREPROC: start of section: getptle
#if (defined(ppctlz_h_getptle) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_getptle
//

 _priv _resident int_32 GETPTLE();
//
#endif
// PREPROC: end of section: getptle
//
// #pragma section TYPEPTLE
//
// PREPROC: start of section: typeptle
#if (defined(ppctlz_h_typeptle) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_typeptle
//

 _priv _resident int_16 TYPEPTLE(int_32  ptleaddr);
//
#endif
// PREPROC: end of section: typeptle
//
// #pragma section ADJUSTPROCESSTIMER
//
// PREPROC: start of section: adjustprocesstimer
#if (defined(ppctlz_h_adjustprocesstimer) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_adjustprocesstimer
//

 _priv _resident void ADJUSTPROCESSTIMER
  (int_16  pin);
//
#endif
// PREPROC: end of section: adjustprocesstimer
//
// #pragma section WAIT_RETURN_TOV_
//
// PREPROC: start of section: wait_return_tov_
#if (defined(ppctlz_h_wait_return_tov_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_wait_return_tov_
//

 _priv _resident int_16 WAIT_RETURN_TOV_
  (int_16   event,       // INPUT: ONE OR MORE EVENTS TO WAIT ON
   int_32   time,        // INPUT: TIMEOUT VALUE
   int_32  *time_waited) // OUTPUT: TIME LEFT AFTER WAIT RETURNED
;
//
#endif
// PREPROC: end of section: wait_return_tov_
//
// #pragma section CPCB_SETINFO_
//
// PREPROC: start of section: cpcb_setinfo_
#if (defined(ppctlz_h_cpcb_setinfo_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_cpcb_setinfo_
//

 _extensible _callable int_16 CPCB_SETINFO_
  (int_16   set_attr_code, // PROCESS ATTRIBUTE TO BE ALTERED
   int_16  *set_value,     // VALUE FOR THE PROCESS ATTRIBUTE
   int_16   set_value_len, // LENGTH IN WORDS OF SET_VALUE
   int_16  *old_val,       // PRIOR VALUE OF THE ATTRIBUTE BEING ALTERED
   int_16   old_max,       // MAX LENGTH IN WORDS OF OLD_VALUE
   int_16  *old_len)       // ACTUAL VALUE IN WORDS OF OLD_VALUE
// INT     ERROR     RETURNS - FEOK             -  SUCCESSFUL
//                             FEMISSPARMVALUE  -  MISSING PARAMETERS
//                             FEBOUNDSERR      -
//                             FEBADPARMVALUE   -  BAD ATTRIBUTE CODE
//                             FESECVIOL        -  CANNOT CHANGE THE ATTRIBUTE
;
//
#endif
// PREPROC: end of section: cpcb_setinfo_
//
// #pragma section XACTIVATE
//
// PREPROC: start of section: xactivate
#if (defined(ppctlz_h_xactivate) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_xactivate
//

 _resident _priv int_16 XACTIVATE(int_16  pin) // PROCESS TO ACTIVATE
;
//
#endif
// PREPROC: end of section: xactivate
//
// #pragma section PK_DO_SIG_OR_TRAP_
//
// PREPROC: start of section: pk_do_sig_or_trap_
#if (defined(ppctlz_h_pk_do_sig_or_trap_) || (!defined(ppctlz_h_including_section) && !defined(ppctlz_h_including_self)))
#undef ppctlz_h_pk_do_sig_or_trap_
//

 _extensible _resident _priv int_16 PK_DO_SIG_OR_TRAP_
  (int_16  pin,  // PROCESS TO TRAP/NONDEF SIGNAL.
   int_16  trap, // TRAP # PARAM FOR TRAPOUT.
   int_16  siv)  // SIV # PARAM FOR TRAPOUT/SIG_SGQ_ADD
; //~ source file above = $QUINCE.GRZDV.SPCTL

#endif
// PREPROC: end of section: pk_do_sig_or_trap_
//
//
#if (!defined(ppctlz_h_including_self))
#undef ppctlz_h_including_section
#endif
// end of file
