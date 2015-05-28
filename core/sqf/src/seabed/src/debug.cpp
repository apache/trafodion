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

#include <ctype.h>
#include <cxxabi.h>
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include "seabed/debug.h"
#include "seabed/trace.h"

#include "apictr.h"
#include "env.h"
#include "mstrace.h"

enum { MAX_ADDR = 400 };
enum { MAX_BT   = 25 };

typedef char Addr_Type[MAX_BT][MAX_ADDR];

static bool debug_exec_exists(char *pp_exec) {
    char         la_fname[PATH_MAX];
    char         la_path[PATH_MAX+1];
    int          lv_err;
    bool         lv_ret;
    struct stat  lv_statbuf;
    char        *lp_beg;
    char        *lp_end;
    char        *lp_env;

    // find absoute path to exec from path
    lp_env = getenv("PATH");
    strcpy(la_path, lp_env);
    lp_beg = la_path;
    lv_ret = false;
    do {
        lp_end = strchr(lp_beg, ':');
        if (lp_end != NULL)
            *lp_end = '\0';
        if (*lp_beg == '/')
            sprintf(la_fname, "%s/%s", lp_beg, pp_exec);
        else // transform relative-to-absolute
            sprintf(la_fname, "%s/%s/%s", getenv("PWD"), lp_beg, pp_exec);
        lv_err = lstat(la_fname, &lv_statbuf);
        if (lv_err == 0) {
            lv_ret = true;
            break;
        }
        if (lp_end == NULL)
            break;
        lp_beg = &lp_end[1];
    } while (lp_end != NULL);
    return lv_ret;
}

//
// Purpose: debug trap handler
//
static void debug_th(int pv_sig) {
    const char *WHERE = "XDEBUG(internal)";
    char       *la_arg[100];
    char        la_debug[PATH_MAX];
    char        la_debugger[10];
    char        la_errno[100];
    char        la_exec[PATH_MAX];
    char        la_pid[40];
    char        la_title[PATH_MAX];
    bool        lv_arg;
    int         lv_inx;
    int         lv_pid;
    int         lv_ret;
    char       *lp_end;
    char       *lp_env;
    char       *lp_p;

    pv_sig = pv_sig; // touch
    lv_ret = static_cast<int>(readlink("/proc/self/exe", la_exec, PATH_MAX));
    if (lv_ret == -1)
        return;
    la_exec[lv_ret] = '\0';

    strcpy(la_debugger, "ddd");
    if (!debug_exec_exists(la_debugger))
        strcpy(la_debugger, "gdb");
    lv_pid = getpid();
    sprintf(la_pid, "%d%c", lv_pid, '\0');

    lp_env = getenv(gp_ms_env_debug);
    lv_arg = false;
    lv_inx = 0;
    if (lp_env != NULL) {
        if (isdigit(lp_env[0])) {
            if (atoi(lp_env)) {
                lv_arg = true;
                la_arg[lv_inx++] = const_cast<char *>("xterm");
                la_arg[lv_inx++] = const_cast<char *>("-geometry");
                la_arg[lv_inx++] = const_cast<char *>("80x25+0+0");
            }
        } else {
            lv_arg = true;
            la_arg[lv_inx++] = const_cast<char *>("xterm");
            strcpy(la_debug, lp_env);
            lp_p = la_debug;
            while (isblank(*lp_p))
                lp_p++;
            while ((lp_p != NULL) && (*lp_p)) {
                la_arg[lv_inx++] = lp_p;
                lp_end = strchr(lp_p, ' ');
                if (lp_end == NULL)
                    lp_p = lp_end;
                else {
                    *lp_end = '\0';
                    lp_p = &lp_end[1];
                    while (isblank(*lp_p))
                        lp_p++;
                }
            }
            la_arg[lv_inx] = NULL;
        }
        if (lv_arg) {
            la_arg[lv_inx++] = const_cast<char *>("-T");
            sprintf(la_title, "%s cmd=%s pid=%d", la_debugger, la_exec, lv_pid);
            la_arg[lv_inx++] = la_title;
            la_arg[lv_inx++] = const_cast<char *>("-e");
            la_arg[lv_inx++] = la_debugger;
            la_arg[lv_inx++] = la_exec;
            la_arg[lv_inx++] = la_pid;
            la_arg[lv_inx] = NULL;
        }
    }

    if (!lv_arg) {
        la_arg[lv_inx++] = la_debugger;
        la_arg[lv_inx++] = la_exec;
        la_arg[lv_inx++] = la_pid;
        la_arg[lv_inx] = NULL;
    }

    lv_pid = fork();

    if (lv_pid < 0) { // error
        if (gv_ms_trace_errors)
            trace_where_printf(WHERE, "cannot fork errno=%d(%s)\n",
                               errno,
                               strerror_r(errno, la_errno, sizeof(la_errno)));
        abort(); // can't use SB_util_abort
    } else if (lv_pid) { // parent
        sleep(5); // Give GDB time to attach
    } else { // child
        execvp(la_arg[0], la_arg);
        if (gv_ms_trace_errors)
            trace_where_printf(WHERE, "cannot exec cmd=%s, errno=%d(%s)\n",
                               la_arg[0],
                               errno,
                               strerror_r(errno, la_errno, sizeof(la_errno)));
        abort(); // in case execvp doesn't work - can't use SB_util_abort
    }
}

