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

// dtm includes
#include "tmlib.h"
#include "tmlogging.h"
#include "tmtxkey.h"

// seabed includes
#include "seabed/trace.h"

extern TMLIB gv_tmlib;


// --------------------------------------------------------------------------
// TM_Transaction::TM_Transaction
// -- constructor with implicit begin
// Input parameters:
//    abort_timeout - > 0 = Timeout in seconds until transaction will be aborted.
//                    -1 = wait forever
//                    0  = system default.
//    transactiontype_bits - Transaction Type bits. TBD.
// --------------------------------------------------------------------------
TM_Transaction::TM_Transaction(int abort_timeout, int64 transactiontype_bits)
{
    TMlibTrace(("TMLIB_TRACE : TM_Transaction::Constructor1 ENTRY\n"), 4);
    iv_coordinator = true;
    iv_last_error = 0;
    iv_server = false;
    iv_abort_timeout = abort_timeout;
    iv_transactiontype = transactiontype_bits;
    begin (abort_timeout, transactiontype_bits);
}

// --------------------------------------------------------------------------
// TM_Transaction::TM_Transaction
// -- constructor with implicit join
// --------------------------------------------------------------------------
TM_Transaction::TM_Transaction(TM_Transid pv_transid, bool pv_server_tx)
{
    TMlibTrace(("TMLIB_TRACE : TM_Transaction::Constructor2 ENTRY. transid "
        "(%d,%d), server flag %d.\n", pv_transid.get_node(), pv_transid.get_seq_num(), pv_server_tx), 4);
    iv_transid = pv_transid;
    iv_coordinator = false;
    iv_server = pv_server_tx;
    iv_last_error = 0;
    iv_abort_timeout = 0;
    iv_transactiontype = 0;
    join (iv_coordinator);
}

// --------------------------------------------------------------------------
// TM_Transaction::TM_Transaction
// -- Default constructor with no join or begin
// --------------------------------------------------------------------------
TM_Transaction::TM_Transaction() 
   :iv_server(false), iv_transid(0)
{
    TMlibTrace(("TMLIB_TRACE : TM_Transaction::Constructor3 Default ENTRY.\n"), 4);
    iv_coordinator = false;
    iv_last_error = 0;
    iv_abort_timeout = 0;
    iv_transactiontype = 0;
}

// --------------------------------------------------------------------------
// TM_Transaction::~TM_Transaction
// -- destructor 
// --------------------------------------------------------------------------
TM_Transaction::~TM_Transaction()
{
    TMlibTrace(("TMLIB_TRACE : TM_Transaction::Destructor ENTRY. transid "
        "(%d,%d).\n", iv_transid.get_node(), iv_transid.get_seq_num()), 4);
}

// TOPL
short  TM_Transaction::register_region(long startid, int port, char *hostName, int hostname_Length, long startcode, char *regionInfo, int regionInfo_Length)
{
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;
    //printf ("TM_Transaction::register_region ENTRY with startid %ld \n", startid);


    TMlibTrace(("TMLIB_TRACE : TM_Transaction::register_region ENTRY\n"), 2);

    if (!gv_tmlib_initialized)
         gv_tmlib.initialize();

    tmlib_init_req_hdr(TM_MSG_TYPE_REGISTERREGION, &lv_req);
    lv_req.u.iv_register_region.iv_startid = startid;
    lv_req.u.iv_register_region.iv_pid = gv_tmlib.iv_my_pid;
    lv_req.u.iv_register_region.iv_nid = gv_tmlib.iv_my_nid;
    iv_transid.set_external_data_type(&lv_req.u.iv_register_region.iv_transid);
    lv_req.u.iv_register_region.iv_port = port;
    memcpy (lv_req.u.iv_register_region.ia_hostname, hostName, hostname_Length);
    lv_req.u.iv_register_region.iv_hostname_length = hostname_Length;
    lv_req.u.iv_register_region.iv_startcode = startcode;

    memcpy (lv_req.u.iv_register_region.ia_regioninfo2, regionInfo, regionInfo_Length);
    lv_req.u.iv_register_region.iv_regioninfo_length = regionInfo_Length;

    iv_last_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, iv_transid.get_node());
    if  (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::register_region returning error %d\n", iv_last_error), 1);
        return iv_last_error;
    }

    // record tag in our param, if an error occurred, this
    // will be -1, so we can do a blind copy here
    iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::register_region EXIT with error %d\n", iv_last_error), 2);

    return iv_last_error; 

}

