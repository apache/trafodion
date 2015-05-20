//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/time.h>

#include "SCMVersHelp.h"

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "idtmsrv.h"

char           ga_name[BUFSIZ];
char          *gp_shm;
unsigned long *gp_shml;
bool           gv_shook   = false;
bool           gv_verbose = false;

DEFINE_EXTERN_COMP_DOVERS(idtmsrv)

// forwards
void do_reply(BMS_SRE *pp_sre, char *pp_reply, int pv_len, short pv_ec);


//
// initialize
//
void do_init(int pv_argc, char **ppp_argv) {
    char *lp_arg;
    int   lv_arg;
    bool  lv_attach;
    int   lv_ferr;

    lv_attach = false;
    for (lv_arg = 1; lv_arg < pv_argc; lv_arg++) {
        lp_arg = ppp_argv[lv_arg];
        if (strcmp(lp_arg, "-attach") == 0)
            lv_attach = true;
        else if (strcmp(lp_arg, "-shook") == 0)
            gv_shook = true;
        else if (strcmp(lp_arg, "-v") == 0)
            gv_verbose = true;
    }
    if (lv_attach)
        lv_ferr = msg_init_attach(&pv_argc, &ppp_argv, false, (char *) "$TSID0");
    else
        lv_ferr = msg_init(&pv_argc, &ppp_argv);
    assert(lv_ferr == XZFIL_ERR_OK);

    if (gv_shook)
        msg_debug_hook("s", "s");
}

//
// process monitor message.
//
// if shutdown, set done
//
void do_mon_msg(BMS_SRE *pp_sre, bool *pp_done) {
    int        lv_ferr;
    MS_Mon_Msg lv_mon_msg;

    lv_ferr = BMSG_READDATA_(pp_sre->sre_msgId,         // msgid
                             (char *) &lv_mon_msg,      // reqdata
                             (int) sizeof(lv_mon_msg)); // bytecount
    assert(lv_ferr == XZFIL_ERR_OK);
    if (lv_mon_msg.type == MS_MsgType_Shutdown)
        *pp_done = true;
    if (gv_verbose)
        printf("srv: received mon message\n");
    do_reply(pp_sre, NULL, 0, 0);
}

//
// process non-mon message
//
void do_req(BMS_SRE *pp_sre) {
    const char     *lp_req_type;
    short           lv_ec;
    int             lv_ferr;
    int             lv_len;
    GID_Rep         lv_rep;
    GID_Req         lv_req;
    struct timeval  lv_tv;

    lv_ec = XZFIL_ERR_OK;
    lv_len = 0;
    if (gv_verbose)
        printf("srv: received NON-mon message\n");
    if (pp_sre->sre_reqDataSize < (int) sizeof(lv_req)) {
        if (gv_verbose)
            printf("srv: received short data - sre_reqDataSize=%d, expecting len=%d, setting BADCOUNT\n",
                   pp_sre->sre_reqDataSize, (int) sizeof(lv_req));
        lv_ec = XZFIL_ERR_BADCOUNT;
    } else {
        lv_ferr = BMSG_READDATA_(pp_sre->sre_msgId,      // msgid
                                 (char *) &lv_req,       // reqdata
                                 (int) sizeof(lv_req));  // bytecount
        assert(lv_ferr == XZFIL_ERR_OK);
        if (gv_verbose) {
            switch (lv_req.iv_req_type) {
            case GID_REQ_PING:
                lp_req_type = "ping";
                break;
            case GID_REQ_ID:
                lp_req_type = "id";
                break;
            default:
                lp_req_type = "unknown";
                break;
            }
            if (gv_verbose)
                printf("srv: received msg. req-type=%d(%s), tag=%ld, len=%d\n",
                       lv_req.iv_req_type, lp_req_type, lv_req.iv_req_tag, lv_req.iv_req_len);
        }
        switch (lv_req.iv_req_type) {
        case GID_REQ_PING:
            if (lv_req.iv_req_len == (int) sizeof(lv_req.u.iv_ping)) {
                if (gv_verbose)
                    printf("srv: received ping request\n");
                lv_rep.iv_rep_type = GID_REP_PING;
                lv_rep.iv_rep_tag = lv_req.iv_req_tag;
                lv_rep.iv_rep_len = (int) sizeof(lv_rep.u.iv_ping);
                lv_rep.u.iv_ping.iv_com.iv_error = GID_ERR_OK;
                gettimeofday(&lv_tv, NULL);
                lv_rep.u.iv_ping.iv_ts_sec = lv_tv.tv_sec;
                lv_rep.u.iv_ping.iv_ts_us = lv_tv.tv_usec;
            } else {
                if (gv_verbose)
                    printf("srv: received ping, req-len=%d, expecting len=%d, setting BADCOUNT\n",
                           lv_req.iv_req_len, (int) sizeof(lv_req.u.iv_ping));
                lv_ec = XZFIL_ERR_BADCOUNT;
            }
            break;

        case GID_REQ_ID:
            if (lv_req.iv_req_len == (int) sizeof(lv_req.u.iv_id)) {
                if (gv_verbose)
                    printf("srv: received id request\n");
                lv_rep.iv_rep_type = GID_REP_ID;
                lv_rep.iv_rep_tag = lv_req.iv_req_tag;
                lv_rep.iv_rep_len = (int) sizeof(lv_rep.u.iv_id);
                lv_rep.u.iv_id.iv_com.iv_error = GID_ERR_OK;
                lv_rep.u.iv_id.iv_id = __sync_add_and_fetch_8(gp_shm, 1);
            } else {
                if (gv_verbose)
                    printf("srv: received id, req-len=%d, expecting len=%d, setting BADCOUNT\n",
                           lv_req.iv_req_len, (int) sizeof(lv_req.u.iv_id));
                lv_ec = XZFIL_ERR_BADCOUNT;
            }
            break;

        default:
            if (gv_verbose)
                printf("srv: received unknown req-type=%d, setting INVALOP\n",
                       lv_req.iv_req_type);
            lv_ec = XZFIL_ERR_INVALOP;
            break;
        }
    }

    if (lv_ec == XZFIL_ERR_OK) {
        if (gv_verbose)
            printf("srv: reply, rep-type=%d, tag=%ld, id=%ld, len=%d\n",
                   lv_rep.iv_rep_type, lv_rep.iv_rep_tag, lv_rep.u.iv_id.iv_id, lv_rep.iv_rep_len);
        lv_len = (int) sizeof(lv_rep);
    } else {
        lv_len = 0;
    }

    do_reply(pp_sre, (char *) &lv_rep, lv_len, lv_ec);
}

