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

#ifndef TMTSELIB_H_
#define TMTSELIB_H_
/***********************************************************************
   tmtselib.h
   TM - TSE Library Header file
   The TM - TSE Library provides TM interfaces specific to the TSE.
   These APIs are only used by the TSE and should not be provided to
   customers.
***********************************************************************/

#include "dtm/tmtransid.h"
#include "dtm/tmtse.h"
#include "tmlibmsg.h"

#define TSETM_MsgSize(MsgType) \
      (sizeof(MESSAGE_HEADER_SQ) + sizeof(MsgType))

#define TMTSETrace(mask, a) \
      if (gv_TMTSELIB.Trace(mask)) \
         trace_printf a

enum TMTSE_TraceMask
{
   TMTSE_TraceOff       = 0x0,
   TMTSE_TraceError     = 0x1,   // Error conditions
   TMTSE_TraceAPI       = 0x2,   // API entrys & exits
   TMTSE_TraceExit      = 0x4,   // Procedure exits
   TMTSE_TraceDetail    = 0x8,   // Detail trace records

   TMTSE_TraceAPIError     = TMTSE_TraceError | TMTSE_TraceAPI, 
   TMTSE_TraceExitError    = TMTSE_TraceError | TMTSE_TraceExit, 
   TMTSE_TraceAPIExitError = TMTSE_TraceError | TMTSE_TraceAPI | TMTSE_TraceExit,
   TMTSE_TraceAPIExit      = TMTSE_TraceAPI | TMTSE_TraceExit,

   TMTSE_TraceAll          = 0xffffffff
};

// Each CTmTse_TM object represents an open connection to a TM.
// These are maintained in CTmTseLib::ia_TMList.
class CTmTse_TM
{
private:
   bool iv_open;                 //true = TM connection has been successfully opened
   int iv_node;
   char iv_TMname[8];
   int16 iv_TM_incarnation_num;   //-1 = not set.
   TPT_DECL(iv_phandle);
   int iv_oid;
   int16 iv_lastError;
   SB_Thread::Mutex iv_mutex;

   void lock();
   void unlock();

public:
   CTmTse_TM();
   ~CTmTse_TM();
   void initialize(int pv_node);

    SB_Phandle_Type *phandle();
   int node() {return iv_node;}
   int16 lastError() {return iv_lastError;}
   char * TMname() {return (char *) &iv_TMname;}
   bool isOpen() {return iv_open;}
   int16 TM_incarnation_num() {return iv_TM_incarnation_num;}
   void TM_incarnation_num(int16 pv_TM_incarnation_num) {iv_TM_incarnation_num=pv_TM_incarnation_num;}

   int16 open();
   void close();

};

class CTmTse_Message
{
private:
   // Private member veriables
   TMTSETYPE iv_replyType;
   int iv_linkTag;    // Unique value set by TMTSE Library but not used.
   int iv_msgid;  // returned by BMSG_LINK_ and returned to caller.
   int32 iv_nid;  // Node id that an ax_reg was sent to.
   int32 iv_retries; // Allow a maximum of 4 retries to avoid loops.
   bool iv_breakCalled; // makes sure that BMSG_BREAK_ is called exactly once.
   TMTSE_OPTNS iv_optns; // LDONE queuing in use or not
   TM_Transid iv_Transid;
   CTmTse_TM * ip_TM;
   Tm_Req_Msg_Type *ip_req;
   Tm_Rsp_Msg_Type *ip_rsp;
   SB_Thread::Mutex iv_mutex;

public:
   // Public methods:
   CTmTse_Message();
   CTmTse_Message(CTmTse_TM * pp_TM, TMTSE_OPTNS pv_optn);
   ~CTmTse_Message();

