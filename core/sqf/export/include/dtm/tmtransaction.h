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
#ifndef TMTRANSACTION_H_
#define TMTRANSACTION_H_

#include "dtm/tmtransid.h"
#include "dtm/tm_util.h"

#define TMLIB_MAX_THREADS 256
#define TMLIB_MAX_TRANS_PER_THREAD 1024

// -------------------------------------------------------------------
// TM_Transaction class
// -- This object will manage transaction
//    There can be at most 1 per active transaction in a given thread
//    Transaction objects are created:
//         1)explicitly to begin a transaction
//         2) implicitly in a propagated tx
//         3) explicitly for a join
// -------------------------------------------------------------------
class TM_Transaction
{
public:
        // begin a transaction
    TM_Transaction(int abort_timeout, int64 transactiontype_bits);
        // join a transaction
    TM_Transaction(TM_Transid transid, bool server_tx);
    TM_Transaction();
    ~TM_Transaction();

        // transaction actions
    short end();
    short abort(bool pv_doom = false);
    short suspend(TM_Transid *transid, bool coordinator_role=true);
    short resume();
    short register_region(long startid, int port, char *hostname, int hostname_length, long startcode, char *regionInfo, int regionInfoLength); //TOPL
    short create_table(char* pa_tbldesc, int pv_tbldesc_len, char* pa_tblname, char** pv_keys, int pv_numsplits, int pv_keylen,
                      char* &pv_err_str, int &pv_err_len);
    short reg_truncateonabort(char* pa_tblname, int pv_tblname_len, 
                              char* &pv_err_str, int &pv_err_len);
    short drop_table(char* pa_tblname, int pv_tblname_len, char* &pv_err_str,
                      int &pv_err_len);
    short alter_table(char * pa_tblname, int pv_tblname_len, char ** pv_tbloptions,
                      int pv_tbloptslen, int pv_tbloptscnt, char* &pv_err_str,
                      int &pv_err_len);
    TM_Transaction *release();
    short status(short *status);
    TM_Transid * getTransid();
    void setTransid(TM_Transid pv_transid) {iv_transid = pv_transid;}
    void setTag(unsigned int pv_tag) {iv_tag = pv_tag;}

    int getTag() { return iv_tag;}
    void setTag(int32 pv_tag) {iv_tag = pv_tag;}
    bool equal (TM_Transid pp_transid) {return ((pp_transid == iv_transid)?true:false);}
    bool isEnder() {return iv_coordinator;}
    short get_error() { return iv_last_error;}

    static TM_Transaction * getCurrent();

    int64 getTypeFlags();
    // short setTypeFlags(int64 transactiontype_bits);


private:
    bool       iv_coordinator;
    short      iv_last_error;
    bool       iv_server;
    TM_Transid iv_transid;
    unsigned int iv_tag;
    int iv_abort_timeout;
    int64 iv_transactiontype;

    short begin(int abort_timeout, int64 transactiontype_bits);
    short join(bool pv_coordinator_role);
};

#endif
