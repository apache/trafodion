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


#ifndef _SRVRCOMMON_DEFINED
#define _SRVRCOMMON_DEFINED
#include <platform_ndcs.h>
#include "SrvrFunctions.h"
#include "CSrvrStmt.h"
#include "CoreCommon.h"
//#include "eventMsgs.h"

#define CLEAR_EXCEPTION(exception) { \
    exception.exception_nr = 0; \
    exception.exception_detail = 0; \
    exception.u.SQLError.errorList._length = 0; \
    exception.u.SQLError.errorList._buffer = NULL; \
}

#define CLEAR_WARNING(warning) { \
    warning._length = 0; \
    warning._buffer = NULL; \
}

#define CLEAR_ERROR(error) { \
    error.errorList._length = 0; \
    error.errorList._buffer = NULL; \
}

#define CLEAR_LIST(SQLValueList) { \
    SQLValueList._length = 0; \
    SQLValueList._buffer = NULL; \
}

struct SRVR_STMT_HDL_LIST {
    SRVR_STMT_HDL   *pSrvrStmt;
    struct SRVR_STMT_HDL_LIST *next;
    struct SRVR_STMT_HDL_LIST *pre;
};

struct SRVR_SESSION_HDL {
    SRVR_STMT_HDL_LIST *pSrvrStmtListHead;
    SRVR_STMT_HDL      *pCurrentSrvrStmt;
    long                count;
};

// Linux port. Temprarily moved from SrvrFunctions.h
#define STMT_LABEL_NOT_FOUND    1
#define DATA_TYPE_ERROR         2
#define PREPARE_EXCEPTION       3
#define EXECUTE_EXCEPTION       4
#define DATA_ERROR              5
#define FETCH_EXCEPTION         6
#define TRANSACTION_EXCEPTION   7

// For Hash support.
#define STREAMING_MODE                0x40000000    //(2^30)
#define STREAMING_DELAYEDERROR_MODE   0x20000000    //(2^29)
#define CHARSET		                  0x10000000    //(2^28) // For charset changes compatibility
#define ROWWISE_ROWSET                0x8000000     //(2^27)
#define PASSWORD_SECURITY             0x4000000     //(2^26)
#define MXO_SPECIAL_1_MODE            0x1000        // Compatibility mode

// Following are the global variables
#define             NO_OF_DESC_ITEMS        16
#define             TIMEBUFSIZE             22

extern SRVR_GLOBAL_Def      *srvrGlobal;
extern __thread SQLDESC_ITEM        gDescItems[];
extern __thread SQLMODULE_ID		nullModule;
extern __thread char                CatalogNm[MAX_ANSI_NAME_LEN+1];
extern __thread char                SchemaNm[MAX_ANSI_NAME_LEN+1];
extern __thread char                TableNm[MAX_ANSI_NAME_LEN+1];
extern __thread char                ColumnName[MAX_ANSI_NAME_LEN+1];
extern __thread char                ColumnHeading[MAX_ANSI_NAME_LEN+1];

extern long                 *TestPointArray;
#define NUMERIC_NULL            -32767
#define NUMEIRC_NULL_IN_CHAR    "-32767"
// Function Prototypes
extern void allocSrvrSessionHdl();
extern int initSqlCore(int argc, char *argv[]);

//Soln. No.: 10-100202-7923
extern SRVR_STMT_HDL *getSrvrStmt(long dialogueId,
                                  long stmtId,
                                  long *retcode = NULL);

extern SRVR_STMT_HDL *getInternalSrvrStmt(long dialogueId, const char* stmtLabel, long* retcode = NULL);

extern void removeSrvrStmt(long dialogueId, long stmtId);
extern SRVR_STMT_HDL *createSrvrStmt(long dialogueId,
                                     const char *stmtLabel,
                                     long *sqlcode = NULL,
                                     const char *moduleName = NULL,
                                     long moduleVersion = SQLCLI_ODBC_MODULE_VERSION,
                                     long long moduleTimestamp = 0,
                                     short sqlStmtType = TYPE_UNKNOWN,
                                     BOOL useDefaultDesc = FALSE,
                                     BOOL internalStmt = FALSE,
                                     long stmtId = 0,
									 short sqlQueryType = SQL_UNKNOWN,
									 Int32  resultSetIndex = 0,
									 SQLSTMT_ID* callStmtId = NULL);

extern SRVR_STMT_HDL *createSpjrsSrvrStmt(SRVR_STMT_HDL *callpStmt,
                                          long dialogueId,
                                          const char *RSstmtLabel,
                                          long *sqlcode = NULL,
                                          const char *moduleName = NULL,
                                          long moduleVersion = SQLCLI_ODBC_MODULE_VERSION,
                                          long long moduleTimestamp = 0,
                                          short sqlStmtType = TYPE_UNKNOWN,
                                          long RSindex = 0,
                                          const char *RSstmtName = NULL,
                                          BOOL useDefaultDesc = FALSE
                                          );
extern void getMemoryAllocInfo(long data_type, long char_set, long data_length, long vc_ind_length, long curr_mem_offset,
                               long *mem_align_offset, int *alloc_size, long *var_layout);
extern void releaseCachedObject(BOOL internalStmt);

extern void convertWildcard(unsigned long metadataId, BOOL isPV, const char *inName, char *outName);
extern void convertWildcardNoEsc(unsigned long metadataId, BOOL isPV, const char *inName, char *outName);
extern BOOL checkIfWildCard(const char *inName, char *outName);
extern short SetAutoCommitOff();

