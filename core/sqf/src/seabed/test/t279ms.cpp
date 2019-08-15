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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

#include "phan.h"

enum { EVENT_ID = 9 };

SB_Thread::CV  cv;
char           my_mon_name[BUFSIZ];
bool           verbose = false;
char          *vn;

void cb(SB_Phandle_Type *, struct MS_Mon_NewProcess_Notice_def *msg) {
    int status;

    msg = msg; // touch
    status = cv.signal();
    assert(status == 0);
}

void get_msg(const char *who) {
    int     lerr;
    BMS_SRE sre;

    if (verbose)
        printf("%s: waiting for msg\n", who);
    do {
        lerr = XWAIT(LREQ, -1);
        TEST_CHK_WAITIGNORE(lerr);
        lerr = XMSG_LISTEN_((short *) &sre, // sre
                            0,              // listenopts
                            0);             // listenertag
    } while (lerr == XSRETYPE_NOWORK);
    XMSG_REPLY_(sre.sre_msgId,       // msgid
                NULL,                // replyctrl
                0,                   // replyctrlsize
                NULL,                // replydata
                0,                   // replydatasize
                0,                   // errorclass
                NULL);               // newphandle
    if (verbose)
        printf("%s: msg done\n", who);
}

int main(int argc, char *argv[]) {
    int                       arg;
#ifdef SQ_PHANDLE_VERIFIER
    char                      arg_verif[20];
#endif
    bool                      client = false;
    const char               *client_name = "$cli";
#ifdef SQ_PHANDLE_VERIFIER
    char                      client_name_seq1[100];
    char                      client_name_seq2[100];
    int                       client_nid;
    int                       client_pid;
    SB_Verif_Type             client_verifier;
#endif
    char                      core_file[MS_MON_MAX_PROCESS_PATH];
#ifdef SQ_PHANDLE_VERIFIER
    int                       disable;
#endif
    int                       err;
    int                       ferr;
    int                       inx;
    int                       loop = 10;
    int                       msgid;
#ifdef SQ_PHANDLE_VERIFIER
    int                       my_compid;
    int                       my_mon_nid;
    int                       my_mon_pid;
    int                       my_mon_ptype;
    SB_Verif_Type             my_mon_verifier;
    int                       my_mon_zid;
    int                       my_os_pid;
    long                      my_os_tid;
    TPT_DECL                 (my_phandle);
    int                       my_pnid;
#endif
    int                       oid;
#ifdef SQ_PHANDLE_VERIFIER
    MS_Mon_Process_Info_Type  pinfo;
#endif
    char                      prog[MS_MON_MAX_PROCESS_PATH];
    RT                        results;
    char                      ret_name[MS_MON_MAX_PROCESS_NAME];
    bool                      save = false;
    const char               *server_name1 = "$SRV";
    const char               *server_name2 = "$SRV2";
#ifdef SQ_PHANDLE_VERIFIER
    const char               *server_name3 = "$SRV3";
#endif
    char                      server_name_seq1[100];
#ifdef SQ_PHANDLE_VERIFIER
    char                      server_name_seq2[100];
#endif
    int                       server_nid1 = -1;
    int                       server_nid2 = -1;
    TPT_DECL                 (server_phandle1);
#ifdef SQ_PHANDLE_VERIFIER
    TPT_DECL                 (server_phandle2);
    SB_Phandle                server_phandle3;
#endif
    int                       server_pid1;
    int                       server_pid2;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type             server_verifier1;
    SB_Verif_Type             server_verifier2;
#endif
    bool                      shell = false;
    bool                      sleepv = false;
    bool                      server2 = false;
    int                       status;
#ifdef SQ_PHANDLE_VERIFIER
    int                       tnid;
    int                       tpid;
    SB_Verif_Type             tverifier;
    MS_Mon_Transid_Type       transid;
#endif
    int                       verif = -1;
    struct stat               statbuf;
    TAD                       zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-save",      TA_Bool, TA_NOMAX,    &save      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-server2",   TA_Bool, TA_NOMAX,    &server2   },
      { "-shell",     TA_Bool, TA_NOMAX,    &shell     },
      { "-sleep",     TA_Bool, TA_NOMAX,    &sleepv    },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-verif",     TA_Int,  TA_NOMAX,    &verif     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    vn = getenv("SQ_VIRTUAL_NODES");
    arg_proc_args(zargs, false, argc, argv);
    if (client && shell)
        ferr = msg_init_attach(&argc, &argv, true, (char *) client_name);
    else
        ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    util_test_start(client);
    ferr = msg_mon_process_startup(false); // system messages
    TEST_CHK_FEOK(ferr);

