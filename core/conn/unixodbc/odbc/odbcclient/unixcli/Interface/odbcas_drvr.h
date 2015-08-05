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

#ifndef ODBCAS_DRVR_H
#define ODBCAS_DRVR_H

CEE_status MAP_AS_ERRORS(long signature);

extern "C" CEE_status
odbcas_ASSvc_GetObjRefHdl_(
    /* In    */ const CEE_handle_def *ph_
  , /* In    */ CEE_tag_def tag_
  , /* In    */ const CONNECTION_CONTEXT_def *inContext
  , /* In    */ const USER_DESC_def *userDesc
  , /* In    */ IDL_long srvrType
  , /* In    */ IDL_short retryCount
  , /* Out   */ odbcas_ASSvc_GetObjRefHdl_exc_ *exception_
  , /* Out   */	char 				*srvrObjRef
  , /* Out   */ DIALOGUE_ID_def 	*dialogueId
  , /* Out   */ char 				*dataSource
  , /* Out   */ USER_SID_def 		*userSid
  , /* Out   */ VERSION_LIST_def 	*versionList
  , /* Out   */ IDL_long		 	*isoMapping
  , /* Out   */ IDL_long		 	*srvrNodeId
  , /* Out   */ IDL_long		 	*srvrProcessId
  , /* Out   */ IDL_long_long		*timestamp
  );
#endif
