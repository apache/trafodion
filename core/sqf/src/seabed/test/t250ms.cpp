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

//#define DEBUG_ENV
#ifdef DEBUG_ENV
char        **env;
#endif

MS_Mon_Process_Info_Type info[1];
char                     my_mon_name[BUFSIZ];
char                     my_mon_name3[BUFSIZ];
char                     my_name[BUFSIZ];
char                     recv_buffer[BUFSIZ];
short                    recv_buffer2[BUFSIZ];
short                    recv_buffer3[BUFSIZ];
char                     send_buffer[BUFSIZ];
short                    send_buffer2[BUFSIZ];

int main(int argc, char *argv[]) {
    char                      *args[4];
    bool                       attach = false;
    bool                       chook = false;
    bool                       client = false;
    bool                       clientd = false;
    int                        cli_nid = -1;
    int                        cli_pid = -1;
    const char                *cname = "$cli";
    int                        count;
    int                        ferr;
    int                        inx;
    int                        len;
    int                        lerr;
    int                        loop = 10;
    TPT_DECL                  (mphandle);
    char                      *mphandlec = (char *) &mphandle;
    TPT_DECL_INT              (mphandlei);
    int                        msgid;
    int                        my_mon_nid;
    int                        my_mon_pid;
    int                        my_mon_ptype;
    int                        my_mon_zid;
    int                        my_os_pid;
    long                       my_os_tid;
    int                        my_compid;
    int                        my_pnid;
    int                        my_segid;
    int                        nid;
    int                        oid;
    TPT_DECL                  (phandle);
    int                        pid;
    char                      *pname = NULL;
    int                        ptype;
    RT                         results;
    int                        scollpid = -1;
    int                        scompid = -1;
    int                        send_len;
    bool                       shook = false;
    const char                *sname = "$srv";
    bool                       sonar = false;
    MS_SRE                     sre;
    int                        srv_nid = -1;
    int                        srv_pid = -1;
    MS_Mon_Monitor_Stats_Type  stats;
    int                        status;
    long                       t_elapsed;
    struct timeval             t_start;
    struct timeval             t_stop;
    int                        tm_seq;
    bool                       val_bool;
    int                        val_int;
    const char                *val_str;
    int                        winx;
    pid_t                      wpid;
    TAD                        zargs[] = {
      { "-attach",    TA_Bool, TA_NOMAX,    &attach    },
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-clientd",   TA_Bool, TA_NOMAX,    &clientd   },
      { "-cname",     TA_Str,  TA_NOMAX,    &cname     },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-name",      TA_Str,  TA_NOMAX,    &pname     },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "-sname",     TA_Str,  TA_NOMAX,    &sname     },
      { "-sonar",     TA_Bool, TA_NOMAX,    &sonar     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

#ifdef DEBUG_ENV
    env = environ;
    while (*env != NULL) {
        printf("env=%s\n", *env);
        env++;
    }
#endif

    arg_proc_args(zargs, false, argc, argv);
    if (client && chook)
        test_debug_hook("c", "c");
    if (!client && shook)
        test_debug_hook("s", "s");

    setenv("TESTKEYBOOL", "1", 1);
    setenv("TESTKEYINT", "44", 1);
    setenv("TESTKEYSTR", "TESTVALSTR", 1);
    //
    // test msg_getenv* before msg_init
    //
    val_bool = true;
    msg_getenv_bool("TESTKEYBOOL", &val_bool);
    assert(val_bool);
    val_bool = false;
    msg_getenv_bool("TESTKEYBOOL", &val_bool);
    assert(!val_bool);
    val_int = 0;
    msg_getenv_int("TESTKEYINT", &val_int);
    assert(val_int == 0);
    val_int = 1;
    msg_getenv_int("TESTKEYINT", &val_int);
    assert(val_int == 1);
    val_str = (const char *) 1;
    val_str = msg_getenv_str("TESTKEYSTR");
    assert(val_str == NULL);

    if (attach)
        msfs_util_init_attach(&argc, &argv, msg_debug_hook, false, pname);
    else
        msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_time_timer_start(&t_start);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);

    //
    // test msg_getenv* after msg_init
    //
    msg_getenv_bool("TESTKEYBOOL", &val_bool);
    assert(val_bool);
    msg_getenv_int("TESTKEYINT", &val_int);
    assert(val_int == 44);
    val_str = msg_getenv_str("TESTKEYSTR");
    assert(strcmp(val_str, "TESTVALSTR") == 0);

    // process-wait for server/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 2, NULL, false);
    TEST_CHK_FEOK(ferr);
    // process-wait for client
    ferr = msfs_util_wait_process_count(MS_ProcessType_TSE, 1, NULL, false);
    TEST_CHK_FEOK(ferr);
    if (client) {
        if (sonar) {
            scollpid = fork();
            if (scollpid == 0) {
                // child
                args[0] = (char *) "sonarcollector";
                args[1] = (char *) "-i"; // sampling interval // cast
                args[2] = (char *) "1"; // cast
                args[3] = NULL;
                lerr = execvp(args[0], args);
                assert(lerr == 0);
            }
        }
        ferr = msg_mon_open_process((char *) sname,       // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }

    // this phandle check assumed a particular data structure
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);
    ferr = XPROCESSHANDLE_GETMINE_(TPT_REF(mphandle));
    util_check("XPROCESSHANDLE_GETMINE_", ferr);
    TPT_COPY_INT(mphandlei, mphandle);
    assert((mphandlei[0] & 0xf0) == 0x20); // named
    assert(strncmp(my_name, &mphandlec[4], 32) == 0);
    printf("phandle=%x.%x.%x.%x.%x\n",
           mphandlei[0],
           mphandlei[1],
           mphandlei[2],
           mphandlei[3],
           mphandlei[4]);
    ferr = msg_mon_get_my_info(&my_mon_nid,
                               &my_mon_pid,
                               my_mon_name,
                               BUFSIZ,
                               &my_mon_ptype,
                               &my_mon_zid,
                               &my_os_pid,
                               &my_os_tid);
    util_check("msg_mon_get_my_info", ferr);
    printf("nid=%d, pid=%d, name=%s, ptype=%d, zid=%d, os-pid=%d, os-tid=%ld\n",
           my_mon_nid,
           my_mon_pid,
           my_mon_name,
           my_mon_ptype,
           my_mon_zid,
           my_os_pid,
           my_os_tid);
    my_mon_nid   = -1;
    my_mon_pid   = -1;
    my_mon_ptype = -1;
    my_mon_zid   = -1;
    my_os_pid    = -1;
    my_os_tid    = -1;
    my_compid    = -1;
    my_pnid      = -1;
    ferr = msg_mon_get_my_info3(&my_mon_nid,
                                &my_mon_pid,
                                my_mon_name3,
                                BUFSIZ,
                                &my_mon_ptype,
                                &my_mon_zid,
                                &my_os_pid,
                                &my_os_tid,
                                &my_compid,
                                &my_pnid);
    util_check("msg_mon_get_my_info3", ferr);
    printf("nid=%d, pid=%d, name=%s, ptype=%d, zid=%d, os-pid=%d, os-tid=%ld, compid=%d, pnid=%d\n",
           my_mon_nid,
           my_mon_pid,
           my_mon_name3,
           my_mon_ptype,
           my_mon_zid,
           my_os_pid,
           my_os_tid,
           my_compid,
           my_pnid);
    ferr = msg_mon_get_my_segid(&my_segid);
    util_check("msg_mon_get_my_segid", ferr);
    printf("segid=%d\n", my_segid);

    ferr = msg_mon_get_monitor_stats(&stats);
    util_check("msg_mon_get_monitor_stats", ferr);
    printf("avail_min=%d, acquired_max=%d, buf_misses=%d\n",
           stats.avail_min, stats.acquired_max, stats.buf_misses);

    util_gethostname(my_name, sizeof(my_name));
    msg_mon_enable_mon_messages(true);
    ferr = msg_mon_get_tm_seq(&tm_seq);
    util_check("msg_mon_get_tm_seq", ferr);
    printf("tm_seq=%d\n", tm_seq);
    if (client) {
        ferr = msg_mon_get_process_info_type(MS_ProcessType_TSE,
                                             &count,
                                             0,
                                             NULL);
        TEST_CHK_FEOK(ferr);
        if (!attach)
            assert(count == 1);
        ferr = msg_mon_get_process_info_type(MS_ProcessType_TSE,
                                             &count,
                                             1,
                                             info);
        TEST_CHK_FEOK(ferr);
        if (!attach)
            assert(count == 1);
    }

    for (inx = 0; inx < loop; inx++) {
        if (client) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            send_len = (int) strlen(send_buffer) + 1;
            ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              send_buffer2,                // reqctrl
                              (ushort) (inx & 1),          // reqctrlsize
                              recv_buffer3,                // replyctrl
                              1,                           // replyctrlmax
                              send_buffer,                 // reqdata
                              (ushort) send_len,           // reqdatasize
                              recv_buffer,                 // replydata
                              BUFSIZ,                      // replydatamax
                              0,                           // linkertag
                              (short) (inx+1),             // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid,
                               results.u.s,
                               TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == (uint) (inx & 1));
            assert(results.u.t.data_size > (strlen(send_buffer) + 14));
            assert(results.u.t.errm == RT_DATA_RCVD); // data
            printf("%s\n", recv_buffer);
        } else {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            ferr = XMSG_GETREQINFO_(MSGINFO_NID,
                                    sre.sre_msgId,
                                    &nid);
            util_check("XMSG_GETREQINFO_", ferr);
            ferr = XMSG_GETREQINFO_(MSGINFO_PID,
                                    sre.sre_msgId,
                                    &pid);
            util_check("XMSG_GETREQINFO_", ferr);
            ferr = XMSG_GETREQINFO_(MSGINFO_PTYPE,
                                    sre.sre_msgId,
                                    &ptype);
            util_check("XMSG_GETREQINFO_", ferr);
            if (sre.sre_flags & XSRE_MON) {
                // mon messages have our nid/pid
                if (srv_nid < 0) {
                    ferr = msg_mon_get_process_info((char *) "",
                                                    &srv_nid,
                                                    &srv_pid);
                    TEST_CHK_FEOK(ferr);
                }
                if (!clientd) {
                    assert(nid == srv_nid);
                    assert(pid == srv_pid);
                }
            } else {
                assert(sre.sre_pri == ((inx+1) & 0xffff));
                if (cli_nid < 0) {
                    ferr = msg_mon_get_process_info((char *) cname,
                                                    &cli_nid,
                                                    &cli_pid);
                    TEST_CHK_FEOK(ferr);
                }
                if (!attach)
                    assert(ptype == MS_ProcessType_TSE);
                assert(nid == cli_nid);
                assert(pid == cli_pid);
            }
            ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                                  recv_buffer2,   // reqctrl
                                  1);             // bytecount
            util_check("XMSG_READCTRL_", ferr);
            ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  BUFSIZ);        // bytecount
            util_check("XMSG_READDATA_", ferr);
            if (sre.sre_flags & XSRE_MON) {
                printf("server received mon message\n");
                inx--;
                len = 0;
                if (clientd) {
                    MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
                    if (msg->type == MS_MsgType_Close)
                        inx = loop;
                }
            } else {
                strcat(recv_buffer, "- reply from ");
                strcat(recv_buffer, my_name);
                len = (int) strlen(recv_buffer) + 1;
            }
            XMSG_REPLY_(sre.sre_msgId,       // msgid
                        recv_buffer2,        // replyctrl
                        sre.sre_reqCtrlSize, // replyctrlsize
                        recv_buffer,         // replydata
                        (ushort) len,        // replydatasize
                        0,                   // errorclass
                        NULL);               // newphandle
        }
    }
    if (sonar) {
        if (client) {
            // for some reason [monitor?], one 'sonarcom stop' doesn't stop coll
            for (winx = 0; winx < 20; winx++) {
                scompid = fork();
                if (scompid == 0) {
                    // child
                    args[0] = (char *) "sonarcom";
                    args[1] = (char *) "stop";
                    args[2] = NULL;
                    lerr = execvp(args[0], args);
                    assert(lerr == 0);
                }
                wpid = waitpid(scompid, &status, 0);
                assert(wpid == scompid);
                wpid = waitpid(scollpid, &status, WNOHANG);
                if (wpid == scollpid)
                    break;
                else
                    sleep(1);
            }
            assert(wpid == scollpid);
            ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
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
            TEST_CHK_FEOK(ferr);
            ferr = XMSG_BREAK_(msgid,
                               results.u.s,
                               TPT_REF(phandle));
            TEST_CHK_FEOK(ferr);
        } else {
            do {
                lerr = XWAIT(LREQ, -1);
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
        }
    }

    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    } else {
        if (!clientd) {
            ferr = msg_mon_process_close();
            TEST_CHK_FEOK(ferr);
        }
    }
    util_time_timer_stop(&t_stop);
    t_elapsed = (t_stop.tv_sec * 1000000 + t_stop.tv_usec) -
                (t_start.tv_sec * 1000000 + t_start.tv_usec);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    if (client) {
        printf("elapsed time (gettimeofday us)=%ld\n", t_elapsed);
    }
    return 0;
}
