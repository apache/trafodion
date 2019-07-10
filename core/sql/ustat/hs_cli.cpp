/********************************************************************
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
********************************************************************/

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_cli.C
 * Description:  Classes for accessing tables using CLI calls.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's

#include <string.h>
#include <netdb.h>

#define HS_FILE "hs_cli"

#include "Platform.h"

#include "str.h"
#include "NumericType.h"
#include "CharType.h"
#include "DatetimeType.h"
#include "IntervalType.h"
#include "MiscType.h"
#include "Int64.h"
#include "cli_stdh.h"
#include "sql_id.h"
#include "ComMPLoc.h"
#include "CmpContext.h"
#include "CompException.h"
#include "hs_globals.h"
#include "hs_cli.h"
#include "hs_auto.h"
#include "hs_la.h"
#include "SqlParserGlobals.h"
#include "ComCextdecs.h"
#include "SchemaDB.h"
#include "exp_clause_derived.h"  // Definition of convDoIt.
#include "NAString.h"
#include "NLSConversion.h"
#include "SqlciError.h"
#include "ExpErrorEnums.h"
#include "CmpSeabaseDDL.h" // call to createHistogramTables
#include "ComMisc.h"   // to get ComTrafReservedColName

// -----------------------------------------------------------------------
// Class to deallocate statement and descriptor.
// -----------------------------------------------------------------------
class HSRefDesc {

public:

  HSRefDesc( SQLSTMT_ID *pstmt
                  , SQLDESC_ID *pdesc
                  )
    : ps_(pstmt), pd_(pdesc), allocatedS_(FALSE), allocatedD_(FALSE)
  {}

  ~HSRefDesc()
  {
    Lng32 error;
    if (allocatedD_)
      {
        error = SQL_EXEC_DeallocDesc(pd_);
        if (error)
          HSLogError(error);
      }

    if (allocatedS_)
      {
        error = SQL_EXEC_DeallocStmt(ps_);
        if (error)
          HSLogError(error);
      }
  }

  Int32 allocatedS_, allocatedD_;

private:

  SQLSTMT_ID *ps_;
  SQLDESC_ID *pd_;
};

// -----------------------------------------------------------------------
// DESCRIPTION: Call SQL CLI to execute a SQL statement. The caller is
//          responsible for all error checking and recovery.
// INPUTS:  stmt = statement descriptor for the SQL query
//          srcDesc = source descriptor pointing to the 
//               text string of SQL query.
//          doPrintPlan = if true, the plan for the query will be
//                printed after the statement is prepared but before
//                execution; if false we do a single CLI ExecDirect call
//          checkForMdam = if true, do a query on the Explain virtual table
//                to see if MDAM was used in the query being executed, and
//                display the result in the ulog.
// -----------------------------------------------------------------------

Lng32 HSExecDirect( SQLSTMT_ID * stmt
                  , SQLDESC_ID * srcDesc
                  , NABoolean doPrintPlan
                  , NABoolean checkForMdam
                  )
{
  Lng32 retcode = 0;

  HSLogMan *LM = HSLogMan::Instance();
  if ((doPrintPlan || checkForMdam) && LM->LogNeeded())
    {
      retcode = SQL_EXEC_Prepare(stmt, srcDesc);
      if (retcode >= 0) // ignore warnings
        {
          if (doPrintPlan)
            printPlan(stmt);
          if (checkForMdam)
            checkMdam(stmt);
          retcode = SQL_EXEC_ExecFetch(stmt,0,0);
        }
    }
  else
    {
      retcode = SQL_EXEC_ExecDirect(stmt, srcDesc, 0, 0);
    }

  return retcode;
}



// -----------------------------------------------------------------------
// DESCRIPTION: Execute a standalone dml/ddl statement.
// INPUTS:  dml = text string of SQL query.
//          sqlcode = the error to issue upon failure, or HS_WARNING if
//                errors should be suppressed.
//          rowsAffected, srcTabRowCount  = pointers (NULL by default) to
//                variables that rowcount info for query will be stored in.
//          errorToken = text string indicating major operation being
//                executed (within ustats, ...).  Used for err processing.
//          tabDef = pointer to HSTableDef of table affected (only used
//                when rowsAffected, srcTabRowCount are non NULL.
//          errorToIgnore = sqlcode of an inconsequential expected error
//                that should not disrupt execution, such as "schema already
//                exists" when executing a Create Schema statement. 0 indicates
//                there is no such expected error.
//          checkMdam = if TRUE, determine whether the query uses MDAM, and
//                include this information in the ulog.
//          inactivateErrorCatcher = TRUE if the caller already has an
//                HSErrorCatcher object active (that is, the caller wants
//                to capture diagnostics itself).
// Note: srcTabRowCount is an obsolete parameter. Its only function now
// is to control whether plan information should be kept. Later we can
// clean this out, replacing it with an NABoolean.
// -----------------------------------------------------------------------
Lng32 HSFuncExecQuery( const char *dml
                    , short sqlcode
                    , Int64 *rowsAffected
                    , const char *errorToken
                    , Int64 *srcTabRowCount
                    , const HSTableDef *tabDef
                    , short errorToIgnore
                    , NABoolean checkMdam
                    , NABoolean inactivateErrorCatcher
                    )
{
  Lng32 retcode;
  // The HSErrorCatcher captures any diagnostics when it goes out-of-scope,
  // unless it is inactivated (in which case it does nothing).
  HSErrorCatcher errorCatcher(retcode, sqlcode, errorToken, TRUE,
                              inactivateErrorCatcher);
  retcode = HSFuncExecQueryBody(dml,sqlcode,rowsAffected,errorToken,
                                srcTabRowCount != NULL,tabDef,errorToIgnore,checkMdam);
  HSHandleError(retcode);
  return retcode;
}

// -----------------------------------------------------------------------
// DESCRIPTION: This is the body of HSFuncExecQuery. It is pulled out
// as a separate function so it can also be used by 
// HSFuncExecTransactionalQueryWithRetry. Each of the callers has its
// own HSErrorCatcher object to capture diagnostics.
// -----------------------------------------------------------------------
Lng32 HSFuncExecQueryBody( const char *dml
                    , short sqlcode
                    , Int64 *rowsAffected
                    , const char *errorToken
                    , NABoolean printPlan
                    , const HSTableDef *tabDef
                    , short errorToIgnore
                    , NABoolean checkMdam
                    )
{
  HSLogMan *LM = HSLogMan::Instance();
  HSGlobalsClass *hs_globals = GetHSContext();
  if (!tabDef && hs_globals) tabDef = hs_globals->objDef;

  if (dml == NULL)
    {
      return 0;
    }

  LM->Log(dml);

  Lng32 retcode;
 
  SQLMODULE_ID module;
  SQLSTMT_ID stmt;
  SQLDESC_ID srcDesc;

  init_SQLMODULE_ID(&module);
  init_SQLCLI_OBJ_ID(&stmt);
  init_SQLCLI_OBJ_ID(&srcDesc);

  HSRefDesc tmp(&stmt, &srcDesc);

  module.module_name = 0;
  module.module_name_len = 0;
    // Allocate a SQL statement
   if (LM->LogNeeded() && (rowsAffected != NULL)) {
  init_SQLCLI_OBJ_ID(&stmt, SQLCLI_CURRENT_VERSION,
         stmt_name, &module, HS_FUNC_EXEC_QUERY_STMTID, 0,
         SQLCHARSETSTRING_ISO88591, (Lng32)strlen(HS_FUNC_EXEC_QUERY_STMTID), 0);
  }
  else {
    stmt.name_mode = stmt_handle;
    stmt.module = &module;
    stmt.identifier = 0;
    stmt.handle = 0;
  }
  retcode = SQL_EXEC_ClearDiagnostics(&stmt);
  HSHandleError(retcode);

  retcode = SQL_EXEC_AllocStmt(&stmt, 0);
  HSHandleError(retcode);
  tmp.allocatedS_ = TRUE;

  // Allocate a descriptor which will hold the SQL statement source
  srcDesc.name_mode = desc_handle;
  srcDesc.module    = &module;
  module.module_name = 0;
  module.module_name_len = 0;
  srcDesc.identifier = 0;
  srcDesc.handle = 0;
  retcode = SQL_EXEC_AllocDesc(&srcDesc, 1);
  HSHandleError(retcode);
  tmp.allocatedD_ = TRUE;

  retcode = SQL_EXEC_SetDescItem(&srcDesc, 1, SQLDESC_TYPE_FS,
                                  REC_BYTE_V_ANSI, 0);
  HSHandleError(retcode);
  retcode = SQL_EXEC_SetDescItem(&srcDesc, 1, SQLDESC_VAR_PTR,
                                 (Long)dml, 0);
  HSHandleError(retcode);
  retcode = SQL_EXEC_SetDescItem(&srcDesc, 1, SQLDESC_LENGTH,
                                  strlen(dml) + 1, 0);
  HSHandleError(retcode);

  // SQLDESC_CHAR_SET must be the last descriptor item set, otherwise
  // it may get reset by other calls to SQL_EXEC_SetDescItem().
  NAString charSet = ActiveSchemaDB()->getDefaults().getValue(ISO_MAPPING);
  NAString defCS   = ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET);
  retcode = SQL_EXEC_SetDescItem(&srcDesc, 1, SQLDESC_CHAR_SET,
                                 SQLCHARSETCODE_UTF8
                                 , 0);
  HSHandleError(retcode);
  retcode = SQL_EXEC_SetStmtAttr(&stmt, SQL_ATTR_PARENT_QID, 0,
            (char *)CmpCommon::context()->sqlSession()->getParentQid());
  HSHandleError(retcode);

  // execute immediate this statement
  retcode = HSExecDirect(&stmt, &srcDesc, printPlan, checkMdam);
  // If retcode is > 0 or sqlcode is HS_WARNING, then set to 0 (no error/ignore).
  if (retcode >= 0) retcode = 0;
  // If sqlcode is HS_WARNING, then this means failures should be returned as
  // warnings.  So, don't call HSHandleError, but rather return 0. Also return
  // 0 if we get an expected and inconsequential error.
  if ((sqlcode == HS_WARNING && retcode < 0) || retcode == errorToIgnore)
    retcode = 0;
  else
    HSHandleError(retcode);

  if (rowsAffected != NULL && tabDef != NULL)
    {
      // set up a descriptor to get the # of rows affected from
      // the SQL Diagnostics area
      Lng32 sql_item = SQLDIAG_ROW_COUNT;
      SQLDESC_ID rc_desc;
      SQLMODULE_ID mod_id;
      rc_desc.version      = SQLCLI_CURRENT_VERSION;
      rc_desc.name_mode    = desc_handle;
      rc_desc.module       = &mod_id;
      mod_id.module_name   = 0;
      rc_desc.identifier   = 0;
      rc_desc.handle       = 0;

      SQL_EXEC_AllocDesc(&rc_desc, 1);
      SQL_EXEC_SetDescItem(&rc_desc, 1, SQLDESC_TYPE_FS,
                           REC_BIN64_SIGNED, 0);
      SQL_EXEC_SetDescItem(&rc_desc, 1, SQLDESC_VAR_PTR,
                           (Long) rowsAffected, 0);
      // SQLDESC_CHAR_SET must be the last descriptor item set, otherwise
      // it may get reset by other calls to SQL_EXEC_SetDescItem().
      NAString charSet = ActiveSchemaDB()->getDefaults().getValue(ISO_MAPPING);
      NAString defCS   = ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET);
      SQL_EXEC_SetDescItem(&rc_desc, 1, SQLDESC_CHAR_SET,
                           SQLCHARSETCODE_UTF8
                           , 0);
      SQL_EXEC_GetDiagnosticsStmtInfo(&sql_item, &rc_desc);
      SQL_EXEC_DeallocDesc(&rc_desc);
    }
  return retcode;
}

// -----------------------------------------------------------------------
// DESCRIPTION: Execute a standalone dml/ddl statement within a 
//              locally-started and committed transaction, with retry.
//              Many Trafodion errors abort the transaction; in order
//              to retry statements we need to manage the transaction
//              here as well.
// INPUTS:  dml = text string of SQL query.
//          sqlcode = the error to issue upon failure, or HS_WARNING if
//                errors should be suppressed.
//          rowsAffected, srcTabRowCount  = pointers (NULL by default) to
//                variables that rowcount info for query will be stored in.
//          errorToken = text string indicating major operation being
//                executed (within ustats, ...).  Used for err processing.
//          tabDef = pointer to HSTableDef of table affected (only used
//                when rowsAffected, srcTabRowCount are non NULL.
//          errorToIgnore = sqlcode of an inconsequential expected error
//                that should not disrupt execution, such as "schema already
//                exists" when executing a Create Schema statement. 0 indicates
//                there is no such expected error.
//          checkMdam = if TRUE, determine whether the query uses MDAM, and
//                include this information in the ulog.
// Note: srcTabRowCount is an obsolete parameter. Its only function now
// is to control whether plan information should be kept. Later we can
// clean this out, replacing it with an NABoolean.
// -----------------------------------------------------------------------
Lng32 HSFuncExecTransactionalQueryWithRetry( const char *dml
                                           , short sqlcode
                                           , Int64 *rowsAffected
                                           , const char *errorToken
                                           , Int64 *srcTabRowCount
                                           , const HSTableDef *tabDef
                                           , short errorToIgnore
                                           , NABoolean checkMdam
                                           )
{
  HSLogMan *LM = HSLogMan::Instance();
  HSTranMan *TM = HSTranMan::Instance();
  HSGlobalsClass *hs_globals = GetHSContext();
  Lng32 retcode = 0;

  if (TM->InTransaction())
  {
    // a transaction is already in progress; can't do retry
    // as we don't know if there is other work already in the
    // transaction
    *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                        << DgString0("HSFuncExecTransactionalQueryWithRetry")
                        << DgString1("-9215")
                        << DgString2("Unexpected transaction in progress");
    retcode = -UERR_GENERIC_ERROR;
    return retcode;
  }

  // Note: We don't use the HSErrorCatcher object to capture diagnostic
  // information here. The reason is, HSErrorCatcher works by lexical scope,
  // but the place where diagnostics are available to us does not nicely
  // match lexical scope. Instead, we call HSFuncMergeDiags directly.
 
  // On very busy system, some "update statistics" implementation steps like
  // "COLLECT FILE STATISTICS" step in HSTableDef::collectFileStatistics()
  // may experience transient failures that may succeed if retried enough
  // times. Note that AQR may do some retries for us under the covers.
  Int32 centiSecs = getDefaultAsLong(USTAT_RETRY_DELAY);
  Int32 limit = getDefaultAsLong(USTAT_RETRY_LIMIT);
  for (Int32 retry = 0; retry <= limit; retry++)
  {  
    // start a transaction
    retcode = TM->Begin("Transaction for retryable Statement",TRUE);
    if (retcode == 0)
    {
      // execute the statement
 
      retcode = HSFuncExecQueryBody(dml, sqlcode, rowsAffected, errorToken,
                                srcTabRowCount != NULL, tabDef, errorToIgnore, checkMdam);

      // Figure out if we want to ignore certain conditions 

      // filter retcode for HSHandleError
      HSFilterWarning(retcode);
      // If retcode is > 0 then set to 0 (no error/ignore).
      if (retcode > 0)
        retcode = 0;
      // If sqlcode is HS_WARNING, then failures should be ignored. Also check
      // for specific error code to be ignored.
      if ((sqlcode == HS_WARNING && retcode < 0) || retcode == errorToIgnore)
        retcode = 0;

      // if successful, commit the transaction
      if (retcode == 0)
        retcode = TM->Commit(TRUE);

      // analyze any error from the statement or the commit to see
      // if we should retry
      if (retcode == 0)
        retry = limit+1; // exit retry loop on success
      if (retcode == -SQLCI_SYNTAX_ERROR)
        retry = limit+1; // don't retry statements with syntax errors
      else if (retcode)
      {
        ComDiagsArea diags(STMTHEAP);
        SQL_EXEC_MergeDiagnostics_Internal(diags); // copy CLI diags area
        if (diags.contains(-EXE_CANCELED))
          retry = limit+1; // don't retry canceled query
      }
    }

    // If we are on our last retry, capture any diagnostics from
    // the begin or statement failure. (If we don't capture them
    // now, they will be cleared out of the CLI when we do the
    // ROLLBACK below.)
    if (retcode && (retry >= limit))
    {
      HSFuncMergeDiags(sqlcode, errorToken, NULL, TRUE /* get CLI diags */);
    }

    // If we had an error (on the begin, the statement or the commit),
    // try rolling back the transaction. It might have already been
    // rolled back, in which case the Rollback method just ignores
    // the error.
    if (retcode)
    {
      TM->Rollback(TRUE);
      if (retry < limit)  // if there are more retries
        DELAY_CSEC(centiSecs); // wait before retrying
    }
  }
    
  if (sqlcode != HS_WARNING || retcode < 0) 
    HSHandleError(retcode);

  return retcode;
}



Lng32 HSFuncExecDDL( const char *dml
                  , short sqlcode
                  , Int64 *rowsAffected
                  , const char *errorToken
                  , const HSTableDef *tabDef
                  )
{
  Lng32 retcode = 0;
  HSGlobalsClass *hs_globals = GetHSContext();
  if (!tabDef && hs_globals) tabDef = hs_globals->objDef;
  if (!tabDef) return -1;

  retcode = HSFuncExecTransactionalQueryWithRetry(dml, sqlcode, 
                             rowsAffected, errorToken, NULL, tabDef);

  HSHandleError(retcode);

  return retcode;
}

Lng32 HSClearCLIDiagnostics()
{
  Lng32 retcode;
  retcode = SQL_EXEC_ClearDiagnostics(NULL);
  return retcode;
}

// Obtain any JNI diagnostic text stored in the CLI
const char * HSFuncGetJniErrorStr()
{
  return GetCliGlobals()->getJniErrorStr();
}

// -----------------------------------------------------------------------
// Create histogram tables if they don't exist.
// -----------------------------------------------------------------------
Lng32 CreateHistTables (const HSGlobalsClass* hsGlobal)
  {
    Lng32 retcode = 0;

    HSLogMan *LM = HSLogMan::Instance();
    if (LM->LogNeeded())
      {
        snprintf(LM->msg, sizeof(LM->msg), "Creating histogram tables for schema %s on demand.",
                         hsGlobal->catSch->data());
        LM->Log(LM->msg);
      }

    // do NOT check volatile tables
    if (hsGlobal->objDef->isVolatile()) return retcode;

    LM->StartTimer("Create histogram tables");
    NAString tableNotCreated;

    // Call createHistogramTables to create any table that does not yet exist.
    NAString histogramsLocation = getHistogramsTableLocation(hsGlobal->catSch->data(), FALSE);

    HSTranController TC("Create histogram tables.",&retcode);
    retcode = (CmpSeabaseDDL::createHistogramTables(NULL, histogramsLocation, TRUE, tableNotCreated)); 
    if (retcode < 0 && LM->LogNeeded())
      {
        snprintf(LM->msg, sizeof(LM->msg), "Create failed for table %s.",
                         tableNotCreated.data());
        LM->Log(LM->msg);
      }
    LM->StopTimer();

    HSFilterWarning(retcode);
    HSHandleError(retcode);
    hsGlobal->diagsArea.clear();

    return retcode;
 }

Lng32 CreateSeabasePersSamples(const HSGlobalsClass* hsGlobal)
  {
    HSLogMan *LM = HSLogMan::Instance();
    Lng32 retcode = 0;
    ComObjectName tableName(hsGlobal->hsperssamp_table->data());
    HSSqTableDef sampleDef(tableName, ANSI_TABLE);
    if (!sampleDef.objExists()) //DROP existing sample table
    {
      if (LM->LogNeeded())
        {
          snprintf(LM->msg, sizeof(LM->msg), "Creating %s table for schema %s on demand.",
                           HBASE_PERS_SAMP_NAME, hsGlobal->catSch->data());
          LM->Log(LM->msg);
        }

      retcode = CreateHistTables(hsGlobal);
    }
    return retcode;
}

/***********************************************/
/* METHOD:  HSSample create() member function  */
/* PURPOSE: Physically create sample table.    */
/*          This is a helper function to       */
/*          HSSample::make()                   */
/* RETCODE:  0 - successful                    */
/*           non-zero otherwise                */
/***********************************************/
Lng32 HSSample::create(NABoolean unpartitioned, NABoolean isPersSample)
  {
    makeTableName(isPersSample); // assigns 'sampleTable' name for MX/MP.
    Lng32 retcode = 0;
    retcode = create(sampleTable, unpartitioned, isPersSample);

    return retcode;
  }

Lng32 HSSample::create(NAString& tblName, NABoolean unpartitioned, NABoolean isPersSample)
  {
    Lng32 retcode = 0;
    NAString ddl;
    NAString dropstmt;
    NAString catName, schName, objName;
    hs_table_type tableType;

    NAString tempTabName = tblName;
    NAString userTabName = objDef->getObjectFullName();

    HSGlobalsClass *hs_globals = GetHSContext();

    NABoolean isNativeTable =  
      HSGlobalsClass::isNativeCat(objDef->getCatName(HSTableDef::EXTERNAL_FORMAT));

    NAString userLocation;
    ComObjectName *sampleName;
    NAString tableOptions = " WITHOUT LOB COLUMNS";
    HSTranMan *TM = HSTranMan::Instance();

    if (objDef->getObjectFormat() == SQLMX)
      {
        
        // Do not emit the WITH PARTITIONS clause for native table. 
        // Rather, the SALT USING clause will be used. 
        if ( !isNativeTable ) 
           tableOptions += " WITH PARTITIONS";
        if (hs_globals->hasOversizedColumns)
          {
            // We will be truncating some columns when populating the sample table,
            // trading off accuracy in UEC against performance. Add a clause to
            // the table options limiting column lengths to the desired maximum.
            tableOptions += " LIMIT COLUMN LENGTH TO ";
            char temp[20];  // long enough for 32-bit integer
            sprintf(temp,"%d",hs_globals->maxCharColumnLengthInBytes);
            tableOptions += temp;
          }

        ddl  = "CREATE TABLE ";
        ddl += tempTabName;
        ddl += " LIKE ";

        // is this an MV LOG table?
        if (objDef->getNameSpace() == COM_IUD_LOG_TABLE_NAME)
          {
            ddl += "TABLE (IUD_LOG_TABLE ";
            ddl += userTabName;
            ddl += ") ";
          }
        else
          {
            ddl += userTabName;
          }

        ddl += tableOptions;    
        tableType = ANSI_TABLE;
        sampleName = new(STMTHEAP) ComObjectName(tempTabName,
                                                 COM_UNKNOWN_NAME,
                                                 FALSE,
                                                 STMTHEAP);
      }
    else
      {
        ComMPLoc tempObj(tblName, ComMPLoc::FILE);
        tempTabName = tempObj.getMPName();

        ComMPLoc userObj(objDef->getObjectFullName(), ComMPLoc::FILE);
        userTabName = userObj.getMPName();
        sampleName = new(STMTHEAP) ComObjectName(tempObj.getSysDotVol(),
                                                 tempObj.getSubvolName(),
                                                 tempObj.getFileName(),
                                                 COM_UNKNOWN_NAME,
                                                 ComAnsiNamePart::INTERNAL_FORMAT,
                                                 STMTHEAP);
        tableType = GUARDIAN_TABLE;

        // Do not remove the NOPURGEUNTIL clause in the following CREATE LIKE
        // query. This clause is needed to override any existing NOPURGEUNTIL
        // clause in the SOURCE table. Without this clause, a future
        // nopurge date in the SOURCE table (nopurgeuntil is automatically
        // inherited via CREATE LIKE) would prevent the TEMP table from being
        // created or purged later.
        // NOPURGEUNTIL is based on the current local time.
        time_t t;
        char purgeDate[12];
        time(&t);
        strftime(purgeDate, 12, "%b %d %Y", localtime(&t));

        //The temporary table location and size is dependent on the base table's
        //primary partition. If the PP0(primary partition) is FORMAT1, then the
        //temporary table will be FORMAT1. If the PP0 is FORMAT2, then the temporary
        //table will be FORMAT2. In addition, the temporary table will share the same
        //volume as the primary partition.
        //FORMAT1: has a limit of 2GB
        //FORMAT2: has no limit, other that hardware constraints.

        // If a transaction is running, the table needs to be created as audited.
        // Otherwise, create table as non-audited.
        if (TM->InTransaction())
          tableOptions = " AUDIT ";
        else
          tableOptions = " NO AUDIT ";

        ddl  = "CREATE TABLE ";
        ddl += tempTabName;
        ddl += " LIKE ";
        ddl += userTabName;
        ddl += " NOPURGEUNTIL ";
        ddl += purgeDate;
        ddl += " CATALOG ";
        ddl += objDef->getCatalogLoc();
        ddl += tableOptions;

        //There is no storage limit on format2, key-sequence tables, therefore we
        //we want to allocate as big as possible. All others, including format 2,
        //entry-sequence tables, have a 2G limit
          ddl += HS_EXTENT_SIZE_MP_FMT1;
      }


    HSSqTableDef sampleDef(*sampleName, tableType);
    if (sampleDef.objExists()) //DROP existing sample table
      {
        // make the sample table droppable
        NAString droppableStmt;
        droppableStmt = "ALTER TABLE ";
        droppableStmt += tempTabName;
        droppableStmt += " DROPPABLE " ;
        retcode = HSFuncExecDDL(droppableStmt,
			    - UERR_GENERIC_ERROR,
                                NULL,
                                tempTabName, objDef);
        HSHandleError(retcode);


        dropstmt  = "DROP TABLE ";
        dropstmt += tempTabName;
        retcode = HSFuncExecDDL(dropstmt,
                                - UERR_UNABLE_TO_DROP_OBJECT,
                                NULL,
                                tempTabName);
        HSHandleError(retcode);
      }

    if (hs_globals && hs_globals->diagsArea.getNumber(DgSqlCode::ERROR_))
      hs_globals->diagsArea.deleteError(0);

    HSLogMan *LM = HSLogMan::Instance();
    if (LM->LogNeeded()) {
      snprintf(LM->msg, sizeof(LM->msg), "CREATE SAMPLE TABLE ddl text=%s", ddl.data());
      LM->Log(LM->msg);
    }

    retcode = HSFuncExecDDL(ddl,
                            - UERR_UNABLE_TO_CREATE_OBJECT,
                            NULL,
                            tempTabName, objDef);

    HSHandleError(retcode);

    //***********************************************************
    //The Persistent sample tables are in PUBLIC_ACCESS schema.
    //The objects in this schema have  grant to everyone all DML and
    //DDL privileges. This means that any user can drop the persistent
    //sample tables, alter the data in them or so forth. The mechanism
    //to "hide" these tables from all users except update statistics code
    // is to make the table not droppable and offline.
    //By making the table not droppable, no users can drop the tables
    //except Update stats code. When the tables are offline, users cannot
    //select or delete from it. A user has to set the internal CQD to allow
    //DML access on offline tables.
    //A user cannot  alter the table to droppable and drop it because
    //its been marked offline.
    //***********************************************************

    // If the table is not volatile, make the sample table 'not droppable' and
    // offline (no dml allowed). Skip this if makeAccessible_ is set; this happens
    // when the sample table name is specified by cqd USTAT_SAMPLE_TABLE_NAME_CREATE
    // so we can manipulate the sample table in tests.
    // Skip this Alter stmt for Trafodion, it is not supported (the table was left
    // as droppable in this case).
    if (objDef->getTblOrigin() == HSTableDef::SQ_TBL &&
        !objDef->isVolatile() && !makeAccessible_)
    {
      NAString notDroppableStmt;
      notDroppableStmt = "ALTER TABLE ";
      notDroppableStmt += tempTabName;
      notDroppableStmt += " NOT DROPPABLE " ;
      retcode = HSFuncExecDDL(notDroppableStmt,
                              -UERR_GENERIC_ERROR,
                              NULL,
                              tempTabName, objDef);
      HSHandleError(retcode);

      // make the sample table offline
      HSFuncExecQuery("CONTROL QUERY DEFAULT IS_DB_TRANSPORTER 'ON'");
      const Int32 hsALLOW_SPECIALTABLETYPE = 0x1;
      SQL_EXEC_SetParserFlagsForExSqlComp_Internal(hsALLOW_SPECIALTABLETYPE);

      NAString offlineStmt;
      offlineStmt = "ALTER TABLE ";
      offlineStmt += tempTabName ;
      offlineStmt += " OFFLINE ";

      retcode = HSFuncExecDDL(offlineStmt,
                              -UERR_GENERIC_ERROR,
                              NULL,
                              tempTabName, objDef);
      HSFuncExecQuery("CONTROL QUERY DEFAULT IS_DB_TRANSPORTER RESET");
      SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(hsALLOW_SPECIALTABLETYPE);
    }

    HSHandleError(retcode);
    return retcode;
  }

