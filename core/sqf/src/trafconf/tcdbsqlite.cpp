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

#include <string>
#include <stdlib.h>
#include <string.h>

using namespace std;

#include "tclog.h"
#include "tctrace.h"
#include "trafconf/trafconfig.h"
#include "tcdbsqlite.h"

#define MAX_PROCESS_PATH           256

///////////////////////////////////////////////////////////////////////////////
//  Cluster Configuration
///////////////////////////////////////////////////////////////////////////////

CTcdbSqlite::CTcdbSqlite( void )
           : CTcdbStore( TCDBSQLITE )
           , db_(NULL)
{
    const char method_name[] = "CTcdbSqlite::CTcdbSqlite";
    TRACE_ENTRY;

    memcpy(&eyecatcher_, "TCSL", 4);

    TRACE_EXIT;
}

CTcdbSqlite::~CTcdbSqlite ( void )
{
    const char method_name[] = "CTcdbSqlite::~CTcdbSqlite";
    TRACE_ENTRY;

    memcpy(&eyecatcher_, "tcsl", 4);

    TRACE_EXIT;
}

int CTcdbSqlite::AddLNodeData( int         nid
                             , int         pnid
                             , int         firstCore
                             , int         lastCore
                             , int         processors
                             , int         roles )
{
    const char method_name[] = "CTcdbSqlite::AddLNodeData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
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
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt, NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_int( prepStmt, 1, nid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(nid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt, 2, pnid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(pnid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt, 3, processors );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(processors) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt, 4, roles );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(roles) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt, 5, firstCore );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(firsCore) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt, 6, lastCore );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(lastCore) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                      "(lNid=%d, pNid=%d, processors=%d, roles=%d, "
                      "firstCore=%d, lastCore=%d)\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_) 
                    , nid, pnid, processors, roles, firstCore, lastCore );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::AddNameServer( const char *nodeName )
{
    const char method_name[] = "CTcdbSqlite::AddNameServer";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    if (TcTraceSettings & (TC_TRACE_NAMESERVER | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d inserting into monRegNameServer values (node=%s)\n"
                     , method_name, __LINE__
                     , nodeName );
    }

    int rc;
    const char *sqlStmt;
    sqlStmt = "insert into monRegNameServer values (?, ?)";

    sqlite3_stmt *prepStmt = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt, NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_text( prepStmt, 1, nodeName, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_text(nodeName) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_text( prepStmt, 2, nodeName, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_text(nodeName) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                      "(nodeName=%s)\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_) 
                    , nodeName );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::AddPNodeData( const char *name
                             , int         pnid
                             , int         excludedFirstCore
                             , int         excludedLastCore )
{
    const char method_name[] = "CTcdbSqlite::AddPNodeData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
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
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt, NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_int( prepStmt, 1, pnid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(pnid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_text( prepStmt, 2, name, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(name) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt, 3, excludedFirstCore );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(excludedFirstCore) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt, 4, excludedLastCore );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(excludedLastCore) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                      "(pNid=%d, nodeName=%s, excFirstCore=%d, excLastCore=%d)\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_)
                    , pnid, name, excludedFirstCore, excludedLastCore );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

// insert key into monRegKeyName table
int CTcdbSqlite::AddRegistryKey( const char *key )
{
    const char method_name[] = "CTcdbSqlite::AddRegistryKey";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting key=%s into monRegKeyName\n",
                     method_name, __LINE__, key);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "insert into monRegKeyName (keyName) values ( :key );";
    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_text( prepStmt, 
                                sqlite3_bind_parameter_index( prepStmt, ":key" ),
                                key, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(:key) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW ) 
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, key=%s, error: %s\n"
                    , method_name, sqlStmt, key, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }
        
    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

// insert key into monRegProcName table
int CTcdbSqlite::AddRegistryProcess( const char *name )
{
    const char method_name[] = "CTcdbSqlite::AddRegistryProcess";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting name=%s into monRegProcName\n",
                     method_name, __LINE__, name);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "insert into monRegProcName (procName) values ( :name );";
    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_text( prepStmt,
                                sqlite3_bind_parameter_index( prepStmt, ":name" ),
                                name, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(:name) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW ) 
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, name=%s, error: %s\n"
                    , method_name, sqlStmt, name, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }
        
    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::AddRegistryClusterData( const char *key
                                       , const char *dataValue )
{
    const char method_name[] = "CTcdbSqlite::AddRegistryClusterData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting key=%s into monRegClusterData\n",
                     method_name, __LINE__, key);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "insert or replace into monRegClusterData (dataValue, keyId)"
              " select :dataValue,"
              "         k.keyId FROM monRegKeyName k"
              " where k.keyName = :key";
    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_text( prepStmt,
                                sqlite3_bind_parameter_index( prepStmt,
                                                              ":dataValue" ),
                                dataValue, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(:dataValue) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_text( prepStmt,
                                sqlite3_bind_parameter_index( prepStmt, ":key" ),
                                key, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(:key) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW ) 
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, key=%s, error: %s\n"
                    , method_name, sqlStmt, key, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }
        
    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::AddRegistryProcessData( const char *procName
                                       , const char *key
                                       , const char *dataValue )
{
    const char method_name[] = "CTcdbSqlite::AddRegistryProcessData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting key=%s into monRegProcData for "
                     "proc=%s\n", method_name, __LINE__, key, procName);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "insert or replace into monRegProcData (dataValue, procId, keyId )"
              "   select :dataValue,"
              "      p.procId,"
              "       (SELECT k.keyId "
              "          FROM monRegKeyName k"
              "         WHERE k.keyName = :key)"
              "   FROM monRegProcName p"
              "   WHERE UPPER(p.procName) = UPPER(:procName)";

    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_text( prepStmt,
                                sqlite3_bind_parameter_index( prepStmt,
                                                              ":procName" ),
                                procName, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(:procName) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_text( prepStmt,
                                sqlite3_bind_parameter_index( prepStmt,
                                                              ":dataValue" ),
                                dataValue, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(:dataValue) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_text( prepStmt,
                                sqlite3_bind_parameter_index( prepStmt, ":key" ),
                                key, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(:key) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW ) 
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, key=%s, proc=%s, error: %s\n"
                    , method_name, sqlStmt, key, procName, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::AddUniqueString( int nid
                                , int id
                                , const char *uniqStr )
{
    const char method_name[] = "CTcdbSqlite::AddUniqueString";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf("%s@%d inserting unique string nid=%d id=%d into "
                     "monRegUniqueStrings\n", method_name, __LINE__,
                     nid, id);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "insert or replace into monRegUniqueStrings values (?, ?, ?)";

    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_int( prepStmt, 1, nid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(nid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt, 2, id );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(id) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_text( prepStmt, 3, uniqStr, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(uniqStr) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if ( rc != SQLITE_DONE )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, nid=%d, id=%d, error: %s\n"
                    , method_name, sqlStmt, nid, id, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::Close( void )
{
    const char method_name[] = "CTcdbSqlite::Close";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    int rc = sqlite3_close( db_ );
    if ( rc == SQLITE_OK)
    {
        db_ = NULL;
    }
    else
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] sqlite3_close() error: %s\n"
                , method_name, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::DeleteNameServer( const char *nodeName )
{
    const char method_name[] = "CTcdbSqlite::DeleteNameServer";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d delete from monRegNameServer, values (nodeName=%s)\n"
                     , method_name, __LINE__
                     , nodeName );
    }

    int rc;

    const char *sqlStmt;
    sqlStmt = "delete from monRegNameServer where monRegNameServer.keyName = ?";

    sqlite3_stmt *prepStmt = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt, NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error=%s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_text( prepStmt, 1, nodeName, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_text(nodeName) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, nodeName=%s, error: %s\n"
                    , method_name, sqlStmt, nodeName, sqlite3_errmsg(db_));
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::DeleteNodeData( int pnid )
{
    const char method_name[] = "CTcdbSqlite::DeleteNodeData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d delete from lnode, pnode values (pNid=%d)\n"
                     , method_name, __LINE__
                     , pnid );
    }

    int rc;

    const char *sqlStmt1;
    sqlStmt1 = "delete from lnode where lnode.pNid = ?";

    sqlite3_stmt *prepStmt1 = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmt1, static_cast<int>(strlen(sqlStmt1)+1), &prepStmt1, NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error=%s\n"
                , method_name, sqlStmt1, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_int( prepStmt1, 1, pnid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(pnid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt1 != NULL )
            {
                sqlite3_finalize( prepStmt1 );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt1 );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, pNid=%d, error: %s\n"
                    , method_name, sqlStmt1, pnid, sqlite3_errmsg(db_));
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt1 != NULL )
            {
                sqlite3_finalize( prepStmt1 );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    const char *sqlStmt2;
    sqlStmt2 = "delete from pnode where pnode.pNid = ?";

    sqlite3_stmt *prepStmt2 = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmt2, static_cast<int>(strlen(sqlStmt2)+1), &prepStmt2, NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error=%s\n"
                , method_name, sqlStmt2, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        if ( prepStmt1 != NULL )
        {
            sqlite3_finalize( prepStmt1 );
        }
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_int( prepStmt2, 1, pnid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(pnid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt1 != NULL )
            {
                sqlite3_finalize( prepStmt1 );
            }
            if ( prepStmt2 != NULL )
            {
                sqlite3_finalize( prepStmt2 );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt2 );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, pNid=%d, error: %s\n"
                    , method_name, sqlStmt2, pnid, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt1 != NULL )
            {
                sqlite3_finalize( prepStmt1 );
            }
            if ( prepStmt2 != NULL )
            {
                sqlite3_finalize( prepStmt2 );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt1 != NULL )
    {
        sqlite3_finalize( prepStmt1 );
    }
    if ( prepStmt2 != NULL )
    {
        sqlite3_finalize( prepStmt2 );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::DeleteUniqueString( int nid )
{
    const char method_name[] = "CTcdbSqlite::DeleteUniqueString";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d delete from monRegUniqueStrings values (nid=%d)\n"
                     , method_name, __LINE__
                     , nid );
    }

    int rc;

    const char *sqlStmtA;
    sqlStmtA = "delete from monRegUniqueStrings where monRegUniqueStrings.nid = ?";

    sqlite3_stmt *prepStmtA = NULL;
    rc = sqlite3_prepare_v2( db_, sqlStmtA, static_cast<int>(strlen(sqlStmtA)+1), &prepStmtA, NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error=%s\n"
                , method_name, sqlStmtA, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_int( prepStmtA, 1, nid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(nid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmtA != NULL )
            {
                sqlite3_finalize( prepStmtA );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmtA );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, pNid=%d, error: %s\n"
                    , method_name, sqlStmtA, nid, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmtA != NULL )
            {
                sqlite3_finalize( prepStmtA );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmtA != NULL )
    {
        sqlite3_finalize( prepStmtA );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::Initialize( void )
{
    const char method_name[] = "CTcdbSqlite::Initialize";
    TRACE_ENTRY;

    if ( IsInitialized() )
    {
        // Already initialized
        TRACE_EXIT;
        return( TCALREADYINIT );
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
                , "%s/sqconfig.db", getenv("TRAF_VAR"));
    }
    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
    {
        trace_printf( "%s@%d Opening SQLite database file %s\n"
                    , method_name, __LINE__, dbase );
    }
    int rc = sqlite3_open_v2( dbase, &db_
                            , SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX
                            , NULL);
    if ( rc )
    {
        db_ = NULL;

        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Opening SQLite database file sqconfig.db in current directory\n"
                        , method_name, __LINE__ );
        }
        // See if database is in current directory
        int rc2 = sqlite3_open_v2( "sqconfig.db", &db_
                                 , SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX
                                 , NULL);
        if ( rc2 )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], sqlite3_open_v2(%s) failed, error: %s\n"
                    , method_name, dbase, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( db_ != NULL )
    {
        rc = sqlite3_busy_timeout(db_, 1000);
        if ( rc )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], sqlite3_busy_timeout(%s) failed, error: %s\n"
                    , method_name,  dbase, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        char *sErrMsg = NULL;
        sqlite3_exec(db_, "PRAGMA synchronous = OFF", NULL, NULL, &sErrMsg);
        if (sErrMsg != NULL)
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s], sqlite3_exec(PRAGMA synchronous = OFF) failed, error: %s\n"
                    , method_name, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetNameServers( int *count, int max, char **nodeNames )
{
    const char method_name[] = "CTcdbSqlite::GetNameServers";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    

    int  rc;

    int  nodeCount = 0;
    const char   *nodename = NULL;
    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;
    sqlStmt = "select p.keyName"
              " from monRegNameServer p";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , static_cast<int>(strlen(sqlStmt)+1)
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    // Process nameservers
    while ( 1 )
    {
        rc = sqlite3_step( prepStmt );
        if ( rc == SQLITE_ROW )
        {  // Process row
            if ( max == 0 )
            {
                nodeCount++;
                continue;
            }

            int colCount = sqlite3_column_count(prepStmt);
            if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d sqlite3_column_count=%d\n",
                             method_name, __LINE__, colCount);
                for (int i=0; i<colCount; ++i)
                {
                    trace_printf("%s@%d column %d is %s\n",
                                 method_name, __LINE__, i,
                                 sqlite3_column_name(prepStmt, i));
                }
            }

            if ( nodeCount < max )
            {
                nodename = (const char *) sqlite3_column_text(prepStmt, 0);
                if (nodename)
                {
                    nodeNames[nodeCount] = new char[strlen(nodename)+1];
                    strcpy(nodeNames[nodeCount], nodename);
                }
                else
                    nodeNames[nodeCount] = NULL;
                nodeCount++;
            }
            else
            {
                *count = nodeCount;
                if ( prepStmt != NULL )
                {
                    sqlite3_finalize( prepStmt );
                }
                TRACE_EXIT;
                return( TCDBTRUNCATE );
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            *count = nodeCount;
            if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d Finished processing nameservers.\n",
                             method_name, __LINE__);
            }
            break;
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetNameServer( const char *nodeName )
{
    const char method_name[] = "CTcdbSqlite::GetNameServer";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    nodeName = nodeName; // touch
    return( TCNOTINIT );

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetNode( int nid
                        , TcNodeConfiguration_t &nodeConfig )
{
    const char method_name[] = "CTcdbSqlite::GetNode";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    int  rc;
    int  firstcore = -1;
    int  lastcore = -1;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  lnid = -1;
    int  pnid = -1;
    int  processors = 0;
    int  roles;
    const char   *nodename = NULL;
    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;

    // Prepare select logical nodes
    sqlStmt = "select p.pNid, l.lNid, p.nodeName, l.firstCore, l.lastCore,"
                   " p.excFirstCore, p.excLastCore, l.processors, l.roles"
                   "  from pnode p, lnode l where p.pNid = l.pNid"
                   "   and l.lNid = ?";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , static_cast<int>(strlen(sqlStmt)+1)
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {   // Set nid in prepared statement
        rc = sqlite3_bind_int(prepStmt, 1, nid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(nid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    rc = sqlite3_step( prepStmt );
    if ( rc == SQLITE_ROW )
    {  // Process row
        int colCount = sqlite3_column_count(prepStmt);
        if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
        {
            trace_printf("%s@%d sqlite3_column_count=%d\n",
                         method_name, __LINE__, colCount);
            for (int i=0; i<colCount; ++i)
            {
                trace_printf("%s@%d column %d is %s\n",
                             method_name, __LINE__, i,
                             sqlite3_column_name(prepStmt, i));
            }
        }

        pnid = sqlite3_column_int(prepStmt, 0);
        lnid = sqlite3_column_int(prepStmt, 1);
        nodename = (const char *) sqlite3_column_text(prepStmt, 2);
        firstcore = sqlite3_column_int(prepStmt, 3);
        lastcore = sqlite3_column_int(prepStmt, 4);
        excfirstcore = sqlite3_column_int(prepStmt, 5);
        exclastcore = sqlite3_column_int(prepStmt, 6);
        processors = sqlite3_column_int(prepStmt, 7);
        roles = sqlite3_column_int(prepStmt, 8);
        SetLNodeData( lnid
                    , pnid
                    , nodename
                    , excfirstcore
                    , exclastcore
                    , firstcore
                    , lastcore
                    , processors
                    , roles
                    , nodeConfig );
    }
    else if ( rc == SQLITE_DONE )
    {
        if ( prepStmt != NULL )
        {
            sqlite3_finalize( prepStmt );
        }

        if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
        {
            trace_printf("%s@%d Finished processing logical nodes.\n",
                         method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCDBNOEXIST );
    }
    else
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        if ( prepStmt != NULL )
        {
            sqlite3_finalize( prepStmt );
        }
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetNode( const char *name
                        , TcNodeConfiguration_t &nodeConfig )
{
    const char method_name[] = "CTcdbSqlite::GetNode";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    int  rc;
    int  firstcore = -1;
    int  lastcore = -1;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  nid = -1;
    int  pnid = -1;
    int  processors = 0;
    int  roles;
    const char   *nodename = NULL;
    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;

    // Prepare select logical nodes
    sqlStmt = "select p.pNid, l.lNid, p.nodeName, l.firstCore, l.lastCore,"
                   " p.excFirstCore, p.excLastCore, l.processors, l.roles"
                   "  from pnode p, lnode l where p.pNid = l.pNid"
                   "   and p.nodeName = ?";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , static_cast<int>(strlen(sqlStmt)+1)
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {   // Set name in prepared statement
        rc = sqlite3_bind_text( prepStmt, 1, name, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(name) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    rc = sqlite3_step( prepStmt );
    if ( rc == SQLITE_ROW )
    {  // Process row
        int colCount = sqlite3_column_count(prepStmt);
        if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
        {
            trace_printf("%s@%d sqlite3_column_count=%d\n",
                         method_name, __LINE__, colCount);
            for (int i=0; i<colCount; ++i)
            {
                trace_printf("%s@%d column %d is %s\n",
                             method_name, __LINE__, i,
                             sqlite3_column_name(prepStmt, i));
            }
        }

        pnid = sqlite3_column_int(prepStmt, 0);
        nid = sqlite3_column_int(prepStmt, 1);
        nodename = (const char *) sqlite3_column_text(prepStmt, 2);
        firstcore = sqlite3_column_int(prepStmt, 3);
        lastcore = sqlite3_column_int(prepStmt, 4);
        excfirstcore = sqlite3_column_int(prepStmt, 5);
        exclastcore = sqlite3_column_int(prepStmt, 6);
        processors = sqlite3_column_int(prepStmt, 7);
        roles = sqlite3_column_int(prepStmt, 8);
        SetLNodeData( nid
                    , pnid
                    , nodename
                    , excfirstcore
                    , exclastcore
                    , firstcore
                    , lastcore
                    , processors
                    , roles
                    , nodeConfig );
    }
    else if ( rc == SQLITE_DONE )
    {
        if ( prepStmt != NULL )
        {
            sqlite3_finalize( prepStmt );
        }

        if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
        {
            trace_printf("%s@%d Finished processing logical nodes.\n",
                         method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCDBNOEXIST );
    }
    else
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        if ( prepStmt != NULL )
        {
            sqlite3_finalize( prepStmt );
        }
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    // Destroy prepared statement object
    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetNodes( int &count
                         , int max
                         , TcNodeConfiguration_t nodeConfig[] )
{
    const char method_name[] = "CTcdbSqlite::GetNodes";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    int  rc;
    int  firstcore = -1;
    int  lastcore = -1;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  nid = -1;
    int  pnid = -1;
    int  processors = 0;
    int  roles;
    int  nodeCount = 0;
    const char   *nodename = NULL;
    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;

    // Prepare select logical nodes
    sqlStmt = "select p.pNid, l.lNid, p.nodeName, l.firstCore, l.lastCore,"
                   " p.excFirstCore, p.excLastCore, l.processors, l.roles"
                   "  from pnode p, lnode l where p.pNid = l.pNid";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , static_cast<int>(strlen(sqlStmt)+1)
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    // Process logical nodes
    while ( 1 )
    {
        rc = sqlite3_step( prepStmt );
        if ( rc == SQLITE_ROW )
        {  // Process row
            if ( max == 0 )
            {
                nodeCount++;
                continue;
            }

            int colCount = sqlite3_column_count(prepStmt);
            if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d sqlite3_column_count=%d\n",
                             method_name, __LINE__, colCount);
                for (int i=0; i<colCount; ++i)
                {
                    trace_printf("%s@%d column %d is %s\n",
                                 method_name, __LINE__, i,
                                 sqlite3_column_name(prepStmt, i));
                }
            }

            if ( nodeCount < max )
            {
                pnid = sqlite3_column_int(prepStmt, 0);
                nid = sqlite3_column_int(prepStmt, 1);
                nodename = (const char *) sqlite3_column_text(prepStmt, 2);
                firstcore = sqlite3_column_int(prepStmt, 3);
                lastcore = sqlite3_column_int(prepStmt, 4);
                excfirstcore = sqlite3_column_int(prepStmt, 5);
                exclastcore = sqlite3_column_int(prepStmt, 6);
                processors = sqlite3_column_int(prepStmt, 7);
                roles = sqlite3_column_int(prepStmt, 8);
                SetLNodeData( nid
                            , pnid
                            , nodename
                            , excfirstcore
                            , exclastcore
                            , firstcore
                            , lastcore
                            , processors
                            , roles
                            , nodeConfig[nodeCount] );
                nodeCount++;
            }
            else
            {
                count = nodeCount;
                if ( prepStmt != NULL )
                {
                    sqlite3_finalize( prepStmt );
                }
                TRACE_EXIT;
                return( TCDBTRUNCATE );
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            count = nodeCount;
            if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d Finished processing logical nodes.\n",
                             method_name, __LINE__);
            }
            break;
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetPNode( int pNid
                         , TcPhysicalNodeConfiguration_t &pnodeConfig )
{
    const char method_name[] = "CTcdbSqlite::GetPNode";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int  rc;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  pnid = -1;
    const char   *nodename = NULL;
    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;

    // Prepare select logical nodes
    sqlStmt = "select p.pNid, p.nodeName, p.excFirstCore, p.excLastCore"
                   "  from pnode p where p.pNid = ?";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , static_cast<int>(strlen(sqlStmt)+1)
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {   // Set nid in prepared statement
        rc = sqlite3_bind_int(prepStmt, 1, pNid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(pNid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    rc = sqlite3_step( prepStmt );
    if ( rc == SQLITE_ROW )
    {  // Process row
        int colCount = sqlite3_column_count(prepStmt);
        if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
        {
            trace_printf("%s@%d sqlite3_column_count=%d\n",
                         method_name, __LINE__, colCount);
            for (int i=0; i<colCount; ++i)
            {
                trace_printf("%s@%d column %d is %s\n",
                             method_name, __LINE__, i,
                             sqlite3_column_name(prepStmt, i));
            }
        }

        pnid = sqlite3_column_int(prepStmt, 0);
        nodename = (const char *) sqlite3_column_text(prepStmt, 1);
        excfirstcore = sqlite3_column_int(prepStmt, 2);
        exclastcore = sqlite3_column_int(prepStmt, 3);
        SetPNodeData( pnid
                    , nodename
                    , excfirstcore
                    , exclastcore
                    , pnodeConfig );
    }
    else if ( rc == SQLITE_DONE )
    {
        if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
        {
            trace_printf("%s@%d Finished processing logical nodes.\n",
                         method_name, __LINE__);
        }
    }
    else
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        if ( prepStmt != NULL )
        {
            sqlite3_finalize( prepStmt );
        }
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetPNode( const char *name
                         , TcPhysicalNodeConfiguration_t &pnodeConfig )
{
    const char method_name[] = "CTcdbSqlite::GetPNode";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int  rc;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  pnid = -1;
    const char   *nodename = NULL;
    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;

    // Prepare select logical nodes
    sqlStmt = "select p.pNid, p.nodeName, p.excFirstCore, p.excLastCore"
                   "  from pnode p where p.nodeName = ?";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , static_cast<int>(strlen(sqlStmt)+1)
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
        
    rc = sqlite3_bind_text( prepStmt, 1, name, -1, SQLITE_STATIC );
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] sqlite3_bind_text(name) failed, error: %s\n"
                , method_name,  sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        if ( prepStmt != NULL )
        {
            sqlite3_finalize( prepStmt );
        }
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    rc = sqlite3_step( prepStmt );
    if ( rc == SQLITE_ROW )
    {  // Process row
        int colCount = sqlite3_column_count(prepStmt);
        if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
        {
            trace_printf("%s@%d sqlite3_column_count=%d\n",
                         method_name, __LINE__, colCount);
            for (int i=0; i<colCount; ++i)
            {
                trace_printf("%s@%d column %d is %s\n",
                             method_name, __LINE__, i,
                             sqlite3_column_name(prepStmt, i));
            }
        }

        pnid = sqlite3_column_int(prepStmt, 0);
        nodename = (const char *) sqlite3_column_text(prepStmt, 1);
        excfirstcore = sqlite3_column_int(prepStmt, 2);
        exclastcore = sqlite3_column_int(prepStmt, 3);
        SetPNodeData( pnid
                    , nodename
                    , excfirstcore
                    , exclastcore
                    , pnodeConfig );
    }
    else if ( rc == SQLITE_DONE )
    {
        if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
        {
            trace_printf("%s@%d Finished processing logical nodes.\n",
                         method_name, __LINE__);
        }
    }
    else
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        if ( prepStmt != NULL )
        {
            sqlite3_finalize( prepStmt );
        }
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetSNodes( int &count
                          , int max
                          , TcPhysicalNodeConfiguration_t spareNodeConfig[] )
{
    const char method_name[] = "CTcdbSqlite::GetSNodes";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }

    int  rc;
    int  pnid = -1;
    int  excfirstcore = -1;
    int  exclastcore = -1;
    int  snodeCount = 0;
    const char   *nodename = NULL;
    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;

    // Prepare select spare nodes
    sqlStmt = "select p.pNid, p.nodeName, p.excFirstCore, p.excLastCore,"
              " s.spNid "
              "  from pnode p, snode s where p.pNid = s.pNid";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , static_cast<int>(strlen(sqlStmt)+1)
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    count = snodeCount;

    // Process spare nodes
    while ( 1 )
    {
        rc = sqlite3_step( prepStmt );
        if ( rc == SQLITE_ROW )
        {  // Process row
            if ( max == 0 )
            {
                snodeCount++;
                continue;
            }

            int colCount = sqlite3_column_count(prepStmt);
            if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d sqlite3_column_count=%d\n",
                             method_name, __LINE__, colCount);
                for (int i=0; i<colCount; ++i)
                {
                    trace_printf("%s@%d column %d is %s\n",
                                 method_name, __LINE__, i,
                                 sqlite3_column_name(prepStmt, i));
                }
            }

            if ( snodeCount < max )
            {
                pnid = sqlite3_column_int(prepStmt, 0);
                nodename = (const char *) sqlite3_column_text(prepStmt, 1);
                excfirstcore = sqlite3_column_int(prepStmt, 2);
                exclastcore = sqlite3_column_int(prepStmt, 3);
                if ( ! GetSNodeData( pnid
                                   , nodename
                                   , excfirstcore
                                   , exclastcore
                                   , spareNodeConfig[snodeCount] ) )
                {
                    char buf[TC_LOG_BUF_SIZE];
                    snprintf( buf, sizeof(buf)
                            , "[%s], Error: Invalid node configuration\n"
                            , method_name);
                    TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
                    if ( prepStmt != NULL )
                    {
                        sqlite3_finalize( prepStmt );
                    }
                    TRACE_EXIT;
                    return( TCDBOPERROR );
                }
                snodeCount++;
            }
            else
            {
                count = snodeCount;
                if ( prepStmt != NULL )
                {
                    sqlite3_finalize( prepStmt );
                }
                TRACE_EXIT;
                return( TCDBTRUNCATE );
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            count = snodeCount;
            if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d Finished processing spare nodes.\n",
                             method_name, __LINE__);
            }
            break;
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetSNodeData( int pnid
                             , const char *nodename
                             , int excfirstcore
                             , int exclastcore 
                             , TcPhysicalNodeConfiguration_t &spareNodeConfig )
{
    const char method_name[] = "CTcdbSqlite::GetSNodeData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int  rc;
    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;

    if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d pnid=%d, name=%s, excluded cores=(%d:%d)\n"
                    , method_name, __LINE__
                    , pnid
                    , nodename
                    , excfirstcore
                    , exclastcore );
    }

    if (TcIsRealCluster)
    {
        char short_node_name[TC_PROCESSOR_NAME_MAX];
        char str1[TC_PROCESSOR_NAME_MAX];
        char *tmpptr = NULL;
        tmpptr = (char*)nodename;

        while ( *tmpptr )
        {
            *tmpptr = (char)tolower( *tmpptr );
            tmpptr++;
        }

        // Extract the domain portion of the name if any
        memset( str1, 0, TC_PROCESSOR_NAME_MAX );
        memset( short_node_name, 0, TC_PROCESSOR_NAME_MAX );
        strcpy (str1, nodename );

        char *str1_dot = strchr( (char *) str1, '.' );
        if ( str1_dot )
        {
            memcpy( short_node_name, str1, str1_dot - str1 );
            // copy the domain portion and skip the '.'
            strcpy( spareNodeConfig.node_name, short_node_name );
            strcpy( spareNodeConfig.domain_name, str1_dot+1 );
        }
        else
        {
            strncpy( spareNodeConfig.node_name
                   , nodename
                   , sizeof(spareNodeConfig.node_name) );
            spareNodeConfig.domain_name[0] = 0;
        }
    }
    else
    {
        strncpy( spareNodeConfig.node_name
               , nodename
               , sizeof(spareNodeConfig.node_name) );
    }

    spareNodeConfig.pnid = pnid;
    spareNodeConfig.excluded_first_core = excfirstcore;
    spareNodeConfig.excluded_last_core = exclastcore;

    // Select all spared nodes configured for this spare node
    sqlStmt = "select p.pNid, s.spNid"
              "  from pnode p, snode s"
              "    where p.pNid = s.pNid and p.pNid = ?";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , static_cast<int>(strlen(sqlStmt)+1)
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {   // Set pnid in prepared statement
        rc = sqlite3_bind_int(prepStmt, 1, pnid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(pnid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    int  sparedpnid;
    int  spareCount = 0;

    // Process spare nodes
    while ( 1 )
    {
        rc = sqlite3_step( prepStmt );
        if ( rc == SQLITE_ROW )
        {  // Process row
            int colCount = sqlite3_column_count(prepStmt);
            if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d sqlite3_column_count=%d\n",
                             method_name, __LINE__, colCount);
                for (int i=0; i<colCount; ++i)
                {
                    trace_printf("%s@%d column %d is %s\n",
                                 method_name, __LINE__, i,
                                 sqlite3_column_name(prepStmt, i));
                }
            }

            sparedpnid = sqlite3_column_int(prepStmt, 1);
            spareNodeConfig.spare_pnid[spareCount] = sparedpnid;
            spareCount++;
        }
        else if ( rc == SQLITE_DONE )
        {
            spareNodeConfig.spare_count = spareCount;
            if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d Finished processing spared node set.\n",
                             method_name, __LINE__);
            }
            break;
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetPersistProcess( const char *persistPrefix
                                  , TcPersistConfiguration_t &persistConfig )
{
    const char method_name[] = "CTcdbSqlite::GetPersistProcess";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int  rc, rs;
    char param[TC_PERSIST_KEY_MAX];
    const char   *persistKey;
    const char   *persistValue;
    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;

    if ( TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d processkey=%s\n"
                    , method_name, __LINE__
                    , persistPrefix );
    }
    
    strncpy( persistConfig.persist_prefix
           , persistPrefix
           , sizeof(persistConfig.persist_prefix) );

    snprintf( param, sizeof(param), "%s_%%", persistPrefix );

    // Prepare select persistent process for the key
    sqlStmt = "select p.keyName, p.valueName"
              " from monRegPersistData p"
              "  where p.keyName like ?";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , static_cast<int>(strlen(sqlStmt)+1)
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {   // Set key in prepared statement
        rc = sqlite3_bind_text( prepStmt, 1, param, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(keyName) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    while ( 1 )
    {
        rc = sqlite3_step( prepStmt );
        if ( rc == SQLITE_ROW )
        {  // Process row
            int colCount = sqlite3_column_count(prepStmt);
            if ( TcTraceSettings & (TC_TRACE_PERSIST | TC_TRACE_REQUEST) )
            {
                trace_printf( "%s@%d sqlite3_column_count=%d\n"
                            , method_name, __LINE__, colCount);
                for (int i=0; i<colCount; ++i)
                {
                    trace_printf("%s@%d column %d is %s\n",
                                 method_name, __LINE__, i,
                                 sqlite3_column_name(prepStmt, i));
                }
            }

            persistKey = (const char *) sqlite3_column_text(prepStmt, 0);
            persistValue = (const char *) sqlite3_column_text(prepStmt, 1);

            if ( TcTraceSettings & (TC_TRACE_PERSIST | TC_TRACE_REQUEST) )
            {
                trace_printf( "%s@%d monRegPersistData key=%s, value=%s\n"
                            , method_name, __LINE__, persistKey, persistValue);
            }

            // Parse the value based on the key
            rs = SetPersistProcessData( persistKey
                                      , persistValue
                                      , persistConfig );
            if ( rs != TCSUCCESS )
            {
                char buf[TC_LOG_BUF_SIZE];
                snprintf( buf, sizeof(buf)
                        , "[%s], Error: Invalid persist key value in "
                          "configuration, key=%s, value=%s\n"
                        , method_name, persistKey, persistValue );
                TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
                if ( prepStmt != NULL )
                {
                    sqlite3_finalize( prepStmt );
                }
                TRACE_EXIT;
                return( TCDBOPERROR );
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            if ( TcTraceSettings & (TC_TRACE_PERSIST | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d Finished processing all rows.\n",
                             method_name, __LINE__);
            }
            break;
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetPersistProcessKeys( const char *persistProcessKeys )
{
    const char method_name[] = "CTcdbSqlite::GetPersistProcessKeys";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    

    int  rc;

    const char   *sqlStmt;
    sqlite3_stmt *prepStmt = NULL;
    sqlStmt = "select p.valueName"
              " from monRegPersistData p"
              "  where p.keyName = 'PERSIST_PROCESS_KEYS'";

    rc = sqlite3_prepare_v2( db_
                           , sqlStmt
                           , static_cast<int>(strlen(sqlStmt)+1)
                           , &prepStmt
                           , NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    // Process persistent process keys
    rc = sqlite3_step( prepStmt );
    if ( rc == SQLITE_ROW )
    {  // Process row
        int colCount = sqlite3_column_count(prepStmt);

        if ( TcTraceSettings & (TC_TRACE_PERSIST | TC_TRACE_REQUEST) )
        {
            trace_printf("%s@%d sqlite3_column_count=%d\n",
                         method_name, __LINE__, colCount);
            for (int i=0; i<colCount; ++i)
            {
                trace_printf("%s@%d column %d is %s\n",
                             method_name, __LINE__, i,
                             sqlite3_column_name(prepStmt, i));
            }
        }

        const unsigned char *value;

        value = sqlite3_column_text(prepStmt, 0);
        strncpy( (char *)persistProcessKeys, (const char *)value, TC_PERSIST_KEYS_VALUE_MAX );
    
    }
    else if ( rc == SQLITE_DONE )
    {
        if ( prepStmt != NULL )
        {
            sqlite3_finalize( prepStmt );
        }
        TRACE_EXIT;
        return( TCDBNOEXIST );
    }
    else
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
        if ( prepStmt != NULL )
        {
            sqlite3_finalize( prepStmt );
        }
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetRegistryClusterSet( int &count
                                      , int max
                                      , TcRegistryConfiguration_t registryConfig[] )
{
    const char method_name[] = "CTcdbSqlite::GetRegistryClusterSet";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int rc;
    const unsigned char *group;
    const unsigned char *key;
    const unsigned char *value;
    int entryNum = 0;

    count = 0;

    if ( db_ == NULL )
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf("%s@%d cannot initialize registry from database "
                         "since database open failed.\n", method_name,
                         __LINE__);
        }

        TRACE_EXIT;
        return( TCNOTINIT );
    }

    // Read cluster configuration registry entries and populate in-memory
    // structures.
    const char *sqlStmt;
    sqlStmt = "select k.keyName, d.dataValue "
              " from monRegKeyName k, monRegClusterData d "
              " where k.keyId = d.keyId";
    sqlite3_stmt *prepStmt;

    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt, NULL);
    if( rc!=SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    while ( 1 )
    {
        rc = sqlite3_step(prepStmt);
        if ( rc == SQLITE_ROW )
        {  // Process row
            if ( max == 0 )
            {
                ++entryNum;
                continue;
            }

            if ( entryNum < max )
            {
                group = (const unsigned char *) "CLUSTER";
                key = sqlite3_column_text(prepStmt, 0);
                value = sqlite3_column_text(prepStmt, 1);
                if ( TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST) )
                {
                    trace_printf( "%s@%d entry %d: group=%s key=%s, value=%s\n"
                                , method_name, __LINE__
                                , entryNum, group, key, value);
                }
                strncpy( registryConfig[entryNum].scope, (const char *)group, TC_REGISTRY_KEY_MAX );
                strncpy( registryConfig[entryNum].key, (const char *)key, TC_REGISTRY_KEY_MAX );
                strncpy( registryConfig[entryNum].value, (const char *)value, TC_REGISTRY_VALUE_MAX );
                ++entryNum;
            }
            else
            {
                count = entryNum;
                if ( prepStmt != NULL )
                {
                    sqlite3_finalize( prepStmt );
                }
                TRACE_EXIT;
                return( TCDBTRUNCATE );
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            count = entryNum;
            break;
        }
        else
        { 
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    count =entryNum;

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetRegistryProcessSet( int &count
                                      , int max
                                      , TcRegistryConfiguration_t registryConfig[] )
{
    const char method_name[] = "CTcdbSqlite::GetRegistryProcessSet";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int rc;
    const unsigned char *group;
    const unsigned char *key;
    const unsigned char *value;
    int entryNum = 0;

    count = 0;

    if ( db_ == NULL )
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf("%s@%d cannot initialize registry from database "
                         "since database open failed.\n", method_name,
                         __LINE__);
        }

        TRACE_EXIT;
        return( TCNOTINIT );
    }

    // Read process configuration registry entries and populate in-memory
    // structures.
    const char *sqlStmt;
    sqlStmt = "select p.procName, k.keyName, d.dataValue"
              " from monRegProcName p, monRegKeyName k, monRegProcData d"
              " where p.procId = d.procId"
              "   and k.keyId = d.keyId";
    sqlite3_stmt *prepStmt;

    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt, NULL);
    if( rc!=SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }

    while ( 1 )
    {
        rc = sqlite3_step(prepStmt);
        if ( rc == SQLITE_ROW )
        {  // Process row
            if ( max == 0 )
            {
                ++entryNum;
                continue;
            }

            if ( entryNum < max )
            {
                group = sqlite3_column_text(prepStmt, 0);
                key = sqlite3_column_text(prepStmt, 1);
                value = sqlite3_column_text(prepStmt, 2);
                if ( TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST) )
                {
                    trace_printf( "%s@%d entry %d: group=%s key=%s, value=%s\n"
                                , method_name, __LINE__
                                , entryNum, group, key, value);
                }
                strncpy( registryConfig[entryNum].scope, (const char *)group, TC_REGISTRY_KEY_MAX );
                strncpy( registryConfig[entryNum].key, (const char *)key, TC_REGISTRY_KEY_MAX );
                strncpy( registryConfig[entryNum].value, (const char *)value, TC_REGISTRY_VALUE_MAX );
                ++entryNum;
                //
            }
            else
            {
                count = entryNum;
                // Destroy prepared statement object
                if ( prepStmt != NULL )
                {
                    sqlite3_finalize( prepStmt );
                }
                TRACE_EXIT;
                return( TCDBTRUNCATE );
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            count = entryNum;
            break;
        }
        else
        { 
            if ( TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST) )
            {
                trace_printf("%s@%d step failed, retrieving process registry"
                             " data, error=%s (%d)\n",
                             method_name, __LINE__, sqlite3_errmsg(db_), rc);
            }

            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                    , method_name, sqlStmt, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetUniqueString( int nid, int id, const char *uniqStr )
{
    const char method_name[] = "CTcdbSqlite::GetUniqueString";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int rc;
    const char *sqlStmt;
    sqlite3_stmt *prepStmt;

    // Read process configuration registry entries and populate in-memory
    // structures.
    sqlStmt = "select dataValue from monRegUniqueStrings where nid = ? and id = ?";
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt, NULL);

    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_int ( prepStmt, 1, nid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(nid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int ( prepStmt, 2, id );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(id) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );

        if ( rc == SQLITE_ROW )
        {
            const unsigned char *value;
    
            value = sqlite3_column_text(prepStmt, 0);
            strncpy( (char *)uniqStr, (const char *)value, TC_UNIQUE_STRING_VALUE_MAX );

            if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
            {
                trace_printf("%s@%d retrieved unique string (%d, %d), "
                             "value=%s\n", method_name, __LINE__,
                             nid, id, uniqStr);
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBNOEXIST );
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, nid=%d, id=%d, error: %s\n"
                    , method_name, sqlStmt, nid, id,sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }
    
    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetUniqueStringId( int nid
                                  , const char *uniqStr
                                  , int &id )
{
    const char method_name[] = "CTcdbSqlite::GetUniqueStringId";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
    {
        trace_printf( "%s@%d Getting unique string id: nid=%d string=%s\n"
                    , method_name, __LINE__, nid, uniqStr);
    }

    int rc;
    const char * sqlStmt;
    sqlStmt = "select id from monRegUniqueStrings where nid = ? and dataValue = ?";

    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt,
                             NULL);

    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_int( prepStmt, 1, nid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(nid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_text( prepStmt, 2, uniqStr, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(uniqStr) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );

        if ( rc == SQLITE_ROW )
        {   // Found string in database, return id
            id = sqlite3_column_int (prepStmt, 0);

            if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
            {
                trace_printf("%s@%d found unique string id: nid=%d, id=%d\n",
                             method_name, __LINE__, nid, id);
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBNOEXIST );
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, nid=%d, id=%d, error: %s\n"
                    , method_name, sqlStmt, nid, id,sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::GetUniqueStringIdMax( int nid, int &id )
{
    const char method_name[] = "CTcdbSqlite::GetUniqueStringIdMax";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    int result = 0;
    int rc;
    const char *sqlStmt;
    sqlStmt = "select max(id) from monRegUniqueStrings where nid=?";

    sqlite3_stmt * prepStmt;
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt,
                             NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_int( prepStmt, 1, nid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(nid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );

        if ( rc == SQLITE_ROW )
        {
            result = sqlite3_column_int(prepStmt, 0);
            id = result;
            if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST))
            {
                trace_printf("%s@%d found max(id)=%d for nid=%d in "
                             "monRegUniqueStrings\n", method_name, __LINE__,
                             result, nid);
            }
        }
        else if ( rc == SQLITE_DONE )
        {
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBNOEXIST );
        }
        else
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, nid=%d, error: %s\n"
                    , method_name, sqlStmt, nid, sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }
        
    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

void CTcdbSqlite::SetLNodeData( int nid
                              , int pnid
                              , const char *nodename
                              , int excfirstcore
                              , int exclastcore
                              , int firstcore
                              , int lastcore
                              , int processors
                              , int roles 
                              , TcNodeConfiguration_t &nodeConfig )
                                 
{
    const char method_name[] = "CTcdbSqlite::SetLNodeData";
    TRACE_ENTRY;

    if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
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

    if (TcIsRealCluster)
    {
        char short_node_name[TC_PROCESSOR_NAME_MAX];
        char str1[TC_PROCESSOR_NAME_MAX];
        char *tmpptr = NULL;
        tmpptr = (char *)nodename;

        while ( *tmpptr )
        { // Set to lowercase characters
            *tmpptr = (char)tolower( *tmpptr );
            tmpptr++;
        }

        // Extract the domain portion of the name if any
        memset( str1, 0, TC_PROCESSOR_NAME_MAX );
        memset( short_node_name, 0, TC_PROCESSOR_NAME_MAX );
        strcpy (str1, nodename );

        char *str1_dot = strchr( (char *) str1, '.' );
        if ( str1_dot )
        {
            memcpy( short_node_name, str1, str1_dot - str1 );
            // copy the domain portion and skip the '.'
            strcpy( nodeConfig.node_name, short_node_name );
            strcpy( nodeConfig.domain_name, str1_dot+1 );
        }
        else
        {
            strncpy( nodeConfig.node_name
                   , nodename
                   , sizeof(nodeConfig.node_name) );
            nodeConfig.domain_name[0] = 0;
        }
    }
    else
    {
        strncpy( nodeConfig.node_name
               , nodename
               , sizeof(nodeConfig.node_name) );
    }

    nodeConfig.nid  = nid;
    nodeConfig.pnid = pnid;
    nodeConfig.excluded_first_core = excfirstcore;
    nodeConfig.excluded_last_core = exclastcore;
    nodeConfig.first_core = firstcore;
    nodeConfig.last_core = lastcore;
    nodeConfig.processors = processors;
    nodeConfig.roles  = roles;

    if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d nid=%d, pnid=%d, node_name=%s, domain_name=%s, "
                      "excluded cores=(%d:%d),  cores=(%d:%d), "
                      "processors=%d, roles=%d\n"
                    , method_name, __LINE__
                    , nodeConfig.nid
                    , nodeConfig.pnid
                    , nodeConfig.node_name
                    , nodeConfig.domain_name
                    , nodeConfig.excluded_first_core
                    , nodeConfig.excluded_last_core
                    , nodeConfig.first_core
                    , nodeConfig.last_core
                    , nodeConfig.processors
                    , nodeConfig.roles );
    }

    TRACE_EXIT;
}

void CTcdbSqlite::SetPNodeData( int pnid
                              , const char *nodename
                              , int excfirstcore
                              , int exclastcore
                              , TcPhysicalNodeConfiguration_t &pnodeConfig )
                                 
{
    const char method_name[] = "CTcdbSqlite::SetLNodeData";
    TRACE_ENTRY;

    if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d pnid=%d, name=%s, excluded cores=(%d:%d)\n"
                    , method_name, __LINE__
                    , pnid
                    , nodename
                    , excfirstcore
                    , exclastcore );
    }

    if (TcIsRealCluster)
    {
        char short_node_name[TC_PROCESSOR_NAME_MAX];
        char str1[TC_PROCESSOR_NAME_MAX];
        char *tmpptr = NULL;
        tmpptr = (char *)nodename;

        while ( *tmpptr )
        {
            *tmpptr = (char)tolower( *tmpptr );
            tmpptr++;
        }

        // Extract the domain portion of the name if any
        memset( str1, 0, TC_PROCESSOR_NAME_MAX );
        memset( short_node_name, 0, TC_PROCESSOR_NAME_MAX );
        strcpy (str1, nodename );

        char *str1_dot = strchr( (char *) str1, '.' );
        if ( str1_dot )
        { // Set to lowercase characters
            memcpy( short_node_name, str1, str1_dot - str1 );
            // copy the domain portion and skip the '.'
            strcpy( pnodeConfig.node_name, short_node_name );
            strcpy( pnodeConfig.domain_name, str1_dot+1 );
        }
        else
        {
            strncpy( pnodeConfig.node_name
                   , nodename
                   , sizeof(pnodeConfig.node_name) );
            pnodeConfig.domain_name[0] = 0;
        }
    }
    else
    {
        strncpy( pnodeConfig.node_name
               , nodename
               , sizeof(pnodeConfig.node_name) );
    }

    pnodeConfig.pnid = pnid;
    pnodeConfig.excluded_first_core = excfirstcore;
    pnodeConfig.excluded_last_core = exclastcore;

    if ( TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d pnid=%d, node_name=%s, domain_name=%s, excluded cores=(%d:%d)\n"
                    , method_name, __LINE__
                    , pnodeConfig.pnid
                    , pnodeConfig.node_name
                    , pnodeConfig.domain_name
                    , pnodeConfig.excluded_first_core
                    , pnodeConfig.excluded_last_core );
    }

    TRACE_EXIT;
}

int CTcdbSqlite::SetPersistProcessData( const char       *persistkey
                                      , const char       *persistvalue
                                      , TcPersistConfiguration_t &persistConfig )
{
    const char method_name[] = "CTcdbSqlite::GetPersistProcessData";
    TRACE_ENTRY;

    char workValue[TC_PERSIST_KEY_MAX];
    char *pch;
    char *token1;
    char *token2;
    static const char *delimNone = "\0";
    static const char *delimComma = ",";

    if ( TcTraceSettings & (TC_TRACE_PERSIST | TC_TRACE_REQUEST) )
    {
        trace_printf( "%s@%d persistKey=%s, persistValue=%s\n"
                    , method_name, __LINE__
                    , persistkey, persistvalue );
    }
    
    strncpy( workValue, persistvalue, sizeof(workValue) );

    pch = (char *) strstr( persistkey, PERSIST_PROCESS_NAME_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.process_name
               , workValue
               , sizeof(persistConfig.process_name) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_PROCESS_TYPE_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.process_type
               , workValue
               , sizeof(persistConfig.process_type) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_PROGRAM_NAME_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.program_name
               , workValue
               , sizeof(persistConfig.program_name) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_PROGRAM_ARGS_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.program_args
               , workValue
               , sizeof(persistConfig.program_args) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_STDOUT_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.std_out
               , workValue
               , sizeof(persistConfig.std_out) );
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_REQUIRES_DTM );
    if (pch != NULL)
    {
        persistConfig.requires_DTM = (strcasecmp(workValue,"Y") == 0) 
                                    ? true : false;
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_RETRIES_KEY );
    if (pch != NULL)
    {
        // Set retries
        token1 = strtok( workValue, delimComma );
        if (token1)
        {
            persistConfig.persist_retries = atoi(token1);
        }
        // Set time window
        token2 = strtok( NULL, delimNone );
        if (token2)
        {
            persistConfig.persist_window = atoi(token2);
        }
        goto done;
    }
    pch = (char *) strstr( persistkey, PERSIST_ZONES_KEY );
    if (pch != NULL)
    {
        strncpy( persistConfig.persist_zones
               , workValue
               , sizeof(persistConfig.persist_zones) );
        goto done;
    }
    else
    {
        TRACE_EXIT;
        return( TCDBCORRUPT );
    }

done:

    TRACE_EXIT;
    return( TCSUCCESS );
}

int CTcdbSqlite::UpdatePNodeData( int         pnid
                                , const char *name
                                , int         excludedFirstCore
                                , int         excludedLastCore )
{
    const char method_name[] = "CTcdbSqlite::UpdatePNodeData";
    TRACE_ENTRY;

    if ( !IsInitialized() )  
    {
        if (TcTraceSettings & (TC_TRACE_REGISTRY | TC_TRACE_REQUEST | TC_TRACE_INIT))
        {
            trace_printf( "%s@%d Database is not initialized for access!\n"
                        , method_name, __LINE__);
        }
        TRACE_EXIT;
        return( TCNOTINIT );
    }
    
    if (TcTraceSettings & (TC_TRACE_NODE | TC_TRACE_REQUEST))
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
    rc = sqlite3_prepare_v2( db_, sqlStmt, static_cast<int>(strlen(sqlStmt)+1), &prepStmt, NULL);
    if ( rc != SQLITE_OK )
    {
        char buf[TC_LOG_BUF_SIZE];
        snprintf( buf, sizeof(buf)
                , "[%s] prepare (%s) failed, error: %s\n"
                , method_name, sqlStmt, sqlite3_errmsg(db_) );
        TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
        TRACE_EXIT;
        return( TCDBOPERROR );
    }
    else
    {
        rc = sqlite3_bind_text( prepStmt, 
                                1, 
                                name, -1, SQLITE_STATIC );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] sqlite3_bind_text(name) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt, 
                               2,
                               excludedFirstCore );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(excludedFirstCore) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt,
                               3, 
                               excludedLastCore );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(excludedLastCore) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
        rc = sqlite3_bind_int( prepStmt,
                               sqlite3_bind_parameter_index( prepStmt, ":pNid" ),
                               pnid );
        if ( rc != SQLITE_OK )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf),
                      "[%s] sqlite3_bind_int(:pNid) failed, error: %s\n"
                    , method_name,  sqlite3_errmsg(db_) );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_CRIT, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }

        rc = sqlite3_step( prepStmt );
        if (( rc != SQLITE_DONE ) && ( rc != SQLITE_ROW )
         && ( rc != SQLITE_CONSTRAINT ) )
        {
            char buf[TC_LOG_BUF_SIZE];
            snprintf( buf, sizeof(buf)
                    , "[%s] (%s) failed, error: %s\n"
                      " (nodeName=%s, excFirstCore=%d, excLastCore=%d) "
                      " where pNid=%d " 
                    , method_name, sqlStmt, sqlite3_errmsg(db_)
                    , name, excludedFirstCore, excludedLastCore, pnid );
            TcLogWrite( SQLITE_DB_ACCESS_ERROR, TC_LOG_ERR, buf );
            if ( prepStmt != NULL )
            {
                sqlite3_finalize( prepStmt );
            }
            TRACE_EXIT;
            return( TCDBOPERROR );
        }
    }

    if ( prepStmt != NULL )
    {
        sqlite3_finalize( prepStmt );
    }
    TRACE_EXIT;
    return( TCSUCCESS );
}