short TM_Transaction::create_table(char* pa_tbldesc, int pv_tbldesc_len,
                                   char* pa_tblname, char** pa_keys,
                                   int pv_numsplits, int pv_keylen,
                                   char* &pv_err_str, int &pv_err_len)
{
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;
    
    int len = sizeof(Tm_Req_Msg_Type);
    int len_aligned = 8*(((len + 7)/8));
    int buffer_size = pv_numsplits*pv_keylen;
    int total_buffer = len_aligned + buffer_size;
    char *buffer = new char[total_buffer];

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::create_table ENTRY tablename: %s\n", pa_tblname), 1);

    if (!gv_tmlib_initialized)
         gv_tmlib.initialize();

    tmlib_init_req_hdr(TM_MSG_TYPE_DDLREQUEST, &lv_req);
    iv_transid.set_external_data_type(&lv_req.u.iv_ddl_request.iv_transid);
    memcpy(lv_req.u.iv_ddl_request.ddlreq, pa_tbldesc, pv_tbldesc_len);
    lv_req.u.iv_ddl_request.ddlreq_len = pv_tbldesc_len;
    lv_req.u.iv_ddl_request.crt_numsplits = pv_numsplits;
    lv_req.u.iv_ddl_request.crt_keylen = pv_keylen;
    lv_req.u.iv_ddl_request.ddlreq_type = TM_DDL_CREATE;
    memcpy(buffer, (char*)&lv_req, len);

    int i;
    int index = len_aligned;
    for(i=0; i<pv_numsplits; i++){
       memcpy((buffer+index), pa_keys[i], pv_keylen);
       index = index + pv_keylen;
    }
  
    iv_last_error = gv_tmlib.send_tm_link(buffer, total_buffer, &lv_rsp, iv_transid.get_node());
    delete buffer;
    if(iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::create_table returning error %d\n", iv_last_error), 1);
        return iv_last_error;
    }
    
    iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;
    if(iv_last_error)
    {
        int maxErrStrBufLen = sizeof(lv_rsp.u.iv_ddl_response.iv_err_str);
        pv_err_len = lv_rsp.u.iv_ddl_response.iv_err_str_len < maxErrStrBufLen ? lv_rsp.u.iv_ddl_response.iv_err_str_len : maxErrStrBufLen;
    	pv_err_str = new char[pv_err_len];
    	memcpy(pv_err_str, lv_rsp.u.iv_ddl_response.iv_err_str, pv_err_len); 
    }
    
    return iv_last_error;
}

short TM_Transaction::alter_table(char * pa_tblname, int pv_tblname_len,
    char ** pv_tbloptions,  int pv_tbloptslen, int pv_tbloptscnt,
    char* &pv_err_str, int &pv_err_len)
{    
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;
 
    TMlibTrace(("TMLIB_TRACE : TM_Transaction::alter_table ENTRY tablename: %s\n", pa_tblname), 1);

    int len = sizeof(Tm_Req_Msg_Type);
    int len_aligned = 8*(((len + 7)/8));
    int buffer_size = pv_tbloptslen*pv_tbloptscnt;
    int total_buffer = len_aligned + buffer_size;
    char *buffer = new char[total_buffer];

    if (!gv_tmlib_initialized)
         gv_tmlib.initialize();

    tmlib_init_req_hdr(TM_MSG_TYPE_DDLREQUEST, &lv_req);
    iv_transid.set_external_data_type(&lv_req.u.iv_ddl_request.iv_transid);
    memcpy(lv_req.u.iv_ddl_request.ddlreq, pa_tblname, pv_tblname_len);
    lv_req.u.iv_ddl_request.ddlreq_len = pv_tblname_len;
    lv_req.u.iv_ddl_request.alt_numopts = pv_tbloptscnt;
    lv_req.u.iv_ddl_request.alt_optslen = pv_tbloptslen;
    lv_req.u.iv_ddl_request.ddlreq_type = TM_DDL_ALTER;
    memcpy(buffer, (char*)&lv_req, len);

    int i;
    int index = len_aligned;
    for(i=0; i<pv_tbloptscnt; i++){
       memcpy((buffer+index), pv_tbloptions[i], pv_tbloptslen);
       index = index + pv_tbloptslen;
    }

    short lv_last_error = gv_tmlib.send_tm_link(buffer, total_buffer, &lv_rsp, iv_transid.get_node());
    delete buffer;
    if(lv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::alter_table returning error %d\n", iv_last_error), 1);
        return lv_last_error;
    }

    iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;
    if(iv_last_error)
    {
        int maxErrStrBufLen = sizeof(lv_rsp.u.iv_ddl_response.iv_err_str);
        pv_err_len = lv_rsp.u.iv_ddl_response.iv_err_str_len < maxErrStrBufLen ? lv_rsp.u.iv_ddl_response.iv_err_str_len : maxErrStrBufLen;
        pv_err_str = new char[pv_err_len];
        memcpy(pv_err_str, lv_rsp.u.iv_ddl_response.iv_err_str, pv_err_len); 
    }
    
    return iv_last_error;
}

