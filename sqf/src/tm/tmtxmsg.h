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

#ifndef TMTXMSG_H_
#define TMTXMSG_H_

#include <assert.h>

#include "tmlibmsg.h"
#include "tmmsg.h"

class CTmTxMessage :public CTmMessage
{
private: 
   int32                iv_msgid;
   Tm_Rsp_Msg_Type     *ip_rsp;
   ushort               iv_rspSize;
   char                *ip_buffer;

public:
   CTmTxMessage(Tm_Req_Msg_Type * pp_req, int32 pv_msgid = NULL_MSGID, char * pv_buffer=NULL);
   CTmTxMessage(short pv_reqType);
   CTmTxMessage(CTmTxMessage * pp_msg);
   ~CTmTxMessage();

   void initialize(int32 pv_msgid);
   void initialize_rsp(short pv_rspType);
   void reply();
   void reply(short error);
   ushort  rspLength(short pv_rspType);

   void msgid(int32 pv_msgid) {iv_msgid = pv_msgid;}
   int32 msgid() {return iv_msgid;}
   bool replyPending() {return (iv_msgid==NULL_MSGID?false:true);}
   void responseError(short error) {ip_rsp->iv_msg_hdr.miv_err.error = error;}
   short responseError() {return ip_rsp->iv_msg_hdr.miv_err.error;}
   short responseType() {return ip_rsp->iv_msg_hdr.rr_type.reply_type;}
   void responseType(short pv_rspType) {ip_rsp->iv_msg_hdr.rr_type.reply_type = pv_rspType;}
   int32 responseSize() {return iv_rspSize;}

   Tm_Rsp_Msg_Type * response() {return ip_rsp;}

   char * getBuffer() {return ip_buffer;}
};

#endif //TMTXMSG_H_
