/**********************************************************************
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
********************************************************************/

#include "WmsEvents.h"

//
static void
sendTokenizedEvent(const QPID_SESSION_START* ss)
{
	try
	{
		TokenizedEventMsg *evt = TokenizedEventMsg::getInstance();
		evt->sendTokenizedEvent(ss);
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in sendTokenizedEvent " << ex << endl;
	}
}

static void
sendTokenizedEvent(const QPID_SESSION_END* se)
{
	try
	{
		TokenizedEventMsg *evt = TokenizedEventMsg::getInstance();
		evt->sendTokenizedEvent(se);
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in sendTokenizedEvent " << ex << endl;
	}
}
/*
static void
sendTokenizedEvent(const QPID_STATEMENT_START_QUERYEXECUTION* ssqe)
{
	try
	{
		TokenizedEventMsg *evt = TokenizedEventMsg::getInstance();
		evt->sendTokenizedEvent(ssqe);
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in sendTokenizedEvent " << ex << endl;
	}
}
*/

static void
sendTokenizedEvent(const QPID_STATEMENT_SQL_TEXT* sst)
{
	try
	{
		TokenizedEventMsg *evt = TokenizedEventMsg::getInstance();
		evt->sendTokenizedEvent(sst);
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in sendTokenizedEvent " << ex << endl;
	}
}

static void
sendTokenizedEvent(const QPID_STATEMENT_END_QUERYEXECUTION* seqe)
{
	try
	{
		TokenizedEventMsg *evt = TokenizedEventMsg::getInstance();
		evt->sendTokenizedEvent(seqe);
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in sendTokenizedEvent " << ex << endl;
	}
}

static void
sendTokenizedEvent(const QPID_WMS_STATS* ws)
{
	try
	{
		TokenizedEventMsg *evt = TokenizedEventMsg::getInstance();
		evt->sendTokenizedEvent(ws);
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in sendTokenizedEvent " << ex << endl;
	}
}

static void
sendTokenizedEvent(const QPID_WMS_RESOURCES* wr)
{
	try
	{
		TokenizedEventMsg *evt = TokenizedEventMsg::getInstance();
		evt->sendTokenizedEvent(wr);
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in sendTokenizedEvent " << ex << endl;
	}
}
static void
sendTokenizedEvent(const QPID_WMS_PERTABLESTATS* wps)
{
	try
	{
		TokenizedEventMsg *evt = TokenizedEventMsg::getInstance();
		evt->sendTokenizedEvent(wps);
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in sendTokenizedEvent " << ex << endl;
	}
}

extern "C" void
sendTokenizedEvent(qpid_struct_type qst, void* qs)
{
	const QPID_SESSION_START* ss = (const QPID_SESSION_START*)qs;
	const QPID_SESSION_END* se = (const QPID_SESSION_END*)qs;
//	const QPID_STATEMENT_START_QUERYEXECUTION* ssqe = (const QPID_STATEMENT_START_QUERYEXECUTION*)qs;
	const QPID_STATEMENT_SQL_TEXT* sst = (const QPID_STATEMENT_SQL_TEXT*)qs;
	const QPID_STATEMENT_END_QUERYEXECUTION* seqe = (const QPID_STATEMENT_END_QUERYEXECUTION*)qs;
	const QPID_WMS_STATS* ws = (const QPID_WMS_STATS*)qs;
	const QPID_WMS_RESOURCES* wr = (const QPID_WMS_RESOURCES*)qs;
	const QPID_WMS_PERTABLESTATS* wps = (const QPID_WMS_PERTABLESTATS*)qs;

	switch (qst){
	case QPID_TYPE_SESSION_START:
		sendTokenizedEvent(ss);
		break;
	case QPID_TYPE_SESSION_END:
		sendTokenizedEvent(se);
		break;
	case QPID_TYPE_STATEMENT_SQL_TEXT:
		sendTokenizedEvent(sst);
		break;
/*
	case QPID_TYPE_STATEMENT_START_QUERYEXECUTION:
		sendTokenizedEvent(ssqe);
		break;
*/
	case QPID_TYPE_STATEMENT_END_QUERYEXECUTION:
		sendTokenizedEvent(seqe);
		break;
	case QPID_TYPE_WMS_STATS:
		sendTokenizedEvent(ws);
		break;
	case QPID_TYPE_WMS_RESOURCES:
		sendTokenizedEvent(wr);
		break;
	case QPID_TYPE_WMS_PERTABLESTATS:
		sendTokenizedEvent(wps);
		break;
	}
}
//
//
extern "C" void
sendToEventLog (short evt_num, short EventLogType, char *ComponentName, char *ObjectRef, short nToken, va_list marker)
{
	try
	{
		send_to_eventlog (evt_num, EventLogType, ComponentName, ObjectRef, nToken, marker);
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in sendToEventLog " << ex << endl;
	}
}
extern "C"void
setCriticalDialoutDll()
{
	try
	{
		set_critical_dialout();
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in setCriticalDialoutDll " << ex << endl;
	}
}

extern "C"void
setIsWmsDll()
{
	try
	{
		set_is_wms();
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in setIsWmsDll " << ex << endl;
	}
}

extern "C"void
setTraceVariables(bool trace_ems_dll, bool trace_legacy_dll)
{
	try
	{
		set_trace_variables(trace_ems_dll, trace_legacy_dll );
	}
	catch (string ex)
	{
		cerr << "Exception: occurred in setTraceVariables " << ex << endl;
	}
}

