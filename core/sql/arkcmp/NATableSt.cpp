/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
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
/* #include "CmpMain.h" */

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
  struct InfoStruct
  {
    ULong counter;
    Int32 iter;
    InfoStruct() : counter(0)
    { iter = NATableDB::begin(); }
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
      if ( static_cast<NABoolean>(NATableDB::empty()) ) { // no entries
        return SP_SUCCESS; // we're done!
      }
      // No more data to be returned
      is = (InfoStruct *)(*spProcHandle);
      if (!is) {
        return SP_FAIL;
      }
      if (is->iter == static_cast<Int32>(NATableDB::end())) {
        return SP_SUCCESS; // we're done!
      }

      NATableEntryDetails details;
      static_cast<void>(NATableDB::getEntryDetails(is->iter++, details));

      fFunc(0,outputData, sizeof(is->counter), &(is->counter), 0);
      fFunc(1,outputData,(Lng32)strlen(details.catalog),(void*)(details.catalog),1);
      fFunc(2,outputData,(Lng32)strlen(details.schema),(void*)(details.schema),1);
      fFunc(3,outputData,(Lng32)strlen(details.object),(void*)(details.object),1);

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

