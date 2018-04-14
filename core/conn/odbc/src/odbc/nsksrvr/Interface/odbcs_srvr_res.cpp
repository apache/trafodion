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

#include "ceercv.h"

#include <platform_ndcs.h>
#include "FileSystemSrvr.h"
#include "TCPIPSystemSrvr.h"
#include "odbcCommon.h"
#include "odbcs_srvr_res.h"

#include "sql.h"
#include "sqlext.h"

#include "DrvrSrvr.h"
#include "Global.h"

#include "ceecfg.h"
#include "odbcCommon.h"
#include "odbc_sv.h"
#include "odbcas_cl.h" 
#include "marshalingsrvr_srvr.h"
#include "CSrvrStmt.h"


CEE_status
odbc_SQLSvc_InitializeDialogue_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_InitializeDialogue_exc_ *exception_
  , /* In    */ const OUT_CONNECTION_CONTEXT_def *outContext
  )
{
	SRVRTRACE_ENTER(FILE_OIOM+1);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	sts = odbc_SQLSvc_InitializeDialogue_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_
			, outContext
	);

	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	SRVRTRACE_EXIT(FILE_OIOM+1);
	return sts;
}

CEE_status
odbc_SQLSvc_TerminateDialogue_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_TerminateDialogue_exc_ *exception_
  )
{
	SRVRTRACE_ENTER(FILE_OIOM+2);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	sts = odbc_SQLSvc_TerminateDialogue_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_
	);
	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	SRVRTRACE_EXIT(FILE_OIOM+2);
	return sts;
}

CEE_status
odbc_SQLSrvr_Close_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ IDL_long returnCode
  , /* In    */ IDL_long rowsAffected
  , /* In    */ IDL_long sqlWarningOrErrorLength
  , /* In    */ BYTE *sqlWarningOrError
  )
{
	SRVRTRACE_ENTER(FILE_OIOM+12);

	CInterface* pnode = (CInterface*)objtag_;
	CEE_status sts = CEE_SUCCESS;

	IDL_char* buffer = NULL;
	IDL_unsigned_long message_length = 0;

	odbc_SQLSrvr_Close_param_res_(
			  pnode
			, buffer
			, message_length
			, returnCode
			, sqlWarningOrErrorLength
			, sqlWarningOrError
			, rowsAffected
	);

	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	SRVRTRACE_EXIT(FILE_OIOM+12);
	return sts;
}

CEE_status
odbc_SQLSvc_StopServer_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_StopServer_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OIOM+16);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	odbc_SQLSvc_StopServer_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_);

	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	if (sts != CEE_SUCCESS)
		pnode->send_error(sts, 0, call_id_);

	SRVRTRACE_EXIT(FILE_OIOM+16);
	return sts;
}

CEE_status
odbc_SQLSvc_EnableServerTrace_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_EnableServerTrace_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OIOM+17);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	odbc_SQLSvc_EnableServerTrace_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_);

	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	if (sts != CEE_SUCCESS)
		pnode->send_error(sts, 0, call_id_);

	SRVRTRACE_EXIT(FILE_OIOM+17);
	return sts;
}

CEE_status
odbc_SQLSvc_DisableServerTrace_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_DisableServerTrace_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OIOM+18);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	odbc_SQLSvc_DisableServerTrace_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_);

	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	if (sts != CEE_SUCCESS)
		pnode->send_error(sts, 0, call_id_);

	SRVRTRACE_EXIT(FILE_OIOM+18);
	return sts;
}

CEE_status
odbc_SQLSvc_EnableServerStatistics_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_EnableServerStatistics_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OIOM+19);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	odbc_SQLSvc_EnableServerStatistics_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_);

	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	if (sts != CEE_SUCCESS)
		pnode->send_error(sts, 0, call_id_);

	SRVRTRACE_EXIT(FILE_OIOM+19);
	return sts;
}

CEE_status
odbc_SQLSvc_DisableServerStatistics_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_DisableServerStatistics_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OIOM+20);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	odbc_SQLSvc_DisableServerStatistics_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_);

	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	if (sts != CEE_SUCCESS)
		pnode->send_error(sts, 0, call_id_);

	SRVRTRACE_EXIT(FILE_OIOM+20);
	return sts;
}

CEE_status
odbc_SQLSvc_UpdateServerContext_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_UpdateServerContext_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OIOM+21);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	odbc_SQLSvc_UpdateServerContext_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_);

	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	if (sts != CEE_SUCCESS)
		pnode->send_error(sts, 0, call_id_);

	SRVRTRACE_EXIT(FILE_OIOM+21);
	return sts;
}

