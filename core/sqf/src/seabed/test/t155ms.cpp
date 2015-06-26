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

#include "seabed/fserr.h"
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

enum  { MAX_CBUF = 1024 * 32 };        // 32 KB
enum  { MAX_DBUF = 1024 * 1024 };      // 1 MB

bool       abandon = false;
bool       bidir = false;
bool       bm = false;
bool       cancelled = false;
int        csize = 0;
int        dsize = 1024;
int        loop = 10;
char       my_name[BUFSIZ];
bool       nocopy = false;
TPT_DECL  (phandle);
bool       rcvd = false;
char       recv_buffer[MAX_DBUF];
short      recv_buffer2[MAX_CBUF/2];
char       send_buffer[MAX_DBUF];
short      send_buffer2[MAX_CBUF/2];
bool       verbose = false;


void *buf_alloc(size_t len) {
    return malloc(len);
}

void  buf_free(void *buf) {
    free(buf);
}

void *thread_cli_fun(void *arg) {
    double          busy;
    int             ferr;
    int             inx;
    int             msgid;
    int             oid;
    struct rusage   r_start;
    struct rusage   r_stop;
    RT              results;
    struct timeval  t_elapsed_data;
    struct timeval  t_elapsed_total;
    struct timeval  t_start_data;
    struct timeval  t_start_total;
    struct timeval  t_stop;

    arg = arg; // touch
    util_time_timer_start(&t_start_total);
    ferr = msg_mon_open_process_self(TPT_REF(phandle), &oid);
    TEST_CHK_FEOK(ferr);

    util_time_timer_start(&t_start_data);
    util_cpu_timer_start(&r_start);
    for (inx = 0; inx < loop; inx++) {
        if (verbose)
            printf("link inx=%d\n", inx);
        ferr = BMSG_LINK_(TPT_REF(phandle),    // phandle
                          &msgid,              // msgid
                          send_buffer2,        // reqctrl
                          (int) csize,         // reqctrlsize // cast
                          recv_buffer2,        // replyctrl
                          bidir ? csize : 0,   // replyctrlmax
                          send_buffer,         // reqdata
                          (int) dsize,         // reqdatasize // cast
                          recv_buffer,         // replydata
                          bidir ? dsize : 0,   // replydatamax
                          0,                   // linkertag
                          0,                   // pri
                          0,                   // xmitclass
                          0);                  // linkopts
        if (abandon) {
            while (__sync_fetch_and_add_1(&rcvd, 0) == 0) { // memory barrier
                usleep(1000);
            }
            rcvd = false;
            ferr = BMSG_ABANDON_(msgid);
            util_check("BMSG_ABANDON_", ferr);
            if (verbose)
                printf("abandoned\n");
            cancelled = true;
            continue;
        }
        ferr = BMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
        util_check("BMSG_BREAK_", ferr);
    }
    util_cpu_timer_stop(&r_stop);
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start_total, &t_stop, &t_elapsed_total);
    util_time_elapsed(&t_start_data, &t_stop, &t_elapsed_data);
    util_cpu_timer_busy(&r_start, &r_stop, &t_elapsed_data, &busy);

    if (!bm) {
        print_elapsed("", &t_elapsed_total);
        print_elapsed(" (data)", &t_elapsed_data);
    }
    print_rate(bm, "", bidir ? 2 * loop : loop, dsize, &t_elapsed_data, busy);
    return NULL;
}

