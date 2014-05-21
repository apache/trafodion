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

/***********************************************************************
   tmtselib.cpp
   TM - TSE Library Implementation
   The TM - TSE Library provides TM interfaces specific to the TSE.
   These APIs are only used by the TSE and should not be provided to
   customers.
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// seabed includes
#include "seabed/ms.h"
#include "seabed/trace.h"
#include "seabed/fserr.h"
#include "seabed/thread.h"

// tm includes  
#include "dtm/tm_util.h"
#include "tmmap.h"
#include "tmtselib.h"
#include "tmlogging.h"

// Externals
//extern CTmTseLib gv_TMTSELIB;
//extern const char *ms_getenv_str(const char *pp_key);
//extern void ms_getenv_bool(const char *pp_key, bool *pp_val);
CTmTseLib gv_TMTSELIB;


//---------------------------------------------------------------------
// translateSBerr
// Convert a Seabed error to FE... 
//---------------------------------------------------------------------
short translateSBerr(int pv_SBerror)
{
   return (short) pv_SBerror;
   /* Return the error directly for now, no attempted translation
   short lv_error = FEOK;

   switch (pv_SBerror)
   {
   case XZFIL_ERR_OK:
      lv_error = FEOK;
      break;
   case XZFIL_ERR_INVALOP:
      lv_error = FEINVALOP;
      break;
   case XZFIL_ERR_SYSMESS:
      lv_error = FESYSMESS;
      break;
   case XZFIL_ERR_BADERR:
      lv_error = FEBADERR;
      break;
   case XZFIL_ERR_NOTFOUND:
      lv_error = FENOTFOUND;
      break;
   case XZFIL_ERR_BADNAME:
      lv_error = FEBADNAME;
      break;
   case XZFIL_ERR_NOSUCHDEV:
      lv_error = FENOSUCHDEV;
      break;
   case XZFIL_ERR_NOTOPEN:
      lv_error = FENOTOPEN;
      break;
   case XZFIL_ERR_BOUNDSERR:
      lv_error = FEBOUNDSERR;
      break;
   case XZFIL_ERR_WAITFILE:
      lv_error = FEWAITFILE;
      break;
   case XZFIL_ERR_NONEOUT:
      lv_error = FENONEOUT;
      break;
   case XZFIL_ERR_TOOMANY:
      lv_error = FETOOMANY;
      break;
   case XZFIL_ERR_NOBUFSPACE:
      lv_error = FENOBUFSPACE;
      break;
   case XZFIL_ERR_TIMEDOUT:
      lv_error = FETIMEDOUT;
      break;
   case XZFIL_ERR_FSERR:
      lv_error = FEFSERR;
      break;
   case XZFIL_ERR_DEVDOWN:
      lv_error = FEDEVDOWN;
      break;
   case XZFIL_ERR_BADREPLY:
      lv_error = FEBADREPLY;
      break;
   case XZFIL_ERR_OVERRUN:
      lv_error = FEOVERRUN;
      break;
   case XZFIL_ERR_INVALIDSTATE:
      lv_error = FEINVALIDSTATE;
      break;
   case XZFIL_ERR_DEVERR:
      lv_error = FEDEVERR;
      break;
   default:
      lv_error = FEBADOP;
   }
   return lv_error; */
}



//---------------------------------------------------------------------
// CTmTseLib Methods
//---------------------------------------------------------------------
CTmTseLib::CTmTseLib()
    : iv_my_nid(0), iv_LeadTM_nid(-1), iv_nextLinkTag(0), iv_traceMask(TMTSE_TraceOff)
{
}


CTmTseLib::~CTmTseLib()
{
   // close any open TMs
   closeAllTMs();
}


// delete Message
// Remove message from messageList, cleanup and delete the message object.
int16 CTmTseLib::deleteMessage(int pv_msgid)
{
   int16 lv_error = FEOK;

   CTmTse_Message *lp_Msg = (CTmTse_Message *) ia_messageList.get(pv_msgid);
   if (!lp_Msg)
      lv_error = FENOTFOUND;
   else
   {
      lock();
      ia_messageList.remove(pv_msgid);
      delete lp_Msg;
      unlock();
   }

   return lv_error;
}


