/**********************************************************************
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
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_la.C
 * Description:  Function to retrieve lable info.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#define HS_FILE "hs_la"

#include "hs_la.h"
#define   SQLPARSERGLOBALS_FLAGS
#include "SqlParserGlobalsCmn.h"
#include "CmpCommon.h" //heap usage
#include "CompException.h"
#include "hs_log.h"
#include "sql_id.h"
#include "ComMPLoc.h"
#include "EHException.h"
#include "hs_cli.h"
#include "SchemaDB.h"
#include "hs_globals.h"
#include "hs_util.h"

#include "OptimizerSimulator.h"
#include "ComSizeDefs.h"

#include "hiveHook.h"
#include "ComSmallDefs.h"
#include "BindWA.h"
#include "RelScan.h"
#include "NATable.h"


HSTableDef* HSTableDef::create(CollHeap* heap,
                               const ComObjectName &tableName,
                               const hs_table_type tableType,
                               const ComAnsiNameSpace nameSpace)
{
  if (HSGlobalsClass::isHbaseCat(tableName.getCatalogNamePart().getInternalName()))
    return new(heap) HSHbaseTableDef(tableName, tableType, nameSpace);
  else if (HSGlobalsClass::isHiveCat(tableName.getCatalogNamePart().getInternalName()))
    return new(heap) HSHiveTableDef(tableName, tableType, nameSpace);
  else
    return new(heap) HSSqTableDef(tableName, tableType, nameSpace);
}

HSTableDef::HSTableDef(const ComObjectName &tableName, const hs_table_type tableType, const ComAnsiNameSpace nameSpace)
       :tableName_(new(STMTHEAP) NAString(STMTHEAP)),
        ansiName_(new(STMTHEAP) NAString(STMTHEAP)),
        naTbl_(NULL),
        catalog_(new(STMTHEAP) NAString(tableName.getCatalogNamePart().getInternalName(), STMTHEAP)),
        schema_(new(STMTHEAP) NAString(tableName.getSchemaNamePart().getInternalName(), STMTHEAP)),
        object_(new(STMTHEAP) NAString(tableName.getObjectNamePart().getInternalName(), STMTHEAP)),
        objectUID_(0),
        objActualFormat_(UNKNOWN),
        objFormat_(tableType),
        numCols_(0),
        colInfo_(NULL),
        retcode_(0),
        isVolatile_(FALSE),
        guardianName_(new(STMTHEAP) NAString(STMTHEAP)),
        nameSpace_(nameSpace),
        labelAccessed_(FALSE),
        objectType_(COM_UNKNOWN_OBJECT),
        isMetadataObject_(FALSE),
        hasSyskey_(FALSE)
{
  // Set volatile schema flag based on schema name.
  if (!strncmp(schema_->data(),
               COM_VOLATILE_SCHEMA_PREFIX,
               strlen(COM_VOLATILE_SCHEMA_PREFIX)))
    isVolatile_ = TRUE;
}



HSTableDef::~HSTableDef()
  {
    if (colInfo_ != NULL)
      {
        delete [] colInfo_;
        colInfo_ = NULL;
      }
  }

NABoolean HSSqTableDef::objExists(NABoolean createExternalTable)
  {
    setNATable();
    if (!naTbl_)
      return FALSE;
    return TRUE;
  }

void HSTableDef::setNATable()
  {
    BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE, 
                  HSGlobalsClass::isNativeCat(*catalog_));

    CorrName corrName(*object_, STMTHEAP, *schema_, *catalog_);
    if (isVolatile())
      corrName.setIsVolatile(TRUE);
    Scan scan(corrName, NULL, REL_SCAN, STMTHEAP);
    ULng32 flagToSet = 0;  // don't turn on flag unless next 'if' is true
    if (CmpCommon::context()->sqlSession()->volatileSchemaInUse())
      flagToSet = ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME;
    
    flagToSet |= ALLOW_SPECIALTABLETYPE;

    // set ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME bit in Sql_ParserFlags
    // if needed, and return it to its entry value on exit
    PushAndSetSqlParserFlags savedParserFlags(flagToSet);

    scan.bindNode(&bindWA);
    if (!bindWA.errStatus())
      {
        naTbl_ = bindWA.getNATable(corrName);
        HS_ASSERT(naTbl_);
        objectType_ = naTbl_->getObjectType();
      }
  }

void HSSqTableDef::GetLabelInfo(labelDetail detail)
  {
    //Don't read label information for objects created by OSIM
    if((CmpCommon::getDefault(CREATE_OBJECTS_IN_METADATA_ONLY) == DF_ON) ||
       (OSIM_runningSimulation() && !OSIM_ustatIsDisabled())  ||
       isInMemoryObjectDefn())
    {
      labelAccessed_ = TRUE;
      return;
    }
  }

Lng32 HSSqTableDef::getColumnNames()
  {
    HSErrorCatcher errorCatcher(retcode_, - UERR_UNABLE_TO_DESCRIBE_COLUMN_NAMES, "HSSqTableDef::DescribeColumnNames", TRUE);
    HSFuncExecQuery("CONTROL QUERY DEFAULT DISPLAY_DIVISION_BY_COLUMNS 'ON'"); 

    Lng32 retcode = DescribeColumnNames();

    HSFuncExecQuery("CONTROL QUERY DEFAULT DISPLAY_DIVISION_BY_COLUMNS RESET");

    return retcode;
  }

Lng32 HSHiveTableDef::getColumnNames()
  {
    HSErrorCatcher errorCatcher(retcode_, - UERR_UNABLE_TO_DESCRIBE_COLUMN_NAMES, "HSHiveTableDef::DescribeColumnNames", TRUE);

    Lng32 retcode = DescribeColumnNames();

    return retcode;
  }

Lng32 HSHbaseTableDef::getColumnNames()
  {
    HSErrorCatcher errorCatcher(retcode_, - UERR_UNABLE_TO_DESCRIBE_COLUMN_NAMES, "HSHbaseTableDef::DescribeColumnNames", TRUE);

    Lng32 retcode = DescribeColumnNames();

    return retcode;
  }

// local function used by the X::DescribeColumnNames functions
NAString getAnsiName(NAString & columnName)
  {
    NAString ansiName = ToAnsiIdentifier(columnName);
    NAString dblQuote="\"";
    NAString result;

    // Surround ANSI name with double quotes, if not already delimited.
    if (ansiName.data()[0] == '"')
      result = ansiName;
    else
      result = dblQuote+ansiName+dblQuote;

    return result;
  }


Lng32 HSSqTableDef::DescribeColumnNames()
  {
    Lng32 entry, len;
    NAString query;
    char colName[ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES + 2];
    HSLogMan *LM = HSLogMan::Instance();

    SQLMODULE_ID module;
    init_SQLMODULE_ID(&module);

    SQLSTMT_ID *stmt = new(STMTHEAP) SQLSTMT_ID;
    init_SQLCLI_OBJ_ID(stmt);
    stmt->module = &module;
    stmt->name_mode = stmt_handle;

    SQLDESC_ID *srcDesc = new(STMTHEAP) SQLDESC_ID;
    init_SQLCLI_OBJ_ID(srcDesc);
    srcDesc->module = &module;
    srcDesc->name_mode = desc_handle;

    SQLDESC_ID *outputDesc = new(STMTHEAP) SQLDESC_ID;
    init_SQLCLI_OBJ_ID(outputDesc);
    outputDesc->module = &module;
    outputDesc->name_mode = desc_handle;

    // Use the header information from a 'select *' to get the column names. 
    // Note that this works for SJIS and UTF8 since the names returned through
    // CLI are encoded correctly.
    retcode_ = setHasSyskeyFlag();
    HSHandleError(retcode_);
    if (hasSyskey_)
      query  = "SELECT SYSKEY, * FROM ";
    else
      query  = "SELECT * FROM ";
    if(objActualFormat_ == SQLMP)
      query += getTableName(tableName_->data(), nameSpace_);
    else
      query += getTableName(ansiName_->data(), nameSpace_);

    retcode_ = SQL_EXEC_ClearDiagnostics(stmt);
    // to prevent false alarms for statement heap memory allocation "smt"
    // coverity[leaked_storage]
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_AllocStmt(stmt, 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_AllocDesc(srcDesc, 1);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_AllocDesc(outputDesc, 4096);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_SetDescItem(srcDesc, 1, SQLDESC_TYPE_FS,
                                    REC_BYTE_V_ANSI, 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc, 1, SQLDESC_VAR_PTR,
                                    (Long)query.data(), 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc, 1, SQLDESC_LENGTH,
                                    query.length() + 1, 0);
    HSHandleError(retcode_);
    // SQLDESC_CHAR_SET must be the last descriptor item set, otherwise
    // it may get reset by other calls to SQL_EXEC_SetDescItem().
    NAString charSet = ActiveSchemaDB()->getDefaults().getValue(ISO_MAPPING);
    NAString defCS   = ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc, 1, SQLDESC_CHAR_SET,
                                    SQLCHARSETCODE_UTF8
                                    , 0);
    HSHandleError(retcode_);

    // ---------------------------------------------------------------------
    // Prepare the statement
    // ---------------------------------------------------------------------
    SQL_QUERY_COST_INFO query_cost_info;
    SQL_QUERY_COMPILER_STATS_INFO comp_stats_info;
   
    retcode_ = SQL_EXEC_Prepare2(stmt, srcDesc,NULL,0,NULL,&query_cost_info, &comp_stats_info,NULL,0,0);
    HSHandleError( retcode_);

    // ---------------------------------------------------------------------
    // describe the column information into the output descriptor
    // ---------------------------------------------------------------------
    retcode_ = SQL_EXEC_DescribeStmt(stmt, 0, outputDesc);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_GetDescEntryCount(outputDesc, &numCols_);
    HSHandleError(retcode_);

    colInfo_ = new(STMTHEAP) HSColumnStruct[numCols_];
    for (Int32 i = 0; i < numCols_; i++)
      {
                                                  /*==   GET COLUMN NAME   ==*/
        entry = i + 1;
        retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,
                                        SQLDESC_NAME,
                                        0, colName, sizeof(colName), &len, 0);
        if ((retcode_ == 0) &&
            (len >= sizeof(colName) ))
          retcode_ = -1;
        HSHandleError(retcode_);
        colName[len] = '\0';
        *colInfo_[i].colname = &*colName;
        *colInfo_[i].externalColumnName = getAnsiName(*colInfo_[i].colname);
                                                  /*== GET COLUMN DATATYPE ==*/
        retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,
                                        SQLDESC_TYPE_FS,
                                        &colInfo_[i].datatype,
                                        0, 0, 0, 0);
        HSHandleError(retcode_);

        retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,
                                        SQLDESC_NULLABLE,
                                        &colInfo_[i].nullflag,
                                        0, 0, 0, 0);
        HSHandleError(retcode_);
                                                  /*==  GET COLUMN LENGTH  ==*/
        retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,
                                        SQLDESC_OCTET_LENGTH,
                                        &colInfo_[i].length,
                                        0, 0, 0, 0);
        HSHandleError(retcode_);

        // If applicable, get the character set, precision and scale
        if (DFS2REC::isAnyCharacter(colInfo_[i].datatype))
          {
            retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,
                                            SQLDESC_CHAR_SET,
                                            (Lng32*)&colInfo_[i].charset, 0, 0, 0, 0);
            HSHandleError(retcode_);
            // UCS2 cols not supported in MODE_SPECIAL_1 or 2 and do not support case insensitivity.
            retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,
                                            SQLDESC_CASEINSENSITIVE,
                                            (Lng32*)&colInfo_[i].caseInsensitive, 0, 0, 0, 0);
            HSHandleError(retcode_);
            retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,
                                            SQLDESC_COLLATION,
                                            (Lng32*)&colInfo_[i].colCollation, 0, 0, 0, 0);
            HSHandleError(retcode_);
          }
        else if ((colInfo_[i].datatype >= REC_MIN_BINARY_NUMERIC &&  // May be type NUMERIC
                  colInfo_[i].datatype <= REC_MAX_BINARY_NUMERIC)    //    instead of INT
                     ||
                 (colInfo_[i].datatype >= REC_MIN_DECIMAL &&
                 colInfo_[i].datatype <= REC_MAX_DECIMAL))
          {
            retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,                
                                           SQLDESC_PRECISION,                  
                                           &colInfo_[i].precision, 0, 0, 0, 0);
            HSHandleError(retcode_);
            retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,                
                                           SQLDESC_SCALE,                  
                                           &colInfo_[i].scale, 0, 0, 0, 0);
            HSHandleError(retcode_);
          }
        else if (DFS2REC::isDateTime(colInfo_[i].datatype))
          {
            retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,
                                           SQLDESC_DATETIME_CODE,
                                           &colInfo_[i].precision, 0, 0, 0, 0);
            HSHandleError(retcode_);
            retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,
                                           SQLDESC_PRECISION,
                                           &colInfo_[i].scale, 0, 0, 0, 0);
            HSHandleError(retcode_);
          }
        else if (DFS2REC::isInterval(colInfo_[i].datatype))
          {
            retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,                
                                            SQLDESC_INT_LEAD_PREC,              
                                            &colInfo_[i].precision, 0, 0, 0, 0);
            HSHandleError(retcode_);                                           
            retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,                
                                            SQLDESC_PRECISION,                  
                                            &colInfo_[i].scale, 0, 0, 0, 0);
            HSHandleError(retcode_);
          }
        else
          {
            /* No additional information about column attributes needed */
          }

        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "COLUMN [%s]: (%d, %d, %d, %d, %d, %d)"
                                , colInfo_[i].colname->data()
                                , colInfo_[i].datatype
                                , colInfo_[i].nullflag
                                , colInfo_[i].charset
                                , colInfo_[i].length
                                , colInfo_[i].precision
                                , colInfo_[i].scale
                   );
            LM->Log(LM->msg);
          }
      }
    retcode_ = SQL_EXEC_DeallocStmt(stmt);
    HSHandleError(retcode_);

    return 0;
  }

