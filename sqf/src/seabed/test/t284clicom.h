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

// callback type
typedef int (*Hash_Cb_Type)(JNIEnv *j_env, char **snames, int count);

// callback
static Hash_Cb_Type g_cb = NULL;

// forwards
static int do_get_servers(MS_Mon_Process_Info_Type **pia, int *count);
static int do_get_servers2(MS_Mon_Process_Info_Type *pia, int count, int max, char **snames);

//
// callback - calculate hash
//
static int do_cb_local(JNIEnv *j_env, char **snames, int count) {
    int hash;

    j_env = j_env; // touch
    snames = snames; // touch
    hash = (unsigned int) getpid() % count;
    if (verbose)
        printf("cli: do_cb_local pid=%d, hash=%d\n", getpid(), hash);
    return hash;
}

//
// link to process
//
static int do_link(SB_Phandle_Type *phandle,
                   GID_Req         *req,
                   GID_Rep         *rep,
                   int              timeout,
                   const char      *req_text,
                   int              rep_type,
                   long             rep_tag,
                   size_t           rep_len) {
    bool          break_done;
    int           ferr;
    int           lerr;
    int           msgid;
    bool          relink;
    RT            results;
    BMS_SRE_LDONE sre;
    int           timeout_tics;

    if (verbose)
        printf("cli: do_link ENTER\n");
    do {
        relink = false;
        if (verbose)
            printf("cli: linking timeout=%d, req-type=%d(%s), req-tag=0x%lx, req-len=%d\n",
                   timeout,
                   req->req_type,
                   req_text,
                   req->req_tag,
                   req->req_len);
        ferr = BMSG_LINK_(phandle,                     // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          NULL,                        // replyctrl
                          0,                           // replyctrlmax
                          (char *) req,                // reqdata
                          (int) sizeof(*req),          // reqdatasize
                          (char *) rep,                // replydata
                          (int) sizeof(*rep),          // replydatamax
                          (SB_Tag_Type) req,           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          BMSG_LINK_LDONEQ);           // linkopts
        if (ferr == XZFIL_ERR_OK) {
            timeout_tics = timeout / 10; // ms -> tics
            break_done = false;
            lerr = XWAIT(LDONE, timeout_tics);
            if (lerr & LDONE) {
                lerr = BMSG_LISTEN_((short *) &sre,// sre
                                    BLISTEN_ALLOW_LDONEM,// listenopts
                                    0);                  // listenertag
                if (lerr == XSRETYPE_LDONE) {
                    if (sre.sre_linkTag != (SB_Tag_Type) req) {
                        if (verbose)
                            printf("cli: BMSG_LISTEN_ tag=0x%lx, expected tag=0x%lx\n",
                                   sre.sre_linkTag, (SB_Tag_Type) req);
                        assert(sre.sre_linkTag == (SB_Tag_Type) req);
                    }
                    break_done = true;
                    ferr = BMSG_BREAK_(msgid,
                                       results.u.s,
                                       phandle);
                    if (verbose)
                        printf("cli: BMSG_BREAK_ ferr=%d\n", ferr);
                    switch (ferr) {
                    case XZFIL_ERR_PATHDOWN:
                        if (verbose)
                            printf("cli: sleeping\n");
                        sleep(1);
                        relink = true;
                        break;
                    default:
                        break;
                    }
                } else {
                    if (verbose)
                        printf("cli: BMSG_LISTEN_ did not return LDONE, ret=%d\n", lerr);
                }
            } else {
                if (verbose)
                    printf("cli: XWAIT timedout\n");
            }
            if (!break_done) {
                ferr = BMSG_ABANDON_(msgid); // cancel
                if (verbose)
                    printf("cli: BMSG_ABANDON_ ret=%d\n", ferr);
                ferr = XZFIL_ERR_TIMEDOUT;
            }
            if (ferr == XZFIL_ERR_OK) {
                if (rep->rep_type != rep_type) {
                    if (verbose)
                        printf("cli: rep-type=%d, expecting rep-type=%d, SETTING FSERR\n",
                               rep->rep_type, rep_type);
                    ferr = XZFIL_ERR_FSERR;
                } else if (rep->rep_tag != rep_tag) {
                    if (verbose)
                        printf("cli: rep-tag=0x%lx, expecting rep-tag=0x%lx, SETTING FSERR\n",
                               rep->rep_tag, rep_tag);
                    ferr = XZFIL_ERR_FSERR;
                } else if (rep->rep_len != (int) rep_len) {
                    if (verbose)
                        printf("cli: rep-len=%d, expecting rep-len=%d, SETTING FSERR\n",
                               rep->rep_len, (int) rep_len);
                    ferr = XZFIL_ERR_FSERR;
                }
            }
        } else {
            if (verbose)
                printf("cli: BMSG_LINK_ ret=%d\n", ferr);
        }
    } while (relink);
    if (verbose)
        printf("cli: do_link EXIT ret=%d\n", ferr);

    return ferr;
}

