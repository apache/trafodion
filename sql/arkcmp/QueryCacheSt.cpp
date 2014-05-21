/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2002-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         QueryCacheSt.cpp
 * Description:
 *
 *
 * Created:      3/19/2002
 * Language:     C++
 *
 *****************************************************************************
 */

#include "QueryCacheSt.h"
#include "QCache.h"
#include "CmpMain.h"

SP_STATUS QueryCacheStatStoredProcedure::sp_NumOutputFields(Lng32 *numFields,
							    SP_COMPILE_HANDLE spCompileObj,
							    SP_HANDLE spObj,
							    SP_ERROR_STRUCT *error)
{
  *numFields = 21;
  return SP_SUCCESS;
}


// Specifies the columns of the QueryCache table and their types
SP_STATUS QueryCacheStatStoredProcedure::sp_OutputFormat(SP_FIELDDESC_STRUCT *format,
							 SP_KEYDESC_STRUCT keyFields[],
							 Lng32 *numKeyFields,
							 SP_HANDLE spCompileObj,
							 SP_HANDLE spObj,
							 SP_ERROR_STRUCT *error)
{
  strcpy(&((format++)->COLUMN_DEF[0]), "Avg_template_size     	INT UNSIGNED"); //  0
  strcpy(&((format++)->COLUMN_DEF[0]), "Current_size       	INT UNSIGNED"); //  1
  strcpy(&((format++)->COLUMN_DEF[0]), "Max_cache_size        	INT UNSIGNED"); //  2
  strcpy(&((format++)->COLUMN_DEF[0]), "Max_num_victims        	INT UNSIGNED"); //  3
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_entries    	    	INT UNSIGNED"); //  4
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_plans    	    	INT UNSIGNED"); //  5
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_compiles        	INT UNSIGNED"); //  6
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_recompiles        	INT UNSIGNED"); //  7
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_retries    	    	INT UNSIGNED"); //  8
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cacheable_parsing  	INT UNSIGNED"); //  9
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cacheable_binding  	INT UNSIGNED"); // 10
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cache_hits_parsing 	INT UNSIGNED"); // 11
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cache_hits_binding 	INT UNSIGNED"); // 12
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cacheable_too_large	INT UNSIGNED"); // 13
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_displaced  		INT UNSIGNED"); // 14
  strcpy(&((format++)->COLUMN_DEF[0]), "Optimization_level    	CHAR(10) character set iso88591");	// 15
  strcpy(&((format++)->COLUMN_DEF[0]), "Text_cache_hits INT UNSIGNED"); // 16
  strcpy(&((format++)->COLUMN_DEF[0]), "Avg_text_size        	INT UNSIGNED"); //  17
  strcpy(&((format++)->COLUMN_DEF[0]), "Text_entries   	    	INT UNSIGNED"); //  18
  strcpy(&((format++)->COLUMN_DEF[0]), "Displaced_texts 		INT UNSIGNED"); //  19
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_lookups        	INT UNSIGNED"); //  20
  return SP_SUCCESS;
}