#ifdef SQ_PHANDLE_VERIFIER
    // acquire some info
    my_mon_nid      = -1;
    my_mon_pid      = -1;
    my_mon_ptype    = -1;
    my_mon_zid      = -1;
    my_os_pid       = -1;
    my_os_tid       = -1;
    my_compid       = -1;
    my_pnid         = -1;
    my_mon_verifier = -1;
    ferr = msg_mon_get_my_info4(&my_mon_nid,
                                &my_mon_pid,
                                my_mon_name,
                                BUFSIZ,         // mon_name_len
                                &my_mon_ptype,
                                &my_mon_zid,
                                &my_os_pid,
                                &my_os_tid,
                                &my_compid,
                                &my_pnid,
                                &my_mon_verifier);
    util_check("msg_mon_get_my_info4", ferr);
    printf("nid=%d, pid=%d, name=%s, ptype=%d, zid=%d, os-pid=%d, os-tid=%ld, compid=%d, pnid=%d, verifier=%d\n",
           my_mon_nid,
           my_mon_pid,
           my_mon_name,
           my_mon_ptype,
           my_mon_zid,
           my_os_pid,
           my_os_tid,
           my_compid,
           my_pnid,
           my_mon_verifier);
    assert(my_mon_verifier > 0);

    ferr = msg_mon_get_process_name2(my_mon_nid,
                                     my_mon_pid,
                                     my_mon_verifier,
                                     my_mon_name);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_process_info_detail(my_mon_name, &pinfo);
    TEST_CHK_FEOK(ferr);
    assert(pinfo.verifier == my_mon_verifier);
    if (!client)
        assert(pinfo.parent_verifier == verif);
#endif

    if (client && !shell) {
        sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
        for (arg = 0; arg < argc; arg++)
            if (strcmp(argv[arg], "-client") == 0) // start_process
                argv[arg] = (char *) "-server";
#ifdef SQ_PHANDLE_VERIFIER
        argv[argc] = (char *) "-verif";
        argv[argc + 1] = arg_verif;
        sprintf(arg_verif, "%d", my_mon_verifier);
        argc += 2;
#endif
        if (vn == NULL)
            server_nid1 = 0; // real cluster needs same node
        if (verbose)
            printf("client: starting server\n");
        ferr = msg_mon_start_process(prog,                   // prog
                                     (char *) server_name1,  // name
                                     ret_name,               // ret name
                                     argc,
                                     argv,
                                     TPT_REF(server_phandle1),
                                     0,                      // open
                                     NULL,                   // oid
                                     MS_ProcessType_Generic, // type
                                     0,                      // priority
                                     0,                      // debug
                                     0,                      // backup
                                     &server_nid1,           // nid
                                     &server_pid1,           // pid
                                     NULL,                   // infile
                                     NULL                    // outfile
#ifdef SQ_PHANDLE_VERIFIER
                                    ,&server_verifier1       // verifier
#endif
                                    );
        TEST_CHK_FEOK(ferr);
        argv[argc] = (char *) "-server2";
        argc++;
        if (vn == NULL)
            server_nid2 = 0; // real cluster needs same node
        ferr =
          msg_mon_start_process_nowait_cb(cb,                     // cb
                                          prog,                   // prog
                                          (char *) server_name2,  // name
                                          ret_name,               // ret name
                                          argc,                   // argc
                                          argv,                   // argv
                                          MS_ProcessType_Generic, // type
                                          0,                      // priority
                                          0,                      // debug
                                          0,                      // backup
                                          0,                      // tag
                                          &server_nid2,           // nid
                                          &server_pid2,           // pid
                                          NULL,                   // infile
                                          NULL                    // outfile
#ifdef SQ_PHANDLE_VERIFIER
                                         ,&server_verifier2       // verifier
#endif
                                         );
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("client waiting for server=%s start\n", server_name2);
        status = cv.wait(true);
        assert(status == 0);

        // process-wait for client/server/shell
        ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 3, NULL, false);
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("client: server is ready\n");

