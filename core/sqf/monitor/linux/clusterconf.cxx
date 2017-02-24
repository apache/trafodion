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
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <mpi.h>
#include "msgdef.h"
#include "seabed/trace.h"
#include "montrace.h"
#include "monlogging.h"
#include "config.h"
#include "pnode.h"
#include "clusterconf.h"

extern CNodeContainer *Nodes;
extern CConfigContainer *Config;

///////////////////////////////////////////////////////////////////////////////
//  Cluster Configuration
///////////////////////////////////////////////////////////////////////////////

CClusterConfig::CClusterConfig( void )
              : CPNodeConfigContainer(MAX_NODES)
              , CLNodeConfigContainer(MAX_LNODES)
              , configReady_(false)
              , excludedCores_(false)
              , newPNodeConfig_(true)
              , newLNodeConfig_(false)
              , currNid_(0)
              , currPNid_(0)
              , currSPNid_(0)
              , currFirstCore_(0)
              , currLastCore_(0)
              , currProcessor_(0)
              , currZoneType_(ZoneType_Undefined)
              , currPNodeConfig_(NULL)
              , prevNid_(-1)
              , prevPNid_(-1)
              , prevSPNid_(-1)
              , prevFirstCore_(0)
              , prevLastCore_(0)
              , prevProcessor_(0)
              , prevZoneType_(ZoneType_Undefined)
              , prevPNodeConfig_(NULL)
              , spareIndex_(0)
              , lnodeConfig_(NULL)
              , processType_(ProcessType_Undefined)
              , requiresDTM_(false)
              , persistRetries_(0)
              , persistWindow_(0)
              , persistConfig_(NULL)
              , db_(NULL)
{
    const char method_name[] = "CClusterConfig::CClusterConfig";
    TRACE_ENTRY;

    currNodename_[0] = '\0';
    prevNodename_[0] = '\0';
    persistPrefix_[0] = '\0';
    processNamePrefix_[0] = '\0';
    processNameFormat_[0] = '\0';
    stdoutPrefix_[0] = '\0';
    stdoutFormat_[0] = '\0';
    programName_[0] = '\0';
    zoneFormat_[0] = '\0';

    memset( sparePNid_, 0, sizeof(sparePNid_) );

    CPU_ZERO( &currExcludedCoreMask_ );
    CPU_ZERO( &currCoreMask_ );
    CPU_ZERO( &prevExcludedCoreMask_ );
    CPU_ZERO( &prevCoreMask_ );

    TRACE_EXIT;
}

CClusterConfig::~CClusterConfig ( void )
{
    const char method_name[] = "CClusterConfig::~CClusterConfig";
    TRACE_ENTRY;

    TRACE_EXIT;
}

void CClusterConfig::Clear( void )
{
    const char method_name[] = "CClusterConfig::Clear";
    TRACE_ENTRY;

    // Delete the node configuration objects
    CPNodeConfigContainer::Clear();
    CLNodeConfigContainer::Clear();
    CPersistConfigContainer::Clear();

    configReady_ = false;
    excludedCores_ = false;
    newPNodeConfig_ = true;
    newLNodeConfig_ = false;
    currNid_ = 0;
    currPNid_ = 0;
    currSPNid_ = 0;
    currFirstCore_ = 0;
    currLastCore_ = 0;
    currProcessor_ = 0;
    currZoneType_ = ZoneType_Undefined;
    currPNodeConfig_ = NULL;
    prevNid_ = -1;
    prevPNid_ = -1;
    prevSPNid_ = -1;
    prevFirstCore_ = 0;
    prevLastCore_ = 0;
    prevProcessor_ = 0;
    prevZoneType_ = ZoneType_Undefined;
    prevPNodeConfig_ = NULL;
    spareIndex_ = 0;
    lnodeConfig_ = NULL;
    processType_ = ProcessType_Undefined;
    requiresDTM_ = false;
    persistRetries_ = 0;
    persistWindow_ = 0;
    persistConfig_ = NULL;

    currNodename_[0] = '\0';
    prevNodename_[0] = '\0';
    persistPrefix_[0] = '\0';
    processNamePrefix_[0] = '\0';
    processNameFormat_[0] = '\0';
    stdoutPrefix_[0] = '\0';
    stdoutFormat_[0] = '\0';
    programName_[0] = '\0';
    zoneFormat_[0] = '\0';

    memset( sparePNid_, 0, sizeof(sparePNid_) );

    CPU_ZERO( &currExcludedCoreMask_ );
    CPU_ZERO( &currCoreMask_ );
    CPU_ZERO( &prevExcludedCoreMask_ );
    CPU_ZERO( &prevCoreMask_ );

    //int rc = sqlite3_close_v2( db_ );
    int rc = sqlite3_close( db_ );
    if ( rc == SQLITE_OK)
    {
        db_ = NULL;
    }
    else
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s], Can't close configuration database, %s\n"
                , method_name, sqlite3_errmsg(db_) );
        mon_log_write( MON_CLUSTERCONF_CLEAR_1, SQ_LOG_CRIT, la_buf );
    }

    TRACE_EXIT;
}

void CClusterConfig::AddNodeConfiguration( bool spareNode )
{
    const char method_name[] = "CClusterConfig::AddNodeConfiguration";
    TRACE_ENTRY;

    if ( trace_settings & TRACE_INIT )
    {
        trace_printf( "%s@%d currNid=%d, currPNid=%d, currNodename=%s, "
                       "prevNid=%d, prevPNid=%d, prevNodename=%s\n"
                    , method_name, __LINE__
                    , currNid_
                    , currPNid_
                    , currNodename_
                    , prevNid_
                    , prevPNid_
                    , prevNodename_ );
    }

    if ( newPNodeConfig_ )
    {
        strncpy( prevNodename_, currNodename_, sizeof(prevNodename_) );
        prevPNid_ = currPNid_;
        prevExcludedCoreMask_ = currExcludedCoreMask_;
        prevExcludedFirstCore_ = currExcludedFirstCore_;
        prevExcludedLastCore_  = currExcludedLastCore_ ;

        prevPNodeConfig_ = AddPNodeConfig( prevPNid_
                                         , prevNodename_
                                         , prevExcludedFirstCore_
                                         , prevExcludedLastCore_
                                         , spareNode );
        prevPNodeConfig_->SetExcludedCoreMask( prevExcludedCoreMask_ );
        if ( spareNode )
        {
            prevPNodeConfig_->SetSpareList( sparePNid_, spareIndex_ );
        }
        newPNodeConfig_ = false;
    }
    if ( newLNodeConfig_ )
    {
        prevNid_ = currNid_;
        prevCoreMask_  = currCoreMask_;
        prevFirstCore_ = currFirstCore_;
        prevLastCore_  = currLastCore_ ;
        prevProcessor_ = currProcessor_;
        prevZoneType_  = currZoneType_;

        lnodeConfig_ = AddLNodeConfig( prevPNodeConfig_
                                     , prevNid_
                                     , prevCoreMask_
                                     , prevFirstCore_
                                     , prevLastCore_
                                     , prevProcessor_
                                     , prevZoneType_ );
        newLNodeConfig_ = false;
    }

    TRACE_EXIT;
}

