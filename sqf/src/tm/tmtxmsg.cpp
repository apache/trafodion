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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/ms.h"
#include "rm.h"
#include "tminfo.h"
#include "tmtx.h"
#include "seabed/trace.h"
#include "tmlogging.h"


//----------------------------------------------------------------------------
// CTmTxMessage Constructor
// Defaults the msgid to NULL_MSGID for requests with no reply
// Note that instantiating a CTmTxMessage object always allocates
// space from the heap for the object.
//----------------------------------------------------------------------------
CTmTxMessage::CTmTxMessage(Tm_Req_Msg_Type * pp_req, int32 pv_msgid, char *pv_buffer)
      :CTmMessage(pp_req), iv_msgid(NULL_MSGID), 
       ip_rsp(NULL), iv_rspSize(0), ip_buffer(pv_buffer)
{
   TMTrace(2, ("CTmTxMessage::CTmTxMessage(req, msgid) : ENTRY, request(%d), msgid(%d).\n", 
                   pp_req->iv_msg_hdr.rr_type.request_type, pv_msgid));

   if (!pp_req)
   {
      TMTrace(1, ("CTmTxMessage::CTmTxMessage : PROGRAMMING ERROR no request message supplied.\n"));
      abort();
   }

   memcpy(&iv_req, pp_req, sizeof(Tm_Req_Msg_Type));
   iv_msgid = NULL_MSGID;
   initialize(pv_msgid);

   TMTrace(2, ("CTmTxMessage::CTmTxMessage : EXIT, msgid(%d).\n", pv_msgid));
} //CTmTxMessage::CTmTxMessage


//----------------------------------------------------------------------------
// CTmTxMessage Constructor (2)
// Constructs a request buffer based on the supplied request type.
// Defaults the msgid to NULL_MSGID since no reply is possible.
//----------------------------------------------------------------------------
CTmTxMessage::CTmTxMessage(short pv_reqType) 
    :CTmMessage(pv_reqType),
     ip_rsp(NULL), iv_rspSize(0), ip_buffer(NULL)
{
   TMTrace(2, ("CTmTxMessage::CTmTxMessage(reqType) : ENTRY.\n"));

   memset(&iv_req, 0, sizeof(Tm_Req_Msg_Type));
   iv_msgid = NULL_MSGID;
   requestType(pv_reqType);
   initialize(NULL_MSGID);

   TMTrace(2, ("CTmTxMessage::CTmTxMessage(reqType) : EXIT.\n"));
} //CTmTxMessage::CTmTxMessage (2)


//----------------------------------------------------------------------------
// CTmTxMessage Constructor (3)
// Constructs a request buffer based on the supplied message. This is
// effectively a copy.
//----------------------------------------------------------------------------
CTmTxMessage::CTmTxMessage(CTmTxMessage * pv_msg) 
    :CTmMessage((CTmMessage *) pv_msg),
     iv_msgid(pv_msg->msgid()),
     iv_rspSize(pv_msg->responseSize()),
     ip_buffer(NULL)
{
   TMTrace(2, ("CTmTxMessage::CTmTxMessage(msg) : ENTRY, copy msg %p, msgid %d, reqType %d.\n", 
           (void *) pv_msg, pv_msg->msgid(), pv_msg->requestType()));

   // Allocate a new response buffer for the message and copy response in.
   // The response buffer may or may not have been filled at this point, but
   // we make no assumptions here.
   ip_rsp = new Tm_Rsp_Msg_Type [gv_tm_info.tms_highest_index_used() + 1];
   memcpy(ip_rsp, pv_msg->response(), sizeof(Tm_Rsp_Msg_Type));

   TMTrace(2, ("CTmTxMessage::CTmTxMessage(msg) : EXIT, new msg %p, msgid %d, reqType %d.\n",
           (void *) this, msgid(), requestType()));
} //CTmTxMessage::CTmTxMessage (3)


//----------------------------------------------------------------------------
// CTmTxMessage Destructor
//----------------------------------------------------------------------------
CTmTxMessage::~CTmTxMessage()
{
   TMTrace(2, ("CTmTxMessage::~CTmTxMessage : ENTRY/EXIT msg %p, request(%d), msgid(%d)\n", (void *) this, requestType(), msgid()));
   iv_msgid = 0;
   if (ip_rsp == NULL) {
      TMTrace(1, ("CTmTxMessage::~CTmTxMessage : Error: second call to destructor for this message!\n"));
      tm_log_event(DTM_DUPLICATE_CALL_MSG_DESTRUCTOR, SQ_LOG_CRIT, "DTM_DUPLICATE_CALL_MSG_DESTRUCTOR");
      abort();
   }
   else
      free(ip_rsp);

   if(ip_buffer == NULL){
   }
   else {
      delete ip_buffer;
      ip_buffer = NULL;
   }
   ip_rsp = NULL;
} 


