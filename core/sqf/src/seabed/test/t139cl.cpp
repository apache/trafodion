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
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

#include "tutil.h"

void wrap_itoa_int(int num, int base, const char *inexp) {
    char  exp[40];
    char  str[40];
    char *str_end;

    memset(str, 'X', 20);
    if (inexp == NULL) {
        switch (base) {
        case 10:
            sprintf(exp, "%d", num);
            break;
        case 16:
            sprintf(exp, "%x", num);
            break;
        }
    } else
        strcpy(exp, inexp);
    str_end = SB_util_itoa_int(str, (unsigned int) num, base);
    *str_end = 0;
    if (strcmp(str, exp) != 0) {
        printf("num=%d, base=%d, exp=%s, act=%s\n", (int) num, base, exp, str);
        assert(strcmp(str, exp) == 0);
    }
}

void wrap_itoa_ptr(void *ptr) {
    char  exp[40];
    char  str[40];
    char *str_end;

    memset(str, 'X', 20);
    sprintf(exp, "%p", ptr);
    str_end = SB_util_itoa_ptr(str, ptr);
    *str_end = 0;
    if (strcmp(str, exp) != 0) {
        printf("ptr=%p, exp=%s, act=%s\n", ptr, exp, str);
        assert(strcmp(str, exp) == 0);
    }
}

int main() {

    util_test_start(true);
    wrap_itoa_int(1, 10, NULL);
    wrap_itoa_int(0, 10, NULL);
    wrap_itoa_int(-1, 10, NULL);
    wrap_itoa_int(2147483647, 10, NULL);

    wrap_itoa_int(1, 2, "1");
    wrap_itoa_int(0, 2, "0");
    wrap_itoa_int(-1, 2, "11111111111111111111111111111111");

    wrap_itoa_int(1, 16, "1");
    wrap_itoa_int(0, 16, "0");
    wrap_itoa_int(-1, 16, "ffffffff");

    wrap_itoa_ptr((void *) 1);
    wrap_itoa_ptr((void *) 0);
    wrap_itoa_ptr((void *) -1);

    util_test_finish(true);
    printf("if there were no asserts, all is well\n");
    return 0;
}
