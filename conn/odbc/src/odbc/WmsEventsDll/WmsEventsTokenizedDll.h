// ===============================================================================================
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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
// ===============================================================================================
#ifndef WMS_EVENTS_TOKENIZED_DLL
#define WMS_EVENTS_TOKENIZED_DLL

class TokenizedEventMsg
{
public:
	static TokenizedEventMsg *getInstance(const string broker_ip = DEFAULT_QPID_NODE_IP);
	int getUTF8CharLength(const char *inputChar, const int inputLength, const int maxLength);
	bool debug() const { return m_debug; }
	bool enable() const { return m_enable; }
	void sendTokenizedEvent(const QPID_SESSION_START* ss);
	void sendTokenizedEvent(const QPID_SESSION_END* se);
	short sendTokenizedEvent(const QPID_STATEMENT_SQL_TEXT* sst);
	void sendTokenizedEvent(const QPID_STATEMENT_END_QUERYEXECUTION* seqe);
	void sendTokenizedEvent(const QPID_WMS_STATS* ws);
	void sendTokenizedEvent(const QPID_WMS_RESOURCES* wr);
	void sendTokenizedEvent(const QPID_WMS_PERTABLESTATS* wps);
//LCOV_EXCL_START
	void debugPrint(const char *name, const char *name2, bool enterFunction = true);
	void debugPrint(const char *msg);
//LCOV_EXCL_STOP
	void SendErrorTextMsg(short nToken, ...);

private:
	TokenizedEventMsg(const string broker_ip = DEFAULT_QPID_NODE_IP);
	~TokenizedEventMsg();

private:
	char* srvrObjRef;
	static TokenizedEventMsg *m_event;
	string m_broker_ip;
	int m_broker_port;
	bool m_debug;
	bool m_enable;
	short m_fragment_number;
};

#endif