// Get Message
// Retrieve the message from messageList associated with pv_msgid.
CTmTse_Message * CTmTseLib::getMessage(int pv_msgid)
{
   return (CTmTse_Message *) ia_messageList.get(pv_msgid);
}


// TM-TSE Library get TM Reply Error
// Returns the error returned in the reply from the TM for the message
// associated with pv_msgid.
int16 CTmTseLib::getError(int pv_msgid)
{
   int16 lv_error = FEOK;

   CTmTse_Message *lp_Msg = (CTmTse_Message *) ia_messageList.get(pv_msgid);
   if (!lp_Msg)
      lv_error = FENOTFOUND;
   else
      lv_error = lp_Msg->getReplyError();

   return lv_error;
}


// TM-TSE Library get next available link tag
int CTmTseLib::getLinkTag()
{
   lock();
   int lv_tag = iv_nextLinkTag++;
   unlock();

   return lv_tag;
}


// Initialize the TM-TSE Library
void CTmTseLib::initialize(TMTSE_TraceMask pv_traceMask)
{
   lock();

   iv_nextLinkTag = 1;

   setTrace(pv_traceMask);

   for (int i=0; i<MAX_NODES; i++)
      ia_TMList[i].initialize(i);

   msg_mon_get_my_info2(&iv_my_nid, // mon node-id
                        &iv_my_pid, // mon process-id
                        NULL,       // mon name
                        NULL,       // mon name-len
                        NULL,       // mon process-type
                        NULL,       // mon zone-id
                        NULL,       // os process-id
                        NULL,       // os thread-id
                        NULL);      // component-id
   unlock();
}


// TM-TSE Library lock semaphore
void CTmTseLib::lock()
{
   iv_mutex.lock();
}


// TM-TSE Library insert TM Message
// Insert the message object into the ia_messageList.
void CTmTseLib::insertMessage(CTmTse_Message *pp_Msg)
{

   if (pp_Msg->msgid() == -1)
   {
      TMTSETrace(TMTSE_TraceError, ("TMTSE: CTmTseLib::insertMessage ERROR: "
                 "msgid = -1. Expected an outstanding message.\n"));
      abort ();
   }

   // Insert message into the messageList.
   ia_messageList.put(pp_Msg->msgid(), pp_Msg);

   TMTSETrace(TMTSE_TraceExit, ("TMTSE: CTmTseLib::insertMessage EXIT.\n"));
}


// TM-TSE Library setTrace
// Sets the value of iv_traceMask.
// Note that because this is a mask it is concatenated to the mask unless
// set to 0. Ie:
// If pv_traceMask == 0, set iv_traceMask = 0
// If pv_traceMask > 0, iv_traceMask |= pv_traceMask; (bit-wise OR)
void CTmTseLib::setTrace(TMTSE_TraceMask pv_traceMask)
{
   TMTSE_TraceMask iv_OldMask = iv_traceMask;

   if (pv_traceMask == TMTSE_TraceOff)
      iv_traceMask = TMTSE_TraceOff;
   else
      iv_traceMask = (TMTSE_TraceMask) (iv_traceMask | pv_traceMask);

   // Don't use XATrace here as we always want to write the trace record.
   if (iv_OldMask != TMTSE_TraceOff && pv_traceMask == TMTSE_TraceOff)
      trace_printf("TMTSE: Tracing off.\n");
   else
      trace_printf("TMTSE: Tracing on, Mask=0x%x.\n", iv_traceMask);
}


// TM-TSE Library Trace
bool CTmTseLib::Trace(TMTSE_TraceMask pv_traceMask)
{
   return ((pv_traceMask & iv_traceMask)? true: false);
}


// TM-TSE Library unlock
void CTmTseLib::unlock()
{
   iv_mutex.unlock();
}


// TM-TSE Library initializePhandle
// Initialize a phandle to zeros.
inline void CTmTseLib::initalizePhandle(TPT_PTR(pp_phandle))
{
   memset(pp_phandle, 0, sizeof (SB_Phandle_Type));
}


// TM-TSE Library setPhandle
// Set a (destination) phandle to a supplied (source) phandle.
inline void CTmTseLib::setPhandle(TPT_PTR(pp_destPhandle),
                                  TPT_PTR(pp_sourcePhandle))
{
   memcpy(pp_destPhandle, pp_sourcePhandle, sizeof (SB_Phandle_Type));
}


