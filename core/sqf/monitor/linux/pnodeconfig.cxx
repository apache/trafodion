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


///////////////////////////////////////////////////////////////////////////////
//  Physical Node Configuration
///////////////////////////////////////////////////////////////////////////////

CPNodeConfig::CPNodeConfig( CPNodeConfigContainer *pnodesConfig
                          , int                    pnid
                          , const char            *hostname
                          )
            : pnodesConfig_(pnodesConfig)
            , pnid_(pnid)
            , spareNode_(false)
            , sparePNids_(NULL)
            , sparePNidsCount_(0)
            , next_(NULL)
            , prev_(NULL)
{
    const char method_name[] = "CPNodeConfig::CPNodeConfig";
    TRACE_ENTRY;

    int len = strlen( hostname );
    assert( len <= MPI_MAX_PROCESSOR_NAME );

    strcpy( name_, hostname );

    CPU_ZERO( &excludedCoreMask_ );

    TRACE_EXIT;
}

CPNodeConfig::~CPNodeConfig( void )
{
    const char method_name[] = "CPNodeConfig::~CPNodeConfig";
    TRACE_ENTRY;

    if (sparePNids_)
    {
        delete [] sparePNids_;
    }

    TRACE_EXIT;
}

int CPNodeConfig::GetSpareList( int sparePNids[] )
{
    const char method_name[] = "CPNodeConfig::GetSpareList";
    TRACE_ENTRY;
    
    if ( ! sparePNids_ ||  ! sparePNids ) 
    {
        return(0);
    }
    
    if ( sparePNidsCount_ > 0 )
    {
        for ( int i = 0; i < sparePNidsCount_ ; i++ )
        {
            sparePNids[i] = sparePNids_[i];
        }
    }
    
    TRACE_EXIT;
    return(sparePNidsCount_);
}

void CPNodeConfig::ResetSpare( void )
{
    const char method_name[] = "CPNodeConfig::ResetSpare";
    TRACE_ENTRY;
    
    spareNode_ = false;
    sparePNidsCount_ = 0;
    if (sparePNids_)
    {
        delete [] sparePNids_;
    }
    
    TRACE_EXIT;
}

void CPNodeConfig::SetSpareList( int sparePNids[], int spareCount )
{
    const char method_name[] = "CPNodeConfig::SetSpareList";
    TRACE_ENTRY;
    
    assert( ! sparePNids_ || spareCount );

    sparePNidsCount_ = spareCount;
    sparePNids_ = new int [sparePNidsCount_];

    if ( ! sparePNids_ )
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Can't allocate spare pnids array - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_PNODECONF_SET_SPARE_1, SQ_LOG_CRIT, la_buf);
    }
    else
    {
        for ( int i = 0; i < sparePNidsCount_ ; i++ )
        {
            sparePNids_[i] = sparePNids[i];
            if (trace_settings & TRACE_INIT)
            {
                trace_printf("%s@%d - Added spare pnid=%d to spare node array in (pnid=%d, nodename=%s)\n", method_name, __LINE__, sparePNids_[i], pnid_, name_);
            }
        }
        spareNode_ = true;
    }

    TRACE_EXIT;
}


CPNodeConfigContainer::CPNodeConfigContainer( void )
                     : pnodeConfig_(NULL)
                     , pnodesCount_(0)
                     , snodesCount_(0)
                     , head_(NULL)
                     , tail_(NULL)
{
    const char method_name[] = "CPNodeConfigContainer::CPNodeConfigContainer";
    TRACE_ENTRY;

    pnodeConfig_ = new CPNodeConfig *[MAX_NODES];

    if ( ! pnodeConfig_ )
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Can't allocate physical node configuration array - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_PNODECONF_CONSTR_1, SQ_LOG_CRIT, la_buf);
    }

    TRACE_EXIT;
}

CPNodeConfigContainer::~CPNodeConfigContainer( void )
{
    CPNodeConfig *pnodeConfig = head_;

    const char method_name[] = "CPNodeConfigContainer::~CPNodeConfigContainer";
    TRACE_ENTRY;

    // Delete entries
    while ( head_ )
    {
        DeletePNodeConfig( pnodeConfig );
        pnodeConfig = head_;
    }

    // Delete array
    if ( pnodeConfig_ )
    {
        delete [] pnodeConfig_;
    }

    TRACE_EXIT;
}