// go over all the table partitions and return the most
// recent lastModTime among them
Int64 HSSqTableDef::getModTime() const
{
   return 0;
}

// 3/29/2013
// NOTE: We probably don't need the following function anymore.
//       It largely duplicates getRowCounts(), and is only called
//       to get the number of IUD rows if getRowCounts() fails.
//       However, getRowCounts() no longer gives up and does an
//       estimate if any partition fails to deliver a valid row
//       count, so we will have an accurate IUD row count except
//       for any partitions for which -1 was returned for the current
//       row count (and only if that fact means that the IUD row counts
//       returned for that partition are not valid).
/****************************************************************/
/* METHOD:  getRowChangeCounts()                                */
/* PURPOSE: Get the number of inserts, deletes, and updates     */
/*          since the last time statistics were updated for the */
/*          table. This is called for processing of the         */
/*          NECESSARY clause only if the earlier call to        */
/*          getRowCounts() failed to yield a row count for all  */
/*          partitions of the table.                            */
/*          Otherwise, the change counts are retrieved at the   */
/*          same time and calling this function isn't necessary.*/
/* PARAMS:  inserts(out) -- Number of inserts since last time   */
/*                          stats were updated for the table.   */
/*          deletes(out) -- Number of deletes since last time   */
/*                          stats were updated for the table.   */
/*          updates(out) -- Number of updates since last time   */ 
/*                          stats were updated for the table.   */
/****************************************************************/
void HSSqTableDef::getRowChangeCounts(Int64 &inserts,
                                      Int64 &deletes,
                                      Int64 &updates)
  {
    inserts = 0;
    deletes = 0;
    updates = 0;
  }

