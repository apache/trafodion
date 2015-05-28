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
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "sqevlog/evl_sqlog_writer.h"

static bool print = true;

tmpl_sq_type_info_t _evlSqTmplTypeInfo[] = {
        /* size, align, isScalar, isInteger, name, default format */
        {0, 0,                  0, 0,   "none",         "%#x"},
        {SZALGN(char),          1, 1,   "char",         "%d"},
        {SZALGN(unsigned char), 1, 1,   "uchar",        "%u"},
        {SZALGN(short),         1, 1,   "short",        "%d"},
        {SZALGN(unsigned short),1, 1,   "ushort",       "%u"},
        {SZALGN(int),           1, 1,   "int",          "%d"},
        {SZALGN(unsigned int),  1, 1,   "uint",         "%u"},
        {SZALGN(long),          1, 1,   "long",         "%ld"},
        {SZALGN(unsigned long), 1, 1,   "ulong",        "%lu"},
        {sizeof(long long), LLALIGN,
                                1, 1,   "longlong",     "%Ld"},
        {sizeof(unsigned long long), ULLALIGN,
                                1, 1,   "ulonglong",    "%Lu"},
        {SZALGN(float),         1, 0,   "float",        "%f"},
        {sizeof(double), DALIGN,
                                1, 0,   "double",       "%f"},
        {sizeof(long double), LDALIGN,
                                1, 0,   "ldouble",      "%Lf"},
        {sizeof(char*), 0,      0, 0,   "string",       "%s"},
        {SZALGN(wchar_t),       1, 1,   "wchar",        "%lc"},
        {sizeof(wchar_t*), 0,   0, 0,   "wstring",      "%ls"},
        {SZALGN(void*),         1, 1,   "address",      "%p"},
        {0, 0,                  0, 0,   "struct",       "%#x"},
        {0, 0,                  0, 0,   "prefix3",      "%#x"},
        {0, 0,                  0, 0,   "list",         "%#x"},
        {0, 0,                  0, 0,   "struct",       "%#x"},/*TY_STRUCTNAME*/
        {0, 0,                  0, 0,   "typedef",      "%#x"}, /*TY_TYPEDEF*/
        {0, 0,                  0, 0,   NULL,           NULL}   /* the end */
};

//
// Simulate event logger
//

int evl_sqlog_add_token(char *buf,
                        int   tk_type,
                        void *tk_value) {
    buf = buf; // touch
    switch (tk_type) {
    case TY_CHAR:
    case TY_UCHAR:
    {
        int st = (int)(long) tk_value;
        if (print)
            fprintf(stderr, "<%c> ", st);
        break;
    }
    case TY_SHORT:
    case TY_USHORT:
    {
        int ust = (int)(long) tk_value;
        if (print)
            fprintf(stderr, "<%d> ", ust);
        break;
    }
    case TY_INT:
    case TY_UINT:
    {
        if (print)
            fprintf(stderr, "<%d> ", (int)(long) tk_value);
        break;
    }
    case TY_LONG:
    case TY_ULONG:
    {
        if (print)
            fprintf(stderr, "<%ld> ", (long) tk_value);
        break;
    }
    case TY_LONGLONG:
    case TY_ULONGLONG:
    {
        if (print)
            fprintf(stderr, "<%lld> ", (long long) tk_value);
        break;
    }
    case TY_ADDRESS:
    {
        if (print)
            fprintf(stderr, "<%p> ", (void*) tk_value);
        break;
    }
    case TY_FLOAT:
    {
        if (print)
            fprintf(stderr, "<%f> ", *((float *) tk_value));
        break;
    }
    case TY_DOUBLE:
    {
        if (print)
            fprintf(stderr, "<%f> ", *((double *) tk_value));
        break;
    }
    case TY_LDOUBLE:
    {
        if (print)
            fprintf(stderr, "<%Lf> ", *((long double *) tk_value));
        break;
    }
    case TY_STRING:
    {
        char *s = (char *) tk_value;
        if (print)
            fprintf(stderr, "<%s> ", s);
        break;
    }
    case TY_WCHAR:
    {
        wchar_t ws = (wchar_t )(long) tk_value;
        if (print)
            fprintf(stderr, "<%c> ", ws);
        break;
    }
    case TY_WSTRING:
    {
        wchar_t *s = (wchar_t *) tk_value;
        if (print)
            fprintf(stderr, "<%ls> ", s);
        break;
    }
    default:
        break;
    }
    return 0;
}