void CClusterConfig::AddPersistConfiguration( void )
{
    const char method_name[] = "CClusterConfig::AddPersistConfiguration";
    TRACE_ENTRY;

    if ( trace_settings & TRACE_INIT )
    {
        trace_printf( "%s@%d persistkey=%s\n"
                    , method_name, __LINE__
                    , persistPrefix_ );
    }

    persistConfig_ = AddPersistConfig( persistPrefix_
                                     , processNamePrefix_
                                     , processNameFormat_
                                     , stdoutPrefix_
                                     , stdoutFormat_
                                     , programName_
                                     , zoneFormat_
                                     , processType_
                                     , requiresDTM_
                                     , persistRetries_
                                     , persistWindow_ );

    TRACE_EXIT;
}

bool CClusterConfig::DeleteNodeConfig( int  pnid )
{
    const char method_name[] = "CClusterConfig::DeleteNodeConfig";
    TRACE_ENTRY;

    bool rs = true;

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d deleting (pnid=%d), pnodesCount=%d, lnodesCount=%d\n"
                     , method_name, __LINE__
                     , pnid
                     , GetPNodesCount()
                     , GetLNodesCount() );
    }

    // Delete logical and physical nodes from the configuration database
    if (DeleteDbNodeData( pnid ))
    {
        // Delete logical and physical nodes from configuration objects
        CPNodeConfig *pnodeConfig = GetPNodeConfig( pnid );
        if (pnodeConfig)
        {

            CLNodeConfig *lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
            while ( lnodeConfig )
            {
                // Delete logical nodes unique strings from the configuration database
                if (!DeleteDbUniqueString( lnodeConfig->GetNid() ))
                {
                    rs = false;
                    break;
                }
                DeleteLNodeConfig( lnodeConfig );
                lnodeConfig = pnodeConfig->GetFirstLNodeConfig();
            }

            if (rs)
            {
                DeletePNodeConfig( pnodeConfig );

                if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
                {
                    trace_printf( "%s@%d deleted (pnid=%d), pnodesCount=%d, lnodesCount=%d\n"
                                 , method_name, __LINE__
                                 , pnid
                                 , GetPNodesCount()
                                 , GetLNodesCount() );
                }
            }
        }
    }
    else
    {
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] Node delete failed, pnid=%d\n",
                  method_name,  pnid );
        mon_log_write( MON_CLUSTERCONF_DELETENODE_1, SQ_LOG_ERR, buf );
        rs = false;
    }

    TRACE_EXIT;
    return( rs );
}

bool CClusterConfig::DeleteDbNodeData( int pnid )
{
    const char method_name[] = "CClusterConfig::DeleteDbNodeData";
    TRACE_ENTRY;

    bool rs = true;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return( false );
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d delete from lnode, pnode values (pNid=%d)\n"
                     , method_name, __LINE__
                     , pnid );
    }

    int rc;

    const char *sqlStmt1;
    sqlStmt1 = "delete from lnode where lnode.pNid = ?";

    sqlite3_stmt *prepStmt1 = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmt1, strlen(sqlStmt1)+1, &prepStmt1, NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        rs = false;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_CLUSTERCONF_DELETEDBNODE_1, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_int( prepStmt1, 1, pnid );

        rc = sqlite3_step( prepStmt1 );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            rs = false;
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] delete from lnode "
                      "value (pNid=%d) failed, error=%s (%d)\n"
                    , method_name
                    , pnid
                    , sqlite3_errmsg(db_), rc );
            mon_log_write( MON_CLUSTERCONF_DELETEDBNODE_2, SQ_LOG_ERR, buf );
        }
    }

    const char *sqlStmt2;
    sqlStmt2 = "delete from pnode where pnode.pNid = ?";

    sqlite3_stmt *prepStmt2 = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmt2, strlen(sqlStmt2)+1, &prepStmt2, NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        rs = false;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_CLUSTERCONF_DELETEDBNODE_3, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_int( prepStmt2, 1, pnid );

        rc = sqlite3_step( prepStmt2 );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            rs = false;
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] delete from pnode "
                      "value (pNid=%d) failed, error=%s (%d)\n"
                    , method_name
                    , pnid
                    , sqlite3_errmsg(db_), rc );
            mon_log_write( MON_CLUSTERCONF_DELETEDBNODE_4, SQ_LOG_ERR, buf );
        }
    }

    if ( prepStmt1 != NULL )
        sqlite3_finalize( prepStmt1 );
    if ( prepStmt2 != NULL )
        sqlite3_finalize( prepStmt2 );

    TRACE_EXIT;
    return( rs );
}

bool CClusterConfig::DeleteDbUniqueString( int nid )
{
    const char method_name[] = "CClusterConfig::DeleteDbUniqueString";
    TRACE_ENTRY;

    bool rs = true;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return( false );
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d delete from monRegUniqueStrings values (nid=%d)\n"
                     , method_name, __LINE__
                     , nid );
    }

    int rc;

    const char *sqlStmtA;
    sqlStmtA = "delete from monRegUniqueStrings where monRegUniqueStrings.nid = ?";

    sqlite3_stmt *prepStmtA = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmtA, strlen(sqlStmtA)+1, &prepStmtA, NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        rs = false;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_CLUSTERCONF_DELETEDBUSTRING_1, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_int( prepStmtA, 1, nid );

        rc = sqlite3_step( prepStmtA );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            rs = false;
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] delete from monRegUniqueStrings "
                      "value (nid=%d) failed, error=%s (%d)\n"
                    , method_name
                    , nid
                    , sqlite3_errmsg(db_), rc );
            mon_log_write( MON_CLUSTERCONF_DELETEDBUSTRING_2, SQ_LOG_ERR, buf );
        }
    }

    if ( prepStmtA != NULL )
        sqlite3_finalize( prepStmtA );

    TRACE_EXIT;
    return( rs );
}

