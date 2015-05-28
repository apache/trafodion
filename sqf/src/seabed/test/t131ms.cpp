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

//
// When *MSG_ABANDON_ is called, '*' (a message) may be in one
// of 7 places.
//
// This program attempts to test for all 7 places
//
// +--------+                *->            +--------+
// | client |                in-flight-4    | server |
// |        +-* LDONE-Q-3                   |        +-* RCV-Q-5
// |        |                               |        |
// |        +-* FSDONE-Q-2                  |        |    -* (no-Q)-6
// |        |                               |        |
// |        +-* (no-Q)-1     <-*            |        |
// +--------+                in-flight-7    +--------+
//

//
// Approach:
//   1. setup socket connection from client-to-server
//   2. execute various *programs* using the function do_exec_program()
//
// do_exec_program()
//   takes a *program* and executes the statements
//   A statement may cause the client or the server to perform some action
//   With this approach, sequencing can be precisely maintained
//

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/trace.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tprog.h"
#include "tsock.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_CTRL = 100 };
enum { MAX_INST = 2 };

bool            g_client = false;
int             g_ferr[MAX_INST];
int             g_inx;
int             g_disable;
int             g_msgid[MAX_INST];
char            g_my_name[BUFSIZ];
int             g_opt;
TPT_DECL       (g_phandle);
char            g_recv_buffer[MAX_INST][40000];
short           g_recvc_buffer[MAX_INST][MAX_CTRL];
RT              g_results;
char            g_send_buffer[MAX_INST][40000];
short           g_sendc_buffer[MAX_INST][MAX_CTRL];
TSockClient     g_sock_client;
TSockListener   g_sock_listener;
TSockServer    *g_sock_server;
MS_SRE          g_sre[MAX_INST];
MS_SRE_ABANDON  g_sre_abandon;
char            g_test[BUFSIZ];
bool            g_trace = false;
bool            g_verbose = false;
const char     *g_who;

// actors
enum {
    ACTOR_A,
    ACTOR_C,
    ACTOR_S,
    ACTOR_Z
};

static const char *actors[] = {
    "C",
    "S",
    "E"
};

// forwards
static void do_action_c_abandon(TScenarioProgStmt *stmt);
static void do_action_c_break(TScenarioProgStmt *stmt);
static void do_action_c_delay(TScenarioProgStmt *stmt);
static void do_action_c_link(TScenarioProgStmt *stmt);
static void do_action_c_nada(TScenarioProgStmt *stmt);
static void do_action_c_wait_done(TScenarioProgStmt *stmt);
static void do_action_s_begin(TScenarioProgStmt *stmt);
static void do_action_s_listen(TScenarioProgStmt *stmt);
static void do_action_s_nada(TScenarioProgStmt *stmt);
static void do_action_s_no_req(TScenarioProgStmt *stmt);
static void do_action_s_readdata(TScenarioProgStmt *stmt);
static void do_action_s_reply(TScenarioProgStmt *stmt);
static void do_action_s_wait_iscan(TScenarioProgStmt *stmt);
static void do_action_s_wait_lcan(TScenarioProgStmt *stmt);
static void do_action_s_wait_lreq(TScenarioProgStmt *stmt);
static void do_action_z(TScenarioProgStmt *stmt);

// actions
enum {
    ACTION_A,
    ACTION_C_A,
    ACTION_C_ABANDON,
    ACTION_C_BREAK,
    ACTION_C_DELAY,
    ACTION_C_LINK,
    ACTION_C_WAIT_DONE,
    ACTION_C_ZLAST,
    ACTION_S_A,
    ACTION_S_BEGIN,
    ACTION_S_LISTEN,
    ACTION_S_NO_REQ,
    ACTION_S_READDATA,
    ACTION_S_REPLY,
    ACTION_S_WAIT_ISCAN,
    ACTION_S_WAIT_LCAN,
    ACTION_S_WAIT_LREQ,
    ACTION_S_ZLAST,
    ACTION_ZLAST
};

