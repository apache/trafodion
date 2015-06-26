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
#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/thread.h"

#include "ml.h"
#include "queue.h"
#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

typedef struct Link {
    SB_ML_Type  iv_link;
    char       *ip_buf;
    int         iv_msgid;
} Link;

bool               im_backup = false;
int                my_argc;
char             **my_argp;
char               my_name[BUFSIZ];
int                my_nid = -1;
int                my_pid = -1;
int                peer_nid = -1;
int                peer_oid;
TPT_DECL          (peer_phandle);
int                peer_pid = -1;
char               prog[MS_MON_MAX_PROCESS_PATH];
char               recv_buffer[40000];
SB_Thread::Thread *thr_recv;
char               send_buffer[40000];
bool               shutdown_rcvd = false;
SB_Sig_Queue       sigq("sigq", false);
bool               takeoverv = false;
int                trip = 0;
bool               verbose = false;

// forwards
static int checkpoint(MS_SRE *sre, void *buf, int *size);
static void process_request(int msgid, char *recvbuf);
static int  recv(MS_SRE *sre, void *buf, int *size);
static void start_backup(int nid);

#  ifdef NDEBUG
#    ifdef USE_ASSERT_ABORT
#      define myassert(exp) (void)((exp)||abort())
#    else
#      define myassert(exp) ((void)0)
#    endif
#  else
#    define myassert(exp) (void)((exp)||(myassertfun(#exp, __FILE__, __LINE__), 0))
#  endif  // NDEBUG
void myassertfun(const char *exp,
                 const char *file,
                 unsigned    line) {
    fprintf(stderr, "TEST Assertion failed (%d): %s, file %s, line %u\n",
            getpid(),
            exp, file, line);
    fflush(stderr);
    abort();
}

void *thread_recv(void *arg) {
    char   *buf;
    Link   *l;
    int     len;
    int     msgid;
    MS_SRE  sre;

    arg = arg; // touch

    for (;;) {
        l = (Link *) sigq.remove();
        buf = l->ip_buf;
        msgid = l->iv_msgid;
        delete l;
        if (buf == NULL)
            break;
        process_request(msgid, buf);
        len = 0;
        checkpoint(&sre, buf, &len);
    }
    return NULL;
}

void *thread_recv_fun(void *arg) {
    return thread_recv(arg);
}


