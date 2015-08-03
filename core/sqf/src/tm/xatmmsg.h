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

#ifndef XATMMSG_H_
#define XATMMSG_H_
/***********************************************************************
   xatmmsg.h
   XATM Message Class Header file
   Each CxaTM_Message object represents a single message interaction
   with an RM.  These structures are pooled in the same way TM_TX_Info
   objects are.
***********************************************************************/

#include "dtm/tmtransid.h"
#include "tmmutex.h"
#include "tmpoolelement.h"
#include "tmglob.h"

// EID must be exactly 9 characters
const char EID_CxaTM_RMMessage[] = {"xaTM_RMMsg"}; 

// external declarations
class CxaTM_RM;

class CxaTM_RMMessage :public virtual CTmPoolElement
{
private:
   // Private members
   int64 iv_msgNum;  //used as index to RMMessagePool.InUseList
   int32 iv_msgid;
   XID iv_xid;
   // ThreadId ties the RM Message to a specific thread.  This is used for
   // an integrity check. We validate that TSE RMs replies are returned 
   // to the sender thread.
   long int iv_threadId; 
   int32 iv_reqLen;
   int32 iv_rspLen;
   int iv_sendAttempts;
   int iv_sleepTime;

   CxaTM_RM * ip_RM;
   RM_Req_Msg_Type *ip_req;
   RM_Rsp_Msg_Type *ip_rsp;

   TM_Mutex *ip_mutex;  // Semaphore to serialize updates to the object.

   void lock();
   void unlock();

public:
   // Public members
   CxaTM_RMMessage(int64 pv_msgNum);
   ~CxaTM_RMMessage();

   static CxaTM_RMMessage *constructPoolElement(int64 pv_msgNum);
   int64 cleanPoolElement();

   void clean();
   void initialize(CxaTM_RM *pp_RM, TM_DP2_SQ_MSG_TYPE pv_req_type,
                   int32 pv_reqLen, int32 pv_rspLen);

   // Set/Get methods:
   CxaTM_RM * RM() {return ip_RM;}

   int32 getRmError() {return ip_rsp->iv_msg_hdr.miv_err.error;}
   bool getRmReplyOpenAx_Reg();
   int32 getRmReplyOpenOpener();
   bool getRmReplyPreparePartic();
   int32 getRmReplyType();
   int send_rm();
   short cancel(bool pv_releaseMsg=false);
   short abandon();
   short link(int32 pv_maxretries = TM_LINKRETRY_RETRIES);

   int32 msgid() {return iv_msgid;}
   void msgid(int32 pv_msgid)
   {
      iv_msgid = pv_msgid;
   }
   int64 msgNum() {return iv_msgNum;}
   int sleepTime() {return iv_sleepTime;}

   void xid(XID *pp_xid)
   {
      lock();
      memcpy(&iv_xid, pp_xid, sizeof(iv_xid));
      unlock();
   }
   XID *xid() {return &iv_xid;}

   long int threadId() {return iv_threadId;}
   void threadId(long int pv_id) 
   {
      iv_threadId = pv_id;
   }

   int sendAttempts() {return iv_sendAttempts;}
#ifdef DEBUG_MODE
   void inc_sendAttempts();
   void clear_sendAttempts();
#else
   void inc_sendAttempts() 
   {
      iv_sendAttempts++;
   }
   void clear_sendAttempts() 
   {
      iv_sendAttempts = 0;
   }
#endif

   bool retrySend();
   bool can_we_retrySend(int *pp_xaError);
   void release_from_RMmsgList();
   bool checkError(short pv_breakError, int *pp_xaError, 
                   bool pv_softRetry=true, int64 pv_transid=-1);

   RM_Req_Msg_Type * Req() {return ip_req;}
   RM_Rsp_Msg_Type * Rsp() {return ip_rsp;}
   int32 ReqLen() {return iv_reqLen;}
   int32 RspLen() {return iv_rspLen;}
};
#endif //XATMMSG_H_
