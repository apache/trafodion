//------------------------------------------------------------------
//
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

#ifndef __SB_SBCONST_H_
#define __SB_SBCONST_H_

enum { SB_MAX_PROG = 32 };

enum {
    MD_STATE_AFIRST                = 0,
    MD_STATE_WR_SENDING            = 1,
    MD_STATE_WR_SEND_FIN           = 2,
    MD_STATE_WR_FIN                = 3,
    MD_STATE_REPLY_SENDING         = 4,
    MD_STATE_REPLY_SEND_FIN        = 5,
    MD_STATE_REPLY_FIN             = 6,
    MD_STATE_ABANDON_SENDING       = 7,
    MD_STATE_ABANDON_FIN           = 8,
    MD_STATE_ABANDON_ACK_SENDING   = 9,
    MD_STATE_ABANDON_ACK_FIN       = 10,
    MD_STATE_CLOSE_SENDING         = 11,
    MD_STATE_CLOSE_FIN             = 12,
    MD_STATE_CLOSE_ACK_SENDING     = 13,
    MD_STATE_CLOSE_ACK_FIN         = 14,
    MD_STATE_OPEN_SENDING          = 15,
    MD_STATE_OPEN_FIN              = 16,
    MD_STATE_OPEN_ACK_SENDING      = 17,
    MD_STATE_OPEN_ACK_FIN          = 18,
    MD_STATE_REPLY_NACK_SENDING    = 19,
    MD_STATE_RSVD_MD               = 20,
    MD_STATE_RCVD_AGGR             = 21,
    MD_STATE_RCVD_AGGR_REPLY       = 22,
    MD_STATE_RCVD_CLOSE            = 23,
    MD_STATE_RCVD_FSREQ            = 24,
    MD_STATE_RCVD_MSREQ            = 25,
    MD_STATE_RCVD_IC               = 26,
    MD_STATE_RCVD_MON_CLOSE        = 27,
    MD_STATE_RCVD_MON_MSG          = 28,
    MD_STATE_RCVD_MON_OPEN         = 29,
    MD_STATE_RCVD_OPEN             = 30,
    MD_STATE_SEND_INLINE_OPEN      = 31,
    MD_STATE_SEND_INLINE_OPEN_SELF = 32,
    MD_STATE_ZERO_MD               = 33,
    MD_STATE_MSG_DEAGGR            = 34,
    MD_STATE_MSG_FS_NOWAIT_OPEN    = 35,
    MD_STATE_MSG_FS_SMSG_CLOSE     = 36,
    MD_STATE_MSG_LINK              = 37,
    MD_STATE_MSG_LOW_LOOP_CLOSE    = 38,
    MD_STATE_MSG_LOW_LOOP_OPEN     = 39,
    MD_STATE_MSG_LOW_LOOP_WR       = 40,
    MD_STATE_MSG_LOW_ABANDON       = 41,
    MD_STATE_MSG_LOW_CAN_ACK       = 42,
    MD_STATE_MSG_LOW_CLOSE_ACK     = 43,
    MD_STATE_MSG_LOW_CONN_ACK      = 44,
    MD_STATE_MSG_LOW_OPEN_ACK      = 45,
    MD_STATE_MSG_LOW_REPLY_NACK    = 46,
    MD_STATE_MSG_MON_MSG           = 47,
    MD_STATE_MSG_MON_OPEN          = 48,
    MD_STATE_MSG_REOPEN_FAIL       = 49,
    MD_STATE_MSG_SEND_SELF         = 50,
    MD_STATE_MSG_START_STREAM      = 51,
    MD_STATE_MSG_TIMER             = 52,
    MD_STATE_SM_INT_MSG            = 53,
    MD_STATE_CONN_SENDING          = 54,
    MD_STATE_CONN_ACK_SENDING      = 55,
    MD_STATE_CONN_FIN              = 56,
    MD_STATE_ZLAST                 = 57
};

#endif // !__SB_SBCONST_H_
