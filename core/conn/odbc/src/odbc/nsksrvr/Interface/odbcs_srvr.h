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

#ifndef ODBCS_SRVR_H
#define ODBCS_SRVR_H

void
DISPATCH_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* Out   */ CEERCV_IOMessage_exc_ *exception_
  , /* In    */ const IDL_short *receiveInfo
  , /* In    */ IDL_short dialogInfo
  , /* In    */ const CEERCV_IOMessage_request_seq_ *request
  , /* Out   */ IDL_short *error
  , /* Out   */ CEERCV_IOMessage_reply_seq_ *reply
  ,	short operation_id
);

void
SQLCONNECT_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
SQLDISCONNECT_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
SQLSETCONNECTATTR_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
SQLENDTRAN_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
SQLPREPARE_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );


#if 0
void
SQLEXECDIRECTROWSET_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
SQLEXECUTEROWSET_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );


void
SQLEXECDIRECT_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
SQLEXECUTE2_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
SQLEXECUTECALL_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

#endif 0

void
SQLEXECUTE_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ short operation_id
  );


void
SQLFETCH_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  , /* In    */ short operation_id
  );

void
SQLFREESTMT_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
 );

void
SQLGETCATALOGS_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );


void
STOPSRVR_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
ENABLETRACE_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
DISABLETRACE_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
ENABLESTATISTICS_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
DISABLESTATISTICS_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
UPDATECONTEXT_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
MONITORCALL_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
ValidateToken_IOMessage(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  );

void
EXTRACTLOB_IOMessage(
    /* In    */CEE_tag_def objtag_
  , /* In    */const CEE_handle_def *call_id_
  );

void
UPDATELOB_IOMessage(
    /* In  */ CEE_tag_def objtag_
  , /* In  */ const CEE_handle_def *call_id_
  );
void LOG_MSG(CError* ierror, short level);

void LOG_ERROR(CError* ierror);

void LOG_INFO(CError* ierror);

void LOG_WARNING(CError* ierror);

//
//========================== TCPIP ========================
//
void
DISPATCH_TCPIPRequest(
    /* In    */ CEE_tag_def objtag_
  , /* In    */ const CEE_handle_def *call_id_
  ,	short operation_id
);

void
ReleaseServer();
#endif
