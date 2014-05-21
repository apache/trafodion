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

#ifndef SP_PROXY_H_
#define SP_PROXY_H_

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

#include "sp_process.h"
// --------------------------------------------------------------
//
// spProxy class - This class will manage brokers and UNCs
//
// --------------------------------------------------------------
class spProxy
{
    public:
    spProxy();
    ~spProxy();

    int  pre_init();
    int  init();
    int  process_death ( MS_Mon_Msg *pp_msg);
    void process_msg(BMS_SRE *pp_sre);
    int  process_nodedown (MS_Mon_Msg *pp_msg);
    int  process_nodePrepare (MS_Mon_Msg *pp_msg);
    int  process_nodeup (MS_Mon_Msg *pp_msg);
    int  process_registry_change(MS_Mon_Change_def *pp_change);
    bool get_failed_start() {return iv_failed_start;}
    void set_failed_start(bool pv_failed_start) {iv_failed_start = pv_failed_start;}
    int  start_processes();
    int  start_process(int pv_inx, bool pv_is_restart);
    int  stop_processes();
    void sp_proxy_exit(const char *reason);

    private:

    char            ia_myProcessName[MS_MON_MAX_PROCESS_NAME];
    SP_NODE_INFO_STRUCT ia_node_info[MS_MON_MAX_NODE_LIST];
    std::string     ip_myIp;
    SP_Process_List iv_list;
    int             iv_myNid;
    int             iv_myPid;
    bool            iv_failed_start;
    int             iv_numTotalNonBackupProcesses;   //actual non-backup local processes
    int             iv_numStartedProcesses;
    int             iv_numPartialUpNodes;  // actual nodes executing proxy, but maybe nothing else
    int             iv_numFullyUpNodes;    // actual nodes executing all configured seaquest processes
    int             iv_numConfigNodes;     // total configured nodes, not counting spare nodes
    int             iv_state;              // proxy state
    int             ia_runState[SP_MAX_PROC_TYPE]; // array that controls which process types to start

    // Values (states) for ia_runState
    enum {
        SP_STATE_OFF,
        SP_STATE_ON
    };

    inline bool is_okToRun(int const pv_index) const { return (ia_runState[pv_index] == SP_STATE_ON); }
    inline int    runState(int const pv_index) const { return (ia_runState[pv_index]); }


    // helper functions for process_registry_change
    int reg_change_proxy(MS_Mon_Change_def *pp_change);
    int reg_change_all_proxy(MS_Mon_Change_def *pp_change);
    int reg_change_single_node_processes(MS_Mon_Change_def *pp_change);
    int reg_change_process(MS_Mon_Change_def *pp_change);
    int reg_change_im_state(MS_Mon_Change_def *pp_change);
    int reg_change_do_route(MS_Mon_Change_def *pp_change);
};

// Expression. Returns TRUE if strings match, FALSE otherwise
// Note it is assumed that strlen(k_) <= strlen(v_)
// Do comparisons case-insensitive so person typing at sqshell
// prompt does not have to type in all uppercase.
#define SP_MATCH_KEY(k_, v_)  (strncasecmp(k_, v_, strlen(v_)) == 0)

int get_ip_from_registry( char *pp_process_name, std::string &ip_out);
#endif
