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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tutilp.h"
#include "tverslib.h"

static void arg_proc_args_usage2(TAD2 *args, char **argv);

VERS_LIB(libsbztutilp)

static bool arg_is_mon_number(char *arg) {
    int inx;
    int len;

    len = (int) strlen(arg);

    if ((len != 5) && (len != 6))
        return false;
    for (inx = 0; inx < len; inx++)
        if (!isdigit(arg[inx]))
            return false;
    return true;
}

void arg_proc_args(TAD args[], bool inv_arg_ok, int argc, char **argv) {
    int   argi;
    int   count;
    TAD2 *largs;

    // convert TAD to TAD2 and then process
    for (argi = 0; args[argi].iv_arg_type != TA_End; argi++) {
    }
    count = argi;
    largs = new TAD2[count+1];
    for (argi = 0; argi <= count; argi++) {
        largs[argi].ip_arg_str = args[argi].ip_arg_str;
        largs[argi].iv_arg_type = args[argi].iv_arg_type;
        largs[argi].iv_arg_max = args[argi].iv_arg_max;
        largs[argi].ip_arg_ref = args[argi].ip_arg_ref;
        largs[argi].ipp_arg_strs = NULL;
    }
    arg_proc_args2(largs, inv_arg_ok, argc, argv);
    delete [] largs;
}

