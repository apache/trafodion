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

#ifndef SP_PROCESS_H_
#define SP_PROCESS_H_

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "sp_common.h"
#include "sp_nodes.h"

#define SP_PARAM_BUFF 32
#define SP_MAX_PROCESS_RETRIES 7
#define SP_NUM_DEST_BROKERS 256
#define SP_MAX_STRING_LENGTH 1024

typedef struct
{
    std::string ip;
    std::string name;
    std::string primary_name;
    int         port;
    int         node;
    int         primary_node;
    std::string subtype;
} SP_BROKER_INFO;

typedef struct
{
    int metrics;
    int node;
    int primary_node;
    int backup;
    int startup;
    int trace;
    std::string mode;
    std::string group;
} SP_PROCESSINFO;

typedef struct
{
    SP_BROKER_INFO   sourcebroker;
    SP_BROKER_INFO   destbrokers[SP_NUM_DEST_BROKERS];
    std::string      subtype;
} SP_PARAMS;

typedef struct
{
   std::string obj;
   int type;
   SP_PARAMS sp_params;
   int  arg_count;
   int  org_arg_count;
   char sp_prog_args[SP_MAX_PROG_ARGS][1024];
} SP_PROCESSPARAMS;

typedef struct
{
    std::string ip;
    int         node;
    int         original_node;
    int         port;
} SP_CONNECTIONINFO;

typedef struct
{
    std::string proto_src;
    std::string text_catalog;
    std::string bad_messages;
    std::string overflow;
    std::string pm_context;
    std::string config_dir;
} SP_GENERAL_INFO;

// -----------------------------------------------------------------
//
// class SP_Process - This class will hold information and allow
//                    operations on a process
//
// -----------------------------------------------------------------
class SP_Process
{
    public:
    SP_Process();
    ~SP_Process();

    // Configuration information
    SP_PROCESSINFO process_info;
    SP_PROCESSPARAMS process_params;
    SP_CONNECTIONINFO connection_info;
    SP_GENERAL_INFO general_info;

    // general configuration set methods
    int  populate();
    int  set_general_info();
    int  set_connection_info (std::string &pp_connection_info);
    int  set_process_info (std::string &pp_process_info);
    int  set_process_params (std::string &pp_process_params);
    int  set_gen_process_params (std::string &pp_process_params);
    void add_process_param(std::string &pp_process_param_key, std::string& pp_process_param_value);
    void set_optional_process_params (std::string &pp_process_params);
    int  set_sub_only_process_params();
    int  set_pub_only_process_params();

    // specific configuration set methods
    int set_broker_process_params ();
    int set_unc_process_params ();
    int set_pm_process_params ();
    int set_dest_brokers (std::string &pp_dest_brokers);
    int set_backup_broker(int pv_inx);
    int set_source_broker();
    int set_lcsh_process_params ();
    int set_snmp_process_params();
    int set_smad_process_params();
    int set_nsa_process_params ();
    int set_tp_process_params();
    int set_una_process_params();
    int set_ptpa_process_params();
    int set_specific_params();

    // helper methods
    bool        is_backup() {return iv_backup;}
    void        set_backup(bool pv_backup) {iv_backup = pv_backup;}
    void        set_info(std::string &pp_name, std::string &pp_ip, int pv_nid)
                         {iv_name = pp_name; iv_ip_addr = pp_ip; iv_nid = pv_nid;}
    void        reset_node (bool pv_backup);
    const char  *get_name() {return iv_name.c_str();}
    int         get_pid() {return iv_pid;}
    int         get_nid() {return iv_nid;}
    int         start_broker(int pv_myNid);
    int         start_process(int pv_myNid, int pv_node_port, std::string& pv_node_ip);
    int         stop_process(int pv_proxy_nid);
    int         type() {return process_params.type;}
    int         reroute_remote_node(std::string &pp_processName);
    int         route_helper(int pv_nid, std::string &pv_myIp, int pv_remote_route);
    int         reset_brokers(int pv_nid);
    int         reset_brokers_up(int pv_nid);
    int         set_publish_info(int pv_broker_index);
    int         set_subscribe_info();
    int         start_helper(char *pp_prog, char *pp_name, char *pp_ret_name, int pv_argc,
                             char **pp_argv, SB_Phandle_Type  *pp_phandle, int pv_open,
                             int  *pp_oid, int pp_ptype, int pv_nid);
    int         start_gen_process(int pv_myNid, int pv_node_port, std::string& pv_node_ip,
                                      bool  pv_reset_retries);
    int         update_source_broker_ip();
    int         update_dest_broker_ip();
    bool        isStarted() const { return iv_started;}

    inline bool is_on_demand_process(void) {
        return  ((process_params.type == SP_UNC) ||
                         (process_params.type == SP_PM)  ||
                         (process_params.type == SP_UAA) ||
                         (process_params.type == SP_UNA) ||
                         (process_params.type == SP_PTPA) ||
                         (process_params.type == SP_SNMP));
    }

    int         start_on_demand_process(int pv_myNid, int pv_node_port, std::string& pv_node_ip);

    inline bool is_category_broker(void) {
        return ((process_params.sp_params.subtype == "CAT-EVENT")  ||
                         (process_params.sp_params.subtype == "CAT-PERF")   ||
                         (process_params.sp_params.subtype == "CAT-HEALTH") ||
                         (process_params.sp_params.subtype == "CAT-SECURITY"));
    }

    protected:
    bool        iv_backup;
    std::string iv_ip_addr;
    std::string iv_name;
    int         iv_nid;
    int         iv_pid;
    int         iv_retries;
    bool        iv_started;
    char        iv_sq_root[MS_MON_MAX_PROCESS_PATH];

    private:
    int         start_normal_process(int pv_myNid, int pv_node_port, std::string& pv_node_ip);

    int         reset_dest_brokers(int pv_nid);
    int         reset_source_brokers(int pv_nid);

    int         reset_dest_brokers_up(int pv_nid);
    int         reset_source_brokers_up(int pv_nid);

};

// ------------------------------------------------------------
//
// class SP_Process_List - Holds the list of processes
//
// -------------------------------------------------------------
class SP_Process_List
{

    public:
        SP_Process_List();
        ~SP_Process_List();

        bool kill_processes(int pv_proxy_nid);
        int  populate_process_list(std::string &pp_list, std::string &pp_ip, int pv_nid);
        int  populate_processes();

        SP_Process *ip_list;
        int         iv_num_processes;
        int         iv_num_backup_processes;

    private:
       int iv_my_nid;
};
#endif
