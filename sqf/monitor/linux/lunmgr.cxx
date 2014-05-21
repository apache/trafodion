///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/trace.h"
#include "montrace.h"
#include "monlogging.h"
#include "lunmgr.h"

typedef list<string>    StringList_t;

// Class inplementation

CLunMgr::CLunMgr( const char *utility )
        :CSystem( utility )
        ,outputList_( )
{
    const char method_name[] = "CLunMgr::CLunMgr";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CLunMgr::~CLunMgr( void )
{
    const char method_name[] = "CLunMgr::~CLunMgr";
    TRACE_ENTRY;

    TRACE_EXIT;
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CLunMgr::LaunchCommand
//
// Description: Executes the command string passed in asynchronously
//              
// Return:
//        0 - success
//        n - failure
//
///////////////////////////////////////////////////////////////////////////////
int CLunMgr::LaunchCommand( const char *command, int &pid )
{
    const char method_name[] = "CLunMgr::ExecuteCommand";
    TRACE_ENTRY;

    int rc;

    rc = CSystem::LaunchCommand( command, pid );
    if ( rc != 0 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s] Error: While executing '%s', lunmgr error=%d(%s)\n"
                      , method_name, command_.data()
                      , rc, LunMgrErrorString(rc));
        mon_log_write(MON_LUNMGR_LAUNCHCOMMAND_1, SQ_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CLunMgr::WaitCommand
//
// Description: Waits for the command executed asynchronously to complete
//              
// Return:
//        0 - success
//        n - failure
//
///////////////////////////////////////////////////////////////////////////////
int CLunMgr::WaitCommand( int pid )
{
    const char method_name[] = "CLunMgr::WaitCommand";
    TRACE_ENTRY;

    int rc;

    rc = CSystem::WaitCommand( outputList_, pid );
    if ( rc != 0 )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s] Error: While executing '%s', lunmgr error=%d(%s)\n"
                      , method_name, command_.data()
                      , rc, LunMgrErrorString(rc));
        mon_log_write(MON_LUNMGR_WAITCOMMAND_1, SQ_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
    return( rc );
}

///////////////////////////////////////////////////////////////////////////////
//
// Function/Method: CLunMgr::LunMgrErrorString
//
// Description: Maps the LunMgr completion state to an error string
//              
// Return:
//        error string
//
///////////////////////////////////////////////////////////////////////////////
const char *CLunMgr::LunMgrErrorString( int status)
{
    const char *str;
    
    switch( status )
    {
        case LUNMGR_STATE_UNMOUNT_ERROR:
            str = "Unmount failed";
            break;
        case LUNMGR_STATE_UNMOUNT_ERROR_DEVICE_BUSY:
            str = "Device busy";
            break;
        case LUNMGR_STATE_UNMOUNT_ERROR_DEVICE_NOT_MOUNTED:
            str = "Device not mounted";
            break;
        case LUNMGR_STATE_UNMOUNT_ERROR_DEVICE_NOT_FOUND:
            str = "Device not found";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_BAD_INVOKE:
            str = "Bad mount invocation";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_SYSERR:
            str = "System error on mount";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_INTBUG:
            str = "Internal mount bug or missing NFS support in mount";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_INTR:
            str = "User interrupt on mount";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_MTAB_ERR:
            str = "mtab error on mount";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_MOUNT_FAILED:
            str = "Mount failed";
            break;
        case LUNMGR_STATE_MOUNT_ERROR_SOME_SUCCEEDED:
            str = "Some mount succeeded";
            break;
        default:
            str = "Unknown lunmgr error";
            break;
    }

    return( str );
}

