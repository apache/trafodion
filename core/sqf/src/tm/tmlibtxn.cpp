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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tmlibtxn.h"
#include "tmlib.h"
#include "tmlogging.h"

// --------------------------------------------------------------------
// EnlistedTxn_Object methods
// --------------------------------------------------------------------
TMLIB_EnlistedTxn_Object::TMLIB_EnlistedTxn_Object(TM_Transaction *pp_trans)
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (pp_trans == NULL)
    {
        sprintf(la_buf, "NULL transaction passed to Enlistment Object\n");
        tm_log_write(DTM_LIBTXN_INVALID_TXN, SQ_LOG_CRIT, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_EnlistedTxn_Object const. %s\n", la_buf), 1);
        abort ();
    }    

    ip_trans = pp_trans;
    iv_depth = 1;
    iv_join_count = 0;
    iv_propagated = iv_suspended =false;
    iv_outstanding_ios = 0;
}
TMLIB_EnlistedTxn_Object::~TMLIB_EnlistedTxn_Object()
{
    ip_trans = NULL;
    iv_depth = 0;
    iv_join_count = 0;
    iv_propagated = iv_suspended = false;
}

// -------------------------------------------------------------------
// TMLIB_EnlistedTxn_Object::set_startid
// -- set the startid in the current enlisted object.
// ------------------------------------------------------------------
void TMLIB_EnlistedTxn_Object::set_startid(TM_Transseq_Type pv_startid)
{
    TMlibTrace (("TMLIB_TRACE : TMLIB_EnlistedTxn_Object::set_startid ENTRY with startid:%ld \n", pv_startid), 2);
    iv_startid = pv_startid;
    TMlibTrace (("TMLIB_TRACE : TMLIB_EnlistedTxn_Object::set_startid EXIT\n"), 2);
}

// -------------------------------------------------------------------
// TMLIB_EnlistedTxn_Object::get_startid
// -- get the startid in the current enlisted object.
// ------------------------------------------------------------------
TM_Transseq_Type TMLIB_EnlistedTxn_Object::get_startid()
{
    TMlibTrace (("TMLIB_TRACE : TMLIB_EnlistedTxn_Object::get_startid EXIT returning startid:%ld\n", iv_startid), 2);
    return iv_startid;
}

// -----------------------------------------------------------------
// TheadTxn_Object methods
// -----------------------------------------------------------------
TMLIB_ThreadTxn_Object::TMLIB_ThreadTxn_Object()
{
    ip_enlisted_trans = NULL;
    iv_initialized = false;
    iv_startid = -1;
}

TMLIB_ThreadTxn_Object::~TMLIB_ThreadTxn_Object()
{
    ip_enlisted_trans = NULL;
}

// -------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::add_trans
//  - Create an enlistment object with the transaction, thread based
// -------------------------------------------------------------------
void TMLIB_ThreadTxn_Object::add_trans (TM_Transaction *pp_trans)
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (pp_trans == NULL)
    {
        sprintf(la_buf, "Transaction to be added to active transaction list is NULL\n");
        tm_log_write(DTM_LIBTXN_INVALID_TXN, SQ_LOG_CRIT, la_buf);  
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::add_trans - %s\n", la_buf), 1);
        abort ();
    }

    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::add_trans node (%d), seq num (%d) ENTRY\n",
                     pp_trans->getTransid()->get_node(), pp_trans->getTransid()->get_seq_num()), 2);
    TMLIB_EnlistedTxn_Object *lp_obj = new TMLIB_EnlistedTxn_Object(pp_trans);
    iv_all_trans.put(pp_trans->getTransid()->get_native_type(), lp_obj );
    ip_enlisted_trans = lp_obj;
    iv_all_tags.put(pp_trans->getTag(), lp_obj);
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::add_trans EXIT\n"), 2);
}


// ---------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::get_trans
// -- return a transaction given the legacy (64 bit) transid
// ---------------------------------------------------------------------
TM_Transaction * TMLIB_ThreadTxn_Object::get_trans (TM_Native_Type pv_transid)
{
    TMLIB_EnlistedTxn_Object *lp_to_return = (TMLIB_EnlistedTxn_Object *)iv_all_trans.get (pv_transid);
    if (lp_to_return)
        return lp_to_return->getTrans();
    return NULL;
}