typedef void (*Action_Cb_Type)(TScenarioProgStmt *);
typedef struct {
    const char     *text;
    Action_Cb_Type  action;
} Action_Type;
static Action_Type actions[] = {
    { "A",             do_action_c_nada                  },
    { "C_A",           do_action_c_nada                  },
    { "C_ABANDON",     do_action_c_abandon               },
    { "C_BREAK",       do_action_c_break                 },
    { "C_DELAY",       do_action_c_delay                 },
    { "C_LINK",        do_action_c_link                  },
    { "C_WAIT_DONE",   do_action_c_wait_done             },
    { "C_ZLAST",       do_action_c_nada                  },
    { "S_A",           do_action_s_nada                  },
    { "S_BEGIN",       do_action_s_begin                 },
    { "S_LISTEN",      do_action_s_listen                },
    { "S_NO_REQ",      do_action_s_no_req                },
    { "S_READDATA",    do_action_s_readdata              },
    { "S_REPLY",       do_action_s_reply                 },
    { "S_WAIT_ISCAN",  do_action_s_wait_iscan            },
    { "S_WAIT_LCAN",   do_action_s_wait_lcan             },
    { "S_WAIT_LREQ",   do_action_s_wait_lreq             },
    { "S_ZLAST",       do_action_s_nada                  },
    { "LAST",          do_action_z                       }
};

//
// Program to cleanup
//
TScenarioProg prog_cleanup[] = {
//    m  actor    action               opt                     inst
    { 0, ACTOR_S, ACTION_S_BEGIN,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       0,                      0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_IREQ,          0    },
    { 0, ACTOR_S, ACTION_S_READDATA,   XZFIL_ERR_OK,           0    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_WAIT_DONE,  0,                      0    },
    { 0, ACTOR_C, ACTION_C_BREAK,      XZFIL_ERR_OK,           0    },
    { 0, ACTOR_C, ACTION_C_ZLAST,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_ZLAST,      0,                      0    },
    { 0, ACTOR_Z, ACTION_ZLAST,        0,                      0    }
};

//
// Program to abandon scenario-1 (client-no-q) above
//
TScenarioProg prog_cli_no_q[] = {
//    m  actor    action               opt                     inst
    { 0, ACTOR_S, ACTION_S_BEGIN,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       0,                      0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_IREQ,          0    },
    { 0, ACTOR_S, ACTION_S_READDATA,   XZFIL_ERR_OK,           0    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_WAIT_DONE,  0,                      0    },
    { 0, ACTOR_C, ACTION_C_ABANDON,    0,                      0    },
    { 0, ACTOR_C, ACTION_C_BREAK,      XZFIL_ERR_NOTFOUND,     0    },
    { 0, ACTOR_C, ACTION_C_ZLAST,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_ZLAST,      0,                      0    },
    { 0, ACTOR_Z, ACTION_ZLAST,        0,                      0    }
};

//
// Program to abandon scenario-2 (client-ldone-q) above
//
TScenarioProg prog_cli_ldone[] = {
//    m  actor    action               opt                     inst
    { 0, ACTOR_S, ACTION_S_BEGIN,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       XMSG_LINK_LDONEQ,       0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_IREQ,          0    },
    { 0, ACTOR_S, ACTION_S_READDATA,   XZFIL_ERR_OK,           0    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_WAIT_DONE,  0,                      0    },
    { 0, ACTOR_C, ACTION_C_ABANDON,    0,                      0    },
    { 0, ACTOR_C, ACTION_C_BREAK,      XZFIL_ERR_NOTFOUND,     0    },
    { 0, ACTOR_C, ACTION_C_ZLAST,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_ZLAST,      0,                      0    },
    { 0, ACTOR_Z, ACTION_ZLAST,        0,                      0    }
};