// The following method maps the 'sqconfig' text file persist section's
// <persist-key>_PROCESS_TYPE string value to the internal
// PROCESSTYPE enum value
PROCESSTYPE CClusterConfig::GetProcessType( const char *processtype )
{
    if (strcmp( "DTM", processtype) == 0)
    {
        return(ProcessType_DTM);
    }
    else if (strcmp( "GENERIC", processtype) == 0)
    {
        return(ProcessType_Generic);
    }
    else if (strcmp( "WDG", processtype) == 0)
    {
        return(ProcessType_Watchdog);
    }
    else if (strcmp( "MXOSRVR", processtype) == 0)
    {
        return(ProcessType_MXOSRVR);
    }
    else if (strcmp( "SPX", processtype) == 0)
    {
        return(ProcessType_SPX);
    }
    else if (strcmp( "SSMP", processtype) == 0)
    {
        return(ProcessType_SSMP);
    }
    else if (strcmp( "PSD", processtype) == 0)
    {
        return(ProcessType_PSD);
    }
    else if (strcmp( "SMS", processtype) == 0)
    {
        return(ProcessType_SMS);
    }
    else if (strcmp( "TMID", processtype) == 0)
    {
        return(ProcessType_TMID);
    }
    else if (strcmp( "PERSIST", processtype) == 0)
    {
        return(ProcessType_PERSIST);
    }

    return(ProcessType_Undefined);
}

bool CClusterConfig::Initialize( void )
{
    const char method_name[] = "CClusterConfig::Initialize";
    TRACE_ENTRY;

    if ( db_ != NULL )
    {
        // Already initialized
        return( true );
    }

    char dbase[MAX_PROCESS_PATH];

    // Open the configuration database file
    char *configenv = getenv("SQ_CONFIGDB");
    if (configenv != NULL)
    {
        snprintf( dbase, sizeof(dbase), "%s", configenv);
    }
    else
    {
        snprintf( dbase, sizeof(dbase)
                , "%s/sql/scripts/sqconfig.db", getenv("TRAF_HOME"));
    }
    int rc = sqlite3_open_v2( dbase, &db_
                            , SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX
                            , NULL);
    if ( rc )
    {
        db_ = NULL;

        // See if database is in current directory
        int rc2 = sqlite3_open_v2( "sqconfig.db", &db_
                                 , SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX
                                 , NULL);
        if ( rc2 )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s], Can't open configuration database %s, %s\n"
                    , method_name, dbase, sqlite3_errmsg(db_) );
            mon_log_write( MON_CLUSTERCONF_INIT_1, SQ_LOG_CRIT, la_buf );
        }
    }

    if ( db_ != NULL )
    {
        rc = sqlite3_busy_timeout(db_, 1000);
        if ( rc )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] Can't set busy timeout for database %s, %s\n"
                    , method_name,  dbase, sqlite3_errmsg(db_) );
            mon_log_write( MON_CLUSTERCONF_INIT_2, SQ_LOG_ERR, la_buf );
        }

        char *sErrMsg = NULL;
        sqlite3_exec(db_, "PRAGMA synchronous = OFF", NULL, NULL, &sErrMsg);
        if (sErrMsg != NULL)
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] Can't set PRAGMA synchronous for database %s, %s\n"
                    , method_name,  dbase, sErrMsg );
            mon_log_write( MON_CLUSTERCONF_INIT_3, SQ_LOG_ERR, la_buf );
        }
    }

    TRACE_EXIT;
    return( true );
}

