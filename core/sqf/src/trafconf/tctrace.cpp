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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

#include "tctrace.h"

#define TC_PROCESS_PATH_MAX    256

const char *CTrafConfigTrace::strTraceEnable_ = "TC_TRACE_ENABLE";
const char *CTrafConfigTrace::strTraceFile_   = "TC_TRACE_FILE";
const char *CTrafConfigTrace::strTraceFileFb_ = "TC_TRACE_FILE_FB";

const CTrafConfigTrace::TraceArea_t CTrafConfigTrace::traceAreaList_[] =
{ 
    {"TC_TRACE_REQUEST",         TC_TRACE_REQUEST},
    {"TC_TRACE_NODE",            TC_TRACE_NODE},
    {"TC_TRACE_PERSIST",         TC_TRACE_PERSIST},
    {"TC_TRACE_PROCESS",         TC_TRACE_PROCESS},
    {"TC_TRACE_REGISTRY",        TC_TRACE_REGISTRY},
    {"TC_TRACE_INIT",            TC_TRACE_INIT},
    {"TC_TRACE_LOG_MSG",         TC_TRACE_LOG_MSG},
    {"TC_TRACE_ENTRY_EXIT",      TC_TRACE_ENTRY_EXIT},
    {"TC_TRACE_NAMESERVER",      TC_TRACE_NAMESERVER}
};

// Global trace flags
long TcTraceSettings = 0;

CTrafConfigTrace::CTrafConfigTrace() 
                : tracingEnabled_(false)
                , traceSettingsSaved_(0)
                , traceFileFb_(0)
                , traceFileBase_(NULL)
{
    numTraceAreas_ = (int)(sizeof(traceAreaList_)/sizeof(TraceArea_t));
}

const char *CTrafConfigTrace::GetEnvStr(const char *key)
{
    const char *value;
    value = getenv(key);
    return value;
}

void CTrafConfigTrace::GetEnvInt(const char *key, int &val)
{
    const char *p = GetEnvStr(key);
    if (p != NULL)
    {
        val = atoi(p);
    }
}

bool CTrafConfigTrace::GetEnvBool(const char *key)
{
    const char *p = GetEnvStr(key);
    bool val = false;
    if (p != NULL)
    {
        val = atoi(p);
    }
    return val;
}

void CTrafConfigTrace::TraceClose( void )
{
    trace_close();
}

void CTrafConfigTrace::TraceInit( bool traceEnabled
                                , const char *traceLevel
                                , const char *pfname)
{
    bool pathnameUsed = false;
    char trace_file_name[TC_PROCESS_PATH_MAX];
    char hostname[TC_PROCESS_PATH_MAX];

    tracingEnabled_ = traceEnabled;

    if (gethostname(hostname,sizeof(hostname)) == -1)
    {
        sprintf( hostname,"TC");
    }

    if (pfname == NULL)
    {   // Caller did not specify a trace file name, get name from
        // environment variable if specified.
        pfname = GetEnvStr(strTraceFile_);
    }
    
    if (pfname == NULL)
    {   // Use default prefix
        pfname = "tctrace";
    }

    if (pfname != NULL)
    {   // User specified trace file name

        if ( pfname[0] == '/')
        {
            // Use filename passed in
            strcpy(trace_file_name, pfname);
            pathnameUsed = true;
        }
        else
        {
            // Format default trace file name and remove any existing trace file.
            if( getenv("SQ_VIRTUAL_NODES") )
            {
                sprintf( trace_file_name,"%s/trafconfig.trace.%d.%s"
                       , getenv("MPI_TMPDIR")
                       , getpid()
                       , hostname);
            }
            else
            {
                sprintf( trace_file_name,"%s/trafconfig.trace.%s"
                       , getenv("MPI_TMPDIR")
                       , hostname);
            }
            
            if ((strcmp(pfname, "STDOUT") == 0)
              || strcmp(pfname, "STDERR") == 0)
            {
                strcpy(trace_file_name, pfname);
            }
            else // Make user specified file name unique per node
            {
                sprintf(trace_file_name,"%s/%s.%d.%s"
                        , getenv("MPI_TMPDIR")
                        , pfname
                        , getpid()
                        , hostname);
            }
        }
    }

    if (!pathnameUsed)
    {
        remove(trace_file_name);
    }

    // Get any trace settings that were specified via environment variables
    const char *value;

    for (int i=0; i<numTraceAreas_; i++)
    {
        value = getenv(traceAreaList_[i].id);
        if (value != NULL)
        {
            if (atoi(value) != 0)
                // set the enabled flag for this trace area
                TcTraceSettings |= traceAreaList_[i].bitFlag;
        }
    }

    if (!tracingEnabled_)
    {
        // Get environment variable specifying whether tracing is enabled
        tracingEnabled_ = GetEnvBool(strTraceEnable_);
        // Get environment variable value for trace buffer size if specified
        GetEnvInt(strTraceFileFb_, traceFileFb_);
    }

    // Convert the user specified trace level string to a number.  The
    // number can be specified as a decimal, octal or hexadecimal
    // constant.  Combine these flags with current TcTraceSettings that
    // may have been set via environment variables.
    long trace_flags;
    trace_flags = strtol(traceLevel, NULL, 0);
    if (errno != ERANGE)
    {
        TcTraceSettings |= trace_flags;
    }

    // If any trace settings were specified initialize the trace file
    if (TcTraceSettings != 0 || tracingEnabled_)
    {
        traceSettingsSaved_ = TcTraceSettings;

        if (pfname != NULL)
        {
            trace_init(trace_file_name,
                       false,  // don't append pid to file name
                       pathnameUsed?(char*)"":(char*)"trafconfig", // file prefix
                       false);
            if (traceFileFb_ > 0)
            {
                trace_set_mem(traceFileFb_);
            }
        }
    }
}


