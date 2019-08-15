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
#include "pnodeconfig.h"
#include "lnodeconfig.h"


///////////////////////////////////////////////////////////////////////////////
//  Logical Node Configuration
///////////////////////////////////////////////////////////////////////////////

CLNodeConfig::CLNodeConfig( CPNodeConfig *pnodeConfig
                          , lnodeConfigInfo_t &lnodeConfigInfo
                          )
            : nid_(lnodeConfigInfo.nid)
            , zid_(pnodeConfig->GetPNid())
            , coreMask_(lnodeConfigInfo.coreMask)
            , firstCore_(lnodeConfigInfo.firstCore)
            , lastCore_(lnodeConfigInfo.lastCore)
            , processors_(lnodeConfigInfo.processor)
            , zoneType_(lnodeConfigInfo.zoneType)
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

const char *CLNodeConfig::GetDomain( void )
{
    return( pnodeConfig_->GetDomain() );
}

const char *CLNodeConfig::GetFqdn( void )
{
    return( pnodeConfig_->GetFqdn() );
}

const char *CLNodeConfig::GetName( void )
{
    return( pnodeConfig_->GetName() );
}

int  CLNodeConfig::GetPNid( void ) 
{
    return( pnodeConfig_->GetPNid() );
}

CLNodeConfigContainer::CLNodeConfigContainer( void )
                     : lnodesCount_(0)
                     , nextNid_(-1)
                     , lnodesConfigMax_(0)
                     , lnodesConfig_(NULL)
                     , head_(NULL)
                     , tail_(NULL)
{
    const char method_name[] = "CLNodeConfigContainer::CLNodeConfigContainer";
    TRACE_ENTRY;

    TRACE_EXIT;
}

