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

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

typedef struct {
    ushort cQc;   // Client reQuest Control
    ushort cQd;   // Client reQuest Data
    ushort cPcM;  // Client rePly Control
    ushort cPdM;  // Client rePly Data
    uint   cEpC;  // Client Expected rePly Control
    uint   cEpD;  // Client Expected rePly Data
    ushort sQc;   // Server reQuest Control
    ushort sQd;   // Server reQuest Data
    ushort sPc;   // Server rePly Control
    ushort sPd;   // Server rePly Data
    int    sEqC;  // Server Expected reQuest Control
    int    sEqD;  // Server Expected reQuest Data
} Test_Results;

Test_Results res[] = {
//
//  should match
//  +-----------------------------------------------------+
//  |    +------------------------------------------------|----+
//  |    |                +---------------------+         |    |
//  |    |                |      +--------------|----+    |    |
//  v    v                v      v              v    v    v    v
// cQc, cQd, cPcM, cPdM, cEpC, cEpD, sQc, sQd, sPc, sPd, sEqC, sEqD
//
{   1 ,  2 ,   3 ,   4 ,  1  ,  2  ,  1 ,  2 ,  1 ,  2 ,  1  ,  2   },// mix
{   8 ,  7 ,   8 ,   7 ,  8  ,  7  ,  5 ,  6 ,  8 ,  7 ,  8  ,  7   },// mix
{   1 ,  0 ,   0 ,   0 ,  0  ,  0  ,  0 ,  0 ,  0 ,  0 ,  1  ,  0   },// cc
{   0 ,  2 ,   0 ,   0 ,  0  ,  0  ,  0 ,  0 ,  0 ,  0 ,  0  ,  2   },// cd
{   0 ,  0 ,   1 ,   0 ,  1  ,  0  ,  0 ,  0 ,  1 ,  0 ,  0  ,  0   },// sc
{   0 ,  0 ,   0 ,   2 ,  0  ,  2  ,  0 ,  0 ,  0 ,  2 ,  0  ,  0   },// sd
{   0 ,  0 ,   1 ,   0 ,  1  ,  0  ,  0 ,  0 ,  3 ,  0 ,  0  ,  0   },// scl
{   0 ,  0 ,   0 ,   2 ,  0  ,  2  ,  0 ,  0 ,  0 ,  4 ,  0  ,  0   },// sdl
{   0 ,  0 ,   0 ,   0 ,  0  ,  0  ,  0 ,  0 ,  3 ,  0 ,  0  ,  0   },// scl
{   0 ,  0 ,   0 ,   0 ,  0  ,  0  ,  0 ,  0 ,  0 ,  4 ,  0  ,  0   },// sdl
{   0 ,  0 ,   0 ,   0 ,  0  ,  0  ,  0 ,  0 ,  0 ,  0 ,  0  ,  0   }
};

short ctrl_buffer[100];
char  my_name[BUFSIZ];
char  recv_buffer[40000];
char  send_buffer[40000];


int main(int argc, char *argv[]) {
    bool       client = false;
    unsigned   expR3;
    int        ferr;
    int        inx;
    int        lc;
    int        lerr;
    int        msgid;
    int        oid;
    RT         results;
    TPT_DECL  (phandle);
    MS_SRE     sre;
    TAD        zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client); // system messages
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",      // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
    }
    util_gethostname(my_name, sizeof(my_name));

    lc = (int) (sizeof(res)/sizeof(Test_Results));
    for (inx = 0; inx < lc; inx++) {
        if (client) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            ferr = XMSG_LINK_(TPT_REF(phandle),            // phandle
                              &msgid,                      // msgid
                              ctrl_buffer,                 // reqctrl
                              res[inx].cQc,                // reqctrlsize
                              ctrl_buffer,                 // replyctrl
                              res[inx].cPcM,               // replyctrlmax
                              send_buffer,                 // reqdata
                              res[inx].cQd,                // reqdatasize
                              recv_buffer,                 // replydata
                              res[inx].cPdM,               // replydatamax
                              0,                           // linkertag
                              0,                           // pri
                              0,                           // xmitclass
                              0);                          // linkopts
            util_check("XMSG_LINK_", ferr);
//          ferr = XMSG_ISDONE_(msgid);
//          checkdone("XMSG_ISDONE_", ferr);
            results.u.t.ctrl_size = (uint) -1;
            results.u.t.data_size = (uint) -1;
            results.u.t.errm = (uint) -1;
            ferr = XMSG_BREAK_(msgid, results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            if (results.u.t.ctrl_size != res[inx].cEpC) {
                 printf("*** ERROR inx=%d, results.ctrl_size=%d, cEpC=%d\n",
                        inx, (int) results.u.t.ctrl_size, (int) res[inx].cEpC);
                 assert(results.u.t.ctrl_size == res[inx].cEpC);
            }
            if (results.u.t.data_size != (uint) res[inx].cEpD) {
                printf("*** ERROR inx=%d, results.data_size=%d, cEpD=%d\n",
                       inx, (int) results.u.t.data_size, (int) res[inx].cEpD);
                assert(results.u.t.data_size == res[inx].cEpD);
            }
            expR3 = 0x0; // no-data
            if (res[inx].cEpD > 0)
                expR3 |= 0x2; // data
            if (results.u.t.errm != expR3) {
                printf("*** ERROR inx=%d, results.errm=%x, expR3=%x\n",
                       inx, results.u.t.errm, expR3);
                assert(results.u.t.errm == expR3);
            }
        } else {
            do {
                lerr = XWAIT(LREQ, -1);
                TEST_CHK_WAITIGNORE(lerr);
                lerr = XMSG_LISTEN_((short *) &sre, // sre
                                    0,              // listenopts
                                    0);             // listenertag
            } while (lerr == XSRETYPE_NOWORK);
            if (sre.sre_reqCtrlSize != res[inx].sEqC) {
                printf("*** ERROR inx=%d, sre.sre_reqCtrlSize=%d, sEqC=%d\n",
                       inx, sre.sre_reqCtrlSize, res[inx].sEqC);
                assert(sre.sre_reqCtrlSize == res[inx].sEqC);
            }
            if (sre.sre_reqDataSize != res[inx].sEqD) {
                printf("*** ERROR inx=%d, sre.sre_reqDataSize=%d, sEqD=%d\n",
                       inx, sre.sre_reqDataSize, res[inx].sEqD);
                assert(sre.sre_reqDataSize == res[inx].sEqD);
            }
            ferr = XMSG_READCTRL_(sre.sre_msgId,  // msgid
                                  ctrl_buffer,    // reqctrl
                                  res[inx].sQc);  // bytecount
            util_check("XMSG_READCTRL_", ferr);
            ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                                  recv_buffer,    // reqdata
                                  res[inx].sQd);  // bytecount
            util_check("XMSG_READDATA_", ferr);
            XMSG_REPLY_(sre.sre_msgId,            // msgid
                        (short *) recv_buffer,    // replyctrl
                        res[inx].sPc,             // replyctrlsize
                        recv_buffer,              // replydata
                        res[inx].sPd,             // replydatasize
                        0,                        // errorclass
                        NULL);                    // newphandle
        }
    }
    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
        printf("if there were no asserts, all is well\n");
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
