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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dtm/tmtransid.h"
//#include "tmglobals.h"
extern int64 TTflagstoint64(TM_TT_Flags pv_flags);


// ---------------------------------------------------------------
// All inline methods for this simple class
// ---------------------------------------------------------------
TM_Transid::TM_Transid()
{
    iv_tx.data.id[0] = 0;
    iv_tx.data.id[1] = 0;
    iv_tx.data.id[2] = 0;
    iv_tx.data.id[3] = 0;
}

TM_Transid::TM_Transid (TM_Native_Type tx)
{ 
    iv_tx.data.id[0] = tx;
    iv_tx.data.id[1] = 0;
    iv_tx.data.id[2] = 0;
    iv_tx.data.id[3] = 0;
}

TM_Transid::TM_Transid (TM_Txid_Internal &tx)
{ 
    memcpy((int *) &iv_tx.txid, (int *) &tx.iv_seq_num, sizeof(TM_Txid_Internal));
}

TM_Transid::TM_Transid(TM_Transid &tx)
{
    if (&tx == NULL)
        TM_Transid();
    else
        *this = tx;
}
TM_Transid &
TM_Transid::operator= (const TM_Transid &rhs)
{
    iv_tx.data.id[0] = rhs.iv_tx.data.id[0];
    iv_tx.data.id[1] = rhs.iv_tx.data.id[1];
    iv_tx.data.id[2] = rhs.iv_tx.data.id[2];
    iv_tx.data.id[3] = rhs.iv_tx.data.id[3];

    return *this;
}


TM_Transid &
TM_Transid::operator= (const TM_Native_Type &rhs)
{
    //TM_Txid_legacy *txid = (TM_Txid_legacy *) &rhs;
    iv_tx.data.id[0] = rhs;
    iv_tx.data.id[1] = 0;
    iv_tx.data.id[2] = 0;
    iv_tx.data.id[3] = 0;

    return *this;
}

TM_Transid &
TM_Transid::operator= (const TM_Transid_Type &rhs)
{
    //TM_Txid_legacy *txid = (TM_Txid_legacy *) &rhs;
    iv_tx.data.id[0] = rhs.id[0];
    iv_tx.data.id[1] = rhs.id[1];
    iv_tx.data.id[2] = rhs.id[2];
    iv_tx.data.id[3] = rhs.id[3];

    return *this;
}

TM_Transid &
TM_Transid::operator= (const TM_Txid_Internal &rhs)
{
    iv_tx.txid.iv_seq_num = rhs.iv_seq_num;
    iv_tx.txid.iv_node = rhs.iv_node;
    iv_tx.txid.iv_incarnation_num = rhs.iv_incarnation_num;
    iv_tx.txid.iv_tx_flags = rhs.iv_tx_flags;
    iv_tx.txid.iv_tt_flags = rhs.iv_tt_flags; 
    iv_tx.txid.iv_version = rhs.iv_version;
    iv_tx.txid.iv_check_sum = rhs.iv_check_sum;
    iv_tx.txid.iv_timestamp = rhs.iv_timestamp;

    return *this;
}


bool TM_Transid::operator== (const TM_Txid_Internal &rhs)
{
    if (iv_tx.txid.iv_seq_num == rhs.iv_seq_num &&
        iv_tx.txid.iv_node == rhs.iv_node &&
        iv_tx.txid.iv_incarnation_num == rhs.iv_incarnation_num &&
        iv_tx.txid.iv_tx_flags == rhs.iv_tx_flags && 
        get_type_flags() == TTflagstoint64(rhs.iv_tt_flags) &&
        iv_tx.txid.iv_version == rhs.iv_version &&
        iv_tx.txid.iv_check_sum == rhs.iv_check_sum &&
        iv_tx.txid.iv_timestamp == rhs.iv_timestamp)
        return true;

    return false;
}
bool TM_Transid::operator== (TM_Transid &rhs)
{
    if (iv_tx.txid.iv_seq_num == rhs.get_seq_num() &&
        iv_tx.txid.iv_node == rhs.get_node() &&
        iv_tx.txid.iv_incarnation_num == rhs.get_incarnation_num() &&
        iv_tx.txid.iv_tx_flags == rhs.get_tx_flags() &&
        TTflagstoint64(iv_tx.txid.iv_tt_flags) 
              == rhs.get_type_flags() &&
        iv_tx.txid.iv_version == rhs.get_version() &&
        iv_tx.txid.iv_check_sum == rhs.get_check_sum() &&
        iv_tx.txid.iv_timestamp == rhs.get_timestamp())
        return true;

    return false;
}

bool TM_Transid::operator== (const TM_Native_Type &rhs)
{
    if (iv_tx.data.id[0] == rhs) 
        return true;

    return false;
}

bool TM_Transid::operator!= (TM_Transid &rhs)
{
    if (*this == rhs)
        return false;
    return true;
}

bool TM_Transid::operator!= (const TM_Native_Type &rhs)
{
    if (*this == rhs)
        return false;
    return true;
}

bool TM_Transid::operator!= (const TM_Txid_Internal &rhs)
{
    if (*this == rhs)
        return false;
    return true;
}

bool TM_Transid::operator> (const TM_Native_Type &rhs)
{
    if (iv_tx.data.id[0] > rhs)
        return true;

    return false;
}

bool TM_Transid::operator< (const TM_Native_Type &rhs)
{
    if (iv_tx.data.id[0] < rhs)
        return true;

    return false;
}

void TM_Transid::set_external_data_type(TM_Transid_Type* external_tx)
{
    external_tx->id[0] = iv_tx.data.id[0];
    external_tx->id[1] = iv_tx.data.id[1];
    external_tx->id[2] = iv_tx.data.id[2];
    external_tx->id[3] = iv_tx.data.id[3];

}

// Returns the node from the transid, not the one 
// stored in node (which should be the same).
int TM_Transid::get_node()
{
    int *node = (int *)&iv_tx.txid.iv_node;
    return *node;
}

int TM_Transid::get_seq_num()
{
    int *seq_num = (int *)&iv_tx.txid.iv_seq_num;
    return *seq_num;
}

int16 TM_Transid::get_incarnation_num()
{
    int16 *incarnation = &iv_tx.txid.iv_incarnation_num;
    return *incarnation;
}

int64 TM_Transid::get_type_flags()
{
    union {
       TM_TT_Flags TTflags;
       int64 flags;
    } u;
    u.TTflags = iv_tx.txid.iv_tt_flags;
    return u.flags;
}

int16 TM_Transid::get_tx_flags()
{
    return iv_tx.txid.iv_tx_flags;
}

int16 TM_Transid::get_version()
{
    return iv_tx.txid.iv_version;
}

int16 TM_Transid::get_check_sum()
{
    return iv_tx.txid.iv_check_sum;
}
int64 TM_Transid::get_timestamp()
{
    return iv_tx.txid.iv_timestamp;
}

void TM_Transid::print_self () const 
{
    if (iv_tx.data.id[0] == 0)
        return;
    printf("\n SELF :[" PFLL "] [" PFLL "] [" PFLL "] [" PFLL "]\n", 
                     iv_tx.data.id[0], iv_tx.data.id[1], 
                     iv_tx.data.id[2], iv_tx.data.id[3]);
}


// ----------------------------------------------------------------------------
// TTflagstoint64
// Convert from TM_TT_Flags to int64.
// ----------------------------------------------------------------------------
int64 TTflagstoint64(TM_TT_Flags pv_flags) 
{
    union 
    {
        TM_TT_Flags iv_flags;
        int64 iv_int;
    } u;

    u.iv_flags = pv_flags;
    return u.iv_int;
}

