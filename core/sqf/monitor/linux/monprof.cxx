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

// LCOV_EXCL_START
// Exclude this module from monitor code coverage.  The code here is used
// only during development time in conjunction with cpu profiler measurements.

#include <dlfcn.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "monprof.h"

typedef void (*Mon_Prof_Start)(const char *);
typedef union {
    Mon_Prof_Start  icall;
    void           *ivoid;
} Mon_Prof_Start_Type;

typedef void (*Mon_Prof_Stop)();
typedef union {
    Mon_Prof_Stop  icall;
    void          *ivoid;
} Mon_Prof_Stop_Type;

static bool                mon_prof_inited = false;
static Mon_Prof_Stop_Type  mon_prof_stop;
static Mon_Prof_Start_Type mon_prof_start;

//
// if signal, then stop profiler
//
static void mon_profiler_handler(int, siginfo_t *, void *)
{
    mon_profiler_stop();
}

//
// try to open libprofiler.so and get ProfilerStart/ProfilerStop
//
static void mon_profiler_dl_init()
{
    void *handle;

    if (!mon_prof_inited)
    {
        mon_prof_inited = true;
        dlerror(); // clear error
        handle = dlopen("libprofiler.so", RTLD_NOW);
        if (handle == NULL)
        {
            mon_prof_start.ivoid = NULL;
            mon_prof_stop.ivoid = NULL;
        } else
        {
            mon_prof_start.ivoid = dlsym(handle, "ProfilerStart");
            mon_prof_stop.ivoid = dlsym(handle, "ProfilerStop");
        }
    }
}
    
//
// initializer profiler
//
void mon_profiler_init(char *prefix)
{
    struct sigaction act;
    char             fname[100];

    act.sa_sigaction = mon_profiler_handler;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGUSR2);
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR2, &act, NULL);
    sprintf(fname, "%s%d", prefix, getpid());
    mon_profiler_start(fname);
}

//
// start profiler
//
void mon_profiler_start(char *fname)
{
    mon_profiler_dl_init();
    if (mon_prof_start.ivoid != NULL)
        mon_prof_start.icall(fname);
}

//
// stop profiler
//
void mon_profiler_stop()
{
    mon_profiler_dl_init();
    if (mon_prof_stop.ivoid != NULL)
        mon_prof_stop.icall();
}

// LCOV_EXCL_STOP
