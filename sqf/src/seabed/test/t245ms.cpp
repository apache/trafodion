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
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "ml.h"
#include "tchkfe.h"
#include "tchkos.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum  { MAX_DATA  = 500 };
enum  { MAX_CTRL  = MAX_DATA/2 };
enum  { MAX_OUT   = 100 };
enum  { MAX_SLEEP = 1000 };
enum  { MAX_THR   = 50 };
enum  { MAX_SRES  = MAX_THR * MAX_OUT };

class MyThread : public SB_Thread::Thread {
public:
    MyThread(Function fun, const char *name, int pinx);
    virtual ~MyThread();

    int inx;
};

typedef struct  {
    SB_ML_Type     link;
    MS_SRE         sre;
} Test_SRE;

int                gargc;
char             **gargp;
int                gcnid;
char               gdisplay_name[MS_MON_MAX_PROCESS_NAME];
int                gdloop = MAX_OUT;
int                gdsize = MAX_DATA;
int                ginxs = 1;
int                gksleep = 0;
bool               gim_backup = false;
int                gloop = 10;
int                gmaxc = 1;
int                gmaxs = 1;
int                gmsgids[MAX_THR][MAX_OUT];
SB_Thread::Mutex   gmutex;
char               gname[MS_MON_MAX_PROCESS_NAME];
int                gnid = -1;
bool               gnodecycle = false;
int                gopen_count = 0;
TPT_DECL          (gphandle);
int                gpeer_nid = -1;
int                gpeer_oid;
TPT_DECL          (gpeer_phandle);
int                gpeer_pid = -1;
int                gpid = -1;
char               gprog[MS_MON_MAX_PROCESS_PATH];
short              grecv_cli_c_buffer[MAX_THR][MAX_OUT][MAX_CTRL];
char               grecv_cli_d_buffer[MAX_THR][MAX_OUT][MAX_DATA];
short              grecv_srv_c_buffer[MAX_OUT][MAX_CTRL];
char               grecv_srv_d_buffer[MAX_OUT][MAX_DATA];
unsigned int       gseed;
short              gsend_cli_c_buffer[MAX_THR][MAX_OUT][MAX_CTRL];
char               gsend_cli_d_buffer[MAX_THR][MAX_OUT][MAX_DATA];
short              gsend_srv_c_buffer[MAX_OUT][MAX_CTRL];
char               gsend_srv_d_buffer[MAX_OUT][MAX_DATA];
bool               gshutdown = false;
MS_SRE             gsre;
int                gsre_count = 0;
bool               gtakeover = false;
MyThread          *gthrc[MAX_THR];
SB_Thread::Thread *gthrs[MAX_THR];
bool               gverbose = false;
bool               gverbosepp = false;
bool               gvirtual = false;
SB_Sig_Queue       gwork_q((char *) "workQ", false);

MyThread::MyThread(Function fun, const char *name, int pinx) :
    SB_Thread::Thread(fun, name), inx(pinx) {
}

MyThread::~MyThread() {
}

// forwards
void kill_server();
void myassertfun(const char *exp, const char *file, unsigned line);
void node_cycle();
bool pp_is_backup();
void pp_pairinfo(bool pri);
void pp_printf(const char *format, ...);
int  pp_recv(void *buf, int *size);
void pp_shutdown();
void pp_start_backup(int nid);
bool pp_takeover();

#  ifdef NDEBUG
#    ifdef USE_ASSERT_ABORT
#      define myassert(exp) (void)((exp)||abort())
#    else
#      define myassert(exp) ((void)0)
#    endif
#  else
#    define myassert(exp) (void)((exp)||(myassertfun(#exp, __FILE__, __LINE__), 0))
#  endif  // NDEBUG

