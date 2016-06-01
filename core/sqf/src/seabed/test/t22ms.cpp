//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seabed/fserr.h"
#include "seabed/ms.h"

#include "tchkfe.h"
#include "tmsfsutil.h"
#include "tutil.h"

int main(int argc, char *argv[]) {
    int ferr;
    int nid;
    int pid;

    msfs_util_init(&argc, &argv, msg_debug_hook);
    util_test_start(true);
    ferr = msg_mon_process_startup(false); // system messages
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_process_info((char *) "$cli", &nid, &pid);
    printf("$cli ferr=%d, nid=%d, pid=%d\n", ferr, nid, pid);
    TEST_CHK_FEOK(ferr);
    ferr = msg_mon_get_process_info((char *) "$bog", &nid, &pid);
    printf("$bog ferr=%d, nid=%d, pid=%d\n", ferr, nid, pid);
    assert(ferr == XZFIL_ERR_NOSUCHDEV);
    sleep(1);
    ferr = msg_mon_process_shutdown();
    TEST_CHK_FEOK(ferr);
    util_test_finish(true);
    return 0;
}
