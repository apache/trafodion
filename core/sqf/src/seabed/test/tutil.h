//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#ifndef __TUTIL_H_
#define __TUTIL_H_

#include <sys/resource.h>
#include <sys/time.h>

#include "seabed/int/types.h" // transid

#define MASTER_RANK 0

enum { MSPS    = 1000 };       // ms per sec
enum { NSPS    = 1000000000 }; // ns per sec
enum { USPS    = 1000000 };    // us per sec

//
// Test Phandle-Type macros
//
#define TCPU_DECL(name)      int name
#define TPIN_DECL(name)      int name
#define TPT_DECL(name)       SB_Phandle_Type name
#define TPT_DECL2(name,size) SB_Phandle_Type name[size]
#define TPT_DECL_INT(name)   int name[sizeof(SB_Phandle_Type)/sizeof(int)]
#define TPT_PTR(name)        SB_Phandle_Type *name
#define TPT_REF(name)        (&name)
#define TPT_REF2(name,inx)   (&name[inx])

#define TPT_COPY_INT(dest, src) memcpy(dest, TPT_REF(src), (int) sizeof(dest))

// Auto array (limited operations)
template <class T>
class Util_AA {
public:
    // constructor - allocate T
    inline Util_AA(int pv_size) : ip_v(new T[pv_size]) {}
    // destructor - deallocate T
    inline ~Util_AA() { delete [] ip_v; }

    // pointer ref - return T *
    inline T *operator->() { return ip_v; }

    // pointer ref - return T *
    inline T *operator&() { return ip_v; }

    // hold T
    T *ip_v;
};

extern void util_abort_core_free(void);
extern void util_check(const char *where, int ferr);
extern void util_checkdone(const char *where, int done);
extern void util_cpu_timer_busy(struct rusage  *r_start,
                                struct rusage  *r_stop,
                                struct timeval *t_elapsed,
                                double         *busy);
extern void util_cpu_timer_start(struct rusage *usage);
extern void util_cpu_timer_stop(struct rusage *usage);
extern long util_cpu_timer_wait(int ms);
extern void util_format_transid(char *buf, SB_Transid_Type transid);
extern void util_gethostname(char *name, int len);
extern bool util_ic_program_skip(const char *program);
extern void util_set_core_free(void);
extern int  util_stoi(unsigned char *s);
extern void util_test_finish(int client);
extern void util_test_start(int client);
extern void util_time_elapsed(struct timeval *t_start,
                              struct timeval *t_stop,
                              struct timeval *t_elapsed);
extern void util_time_sleep_ms(int ms);
extern void util_time_sleep_ns(int ns);
extern void util_time_sleep_us(int us);
extern void util_time_timer_start(struct timeval *t);
extern void util_time_timer_stop(struct timeval *t);

#endif // !__TUTIL_H_
