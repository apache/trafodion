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
#ifndef __TMTRANSID_H_
#define __TMTRANSID_H_

#include <stdio.h>
#include "seabed/int/types.h"
#include "dtm/tm_util.h"

typedef struct _tm_h_as_0 {
    short Application; // namely SQL, 16 possibilities
    short Reserved [2];      // 32 possibilities
    short Predefined;        // 16 possibilities

} TM_TT_Flags;

// Native Data Type
typedef SB_Int64_Type TM_Native_Type;
typedef SB_Transid_Type TM_Transid_Type;
typedef SB_Transseq_Type TM_Transseq_Type;

//  legacy transid struct
typedef struct _tmtransid_h_as_0 {
   int32 iv_seq_num;
   int32 iv_node;
} TM_Txid_legacy;

// Extended Transid
typedef struct _tmtransid_h_as_1 {
    // DO NOT MOVE THE NEXT 3 MEMBERS
    int32 iv_seq_num;
    int32 iv_node;
    int16 iv_incarnation_num;
    // DO NOT MOVE THE ABOVE MEMBERS

    int16 iv_tx_flags;
    TM_TT_Flags iv_tt_flags; 
    int16 iv_version;
    int16 iv_check_sum;
    int64 iv_timestamp;
} TM_Txid_Internal;

// transid encapsulation
class TM_Transid
{
    public:
        TM_Transid ();
        TM_Transid (int i);
        TM_Transid (TM_Transid &tx);
        TM_Transid (TM_Native_Type &tx);
        TM_Transid (TM_Txid_Internal &tx);
        ~TM_Transid(){}

        TM_Transid &operator=  (const TM_Transid &rhs);
        TM_Transid &operator=  (const TM_Transid_Type &rhs);
        TM_Transid &operator=  (const TM_Native_Type &rhs);
        TM_Transid &operator=  (const TM_Txid_Internal &rhs);

        bool        operator== (TM_Transid &rhs);
        bool        operator== (const TM_Native_Type &rhs);
        bool        operator== (const TM_Txid_Internal &rhs);

        bool        operator!= (TM_Transid &rhs);
        bool        operator!= (const TM_Native_Type &rhs);
        bool        operator!= (const TM_Txid_Internal &rhs);

        bool        operator> (const TM_Native_Type &rhs);

        bool        operator< (const TM_Native_Type &rhs);

        TM_Native_Type get_native_type() {return iv_tx.data.id[0];}
        TM_Transid_Type get_data() {return iv_tx.data;}
        TM_Transid_Type *get_data_address() {return &iv_tx.data;}
        TM_Txid_Internal get_txid() {return iv_tx.txid;}
        void set_external_data_type(TM_Transid_Type* external_tx);

        int      get_node();
        int      get_seq_num();
        int16    get_incarnation_num();
        int64    get_type_flags();
        int16    get_tx_flags();
        int16    get_version();
        int16    get_check_sum();
        int64    get_timestamp();
        void     print_self () const;

    private:
        union iv_tx {
            TM_Transid_Type data;
            TM_Txid_Internal txid;
        } iv_tx;
};

int64 TTflagstoint64(TM_TT_Flags pv_flags);

#endif


