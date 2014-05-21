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

#include <assert.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <queue>
#include <string>

#include "sp_common.h"
#include "common/sp_defines.h"
#include "sp_registry.h"
#include "sp_proxy.h"

using std::queue;
using std::vector;

int gv_sp_trace_level = 0;
pid_t gv_mypid = 0;

namespace {
    pthread_t gv_sp_non_efb_tid;
    pthread_mutex_t gv_sp_non_efb_route_lock;
    pthread_cond_t gv_sp_non_efb_route_ready;
    queue<std::string> gv_sp_non_efb_route_queue;
    int gv_sp_non_efb_route_concurrent_factor = 5;

    pthread_t gv_sp_event_efb_tid;
    pthread_mutex_t gv_sp_event_efb_route_lock;
    pthread_cond_t gv_sp_event_efb_route_ready;
    queue<std::string> gv_sp_event_efb_route_queue;
    int gv_sp_event_efb_route_concurrent_factor = 2;

    pthread_t gv_sp_perf_efb_tid;
    pthread_mutex_t gv_sp_perf_efb_route_lock;
    pthread_cond_t gv_sp_perf_efb_route_ready;
    queue<std::string> gv_sp_perf_efb_route_queue;
    int gv_sp_perf_efb_route_concurrent_factor = 8;

    pthread_t gv_sp_health_efb_tid;
    pthread_mutex_t gv_sp_health_efb_route_lock;
    pthread_cond_t gv_sp_health_efb_route_ready;
    queue<std::string> gv_sp_health_efb_route_queue;
    int gv_sp_health_efb_route_concurrent_factor = 9;

    pthread_mutex_t gv_sp_all_processes_up_perf_lock;
    pthread_cond_t gv_sp_all_processes_up_perf_cond;
    pthread_mutex_t gv_sp_all_processes_up_health_lock;
    pthread_cond_t gv_sp_all_processes_up_health_cond;
    bool gv_sp_all_processes_up_perf = false;
    bool gv_sp_all_processes_up_health = false;

    int gv_sp_route_error = SP_SUCCESS;
}

QpidMethodInvoker::QpidMethodInvoker(Session& session) : 
		replyTo("#; {create:always, node:{x-declare:{auto-delete:true}}}"),
		sender(session.createSender("qmf.default.direct/broker")),
		receiver(session.createReceiver(replyTo)) 
{
}
									  
void QpidMethodInvoker::createLink(const std::string& host, uint32_t port, bool durable,
				const std::string& mechanism,
				const std::string& username,
				const std::string& password,
				const std::string& transport)
{
	Variant::Map params;
	//params["name"]=name;
	params["host"]=host;
	params["port"]=port;
	params["durable"]=durable;
	if (mechanism.size()) params["authMechanism"]=mechanism;
	if (username.size()) params["username"]=username;
	if (password.size()) params["password"]=password;
	if (transport.size()) params["transport"]=transport;
	methodRequest("connect", params);
}

void QpidMethodInvoker::createBridge(const std::string& linkName,
				  const std::string& source, const std::string& destination,
				  const std::string& key, bool durable)
{
	Variant::Map params;
	params["durable"]=durable;
	params["src"]=source;
	params["dest"]=destination;
	if (key.size()) params["key"]=key;
	params["dynamic"]=false;
	params["srcIsLocal"]=false;
	params["srcIsQueue"]=false;
	methodRequest("bridge", params, linkName, "link");
}

void QpidMethodInvoker::methodRequest(const std::string& method, 
					const Variant::Map& inParams, 
					const std::string& objectName, 
					const std::string& objectType, 
					Variant::Map* outParams)
{
	Variant::Map content;
	Variant::Map objectId;
	std::stringstream name;
	name << "org.apache.qpid.broker:" << objectType << ":" << objectName;
	objectId["_object_name"] = name.str();
	content["_object_id"] = objectId;
	content["_method_name"] = method;
	content["_arguments"] = inParams;

	Message request;
	request.setReplyTo(replyTo);
	request.getProperties()["x-amqp-0-10.app-id"] = "qmf2";
	request.getProperties()["qmf.opcode"] = "_method_request";
	encode(content, request);

	sender.send(request);

	Message response;
	if (receiver.fetch(response, Duration::SECOND*5)) {
		if (response.getProperties()["x-amqp-0-10.app-id"] == "qmf2") {
			std::string opcode = response.getProperties()["qmf.opcode"];
			if (opcode == "_method_response") {
				if (outParams) {
					Variant::Map m;
					decode(response, m);
					*outParams = m["_arguments"].asMap();
				}
			} else if (opcode == "_exception") {
				Variant::Map m;
				decode(response, m);
				throw Exception(QPID_MSG("Error: " << m["_values"]));
			} else {
				throw Exception(QPID_MSG("Invalid response received, unexpected opcode: " << opcode));
			}
		} else {
			throw Exception(QPID_MSG("Invalid response received, not a qmfv2 message: app-id="
									 << response.getProperties()["x-amqp-0-10.app-id"]));
		}
	} else {
		throw Exception(QPID_MSG("No response received"));
	}
}

