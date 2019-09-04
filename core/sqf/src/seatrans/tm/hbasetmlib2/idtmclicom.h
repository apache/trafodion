//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

typedef struct {
    union {
        short              s[6];
        struct {
            unsigned int   ctrl_size;
            unsigned int   data_size;
            unsigned int   errm;
        } t;
    } u;
} RT; // result type

// callback type
typedef int (*Hash_Cb_Type)(JNIEnv *pp_j_env, char **ppp_snames, int pv_count);

// callback
static Hash_Cb_Type g_cb = NULL;

// forwards
static int do_get_servers(MS_Mon_Process_Info_Type **ppp_pi, int *pp_count);
static int do_get_servers2(MS_Mon_Process_Info_Type *pp_pi, int pv_count, int pv_max, char **ppp_snames);

//
// callback - calculate hash
//
static int do_cb_local(JNIEnv *j_env, char **ppp_snames, int pv_count) {
    int lv_hash;

    j_env = j_env; // touch
    ppp_snames = ppp_snames; // touch
    lv_hash = (unsigned int) getpid() % pv_count;
    if (gv_verbose)
        printf("cli: do_cb_local pid=%d, hash=%d\n", getpid(), lv_hash);
    return lv_hash;
}

//
// link to process
//
static int do_link(SB_Phandle_Type *pp_phandle,
                   GID_Req         *pp_req,
                   GID_Rep         *pp_rep,
                   int              pv_timeout,
                   const char      *pp_req_text,
                   int              pv_rep_type,
                   long             pv_rep_tag,
                   size_t           pv_rep_len) {
    bool          lv_break_done;
    bool          lv_wait_done;
    int           lv_ferr;
    int           lv_lerr;
    int           lv_msgid;
    bool          lv_relink;
    RT            lv_results;
    BMS_SRE_LDONE lv_sre;
    int           lv_timeout_tics;

    if (gv_verbose)
        printf("cli: do_link ENTER\n");
    do {
        lv_relink = false;
        if (gv_verbose)
            printf("cli: linking timeout=%d, req-type=%d(%s), req-tag=0x%lx, req-len=%d\n",
                   pv_timeout,
                   pp_req->iv_req_type,
                   pp_req_text,
                   pp_req->iv_req_tag,
                   pp_req->iv_req_len);
        lv_ferr = BMSG_LINK_(pp_phandle,                  // phandle
                             &lv_msgid,                   // msgid
                             NULL,                        // reqctrl
                             0,                           // reqctrlsize
                             NULL,                        // replyctrl
                             0,                           // replyctrlmax
                             (char *) pp_req,             // reqdata
                             (int) sizeof(*pp_req),       // reqdatasize
                             (char *) pp_rep,             // replydata
                             (int) sizeof(*pp_rep),       // replydatamax
                             (SB_Tag_Type) pp_req,        // linkertag
                             0,                           // pri
                             0,                           // xmitclass
                             BMSG_LINK_LDONEQ);           // linkopts
        if (lv_ferr == XZFIL_ERR_OK) {
        	if (pv_timeout == -1){
               lv_timeout_tics = pv_timeout; // no timeout
        	}
        	else{
               lv_timeout_tics = pv_timeout / 10; // ms -> tics
        	}
            lv_break_done = false;
            lv_wait_done = false;
            do {
               lv_lerr = XWAIT(LDONE, lv_timeout_tics);
               if (gv_verbose)
                   printf("cli: XWAIT ret=%d\n", lv_lerr);
               if (lv_lerr & LDONE) {
                  lv_lerr = BMSG_LISTEN_((short *) &lv_sre,    // sre
                                         BLISTEN_ALLOW_LDONEM, // listenopts
                                         0);                   // listenertag
                  if (lv_lerr == XSRETYPE_LDONE) {
                     if (lv_sre.sre_linkTag != (SB_Tag_Type) pp_req) {
                         if (gv_verbose)
                             printf("cli: BMSG_LISTEN_ tag=0x%lx, expected tag=0x%lx\n",
                                    lv_sre.sre_linkTag, (SB_Tag_Type) pp_req);
                         assert(lv_sre.sre_linkTag == (SB_Tag_Type) pp_req);
                     }
                     lv_break_done = true;
                     lv_wait_done = true;
                     lv_ferr = BMSG_BREAK_(lv_msgid,
                                           lv_results.u.s,
                                           pp_phandle);
                     if (gv_verbose)
                         printf("cli: BMSG_BREAK_ ferr=%d\n", lv_ferr);
                     switch (lv_ferr) {
                     case XZFIL_ERR_PATHDOWN:
                        if (gv_verbose)
                            printf("cli: sleeping\n");
                        sleep(1);
                        lv_relink = true;
                        break;
                     default:
                        break;
                     }
                  } else {
                     if (gv_verbose)
                        printf("cli: BMSG_LISTEN_ did not return LDONE, ret=%d\n", lv_lerr);
                     if (lv_lerr == BSRETYPE_NOWORK){
                        if (gv_verbose)
                           printf("cli: BMSG_LISTEN_ returned BSRETYPE_NOWORK, retrying XWAIT\n");
                        continue;
                     }
                  }
               } else {
                  if (gv_verbose)
                     printf("cli: XWAIT timedout, ret=%d\n", lv_lerr);
                  lv_wait_done = true;
               }
               if (!lv_break_done) {
                  lv_ferr = BMSG_ABANDON_(lv_msgid); // cancel
                  if (gv_verbose)
                      printf("cli: BMSG_ABANDON_ ret=%d\n", lv_ferr);
                  lv_ferr = XZFIL_ERR_TIMEDOUT;
               }
               if (lv_ferr == XZFIL_ERR_OK) {
                  if (pp_rep->iv_rep_type != pv_rep_type) {
                     if (gv_verbose)
                         printf("cli: rep-type=%d, expecting rep-type=%d, SETTING FSERR\n",
                                pp_rep->iv_rep_type, pv_rep_type);
                     lv_ferr = XZFIL_ERR_FSERR;
                  } else if (pp_rep->iv_rep_tag != pv_rep_tag) {
                     if (gv_verbose)
                        printf("cli: rep-tag=0x%lx, expecting rep-tag=0x%lx, SETTING FSERR\n",
                                pp_rep->iv_rep_tag, pv_rep_tag);
                     lv_ferr = XZFIL_ERR_FSERR;
                  } else if (pp_rep->iv_rep_len != (int) pv_rep_len) {
                     if (gv_verbose)
                        printf("cli: rep-len=%d, expecting rep-len=%d, SETTING FSERR\n",
                                pp_rep->iv_rep_len, (int) pv_rep_len);
                     lv_ferr = XZFIL_ERR_FSERR;
                  }
               }
            } while (! lv_wait_done);

         } else {
            if (gv_verbose)
               printf("cli: BMSG_LINK_ ret=%d\n", lv_ferr);
         }
     } while (lv_relink);
     if (gv_verbose)
        printf("cli: do_link EXIT ret=%d\n", lv_ferr);

     return lv_ferr;
}

