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
#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum        { MAX_CLI = 100 };
enum        { MAX_OUT = 100 };
enum        { MAX_SRV = 100 };
char          my_name[BUFSIZ];
SB_Tag_Type   out_list[MAX_SRV][MAX_OUT];
char          recv_buffer[BUFSIZ];
unsigned int  seed;
char          send_buffer[MAX_OUT][BUFSIZ];

int rnd(int max) {
    int rndret = (int) (1.0 * max*rand_r(&seed)/(RAND_MAX+1.0));
    return rndret;
}


int main(int argc, char *argv[]) {
    _bcc_status         bcc;
    void               *buf;
    int                 cancel = 0;
    bool                client = false;
    int                 closes;
    int                 count;
    int                 count_cancel;
    int                 count_read;
    int                 count_written;
    int                 count_xferred;
    int                 ferr;
    int                 inxl;
    int                 inxo;
    int                 inxs;
    int                 inxx;
    int                 loop = 10;
    int                 maxcp = 1;
    int                 maxout = 1;
    int                 maxsp = 1;
    int                 msg_count;
    bool                mq = false;
    short               filenumr;
    short               filenums[MAX_SRV];
    int                 send_len;
    char                serv[20];
    xzsys_ddl_smsg_def *sys_msg = (xzsys_ddl_smsg_def *) recv_buffer;
    SB_Tag_Type         tago;
    short               tfilenum;
    int                 timeout = -1;
    bool                verbose = false;
    TAD                 zargs[] = {
      { "-cancel",    TA_Int,  TA_NOMAX,    &cancel    },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-maxcp",     TA_Int,  MAX_CLI,     &maxcp     },
      { "-maxout",    TA_Int,  MAX_OUT,     &maxout    },
      { "-maxsp",     TA_Int,  MAX_SRV,     &maxsp     },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq        },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("file_mon_get_my_process_name", ferr);

    if (client) {
        ferr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETSENDLIMIT,
                                     XMAX_SETTABLE_SENDLIMIT);
        assert(ferr == XZFIL_ERR_OK);
        for (inxs = 0; inxs < maxsp; inxs++) {
            sprintf(serv, "$srv%d", inxs);
            ferr = BFILE_OPEN_(serv, (short) strlen(serv), &filenums[inxs],
                               0, 0, (short) maxout,
                               0, 0, 0, 0, NULL);
            TEST_CHK_FEOK(ferr);
        }
        msg_count = 0;
        count_cancel = 0;
        for (inxl = 0; inxl < loop; inxl++) {
            count = 0;
            if (cancel)
                count_cancel = rnd(cancel);
            for (inxs = 0; inxs < maxsp; inxs++) {
                for (inxo = 0; inxo < maxout; inxo++) {
                    send_len = rnd(BUFSIZ);
                    bcc = BWRITEREADX(filenums[inxs],
                                      send_buffer[inxo],
                                      send_len,
                                      BUFSIZ,
                                      &count_read,
                                      inxo + 1);
                    out_list[inxs][inxo] = inxo + 1;
                    util_check("BWRITEREADX", ferr);
                }
            }
            for (inxs = 0; inxs < maxsp; inxs++) {
                for (inxo = 0; inxo < maxout; inxo++) {
                    msg_count++;
                    if (verbose)
                        printf("client(%s): msg-count=%d\n",
                               my_name, msg_count);
                    count++;
                    if (cancel && (count >= count_cancel)) {
                        if (verbose)
                            printf("client(%s): cancel, count=%d\n",
                                   my_name, count);
                        count_cancel = rnd(cancel);
                        count = 0;
                        tago = out_list[inxs][inxo];
                        if (tago == 0) {
                            for (inxx = maxout - 1; inxx > 0; inxx--) {
                                tago = out_list[inxs][inxx];
                                if (tago > 0)
                                    break;
                            }
                        }
                        if (verbose)
                            printf("client(%s): cancel, tag=" PFTAG "\n",
                                   my_name, tago);
                        ferr = BCANCELREQ(filenums[inxs], tago);
                        util_check("BCANCELREQ", ferr);
                    } else {
                        tfilenum = filenums[inxs];
                        bcc = BAWAITIOX(&tfilenum,
                                        &buf,
                                        &count_xferred,
                                        &tago,
                                        timeout,
                                        NULL);
                        TEST_CHK_BCCEQ(bcc);
                        assert(tago <= maxout);
                        out_list[inxs][tago-1] = 0;
                    }
                }
            }
            if (mq) {
                if ((inxl % 100) == 0)
                    printf("client(%s): count=%d\n", my_name, inxl);
            }
        }
        for (inxs = 0; inxs < maxsp; inxs++) {
            ferr = BFILE_CLOSE_(filenums[inxs]);
            TEST_CHK_FEOK(ferr);
        }
    } else {
        ferr = XCONTROLMESSAGESYSTEM(XCTLMSGSYS_SETRECVLIMIT,
                                     XMAX_SETTABLE_RECVLIMIT);
        assert(ferr == XZFIL_ERR_OK);
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        closes = 0;
        msg_count = 0;
        // process requests
        for (;;) {
            bcc = BREADUPDATEX(filenumr,
                               recv_buffer,
                               BUFSIZ,
                               &count_read,
                               0);
            if (_xstatus_ne(bcc)) {
                int mt = sys_msg->u_z_msg.z_msgnumber[0];
                if (mt == XZSYS_VAL_SMSG_CLOSE) {
                    closes++;
                    if (verbose)
                        printf("server(%s): closes=%d\n",
                               my_name, closes);
                } else {
                    const char *mtstr = msfs_util_get_sysmsg_str(mt);
                    if (verbose)
                        printf("server(%s): type=%s(%d)\n", my_name, mtstr, mt);
                }
            } else {
                msg_count++;
                if (verbose)
                    printf("server(%s): msg-count=%d\n",
                           my_name, msg_count);
            }
            bcc = BREPLYX(recv_buffer,
                          count_read,
                          &count_written,
                          0,
                          XZFIL_ERR_OK);
            TEST_CHK_BCCEQ(bcc);
            if (closes >= maxcp)
                break;
        }
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