bool CClusterConfig::LoadConfig( void )
{
    const char method_name[] = "CClusterConfig::LoadConfig";
    TRACE_ENTRY;

    int  firstcore;
    int  lastcore;
    int  excfirstcore;
    int  exclastcore;
    int  nid;
    int  pnid;
    int  spnid;
    int  processors;
    int  rc;
    const char   *nodename;
    const char   *persistKeysValue;
    const char   *selLnodeStmt;
    sqlite3_stmt *prepLnodeStmt = NULL;
    const char   *selSnodeStmt;
    sqlite3_stmt *prepSnodeStmt = NULL;
    const char   *selPersistKeysStmt;
    sqlite3_stmt *prepPersistKeysStmt = NULL;
    ZoneType roles;

    // Prepare select logical nodes
    selLnodeStmt = "select p.pNid, l.lNid, p.nodeName, l.firstCore, l.lastCore,"
                   " p.excFirstCore, p.excLastCore, l.processors, l.roles"
                   "  from pnode p, lnode l where p.pNid = l.pNid";

    rc = sqlite3_prepare_v2( db_
                           , selLnodeStmt
                           , strlen(selLnodeStmt)+1
                           , &prepLnodeStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] prepare logical nodes failed, %s\n"
                , method_name,  sqlite3_errmsg(db_) );
        mon_log_write(MON_CLUSTERCONF_LOAD_1, SQ_LOG_CRIT, la_buf);
        return( false );
    }

    // Prepare select spare nodes
    selSnodeStmt = "select p.pNid, p.nodeName, p.excFirstCore, p.excLastCore,"
                   " s.spNid "
                   "  from pnode p, snode s where p.pNid = s.pNid";

    rc = sqlite3_prepare_v2( db_
                           , selSnodeStmt
                           , strlen(selSnodeStmt)+1
                           , &prepSnodeStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] prepare spare nodes failed, %s\n"
                , method_name,  sqlite3_errmsg(db_) );
        mon_log_write(MON_CLUSTERCONF_LOAD_2, SQ_LOG_CRIT, la_buf);
        return( false );
    }

    // Prepare select persistent process keys
    selPersistKeysStmt = "select p.valueName"
                         " from monRegPersistData p"
                         "  where p.keyName = 'PERSIST_PROCESS_KEYS'";

    rc = sqlite3_prepare_v2( db_
                           , selPersistKeysStmt
                           , strlen(selPersistKeysStmt)+1
                           , &prepPersistKeysStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] prepare persistent keys failed, %s\n"
                , method_name,  sqlite3_errmsg(db_) );
        mon_log_write(MON_CLUSTERCONF_LOAD_3, SQ_LOG_CRIT, la_buf);
        return( false );
    }

    // Process logical nodes
    while ( 1 )
    {
        rc = sqlite3_step( prepLnodeStmt );
        if ( rc == SQLITE_ROW )
        {  // Process row
            int colCount = sqlite3_column_count(prepLnodeStmt);
            if ( trace_settings & TRACE_INIT )
            {
                trace_printf("%s@%d sqlite3_column_count=%d\n",
                             method_name, __LINE__, colCount);
                for (int i=0; i<colCount; ++i)
                {
                    trace_printf("%s@%d column %d is %s\n",
                                 method_name, __LINE__, i,
                                 sqlite3_column_name(prepLnodeStmt, i));
                }
            }

            pnid = sqlite3_column_int(prepLnodeStmt, 0);
            nid = sqlite3_column_int(prepLnodeStmt, 1);
            nodename = (const char *) sqlite3_column_text(prepLnodeStmt, 2);
            firstcore = sqlite3_column_int(prepLnodeStmt, 3);
            lastcore = sqlite3_column_int(prepLnodeStmt, 4);
            excfirstcore = sqlite3_column_int(prepLnodeStmt, 5);
            exclastcore = sqlite3_column_int(prepLnodeStmt, 6);
            processors = sqlite3_column_int(prepLnodeStmt, 7);
            roles = (ZoneType) sqlite3_column_int(prepLnodeStmt, 8);
            ProcessLNode( nid
                        , pnid
                        , nodename
                        , excfirstcore
                        , exclastcore
                        , firstcore
                        , lastcore
                        , processors
                        , roles );
        }
        else if ( rc == SQLITE_DONE )
        {
            // Destroy prepared statement object
            if ( prepLnodeStmt != NULL )
            {
                sqlite3_finalize(prepLnodeStmt);
            }

            if ( trace_settings & TRACE_INIT )
            {
                trace_printf("%s@%d Finished processing logical nodes.\n",
                             method_name, __LINE__);
            }

            break;
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] Configuration database select node failed, %s\n"
                    , method_name, sqlite3_errmsg(db_));
            mon_log_write(MON_CLUSTERCONF_LOAD_5, SQ_LOG_CRIT, la_buf);
            return( false );
        }

        AddNodeConfiguration( false );
    }

    nid = -1;
    firstcore = -1;
    lastcore = -1;

    // Process spare nodes
    while ( 1 )
    {
        rc = sqlite3_step( prepSnodeStmt );
        if ( rc == SQLITE_ROW )
        {  // Process row
            int colCount = sqlite3_column_count(prepSnodeStmt);
            if ( trace_settings & TRACE_INIT )
            {
                trace_printf("%s@%d sqlite3_column_count=%d\n",
                             method_name, __LINE__, colCount);
                for (int i=0; i<colCount; ++i)
                {
                    trace_printf("%s@%d column %d is %s\n",
                                 method_name, __LINE__, i,
                                 sqlite3_column_name(prepSnodeStmt, i));
                }
            }

            pnid = sqlite3_column_int(prepSnodeStmt, 0);
            nodename = (const char *) sqlite3_column_text(prepSnodeStmt, 1);
            excfirstcore = sqlite3_column_int(prepSnodeStmt, 2);
            exclastcore = sqlite3_column_int(prepSnodeStmt, 3);
            spnid = sqlite3_column_int(prepSnodeStmt, 4);
            if ( ! ProcessSNode( pnid
                               , nodename
                               , excfirstcore
                               , exclastcore
                               , spnid ) )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                snprintf( la_buf, sizeof(la_buf)
                        , "[%s], Error: Invalid node configuration\n"
                        , method_name);
                mon_log_write(MON_CLUSTERCONF_LOAD_6, SQ_LOG_CRIT, la_buf);
                return( false );
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            // Destroy prepared statement object
            if ( prepSnodeStmt != NULL )
            {
                sqlite3_finalize(prepSnodeStmt);
            }

            if ( trace_settings & TRACE_INIT )
            {
                trace_printf("%s@%d Finished processing spare nodes.\n",
                             method_name, __LINE__);
            }

            break;
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] Configuration database select node failed, %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            mon_log_write(MON_CLUSTERCONF_LOAD_7, SQ_LOG_CRIT, la_buf);
            return( false );
        }

        AddNodeConfiguration( true );
    }

    // Process persistent process keys
    rc = sqlite3_step( prepPersistKeysStmt );
    if ( rc == SQLITE_ROW )
    {  // Process row
        int colCount = sqlite3_column_count(prepPersistKeysStmt);
        if ( trace_settings & TRACE_INIT )
        {
            trace_printf("%s@%d sqlite3_column_count=%d\n",
                         method_name, __LINE__, colCount);
            for (int i=0; i<colCount; ++i)
            {
                trace_printf("%s@%d column %d is %s\n",
                             method_name, __LINE__, i,
                             sqlite3_column_name(prepPersistKeysStmt, i));
            }
        }

        persistKeysValue = (const char *) sqlite3_column_text(prepPersistKeysStmt, 0);
        // Initialize vector of persistent keys
        InitializePersistKeys( (char *)persistKeysValue );
        if ( GetPersistKeysCount() == 0 )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] Invalid PERSIST_PROCESS_KEYS value, %s\n"
                    , method_name, persistKeysValue );
            mon_log_write(MON_CLUSTERCONF_LOAD_8, SQ_LOG_CRIT, la_buf);
            return( false );
        }
    
        vector<string>::iterator pkit;
        
        // Process each key in the vector
        for (pkit = pkeysVector_.begin(); pkit < pkeysVector_.end(); pkit++ )
        {
            strncpy(persistPrefix_, pkit->c_str(), sizeof(persistPrefix_));
            processNamePrefix_[0] = '\0';
            processNameFormat_[0] = '\0';
            stdoutPrefix_[0] = '\0';
            stdoutFormat_[0] = '\0';
            programName_[0] = '\0';
            zoneFormat_[0] = '\0';
            processType_ = ProcessType_Undefined;
            requiresDTM_ = false;
            persistRetries_ = 0;
            persistWindow_ = 0;
            if ( ! ProcessPersist() )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                snprintf( la_buf, sizeof(la_buf)
                        , "[%s], Invalid persistent process configuration!\n"
                        , method_name);
                mon_log_write(MON_CLUSTERCONF_LOAD_9, SQ_LOG_CRIT, la_buf);
            }

            AddPersistConfiguration();
        }
        // Destroy prepared statement object
        if ( prepPersistKeysStmt != NULL )
        {
            sqlite3_finalize(prepPersistKeysStmt);
        }
    }
    else if ( rc != SQLITE_DONE )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] Configuration database select persist keys failed, %s (rc=%d)\n"
                , method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write(MON_CLUSTERCONF_LOAD_9, SQ_LOG_CRIT, la_buf);
    }

    configReady_ = true;

    if ( trace_settings & TRACE_INIT )
    {
        if ( configReady_ )
            trace_printf("%s@%d - Successfully loaded 'sqconfig.db'" "\n", method_name, __LINE__);
        else
            trace_printf("%s@%d - Failed to load 'sqconfig.db'" "\n", method_name, __LINE__);
    }

    TRACE_EXIT;
    return( configReady_ );
}

