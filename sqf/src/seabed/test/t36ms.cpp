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

#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"

int main(int argc, char *argv[]) {
    int                      disable;
    int                      ferr;
    MS_Mon_Process_Info_Type info;
    char                     name[MS_MON_MAX_PROCESS_NAME];
    int                      nid;
    int                      pid;

    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(true);
    ferr = msg_mon_process_startup(true); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_my_process_name(name, MS_MON_MAX_PROCESS_NAME);
    TEST_CHK_FEOK(ferr);
    printf("my name is %s\n", name);
    ferr = msg_mon_get_process_info((char *) "", &nid, &pid);
    TEST_CHK_FEOK(ferr);
#ifdef SQ_PHANDLE_VERIFIER
    ferr = msg_mon_get_process_name2(nid,
                                     pid,
                                     -1,
                                     name);
#else
    ferr = msg_mon_get_process_name(nid,
                                    pid,
                                    name);
#endif
    TEST_CHK_FEOK(ferr);
    disable = msg_test_assert_disable();
    ferr = msg_mon_get_process_name(nid,
                                    pid+1,
                                    name);
    assert(ferr == XZFIL_ERR_NOSUCHDEV);
    msg_test_assert_enable(disable);
    ferr = msg_mon_get_process_info_detail((char *) "", &info);
    TEST_CHK_FEOK(ferr);
    assert(strcmp(info.process_name, name) == 0);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
