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

#include "seabed/trace.h"
#include "montrace.h"
#include "monlogging.h"
#include "pkillall.h"

// Class inplementation

CPKillAll::CPKillAll( const char *utility )
          :CSystem( utility )
          ,outputList_( )
{
    const char method_name[] = "CPKillAll::CPKillAll";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CPKillAll::~CPKillAll( void )
{
    const char method_name[] = "CPKillAll::~CPKillAll";
    TRACE_ENTRY;

    TRACE_EXIT;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CPKillAll::ExecuteCommand
//
// Description: Executes the command string passed in
//              
// Return:
//        0 - success
//        n - failure
//
///////////////////////////////////////////////////////////////////////////////
int CPKillAll::ExecuteCommand( const char *command )
{
    const char method_name[] = "CPKillAll::ExecuteCommand";
    TRACE_ENTRY;

    int rc;

    rc = CSystem::ExecuteCommand( command, outputList_ );
    if ( rc != 0 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf( la_buf, "[%s] Error: While executing '%s'\n"
                       , method_name, command_.data() );
        mon_log_write(MON_PKILLALL_EXECUTECOMMAND_1, SQ_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
    return( rc );
}
