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

enum  { MAX_DBUF = 1024 * 1024 };        // 1 MB

char recv_buffer[MAX_DBUF];
char send_buffer[MAX_DBUF];


int main(int argc, char *argv[]) {
    bool           abort1 = false;
    bool           abort2 = false;
    void          *buf;
    _xcc_status    cc;
    bool           client = false;
    int            count_read;
    int            count_written;
    int            count_xferred;
    int            dsize = 1024;
    int            ferr;
    short          filenum;
    int            inx;
    int            loop = 10;
    short          mt;
    char           my_name[BUFSIZ];
    bool           verbose = false;
    SB_Tag_Type    tag;
    short          tfilenum;
    int            timeout = -1;
    TAD            zargs[] = {
      { "-abort1",    TA_Bool, TA_NOMAX,    &abort1    },
      { "-abort2",    TA_Bool, TA_NOMAX,    &abort2    },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
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
    util_check("msg_mon_get_my_process_name", ferr);

    if (client) {
        ferr = BFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, (short) 1, // nowait
                           0, 0, 0, 0, NULL);
        if (verbose)
            printf("client open ferr=%d\n", ferr);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            cc = BWRITEREADX(filenum,
                             send_buffer,
                             (unsigned short) (strlen(send_buffer) + 1), // cast
                             0,
                             &count_read,
                             0);
            if (verbose)
                printf("client wr cc=%d\n", cc);
            TEST_CHK_CCEQ(cc);
            tfilenum = -2;
            cc = BAWAITIOX(&tfilenum,
                           &buf,
                           &count_xferred,
                           &tag,
                           timeout,
                           NULL);
            if (verbose)
                printf("client awaitiox cc=%d, fn=%d\n", cc, tfilenum);
            if (abort1) {
                if (_xstatus_ne(cc)) {
                    sleep(1);
                    inx--;
                    continue;
                }
            } else {
                TEST_CHK_CCEQ(cc);
            }
            printf("%s\n", send_buffer);
        }
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, (short) 0,
                           1, 0, // sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        if (abort1) {
        } else if (abort2) {
            loop -= 2;
        } else {
            loop += 2;
        }
        for (inx = 0; inx < loop; inx++) {
            if (abort1 && (inx == 3))
                util_abort_core_free();
            cc = BREADUPDATEX(filenum,
                              recv_buffer,
                              (int) dsize, // cast
                              &count_read,
                              0);
            if (verbose) {
                memcpy(&mt, recv_buffer, sizeof(mt));
                printf("server cc=%d, mt=%d\n", cc, mt);
            }
            cc = BREPLYX(recv_buffer,
                         0,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
    }

    if (client) {
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }

    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