#ifdef SQ_PHANDLE_VERIFIER
        ferr = msg_mon_get_process_info2((char *) client_name,
                                         &client_nid,
                                         &client_pid,
                                         &client_verifier);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_create_name_seq((char *) client_name, // cast
                                       client_verifier,
                                       client_name_seq1,
                                       (int) sizeof(client_name_seq1)); // cast
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_create_name_seq((char *) client_name, // cast
                                       client_verifier + 99,
                                       client_name_seq2,
                                       (int) sizeof(client_name_seq2)); // cast
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_create_name_seq((char *) server_name1, // cast
                                       server_verifier1,
                                       server_name_seq1,
                                       (int) sizeof(server_name_seq1)); // cast
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_create_name_seq((char *) server_name2, // cast
                                       server_verifier1 + 99,
                                       server_name_seq2,
                                       (int) sizeof(server_name_seq2)); // cast
        TEST_CHK_FEOK(ferr);

        transid.id[0] = -1;
        transid.id[1] = -1;
        transid.id[2] = -1;
        transid.id[3] = -1;

        // check name
        ferr = msg_mon_register_death_notification_name((char *) server_name1); // cast
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_deregister_death_notification_name((char *) server_name1, // cast
                                                          transid);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_register_death_notification_name(server_name_seq1);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_deregister_death_notification_name(server_name_seq1,
                                                          transid);
        TEST_CHK_FEOK(ferr);

        // check name2
        ferr = msg_mon_register_death_notification_name2((char *) client_name, // cast
                                                         (char *) server_name1); // cast
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_deregister_death_notification_name((char *) server_name1, // cast
                                                          transid);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_register_death_notification_name2(client_name_seq1, // cast
                                                         (char *) server_name1); // cast
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_deregister_death_notification_name((char *) server_name1, // cast
                                                          transid);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_register_death_notification_name2((char *) client_name, // cast
                                                         server_name_seq1);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_deregister_death_notification_name(server_name_seq1, // cast
                                                          transid);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_register_death_notification_name2(client_name_seq1, // cast
                                                         server_name_seq1);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_deregister_death_notification_name(server_name_seq1,
                                                          transid);
        TEST_CHK_FEOK(ferr);

        disable = msg_test_assert_disable();
        // make sure seq # is checked
        ferr = msg_mon_register_death_notification_name(server_name_seq2);
        assert(ferr != XZFIL_ERR_OK);
        ferr = msg_mon_register_death_notification_name2(client_name_seq2,
                                                         server_name_seq2);
        assert(ferr != XZFIL_ERR_OK);
        msg_test_assert_enable(disable);

        // check old
        ferr = msg_mon_register_death_notification4(server_nid1,
                                                    server_pid1,
                                                    server_verifier1);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_deregister_death_notification3(client_nid,
                                                      client_pid,
                                                      client_verifier,
                                                      server_nid1,
                                                      server_nid1,
                                                      server_verifier1,
                                                      transid);
        TEST_CHK_FEOK(ferr);
#else
        strcpy(server_name_seq1, server_name1);
