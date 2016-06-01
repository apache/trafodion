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
 * File:         hs_update.C
 * Descriptioon: Entry for UPDATE STATISTICS.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#define HS_FILE "hs_update"




#include <stdlib.h>
#include "Platform.h"                                    // NT_PORT SK 02/08/97
#include "ComDiags.h"
#include "CmpCommon.h"
#include "cli_stdh.h"
#include "hs_globals.h"
#include "hs_parser.h"
#include "hs_cli.h"
#include "hs_auto.h"
#include "NATable.h"
#include "ComSpace.h" 
#include "CmpDescribe.h"
#include "ComSmallDefs.h"
#define   SQLPARSERGLOBALS_FLAGS				  
#include "SqlParserGlobals.h"
#include "SchemaDB.h"
#include "ReadTableDef.h"

THREAD_P char *hs_input = NULL;
THREAD_P HSGlobalsClass *hs_globals_y = NULL; // Declare global pointer to hs_globals.  Used by 
                                     // parser (Yacc code) only.  Assigned by ShowStats()
                                     // and UpdateStats() functions.
THREAD_P Lng32 HSCliStatement::statementNum = 0;

// -----------------------------------------------------------------------
// Entry for SHOWSTATS.
// -----------------------------------------------------------------------
Lng32 ShowStats(const char* input, char* &outBuf,
               ULng32 &outBufLen, CollHeap *heap)
  {
    WMSController wmsController;  // Control WMS CQD settings in secondary compiler.

    Lng32 retcode = 0;
    Space space;
    hs_input = (char *) input;
    ComDiagsArea diags(STMTHEAP);
    HSGlobalsClass::schemaVersion = COM_VERS_UNKNOWN;

    ComDiagsArea *ptrDiags = CmpCommon::diags();
    if (!ptrDiags)
      ptrDiags = &diags;

    HSLogMan *LM = HSLogMan::Instance();
    LM->Log("\nSHOWSTATS\n=====================================================================");

    HSGlobalsClass hs_globals_obj(*ptrDiags);
    hs_globals_y = &hs_globals_obj;

    NAString displayData("\n");
    
    // Do not show missing histogram warnings
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT HIST_MISSING_STATS_WARNING_LEVEL '0'");
    HSExitIfError(retcode);
    // Turn off automation for internal queries
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_AUTOMATION_INTERVAL '0'");
    HSExitIfError(retcode);
    // Turn off quick stats
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT HIST_ON_DEMAND_STATS_SIZE '0'");
    HSExitIfError(retcode);

    // Allow select * to return system added columns
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT MV_ALLOW_SELECT_SYSTEM_ADDED_COLUMNS 'ON'");
    HSExitIfError(retcode);

    // Parse showstats statement.
    retcode = HSFuncParseStmt();
    HSExitIfError(retcode);

    // check privileges
    NABoolean isShowStats = TRUE;
    if (!hs_globals_obj.isAuthorized(isShowStats))
      {
        HSFuncMergeDiags(-UERR_NO_PRIVILEGE, hs_globals_obj.user_table->data());
        retcode = -1;
        HSExitIfError(retcode);
      }

    // histogram versioning
    if (hs_globals_obj.tableFormat == SQLMX) 
      {
        HSGlobalsClass::schemaVersion = getTableSchemaVersion(*(hs_globals_obj.user_table));
        // LCOV_EXCL_START :rfi
        if (HSGlobalsClass::schemaVersion == COM_VERS_UNKNOWN)
        {
          HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "GET_SCHEMA_VERSION");
          retcode = -1;
          HSExitIfError(retcode);
        }
        // LCOV_EXCL_STOP
      }
    if (LM->LogNeeded())
      {
        sprintf(LM->msg, "\nShowStats: TABLE: %s; SCHEMA VERSION: %d\n", 
                  hs_globals_obj.user_table->data(), 
                  HSGlobalsClass::schemaVersion);
        LM->Log(LM->msg);
      }
    retcode = hs_globals_obj.GetStatistics(displayData, space);
    space.allocateAndCopyToAlignedSpace(displayData, displayData.length(), sizeof(short));
    outBufLen = space.getAllocatedSpaceSize();
    outBuf = new (heap) char[outBufLen];
    space.makeContiguous(outBuf, outBufLen);

    // Reset CQDs.
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT HIST_MISSING_STATS_WARNING_LEVEL RESET");
    HSExitIfError(retcode);
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_AUTOMATION_INTERVAL RESET");
    HSExitIfError(retcode);
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT HIST_ON_DEMAND_STATS_SIZE RESET");
    HSExitIfError(retcode);
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT MV_ALLOW_SELECT_SYSTEM_ADDED_COLUMNS RESET");
    HSExitIfError(retcode);

    HSClearCLIDiagnostics();
    hs_globals_y = NULL;
    return retcode;
  }



