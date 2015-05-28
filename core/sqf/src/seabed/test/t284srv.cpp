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

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "t284.h"

char           name[BUFSIZ];
char          *shm;
unsigned long *shml;
bool           shook   = false;
bool           verbose = false;

// forwards
void do_reply(BMS_SRE *sre, char *reply, int len, short ec);


//
// initialize
//
void do_init(int argc, char **argv) {
    int   arg;
    char *argp;
    bool  attach;
    int   ferr;

    attach = false;
    for (arg = 1; arg < argc; arg++) {
        argp = argv[arg];
        if (strcmp(argp, "-attach") == 0)
            attach = true;
        else if (strcmp(argp, "-shook") == 0)
            shook = true;
        else if (strcmp(argp, "-v") == 0)
            verbose = true;
    }
    if (attach)
        ferr = msg_init_attach(&argc, &argv, false, (char *) "$TSID0");
    else
        ferr = msg_init(&argc, &argv);
    assert(ferr == XZFIL_ERR_OK);

    if (shook)
        msg_debug_hook("s", "s");
}

//
// process monitor message.
//
// if shutdown, set done
//
void do_mon_msg(BMS_SRE *sre, bool *done) {
    int        ferr;
    MS_Mon_Msg mon_msg;

    ferr = BMSG_READDATA_(sre->sre_msgId,         // msgid
                          (char *) &mon_msg,      // reqdata
                          (int) sizeof(mon_msg)); // bytecount
    assert(ferr == XZFIL_ERR_OK);
    if (mon_msg.type == MS_MsgType_Shutdown)
        *done = true;
    if (verbose)
        printf("srv: received mon message\n");
    do_reply(sre, NULL, 0, 0);
}

//
// process non-mon message
//
void do_req(BMS_SRE *sre) {
    short           ec;
    int             ferr;
    int             len;
    GID_Rep         rep;
    GID_Req         req;
    const char     *req_type;
    struct timeval  tv;

    ec = XZFIL_ERR_OK;
    len = 0;
    if (verbose)
        printf("srv: received NON-mon message\n");
    if (sre->sre_reqDataSize < (int) sizeof(req)) {
        if (verbose)
            printf("srv: received short data - sre_reqDataSize=%d, expecting len=%d, setting BADCOUNT\n",
                   sre->sre_reqDataSize, (int) sizeof(req));
        ec = XZFIL_ERR_BADCOUNT;
    } else {
        ferr = BMSG_READDATA_(sre->sre_msgId,      // msgid
                              (char *) &req,       // reqdata
                              (int) sizeof(req));  // bytecount
        assert(ferr == XZFIL_ERR_OK);
        if (verbose) {
            switch (req.req_type) {
            case GID_REQ_PING:
                req_type = "ping";
                break;
            case GID_REQ_ID:
                req_type = "id";
                break;
            default:
                req_type = "unknown";
                break;
            }
            if (verbose)
                printf("srv: received msg. req-type=%d(%s), tag=%ld, len=%d\n",
                       req.req_type, req_type, req.req_tag, req.req_len);
        }
        switch (req.req_type) {
        case GID_REQ_PING:
            if (req.req_len == (int) sizeof(req.u.ping)) {
                if (verbose)
                    printf("srv: received ping request\n");
                rep.rep_type = GID_REP_PING;
                rep.rep_tag = req.req_tag;
                rep.rep_len = (int) sizeof(rep.u.ping);
                rep.u.ping.com.error = GID_ERR_OK;
                gettimeofday(&tv, NULL);
                rep.u.ping.ts_sec = tv.tv_sec;
                rep.u.ping.ts_us = tv.tv_usec;
            } else {
                if (verbose)
                    printf("srv: received ping, req-len=%d, expecting len=%d, setting BADCOUNT\n",
                           req.req_len, (int) sizeof(req.u.ping));
                ec = XZFIL_ERR_BADCOUNT;
            }
            break;

        case GID_REQ_ID:
            if (req.req_len == (int) sizeof(req.u.id)) {
                if (verbose)
                    printf("srv: received id request\n");
                rep.rep_type = GID_REP_ID;
                rep.rep_tag = req.req_tag;
                rep.rep_len = (int) sizeof(rep.u.id);
                rep.u.id.com.error = GID_ERR_OK;
                rep.u.id.id = __sync_add_and_fetch_8(shm, 1);
            } else {
                if (verbose)
                    printf("srv: received id, req-len=%d, expecting len=%d, setting BADCOUNT\n",
                           req.req_len, (int) sizeof(req.u.id));
                ec = XZFIL_ERR_BADCOUNT;
            }
            break;

        default:
            if (verbose)
                printf("srv: received unknown req-type=%d, setting INVALOP\n",
                       req.req_type);
            ec = XZFIL_ERR_INVALOP;
            break;
        }
    }

    if (ec == XZFIL_ERR_OK) {
        if (verbose)
            printf("srv: reply, rep-type=%d, tag=%ld, len=%d\n",
                   rep.rep_type, rep.rep_tag, rep.rep_len);
        len = (int) sizeof(rep);
    } else {
        len = 0;
    }

    do_reply(sre, (char *) &rep, len, ec);
}

