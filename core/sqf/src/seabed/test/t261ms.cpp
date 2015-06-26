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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

bool                   chk = true;
MS_Mon_Node_Info_Type  cinfo;
int                    cnid = -1;
int                    cpid = -1;
int                    gargc;
char                 **gargv;
char                   group[20];
char                  *key;
bool                   quiesce = false;
char                   recv_buffer[40000];
char                   send_buffer[40000];
int                    snid = -1;
int                    spid = -1;
MS_Mon_Msg            *sys_msg = (MS_Mon_Msg *) recv_buffer;
bool                   sys_mon_msg;
char                  *value;
bool                   verbose = false;
MS_Mon_Zone_Info_Type  zinfo, zinfo1, zinfo2;

// forwards
void        display_info_entry(int                          count,
                               MS_Mon_Zone_Info_Entry_Type *info);
void        display_zone_info(const char *str,
                              MS_Mon_Zone_Info_Type *zone_info);
void        srv_process_sm_close_chk();
void        srv_process_sm_node_down_chk();
void        srv_process_sm_process_death_chk();
const char *srv_sm_lookup(int mt);

void display_info_entry(int                          count,
                        MS_Mon_Zone_Info_Entry_Type *info) {
    int node;

    printf("cluster-nodes=%d\n",
           count);
    for (node = 0; node < count; node++) {
        printf("node[%d].nid=%d, zid=%d, pnid=%d, pstate=%d, node_name=%s\n",
               node,
               info[node].nid,
               info[node].zid,
               info[node].pnid,
               info[node].pstate,
               info[node].node_name);
    }
}

void display_zone_info(const char *str, MS_Mon_Zone_Info_Type *zone_info) {
    printf("\n");
    printf("display_zone_info %s, num-returned=%d\n", str, zone_info->num_returned);
    for (int n = 0; n < zone_info->num_returned; n++)
        printf("  nid=%d, pnid=%d, zid=%d, pstate=%d, node_name=%s\n",
               zone_info->node[n].nid,
               zone_info->node[n].pnid,
               zone_info->node[n].zid,
               zone_info->node[n].pstate,
               zone_info->node[n].node_name);
}

void srv_do_change() {
    int ferr;

    sprintf(group, "NODE%d", snid);
    key = (char *) "akey";
    value = (char *) "avalue";
    ferr = msg_mon_reg_set(MS_Mon_ConfigType_Node,       // type
                           group,                        // group
                           key,                          // key
                           value);                       // value
    TEST_CHK_FEOK(ferr);
}

void srv_do_shutdown() {
    int ferr;

    ferr = msg_mon_shutdown(MS_Mon_ShutdownLevel_Normal);
    TEST_CHK_FEOK(ferr);
}

void srv_do_start() {
    int             arg;
    int             ferr;
    char            name[MS_MON_MAX_PROCESS_NAME];
    int             nid;
    SB_Phandle_Type phandle;
    char            prog[MS_MON_MAX_PROCESS_PATH];

    sprintf(prog, "%s/%s", getenv("PWD"), gargv[0]);
    for (arg = 0; arg < gargc; arg++)
        if (strcmp(gargv[arg], "-server") == 0) // start_process
            gargv[arg] = (char *) "-die";
    nid = -1;
    strcpy(name, "$die");
    ferr = msg_mon_start_process_nowait(prog,                   // prog
                                        name,                   // name
                                        NULL,                   // ret_name
                                        gargc,                  // argc
                                        gargv,                  // argv
                                        TPT_REF(phandle),       // phandle
                                        MS_ProcessType_Generic, // ptype
                                        0,                      // priority
                                        0,                      // debug
                                        0,                      // backup
                                        9,                      // tag
                                        &nid,                   // nid
                                        NULL,                   // pid
                                        NULL,                   // infile
                                        NULL);                  // outfile
    TEST_CHK_FEOK(ferr);
}

