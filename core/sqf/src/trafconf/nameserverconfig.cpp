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
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <iostream>
#include "tclog.h"
#include "tctrace.h"
#include "nameserverconfig.h"
#include "pnodeconfig.h"


///////////////////////////////////////////////////////////////////////////////
//  Name Server Configuration
///////////////////////////////////////////////////////////////////////////////

CNameServerConfig::CNameServerConfig( const char *nodeName )
            : next_(NULL)
            , prev_(NULL)
{
    const char method_name[] = "CNameServerConfig::CNameServerConfig";
    TRACE_ENTRY;

    nodeName_ = new char[strlen(nodeName)+1];
    strcpy(nodeName_, nodeName);

    TRACE_EXIT;
}

CNameServerConfig::~CNameServerConfig( void )
{
    const char method_name[] = "CNameServerConfig::~CNameServerConfig";
    TRACE_ENTRY;

    delete [] nodeName_;

    TRACE_EXIT;
}

const char *CNameServerConfig::GetName( void )
{
    return( nodeName_ );
}

CNameServerConfigContainer::CNameServerConfigContainer( void )
                     : count_(0)
                     , configMax_(100)
                     , config_(NULL)
                     , head_(NULL)
                     , tail_(NULL)
{
    const char method_name[] = "CNameServerConfigContainer::CNameServerConfigContainer";
    TRACE_ENTRY;

    config_ = new CNameServerConfig *[configMax_];

    if ( ! config_ )
    {
        int err = errno;
        char la_buf[TC_LOG_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Can't allocate nameserver configuration array - errno=%d (%s)\n", method_name, err, strerror(errno));
        TcLogWrite(MON_LNODECONF_CONSTR_1, TC_LOG_CRIT, la_buf); // TODO
    }
    else
    {
        // Initialize array
        for ( int i = 0; i < configMax_ ;i++ )
        {
            config_[i] = NULL;
        }
    }

    TRACE_EXIT;
}

CNameServerConfigContainer::~CNameServerConfigContainer(void)
{
    CNameServerConfig *Config = head_;

    const char method_name[] = "CNameServerConfigContainer::~CNameServerConfigContainer";
    TRACE_ENTRY;

    // Only the main container builds the array of 
    // nameserver configuration objects. 
    // The nameserver container in a nameserver configuration object
    // only stores the configured nameserver it hosts.
    if ( config_ )
    { // This is the main container
        // Delete entries
        while ( head_ )
        {
            RemoveConfig( Config );
            Config = head_;
        }
    
        // Delete array
        delete [] config_;
    }

    TRACE_EXIT;
}

void CNameServerConfigContainer::Clear( void )
{
    const char method_name[] = "CNameServerConfigContainer::Clear";
    TRACE_ENTRY;

    if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Clearing nameservers\n"
                     , method_name, __LINE__);
    }
    
    CNameServerConfig *Config = head_;

    // Only the main container builds the array of 
    // nameserver configuration objects. 
    // The nameserver container in a nameserver configuration object
    // only stores the configured nameserver it hosts.
    if ( config_ )
    {
        while ( head_ )
        {
            RemoveConfig( Config );
            Config = head_;
        }
    
        // Initialize array
        for ( int i = 0; i < configMax_; i++ )
        {
            config_[i] = NULL;
        }
    }

    count_ = 0;
    head_ = NULL;
    tail_ = NULL;

    TRACE_EXIT;
}

void CNameServerConfigContainer::AddConfig( const char *nodeName )
{
    const char method_name[] = "CNameServerConfigContainer::AddConfig";
    TRACE_ENTRY;

    CNameServerConfig *config = new CNameServerConfig( nodeName );

    // Bump the nameserver count
    count_++;
    // Add it to the container list
    if ( head_ == NULL )
    {
        head_ = tail_ = config;
    }
    else
    {
        tail_->next_ = config;
        config->prev_ = tail_;
        tail_ = config;
    }

    if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d - Added nameserver configuration\n"
                      "        (name=%s)\n"
                      "        (count_=%d,ConfigMax=%d)\n"
                    , method_name, __LINE__
                    , config->GetName()
                    , count_, configMax_);
    }

    TRACE_EXIT;
}

