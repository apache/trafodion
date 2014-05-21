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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <time.h>
//#include <unistd.h>
//#include <sys/time.h>

#include "dtm/tmtse.h"
#include "tmlogging.h"

// seabed includes
#include "seabed/ms.h"
//#include "seabed/msevents.h"
#include "seabed/trace.h"
//#include "common/sq_common.h"

#include "tmmap.h"
#include "dtm/tmtse.h"
#include "tmtselib.h"

// Globals:
// CTmTseLib gv_TMTSELIB;
bool gv_TMTSEInitialized = false;


// ---------------------------------------------------------------
// TMTSE_invalidOptn
// Returns true if the TMTSE_OPTNS parameter is invalid.
// ---------------------------------------------------------------
bool TMTSE_invalidOptn(TMTSE_OPTNS pv_optn)
{
    bool pv_return = true;
    switch (pv_optn)
    {
    case TMTSE_OPTN_NONE:
    case TMTSE_OPTN_LDONEQ:
        pv_return = false;
    }
    return pv_return;
}

// ---------------------------------------------------------------
// TMTSE_initialize_trace
// Purpose - read environment variables and initialize tracing.
// pv_enable == true  --> enable tracing
// pv_enable == false --> disable tracing.
// ---------------------------------------------------------------
void TMTSE_initialize_trace(bool pv_enable, TMTSE_TraceMask *pp_mask)
{
   static bool lv_trace_enabled = false;
   union {
      TMTSE_TraceMask lv_traceMask;
      int lv_traceMaskInt;
   } u;

   if (!pv_enable)
   {
      lv_trace_enabled = false;
      return;
   }
   if (lv_trace_enabled)
      return;

   //initialize trace file
   char * lp_traceString = (char *) ms_getenv_str("TMTSE_TRACE");
   if (lp_traceString)
   {  // The environment variable was defined, extract the trace mask
      char * lp_traceStringEnd;
      lp_traceStringEnd = lp_traceString + strlen(lp_traceString);

      if (lp_traceStringEnd == NULL)
      {
         char la_buf[] = "DTM_SEAPI_INVALID_STRING_SIZE";
         tm_log_write(DTM_SEAPI_INVALID_STRING_SIZE, SQ_LOG_CRIT, (char *) &la_buf);
         // Make sure the lp_traceStringEnd pointer points to the null terminator.
      }
      assert(lp_traceStringEnd);

      //Convert hexadecimal string to int
      unsigned long lv_traceMaskul = strtoul(lp_traceString, &lp_traceStringEnd, 16); 
      u.lv_traceMaskInt = (int) lv_traceMaskul;
   }
   else
      u.lv_traceMask = TMTSE_TraceOff;

   if (u.lv_traceMask != TMTSE_TraceOff)
   {
      bool lv_unique = false;
      ms_getenv_bool("TMTSE_TRACE_UNIQUE", &lv_unique);
      const char *lp_file = ms_getenv_str("TMTSE_TRACE_FILE");
      if (lp_file != NULL)
      {
         char *lp_trace_file = (char *) lp_file;
         trace_init(lp_trace_file, lv_unique, NULL, false);
      }
      else 
         trace_init((char *)"tmtse_trace", lv_unique, NULL, false);
   }

   *pp_mask = u.lv_traceMask;
   lv_trace_enabled = true;
} //TMTSE_initialize_trace


// ---------------------------------------------------------------
// TMTSE_initalize
// Purpose - Initialize the TM-TSE library.
// ---------------------------------------------------------------
short TMTSE_initialize ()
{
   short lv_error = 0;
   TMTSE_TraceMask lv_traceMask = TMTSE_TraceOff;

   if (gv_TMTSEInitialized != true)
   {
      TMTSE_initialize_trace(true, &lv_traceMask);
      gv_TMTSELIB.initialize(lv_traceMask);

      gv_TMTSEInitialized = true;
   }

   TMTSETrace((lv_error?TMTSE_TraceAPIExitError:TMTSE_TraceAPIExit),
              ("TMTSE: TMTSE_initialize EXIT returning %d\n", lv_error));
   return lv_error;
} //TMTSE_initialize


