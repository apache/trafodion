/**********************************************************************
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
********************************************************************/
#ifndef TCTRACE_H_
#define TCTRACE_H_

#include "seabed/trace.h"

// All request processing
#define TC_TRACE_REQUEST         0x00001
// Node configuration processing
#define TC_TRACE_NODE            0x00002
// Persist configuration processing
#define TC_TRACE_PERSIST         0x00004
// Persist configuration processing
#define TC_TRACE_PROCESS         0x00008
// Registry configuration processing
#define TC_TRACE_REGISTRY        0x00010
// Initialization/shutdown information
#define TC_TRACE_INIT            0x00020
// Event messages logged (error,informational,critical,etc.)
#define TC_TRACE_LOG_MSG         0x00040
// Entry and exit from methods/functions
#define TC_TRACE_ENTRY_EXIT      0x00080
// NameServer configuration processing
#define TC_TRACE_NAMESERVER      0x00100

#define TRACE_ENTRY \
   if (TcTraceSettings & TC_TRACE_ENTRY_EXIT) trace_printf("%s@%d\n", method_name, __LINE__)

#define TRACE_EXIT \
   if (TcTraceSettings & TC_TRACE_ENTRY_EXIT) trace_printf("%s@%d - Exit\n", method_name, __LINE__)

extern long TcTraceSettings;

class CTrafConfigTrace
{
public:
    CTrafConfigTrace();
    virtual ~CTrafConfigTrace() {}

    void TraceChange( const char *key, const char *value );
    void TraceClose( void );
    void TraceInit( bool traceEnabled
                  , const char *traceLevel
                  , const char *pfname);

private:

    const char *GetEnvStr( const char *key );
    void        GetEnvInt( const char *key, int &val );
    bool        GetEnvBool( const char *key );
    void        TraceHelpBool( const char *key
                             , const char *value
                             , const char *key_cmp
                             , bool &value_ret );
    void        TraceHelpInt( const char *key
                            , const char *value
                            , const char *key_cmp
                            , int &value_ret );
    const char *TraceHelpStr( const char *key
                            , const char *value
                            , const char *key_cmp );

    // The number of trace areas held in "traceAreaList"
    long  numTraceAreas_;
    bool  tracingEnabled_;

    // Save area for retaining prior trace settings if tracing is
    // disabled.  These are used to restore the values when tracing
    // is re-enabled.
    long  traceSettingsSaved_;

    // Optional size of trace file buffer.
    int   traceFileFb_;
    char *traceFileBase_;

    // Array of strings defining various trace areas and the
    // trace bit flag associated with that area
    typedef struct {const char *id; long bitFlag;} TraceArea_t;
    static const TraceArea_t traceAreaList_[];

    static const char *strTraceEnable_;
    static const char *strTraceFile_;
    static const char *strTraceFileFb_;
};

#endif /* TCTRACE_H_ */
