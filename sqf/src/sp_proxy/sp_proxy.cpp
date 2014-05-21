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
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// #include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <vector>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <qpid/messaging/Address.h>
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Session.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/exceptions.h>
#include <qpid/types/Variant.h>
#include <qpid/Msg.h>

#include "sp_proxy.h"
#include "sp_registry.h"
#include "sp_common.h"

#include "common/evl_sqlog_eventnum.h"
#include "common/sq_common.h"
#include "seabed/fserr.h"

#include "SCMVersHelp.h"

using namespace boost;
using namespace qpid::messaging;
using namespace qpid::types;

DEFINE_EXTERN_COMP_DOVERS(sp_proxy)
DEFINE_EXTERN_COMP_GETVERS(sp_proxy)

SP_Nodes gv_nodes;

// -----------------------------------------------------------
//
// spProxy::spProxy
// Purpose : Constructor
//
// -----------------------------------------------------------
spProxy::spProxy()
{
    iv_myNid = -1;
    iv_myPid = -1;
    iv_numPartialUpNodes = iv_numFullyUpNodes = iv_failed_start = 0;
    iv_state = PROXY_STATE_DOWN;
    iv_numTotalNonBackupProcesses = 0;
    iv_numStartedProcesses = 0;

    // by default we start all of the processes unless told otherwise
    for (int i=SP_MIN_PROC_TYPE; i<SP_MAX_PROC_TYPE; i++) {
        ia_runState[i] = SP_STATE_ON;
    }
}

// -----------------------------------------------------------
//
// spProxy::~spProxy
// Purpose : Destructor
//
// -----------------------------------------------------------
spProxy::~spProxy()
{
}

// -----------------------------------------------------------
//
// spProxy::sp_proxy_exit
// Purpose : Print message to stdout and exit
//
// -----------------------------------------------------------
void spProxy::sp_proxy_exit(const char *pp_reason)
{
    if (pp_reason)
        LOG_AND_TRACE (1,("%s\n", pp_reason));

    bool lv_success = iv_list.kill_processes(iv_myNid);
    sleep(5); // give the UNCs a little time
    sp_stop_all_threads();

    // if we weren't able to stop, we need to do a hard down so the monitor kills them
    if (lv_success)
    {
        LOG_AND_TRACE (1, ("spProxy::sp_proxy_exit Proxy Stopping cleanly\n"));
        msg_mon_process_shutdown();
    }
    else
        LOG_AND_TRACE (1, ("spProxy::sp_proxy_exit Proxy Stopping hard, error in stopping processes\n"));
    exit(1);
}

// ------------------------------------------------------------------
//
// spProxy::pre_init
// Purpose : Initialize all we can before other proxies get involved
//
// ------------------------------------------------------------------
int spProxy::pre_init()
{
    char                     la_buf[MS_MON_MAX_PROCESS_PATH];
    char                     la_hostname[MS_MON_MAX_PROCESS_PATH];
    int                      lv_error = SP_SUCCESS;
    MS_Mon_Process_Info_Type lv_info_name;
    bool                     lv_success = false;
    std::string              lv_value;

    ms_getenv_int("PROXY_TRACE", &gv_sp_trace_level);

    if (gv_sp_trace_level > 0)
        trace_init((char *) "proxy_trace", true /*unique*/, NULL, false);

    SPTrace (2, ("spProxy::pre_init ENTRY\n"));
    SPTrace (1, ("spProxy::pre_init Proxy Starting\n"));

    // get data about ourselves such as seaquest pid/nid and physical ip
    lv_error = msg_mon_get_process_info_detail (NULL, &lv_info_name);
    if (lv_error)
    {
       LOG_AND_TRACE (1, ("spProxy::pre_init EXIT with error %d from msg_mon_get_process_info_detail\n",
                    lv_error));
       return SP_UNKNOWN;
    }
    iv_myNid = lv_info_name.nid;
    iv_myPid = lv_info_name.pid;
    gv_nodes.set_my_node(iv_myNid);

    sprintf(ia_myProcessName, "%s", lv_info_name.process_name);

    lv_error = gethostname(la_hostname, sizeof(la_hostname));
    if (!lv_error)
       lv_success = sp_addrs (la_hostname, la_buf);

    if ((!lv_success) || (lv_error))
    {
       LOG_AND_TRACE (1, ("spProxy::pre_init EXIT with error %d\n", SP_CANNOT_GET_HOSTNAME));
       return SP_CANNOT_GET_HOSTNAME;
    }

    ip_myIp = la_buf;

    SPTrace (3, ("spProxy::pre_init %s retrieved local IP of %s\n",lv_info_name.process_name,
                 ip_myIp.c_str()));

    // set IP address for this physical node in the registry
    lv_error = sp_reg_set(MS_Mon_ConfigType_Process,
                  ia_myProcessName, SP_IP_ADDRESS, la_buf);

    lv_error = sp_init_all_threads();

    if (lv_error)
    {
       LOG_AND_TRACE (1, ("spProxy::pre_init EXIT with error %d\n", lv_error));
       return lv_error;
    }

    return SP_SUCCESS;
}
// -----------------------------------------------------------
//
// spProxy::init
// Purpose : initialize the configuration
//
// -----------------------------------------------------------
int spProxy::init()
{
    int                         lv_available_spares_count = 0;
    int                         lv_count = 0;
    int                         lv_error = 0;
    MS_Mon_Node_Info_Entry_Type lv_info[MS_MON_MAX_NODE_LIST];
    int                         lv_max = MS_MON_MAX_NODE_LIST;
    int                         lv_node_count = 0;
    int                         lv_pnode_count = 0;
    int                         lv_spares_count = 0;
    std::string                 lv_temp;
    int                         lv_total_count = 0;
    std::string                 lv_value;
    std::string                 lv_value2;

    iv_state = PROXY_STATE_STARTING;

    // we have to call this twice (seabed designe).  The first time to get the
    // total count.  The second time to get the data.
    lv_error = msg_mon_get_node_info2(
                         &lv_total_count,                  //o
                         0,                                //i
                         NULL,                             //o
                         &lv_node_count,                   //o
                         &lv_pnode_count,                  //o
                         &lv_spares_count,                 //o
                         &lv_available_spares_count
   );

    if (!lv_error)
        lv_error = msg_mon_get_node_info2(
                         &lv_total_count,                 //o
                         lv_max,                          //i
                         lv_info,                         //o
                         &lv_node_count,                  //o
                         &lv_pnode_count,                 //o
                         &lv_spares_count,                //o
                         &lv_available_spares_count
    );


    if (lv_error)
    {
        LOG_AND_TRACE (1, ("spProxy::init EXIT (msg_mon_get_node_info2) with error %d\n", lv_error));
        return lv_error;
    }

    // walk through the list of configured nodes, record them, and reduce our actual
    // actual count if a node is not up or if one of them is a spare
    else
    {
        lv_count = lv_total_count;
        iv_numConfigNodes = lv_node_count;
        for (int lv_inx = 0; lv_inx < lv_total_count; lv_inx++)
        {
             // the total number of logical nodes should enver be more
             // than total count.  Logical nodes start with 0 and increase
             // without skipping.
             if (lv_info[lv_inx].nid >= lv_total_count)
             {
                 LOG_AND_TRACE (1, ("spProxy::init logical node numbering error\n"));
                 msg_mon_process_shutdown();
                 exit(0);
             }
             // the nid is the index
             gv_nodes.set_node(lv_info[lv_inx].nid, lv_info[lv_inx].nid);

#ifdef SP_CHECK_NODE_DOWN
             // this is debug code to introduce a node down [without spare] on startup
             int lv_downed_node = -1;
             ms_getenv_int ("SP_NODE_TO_DOWN", &lv_downed_node);
             SPTrace (3, ("spProxy::init SP_NODE_TO_DOWN %d (nid = %d)\n", lv_downed_node,
                           lv_info[lv_inx].nid ));
             if (lv_downed_node == lv_info[lv_inx].nid)
             {
                 lv_info[lv_inx].state =MS_Mon_State_Down;
                 SPTrace (3, ("spProxy::init setting state to down (nid = %d)\n", lv_info[lv_inx].nid));
              }
#endif
             gv_nodes.set_spare(lv_info[lv_inx].nid, lv_info[lv_inx].spare_node);
             gv_nodes.set_state(lv_info[lv_inx].nid,lv_info[lv_inx].state);

             if (lv_info[lv_inx].state != MS_Mon_State_Up)
             {
                 lv_count--;
                 LOG_AND_TRACE (3, ("spProxy::init setting node %d down\n", lv_inx ));
             }
             else if (lv_info[lv_inx].spare_node == true)
             {
                   lv_count--;
                   LOG_AND_TRACE (3, ("spProxy::init spare node on %d\n", lv_inx ));
             }
        }
        SPTrace (3, ("spProxy::init configured nodes %d, actual nodes %d\n",iv_numConfigNodes, lv_count ));
    }


    // get our process list
    lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                  (char *) ia_myProcessName, (char *) "PROCESSFULLLIST", lv_value);

    // read in configuration for each process we are responsible for
    if (!lv_error)
       lv_error = iv_list.populate_process_list (lv_value, ip_myIp, iv_myNid);
    if (!lv_error)
        lv_error = iv_list.populate_processes();

    if (lv_error)
    {
        SPTrace (1, ("spProxy::init EXIT (PROCESSFULLLIST/populate) with error %d\n", lv_error));
        return lv_error;
    }

    // Turn off the state of all brokers and/or the UNC brokers if the
    // value in the repository is OFF. See spProxy::reg_change_im_state()
    // for a full description of SP_IM_STATE and SP_IM_STATE_UNC.
    lv_error = sp_reg_get(MS_Mon_ConfigType_Cluster,
                          (char *)CLUSTER_GROUP,
                          (char *)SP_IM_STATE,
                          lv_value);

    if (!lv_error && lv_value == SP_IM_STATE_OFF)
    {
        ia_runState[SP_BROKER] = SP_STATE_OFF;
        LOG_AND_TRACE(1, ("spProxy::init SP_IM_STATE is OFF. All SeaPilot processes (brokers) are disabled.\n"));
    }

    lv_error = sp_reg_get(MS_Mon_ConfigType_Cluster,
                          (char *)CLUSTER_GROUP,
                          (char *)SP_IM_STATE_UNC,
                          lv_value);

    if (!lv_error && lv_value == SP_IM_STATE_OFF)
    {
       ia_runState[SP_UNC] = SP_STATE_OFF;
       LOG_AND_TRACE(1, ("spProxy::init SP_IM_STATE_UNC is OFF. SeaPilot's UNC brokers are disabled.\n"));
    }

    lv_error = sp_reg_get(MS_Mon_ConfigType_Cluster,
                          (char *) "CLUSTER_GROUP",
                          (char *) "SQ_SEAPILOT_PERF_SUSPENDED",
                          lv_value);

    if (lv_error == 0)
    {
        int lv_sp_disabled = atoi (lv_value.c_str());
        switch (lv_sp_disabled)
        {
            case 0:
                ia_runState[SP_METRICS] = SP_STATE_ON;
                break;

            case 1:
            default:
                ia_runState[SP_METRICS] = SP_STATE_OFF;
                break;
        }
    }
    else
        ia_runState[SP_METRICS] = SP_STATE_OFF;

    iv_numPartialUpNodes = 0;
    iv_numTotalNonBackupProcesses = iv_list.iv_num_processes -
                                    iv_list.iv_num_backup_processes;

    SPTrace (1, ("spProxy:: init numStartProcesses = %d\n",
                 iv_numTotalNonBackupProcesses));
    SPTrace (2, ("spProxy::init EXIT (final) with error %d\n", SP_SUCCESS));
    return SP_SUCCESS;
}

