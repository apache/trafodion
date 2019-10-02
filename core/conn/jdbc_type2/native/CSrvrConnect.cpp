/**************************************************************************
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
**************************************************************************/
//
// MODULE: CSrvrConnect.cpp
//
/* Change Log:
 * Methods Changed: addSrvrStmt(SRVR_STMT_HDL *); removeSrvrStmt(SRVR_STMT_HDL *); getSrvrStmt(const char *,long *,const char *)
 * Methods Added: getSrvrStmt(long,long,long *)
 */

#include <platform_ndcs.h>
#ifdef NSK_PLATFORM
#include <sqlWin.h>
#else
#include <sql.h>
#include <sys/syscall.h>
#endif
#include <sqlext.h>
#include "Debug.h"
#include "SrvrCommon.h"
#include "CommonDiags.h"
#include "CSrvrConnect.h"
#include "SqlInterface.h"
#include <string>
#include <map>
#include <vector>
#include <ext/hash_map>
using namespace std;

// +++ T2_REPO
#include <tr1/memory>
#include <pthread.h>
#include <PubQueryStats.h>

typedef struct _REPOS_STATS
{
    std::tr1::shared_ptr<SESSION_END> m_pSessionStats;
    std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> m_pQuery_stats;
    std::tr1::shared_ptr<SESSION_AGGREGATION> m_pAggr_stats;
    pub_struct_type m_pub_type;
}REPOS_STATS, *pREPOS_STATS;

extern void sendSessionEnd(std::tr1::shared_ptr<SESSION_END> pSession_info);
extern void sendQueryStats(pub_struct_type pub_type, std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> pQuery_info);
//

SRVR_CONNECT_HDL::SRVR_CONNECT_HDL()
{
    FUNCTION_ENTRY("SRVR_CONNECT_HDL::SRVR_CONNECT_HDL",(NULL));
    isClosed = TRUE;
    pSrvrStmtListHead = NULL;
    pCurrentSrvrStmt = NULL;
    count = 0;
    isSPJRS = 0;
    pSrvrStmtInternal = NULL;
    CLEAR_WARNING(sqlWarning);
    CLEAR_ERROR(sqlError);

    memset(DefaultCatalog, '\0', 129);
    memset(DefaultSchema, '\0', 129);
    memset(CurrentCatalog, '\0', 129);
    memset(CurrentSchema, '\0', 129);

    FUNCTION_RETURN_VOID((NULL));
}

SRVR_CONNECT_HDL::~SRVR_CONNECT_HDL()
{
    FUNCTION_ENTRY("SRVR_CONNECT_HDL::~SRVR_CONNECT_HDL",(NULL));
    sqlClose();
    cleanupSQLMessage();

    for (MapOfSrvrStmt::iterator iter = mapOfSrvrStmt.begin(); iter != mapOfSrvrStmt.end(); )
    {
        MapOfSrvrStmt::iterator current = iter;
        iter++;
        MEMORY_DELETE(current->second);
        mapOfSrvrStmt.erase(current);
    }

    FUNCTION_RETURN_VOID((NULL));
}

SQLRETURN SRVR_CONNECT_HDL::sqlConnect(const char *uid, const char *pwd)
{
    FUNCTION_ENTRY("SRVR_CONNECT_HDL::sqlConnect",("uid=%s,pwd=%s",
        DebugString(uid),
        DebugString(pwd)));

    SQLRETURN rc;

    cleanupSQLMessage();
    rc = CONNECT(this);
    switch (rc)
    {
    case SQL_SUCCESS:
        isClosed = FALSE;
        break;
    case SQL_SUCCESS_WITH_INFO:
        isClosed = FALSE;
        GETSQLWARNING(NULL, &sqlWarning);
        break;
    default:
        GETSQLERROR(NULL, &sqlError);
        break;
    }
    CLI_DEBUG_RETURN_SQL(rc);
}

