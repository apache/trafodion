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

#include "seabed/fs.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"
#include "ufsri.h"

enum { MAX_BUF = 0x40000 };

char my_name[BUFSIZ];
char recv_buffer[MAX_BUF];
char send_buffer[MAX_BUF];


int main(int argc, char *argv[]) {
    _bcc_status  bcc;
    bool         client = false;
    int          count_read;
    int          count_written;
    int          ferr;
    short        filenum;
    int          inx;
    int          loop = 10;
    RI_Type      ri;
    TAD          zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, (int) sizeof(my_name));

    if (client) {
        ferr = BFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            bcc = BWRITEREADX(filenum,
                              send_buffer,
                              MAX_BUF,
                              MAX_BUF,
                              &count_read,
                              0);
            TEST_CHK_BCCEQ(bcc);
            assert(count_read == MAX_BUF);
            printf("%s\n", send_buffer);
        }
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = BFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            bcc = BREADUPDATEX(filenum,
                               recv_buffer,
                               MAX_BUF,
                               &count_read,
                               0);
            TEST_CHK_BCCEQ(bcc);
            getri(&ri);
#ifdef USE_SB_NEW_RI
            assert(ri.max_reply_count == MAX_BUF);
#endif
            getri(&ri);
#ifdef USE_SB_NEW_RI
            assert(ri.max_reply_count == MAX_BUF);
#endif
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (int) strlen(recv_buffer) + 1; // cast
            bcc = BREPLYX(recv_buffer,
                          MAX_BUF,
                          &count_written,
                          0,
                          XZFIL_ERR_OK);
            TEST_CHK_BCCEQ(bcc);
            assert(count_written == MAX_BUF);
        }
        ferr = BFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