/****************************************************************/
/* METHOD:  resetRowCounts()                                    */
/* PURPOSE: Reset the counts of inserts, deletes, and updates   */
/*          stored in the file label in each partition for this */
/*          table.                                              */
/****************************************************************/

void HSSqTableDef::resetRowCounts()
#ifdef GETROWCOUNTS_UNSUPPORTED
  {}  // empty function if platform doesn't support getRowCounts()
#else
  {
  }
#endif

Int64 HSSqTableDef::getRowCount(NABoolean &isEstimate,
                                Int32 &errorCode,
                                Int32 &breadCrumb,
                                NABoolean estimateIfNecessary)
  {
    Int64 bogus;
    return getRowCount(isEstimate, bogus, bogus, bogus, bogus, bogus, 
                       errorCode, breadCrumb, estimateIfNecessary);
  }

/***************************************************************************/
/* METHOD:  getRowCount()                                                  */
/* PURPOSE: Attempt to get the row count for the table from the partition  */ 
/*          file labels.  If unable, then instead get partition size and   */
/*          estimate rows. While accessing the file labels, we also get    */
/*          the counters for inserts, updates, and deletes since the last  */
/*          Update Stats using NECESSARY. However, these will be discarded */
/*          if all the partitions are not accessed because at least one    */
/*          does not have accurate row count.  These counts will be        */
/*          acquired later only if NECESSARY is used.                      */
/* ARGUMENTS: isEstimate: an output value, set to TRUE if the returned row */
/*              count is an estimate, FALSE otherwise.                     */
/*            numInserts: an output value, set to the number of inserts on */
/*              the table since the last update stats using NECESSARY.     */
/*            numDeletes: an output value, set to the number of deletes on */
/*              the table since the last update stats using NECESSARY.     */
/*            numUpdates: an output value, set to the number of updates on */
/*              the table since the last update stats using NECESSARY.     */
/*            estimateIfNecessary: not used in this redefinition.          */
/* RETURN VALUE: The number of rows in the table, -1 if there is an error  */
/*               reading a partition.                                      */
/***************************************************************************/
Int64 HSSqTableDef::getRowCount(NABoolean &isEstimate,
                              Int64 &numInserts,
                              Int64 &numDeletes,
                              Int64 &numUpdates,
                              Int64 &numPartitions,
                              Int64 &minRowCtPerPartition,
                              Int32 &errorCode,
                              Int32 &breadCrumb,
                              NABoolean estimateIfNecessary)
  {
    errorCode = 0;
    breadCrumb = -5;
    isEstimate = TRUE;
    numInserts =
      numDeletes =
      numUpdates = 0;
    numPartitions = 1;
    minRowCtPerPartition = 1000000;
    return 1000000;
  }