// TM-TSE Library openTM
// Lookup the node in ia_TMList.  If the TM is open then pass back a pointer to the 
// CTmTse_TM object.  Otherwise open it and add to the ia_TMList.
CTmTse_TM * CTmTseLib::openTM(int pv_node)
{
   int16 lv_error = FEOK;

   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTseLib::openTM ENTRY, node=%d.\n ",
              pv_node));

   if (pv_node < 0 || pv_node >= MAX_NODES)
   {
      TMTSETrace(TMTSE_TraceError, ("TMTSE: CTmTseLib::openTM ERROR: node %d invalid "
                 "(< 0 or >= %d)\n.",
                 pv_node, MAX_NODES));
      return NULL;
   }

   lock();

   // Lookup the TM object
   CTmTse_TM *lp_TM = (CTmTse_TM *) &ia_TMList[pv_node];

   //10/14/2011 Removed call to close as it only sets isOpen() = false
   // and returns.  If the open fails we will pass the error back to caller.
   // If the TM had an error then close, cleanup and retry.
   //if (lp_TM->isOpen() && lp_TM->lastError() != FEOK)
   //   lp_TM->close();

   if (!lp_TM->isOpen())
         lv_error = lp_TM->open();

   unlock();

   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTseLib::openTM EXIT, node=%d, error=%d.\n ",
              pv_node, lv_error));

   return lp_TM;
}


// TM-TSE Library closeTM
// Close the TM, clean up CTmTse_TM object and mark TM connection closed.
void CTmTseLib::closeTM(int pv_node)
{
   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTseLib::closeTM ENTRY, node=%d.\n ",
              pv_node));

   if (pv_node < 0 || pv_node >= MAX_NODES)
   {
      TMTSETrace(TMTSE_TraceError, ("TMTSE: CTmTseLib::closeTM ERROR: node %d invalid "
                 "(< 0 or >= %d)\n.",
                 pv_node, MAX_NODES));
      return;
   }


   if (ia_TMList[pv_node].isOpen())
   {
      TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTseLib::closeTM TM was open on node=%d.\n ",
                 pv_node));
      ia_TMList[pv_node].close();
   }

   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTseLib::closeTM EXIT, node=%d.\n ",
              pv_node));
}

void CTmTseLib::closeAllTMs()
{
   for (int lv_node=0; lv_node<MAX_NODES; lv_node++)
      closeTM(lv_node);
}



//---------------------------------------------------------------------
// CTmTse_TM Methods
//---------------------------------------------------------------------

// TM-TSE TM Default Constructor
inline CTmTse_TM::CTmTse_TM()
    :iv_open(false), iv_node(0), iv_TM_incarnation_num(-1),  iv_oid(0), iv_lastError(0) 
{
    memset(iv_TMname, 0, 8);
    memset(&iv_phandle, 0, sizeof(SB_Phandle_Type));
}


// TM-TSE TM destructor
inline CTmTse_TM::~CTmTse_TM()
{
   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTse_TM::~CTmTse_TM "
              "Destructor called. node=%d\n", iv_node));
   close();
}


// TM-TSE TM Initialize
void CTmTse_TM::initialize(int pv_node)
{
   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTse_TM::initialize "
              "ENTRY. node=%d\n", pv_node));
   
   iv_open = false;
   iv_node = pv_node;
   sprintf (iv_TMname, "$tm%d", pv_node);
   iv_lastError = FEOK;
}


// TM-TSE TM get phandle
SB_Phandle_Type * CTmTse_TM::phandle()
{
   return (SB_Phandle_Type *) &iv_phandle;
}


// TM_TSE TM close
void CTmTse_TM::close()
{
   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTse_TM::close ENTRY, node=%d, TM=%s\n",
              iv_node, iv_TMname));
   lock();
   // Removed call to msg_mon_close_process because it causes a problem during shutdown.
   // iv_lastError = translateSBerr(msg_mon_close_process(phandle()));
   iv_lastError = FEOK;
   iv_open = false;
   unlock();

   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTse_TM:close EXIT: last "
              "error %d.\n", iv_lastError));
}