// -----------------------------------------------------------
//
// spProxy::start_processes
// Purpose : called at startup to get all the brokers/UNC wrappers
//           on this node up and going
//
// -----------------------------------------------------------
int spProxy::start_processes ()
{
    char la_buf[2];
    int  lv_error = SP_SUCCESS;
    bool const is_node_restart = get_failed_start();

    SPTrace (2, ("spProxy::start_processes ENTRY\n"));

    // Check to see if we are allowed to start. The user can disable all
    // or part of SeaPilot through the sqshell variable SP_IM_STATE, which
    // is tracked in ia_runState.
    //
    // If the node broker, SP_BROKER, is disabled, then all of SeaPilot
    // is disabled.
    if (!is_okToRun(SP_BROKER))
    {
        LOG_AND_TRACE(2, ("spProxy::start_processes all brokers disabled. "
                    "Early EXIT with %d\n", lv_error));
        return lv_error;
    }  // !okToRun SP_BROKER


    // start node broker. This will always be first, except on virtual nodes
    // [need to account for that]...
    SP_Process * lv_broker(&iv_list.ip_list[SP_NODE_BROKER_INDEX]);

     if ((lv_broker->is_backup() != true) ||
        ((lv_broker->is_backup() == true) &&
          (!gv_nodes.is_up(SP_NODE_BROKER_INDEX))))
     {
            if (lv_broker->is_backup())
                lv_broker->reset_node(false);

            lv_error = lv_broker->start_process(iv_myNid,
                                                lv_broker->connection_info.port,
                                                lv_broker->connection_info.ip);

            if (lv_broker->isStarted())
                ++iv_numStartedProcesses;

            SPTrace(3, ("spProxy::start_processes starting node broker"
                        " returned %d iv_numStartedProcesses = %d\n",
                        lv_broker->isStarted(), iv_numStartedProcesses));
    }

    if (lv_error)
    {
        SPTrace (1, ("spProxy::start_processes failed to start %s,"
                     " EXIT with error %d.\n",
                      lv_broker->get_name(), lv_error));
        return  lv_error;
    }

    // Node broker has started. If we are coming up after a failure, need to
    // establish routes from the node broker to the destination brokers
    // on other nodes.
    if (is_node_restart)
    {
          LOG_AND_TRACE (3, ("spProxy::start_processes node is restarting"
                       " after failure, creating remote routes.\n"));
          for (int lv_inx = 0; lv_inx < SP_NUM_DEST_BROKERS; lv_inx++)
          {
            SP_BROKER_INFO * broker_info;

            broker_info = &lv_broker->process_params.sp_params.destbrokers[lv_inx];
              // reroute remotes if we need to.
             if ((broker_info->name != "") &&
                 (broker_info->node != iv_myNid))
             {
                 SPTrace (3, ("spProxy::start_processes creating remote route"
                              " to node %d port %d.\n",
                              broker_info->node,
                              broker_info->port));

                  // get most up to date ip
                  std::string lv_ip;
                  lv_error = sp_get_ip_from_registry(broker_info->node, lv_ip);
                  if (lv_error)
                     break;

                  lv_error = sp_route_broker(lv_broker->connection_info.ip,
                       lv_broker->connection_info.port,
                       lv_broker->process_params.sp_params.subtype, lv_ip,
                       broker_info->port,
                       broker_info->subtype);
                  if (lv_error)
                     break;
             } // name !="" && node != mine
          }  //endfor
    }  // is_node_restart

    if (lv_error)
    {
        SPTrace (1, ("spProxy::start_processes EXIT with error %d.\n", lv_error));
        return  lv_error;
    }

   // Start the rest of the SeaPilot processes. Begin the loop after
   // SP_NODE_BROKER_INDEX because we did that above. Note this assumes
   // that there are no brokers earlier than SP_NODE_BROKER_INDEX and that
   // the remaining brokers are consecutive.
   for (int lv_inx = SP_NODE_BROKER_INDEX+1;
        lv_inx < iv_list.iv_num_processes;
        lv_inx++)
   {
       lv_error = start_process(lv_inx, is_node_restart);
       if (lv_error)
           break;
   }

   if (lv_error)
   {
       SPTrace (1, ("spProxy::start_processes EXIT (after routing) with error %d. \n", lv_error));
       return lv_error;
   }

    // all brokers on this node are up and routes are all done
    sprintf(la_buf, "%d", 1);
    char la_buf2[1024];
    memset(la_buf2, 0, 1024);
    sprintf(la_buf2, "%s_%d", SP_PROXY_UP, iv_myNid);
    lv_error = sp_reg_set(MS_Mon_ConfigType_Cluster, CLUSTER_GROUP,
                         la_buf2, la_buf);

    SPTrace (2, ("spProxy::start_processes EXIT with error %d \n", lv_error));
    return lv_error;
}