// ---------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::get_trans
// -- return a transaction given the tag.
// ---------------------------------------------------------------------
TM_Transaction * TMLIB_ThreadTxn_Object::get_trans (unsigned int pv_tag)
{
    TMLIB_EnlistedTxn_Object *lp_to_return = (TMLIB_EnlistedTxn_Object *)iv_all_tags.get(pv_tag);
    if (lp_to_return)
        return lp_to_return->getTrans();
    return NULL;
}

// ----------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::remove_trans
// -- remove a transaction from the enlisted list.  We will delete the
//    enlistment object, but not the transaction.  That is up to the caller
// ----------------------------------------------------------------------
TM_Transaction * TMLIB_ThreadTxn_Object::remove_trans (TM_Transaction *pp_trans, bool pv_delete_enlisted)
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (pp_trans == NULL)
    {
        sprintf(la_buf, "Transaction identifier to be removed from active transaction list is NULL\n");
        tm_log_write(DTM_LIBTXN_INVALID_TXN, SQ_LOG_CRIT, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::remove_trans - %s\n", la_buf), 1);    
        abort ();
    }

    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::remove_trans (node %d, seq num %d) ENTRY\n",
                  pp_trans->getTransid()->get_node(), pp_trans->getTransid()->get_seq_num()), 2);

    TM_Native_Type lv_transid = pp_trans->getTransid()->get_native_type();
    TM_Transaction *lp_trans = NULL;
    TMLIB_EnlistedTxn_Object *lp_enlist_obj = (TMLIB_EnlistedTxn_Object *)iv_all_trans.get (lv_transid);
    if (lp_enlist_obj)
    {
        // do not delete transaction, it is not ours
        lp_trans = lp_enlist_obj->getTrans();
        iv_all_trans.remove(lv_transid);
        iv_all_tags.remove(pp_trans->getTag());
        if (pv_delete_enlisted)
            delete lp_enlist_obj;
    }
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::remove_trans EXIT\n"), 2);
    return lp_trans; 
 }

// ------------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::decrease_depth
// -- decrease depth associated with enlistment object
// -----------------------------------------------------------------------
int TMLIB_ThreadTxn_Object::decrease_current_depth ()
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (ip_enlisted_trans == NULL)
    {
        sprintf(la_buf, "Current enlisted transaction is NULL.  Trying to decrease depth.\n");
        tm_log_write(DTM_LIBTXN_INVALID_ENLISTED_TXN, SQ_LOG_WARNING, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::decrease_current_depth::remove_trans - %s\n",
                    la_buf), 1);
        abort ();
    }
   
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::decrease_current_depth\n"), 4);
    return ip_enlisted_trans->decrease_depth();
}

// ----------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::increase_depth
// -- increase depth associated with enlistment object
// ----------------------------------------------------------------------
int TMLIB_ThreadTxn_Object::increase_current_depth()
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (ip_enlisted_trans == NULL)
    {
        sprintf(la_buf, "Current enlisted transaction is NULL.  Trying to increase depth.\n");
        tm_log_write(DTM_LIBTXN_INVALID_ENLISTED_TXN, SQ_LOG_CRIT, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::aincrease_current_depth : %s\n",
                    la_buf), 1);
        abort ();
    }
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::increase_current_depth\n"), 4);
    return ip_enlisted_trans->increase_depth();
}

// ----------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::set_current_propagated
// -- set the propagated flag to true on the current enlisted objected
// ----------------------------------------------------------------------
void TMLIB_ThreadTxn_Object::set_current_propagated(bool pv_propagated)
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (ip_enlisted_trans == NULL)
    {
        sprintf(la_buf, "Current enlisted transaction is NULL.  Trying to set propagated flag.\n");
        tm_log_write(DTM_LIBTXN_INVALID_ENLISTED_TXN, SQ_LOG_CRIT, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::set_curruent_propagated : %s\n",
                    la_buf), 1);
        abort ();
    }
    
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::set_current_propagated\n"), 4);
    ip_enlisted_trans->propagated_tx(pv_propagated);
}