// -----------------------------------------------------
//
// sp_thread_error
// Purpose - Report thread error
//
// -----------------------------------------------------
void sp_thread_error(int pv_code)
{
    if (pv_code != 0)
    {
        LOG_AND_TRACE(1, ("sp_thread_error: abort with error %d\n", pv_code));
        abort();
    }
}

bool sp_find_exec(const char *pp_exec, char *full_exe) {
    char         la_fname[PATH_MAX];
    char         la_path[PATH_MAX+1];
    int          lv_err;
    bool         lv_ret;
    struct stat  lv_statbuf;
    char        *lp_beg;
    char        *lp_end;
    char        *lp_env;

    // find absoute path to exec from path
    lp_env = getenv("PATH");

    strcpy(la_path, lp_env);
    lp_beg = la_path;
    lv_ret = false;
    do
    {
        lp_end = strchr(lp_beg, ':');
        if (lp_end != NULL)
            *lp_end = '\0';
        if (*lp_beg == '/')
            sprintf(la_fname, "%s/%s", lp_beg, pp_exec);
        else // transform relative-to-absolute
            sprintf(la_fname, "%s/%s/%s", getenv("PWD"), lp_beg, pp_exec);
        lv_err = lstat(la_fname, &lv_statbuf);
        if (lv_err == 0) {
            strcpy (full_exe, la_fname);
            lv_ret = true;
            break;
        }
        if (lp_end == NULL)
            break;
        lp_beg = &lp_end[1];
    } while (lp_end != NULL);
    return lv_ret;
}

// -----------------------------------------------------
//
// sp_find_key
// Purpose - Find a key/value pair
//
// -----------------------------------------------------
int sp_find_key (std::string &param, const char *pp_key, std::string &value_out, std::string &delim)
{
   size_t pos;

   if (pp_key == NULL)
       return SP_NOT_FOUND;

   std::string temp = param;
   std::string lp_key_str = pp_key;

   pos = temp.find (lp_key_str);

   if (pos != std::string::npos)
   {
       // +1 for the : deliminter after the key
       temp.erase (0, pos+lp_key_str.size()+1);
       pos = temp.find(delim, 0);

       if (pos == std::string::npos)
           value_out = temp;
       else
           value_out = temp.substr(0,pos);

       if (value_out.length() > 0)
           return SP_SUCCESS;
   }

   SPTrace (2, ("sp_find_key : key %s not found in %s\n",pp_key, param.c_str()));

   return SP_NOT_FOUND;
}

// -----------------------------------------------------
//
// sp_find_and_set_key_str
// Purpose - get a key/value pair
//
// -----------------------------------------------------
bool sp_find_and_set_key_str (std::string &pr_param, const char *pp_key,
                              std::string &pr_delim, std::string &pr_to_set)
{
    std::string lp_value;
    int lv_error = sp_find_key (pr_param, pp_key, lp_value, pr_delim);
    if (lv_error != SP_SUCCESS)
        return false;
    pr_to_set = lp_value;
    return true;
}

// -----------------------------------------------------
//
// sp_find_and_set_key_int
// Purpose - get a key/value pair
//
// -----------------------------------------------------
bool sp_find_and_set_key_int (std::string &pr_param,  const char *pp_key,
                              std::string &pr_delim,  int &pr_to_set)
{
    std::string lp_value;
    int lv_error = sp_find_key (pr_param, pp_key, lp_value, pr_delim);
    if (lv_error != SP_SUCCESS)
        return false;
    pr_to_set = atoi(lp_value.c_str());
    return true;
}

// -----------------------------------------------------
//
// sp_get_process_name
// Purpose - get a process name from a deliniated string
//
// -----------------------------------------------------
int sp_get_process_name(std::string &param, std::string &value_out, std::string &delim)
{
   size_t pos, pos2;
   std::string first_char = "$";

   pos = param.find (first_char);

   if (pos != std::string::npos)
   {
       if (pos > 0)
          param.erase(0, pos);

       pos2 = param.find(delim, 0);

       if (pos == std::string::npos)
           value_out = param.substr(pos, std::string::npos);
       else
       {
           value_out = param.substr(0,pos2);
           param.erase(0, pos2+1/*delimiter*/);
       }
       if (value_out.length() > 0)
           return SP_SUCCESS;
   }
   return SP_NOT_FOUND;
}

