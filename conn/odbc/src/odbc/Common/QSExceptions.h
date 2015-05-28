/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
********************************************************************/

#ifndef QSEXCEPTIONS_H
#define QSEXCEPTIONS_H

/*
 * Exception number constants for
 * operation 'Query Service'
 */
#define qrysrvc_QueryRejected_exn_					1
#define qrysrvc_OperationFailed_exn_				2
#define qrysrvc_Com_InvalidRequest_exn_				3
#define qrysrvc_Com_SQLError_exn_					4
#define qrysrvc_Com_SQLNoDataFound_exn_				5		// must be 5
#define qrysrvc_Com_ConnInfo_exn_					6
#define qrysrvc_Com_MemoryAllocationError_exn_		7
#define qrysrvc_Com_PickDataError_exn_				8
#define qrysrvc_Com_MaxExecQueryCntZero_exn_		9
#define qrysrvc_Com_CurExecQueryCntZero_exn_		10
#define qrysrvc_Com_MaxQueryCntZero_exn_			11
#define qrysrvc_Com_CurQueryCntZero_exn_			12
#define qrysrvc_Com_QueueCntZero_exn_				13
#define qrysrvc_UnknownUser_exn_					14
#define qrysrvc_Com_InvalidUser_exn_				15
#define qrysrvc_Com_NoServicesFound_exn_			16
#define qrysrvc_Com_NoThresholdsFound_exn_			17
#define qrysrvc_Com_IncorrectVersion_exn_			18
#define qrysrvc_Com_ConfigError_exn_				19
#define qrysrvc_QueryCanceled_exn_					20
#define qrysrvc_QS_XATransaction_exn_				21

#define qrysrvc_QS_UnknownQueue_detail_				1
#define qrysrvc_QS_NoMoreResources_detail_			2
#define qrysrvc_QS_EstCostTooBig_detail_			3
#define qrysrvc_QS_NoMoreGlobalResources_detail_	4
#define qrysrvc_QS_QueryCanceledByAdmin_detail_		5
#define qrysrvc_QS_MaxNumberOfQueries_detail_		6
#define qrysrvc_QS_SystemIsOnHold_detail_			7
#define qrysrvc_QS_ServiceIsOnHold_detail_			8
#define qrysrvc_QS_WMSisNotRunning_detail_			9
#define qrysrvc_QS_WMSFileSystemError_detail_		10
#define qrysrvc_QS_GetServiceNotApplicabel_detail_	11
#define qrysrvc_QS_SetServiceNotApplicabel_detail_	12
#define qrysrvc_QS_WaitQueryTimeout_detail_			13
#define qrysrvc_QS_HoldQueryTimeout_detail_			14
#define qrysrvc_QS_SetServiceEnvError_detail_		15
#define qrysrvc_QS_IncorrectVersion_detail_			16
#define qrysrvc_QS_BINSEM_LOCK_error_detail_		17
#define qrysrvc_QS_BINSEM_UNLOCK_error_detail_		18
#define qrysrvc_QS_RejectedCompRule_error_detail_	19
#define qrysrvc_QS_SQL_CMD_error_detail_			20
#define qrysrvc_QS_IncorrectQueryState_detail_		21
#define qrysrvc_QS_QueryNotFound_detail_			22
#define qrysrvc_QS_MemoryAllocFailed_detail_		23
#define qrysrvc_QS_Query_Canceled_detail_			24
#define qrysrvc_QS_XATransaction_detail_			25
#define qrysrvc_QS_Query_RejectAborting_detail_         26

#define qrysrvc_QueryRejected_				"Query Rejected"
#define qrysrvc_OperationFailed_			"Operation Failed"
#define qrysrvc_UnknownUser_				"Unknown User"
#define qrysrvc_QueryCanceled_				"Query Aggregation Canceled"