/***************************************************************************/
/* METHOD:  getRowCountUsingSelect()                                       */
/* PURPOSE: Get row count of a table using 'select count(*)'.              */ 
/* RETURN VALUE: The number of rows in the table, -1 if there is an error. */
/***************************************************************************/
Int64 HSTableDef::getRowCountUsingSelect()
{
  // disable MVQR prior to executing the FetchCount query
  HSFuncExecQuery("CONTROL QUERY DEFAULT MVQR_REWRITE_LEVEL '0'");

  Int64 rows = -1;
  HSCursor cursor;

  NAString query  = "SELECT COUNT(*) FROM ";
  query += getTableName(getObjectFullName(), getNameSpace());
  query += " FOR SKIP CONFLICT ACCESS;";

  if (cursor.fetchNumColumn(query, NULL, &rows) < 0) 
     rows = -1; // Error

  // reset the MVQR MVQR_REWRITE_LEVEL setting
  HSFuncExecQuery("CONTROL QUERY DEFAULT MVQR_REWRITE_LEVEL RESET");

  return rows; 
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 10-060424-6040
//SECURITY ISSUE WORKAROUND (CATMAN):
//To facilitate all users to be able create a temporary sample table,
//Catman has allowed the creation of a PUBLIC schema, which will allow
//all users to issue DDL statements. In release 2, Catman plans to
//provide a complete solution to this problem. When the solution is
//implemented, this workaround will need to be removed.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
NABoolean HSSqTableDef::publicSchemaExists()
  {
    return FALSE;
  }

// Check to see if a system defined SYSKEY exists and set hasSyskey_ flag.
Lng32 HSSqTableDef::setHasSyskeyFlag()
  {
    HSErrorCatcher errorCatcher(retcode_, -UERR_INTERNAL_ERROR, "HSTableDef::setHasSyskeyFlag()", TRUE);
    Lng32 entry, len;
    NAString query;
    char colName[ComMAX_1_PART_INTERNAL_UTF8_NAME_LEN_IN_BYTES + 2];
    HSLogMan *LM = HSLogMan::Instance();

    SQLMODULE_ID module;
    init_SQLMODULE_ID(&module);

    SQLSTMT_ID *stmt = new(STMTHEAP) SQLSTMT_ID;
    init_SQLCLI_OBJ_ID(stmt);
    stmt->module = &module;
    stmt->name_mode = stmt_handle;

    SQLDESC_ID *srcDesc = new(STMTHEAP) SQLDESC_ID;
    init_SQLCLI_OBJ_ID(srcDesc);
    srcDesc->module = &module;
    srcDesc->name_mode = desc_handle;

    SQLDESC_ID *outputDesc = new(STMTHEAP) SQLDESC_ID;
    init_SQLCLI_OBJ_ID(outputDesc);
    outputDesc->module = &module;
    outputDesc->name_mode = desc_handle;

    query  = "SELECT SYSKEY, * FROM ";
    if(objActualFormat_ == SQLMP)
      query += getTableName(tableName_->data(), nameSpace_);
    else
      query += getTableName(ansiName_->data(), nameSpace_);

    retcode_ = SQL_EXEC_ClearDiagnostics(stmt);
    // to prevent false alarms for statement heap memory allocation "smt"
    // coverity[leaked_storage]
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_AllocStmt(stmt, 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_AllocDesc(srcDesc, 1);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_AllocDesc(outputDesc, 4096);
    HSHandleError(retcode_);

    retcode_ = SQL_EXEC_SetDescItem(srcDesc, 1, SQLDESC_TYPE_FS,
                                    REC_BYTE_V_ANSI, 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc, 1, SQLDESC_VAR_PTR,
                                    (Long)query.data(), 0);
    HSHandleError(retcode_);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc, 1, SQLDESC_LENGTH,
                                    query.length() + 1, 0);
    HSHandleError(retcode_);
    // SQLDESC_CHAR_SET must be the last descriptor item set, otherwise
    // it may get reset by other calls to SQL_EXEC_SetDescItem().
    NAString charSet = ActiveSchemaDB()->getDefaults().getValue(ISO_MAPPING);
    NAString defCS   = ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET);
    retcode_ = SQL_EXEC_SetDescItem(srcDesc, 1, SQLDESC_CHAR_SET,
                                    SQLCHARSETCODE_UTF8
                                    , 0);
    HSHandleError(retcode_);

    // ---------------------------------------------------------------------
    // Prepare the statement
    // If there is an error, there are no SYSKEYs for the table.
    // (Most common case).
    // ---------------------------------------------------------------------
    retcode_ = SQL_EXEC_Prepare(stmt, srcDesc);
    if (retcode_ == -4001) retcode_ = 0; // -4001 is 'Column not found'.
    else if (retcode_ != 0) { HSHandleError(retcode_); }
    else // retcode is 0 - SYSKEY column found.
    {
      // There were columns named SYSKEY.  If there is only 1 column
      // named SYSKEY, then it is a system defined SYSKEY.  If there
      // are two columns named SYSKEY, then both are actually the
      // same column and are a user defined column.
      retcode_ = SQL_EXEC_DescribeStmt(stmt, 0, outputDesc);
      HSHandleError(retcode_);

      retcode_ = SQL_EXEC_GetDescEntryCount(outputDesc, &numCols_);
      HSHandleError(retcode_);

      Int32 syskeyCnt=0;
      for (Int32 i = 0; i < numCols_; i++)
        {
          // Check to see if column name is SYSKEY.  If there are 
          // two present, then a system defined SYSKEY does not exist.
          entry = i + 1;
          retcode_ = SQL_EXEC_GetDescItem(outputDesc, entry,
                                          SQLDESC_NAME,
                                          0, colName, sizeof(colName), &len, 0);
          if (retcode_ == 0 && len >= sizeof(colName) ) retcode_ = -1;
          HSHandleError(retcode_);
          colName[len] = '\0';
          if (!strcmp(colName, "SYSKEY")) syskeyCnt++;
          if (syskeyCnt > 1) break;
        }
      if (syskeyCnt == 1) hasSyskey_ = TRUE;
    }
    retcode_ = SQL_EXEC_DeallocStmt(stmt);
    HSHandleError(retcode_);

    // to prevent false alarms for statement heap memory allocation
    // for "stmt", "srcDesc", "outoutDesc"
    // coverity[leaked_storage]
    return retcode_;
  }