// TM_TSE TM open
int16 CTmTse_TM::open()
{
   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTse_TM::open ENTRY, node=%d\n",
              iv_node));

   lock();
   iv_lastError = translateSBerr(
                  msg_mon_open_process(iv_TMname,
                                       &iv_phandle,
                                       &iv_oid));

   if (iv_lastError == FEOK)
      iv_open = true;
   unlock();

   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTse_TM:open EXIT: msg_mon_open_process "
              "error %d, trying to open TM %s on node %d.\n",
              iv_lastError, iv_TMname, iv_node));

   return iv_lastError;
}


// TM-TSE TM lock semaphore
void CTmTse_TM::lock()
{
   iv_mutex.lock();
}


// TM-TSE TM unlock semaphore
void CTmTse_TM::unlock()
{
   iv_mutex.unlock();
}


//---------------------------------------------------------------------
// CTmTse_Message Methods
//---------------------------------------------------------------------

// TM-TSE Message Default Constructor
CTmTse_Message::CTmTse_Message()
{
   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTse_Message::CTmTse_Message "
              "default constructor called.\n"));
   const char la_buf[] = "DTM_SELIB_DEF_CONS";
   tm_log_write(DTM_SELIB_DEF_CONS, SQ_LOG_CRIT, (char *) &la_buf);
   // Logic error
   abort ();
}

// TM-TSE Message Constructor
CTmTse_Message::CTmTse_Message(CTmTse_TM *pp_TM, TMTSE_OPTNS pv_optn)
{

   iv_linkTag = gv_TMTSELIB.getLinkTag();
   iv_msgid = -1;
   ip_TM = pp_TM;
   iv_breakCalled = false;
   iv_replyType = TMTSETYPE_UNDEFINED;
   iv_Transid = 0;
   iv_nid = 0;
   iv_retries = 0;
   iv_optns = pv_optn;

   ip_req = NULL;
   ip_rsp = NULL;

   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTse_Message::CTmTse_Message "
              "Constructor called for TM %s, link tag %d.\n", 
              pp_TM->TMname(), iv_linkTag));
}


// TM-TSE Message destructor
CTmTse_Message::~CTmTse_Message()
{
   delete ip_req;
   delete ip_rsp;
}


// TM-TSE Message break
// Calls BMSG_BREAK_.
// Multiple calls can be made to this routine but it will call break only once.
short CTmTse_Message::msgBreak()
{
   short lv_error = FEOK;
   short   la_results[6];

   TMTSETrace(TMTSE_TraceDetail, ("TMTSE: CTmTse_Message::msgBreak ENTRY, msgid=%d\n",
              iv_msgid));

   if (!iv_breakCalled)
   {
      lock();
      lv_error = BMSG_BREAK_(iv_msgid, la_results, ip_TM->phandle());
      iv_breakCalled = true;
      unlock();
      TMTSETrace(TMTSE_TraceExit, ("TMTSE: CTmTse_Message::msgBreak EXIT, msgid=%d, "
                 "MSG_BREAK returned %d\n",
                 iv_msgid, lv_error));
   }
   else
      TMTSETrace(TMTSE_TraceExit, ("TMTSE: CTmTse_Message::msgBreak EXIT, msgid=%d, "
                 "MSG_BREAK not called, returning %d\n",
                 iv_msgid, lv_error));

   return translateSBerr(lv_error);
}


// TM-TSE Message getReplyError
// Returns the error from the reply message
int16 CTmTse_Message::getReplyError()
{
   return ip_rsp->iv_msg_hdr.miv_err.error;
}


// TM-TSE Message ax_reg
// We initially try to send the request to the transaction 
// owners TM.  If we get an FEPATHDOWN error, we first check
// for a Lead TM and send it there if there is one, otherwise
// we try nid 0 and 1 and assume DTM is down if both fail.
int16 CTmTse_Message::ax_reg(TM_Transid *pp_transid, int pv_rmid, 
                             int64 pv_flags, int *pp_msgid)
{
   int16 lv_error = FEOK;

   lock();
   iv_nid = pp_transid->get_node();
   iv_retries = 0;
   initialize_hdr(TM_MSG_TYPE_AX_REG);
   ip_req->u.iv_ax_reg.iv_txid = pp_transid->get_data();
   ip_req->u.iv_ax_reg.iv_rmid = pv_rmid;
   ip_req->u.iv_ax_reg.iv_flags = pv_flags;
   iv_replyType = TMTSETYPE_REGISTERTRANSACTION;
   iv_Transid = *pp_transid;

   lv_error = send_ax_reg();

   unlock();

   TMTSETrace((lv_error?TMTSE_TraceExitError:TMTSE_TraceExit),
              ("TMTSE: CTmTse_Message::ax_reg EXIT, returning %d from $TM%d.\n", 
              lv_error, iv_nid));

   // Return the Msgid from BMSG_LINK_
   *pp_msgid = iv_msgid;
   return lv_error;
} //ax_reg