CEE_status
odbc_SQLSvc_MonitorCall_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_MonitorCall_exc_ *exception_
)
{
	SRVRTRACE_ENTER(FILE_OIOM+22);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	odbc_SQLSvc_MonitorCall_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_);

	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	if (sts != CEE_SUCCESS)
		pnode->send_error(sts, 0, call_id_);

	SRVRTRACE_EXIT(FILE_OIOM+22);
	return sts;
}

CEE_status
odbc_SQLSrvr_Prepare_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In   */ IDL_long returnCode
  , /* In   */ IDL_long sqlWarningOrErrorLength
  , /* In   */ BYTE *sqlWarningOrError
  , /* In   */ IDL_long sqlQueryType
  , /* In   */ IDL_long stmtHandleKey
  , /* In   */ IDL_long estimatedCost
  , /* In   */ IDL_long inputDescLength
  , /* In   */ BYTE *inputDesc
  , /* In   */ IDL_long outputDescLength
  , /* In   */ BYTE *outputDesc
 )
{
	SRVRTRACE_ENTER(FILE_OIOM+11);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	odbc_SQLSrvr_Prepare_param_res_(
			  pnode
			, buffer
			, message_length
			, returnCode
			, sqlWarningOrErrorLength
			, sqlWarningOrError
			, sqlQueryType
			, stmtHandleKey
			, estimatedCost
			, inputDescLength
			, inputDesc
			, outputDescLength
			, outputDesc
	);

	if (sts == CEE_SUCCESS)
	{
		sts = pnode->send_response(buffer, message_length, call_id_);
	}
	SRVRTRACE_EXIT(FILE_OIOM+11);
	return sts;
} /* odbc_SQLSrvr_Prepare_ts_res_() */

CEE_status
odbc_SQLSrvr_Fetch_ts_res_(
    /* In    */       CEE_tag_def     objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */       IDL_long        returnCode
  , /* In    */       IDL_long        sqlWarningOrErrorLength
  , /* In    */       BYTE           *sqlWarningOrError
  , /* In    */       IDL_long        rowsReturned
  , /* In    */       IDL_long        outValuesFormat
  , /* In    */       IDL_long        outValuesLength
  , /* In    */       BYTE           *outValues
                          )
{
  SRVRTRACE_ENTER(FILE_OIOM+23);

  CInterface                   *pnode = (CInterface*)objtag_;
  CEE_status                    sts = CEE_SUCCESS;
  CEERCV_IOMessage_exc_         RCVexception_;
  IDL_short                     error;
  short                         reply_count;
  CEERCV_IOMessage_reply_seq_   reply;
  char                         *buffer = NULL;
  UInt32                        message_length = 0;

  odbc_SQLSrvr_Fetch_param_res_( pnode
			       , buffer
			       , message_length
			       , returnCode
			       , sqlWarningOrErrorLength
			       , sqlWarningOrError
			       , rowsReturned
			       , outValuesFormat
			       , outValuesLength
			       , outValues
                               );

  if (sts == CEE_SUCCESS)
    {
    sts = pnode->send_response(buffer, message_length, call_id_);
    }
  SRVRTRACE_EXIT(FILE_OIOM+11);
  return sts;

}  // end odbc_SQLSrvr_Fetch_ts_res_

CEE_status
odbc_SQLSrvr_Execute_ts_res_(
    /* In    */       CEE_tag_def     objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */       IDL_long        returnCode
  , /* In    */       IDL_long        sqlWarningOrErrorLength
  , /* In    */       BYTE           *sqlWarningOrError
  , /* In    */       IDL_long        rowsReturned
  , /* In    */       IDL_long        sqlQueryType
  , /* In    */       IDL_long        estimatedCost
  , /* In    */       IDL_long        outValuesLength
  , /* In    */       BYTE           *outValues
  , /* In    */		  IDL_long outputDescLength // Used with execdirect
  , /* In    */		  BYTE *outputDesc          // Used with execdirect
  , /* In    */		  Long stmtHandle       // Used for SPJ result sets
  , /* In    */       IDL_long   stmtHandleKey  
                          )
{
  SRVRTRACE_ENTER(FILE_OIOM+23);

  CInterface                   *pnode = (CInterface*)objtag_;
  CEE_status                    sts = CEE_SUCCESS;
  CEERCV_IOMessage_exc_         RCVexception_;
  IDL_short                     error;
  short                         reply_count;
  CEERCV_IOMessage_reply_seq_   reply;
  char                         *buffer = NULL;
  UInt32 message_length = 0;

  odbc_SQLSrvr_Execute_param_res_( pnode
			       , buffer
			       , message_length
			       , returnCode
			       , sqlWarningOrErrorLength
			       , sqlWarningOrError
			       , rowsReturned
				   , sqlQueryType
				   , estimatedCost
			       , outValuesLength
			       , outValues
				   , outputDescLength
				   , outputDesc
				   , stmtHandle
				   , stmtHandleKey
				   );

  if (sts == CEE_SUCCESS)
    sts = pnode->send_response(buffer, message_length, call_id_);
  SRVRTRACE_EXIT(FILE_OIOM+11);
  return sts;

}  // end odbc_SQLSrvr_Execute_ts_res_

