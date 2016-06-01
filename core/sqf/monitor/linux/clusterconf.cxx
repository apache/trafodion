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

#include <errno.h>
#include <assert.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <iostream>
#include <mpi.h>
#include "msgdef.h"
#include "seabed/trace.h"
#include "montrace.h"
#include "monlogging.h"
#include "clusterconf.h"


///////////////////////////////////////////////////////////////////////////////
//  Tokenizer
///////////////////////////////////////////////////////////////////////////////

CTokenizer::CTokenizer( void )
          : confFile_(NULL)
          , cmdTail_(NULL)
          , line_(0)
{
    const char method_name[] = "CTokenizer::CTokenizer";
    TRACE_ENTRY;

    buffer_[0] = '\0';
    token_[0] = '\0';

    TRACE_EXIT;
}

CTokenizer::~CTokenizer( void )
{
    const char method_name[] = "CTokenizer::~CTokenizer";
    TRACE_ENTRY;

    TRACE_EXIT;
}

char *CTokenizer::FindDelimiter( char *str )
{
    char *ptr = str;
    const char method_name[] = "CTokenizer::FindDelimiter";
    TRACE_ENTRY;

    while (ptr && *ptr && *ptr != ' ' && *ptr != ',' && *ptr != '{' && *ptr != '}' && *ptr != '=' && *ptr != ':')
    {
        ptr++;
    }

    TRACE_EXIT;
    return ptr;
}

char *CTokenizer::FindEndOfToken( char *str, int maxlen )
{
    int length = 0;
    char *ptr = str;
    const char method_name[] = "CTokenizer::FindEndOfToken";
    TRACE_ENTRY;

    while (ptr && *ptr && *ptr != ' ' && *ptr != ',' && *ptr != '{' && *ptr != '}' && *ptr != '=' && *ptr != ':')
    {
        length++;
        ptr++;
        if (length == maxlen)
        {
            break;
        }
    }

    TRACE_EXIT;
    return ptr;
}

char *CTokenizer::GetToken(char *cmd, char *token, char *delimiter, int maxlen)
{
    char *ptr = RemoveWhiteSpace( cmd );
    char *end = FindEndOfToken( ptr, maxlen );
    const char method_name[] = "CTokenizer::GetToken";
    TRACE_ENTRY;

    *delimiter = *end;
    if ( *end )
    {
        *end = '\0';
        strcpy( token, ptr );
        *end = *delimiter;
        end = FindDelimiter( end );
        *delimiter = *end;
        ptr = RemoveWhiteSpace( end );
        if ( *ptr == '{' || *ptr == '}' || *ptr == '=' || *ptr == ':' )
        {
            *delimiter = *ptr;
            ptr++;
            ptr = RemoveWhiteSpace( ptr );
        }
    }
    else
    {
        strcpy( token, ptr );
        ptr = end;
    }

    TRACE_EXIT;
    return ptr;
}


char *CTokenizer::NormalizeCase( char *token )
{
    char *ptr = token;

    const char method_name[] = "CTokenizer::NormalizeCase";
    TRACE_ENTRY;

    while ( *ptr )
    {
        *ptr = tolower( *ptr );
        if ( *ptr == '\n' ) *ptr = '\0';
        ptr++;
    }

    TRACE_EXIT;
    return token;
}

bool CTokenizer::ReadLine( void )
{
    const char method_name[] = "CTokenizer::ReadLine";
    TRACE_ENTRY;
    cmdTail_ = fgets( buffer_, MPI_MAX_PROCESSOR_NAME, confFile_ );
    if ( cmdTail_ )
    {
        NormalizeCase( cmdTail_ );
        if ( strlen( cmdTail_ ) >= 1 )
        {
            memset( token_, 0, MAX_TOKEN );
            line_++;
        }
        else
        {
            TRACE_EXIT;
            return( false );
        }
    }
    else
    {
        TRACE_EXIT;
        return( false );
    }

    TRACE_EXIT;
    return( true );
}