Lng32 CreateHistView (const HSGlobalsClass* hsGlobal)
  {
    Lng32 retcode = 0;
    NAString ddl;
    NAString viewName;


    if (hsGlobal->tableFormat == SQLMX)
      {
        NAString schVer(hsGlobal->objDef->getCatName(HSTableDef::EXTERNAL_FORMAT));
        schVer += ".HP_DEFINITION_SCHEMA";

        viewName = hsGlobal->catSch->data();
        viewName += ".HISTVIEW";

        ddl  = "CREATE VIEW ";
        ddl += viewName;
        ddl += " (TABLE_NAME, TABLE_UID, COLUMN_NAME, HIST_ID, INTERVAL_COUNT, ROWCOUNT, TOTAL_UEC, LOW, HI) AS SELECT";
        ddl += " SUBSTRING(O.OBJECT_NAME,1,20), TABLE_UID, SUBSTRING(C.COLUMN_NAME,1,20), CAST(H.HISTOGRAM_ID AS INTEGER), CAST(H.INTERVAL_COUNT AS INTEGER), CAST(H.ROWCOUNT AS INTEGER), CAST(H.TOTAL_UEC AS INTEGER), SUBSTRING(LOW_VALUE,1,50), SUBSTRING(HIGH_VALUE,1,50) FROM ";
        ddl += schVer;
        ddl += ".OBJECTS O, ";
        ddl += schVer;
        ddl += ".COLS C, ";
        ddl += hsGlobal->hstogram_table->data();
        ddl += " H WHERE O.OBJECT_UID = H.TABLE_UID AND O.OBJECT_UID = C.OBJECT_UID AND C.COLUMN_NUMBER = H.COLUMN_NUMBER";
      }
    else
      {
        viewName = hsGlobal->objDef->getCatalogLoc();
        viewName += ".HISTVIEW";
        ComMPLoc viewObj(viewName, ComMPLoc::FILE);
        viewName = viewObj.getMPName();

        NAString locName = viewObj.getSysDotVol();
        locName += ".";
        locName += viewObj.getSubvolName();

        ddl  = "CREATE VIEW ";
        ddl += viewName;
        ddl += " (TABLE_NAME, TABLE_UID, COLUMN_NAME, HIST_ID, INTERVAL_COUNT, ROWCOUNT, TOTAL_UEC, LOW, HI) AS SELECT";
        ddl += " T.TABLENAME, H.TABLE_UID, SUBSTRING(C.COLNAME FROM 1 FOR 20), H.HISTOGRAM_ID, H.INTERVAL_COUNT, H.ROWCOUNT, H.TOTAL_UEC, SUBSTRING(H.LOW_VALUE FROM 1 FOR 50), SUBSTRING(H.HIGH_VALUE FROM 1 FOR 50) FROM ";
        ddl += locName;
        ddl += ".TABLES T, ";
        ddl += locName;
        ddl += ".COLUMNS C, ";
        ddl += locName;
        ddl += ".HISTOGRM H WHERE T.CREATETIME = H.TABLE_UID AND T.TABLENAME = C.TABLENAME AND H.COLUMN_NUMBER = C.COLNUMBER SECURE \"UUUN\" CATALOG ";
        ddl += locName;
      }

    retcode = HSFuncExecDDL(ddl,
                            - UERR_UNABLE_TO_CREATE_OBJECT,
                            NULL,
                            viewName);
    HSHandleError(retcode);
    return retcode;
  }


/*****************************************************/
/* METHOD:  dropSample() static member function.     */
/*                                                   */
/* PURPOSE: Drop sample table sampTblName if it      */
/*          exists.                                  */
/*                                                   */
/* RETURN:  0 if successful,                         */
/*          non-zero if failure.                     */
/*****************************************************/
Lng32 HSSample::dropSample(NAString& sampTblName, HSTableDef *sourceTblDef)
  {
    Lng32 retcode = 0;
    NAString dropstmt, tempTabName;
    HSLogMan *LM = HSLogMan::Instance();

    if (sampTblName.length())
    {
      LM->Log("\tDROP SAMPLE TABLE");

      tempTabName = sampTblName;

      // If the table is not volatile, make the sample table 'droppable', except
      // in the case of Trafodion, which does not support this Alter statement.
      if (sourceTblDef->getTblOrigin() == HSTableDef::SQ_TBL &&
          !sourceTblDef->isVolatile())
      {
        // make the sample table droppable
        NAString droppableStmt;
        droppableStmt = "ALTER TABLE ";
        droppableStmt += tempTabName;
        droppableStmt += " DROPPABLE " ;
        retcode = HSFuncExecDDL(droppableStmt,
                                - UERR_GENERIC_ERROR,
                                NULL,
                                tempTabName, sourceTblDef);
        HSHandleError(retcode);
      }

      dropstmt  = "DROP TABLE ";
      dropstmt += tempTabName;
      retcode = HSFuncExecDDL(dropstmt,
                              - UERR_UNABLE_TO_DROP_OBJECT,
                              NULL,
                              tempTabName);
      HSHandleError(retcode);
    }
    HSGlobalsClass *hs_globals = GetHSContext();
    if (hs_globals)
    {
      hs_globals->sampleTableUsed = FALSE;
      hs_globals->samplingUsed = FALSE;
    }
    return retcode;
  }


/******************************************************/
/* METHOD:  HSPrologEpilog constructor                */
/* PURPOSE: Record transaction state when instance of */
/*          HSPrologEpilog is created. This info is   */
/*          used by the destructor to check for an    */
/*          unterminated transaction                  */
/******************************************************/
HSPrologEpilog::HSPrologEpilog(NAString scopeName)
  : scopeName_(scopeName),
    tranMan_(HSTranMan::Instance()),
    logMan_(HSLogMan::Instance())
{
  enteredWithTranInProgress_ = tranMan_->InTransaction();
}

/******************************************************/
/* METHOD:  HSPrologEpilog destructor                 */
/* PURPOSE: Compare current transaction state with    */
/*          the state when the object was created.    */
/*          The expectation is that they will be the  */
/*          same. If a transaction is in progress now */
/*          but was not when the object was created,  */
/*          a rollback is performed under the         */
/*          assumption that its termination was       */
/*          bypassed due to a thrown exception. An    */
/*          external transaction that was terminated  */
/*          during the life of this object causes a   */
/*          message to be logged.                     */
/******************************************************/
HSPrologEpilog::~HSPrologEpilog()
{
  NABoolean inTranNow = tranMan_->InTransaction();
  if (!enteredWithTranInProgress_ && inTranNow)
    {
      if (logMan_->LogNeeded())
        {
          snprintf(logMan_->msg, sizeof(logMan_->msg),
                   "Rolling back transaction started but not terminated during %s",
                   scopeName_.data());
          logMan_->Log(logMan_->msg);
        }
      tranMan_->Rollback();
    }
  else if (enteredWithTranInProgress_ && !inTranNow)
    {
      if (logMan_->LogNeeded())
        {
          snprintf(logMan_->msg, sizeof(logMan_->msg),
                   "External transaction terminated within dynamic scope of %s",
                   scopeName_.data());
          logMan_->Log(logMan_->msg);
        }
    }
}

/*****************************************************************************/
/* CLASS:   HSTranMan                                                        */
/* PURPOSE: Transaction manager for all of Update Statistics needs.          */
/* DEPENDENCY: Pre-compiled statements in SQLHIST.mdf                        */
/* NOTES:   This is a singleton class, which means that there could only be  */
/*          one instance of this class.                                      */
/*****************************************************************************/
THREAD_P HSTranMan* HSTranMan::instance_ = 0;
HSTranMan::HSTranMan()
         : transStarted_(FALSE), extTrans_(FALSE), retcode_(0)
  {}

/***********************************************/
/* METHOD:  Instance()                         */
/* PURPOSE: Returns the instance of the class. */
/* NOTES:   We need to instantiate using the   */
/*          contextHeap because we need this   */
/*          class to stay around through       */
/*          multiple statements.               */
/***********************************************/
HSTranMan* HSTranMan::Instance()
  {
    if (instance_ == 0)
      instance_ = new (GetCliGlobals()->exCollHeap()) HSTranMan;
    return instance_;
  }

/***********************************************/
/* METHOD:  Begin()                            */
/* PURPOSE: Starts a transaction               */
/* RETCODE:       0 - successful               */
/*                4 - transaction is running   */
/*          SQLCODE - severe error             */
/***********************************************/
Lng32 HSTranMan::Begin(const char *title,NABoolean inactivateErrorCatcher)
  {
    HSLogMan *LM = HSLogMan::Instance();

    // if an error occurred on a previous HSTransMan call, try to clean it up here
    if ((retcode_ < 0) && (!InTransaction()))
      retcode_ = 0;  // not in a transaction, so OK to try starting one

    if (retcode_ < 0)                              /*== ERROR HAD OCCURRED ==*/
      {
        if (LM->LogNeeded())
          {
            snprintf(LM->msg, sizeof(LM->msg), "BEGINWORK(%s) ***ERROR(%d)", title, retcode_);
            LM->Log(LM->msg);
          }
      }
    else
      {
                                                   /*== BEGIN TRANSACTION  ==*/
        if (NOT transStarted_ &&
            NOT (extTrans_ = ((SQL_EXEC_Xact(SQLTRANS_STATUS, 0) == 0) ? TRUE : FALSE)))
          {
            NAString stmtText = "BEGIN WORK";
            retcode_ = HSFuncExecQuery(stmtText.data(), - UERR_INTERNAL_ERROR, NULL,
                                       HS_QUERY_ERROR, NULL, NULL, 0, FALSE, inactivateErrorCatcher);
            if (retcode_ >= 0)
              {
                transStarted_ = TRUE;
                retcode_ = 0;

                if (LM->LogNeeded())
                  {
                    snprintf(LM->msg, sizeof(LM->msg), "BEGINWORK(%s)", title);
                    LM->Log(LM->msg);
                  }
              }
          }
        else
          {                                        /*== TRANSACTION RUNNING==*/
            retcode_ = HS_WARNING;
            if (extTrans_ && LM->LogNeeded())
              {
                snprintf(LM->msg, sizeof(LM->msg), "BEGINWORK(%s) ***transaction running", title);
                LM->Log(LM->msg);
              }
          }
      }
     return retcode_;
  }

/***********************************************/
/* METHOD:  Commit()                           */
/* PURPOSE: Commits any changes made during a  */
/*          transaction and ends the           */
/*          transaction.                       */
/* RETCODE:       0 - successful               */
/*                4 - no transaction running   */
/*                    or cannot commit user    */
/*                    transaction              */
/*          SQLCODE - severe error             */
/***********************************************/
Lng32 HSTranMan::Commit(NABoolean inactivateErrorCatcher)
  {
    HSLogMan *LM = HSLogMan::Instance();

    if (retcode_ < 0)                              /*== ERROR HAD OCCURRED ==*/
      {
        if (LM->LogNeeded())
          {
            snprintf(LM->msg, sizeof(LM->msg), "COMMITWORK(unstable state: %d)\n", retcode_);
            LM->Log(LM->msg);
          }
      }
    else
      {
        if (transStarted_)                         /*== COMMIT TRANSACTION ==*/
          {
            NAString stmtText = "COMMIT WORK";
            retcode_ = HSFuncExecQuery(stmtText.data(), - UERR_INTERNAL_ERROR, NULL,
                                       HS_QUERY_ERROR, NULL, NULL, 0, FALSE, inactivateErrorCatcher);

            // transaction has ended
            transStarted_ = FALSE;
            if (retcode_ >= 0) {
              retcode_ = 0;
              LM->Log("COMMITWORK()\n");
            } else {
              LM->Log("COMMIT WORK failed\n");
            }

          }
        else
          {                                      /*==NO TRANSACTION RUNNING==*/
                                                 /*==         OR           ==*/
            retcode_ = HS_WARNING;               /*== EXTERNAL TRANSACTION ==*/
            if (extTrans_)
              LM->Log("COMMITWORK(external transaction being used)\n");
            else
              LM->Log("COMMITWORK(no running transaction)\n");
          }
      }
    return retcode_;
  }

void HSTranMan::logXactCode(const char* title)
  {
    HSLogMan *LM = HSLogMan::Instance();

    if (LM->LogNeeded()) {
      LM->Log(title);
      Lng32 transCode = SQL_EXEC_Xact(SQLTRANS_STATUS, 0);
      char buf[80];
      snprintf(buf, sizeof(buf), "SQL_EXEC_Xact() code=%d\n", transCode);
      LM->Log(buf);
      snprintf(buf, sizeof(buf), "transStarted_ = %s, extTrans_ = %s, retcode_ = %d\n",
        transStarted_ ? "TRUE" : "FALSE", extTrans_ ? "TRUE" : "FALSE", retcode_);
      LM->Log(buf);   
    }
 }

/***********************************************/
/* METHOD:  Rollback()                         */
/* PURPOSE: Undoes any changes made during a   */
/*          transaction and ends the           */
/*          transaction.                       */
/* RETCODE:       0 - successful               */
/*                4 - no transaction running   */
/*                    or cannot rollback user  */
/*                    transaction              */
/*          SQLCODE - severe error             */
/***********************************************/
Lng32 HSTranMan::Rollback(NABoolean inactivateErrorCatcher)
  {
    HSLogMan *LM = HSLogMan::Instance();

    // if an error occurred on a previous HSTransMan call, try to clean it up here
    if ((retcode_ < 0) && (!InTransaction()))
      {
        retcode_ = 0;  // not in a transaction, and therefore no need to roll back
        if (LM->LogNeeded())
          {
            snprintf(LM->msg, sizeof(LM->msg), "ROBACKWORK(cleaned up; not in transaction)");
            LM->Log(LM->msg);
          }       
        return retcode_;
      }

    if (retcode_ < 0)                              /*== ERROR HAD OCCURRED ==*/
      {
        if (LM->LogNeeded())
          {
            snprintf(LM->msg, sizeof(LM->msg), "ROBACKWORK(unstable state: %d)", retcode_);
            LM->Log(LM->msg);
          }
      }
    else
      {
        if (transStarted_)                         /*==ROLLBACK TRANSACTION==*/
          {
            NAString stmtText = "ROLLBACK WORK";
            retcode_ = HSFuncExecQuery(stmtText.data(), - UERR_INTERNAL_ERROR, NULL,
                                       HS_QUERY_ERROR, NULL, NULL, 0, FALSE, inactivateErrorCatcher);
            // transaction has ended
            transStarted_ = FALSE;
            if (retcode_ < 0)
              {
                // The rollback may have failed because the Executor already
                // aborted the transaction.
                if (!InTransaction())
                  retcode_ = 0;  // just ignore the error
                else
                  {
                    snprintf(LM->msg, sizeof(LM->msg), "ROBACKWORK failed, retcode_: %d)", retcode_);
                    LM->Log(LM->msg);
                  }
              }
            if (retcode_ >= 0) 
              {
                retcode_ = 0;
                LM->Log("ROLLBACK()");
              }
          }
        else
          {                                      /*==NO TRANSACTION RUNNING==*/
                                                 /*==         OR           ==*/
            retcode_ = HS_WARNING;               /*== EXTERNAL TRANSACTION ==*/
            if (extTrans_)
              LM->Log("ROBACKWORK(external transaction being used)");
            else
              LM->Log("ROBACKWORK(no running transaction)");
          }
      }
    return retcode_;
  }

/***********************************************/
/* METHOD:  InTransaction()                    */
/* PURPOSE: Tells you if there is a transaction*/
/*          currently running. It does not     */
/*          consider if the transaction was    */
/*          started by user or instance.       */
/* RETCODE: TRUE  - transaction is running     */
/*          FALSE - no transaction running     */
/***********************************************/
NABoolean HSTranMan::InTransaction()
  {
    return (SQL_EXEC_Xact(SQLTRANS_STATUS, 0) == 0);
  }

/***********************************************/
/* METHOD:  HSTranController constructor       */
/* PURPOSE: Instantiates an HSTranController   */
/*          object, which also begins a        */
/*          transaction. If returnCodePtr is   */
/*          not NULL, then the value at that   */
/*          address when the destructor is     */
/*          called determines whether the      */
/*          transaction is committed or rolled */
/*          back. returnCodePtr is also used   */
/*          to indicate the status of the      */
/*          begintran done in the ctor.  This  */
/*          value should be checked upon ctor  */
/*          return.                            */
/***********************************************/
HSTranController::HSTranController(const char* title,
                                   Lng32* returnCodePtr)
  : tranMan_(HSTranMan::Instance()),
    logMan_(HSLogMan::Instance()),
    title_(title),
    returnCodePtr_(returnCodePtr)
{
  Lng32 retcode = tranMan_->Begin(title);
  if (retcode == 0)
    {
      startedTrans_ = TRUE;
      if (logMan_->LogNeeded())
        logMan_->LogTimestamp("Transaction started");
    } else
      if (logMan_->LogNeeded())
        logMan_->LogTimestamp("HSTranController: Transaction fail to start");
  if (returnCodePtr)
    *returnCodePtr = retcode;
}

/**************************************************/
/* METHOD:  HSTranController end                  */
/* PURPOSE: When an HSTranController is destroyed,*/
/*          its associated transaction is ended.  */
/**************************************************/
HSTranController::~HSTranController()
{  endTrans(); }

/**************************************************/
/* METHOD:  HSTranController end                  */
/* PURPOSE: If the return code address given to   */
/*          the ctor points to a negative value,  */
/*          the transaction is rolled back, else  */
/*          it is committed.                      */
/**************************************************/
void HSTranController::endTrans()
{
  if (startedTrans_)
    {
      if (returnCodePtr_ && *returnCodePtr_ < 0)
        {
          tranMan_->Rollback();
          if (logMan_->LogNeeded())
            {
              snprintf(logMan_->msg, sizeof(logMan_->msg), "Transaction \"%s\" rolled back due to error %d",
                      title_.data(), *returnCodePtr_);
              logMan_->Log(logMan_->msg);
            }
        }
      else
        {
          tranMan_->Commit();
          if (logMan_->LogNeeded())
            logMan_->LogTimestamp("Transaction committed");
        }
    }
}

/***********************************************/
/* METHOD:  HSTranController stop/start        */
/* PURPOSE: Stops previous transaction and     */
/*          starts a new one.  The new one can */
/*          ended with the destructor or       */
/*          another call to stopStart().       */
/***********************************************/
void HSTranController::stopStart(const char* title)
{
  endTrans();
  Lng32 retcode = tranMan_->Begin(title);
  if (logMan_->LogNeeded() && retcode >= 0)
    logMan_->LogTimestamp("Transaction started");
}


/*****************************************************************************/
/* CLASS:   HSPersSamples                                                    */
/* FUNCTION: Handles persistent samples created for statistics needs.        */
/* NOTES:    This is a singleton class, which means that there could only be */
/*           one instance of this class.                                     */
/*****************************************************************************/
THREAD_P HSPersSamples* HSPersSamples::instance_ = 0;


HSPersSamples::HSPersSamples(const NAString & catalog, const NAString & schema) 
: catalog_(new (CTXTHEAP) NAString(catalog)), 
  schema_(new (CTXTHEAP) NAString(schema)),
  triedCreatingSBPersistentSamples_(false)
  {}

HSPersSamples::~HSPersSamples()
  {
    delete catalog_;
    delete schema_;
  }

void HSPersSamples::setCatalogSchema(const NAString &catalog,
                                     const NAString &schema)
  {
    if ((schema != *schema_) || (catalog != *catalog_))
      {
        *catalog_ = catalog;
        *schema_ = schema;
        triedCreatingSBPersistentSamples_ = false; // will try again on a new schema
      }
  }

//
// METHOD:  Instance()
// PURPOSE: Returns the instance of the class  */
//          and searches for or creates        */
//          SB_PERSISTENT_SAMPLES table (list).   */
//          For every catalog for which a      */
//          SB_PERSISTENT_SAMPLES table is known  */
//          to exist, the catalog name is added*/
//          to the 'persSampleTablesList'.     */
// RETCODE: 0 if no errors.                    */
//          non-zero otherwise.                */
// INPUT:   'catalog', the catalog of the table*/
//          for which a sample is created.     */
//          'createTable', if TRUE, create the */
//          SB_PERSISTENT_SAMPLES table for this  */
//          catalog if it doesn't exist.       */
// NOTES:   We need to instantiate using the   */
//          contextHeap because we need this   */
//          class to stay around through       */
//          multiple statements.
//
HSPersSamples* HSPersSamples::Instance(const NAString &catalog,
                                       const NAString &schema)
  {
    Lng32 retcode = 0;
    HSGlobalsClass *hs_globals = GetHSContext();

    if (instance_ == 0)
      instance_ = new (CTXTHEAP) HSPersSamples(catalog,schema);
    else
      {
        instance_->setCatalogSchema(catalog,schema);
      }

    if (HSGlobalsClass::isHiveCat(catalog))
      {
        NAString hiveStatsCatalog(HIVE_STATS_CATALOG);
        NAString hiveStatsSchema(HIVE_STATS_SCHEMA_NO_QUOTES);
        instance_->setCatalogSchema(hiveStatsCatalog,hiveStatsSchema);
      }
    else if (HSGlobalsClass::isNativeHbaseCat(catalog))
      {
        NAString hbaseStatsCatalog(HBASE_STATS_CATALOG);
        NAString hbaseStatsSchema(HBASE_STATS_SCHEMA_NO_QUOTES);
        instance_->setCatalogSchema(hbaseStatsCatalog,hbaseStatsSchema);
      }

    // Create the SB_PERSISTENT_SAMPLES table if it does not already exist
    // Do this at most once

    if ((instance_) && (!instance_->triedCreatingSBPersistentSamples_))
      {
        retcode = CreateSeabasePersSamples(hs_globals);
        instance_->triedCreatingSBPersistentSamples_ = true;
      }

    if (retcode) return 0;
    else         return instance_;
  }

/***********************************************/
/* METHOD:  find()                             */
/* PURPOSE: finds a                            */
/*          persistent sample table for UID    */
/*          and sample size. If a persistent   */
/*          sample is found, the name will be  */
/*          returned in 'table', or blank if   */
/*          not.                               */
/* INPUT:   uid - source table UID.            */
/*          sampleRows - the numner of rows a  */
/*            sample table should be close to  */
/*            in size.                         */
/*          allowedDiff - the fraction amount  */
/*            that a matching table may differ */
/*            from 'sampleRows'.               */
/* OUTPUT:  table - the name of a sample table */
/*            found or "" if none found.       */
/* RETCODE: 0 if no error during search.       */
/*          non-zero if error                  */
/***********************************************/
Lng32 HSPersSamples::find(HSTableDef *objDef, Int64 &actualRows, NABoolean isEstimate,
                         Int64 &sampleRows, double allowedDiff, NAString &table)
  {
    // Note: Earlier versions of this code supported multiple sample tables
    // (with different sample ratios) for the same base table, which seems
    // like overkill. So, in making this code work for Trafodion, we've made
    // a simplifying assumption: Assume just one sample table per base table.
    // (Indeed, we defined SB_PERSISTENT_SAMPLES to be keyed solely on the
    // base table UID.)
    //
    // So, this method now ignores the actualRows, isEstimate, sampleRows and
    // allowedDiff parameters. We can take them out later if this design
    // decision becomes permanent.

    Lng32 retcode = 0;
    NABoolean removedObsolete=FALSE;
    table="";  // in case no sample table is found
    NAWchar tempTabNameUCS2[408]; // first double byte is varchar length + a few extras NAWchars
    ComObjectName persSampTblObjName(*catalog_,
                                     *schema_,
                                     "SB_PERSISTENT_SAMPLES",
                                     COM_TABLE_NAME,
                                     ComAnsiNamePart::INTERNAL_FORMAT,
                                     STMTHEAP);
    NAString fromTable = persSampTblObjName.getExternalName();
    Int64 uid = objDef->getObjectUID();
    char uidStr[30];
    convertInt64ToAscii(uid,uidStr);

    HSCursor findSampleTableCursor1(STMTHEAP,"findSampleTable1");

    NAString query = "SELECT SAMPLE_NAME FROM ";
    query += fromTable;
    query += " WHERE TABLE_UID = ";
    query += uidStr;

    retcode = findSampleTableCursor1.prepareQuery(query, 0, 4); // no input parms, 1 output col
    HSLogError(retcode);

    retcode = findSampleTableCursor1.open();
    HSLogError(retcode);

    // expect at most one row will match the condition because of one
    // IUS sample table only assumption.

    // sel_stmt.fetch() will set 'tempTabName' to a persistent table name,
    // if one is found. 

    retcode = findSampleTableCursor1.fetch (1, (void *)&tempTabNameUCS2[0]);

    // Handle errors from fetch. If retcode is 100 (meaning no more data),
    // then set retcode = 0.
    if (retcode)
      {
        if (retcode != HS_EOF)
          {
            HSHandleError(retcode);
          }
        else
          {
            retcode = 0;
          }
      }
    else 
      { // fetch() successful.
        char tempTabNameUTF8[2*sizeof(tempTabNameUCS2)];
        char *dummyFirstUntranslatedChar;
        unsigned int outputDataLen;

        Int32 rc = UTF16ToLocale(cnv_version1,
                                 (char *) &tempTabNameUCS2[1],
                                 tempTabNameUCS2[0], // varchar length
                                 tempTabNameUTF8,
                                 sizeof(tempTabNameUTF8),
                                 cnv_UTF8,
                                 dummyFirstUntranslatedChar,
                                 &outputDataLen);
        // conversion UCS2 to UTF8 should always be successful
        HS_ASSERT(rc == 0 && outputDataLen >=0 && outputDataLen < sizeof(tempTabNameUTF8));
        tempTabNameUTF8[outputDataLen] = 0;
        table = tempTabNameUTF8;
     }

    // Don't overwrite the return code if an error has occurred, but attempt to
    // close the cursor anyway (in case it was successfully opened).
    if (retcode < 0)
      findSampleTableCursor1.close();
    else
      retcode = findSampleTableCursor1.close();

    HSTranMan * TM = HSTranMan::Instance();
    if (TM->InTransaction())
      TM->Commit();

    return retcode;
  }