SQLRETURN SRVR_CONNECT_HDL::sqlClose()
{
    FUNCTION_ENTRY("SRVR_CONNECT_HDL::sqlClose",(NULL));
    SQLRETURN rc;
    long      sqlcode;
    if (isClosed) CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);

    rc = switchContext(&sqlcode);
    if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    {
        // Might need to write an EMS event
        CLI_DEBUG_RETURN_SQL(SQL_SUCCESS);
    }

    cleanupSQLMessage();
    // We have to do this since the ptrToStmt->Close(SQL_DROP) will modify the listOfSrvrStmt.
    // First we put all the pointers in a tempContainer and just close them and clear the list and the tempContainer
    std::vector<SRVR_STMT_HDL*> vecOfStmts;
    //Stan's performance improvement changes
    for (MapOfInternalSrvrStmt::iterator iterOfStmt =
             mapOfInternalSrvrStmt.begin();
            ! (iterOfStmt == mapOfInternalSrvrStmt.end()); ++iterOfStmt)
        {
            SRVR_STMT_HDL* ptrToStmt = (SRVR_STMT_HDL *)iterOfStmt->second;
            vecOfStmts.push_back(ptrToStmt);
    }
    for(int nfor = 0; nfor < vecOfStmts.size(); ++nfor)
    {
        ((SRVR_STMT_HDL*)vecOfStmts[nfor])->InternalStmtClose(SQL_DROP);
    }
    vecOfStmts.clear();
    //Stan's performance improvement changes
//  for(__gnu_cxx::hash_map<long,SRVR_STMT_HDL*>::iterator iterOfStmt = mapOfSrvrStmt.begin(); ! (iterOfStmt == mapOfSrvrStmt.end()); ++iterOfStmt)
    for(MapOfSrvrStmt::iterator iterOfStmt = mapOfSrvrStmt.begin(); !(iterOfStmt == mapOfSrvrStmt.end()); ++iterOfStmt)
    {
        SRVR_STMT_HDL* ptrToStmt = (SRVR_STMT_HDL *)iterOfStmt->second;
        vecOfStmts.push_back(ptrToStmt);
    }
    for(int nfor = 0; nfor < vecOfStmts.size(); ++nfor)
    {
        ((SRVR_STMT_HDL*)vecOfStmts[nfor])->Close(SQL_DROP);
    }
    vecOfStmts.clear();

    //End of Soln. No.: 10-100202-7923
    count = 0;
    rc = DISCONNECT(this);
    switch (rc)
    {
    case SQL_SUCCESS:
        isClosed = TRUE;
        break;
    case SQL_SUCCESS_WITH_INFO:
        isClosed = TRUE;
        GETSQLWARNING(NULL, &sqlWarning);
        break;
    default:
        GETSQLERROR(NULL, &sqlError);
        break;
    }
    CLI_DEBUG_RETURN_SQL(rc);
}

SQLRETURN SRVR_CONNECT_HDL::switchContext(long *sqlcode)
{
    FUNCTION_ENTRY("SRVR_CONNECT_HDL::switchContext",("sqlcode=0x%08x",sqlcode));
    SQLRETURN rc;

    cleanupSQLMessage();
    if (isClosed)           // Need to populdate the error
        CLI_DEBUG_RETURN_SQL(SQL_ERROR);
    rc = SWITCHCONTEXT(this, sqlcode);
    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        break;
    default:
        GETSQLERROR(NULL, &sqlError);
        break;
    }
    CLI_DEBUG_RETURN_SQL(rc);
}

void SRVR_CONNECT_HDL::cleanupSQLMessage()
{
    FUNCTION_ENTRY("SRVR_CONNECT_HDL::cleanupSQLMessage",(NULL));
    unsigned long i;
    ERROR_DESC_def *errorDesc;

    // Cleanup SQLWarning
    if (sqlWarning._buffer)
    {
        DEBUG_ASSERT(sqlError.errorList._buffer!=sqlWarning._buffer,
            ("sqlError and sqlWarning buffers (0x%08x) are the same",sqlError.errorList._buffer));

        for (i = 0 ; i < sqlWarning._length; i++)
        {
            errorDesc = (ERROR_DESC_def *)sqlWarning._buffer + i;
            DEBUG_OUT(DEBUG_LEVEL_MEM,("errorDesc = (ERROR_DESC_def *) sqlWarning._buffer[%ld] (0x%08x)",i,errorDesc));
            DEBUG_ASSERT(errorDesc!=NULL,("sqlWarning._buffer + %ld is NULL",i));
            MEMORY_DELETE(errorDesc->errorText);
        }
        MEMORY_DELETE(sqlWarning._buffer);
        sqlWarning._length = 0;
    }

    // Cleanup sqlErrror
    for (i = 0 ; i < sqlError.errorList._length && sqlError.errorList._buffer != NULL ; i++)
    {
        errorDesc = (ERROR_DESC_def *)sqlError.errorList._buffer + i;
        DEBUG_OUT(DEBUG_LEVEL_MEM,("errorDesc = (ERROR_DESC_def *) sqlError.errorList._buffer[%ld] (0x%08x)",i,errorDesc));
        DEBUG_ASSERT(errorDesc!=NULL,("sqlError.errorList._buffer + %ld is NULL",i));
        MEMORY_DELETE(errorDesc->errorText);
        MEMORY_DELETE(errorDesc->Param1);
        MEMORY_DELETE(errorDesc->Param2);
        MEMORY_DELETE(errorDesc->Param3);
        MEMORY_DELETE(errorDesc->Param4);
        MEMORY_DELETE(errorDesc->Param5);
        MEMORY_DELETE(errorDesc->Param6);
        MEMORY_DELETE(errorDesc->Param7);
    }
    MEMORY_DELETE(sqlError.errorList._buffer);
    sqlError.errorList._length = 0;
    FUNCTION_RETURN_VOID((NULL));
}

