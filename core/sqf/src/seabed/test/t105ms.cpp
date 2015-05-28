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

#include "imap.h"

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

enum { MAX_SERVERS = 3 };
enum { MAX_THR     = 5 };
enum { TAG0        = 39 };
enum { TAG1        = 3 };
//enum { TAG2        = 103 };
enum { TAG2        = 0 };

typedef struct {
    SB_ML_Type   link;                // MUST be first
    int          handle;
} Item;

SB_Ts_Imap         handle_issue_map;
char               pname[BUFSIZ];
bool               quiet = false;
int                snid;
int                spid;
const char        *sync_data = "test-data";
bool               sync_rcvd = false;
SB_Thread::Thread *sync_thr[MAX_THR];
bool               sync_thr_shutdown = false;
int                tag;
SB_Ts_Imap         tag_issue_map;


void do_sync() {
    for (int inx = 0; inx < 10; inx++) {
        char sync_data_act[BUFSIZ];
        sprintf(sync_data_act, "%s-%d", sync_data, inx);
        int len = (int) strlen(sync_data_act) + 1; // cast
        if (!quiet)
            printf("server (%s) about to issue sync nid=%d, len=%d, tag=%d\n",
                   pname, snid, len, tag + inx);
        int handle;
        int ferr = msg_mon_tmsync_issue(sync_data_act,
                                        len,
                                        &handle,
                                        tag + inx);
        util_check("msg_mon_tmsync_issue", ferr);
        Item *item = new Item();
        item->link.iv_id.i = handle;
        handle_issue_map.put(&item->link);
        item = new Item();
        item->link.iv_id.i = tag + inx;
        item->handle = handle;
        tag_issue_map.put(&item->link);
        if (!quiet)
            printf("server (%s) sync returned handle=%d\n",
                   pname, handle);
    }
}

void *do_sync_fun(void *) {
    while (!sync_thr_shutdown) {
        do_sync();
    }
    return NULL;
}

void do_sync_threaded() {
    for (int thr = 0; thr < MAX_THR; thr++) {
        sync_thr[thr] = new SB_Thread::Thread(do_sync_fun, "sync");
        sync_thr[thr]->start();
    }
}

int tmsync_cb(void *data, int len, int handle) {
    if (!quiet)
        printf("server (%s) in tmsync-cb, data=%p, len=%d, handle=%d\n",
               pname, data, len, handle);
    static int inx = 0;
    char sync_data_act[BUFSIZ];
    sprintf(sync_data_act, "%s-%d", sync_data, inx);
    assert(strcmp((char *) data, sync_data_act) == 0);
    inx++;
    sync_rcvd = true;
    return 0;
}

char  event_data[MS_MON_MAX_SYNC_DATA];
char  lname[BUFSIZ];
char  my_name[BUFSIZ];
int   oid[MAX_SERVERS];
char  recv_buffer[BUFSIZ];
short recv_buffer2[BUFSIZ];
short recv_buffer3[BUFSIZ];
char  send_buffer[BUFSIZ];
short send_buffer2[BUFSIZ];