void srv_print_sm(int exp_type, int act_type) {
    const char *str_act;
    const char *str_exp;

    if (verbose) {
        if (exp_type == -1)
            str_exp = "Close|NodeDown|ProcessDeath";
        else
            str_exp = srv_sm_lookup(exp_type);
        str_act = srv_sm_lookup(act_type);
        printf("expecting msg-type=%d(%s), actual msg-type=%d(%s)\n",
               exp_type, str_exp, act_type, str_act);
    }
    if (chk) {
        assert(exp_type == act_type);
    }
}

void srv_process_sm(int exp_type) {
    int         ferr;
    int         lerr;
    BMS_SRE     sre;

    if (verbose)
        printf("listen before\n");
    do {
        lerr = XWAIT(LREQ, -1);
        TEST_CHK_WAITIGNORE(lerr);
        lerr = BMSG_LISTEN_((short *) &sre, // sre
                            0,              // listenopts
                            0);             // listenertag
    } while (lerr == XSRETYPE_NOWORK);
    ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                          recv_buffer,    // reqdata
                          40000);         // bytecount
    util_check("BMSG_READDATA_", ferr);
    sys_mon_msg = (sre.sre_flags & BSRE_MON);
    if (verbose) {
        if (sys_mon_msg)
            printf("listen after sre.flags=0x%x, msg-type=%d(%s)\n",
                   sre.sre_flags, sys_msg->type, srv_sm_lookup(sys_msg->type));
        else
            printf("listen after sre.flags=0x%x\n", sre.sre_flags);
    }

    if (exp_type == 0) {
        if (chk) {
            assert(!sys_mon_msg);
        }
    }
    if (sys_mon_msg) {
        srv_print_sm(exp_type, sys_msg->type);
    }

    if (verbose)
        printf("reply\n");
    BMSG_REPLY_(sre.sre_msgId,  // msgid
                NULL,           // replyctrl
                0,              // replyctrlsize
                recv_buffer,    // replydata
                0,              // replydatasize
                0,              // errorclass
                NULL);          // newphandle
}

void srv_process_sm_change() {
    srv_process_sm(MS_MsgType_Change);
    if (chk) {
        assert(sys_msg->u.change.type == MS_Mon_ConfigType_Node);
        assert(strcmp(sys_msg->u.change.group, group) == 0);
        assert(strcasecmp(sys_msg->u.change.key, key) == 0);
        assert(strcmp(sys_msg->u.change.value, value) == 0);
    }
}

void srv_process_sm_close() {
    srv_process_sm(MS_MsgType_Close);
    if (chk)
        srv_process_sm_close_chk();
}

void srv_process_sm_close_chk() {
    assert(sys_msg->u.close.nid == cnid);
    assert(sys_msg->u.close.pid == cpid);
    assert(strcasecmp(sys_msg->u.close.process_name, "$cli") == 0);
    assert(sys_msg->u.close.aborted);
    assert(sys_msg->u.close.mon);
}

