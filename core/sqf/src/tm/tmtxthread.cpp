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

#include <stdlib.h>
#include "seabed/thread.h"
#include "tmtxthread.h"
#include "tmlogging.h"
#include "tmtx.h"
#include "tminfo.h"
#include "seabed/trace.h"
#include "hbasetm.h"

// Globals



//----------------------------------------------------------------------------
// CTxThread::CTxThread   
// Purpose : Constructor
//----------------------------------------------------------------------------
CTxThread::CTxThread(SB_Thread::Sthr::Function pv_fun, int64 pv_num, const char *pp_name)
   :CTmThread(pv_fun, pv_num, pp_name),iv_id(0)
{
   TMTrace(2, ("CTxThread::CTxThread: ENTRY thread object %p(%s)\n",
               (void *) this, pp_name));
   //EID(EID_CTxThread);
   delete_exit(false);
   set_daemon(true);
   reset();
}

//----------------------------------------------------------------------------
// CTxThread::~CTxThread   
// Purpose : Destructor
//----------------------------------------------------------------------------
CTxThread::~CTxThread()
{
   TMTrace(2, ("CTxThread::~CTxThread : ENTRY.\n"));
   ip_txn = NULL;
   int lv_error = gv_HbaseTM.detachThread();
   if (lv_error)
   {
      TMTrace(1,("CTxThread::~CTxThread: CHbaseTM::detachThread failed with error %d.\n", lv_error));
      tm_log_event(DTM_HBASE_DETACHTHREAD_FAILED, SQ_LOG_CRIT, "DTM_HBASE_DETACHTHREAD_FAILED", lv_error);
      abort();
   }
}


//----------------------------------------------------------------------------
// CTxThread::reset
// Purpose : reset values in this thread object.
//----------------------------------------------------------------------------
void CTxThread::reset()
{
   ip_txn = NULL;
   iv_state = TM_TX_TH_STATE_IDLE;
} //CTxThread::reset


//----------------------------------------------------------------------------
// CTxThread::constructPoolElement
// Purpose : Callback for CTmPool elements.
// This method is called to construct a CTxThread object by CTmPool::newElement.
//----------------------------------------------------------------------------
CTxThread * CTxThread::constructPoolElement(int64 pv_threadNum)
{
   char lv_name[20];

   sprintf(lv_name, "workTh" PFLL "", pv_threadNum);
   TMTrace (2, ("CTxThread::constructPoolElement : ENTRY Instantiating new txn thread object %s.\n", 
                (char *) &lv_name));

   CTxThread *lp_Thread = new CTxThread(txThread_main, pv_threadNum, (const char *) &lv_name);
   if (lp_Thread)
      lp_Thread->start();
   else
   {
      tm_log_event(DTM_LOGIC_ERROR, SQ_LOG_CRIT, "DTM_LOGIC_ERROR",
                   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,lv_name);
      TMTrace (1, ("CTxThread::constructPoolElement : Failed to instantiate transaction thread object.\n"));
      abort();
   }

   TMTrace (2, ("CTxThread::constructPoolElement : EXIT Thread object %p(%s) instantiated.\n", 
                (void *) lp_Thread, (char *) &lv_name));
   return lp_Thread;
} //CTxThread::constructPoolElement


//----------------------------------------------------------------------------
// CTxThread::cleanPoolElement
// Purpose : Callback for CTmPool elements.
// This method is called to clean a CTxThread object when CTmPool::newElement
// allocated it from the freeList.
// Returns the threadNum which is reused to add the thread object to the 
// CTmPool<CTxThread> inUseList.
//----------------------------------------------------------------------------
int64 CTxThread::cleanPoolElement()
{
   reset();
   return threadNum();
} //CTxThread::cleanPoolElement


//----------------------------------------------------------------------------
// CTxThread::associate
// Purpose : Associate a transaction object with this thread.
//----------------------------------------------------------------------------
void CTxThread::associate(TM_TX_Info * pp_txn)
{
   TM_TX_Info * lp_disassociatedTxn = NULL;

   TMTrace(2, ("CTxThread::associate ENTRY, txn tag=%d\n", pp_txn->tag()));

   lock();
   ip_txn = pp_txn;
   ip_txn->thread(this);
   ip_txn->threadPending(false);
   iv_state = TM_TX_TH_STATE_ACTIVE;
   unlock();

   // Check the disassociatedTxn list for this transaction and remove any entries
   // if present.  There could be more than one.  We now have an association.
   int lv_count = 0;
   gv_tm_info.txdisassociatedQ()->lock();
   lp_disassociatedTxn = (TM_TX_Info *) gv_tm_info.txdisassociatedQ()->get_firstFIFO();

   while (lp_disassociatedTxn != NULL) {
      // If we found the transaction in the list, remove it.
      if (pp_txn == lp_disassociatedTxn)
      {
         lv_count++;
         TMTrace(3, ("CTxThread::associate : Txn ID (%d,%d) found on disassociated "
                           "transaction list, deleting it (entry %d).\n", 
                           pp_txn->node(), pp_txn->seqnum(), lv_count));
         gv_tm_info.txdisassociatedQ()->erase(); //delete txn from list
      }
      lp_disassociatedTxn = (TM_TX_Info *) gv_tm_info.txdisassociatedQ()->get_nextFIFO();
   }
   gv_tm_info.txdisassociatedQ()->unlock();

   TMTrace(2, ("CTxThread::associate EXIT. Removed %d entries from disassociatedTxnQ\n", lv_count));
}