// -----------------------------------------------------
//
//  addrs
// Purpose - get an ip address of this physical node
//
// -----------------------------------------------------
bool sp_addrs(char *pp_host, char *pa_buf)
{
    long               *lp_addr;
    unsigned char      *lp_addrp;
    struct hostent     *lp_hostent = NULL;
    struct sockaddr_in  lv_addr;
    int                 lv_inx;
    int                 lv_retries = 0;

    while ((lp_hostent == NULL) && (lv_retries < 3))
    {
        lp_hostent = gethostbyname(pp_host);
        lv_retries++;
    }

    if (lp_hostent == NULL)
       return false;

    for (lv_inx = 0;; lv_inx++)
    {
        lp_addr = reinterpret_cast<long *>(lp_hostent->h_addr_list[lv_inx]);
        if (lp_addr == NULL)
           return false;
        if (*lp_addr == 0)
           return false;

        lv_addr.sin_addr.s_addr = static_cast<int>(*lp_addr);
        lp_addrp = reinterpret_cast<unsigned char *>(lp_addr);
        sprintf(pa_buf, "%d.%d.%d.%d",lp_addrp[0], lp_addrp[1],
                                      lp_addrp[2],lp_addrp[3]);
        return true;
    }
}
// -------------------------------------------------------------
//
// get_ip_from_registry
// Purpose : given a proxy name, get its IP address
//
// -------------------------------------------------------------
int sp_get_ip_from_registry( int pv_nid, std::string &ip_out)
{
    char la_process_name[MS_MON_MAX_PROCESS_NAME];
    sprintf (la_process_name, "$XDN%d", pv_nid);
    int lv_error = sp_reg_get(MS_Mon_ConfigType_Process, la_process_name,
                              (char *) SP_IP_ADDRESS, ip_out);

    return lv_error;
}