/***********************************************/
/* METHOD:  find()                             */
/* PURPOSE: finds a persistent sample table for*/
/*          specified reason code.If found,    */
/*          the name will be returned in       */
/*          'table', or blank if not.          */
/* INPUT:   uid - source table UID.            */
/*          reason - (ignored for now)         */
/* OUTPUT:  table - the name of a sample table */
/*            found or "" if none found.       */
/*            requestedRows: requested rows    */
/*            sampleRows: sampled rows         */
/*            sampleRate: sample rate          */
/* RETCODE: 0 if no error during search.       */
/*          non-zero if error                  */
/***********************************************/
Lng32 HSPersSamples::find(HSTableDef *objDef, char reason,
                          NAString &table, Int64 &requestedRows,
                          Int64 &sampleRows, double &sampleRate)
  {
    Lng32 retcode = 0;
    NABoolean removedObsolete=FALSE;
    table="";
    NAWchar tempTabNameUCS2[408]; // first double byte is varchar length + a few extras NAWchars
    ComObjectName persSampTblObjName(*catalog_,
                                     *schema_,
                                     "SB_PERSISTENT_SAMPLES",
                                     COM_TABLE_NAME,
                                     ComAnsiNamePart::INTERNAL_FORMAT,
                                     STMTHEAP);
    NAString fromTable = persSampTblObjName.getExternalName();
    Int64 uid = objDef->getObjectUID();
    char uidStr[30];
    convertInt64ToAscii(uid,uidStr);

    // Create query to search for a matching persistent sample in
    // SB_PERSISTENT_SAMPLES table.

    HSCursor findSampleTableCursor(STMTHEAP,"findSampleTable");

    NAString query = "SELECT SAMPLE_NAME, REQUESTED_SAMPLE_ROWS, ACTUAL_SAMPLE_ROWS, SAMPLING_RATIO FROM ";
    query += fromTable;
    query += " WHERE TABLE_UID = ";
    query += uidStr;

    retcode = findSampleTableCursor.prepareQuery(query, 0, 4); // no input parms, 2 output cols
    HSLogError(retcode);

    retcode = findSampleTableCursor.open();
    HSLogError(retcode);

    // expect at most one row will match the condition because of one
    // IUS sample table only assumption.

      // sel_stmt.fetch() will set 'tempTabName' to a persistent table name,
      // requestedRows to requested rows, sampleRows to sampled rows, and
      // sampleRate to percent, if one is found.  Otherwise 'table'
      // is set to "" and requestdRows, sampleRows and sampleRate
      // are not set.

      retcode = findSampleTableCursor.fetch (4, (void *)&tempTabNameUCS2[0],
                                                 (void *)&requestedRows,
                                                 (void *)&sampleRows,
                                                 (void *)&sampleRate
                                             );

      // Handle errors from fetch. If retcode is 100 (meaning no more data),
      // then set retcode = 0.
      if (retcode)
      {
        if (retcode != HS_EOF)
        {
          HSHandleError(retcode);
        }
        else
        {
          retcode = 0;
        }
      }
      else { // fetch() successful.
        char tempTabNameUTF8[2*sizeof(tempTabNameUCS2)];
        char *dummyFirstUntranslatedChar;
        unsigned int outputDataLen;

        Int32 rc = UTF16ToLocale(cnv_version1,
                                 (char *) &tempTabNameUCS2[1],
                                 tempTabNameUCS2[0], // varchar length
                                 tempTabNameUTF8,
                                 sizeof(tempTabNameUTF8),
                                 cnv_UTF8,
                                 dummyFirstUntranslatedChar,
                                 &outputDataLen);
        // conversion UCS2 to UTF8 should always be successful
        HS_ASSERT(rc == 0 && outputDataLen >=0 && outputDataLen < sizeof(tempTabNameUTF8));
        tempTabNameUTF8[outputDataLen] = 0;
        table = tempTabNameUTF8;

      }

    // Don't overwrite the return code if an error has occurred, but attempt to
    // close the cursor anyway (in case it was successfully opened).
    if (retcode < 0)
      findSampleTableCursor.close();
    else
      retcode = findSampleTableCursor.close();

    return retcode;
  }

Lng32 HSPersSamples::removeSample(HSTableDef* tabDef, NAString& sampTblName,
                                  char reason, const char* txnLabel)
{
  Lng32 retcode = 0;
  HSTranMan *TM = HSTranMan::Instance();

  if (sampTblName.length() > 0)
    {
      // Delete row in persistent samples table regardless of whether sample
      // could be dropped.
      ComObjectName persSampTblObjName(*catalog_,
                                        *schema_,
                                        "SB_PERSISTENT_SAMPLES",
                                        COM_TABLE_NAME,
                                        ComAnsiNamePart::INTERNAL_FORMAT,
                                        STMTHEAP);
      NAString fromTable = persSampTblObjName.getExternalName();
      Int64 objUID = tabDef->getObjectUID();

      NAString dml((size_t)500/*allocate 500 bytes*/, STMTHEAP);
      dml.append("DELETE FROM ");
      dml += fromTable; // in UTF8
      dml += " WHERE TABLE_UID = ";
      dml += Int64ToNAString(objUID);
      // for now, the reason is ignored

      retcode = HSFuncExecTransactionalQueryWithRetry(dml, - UERR_INTERNAL_ERROR,
                                 NULL, txnLabel, NULL, NULL);
      HSSample::dropSample(sampTblName, tabDef);
      HSHandleError(retcode);
    }

  return retcode;
}

/****************************************************************/
/* METHOD:  createAndInsert()                                   */
/* PURPOSE: create a persistent sample table and insert an entry*/
/*          for it into list. Note that actualRows will modified*/
/*          if isEstimate = TRUE and the rows of original table */
/*          can be determined from statistics.                  */
/* INPUTS:  tabDef - a pointer to table struct.                 */
/*          isEstimate - TRUE if 'actualRows' is est. on input. */
/*          reason - sets REASON in SB_PERSISTENT_SAMPLES.      */
/* IN/OUTS: sampleRows - the desired size of sample on input,   */
/*            actual sample size on output.                     */
/*          actualRows - table size (possibly an est.) on input */
/*            actual table size on output if modified.          */
/* OUTPUTS: sampleName - the name of persistent sample table.   */
/* RETCODE: 0 if success                                        */
/*          non-zero otherwise                                  */
/****************************************************************/
Lng32 HSPersSamples::createAndInsert(HSTableDef *tabDef, NAString &sampleName,
                                    Int64 &sampleRows, Int64 &actualRows,
                                    NABoolean isEstimate, char reason,
                                    NABoolean createDandI,
                                    Int64 minRowCtPerPartition
                                    )
  {
    HSTranMan *TM = HSTranMan::Instance();
    HSCliStatement::statementIndex stmt;
    Int64 reqSampleRows = 0, objUID = 0;
    double percent = 0;
    NAString ddl, into_table;
    Lng32 retcode = 0;
    NABoolean isPersistent = TRUE;
    if (!tabDef || tabDef->setHasSyskeyFlag() != 0) return -1;
      // Sets flag in tabDef indicating primary key is SYSKEY.

    // If a persistent sample table already exists, raise an error telling
    // the user about it.

    Int64 dummy1, dummy2;
    double dummy3;
    NAString oldSampTblName;
    retcode = find(tabDef, reason, oldSampTblName,
                   dummy1, dummy2, dummy3);
    if ((retcode == 0) && (oldSampTblName.length() > 0))
      {
        retcode = -UERR_DROP_PERSISTANT_SAMPLE_FIRST;
        HSFuncMergeDiags(retcode);
        HSHandleError(retcode);
      }

    // Save original requested number of sample rows for inserting to SB_PERSISTENT_SAMPLES.
    reqSampleRows = sampleRows;

    Lng32 st = (CmpCommon::getDefault(USTAT_IUS_USE_PERIODIC_SAMPLING) == DF_ON) ?
                              SAMPLE_PERIODIC : SAMPLE_RAND_1;

    HSSample sample(tabDef, st, (double)sampleRows/(double)actualRows*100,
                    isPersistent, reason == 'I');

    // The last parameter in the make method indicates that it is a persistent sample
    // table being created for fetch count.
    // Create a partitioned persistent sample table for incremental update stats
    // (reason code is 'I').

    retcode = sample.make(isEstimate, sampleName, actualRows, sampleRows, TRUE,
                          TRUE, /* partitioned */
                          minRowCtPerPartition
                         );
      // sampleName output & actualRows will get modified if necessary
      //  (based on isEstimate).

    if (!retcode)
    {

      // Sample table successfully created, insert entry into
      // SB_PERSISTENT_SAMPLES table.  If unable to insert entry,
      // drop sample table.
      stmt = HSCliStatement::INSERT_PST;
      percent = ((float)sampleRows/(float)actualRows)*100;
      ComObjectName persSampTblObjName(*catalog_,
                                       *schema_,
                                       "SB_PERSISTENT_SAMPLES",
                                       COM_TABLE_NAME,
                                       ComAnsiNamePart::INTERNAL_FORMAT,
                                       STMTHEAP);
      into_table = persSampTblObjName.getExternalName();
      objUID = tabDef->getObjectUID();
      char timeStr[HS_TIMESTAMP_SIZE];
      hs_formatTimestamp(timeStr);

      NAString dml((size_t)500/*allocate 500 bytes*/, STMTHEAP);
      dml.append("INSERT INTO ");
      dml += into_table; // in UTF8
      dml += " VALUES(";
      dml += Int64ToNAString(objUID);
      dml += ",";
      dml += Int64ToNAString(reqSampleRows);
      dml += ",";
      dml += Int64ToNAString(sampleRows);
      dml += ",";
      dml += RealToNAString(percent);
      dml += ",TIMESTAMP'";
      dml += timeStr;
      dml += "',_ISO88591'";
      dml += reason;
      dml += "',_UCS2";
      NAString quotedSampleName(STMTHEAP);
      ToQuotedString ( quotedSampleName // out - NAString &quotedStr
                     , sampleName       // in  - const NAString &internalStr
                     , TRUE             // in  - NABoolean encloseInQuotes = TRUE
                     );
      dml += quotedSampleName; // in UTF8
      HSGlobalsClass *hs_globals = GetHSContext();
      ToQuotedString ( hs_globals->getWherePredicateForIUS()   // out - NAString &quotedStr
                     , ""               // in  - const NAString &internalStr
                     , TRUE             // in  - NABoolean encloseInQuotes = TRUE
                     );
      dml += ",_UCS2''";               // IUS_SEARCH_CONDITION (added after IUS stmt)
      dml += ",TIMESTAMP'1970-01-01 00:00:00'"; // UPDATE_START_TIME TIMESTAMP
      dml += ",''"; // UPDATER_INFO
      dml += ",_UCS2''";  // V1
      dml += ",_UCS2''";  // V2
      dml += ");";

      retcode = HSFuncExecTransactionalQueryWithRetry(dml, - UERR_INTERNAL_ERROR,
                                 NULL, HS_QUERY_ERROR, NULL, NULL);
      HSFilterWarning(retcode);  // can't do HSHandleError here since we want to do the drop    
      if (retcode)
        sample.drop();
      
      HSHandleError(retcode);
    }

    return retcode;
  }

/***********************************************/
/* METHOD:  remove()                           */
/* PURPOSE: Remove all manual (i.e., not IUS)  */
/*          persistent sample tables for       */
/*          the table with object uid 'uid'    */
/*          and within 'allowedDiff' fraction  */
/*          of 'sampleRows' in size.           */
/* INPUT:   uid - source table UID.            */
/*          sampleRows - the numner of rows a  */
/*            sample table should be close to  */
/*            in size.                         */
/*          allowedDiff - the fraction amount  */
/*            that a matching table may differ */
/*            from 'sampleRows' that will be   */
/*            removed.                         */
/* RETCODE: 0 if a table was found and removed */
/*          non-zero otherwise                 */
/* NOTE:    This function implements a non-    */
/*          published command of update stats. */
/*          Because of the transaction scope,  */
/*          it should not be used for functions*/
/*          that will run on a production      */
/*          system.                            */
/***********************************************/
Lng32 HSPersSamples::removeMatchingSamples(HSTableDef *tabDef,
                                           Int64 sampleRows,
                                           double allowedDiff)
  {
    Lng32 retcode = 0;
    NAString ddl, table, fromTable;
    NABoolean nothingToDrop = TRUE;

    // Loop until all persistent samples matching criteria have been removed.
    Int64 actualRows = -1; // Obsolete samples will not be removed by find().
    NABoolean isEstimate = TRUE;
    retcode = find(tabDef, actualRows, isEstimate, sampleRows, allowedDiff, table);
    ComObjectName persSampTblObjName(*catalog_,
                                     *schema_,
                                     "SB_PERSISTENT_SAMPLES",
                                     COM_TABLE_NAME,
                                     ComAnsiNamePart::INTERNAL_FORMAT,
                                     STMTHEAP);
    fromTable = persSampTblObjName.getExternalName();
    while (retcode == 0 && table != "")
    {
      // Drop persistent sample table and remove from list.
      nothingToDrop = FALSE;
      retcode = removeSample(tabDef, table, 'M', "");
      if (!retcode)
        retcode = find(tabDef, actualRows, isEstimate, sampleRows, allowedDiff, table);
    }

    if (nothingToDrop)
    {
      // we didn't drop anything; warn the user
      *CmpCommon::diags() << DgSqlCode(UERR_WARNING_NO_SAMPLE_TABLE);
    }  
 
    return retcode;
  }


/*********************************************************************/
/* METHOD:  readIUSUpdateInfo()                                      */
/* PURPOSE: Retrieve the UPDATE_START_TIME and UPDATER_INFO columns  */
/*          of the persistent samples table for the row identified   */
/*          by the UID of the passed HSTableDef.                     */
/* INPUT:   tblDef - Ptr to HSTableDef for table the sample is on.   */
/*          updHistory - Buffer to store update history value.       */
/*          updTimestamp - Receives update timestamp value.          */
/* RETCODE: Status code from the update operation.                   */
/*********************************************************************/
Lng32 HSPersSamples::readIUSUpdateInfo(HSTableDef* tblDef,
                                       char* updHistory,
                                       Int64* updTimestamp)
  {
    Lng32 retcode = 0;
    HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR, "readIUSUpdateInfo", TRUE);

    ComObjectName persSampTblObjName(*catalog_,
                                     *schema_,
                                     "SB_PERSISTENT_SAMPLES",
                                     COM_TABLE_NAME,
                                     ComAnsiNamePart::INTERNAL_FORMAT,
                                     STMTHEAP);
    NAString fromTable = persSampTblObjName.getExternalName();
    Int64 uid = tblDef->getObjectUID();
    char uidStr[30];
    convertInt64ToAscii(uid,uidStr);

    HSCursor readIUSInfoCursor(STMTHEAP,"readIUSUpdateInfo");

    NAString query = "SELECT UPDATER_INFO, "
                     "CAST(UPDATE_START_TIME - TIMESTAMP '1970-01-01 00:00:00' AS LARGEINT) FROM ";
    query += fromTable;
    query += " WHERE TABLE_UID = ";
    query += uidStr;

    retcode = readIUSInfoCursor.prepareQuery(query, 0, 2); // no input parms, 2 output cols
    HSLogError(retcode);

    retcode = readIUSInfoCursor.open();
    HSLogError(retcode);

    if (retcode == 0)
      {
        struct 
          {
            short len;
            char data[129];   // to fetch a varchar; UPDATER_INFO is VARCHAR(128)
          } buffer;

        retcode = readIUSInfoCursor.fetch(2, (void *)&buffer, (void *)updTimestamp);
        HSLogError(retcode);
        if (retcode == 0)
          {
            memmove(updHistory,buffer.data,buffer.len);
            updHistory[buffer.len] = '\0';  // to insure a null terminator
          }
      }

    // Don't overwrite a nonzero return code (error, warning, or HS_EOF) from
    // the fetch, but attempt to close the cursor anyway.
    if (retcode != 0)
      readIUSInfoCursor.close();
    else
      retcode = readIUSInfoCursor.close();
    return retcode;
  }


// helper function that doubles any single quotes in text, so that
// we can write SQL text into a SQL table
void doubleUpSingleQuotes(const char *text, NAString & result)
  {
    const char * next = text;
    char buf[2];  
    char doubledSingleQuote[3] = "''";

    buf[1] = '\0';

    while (*next)
      {
        if (*next == '\'')
          result += doubledSingleQuote;  // double up any single quote
        else
          {
            // NAString += with a char doesn't work; have to use a char[]
            buf[0] = *next;
            result += buf;
          }
        next++;
      }   
  }


/*********************************************************************/
/* METHOD:  updIUSUpdateInfo()                                       */
/* PURPOSE: Update the UPDATE_START_TIME and UPDATER_INFO columns of */
/*          the persistent samples table for the row identified by   */
/*          the UID of the passed HSTableDef.                        */
/* INPUT:   tblDef - Ptr to HSTableDef for table the sample is on.   */
/*          updHistory - Buffer containing updater info.             */
/*          updTimestamp - Char representation of update timestamp.  */
/*          updWhereCondition - if not null, the where predicate of  */
/*          the last completed IUS operation. (If null, we don't     */
/*          update this column.)                                     */
/*          requestedSampleRows - if non-null, points to expected #  */
/*          of rows in sample table (based on sampling rate).        */
/*          actualSampleRows - if non-null, actual # of rows.        */
/* RETCODE: Status code from the update operation.                   */
/*********************************************************************/
Lng32 HSPersSamples::updIUSUpdateInfo(HSTableDef* tblDef,
                                      const char* updHistory,
                                      const char* updTimestampStr,
                                      const char* updWhereCondition,
                                      const Int64* requestedSampleRows,
                                      const Int64* actualSampleRows)
{
  Lng32 retcode = 0;
  HSErrorCatcher errorCatcher(retcode, - UERR_INTERNAL_ERROR, "updIUSUpdateInfo", TRUE);
  HSTranMan *TM = HSTranMan::Instance();
  Int64 tblUID = tblDef->getObjectUID();


  TM->Begin("UPDATE THE UPDATE DATE AND HISTORY FOR PERSISTENT SAMPLE TABLE");

  // Update the update date and history in the row of the persistent samples
  // table for the passed table UID.
  ComObjectName persSampTblObjName(*catalog_,
                                   *schema_,
                                   "SB_PERSISTENT_SAMPLES",
                                   COM_TABLE_NAME,
                                   ComAnsiNamePart::INTERNAL_FORMAT,
                                   STMTHEAP);
  NAString updTable = persSampTblObjName.getExternalName();
  Int64 uid = tblDef->getObjectUID();
  char buf[30];

  HSCursor writeIUSInfoCursor(STMTHEAP,"writeIUSUpdateInfo");

  // The caller should have checked to see that the row existed first,
  // but even so it is possible (though unlikely) that someone could
  // have deleted the row in the meantime.

  NAString query = "UPDATE ";                   
  query += updTable;
  query += " SET UPDATE_START_TIME = TIMESTAMP '";
  query += updTimestampStr;
  query += "', UPDATER_INFO = '";
  query += updHistory;
  query += "'";
  if (updWhereCondition)
    {
      NAString doubledUp;
      doubleUpSingleQuotes(updWhereCondition,doubledUp /* out*/);
      query += ", LAST_WHERE_PREDICATE = ";
      if (strlen(updWhereCondition) > 250)
        {
          // let SQL deal with character truncation issues
          query += "SUBSTRING('";
          query += doubledUp;
          query += "' FOR 250)";
        }    
      else
        {
          query += "'";
          query += doubledUp;
          query += "'";
        }
    }
  if (requestedSampleRows)
    {
      convertInt64ToAscii(*requestedSampleRows, buf);
      query += ", REQUESTED_SAMPLE_ROWS = ";
      query += buf;
    }
  if (actualSampleRows)
    {
      convertInt64ToAscii(*actualSampleRows, buf);
      query += ", ACTUAL_SAMPLE_ROWS = ";
      query += buf;
    }
  query += " WHERE TABLE_UID = ";
  convertInt64ToAscii(uid,buf);
  query += buf;

  retcode = writeIUSInfoCursor.prepareQuery(query, 0, 0);
  HSLogError(retcode);

  retcode = writeIUSInfoCursor.open();
  HSLogError(retcode);

  if (retcode == 0)
    {
      retcode = writeIUSInfoCursor.fetch(0,0);
      if (retcode == 100)  // a successful UPDATE returns 100 on fetch
        retcode = 0;  
      HSLogError(retcode);
    }

  // Don't overwrite a nonzero return code (error, warning) from
  // the fetch, but attempt to close the cursor anyway.
  if (retcode != 0)
    writeIUSInfoCursor.close();
  else
    {
      retcode = writeIUSInfoCursor.close();
      if (retcode == 100) // a successful UPDATE returns 100 on close too
        retcode = 0;
    }

  if (retcode)
    TM->Rollback();
  else
    TM->Commit();

  return retcode;
}

/*****************************************************************************/
/* CLASS:   HSPersData - This class is modeled after HSPersSamples           */
/* FUNCTION: Stores data about CBFs created for statistics needs.            */
/* NOTES:    This is a singleton class, which means that there could only be */
/*           one instance of this class.                                     */
/*****************************************************************************/
THREAD_P HSPersData* HSPersData::instance_ = 0;
THREAD_P NAList<NAString>* HSPersData::persDataList_ = NULL;
THREAD_P NAString* HSPersData::catalog_ = NULL;
THREAD_P NAString* HSPersData::schema_ = NULL;
HSPersData::HSPersData()
  {}

HSPersData* HSPersData::Instance(const NAString &catalog)
  {
    Lng32 retcode = 0;
    if (instance_ == 0)
      instance_ = new (CTXTHEAP) HSPersData;

    if (persDataList_ == 0)
      persDataList_ = new (CTXTHEAP) NAList<NAString>(CTXTHEAP);
    if (catalog_ == 0)
      catalog_ = new (CTXTHEAP) NAString("");
    if (schema_ == 0)
      schema_  = new (CTXTHEAP) NAString("");

    // Search for an instance of catalog in memory resident list.
    if (HSGlobalsClass::isHiveCat(catalog))
      {
        *catalog_ = HIVE_STATS_CATALOG;
        *schema_  = HIVE_STATS_SCHEMA;
      }
    else
      {
        *catalog_ = catalog;
        *schema_  = "PUBLIC_ACCESS_SCHEMA";
      }
    if (!(*persDataList_).contains(*catalog_))
    {
      *(CmpCommon::diags()) << DgSqlCode(-4222)
                            << DgString0("Persistent Sample Table");
      return 0;
    }

    if (retcode) return 0;
    else         return instance_;
  }

Lng32 HSPersData::insert(NAString &tableName, ULng32 objectSubId, ULng32 seqNum, NAString &data)
  {
      Lng32 retcode = 0;
      HSTranMan *TM = HSTranMan::Instance();
      HSCliStatement::statementIndex stmt = HSCliStatement::INSERT_PDT;
      ComObjectName persDataTblObjName(*catalog_,
                                       *schema_,
                                       "PERSISTENT_DATA",
                                       COM_TABLE_NAME,
                                       ComAnsiNamePart::INTERNAL_FORMAT,
                                       STMTHEAP);

      NAString into_table = persDataTblObjName.getExternalName();

      NAWString *pTableNameInUTF16 =
        charToUnicode ( (Lng32) CharInfo::UTF8                 // char set of src str
                      , tableName/*in_utf8*/.data()           // src str
                      , (Int32)tableName/*in_utf8*/.length()  // src str len in bytes
                      , STMTHEAP                               // heap for allocated target str
                      );
      NAWString tableNameInUTF16(STMTHEAP);
      if (pTableNameInUTF16 != NULL && pTableNameInUTF16->length() > 0)
        tableNameInUTF16.append(*pTableNameInUTF16);
      delete pTableNameInUTF16; pTableNameInUTF16 = NULL;
      if (!tableName.isNull() && tableNameInUTF16.length() <= 0)
      {
        NAString str0((size_t)600/*allocate 600 bytes*/, STMTHEAP);
        str0.append("INSERT ");
        str0.append(tableName);
        *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                            << DgString0(str0.data())
                            << DgString1("-2109")
                            << DgString2("Unable to translate sample name from UTF8 to UCS2");
        // HSHandleError(-UERR_GENERIC_ERROR);
        return (-UERR_GENERIC_ERROR);
      }

      TM->Begin("INSERT INTO PERSISTENT_DATA TABLE.");
      HSCliStatement insertPDT(stmt,
                              (char *)into_table.data(),
                              (char *)tableNameInUTF16.data(),
                              (char *)&objectSubId,
                              (char *)&seqNum,
                              (char *)data.data());

      retcode = insertPDT.execFetch("INSERT_PDT " + into_table );
      HSHandleError(retcode);
      if (!retcode)
	TM->Commit();
      else
        TM->Rollback();
      return retcode;
  }

Lng32 HSPersData::remove(NAString &tableName, ULng32 objectSubId, ULng32 seqNum)
  {
    Lng32 retcode = 0;
    NAString fromTable;
    HSTranMan *TM = HSTranMan::Instance();
    HSCliStatement::statementIndex stmt = HSCliStatement::DELETE_PDT;
    ComObjectName persDataTblObjName(*catalog_,
                                     *schema_,
                                     "PERSISTENT_DATA",
                                     COM_TABLE_NAME,
                                     ComAnsiNamePart::INTERNAL_FORMAT,
                                     STMTHEAP);
    fromTable = persDataTblObjName.getExternalName();

    NAWString *pTableNameInUTF16 =
      charToUnicode ( (Lng32) CharInfo::UTF8                 // char set of src str
                    , tableName/*in_utf8*/.data()           // src str
                    , (Int32)tableName/*in_utf8*/.length()  // src str len in bytes
                    , STMTHEAP                               // heap for allocated target str
                    );
    NAWString tableNameInUTF16(STMTHEAP);
    if (pTableNameInUTF16 != NULL && pTableNameInUTF16->length() > 0)
      tableNameInUTF16.append(*pTableNameInUTF16);
    delete pTableNameInUTF16; pTableNameInUTF16 = NULL;
    if (!tableName.isNull() && tableNameInUTF16.length() <= 0)
    {
      NAString str0((size_t)600/*allocate 600 bytes*/, STMTHEAP);
      str0.append("INSERT ");
      str0.append(tableName);
      *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                          << DgString0(str0.data())
                          << DgString1("-2109")
                          << DgString2("Unable to translate sample name from UTF8 to UCS2");
      // HSHandleError(-UERR_GENERIC_ERROR);
      return (-UERR_GENERIC_ERROR);
    }

    TM->Begin("DELETE ROW FROM PERSISTENT_DATA TABLE.");
    HSCliStatement deletePDT(stmt,
                               (char *)fromTable.data(),
                               (char *)tableNameInUTF16.data(),
                               (char *)&objectSubId,
                               (char *)&seqNum);

    retcode = deletePDT.execFetch("DELETE_PDT " + fromTable );
    if (!retcode)
      TM->Commit();
     else
      TM->Rollback();
    return retcode;
  }

