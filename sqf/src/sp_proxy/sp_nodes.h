// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef SP_NODES_H_
#define SP_NODES_H_

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "seabed/ms.h"
#include "seabed/pctl.h"
#include "seabed/pevents.h"

// ------------------------------------------------------------------------------
//
// ALL levels of classes need access to our nodes array
//
// ------------------------------------------------------------------------------

typedef struct
{
    int               node;
    bool              spare;
    MS_MON_PROC_STATE state;
} SP_NODE_INFO_STRUCT;

// ----------------------------------------------------------------------------
//
// SP_Nodes will hold data about each configured node in the system.  The
// logical nid will be the same as the index.
//
// // --------------------------------------------------------------------------
class SP_Nodes
{
   public:
    SP_Nodes()
    {
        iv_myNid = -1;
        for (int lv_inx = 0; lv_inx < MS_MON_MAX_NODE_LIST; lv_inx++) {
             ia_node_info[lv_inx].node = -1;
             ia_node_info[lv_inx].spare = 0;
             ia_node_info[lv_inx].state = MS_Mon_State_Unknown;
        }
    }
    ~SP_Nodes(){}

   void set_my_node(int pv_nid) {iv_myNid = pv_nid;}
   int  get_my_node() {return iv_myNid;}
   void set_node(int pv_index, int pv_node) {ia_node_info[pv_index].node = pv_node;}
   void set_spare(int pv_index, int pv_spare) {ia_node_info[pv_index].spare = pv_spare;}
   void set_state(int pv_index, MS_MON_PROC_STATE pv_state) {ia_node_info[pv_index].state = pv_state;}

   bool              get_spare(int pv_index) {return ia_node_info[pv_index].spare;}
   MS_MON_PROC_STATE get_state(int pv_index) {return ia_node_info[pv_index].state;}

   bool is_up(int pv_node) {
        if (ia_node_info[pv_node].state == MS_Mon_State_Up)
            return true;
        return false;
   }

   private:

   SP_NODE_INFO_STRUCT ia_node_info[MS_MON_MAX_NODE_LIST];
   int iv_myNid;
};

extern SP_Nodes gv_nodes;

#endif