// ----------------------------------------------------------
// start_process
// Purpose - if conditions are met, start a single broker
//                       process and create the routes to it
// -----------------------------------------------------------
int spProxy::start_process(int pv_inx,
                           bool pv_is_node_restart)
{
    int lv_error = SP_SUCCESS;
    char lv_one[] = "1";
    SP_Process * lv_process(&iv_list.ip_list[pv_inx]);

    SPTrace (2, ("spProxy::start_process ENTRY %d\n", pv_inx));

    // Don't start the broker if it is disabled.
    if (!is_okToRun(lv_process->type()))
        return SP_SUCCESS;

    // Don't start on-demand processes at the initial startup. They
    // will be started later, after the node local broker is started.
    if (lv_process->is_on_demand_process() && !pv_is_node_restart)
        return SP_SUCCESS;

    // if this process is a backup and the primary node IS NOT down,
    // don't start the backup
    if ((lv_process->is_backup() == true) &&
        (gv_nodes.is_up(lv_process->connection_info.original_node)))
    {
        LOG_AND_TRACE (3, ("spProxy::start_process skipping backup process %s,"
                     " original node %d is up.\n",
                     lv_process->get_name(),
                     lv_process->connection_info.original_node));
        return SP_SUCCESS;
    }

    // if this process is a backup and its primary node IS down,
    // then start it after updating routing
    if ((lv_process->is_backup() == true) &&
        (!gv_nodes.is_up(lv_process->connection_info.original_node)))
    {
        LOG_AND_TRACE (3, ("spProxy::start_process starting ( %s ) backup process.\n",
                      lv_process->get_name()));
        lv_process->reset_node(false);
    }

    // start tbe broker
    lv_error = lv_process->start_process(iv_myNid,
                                        lv_process->connection_info.port,
                                        lv_process->connection_info.ip);

    if (lv_process->isStarted())
        ++iv_numStartedProcesses;

    SPTrace(3, ("spProxy::start_process returned %d"
                " iv_numStartedProcesses %d"
                " iv_numTotalNonBackupProcesses %d\n",
                lv_process->isStarted(),
                iv_numStartedProcesses,
                iv_numTotalNonBackupProcesses));

    if (iv_numStartedProcesses == iv_numTotalNonBackupProcesses)
    {
        SPTrace (3, ("spProxy::start_process setting SP_SINGLE_NODE_PROCESSES_UP.\n"));
        char la_reg_buf [1024];
        memset(la_reg_buf, 0, 1024);

        sprintf(la_reg_buf, "%s_%d", SP_SINGLE_NODE_PROCESSES_UP, iv_myNid);
        lv_error = sp_reg_set(MS_Mon_ConfigType_Cluster,
                              CLUSTER_GROUP,
                              la_reg_buf, lv_one);
    }

    if (lv_error)
    {
        SPTrace (1, ("spProxy::start_processes failed to start %s, error %d\n",
                      lv_process->get_name(), lv_error));
        return lv_error;
    }

    // Process started. If it's a broker, set up routing
    if (lv_process->type() == SP_BROKER)
    {
       // route to all category brokers all the time, regardless of normal
       // (cold) startup or reintegration (warm) startup. If this is a cold
       // start, then all the routes are new and have to be created for the
       // first time. If this is a warm start, existing routes might need to
       // be changed from a backup to a primary broker.

       if (lv_process->is_category_broker())
       {
         SP_Process * lv_broker(&iv_list.ip_list[SP_NODE_BROKER_INDEX]);

             // Setup routing between this category broker and the node broker
             lv_error = sp_route_broker(ip_myIp,
                                lv_broker->connection_info.port,
                                lv_broker->process_params.sp_params.subtype,
                                ip_myIp, lv_process->connection_info.port,
                                lv_process->process_params.sp_params.subtype );
      }

      // set up all other local and remote routes
      lv_error = lv_process->route_helper(iv_myNid, ip_myIp, pv_is_node_restart);
    }  // endif type() == SP_BROKER

    return lv_error;
}

// ----------------------------------------------------------
// process_registry_change
// Purpose - determine if a DTM key is referenced, and if so,
//               dispatch to the action helper routine to process it
// -----------------------------------------------------------
int spProxy::process_registry_change(MS_Mon_Change_def *pp_change )
{
    int lv_error = SP_SUCCESS;

    SPTrace (2, ("spProxy::process_registry_change ENTER"
                 " with key %s and value %s\n",
                 pp_change->key, pp_change->value));

    LOG_AND_TRACE (1, ("spProxy::reg_change %s::%s=%s\n",
                pp_change->group, pp_change->key, pp_change->value));

    // unrecognized keys are ignored without generating an error

    if (SP_MATCH_KEY(pp_change->key, SP_PROXY_UP))
        lv_error = reg_change_proxy(pp_change);
    else if (SP_MATCH_KEY(pp_change->key, SP_ALL_PROXY_UP))
        lv_error = reg_change_all_proxy(pp_change);
    else if (SP_MATCH_KEY(pp_change->key, SP_SINGLE_NODE_PROCESSES_UP))
        lv_error = reg_change_single_node_processes(pp_change);
    else if (SP_MATCH_KEY(pp_change->key, SP_PROCESS_UP))
        lv_error = reg_change_process(pp_change);
    else if (SP_MATCH_KEY(pp_change->key, SP_IM_STATE))
        lv_error = reg_change_im_state(pp_change);
    else if (SP_MATCH_KEY(pp_change->key, SP_DO_ROUTE))
    {
        if(contains(pp_change->value, "DONE") || contains(pp_change->value, "FAILED"))
		{	
			return SP_SUCCESS;
		}
		lv_error = reg_change_do_route(pp_change);
    }

    SPTrace (2, ("spProxy::process_registry_change EXIT with error %d\n",
                 lv_error));
    return lv_error;
}

// ----------------------------------------------------------
// reg_change_proxy
// Purpose - act on SP_PROXY_UP
// -----------------------------------------------------------

int spProxy::reg_change_proxy(MS_Mon_Change_def *pp_change)
{
        int     lv_error = SP_SUCCESS;
        char    lv_one[] = "1";

        // If we are coming up for the first time this will be true.
        // It gets changed once all proxies are up (see reg_change_all_proxy).
        if (iv_state == PROXY_STATE_STARTING)
        {
                iv_numPartialUpNodes++;
                SPTrace(3, ("spProxy::process_registry_change"
                            " iv_numPartialUpNodes %d iv_numConfigNodes %d\n",
                            iv_numPartialUpNodes, iv_numConfigNodes));

                if (iv_numPartialUpNodes == iv_numConfigNodes)
                {
                        lv_error = msg_mon_event_send(-1, /*nid*/
                                        -1,                       /*pid*/
                                        MS_ProcessType_SPX,       /*process_type*/
                                        PROXY_START_EVENT_ID,     /*event_id*/
                                        0,                        /*event_len*/
                                        NULL                      /*event_data*/);

                        if (!lv_error)
                        {
                                SPTrace (3, ("spProxy::reg_change_proxy :"
                                             " setting SP_ALL_PROXY_UP. \n"));
                                lv_error = sp_reg_set(MS_Mon_ConfigType_Cluster,
                                                      CLUSTER_GROUP,
                                                      SP_ALL_PROXY_UP,
                                                      lv_one);
                        }
                }
        }
        return lv_error;
}

// ----------------------------------------------------------
// reg_change_all_proxy
// Purpose - act on SP_ALL_PROXY_UP
// -----------------------------------------------------------
int spProxy::reg_change_all_proxy(MS_Mon_Change_def *pp_change)
{
        SPTrace (3, ("spProxy::reg_change_all_proxy : setting state to UP. \n"));
        iv_state = PROXY_STATE_UP;
    return SP_SUCCESS;
}

// ----------------------------------------------------------
// reg_change_single_node_processes
// Purpose - act on SP_SINGLE_NODE_PROCESSES_UP
// -----------------------------------------------------------
int spProxy::reg_change_single_node_processes(MS_Mon_Change_def *pp_change)
{
        iv_numFullyUpNodes++;
        SPTrace (3, ("spProxy::reg_change_single_node_processes"
                     " iv_numPartialUpNodes %d iv_numFullyUpNodes %d\n",
                     iv_numPartialUpNodes, iv_numFullyUpNodes));

        // iv_numFullyUpNodes never gets decremented, only incremented.
        // Therefore, the following call will only happen once, during
        // startup, when all nodes are finally up and running.
        if (iv_numFullyUpNodes == iv_numConfigNodes)
        {
                sp_all_processes_up();
        }
        return SP_SUCCESS;
}