// ----------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::get_current_propagated
// -- get the propagated flag to true on the current enlisted objected
// ----------------------------------------------------------------------
bool TMLIB_ThreadTxn_Object::get_current_propagated()
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (ip_enlisted_trans == NULL)
    {
        sprintf(la_buf, "Invalid ip enlisted transaction\n");
        tm_log_write(DTM_LIBTXN_INVALID_IP_TXN, SQ_LOG_CRIT, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::get_current_propagated - %s\n",
                      la_buf), 1);
        abort ();
    }
    
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::get_current_propagated\n"), 4);
    return ip_enlisted_trans->propagated_tx();
}

// ----------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::decrease_current_ios
// -- decrease io count on the current tx
// ----------------------------------------------------------------------
int TMLIB_ThreadTxn_Object::decrease_current_ios()
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (ip_enlisted_trans == NULL)
    {
        sprintf(la_buf, "Current enlisted transaction is NULL.  Trying to decrease IOs.\n");
        tm_log_write(DTM_LIBTXN_INVALID_ENLISTED_TXN, SQ_LOG_CRIT, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::decrease_current_ios - %s\n", la_buf), 1);
        abort ();
    }
    
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::decrease_current_ios %d outstanding_ios for ID (%d,%d).\n",
               ip_enlisted_trans->outstanding_ios(), ip_enlisted_trans->getTrans()->getTransid()->get_node(), 
               ip_enlisted_trans->getTrans()->getTransid()->get_seq_num()), 4);
    return ip_enlisted_trans->decrease_outstanding_ios();
}

// ----------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::increase_current_ios
// -- increase io count on the current tx
// ----------------------------------------------------------------------
int TMLIB_ThreadTxn_Object::increase_current_ios()
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (ip_enlisted_trans == NULL)
    {
        sprintf(la_buf, "Current enlisted transaction is NULL.  Trying to increase IOs.\n");
        tm_log_write(DTM_LIBTXN_INVALID_ENLISTED_TXN, SQ_LOG_CRIT, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::increase_current_ios - %s\n", la_buf), 1);
        abort ();
    }
    
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::increase_current_ios %d outstanding_ios for ID (%d,%d).\n",
               ip_enlisted_trans->outstanding_ios(), ip_enlisted_trans->getTrans()->getTransid()->get_node(), 
               ip_enlisted_trans->getTrans()->getTransid()->get_seq_num()), 4);
    return ip_enlisted_trans->increase_outstanding_ios();
}

int TMLIB_ThreadTxn_Object::get_current_ios()
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (ip_enlisted_trans == NULL)
    {
        sprintf(la_buf, "Current enlisted transaction is NULL.  Trying to get current IOs.\n");
        tm_log_write(DTM_LIBTXN_INVALID_ENLISTED_TXN, SQ_LOG_CRIT, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::get_current_ios - %s\n", la_buf), 1);
        abort ();
    }
    
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::get_current_ios returning %d.\n",
        ip_enlisted_trans->outstanding_ios()), 4);
    return ip_enlisted_trans->outstanding_ios();
}


// ----------------------------------------------------------------------
// set_current_suspended, get_current_suspend
// passthrough methods
// ----------------------------------------------------------------------
void TMLIB_ThreadTxn_Object::set_current_suspended(bool pv_suspended)
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (ip_enlisted_trans == NULL)
    {
        sprintf(la_buf, "Current enlisted transaction is NULL.  Trying to get set suspended flag.\n");
        tm_log_write(DTM_LIBTXN_INVALID_ENLISTED_TXN, SQ_LOG_CRIT, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::set_current_suspended - %s\n", la_buf), 1);
        abort ();
    }
    
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::set_current_suspended\n"), 4);
    ip_enlisted_trans->suspended_tx(pv_suspended);
}