void CClusterConfig::ProcessLNode( int nid
                                 , int pnid
                                 , const char *nodename
                                 , int excfirstcore
                                 , int exclastcore
                                 , int firstcore
                                 , int lastcore
                                 , int processors
                                 , int roles )
{
    const char method_name[] = "CClusterConfig::ProcessLNode";
    TRACE_ENTRY;

    if ( trace_settings & TRACE_INIT )
    {
        trace_printf( "%s@%d nid=%d, pnid=%d, name=%s, excluded cores=(%d:%d),"
                      " cores=(%d:%d), processors=%d, roles=%d\n"
                    , method_name, __LINE__
                    , nid
                    , pnid
                    , nodename
                    , excfirstcore
                    , exclastcore
                    , firstcore
                    , lastcore
                    , processors
                    , roles );
    }

    currPNid_ = pnid;
    newPNodeConfig_ = (currPNid_ != prevPNid_) ? true : false;
    if ( newPNodeConfig_ )
    {
        strncpy( currNodename_, nodename, sizeof(currNodename_) );
        excludedCores_ = (excfirstcore != -1 || exclastcore != -1)?true:false;
        if ( excludedCores_ )
        {
            SetCoreMask( excfirstcore, exclastcore, currExcludedCoreMask_ );
        }
    }

    currNid_ = nid;
    newLNodeConfig_ = (currNid_ != prevNid_) ? true : false;
    if ( newLNodeConfig_ )
    {
        SetCoreMask( firstcore, lastcore, currCoreMask_ );
        currFirstCore_ = firstcore;
        currLastCore_  = lastcore;
        currProcessor_ = processors;
        currZoneType_  = (ZoneType)roles;
    }

    TRACE_EXIT;
}

bool CClusterConfig::ProcessSNode( int pnid
                                 , const char *nodename
                                 , int excfirstcore
                                 , int exclastcore
                                 , int spnid )
{
    const char method_name[] = "CClusterConfig::ProcessSNode";
    TRACE_ENTRY;

    int  rc;
    const char   *selSnodeStmt;
    sqlite3_stmt *prepSnodeStmt = NULL;

    if ( trace_settings & TRACE_INIT )
    {
        trace_printf( "%s@%d pnid=%d, name=%s, excluded cores=(%d:%d),"
                      " spared pnid=%d\n"
                    , method_name, __LINE__
                    , pnid
                    , nodename
                    , excfirstcore
                    , exclastcore
                    , spnid );
    }

    currPNid_ = pnid;
    newPNodeConfig_ = (currPNid_ != prevPNid_) ? true : false;
    if ( newPNodeConfig_ )
    {
        strncpy( currNodename_, nodename, sizeof(currNodename_) );
        excludedCores_ = (excfirstcore != -1 || exclastcore != -1)?true:false;
        if ( excludedCores_ )
        {
            SetCoreMask( excfirstcore, exclastcore, currExcludedCoreMask_ );
        }
        spareIndex_ = 0;
        memset( sparePNid_, 0, sizeof(sparePNid_) );

        // Select all spared nodes configured for this spare node
        selSnodeStmt = "select p.pNid, s.spNid"
                       "  from pnode p, snode s"
                       "    where p.pNid = s.pNid and p.pNid = ?";

        rc = sqlite3_prepare_v2( db_
                               , selSnodeStmt
                               , strlen(selSnodeStmt)+1
                               , &prepSnodeStmt
                               , NULL);
        if ( rc != SQLITE_OK )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] prepare failed, %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            mon_log_write(MON_CLUSTERCONF_PROCESS_SNODE_1, SQ_LOG_CRIT, la_buf);
            abort();
        }
        else
        {   // Set pnid in prepared statement
            rc = sqlite3_bind_int(prepSnodeStmt, 1, currPNid_ );
            if ( rc != SQLITE_OK )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                snprintf( la_buf, sizeof(la_buf),
                          "[%s] sqlite3_bind_int failed: %s\n",
                          method_name,  sqlite3_errmsg(db_) );
                mon_log_write( MON_CLUSTERCONF_PROCESS_SNODE_2, SQ_LOG_CRIT, la_buf );
                abort();
            }
        }

        int  pnid;
        int  sparedpnid;

        // Process spare nodes
        while ( 1 )
        {
            rc = sqlite3_step( prepSnodeStmt );
            if ( rc == SQLITE_ROW )
            {  // Process row
                int colCount = sqlite3_column_count(prepSnodeStmt);
                if ( trace_settings & TRACE_INIT )
                {
                    trace_printf("%s@%d sqlite3_column_count=%d\n",
                                 method_name, __LINE__, colCount);
                    for (int i=0; i<colCount; ++i)
                    {
                        trace_printf("%s@%d column %d is %s\n",
                                     method_name, __LINE__, i,
                                     sqlite3_column_name(prepSnodeStmt, i));
                    }
                }

                pnid = sqlite3_column_int(prepSnodeStmt, 0);
                sparedpnid = sqlite3_column_int(prepSnodeStmt, 1);
                sparePNid_[spareIndex_] = sparedpnid;
                spareIndex_++;
            }
            else if ( rc == SQLITE_DONE )
            {
                // Destroy prepared statement object
                if ( prepSnodeStmt != NULL )
                {
                    sqlite3_finalize(prepSnodeStmt);
                }

                if ( trace_settings & TRACE_INIT )
                {
                    trace_printf("%s@%d Finished processing spared node set.\n",
                                 method_name, __LINE__);
                }

                break;
            }
            else
            {
                char la_buf[MON_STRING_BUF_SIZE];
                snprintf( la_buf, sizeof(la_buf)
                        , "[%s] Configuration database select node failed, %s\n"
                        , method_name,  sqlite3_errmsg(db_) );
                mon_log_write(MON_CLUSTERCONF_PROCESS_SNODE_3, SQ_LOG_CRIT, la_buf);
                abort();
            }
        }
    }

    TRACE_EXIT;
    return( true );
}