// ----------------------------------------------------------
// reg_change_process
// Purpose - act on SP_PROCESS_UP
// -----------------------------------------------------------
int spProxy::reg_change_process(MS_Mon_Change_def *pp_change)
{
    std::string lp_ip;
    std::string lv_copyValue, lv_temp;
    int         lv_error = SP_SUCCESS;
    int         lv_nid;
    size_t      lv_pos;
    char        lv_processName[MS_MON_MAX_PROCESS_NAME];
    char        lv_one[] = "1";

    // copy the value of SP_PROCESS_UP and parse out the process name
    // and the node number. The value is of the form $ZZZZn,n
    // where:
    //      $ZZZZ is the process 4- or 5-character name, e.g. $UNC, $SNMP, etc.
    //      n     is the node number
    //      ,     is syntax sugar
    //
    lv_copyValue = pp_change->value ;
    lv_pos = lv_copyValue.find (",",0);     // comma marks the end of the name

    // silently ignore malformed entries

    if (lv_pos != std::string::npos)   // found a comma
    {
        // copy then erase the process name
        lv_temp = lv_copyValue.substr(0,lv_pos);
        sprintf(lv_processName,"%s", lv_temp.c_str());
        lv_copyValue.erase (0,lv_pos+1);

        // parse the node number
        lv_nid = atoi (lv_copyValue.c_str());

        // Don't reroute if it's our own node. Local-to-local routing
        // will be handled in process_nodeup() and process_nodedown().
        // We only handle remote routing changes.
        if (lv_nid != iv_myNid)
        {
                sleep(1);  // give it time to initialize
                for (int lv_inx = 0;
                     lv_inx < iv_list.iv_num_processes; lv_inx++)
                {
                        SP_Process * lv_process;

                        lv_process = &iv_list.ip_list[lv_inx];

                        // if the process started is a broker and not a backup,
                        // then setup routing
                        if ((lv_process->type() == SP_BROKER) &&
                            (!lv_process->is_backup()))
                        {
                                lv_error = lv_process->reroute_remote_node(lv_temp);
                        }
                        // stop if we can't route
               if (lv_error)
                   break;
                }  // endfor
        }  // endif not my node

        // no errors and not a node restart,
        // or is a node restart and not my node
        if ((!lv_error && !iv_failed_start) ||
            (iv_failed_start && (lv_nid != iv_myNid)))
          {
                for (int lv_inx = 0; lv_inx < iv_list.iv_num_processes; lv_inx++)
                {
                        int lv_proc_type;
                        SP_Process * lv_process;
                        SP_Process * lv_broker(&iv_list.ip_list[SP_NODE_BROKER_INDEX]);;

                        // copy into shorter variables for better readability
                        lv_process = &iv_list.ip_list[lv_inx];
                        lv_proc_type = lv_process->process_params.type;

                        // start and connect the remote UNCs with the current
                        // broker under the following conditions:
                        // 1. the user hasn't disabled it; and
                        // 2. it's not a backup; and
                        // 3. it's one of a few specific process types.
                        if ((is_okToRun(lv_proc_type)) &&
                            (!lv_process->is_backup()) &&
                            (lv_process->is_on_demand_process()))
                        {
                                // final check:
                                // 4. the name of the broker that started
                                // matches the one in our list
                                if (strcmp(lv_process->process_params.sp_params.sourcebroker.name.c_str(),
                                           lv_processName) == 0)
                                {
                                        lv_error = lv_process->start_on_demand_process(iv_myNid,
                                                        lv_broker->connection_info.port,
                                                        lv_broker->connection_info.ip);

                                        if (lv_process->isStarted())
                                        {
                                                ++iv_numStartedProcesses;

                                                SPTrace(3, ("spProxy::process_registry_change iv_numStartedProcesses = %d iv_numTotalNonBackupProcesses %d\n",
                                                                iv_numStartedProcesses,  iv_numTotalNonBackupProcesses));

                                                if (iv_numStartedProcesses == iv_numTotalNonBackupProcesses)
                                                {
                                                        char la_reg_buf [1024];

                                                        SPTrace (3, ("spProxy::process_registry_change  setting SP_SINGLE_NODE_PROCESSES_UP. \n"));
                                                        memset(la_reg_buf, 0, 1024);

                                                        sprintf(la_reg_buf, "%s_%d", SP_SINGLE_NODE_PROCESSES_UP, iv_myNid);
                                                        lv_error = sp_reg_set(MS_Mon_ConfigType_Cluster,
                                                                        CLUSTER_GROUP,
                                                                        la_reg_buf, lv_one);
                                                }  // endif numStarted == totalNonBackup
                                        } // endif isStarted
                                        if (lv_error)
                                                break;
                                }  // endif name matches
                        }  // okToRun && !backup && is_on_demand
                }  // endfor loop over all processes
        }  // endif no errors or restart
    }  // found a comma

    return lv_error;
}

// ----------------------------------------------------------
// reg_change_sp_im_state
// Purpose - act on SP_IM_STATE changes
// -----------------------------------------------------------
// SP_IM_STATE is set by the user when he wants to control the
// IM portion of SeaQuest (i.e., SeaPilot). The states that are
// supported are:
//
// SP_IM_STATE=ON         // run the instance; normal operation, the default
// SP_IM_STATE=OFF        // terminate the proxy and its brokers. On startup,
//                           proxy can run, but it is not allowed to bring
//                           up anything else until the state is back ON.
//
// The end user is also allowed to manipulate the state of the UNC brokers.
// The states are equivalent to those above:
//
// SP_IM_STATE_UNC=ON     // run UNCs; normal, default behavior
// SP_IM_STATE_UNC=OFF    // terminate all UNCs and do not restart.
//
// SP_IM_STATE is like a master switch; it takes precedence over
// the UNC (or any other future) state. This relationship is
// explicitly defined as follows:
//
// When SP_IM_STATE is        the value in SP_IM_STATE_UNC is
//         ON                            honored
//        OFF                            ignored (OFF)
//
// STATE TRANSITIONS
// The following table documents the transitions that are permitted.
//
//  FROM  TO  ALLOWED   DESCRIPTION
//  OFF   ON    YES     turns on SeaPilot. All brokers may run.
//  OFF   OFF    NO     no-op
//  ON    OFF   YES     stops SeaPilot and prevents SeaPilot
//                      from starting
//  ON    ON     NO     no-op
//
//  Note: This is overspecified for a reason: it once described
//  three states, not two. The documentation is left like this
//  to make it easier to add a state.
// -----------------------------------------------------------
int spProxy::reg_change_im_state(MS_Mon_Change_def *pp_change)
{
        // list of all valid source-to-destination combinations
        enum {
                TRANS_OFF_TO_ON,
                TRANS_ON_TO_OFF
        };

        int lv_new_state;  // target (requested) state
        int lv_old_state;  // current state
        int lv_process_type; // which broker state to change
        int lv_error = SP_SUCCESS;
        int lv_transition;

        // The command recognized so far starts with "SP_IM_STATE".
        // Check to see if it's just the prefix for a longer command.
        // For now the only other command it could be is SP_IM_STATE_UNC.
        if (SP_MATCH_KEY(pp_change->key, SP_IM_STATE_UNC))
        {
                lv_process_type = SP_UNC;
        }     // key match:   SP_IM_STATE_UNC
        else
        {     // default key: SP_IM_STATE
                lv_process_type = SP_BROKER;
        }

        // We only permit changes to broker states if the "master switch" is
        // on. Changes to the "master switch" (aka SP_BROKER) are always
        // allowed. Test for the broker being off AND it's not the broker
        // being maniuplated.
        if (!(is_okToRun(SP_BROKER) || (lv_process_type == SP_BROKER)))
                return SP_SUCCESS;  // ignore

        if (SP_MATCH_KEY(pp_change->value, SP_IM_STATE_ON))
                lv_new_state = SP_STATE_ON;
        else if (SP_MATCH_KEY(pp_change->value, SP_IM_STATE_OFF))
                lv_new_state = SP_STATE_OFF;
        else
        {
                SPTrace(3, ("sp_proxy::reg_change_im_state ignoring"
                            " invalid value %s\n", pp_change->value));
                return SP_SUCCESS;
        }

        // Validate and select the transition to be performed
        SPTrace(3, ("sp_proxy::reg_change_im_state old state %d new state %d\n",
                    runState(lv_process_type), lv_new_state));

        switch (runState(lv_process_type))    // old state
        {
        case SP_STATE_OFF:
                if (lv_new_state == SP_STATE_OFF)
                {
                        SPTrace(3, ("sp_proxy::reg_change_im_state ignoring"
                                    " request from OFF to OFF\n"));
                        return SP_SUCCESS;
                }
                else
                        lv_transition = TRANS_OFF_TO_ON;
                break;

        case SP_STATE_ON:
                if (lv_new_state == SP_STATE_ON)
                {
                        SPTrace(3, ("sp_proxy::reg_change_im_state ignoring"
                                    " request from ON to ON\n"));
                        return SP_SUCCESS;
                }
                else
                        lv_transition = TRANS_ON_TO_OFF;
                break;

        default:   // should never be reached
                return SP_UNKNOWN;
        }

        // There are multiple transitions, but the actions are simple:
        // (maybe) change a repository variable and either start or
        // stop brokers.
        switch (lv_transition)
        {
                case TRANS_OFF_TO_ON:
                        SPTrace(3, ("sp_proxy::reg_change_im_state transition"
                                    " from OFF to ON\n"));
                        // No need to change the value in the repository;
                        // it's already ON. Make the change to the internal
                        // state and then start the brokers.
                        ia_runState[lv_process_type] = SP_STATE_ON;

                        // If we turned on the master switch start up
                        // everything, otherwise start the specified broker.
                        if (lv_process_type == SP_BROKER)
                                lv_error = start_processes();
                        else
                            for (int i=1;
                                 ((i<iv_list.iv_num_processes) && !lv_error);
                                  i++)
                            {
                                if (iv_list.ip_list[i].type() == lv_process_type)
                                    lv_error = start_process(i, true);
                            }  //endfor endelse
                        break;

                case TRANS_ON_TO_OFF:
                        SPTrace(3, ("sp_proxy::reg_change_im_state transition"
                                    " from ON to OFF\n"));

                        // Change the repository value to OFF so the broker
                        // will not start. Don't check return value; we're
                        // going away and that's more important.

                        sp_reg_set(MS_Mon_ConfigType_Cluster,
                                   CLUSTER_GROUP,
                                   pp_change->key,
                                   SP_IM_STATE_OFF);

                        // Change the internal state and then stop the brokers.
                        ia_runState[lv_process_type] = SP_STATE_OFF;

                        // If we turned off the master switch, kill ourselves
                        // now. This will take care of all the other brokers.
                        // When proxy restarts, it will find the state set to
                        // OFF.
                        if (lv_process_type == SP_BROKER)
                                sp_proxy_exit("User requested shutdown.");
                        else
                            for (int i=1;
                                 ((i<iv_list.iv_num_processes) && !lv_error);
                                  i++)
                            {
                                if (iv_list.ip_list[i].type() == lv_process_type)
                                {
                                    lv_error = iv_list.ip_list[i].stop_process(iv_myNid);
                                    // it is OK if the process doesn't exist. It might
                                    // have died and we gave up trying to restart it.
                                    lv_error = (lv_error == XZFIL_ERR_NOSUCHDEV) ? SP_SUCCESS : lv_error;
                                } // endif
                            }  //endfor endelse
                        break;

                default:   // should never be reached
                        return SP_UNKNOWN;
        }

        return lv_error;
}

