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

#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "ttrans.h"
#include "tutil.h"

int tm_inherit_tx(MS_Mon_Tmlib_Fun_Type  fun,
                  MS_Mon_Transid_Type    transid,
                  MS_Mon_Transid_Type   *transid_out) {
    static MS_Mon_Transid_Type current_transid;
    fun = fun;         // no-warn
    if (fun != TMLIB_FUN_GET_TX)
        current_transid = transid;
    *transid_out = current_transid;
    return 0;
}

int main(int argc, char *argv[]) {
    int                 ferr;
    int                 nid;
    int                 pid;
    MS_Mon_Transid_Type transid;

    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(true);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_process_info((char *) "", &nid, &pid);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_trans_register_tmlib(tm_inherit_tx);
    TEST_CHK_FEOK(ferr);
    TRANSID_SET_SEQ(transid, 1);
    ferr = msg_mon_trans_enlist(nid, pid, transid);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_trans_delist(nid, pid, transid);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_trans_end(nid, pid, transid);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    printf("if there were no asserts, all is well\n");
    return 0;
}