Lng32 HSPersData::fetch(NAString &tableName, ULng32 objectSubId, ULng32 seqNum, NAString &data)
  {
    Lng32 retcode = 0;
    NAString fromTable;
    HSCliStatement::statementIndex stmt = HSCliStatement::CURSOR_PDT;;
    ComObjectName persDataTblObjName(*catalog_,
                                     *schema_,
                                     "PERSISTENT_DATA",
                                     COM_TABLE_NAME,
                                     ComAnsiNamePart::INTERNAL_FORMAT,
                                     STMTHEAP);
    fromTable = persDataTblObjName.getExternalName();

    NAWString *pTableNameInUTF16 =
      charToUnicode ( (Lng32) CharInfo::UTF8                 // char set of src str
                    , tableName/*in_utf8*/.data()           // src str
                    , (Int32)tableName/*in_utf8*/.length()  // src str len in bytes
                    , STMTHEAP                               // heap for allocated target str
                    );
    NAWString tableNameInUTF16(STMTHEAP);
    if (pTableNameInUTF16 != NULL && pTableNameInUTF16->length() > 0)
      tableNameInUTF16.append(*pTableNameInUTF16);
    delete pTableNameInUTF16; pTableNameInUTF16 = NULL;
    if (!tableName.isNull() && tableNameInUTF16.length() <= 0)
    {
      NAString str0((size_t)600/*allocate 600 bytes*/, STMTHEAP);
      str0.append("INSERT ");
      str0.append(tableName);
      *CmpCommon::diags() << DgSqlCode(-UERR_GENERIC_ERROR)
                          << DgString0(str0.data())
                          << DgString1("-2109")
                          << DgString2("Unable to translate sample name from UTF8 to UCS2");
      // HSHandleError(-UERR_GENERIC_ERROR);
      return (-UERR_GENERIC_ERROR);
    }

    HSCliStatement cursorPDT(stmt,
                              (char *)fromTable.data(),
                              (char *)tableNameInUTF16.data(),
                              (char *)&objectSubId,
                              (char *)&seqNum);

    retcode = cursorPDT.open();
    HSHandleError(retcode);

    char *outputData;
    outputData = new STMTHEAP char[32008];
    retcode = cursorPDT.fetch (1, (void *)outputData);

    // Handle errors from fetch. If retcode is 100 (meaning no more data),
    // then set retcode = 0.
    if (retcode)
    {
      if (retcode != HS_EOF)
        HSHandleError(retcode);
    }
    else
    {
      short length = 0;
      char *vclenPtr = NULL;
      vclenPtr = (char *)&length;
      str_cpy_all(vclenPtr,outputData,sizeof(short));
      data = NAString(&outputData[sizeof(short)],length);
    }

    // Don't overwrite the return code if an error has occurred, but attempt to
    // close the cursor anyway (in case it was successfully opened).
    if (retcode < 0 || retcode == HS_EOF)
      cursorPDT.close();
    else
      retcode = cursorPDT.close();
    return retcode;
  }

// -----------------------------------------------------------------------
// Some global constants for HSCliStatement.
// -----------------------------------------------------------------------
static const char *const StatementNames[] = {
     "BEGINWORK"
   , "COMMITWORK"
   , "ROBACKWORK"
   , "PRINTPLAN"
   , "INSERT101_MP"         , "INSERT101_MX",       "INSERT101_MX_2300"
   , "INSERT201_MP"         , "INSERT201_MX",       "INSERT201_MX_2300"
   , "DELETE101_MP"         , "DELETE101_MX",       "DELETE101_MX_2300"
   , "DELETE201_MP"         , "DELETE201_MX",       "DELETE201_MX_2300"
   , "DELETE102_MP"         , "DELETE102_MX",       "DELETE102_MX_2300"
   , "DELETE202_MP"         , "DELETE202_MX",       "DELETE202_MX_2300"
   , "SECURITY101_MP"       , "SECURITY101_MX",     "SECURITY101_MX_2300"
   , "SECURITY201_MP"       , "SECURITY201_MX",     "SECURITY201_MX_2300"
   , "CURSOR101_MP"         , "CURSOR101_MX",       "CURSOR101_MX_2300"
   , "CURSOR101_NOMC_MP"    , "CURSOR101_NOMC_MX",  "CURSOR101_NOMC_MX_2300"
   , "CURSOR102_MP"         , "CURSOR102_MX",       "CURSOR102_MX_2300"
   , "CURSOR103_MP"         , "CURSOR103_MX",       "CURSOR103_MX_2300"
   , "CURSOR201_MP"         , "CURSOR201_MX",       "CURSOR201_MX_2300"
   , "SHOWHIST_MP"          , "SHOWHIST_MX",        "SHOWHIST_MX_2300"
   , "SHOWINT_MP"           , "SHOWINT_MX",         "SHOWINT_MX_2300"
   , "ROWCOUNT_FROM_STATS"  , "CURSOR104_MX_2300",  "INSERT104_MX_2300"
   , "CURSOR105_MX_2300"
   , "DELETE_PST"           , "INSERT_PST",         "CURSOR_PST"
   , "CURSOR_PST_REASON_CODE"
   , "CURSOR107_MX_2300"
   , "DELETE_PDT"           , "INSERT_PDT",         "CURSOR_PDT"
   , "CURSOR_PST_UPDATE_INFO", "UPDATE_PST_UPDATE_INFO"
   , "CURSOR103_MX_2300_X"
};


// -----------------------------------------------------------------------
// Constructor and destructor for HSCliStatement which runs static SQL.
// -----------------------------------------------------------------------
HSCliStatement::HSCliStatement( statementIndex ix,
                                char *  in01, char *  in02, char *  in03,
                                char *  in04, char *  in05, char *  in06,
                                char *  in07, char *  in08, char *  in09,
                                char *  in10, char *  in11, char *  in12,
                                char *  in13, char *  in14, char *  in15,
                                char *  in16, char *  in17, char *  in18,
                                char *  in19, char *  in20, char *  in21,
                                char *  in22, char *  in23, char *  in24,
                                char *  in25
                              )
  : pInputDesc_(NULL), validStmt_(FALSE),
    in01_(in01), in02_(in02), in03_(in03),
    in04_(in04), in05_(in05), in06_(in06),
    in07_(in07), in08_(in08), in09_(in09),
    in10_(in10), in11_(in11), in12_(in12),
    in13_(in13), in14_(in14), in15_(in15),
    in16_(in16), in17_(in17), in18_(in18),
    in19_(in19), in20_(in20), in21_(in21),
    in22_(in22), in23_(in23), in24_(in24),
    in25_(in25),
    retcode_(0)
{
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    {
      snprintf(LM->msg, sizeof(LM->msg), "!!>> %s", StatementNames[ix & 0xFFFF]);
      LM->Log(LM->msg);
    }

  static SQLMODULE_ID module;
  static char moduleName[HS_MODULE_LENGTH];
  static NABoolean moduleSet = FALSE;

  if (!moduleSet)
    {

      init_SQLMODULE_ID(&module);
      strncpy(moduleName, HS_MODULE, HS_MODULE_LENGTH);

      module.module_name = (char *)moduleName;
      module.module_name_len = strlen((char*)moduleName);
      module.creation_timestamp = 1234567890;
      moduleSet = TRUE;
    }


  init_SQLCLI_OBJ_ID(&stmt_);
  stmt_.name_mode = stmt_name;
  stmt_.module = &module;

  strncpy(stmtID_, StatementNames[ix & 0xFFFF], HS_STMTID_LENGTH);

  stmt_.identifier = (char *)stmtID_;
  stmt_.identifier_len = strlen((char *)stmtID_);

  stmt_.handle = 0;

  numVars_ = ((ix & 0x7FFF0000) >> 16);
  const Int32 PRESET_VARS_NUM = 18; // 1 + max number of return values (in sqlhist.mdf) + 1
  if (numVars_ < PRESET_VARS_NUM)
    {
      validStmt_ = TRUE;
      return;
    }

  HS_ASSERT(numVars_ <= HSCliStatement::MAX_NUM_HOST_VARIABLES);

  init_SQLCLI_OBJ_ID(&desc_);
  pInputDesc_ = &desc_;
  desc_.name_mode = desc_name;
  desc_.module = &module;

  strncpy(descID_, stmtID_, HS_STMTID_LENGTH);
  strncat(descID_, "_IVAR", HS_STMTID_LENGTH);

  desc_.identifier = (char *)descID_;
  desc_.identifier_len = strlen((char *)descID_);

  stmt_.handle = 0;
  retcode_ = SQL_EXEC_SetDescPointers(pInputDesc_, PRESET_VARS_NUM, (numVars_ - PRESET_VARS_NUM + 1),
                                      in18_, NULL, // starting parameter for PRESET_VARS_NUM
                                      in19_, NULL,
                                      in20_, NULL,
                                      in21_, NULL,
                                      in22_, NULL,
                                      in23_, NULL,
                                      in24_, NULL,
                                      in25_, NULL);
  if (retcode_)
    {
      HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "HSCliStatement constructor", NULL, TRUE);
      return;
    }

  validStmt_ = TRUE;
}

HSCliStatement::~HSCliStatement()
{}

Lng32 HSCliStatement::open()
  {
    Lng32 retcode = 0;
    HSErrorCatcher errorCatcher(retcode, -UERR_INTERNAL_ERROR, "HSCliStatement destructor", TRUE);

    retcode = SQL_EXEC_ClearDiagnostics(&stmt_);
    HSHandleError(retcode);
    retcode = SQL_EXEC_Exec(&stmt_, NULL, numVars_,       /* OPEN CURSOR */
                            in01_, NULL, in02_, NULL, in03_, NULL,
                            in04_, NULL, in05_, NULL, in06_, NULL,
                            in07_, NULL, in08_, NULL, in09_, NULL,
                            in10_, NULL, in11_, NULL, in12_, NULL,
                            in13_, NULL, in14_, NULL, in15_, NULL,
                            in16_, NULL, in17_, NULL, in18_, NULL,
                            in19_, NULL, in20_, NULL, in21_, NULL,
                            in22_, NULL, in23_, NULL, in24_, NULL,
                            in25_, NULL);

    HSHandleError(retcode);
    return retcode;
  }


Lng32 HSCliStatement::fetch(Lng32 numParam,
                           void *  out01, void *  out02, void *  out03,
                           void *  out04, void *  out05, void *  out06,
                           void *  out07, void *  out08, void *  out09,
                           void *  out10, void *  out11, void *  out12,
                           void *  out13, void *  out14, void *  out15,
                           void *  out16, void *  out17, void *  out18,
                           void *  out19, void *  out20, void *  out21,
                           void *  out22, void *  out23, void *  out24,
                           void *  out25
                          )
  {
    Lng32 retcode = 0;
    HS_ASSERT(numParam > 0 && out01 != NULL);
    retcode = SQL_EXEC_Fetch(&stmt_, NULL, numParam,
                             out01, NULL, out02, NULL, out03, NULL,
                             out04, NULL, out05, NULL, out06, NULL,
                             out07, NULL, out08, NULL, out09, NULL,
                             out10, NULL, out11, NULL, out12, NULL,
                             out13, NULL, out14, NULL, out15, NULL,
                             out16, NULL, out17, NULL, out18, NULL,
                             out19, NULL, out20, NULL, out21, NULL,
                             out22, NULL, out23, NULL, out24, NULL,
                             out25, NULL
                            );

    // Avoid overhead of HSErrorCatcher object for merging diagnostics from CLI,
    // since fetch() will be called many times.
    HSFilterWarning(retcode);
    if (retcode)
      {
        HSFuncLogError(retcode, (char*)HS_FILE, (Lng32)__LINE__);
        HSFilterError(retcode);
        if (retcode < 0)
          HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "HSCliStatement::fetch()", NULL, TRUE);
      }

    return retcode;
  }


Lng32 HSCliStatement::close()
  {
    Lng32 retcode = 0;
    HSErrorCatcher errorCatcher(retcode, -UERR_INTERNAL_ERROR, "HSCliStatement::close()", TRUE);

    retcode = SQL_EXEC_ClearDiagnostics(&stmt_);
    HSHandleError(retcode);

    retcode = SQL_EXEC_CloseStmt(&stmt_);
    HSHandleError(retcode);

    return retcode;
  }


// -----------------------------------------------------------------------
// Run a static sql statement without output host variables.
// -----------------------------------------------------------------------
Lng32 HSCliStatement::execFetch(const char *dml, NABoolean hideError)
{
  HSErrorCatcher errorCatcher(retcode_, hideError ? 0 : (- UERR_INTERNAL_ERROR),
                              dml, TRUE);

  if (!validStmt_)
    return -1;

   statementNum++;

  // Clear the sqlcode from any previous errors, so that this statement
  // will execute...
  Lng32 error = SQL_EXEC_ClearDiagnostics(&stmt_);

  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    {
      snprintf(LM->msg, sizeof(LM->msg), "SQL_EXEC_ExecFetch for %s", dml);
      LM->StartTimer(LM->msg);
    }
  retcode_ = SQL_EXEC_ExecFetch(&stmt_, pInputDesc_, numVars_,
                                in01_, NULL, in02_, NULL, in03_, NULL,
                                in04_, NULL, in05_, NULL, in06_, NULL,
                                in07_, NULL, in08_, NULL, in09_, NULL,
                                in10_, NULL, in11_, NULL, in12_, NULL,
                                in13_, NULL, in14_, NULL, in15_, NULL,
                                in16_, NULL, in17_, NULL, in18_, NULL,
                                in19_, NULL, in20_, NULL, in21_, NULL,
                                in22_, NULL, in23_, NULL, in24_, NULL,
                                in25_, NULL);
  if (LM->LogNeeded())
    LM->StopTimer();
  retcode_ = (retcode_ >= 0 ? 0 : retcode_);
  if (retcode_)
    validStmt_ = FALSE;

  // Optionally, we could try dynamic sql if static sql fails, i.e.,
  // retcode_ = HSFuncExecQuery(dml);

  Lng32 retcode = ((retcode_ < 0) ? -1 : retcode_);
  return retcode;
}

// -----------------------------------------------------------------------
// Constructor and destructor for HSCursor.
// -----------------------------------------------------------------------
HSCursor::HSCursor(NAHeap *heap, const char* stmtName)
 : stmtAllocated_(FALSE),
   srcDescAllocated_(FALSE),
   outputDescAllocated_(FALSE),
   inputDescAllocated_(FALSE),
   cursorName_(NULL),
   rowsetSize_(0),
   colDesc_(NULL), numEntries_(0),
   ptrNAType_(NULL), dataBuf_(NULL), outputDataLen_(0),
   group_(NULL), boundaryRowSet_(NULL), rowset_fields_(NULL),
   closeStmtNeeded_(FALSE), retcode_(0), heap_(heap),
   lastFetchReturned100_(FALSE)
{
  static SQLMODULE_ID module;
  static NABoolean moduleSet = FALSE;

  stmt_ = new(heap_) SQLSTMT_ID;
  srcDesc_ = new(heap_) SQLDESC_ID;
  outputDesc_ = new(heap_) SQLDESC_ID;
  inputDesc_ = new(heap_) SQLDESC_ID;

  if (!moduleSet)
    {
      init_SQLMODULE_ID(&module);
      moduleSet = TRUE;
    }


  init_SQLCLI_OBJ_ID(stmt_, SQLCLI_CURRENT_VERSION,
                     stmt_name, &module, stmtName, 0,
                     SQLCHARSETSTRING_ISO88591, (Lng32)strlen(stmtName), 0);
  init_SQLCLI_OBJ_ID(srcDesc_);
  init_SQLCLI_OBJ_ID(outputDesc_);
  init_SQLCLI_OBJ_ID(inputDesc_);


  srcDesc_->module = &module;
  outputDesc_->module = &module;
  inputDesc_->module = &module;

  // ---------------------------------------------------------------------
  // Initialize a descriptor which will hold the SQL statement source
  // ---------------------------------------------------------------------
  srcDesc_->name_mode  = desc_handle;
  srcDesc_->identifier = 0;
  srcDesc_->identifier_len = 0;
  srcDesc_->handle     = 0;

  // ---------------------------------------------------------------------
  // Initialize an output descriptor to retrieve values
  // ---------------------------------------------------------------------
  outputDesc_->name_mode  = desc_handle;
  outputDesc_->identifier = 0;
  outputDesc_->identifier_len = 0;
  outputDesc_->handle     = 0;

  // ---------------------------------------------------------------------
  // Initialize an input descriptor to deliver parameters.
  // This is for the dynamic interface, used by TOPL.
  // ---------------------------------------------------------------------
  inputDesc_->name_mode  = desc_handle;
  inputDesc_->identifier = 0;
  inputDesc_->identifier_len = 0;
  inputDesc_->handle     = 0;
}

HSCursor::~HSCursor()
{
  // For errors occurring in CLI calls here, just log the error rather than
  // causing the statement to fail.
  Lng32 retcode;
  if (closeStmtNeeded_)
    {
      retcode = SQL_EXEC_CloseStmt(stmt_);
      HSLogError(retcode);
      retcode = SQL_EXEC_ClearDiagnostics(stmt_);
      HSLogError(retcode);
    }
  if (inputDescAllocated_)
    SQL_EXEC_DeallocDesc(inputDesc_);
  if (outputDescAllocated_)
    {
      retcode = SQL_EXEC_DeallocDesc(outputDesc_);
      HSLogError(retcode);
    }
  if (srcDescAllocated_)
    {
      retcode = SQL_EXEC_DeallocDesc(srcDesc_);
      HSLogError(retcode);
    }
  if (stmtAllocated_)
    {
      retcode = SQL_EXEC_DeallocStmt(stmt_);
      HSLogError(retcode);
    }

  NADELETEBASIC(stmt_, heap_);
  stmt_ = NULL;
  NADELETEBASIC(srcDesc_,heap_);
  srcDesc_ = NULL;
  NADELETEBASIC(outputDesc_,heap_);
  outputDesc_ = NULL;
  NADELETEBASIC(inputDesc_,heap_);
  inputDesc_ = NULL;
  NADELETEBASIC(dataBuf_, heap_);
  dataBuf_ = NULL;

  delete cursorName_;

  NADELETEBASIC(boundaryRowSet_, heap_);
  boundaryRowSet_ = NULL;

//  NADELETEBASIC(colDesc_, heap_);
//  delete colDesc_;
//  NADELETEARRAY(colDesc_, this->numEntries_, HSColDesc, heap_);
//  colDesc_ = NULL;

//  NADELETEBASIC(ptrNAType_, heap_);
//  ptrNAType_ = NULL;

//  NADELETEBASIC(group_, heap_);
//  group_ = NULL;
}


/***********************************************/
/* METHOD:  prepareRowset()                    */
/* PURPOSE: Prepare the query and store results*/
/*          in ROWSET.                         */
/* RETCODE:  0 - successful                    */
/*          -1 - failure                       */
/* PARAMS:  cliStr(input) - single-column query*/
/*          orderAndGroup(input) - true if no  */
/*                                 internal sort */
/*          group - list of single columns     */
/*          maxRows - limit on number of rows  */
/*                    for a single fetch       */
/* ASSUMPTIONS: query is in the form:          */
/*                 SELECT column, COUNT(*) and */
/*          data retrieved in ascending order  */
/***********************************************/
Lng32 HSCursor::prepareRowset(const char *cliStr, NABoolean orderAndGroup,
                              HSColGroupStruct *group, Lng32 maxRows)
{
  // on very busy system, some "update statistics" implementation steps like
  // read_cols_into_mem, fetch_boundary_rowset, etc may experience failures
  // resulting in a flurry of callcatcher error 9200 events that show up in
  // genesis solutions like 10-110215-6139, 10-100730-2173, 10-100530-0718,
  // bugzilla 2457, etc. we suspect some of these errors may be transient
  // failures that may succeed if retried enough times.
  // 2 cqds allow user control of these retries.
  Int32 centiSecs = getDefaultAsLong(USTAT_RETRY_DELAY);
  Int32 limit = getDefaultAsLong(USTAT_RETRY_LIMIT);
  Lng32 retcode = 0;
  Int32 retry;
  for (retry = 0; retry <= limit; retry++) {
    // this must be a retry-able call
    retcode = prepareRowsetInternal(cliStr, orderAndGroup, group, maxRows);
    HSFilterWarning(retcode);
    if (retcode >= 0) break;
    if (limit && retry < limit) DELAY_CSEC(centiSecs);
  }
  return retcode;
}

Lng32 HSCursor::prepareRowsetInternal
(const char *cliStr, NABoolean orderAndGroup,
 HSColGroupStruct *group, Lng32 maxRows)
  {
    // Not needed, as there is an error catcher in all of the caller's
    // code paths. (And having two of them results in double reporting
    // of the errors.)
    //HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR,
    //                            "HSCursor::prepareRowsetInternal()", TRUE);
    HSLogMan *LM = HSLogMan::Instance();
    HSColGroupStruct *col = group;
    Lng32 numResults = 0;
    Lng32 rowset_status[1];      //Has no functionality currently.
                                //However it is part of RowsetSetDesc API

    if (orderAndGroup)
    {
      if (group->computeAvgVarCharSize())
      {
        numResults = 3; //FMT(value), UEC(value), AVG(OCTET_LENGTH(value))
      }
      else
        numResults = 2;  //FMT(value), UEC(value)
    }
    else if (col)
      {
        do
          {
            if (col->state == PENDING)
              numResults++;
          }
        while (col = col->next);
      }
    else
    {
      LM->Log("***[ERROR] prepareRowset: GROUP PTR IS NULL\n");
      return -1;
    }

    LM->Log(cliStr);

      // Cannot reuse a cursor.  This causes memory leak.
    if (stmtAllocated_)
      {
        LM->Log("***[ERROR] REUSING ALLOCATED CURSOR\n");
        return -1;
      }


    retcode_ = SQL_EXEC_ClearDiagnostics(stmt_);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_AllocStmt(stmt_, 0);
    HSHandleError(retcode_);
    stmtAllocated_ = TRUE;

    retcode_ = SQL_EXEC_AllocDesc(srcDesc_, 1);
    HSHandleError(retcode_);
    srcDescAllocated_ = TRUE;

    retcode_ = SQL_EXEC_AllocDesc(outputDesc_, numResults);
    HSHandleError(retcode_);
    outputDescAllocated_ = TRUE;

    retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_TYPE_FS,
                                    REC_BYTE_V_ANSI, 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_VAR_PTR,
                                    (Long)cliStr, 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_LENGTH,
                                    strlen(cliStr) + 1, 0);
    HSHandleError(retcode_);
    // SQLDESC_CHAR_SET must be the last descriptor item set, otherwise
    // it may get reset by other calls to SQL_EXEC_SetDescItem().
    NAString charSet = ActiveSchemaDB()->getDefaults().getValue(ISO_MAPPING);
    NAString defCS   = ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_CHAR_SET,
                                    SQLCHARSETCODE_UTF8
                                    , 0);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_SetStmtAttr(stmt_, SQL_ATTR_PARENT_QID, 0,
            (char *)CmpCommon::context()->sqlSession()->getParentQid());
    HSHandleError(retcode_);


                                              /*==============================*/
                                              /*      PREPARE STATEMENT       */
                                              /*==============================*/
    SQL_QUERY_COST_INFO query_cost_info;
    SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;


    retcode_ = SQL_EXEC_Prepare2(stmt_, srcDesc_,NULL,0,NULL,&query_cost_info, &comp_stats_info,NULL,0,0);
    HSHandleError(retcode_);
    if (LM->LogNeeded())
      {
        printPlan(stmt_);
        SQL_EXEC_ClearDiagnostics(stmt_);
      }
                                              /*==============================*/
                                              /*    SET OUTPUT DESCRIPTOR     */
                                              /*        USING ROWSETS         */
                                              /*==============================*/
    retcode_ = SQL_EXEC_DescribeStmt(stmt_, 0, outputDesc_);
    HSHandleError(retcode_);

    rowset_fields_  = new(heap_) SQLCLI_QUAD_FIELDS[numResults];

    if (orderAndGroup)
      {
        boundaryRowSet_ = new (heap_) boundarySet<myVarChar>;
        rowset_fields_[0].var_layout = HS_MAX_BOUNDARY_LEN;
        rowset_fields_[0].var_ptr = (void *)&boundaryRowSet_->data[0];
        rowset_fields_[0].ind_layout = sizeof(short);
        rowset_fields_[0].ind_ptr = (void *)&boundaryRowSet_->nullInd[0];

        rowset_fields_[1].var_layout = sizeof(Int64);
        rowset_fields_[1].var_ptr = (void *)&boundaryRowSet_->dataSum[0];
        rowset_fields_[1].ind_layout = 0;
        rowset_fields_[1].ind_ptr = 0;

        if (numResults == 3)
        {
          rowset_fields_[2].var_layout = sizeof(Int64);
          rowset_fields_[2].var_ptr = (void *)&boundaryRowSet_->avgVarCharSize[0];
          rowset_fields_[2].ind_layout = sizeof(short);
          rowset_fields_[2].ind_ptr = (void *)&boundaryRowSet_->avgVarCharNullInd[0];
        }

        retcode_ = SQL_EXEC_SETROWSETDESCPOINTERS(outputDesc_,
                                                  MAX_ROWSET,
                                                  rowset_status,
                                                  1,
                                                  numResults,
                                                  rowset_fields_);
        HSHandleError(retcode_);
      }
    else
      retcode_ = setRowsetPointers(group, maxRows);

                                              /*==============================*/
                                              /*         OPEN CURSOR          */
                                              /*==============================*/
    retcode_ = SQL_EXEC_ClearDiagnostics(stmt_);
    HSHandleError(retcode_);
    LM->StartTimer("open cursor (execute)");
    retcode_ = SQL_EXEC_Exec(stmt_, 0, 0,
                             0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
    LM->StopTimer();
    if (retcode_ < 0)
      {
        LM->Log("***[FAILED] Unable to open rowset cursor.\n");
        HSHandleError(retcode_);
      }

    closeStmtNeeded_ = TRUE;
    return 0;
  }

/********************************************************/
/* METHOD:  setRowsetPointers()                         */
/* PURPOSE: Bind data and indicator addresses for       */
/*          columns retrieved for use by internal sort. */
/* PARAMS:  group - list of single columns histograms   */
/*                  are being created for.              */
/*          maxRows - limit on number of rows to        */
/*                    retrieve in a single fetch.       */
/* RETCODE: 0 - successful                              */
/********************************************************/
Lng32 HSCursor::setRowsetPointers(HSColGroupStruct *group, Lng32 maxRows)
  {
    HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR, "HSCursor::setRowsetPointers()", TRUE);
    Int32 j=0;
    Lng32 rowset_status[1];      //Has no functionality currently.
                                //However it is part of RowsetSetDesc API

    do
    {
      if (group->state != PENDING)
        continue;

      if (group->nullIndics)
        {
          rowset_fields_[j].ind_layout = sizeof(short);
          rowset_fields_[j].ind_ptr = (void *)group->nullIndics;
        }
      else
        {
          rowset_fields_[j].ind_layout = 0;
          rowset_fields_[j].ind_ptr    = 0;
        }

      rowset_fields_[j].var_layout = group->ISlength;

      // Set the first offset that a value of this column will be written to.
      // Character data is written into a different buffer, and the data buffer
      // will consist of pointers to the char values.
      if (DFS2REC::isAnyCharacter(group->ISdatatype))
        {
          if (DFS2REC::isSQLVarChar(group->ISdatatype) && group->isCompacted())
            rowset_fields_[j].var_ptr = (void *)group->varcharFetchBuffer;
          else
            rowset_fields_[j].var_ptr = (void *)group->strNextData;
        }
      else
        rowset_fields_[j].var_ptr = (void *)group->nextData;
      j++;
    }
    while (group = group->next);

    retcode_ = SQL_EXEC_SETROWSETDESCPOINTERS(outputDesc_,
                                              maxRows,
                                              rowset_status,
                                              1,
                                              j,
                                              rowset_fields_);
    HSHandleError(retcode_);
    return retcode_;
  }