bool CClusterConfig::ProcessPersist( void )
{
    const char method_name[] = "CClusterConfig::ProcessPersist";
    TRACE_ENTRY;

    int  rc;
    char param[MAX_PERSIST_KEY_STR];
    const char   *persistKey;
    const char   *persistValue;
    const char   *selPersistStmt;
    sqlite3_stmt *prepPersistStmt = NULL;

    if ( trace_settings & TRACE_INIT )
    {
        trace_printf( "%s@%d processkey=%s\n"
                    , method_name, __LINE__
                    , persistPrefix_ );
    }
    
    snprintf( param, sizeof(param), "%s_%%", persistPrefix_ );

    // Prepare select persistent process for the key
    selPersistStmt = "select p.keyName, p.valueName"
                     " from monRegPersistData p"
                     "  where p.keyName like ?";

    rc = sqlite3_prepare_v2( db_
                           , selPersistStmt
                           , strlen(selPersistStmt)+1
                           , &prepPersistStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char la_buf[MON_STRING_BUF_SIZE];
        snprintf( la_buf, sizeof(la_buf)
                , "[%s] prepare persistent process failed, %s\n"
                , method_name,  sqlite3_errmsg(db_) );
        mon_log_write( MON_CLUSTERCONF_PROCESSPERSIST_1, SQ_LOG_CRIT, la_buf );
        abort();
    }
    else
    {   // Set key in prepared statement
        rc = sqlite3_bind_text( prepPersistStmt, 1, param, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] sqlite3_bind_text persistent processkey (%s) failed: %s\n"
                    , method_name, param,  sqlite3_errmsg(db_) );
            mon_log_write( MON_CLUSTERCONF_PROCESSPERSIST_2, SQ_LOG_CRIT, la_buf );
            abort();
        }
    }

    // Process each persist key value pair
    while ( 1 )
    {
        rc = sqlite3_step( prepPersistStmt );
        if ( rc == SQLITE_ROW )
        {  // Process row
            int colCount = sqlite3_column_count(prepPersistStmt);
            if ( trace_settings & TRACE_INIT )
            {
                trace_printf("%s@%d sqlite3_column_count=%d\n",
                             method_name, __LINE__, colCount);
                for (int i=0; i<colCount; ++i)
                {
                    trace_printf("%s@%d column %d is %s\n",
                                 method_name, __LINE__, i,
                                 sqlite3_column_name(prepPersistStmt, i));
                }
            }

            persistKey = (const char *) sqlite3_column_text(prepPersistStmt, 0);
            persistValue = (const char *) sqlite3_column_text(prepPersistStmt, 1);

            // Parse the value based on the key
            if ( ! ProcessPersistData( persistKey, persistValue ) )
            {
                char la_buf[MON_STRING_BUF_SIZE];
                snprintf( la_buf, sizeof(la_buf)
                        , "[%s], Error: Invalid persist key value in "
                          "configuration, key=%s, value=%s\n"
                        , method_name, persistKey, persistValue );
                mon_log_write(MON_CLUSTERCONF_PROCESSPERSIST_3, SQ_LOG_CRIT, la_buf);
                abort();
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            // Destroy prepared statement object
            if ( prepPersistStmt != NULL )
            {
                sqlite3_finalize(prepPersistStmt);
            }

            if ( trace_settings & TRACE_INIT )
            {
                trace_printf( "%s@%d Finished processing persistent process configuration.\n"
                            , method_name, __LINE__);
            }

            break;
        }
        else
        {
            char la_buf[MON_STRING_BUF_SIZE];
            snprintf( la_buf, sizeof(la_buf)
                    , "[%s] Configuration database select persistent process failed, %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            mon_log_write(MON_CLUSTERCONF_PROCESSPERSIST_4, SQ_LOG_CRIT, la_buf);
            abort();
        }
    }

    TRACE_EXIT;
    return( true );
}

bool CClusterConfig::ProcessPersistData( const char *persistkey
                                       , const char *persistvalue )
{
    const char method_name[] = "CClusterConfig::ProcessPersistData";
    TRACE_ENTRY;

    char workValue[MAX_PERSIST_KEY_STR];
    char *pch;
    char *token1;
    char *token2;
    static const char *delimNone = "\0";
    static const char *delimComma = ",";
    static const char *delimPercent = "%";
    static int chPercent = '%';

    if ( trace_settings & TRACE_INIT )
    {
        trace_printf( "%s@%d persistKey=%s, persistValue=%s\n"
                    , method_name, __LINE__
                    , persistkey, persistvalue );
    }
    
    strncpy( workValue, persistvalue, sizeof(workValue) );
    
    pch = (char *) strstr( persistkey, PERSIST_PROCESS_NAME_KEY );
    if (pch != NULL)
    {
        // Extract name prefix
        token1 = strtok( workValue, delimPercent );
        if (token1)
        {
            strncpy( processNamePrefix_, token1, sizeof(processNamePrefix_) );
        }
        // Extract nid format
        strncpy( workValue, persistvalue, sizeof(workValue) );
        token2 = strchr( workValue, chPercent );
        if (token2)
        {
            strncpy( processNameFormat_, token2, sizeof(processNameFormat_) );
        }
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_PROCESS_TYPE_KEY );
    if (pch != NULL)
    {
        // Set process type
        processType_ = GetProcessType( workValue );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_PROGRAM_NAME_KEY );
    if (pch != NULL)
    {
        // Save program name
        strncpy( programName_, workValue, sizeof(programName_) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_REQUIRES_DTM );
    if (pch != NULL)
    {
        // Set flag
        requiresDTM_ = (strcasecmp(workValue,"Y") == 0) ? true : false;
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_STDOUT_KEY );
    if (pch != NULL)
    {
        // Extract name prefix
        token1 = strtok( workValue, delimPercent );
        if (token1)
        {
            strncpy( stdoutPrefix_, token1, sizeof(stdoutPrefix_) );
        }
        // Extract nid format
        strncpy( workValue, persistvalue, sizeof(workValue) );
        token2 = strchr( workValue, chPercent );
        if (token2)
        {
            strncpy( stdoutFormat_, token2, sizeof(stdoutFormat_) );
        }
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_RETRIES_KEY );
    if (pch != NULL)
    {
        // Set retries
        token1 = strtok( workValue, delimComma );
        if (token1)
        {
            persistRetries_ = atoi(token1);
        }
        // Set time window
        token2 = strtok( NULL, delimNone );
        if (token2)
        {
            persistWindow_ = atoi(token2);
        }
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_ZONES_KEY );
    if (pch != NULL)
    {
        // Extract zid format
        strncpy( zoneFormat_, workValue, sizeof(zoneFormat_) );
        goto done;
    }
    else
    {
        TRACE_EXIT;
        return( false );
    }

done:

    if ( trace_settings & TRACE_INIT )
    {
        trace_printf( "%s@%d pch=%s\n", method_name, __LINE__, pch);
    }

    TRACE_EXIT;
    return( true );
}

bool CClusterConfig::SaveNodeConfig( const char *name
                                   , int         nid
                                   , int         pnid
                                   , int         firstCore
                                   , int         lastCore
                                   , int         processors
                                   , int         excludedFirstCore
                                   , int         excludedLastCore
                                   , int         roles )
{
    const char method_name[] = "CClusterConfig::SaveNodeConfig";
    TRACE_ENTRY;

    bool rs = true;

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d Saving node config (node_name=%s, processors=%d, "
                      "roles=%d, firstCore=%d, lastCore=%d "
                      "excludedFirstCore=%d, excludedLastCore=%d)\n"
                     , method_name, __LINE__
                     , name
                     , processors
                     , roles
                     , firstCore
                     , lastCore
                     , excludedFirstCore
                     , excludedLastCore );
    }

    prevNid_ = -1;
    prevPNid_ = -1;

    // Insert data into pnode and lnode tables
    if (SaveDbPNodeData( name
                       , pnid
                       , excludedFirstCore
                       , excludedLastCore ))
    {
        if (SaveDbLNodeData( nid
                           , pnid
                           , firstCore
                           , lastCore
                           , processors
                           , roles ))
        
        {
            // Pre-process the Node configuration attributes
            ProcessLNode( nid
                        , pnid
                        , name
                        , excludedFirstCore
                        , excludedLastCore
                        , firstCore
                        , lastCore
                        , processors
                        , roles );

            // Add new logical and physical nodes to configuration objects
            AddNodeConfiguration( false );
        }
        else
        {
            rs = false;
        }
    }
    else
    {
        rs = false;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] Node delete failed, pnid=%d\n",
                  method_name,  pnid );
        mon_log_write( MON_CLUSTERCONF_SAVENODE_1, SQ_LOG_ERR, buf );
    }

    TRACE_EXIT;
    return( rs );
}

