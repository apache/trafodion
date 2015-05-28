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

#include "array.h"
#include "tchkfe.h"
#include "tms.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum       { MAX_DATA = 100 };
enum       { MAX_CTRL = MAX_DATA/2 };
enum       { MAX_OUT  = 100 };
short        recv_cli_c_buffer[MAX_OUT][MAX_CTRL];
char         recv_cli_d_buffer[MAX_OUT][MAX_DATA];
short        recv_srv_c_buffer[MAX_OUT][MAX_CTRL];
char         recv_srv_d_buffer[MAX_OUT][MAX_DATA];
short        send_cli_c_buffer[MAX_OUT][MAX_CTRL];
char         send_cli_d_buffer[MAX_OUT][MAX_DATA];
short        send_srv_c_buffer[MAX_OUT][MAX_CTRL];
char         send_srv_d_buffer[MAX_OUT][MAX_DATA];
int          msgids[MAX_OUT];
unsigned int seed;
MS_SRE       sres[MAX_OUT];


int main(int argc, char *argv[]) {
    SB_Array<int>  array(MAX_OUT);
    int            client = false;
    int            ferr;
    int            inx;
    int            lerr;
    int            oid;
    int            req_ctrl_size;
    int            req_data_size;
    int            rep_ctrl_size;
    int            rep_data_size;
    RT             results;
    TPT_DECL      (phandle);
    TAD            zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    msfs_util_init(&argc, &argv, msg_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = msg_mon_process_startup(!client);             // system messages
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) "$srv",              // name
                                    TPT_REF(phandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);

        for (inx = 0; inx < MAX_OUT; inx++) {
            req_ctrl_size = (inx + 1) % MAX_DATA;
            req_data_size = (inx + 2) % MAX_DATA;
            ferr = XMSG_LINK_(TPT_REF(phandle),         // phandle
                              &msgids[inx],             // msgid
                              send_cli_c_buffer[inx],   // reqctrl
                              (ushort) req_ctrl_size,   // reqctrlsize
                              recv_cli_c_buffer[inx],   // replyctrl
                              (ushort) MAX_DATA,        // replyctrlmax
                              send_cli_d_buffer[inx],   // reqdata
                              (ushort) req_data_size,   // reqdatasize
                              recv_cli_d_buffer[inx],   // replydata
                              (ushort) MAX_DATA,        // replydatamax
                              0,                        // linkertag
                              0,                        // pri
                              0,                        // xmitclass
                              0);                       // linkopts
            util_check("XMSG_LINK_", ferr);
        }
        for (inx = 0; inx < MAX_OUT; inx++) {
            array.add();
            array.set_val(inx, inx);
        }
        for (inx = 0; inx < MAX_OUT; inx++) {
            int rnd =
              (int) (1.0 * array.get_size() * rand_r(&seed) / (RAND_MAX + 1.0));
            int xinx = array.get_val(rnd);
            array.remove(rnd);
            rep_ctrl_size = (xinx + 3) % MAX_DATA;
            rep_data_size = (xinx + 4) % MAX_DATA;
            ferr = XMSG_BREAK_(msgids[xinx], results.u.s, TPT_REF(phandle));
            util_check("XMSG_BREAK_", ferr);
            assert(results.u.t.ctrl_size == (unsigned short) rep_ctrl_size);
            assert(results.u.t.data_size == (unsigned short) rep_data_size);
        }
        printf("if there were no asserts, all is well\n");
    } else {
        for (inx = 0; inx < MAX_OUT; inx++) {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            do {
                lerr = XMSG_LISTEN_((short *) &sres[inx], // sre
                                    0,                    // listenopts
                                    0);                   // listenertag
                if (lerr == XSRETYPE_NOWORK)
                    util_time_sleep_ms(1);
            } while (lerr == XSRETYPE_NOWORK);
        }
        for (inx = 0; inx < MAX_OUT; inx++) {
            array.add();
            array.set_val(inx, inx);
        }
        for (inx = 0; inx < MAX_OUT; inx++) {
            int rnd =
              (int) (1.0 * array.get_size() * rand_r(&seed) / (RAND_MAX + 1.0));
            int xinx = array.get_val(rnd);
            array.remove(rnd);
            req_ctrl_size = (xinx + 1) % MAX_DATA;
            req_data_size = (xinx + 2) % MAX_DATA;
            assert(req_ctrl_size == sres[xinx].sre_reqCtrlSize);
            assert(req_data_size == sres[xinx].sre_reqDataSize);
            ferr = XMSG_READCTRL_(sres[xinx].sre_msgId,        // msgid
                                  recv_srv_c_buffer[xinx],     // reqctrl
                                  sres[xinx].sre_reqCtrlSize); // bytecount
            util_check("XMSG_READCTRL_", ferr);
            ferr = XMSG_READDATA_(sres[xinx].sre_msgId,        // msgid
                                  recv_srv_d_buffer[xinx],     // reqdata
                                  sres[xinx].sre_reqDataSize); // bytecount
            util_check("XMSG_READDATA_", ferr);
        }
        for (inx = 0; inx < MAX_OUT; inx++) {
            array.add();
            array.set_val(inx, inx);
        }
        for (inx = 0; inx < MAX_OUT; inx++) {
            int rnd =
              (int) (1.0 * array.get_size() * rand_r(&seed) / (RAND_MAX + 1.0));
            int xinx = array.get_val(rnd);
            array.remove(rnd);
            rep_ctrl_size = (xinx + 3) % MAX_DATA;
            rep_data_size = (xinx + 4) % MAX_DATA;
            XMSG_REPLY_(sres[xinx].sre_msgId,    // msgid
                        recv_srv_c_buffer[xinx], // replyctrl
                        (ushort) rep_ctrl_size,  // replyctrlsize
                        recv_srv_d_buffer[xinx], // replydata
                        (ushort) rep_data_size,  // replydatasize
                        0,                       // errorclass
                        NULL);                   // newphandle
        }
    }

    if (client) {
        ferr = msg_mon_close_process(TPT_REF(phandle));
        TEST_CHK_FEOK(ferr);
    }
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
