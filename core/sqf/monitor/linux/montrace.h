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

#ifndef MONTRACE_H_
#define MONTRACE_H_

#include "seabed/trace.h"

// Monitor request processing
#define TRACE_REQUEST         0x00001
#define TRACE_REQUEST_DETAIL  0x00002
#define TRACE_PROCESS         0x00004
#define TRACE_PROCESS_DETAIL  0x00008
// Monitor synchronization cycle
#define TRACE_SYNC            0x00010
#define TRACE_SYNC_DETAIL     0x00020
// Notice processing
#define TRACE_NOTICE          0x00040
#define TRACE_NOTICE_DETAIL   0x00080
// Local I/O
#define TRACE_MLIO            0x00100
#define TRACE_MLIO_DETAIL     0x00200
// TMSync processing
#define TRACE_TMSYNC          0x00400
// Monitor initialization/shutdown information
#define TRACE_INIT            0x00800
// Failed node recovery
#define TRACE_RECOVERY        0x01000
// Monitor statistics
#define TRACE_STATS           0x02000
// Events logged to event log
#define TRACE_EVLOG_MSG       0x04000
// Entry and exit from methods
#define TRACE_ENTRY_EXIT      0x08000
// Input/output redirection from child processes
#define TRACE_REDIRECTION     0x10000
// Cluster Configration
#define TRACE_CLUST_CONF        0x20000
#define TRACE_CLUST_CONF_DETAIL 0x20000
// Health check
#define TRACE_HEALTH          0x40000
// Signal handler
#define TRACE_SIG_HANDLER     0x80000


#define TRACE_ENTRY \
   if (trace_settings & TRACE_ENTRY_EXIT) trace_printf("%s@%d\n", method_name, __LINE__)

#define TRACE_EXIT \
   if (trace_settings & TRACE_ENTRY_EXIT) trace_printf("%s@%d - Exit\n", method_name, __LINE__)

extern long trace_settings;

class CMonTrace
{
public:
    CMonTrace();
    virtual ~CMonTrace() {}

    void mon_trace_init(const char * traceLevel, const char *pfname);

    void mon_trace_change(const char *key, const char *value);

private:

    const char *getenv_str(const char *key);
    void getenv_int(const char *key, int &val);
    bool getenv_bool(const char *key);

    void mon_help_bool(const char *key, const char *value,
                       const char *key_cmp, bool &value_ret);

    void mon_help_int(const char *key, const char *value,
                      const char *key_cmp, int &value_ret);

    const char *mon_help_str(const char *key, const char *value,
                             const char *key_cmp);

    // The number of trace areas held in "traceAreaList"
    int numTraceAreas;

    bool tracingEnabled;

    // Save area for retaining prior trace settings if tracing is
    // disabled.  These are used to restore the values when tracing
    // is re-enabled.
    long trace_settings_saved;

    // Optional size of trace file buffer.
    int  trace_file_fb;

    char *trace_file_base;

    // Array of strings defining various trace areas and the
    // trace bit flag associated with that area
    typedef struct {const char *id; long bitFlag;} traceArea;
    static const traceArea traceAreaList[];

    static const char *str_trace_enable;
    static const char *str_trace_file;
    static const char *str_trace_file_fb;
};

#endif