void srv_process_sm_open() {
    int disable;
    int ferr;
    int node_info_count = 0;
    int node_info_max = 0;
    MS_Mon_Zone_Info_Entry_Type *zone_info = NULL;

    srv_process_sm(MS_MsgType_Open);
    if (chk) {
        assert(strcasecmp(sys_msg->u.open.target_process_name, "$cli") == 0);
        assert(sys_msg->u.open.death_notification);
    }
    cnid = sys_msg->u.open.nid;
    cpid = sys_msg->u.open.pid;
    ferr = msg_mon_get_node_info_detail(cnid, &cinfo);
    TEST_CHK_FEOK(ferr);
    disable = msg_test_assert_disable();
    ferr = msg_mon_get_zone_info_detail(-1, -1, &zinfo);
    assert(ferr == XZFIL_ERR_BOUNDSERR);
    msg_test_assert_enable(disable);
    printf("\n");
    display_zone_info("msg_mon_get_zone_info_detail - nid is -1, zid is -1",&zinfo);
    printf("Error returned should be BOUNDSERR 22, value was %d",ferr);
    ferr = msg_mon_get_zone_info_detail(cnid, -1, &zinfo1);
    TEST_CHK_FEOK(ferr);
    printf("\n");
    printf("Opened nid is %d\n", cnid);
    display_zone_info("msg_mon_get_zone_info_detail - nid is opened nid, zid is -1",&zinfo1);
    printf("Previous zid was %d\n", zinfo.node[0].zid);
    ferr = msg_mon_get_zone_info_detail(-1, zinfo.node[0].zid, &zinfo2);
    TEST_CHK_FEOK(ferr);
    display_zone_info("msg_mon_get_zone_info_detail - nid is -1, zid is previous zid",&zinfo2);
    printf("\n");
    sleep(20);
    ferr = msg_mon_get_zone_info(&node_info_count, 0, NULL);
    printf("\nmsg_mon_get_zone_info returned %d nodes\n", node_info_count);
    TEST_CHK_FEOK(ferr);

    node_info_max = node_info_count;
    zone_info = (MS_Mon_Zone_Info_Entry_Type *)malloc(node_info_count * sizeof(MS_Mon_Zone_Info_Entry_Type));
    //zone_info = (MS_Mon_Zone_Info_Type *)malloc(node_info_count * sizeof(MS_Mon_Zone_Info_Type));
    ferr = msg_mon_get_zone_info(&node_info_count, node_info_max, zone_info);
    TEST_CHK_FEOK(ferr);
    printf("\nUsing display_info_entry\n");
    display_info_entry(node_info_count, zone_info);
    free(zone_info);
    ferr = msg_mon_register_death_notification(cnid, cpid);
    TEST_CHK_FEOK(ferr);
}

void srv_process_sm_node_down() {
    srv_process_sm(MS_MsgType_NodeDown);
    if (chk)
        srv_process_sm_node_down_chk();
}

void srv_process_sm_node_down_all() {
    enum {
        SM_C  = 1,
        SM_ND = 2,
        SM_PD = 4
    };
    int         inx;
    int         sm_mask = 0;
    int         sm_type;
    const char *str_act;
    const char *str_sm_c;
    const char *str_sm_nd;
    const char *str_sm_pd;
    bool        xchk;

    // close/process-death/node-down can happen in any order
    xchk = chk;
    chk = false;
    for (inx = 0; inx < 3; inx++) {
        srv_process_sm(-1);
        switch (sys_msg->type) {
        case MS_MsgType_Close:
            sm_type = SM_C;
            break;
        case MS_MsgType_NodeDown:
            sm_type = SM_ND;
            break;
        case MS_MsgType_ProcessDeath:
            sm_type = SM_PD;
            break;
        default:
            sm_type = 0;
            str_sm_c = srv_sm_lookup(MS_MsgType_Close);
            str_sm_nd = srv_sm_lookup(MS_MsgType_NodeDown);
            str_sm_pd = srv_sm_lookup(MS_MsgType_ProcessDeath);
            str_act = srv_sm_lookup(sys_msg->type);
            printf("expecting msg-type=%d(%s)/%d(%s)/%d(%s), actual msg-type=%d(%s)\n",
                   MS_MsgType_Close, str_sm_c,
                   MS_MsgType_NodeDown, str_sm_nd,
                   MS_MsgType_ProcessDeath, str_sm_pd,
                   sys_msg->type, str_act);
            assert((sys_msg->type == MS_MsgType_Close) ||
                   (sys_msg->type == MS_MsgType_NodeDown) ||
                   (sys_msg->type == MS_MsgType_ProcessDeath));
            break;
        }
        assert((sm_type & sm_mask) == 0);
        sm_mask |= sm_type;
        if (xchk) {
            switch (sys_msg->type) {
            case MS_MsgType_Close:
                srv_process_sm_close_chk();
                break;
            case MS_MsgType_NodeDown:
                srv_process_sm_node_down_chk();
                break;
            case MS_MsgType_ProcessDeath:
                srv_process_sm_process_death_chk();
                break;
            default:
                break;
            }
        }
    }
    chk = xchk;
}

