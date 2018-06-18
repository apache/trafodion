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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fs.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_OUT = 15 };

bool                      verbose = false;
int                       server_nid = -1;
TPT_DECL                 (server_phandle);
int                       server_pid;
char                     *srv1 = (char *) "$srv1";
char                     *srv2 = (char *) "$srv2";
char                     *vn;

void restart_server(int argc, char **argv) {
    int  arg;
    int  disable;
    int  ferr;
    char prog[MS_MON_MAX_PROCESS_PATH];
    char ret_name[MS_MON_MAX_PROCESS_NAME];

    sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
    for (arg = 0; arg < argc; arg++)
        if (strcmp(argv[arg], "-client") == 0) // start_process
            argv[arg] = (char *) "-server";
    printf("restarting server\n");
    for (int inx = 0; inx < 10; inx++) {
        server_nid = -1;
        if (vn == NULL)
            server_nid = 0; // real cluster needs same node
        disable = msg_test_assert_disable(); // allow for errors in start
        ferr = msg_mon_start_process(prog,                   // prog
                                     srv1,                   // name
                                     ret_name,               // ret name
                                     argc,
                                     argv,
                                     TPT_REF(server_phandle),
                                     0,                      // open
                                     NULL,                   // oid
                                     MS_ProcessType_Generic, // type
                                     0,                      // priority
                                     0,                      // debug
                                     0,                      // backup
                                     &server_nid,            // nid
                                     &server_pid,            // pid
                                     NULL,                   // infile
                                     NULL);                  // outfile
        msg_test_assert_enable(disable);
        if (ferr == XZFIL_ERR_OK) break;
        sleep(1);
        printf("retrying-start\n");
    }
    TEST_CHK_FEOK(ferr);
}

int main(int argc, char *argv[]) {
    void                     *buf;
    _xcc_status               cc;
    bool                      client = false;
    unsigned short            count_written;
    unsigned short            count_xferred;
    int                       disable;
    bool                      done;
    int                       ferr;
    short                     filenum1;
    short                     filenum2;
    int                       inx;
    int                       loop = 10;
    char                      my_name[BUFSIZ];
    char                      my_pname[BUFSIZ];
    int                       out;
    MS_Mon_Process_Info_Type  proc_info;
    char                      recv_buffer[BUFSIZ];
    char                      send_buffer[BUFSIZ];
    int                       send_len;
    SB_Tag_Type               tag;
    int                       timeout = -1;
    short                     tfilenum;
    TAD                       zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    vn = getenv("SQ_VIRTUAL_NODES");
    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, (int) sizeof(my_name));
    ferr = msg_mon_get_my_process_name(my_pname, sizeof(my_pname));

    if (client) {
        ferr = XFILE_OPEN_(srv1, (short) strlen(srv1), &filenum1,
                           0, 0, MAX_OUT, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_get_process_info_detail(srv1, &proc_info);
        TEST_CHK_FEOK(ferr);
        ferr = XFILE_OPEN_(srv2, (short) strlen(srv2), &filenum2,
                           0, 0, MAX_OUT, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            send_len = (int) strlen(send_buffer) + 1; // cast
            if (verbose)
                printf("client doing writereadx's to fn1\n");
            for (out = 0; out < MAX_OUT; out++) {
                cc = XWRITEREADX(filenum1,
                                 send_buffer,
                                 (ushort) send_len,
                                 BUFSIZ,
                                 &count_written,
                                 0);
                TEST_CHK_CCEQ(cc);
            }
            if (verbose)
                printf("client doing writereadx's to fn2\n");
            for (out = 0; out < MAX_OUT; out++) {
                cc = XWRITEREADX(filenum2,
                                 send_buffer,
                                 (ushort) send_len,
                                 BUFSIZ,
                                 &count_written,
                                 0);
                TEST_CHK_CCEQ(cc);
            }
            if (verbose)
                printf("client killing srv1 (pid=%d), closing fn1\n",
                       proc_info.os_pid);
            kill(proc_info.os_pid, SIGTERM);
            disable = msg_test_assert_disable();
            ferr = XFILE_CLOSE_(filenum1);
            TEST_CHK_FEIGNORE(ferr);
            msg_test_assert_enable(disable);
            if (verbose)
                printf("client doing awaitios on fn1\n");
            for (out = 0; out < MAX_OUT; out++) {
                tfilenum = -2;
                disable = msg_test_assert_disable();
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag,
                               timeout,
                               NULL);
                msg_test_assert_enable(disable);
                // can't check cc because of -2
                if (_xstatus_eq(cc))
                    printf("%s\n", send_buffer);
            }
            if (verbose)
                printf("client restarting srv1, reopening fn1\n");
            restart_server(argc, argv);
            ferr = XFILE_OPEN_(srv1, (short) strlen(srv1), &filenum1,
                               0, 0, MAX_OUT, 0, 0, 0, 0, NULL);
            TEST_CHK_FEOK(ferr);
            ferr = msg_mon_get_process_info_detail(srv1, &proc_info);
        }
        if (verbose)
            printf("client sending stop to srv1\n");
        strcpy(send_buffer, "stop");
        send_len = (int) strlen(send_buffer) + 1; // cast
        cc = XWRITEREADX(filenum1,
                         send_buffer,
                         (ushort) send_len,
                         BUFSIZ,
                         &count_written,
                         0);
        TEST_CHK_CCEQ(cc);
        tfilenum = filenum1;
        cc = XAWAITIOX(&tfilenum,
                       &buf,
                       &count_xferred,
                       &tag,
                       timeout,
                       NULL);
        TEST_CHK_CCEQ(cc);
        strcpy(send_buffer, "stop");
        if (verbose)
            printf("client sending stop to srv2\n");
        cc = XWRITEREADX(filenum2,
                         send_buffer,
                         (ushort) send_len,
                         BUFSIZ,
                         &count_written,
                         0);
        TEST_CHK_CCEQ(cc);
        tfilenum = filenum2;
        cc = XAWAITIOX(&tfilenum,
                       &buf,
                       &count_xferred,
                       &tag,
                       timeout,
                       NULL);
        TEST_CHK_CCEQ(cc);
        if (verbose)
            printf("client finishing\n");
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum1,
                           0, 0, 0,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        do {
            if (verbose)
                printf("server-%s: doing readupdate\n", my_pname);
            cc = XREADUPDATEX(filenum1,
                              recv_buffer,
                              BUFSIZ,
                              NULL,
                              0);
            if (verbose)
                printf("server-%s: readupdate done cc=%d\n", my_pname, cc);
            TEST_CHK_CCEQ(cc);
            util_time_sleep_ms(10);
            done = (strcmp(recv_buffer, "stop") == 0);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            if (verbose)
                printf("server-%s: doing reply, done=%d\n", my_pname, done);
            cc = XREPLYX(NULL,
                         0,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
            assert(count_written == 0);
        } while (!done);
        ferr = XFILE_CLOSE_(filenum1, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