CPNodeConfig *CPNodeConfigContainer::AddPNodeConfig( int pnid
                                                   , char *name
                                                   , bool spare
                                                   )
{
    const char method_name[] = "CPNodeConfigContainer::AddPNodeConfig";
    TRACE_ENTRY;

    // Assume pnid list is sequential from zero
    if ( ! (pnid >= 0 && pnid <= (pnodesCount_ + 1)) )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Invalid pnid=%d - should be >=0 and <=%d)\n", method_name, pnid, (pnodesCount_ + 1));
        mon_log_write(MON_PNODECONF_ADD_PNODE_1, SQ_LOG_CRIT, la_buf);
        return( NULL );
    }

    CPNodeConfig *pnodeConfig = new CPNodeConfig( this
                                                , pnid
                                                , name );
    if (pnodeConfig)
    {
        if (trace_settings & TRACE_INIT)
        {
            trace_printf("%s@%d - Added physical node configuration object (pnid=%d, nodename=%s)\n", method_name, __LINE__, pnid, name);
        }

        if ( spare )
        {
            snodesCount_++;
            spareNodesConfigList_.push_back( pnodeConfig );
        }

        pnodesCount_++;
        // Add it to the array
        pnodeConfig_[pnid] = pnodeConfig;
        // Add it to the container list
        if ( head_ == NULL )
        {
            head_ = tail_ = pnodeConfig;
        }
        else
        {
            tail_->next_ = pnodeConfig;
            pnodeConfig->prev_ = tail_;
            tail_ = pnodeConfig;
        }
    }
    else
    {
        int err = errno;
        char la_buf[MON_STRING_BUF_SIZE];
        sprintf(la_buf, "[%s], Error: Can't allocate physical node configuration object - errno=%d (%s)\n", method_name, err, strerror(errno));
        mon_log_write(MON_PNODECONF_ADD_PNODE_2, SQ_LOG_ERR, la_buf);
    }

    TRACE_EXIT;
    return( pnodeConfig );
}

void CPNodeConfigContainer::DeletePNodeConfig( CPNodeConfig *pnodeConfig )
{
    
    if ( head_ == pnodeConfig )
        head_ = pnodeConfig->next_;
    if ( tail_ == pnodeConfig )
        tail_ = pnodeConfig->prev_;
    if ( pnodeConfig->prev_ )
        pnodeConfig->prev_->next_ = pnodeConfig->next_;
    if ( pnodeConfig->next_ )
        pnodeConfig->next_->prev_ = pnodeConfig->prev_;
    delete pnodeConfig;
}

int CPNodeConfigContainer::GetPNid( char *nodename )
{
    int pnid = -1;
    const char method_name[] = "CPNodeConfigContainer::GetPNid";
    TRACE_ENTRY;

    for (int i = 0; i < pnodesCount_; i++ )
    {
        if ( CPNodeConfigContainer::hostnamecmp( pnodeConfig_[i]->GetName(), nodename ) == 0 )
        {
            pnid = pnodeConfig_[i]->GetPNid();
        }
    }
    
    TRACE_EXIT;
    return( pnid );
}

void CPNodeConfig::SetName( char *newName ) 
{ 
    if (newName) 
      strcpy(name_, newName); 
} 

CPNodeConfig *CPNodeConfigContainer::GetPNodeConfig( int pnid )
{
    CPNodeConfig *config;

    const char method_name[] = "CPNodeConfigContainer::GetPNodeConfig";
    TRACE_ENTRY;

    if ( pnid >= 0 && pnid < pnodesCount_ )
    {
        config = pnodeConfig_[pnid];
    }
    else
    {
        config = NULL;
    }

    TRACE_EXIT;
    return config;
}

