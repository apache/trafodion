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
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"

char my_name[BUFSIZ];
char recv_buffer[BUFSIZ];
char send_buffer[BUFSIZ];


int main(int argc, char *argv[]) {
    _xcc_status     cc;
    bool            client = false;
    unsigned short  count_read;
    unsigned short  count_written;
    int             disable;
    int             ferr;
    short           filenum;
    int             inx;
    int             loop = 10;
    TPT_DECL       (mphandle);
    int             nid;
    int             oid;
    int             pid;
    const char     *sname = "$srv";
    TPT_DECL       (sphandle);
    TPT_PTR        (sphandle1);
    TPT_PTR        (sphandle2);
    TPT_DECL       (tphandle);
    TAD             zargs[] = {
      { "-client",    TA_Bool, TA_NOMAX,    &client    },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop      },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-sname",     TA_Str,  TA_NOMAX,    &sname     },
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
    ferr = msg_mon_get_process_info(my_name, &nid, &pid);
    TEST_CHK_FEOK(ferr);

    ferr = XPROCESSHANDLE_GETMINE_(TPT_REF(mphandle));
    util_check("XPROCESSHANDLE_GETMINE_", ferr);
    ferr = XPROCESSHANDLE_NULLIT_(TPT_REF(tphandle));
    util_check("XPROCESSHANDLE_NULLIT_", ferr);
    ferr = XFILENAME_TO_PROCESSHANDLE_(my_name,
                                       (short) strlen(my_name),
                                       TPT_REF(tphandle));
    util_check("XFILENAME_TO_PROCESSHANDLE_", ferr);
    ferr = XPROCESSHANDLE_COMPARE_(TPT_REF(mphandle),
                                   TPT_REF(tphandle));
    assert(ferr == 2); // should be identical

    ferr = msg_set_phandle((char *) my_name, TPT_REF(tphandle));
    util_check("msg_set_phandle", ferr);
    sphandle1 = msg_get_phandle((char *) my_name);
    ferr = XPROCESSHANDLE_COMPARE_(sphandle1, TPT_REF(tphandle));
    assert(ferr == 2); // should be identical
    msg_get_phandle((char *) my_name, &ferr);
    assert(ferr == 0);
    disable = msg_test_assert_disable();
    msg_get_phandle((char *) "junk", &ferr);
    assert(ferr == XZFIL_ERR_NOSUCHDEV);
    msg_test_assert_enable(disable);

    sphandle1 = msg_get_phandle_no_open((char *) my_name);
    ferr = XPROCESSHANDLE_COMPARE_(sphandle1, TPT_REF(tphandle));
    assert(ferr == 2); // should be identical
    ferr = msg_set_phandle((char *) my_name, NULL);
    util_check("msg_set_phandle", ferr);
    sphandle1 = msg_get_phandle_no_open((char *) my_name);
    assert(sphandle1 == NULL);

    // process-wait for client/server/shell
    ferr = msfs_util_wait_process_count(MS_ProcessType_Generic, 3, NULL, false);
    TEST_CHK_FEOK(ferr);
    if (client) {
        ferr = msg_mon_open_process((char *) sname,
                                    TPT_REF(sphandle),
                                    &oid);
        TEST_CHK_FEOK(ferr);
        ferr = msg_set_phandle((char *) my_name, TPT_REF(sphandle));
        util_check("msg_set_phandle", ferr);
        sphandle2 = msg_get_phandle((char *) sname);
        ferr = XPROCESSHANDLE_COMPARE_(sphandle2, TPT_REF(sphandle));
        assert(ferr == 2); // should be identical

        ferr = XFILE_OPEN_((char *) sname, 4, &filenum,
                           0, 0, 0, 0, 0, 0, 0, NULL);
        TEST_CHK_FEOK(ferr);
        for (inx = 0; inx < loop; inx++) {
            sprintf(send_buffer, "hello, greetings from %s, inx=%d",
                    my_name, inx);
            cc = XWRITEREADX(filenum,
                             send_buffer,
                             (unsigned short) (strlen(send_buffer) + 1), // cast
                             BUFSIZ,
                             &count_read,
                             0);
            TEST_CHK_CCEQ(cc);
            printf("%s\n", send_buffer);
        }
        disable = msg_test_assert_disable();
        ferr = XFILE_CLOSE_(filenum, 0);
        TEST_CHK_FEIGNORE(ferr);
        msg_test_assert_enable(disable);
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
            count_read = (unsigned short) (strlen(recv_buffer) + 1); // cast
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