short TM_Transaction::reg_truncateonabort(char* pa_tblname, int pv_tblname_len,
                                          char* &pv_err_str, int &pv_err_len)
{
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::reg_truncateonabort ENTRY tablename: %s\n", pa_tblname), 1);

    if (!gv_tmlib_initialized)
         gv_tmlib.initialize();

    tmlib_init_req_hdr(TM_MSG_TYPE_DDLREQUEST, &lv_req);
    iv_transid.set_external_data_type(&lv_req.u.iv_ddl_request.iv_transid);
    memcpy(lv_req.u.iv_ddl_request.ddlreq, pa_tblname, pv_tblname_len);
    lv_req.u.iv_ddl_request.ddlreq_len = pv_tblname_len;
    lv_req.u.iv_ddl_request.ddlreq_type = TM_DDL_TRUNCATE;

    iv_last_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, iv_transid.get_node());
    if(iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::reg_truncateonabort returning error %d\n", iv_last_error), 1);
        return iv_last_error;
    }
    iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;
    if(iv_last_error)
    {
        int maxErrStrBufLen = sizeof(lv_rsp.u.iv_ddl_response.iv_err_str);
        pv_err_len = lv_rsp.u.iv_ddl_response.iv_err_str_len < maxErrStrBufLen ? lv_rsp.u.iv_ddl_response.iv_err_str_len : maxErrStrBufLen;
        pv_err_str = new char[pv_err_len];
        memcpy(pv_err_str, lv_rsp.u.iv_ddl_response.iv_err_str, pv_err_len); 
    }    
    return iv_last_error;
}

short TM_Transaction::drop_table(char* pa_tblname, int pv_tblname_len,
                                 char* &pv_err_str, int &pv_err_len)
{
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::drop_table ENTRY tablename: %s\n", pa_tblname), 1);

    if (!gv_tmlib_initialized)
         gv_tmlib.initialize();

    tmlib_init_req_hdr(TM_MSG_TYPE_DDLREQUEST, &lv_req);
    iv_transid.set_external_data_type(&lv_req.u.iv_ddl_request.iv_transid);
    memcpy(lv_req.u.iv_ddl_request.ddlreq, pa_tblname, pv_tblname_len);
    lv_req.u.iv_ddl_request.ddlreq_len = pv_tblname_len;
    lv_req.u.iv_ddl_request.ddlreq_type = TM_DDL_DROP;

    iv_last_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, iv_transid.get_node());
    if(iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::drop_table returning error %d\n", iv_last_error), 1);
        return iv_last_error;
    }
    iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;
    if(iv_last_error)
    {
        int maxErrStrBufLen = sizeof(lv_rsp.u.iv_ddl_response.iv_err_str);
        pv_err_len = lv_rsp.u.iv_ddl_response.iv_err_str_len < maxErrStrBufLen ? lv_rsp.u.iv_ddl_response.iv_err_str_len : maxErrStrBufLen;
        pv_err_str = new char[pv_err_len];
        memcpy(pv_err_str, lv_rsp.u.iv_ddl_response.iv_err_str, pv_err_len); 
    }    
    return iv_last_error;
}

