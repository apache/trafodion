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
 * File:         CmpISPStd.h
 * Description:  This file contains the standard structure declaration/
 *               interface definition for the internal developer to use
 *               the internal stored procedure feature provided by
 *               executor/arkcmp.
 *
 *
 * Created:      03/14/97
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#ifndef CMPISPSTD__H
#define CMPISPSTD__H

#include "Platform.h"
#include "ComSizeDefs.h"
// include the internal stored procedure implementation provided by SQL/Util group
#if defined (_SPUTIL_SP_DLL_)  // used in sqluti interface routine.
#define ARKLIBAPI _declspec(dllexport)
#else
#undef ARKLIBAPI
#define ARKLIBAPI _declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/* the version of interface routines */
#define CMPISPVERSION "V970821"

/* The return status definition for the interface functions from ISP.
   All the interface fucntions should return status,
   the exception handling should be done inside the internal
   stored procedures. ARKCMP ( the server ) is not expecting any
   longjmp or exception. */

enum SP_STATUS
  {
    SP_FAIL = 0,
    SP_MOREDATA = 1,
    SP_SUCCESS = 2,
    SP_SUCCESS_WARNING = 3
  };

/* Error status returned from the helper routines provided by arkcmp(the server)  */

enum SP_HELPER_STATUS
  {
    SP_NO_ERROR = 0,
    SP_NOT_SUPPORTED_YET = 1,
    SP_ERROR_INVALIDFIELDNO = 100,
    SP_ERROR_EXTRACT_DATA = 101,
    SP_ERROR_FORMAT_DATA = 102
  };

/* Error code, in the case of error, the error structure ( defined later )
   should be filled with certain error code. This error code is the
   index to the error files maintained in the sqlark environment,
   currently bin/SqlciErrors.txt */

typedef Lng32 SP_ERRORCODE;

/* SP_DLL_HANDLE is a handle passed in and out for the SQLISP_INIT and
   SQLISP_EXIT. This is used for as a handle to the objects instantiated
   inside SQLISP_INIT. It will then be passed back when calling
   SQLISP_EXIT for cleanup. */

typedef void* SP_DLL_HANDLE;

/* SP_HANDLE is a handle registered by the internal stored procedures in DLL
   initialization time, the value will be passed into every interface
   routine. This SP_HANDLE should not contain state information or data
   passed from compilation time to execution time, since arkcmp might
   die then get restarted or compilation and execution are done in
   different processes. */

typedef void* SP_HANDLE;

/* SP_COMPILE_HANDLE is used in the interface routines called in query
   compilation time. It can be used to store the information carried
   between interface routines for compilation */

typedef void* SP_COMPILE_HANDLE;

/* SP_PROCESS_HANDLE is used in SP_xxx_PROCESS interface routine. It can be used
   for the stored procedure to keep the execution state. */

typedef void* SP_PROCESS_HANDLE;

/* The action flag for processing data */

enum SP_PROCESS_ACTION
  { SP_PROC_OPEN = 1,
    SP_PROC_FETCH = 2,
    SP_PROC_CLOSE = 3 };

/* The action flag for SP_xxx_COMPILE interface routine. */

enum SP_COMPILE_ACTION
  { SP_COMP_INIT = 1,
    SP_COMP_EXIT = 2 };

/* The structure for virtual table format description, it describes
   the format for each field */

#define SP_STRING_MAX_LENGTH 1024
#define SP_MAX_ERROR_STRUCTS 5

struct SP_FIELDDESC_STRUCT
{
  /* column definition, null terminated. The format is
     the same as in create table DDL commands, e.g.
     'C1 int not null' */
  char COLUMN_DEF[SP_STRING_MAX_LENGTH];
};

/* The structure to sepcify key information for output table */

enum SP_KEY_ORDER { SP_KEY_ASCEND=0, SP_KEY_DESCEND };

struct SP_KEYDESC_STRUCT
{
  char COLUMN_NAME[ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1]; /* name of the key column */
  SP_KEY_ORDER ORDER; /* SP_KEY_ASCEND or SP_KEY_DESCEND */
};

/* input/output data for processing */
typedef void* SP_ROW_DATA;

/* key values for processing routine */
typedef void* SP_KEY_VALUE;

/* The helper routines provided by arkcmp for data manipulation,
   including extract data, format data and retrieve key values */