void srv_process_sm_node_down_chk() {
    assert(sys_msg->u.down.nid == cnid);
    assert(strcmp(sys_msg->u.down.node_name, cinfo.node[0].node_name) == 0);
}

void srv_process_sm_node_quiesce() {
    srv_process_sm(MS_MsgType_NodeQuiesce);
    assert(sys_msg->u.quiesce.nid == cnid);
    assert(strcmp(sys_msg->u.quiesce.node_name, cinfo.node[0].node_name) == 0);
}

void srv_process_sm_node_up() {
    srv_process_sm(MS_MsgType_NodeUp);
    if (chk) {
        assert(sys_msg->u.up.nid == cnid);
        assert(strcmp(sys_msg->u.up.node_name, cinfo.node[0].node_name) == 0);
    }
}

void srv_process_sm_process_created() {
    srv_process_sm(MS_MsgType_ProcessCreated);
    // can't check nid/pid
    assert(sys_msg->u.process_created.tag == 9);
    assert(strcasecmp(sys_msg->u.process_created.process_name, "$die") == 0);
    assert(sys_msg->u.process_created.ferr == XZFIL_ERR_OK);
}

void srv_process_sm_process_death() {
    srv_process_sm(MS_MsgType_ProcessDeath);
    if (chk)
        srv_process_sm_process_death_chk();
}

void srv_process_sm_process_death_chk() {
    int inx;
    int len;

    assert(sys_msg->u.death.nid == cnid);
    assert(sys_msg->u.death.pid == cpid);
    len = (int) (sizeof(sys_msg->u.death.transid) /
                 sizeof(sys_msg->u.death.transid.id[0]));
    for (inx = 0; inx < len; inx++)
        assert(sys_msg->u.death.transid.id[inx] == 0);
    assert(sys_msg->u.death.aborted);
    assert(strcasecmp(sys_msg->u.death.process_name, "$cli") == 0);
    assert(sys_msg->u.death.type == MS_ProcessType_TSE);
}

void srv_process_sm_shutdown() {
    const char *str_act;
    const char *str_sm_pd;
    const char *str_sm_s;
    bool        xchk;

    // there might be a process death before shutdown
    xchk = chk;
    chk = false;
    srv_process_sm(-1);
    switch (sys_msg->type) {
    case MS_MsgType_ProcessDeath:
        srv_process_sm(MS_MsgType_Shutdown);
        break;
    case MS_MsgType_Shutdown:
        break;
    default:
        str_sm_pd = srv_sm_lookup(MS_MsgType_ProcessDeath);
        str_sm_s = srv_sm_lookup(MS_MsgType_Shutdown);
        str_act = srv_sm_lookup(sys_msg->type);
        printf("expecting msg-type=%d(%s)/%d(%s), actual msg-type=%d(%s)\n",
               MS_MsgType_ProcessDeath, str_sm_pd,
               MS_MsgType_Shutdown, str_sm_s,
               sys_msg->type, str_act);
        assert((sys_msg->type == MS_MsgType_ProcessDeath) ||
               (sys_msg->type == MS_MsgType_Shutdown));
        break;
    }
    if (chk) {
        assert(sys_msg->u.shutdown.nid == snid);
        assert(sys_msg->u.shutdown.pid == -1);
        assert(sys_msg->u.shutdown.level == MS_Mon_ShutdownLevel_Normal);
    }
    chk = xchk;
}