// ------------------------------------------------------------------------
//
// spProxy::reg_change_do_route
// Purpose : Process registry change to add route from the dest node, using 
//           qpid C++ api. When succeeds, the registry value is set "DONE",
//           otherwise it is set "FAILED".
//
// ------------------------------------------------------------------------
int spProxy::reg_change_do_route(MS_Mon_Change_def *pp_change)
{
	int lv_error=SP_SUCCESS;
	int i_source_port;
	int retryTime=3;
	bool doRoute=false;
	
	std::string srcBroker="";
	std::string destBroker="";
	std::string routingkey="";
	std::string lv_value;

	std::string dest_ip;
	std::string dest_port;

	std::string source_ip;
	std::string source_port;

	typedef std::vector<std::string> split_vector_type;
	split_vector_type SplitVec;

	split_vector_type DestSplitVec;
	split_vector_type DestIpInfo;
	split_vector_type DestNodeInfo;
	split_vector_type DestPortInfo;

	split_vector_type SourceSplitVec;
	split_vector_type SourceIpInfo;
	split_vector_type SourceNodeInfo;
	split_vector_type SourcePortInfo;

	std::string regvalue = pp_change->value;
	LOG_AND_TRACE(1, ("reg_change_do_route: change value: %s.\n", regvalue.c_str()));

	//Match broker name: any word followed by any digits.
	boost::regex reg("[a-zA-Z]+[\\d]+");
	bool isMatched ;
	
	//Split input value into 3 parts - source broker, dest broker, and routingkey.
	split(SplitVec, regvalue, is_any_of("_"), token_compress_on);

	//Bad format, go to failure.
	if(SplitVec.size()!=3)
	{
		LOG_AND_TRACE(1, ("reg_change_do_route: too few parameters, exit..\n"));
		goto do_route_failed;
	}

	//Such like $PCB1, $CPCB1, Routingkey
	srcBroker = "$" + SplitVec[0] ;
	destBroker ="$" + SplitVec[1] ;
	routingkey = SplitVec[2];

	isMatched = boost::regex_match(SplitVec[1].c_str(), reg);
	if(!isMatched) 
	{
		LOG_AND_TRACE(1, ("reg_change_do_route: unrecognized broker name: %s, FAILED.\n", SplitVec[1].c_str()));
		goto do_route_failed;
	}
	
	// get our process list
	lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
				  (char *) ia_myProcessName, (char *) "PROCESSLIST", lv_value);
				
	if(contains(lv_value, SplitVec[1]))
	{
		doRoute=true;
		//I'm the dest broker
		LOG_AND_TRACE(1, ("reg_change_do_route: I'm the dest, going to do route..\n"));
		// get dest broker connection information.
		lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
					  (char *) destBroker.c_str(), (char *) "CONNECTIONINFO", lv_value);
		if(lv_error)
		{
			LOG_AND_TRACE(1, ("reg_change_do_route: Cannot get dest broker connection info from monitor, exit..\n"));
			goto do_route_failed;
		}

		split(DestSplitVec, lv_value, is_any_of(" "), token_compress_on);
		if(DestSplitVec.size()!=3)
		{
			LOG_AND_TRACE(1, ("reg_change_do_route: Cannot get dest broker connection info, exit..\n"));
			goto do_route_failed;
		}

		//Get node, dest ip, and dest port.
		split(DestNodeInfo, DestSplitVec[0], is_any_of(":"), token_compress_on);
		split(DestIpInfo, DestSplitVec[1], is_any_of(":"), token_compress_on);
		split(DestPortInfo, DestSplitVec[2], is_any_of(":"), token_compress_on);

		if(DestIpInfo.size()!=2||DestPortInfo.size()!=2 ||DestNodeInfo.size()!=2)
		{
			LOG_AND_TRACE(1, ("reg_change_do_route: Cannot get dest broker Node, IP or Port info, exit..\n"));
			goto do_route_failed;
		}

		// get source broker connection information.
		lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
					  (char *) srcBroker.c_str(), (char *) "CONNECTIONINFO", lv_value);
		if(lv_error)
		{
			LOG_AND_TRACE(1, ("reg_change_do_route: Cannot get source broker connection info from monitor, exit..\n"));
			goto do_route_failed;
		}

		split(SourceSplitVec, lv_value, is_any_of(" "), token_compress_on);
		if(SourceSplitVec.size()!=3)
		{
			LOG_AND_TRACE(1, ("reg_change_do_route: Cannot get source broker connection info, exit..\n"));
			goto do_route_failed;
		}

		//Get node, source ip, and sources port.
		split(SourceNodeInfo, SourceSplitVec[0], is_any_of(":"), token_compress_on);
		split(SourceIpInfo, SourceSplitVec[1], is_any_of(":"), token_compress_on);
		split(SourcePortInfo, SourceSplitVec[2], is_any_of(":"), token_compress_on);

		if(SourceIpInfo.size()!=2||SourcePortInfo.size()!=2)
		{
			LOG_AND_TRACE(1, ("reg_change_do_route: Cannot get source broker Node, IP or Port info, exit..\n"));
			goto do_route_failed;
		}

		//Get IP address for dest.
		lv_error = sp_get_ip_from_registry(lexical_cast<int>(DestNodeInfo[1]), dest_ip);

		if(lv_error)
		{
			LOG_AND_TRACE(1, ("reg_change_do_route: Cannot get dest broker real IP, exist..\n"));
			goto do_route_failed;
		}

		dest_port = DestPortInfo[1];

		//Get IP address for source.
		lv_error = sp_get_ip_from_registry(lexical_cast<int>(SourceNodeInfo[1]), source_ip);

		if(lv_error)
		{
			LOG_AND_TRACE(1, ("reg_change_do_route: Cannot get source broker real IP, exist..\n"));
			goto do_route_failed;
		}

		source_port = SourcePortInfo[1];

		LOG_AND_TRACE(1, ("reg_change_do_route: dest ip: %s dest port: %s source ip: %s source port: %s routingkey: %s\n",
						dest_ip.c_str(), dest_port.c_str(), source_ip.c_str(), source_port.c_str(), routingkey.c_str()));

		i_source_port = boost::lexical_cast<int>(source_port);

		while(retryTime>0)
		{
			try
			{
				//Connection info
				Connection conn(dest_ip + ":" + dest_port);
				conn.open();
				Session session = conn.createSession();

				//Create invoker using session.
				QpidMethodInvoker control(session);
				
				//Create link first, if the link already exists, the function does nothing and returns. 
				control.createLink(source_ip, i_source_port, true);
				
				//Create bridge for the routingkey, that is, add route.
				control.createBridge(source_ip + "," + source_port, "amq.topic", "amq.topic", routingkey, true);
				conn.close();

				LOG_AND_TRACE(1, ("reg_change_do_route: succeeded..\n"));

				//If no error, we will set a reg modifying the value to  'DONE'. 
				lv_error = sp_reg_set(MS_Mon_ConfigType_Cluster,
									  CLUSTER_GROUP,
									  pp_change->key, "DONE");

				LOG_AND_TRACE(1, ("reg_change_do_route: reg_set done.\n"));
				return SP_SUCCESS;
			}
			catch(std::exception& e)
			{
				//Retry 3 times if there's error.
				retryTime--;
				LOG_AND_TRACE(1, ("reg_change_do_route: error: %s , sleep 5 seconds..\n", e.what()));
				sleep(5);
			}

			LOG_AND_TRACE(1, ("reg_change_do_route: reg_set failed.\n"));
			goto do_route_failed;
		}
	}
	else
	{
		//The dest broker name SplitVec[1] is not in my process list.
		//Let's see if it has the node id same as my node id.
		boost::cmatch mat;
		boost::regex reg( "\\d+" );   
		//Extract the digital part.
		if(boost::regex_search(SplitVec[1].c_str(), mat, reg))
		{
			//Compare if it is my node id, or is not an id within cluster. 
			//If yes, this broker really does not exist, set reg 'FAILURE'.
			if(iv_myNid==lexical_cast<int>(mat[0]) || iv_numConfigNodes-1 < lexical_cast<int>(mat[0]))
			{
				LOG_AND_TRACE(1, ("reg_change_do_route: unrecognized broker name: %s, FAILED.\n", SplitVec[1].c_str()));
				goto do_route_failed;
			}
		}
	}

	if(!doRoute)
		return SP_SUCCESS;
		