bool CClusterConfig::SaveDbLNodeData( int         nid
                                    , int         pnid
                                    , int         firstCore
                                    , int         lastCore
                                    , int         processors
                                    , int         roles )
{
    const char method_name[] = "CClusterConfig::SaveDbLNodeData";
    TRACE_ENTRY;

    bool rs = true;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return( false );
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d inserting into lnode values (lNid=%d, pNid=%d, "
                      "processors=%d, roles=%d, firstCore=%d, lastCore=%d)\n"
                     , method_name, __LINE__
                     , nid
                     , pnid
                     , processors
                     , roles
                     , firstCore
                     , lastCore );
    }

    int rc;
    const char *sqlStmt;
    sqlStmt = "insert into lnode values (?, ?, ?, ?, ?, ?)";

    sqlite3_stmt *prepStmt = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt, NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        rs = false;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_CLUSTERCONF_SAVELNODE_1, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_int( prepStmt, 1, nid );
        sqlite3_bind_int( prepStmt, 2, pnid );
        sqlite3_bind_int( prepStmt, 3, processors );
        sqlite3_bind_int( prepStmt, 4, roles );
        sqlite3_bind_int( prepStmt, 5, firstCore );
        sqlite3_bind_int( prepStmt, 6, lastCore );

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            rs = false;
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] inserting into lnode values "
                      "(lNid=%d, pNid=%d, processors=%d, roles=%d, "
                      "firstCore=%d, lastCore=%d) "
                      "failed, error=%s (%d)\n"
                    , method_name
                    , nid
                    , pnid
                    , processors
                    , roles
                    , firstCore
                    , lastCore
                    , sqlite3_errmsg(db_), rc );
            mon_log_write( MON_CLUSTERCONF_SAVELNODE_2, SQ_LOG_ERR, buf );
        }
    }

    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;
    return( rs );
}

