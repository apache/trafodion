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

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <mpi.h>

#include "msgdef.h"
#include "montrace.h"

extern int MyPNID;
extern char Node_name[MPI_MAX_PROCESSOR_NAME];

const char *CMonTrace::str_trace_enable  = "MON_TRACE_ENABLE";
const char *CMonTrace::str_trace_file    = "MON_TRACE_FILE";
const char *CMonTrace::str_trace_file_fb = "MON_TRACE_FILE_FB";

const CMonTrace::traceArea CMonTrace::traceAreaList[] =
{ {"MON_TRACE_REQUEST",        TRACE_REQUEST},
  {"MON_TRACE_REQUEST_DETAIL", TRACE_REQUEST_DETAIL},
  {"MON_TRACE_PROCESS",        TRACE_PROCESS},
  {"MON_TRACE_PROCESS_DETAIL", TRACE_PROCESS_DETAIL},
  {"MON_TRACE_SYNC",           TRACE_SYNC},
  {"MON_TRACE_SYNC_DETAIL",    TRACE_SYNC_DETAIL},
  {"MON_TRACE_NOTICE",         TRACE_NOTICE},
  {"MON_TRACE_NOTICE_DETAIL",  TRACE_NOTICE_DETAIL},
  {"MON_TRACE_MLIO",           TRACE_MLIO},
  {"MON_TRACE_MLIO_DETAIL",    TRACE_MLIO_DETAIL},
  {"MON_TRACE_TMSYNC",         TRACE_TMSYNC},
  {"MON_TRACE_INIT",           TRACE_INIT},
  {"MON_TRACE_RECOVERY",       TRACE_RECOVERY},
  {"MON_TRACE_STATS",          TRACE_STATS},
  {"MON_TRACE_EVLOG_MSG",      TRACE_EVLOG_MSG},
  {"MON_TRACE_ENTRY_EXIT",     TRACE_ENTRY_EXIT},
  {"MON_TRACE_REDIRECTION",    TRACE_REDIRECTION},
  {"MON_TRACE_TRAFCONFIG",     TRACE_TRAFCONFIG},
  {"MON_TRACE_HEALTH",         TRACE_HEALTH},
  {"MON_TRACE_SIG_HANDLER",    TRACE_SIG_HANDLER}
};

// Global trace flags
long trace_settings = 0;

CMonTrace *MonTrace = new CMonTrace ();

CMonTrace::CMonTrace() : tracingEnabled(false), trace_settings_saved(0), trace_file_fb(0), trace_file_base(NULL)
{
    numTraceAreas = sizeof(traceAreaList)/sizeof(traceArea);
}

const char *CMonTrace::getenv_str(const char *key)
{
    const char *value;
    value = getenv(key);
    return value;
}

void CMonTrace::getenv_int(const char *key, int &val)
{
    const char *p = getenv_str(key);
    if (p != NULL)
    {
        val = atoi(p);
    }
}

bool CMonTrace::getenv_bool(const char *key) {
    const char *p = getenv_str(key);
    bool val = false;
    if (p != NULL)
    {
        val = atoi(p);
    }
    return val;
}

void CMonTrace::mon_trace_init(const char * traceLevel, const char *pfname)
{
    char trace_file_name[MAX_PROCESS_PATH];

    // Format default trace file name and remove any existing trace file.
    if( getenv("SQ_VIRTUAL_NODES") )
    {
        sprintf(trace_file_name,"%s/monitor.trace.%d.%s",
                getenv("MPI_TMPDIR"),MyPNID,Node_name);
    }
    else
    {
        sprintf(trace_file_name,"%s/monitor.trace.%s",
                getenv("MPI_TMPDIR"), Node_name);
    }
    remove(trace_file_name);

    if (pfname == NULL)
    {   // Caller did not specify a trace file name, get name from
        // environment variable if specified.
        pfname = getenv_str(str_trace_file);
    }

    if (pfname != NULL)
    {   // User specified trace file name

        // Save the base trace file name for possible use later
        trace_file_base = new char[strlen(pfname) + 1];
        strcpy(trace_file_base, pfname);

        if ((strcmp(pfname, "STDOUT") == 0)
          || strcmp(pfname, "STDERR") == 0)
        {
            strcpy(trace_file_name, pfname);
        }
        else // Make user specified file name unique per node
        {
            sprintf(trace_file_name,"%s/%s.%d.%s",getenv("MPI_TMPDIR"),
                    pfname,MyPNID,Node_name);
        }
    }

    // Get any trace settings that were specified via environment variables
    const char *value;

    for (int i=0; i<numTraceAreas; i++)
    {
        value = getenv(traceAreaList[i].id);
        if (value != NULL)
        {
            if (atoi(value) != 0)
                // set the enabled flag for this trace area
                trace_settings |= traceAreaList[i].bitFlag;
        }
    }

    // Get environment variable specifying whether tracing is enabled
    tracingEnabled = getenv_bool(str_trace_enable);
    // Get environment variable value for trace buffer size if specified
    getenv_int (str_trace_file_fb, trace_file_fb);

    // Convert the user specified trace level string to a number.  The
    // number can be specified as a decimal, octal or hexadecimal
    // constant.  Combine these flags with current trace_settings that
    // may have been set via environment variables.
    long trace_flags;
    trace_flags = strtol(traceLevel, NULL, 0);
    if (errno != ERANGE)
    {
        trace_settings |= trace_flags;
    }

    // If any trace settings were specified initialize the trace file
    if (trace_settings != 0 || tracingEnabled)
    {
        tracingEnabled = true;
        trace_settings_saved = trace_settings;

        trace_init(trace_file_name,
                   false,  // don't append pid to file name
                   Node_name,  // prefix
                   false);
        if (trace_file_fb > 0)
        {
            trace_set_mem(trace_file_fb);
        }
    }
}