bool CNameServerConfigContainer::DeleteConfig( CNameServerConfig *config )
{
    const char method_name[] = "CNameServerConfigContainer::DeleteConfig";
    TRACE_ENTRY;

    bool rs = true;
    int rc;

    if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Deleting nameserver configuration nodeName=%s\n"
                     , method_name, __LINE__
                     , config->GetName());
    }
    
    rc = tc_delete_nameserver( config->GetName() );
    if ( rc == 0 )
    {
        RemoveConfig( config );
    }
    else
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] NameServer delete failed, node=%s\n",
                  method_name,  config->GetName() );
        TcLogWrite( MON_CLUSTERCONF_DELETENODE_1, TC_LOG_ERR, buf ); // TODO
        rs = false;
    }

    TRACE_EXIT;
    return( rs );
}

CNameServerConfig *CNameServerConfigContainer::GetConfig( const char *nodeName )
{
    const char method_name[] = "CNameServerConfigContainer::GetConfig";
    TRACE_ENTRY;

    CNameServerConfig *config = head_;
    while (config)
    {
        if ( CPNodeConfigContainer::hostnamecmp( config->GetName(), nodeName ) == 0 )
        {
            break;
        }
        config = config->GetNext();
    }

    TRACE_EXIT;
    return config;
}

bool CNameServerConfigContainer::LoadConfig( void )
{
    const char method_name[] = "CNameServerConfigContainer::LoadConfig";
    TRACE_ENTRY;

    int rc;
    int count = 0;
    char *nameservers[TC_NODES_MAX];

    rc = tc_get_nameservers( &count
                           , TC_NODES_MAX
                           , nameservers );
    if ( rc )
    {
        char la_buf[TC_LOG_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] Nameserver configuration access failed!\n"
                , method_name );
        TcLogWrite(MON_CLUSTERCONF_LOADNODE_1, TC_LOG_CRIT, la_buf); // TODO
        return( false );
    }

    // Process nameservers
    for (int i = 0; i < count; i++ )
    {
        AddConfig( nameservers[i] );
        delete [] nameservers[i];
    }

    if ( TcTraceSettings & TC_TRACE_INIT )
    {
        trace_printf("%s@%d - Successfully nameserver configuration\n", method_name, __LINE__);
    }

    TRACE_EXIT;
    return( true );
}

void CNameServerConfigContainer::RemoveConfig( CNameServerConfig *config )
{
    const char method_name[] = "CNameServerConfigContainer::RemoveConfig";
    TRACE_ENTRY;

    if ( head_ == config )
        head_ = config->next_;
    if ( tail_ == config )
        tail_ = config->prev_;
    if ( config->prev_ )
        config->prev_->next_ = config->next_;
    if ( config->next_ )
        config->next_->prev_ = config->prev_;
    delete config;

    // Decrement the nameserver count
    count_--;

    TRACE_EXIT;
}

bool CNameServerConfigContainer::SaveConfig( const char *nodeName )
{
    const char method_name[] = "CNameServerConfigContainer::SaveConfig";
    TRACE_ENTRY;

    bool rs = true;
    int  rc;

    if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Saving NameServer config (nodeName=%s)\n"
                     , method_name, __LINE__
                     , nodeName);
    }
    
    // Insert data
    rc = tc_put_nameserver( nodeName );
    if ( rc == 0 )
    {
        AddConfig( nodeName );
    }
    else
    {
        rs = false;
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] Nameserver add failed, nodeName=%s\n",
                  method_name,  nodeName );
        TcLogWrite( MON_CLUSTERCONF_SAVENODE_1, TC_LOG_ERR, buf ); // TODO
    }

    TRACE_EXIT;
    return( rs );
}