do_route_failed:
	//Set the registry if failed.
	LOG_AND_TRACE(1, ("reg_change_do_route: reg_set op: FAILED.\n"));
	lv_error = sp_reg_set(MS_Mon_ConfigType_Cluster,
						  CLUSTER_GROUP,
						  pp_change->key, "FAILED");
	return lv_error;
}



int spProxy::process_nodePrepare (MS_Mon_Msg *pp_msg)
{
    int lv_error = SP_SUCCESS;

    SPTrace (2, ("spProxy::process_nodePrepare ENTRY with nid %d\n",
                 pp_msg->u.up.nid));

    // work around the incorrect setting of takeover.
    // When the takeover flag works as expected, remove the #ifdef and
    // everything in between the #else--#endif (and the #else #endif lines
    // too).
#ifdef TAKEOVER_FLAG_FIXED_CR5478
    if (pp_msg->u.down.takeover)
    {
       SPTrace (2, ("spProxy::process_nodePrepare EXIT, takeover - no-op,"
                    " with nid %d\n", pp_msg->u.down.nid));
       return lv_error;
    }
#else
    // takeover is supposed to be set if a spare node comes online to take
    // over for a node that died. But it is also being set on nodePrepare
    // if a node dies and then restarts. this is not right. ignore the flag
    // for now.
    if (pp_msg->u.up.takeover)
    {
       SPTrace (2, ("spProxy::process_nodePrepare takeover flag incorrectly"
                    " set for node %d, ignoring!\n",pp_msg->u.up.nid));
    } else {
       SPTrace (2, ("spProxy::process_nodePrepare takeover flag FIXED!!!"
                    " change process_nodePrepare!!!\n"));
    }
#endif

    gv_nodes.set_state(pp_msg->u.up.nid, MS_Mon_State_Up);

    for (int lv_inx = 0; lv_inx < iv_list.iv_num_processes; lv_inx++)
    {
        // reset from backup processes to primary. This just changes internal
        // bookkeeping. Actual processes are changed in process_nodeup.
        lv_error = iv_list.ip_list[lv_inx].reset_brokers_up(pp_msg->u.up.nid);
        if (lv_error)
        {
           SPTrace (2, ("spProxy::process_nodePrepare (reset_brokers_up)"
                        " error %d\n",lv_error));
           return lv_error;
        }
    }
    SPTrace (2, ("spProxy::process_nodePrepare EXIT with error %d\n", lv_error));
    return lv_error;
}

// ------------------------------------------------------------------------
//
// spProxy::process_nodeup
// Purpose : process a node up received from the monitor.  This is when a
//           node is re-integrated.
//
// ------------------------------------------------------------------------
int spProxy::process_nodeup (MS_Mon_Msg *pp_msg)
{
    int lv_error = SP_SUCCESS;

    LOG_AND_TRACE (2, ("spProxy::process_nodeup ENTRY with nid %d\n",pp_msg->u.up.nid));

    if (pp_msg->u.up.takeover)
    {
       LOG_AND_TRACE (2, ("spProxy::process_nodeup EXIT, takeover - no-op,  with nid %d\n",pp_msg->u.up.nid));
       return lv_error;
    }

    // routes from destination to source brokers have been recomputed in process_nodePrepare,
    // but that is just an internal housekeeping update. Now that the node is all the way
    // up, we can stop and restart the processes that are affected, thus reconnecting the
    // routes to the current (primary) processes.

    gv_nodes.set_state(pp_msg->u.up.nid, MS_Mon_State_Up);

    for (int lv_inx = 0; lv_inx < iv_list.iv_num_processes; lv_inx++)
    {
        // The primary node just came back up.  Kill this one and set it as the backup again
        if (iv_list.ip_list[lv_inx].process_info.primary_node == pp_msg->u.up.nid)
        {
           LOG_AND_TRACE (3, ("spProxy::process_nodeup stop/reset %s\n", iv_list.ip_list[lv_inx].get_name()));
           lv_error = iv_list.ip_list[lv_inx].stop_process(iv_myNid);
           iv_list.ip_list[lv_inx].reset_node(true);
        }

        if (lv_error)
        {
           LOG_AND_TRACE (2, ("spProxy::process_nodeup stop_process failed with %d, continuing\n",lv_error));
        }
    }
    LOG_AND_TRACE (2, ("spProxy::process_nodeup EXIT with error %d\n", lv_error));
    return lv_error;
}

