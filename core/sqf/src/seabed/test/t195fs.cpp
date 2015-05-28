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

enum { MAX_FILENUMS = 20 };
enum { MAX_SERVERS  = 100 };

SB_Thread::CV cv[MAX_SERVERS];
unsigned int  seed;


void cb(SB_Phandle_Type *, struct MS_Mon_NewProcess_Notice_def *msg) {
    int status;

    status = cv[msg->tag].signal();
    assert(status == 0);
}

int rnd() {
    int rndret = (int) (100.0*rand_r(&seed)/(RAND_MAX+1.0));
    return rndret;
}

int main(int argc, char *argv[]) {
    int             arg;
    void           *buf;
    _xcc_status     cc;
    bool            client = false;
    unsigned short  count_read;
    unsigned short  count_written;
    unsigned short  count_xferred;
    int             disable = false;
    bool            errok = false;
    int             ferr;
    short           filenum[MAX_SERVERS][MAX_FILENUMS];
    short           filenumr;
    int             filenums = 1;
    int             finx;
    int             inx;
    int             linx;
    int             loop = 10;
    int             maxsp = 1;
    bool            mq = false;
    char            my_name[BUFSIZ];
    bool            noclose = false;
    bool            nowaito = false;
    bool            nowaits = false;
    int             num;
    char            prog[MS_MON_MAX_PROCESS_PATH];
    bool            rand = false;
    char            recv_buffer1[BUFSIZ];
    char            recv_buffer2[BUFSIZ];
    char            ret_name[MS_MON_MAX_PROCESS_NAME];
    char            send_buffer1[BUFSIZ];
    char            send_buffer2[BUFSIZ];
    char            serv[BUFSIZ];
    TPT_DECL2      (server_phandle, MAX_SERVERS);
    int             server_nid;
    int             server_pid;
    int             sinx;
    int             status;
    SB_Tag_Type     tag1;
    SB_Tag_Type     tag2;
    short           tfilenum;
    int             timeout = -1;
    bool            verbose = false;
    bool            wait = false;
    int             xinx;
    SB_Tag_Type     xtag;
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,     &client    },
      { "-dup",       TA_Int,  MAX_FILENUMS, &filenums  },
      { "-errok",     TA_Bool, TA_NOMAX,     &errok     },
      { "-loop",      TA_Int,  TA_NOMAX,     &loop      },
      { "-maxsp",     TA_Int,  MAX_SERVERS,  &maxsp     },
      { "-mq",        TA_Bool, TA_NOMAX,     &mq        },
      { "-noclose",   TA_Bool, TA_NOMAX,     &noclose   },
      { "-nowaito",   TA_Bool, TA_NOMAX,     &nowaito   },
      { "-nowaits",   TA_Bool, TA_NOMAX,     &nowaits   },
      { "-rand",      TA_Bool, TA_NOMAX,     &rand      },
      { "-server",    TA_Ign,  TA_NOMAX,     NULL       },
      { "-v",         TA_Bool, TA_NOMAX,     &verbose   },
      { "-wait",      TA_Bool, TA_NOMAX,     &wait      },
      { "",           TA_End,  TA_NOMAX,     NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);
    if (errok && (strcasecmp(my_name, "$srv3") == 0))
        util_abort_core_free();

    if (client) {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 1,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
        for (linx = 0; linx < loop; linx++) {
            if (mq) {
                if ((linx % 10) == 0)
                    printf("inx=%d\n", linx);
            } else
                printf("linx=%d\n", linx);
            for (sinx = 0; sinx < maxsp; sinx++) {
                sprintf(serv, "$srv%d", sinx);
                for (arg = 0; arg < argc; arg++)
                    if (strcmp(argv[arg], "-client") == 0) // start_process
                        argv[arg] = (char *) "-server";
                server_nid = -1;
                if (verbose)
                    printf("client starting server=%s\n", serv);
                // TODO: remove disable/loop - maybe
                disable = msg_test_assert_disable();
                do {
                    if (nowaits)
                        ferr = msg_mon_start_process_nowait_cb(cb,                     // callback
                                                               prog,                   // prog
                                                               serv,                   // name
                                                               ret_name,               // ret name
                                                               argc,
                                                               argv,
                                                               MS_ProcessType_Generic, // type
                                                               0,                      // priority
                                                               0,                      // debug
                                                               0,                      // backup
                                                               sinx,                   // tag
                                                               &server_nid,            // nid
                                                               &server_pid,            // pid
                                                               NULL,                   // infile
                                                               NULL);                  // outfile
                    else
                        ferr = msg_mon_start_process(prog,                   // prog
                                                     serv,                   // name
                                                     ret_name,               // ret name
                                                     argc,
                                                     argv,
                                                     TPT_REF2(server_phandle, sinx),
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
                        printf("TODO: remove this sleep - $srv already in use\n");
                        sleep(1);
                    }
                } while (ferr == XZFIL_ERR_BOUNDSERR);
                TEST_CHK_FEOK(ferr);
                msg_test_assert_enable(disable);
            }
            if (nowaits) {
                for (sinx = 0; sinx < maxsp; sinx++) {
                    sprintf(serv, "$srv%d", sinx);
                    if (verbose)
                        printf("client waiting for server=%s start\n", serv);
                    status = cv[sinx].wait(true);
                    assert(status == 0);
                }
            }
            if (verbose)
                printf("client opening server\n");
            if (errok)
                disable = msg_test_assert_disable();
            if (nowaito) {
                for (sinx = 0; sinx < maxsp; sinx++) {
                    sprintf(serv, "$srv%d", sinx);
                    for (finx = 0; finx < filenums; finx++) {
                        ferr = XFILE_OPEN_(serv,
                                           (short) strlen(serv),
                                           &filenum[sinx][finx],
                                           0, 0, 2,
                                           0,
                                           0x4000, // nowait open
                                           0, 0, NULL);
                        TEST_CHK_FEOK(ferr);
                        if (rand)
                            usleep(rnd());
                    }
                }
                for (sinx = 0; sinx < maxsp; sinx++) {
                    for (finx = 0; finx < filenums; finx++) {
                        tfilenum = filenum[sinx][finx];
                        cc = XAWAITIOX(&tfilenum,
                                       &buf,
                                       &count_xferred,
                                       &tag1,
                                       timeout,
                                       NULL);
                        if (!errok) {
                            TEST_CHK_CCEQ(cc);
                            assert(tag1 == -30);
                        }
                    }
                }
            } else {
                for (sinx = 0; sinx < maxsp; sinx++) {
                    sprintf(serv, "$srv%d", sinx);
                    for (finx = 0; finx < filenums; finx++) {
                        ferr = XFILE_OPEN_(serv,
                                           (short) strlen(serv),
                                           &filenum[sinx][finx],
                                           0, 0, 2,
                                           0, 0, 0, 0, NULL);
                        TEST_CHK_FEOK(ferr);
                    }
                }
            }
            for (sinx = 0; sinx < maxsp; sinx++) {
                if (errok && (sinx == 3)) continue;
                for (inx = 0; inx < 10; inx++) {
                    for (finx = 0; finx < filenums; finx++) {
                        if (verbose)
                            printf("client wr-1 sinx=%d, inx=%d, finx=%d\n",
                                   sinx, inx, finx);
                        sprintf(send_buffer1, "sinx=%d, inx=%d, tag=1", sinx, inx);
                        cc = XWRITEREADX(filenum[sinx][finx],
                                         send_buffer1,
                                         (short) (strlen(send_buffer1) + 1),
                                         BUFSIZ,
                                         &count_read,
                                         1);
                        if (!errok) {
                            TEST_CHK_CCEQ(cc);
                        }
                        sprintf(send_buffer2, "sinx=%d, inx=%d, tag=2", sinx, inx);
                        if (verbose)
                            printf("client wr-2 sinx=%d, inx=%d, finx=%d\n",
                                   sinx, inx, finx);
                        cc = XWRITEREADX(filenum[sinx][finx],
                                         send_buffer2,
                                         (short) (strlen(send_buffer2) + 1),
                                         BUFSIZ,
                                         &count_read,
                                         2);
                        if (!errok) {
                            TEST_CHK_CCEQ(cc);
                        }
                        tfilenum = filenum[sinx][finx];
                        if (verbose)
                            printf("client awaitio-1 sinx=%d, inx=%d, finx=%d\n",
                                   sinx, inx, finx);
                        cc = XAWAITIOX(&tfilenum,
                                       &buf,
                                       &count_xferred,
                                       &tag1,
                                       timeout,
                                       NULL);
                        if (!errok) {
                            TEST_CHK_CCEQ(cc);
                            assert(tag1 == 2);
                            num = sscanf((char *) buf, "sinx=%d, inx=%d, tag=" PFTAG,
                                         &xinx, &xinx, &xtag);
                            assert(num == 3);
                            assert(xinx == inx);
                            assert(xtag == 2);
                        }
                        tfilenum = filenum[sinx][finx];
                        if (verbose)
                            printf("client awaitio-2 sinx=%d, inx=%d, finx=%d\n",
                                   inx, inx, finx);
                        cc = XAWAITIOX(&tfilenum,
                                       &buf,
                                       &count_xferred,
                                       &tag2,
                                       timeout,
                                       NULL);
                        if (!errok) {
                            TEST_CHK_CCEQ(cc);
                            assert(tag2 == 1);
                            num = sscanf((char *) buf, "sinx=%d, inx=%d, tag=" PFTAG,
                                        &xinx, &xinx, &xtag);
                            assert(num == 3);
                            assert(xinx == inx);
                            assert(xtag == 1);
                        }
                    }
                }
            }
            if (!noclose) {
                for (sinx = 0; sinx < maxsp; sinx++) {
                    if (errok && (sinx == 3)) continue;
                    for (finx = 0; finx < filenums; finx++) {
                        if (verbose)
                            printf("client closing server sinx=%d, finx=%d\n",
                                   sinx, finx);
                        ferr = XFILE_CLOSE_(filenum[sinx][finx], 0);
                        TEST_CHK_FEOK(ferr);
                    }
                }
            }

            // wait for deaths
            for (sinx = 0; sinx < maxsp; sinx++) {
                if (verbose)
                    printf("client ru-death sinx=%d\n", sinx);
                cc = XREADUPDATEX(filenumr,
                                  recv_buffer1,
                                  BUFSIZ,
                                  &count_read,
                                  1);
                TEST_CHK_CCEQ(cc);
                tfilenum = filenumr,
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag1,
                               timeout,
                               NULL);
                TEST_CHK_CCNE(cc);
                assert(recv_buffer1[0] == XZSYS_VAL_SMSG_PROCDEATH);
                if (verbose)
                    printf("client reply-death sinx=%d\n", sinx);
                cc = XREPLYX(recv_buffer1,
                             0,
                             &count_written,
                             0,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
            }
        }
        if (errok)
            disable = msg_test_assert_disable();
        if (wait)
            sleep(5);
        printf("if there were no asserts, all is well\n");
    } else {
        if (verbose)
            printf("server opening $receive\n");
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 1,
                           2, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (finx = 0; finx < filenums; finx++) {
            if (verbose)
                printf("server ru-open finx=%d\n", finx);
            cc = XREADUPDATEX(filenumr,
                              recv_buffer1,
                              BUFSIZ,
                              &count_read,
                              1);
            TEST_CHK_CCEQ(cc);
            tfilenum = filenumr,
            cc = XAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag1,
                           timeout,
                           NULL);
            TEST_CHK_CCNE(cc);
            assert(recv_buffer1[0] == XZSYS_VAL_SMSG_OPEN);
            if (verbose)
                printf("server reply-open finx=%d\n", finx);
            cc = XREPLYX(recv_buffer1,
                         0,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        for (inx = 0; inx < 10; inx++) {
            for (finx = 0; finx < filenums; finx++) {
                if (verbose)
                    printf("server ru-1 inx=%d, finx=%d\n", inx, finx);
                cc = XREADUPDATEX(filenumr,
                                  recv_buffer1,
                                  BUFSIZ,
                                  &count_read,
                                  1);
                TEST_CHK_CCEQ(cc);
                tfilenum = filenumr,
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag1,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                if (verbose)
                    printf("server ru-2 inx=%d, finx=%d\n", inx, finx);
                cc = XREADUPDATEX(filenumr,
                                  recv_buffer2,
                                  BUFSIZ,
                                  &count_read,
                                  2);
                TEST_CHK_CCEQ(cc);
                tfilenum = filenumr,
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag2,
                               timeout,
                               NULL);
                TEST_CHK_CCEQ(cc);
                // reply in reverse order
                count_read = (short) (strlen(recv_buffer2) + 1);
                if (verbose)
                    printf("server reply-2 inx=%d, finx=%d\n", inx, finx);
                cc = XREPLYX(recv_buffer2,
                             count_read,
                             &count_written,
                             1,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
                count_read = (short) (strlen(recv_buffer1) + 1);
                if (verbose)
                    printf("server reply-1 inx=%d, finx=%d\n", inx, finx);
                cc = XREPLYX(recv_buffer1,
                             count_read,
                             &count_written,
                             0,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
            }
        }
        if (!noclose) {
            for (finx = 0; finx < filenums; finx++) {
                if (verbose)
                    printf("server ru-close finx=%d\n", finx);
                cc = XREADUPDATEX(filenumr,
                                  recv_buffer1,
                                  BUFSIZ,
                                  &count_read,
                                  1);
                TEST_CHK_CCEQ(cc);
                tfilenum = filenumr,
                cc = XAWAITIOX(&tfilenum,
                               &buf,
                               &count_xferred,
                               &tag1,
                               timeout,
                               NULL);
                TEST_CHK_CCNE(cc);
                assert(recv_buffer1[0] == XZSYS_VAL_SMSG_CLOSE);
                if (verbose)
                    printf("server reply-close finx=%d\n", finx);
                cc = XREPLYX(recv_buffer1,
                             0,
                             &count_written,
                             0,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
            }
        }
        if (verbose)
            printf("server closing $receive\n");
        ferr = XFILE_CLOSE_(filenumr);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
