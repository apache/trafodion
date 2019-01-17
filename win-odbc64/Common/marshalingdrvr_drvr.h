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

#ifndef MARSHALING_DRVR_H
#define MARSHALING_DRVR_H

#include "marshaling.h"
#include "swap.h"

CEE_status 
odbcas_ASSvc_GetObjRefHdl_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ const CONNECTION_CONTEXT_def *inContext
		, /* In    */ const USER_DESC_def *userDesc
		, /* In    */ IDL_long srvrType
		, /* In    */ IDL_short retryCount
);


CEE_status
odbcas_ASSvc_StopSrvr_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_long srvrType
		, /* In    */ const IDL_char *srvrObjRef
		, /* In    */ IDL_long StopType
);


CEE_status
odbc_SQLSvc_InitializeDialogue_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ const USER_DESC_def *userDesc
		, /* In    */ const CONNECTION_CONTEXT_def *inContext
		, /* In    */ DIALOGUE_ID_def dialogueId
);

CEE_status
odbc_SQLSvc_TerminateDialogue_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
);


CEE_status
odbc_SQLDrvr_Prepare_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_long sqlAsyncEnable
		, /* In    */ IDL_long queryTimeout
		, /* In    */ IDL_long sqlStmtType
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_long stmtCharset
		, /* In    */ IDL_string cursorName
		, /* In    */ IDL_long cursorCharset
		, /* In    */ const IDL_char *moduleName
		, /* In    */ IDL_long moduleCharset
		, /* In    */ IDL_long_long moduleTimestamp
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_long sqlStringCharset
		, /* In    */ IDL_string setStmtOptions
		, /* In    */ const IDL_char *stmtExplainLabel
		, /* In    */ IDL_short stmtType
        	, /* In    */ IDL_long maxRowsetSize
       		, /* In    */ IDL_long holdableCursor
);

CEE_status
odbc_SQLDrvr_Fetch_param_pst_(
                  CInterface* pSystem
                , IDL_char*& buffer
                , long& message_length
                , /* In    */ DIALOGUE_ID_def dialogueId
                , /* In    */ IDL_unsigned_long_long MaxRowCnt
                , /* In    */ IDL_unsigned_long_long MaxRowLen
                , /* In    */ IDL_long sqlAsyncEnable
                , /* In    */ IDL_long queryTimeout
                , /* In    */ Long stmtHandle
                , /* In    */ const IDL_char *stmtLabel
		        , /* In    */ IDL_long stmtCharset
                , /* In    */ IDL_string cursorName
                , /* In    */ IDL_long cursorCharset
		        , /* In    */ const IDL_char *setStmtOptions
);



CEE_status
odbc_SQLDrvr_Close_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_unsigned_short freeResourceOpt
);


CEE_status
odbc_SQLDrvr_Execute_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_long sqlAsyncEnable
       		, /* In    */ IDL_long holdableCursor		
		, /* In    */ IDL_long queryTimeout
		, /* In    */ IDL_long inputRowCnt
		, /* In    */ IDL_long maxRowsetSize
		, /* In    */ IDL_long sqlStmtType
		, /* In    */ Long stmtHandle
		, /* In    */ IDL_long stmtType
		, /* In    */ IDL_string sqlString
		, /* In    */ IDL_long sqlStringCharset
		, /* In    */ IDL_string cursorName
		, /* In    */ IDL_long cursorCharset
		, /* In    */ IDL_string stmtLabel
		, /* In    */ IDL_long stmtCharset
		, /* In    */ IDL_string stmtExplainLabel
		, /* In    */ IDL_long inValuesLength
		, /* In    */ BYTE *inValues
		, /* In    */ const SQLValueList_def *inValueList
);

CEE_status
odbc_SQLDrvr_SetConnectionOption_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ IDL_short connectionOption
		, /* In    */ IDL_long optionValueNum
		, /* In    */ IDL_string optionValueStr
  );

CEE_status
odbc_SQLDrvr_GetSQLCatalogs_param_pst_(
 		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
		, /* In    */ DIALOGUE_ID_def dialogueId
		, /* In    */ const IDL_char *stmtLabel
		, /* In    */ IDL_short APIType
		, /* In    */ const IDL_char *catalogNm
		, /* In    */ const IDL_char *schemaNm
		, /* In    */ const IDL_char *tableNm
		, /* In    */ const IDL_char *tableTypeList
		, /* In    */ const IDL_char *columnNm
		, /* In    */ IDL_long columnType
		, /* In    */ IDL_long rowIdScope
		, /* In    */ IDL_long nullable
		, /* In    */ IDL_long uniqueness
		, /* In    */ IDL_long accuracy
		, /* In    */ IDL_short sqlType
		, /* In    */ IDL_unsigned_long metadataId
	    , /* In    */ const IDL_char *fkcatalogNm
	    , /* In    */ const IDL_char *fkschemaNm
	    , /* In    */ const IDL_char *fktableNm
);

CEE_status
odbc_SQLDrvr_EndTransaction_param_pst_(
		  CInterface* pSystem
		, IDL_char*& buffer
		, long& message_length
	    , /* In    */ DIALOGUE_ID_def dialogueId
	    , /* In    */ IDL_unsigned_short transactionOpt
);

CEE_status
odbc_SQLDrvr_ExtractLob_param_pst_(
    /* In    */ CInterface * pSystem
  , /* In    */ IDL_char * & buffer
  , /* In    */ long        & message_length
  , /* In    */ IDL_long     extractType
  , /* In    */ IDL_string   lobHandle
  , /* In    */ IDL_long     lobHandleLen
  , /* In    */ IDL_long     lobHandleCharset
  , /* In    */ IDL_long     extractlen
);

CEE_status
odbc_SQLDrvr_UpdateLob_param_pst_(
    /* In    */ CInterface * pSystem
  , /* In    */ IDL_char * & buffer
  , /* In    */ long        & message_length
  , /* In    */ IDL_long     updataType
  , /* In    */ IDL_string   lobHandle
  , /* In    */ IDL_long     lobHandleLen
  , /* In    */ IDL_long     lobHandleCharset
  , /* In    */ IDL_long_long     totalLength
  , /* In    */ IDL_long_long     offset
  , /* In    */ BYTE *        data
  , /* In    */ IDL_long_long pos
  , /* In    */ IDL_long_long length
);
/************************************************************************************************************
 *                                                                                                          *
 * Keeping these functions around for the collapsed driver - get rid of these when it is not needed anymore *
 *                                                                                                          *
 ************************************************************************************************************/


#endif /* MARSHALING_DRVR_H */
