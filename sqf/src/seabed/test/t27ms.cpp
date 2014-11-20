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
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

class Dirty_Thread : public SB_Thread::Thread {
public:
    Dirty_Thread(Function fun, const char *name)
    : SB_Thread::Thread(fun, name), ifin(false) {
    }
    void fin() {
        ifin = true;
    }
    void run();

private:
    bool ifin;
};

class Server_Thread : public SB_Thread::Thread {
public:
    Server_Thread(Function fun, const char *name)
    : SB_Thread::Thread(fun, name), ifin(false) {
    }
    void fin() {
        ifin = true;
    }
    void run();

private:
    bool ifin;
};

enum  { MAX_CBUF = 1024 * 32 };        // 32 KB
enum  { MAX_DBUF = 1024 * 1024 };      // 1 MB


void *buf_alloc(size_t len) {
    return malloc(len);
}

void  buf_free(void *buf) {
    free(buf);
}

int   csize = 0;
int   dsize = 1024;
bool  bidir = false;
int   loop = 10;
bool  nocopy = false;
char  recv_buffer[MAX_DBUF];
short recv_buffer2[MAX_CBUF/2];
char  send_buffer[MAX_DBUF];
short send_buffer2[MAX_CBUF/2];

// forwards
void server_duty();

void Dirty_Thread::run() {
    long long *ptr = (long long *) recv_buffer;
    while (__sync_fetch_and_add_1(&ifin, 0) == 0) { // memory barrier
        usleep(100);
        *ptr = *ptr + 1;
    }
}

void *dirty_fun(void *pp_arg) {
    Dirty_Thread *lp_thr = (Dirty_Thread *) pp_arg; // cast
    lp_thr->run();
    return NULL;
}

void Server_Thread::run() {
    server_duty();
}

void *server_fun(void *pp_arg) {
    Server_Thread *lp_thr = (Server_Thread *) pp_arg; // cast
    lp_thr->run();
    return NULL;
}

void server_duty() {
    int      ferr;
    int      inx;
    int      lerr;
    short   *reqctrl;
    char    *reqdata;
    BMS_SRE  sre;

    for (inx = 0; inx < loop; inx++) {
        do {
            lerr = XWAITNO0(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = BMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        if (nocopy) {
            ferr = msg_buf_read_ctrl(sre.sre_msgId,
                                    &reqctrl,
                                    NULL,
                                    true);
            TEST_CHK_FEOK(ferr);
            buf_free(reqctrl);
            ferr = msg_buf_read_data(sre.sre_msgId,
                                    &reqdata,
                                    NULL,
                                    true);
            TEST_CHK_FEOK(ferr);
            buf_free(reqdata);
        } else {
            ferr = BMSG_READCTRL_(sre.sre_msgId,  // msgid
                                  recv_buffer2,   // reqctrl
                                  1);             // bytecount
            util_check("BMSG_READCTRL_", ferr);
            ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  BUFSIZ);        // bytecount
            util_check("BMSG_READDATA_", ferr);
        }
        BMSG_REPLY_(sre.sre_msgId,       // msgid
                    recv_buffer2,        // replyctrl
                    bidir ? csize : 0,   // replyctrlsize
                    recv_buffer,         // replydata
                    bidir ? dsize : 0,   // replydatasize
                    0,                   // errorclass
                    NULL);               // newphandle
    }
}

int main(int argc, char *argv[]) {
    bool            bm = false;
    double          busy;
    bool            client = false;
    bool            dirty = false;
    Dirty_Thread   *dirty_thr = NULL;
    int             ferr;
    int             inx;
    int             msgid;
    int             oid;
    TPT_DECL       (phandle);
    struct rusage   r_start;
    struct rusage   r_stop;
    MS_Result_Type  results;
    bool            self = false;
    Server_Thread  *server_thr = NULL;
    int             statcount = 0;
    int             status;
    struct timeval  t_elapsed;
    struct timeval  t_elapsed_total;
    struct timeval  t_start;
    struct timeval  t_start_total;
    struct timeval  t_stop;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-bidir",     TA_Bool, TA_NOMAX,    &bidir     },
      { "-bm",        TA_Bool, TA_NOMAX,    &bm        },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-csize",     TA_Int,  MAX_CBUF,    &csize     },
      { "-dirty",     TA_Bool, TA_NOMAX,    &dirty     },
      { "-dsize",     TA_Int,  MAX_DBUF,    &dsize     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-nocopy",    TA_Bool, TA_NOMAX,    &nocopy    },
      { "-self",      TA_Bool, TA_NOMAX,    &self      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-stats",     TA_Int,  TA_NOMAX,    &statcount },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (nocopy) {
        ferr = msg_buf_register(buf_alloc, buf_free);
        TEST_CHK_FEOK(ferr);
    }
    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    util_time_timer_start(&t_start_total);
    if (client) {
        printf("csize=%d, dsize=%d, loop=%d\n", csize, dsize, loop);
        if (self)
            ferr = msg_mon_open_process_self(TPT_REF(phandle), &oid);
        else
            ferr = msg_mon_open_process((char *) "$srv",      // name
                                        TPT_REF(phandle),
                                        &oid);
        TEST_CHK_FEOK(ferr);
    }

    util_time_timer_start(&t_start);
    util_cpu_timer_start(&r_start);
    if (client) {
        if (self) {
            server_thr = new Server_Thread(server_fun, "server");
            server_thr->start();
        }
        for (inx = 0; inx < loop; inx++) {
            if (verbose)
                printf("count=%d\n", inx);
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
            util_check("BMSG_LINK_", ferr);
            ferr = BMSG_BREAK_(msgid, (short *) &results, TPT_REF(phandle));
            util_check("BMSG_BREAK_", ferr);
            if ((statcount > 0) && (inx > 0) && ((inx % statcount) == 0)) {
                util_cpu_timer_stop(&r_stop);
                util_time_timer_stop(&t_stop);
                util_time_elapsed(&t_start, &t_stop, &t_elapsed);
                util_cpu_timer_busy(&r_start, &r_stop, &t_elapsed, &busy);
                print_rate(bm, "", bidir ? 2 * statcount : statcount,
                           dsize, &t_elapsed, busy);
                fflush(stdout);
                util_time_timer_start(&t_start);
                util_cpu_timer_start(&r_start);
            }
        }
    } else {
        if (dirty) {
            dirty_thr = new Dirty_Thread(dirty_fun, "dirty");
            dirty_thr->start();
        }
        if (!self)
            server_duty();
        if (dirty_thr != NULL) {
            dirty_thr->fin();
            void *result;
            status = dirty_thr->join(&result);
            assert(status == 0);
        }
    }
    util_cpu_timer_stop(&r_stop);
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start_total, &t_stop, &t_elapsed_total);
    util_time_elapsed(&t_start, &t_stop, &t_elapsed);
    util_cpu_timer_busy(&r_start, &r_stop, &t_elapsed, &busy);

    if (client) {
        if (!bm) {
            print_elapsed("", &t_elapsed_total);
            print_elapsed(" (data)", &t_elapsed);
        }
        print_rate(bm, "", bidir ? 2 * loop : loop, dsize, &t_elapsed, busy);
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    } else
        print_server_busy(bm, "", busy);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
