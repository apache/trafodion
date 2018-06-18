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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fs.h"
#include "seabed/ms.h"
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

char   my_name[BUFSIZ];
char   recv_buffer[BUFSIZ];
char   send_buffer[BUFSIZ];

SB_Thread::CV cv;
#ifdef SQ_PHANDLE_VERIFIER
SB_Verif_Type srv_verifier = -1;
#endif

void cb(SB_Phandle_Type *, struct MS_Mon_NewProcess_Notice_def *msg) {
    int status;

#ifdef SQ_PHANDLE_VERIFIER
    srv_verifier = msg->verifier;
#else
    msg = msg; // touch
#endif
    status = cv.signal();
    assert(status == 0);
}

int main(int argc, char *argv[]) {
    int             arg;
    _xcc_status     cc;
    bool            client = false;
    unsigned short  count_read;
    unsigned short  count_written;
    bool            death = false;
#ifdef SQ_PHANDLE_VERIFIER
    int             disable;
#endif
    int             ferr;
    short           filenumr;
    short           filenums;
#ifdef SQ_PHANDLE_VERIFIER
    short           filenums2;
#endif
    int             inxl;
    short           len;
#ifdef SQ_PHANDLE_VERIFIER
    short           len2;
#endif
    int             loop = 10;
    int             nid;
    int             pid;
    char            prog[MS_MON_MAX_PROCESS_PATH];
    char            retname[BUFSIZ];
    char            sname[10];
    char            sname_seq1[40];
#ifdef SQ_PHANDLE_VERIFIER
    char            sname_seq2[40];
#endif
    int             status;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-death",     TA_Bool, TA_NOMAX,    &death     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    for (arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "-client") == 0) // start_process
            argv[arg] = (char *) "-server";
    }
    util_test_start(client);
    if (death)
        ferr = file_mon_process_startup(true);     // system messages?
    else
        ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);

    if (client) {
        if (death) {
            ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                               0, 0, 0,
                               1, 0, // sys msg
                               0, 0, NULL);
            TEST_CHK_FEOK(ferr);
        }
        strcpy(sname, "$srv");
        if (verbose)
            printf("client starting server %s\n", sname);
        sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
        nid = -1;
        ferr =
          msg_mon_start_process_nowait_cb(cb,                    // callback
                                          prog,                  // prog
                                          sname,                 // name
                                          retname,               // ret-name
                                          argc,                  // argc
                                          argv,                  // argv
                                          MS_ProcessType_Generic,// ptype
                                          0,                     // priority
                                          false,                 // debug
                                          false,                 // backup
                                          0,                     // tag
                                          &nid,                  // nid
                                          &pid,                  // pid
                                          NULL,                  // infile
                                          NULL);                 // outfile
        TEST_CHK_FEOK(ferr);
        if (verbose)
            printf("client waiting for server=%s start\n", sname);
        status = cv.wait(true);
        assert(status == 0);

#ifdef SQ_PHANDLE_VERIFIER
        ferr = msg_mon_create_name_seq(sname,
                                       srv_verifier,
                                       sname_seq1,
                                       (int) sizeof(sname_seq1));
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_create_name_seq(sname,
                                       srv_verifier + 99,
                                       sname_seq2,
                                       (int) sizeof(sname_seq2));
        TEST_CHK_FEOK(ferr);
#else
        strcpy(sname_seq1, sname);
#endif

        len = (short) strlen(sname_seq1);
        if (verbose)
            printf("client opening server %s\n", sname);
        ferr = XFILE_OPEN_(sname_seq1, len, &filenums,
                           0, 0, 0, 0,
                           0,
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);

#ifdef SQ_PHANDLE_VERIFIER
        len2 = (short) strlen(sname_seq2);
        disable = msg_test_assert_disable();
        // make sure seq # is checked
        ferr = XFILE_OPEN_(sname_seq2, len2, &filenums2,
                           0, 0, 0, 0,
                           0,
                           0, 0, NULL);
        assert(ferr != XZFIL_ERR_OK);
        msg_test_assert_enable(disable);
#endif

        if (verbose)
            printf("open filenums=%d\n", filenums);
        for (inxl = 0; inxl < loop; inxl++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inxl);
            cc = XWRITEREADX(filenums,
                             send_buffer,
                             (unsigned short) (strlen(send_buffer) + 1), // cast
                             BUFSIZ,
                             &count_read,
                             0);
            TEST_CHK_CCEQ(cc);
            printf("%s\n", send_buffer);
        }
        disable = msg_test_assert_disable();
        ferr = XFILE_CLOSE_(filenums, 0);
        TEST_CHK_FEOK(ferr);
        msg_test_assert_enable(disable);
        if (death) {
            sleep(2);
        }

        if (verbose)
            printf("client calling shutdown\n");
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        cc = XREADUPDATEX(filenumr,
                          recv_buffer,
                          BUFSIZ,
                          &count_read,
                          0);
        TEST_CHK_CCNE(cc);
        count_read = 0;
        cc = XREPLYX(recv_buffer,
                     count_read,
                     &count_written,
                     0,
                     XZFIL_ERR_OK);
        TEST_CHK_CCEQ(cc);
        for (inxl = 0; inxl < loop; inxl++) {
            cc = XREADUPDATEX(filenumr,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCEQ(cc);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (unsigned short) (strlen(recv_buffer) + 1); // cast
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        if (death) {
            sleep(1);
            util_abort_core_free();
        }
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