// Copies information on the query cache by the interface provided in
// QueryCache.h and processes it
SP_STATUS
  QueryCacheStatStoredProcedure::sp_Process(SP_PROCESS_ACTION action,
					    SP_ROW_DATA inputData,
					    SP_EXTRACT_FUNCPTR eFunc,
					    SP_ROW_DATA outputData,
					    SP_FORMAT_FUNCPTR fFunc,
					    SP_KEY_VALUE keys,
					    SP_KEYVALUE_FUNCPTR kFunc,
					    SP_PROCESS_HANDLE *spProcHandle,
					    SP_HANDLE spObj,
					    SP_ERROR_STRUCT *error)
{
  struct InfoStruct
  {
    ULong counter;
  };

  if (action == SP_PROC_OPEN) {
    InfoStruct *is = new InfoStruct;
    is->counter = 0;
    *spProcHandle = is;
    return SP_SUCCESS;
  }

  if (action == SP_PROC_FETCH) {
    // do nothing if caching is off
    if (!CURRENTQCACHE->isCachingOn()) { // caching is off
      return SP_SUCCESS; // we're done!
    }
    InfoStruct* is = (InfoStruct *)(*spProcHandle);
    if (is) {
      if (is->counter > 0) {
        return SP_SUCCESS;
      }
      is->counter++;
    }
    else {
      return SP_FAIL;
    }

    QueryCacheStats stats;
    CURRENTQCACHE->getCacheStats(stats);

    ULong kBytes;

    kBytes = stats.avgPlanSize;
    fFunc(0,  outputData, sizeof(kBytes), &(kBytes), 0);
    kBytes = stats.s.currentSize / 1024;
    fFunc(1,  outputData, sizeof(kBytes), &(kBytes), 0);
    kBytes = stats.maxSize / 1024;
    fFunc(2,  outputData, sizeof(kBytes), &(kBytes), 0);
    fFunc(3,  outputData, sizeof(stats.maxVictims),	&(stats.maxVictims),  0);
    fFunc(4,  outputData, sizeof(stats.nEntries),	&(stats.nEntries),    0);
    fFunc(5,  outputData, sizeof(stats.nPlans),		&(stats.nPlans),    0);
    fFunc(6,  outputData, sizeof(stats.nCompiles),	&(stats.nCompiles),   0);
    fFunc(7,  outputData, sizeof(stats.s.nRecompiles),	&(stats.s.nRecompiles), 0);
    fFunc(8,  outputData, sizeof(stats.nRetries),	&(stats.nRetries),    0);
    fFunc(9,  outputData, sizeof(stats.s.nCacheableP),	&(stats.s.nCacheableP), 0);
    fFunc(10, outputData, sizeof(stats.s.nCacheableB),	&(stats.s.nCacheableB), 0);
    fFunc(11, outputData, sizeof(stats.s.nCacheHitsP),	&(stats.s.nCacheHitsP), 0);
    fFunc(12, outputData, sizeof(stats.s.nCacheHitsB),	&(stats.s.nCacheHitsB), 0);
    fFunc(13, outputData, sizeof(stats.nTooLarge),	&(stats.nTooLarge),   0);
    fFunc(14, outputData, sizeof(stats.nDisplaced),	&(stats.nDisplaced),  0);
    char optimizationLevel[8];
    switch (stats.optimLvl) {
      case CompilerEnv::OPT_MINIMUM:
	strcpy(optimizationLevel, "0      ");	  break;
      case CompilerEnv::OPT_MEDIUM_LOW:
	strcpy(optimizationLevel, "2      ");	  break;
      case CompilerEnv::OPT_MEDIUM:
	strcpy(optimizationLevel, "3      ");	  break;
      case CompilerEnv::OPT_MAXIMUM:
	strcpy(optimizationLevel, "5      ");	  break;
	default: strcpy(optimizationLevel, "UNKNOWN"); break;
    }

    fFunc(15,outputData,(Lng32)strlen(optimizationLevel),optimizationLevel,1);
    fFunc(16,outputData,sizeof(stats.s.nCacheHitsPP),&(stats.s.nCacheHitsPP),0);
    fFunc(17,outputData,sizeof(stats.avgTEntSize),&(stats.avgTEntSize),0);
    fFunc(18,outputData,sizeof(stats.nTextEntries),&(stats.nTextEntries),0);
    fFunc(19,outputData,sizeof(stats.nDispTEnts),&(stats.nDispTEnts),0);
    fFunc(20,outputData,sizeof(stats.s.nLookups),&(stats.s.nLookups),0);

    return SP_MOREDATA;
  } // if (action == SP_PROC_FETCH)

  if (action == SP_PROC_CLOSE) {
    delete (InfoStruct *)(*spProcHandle);
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

// Registers the QueryCache function
void QueryCacheStatStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("QUERYCACHE",
           sp_Compile,
           sp_InputFormat,
	   0,
           sp_NumOutputFields,
           sp_OutputFormat,
	   sp_Process,
	   0,
	   CMPISPVERSION);
}

SP_STATUS
  QueryCacheEntriesStoredProcedure::sp_NumOutputFields(Lng32 *numFields,
						       SP_COMPILE_HANDLE spCompileObj,
						       SP_HANDLE spObj,
						       SP_ERROR_STRUCT *error)
{
  *numFields = 20;
  return SP_SUCCESS;
}

// Specifies the columns of the QueryCacheEntries table and their types
SP_STATUS QueryCacheEntriesStoredProcedure::sp_OutputFormat(
						SP_FIELDDESC_STRUCT* format,
						SP_KEYDESC_STRUCT*  /*keyFields */,
						Lng32*  /*numKeyFields */,
						SP_COMPILE_HANDLE cmpHandle,
						SP_HANDLE /* spHandle */,
						SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "Row_id        		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Plan_id     		LARGEINT");
  strcpy(&((format++)->COLUMN_DEF[0]), "Text  			CHAR(1024) character set utf8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Entry_size  		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Plan_length  		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_hits        	INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Phase        		CHAR(10) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "Optimization_level	CHAR(10) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "Catalog_name 		CHAR(40) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "Schema_name 		CHAR(40) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_params   		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Param_types   		CHAR(1024) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "Compilation_time	INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Average_hit_time	INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Shape    		CHAR(1024) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "Isolation_level    	CHAR(20) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "Isolation_level_For_Updates   	CHAR(20) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "Access_mode     	CHAR(20) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "Auto_commit     	CHAR(15) character set iso88591");
  strcpy(&((format++)->COLUMN_DEF[0]), "Rollback_mode     	CHAR(15) character set iso88591");
  return SP_SUCCESS;
}

