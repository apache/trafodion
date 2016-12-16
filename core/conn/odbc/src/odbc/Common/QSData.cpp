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

#include <string>
#include <sstream>
using namespace std;

#include <platform_ndcs.h>
#include <platform_utils.h>
#include "sqlcli.h"
#include "Global.h"
#include "QSGlobal.h"
#include "QSSharedSegment.h"
#include "QSData.h"

//convert UCT to Local Time in u-seconds using Julian timestamp
int64 utc2lct_useconds_jts(const int64 utcTimestamp)
{
	short error;
	return CONVERTTIMESTAMP(JULIANTIMESTAMP(), 0, -1, &error);
}

//convert Local Time to UCT in u-seconds using Julian timestamp
int64  lct2utc_useconds_jts(const int64 lctTimestamp)
{
	short error;
	return CONVERTTIMESTAMP(lctTimestamp, 2, -1, &error);
}

char* GetNameOfPrimarySegment()
{
	static char segmentname[256] = {0};
	short rc;
	int f_segment;

	segmentname[0] = 0;

	rc = msg_mon_get_process_info((char *)SYNC_PROCESS_NAME, &f_segment,NULL);
	if (rc == 0)
	{
		MS_Mon_Node_Info_Type info;
		rc = msg_mon_get_node_info_detail(f_segment,&info);
		if (rc == 0)
			memcpy(segmentname, info.node[0].node_name, sizeof(segmentname));
	}
	return segmentname;
}

int GetSetSegment(int& my_segment, bool bSetGet)
{
	static int m_segment = -1;
	static int f_segment = -1;

	if (f_segment == -1)
	{
		short rc;
		char segmentname[256];
		short maxlen = 255;
		short length;

		msg_mon_get_process_info((char *)SYNC_PROCESS_NAME, &f_segment,NULL);
	}
 
	if (bSetGet)		//Set 
	{
		if (my_segment != -1)
			m_segment = my_segment;
	}
	else
		my_segment = m_segment;

	return f_segment;
}

int64* pTimestampDelay;

//LCOV_EXCL_START
void debugTimestamp()
{
	short error;
	int64 utc = JULIANTIMESTAMP();
	int64 lct = CONVERTTIMESTAMP(utc, 0, -1, &error);
	return;
}
//LCOV_EXCL_STOP

int64 QSTIMESTAMP()
{
	short error;
	//debugTimestamp();
	int64 gmtTime = JULIANTIMESTAMP() - (pTimestampDelay ? *pTimestampDelay : 0);
	int64 result = CONVERTTIMESTAMP(gmtTime,
					0,/* direction of conversion, see Guardian Proc Calls Ref Manual */
					-1,/* Node number, -1 indicates current node */
					&error);
	return result;
}

int64 LQSTIMESTAMP()
{
	short error;
	int64 gmtTime = JULIANTIMESTAMP();
	int64 result  =  CONVERTTIMESTAMP(gmtTime, 0, -1, &error);
	return result;
}

// SQ_SP2_MERGE - +++ Need porting forfirst_segment 
void SET_QSTIMESTAMP()
{
	static int first_segment=-1;
	static int my_segment=-1;
	short error;

	int64 delay_time = 0;
	int64 time_before;
	int64 time_after;
	int64 remote_time;
	int64 local_time;

	short tuid = 0;
	if (pTimestampDelay)
	{
		if (first_segment == -1 && my_segment == -1)
			first_segment = GetSetSegment(my_segment);

		if (first_segment != -1 && my_segment != -1)
		{
			if (first_segment != my_segment)
			{
				time_before = JULIANTIMESTAMP();
				remote_time = JULIANTIMESTAMP(0,
							&tuid,
							&error,
							first_segment);

				if (error == 0)
				{
					time_after = JULIANTIMESTAMP();
					local_time = (time_after + time_before)/2;
					delay_time = local_time - remote_time;
				}
			}
		}
		*pTimestampDelay = delay_time;
	}
}


string getTcpState(int tcpPort, int& tcpState)
{
	char tmpl_netstat[] = "netstat -n -t 2>/dev/null| grep :%d ";
	char request[50];
	char buffer[500];

//	Proto Recv-Q Send-Q Local Address               Foreign Address             State
//  3-----6------6------27--------------------------27--------------------------11
//  123   123456 123456 123456789012345678901234567 123456789012345678901234567 12345678901

	char Proto[7];
	long Recv_Q;
	long Send_Q;
	char LocalAddress[40];
	char ForeignAddress[40];
	char State[20];
	int state;

	sprintf(request, tmpl_netstat, tcpPort);

	tcpState = TCP_STATE_CLOSED;

	FILE   *fp;

	if ((fp = popen( request, "r" )) != NULL)
	{
 		while( !feof( fp )  )
		{
			if( fgets( buffer, sizeof(buffer), fp ) != NULL )
			{
				if (memcmp(buffer,"tcp",3) != 0) continue;

				sscanf(buffer, "%s %d %d %s %s %s", Proto, &Recv_Q, &Send_Q, LocalAddress, ForeignAddress, State);

				if (strcmp( State, "ESTABLISHED" ) == 0)	state = TCP_STATE_ESTAB;
				else if (strcmp( State, "SYN_SENT" ) == 0)	state = TCP_STATE_SYNC_SENT;
				else if (strcmp( State, "SYN_RECV" ) == 0)	state = TCP_STATE_SYNC_RECV;
				else if (strcmp( State, "FIN_WAIT1" ) == 0)	state = TCP_STATE_FIN_WAIT_1;
				else if (strcmp( State, "FIN_WAIT2" ) == 0)	state = TCP_STATE_FIN_WAIT_2;
				else if (strcmp( State, "TIME_WAIT" ) == 0)	state = TCP_STATE_TIME_WAIT;
				else if (strcmp( State, "CLOSED" ) == 0)	state = TCP_STATE_CLOSED;
				else if (strcmp( State, "CLOSE_WAIT" ) == 0)state = TCP_STATE_CLOSE_WAIT;
				else if (strcmp( State, "LAST_ACK" ) == 0)	state = TCP_STATE_LAST_ACK;
				else if (strcmp( State, "LISTEN" ) == 0)	state = TCP_STATE_LISTEN;
				else if (strcmp( State, "CLOSING" ) == 0)	state = TCP_STATE_CLOSING;
				else if (strcmp( State, "UNKNOWN" ) == 0)	state = TCP_STATE_UNKNOWN;
				else state = TCP_STATE_UNKNOWN;

				switch (state)
				{
				case TCP_STATE_ESTAB:
					tcpState = TCP_STATE_ESTAB;
					break;
				default:
					if (tcpState != TCP_STATE_ESTAB)
						tcpState = state;
				}
			}
		}
		pclose(fp);
	}

	switch(tcpState)
	{

	case TCP_STATE_ESTAB:		return FMT_TCP_STATE_ESTAB; 
	case TCP_STATE_SYNC_SENT:	return FMT_TCP_STATE_SYNC_SENT; 
	case TCP_STATE_SYNC_RECV:	return FMT_TCP_STATE_SYNC_RECV; 
	case TCP_STATE_FIN_WAIT_1:	return FMT_TCP_STATE_FIN_WAIT_1; 
	case TCP_STATE_FIN_WAIT_2:	return FMT_TCP_STATE_FIN_WAIT_2; 
	case TCP_STATE_TIME_WAIT:	return FMT_TCP_STATE_TIME_WAIT; 
	case TCP_STATE_CLOSED:		return FMT_TCP_STATE_CLOSED; 
	case TCP_STATE_CLOSE_WAIT:	return FMT_TCP_STATE_CLOSE_WAIT; 
	case TCP_STATE_LAST_ACK:	return FMT_TCP_STATE_LAST_ACK; 
	case TCP_STATE_LISTEN:		return FMT_TCP_STATE_LISTEN; 
	case TCP_STATE_CLOSING:		return FMT_TCP_STATE_CLOSING; 
	case TCP_STATE_UNKNOWN:		return string("UNKNOWN");
	}

	return string(State);
}

string getQueryTypeString(long queryType)
{
	switch (queryType)
	{
	case SQL_OTHER:
		return "SQL_OTHER";
	case SQL_UNKNOWN:
		return "SQL_UNKNOWN";
	case SQL_SELECT_UNIQUE:
		return "SQL_SELECT_UNIQUE";
	case SQL_SELECT_NON_UNIQUE:
		return "SQL_SELECT_NON_UNIQUE";
	case SQL_INSERT_UNIQUE:
		return "SQL_INSERT_UNIQUE";
	case SQL_INSERT_NON_UNIQUE:
		return "SQL_INSERT_NON_UNIQUE";
	case SQL_UPDATE_UNIQUE:
		return "SQL_UPDATE_UNIQUE";
	case SQL_UPDATE_NON_UNIQUE:
		return "SQL_UPDATE_NON_UNIQUE";
	case SQL_DELETE_UNIQUE:
		return "SQL_DELETE_UNIQUE";
	case SQL_DELETE_NON_UNIQUE:
		return "SQL_DELETE_NON_UNIQUE";
	case SQL_CONTROL:
		return "SQL_CONTROL";
	case SQL_SET_TRANSACTION:
		return "SQL_SET_TRANSACTION";
	case SQL_SET_CATALOG:
		return "SQL_SET_CATALOG";
	case SQL_SET_SCHEMA:
		return "SQL_SET_SCHEMA";
	case SQL_CALL_NO_RESULT_SETS:
		return "SQL_CALL_NO_RESULT_SETS";
	case SQL_CALL_WITH_RESULT_SETS:
		return "SQL_CALL_WITH_RESULT_SETS";
	case SQL_SP_RESULT_SET:
		return "SQL_SP_RESULT_SET";
	case SQL_INSERT_RWRS:
		return "SQL_INSERT_RWRS";
	case SQL_CAT_UTIL:
		return "SQL_CAT_UTIL";
	case SQL_EXE_UTIL:
		return "SQL_EXE_UTIL";
	case SQL_SELECT_UNLOAD:
		return "SQL_SELECT_UNLOAD";
	default:
		return "SQL_UNKNOWN";
  }
} //getQueryTypeString

string getSQLStateString(unsigned long state)
{
	switch(state)
	{
	case SQLSTMT_STATE_INITIAL:			
		return FMT_INITIAL;
	case SQLSTMT_STATE_OPEN:			
		return FMT_OPEN;
	case SQLSTMT_STATE_EOF:				
		return FMT_EOF;
	case SQLSTMT_STATE_CLOSE:			
		return FMT_CLOSE;
	case SQLSTMT_STATE_DEALLOCATED:		
		return FMT_DEALLOCATED;
	case SQLSTMT_STATE_FETCH:			
		return FMT_FETCH;
	case SQLSTMT_STATE_CLOSE_TABLES:	
		return FMT_CLOSE_TABLES;
	case SQLSTMT_STATE_PREPARE:			
		return FMT_PREPARE;
	case SQLSTMT_STATE_PROCESS_ENDED:	
		return FMT_PROCESS_ENDED;

	default:
		return FMT_UNKNOWN; 
	}
}