// --------------------------------------------------------------------------
// TM_Transaction::begin
// -- begin transaction, private method
// Added local transaction support for Trafodion
// --------------------------------------------------------------------------
short TM_Transaction::begin(int abort_timeout, int64 transactiontype_bits)
{
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::begin ENTRY\n"), 2);
 
    // Validate abort_timeout
    if (abort_timeout != -1 && abort_timeout < 0)
    {
       TMlibTrace(("TMLIB_TRACE : TM_Transaction::begin : abort_timeout not -1 or > 0\n"), 1);
       iv_last_error = FEBOUNDSERR;
       return iv_last_error;
    }

    // We don't validate transactiontype flags

    if (!gv_tmlib_initialized)
         gv_tmlib.initialize();

    // if there is already an active tx, switch it out
    // allows user to do multiple BEGINS
    if (gp_trans_thr->get_current() != NULL)
        gp_trans_thr->get_current()->release();
    
    // Bypass BEGINTRANSACTION call for Trafodion local transactions.
    if (gv_tmlib.localBegin()) {
       CTmTxKey *lp_txid = new CTmTxKey(gv_tmlib.iv_my_nid, gv_tmlib.seqNum()->nextSeqNum());
       iv_transid = lp_txid->id();
       iv_tag = lp_txid->seqnum();
       gp_trans_thr->add_trans(this);
       delete lp_txid;
       //printf("Local begin bypassing begin txn (%d,%d)\n", lp_txid->node(), lp_txid->seqnum());
    }
    else { //Begin in TM
       tmlib_init_req_hdr(TM_MSG_TYPE_BEGINTRANSACTION, &lv_req);
       lv_req.u.iv_begin_trans.iv_pid = gv_tmlib.iv_my_pid;
       lv_req.u.iv_begin_trans.iv_nid = gv_tmlib.iv_my_nid;
       lv_req.u.iv_begin_trans.iv_abort_timeout = abort_timeout;
       lv_req.u.iv_begin_trans.iv_transactiontype_bits = transactiontype_bits;

       iv_last_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, gv_tmlib.beginner_nid());
       if  (iv_last_error)
       {
           TMlibTrace(("TMLIB_TRACE : TM_Transaction::begin returning error %d\n", iv_last_error), 1);
           return iv_last_error;
       }

       // record tag in our param, if an error occurred, this
       // will be -1, so we can do a blind copy here
       iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;
       
       if (!iv_last_error)
       {    
           // record as the active tx
           iv_transid = lv_rsp.u.iv_begin_trans.iv_transid;  
           iv_tag = gv_tmlib.new_tag();
           gp_trans_thr->add_trans(this);
       }
    }

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::begin (seq num %d, tag %d) EXIT\n", 
        iv_transid.get_seq_num(), iv_tag), 2);

    return iv_last_error; 
}


