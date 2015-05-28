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
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fs.h"
#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "ttrans.h"
#include "tutil.h"
#include "tutilp.h"
#include "ufsri.h"

char                      my_name[BUFSIZ];
int                       my_nid;
int                       my_pid;
char                      open_sender[BUFSIZ];
short                     open_sender_file_number;
int                       open_sender_nid;
TPT_DECL                 (open_sender_phandle);
int                       open_sender_pid;
char                      recv_buffer[BUFSIZ];
char                      send_buffer[BUFSIZ];
char                      sender[BUFSIZ];
int                       sender_nid;
int                       sender_pid;
const char               *server_name = "$SRV";
short                     server_name_len;
int                       server_nid = -1;
TPT_DECL                 (server_phandle);
int                       server_pid;

void start_server(int argc, char **argv) {
    int  arg;
    int  disable;
    int  ferr;
    char prog[MS_MON_MAX_PROCESS_PATH];
    char ret_name[MS_MON_MAX_PROCESS_NAME];

    sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
    for (arg = 0; arg < argc; arg++)
        if (strcmp(argv[arg], "-client") == 0) // start_process
            argv[arg] = (char *) "-server";
    // TODO: remove disable/loop
    disable = msg_test_assert_disable();
    do {
        server_nid = my_nid;
        ferr = msg_mon_start_process(prog,                   // prog
                                     (char *) server_name,   // name
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
        if (ferr == XZFIL_ERR_BOUNDSERR) {
            printf("TODO: remove this sleep - BOUNDSERR\n");
            sleep(1);
        }
        if (ferr == XZFIL_ERR_FSERR) {
            printf("TODO: remove this sleep - FSERR\n");
            sleep(1);
        }
    } while ((ferr == XZFIL_ERR_BOUNDSERR) || (ferr == XZFIL_ERR_FSERR));
    TEST_CHK_FEOK(ferr);
    msg_test_assert_enable(disable);
}

int main(int argc, char *argv[]) {
    _bcc_status         bcc;
    bool                client = false;
    int                 count_read;
    int                 count_written;
    int                 disable;
    int                 err;
    short               error_reply;
    int                 ferr;
    short               filenum;
    int                 inx;
    short               lasterr;
    int                 loop = 10;
    int                 msgnum;
    bool                open = false;
    short               sender_len;
    xzsys_ddl_smsg_def *sys_msg = (xzsys_ddl_smsg_def *) recv_buffer;
    RI_Type             ri;
    TAD                 zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(true);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);

    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_process_info(NULL, &my_nid, &my_pid);
    TEST_CHK_FEOK(ferr);
    if (client) {
        printf("client name=%s, nid=%d, pid=%d\n",
               my_name, my_nid, my_pid);
        start_server(argc, argv);
        server_name_len = (short) strlen(server_name);
        ferr = BFILE_OPEN_((char *) server_name, server_name_len, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);

        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            disable = msg_test_assert_disable();
            bcc = BWRITEREADX(filenum,
                              send_buffer,
                              (int) (strlen(send_buffer) + 1), // cast
                              BUFSIZ,
                              &count_read,
                              0);
            msg_test_assert_enable(disable);
            if (_bstatus_eq(bcc))
                printf("%s\n", send_buffer);
            else {
                ferr = XFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
                printf("WRITEREAD error=%d\n", lasterr);
            }
            kill(server_pid, SIGKILL);
            for (;;) {
                err = kill(server_pid, 0);
                if ((err == -1) && (errno == ESRCH))
                    break;
                usleep(10000);
            }
            start_server(argc, argv);
        }
        kill(server_pid, SIGKILL);
        for (;;) {
            err = kill(server_pid, 0);
            if ((err == -1) && (errno == ESRCH))
                break;
            usleep(10000);
        }
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        printf("server name=%s, nid=%d, pid=%d\n",
               my_name, my_nid, my_pid);
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            bcc = BREADUPDATEX(filenum,
                               recv_buffer,
                               BUFSIZ,
                               &count_read,
                               0);
            getri(&ri);
            printri(&ri);
            sender_len = 0;
            sender_nid = -1;
            sender_pid = -1;
            disable = msg_test_assert_disable(); // errors are ok
            ferr = XPROCESSHANDLE_DECOMPOSE_(TPT_REF(ri.sender),
                                             &sender_nid, // cpu
                                             &sender_pid, // pin
                                             NULL, // nodenumber
                                             NULL, // nodename
                                             0,    // nodename
                                             NULL, // nodename_length
                                             sender,
                                             sizeof(sender),
                                             &sender_len,
                                             NULL); // sequence_number
            TEST_CHK_FEIGNORE(ferr);
            msg_test_assert_enable(disable);
            sender[sender_len] = 0;
            printf("sender=%s, nid=%d, pid=%d\n",
                   sender, sender_nid, sender_pid);
            error_reply = XZFIL_ERR_OK;
            if (_bstatus_eq(bcc)) {
                if (open) {
                    assert(sender_nid == open_sender_nid);
                    assert(sender_pid == open_sender_pid);
                    assert(strcmp(sender, open_sender) == 0);
                    assert(ri.file_number == open_sender_file_number);
                    strcat(recv_buffer, "- reply from ");
                    strcat(recv_buffer, my_name);
                    count_read = (int) (strlen(recv_buffer) + 1); // cast
                } else {
                    printf("server not opened by client - returning WRONGID\n");
                    error_reply = XZFIL_ERR_WRONGID;
                }
            } else {
                msgnum = sys_msg->u_z_msg.z_msgnumber[0];
                switch (msgnum) {
                case XZSYS_VAL_SMSG_OPEN:
                    printf("msgnum=%d (open)\n", msgnum);
                    assert(!open);
                    TPT_COPY_INT(TPT_REF(open_sender_phandle), ri.sender);
                    strcpy(open_sender, sender);
                    open_sender_nid = sender_nid;
                    open_sender_pid = sender_pid;
                    open_sender_file_number = ri.file_number;
                    open = true;
                    break;
                case XZSYS_VAL_SMSG_CLOSE:
                    printf("msgnum=%d (close)\n", msgnum);
                    assert(open);
                    open = false;
                    break;
                case XZSYS_VAL_SMSG_SHUTDOWN:
                    printf("msgnum=%d (shutdown)\n", msgnum);
                    inx = loop; // exit
                    break;
                default:
                    printf("unexpected msgnum=%d\n", msgnum);
                    abort();
                }
                count_read = 0;
                inx--;
            }
            bcc = BREPLYX(recv_buffer,
                          count_read,
                          &count_written,
                          0,
                          error_reply);
            TEST_CHK_BCCEQ(bcc);
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    printf("if there were no asserts, all is well\n");
    return 0;
}