int evl_sqlog_add_array_token(char   *buf,
                              int     tk_type,
                              void   *tk_value,
                              size_t  count) {
    char   *array = (char*) tk_value;
    int     n = (int) count;
    size_t  size = _evlSqTmplTypeInfo[tk_type].ti_size;

    buf = buf; // touch
    switch (tk_type) {
    case TY_STRING:
    {
        /* array points to an array of char* */
        char **sarray = (char**) array;
        for (int i = 0; i < n; i++) {
            if (print)
                fprintf(stderr, "<%s> ", sarray[i]);
        }
        break;
    }
    case TY_WSTRING:
    {
        /* array points to an array of wchar_t* */
        wchar_t **warray = (wchar_t**) array;
        for (int i = 0; i < n; i++) {
            if (print)
                fprintf(stderr, "<%ls> ", warray[i]);
        }
    }
    default:
        for (char *a = array; n > 0; a += size, n--) {
        }
        break;
    }
    return 0;
}

int evl_sqlog_buf_used(char *buf) {
    buf = buf; // touch
    return 0;
}

int evl_sqlog_init(char   *buf,
                   size_t  buf_maxlen) {
    buf = buf; // touch
    buf_maxlen = buf_maxlen; // touch
    return 0;
}

int evl_sqlog_init_header(char               *buf,
                          size_t              buf_maxlen,
                          sq_common_header_t *common_tokens) {
    char ctimebuf[100];

    buf_maxlen = buf_maxlen; // touch
    time_t tim = time(NULL);
    char *s = ctime_r(&tim, ctimebuf);
    s[strlen(s)-1] = 0;        // remove \n
    static bool init = true;

    if (init) {
        init = false;
        char *env_va = getenv("SQ_EVLOG_NONE");
        if ((env_va != NULL) && (*env_va != '0'))
            print = false;
    }
    if (print) {
        fprintf(stderr, "==================================\nLog time: %s || ", s);
        fprintf(stderr, "<ComponentID> <Pid> <ZonIDd> <ThreadID> <Message...>: \n");
    }

    if (print) {
        char lhost[256];   /* local host name */
        if (gethostname(lhost, sizeof(lhost)) >= 0)
            fprintf(stderr, "Log host: %s || ", lhost);
        fprintf(stderr, "UserID: %d\n", (int)getuid());
    }
    int* componentID = (int*) (long) common_tokens->comp_id;
    int* processID = (int*) (long) common_tokens->process_id;
    int* zoneID = (int*) (long) common_tokens->zone_id;
    int* threadID = (int*) (long) common_tokens->thread_id;
    char *di = buf;
    evl_sqlog_add_token(di, TY_INT, componentID);
    evl_sqlog_add_token(di, TY_INT, processID);
    evl_sqlog_add_token(di, TY_INT, zoneID);
    evl_sqlog_add_token(di, TY_INT, threadID);
    return 0;
}

int evl_sqlog_write(posix_sqlog_facility_t  facility,
                    int                     event_type,
                    posix_sqlog_severity_t  severity,
                    char                   *evl_buf) {
    facility = facility; // touch
    evl_buf = evl_buf; // touch
    if (print)
        fprintf(stderr, "\nEvent Number:%d, Severity:%d. \n==================================\n\n",
                event_type, severity);
    return 0;
}