// --------------------------------------------------------------------------
// TM_Transaction::end
// -- end transaction
// Trafodion: added support for local transactions.
// --------------------------------------------------------------------------
short TM_Transaction::end(char* &pv_err_str, int &pv_err_len)
{
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::end (seq num %d) ENTRY\n", iv_transid.get_seq_num()), 2);

    iv_last_error = tmlib_check_active_tx();
    if (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::end returning error %d \n", iv_last_error), 1);
        return iv_last_error;    
    }

    iv_last_error = tmlib_check_outstanding_ios();
    if (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::end returning error %d \n", iv_last_error), 1);
        return iv_last_error;    
    } 

    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    if (gv_tmlib.localBegin()) {
       //printf("Local begin ENDTRANSACTION %d\n", iv_transid.get_seq_num());
       gv_tmlib.initJNI();
       short lv_HBerror = gv_tmlib.endTransactionLocal((int64)iv_transid.get_native_type());
       iv_last_error = HBasetoTxnError(lv_HBerror);
       if (iv_last_error)
       {
           printf("TM_Transaction::end returning error (%d,%d)\n",iv_last_error, lv_HBerror);
           TMlibTrace(("TMLIB_TRACE : TM_Transaction::end local txn returning error (%d, %d).\n", iv_last_error, lv_HBerror), 1);
           return iv_last_error;    
       }
    }
    else {
       tmlib_init_req_hdr(TM_MSG_TYPE_ENDTRANSACTION, &lv_req);
       lv_req.u.iv_end_trans.iv_tag = iv_transid.get_seq_num();
       lv_req.u.iv_end_trans.iv_pid = gv_tmlib.iv_my_pid;
       lv_req.u.iv_end_trans.iv_nid = gv_tmlib.iv_my_nid;
       iv_transid.set_external_data_type(&lv_req.u.iv_end_trans.iv_transid);

       iv_last_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, iv_transid.get_node());
       
       if (iv_last_error)
       {
           TMlibTrace(("TMLIB_TRACE : TM_Transaction::end returning error %d \n", iv_last_error), 1);
           return iv_last_error;    
       }

       // so far, FEJOINSOUSTANDING is the only retryable error
       // We want to clean up for things like FEOK, FENOTRANSID, FEINVTRANSID
       // if its FEDEVICEDOWNFORTMF, then it will get aborted as that means the
       // owner TM is down.
       // FEABORTEDTRANSID & FEENDEDTRANSID is ok, 
        if  ((lv_rsp.iv_msg_hdr.miv_err.error == FEINVTRANSID) ||
            (lv_rsp.iv_msg_hdr.miv_err.error == FENOTRANSID) ||
            (lv_rsp.iv_msg_hdr.miv_err.error == FEOK)  ||
            (lv_rsp.iv_msg_hdr.miv_err.error == FEABORTEDTRANSID) ||
            (lv_rsp.iv_msg_hdr.miv_err.error == FEENDEDTRANSID))
        {
           gv_tmlib.clear_entry (iv_transid , false, true);
           gp_trans_thr->remove_trans (this);
       }
    
       iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;
       if(iv_last_error)
       {
           int maxErrStrBufLen = sizeof(lv_rsp.u.iv_end_trans.iv_err_str);
           pv_err_len = lv_rsp.u.iv_end_trans.iv_err_str_len < maxErrStrBufLen ? lv_rsp.u.iv_end_trans.iv_err_str_len : maxErrStrBufLen;
           pv_err_str = new char[pv_err_len];
           memcpy(pv_err_str, lv_rsp.u.iv_end_trans.iv_err_str, pv_err_len); 
       }
    }
    TMlibTrace(("TMLIB_TRACE : TM_Transaction::end  (seq num %d) EXIT\n", iv_transid.get_seq_num()), 2);

    return iv_last_error;
}