//
// Program to abandon scenario-3 (client-fsdone-q) above
//
TScenarioProg prog_cli_fsdone[] = {
//    m  actor    action               opt                     inst
    { 0, ACTOR_S, ACTION_S_BEGIN,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       XMSG_LINK_FSDONEQ,      0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_IREQ,          0    },
    { 0, ACTOR_S, ACTION_S_READDATA,   XZFIL_ERR_OK,           0    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_WAIT_DONE,  0,                      0    },
    { 0, ACTOR_C, ACTION_C_ABANDON,    0,                      0    },
    { 0, ACTOR_C, ACTION_C_BREAK,      XZFIL_ERR_NOTFOUND,     0    },
    { 0, ACTOR_C, ACTION_C_ZLAST,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_ZLAST,      0,                      0    },
    { 0, ACTOR_Z, ACTION_ZLAST,        0,                      0    }
};

//
// Program to abandon scenario-4/5 (server-rcv-q) above
//
TScenarioProg prog_srv_rcv_q[] = {
//    m  actor    action               opt                     inst
    { 0, ACTOR_S, ACTION_S_BEGIN,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       0,                      0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_C, ACTION_C_ABANDON,    0,                      0    },
    { 0, ACTOR_C, ACTION_C_BREAK,      XZFIL_ERR_NOTFOUND,     0    },
    { 0, ACTOR_S, ACTION_S_NO_REQ,     0,                      0    },
    { 0, ACTOR_C, ACTION_C_ZLAST,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_ZLAST,      0,                      0    },
    { 0, ACTOR_Z, ACTION_ZLAST,        0,                      0    }
};

//
// Program to abandon scenario-6 (server-lcan) above
//
TScenarioProg prog_srv_lcan[] = {
//    m  actor    action               opt                     inst
    { 0, ACTOR_S, ACTION_S_BEGIN,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       0,                      0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_IREQ,          0    },
    { 0, ACTOR_S, ACTION_S_READDATA,   XZFIL_ERR_OK,           0    },
    { 0, ACTOR_C, ACTION_C_ABANDON,    0,                      0    },
    { 0, ACTOR_C, ACTION_C_BREAK,      XZFIL_ERR_NOTFOUND,     0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LCAN,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_ZLAST,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_ZLAST,      0,                      0    },
    { 0, ACTOR_Z, ACTION_ZLAST,        0,                      0    }
};

//
// Program to abandon scenario-6 (server-iscan) above
//
TScenarioProg prog_srv_iscan[] = {
//    m  actor    action               opt                     inst
    { 0, ACTOR_S, ACTION_S_BEGIN,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       0,                      0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_IREQ,          0    },
    { 0, ACTOR_S, ACTION_S_READDATA,   XZFIL_ERR_OK,           0    },
    { 0, ACTOR_C, ACTION_C_ABANDON,    0,                      0    },
    { 0, ACTOR_C, ACTION_C_BREAK,      XZFIL_ERR_NOTFOUND,     0    },
    { 0, ACTOR_S, ACTION_S_WAIT_ISCAN, 0,                      0    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_ZLAST,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_ZLAST,      0,                      0    },
    { 0, ACTOR_Z, ACTION_ZLAST,        0,                      0    }
};

//
// Program to abandon scenario-6 (server-iscan) above
// 2nd req follows cancel
//
TScenarioProg prog_srv_iscan_wait[] = {
//    m  actor    action               opt                     inst
    { 0, ACTOR_S, ACTION_S_BEGIN,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       0,                      0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_IREQ,          0    },
    { 0, ACTOR_S, ACTION_S_READDATA,   XZFIL_ERR_OK,           0    },
    { 0, ACTOR_C, ACTION_C_ABANDON,    0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       0,                      1    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_ABANDON,       0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_IREQ,          1    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      1    },
    { 0, ACTOR_C, ACTION_C_BREAK,      XZFIL_ERR_OK,           1    },
    { 0, ACTOR_C, ACTION_C_ZLAST,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_ZLAST,      0,                      0    },
    { 0, ACTOR_Z, ACTION_ZLAST,        0,                      0    }
};