//----------------------------------------------------------------------------
// CTxThread::disassociate
// Purpose : Disassociate the transaction object from this thread.
// Returns true if the transaction was successfully disassociated from the 
//              thread.
//         false if the transaction still has queued events outstanding. This
//               will only happen when using worker threads.
//----------------------------------------------------------------------------
bool CTxThread::disassociate()
{
   bool lv_cleanedup = false;
   TM_TX_Info *lp_disassociatedTxn = NULL;

   TMTrace(2, ("CTxThread::disassociate ENTRY, ID %d, Thread %s(%ld)\n", 
      ip_txn->seqnum(), get_name(), get_id()));
      
   if(ip_txn == NULL) {
      TMTrace (1, ("CTxThread::disassociate : ERROR! ip_txn is NULL\n"));
      return false;
   }
 
   // Grab the transactions Qmutex here to make sure no one can queue new requests against
   // this transaction until we've finished disassociating.
   ip_txn->Qlock();

   // Now that we've locked the event queues, check to see if they're empty
   // This can happen when a request arrives and is queued between reset_transactionBusy
   // completing and disassociate being called.
   // Note this only applies to worker threads.  Transaction threads will be finished when
   // we disassociate.
   if (!ip_txn->eventQ()->empty() || !ip_txn->PendingRequestQ()->empty())
   {
      ip_txn->Qunlock();
      if (ip_txn->eventQ()->empty())
      {
         // Check the PendingRequest queue
         CTmTxMessage *lp_msg = (CTmTxMessage *) ip_txn->PendingRequestQ()->pop_end();
         if (lp_msg)
         {
            TMTrace (3, ("CTxThread::disassociate : ID %d"
                           " request popped off PendingRequestQ for msgid(%d).\n", 
                           ip_txn->seqnum(), lp_msg->msgid()));

            ip_txn->queueToTransaction(ip_txn->transid(), lp_msg);
         }
         else
         {
            tm_log_event(DTM_LOGIC_ERROR_NO_ENTRY, SQ_LOG_CRIT, "DTM_LOGIC_ERROR_NO_ENTRY",
                         -1,-1,-1,ip_txn->seqnum());
            TMTrace (1, ("CTxThread::disassociate : LOGIC ERROR! ID %d "
                            "Expected to find an entry in eventQ or PendingRequestQ "
                            "for txn.\n", ip_txn->seqnum()));
            abort();
         }
      }
      TMTrace(2, ("CTxThread::disassociate EXIT returning false(events queued, not disassociated)\n"));
      return false;
   }

   lock();

   // Save the transaction object pointer so that we can unlock later.
   if(ip_txn != NULL)
      lp_disassociatedTxn = ip_txn;

   // This would indicate that we are aborting a transaction on
   // behalf of another TM.  We do not have an internal rep for
   // this, so don't try to delete it
   if (ip_txn != NULL && ip_txn->dec_cleanup() && ip_txn->tag() != -1)
   {
      ip_txn->Qunlock();
      gv_tm_info.cleanup(ip_txn);
      // Can't use ip_txn after cleanup because it may have been deleted!
      ip_txn = NULL;
      lv_cleanedup = true;
   }
   else
      if (gv_tm_info.iv_trace_level >= 3)
      {
         //There's a possibility that ip_txn could be NULL here, so check
         //for that before accessing fields within it.
         if (ip_txn == NULL)
            trace_printf("CTxThread::disassociate not cleaning up. Null Tx\n");
         else
            trace_printf("CTxThread::disassociate not cleaning up. Tx cleanup flag %d.\n", 
                      ip_txn->list_cleanup());
      }

   // For worker threads we need to clear the thread pointer from the transaction object
   // each time we disassociate.
   if (gv_tm_info.threadModel() == worker && ip_txn != NULL && ip_txn->thread())
      ip_txn->thread(NULL);
   
   ip_txn = NULL;
   iv_state = TM_TX_TH_STATE_IDLE;

   unlock();
   // Release the transaction event queues.
   if (lv_cleanedup == false)
      lp_disassociatedTxn->Qunlock();

   TMTrace(2, ("CTxThread::disassociate EXIT returning true(disassociated)\n"));
   return true;
} //disassociate