// -----------------------------------------------------------
//
// route_broker
// Purpose : Distribute routes jobs to related threads and send notification to start.
//
// -----------------------------------------------------------
int sp_route_broker (std::string& pr_src_ip, int pv_src_port, std::string &pp_src_subtype, std::string& pr_dest_ip,
                    int pv_dest_port, std::string &pp_subtype)
{
    char la_buffer[SP_MAX_ROUTING_LENGTH];
    int  lv_error = SP_SUCCESS;
    int  lv_numRoutingKeys = 0;
    bool lb_doRouting = true;
    char la_route[256];
    char la_srcBrk_pubType[256];

    SPTrace (2, ("sp_route_broker : ENTRY\n"));
    LOG_AND_TRACE (3, ("sp_route_broker : src_ip (%s), src_port (%d), src_type (%s), dest_ip (%s), dest_port (%d), type (%s)\n",
                 pr_src_ip.c_str(), pv_src_port, pp_src_subtype.c_str(), pr_dest_ip.c_str(), pv_dest_port, pp_subtype.c_str()));

   // Determine the potential number of routing keys/publications
   if (pp_subtype == "EXTERN")
   {
       lv_numRoutingKeys = SP_NUM_EFB_KEYS;

       // Determine publications type (which matches the routing key/publication prefix) for this source broker
       if ((pp_src_subtype == "CAT-PERF") || (pp_src_subtype == "CONS-PERF"))
           sprintf(la_srcBrk_pubType, "%s", SP_PERF_STAT);
       else if ((pp_src_subtype == "CAT-HEALTH") || (pp_src_subtype == "CONS-HEALTH"))
          sprintf(la_srcBrk_pubType, "%s", SP_HEALTH_STATE);
       else if ((pp_src_subtype == "CAT-EVENT") || (pp_src_subtype == "CONS-EVENT"))
          sprintf(la_srcBrk_pubType, "%s", SP_EVENT);
       else if ((pp_src_subtype == "CAT-SECURITY"))
          sprintf(la_srcBrk_pubType, "%s", SP_SECURITY);
       else if ((pp_src_subtype == "NODE"))
          sprintf(la_srcBrk_pubType, "%s", ALL_ROUTE); // For completeness, should not be used
       else
       {
            // EXTERN or Unknown source broker subtype: trace and exit
          char la_buf[SP_MAX_ERROR_TEXT];
          sprintf(la_buf, "\nProxy Error : Unknown source broker subtype: %s. Nothing will get routed.\n", pp_src_subtype.c_str());
          LOG_AND_TRACE (2, (la_buf));
          lv_error = SP_ROUTE_FAILED;
          SPTrace (2, ("sp_route_broker : EXIT with error %d\n", lv_error));
          return lv_error;
       }  // unknown source broker subtype

    }  // if EXTERN
    else
    {
         // All other brokers need a single route (for now)
       lv_numRoutingKeys = 1;
       sprintf(la_srcBrk_pubType, "%s", SP_NONE);  // For completeness, should not be used
    }

   bool lv_newPerfEfbRoute = false;
   bool lv_newHealthEfbRoute = false;
   // Loop through routing keys to set them up one by one. Depending on broker type,
   // there will be one or more routing key(s) to setup
   for (int i=0;i<(lv_numRoutingKeys);i++)
   {
     // By default we establish a route for this routing key/publication
     lb_doRouting = true;

     if (pp_subtype == "EXTERN")
        {
        sprintf(la_route, "%s", efb_keys[i]);
                // External Facing Brokers need routes for a specific list of publications,
                // coming from various brokers. Establish routes that are likely coming from
                // the current source broker by matching the routing key/publication prefix with
                // the type of publications for this source broker (established above)
                if (strncasecmp(la_route, la_srcBrk_pubType, strlen(la_srcBrk_pubType) ) != 0)
                // No match: this routing key/publication will come from a different source broker;
                // we could establish a route but it would likely never be used, so skip routing
                lb_doRouting = false;
      }
    else if ((pp_subtype == "CAT-PERF") || (pp_subtype == "CONS-PERF"))
       sprintf(la_route, "%s", SP_PERF_STAT);
    else if ((pp_subtype == "CAT-HEALTH") || (pp_subtype == "CONS-HEALTH"))
        sprintf(la_route, "%s",SP_HEALTH_STATE);
    else if ((pp_subtype == "CAT-EVENT") || (pp_subtype == "CONS-EVENT"))
         sprintf(la_route, "%s", SP_EVENT);
    else if ((pp_subtype == "CAT-SECURITY"))
         sprintf(la_route, "%s", SP_SECURITY);
    else
        {
          // Unknown dest broker subtype: trace and exit
        char la_buf[SP_MAX_ERROR_TEXT];
        sprintf(la_buf, "\nProxy Error : Unknown dest broker subtype: %s. No publications will go through.\n", pp_subtype.c_str());
        LOG_AND_TRACE (2, (la_buf));
        lv_error = SP_ROUTE_FAILED;
        SPTrace (2, ("sp_route_broker : EXIT with error %d\n", lv_error));
        return lv_error;
        }


    if (lb_doRouting)
    {
        sprintf(la_buffer, "%s %s %d %s %d %s.#", ROUTING_SCRIPT, pr_src_ip.c_str(), 
                                                  pv_src_port, pr_dest_ip.c_str(),
                                                  pv_dest_port, la_route);
        SPTrace (2, ("sp_route_broker: About to perform routing using:\n     %s\n", la_buffer));
        std::string str_buffer = la_buffer;
        if (pp_subtype != "EXTERN")
        {
            sp_thread_error(pthread_mutex_lock(&gv_sp_non_efb_route_lock));
            gv_sp_non_efb_route_queue.push(str_buffer);
            sp_thread_error(pthread_mutex_unlock(&gv_sp_non_efb_route_lock));
            pthread_cond_signal(&gv_sp_non_efb_route_ready);
        }
        else
        {
            if ((pp_src_subtype == "CAT-EVENT") || (pp_src_subtype == "CONS-EVENT"))
            {
                sp_thread_error(pthread_mutex_lock(&gv_sp_event_efb_route_lock));
                gv_sp_event_efb_route_queue.push(str_buffer);
                sp_thread_error(pthread_mutex_unlock(&gv_sp_event_efb_route_lock));
                pthread_cond_signal(&gv_sp_event_efb_route_ready);
            }
            else if ((pp_src_subtype == "CAT-PERF") || (pp_src_subtype == "CONS-PERF"))
            {
                sp_thread_error(pthread_mutex_lock(&gv_sp_perf_efb_route_lock));
                gv_sp_perf_efb_route_queue.push(str_buffer);
                sp_thread_error(pthread_mutex_unlock(&gv_sp_perf_efb_route_lock));
                lv_newPerfEfbRoute = true;
            }
            else if ((pp_src_subtype == "CAT-HEALTH") || (pp_src_subtype == "CONS-HEALTH"))
            {
                sp_thread_error(pthread_mutex_lock(&gv_sp_health_efb_route_lock));
                gv_sp_health_efb_route_queue.push(str_buffer);
                sp_thread_error(pthread_mutex_unlock(&gv_sp_health_efb_route_lock));
                lv_newHealthEfbRoute = true;
            }
        }
    }  // do routing
    else
    // Skip routing
    {
      SPTrace (2, ("sp_route_broker : skipping routing for %s\n", la_route));
    }

   } // for

   if( lv_newPerfEfbRoute)
       pthread_cond_signal(&gv_sp_perf_efb_route_ready);
   if( lv_newHealthEfbRoute)
       pthread_cond_signal(&gv_sp_health_efb_route_ready);

   if( gv_sp_route_error == SP_ROUTE_FAILED)
       lv_error = SP_ROUTE_FAILED;

   SPTrace (2, ("sp_route_broker : EXIT with error %d\n", lv_error));
   return lv_error;
}