// --------------------------------------------------------------------------
// TM_Transaction::abort
// -- abort transaction
// --------------------------------------------------------------------------
short TM_Transaction::abort(bool pv_doom)
{
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::abort (seq num %d) ENTRY\n", iv_transid.get_seq_num()), 2);

    iv_last_error = tmlib_check_active_tx();
    if (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::abort returning error %d \n", iv_last_error), 1);
        return iv_last_error;    
    }

   // SQL needs to be
   // able to call ABORTTRANSACTION while having outstanding IOs.
   /*
    iv_last_error = tmlib_check_outstanding_ios();
    if (iv_last_error)
        return iv_last_error;   
   */

    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    if (gv_tmlib.localBegin()) {
       //printf("Local begin ABORTTRANSACTION %d\n", iv_transid.get_seq_num());
       gv_tmlib.initJNI();
       short lv_HBerror = gv_tmlib.abortTransactionLocal((int64) iv_transid.get_native_type());
       iv_last_error = HBasetoTxnError(lv_HBerror);
       if (iv_last_error)
       {
           TMlibTrace(("TMLIB_TRACE : TM_Transaction::abort local txn returning error (%d, %d).\n", iv_last_error, lv_HBerror), 1);
           return iv_last_error;    
       }
    }
    else {
       if (pv_doom)
           tmlib_init_req_hdr(TM_MSG_TYPE_DOOMTX, &lv_req);
       else
           tmlib_init_req_hdr(TM_MSG_TYPE_ABORTTRANSACTION, &lv_req);

       lv_req.u.iv_abort_trans.iv_tag =  iv_transid.get_seq_num();
       lv_req.u.iv_abort_trans.iv_nid = gv_tmlib.iv_my_nid;
       lv_req.u.iv_abort_trans.iv_pid = gv_tmlib.iv_my_pid;
       iv_transid.set_external_data_type (&lv_req.u.iv_abort_trans.iv_transid);
 
       iv_last_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, iv_transid.get_node());
       if (iv_last_error)
       {
           TMlibTrace(("TMLIB_TRACE : TM_Transaction::abort returning error %d \n", iv_last_error), 1);
           return iv_last_error;    
       }

       // so far, FEJOINSOUSTANDING is the only retryable error
       // We want to clean up for things like FEOK, FENOTRANSID, FEINVTRANSID
       // if its FEDEVICEDOWNFORTMF, then it will get aborted as that means the
       // owner TM is down.
       // FEABORTEDTRANSID & FEENDEDTRANSID is ok, 
       if  ((lv_rsp.iv_msg_hdr.miv_err.error == FEINVTRANSID) ||
           (lv_rsp.iv_msg_hdr.miv_err.error == FENOTRANSID) ||
           (lv_rsp.iv_msg_hdr.miv_err.error == FEOK) ||
           (lv_rsp.iv_msg_hdr.miv_err.error == FEABORTEDTRANSID) ||
           (lv_rsp.iv_msg_hdr.miv_err.error == FEENDEDTRANSID))
       {
           if (!pv_doom)
           {
               gv_tmlib.clear_entry(iv_transid, false, true);
               gp_trans_thr->remove_trans (this);
           }
       }

       iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;
    }
    TMlibTrace(("TMLIB_TRACE : TM_Transaction::abort (seq num %d) EXIT\n", iv_transid.get_seq_num()), 2);
    return iv_last_error;
}

// --------------------------------------------------------------------------
// TM_Transaction::join
// -- join an existing transaction, private method 
// --------------------------------------------------------------------------
short TM_Transaction::join(bool pv_coordinator_role)
{
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::join ENTRY\n"), 2);

    if (!gv_tmlib_initialized)
        gv_tmlib.initialize();

    // if there is already an active tx, switch it out
    // allows user to do multiple JOINS
    if (gp_trans_thr->get_current() != NULL)
    {
        if (gp_trans_thr->get_current()->equal(iv_transid))
        {
             iv_last_error = FEALREADYJOINED;
             TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION_EXT returning error %d for seq num %d\n", 
                          iv_last_error, iv_transid.get_seq_num()), 1);
             return iv_last_error;
        }
        TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION_EXT called with active tx\n"), 3);
        RESUMETRANSACTION(0);
    }

    // no joins or enlists
    if (iv_server)
    {
         iv_last_error = FEOK;
         iv_tag = gv_tmlib.new_tag();
         gp_trans_thr->add_trans(this);
         TMlibTrace(("TMLIB_TRACE : TM_Transaction::join, server EXIT\n"), 2);
         return iv_last_error;
    }

    // do a join
    tmlib_init_req_hdr(TM_MSG_TYPE_JOINTRANSACTION, &lv_req);
    iv_transid.set_external_data_type(&lv_req.u.iv_join_trans.iv_transid);
    lv_req.u.iv_join_trans.iv_coord_role = pv_coordinator_role;
    lv_req.u.iv_join_trans.iv_pid = gv_tmlib.iv_my_pid;
    lv_req.u.iv_join_trans.iv_nid = gv_tmlib.iv_my_nid;
    iv_last_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, iv_transid.get_node());
    if (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : JOINTRANSACTION_EXT returning error %d for seq num %d\n", 
                     iv_last_error, iv_transid.get_seq_num()), 1);
        return iv_last_error;
    }

    iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;

    if (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : Join returning error %d for txn seq num %d\n",
                       lv_rsp.iv_msg_hdr.miv_err.error, iv_transid.get_seq_num()), 1);
        return iv_last_error;
    }

    iv_coordinator = pv_coordinator_role;

    iv_tag = gv_tmlib.new_tag();
    gp_trans_thr->add_trans(this);
    if (!gv_tmlib.is_enlisted (iv_transid.get_node()))
    {
        TMlibTrace(("TMLIB_TRACE : TM_transaction::join, adding enlist for node %d\n", iv_transid.get_node()), 3);
        TM_Transid_Type lv_null_transid;
        lv_null_transid.id[0] = lv_null_transid.id[1] = lv_null_transid.id[2] =
                                lv_null_transid.id[3] = 0;
        msg_mon_trans_enlist (iv_transid.get_node(), lv_rsp.u.iv_join_trans.iv_tm_pid,
                               lv_null_transid);
        gv_tmlib.enlist(iv_transid.get_node());
    }


    iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;
    TMlibTrace(("TMLIB_TRACE : TM_transaction::join (seq num %d) EXIT\n", iv_transid.get_seq_num()), 2);

    return iv_last_error;
}