//
// do reply
//
void do_reply(BMS_SRE *sre, char *reply, int len, short ec) {
    if (verbose)
        printf("srv: reply, len=%d, ec=%d\n", len, ec);
    BMSG_REPLY_(sre->sre_msgId,      // msgid
                NULL,                // replyctrl
                0,                   // replyctrlsize
                reply,               // replydata
                len,                 // replydatasize
                ec,                  // errorclass
                NULL);               // newphandle
}

//
// setup shared memory.
//
// if shared memory created, then initialize to current time.
//
void do_shm() {
    bool            created;
    int             ferr;
    int             key;
    int             msid;
    struct timespec ts;
    unsigned long   xts;

    ferr = msg_mon_get_my_segid(&key);
    assert(ferr == XZFIL_ERR_OK);
    if (verbose)
        printf("srv: shm key=%d\n", key);
    msid = shmget(key, sizeof(long), 0640);
    if (msid == -1) {
        created = true;
        if (verbose)
            printf("srv: shmget failed, errno=%d\n", errno);
        msid = shmget(key, sizeof(long), IPC_CREAT | 0640);
        if (msid == -1) {
            if (verbose)
                printf("srv: shmget(IPC_CREAT) failed, errno=%d\n", errno);
            assert(msid != -1);
        } else {
            if (verbose)
                printf("srv: shmget(IPC_CREAT) ok\n");
        }
    } else {
        created = false;
        if (verbose)
            printf("srv: shmget ok\n");
    }
    shm = (char *) shmat(msid, NULL, 0);
    if (shm == NULL) {
        if (verbose)
            printf("srv: shmat failed, errno=%d\n", errno);
        assert(shm != NULL);
    } else {
        shml = (unsigned long *) shm;
        if (verbose)
            printf("srv: shmat ok, shm=%p\n", shm);
        if (created) {
            clock_gettime(CLOCK_REALTIME, &ts);
            // nsec / 1000 => usec
            // 1,000,000 usec requires 20 bits. i.e. 2^20=1,048,576
            // unsigned long is 64 bits, so squeeze secs into leftover 44 bits
            // 100(yrs) * 365(days) * 24(hrs) * 60(min) * 60(sec)=3,153,600,000
            // requires 32 bits
            xts = ((unsigned long) ts.tv_sec << 20) | ((unsigned long) ts.tv_nsec / 1000);
            *shml = xts;
            if (verbose)
                printf("srv: initializing shm=0x%lx\n", xts);
        }
    }
}

//
// server main
//
int main(int argc, char *argv[]) {
    bool     done;
    int      ferr;
    int      lerr;
    BMS_SRE  sre;

    do_init(argc, argv);

    ferr = msg_mon_process_startup(true); // system messages
    assert(ferr == XZFIL_ERR_OK);
    msg_mon_enable_mon_messages(true);
    ferr = msg_mon_get_my_process_name(name, sizeof(name));
    assert(ferr == XZFIL_ERR_OK);

    do_shm();

    done = false;
    while (!done) {
        do {
            lerr = XWAIT(LREQ, -1);
            lerr = BMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        if (sre.sre_flags & XSRE_MON) {
            do_mon_msg(&sre, &done);
        } else {
            do_req(&sre);
        }
    }

    if (verbose)
        printf("server %s shutting down\n", name);
    ferr = msg_mon_process_shutdown();
    assert(ferr == XZFIL_ERR_OK);
    return 0;
}