char *CTokenizer::RemoveWhiteSpace( char *token )
{
    const char method_name[] = "CTokenizer::RemoveWhiteSpace";
    TRACE_ENTRY;

    char *ptr = token;

    while (ptr && *ptr && (*ptr == ' ' || *ptr == '\t' || *ptr == ','))
    {
        ptr++;
    }

    TRACE_EXIT;
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
//  Cluster Configuration
///////////////////////////////////////////////////////////////////////////////

CClusterConfig::CClusterConfig( void )
              : CLNodeConfigContainer(MAX_LNODES)
              , configReady_(false)
              , delimiter_('\0')
              , excludedNid_(false)
              , excludedProcessor_(false)
              , excludedCores_(false)
              , gatherSpares_(true)
              , newPNodeConfig_(true)
              , newLNodeConfig_(false)
              , spareNode_(false)
              , currProcessor_(0)
              , prevProcessor_(0)
              , currNid_(0)
              , currPNid_(0)
              , prevNid_(0)
              , prevPNid_(0)
              , spareIndex_(0)
              , currZoneType_(ZoneType_Undefined)
              , prevZoneType_(ZoneType_Undefined)
              , currPNodeConfig_(NULL)
              , prevPNodeConfig_(NULL)
              , lnodeConfig_(NULL)
{
    const char method_name[] = "CClusterConfig::CClusterConfig";
    TRACE_ENTRY;

    nodename_[0] = '\0';

    memset( sparePNid_, 0, sizeof(sparePNid_) );

    CPU_ZERO( &currCoreMask_ );
    CPU_ZERO( &excludedCoreMask_ );
    CPU_ZERO( &prevCoreMask_ );
    CPU_ZERO( &prevExcludedCoreMask_ );

    TRACE_EXIT;
}

CClusterConfig::~CClusterConfig ( void )
{
    const char method_name[] = "CClusterConfig::~CClusterConfig";
    TRACE_ENTRY;

    if ( confFile_ )
    {
        fclose( confFile_ );
    }

    TRACE_EXIT;
}

bool CClusterConfig::Initialize( void )
{
    char buffer[MPI_MAX_PROCESSOR_NAME+MAX_ROLEBUF_SIZE];
    const char method_name[] = "CClusterConfig::Initialize";
    TRACE_ENTRY;

    if ( confFile_ )
    {
        // Already initialized
        return ( true );
    }

    char *env = getenv( "MPI_TMPDIR" );
    sprintf( buffer,"%s%scluster.conf",(env?env:""),(env?"/":"") );

    confFile_ = fopen( buffer, "r" );
    if ( confFile_ )
    {
        if (trace_settings & TRACE_INIT)
        {
            trace_printf("%s@%d - Opened %s " "\n", method_name, __LINE__, buffer);
        }
        TRACE_EXIT;
        return ( true );
    }
    else
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Can't open cluster configuration file (%s) - errno=%d (%s)\n", method_name, buffer, err, strerror(errno));
        mon_log_write(MON_CLUSTERCONF_INIT_1, SQ_LOG_CRIT, la_buf);
    }

    TRACE_EXIT;
    return ( false );
}

