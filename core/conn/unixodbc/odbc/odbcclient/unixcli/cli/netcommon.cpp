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
********************************************************************/
/*************************************************************************
**************************************************************************/
//
// MODULE:  NetConnect.cpp
//
//
//
// 

#include "drvrglobal.h"
#include "odbcsrvrcommon.h"
#include "drvrnet.h"
#include "cconnect.h"
#include "cstmt.h"
#include "diagfunctions.h"
#include "mxomsg.h"
#include "odbcas_cl.h"
#include "ping.h"


long stopSrvrAbrupt(SRVR_CALL_CONTEXT *srvrCallContext,odbcas_ASSvc_StopSrvr_exc_ *stopSrvrException)
{

	short			retryCount = 0;

	CConnect *pConnection = (CConnect *)srvrCallContext->sqlHandle;
	
	if (srvrCallContext->SQLSvc_ObjRef != '\0')
	{
		do
		{
			odbcas_ASSvc_StopSrvr(
					(CEE_handle_tag*)srvrCallContext,
					stopSrvrException,
					srvrCallContext->dialogueId,
					CORE_SRVR,
					srvrCallContext->SQLSvc_ObjRef,
					pConnection->m_srvrTCPIPSystem->odbcAPI);

			retryCount++;
		} while (stopSrvrException->exception_nr == -29 && retryCount <= 3);
	}
	return stopSrvrException->exception_nr;
}
	