   short msgBreak();
   int16 ax_reg(TM_Transid *pp_transid, int pv_rmid,
                int64 pv_flags, int * pp_msgid);
   int16 resend_ax_reg();
   int16 send_ax_reg();
   int32 next_nid_ax_reg(int32 pv_last_nid);
   int16 doomTransaction(TM_Transid *pp_transid, int * pp_msgid);
   int16 getReplyError();
   int msgid() {return iv_msgid;}
   bool ldone() {return (iv_optns & TMTSE_OPTN_LDONEQ);}
   TMTSETYPE replyType() {return iv_replyType;}
   TM_Transid * transid() {return &iv_Transid;}
   void copyReplyData(void *pp_data) 
   {
       int lv_len = sizeof(ip_rsp->u);
       switch (ip_req->iv_msg_hdr.rr_type.request_type)
       {
       case TM_MSG_TYPE_AX_REG:
           lv_len = sizeof(ip_rsp->u.iv_ax_reg);
           break;
       case TM_MSG_TYPE_TSE_DOOMTX:
           lv_len = sizeof(ip_rsp->u.iv_abort_trans);
           break;
       }
       memcpy(pp_data, &ip_rsp->u, lv_len);
   }
   int msgLength(TM_MSG_TYPE pv_msgType)
   {
       int lv_len;
       switch (pv_msgType)
       {
       case TM_MSG_TYPE_AX_REG:
           lv_len = TM_MsgSize(ip_req->u.iv_ax_reg);
           break;
       case TM_MSG_TYPE_AX_REG_REPLY:
           lv_len = TM_MsgSize(ip_rsp->u.iv_ax_reg);
           break;
       case TM_MSG_TYPE_TSE_DOOMTX:
           lv_len = TM_MsgSize(ip_req->u.iv_abort_trans);
           break;
       case TM_MSG_TYPE_TSE_DOOMTX_REPLY:
           lv_len = TM_MsgSize(ip_rsp->u.iv_abort_trans);
           break;
       default:
           lv_len = sizeof(MESSAGE_HEADER_SQ) + TM_MAX_DATA;
       }
       return lv_len;
   }

   Tm_Rsp_Msg_Type *Reply() {return ip_rsp;}
   int32 nid() {return iv_nid;}

   int initialize_hdr(TM_MSG_TYPE pv_req_type);
   void lock();
   int send(int pv_reqLen, int pv_rspLen);
   void unlock();
};

// CTmTseLib class represents the interface. There is one object
// instantiated per TSE process.
class CTmTseLib
{
private:
   // Private member variables
   int32 iv_my_nid;
   int32 iv_my_pid;
   int32 iv_LeadTM_nid; // if unknown, -1
   int iv_nextLinkTag;
   CTmTse_TM ia_TMList[MAX_NODES]; // List of open TMs
   TM_MAP ia_messageList;  // Each element contains a CTmTse_Message
   SB_Thread::Mutex iv_mutex;
   TMTSE_TraceMask iv_traceMask;

public:

   // Public methods:
   CTmTseLib();
   ~CTmTseLib();
   void initialize(TMTSE_TraceMask pv_traceMask);
   void lock();
   void setTrace(TMTSE_TraceMask pv_traceMask);      // Set TM-TSE Library tracing.
   bool Trace(TMTSE_TraceMask pv_traceMask);
   void unlock();

   // These two methods might be better separated into a new 
   // pHandle class (also used in xatmlib).
   static void initalizePhandle(TPT_PTR(pp_phandle));
   static void setPhandle(TPT_PTR(pp_destPhandle), TPT_PTR(pp_sourcePhandle));
   
   // Message access methods are here and not in the CTmTse_TM because
   // TMTSE_COMPLETE() receives only a msgid and can't easily determine
   // the associated TM.
   void insertMessage(CTmTse_Message * pp_Msg);
   int16 deleteMessage(int pv_msgid);
   CTmTse_Message * getMessage(int pv_msgid);
   int16 getError(int pv_msgid);
   int32 LeadTM_nid() {return iv_LeadTM_nid;}
   void LeadTM_nid(int32 pv_LeadTM_nid) {iv_LeadTM_nid=pv_LeadTM_nid;}
   int32 my_nid() {return iv_my_nid;}
   int32 my_pid() {return iv_my_pid;}

   // 
   CTmTse_TM * openTM(int pv_node);
   void closeTM(int pv_node);
   void closeAllTMs();
   int getLinkTag();
};

// Externals
extern CTmTseLib gv_TMTSELIB;
extern const char *ms_getenv_str(const char *pp_key);
extern void ms_getenv_bool(const char *pp_key, bool *pp_val);

#endif //TMTSELIB_H_