// ---------------------------------------------------------------
// DOOMTRANSACTION
// Purpose - sends an ABORT transaction message to the TM.  This 
// differs from the TM Library TMF_DOOMTRANSACTION_ API which
// implements the genuine TMF doomtransaction logic.  DOOMTRANSACTION
// really performs a no-waited ABORTTRANSACTION on behalf of a TSE.
// This function allows callers to use the short/old transid form
// and calls DOOMTRANSACTION_EXT to do the real work.
// ---------------------------------------------------------------
extern "C" short DOOMTRANSACTION (int64 pv_transid,
                                  int *pp_msgid,
                                  TMTSE_OPTNS pv_optns)
{
   TM_Transid lv_transid;
   lv_transid = pv_transid;
   TM_Transid_Type lv_extTransid = lv_transid.get_data();
   
   return DOOMTRANSACTION_EXT(&lv_extTransid, pp_msgid, pv_optns);
} //DOOMTRANSACTION


// ---------------------------------------------------------------
// DOOMTRANSACTION_EXT
// Purpose - sends an ABORT transaction message to the TM.  This 
// differs from the TM Library TMF_DOOMTRANSACTION_ API which
// implements the genuine TMF doomtransaction logic.  DOOMTRANSACTION
// really performs a no-waited ABORTTRANSACTION on behalf of a TSE.
// This function allows the TSE to abort a transaction without
// participating and in a nowaited manner.
// msgid from the BMSG_LINK_ call is returned as pp_msgid so that
// the caller can associate the request with a BMSG_LISTEN_ completion
// and then call TMTSE_COMPLETE to cleanup the buffers and possibly
// extract the contents of the reply.
//
// Note that DOOMTRANSACTION[_EXT] can be called more than once 
// against the same transaction.  DOOMTRANSACTION will return FEOK, 
// but only one call will return FEOK to the associated TMTSE_COMPLETE 
// calls.
//
// If successful, DOOMTRANSACTION_EXT sets pp_msgid to
// the msgid returned by BMSG_LINK_.
// If unsuccessful, DOOMTRANSACTION_EXT an error (!= FEOK).
// 
// pv_optns determines whether LDONE queuing is to be used. By default
// LDONE queuing is used for DOOMTRANSACTION.  If LDONE queuing is specified,
// callers must listen on LDONE and then call TMTSE_COMPLETE[_EXT]() 
// when they received an LDONE completion for the msgid returned by 
// DOOMTRANSACITON_EXT.  TMTSE_COMPLETE(msgid) will call BMSG_BREAK_, 
// free up the message buffers and any other structures associated 
// with the message.
// 
// Return Codes:
//    FEOK              Success
//    FEBADPARMVALUE    Missing parameter. pp_msgid or pp_extTransid
//    FEINVTRANSID      Failed to locate DTM instance for transaction.
//    any error returned by msg_mon_open_process().
// ---------------------------------------------------------------
extern "C" short DOOMTRANSACTION_EXT (TM_Transid_Type *pp_extTransid,
                                      int *pp_msgid,
                                      TMTSE_OPTNS pv_optns)
{
   short lv_error = FEOK;
   
   if (!gv_TMTSEInitialized)
   {
      lv_error = (int16) TMTSE_initialize();
      if (lv_error)
         return lv_error;
   }

   // Validate parameters
   if (!pp_extTransid || !pp_msgid || TMTSE_invalidOptn(pv_optns))
   {
      TMTSETrace(TMTSE_TraceAPIError,("TMTSE: DOOMTRANSACTION missing parameter.\n"));
      return FEBADPARMVALUE;
   }

   *pp_msgid = -1;
   TM_Transid lv_extTransid;
   lv_extTransid = *pp_extTransid;

   TMTSETrace(TMTSE_TraceAPI,("TMTSE: DOOMTRANSACTION_EXT ENTRY transid=" PFLL "\n",
              lv_extTransid.get_native_type()));
   
   // Open the TM if necessary
   int lv_node = lv_extTransid.get_node();
   CTmTse_TM *lp_TM = gv_TMTSELIB.openTM(lv_node);
   if (!lp_TM)
      lv_error = FEINVTRANSID;
   else
      lv_error = lp_TM->lastError();

   if (lv_error == FEOK)
   {
      // Instantiate a message object
      CTmTse_Message * lp_Msg = new CTmTse_Message(lp_TM, pv_optns);

      if (lp_Msg)
      {
         lv_error = lp_Msg->doomTransaction(&lv_extTransid, pp_msgid);
         if (lv_error == FEOK)
            // Add to message list
            gv_TMTSELIB.insertMessage(lp_Msg);
      }
      else
         TMTSETrace(TMTSE_TraceError,("TMTSE: CTmTse_TM::doomTransaction failed "
                    "to allocate new CTmTse_Message object.\n"));
      }

   TMTSETrace((lv_error?TMTSE_TraceAPIExitError:TMTSE_TraceAPIExit),
              ("TMTSE: DOOMTRANSACTION EXIT returning %d.\n", lv_error));
   return lv_error;
} //DOOMTRANSACTION_EXT