const ERROR_DESC_LIST_def *SRVR_CONNECT_HDL::getSQLError()
{
    FUNCTION_ENTRY("SRVR_CONNECT_HDL::getSQLError",(NULL));
    FUNCTION_RETURN_PTR((const ERROR_DESC_LIST_def *)&sqlError.errorList,(NULL));
}

void SRVR_CONNECT_HDL::addSrvrStmt(SRVR_STMT_HDL *pSrvrStmt,BOOL internalStmt)
{
    FUNCTION_ENTRY("SRVR_CONNECT_HDL::addSrvrStmt",("pSrvrStmt=0x%08x",
        pSrvrStmt));

    mapOfSrvrStmt[(long)pSrvrStmt]=pSrvrStmt;

    if(internalStmt){
        mapOfInternalSrvrStmt[pSrvrStmt->stmtName]= pSrvrStmt;  // +++ map error
    }

    count++;
    //pSrvrStmt->myKey = count;
    FUNCTION_RETURN_VOID((NULL));
}

void SRVR_CONNECT_HDL::removeSrvrStmt(SRVR_STMT_HDL *pSrvrStmt)
{
    FUNCTION_ENTRY_LEVEL(DEBUG_LEVEL_STMT,"SRVR_CONNECT_HDL::removeSrvrStmt",("pSrvrStmt=0x%08x",
        pSrvrStmt));

#if defined(TAG64)
    int _ptr32 *tempPtr;
#endif
    //Soln. No.: 10-100202-7923

    /*SRVR_STMT_HDL_LIST *pSrvrStmtList;*/
    SRVR_STMT_HDL *lpSrvrStmt;
    //End of Soln. No.: 10-100202-7923

    if (pSrvrStmt->stmtType == INTERNAL_STMT)
        mapOfInternalSrvrStmt.erase(pSrvrStmt->stmtName);

    if(mapOfSrvrStmt.find((long)pSrvrStmt) != mapOfSrvrStmt.end())
    {
        lpSrvrStmt=(SRVR_STMT_HDL * )mapOfSrvrStmt.find((long)pSrvrStmt)->second;
        if (lpSrvrStmt == pSrvrStmt)
        {
#if defined(TAG64)
            if(tempStmtIdMap.find(lpSrvrStmt->stmt.tag) != tempStmtIdMap.end())
            {

                tempPtr=(int _ptr32*)lpSrvrStmt->stmt.tag;
                //printf("The value is frestmt  %ld %p\n",tempPtr,lpSrvrStmt);
                tempStmtIdMap.erase(lpSrvrStmt->stmt.tag);
                free32(tempPtr);
            }
#endif
            // If the statement being deleted is current statement, reset the current statement
            if (pCurrentSrvrStmt == lpSrvrStmt)
            {
                pCurrentSrvrStmt = NULL;
            }
            mapOfSrvrStmt.erase((long)pSrvrStmt);
            MEMORY_DELETE_OBJ(lpSrvrStmt);
        }

        count--;
    }
    FUNCTION_RETURN_VOID((NULL));
}