int checkpoint(MS_SRE *sre, void *buf, int *size) {
    int   disable;
    int   ferr;
    int   lerr;

    if (im_backup) {
        if (verbose)
            printf("srv-b: Waiting for checkpoint.\n");
        if (recv(sre, buf, size))
            ferr = XZFIL_ERR_PATHDOWN;
        else
            ferr = XZFIL_ERR_OK;
    } else {
        int            msgid;
        MS_Result_Type results;
        if (verbose)
            printf("srv-p: Sending checkpoint.\n");
        disable = msg_test_assert_disable();
        ferr = XMSG_LINK_(TPT_REF(peer_phandle),       // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          NULL,                        // replyctrl
                          0,                           // replyctrlmax
                          (char *) buf,                // reqdata
                          (ushort) *size,              // reqdatasize
                          NULL,                        // replydata
                          0,                           // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          XMSG_LINK_LDONEQ);           // linkopts
        msg_test_assert_enable(disable);
        // ignore checkpoint error
        TEST_CHK_FEIGNORE(ferr);
        lerr = XWAIT(LDONE, -1);
        TEST_CHK_WAITIGNORE(lerr);
        lerr = XMSG_LISTEN_((short *) sre,          // sre
                           XLISTEN_ALLOW_LDONEM,    // listenopts
                           0);                      // listenertag
        TEST_CHK_WAITIGNORE(lerr);
        disable = msg_test_assert_disable();
        ferr = XMSG_BREAK_(sre->sre_msgId,
                           (short *) &results,
                           TPT_REF(peer_phandle));
        msg_test_assert_enable(disable);
        TEST_CHK_FEIGNORE(ferr);
        if (ferr != XZFIL_ERR_OK)
            start_backup(-1);
    }
    return ferr;
}

bool is_backup() {
    int                      ferr;
    MS_Mon_Process_Info_Type proc_info;

    ferr = msg_mon_get_process_info_detail(my_name, &proc_info);
    TEST_CHK_FEOK(ferr);
    my_nid = proc_info.nid;
    my_pid = proc_info.pid;
    bool backup = proc_info.backup ? true : false;
    if (backup) {
        ferr = msg_mon_register_death_notification(proc_info.parent_nid,
                                                   proc_info.parent_pid);
        assert(ferr == XZFIL_ERR_OK);
    }
    return backup;
}

void process_request(int msgid, char *recvbuf) {
    static int trip_cnt = 0;

    recvbuf = recvbuf; // touch
    if (im_backup) {
        if (verbose)
            printf("srv-b: processing checkpointed data\n");
        if (trip) {
            trip_cnt++;
            if (trip_cnt >= trip)
                util_abort_core_free();
        }
    } else {
        if (verbose)
            printf("srv-p: processed client request\n");
    }
    if (verbose)
        printf("srv-%c: sending reply\n", im_backup ? 'b' : 'p');
    XMSG_REPLY_(msgid,             // msgid
                NULL,              // replyctrl
                0,                 // replyctrlsize
                NULL,              // replydata
                0,                 // replydatasize
                0,                 // errorclass
                NULL);             // newphandle

}

void queue_process_request(char *recvbuf, int msgid) {
    Link *l = new Link;
    l->ip_buf = recvbuf;
    l->iv_msgid = msgid;
    sigq.add(&l->iv_link);
}

void queue_stop() {
    Link *l = new Link;
    l->ip_buf = NULL;
    sigq.add(&l->iv_link);
}

int recv(MS_SRE *sre, void *buf, int *size) {
    int    *lbuf;
    int     ferr;
    int     lerr;
    bool    mon_msg;
    int     nid;
    int     pid;

    do {
        do {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = XMSG_LISTEN_((short *) sre,    // sre
                               0,                 // listenopts
                               0);                // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        mon_msg = (sre->sre_flags & XSRE_MON);
        if (mon_msg) {
            int msg_size = sre->sre_reqDataSize;
            char *msg = new char[msg_size];
            ferr = XMSG_READDATA_(sre->sre_msgId,     // msgid
                                  msg,                // reqdata
                                  (ushort) msg_size); // bytecount
            assert(ferr == XZFIL_ERR_OK);
            MS_Mon_Msg *mon_msgp = (MS_Mon_Msg *) msg;
            if (mon_msgp->type == MS_MsgType_Shutdown) {
                shutdown_rcvd = true;
                if (verbose)
                    printf("srv-%c: shutdown rcvd\n", im_backup ? 'b' : 'p');
            }
            XMSG_REPLY_(sre->sre_msgId,      // msgid
                        NULL,                // replyctrl
                        0,                   // replyctrlsize
                        NULL,                // replydata
                        0,                   // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
            delete [] msg;
            if (takeoverv)
                return 1;
            if (shutdown_rcvd)
                return 2;
        } else {
            ferr = XMSG_READDATA_(sre->sre_msgId,   // msgid
                                  (char *) buf,     // reqctrl
                                  (ushort) *size);  // bytecount
            assert(ferr == XZFIL_ERR_OK);
            *size = sre->sre_reqDataSize;
            ferr = XMSG_GETREQINFO_(MSGINFO_NID,
                                    sre->sre_msgId,
                                    &nid);
            assert(ferr == XZFIL_ERR_OK);
            ferr = XMSG_GETREQINFO_(MSGINFO_PID,
                                    sre->sre_msgId,
                                    &pid);
            assert(ferr == XZFIL_ERR_OK);
            lbuf = (int *) buf;
            if (verbose)
                printf("srv-%c: received from p-id=%d/%d: %d.%d.%d\n",
                       im_backup ? 'b' : 'p',
                       nid, pid, lbuf[0], lbuf[1], lbuf[2]);
        }
    } while (mon_msg);
    return 0;
}

void start_backup(int nid) {
    int ferr;

    nid = nid; // no-warn
    ferr = msg_mon_get_process_info(NULL, &nid, &peer_pid);
    TEST_CHK_FEOK(ferr);
    peer_nid = 1 - nid;
    peer_pid = -1;
    if (verbose)
        printf("srv-p: starting backup process with open.\n");
    ferr = msg_mon_start_process(prog,                   // prog
                                 my_name,                // name
                                 NULL,                   // ret name
                                 my_argc,
                                 my_argp,
                                 TPT_REF(peer_phandle),
                                 0,                      // open
                                 &peer_oid,
                                 MS_ProcessType_Generic, // type
                                 0,                      // priority
                                 0,                      // debug
                                 1,                      // backup
                                 &peer_nid,              // nid
                                 &peer_pid,              // pid
                                 NULL,                   // infile
                                 NULL);                  // outfile
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_open_process_backup(my_name,
                                       TPT_REF(peer_phandle),
                                       &peer_oid);
    TEST_CHK_FEOK(ferr);
    if (verbose)
        printf("srv-p: after start_backup - peer p-id=%d/%d\n",
               peer_nid, peer_pid);
}

int main(int argc, char *argv[]) {
    bool       client = false;
    int        err;
    int        ferr;
    int        inx;
    int        len;
    int        loop = 10;
    int        msgid;
    int        oid;
    TPT_DECL  (phandle);
    void      *result;
    RT         results;
    MS_SRE     sre;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-trip",      TA_Int,  TA_NOMAX,    &trip      },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    my_argc = argc; // after msg_init - it removes args
    my_argp = argv;
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_get_my_process_name(my_name, sizeof(my_name));
    assert(ferr == XZFIL_ERR_OK);

    for (inx = 0; inx < loop; inx++) {
        if (client) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              NULL,                        // reqctrl
                              0,                           // reqctrlsize
                              NULL,                        // replyctrl
                              0,                           // replyctrlmax
                              send_buffer,                 // reqdata
                              39000,                       // reqdatasize
                              recv_buffer,                 // replydata
                              40000,                       // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == 0);
        } else {
            sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
            msg_mon_enable_mon_messages(true); // get mon messages
            thr_recv = new SB_Thread::Thread(thread_recv_fun, "recv");
            thr_recv->start();
            if (is_backup()) {
                im_backup = true;
                while (!takeoverv) {
                    len = sizeof(recv_buffer);
                    ferr = checkpoint(&sre, recv_buffer, &len);
                    if (ferr == XZFIL_ERR_OK)
                        process_request(sre.sre_msgId, recv_buffer);
                    else if (shutdown_rcvd)
                        break;
                }
            }
            if (!shutdown_rcvd) {
                start_backup(-1);
                for (inx = 0; inx < loop; inx++) {
                    len = sizeof(recv_buffer);
                    if (!recv(&sre, recv_buffer, &len)) {
                        queue_process_request(recv_buffer, sre.sre_msgId);
                    }
                }
            }
            queue_stop();
            err = thr_recv->join(&result);
            assert(err == 0);
            delete thr_recv;
        }
    }
    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