#define qrysrvc_QS_UnknownQueue_			"Unknown Service"
#define qrysrvc_QS_NoMoreResources_			"No More Resources"
#define qrysrvc_QS_EstCostTooBig_			"Estimated Cost Too Big"
#define qrysrvc_QS_NoMoreGlobalResources_	"No More Global Resources"
#define qrysrvc_QS_QueryCanceledByAdmin_	"Query Canceled By WMS Administrator"
#define qrysrvc_QS_MaxNumberOfQueries_		"Number of queries in WMS has reached the predefined limit"
#define qrysrvc_QS_SystemIsOnHold_			"System state is Hold or Stopped"
#define qrysrvc_QS_ServiceIsOnHold_			"Service is quiescing, stopping, stopped or on hold"
#define qrysrvc_QS_WMSisNotRunning_			"WMS is not running"
#define qrysrvc_QS_WMSFileSystemError_		"File System error"
#define qrysrvc_QS_GetServiceNotApplicabel_	"GET SERVICE command is not applicable within WMSOPEN-WMSCLOSE"
#define qrysrvc_QS_SetServiceNotApplicabel_	"SET SERVICE command is not applicable within WMSOPEN-WMSCLOSE"
#define qrysrvc_WaitQueryTimeout_			"Wait Query Timeout occurred"
#define qrysrvc_HoldQueryTimeout_			"Hold Query Timeout occurred"
#define qrysrvc_QS_SetServiceEnvError_		"Error setting service defaults"
#define qrysrvc_QS_IncorrectVersion_		"Version mismatch between NDCS and WMS"
#define qrysrvc_QS_BINSEM_LOCK_error_		"BINSEM_LOCK_ error occurred"
#define qrysrvc_QS_BINSEM_UNLOCK_error_		"BINSEM_UNLOCK_ error occurred"
#define qrysrvc_QS_RejectedCompRule_error_	"Query Rejected by Comp Rules"
#define qrysrvc_QS_SQL_CMD_error_error_		"Query assigned by WMS failed"
#define qrysrvc_QS_IncorrectQueryState_		"Incorrect Query state"
#define qrysrvc_QS_QueryNotFound_			"Query not found"
#define qrysrvc_QS_MemoryAllocFailed_		"Memory Allocation Failed"
#define qrysrvc_QS_Query_Canceled_			"Query canceled by WMS"
#define qrysrvc_QS_Query_RejectedAbt_		"Query rejected due to long-running transaction rollback in progress on the system "

