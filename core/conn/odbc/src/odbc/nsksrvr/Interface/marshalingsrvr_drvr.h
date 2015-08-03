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

CEE_status
odbcas_ASSvc_RegProcess_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
		, /* In    */ const VERSION_def *intfVersion
		, /* In    */ IDL_long srvrType
		, /* In    */ const IDL_char *srvrObjRef
		, /* In    */ const PROCESS_ID_def *nskProcessInfo
);

CEE_status
odbcas_ASSvc_UpdateSrvrState_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
	    , /* In    */ IDL_long srvrType
		, /* In    */ const IDL_char *srvrObjRef
		, /* In    */ IDL_long srvrState
  	    , /* In    */ IDL_long userID
		, /* In    */ const IDL_char *userSID
);

CEE_status
odbcas_ASSvc_WouldLikeToLive_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
	    , /* In    */ IDL_long srvrType
	    , /* In    */ const IDL_char *srvrObjRef
);

CEE_status
MxoSrvr_ValidateToken_param_pst_(
		  CInterface* pSystem
		, char*& buffer
		, long& message_length
  , /* In    */ int inTokenLen
  , /* In    */ unsigned char *inToken
  , /* In    */ int maxOutTokenLen
  );

#endif