CLNodeConfigContainer::CLNodeConfigContainer( int lnodesConfigMax )
                     : lnodesCount_(0)
                     , nextNid_(0)
                     , lnodesConfigMax_(lnodesConfigMax)
                     , lnodesConfig_(NULL)
                     , head_(NULL)
                     , tail_(NULL)
{
    const char method_name[] = "CLNodeConfigContainer::CLNodeConfigContainer";
    TRACE_ENTRY;

    lnodesConfig_ = new CLNodeConfig *[lnodesConfigMax_];

    if ( ! lnodesConfig_ )
    {
        int err = errno;
        char la_buf[TC_LOG_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Can't allocate logical node configuration array - errno=%d (%s)\n", method_name, err, strerror(errno));
        TcLogWrite(MON_LNODECONF_CONSTR_1, TC_LOG_CRIT, la_buf);
    }
    else
    {
        // Initialize array
        for ( int i = 0; i < lnodesConfigMax_ ;i++ )
        {
            lnodesConfig_[i] = NULL;
        }
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
    if ( lnodesConfig_ )
    { // This is the main container
        // Delete entries
        while ( head_ )
        {
            DeleteLNodeConfig( lnodeConfig );
            lnodeConfig = head_;
        }
    
        // Delete array
        delete [] lnodesConfig_;
    }

    TRACE_EXIT;
}

void CLNodeConfigContainer::Clear( void )
{
    const char method_name[] = "CLNodeConfigContainer::Clear";
    TRACE_ENTRY;

    CLNodeConfig *lnodeConfig = head_;

    // Only the main container builds the array of 
    // logical node configuration objects. 
    // The logical nodes container in a physical node configuration object
    // only stores the configured logical nodes it hosts.
    if ( lnodesConfig_ )
    {
        while ( head_ )
        {
            DeleteLNodeConfig( lnodeConfig );
            lnodeConfig = head_;
        }
    
        // Initialize array
        for ( int i = 0; i < lnodesConfigMax_; i++ )
        {
            lnodesConfig_[i] = NULL;
        }
    }

    lnodesCount_ = 0;
    nextNid_ = 0;
    head_ = NULL;
    tail_ = NULL;

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
                                                   , lnodeConfigInfo_t &lnodeConfigInfo
                                                   )
{
    const char method_name[] = "CLNodeConfigContainer::AddLNodeConfig";
    TRACE_ENTRY;

    // nid list is NOT sequential from zero
    if ( ! (lnodeConfigInfo.nid >= 0 && lnodeConfigInfo.nid < lnodesConfigMax_) )
    {
        char la_buf[TC_LOG_BUF_SIZE];
        sprintf( la_buf, "[%s], Error: Invalid nid=%d - should be >= 0 and < %d)\n"
               , method_name, lnodeConfigInfo.nid, lnodesConfigMax_);
        TcLogWrite(MON_LNODECONF_ADD_LNODE_1, TC_LOG_CRIT, la_buf);
        return( NULL );
    }

    if( lnodesConfig_[lnodeConfigInfo.nid] != NULL )
    {
        if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
        {
            trace_printf( "%s@%d - Existing logical node configuration object\n"
                          "        (nid=%d, pnid=%d, nextNid_=%d)\n"
                          "        (lnodesCount_=%d,lnodesConfigMax=%d)\n"
                        , method_name, __LINE__
                        , lnodeConfigInfo.nid, pnodeConfig->GetPNid(), nextNid_
                        , lnodesCount_, lnodesConfigMax_);
        }
        TRACE_EXIT;
        return( lnodesConfig_[lnodeConfigInfo.nid] );
    }

    CLNodeConfig *lnodeConfig = new CLNodeConfig( pnodeConfig
                                                , lnodeConfigInfo );
    if (lnodeConfig)
    {
        // Bump the logical node count
        lnodesCount_++;
        // Add it to the array
        lnodesConfig_[lnodeConfigInfo.nid] = lnodeConfig;
        // Add it to the container list
        if ( head_ == NULL )
        {
            head_ = tail_ = lnodeConfig;
        }
        else
        {
            tail_->next_ = lnodeConfig;
            lnodeConfig->prev_ = tail_;
            tail_ = lnodeConfig;
        }

        // Set the next available nid
        nextNid_ = (lnodeConfigInfo.nid == nextNid_) ? (lnodeConfigInfo.nid+1) : nextNid_ ;
        if ( nextNid_ == lnodesConfigMax_ )
        {   // We are at the limit, search for unused nid from begining
            nextNid_ = -1;
            for (int i = 0; i < lnodesConfigMax_; i++ )
            {
                if ( lnodesConfig_[i] == NULL )
                {
                    nextNid_ = i;
                    break;
                }
            }
        }
        else if ( lnodesConfig_[nextNid_] != NULL )
        {   // nid is in use
            int next = ((nextNid_ + 1) < lnodesConfigMax_) ? nextNid_ + 1 : 0 ;
            nextNid_ = -1;
            for (int i = next; i < lnodesConfigMax_; i++ )
            {
                if ( lnodesConfig_[i] == NULL )
                {
                    nextNid_ = i;
                    break;
                }
            }
        }

        if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
        {
            trace_printf( "%s@%d - Added logical node configuration object\n"
                          "        (nid=%d, pnid=%d, nextNid_=%d)\n"
                          "        (lnodesCount_=%d,lnodesConfigMax=%d)\n"
                        , method_name, __LINE__
                        , lnodeConfigInfo.nid, pnodeConfig->GetPNid(), nextNid_
                        , lnodesCount_, lnodesConfigMax_);
        }
    }
    else
    {
        int err = errno;
        char la_buf[TC_LOG_BUF_SIZE];
        sprintf( la_buf, "[%s], Error: Can't allocate logical node configuration object - errno=%d (%s)\n"
               , method_name, err, strerror(errno));
        TcLogWrite(MON_LNODECONF_ADD_LNODE_2, TC_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
    return( lnodeConfig );
}

void CLNodeConfigContainer::DeleteLNodeConfig( CLNodeConfig *lnodeConfig )
{
    const char method_name[] = "CLNodeConfigContainer::DeleteLNodeConfig";
    TRACE_ENTRY;

    if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Deleting nid=%d, nextNid_=%d\n"
                     , method_name, __LINE__
                     , lnodeConfig->GetNid()
                     , nextNid_ );
    }
    
    int nid = lnodeConfig->GetNid();
    lnodesConfig_[nid] = NULL;
    
    if ( head_ == lnodeConfig )
        head_ = lnodeConfig->next_;
    if ( tail_ == lnodeConfig )
        tail_ = lnodeConfig->prev_;
    if ( lnodeConfig->prev_ )
        lnodeConfig->prev_->next_ = lnodeConfig->next_;
    if ( lnodeConfig->next_ )
        lnodeConfig->next_->prev_ = lnodeConfig->prev_;
    delete lnodeConfig;

    // Decrement the logical node count
    lnodesCount_--;
    
    if ( nextNid_ == -1 )
    { // We are at the limit, use the deleted nid as the next available
        nextNid_ = nid;
    }
    else if ( nextNid_ > nid )
    { // Always use the lower nid value
        nextNid_ = nid;
    }

    if (TcTraceSettings & (TC_TRACE_INIT | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d - Deleted logical node configuration object\n"
                      "        (nid=%d, nextNid_=%d)\n"
                      "        (lnodesCount_=%d,lnodesConfigMax=%d)\n"
                    , method_name, __LINE__
                    , nid, nextNid_
                    , lnodesCount_, lnodesConfigMax_);
    }

    TRACE_EXIT;
}

CLNodeConfig *CLNodeConfigContainer::GetLNodeConfig( int nid )
{
    const char method_name[] = "CLNodeConfigContainer::GetLNodeConfig";
    TRACE_ENTRY;

    CLNodeConfig *config = head_;
    while (config)
    {
        if ( config->GetNid() == nid )
        { 
            break;
        }
        config = config->GetNext();
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