//
// Program to abandon scenario-6 (server-iscan) above
//
TScenarioProg prog_srv_iscan_reply[] = {
//    m  actor    action               opt                     inst
    { 0, ACTOR_S, ACTION_S_BEGIN,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       0,                      0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_IREQ,          0    },
    { 0, ACTOR_S, ACTION_S_READDATA,   XZFIL_ERR_OK,           0    },
    { 0, ACTOR_C, ACTION_C_ABANDON,    0,                      0    },
    { 0, ACTOR_C, ACTION_C_BREAK,      XZFIL_ERR_NOTFOUND,     0    },
    { 0, ACTOR_S, ACTION_S_WAIT_ISCAN, 0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_ABANDON,       0    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_ZLAST,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_ZLAST,      0,                      0    },
    { 0, ACTOR_Z, ACTION_ZLAST,        0,                      0    }
};

//
// Program to abandon scenario-7 (server-reply) above
// Add delay after reply, to make sure reply gets to client
//
TScenarioProg prog_srv_reply[] = {
//    m  actor    action               opt                     inst
    { 0, ACTOR_S, ACTION_S_BEGIN,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_LINK,       0,                      0    },
    { 0, ACTOR_S, ACTION_S_WAIT_LREQ,  0,                      0    },
    { 0, ACTOR_S, ACTION_S_LISTEN,     XSRETYPE_IREQ,          0    },
    { 0, ACTOR_S, ACTION_S_READDATA,   XZFIL_ERR_OK,           0    },
    { 0, ACTOR_S, ACTION_S_REPLY,      0,                      0    },
    { 0, ACTOR_C, ACTION_C_DELAY,      1,                      0    },
    { 0, ACTOR_C, ACTION_C_ABANDON,    0,                      0    },
    { 0, ACTOR_C, ACTION_C_BREAK,      XZFIL_ERR_NOTFOUND,     0    },
    { 0, ACTOR_C, ACTION_C_ZLAST,      0,                      0    },
    { 0, ACTOR_S, ACTION_S_ZLAST,      0,                      0    },
    { 0, ACTOR_Z, ACTION_ZLAST,        0,                      0    }
};


//
// forwards
//
static void do_exec_server_program();
static void do_exec_stmt_client_action(TScenarioProgStmt *stmt);
static void do_exec_stmt_client_server(TScenarioProgStmt *stmt);
static void do_exec_stmt_server_action(TScenarioProgStmt *stmt);


//
// action-abandon
//
void do_action_c_abandon(TScenarioProgStmt *stmt) {
    int inst;

    inst = stmt->inst;
    g_ferr[inst] = XMSG_ABANDON_(g_msgid[inst]);
    util_check("XMSG_ABANDON_", g_ferr[inst]);
}

//
// action-break
//
void do_action_c_break(TScenarioProgStmt *stmt) {
    int inst;

    inst = stmt->inst;
    g_disable = msg_test_assert_disable();
    g_ferr[inst] = XMSG_BREAK_(g_msgid[inst],
                               g_results.u.s,
                               TPT_REF(g_phandle));
    msg_test_assert_enable(g_disable);
    if (g_verbose)
        printf("break (comp-q), err=%d (should be %d)\n",
               g_ferr[inst], stmt->opt);
    assert(g_ferr[inst] == stmt->opt);
}

//
// action-delay
//
void do_action_c_delay(TScenarioProgStmt *stmt) {

    usleep(stmt->opt);
    if (g_verbose)
        printf("delay=%d us\n", stmt->opt);
}