bool CClusterConfig::LoadConfig( void )
{
    const char method_name[] = "CClusterConfig::LoadConfig";
    TRACE_ENTRY;

    bool configLoaded = true;
    bool notDone = true;
    
    if ( confFile_ )
    {
        // Read 'cluster.conf' file and process one line at time
        do
        {
            if ( ReadLine() )
            {
                // Token: PNID
                if ( ! ParsePNid() )
                {
                    char la_buf[MON_STRING_BUF_SIZE];
                    sprintf(la_buf, "[%s], Error: Invalid 'cluster.conf' syntax, looking for pnid, line = %d\n", method_name, line_);
                    mon_log_write(MON_CLUSTERCONF_LOAD_1, SQ_LOG_CRIT, la_buf);
                    configLoaded = notDone = false;
                }
                // Token: NID
                if ( notDone && delimiter_ == ':' )
                {
                    if ( ! ParseNid() )
                    {
                        char la_buf[MON_STRING_BUF_SIZE];
                        sprintf(la_buf, "[%s], Error: Invalid 'cluster.conf' syntax, looking for nid, line = %d\n", method_name, line_);
                        mon_log_write(MON_CLUSTERCONF_LOAD_2, SQ_LOG_CRIT, la_buf);
                        configLoaded = notDone = false;
                    }
                }
                // Token: Nodename
                if ( notDone && delimiter_ == ':' )
                {
                    if ( ! ParseNodename() )
                    {
                        char la_buf[MON_STRING_BUF_SIZE];
                        sprintf(la_buf, "[%s], Error: Invalid 'cluster.conf' syntax, looking for nodename, line = %d\n", method_name, line_);
                        mon_log_write(MON_CLUSTERCONF_LOAD_3, SQ_LOG_CRIT, la_buf);
                        configLoaded = notDone = false;
                    }
                }
                // Token: Processor
                if ( notDone && delimiter_ == ':' )
                {
                    if ( ! ParseProcessor() )
                    {
                        char la_buf[MON_STRING_BUF_SIZE];
                        sprintf(la_buf, "[%s], Error: Invalid 'cluster.conf' syntax, looking for processor, line = %d\n", method_name, line_);
                        mon_log_write(MON_CLUSTERCONF_LOAD_4, SQ_LOG_CRIT, la_buf);
                        configLoaded = notDone = false;
                    }
                }
                // Token: Core
                if ( notDone && delimiter_ == ':' )
                {
                    if ( ! ParseCore() )
                    {
                        char la_buf[MON_STRING_BUF_SIZE];
                        sprintf(la_buf, "[%s], Error: Invalid 'cluster.conf' syntax, looking for core, line = %d\n", method_name, line_);
                        mon_log_write(MON_CLUSTERCONF_LOAD_5, SQ_LOG_CRIT, la_buf);
                        configLoaded = notDone = false;
                    }
                }
                // Token: Roles
                if ( notDone && delimiter_ == ':' )
                {
                    if ( ! ParseRoles() )
                    {
                        char la_buf[MON_STRING_BUF_SIZE];
                        sprintf(la_buf, "[%s], Error: Invalid 'cluster.conf' syntax, looking for role, line = %d\n", method_name, line_);
                        mon_log_write(MON_CLUSTERCONF_LOAD_6, SQ_LOG_CRIT, la_buf);
                        configLoaded = notDone = false;
                    }
                }
                else
                {
                    if ( notDone )
                    {
                        if ( ! spareNode_ && excludedNid_ && excludedProcessor_ )
                        {
                            excludedCores_ = true;
                            prevExcludedCoreMask_ = excludedCoreMask_;
                            CPU_ZERO( &excludedCoreMask_ );
                            excludedNid_ = excludedProcessor_ = false;
                        }
                        if ( currPNid_ != prevPNid_ )
                        {
                            prevPNodeConfig_ = currPNodeConfig_;
                            prevPNid_ = currPNid_;
                        }
                        if ( currNid_ != prevNid_ )
                        {
                            prevNid_ = currNid_;
                            prevCoreMask_ = currCoreMask_;
                            prevExcludedCoreMask_ = excludedCoreMask_;
                            prevProcessor_ = ++currProcessor_;
                            prevZoneType_ = currZoneType_;
                            newLNodeConfig_ = true;
                        }
                        char la_buf[MON_STRING_BUF_SIZE];
                        sprintf(la_buf, "[%s], Warning: can't parse line %d, skipping\n", method_name, line_);
                        mon_log_write(MON_CLUSTERCONF_LOAD_7, SQ_LOG_CRIT, la_buf);
                        //line_--;
                    }
                }
            }
            else
            {
                if ( ! spareNode_ && excludedNid_ && excludedProcessor_ )
                {
                    excludedCores_ = true;
                    prevExcludedCoreMask_ = excludedCoreMask_;
                    CPU_ZERO( &excludedCoreMask_ );
                    excludedNid_ = excludedProcessor_ = false;
                }
                if ( currPNid_ != prevPNid_ )
                {
                    prevPNodeConfig_ = currPNodeConfig_;
                    prevPNid_ = currPNid_;
                }
                if ( currNid_ != -1 )
                {
                    prevNid_ = currNid_;
                    prevCoreMask_ = currCoreMask_;
                    prevExcludedCoreMask_ = excludedCoreMask_;
                    prevProcessor_ = ++currProcessor_;
                    prevZoneType_ = currZoneType_;
                    newLNodeConfig_ = true;
                }
                notDone = false;
            }

            if ( newPNodeConfig_ )
            {
                // Physical node object is created at first line detected
                currPNodeConfig_ = AddPNodeConfig( currPNid_, nodename_, spareNode_ );
                if ( spareNode_ )
                {
                    currPNodeConfig_->SetSpareList( sparePNid_, spareIndex_ );
                    gatherSpares_ = true;
                    spareNode_ = excludedNid_ = excludedProcessor_ = false;
                }
                newPNodeConfig_ = false;
            }
            if ( newLNodeConfig_ )
            {
                prevPNodeConfig_ = GetPNodeConfig( prevPNid_ );
                // Logical node object is created when all lines of a given
                // nid are read (meaning when nid read changes or eof)
                lnodeConfig_ = AddLNodeConfig( prevPNodeConfig_
                                             , prevNid_
                                             , prevCoreMask_
                                             , prevProcessor_
                                             , prevZoneType_ );
                // assume that new PNodeConfig was added so next LNodeconfig
                // will have correct pnid
                prevPNid_ = currPNid_;
                newLNodeConfig_ = false;
            }
            if ( excludedCores_ )
            {
                prevPNodeConfig_ = GetPNodeConfig( prevPNid_ );
                prevPNodeConfig_->SetExcludedCoreMask( prevExcludedCoreMask_ );
                prevPNid_ = currPNid_;
                excludedCores_ = false;
            }
        } while( notDone );

        configReady_ = configLoaded ? true : false;
    }
    else
    {
        return( configLoaded );
    }

    if ( trace_settings & TRACE_INIT )
    {
        if ( configLoaded )
            trace_printf("%s@%d - Successfully loaded 'cluster.conf'" "\n", method_name, __LINE__);
        else
            trace_printf("%s@%d - Failed to load 'cluster.conf'" "\n", method_name, __LINE__);
    }
 
    TRACE_EXIT;
    return( configLoaded );
}