string getQueryStateString(const QUERY_STATE state)
{
	switch(state)
	{
	case QUERY_INIT:
		return FMT_INIT; 
		break;
	case QUERY_WAITING:
	case QUERY_WAITING_MAX_CPU_BUSY:
	case QUERY_WAITING_MAX_MEM_USAGE:
	case QUERY_WAITING_RELEASED_BY_ADMIN:
//	case QUERY_WAITING_MAX_CUR_EXEC_QUERIES:
	case QUERY_WAITING_MAX_SERVICE_EXEC_QUERIES:
	case QUERY_WAITING_MAX_INSTANCE_EXEC_QUERIES:
	case QUERY_WAITING_TXN_BACKOUT:
	case QUERY_WAITING_MAX_SSD_USAGE:	//ssd overflow
	case QUERY_WAITING_MAX_ESPS:
	case QUERY_WAITING_CANARY_EXEC:
	case QUERY_WAITING_EST_MAX_CPU_BUSY:
		return FMT_WAITING; 
		break;
	case QUERY_EXECUTING:
	case QUERY_EXECUTING_RELEASED_BY_ADMIN:
	case QUERY_EXECUTING_RELEASED_BY_RULE:
	case QUERY_EXECUTING_CANCEL_IN_PROGRESS:
	case QUERY_EXECUTING_CANCEL_FAILED:
	case QUERY_EXECUTING_CANCEL_FAILED_8026:
	case QUERY_EXECUTING_CANCEL_FAILED_8027:
	case QUERY_EXECUTING_CANCEL_FAILED_8028:
	case QUERY_EXECUTING_CANCEL_FAILED_8029:
	case QUERY_EXECUTING_CANCEL_FAILED_8031:
		return FMT_EXECUTING; 
		break;
	case QUERY_HOLDING:
	case QUERY_HOLDING_LOAD:
	case QUERY_HOLDING_REPREPARING:
	case QUERY_HOLDING_EXECUTING_SQL_CMD:
	case QUERY_HOLDING_BY_RULE:
	case QUERY_HOLDING_BY_ADMIN:
		return FMT_HOLDING; 
		break;
	case QUERY_COMPLETED:
	case QUERY_COMPLETED_HOLD_TIMEOUT:
	case QUERY_COMPLETED_EXEC_TIMEOUT:
	case QUERY_COMPLETED_BY_ADMIN:
	case QUERY_COMPLETED_QUERY_NOT_FOUND:
	case QUERY_COMPLETED_CONNECTION_FAILED:
	case QUERY_COMPLETED_NDCS_PROCESS_FAILED:
	case QUERY_COMPLETED_CPU_FAILED:
	case QUERY_COMPLETED_SEGMENT_FAILED:
	case QUERY_COMPLETED_BY_RULE:
	case QUERY_COMPLETED_SERVICE_NOT_ACTIVE:
	case QUERY_COMPLETED_HARDWARE_FAILURE:
	case QUERY_COMPLETED_UNEXPECTED_STATE:
	case QUERY_COMPLETED_CLIENT_DISAPPEARED:
	case QUERY_COMPLETED_BY_CLIENT:
	case QUERY_COMPLETED_NDCS_DLG_INIT:
	case QUERY_COMPLETED_NDCS_CONN_IDLE:
	case QUERY_COMPLETED_NDCS_DLG_TERM:
	case QUERY_COMPLETED_NDCS_DLG_BREAK:
	case QUERY_COMPLETED_NDCS_STOP_SRVR:
	case QUERY_COMPLETED_NDCS_RMS_ERROR:
	case QUERY_COMPLETED_NDCS_REPOS_IDLE:
	case QUERY_COMPLETED_NDCS_REPOS_INTERVAL:
	case QUERY_COMPLETED_NDCS_REPOS_PARTIAL:
	case QUERY_COMPLETED_NDCS_EXEC_INTERVAL:
	case QUERY_COMPLETED_NDCS_CONN_RULE_CHANGED:
	case QUERY_COMPLETED_NDCS_CLOSE:
	case QUERY_COMPLETED_NDCS_PREPARE:
	case QUERY_COMPLETED_NDCS_WMS_ERROR:
	case QUERY_COMPLETED_BY_ADMIN_SERVER:
		return FMT_COMPLETED; 
		break;
	case QUERY_REJECTED:
	case QUERY_REJECTED_BY_ADMIN:
	case QUERY_REJECTED_CONNECTION_FAILED:
	case QUERY_REJECTED_NDCS_PROCESS_FAILED:
	case QUERY_REJECTED_CPU_FAILED:
	case QUERY_REJECTED_SEGMENT_FAILED:
	case QUERY_REJECTED_QMSGCANCELLED:
	case QUERY_REJECTED_VERSION_MISMATCH:
	case QUERY_REJECTED_WMSONHOLD:
	case QUERY_REJECTED_MAX_QUERIES_REACHED:
	case QUERY_REJECTED_SERVICE_NOT_FOUND:
	case QUERY_REJECTED_SERVICE_ON_HOLD:
	case QUERY_REJECTED_BY_RULE:
	case QUERY_REJECTED_UNKNOWNUSER:
	case QUERY_REJECTED_UNEXPECTED_STATE:
	case QUERY_REJECTED_HOLD_TIMEOUT:
	case QUERY_REJECTED_WAIT_TIMEOUT:
	case QUERY_REJECTED_CLIENT_DISAPPEARED:
	case QUERY_REJECTED_LONG_TRANS_ABORTING:
		return FMT_REJECTED; 
		break;
	case QUERY_SUSPENDED:
	case QUERY_SUSPENDED_BY_ADMIN:
	case QUERY_SUSPENDED_BY_RULE:
	case QUERY_SUSPENDED_CANCELED:
	case QUERY_SUSPENDED_CANCELED_BY_ADMIN:
	case QUERY_SUSPENDED_CANCELED_BY_RULE:
	case QUERY_SUSPENDED_CANCELED_BY_TIMEOUT:
		return FMT_SUSPENDED;
		break;
	default:
		return FMT_UNKNOWN; 
		break;
	}
}//end getQueryStateString

//================================================================

string getQuerySubStateString(const QUERY_STATE state)
{
	switch(state)
	{
	case QUERY_WAITING_MAX_CPU_BUSY:			return FMT_MAX_CPU_BUSY;
	case QUERY_WAITING_EST_MAX_CPU_BUSY:		return FMT_EST_MAX_CPU_BUSY;
//
	case QUERY_WAITING_MAX_MEM_USAGE:			return FMT_MAX_MEM_USAGE;
//
	case QUERY_WAITING_RELEASED_BY_ADMIN:
	case QUERY_EXECUTING_RELEASED_BY_ADMIN:		return FMT_RELEASED_BY_ADMIN;
//
	case QUERY_EXECUTING_CANCEL_IN_PROGRESS:	return FMT_CANCEL_IN_PROGRESS;
	case QUERY_EXECUTING_CANCEL_FAILED:			return FMT_CANCEL_FAILED;
	case QUERY_EXECUTING_CANCEL_FAILED_8026:	return FMT_CANCEL_FAILED_8026;
	case QUERY_EXECUTING_CANCEL_FAILED_8027:	return FMT_CANCEL_FAILED_8027;
	case QUERY_EXECUTING_CANCEL_FAILED_8028:	return FMT_CANCEL_FAILED_8028;
	case QUERY_EXECUTING_CANCEL_FAILED_8029:	return FMT_CANCEL_FAILED_8029;
	case QUERY_EXECUTING_CANCEL_FAILED_8031:	return FMT_CANCEL_FAILED_8031;
//
	case QUERY_EXECUTING_RELEASED_BY_RULE:		return FMT_RELEASED_BY_EXEC_RULE;
//
	case QUERY_WAITING_MAX_SERVICE_EXEC_QUERIES:    return FMT_MAX_SERVICE_EXEC_QUERIES;
	case QUERY_WAITING_MAX_INSTANCE_EXEC_QUERIES:   return FMT_MAX_INSTANCE_EXEC_QUERIES;
//
	case QUERY_WAITING_TXN_BACKOUT:				return FMT_WAITING_TXN_BACKOUT;
	case QUERY_WAITING_MAX_SSD_USAGE:			return FMT_MAX_SSD_USAGE;	//ssd overflow
	case QUERY_WAITING_MAX_ESPS:				return FMT_MAX_AVG_ESPS;
	case QUERY_WAITING_CANARY_EXEC:             return FMT_WAITING_CANARY;
//
	case QUERY_HOLDING_LOAD:					return FMT_LOADING;
//
	case QUERY_HOLDING_REPREPARING:				return FMT_REPREPARING;
//
	case QUERY_HOLDING_EXECUTING_SQL_CMD:		return FMT_EXECUTING_SQL_CMD;
//
	case QUERY_REJECTED_BY_RULE:
	case QUERY_HOLDING_BY_RULE:					return FMT_BY_COMP_RULE;
//
	case QUERY_SUSPENDED_BY_RULE:				return FMT_BY_EXEC_RULE;
//
	case QUERY_SUSPENDED_BY_ADMIN:
	case QUERY_REJECTED_BY_ADMIN:
	case QUERY_HOLDING_BY_ADMIN:				return FMT_BY_ADMIN;
//
	case QUERY_REJECTED_HOLD_TIMEOUT:
	case QUERY_COMPLETED_HOLD_TIMEOUT:			return FMT_HOLD_TIMEOUT;
//
	case QUERY_COMPLETED_EXEC_TIMEOUT:			return FMT_EXEC_TIMEOUT;
//
	case QUERY_COMPLETED_BY_ADMIN:				return FMT_CANCELLED_BY_ADMIN;
	case QUERY_COMPLETED_BY_ADMIN_SERVER:       return FMT_CANCELLED_BY_ADMIN_SERVER;
	case QUERY_COMPLETED_BY_CLIENT:				return FMT_CANCELLED_BY_CLIENT;
//
	case QUERY_COMPLETED_QUERY_NOT_FOUND:		return FMT_QUERY_NOT_FOUND;
//
	case QUERY_REJECTED_CONNECTION_FAILED:
	case QUERY_COMPLETED_CONNECTION_FAILED:		return FMT_CONNECTION_FAILED;
//
	case QUERY_REJECTED_NDCS_PROCESS_FAILED:
	case QUERY_COMPLETED_NDCS_PROCESS_FAILED:	return FMT_NDCS_PROCESS_FAILED;
//
	case QUERY_REJECTED_CPU_FAILED:
	case QUERY_COMPLETED_CPU_FAILED:			return FMT_CPU_FAILED; 
//
	case QUERY_REJECTED_SEGMENT_FAILED:
	case QUERY_COMPLETED_SEGMENT_FAILED:		return FMT_SEGMENT_FAILED; 
//
	case QUERY_COMPLETED_BY_RULE:				return FMT_BY_EXEC_RULE;
//
	case QUERY_COMPLETED_SERVICE_NOT_ACTIVE:	return FMT_SERVICE_NOT_ACTIVE;
//
	case QUERY_REJECTED_UNEXPECTED_STATE:
	case QUERY_COMPLETED_UNEXPECTED_STATE:		return FMT_UNEXPECTED_STATE; 
//
	case QUERY_REJECTED_CLIENT_DISAPPEARED:
	case QUERY_COMPLETED_CLIENT_DISAPPEARED:	return FMT_CLIENT_DISAPPEARED;
//
	case QUERY_REJECTED_QMSGCANCELLED:			return FMT_QUEUE_MSG_CANCELLED;
//
	case QUERY_REJECTED_VERSION_MISMATCH:		return FMT_VERSION_MISMATCH;
//
	case QUERY_REJECTED_WMSONHOLD:				return FMT_WMS_ON_HOLD;
//
	case QUERY_REJECTED_MAX_QUERIES_REACHED:	return FMT_MAX_QUERIES_REACHED;
	
	case QUERY_REJECTED_LONG_TRANS_ABORTING:	return FMT_LONG_TRANS_ABORTING;
//
	case QUERY_REJECTED_SERVICE_NOT_FOUND:		return FMT_SERVICE_NOT_FOUND;
//
	case QUERY_REJECTED_SERVICE_ON_HOLD:		return FMT_SERVICE_ON_HOLD;
//
	case QUERY_REJECTED_UNKNOWNUSER:			return FMT_UNKNOWN_USER;
//
	case QUERY_REJECTED_WAIT_TIMEOUT:			return FMT_WAIT_TIMEOUT;
//
	case QUERY_SUSPENDED_CANCELED:				return FMT_QUERY_CANCELED;
	case QUERY_SUSPENDED_CANCELED_BY_RULE:		return FMT_QUERY_CANCELED_BY_RULE;
	case QUERY_SUSPENDED_CANCELED_BY_ADMIN:		return FMT_QUERY_CANCELED_BY_ADMIN;
	case QUERY_SUSPENDED_CANCELED_BY_TIMEOUT:	return FMT_QUERY_CANCELED_BY_TIMEOUT;
//
	case QUERY_COMPLETED_NDCS_DLG_INIT:			return FMT_NDCS_DLG_INIT;
	case QUERY_COMPLETED_NDCS_CONN_IDLE:		return FMT_NDCS_CONN_IDLE;
	case QUERY_COMPLETED_NDCS_DLG_TERM:			return FMT_NDCS_DLG_TERM;
	case QUERY_COMPLETED_NDCS_DLG_BREAK:		return FMT_NDCS_DLG_BREAK;
	case QUERY_COMPLETED_NDCS_STOP_SRVR:		return FMT_NDCS_STOP_SRVR;
	case QUERY_COMPLETED_NDCS_RMS_ERROR:		return FMT_NDCS_RMS_ERROR;
	case QUERY_COMPLETED_NDCS_REPOS_IDLE:		return FMT_NDCS_REPOS_IDLE;
	case QUERY_COMPLETED_NDCS_REPOS_INTERVAL:	return FMT_NDCS_REPOS_INTERVAL;
	case QUERY_COMPLETED_NDCS_REPOS_PARTIAL:	return FMT_NDCS_REPOS_PARTIAL;
	case QUERY_COMPLETED_NDCS_EXEC_INTERVAL:	return FMT_NDCS_EXEC_INTERVAL;
	case QUERY_COMPLETED_NDCS_CONN_RULE_CHANGED:	return FMT_NDCS_CONN_RULE_CHANGED;
	case QUERY_COMPLETED_NDCS_CLOSE:			return FMT_NDCS_CLOSE;
	case QUERY_COMPLETED_NDCS_PREPARE:			return FMT_NDCS_PREPARE;
	case QUERY_COMPLETED_NDCS_WMS_ERROR:		return FMT_NDCS_WMS_ERROR;

	default:
		return ""; 
	}
}//getQuerySubStateString