// ---------------------------------------------------------------
// REGISTERTRANSACTION
// Purpose - sends an ax_reg message to the TM which owns the transaction.
// This allows the TSE to dynamically register transaction enlistment with 
// DTM on demand.
// This function allows callers to use the short/old transid form
// and calls REGISTERTRANSACTION_EXT to do the real work.
// ---------------------------------------------------------------
extern "C" short REGISTERTRANSACTION (int64 pv_transid, int pv_rmid,
                                      int64 pv_flags, int *pp_msgid,
                                      TMTSE_OPTNS pv_optns)
{
   TM_Transid lv_transid;
   lv_transid = pv_transid;
   TM_Transid_Type lv_extTransid = lv_transid.get_data();
   
   return REGISTERTRANSACTION_EXT(&lv_extTransid, pv_rmid, pv_flags, pp_msgid, pv_optns);
} //REGISTERTRANSACTION


// ---------------------------------------------------------------
// REGISTERTRANSACTION_EXT
// Purpose - sends an ax_reg message to the TM which owns the transaction.
// This allows the TSE to dynamically register transaction enlistment with 
// DTM on demand.
// REGISTERTRANSACTION_ is a nowaited call.  The caller is required to 
// call XWAIT and BMSG_LISTEN_, followed by TMTSE_COMPLETE to cleanup buffers.
// msgid from the BMSG_LINK_ call is returned as pp_msgid so that
// the caller can associate the request with a BMSG_LISTEN_ completion
// and then call TMTSE_COMPLETE to cleanup the buffers and possibly
// extract the contents of the reply.
// pv_rmid must be the RM id for the caller TSE.  pv_flags is not currently
// used and should be set to 0.
//
// Note that REGISTERTRANSACTION[_EXT] can be called more than once 
// against the same transaction.  REGISTERTRANSACTION will return FEOK to 
// each call.
//
// If successful, REGISTERTRANSACTION_EXT sets pp_msgid to
// the msgid returned by BMSG_LINK_.
// If unsuccessful, REGISTERTRANSACTION_EXT an error (!= FEOK).
// 
// pv_optns defaults to TMTSE_OPTN_NONE, so LDONE queuing is off.
// Callers must call TMTSE_COMPLETE[_EXT]() when the completion
// arrives for the msgid returned by 
// REGISTERTRANSACTION[_EXT].  TMTSE_COMPLETE(msgid) will call BMSG_BREAK_, 
// free up the message buffers and any other structures associated 
// with the message.
// 
// Return Codes:
//    FEOK              Success
//    FEBADPARMVALUE    Missing parameter.
//    FEINVTRANSID      Failed to locate DTM instance for transaction.
//    any error returned by msg_mon_open_process().
// ---------------------------------------------------------------
extern "C" short REGISTERTRANSACTION_EXT (TM_Transid_Type *pp_extTransid, int pv_rmid,
                                          int64 pv_flags, int *pp_msgid,
                                          TMTSE_OPTNS pv_optns)
{
   short lv_error = FEOK;
   
   if (!gv_TMTSEInitialized)
   {
      lv_error = (int16) TMTSE_initialize();
      if (lv_error)
         return lv_error;
   }

   // Validate parameters
   if (!pp_extTransid || !pp_msgid || TMTSE_invalidOptn(pv_optns))
   {
      TMTSETrace(TMTSE_TraceAPIError,("TMTSE: REGISTERTRANSACTION missing parameter.\n"));
      return FEBADPARMVALUE;
   }

   *pp_msgid = -1;
   TM_Transid lv_extTransid;
   lv_extTransid = *pp_extTransid;

   TMTSETrace(TMTSE_TraceAPI,("TMTSE: REGISTERTRANSACTION_EXT ENTRY transid=" PFLL "\n",
              lv_extTransid.get_native_type()));
   
   // Open the TM if necessary
   int lv_node = lv_extTransid.get_node();
   CTmTse_TM *lp_TM = gv_TMTSELIB.openTM(lv_node);
   if (!lp_TM)
      lv_error = FEINVTRANSID;
   else
      lv_error = lp_TM->lastError();

   if (lv_error == FEOK)
   {
      // Instantiate a message object
      CTmTse_Message * lp_Msg = new CTmTse_Message(lp_TM, pv_optns);

      if (lp_Msg)
      {
         lv_error = lp_Msg->ax_reg(&lv_extTransid, pv_rmid, pv_flags, pp_msgid);
         if (lv_error == FEOK)
            // Add to message list
            gv_TMTSELIB.insertMessage(lp_Msg);
      }
      else
         TMTSETrace(TMTSE_TraceError,("TMTSE: CTmTse_TM::ax_reg failed "
                    "to allocate new CTmTse_Message object.\n"));
   }

   TMTSETrace((lv_error?TMTSE_TraceAPIExitError:TMTSE_TraceAPIExit),
              ("TMTSE: REGISTERTRANSACTION EXIT returning %d.\n", lv_error));
   return lv_error;
} //REGISTERTRANSACTION_EXT


