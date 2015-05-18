//------------------------------------------------------------------
//
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

#ifndef __SB_MSUTIL_H_
#define __SB_MSUTIL_H_

#include "seabed/int/types.h"

#include "monmsgtype.h" // Mon_Msg_Type

// see msutil.cpp for details - only need 213
enum { MSG_UTIL_PHANDLE_LEN = 300 };

extern void        msg_util_format_phandle(char            *pp_buf,
                                           SB_Phandle_Type *pp_phandle);
extern void        msg_util_format_transid(char            *pp_buf,
                                           SB_Transid_Type  pv_transid);
extern void        msg_util_format_transseq(char             *pp_buf,
                                            SB_Transseq_Type  pv_transseq);
extern const char *msg_util_get_msg_type(MSGTYPE pv_msg_type);
extern const char *msg_util_get_req_type(REQTYPE pv_req_type);
extern const char *msg_util_get_reply_type(REPLYTYPE pv_reply_type);

#endif // !__SB_MSUTIL_H_