//=================================================================================

string getWarnLevelString(unsigned short value)
{
	if(WLVL_HIGH & value)
		return FMT_WLVL_HIGH;
	else if(WLVL_MEDIUM & value)
		return FMT_WLVL_MEDIUM;
	else if(WLVL_LOW & value)
		return FMT_WLVL_LOW;
	else if(WLVL_NO_WARN & value)
		return FMT_WLVL_NO_WARN;
	else
		return ""; 
}

//=================================================================================

string getActionString(RULE_SPEC_ACT action)
{
	switch(action)
	{
	case ACT_INIT:
		return FMT_ACT_INIT;
//	case ACT_WARN:
//		return FMT_ACT_WARN;
	case CONN_EXEC:
		return FMT_CONN_EXEC;
	case COMP_EXEC:
		return FMT_COMP_EXEC;
	case COMP_EXECUTE:
		return FMT_COMP_EXECUTE;
	case COMP_REJECT:
		return FMT_COMP_REJECT;
	case COMP_HOLD:
		return FMT_COMP_HOLD;
 	case EXEC_CANCEL:
		return FMT_EXEC_CANCEL;
 	case EXEC_STATS_PERTABLE:
 		return FMT_EXEC_STATS_PERTABLE;
 	case EXEC_STATS_PERTABLE_CANCEL:
 		return FMT_EXEC_STATS_PERTABLE_CANCEL;
	case ACT_NO_WARN:
		return FMT_ACT_NO_WARN;
	default:
		return ""; 
	}
}

//=================================================================================

string getWarnReasonString(short reason)
{
	switch(reason)
	{
	case WRSNE_INIT:
		return FMT_WRSNE_INIT;
	case WRSNE_RULE:
		return FMT_WRSNE_RULE;
	case WRSNE_USED_ROWS:
		return FMT_WRSNE_USED_ROWS;
	case WRSNE_ACCESSED_ROWS:
		return FMT_WRSNE_ACCESSED_ROWS;
	case WRSNE_TOTAL_MEM_ALLOC:
		return FMT_WRSNE_TOTAL_MEM_ALLOC;
	case WRSNE_ELAPSED_TIME:
		return FMT_WRSNE_ELAPSED_TIME;
	case WRSNE_CPU_TIME:
		return FMT_WRSNE_CPU_TIME;
	case WRSNE_STATS_IDLE_TIME:
		return FMT_WRSNE_STATS_IDLE_TIME;
	case WRSNE_PROCESS_BUSY_TIME:
		return FMT_WRSNE_PROCESS_BUSY_TIME;
	default:
		return ""; 
	}
}

//=================================================================================

string getQueueStateString(const QUEUE_STATE state)
{
	switch(state)
	{
	case QUEUE_INIT:
		return FMT_INIT; 
	case QUEUE_ACTIVE:
		return FMT_ACTIVE; 
	case QUEUE_HOLD:
		return FMT_HOLD; 
	case QUEUE_QUIESCE:
		return FMT_QUIESCE;
	case QUEUE_STOPPING:
		return FMT_STOPPING; 
	case QUEUE_STOPPED:
		return FMT_STOPPED; 
	case QUEUE_TRANSIENT:
		return FMT_TRANSIENT; 
	default:
		return FMT_UNKNOWN; 
	}
}//end getQueueStateString

string getQueuePrtyString(const QUEUE_PRTY prty)
{
	switch(prty)
	{
	case 1:
		return FMT_URGENT; 
	case 2:
		return FMT_HIGH; 
	case 3:
		return FMT_MEDIUM_HIGH; 
	case 4:
		return FMT_MEDIUM; 
	case 5:
		return FMT_LOW_MEDIUM; 
	case 6:
		return FMT_LOW; 
	default:
		return FMT_UNKNOWN; 
	}
}//end getQueuePrtyString

string getGlobalStateString(const GLOBAL_STATE state)
{
	switch(state)
	{
	case GLOBAL_INIT:
		return FMT_INIT; 
	case GLOBAL_ACTIVE:
		return FMT_ACTIVE; 
	case GLOBAL_HOLD:
		return FMT_HOLD; 
	case GLOBAL_SHUTDOWN:
		return FMT_SHUTDOWN; 
	case GLOBAL_STOPPED:
		return FMT_STOPPED; 
	default:
		return FMT_UNKNOWN; 
	}
}//end getGlobalStateString

string getProcessTypeString(PROCESS_TYPE type)
{
	switch(type)
	{
	case PROCESS_INIT:
		return FMT_PROCESS_TYPE_INIT;
	case PROCESS_SRVR:
		return FMT_PROCESS_TYPE_SRVR;
	case PROCESS_CMP:
		return FMT_PROCESS_TYPE_CMP;
	case PROCESS_ESP:
		return FMT_PROCESS_TYPE_ESP;
	case PROCESS_UDR:
		return FMT_PROCESS_TYPE_UDR;
	case PROCESS_CI:
		return FMT_PROCESS_TYPE_CI;
	case PROCESS_OTHER:
		return FMT_PROCESS_TYPE_OTHER;
	case PROCESS_ALL:
		return FMT_PROCESS_TYPE_ALL;
	case PROCESS_SQL:
		return FMT_PROCESS_TYPE_SQL;
	case PROCESS_WMS:
		return FMT_PROCESS_TYPE_WMS;
	default:
		return FMT_UNKNOWN;
	}
} //end getProcessTypeString

string getAggrQueryString(const bool value)
{
	string result;

	(value==true)? result = "YES": result = "NO";

	return result;
}//end getGlobalStateString

QUERY_STATE convertMXSubStateToState(NDCS_SUBSTATE mxsrvr_substate)
{
	switch(mxsrvr_substate)
	{
	case NDCS_DLG_INIT:		return QUERY_COMPLETED_NDCS_DLG_INIT;
	case NDCS_CONN_IDLE:	return QUERY_COMPLETED_NDCS_CONN_IDLE;
	case NDCS_DLG_TERM:		return QUERY_COMPLETED_NDCS_DLG_TERM;
	case NDCS_DLG_BREAK:	return QUERY_COMPLETED_NDCS_DLG_BREAK;
	case NDCS_STOP_SRVR:	return QUERY_COMPLETED_NDCS_STOP_SRVR;
	case NDCS_RMS_ERROR:	return QUERY_COMPLETED_NDCS_RMS_ERROR;
	case NDCS_REPOS_IDLE:	return QUERY_COMPLETED_NDCS_REPOS_IDLE;
	case NDCS_REPOS_INTERVAL:		return QUERY_COMPLETED_NDCS_REPOS_INTERVAL;
	case NDCS_REPOS_PARTIAL: return QUERY_COMPLETED_NDCS_REPOS_PARTIAL;
	case NDCS_EXEC_INTERVAL: return QUERY_COMPLETED_NDCS_EXEC_INTERVAL;
	case NDCS_CONN_RULE_CHANGED:	return QUERY_COMPLETED_NDCS_CONN_RULE_CHANGED;
	case NDCS_CLOSE:		return QUERY_COMPLETED_NDCS_CLOSE;
	case NDCS_PREPARE:		return QUERY_COMPLETED_NDCS_PREPARE;
	case NDCS_WMS_ERROR:	return QUERY_COMPLETED_NDCS_WMS_ERROR;
	case NDCS_QUERY_CANCELED: return QUERY_EXECUTING_CANCEL_IN_PROGRESS;

	case NDCS_INIT:
	default:
		return QUERY_COMPLETED;

	}
}

bool checkPrivMask( char *privStr, bitmask_type bitMask )
{
	char *p;
	long priv = strtol(privStr, &p, 16);
	if( *p != NULL )
		return false;

	int bitIdx = priv/(sizeof(mask_type)*8);
	priv = priv - ((sizeof(mask_type)*8)*bitIdx);
	mask_type mask = bitMask[bitIdx];
	return mask & ((mask_type)1 << priv);
}

string getRuleExprString(RULE_TYPE type,
									RULE_EXPR_LOPND lopnd,
									RULE_EXPR_OPER oper,
									short operNum,
									RULE_EXPR_ROPND ropnd,
									int64 ropndNum,
									char * ropndStr)
{
	stringstream msg;

	msg << getRuleExprLopndString(lopnd);
	msg << getRuleExprOperString(type,lopnd,oper,operNum);
	msg << getRuleExprRopndString(type,lopnd,oper,ropnd,ropndNum,ropndStr);

	return msg.str();

}//end getRuleExprString