// ---------------------------------------------------------------
// GETTRANSACTIONNODE
// Purpose - Returns the TM node number for the specified 
// transaciton.
// 
// Return Codes:
//    FEOK           Success
// ---------------------------------------------------------------
extern "C" short GETTRANSACTIONNODE (int64 pv_transid, 
                                     int *pp_node)
{
   TM_Transid lv_transid;
   lv_transid = pv_transid;
   TM_Transid_Type lv_extTransid = lv_transid.get_data();
   
   return GETTRANSACTIONNODE_EXT(&lv_extTransid, pp_node);
} //GETTRANSACTIONNODE


// ---------------------------------------------------------------
// GETTRANSACTIONNODE_EXT
// Purpose - Returns the TM node number for the specified 
// extended transaction.
// 
// Return Codes:
//    FEOK           Success
// ---------------------------------------------------------------
extern "C" short GETTRANSACTIONNODE_EXT (TM_Transid_Type *pp_extTransid,
                                         int *pp_node)
{
   short lv_error = FEOK;
   TM_Transid lv_extTransid;
   lv_extTransid = *pp_extTransid;

   *pp_node = lv_extTransid.get_node();
   if (*pp_node < 0 || *pp_node > MAX_NODES)
   {
      lv_error = FENOTFOUND;
      TMTSETrace(TMTSE_TraceError,
                 ("TMTSE: GETTRANSACTIONNODE_EXT bad node=%d\n",
                  *pp_node));
   }

   TMTSETrace((lv_error?TMTSE_TraceAPIExitError:TMTSE_TraceAPIExit),
              ("TMTSE: GETTRANSACTIONNODE_EXT EXIT returning %d.\n", lv_error));
   return lv_error;
} //GETTRANSACTIONNODE_EXT


// ---------------------------------------------------------------
// GETTRANSACTIONSEQNUM
// Purpose - Returns the sequence number for the specified 
// transaciton.
// 
// Return Codes:
//    FEOK           Success
// ---------------------------------------------------------------
extern "C" short GETTRANSACTIONSEQNUM (int64 pv_transid, 
                                       int *pp_seqNum)
{
   TM_Transid lv_transid;
   lv_transid = pv_transid;
   TM_Transid_Type lv_extTransid = lv_transid.get_data();
   
   return GETTRANSACTIONSEQNUM_EXT(&lv_extTransid, pp_seqNum);
} //GETTRANSACTIONSEQNUM