bool CClusterConfig::ParseCore( void )
{
    const char method_name[] = "CClusterConfig::ParseCore";
    TRACE_ENTRY;

    cmdTail_ = GetToken( cmdTail_, token_, &delimiter_ );
    if ( strlen( token_ ) >= 1 )
    {
        if ( isdigit( *token_ ) )
        {
            int currCore = atoi( token_ );
            if ( excludedNid_ && excludedProcessor_ )
            {
                CPU_SET( currCore, &excludedCoreMask_ );
            }
            else
            {
                CPU_SET( currCore, &currCoreMask_ );
            }
        }
        else
        {
            return( false );
        }
    }
    else
    {
        return( false );
    }

    TRACE_EXIT;
    return( true );
}

bool CClusterConfig::ParseNid( void )
{
    int  nid;

    const char method_name[] = "CClusterConfig::ParseNid";
    TRACE_ENTRY;

    cmdTail_ = GetToken( cmdTail_, token_, &delimiter_ );
    if ( strlen( token_ ) >= 1 )
    {
        if ( isdigit( *token_ ) )
        {
            nid = atoi( token_ );
            if ( ( nid >= 0 && nid <= (currNid_ + 1)) || nid == -1 )
            {
                // If all lines of the current nid have been read
                if ( currNid_ != nid )
                {
                    if ( excludedNid_ && excludedProcessor_ )
                    {
                        excludedCores_ = true;
                        prevExcludedCoreMask_ = excludedCoreMask_;
                        CPU_ZERO( &excludedCoreMask_ );
                        excludedNid_ = excludedProcessor_ = false;
                    }
                    //if ( nid != -1 && currNid_ != prevNid_ )
                    if ( nid != -1 )
                    {
                        prevNid_ = currNid_;
                        prevCoreMask_ = currCoreMask_;
                        prevProcessor_ = ++currProcessor_;
                        prevZoneType_ = currZoneType_;
                        newLNodeConfig_ = true;
                        CPU_ZERO( &currCoreMask_ );
                        currProcessor_ = 0;
                        currZoneType_ = ZoneType_Undefined;
                    }
                    currNid_ = nid != -1 ? nid : currNid_;
                }
            }
            else
            {
                return( false );
            }
        }
        else
        {
            int number = 0;
            number = atoi( token_ );
            if ( number == -1 )
            {
                excludedNid_ = true;
                if ( currNid_ != -1 && currNid_ != prevNid_ )
                {
                    prevNid_ = currNid_;
                    prevCoreMask_ = currCoreMask_;
                    prevProcessor_ = ++currProcessor_;
                    prevZoneType_ = currZoneType_;
                    newLNodeConfig_ = true;
                    CPU_ZERO( &currCoreMask_ );
                    currProcessor_ = 0;
                    currZoneType_ = ZoneType_Undefined;
                }
                currNid_ =  -1;
            }
            else
            {
                return( false );
            }
        }
    }
    else
    {
        return( false );
    }

    TRACE_EXIT;
    return( true );
}