string getRuleExprLopndString(RULE_EXPR_LOPND value)
{
	stringstream ss;

	switch(value)
	{
	case LOPND_INIT:
		ss << FMT_INIT;
		break;
	/* CONN */
	case LOPND_APPL:
		ss << FMT_APPL;
		break;
	case LOPND_SESSION:
		ss << FMT_SESSION;
		break;
	case LOPND_LOGIN:
	case LOPND_LOGIN_ROLE:
	case LOPND_LOGIN_USER:
	case (LOPND_LOGIN_USER | LOPND_LOGIN_ROLE):
		ss << FMT_LOGIN;
		break;
	case LOPND_DSN:
		ss << FMT_DSN;
		break;
	/* COMP */
	case LOPND_EST_TOTAL_MEMORY:
		ss << FMT_EST_TOTAL_MEMORY;
		break;
	case LOPND_EST_TOTAL_TIME:
		ss << FMT_EST_TOTAL_TIME;
		break;
	case LOPND_EST_CARDINALITY:
		ss << FMT_EST_CARDINALITY;
		break;
	case LOPND_EST_ACCESSED_ROWS:
		ss << FMT_EST_ACCESSED_ROWS;
		break;
	case LOPND_EST_USED_ROWS:
		ss << FMT_EST_USED_ROWS;
		break;
	case LOPND_NUM_JOINS:
		ss << FMT_NUM_JOINS;
		break;
	case LOPND_UPDATE_STATS_WARNING:
		ss << FMT_UPDATE_STATS_WARNING;
		break;
	case LOPND_CROSS_PRODUCT:
		ss << FMT_CROSS_PRODUCT;
		break;
	case LOPND_SCAN_SIZE:
		ss << FMT_SCAN_SIZE;
		break;
	/** EXEC **/
	case LOPND_USED_ROWS:
		ss << FMT_USED_ROWS;
		break;
	case LOPND_ACCESSED_ROWS:
		ss << FMT_ACCESSED_ROWS;
		break;
	case LOPND_TOTAL_MEM_ALLOC:
		ss << FMT_TOTAL_MEM_ALLOC;
		break;
	case LOPND_ELAPSED_TIME:
		ss << FMT_ELAPSED_TIME;
		break;
	case LOPND_CPU_TIME:
		ss << FMT_CPU_TIME;
		break;
	case LOPND_PROCESS_BUSY_TIME:
		ss << FMT_PROCESS_BUSY_TIME;
		break;
	case LOPND_STATS_IDLE_TIME:
		ss << FMT_STATS_IDLE_TIME;
		break;
	}

	return ss.str();

}//end getRuleExprLopndString

string getRuleExprOperString(RULE_TYPE type,
										RULE_EXPR_LOPND lopnd,
										RULE_EXPR_OPER oper,
										short operNum)
{
	stringstream ss;

	if(type == RLTYPE_COMP)
	{
		switch(lopnd)
		{
		case LOPND_UPDATE_STATS_WARNING:
		case LOPND_CROSS_PRODUCT:
			return "";
		}
	}

	ss << " ";

	switch(oper)
	{
	case OPER_NOP:
		ss << FMT_NOP;
		break;
	case OPER_EQ:
		ss << FMT_EQ;
		break;
	case OPER_GE:
		ss << FMT_GE;
		break;
	case OPER_GT:
		ss << FMT_GT;
		break;
	case OPER_LT:
		ss << FMT_LT;
		break;
	case OPER_LE:
		ss << FMT_LE;
		break;
	case OPER_NE:
		ss << FMT_NE;
		break;
	case OPER_PR:
		ss << operNum << " " << FMT_PCT;
		break;
	case OPER_SC:
		ss << FMT_SC;
		break;
	case OPER_IC:
		ss << FMT_ICASE;
		break;
	}

	return ss.str();

}//end getRuleExprOperString

string getRuleExprRopndString(RULE_TYPE type,
							  RULE_EXPR_LOPND lopnd,
							  RULE_EXPR_OPER oper,
							  RULE_EXPR_ROPND ropnd,
							  int64 ropndNum,
							  char * ropndStr)
{
	stringstream ss;

	//if CONN expr return the string value
	if(type == RLTYPE_CONN)
	{
		ss << " ";
		if(oper == OPER_IC)
		{
			ss << FMT_OPEN_PAREN;
			ss << ropndStr;
			ss << FMT_CLOSE_PAREN;
		}
		else
		{
			ss << ropndStr;
		}

		return ss.str();
	}

	if(type == RLTYPE_COMP)
	{
		switch(lopnd)
		{
		case LOPND_UPDATE_STATS_WARNING:
		case LOPND_CROSS_PRODUCT:
			return "";
		}
	}

	ss << " ";

	switch(ropnd)
	{
	case ROPND_NUMBER:
		char out[20];
		sprintf(out,"%Ld",ropndNum);
		ss << out;
		break;
	case ROPND_EST_USED_ROWS:
		ss << FMT_EST_USED_ROWS;
		break;
	case ROPND_EST_ACCESSED_ROWS:
		ss << FMT_EST_ACCESSED_ROWS;
		break;
	case ROPND_EST_TOTAL_MEMORY:
		ss << FMT_EST_TOTAL_MEMORY;
		break;
	case ROPND_EST_CPU_TIME:
		ss << FMT_EST_CPU_TIME;
		break;
	case ROPND_EST_IO_TIME:
		ss << FMT_EST_IO_TIME;
		break;
	}

	return ss.str();

}//end getRuleExprRopndString

string getRuleTypeString(RULE_TYPE value)
{
	switch(value)
	{
	case RLTYPE_CONN:
		return FMT_CONN;
	case RLTYPE_COMP:
		return FMT_COMP;
	case RLTYPE_EXEC:
		return FMT_EXEC;
	default:
		return FMT_UNKN;
	}
}//end getRuleTypeString

string getRuleOperString(RULE_SPEC_OPER value)
{
	switch(value)
	{
	case RSOPR_NOP:
		return FMT_AND;
	case RSOPR_OR:
		return FMT_OR;
	case RSOPR_AND:
		return FMT_AND;
	default:
		return FMT_AND;
	}
}//end getRuleOperString

string getRuleActionString(RULE_SPEC_ACT value,char *sqlCmds)
{
	switch(value)
	{
//	case ACT_WARN:
//		return FMT_WARN;
	case ACT_NO_WARN:
		return FMT_NO_WARN;
	case CONN_EXEC:
	case COMP_EXEC:
		return sqlCmds;
	case COMP_EXECUTE:
		return FMT_COMP_EXECUTE;
	case COMP_REJECT:
		return FMT_REJECT;
	case COMP_HOLD:
		return FMT_HOLD;
	case EXEC_CANCEL:
		return FMT_CANCEL;
 	case EXEC_STATS_PERTABLE:
 		return FMT_STATS_PERTABLE;
 	case EXEC_STATS_PERTABLE_CANCEL:
 		return FMT_STATS_PERTABLE_CANCEL;
	default:
		return "";
	}
}//end getRuleActionString

string getRuleAggrQueryTypesString(short value)
{
	stringstream ss;

	bool prev = false;

	if(value & AGGR_QT_INSERT)
	{
		ss << FMT_INSERT;
		prev = true;
	}
	if(value & AGGR_QT_UPDATE)
	{
		if(prev)
			ss << ",";
		else
			prev = true;
		ss << FMT_UPDATE;
	}
	if(value & AGGR_QT_DELETE)
	{
		if(prev)
			ss << ",";
		else
			prev = true;
		ss << FMT_DELETE;
	}
	if(value & AGGR_QT_SELECT)
	{
		if(prev)
			ss << ",";
		else
			prev = true;
		ss << FMT_SELECT;
	}

	return ss.str();

}//end getRuleAggrQueryTypesString

string getRuleAggrStatsOnceString(short value)
{
	if(value)
		return FMT_ON;
	else
		return FMT_OFF;
}//end getRuleAggrStatsOnceString

string getRuleWarnLevelString(int value)
{
	if(WLVL_HIGH & value)
		return FMT_HIGH;
	else if(WLVL_MEDIUM & value)
		return FMT_MEDIUM;
	else if(WLVL_LOW & value)
		return FMT_LOW;
	else if(WLVL_NO_WARN & value)
		return FMT_NO_WARN;
	else
		return "";

}//end getRuleWarnLevelString