// -----------------------------------------------------------
//
// all_processes_up
// Purpose : invoke to notify performance and health route thread to start to add new routes.
//
// -----------------------------------------------------------
void sp_all_processes_up()
{
    SPTrace (2, ("sp_all_processes_up : ENTRY\n"));
    sp_thread_error(pthread_mutex_lock(&gv_sp_all_processes_up_perf_lock));
    gv_sp_all_processes_up_perf = true;
    sp_thread_error(pthread_mutex_unlock(&gv_sp_all_processes_up_perf_lock));
    pthread_cond_signal(&gv_sp_all_processes_up_perf_cond);

    sp_thread_error(pthread_mutex_lock(&gv_sp_all_processes_up_health_lock));
    gv_sp_all_processes_up_health = true;
    sp_thread_error(pthread_mutex_unlock(&gv_sp_all_processes_up_health_lock));
    pthread_cond_signal(&gv_sp_all_processes_up_health_cond);
    SPTrace (2, ("sp_all_processes_up : EXIT\n"));
}

// -----------------------------------------------------------
//
// init_all_threads
// Purpose : Init all threads related resources.
//
// -----------------------------------------------------------
int sp_init_all_threads()
{
    pthread_mutex_init(&gv_sp_non_efb_route_lock, NULL);
    pthread_cond_init(&gv_sp_non_efb_route_ready, NULL);
    pthread_mutex_init(&gv_sp_event_efb_route_lock, NULL);
    pthread_cond_init(&gv_sp_event_efb_route_ready, NULL);
    pthread_mutex_init(&gv_sp_perf_efb_route_lock, NULL);
    pthread_cond_init(&gv_sp_perf_efb_route_ready, NULL);
    pthread_mutex_init(&gv_sp_health_efb_route_lock, NULL);
    pthread_cond_init(&gv_sp_health_efb_route_ready, NULL);
    pthread_mutex_init(&gv_sp_all_processes_up_perf_lock, NULL);
    pthread_cond_init(&gv_sp_all_processes_up_perf_cond, NULL);
    pthread_mutex_init(&gv_sp_all_processes_up_health_lock, NULL);
    pthread_cond_init(&gv_sp_all_processes_up_health_cond, NULL);

    SPTrace(3, ("sp_route_broker : create non_efb thread in main thread.\n"));
    int lv_error = SP_SUCCESS;
    lv_error = pthread_create(&gv_sp_non_efb_tid, NULL, sp_non_efb_route, NULL);
    if (lv_error != 0)
    {
        char la_buf[SP_MAX_ERROR_TEXT];
        sprintf(la_buf, "sp_route_broker : EXIT - Can't create non_efb thread. \n");
        LOG_AND_TRACE (1, (la_buf));
        lv_error = SP_ROUTE_FAILED;
        return lv_error;
    }

    SPTrace(3, ("sp_route_broker : create event_efb thread in main thread.\n"));
    lv_error = pthread_create(&gv_sp_event_efb_tid, NULL, sp_event_efb_route, NULL);
    if (lv_error != 0)
    {
        char la_buf[SP_MAX_ERROR_TEXT];
        sprintf(la_buf, "sp_route_broker : EXIT - Can't create event_efb thread. \n");
        LOG_AND_TRACE (1, (la_buf));
        lv_error = SP_ROUTE_FAILED;
        return lv_error;
    }

    SPTrace(3, ("sp_route_broker : create performance_efb thread in main thread.\n"));
    lv_error = pthread_create(&gv_sp_perf_efb_tid, NULL, sp_performance_efb_route, NULL);
    if (lv_error != 0)
    {
        char la_buf[SP_MAX_ERROR_TEXT];
        sprintf(la_buf, "sp_route_broker : EXIT - Can't create performance_efb thread. \n");
        LOG_AND_TRACE (1, (la_buf));
        lv_error = SP_ROUTE_FAILED;
        return lv_error;
    }

    SPTrace(3, ("sp_route_broker -- create health_efb thread in main thread.\n"));
    lv_error = pthread_create(&gv_sp_health_efb_tid, NULL, sp_health_efb_route, NULL);
    if (lv_error != 0)
    {
        char la_buf[SP_MAX_ERROR_TEXT];
        sprintf(la_buf, "sp_route_broker : EXIT - Can't create health_efb thread. \n");
        LOG_AND_TRACE (1, (la_buf));
        lv_error = SP_ROUTE_FAILED;
        return lv_error;
    }
    return lv_error;
}