bool TMLIB_ThreadTxn_Object::get_current_suspended()
{
    char la_buf[DTM_STRING_BUF_SIZE];
    if (ip_enlisted_trans == NULL)
    {
        sprintf(la_buf, "Current enlisted transaction is NULL.  Trying to get get suspended flag.\n");
        tm_log_write(DTM_LIBTXN_INVALID_ENLISTED_TXN, SQ_LOG_CRIT, la_buf);
        TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::get_current_suspended - %s\n", la_buf), 1);
        abort ();
    }
    
    TMlibTrace(("TMLIB_TRACE : TMLIB_ThreadTxn_Object::get_current_suspended\n"), 4);
    return ip_enlisted_trans->suspended_tx();
}

// -------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::set_current_
// -- set this transaction as the current tx.  If it isn't already on
//    the enlistment list, we cannot set it as this tx has never
//    existed in this thread before.
// --------------------------------------------------------------------
void TMLIB_ThreadTxn_Object::set_current(TM_Transaction *pp_trans)
{
    TMlibTrace (("TMlibTrace : TMLIB_ThreadTxn_Object::set_current ENTRY\n"), 2);

    if (pp_trans == NULL)
        ip_enlisted_trans = NULL;
    else
        ip_enlisted_trans = get_enlisted(pp_trans->getTransid()->get_native_type());

   if (ip_enlisted_trans)
   {
      TMlibTrace (("TMLIB_TRACE : TMLIB_ThreadTxn_Object::set_current, ID (%d,%d) (EXIT)\n",
                    pp_trans->getTransid()->get_node(), pp_trans->getTransid()->get_seq_num()), 2);
   }
   else
   {
      TMlibTrace (("TMLIB_TRACE : TMLIB_ThreadTxn_Object::set_current, enlisted == NULL (EXIT)\n"), 2);
   }
}

// -------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::set_startid
// -- set the startid in the current enlisted object.
// ------------------------------------------------------------------
void TMLIB_ThreadTxn_Object::set_startid(long pv_startid)
{
    TMlibTrace (("TMLIB_TRACE : TMLIB_ThreadTxn_Object::set_startid ENTRY with startid:%ld \n", pv_startid), 2);
    if (ip_enlisted_trans)
    {
        ip_enlisted_trans->set_startid(pv_startid);
    }
    TMlibTrace (("TMLIB_TRACE : TMLIB_ThreadTxn_Object::set_startid EXIT\n"), 2);
}

// -------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::get_startid
// -- get the startid in the current enlisted object.
// ------------------------------------------------------------------
TM_Transseq_Type TMLIB_ThreadTxn_Object::get_startid()
{
    TM_Transseq_Type lv_startid = -1;
    TMlibTrace (("TMLIB_TRACE : TMLIB_ThreadTxn_Object::get_startid ENTRY \n"), 2);
    if (ip_enlisted_trans)
    {
        lv_startid = ip_enlisted_trans->get_startid();
    }
    TMlibTrace (("TMLIB_TRACE : TMLIB_ThreadTxn_Object::get_startid EXIT returning startid:%ld\n", lv_startid), 2);
    return lv_startid;
}

// -------------------------------------------------------------------
// TMLIB_ThreadTxn_Object::delete_current
// -- detele the current enlisted object.  This method is only called 
//    internally and we are responsible for deleting everything.
// ------------------------------------------------------------------
void TMLIB_ThreadTxn_Object::delete_current()
{
    TMlibTrace (("TMLIB_TRACE : TMLIB_ThreadTxn_Object::delete_current ENTRY \n"), 2);
    if (ip_enlisted_trans)
    {
        TM_Transaction *lp_trans = ip_enlisted_trans->getTrans();
        if (lp_trans)
        {
             remove_trans(lp_trans, false); // do not delete trans         
             delete lp_trans;
        }
        delete ip_enlisted_trans;
        set_startid(-1);
        ip_enlisted_trans = NULL;
    }
    TMlibTrace (("TMLIB_TRACE : TMLIB_ThreadTxn_Object::delete_current EXIT\n"), 2);
}

// ------------------------------------------------------------------
// TMLIB_EnlistedTxn_Object::get_enlisted
// -- return current enlisted object
// -----------------------------------------------------------------
TMLIB_EnlistedTxn_Object *TMLIB_ThreadTxn_Object::get_enlisted(TM_Native_Type pv_transid)
{
    return (TMLIB_EnlistedTxn_Object *)iv_all_trans.get (pv_transid);
}