//----------------------------------------------------------------------------
// CTmTxMessage::initialize
// Purpose :  initialize the CTmTxMessage contents.
// This is called by the constructor, but may also be called repeatedly
// against a statically allocated CTmTxMessage object.
//----------------------------------------------------------------------------
void CTmTxMessage::initialize(int32 pv_msgid)
{

   TMTrace(2, ("CTmTxMessage::initialize : ENTRY, msgid(%d).\n", pv_msgid));

   // Check that we're not trying to overwrite a message that we haven't replied to yet
   if (iv_msgid != NULL_MSGID)
   {
      tm_log_event(DTM_TMTX_INVALID_MSGID, SQ_LOG_CRIT, "DTM_TMTX_INVALID_MSGID", FEDUP, -1, 
          gv_tm_info.nid(), -1, iv_msgid, -1, -1, -1, -1, -1, -1, -1, -1, pv_msgid);
      TMTrace(1, ("CTmTxMessage::initialize : ERROR msgid == 0! current msgid(%d), new msgid(%d)\n",
                      iv_msgid, pv_msgid));
      abort();
   }

   iv_msgid = pv_msgid;

   initialize_rsp((short) (iv_req.iv_msg_hdr.rr_type.request_type + 1));

   TMTrace(2, ("CTmTxMessage::initialize : EXIT, msgid(%d).\n", pv_msgid));
} //CTmTxMessage::initialize


//----------------------------------------------------------------------------
// initialize_rsp
// Purpose : Allocate and initialize a TM Library message response buffer.
//----------------------------------------------------------------------------
void CTmTxMessage::initialize_rsp(short pv_rspType)
{
   // If we're reusing the message then free the old response buffer now.
   if (ip_rsp)
   {
       free(ip_rsp);
       ip_rsp = NULL;
       iv_rspSize = 0;
   }
   iv_rspSize = rspLength(pv_rspType);
   // STATUS TM needs to allocate space for the RMs
   if (pv_rspType == TM_MSG_TYPE_STATUSTM_REPLY)
       iv_rspSize += gv_RMs.TSE()->return_highest_index_used() * sizeof(RM_INFO);
   
   ip_rsp = (Tm_Rsp_Msg_Type *) malloc(iv_rspSize);
   memset(ip_rsp, 0, iv_rspSize); //Clear out the response buffer

   ip_rsp->iv_msg_hdr.dialect_type = DIALECT_TM_SQ;
   ip_rsp->iv_msg_hdr.rr_type.reply_type = pv_rspType;
   ip_rsp->iv_msg_hdr.version.reply_version = TM_SQ_MSG_VERSION_CURRENT;
   ip_rsp->iv_msg_hdr.miv_err.error = FEOK;
}


//----------------------------------------------------------------------------
// CTmTxMessage::reply
// Purpose :  send a reply to the client (TM Library)
//----------------------------------------------------------------------------
void CTmTxMessage::reply()
{
    ushort lv_len = iv_rspSize;

    TMTrace(2, ("CTmTxMessage::reply : ENTRY. msgid(%d), reply code(%d), error(%d).\n", 
                    iv_msgid, ip_rsp->iv_msg_hdr.rr_type.reply_type, 
                    ip_rsp->iv_msg_hdr.miv_err.error));
    
    if (iv_msgid == NULL_MSGID)
    {
      tm_log_event(DTM_TMTX_INVALID_MSGID, SQ_LOG_INFO, "DTM_TMTX_INVALID_MSGID", FETOOMANY);
      TMTrace(1, ("CTmTxMessage::reply : ERROR msgid == 0!\n"));
      return; // Ignore error but don't reply
    }

    XMSG_REPLY_(iv_msgid,                // msgid
                NULL,                    // replyctrl
                0,                       // replyctrlsize
                (char *) ip_rsp,        // replydata
                lv_len,                  // replydatasize
                0,                       // errorclass
                NULL);                   // newphandle

    TMTrace(2, ("CTmTxMessage::reply EXIT. XMSG_REPLY_ msgid(%d), error(%d).\n",
                    iv_msgid, ip_rsp->iv_msg_hdr.miv_err.error));

    // Cleanup and invalid message object
    iv_msgid = NULL_MSGID;
} //CTmTxMessage::reply


//----------------------------------------------------------------------------
// CTmTxMessage::reply
// Purpose :  send a reply to the client (TM Library)
//----------------------------------------------------------------------------
void CTmTxMessage::reply(short pv_error)
{
   responseError(pv_error);
   reply();
}