void *client_thr(void *arg) {
    int           dinx;
    int           ferr;
    static int    kinx = -1;
    int           lerr;
    int           req_ctrl_size;
    int           req_data_size;
    RT            results;
    MS_SRE_LDONE  sre_ldone;
    MyThread     *thr;
    int           tinx;

    thr = (MyThread *) arg;
    tinx = thr->inx;

    if (kinx < 0)
        kinx = gdloop - 1;
    for (dinx = 0; dinx < gdloop; dinx++) {
        req_ctrl_size = 0;
        req_data_size = gdsize;
        if (gverbose)
            printf("client LINK tinx=%d, dinx=%d\n", tinx, dinx);
        ferr = XMSG_LINK_(TPT_REF(gphandle),              // phandle
                          &gmsgids[tinx][dinx],           // msgid
                          gsend_cli_c_buffer[tinx][dinx], // reqctrl
                          (ushort) req_ctrl_size,         // reqctrlsize
                          grecv_cli_c_buffer[tinx][dinx], // replyctrl
                          (ushort) req_ctrl_size,         // replyctrlmax
                          gsend_cli_d_buffer[tinx][dinx], // reqdata
                          (ushort) req_data_size,         // reqdatasize
                          grecv_cli_d_buffer[tinx][dinx], // replydata
                          (ushort) req_data_size,         // replydatamax
                          0,                              // linkertag
                          0,                              // pri
                          0,                              // xmitclass
                          XMSG_LINK_LDONEQ);              // linkopts
        util_check("XMSG_LINK_", ferr);
        if ((tinx == 0) && (dinx == kinx)) {
            usleep(10000);
            if (gnodecycle && gvirtual)
                node_cycle();
            else
                kill_server();
            kinx--;
            if (kinx < 0)
                kinx = gdloop - 1;
        }
    }
    for (dinx = 0; dinx < gdloop; ) {
        lerr = XWAIT(LDONE, -1);
        TEST_CHK_WAITIGNORE(lerr);
        do {
            lerr = XMSG_LISTEN_((short *) &sre_ldone, // sre
                                XLISTEN_ALLOW_LDONEM, // listenopts
                                0);                   // listenertag
            if (lerr == XSRETYPE_LDONE) {
                if (gverbose)
                    printf("client LISTEN tinx=%d, dinx=%d\n", tinx, dinx);
                dinx++;
                ferr = XMSG_BREAK_(sre_ldone.sre_msgId,
                                   results.u.s,
                                   TPT_REF(gphandle));
                ferr = ferr; // ignore
            }
        } while (lerr);
    }
    return NULL;
}

void handle_sig(int, siginfo_t *, void *) {
    if (gverbosepp)
        pp_printf("handle-sig pid=%d\n", getpid());
    util_abort_core_free();
}

void kill_server() {
    int                       count;
    int                       err;
    int                       ferr;
    char                      killcmd[100];
    MS_Mon_Node_Info_Type     ninfo;
    bool                      pp;
    MS_Mon_Process_Info_Type *ppinfo1;
    MS_Mon_Process_Info_Type  pinfo1[2];
    MS_Mon_Process_Info_Type  pinfo2;

    do {
        pp = false;
        ferr = msg_mon_get_process_info_type(MS_ProcessType_TSE,     // ptype
                                             &count,                 // count
                                             2,                      // max
                                             pinfo1);                // info
        myassert(ferr == XZFIL_ERR_OK); // oops
        myassert(count > 0);
        if (count == 2)
            pp = true;
        else {
            printf("server is not up yet\n");
            usleep(10000); // 1/100 sec
        }
    } while (!pp);
    // pick primary
    if (pinfo1[0].backup)
        ppinfo1 = &pinfo1[1];
    else
        ppinfo1 = &pinfo1[0];
    assert(ferr == XZFIL_ERR_OK);
    ferr = msg_mon_get_node_info_detail(ppinfo1->nid, &ninfo);
    assert(ferr == XZFIL_ERR_OK);
    usleep(gksleep);
    if (gvirtual || (ppinfo1->nid == gcnid)) {
        do {
            if (gverbose)
                printf("kc=kill(%d, SIGURG)\n", pinfo1->pid);
            kill(ppinfo1->pid, SIGURG);
            usleep(1000);
            err = kill(ppinfo1->pid, 0);
            if (err == 0)
                sleep(1);
        } while (err == 0);
    } else {
        assert(ninfo.num_returned == 1);
        sprintf(killcmd, "pdsh -w %s kill -s URG %d",
                ninfo.node[0].node_name, ppinfo1->pid);
        do {
            if (gverbose)
                printf("kc=%s\n", killcmd);
            system(killcmd);
            ferr = msg_mon_get_process_info_detail((char *) "$srv", &pinfo2);
            if (ferr == XZFIL_ERR_OK) {
                if ((pinfo2.nid != ppinfo1->nid) && (pinfo2.pid != ppinfo1->pid))
                    ferr = XZFIL_ERR_NOSUCHDEV;
                else
                    sleep(1);
            }
        } while (ferr == XZFIL_ERR_OK);
    }
}

void myassertfun(const char *exp,
                 const char *file,
                 unsigned    line) {
    fprintf(stderr, "TEST Assertion failed (%d): %s, file %s, line %u\n",
            getpid(),
            exp, file, line);
    fflush(stderr);
    abort();
}

