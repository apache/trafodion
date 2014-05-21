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

#ifndef WMS_EVENTS_H
#define WMS_EVENTS_H

#define DEFAULT_QPID_PERFSTATS 1
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>

#include "sqevlog/evl_sqlog_writer.h"
#include "wrapper/amqpwrapper.h"
#include "wrapper/routingkey.h"
#include "wrapper/externroutingkey.h"
#include "common.text_event.pb.h"
#include "ndcs.query_start_stats.pb.h"
#include "ndcs.query_end_stats.pb.h"
#include "ndcs.query_sql_text.pb.h"
#include "ndcs.session_start_stats.pb.h"
#include "ndcs.session_end_stats.pb.h"
#include "wms.wms_stats.pb.h"
#include "wms.wms_resources.pb.h"
#include "wms.wms_pertablestats.pb.h"

#include <qpid/client/Connection.h>
#include <qpid/client/Session.h>
#include <qpid/client/Message.h>
#include <qpid/client/SubscriptionManager.h>
#include <qpid/client/LocalQueue.h>

#include <platform_ndcs.h>
#include "Global.h"
#include "QSGlobal.h"
#include "tdm_odbcSrvrMsg.h"

using namespace std;
using namespace qpid;

static const int   K_BYTES = 1024;
static const int   M_BYTES = K_BYTES * K_BYTES;
static const int64 TMP_FILE_SIZE = M_BYTES * 10;
static const char *DEBUG_QPID_PERFSTATS = "DEBUG_QPID_PERFSTATS";
static const char *ENABLE_QPID_PERFSTATS = "ENABLE_QPID_PERFSTATS";
static const char *QPID_PERFSTATS_FILENAME = "/tmp/qpid_perfstats.out";
static const char *QPID_PERFSTATS_BACK_FILENAME = "/tmp/qpid_perfstats.old";
static const char *QPID_NODE_PORT_ENV = "QPID_NODE_PORT";
static const char *DEFAULT_QPID_NODE_IP = "127.0.0.1";
static const char *PUB_QUERY_START_STATS = "query_start_stats";
static const char *PUB_QUERY_END_STATS = "query_end_stats";
static const char *PUB_QUERY_SQL_TEXT = "query_sql_text";
static const char *PUB_SESSION_START_STATS = "session_start_stats";
static const char *PUB_SESSION_END_STATS = "session_end_stats";
static const char *PUB_WMS_STATS = "wms_stats";
static const char *PUB_WMS_RESOURCES = "wms_resources";
static const char *PUB_WMS_PERTABLESTATS = "wms_pertablestats";

#define JULIANTIME_DIFF 210866760000000000LL

#include "QpidQueryStats.h"
#include "WmsEventsUtil.h"
#include "WmsEventsExportDll.h"
#include "WmsEventsTokenizedDll.h"
#include "WmsEventsLogDll.h"

#endif
