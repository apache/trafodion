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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "vers.h"

#define SQ_LIO_SIGNAL_REQUEST_REPLY (SIGRTMAX - 4)

VERS_LIB(libseabasesig)

static int monitor_rtsigblock_proc() {
    sigset_t lv_sig_set;
    // Setup signal handling
    sigemptyset(&lv_sig_set);
    sigaddset(&lv_sig_set, SQ_LIO_SIGNAL_REQUEST_REPLY);
    int err = pthread_sigmask(SIG_BLOCK, &lv_sig_set, NULL);
    if (err)
        abort();
    return 0;
}

static int dummy_var = monitor_rtsigblock_proc();