//
// action-link
//
void do_action_c_link(TScenarioProgStmt *stmt) {
    int inst;
    int len;

    g_opt = stmt->opt;
    inst = stmt->inst;
    sprintf(g_send_buffer[inst], "hello, greetings(%d) from %s, inx=%d, test=%s",
            inst, g_my_name, g_inx, g_test);
    memset(g_sendc_buffer[inst], 0, MAX_CTRL);
    len = (int) strlen(g_send_buffer[inst]) + 1;
    g_ferr[inst] = XMSG_LINK_(TPT_REF(g_phandle),            // phandle
                              &g_msgid[inst],                // msgid
                              g_sendc_buffer[inst],          // reqctrl
                              MAX_CTRL,                      // reqctrlsize
                              g_recvc_buffer[inst],          // replyctrl
                              MAX_CTRL,                      // replyctrlmax
                              g_send_buffer[inst],           // reqdata
                              (ushort) len,                  // reqdatasize
                              g_recv_buffer[inst],           // replydata
                              sizeof(g_recv_buffer[inst]),   // replydatamax
                              0,                             // linkertag
                              0,                             // pri
                              0,                             // xmitclass
                              (short) stmt->opt);            // linkopts
    util_check("XMSG_LINK_", g_ferr[inst]);
}

//
// action-nada
//
void do_action_c_nada(TScenarioProgStmt *) {
}

//
// action-wait-done
//
void do_action_c_wait_done(TScenarioProgStmt *stmt) {
    bool done;
    int  inst;
    int  inx;

    inst = stmt->inst;
    done = false;
    // wait up to 5 sec for done
    for (inx = 0; inx < 5000; inx++) {
        g_ferr[inst] = XMSG_ISDONE_(g_msgid[inst]);
        if (g_ferr[inst]) {
            done = true;
            util_time_sleep_ms(1); // sleep a little more
            break;
        }
        util_time_sleep_ms(1);
    }
    assert(done);
}

//
// action-begin
//
void do_action_s_begin(TScenarioProgStmt *) {
}

//
// action-listen
//
void do_action_s_listen(TScenarioProgStmt *stmt) {
    int inst;
    int lerr;

    inst = stmt->inst;
    lerr = XMSG_LISTEN_((short *) &g_sre[inst], // sre
                        0,                      // listenopts
                        0);                     // listenertag
    if (lerr != stmt->opt) {
        printf("LISTEN expected ret=%d, actual ret=%d\n", stmt->opt, lerr);
        assert(lerr == stmt->opt);
    }
}

//
// action-nada
//
void do_action_s_nada(TScenarioProgStmt *) {
}

//
// action-no-req
//
void do_action_s_no_req(TScenarioProgStmt *) {
    int    lerr;

    lerr = XWAIT(LREQ, 5);
    TEST_CHK_WAITIGNORE(lerr);
    if (lerr == LREQ) {
        lerr = XMSG_LISTEN_((short *) &g_sre, // sre
                            0,                // listenopts
                            0);               // listenertag
        assert(lerr == XSRETYPE_NOWORK);
    }
}

//
// action-readdata
//
void do_action_s_readdata(TScenarioProgStmt *stmt) {
    int   inst;
    bool  match;
    int   mon_msg;
    char *p;

    inst = stmt->inst;
    mon_msg = (g_sre[inst].sre_flags & XSRE_MON);
    if (!mon_msg) {
        // disable in case it returns cancelled
        g_disable = msg_test_assert_disable();
        g_ferr[inst] = XMSG_READDATA_(g_sre[inst].sre_msgId,  // msgid
                                      g_recv_buffer[inst],    // reqdata
                                      40000);                 // bytecount
        msg_test_assert_enable(g_disable);
        if (g_ferr[inst] != stmt->opt) {
            printf("READDATA expected ret=%d, actual ret=%d\n",
                   stmt->opt, g_ferr[inst]);
            assert(g_ferr[inst] == stmt->opt);
        }
        if (g_ferr[inst] == XZFIL_ERR_OK) {
            if (g_verbose)
                printf("server READDATA '%s'\n", g_recv_buffer[inst]);
            p = strstr(g_recv_buffer[inst], "test=");
            if (p != NULL) {
                match = (strcmp(&p[5], g_test) == 0);
                if (!match) {
                    printf("server expecting test=%s, but got %s\n",
                           g_test, p);
                    assert(match);
                }
            }
        }
    }
}