// --------------------------------------------------------------
// eventQ_push
// Purpose - push a new event to the threads event queue.
// These are pushed in FIFO order.
// --------------------------------------------------------------
void CTxThread::eventQ_push(CTmEvent *pp_event)
{
    Tm_Req_Msg_Type *lp_req = pp_event->request();

    if (lp_req == NULL)
    {
        tm_log_event(DTM_THRD_INVALID_MSG_TYPE, SQ_LOG_CRIT, "DTM_THRD_INVALID_MSG_TYPE");
        TMTrace(1, ("CTxThread::eventQ_push - Request to be queued is NULL.\n")); 
        abort ();
    }    

   eventQ()->push(pp_event);

   TMTrace(3, ("CTxThread::eventQ_push : signaling thread, event %d.\n",
                   lp_req->iv_msg_hdr.rr_type.request_type));
         
   eventQ_CV()->signal(true /*lock*/);
} //CTxThread::eventQ_push


// --------------------------------------------------------------
// eventQ_push_top
// Purpose - push an event to the top of the queue.  
// This is used for TM internal messages where they must be 
// processed before anything else like an Initialize.
// --------------------------------------------------------------
void CTxThread::eventQ_push_top(CTmEvent *pp_event)
{
    Tm_Req_Msg_Type *lp_req = pp_event->request();

    if (lp_req == NULL)
    {
        tm_log_event(DTM_THRD_INVALID_MSG_TYPE, SQ_LOG_CRIT, "DTM_THRD_INVALID_MSG_TYPE");
        TMTrace(1, ("CTxThread::eventQ_push_top - Request to be queued is NULL.\n"));
        abort ();
    }    

   eventQ()->push_back(pp_event);

   TMTrace(3, ("CTxThread::eventQ_push_top : signaling thread, event %d.\n",
                   lp_req->iv_msg_hdr.rr_type.request_type));

   eventQ_CV()->signal(true /*lock*/);
} //CTxThread::eventQ_push_top


// --------------------------------------------------------------
// eventQ_pop
// Purpose - pop an event from the end of the queue.  Events are
// always processed in FIFO order.
// Remember to dispose of the returned event once finished with
// it!
// --------------------------------------------------------------
CTmEvent * CTxThread::eventQ_pop()
{
   CTmEvent *lp_event = NULL;

   while (eventQ()->empty())
   {
      TMTrace(3, ("CTxThread::eventQ_pop event Q empty, waiting for signal.\n"));
      eventQ_CV()->wait(true /*lock*/);
   }

   lp_event = (CTmEvent *) eventQ()->pop_end();
   TMTrace(3, ("CTxThread::eventQ_pop Thread %s returning event %d.\n",
                   get_name(), lp_event->request()->iv_msg_hdr.rr_type.request_type));

   return lp_event;
} //CTxThread::eventQ_pop