void CMonTrace::mon_help_bool(const char *key, const char *value,
                              const char *key_cmp, bool &value_ret)
{
    if ((key != NULL) && (strcasecmp(key, key_cmp) == 0))
        value_ret = atoi(value);
}

void CMonTrace::mon_help_int(const char *key, const char *value,
                             const char *key_cmp, int &value_ret) {
    if ((key != NULL) && (strcasecmp(key, key_cmp) == 0))
        value_ret = atoi(value);
}

const char *CMonTrace::mon_help_str(const char *key, const char *value,
                                    const char *key_cmp)
{
    if ((key != NULL) && (strcasecmp(key, key_cmp) == 0))
        return value;
    return NULL;
}


void CMonTrace::mon_trace_change(const char *key, const char *value)
{
    bool trace_was_enabled = tracingEnabled;
    const char *pfname;
    int  old_fb = trace_file_fb;

    if (key == NULL)
        return;

    // Restore saved trace settings in case trace flags get modified
    trace_settings = trace_settings_saved;

    // Compare the key with each of the trace flag strings.  When
    // there is an equal compare, assign the value to the appropriate flag.
    for (int i=0; i<numTraceAreas; i++)
    {
        if (strcasecmp(key, traceAreaList[i].id) == 0)
        {
            if (atoi(value) != 0)
            {
                // set the enabled flag for this trace area
                trace_settings |= traceAreaList[i].bitFlag;
            }
            else // clear the enabled flag for this trace area
            {
                trace_settings &= ~traceAreaList[i].bitFlag;
            }
            break;
        }
    }
    // Save current trace settings
    trace_settings_saved = trace_settings;


    // Check if tracing is being enabled/disabled
    mon_help_bool(key, value, str_trace_enable, tracingEnabled);
    // Check if trace file buffer size is being specified
    mon_help_int (key, value, str_trace_file_fb, trace_file_fb);

    // Check if trace file base name is being specified
    pfname = mon_help_str(key, value, str_trace_file);
    if (pfname != NULL)
    {   // Save trace file base name
        delete [] trace_file_base;
        trace_file_base = new char[strlen(pfname) + 1];
        strcpy(trace_file_base, pfname);
    }

    if (!trace_was_enabled && tracingEnabled)
    {
        char fname[MAX_PROCESS_PATH];

        // Formulate trace file name
        if (trace_file_base != NULL)
        {  // User specified trace file name
            if ((strcmp(trace_file_base, "STDOUT") == 0)
                || strcmp(trace_file_base, "STDERR") == 0)
            {
                strcpy(fname, trace_file_base);
            }
            else 
            {   // Make user specified file name unique per node
                sprintf(fname,"%s/%s.%d.%s",getenv("MPI_TMPDIR"),
                        trace_file_base,MyPNID,Node_name);
            }
        }
        else
        {   // No user specified trace file name, use default
            if( getenv("SQ_VIRTUAL_NODES") )
            {
                sprintf(fname,"%s/monitor.trace.%d.%s",getenv("MPI_TMPDIR"), MyPNID,
                        Node_name);
            }
            else
            {
                sprintf(fname,"%s/monitor.trace.%s",getenv("MPI_TMPDIR"),
                        Node_name);
            }
        }

        // Tracing was disabled and is now enabled, initialize trace
        trace_init(fname,
                   false,  // don't append pid to file name
                   Node_name,  // prefix
                   false);
    }
    if (trace_was_enabled && !tracingEnabled)
    {
        // Tracing was enabled and now is disabled, flush trace.  Save
        // current trace settings.
        trace_flush();

        trace_settings = 0;
    }

    // If a new trace file buffer size was specified, set it
    if ((trace_file_fb > 0) && (old_fb != trace_file_fb))
        trace_set_mem(trace_file_fb);
}