void wait_node_notice() {
    int    ferr;
    int    lerr;
    bool   node_change;
    MS_SRE sre;

    node_change = false;
    while (!node_change) {
        lerr = XWAIT(LREQ, -1);
        TEST_CHK_WAITIGNORE(lerr);
        do {
            lerr = XMSG_LISTEN_((short *) &sre,       // sre
                                XLISTEN_ALLOW_IREQM,  // listenopts
                                0);                   // listenertag
            if (lerr == XSRETYPE_IREQ) {
                bool mon_msg = (sre.sre_flags & XSRE_MON);
                if (gverbose)
                    printf("client LISTEN mon-msg=%d\n", mon_msg);
                assert(mon_msg);
                int msg_size = sre.sre_reqDataSize;
                char *msg = new char[msg_size];
                ferr = XMSG_READDATA_(sre.sre_msgId,      // msgid
                                      msg,                // reqdata
                                      (ushort) msg_size); // bytecount
                assert(ferr == XZFIL_ERR_OK);
                MS_Mon_Msg *msgp = (MS_Mon_Msg *) msg;

                switch (msgp->type) {
                case MS_MsgType_NodeDown:
                    if (gverbose)
                        printf("client mon-msg-type=node-down(%d)\n", msgp->type);
                    node_change = true;
                    break;
                case MS_MsgType_NodeUp:
                    if (gverbose)
                        printf("client mon-msg-type=node-up(%d)\n", msgp->type);
                    node_change = true;
                    break;
                case MS_MsgType_ProcessDeath:
                    if (gverbose)
                        printf("client mon-msg-type=proc-death(%d)\n", msgp->type);
                    break;
                default:
                    if (gverbose)
                        printf("client mon-msg-type=%d\n", msgp->type);
                    break;
                }
                XMSG_REPLY_(sre.sre_msgId,       // msgid
                            NULL,                // replyctrl
                            0,                   // replyctrlsize
                            NULL,                // replydata
                            0,                   // replydatasize
                            0,                   // errorclass
                            NULL);               // newphandle
                delete [] msg;
            }
        } while (lerr);
    }
}

void node_cycle() {
    int                       count;
    int                       ferr;
    MS_Mon_Node_Info_Type     ninfo;
    bool                      pp;
    MS_Mon_Process_Info_Type *ppinfo1;
    MS_Mon_Process_Info_Type  pinfo1[2];

    do {
        pp = false;
do {
        ferr = msg_mon_get_process_info_type(MS_ProcessType_TSE,     // ptype
                                             &count,                 // count
                                             2,                      // max
                                             pinfo1);                // info
        myassert(ferr == XZFIL_ERR_OK); // oops
if (count == 0) { sleep(1); printf("count is 0\n"); }
} while (count == 0);

        myassert(count > 0);
        if (count == 2)
            pp = true;
        else {
            printf("server is not up yet\n");
            usleep(10000); // 1/100 sec
        }
    } while (!pp);
    // pick primary
    if (pinfo1[0].backup)
        ppinfo1 = &pinfo1[1];
    else
        ppinfo1 = &pinfo1[0];
    assert(ferr == XZFIL_ERR_OK);
    ferr = msg_mon_get_node_info_detail(ppinfo1->nid, &ninfo);
    assert(ferr == XZFIL_ERR_OK);
    if (gverbose)
        printf("node-down(%d)\n", pinfo1->nid);
    msg_mon_node_down(pinfo1->nid);
usleep(100000);
//  wait_node_notice();
    if (gverbose)
        printf("node-up(%d)\n", pinfo1->nid);
    msg_mon_node_up(pinfo1->nid);
usleep(100000);
//  wait_node_notice();
}

void pp_check_mon_msg(char *mon_msg) {
    MS_Mon_Msg *msg = (MS_Mon_Msg *) mon_msg;

    switch (msg->type) {
    case MS_MsgType_Close:    // process close notification
        if (gverbosepp)
            pp_printf("close notice received from %s.\n",
                      msg->u.close.process_name);
        if (strcmp(gname, msg->u.close.process_name) != 0) {
            gopen_count--;
            if (gverbosepp)
                pp_printf("disconnecting from client\n");
        }
        break;
    case MS_MsgType_NodeDown: // node is down notification
        if (gverbosepp)
            pp_printf("node own notice not currently supported.\n");
        if (gpeer_nid == msg->u.down.nid)
            gtakeover = pp_takeover();
        break;
    case MS_MsgType_NodeUp:   // node is up notification
        if (gverbosepp)
            pp_printf("node Up notice not currently supported.\n");
        if (gpeer_nid == msg->u.up.nid)
            gtakeover = pp_takeover();
        break;
    case MS_MsgType_Open:     // process open notification
        if (gverbosepp)
            pp_printf("open notice received from %s.\n",
                 msg->u.open.target_process_name);
        if (strcmp(gname, msg->u.open.target_process_name) == 0) {
            gpeer_nid = msg->u.open.nid;
            gpeer_pid = msg->u.open.pid;
            if (gverbosepp)
                pp_printf("peer p-id=%d/%d\n", gpeer_nid, gpeer_pid);
            pp_pairinfo(false);
        }
        break;
    case MS_MsgType_ProcessDeath: // process death notification
        if (gverbosepp)
            pp_printf("processDeath notice received from p-id=%d/%d, peer-p-id=%d/%d.\n",
                       msg->u.death.nid, msg->u.death.pid, gpeer_nid, gpeer_pid);
        if ((gpeer_nid == msg->u.death.nid) &&
            (gpeer_pid == msg->u.death.pid))
            gtakeover = pp_takeover();
        break;
    case MS_MsgType_Shutdown: // shutdown notification
        if (gverbosepp)
            pp_printf("shutdown notice received.\n");
        gshutdown = true;
        break;
    default:
        if (gverbosepp)
            pp_printf("invalid notice message type received.\n");
        abort(); // oops
    }
}