//----------------------------------------------------------------------------
// txThread_main
// Purpose : Main for transaction threads
//----------------------------------------------------------------------------
void * txThread_main(void *arg)
{
   char la_buf[DTM_STRING_BUF_SIZE];
   char            *lp_threadname = SB_Thread::Sthr::self_name();
   CTmEvent        *lp_event;
   CTxThread       *lp_thread = NULL;
   bool             lv_exit = false;
   bool             lv_disassociated = false;

   arg = arg;

   TMTrace(2, ("txThread_main : ENTRY for thread %s.\n", lp_threadname));

   //Waiting for the threadpool to be unlocked to continue
   gv_tm_info.threadPool()->lock();
   gv_tm_info.threadPool()->unlock();

   lp_thread = gv_tm_info.get_thread(lp_threadname);

   if (lp_thread == NULL)
   {
      tm_log_event(DTM_THRD_INVALID_THREAD, SQ_LOG_CRIT, "DTM_THRD_INVALID_THREAD");
      TMTrace(1, ("txThread_main : DEATH, couldn't find my thread %s!\n", lp_threadname));
      fflush(stdout);
      abort ();
   }   

   lp_thread->set_id(SB_Thread::Sthr::self_id());
   lp_thread->state(TM_TX_TH_STATE_IDLE);

   // Attach the thread to the JVM for HBase TM LIbrary.
   int lv_error = gv_HbaseTM.initJVM();
   if (lv_error)
   {
      TMTrace(1,("txThread_main: CHbaseTM::attachThread failed with error %d.\n", lv_error));
      tm_log_event(DTM_HBASE_ATTACHTHREAD_FAILED, SQ_LOG_CRIT, "DTM_HBASE_ATTACHTHREAD_FAILED", lv_error);
      abort();
   }
   else
      TMTrace(1,("CTxThread::CTxThread: CHbaseTM::attachThread succeeded for thread " PFLL "(%s).\n", lp_thread->threadNum(), lp_threadname));

   while (!lv_exit)
   {
      lp_event = lp_thread->eventQ_pop();

      switch (lp_event->requestType())
      {
      case TM_MSG_TXTHREAD_INITIALIZE:
        {
         if (lp_thread->state() == TM_TX_TH_STATE_IDLE)
         {
            TM_TX_Info *lp_tx = (TM_TX_Info *) lp_event->request()->u.iv_init_txthread.ip_txObject;
            TMTrace(3, ("txThread_main : Initialize Thread Obj %p(%s), Txn Obj %p, ID %d\n",
               (void *) lp_thread, lp_thread->get_name(),
               (void *) lp_tx, lp_tx->seqnum()));
            lp_thread->associate(lp_tx);
            do
            {
               lp_tx->process_eventQ();
               lv_disassociated = lp_thread->disassociate();
            } while (!lv_disassociated);
         }
         else
         {
            lv_exit = true;
            // EMS DTM_TXTHREAD_BAD_STATE
            sprintf(la_buf, "Txn Thread %s main received Initialize event but not in "
                    "Idle state, terminating.\n", lp_threadname);
            TMTrace(3, ("txThread_main : %s", la_buf));
            tm_log_event(DTM_TXTHREAD_BAD_STATE, SQ_LOG_CRIT, "DTM_TXTHREAD_BAD_STATE",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    -1, /*data */
                    -1, /*data1*/
                    -1,/*data2 */
                    lp_threadname);
         }
         break;
        } //case INITIALIZE

      case TM_MSG_TXTHREAD_RELEASE:
         // Nothing to do here as we aren't using this right now.
         // If we receive a release here while in idle state, we just ignore it.
         // EMS DTM_TXTHREAD_BAD_EVENT_RELEASE
         sprintf(la_buf, "Txn Thread %s main received an unexpected Release event "
                 "in idle state.\n", lp_threadname);
         TMTrace(3, ("txThread_main : %s", la_buf));
         tm_log_event(DTM_TXTHREAD_BAD_EVENT_RELEASE, SQ_LOG_CRIT,
                     "DTM_TXTHREAD_BAD_EVENT_RELEASE",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    -1, /*data */
                    -1, /*data1*/
                    -1,/*data2 */
                    lp_threadname);
         break;
         
      case TM_MSG_TXTHREAD_TERMINATE:
         TMTrace(3, ("txThread_main : Thread %s processing terminate request.\n", 
                         lp_threadname));
         lv_exit = true;
         // If there was an associated transaction, delete it now
         if (lp_thread->transaction() != NULL)
            gv_tm_info.remove_tx(lp_thread->transaction());
         break;

      default:
         lv_exit = true;
         // EMS DTM_TXTHREAD_BAD_EVENT
         TMTrace(3, ("txThread_main : Txn Thread %s main received an unexpected event %d, "
                  "terminating.\n", lp_threadname, lp_event->requestType()));
        tm_log_event(DTM_TXTHREAD_BAD_EVENT, SQ_LOG_CRIT,
                     "DTM_TXTHREAD_BAD_EVENT",
                    -1, /*error_code*/ 
                    -1, /*rmid*/
                    -1, /*dtmid*/ 
                    -1, /*seq_num*/
                    -1, /*msgid*/
                    -1, /*xa_error*/
                    -1, /*pool_size*/
                    -1, /*pool_elems*/
                    -1, /*msg_retries*/
                    -1, /*pool_high*/
                    -1, /*pool_low*/
                    -1, /*pool_max*/
                    -1, /*tx_state*/
                    lp_event->requestType(), /*data */
                    -1, /*data1*/
                    -1,/*data2 */
                    lp_threadname);
      } //switch

      //Now that we're done with the event, dispose of it.
      delete lp_event;
 
      // Check to see if we have pending work for any transactions not 
      // currently associated with a thread. If there are no transaction
      // objects with queued events waiting for a thread, then release this
      // thread back to the pool.
      if (lv_exit == false)
         lv_exit = gv_tm_info.release_thread(lp_thread);
   } //while

   gv_tm_info.threadPool()->dec_totalElements();
   TMTrace(2, ("txThread_main : EXIT for thread %p(%s). Thread terminating. %d txn "
      "threads still active.\n", 
      (void *) lp_thread, lp_threadname, gv_tm_info.threadPool()->totalElements()));

   return NULL;
} //txThread_main