// TM-TSE Message resend_ax_reg
// resend_ax_reg is called when an ax_reg reply is received indicating that
// the destination TM does not know of the transaction.  We determine this
// in the TMTSE_COMPLETE function when reply error is FEWRONGID.
int16 CTmTse_Message::resend_ax_reg()
{
   lock();
   iv_nid = next_nid_ax_reg(iv_nid);

   int16 lv_error = send_ax_reg();
   unlock();

   // Convert the error to FERETRY to inform the TSE that it needs to retry the listen.
   if (lv_error == FEOK)
      lv_error = FERETRY;

   TMTSETrace((lv_error?TMTSE_TraceExitError:TMTSE_TraceExit),
              ("TMTSE: CTmTse_Message::resend_ax_reg EXIT, returning %d from $TM%d.\n", 
              lv_error, iv_nid));
   return lv_error;
} //resend_ax_reg


// TM-TSE Message send_ax_reg
// Sends an ax_reg message to a TM
// Returns the error code returned by the last send.
int16 CTmTse_Message::send_ax_reg()
{
   int16 lv_error = FEPATHDOWN;
   int32 lv_nid = iv_nid;

   while (lv_error == FEPATHDOWN)
   {
      ip_req->u.iv_ax_reg.iv_tm_nid = lv_nid;
      lv_error = (int16) send(TSETM_MsgSize(ip_req->u.iv_ax_reg),
                              TSETM_MsgSize(ip_rsp->u.iv_ax_reg));

      if (lv_error == FEPATHDOWN)
      {
         lv_nid = next_nid_ax_reg(lv_nid);
         if (lv_nid < 0)
            lv_error = FETMFNOTRUNNING;
         TMTSETrace((TMTSE_TraceExitError),
                    ("TMTSE: CTmTse_Message::send_ax_reg ERROR FEPATHDOWN (201), Retrying $TM%d.\n", 
                    lv_nid));
      }                  
   }
   // Remember which TM we sent the request to
   iv_nid = lv_nid;

   TMTSETrace((lv_error?TMTSE_TraceExitError:TMTSE_TraceExit),
              ("TMTSE: CTmTse_Message::send_ax_reg EXIT, returning %d from $TM%d.\n", 
              lv_error, lv_nid));
   return lv_error;
} //send_ax_reg


// TM-TSE Message next_nid_ax_reg
// Determine the next TM to try for ax_reg 
// The algorithm is:
//   1. Try the owner TM
//   2. Try Lead TM if known
//   3. Try TM0
//   4. Try TM1.
// Returns the next nid or -1 if retries exceeded.
int32 CTmTse_Message::next_nid_ax_reg(int32 pv_last_nid)
{
   int32 lv_next_nid = -1;
   const int32 lc_max_retries = 4;

   if (iv_retries >= lc_max_retries)
      return -1;
   // Start with the Lead TM if defined, otherwise start at local node.
   if (ip_rsp->u.iv_ax_reg.iv_LeadTM_nid >= 0 &&
       pv_last_nid != ip_rsp->u.iv_ax_reg.iv_LeadTM_nid)
      lv_next_nid = ip_rsp->u.iv_ax_reg.iv_LeadTM_nid;
   else if (pv_last_nid != gv_TMTSELIB.my_nid())
      lv_next_nid = gv_TMTSELIB.my_nid();
   else if (pv_last_nid != 0)
      lv_next_nid = 0;
   else
      lv_next_nid = 1;

   return lv_next_nid;
} //next_nid_ax_reg


