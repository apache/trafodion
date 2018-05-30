// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
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
#include "tmseqnum.h"
#include "javaobjectinterfacetm.h"

//
// TM handle information 
//
typedef struct _tmlib_h_as_0 {
  TPT_DECL (iv_phandle);
  int32 iv_pid;
  short  iv_open;
} tmlib_tmhandle;

extern __thread JNIEnv* _tlp_jenv;
extern __thread bool  _tlv_jenv_set;

//
// process specific information
//
extern bool gv_tmlib_initialized;

class TMLIB : public JavaObjectInterfaceTM
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
        CtmSeqNum *ip_seqNum;
        int32 iv_seqNum_blockSize;
        bool iv_localBegin;
        
        // JNI interface
        enum JAVA_METHODS {
               //
               JM_CTOR= 0,
               JM_INIT1,  
               JM_ABORT,
               JM_TRYCOMMIT,
               JM_LAST_HBASETXCLIENT,
               //RMInterface
               JM_CLEARTRANSACTIONSTATES,
               JM_LAST
        };

        static const char *hbasetxclient_classname;
        static const char *rminterface_classname;

        static jclass hbasetxclient_class;
        static jclass RMInterface_class;

        static JavaMethodInit *TMLibJavaMethods_;
 
        static bool javaMethodsInitialized_; 
        static bool enableCleanupRMInterface_;
        // this mutex protects this class and JaveMethods_ initialization
        static TM_Mutex *initMutex_;
        short setupJNI();
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
        short add_or_update(TM_Transid pv_transid, TM_Transseq_Type pv_startid,
                            bool pv_can_end = false, int pv_tag = -1);
        bool clear_entry (TM_Transid pv_transid, bool pv_server, /*bool pv_suspend,*/ 
                           bool pv_force = true);
        bool is_enlisted (int32 pv_node) {return ia_is_enlisted[pv_node];}
        void enlist (int32 pv_node) {ia_is_enlisted[pv_node] = true;}
        bool reinstate_tx(TM_Transid *pv_transid, bool pv_settx = false);

        void initialize();

        int initJNI(); // Used only when a JNI connection is needed

        CtmSeqNum *seqNum() {return ip_seqNum;}
        bool localBegin() {return iv_localBegin;}
        void localBegin(bool pv_localBegin) {iv_localBegin=pv_localBegin;}
        int32 seqNum_blockSize() {return iv_seqNum_blockSize;}
        void seqNum_blockSize(int32 pv_blockSize) {iv_seqNum_blockSize=pv_blockSize;}
        bool enableCleanupRMInterface() {return enableCleanupRMInterface_;}
        void enableCleanupRMInterface(bool pv_bool) {enableCleanupRMInterface_=pv_bool;}

        bool open_tm(int pv_node, bool pv_startup = false);
        short send_tm(Tm_Req_Msg_Type *pp_req, Tm_Rsp_Msg_Type *pp_rsp, 
                    int pv_node);  
        short send_tm_link(char *pp_req, int buffer_size, Tm_Rsp_Msg_Type *pp_rsp,
                    int pv_node);

        // table get methods
        bool phandle_get ( TPT_PTR (pv_phandle), int pv_node);
        void phandle_set ( TPT_PTR (pa_phandle), int pv_node);

        int beginner_nid();
        unsigned int new_tag();

        // Local transaction methods
        short initConnection(short pv_nid);
        short abortTransactionLocal(long transactionID);
        short endTransactionLocal(long transactionID);
        void cleanupTransactionLocal(long transactionID);
        bool close_tm();
};

// helper methods, C style 
int   tmlib_init_req_hdr(short req_type, Tm_Req_Msg_Type *pp_req);
short tmlib_check_active_tx ( );
short tmlib_check_miss_param( void * pp_param);
short tmlib_send_suspend(TM_Transid pv_transid, bool pv_coord_role, int pv_pid);
short tmlib_check_outstanding_ios();
short HBasetoTxnError(short pv_HBerr);

//extern TMLIB_ThreadTxn_Object        gp_trans_thr;
extern __thread TMLIB_ThreadTxn_Object *gp_trans_thr;
extern TMLIB                         gv_tmlib;
#endif