int main(int argc, char *argv[]) {
    bool       client = false;
    int        client_mon;
    int        cnid = -1;
    bool       commit_rcvd = false;
    int        cpid = -1;
    int        disable;
    bool       done = false;
    int        event_len;
    int        ferr;
    int        first;
    int        handle;
    int        inx;
    int        len;
    int        lerr;
    int        loop = 10;
    int        msgid;
    int        nid;
    bool       open_rcvd = false;
    int        orig_tag;
    TPT_DECL2 (phandle,MAX_SERVERS);
    int        pid;
    int        pinx;
    RT         results;
    int        send_len;
    MS_SRE     sre;
    int        status;
    bool       sync = false;
    bool       threaded = false;
    bool       verbose = false;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-quiet",     TA_Bool, TA_NOMAX,    &quiet     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-sync",      TA_Bool, TA_NOMAX,    &sync      },
      { "-threaded",  TA_Bool, TA_NOMAX,    &threaded  },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    sleep(1); // wait for client to get into msg_mon_process_info
    ferr = msg_mon_get_my_process_name(pname, sizeof(pname));
    TEST_CHK_FEOK(ferr);
    if (strcasecmp(pname, "$tm0") == 0)
         tag = TAG0;
    else if (strcasecmp(pname, "$tm1") == 0)
         tag = TAG1;
    else
         tag = TAG2;
    ferr = msg_mon_get_process_info(NULL, &snid, &spid);
    TEST_CHK_FEOK(ferr);
    if (client) {
        char serv[BUFSIZ];
        for (pinx = 0; pinx < MAX_SERVERS; pinx++) {
            sprintf(serv, "$tm%d", pinx);
            ferr = msg_mon_open_process(serv,         // name
                                        TPT_REF2(phandle,pinx),
                                        &oid[pinx]);
            TEST_CHK_FEOK(ferr);
        }
    } else {
        first = (strcasecmp(pname, "$tm0") == 0);
        disable = msg_test_assert_disable();
        ferr = msg_mon_tm_leader_set(&nid, &pid, lname);
        msg_test_assert_enable(disable);
        if (first) {
            TEST_CHK_FEOK(ferr);
        } else
            assert(ferr != XZFIL_ERR_OK);
        printf("leader is p-id=%d/%d, name=%s\n",
               nid, pid, lname);
    }

    util_gethostname(my_name, sizeof(my_name));
    msg_mon_enable_mon_messages(true);
    if (!client) {
        ferr = msg_mon_event_wait(1, &event_len, event_data);
        util_check("msg_mon_event_wait", ferr);
        ferr = msg_mon_tmsync_register(tmsync_cb);
        util_check("msg_mon_tmsync_register", ferr);
        if (sync) {
            char serv[BUFSIZ];
            for (pinx = 0; pinx < MAX_SERVERS; pinx++) {
                sprintf(serv, "$tm%d", pinx);
                if (strcasecmp(serv, pname) != 0) {
                    ferr = msg_mon_open_process(serv,         // name
                                                TPT_REF2(phandle,pinx),
                                                &oid[pinx]);
                    TEST_CHK_FEOK(ferr);
                }
            }
            // at this point, we know everyone is registered
            // if everyone isn't registered, there will be an abort
            do_sync();
            if (threaded)
                do_sync_threaded();
        }
    }

    if (client) {
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            send_len = (int) strlen(send_buffer) + 1;
            for (pinx = 0; pinx < MAX_SERVERS; pinx++) {
                if (verbose)
                    printf("%s: sending message, pinx=%d, inx=%d\n",
                           pname, pinx, inx);
                ferr = XMSG_LINK_(TPT_REF2(phandle,pinx),      // phandle
                                  &msgid,                      // msgid
                                  send_buffer2,                // reqctrl
                                  (ushort) (inx & 1),          // reqctrlsize
                                  recv_buffer3,                // replyctrl
                                  1,                           // replyctrlmax
                                  send_buffer,                 // reqdata
                                  (ushort) send_len,           // reqdatasize
                                  recv_buffer,                 // replydata
                                  BUFSIZ,                      // replydatamax
                                  0,                           // linkertag
                                  0,                           // pri
                                  0,                           // xmitclass
                                  0);                          // linkopts
                util_check("XMSG_LINK_", ferr);
                ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF2(phandle,pinx));
                util_check("XMSG_BREAK_", ferr);
                assert(results.u.t.ctrl_size == (uint) (inx & 1));
                assert(results.u.t.data_size > (strlen(send_buffer) + 14));
                assert(results.u.t.errm == RT_DATA_RCVD); // data
                if (!quiet)
                    printf("%s\n", recv_buffer);
            }
        }
        for (pinx = 0; pinx < MAX_SERVERS; pinx++) {
            ferr = msg_mon_close_process(TPT_REF2(phandle,pinx));
            TEST_CHK_FEOK(ferr);
        }
    } else {
        inx = 0;
        while (!done) {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            for (;;) {
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
                if (lerr == XSRETYPE_NOWORK)
                    break;
                ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                                      recv_buffer2,   // reqctrl
                                      1);             // bytecount
                util_check("XMSG_READCTRL_", ferr);
                ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                      recv_buffer,    // reqdata
                                      BUFSIZ);        // bytecount
                util_check("XMSG_READDATA_", ferr);
                if (sre.sre_flags & XSRE_MON) {
                    MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                    if (cnid < 0) {
                        ferr = msg_mon_get_process_info((char *) "$cli", &cnid, &cpid);
                        if (ferr != XZFIL_ERR_OK) {
                            cnid = -1;
                            cpid = -1;
                        }
                    }
                    client_mon = true;
                    Item *item_hdl;
                    Item *item_tag;
                    const char *type;
                    switch (msg->type) {
                    case MS_MsgType_Close:
                        type = "close";
                        if ((cnid == msg->u.close.nid) &&
                            (cpid == msg->u.close.pid)) {
                        } else
                            client_mon = false;
                        break;
                    case MS_MsgType_NodeDown:
                        type = "node-down";
                        break;
                    case MS_MsgType_Open:
                        type = "open";
                        if ((cnid == msg->u.open.nid) &&
                            (cpid == msg->u.open.pid))
                            open_rcvd = true;
                        else
                            client_mon = false;
                        break;
                    case MS_MsgType_ProcessDeath:
                        type = "process-death";
                        break;
                    case MS_MsgType_Shutdown:
                        type = "shutdown";
                        done = true;
                        break;
                    case MS_MsgType_TmSyncAbort:
                    case MS_MsgType_TmSyncCommit:
                        if (msg->type == MS_MsgType_TmSyncAbort)
                            type = "tmsync-abort";
                        else
                            type = "tmsync-commit";
                        nid = msg->u.tmsync.nid[0];
                        handle = msg->u.tmsync.handle[0];
                        if (msg->u.tmsync.orig_count == 0) {
                            msg->u.tmsync.orig_tag[0] = -1;
                            msg->u.tmsync.orig_handle[0] = -1;
                        }
                        orig_tag = msg->u.tmsync.orig_tag[0];
                        if (!quiet) {
                            printf("server (%s) received (%s) mon message, type=%s(%d), nid=%d\n",
                               pname,
                               client_mon ? "cli" : "dtm",
                               type,
                               msg->type,
                               nid);
                            printf("  orig_count=%d, orig_tag=%d, orig_handle=%d, count=%d, handle[0]=%d\n",
                               msg->u.tmsync.orig_count,
                               orig_tag,
                               msg->u.tmsync.orig_handle[0],
                               msg->u.tmsync.count,
                               handle);
                        }
                        if (nid == snid) {
                            if (!quiet)
                                printf("server (%s) commit handle=%d, orig-count=%d\n",
                                       pname, handle, msg->u.tmsync.orig_count);
                            for (int inx2 = 0;
                                 inx2 < msg->u.tmsync.count;
                                 inx2++) {
                                int ltag = msg->u.tmsync.orig_tag[inx2];
                                int lhandle = msg->u.tmsync.orig_handle[inx2];
                                if (!quiet)
                                    printf("server (%s) commit orig-tag/handle=%d/%d\n",
                                           pname, ltag, lhandle);
                                item_tag = (Item *) tag_issue_map.remove(ltag);
                                assert(item_tag != NULL);
                                delete item_tag;
                                item_hdl = (Item *) handle_issue_map.remove(lhandle);
                                assert(item_hdl != NULL);
                                delete item_hdl;
                            }
                        } else {
                            assert(msg->u.tmsync.orig_count == 0);
                        }
                        commit_rcvd = true;
                        break;
                    default:
                        type = "<unknown>";
                    }
                    if (!quiet)
                        printf("server (%s) received (%s) mon message, type=%s(%d)\n",
                               pname, client_mon ? "cli" : "dtm", type, msg->type);
                    len = 0;
                } else {
                    inx++;
                    if (verbose)
                        printf("%s: received non-mon message, inx=%d\n",
                               pname, inx);
                    if (!sync)
                        assert(sync_rcvd);
                    assert(commit_rcvd);
                    assert(open_rcvd);
                    strcat(recv_buffer, "- reply from ");
                    strcat(recv_buffer, my_name);
                    strcat(recv_buffer, " - ");
                    strcat(recv_buffer, pname);
                    len = (int) strlen(recv_buffer) + 1;
                }
                XMSG_REPLY_(sre.sre_msgId,       // msgid
                            recv_buffer2,        // replyctrl
                            sre.sre_reqCtrlSize, // replyctrlsize
                            recv_buffer,         // replydata
                            (ushort) len,        // replydatasize
                            0,                   // errorclass
                            NULL);               // newphandle
            }
        }
        assert(inx == loop);
        assert(tag_issue_map.empty());
        assert(handle_issue_map.empty());
        // need to wait for shutdown to be started
        ferr = msg_mon_event_wait(2, &event_len, event_data);
        util_check("msg_mon_event_wait", ferr);
        if (sync && threaded) {
            sync_thr_shutdown = true;
            for (int thr = 0; thr < MAX_THR; thr++) {
                void *larg;
                status = sync_thr[thr]->join(&larg);
                TEST_CHK_STATUSOK(status);
            }
        }
    }

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