void
setIsolationLevelAsString(TransMode::IsolationLevel il,
                          char* buffer, Int32 bufLen)
{
   memset(buffer, ' ', bufLen);
   strcpy(buffer, getStrOfIsolationLevel(il, FALSE));
}

// Copies information on the query cache by the interface provided in
// QueryCache.h and processes it
SP_STATUS QueryCacheEntriesStoredProcedure::sp_Process(SP_PROCESS_ACTION action,
						       SP_ROW_DATA  inputData ,
						       SP_EXTRACT_FUNCPTR  eFunc ,
						       SP_ROW_DATA outputData ,
						       SP_FORMAT_FUNCPTR fFunc,
						       SP_KEY_VALUE,
						       SP_KEYVALUE_FUNCPTR,
						       SP_PROCESS_HANDLE* spProcHandle,
						       SP_HANDLE /* spHandle */,
						       SP_ERROR_STRUCT*  error )
{
  struct InfoStruct
  {
    ULong counter;
    LRUList::iterator iter;
    InfoStruct() : counter(0)
    { iter = CURRENTQCACHE->beginPre(); } // start with preparser cache entries
  };

  InfoStruct *is;

  switch (action) {
  case SP_PROC_OPEN:
    // No inputs to process
    *spProcHandle = is = new InfoStruct;
    return SP_SUCCESS;
    break;

  case SP_PROC_FETCH:
    {
      // guard against an empty cache
      if (CURRENTQCACHE->empty()) { // no entries
        return SP_SUCCESS; // we're done!
      }
      // No more data to be returned
      is = (InfoStruct *)(*spProcHandle);
      if (!is) {
        return SP_FAIL;
      }
      if (is->iter == CURRENTQCACHE->end()) {
        // end of postparser cache entries
        return SP_SUCCESS; // we're done!
      }

      if (is->iter == CURRENTQCACHE->endPre()) {
        // end of preparser cache, continue with postparser entries
        is->iter = CURRENTQCACHE->begin();
      }

      QueryCacheDetails details;
      CURRENTQCACHE->getEntryDetails(is->iter++, details);

      fFunc(0,  outputData, sizeof(is->counter),	&(is->counter), 0);
      fFunc(1,  outputData, sizeof(details.planId),	&(details.planId), 0);
      fFunc(2,outputData,(Lng32)strlen(details.qryTxt),(void*)(details.qryTxt),1);
      fFunc(3,  outputData, sizeof(details.entrySize),	&(details.entrySize), 0);
      fFunc(4,  outputData, sizeof(details.planLength), &(details.planLength), 0);
      fFunc(5,  outputData, sizeof(details.nOfHits),	&(details.nOfHits), 0);

      char phase[11];

      switch (details.phase) {
      case CmpMain::PREPARSE:  strcpy(phase, "PREPARSING"); break;
      case CmpMain::PARSE:  strcpy(phase, "PARSING   "); break;
      case CmpMain::BIND:  strcpy(phase, "BINDING   "); break;
      default: strcpy(phase, "UNKNOWN   "); break;
      }
      fFunc(6,outputData,(Lng32)strlen(phase),phase,1);

      char optimizationLevel[8];

      switch (details.optLvl) {
      case CompilerEnv::OPT_MINIMUM:    strcpy(optimizationLevel, "0      ");	break;
      case CompilerEnv::OPT_MEDIUM_LOW: strcpy(optimizationLevel, "2      ");	break;
      case CompilerEnv::OPT_MEDIUM:     strcpy(optimizationLevel, "3      ");	break;
      case CompilerEnv::OPT_MAXIMUM:    strcpy(optimizationLevel, "5      ");	break;
      default: strcpy(optimizationLevel,"UNKNOWN");				break;
	  }
      fFunc(7,outputData,(Lng32)strlen(optimizationLevel),optimizationLevel,1);
      fFunc(8,outputData,(Lng32)strlen(details.catalog),(void*)(details.catalog),1);
      fFunc(9,outputData,(Lng32)strlen(details.schema),(void*)(details.schema),1);
      fFunc(10, outputData, sizeof(details.nParams), &(details.nParams), 0);
      fFunc(11,outputData,(Lng32)strlen(details.paramTypes),(void*)(details.paramTypes),1);

      ULong time = details.compTime / 1000;
      fFunc(12, outputData, sizeof(time), &(time), 0);
      time = details.avgHitTime / 1000;
      fFunc(13, outputData, sizeof(time), &(time), 0);
      fFunc(14,outputData,(Lng32)strlen(details.reqdShape),(void*)(details.reqdShape),1);

      char isolationLevel[18];

      setIsolationLevelAsString(details.isoLvl, isolationLevel, 18);
      fFunc(15,outputData,(Lng32)strlen(isolationLevel),isolationLevel,1);

      setIsolationLevelAsString(details.isoLvlForUpdates, isolationLevel, 18);
      fFunc(16,outputData,(Lng32)strlen(isolationLevel),isolationLevel,1);

      char accessMode[14];
      switch(details.accMode) {
      case TransMode::READ_ONLY_:
      case TransMode::READ_ONLY_SPECIFIED_BY_USER_:
        strcpy(accessMode, "READ ONLY    ");	break;
      case TransMode::READ_WRITE_:
        strcpy(accessMode, "READ/WRITE   ");	break;
      default:
        strcpy(accessMode, "NOT SPECIFIED");	break;
      }
      fFunc(17,outputData,(Lng32)strlen(accessMode),accessMode,1);

      char autoCommit[14];
      switch(details.autoCmt) {
      case TransMode::ON_:
        strcpy(autoCommit, "ON           ");	break;
      case TransMode::OFF_:
        strcpy(autoCommit, "OFF          ");	break;
      default:
        strcpy(autoCommit, "NOT SPECIFIED");	break;
      }
      fFunc(18,outputData,(Lng32)strlen(autoCommit),autoCommit,1);

      char rollBackMode[14];
      switch(details.rbackMode) {
      case TransMode::ROLLBACK_MODE_WAITED_:
        strcpy(rollBackMode, "WAITED       ");	break;
      case TransMode::ROLLBACK_MODE_NOWAITED_:
        strcpy(rollBackMode, "NO WAITED    ");	break;
      default:
        strcpy(rollBackMode, "NOT SPECIFIED");	break;
	  }
      fFunc(19,outputData,(Lng32)strlen(rollBackMode),rollBackMode,1);

      is->counter++;

      return SP_MOREDATA;
    }
    break;

  case SP_PROC_CLOSE: {
    delete (InfoStruct *)(*spProcHandle);
    return SP_SUCCESS;
  }

  default: break;
  } // switch
  return SP_SUCCESS;
}

