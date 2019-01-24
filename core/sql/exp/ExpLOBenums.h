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
#ifndef EXP_LOB_ENUMS_H
#define EXP_LOB_ENUMS_H

#include "ComSmallDefs.h"
#include "ComAnsiNamePart.h"
#define LOB_HANDLE_LEN 1024
#define LOB_NAME_LEN 50 // LOBP_<objectUid>_<LOBnum>  
                         //      <---20----> <--4--->Need 30 bytes -  allocating extra.
#define MAX_LOB_FILE_NAME_LEN 256
#define MAX_BLACK_BOX_LEN 2048
#define LOB_DESC_HEADER_KEY 1
#define NUM_WORKER_THREADS 2
#define LOB_LOCK_ID_SIZE 12
 
// 2 threads at most, one to read and the other to pick up next read from preOpen

#define LOB_CURSOR_PREFETCH_BYTES_MAX (1 << 27) // 128MB
// IMPORTANT //
// If an enum is added here, make sure that a corresponding entry is
// made in lobErrorEnumStr that follows this enum list.
typedef enum {
  LOB_MIN_ERROR_NUM = 500,
  LOB_OPER_OK = LOB_MIN_ERROR_NUM,
  LOB_INIT_ERROR,
  LOB_GLOB_PTR_ERROR,
  LOB_DIR_UNDEFINED,
  LOB_DATA_FILE_FULL_ERROR,
  LOB_DESC_FILE_CREATE_ERROR,
  LOB_DATA_FILE_CREATE_ERROR,
  LOB_DESC_FILE_OPEN_ERROR,
  LOB_DATA_FILE_OPEN_ERROR,
  LOB_DESC_FILE_WRITE_ERROR,
  LOB_DATA_FILE_WRITE_ERROR =510,
  LOB_DESC_FILE_LOCK_ERROR,
  LOB_DATA_FILE_LOCK_ERROR,
  LOB_DESC_HEADER_READ_ERROR,
  LOB_DESC_HEADER_WRITE_ERROR,
  LOB_DESC_WRITE_ERROR,
  LOB_DATA_WRITE_ERROR,
  LOB_DATA_FILE_DELETE_ERROR,
  LOB_DEST_FILE_OPEN_ERROR,
  LOB_DESC_FILE_DELETE_ERROR,
  LOB_SOURCE_FILE_LOCK_ERROR =520,
  LOB_SOURCE_FILE_STAT_ERROR ,
  LOB_SOURCE_FILE_OPEN_ERROR,
  LOB_SOURCE_FILE_READ_ERROR,
  LOB_SOURCE_DATA_ALLOC_ERROR,
  LOB_SOURCE_DATA_ERROR,
  LOB_TARGET_FILE_OPEN_ERROR,
  LOB_TARGET_FILE_WRITE_ERROR,
  LOB_DESC_READ_ERROR,
  LOB_DATA_READ_ERROR,
  LOB_DATA_READ_SIZE_ERROR =530,
  LOB_ALLOC_ERROR,
  LOB_PTR_ERROR ,
  LOB_OPER_ERROR,
  LOB_SUBOPER_ERROR,
  LOB_DIR_NAME_ERROR,
  LOB_STORAGE_TYPE_ERROR,
  LOB_DESC_SIZE_ERROR,
  LOB_REQUEST_UNDEFINED_ERROR,
  LOB_SEND_MSG_ERROR,
  LOB_OPER_REQ_IN_PROGRESS = 540,
  LOB_OPER_REQ_DONE,
  LOB_SERVER_OPEN_ERROR ,
  LOB_DESC_APPEND_ERROR,
  LOB_CURSOR_NOT_OPEN,
  LOB_DESC_UPDATE_ERROR,
  LOB_HANDLE_IN_LEN_ERROR,
  LOB_HANDLE_OUT_LEN_ERROR,
  LOB_BLACK_BOX_LEN_ERROR,
  LOB_DATA_FLUSH_ERROR ,
  LOB_HDFS_CONNECT_ERROR =550,
  LOB_HDFS_THREAD_CREATE_ERROR,
  LOB_HDFS_THREAD_SIGMASK_ERROR,
  LOB_HDFS_REQUEST_UNKNOWN,
  LOB_DATA_FILE_NOT_FOUND_ERROR,
  LOB_DATA_FILE_NOT_EMPTY_ERROR,
  LOB_DATA_FILE_POSITION_ERROR,
  LOB_CURSOR_NOT_OPEN_ERROR,
  LOB_OPER_CONTINUE,
  LOB_INVALID_ERROR_VAL,
  LOB_MAX_LIMIT_ERROR = 560,
  LOB_TARGET_FILE_EXISTS_ERROR,
  LOB_DATA_MOD_CHECK_ERROR,
  LOB_DATA_EMPTY_ERROR,
  LOB_MAX_ERROR_NUM     // keep this as the last element in enum list.
} Ex_Lob_Error;

