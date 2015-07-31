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
//********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         NATableSt.cpp
 * Description:
 *
 * Created:      2/06/2012
 * Language:     C++
 *
 *****************************************************************************
 */

#include "NATable.h"
#include "NATableSt.h"
#include "SchemaDB.h"
#include "CmpErrors.h"
#include "CmpMain.h"
#include "Globals.h"
#include "Context.h"

//-----------------------------------------------------------------------
// NATableCacheStoredProcedure is a class that contains functions used by
// the NATableCache virtual table, whose purpose is to serve as an interface
// to the SQL/MX NATable cache statistics. This table is implemented as
// an internal stored procedure.
//-----------------------------------------------------------------------

SP_STATUS NATableCacheStatStoredProcedure::sp_InputFormat(SP_FIELDDESC_STRUCT *inputFieldFormat,
				  Lng32 numFields,
				  SP_COMPILE_HANDLE spCompileObj,
				  SP_HANDLE spObj,
	    			  SP_ERROR_STRUCT *error)
{
      if ( numFields != 2 )
      {
        //accepts 2 input columns
        error->error = arkcmpErrorISPWrongInputNum;
        strcpy(error->optionalString[0], "NATableCache");
        error->optionalInteger[0] = 2;
        return SP_FAIL;
      }
  
     //column as input parameter for ISP, specifiy cache of metadata or user context
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "instance char(16)  not null");
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "location char(16)  not null"); 
     return SP_SUCCESS;
}

const Lng32 NUM_OF_OUTPUT = 6;

SP_STATUS NATableCacheStatStoredProcedure::sp_NumOutputFields(
  Lng32 *numFields,
  SP_COMPILE_HANDLE spCompileObj,
  SP_HANDLE spObj,
  SP_ERROR_STRUCT *error)
{
  *numFields = NUM_OF_OUTPUT;
  return SP_SUCCESS;
}

SP_STATUS NATableCacheStatStoredProcedure::sp_OutputFormat(
  SP_FIELDDESC_STRUCT *format,
  SP_KEYDESC_STRUCT keyFields[],
  Lng32 *numKeyFields,
  SP_HANDLE spCompileObj,
  SP_HANDLE spObj,
  SP_ERROR_STRUCT *error)
{
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_lookups      INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_cache_hits   INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Num_entries   INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Current_cache_size   INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "High_watermark   INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Max_cache_size   INT UNSIGNED");

  return SP_SUCCESS;
}

SP_STATUS NATableCacheStatStoredProcedure::sp_Process(
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
  if (action == SP_PROC_OPEN) {
  
    NATableCacheStatsISPIterator * it = new (GetCliGlobals()->exCollHeap()) 
      NATableCacheStatsISPIterator(inputData, eFunc, error,
                                   GetCliGlobals()->currContext()->getCmpContextInfo(), 
                                   GetCliGlobals()->exCollHeap());
    *spProcHandle = it;
    return SP_SUCCESS;
  }

  if (action == SP_PROC_FETCH) {
    NATableCacheStatsISPIterator* it = (NATableCacheStatsISPIterator *)(*spProcHandle);

    if (!it) {
      return SP_FAIL;
    }

    NATableCacheStats stats;
    if(!it->getNext(stats))
       return SP_SUCCESS;

    fFunc(0, outputData, sizeof(ULng32), &(stats.numLookups), 0);
    fFunc(1, outputData, sizeof(ULng32), &(stats.numCacheHits), 0);
    fFunc(2, outputData, sizeof(ULng32), &(stats.numEntries), 0);
    fFunc(3, outputData, sizeof(ULng32), &(stats.currentCacheSize), 0);
    fFunc(4, outputData, sizeof(ULng32), &(stats.highWaterMark), 0);
    fFunc(5, outputData, sizeof(ULng32), &(stats.maxCacheSize), 0);
    return SP_MOREDATA;
  }

  
  if (action == SP_PROC_CLOSE) {
    if (*spProcHandle)
      NADELETEBASIC((NATableCacheStatsISPIterator *)(*spProcHandle), GetCliGlobals()->exCollHeap());
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

void NATableCacheStatStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("NATABLECACHE",
          sp_Compile,
          sp_InputFormat,
          0,
          sp_NumOutputFields,
          sp_OutputFormat,
          sp_Process,
          0,
	  CMPISPVERSION);
}


SP_STATUS NATableCacheEntriesStoredProcedure::sp_InputFormat(SP_FIELDDESC_STRUCT *inputFieldFormat,
				  Lng32 numFields,
				  SP_COMPILE_HANDLE spCompileObj,
				  SP_HANDLE spObj,
	    			  SP_ERROR_STRUCT *error)
{
      if ( numFields != 2 )
      {
        //accepts 2 input columns
        error->error = arkcmpErrorISPWrongInputNum;
        strcpy(error->optionalString[0], "NATableCacheEntries");
        error->optionalInteger[0] = 2;
        return SP_FAIL;
      }
  
     //column as input parameter for ISP, specifiy cache of metadata or user context
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "instance char(16)  not null");
     strcpy(&((inputFieldFormat++)->COLUMN_DEF[0]), "location char(16)  not null"); 
     return SP_SUCCESS;
}