// -----------------------------------------------------------------------
// Entry for UPDATE STATISTICS.
// -----------------------------------------------------------------------
Lng32 UpdateStats(char *input, NABoolean requestedByCompiler)
  {
    Lng32 retcode = 0;
    HSCliStatement::statementNum = 0;
    hs_input = input;
    ComDiagsArea diags(STMTHEAP);
    HSGlobalsClass::schemaVersion = COM_VERS_UNKNOWN;
    HSGlobalsClass::autoInterval = 0;
    sortBuffer1 = NULL;
    sortBuffer2 = NULL;

    HSLogMan *LM = HSLogMan::Instance();

    ComDiagsArea *ptrDiags = CmpCommon::diags();
    if (!ptrDiags)
      ptrDiags = &diags;

    LM->Log("\nUPDATE STATISTICS\n=====================================================================");
    LM->Log(hs_input);
    LM->StartTimer("UpdateStats()");
    HSPrologEpilog pe("UpdateStats()");
    HSGlobalsClass hs_globals_obj(*ptrDiags);
    hs_globals_obj.requestedByCompiler = requestedByCompiler;
    hs_globals_y = &hs_globals_obj;
#ifdef _TEST_ALLOC_FAILURE
    HSColGroupStruct::allocCount = 1;  // start at 1 for each new statement
#endif

    // Disallow UPDATE STATS in a user transaction
    HSTranMan *TM = HSTranMan::Instance();
    if (TM->InTransaction())
      {
        HSFuncMergeDiags(-UERR_USER_TRANSACTION);
        retcode = -1;
        HSExitIfError(retcode);
      }


                                             /*==============================*/
                                             /*       PARSE STATEMENT        */
                                             /*==============================*/
    // Set up cnotrols (CQDs) for queries.  These queries will be run in a spawned MXCMP.
    LM->StartTimer("Setup CQDs prior to parsing");
    retcode = sendAllControls(FALSE,  // do not copyCQS
                              FALSE,  // do not sendAllCQDs
                              FALSE,  // do not sendUserCQDs
                              COM_VERS_COMPILER_VERSION); // versionOfCmplrRcvCntrlInfo
    HSExitIfError(retcode);
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT QUERY_CACHE '0'");
    HSExitIfError(retcode);
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT CACHE_HISTOGRAMS 'OFF'");
    HSExitIfError(retcode);
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_MODIFY_DEFAULT_UEC '0.05'");
    HSExitIfError(retcode);
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT OUTPUT_DATE_FORMAT 'ANSI'");
    HSExitIfError(retcode);
    // Do not show missing histogram warnings
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT HIST_MISSING_STATS_WARNING_LEVEL '0'");
    HSExitIfError(retcode);
    // Turn off automation for internal queries
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_AUTOMATION_INTERVAL '0'");
    HSExitIfError(retcode);
    // Allow select * to return system added columns
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT MV_ALLOW_SELECT_SYSTEM_ADDED_COLUMNS 'ON'");
    HSExitIfError(retcode);
    // Turn off on demand stats (Quick stats) for internal queries.  
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT HIST_ON_DEMAND_STATS_SIZE '0'");
    HSExitIfError(retcode);

    // If ISOLATION_LEVEL is read committed, pass this to other MXCMP.
    if (CmpCommon::getDefault(ISOLATION_LEVEL) == DF_READ_COMMITTED)
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT ISOLATION_LEVEL 'READ COMMITTED'");
    HSExitIfError(retcode);

    // If ISOLATION_LEVEL for updates is read committed, pass this to other MXCMP.
    if (CmpCommon::getDefault(ISOLATION_LEVEL_FOR_UPDATES) == DF_READ_COMMITTED)
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT ISOLATION_LEVEL_FOR_UPDATES 'READ COMMITTED'");
    HSExitIfError(retcode);

    // may need to query the non-audit sample table. If CQD is off the only
    // type of DML allowed on a non-audit table are sideTree inserts
    HSFuncExecQuery("CONTROL QUERY DEFAULT ALLOW_DML_ON_NONAUDITED_TABLE 'ON'");


    // allow select * to return system added columns
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT MV_ALLOW_SELECT_SYSTEM_ADDED_COLUMNS 'ON'");
    HSExitIfError(retcode);
    // the next 2 defaults are used to enable nullable primary and store by
    // keys. If these are enabled, send these 2 cqds to the second mxcmp so
    // the sample table could be created with nullable keys.
    if (CmpCommon::getDefault(ALLOW_NULLABLE_UNIQUE_KEY_CONSTRAINT) == DF_ON)
      {
	retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT ALLOW_NULLABLE_UNIQUE_KEY_CONSTRAINT 'ON'");
	HSExitIfError(retcode);
      }
    else
      {
	retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT ALLOW_NULLABLE_UNIQUE_KEY_CONSTRAINT 'OFF'");
	HSExitIfError(retcode);
      }

    // OFF enables no error on nullable storeby
    if (CmpCommon::getDefault(CAT_ERROR_ON_NOTNULL_STOREBY) == DF_OFF)
      {
	retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT CAT_ERROR_ON_NOTNULL_STOREBY 'OFF'");
	HSExitIfError(retcode);
      }
    else
      {
	retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT CAT_ERROR_ON_NOTNULL_STOREBY 'ON'");
	HSExitIfError(retcode);
      }

     if (CmpCommon::getDefault(WMS_CHILD_QUERY_MONITORING) == DF_OFF)
       {
	 retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT WMS_CHILD_QUERY_MONITORING 'OFF'");
	 HSExitIfError(retcode);
       }
     else
       {
	 retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT WMS_CHILD_QUERY_MONITORING 'ON'");
	 HSExitIfError(retcode);
       }
     if (CmpCommon::getDefault(WMS_QUERY_MONITORING) == DF_OFF)
       {
	 retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT WMS_QUERY_MONITORING 'OFF'");
	 HSExitIfError(retcode);
       }
     else
       {
	 retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT WMS_QUERY_MONITORING 'ON'");
	 HSExitIfError(retcode);
       }
    LM->StopTimer();

    LM->StartTimer("Parse statement");

    retcode = HSFuncParseStmt();
    LM->StopTimer();
    HSExitIfError(retcode);
                                             /*==============================*/
                                             /*    CHECK SCHEMA VERSION      */
                                             /*==============================*/
    // Also checked in AddTableName() during parse.  Check again, just in case we don't 
    // reach that code.  HISTOGRAM corruption can occur if this is not set.
    // LCOV_EXCL_START :rfi
    if (hs_globals_obj.tableFormat == SQLMX && HSGlobalsClass::schemaVersion == COM_VERS_UNKNOWN)
    {
      HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "GET_SCHEMA_VERSION");
      retcode = -1;
      HSExitIfError(retcode);
    }
    // LCOV_EXCL_STOP

                                                /*==============================*/
                                                /* HANDLE OPTIONS WHICH DO NOT  */
                                                /* MODIFY HISTOGRAMS.           */
                                                /*==============================*/
    if (hs_globals_obj.optFlags & LOG_OPT)            /* log option requested   */
      return 0;

                                                /*==============================*/
                                                /* VERIFY THAT THE REQUESTOR    */
                                                /* HAS NECESSARY PRIVILEGES.    */
                                                /*==============================*/

    NABoolean isShowStats = FALSE;
    if (!hs_globals_obj.isAuthorized(isShowStats))
      {
        HSFuncMergeDiags(-UERR_NO_PRIVILEGE, hs_globals_obj.user_table->data());
        retcode = -1;
        HSExitIfError(retcode);
      }


     // for ustat - we selectively allow sending specific external CQDs to the
     // secondary compilers instead of allowing all external CQDS to be sent so
     // to not affect the performance of the dynamic statements executed by ustat
     NADefaults &defs = CmpCommon::context()->schemaDB_->getDefaults();
     char* allowedCqds = (char*)defs.getValue(USTAT_CQDS_ALLOWED_FOR_SPAWNED_COMPILERS);
     Int32 allowedCqdsSize = 0;
     if (allowedCqds)
     {
        allowedCqdsSize = strlen(allowedCqds);
     }

     if (allowedCqdsSize && (allowedCqdsSize < USTAT_CQDS_ALLOWED_FOR_SPAWNED_COMPILERS_MAX_SIZE))
     {
        if (LM->LogNeeded())
        {
           sprintf(LM->msg, "\nUSTAT_CQDS_ALLOWED_FOR_SPAWNED_COMPILERS is not empty, and its value is (%s)", allowedCqds);
           LM->Log(LM->msg);
        }

        char* filterString = new (STMTHEAP) char[allowedCqdsSize+1];
        // We need to make a copy of the CQD value here since strtok
        // overwrites delims with nulls in stored cqd value.
        strcpy(filterString, allowedCqds);
        char* name = strtok(filterString, ",");

        while (name)
        {
           if (LM->LogNeeded())
           {
              sprintf(LM->msg, "\n\tCQD name: (%s)", name);
              LM->Log(LM->msg);
           }

           DefaultConstants attrEnum = defs.lookupAttrName(name, -1);
           char* value = (char*)defs.getValue(attrEnum);

           if (value)
           {
             NAString quotedString;
             ToQuotedString (quotedString, value);
             char buf[strlen(name)+quotedString.length()+4+1+1+1];  // room for "CQD %s %s;" and null terminator
             sprintf(buf, "CQD %s %s;", name, quotedString.data());
             retcode = HSFuncExecQuery(buf);

             HSExitIfError(retcode);
           }

           name = strtok(NULL, ",");
        }

        NADELETEBASIC(filterString, STMTHEAP);
     }
     else // size is zero or too large
     {
        if (LM->LogNeeded())
        {
           sprintf(LM->msg, "\nUSTAT_CQDS_ALLOWED_FOR_SPAWNED_COMPILERS size of (%d) is not acceptable", allowedCqdsSize);
           LM->Log(LM->msg);
        }
     }

    if (hs_globals_obj.optFlags & CREATE_SAMPLE_OPT ||/* create sample requested*/
        hs_globals_obj.optFlags & REMOVE_SAMPLE_OPT)  /* delete sample requested*/
    {
      retcode =  managePersistentSamples();
      HSClearCLIDiagnostics();
      return retcode;
    }

    LM->StartTimer("Initialize environment");
    retcode = hs_globals_obj.Initialize();      /* Initialize structures.       */
    LM->StopTimer();
    HSExitIfError(retcode);

    if (hs_globals_obj.optFlags & VIEWONLY_OPT)
    {
      return 0;
    }

                                                /*==============================*/
                                                /*      COLLECT STATISTICS      */
                                                /*==============================*/
    // If an elapsed-time threshold for activating logging is defined for the
    // source table, set it, unless logging is already on.
    if (!LM->LogNeeded())
      {
        hs_globals_obj.setJitLogThreshold();
        if (hs_globals_obj.getJitLogThreshold() > 0)
          hs_globals_obj.setStmtStartTime(hs_getEpochTime());
      }

    if (hs_globals_obj.StatsNeeded())
      {
        retcode = hs_globals_obj.CollectStatistics();
        HSExitIfError(retcode);
      }


    // do not care about warning messages now  
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT HIST_MISSING_STATS_WARNING_LEVEL RESET");
    HSExitIfError(retcode);

    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT USTAT_AUTOMATION_INTERVAL RESET");
    HSExitIfError(retcode);

    // don't need to select system added columns anymore
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT MV_ALLOW_SELECT_SYSTEM_ADDED_COLUMNS RESET");

    // reset the no dml on non-audit table CQD
    HSFuncExecQuery("CONTROL QUERY DEFAULT ALLOW_DML_ON_NONAUDITED_TABLE RESET");

                                             /*==============================*/
                                             /*    FLUSH/WRITE STATISTICS    */
                                             /*==============================*/
    NABoolean statsWritten = FALSE;
    LM->StartTimer("Flush out stats");
    retcode = hs_globals_obj.FlushStatistics(statsWritten);
    LM->StopTimer();
    HSExitIfError(retcode);

    // Clear this after flush statistics.  FILE_STATS requires this.
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT ISOLATION_LEVEL RESET");
    HSExitIfError(retcode);

    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT ISOLATION_LEVEL_FOR_UPDATES RESET");
    HSExitIfError(retcode);

    // Reset insert/delete/update counters if NECESSARY, EVERY or EXISTING was used.
    if (statsWritten && 
        ((hs_globals_obj.optFlags & NECESSARY_OPT && !hs_globals_obj.allMissingStats) ||
         (hs_globals_obj.optFlags & EVERYCOL_OPT) || 
         (hs_globals_obj.optFlags & EXISTING_OPT) )) 
      {
        // Reset the row counts if the stats were written successfully.
        LM->Log("Resetting ins/upd/del DP2 counts.");
        hs_globals_obj.objDef->resetRowCounts();
      }


    // Reset the quick stats setting.
    retcode = HSFuncExecQuery("CONTROL QUERY DEFAULT HIST_ON_DEMAND_STATS_SIZE RESET");
    HSExitIfError(retcode);

    // set Update Stats time on successful completion of update statistics
    TimeVal currTime;
    GETTIMEOFDAY(&currTime, 0); 
    Int64 lastUpdateTime = currTime.tv_sec;
    HistogramsCacheEntry::setUpdateStatsTime(lastUpdateTime);    // for use by the optimizer

    HSClearCLIDiagnostics();

    if ( hs_globals_obj.canDoIUS() )
      hs_globals_obj.end_IUS_work();

    hs_globals_y = NULL;

    // Remove IUS persistent sample if necessary.
    //@ZXhbase -- need to make sure seabase.seabase.persistent_samples exists, and
    //            provide dynamic version of CURSOR_PST_REASON_CODE.