// ------------------------------------------------------------------------
//
// spProxy::process_nodedown
// Purpose : process a node down received from the monitor.  We will not
//           receive a node down when a spare is available
//
// ------------------------------------------------------------------------
int spProxy::process_nodedown (MS_Mon_Msg *pp_msg)
{
    int lv_error = SP_SUCCESS;

    LOG_AND_TRACE (2, ("spProxy::process_nodedown ENTRY with nid %d\n",pp_msg->u.down.nid));
    if (pp_msg->u.down.takeover)
    {
       LOG_AND_TRACE (2, ("spProxy::process_nodedown EXIT, takeover - no-op,  with nid %d\n",pp_msg->u.down.nid));
       return lv_error;
    }

    gv_nodes.set_state(pp_msg->u.down.nid, MS_Mon_State_Down);
    iv_numPartialUpNodes--;
    iv_numFullyUpNodes--;

    // Cannot modify SP_ALL_PROXY_UP; it is only used to track
    // startup sequence. Instead, use iv_state to indicate
    // the proxy's status.
    //
    if (iv_numFullyUpNodes < iv_numConfigNodes)
    {
        SPTrace (3, ("spProxy::process_nodedown : changing iv_state to PROXY_STATE_STARTING\n"));
        iv_state = PROXY_STATE_STARTING;
    }

    //reset our sourcebrokers and destbrokers if applicable...  That way when we get our process up
    // messages and when when a given process dies, it will get connected to the right place.
    // It will be a no-op if there are no source/dest brokers for this process.
   for (int lv_inx = 0; lv_inx < iv_list.iv_num_processes; lv_inx++)
   {
        lv_error = iv_list.ip_list[lv_inx].reset_brokers(pp_msg->u.down.nid);
        if (lv_error)
        {
           LOG_AND_TRACE (2, ("spProxy::process_nodedown (reset_brokers) error %d\n",lv_error));
           return lv_error;
        }
     }

    // now go through and see if we need to start anything.
    // check if we are the backup
    for (int lv_inx = 0; lv_inx < iv_list.iv_num_processes; lv_inx++)
    {
        // we are the backup for a process on the down node
        if ((iv_list.ip_list[lv_inx].is_backup()) &&
            (iv_list.ip_list[lv_inx].process_info.node == pp_msg->u.down.nid))
        {
             SPTrace (3, ("spProxy::process_nodedown starting process %s\n",
                            iv_list.ip_list[lv_inx].get_name()));

             // all is ready for this process to start except for the connection node and name.
             // reset that to be our current node and proper name and then start the process
             iv_list.ip_list[lv_inx].reset_node(false);

             lv_error = iv_list.ip_list[lv_inx].start_process(iv_myNid,
                                    iv_list.ip_list[SP_NODE_BROKER_INDEX].connection_info.port,
                                    iv_list.ip_list[SP_NODE_BROKER_INDEX].connection_info.ip);

             //route the node broker to it if applicable
             if ((!lv_error) && (iv_list.ip_list[lv_inx].type() == SP_BROKER))
             {
                 if (iv_list.ip_list[lv_inx].is_category_broker())
                 {
                         // Setup routing between this category broker and the node broker
                         lv_error = sp_route_broker(ip_myIp,
                                         iv_list.ip_list[SP_NODE_BROKER_INDEX].connection_info.port,
                             iv_list.ip_list[SP_NODE_BROKER_INDEX].process_params.sp_params.subtype,
                             ip_myIp, iv_list.ip_list[lv_inx].connection_info.port,
                             iv_list.ip_list[lv_inx].process_params.sp_params.subtype );
                  }

                 // potentially route to the destination brokers
                 lv_error = iv_list.ip_list[lv_inx].route_helper(iv_myNid,ip_myIp, true);
             }  // !lv_error && SP_BROKER
        }  // we are backup for process on downed node
    }  // endfor

    LOG_AND_TRACE (2, ("spProxy::process_nodedown EXIT with error %d\n",lv_error));
    return lv_error;
}
// ------------------------------------------------------------------------
//
// spProxy::process_death
// Purpose : process a death notice received from the monitor
//
// ------------------------------------------------------------------------
int spProxy::process_death (MS_Mon_Msg *pp_msg)
{
    int lv_error = SP_SUCCESS;

    LOG_AND_TRACE (1, ("spProxy::process_death ENTRY with process type %d,"
                 " process %s\n",
                 pp_msg->u.death.type, pp_msg->u.death.process_name ));

    // we don't do anything with a proxy death.
    // re-routing happens when the registry changes
    if (pp_msg->u.death.type == MS_ProcessType_SPX)
    {
        sleep (5);
        //reset our sourcebrokers and destbrokers if applicable...
        // That way when we get our process up messages and when when a
        // given process dies, it will get connected to the right place.
        // It will be a no-op if there are no source/dest brokers for
        // this process.
       for (int lv_inx = 0; lv_inx < iv_list.iv_num_processes; lv_inx++)
       {
            SP_Process * lv_process;

            lv_process = &iv_list.ip_list[lv_inx];
            lv_error = lv_process->update_source_broker_ip();
            if (lv_error)
            {
               SPTrace (2, ("spProxy::process_death (update_source_broker_ip)"
                            " error %d\n",lv_error));
               return lv_error;
            }
            lv_error = lv_process->update_dest_broker_ip();
            if (lv_error)
            {
               SPTrace (2, ("spProxy::process_death (update_dest_broker_ip)"
                            " error %d\n",lv_error));
               return lv_error;
            }
        }
        SPTrace (2, ("spProxy::process_death EXIT\n"));
        return SP_SUCCESS;
    }

    SPTrace (3, ("spProxy::process_death received death notice for %s (node %d)\n",
                 pp_msg->u.death.process_name, pp_msg->u.death.nid ));

    // check if its any of our topic brokers
    if (pp_msg->u.death.nid == iv_myNid)
    {
        for (int lv_inx = 0; lv_inx < iv_list.iv_num_processes; lv_inx++)
        {
             SP_Process * lv_process;

             lv_process = &iv_list.ip_list[lv_inx];

             // if the process that died matches one in our list we connect
             // with...
             if (strcmp(lv_process->get_name(), pp_msg->u.death.process_name) == 0)
             {
                  SPTrace (3, ("spProxy::process_death process %s found\n",
                               pp_msg->u.death.process_name));

                  // this will happen with node reintegration.  One of our
                  // backup nodes came back up and we need to NOT restart
                  // the process
                  if (lv_process->is_backup())
                  {
                      LOG_AND_TRACE (3, ("spProxy::process_death EXIT process %s is"
                                   " now a backup, ignoring\n",
                               pp_msg->u.death.process_name));
                      return SP_SUCCESS;
                  }
                  // start it -- if the user has not disabled it.
                  if (is_okToRun(lv_process->type()))

                  {
                          lv_error = lv_process->start_process(iv_myNid,
                                          iv_list.ip_list[SP_NODE_BROKER_INDEX].connection_info.port,
                                          iv_list.ip_list[SP_NODE_BROKER_INDEX].connection_info.ip);

                          // Most routes are established from the destination
                          // node to the source node. But broker nodes do the
                          // opposite; they route from source to destination.
                          // So, we will start all routes for which the
                          // current broker is the destination.
                          if ((lv_inx != SP_NODE_BROKER_INDEX) &&
                              (lv_process->type() == SP_BROKER) && (!lv_error))
                          {
                                  SPTrace (3, ("spProxy::process_death rerouting to local broker\n"));

                                  if (lv_process->is_category_broker())
                                  {
                                          // if a CATegory broker is restarted, set up routes between it and the NLB
                                          // get most up to date ip
                                          std::string lv_ip;
                                          lv_error = sp_get_ip_from_registry (lv_process->connection_info.node, lv_ip);
                                          if (!lv_error)
                                                  lv_error = sp_route_broker(lv_ip,
                                                                  iv_list.ip_list[SP_NODE_BROKER_INDEX].connection_info.port,
                                                                  iv_list.ip_list[SP_NODE_BROKER_INDEX].process_params.sp_params.subtype,
                                                                  lv_ip,
                                                                  lv_process->connection_info.port,
                                                                  lv_process->process_params.sp_params.subtype);
                                          break;
                                  }   // end it's a CATegory broker
                                  else
                                  {   // it's not a CATegory broker
                                          // The only broker subtypes left are CONSolodating and External Facing (EFB)
                                          // we must restart routes from these broker subtypes to the current broker
                                          for (int lv_inx_j = 0; lv_inx_j < iv_list.iv_num_processes; lv_inx_j++)
                                          {
                                                  SP_Process * lv_process_j;

                                                  lv_process_j = &iv_list.ip_list[lv_inx_j];
                                                  if ((lv_process_j->type() == SP_BROKER))
                                                  {
                                                          for (int lv_inx_broker = 0; lv_inx_broker < SP_NUM_DEST_BROKERS; lv_inx_broker++)
                                                          {
                                                                  // get most up to date ip
                                                                  std::string lv_ip;
                                                                  lv_error = sp_get_ip_from_registry (iv_list.ip_list[lv_inx].connection_info.node, lv_ip);
                                                                  if (lv_process_j->process_params.sp_params.destbrokers[lv_inx_broker].name == pp_msg->u.death.process_name)
                                                                          // TODO/BUG: We save the error but never check it. It gets overwritten each time through the loop.
                                                                          lv_error = sp_route_broker(lv_ip,
                                                                                                     lv_process_j->connection_info.port,
                                                                                                     lv_process_j->process_params.sp_params.subtype,
                                                                                                     lv_ip,
                                                                                                     lv_process->connection_info.port,
                                                                                                     lv_process->process_params.sp_params.subtype);
                                                          }  // endfor
                                                  }  // endif type is SP_BROKER
                                          }  // endfor loop over num_processes
                                          break;
                                  }  // end-else it's not a CATegory broker
                          }  // endif it's a route from some process to the current broker
                  } // endif okToRun
             } // endif process name matches
           } // endfor loop over num_processes
        }  // endif process died on our node

    SPTrace (2, ("spProxy::process_death EXIT with error %d\n", lv_error));
    return lv_error;
}
// -----------------------------------------------------------------------
// spProxy::process_msg
// Purpose - process messages incoming to the Proxy
// -----------------------------------------------------------------------
void spProxy::process_msg(BMS_SRE *pp_sre)
{
    char                   la_recv_buffer[4096];
    MS_Mon_Msg             lv_msg;
    int                    lv_error = SP_SUCCESS;

    SPTrace (2, ("spProxy::process_msg ENTRY\n"));

    short lv_ret = BMSG_READDATA_(pp_sre->sre_msgId,           // msgid
                            la_recv_buffer,              // reqdata
                            pp_sre->sre_reqDataSize);    // bytecount
    lv_error = static_cast<int> (lv_ret);

    if ((!lv_error) && (pp_sre->sre_flags & XSRE_MON))
    {
        memcpy (&lv_msg, la_recv_buffer, sizeof (MS_Mon_Msg));

        // At this point we will not reply with an error so get the reply
        // out of the way so we can do some real processing
        XMSG_REPLY_(pp_sre->sre_msgId,       /*msgid*/
                NULL,           /*replyctrl*/
                0,              /*replyctrlsize*/
                NULL,           /*replydata*/
                0,              /*replydatasize*/
                0,              /*errorclass*/
                NULL);          /*newphandle*/

        switch (lv_msg.type)
        {
        case MS_MsgType_Change:
        {
             lv_error = process_registry_change(&lv_msg.u.change);
             break;
        }
        case MS_MsgType_ProcessDeath:
        {
             SPTrace (3, ("spProxy::process_msg processing ProcessDeath\n"));
             lv_error = process_death(&lv_msg);
             break;
        }
        case MS_MsgType_NodeDown:
        {
             SPTrace (3, ("spProxy::process_msg processing NodeDown\n"));
             lv_error = process_nodedown(&lv_msg);
             break;
        }
        case MS_MsgType_NodeUp:
        {
             SPTrace (3, ("spProxy::process_msg processing NodeUp\n"));
             lv_error = process_nodeup(&lv_msg);
             break;
        }
        case MS_MsgType_NodePrepare:
        {
             SPTrace (3, ("spProxy::process_msg processing NodePrepare\n"));
             lv_error = process_nodePrepare(&lv_msg);
             break;
        }
        case MS_MsgType_Shutdown:
        {
             SPTrace (1, ("spProxy::process_msg processing shutting down.\n"));
             bool lv_success = iv_list.kill_processes(iv_myNid);
             sleep(5); // give the UNCs a little time
             sp_stop_all_threads();
             // if we weren't able to stop, we need to do a hard down so the monitor kills them
             if (lv_success)
             {
                 LOG_AND_TRACE (1, ("spProxy::sp_proxy_exit Proxy Stopping cleanly\n"));
                 msg_mon_process_shutdown();
             }
             else
                 LOG_AND_TRACE (1, ("spProxy::sp_proxy_exit Proxy Stopping hard, error in stopping processes\n"));
             exit (0);
             break;
        }
        case MS_MsgType_Event:
        case MS_MsgType_UnsolicitedMessage:
        default:
        {
             LOG_AND_TRACE (3, ("spProxy::process_msg ignoring message type %d\n", lv_msg.type));
             break;
        }
        };

    }


    if (lv_error)
    {
        // report the error but don't exit. Proxy should keep running
        LOG_AND_TRACE (1, ("spProxy::process_msg EXIT with error %d. Exiting!\n", lv_error));
    }

    SPTrace (2, ("spProxy::process_msg EXIT.\n"));
}

