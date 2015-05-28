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
#include <unistd.h>

#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tchkos.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum  { MAX_DBUF = 1024 * 1024 };        // 1 MB

bool           bidir = false;
bool           bm = false;
int            dsize = 1024;
short          filenum_cli;
short          filenum_cli2;
int            loop = 10;
bool           nocopy = false;
bool           nowaitc = false;
bool           nowaits = false;
TPT_DECL      (phandle);
struct rusage  r_start;
struct rusage  r_stop;
char           recv_buffer[MAX_DBUF];
char           send_buffer[MAX_DBUF];
bool           verbose = false;
struct timeval t_elapsed_data;
struct timeval t_elapsed_total;
struct timeval t_start_data;
struct timeval t_start_total;
struct timeval t_stop;
int            timeout = -1;

#include "ufsri.h"


void *buf_alloc(size_t len) {
    return malloc(len);
}

void  buf_free(void *buf) {
    free(buf);
}

void *thread_cli_fun(void *arg) {
    void        *buf;
    double       busy;
    _xcc_status  cc;
    int          count_read;
    int          count_xferred;
    int          ferr;
    short        filenum;
    int          inx;
    SB_Tag_Type  tag;
    short        tfilenum;

    arg = arg; // touch
    filenum = filenum_cli;
    util_time_timer_start(&t_start_data);
    util_cpu_timer_start(&r_start);
    for (inx = 0; inx < loop; inx++) {
        if (verbose)
            printf("count=%d\n", inx);
        cc = BWRITEREADX(filenum,
                         send_buffer,
                         (int) dsize, // cast
                         bidir ? dsize : 0,
                         &count_read,
                         0);
        if (nowaitc) {
            TEST_CHK_CCEQ(cc);
            tfilenum = filenum;
            cc = BAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            TEST_CHK_CCEQ(cc);
        }
    }

    util_cpu_timer_stop(&r_stop);
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start_total, &t_stop, &t_elapsed_total);
    util_time_elapsed(&t_start_data, &t_stop, &t_elapsed_data);
    util_cpu_timer_busy(&r_start, &r_stop, &t_elapsed_data, &busy);
    ferr = BFILE_CLOSE_(filenum_cli, 0);
    TEST_CHK_FEOK(ferr);
    ferr = BFILE_CLOSE_(filenum_cli2, 0);
    TEST_CHK_FEOK(ferr);

    if (!bm) {
        print_elapsed("", &t_elapsed_total);
        print_elapsed(" (data)", &t_elapsed_data);
    }
    print_rate(bm, "", bidir ? 2 * loop : loop, dsize, &t_elapsed_data, busy);
    return NULL;
}

void *thread_srv_fun(void *arg) {
    void        *buf;
    _xcc_status  cc;
    bool         close_msg1;
    bool         close_msg2;
    int          count_read;
    int          count_xferred;
    int          count_written;
    int          ferr;
    short        filenum;
    int          inx;
    bool         open_msg1;
    bool         open_msg2;
    char        *recv_buffer_ptr;
    RI_Type      ri;
    int          sys_msg;
    SB_Tag_Type  tag;
    short        tfilenum;

    arg = arg; // touch
    close_msg1 = false;
    close_msg2 = false;
    open_msg1 = false;
    open_msg2 = false;

    ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                       0, 0, nowaits ? (short) 1 : (short) 0, // nowait
                       1, 0, // sys msg
                       0, 0, NULL);
    TEST_CHK_FEOK(ferr);
    inx = 0;
    while (!close_msg2) {
        if (nocopy) {
            cc = file_buf_readupdatex(filenum,
                                      &recv_buffer_ptr,
                                      &count_read,
                                      0);
            buf_free(recv_buffer_ptr);
        } else
            cc = BREADUPDATEX(filenum,
                              recv_buffer,
                              (int) dsize, // cast
                              &count_read,
                              0);
        if (nowaits) {
            tfilenum = -1;
            cc = BAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            // don't check cc - could be sys msg
            sys_msg = _xstatus_ne(cc);
        } else
            sys_msg = _xstatus_ne(cc);
        if (sys_msg) {
            if (!open_msg1) {
                assert(recv_buffer[0] == XZSYS_VAL_SMSG_OPEN);
                open_msg1 = true;
            } else if (!open_msg2) {
                assert(recv_buffer[0] == XZSYS_VAL_SMSG_OPEN);
                open_msg2 = true;
            } else if (!close_msg1) {
                assert(recv_buffer[0] == XZSYS_VAL_SMSG_CLOSE);
                close_msg1 = true;
            } else if (!close_msg2) {
                assert(recv_buffer[0] == XZSYS_VAL_SMSG_CLOSE);
                close_msg2 = true;
            }
        } else if (!bm) {
            getri(&ri);
            assert(ri.io_type == XZSYS_VAL_RCV_IOTYPE_WRITEREAD);
        }
        cc = BREPLYX(recv_buffer,
                     bidir ? dsize : 0,
                     &count_written,
                     0,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        if (!sys_msg)
            inx++;
    }
    assert(open_msg1);
    assert(open_msg2);
    assert(close_msg1);
    assert(close_msg2);
    ferr = BFILE_CLOSE_(filenum, 0);
    TEST_CHK_FEOK(ferr);
    return NULL;
}

int main(int argc, char *argv[]) {
    int                ferr;
    void              *result;
    int                status;
    SB_Thread::Thread *thr_cli;
    TAD                zargs[] = {
      { "-bidir",     TA_Bool, TA_NOMAX,    &bidir     },
      { "-bm",        TA_Bool, TA_NOMAX,    &bm        },
      { "-client",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-dsize",     TA_Int,  MAX_DBUF,    &dsize     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-nocopy",    TA_Bool, TA_NOMAX,    &nocopy    },
      { "-nowaitc",   TA_Bool, TA_NOMAX,    &nowaitc   },
      { "-nowaits",   TA_Bool, TA_NOMAX,    &nowaits   },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };
    SB_Thread::Thread *thr_srv;

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(true);
    ferr = file_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_time_timer_start(&t_start_total);
    ferr = BFILE_OPEN_SELF_(&filenum_cli,
                            0, 0, nowaitc ? (short) 1 : (short) 0,
                            0, 0, 0, 0, NULL);
    TEST_CHK_FEOK(ferr);
    ferr = BFILE_OPEN_SELF_(&filenum_cli2,
                            0, 0, nowaitc ? (short) 1 : (short) 0,
                            0, 0, 0, 0, NULL);
    TEST_CHK_FEOK(ferr);

    thr_srv = new SB_Thread::Thread(thread_srv_fun, "server");
    thr_srv->start();
    thr_cli = new SB_Thread::Thread(thread_cli_fun, "client");
    thr_cli->start();
    status = thr_cli->join(&result);
    TEST_CHK_STATUSOK(status);
    status = thr_srv->join(&result);
    TEST_CHK_STATUSOK(status);
    delete thr_cli;
    delete thr_srv;

    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
