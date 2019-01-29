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
#ifndef EXP_LOB_EXTERNAL_H
#define EXP_LOB_EXTERNAL_H


/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExpLOBexternal.h
 * Description:  
 *               
 *               
 * Created:      2/12/2013
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#ifdef __cplusplus
/* use C linkage */
extern "C" {
#endif

enum LOBcliQueryType
  {
    LOB_CLI_INIT,

    // create the desc tables
    LOB_CLI_CREATE,

    // alter the MD table and create LOB desc tables
    LOB_CLI_ALTER,
    

    // drops the desc tables
    LOB_CLI_DROP,

    // cleanup LOBs. Cant use CLI_DROP as MD may be in an inconsistent state.
    LOB_CLI_CLEANUP,

    // inserts first/new chunk into the desc table
    LOB_CLI_INSERT,

    // inserts next chunk and appends to existing row
    LOB_CLI_INSERT_APPEND,

    // updates chunklen of the last chunk
    LOB_CLI_UPDATE_UNIQUE,

    // deletes all chunks
    LOB_CLI_DELETE,

    // deletes all rows and all chunks from the desc table
    LOB_CLI_PURGEDATA,

    // selects and returns data for the last chunk.
    LOB_CLI_SELECT_UNIQUE,

    // prepares cursor select of all chunks. Does not return a row.
    // use SELECT_FETCH to retrieve rows.
    LOB_CLI_SELECT_CURSOR,
    LOB_CLI_SELECT_OPEN = LOB_CLI_SELECT_CURSOR,

    // fetches the next chunk. Error 100 indicates EOD.
    LOB_CLI_SELECT_FETCH,

    LOB_CLI_SELECT_CLOSE,

    // loads data from input buffer
    LOB_DATA_LOAD,

    // returns data into output buffer
    LOB_DATA_EXTRACT,

    // returns length of lob given a lobhandle
    LOB_CLI_SELECT_LOBLENGTH,
   
    // performs GC of lob file
    LOB_CLI_PERFORM_LOB_GC,

    //returns beginning offset of a lob
    LOB_CLI_SELECT_LOBOFFSET

  };

enum LOBcliQueryPhase
  {
    LOB_CLI_ExecImmed,
    LOB_CLI_Prepare,
    LOB_CLI_Execute,
    LOB_CLI_Fetch,
    LOB_CLI_Close,
    LOB_CLI_Dealloc
  };


/**********************************************************
  
 **********************************************************/
    
Lng32 SQL_EXEC_LOBcliInterface
(
 /*IN*/     char * inLobHandle,
 /*IN*/     Lng32  inLobHandleLen,
 /*IN*/     char * blackBox,
 /*IN*/     Int32* blackBoxLen,
 /*OUT*/    char * outLobHandle,
 /*OUT*/    Lng32 * outLobHandleLen,
 /*IN*/     LOBcliQueryType qType,
 /*IN*/     LOBcliQueryPhase qPhase,
 /*INOUT*/  Int64 * dataOffset, /* IN: for insert, 
                                   IN: for load/extract,
                                   OUT: for select */
 /*INOUT*/  Int64 * dataLen,    /* length of data.
                                   IN: for insert, out: for select.
                                   IN: for load (NULL or 0 length is EOD) */
 /*OUT*/    Int64 * outDescPartnKey,  /* returned after insert and select */
 /*OUT*/    Int64 * outDescSyskey,    /* returned after insert and select */
 /*INOUT*/  void* *cliInterface,  /* IN: if passed in and not null, use it.
                                        Used in case of cursor fetches. 
                                    OUT: if returned, save it and pass it back
           				in */
 /*IN*/     Int64 xnId,          /* xn id of the parent process, if non-zero */
 /*IN*/     NABoolean lobTrace
 );
Lng32 SQL_EXEC_LOB_GC_Interface
(
 /*IN*/     void *lobGlobals, // can be passed or NULL
 /*IN*/     char * handle,
 /*IN*/     Lng32  handleLen,
 /*IN*/     char*  hdfsServer,
 /*IN*/     Lng32  hdfsPort,
 /*IN*/     char *lobLocation,
 /*IN*/     Int64 lobMaxMemChunkLen, // if passed in as 0, will use default value of 1G for the in memory buffer to do compaction.
 /*IN*/     NABoolean lobTrace
 );

Lng32 SQL_EXEC_LOBddlInterface
(
 /*IN*/     char * schName,
 /*IN*/     Lng32  schNameLen,
 /*IN*/     Int64  objectUID,
 /*IN*/     Lng32  &numLOBs,
 /*IN*/     LOBcliQueryType qType,
 /*IN*/     short *lobNumList,
 /*IN*/     short *lobTypList,
 /*IN*/     char* *lobLocList,
 /*IN*/     char* *lobColNameList,
 /*IN*/     char*  hdfsServer,
 /*IN*/     Lng32  hdfsPort,
 /*IN*/     Int64 lobMaxSize,
 /*IN*/     NABoolean lobTrace
 );
Lng32 SQL_EXEC_SetLobLock(/* IN */   char *llid);
Lng32 SQL_EXEC_ReleaseLobLock(/* IN */ char *llid);
Lng32 SQL_EXEC_CheckLobLock(/* IN */   char *llid, /* IN */ Int32 *found);

  /***************************************************************************
    Called by loader to load or extract buffers of data.

    dataLoc: For load:    address of buffer containing data.
             For extract: address of buffer where data will be retrieved.
    dataLen: On input, length of buffer.Len of 0 indicate EOD.
             On output, length of data moved (for extract)
    lobHandle, lobHandleLen, lobInfo, lobInfoLen: these are values that are
             given to loader during the call to LOBsql2loaderInterface.
             Need to be passed in back to this method.
    Return code:  -ve num, if error. Errnums tbd.
                  0, if ok.
             
  **************************************************************************/
Lng32 SQL_EXEC_LOBloader2sqlInterface
(
 /*IN*/     char * lobHandle,
 /*IN*/     Lng32  lobHandleLen,
 /*IN*/     char * lobInfo,
 /*IN*/     Lng32  lobInfoLen,
 /*IN*/     LOBcliQueryType qType, /* LOB_DATA_LOAD or LOAD_DATA_EXTRACT */
 /*INOUT*/  char * dataLoc, 
 /*INOUT*/  Int64 &dataLen  , 
 /*INOUT*/  void* *cliInterface  /* INOUT: if returned, save it and 
           				   pass it back in on the next call */

 );

  /********************************************************************
    Interface between sql/executor to loader api.
    Called by sql to load data.
    This api will end up calling cli SQL_EXEC_LOBloader2sqlInterface to load
    buffers of data.
 
    fileNameLen, 
    fileName:             client file which contains lob data. Used by loader.
    loaderInfoLen, 
    loaderInfo:           other loader info (port id, etc). Used by loader.

    handleLen, handle:    lob handle info. Should be passed back to cli
    lobInfoLen, lobInfo:  Other info related to lob. Should be passed back to cli

    Return code:  -ve num, if error. Errnums tbd.
                  0, if ok.
  ************************************************************************/
Lng32 LOBsql2loaderInterface
(
 /*IN*/     char * fileName,
 /*IN*/     Lng32  fileNameLen,
 /*IN*/     char * loaderInfo,
 /*IN*/     Lng32  loaderInfoLen,
 /*IN*/     char * handle,
 /*IN*/     Lng32  handleLen,
 /*IN*/     char * lobInfo,
 /*IN*/     Lng32  lobInfoLen
 );
	
#ifdef __cplusplus
/* end of C linkage */
}
#endif
		 
#endif