Lng32 HSTableDef::getColNum(const char *colName, NABoolean errIfNotFound) const
  {
    Lng32 retcode = -1;
    NAString ansiForm(colName);
    HSLogMan *LM = HSLogMan::Instance();

    for (Int32 i = 0; i < numCols_; i++)
      {
        if ((&colInfo_[i] != NULL) &&
            (ansiForm == *colInfo_[i].colname))
          {
            // raise an error when a LOB column is explicitly specified
            if (DFS2REC::isLOB(colInfo_[i].datatype))
              {
                if (LM->LogNeeded())
                  {
                    sprintf(LM->msg, "***[ERROR]:  Column (%s) is a LOB column.", colName);
                    LM->Log(LM->msg);
                  }
                HSFuncMergeDiags(-UERR_LOB_STATS_NOT_SUPPORTED, colName);
                HSHandleError(retcode);
                return retcode;               
              }

            return i;
          }
      }

    if (!errIfNotFound)
      return -1;

    if (LM->LogNeeded())
      {
        sprintf(LM->msg, "***[ERROR]:  Column (%s) does not exist.", colName);
        LM->Log(LM->msg);
      }

    HSFuncMergeDiags( - UERR_COLUMN_NAME_DOES_NOT_EXIST
                    , colName
                    , getObjectFullName().data()
                    );
    HSHandleError(retcode);
    return retcode;
  }

char* HSTableDef::getColName(Lng32 colNum) const
  {
    HS_ASSERT((colNum >= 0) &&
       (colNum < numCols_));

    return (char*)colInfo_[colNum].colname->data();
  }

HSColumnStruct& HSTableDef::getColInfo(Lng32 colNum) const
  {
    HS_ASSERT((colNum >= 0) &&
       (colNum < numCols_));

    return colInfo_[colNum];
  }

NAString HSSqTableDef::getNodeName() const
  {
    NAString node;
    return node;
  }

NAString HSTableDef::getObjectFullName() const
  {
    if(objActualFormat_ == SQLMP)
      return tableName_->data();
    else
      return ansiName_->data();
  }


NAString HSSqTableDef::getCatalogLoc(formatType format) const
  {
    NAString catalogName;
    return catalogName;
  }


NAString HSTableDef::getPrimaryLoc(formatType format) const
  {
    NAString loc;

    if (format == INTERNAL_FORMAT)
      {
        loc.append(catalog_->data());
        loc.append(".");
        loc.append(schema_->data());
      }
    else
      {
        if (objActualFormat_ == SQLMP)
          {
            loc.append(catalog_->data());
            loc.append(".");
            loc.append(ToAnsiIdentifier(schema_->data()));
          }
        else
          {
            loc.append(ToAnsiIdentifier(catalog_->data()));
            loc.append(".");
            loc.append(ToAnsiIdentifier(schema_->data()));
          }
      }

    return loc;
  }

NAString HSTableDef::getCatName(formatType format) const
  {
    NAString cat;

    if (format == INTERNAL_FORMAT ||
        objActualFormat_ == SQLMP)
      {
        cat.append(catalog_->data());
      }
    else
      {
        cat.append(ToAnsiIdentifier(catalog_->data()));
      }

    return cat;
  }


NAString HSTableDef::getSchemaName(formatType format) const
  {
    NAString sch;

    if (format == INTERNAL_FORMAT)
      {
        sch.append(schema_->data());
      }
    else
      {
        sch.append(ToAnsiIdentifier(schema_->data()));
      }

    return sch;
  }


NAString HSTableDef::getObjectName(formatType format) const
  {
    NAString obj;

    if (format == INTERNAL_FORMAT)
      {
        obj.append(object_->data());
      }
    else
      {
        obj.append(ToAnsiIdentifier(object_->data()));
      }

    return obj;
  }

Lng32 HSSqTableDef::getTotalVarcharLength() const
  {

    Lng32 length = 0;
    for (Int32 i = 0; i < numCols_; i++)
      {
        if (DFS2REC::isAnyVarChar(colInfo_[i].datatype))
          {
            length += colInfo_[i].length ;
          }
      }
    return length;
  }

/***********************************************/
/* METHOD:  collectFileStatistics()            */
/* PURPOSE: Collect File-Level Statistics      */
/* RETCODE:       0 - successful               */
/*               -1 - internal error, buffer   */
/*                    length too small.        */
/*          SQLCODE - <0 failure               */
/*                    >=0 successful           */
/* ASSUMPTIONS: A transaction has already been */
/*              started.                       */
/***********************************************/
Lng32 HSSqTableDef::collectFileStatistics() const
  {
    Lng32 retcode = 0;
    char buf[2000];
    char *ddl = buf;
    HSLogMan *LM = HSLogMan::Instance();

    HS_ASSERT(labelAccessed_ == TRUE);
    LM->StartTimer("Collect file statistics");
    // File-Level Statistics are only determined for MX objects
    if (objActualFormat_ == SQLMX)
      {
        // feature is not supported
        HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "FILE_STATS");
        retcode = -1;
      }

    LM->StopTimer();
    return retcode;
  }

//=====================================================

