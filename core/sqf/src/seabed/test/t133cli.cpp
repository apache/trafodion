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

#include "tchkfe.h"
#include "tutil.h"

char  pname[BUFSIZ];
char  prog[MS_MON_MAX_PROCESS_PATH];
char  recv_buffer[BUFSIZ];
short recv_buffer3[BUFSIZ];
char  retname[BUFSIZ];
char  send_buffer[BUFSIZ];

int main(int argc, char *argv[]) {
    char           *cli;
    int             ferr;
    int             lerr;
    int             msgid;
    int             nid;
    int             oid;
    TPT_DECL       (phandle);
    int             pid;
    MS_Result_Type  results;
    int             send_len;
    const char     *sname = "$srv";
    MS_SRE          sre;

    ferr = msg_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_startup(true);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(pname, sizeof(pname));
    TEST_CHK_FEOK(ferr);
    msg_mon_enable_mon_messages(true);

    sprintf(prog, "%s/%s", getenv("PWD"), argv[0]);
    cli = strstr(prog, "cli");
    strcpy(cli, "srv");
    nid = 0;
    ferr = msg_mon_start_process_nowait(prog,                     // prog
                                        (char *) sname,           // name
                                        retname,                  // ret-name
                                        argc,                     // argc
                                        argv,                     // argv
                                        TPT_REF(phandle),         // phandle
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
    assert(strcasecmp(retname, sname) == 0);
    assert(nid == 0);
    lerr = XWAIT(LREQ, -1);
    TEST_CHK_WAITIGNORE(lerr);
    for (;;) {
        lerr = XMSG_LISTEN_((short *) &sre, // sre
                            0,              // listenopts
                            0);             // listenertag
        if (lerr == XSRETYPE_NOWORK)
            continue;
        ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                              recv_buffer,    // reqdata
                              BUFSIZ);        // bytecount
        TEST_CHK_FEOK(ferr);
        if (sre.sre_flags & XSRE_MON) {
            MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
            printf("client (%s) received mon message, type=%d\n",
                   pname, msg->type);
            assert(msg->type == MS_MsgType_ProcessCreated);
            printf("client (%s) received new process message, ferr=%d, nid=%d, pid=%d, tag=0x%llx, pname=%s\n",
                   pname,
                   msg->u.process_created.ferr,
                   msg->u.process_created.nid,
                   msg->u.process_created.pid,
                   msg->u.process_created.tag,
                   msg->u.process_created.process_name);
            assert(msg->u.process_created.ferr == XZFIL_ERR_OK);
            assert(msg->u.process_created.tag == 0x123456789abcdefLL);
            ferr = msg_mon_open_process((char *) sname,       // name
                                        TPT_REF(phandle),
                                        &oid);
            TEST_CHK_FEOK(ferr);
        }
        XMSG_REPLY_(sre.sre_msgId,       // msgid
                    NULL,                // replyctrl
                    0,                   // replyctrlsize
                    NULL,                // replydata
                    0,                   // replydatasize
                    0,                   // errorclass
                    NULL);               // newphandle
        if (sre.sre_flags & XSRE_MON)
            break;
    }

    strcpy(send_buffer, "hello, greetings from client");
    send_len = (int) strlen(send_buffer) + 1;
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

    ferr = msg_mon_close_process(TPT_REF(phandle));
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    return 0;
}
