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
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"
#include "tutilp.h"


int main(int argc, char *argv[]) {
    bool                 chook = false;
    int                  ferr;
    const char          *group;
    MS_Mon_Reg_Get_Type  info;
    int                  inx;
#ifndef USE_SB_DISABLED_MON_MSG
    int                  lerr;
#endif
    const char          *key;
    bool                 match = false;
    char                 my_name[BUFSIZ];
#ifndef USE_SB_DISABLED_MON_MSG
    char                 recv_buffer[BUFSIZ];
    MS_SRE               sre;
#endif
    bool                 sysmsg = false;
    const char          *value;
    bool                 verbose = false;
    TAD                  zargs[] = {
      { "-chook",     TA_Bool, TA_NOMAX,    &chook     },
      { "-client",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-sysmsg",    TA_Bool, TA_NOMAX,    &sysmsg    },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    if (chook)
        test_debug_hook("c", "c");
    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(true);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
#ifndef USE_SB_DISABLED_MON_MSG
    if (sysmsg)
        msg_mon_enable_mon_messages(true);
#endif
    ferr = msg_mon_get_my_process_name(my_name, sizeof(my_name));
    TEST_CHK_FEOK(ferr);
    if (sysmsg)
        group = my_name;
    else
        group = "agroup"; // monitor won't find this name - so no change-notice
    key = "akey";
    value = "avalue";
    if (verbose)
        printf("calling msg_mon_reg_set\n");
    ferr = (short) msg_mon_reg_set(MS_Mon_ConfigType_Process,  // type
                                   (char *) group,             // group
                                   (char *) key,               // key
                                   (char *) value);            // value
    TEST_CHK_FEOK(ferr);
#ifndef USE_SB_DISABLED_MON_MSG
    if (sysmsg) {
        if (verbose)
            printf("expecting and waiting for change-notice\n");
        do {
            lerr = XWAIT(LREQ, -1);
            TEST_CHK_WAITIGNORE(lerr);
            lerr = XMSG_LISTEN_((short *) &sre, // sre
                                0,              // listenopts
                                0);             // listenertag
        } while (lerr == XSRETYPE_NOWORK);
        assert(sre.sre_flags & XSRE_MON);
        ferr = XMSG_READDATA_(sre.sre_msgId,  // msgid
                              recv_buffer,    // reqdata
                              BUFSIZ);        // bytecount
        util_check("XMSG_READDATA_", ferr);
        MS_Mon_Msg *msg = (MS_Mon_Msg *) recv_buffer;
        assert(msg->type == MS_MsgType_Change);
        XMSG_REPLY_(sre.sre_msgId,       // msgid
                    NULL,                // replyctrl
                    0,                   // replyctrlsize
                    NULL,                // replydata
                    0,                   // replydatasize
                    0,                   // errorclass
                    NULL);               // newphandle
        if (verbose)
            printf("change-notice received\n");
    }
#endif

    if (verbose)
        printf("calling msg_mon_reg_get - ok\n");
    ferr = (short) msg_mon_reg_get(MS_Mon_ConfigType_Process,    // type
                                   false,                        // next
                                   (char *) group,               // group
                                   (char *) key,                 // key
                                   &info);                       // info
    TEST_CHK_FEOK(ferr);
    assert(info.num_returned >= 1);
    for (inx = 0; inx < info.num_returned; inx++) {
        if (strcasecmp(key, info.list[inx].key) == 0) { // key upshifted
            match = true;
            assert(strcmp(value, info.list[inx].value) == 0);
        }
    }
    assert(match);

    if (verbose)
        printf("calling msg_mon_reg_get - invalid key\n");
    key = (char *) "bogusKey";
    ferr = (short) msg_mon_reg_get(MS_Mon_ConfigType_Process,    // type
                                   false,                        // next
                                   (char *) group,               // group
                                   (char *) key,                 // key
                                   &info);                       // info
    TEST_CHK_FEOK(ferr);
    assert(info.num_returned == 0);
    for (inx = 0; inx < info.num_returned; inx++)
        assert(strcasecmp(key, info.list[inx].key) != 0); // key upshifted

    if (verbose)
        printf("calling msg_mon_process_shutdown\n");
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    printf("if there were no asserts, all is well\n");
    return 0;
}
