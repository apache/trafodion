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

enum        { MAX_SERVERS = 10 };
SB_Thread::CV cv;
char          pname[BUFSIZ];
char          prog[MS_MON_MAX_PROCESS_PATH];
char          recv_buffer[BUFSIZ];
short         recv_buffer3[BUFSIZ];
char          retname[BUFSIZ];
char          send_buffer[BUFSIZ];
int           server_cnt = 0;
long long     tag;
bool          verbose = false;

void cb(SB_Phandle_Type *, struct MS_Mon_NewProcess_Notice_def *msg) {
    printf("client (%s) received new process message, ferr=%d, nid=%d, pid=%d, tag=0x%llx, pname=%s, count=%d\n",
           pname,
           msg->ferr,
           msg->nid,
           msg->pid,
           msg->tag,
           msg->process_name,
           server_cnt + 1);
    assert(msg->ferr == XZFIL_ERR_OK);
    if (server_cnt == 0)
        assert(msg->tag == 0x123456789abcdefLL);
    cv.signal(true);
    server_cnt++;
}


int main(int argc, char *argv[]) {
    int             arg;
    bool            attach = false;
    char           *cli;
    bool            chook = false;
    int             err;
    int             ferr;
    int             inx;
    int             msgid;
    int             nid;
    int             oid;
    TPT_DECL       (phandle);
    int             pid;
    MS_Result_Type  results;
    int             send_len;
    char            sname[MAX_SERVERS][20];
    BMS_SRE         sre;
    int             status;
    TAD             zargs[] = {
      { "-attach",    TA_Bool, TA_NOMAX,    &attach      },
      { "-chook",     TA_Bool, TA_NOMAX,    &chook       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose     },
      { "",           TA_End,  TA_NOMAX,    NULL         }
    };

    arg_proc_args(zargs, false, argc, argv);
    for (arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "-attach") == 0) // start_process
            argv[arg] = (char *) "";
    }
    if (chook)
        test_debug_hook("c", "c");

    if (attach)
        ferr = msg_init_attach(&argc, &argv, false, (char *) "$cli");
    else
        ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(pname, sizeof(pname));
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);

    for (inx = 0; inx < MAX_SERVERS; inx++)
        sprintf(sname[inx], "$srv%d", inx);
    sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
    cli = strstr(prog, "cli");
    strcpy(cli, "srv");
    nid = 0;
    if (verbose)
        printf("client starting nowait srv0\n");
    ferr = msg_mon_start_process_nowait_cb(cb,                       // callback
                                           prog,                     // prog
                                           (char *) sname[0],        // name
                                           retname,                  // ret-name
                                           argc,                     // argc
                                           argv,                     // argv
                                           MS_ProcessType_Generic,   // type
                                           0,                        // priority
                                           false,                    // debug
                                           false,                    // backup
                                           0x123456789abcdefLL,      // tag
                                           &nid,                     // nid
                                           &pid,                     // pid
                                           NULL,                     // infile
                                           NULL);                    // outfile
    TEST_CHK_FEOK(ferr);
    assert(strcasecmp(retname, sname[0]) == 0);
    assert(nid == 0);
#if 0 // cannot test reliably - completion of above may be too quick
    ferr = msg_mon_start_process_nowait_cb(cb,                       // callback
                                           prog,                     // prog
                                           (char *) sname[0],        // name
                                           retname,                  // ret-name
                                           argc,                     // argc
                                           argv,                     // argv
                                           MS_ProcessType_Generic,   // type
                                           0,                        // priority
                                           false,                    // debug
                                           false,                    // backup
                                           0x123456789abcdefLL,      // tag
                                           &nid,                     // nid
                                           &pid,                     // pid
                                           NULL,                     // infile
                                           NULL);                    // outfile
    assert(ferr == XZFIL_ERR_BADPARMVALUE);
#endif
    status = cv.wait(true);
    TEST_CHK_STATUSOK(status);
    for (inx = 1; inx < MAX_SERVERS; inx++) {
        nid = -1;
        if (verbose)
            printf("client starting nowait srv%d\n", inx);
        ferr = msg_mon_start_process_nowait_cb(cb,                       // callback
                                               prog,                     // prog
                                               (char *) sname[inx],      // name
                                               retname,                  // ret-name
                                               argc,                     // argc
                                               argv,                     // argv
                                               MS_ProcessType_Generic,   // type
                                               0,                        // priority
                                               false,                    // debug
                                               false,                    // backup
                                               inx,                      // tag
                                               &nid,                     // nid
                                               &pid,                     // pid
                                               NULL,                     // infile
                                               NULL);                    // outfile
        TEST_CHK_FEOK(ferr);
        assert(strcasecmp(retname, sname[inx]) == 0);
    }
    do {
        status = cv.wait(true, 0, 10000);
        TEST_CHK_STATUSOKORTIMEOUT(status);
    } while (server_cnt < MAX_SERVERS);

    for (inx = 0; inx < MAX_SERVERS; inx++) {
        ferr = msg_mon_open_process((char *) sname[inx],  // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
        strcpy(send_buffer, "hello, greetings from client");
        send_len = (int) strlen(send_buffer) + 1;
        if (verbose)
            printf("client link inx=%d\n", inx);
        ferr = BMSG_LINK_(TPT_REF(phandle),            // phandle
                          &msgid,                      // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          recv_buffer3,                // replyctrl
                          1,                           // replyctrlmax
                          send_buffer,                 // reqdata
                          (ushort) send_len,           // reqdatasize
                          recv_buffer,                 // replydata
                          BUFSIZ,                      // replydatamax
                          0,                           // linkertag
                          0,                           // pri
                          0,                           // xmitclass
                          0);                          // linkopts
        TEST_CHK_FEOK(ferr);
        ferr = BMSG_BREAK_(msgid, (short *) &results, TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
        printf("%s\n", recv_buffer);

        if (verbose)
            printf("client closing inx=%d\n", inx);
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    for (inx = 0; inx < MAX_SERVERS; inx++) {
        do {
            err = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(err);
            err = BMSG_LISTEN_((short *) &sre, // sre
                               0,              // listenopts
                               0);             // listenertag
        } while (err == BSRETYPE_NOWORK);
        assert(sre.sre_flags & BSRE_MON);
        ferr = BMSG_READDATA_(sre.sre_msgId,  // msgid
                              recv_buffer,    // reqdata
                              BUFSIZ);        // bytecount
        TEST_CHK_FEOK(ferr);
        MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
        assert(msg->type == MS_MsgType_ProcessDeath);
        if (verbose)
            printf("client rcvd death cnt=%d, name=%s\n",
                   inx + 1, msg->u.death.process_name);
        BMSG_REPLY_(sre.sre_msgId,       // msgid
                    NULL,                // replyctrl
                    0,                   // replyctrlsize
                    NULL,                // replydata
                    0,                   // replydatasize
                    0,                   // errorclass
                    NULL);               // newphandle
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    return 0;
}