NABoolean HSHiveTableDef::objExists(NABoolean createExternalTable)
{
  setNATable();
  if (!naTbl_)
    return FALSE;

  if (!setObjectUID(createExternalTable))
    return FALSE;

  // cannot upd stats if the object is a hive view
  if (naTbl_->getViewText())
    {
      *CmpCommon::diags()
          << DgSqlCode(-UERR_INVALID_OBJECT)
          << DgString0(naTbl_->getTableName().getQualifiedNameAsString());

      return FALSE;
    }

  tableStats_ = naTbl_->getClusteringIndex()->getHHDFSTableStats();

  *ansiName_ = *catalog_;
  ansiName_->append('.');
  ansiName_->append(*schema_);
  ansiName_->append('.');
  ansiName_->append(*object_);

  HiveMetaData* hiveMetaDB;
  if (CmpCommon::getDefault(HIVE_USE_FAKE_TABLE_DESC) != DF_ON)
    {
      hiveMetaDB = new(STMTHEAP) HiveMetaData(STMTHEAP);

      if (!hiveMetaDB->init())
        {
          *CmpCommon::diags() << DgSqlCode(-1190)
                              << DgString0(hiveMetaDB->getErrMethodName())
                              << DgString1(hiveMetaDB->getErrCodeStr())
                              << DgString2(hiveMetaDB->getErrDetail())
                              << DgInt0(hiveMetaDB->getErrCode());
          NADELETEBASIC(hiveMetaDB, STMTHEAP);  // HiveMetaData not 
          //derived from NABasicObject
          return FALSE;
        }
    }
  else
    hiveMetaDB = new(STMTHEAP) HiveMetaData(STMTHEAP); // fake metadata

  if (!HSGlobalsClass::isHiveCat(*catalog_))
    {
      *CmpCommon::diags()
          << DgSqlCode(-1388)
          << DgString0("Object")
          << DgString1(*object_);
      return FALSE;
    }

  // The default schema name is what the Hive default schema is called in SeaHive.
  NAString defSchema = ActiveSchemaDB()->getDefaults().getValue(HIVE_DEFAULT_SCHEMA);
  defSchema.toUpper();
  NAString obj = *object_;
  NAString sch = *schema_;
  if (sch == defSchema)
    sch = hiveMetaDB->getDefaultSchemaName();

  // Hive stores names in lower case.
  sch.toLower();
  obj.toLower();

  if (CmpCommon::getDefault(HIVE_USE_FAKE_TABLE_DESC) == DF_ON)
    hiveTblDesc_ = hiveMetaDB->getFakedTableDesc(obj.data());
  else
    hiveTblDesc_ = hiveMetaDB->getTableDesc(sch.data(), obj.data(),
                FALSE,
                // reread Hive Table Desc from MD.
                (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_ON),
                TRUE);

  if (!hiveTblDesc_)
  {
    if ((hiveMetaDB->getErrCode() == 0)||(hiveMetaDB->getErrCode() == 100))
    {
      *CmpCommon::diags()
        << DgSqlCode(-1388)
        << DgString0("Object")
        << DgString1(*object_);
    }
    else
    {
      *CmpCommon::diags()
        << DgSqlCode(-1192)
        << DgString0(hiveMetaDB->getErrMethodName())
        << DgString1(hiveMetaDB->getErrCodeStr())
        << DgString2(hiveMetaDB->getErrDetail())
        << DgInt0(hiveMetaDB->getErrCode());
        hiveMetaDB->resetErrorInfo();
    } 
    return FALSE;
  }

  objActualFormat_ = SQLMX;

  return TRUE;
}

NAString HSHiveTableDef::getNodeName() const
  {
    HS_ASSERT(FALSE);  // MP only
    return "";
  }

NAString HSHiveTableDef::getCatalogLoc(formatType format) const
  {
    HS_ASSERT(FALSE);  // MP only
    return "";
  }

NAString HSHiveTableDef::getHistLoc(formatType format) const
{
  return HIVE_STATS_CATALOG "." HIVE_STATS_SCHEMA;
}

Int64 HSHiveTableDef::getRowCount(NABoolean &isEstimate,
                                  Int64 &numInserts,
                                  Int64 &numDeletes,
                                  Int64 &numUpdates,
                                  Int64 &numPartitions,
                                  Int64 &minRowCtPerPartition,
                                  Int32 &errorCode,
                                  Int32 &breadCrumb,
                                  NABoolean estimateIfNecessary)
{
  errorCode = 0;
  breadCrumb = -6;
  if (minPartitionRows_ == -1)
    {
      Int64 partitionEstRows;
      for (CollIndex i=0; i<tableStats_->entries(); i++)
        {
          partitionEstRows = (*tableStats_)[i]->getEstimatedRowCount();
          if (minPartitionRows_ == -1 || partitionEstRows < minPartitionRows_)
            minPartitionRows_ = partitionEstRows;
        }
    }

  numInserts = numDeletes = numUpdates = 0;
  numPartitions = getNumPartitions();
  minRowCtPerPartition = minPartitionRows_;

  return getRowCount(isEstimate, errorCode, breadCrumb, estimateIfNecessary);
}

Lng32 HSHiveTableDef::DescribeColumnNames()
{
  hive_column_desc* hiveColDesc = hiveTblDesc_->getColumns();
  while (hiveColDesc)
    {
      numCols_++;
      hiveColDesc = hiveColDesc->next_;
    }

  colInfo_ = new(STMTHEAP) HSColumnStruct[numCols_];
  hiveColDesc = hiveTblDesc_->getColumns();
  for (Int32 i=0; i<numCols_; i++)
    {
      *(colInfo_[i].colname) = hiveColDesc->name_;
      colInfo_[i].colname->toUpper();
      *colInfo_[i].externalColumnName = getAnsiName(*colInfo_[i].colname);

      NAType* natype = getSQColTypeForHive(hiveColDesc->type_, STMTHEAP);
      colInfo_[i].datatype = natype->getFSDatatype();
      colInfo_[i].nullflag = natype->supportsSQLnullLogical();
      colInfo_[i].charset = natype->getCharSet();
      if (DFS2REC::isAnyCharacter(natype->getFSDatatype()))
        {
          colInfo_[i].colCollation = (static_cast<CharType*>(natype))->getCollation();
          colInfo_[i].caseInsensitive = (static_cast<CharType*>(natype))->isCaseinsensitive();
        }
      colInfo_[i].length = natype->getNominalSize();
      colInfo_[i].precision = natype->getPrecision();
      colInfo_[i].scale = natype->getScale();

      hiveColDesc = hiveColDesc->next_;
    }

  return 0;
}