static const char * const lobErrorEnumStr[] =
  {
  "LOB_OPER_OK",
  "LOB_INIT_ERROR",
  "LOB_GLOB_PTR_ERROR",
  "LOB_DIR_UNDEFINED",
  "LOB_DATA_FILE_FULL_ERROR",
  "LOB_DESC_FILE_CREATE_ERROR",
  "LOB_DATA_FILE_CREATE_ERROR",
  "LOB_DESC_FILE_OPEN_ERROR",
  "LOB_DATA_FILE_OPEN_ERROR",
  "LOB_DESC_FILE_WRITE_ERROR", 
  "LOB_DATA_FILE_WRITE_ERROR",//510
  "LOB_DESC_FILE_LOCK_ERROR",
  "LOB_DATA_FILE_LOCK_ERROR",
  "LOB_DESC_HEADER_READ_ERROR",
  "LOB_DESC_HEADER_WRITE_ERROR",
  "LOB_DESC_WRITE_ERROR",
  "LOB_DATA_WRITE_ERROR",
  "LOB_DATA_FILE_DELETE_ERROR", 
  "LOB_DEST_FILE_OPEN_ERROR",
  "LOB_DESC_FILE_DELETE_ERROR",
  "LOB_SOURCE_FILE_LOCK_ERROR", //520
  "LOB_SOURCE_FILE_STAT_ERROR",
  "LOB_SOURCE_FILE_OPEN_ERROR",
  "LOB_SOURCE_FILE_READ_ERROR",
  "LOB_SOURCE_DATA_ALLOC_ERROR",
  "LOB_SOURCE_DATA_ERROR",
  "LOB_TARGET_FILE_OPEN_ERROR",
  "LOB_TARGET_FILE_WRITE_ERROR",
  "LOB_DESC_READ_ERROR", 
  "LOB_DATA_READ_ERROR", 
  "LOB_DATA_READ_SIZE_ERROR", //530
  "LOB_ALLOC_ERROR",
  "LOB_PTR_ERROR",
  "LOB_OPER_ERROR",
  "LOB_SUBOPER_ERROR",
  "LOB_DIR_NAME_ERROR",
  "LOB_STORAGE_TYPE_ERROR",
  "LOB_DESC_SIZE_ERROR",
  "LOB_REQUEST_UNDEFINED_ERROR",
  "LOB_SEND_MSG_ERROR",
  "LOB_OPER_REQ_IN_PROGRESS",//540
  "LOB_OPER_REQ_DONE",
  "LOB_SERVER_OPEN_ERROR",
  "LOB_DESC_APPEND_ERROR",
  "LOB_CURSOR_NOT_OPEN",
  "LOB_DESC_UPDATE_ERROR",
  "LOB_HANDLE_IN_LEN_ERROR",
  "LOB_HANDLE_OUT_LEN_ERROR",
  "LOB_BLACK_BOX_LEN_ERROR",
  "LOB_DATA_FLUSH_ERROR", 
  "LOB_HDFS_CONNECT_ERROR", //550
  "LOB_HDFS_THREAD_CREATE_ERROR",
  "LOB_HDFS_THREAD_SIGMASK_ERROR",
  "LOB_HDFS_REQUEST_UNKNOWN",
  "LOB_DATA_FILE_NOT_FOUND_ERROR",
  "LOB_DATA_FILE_NOT_EMPTY_ERROR",
  "LOB_DATA_FILE_POSITION_ERROR",
  "LOB_CURSOR_NOT_OPEN_ERROR",
  "LOB_OPER_CONTINUE",
  "LOB_INVALID_ERROR_VAL", 
  "LOB_MAX_LIMIT_ERROR", //560
  "LOB_TGT_FILE_EXISTS_ERROR",
  "LOB_DATA_MOD_CHECK_ERROR",
  "LOB_DATA_EMPTY_ERROR",
  "LOB_MAX_ERROR_NUM"     // keep this as the last element in enum list.
};