void pp_check_mon_msg_shutdown(char *mon_msg) {
    MS_Mon_Msg *msg = (MS_Mon_Msg *) mon_msg;

    if (msg->type == MS_MsgType_Shutdown)
        gshutdown = true;
}

int pp_checkpoint(void *buf, int *size) {
    int   ferr;

    if (gim_backup) {
        if (gverbosepp)
            pp_printf("waiting for checkpoint.\n");
        ferr = pp_recv(buf, size);
    } else {
        int            msgid;
        MS_Result_Type results;
        if (gverbosepp)
            pp_printf("sending checkpoint.\n");
        ferr = XMSG_LINK_(TPT_REF(gpeer_phandle),               // phandle
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
                          0);                          // linkopts
        TEST_CHK_FEOK(ferr);
        // ignore checkpoint error
        ferr = XMSG_BREAK_(msgid, (short *) &results, TPT_REF(gpeer_phandle));
        TEST_CHK_FEIGNORE(ferr);
    }
    return ferr;
}

void pp_init() {
    int ferr;
    int len;
    int recvbuf[3];

    if (pp_is_backup()) {
        gim_backup = true;
        while (!gtakeover && !gshutdown) {
            len = sizeof(recvbuf);
            ferr = pp_checkpoint(recvbuf, &len);
            if (ferr == XZFIL_ERR_OK)
                XMSG_REPLY_(gsre.sre_msgId,    // msgid
                            NULL,              // replyctrl
                            0,                 // replyctrlsize
                            NULL,              // replydata
                            0,                 // replydatasize
                            0,                 // errorclass
                            NULL);             // newphandle
            if (gshutdown)
                pp_shutdown();
        }
        if (gverbosepp)
            pp_printf("the backup is now the primary process.\n");
        sprintf(gdisplay_name, "%s-%d-%d-%d-P", gname, gnid, gpid, getpid());
    } else {
        strcpy(gdisplay_name, gname);
        sprintf(&gdisplay_name[strlen(gdisplay_name)],
                "-%d-%d-%d-P", gnid, gpid, getpid());
        if (gverbosepp)
            pp_printf("we are the primary process.\n");
    }
    pp_start_backup(-1);
}

bool pp_is_backup() {
    bool                     backup;
    int                      disable;
    int                      ferr;
    MS_Mon_Node_Info_Type    node_info;
    MS_Mon_Process_Info_Type proc_info;

    ferr = msg_mon_get_process_info_detail(gname, &proc_info);
    TEST_CHK_FEOK(ferr);
    gnid = proc_info.nid;
    gpid = proc_info.pid;
    backup = proc_info.backup ? true : false;
    if (backup) {
        gpeer_nid = proc_info.parent_nid;
        gpeer_pid = proc_info.parent_pid;
        // set this up in case register fails
        strcpy(gdisplay_name, gname);
        sprintf(&gdisplay_name[strlen(gdisplay_name)],
                "-%d-%d-%d-B", gnid, gpid, getpid());
        if (gverbosepp)
            pp_printf("we are the backup process.\n");
        disable = msg_test_assert_disable();
        ferr = msg_mon_register_death_notification(gpeer_nid, gpeer_pid);
        if (ferr != XZFIL_ERR_OK) {
            if (gverbosepp)
                pp_printf("msg_mon_register_death_notification FAILED ferr=%d.\n", ferr);
            ferr = msg_mon_get_node_info_detail(gpeer_nid, &node_info);
            myassert(ferr == XZFIL_ERR_OK);
            if (gverbosepp)
                pp_printf("node-state=%d.\n", node_info.node[0].state);
            if (node_info.node[0].state == MS_Mon_State_Shutdown)
                gshutdown = true;
        }
        msg_test_assert_enable(disable);
    }
    return backup;
}