// -----------------------------------------------------------
//
// stop_all_threads
// Purpose : Stop all running threads.
//
// -----------------------------------------------------------
void sp_stop_all_threads()
{
   SPTrace (2, ("sp_stop_all_threads : ENTRY\n"));
   sp_thread_error(pthread_cancel(gv_sp_non_efb_tid));
   sp_thread_error(pthread_cancel(gv_sp_event_efb_tid));
   sp_thread_error(pthread_cancel(gv_sp_perf_efb_tid));
   sp_thread_error(pthread_cancel(gv_sp_health_efb_tid));

   void * lp_tret = 0;
   pthread_join(gv_sp_non_efb_tid, &lp_tret);
   pthread_join(gv_sp_event_efb_tid, &lp_tret);
   pthread_join(gv_sp_perf_efb_tid, &lp_tret);
   pthread_join(gv_sp_health_efb_tid, &lp_tret);
   SPTrace (2, ("sp_stop_all_threads : EXIT\n"));
}

// -----------------------------------------------------------
//
// non_efb_route
// Purpose : Thread for handling new coming non efb routes.
//
// -----------------------------------------------------------
void * sp_non_efb_route(void *pp_arg)
{
    SPTrace (2, ("sp_non_efb_route : ENTRY\n"));
    while(true)
    {
        sp_thread_error(pthread_mutex_lock(&gv_sp_non_efb_route_lock));
        while(gv_sp_non_efb_route_queue.empty())
            pthread_cond_wait(&gv_sp_non_efb_route_ready, &gv_sp_non_efb_route_lock);

        SPTrace (3, ("sp_non_efb_route : route arrived.\n"));
        vector<std::string> lv_routeVector;
        int lv_startThreadNum = 0;
        for(; !gv_sp_non_efb_route_queue.empty() && (lv_startThreadNum < gv_sp_non_efb_route_concurrent_factor); ++lv_startThreadNum)
        {
            std::string lv_route = gv_sp_non_efb_route_queue.front();
            gv_sp_non_efb_route_queue.pop();
            lv_routeVector.push_back(lv_route);
        }
        sp_thread_error(pthread_mutex_unlock(&gv_sp_non_efb_route_lock));

        //In order to start the route in parallel, create serveral threads to do the job.
        vector<pthread_t> lv_tidVector;
        for(int lv_i = 0; lv_i < lv_startThreadNum; ++lv_i)
        {
            pthread_t lv_tid;
            int lv_err = pthread_create(&lv_tid, NULL, sp_do_route, reinterpret_cast<void*>(&lv_routeVector[lv_i]));
            lv_tidVector.push_back(lv_tid);
            if (lv_err != 0)
            {
                char la_buf[SP_MAX_ERROR_TEXT];
                sprintf(la_buf, "sp_non_efb_route : EXIT - Can't create sp_do_route thread. \n");
                SPTrace (1, (la_buf));
            }
        }

        for(int lv_i = 0; lv_i < lv_startThreadNum; ++lv_i)
        {
            void * lp_tret = 0;
            pthread_join(lv_tidVector[lv_i], &lp_tret);
            if(lp_tret != 0)
            {
                char la_buf[SP_MAX_ERROR_TEXT];
                sprintf(la_buf, "sp_non_efb_route : EXIT - Can't create route. \n");
                SPTrace (1, (la_buf));
            }
        }
    }
    SPTrace (2, ("sp_non_efb_route : EXIT\n"));
    return reinterpret_cast<void *> (0);
}