//
// Pupose: backtrace
//
SB_Export void SB_backtrace(SB_BT_CB pv_callback) {
    Addr_Type  la_addr;
    void      *la_array[MAX_BT];
    char       la_line[BUFSIZ];
    int        lv_array_size;

    lv_array_size = backtrace(la_array, MAX_BT);

    pv_callback(SB_BT_REASON_BEGIN, NULL);
    // start at 1 to exclude self
    for (int lv_inx1 = 1; lv_inx1 < lv_array_size; lv_inx1++) {
        char *lp_symbol = la_addr[lv_inx1];
        char la_symbol_save[BUFSIZ];
        strcpy(la_symbol_save, lp_symbol);
        // eg:
        // a.out(_ZN15ExceptionTracerC1Ev+0x11) [0x400943]
        enum { TOKEN_PROG = 0, TOKEN_FUN, TOKEN_OFF, TOKEN_ADDR };
        enum { MAX_TOKENS = 4 };
        char *la_tokens[MAX_TOKENS];
        for (int inx2 = 0; inx2 < MAX_TOKENS; inx2++) {
            static const char *sa_delim = "()[]+ ";
            char *lp_save;
            char *lp_p = strtok_r(lp_symbol, sa_delim, &lp_save);
            if (lp_p != NULL)
                la_tokens[inx2] = lp_p;
            else
                la_tokens[inx2] = const_cast<char *>("");
            lp_symbol = NULL;
        }
        char *lp_fun = la_tokens[TOKEN_FUN];
        if (lp_fun[0] == '\0') {
            sprintf(la_line, "sym1=%s\n", la_symbol_save);
            pv_callback(SB_BT_REASON_STR, la_line);
        } else {
            int lv_status;
            char *lp_name = abi::__cxa_demangle(lp_fun, 0, 0, &lv_status);
            if (lv_status)
                lp_name = lp_fun;
            sprintf(la_line, "%s(%s+%s) [%s]\n",
                    la_tokens[0], lp_name, la_tokens[2], la_tokens[3]);
            if (lv_status == 0)
                free(lp_name);
            pv_callback(SB_BT_REASON_STR, la_line);
        }
    }
    pv_callback(SB_BT_REASON_END, NULL);
}

//
// Pupose: backtrace2
//
SB_Export void SB_backtrace2(int   pv_max_rec_count,
                             int   pv_rec_size,
                             int  *pp_rec_count,
                             char *pp_records) {
    Addr_Type  la_addr;
    void      *la_array[MAX_BT];
    char       la_line[BUFSIZ];
    int        lv_array_size;
    int        lv_rec_count;

    lv_array_size = backtrace(la_array, MAX_BT);

    // start at 1 to exclude self
    lv_rec_count = 0;
    for (int lv_inx1 = 1; lv_inx1 < lv_array_size; lv_inx1++, lv_rec_count++) {
        char *lp_symbol = la_addr[lv_inx1];
        char la_symbol_save[BUFSIZ];

        if (lv_rec_count >= pv_max_rec_count)
            break;

        strcpy(la_symbol_save, lp_symbol);
        // eg:
        // a.out(_ZN15ExceptionTracerC1Ev+0x11) [0x400943]
        enum { TOKEN_PROG = 0, TOKEN_FUN, TOKEN_OFF, TOKEN_ADDR };
        enum { MAX_TOKENS = 4 };
        char *la_tokens[MAX_TOKENS];
        for (int inx2 = 0; inx2 < MAX_TOKENS; inx2++) {
            static const char *sa_delim = "()[]+ ";
            char *lp_save;
            char *lp_p = strtok_r(lp_symbol, sa_delim, &lp_save);
            if (lp_p != NULL)
                la_tokens[inx2] = lp_p;
            else
                la_tokens[inx2] = const_cast<char *>("");
            lp_symbol = NULL;
        }
        char *lp_fun = la_tokens[TOKEN_FUN];
        if (lp_fun[0] == '\0') {
            sprintf(la_line, "sym1=%s", la_symbol_save);
        } else {
            int lv_status;
            char *lp_name = abi::__cxa_demangle(lp_fun, 0, 0, &lv_status);
            if (lv_status)
                lp_name = lp_fun;
            sprintf(la_line, "%s(%s+%s) [%s]",
                    la_tokens[0], lp_name, la_tokens[2], la_tokens[3]);
            if (lv_status == 0)
                free(lp_name);
        }
        if (static_cast<int>(strlen(la_line)) >= pv_rec_size)
            la_line[pv_rec_size-1] = '\0';
        strcpy(&pp_records[lv_rec_count * pv_rec_size], la_line);
    }
    *pp_rec_count = lv_rec_count;
}

//
// Purpose: call debug
//
SB_Export void XDEBUG() {
    const char  *WHERE = "XDEBUG";
    static bool  lv_sig_reg = false;
    SB_API_CTR  (lv_zctr, XDEBUG);

    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "ENTER\n");
    if (!lv_sig_reg) {
        signal(SIGTRAP, debug_th);
        lv_sig_reg = true;
    } else
        signal(SIGTRAP, SIG_DFL);
    kill(getpid(), SIGTRAP);
    if (gv_ms_trace_params)
        trace_where_printf(WHERE, "EXIT\n");
}