//=====================================================
//
NAString HSHbaseTableDef::getNodeName() const
  {
    HS_ASSERT(FALSE);  // MP only
    return "";
  }

NAString HSHbaseTableDef::getCatalogLoc(formatType format) const
  {
    HS_ASSERT(FALSE);  // MP only
    return "";
  }

NAString HSHbaseTableDef::getHistLoc(formatType format) const
{
  if ( HSGlobalsClass::isNativeHbaseCat(getCatName(format))) {
    return HBASE_STATS_CATALOG "." HBASE_STATS_SCHEMA;
  } else {  
    NAString name(getCatName(format));
    name.append(".");
    name.append(getSchemaName(format));
    return name;
  }
}

static 
Lng32 RegisterHiveTable(const NAString& catName, const NAString& schName, const NAString& nativeTableName)
{
   HSLogMan *LM = HSLogMan::Instance();
   if (LM->LogNeeded())
      {
        snprintf(LM->msg, sizeof(LM->msg), "Registering hive table %s on demand.",
                          nativeTableName.data());
        LM->Log(LM->msg);
      }

   // do not have to worry about the catalog and schema for the new external table 
   // here. These names will be determined by the processing logic. 
   NAString ddl = "REGISTER INTERNAL HIVE TABLE IF NOT EXISTS ";
   ddl.append(catName);
   ddl.append(".");
   ddl.append(schName);
   ddl.append(".");
   ddl.append(nativeTableName);

   // set INTERNAL_QUERY_FROM_EXEUTIL bit in Sql_ParserFlags.
   // This is needed to process 'register internal' syntax
   ULng32 flagToSet = INTERNAL_QUERY_FROM_EXEUTIL;
   PushAndSetSqlParserFlags savedParserFlags(flagToSet);

   Lng32 retcode = HSFuncExecDDL(ddl.data(), - UERR_INTERNAL_ERROR, NULL,
                            "register hive table", NULL);

   if (retcode < 0 && LM->LogNeeded())
      {
        snprintf(LM->msg, sizeof(LM->msg), "Registration of the hive table failed.");
        LM->Log(LM->msg);
      }

   return retcode;
}

static 
Lng32 RegisterHBaseTable(const NAString& catName, const NAString& schName, 
                         const NAString& nativeTableName)
{
   HSLogMan *LM = HSLogMan::Instance();
   if (LM->LogNeeded())
      {
        snprintf(LM->msg, sizeof(LM->msg), "Registering hbase table %s on demand.",
                 nativeTableName.data());
        LM->Log(LM->msg);
      }

   // do not have to worry about the catalog and schema for the new external table 
   // here. These names will be determined by the processing logic. 
   NAString ddl = "REGISTER INTERNAL HBASE TABLE IF NOT EXISTS ";
   ddl.append(catName);
   ddl.append(".");
   ddl.append(schName);
   ddl.append(".");
   ddl.append(nativeTableName);

   // set INTERNAL_QUERY_FROM_EXEUTIL bit in Sql_ParserFlags.
   // This is needed to process 'register internal' syntax
   ULng32 flagToSet = INTERNAL_QUERY_FROM_EXEUTIL;
   PushAndSetSqlParserFlags savedParserFlags(flagToSet);

   Lng32 retcode = HSFuncExecDDL(ddl.data(), - UERR_INTERNAL_ERROR, NULL,
                            "register hbase table", NULL);

   if (retcode < 0 && LM->LogNeeded())
      {
        snprintf(LM->msg, sizeof(LM->msg), "Registration of the hbase table failed.");
        LM->Log(LM->msg);
      }

   return retcode;
}

NABoolean HSTableDef::setObjectUID(NABoolean createExternalTable)
{
  objectUID_ = naTbl_->objectUid().get_value();

  if (createExternalTable && objectUID_ <= 0 && 
      HSGlobalsClass::isNativeCat(getCatName(EXTERNAL_FORMAT)) ) {

    // If objectUID is not set, it means there is no corresponding
    // external table created for it. Need to create one here.
    NAString catName = getCatName(EXTERNAL_FORMAT);
    NAString schName = getSchemaName(EXTERNAL_FORMAT);
    NAString objName = getObjectName(EXTERNAL_FORMAT);
    Lng32 retcode = 0;

    setNATable();
    if (!naTbl_)
      return FALSE;

   if ((catName == HIVE_SYSTEM_CATALOG) &&
        (naTbl_->isView()))
      {
        CmpCommon::diags()->clear();
        *CmpCommon::diags()
          << DgSqlCode(-UERR_INVALID_OBJECT)
          << DgString0(naTbl_->getTableName().getQualifiedNameAsString());
        
        return FALSE;
      }

    if (catName == HIVE_SYSTEM_CATALOG)
      retcode = RegisterHiveTable(catName, schName, objName);
    else
      retcode = RegisterHBaseTable(catName, schName, objName);

    if (retcode != 0)
      return FALSE;

    setNATable();
    if (!naTbl_)
      return FALSE;

    CorrName corrName(objName, STMTHEAP, schName, catName);
    if ( !naTbl_->fetchObjectUIDForNativeTable(corrName, naTbl_->isView()) )
      return FALSE;

    objectUID_ = naTbl_->objectUid().get_value();

    HSLogMan *LM = HSLogMan::Instance();
    if (LM->LogNeeded()) {
       sprintf(LM->msg, "NATable::fetchObjectUIDForNativeTable() returns %ld\n", objectUID_);
       LM->Log(LM->msg);
    }
  }

  if ( createExternalTable )
    return (objectUID_ > 0);
  else 
    return TRUE;
}