//
// do reply
//
void do_reply(BMS_SRE *pp_sre, char *pp_reply, int pv_len, short pv_ec) {
    if (gv_verbose)
        printf("srv: reply, len=%d, ec=%d\n", pv_len, pv_ec);
    BMSG_REPLY_(pp_sre->sre_msgId,   // msgid
                NULL,                // replyctrl
                0,                   // replyctrlsize
                pp_reply,            // replydata
                pv_len,              // replydatasize
                pv_ec,               // errorclass
                NULL);               // newphandle
}

//
// setup shared memory.
//
// if shared memory created, then initialize to current time.
//
void do_shm() {
    bool            lv_created;
    int             lv_ferr;
    int             lv_key;
    int             lv_msid;
    struct timespec lv_ts;
    unsigned long   lv_tsl;

    lv_ferr = msg_mon_get_my_segid(&lv_key);
    assert(lv_ferr == XZFIL_ERR_OK);
    if (gv_verbose)
        printf("srv: shm key=%d\n", lv_key);
    lv_msid = shmget(lv_key, sizeof(long), 0640);
    if (lv_msid == -1) {
        lv_created = true;
        if (gv_verbose)
            printf("srv: shmget failed, errno=%d\n", errno);
        lv_msid = shmget(lv_key, sizeof(long), IPC_CREAT | 0640);
        if (lv_msid == -1) {
            if (gv_verbose)
                printf("srv: shmget(IPC_CREAT) failed, errno=%d\n", errno);
            assert(lv_msid != -1);
        } else {
            if (gv_verbose)
                printf("srv: shmget(IPC_CREAT) ok\n");
        }
    } else {
        lv_created = false;
        if (gv_verbose)
            printf("srv: shmget ok\n");
    }
    gp_shm = (char *) shmat(lv_msid, NULL, 0);
    if (gp_shm == NULL) {
        if (gv_verbose)
            printf("srv: shmat failed, errno=%d\n", errno);
        assert(gp_shm != NULL);
    } else {
        gp_shml = (unsigned long *) gp_shm;
        if (gv_verbose)
            printf("srv: shmat ok, shm=%p\n", gp_shm);
        if (lv_created) {
            clock_gettime(CLOCK_REALTIME, &lv_ts);
            // nsec / 1000 => usec
            // 1,000,000 usec requires 20 bits. i.e. 2^20=1,048,576
            // unsigned long is 64 bits, so squeeze secs into leftover 44 bits
            // 100(yrs) * 365(days) * 24(hrs) * 60(min) * 60(sec)=3,153,600,000
            // requires 32 bits
            lv_tsl = ((unsigned long) lv_ts.tv_sec << 20) | ((unsigned long) lv_ts.tv_nsec / 1000);
            *gp_shml = lv_tsl;
            if (gv_verbose)
                printf("srv: initializing shm=0x%lx\n", lv_tsl);
        }
    }
}

//
// server main
//
int main(int pv_argc, char *pa_argv[]) {
    bool     lv_done;
    int      lv_ferr;
    int      lv_lerr;
    BMS_SRE  lv_sre;

    CALL_COMP_DOVERS(idtmsrv, pv_argc, pa_argv);

    do_init(pv_argc, pa_argv);

    lv_ferr = msg_mon_process_startup(true); // system messages
    assert(lv_ferr == XZFIL_ERR_OK);
    msg_mon_enable_mon_messages(true);
    lv_ferr = msg_mon_get_my_process_name(ga_name, sizeof(ga_name));
    assert(lv_ferr == XZFIL_ERR_OK);

    do_shm();

    lv_done = false;
    while (!lv_done) {
        do {
            lv_lerr = XWAIT(LREQ, -1);
            lv_lerr = BMSG_LISTEN_((short *) &lv_sre, // sre
                                   0,                 // listenopts
                                   0);                // listenertag
        } while (lv_lerr == XSRETYPE_NOWORK);
        if (lv_sre.sre_flags & XSRE_MON) {
            do_mon_msg(&lv_sre, &lv_done);
        } else {
            do_req(&lv_sre);
        }
    }

    if (gv_verbose)
        printf("server %s shutting down\n", ga_name);
    lv_ferr = msg_mon_process_shutdown();
    assert(lv_ferr == XZFIL_ERR_OK);

    return 0;
}
