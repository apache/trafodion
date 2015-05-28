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

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

enum { MAX_ARGS = 20 };
enum { MAX_CLI = 100 };
enum { MAX_WKR = 100 };
char   my_name[BUFSIZ];
char   recv_buffer[BUFSIZ];
char   send_buffer[BUFSIZ];


int main(int argc, char *argv[]) {
    _xcc_status         cc;
    bool                client = false;
    int                 closes;
    int                 count_read;
    int                 count_written;
    int                 ferr;
    short               filenumr;
    short               filenums;
    int                 inxl;
    int                 inxw;
    int                 len;
    int                 loop = 10;
    int                 loopwp = 10;
    int                 maxwp = 10;
    bool                mq = false;
    char               *sname = (char *) "$srv";
    xzsys_ddl_smsg_def *sys_msg = (xzsys_ddl_smsg_def *) recv_buffer;
    bool                verbose = false;
    int                 wargc;
    char               *wargv[MAX_ARGS];
    char                wname[MAX_WKR][20];
    int                 wnid;
    TPT_DECL2          (wphandle,MAX_WKR);
    int                 wpid;
    char                wprog[BUFSIZ];
    bool                worker = false;
    TAD                 zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-loopwp",    TA_Int,  TA_NOMAX,    &loopwp    },
      { "-maxwp",     TA_Int,  MAX_WKR,     &maxwp     },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq        },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-worker",    TA_Bool, TA_NOMAX,    &worker    },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(!worker);  // system messages?
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);

    if (client) {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);

        for (inxw = 0; inxw < maxwp; inxw++)
            sprintf(wname[inxw], "$wrk%d", inxw);
        for (inxw = 0; inxw < argc; inxw++) {
            if (strcmp(argv[inxw], "-client") == 0)
                wargv[inxw] = (char *) "-worker";
            else
                wargv[inxw] = argv[inxw];
        }
        wargv[argc] = NULL;
        wargc = argc;
        sprintf(wprog, "%s/%s", getenv("PWD"), argv[0]);
        for (inxl = 0; inxl < loopwp; inxl++) {
            for (inxw = 0; inxw < maxwp; inxw++) {
                wnid = -1;
                if (verbose)
                    printf("client starting process inxl=%d, inxw=%d\n",
                           inxl, inxw);
                ferr = msg_mon_start_process(wprog,                  // prog
                                             wname[inxw],            // name
                                             NULL,                   // ret_name
                                             wargc,                  // argc
                                             wargv,                  // argv
                                             TPT_REF2(wphandle,inxw),// phandle
                                             false,                  // open
                                             NULL,                   // oid
                                             MS_ProcessType_Generic, // type
                                             0,                      // priority
                                             false,                  // debug
                                             false,                  // backup
                                             &wnid,                  // nid
                                             &wpid,                  // pid
                                             NULL,                   // infile
                                             NULL);                  // outfile
            }
            sleep(2);
            for (inxw = 0; inxw < maxwp;) {
                cc = BREADUPDATEX(filenumr,
                                  recv_buffer,
                                  BUFSIZ,
                                  &count_read,
                                  0);
                TEST_CHK_CCNE(cc);
                int mt = sys_msg->u_z_msg.z_msgnumber[0];
                if (verbose) {
                    const char *mtstr = msfs_util_get_sysmsg_str(mt);
                    printf("client sys-msg inxl=%d, inxw=%d, type=%s(%d)\n",
                           inxl, inxw, mtstr, mt);
                }
                if (mt == XZSYS_VAL_SMSG_PROCDEATH)
                    inxw++;
                cc = BREPLYX(recv_buffer,
                             0,
                             &count_written,
                             0,
                             XZFIL_ERR_OK);
                TEST_CHK_CCEQ(cc);
            }
        }
    } else if (worker) {
        len = (short) strlen(sname);
        ferr = BFILE_OPEN_(sname, (short) len, &filenums,
                           0, 0, 0,
                           0, 0, 0, 0);
        TEST_CHK_FEOK(ferr);
        for (inxl = 0; inxl < loop; inxl++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inxl);
            cc = BWRITEREADX(filenums,
                             send_buffer,
                             (int) (strlen(send_buffer) + 1), // cast
                             BUFSIZ,
                             &count_read,
                             0);
            TEST_CHK_CCEQ(cc);
        }
        ferr = BFILE_CLOSE_(filenums, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        closes = 0;
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenumr,
                           0, 0, 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);

        // process requests
        for (inxl = 0;; inxl++) {
            cc = BREADUPDATEX(filenumr,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            if (_xstatus_ne(cc)) {
                int mt = sys_msg->u_z_msg.z_msgnumber[0];
                if (verbose) {
                    const char *mtstr = msfs_util_get_sysmsg_str(mt);
                    if (mt == XZSYS_VAL_SMSG_CLOSE)
                        printf("server sys-msg inx=%d, type=%s(%d), cc=%d\n",
                               inxl, mtstr, mt, closes);
                    else
                        printf("server sys-msg inx=%d, type=%s(%d)\n",
                               inxl, mtstr, mt);
                }
                if (mt == XZSYS_VAL_SMSG_CLOSE)
                    closes++;
                count_read = 0;
            } else {
                strcat(recv_buffer, "- reply from ");
                strcat(recv_buffer, my_name);
                count_read = (int) (strlen(recv_buffer) + 1); // cast
            }
            cc = BREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
            if (closes >= (maxwp * loopwp))
                break;
        }
        ferr = BFILE_CLOSE_(filenumr, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