/***********************************************/
/* METHOD:  prepare()                          */
/* PURPOSE: Prepare the query and use single   */
/*          fetch cursor.                      */
/* RETCODE:  0 - successful                    */
/*          -1 - failure                       */
/* PARAMS:  cliStr(input) - single-column query*/
/* ASSUMPTIONS: query is in the form:          */
/*                 SELECT column, COUNT(*) and */
/*          data retrieved in ascending order  */
/***********************************************/
Lng32 HSCursor::prepare( const char *clistr
                      , const Lng32 outDescEntries
                      )
{
  HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR, "HSCursor::prepare()", TRUE);
  Lng32 entry;
  HSLogMan *LM = HSLogMan::Instance();

  LM->Log(clistr);

  // Cannot reuse a cursor.  This causes memory leak.
  if (stmtAllocated_)
    {
      LM->Log("***[ERROR] REUSING ALLOCATED CURSOR\n");
      return -1;
    }

  retcode_ = SQL_EXEC_ClearDiagnostics(stmt_);
  HSHandleError(retcode_);

  retcode_ = SQL_EXEC_AllocStmt(stmt_, 0);
  HSHandleError(retcode_);
  stmtAllocated_ = TRUE;

  retcode_ = SQL_EXEC_AllocDesc(srcDesc_, 1);
  HSHandleError(retcode_);
  srcDescAllocated_ = TRUE;

  retcode_ = SQL_EXEC_AllocDesc(outputDesc_, outDescEntries);
  HSHandleError(retcode_);
  outputDescAllocated_ = TRUE;

  retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_TYPE_FS,
                                  REC_BYTE_V_ANSI, 0);
  HSHandleError(retcode_);
  retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_VAR_PTR,
                                  (Long)clistr, 0);
  HSHandleError(retcode_);
  retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_LENGTH,
                                  strlen(clistr) + 1, 0);
  HSHandleError(retcode_);

  // SQLDESC_CHAR_SET must be the last descriptor item set, otherwise
  // it may get reset by other calls to SQL_EXEC_SetDescItem().
  NAString charSet = ActiveSchemaDB()->getDefaults().getValue(ISO_MAPPING);
  NAString defCS   = ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET);
  retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_CHAR_SET,
                                  SQLCHARSETCODE_UTF8
                                  , 0);
  HSHandleError(retcode_);
  retcode_ = SQL_EXEC_SetStmtAttr(stmt_, SQL_ATTR_PARENT_QID, 0,
          (char *)CmpCommon::context()->sqlSession()->getParentQid());
  HSHandleError(retcode_);

                                              /*==============================*/
                                              /*      PREPARE STATEMENT       */
                                              /*==============================*/
  LM->StartTimer("SQL_EXEC_Prepare");
  retcode_ = SQL_EXEC_Prepare(stmt_, srcDesc_);
  LM->StopTimer();
  HSHandleError(retcode_);
  if (LM->LogNeeded())
    {
      printPlan(stmt_);
      SQL_EXEC_ClearDiagnostics(stmt_);
    }
                                              /*==============================*/
                                              /*    SET OUTPUT DESCRIPTOR     */
                                              /*     SINGLE FETCH CURSOR      */
                                              /*==============================*/
  LM->StartTimer("Set up output descriptor");
  retcode_ = SQL_EXEC_DescribeStmt(stmt_, 0, outputDesc_);
  HSHandleError(retcode_);

  retcode_ = SQL_EXEC_GetDescEntryCount(outputDesc_, &numEntries_);
  HSHandleError(retcode_);

  delete [] colDesc_;
  colDesc_ = new(heap_) HSColDesc[numEntries_];
  outputDataLen_ = 0;

  Int32 i = 0;
  for (; i < numEntries_; i++)
    {
      entry = i + 1;
      retcode_ = SQL_EXEC_GetDescItem(outputDesc_, entry,
                                      SQLDESC_TYPE_FS,
                                      &colDesc_[i].datatype,
                                      0, 0, 0, 0);
      HSHandleError(retcode_);

      retcode_ = SQL_EXEC_GetDescItem(outputDesc_, entry,
                                      SQLDESC_OCTET_LENGTH,
                                      &colDesc_[i].length,
                                      0, 0, 0, 0);
      HSHandleError(retcode_);

      if (colDesc_[i].datatype == REC_DATETIME)
        {
          retcode_ = SQL_EXEC_GetDescItem(outputDesc_, entry,
                                         SQLDESC_DATETIME_CODE,
                                         &colDesc_[i].precision, 0, 0, 0, 0);
          HSHandleError(retcode_);
          retcode_ = SQL_EXEC_GetDescItem(outputDesc_, entry,
                                         SQLDESC_PRECISION,
                                         &colDesc_[i].scale, 0, 0, 0, 0);
          HSHandleError(retcode_);
        }
      else if ((colDesc_[i].datatype >= REC_MIN_INTERVAL) &&
               (colDesc_[i].datatype <= REC_MAX_INTERVAL))
        {
          retcode_ = SQL_EXEC_GetDescItem(outputDesc_, entry,
                                         SQLDESC_INT_LEAD_PREC,
                                         &colDesc_[i].precision, 0, 0, 0, 0);
          HSHandleError(retcode_);
          retcode_ = SQL_EXEC_GetDescItem(outputDesc_, entry,
                                         SQLDESC_PRECISION,
                                         &colDesc_[i].scale, 0, 0, 0, 0);
          HSHandleError(retcode_);
        }
      else
        {
          retcode_ = SQL_EXEC_GetDescItem(outputDesc_, entry,
                                          SQLDESC_PRECISION,
                                          &colDesc_[i].precision,
                                          0, 0, 0, 0);
          HSHandleError(retcode_);

          retcode_ = SQL_EXEC_GetDescItem(outputDesc_, entry,
                                          SQLDESC_SCALE,
                                          &colDesc_[i].scale,
                                          0, 0, 0, 0);
          HSHandleError(retcode_);
        }

      retcode_ = SQL_EXEC_GetDescItem(outputDesc_, entry,
                                      SQLDESC_NULLABLE,
                                      &colDesc_[i].nullflag,
                                      0, 0, 0, 0);
      HSHandleError(retcode_);

      colDesc_[i].groupNum = 0;

      colDesc_[i].indDataOffset = outputDataLen_;
      if (colDesc_[i].nullflag)
        outputDataLen_ += SQL_NULL_HDR_SIZE;

      colDesc_[i].dataOffset = outputDataLen_;

      //We need to adjust the buffer length to incorporate the length field for
      //VARCHAR datatypes. Currently, the length field for VARCHAR is 2bytes. If
      //a new datatype is created and requires a length field larger than 2bytes
      //we will need to re-write this code. Possibly use
      //SQLTYPECODE_VARCHAR_WITH_LENGTH
      if (DFS2REC::isAnyVarChar(colDesc_[i].datatype))
        outputDataLen_ += SQL_VARCHAR_HDR_SIZE;
      outputDataLen_ += colDesc_[i].length;
    } // for loop

  // aligned on 4-byte boundary
  outputDataLen_ = roundup4(outputDataLen_);

  // Make sure dataBuf_ starts at a 4-byte boundary.
  dataBuf_ = (char *)(new(heap_) Lng32[(outputDataLen_ >> 2) + 1]);
  Long addr;

  for (i = 0; i < numEntries_; i++)
    {
      entry = i + 1;
      colDesc_[i].data = &dataBuf_[colDesc_[i].dataOffset];
      colDesc_[i].indData = &dataBuf_[colDesc_[i].indDataOffset];

      if (colDesc_[i].nullflag)
        {
          addr = (Long) colDesc_[i].indData;
          retcode_ = SQL_EXEC_SetDescItem(outputDesc_, entry,
                                          SQLDESC_IND_PTR,
                                          addr, 0);
          HSHandleError(retcode_);
        }
      addr = (Long) colDesc_[i].data;
      retcode_ = SQL_EXEC_SetDescItem(outputDesc_,
                                      entry,
                                      SQLDESC_VAR_PTR,
                                      addr, 0);
      HSHandleError(retcode_);
    }
  LM->StopTimer();

                                              /*==============================*/
                                              /*         OPEN CURSOR          */
                                              /*==============================*/
  retcode_ = SQL_EXEC_ClearDiagnostics(stmt_);
  HSHandleError(retcode_);
  LM->StartTimer("SQL_EXEC_Exec (open cursor)");
  retcode_ = SQL_EXEC_Exec(stmt_, 0, 0,
                           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
  LM->StopTimer();
  if (retcode_)
    {
      if (LM->LogNeeded())
        {
          snprintf(LM->msg, sizeof(LM->msg), "***[FAILED] OPEN CURSOR, retcode=%d", retcode_);
          LM->Log(LM->msg);
        }
      HSHandleError(retcode_);
    }

  closeStmtNeeded_ = TRUE;
  return 0;
}

// -----------------------------------------------------------------------
// Classes specialized for fast encoding.
// -----------------------------------------------------------------------
#define ALIGN2(addr) ((addr & 0x1) == 0)
#define ALIGN4(addr) ((addr & 0x3) == 0)
#define ALIGN8(addr) ((addr & 0x7) == 0)

template <class T> class HSBin : public SQLInt {

public:

  HSBin(Lng32 id)
    : SQLInt(NULL)
    , id_(id) {}
  ~HSBin() {}
  double encode(void *data) const
  {  return *((T *)data);  }

private:

  Lng32 id_;
};

// -----------------------------------------------------------------------
class HSLargeint : public SQLInt {

public:

  HSLargeint(Lng32 id)
    : SQLInt(NULL)
    , id_(id) {}
  ~HSLargeint() {}
  double encode(void *data) const
  {  return convertInt64ToDouble(*((Int64 *)data)); }

private:

  Lng32 id_;
};

// -----------------------------------------------------------------------
class HSDate : public SQLDate {

public:

  HSDate() : SQLDate(NULL, FALSE /*nullflag*/) {}
  double encode(void *data) const
  {
    ULng32 w[4];
    datetimeToLong((char *)data, w);
    double val = (w[0] - 1) * 365 + (w[1] - 1) * 30 + w[2];
    return val;
  }
};

// -----------------------------------------------------------------------
NAType* ConstructNumericType( Long addr
                                   , Lng32 id
                                   , Lng32 length
                                   , Lng32 precision
                                   , Lng32 scale
                                   , NABoolean allowNeg
                                   , NABoolean nullflag
                                   , NAHeap *currHeap
                                   )
{
  NAType *type;
  switch(length) {
  case 1:
    type = new(currHeap) SQLTiny(currHeap, allowNeg, nullflag);
    break;
  case 2:
    if (!ALIGN2(addr))
      {
        type = new(currHeap) SQLSmall(currHeap, allowNeg, nullflag);
        break;
      }
    if (allowNeg)  // 2-byte aligned
      type = new(currHeap) HSBin<short>(id);
    else
      type = new(currHeap) HSBin<unsigned short>(id);
    break;
  case 4:
    if (!ALIGN4(addr))
      {
        type = new(currHeap) SQLInt(currHeap, allowNeg, nullflag);
        break;
      }
    if (allowNeg)  // 4-byte aligned
      type = new(currHeap) HSBin<Lng32>(id);
    else
      type = new(currHeap) HSBin<ULng32>(id);
    break;
  case 8:
    if (!ALIGN8(addr))
      {
        type = new(currHeap) SQLLargeInt(currHeap, allowNeg, nullflag);
        break;
      }
    if (allowNeg)  // 8-byte aligned
      type = new(currHeap) HSLargeint(id);
    else
      type = new(currHeap) SQLLargeInt(currHeap, allowNeg, nullflag);
    break;
  default:
    type = new(currHeap) SQLNumeric(currHeap, length, precision, scale, allowNeg, nullflag);
    break;
  }
  return type;
}

// -----------------------------------------------------------------------
// Construct a NA type from an SQL type so that NAtype.encode can be
// called later.
// -----------------------------------------------------------------------
Lng32 HSCursor::buildNAType()
{
#define REC_INTERVAL REC_MIN_INTERVAL
  NAType *type;
  Lng32 datatype, intervalType, length, precision, scaleword, nullword;
  Int32 nullflag = 0;
  Lng32 scale = 0;
  Long addr;

  if (ptrNAType_ != NULL)
    delete [] ptrNAType_;
  ptrNAType_ = new(heap_) HSPtrObj<NAType>[numEntries_];

  for (Lng32 i = 0; i < numEntries_; i++)
    {
      datatype = colDesc_[i].datatype;
      length = colDesc_[i].length;
      precision = colDesc_[i].precision;
      scaleword = colDesc_[i].scale;
      nullword = colDesc_[i].nullflag;
      type = NULL;
      addr = (Long) colDesc_[i].data;

      if (REC_MIN_INTERVAL <= datatype && datatype <= REC_MAX_INTERVAL)
        {
          intervalType = datatype;
          datatype = REC_INTERVAL;
        }

      switch(datatype)
        {
        case REC_BIN8_SIGNED:
          if (precision <= 0)
            length = 1;
          type = ConstructNumericType(addr, i, length, precision, scale,
                                      TRUE, nullflag, heap_);
          break;
        case REC_BIN8_UNSIGNED:
          if (precision <= 0)
            length = 1;
          type = ConstructNumericType(addr, i, length, precision, scale,
                                      FALSE, nullflag, heap_);
          break;

       case REC_BIN16_SIGNED:
          if (precision <= 0)
            length = 2;
          type = ConstructNumericType(addr, i, length, precision, scale,
                                      TRUE, nullflag, heap_);
          break;
        case REC_BPINT_UNSIGNED:
        case REC_BIN16_UNSIGNED:
          if (precision <= 0)
            length = 2;
          type = ConstructNumericType(addr, i, length, precision, scale,
                                      FALSE, nullflag, heap_);
          break;

        //
        //
        case REC_BIN32_SIGNED:
          if (precision <= 0)
            length = 4;
          type = ConstructNumericType(addr, i, length, precision, scale,
                                      TRUE, nullflag, heap_);
          break;
        case REC_BIN32_UNSIGNED:
          if (precision <= 0)
            length = 4;
          type = ConstructNumericType(addr, i, length, precision, scale,
                                      FALSE, nullflag, heap_);
          break;
        //
        //
        case REC_BIN64_SIGNED:
          if (precision <= 0)
            length = 8;
          type = ConstructNumericType(addr, i, length, precision, scale,
                                      TRUE, nullflag, heap_);
          break;
        case REC_BIN64_UNSIGNED:
          if (precision <= 0)
            length = 8;
          type = ConstructNumericType(addr, i, length, precision, scale,
                                      FALSE, nullflag, heap_);
          break;
        //
        //----------------------------------------------------------------
	case REC_FLOAT32:
	case REC_FLOAT64:
	  //datatype = ((precision <= SQL_REAL_PRECISION) ?
	  //           REC_FLOAT32 : REC_FLOAT64);
	  if (datatype == REC_FLOAT32)
	    type = new(heap_) SQLReal(heap_, nullflag, precision);
	  else  if (datatype == REC_FLOAT64)
	    type = new(heap_) SQLDoublePrecision(heap_, nullflag, precision);
	  break;
        //
        //
        case REC_DECIMAL_UNSIGNED:
          type = new(heap_) SQLDecimal(heap_, length, scale, FALSE, nullflag);
          break;
        case REC_DECIMAL_LSE:
          type = new(heap_) SQLDecimal(heap_, length, scale, TRUE, nullflag);
          break;
        case REC_NUM_BIG_UNSIGNED:
          type = new(heap_) SQLBigNum(heap_, precision, scale, FALSE, FALSE, nullflag);
          break;
        case REC_NUM_BIG_SIGNED:
          type = new(heap_) SQLBigNum(heap_, precision, scale, FALSE, TRUE, nullflag);
          break;
         //
        //
        case REC_BYTE_F_ASCII:
        case REC_NCHAR_F_UNICODE:
          type = new(heap_) SQLChar(heap_,    length
                             ,   nullflag
                             #ifdef FULL_CHARSET_SUPPORT  //##NCHAR: to be done!
                             ,   colDesc_[i].upshifted
                             ,   FALSE/*varLenFlag*/
                             ,   colDesc_[i].charset
                             ,   colDesc_[i].collation
                             ,   colDesc_[i].coercibility
                             #endif
                            );
          break;
        case REC_BYTE_V_ASCII:
        case REC_NCHAR_V_UNICODE:
          type = new(heap_) SQLVarChar(heap_,  length
                             ,   nullflag
                             #ifdef FULL_CHARSET_SUPPORT  //##NCHAR: to be done!
                             ,   colDesc_[i].upshifted
                             ,   colDesc_[i].charset
                             ,   colDesc_[i].collation
                             ,   colDesc_[i].coercibility
                             #endif
                            );
          break;

	  // datetime and interval datatype values are fetched in external
	  // string format. Create a 'SQLChar' type for them so they could
	  // be encoded correctly.
	case REC_DATETIME:
	case REC_INTERVAL:
          type = new(heap_) SQLChar(heap_,    length
                             ,   nullflag
                            );

	  break;

        case REC_BOOLEAN:
          type = new(heap_) SQLBooleanNative(heap_, nullflag);
          break;

        case REC_BINARY_STRING:
          type = new(heap_) SQLBinaryString(heap_, length, nullflag, FALSE);
          break;

        case REC_VARBINARY_STRING:
          type = new(heap_) SQLBinaryString(heap_, length, nullflag, TRUE);
          break;

        default:
          HSFuncMergeDiags(- UERR_UNSUPPORTED_DATATYPE);
          retcode_ = -1;
          HSHandleError(retcode_);
        }
      ptrNAType_[i].pt_ = type;
    }

  return 0;
}

/***********************************************/
/* METHOD:  fetchRowset()                      */
/* PURPOSE: Fetch results from query and store */
/*          in rowset.                         */
/* RETCODE:  0 - successful                    */
/*          -1 - failure                       */
/* PARAMS:  none                               */
/* ASSUMPTIONS: cursor has been prepared       */
/***********************************************/
Lng32 HSCursor::fetchRowset()
  {
    HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR, "HSCursor::fetchRowset()", TRUE);
    retcode_ = SQL_EXEC_Fetch(stmt_,outputDesc_, 0);
    HSHandleError(retcode_);
    if (retcode_ == 0 || retcode_ == 100)
      {
        retcode_ = SQL_EXEC_GetDescItem(outputDesc_, 1,
                                        SQLDESC_ROWSET_NUM_PROCESSED,
                                        &rowsetSize_,
                                        0, 0, 0, 0);
        HSHandleError(retcode_);
        if (boundaryRowSet_)
          boundaryRowSet_->size = rowsetSize_;
      }

    return retcode_;
  }

/***************************************************************************/
/* METHOD:  mergeAverage()                                                 */
/* PURPOSE: Merges the average gap magnitude for a rowset into the average */
/*          for all gaps seen so far.                                      */
/* PARAMS:  firstRowset(in)         -- TRUE if 1st rowset for this column. */
/*          rowsetGapAvg(in)        -- Average gap magnitude for this      */
/*                                     rowset.                             */
/*          rowsetGapCount(in)      -- Number of gaps (distinct values     */
/*                                     minus 1) in the rowset.             */
/*          overallGapAvg(in/out)   -- Overall gap magnitude so far.       */
/*          overallGapCount(in/out) -- Overall number of gaps so far.      */
/***************************************************************************/
void mergeAverage(NABoolean firstRowset,
                  double rowsetGapAvg, Int64 rowsetGapCount,
                  double &overallGapAvg, Int64 &overallGapCount)
{
  HSLogMan *LM = HSLogMan::Instance();

  // If this is the first rowset, just use the values for that rowset.
  if (overallGapCount == 0)
    {
      overallGapAvg = rowsetGapAvg;
      overallGapCount = rowsetGapCount;
      return;
    }

  Int64 newOverallGapCount = overallGapCount + rowsetGapCount;
  overallGapAvg = (overallGapAvg * overallGapCount) / newOverallGapCount
                   +(rowsetGapAvg  * rowsetGapCount)  / newOverallGapCount;
  overallGapCount = newOverallGapCount;

  if (LM->LogNeeded())
    {
      static Int32 numRowsets = 0;
      if (firstRowset)
        numRowsets = 0;
      snprintf(LM->msg, sizeof(LM->msg), "Gap average for this rowset/first %d rowsets = %.2f/%.2f",
                       ++numRowsets, rowsetGapAvg, overallGapAvg);
      LM->Log(LM->msg);
    }
}

/***********************************************/
/* METHOD:  fetchBoundaries()                  */
/* PURPOSE: Using ROWSETS - Fetch data from    */
/*          table and build histogram          */
/*          intervals.                         */
/* RETCODE:  0 - successful                    */
/*          -1 - failure                       */
/* PARAMS:  group (input) -                    */
/*               single-column group           */
/*          rowCount (input/output)            */
/*               #rows expected to read        */
/*          intCount (input)                   */
/*               #intervals to generate        */
/* ASSUMPTIONS: query is in the form:          */
/*                 SELECT column, COUNT(*) and */
/*              retrieved in ascending order   */
/* NOTES:   The last interval is reserved for  */
/*          NULL values only.                  */
/*          bndry:   )[----](----]...(----]    */
/*          int#     0   1     2  ...   n      */
/***********************************************/
Lng32 HSCursor::fetchBoundaries(HSColGroupStruct *group,
                               Int64 &rowCount,
                               Lng32 intCount,
                               NABoolean sampleUsed)
  {
    Int32 i;
    Lng32 adjIntCount = intCount;
    Int64 xTotalRowsRead = 0;
    Int64 rowsInSet = 0;
    HSGlobalsClass *hs_globals = GetHSContext();
    NABoolean singleIntervalPerUec = FALSE;
    Lng32 gapIntCount;      // # intervals to use for gaps
    Lng32 highFreqIntCount; // # intervals added for possible high frequency values

    HSErrorCatcher errorCatcher(retcode_, - UERR_INTERNAL_ERROR, "FETCH_BOUNDARY_ROWSET", TRUE);

                                              /*==============================*/
                                              /*        PREPARE ROWSET        */
                                              /*==============================*/
    // prepareRowset may do retries
    retcode_ = prepareRowset(group->clistr->data(), TRUE, group, 0);
    HSHandleError(retcode_);
                                              /*==============================*/
                                              /*    FETCH DATA INTO ROWSET    */
                                              /*==============================*/
    // this is unlikely to succeed on a retry, so we don't.
    retcode_ = fetchRowset();
    if (rowsetSize() == 0 && retcode_ >= 0)        /* no need to proces empty */
      {                                            /* table, set rowcount=0   */
        rowCount = 0;
        return SUCCESS;
      }
    HSHandleError(retcode_);
                                              /*==============================*/
                                              /*       ADJUST #INTERVALS      */
                                              /*==============================*/
    // Don't include group for null in rowset count used for interval adjustment,
    // or singleIntervalPerUec won't be set rowset count exactly equals the
    // user-specified interval count.
    Lng32 nonNullRowsets = boundaryRowSet_->nullInd[rowsetSize()-1] == -1
                            ? rowsetSize() - 1
                            : rowsetSize();
    adjIntCount = hs_globals->getAdjustedIntervalCount(group, intCount,
                                                       rowCount, nonNullRowsets,
                                                       singleIntervalPerUec,
                                                       gapIntCount,
                                                       highFreqIntCount);
    group->groupHist = new(heap_) HSHistogram(adjIntCount, rowCount,
                                                 gapIntCount, highFreqIntCount,
                                                 sampleUsed, singleIntervalPerUec);

    // If gaps are not to be processed, set all the gap sizes to 0 so subsequent
    // processing won't have to take it into account. Don't need to do this again
    // for subsequent rowsets of the same group; the values will remain 0.
    // Gap processing is not done if using a single uec per interval, the group
    // is a multi-column group, or the single column is not numeric.
    if (gapIntCount == 0)
      {
        for (i=0; i<MAX_ROWSET; i++)
          {
            boundaryRowSet_->gapMagnitude[i] = 0;
          }
      }

    NABoolean firstRowset = TRUE;    // for use by profileGaps

    // Maintain running average of magnitude of gaps and total # gaps. A gap is
    // the numeric difference between 2 consecutive values, so the integers 5 and
    // 6 are considered to have a gap of 1.
    double overallGapAvg = 0;
    Int64 overallGapCount = 0;

    while (retcode_ == 0)
      {
        if (gapIntCount > 0)
          profileGaps(group, boundaryRowSet_, overallGapAvg, overallGapCount,
                      firstRowset);
        group->groupHist->processIntervalValues(
                             boundaryRowSet_,
                             group,
                             rowsInSet,
                             overallGapAvg);
        xTotalRowsRead += rowsInSet;

                                              /*==============================*/
                                              /*    FETCH DATA INTO ROWSET    */
                                              /*==============================*/
        retcode_ = fetchRowset(); // not retry-able
        firstRowset = FALSE;
        if (retcode_ == HS_EOF)
          {
            //update the actual number of rows processed
            rowCount = xTotalRowsRead;
            break;
          }
        HSHandleError(retcode_);
      }

    hs_globals->checkTime("after fetching data into rowsets for RUS");

    // Now that all distinct values and their frequencies have been seen, we
    // know the actual gap average. Revisit the gap intervals we created,
    // and keep the gapIntCount ones with the greatest gap magnitude. The rest
    // are merged into adjacent intervals, unless that would create an interval
    // of excessive height.
    if (gapIntCount > 0)
      group->groupHist->removeLesserGapIntervals(overallGapAvg);

    return SUCCESS;
  }



// -----------------------------------------------------------------------
// Fetch a numerical type column.
// -----------------------------------------------------------------------
Lng32 HSCursor::fetchNumColumn( const char *clistr
                             , Lng32 *pSmallValue
                             , Int64 *pLargeValue
                             )
{
  HSErrorCatcher errorCatcher(retcode_, - UERR_INTERNAL_ERROR, "fetchNumColumn", TRUE);
  HSLogMan *LM = HSLogMan::Instance();

  LM->StartTimer("prepare() from HSCursor::fetchNumColumn()");
  retcode_ = prepare(clistr, 1);
  LM->StopTimer();
  HSHandleError(retcode_);

  LM->StartTimer("getRow() from HSCursor::fetchNumColumn()");
  retcode_ = getRow();
  LM->StopTimer();
  HSHandleError(retcode_);

  switch(colDesc_[0].length)
    {
    case sizeof(Int32):
      {
        Int32 tmp;
        memcpy((char *) &tmp,
               colDesc_[0].data,
               sizeof(Int32));
        if (pSmallValue != NULL)
          *pSmallValue = tmp;
        if (pLargeValue != NULL)
          *pLargeValue = (Int64)tmp;
        break;
      }
    case sizeof(Int64):
      {
        Int64 tmp;
        memcpy((char *) &tmp,
               colDesc_[0].data,
               sizeof(Int64));
        if (pSmallValue != NULL)
          *pSmallValue = int64ToInt32(tmp);
        if (pLargeValue != NULL)
          *pLargeValue = tmp;
        break;
      }
      return -1;
      // Following assert is being commented to let the query compile with optimizer using
      // histograms to estimate cardinalities instead of relying on fetchCount. Also created
      // a Sol: 10-100318-8861 to track the issue as to why the colDesc_[0].length is not being
      // set correctly.
      // default: HS_ASSERT(FALSE);
    }

  return 0;
}