// ---------------------------------------------------------------
// GETTRANSACTIONSEQNUM_EXT
// Purpose - Returns the sequence number for the specified 
// extended transaction.
// 
// Return Codes:
//    FEOK           Success
// ---------------------------------------------------------------
extern "C" short GETTRANSACTIONSEQNUM_EXT (TM_Transid_Type *pp_extTransid,
                                           int *pp_seqNum)
{
   short lv_error = FEOK;
   TM_Transid lv_extTransid;
   lv_extTransid = *pp_extTransid;

   *pp_seqNum = lv_extTransid.get_seq_num();

   TMTSETrace((lv_error?TMTSE_TraceAPIExitError:TMTSE_TraceAPIExit),
              ("TMTSE: GETTRANSACTIONSEQNUM_EXT EXIT returning %d.\n", lv_error));
   return lv_error;
} //GETTRANSACTIONSEQNUM_EXT


// ---------------------------------------------------------------
// TMTSE_GETTRANSINFO
// Purpose - Used to test whether transaction type bits are set
// 
// Return Codes:
//    true           Transaction type bits in pv_TTbits are set in
//                      transid provided by the XID
//    false          Transaction type bits are not set 
// ---------------------------------------------------------------
extern "C" bool TMTSE_GETTRANSINFO (XID  *pp_xid, 
                                    int64 pv_TTbits)
{
   TM_Txid_Internal *lv_extTransid;
   lv_extTransid = (TM_Txid_Internal *) pp_xid->data;// get the transid from XID

   if(pv_TTbits & lv_extTransid->iv_tt_flags.Application) 
   {
       return true;
   }
   else
   {
        return false;
   }
} //TMTSE_GETTRANSINFO


// ---------------------------------------------------------------
// TMTSE_GETTRANSFLAGS
// Purpose - Used to retrieve the TT flags from an xid.
// 
// Returns 64 bit Transaction type flags field.
// ---------------------------------------------------------------
extern "C" int64 TMTSE_GETTRANSFLAGS (XID  *pp_xid)
{
   int64 *lp_TTflags;
   TM_Txid_Internal *lp_extTransid;
   lp_extTransid = (TM_Txid_Internal *) pp_xid->data;// get the transid from XID

   lp_TTflags = (int64 *) &lp_extTransid->iv_tt_flags;
   return *lp_TTflags;
  
} //TMTSE_GETTRANSINFO


// ---------------------------------------------------------------
// TMTSE_GETREPLY
// Purpose - Retrieve the fields from a reply.  
// This is identical to TMTSE_GETREPLY_EXT, except that it returns a 
// 64 bit legacy transid in place of an extended one.
//
// NOTE: This function will call BMSG_BREAK_ the first time it is called
// for a message.  To ensure that BMSG_BREAK_ does not block, the caller 
// must ensure that they call XWAIT(LDONE) followed by BMSG_LISTEN_.
// 
// Return Codes:
//    FEOK           Success
//    FENOTFOUND     No associated message found. 
//    error returned by BMSG_BREAK_
// ---------------------------------------------------------------
extern "C" short TMTSE_GETREPLY(int pv_msgid, TMTSETYPE *pp_replyType, 
                                short *pp_replyError, int64 *pp_transid,
                                void *pp_data)
{

   TM_Native_Type *lp_transid_to_return = (TM_Native_Type *) pp_transid;
   TM_Transid lv_transid;
   
   short lv_error = TMTSE_GETREPLY_EXT(pv_msgid, pp_replyType, pp_replyError, 
                                       (TM_Transid_Type *) &lv_transid, pp_data);

   *lp_transid_to_return = lv_transid.get_native_type();

   return lv_error;
} //TMTSE_GETREPLY