//
// initialize request
//
static void init_req(GID_Req      *req,
                     GID_REQ_TYPE  req_type,
                     size_t        req_len) {
    static int req_seq = 0;
    long       req_tag;
    long       req_pid;

    req_pid = (long) getpid() << 32;
    req_tag = req_pid | __sync_add_and_fetch_4(&req_seq, 1);

    req->req_type = req_type;
    req->req_tag = req_tag;
    req->req_len = (int) req_len;
}

//
// initialize reply
//
static void init_rep(GID_Rep *rep) {
    rep = rep; // touch
}

//
// id operation
//
static int do_cli_id(SB_Phandle_Type *phandle, int timeout, long *id) {
    int     ferr;
    GID_Rep rep;
    GID_Req req;

    init_req(&req, GID_REQ_ID, sizeof(req.u.id));
    init_rep(&rep);
    ferr = do_link(phandle,
                   &req,
                   &rep,
                   timeout,
                   "id",
                   GID_REP_ID,
                   req.req_tag,
                   sizeof(rep.u.id));
    if (ferr == XZFIL_ERR_OK) {
        if (verbose)
            printf("cli: id-reply, rep-tag=0x%lx, rep-len=%d, id=0x%lx\n",
                   rep.rep_tag, rep.rep_len, rep.u.id.id);
        *id = rep.u.id.id;
    }
    return ferr;
}

//
// open
//
static int do_cli_open(JNIEnv *j_env, SB_Phandle_Type *phandle, int *oid) {
    enum { MAX_P = 100 };
    int                       count;
    int                       ferr;
    int                       hash;
    MS_Mon_Process_Info_Type *pia;
    int                       scount;
    char                     *sname;
    char                     *snames[MAX_P];

    ferr = do_get_servers(&pia, &count);
    if (ferr == XZFIL_ERR_OK) {
        scount = do_get_servers2(pia, count, MAX_P, snames);
        if (scount > 0) {
            if (j_cb != NULL)
                hash = do_cb(j_env, snames, scount);
            else if (g_cb != NULL)
                hash = g_cb(j_env, snames, scount);
            else
                hash = do_cb_local(j_env, snames, scount);
            sname = snames[hash];
            if (verbose)
                printf("cli: pid=%d, scount=%d, hash=%d, sname=%s\n",
                       getpid(),
                       scount,
                       hash,
                       sname);
    
            ferr = msg_mon_open_process(sname,       // name
                                        phandle,
                                        oid);
            if (ferr != XZFIL_ERR_OK) {
                if (verbose)
                    printf("cli: msg_mon_open_process(%s) returned err=%d\n",
                           sname, ferr);
            }
        } else {
            if (verbose)
                printf("cli: no servers, setting NOTFOUND\n");
            ferr = XZFIL_ERR_NOTFOUND;
        }
    } else {
        if (verbose)
            printf("cli: msg_mon_get_process_info_type returned err=%d\n",
                   ferr);
    }
    if (verbose)
        printf("cli: do_cli_open returning err=%d\n", ferr);

    if (pia != NULL)
        delete [] pia;

    return ferr;
}

//
// ping operation
//
static int do_cli_ping(SB_Phandle_Type *phandle, int timeout) {
    int     ferr;
    GID_Rep rep;
    GID_Req req;

    init_req(&req, GID_REQ_PING, sizeof(req.u.ping));
    init_rep(&rep);
    ferr = do_link(phandle,
                   &req,
                   &rep,
                   timeout,
                   "ping",
                   GID_REP_PING,
                   req.req_tag,
                   sizeof(rep.u.ping));
    if (ferr == XZFIL_ERR_OK) {
        if (verbose)
            printf("cli: ping-reply, rep-tag=0x%lx, rep-len=%d, ts=%ld.%ld\n",
                   rep.rep_tag, rep.rep_len, rep.u.ping.ts_sec, rep.u.ping.ts_us);
    }
    return ferr;
}

//
// register hash callback
//
static int do_reg_hash_cb(Hash_Cb_Type cb_in) {
    if (verbose)
        printf("cli: do_reg_hash_cb\n");
    g_cb = cb_in;
    return 0;
}

//
// get servers
//
static int do_get_servers(MS_Mon_Process_Info_Type **pia,
                          int                       *count) {
    int ferr;
    int tmpcount;

    ferr = msg_mon_get_process_info_type(MS_ProcessType_Generic,
                                         count,
                                         0,      // max
                                         NULL);  // info
    if (ferr == XZFIL_ERR_OK) {
        *pia = new MS_Mon_Process_Info_Type[*count];
        ferr = msg_mon_get_process_info_type(MS_ProcessType_Generic,
                                             &tmpcount,
                                             *count, // max
                                             *pia);  // info
    } else {
        *pia = NULL;
    }

    return ferr;
}

//
// get servers part II
//
static int do_get_servers2(MS_Mon_Process_Info_Type  *pia,
                           int                        count,
                           int                        max,
                           char                     **snames) {
    int   inx;
    int   scount;
    char *sname;

    scount = 0;
    for (inx = 0; inx < count; inx++) {
        sname = pia[inx].process_name;
        if (memcmp(sname, "$TSID", 5) == 0) {
            if (isdigit(sname[5])) {
                if (verbose)
                    printf("cli: sname=%s\n", sname);
                assert(scount < max);
                snames[scount] = sname;
                scount++;
            }
        }
    }
    return scount;
}