//
// initialize request
//
static void init_req(GID_Req      *pp_req,
                     GID_REQ_TYPE  pv_req_type,
                     size_t        pv_req_len) {
    static int lv_req_seq = 0;
    long       lv_req_tag;
    long       lv_req_pid;

    lv_req_pid = (long) getpid() << 32;
    lv_req_tag = lv_req_pid | __sync_add_and_fetch_4(&lv_req_seq, 1);

    pp_req->iv_req_type = pv_req_type;
    pp_req->iv_req_tag = lv_req_tag;
    pp_req->iv_req_len = (int) pv_req_len;
}

//
// initialize reply
//
static void init_rep(GID_Rep *pp_rep) {
    pp_rep = pp_rep; // touch
}

//
// id operation
//
static int do_cli_id(SB_Phandle_Type *pp_phandle, int pv_timeout, long *pp_id) {
    int     lv_ferr;
    GID_Rep lv_rep;
    GID_Req lv_req;

    if (gv_verbose)
        printf("cli: get id with timeout %d\n", pv_timeout);

    init_req(&lv_req, GID_REQ_ID, sizeof(lv_req.u.iv_id));
    init_rep(&lv_rep);
    lv_ferr = do_link(pp_phandle,
                      &lv_req,
                      &lv_rep,
                      pv_timeout,
                      "id",
                      GID_REP_ID,
                      lv_req.iv_req_tag,
                      sizeof(lv_rep.u.iv_id));
    if (lv_ferr == XZFIL_ERR_OK) {
        if (gv_verbose)
            printf("cli: id-reply, rep-tag=0x%lx, rep-len=%d, id=0x%lx\n",
                   lv_rep.iv_rep_tag, lv_rep.iv_rep_len, lv_rep.u.iv_id.iv_id);
        *pp_id = lv_rep.u.iv_id.iv_id;
    }
    return lv_ferr;
}

