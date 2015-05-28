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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/resource.h>

#include "seabed/fserr.h"

#include "tchkfe.h"
#include "tutil.h"
#include "tverslib.h"


VERS_LIB(libsbztutil)

// don't create core dump
void util_abort_core_free() {
    util_set_core_free();
    abort();
}

void util_check(const char *where, int ferr) {
    if (ferr != XZFIL_ERR_OK) {
        printf("WOOPS - %s, ferr=%d\n", where, ferr);
        TEST_CHK_FEOK(ferr);
    }
}

void util_checkdone(const char *where, int done) {
    if (done == 0) {
        printf("WOOPS - %s, done=%d\n", where, done);
        assert(done);
    }
}

void util_cpu_timer_busy(struct rusage  *r_start,
                         struct rusage  *r_stop,
                         struct timeval *t_elapsed,
                         double         *busy) {
    float cpu_sys_us;
    float cpu_tot_us;
    float cpu_user_us;
    float elapsed_us;

    cpu_user_us = (float) r_stop->ru_utime.tv_sec * USPS +
                  (float) r_stop->ru_utime.tv_usec -
                  (float) r_start->ru_utime.tv_sec * USPS -
                  (float) r_start->ru_utime.tv_usec;
    cpu_sys_us = (float) r_stop->ru_stime.tv_sec * USPS +
                 (float) r_stop->ru_stime.tv_usec -
                 (float) r_start->ru_stime.tv_sec * USPS -
                 (float) r_start->ru_stime.tv_usec;
    cpu_tot_us = cpu_user_us + cpu_sys_us;
    elapsed_us = (float) t_elapsed->tv_sec * USPS +
                 (float) t_elapsed->tv_usec;
    if ((t_elapsed->tv_sec == 0) && (t_elapsed->tv_usec == 0))
        *busy = 0.0;
    else
        *busy = cpu_tot_us / elapsed_us * 100.0;
//printf("%d: cpu-elapsed=%f\n", getpid(), cpu_tot_us);
}

void util_cpu_timer_start(struct rusage *usage) {
    getrusage(RUSAGE_SELF, usage);
//printf("%d: cpu-start=%ld\n", getpid(), usage->ru_utime.tv_sec * USPS + usage->ru_utime.tv_usec +
//                              usage->ru_stime.tv_sec * USPS +  usage->ru_stime.tv_usec);
}

void util_cpu_timer_stop(struct rusage *usage) {
    getrusage(RUSAGE_SELF, usage);
//printf("%d: cpu-stop=%ld\n", getpid(), usage->ru_utime.tv_sec * USPS + usage->ru_utime.tv_usec +
//                         usage->ru_stime.tv_sec * USPS +  usage->ru_stime.tv_usec);
}

long util_cpu_timer_wait(int ms) {
    struct rusage r_stop;
    int           inx;
    struct rusage r_start;
    long          us;

    util_cpu_timer_start(&r_start);
    // use up processor time
    for (;;) {
        for (inx = 0; inx < 1000000; inx++) {
        }
        util_cpu_timer_start(&r_stop);
        us = (r_stop.ru_utime.tv_sec - r_start.ru_utime.tv_sec) * USPS +
             r_stop.ru_utime.tv_usec -
             r_start.ru_utime.tv_usec;
        if (us > ms * 10000)
            break;
    }
    return us;
}

void util_format_transid(char *buf, SB_Transid_Type transid) {
    sprintf(buf, PF64 "." PF64 "." PF64 "." PF64,
            transid.id[0], transid.id[1], transid.id[2], transid.id[3]);
}

void util_gethostname(char *name, int len) {
    char *dot;
    int   ret;

    ret = gethostname(name, len);
    assert(ret == 0);
    dot = strchr(name, '.');
    if (dot != NULL)
        *dot = '\0';
}

bool util_ic_program_skip(const char *program) {
    bool ret;

    if (strcmp(program, "pstartd") == 0)
        ret = true;
    else if (strcmp(program, "shell") == 0)
        ret = true;
    else if (strcmp(program, "sp_wrapper") == 0)
        ret = true;
    else if (strcmp(program, "sqwatchdog") == 0)
        ret = true;
    else
        ret = false;
    return ret;
}

void util_set_core_free() {
    struct rlimit limit;
    limit.rlim_cur = 0;
    limit.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &limit);
}

int util_stoi(unsigned char *s) {
    int            base;    // base of digits
    int            i;       // result
    int            sign;    // sign of result
    int            valid;   // valid digit?
    unsigned char  c;       // current digit

    if (s[0] == '-') {
        sign = -1;
        s++;
    } else {
        sign = 1;
    }

    if ((s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X'))) {
        base = 16;
        s += 2;
    } else {
        base = 10;
    }

    i = 0;
    valid = 1;

    while (*s && valid) {
        c = *s;

        if ((c >= '0') && (c <= '9')) {
            i = (i * base) + (c - '0');
        } else if (base == 16) {
            if ((c >= 'a') && (c <= 'f')) {
                i = (i * base) + (c - 'a' + 0xa);
            } else if ((c >= 'A') && (c <= 'F')) {
                i = (i * base) + (c - 'A' + 0xA);
            } else {
                valid = 0;
            }
        } else {
            valid = 0;
        }

        s++;
    }

    i *= sign;

    return(i);
}

void util_test_finish(int client) {
    FILE *f;
    char *p;

    if (client) {
        p = getenv("TEST_STOP");
        if (p == NULL)
            p = (char *) "test.status"; // cast
        f = fopen(p, "w"); // create file
        fclose(f);
    }
}

void util_test_start(int client) {
    char *p;

    client = client; // no-warn
    p = getenv("TEST_STOP");
    if (p == NULL)
        p = (char *) "test.status"; // cast
    unlink(p);
}

void util_time_elapsed(struct timeval *t_start,
                       struct timeval *t_stop,
                       struct timeval *t_elapsed) {
    long sec;
    long us;

    us = (t_stop->tv_sec * USPS + t_stop->tv_usec) -
         (t_start->tv_sec * USPS + t_start->tv_usec);
    sec = us / USPS;
    us -= sec * USPS;
    t_elapsed->tv_sec = sec;
    t_elapsed->tv_usec = us;
//printf("%d: elapsed=%ld.%ld\n", getpid(), sec, us);
}

void util_time_sleep_ms(int ms) {
    struct timespec tv;

    tv.tv_sec = ms / USPS;
    tv.tv_nsec = (ms - tv.tv_sec * MSPS) * 1000000; // ms
    nanosleep(&tv, NULL);
}

void util_time_sleep_ns(int ns) {
    struct timespec tv;

    tv.tv_sec = ns / NSPS;
    tv.tv_nsec = (ns - tv.tv_sec * NSPS);
    nanosleep(&tv, NULL);
}

void util_time_sleep_us(int us) {
    struct timespec tv;

    tv.tv_sec = us / USPS;
    tv.tv_nsec = (us - tv.tv_sec * USPS) * 1000; // us
    nanosleep(&tv, NULL);
}

void util_time_timer_start(struct timeval *t) {
    gettimeofday(t, NULL);
//printf("%d: start=%ld.%ld\n", getpid(), t->tv_sec, t->tv_usec);
}

void util_time_timer_stop(struct timeval *t) {
    gettimeofday(t, NULL);
//printf("%d: stop=%ld.%ld\n", getpid(), t->tv_sec, t->tv_usec);
}