typedef enum {
  EX_LOB_CREATE = 500,
  EX_LOB_RW
} Ex_Lob_Mode;

typedef ComLobsStorage LobsStorage;

typedef enum {
   Lob_None,
   Lob_File,
   Lob_Memory,
   Lob_Buffer,
   Lob_Lob, // tranfer from lob column of one table to another
   Lob_External_Lob,//transfer from external lob of one table to another
   Lob_External_File // link external hdfs file into traf lob column without an hdfs copy. 
} LobsSubOper;

typedef enum {
  HDFS_FILE, // specified with "hdfs:///<dir>/<file>" syntax
  CURL_FILE, // external http file specified "http://www.xyz.com" or "file:///<dir>/<filename>"syntax
  LOCAL_FILE // specifiled with simple /dir/filename syntax
 
} LobInputOutputFileType;

typedef enum {
    EX_LOB_DATA_INITIALIZING = 1000,
    EX_LOB_DATA_WRITING,
    EX_LOB_DATA_WRITTEN,
    EX_LOB_DATA_DELETED
} Ex_Lob_Data_State;

typedef enum {
   Lob_Init,
   Lob_Create,
   Lob_Insert,
   Lob_InsertDesc,
   Lob_InsertData,
   Lob_InsertDataSimple,

   Lob_InsSel,
   Lob_Delete,
   Lob_Purge,
   Lob_Append,
   Lob_Update,

   Lob_Read,
   Lob_ReadDesc,
   Lob_ReadData,
   Lob_ReadDataSimple,

   Lob_OpenCursor,
   Lob_ReadCursor,
   Lob_CloseCursor,

   Lob_OpenDescCursor,
   Lob_ReadDescCursor,
   Lob_CloseDescCursor,

   Lob_OpenDataCursor,
   Lob_ReadDataCursor,
   Lob_CloseDataCursor,

   Lob_OpenDataCursorSimple,
   Lob_ReadDataCursorSimple,
   Lob_CloseDataCursorSimple,

   Lob_CloseFile,

   Lob_Drop,
   Lob_Check_Status,

   Lob_Print, // debugging purposes

   Lob_Empty_Directory,
   Lob_Data_Mod_Check,

   Lob_Cleanup, // destroy everything under globals
   Lob_PerformGC,
   Lob_RestoreLobDataFile,
   Lob_PurgeBackupLobDataFile,
   Lob_GetLength,
   Lob_GetFileName,
   Lob_GetOffset,
   Lob_GetFileSize,
   Lob_ReadSourceFile
   
} LobsOper;

typedef enum {
   Lob_Commit,
   Lob_UnCommit,
   Lob_Rollback
} LobsTrans;

typedef enum {
    Lob_Req_Invalid,
    Lob_Req_Allocate_Desc,
    Lob_Req_Delete,
    Lob_Req_Read,
    Lob_Req_Append,
    Lob_Req_Update,
    Lob_Req_Create,
    Lob_Req_Get_Desc,
    Lob_Req_Put_Desc,
    Lob_Req_Del_Desc,
    Lob_Req_Purge_Desc,
    Lob_Req_Select_Cursor,
    Lob_Req_Fetch_Cursor,
    Lob_Req_Print
} LobsRequest;

typedef enum {
   Lob_Hdfs_Cursor_Prefetch,
   Lob_Hdfs_Read,
   Lob_Hdfs_Write,
   Lob_Hdfs_Shutdown
} LobsHdfsRequestType;

typedef enum {
   Lob_Cursor_Simple
} LobsCursorType;

typedef enum { 
  Lob_Append_Or_Error =0,
  Lob_Truncate_Or_Error,
  Lob_Truncate_Or_Create,
  Lob_Error_Or_Create,
  Lob_Append_Or_Create
     
} LobTgtFileFlags;
#endif