//
// id_to_string operation
//
static int do_cli_id_to_string(SB_Phandle_Type *pp_phandle, int pv_timeout, unsigned long pv_id, char* pp_id_string) {
    int     lv_ferr;
    GID_Rep lv_rep;
    GID_Req lv_req;

    if (gv_verbose)
        printf("cli: do_cli_id_to_string begin for id=0x%lx\n", pv_id);

    init_req(&lv_req, GID_REQ_ID_TO_STRING, sizeof(lv_req.u.iv_id_to_string));
    init_rep(&lv_rep);
    lv_req.u.iv_id_to_string.iv_req_id_to_string = pv_id;
    lv_ferr = do_link(pp_phandle,
                      &lv_req,
                      &lv_rep,
                      pv_timeout,
                      "id_to_string",
                      GID_REP_ID_TO_STRING,
                      lv_req.iv_req_tag,
                      sizeof(lv_rep.u.iv_id_to_string));

    if (lv_ferr == XZFIL_ERR_OK) {
        if (gv_verbose)
            printf("cli: id-to-string-reply, rep-tag=0x%lx, rep-len=%d, id-string=%s, size=%d\n",
                   lv_rep.iv_rep_tag, lv_rep.iv_rep_len, lv_rep.u.iv_id_to_string.iv_id_to_string,
                   (int)strlen(lv_rep.u.iv_id_to_string.iv_id_to_string));
        strcpy(pp_id_string, lv_rep.u.iv_id_to_string.iv_id_to_string);
    }
    return lv_ferr;
}

//
// string_to_id operation
//
static int do_cli_string_to_id(SB_Phandle_Type *pp_phandle, int pv_timeout, unsigned long *pp_id, char* pp_id_string) {
    int     lv_ferr;
    GID_Rep lv_rep;
    GID_Req lv_req;

    if (gv_verbose)
        printf("cli: do_cli_string_to_id begin for string=%s\n", pp_id_string);

    init_req(&lv_req, GID_REQ_STRING_TO_ID, sizeof(lv_req.u.iv_string_to_id));
    init_rep(&lv_rep);
    strcpy(lv_req.u.iv_string_to_id.iv_string_to_id,pp_id_string);
    lv_ferr = do_link(pp_phandle,
                      &lv_req,
                      &lv_rep,
                      pv_timeout,
                      "string_to_id",
                      GID_REP_STRING_TO_ID,
                      lv_req.iv_req_tag,
                      sizeof(lv_rep.u.iv_string_to_id));

    if (lv_ferr == XZFIL_ERR_OK) {
    	lv_ferr = lv_rep.u.iv_string_to_id.iv_com.iv_error;
        if (gv_verbose)
            printf("cli: string-to-id-reply, error=%d, rep-tag=0x%lx, rep-len=%d, id=0x%lx\n",
                   lv_ferr, lv_rep.iv_rep_tag, lv_rep.iv_rep_len, lv_rep.u.iv_string_to_id.iv_string_to_id);
        *pp_id = lv_rep.u.iv_string_to_id.iv_string_to_id;
    }
    return lv_ferr;
}

//
// open
//
static int do_cli_open(JNIEnv *j_env, SB_Phandle_Type *pp_phandle, int *pp_oid) {
    enum { MAX_P = 100 };
    MS_Mon_Process_Info_Type *lp_pi;
    char                     *lp_sname;
    char                     *lp_snames[MAX_P];
    int                       lv_count;
    int                       lv_ferr;
    int                       lv_hash;
    int                       lv_scount;

    if (gv_verbose)
         printf("cli: do_cli_open() pid=%d\n", getpid());
    lv_ferr = do_get_servers(&lp_pi, &lv_count);
    if (lv_ferr == XZFIL_ERR_OK) {
        lv_scount = do_get_servers2(lp_pi, lv_count, MAX_P, lp_snames);
        if (lv_scount > 0) {
            if (gv_j_cb != NULL)
                lv_hash = do_cb(j_env, lp_snames, lv_scount);
            else if (g_cb != NULL)
                lv_hash = g_cb(j_env, lp_snames, lv_scount);
            else
                lv_hash = do_cb_local(j_env, lp_snames, lv_scount);
            lp_sname = lp_snames[lv_hash];
            if (gv_verbose)
                printf("cli: pid=%d, scount=%d, hash=%d, sname=%s\n",
                       getpid(),
                       lv_scount,
                       lv_hash,
                       lp_sname);
    
            lv_ferr = msg_mon_open_process(lp_sname,    // name
                                           pp_phandle,
                                           pp_oid);
            if (lv_ferr != XZFIL_ERR_OK) {
                if (gv_verbose)
                    printf("cli: msg_mon_open_process(%s) returned err=%d\n",
                           lp_sname, lv_ferr);
            }
        } else {
            if (gv_verbose)
                printf("cli: no servers, setting NOTFOUND\n");
            lv_ferr = XZFIL_ERR_NOTFOUND;
        }
    } else {
        if (gv_verbose)
            printf("cli: msg_mon_get_process_info_type returned err=%d\n",
                   lv_ferr);
    }
    if (gv_verbose)
        printf("cli: do_cli_open returning err=%d\n", lv_ferr);

    if (lp_pi != NULL)
        delete [] lp_pi;

    return lv_ferr;
}