inline char* BuildExceptionMessage(char* msg, short msg_size, struct qrysrvc_exc_ *exception_,  short ierror)
{
	int error = ierror;

	short csize = msg_size;
	short len = 0;

	strcpy(msg," [WMS]");

	switch(exception_->exception_nr)
	{
	case qrysrvc_QueryRejected_exn_:
		strcat(msg,qrysrvc_QueryRejected_);
		break;
	case qrysrvc_OperationFailed_exn_:
		strcat(msg,qrysrvc_OperationFailed_);
		break;
	case qrysrvc_UnknownUser_exn_:
		strcat(msg,qrysrvc_UnknownUser_);
		break;
	case qrysrvc_QueryCanceled_exn_:
		strcat(msg,qrysrvc_QueryCanceled_);
		break;
	default:
		sprintf(&msg[strlen(msg)],"Unknown exception %d",exception_->exception_nr);
	}

	strcat(msg," - ");

	switch (exception_->exception_detail)
	{
	case 0:
		break;
	case qrysrvc_QS_UnknownQueue_detail_:
		strcat(msg,qrysrvc_QS_UnknownQueue_);
		break;
	case qrysrvc_QS_NoMoreResources_detail_:
		strcat(msg,qrysrvc_QS_NoMoreResources_);
		break;
	case qrysrvc_QS_EstCostTooBig_detail_:
		strcat(msg,qrysrvc_QS_EstCostTooBig_);
		break;
	case qrysrvc_QS_NoMoreGlobalResources_detail_:
		strcat(msg,qrysrvc_QS_NoMoreGlobalResources_);
		break;
	case qrysrvc_QS_QueryCanceledByAdmin_detail_:
		strcat(msg,qrysrvc_QS_QueryCanceledByAdmin_);
		break;
	case qrysrvc_QS_MaxNumberOfQueries_detail_:
		strcat(msg,qrysrvc_QS_MaxNumberOfQueries_);
		break;
	case qrysrvc_QS_SystemIsOnHold_detail_:
		strcat(msg,qrysrvc_QS_SystemIsOnHold_);
		break;
	case qrysrvc_QS_ServiceIsOnHold_detail_:
		strcat(msg,qrysrvc_QS_ServiceIsOnHold_);
		break;
	case qrysrvc_QS_WMSisNotRunning_detail_:
		strcat(msg,qrysrvc_QS_WMSisNotRunning_);
		break;
	case qrysrvc_QS_WMSFileSystemError_detail_:
		strcat(msg,qrysrvc_QS_WMSFileSystemError_);
		break;
	case qrysrvc_QS_GetServiceNotApplicabel_detail_:
		strcat(msg,qrysrvc_QS_GetServiceNotApplicabel_);
		break;
	case qrysrvc_QS_SetServiceNotApplicabel_detail_:
		strcat(msg,qrysrvc_QS_SetServiceNotApplicabel_);
		break;
	case qrysrvc_QS_WaitQueryTimeout_detail_:
		strcat(msg,qrysrvc_WaitQueryTimeout_);
		break;
	case qrysrvc_QS_HoldQueryTimeout_detail_:
		strcat(msg,qrysrvc_HoldQueryTimeout_);
		break;
	case qrysrvc_QS_SetServiceEnvError_detail_:
		strcat(msg,qrysrvc_QS_SetServiceEnvError_);
		break;
	case qrysrvc_QS_IncorrectVersion_detail_:
		strcat(msg,qrysrvc_QS_IncorrectVersion_);
		break;
	case qrysrvc_QS_BINSEM_LOCK_error_detail_:
		strcat(msg,qrysrvc_QS_BINSEM_LOCK_error_);
		error = exception_->exception_error;
		break;
	case qrysrvc_QS_BINSEM_UNLOCK_error_detail_:
		strcat(msg,qrysrvc_QS_BINSEM_UNLOCK_error_);
		error = exception_->exception_error;
		break;
	case qrysrvc_QS_RejectedCompRule_error_detail_:
		strcat(msg,qrysrvc_QS_RejectedCompRule_error_);
		error = exception_->exception_error;
		break;
	case qrysrvc_QS_SQL_CMD_error_detail_:
		strcat(msg,qrysrvc_QS_SQL_CMD_error_error_);
		error = exception_->exception_error;
		break;
	case qrysrvc_QS_IncorrectQueryState_detail_:
		strcat(msg,qrysrvc_QS_IncorrectQueryState_);
		error = exception_->exception_error;
		break;
	case qrysrvc_QS_QueryNotFound_detail_:
		strcat(msg,qrysrvc_QS_QueryNotFound_);
		error = exception_->exception_error;
		break;
	case qrysrvc_QS_MemoryAllocFailed_detail_:
		strcat(msg,qrysrvc_QS_MemoryAllocFailed_);
		break;
	case qrysrvc_QS_Query_Canceled_detail_:
		strcat(msg,qrysrvc_QS_Query_Canceled_);
		break;
	case qrysrvc_QS_Query_RejectAborting_detail_:
		strcat(msg,qrysrvc_QS_Query_RejectedAbt_);
		break;


	default:
		sprintf(&msg[strlen(msg)],"Unknown exception detail %d",exception_->exception_detail);
		break;
	}

	if (error != 0)
		sprintf(&msg[strlen(msg)],", error = %d",error);

	len = strlen(msg);
	csize -= len;

	if (csize > 0)
	{
		if (exception_->exception_txt[0] != 0)
			snprintf(msg + len, csize,", %s",exception_->exception_txt);
	}

	return msg;
}

inline void BuildWmsErrorMessage(char* msg, short msg_size, char* header, long exception_detail,  char* errorTxt)
{
	short csize = msg_size;
	short len = 0;
	short tlen = 0;

	len = snprintf(msg, csize, " [WMS]Error: %s", header);
	csize -= len;
	tlen += len;

	if (csize > 0)
	{
		if (exception_detail != 0)
		{
			len = snprintf(msg + tlen, csize, ", %d",exception_detail);
			csize -= len;
			tlen += len;
		}
	}
	if (csize > 0)
	{
		if (errorTxt[0] != 0)
		{
			len = snprintf(msg + tlen, csize,", %s",errorTxt);
			csize -= len;
			tlen += len;
		}
	}
}


#endif