// ---------------------------------------------------------------
// TMTSE_GETREPLY_EXT
// Purpose - Retrieve the fields from a reply.  
// This must be called after a BMSG_LISTEN_ has completed for the
// message specified by pv_msgid.  This function can be called as
// many times as needed up until TMTSE_COMPLETE is called.  Once
// TMTSE_COMPLETE has been called the message no longer exists and
// TMTSE_GETREPLY_EXT will return FENOTFOUND.
//
// NOTE: This function will call BMSG_BREAK_ the first time it is called
// for a message.  To ensure that BMSG_BREAK_ does not block, the caller 
// must ensure that they call XWAIT(LDONE) followed by BMSG_LISTEN_.
// 
// Return Codes:
//    FEOK           Success
//    FENOTFOUND     No associated message found. 
//    error returned by BMSG_BREAK_
//
// replyError:
//    FEOK           Success
//    FENOTFOUND     The TM does not know this transaction.
//    FEINVTRANSID   Transaction not found.
//                   or Transaction is not in active or beginning state.
//    FEDEVICEDOWNFORTMF   Could not initialize RM slot in TM.
// ---------------------------------------------------------------
extern "C" short TMTSE_GETREPLY_EXT(int pv_msgid, TMTSETYPE *pp_replyType, 
                                    short *pp_replyError, TM_Transid_Type *pp_extTransid,
                                    void *pp_data)
{
   short lv_error = FEOK;

   if (!gv_TMTSEInitialized)
   {
      lv_error = (int16) TMTSE_initialize();
      if (lv_error)
         return lv_error;
      // Even if we succeeded in initializing, we dont have a message, so return an error.
      TMTSETrace(TMTSE_TraceError, ("TMTSE: TMTSE_GETREPLY_EXT ERROR: TMTSE "
                 "Library was not initialized.\n"));
      return FENOTFOUND;
   }

   TMTSETrace(TMTSE_TraceAPI, ("TMTSE: TMTSE_GETREPLY_EXT ENTRY, msgid=%d\n", pv_msgid));

   CTmTse_Message *lp_Msg = gv_TMTSELIB.getMessage(pv_msgid);

   if (!lp_Msg)
   {
      TMTSETrace(TMTSE_TraceError, ("TMTSE: TMTSE_GETREPLY_EXT ERROR: Message not "
                 "found, msgid=%d\n. ", pv_msgid));
      return FENOTFOUND;
   }

   short lp_error = lp_Msg->msgBreak();

   if (lp_error != FEOK)
   {
      TMTSETrace(TMTSE_TraceError, ("TMTSE: TMTSE_GETREPLY_EXT ERROR: BMSG_BREAK_ "
                 "returned error %d, msgid=%d\n. ", lp_error, pv_msgid));
      return lp_error;
   }
   

   // set reply data
   if (pp_replyType)
      *pp_replyType = lp_Msg->replyType();
   if (pp_replyError)
      *pp_replyError = lp_Msg->getReplyError();
   if (pp_extTransid)
      memcpy(pp_extTransid, lp_Msg->transid(), sizeof(TM_Transid_Type));
   if (pp_data)
      lp_Msg->copyReplyData(pp_data);

   TMTSETrace(TMTSE_TraceAPIExit, ("TMTSE: TMTSE_GETREPLY_EXT EXIT, msgid=%d, reply error=%d\n",
              pv_msgid, lp_Msg->getReplyError()));
   
   return FEOK;
} //TMTSE_GETREPLY_EXT


