/**********************************************************************
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
**********************************************************************/
#ifndef EX_TRACE_H
#define EX_TRACE_H


#include "ComTdb.h"

//
// Forward declarations
//


//
// class ExTrace
//   This is a container class for TraceState enum values and TracePoint enum
//   values.  See executor/ExDp2Trace.h for directions on how to add new
//   trace states or trace points in executor code.
class ExTrace
{
public:

  //
  // Values used to capture transition state changes by ExDp2TraceState
  // See ExDp2Trace.h for detailed information.
  enum TraceState {
    ROW_SELECTED_    = 1,
    ROW_NOT_SELECTED_,
    TEA_BREAK_,
    INIT_KEY_RANGE_,
    MOVE_IN_REPLY_,           // call to moveInSendOrReplyData
    EXPR_EVAL_,
    POSITION_UNIQUE_,
    POSITION_,
    POSITION_SAMPLE_,
    DELETE_UNIQ_,
    FETCH_UNIQUE_,
    FETCH_BLOCK_,
    FETCH_ROW_,
    FETCH_,
    LOCK_FILE_,
    INSERT_ROW_,
    INSERT_VP_ROW_,
    INSERT_ROW_UNIQUE_,
    INSERT_VSBB_,
    SWITCH_CONTEXT_,
    VSBB_NAK_,
    SIDETREE_SETUP_,
    SIDETREE_INSERT_,
    SIDETREE_COMMIT_,
    GET_DELTA_KEY_,
    UPDATE_UNIQUE_,
    DELETE_UNIQUE_, 
    DELETE_RANGE_,
    GET_OVERFLOW_DATA_,
    REPLY_
  };

  //
  // Values below are for trace points within an operator.
  // There can be up to 32 different trace points per operator.
  // A developer can choose which trace points to turn on - from 1 - 32 can
  // be turned on at one time per operator.
  // See ExDp2Trace.h for detailed information.
  enum  TracePoint {
    EID_ROOT_OP                 = ComTdb::ex_EID_ROOT,                  // 10

    TP_DELETE_RANGE             = 0x00000001,

    DP2_SUBS_OP                 = ComTdb::ex_DP2_SUBS_OPER,             // 23
    TP_FETCH                    = 0x00000001,
    TP_FETCH_ROW                = 0x00000002,
    TP_FETCH_BLOCK_ROW          = 0x00000004,
    TP_FETCH_UNIQUE             = 0x00000008,
    TP_ROW_SELECTED             = 0x00000010,
    TP_ROW_NOT_SELECTED         = 0x00000020,
    TP_TEA_BREAK                = 0x00000040,
    TP_EXPAND_SHORT_ROW         = 0x00000080,
    TP_LOCK_TRANSITION          = 0x00000100,

    DP2_UNIQUE_OP               = ComTdb::ex_DP2_UNIQUE_OPER,           // 25
    TP_FETCH_                   = 0x00000001,
    TP_FETCH_UNIQUE_            = 0x00000002,
    TP_DELETE_UNIQUE            = 0x00000004,
    TP_UPDATE_UNIQUE            = 0x00000008,
    TP_INSERT_UNIQUE            = 0x00000010,
    TP_ROW_SELECTED_            = 0x00000020,
    TP_ROW_NOT_SELECTED_        = 0x00000040,
    TP_INSERT                   = 0x00000080,
    TP_INSERT_DUP_REC_          = 0x00000100,
    TP_EXPAND_SHORT_ROW_        = 0x00000200,

    DP2_INSERT_OP               = ComTdb::ex_DP2_INSERT,                // 30
    TP_INSERT_DUP_REC           = 0x00000001,
    TP_INSERT_UNIQUE_DUP_REC    = 0x00000002,
    TP_INSERT_WORK              = 0x00000004,

    DP2_INSERT_VSBB_OP          = ComTdb::ex_DP2_VSBB_INSERT,           // 31
    TP_INSERT_VSBB_DUP_REC      = 0x00000001,

    DP2_INSERT_VSBB_SIDETREE_OP = ComTdb::ex_DP2_VSBB_SIDETREE_INSERT,  // 32
    TP_INSERT_SIDETREE_DUP_REC  = 0x00000001,

    DP2_UNIQUE_LEAN_OP          = ComTdb::ex_DP2_UNIQUE_LEAN_OPER,
    TP_FETCH_UNIQUE_LEAN        = 0x00000001,
    TP_DELETE_UNIQUE_           = 0x00000002,
    TP_UPDATE_UNIQUE_           = 0x00000004,
    TP_INSERT_UNIQUE_           = 0x00000008,

    MAX_OPERATOR                = ComTdb::ex_LAST
  };

};

#endif // EX_TRACE_H