void pp_pairinfo(bool pri) {
    TPT_DECL        (bphandle);
    short            ferr;
    static bool      init = true;
    int              nid;
    char             pair[MS_MON_MAX_PROCESS_NAME+1];
    short            pair_length;
    static TPT_DECL (phandle);
    TPT_DECL        (pphandle);
    int              pid;

    if (init) {
        ferr = XPROCESSHANDLE_GETMINE_(TPT_REF(phandle));
        myassert(ferr == XZFIL_ERR_OK);
        init = false;
    }
    ferr = XPROCESS_GETPAIRINFO_(TPT_REF(phandle),           // phandle
                                 pair,
                                 MS_MON_MAX_PROCESS_NAME+1,  // maxlen
                                 &pair_length,
                                 TPT_REF(pphandle),          // primary_phandle
                                 TPT_REF(bphandle));         // backup_phandle
    if (pri)
        myassert(ferr == XPROC_PRIMARY); // caller is primary
    else
        myassert(ferr == XPROC_BACKUP);  // caller is backup
    pair[pair_length] = 0;
    myassert(strcmp(pair, "$SRV") == 0);
    myassert(XPROCESSHANDLE_COMPARE_(TPT_REF(pphandle),
                                     TPT_REF(bphandle)) == 1);
    ferr = XPROCESS_GETPAIRINFO_(TPT_REF(phandle),           // phandle
                                 pair,
                                 MS_MON_MAX_PROCESS_NAME+1,  // maxlen
                                 NULL,
                                 TPT_REF(pphandle),          // primary_phandle
                                 TPT_REF(bphandle));         // backup_phandle
    if (pri)
        myassert(ferr == XPROC_PRIMARY); // caller is primary
    else
        myassert(ferr == XPROC_BACKUP);  // caller is backup
    myassert(XPROCESSHANDLE_COMPARE_(TPT_REF(pphandle),
                                     TPT_REF(bphandle)) == 1);
    ferr = XPROCESS_GETPAIRINFO_(NULL,                       // phandle
                                 pair,
                                 MS_MON_MAX_PROCESS_NAME+1,  // maxlen
                                 NULL,
                                 TPT_REF(pphandle),          // primary_phandle
                                 TPT_REF(bphandle));         // backup_phandle
    if (pri)
        myassert(ferr == XPROC_PRIMARY); // caller is primary
    else
        myassert(ferr == XPROC_BACKUP);  // caller is backup
    myassert(XPROCESSHANDLE_COMPARE_(TPT_REF(pphandle),
                                     TPT_REF(bphandle)) == 1);
    ferr = XPROCESS_GETPAIRINFO_(NULL,                       // phandle
                                 pair,
                                 MS_MON_MAX_PROCESS_NAME+1,  // maxlen
                                 &pair_length,
                                 TPT_REF(pphandle),          // primary_phandle
                                 TPT_REF(bphandle));         // backup_phandle
    if (pri)
        myassert(ferr == XPROC_PRIMARY); // caller is primary
    else
        myassert(ferr == XPROC_BACKUP);  // caller is backup
    myassert(strcmp(pair, "$SRV") == 0);
    myassert(XPROCESSHANDLE_COMPARE_(TPT_REF(pphandle),
                                     TPT_REF(bphandle)) == 1);
    ferr = XPROCESSHANDLE_DECOMPOSE_(TPT_REF(pphandle), &nid, &pid);
    myassert(ferr == XZFIL_ERR_OK);
    if (pri) {
        myassert(nid == gnid);
        myassert(pid == gpid);
    } else {
        myassert(nid == gpeer_nid);
        myassert(pid == gpeer_pid);
    }
    ferr = XPROCESSHANDLE_DECOMPOSE_(TPT_REF(bphandle), &nid, &pid);
    myassert(ferr == XZFIL_ERR_OK);
    if (pri) {
        myassert(nid == gpeer_nid);
        myassert(pid == gpeer_pid);
    } else {
        myassert(nid == gnid);
        myassert(pid == gpid);
    }
}

void pp_printf(const char *format, ...) {
    va_list         ap;
    int             len;
    char            line[1000];
#ifdef TS
    int             ms;
    struct timeval  t;
    struct tm       tmbuf;
    struct tm      *tx;
    int             us;
#endif

    myassert(gverbosepp); // all pp_printf call's should check this
#ifdef TS
    gettimeofday(&t, NULL);
    tx = localtime_r(&t.tv_sec, tmbuf);
    ms = t.tv_usec / 1000;
    us = t.tv_usec - ms * 1000;
    sprintf(line, "%02d:%02d:%02d.%03d.%03d [%s] ",
            tx->tm_hour, tx->tm_min, tx->tm_sec, ms, us,
            gdisplay_name);
#else
    sprintf(line, "[%s] ", gdisplay_name);
#endif
    len = (int) strlen(line); // cast
    va_start(ap, format);
    vsprintf(&line[len], format, ap);
    va_end(ap);
    printf(line);
    fflush(stdout);
}