// -------------------------------------------------------
//
// main
// Purpose : main processing loop
//
// -------------------------------------------------------
int main(int pv_argc, char *pp_argv[])
{
    std::string     la_buf;
    char            la_event_data[MS_MON_MAX_SYNC_DATA];
    int             lv_event_len;
    int             lv_error;
    int             lv_ret;
    BMS_SRE         lv_sre;
    bool            lv_start_od = false;
    int             lv_started = 0;
    spProxy         sp_proxy;

    // Security requirement: all files created must show no permissions for "others"
    // so change default umask from 022 to 027
    umask(027);

    CALL_COMP_DOVERS(sp_proxy, pv_argc, pp_argv);
    LOG_AND_TRACE(1, ("main: %s\n", CALL_COMP_GETVERS(sp_proxy)));
    // seaquest related stuff
    lv_error = msg_init(&pv_argc, &pp_argv);
    if (!lv_error)
        lv_error = msg_mon_process_startup(true); // server

    if (!lv_error)
        msg_mon_enable_mon_messages (1);
    else
    {
        LOG_AND_TRACE (1, ("main: Seabed init failed with error of %d.  Exiting.\n", lv_error));
        exit(1);
    }


    msg_debug_hook ("spp.hook", "spp.hook");

    lv_ret = sp_proxy.pre_init();
    if (lv_ret != 0)
    {
        char la_buf[SP_MAX_ERROR_TEXT];
        sprintf(la_buf, "main: proxy pre_init returned error of %d.  Exiting.\n", lv_ret);
        LOG_AND_TRACE (1, (la_buf));
        sp_proxy.sp_proxy_exit(la_buf);
    }

    lv_error = sp_reg_get(MS_Mon_ConfigType_Process,
                  (char *) CLUSTER_GROUP, (char *) SP_PROXIES_STARTED, la_buf);

    if (!lv_error)
       lv_started = atoi (la_buf.c_str());

    SPTrace (1, ("main: SP_PROXIES_STARTED returned %d, with error %d\n", lv_started, lv_error));

    // if the proxy subsystem is already started, no need to wait (failure scenarios)
    if (!lv_started)
        msg_mon_event_wait (PROXY_CONTINUE_EVENT_ID, &lv_event_len, la_event_data);
    else
    {
        sp_proxy.set_failed_start(true);
        lv_start_od = true;
        sp_all_processes_up();
    }

     SPTrace (1, ("main: Proxy FAILED start %d\n", sp_proxy.get_failed_start()));
    // proxy init and start up the wrapper that will exec into the broker
    lv_ret = sp_proxy.init();
    if (lv_ret != 0)
    {
        char la_buf[SP_MAX_ERROR_TEXT];
        sprintf(la_buf, "main: proxy init returned error of %d.  Exiting.\n", lv_ret);
        LOG_AND_TRACE (1, (la_buf));
        sp_proxy.sp_proxy_exit(la_buf);
    }

    LOG_AND_TRACE (1, ("main: starting Seapilot Processes\n"));
    lv_ret = sp_proxy.start_processes();

    if (lv_ret != 0)
    {
        char la_buf[SP_MAX_ERROR_TEXT];
        sprintf(la_buf, "main: starting processes returned error of %d.  Exiting.\n", lv_ret);
        LOG_AND_TRACE (1, (la_buf));
        sp_proxy.sp_proxy_exit(la_buf);
    }

    LOG_AND_TRACE (1, ("main: steady state\n"));

    for(;;)
    {
        XWAIT(LREQ, -1);
        do
        {
            lv_error = BMSG_LISTEN_((short *) &lv_sre, // sre
                                 BLISTEN_ALLOW_IREQM,
                                 0);                   // listenertag
            if (lv_error != BSRETYPE_NOWORK)
                 sp_proxy.process_msg(&lv_sre);

        } while (lv_error != BSRETYPE_NOWORK);
    }

    LOG_AND_TRACE (1, ("main: EXIT.\n"));
    msg_mon_process_shutdown();
}
