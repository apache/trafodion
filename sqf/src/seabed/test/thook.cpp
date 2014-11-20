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

#include <stdio.h>
#include <unistd.h>

#include "tutilp.h"

//
// Purpose: debug hook
//
void test_debug_hook(const char *p_who, const char *p_fname) {
    FILE        *f;
    static int   fexists = 0;

    if (fexists)
        return;
    f = fopen(p_fname, "r");
    if (f) {
        fclose(f);
        fexists = 1;
    } else {
        printf("%s: create file %s to continue\n", p_who, p_fname);
        printf("%s: slave pid=%d\n", p_who, getpid());
        fflush(stdout);
        for (;;) {
            f = fopen(p_fname, "r");
            if (f) {
                fclose(f);
                fexists = 1;
                printf("%s: %s detected - continuing\n", p_who, p_fname);
                fflush(stdout);
                break;
            }
            sleep(1);
        }
    }
}

