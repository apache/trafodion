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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "sqevlog/evl_sqlog_writer.h"
#include "clio.h"
#include "clusterconf.h"
#include "seabed/trace.h"
#include "montrace.h"
#include "SCMVersHelp.h"

#include "SCMVersHelp.h"

DEFINE_EXTERN_COMP_DOVERS(monwdt)
DEFINE_EXTERN_COMP_GETVERS2(monwdt)
DEFINE_EXTERN_COMP_PRINTVERS(monwdt)

const char *MyName = "monwdt";
int MyPNid = -1;
int MyNid = -1;
int gv_ms_su_nid = -1;          // Local IO nid to make compatible w/ Seabed
char ga_ms_su_c_port[MPI_MAX_PORT_NAME] = {0}; // connect
int VirtualNodes = 0;
long trace_settings = 0;

FILE *shell_locio_trace_file = NULL;

DEFINE_EXTERN_COMP_DOVERS(monwdt)
DEFINE_EXTERN_COMP_PRINTVERS(monwdt)

void DisplayUsage( void )
{
    fprintf (stderr, "\nUsage: monwdt { -disable } \n"
                     "   Where:\n"
                     "          -disable      - disable watchdog timer\n\n");
    exit( EXIT_FAILURE );
}

int mon_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string)
{
    
    pv_event_type = pv_event_type;
    pv_severity = pv_severity;
    int lv_err = 0;

    printf("%s", pp_string );

    return lv_err;
}

void LocIOTrace(const char *where, const char *format, va_list ap)
{
    if (shell_locio_trace_file != NULL)
    {
        int             ms;
        int             us;
        struct timeval  t;
        struct tm       tx;
        struct tm      *txp;
        char            buf[BUFSIZ];
        gettimeofday(&t, NULL);
        txp = localtime_r(&t.tv_sec, &tx);
        ms = (int) t.tv_usec / 1000;
        us = (int) t.tv_usec - ms * 1000;

        sprintf(buf, "%02d:%02d:%02d.%03d.%03d %s: (%lx)",
                tx.tm_hour, tx.tm_min, tx.tm_sec, ms, us, where,
                pthread_self());
        vsprintf(&buf[strlen(buf)], format, ap);
        fprintf(shell_locio_trace_file, buf);
        fflush(shell_locio_trace_file);
    }
}

void InitLocalIO( void )
{
    char *cmd_buffer;

    if ( MyPNid == -1 )
    {
        CClusterConfig  ClusterConfig; // 'cluster.conf' objects
        CPNodeConfig   *pnodeConfig;
        CLNodeConfig   *lnodeConfig;

        if ( ClusterConfig.Initialize() )
        {
            if ( ! ClusterConfig.LoadConfig() )
            {
                printf("[%s], Failed to load cluster configuration.\n", MyName);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            printf( "[%s] Warning: No cluster.conf found\n",MyName);
            if (MyNid == -1)
            {
                MyNid = 0;
            }
            exit(EXIT_FAILURE);
        }

        lnodeConfig = ClusterConfig.GetLNodeConfig( MyNid );
        pnodeConfig = lnodeConfig->GetPNodeConfig();
        gv_ms_su_nid = MyPNid = pnodeConfig->GetPNid();
    }

    gp_local_mon_io = new Local_IO_To_Monitor( -1 );
    cmd_buffer = getenv("SQ_MONWDT_TRACE");
    if (cmd_buffer && *cmd_buffer == '1')
    {
        gp_local_mon_io->cv_trace = true;
        
        char tracefile[MAX_SEARCH_PATH];
        char *tmpDir;
    
        tmpDir = getenv( "TRAF_LOG" );
        if (tmpDir)
        {
            sprintf( tracefile, "%s/monwdt.trace.%d", tmpDir, getpid() );
        }
        else
        {
            sprintf( tracefile, "./monwdt.trace.%d", getpid() );
        }
        
        shell_locio_trace_file = fopen(tracefile, "w+");
        gp_local_mon_io->cp_trace_cb = LocIOTrace;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: main()
//
// Description:     Disables watchdog timer logic in local monitor process
//
///////////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[])
{
    bool disableWatchdog = false;
    char *cmd_buffer;

    CALL_COMP_DOVERS(monwdt, argc, argv);
    CALL_COMP_PRINTVERS(monwdt)

    if ( argc == 1 || argc > 2 )
    {
        DisplayUsage();
    }

    // Get required runtime options
    for ( int argx = 1; argx < argc; argx++ )
    {
        if ( strcasecmp( argv [argx], "-disable" ) == 0 )
        {
            disableWatchdog = true;
        }
        else
        {
            DisplayUsage();
        }
    }
    
    // Check if we are using virtual nodes ... if so, set MyNid
    cmd_buffer = getenv("SQ_VIRTUAL_NODES");
    if (cmd_buffer && isdigit(cmd_buffer[0]))
    {
        VirtualNodes = atoi(cmd_buffer);
        if (VirtualNodes > 8) VirtualNodes = 8;
    }
    else
    {
        VirtualNodes = 0;
    }
    cmd_buffer = getenv("SQ_VIRTUAL_NID");
    if ( VirtualNodes && cmd_buffer )
    {
        MyNid = atoi(cmd_buffer);
    }
    else
    {
        MyNid = 0;
    }

    if ( ! gp_local_mon_io )
    {
        InitLocalIO();
        assert (gp_local_mon_io);
    }
    
    if ( gp_local_mon_io && disableWatchdog )
    {
        if ( gp_local_mon_io->init_comm() )
        {
            printf ("[%s] Disabling watchdog timer in monitor\n", MyName);
            gp_local_mon_io->disableWDT();
        }
        else
        {
            printf ("[%s] Environment is not up!\n", MyName);
        }
        delete gp_local_mon_io;
    }
    else
    {
        printf ("[%s] Can't disable watchdog time in monitor\n", MyName);
        exit( EXIT_FAILURE );
    }

    exit( EXIT_SUCCESS );
}