// -----------------------------------------------------------
//
// event_efb_route
// Purpose : Thread for handling new coming event efb routes.
//
// -----------------------------------------------------------
void * sp_event_efb_route(void *pp_arg)
{

   int          lv_error = SP_SUCCESS;
    static bool lv_signaled_for_start = 0;

    SPTrace (2, ("sp_event_efb_route : ENTRY\n"));
    while(true)
    {
        sp_thread_error(pthread_mutex_lock(&gv_sp_event_efb_route_lock));
        while(gv_sp_event_efb_route_queue.empty())
            pthread_cond_wait(&gv_sp_event_efb_route_ready, &gv_sp_event_efb_route_lock);

        SPTrace (3, ("sp_event_efb_route -- route arrived.\n"));
        vector<std::string> lv_routeVector;
        int lv_startThreadNum = 0;
        for(; !gv_sp_event_efb_route_queue.empty() && (lv_startThreadNum < gv_sp_event_efb_route_concurrent_factor); ++lv_startThreadNum)
        {
            std::string lv_route = gv_sp_event_efb_route_queue.front();
            gv_sp_event_efb_route_queue.pop();
            lv_routeVector.push_back(lv_route);
        }
        sp_thread_error(pthread_mutex_unlock(&gv_sp_event_efb_route_lock));

        //In order to start the route in parallel, create serveral threads to do the job.
        vector<pthread_t> lv_tidVector;
        for(int lv_i = 0; lv_i < lv_startThreadNum; ++lv_i)
        {
            pthread_t lv_tid;
            int lv_err = pthread_create(&lv_tid, NULL, sp_do_route, reinterpret_cast<void*>(&lv_routeVector[lv_i]));
            lv_tidVector.push_back(lv_tid);
            if (lv_err != 0)
            {
                char la_buf[SP_MAX_ERROR_TEXT];
                sprintf(la_buf, "sp_event_efb_route : EXIT - Can't create sp_do_route thread. \n");
                SPTrace (1, (la_buf));
            }
        }

        for(int lv_i = 0; lv_i < lv_startThreadNum; ++lv_i)
        {
            void * lp_tret = 0;
            pthread_join(lv_tidVector[lv_i], &lp_tret);
            if(lp_tret != 0)
            {
                char la_buf[SP_MAX_ERROR_TEXT];
                sprintf(la_buf, "sp_event_efb_route : EXIT - Can't create route. \n");
                SPTrace (1, (la_buf));
            }
        }

        if (!lv_signaled_for_start)
        {
            char la_buf[2];
            lv_signaled_for_start = true;
            SPTrace (3, ("sp_event_efb_route : issuing SP_IMSVC_READY event\n"));
            sprintf(la_buf, "%d", 1);
            lv_error = sp_reg_set(MS_Mon_ConfigType_Cluster, CLUSTER_GROUP,
                             SP_IMSVC_READY, la_buf);
        }


    }
    SPTrace (2, ("sp_event_efb_route : EXIT\n"));
    return reinterpret_cast<void *> (0);
}

// -----------------------------------------------------------
//
// performance_efb_route
// Purpose : Thread for handling new coming performance efb routes.
//
// -----------------------------------------------------------
void * sp_performance_efb_route(void *pp_arg)
{
    SPTrace (2, ("sp_performance_efb_route : ENTRY\n"));
    //Wait until all processes are up.
    sp_thread_error(pthread_mutex_lock(&gv_sp_all_processes_up_perf_lock));
    while(!gv_sp_all_processes_up_perf)
        pthread_cond_wait(&gv_sp_all_processes_up_perf_cond, &gv_sp_all_processes_up_perf_lock);
    sp_thread_error(pthread_mutex_unlock(&gv_sp_all_processes_up_perf_lock));

    SPTrace (3, ("sp_performance_efb_route -- all processes up. Continue to set route.  \n"));
    while(true)
    {
        sp_thread_error(pthread_mutex_lock(&gv_sp_perf_efb_route_lock));
        while(gv_sp_perf_efb_route_queue.empty())
            pthread_cond_wait(&gv_sp_perf_efb_route_ready, &gv_sp_perf_efb_route_lock);

        SPTrace (3, ("sp_performance_efb_route -- route arrived.\n"));
        vector<std::string> lv_routeVector;
        int lv_startThreadNum = 0;
        for(; !gv_sp_perf_efb_route_queue.empty() && (lv_startThreadNum < gv_sp_perf_efb_route_concurrent_factor); ++lv_startThreadNum)
        {
            std::string lv_route = gv_sp_perf_efb_route_queue.front();
            gv_sp_perf_efb_route_queue.pop();
            lv_routeVector.push_back(lv_route);
        }
        sp_thread_error(pthread_mutex_unlock(&gv_sp_perf_efb_route_lock));

        //In order to start the route in parallel, create serveral threads to do the job.
        vector<pthread_t> lv_tidVector;
        for(int lv_i = 0; lv_i < lv_startThreadNum; ++lv_i)
        {
            pthread_t lv_tid;
            int lv_err = pthread_create(&lv_tid, NULL, sp_do_route, reinterpret_cast<void*>(&lv_routeVector[lv_i]));
            lv_tidVector.push_back(lv_tid);
            if (lv_err != 0)
            {
                char la_buf[SP_MAX_ERROR_TEXT];
                sprintf(la_buf, "sp_performance_efb_route : EXIT - Can't create sp_do_route thread. \n");
                SPTrace (1, (la_buf));
            }
        }

        for(int lv_i = 0; lv_i < lv_startThreadNum; ++lv_i)
        {
            void * lp_tret = 0;
            pthread_join(lv_tidVector[lv_i], &lp_tret);
            if(lp_tret != 0)
            {
                char la_buf[SP_MAX_ERROR_TEXT];
                sprintf(la_buf, "sp_performance_efb_route : EXIT - Can't create route. \n");
                SPTrace (1, (la_buf));
            }
        }
    }
    SPTrace (2, ("sp_performance_efb_route : EXIT\n"));
    return reinterpret_cast<void *> (0);
}

