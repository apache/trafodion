// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2015 Hewlett-Packard Development Company, L.P.
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

#ifndef TMLIBTXN_H_
#define TMLIBTXN_H_

#include "dtm/tmtransid.h"
#include "dtm/tmtransaction.h"
#include "tmmap.h"

// --------------------------------------------------------------------------
// TMLIB_EnlistedTxn_Object
// -- will be maintained in a list by TMLIB_ThreadTxn_Object
// --------------------------------------------------------------------------
class TMLIB_EnlistedTxn_Object
{
public: 
    TMLIB_EnlistedTxn_Object(TM_Transaction *pp_trans);
    ~TMLIB_EnlistedTxn_Object();

    TM_Transaction *getTrans () {return ip_trans;}
    int decrease_depth()        {return --iv_depth;}
    int increase_depth()       {return ++iv_depth;}

    int decrease_join_count() {return --iv_join_count;}
    int increase_join_count() {return ++iv_join_count;}

    int decrease_outstanding_ios() {return --iv_outstanding_ios;}
    int increase_outstanding_ios() {return ++iv_outstanding_ios;}
    int outstanding_ios() {return iv_outstanding_ios;}

    void propagated_tx(bool pv_propagated) {iv_propagated = pv_propagated;}
    bool propagated_tx() {return iv_propagated;}

    void suspended_tx(bool pv_suspended) {iv_suspended = pv_suspended;}
    bool suspended_tx() {return iv_suspended;}

    TM_Transseq_Type get_startid();
    void set_startid(long startid);

private:
    TM_Transaction *ip_trans;
    int             iv_depth;
    int             iv_join_count;
    int             iv_outstanding_ios;
    bool            iv_propagated;
    bool            iv_suspended;
    TM_Transseq_Type iv_startid;

    TMLIB_EnlistedTxn_Object() {}
};

// --------------------------------------------------------------------------
// TMLIB_ThreadTxn_Object
// -- will hold a list of all transactions associated with this thread
// --------------------------------------------------------------------------
class TMLIB_ThreadTxn_Object
{
public:
    TMLIB_ThreadTxn_Object();
    ~TMLIB_ThreadTxn_Object();

    // add/remove transaction to enlistment list
    void add_trans (TM_Transaction *pp_trans);
    TM_Transaction * remove_trans (TM_Transaction *pp_trans, bool pv_delete_enlisted = true);

    // increase/decrease depth of current transaction
    int  decrease_current_depth();
    int  increase_current_depth ();

    // get/set/delete current 
    void            delete_current();
    TM_Transaction *get_current() 
    { 
       if (ip_enlisted_trans)
           return ip_enlisted_trans->getTrans();
       return NULL;
    }
    void set_current(TM_Transaction *pp_trans);

    void set_current_propagated(bool pv_propagated);
    bool get_current_propagated();

    void set_current_suspended(bool pv_suspended);
    bool get_current_suspended();

    int decrease_current_ios();
    int increase_current_ios();
    int get_current_ios();

    // misc helper methods
    TM_Transaction * get_trans (TM_Native_Type pv_transid);
    TM_Transaction * get_trans (unsigned int pv_tag);
    TM_Transseq_Type get_startid();
    void set_startid(long startid);
    void is_initialized (bool pv_init) { iv_initialized = pv_init;}

    // return current enlisted object,
    TMLIB_EnlistedTxn_Object *get_enlisted(TM_Native_Type pv_transid);

    private:
        TM_MAP iv_all_trans;
        TM_MAP iv_all_tags; //maps tags to TM_Transaction
        TMLIB_EnlistedTxn_Object *ip_enlisted_trans;
        bool iv_initialized;
        TM_Transseq_Type iv_startid;
};

#endif