#ifdef NA_USTAT_USE_STATIC  // use static query defined in module file
    if (statsWritten)
      {
        // The update has completed successfully. If it was neither a persistent
        // (i.e., using the PERSISTENT keyword) RUS nor an IUS, drop the target
        // table's IUS persistent sample if it exists, and remove the corresponding
        // row from the PERSISTENT_SAMPLES table.
        HSGlobalsClass* hs_globals = GetHSContext();
        if (!(hs_globals->optFlags & IUS_PERSIST ||
              (hs_globals->okToPerformIUS() &&
               hs_globals->wherePredicateSpecifiedForIUS())))
          {
            Int64 dummy1, dummy2;
            double dummy3;
            HSPersSamples* ps =
                  HSPersSamples::Instance(hs_globals->objDef->getCatName(), FALSE);
            NAString IUSSampTblName;
            retcode = ps->find(hs_globals->objDef, char('I'), IUSSampTblName,
                               dummy1, dummy2, dummy3);
            if (retcode >= 0 && IUSSampTblName.length() > 0)
              ps->removeSample(hs_globals->objDef, IUSSampTblName, TRUE,
                               "DROP IUS PERSISTENT SAMPLE TABLE AND REMOVE FROM LIST");
          }
      }
#endif
    LM->StopTimer();

    return retcode;
  }