SRVR_STMT_HDL *SRVR_CONNECT_HDL::createSrvrStmt(
    const char *stmtLabel,
    long    *sqlcode,
    const char *moduleName,
    long moduleVersion,
    long long moduleTimestamp,
    short   sqlStmtType,
    BOOL    useDefaultDesc,
    BOOL    internalStmt,
    long stmtId,
    short sqlQueryType,      // If sqlQueryType indicates it's SPJ statement, then the member variable
    Int32  resultSetIndex,   // resultSetIndex and callStmtId of SRVR_STMT_HDL should be initialized
    SQLSTMT_ID* callStmtId)  // with these passed in args
{
    FUNCTION_ENTRY("SRVR_CONNECT_HDL::createSrvrStmt",("..."));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  stmtLabel=%s, sqlcode=0x%08x",
        DebugString(stmtLabel),
        sqlcode));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  moduleName=%s",
        DebugString(moduleName)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  moduleVersion=%ld, moduleTimestamp=%s",
        moduleVersion,
        DebugTimestampStr(moduleTimestamp)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  sqlStmtType=%s, useDefaultDesc=%d",
        CliDebugSqlStatementType(sqlStmtType),
        useDefaultDesc));

    SQLRETURN rc;
    SRVR_STMT_HDL *pSrvrStmt;
    long internalStmtid=0;

    pSrvrStmt = NULL;
    if(internalStmt)
    {
        //Stan's performance improvement changes
        MapOfInternalSrvrStmt::iterator iterOfStmtId
                   = mapOfInternalSrvrStmt.find(stmtLabel);
           if( !(iterOfStmtId == mapOfInternalSrvrStmt.end()) )
        pSrvrStmt =(SRVR_STMT_HDL *)iterOfStmtId->second;
           if(pSrvrStmt != NULL)
        FUNCTION_RETURN_PTR(pSrvrStmt,(NULL));
    }
    else
        pSrvrStmt = getSrvrStmt(0,stmtId,0);

    if (pSrvrStmt == NULL)
    {
        MEMORY_ALLOC_OBJ(pSrvrStmt,SRVR_STMT_HDL((long)this));

        if (sqlQueryType == SQL_SP_RESULT_SET)
        {
            pSrvrStmt->sqlQueryType   = SQL_SP_RESULT_SET;
            pSrvrStmt->callStmtId     = callStmtId;
            pSrvrStmt->resultSetIndex = resultSetIndex;
        }

        rc = pSrvrStmt->allocSqlmxHdls(stmtLabel, moduleName, moduleTimestamp,
            moduleVersion, sqlStmtType, useDefaultDesc);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        {
            MEMORY_DELETE_OBJ(pSrvrStmt);
            if (sqlcode) *sqlcode = rc;
            FUNCTION_RETURN_PTR(NULL,("pSrvrStmt->allocSqlmxHdls returned %s",CliDebugSqlError(rc)));
        }
        addSrvrStmt(pSrvrStmt,internalStmt);
    }
    if (sqlcode) *sqlcode = SQL_SUCCESS;
    FUNCTION_RETURN_PTR(pSrvrStmt,(NULL));
}

SRVR_STMT_HDL *SRVR_CONNECT_HDL::createSpjrsSrvrStmt(
    SRVR_STMT_HDL *inpSrvrStmt,
    const char *RSstmtLabel,
    long    *sqlcode,
    const char *moduleName,
    long moduleVersion,
    long long moduleTimestamp,
    short   sqlStmtType,
    long    RSindex,
    const char *RSstmtName,
    BOOL    useDefaultDesc)
{
    FUNCTION_ENTRY("SRVR_CONNECT_HDL::createSrvrStmt",("..."));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  inpSrvrStmt=0x%08x , RSstmtLabel=%s, sqlcode=0x%08x",
        inpSrvrStmt,
        DebugString(RSstmtLabel),
        sqlcode));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  moduleName=%s",
        DebugString(moduleName)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  moduleVersion=%ld, moduleTimestamp=%s",
        moduleVersion,
        DebugTimestampStr(moduleTimestamp)));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,("  sqlStmtType=%s, useDefaultDesc=%d",
        CliDebugSqlStatementType(sqlStmtType),
        useDefaultDesc));
    DEBUG_OUT(DEBUG_LEVEL_ENTRY,(" RSindex=%ld,  RSstmtName=%s",
        RSindex,
        DebugString(RSstmtName)));

    SQLRETURN rc;
    SRVR_STMT_HDL *pSrvrStmt;
    SQLSTMT_ID  *callpStmt = &inpSrvrStmt->stmt;
    int retcode;

    pSrvrStmt = NULL;//getSrvrStmt(RSstmtLabel, sqlcode, moduleName);

        MEMORY_ALLOC_OBJ(pSrvrStmt,SRVR_STMT_HDL((long)this));

        rc = pSrvrStmt->allocSqlmxHdls_spjrs(callpStmt, RSstmtLabel, moduleName, moduleTimestamp,
            moduleVersion, sqlStmtType, useDefaultDesc, RSindex, RSstmtName);

        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
        {
            MEMORY_DELETE_OBJ(pSrvrStmt);
            if (sqlcode) *sqlcode = rc;
            FUNCTION_RETURN_PTR(NULL,("pSrvrStmt->allocSqlmxHdls_spjrs returned %s",CliDebugSqlError(rc)));
        }
        addSrvrStmt(pSrvrStmt);

    if (sqlcode) *sqlcode = SQL_SUCCESS;
    FUNCTION_RETURN_PTR(pSrvrStmt,(NULL));
}

