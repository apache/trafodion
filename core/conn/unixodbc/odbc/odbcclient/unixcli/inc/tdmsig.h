/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

/* T8645D40 - (D40Tool) - tdmsig.h   Extended signal definitions */

#ifndef _TDMSIGH
#define _TDMSIGH


#ifdef __cplusplus
   extern "C" {
#endif

#include <sys/types.h>
#include <signal.h>
#include <setjmp.h>

/* this version provides data and function declarations for the extended-signal
 * API available for native code.
 */

//#ifdef _TNS_R_TARGET /* native */


#ifndef unixcli
   typedef struct stack_t {
      void   *ss_sp;
      size_t ss_size;
      int    ss_flags;
   } stack_t;

   typedef void *mcontext_t;
   typedef void *siginfo_t;


   typedef struct ucontext_t {
      struct ucontext_t *uc_link;
      int        filler1;                                                        /*D40:SSR019710.3*/
      sigset_t   uc_sigmask;
      stack_t    uc_stack;
      mcontext_t uc_mcontext;
      double     filler2[ 61 ];                                                  /*G07:JLC192113.1*/
   } ucontext_t;

   #define ucontext_t_len      sizeof( ucontext_t )

#endif
   typedef struct sig_save_template {
      double filler[ 37 ];
   } sig_save_template;

   #define sigsave_len         sizeof( sig_save_template )

   #define SIG_DEBUG           ((void (*)(int)) 0xFFFC0004)
   #define SIG_ABORT           ((void (*)(int)) 0xFFFC0005)

   int SIGACTION_INIT_(
            void (*)(int, siginfo_t *, void *));                                 /*G05/Rosetta:DFH1256.22.1*/

   int SIGACTION_SUPPLANT_(
            void (*)(int, siginfo_t *, void *),                                  /*G05/Rosetta:DFH1256.22.2*/
                  sig_save_template *, short);                                   /*G05/Rosetta:DFH1256.22.3*/

   int SIGACTION_RESTORE_( sig_save_template * );

   int SIGJMP_MASKSET_( jmp_buf, sigset_t * );

   const char *signal_name_(const int sig);                                      /*D40:DFH0439.1*/
//#endif /* _TNS_R_TARGET */

#ifdef __cplusplus
   }
#endif

/*  ----------------- End of tdmsig.h -----------------  */

#endif  /* _TDMSIGH defined */