bool CClusterConfig::ParseNodename( void )
{
    const char method_name[] = "CClusterConfig::ParseNodename";
    TRACE_ENTRY;

    cmdTail_ = GetToken( cmdTail_, token_, &delimiter_ );
    if ( strlen( token_ ) > 1 )
    {
        strcpy( nodename_,token_ );
    }
    else
    {
        return( false );
    }
    
    TRACE_EXIT;
    return( true );
}

bool CClusterConfig::ParsePNid( void )
{
    int  pnid;
    const char method_name[] = "CClusterConfig::ParsePNid";
    TRACE_ENTRY;

    // Token: PNID
    cmdTail_ = GetToken( cmdTail_, token_, &delimiter_ );
    if ( strlen( token_ ) >= 1 )
    {
        if ( isdigit( *token_ ) )
        {
            pnid = atoi( token_ );
            if ( currPNid_ != pnid )
            {
                if ( ! spareNode_ && excludedNid_ && excludedProcessor_ )
                {
                    excludedCores_ = true;
                    prevExcludedCoreMask_ = excludedCoreMask_;
                    CPU_ZERO( &excludedCoreMask_ );
                    excludedNid_ = excludedProcessor_ = false;
                }
                prevPNid_ = currPNid_;
                currPNid_ = pnid;
                gatherSpares_ = newPNodeConfig_ = true;
            }
        }
        else
        {
            return( false );
        }
    }
    else
    {
        return( false );
    }

    TRACE_EXIT;
    return( true );
}
                    
bool CClusterConfig::ParseProcessor( void )
{
    const char method_name[] = "CClusterConfig::ParseProcessor";
    TRACE_ENTRY;

    cmdTail_ = GetToken( cmdTail_, token_, &delimiter_ );
    if ( strlen( token_ ) >= 1 )
    {
        if ( isdigit( *token_ ) )
        {
            currProcessor_ = atoi( token_ );
        }
        else
        {
            int number = 0;
            number = atoi( token_ );
            if ( number == -1 && excludedNid_ )
            {
                excludedProcessor_ = true;
            }
            else
            {
                return( false );
            }
        }
    }
    else
    {
        return( false );
    }

    TRACE_EXIT;
    return( true );
}

bool CClusterConfig::ParseRoles( void )
{
    const char method_name[] = "CClusterConfig::ParseRoles";
    TRACE_ENTRY;

    spareIndex_ = 0;
    do
    {
        cmdTail_ = GetToken( cmdTail_, token_, &delimiter_ );
        if ( strlen( token_ ) >=1 )
        {
            if ( isdigit( *token_ ) )
            {
                if ( excludedNid_ && excludedProcessor_ )
                {
                    spareNode_ = true;
                    if ( gatherSpares_ && newPNodeConfig_ )
                    {
                        sparePNid_[spareIndex_] = atoi( token_ );
                        spareIndex_++;
                    }
                }
                else
                {
                    return( false );
                }
            }
            else if ( strcmp( token_, "aggregation" )==0 )
            {
                currZoneType_ = (ZoneType)(currZoneType_ | ZoneType_Aggregation);
            }
            else if ( strcmp( token_, "storage" )==0 )
            {
                currZoneType_ = (ZoneType)(currZoneType_ | ZoneType_Storage);
            }
            else if ( strcmp( token_, "operation" )==0 )
            {
                currZoneType_ = (ZoneType)(currZoneType_ | ZoneType_Edge);
            }
            else if ( strcmp( token_, "maintenance" )==0 )
            {
                currZoneType_ = (ZoneType)(currZoneType_ | ZoneType_Edge);
            }
            else if ( strcmp( token_, "loader" )==0 )
            {
                currZoneType_ = (ZoneType)(currZoneType_ | ZoneType_Edge);
            }
            else if ( strcmp( token_, "connection" )==0 )
            {
                currZoneType_ = (ZoneType)(currZoneType_ | ZoneType_Edge);
            }
            else if ( strcmp( token_, "excluded" )==0 )
            {
                if ( excludedNid_ && excludedProcessor_ )
                {
                    // Ignore
                }
                else
                {
                    return( false );
                }
            }
            else
            {
                currZoneType_ = ZoneType_Any;
                break;
            }
        }
        else
        {
            return( false );
        }
    }
    while ( *cmdTail_ );

    if ( spareNode_ )
    {
        gatherSpares_ = false;
    }

    TRACE_EXIT;
    return( true );
}