// -----------------------------------------------------------
//
// health_efb_route
// Purpose : Thread for handling new coming health efb routes.
//
// -----------------------------------------------------------
void * sp_health_efb_route(void *pp_arg)
{
    SPTrace (2, ("sp_health_efb_route : ENTRY\n"));
    //Wait until all processes are up.
    sp_thread_error(pthread_mutex_lock(&gv_sp_all_processes_up_health_lock));
    while(!gv_sp_all_processes_up_health)
        pthread_cond_wait(&gv_sp_all_processes_up_health_cond, &gv_sp_all_processes_up_health_lock);
    sp_thread_error(pthread_mutex_unlock(&gv_sp_all_processes_up_health_lock));

    SPTrace (3, ("sp_health_efb_route -- all processes up. Continue to set route.  \n"));
    while(true)
    {
        sp_thread_error(pthread_mutex_lock(&gv_sp_health_efb_route_lock));
        while(gv_sp_health_efb_route_queue.empty())
            pthread_cond_wait(&gv_sp_health_efb_route_ready, &gv_sp_health_efb_route_lock);

        SPTrace (3, ("sp_health_efb_route -- route arrived.\n"));
        vector<std::string> lv_routeVector;
        int lv_startThreadNum = 0;
        for(; !gv_sp_health_efb_route_queue.empty() && (lv_startThreadNum < gv_sp_health_efb_route_concurrent_factor); ++lv_startThreadNum)
        {
            std::string lv_route = gv_sp_health_efb_route_queue.front();
            gv_sp_health_efb_route_queue.pop();
            lv_routeVector.push_back(lv_route);
        }
        sp_thread_error(pthread_mutex_unlock(&gv_sp_health_efb_route_lock));

        //In order to start the route in parallel, create serveral threads to do the job.
        vector<pthread_t> lv_tidVector;
        for(int lv_i = 0; lv_i < lv_startThreadNum; ++lv_i)
        {
            pthread_t lv_tid;
            int lv_err = pthread_create(&lv_tid, NULL, sp_do_route, reinterpret_cast<void*>(&lv_routeVector[lv_i]));
            lv_tidVector.push_back(lv_tid);
            if (lv_err != 0)
            {
                char la_buf[SP_MAX_ERROR_TEXT];
                sprintf(la_buf, "sp_health_efb_route : EXIT - Can't create sp_do_route thread. \n");
                SPTrace (1, (la_buf));
            }
        }

        for(int lv_i = 0; lv_i < lv_startThreadNum; ++lv_i)
        {
            void * lp_tret = 0;
            pthread_join(lv_tidVector[lv_i], &lp_tret);
            if(lp_tret != 0)
            {
                char la_buf[SP_MAX_ERROR_TEXT];
                sprintf(la_buf, "sp_health_efb_route : EXIT - Can't create route. \n");
                SPTrace (1, (la_buf));
            }
        }
    }
    SPTrace (2, ("sp_health_efb_route : EXIT\n"));
    return reinterpret_cast<void *> (0);
}

// -----------------------------------------------------------
//
// do_route
// Purpose : invoke routing script from src to dest
//
// -----------------------------------------------------------
void * sp_do_route(void * pp_arg)
{
    SPTrace (2, ("sp_do_route : ENTRY\n"));
    std::string lv_routeBuffer = *(static_cast<std::string *>(pp_arg));
    LOG_AND_TRACE(1, ("sp_do_route: %s\n", lv_routeBuffer.c_str()));

    std::vector<std::string> SplitVec;
    split( SplitVec,lv_routeBuffer, is_any_of(" "), token_compress_on );

    if(SplitVec.size()<6) //script, source_ip, source_port, dest_ip, dest_port, routingkey
    {
    	LOG_AND_TRACE(1, ("sp_do_route: ERROR vec.size()<6..\n"));
		return reinterpret_cast<void *> (1);
    }
    std::string source_ip = SplitVec[1];
    std::string source_port = SplitVec[2];
    std::string dest_ip = SplitVec[3];
    std::string dest_port = SplitVec[4];
    std::string routingkey = SplitVec[5];
    int i_source_port = boost::lexical_cast<int>(source_port);

    int retryTime=3;
	while(retryTime>0)
	{
		try
		{
			Connection conn(dest_ip+":"+dest_port);
			conn.open();
			Session session = conn.createSession();

			QpidMethodInvoker control(session);
			control.createLink(source_ip,i_source_port, true);
			control.createBridge(source_ip+","+ source_port,"amq.topic", "amq.topic",routingkey, true);
			conn.close();

			LOG_AND_TRACE(1, ("sp_do_route: succeeded..\n"));
			break;
		}
		catch(std::exception& e)
		{
			retryTime--;
			LOG_AND_TRACE(1, ("sp_do_route: errror: %s\n", e.what()));
			sleep(5);
		}
	}
	SPTrace (2, ("sp_do_route : EXIT\n"));
	return reinterpret_cast<void *> (0);
}

