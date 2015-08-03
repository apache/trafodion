//------------------------------------------------------------------
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

#ifndef __SB_INT_ASSERT_H_
#define __SB_INT_ASSERT_H_

//#define USE_ASSERT_DEBUG

#include <assert.h>
#include <stddef.h> // size_t

extern void  SB_util_assert_fun(const char *pp_exp,
                                const char *pp_file,
                                unsigned    pv_line,
                                const char *pp_fun);
// bool-type assert
extern void  SB_util_assert_fun_bf(const char *pp_exp,
                                   bool        pv_exp,
                                   const char *pp_file,
                                   unsigned    pv_line,
                                   const char *pp_fun);
extern void  SB_util_assert_fun_bt(const char *pp_exp,
                                   bool        pv_exp,
                                   const char *pp_file,
                                   unsigned    pv_line,
                                   const char *pp_fun);
// const-ptr-type assert
extern void  SB_util_assert_fun_cpeq(const char *pp_exp,
                                     const void *pp_lhs,
                                     const void *pp_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_cpne(const char *pp_exp,
                                     const void *pp_lhs,
                                     const void *pp_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
// int-type assert
extern void  SB_util_assert_fun_if(const char *pp_exp,
                                   int         pv_exp,
                                   const char *pp_file,
                                   unsigned    pv_line,
                                   const char *pp_fun);
extern void  SB_util_assert_fun_ieq(const char *pp_exp,
                                    int         pv_lhs,
                                    int         pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_igt(const char *pp_exp,
                                    int         pv_lhs,
                                    int         pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_ige(const char *pp_exp,
                                    int         pv_lhs,
                                    int         pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_ilt(const char *pp_exp,
                                    int         pv_lhs,
                                    int         pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_ile(const char *pp_exp,
                                    int         pv_lhs,
                                    int         pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_ine(const char *pp_exp,
                                    int         pv_lhs,
                                    int         pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_it(const char *pp_exp,
                                   int         pv_exp,
                                   const char *pp_file,
                                   unsigned    pv_line,
                                   const char *pp_fun);
// long-type assert
extern void  SB_util_assert_fun_leq(const char *pp_exp,
                                    long        pv_lhs,
                                    long        pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_lgt(const char *pp_exp,
                                    long        pv_lhs,
                                    long        pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_lge(const char *pp_exp,
                                    long        pv_lhs,
                                    long        pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_llt(const char *pp_exp,
                                    long        pv_lhs,
                                    long        pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_lle(const char *pp_exp,
                                    long        pv_lhs,
                                    long        pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_lne(const char *pp_exp,
                                    long        pv_lhs,
                                    long        pv_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
// long-long-type assert
extern void  SB_util_assert_fun_lleq(const char *pp_exp,
                                     long long   pv_lhs,
                                     long long   pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_llgt(const char *pp_exp,
                                     long long   pv_lhs,
                                     long long   pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_llge(const char *pp_exp,
                                     long long   pv_lhs,
                                     long long   pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_lllt(const char *pp_exp,
                                     long long   pv_lhs,
                                     long long   pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_llle(const char *pp_exp,
                                     long long   pv_lhs,
                                     long long   pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_llne(const char *pp_exp,
                                     long long    pv_lhs,
                                     long long   pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
// ptr-type assert
extern void  SB_util_assert_fun_peq(const char *pp_exp,
                                    void       *pp_lhs,
                                    void       *pp_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_pgt(const char *pp_exp,
                                    void       *pp_lhs,
                                    void       *pp_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_pge(const char *pp_exp,
                                    void       *pp_lhs,
                                    void       *pp_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_plt(const char *pp_exp,
                                    void       *pp_lhs,
                                    void       *pp_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_ple(const char *pp_exp,
                                    void       *pp_lhs,
                                    void       *pp_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
extern void  SB_util_assert_fun_pne(const char *pp_exp,
                                    void       *pp_lhs,
                                    void       *pp_rhs,
                                    const char *pp_file,
                                    unsigned    pv_line,
                                    const char *pp_fun);
// size_t-type assert
extern void  SB_util_assert_fun_steq(const char *pp_exp,
                                     size_t      pv_lhs,
                                     size_t      pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_stgt(const char *pp_exp,
                                     size_t      pv_lhs,
                                     size_t      pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_stge(const char *pp_exp,
                                     size_t      pv_lhs,
                                     size_t      pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_stlt(const char *pp_exp,
                                     size_t      pv_lhs,
                                     size_t      pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_stle(const char *pp_exp,
                                     size_t      pv_lhs,
                                     size_t      pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_assert_fun_stne(const char *pp_exp,
                                     size_t      pv_lhs,
                                     size_t      pv_rhs,
                                     const char *pp_file,
                                     unsigned    pv_line,
                                     const char *pp_fun);
extern void  SB_util_set_log(int pv_compid, int pv_zid);
extern void  SB_util_set_pname(const char *pp_pname);

#ifdef USE_ASSERT_REGULAR
#  ifdef NDEBUG
#    ifdef USE_ASSERT_ABORT
#      define SB_util_assert(exp) (void)((exp)||abort())
#      define SB_util_assert_bf(iexp) (void)((!iexp)||abort())
#      define SB_util_assert_bt(iexp) (void)((iexp)||abort())
#      define SB_util_assert_cpeq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_cpne(lhs,rhs) (void)((lhs != rhs)||abort())
#      define SB_util_assert_ieq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_if(iexp) (void)((!iexp)||abort())
#      define SB_util_assert_igt(lhs,rhs) (void)((lhs > rhs)||abort())
#      define SB_util_assert_ige(lhs,rhs) (void)((lhs >= rhs)||abort())
#      define SB_util_assert_ilt(lhs,rhs) (void)((lhs < rhs)||abort())
#      define SB_util_assert_ile(lhs,rhs) (void)((lhs <= rhs)||abort())
#      define SB_util_assert_ine(lhs,rhs) (void)((lhs != rhs)||abort())
#      define SB_util_assert_it(iexp) (void)((iexp)||abort())
#      define SB_util_assert_leq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_lgt(lhs,rhs) (void)((lhs > rhs)||abort())
#      define SB_util_assert_lge(lhs,rhs) (void)((lhs >= rhs)||abort())
#      define SB_util_assert_llt(lhs,rhs) (void)((lhs < rhs)||abort())
#      define SB_util_assert_lle(lhs,rhs) (void)((lhs <= rhs)||abort())
#      define SB_util_assert_lne(lhs,rhs) (void)((lhs != rhs)||abort())
#      define SB_util_assert_lleq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_llgt(lhs,rhs) (void)((lhs > rhs)||abort())
#      define SB_util_assert_llge(lhs,rhs) (void)((lhs >= rhs)||abort())
#      define SB_util_assert_lllt(lhs,rhs) (void)((lhs < rhs)||abort())
#      define SB_util_assert_llle(lhs,rhs) (void)((lhs <= rhs)||abort())
#      define SB_util_assert_llne(lhs,rhs) (void)((lhs != rhs)||abort())
#      define SB_util_assert_peq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_pgt(lhs,rhs) (void)((lhs > rhs)||abort())
#      define SB_util_assert_pge(lhs,rhs) (void)((lhs >= rhs)||abort())
#      define SB_util_assert_plt(lhs,rhs) (void)((lhs < rhs)||abort())
#      define SB_util_assert_ple(lhs,rhs) (void)((lhs <= rhs)||abort())
#      define SB_util_assert_pne(lhs,rhs) (void)((lhs != rhs)||abort())
#      define SB_util_assert_steq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_stgt(lhs,rhs) (void)((lhs > rhs)||abort())
#      define SB_util_assert_stge(lhs,rhs) (void)((lhs >= rhs)||abort())
#      define SB_util_assert_stlt(lhs,rhs) (void)((lhs < rhs)||abort())
#      define SB_util_assert_stle(lhs,rhs) (void)((lhs <= rhs)||abort())
#      define SB_util_assert_stne(lhs,rhs) (void)((lhs != rhs)||abort())
#    else
#      define SB_util_assert(exp) ((void)0)
#      define SB_util_assert_bf(iexp) ((void)0)
#      define SB_util_assert_bt(iexp) ((void)0)
#      define SB_util_assert_cpeq(lhs,rhs) ((void)0)
#      define SB_util_assert_cpne(lhs,rhs) ((void)0)
#      define SB_util_assert_ieq(lhs,rhs) ((void)0)
#      define SB_util_assert_if(iexp) ((void)0)
#      define SB_util_assert_igt(lhs,rhs) ((void)0)
#      define SB_util_assert_ige(lhs,rhs) ((void)0)
#      define SB_util_assert_ilt(lhs,rhs) ((void)0)
#      define SB_util_assert_ile(lhs,rhs) ((void)0)
#      define SB_util_assert_ine(lhs,rhs) ((void)0)
#      define SB_util_assert_it(iexp) ((void)0)
#      define SB_util_assert_leq(lhs,rhs) ((void)0)
#      define SB_util_assert_lgt(lhs,rhs) ((void)0)
#      define SB_util_assert_lge(lhs,rhs) ((void)0)
#      define SB_util_assert_llt(lhs,rhs) ((void)0)
#      define SB_util_assert_lle(lhs,rhs) ((void)0)
#      define SB_util_assert_lne(lhs,rhs) ((void)0)
#      define SB_util_assert_lleq(lhs,rhs) ((void)0)
#      define SB_util_assert_llgt(lhs,rhs) ((void)0)
#      define SB_util_assert_llge(lhs,rhs) ((void)0)
#      define SB_util_assert_lllt(lhs,rhs) ((void)0)
#      define SB_util_assert_llle(lhs,rhs) ((void)0)
#      define SB_util_assert_llne(lhs,rhs) ((void)0)
#      define SB_util_assert_peq(lhs,rhs) ((void)0)
#      define SB_util_assert_pgt(lhs,rhs) ((void)0)
#      define SB_util_assert_pge(lhs,rhs) ((void)0)
#      define SB_util_assert_plt(lhs,rhs) ((void)0)
#      define SB_util_assert_ple(lhs,rhs) ((void)0)
#      define SB_util_assert_pne(lhs,rhs) ((void)0)
#      define SB_util_assert_steq(lhs,rhs) ((void)0)
#      define SB_util_assert_stgt(lhs,rhs) ((void)0)
#      define SB_util_assert_stge(lhs,rhs) ((void)0)
#      define SB_util_assert_stlt(lhs,rhs) ((void)0)
#      define SB_util_assert_stle(lhs,rhs) ((void)0)
#      define SB_util_assert_stne(lhs,rhs) ((void)0)
#    endif
#  else
#    define SB_util_assert(exp) assert(exp)
#    define SB_util_assert_bf(iexp) assert(!iexp)
#    define SB_util_assert_bt(iexp) assert(iexp)
#    define SB_util_assert_cpeq(lhs,rhs) assert(lhs == rhs)
#    define SB_util_assert_cpne(lhs,rhs) assert(lhs != rhs)
#    define SB_util_assert_ieq(lhs,rhs) assert(lhs == rhs)
#    define SB_util_assert_if(iexp) assert(!iexp)
#    define SB_util_assert_igt(lhs,rhs) assert(lhs > rhs)
#    define SB_util_assert_ige(lhs,rhs) assert(lhs >= rhs)
#    define SB_util_assert_ilt(lhs,rhs) assert(lhs < rhs)
#    define SB_util_assert_ile(lhs,rhs) assert(lhs <= rhs)
#    define SB_util_assert_ine(lhs,rhs) assert(lhs != rhs)
#    define SB_util_assert_it(iexp) assert(iexp)
#    define SB_util_assert_leq(lhs,rhs) assert(lhs == rhs)
#    define SB_util_assert_lgt(lhs,rhs) assert(lhs > rhs)
#    define SB_util_assert_lge(lhs,rhs) assert(lhs >= rhs)
#    define SB_util_assert_llt(lhs,rhs) assert(lhs < rhs)
#    define SB_util_assert_lle(lhs,rhs) assert(lhs <= rhs)
#    define SB_util_assert_lne(lhs,rhs) assert(lhs != rhs)
#    define SB_util_assert_lleq(lhs,rhs) assert(lhs == rhs)
#    define SB_util_assert_llgt(lhs,rhs) assert(lhs > rhs)
#    define SB_util_assert_llge(lhs,rhs) assert(lhs >= rhs)
#    define SB_util_assert_lllt(lhs,rhs) assert(lhs < rhs)
#    define SB_util_assert_llle(lhs,rhs) assert(lhs <= rhs)
#    define SB_util_assert_llne(lhs,rhs) assert(lhs != rhs)
#    define SB_util_assert_peq(lhs,rhs) assert(lhs == rhs)
#    define SB_util_assert_pgt(lhs,rhs) assert(lhs > rhs)
#    define SB_util_assert_pge(lhs,rhs) assert(lhs >= rhs)
#    define SB_util_assert_plt(lhs,rhs) assert(lhs < rhs)
#    define SB_util_assert_ple(lhs,rhs) assert(lhs <= rhs)
#    define SB_util_assert_pne(lhs,rhs) assert(lhs != rhs)
#    define SB_util_assert_steq(lhs,rhs) assert(lhs == rhs)
#    define SB_util_assert_stgt(lhs,rhs) assert(lhs > rhs)
#    define SB_util_assert_stge(lhs,rhs) assert(lhs >= rhs)
#    define SB_util_assert_stlt(lhs,rhs) assert(lhs < rhs)
#    define SB_util_assert_stle(lhs,rhs) assert(lhs <= rhs)
#    define SB_util_assert_stne(lhs,rhs) assert(lhs != rhs)
#  endif
#else
#  ifdef NDEBUG
#    ifdef USE_ASSERT_ABORT
#      define SB_util_assert(exp) (void)((exp)||abort())
#      define SB_util_assert_bf(iexp) (void)((!iexp)||abort())
#      define SB_util_assert_bt(iexp) (void)((iexp)||abort())
#      define SB_util_assert_cpeq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_cpne(lhs,rhs) (void)((lhs != rhs)||abort())
#      define SB_util_assert_ieq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_if(iexp) (void)((!iexp)||abort())
#      define SB_util_assert_igt(lhs,rhs) (void)((lhs > rhs)||abort())
#      define SB_util_assert_ige(lhs,rhs) (void)((lhs >= rhs)||abort())
#      define SB_util_assert_ilt(lhs,rhs) (void)((lhs < rhs)||abort())
#      define SB_util_assert_ile(lhs,rhs) (void)((lhs <= rhs)||abort())
#      define SB_util_assert_ine(lhs,rhs) (void)((lhs != rhs)||abort())
#      define SB_util_assert_it(iexp) (void)((iexp)||abort())
#      define SB_util_assert_leq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_lgt(lhs,rhs) (void)((lhs > rhs)||abort())
#      define SB_util_assert_lge(lhs,rhs) (void)((lhs >= rhs)||abort())
#      define SB_util_assert_llt(lhs,rhs) (void)((lhs < rhs)||abort())
#      define SB_util_assert_lle(lhs,rhs) (void)((lhs <= rhs)||abort())
#      define SB_util_assert_lne(lhs,rhs) (void)((lhs != rhs)||abort())
#      define SB_util_assert_lleq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_llgt(lhs,rhs) (void)((lhs > rhs)||abort())
#      define SB_util_assert_llge(lhs,rhs) (void)((lhs >= rhs)||abort())
#      define SB_util_assert_lllt(lhs,rhs) (void)((lhs < rhs)||abort())
#      define SB_util_assert_llle(lhs,rhs) (void)((lhs <= rhs)||abort())
#      define SB_util_assert_llne(lhs,rhs) (void)((lhs != rhs)||abort())
#      define SB_util_assert_peq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_pgt(lhs,rhs) (void)((lhs > rhs)||abort())
#      define SB_util_assert_pge(lhs,rhs) (void)((lhs >= rhs)||abort())
#      define SB_util_assert_plt(lhs,rhs) (void)((lhs < rhs)||abort())
#      define SB_util_assert_ple(lhs,rhs) (void)((lhs <= rhs)||abort())
#      define SB_util_assert_pne(lhs,rhs) (void)((lhs != rhs)||abort())
#      define SB_util_assert_steq(lhs,rhs) (void)((lhs == rhs)||abort())
#      define SB_util_assert_stgt(lhs,rhs) (void)((lhs > rhs)||abort())
#      define SB_util_assert_stge(lhs,rhs) (void)((lhs >= rhs)||abort())
#      define SB_util_assert_stlt(lhs,rhs) (void)((lhs < rhs)||abort())
#      define SB_util_assert_stle(lhs,rhs) (void)((lhs <= rhs)||abort())
#      define SB_util_assert_stne(lhs,rhs) (void)((lhs != rhs)||abort())
#    else
#      define SB_util_assert(exp) ((void)0)
#      define SB_util_assert_bf(iexp) ((void)0)
#      define SB_util_assert_bt(iexp) ((void)0)
#      define SB_util_assert_cpeq(lhs,rhs) ((void)0)
#      define SB_util_assert_cpne(lhs,rhs) ((void)0)
#      define SB_util_assert_ieq(lhs,rhs) ((void)0)
#      define SB_util_assert_if(iexp) ((void)0)
#      define SB_util_assert_igt(lhs,rhs) ((void)0)
#      define SB_util_assert_ige(lhs,rhs) ((void)0)
#      define SB_util_assert_ilt(lhs,rhs) ((void)0)
#      define SB_util_assert_ile(lhs,rhs) ((void)0)
#      define SB_util_assert_ine(lhs,rhs) ((void)0)
#      define SB_util_assert_it(iexp) ((void)0)
#      define SB_util_assert_leq(lhs,rhs) ((void)0)
#      define SB_util_assert_lgt(lhs,rhs) ((void)0)
#      define SB_util_assert_lge(lhs,rhs) ((void)0)
#      define SB_util_assert_llt(lhs,rhs) ((void)0)
#      define SB_util_assert_lle(lhs,rhs) ((void)0)
#      define SB_util_assert_lne(lhs,rhs) ((void)0)
#      define SB_util_assert_lleq(lhs,rhs) ((void)0)
#      define SB_util_assert_llgt(lhs,rhs) ((void)0)
#      define SB_util_assert_llge(lhs,rhs) ((void)0)
#      define SB_util_assert_lllt(lhs,rhs) ((void)0)
#      define SB_util_assert_llle(lhs,rhs) ((void)0)
#      define SB_util_assert_llne(lhs,rhs) ((void)0)
#      define SB_util_assert_peq(lhs,rhs) ((void)0)
#      define SB_util_assert_pgt(lhs,rhs) ((void)0)
#      define SB_util_assert_pge(lhs,rhs) ((void)0)
#      define SB_util_assert_plt(lhs,rhs) ((void)0)
#      define SB_util_assert_ple(lhs,rhs) ((void)0)
#      define SB_util_assert_pne(lhs,rhs) ((void)0)
#      define SB_util_assert_steq(lhs,rhs) ((void)0)
#      define SB_util_assert_stgt(lhs,rhs) ((void)0)
#      define SB_util_assert_stge(lhs,rhs) ((void)0)
#      define SB_util_assert_stlt(lhs,rhs) ((void)0)
#      define SB_util_assert_stle(lhs,rhs) ((void)0)
#      define SB_util_assert_stne(lhs,rhs) ((void)0)
#    endif
#  else
#    define SB_util_assert(exp) (void)((exp)||(SB_util_assert_fun(#exp, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_bf(iexp) (void)((!iexp)||(SB_util_assert_fun_bf("!" #iexp, iexp, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_bt(iexp) (void)((iexp)||(SB_util_assert_fun_bt(#iexp, iexp, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_cpeq(lhs,rhs) (void)((lhs == rhs)||(SB_util_assert_fun_cpeq(#lhs " == " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_cpne(lhs,rhs) (void)((lhs != rhs)||(SB_util_assert_fun_cpne(#lhs " != " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_ieq(lhs,rhs) (void)((lhs == rhs)||(SB_util_assert_fun_ieq(#lhs " == " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_if(iexp) (void)((!iexp)||(SB_util_assert_fun_if("!" #iexp, iexp, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_igt(lhs,rhs) (void)((lhs > rhs)||(SB_util_assert_fun_igt(#lhs " > " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_ige(lhs,rhs) (void)((lhs >= rhs)||(SB_util_assert_fun_ige(#lhs " >= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_ilt(lhs,rhs) (void)((lhs < rhs)||(SB_util_assert_fun_ilt(#lhs " < " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_ile(lhs,rhs) (void)((lhs <= rhs)||(SB_util_assert_fun_ile(#lhs " <= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_ine(lhs,rhs) (void)((lhs != rhs)||(SB_util_assert_fun_ine(#lhs " != " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_it(iexp) (void)((iexp)||(SB_util_assert_fun_it(#iexp, iexp, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_leq(lhs,rhs) (void)((lhs == rhs)||(SB_util_assert_fun_leq(#lhs " == " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_lgt(lhs,rhs) (void)((lhs > rhs)||(SB_util_assert_fun_lgt(#lhs " > " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_lge(lhs,rhs) (void)((lhs >= rhs)||(SB_util_assert_fun_lge(#lhs " >= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_llt(lhs,rhs) (void)((lhs < rhs)||(SB_util_assert_fun_llt(#lhs " < " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_lle(lhs,rhs) (void)((lhs <= rhs)||(SB_util_assert_fun_lle(#lhs " <= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_lne(lhs,rhs) (void)((lhs != rhs)||(SB_util_assert_fun_lne(#lhs " != " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_lleq(lhs,rhs) (void)((lhs == rhs)||(SB_util_assert_fun_lleq(#lhs " == " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_llgt(lhs,rhs) (void)((lhs > rhs)||(SB_util_assert_fun_llgt(#lhs " > " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_llge(lhs,rhs) (void)((lhs >= rhs)||(SB_util_assert_fun_llge(#lhs " >= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_lllt(lhs,rhs) (void)((lhs < rhs)||(SB_util_assert_fun_lllt(#lhs " < " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_llle(lhs,rhs) (void)((lhs <= rhs)||(SB_util_assert_fun_llle(#lhs " <= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_llne(lhs,rhs) (void)((lhs != rhs)||(SB_util_assert_fun_llne(#lhs " != " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_peq(lhs,rhs) (void)((lhs == rhs)||(SB_util_assert_fun_peq(#lhs " == " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_pgt(lhs,rhs) (void)((lhs == rhs)||(SB_util_assert_fun_pgt(#lhs " > " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_pge(lhs,rhs) (void)((lhs == rhs)||(SB_util_assert_fun_pge(#lhs " >= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_plt(lhs,rhs) (void)((lhs == rhs)||(SB_util_assert_fun_plt(#lhs " < " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_ple(lhs,rhs) (void)((lhs == rhs)||(SB_util_assert_fun_ple(#lhs " <= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_pne(lhs,rhs) (void)((lhs != rhs)||(SB_util_assert_fun_pne(#lhs " != " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_steq(lhs,rhs) (void)((lhs == rhs)||(SB_util_assert_fun_steq(#lhs " == " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_stgt(lhs,rhs) (void)((lhs > rhs)||(SB_util_assert_fun_stgt(#lhs " > " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_stge(lhs,rhs) (void)((lhs >= rhs)||(SB_util_assert_fun_stge(#lhs " >= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_stlt(lhs,rhs) (void)((lhs < rhs)||(SB_util_assert_fun_stlt(#lhs " < " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_stle(lhs,rhs) (void)((lhs <= rhs)||(SB_util_assert_fun_stle(#lhs " <= " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#    define SB_util_assert_stne(lhs,rhs) (void)((lhs != rhs)||(SB_util_assert_fun_stne(#lhs " != " #rhs, lhs, rhs, __FILE__, __LINE__, __PRETTY_FUNCTION__), 0))
#  endif  // NDEBUG

#endif // USE_ASSERT_REGULAR

// static assert helpers
template <bool x> struct sb_static_assert_tmpl;
template <> struct sb_static_assert_tmpl<true> {
    enum { value = 1 };
};
template<int x> struct sb_static_assert_test{};
#define SB_UTIL_ASSERT_CONCATX(a, b) a##b
#define SB_UTIL_ASSERT_CONCAT(a, b) SB_UTIL_ASSERT_CONCATX(a, b)

// use like SB_util_assert - but can be used for compile-time-check
#define SB_util_static_assert(exp) \
  enum { SB_UTIL_ASSERT_CONCAT(sb_static_assert_enum_, __LINE__) = \
    sizeof(sb_static_assert_tmpl<(bool)(exp)>) \
   }


#endif // !__SB_INT_ASSERT_H_
