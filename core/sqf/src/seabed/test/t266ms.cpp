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
#include <pthread.h>
#include <stdio.h>

#include "tutil.h"
#include "tutilp.h"

extern void SB_thread_print_specific(pthread_key_t pv_key);

int main(int argc, char *argv[]) {
    enum {        MAX_KEYS = 50 };
    int           err;
    int           inx;
    pthread_key_t key[MAX_KEYS];
    bool          verbose = false;
    TAD           zargs[] = {
      { "-cluster",   TA_Ign,  TA_NOMAX,    NULL       },
      { "-server",    TA_Ign,  TA_NOMAX,    NULL       },
      { "-v",         TA_Bool, TA_NOMAX,    &verbose   },
      { "-verbose",   TA_Ign,  TA_NOMAX,    NULL       },
      { "",           TA_End,  TA_NOMAX,    NULL       }
    };

    arg_proc_args(zargs, false, argc, argv);
    util_test_start(true);
    for (inx = 0; inx < MAX_KEYS; inx++) {
        err = pthread_key_create(&key[inx], NULL);
        assert(err == 0);
        err = pthread_setspecific(key[inx], (const void *) (long) (inx + 1));
        assert(err == 0);
    }
    for (inx = 0; inx < MAX_KEYS; inx++) {
        SB_thread_print_specific(key[inx]);
    }

    util_test_finish(true);
    return 0;
}