SP_STATUS
  NATableCacheEntriesStoredProcedure::sp_NumOutputFields(Lng32 *numFields,
						       SP_COMPILE_HANDLE spCompileObj,
						       SP_HANDLE spObj,
						       SP_ERROR_STRUCT *error)
{
  *numFields = 4;
  return SP_SUCCESS;
}

// Specifies the columns of the NATableEntries table and their types
SP_STATUS NATableCacheEntriesStoredProcedure::sp_OutputFormat(
						SP_FIELDDESC_STRUCT* format,
						SP_KEYDESC_STRUCT*  /*keyFields */,
						Lng32*  /*numKeyFields */,
						SP_COMPILE_HANDLE cmpHandle,
						SP_HANDLE /* spHandle */,
						SP_ERROR_STRUCT* /* error */  )
{
  strcpy(&((format++)->COLUMN_DEF[0]), "Row_id        		INT UNSIGNED");
  strcpy(&((format++)->COLUMN_DEF[0]), "Catalog_name 		VARCHAR(128) character set UTF8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Schema_name 		VARCHAR(128) character set UTF8");
  strcpy(&((format++)->COLUMN_DEF[0]), "Object_name 		VARCHAR(128) character set UTF8");
  return SP_SUCCESS;
}

// Copies information on the NATable cache by the interface provided in
// NATable.h and processes it
SP_STATUS NATableCacheEntriesStoredProcedure::sp_Process(SP_PROCESS_ACTION action,
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

  if (action == SP_PROC_OPEN) {
  
    NATableCacheEntriesISPIterator * it = new (GetCliGlobals()->exCollHeap()) 
      NATableCacheEntriesISPIterator(inputData, eFunc, error,
                                     GetCliGlobals()->currContext()->getCmpContextInfo(), 
                                     GetCliGlobals()->exCollHeap());
    *spProcHandle = it;
    return SP_SUCCESS;
  }

  if (action == SP_PROC_FETCH) {
    NATableCacheEntriesISPIterator* it = (NATableCacheEntriesISPIterator *)(*spProcHandle);

    if (!it) {
      return SP_FAIL;
    }

    NATableEntryDetails details;
    if(!it->getNext(details))
       return SP_SUCCESS;

    fFunc(0,outputData, sizeof(it->rowid()), &(it->rowid()), 0);
    fFunc(1,outputData,(Lng32)strlen(details.catalog),(void*)(details.catalog),1);
    fFunc(2,outputData,(Lng32)strlen(details.schema),(void*)(details.schema),1);
    fFunc(3,outputData,(Lng32)strlen(details.object),(void*)(details.object),1);

    return SP_MOREDATA;
  }

  
  if (action == SP_PROC_CLOSE) {
    if (*spProcHandle)
      NADELETEBASIC((NATableCacheEntriesISPIterator *)(*spProcHandle), GetCliGlobals()->exCollHeap());
    return SP_SUCCESS;
  }

  return SP_SUCCESS;
}