const char *srv_sm_lookup(int mt) {
    const char *ret;

    switch (mt) {
    case MS_MsgType_Change:                  // tested
        ret = "MS_MsgType_Change";
        break;
    case MS_MsgType_Close:                   // tested
        ret = "MS_MsgType_Close";
        break;
    case MS_MsgType_Event:                   // impossible
        ret = "MS_MsgType_Event";
        break;
    case MS_MsgType_NodeDown:                // tested
        ret = "MS_MsgType_NodeDown";
        break;
    case MS_MsgType_NodeQuiesce:             // tested
        ret = "MS_MsgType_NodeQuiesce";
        break;
    case MS_MsgType_NodeUp:                  // tested
        ret = "MS_MsgType_NodeUp";
        break;
    case MS_MsgType_Open:                    // tested
        ret = "MS_MsgType_Open";
        break;
    case MS_MsgType_ProcessCreated:          // tested
        ret = "MS_MsgType_ProcessCreated";
        break;
    case MS_MsgType_ProcessDeath:            // tested
        ret = "MS_MsgType_ProcessDeath";
        break;
    case MS_MsgType_Service:                 // impossible
        ret = "MS_MsgType_Service";
        break;
    case MS_MsgType_Shutdown:                // tested
        ret = "MS_MsgType_Shutdown";
        break;
    case MS_MsgType_TmSyncAbort:             // trans-test
        ret = "MS_MsgType_TmSyncAbort";
        break;
    case MS_MsgType_TmSyncCommit:            // trans-test
        ret = "MS_MsgType_TmSyncCommit";
        break;
    case MS_MsgType_UnsolicitedMessage:      // trans-test
        ret = "MS_MsgType_UnsolicitedMessage";
        break;
    default:
        ret = "?";
        break;
    }
    return ret;
}

int main(int argc, char *argv[]) {
    bool       client = false;
    bool       die = false;
    int        disable;
    char       event_data[MS_MON_MAX_SYNC_DATA];
    int        event_len;
    int        ferr;
    int        len;
    int        msgid;
    bool       nochk = false;
    int        oid;
    TPT_DECL  (phandle);
    RT         results;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-die",       TA_Bool, TA_NOMAX,    &die       },
      { "-nochk",     TA_Bool, TA_NOMAX,    &nochk     },
      { "-quiesce",   TA_Bool, TA_NOMAX,    &quiesce   },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    gargc = argc;
    gargv = argv;
    arg_proc_args(zargs, false, argc, argv);
    if (nochk)
        chk = false;
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    if (die) {
        ferr = msg_mon_process_shutdown();
        TEST_CHK_FEOK(ferr);
        return 0;
    }

    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
        strcpy(send_buffer, "control\n");
        len = (int) strlen(send_buffer) + 1;
        ferr = BMSG_LINK_(TPT_REF(phandle),            // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          NULL,                        // replyctrl
                          0,                           // replyctrlmax
                          send_buffer,                 // reqdata
                          len,                         // reqdatasize
                          recv_buffer,                 // replydata
                          40000,                       // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          0);                          // linkopts
        util_check("BMSG_LINK_", ferr);
        ferr = BMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
        util_check("BMSG_BREAK_", ferr);

        disable = msg_test_assert_disable();
        if (quiesce) {
            // just exit on a node quiesce
            srv_process_sm_node_quiesce();
        } else {
            // wait, but the node will be downed before finishing
            ferr = msg_mon_event_wait(2, &event_len, event_data);
            msg_test_assert_enable(disable);
        }
    } else {
        ferr = msg_mon_get_process_info(NULL, &snid, &spid);
        TEST_CHK_FEOK(ferr);
        msg_mon_enable_mon_messages(true);
        srv_process_sm_open();
        srv_process_sm(0);
        srv_process_sm_close();
        if (quiesce) {
            //sleep(4);
            srv_process_sm_node_quiesce();
            srv_process_sm_process_death();
        } else {
            srv_process_sm_process_death();
            srv_process_sm_node_down();
        }
        if (getenv("SQ_VIRTUAL_NODES") != NULL)
            srv_process_sm_node_up();
        srv_do_change();
        srv_process_sm_change();
        srv_do_start();
        srv_process_sm_process_created();
        srv_do_shutdown();
        srv_process_sm_shutdown();
    }
    if (client && !quiesce) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(!client);
    return 0;
}
