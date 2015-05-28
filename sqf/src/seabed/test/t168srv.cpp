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


int main(int argc, char *argv[]) {
    _bcc_status   bcc;
    int           count_read;
    int           count_written;
    int           ferr;
    short         filenum;
    int           inx;
    int           loop = 10;
    bool          mq = false;
    char          my_name[BUFSIZ];
    char          recv_buffer[BUFSIZ];
    TAD           zargs[] = {
      { "-client",    TA_Ign,  TA_NOMAX,    NULL          },
      { "-loop",      TA_Int,  TA_NOMAX,    &loop         },
      { "-maxsp",     TA_Next, TA_NOMAX,    NULL          },
      { "-mq",        TA_Bool, TA_NOMAX,    &mq           },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL          },
      { "",           TA_End,  TA_NOMAX,    NULL          }
    };

    ferr = file_init(&argc, &argv);
    TEST_CHK_FEOK(ferr);
    msfs_util_init_fs(&argc, &argv, file_debug_hook);
    arg_proc_args(zargs, false, argc, argv);
    ferr = file_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(my_name, BUFSIZ);
    util_check("msg_mon_get_my_process_name", ferr);

    ferr = XFILE_OPEN_((char *) "$RECEIVE", 8, &filenum,
                       0, 0, 0,
                       1, 0, // sys msg
                       0, 0, NULL);
    TEST_CHK_FEOK(ferr);

    // open
    bcc = BREADUPDATEX(filenum,
                       recv_buffer,
                       BUFSIZ,
                       &count_read,
                       1);
    TEST_CHK_BCCNE(bcc);
    assert(recv_buffer[0] == XZSYS_VAL_SMSG_OPEN);
    bcc = BREPLYX(recv_buffer,
                  0,
                  &count_written,
                  0,
                  XZFIL_ERR_OK);
    TEST_CHK_BCCEQ(bcc);

    for (inx = 0; inx < loop; inx++) {
        bcc = BREADUPDATEX(filenum,
                           recv_buffer,
                           BUFSIZ,
                           &count_read,
                           1);
        TEST_CHK_BCCEQ(bcc);
        bcc = BREPLYX(recv_buffer,
                      0,
                      &count_written,
                      0,
                      XZFIL_ERR_OK);
        TEST_CHK_BCCEQ(bcc);
    }

    // close
    bcc = BREADUPDATEX(filenum,
                       recv_buffer,
                       BUFSIZ,
                       &count_read,
                       1);
    TEST_CHK_BCCNE(bcc);
    assert(recv_buffer[0] == XZSYS_VAL_SMSG_CLOSE);
    bcc = BREPLYX(recv_buffer,
                  0,
                  &count_written,
                  0,
                  XZFIL_ERR_OK);
    TEST_CHK_BCCEQ(bcc);

    ferr = XFILE_CLOSE_(filenum, 0);
    TEST_CHK_FEOK(ferr);
    ferr = file_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    return 0;
}