// Registers the QueryCache function
void QueryCacheEntriesStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("QUERYCACHEENTRIES",
           sp_Compile,
           sp_InputFormat,
	   0,
           sp_NumOutputFields,
           sp_OutputFormat,
	   sp_Process,
	   0,
	   CMPISPVERSION);
}


//-----------------------------------------------------------------------
// QueryCacheDeleteStoredProcedure is a class that contains functions used
// to delete the contents of the  QueryCache virtual table. The delete 
// function is implemented as an internal stored procedure.
//-----------------------------------------------------------------------


SP_STATUS QueryCacheDeleteStoredProcedure::sp_Process(
  SP_PROCESS_ACTION action,
  SP_ROW_DATA inputData,
  SP_EXTRACT_FUNCPTR eFunc,
  SP_ROW_DATA outputData,
  SP_FORMAT_FUNCPTR fFunc,
  SP_KEY_VALUE keys,
  SP_KEYVALUE_FUNCPTR kFunc,
  SP_PROCESS_HANDLE *spProcHandle,
  SP_HANDLE spObj,
  SP_ERROR_STRUCT *error)
{
  struct InfoStruct
  {
    ULng32 counter;
  };

  SP_STATUS status = SP_SUCCESS;
  InfoStruct *is = NULL;

  switch (action)
  {
  case SP_PROC_OPEN:
    // No inputs to process
    is = new InfoStruct;
    is->counter = 0;
    *spProcHandle = is;
    break;

  case SP_PROC_FETCH:
    is = (InfoStruct*)(*spProcHandle);
    if (is == NULL )
    {
      status = SP_FAIL;
      break;
    }

    //clear out QueryCache
    CURRENTQCACHE->makeEmpty();
    break;

  case SP_PROC_CLOSE:
    delete (InfoStruct*) (*spProcHandle);
    break;
  }
  return status;
}

void QueryCacheDeleteStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("QUERYCACHEDELETE",
          sp_Compile,
          sp_InputFormat,
          0,
          sp_NumOutputFields,
          sp_OutputFormat,
          sp_Process,
          0,
	  CMPISPVERSION);
}


