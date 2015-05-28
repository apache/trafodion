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

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"


int main(int argc, char *argv[]) {
    _xcc_status     cc;
    bool            chook = false;
    bool            client = false;
    unsigned short  count_read;
    unsigned short  count_written;
    int             ferr;
    short           filenum;
    int             inx;
    short           lasterr;
    int             loop = 10;
    char            my_name[BUFSIZ];
    char            recv_buffer[BUFSIZ];
    char            send_buffer[BUFSIZ];
    bool            shook = false;
    File_AS_Type    state;
    short           tfilenum;
    TAD             zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-shook",     TA_Bool, TA_NOMAX,    &shook     },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (client && chook)
        test_debug_hook("c", "c");
    if (!client && shook)
        test_debug_hook("s", "s");

    if (client) {
        // before init
        // test cc
        ferr = file_test_init(&argc, &argv, false);
        TEST_CHK_FEOK(ferr);
        file_test_assert_disable(&state);
        cc = BACTIVATERECEIVETRANSID(0);
        assert(_xstatus_lt(cc));
        ferr = BFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
        assert(lasterr == XZFIL_ERR_NOTOPEN);
        tfilenum = 0;
        cc = BAWAITIOX(&tfilenum,   // filenum
                       NULL,        // buf
                       NULL,        // xfercount
                       NULL,        // tag
                       0,           // timeout
                       NULL);       // segid
        assert(_xstatus_lt(cc));
        ferr = BFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
        assert(lasterr == XZFIL_ERR_NOTOPEN);
        filenum = tfilenum;
        cc = BREADUPDATEX(filenum,
                          NULL,     // buffer
                          0,        // read_count
                          0,        // count_read
                          0);       // tag
        assert(_xstatus_lt(cc));
        ferr = BFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
        assert(lasterr == XZFIL_ERR_NOTOPEN);
        cc = BREPLYX(NULL,  // buffer
                     0,     // write_count
                     NULL,  // count_written
                     0,     // reply_num
                     0);    // err_ret
        assert(_xstatus_lt(cc));
        ferr = BFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
        assert(lasterr == XZFIL_ERR_NOTOPEN);
        cc = BSETMODE(0,     // filenum
                      0,     // modenum
                      0,     // parm1
                      0,     // parm2
                      NULL); // oldval
        assert(_xstatus_lt(cc));
        ferr = BFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
        assert(lasterr == XZFIL_ERR_NOTOPEN);
        cc = BWRITEREADX(0,           // filenum
                         NULL,        // buffer
                         0,           // write_count
                         0,           // read_count
                         NULL,        // count_read
                         0);          // tag
        assert(_xstatus_lt(cc));
        ferr = BFILE_GETINFO_(-1, &lasterr, NULL, 0, NULL, NULL, NULL);
        assert(lasterr == XZFIL_ERR_NOTOPEN);
        file_test_assert_enable(&state);
    }
    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    util_test_start(client);
    ferr = file_mon_process_startup(!client);  // system messages?
    TEST_CHK_FEOK(ferr);
    util_gethostname(my_name, sizeof(my_name));

    if (client) {
        ferr = XFILE_OPEN_((char *) "$srv", 4, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            cc = XWRITEREADX(filenum,
                             send_buffer,
                             (short) (strlen(send_buffer) + 1),
                             BUFSIZ,
                             &count_read,
                             0);
            TEST_CHK_CCEQ(cc);
            printf("%s\n", send_buffer);
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    } else {
        ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                           0, 0, 0,
                           1, 1, // no sys msg
                           0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            cc = XREADUPDATEX(filenum,
                              recv_buffer,
                              BUFSIZ,
                              &count_read,
                              0);
            TEST_CHK_CCEQ(cc);
            strcat(recv_buffer, "- reply from ");
            strcat(recv_buffer, my_name);
            count_read = (short) (strlen(recv_buffer) + 1);
            cc = XREPLYX(recv_buffer,
                         count_read,
                         &count_written,
                         0,
                         XZFIL_ERR_OK);
            TEST_CHK_CCEQ(cc);
        }
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEOK(ferr);
    }
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(client);
    return 0;
}