CEE_status
odbc_SQLSrvr_SetConnectionOption_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_SetConnectionOption_exc_ *exception_
  , /* In    */ ERROR_DESC_LIST_def *sqlWarning
  )
{
	SRVRTRACE_ENTER(FILE_OIOM+3);
	CInterface* pnode = (CInterface*)objtag_;
	CEE_status sts = CEE_SUCCESS;

	IDL_char* buffer = NULL;
	IDL_unsigned_long message_length = 0;

	sts = odbc_SQLSrvr_SetConnectionOption_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_
			, sqlWarning
	);
	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	SRVRTRACE_EXIT(FILE_OIOM+3);
	return sts;
}

CEE_status
odbc_SQLSrvr_GetSQLCatalogs_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_GetSQLCatalogs_exc_ *exception_
  , /* In    */ const IDL_char *catStmtLabel
  , /* In    */ SQLItemDescList_def *outputDesc
  , /* In    */ ERROR_DESC_LIST_def *sqlWarning
  , /* In    */ SRVR_STMT_HDL *pSrvrStmt
  )
{
	SRVRTRACE_ENTER(FILE_OIOM+14);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	odbc_SQLSrvr_GetSQLCatalogs_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_
			, catStmtLabel
			, outputDesc
			, sqlWarning
			, pSrvrStmt
	);
	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	SRVRTRACE_EXIT(FILE_OIOM+14);
	return sts;

} // odbc_SQLSrvr_GetSQLCatalogs_ts_res_()

CEE_status
odbc_SQLSrvr_EndTransaction_ts_res_(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ const struct odbc_SQLSvc_EndTransaction_exc_ *exception_
  , /* In    */ ERROR_DESC_LIST_def *sqlWarning
  )
{
	SRVRTRACE_ENTER(FILE_OIOM+13);
	CInterface* pnode = (CInterface*)objtag_;

	CEE_status sts = CEE_SUCCESS;

	CEERCV_IOMessage_exc_ RCVexception_;
	IDL_short error;
	short reply_count;
	CEERCV_IOMessage_reply_seq_ reply;

	char* buffer = NULL;
	UInt32 message_length = 0;

	odbc_SQLSrvr_EndTransaction_param_res_(
			  pnode
			, buffer
			, message_length
			, exception_
			, sqlWarning
	);
	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
	SRVRTRACE_EXIT(FILE_OIOM+13);
	return sts;

} // odbc_SQLSrvr_EndTransaction_ts_res_()

CEE_status
odbc_SQLSrvr_ExtractLob_ts_res_(
    /* In   */ CEE_tag_def    objtag_
  , /* In   */ const CEE_handle_def *call_id_
  , /* In   */ const struct odbc_SQLsrvr_ExtractLob_exc_ *exception_
  , /* In   */ IDL_short extractLobAPI
  , /* In   */ IDL_long_long  lobLength
  , /* In   */ IDL_long_long  extractLen
  , /* In   */ BYTE   *  extractData
  )
{
    CInterface* pnode = (CInterface *)objtag_;

    CEE_status sts = CEE_SUCCESS;

    IDL_short error;
    CEERCV_IOMessage_reply_seq_ reply;

    char * buffer = NULL;
    UInt32 message_length = 0;

    sts = odbc_SQLsrvr_ExtractLob_param_res_(
              pnode
            , buffer
            , message_length
            , exception_
            , extractLobAPI
            , lobLength
            , extractLen
            , extractData
            );

    if (sts == CEE_SUCCESS)
        sts = pnode->send_response(buffer, message_length, call_id_);

    if (extractData != NULL)
    {
        delete [] extractData;
        extractData = NULL;
    }
    return sts;
}

void
odbc_SQLSrvr_UpdateLob_ts_res_(
	/* In   */ CEE_tag_def objtag_
  , /* In   */ const CEE_handle_def * call_id_
  , /* In   */ const struct odbc_SQLSvc_UpdateLob_exc_ * exception_
  )
{
	CInterface * pnode = (CInterface *) objtag_;

	CEE_status sts = CEE_SUCCESS;

	IDL_short error;
	CEERCV_IOMessage_reply_seq_ reply;

	char * buffer = NULL;
	UInt32 message_length = 0;

	sts = odbc_SQLsrvr_UpdateLob_param_res_(
		      pnode
		    , buffer
		    , message_length
		    , exception_
		    );

	if (sts == CEE_SUCCESS)
		sts = pnode->send_response(buffer, message_length, call_id_);
}
//LCOV_EXCL_START
//LCOV_EXCL_STOP

