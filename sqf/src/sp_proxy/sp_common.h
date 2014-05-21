// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef SP_COMMON_H_
#define SP_COMMON_H_

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

#include <time.h>       // for asctime
#include <sys/time.h>   // for gettimeofday

#include "common/evl_sqlog_eventnum.h"
#include "seabed/trace.h"

#include <sstream>
#include <qpid/messaging/Address.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Session.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/exceptions.h>
#include <qpid/types/Variant.h>
#include <qpid/Msg.h>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace qpid::messaging;
using namespace qpid::types;
using namespace boost;

#define SP_DO_ROUTE "SP_DO_ROUTE"
#define SP_IP_ADDRESS "SP_IP_ADDRESS"
#define SP_PROXY_UP "SP_PROXY_UP"
#define SP_PROXY_START_STATE "SP_PROXY_START_STATE"
#define SP_PROXIES_STARTED "SP_PROXIES_STARTED"
#define SP_ALL_PROXY_UP "SP_ALL_PROXY_UP"
#define SP_PROCESS_UP "SP_PROCESS_UP"
#define SP_SINGLE_NODE_PROCESSES_UP "SP_SINGLE_NODE_PROCESSES_UP"
#define SP_IMSVC_READY "SP_IMSVC_READY"
#define SP_IMSVC_PROXY_READY "SP_IMSVC_PROXY_READY"
#define PROCESSLIST "PROCESSLIST"
#define PROCESSINFO "PROCESSINFO"
#define PROCESSPARAMS "PROCESSPARAMS"
#define STARTUPSTRING "STARTUPSTRING"
#define CONNECTIONINFO "CONNECTIONINFO"
#define BROKERARGS "BROKERARGS"
#define UNCARGS "UNCARGS"
#define PMARGS "PMARGS"
#define LCSHARGS "LCSHARGS"
#define SNMPARGS "SNMPARGS"
#define TPARGS "TPARGS"
#define UNAARGS "UNAARGS"
#define PTPAARGS "PTPAARGS"
#define PROCESSBACKUPLIST "PROCESSBACKUPLIST"

// SP_IM_STATE is the name of the registry variable that controls how far IM
// comes up. It is used to quickly kill off the proxy and prevents it from
// starting up again. SP_IM_STATE_UNC is used to control the UNC brokers.
//
// See the comments in sp_proxy.cpp, method spProxy::reg_change_im_state, for
// full details.
//
#define SP_IM_STATE "SP_IM_STATE"
#define SP_IM_STATE_OFF "OFF"      // leave off
#define SP_IM_STATE_ON  "ON"       // turn on
#define SP_IM_STATE_UNC "SP_IM_STATE_UNC"

#define WRAPPER_EXE "sp_wrapper"
#define ROUTING_SCRIPT "spqpid_p"
#define SP_MAX_ROUTING_LENGTH 256
#define SP_PORT_IP_MAX 64
#define CLUSTER_GROUP "CLUSTER"
#define SP_MAX_ERROR_TEXT 1024
#define SP_INTERVAL_MAX 64
#define SP_MAX_PROG_ARGS 64
#define SP_PROXY_ARGS 3

#define SP_EVENT "event"
#define SP_PERF_STAT "performance_stat"
#define SP_HEALTH_STATE "health_state"
#define SP_SECURITY "security"
#define SP_NONE "none"
#define ALL_ROUTE "*"
#define SP_NODE_BROKER_INDEX 0
#define SP_SUCCESS 0

#define _SP_IP_ "_SP_IP_"
#define _SP_IP_NODE_ "_SP_IP_NODE_"
#define _SP_IP_SRC_ "_SP_IP_SRC_"

enum {
        PROXY_STATE_DOWN,            // initial and final state
        PROXY_STATE_STARTING,        // starting brokers
        PROXY_STATE_UP,              // running
        PROXY_STATE_STOPPING         // shutting down
};

enum {
        SP_MIN_PROC_TYPE,               // Must be first to get lowest value
        SP_BROKER       = SP_MIN_PROC_TYPE, // Must ref SP_MIN_PROC_TYPE
        SP_UNC,
        SP_PA,
        SP_AAA,
        SP_METRICS,
        SP_TPA,
        SP_TPA_SCRIPTS,
        SP_NSA,
        SP_PM,
        SP_UAA,
        SP_TP,
        SP_PTPA,
        SP_SNMP,
        SP_HARNESS,
        SP_LCSH,
        SP_UNA,
        SP_SMAD,
        SP_EBCM,
        SP_GENERIC_EXE,
        SP_GENERIC_SCRIPT,
        SP_MAX_PROC_TYPE=SP_GENERIC_SCRIPT+1 // Must reference last
};


enum {SP_START_UP = 1, SP_START_DOWN = 2};
enum {SP_YES = 1, SP_NO = 2};