// -----------------------------------------------------------------------
// Fetch char type and largeint type columns from a table.
// -----------------------------------------------------------------------
Lng32 HSCursor::fetchCharNumColumn(const char *clistr, NAString &value1, Int64 &value2, double &value3)
{
  HSErrorCatcher errorCatcher(retcode_, - UERR_INTERNAL_ERROR, "fetchCharColumn", TRUE);

  retcode_ = prepare(clistr, 1);
  HSHandleError(retcode_);

  retcode_ = getRow();
  if (retcode_ && retcode_ != HS_EOF) HSHandleError(retcode_);
  if ((colDesc_[0].datatype != REC_BYTE_V_ASCII && colDesc_[0].datatype != REC_BYTE_V_DOUBLE) ||
       colDesc_[1].datatype != REC_BIN64_SIGNED ||
      (colDesc_[2].datatype != REC_IEEE_FLOAT64))
    return -1; // Result should be VARCHAR, LARGEINT, and FLOAT64 datatypes.

  if (retcode_ != HS_EOF)
  {
    // Copy varchar output to 'value'.  First byte is length of varchar.
    // Varchar data starts at byte 3.
#if defined(NA_LITTLE_ENDIAN)
    char length = colDesc_[0].data[0];
#elif defined(NA_BIG_ENDIAN)
    char length = colDesc_[0].data[1];
#else
    "Neither NA_LITTLE_ENDIAN or NA_BIG_ENDIAN is defined."
#endif
    char *tmp1 = new (heap_) char[length + 1];
    memcpy(tmp1, colDesc_[0].data+2, length);
    tmp1[length] = '\0';
    value1 = tmp1;
    NADELETEBASIC(tmp1, heap_);

    Int64 tmp2;
    memcpy((char *) &tmp2, colDesc_[1].data, sizeof(Int64));
    value2 = tmp2;

    // Assign value3 based on whether it is Tandem or IEEE float.
    double tmp3;
    if (colDesc_[2].datatype == REC_IEEE_FLOAT64)
      memcpy((char *) &tmp3, colDesc_[2].data, colDesc_[2].length);
    value3 = tmp3;
  }
  else value1="";
  return 0;
}


// Constructor used for static interface.
HSinsertHist::HSinsertHist(const char *stmtID,
                           const char *histTable)
    : tableName_(histTable),
      numRows_(0),
      retcode_(0),
      stmtText_(""),
      stmtAllocated_(FALSE),
      srcDescAllocated_(FALSE),
      inputDescAllocated_(FALSE)
  {
    static SQLMODULE_ID module;
    static char moduleName[HS_MODULE_LENGTH];
    static NABoolean moduleSet = FALSE;

    if (!moduleSet)
      {

        init_SQLMODULE_ID(&module);
        strncpy(moduleName, HS_MODULE, HS_MODULE_LENGTH);

        module.module_name = (char *)moduleName;
        module.module_name_len = strlen((char*)moduleName);
        module.creation_timestamp = 1234567890;
        moduleSet = TRUE;
      }

    init_SQLCLI_OBJ_ID(&stmt_);
    stmt_.name_mode = stmt_name;
    stmt_.module = &module;
    strncpy(stmtID_, stmtID, HS_STMTID_LENGTH);
    stmt_.identifier = (char *)stmtID_;
    stmt_.identifier_len = strlen((char *)stmtID_);
    stmt_.handle = 0;


    init_SQLCLI_OBJ_ID(&desc_);
    desc_.name_mode = desc_name;
    desc_.module = &module;
    strncpy(descID_, stmtID_, HS_STMTID_LENGTH);
    strncat(descID_, "_IVAR", HS_STMTID_LENGTH);
    desc_.identifier = (char *)descID_;
    desc_.identifier_len = strlen((char *)descID_);
    stmt_.handle = 0;
  }

// Constructor used for dynamic interface.
HSinsertHist::HSinsertHist(const char *histTable)
    : tableName_(histTable),
      numRows_(0),
      retcode_(0),
      stmtText_(""),
      stmtAllocated_(FALSE),
      srcDescAllocated_(FALSE),
      inputDescAllocated_(FALSE)
  {
    static SQLMODULE_ID module;
    static NABoolean moduleSet = FALSE;
    static const char stmtName[] = "INSERT_HIST_STMT";

    if (!moduleSet)
      {
        init_SQLMODULE_ID(&module);
        moduleSet = TRUE;
      }

    init_SQLCLI_OBJ_ID(&stmt_, SQLCLI_CURRENT_VERSION, stmt_handle, &module);
    init_SQLCLI_OBJ_ID(&srcDesc_);
    init_SQLCLI_OBJ_ID(&desc_);

    srcDesc_.module = &module;
    desc_.module = &module;

    // ---------------------------------------------------------------------
    // Initialize a descriptor which will hold the SQL statement source
    // ---------------------------------------------------------------------
    srcDesc_.name_mode  = desc_handle;
    srcDesc_.identifier = 0;
    srcDesc_.identifier_len = 0;
    srcDesc_.handle     = 0;

    // ---------------------------------------------------------------------
    // Initialize an input descriptor to deliver parameters
    // ---------------------------------------------------------------------
    desc_.name_mode  = desc_handle;
    desc_.identifier = 0;
    desc_.identifier_len = 0;
    desc_.handle     = 0;
  }

HSinsertHist::~HSinsertHist()
  {
    if (inputDescAllocated_)
      SQL_EXEC_DeallocDesc(&desc_);
    if (srcDescAllocated_)
      SQL_EXEC_DeallocDesc(&srcDesc_);
    if (stmtAllocated_)
      SQL_EXEC_DeallocStmt(&stmt_);
  }

void HSinsertHist::setText()
  {
    char rowsetSizeStr[10];
    snprintf(rowsetSizeStr, sizeof(rowsetSizeStr), "%d", MAX_ROWSET);
    stmtText_ = "ROWSET FOR INPUT SIZE ? "
                "INSERT INTO ";
    stmtText_.append(tableName_)
       .append(" VALUES (");

    for (Int32 i=2; i<25; i++)
      stmtText_.append("?[").append(rowsetSizeStr).append("], ");

    stmtText_.append("?[").append(rowsetSizeStr).append("])");
  }

Lng32 HSinsertHist::prepareDynamic()
  {
    HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR,
                                "HSinsertHist::prepareDynamic()", TRUE);
    HSLogMan *LM = HSLogMan::Instance();

    retcode_ = SQL_EXEC_ClearDiagnostics(&stmt_);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_AllocStmt(&stmt_, 0);
    HSHandleError(retcode_);
    stmtAllocated_ = TRUE;

    retcode_ = SQL_EXEC_AllocDesc(&srcDesc_, 1);
    HSHandleError(retcode_);
    srcDescAllocated_ = TRUE;

    retcode_ = SQL_EXEC_AllocDesc(&desc_, 25);
    HSHandleError(retcode_);
    inputDescAllocated_ = TRUE;

    retcode_ = SQL_EXEC_SetDescItem(&srcDesc_, 1, SQLDESC_TYPE_FS,
                                    REC_BYTE_V_ANSI, 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(&srcDesc_, 1, SQLDESC_VAR_PTR,
                                    (Long)stmtText_.data(), 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(&srcDesc_, 1, SQLDESC_LENGTH,
                                    stmtText_.length() + 1, 0);
    HSHandleError(retcode_);
    // SQLDESC_CHAR_SET must be the last descriptor item set, otherwise
    // it may get reset by other calls to SQL_EXEC_SetDescItem().
    NAString charSet = ActiveSchemaDB()->getDefaults().getValue(ISO_MAPPING);
    NAString defCS   = ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET);
    retcode_ = SQL_EXEC_SetDescItem(&srcDesc_, 1, SQLDESC_CHAR_SET,
                                    SQLCHARSETCODE_UTF8, 0);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_SetStmtAttr(&stmt_, SQL_ATTR_PARENT_QID, 0,
                     (char *)CmpCommon::context()->sqlSession()->getParentQid());
    HSHandleError(retcode_);

    SQL_QUERY_COST_INFO query_cost_info;
    SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;

    LM->StartTimer("SQL_EXEC_Prepare2() from HSinsertHist::prepareDynamic()");
    retcode_ = SQL_EXEC_Prepare2(&stmt_, &srcDesc_, NULL,0,NULL,&query_cost_info, &comp_stats_info,NULL,0,0);
    LM->StopTimer();
    if (retcode_ < 0 && LM->LogNeeded())
      {
        LM->Log("Failure in preparing the following statement:");
        LM->Log(stmtText_.data());
      }
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_DescribeStmt(&stmt_, &desc_, NULL);
    HSHandleError(retcode_);
    return retcode_;
  }

Lng32 HSinsertHist::initialize()
  {
    HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR, "HSinsertHist::initialize()", TRUE);
    retcode_ = SQL_EXEC_ClearDiagnostics(&stmt_);
    HSHandleError(retcode_);

#ifndef NA_USTAT_USE_STATIC  // extra step for dynamic query
    setText();
    retcode_ = prepareDynamic();
    HSHandleError(retcode_);