NABoolean HSHbaseTableDef::objExists(NABoolean createExternalTable)
{
  HSLogMan *LM = HSLogMan::Instance();

  if (LM->LogNeeded()) {
     sprintf(LM->msg, "call HSHbaseTableDef::objExists\n");
     LM->Log(LM->msg);
  }

  setNATable();
  if (!naTbl_)
    return FALSE;

  if (!setObjectUID(createExternalTable))
    return FALSE;


  *ansiName_ = getCatName(EXTERNAL_FORMAT);
  ansiName_->append(".");
  ansiName_->append(getSchemaName(EXTERNAL_FORMAT));
  ansiName_->append(".");
  ansiName_->append(getObjectName(EXTERNAL_FORMAT));

  if (!HSGlobalsClass::isHbaseCat(*catalog_))
    {
      *CmpCommon::diags()
          << DgSqlCode(-1389)
          << DgTableName(*object_);
      return FALSE;
    }

  Lng32 retcode_ = getColumnNames();
  HSHandleError(retcode_);

  if (LM->LogNeeded()) {
     sprintf(LM->msg, "objectUID_ set to %ld\n", objectUID_);
     sprintf(LM->msg, "naTbl_->objectUid() is %ld\n", naTbl_->objectUid().get_value());
     LM->Log(LM->msg);
           
  }


  objActualFormat_ = SQLMX;

  return TRUE;
}

Lng32 HSHbaseTableDef::getNumPartitions() const
{
  return getNATable()->getClusteringIndex()->getCountOfPartitions();
}

Int64 HSHbaseTableDef::getRowCount(NABoolean &isEstimate, 
                                   Int32 &errorCode,
                                   Int32 &breadCrumb,
                                   NABoolean estimateIfNecessary)
{
  errorCode = 0;
  breadCrumb = -2;
  isEstimate = TRUE;
  if (estimateIfNecessary &&
      !naTbl_->isSeabaseMDTable() &&
      CmpCommon::getDefault(USTAT_ESTIMATE_HBASE_ROW_COUNT) == DF_ON)
    // use a 4 minute retry limit (expressed in milliseconds)
    return naTbl_->estimateHBaseRowCount(4*60*1000, errorCode, breadCrumb);
  else
    return 0;
}

Int64 HSHbaseTableDef::getRowCount(NABoolean &isEstimate,
                                  Int64 &numInserts,
                                  Int64 &numDeletes,
                                  Int64 &numUpdates,
                                  Int64 &numPartitions,
                                  Int64 &minRowCtPerPartition,
                                  Int32 &errorCode,
                                  Int32 &breadCrumb,
                                  NABoolean estimateIfNecessary)
{
  // Comparable code for Hive tables:
  //if (minPartitionRows_ == -1)
  //  {
  //    Int64 partitionEstRows;
  //    for (CollIndex i=0; i<tableStats_->entries(); i++)
  //      {
  //        partitionEstRows = (*tableStats_)[i]->getEstimatedRowCount();
  //        if (minPartitionRows_ == -1 || partitionEstRows < minPartitionRows_)
  //          minPartitionRows_ = partitionEstRows;
  //      }
  //  }
  //numInserts = numDeletes = numUpdates = 0;
  //numPartitions = getNumPartitions();
  //minRowCtPerPartition = minPartitionRows_;

  return getRowCount(isEstimate, errorCode, breadCrumb, estimateIfNecessary);
}

Lng32 HSHbaseTableDef::DescribeColumnNames()
{
  // This will be called from AddTableName() with the default (NULL) argument,
  // but for this subclass, it will have already been called from objExists()
  // with a non-null NATable*.
  if (!naTbl_)
    return 0;

  const NAType* natype;
  const NAColumnArray& colArr = naTbl_->getNAColumnArray();
  numCols_ = colArr.entries();
  colInfo_ = new(STMTHEAP) HSColumnStruct[numCols_];
  for (CollIndex i=0; i<numCols_; i++)
    {
      colInfo_[i].colnum = i;  // position of col in table
      *(colInfo_[i].colname) = colArr[i]->getColName();
      *colInfo_[i].externalColumnName = getAnsiName(*colInfo_[i].colname);
      natype = colArr[i]->getType();
      colInfo_[i].datatype = natype->getFSDatatype();
      colInfo_[i].nullflag = natype->supportsSQLnullLogical();
      colInfo_[i].charset = natype->getCharSet();
      if (DFS2REC::isAnyCharacter(natype->getFSDatatype()))
        {
          colInfo_[i].colCollation = (static_cast<const CharType*>(natype))->getCollation();
          colInfo_[i].caseInsensitive = (static_cast<const CharType*>(natype))->isCaseinsensitive();
        }
      colInfo_[i].length = natype->getNominalSize();
      colInfo_[i].precision = natype->getPrecision();
      colInfo_[i].scale = natype->getScale();
    }

  return 0;
}

//
// METHOD:  addTruncatedSelectList()
//
// PURPOSE: Generates a SELECT list consisting of 
//          column references or a SUBSTRING
//          on column references which truncates the
//          column to the maximum length allowed in
//          UPDATE STATISTICS. Also skips LOB columns.
//
// INPUT:   'qry' - the SQL query string to append the 
//          select list to.
//
void HSTableDef::addTruncatedSelectList(NAString & qry)
  {
    bool first = true;
    for (Lng32 i = 0; i < getNumCols(); i++)
      {
        if (DFS2REC::isLOB(getColInfo(i).datatype)) // skip LOB columns
          continue;

        // skip derived column names (e.g. "_SALT_", "_DIVISION_n_")
        // but only in Trafodion tables
        if ((getTblOrigin() != HBASE_TBL) ||
            (!ComTrafReservedColName(*getColInfo(i).colname)))
          {
            if (!first)
              qry += ", ";

            getColInfo(i).addTruncatedColumnReference(qry);
            first = false;
          }
      }
  }

//
// METHOD:  allUserColumnsAreLOBs()
//
// PURPOSE: Determines if all user columns are LOBs. If so,
//          returns TRUE. Otherwise, returns FALSE.
//
NABoolean HSTableDef::allUserColumnsAreLOBs()
  {
    for (Int32 i = 0; i < numCols_; i++)
      {
        if (!DFS2REC::isLOB(colInfo_[i].datatype))
          {
            if (strcmp(*colInfo_[i].colname,"SYSKEY") != 0) // SYSKEY is not a user column
              return FALSE;
            // we don't need to check for "_SALT_" or divisioning columns
            // since those could only be present if there were user key columns
          }
      }
    return TRUE;
  }