string getExeErrorCodeString(ExeErrorCode value)
{
	switch(value)
	{
	case EXE_FIRST_ERROR: 							return FMT_EXE_FIRST_ERROR;
	case EXE_INTERNAL_ERROR: 						return FMT_EXE_INTERNAL_ERROR;
	case EXE_NOWAIT_OP_INCOMPLETE: 					return FMT_EXE_NOWAIT_OP_INCOMPLETE;
	case EXE_OUTPUT_DESCRIPTOR_LOCKED: 				return FMT_EXE_OUTPUT_DESCRIPTOR_LOCKED;
	case EXE_CURSOR_ALREADY_OPEN: 					return FMT_EXE_CURSOR_ALREADY_OPEN;
	case EXE_CURSOR_NOT_OPEN: 						return FMT_EXE_CURSOR_NOT_OPEN;
	case EXE_STREAM_TIMEOUT: 						return FMT_EXE_STREAM_TIMEOUT;
	case EXE_CANCELED: 								return FMT_EXE_CANCELED;
	case EXE_INVALID_CAT_NAME: 						return FMT_EXE_INVALID_CAT_NAME;
	case EXE_INVALID_SCH_NAME: 						return FMT_EXE_INVALID_SCH_NAME;
	case EXE_INFO_DEFAULT_CAT_SCH: 					return FMT_EXE_INFO_DEFAULT_CAT_SCH;
	case EXE_BLOCK_CARDINALITY_VIOLATION: 			return FMT_EXE_BLOCK_CARDINALITY_VIOLATION;
	case EXE_INFO_CQD_NAME_VALUE_PAIRS: 			return FMT_EXE_INFO_CQD_NAME_VALUE_PAIRS;
	case EXE_CURSOR_NOT_FETCHED: 					return FMT_EXE_CURSOR_NOT_FETCHED;
	case EXE_CS_EOD: 								return FMT_EXE_CS_EOD;
	case EXE_CS_EOD_ROLLBACK_ERROR: 				return FMT_EXE_CS_EOD_ROLLBACK_ERROR;
	case EXE_VERSION_ERROR: 						return FMT_EXE_VERSION_ERROR;
	case EXE_NO_EXPLAIN_INFO: 						return FMT_EXE_NO_EXPLAIN_INFO;
	case EXE_PARTN_SKIPPED: 						return FMT_EXE_PARTN_SKIPPED;
	case EXE_EXPLAIN_BAD_DATA: 						return FMT_EXE_EXPLAIN_BAD_DATA;

	case EXE_INITIALIZE_MAINTAIN: 					return FMT_EXE_INITIALIZE_MAINTAIN;
	case EXE_PURGEDATA_CAT: 						return FMT_EXE_PURGEDATA_CAT;
	case EXE_PARALLEL_PURGEDATA_FAILED: 			return FMT_EXE_PARALLEL_PURGEDATA_FAILED;

	case EXE_QUERY_LIMITS_CPU: 						return FMT_EXE_QUERY_LIMITS_CPU;
	case EXE_QUERY_LIMITS_CPU_DEBUG: 				return FMT_EXE_QUERY_LIMITS_CPU_DEBUG;
	case EXE_QUERY_LIMITS_CPU_DP2: 					return FMT_EXE_QUERY_LIMITS_CPU_DP2;

	case EXE_CANCEL_QID_NOT_FOUND: 					return FMT_EXE_CANCEL_QID_NOT_FOUND;
	case EXE_CANCEL_TIMEOUT: 						return FMT_EXE_CANCEL_TIMEOUT;
	case EXE_CANCEL_PROCESS_NOT_FOUND: 				return FMT_EXE_CANCEL_PROCESS_NOT_FOUND;
	case EXE_CANCEL_NOT_AUTHORIZED: 				return FMT_EXE_CANCEL_NOT_AUTHORIZED;

// ---------------------------------------------------------------------
// Data integrity errors
// ---------------------------------------------------------------------
	case EXE_INVALID_DEFINE_OR_ENVVAR: 				return FMT_EXE_INVALID_DEFINE_OR_ENVVAR;
	case EXE_TABLE_CHECK_CONSTRAINT: 				return FMT_EXE_TABLE_CHECK_CONSTRAINT;
	case EXE_DUPLICATE_RECORD: 						return FMT_EXE_DUPLICATE_RECORD;
	case EXE_RI_CONSTRAINT_VIOLATION: 				return FMT_EXE_RI_CONSTRAINT_VIOLATION;
	case EXE_CHECK_OPTION_VIOLATION_CASCADED: 		return FMT_EXE_CHECK_OPTION_VIOLATION_CASCADED;
	case EXE_CHECK_OPTION_VIOLATION: 				return FMT_EXE_CHECK_OPTION_VIOLATION;
	case EXE_CURSOR_UPDATE_CONFLICT: 				return FMT_EXE_CURSOR_UPDATE_CONFLICT;
	case EXE_HALLOWEEN_INSERT_AUTOCOMMIT: 			return FMT_EXE_HALLOWEEN_INSERT_AUTOCOMMIT;
	case EXE_DUPLICATE_IDENTITY_VALUE: 				return FMT_EXE_DUPLICATE_IDENTITY_VALUE;
	case EXE_INVALID_SESSION_DEFAULT: 				return FMT_EXE_INVALID_SESSION_DEFAULT;
	case EXE_DUPLICATE_ENTIRE_RECORD: 				return FMT_EXE_DUPLICATE_ENTIRE_RECORD;
	case EXE_LAST_INTEGRITY_ERROR: 					return FMT_EXE_LAST_INTEGRITY_ERROR;

// ---------------------------------------------------------------------
// Some internal testing "errors"
// ---------------------------------------------------------------------
	case EXE_CANCEL_INJECTED: 						return FMT_EXE_CANCEL_INJECTED;
	case EXE_ERROR_INJECTED: 						return FMT_EXE_ERROR_INJECTED;

// ---------------------------------------------------------------------
// Set session default "warning".
// ---------------------------------------------------------------------
	case EXE_CLEANUP_ESP: 							return FMT_EXE_CLEANUP_ESP;

//----------------------------------------------------------------------
// Late-name resolution and late-binding/similarity check errors.
//----------------------------------------------------------------------
	case EXE_NAME_MAPPING_ERROR: 					return FMT_EXE_NAME_MAPPING_ERROR;
	case EXE_NAME_MAPPING_FS_ERROR: 				return FMT_EXE_NAME_MAPPING_FS_ERROR;
	case EXE_NAME_MAPPING_NO_PART_AVAILABLE: 		return FMT_EXE_NAME_MAPPING_NO_PART_AVAILABLE;
	case EXE_NAME_MAPPING_BAD_ANCHOR: 				return FMT_EXE_NAME_MAPPING_BAD_ANCHOR;

//----------------------------------------------------------------------
// Available for future use.
//----------------------------------------------------------------------
	case CLI_ASSIGN_INCOMPATIBLE_CHARSET: 			return FMT_ASSIGN_INCOMPATIBLE_CHARSET;

//----------------------------------------------------------------------
// Expressions errors
//----------------------------------------------------------------------
	case EXE_INVALID_DEFINE_CLASS_ERROR: 			return FMT_EXE_INVALID_DEFINE_CLASS_ERROR;
//	case EXE_CARDINALITY_VIOLATION: CLI_SELECT_INTO_ERROR
	case EXE_STRING_OVERFLOW: 						return FMT_EXE_STRING_OVERFLOW;
	case EXE_SUBSTRING_ERROR: 						return FMT_EXE_SUBSTRING_ERROR;
	case EXE_TRIM_ERROR: 							return FMT_EXE_TRIM_ERROR;
	case EXE_CONVERTTIMESTAMP_ERROR: 				return FMT_EXE_CONVERTTIMESTAMP_ERROR;
	case EXE_JULIANTIMESTAMP_ERROR: 				return FMT_EXE_JULIANTIMESTAMP_ERROR;
	case EXE_INVALID_ESCAPE_CHARACTER: 				return FMT_EXE_INVALID_ESCAPE_CHARACTER;
	case EXE_INVALID_ESCAPE_SEQUENCE: 				return FMT_EXE_INVALID_ESCAPE_SEQUENCE;
	case EXE_NUMERIC_OVERFLOW: 						return FMT_EXE_NUMERIC_OVERFLOW;
	case EXE_MISSING_NULL_TERMINATOR: 				return FMT_EXE_MISSING_NULL_TERMINATOR;
	case EXE_CONVERT_STRING_ERROR: 					return FMT_EXE_CONVERT_STRING_ERROR;
	case EXE_CONVERT_NOT_SUPPORTED: 				return FMT_EXE_CONVERT_NOT_SUPPORTED;
	case EXE_CONVERT_DATETIME_ERROR: 				return FMT_EXE_CONVERT_DATETIME_ERROR;
	case EXE_DATETIME_FIELD_OVERFLOW: 				return FMT_EXE_DATETIME_FIELD_OVERFLOW;
	case EXE_USER_FUNCTION_ERROR: 					return FMT_EXE_USER_FUNCTION_ERROR;
	case EXE_USER_FUNCTION_NOT_SUPP: 				return FMT_EXE_USER_FUNCTION_NOT_SUPP;
	case EXE_DIVISION_BY_ZERO: 						return FMT_EXE_DIVISION_BY_ZERO;
	case EXE_MISSING_INDICATOR_VARIABLE: 			return FMT_EXE_MISSING_INDICATOR_VARIABLE;
	case EXE_ASSIGNING_NULL_TO_NOT_NULL: 			return FMT_EXE_ASSIGNING_NULL_TO_NOT_NULL;
	case EXE_CONVERT_INTERVAL_ERROR: 				return FMT_EXE_CONVERT_INTERVAL_ERROR;
	case EXE_FIELD_NUM_OVERFLOW: 					return FMT_EXE_FIELD_NUM_OVERFLOW;
	case EXE_MATH_FUNC_NOT_SUPPORTED: 				return FMT_EXE_MATH_FUNC_NOT_SUPPORTED;
	case EXE_DEFAULT_VALUE_ERROR: 					return FMT_EXE_DEFAULT_VALUE_ERROR;
	case EXE_SORT_ERROR: 							return FMT_EXE_SORT_ERROR;
	case EXE_BAD_ARG_TO_MATH_FUNC: 					return FMT_EXE_BAD_ARG_TO_MATH_FUNC;
	case EXE_MAPPED_FUNCTION_ERROR: 				return FMT_EXE_MAPPED_FUNCTION_ERROR;
	case EXE_GETBIT_ERROR: 							return FMT_EXE_GETBIT_ERROR;
	case EXE_IS_BITWISE_AND_ERROR: 					return FMT_EXE_IS_BITWISE_AND_ERROR;
	case EXE_UNSIGNED_OVERFLOW: 					return FMT_EXE_UNSIGNED_OVERFLOW;
	case EXE_INVALID_CHARACTER: 					return FMT_EXE_INVALID_CHARACTER;
	case EXE_HISTORY_BUFFER_TOO_SMALL: 				return FMT_EXE_HISTORY_BUFFER_TOO_SMALL;
	case EXE_OLAP_OVERFLOW_NOT_SUPPORTED: 			return FMT_EXE_OLAP_OVERFLOW_NOT_SUPPORTED;
	case EXE_LAST_EXPRESSIONS_ERROR: 				return FMT_EXE_LAST_EXPRESSIONS_ERROR;

// ---------------------------------------------------------------------
// File System and DP2 errors.
// ---------------------------------------------------------------------
	case EXE_ERROR_FROM_DP2: 						return FMT_EXE_ERROR_FROM_DP2;
	case EXE_ERROR_FROM_FS2: 						return FMT_EXE_ERROR_FROM_FS2;
	case EXE_FS2_FETCH_VERSION_ERROR: 				return FMT_EXE_FS2_FETCH_VERSION_ERROR;
	case EXE_ERROR_STREAM_OVERFLOW: 				return FMT_EXE_ERROR_STREAM_OVERFLOW;
	case EXE_EID_INTERNAL_ERROR: 					return FMT_EXE_EID_INTERNAL_ERROR;
	case EXE_LAST_ERROR_FROM_FS_DP2: 				return FMT_EXE_LAST_ERROR_FROM_FS_DP2;

// ---------------------------------------------------------------------
// Build-time and other catastophic errors
// ---------------------------------------------------------------------
	case EXE_NO_MEM_TO_BUILD: 						return FMT_EXE_NO_MEM_TO_BUILD;
	case EXE_NO_MEM_TO_EXEC: 						return FMT_EXE_NO_MEM_TO_EXEC;
	case EXE_CANNOT_CONTINUE: 						return FMT_EXE_CANNOT_CONTINUE;
	case EXE_ACCESS_VIOLATION: 						return FMT_EXE_ACCESS_VIOLATION;

// ------------------------------------------------------------
// Error 8574, lost open. Could result in reopening the table.
// Error 8575, could result in recompilation.
// Warning 8576, statement was recompiled.
// ------------------------------------------------------------
	case EXE_LOST_OPEN: 							return FMT_EXE_LOST_OPEN;
	case EXE_TIMESTAMP_MISMATCH: 					return FMT_EXE_TIMESTAMP_MISMATCH;
	case EXE_RECOMPILE: 							return FMT_EXE_RECOMPILE;
	case EXE_TABLE_NOT_FOUND: 						return FMT_EXE_TABLE_NOT_FOUND;
	case EXE_SIM_CHECK_PASSED: 						return FMT_EXE_SIM_CHECK_PASSED;
	case EXE_SIM_CHECK_FAILED: 						return FMT_EXE_SIM_CHECK_FAILED;
	case EXE_PARTITION_UNAVAILABLE: 				return FMT_EXE_PARTITION_UNAVAILABLE;
	case EXE_NO_MEM_FOR_IN_MEM_JOIN: 				return FMT_EXE_NO_MEM_FOR_IN_MEM_JOIN;
	case EXE_USER_PREPARE_NEEDED: 					return FMT_EXE_USER_PREPARE_NEEDED;
	case EXE_RELEASE_WORK_TIMEOUT: 					return FMT_EXE_RELEASE_WORK_TIMEOUT;
	case EXE_SCHEMA_SECURITY_CHANGED: 				return FMT_EXE_SCHEMA_SECURITY_CHANGED;
	case EXE_ASSIGN_ESPS_ERROR: 					return FMT_EXE_ASSIGN_ESPS_ERROR;
	case EXE_IAR_MFMAP_BAD: 						return FMT_EXE_IAR_MFMAP_BAD;
	case EXE_IAR_ERROR_EXTRACTING_COLUMNS: 			return FMT_EXE_IAR_ERROR_EXTRACTING_COLUMNS;
	case EXE_IAR_NO_MFMAP: 							return FMT_EXE_IAR_NO_MFMAP;
	case EXE_IAR_MISSING_COLS_COMPRESSED_AUDIT: 	return FMT_EXE_IAR_MISSING_COLS_COMPRESSED_AUDIT;
	case EXE_AUDIT_IMAGE_EXPR_EVAL_ERROR: 			return FMT_EXE_AUDIT_IMAGE_EXPR_EVAL_ERROR;
	case EXE_MERGE_STMT_ERROR: 						return FMT_EXE_MERGE_STMT_ERROR;
	case EXE_ESP_CHANGE_PRIORITY_FAILED: 			return FMT_EXE_ESP_CHANGE_PRIORITY_FAILED;
	case EXE_RECOMPILE_AUTO_QUERY_RETRY: 			return FMT_EXE_RECOMPILE_AUTO_QUERY_RETRY;
	case EXE_VIEW_NOT_FOUND: 						return FMT_EXE_VIEW_NOT_FOUND;
	case EXE_MV_UNAVILABLE: 						return FMT_EXE_MV_UNAVILABLE;

//-------------------------------------------------------------
// Errors codes for concurrency control.
//-------------------------------------------------------------
	case EXE_FIRST_CONCURRENCY_CONTROL_ERROR: 		return FMT_EXE_FIRST_CONCURRENCY_CONTROL_ERROR;
	case EXE_LOCK_UNLOCK_ERROR: 					return FMT_EXE_LOCK_UNLOCK_ERROR;
	case EXE_FILESYSTEM_ERROR: 						return FMT_EXE_FILESYSTEM_ERROR;
	case EXE_BEGIN_TRANSACTION_ERROR: 				return FMT_EXE_BEGIN_TRANSACTION_ERROR;
	case EXE_BEGIN_ERROR_FROM_TRANS_SUBSYS: 		return FMT_EXE_BEGIN_ERROR_FROM_TRANS_SUBSYS;
	case EXE_COMMIT_TRANSACTION_ERROR: 				return FMT_EXE_COMMIT_TRANSACTION_ERROR;
	case EXE_COMMIT_ERROR_FROM_TRANS_SUBSYS: 		return FMT_EXE_COMMIT_ERROR_FROM_TRANS_SUBSYS;
	case EXE_ROLLBACK_TRANSACTION_ERROR: 			return FMT_EXE_ROLLBACK_TRANSACTION_ERROR;
	case EXE_ROLLBACK_ERROR_FROM_TRANS_SUBSYS: 		return FMT_EXE_ROLLBACK_ERROR_FROM_TRANS_SUBSYS;
	case EXE_ROLLBACK_TRANSACTION_WAITED_ERROR: 	return FMT_EXE_ROLLBACK_TRANSACTION_WAITED_ERROR;
	case EXE_ROLLBACK_WAITED_ERROR_TRANS_SUBSYS: 	return FMT_EXE_ROLLBACK_WAITED_ERROR_TRANS_SUBSYS;
	case EXE_SET_TRANS_ERROR_FROM_TRANS_SUBSYS: 	return FMT_EXE_CANT_COMMIT_OR_ROLLBACK;
	case EXE_CANT_COMMIT_OR_ROLLBACK: 				return FMT_EXE_CANT_COMMIT_OR_ROLLBACK;
	case EXE_CANT_BEGIN_WITH_MULTIPLE_CONTEXTS: 	return FMT_EXE_CANT_BEGIN_WITH_MULTIPLE_CONTEXTS;
	case EXE_CANT_BEGIN_USER_TRANS_WITH_LRU: 		return FMT_EXE_CANT_BEGIN_USER_TRANS_WITH_LRU;
	case EXE_LAST_CONCURRENCY_CONTROL_ERROR: 		return FMT_EXE_LAST_CONCURRENCY_CONTROL_ERROR;

//-------------------------------------------------------------
// Error codes for bulk replicate
//-------------------------------------------------------------
	case EXE_REPL_TO_UNSUPPORTED_TGT_SYS: 			return FMT_EXE_REPL_TO_UNSUPPORTED_TGT_SYS;
	case EXE_BDR_ALREADY_INITIALIZED: 				return FMT_EXE_BDR_ALREADY_INITIALIZED;
	case EXE_BDR_SERVICE_PROCESS_COMM_ERROR: 		return FMT_EXE_BDR_SERVICE_PROCESS_COMM_ERROR;
	case EXE_REPL_TARGET_REPL_PROCESS_COMM_ERROR: 	return FMT_EXE_REPL_TARGET_REPL_PROCESS_COMM_ERROR;
	case EXE_REPL_SRC_TGT_PARTN_MISMATCH: 			return FMT_EXE_REPL_SRC_TGT_PARTN_MISMATCH;
	case EXE_REPL_SRC_TGT_DDL_MISMATCH: 			return FMT_EXE_REPL_SRC_TGT_DDL_MISMATCH;
	case EXE_REPL_SRC_TGT_VERSION_MISMATCH: 		return FMT_EXE_REPL_SRC_TGT_VERSION_MISMATCH;
	case EXE_BDR_REPL_PROCESS_COMM_ERROR: 			return FMT_EXE_BDR_REPL_PROCESS_COMM_ERROR;
	case EXE_REPL_QUERY_ID_NOT_FOUND: 				return FMT_EXE_REPL_QUERY_ID_NOT_FOUND;
	case EXE_REPL_COULD_NOT_ABORT_QUERY: 			return FMT_EXE_REPL_COULD_NOT_ABORT_QUERY;
	case EXE_REPL_QUERY_WAS_ABORTED: 				return FMT_EXE_REPL_QUERY_WAS_ABORTED;
	case EXE_REPL_COULD_NOT_RECOVER: 				return FMT_EXE_REPL_COULD_NOT_RECOVER;
	case EXE_REPL_INVALID_IPADDR_OR_PORTNUM: 		return FMT_EXE_REPL_INVALID_IPADDR_OR_PORTNUM;

//-------------------------------------------------------------
// Errors codes for suspend/resume.
//-------------------------------------------------------------
	case EXE_SUSPEND_AUDIT: 						return FMT_EXE_SUSPEND_AUDIT;
	case EXE_SUSPEND_LOCKS: 						return FMT_EXE_SUSPEND_LOCKS;
	case EXE_SUSPEND_QID_NOT_ACTIVE: 				return FMT_EXE_SUSPEND_QID_NOT_ACTIVE;
	case EXE_SUSPEND_GUARDIAN_ERROR_1: 				return FMT_EXE_SUSPEND_GUARDIAN_ERROR_1;
	case EXE_SUSPEND_GUARDIAN_ERROR_2: 				return FMT_EXE_SUSPEND_GUARDIAN_ERROR_2;
	case EXE_SUSPEND_SQL_ERROR: 					return FMT_EXE_SUSPEND_SQL_ERROR;
	case EXE_SUSPEND_ALREADY_SUSPENDED: 			return FMT_EXE_SUSPEND_ALREADY_SUSPENDED;
	case EXE_SUSPEND_NOT_SUSPENDED: 				return FMT_EXE_SUSPEND_NOT_SUSPENDED;

//-------------------------------------------------------------
// Errors codes translate function.
//-------------------------------------------------------------
	case EXE_INVALID_CHAR_IN_TRANSLATE_FUNC: 		return FMT_EXE_INVALID_CHAR_IN_TRANSLATE_FUNC;

// ---------------------------------------------------------------------
// Parallel execution
// ---------------------------------------------------------------------
	case EXE_PARALLEL_EXECUTION_ERROR: 				return FMT_EXE_PARALLEL_EXECUTION_ERROR;
	case EXE_PARALLEL_EXTRACT_OPEN_ERROR: 			return FMT_EXE_PARALLEL_EXTRACT_OPEN_ERROR;
	case EXE_PARALLEL_EXTRACT_CONNECT_ERROR: 		return FMT_EXE_PARALLEL_EXTRACT_CONNECT_ERROR;

// ---------------------------------------------------------------------
// Warning from updating Measure SQL counters.
// ---------------------------------------------------------------------
	case EXE_MEASURE: 								return FMT_EXE_MEASURE;

//----------------------------------------------------------------------
// Errors generated in the CLI code
//----------------------------------------------------------------------
	case CLI_FIRST_ERROR: 							return FMT_CLI_FIRST_ERROR;
	case CLI_PROBLEM_READING_USERS: 				return FMT_CLI_PROBLEM_READING_USERS;
	case CLI_USER_NOT_REGISTERED: 					return FMT_CLI_USER_NOT_REGISTERED;
	case CLI_USER_NOT_VALID: 						return FMT_CLI_USER_NOT_VALID;
	case CLI_INVALID_QUERY_PRIVS: 					return FMT_CLI_INVALID_QUERY_PRIVS;

	case CLI_UNUSED: 								return FMT_CLI_UNUSED;
	case CLI_CANNOT_EXECUTE_IN_MEM_DEFN: 			return FMT_CLI_CANNOT_EXECUTE_IN_MEM_DEFN;
	case CLI_DUPLICATE_DESC: 						return FMT_CLI_DUPLICATE_DESC;
	case CLI_DUPLICATE_STMT: 						return FMT_CLI_DUPLICATE_STMT;
	case CLI_DESC_NOT_EXSISTS: 						return FMT_CLI_DESC_NOT_EXSISTS;
	case CLI_STMT_NOT_EXSISTS: 						return FMT_CLI_STMT_NOT_EXSISTS;
	case CLI_NOT_DYNAMIC_DESC: 						return FMT_CLI_NOT_DYNAMIC_DESC;
	case CLI_NOT_DYNAMIC_STMT: 						return FMT_CLI_NOT_DYNAMIC_STMT;
	case CLI_DATA_OUTOFRANGE: 						return FMT_CLI_DATA_OUTOFRANGE;
	case CLI_MODULEFILE_CORRUPTED: 					return FMT_CLI_MODULEFILE_CORRUPTED;
	case CLI_MODULEFILE_OPEN_ERROR: 				return FMT_CLI_MODULEFILE_OPEN_ERROR;

	case CLI_NO_ERROR_IN_DIAGS: 					return FMT_CLI_NO_ERROR_IN_DIAGS;

	case CLI_STMT_NOT_OPEN: 						return FMT_CLI_STMT_NOT_OPEN;
	case CLI_STMT_NOT_CLOSE: 						return FMT_CLI_STMT_NOT_CLOSE;
	case CLI_STMT_CLOSE: 							return FMT_CLI_STMT_CLOSE;
	case CLI_TRANS_MODE_MISMATCH: 					return FMT_CLI_TRANS_MODE_MISMATCH;
	case CLI_TCB_EXECUTE_ERROR: 					return FMT_CLI_TCB_EXECUTE_ERROR;
	case CLI_TCB_FETCH_ERROR: 						return FMT_CLI_TCB_FETCH_ERROR;
	case CLI_TDB_DESCRIBE_ERROR: 					return FMT_CLI_TDB_DESCRIBE_ERROR;
	case CLI_BEGIN_TRANSACTION_ERROR: 				return FMT_CLI_BEGIN_TRANSACTION_ERROR;
	case CLI_COMMIT_TRANSACTION_ERROR: 				return FMT_CLI_COMMIT_TRANSACTION_ERROR;

	case CLI_ROLLBACK_TRANSACTION_ERROR: 			return FMT_CLI_ROLLBACK_TRANSACTION_ERROR;
	case CLI_STMT_NOT_PREPARED: 					return FMT_CLI_STMT_NOT_PREPARED;
	case CLI_IO_REQUESTS_PENDING: 					return FMT_CLI_IO_REQUESTS_PENDING;
	case CLI_NO_MODULE_NAME: 						return FMT_CLI_NO_MODULE_NAME;
	case CLI_MODULE_ALREADY_ADDED: 					return FMT_CLI_MODULE_ALREADY_ADDED;
	case CLI_ADD_MODULE_ERROR: 						return FMT_CLI_ADD_MODULE_ERROR;
	case CLI_SEND_REQUEST_ERROR: 					return FMT_CLI_SEND_REQUEST_ERROR;
	case CLI_OUT_OF_MEMORY: 						return FMT_CLI_OUT_OF_MEMORY;
	case CLI_INVALID_DESC_ENTRY: 					return FMT_CLI_INVALID_DESC_ENTRY;
	case CLI_NO_CURRENT_CONTEXT: 					return FMT_CLI_NO_CURRENT_CONTEXT;

	case CLI_MODULE_NOT_ADDED: 						return FMT_CLI_MODULE_NOT_ADDED;
	case CLI_TRANSACTION_NOT_STARTED: 				return FMT_CLI_TRANSACTION_NOT_STARTED;
	case CLI_INVALID_SQLTRANS_COMMAND: 				return FMT_CLI_INVALID_SQLTRANS_COMMAND;
	case CLI_NO_INSTALL_DIR: 						return FMT_CLI_NO_INSTALL_DIR;
	case CLI_INVALID_DESC_INFO_REQUEST: 			return FMT_CLI_INVALID_DESC_INFO_REQUEST;
	case CLI_INVALID_UPDATE_COLUMN: 				return FMT_CLI_INVALID_UPDATE_COLUMN;
	case CLI_INVALID_USERID: 						return FMT_CLI_INVALID_USERID;
	case CLI_RECEIVE_ERROR: 						return FMT_CLI_RECEIVE_ERROR;
	case CLI_VALIDATE_TRANSACTION_ERROR: 			return FMT_CLI_VALIDATE_TRANSACTION_ERROR;
	case CLI_SELECT_INTO_ERROR: 					return FMT_CLI_SELECT_INTO_ERROR;
	case CLI_INVALID_OBJECTNAME: 					return FMT_CLI_INVALID_OBJECTNAME;

	case CLI_USER_ENDED_EXE_XN: 					return FMT_CLI_USER_ENDED_EXE_XN;
	case CLI_NON_UPDATABLE_SELECT_CURSOR: 			return FMT_CLI_NON_UPDATABLE_SELECT_CURSOR;
	case CLI_ITEM_NUM_OUT_OF_RANGE: 				return FMT_CLI_ITEM_NUM_OUT_OF_RANGE;
	case CLI_USER_ENDED_XN_CLEANUP: 				return FMT_CLI_USER_ENDED_XN_CLEANUP;
	case CLI_INTERR_NULL_TCB: 						return FMT_CLI_INTERR_NULL_TCB;
	case CLI_EMPTY_SQL_STMT: 						return FMT_CLI_EMPTY_SQL_STMT;
	case CLI_SQLMP_RTD_ERROR: 						return FMT_CLI_SQLMP_RTD_ERROR;
	case CLI_CANCEL_REJECTED: 						return FMT_CLI_CANCEL_REJECTED;
	case CLI_NON_CURSOR_UPDEL_TABLE: 				return FMT_CLI_NON_CURSOR_UPDEL_TABLE;
	case CLI_USER_MEMORY_IN_EXECUTOR_SEGMENT: 		return FMT_CLI_USER_MEMORY_IN_EXECUTOR_SEGMENT;
	case CLI_CURSOR_CANNOT_BE_HOLDABLE: 			return FMT_CLI_CURSOR_CANNOT_BE_HOLDABLE;
	case CLI_INVALID_ATTR_NAME: 					return FMT_CLI_INVALID_ATTR_NAME;
	case CLI_INVALID_ATTR_VALUE: 					return FMT_CLI_INVALID_ATTR_VALUE;
	case CLI_CURSOR_ATTR_CANNOT_BE_SET: 			return FMT_CLI_CURSOR_ATTR_CANNOT_BE_SET;
	case CLI_ARRAY_MAXSIZE_INVALID_ENTRY: 			return FMT_CLI_ARRAY_MAXSIZE_INVALID_ENTRY;
	case CLI_LOCAL_AUTHENTICATION: 					return FMT_CLI_LOCAL_AUTHENTICATION;
	case CLI_INVALID_SQL_ID: 						return FMT_CLI_INVALID_SQL_ID;
	case CLI_UPDATE_PENDING: 						return FMT_CLI_UPDATE_PENDING;
// ---------------------------------------------------------------------
// Module versioning errors
// ---------------------------------------------------------------------
	case CLI_MODULE_HDR_VERSION_ERROR: 				return FMT_CLI_MODULE_HDR_VERSION_ERROR;
	case CLI_MOD_DLT_HDR_VERSION_ERROR: 			return FMT_CLI_MOD_DLT_HDR_VERSION_ERROR;
	case CLI_MOD_DLT_ENT_VERSION_ERROR: 			return FMT_CLI_MOD_DLT_ENT_VERSION_ERROR;
	case CLI_MOD_DESC_HDR_VERSION_ERROR: 			return FMT_CLI_MOD_DESC_HDR_VERSION_ERROR;
	case CLI_MOD_DESC_ENT_VERSION_ERROR: 			return FMT_CLI_MOD_DESC_ENT_VERSION_ERROR;
	case CLI_MOD_PLT_HDR_VERSION_ERROR: 			return FMT_CLI_MOD_PLT_HDR_VERSION_ERROR;
	case CLI_MOD_PLT_ENT_VERSION_ERROR: 			return FMT_CLI_MOD_PLT_ENT_VERSION_ERROR;
	case CLI_READ_ERROR: 							return FMT_CLI_READ_ERROR;
	case CLI_CREATE_CONTEXT_EXE_TRANSACTION: 		return FMT_CLI_CREATE_CONTEXT_EXE_TRANSACTION;
	case CLI_INVALID_QFO_NUMBER: 					return FMT_CLI_INVALID_QFO_NUMBER;
	case CLI_STATEMENT_WITH_NO_QFO: 				return FMT_CLI_STATEMENT_WITH_NO_QFO;
	case CLI_NOWAIT_TAG_NOT_SPECIFIED: 				return FMT_CLI_NOWAIT_TAG_NOT_SPECIFIED;
	case CLI_OPERATION_WITH_PENDING_OPS: 			return FMT_CLI_OPERATION_WITH_PENDING_OPS;
	case CLI_STATEMENT_ASSOCIATED_WITH_QFO: 		return FMT_CLI_STATEMENT_ASSOCIATED_WITH_QFO;

	case CLI_SAVEPOINT_ROLLBACK_FAILED: 			return FMT_CLI_SAVEPOINT_ROLLBACK_FAILED;
	case CLI_SAVEPOINT_ROLLBACK_DONE: 				return FMT_CLI_SAVEPOINT_ROLLBACK_DONE;
	case CLI_PARTIAL_UPDATED_DATA: 					return FMT_CLI_PARTIAL_UPDATED_DATA;
	case CLI_AUTO_BEGIN_TRANSACTION_ERROR: 			return FMT_CLI_AUTO_BEGIN_TRANSACTION_ERROR;
	case CLI_BUFFER_TOO_SMALL: 						return FMT_CLI_BUFFER_TOO_SMALL;
	case CLI_REMOVE_CURRENT_CONTEXT: 				return FMT_CLI_REMOVE_CURRENT_CONTEXT;
	case CLI_CONTEXT_NOT_FOUND: 					return FMT_CLI_CONTEXT_NOT_FOUND;
	case CLI_NO_SQL_ACCESS_MODE_VIOLATION: 			return FMT_CLI_NO_SQL_ACCESS_MODE_VIOLATION;
	case CLI_NOT_CHECK_VIOLATION: 					return FMT_CLI_NOT_CHECK_VIOLATION;
	case CLI_NO_TRANS_STMT_VIOLATION: 				return FMT_CLI_NO_TRANS_STMT_VIOLATION;

	case CLI_SEND_ARKCMP_CONTROL: 					return FMT_CLI_SEND_ARKCMP_CONTROL;

	case CLI_INTERR_ON_CONTEXT_SWITCH: 				return FMT_CLI_INTERR_ON_CONTEXT_SWITCH;

	case CLI_GENCODE_BUFFER_TOO_SMALL: 				return FMT_CLI_GENCODE_BUFFER_TOO_SMALL;

	case CLI_IUD_IN_PROGRESS: 						return FMT_CLI_IUD_IN_PROGRESS;

	case CLI_RS_PROXY_BUFFER_SMALL_OR_NULL: 		return FMT_CLI_RS_PROXY_BUFFER_SMALL_OR_NULL;

	case CLI_ARKCMP_INIT_FAILED: 					return FMT_CLI_ARKCMP_INIT_FAILED;
	case CLI_NOT_ASCII_CHAR_TYPE: 					return FMT_CLI_NOT_ASCII_CHAR_TYPE;
	case CLI_RTD_BUFFER_TOO_SMALL: 					return FMT_CLI_RTD_BUFFER_TOO_SMALL;
	case CLI_STMT_DESC_COUNT_MISMATCH: 				return FMT_CLI_STMT_DESC_COUNT_MISMATCH;
	case CLI_RESERVED_ARGUMENT: 					return FMT_CLI_RESERVED_ARGUMENT;
	case CLI_INVALID_CHARSET_FOR_DESCRIPTOR: 		return FMT_CLI_INVALID_CHARSET_FOR_DESCRIPTOR;
	case CLI_CHARSET_MISMATCH: 						return FMT_CLI_CHARSET_MISMATCH;

	case CLI_SHADOW_RPC_EXCEPTION: 					return FMT_CLI_SHADOW_RPC_EXCEPTION;
	case CLI_INTERNAL_ERROR: 						return FMT_CLI_INTERNAL_ERROR;
	case CLI_LAST_ERROR: 							return FMT_CLI_LAST_ERROR;

// ---------------------------------------------------------------------
// Diagnostic message errors
// ---------------------------------------------------------------------
	case CLI_MSG_CHAR_SET_NOT_SUPPORTED: 			return FMT_CLI_MSG_CHAR_SET_NOT_SUPPORTED;

// ---------------------------------------------------------------------
// Execution errors for user-defined functions and procedures
// ---------------------------------------------------------------------
	case EXE_UDR_SERVER_WENT_AWAY: 					return FMT_EXE_UDR_SERVER_WENT_AWAY;
	case EXE_UDR_INVALID_HANDLE: 					return FMT_EXE_UDR_INVALID_HANDLE;
	case EXE_UDR_ATTEMPT_TO_KILL: 					return FMT_EXE_UDR_ATTEMPT_TO_KILL;
	case EXE_UDR_REPLY_ERROR: 						return FMT_EXE_UDR_REPLY_ERROR;
	case EXE_UDR_ACCESS_VIOLATION: 					return FMT_EXE_UDR_ACCESS_VIOLATION;
	case EXE_UDR_INVALID_OR_CORRUPT_REPLY: 			return FMT_EXE_UDR_INVALID_OR_CORRUPT_REPLY;
	case EXE_UDR_RESULTSETS_NOT_SUPPORTED: 			return FMT_EXE_UDR_RESULTSETS_NOT_SUPPORTED;
	case EXE_UDR_RS_ALLOC_RS_NOT_SUPPORTED: 		return FMT_EXE_UDR_RS_ALLOC_RS_NOT_SUPPORTED;
	case EXE_UDR_RS_ALLOC_STMT_NOT_CALL: 			return FMT_EXE_UDR_RS_ALLOC_STMT_NOT_CALL;
//                                      8910 is used by some other feature
//                                      8911 is used by some other feature
	case EXE_UDR_RS_ALLOC_INTERNAL_ERROR: 			return FMT_EXE_UDR_RS_ALLOC_INTERNAL_ERROR;
	case EXE_UDR_RS_PREPARE_NOT_ALLOWED: 			return FMT_EXE_UDR_RS_PREPARE_NOT_ALLOWED;
	case EXE_UDR_RS_REOPEN_NOT_ALLOWED: 			return FMT_EXE_UDR_RS_REOPEN_NOT_ALLOWED;
	case EXE_UDR_RS_NOT_AVAILABLE: 					return FMT_EXE_UDR_RS_NOT_AVAILABLE;
	case EXE_UDR_RS_ALLOC_INVALID_INDEX: 			return FMT_EXE_UDR_RS_ALLOC_INVALID_INDEX;
	case EXE_UDR_RS_ALLOC_ALREADY_EXISTS: 			return FMT_EXE_UDR_RS_ALLOC_ALREADY_EXISTS;
	case EXE_RTS_NOT_STARTED: 						return FMT_EXE_RTS_NOT_STARTED;
	case EXE_RTS_INVALID_QID: 						return FMT_EXE_RTS_INVALID_QID;
	case EXE_RTS_INVALID_CPU_PID: 					return FMT_EXE_RTS_INVALID_CPU_PID;
	case EXE_RTS_TIMED_OUT: 						return FMT_EXE_RTS_TIMED_OUT;
	case EXE_RTS_REQ_PARTIALY_SATISFIED: 			return FMT_EXE_RTS_REQ_PARTIALY_SATISFIED;
	case EXE_RTS_QID_NOT_FOUND: 					return FMT_EXE_RTS_QID_NOT_FOUND;
	case CLI_MERGED_STATS_NOT_AVAILABLE: 			return FMT_CLI_MERGED_STATS_NOT_AVAILABLE;
	case CLI_QID_NOT_MATCHING: 						return FMT_CLI_QID_NOT_MATCHING;
	case EXE_STAT_NOT_FOUND: 						return FMT_EXE_STAT_NOT_FOUND;
	case EXE_ERROR_IN_STAT_ITEM: 					return FMT_EXE_ERROR_IN_STAT_ITEM;
	case CLI_INSUFFICIENT_STATS_DESC_SQL: 			return FMT_CLI_INSUFFICIENT_STATS_DESC_SQL;
	case CLI_INSUFFICIENT_SIKEY_BUFF: 				return FMT_CLI_INSUFFICIENT_SIKEY_BUFF;

	case CLI_CONSUMER_QUERY_BUF_TOO_SMALL: 			return FMT_CLI_CONSUMER_QUERY_BUF_TOO_SMALL;

	case EXE_SG_MAXVALUE_EXCEEDED: 					return FMT_EXE_SG_MAXVALUE_EXCEEDED;
	case EXE_SG_UPDATE_FAILURE: 					return FMT_EXE_SG_UPDATE_FAILURE;

	case EXE_UDR_INVALID_DATA: 						return FMT_EXE_UDR_INVALID_DATA;

	case CLI_SESSION_ATTR_BUFFER_TOO_SMALL: 		return FMT_CLI_SESSION_ATTR_BUFFER_TOO_SMALL;
	case CLI_USERNAME_BUFFER_TOO_SMALL: 			return FMT_CLI_USERNAME_BUFFER_TOO_SMALL;

	case EXE_ROWLENGTH_EXCEEDS_BUFFER: 				return FMT_EXE_ROWLENGTH_EXCEEDS_BUFFER;

//-------------------------------------------------------------
// Error codes for bulk replicate - Part 2
//-------------------------------------------------------------
	case EXE_INTERNALLY_GENERATED_COMMAND: 			return FMT_EXE_INTERNALLY_GENERATED_COMMAND;

//-------------------------------------------------------------
// Error codes for AES encrpt/decrypt functions
//-------------------------------------------------------------
    case EXE_AES_INVALID_IV:                        return FMT_EXE_AES_INVALID_IV;
    case EXE_ERR_PARAMCOUNT_FOR_FUNC:               return FMT_EXE_ERR_PARAMCOUNT_FOR_FUNC;
    case EXE_OPTION_IGNORED:                        return FMT_EXE_OPTION_IGNORED;
    case EXE_OPENSSL_ERROR:                         return FMT_EXE_OPENSSL_ERROR;

//fast transport

	case EXE_EXTRACT_ERROR_CREATING_FILE: 			return FMT_EXE_EXTRACT_ERROR_CREATING_FILE;
	case EXE_EXTRACT_ERROR_WRITING_TO_FILE: 		return FMT_EXE_EXTRACT_ERROR_WRITING_TO_FILE;
	case EXE_EXTRACT_CANNOT_ALLOCATE_BUFFER: 		return FMT_EXE_EXTRACT_CANNOT_ALLOCATE_BUFFER;

// ---------------------------------------------------------------------
// Scratch file I/O errors (10100 - 10199)
// ---------------------------------------------------------------------
	case EXE_SCR_IO_CREATE: 						return FMT_EXE_SCR_IO_CREATE;
	case EXE_SCR_IO_OPEN: 							return FMT_EXE_SCR_IO_OPEN;
	case EXE_SCR_IO_CLOSE: 							return FMT_EXE_SCR_IO_CLOSE;
	case EXE_SCR_IO_WRITE: 							return FMT_EXE_SCR_IO_WRITE;
	case EXE_SCR_IO_READ: 							return FMT_EXE_SCR_IO_READ;

	case EXE_SCR_IO_SETMODE: 						return FMT_EXE_SCR_IO_SETMODE;
	case EXE_SCR_IO_AWAITIOX: 						return FMT_EXE_SCR_IO_AWAITIOX;
	case EXE_SCR_IO_POSITION: 						return FMT_EXE_SCR_IO_POSITION;
	case EXE_SCR_IO_GETINFO: 						return FMT_EXE_SCR_IO_GETINFO;
	case EXE_SCR_IO_GETINFOLIST: 					return FMT_EXE_SCR_IO_GETINFOLIST;
	case EXE_SCR_IO_GETINFOLISTBYNAME: 				return FMT_EXE_SCR_IO_GETINFOLISTBYNAME;
	case EXE_SCR_IO_GET_PHANDLE: 					return FMT_EXE_SCR_IO_GET_PHANDLE;
	case EXE_SCR_IO_DECOMPOSE_PHANDLE: 				return FMT_EXE_SCR_IO_DECOMPOSE_PHANDLE;
	case EXE_SCR_IO_FILENAME_FINDSTART: 			return FMT_EXE_SCR_IO_FILENAME_FINDSTART;
	case EXE_SCR_IO_FILENAME_FINDNEXT: 				return FMT_EXE_SCR_IO_FILENAME_FINDNEXT;

	case EXE_SCR_IO_CREATEDIR: 						return FMT_EXE_SCR_IO_CREATEDIR;
	case EXE_SCR_IO_CREATEFILE: 					return FMT_EXE_SCR_IO_CREATEFILE;
	case EXE_SCR_IO_GETTMPFNAME: 					return FMT_EXE_SCR_IO_GETTMPFNAME;
	case EXE_SCR_IO_CLOSEHANDLE: 					return FMT_EXE_SCR_IO_CLOSEHANDLE;
	case EXE_SCR_IO_WRITEFILE: 						return FMT_EXE_SCR_IO_WRITEFILE;
	case EXE_SCR_IO_SETFILEPOINTER: 				return FMT_EXE_SCR_IO_SETFILEPOINTER;
	case EXE_SCR_IO_CREATEEVENT: 					return FMT_EXE_SCR_IO_CREATEEVENT;
	case EXE_SCR_IO_WAITMULTOBJ: 					return FMT_EXE_SCR_IO_WAITMULTOBJ;
	case EXE_SCR_IO_WAITSINGLEOBJ: 					return FMT_EXE_SCR_IO_WAITSINGLEOBJ;
	case EXE_SCR_IO_GETOVERLAPPEDRESULT: 			return FMT_EXE_SCR_IO_GETOVERLAPPEDRESULT;
	case EXE_SCR_IO_RESETEVENT: 					return FMT_EXE_SCR_IO_RESETEVENT;
	case EXE_SCR_IO_GETDISKFREESPACE: 				return FMT_EXE_SCR_IO_GETDISKFREESPACE;

	case EXE_SCR_IO_NO_DISKS: 						return FMT_EXE_SCR_IO_NO_DISKS;
	case EXE_SCR_IO_THRESHOLD: 						return FMT_EXE_SCR_IO_THRESHOLD;
	case EXE_SCR_IO_INVALID_BLOCKNUM: 				return FMT_EXE_SCR_IO_INVALID_BLOCKNUM;
	case EXE_SCR_IO_UNMAPPED_BLOCKNUM: 				return FMT_EXE_SCR_IO_UNMAPPED_BLOCKNUM;

// ---------------------------------------------------------------------
// Execution errors related to Materialized Views
// ---------------------------------------------------------------------
	case CLI_MV_EXECUTE_UNINITIALIZED: 				return FMT_CLI_MV_EXECUTE_UNINITIALIZED;

// ---------------------------------------------------------------------
// Execution errors related to Rowsets
// ---------------------------------------------------------------------
	case EXE_ROWSET_INDEX_OUTOF_RANGE: 				return FMT_EXE_ROWSET_INDEX_OUTOF_RANGE;
	case EXE_ROWSET_OVERFLOW: 						return FMT_EXE_ROWSET_OVERFLOW;
	case EXE_ROWSET_CORRUPTED: 						return FMT_EXE_ROWSET_CORRUPTED;

	case EXE_ROWSET_NEGATIVE_SIZE: 					return FMT_EXE_ROWSET_NEGATIVE_SIZE;
	case EXE_ROWSET_WRONG_SIZETYPE: 				return FMT_EXE_ROWSET_WRONG_SIZETYPE;
	case EXE_ROWSET_VARDATA_OR_INDDATA_ERROR: 		return FMT_EXE_ROWSET_VARDATA_OR_INDDATA_ERROR;
	case EXE_ROWSET_SCALAR_ARRAY_MISMATCH: 			return FMT_EXE_ROWSET_SCALAR_ARRAY_MISMATCH;
	case EXE_NONFATAL_ERROR_SEEN: 					return FMT_EXE_NONFATAL_ERROR_SEEN;
	case EXE_ROWSET_ROW_COUNT_ARRAY_WRONG_SIZE: 	return FMT_EXE_ROWSET_ROW_COUNT_ARRAY_WRONG_SIZE;
	case EXE_ROWSET_ROW_COUNT_ARRAY_NOT_AVAILABLE: 	return FMT_EXE_ROWSET_ROW_COUNT_ARRAY_NOT_AVAILABLE;
	case EXE_NOTATOMIC_ENABLED_AFTER_TRIGGER: 		return FMT_EXE_NOTATOMIC_ENABLED_AFTER_TRIGGER;
	case EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED: 		return FMT_EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED;
	case CLI_STMT_NEEDS_PREPARE: 					return FMT_CLI_STMT_NEEDS_PREPARE;
	case EXE_NONFATAL_ERROR_ON_ALL_ROWS: 			return FMT_EXE_NONFATAL_ERROR_ON_ALL_ROWS;

	case CLI_ROWWISE_ROWSETS_NOT_SUPPORTED: 		return FMT_CLI_ROWWISE_ROWSETS_NOT_SUPPORTED;

	case CLI_RWRS_DECOMPRESS_ERROR: 				return FMT_CLI_RWRS_DECOMPRESS_ERROR;
	case CLI_RWRS_DECOMPRESS_LENGTH_ERROR: 			return FMT_CLI_RWRS_DECOMPRESS_LENGTH_ERROR;
	case CLI_NAR_ERROR_DETAILS: 					return FMT_CLI_NAR_ERROR_DETAILS;
// ---------------------------------------------------------------------
// Execution errors related to json
// ---------------------------------------------------------------------
    case EXE_JSON_INVALID_TOKEN:                    return FMT_EXE_JSON_INVALID_TOKEN;
    case EXE_JSON_INVALID_VALUE:                    return FMT_EXE_JSON_INVALID_VALUE;
    case EXE_JSON_INVALID_STRING:                   return FMT_EXE_JSON_INVALID_STRING;
    case EXE_JSON_INVALID_ARRAY_START:              return FMT_EXE_JSON_INVALID_ARRAY_START;
    case EXE_JSON_INVALID_ARRAY_NEXT:               return FMT_EXE_JSON_INVALID_ARRAY_NEXT;
    case EXE_JSON_INVALID_OBJECT_START:             return FMT_EXE_JSON_INVALID_OBJECT_START;
    case EXE_JSON_INVALID_OBJECT_LABEL:             return FMT_EXE_JSON_INVALID_OBJECT_LABEL;
    case EXE_JSON_INVALID_OBJECT_NEXT:              return FMT_EXE_JSON_INVALID_OBJECT_NEXT;
    case EXE_JSON_INVALID_OBJECT_COMMA:             return FMT_EXE_JSON_INVALID_OBJECT_COMMA;
    case EXE_JSON_INVALID_END:                      return FMT_EXE_JSON_INVALID_END;
    case EXE_JSON_END_PREMATURELY:                  return FMT_EXE_JSON_END_PREMATURELY;
    case EXE_JSON_UNEXPECTED_ERROR:                 return FMT_EXE_JSON_UNEXPECTED_ERROR;

    
// ---------------------------------------------------------------------
//
// ---------------------------------------------------------------------
	default:
		return "";
	}
};
