// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
#include <wchar.h>

char    a[100];
char    b[100];
wchar_t wa[100];
wchar_t wb[100];

#define EXP_OVERLAP(a) fprintf(stderr, "expecting %s overlap at %s:%d\n", a, __FILE__, __LINE__ + 1)

int main() {
    void *pv;

    setenv("OVERLAP_NONFATAL", "1", 1);
    b[0] = 3;
    b[1] = 7;
    memcpy(a, b, 2);              // verified
    assert(a[0] == b[0]);
    assert(a[1] == b[1]);
    memcpy(b, a, 2);              // verified
    assert(a[0] == b[0]);
    assert(a[1] == b[1]);
    EXP_OVERLAP("memcpy");
    memcpy(&a[0], &a[1], 2);      // verified
    EXP_OVERLAP("memcpy");
    memcpy(&a[1], &a[0], 2);      // verified

    memset(b, 0, 2);
    pv = memccpy(a, b, 1, 2);     // verified
    assert(a[0] == 0);
    assert(a[1] == 0);
    assert(pv == NULL);
    a[0] = 1;
    pv = memccpy(a, b, 0, 2);     // verified
    assert(a[0] == 0);
    assert(pv == &a[1]);
    memccpy(b, a, 1, 2);          // verified
    assert(b[0] == 0);
    assert(b[1] == 0);
    memset(a, 0, 2);
    EXP_OVERLAP("memccpy");
    memccpy(&a[0], &a[1], 1, 2);  // verified
    EXP_OVERLAP("memccpy");
    memccpy(&a[1], &a[0], 1, 2);  // verified

    strcpy(a, "str");
    strcpy(b, a);                 // verified
    strcpy(a, b);                 // verified
    EXP_OVERLAP("strcpy");
    strcpy(&a[0], &a[1]);         // verified
    EXP_OVERLAP("strcpy");
    strcpy(&a[1], &a[0]);         // verified

    strcpy(a, "stra");
    strcpy(b, "strb");
    strncpy(b, a, 4);             // verified
    assert(memcmp(b, "stra\0", 5) == 0);
    strncpy(a, b, 4);             // verified
    assert(memcmp(a, "stra\0", 5) == 0);
    strcpy(a, "stra");
    strcpy(b, "strb");
    strncpy(b, a, 4);             // verified
    assert(memcmp(b, "stra\0", 5) == 0);
    strncpy(a, b, 4);             // verified
    assert(memcmp(a, "stra\0", 5) == 0);
    EXP_OVERLAP("strncpy");
    strncpy(&a[0], &a[1], 4);     // verified
    EXP_OVERLAP("strncpy");
    strncpy(&a[1], &a[0], 4);     // verified

    strcpy(a, "str");
    b[0] = 0;
    strcat(b, a);                 // verified
    assert(memcmp(b, "str\0", 4) == 0);
    strcat(a, b);                 // verified
    assert(memcmp(a, "strstr\0", 7) == 0);
    EXP_OVERLAP("strcat");
    strcat(&a[0], &a[1]);         // verified
    EXP_OVERLAP("strcat");
    strcat(&a[1], &a[0]);         // verified

    strcpy(a, "str");
    b[0] = 0;
    b[2] = 0;
    strncat(b, a, 2);             // verified
    assert(memcmp(b, "st\0", 3) == 0);
    strncat(a, b, 2);             // verified
    assert(memcmp(a, "str", 3) == 0);
    EXP_OVERLAP("strncat");
    strncat(&a[0], &a[1], 2);     // verified
    EXP_OVERLAP("strncat");
    strncat(&a[1], &a[0], 2);     // verified

    mbstowcs(wa, "str", 4);
    wcscpy(wb, wa);               // verified
    wcscpy(wa, wb);               // verified
    EXP_OVERLAP("wcscpy");
    wcscpy(&wa[0], &wa[1]);       // verified
    wa[0] = 0;
    wa[1] = 0;
    EXP_OVERLAP("wcscpy");
    wcscpy(&wa[1], &wa[0]);       // verified

    mbstowcs(wa, "stra", 5);
    mbstowcs(wb, "strb", 5);
    wcscat(wb, wa);               // verified
    wcscat(wa, wb);               // verified
    mbstowcs(wa, "stra", 5);
    mbstowcs(wb, "strb", 5);
    wa[5] = 0;
    wa[6] = 0;
//  wcscat(&wa[0], &wa[1]);       // verified
//  wcscat(&wa[1], &wa[0]);       // verified

    mbstowcs(wa, "str", 4);
    wcsncpy(wb, wa, 2);           // verified
    wcsncpy(wa, wb, 2);           // verified
    EXP_OVERLAP("wcsncpy");
    wcsncpy(&wa[0], &wa[1], 2);   // verified
    EXP_OVERLAP("wcsncpy");
    wcsncpy(&wa[1], &wa[0], 2);   // verified

    mbstowcs(wa, "str", 4);
    wb[0] = 0;
    wcsncat(wb, wa, 2);           // verified
    wcsncat(wa, wb, 2);           // verified
    EXP_OVERLAP("wcsncat");
    wcsncat(&wa[0], &wa[1], 2);   // verified
    EXP_OVERLAP("wcsncat");
    wcsncat(&wa[1], &wa[0], 2);   // verified

    return 0;
}