//
// action-reply
//
void do_action_s_reply(TScenarioProgStmt *stmt) {
    int inst;

    inst = stmt->inst;
    XMSG_REPLY_(g_sre[inst].sre_msgId,         // msgid
                g_recvc_buffer[inst],          // replyctrl
                MAX_CTRL,                      // replyctrlsize
                g_recv_buffer[inst],           // replydata
                g_sre[inst].sre_reqDataSize,   // replydatasize
                0,                             // errorclass
                NULL);                         // newphandle
}

//
// action-wait-iscan
//
void do_action_s_wait_iscan(TScenarioProgStmt *stmt) {
    int inst;
    int lerr;

    inst = stmt->inst;
    lerr = XWAIT(LCAN, -1);
    TEST_CHK_WAITIGNORE(lerr);
    g_ferr[inst] = XMSG_ISCANCELED_(g_sre[inst].sre_msgId);
    assert(g_ferr[inst]);
}

//
// action-wait-lcan
//
void do_action_s_wait_lcan(TScenarioProgStmt *) {
    int lerr;

    lerr = XWAIT(LCAN, -1);
    TEST_CHK_WAITIGNORE(lerr);
    lerr = XMSG_LISTEN_((short *) &g_sre_abandon,   // sre
                        XLISTEN_ALLOW_ABANDONM,     // listenopts
                        0);                         // listenertag
    assert(lerr == XSRETYPE_ABANDON);
    // can't check if sre_abandon is correct
}

//
// action-wait-lreq
//
void do_action_s_wait_lreq(TScenarioProgStmt *) {
    int lerr;

    lerr = XWAIT(LREQ, -1);
    TEST_CHK_WAITIGNORE(lerr);
}

//
// action-nada
//
void do_action_z(TScenarioProgStmt *) {
}

//
// get action text
//
void do_get_action_text(int action, char *taction) {
    assert(sizeof(actions)/sizeof(Action_Type) == (ACTION_ZLAST + 1));
    if ((action <= 0) || (action > ACTION_ZLAST))
        sprintf(taction, "%d(%s)", action, "unknown");
    else
        sprintf(taction, "%d(%s)", action, actions[action].text);
}

//
// get actor text
//
void do_get_actor_text(int actor, char *tactor) {
    assert(sizeof(actors)/sizeof(char *) == ACTOR_Z);
    if ((actor <= 0) || (actor > ACTOR_Z))
        sprintf(tactor, "%d(%s)", actor, "unknown");
    else
        sprintf(tactor, "%d(%s)", actor, actors[actor-1]);
}