// --------------------------------------------------------------------------
// TM_Transaction::suspend
// -- suspend the transaction, will remove reference to it in the library
// --------------------------------------------------------------------------
short TM_Transaction::suspend(TM_Transid *pp_transid, bool pv_coordinator_role)
{
     char la_buf[DTM_STRING_BUF_SIZE];
     TM_Transid lp_ext_transid;
     short lv_error = FEOK;
   
     TMlibTrace(("TMLIB_TRACE : TM_Transaction::suspend ENTRY \n"), 2);

    iv_last_error = tmlib_check_miss_param(pp_transid);
    if (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::suspend returning error %d for seq num %d \n",
                     iv_last_error, iv_transid.get_seq_num()), 1);
        return iv_last_error;
    }

    iv_last_error = tmlib_check_active_tx();
    if (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::suspend returning error %d for seq num %d \n",
                     iv_last_error, iv_transid.get_seq_num()), 1);
        return iv_last_error;
    }

    iv_last_error = tmlib_check_outstanding_ios();
    if (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::suspend returning error %d for seq num %d (IGNORED)\n",
                     iv_last_error, iv_transid.get_seq_num()), 1);
        iv_last_error = FEOK; //12/1/10 Temp change to ignore error 81.
        //return iv_last_error;
    }

    if (!gv_tmlib_initialized)
    {
        gv_tmlib.initialize();
    }

     // they did not join and hence cannot suspend
     if (gp_trans_thr->get_current_propagated() == true)
     {
        iv_last_error = FETXSUSPENDREJECTED;
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::suspend returning error %d for seq num %d \n",
                     iv_last_error, iv_transid.get_seq_num()), 1);
        return iv_last_error; 
     }
     memcpy (pp_transid, iv_transid.get_data_address(), sizeof (TM_Transid_Type));
     
     if (pv_coordinator_role == false)
     {
        if (!isEnder())
        {
            sprintf(la_buf, "Transaction with coordinator role of false is marked as the coordinator.\n");
            tm_log_write(DTM_TRANS_ENDER, SQ_LOG_CRIT, la_buf);
            TMlibTrace(("TMLIB_TRACE : TM_Transaction::suspend : %s, seq num %d\n", 
                         la_buf, iv_transid.get_seq_num()), 1);
            abort();
        }        

        iv_coordinator = pv_coordinator_role;  // TODO - errors in suspend
     }

     if (isEnder())
     {
         gp_trans_thr->set_current_suspended(true);
         gv_tmlib.clear_entry (iv_transid, false,  true);
     }
     else 
     {
         lv_error = tmlib_send_suspend(iv_transid, pv_coordinator_role,gv_tmlib.iv_my_pid);
         if (gp_trans_thr->get_current_ios() == 0)
         {
            gv_tmlib.clear_entry (iv_transid, false,  true);
            gp_trans_thr->remove_trans (this);
         }
         else
         {
            TMlibTrace(("TMLIB_TRACE : TM_Transaction::suspend : seq num %d not removing txn "
                "because there are still %d outstanding ios.\n", 
                iv_transid.get_seq_num(), gp_trans_thr->get_current_ios()), 3);
         }
         iv_last_error = lv_error;
     }

     TMlibTrace(("TMLIB_TRACE : TM_Transaction::suspend (seq num %d) returning %dEXIT\n", 
         iv_transid.get_seq_num(), iv_last_error), 2);
     return iv_last_error;  
}

