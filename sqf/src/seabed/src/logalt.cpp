//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2015 Hewlett-Packard Development Company, L.P.
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

#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <linux/unistd.h>

#include <sys/time.h>

#include "seabed/logalt.h"

#include "verslib.h"

VERS_LIB(libsblogalt)

#define gettid() static_cast<pid_t>(syscall(__NR_gettid))

#define SBX_LOG_DEFAULT_SNMPTRAP_ADDR1 "172.31.2.240"
#define SBX_LOG_DEFAULT_SNMPTRAP_ADDR2 "172.31.2.241"
#define SBX_LOG_DEFAULT_SNMPTRAP_CMD   "snmptrap -v 1 -c public"
#define SBX_LOG_PREFIX                 "SBX_log_write"

// mutex
static pthread_mutex_t sbx_log_mutex = PTHREAD_MUTEX_INITIALIZER;

static void sbx_log_lock() {
    int lv_err;

    lv_err = pthread_mutex_lock(&sbx_log_mutex);
    if (lv_err != 0) {
        fprintf(stderr, "pthread_mutex_lock err=%d\n", lv_err);
        abort();
    }
}

static void sbx_log_unlock() {
    int lv_err;

    lv_err = pthread_mutex_unlock(&sbx_log_mutex);
    if (lv_err != 0) {
        fprintf(stderr, "pthread_mutex_unlock err=%d\n", lv_err);
        abort();
    }
}