void *thread_srv_fun(void *arg) {
    bool                      disable = false;
    int                       ferr;
    MS_Mon_Process_Info_Type  info;
    int                       inx;
    int                       lerr;
    int                       req_nid;
    int                       req_pid;
    int                       req_ptype;
    short                    *reqctrl;
    char                     *reqdata;
    BMS_SRE                   sre;

    arg = arg; // touch
    ferr = msg_mon_get_process_info_detail((char *) "$cli", &info);
    TEST_CHK_FEOK(ferr);

    do {
        lerr = XWAIT(LREQ, -1);
        TEST_CHK_WAITIGNORE(lerr);
        lerr = BMSG_LISTEN_((short *) &sre, // sre
                            0,              // listenopts
                            0);             // listenertag
    } while (lerr == BSRETYPE_NOWORK);
    assert(sre.sre_flags & XSRE_MON);
    BMSG_REPLY_(sre.sre_msgId,       // msgid
                NULL,                // replyctrl
                0,                   // replyctrlsize
                NULL,                // replydata
                0,                   // replydatasize
                0,                   // errorclass
                NULL);               // newphandle
    for (inx = 0; inx < loop; inx++) {
        do {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = BMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == BSRETYPE_NOWORK);
        if (verbose)
            printf("listen complete, inx=%d, err=%d, sre-flags=%x\n",
                   inx, lerr, sre.sre_flags);
        if (abandon) {
            rcvd = true;
            while (__sync_fetch_and_add_1(&cancelled, 0) == 0) { // memory barrier
                usleep(1000);
            }
            cancelled = false;
            if (verbose)
                printf("abandon ack\n");
        }
        assert((sre.sre_flags & XSRE_MON) == 0);
        if (!bm) {
            ferr = BMSG_GETREQINFO_(MSGINFO_NID,
                                    sre.sre_msgId,
                                    &req_nid);
            util_check("BMSG_GETREQINFO_", ferr);
            assert(req_nid == info.nid);
            ferr = BMSG_GETREQINFO_(MSGINFO_PID,
                                    sre.sre_msgId,
                                    &req_pid);
            util_check("BMSG_GETREQINFO_", ferr);
            assert(req_pid == info.pid);
            ferr = BMSG_GETREQINFO_(MSGINFO_PTYPE,
                                    sre.sre_msgId,
                                    &req_ptype);
            util_check("BMSG_GETREQINFO_", ferr);
            assert(req_ptype == info.type);
        }
        if (nocopy) {
            if (abandon)
                disable = msg_test_assert_disable();
            ferr = msg_buf_read_ctrl(sre.sre_msgId,
                                    &reqctrl,
                                    NULL,
                                    true);
            if (abandon)
                TEST_CHK_FEIGNORE(ferr);
            else
                TEST_CHK_FEOK(ferr);
            if (reqctrl != NULL)
                buf_free(reqctrl);
            ferr = msg_buf_read_data(sre.sre_msgId,
                                    &reqdata,
                                    NULL,
                                    true);
            if (abandon)
                TEST_CHK_FEIGNORE(ferr);
            else
                TEST_CHK_FEOK(ferr);
            if (reqdata != NULL)
                buf_free(reqdata);
            if (abandon)
                msg_test_assert_enable(disable);
        } else {
            if (abandon)
                disable = msg_test_assert_disable();
            ferr = BMSG_READCTRL_(sre.sre_msgId,  // msgid
                                  recv_buffer2,   // reqctrl
                                  1);             // bytecount
            if (abandon)
                TEST_CHK_FEIGNORE(ferr);
            else
                TEST_CHK_FEOK(ferr);
            ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  BUFSIZ);        // bytecount
            if (abandon)
                TEST_CHK_FEIGNORE(ferr);
            else
                TEST_CHK_FEOK(ferr);
            if (abandon)
                msg_test_assert_enable(disable);
        }
        if (verbose)
            printf("reply inx=%d\n", inx);
        BMSG_REPLY_(sre.sre_msgId,       // msgid
                    recv_buffer2,        // replyctrl
                    bidir ? csize : 0,   // replyctrlsize
                    recv_buffer,         // replydata
                    bidir ? dsize : 0,   // replydatasize
                    0,                   // errorclass
                    NULL);               // newphandle
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int                ferr;
    void              *result;
    int                status;
    SB_Thread::Thread *thr_cli;
    SB_Thread::Thread *thr_srv;
    TAD                zargs[] = {
      { "-abandon",   TA_Bool, TA_NOMAX,    &abandon   },
      { "-bidir",     TA_Bool, TA_NOMAX,    &bidir     },
      { "-bm",        TA_Bool, TA_NOMAX,    &bm        },
      { "-client",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-csize",     TA_Int,  MAX_CBUF,    &csize     },
      { "-dsize",     TA_Int,  MAX_DBUF,    &dsize     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-nocopy",    TA_Bool, TA_NOMAX,    &nocopy    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (nocopy) {
        ferr = msg_buf_register(buf_alloc, buf_free);
        TEST_CHK_FEOK(ferr);
    }
    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(true);
    ferr = msg_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);
    util_gethostname(my_name, sizeof(my_name));
    thr_srv = new SB_Thread::Thread(thread_srv_fun, "server");
    thr_srv->start();
    thr_cli = new SB_Thread::Thread(thread_cli_fun, "client");
    thr_cli->start();
    status = thr_cli->join(&result);
    TEST_CHK_STATUSOK(status);
    delete thr_cli;
    status = thr_srv->join(&result);
    TEST_CHK_STATUSOK(status);
    delete thr_srv;

    ferr = msg_mon_close_process(TPT_REF(phandle));
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_close();
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