// The following two function protoypes are placed here, though it is not implemented in SrvrCommon.cpp
// Each of the DLL which links with this static LIB, should implement these functions
// in one of their module.
// This is done to minimize the compilcations in project settings and also it is felt that
// each DLL may have different implementation for these functions
extern void destroyConnIdleTimer();
extern void startConnIdleTimer();


extern short do_ExecSql(
                        /* In   */ void * objtag_
                        , /* In */ const CEE_handle_def *call_id_
                        , /* Out   */ ExceptionStruct *prepareException
                        , /* Out   */ ExceptionStruct *executeException
                        , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                        , /* In */ long dialogueId
                        , /* In */ SMD_QUERY_TABLE *queryTable
                        , /* In */ const char *stmtLabel
                        , /* In */ const char *catalogNm
                        , /* In */ const char *locationNm
                        , /* In */ char *inputParam[]
, /* Out   */ SQLItemDescList_def *outputDesc);

extern short executeSQLQuery( void * objtag_
                             , const CEE_handle_def *call_id_
                             , long dialogueId
                             , SMD_QUERY_TABLE *queryTable
                             , const char *stmtLabel
                             , const char *locationNm
                             , char *inputParam[]
, ExceptionStruct *prepareException
, ExceptionStruct *executeException
, ERROR_DESC_LIST_def   *sqlWarning);

extern short executeAndFetchSQLQuery( void * objtag_
                                     , const CEE_handle_def *call_id_
                                     , long dialogueId
                                     , SMD_QUERY_TABLE *queryTable
                                     , const char *stmtLabel
                                     , long maxRowCnt
                                     , char *inputParam[]
, ExceptionStruct *prepareException
, ExceptionStruct *executeException
, ExceptionStruct *fetchException
, ERROR_DESC_LIST_def   *sqlWarning
, long *rowsAffected
, SQLValueList_def *outputValueList);

short executeAndFetchSMDQuery(
    /* In   */ void * objtag_
  , /* In   */ const CEE_handle_def *call_id_
  , /* In   */ long dialogueId
  , /* In   */ short APIType
  , /* In   */ const char *stmtLabel
  , /* In   */ short sqlStmtType
  , /* In   */ const char *tableParam[]
  , /* In   */ const char *inputParam[]
  , /* In   */ const char *catalogNm
  , /* In   */ const char *schemaNm
  , /* In   */ const char *tableNm
  , /* In   */ const char *columnNm
  , /* In   */ const char *tableTypeList
  , /* In   */ unsigned long metadataId
  , /* Out   */ SQLItemDescList_def *outputDesc
  , /* Out   */ ExceptionStruct *executeException
  , /* Out   */ ExceptionStruct *fetchException
  , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
  , /* Out   */ long *rowsAffected
  , /* Out   */ SQLValueList_def *outputValueList
  , /* Out   */ long *stmtId
  );

extern short completeTransaction( void * objtag_
                                 , const CEE_handle_def *call_id_
                                 , long dialogueId
                                 , unsigned short transactionOpt
                                 , ExceptionStruct *transactionException
                                 , ERROR_DESC_LIST_def  *sqlWarning);

extern BOOL writeServerException( short retCode
                                 , ExceptionStruct *exception_
                                 , ExceptionStruct *prepareException
                                 , ExceptionStruct *executeException
                                 , ExceptionStruct *fetchException);
short do_ExecSMD(
                        /* In   */ void * objtag_
                        , /* In */ const CEE_handle_def *call_id_
                        , /* Out   */ ExceptionStruct *executeException
                        , /* Out   */ ERROR_DESC_LIST_def *sqlWarning
                        , /* In */ long dialogueId
                        , /* In */ const char *tableTypeList
                        , /* In */ unsigned long metadataId
                        , /* In */ short APIType
                        , /* In */ const char *stmtLabel
                        , /* In */ short sqlStmtType
                        , /* In */ const char *catalogNm
                        , /* In */ const char *schemaNm
                        , /* In */ const char *tableNm
                        , /* In */ const char *columnNm
                        , /* In */ const char *tableParam[]
, /* In */ const char *inputParam[]
, /* Out   */ SQLItemDescList_def *outputDesc
, /* Out   */ long *stmtId
);

extern "C" void 
GETMXCSWARNINGORERROR(
        /* In    */ Int32 sqlcode
        , /* In    */ char *sqlState
        , /* In    */ char *msg_buf
        , /* Out   */ Int32 *MXCSWarningOrErrorLength
        , /* Out   */ BYTE *&MXCSWarningOrError);

void appendOutputValueList(SQLValueList_def *targ, SQLValueList_def *src, bool free_src);
void freeOutputValueList(SQLValueList_def *ovl);

extern void print_outputValueList(SQLValueList_def *oVL, long colCount, const char * fcn_name);

extern BOOL nullRequired(long charSet);

// Linux port - ToDo
// Added dummy functions for transaction support (originally in pThreadsSync.h - we are not including that)
extern short abortTransaction (void);
extern short beginTransaction (long *transTag);
extern short resumeTransaction (long transTag);
extern short endTransaction (void);

// Add for new Rowsets logic
extern char* strcpyUTF8(char *dest, const char *src, size_t destSize, size_t copySize=0);
extern int getAllocLength(int DataType, int Length);

// +++ T2_REPO
void getCurrentTime(char* timestamp);
extern void translateToUTF8(Int32 inCharset, char* inStr, Int32 inStrLen, char* outStr, Int32 outStrMax);
//

#endif