//
// get remote-info
//
void do_get_remote_info() {
    int     err;
    char    host[BUFSIZ];
    int     inst;
    int     len;
    int     lerr;
    bool    mon_msg;
    char   *p;
    int     port = 0;

    inst = 0;
    if (g_client) {
        if (g_verbose)
            printf("client getting remote info\n");
        g_ferr[inst] = XMSG_LINK_(TPT_REF(g_phandle),          // phandle
                                  &g_msgid[inst],              // msgid
                                  NULL,                        // reqctrl
                                  0,                           // reqctrlsize
                                  NULL,                        // replyctrl
                                  0,                           // replyctrlmax
                                  NULL,                        // reqdata
                                  0,                           // reqdatasize
                                  g_recv_buffer[inst],         // replydata
                                  sizeof(g_recv_buffer[inst]), // replydatamax
                                  0,                           // linkertag
                                  0,                           // pri
                                  0,                           // xmitclass
                                  0);                          // linkopts
        util_check("XMSG_LINK_", g_ferr[inst]);
        g_ferr[inst] = XMSG_BREAK_(g_msgid[inst],
                                   g_results.u.s,
                                   TPT_REF(g_phandle));
        util_check("XMSG_BREAK_", g_ferr[inst]);
        if (g_verbose)
            printf("client got remote info=%s\n", g_recv_buffer[inst]);
        p = strstr(g_recv_buffer[inst], ":");
        assert(p != NULL);
        *p = 0;
        port = atoi(&p[1]);
        if (g_verbose)
            printf("client connecting...\n");
        g_sock_client.connect(g_recv_buffer[inst], port);
        if (g_verbose)
            printf("client connected\n");
    } else {
        mon_msg = true;
        while (mon_msg) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &g_sre[inst], // sre
                                    0,                      // listenopts
                                    0);                     // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            mon_msg = g_sre[inst].sre_flags & XSRE_MON;
            if (mon_msg)
                len = 0;
            else {
                err = gethostname(host, sizeof(host));
                assert(!err);
                g_sock_listener.listen(host, &port, NULL);
                sprintf(g_recv_buffer[inst], "%s:%d", host, port);
                len = (int) strlen(g_recv_buffer[inst]) + 1;
            }
            XMSG_REPLY_(g_sre[inst].sre_msgId,               // msgid
                        NULL,                                // replyctrl
                        0,                                   // replyctrlsize
                        g_recv_buffer[inst],                 // replydata
                        (ushort) len,                        // replydatasize
                        0,                                   // errorclass
                        NULL);                               // newphandle
        }
        if (g_verbose)
            printf("server accepting...\n");
        g_sock_server = g_sock_listener.accept();
        if (g_verbose)
            printf("server accepted\n");
    }
}

//
// execute a client-stmt
//
void do_exec_client_stmt(const char *prog_name, TScenarioProgStmt *stmt) {
    strcpy(g_test, prog_name);
    if (g_verbose) {
        char taction[BUFSIZ];
        char tactor[BUFSIZ];
        do_get_actor_text(stmt->actor, tactor);
        do_get_action_text(stmt->action, taction);
        printf("client-exec, test=%s, actor=%s, action=%s, opt=%d, inst=%d\n",
               prog_name, tactor, taction, stmt->opt, stmt->inst);
    }
    switch (stmt->actor) {
    case ACTOR_C:
        do_exec_stmt_client_action(stmt);
        break;
    case ACTOR_S:
        do_exec_stmt_client_server(stmt);
        break;
    }
}

//
// execute a program
//
void do_exec_program(const char *prog_name, TScenarioProg *prog) {
    if (g_client) {
        for (; prog->actor != ACTOR_Z; prog++)
            do_exec_client_stmt(prog_name, prog);
    } else {
        do_exec_server_program();
    }
}

//
// execute a program from server
//
void do_exec_server_program() {
    TScenarioProgStmt stmt;

    do {
        g_sock_server->read(&stmt, sizeof(stmt));
        if (g_verbose) {
            char taction[BUFSIZ];
            do_get_action_text(stmt.action, taction);
            printf("server received action=%s\n", taction);
        }
        if (stmt.marker != TMARKER)
            printf("server received stmt with invalid marker=%x\n",
                   stmt.marker);
        assert(stmt.marker == TMARKER);
        if (stmt.action == ACTION_S_BEGIN) {
            int len;
            g_sock_server->read(&len, sizeof(len));
            g_sock_server->read(g_test, len);
            if (g_verbose)
                printf("server received test=%s\n", g_test);
        } else if (stmt.action != ACTION_S_ZLAST)
            do_exec_stmt_server_action(&stmt);
        g_sock_server->write(&stmt, sizeof(stmt));
        if (g_verbose)
            printf("server sent ack\n");
    } while (stmt.action != ACTION_S_ZLAST);
}