void CPNodeConfigContainer::GetSpareNodesConfigSet( const char *name
                                                  , PNodesConfigList_t &spareSet )
{
    bool foundInSpareSet = false;
    CPNodeConfig *spareNodeConfig;
    CPNodeConfig *pNodeconfig;
    PNodesConfigList_t tempSpareSet;

    const char method_name[] = "CPNodeConfigContainer::GetSpareNodesConfigSet";
    TRACE_ENTRY;

    PNodesConfigList_t::iterator itSn;
    for ( itSn = spareNodesConfigList_.begin(); 
          itSn != spareNodesConfigList_.end(); 
          itSn++ ) 
    {
        spareNodeConfig = *itSn;
        if (trace_settings & TRACE_INIT)
        {
            trace_printf( "%s@%d - %s is a configured spare node\n"
                        , method_name, __LINE__
                        , spareNodeConfig->GetName());
        }

        // Build the set of pnids that constitute the 'spare set'
        int sparePNidsCount = spareNodeConfig->GetSparesCount()+1;
        int *sparePNids = new int [sparePNidsCount];
        spareNodeConfig->GetSpareList( sparePNids );
        sparePNids[spareNodeConfig->GetSparesCount()] = spareNodeConfig->GetPNid();

        // Build the list of configured physical nodes that
        // constitute the 'spare set'
        for ( int i = 0; i < sparePNidsCount ; i++ )
        {
            pNodeconfig = GetPNodeConfig( sparePNids[i] );
            if ( pNodeconfig )
            {
                tempSpareSet.push_back( pNodeconfig );
                if (trace_settings & TRACE_INIT)
                {
                    trace_printf( "%s@%d - Added %s as member of spare set (%s, count=%ld)\n"
                                , method_name, __LINE__
                                , pNodeconfig->GetName()
                                , spareNodeConfig->GetName()
                                , tempSpareSet.size() );
                }
            }
        }
        
        // Check each node in the 'spare set'
        PNodesConfigList_t::iterator itSnSet;
        for ( itSnSet = tempSpareSet.begin(); 
              itSnSet != tempSpareSet.end(); 
              itSnSet++ ) 
        {
            pNodeconfig = *itSnSet;
            if (trace_settings & TRACE_INIT)
            {
                trace_printf( "%s@%d - %s is in spare set (%s, count=%ld)\n"
                            , method_name, __LINE__
                            , pNodeconfig->GetName()
                            , spareNodeConfig->GetName()
                            , tempSpareSet.size() );
            }
            if ( CPNodeConfigContainer::hostnamecmp( pNodeconfig->GetName(), name ) == 0 )
            {
                foundInSpareSet = true;
                spareSet = tempSpareSet;
                if (trace_settings & TRACE_INIT)
                {
                    trace_printf( "%s@%d - Found %s in spare set (%s, count=%ld)\n"
                                , method_name, __LINE__
                                , pNodeconfig->GetName()
                                , spareNodeConfig->GetName()
                                , tempSpareSet.size() );
                }
            }
        }
        if (sparePNids)
        {
            tempSpareSet.clear();
            delete [] sparePNids;
        }
        if (foundInSpareSet)
        {
            break;
        }
    }

    TRACE_EXIT;
}

int CPNodeConfigContainer::hostnamecmp(const char *p_str1, 
				       const char *p_str2)
{
  static bool sb_first_time = true;
  static bool sb_strict_hostname_check = false;
  if (sb_first_time) {
    sb_first_time = false;
    char *lv_envvar=getenv("MON_STRICT_HOSTNAME_CHECK");
    
    if (lv_envvar && (atoi(lv_envvar) == 1)) {
      sb_strict_hostname_check = true;
    }
  }

  if (!p_str1) return 1;
  if (!p_str2) return 1;

  int lv_ret = strcmp(p_str1, p_str2);
  if (lv_ret == 0) {
    return lv_ret;
  }
  if (sb_strict_hostname_check) {
    return lv_ret;
  }

  char lv_str1_to_cmp[1024];
  char lv_str2_to_cmp[1024];
  memset(lv_str1_to_cmp, 0, 1024);
  memset(lv_str2_to_cmp, 0, 1024);

  char *lp_str1_dot = strchr((char *) p_str1, '.');
  if (lp_str1_dot) {
    memcpy(lv_str1_to_cmp, p_str1, lp_str1_dot - p_str1);
  }
  else {
    strcpy(lv_str1_to_cmp, p_str1);
  }

  char *lp_str2_dot = strchr((char *) p_str2, '.');
  if (lp_str2_dot) {
    memcpy(lv_str2_to_cmp, p_str2, lp_str2_dot - p_str2);
  }
  else {
    strcpy(lv_str2_to_cmp, p_str2);
  }

  return strcmp(lv_str1_to_cmp, lv_str2_to_cmp);

}