#define SP_NUM_EFB_KEYS 18
const char* const efb_keys[SP_NUM_EFB_KEYS] = {
            "event.common.instance.public.gpb.text_event",
            "health_state.accesslayer.instance.public.gpb.level_1_check",
            "health_state.databaselayer.instance.public.gpb.level_1_check",
            "health_state.foundationlayer.instance.public.gpb.level_1_check",
            "health_state.oslayer.instance.public.gpb.level_1_check",
            "health_state.problem_management.instance.public.gpb.problem_open",
            "health_state.problem_management.instance.public.gpb.problem_close",
            "health_state.se.se_check.instance.public.se_check",
            "health_state.serverlayer.instance.public.gpb.level_1_check",
            "health_state.storagelayer.instance.public.gpb.level_1_check",
            "performance_stat.dtm.instance.public.gpb.perf_stats",
            "performance_stat.linuxcounters.instance.public.gpb.core_metrics_shortbusy_assembled",
            "performance_stat.linuxcounters.instance.public.gpb.disk_metrics_shortbusy_assembled",
            "performance_stat.linuxcounters.instance.public.gpb.filesystem_metrics_shortbusy_assembled",
            "performance_stat.linuxcounters.instance.public.gpb.loadavg_metrics_shortbusy_assembled",
            "performance_stat.linuxcounters.instance.public.gpb.memory_metrics_shortbusy_assembled",
            "performance_stat.linuxcounters.instance.public.gpb.network_metrics_shortbusy_assembled",
            "performance_stat.linuxcounters.instance.public.gpb.virtualmem_metrics_shortbusy_assembled"
          };

class QpidMethodInvoker
{
  public:
	QpidMethodInvoker(Session& session);

    void createLink(const std::string& host, uint32_t port, bool durable=false,
                    const std::string& mechanism=std::string(),
                    const std::string& username=std::string(),
                    const std::string& password=std::string(),
                    const std::string& transport=std::string());

    void createBridge(const std::string& linkName,
                      const std::string& source, const std::string& destination,
                      const std::string& key=std::string(), bool durable=false);

    void methodRequest(const std::string& method,
						const Variant::Map& inParams,
						const std::string& objectName="amqp-broker",
						const std::string& objectType="broker",
						Variant::Map* outParams = 0);
  private:
    Address replyTo;
    Sender sender;
    Receiver receiver;
};


int  sp_find_key (std::string &param, const char *pp_key, std::string &value_out, std::string &delim);

bool sp_find_and_set_key_int (std::string &pr_param, const char *pp_key,
                              std::string &pr_delim, int &pr_to_set);

bool sp_find_and_set_key_str (std::string &pr_param, const char *pp_key,
                              std::string &pr_delim, std::string &pr_to_set);

int  sp_get_process_name(std::string &param, std::string &value_out, std::string &delim);

int  sp_route_broker (std::string& pp_src_ip, int pv_src_port, std::string &pr_src_subtype, std::string& pp_dest_ip,
                int pv_dest_port, std::string &pr_subtype);

bool sp_addrs(char *pp_host, char *pa_buf);

int  sp_get_ip_from_registry( int pv_node, std::string &ip_out);

bool sp_find_exec(const char *pp_exec, char *full_exe);

void sp_all_processes_up();

int sp_init_all_threads();

void sp_stop_all_threads();

void * sp_non_efb_route(void *arg);

void * sp_event_efb_route(void *arg);

void * sp_performance_efb_route(void *arg);

void * sp_health_efb_route(void *arg);

void * sp_do_route(void *arg);

extern const char *ms_getenv_int(const char *pp_key, int *pp_val);
extern int gv_sp_trace_level;  // 0 : no tracing, 1 : errors, 2 : information and errors

#define SPTrace(level, a)                               \
        do {                                            \
                if (gv_sp_trace_level >= level)         \
                        trace_printf a;                 \
        } while (0)


// LOG_AND_TRACE takes the same arguments as SPTrace so the two can be interchanged without any additional
// code modifications.
//
// LOG_AND_TRACE timestamps the output to stdout and also calls SPTrace to do its usual thing so the same
// output can be correlated with other traced output.
//
// 1 is added to the month field because it is zero-based.
//
extern pid_t gv_mypid;

#define LOG_AND_TRACE(x_, y_)                           \
        do {                                            \
                struct timeval tv;                      \
                struct tm      tm_val;                  \
                                                        \
                if (gettimeofday(&tv, NULL) < 0)        \
                        tv.tv_sec = tv.tv_usec = 0;     \
                                                        \
                if (!gv_mypid)                          \
                        gv_mypid = getpid();            \
                                                        \
                SPTrace(x_, y_);                        \
                                                        \
                if (localtime_r(&tv.tv_sec, &tm_val))   \
                        printf("%d-%.2d-%.2d %.2d:%.2d:%.2d.%.6d(%d) ",         \
                                        tm_val.tm_year + 1900,                  \
                                        tm_val.tm_mon  + 1,                     \
                                        tm_val.tm_mday,                         \
                                        tm_val.tm_hour,                         \
                                        tm_val.tm_min,                          \
                                        tm_val.tm_sec,                          \
                                        (int)tv.tv_usec,                        \
                                        gv_mypid);      \
                printf y_ ;                             \
        } while (0)

#endif