bool CClusterConfig::SaveDbPNodeData( const char *name
                                    , int         pnid
                                    , int         excludedFirstCore
                                    , int         excludedLastCore )
{
    const char method_name[] = "CClusterConfig::SaveDbPNodeData";
    TRACE_ENTRY;

    bool rs = true;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return( false );
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d inserting into pnode values (pNid=%d, "
                      "nodeName=%s, excFirstCore=%d, excLastCore=%d)\n"
                     , method_name, __LINE__
                     , pnid
                     , name
                     , excludedFirstCore
                     , excludedLastCore );
    }

    int rc;
    const char *sqlStmt;
    sqlStmt = "insert into pnode values (?, ?, ?, ?)";

    sqlite3_stmt *prepStmt = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt, NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        rs = false;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_CLUSTERCONF_SAVEPNODE_1, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_int( prepStmt, 1, pnid );
        sqlite3_bind_text( prepStmt, 2, name, -1, SQLITE_STATIC );
        sqlite3_bind_int( prepStmt, 3, excludedFirstCore );
        sqlite3_bind_int( prepStmt, 4, excludedLastCore );

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            rs = false;
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] inserting into "
                      "pnode (pNid=%d, nodeName=%s, excFirstCore=%d, excLastCore=%d) "
                      "failed, error=%s (%d)\n"
                    , method_name
                    , pnid
                    , name
                    , excludedFirstCore
                    , excludedLastCore
                    , sqlite3_errmsg(db_), rc );
            mon_log_write( MON_CLUSTERCONF_SAVEPNODE_2, SQ_LOG_ERR, buf );
        }
    }

    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;
    return( rs );
}

void CClusterConfig::SetCoreMask( int        firstCore
                                , int        lastCore
                                , cpu_set_t &coreMask )
{
    CPU_ZERO( &coreMask );
    for (int i = firstCore; i < (lastCore+1) ; i++ )
    {
        CPU_SET( i, &coreMask );
    }
}

bool CClusterConfig::UpdatePNodeConfig( int         pnid
                                      , const char *name
                                      , int         excludedFirstCore
                                      , int         excludedLastCore )
{
    const char method_name[] = "CClusterConfig::UpdatePNodeConfig";
    TRACE_ENTRY;

    bool rs = true;

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d Updating pnode config "
                      "(pnid=%d, node_name=%s, "
                      "excludedFirstCore=%d, excludedLastCore=%d)\n"
                     , method_name, __LINE__
                     , pnid
                     , name
                     , excludedFirstCore
                     , excludedLastCore );
    }

    // Update pnode table
    if (UpdateDbPNodeData( pnid
                         , name
                         , excludedFirstCore
                         , excludedLastCore ))
    {
        // Update physical node to configuration object
        UpdatePNodeConfiguration( pnid
                                , name
                                , excludedFirstCore
                                , excludedLastCore );
    }
    else
    {
        rs = false;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] PNode update failed, pnid=%d, node_name=%s\n"
                , method_name,  pnid, name );
        mon_log_write( MON_CLUSTERCONF_UPDATEPNODECFG_1, SQ_LOG_ERR, buf );
    }

    TRACE_EXIT;
    return( rs );
}

bool CClusterConfig::UpdateDbPNodeData( int         pnid
                                      , const char *name
                                      , int         excludedFirstCore
                                      , int         excludedLastCore )
{
    const char method_name[] = "CClusterConfig::UpdateDbPNodeData";
    TRACE_ENTRY;

    bool rs = true;

    if ( db_ == NULL)
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d cannot use database since database open"
                         " failed.\n", method_name, __LINE__);
        }

        TRACE_EXIT;
        return( false );
    }

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d update pnode "
                      "nodeName=%s, excFirstCore=%d, excLastCore=%d\n"
                      "where pnid=%d)\n"
                     , method_name, __LINE__
                     , name
                     , excludedFirstCore
                     , excludedLastCore
                     , pnid );
    }

    int rc;
    const char *sqlStmt;
    sqlStmt = "update or replace pnode "
              " set nodeName = ?, excFirstCore = ?, excLastCore = ?"
              "   where pnode.pNid = :pNid";

    sqlite3_stmt *prepStmt = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmt, strlen(sqlStmt)+1, &prepStmt, NULL);
    if ( rc != SQLITE_OK )
    {
        if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
        {
            trace_printf("%s@%d prepare failed, error=%s (%d)\n",
                         method_name, __LINE__, sqlite3_errmsg(db_), rc);
        }

        rs = false;
        char buf[MON_STRING_BUF_SIZE];
        snprintf( buf, sizeof(buf), "[%s] prepare failed, error=%s (%d)\n",
                  method_name,  sqlite3_errmsg(db_), rc );
        mon_log_write( MON_CLUSTERCONF_UPDATEDBPNODE_1, SQ_LOG_ERR, buf );
    }
    else
    {
        sqlite3_bind_text( prepStmt, 
                           1, 
                           name, -1, SQLITE_STATIC );
        sqlite3_bind_int( prepStmt, 
                          2,
                          excludedFirstCore );
        sqlite3_bind_int( prepStmt,
                          3, 
                          excludedLastCore );
        sqlite3_bind_int( prepStmt,
                          sqlite3_bind_parameter_index( prepStmt, ":pNid" ),
                          pnid );

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            rs = false;
            char buf[MON_STRING_BUF_SIZE];
            snprintf( buf, sizeof(buf), "[%s] update pnode"
                      " (nodeName=%s, excFirstCore=%d, excLastCore=%d) "
                      " where pNid=%d " 
                      "failed, error=%s (%d)\n"
                    , method_name
                    , name
                    , excludedFirstCore
                    , excludedLastCore
                    , pnid
                    , sqlite3_errmsg(db_), rc );
            mon_log_write( MON_CLUSTERCONF_UPDATEDBPNODE_2, SQ_LOG_ERR, buf );
        }
    }

    if ( prepStmt != NULL )
        sqlite3_finalize( prepStmt );

    TRACE_EXIT;
    return( rs );
}

void CClusterConfig::UpdatePNodeConfiguration( int         pnid
                                             , const char *name
                                             , int         excludedFirstCore
                                             , int         excludedLastCore )
{
    const char method_name[] = "CClusterConfig::UpdatePNodeConfiguration";
    TRACE_ENTRY;

    if (trace_settings & (TRACE_INIT | TRACE_REQUEST))
    {
        trace_printf( "%s@%d pnid=%d, name=%s, "
                       "excludedFirstCore=%d, excludedLastCore=%d\n"
                    , method_name, __LINE__
                    , pnid
                    , name
                    , excludedFirstCore
                    , excludedLastCore );
    }

    CPNodeConfig *pnodeConfig = GetPNodeConfig( pnid );
    if ( pnodeConfig )
    {
        pnodeConfig->SetName( name );
        pnodeConfig->SetExcludedFirstCore( excludedFirstCore );
        pnodeConfig->SetExcludedLastCore( excludedLastCore );
    }

    TRACE_EXIT;
}