int pp_recv(void *buf, int *size) {
    int  *lbuf;
    int   ferr;
    int   lerr;
    bool  mon_msg;
    int   nid;
    int   pid;

    do {
        do {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = XMSG_LISTEN_((short *) &gsre,   // sre
                               0,                 // listenopts
                               0);                // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        mon_msg = (gsre.sre_flags & XSRE_MON);
        if (mon_msg) {
            int msg_size = gsre.sre_reqDataSize;
            char *msg = new char[msg_size];
            ferr = XMSG_READDATA_(gsre.sre_msgId,     // msgid
                                  msg,                // reqdata
                                  (ushort) msg_size); // bytecount
            assert(ferr == XZFIL_ERR_OK);
            pp_check_mon_msg(msg);
            pp_check_mon_msg_shutdown(msg);
            XMSG_REPLY_(gsre.sre_msgId,      // msgid
                        NULL,                // replyctrl
                        0,                   // replyctrlsize
                        NULL,                // replydata
                        0,                   // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
            delete [] msg;
            if (gshutdown)
                return 1;
            if (gtakeover)
                return 2;
            if (gshutdown)
                pp_shutdown();
        } else {
            ferr = XMSG_READDATA_(gsre.sre_msgId,   // msgid
                                  (char *) buf,     // reqctrl
                                  (ushort) *size);  // bytecount
            assert(ferr == XZFIL_ERR_OK);
            *size = gsre.sre_reqDataSize;
            ferr = XMSG_GETREQINFO_(MSGINFO_NID,
                                    gsre.sre_msgId,
                                    &nid);
            assert(ferr == XZFIL_ERR_OK);
            ferr = XMSG_GETREQINFO_(MSGINFO_PID,
                                    gsre.sre_msgId,
                                    &pid);
            assert(ferr == XZFIL_ERR_OK);
            lbuf = (int *) buf;
            if (gverbosepp)
                pp_printf("received from p-id=%d/%d: %d.%d.%d\n",
                          nid, pid, lbuf[0], lbuf[1], lbuf[2]);
        }
    } while (mon_msg);
    return 0;
}

void pp_shutdown() {
    int ferr;

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    exit(0);
}

void pp_start_backup(int nid) {
    int ferr;

    if (gshutdown) {
        if (gverbosepp)
            pp_printf("NOT starting backup process - shutdown.\n");
        return;
    }

    nid = nid; // no-warn
    ferr = msg_mon_get_process_info(NULL, &nid, &gpeer_pid);
    TEST_CHK_FEOK(ferr);
for (;;) {
    gpeer_nid = -1;
if (gnid == 1)
gpeer_nid = 2;
else
gpeer_nid = 1;
    gpeer_pid = -1;
    if (gverbosepp)
        pp_printf("starting backup process with open, nid=%d.\n", gpeer_nid);
    ferr = msg_mon_start_process(gprog,                  // prog
                                 gname,                  // name
                                 NULL,                   // ret name
                                 gargc,
                                 gargp,
                                 TPT_REF(gpeer_phandle),
                                 0,                      // open
                                 &gpeer_oid,
                                 MS_ProcessType_TSE,     // type
                                 0,                      // priority
                                 0,                      // debug
                                 1,                      // backup
                                 &gpeer_nid,             // nid
                                 &gpeer_pid,             // pid
                                 NULL,                   // infile
                                 NULL);                  // outfile
if (ferr == XZFIL_ERR_OK)
break;
printf("backup did not start error=%d\n", ferr);
sleep(1);
}
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_open_process_backup(gname,
                                       TPT_REF(gpeer_phandle),
                                       &gpeer_oid);
    TEST_CHK_FEOK(ferr);
    if (gverbosepp)
        pp_printf("after start_backup - peer p-id=%d/%d\n",
                  gpeer_nid, gpeer_pid);
    pp_pairinfo(true);
}

bool pp_takeover() {
    bool activate = false;

    if (gshutdown) {
    } else if (gim_backup) {
        if (gverbosepp)
            pp_printf("mark primary down, takeover and start backup.\n");
        gpeer_nid = -1;
        gpeer_pid = -1;
        gim_backup = false;
        sprintf(gdisplay_name, "%s-%d-%d-%d-P(B)", gname, gnid, gpid, getpid());
        activate = true;
    } else {
        if (gverbosepp)
            pp_printf("mark backup down and restart backup.\n");
        pp_start_backup(-1);
    }

    return activate;
}

void server(int whoami, Test_SRE *sre) {
    int   len;
    char  recv_buffer[MAX_DATA];
    short recv_buffer2[MAX_DATA];

    if (gverbose)
        printf("s-%d: received/sending msgid=%d\n", whoami, sre->sre.sre_msgId);
    len = MAX_DATA;
    XMSG_REPLY_(sre->sre.sre_msgId,          // msgid
                recv_buffer2,                // replyctrl
                (ushort) len,                // replyctrlsize
                recv_buffer,                 // replydata
                (ushort) len,                // replydatasize
                0,                           // errorclass
                NULL);                       // newphandle
}

void *server_thr(void *arg) {
    int       ferr;
    int       lerr;
    int       max_msgs = gmaxc * gloop;
    bool      mon_msg;
    Test_SRE  sre;
    int       status;
    int       whoami = ginxs++;

    arg = arg; // touch
    for (;;) {
        if (gsre_count >= max_msgs)
            break;
        if (gshutdown)
            break;
        do {
            lerr = XWAIT(LREQ, 100);
            lerr = XMSG_LISTEN_((short *) &sre.sre,  // sre
                                0,                   // listenopts
                                0);                  // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        mon_msg = (sre.sre.sre_flags & XSRE_MON);
        if (mon_msg) {
            int msg_size = sre.sre.sre_reqDataSize;
            char *msg = new char[msg_size];
            ferr = XMSG_READDATA_(sre.sre.sre_msgId,     // msgid
                                  msg,                   // reqdata
                                  (ushort) msg_size);    // bytecount
            assert(ferr == XZFIL_ERR_OK);
            pp_check_mon_msg(msg);
            pp_check_mon_msg_shutdown(msg);
            XMSG_REPLY_(sre.sre.sre_msgId,   // msgid
                        NULL,                // replyctrl
                        0,                   // replyctrlsize
                        NULL,                // replydata
                        0,                   // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
            delete [] msg;
            if (gshutdown)
                break;
        } else {
            status = gmutex.lock();
            TEST_CHK_STATUSOK(status);
            gsre_count++;
            status = gmutex.unlock();
            TEST_CHK_STATUSOK(status);
            if (gverbose)
                printf("s-%d: have work, sre-count=%d\n",
                       whoami, gsre_count);
            server(whoami, &sre);
        }
    }
    return NULL;
}

void setup_sig() {
    struct sigaction act;
    sigset_t         sig_set;

    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGURG);
    act.sa_sigaction = handle_sig;
    sigaction(SIGURG, &act, NULL);
}

void start_server(int argc, char **argv) {
    int         arg;
    char        device[MS_MON_MAX_PROCESS_PATH+10];
    int         disable;
    bool        done;
    int         ferr;
    char       *nodes;
    char        prog[MS_MON_MAX_PROCESS_PATH];
    char        server_name[10];
    int         server_nid;
    int         server_pid;
    const char *z0;
    const char *z1;

    strcpy(server_name, "$srv");
    nodes = getenv("SQ_VIRTUAL_NODES");
    if ((nodes != NULL) && (atoi(nodes) > 2)) {
        z0 = "1";
        z1 = "2";
    } else {
        z0 = "0";
        z1 = "1";
    }
    ferr = msg_mon_reg_set(MS_Mon_ConfigType_Process,    // type
                    server_name,                         // group
                    (char *) "ZONE",                     // key
                    (char *) z0);                        // value
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_reg_set(MS_Mon_ConfigType_Process,    // type
                    server_name,                         // group
                    (char *) "ZONE-B",                   // key
                    (char *) z1);                        // value
    TEST_CHK_FEOK(ferr);
    sprintf(device, "%s/%s", getenv("PWD"), server_name);
    ferr = msg_mon_reg_set(MS_Mon_ConfigType_Process,    // type
                    server_name,                         // group
                    (char *) "DEVICE",                   // key
                    device);                             // value
    TEST_CHK_FEOK(ferr);
    sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
    for (arg = 0; arg < argc; arg++)
        if (strcmp(argv[arg], "-client") == 0) // start_process
            argv[arg] = (char *) "-server";
    // TODO: remove disable/loop
    disable = msg_test_assert_disable();
    do {
        done = true;
        server_nid = -1;
        ferr = msg_mon_start_process(prog,                   // prog
                                     server_name,            // name
                                     NULL,                   // ret name
                                     argc,
                                     argv,
                                     NULL,                   // phandle
                                     0,                      // open
                                     NULL,                   // oid
                                     MS_ProcessType_TSE,     // type
                                     0,                      // priority
                                     0,                      // debug
                                     0,                      // backup
                                     &server_nid,            // nid
                                     &server_pid,            // pid
                                     NULL,                   // infile
                                     NULL);                  // outfile
        if (ferr == XZFIL_ERR_BOUNDSERR) {
            printf("TODO: remove this sleep - $srv already in use\n");
            sleep(1);
            done = false;
        } else if (ferr == XZFIL_ERR_FSERR) {
            printf("TODO: remove this sleep - $srv already in use\n");
            sleep(1);
            done = false;
        }
    } while (!done);
    TEST_CHK_FEOK(ferr);
    msg_test_assert_enable(disable);
}

int main(int argc, char *argv[]) {
    bool          chook = false;
    int           client = false;
    int           dinx;
    int           disable;
    int           ferr;
    int           inx;
    char          lname[10];
    bool          mq = false;
    int           oid;
    void         *res;
    bool          rnd = false;
    int           sleept;
    int           snid;
    int           spid;
    int           status;
    TAD           zargs[] =  {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook      },
      { "-client",    TA_Bool, TA_NOMAX,    &client     },
      { "-dloop",     TA_Int,  MAX_OUT,     &gdloop     },
      { "-dsize",     TA_Int,  TA_NOMAX,    &gdsize     },
      { "-ksleep",    TA_Int,  TA_NOMAX,    &gksleep    },
      { "-loop",      TA_Int,  TA_NOMAX,    &gloop      },
      { "-maxc",      TA_Int,  MAX_THR,     &gmaxc      },
      { "-maxs",      TA_Int,  MAX_THR,     &gmaxs      },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq         },
      { "-nodecycle", TA_Bool, TA_NOMAX,    &gnodecycle },
      { "-rnd",       TA_Bool, TA_NOMAX,    &rnd        },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL        },
      { "-v",         TA_Bool, TA_NOMAX,    &gverbose   },
      { "-vpp",       TA_Bool, TA_NOMAX,    &gverbosepp },
      { "",           TA_End,  TA_NOMAX,    NULL        }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (client && chook)
        test_debug_hook("c", "c");
    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(client);
    if (!client)
        setup_sig();
    ferr = msg_mon_process_startup(true);             // system messages
    TEST_CHK_FEOK(ferr);
    gargc = argc; // after msg_init - it removes args
    gargp = argv;
    sprintf(gprog, "%s/%s", getenv("PWD"), argv[0]);
    gvirtual = (getenv("SQ_VIRTUAL_NODES") != NULL);
    ferr = msg_mon_get_my_process_name(gname, sizeof(gname));
    assert(ferr == XZFIL_ERR_OK);
    if (client) {
        msg_mon_enable_mon_messages(true); // get mon messages
        ferr = msg_mon_get_process_info(NULL, &gcnid, NULL);
        assert(ferr == XZFIL_ERR_OK);
        ferr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETSENDLIMIT,
                                     (short) (gmaxc * gdloop));
        assert(ferr == XZFIL_ERR_OK);
        srand(1);
        sleept = 1000;
        start_server(argc, argv);
        sleep(1);
        ferr = msg_mon_open_process((char *) "$srv",              // name
                                    TPT_REF(gphandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
        ferr = XPROCESSHANDLE_DECOMPOSE_(TPT_REF(gphandle), &snid, &spid);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < gloop; inx++) {
            if (mq)
                printf("client: inx=%d\n", inx);
            for (dinx = 0; dinx < gmaxc; dinx++) {
                sprintf(lname, "c%d", dinx);
                gthrc[dinx] = new MyThread(client_thr, lname, dinx);
            }
            disable = msg_test_assert_disable();
            for (dinx = 0; dinx < gmaxc; dinx++)
                gthrc[dinx]->start();
            if (rnd)
                sleept = (int) ((float) gksleep * (rand_r(&gseed) / (RAND_MAX + 1.0)));
            usleep(sleept);
            for (dinx = 0; dinx < gmaxc; dinx++) {
                status = gthrc[dinx]->join(&res);
                TEST_CHK_STATUSOK(status);
                if (gverbose)
                    printf("joined with client %d\n", dinx);
            }
            for (dinx = 0; dinx < gmaxc; dinx++)
                delete gthrc[dinx];
            msg_test_assert_enable(disable);
        }
    } else {
        msg_mon_enable_mon_messages(true); // get mon messages
        ferr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETRECVLIMIT,
                                     XMAX_SETTABLE_RECVLIMIT);
        assert(ferr == XZFIL_ERR_OK);
        pp_init();
        if (!gshutdown) {
            for (dinx = 0; dinx < gmaxs; dinx++) {
                char lname[10];
                sprintf(lname, "s%d", dinx);
                gthrs[dinx] = new SB_Thread::Thread(server_thr, lname);
            }
            for (dinx = 0; dinx < gmaxs; dinx++)
                gthrs[dinx]->start();
            for (dinx = 0; dinx < gmaxs; dinx++) {
                status = gthrs[dinx]->join(&res);
                TEST_CHK_STATUSOK(status);
                if (gverbose)
                    printf("joined with server %d\n", dinx);
            }
        }
        if (gverbosepp)
            pp_printf("exit pid=%d\n", getpid());
    }

    if (client) {
        sleep(1);
        ferr = msg_mon_close_process(TPT_REF(gphandle));
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