//
// Purpose: log string
//
void SBX_log_write(int                     pv_log_type,
                   const char             *pp_log_file_dir,
                   const char             *pp_log_file_prefix,
                   int                     pv_comp_id,
                   int                     pv_event_id,
                   posix_sqlog_facility_t  pv_facility,
                   posix_sqlog_severity_t  pv_severity,
                   const char             *pp_name,
                   const char             *pp_msg_prefix,
                   const char             *pp_msg,
                   const char             *pp_snmptrap_cmd,
                   const char             *pp_msg_snmptrap,
                   char                   *pp_msg_ret,
                   size_t                  pv_msg_ret_size) {
    char            la_cmdline[BUFSIZ];
    char            la_host[BUFSIZ];
    char            la_log_file_name[PATH_MAX];
    char            la_log_file_name_persist[PATH_MAX];
    char            la_log_file_prefix[BUFSIZ];
    char            la_log_msg[BUFSIZ];
    char            la_pstack[PATH_MAX + 50];
    char            la_snmptrap_cmd[BUFSIZ];
    char            la_syslog_ident[BUFSIZ];
    char            la_tl[40];
    FILE           *lp_cmdline_file;
    char           *lp_debug;
    FILE           *lp_log_file;
    static FILE    *lp_log_file_persist = NULL;
    const char     *lp_snmptrap_addr;
    const char     *lp_snmptrap_addr_env;
    const char     *lp_snmptrap_prefix;
    struct tm      *lp_tx;
    int             lv_debug;
    int             lv_inx;
    int             lv_ms;
    bool            lv_persist;
    pid_t           lv_pid;
    int             lv_status;
    struct timeval  lv_t;
    pid_t           lv_tid;
    struct tm       lv_tx;
    int             lv_us;

    lp_debug = getenv("SBX_LOG_DEBUG");
    if (lp_debug != NULL)
        lv_debug = atoi(lp_debug);
    else
        lv_debug = 0;

    // get/format time
    gettimeofday(&lv_t, NULL);
    lp_tx = localtime_r(&lv_t.tv_sec, &lv_tx);
    lv_ms = static_cast<int>(lv_t.tv_usec) / 1000;
    lv_us = static_cast<int>(lv_t.tv_usec) - lv_ms * 1000;
    sprintf(la_tl, "%02d/%02d/%04d-%02d:%02d:%02d.%03d.%03d",
            lp_tx->tm_mon + 1, lp_tx->tm_mday, lp_tx->tm_year + 1900,
            lp_tx->tm_hour, lp_tx->tm_min, lp_tx->tm_sec, lv_ms, lv_us);

    // get/format time
    lv_pid = getpid();
    lv_tid = gettid();

    // get cmdline
    lp_cmdline_file = fopen("/proc/self/cmdline", "r");
    if (lp_cmdline_file != NULL) {
        fgets(la_cmdline, sizeof(la_cmdline), lp_cmdline_file);
        fclose(lp_cmdline_file);
    }

    // format msg
    if (pp_msg_prefix == NULL)
        pp_msg_prefix = "";
    sprintf(la_log_msg, "%s: %s (name=%s/pid=%d/tid=%d) (cmp=%d/ev=%d/fac=%d/sev=%d): %s",
            la_tl, pp_msg_prefix,
            pp_name, lv_pid, lv_tid,
            pv_comp_id, pv_event_id, pv_facility, pv_severity,
            pp_msg);

    // return msg?
    if (pp_msg_ret != NULL) {
        pv_msg_ret_size--;
        strncpy(pp_msg_ret, la_log_msg, pv_msg_ret_size);
        pp_msg_ret[pv_msg_ret_size] = '\0';
    }

    sbx_log_lock();

    // write to stderr?
    if (pv_log_type & SBX_LOG_TYPE_STDERR) {
        if (lv_debug)
            printf("%s: STDERR: %s\n", SBX_LOG_PREFIX, la_log_msg);
        fprintf(stderr, la_log_msg);
        fflush(stderr);
        if (pv_log_type & SBX_LOG_TYPE_STDERR_PSTACK) {
            sprintf(la_pstack, "pstack %d 1>&2", lv_pid);
            system(la_pstack);
            fflush(stderr);
        }
    }

    // write to stdout?
    if (pv_log_type & SBX_LOG_TYPE_STDOUT) {
        if (lv_debug)
            printf("%s: STDOUT: %s\n", SBX_LOG_PREFIX, la_log_msg);
        fprintf(stdout, la_log_msg);
        fflush(stdout);
        if (pv_log_type & SBX_LOG_TYPE_STDOUT_PSTACK) {
            sprintf(la_pstack, "pstack %d", lv_pid);
            system(la_pstack);
            fflush(stdout);
        }
    }

    // write to log file?
    if (pv_log_type & SBX_LOG_TYPE_LOGFILE) {
        lv_persist = (pv_log_type & SBX_LOG_TYPE_LOGFILE_PERSIST);
        if (!lv_persist && (lp_log_file_persist != NULL)) {
            fclose(lp_log_file_persist);
            lp_log_file_persist = NULL;
        }
        if (lv_persist && (lp_log_file_persist != NULL)) {
            lp_log_file = lp_log_file_persist;
            if (lv_debug)
                printf("%s: log-file-%s: %s\n",
                       SBX_LOG_PREFIX,
                       la_log_file_name_persist,
                       la_log_msg);
        } else {
            if (pp_log_file_prefix == NULL) {
                sprintf(la_log_file_prefix, "z%s", la_cmdline);
                pp_log_file_prefix = la_log_file_prefix;
            }
            lv_status = gethostname(la_host, sizeof(la_host));
            if (lv_status == -1)
                strcpy(la_host, "unknown");
            if (pp_log_file_dir == NULL)
                sprintf(la_log_file_name,
                        "%s.%s.%d.log",
                        pp_log_file_prefix,
                        la_host,
                        lv_pid);
            else
                sprintf(la_log_file_name,
                        "%s/%s.%s.%d.log",
                        pp_log_file_dir,
                        pp_log_file_prefix,
                        la_host,
                        lv_pid);
            if (lv_debug)
                printf("%s: log-file-%s: %s\n",
                       SBX_LOG_PREFIX,
                       la_log_file_name,
                       la_log_msg);
            lp_log_file = fopen(la_log_file_name, "a");
            if (lv_persist) {
                lp_log_file_persist = lp_log_file;
                sprintf(la_log_file_name_persist, "%s(p)", la_log_file_name);
            }
        }
        if (lp_log_file != NULL) {
            fprintf(lp_log_file, "%s", la_log_msg);
            fflush(lp_log_file);
            if (pv_log_type & SBX_LOG_TYPE_LOGFILE_PSTACK) {
                sprintf(la_pstack, "pstack %d >> %s", lv_pid, la_log_file_name);
                system(la_pstack);
            }
            if (lv_persist) {
                fflush(lp_log_file);
            } else
                fclose(lp_log_file);
        }
    }

    // write to syslog?
    if (pv_log_type & SBX_LOG_TYPE_SYSLOG) {
        sprintf(la_syslog_ident, "%s[%d]", la_cmdline, lv_pid);
        openlog(la_syslog_ident, 0, 0);
        if (lv_debug)
            printf("%s: syslog: <%d> <%d> <%d> %s\n",
                   SBX_LOG_PREFIX,
                   pv_comp_id,
                   pv_event_id,
                   pv_facility,
                   pp_msg);
        // <comp-id> <event-id> <facility> str
        syslog(pv_severity, "<%d> <%d> <%d> %s\n",
               pv_comp_id,
               pv_event_id,
               pv_facility,
               pp_msg);
    }

    // write to snmptrap?
    if (pv_log_type & SBX_LOG_TYPE_SNMPTRAP) {
        if (pp_snmptrap_cmd == NULL)
            lp_snmptrap_prefix = SBX_LOG_DEFAULT_SNMPTRAP_CMD;
        else
            lp_snmptrap_prefix = pp_snmptrap_cmd;
        lp_snmptrap_addr_env = getenv("SBX_LOG_SNMPTRAP_ADDR");
        if (lp_snmptrap_addr_env == NULL)
            lp_snmptrap_addr = SBX_LOG_DEFAULT_SNMPTRAP_ADDR1;
        else
            lp_snmptrap_addr = lp_snmptrap_addr_env;
        for (lv_inx = 0; lv_inx < 2; lv_inx++) {
            sprintf(la_snmptrap_cmd, "%s %s %s",
                    lp_snmptrap_prefix,
                    lp_snmptrap_addr,
                    pp_msg_snmptrap);
            if (lv_debug)
                printf("%s: snmptrap: %s\n",
                       SBX_LOG_PREFIX,
                       la_snmptrap_cmd);
            sbx_log_unlock();
            system(la_snmptrap_cmd);
            sbx_log_lock();
            if (lp_snmptrap_addr_env == NULL)
                lp_snmptrap_addr = SBX_LOG_DEFAULT_SNMPTRAP_ADDR2;
            else
                break;
        }
    }
    sbx_log_unlock();

}

