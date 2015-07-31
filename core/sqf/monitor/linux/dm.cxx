///////////////////////////////////////////////////////////////////////////////
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
//
///////////////////////////////////////////////////////////////////////////////

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "dm.h"

class XXX_Dtor {
public:
XXX_Dtor() {}
~XXX_Dtor();
};

typedef void (*Setup)(const char *);
typedef void (*Shutdown)(void);
typedef void *(*Malloc)(size_t);

typedef union {
    Malloc    imalloc;
    void     *ipmalloc;
} Malloc_Type;

typedef union {
    Setup  isetup;
    void  *ipsetup;
} Setup_Type;

typedef union {
    Shutdown  ishutdown;
    void     *ipshutdown;
} Shutdown_Type;

static XXX_Dtor  gdtor;
Malloc_Type      gmallocd = { NULL };
Malloc_Type      gmallocg = { NULL };
Shutdown_Type    gshutdown = { NULL };

//
// On destruction, call shutdown
// Doing it here seems to get around the problem
// of being called before the global destructors are called.
//
// This works because this library seems to unload
// after the other libraries we are interested in.
//
XXX_Dtor::~XXX_Dtor() {
    if (gshutdown.ipshutdown != NULL) {
        FILE *lp_file = fopen("/proc/self/cmdline", "r");
        if (lp_file != NULL) {
            char  la_line[BUFSIZ];
            char *lp_s = fgets(la_line, sizeof(la_line), lp_file);
            if (lp_s != NULL)
                printf("dmalloc shutdown, prog=%s, pid=%d\n",
                       la_line, getpid());
            fclose(lp_file);
        }
        gshutdown.ishutdown();
    }
}

void util_dmalloc_kill() {
    gshutdown.ipshutdown = NULL;
}

void util_dmalloc_start() {
    void        *handle;
    char        *options;
    Setup_Type   setup;
    static bool  started = false;

    if (started)
        return;
    started = true;
    handle = dlopen("libdmalloc.so", RTLD_LAZY);
    if (handle != NULL) {
        options = getenv("DMALLOC_OPTIONS");
        if (options != NULL) {
            gmallocg.imalloc = malloc;
            gmallocd.ipmalloc = dlsym(handle, "malloc");
            setup.ipsetup = dlsym(handle, "dmalloc_debug_setup");
            if (setup.ipsetup != NULL)
                setup.isetup(options);
            gshutdown.ipshutdown = dlsym(handle, "dmalloc_shutdown");
        } else
            dlclose(handle);
    }
}

void util_dmalloc_stop() {
printf("shutdown, ipshutdown=%p\n", (void *) gshutdown.ipshutdown);
    if (gshutdown.ipshutdown != NULL) {
        gshutdown.ishutdown();
        gshutdown.ipshutdown = NULL;
    }
}

void util_debug_hook(const char *pp_who, const char *pp_fname) {
    FILE *lp_f;
    char *lp_p;
    int   lv_enable;
    bool  lv_fexists = false;

    if (lv_fexists)
        return;
    lp_p = getenv("HOOK_ENABLE");
    if (lp_p == NULL)
        return;
    lv_enable = atoi(lp_p);
    if (!lv_enable)
        return;
    lp_f = fopen(pp_fname, "r");
    if (lp_f) {
        fclose(lp_f);
        lv_fexists = true;
    } else {
        printf("%s: create file %s/%s to continue\n", pp_who, getenv("PWD"), pp_fname);
        printf("%s: slave pid=%d\n", pp_who, getpid());
        fflush(stdout);
        for (;;) {
            lp_f = fopen(pp_fname, "r");
            if (lp_f) {
                fclose(lp_f);
                lv_fexists = true;
                printf("%s: %s detected - continuing\n", pp_who, pp_fname);
                fflush(stdout);
                break;
            }
            sleep(1);
        }
    }
}