#endif

    retcode_ = SQL_EXEC_SetRowsetDescPointers(&desc_
                                             ,MAX_ROWSET
                                             ,0
                                             ,1
#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
                                             ,26
#else // NA_USTAT_USE_STATIC not defined, use dynamic query
                                             ,25
#endif

                                             ,0
                                             ,&(numRows_)
                                             ,(void*)0
                                             ,(void*)0

#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
                                             ,0
                                             ,tableName_
                                             ,(void*)0
                                             ,(void*)0
#endif
                                             ,sizeof(Int64)
                                             ,&(tableUid_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(ULng32)
                                             ,&(histID_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Lng32)
                                             ,&(colPosition_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Lng32)
                                             ,&(colNumber_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Lng32)
                                             ,&(colcount_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(signed short)
                                             ,&(intCount_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(rowCount_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(totalUEC_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,TIMESTAMP_CHAR_LEN+1
                                             ,&(statsTime_[0])
                                             ,(void*)0
                                             ,(void*)0

                                             ,250*sizeof(wchar_t)
                                             ,&(lowValue_[0])
                                             ,(void*)0
                                             ,(void*)0

                                             ,250*sizeof(wchar_t)
                                             ,&(hiValue_[0])
                                             ,(void*)0
                                             ,(void*)0

                                             // the following are columns for automation
                                             ,TIMESTAMP_CHAR_LEN+1
                                             ,&(readTime_[0])
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(short)
                                             ,&(readCount_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(sampleSecs_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(colSecs_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(short)
                                             ,&(samplePercent_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(double)
                                             ,&(cv_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(char)
                                             ,&(reason_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(v1_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(v2_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(v3_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(v4_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,250*sizeof(wchar_t)
                                             ,&(v5_[0])
                                             ,(void*)0
                                             ,(void*)0

                                             ,250*sizeof(wchar_t)
                                             ,&(v6_[0])
                                             ,(void*)0
                                             ,(void*)0
                                            );
    HSHandleError(retcode_);

    return retcode_;
  }

Lng32 HSinsertHist::addRow(const Int64 table_uid,
                               const ULng32 histogram_id,
                               const Lng32 col_position,
                               const Lng32 column_number,
                               const Lng32 colcount,
                               const short interval_count,
                               const Int64 rowcount,
                               const Int64 total_uec,
                               const char *stats_time,
                               const HSDataBuffer &low_value,
                               const HSDataBuffer &high_value,
                               // the following columns are for automation
                               const char *read_time,
                               const short read_count,
                               const Int64 sample_secs,
                               const Int64 col_secs,
                               const short sample_percent,
                               const double cv,
                               const char reason,
                               const Int64 v1,
                               const Int64 v2,
                               const Int64 v3,
                               const Int64 v4,
                               const HSDataBuffer &v5,
                               const HSDataBuffer &v6)
  {
    Lng32 retcode = 0;

    tableUid_[numRows_] = table_uid;
    histID_[numRows_] = histogram_id;
    colPosition_[numRows_] = col_position;
    colNumber_[numRows_] = column_number;
    colcount_[numRows_] = colcount;
    intCount_[numRows_] = interval_count;
    rowCount_[numRows_] = rowcount;
    totalUEC_[numRows_] = total_uec;
    strcpy(&statsTime_[numRows_][0], stats_time);

    // the following columns are for automation
    strcpy(&readTime_[numRows_][0], read_time);
    readCount_[numRows_] = read_count;
    sampleSecs_[numRows_] = sample_secs;
    colSecs_[numRows_] = col_secs;
    samplePercent_[numRows_] = sample_percent;
    cv_[numRows_] = cv;
    reason_[numRows_] = reason;
    v1_[numRows_] = v1;
    v2_[numRows_] = v2;
    v3_[numRows_] = v3;
    v4_[numRows_] = v4;
    v5_[numRows_][0] = v5.length();
    memmove(&v5_[numRows_][1], v5.data(), v5.length());
    v6_[numRows_][0] = v6.length();
    memmove(&v6_[numRows_][1], v6.data(), v6.length());

    //set low_value: LEN + DATA
    lowValue_[numRows_][0] = low_value.length();
    memmove(&lowValue_[numRows_][1], low_value.data(), low_value.length());

    //set high_value: LEN + DATA
    hiValue_[numRows_][0] = high_value.length();
    memmove(&hiValue_[numRows_][1], high_value.data(), high_value.length());

    if (++numRows_ == MAX_ROWSET)
      {
        retcode = flush();
        HSHandleError(retcode);
      }

    return retcode;
  }


Lng32 HSinsertHist::flush()
  {
    HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR, "HSinsertHist::flush()", TRUE);
    HSLogMan *LM = HSLogMan::Instance();

    if (numRows_ > 0 && retcode_ == 0)
      {
        retcode_ = SQL_EXEC_ExecFetch(&stmt_,&desc_,0);
        if (LM->LogNeeded())
          {
            snprintf(LM->msg, sizeof(LM->msg), "HISTOGRAM ROWSET INSERT of %d rows. Retcode=(%d)", numRows_, retcode_);
            LM->Log(LM->msg);
            print();
          }
        HSHandleError(retcode_);

        numRows_ = 0;
      }

    return retcode_;
  }

void HSinsertHist::print()
  {
    char tableUidStr[30];
    char rowCountStr[30];
    char uecStr[30];
    NAString low, high;
    // the following are for automation
    char sampleSecsStr[30];
    char colSecsStr[30];
    //char v1[30],v2[30],v3[30],v4[30];
    //NAString v5, v6;

    HSLogMan *LM = HSLogMan::Instance();

    for (Lng32 i = 0; i < numRows_; i++)
      {
        convertInt64ToAscii(tableUid_[i], tableUidStr);
        convertInt64ToAscii(rowCount_[i], rowCountStr);
        convertInt64ToAscii(totalUEC_[i], uecStr);
        ConvWcharToHexadecimal(&lowValue_[i][1], lowValue_[i][0] / 2, low);
        ConvWcharToHexadecimal(&hiValue_[i][1], hiValue_[i][0] / 2, high);
        // the following are for automation
        convertInt64ToAscii(sampleSecs_[i], sampleSecsStr);
        convertInt64ToAscii(colSecs_[i], colSecsStr);
        //convertInt64ToAscii(v1_[i], v1);
        //convertInt64ToAscii(v2_[i], v2);
        //convertInt64ToAscii(v3_[i], v3);
        //convertInt64ToAscii(v4_[i], v4);
        //ConvWcharToHexadecimal(&v5_[i][1], v5_[i][0] / 2, v5);
        //ConvWcharToHexadecimal(&v6_[i][1], v6_[i][0] / 2, v6);

        snprintf(LM->msg, sizeof(LM->msg), "\t( %s, %u, %d, %d, %d, %d, %s, %s, %s, %s, %s, %s, %d, %s, %s, %d, %f, %c)",
                            tableUidStr,
                            histID_[i],
                            colPosition_[i],
                            colNumber_[i],
                            colcount_[i],
                            intCount_[i],
                            rowCountStr,
                            uecStr,
                            statsTime_[i],
                            low.data(),
                            high.data(),
                            // the following are for automation
                            readTime_[i],
                            readCount_[i],
                            sampleSecsStr,
                            colSecsStr,
                            samplePercent_[i],
                            cv_[i],
                            reason_[i]
                            //v1,v2,v3,v4,v5.data(),v6.data()
               );
        LM->Log(LM->msg);
      }
  }




HSinsertHistint::HSinsertHistint(const char *stmtID,
                                 const char *histIntTable)
    : tableName_(histIntTable),
      numRows_(0),
      retcode_(0),
      stmtText_(""),
      stmtAllocated_(FALSE),
      srcDescAllocated_(FALSE),
      inputDescAllocated_(FALSE)
  {
    static SQLMODULE_ID module;
    static char moduleName[HS_MODULE_LENGTH];
    static NABoolean moduleSet = FALSE;

    if (!moduleSet)
      {

        init_SQLMODULE_ID(&module);
        strncpy(moduleName, HS_MODULE, HS_MODULE_LENGTH);

        module.module_name = (char *)moduleName;
        module.module_name_len = strlen((char*)moduleName);
        module.creation_timestamp = 1234567890;
        moduleSet = TRUE;
      }

    init_SQLCLI_OBJ_ID(&stmt_);
    stmt_.name_mode = stmt_name;
    stmt_.module = &module;
    strncpy(stmtID_, stmtID, HS_STMTID_LENGTH);
    stmt_.identifier = (char *)stmtID_;
    stmt_.identifier_len = strlen((char *)stmtID_);
    stmt_.handle = 0;


    init_SQLCLI_OBJ_ID(&desc_);
    desc_.name_mode = desc_name;
    desc_.module = &module;
    strncpy(descID_, stmtID_, HS_STMTID_LENGTH);
    strncat(descID_, "_IVAR", HS_STMTID_LENGTH);
    desc_.identifier = (char *)descID_;
    desc_.identifier_len = strlen((char *)descID_);
    stmt_.handle = 0;
  }

// Ctor used for dynamic version of statement.
HSinsertHistint::HSinsertHistint(const char *histIntTable)
    : tableName_(histIntTable),
      numRows_(0),
      retcode_(0),
      stmtText_(""),
      stmtAllocated_(FALSE),
      srcDescAllocated_(FALSE),
      inputDescAllocated_(FALSE)
  {
    static SQLMODULE_ID module;
    static NABoolean moduleSet = FALSE;
    static const char stmtName[] = "INSERT_HISTINT_STMT";

    if (!moduleSet)
      {
        init_SQLMODULE_ID(&module);
        moduleSet = TRUE;
      }

    init_SQLCLI_OBJ_ID(&stmt_, SQLCLI_CURRENT_VERSION,
                       stmt_name, &module, stmtName, 0,
                       SQLCHARSETSTRING_ISO88591, (Lng32)strlen(stmtName), 0);
    init_SQLCLI_OBJ_ID(&srcDesc_);
    init_SQLCLI_OBJ_ID(&desc_);

    srcDesc_.module = &module;
    desc_.module = &module;

    // ---------------------------------------------------------------------
    // Initialize a descriptor which will hold the SQL statement source
    // ---------------------------------------------------------------------
    srcDesc_.name_mode  = desc_handle;
    srcDesc_.identifier = 0;
    srcDesc_.identifier_len = 0;
    srcDesc_.handle     = 0;

    // ---------------------------------------------------------------------
    // Initialize an input descriptor to deliver parameters
    // ---------------------------------------------------------------------
    desc_.name_mode  = desc_handle;
    desc_.identifier = 0;
    desc_.identifier_len = 0;
    desc_.handle     = 0;
  }


HSinsertHistint::~HSinsertHistint()
  {
    if (inputDescAllocated_)
      SQL_EXEC_DeallocDesc(&desc_);
    if (srcDescAllocated_)
      SQL_EXEC_DeallocDesc(&srcDesc_);
    if (stmtAllocated_)
      SQL_EXEC_DeallocStmt(&stmt_);
  }

void HSinsertHistint::setText()
  {
    char rowsetSizeStr[10];
    snprintf(rowsetSizeStr, sizeof(rowsetSizeStr), "%d", MAX_ROWSET);
    stmtText_ = "ROWSET FOR INPUT SIZE ? "
                "INSERT INTO ";
    stmtText_.append(tableName_)
       .append(" VALUES (");

    for (Int32 i=2; i<14; i++)
      {
        if (i == 8)
          // cast std_dev_of_freq as a float, since that's the client data type
          stmtText_.append("CAST(?[").append(rowsetSizeStr).append("] AS FLOAT), ");
        else
          stmtText_.append("?[").append(rowsetSizeStr).append("], ");
      }

    stmtText_.append("?[").append(rowsetSizeStr).append("])");
  }

Lng32 HSinsertHistint::prepareDynamic()
  {
    HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR,
                                "HSinsertHistint::prepareDynamic()", TRUE);
    HSLogMan *LM = HSLogMan::Instance();

    retcode_ = SQL_EXEC_ClearDiagnostics(&stmt_);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_AllocStmt(&stmt_, 0);
    HSHandleError(retcode_);
    stmtAllocated_ = TRUE;

    retcode_ = SQL_EXEC_AllocDesc(&srcDesc_, 1);
    HSHandleError(retcode_);
    srcDescAllocated_ = TRUE;

    retcode_ = SQL_EXEC_AllocDesc(&desc_, 14);
    HSHandleError(retcode_);
    inputDescAllocated_ = TRUE;

    retcode_ = SQL_EXEC_SetDescItem(&srcDesc_, 1, SQLDESC_TYPE_FS,
                                    REC_BYTE_V_ANSI, 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(&srcDesc_, 1, SQLDESC_VAR_PTR,
                                    (Long)stmtText_.data(), 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(&srcDesc_, 1, SQLDESC_LENGTH,
                                    stmtText_.length() + 1, 0);
    HSHandleError(retcode_);
    // SQLDESC_CHAR_SET must be the last descriptor item set, otherwise
    // it may get reset by other calls to SQL_EXEC_SetDescItem().
    NAString charSet = ActiveSchemaDB()->getDefaults().getValue(ISO_MAPPING);
    NAString defCS   = ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET);
    retcode_ = SQL_EXEC_SetDescItem(&srcDesc_, 1, SQLDESC_CHAR_SET,
                                    SQLCHARSETCODE_UTF8, 0);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_SetStmtAttr(&stmt_, SQL_ATTR_PARENT_QID, 0,
            (char *)CmpCommon::context()->sqlSession()->getParentQid());
    HSHandleError(retcode_);

    SQL_QUERY_COST_INFO query_cost_info;
    SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;

    LM->StartTimer("SQL_EXEC_Prepare2() from HSinsertHistint::prepareDynamic()");
    retcode_ = SQL_EXEC_Prepare2(&stmt_, &srcDesc_, NULL,0,NULL,&query_cost_info, &comp_stats_info,NULL,0,0);
    LM->StopTimer();
    if (retcode_ < 0 && LM->LogNeeded())
      {
        LM->Log("Failure in preparing the following statement:");
        LM->Log(stmtText_.data());
      }
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_DescribeStmt(&stmt_, &desc_, NULL);
    HSHandleError(retcode_);

    return retcode_;
  }


Lng32 HSinsertHistint::initialize()
  {
    HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR, "HSinsertHistint::initialize()", TRUE);
    retcode_ = SQL_EXEC_ClearDiagnostics(&stmt_);
    HSHandleError(retcode_);

#ifndef NA_USTAT_USE_STATIC  // extra step for dynamic query
    setText();
    retcode_ = prepareDynamic();
    HSHandleError(retcode_);
#endif

    // histogram versioning
    if (HSGlobalsClass::schemaVersion >= COM_VERS_2300) {
      retcode_ = SQL_EXEC_SetRowsetDescPointers(&desc_
                                             ,MAX_ROWSET
                                             ,0
                                             ,1
#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
                                             ,15
#else // NA_USTAT_USE_STATIC not defined, use dynamic query
                                             ,14
#endif
                                             ,0
                                             ,&(numRows_)
                                             ,(void*)0
                                             ,(void*)0

#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
                                             ,0
                                             ,tableName_
                                             ,(void*)0
                                             ,(void*)0
#endif
                                             ,sizeof(Int64)
                                             ,&(tableUid_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(ULng32)
                                             ,&(histID_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(signed short)
                                             ,&(intNumber_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(intRowcount_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(intUec_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,250*sizeof(wchar_t)
                                             ,&(intBoundary_[0])
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(double)
                                             ,&(stdDevOfFreq_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(intMFVrowcount_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(intMFV2rowcount_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(v3_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(v4_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,250*sizeof(wchar_t)
                                             ,&(mostFreqVal_[0])
                                             ,(void*)0
                                             ,(void*)0

                                             ,250*sizeof(wchar_t)
                                             ,&(v6_[0])
                                             ,(void*)0
                                             ,(void*)0
                                            );
    } else {
      retcode_ = SQL_EXEC_SetRowsetDescPointers(&desc_
                                             ,MAX_ROWSET
                                             ,0
                                             ,1
                                             ,8

                                             ,0
                                             ,&(numRows_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,0
                                             ,tableName_
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(tableUid_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(ULng32)
                                             ,&(histID_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(signed short)
                                             ,&(intNumber_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(intRowcount_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,sizeof(Int64)
                                             ,&(intUec_)
                                             ,(void*)0
                                             ,(void*)0

                                             ,250*sizeof(NAWchar)
                                             ,&(intBoundary_[0])
                                             ,(void*)0
                                             ,(void*)0
                                            );
    }
    HSHandleError(retcode_);
    return retcode_;
  }

Lng32 HSinsertHistint::addRow(const Int64 table_uid,
                             const ULng32 histogram_id,
                             const short interval_number,
                             const Int64 interval_rowcount,
                             const Int64 interval_uec,
                             const HSDataBuffer &interval_boundary,
                             // the following are added in R2.3
                             const double std_dev_of_freq,
                             const Int64 interval_MFV_rowcount,
                             const Int64 interval_MFV2_rowcount,
                             const Int64 v3,
                             const Int64 v4,
                             const HSDataBuffer &mostFreqVal,
                             const HSDataBuffer &v6)
  {
    Lng32 retcode = 0;

    tableUid_[numRows_] = table_uid;
    histID_[numRows_] = histogram_id;
    intNumber_[numRows_] = interval_number;
    intRowcount_[numRows_] = interval_rowcount;
    intUec_[numRows_] = interval_uec;

    //set boundary_value: LEN + DATA
    intBoundary_[numRows_][0] = interval_boundary.length();
    memmove(&intBoundary_[numRows_][1], interval_boundary.data(), interval_boundary.length());

    stdDevOfFreq_[numRows_] = std_dev_of_freq;
    intMFVrowcount_[numRows_] = interval_MFV_rowcount;
    intMFV2rowcount_[numRows_] = interval_MFV2_rowcount;
    v3_[numRows_] = v3;
    v4_[numRows_] = v4;

    mostFreqVal_[numRows_][0] = mostFreqVal.length();
    memmove(&mostFreqVal_[numRows_][1], mostFreqVal.data(), mostFreqVal.length());
    v6_[numRows_][0] = v6.length();
    memmove(&v6_[numRows_][1], v6.data(), v6.length());

    if (++numRows_ == MAX_ROWSET)
      {
        retcode = flush();
        HSHandleError(retcode);
      }

    return retcode;
  }


Lng32 HSinsertHistint::flush()
  {
    HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR, "HSinsertHistint::flush()", TRUE);
    HSLogMan *LM = HSLogMan::Instance();

    if (numRows_ > 0 && retcode_ == 0)
      {
        retcode_ = SQL_EXEC_ExecFetch(&stmt_,&desc_,0);
        if (LM->LogNeeded())
          {
            snprintf(LM->msg, sizeof(LM->msg), "HISTOGRAM_INTERVALS ROWSET INSERT of %d rows. Retcode=(%d)", numRows_, retcode_);
            LM->Log(LM->msg);
            print();
          }
        HSHandleError(retcode_);

        numRows_ = 0;
      }
    return retcode_;
  }

void HSinsertHistint::print()
  {
    HSLogMan *LM = HSLogMan::Instance();
    char tableUidStr[30];
    char rowCountStr[30];
    char uecStr[30];
    char MFVrowCountStr[30];
    char MFV2rowCountStr[30];
    NAString bound;
    NAString mostFreqVal;
    // the following are for new columns added in R2.3
    //char v1Str[30], v2Str[30], v3Str[30], v4Str[30];
    //NAString v5, v6;

    for (Lng32 i = 0; i < numRows_; i++)
      {
        convertInt64ToAscii(tableUid_[i], tableUidStr);
        convertInt64ToAscii(intRowcount_[i], rowCountStr);
        convertInt64ToAscii(intUec_[i], uecStr);
        ConvWcharToHexadecimal(&intBoundary_[i][1], intBoundary_[i][0] / 2, bound);
        convertInt64ToAscii(intMFVrowcount_[i], MFVrowCountStr);
        convertInt64ToAscii(intMFV2rowcount_[i], MFV2rowCountStr);
        //convertInt64ToAscii(v3_[i], v3Str);
        //convertInt64ToAscii(v4_[i], v4Str);
        ConvWcharToHexadecimal(&mostFreqVal_[i][1], mostFreqVal_[i][0] / 2, mostFreqVal);
        //ConvWcharToHexadecimal(&v6_[i][1], v6_[i][0] / 2, v6);

        snprintf(LM->msg, sizeof(LM->msg), "\t( %s, %u, %d, %s, %s, %s, %f, %s, %s, %s)",
                            tableUidStr,
                            histID_[i],
                            intNumber_[i],
                            rowCountStr,
                            uecStr,
                            bound.data(),
                            stdDevOfFreq_[i],
                            MFVrowCountStr,
                            MFV2rowCountStr,
                            //v3Str,
                            //v4Str,
                            mostFreqVal.data()
                            //v6.data()
               );
        LM->Log(LM->msg);
      }
  }

/**********************************************************/
/* METHOD:  addColumn(const Lng32 columnNumber)            */
/* PURPOSE: This method is used to add a column in a set  */
/*          of columns that will be used to construct     */
/*          empty multi-column histograms                 */
/* PARAMS:  columnNumber - the column number to be added  */
/* RETCODE:  0 - successful                               */
/*          -1 - failure                                  */
/**********************************************************/
Lng32 HSinsertEmptyHist::addColumn(const Lng32 columnNumber)
  {
    HSLogMan *LM = HSLogMan::Instance();
    if (colCount_ >= MAX_MC_COLUMNS)
      {
         if (LM->LogNeeded())
          {
            snprintf(LM->msg, sizeof(LM->msg), "\t***[ERROR] HSinsertEmptyHist::addColumn: ColCount reaches maximum(%d)\n",
              MAX_MC_COLUMNS);
            LM->Log(LM->msg);
          }
        return -1;
      }

    colNumber_[colCount_++] = columnNumber;
    if (LM->LogNeeded())
      {
        snprintf(LM->msg, sizeof(LM->msg), "\tHSinsertEmptyHist::addColumn: add a colume(%d)\n",
          columnNumber);
        LM->Log(LM->msg);
      }
    return 0;
  }

/**********************************************************/
/* METHOD:  insert()                                      */
/* PURPOSE: This method is used to insert empty           */
/*          multi-column histograms into the Histograms   */
/*          table based on the columns in colNumber_.     */
/* RETCODE:  0 - successful                               */
/*          -1 - failure                                  */
/* NOTES:   If the empty histograms exist, will not       */
/*          insert again.                                 */
/**********************************************************/
Lng32 HSinsertEmptyHist::insert()
  {
    HSGlobalsClass::schemaVersion = getTableSchemaVersion(histTable_);
    Lng32 retcode = 0;
    ULng32 histid = 0;
    Lng32 colPos = 0;
    Lng32 colNum = 0;
    Lng32 colCnt = 0;
    char reason = HS_REASON_UNKNOWN;
    char readTime[TIMESTAMP_CHAR_LEN+1]; // not used, but retrieved by cursor used
    ULng32 maxHistid = 0;
    ULng32 prevHistid = 0;
    Lng32 matchCount = 0;
    DefaultToken jit;
    HSLogMan *LM = HSLogMan::Instance();

    if (colCount_ == 0) return 0;  // it is ok, but do nothing
    Lng32 maxMCWidthForAutomation = (Lng32) CmpCommon::getDefaultLong(USTAT_AUTO_MC_MAX_WIDTH);
    if (colCount_ > maxMCWidthForAutomation)
    {
      if (LM->LogNeeded())
      {
        snprintf(LM->msg, sizeof(LM->msg), "HSinsertEmptyHist::insert: Skipping insert of MC stats for Table(%s), ColCount(%d) > CQD(%d)\n",
                tableName_, colCount_, maxMCWidthForAutomation);
        LM->Log(LM->msg);
      }
      return 0;
    }

    if ((jit = CmpCommon::getDefault(USTAT_JIT_LOGGING)) == DF_ON) // If JIT logging is on, turn it off here.
      HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_JIT_LOGGING 'OFF'"); // do not check for error

    if (LM->LogNeeded())
      {
        snprintf(LM->msg, sizeof(LM->msg), "\nHSinsertEmptyHist::insert: Table(%s), Histograms(%s), ColCount(%d)\n",
          tableName_, histTable_, colCount_);
        LM->Log(LM->msg);
      }

    LM->LogTimeDiff("START GET EXISTING HISTOGRAMS BEFORE INSERT EMPTY ONES");
    HSTranController TC("INSERT EMPTY HISTOGRAMS", &retcode);
    HSLogError(retcode);
    // find max hist id and check existing empty histogram
#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
    HSCliStatement::statementIndex stmt = HSCliStatement::CURSOR104_MX_2300;
    HSCliStatement cursor( stmt,
                           (char *)histTable_,
                           (char *)&tableUID_
                         );
#else // NA_USTAT_USE_STATIC not defined, use dynamic query
    char sbuf[25];
    NAString qry = "SELECT HISTOGRAM_ID, COL_POSITION, COLUMN_NUMBER, COLCOUNT, "
                          "cast(READ_TIME as char(19) character set iso88591), REASON "
                   "FROM ";
    qry.append(histTable_);
    qry.append(    " WHERE TABLE_UID = ");
    snprintf(sbuf, sizeof(sbuf), PF64, tableUID_);
    qry.append(sbuf);
    qry.append(    " ORDER BY TABLE_UID, HISTOGRAM_ID, COL_POSITION ");
    qry.append(    " FOR READ UNCOMMITTED ACCESS");

    HSCursor cursor;
    retcode = cursor.prepareQuery(qry.data(), 0, 6);
    HSLogError(retcode);
#endif // NA_USTAT_USE_STATIC not defined
    retcode = cursor.open();
    HSLogError(retcode);

    // results should be ordered by hist id and col position
    // check if the same histograms exist and get max hist id
    while ( 0 == (retcode = cursor.fetch(6, (void *)&histid, (void *)&colPos,
                                        (void *)&colNum, (void *)&colCnt,
                                        (void *)&readTime, (void *)&reason)) )
      {
        maxHistid = MAXOF(maxHistid, histid);
        if (histid != prevHistid) matchCount = 0; // reset matchcount if new histogram.
        if (colCnt == colCount_) // found histogram with same # cols.
          for (Lng32 i=0; i<colCount_; i++)
            if (colNumber_[i] == colNum) { matchCount++; break; } // Hist contains same col.
        if (matchCount == colCount_) break;   // Histogram to insert already exists.

        prevHistid = histid;
      }

    if (retcode != HS_EOF) HSLogError(retcode);

    retcode = cursor.close();
    HSLogError(retcode);

    if (CmpCommon::getDefault(USTAT_AUTO_EMPTYHIST_TWO_TRANS) == DF_ON)
      TC.stopStart("INSERT EMPTY HIST - END SELECT START INSERT");
    LM->LogTimeDiff("END GET EXISTING HISTOGRAMS BEFORE INSERT EMPTY ONES");

    // do not insert empty histograms if they exist
    if (matchCount == colCount_ || retcode < 0)
      {
        if (LM->LogNeeded() && matchCount == colCount_)
          {
            snprintf(LM->msg, sizeof(LM->msg), "\tThere are existing empty histograms for Hist Id(%d). No new empty histograms will be inserted.\n",
              histid);
            LM->Log(LM->msg);
          }
        if (jit == DF_ON) // Turn JIT logging back on if it was ON when function was entered.
          HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_JIT_LOGGING 'ON'"); // do not check for error
        return retcode;
      }

    LM->LogTimeDiff("START INSERT EMPTY HISTOGRAMS");

#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
    HSinsertHist* histRS=new HSinsertHist("INSERT104_MX_2300", histTable_);
#else // NA_USTAT_USE_STATIC not defined, use dynamic query
    HSinsertHist* histRS=new HSinsertHist(histTable_);
#endif // NA_USTAT_USE_STATIC not defined

    // The format expected for an input parameter that is a timestamp value
    // depends on the INTERNAL_FORMAT_IO session default. Sqlci sets this to 1
    // (meaning use internal format), which affects us if running from sqlci
    // and using the embedded compiler. We switch to external format if necessary,
    // to match the constant we provide, and switch back afterwards. It must be
    // set before calling initialize(), because that includes compiling the
    // statement, which sets up its input descriptor.
    SessionDefaults* sessionDefaults = GetCliGlobals()->currContext()->getSessionDefaults();
    NABoolean usingInternalFormat = sessionDefaults->getInternalFormatIO();
    if (usingInternalFormat)
      sessionDefaults->setInternalFormatIO(FALSE);

    try
    {
      retcode = histRS->initialize();

      if (maxHistid ==0)
        {
          Int64 timeStamp = NA_JulianTimestamp();
          maxHistid = (ULng32) (timeStamp & 0x7FFFFFFF); //histogram ID = Julian Timestamp masked with 0x7FFFFFFF
        }
      else
        // Since we are in a transaction, simply add a 5 to the max histid found.
        maxHistid += 5;

      Lng32 histId = maxHistid;

      const char* statsTime = "0001-01-01 00:00:00";  // external format
      HSDataBuffer lval(L"()");
      HSDataBuffer hval(L"()");
      //HSDataBuffer v5(L"");
      //HSDataBuffer v6(L"");
      for (Lng32 i = 0; i < colCount_; i++)
        {
          retcode = histRS->addRow( tableUID_,
                                    histId,
                                    i, // col position
                                    colNumber_[i], // position in table
                                    colCount_, // number of columns
                                    0,
                                    0,
                                    0,
                                    statsTime,
                                    lval,
                                    hval,
                                    statsTime,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    HS_REASON_EMPTY);
                                    //0,0,0,0,v5,v6);
          HSLogError(retcode);
          if (LM->LogNeeded())
            {
              snprintf(LM->msg, sizeof(LM->msg), "\tAdd an empty histogram: HistID(%d),ColPos(%d),ColNum(%d),ColCount(%d),retcode(%d)\n",
                histId, i, colNumber_[i], colCount_, retcode);
              LM->Log(LM->msg);
            }
        }  // for

      retcode = histRS->flush();
      HSLogError(retcode);
      if (LM->LogNeeded())
        {
          snprintf(LM->msg, sizeof(LM->msg), "Flush empty histograms: retcode(%d)\n", retcode);
          LM->Log(LM->msg);
        }

      // Switch back to internal timestamp format if we changed it above.
      if (usingInternalFormat)
        sessionDefaults->setInternalFormatIO(TRUE);
      delete histRS;
      histRS = NULL;
    }
    catch(...)
    {
      // Make sure session value is reset and histRS is deleted in case of an exception.
      if (usingInternalFormat)
        sessionDefaults->setInternalFormatIO(TRUE);
      delete histRS;
      throw;
    }

    LM->LogTimeDiff("END INSERT EMPTY HISTOGRAMS");

    if (jit == DF_ON) // Turn JIT logging back on if it was ON when function was entered.
      HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_JIT_LOGGING 'ON'"); // do not check for error
    return retcode;
  }

Lng32 HSDataBuffer::addParenthesis()
  {
    Lng32 retcode = 0;
    NAWString tempBuf(WIDE_("("));
    tempBuf.append((NAWchar*)this->data(), this->length() / sizeof(NAWchar));
    tempBuf.append(WIDE_(")"));

    if (tempBuf.length() * sizeof(NAWchar) > HS_MAX_BOUNDARY_LEN)
      {
        retcode = -1;
        HSHandleError(retcode);
      }
    else
      *this = tempBuf.data();

    return retcode;
  }

Lng32 HSDataBuffer::append(HSDataBuffer &dataBuffer)
  {
    Lng32 retcode = 0;
    NAWString tempBuf(WIDE_(""));
    tempBuf.append((NAWchar*)this->data(), this->length() / sizeof(NAWchar));

    tempBuf.append((NAWchar*)dataBuffer.data(), dataBuffer.length() / sizeof(NAWchar));

    if (tempBuf.length() * sizeof(NAWchar) > HS_MAX_BOUNDARY_LEN)
      {
        retcode = -1;
        HSHandleError(retcode);
      }
    else
      *this = tempBuf.data();

    return retcode;
  }

/*******************************************************/
/* METHOD : getLocalNodeName()                         */
/*                                                     */
/* PURPOSE : get the name of the local node by making  */
/* a Guardian call. Returns NULL if there is an error  */
/* This method is almost identical to a part of        */
/* CatExecCollectNodeInfo function                     */
/* in catman/CatExecCommon.cpp. It is being duplicated */
/* here to avoid linking catman code here.             */
/*******************************************************/
NAString* getLocalNodeName()
{

    char localNodeName[9];
    short actualNodeNameLen = 0;
    short retcode;

    //The NODENUMBER_TO_NODENAME_ procedure converts a node number (system
    //number) to the corresponding node name (system name). It can also be used to
    //obtain the name of the caller�s node, by passing -1 for the system number,
    // the first parameter in the call.
    retcode = NODENUMBER_TO_NODENAME_(-1,
                                      localNodeName,
                                      9-1, // leave room for NUL
                                      &actualNodeNameLen);

    if (retcode || actualNodeNameLen==0)
    {
      return NULL;
    }

    localNodeName[actualNodeNameLen]='\0';

    NAString *localNode = new (STMTHEAP) NAString(localNodeName, STMTHEAP);
    localNode->toUpper();
    return localNode;
}

/*****************************************************/
/* METHOD:  getTempTablePartitionInfo()              */
/*                                                   */
/* PURPOSE: Based on the HIST_SCRATCH_VOL attribute, */
/*          appropriate syntax and CQD settings are  */
/*          are used to create a hash partitioned    */
/*          temporary table where necessary.         */
/*                                                   */
/* INPUT: 'unpartitionedSample' - an NABoolean which */
/*          if TRUE, specifies that the sample table */
/*          should not be partitioned.               */
/*        'isPersSample' - NABoolean which is TRUE   */
/*         if the method is called to create persistent*/
/*         sample table                              */
/* Exceptions: For SQL/MP tables, only the first     */
/*          value in the attribute is used to specify*/
/*          the location of the single partitioned   */
/*          temporary table.                         */
/*          For SQL/MX base tables that contain a    */
/*          system generated key, SYSKEY, only the   */
/*          first value in the attribute is used to  */
/*          specify the location of the single       */
/*          partitioned temporary table.             */
/*****************************************************/
NAString HSSample::getTempTablePartitionInfo(NABoolean unpartitionedSample,
                                             NABoolean isPersSample)
  {
    NAString tableOptions = "";
    Lng32 partitionCount = 0 ;
    Lng32 numScratchVols = 0;
    Lng32 numPartitionsNeeded = 0;
    char tempStr[4200];
    const Lng32 MaxNumVols = 256;  // sized to hold max possible vols
    char *scratchVols[MaxNumVols];
    char *thisVol;
    Lng32 volIx;
    NABoolean userDefinedScratchVols = FALSE;
    NABoolean usePOS = ((CmpCommon::getDefault(POS) == DF_OFF) ? FALSE : TRUE);
    NABoolean usePOSMultiNode = FALSE ;
    NABoolean usePOSLocalNodeWithTableSize = FALSE ;
    Lng32 sampleTableSizeInMB = 0;
    Lng32 scratchVolThreshold = 10240000;
    if (isPersSample)
      scratchVolThreshold = CmpCommon::getDefaultLong(HIST_FETCHCOUNT_SCRATCH_VOL_THRESHOLD);
    else
      scratchVolThreshold = CmpCommon::getDefaultLong(HIST_SCRATCH_VOL_THRESHOLD);

    HSLogMan *LM = HSLogMan::Instance();
    LM->Log("Creating partitioning scheme for sample table.");

    //Allow user to specify volume location(s) for temporary table
    //through HIST_SCRATCH_VOL.
    //
    //10-050217-4819: Make sure the LOCATION clause is either before or
    //after the ATTRIBUTE clause. Normally, we would place it before the
    //ATTRIBUTE clause, but since it is not often used, I'm putting it after.
    //
    //CASE: 10-041025-8056
    //Make sure the temporary table is not restricted by the primary
    //partitions's extent sizes. The temporary table size should be
    //unrestricted, except for limited disk space on the volume.


                                   /*=========================================*/
                                   /*  RETRIEVE USER-DEFINED SCRATCH VOLUMES  */
                                   /*=========================================*/
    CmpCommon::getDefault(HIST_SCRATCH_VOL, tableOptions);
    HS_ASSERT(tableOptions.length() <= 4096);
    strcpy(tempStr, tableOptions.data());

    tableOptions = "";

    if (strlen(tempStr))
      {
        const char *sep = " ,:" ;
        char *token = strtok(tempStr, sep);
                                              /*   STORE VOLUMES IN ARRAY     */
        while (token != NULL &&
               numScratchVols < MaxNumVols)
          {
            scratchVols[numScratchVols++] = token;
            token = strtok(NULL, sep);
          }

        //Even after parsing the CQD for volume tokens, it is still possible
        //to end up with an empty volume list.
        if (numScratchVols > 0)
          userDefinedScratchVols = TRUE;
      }

                                   /*=========================================*/
                                   /*   DETERMINE HOW MANY PARITIONS NEEDED   */
                                   /*=========================================*/
    // We cannot use hash partitioning scheme when,
    // 1. this is an MP tables
    // 2. this table has a primary key containing only a system generated column, SYSKEY.
    // 3. if USEPOS is false and there are no user defined scratch volumes.
    // 4. The table is an hbase table.
    // In any of these case, the sample table will be single partitioned.
    if (objDef->getObjectFormat() == SQLMP
        // Set by HSTableDef::getLabelInfo().
        || (usePOS == FALSE && userDefinedScratchVols == FALSE)
        || HSGlobalsClass::isTrafodionCatalog(objDef->getCatName()) )
     {
        numPartitionsNeeded = 1;

        if (objDef->hasSyskey())
          {
            usePOS = FALSE;
            LM->Log("SYSKEY primary key - sample table will not have partitions.");
            HSFuncExecQuery("CONTROL QUERY DEFAULT POS 'OFF'");
          }
      }
    else
     // if the table is a native HBase or Hive table
     if ( HSGlobalsClass::isNativeCat(objDef->getCatName()) )
     {

           usePOS = FALSE;

           numPartitionsNeeded = (Lng32) ceil((double)sampleRowCount
                                                     *
                                           objDef->getRecordLength()
                                                     /
                                           scratchVolThreshold
                                         );

           NADefaults &defs = CmpCommon::context()->schemaDB_->getDefaults();

           NABoolean fakeEnv= FALSE;
           ComUInt32 numConfiguredESPs = defs.getTotalNumOfESPsInCluster(fakeEnv);

           numPartitionsNeeded = MINOF(numConfiguredESPs, numPartitionsNeeded);
           numPartitionsNeeded = MAXOF(1, numPartitionsNeeded);

           LM->Log("SYSKEY primary key.");

           snprintf(LM->msg, sizeof(LM->msg), "Partitions Needed: %d (TableType=%s, ~SampleSet=%d, RecLen=%d, threshold=%d)"
                            , numPartitionsNeeded
                            , "NATIVE"
                            , (Lng32)sampleRowCount
                            , (Lng32)objDef->getRecordLength()
                            , scratchVolThreshold);
  
           LM->Log(LM->msg);

           if ( numPartitionsNeeded > 1 ) {
               tableOptions +=" salt using ";
               snprintf(tempStr, sizeof(tempStr), "%d", numPartitionsNeeded);
               tableOptions += tempStr;
               tableOptions +=" partitions on( ";
               NAFileSet *naSet = objDef->getNATable()->getClusteringIndex();
               tableOptions +=  naSet->getBestPartitioningKeyColumns(',');
               tableOptions +=" ) ";
           }

           return tableOptions;

        } else {

           numPartitionsNeeded = (Lng32) ceil((double)sampleRowCount
                                                     *
                                           objDef->getRecordLength()
                                                     /
                                           scratchVolThreshold
                                         );

           // For both IUS and RUS, make sure the number of partitions of the source table
           // is a multiple of numPartitionsNeeded.
           if ( CmpCommon::getDefault(USTAT_USE_GROUPING_FOR_SAMPLING) == DF_ON )
           {
   
             Lng32 tblPartns = objDef->getNumPartitions();
   
             if ( tblPartns < numPartitionsNeeded )
               numPartitionsNeeded = tblPartns;
   
             if ( numPartitionsNeeded > 0 )
               while ( tblPartns % numPartitionsNeeded != 0 )
                 numPartitionsNeeded++;
   
             // Force to take the default code path of local node POS mode
             // (which is identical to multi-node POS for SQ) to set the number of
             // partitions for POS. That is, the # of partns of the sample table
             // is specified via CQD POS_NUM_OF_PARTNS.
             usePOSMultiNode = FALSE;
             usePOSLocalNodeWithTableSize = FALSE;
   
           } else {
   
           // part of fix/workaround to bugzilla 2784: we need to guard against
           // partitioning the sample table in a way that mxcmp is unable to
           // generate a parallel plan for the sideinsert to populate it.
           // For now, choosing numPartitionsNeeded to be even is safe.
           if (((numPartitionsNeeded % 2) != 0) AND (numPartitionsNeeded > 1))
             --numPartitionsNeeded;
           } // end of USTAT_USE_GROUPING_FOR_SAMPLING is OFF
     } // end of non hive tables
                                   /*=========================================*/
                                   /*   FLOAT PRIMARY KEY - NO PARTITIONING   */
                                   /*=========================================*/
    if (unpartitionedSample && objDef->getObjectFormat() == SQLMX)
      {
         LM->Log("Float type primary key - sample table will not have partitions.");
         tableOptions += HS_EXTENT_SIZE_MX;
         tableOptions +=" NO PARTITION ";
      }
                                   /*=========================================*/
                                   /*      DEFAULT PARTITIONING SCHEME        */
                                   /*=========================================*/
    else if (NOT userDefinedScratchVols)
      {
        if (usePOS && objDef->getObjectFormat() == SQLMX)
          {
            if (usePOSMultiNode)
            {
              HSFuncExecQuery("CONTROL QUERY DEFAULT POS 'MULTI_NODE'");
              tableOptions +=" MAX TABLE SIZE ";
              tableOptions.append(LongToNAString(sampleTableSizeInMB));
              LM->Log("Setting POS to MULTI_NODE");
              snprintf(LM->msg, sizeof(LM->msg), "Sample Table Size (in MB): %d (~SampleSet=%d, RecLen=%d)"
                              , sampleTableSizeInMB
                              , (Lng32)sampleRowCount
                              , (Lng32)objDef->getRecordLength());
            }
            else if (usePOSLocalNodeWithTableSize)
            {
              HSFuncExecQuery("CONTROL QUERY DEFAULT POS 'LOCAL_NODE'");
              tableOptions +=" MAX TABLE SIZE ";
              tableOptions.append(LongToNAString(sampleTableSizeInMB));
              LM->Log("Setting POS to LOCAL_NODE");
              snprintf(LM->msg, sizeof(LM->msg), "Sample Table Size (in MB): %d (NumPartitionsCreated=%d, ~SampleSet=%d, RecLen=%d)"
                              , sampleTableSizeInMB
                              , numPartitionsNeeded
                              , (Lng32)sampleRowCount
                              , (Lng32)objDef->getRecordLength());
              LM->Log(LM->msg);
            }
            else
            {
              //Due to performance reasons - we want POS to only choose volumes
              //on the local node, for small sample tables.
              HSFuncExecQuery("CONTROL QUERY DEFAULT POS 'LOCAL_NODE'");
              snprintf(tempStr, sizeof(tempStr), "CONTROL QUERY DEFAULT POS_NUM_OF_PARTNS '%d'", numPartitionsNeeded);
              HSFuncExecQuery(tempStr);
              tableOptions += HS_EXTENT_SIZE_MX;
              snprintf(LM->msg, sizeof(LM->msg), "Partitions Needed: %d (TableType=%s, ~SampleSet=%d, RecLen=%d, threshold=%d)"
                            , numPartitionsNeeded
                            , (objDef->getObjectFormat() == SQLMP) ? "MP" : "MX"
                            , (Lng32)sampleRowCount
                            , (Lng32)objDef->getRecordLength()
                            , scratchVolThreshold);
              LM->Log(LM->msg);
            }
          }
        else {
          LM->Log("Not using POS for sample table.");
          tableOptions += HS_EXTENT_SIZE_MX;
        }
      }
                                   /*=========================================*/
                                   /*  PARTITIONING BASED ON SCRATCH VOLUMES  */
                                   /*=========================================*/
    else
      {
        //Disable Partition Overlay Support (POS) when user specified scratch
        //volumes.
        LM->Log("User has specified scratch volumes for sample.  Turning POS off.");
        HSFuncExecQuery("CONTROL QUERY DEFAULT POS 'OFF'");

        //CASE: 10-041025-8056
        //Make sure the temporary table is not restricted by the primary
        //partitions's extent sizes. The temporary table size should be
        //unrestricted, except for limited disk space on the volume.
        tableOptions += HS_EXTENT_SIZE_MX;

        // Determine a random starting index in the scratchVols array. This
        // will provide better performance, in general, when customers run
        // multiple concurrent update stats jobs.
        volIx = (Lng32)(NA_JulianTimestamp() % (Int64)numScratchVols);
        thisVol = scratchVols[volIx];

        //for SQL/MP tables, user may only specify the location of the
        //temporary sample table. It will always be single partitioned.
        if (objDef->getObjectFormat() == SQLMP)
          {
            tableOptions = thisVol;
            partitionCount++;
          }
        else
          {
            //For every volume specified and needed, generate a syntactically
            //correct HASH PARTITION.
            while ( partitionCount < numPartitionsNeeded &&
                    partitionCount < numScratchVols)
              {
                switch (partitionCount)
                  {
                    case 0:  //first one -- LOCATION CLAUSE
                      tableOptions = " LOCATION ";
                      tableOptions.append(thisVol);
                      partitionCount++;
                      break;

                    case 1:  //initialize hash-partition clause
                      tableOptions.append(" HASH PARTITION ( ADD LOCATION ");
                      tableOptions.append(thisVol);
                      tableOptions.append(HS_EXTENT_SIZE_MX);
                      partitionCount++;
                      break;

                    default: //additional hash partitions
                      tableOptions.append( ", ADD LOCATION ");
                      tableOptions.append(thisVol);
                      tableOptions.append(HS_EXTENT_SIZE_MX);
                      partitionCount++;
                      break;
                  }
                thisVol = scratchVols[(volIx + partitionCount) % numScratchVols];
              } // end of while
          }
        if ( partitionCount > 1 )
          tableOptions.append(")");
      }
    snprintf(LM->msg, sizeof(LM->msg), "returning tableOptions: %s", tableOptions.data());
    LM->Log(LM->msg);

    return tableOptions;
  }

// Print the heading for the display of a query plan to the log.
void printPlanHeader(HSLogMan *LM)
  {
    snprintf(LM->msg, sizeof(LM->msg), "\t%-3s %-3s %-3s %-20s %-8s %-20s %-11s %-11s %-11s"
                     , "LC"
                     , "RC"
                     , "OP"
                     , "OPERATOR"
                     , "OPT"
                     , "DESCRIPTION"
                     , "CARDINALITY"
                     , "OPER_COST"
                     , "TOT_COST"
           );
    LM->Log(LM->msg);

    snprintf(LM->msg, sizeof(LM->msg), "\t%-3s %-3s %-3s %-20s %-8s %-20s %-11s %-11s %-11s"
                     , "---"
                     , "---"
                     , "---"
                     , "--------------------"
                     , "--------"
                     , "--------------------"
                     , "-----------"
                     , "-----------"
                     , "-----------"
           );
    LM->Log(LM->msg);
  }

#ifdef NA_USTAT_USE_STATIC

/***********************************************/
/* METHOD:  printPlan(SQLSTMT_ID*)             */
/* PURPOSE: Write plan information pertaining  */
/*          to query to the log.               */
/* RETCODE:  0 - successful                    */
/*          -1 - failure                       */
/* INPUT:   none                               */
/***********************************************/
Lng32 printPlan(SQLSTMT_ID *stmt)
  {
    Lng32 retcode = 0;
    char  lc[4],                //left child
          rc[4],                //right child
          seqNum[4],            //sequence number
          oper[21],             //operator
          opt[9],               //optimization
          desc[21],             //description
          cardinality[12],      //cardinality
          operCost[12],         //operator cost
          totalCost[12];        //total cost

    HSLogMan *LM = HSLogMan::Instance();

    HSCliStatement prntPlan(HSCliStatement::PRINTPLAN,
                            (char *)stmt->identifier);

    retcode = prntPlan.open();
    HSHandleError(retcode);

    printPlanHeader(LM);

    while (retcode == 0)
      {
        retcode = prntPlan.fetch(9
                                 ,(void *)&lc[0]
                                 ,(void *)&rc[0]
                                 ,(void *)&seqNum[0]
                                 ,(void *)&oper[0]
                                 ,(void *)&opt[0]
                                 ,(void *)&desc[0]
                                 ,(void *)&cardinality[0]
                                 ,(void *)&operCost[0]
                                 ,(void *)&totalCost[0]
                                );
        if (retcode == 0)
          {
            snprintf(LM->msg, sizeof(LM->msg), "\t%-3s %-3s %-3s %-20s %-8s %-20s %-11s %-11s %-11s"
                              , lc
                              , rc
                              , seqNum
                              , oper
                              , opt
                              , desc
                              , cardinality
                              , operCost
                              , totalCost
                   );
            LM->Log(LM->msg);
          }
        else if (retcode != 100)
          HSLogError(retcode);
      }

    retcode = prntPlan.close();
    HSHandleError(retcode);

    return retcode;
  }

#else // NA_USTAT_USE_STATIC not defined

static char ppStmtText[] =
"select"
// LEFT CHILD
"        case"
"          when LEFT_CHILD_SEQ_NUM is null then"
"            '.  '"
"          else"
"            cast(cast(LEFT_CHILD_SEQ_NUM as numeric(3)) as char(3) character set iso88591)"
"        end"
""
// RIGHT CHILD
"      , case"
"          when RIGHT_CHILD_SEQ_NUM is null then"
"            '.  '"
"          else"
"            cast(cast(RIGHT_CHILD_SEQ_NUM as numeric(3)) as char(3) character set iso88591)"
"        end"
""
// SEQUENCE NUMBER
"      , cast(cast(SEQ_NUM as numeric(3)) as char(3) character set iso88591)"
""
// OPERATOR
"      , cast(substring(lower(OPERATOR) from 1 for 20) as char(20))"
""
// OPTIMIZATION
"      , cast (case when position('olt_opt_lean: used' in description) > 0 then 'ol ' else"
"              case when position('olt_optimization: used' in description) > 0 then 'o ' else '' end end"
"              ||"
"              case when position('fast_scan: used' in description) > 0 then 'fs ' else '' end"
"              ||"
"              case when position('fast_replydata_move: used' in description) > 0 then 'fr ' else '' end"
"              as char(8))"
""
// DESCRIPTION
"      , cast(substring("
"            case"
"                when OPERATOR = 'ROOT' then"
"                  case"
"                    when position('upd_action_on_error: return' IN DESCRIPTION) > 0 then 'r'"
"                    when position('upd_action_on_error: xn_rollback' IN DESCRIPTION) > 0 then 'x'"
"                    when position('upd_action_on_error: partial_upd' IN DESCRIPTION) > 0 then 'p'"
"                    when position('upd_action_on_error: savepoint' IN DESCRIPTION) > 0 then 's'"
"                    else ' '"
"                  end"
"                when OPERATOR in ('INDEX_SCAN', 'INDEX_SCAN_UNIQUE') then"
"                    substring("
"                        substring("
"                            substring("
"                                DESCRIPTION from position("
"                                    'index_scan' in DESCRIPTION"
"                                )"
"                            )"
"                            from ("
"                                position("
"                                    '.' in substring("
"                                        DESCRIPTION from position("
"                                            'index_scan' in DESCRIPTION"
"                                        )"
"                                    )"
"                                ) + 1"
"                            )"
"                        )"
"                        from ("
"                            position("
"                                '.' in substring("
"                                    substring("
"                                        DESCRIPTION from position("
"                                            'index_scan' in DESCRIPTION"
"                                        )"
"                                    )"
"                                    from ("
"                                        position("
"                                            '.' in substring("
"                                                DESCRIPTION from position("
"                                                    'index_scan' in DESCRIPTION"
"                                                )"
"                                            )"
"                                        ) + 1"
"                                    )"
"                                )"
"                            ) + 1"
"                        )"
"                        for ("
"                            position("
"                                '(' in substring("
"                                    substring("
"                                        substring("
"                                            DESCRIPTION from position("
"                                                'index_scan' in DESCRIPTION"
"                                            )"
"                                        )"
"                                        from ("
"                                            position("
"                                                '.' in substring("
"                                                    DESCRIPTION from position("
"                                                        'index_scan' in DESCRIPTION"
"                                                    )"
"                                                )"
"                                            ) + 1"
"                                        )"
"                                    )"
"                                    from ("
"                                        position("
"                                            '.' in substring("
"                                                substring("
"                                                    DESCRIPTION from position("
"                                                        'index_scan' in DESCRIPTION"
"                                                    )"
"                                                )"
"                                                from ("
"                                                    position("
"                                                        '.' in substring("
"                                                            DESCRIPTION from position("
"                                                                'index_scan' in DESCRIPTION"
"                                                            )"
"                                                        )"
"                                                    ) + 1"
"                                                )"
"                                            )"
"                                        ) + 1"
"                                    )"
"                                )"
"                            ) - 1"
"                        )"
"                    )"
"                    || ' ('"
"                    || substring(DESCRIPTION"
"                                 from (position('key_type' in DESCRIPTION) + 10)"
"                                 for 1)"
"                    || ')'"
"                when OPERATOR in ('ESP_EXCHANGE', 'SPLIT_TOP') then"
"                    trim(substring(DESCRIPTION"
"                                   from (position('parent_processes' in DESCRIPTION) + 18)"
"                                   for  (position(' ' in substring(DESCRIPTION"
"                                                                   from (position('parent_processes' in DESCRIPTION) + 18)"
"                                                                  )"
"                                                 ) - 1"
"                                        )"
"                                  )"
"                        )"
"                    || case position('top_partitioning_function' in DESCRIPTION)"
"                           when 0 then"
"                               ':'"
"                           else"
"                               '(' ||"
"                               case position('replicate no broadcast' in substring(DESCRIPTION"
"                                                                                   from (position('top_partitioning_function' in DESCRIPTION) + 27)"
"                                                                                   for 22"
"                                                                                  )"
"                                            )"
"                                    when 0 then"
"                                        case position('replicate via broadcast' in substring(DESCRIPTION"
"                                                                                             from (position('top_partitioning_function' in DESCRIPTION) + 27)"
"                                                                                             for 23"
"                                                                                            )"
"                                                     )"
"                                            when 0 then"
"                                                trim(substring(DESCRIPTION"
"                                                               from (position('top_partitioning_function' in DESCRIPTION) + 27)"
"                                                               for 5"
"                                                              )"
"                                                    )"
"                                            else"
"                                                'rep-b'"
"                                        end"
"                                    else"
"                                        'rep-n'"
"                               end ||"
"                               '):'"
"                       end"
"                    || trim(substring(DESCRIPTION"
"                                      from (position('child_processes' in DESCRIPTION) + 17)"
"                                      for  (position(' ' in substring(DESCRIPTION"
"                                                                      from (position('child_processes' in DESCRIPTION) + 17)"
"                                                                     )"
"                                                    ) - 1"
"                                           )"
"                                     )"
"                           )"
"                    || case position('bottom_partitioning_function' in DESCRIPTION)"
"                           when 0 then"
"                               ' '"
"                           else"
"                               '(' ||"
"                               case position('replicate no broadcast' in substring(DESCRIPTION"
"                                                                                   from (position('bottom_partitioning_function' in DESCRIPTION) + 30)"
"                                                                                   for 22"
"                                                                                  )"
"                                            )"
"                                   when 0 then"
"                                       case position('replicate via broadcast' in substring(DESCRIPTION"
"                                                                                            from (position('bottom_partitioning_function' in DESCRIPTION) + 30)"
"                                                                                            for 23"
"                                                                                           )"
"                                                    )"
"                                           when 0 then"
"                                               trim(substring(DESCRIPTION"
"                                                              from (position('bottom_partitioning_function' in DESCRIPTION) + 30)"
"                                                              for 5"
"                                                             )"
"                                                   )"
"                                           else"
"                                               'rep-b'"
"                                       end"
"                                   else"
"                                       'rep-n'"
"                               end ||"
"                               ')'"
"                       end"
"                else"
"                    trim("
"                        trailing ')' from trim("
"                            trailing ' ' from substring("
"                                substring("
"                                    TNAME"
"                                    from ("
"                                        position("
"                                            '.' in TNAME"
"                                        ) + 1"
"                                    )"
"                                )"
"                                from ("
"                                    position("
"                                        '.' in substring("
"                                            TNAME"
"                                            from ("
"                                                position("
"                                                    '.' in TNAME"
"                                                ) + 1"
"                                            )"
"                                        )"
"                                    ) + 1"
"                                )"
"                            )"
"                        )"
"                    )"
"                    || case position('key_type' in DESCRIPTION)"
"                           when 0 then"
"                               ' '"
"                           else"
"                               ' (' || substring(DESCRIPTION"
"                                                 from (position('key_type' in DESCRIPTION) + 10)"
"                                                 for 1"
"                                                )"
"                                    || ')'"
"                       end"
"            end"
"            from 1 for 20) as char(20))"
""
// CARDINALITY
"      , CAST(CARDINALITY AS CHAR(11) character set iso88591)"
""
// OPERATOR COST
"      , CAST(OPERATOR_COST AS CHAR(11) character set iso88591)"
""
// TOTAL COST
"      , CAST(TOTAL_COST AS CHAR(11) character set iso88591)"
""
"        FROM TABLE(EXPLAIN(NULL, '";

// Version of printPlan() that gets compiled if using dynamic queries.
Lng32 printPlan(SQLSTMT_ID *stmt)
  {
    Lng32 retcode = 0;
    struct
      {
        char lc[4];                //left child
        char rc[4];                //right child
        char seqNum[4];            //sequence number
        char oper[21];             //operator
        char opt[9];               //optimization
        char desc[21];             //description
        char cardinality[12];      //cardinality
        char operCost[12];         //operator cost
        char totalCost[12];        //total cost
      } row;

    HSLogMan *LM = HSLogMan::Instance();

    HSFuncExecQuery("CQD DEFAULT_CHARSET 'ISO88591'"); // to avoid buffer overruns in row

    NAString ppStmtStr = ppStmtText;
    ppStmtStr.append((char *)stmt->identifier).append("')) ORDER BY SEQ_NUM DESC;");

    HSCursor ppCursor(STMTHEAP, "HS_CLI_PPSTMT");
    retcode = ppCursor.prepareQuery(ppStmtStr.data(), 0, 9);
    HSHandleError(retcode);
    SQLSTMT_ID* ppStmtId = ppCursor.getStmt();
    SQLDESC_ID* ppOutputDesc = ppCursor.getOutDesc();
    retcode = ppCursor.open();
    HSFilterWarning(retcode);
    HSHandleError(retcode);

    printPlanHeader(LM);

    memset(&row, '\0', sizeof row);
    while (retcode == 0)
      {
        retcode = SQL_EXEC_Fetch(ppStmtId, ppOutputDesc, 9
                                 ,(void*)&row.lc[0], NULL
                                 ,(void*)&row.rc[0], NULL
                                 ,(void*)&row.seqNum[0], NULL
                                 ,(void*)&row.oper[0], NULL
                                 ,(void*)&row.opt[0], NULL
                                 ,(void*)&row.desc[0], NULL
                                 ,(void*)&row.cardinality[0], NULL
                                 ,(void*)&row.operCost[0], NULL
                                 ,(void*)&row.totalCost[0], NULL
                                );
        if (retcode == 0)
          {
            snprintf(LM->msg, sizeof(LM->msg), "\t%-3s %-3s %-3s %-20s %-8s %-20s %-11s %-11s %-11s"
                              , row.lc
                              , row.rc
                              , row.seqNum
                              , row.oper
                              , row.opt
                              , row.desc
                              , row.cardinality
                              , row.operCost
                              , row.totalCost
                   );
            LM->Log(LM->msg);
          }
        else if (retcode != 100)
          {
            HSFuncExecQuery("CQD DEFAULT_CHARSET RESET");
            HSLogError(retcode);
          }
      }

    // ppStmtId will be closed by ~HSCursor if closeStmtNeeded_ is set.

    HSFuncExecQuery("CQD DEFAULT_CHARSET RESET");
    return retcode;
  }

#endif // NA_USTAT_USE_STATIC not defined

// Use a query on the Explain virtual table to see if the plan for stmt uses
// MDAM, and include this information in the ulog. This function is called
// only if logging is turned on.
Lng32 checkMdam(SQLSTMT_ID *stmt)
{
  Lng32 retcode = 0;
  HSLogMan *LM = HSLogMan::Instance();

  NAString stmtStr = "select cast(count(*) as int) from table(explain(null,'";
  stmtStr.append((char*)stmt->identifier)
         .append("')) where description like '%mdam%'");

  HSCursor cursor(STMTHEAP, "HS_CLI_CHECK_MDAM");
  retcode = cursor.prepareQuery(stmtStr.data(), 0, 1);
  HSHandleError(retcode);
  SQLSTMT_ID* stmtId = cursor.getStmt();
  SQLDESC_ID* outputDesc = cursor.getOutDesc();
  retcode = cursor.open();
  HSHandleError(retcode);

  Int32 rowCount = 0;
  retcode = SQL_EXEC_Fetch(stmtId, outputDesc, 1, &rowCount, NULL);
  if (retcode == 0)
    {
      if (rowCount > 0)
        LM->Log("MDAM is used for this query.");
      else
        LM->Log("MDAM is NOT used for this query.");
    }
  else if (retcode == 100)
    LM->Log("MDAM check query returned no rows.");
  else
    {
      LM->Log("MDAM check query failed.");
      HSLogError(retcode);
    }

  return retcode;
}

/**************************************************************************/
/* METHOD:  ucsToDouble()                                                 */
/* PURPOSE: Interpret the Unicode string pointed to by ucs as a numeric   */
/*          type and return it as a double precision value.               */
/* PARAMS:  ucs(in)  -- The Unicode representation of the value.          */
/* RETCODE: Double precision value corresponding to the input string.     */
/**************************************************************************/
double ucsToDouble(Int16 len, const char* val)
{
  // Create arrays for holding 7 byte decoded values.
  static char returnVal[HS_MAX_UCS_BOUNDARY_CHAR]; // make this larger for BigNum
  ComDiagsArea diagsArea;
  ComDiagsArea *diagsAreaPtr = &diagsArea;
  Lng32 dataConversionErrorFlag;

  // Convert the argument to a value from UCS2 format and place in val.
  ex_expr::exp_return_type ok =
    convDoIt((char *)val,
             len,
             REC_NCHAR_F_UNICODE,
             0, // sourcePrecision,
             0, // sourceScale,
             returnVal,
             30,
             REC_IEEE_FLOAT64,
             0, // targetPrecision,
             0, // targetScale,
             NULL, // varCharLen
             0,    // varCharLenSize
             STMTHEAP, // heap,
             &diagsAreaPtr,
             CONV_UNICODE_FLOAT64,
             &dataConversionErrorFlag,
             0);
  return *((double*)returnVal);
}

double ucsToDouble(myVarChar* ucs)
{
  return ucsToDouble(ucs->len, ucs->val);
}

double ucsToDouble(const HSDataBuffer& hsDataBuff)
{
  return ucsToDouble(hsDataBuff.length(), hsDataBuff.data());
}

/****************************************************************************/
/* METHOD:  profileGaps()                                                   */
/* PURPOSE: Go through the distinct values that make up a rowset, and set   */
/*          gap magnitude for each. The running overall gap magnitude       */
/*          average is updated with the information from this rowset.       */
/* PARAMS:  group(in)               -- Group the rowset is for.             */
/*          boundaryRowSet(in)      -- Contains the distinct values and     */
/*                                     their frequencies for this rowset.   */
/*          overallGapAvg(in/out)   -- Average gap size for all values      */
/*                                     seen so far. Updated with            */
/*                                     information from this rowset.        */
/*          overallGapCount(in/out) -- Total number of gaps considered so   */
/*                                     far. Updated with the count from     */
/*                                     this rowset.                         */
/*          firstRowset(in)         -- TRUE if this is the first rowset for */
/*                                     this group. If this is NOT the case, */
/*                                     the static prevValue has the last    */
/*                                     value from the previous rowset.      */
/****************************************************************************/
template <class T>
void profileGaps(HSColGroupStruct *group, boundarySet<T> *boundaryRowSet,
                 double &overallGapAvg, Int64 &overallGapCount,
                 NABoolean firstRowset)
{
  Int32 i;
  double gapAvg = 0;            // avg for this rowset

  // Values to compare to get gap size, which is calculated simply as the
  // difference between two successive distinct values. prevValue is static so
  // it can retain the last value of a rowset for comparison with the first
  // value of the next rowset.
  static double prevValue;
  double currentValue;
  Lng32 dataType = group->colSet[0].datatype;

  // Get the number of non-null values in the rowset. If there is a null, it
  // will be represented by the last row of the rowset, because we do an
  // ascending sort and SQLMX orders null as greater than any non-null value.
  Int32 numNonNullRows = boundaryRowSet->size;
  if (boundaryRowSet->nullInd[boundaryRowSet->size-1] == -1)
    numNonNullRows--;

  // These values are adjusted immediately below for the first rowset.
  Int32 numGaps = numNonNullRows;
  Int32 startIndex = 0;

  if (firstRowset)
    {
      boundaryRowSet->gapMagnitude[0] = 0;  // No gap before 1st overall value
      prevValue = getValueAsDouble(&boundaryRowSet->data[0]);
      numGaps--;       // no value from previous rowset to compare to 1st value
      startIndex = 1;  // for 1st rowset, prevValue is from index 0
    }

  // Save the lowest value used in determining gaps for this rowset (saved over
  // from previous rowset except for the initial rowset), so we can calculate
  // the total of gap sizes in a single step.
  double lowValue = prevValue;

  // Traverse the distinct values of this rowset, calculating the gap between
  // each pair.
  for (i=startIndex; i<numNonNullRows; i++)
    {
      currentValue = getValueAsDouble(&boundaryRowSet->data[i]);
      boundaryRowSet->gapMagnitude[i] = (currentValue - prevValue);
      prevValue = currentValue;
    }

  // prevValue is now the last one of this rowset, lowValue the one we started
  // with; their difference is the sum of gap magnitudes for this rowset.
  gapAvg = (prevValue - lowValue) / numGaps;

  // Combine the average gap for this rowset with the running average so far.
  // Gaps in this rowset will be compared to the current overall average.
  mergeAverage(firstRowset, gapAvg, numGaps, overallGapAvg, overallGapCount);
}

// Explicit instantiation of each type
template
void profileGaps(HSColGroupStruct *, boundarySet<myVarChar>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<Int32>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<UInt32>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<Int8>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<UInt8>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<short>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<unsigned short>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<Int64>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<UInt64>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<ISFixedChar>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<ISVarChar>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<float>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<double>*, double&, Int64&,
                 NABoolean);
template
void profileGaps(HSColGroupStruct *, boundarySet<MCWrapper>*, double&, Int64&,
                 NABoolean);

//=========================================================
//@ZXnew
Lng32 HSCursor::prepareQuery(const char *cliStr, Lng32 numParams, Lng32 numResults)
{
    HSLogMan *LM = HSLogMan::Instance();
    LM->Log("In HSCursor::prepareQuery: Query Text ");
    LM->Log(cliStr);
    LM->StartTimer("HSCursor::prepareQuery()");

    retcode_ = SQL_EXEC_ClearDiagnostics(stmt_);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_AllocStmt(stmt_, 0);
    HSHandleError(retcode_);
    stmtAllocated_ = TRUE;

    retcode_ = SQL_EXEC_AllocDesc(srcDesc_, 1);
    HSHandleError(retcode_);
    srcDescAllocated_ = TRUE;

    retcode_ = SQL_EXEC_AllocDesc(outputDesc_, numResults);
    HSHandleError(retcode_);
    outputDescAllocated_ = TRUE;

    retcode_ = SQL_EXEC_AllocDesc(inputDesc_, numParams);
    HSHandleError(retcode_);
    inputDescAllocated_ = TRUE;

    retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_TYPE_FS,
                                    REC_BYTE_V_ANSI, 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_VAR_PTR,
                                    (Long)cliStr, 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_LENGTH,
                                    strlen(cliStr) + 1, 0);
    HSHandleError(retcode_);
    // SQLDESC_CHAR_SET must be the last descriptor item set, otherwise
    // it may get reset by other calls to SQL_EXEC_SetDescItem().
    NAString charSet = ActiveSchemaDB()->getDefaults().getValue(ISO_MAPPING);
    NAString defCS   = ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc_, 1, SQLDESC_CHAR_SET,
                                    SQLCHARSETCODE_UTF8
                                    , 0);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_SetStmtAttr(stmt_, SQL_ATTR_PARENT_QID, 0,
            (char *)CmpCommon::context()->sqlSession()->getParentQid());
    HSHandleError(retcode_);

    SQL_QUERY_COST_INFO query_cost_info;
    SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;


    LM->StartTimer("SQL_EXEC_Prepare2()");
    // SQL_EXEC_Prepare2 is executed in an embedded compiler which might change the value
    // of HSGlobalsClass::schemaVersion so we need to save it here and then restore it after
    // the execution of SQL_EXEC_Prepare2
    COM_VERSION    schemaVersion_sav = HSGlobalsClass::schemaVersion;
    retcode_ = SQL_EXEC_Prepare2(stmt_, srcDesc_,NULL,0,NULL,&query_cost_info, &comp_stats_info,NULL,0,0);
    HSGlobalsClass::schemaVersion = schemaVersion_sav;
    LM->StopTimer();
    if (retcode_ < 0)
      {
        if (LM->LogNeeded())
          {
            LM->Log("Failure in preparing the following query:");
            LM->Log(cliStr);
          }
        // @ZX: CLI error not visible to user without following step; investigate
        //      possibility that this is the case for other CLI invocations.
        HSFuncMergeDiags(- UERR_INTERNAL_ERROR, NULL, NULL, TRUE);
      }
    HSHandleError(retcode_);
    //if (LM->LogNeeded())
    //  {
    //    printPlan(stmt_);
    //    SQL_EXEC_ClearDiagnostics(stmt_);
    //  }

    retcode_ = SQL_EXEC_DescribeStmt(stmt_, inputDesc_, outputDesc_);
    HSHandleError(retcode_);

    LM->StopTimer();  // HSCursor::prepareQuery()
    return retcode_;
}

//@ZXnew
Lng32 HSCursor::open(Lng32 numParams, void * in01)
{
  Lng32 retcode = SQL_EXEC_ClearDiagnostics(stmt_);
  HSHandleError(retcode);
  retcode = SQL_EXEC_Exec(stmt_, inputDesc_, numParams, in01, NULL);
  HSHandleError(retcode);
  closeStmtNeeded_ = TRUE;
  lastFetchReturned100_ = FALSE;
  return retcode;
}

//@ZXnew
Lng32 HSCursor::fetch(Lng32 numResults,
                      void *  out01, void *  out02, void *  out03,
                      void *  out04, void *  out05, void *  out06,
                      void *  out07, void *  out08, void *  out09,
                      void *  out10, void *  out11, void *  out12,
                      void *  out13, void *  out14, void *  out15,
                      void *  out16, void *  out17, void *  out18,
                      void *  out19, void *  out20, void *  out21,
                      void *  out22, void *  out23, void *  out24,
                      void *  out25)
{
  HS_ASSERT((numResults == 0) || (numResults > 0 && out01 != NULL));
  Lng32 retcode = SQL_EXEC_Fetch(stmt_, outputDesc_, numResults,
                                 out01, NULL, out02, NULL, out03, NULL,
                                 out04, NULL, out05, NULL, out06, NULL,
                                 out07, NULL, out08, NULL, out09, NULL,
                                 out10, NULL, out11, NULL, out12, NULL,
                                 out13, NULL, out14, NULL, out15, NULL,
                                 out16, NULL, out17, NULL, out18, NULL,
                                 out19, NULL, out20, NULL, out21, NULL,
                                 out22, NULL, out23, NULL, out24, NULL,
                                 out25, NULL);
  lastFetchReturned100_ = (retcode == 100);
  return retcode;
}

Lng32 HSCursor::close()
{
  // This is done by the dtor ordinarily. However, if a cursor needs to
  // be reused (e.g., different input parameters), we need to be able
  // to close without destroying the object.

  Lng32 retcode = 0;
  if (closeStmtNeeded_)
    {
      if (lastFetchReturned100_)
        {
          // if the last fetch simply ran to the end of the cursor, reset
          // the diags area so we don't return the 100 again on the close
          SQL_EXEC_ClearDiagnostics(stmt_);
        }
      retcode = SQL_EXEC_CloseStmt(stmt_);
      closeStmtNeeded_ = FALSE;  // always set to FALSE so we don't try again
    }
  lastFetchReturned100_ = FALSE;
  return retcode;
}

// Set the cursor name used for a dynamic statement.
Lng32 HSCursor::setCursorName(const char* name)
{
  if (cursorName_)
    delete cursorName_;
  cursorName_ = new SQLCLI_OBJ_ID;
  init_SQLCLI_OBJ_ID(cursorName_, SQLCLI_CURRENT_VERSION,
                     stmt_name, stmt_->module, name, 0,
                     SQLCHARSETSTRING_ISO88591, (Lng32)strlen(name), 0);
  Lng32 retcode = SQL_EXEC_SetCursorName(stmt_, cursorName_);
  return retcode;
}