void arg_proc_args2(TAD2 *args, bool inv_arg_ok, int argc, char **argv) {
    int          arg;
    bool         arg_ok;
    bool         arg_match;
    int          arg_max;
    void        *arg_ref;
    bool        *arg_ref_bool;
    int         *arg_ref_int;
    short       *arg_ref_shrt;
    char       **arg_ref_str;
    int          arg_start;
    const char  *arg_str;
    TSD         *arg_strs;
    int          arg_type;
    char        *argv_str;
    int          argi;
    int          argis;

    // validate arg-table [in sorted order]
    for (argi = 1; args[argi].iv_arg_type != TA_End; argi++)
        assert(strcmp(args[argi].ip_arg_str, args[argi-1].ip_arg_str) > 0);

    arg_start = 1;
#ifdef SQ_PHANDLE_VERIFIER
    if (argc >= 11) {
        // "SQMON1.1" <pnid> <nid> <pid> <pname> <port> <ptype> <zid> <verif> "SPARE"
        //     [1]     [2]    [3]   [4]    [5]    [6]    [7]     [8]    [9]     [10]
        if ((!strcmp(argv[1], "SQMON1.1")) &&
            arg_is_mon_number(argv[2]) && // pnid
            arg_is_mon_number(argv[3]) && // nid
            arg_is_mon_number(argv[4]) && // pid
            arg_is_mon_number(argv[7]) && // ptype
            arg_is_mon_number(argv[8]) && // zid
            (strlen(argv[5]) >= 2) &&     // pname
            (argv[5][0] == '$'))
            arg_start = 11;
    }
#else
    if (argc >= 10) {
        // "SQMON1.0" <pnid> <nid> <pid> <pname> <port> <ptype> <zid> "SPARE"
        //     [1]     [2]    [3]   [4]    [5]    [6]    [7]     [8]    [9]
        if ((!strcmp(argv[1], "SQMON1.0")) &&
            arg_is_mon_number(argv[2]) && // pnid
            arg_is_mon_number(argv[3]) && // nid
            arg_is_mon_number(argv[4]) && // pid
            arg_is_mon_number(argv[7]) && // ptype
            arg_is_mon_number(argv[8]) && // zid
            (strlen(argv[5]) >= 2) &&     // pname
            (argv[5][0] == '$'))
            arg_start = 10;
    }
#endif
    for (arg = arg_start; arg < argc; arg++) {
        argv_str = argv[arg];
        arg_ok = false;
        for (argi = 0; args[argi].iv_arg_type != TA_End; argi++) {
            if (argv_str == NULL)
                break;
            arg_str = args[argi].ip_arg_str;
            if (strcmp(argv_str, arg_str) == 0) {
                arg_ok = true;
                arg_type = args[argi].iv_arg_type;
                arg_max = args[argi].iv_arg_max;
                arg_ref =  args[argi].ip_arg_ref;
                arg_strs =  args[argi].ipp_arg_strs;
                switch (arg_type) {
                case TA_Bool:
                    assert(arg_ref != NULL);
                    arg_ref_bool = (bool *) arg_ref;
                    *arg_ref_bool = true;
                    break;
                case TA_Ign:
                    assert(arg_ref == NULL);
                    break;
                case TA_Int:
                    assert(arg_ref != NULL);
                    arg_ref_int = (int *) arg_ref;
                    arg++;
                    argv_str = argv[arg];
                    if (arg >= argc) {
                        printf("expecting <%s>\n", &arg_str[1]);
                        abort();
                    }
                    *arg_ref_int = atoi(argv_str);
                    if ((arg_max != TA_NOMAX) && (*arg_ref_int > arg_max))
                        *arg_ref_int = arg_max;
                    break;
                case TA_Next:
                    assert(arg_ref == NULL);
                    arg++;
                    argv_str = argv[arg];
                    if (arg >= argc) {
                        printf("expecting <%s>\n", &arg_str[1]);
                        abort();
                    }
                    break;
                case TA_Shrt:
                    assert(arg_ref != NULL);
                    arg_ref_shrt = (short *) arg_ref;
                    arg++;
                    argv_str = argv[arg];
                    if (arg >= argc) {
                        printf("expecting <%s>\n", &arg_str[1]);
                        abort();
                    }
                    *arg_ref_shrt = (short) atoi(argv_str);
                    if ((arg_max != TA_NOMAX) && (*arg_ref_shrt > arg_max))
                        *arg_ref_shrt = (short) arg_max;
                    break;
                case TA_Str:
                    assert(arg_ref != NULL);
                    arg_ref_str = (char **) arg_ref;
                    arg++;
                    argv_str = argv[arg];
                    if (arg >= argc) {
                        printf("expecting <%s>\n", &arg_str[1]);
                        abort();
                    }
                    *arg_ref_str = argv_str;
                    if (arg_strs != NULL) {
                        arg_match = false;
                        for (argis = 0;
                             arg_strs[argis].ip_str != NULL;
                             argis++) {
                            if (strcmp(argv_str, arg_strs[argis].ip_str) == 0) {
                                arg_match = true;
                                break;
                            }
                        }
                        if (!arg_match) {
                            printf("expecting %s to be one of: ", arg_str);
                            for (argis = 0;
                                 arg_strs[argis].ip_str != NULL;
                                 argis++)
                                printf("%s ", arg_strs[argis].ip_str);
                            printf("\n");
                            abort();
                        }
                    }
                    break;
                default:
                    printf("bad arg-type=%d\n", arg_type);
                    abort();
                    break;
                }
                break;
            } else if (strcmp(argv_str, "-help") == 0) {
                arg_proc_args_usage2(args, argv);
                exit(1);
            }
        }
        if (!inv_arg_ok && !arg_ok) {
            printf("unexpected arg '%s'\n", argv_str);
            arg_proc_args_usage2(args, argv);
            abort();
        }
    }
}

void arg_proc_args_usage2(TAD2 *args, char **argv) {
    int   argi;
    char *argv0;

    argv0 = rindex(argv[0], '/');
    if (argv0 != NULL)
        argv0++;
    else
        argv0 = argv[0];

    printf("usage: %s", argv0);
    for (argi = 0; args[argi].iv_arg_type != TA_End; argi++) {
        switch (args[argi].iv_arg_type) {
        case TA_Ign:
            break;
        default:
            printf(" %s",  args[argi].ip_arg_str);
        }
        switch (args[argi].iv_arg_type) {
        case TA_Int:
        case TA_Next:
        case TA_Str:
            printf(" <%s>",  &args[argi].ip_arg_str[1]);
            break;
        default:
            break;
        }
    }
    printf("\n");
}

bool arg_proc_str(TSD         strs[],
                  const char *str,
                  int        *str_val) {
    bool match;
    int  stri;

    match = false;
    for (stri = 0; strs[stri].ip_str != NULL; stri++) {
        if (strcmp(str, strs[stri].ip_str) == 0) {
            match = true;
            break;
        }
    }
    if (match)
        *str_val = strs[stri].iv_str_val;
    return match;
}
