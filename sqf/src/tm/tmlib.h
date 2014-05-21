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

#ifndef TMLIB_H_
#define TMLIB_H_

#include <jni.h>

#include "dtm/tmtransaction.h"
#include "tmmap.h"
#include "tmlibmsg.h"
#include "tmlibtxn.h"
#include "tmlibglobal.h"
//
// TM handle information 
//
typedef struct _tmlib_h_as_0 {
  TPT_DECL (iv_phandle);
  int32 iv_pid;
  short  iv_open;
} tmlib_tmhandle;

//
// process specific information
//
class TMLIB
{
   private:
        TRANSACTION_DISTRIBUTION iv_txn_distribute;
        int iv_next_nid;
        int iv_node_count;
        uint32 iv_next_tag;
        TM_MAP iv_tm_table;
        tmlib_tmhandle ia_tm_phandle[MAX_NODES];
        bool ia_is_enlisted[MAX_NODES];
        bool iv_initialized;

    public:
        TMLIB();
        ~TMLIB(){}  
        // not needed right now TM_Mutex iv_mutex_tm;

        // the following members are not covered under mutex
        // they are only modified a single time'
        int iv_tm_pid; // Only used for non-transactional requests
        int iv_my_nid;
        int iv_my_pid;
        
        // methods
        short add_or_update(TM_Transid pv_transid, bool pv_can_end = false,
                            int pv_tag = -1);
        bool clear_entry (TM_Transid pv_transid, bool pv_server, /*bool pv_suspend,*/ 
                           bool pv_force = true);
        bool is_enlisted (int32 pv_node) {return ia_is_enlisted[pv_node];}
        void enlist (int32 pv_node) {ia_is_enlisted[pv_node] = true;}
        bool reinstate_tx(TM_Transid *pv_transid, bool pv_settx = false);

        void initialize();
        bool is_initialized() {return iv_initialized;}
        void is_initialized(bool pv_init) {iv_initialized = pv_init;}

        bool open_tm(int pv_node, bool pv_startup = false);
        short send_tm(Tm_Req_Msg_Type *pp_req, Tm_Rsp_Msg_Type *pp_rsp, 
                    int pv_node);  

        // table get methods
        bool phandle_get ( TPT_PTR (pv_phandle), int pv_node);
        void phandle_set ( TPT_PTR (pa_phandle), int pv_node);

        int beginner_nid();
        unsigned int new_tag();
};

// helper methods, C style 
int   tmlib_init_req_hdr(short req_type, Tm_Req_Msg_Type *pp_req);
short tmlib_check_active_tx ( );
short tmlib_check_miss_param( void * pp_param);
short tmlib_send_suspend(TM_Transid pv_transid, bool pv_coord_role, int pv_pid);
short tmlib_check_outstanding_ios();

//extern TMLIB_ThreadTxn_Object        gp_trans_thr;
extern __thread TMLIB_ThreadTxn_Object *gp_trans_thr;
extern TMLIB                         gv_tmlib;
#endif

