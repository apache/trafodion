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
#include "pnodeconfig.h"
#include "lnodeconfig.h"


///////////////////////////////////////////////////////////////////////////////
//  Logical Node Configuration
///////////////////////////////////////////////////////////////////////////////

CLNodeConfig::CLNodeConfig( CPNodeConfig *pnodeConfig
                          , int           nid
                          , cpu_set_t    &coreMask
                          , int           processors
                          , ZoneType      zoneType
                          )
            : nid_(nid)
            , coreMask_(coreMask)
            , processors_(processors)
            , zoneType_(zoneType)
            , pnodeConfig_(pnodeConfig)
            , next_(NULL)
            , prev_(NULL)
            , nextP_(NULL)
            , prevP_(NULL)
{
    const char method_name[] = "CLNodeConfig::CLNodeConfig";
    TRACE_ENTRY;

    pnodeConfig_->AddLNodeConfigP( this );

    TRACE_EXIT;
}

CLNodeConfig::~CLNodeConfig( void )
{
    const char method_name[] = "CLNodeConfig::~CLNodeConfig";
    TRACE_ENTRY;

    pnodeConfig_->RemoveLNodeConfigP( this );

    TRACE_EXIT;
}

int  CLNodeConfig::GetPNid( void ) 
{
    return ( pnodeConfig_->GetPNid( ) );
}

CLNodeConfigContainer::CLNodeConfigContainer( void )
                     : lnodesCount_(0)
                     , lnodeConfigSize_(0)
                     , lnodeConfig_(NULL)
                     , head_(NULL)
                     , tail_(NULL)
{
    const char method_name[] = "CLNodeConfigContainer::CLNodeConfigContainer";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CLNodeConfigContainer::CLNodeConfigContainer( int lnodesSize )
                     : lnodesCount_(0)
                     , lnodeConfigSize_(lnodesSize)
                     , lnodeConfig_(NULL)
                     , head_(NULL)
                     , tail_(NULL)
{
    const char method_name[] = "CLNodeConfigContainer::CLNodeConfigContainer";
    TRACE_ENTRY;

    lnodeConfig_ = new CLNodeConfig *[lnodeConfigSize_];

    if ( ! lnodeConfig_ )
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Can't allocate logical node configuration array - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LNODECONF_CONSTR_1, SQ_LOG_CRIT, la_buf);
    }

    TRACE_EXIT;
}

CLNodeConfigContainer::~CLNodeConfigContainer(void)
{
    CLNodeConfig *lnodeConfig = head_;

    const char method_name[] = "CLNodeConfigContainer::~CLNodeConfigContainer";
    TRACE_ENTRY;

    // Only the main container builds the array of 
    // logical node configuration objects. 
    // The logical nodes container in a physical node configuration object
    // only stores the configured logical nodes it hosts.
    if ( lnodeConfig_ )
    {
        while ( head_ )
        {
            DeleteLNodeConfig( lnodeConfig );
            lnodeConfig = head_;
        }
    
        delete [] lnodeConfig_;
    }

    TRACE_EXIT;
}

CLNodeConfig *CLNodeConfigContainer::AddLNodeConfigP( CLNodeConfig *lnodeConfig )
{
    const char method_name[] = "CLNodeConfigContainer::AddLNodeConfig";
    TRACE_ENTRY;

    assert( lnodeConfig != NULL );
    
    if ( lnodeConfig )
    {
        lnodesCount_++;
        // Add it to the container list
        if ( head_ == NULL )
        {
            head_ = tail_ = lnodeConfig;
        }
        else
        {
            //tail_ = tail_->LinkP( entry );
            tail_->nextP_ = lnodeConfig;
            lnodeConfig->prevP_ = tail_;
            tail_ = lnodeConfig;
        }
    }

    TRACE_EXIT;
    return( lnodeConfig );
}

CLNodeConfig *CLNodeConfigContainer::AddLNodeConfig( CPNodeConfig *pnodeConfig
                                                   , int           nid
                                                   , cpu_set_t    &coreMask 
                                                   , int           processors
                                                   , ZoneType      zoneType
                                                   )
{
    const char method_name[] = "CLNodeConfigContainer::AddLNodeConfig";
    TRACE_ENTRY;

    // Assume nid list is sequential from zero
    if ( ! (nid >= 0 && nid <= (lnodesCount_ + 1)) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Invalid nid=%d - should be >=0 and <=%d)\n", method_name, nid, (lnodesCount_ + 1));
        mon_log_write(MON_LNODECONF_ADD_LNODE_1, SQ_LOG_CRIT, la_buf);
        return( NULL );
    }

    CLNodeConfig *lnodeConfig = new CLNodeConfig( pnodeConfig
                                                , nid
                                                , coreMask 
                                                , processors
                                                , zoneType );
    if (lnodeConfig)
    {
        lnodesCount_++;
        // Add it to the array
        if ( lnodeConfig_ && nid < lnodeConfigSize_ )
        {
            lnodeConfig_[nid] = lnodeConfig;
        }
        // Add it to the container list
        if ( head_ == NULL )
        {
            head_ = tail_ = lnodeConfig;
        }
        else
        {
            //tail_ = tail_->Link( lnodeConfig );
            tail_->next_ = lnodeConfig;
            lnodeConfig->prev_ = tail_;
            tail_ = lnodeConfig;
        }
    }
    else
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Can't allocate logical node configuration object - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_LNODECONF_ADD_LNODE_2, SQ_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
    return lnodeConfig;
}

void CLNodeConfigContainer::DeleteLNodeConfig( CLNodeConfig *lnodeConfig )
{
    
    if ( head_ == lnodeConfig )
        head_ = lnodeConfig->next_;
    if ( tail_ == lnodeConfig )
        tail_ = lnodeConfig->prev_;
    if ( lnodeConfig->prev_ )
        lnodeConfig->prev_->next_ = lnodeConfig->next_;
    if ( lnodeConfig->next_ )
        lnodeConfig->next_->prev_ = lnodeConfig->prev_;
    delete lnodeConfig;
}

CLNodeConfig *CLNodeConfigContainer::GetLNodeConfig( int nid )
{
    CLNodeConfig *config;

    const char method_name[] = "CLNodeConfigContainer::GetLNodeConfig";
    TRACE_ENTRY;

    if ( nid >= 0 && nid < lnodesCount_ )
    {
        config = lnodeConfig_[nid];
    }
    else
    {
        config = NULL;
    }

    TRACE_EXIT;
    return config;
}

void CLNodeConfigContainer::RemoveLNodeConfigP( CLNodeConfig *lnodeConfig )
{
    
    if ( head_ == lnodeConfig )
        head_ = lnodeConfig->nextP_;
    if ( tail_ == lnodeConfig )
        tail_ = lnodeConfig->prevP_;
    if ( lnodeConfig->prevP_ )
        lnodeConfig->prevP_->nextP_ = lnodeConfig->nextP_;
    if ( lnodeConfig->nextP_ )
        lnodeConfig->nextP_->prevP_ = lnodeConfig->prevP_;
}