#endif

        for (inx = 0; inx < 2; inx++) {
            if (inx == 0) {
                if (verbose)
                    printf("client: opening server inx=%d\n", inx);
                ferr = msg_mon_open_process(server_name_seq1,
                                            TPT_REF(server_phandle1),
                                            &oid);
                TEST_CHK_FEOK(ferr);

#ifdef SQ_PHANDLE_VERIFIER
                int           tcpu;
                int           tpin;
                char          tprocname[100];
                short         tprocname_size = 100;
                short         tprocname_len;
                SB_Int64_Type tseq;
                XPROCESSHANDLE_DECOMPOSE_(TPT_REF(server_phandle1),
                                          &tcpu,
                                          &tpin,
                                          NULL,       // nodenumber
                                          NULL,       // nodename
                                          XOMITSHORT, // nodename_maxlen
                                          NULL,       // nodename_length
                                          tprocname,
                                          tprocname_size,
                                          &tprocname_len,
                                          &tseq);
                assert(tcpu == server_nid1);
                assert(tpin == server_pid1);
                assert(tprocname_len > 0);
                tprocname[tprocname_len] = '\0';
                assert(strcmp(server_name1, tprocname) == 0);
                assert(tseq == server_verifier1);

                ferr = XPROCESSHANDLE_COMPARE_(TPT_REF(server_phandle1),
                                               TPT_REF(server_phandle1));
                assert(ferr == 2); // should be identical
                ferr = XPROCESSHANDLE_GETMINE_(TPT_REF(my_phandle));
                assert(ferr == 0);
                ferr = XPROCESSHANDLE_COMPARE_(TPT_REF(server_phandle1),
                                               TPT_REF(my_phandle));
                assert(ferr == 0); // should be unrelated
                memcpy(&server_phandle3,
                       TPT_REF(server_phandle1),
                       sizeof(server_phandle3));
                ferr = XPROCESSHANDLE_COMPARE_(TPT_REF(server_phandle1),
                                               (SB_Phandle_Type *) &server_phandle3);
                assert(ferr == 2); // should be identical
                server_phandle3.iv_pid = ~server_phandle3.iv_pid; // alter pid
                ferr = XPROCESSHANDLE_COMPARE_(TPT_REF(server_phandle1),
                                               (SB_Phandle_Type *) &server_phandle3);
                assert(ferr == 1); // should be pair
                server_phandle3.iv_pid = ~server_phandle3.iv_pid; // fix pid
                server_phandle3.iv_verifier = ~server_phandle3.iv_verifier;
                ferr = XPROCESSHANDLE_COMPARE_(TPT_REF(server_phandle1),
                                               (SB_Phandle_Type *) &server_phandle3);
                assert(ferr == 1); // should be pair

                ferr = msg_mon_get_process_info_detail(server_name_seq1, &pinfo);
                TEST_CHK_FEOK(ferr);
                disable = msg_test_assert_disable();
                ferr = msg_mon_get_process_info_detail(server_name_seq2, &pinfo);
                assert(ferr != XZFIL_ERR_OK);
                ferr = msg_mon_get_process_info_detail((char *) server_name3, &pinfo);
                assert(ferr != XZFIL_ERR_OK);
                msg_test_assert_enable(disable);

                ferr = msg_mon_get_process_info(server_name_seq1, &tnid, &tpid);
                TEST_CHK_FEOK(ferr);
                assert(tnid == server_nid1);
                assert(tpid == server_pid1);
                disable = msg_test_assert_disable();
                ferr = msg_mon_get_process_info(server_name_seq2, &tnid, &tpid);
                assert(ferr != XZFIL_ERR_OK);
                ferr = msg_mon_get_process_info((char *) server_name3, &tnid, &tpid);
                assert(ferr != XZFIL_ERR_OK);
                msg_test_assert_enable(disable);

                ferr = msg_mon_get_process_info2(server_name_seq1, &tnid, &tpid, &tverifier);
                TEST_CHK_FEOK(ferr);
                assert(tnid == server_nid1);
                assert(tpid == server_pid1);
                assert(tverifier == server_verifier1);
                disable = msg_test_assert_disable();
                ferr = msg_mon_get_process_info2(server_name_seq2, &tnid, &tpid, &tverifier);
                assert(ferr != XZFIL_ERR_OK);
                ferr = msg_mon_get_process_info2((char *) server_name3, &tnid, &tpid, &tverifier);
                assert(ferr != XZFIL_ERR_OK);
                msg_test_assert_enable(disable);

                disable = msg_test_assert_disable();
                // make sure seq # is checked
                ferr = msg_mon_open_process(server_name_seq2,
                                            TPT_REF(server_phandle2),
                                            &oid);
                assert(ferr != XZFIL_ERR_OK);
                msg_test_assert_enable(disable);
#endif

                ferr = XMSG_LINK_(TPT_REF(server_phandle1),    // phandle
                                  &msgid,                      // msgid
                                  NULL,                        // reqctrl
                                  0,                           // reqctrlsize
                                  NULL,                        // replyctrl
                                  0,                           // replyctrlmax
                                  NULL,                        // reqdata
                                  0,                           // reqdatasize
                                  NULL,                        // replydata
                                  0,                           // replydatamax
                                  0,                           // linkertag
                                  0,                           // pri
                                  0,                           // xmitclass
                                  0);                          // linkopts
                util_check("XMSG_LINK_", ferr);
                ferr = XMSG_BREAK_(msgid,
                                   results.u.s,
                                   TPT_REF(server_phandle1));
                util_check("XMSG_BREAK_", ferr);
                assert(results.u.t.ctrl_size == 0);
                assert(results.u.t.data_size == 0);
            }
            if (verbose)
                printf("client: dumping server inx=%d\n", inx);
            if (inx == 0) {
                ferr = msg_mon_dump_process_name(NULL,
                                                 server_name1,
                                                 core_file);
                TEST_CHK_FEOK(ferr);
            } else {
                ferr = msg_mon_dump_process_name(NULL,
                                                 server_name_seq1,
                                                 core_file);
                TEST_CHK_FEOK(ferr);

#ifdef SQ_PHANDLE_VERIFIER
                disable = msg_test_assert_disable();
                // make sure seq # is checked
                ferr = msg_mon_dump_process_name(NULL,
                                                 server_name_seq2,
                                                 core_file);
                assert(ferr != XZFIL_ERR_OK);
                msg_test_assert_enable(disable);
#endif
            }

            printf("core-file=%s\n", core_file);
            char *pch;
            pch= strtok (core_file,":");
            pch = strtok (NULL,":");
            printf("pch=%s\n", pch);
            err = stat(pch, &statbuf);
            assert(err == 0);
            if (!save)
                unlink(pch);
            if ((loop > 1) && sleepv)
                sleep(1);
        }
    }
    if (client && shell)
        sleep(4);
    else if (!client) {
        if (server2) {
            if (verbose)
                printf("server2: sleeping\n");
            for (;;) {
                sleep(1);
            }
        }
#ifdef SQ_PHANDLE_VERIFIER
        ferr = msg_mon_get_process_info2((char *) client_name,
                                         &client_nid,
                                         &client_pid,
                                         &client_verifier);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_create_name_seq((char *) client_name, // cast
                                       client_verifier,
                                       client_name_seq1,
                                       (int) sizeof(client_name_seq1)); // cast
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_create_name_seq((char *) client_name, // cast
                                       client_verifier + 99,
                                       client_name_seq2,
                                       (int) sizeof(client_name_seq2)); // cast
        TEST_CHK_FEOK(ferr);

        ferr =
          msg_mon_event_send_name(client_name,                // name
                                  MS_ProcessType_Undefined,   // process-type
                                  EVENT_ID,                   // event-id
                                  0,                          // event-len
                                  NULL);                      // event-data
        TEST_CHK_FEOK(ferr);
        ferr =
          msg_mon_event_send_name(client_name_seq1,           // name
                                  MS_ProcessType_Undefined,   // process-type
                                  EVENT_ID,                   // event-id
                                  0,                          // event-len
                                  NULL);                      // event-data
        TEST_CHK_FEOK(ferr);
        disable = msg_test_assert_disable();
        // make sure seq # is checked
        ferr =
          msg_mon_event_send_name(client_name_seq2,           // name
                                  MS_ProcessType_Undefined,   // process-type
                                  EVENT_ID,                   // event-id
                                  0,                          // event-len
                                  NULL);                      // event-data
        assert(ferr != XZFIL_ERR_OK);
        msg_test_assert_enable(disable);
#else
        int           cli_nid;
        int           cli_pid;
        ferr = msg_mon_get_process_info((char *) "$cli",
                                         &cli_nid,
                                         &cli_pid);

        if (verbose)
            printf("server: sending event\n");
        ferr =
          msg_mon_event_send(cli_nid,                    // nid
                             cli_pid,                    // pid
                             MS_ProcessType_Undefined,   // process-type
                             EVENT_ID,                   // event-id
                             0,                          // event-len
                             NULL);                      // event-data
        TEST_CHK_FEOK(ferr);
#endif
        get_msg("server");
        get_msg("server");
        if (verbose)
            printf("server: sleeping\n");
        for (;;) {
            sleep(1);
        }
    }
    if (client && !shell) {
        if (verbose)
            printf("client: stopping server\n");

#ifdef SQ_PHANDLE_VERIFIER
        disable = msg_test_assert_disable();
        // make sure seq # is checked
        ferr = msg_mon_stop_process_name(server_name_seq2);
        assert(ferr != XZFIL_ERR_OK);
        msg_test_assert_enable(disable);
        ferr = msg_mon_stop_process_name((char *) server_name_seq1);
        TEST_CHK_FEIGNORE(ferr);

        ferr = msg_mon_stop_process_name((char *) server_name2);
        TEST_CHK_FEIGNORE(ferr);
#else
        ferr = msg_mon_stop_process((char *) server_name1, -1, -1);
        TEST_CHK_FEIGNORE(ferr);
#endif
    }

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
