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

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"
#include "seabed/thread.h"

#include "tchkfe.h"
#include "tchkos.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_SERVERS = 100 };

SB_Thread::CV   cv;
int             count_opens = 0;
int             count_starts = 0;
int             maxsp = 1;
char            my_name[BUFSIZ];
bool            nowait = false;
TPT_DECL2      (phandle, MAX_SERVERS);
char            pname[BUFSIZ];
char            prog[MS_MON_MAX_PROCESS_PATH];
char            retname[BUFSIZ];
char            recv_buffer[40000];
char            send_buffer[40000];
long            t_elapsed;
struct timeval  t_start;
struct timeval  t_stop;
bool            verbose = false;


void cb_open(MS_Mon_Open_Comp_Type *comp) {
    int inx;
    int status;

    if (verbose)
        printf("client (%s) received open complete message, ferr=%d, name=%s, oid=%d, tag=0x%llx\n",
               pname,
               comp->ferr,
               comp->name,
               comp->oid,
               comp->tag);
    assert(comp->ferr == XZFIL_ERR_OK);
    assert(memcmp(comp->name, "$srv", 4) == 0);
    inx = atoi(&comp->name[4]);
    assert(comp->tag == inx);
    memcpy(TPT_REF2(phandle, inx), &comp->phandle, sizeof(comp->phandle));
    count_opens++;
    if (count_opens >= maxsp) {
        status = cv.signal(true);
        TEST_CHK_STATUSOK(status);
    }
}

void cb_start(SB_Phandle_Type *, struct MS_Mon_NewProcess_Notice_def *msg) {
    int inx;

    if (verbose)
        printf("client (%s) received new process message, ferr=%d, nid=%d, pid=%d, tag=0x%llx, pname=%s\n",
               pname,
               msg->ferr,
               msg->nid,
               msg->pid,
               msg->tag,
               msg->process_name);
    assert(msg->ferr == XZFIL_ERR_OK);
    assert(memcmp(msg->process_name, "$SRV", 4) == 0);
    inx = atoi(&msg->process_name[4]);
    assert(msg->tag == inx);
    count_starts++;
    if (count_starts >= maxsp)
        cv.signal(true);
}

void telapsed(const char *text, struct timeval *start, struct timeval *stop) {
    t_elapsed = stop->tv_sec * 1000000 + stop->tv_usec -
                start->tv_sec * 1000000 - start->tv_usec;
    printf("%s: elapsed=%ld\n", text, t_elapsed);
}

int main(int argc, char *argv[]) {
    bool            chook = false;
    char           *cli;
    int             done;
    int             ferr;
    int             linx;
    int             loop = 10;
    int             msgid;
    int             nid;
    int             oid;
    int             pid;
    MS_Result_Type  results;
    int             sinx;
    char            sname[100];
    int             status;
    TAD             zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxsp",     TA_Int,  MAX_SERVERS, &maxsp     },
      { "-nowait",    TA_Bool, TA_NOMAX,    &nowait    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    if (chook)
        msg_debug_hook("c", "c");
    util_test_start(true);
    ferr = msg_mon_process_startup(false);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(pname, sizeof(pname));
    TEST_CHK_FEOK(ferr);
    sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
    cli = strstr(prog, "cli");
    strcpy(cli, "srv");
    gettimeofday(&t_start, NULL);
    for (sinx = 0; sinx < maxsp; sinx++) {
        sprintf(sname, "$srv%d", sinx);
        nid = -1;
        if (nowait)
            ferr = msg_mon_start_process_nowait_cb(cb_start,                 // callback
                                                   prog,                     // prog
                                                   (char *) sname,           // name
                                                   retname,                  // ret-name
                                                   argc,                     // argc
                                                   argv,                     // argv
                                                   MS_ProcessType_Generic,   // type
                                                   0,                        // priority
                                                   false,                    // debug
                                                   false,                    // backup
                                                   sinx,                     // tag
                                                   &nid,                     // nid
                                                   &pid,                     // pid
                                                   NULL,                     // infile
                                                   NULL);                    // outfile
        else
            ferr = msg_mon_start_process(prog,                     // prog
                                         (char *) sname,           // name
                                         retname,                  // ret-name
                                         argc,                     // argc
                                         argv,                     // argv
                                         TPT_REF2(phandle, sinx),  // phandle
                                         0,                        // open
                                         &oid,                     // oid
                                         MS_ProcessType_Generic,   // type
                                         0,                        // priority
                                         false,                    // debug
                                         false,                    // backup
                                         &nid,                     // nid
                                         &pid,                     // pid
                                         NULL,                     // infile
                                         NULL);                    // outfile
        TEST_CHK_FEOK(ferr);
    }
    if (nowait) {
        status = cv.wait(true);
        TEST_CHK_STATUSOK(status);
    }
    gettimeofday(&t_stop, NULL);
    telapsed("start", &t_start, &t_stop);
    gettimeofday(&t_start, NULL);
    for (sinx = 0; sinx < maxsp; sinx++) {
        sprintf(sname, "$srv%d", sinx);
        if (nowait) {
            ferr = msg_mon_open_process_nowait_cb((char *) sname,
                                                  TPT_REF2(phandle, sinx),
                                                  cb_open,
                                                  sinx,   // tag
                                                  &done,
                                                  &oid);
            TEST_CHK_FEOK(ferr);
            assert(!done);
        } else {
            ferr = msg_mon_open_process((char *) sname,
                                        TPT_REF2(phandle, sinx),
                                        &oid);
            TEST_CHK_FEOK(ferr);
        }
    }
    if (nowait) {
        status = cv.wait(true);
        TEST_CHK_STATUSOK(status);
    }
    gettimeofday(&t_stop, NULL);
    telapsed("open", &t_start, &t_stop);
    util_gethostname(my_name, sizeof(my_name));

    for (sinx = 0; sinx < maxsp; sinx++) {
        for (linx = 0; linx < loop; linx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, linx);
            ferr = XMSG_LINK_(TPT_REF2(phandle, sinx),     // phandle
                              &msgid,                      // msgid
                              NULL,                        // reqctrl
                              0,                           // reqctrlsize
                              NULL,                        // replyctrl
                              0,                           // replyctrlmax
                              send_buffer,                 // reqdata
                              39000,                       // reqdatasize
                              recv_buffer,                 // replydata
                              40000,                       // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
            ferr = XMSG_BREAK_(msgid, (short *) &results, TPT_REF2(phandle, sinx));
            util_check("XMSG_BREAK_", ferr);
            if (verbose)
                printf("%s\n", recv_buffer);
        }
    }
    for (sinx = 0; sinx < maxsp; sinx++) {
        ferr = msg_mon_close_process(TPT_REF2(phandle, sinx));
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
