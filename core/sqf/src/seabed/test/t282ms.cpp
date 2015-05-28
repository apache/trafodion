//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2015 Hewlett-Packard Development Company, L.P.
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

#include <sys/wait.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_CLIENTS = 100 };
enum { MAX_SEND    = 1 * 1024 * 1024 };
enum { MAX_OUT     = 5 };

MS_Mon_Process_Info_Type info[1];
char                     my_name[BUFSIZ];
char                     prog[MS_MON_MAX_PROCESS_PATH];
char                     recv_buffer[MAX_OUT][MAX_SEND];
short                    recv_buffer2[MAX_OUT][BUFSIZ];
short                    recv_buffer3[MAX_OUT][BUFSIZ];
char                     send_buffer[MAX_SEND];
short                    send_buffer2[BUFSIZ];

int rnd(int max) {
    static unsigned int seed = 0;
    int rndret = (int) (1.0 * max*rand_r(&seed)/(RAND_MAX+1.0));
    return rndret;
}

void rats(int sig) {
    printf("rats, sig=%d\n", sig);
    msg_mon_process_shutdown();
//abort();
    exit(1);
}

int main(int argc, char *argv[]) {
    bool            attach = false;
    bool            chook = false;
    int             cli_oid;
    TPT_DECL       (cli_phandle);
    bool            client = false;
    bool            client2 = false;
    const char     *cname = "$cli";
    int             count;
    int             ferr;
    int             inx;
    int             len;
    int             lerr;
    int             loop = 10;
    int             maxcp = 1;
    int             maxsp = 1;
    MS_Mon_Msg     *msg;
    int             msgid[MAX_OUT];
    int             oid;
    TPT_DECL       (phandle);
    char           *pname = NULL;
    RT              results;
    bool            shook = false;
    const char     *sname = "$srv";
    BMS_SRE         sre;
    bool            verbose = false;
    TAD             zargs[] = {
      { "-attach",    TA_Bool, TA_NOMAX,    &attach    },
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-client2",   TA_Bool, TA_NOMAX,    &client2   },
      { "-cname",     TA_Str,  TA_NOMAX,    &cname     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcp",     TA_Int,  MAX_CLIENTS, &maxcp     },
      { "-maxsp",     TA_Int,  MAX_CLIENTS, &maxsp     },
      { "-name",      TA_Str,  TA_NOMAX,    &pname     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "-sname",     TA_Str,  TA_NOMAX,    &sname     },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    signal(SIGINT, rats);
    arg_proc_args(zargs, false, argc, argv);
    if (client && chook)
        test_debug_hook("c", "c");
    if (!client && shook)
        test_debug_hook("s", "s");

    sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
    if (attach)
        msfs_util_init_attach(&argc, &argv, msg_debug_hook, false, pname);
    else
        msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    msg_mon_enable_mon_messages(true);
    ferr = msg_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);

    if (client) {
        for (inx = 1; inx < argc; inx++) {
            if (strcmp(argv[inx], "-client") == 0)
                argv[inx] = (char *) "-client2";
        }
        for (inx = 0; inx < 2; inx++) {
            char pname[10];
            sprintf(pname, "$CLI%d", inx);
            ferr = msg_mon_start_process(prog,                   // prog
                                         pname,                  // name
                                         NULL,                   // ret name
                                         argc,
                                         argv,
                                         TPT_REF(cli_phandle),
                                         0,                      // open
                                         &cli_oid,   
                                         MS_ProcessType_Generic, // type
                                         0,                      // priority
                                         0,                      // debug
                                         0,                      // backup
                                         NULL,                   // nid
                                         NULL,                   // pid
                                         NULL,                   // infile
                                         NULL);                  // outfile
            TEST_CHK_FEOK(ferr);
        }
    } else if (client2) {
        int disable = msg_test_assert_disable();
        ferr = msg_mon_open_process((char *) sname,       // name
                                    TPT_REF(phandle),
                                    &oid);
        if (ferr != XZFIL_ERR_OK) {
            int xpid;
            msg_mon_get_process_info((char *) "$SRV", NULL, &xpid);
            kill(xpid, SIGABRT);
            msg_mon_get_process_info((char *) "$CLI", NULL, &xpid);
            kill(xpid, SIGABRT);
        }
        msg_test_assert_enable(disable);
        TEST_CHK_FEOK(ferr);
    } else {
    }

    util_gethostname(my_name, sizeof(my_name));
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (client) {
        ferr = msg_mon_get_process_info_type(MS_ProcessType_Generic,
                                             &count,
                                             0,
                                             NULL);
        TEST_CHK_FEOK(ferr);
        if (!attach)
            assert(count > 1);
    } else if (client2) {
    } else {
    }

    if (client) {
        if (verbose)
            printf("client-%d: starting\n", getpid());
    } else if (client2) {
        if (verbose)
            printf("client2-%d: starting\n", getpid());
    } else {
        if (verbose)
            printf("server-%d: starting\n", getpid());
    }
    if (client) {
        int which_c = 0;
        for (inx = 0; ; inx++) {
            do {
                int xrnd = rnd(100);
                lerr = XWAIT(LREQ, xrnd);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = BMSG_LISTEN_((short *) &sre,     // sre
                                    0,                  // listenopts
                                    0);                 // listenertag
                if (lerr == BSRETYPE_NOWORK) {
                    char pname[10];
                    sprintf(pname, "$CLI%d", which_c);
                    ferr = msg_mon_get_process_info(pname, NULL, NULL);
                    if (ferr == XZFIL_ERR_OK)
                    {
                        if (verbose)
                            printf("client2-%d: stopping pname=%s\n", getpid(), pname);
                        int disable = msg_test_assert_disable();
                        for (;;) {
                            msg_mon_stop_process_name(pname);
                            ferr = msg_mon_get_process_info(pname, NULL, NULL);
                            if (ferr != XZFIL_ERR_OK)
                                break;
                            if (verbose)
                                printf("client2-%d: sleeping - waiting on pname=%s\n", getpid(), pname);
                            sleep(1);
                        }
                        msg_test_assert_enable(disable);
                        which_c = 1 - which_c;
                    }
                }
            } while (lerr == BSRETYPE_NOWORK);
            if (sre.sre_flags & BSRE_MON) {
                ferr = BMSG_READDATA_(sre.sre_msgId,        // msgid
                                      recv_buffer[0],       // reqdata
                                      BUFSIZ);              // bytecount
                util_check("BMSG_READDATA_", ferr);
                msg = (MS_Mon_Msg *) recv_buffer[0];
                if (verbose) {
                    const char *mtype = msg_mon_text_get_mon_msg_type(msg->type);
                    if (msg->type == MS_MsgType_ProcessDeath)
                        printf("client2-%d: received mon message, msg-type=%d(death), name=%s\n", getpid(), msg->type, msg->u.death.process_name);
                    else
                        printf("client2-%d: received mon message, msg-type=%d(%s)\n", getpid(), msg->type, mtype);
                }
                if ((msg->type == MS_MsgType_Close) ||
                    (msg->type == MS_MsgType_ProcessDeath)) {
                    char *pname;
                    if (msg->type == MS_MsgType_Close) {
                        pname = msg->u.close.process_name;
                    } else {
                        pname = msg->u.death.process_name;
                    }
                    int disable = msg_test_assert_disable();
                    ferr = msg_mon_get_process_info(pname, NULL, NULL);
                    msg_test_assert_enable(disable);
                    if (ferr != XZFIL_ERR_OK) {
                        if (verbose)
                            printf("client-%d: starting client %s\n", getpid(), pname);
                        for (;;) {
                            int disable = msg_test_assert_disable();
                            ferr = msg_mon_start_process(prog,                   // prog
                                                         pname,                  // name
                                                         NULL,                   // ret name
                                                         argc,
                                                         argv,
                                                         TPT_REF(cli_phandle),
                                                         0,                      // open
                                                         &cli_oid,   
                                                         MS_ProcessType_Generic, // type
                                                         0,                      // priority
                                                         0,                      // debug
                                                         0,                      // backup
                                                         NULL,                   // nid
                                                         NULL,                   // pid
                                                         NULL,                   // infile
                                                         NULL);                  // outfile
                            msg_test_assert_enable(disable);
                            if (ferr == XZFIL_ERR_OK)
                                break;
                            if (ferr == XZFIL_ERR_FSERR) {
                                if (verbose)
                                    printf("client-%d: waiting\n", getpid());
                                sleep(1);
                                continue;
                            }
                            TEST_CHK_FEOK(ferr);
                        }
                    }
                }
            }
        }
    } else if (client2) {
        for (inx = 0; ; inx++) {
//          if (verbose)
//              printf("client-%d: linking\n", getpid());
            for (int xx = 0; xx < MAX_OUT; xx++) {
                ferr = BMSG_LINK_(TPT_REF(phandle),            // phandle
                                  &msgid[xx],                  // msgid
                                  send_buffer2,                // reqctrl
                                  (ushort) (inx & 1),          // reqctrlsize
                                  recv_buffer3[xx],            // replyctrl
                                  1,                           // replyctrlmax
                                  send_buffer,                 // reqdata
                                  MAX_SEND,                    // reqdatasize
                                  recv_buffer[xx],             // replydata
                                  MAX_SEND,                    // replydatamax
                                  0,                           // linkertag
                                  (short) (inx+1),             // pri
                                  0,                           // xmitclass
                                  0);                          // linkopts
                util_check("BMSG_LINK_", ferr);
            }
//          if (verbose)
//              printf("client-%d: breaking\n", getpid());
            for (int xx = 0; xx < MAX_OUT; xx++) {
                int disable = msg_test_assert_disable();
                ferr = BMSG_BREAK_(msgid[xx],
                                   results.u.s,
                                   TPT_REF(phandle));
                msg_test_assert_enable(disable);
                switch (ferr) {
                case XZFIL_ERR_OK:
                    break;
                case XZFIL_ERR_NOLCB:
                    usleep(100);
                    break;
                default:
                    TEST_CHK_FEOK(ferr);
                    break;
                }
            }
        }
    } else {
        for (inx = 0; ; inx++) {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = BMSG_LISTEN_((short *) &sre,     // sre
                                    0,                  // listenopts
                                    0);                 // listenertag
            } while (lerr == BSRETYPE_NOWORK);
            if (sre.sre_flags & BSRE_MON)
                len = 0;
            else
                len = rnd(MAX_SEND);
            BMSG_REPLY_(sre.sre_msgId,           // msgid
                        recv_buffer2[0],         // replyctrl
                        sre.sre_reqCtrlSize,     // replyctrlsize
                        recv_buffer[0],          // replydata
                        len,                     // replydatasize
                        0,                       // errorclass
                        NULL);                   // newphandle
        }
    }

    if (client) {
    } else if (client2) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = msg_mon_process_close();
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