// ---------------------------------------------------------------
// TMTSE_COMPLETE
// Purpose - Cleanup buffers.
//
// NOTE: This function will call BMSG_BREAK_ the first time it is called
// for a message if TMTSE_GETREPLY[_EXT] has not already been called.  
// To ensure that BMSG_BREAK_ does not block, the caller must
// ensure that they call XWAIT(LDONE) followed by BMSG_LISTEN_.
// 
// Return Codes:
//    FEOK           Success
//    FENOTFOUND     No associated message found. 
//    FEINVTRANSID   Failed to find a TM which owned the transaction or
//                   TM decided txn in wrong state for operation.
//    error returned by BMSG_BREAK_
//    error returned by CTmTse_Message::resend_ax_reg()
// ---------------------------------------------------------------
extern "C" short TMTSE_COMPLETE(int pv_msgid)
{
   short lv_error = FEOK;
   bool lv_redirected = false;

   if (!gv_TMTSEInitialized)
   {
      lv_error = (int16) TMTSE_initialize();
      if (lv_error)
         return lv_error;
      // Even if we succeeded in initializing, we dont have a message, so return an error.
      TMTSETrace(TMTSE_TraceError, ("TMTSE: TMTSE_COMPLETE ERROR: TMTSE "
                 "Library was not initialized.\n"));
      return FENOTFOUND;
   }

   TMTSETrace(TMTSE_TraceAPI, ("TMTSE: TMTSE_COMPLETE ENTRY, msgid=%d\n", pv_msgid));

   CTmTse_Message *lp_Msg = gv_TMTSELIB.getMessage(pv_msgid);

   if (!lp_Msg)
   {
      TMTSETrace(TMTSE_TraceError, ("TMTSE: TMTSE_COMPLETE ERROR: Message not "
                 "found, msgid=%d\n. ", pv_msgid));
      return FENOTFOUND;
   }

   short lv_breakError = lp_Msg->msgBreak();

   if (lv_breakError != FEOK)
   {
      TMTSETrace(TMTSE_TraceError, ("TMTSE: TMTSE_COMPLETE ERROR: BMSG_BREAK_ "
                 "returned error %d, msgid=%d\n. ", lv_breakError, pv_msgid));
      lv_error = gv_TMTSELIB.deleteMessage(pv_msgid);
      return lv_breakError;
   }

   // If this was an ax_reg reply save the Lead TM nid and incarnation number
   if (lp_Msg->replyType() ==  TMTSETYPE_REGISTERTRANSACTION)
   {
       switch (lp_Msg->getReplyError())
       {
       case FEWRONGID:
         {
           // Save the lead TM and replying TMs incarnation number.
           gv_TMTSELIB.LeadTM_nid(lp_Msg->Reply()->u.iv_ax_reg.iv_LeadTM_nid);
           // Need to try the Lead TM. If the resend is successful, error
           // FERETRY will be returned.  The TSE then needs to call listen and
           // then TMTSE_COMPLETE again until the error is not FERETRY.
           CTmTse_TM *lp_TM = gv_TMTSELIB.openTM(lp_Msg->nid());

           if (lp_Msg->Reply()->u.iv_ax_reg.iv_TM_incarnation_num > lp_TM->TM_incarnation_num())
           {
               TMTSETrace(TMTSE_TraceError, ("TMTSE: TMTSE_COMPLETE detected restarted TM old "
                          "instance %d, new instance %d, retrying ax_reg.\n",
                         lp_TM->TM_incarnation_num(), 
                         lp_Msg->Reply()->u.iv_ax_reg.iv_TM_incarnation_num));
               lp_TM->TM_incarnation_num(lp_Msg->Reply()->u.iv_ax_reg.iv_TM_incarnation_num);
               lv_error = lp_Msg->resend_ax_reg();
               lv_redirected = true;
           }
           else
           {   // Same instance of TM, so set error to indicate txn not found by TM
               TMTSETrace(TMTSE_TraceError, ("TMTSE: TMTSE_COMPLETE TM could not find transaction, "
                          "returning FENOTRANSID(75) old instance %d, new instance %d, TM returned FEWRONGID.\n",
                         lp_TM->TM_incarnation_num(), 
                         lp_Msg->Reply()->u.iv_ax_reg.iv_TM_incarnation_num));
               lv_error = FEINVTRANSID;
           }
           break;
         }
       case FEOK:
           // Save the lead TM and replying TMs incarnation number.
           gv_TMTSELIB.LeadTM_nid(lp_Msg->Reply()->u.iv_ax_reg.iv_LeadTM_nid);
           gv_TMTSELIB.openTM(lp_Msg->nid())->TM_incarnation_num(lp_Msg->Reply()->u.iv_ax_reg.iv_TM_incarnation_num);
           break;
       default:
           // Nothing to do here
           ;
       }
   }

   if (!lv_redirected)
       lv_error = gv_TMTSELIB.deleteMessage(pv_msgid);

   TMTSETrace((lv_error?TMTSE_TraceAPIExitError:TMTSE_TraceAPIExit),
              ("TMTSE: TMTSE_COMPLETE_EXT EXIT returning %d, msgid=%d.\n", 
               lv_error, pv_msgid));

   return lv_error;
} //TMTSE_COMPLETE