void CTrafConfigTrace::TraceHelpBool( const char *key
                                    , const char *value
                                    , const char *key_cmp
                                    , bool &value_ret )
{
    if ((key != NULL) && (strcasecmp(key, key_cmp) == 0))
    {
        value_ret = atoi(value);
    }
}

void CTrafConfigTrace::TraceHelpInt( const char *key
                                   , const char *value
                                   , const char *key_cmp
                                   , int &value_ret )
{
    if ((key != NULL) && (strcasecmp(key, key_cmp) == 0))
    {
        value_ret = atoi(value);
    }
}

const char *CTrafConfigTrace::TraceHelpStr( const char *key
                                          , const char *value
                                          , const char *key_cmp )
{
    if ((key != NULL) && (strcasecmp(key, key_cmp) == 0))
    {
        return value;
    }
    return NULL;
}


void CTrafConfigTrace::TraceChange(const char *key, const char *value)
{
    bool trace_was_enabled = tracingEnabled_;
    const char *pfname;
    int  old_fb = traceFileFb_;

    if (key == NULL)
        return;

    // Restore saved trace settings in case trace flags get modified
    TcTraceSettings = traceSettingsSaved_;

    // Compare the key with each of the trace flag strings.  When
    // there is an equal compare, assign the value to the appropriate flag.
    for (int i=0; i<numTraceAreas_; i++)
    {
        if (strcasecmp(key, traceAreaList_[i].id) == 0)
        {
            if (atoi(value) != 0)
            {
                // set the enabled flag for this trace area
                TcTraceSettings |= traceAreaList_[i].bitFlag;
            }
            else // clear the enabled flag for this trace area
            {
                TcTraceSettings &= ~traceAreaList_[i].bitFlag;
            }
            break;
        }
    }
    // Save current trace settings
    traceSettingsSaved_ = TcTraceSettings;


    // Check if tracing is being enabled/disabled
    TraceHelpBool(key, value, strTraceEnable_, tracingEnabled_);
    // Check if trace file buffer size is being specified
    TraceHelpInt(key, value, strTraceFileFb_, traceFileFb_);

    // Check if trace file base name is being specified
    pfname = TraceHelpStr(key, value, strTraceFile_);
    if (pfname != NULL)
    {   // Save trace file base name
        delete [] traceFileBase_;
        traceFileBase_ = new char[strlen(pfname) + 1];
        strcpy(traceFileBase_, pfname);
    }

    if (!trace_was_enabled && tracingEnabled_)
    {
        char fname[TC_PROCESS_PATH_MAX];
        char hostname[TC_PROCESS_PATH_MAX];
        if (gethostname(hostname,sizeof(hostname)) == -1)
        {
            sprintf( hostname,"TC");
        }

        // Formulate trace file name
        if (traceFileBase_ != NULL)
        {  // User specified trace file name
            if ((strcmp(traceFileBase_, "STDOUT") == 0)
                || strcmp(traceFileBase_, "STDERR") == 0)
            {
                strcpy(fname, traceFileBase_);
            }
            else 
            {   // Make user specified file name unique per node
                sprintf( fname,"%s/%s.%d.%s"
                       , getenv("MPI_TMPDIR")
                       , traceFileBase_
                       , getpid()
                       , hostname);
            }
        }
        else
        {   // No user specified trace file name, use default
            if( getenv("SQ_VIRTUAL_NODES") )
            {
                sprintf( fname,"%s/trafconfig.trace.%d.%s"
                       , getenv("MPI_TMPDIR")
                       , getpid()
                       , hostname);
            }
            else
            {
                sprintf( fname,"%s/trafconfig.trace.%s"
                       , getenv("MPI_TMPDIR")
                       , hostname);
            }
        }

        // Tracing was disabled and is now enabled, initialize trace
        trace_init(fname,
                   false,  // don't append pid to file name
                   hostname,  // prefix
                   false);
    }
    if (trace_was_enabled && !tracingEnabled_)
    {
        // Tracing was enabled and now is disabled, flush trace.  Save
        // current trace settings.
        trace_flush();

        TcTraceSettings = 0;
    }

    // If a new trace file buffer size was specified, set it
    if ((traceFileFb_ > 0) && (old_fb != traceFileFb_))
        trace_set_mem(traceFileFb_);
}
