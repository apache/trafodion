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

#ifndef TMRMTSEBRANCH_H_
#define TMRMTSEBRANCH_H_
#include "dtm/tm.h"
#include "tmglob.h"
#include "tmmutex.h"
#include "tmrmtse.h"


// TSE RM implementation.  This is also used for TSE branches listed within a transaction.
class RM_Info_TSEBranch {
  private:
      char        ia_name[256];
      TSEBranch_state iv_state;
      int32       iv_nid;
      int32       iv_rmid;
      bool        iv_in_use;   // This branch is in use - ie the RM is up.
      bool        iv_partic;   // This branch is participating in the transaction.
      bool        iv_resolved; // Only set once the transaction has received 
                               // xa_rollback or xa_prepare replies from the associated TSE.
      bool        iv_enlisted;
      int32       iv_pid;
      int32       iv_totalBranchesLeftToRecover;

   public:
      XID     iv_xid;

      RM_Info_TSEBranch() : iv_state(TSEBranch_UP), iv_totalBranchesLeftToRecover(0) {
         iv_rmid = iv_pid = iv_nid = -1; 
         iv_in_use = iv_partic = iv_enlisted = iv_resolved = false;
         memset(&iv_xid, 0, sizeof(XID));
         memset(ia_name, 0, 256);         
      }
      ~RM_Info_TSEBranch(){}

      void init (int32 pv_pid, int32 pv_nid, char *pp_name, int32 pv_rmid, 
               bool pv_is_ax_reg, TSEBranch_state pv_state= TSEBranch_DOWN, 
               bool pv_clean_init = false);

      // get and set methods
      void state(TSEBranch_state pv_state)
               {iv_state = pv_state;}
      TSEBranch_state state () {return iv_state;}
#ifdef DEBUG_MODE
      void in_use( bool pv_in_use);
      void partic( bool pv_partic );
      void resolved( bool pv_resolve );
      void enlisted( bool pv_enlist );
#else
      void in_use( bool pv_in_use)
                 {iv_in_use = pv_in_use;}
      void partic( bool pv_partic )
                 {iv_partic = pv_partic;}
      void resolved( bool pv_resolve )
                 {iv_resolved = pv_resolve;}
      void enlisted( bool pv_enlist )
                 {iv_enlisted = pv_enlist;}
#endif //DEBUG_MODE
      bool in_use () {return iv_in_use;}
      bool partic() {return iv_partic;}
      bool resolved() {return iv_resolved;}
      bool enlisted() {return iv_enlisted;}
      int32 nid() {return iv_nid;}
      void nid(int32 pv_nid) {iv_nid=pv_nid;}
      int32 pid() {return iv_pid;}
      void pid(int32 pv_pid) {iv_pid=pv_pid;}
     char * pname() {return (char *) ia_name;}
      void rmid(int32 pv_rmid)
                  {iv_rmid = pv_rmid;}
      int32 rmid() { return iv_rmid;}
      int32 totalBranchesLeftToRecover() {return iv_totalBranchesLeftToRecover;}
      void totalBranchesLeftToRecover(int32 pv_total) 
         { iv_totalBranchesLeftToRecover=pv_total; }
      void inc_totalBranchesLeftToRecover() { iv_totalBranchesLeftToRecover++; }
      void dec_totalBranchesLeftToRecover(); 

      void close();
      void fail();
      void up();
      bool add_partic(TM_Txid_Internal pv_transid);

      void copyto(RM_INFO *pp_output);

};
 
#endif // TMRMTSEBRANCH_H_