// --------------------------------------------------------------------------
// TM_Transaction::resume
// -- resume previosly joined or begun transaction
// --------------------------------------------------------------------------
short TM_Transaction::resume()
{
    char la_buf[DTM_STRING_BUF_SIZE];
    TMlibTrace(("TMLIB_TRACE : TM_Transaction::resume ENTRY\n"), 2);
  
    TM_Transaction *lp_trans = gp_trans_thr->get_trans (iv_transid.get_native_type());
    if (lp_trans == NULL)
    {
        sprintf(la_buf, "Unable to find transaction to resume.\n");
        tm_log_write(DTM_TRANS_BAD_TRANS_SEQ, SQ_LOG_CRIT, la_buf); 
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::resume - %s \n", la_buf), 1);
        abort();
    }    

    gp_trans_thr->set_current (lp_trans);
    iv_last_error = FEOK;
    TMlibTrace(("TMLIB_TRACE : TM_Transaction::resume EXIT\n"), 2);

    return iv_last_error;
}

// --------------------------------------------------------------------------
// TM_Transaction::release
// -- release transaction (wont delete, it will just not be current anymore)
// --------------------------------------------------------------------------
TM_Transaction* TM_Transaction::release()
{
    TMlibTrace(("TMLIB_TRACE : TM_Transaction::release ENTRY\n"), 2);

    gp_trans_thr->set_current (NULL);
    iv_last_error = FEOK;

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::release EXIT\n"), 2);
    return this;
}

// --------------------------------------------------------------------------
// TM_Transaction::status
// -- return status of transaction
// --------------------------------------------------------------------------
short TM_Transaction::status(short *pp_status)
{
    TM_Transaction *lp_trans = NULL;
    Tm_Req_Msg_Type lv_req;
    Tm_Rsp_Msg_Type lv_rsp;

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::status ENTRY\n"), 2);

    iv_last_error = tmlib_check_miss_param (pp_status);
    if (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::status returning error %d \n",
                     iv_last_error), 1);
        return iv_last_error;
    }

    iv_last_error = tmlib_check_active_tx();
    if (iv_last_error)
    {
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::status returning error %d \n",
                     iv_last_error), 1);
        return iv_last_error;
    }

    if (!gv_tmlib_initialized)
    {
        gv_tmlib.initialize();
    }

    // if the use passed in a transid, we must use that and make sure
    // we go to the correct DTM
    lp_trans = gp_trans_thr->get_trans (iv_transid.get_native_type());

    if (lp_trans == NULL)
    {
        iv_last_error = FEINVTRANSID;
        TMlibTrace(("TMLIB_TRACE : TM_Transaction::status returning error %d\n", iv_last_error), 1);
        return iv_last_error;
    } 

    tmlib_init_req_hdr(TM_MSG_TYPE_STATUSTRANSACTION, &lv_req);
    iv_transid.set_external_data_type(&lv_req.u.iv_status_trans.iv_transid);
    iv_last_error = gv_tmlib.send_tm(&lv_req, &lv_rsp, iv_transid.get_node());
    if (iv_last_error)
        return iv_last_error;

    *pp_status = lv_rsp.u.iv_status_trans.iv_status;
    iv_last_error = lv_rsp.iv_msg_hdr.miv_err.error;

    TMlibTrace(("TMLIB_TRACE : TM_Transaction::status EXIT\n"), 2);

    return iv_last_error;
}

// --------------------------------------------------------------------------
// TM_Transaction::getTransid
// -- Return transid
// --------------------------------------------------------------------------
TM_Transid * TM_Transaction::getTransid()
{
   TMlibTrace(("TMLIB_TRACE : GETTRANSID_EXT ENTRY/EXIT\n"), 2);
   iv_last_error = FEOK;
   return &iv_transid;
}

// --------------------------------------------------------------------------
// TM_Transaction::getCurrent
// Return a pointer to the current transaction object (not specifically this 
// transaction!).
// --------------------------------------------------------------------------
TM_Transaction * TM_Transaction::getCurrent()
{
    return gp_trans_thr->get_current(); 
}

// --------------------------------------------------------------------------
// TM_Transaction::getTypeFlags
// Returns the transaction type flags for this transaction.
// --------------------------------------------------------------------------
int64 TM_Transaction::getTypeFlags()
{
   return iv_transid.get_type_flags();
}