/* The helper function to extract fields from input row :
   In the SP_EXTRACT_FUNCPTR helper routine, the contents of the fieldNo'th
   field ( starting from 0 ) will be copied into fieldData. fieldData is
   expected to be in fieldLen long, the caller should make sure it is big enough
   to hold the contents of the data. Otherwise, only fieldLen bytes are copied.
   If in the previous INPUT_FORMAT routine, the casting flag is set 1, the
   fieldData returned will be in sql varchar format. Otherwise, it will be in
   the sql internal format. */

typedef SP_HELPER_STATUS (*SP_EXTRACT_FUNCPTR)
     (
      Lng32 fieldNo, /* input, index to the field to be extracted */
      SP_ROW_DATA rowData, /* input, data passed into SP_PROCESS_FUNCPTR */
      Lng32 fieldLen, /* input, the length of the field */
      void* fieldData, /* input/output, the contents of the data */
      Lng32 casting /* input, if 1, fieldData is in varchar format,
                      the helper routine will convert it into internal format.
                      if 0, fieldData is in sql internal format. */
      );

/* The helper function to format output row :
   In this routine, the fieldData ( in caller's space ) should contain the
   contents of the data ( with size fieldLen ) to be copied into rowData.
   If the outputCasting flag is set to 1 in the previous OUTPUT_FORMAT function,
   fieldData is expected to be in sql varchar format, SP_FORMAT_FUNCPTR will
   then convert it into sql internal format. If not set, fieldData is expected
   to be in sql internal format.  */

typedef SP_HELPER_STATUS (*SP_FORMAT_FUNCPTR)
     (
      Lng32 fieldNo, /* input, index to the field to put the data in */
      SP_ROW_DATA rowData, /* input, data passed into SP_PROCESS_FUNCPTR */
      Lng32 fieldLen, /* input, length of the data */
      void* fieldData, /* input, contents of the data */
      Lng32 casting /* input, if 1, fieldData is in varchar format,
                      the helper routine will convert it into internal format.
                      if 0, fieldData is in sql internal format. */
      );

/* The helper function to retrieve the key values for ouput virtual tables.
   The keyLength and keyValue are returned as in SP_EXTRACT_FUNCPTR */

typedef SP_HELPER_STATUS (*SP_KEYVALUE_FUNCPTR)
    (
     Lng32 keyIndex, /* input, key index starts from 0 */
     SP_KEY_VALUE key, /* input, key passed in SP_PROCESS_FUNCPTR */
     Lng32 keyLength, /* input, the length of the key field */
     void* keyValue, /* input/output the contents of the key field */
     Lng32 casting /* input, if 1, fieldData is in varchar format,
                     the helper routine will convert it into internal format.
                     if 0, fieldData is in sql internal format. */
    );

/* The structure for error handling */

#define SP_ERROR_MAX_OPTIONAL_STRINGS 5
#define SP_ERROR_MAX_OPTIONAL_INTS 5
struct SP_ERROR_STRUCT
{
  SP_ERRORCODE error; /* error code, index into bin/SqlciErrors.txt */
  char* optionalString[SP_ERROR_MAX_OPTIONAL_STRINGS];
  /* $string0, $string1, .. $string4 to be formatted
     into the error message text. */
  Int32 optionalInteger[SP_ERROR_MAX_OPTIONAL_INTS];
};

/* The interface routines between arkcmp and the user defined internal
   stored procedure.

   At SQL compilation time, the following routines are called for ISP
   (*SP_COMPILE_FUNCPTR)(SP_COMP_INIT....);
   (*SP_INPUTFORMAT_FUNCPTR)(.......);
   (*SP_PARSE_FUNCPTR)(.....);     ( an opional function )
   (*SP_NUMOUTPUTFIELDS_FUNCPTR)(....);
   (*SP_OUTPUTFORMAT_FUNCPTR)(....);
   (*SP_COMPILE_FUNCPTR)(SP_COMP_EXIT...);
   */

/* Function to start/end the compilation for each SP statement, this
   function will be called 2 times, one for SP_COMP_INIT and one for
   SP_COMP_EXIT even the in between compilation interface routines failed. */

typedef SP_STATUS (*SP_COMPILE_FUNCPTR)
     (
      SP_COMPILE_ACTION action, /* input */
      SP_COMPILE_HANDLE* cmpHandle, /* output */
      SP_HANDLE spHandle, /* input */
      SP_ERROR_STRUCT* error /* output */
     );

/* Function to retrieve the input data format */