// TM-TSE Message doomTransaction
int16 CTmTse_Message::doomTransaction(TM_Transid *pp_transid, int *pp_msgid)
{
   int16 lv_error = FEOK;

   lock();
   initialize_hdr(TM_MSG_TYPE_TSE_DOOMTX);
   ip_req->u.iv_abort_trans.iv_tag = 0;
   ip_req->u.iv_abort_trans.iv_transid = pp_transid->get_data();
   ip_req->u.iv_abort_trans.iv_nid = gv_TMTSELIB.my_nid();
   ip_req->u.iv_abort_trans.iv_pid = gv_TMTSELIB.my_pid();
   iv_replyType = TMTSETYPE_DOOMTRANSACTION;
   iv_Transid = *pp_transid;
   unlock();

   lv_error = (int16) send(TSETM_MsgSize(ip_req->u.iv_abort_trans),
                           TSETM_MsgSize(ip_rsp->u.iv_abort_trans));

   TMTSETrace((lv_error?TMTSE_TraceExitError:TMTSE_TraceExit),
              ("TMTSE: CTmTse_Message::doomTransaction EXIT, returning %d.\n", 
              lv_error));

   // Return the Msgid from BMSG_LINK_
   *pp_msgid = iv_msgid;
   return lv_error;
}

// TM-TSE Message initialize_hdr
// Note that the caller should encapsulate a call to initialize_hdr with
// lock() and unlock() to protect the data structures.
int CTmTse_Message::initialize_hdr(TM_MSG_TYPE pv_req_type)
{
   ip_req = new_req(msgLength(pv_req_type));
   ip_rsp = new_rsp(msgLength(rsp_type(pv_req_type)));

   ip_req->iv_msg_hdr.dialect_type = DIALECT_DP2_TM_SQ_PRIV;
   ip_req->iv_msg_hdr.rr_type.request_type = (short) pv_req_type;
   ip_req->iv_msg_hdr.version.request_version = 
                               TM_SQ_MSG_VERSION_CURRENT;
   ip_req->iv_msg_hdr.miv_err.minimum_interpretation_version = 
                               TM_SQ_MSG_VERSION_CURRENT;

   return 0;
}


// TM-TSE Messgae lock semaphore
void CTmTse_Message::lock()
{
   iv_mutex.lock();
}


// TM-TSE Message send
// Send the message to the TM
int CTmTse_Message::send(int pv_reqLen, int pv_rspLen)
{
   short           lv_ret = 0;
   int             lv_error = FEOK;
   const unsigned short TM_MAX_DATA = tm_max(sizeof(Tm_Req_Msg_Type), 
                                             sizeof(Tm_Rsp_Msg_Type));

   if (pv_reqLen > TM_MAX_DATA || pv_rspLen > TM_MAX_DATA)
   {
      // TODO: Log error
      TMTSETrace(TMTSE_TraceError,("TMTSE: LOGIC ERROR! CTmTse_Message::send "
                 "request or reply length too large.\n"));
      char la_buf[] = "DTM_SELIB_TSE_MSG_TOO_LONG";
      tm_log_write(DTM_SELIB_TSE_MSG_TOO_LONG, SQ_LOG_CRIT, (char *) &la_buf);
      abort ();
   }
   else
      lv_ret = BMSG_LINK_(ip_TM->phandle(),            // phandle,
                          &iv_msgid,                   // msgid
                          NULL,                        // reqctrl
                          0,                           // reqctrlsize
                          NULL,                        // replyctrl
                          0,                           // replyctrlmax
                          (char *) ip_req,             // reqdata
                          pv_reqLen,                   // reqdatasize
                          (char *) ip_rsp,             // replydata
                          pv_rspLen,                   // replydatamax
                          iv_linkTag,                  // linkertag
                          TSE_LINK_PRIORITY,           // pri
                          0,                           // xmitclass
                          (ldone()?BMSG_LINK_LDONEQ:0));// linkopts - LDONE queuing if set

   if (lv_ret != 0)
   {
      // Log error
      lv_error = lv_ret;
   }

   TMTSETrace((lv_error?TMTSE_TraceExitError:TMTSE_TraceExit),
              ("TMTSE: CTmTse_Message::send EXIT, BMSG_LINK_ returned %d. LDONE queuing=%d\n", 
              lv_ret, ldone()));
   return lv_error;
}


// TM-TSE Message unlock
void CTmTse_Message::unlock()
{
   iv_mutex.unlock();
}