// Registers the NATableEntries function
void NATableCacheEntriesStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("NATABLECACHEENTRIES",
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
// NATableCacheDeleteStoredProcedure is a class that contains functions used
// to delete the contents of the  NATableCache virtual table. The delete 
// function is implemented as an internal stored procedure.
//-----------------------------------------------------------------------


SP_STATUS NATableCacheDeleteStoredProcedure::sp_Process(
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
  SP_STATUS status = SP_SUCCESS;
  NATableDB * tableDB = NULL;

  switch (action)
  {
  case SP_PROC_OPEN:
    // No inputs to process
    break;

  case SP_PROC_FETCH:
    tableDB = ActiveSchemaDB()->getNATableDB();

    //clear out NATableCache
    tableDB->setCachingOFF();
    tableDB->setCachingON();
    break;

  case SP_PROC_CLOSE:
    break;
  }
  return status;
}

void NATableCacheDeleteStoredProcedure::Initialize(SP_REGISTER_FUNCPTR regFunc)
{
  regFunc("NATABLECACHEDELETE",
          sp_Compile,
          sp_InputFormat,
          0,
          sp_NumOutputFields,
          sp_OutputFormat,
          sp_Process,
          0,
	  CMPISPVERSION);
}


NATableCacheStatsISPIterator::NATableCacheStatsISPIterator(
     SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
     SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, 
     CollHeap * h)
: ISPIterator(ctxs, h)
{
    initializeISPCaches(inputData, eFunc, error, ctxs, 
                        contextName_, currCacheIndex_);
}


NABoolean NATableCacheStatsISPIterator::getNext(NATableCacheStats & stats)
{
   //Only for remote tdm_arkcmp with 0 context
   if(currCacheIndex_ == -1)
   {
     ActiveSchemaDB()->getNATableDB()->getCacheStats(stats);
     currCacheIndex_ = -2;
     return TRUE;
   }
   
   //fetch QueryCaches of all CmpContexts with name equal to contextName_
   if(currCacheIndex_ > -1 && currCacheIndex_ < ctxInfos_.entries())
   {
      if( !ctxInfos_[currCacheIndex_]->isSameClass(contextName_.data()) //current context name is not equal to contextName_
       && contextName_.compareTo("ALL", NAString::exact)!=0) //and contextName_ is not "ALL"
      {// go to next context in ctxInfos_
          currCacheIndex_++;
          return getNext(stats);
      }
      ctxInfos_[currCacheIndex_++]->getCmpContext()->getSchemaDB()->getNATableDB()->getCacheStats(stats);
      return TRUE;
   }
   //all entries of all caches are fetched, we are done!
   return FALSE;
}


NATableCacheEntriesISPIterator::NATableCacheEntriesISPIterator(
     SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
     SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, 
     CollHeap * h)
: ISPIterator(ctxs, h)
{
    initializeISPCaches(inputData, eFunc, error, ctxs, 
                        contextName_, currCacheIndex_);
    counter_ = 0;
    rowid_ = 0;
}


NABoolean NATableCacheEntriesISPIterator::getNext(NATableEntryDetails & details)
{
  NATableDB* currNATableDB ;
   //Only for remote tdm_arkcmp with 0 context
   if(currCacheIndex_ == -1)
   {
     currNATableDB = ActiveSchemaDB()->getNATableDB();
     if (currNATableDB->empty() || currNATableDB->end() == counter_)
       return FALSE;
     currNATableDB->getEntryDetails(counter_, details);
     counter_++;
     rowid_++;
     return TRUE;
   }
   
   //fetch QueryCaches of all CmpContexts with name equal to contextName_
   while(currCacheIndex_ > -1 && currCacheIndex_ < ctxInfos_.entries())
   {
      if( !ctxInfos_[currCacheIndex_]->isSameClass(contextName_.data()) //current context name is not equal to contextName_
       && contextName_.compareTo("ALL", NAString::exact)!=0) //and contextName_ is not "ALL"
      {// go to next context in ctxInfos_
          currCacheIndex_++;
          return getNext(details);
      }
      currNATableDB = ctxInfos_[currCacheIndex_]->getCmpContext()->getSchemaDB()->getNATableDB();
      if (currNATableDB->empty() || currNATableDB->end() == counter_)
      {
        currCacheIndex_++;
        counter_ = 0;
        continue;
      }
      currNATableDB->getEntryDetails(counter_, details);
      counter_++;
      rowid_++;
      return TRUE;
   }
   //all entries of all caches are fetched, we are done!
   return FALSE;
}