//Soln. No. : 10-100202-7923
SRVR_STMT_HDL *SRVR_CONNECT_HDL::getSrvrStmt(long dialogueId,long stmtId,long   *sqlcode)
{
    FUNCTION_ENTRY("getSrvrStmt",("dialogueId=0x%08x, stmtId=0x%08x, sqlcode=0x%08x",
        dialogueId,
        stmtId,
        sqlcode));
    //Stan's performance improvement changes
    MapOfSrvrStmt::iterator iterOfStmtId = mapOfSrvrStmt.find(stmtId);

    if(! (iterOfStmtId == mapOfSrvrStmt.end()) )
        FUNCTION_RETURN_PTR(((SRVR_STMT_HDL *)iterOfStmtId->second),(NULL));

    FUNCTION_RETURN_PTR(NULL,(NULL));
}

SRVR_STMT_HDL *SRVR_CONNECT_HDL::getInternalSrvrStmt(long dialogueId, const char* stmtLabel, long *sqlcode)
{
    FUNCTION_ENTRY("getInternalSrvrStmt",("dialogueId=0x%08x, stmtLabel=0x%08x, sqlcode=0x%08x",
                dialogueId,
                stmtLabel,
                sqlcode));

    SRVR_STMT_HDL *pSrvrStmt = NULL;

    MapOfInternalSrvrStmt::iterator iterOfStmtId = mapOfInternalSrvrStmt.find(stmtLabel);
    if( !(iterOfStmtId == mapOfInternalSrvrStmt.end()) )
        pSrvrStmt =(SRVR_STMT_HDL *)iterOfStmtId->second;

    if(pSrvrStmt != NULL)
        FUNCTION_RETURN_PTR(pSrvrStmt,(NULL));
    else
    {
        *sqlcode = SQL_INVALID_HANDLE;
        FUNCTION_RETURN_PTR(NULL, ("getInternalSrvrStmt() failed to find the internal statement \"%s\"", stmtLabel));
    }
}

// +++ T2_REPO
void sendSessionEnd(std::tr1::shared_ptr<SESSION_END> pSession_info)
{
    REPOS_STATS session_stats;
    session_stats.m_pSessionStats = pSession_info;
    session_stats.m_pub_type = PUB_TYPE_SESSION_END;

/* +++ T2_REPO ToDo
    if (record_session_done)
    {
        //boost::thread thrd(&SessionWatchDog);
        pthread_t thrd;
        pthread_create(&thrd, NULL, SessionWatchDog, NULL);
    }
    repos_queue.push_task(session_stats);
*/
}

void sendAggrStats(pub_struct_type pub_type, std::tr1::shared_ptr<SESSION_AGGREGATION> pAggr_info)
{
    REPOS_STATS aggr_stats;
    aggr_stats.m_pAggr_stats = pAggr_info;
    aggr_stats.m_pub_type = pub_type;

/* +++ T2_REPO ToDo
    if (record_session_done)
    {
        //boost::thread thrd(&SessionWatchDog);
        pthread_t thrd;
        pthread_create(&thrd, NULL, SessionWatchDog, NULL);
    }
    repos_queue.push_task(aggr_stats);
*/
}

void sendQueryStats(pub_struct_type pub_type, std::tr1::shared_ptr<STATEMENT_QUERYEXECUTION> pQuery_info)
{
    REPOS_STATS query_stats;
    query_stats.m_pQuery_stats = pQuery_info;
    query_stats.m_pub_type = pub_type;

/* +++ T2_REPO ToDo
    if (record_session_done)
    {
        //boost::thread thrd(&SessionWatchDog);
        pthread_t thrd;
        pthread_create(&thrd, NULL, SessionWatchDog, NULL);
    }
    repos_queue.push_task(query_stats);
*/
}

// T2_REPO