//
// ping operation
//
static int do_cli_ping(SB_Phandle_Type *pp_phandle, int pv_timeout) {
    int     lv_ferr;
    GID_Rep lv_rep;
    GID_Req lv_req;

    init_req(&lv_req, GID_REQ_PING, sizeof(lv_req.u.iv_ping));
    init_rep(&lv_rep);
    lv_ferr = do_link(pp_phandle,
                      &lv_req,
                      &lv_rep,
                      pv_timeout,
                      "ping",
                      GID_REP_PING,
                      lv_req.iv_req_tag,
                      sizeof(lv_rep.u.iv_ping));
    if (lv_ferr == XZFIL_ERR_OK) {
        if (gv_verbose)
            printf("cli: ping-reply, rep-tag=0x%lx, rep-len=%d, ts=%ld.%ld\n",
                   lv_rep.iv_rep_tag, lv_rep.iv_rep_len, lv_rep.u.iv_ping.iv_ts_sec, lv_rep.u.iv_ping.iv_ts_us);
    }
    return lv_ferr;
}

//
// register hash callback
//
static int do_reg_hash_cb(Hash_Cb_Type pv_cb_in) {
    if (gv_verbose)
        printf("cli: do_reg_hash_cb\n");
    g_cb = pv_cb_in;
    return 0;
}

//
// get servers
//
static int do_get_servers(MS_Mon_Process_Info_Type **ppp_pi,
                          int                       *pp_count) {
    int lv_ferr;
    int lv_tmpcount;

    // Added retries to get the 'idtm' process info. 
    // Reason: The DTM process is now a primitive process and
    // the monitor starts it up right at the outset. At that point 
    // the 'idtm' may not be up. The TM (java) code wants to get the
    // ID from the idtm server during its initialisation.

    int lv_Retries = 0;
    int lc_maxRetries = 200;
    int lc_Pause = 3000; // 3 seconds

    do {

      if (lv_Retries > 0) {
        if (gv_verbose) {
            printf("cli: do_get_servers, retry#%d, going to sleep for %d ms.\n", lv_Retries, lc_Pause);
        }
        SB_Thread::Sthr::sleep(lc_Pause); // in msec
      }

      lv_ferr = msg_mon_get_process_info_type(MS_ProcessType_TMID,
                                              pp_count,
                                              0,      // max
                                              NULL);  // info
      if (gv_verbose) {
        printf("cli: do_get_servers process type TMID err=%d, num_servers=%d\n", lv_ferr, *pp_count);
      }

      lv_Retries++;
    }
    while ((lv_Retries <= lc_maxRetries) && 
           ((lv_ferr != XZFIL_ERR_OK) ||
            (*pp_count <= 0)))
      ;

    if (lv_ferr == XZFIL_ERR_OK) {
        *ppp_pi = new MS_Mon_Process_Info_Type[*pp_count];
        lv_ferr = msg_mon_get_process_info_type(MS_ProcessType_TMID,
                                                &lv_tmpcount,
                                                *pp_count, // max
                                                *ppp_pi);  // info
    } else {
        *ppp_pi = NULL;
    }

    return lv_ferr;
}

//
// get servers part II
//
static int do_get_servers2(MS_Mon_Process_Info_Type  *pp_pi,
                           int                        pv_count,
                           int                        pv_max,
                           char                     **ppp_snames) {
    char *lp_sname;
    int   lv_inx;
    int   lv_scount;

    lv_scount = 0;
    for (lv_inx = 0; lv_inx < pv_count; lv_inx++) {
        lp_sname = pp_pi[lv_inx].process_name;
        if (gv_verbose)
            printf("cli(%d): sname=%s\n", getpid(), lp_sname?lp_sname:"");
        if (memcmp(lp_sname, "$TSID", 5) == 0) {
            if (isdigit(lp_sname[5])) {
                if (gv_verbose)
                    printf("cli: sname=%s\n", lp_sname);
                assert(lv_scount < pv_max);
                ppp_snames[lv_scount] = lp_sname;
                lv_scount++;
            }
        }
    }
    return lv_scount;
}

