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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tversbin.h"

VERS_BIN(seabed_test_tcheck)

DEFINE_COMP_DOVERS(seabed_test_tcheck)

#include "propsx.cpp"

typedef enum {
    OP_CHECK = 1,
    OP_FAIL  = 2,
    OP_SETUP = 3
} Op_Type;

void fail(char *msg) {
    printf("%s did not succeed\n", (msg == NULL) ? "<test>" : msg);
    fflush(stdout);
    kill(0, SIGABRT);
}

void do_op(Op_Type op, char *msg, char *file) {
    FILE *f;

    switch (op) {
    case OP_CHECK:
        f = fopen(file, "r");
        if (f == NULL) {
            fail(msg);
        } else
            fclose(f);
        break;

    case OP_FAIL:
        fail(msg);
        break;

    case OP_SETUP:
        f = fopen(file, "r");
        if (f != NULL) {
            int err = unlink(file);
            if (err == -1)
                perror("unlink");
            fclose(f);
        }
        break;
    }
}

int main(int argc, char *argv[]) {
    const char *config_file = "ms.env";
    SB_Props    env;
    char       *msg = NULL;
    Op_Type     op = OP_CHECK;

    CALL_COMP_DOVERS(seabed_test_tcheck, argc, argv);
    for (int arg = 1; arg < argc; arg++) {
        char *p = argv[arg];
        if (strcmp(p, "-fail") == 0)
            op = OP_FAIL;
        else if (strcmp(p, "-setup") == 0)
            op = OP_SETUP;
        else if (*p != '-') {
            if (msg == NULL)
                msg = argv[arg];
        }
    }

    char *value = getenv("TEST_STOP");
    if (value != NULL) {
        do_op(op, msg, value);
        return 0;
    }
    if (!env.load(config_file)) { // try to load default config
        char *root = getenv("TRAF_HOME");
        if (root != NULL) {
            char la_file[BUFSIZ];
            sprintf(la_file, "%s/etc/%s", root, config_file);
            env.load(la_file);
        }
    }
    value = (char *) env.get("TEST_STOP");
    if (value != NULL) {
        do_op(op, msg, value);
        return 0;
    }
    setenv("TEST_STOP", "test.status", 1);
    return 0;
}

