/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

#include "WmsEvents.h"
#include "evl_sqlog_eventnum.h"

//Singleton event message
TokenizedEventMsg *TokenizedEventMsg::m_event = NULL;

TokenizedEventMsg *TokenizedEventMsg::getInstance(const string broker_ip)
{
	if (TokenizedEventMsg::m_event == NULL)
	{
		m_event = new TokenizedEventMsg(broker_ip);
	}

	return m_event;
}

TokenizedEventMsg::TokenizedEventMsg(const string broker_ip) : m_broker_ip(broker_ip)
{
	srvrObjRef = "TokenizedDll";

	char *node_port = getenv(QPID_NODE_PORT_ENV);
	if (node_port == NULL)
	{
		stringstream ss;
		ss << ": getenv() could not get the environment " << QPID_NODE_PORT_ENV;
		//throw ss.str().c_str();
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}

#if DEFAULT_QPID_PERFSTATS
	m_enable = true;
	char *enable_qpid = getenv(ENABLE_QPID_PERFSTATS);
	if (enable_qpid != NULL && strcmp(enable_qpid, "0") == 0)
		m_enable = false;
#else
	m_enable = false;
	char *enable_qpid = getenv(ENABLE_QPID_PERFSTATS);
	if (enable_qpid != NULL && strcmp(enable_qpid, "1") == 0)
		m_enable = true;
#endif
	m_debug = false;
	char *debug_qpid = getenv(DEBUG_QPID_PERFSTATS);
	if (debug_qpid != NULL && strcmp(debug_qpid, "1") == 0)
		m_debug = true;

	m_broker_port = atoi(node_port);

	int rc = createAMQPConnection(m_broker_ip.c_str(), m_broker_port);
	if (rc != SP_SUCCESS)
	{
		stringstream ss;
		ss << ": createAMQPConnection() broker_ip " << m_broker_ip.c_str() << ",broker_port " << m_broker_port << " rc " << rc;
		//throw ss.str().c_str();
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
}

TokenizedEventMsg::~TokenizedEventMsg()
{
	int rc = closeAMQPConnection();
	if (rc != SP_SUCCESS)
	{
		stringstream ss;
		ss << ": closeAMQPConnection() rc " << rc;
		//throw ss.str().c_str();
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
}

inline void TokenizedEventMsg::SendErrorTextMsg(short nToken, ...)
{
	char *tokenPtr = NULL;

	va_list marker;
	va_start( marker, nToken);

	send_to_eventlog(MSG_ODBC_NSK_ERROR,
			EVENTLOG_ERROR_TYPE,
			ODBCMX_SERVER,
			srvrObjRef,
			nToken,
			marker);

	tokenPtr = va_arg(marker, char *);
	string ss(tokenPtr);
	throw(ss);
}

//UTF8 character identification in the following code
//The most significant bit of a single-byte character is always 0.
//The most significant bits of the first byte of a multi-byte sequence determine the length of the sequence.
//These most significant bits are: 110 for two-byte sequences; 1110 for three-byte sequences, and so on.
//The remaining bytes in a multi-byte sequence have 10 as their two most significant bits.
int TokenizedEventMsg::getUTF8CharLength(const char *inputChar, const int inputLength, const int maxLength)
{
	int length = 0;
	int numBytesInChar = 0;
	int idx = 0;
	unsigned char byte;

	while (inputChar[idx])
	{
		byte = inputChar[idx];
		if (byte & 0x80) //multibyte ?
		{
			if ((byte & 0x40) && (byte & 0x20) && (byte & 0x10))
				numBytesInChar = 4;
			else if ((byte & 0x40) && (byte & 0x20) && !(byte & 0x10))
				numBytesInChar = 3;
			else if ((byte & 0x40) && !(byte & 0x20) && !(byte & 0x10))
				numBytesInChar = 2;
			if ((idx + numBytesInChar) > maxLength)
				break;
		}
		else
		{
			numBytesInChar = 1;
			if ((idx + numBytesInChar) > maxLength)
				break;
		}

		idx += numBytesInChar;
		length += numBytesInChar;
	}

	return length;
}

//LCOV_EXCL_START
void TokenizedEventMsg::debugPrint(const char *name, const char *name2, bool enterFunction)
{
	if (m_debug)
	{
		struct stat infobuf; /* place to store info */

		if (stat(QPID_PERFSTATS_FILENAME, &infobuf) != 0)
		{
			if (errno != ENOENT)
			{
				//cerr << "stat(): failed to get file info " << QPID_PERFSTATS_FILENAME << endl;
				stringstream ss;
				ss << "stat(): failed to get file info " << QPID_PERFSTATS_FILENAME;
				SendErrorTextMsg(1,(char*)ss.str().c_str());
			}
		}
		else
		{
			if (infobuf.st_size >= TMP_FILE_SIZE)
			{
				if (remove(QPID_PERFSTATS_BACK_FILENAME) != 0)
				{
					if (errno != ENOENT)
					{
						//cerr << "remove(): failed to remove file " << QPID_PERFSTATS_BACK_FILENAME << endl;
						stringstream ss;
						ss << "remove(): failed to remove file " << QPID_PERFSTATS_BACK_FILENAME;
						SendErrorTextMsg(1,(char*)ss.str().c_str());
					}
				}

				if (rename(QPID_PERFSTATS_FILENAME, QPID_PERFSTATS_BACK_FILENAME) != 0)
				{
					if (errno != ENOENT)
					{
						stringstream ss;
						ss << "remove(): failed to rename file " << QPID_PERFSTATS_FILENAME << " to "	<< QPID_PERFSTATS_BACK_FILENAME;
						SendErrorTextMsg(1,(char*)ss.str().c_str());
					}
				}
			}
		}

		//Get the timetsamp
		char strNow[20]; //"yyyy-mm-dd hh:mm:ss"
		time_t now = time(NULL);
		bzero(strNow, sizeof(strNow));
		strftime(strNow, sizeof(strNow), "%Y-%m-%d %H:%M:%S", localtime(&now));
		ofstream ofs;
		ofs.open(QPID_PERFSTATS_FILENAME, ios::out | ios::app);

		if (enterFunction)
		{
			ofs << "QPID:  [" << strNow << "] " << ENABLE_QPID_PERFSTATS << "=" << (m_enable ? 1 : 0) << endl;
			ofs << "Enter: [" << strNow << "] " << name << ", called by " << name2 << endl;
		}
		else
		{
			ofs << "Exit:  [" << strNow << "] " << name << ", called by " << name2 << endl << endl;
		}
		ofs.close();
	}
}

void TokenizedEventMsg::debugPrint(const char *info)
{
	if (m_debug)
	{
		struct stat infobuf; /* place to store info */

		if (stat(QPID_PERFSTATS_FILENAME, &infobuf) != 0)
		{
			if (errno != ENOENT)
			{
				//cerr << "stat(): failed to get file info " << QPID_PERFSTATS_FILENAME << endl;
				stringstream ss;
				ss << "stat(): failed to get file info " << QPID_PERFSTATS_FILENAME;
				SendErrorTextMsg(1,(char*)ss.str().c_str());
			}
		}
		else
		{
			if (infobuf.st_size >= TMP_FILE_SIZE)
			{
				if (remove(QPID_PERFSTATS_BACK_FILENAME) != 0)
				{
					if (errno != ENOENT)
					{
						//cerr << "remove(): failed to remove file " << QPID_PERFSTATS_BACK_FILENAME << endl;
						stringstream ss;
						ss << "remove(): failed to remove file " << QPID_PERFSTATS_BACK_FILENAME;
						SendErrorTextMsg(1,(char*)ss.str().c_str());
					}
				}

				if (rename(QPID_PERFSTATS_FILENAME, QPID_PERFSTATS_BACK_FILENAME) != 0)
				{
					if (errno != ENOENT)
					{
						stringstream ss;
						ss << "remove(): failed to rename file " << QPID_PERFSTATS_FILENAME << " to " << QPID_PERFSTATS_BACK_FILENAME;
						SendErrorTextMsg(1,(char*)ss.str().c_str());
					}
				}
			}
		}

		//Get the timetsamp
		char strNow[20]; //"yyyy-mm-dd hh:mm:ss"
		time_t now = time(NULL);
		bzero(strNow, sizeof(strNow));
		strftime(strNow, sizeof(strNow), "%Y-%m-%d %H:%M:%S", localtime(&now));
		ofstream ofs;
		ofs.open(QPID_PERFSTATS_FILENAME, ios::out | ios::app);
		ofs << "Info: [" << strNow << "] " << info << endl;
		ofs.close();
	}
}
//LCOV_EXCL_STOP

//public overloaded function sendTokenizedEvent
void TokenizedEventMsg::sendTokenizedEvent(const QPID_SESSION_START* ss)
{
	if (!m_enable)
		return;
	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_SESSION_START* ss)", ss->m_functionCalled.c_str());

	//declare event variables
	ndcs::session_start_stats sessionStartStats;
	common::info_header *sessionStartStatsInfoHeader = sessionStartStats.mutable_header();
	common::qpid_header *sessionStartStatsQpidHeader = sessionStartStatsInfoHeader->mutable_header();

	//populate event header fields
	//call wrapper to populate info header
	int rc = initAMQPInfoHeader(sessionStartStatsInfoHeader, SQEVL_TRANSDUCER);
	if (rc != SP_SUCCESS)
	{
		stringstream ss;
		ss << ": sendTokenizedEvent() encountered error in initAMQPInfoHeader " << rc;
		//throw ss.str().c_str();
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("initAMQPInfoHeader(sessionStartStatsInfoHeader, ...)");

	sessionStartStats.set_session_id(ss->m_sessionId.c_str()); //required field
	sessionStartStats.set_session_status(ss->m_session_status.c_str());
	sessionStartStats.set_user_id(ss->m_user_id);
	sessionStartStats.set_user_name(ss->m_user_name.c_str());
	sessionStartStats.set_role_name(ss->m_role_name.c_str());
	sessionStartStats.set_client_name(ss->m_client_name.c_str());
	sessionStartStats.set_client_user_name(ss->m_client_user_name.c_str());
	sessionStartStats.set_application_name(ss->m_application_name.c_str());
	sessionStartStats.set_datasource_name(ss->m_datasource_name.c_str());
	if (getValidJulianTimestamp(ss->m_entryTime) != -1)
	{
		sessionStartStats.set_entry_id_utc_ts(ss->m_entryTime - JULIANTIME_DIFF);
		sessionStartStats.set_entry_id_lct_ts(utc2lct_useconds_jts(ss->m_entryTime) - JULIANTIME_DIFF);

	}
	sessionStartStats.set_total_login_elapsed_time_mcsec(ss->m_total_login_elapsed_time_mcsec);
	sessionStartStats.set_ldap_login_elapsed_time_mcsec(ss->m_ldap_login_elapsed_time_mcsec);
	sessionStartStats.set_sql_user_elapsed_time_mcsec(ss->m_sql_user_elapsed_time_mcsec);
	sessionStartStats.set_search_connection_elapsed_time_mcsec(ss->m_search_connection_elapsed_time_mcsec);
	sessionStartStats.set_search_elapsed_time_mcsec(ss->m_search_elapsed_time_mcsec);
	sessionStartStats.set_authentication_connection_elapsed_time_mcsec(ss->m_authentication_connection_elapsed_time_mcsec);
	sessionStartStats.set_authentication_elapsed_time_mcsec(ss->m_authentication_elapsed_time_mcsec);

	if (getValidJulianTimestamp(ss->m_session_start_time) != -1)
	{
		sessionStartStatsInfoHeader->set_info_generation_time_ts_utc(ss->m_session_start_time - JULIANTIME_DIFF);
		sessionStartStatsInfoHeader->set_info_generation_time_ts_lct(utc2lct_useconds_jts(ss->m_session_start_time) - JULIANTIME_DIFF);
	}

	AMQPRoutingKey routingKey(SP_PERF_STAT, //category
		SP_NDCSPACKAGE, //package
		SP_INSTANCE, //scope
		SP_PUBLIC, //security
		SP_GPBPROTOCOL, //protocol
		PUB_SESSION_START_STATS); //publication
	debugPrint("AMQPRoutingKey(..., \"session_start_stats\")");

	//print protobuf string
	string message = "message\n" + sessionStartStats.DebugString();
	debugPrint(message.c_str());
    try {
	  rc = sendAMQPMessage(false, sessionStartStats.SerializeAsString(),
                           SP_CONTENT_TYPE_APP, routingKey, true);
	  if (rc != SP_SUCCESS) throw 1;
	} catch (...) {
	  if (rc == SP_SUCCESS) rc = SP_SEND_FAILED;
	  stringstream ss;
	  ss << ": sendTokenizedEvent() encountered error in sendAMQPMessage "
             << rc;
	  SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("sendAMQPMessage(..., sessionStartStats, ...)");

	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_SESSION_START& ss)",
		ss->m_functionCalled.c_str(), false);
}

//public overloaded function sendTokenizedEvent
void TokenizedEventMsg::sendTokenizedEvent(const QPID_SESSION_END* se)
{
	if (!m_enable)
		return;

	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_SESSION_END* se)", se->m_functionCalled.c_str());

	//declare event variables
	ndcs::session_end_stats sessionEndStats;
	common::info_header *sessionEndStatsInfoHeader = sessionEndStats.mutable_header();
	common::qpid_header *sessionEndStatsQpidHeader = sessionEndStatsInfoHeader->mutable_header();

	//populate event header fields
	//call wrapper to populate info header
	int rc = initAMQPInfoHeader(sessionEndStatsInfoHeader, SQEVL_TRANSDUCER);
	if (rc != SP_SUCCESS)
	{
		stringstream ss;
		ss << ": sendTokenizedEvent() encountered error in initAMQPInfoHeader " << rc;
		//throw ss.str().c_str();
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("initAMQPInfoHeader(sessionEndStatsInfoHeader, ...)");

	sessionEndStats.set_session_id(se->m_sessionId.c_str()); //required field
	sessionEndStats.set_session_status(se->m_session_status.c_str());
	sessionEndStats.set_total_odbc_execution_time(se->m_total_odbc_execution_time);
	sessionEndStats.set_total_odbc_elapsed_time(se->m_total_odbc_elapsed_time);
	sessionEndStats.set_total_insert_stmts_executed(se->m_total_insert_stmts_executed);
	sessionEndStats.set_total_delete_stmts_executed(se->m_total_delete_stmts_executed);
	sessionEndStats.set_total_update_stmts_executed(se->m_total_update_stmts_executed);
	sessionEndStats.set_total_select_stmts_executed(se->m_total_select_stmts_executed);
	sessionEndStats.set_total_catalog_stmts(se->m_total_catalog_stmts);
	sessionEndStats.set_total_prepares(se->m_total_prepares);
	sessionEndStats.set_total_executes(se->m_total_executes);
	sessionEndStats.set_total_fetches(se->m_total_fetches);
	sessionEndStats.set_total_closes(se->m_total_closes);
	sessionEndStats.set_total_execdirects(se->m_total_execdirects);
	sessionEndStats.set_total_errors(se->m_total_errors);
	sessionEndStats.set_total_warnings(se->m_total_warnings);

	if (getValidJulianTimestamp(se->m_session_end_time) != -1)
	{
		sessionEndStatsInfoHeader->set_info_generation_time_ts_utc(se->m_session_end_time - JULIANTIME_DIFF);
		sessionEndStatsInfoHeader->set_info_generation_time_ts_lct(utc2lct_useconds_jts(se->m_session_end_time) - JULIANTIME_DIFF);
	}

	AMQPRoutingKey routingKey(SP_PERF_STAT, //category
		SP_NDCSPACKAGE, //package
		SP_INSTANCE, //scope
		SP_PUBLIC, //security
		SP_GPBPROTOCOL, //protocol
		PUB_SESSION_END_STATS); //publication
	debugPrint("AMQPRoutingKey(..., \"session_end_stats\")");

	//print protobuf string
	string message = "message\n" + sessionEndStats.DebugString();
	debugPrint(message.c_str());
    try {
	  rc = sendAMQPMessage(false, sessionEndStats.SerializeAsString(),
	                       SP_CONTENT_TYPE_APP, routingKey, true);
	  if (rc != SP_SUCCESS) throw 1;
	} catch(...) {
      stringstream ss;
      if (rc == SP_SUCCESS) rc = SP_SEND_FAILED;
      ss << ": sendTokenizedEvent() encountered error in sendAMQPMessage "
         << rc;
      //throw ss.str().c_str();
      SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("sendAMQPMessage(..., sessionEndStats, ...)");

	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_SESSION_END* se)", se->m_functionCalled.c_str(), false);
}
// overload  sendTokenizedEvent
short TokenizedEventMsg::sendTokenizedEvent(const QPID_STATEMENT_SQL_TEXT* sst)
{
	const int FRAGMENT_SIZE = 14000; //SQL_TEXT columnn in QUERY_SQL_TEXT table defined as varchar(14,000)
	m_fragment_number = 0;

	//declare event variables
	debugPrint("TokenizedEventMsg::sendTokenizedEventSqlText(const QPID_STATEMENT_START_QUERYEXECUTION& sst)", sst->m_functionCalled.c_str());

	ndcs::query_sql_text querySqlText;
	common::info_header *querySqlTextInfoHeader = querySqlText.mutable_header();
	common::qpid_header *querySqlTextQpidHeader = querySqlTextInfoHeader->mutable_header();

	//populate event header fields
	//call wrapper to populate info header
	int rc2 = initAMQPInfoHeader(querySqlTextInfoHeader, SQEVL_TRANSDUCER);

	if (rc2 != SP_SUCCESS)
	{
		stringstream ss;
		ss << ": sendTokenizedEvent() encountered error in initAMQPInfoHeader " << rc2;
		//throw ss.str().c_str();
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("initAMQPInfoHeader(querySqlTextInfoHeader, ...)");

	querySqlText.set_query_id(sst->m_queryId); //required field
	if (getValidJulianTimestamp(sst->m_queryStartTime) != -1)
	{
		querySqlText.set_time_ts_utc(sst->m_queryStartTime - JULIANTIME_DIFF);
		querySqlText.set_time_ts_lct(utc2lct_useconds_jts(sst->m_queryStartTime) - JULIANTIME_DIFF);
	}

	debugPrint(sst->m_msgBuffer.c_str());

	AMQPRoutingKey routingKey2(SP_PERF_STAT, //category
		SP_NDCSPACKAGE, //package
		SP_INSTANCE, //scope
		SP_PUBLIC, //security
		SP_GPBPROTOCOL, //protocol
		PUB_QUERY_SQL_TEXT); //publication
	debugPrint("AMQPRoutingKey(..., \"query_sql_text\")");

	string sql_text = sst->m_msgBuffer;
	int sql_text_len = (int)sql_text.length();
	int pos = 0;
	int npos = 0;

	//2989 - M6:(Not Started) UNC gives error 8413 string argument cannot be converted when partial UTF8 characters appear in the SQL_TEXT preview
	do
	{
		querySqlText.set_fragment_number(m_fragment_number++);
		char *pc = (char *)sql_text.c_str();
		npos = getUTF8CharLength(pc + pos, sql_text_len, FRAGMENT_SIZE);
		if (npos == 0)
			break;
		string sql_text2 = sql_text.substr(pos, npos);
		querySqlText.set_sql_text(sql_text2);
		pos += npos;
		sql_text_len -= npos;

        try {
		  rc2 = sendAMQPMessage(false, querySqlText.SerializeAsString(),
                                SP_CONTENT_TYPE_APP, routingKey2, true);
		  if (rc2 != SP_SUCCESS) throw 1;
		} catch(...) {
          stringstream ss;
          if (rc2 == SP_SUCCESS) rc2 = SP_SEND_FAILED;
          ss << ": sendTokenizedEvent() encountered error in sendAMQPMessage " << rc2;
          //throw ss.str().c_str();
          SendErrorTextMsg(1,(char*)ss.str().c_str());
		  }
		debugPrint("sendAMQPMessage(..., querySqlText, ...)");
	} while (sql_text_len > 0);

	debugPrint("TokenizedEventMsg::sendTokenizedEventSqlText(const QPID_STATEMENT_START_QUERYEXECUTION& sst)", sst->m_functionCalled.c_str(), false);

	return m_fragment_number;
}
//public overloaded function sendTokenizedEvent
void TokenizedEventMsg::sendTokenizedEvent(const QPID_STATEMENT_END_QUERYEXECUTION* seqe)
{
	int const MAX_MSG_LENGTH = 254;
	if (!m_enable)
		return;

	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_STATEMENT_END_QUERYEXECUTION* seqe)", seqe->m_functionCalled.c_str());

	//declare event variables
	ndcs::query_end_stats queryEndStats;
	common::info_header *queryEndStatsInfoHeader = queryEndStats.mutable_header();
	common::qpid_header *queryEndStatsQpidHeader = queryEndStatsInfoHeader->mutable_header();

	//populate event header fields
	//call wrapper to populate info header
	int rc = initAMQPInfoHeader(queryEndStatsInfoHeader, SQEVL_TRANSDUCER);
	if (rc != SP_SUCCESS)
	{
		stringstream ss;
		ss << ": sendTokenizedEvent() encountered error in initAMQPInfoHeader " << rc;
		//throw ss.str().c_str();
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("initAMQPInfoHeader(queryEndStatsInfoHeader, ...)");

	queryEndStats.set_session_id(seqe->m_sessionId.c_str());
	queryEndStats.set_query_id(seqe->m_sqlUniqueQueryID.c_str()); //required field
	queryEndStats.set_statement_id(seqe->m_stmtName.c_str());
	queryEndStats.set_parent_query_id(seqe->m_parentQID.c_str());
	queryEndStats.set_transaction_id(seqe->m_transID.c_str());
	queryEndStats.set_statement_type(seqe->m_statementType.c_str());
	queryEndStats.set_client_id(seqe->m_clientId.c_str());
	queryEndStats.set_user_name(seqe->m_userName.c_str());
	queryEndStats.set_user_id(seqe->m_userId);
	queryEndStats.set_role_name(seqe->m_QSRoleName.c_str());
	queryEndStats.set_application_id(seqe->m_applicationId.c_str());
	queryEndStats.set_node_name(seqe->m_nodeName.c_str());
	queryEndStats.set_cpu_pin(seqe->m_cpuPin.c_str());
	queryEndStats.set_ds_name(seqe->m_DSName.c_str());
	queryEndStats.set_service_name(seqe->m_QSServiceName.c_str());
	queryEndStats.set_estimated_rows_accessed(seqe->m_estRowsAccessed);
	queryEndStats.set_estimated_rows_used(seqe->m_estRowsUsed);

	if (getValidJulianTimestamp(seqe->m_queryStartTime) != -1) //required field
	{
		queryEndStats.set_execute_start_time_ts_utc(seqe->m_queryStartTime - JULIANTIME_DIFF);
		queryEndStats.set_execute_start_time_ts_lct(utc2lct_useconds_jts(seqe->m_queryStartTime) - JULIANTIME_DIFF); //required field
	}
	else
	{
		stringstream ss;
		ss << "sendTokenizedEvent: queryEndStats seqe->m_functionCalle="<< seqe->m_functionCalled.c_str() << ", seqe->m_queryStartTime=" << seqe->m_queryStartTime;
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
//new fields start
	if (getValidJulianTimestamp(seqe->m_entryTime) != -1)
	{
		queryEndStats.set_entry_ts_utc(seqe->m_entryTime - JULIANTIME_DIFF);
		queryEndStats.set_entry_ts_lct(utc2lct_useconds_jts(seqe->m_entryTime)- JULIANTIME_DIFF);
	}
	queryEndStats.set_start_priority(seqe->m_currentPriority);
	if (getValidJulianTimestamp(seqe->m_prepareStartTime) != -1)
	{
		queryEndStats.set_cmp_start_time_ts_utc(seqe->m_prepareStartTime  - JULIANTIME_DIFF);
		queryEndStats.set_cmp_start_time_ts_lct(utc2lct_useconds_jts(seqe->m_prepareStartTime) - JULIANTIME_DIFF);
	}
	if (getValidJulianTimestamp(seqe->m_prepareEndTime) != -1)
	{
		queryEndStats.set_cmp_end_time_ts_utc(seqe->m_prepareEndTime - JULIANTIME_DIFF);
		queryEndStats.set_cmp_end_time_ts_lct(utc2lct_useconds_jts(seqe->m_prepareEndTime)  - JULIANTIME_DIFF);
	}
	queryEndStats.set_cmp_time(seqe->m_prepareTime);
	queryEndStats.set_estimated_cost(seqe->m_est_cost);
	queryEndStats.set_estimated_cardinality(seqe->m_cardinality);
	queryEndStats.set_estimated_total_time(seqe->m_est_totalTime);
	queryEndStats.set_estimated_io_time(seqe->m_ioTime);
	queryEndStats.set_estimated_msg_time(seqe->m_msgTime);
	queryEndStats.set_estimated_idle_time(seqe->m_idleTime);
	queryEndStats.set_estimated_cpu_time(seqe->m_cpuTime);
	queryEndStats.set_estimated_total_memory(seqe->m_estTotalMem);
	queryEndStats.set_estimated_resource_usage(seqe->m_resourceUsage);
	// Added new NCM counters for Sprint-3
	queryEndStats.set_estimated_num_seq_ios(seqe->m_estNumSeqIOs);
	queryEndStats.set_estimated_num_rand_ios(seqe->m_estNumRandIOs);

	if (seqe->m_affinityNumber != -1)
		queryEndStats.set_cmp_affinity_num(seqe->m_affinityNumber);
	queryEndStats.set_cmp_dop(seqe->m_dop);
	queryEndStats.set_cmp_txn_needed(seqe->m_xnNeeded);
	queryEndStats.set_cmp_mandate_x_prod(seqe->m_mandatoryCrossProduct);
	queryEndStats.set_cmp_missing_stats(seqe->m_missingStats);
	queryEndStats.set_cmp_num_joins(seqe->m_numOfJoins);
	queryEndStats.set_cmp_full_scan_table(seqe->m_fullScanOnTable);
	queryEndStats.set_cmp_high_eid_max_buf_usage(seqe->m_highDp2MxBufferUsage);
	if (seqe->m_rowsAccessedForFullScan != -1)
		queryEndStats.set_cmp_rows_accessed_full_scan(seqe->m_rowsAccessedForFullScan);
	if (seqe->m_dp2RowsAccessed != -1)
		queryEndStats.set_cmp_eid_rows_accessed(seqe->m_dp2RowsAccessed);
	if (seqe->m_dp2RowsUsed != -1)
		queryEndStats.set_cmp_eid_rows_used(seqe->m_dp2RowsUsed);
	int len = seqe->m_msgBuffer.length();
	char msgBuff[MAX_MSG_LENGTH + 1];
	if (len > MAX_MSG_LENGTH)
	{
		len = getUTF8CharLength(seqe->m_msgBuffer.c_str(), len, MAX_MSG_LENGTH);
		if (len > 0 && len <= MAX_MSG_LENGTH)
		{
			strncpy(msgBuff, seqe->m_msgBuffer.c_str(), len);
			msgBuff[len] = '\0';
		}
		else
		{
			strcpy(msgBuff, "<N/A>");
		}
		queryEndStats.set_sql_text(msgBuff);
		queryEndStats.set_sql_text_overflow_indicator(m_fragment_number);
		m_fragment_number=0;
	}
	else
	{
		queryEndStats.set_sql_text(seqe->m_msgBuffer);
		queryEndStats.set_sql_text_overflow_indicator(0);
	}

	//additional R2.5 counters
	queryEndStats.set_cmp_compiler_id(seqe->m_compilerId.c_str());
	queryEndStats.set_cmp_cpu_path_length(seqe->m_cmpCpuTotal);
	queryEndStats.set_cmp_cpu_binder(seqe->m_cmpCpuBinder);
	queryEndStats.set_cmp_cpu_normalizer(seqe->m_cmpCpuNormalizer);
	queryEndStats.set_cmp_cpu_analyzer(seqe->m_cmpCpuAnalyzer);
	queryEndStats.set_cmp_cpu_optimizer(seqe->m_cmpCpuOptimizer);
	queryEndStats.set_cmp_cpu_generator(seqe->m_cmpCpuGenerator);
	queryEndStats.set_cmp_metadata_cache_hits(seqe->m_metadataCacheHits);
	queryEndStats.set_cmp_metadata_cache_lookups(seqe->m_metadataCacheLookups);
	queryEndStats.set_cmp_query_cache_status(seqe->m_queryCacheState);
	queryEndStats.set_cmp_histogram_cache_hits(seqe->m_histogramCacheHits);
	queryEndStats.set_cmp_histogram_cache_lookups(seqe->m_histogramCacheLookups);
	queryEndStats.set_cmp_stmt_heap_size(seqe->m_stmtHeapSize);
	queryEndStats.set_cmp_context_heap_size(seqe->m_cxtHeapSize);
	queryEndStats.set_cmp_optimization_tasks(seqe->m_optTasks);
	queryEndStats.set_cmp_optimization_contexts(seqe->m_optContexts);
	queryEndStats.set_cmp_is_recompile(seqe->m_isRecompile);
	queryEndStats.set_connection_rule(seqe->m_con_rule_name.c_str());
	queryEndStats.set_compilation_rule(seqe->m_cmp_rule_name.c_str());
	queryEndStats.set_aggregate_option(seqe->m_aggregation.c_str());
	//additional R2.5 counters

	queryEndStats.set_cmp_number_of_bmos(seqe->m_cmp_number_of_bmos); //ssd overflow
	if (seqe->m_cmp_overflow_mode == 0)
		queryEndStats.set_cmp_overflow_mode(OVERFLOW_DISK);
	else if (seqe->m_cmp_overflow_mode == 1)
		queryEndStats.set_cmp_overflow_mode(OVERFLOW_SSD);
	queryEndStats.set_cmp_overflow_size(seqe->m_cmp_overflow_size);

//new fields end
	if (getValidJulianTimestamp(seqe->m_inexeEndTime) != -1)
	{
		queryEndStats.set_time_ts_utc(seqe->m_inexeEndTime - JULIANTIME_DIFF);
		queryEndStats.set_time_ts_lct(utc2lct_useconds_jts(seqe->m_inexeEndTime) - JULIANTIME_DIFF);
	}
	queryEndStats.set_odbc_elapsed_time(seqe->m_inqueryElapseTime);
	queryEndStats.set_odbc_execution_time(seqe->m_inqueryExecutionTime);
	if (getValidJulianTimestamp(seqe->m_firstRowReturnTime) != -1)
	{
		queryEndStats.set_first_result_return_time_ts_utc(seqe->m_firstRowReturnTime - JULIANTIME_DIFF);
		queryEndStats.set_first_result_return_time_ts_lct(utc2lct_useconds_jts(seqe->m_firstRowReturnTime) - JULIANTIME_DIFF);
	}
	if (seqe->m_rowsReturned != -1)
		queryEndStats.set_rows_returned(seqe->m_rowsReturned);
	queryEndStats.set_sql_process_busy_time(seqe->m_ProcessBusyTime);
	queryEndStats.set_num_sql_process(seqe->m_numSqlProcs);
	if (seqe->m_numberOfRows != -1)
		queryEndStats.set_number_of_rows(seqe->m_numberOfRows);
	queryEndStats.set_error_code(seqe->m_errorCode);
	queryEndStats.set_sql_error_code(seqe->m_sqlErrorCode);
	queryEndStats.set_stats_error_code(seqe->m_statsErrorCode);
	queryEndStats.set_aqr_last_error(seqe->m_AQRlastError);
	queryEndStats.set_aqr_num_retries(seqe->m_AQRnumRetries);
	queryEndStats.set_aqr_delay_before_retry(seqe->m_AQRdelayBeforeRetry);

	if (getValidJulianTimestamp(seqe->m_WMSstartTS) != -1)
	{
		//m_WMSstartTS is in local time
		int64 wms_start_time_lct = seqe->m_WMSstartTS - JULIANTIME_DIFF;
		queryEndStats.set_wms_start_time_ts_lct(wms_start_time_lct);
		int64 wms_start_time_utc = lct2utc_useconds(wms_start_time_lct);
		queryEndStats.set_wms_start_time_ts_utc(wms_start_time_utc);
	}

	queryEndStats.set_exec_time(seqe->m_execTime);
	queryEndStats.set_wait_time(seqe->m_waitTime);
	queryEndStats.set_hold_time(seqe->m_holdTime);
	queryEndStats.set_suspend_time(seqe->m_suspendTime);
	queryEndStats.set_open_busy_time(seqe->m_OpenTime);
	queryEndStats.set_num_opens(seqe->m_Opens);
	queryEndStats.set_processes_created(seqe->m_NewProcess);
	queryEndStats.set_process_create_busy_time(seqe->m_NewProcessTime);
	if (seqe->m_AccessedRows != -1)
		queryEndStats.set_rows_accessed(seqe->m_AccessedRows);
	if (seqe->m_UsedRows != -1)
		queryEndStats.set_rows_retrieved(seqe->m_UsedRows);
	if (seqe->m_DiskProcessBusyTime != -1)
		queryEndStats.set_disc_process_busy_time(seqe->m_DiskProcessBusyTime);
	if (seqe->m_DiskIOs != -1)
		queryEndStats.set_disc_reads(seqe->m_DiskIOs);
	if (seqe->m_SpaceTotal != -1)
		queryEndStats.set_space_total(seqe->m_SpaceTotal);
	if (seqe->m_SpaceUsed != -1)
		queryEndStats.set_space_used(seqe->m_SpaceUsed);
	if (seqe->m_HeapTotal != -1)
		queryEndStats.set_heap_total(seqe->m_HeapTotal);
	if (seqe->m_HeapUsed != -1)
		queryEndStats.set_heap_used(seqe->m_HeapUsed);
	if (seqe->m_TotalMemAlloc != -1)
		queryEndStats.set_total_memory(seqe->m_TotalMemAlloc);
	if (seqe->m_MaxMemUsed != -1)
		queryEndStats.set_max_memory_used(seqe->m_MaxMemUsed);
	if (seqe->m_Dp2SpaceTotal != -1)
		queryEndStats.set_eid_space_total(seqe->m_Dp2SpaceTotal);
	if (seqe->m_Dp2SpaceUsed != -1)
		queryEndStats.set_eid_space_used(seqe->m_Dp2SpaceUsed);
	if (seqe->m_Dp2HeapTotal != -1)
		queryEndStats.set_eid_heap_total(seqe->m_Dp2HeapTotal);
	if (seqe->m_Dp2HeapUsed != -1)
		queryEndStats.set_eid_heap_used(seqe->m_Dp2HeapUsed);
	if (seqe->m_NumMessages != -1)
		queryEndStats.set_msgs_to_disc(seqe->m_NumMessages);
	if (seqe->m_MessagesBytes != -1)
		queryEndStats.set_msgs_bytes_to_disc(seqe->m_MessagesBytes);
	if (seqe->m_reqMsgCnt != -1)
		queryEndStats.set_num_rqst_msgs(seqe->m_reqMsgCnt);
	if (seqe->m_reqMsgBytes != -1)
		queryEndStats.set_num_rqst_msg_bytes(seqe->m_reqMsgBytes);
	if (seqe->m_replyMsgCnt != -1)
		queryEndStats.set_num_rply_msgs(seqe->m_replyMsgCnt);
	if (seqe->m_replyMsgBytes != -1)
		queryEndStats.set_num_rply_msg_bytes(seqe->m_replyMsgBytes);
	if (seqe->m_LockWaits != -1)
		queryEndStats.set_lock_waits(seqe->m_LockWaits);
	if (seqe->m_Escalations != -1)
		queryEndStats.set_lock_escalation(seqe->m_Escalations);
	queryEndStats.set_query_state(seqe->m_queryState.c_str());
	queryEndStats.set_query_sub_state(seqe->m_querySubstate.c_str());
	queryEndStats.set_warn_level(seqe->m_WarnLevel.c_str());
	queryEndStats.set_exec_state(seqe->m_SQLState.c_str());
	//queryEndStats.set_total_executes(seqe->m_totalStatementExecutes);
	queryEndStats.set_error_text(seqe->m_errorText);
	queryEndStats.set_execution_rule(seqe->m_exe_rule_name.c_str());

	//additional R2.5 counters
	if (seqe->m_TotalAggregates != -1)
		queryEndStats.set_aggregate_total(seqe->m_TotalAggregates);
	//additional R2.5 counters

	queryEndStats.set_ovf_file_count(seqe->m_ovf_file_count); //ssd overflow
	queryEndStats.set_ovf_space_allocated(seqe->m_ovf_space_allocated);
	queryEndStats.set_ovf_space_used(seqe->m_ovf_space_used);
	queryEndStats.set_ovf_block_size(seqe->m_ovf_block_size);
	queryEndStats.set_ovf_ios(seqe->m_ovf_ios);
	queryEndStats.set_ovf_message_buffers_to(seqe->m_ovf_message_buffers_to);
	queryEndStats.set_ovf_message_to(seqe->m_ovf_message_to);
	queryEndStats.set_ovf_message_bytes_to(seqe->m_ovf_message_bytes_to);
	queryEndStats.set_ovf_message_buffers_out(seqe->m_ovf_message_buffers_out);
	queryEndStats.set_ovf_message_out(seqe->m_ovf_message_out);
	queryEndStats.set_ovf_message_bytes_out(seqe->m_ovf_message_bytes_out);

	if (getValidJulianTimestamp(seqe->m_suspended_ts) != -1) //3289
	{
		int64 suspended_ts_lct = seqe->m_suspended_ts - JULIANTIME_DIFF;
		queryEndStats.set_suspended_ts_lct(suspended_ts_lct);
		int64 suspend_ts_utc = lct2utc_useconds(suspended_ts_lct);
		queryEndStats.set_suspended_ts_utc(suspend_ts_utc);
	}
	if (getValidJulianTimestamp(seqe->m_released_ts) != -1)
	{
		int64 released_ts_lct = seqe->m_released_ts - JULIANTIME_DIFF;
		queryEndStats.set_released_ts_lct(released_ts_lct);
		int64 released_ts_utc = lct2utc_useconds(released_ts_lct);
		queryEndStats.set_released_ts_utc(released_ts_utc);
	}
	if (getValidJulianTimestamp(seqe->m_cancelled_ts) != -1)
	{
		int64 cancelled_ts_lct = seqe->m_cancelled_ts - JULIANTIME_DIFF;
		queryEndStats.set_cancelled_ts_lct(cancelled_ts_lct);
		int64 cancelled_ts_utc = lct2utc_useconds(cancelled_ts_lct);
		queryEndStats.set_cancelled_ts_utc(cancelled_ts_utc);
	}

	queryEndStats.set_num_cpus(seqe->m_numCpus);
	queryEndStats.set_udr_process_busy_time(seqe->m_UDRProcessBusyTime);
	queryEndStats.set_query_subtype(seqe->m_QuerySubType);
	queryEndStats.set_parent_system_name(seqe->m_ParentSystemName);
	queryEndStats.set_pertable_stats(seqe->m_pertable_stats == true? 1 : 0);

	AMQPRoutingKey routingKey(SP_PERF_STAT, //category
		SP_NDCSPACKAGE, //package
		SP_INSTANCE, //scope
		SP_PUBLIC, //security
		SP_GPBPROTOCOL, //protocol
		PUB_QUERY_END_STATS); //publication
	debugPrint("AMQPRoutingKey(..., \"query_end_stats\")");

	//print protobuf string
	string message = "message\n" + queryEndStats.DebugString();
	debugPrint(message.c_str());
    try {
	  rc = sendAMQPMessage(false, queryEndStats.SerializeAsString(),
                           SP_CONTENT_TYPE_APP, routingKey, true);
	  if (rc != SP_SUCCESS) throw 1;
    } catch(...) {
      stringstream ss;
      if (rc == SP_SUCCESS) rc = SP_SEND_FAILED;
      ss << ": sendTokenizedEvent() encountered error in sendAMQPMessage "
         << rc;
      //throw ss.str().c_str();
      SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("sendAMQPMessage(..., queryEndStats, ...)");

	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_STATEMENT_END_QUERYEXECUTION& seqe)", seqe->m_functionCalled.c_str(), false);
}

void TokenizedEventMsg::sendTokenizedEvent(const QPID_WMS_STATS* ws)
{
	if (!m_enable)
		return;

	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_WMS_STATS* ws)", ws->m_functionCalled.c_str());

	//declare event variables
	wms::wms_stats wmsStats;
	common::info_header *wmsStatsInfoHeader = wmsStats.mutable_header();
	common::qpid_header *wmsStatsQpidHeader = wmsStatsInfoHeader->mutable_header();

	//call wrapper to populate info header
	int rc = initAMQPInfoHeader(wmsStatsInfoHeader, SQEVL_TRANSDUCER);
	if (rc != SP_SUCCESS)
	{
		stringstream ss;
		ss << ": sendTokenizedEvent() encountered error in initAMQPInfoHeader " << rc;
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("initAMQPInfoHeader(wmsStatsInfoHeader, ...)");

	wmsStats.set_wms_node_id(ws->m_node_id);
	wmsStats.set_wms_node_name(ws->m_node_name.c_str());
	wmsStats.set_wms_node_list(ws->m_node_list.c_str());
	wmsStats.set_total_queries(ws->m_total_queries);
	wmsStats.set_total_exec(ws->m_total_exec);
	wmsStats.set_total_wait(ws->m_total_wait);
	wmsStats.set_total_hold(ws->m_total_hold);
	wmsStats.set_total_suspend(ws->m_total_suspend);
	wmsStats.set_total_reject(ws->m_total_reject);
	wmsStats.set_total_cancel(ws->m_total_cancel);
	wmsStats.set_total_complete(ws->m_total_complete);
	wmsStats.set_avg_exec_secs(ws->m_avg_exec_secs);
	wmsStats.set_avg_wait_secs(ws->m_avg_wait_secs);
	wmsStats.set_avg_hold_secs(ws->m_avg_hold_secs);
	wmsStats.set_avg_suspend_secs(ws->m_avg_suspend_secs);
	wmsStats.set_conn_rule_triggered(ws->m_conn_rule);
	wmsStats.set_comp_rule_triggered(ws->m_comp_rule);
	wmsStats.set_exec_rule_triggered(ws->m_exec_rule);
	wmsStats.set_cur_esps(ws->m_cur_esps);
	int64 begin_ts_lct = ws->m_begin_ts_lct - JULIANTIME_DIFF;
	wmsStats.set_begin_ts_lct(begin_ts_lct); //required field
	int64 begin_ts_utc = lct2utc_useconds(begin_ts_lct);
	wmsStats.set_begin_ts_utc(begin_ts_utc); //required field
	int64 end_ts_lct = ws->m_end_ts_lct - JULIANTIME_DIFF;
	wmsStats.set_end_ts_lct(end_ts_lct); //required field
	int64 end_ts_utc = lct2utc_useconds(end_ts_lct);
	wmsStats.set_end_ts_utc(end_ts_utc); //required field

	//create a routing key
	AMQPRoutingKey routingKey(
		SP_PERF_STAT, //category
		SP_WMSPACKAGE, //package
		SP_INSTANCE, //scope
		SP_PUBLIC, //security
		SP_GPBPROTOCOL, //protocol
		PUB_WMS_STATS //publication
		);
	debugPrint("AMQPRoutingKey(..., \"wms_stats\")");

	//print protobuf string
	string message = "message\n" + wmsStats.DebugString();
	debugPrint(message.c_str());

	//send message to QPID asynchronously
	try {
	  rc = sendAMQPMessage(false, wmsStats.SerializeAsString(),
                           SP_CONTENT_TYPE_APP, routingKey, true);
	  if (rc != SP_SUCCESS) throw 1;
	} catch(...) {
      stringstream ss;
      if (rc == SP_SUCCESS) rc = SP_SEND_FAILED;
      ss << ": sendTokenizedEvent() encountered error in sendAMQPMessage "
         << rc;
      SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("sendAMQPMessage(..., wmsStats, ...)");

	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_WMS_STATS& ws)", ws->m_functionCalled.c_str(), false);
}

void TokenizedEventMsg::sendTokenizedEvent(const QPID_WMS_RESOURCES* wr)
{
	if (!m_enable)
		return;

	//declare event variables
	wms::wms_resources wmsResources;
	common::info_header *wmsResourcesInfoHeader = wmsResources.mutable_header();
	common::qpid_header *wmsResourcesQpidHeader = wmsResourcesInfoHeader->mutable_header();

	//call wrapper to populate info header
	int rc = initAMQPInfoHeader(wmsResourcesInfoHeader, SQEVL_TRANSDUCER);
	if (rc != SP_SUCCESS)
	{
		stringstream ss;
		ss << ": sendTokenizedEvent() encountered error in initAMQPInfoHeader " << rc;
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("initAMQPInfoHeader(wmsResourcesInfoHeader, ...)");

	wmsResources.set_cpu_busy(wr->m_cpu_busy);
	wmsResources.set_memory_usage(wr->m_memory_usage);
	wmsResources.set_ssd_usage(wr->m_ssd_usage);
	wmsResources.set_max_node_esps(wr->m_max_node_esps);
	wmsResources.set_avg_node_esps(wr->m_avg_node_esps);
	int64 current_ts_lct = wr->m_current_ts_lct - JULIANTIME_DIFF;
	wmsResources.set_current_ts_lct(current_ts_lct); //required field
	int64 current_ts_utc = lct2utc_useconds(current_ts_lct);
	wmsResources.set_current_ts_utc(current_ts_utc); //required field

	//create a routing key
	AMQPRoutingKey routingKey(
		SP_PERF_STAT, //category
		SP_WMSPACKAGE, //package
		SP_INSTANCE, //scope
		SP_PUBLIC, //security
		SP_GPBPROTOCOL, //protocol
		PUB_WMS_RESOURCES //publication
		);
	debugPrint("AMQPRoutingKey(..., \"wms_resources\")");

	//print protobuf string
	string message = "message\n" + wmsResources.DebugString();
	debugPrint(message.c_str());

	//send message to QPID asynchronously
	try {
	  rc = sendAMQPMessage(false, wmsResources.SerializeAsString(),
                           SP_CONTENT_TYPE_APP, routingKey, true);
	  if (rc != SP_SUCCESS) throw 1;
	} catch(...) {
      stringstream ss;
      if (rc == SP_SUCCESS) rc = SP_SEND_FAILED;
      ss << ": sendTokenizedEvent() encountered error in sendAMQPMessage "
         << rc;
      SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("sendAMQPMessage(..., wmsResources, ...)");

	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_WMS_RESOURCES& wr)", wr->m_functionCalled.c_str(), false);
}

void TokenizedEventMsg::sendTokenizedEvent(const QPID_WMS_PERTABLESTATS* wps)
{
	if (!m_enable)
		return;

	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_WMS_PERTABLESTATS* wps)", wps->m_functionCalled.c_str());

	//declare event variables
	wms::wms_pertablestats wmsPertableStats;
	common::info_header *wmsPertableStatsInfoHeader = wmsPertableStats.mutable_header();
	common::qpid_header *wmsPertableStatsQpidHeader = wmsPertableStatsInfoHeader->mutable_header();

	//call wrapper to populate info header
	int rc = initAMQPInfoHeader(wmsPertableStatsInfoHeader, SQEVL_TRANSDUCER);
	if (rc != SP_SUCCESS)
	{
		stringstream ss;
		ss << ": sendTokenizedEvent() encountered error in initAMQPInfoHeader " << rc;
		SendErrorTextMsg(1,(char*)ss.str().c_str());
	}
	debugPrint("initAMQPInfoHeader(wmsPertableStatsInfoHeader, ...)");

	wmsPertableStats.set_query_id(wps->m_queryId);
	wmsPertableStats.set_table_ansi_name(wps->m_tblName);
	wmsPertableStats.set_accessed_rows(wps->m_accessedRows);
	wmsPertableStats.set_used_rows(wps->m_usedRows);
	wmsPertableStats.set_disk_ios(wps->m_diskIOs);
	wmsPertableStats.set_num_messages(wps->m_numMessages);
	wmsPertableStats.set_messages_bytes(wps->m_messagesBytes);
	wmsPertableStats.set_escalations(wps->m_escalations);
	wmsPertableStats.set_lock_waits(wps->m_lockWaits);
	wmsPertableStats.set_process_busy_time(wps->m_processBusyTime);
	wmsPertableStats.set_opens(wps->m_opens);
	wmsPertableStats.set_open_time(wps->m_openTime);
	int64 current_ts_lct = wps->m_current_ts_lct - JULIANTIME_DIFF;
	wmsPertableStats.set_current_ts_lct(current_ts_lct);
	int64 current_ts_utc = lct2utc_useconds(current_ts_lct);
	wmsPertableStats.set_current_ts_utc(current_ts_utc);

	//create a routing key
	AMQPRoutingKey routingKey(
		SP_PERF_STAT, //category
		SP_WMSPACKAGE, //package
		SP_INSTANCE, //scope
		SP_PUBLIC, //security
		SP_GPBPROTOCOL, //protocol
		PUB_WMS_PERTABLESTATS //publication
		);
	debugPrint("AMQPRoutingKey(..., \"wms_pertable_stats\")");

	//print protobuf string
	string message = "message\n" + wmsPertableStats.DebugString();
	debugPrint(message.c_str());

	//send message to QPID asynchronously
	try {
	  rc = sendAMQPMessage(false, wmsPertableStats.SerializeAsString(),
                           SP_CONTENT_TYPE_APP, routingKey, true);
	  if (rc != SP_SUCCESS) throw 1;
	} catch(...) {
      stringstream ss;
      if (rc == SP_SUCCESS) rc = SP_SEND_FAILED;
      ss << ": sendTokenizedEvent() encountered error in sendAMQPMessage "
         << rc;
      SendErrorTextMsg(1,(char*)ss.str().c_str());
	}

	debugPrint("sendAMQPMessage(..., wmsPertableStats, ...)");

	debugPrint("TokenizedEventMsg::sendTokenizedEvent(const QPID_WMS_RESOURCES& wps)", wps->m_functionCalled.c_str(), false);
}
