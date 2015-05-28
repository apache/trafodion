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

#include <sys/ipc.h>
#include <sys/shm.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

char my_name[BUFSIZ];
char recv_buffer[40000];


int main(int argc, char *argv[]) {
    int             arg;
    bool            bm = false;
    char           *buf;
    double          busy;
    bool            chook = false;
    bool            client = false;
    char            errnobuf[100];
    char            error_txt[200];
    int             event_len;
    char            event_data[MS_MON_MAX_SYNC_DATA];
    int             ferr;
    int             inx;
    int             key;
    int             lerr;
    int             loop = 10;
    int             msid;
    int             pid;
    char            prog[MS_MON_MAX_PROCESS_PATH];
    struct rusage   r_start;
    struct rusage   r_stop;
    const char     *server_name = "$SRV";
    int             server_nid;
    TPT_DECL       (server_phandle);
    int             server_pid;
    bool            shook = false;
    struct timeval  t_elapsed;
    struct timeval  t_start;
    struct timeval  t_stop;
    TAD             zargs[] = {
      { "-bm",        TA_Bool, TA_NOMAX,    &bm        },
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = msg_init(&argc, &argv);
    arg_proc_args(zargs, false, argc, argv);
    if (chook && client)
        test_debug_hook("c", "c");
    if (shook && !client)
        test_debug_hook("s", "s");
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    proc_enable_external_wakeups();  // allow external wakeups
    if (client) {
        ferr = msg_mon_get_process_info(NULL, &server_nid, &server_pid);
        TEST_CHK_FEOK(ferr);
        sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
        for (arg = 0; arg < argc; arg++)
            if (strcmp(argv[arg], "-client") == 0) // start_process
                argv[arg] = (char *) "-server";
        ferr = msg_mon_start_process(prog,                   // prog
                                     (char *) server_name,   // name
                                     NULL,                   // ret name
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
        printf("process started, err=%d\n", ferr);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));
    lerr = XWAIT(LREQ, -1); // remove first LREQ
    assert(lerr == LREQ);
    ferr = msg_mon_get_my_segid(&key);
    TEST_CHK_FEOK(ferr);
    // process-wait for client/server/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 3, NULL, false);
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_event_wait(1, &event_len, event_data);
        TEST_CHK_FEOK(ferr);
        msid = shmget(key, sizeof(recv_buffer), 0640);
        if (msid == -1) {
            perror("client shmget");
            sprintf(error_txt, "client shmget(%d)=%s\n",
                    key,
                    strerror_r(errno, errnobuf, sizeof(errnobuf)));
        }
        assert(msid != -1);
        buf = (char *) shmat(msid, NULL, 0);
        assert(buf != NULL);
    } else {
        msid = shmget(key, sizeof(recv_buffer), IPC_CREAT | 0640);
        if (msid == -1) {
            perror("server shmget");
            sprintf(error_txt, "server shmget(%d)=%s\n",
                    key,
                    strerror_r(errno, errnobuf, sizeof(errnobuf)));
        }
        assert(msid != -1);
        buf = (char *) shmat(msid, NULL, 0);
        assert(buf != NULL);
        ferr =
          msg_mon_event_send(-1,                         // nid
                             -1,                         // pid
                             MS_ProcessType_Undefined,   // process-type
                             1,                          // event-id
                             0,                          // event-len
                             NULL);                      // event-data
        TEST_CHK_FEOK(ferr);
        ferr = msg_mon_event_wait(1, &event_len, event_data);
        TEST_CHK_FEOK(ferr);
    }

    util_time_timer_start(&t_start);
    util_cpu_timer_start(&r_start);
    if (client) {
        pid = server_pid;
        for (inx = 0; inx < loop; inx++) {
            lerr = XPROCESS_AWAKE_(pid, LREQ);
            assert(lerr == XZFIL_ERR_OK);
            lerr = XWAIT(LDONE, -1);
            assert(lerr == LDONE);
        }
    } else {
        ferr = msg_mon_get_process_info((char *) "$CLI", &server_nid, &pid);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            lerr = XWAIT(LREQ, -1);
            assert(lerr == LREQ);
            lerr = XPROCESS_AWAKE_(pid, LDONE);
            assert(lerr == XZFIL_ERR_OK);
        }
    }
    util_cpu_timer_stop(&r_stop);
    util_time_timer_stop(&t_stop);
    util_time_elapsed(&t_start, &t_stop, &t_elapsed);
    util_cpu_timer_busy(&r_start, &r_stop, &t_elapsed, &busy);

    if (client) {
        if (!bm)
            print_elapsed("", &t_elapsed);
        print_rate(bm, "", loop, 1024, &t_elapsed, busy);
    } else
        print_server_busy(bm, "", busy);

    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