//
// execute a client-action
//
void do_exec_stmt_client_action(TScenarioProgStmt *stmt) {
    int  action;
    char taction[BUFSIZ];

    action = stmt->action;
    if ((action > ACTION_C_A) && (action <= ACTION_C_ZLAST))
        actions[action].action(stmt);
    else {
        do_get_action_text(action, taction);
        printf("client-action didn't implement action=%s\n", taction);
        assert(false);
    }
}

//
// execute a server-stmt from client
//
void do_exec_stmt_client_server(TScenarioProgStmt *stmt) {
    stmt->marker = TMARKER;
    g_sock_client.write(stmt, sizeof(*stmt));
    if (g_verbose) {
        char taction[BUFSIZ];
        do_get_action_text(stmt->action, taction);
        printf("client sent action=%s to server\n", taction);
    }
    if (stmt->action == ACTION_S_BEGIN) {
        int len = (int) strlen(g_test) + 1;
        g_sock_client.write(&len, sizeof(len));
        g_sock_client.write(g_test, len);
        if (g_verbose)
            printf("client sent test=%s\n", g_test);
    }
    g_sock_client.read(stmt, sizeof(*stmt));
    if (g_verbose)
        printf("client received ack\n");
    if (stmt->marker != TMARKER)
        printf("client received stmt with invalid marker=%x\n",
               stmt->marker);
    assert(stmt->marker == TMARKER);
}

//
// execute a server-action
//
void do_exec_stmt_server_action(TScenarioProgStmt *stmt) {
    int  action;
    char taction[BUFSIZ];

    action = stmt->action;
    if ((action > ACTION_S_A) && (action <= ACTION_S_ZLAST))
        actions[action].action(stmt);
    else {
        do_get_action_text(action, taction);
        printf("server-action didn't implement action=%s\n", taction);
        assert(false);
    }
}

int main(int argc, char *argv[]) {
    int   ferr;
    int   loop = 10;
    int   oid;
    TAD   zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &g_client  },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-trace",     TA_Bool, TA_NOMAX,    &g_trace   },
      { "-v",         TA_Bool, TA_NOMAX,    &g_verbose },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    msg_test_set_md_count(5); // should only need a few mds
    arg_proc_args(zargs, false, argc, argv);
    g_who = g_client ? "cli" : "srv";
    util_test_start(g_client);
    ferr = msg_mon_process_startup(!g_client);             // system messages?
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);
    if (g_client) {
        ferr = msg_mon_open_process((char *) "$srv",              // name
                                    TPT_REF(g_phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(g_my_name, sizeof(g_my_name));

    do_get_remote_info();

    for (g_inx = 0; g_inx < loop; g_inx++) {
        if (g_verbose)
            printf("%s: inx=%d\n", g_who, g_inx);
        do_exec_program("cli-no-q", prog_cli_no_q);
        do_exec_program("cleanup", prog_cleanup);
        do_exec_program("cli-ldone", prog_cli_ldone);
        do_exec_program("cleanup", prog_cleanup);
        do_exec_program("cli-fsone", prog_cli_fsdone);
        do_exec_program("cleanup", prog_cleanup);
        do_exec_program("srv-rcv-q", prog_srv_rcv_q);
        do_exec_program("cleanup", prog_cleanup);
        do_exec_program("srv-lcan", prog_srv_lcan);
        do_exec_program("cleanup", prog_cleanup);
        do_exec_program("srv-iscan", prog_srv_iscan);
        do_exec_program("cleanup", prog_cleanup);
        do_exec_program("srv-iscan-wait", prog_srv_iscan_wait);
        do_exec_program("cleanup", prog_cleanup);
#if 0
        // verify double-reply will cause crash
        do_exec_program("srv-iscan-reply", prog_srv_iscan_reply);
        do_exec_program("cleanup", prog_cleanup);
#endif
        do_exec_program("srv-reply", prog_srv_reply);
        do_exec_program("cleanup", prog_cleanup);
    }
    if (g_client) {
        ferr = msg_mon_close_process(TPT_REF(g_phandle));
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = msg_mon_process_close();
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(g_client);
    return 0;
}