typedef SP_STATUS (*SP_INPUTFORMAT_FUNCPTR)
     (
      SP_FIELDDESC_STRUCT *inputFiledFormat, /* input/output */
      Lng32 numFields, /* input */
      SP_COMPILE_HANDLE compHandle, /* input */
      SP_HANDLE spHandle, /* input */
      SP_ERROR_STRUCT* error /* output */
     );

/* Function to parse the first input parameter.
   This is an optional function to call,
   only called if provided with a non 0 function pointer.
   The first input parameter has to be a literal string since the
   value is known at SQL compilation time. */

typedef SP_STATUS (*SP_PARSE_FUNCPTR)
    (
     char* param, /* first input parameter, null terminated string */
     SP_COMPILE_HANDLE cmpHandle, /* input */
     SP_HANDLE spHandle, /* input */
     SP_ERROR_STRUCT* error /* output */
    );

/* Function to retrieve the number of output fields */

typedef SP_STATUS (*SP_NUM_OUTPUTFIELDS_FUNCPTR)
     (
      Lng32* numFields, /* output */
      SP_COMPILE_HANDLE cmpHandle, /* input */
      SP_HANDLE spHandle, /* input */
      SP_ERROR_STRUCT* error /* output */
      );

/* Function to retrieve the output data format */

typedef SP_STATUS (*SP_OUTPUTFORMAT_FUNCPTR)
     (
      SP_FIELDDESC_STRUCT *outputFieldFormat, /* input/output */
      SP_KEYDESC_STRUCT* keyFields, /* output */
      Lng32* numKeyFields, /* output */
      SP_COMPILE_HANDLE cmpHandle, /* input */
      SP_HANDLE spHandle, /* input */
      SP_ERROR_STRUCT* error /* output */
      );

/* Function to process the input data and fetch output data.
   For each row of input data sent from executor, the following
   calling sequence occur :
   openStatus = SP_PROCESS ( SP_PROC_OPEN, ....);
   while ( (fetchStatus=SP_PROCESS(SP_PROC_FETCH....))== SP_MOREDATA )
     ...
   if ( openStatus != SP_ERROR )
      SP_PROCESS(SP_CLOSE....);
*/

typedef SP_STATUS (*SP_PROCESS_FUNCPTR)
     (
      SP_PROCESS_ACTION action, /* input */
      SP_ROW_DATA inputData, /* input */
      SP_EXTRACT_FUNCPTR eFunc, /* input */
      SP_ROW_DATA outputData, /* output */
      SP_FORMAT_FUNCPTR fFunc, /* input */
      SP_KEY_VALUE keyData, /* input */
      SP_KEYVALUE_FUNCPTR kFunc, /* input */
      SP_PROCESS_HANDLE *spProcHandle, /* input/output */
      SP_HANDLE spHandle,
      SP_ERROR_STRUCT* error /* output */
      );

/* The initialization routine to link in ISP implementation.
   This function will be called in the beginning of the main program
   to register the stored procedures linked into the server program which
   compiles and executes the built-in stored procedure querys. Currently
   ( 97 FCS release ) there will be only one set of built-in stored
   procedures provided by SQL/Util group. */

/* The callback routine that will be used for SQLISP_INIT routine to
   register the built-in stored procedure implementation. */
/* return 1 as SUCCESS, 0 as FAIL */
typedef Int32 (*SP_REGISTER_FUNCPTR)
  (
   const char* procName, /* null terminated */
   SP_COMPILE_FUNCPTR compileFunc,
   SP_INPUTFORMAT_FUNCPTR inFormatFunc,
   SP_PARSE_FUNCPTR parseFunc,
   SP_NUM_OUTPUTFIELDS_FUNCPTR outNumFormatFunc,
   SP_OUTPUTFORMAT_FUNCPTR outFormatFunc,
   SP_PROCESS_FUNCPTR procFunc,
   SP_HANDLE spHandle,
   const char* version  // Default is CMPISPVERSION
  );

/* The only entry points from arkcmp(the server) to ISP implementation */

/* return 1 as SUCCESS, 0 as FAIL */
ARKLIBAPI Int32 SQLISP_INIT( SP_REGISTER_FUNCPTR, SP_DLL_HANDLE* h=0 );
ARKLIBAPI Int32 SQLISP_EXIT( SP_DLL_HANDLE=0 );

#ifdef __cplusplus
}
#endif

#endif