//----------------------------------------------------------------------------
// CTmTxMessage::rspLength
// Purpose :  Calculate the size of the buffer to allocate
// for a response.
//----------------------------------------------------------------------------
ushort CTmTxMessage::rspLength(short pv_rspType)
{
   ushort lv_len = sizeof(MESSAGE_HEADER_SQ); // Allow for header
   switch (pv_rspType)
   {
   case TM_MSG_TYPE_ABORTTRANSACTION_REPLY: lv_len += sizeof(Abort_Trans_Rsp_Type); break;
   case TM_MSG_TYPE_AX_REG_REPLY: lv_len += sizeof(Ax_Reg_Rsp_Type); break;
   case TM_MSG_TYPE_BEGINTRANSACTION_REPLY: lv_len += sizeof(Begin_Trans_Rsp_Type); break;
   case TM_MSG_TYPE_ENDTRANSACTION_REPLY: lv_len += sizeof(End_Trans_Rsp_Type); break;
   case TM_MSG_TYPE_GETTRANSID_REPLY: lv_len += sizeof(Get_Transid_Rsp_Type); break;
   case TM_MSG_TYPE_JOINTRANSACTION_REPLY: lv_len += sizeof(Join_Trans_Rsp_Type); break;
   case TM_MSG_TYPE_LISTTRANSACTION_REPLY: lv_len += sizeof(List_Trans_Rsp_Type); break;
   case TM_MSG_TYPE_STATUSALLTRANSMGT_REPLY: lv_len += sizeof(StatusAllTrans_Rsp_Type); break;
   case TM_MSG_TYPE_TMSTATS_REPLY: lv_len += sizeof(Tmstats_Rsp_Type); break;
   case TM_MSG_TYPE_LEADTM_REPLY: lv_len += sizeof(Leadtm_Rsp_Type); break;
   case TM_MSG_TYPE_STATUSSYSTEM_REPLY: lv_len += sizeof(Tm_Sys_Status_Rsp_Type); break;
   case TM_MSG_TYPE_CALLSTATUSSYSTEM_REPLY: lv_len += sizeof(Tm_CSys_Status_Rsp_Type); break;
   case TM_MSG_TYPE_STATUSTM_REPLY: lv_len += sizeof(Statustm_Rsp_Type); break;
   case TM_MSG_TYPE_ATTACHRM_REPLY: lv_len += sizeof(Attachrm_Rsp_Type); break;
   case TM_MSG_TYPE_STATUSTRANSMGMT_REPLY: lv_len += sizeof(Status_TransM_Rsp_Type); break;
   case TM_MSG_TYPE_GETTRANSINFO_REPLY: lv_len += sizeof(GetTransInfo_Rsp_Type); break;
   case TM_MSG_TYPE_ENABLETRANS_REPLY: lv_len += sizeof(Enabletrans_Rsp_Type); break;
   case TM_MSG_TYPE_DISABLETRANS_REPLY: lv_len += sizeof(Disabletrans_Rsp_Type); break;
   case TM_MSG_TYPE_DRAINTRANS_REPLY: lv_len += sizeof(Draintrans_Rsp_Type); break;
   case TM_MSG_TYPE_QUIESCE_REPLY: lv_len += sizeof(Quiesce_Rsp_Type); break;
   case TM_MSG_TYPE_STATUSTRANSACTION_REPLY: lv_len += sizeof(Status_Trans_Rsp_Type); break;
   case TM_MSG_TYPE_SUSPENDTRANSACTION_REPLY: lv_len += sizeof(Suspend_Trans_Rsp_Type); break;
   case TM_MSG_TYPE_WAIT_TMUP_REPLY: lv_len += sizeof(Wait_TmUp_Rsp_Type); break;
   case TM_MSG_TYPE_TEST_TX_COUNT_REPLY: lv_len += sizeof(Test_Tx_Count_Rsp_Type); break;
   case TM_MSG_TYPE_ROLLOVER_CP_REPLY: lv_len += sizeof(Tm_RolloverCP_Rsp_Type); break;

   // XARM Responses
   case TM_DP2_SQ_XA_START_REPLY: lv_len += sizeof(RM_Start_Rsp_Type); break;
   case TM_DP2_SQ_XA_END_REPLY: lv_len += sizeof(RM_End_Rsp_Type); break;
   case TM_DP2_SQ_XA_ROLLBACK_REPLY: lv_len += sizeof(RM_Rollback_Rsp_Type); break;
   case TM_DP2_SQ_XA_FORGET_REPLY: lv_len += sizeof(RM_Forget_Rsp_Type); break;
   case TM_DP2_SQ_XA_COMMIT_REPLY: lv_len += sizeof(RM_Commit_Rsp_Type); break;
   case TM_DP2_SQ_XA_PREPARE_REPLY: lv_len += sizeof(RM_Prepare_Rsp_Type); break;
   case TM_DP2_SQ_XA_OPEN_REPLY: lv_len += sizeof(RM_Open_Rsp_Type); break;
   case TM_DP2_SQ_XA_CLOSE_REPLY: lv_len += sizeof(RM_Close_Rsp_Type); break;
   case TM_DP2_SQ_XA_RECOVER_REPLY: lv_len += sizeof(RM_Recover_Rsp_Type); break;
   default: lv_len += max(sizeof(Tm_Rsp_Msg_Type), sizeof(RM_Rsp_Msg_Type)); // Maximum
   }
   return lv_len;
} // rspLength
