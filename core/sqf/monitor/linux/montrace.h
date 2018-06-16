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

using namespace std;

#include <string>
#include "seabed/trace.h"

// Monitor request processing
#define TRACE_REQUEST         0x000001
#define TRACE_REQUEST_DETAIL  0x000002
#define TRACE_PROCESS         0x000004
#define TRACE_PROCESS_DETAIL  0x000008
// Monitor synchronization cycle
#define TRACE_SYNC            0x000010
#define TRACE_SYNC_DETAIL     0x000020
// Notice processing
#define TRACE_NOTICE          0x000040
#define TRACE_NOTICE_DETAIL   0x000080
// Local I/O
#define TRACE_MLIO            0x000100
#define TRACE_MLIO_DETAIL     0x000200
// TMSync processing
#define TRACE_TMSYNC          0x000400
// Monitor initialization/shutdown information
#define TRACE_INIT            0x000800
// Failed node recovery
#define TRACE_RECOVERY        0x001000
// Monitor statistics
#define TRACE_STATS           0x002000
// Events logged to event log
#define TRACE_EVLOG_MSG       0x004000
// Entry and exit from methods
#define TRACE_ENTRY_EXIT      0x008000
// Input/output redirection from child processes
#define TRACE_REDIRECTION     0x010000
// Cluster Configration
#define TRACE_TRAFCONFIG      0x020000
// Health check
#define TRACE_HEALTH          0x040000
// Signal handler
#define TRACE_SIG_HANDLER     0x080000
// nameserver
#define TRACE_NS              0x100000
// lightweight measure
#define TRACE_MEAS            0x200000


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

    inline const char *getTraceFileName(void) { return( traceFileName_.c_str() ); };

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

    bool tracingEnabled_;

    // The number of trace areas held in "traceAreaList"
    int     numTraceAreas_;

    // Save area for retaining prior trace settings if tracing is
    // disabled.  These are used to restore the values when tracing
    // is re-enabled.
    long    traceSettingsSaved_;

    int     traceFileFB_;  // Optional size of trace file buffer.
    char   *traceFileBase_;
    string  traceFileName_;

    // Array of strings defining various trace areas and the
    // trace bit flag associated with that area
    typedef struct {const char *id; long bitFlag;} TraceArea_t;
    static const TraceArea_t traceAreaList_[];

    static const char *traceEnableStr_;
    static const char *traceFileStr_;
    static const char *traceFileFbStr_;
};

#endif
