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

#ifndef CTMTXBASE_H_
#define CTMTXBASE_H_

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/atomic.h"

#include "dtm/tmtransid.h"
#include "tmaudit.h"
#include "tmlibmsg.h"
#include "tmtxbranches.h"
#include "tmmap.h"
#include "tmdeque.h"
#include "tmsync.h"
#include "tmtxmsg.h"
#include "tmtxkey.h"
#include "tmtimer.h"
#include "tmpoolelement.h"
#include "tmpool.h"
#include "tmtxstats.h"
#include "tmtxthread.h"


// EID must be exactly 9 characters
const char EID_CTmTxBase[] = {"CTmTxBase"}; 

// NEW DTM errors
#define TM_NOTENDER 3001
#define TM_COMMITLOCKSHELD 3002
#define TM_ABORTLOCKSHELD 3003
#define TM_UNILATERALABORT 3004
#define TM_BEGINFAILEDWITHRM 3005

#define TM_TRANSID_BYTE_SIZE 32
#define TM_LEGACY_TRANSID_BYTE_SIZE 8

// timeout parameter values for eventQ_pop:
#define TX_EVENTQ_WAITFOREVER -1
#define TX_EVENTQ_NOWAIT -2

extern const char *ms_getenv_str(const char *pp_key);
extern void ms_getenv_bool(const char *pp_key, bool *pp_val);
extern const char *ms_getenv_int(const char *pp_key, int *pp_val);
extern int xaTM_initialize (bool pp_tracing, bool pv_tm_stats, CTmTimer *pp_tmTimer);
extern CTmPool<class T> * xaTM_RMMessagePool();
class CTxThread;

typedef enum {
    TX_BEGIN                =1001,
    TX_COMMIT,
    TX_ROLLBACK,
    TX_FORGET,
    TX_REDRIVE_COMMIT,
    TX_REDRIVE_ROLLBACK,
    TX_BEGIN_COMPLETE,
    // XARM only:
    TX_SUSPEND,
    TX_JOIN,
    TX_END,
    TX_PREPARE,
    TX_TERMINATE
} TX_EVENT;

typedef enum {
    TX_SUB_END          =1,
    TX_SUB_PREPARE_PH1  =2,
    TX_SUB_COMMIT_PH2   =3,
    TX_SUB_ROLLBACK     =4,
    TX_SUB_FORGET       =5,
    TX_SUB_START        =6
} TX_SUBSTATE_TYPE;

class TM_Audit;
class TM_Info;


typedef struct _tmtx_h_as_2 {
   int32                iv_tag;
   TM_Txid_Internal     iv_transid;
   bool                 iv_in_use;
}Tm_Tag_Transid;

class TM_TX_Info;
class CTmXaTxn;
class RM_Info_TSEBranch;

//
// Base class for Transaction and XARM subordinate branches.
//
class CTmTxBase :public virtual CTmPoolElement
{
    protected:
       TM_TX_TYPE       iv_txnType;
       union {
          TM_TX_Info * ip_Txn;
          CTmXaTxn * ip_xaTxn;
       } iv_txnObj;
       TM_Txid_Internal iv_transid;
       int32            iv_tag;
       TM_TX_STATE      iv_tx_state;
       int32            iv_ender_pid;
       int32            iv_ender_nid;
       SB_Atomic_Int    iv_num_active_partic;
       int32            iv_prepared_rms;
       int32            iv_cleanup_sem; // cleanup semaphore
       int32            iv_rm_wait_time;

       // to be flags soon
       bool             iv_in_use;
       bool             iv_incremented;
       bool             iv_mark_for_rollback;
       bool             iv_tm_aborted;
       bool             iv_tse_aborted;
       bool             iv_appl_aborted;
       bool             iv_heur_aborted;
       bool             iv_read_only;
       int32            iv_trace_level;
       bool             iv_use_ext_transid; // Currently unused
       bool             iv_written_in_cp;
       bool             iv_wrote_trans_state;
       // Transaction was created from scan trails during a recovery cycle but one or more
       // RMs failed to respond to xa_commit/rollback.
       bool             iv_recovering; 

       // iv_transactionBusy flag is set when the transaction object
       // is processing a request which alters the transaction state
       // and so requires exclusive access to the transaction object.
       // An example of this type of request is ENDTRANSACTION.
       // These requests are processed by a transaction/worker thread
       // and typically span multiple waits.  New requests which require
       // exclusive access will be queued to the iv_pendingRequestQ
       // if iv_transactionBusy is true.
       bool        iv_transactionBusy;

       // Transaction abort timeout
       int32            iv_timeout; //-1 = wait forever, >0 = timeout in seconds

       Ctimeval         iv_beginTime;     // Timestamp of BEGINTRANSACTION
       CTmTimerEvent *  ip_timeoutEvent;
       CTmTimerEvent *  ip_hung_event;    // Set if there is an outstanding hung timer event.

       CTxThread *ip_Thread;             // Thread in use for this transaction.
       bool iv_threadPending;            // True when the thread is starting

       TM_Mutex        *ip_mutex;  // Semaphore to serialize updates to the object.

       // iv_Qmutex is used by Qlock and Qunlock to serialize changes to the transactionBusy
       // state with writes to the PendingRequestQ.
       TM_Mutex         iv_Qmutex; 
       CTxThread * ip_Qlock_owner;       // Thread which owns the lock, when object is locked
       
       // Client/TMLib reply related
       CTmTxMessage * ip_currRequest;

       // List of all participating branches - includes TSEs and HBase branches
       CTmTxBranches *ip_branches;

       // Queue of events against this transaction.
       // Note that this is against the transaction, not the thread object 
       // because a thread might change with time.
       TM_DEQUE      iv_eventQ; 
       SB_Thread::CV iv_CV; // Condition variable for controling the event queue

       // PendingRequest queue contains requests which could not be 
       // processed because the transaction was already working on a request
       // which alters the transaction state across a number of wait states.  
       // This is used in conjunction with the iv_transactionBusy flag.
       TM_DEQUE    iv_PendingRequestQ;  

       // The application participation list is keyed by (nid, pid).
       TM_MAP iv_app_partic_list;

       // Statistics timers & counters
       CTmTxStats iv_stats;

       // Private member functions
       int      initialize_slot (int32 pv_rmid);

    public:
       CTmTxBase(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, int32 pv_seq, 
                  int32 pv_pid, int32 pv_rm_wait_time);
       CTmTxBase(){iv_in_use = false;}
       ~CTmTxBase();

       static CTmTxBase *constructPoolElement(int64 pv_seqnum); // Not implemented by base class
       virtual int64 cleanPoolElement();

       void lock() {ip_mutex->lock();}
       void unlock() {ip_mutex->unlock();}

       // For serializing changes to the transactionBusy flag and writes to PendingRequestQ
       void Qlock() {iv_Qmutex.lock();}
       void Qunlock() {iv_Qmutex.unlock();}

       int32 trace_level() {return iv_trace_level;}
       
       virtual void initialize(int32 pv_nid, int64 pv_flags, int32 pv_trace_level, 
                               int32 pv_seq, int32 pv_creator_nid, int32 pv_creator_pid, 
                               int32 pv_rm_wait_time);
       virtual int safe_initialize_slot (int32 pv_rmid);
       virtual void initialize_tx_rms(bool pv_partic_true);
       virtual RM_Info_TSEBranch * get_rm(int32 pv_rmid);

       virtual short register_branch(int32 pv_rmid, CTmTxMessage * pp_msg);

       void sync_write (int32 pv_nid, int32 pv_pid, TM_TX_STATE pv_state);

       virtual int32 internal_abortTrans(bool pv_takeover = false);
       virtual int32 redrive_rollback();
       virtual int32 redrive_commit();
       virtual void schedule_redrive_sync();


       virtual void cleanup();  //Finished with this transaction object, prepare for reuse.

       // Event queue management methods
       TM_DEQUE * eventQ();
       void eventQ_push(CTmTxMessage * pp_msg, bool pv_threadAssociated=false);
       void eventQ_push_top(CTmTxMessage * pp_msg);
       CTmTxMessage * eventQ_pop(int pv_timeout=TX_EVENTQ_WAITFOREVER);

       virtual void process_eventQ(); 
       virtual void schedule_eventQ();

       void PendingRequestQ_push(CTmTxMessage * pp_msg);
       bool queueToTransaction(TM_Txid_Internal *pp_transid, CTmTxMessage * pp_msg);
       bool txn_object_good(TM_Txid_Internal *pp_transid, CTmTxMessage * pp_msg);
       
       virtual bool associate_tx(CTmTxMessage *pp_msg);
       virtual void recover_tx(TM_MSG_TYPE pv_type);

       virtual int32 schedule_abort();
       virtual short doom_txn();

       // event specific methods
       virtual bool req_begin(CTmTxMessage *pp_msg);
       virtual bool req_begin_complete(CTmTxMessage *pp_msg);
       virtual bool req_end(CTmTxMessage *pp_msg);
       virtual bool req_end_complete(CTmTxMessage *pp_msg = NULL);
       virtual bool req_forget(CTmTxMessage *pp_msg = NULL);
       virtual bool req_abort(CTmTxMessage *pp_msg);
       virtual bool req_abort_complete(CTmTxMessage *pp_msg = NULL);
       virtual bool rollback_txn(CTmTxMessage *pp_msg);
       virtual bool redriverollback_txn(CTmTxMessage *pp_msg);
       virtual bool redrivecommit_txn(CTmTxMessage *pp_msg);
       // Implemented in base class
       virtual bool req_status(CTmTxMessage *pp_msg);
       virtual bool req_ax_reg(CTmTxMessage *pp_msg);
       virtual bool req_join(CTmTxMessage *pp_msg);
       virtual bool req_suspend(CTmTxMessage *pp_msg);
       virtual bool req_registerRegion(CTmTxMessage *pp_msg);
       virtual bool req_ddloperation(CTmTxMessage *pp_msg, char *ddlbuffer);

       virtual int mapErr(short pv_tmError) = 0;

       // cleanup semaphore methods
       bool dec_cleanup();
       void inc_cleanup();

       // Methods to handle the iv_transactionBusy flag
       bool set_transactionBusy(CTmTxMessage * pp_msg);
       bool reset_transactionBusy(bool pv_cleanupPending = false);
       void reply_to_queuedRequests(short pv_error);

       // Timer event methods
       CTmTimerEvent * addTimerEvent(TM_MSG_TYPE pv_type, int pv_delayInterval);

       // Get/Set methods
   
       CTmTxBranches *branches() {return ip_branches;}
       bool add_app_partic (int32 pid, int32 nid);
       void cleanup_app_partic();
       bool remove_app_partic (int32 pid, int32 nid);
       bool erase_app_partic (CTmTxKey *key);
       bool is_app_partic (int32 pid, int32 nid);

       int32 ender_nid() {return iv_ender_nid;}
       void  ender_nid(int32 pv_nid) 
       {
          lock();
          iv_ender_nid = pv_nid;
          ip_mutex->unlock();
       }
       int32 ender_pid() {return iv_ender_pid;}
       void  ender_pid(int32 pv_pid) 
       {
          lock();
          iv_ender_pid = pv_pid;
          ip_mutex->unlock();
       }
       bool is_ender(int32 pv_nid, int32 pv_pid)
       {
         return ((iv_ender_nid == pv_nid && iv_ender_pid == pv_pid)?true:false);
       }
       int64 timestamp() {return iv_transid.iv_timestamp;}
       int32 num_active_partic()
       {
          int32 lv_rtn = iv_num_active_partic.read_val();
          return lv_rtn;
       }
       void inc_active_partic() 
       {
          iv_num_active_partic.add_val(1);
       }
       void dec_active_partic()
       {
          iv_num_active_partic.sub_val(1);
       }
       int32 prepared_rms() {return iv_prepared_rms;}
       void inc_prepared_rms() 
       {
          lock();
          iv_prepared_rms++;
          ip_mutex->unlock();
       }
       void dec_prepared_rms()
       {
          lock();
          iv_prepared_rms--;
          ip_mutex->unlock();
       }

        bool in_use() {return iv_in_use;}
        void in_use(bool pv_in_use)
        {
           lock();
           iv_in_use = pv_in_use;
           ip_mutex->unlock();
        }
        bool recovering() {return iv_recovering;}
        void recovering(bool pv_recovering)
        {
           lock();
           iv_recovering = pv_recovering;
           ip_mutex->unlock();
        }
       void rm_wait_time (int32 pv_rm_wait_time) {iv_rm_wait_time = pv_rm_wait_time;}
       int32 rm_wait_time () {return iv_rm_wait_time;}
       int32 tag() {return iv_tag;}
       void tag(int32 pv_tag) 
       {
          lock();
          iv_tag = pv_tag;
          ip_mutex->unlock();
       }
       int32 timeout() {return iv_timeout;}
       void timeout(int32 pv_timeout) 
       {
          lock();
          iv_timeout = pv_timeout;
          ip_mutex->unlock();
       }
       CTmTimerEvent * timeoutEvent() {return ip_timeoutEvent;}
       void timeoutEvent(CTmTimerEvent * pp_timeoutEvent) 
       {
          lock();
          ip_timeoutEvent = pp_timeoutEvent;
          ip_mutex->unlock();
       }
       void setAbortTimeout(int32 pv_timeout);
       void addHungTimerEvent(TM_MSG_TYPE pv_type);
       

       int64 TT_flags() 
       {
          union 
          {
             TM_TT_Flags iv_flags;
             int64 iv_int;
          } u;

          u.iv_flags = iv_transid.iv_tt_flags;
          return u.iv_int;
       }

       void TT_flags(int64 pv_tt_flags) 
       {
          union 
          {
             TM_TT_Flags iv_flags;
             int64 iv_int;
          } u;

          u.iv_int = pv_tt_flags;
          // Locking removed as it causes a deadly embrace when an ax_reg(no undo)
          // arrives while we're processing a commit/rollback.
          //lock();
          iv_transid.iv_tt_flags = u.iv_flags;
          //unlock();
       }

       TM_Txid_Internal * transid() { return &iv_transid; }
       void transid(TM_Txid_Internal *pp_transid)
       {
          lock();
          memcpy(&iv_transid, pp_transid, TM_TRANSID_BYTE_SIZE);
          ip_mutex->unlock();
       }
       int64 legacyTransid() 
       {
           union {
               TM_Txid_Internal iv_txid;
               int64 iv_int[4];
           } u;

           memcpy (&u.iv_txid, &iv_transid, TM_TRANSID_BYTE_SIZE);
           return u.iv_int[0];
       }
       int32 seqnum() {return iv_transid.iv_seq_num;}
       int32 node() {return iv_transid.iv_node;}
       bool read_only() {return iv_read_only;}
       void read_only(bool pv_value) {iv_read_only=pv_value;}
       bool mark_for_rollback() {return iv_mark_for_rollback;}
       void mark_for_rollback(bool pv_value) {iv_mark_for_rollback=pv_value;}
       bool tm_aborted() {return iv_tm_aborted;}
       bool tse_aborted() {return iv_tse_aborted;}
       int32 abort_flags() 
       {
           return (iv_tm_aborted * TM_TX_ABORTFLAGS_OFFSET_TM +
                   (iv_tse_aborted * TM_TX_ABORTFLAGS_OFFSET_TSE) + 
                   (iv_appl_aborted * TM_TX_ABORTFLAGS_OFFSET_APPL) + 
                   (iv_heur_aborted * TM_TX_ABORTFLAGS_OFFSET_HEUR));
       }
       void abort_flags(int32 pv_flags) 
       {
           iv_tm_aborted = pv_flags & TM_TX_ABORTFLAGS_OFFSET_TM;
           iv_tse_aborted = pv_flags & TM_TX_ABORTFLAGS_OFFSET_TSE;
           iv_appl_aborted = pv_flags & TM_TX_ABORTFLAGS_OFFSET_APPL;
           iv_heur_aborted = pv_flags & TM_TX_ABORTFLAGS_OFFSET_HEUR;
       }
       int get_TSEBranchesParticCount() { return branches()->TSE()->num_rm_partic(); }
       int get_TSEBranchesUnresolvedCount(){ return branches()->TSE()->num_rms_unresolved();}

       RM_Info_TSEBranch *TSEBranch(int32 pv_index) { return branches()->TSE()->get_slot(pv_index); }

       TM_TX_STATE tx_state(){return iv_tx_state;}
       void tx_state(TM_TX_STATE pv_tx_state)
       {
          lock();
          iv_tx_state = pv_tx_state;
          ip_mutex->unlock();
       }
       bool tx_hung()
       {
          if (tx_state() == TM_TX_STATE_HUNGCOMMITTED ||
              tx_state() == TM_TX_STATE_HUNGABORTED)
             return true;
          else
             return false;
       }
       void hung_redrive();
       bool wrote_trans_state() {return iv_wrote_trans_state;}
       void wrote_trans_state(bool pv_state)
       {
          iv_wrote_trans_state = pv_state;
       }
       CTxThread * thread() {return ip_Thread;}
       void thread(CTxThread * pp_thread)
       {
          lock();
          ip_Thread = pp_thread;
          ip_mutex->unlock();
       }
       bool threadPending() {return iv_threadPending;}
       void threadPending(bool pv_set) {iv_threadPending = pv_set;}

       bool transactionBusy() {return iv_transactionBusy;}
       void transactionBusy(bool pv_setting)
       {
          lock();
          iv_transactionBusy = pv_setting;
          ip_mutex->unlock();
       }
       bool isAborting()
       {
          if (tx_state() == TM_TX_STATE_ABORTING ||
              tx_state() == TM_TX_STATE_ABORTING_PART2 ||
              tx_state() == TM_TX_STATE_ABORTED ||
              tx_state() == TM_TX_STATE_HUNGABORTED ||
              abort_flags() ||
              iv_mark_for_rollback)
             return true;
          else
             return false;
       }
       // display cleanup semaphore (don't change it)
       int32 list_cleanup() { return iv_cleanup_sem;}

       // cleaning_up is true if the transaction object has
       // been marked for cleanup.  We use this to determine whether
       // new requests can be queued to the transaction.
       bool cleaning_up() { return (iv_cleanup_sem > 0)?true:false; }

       TM_DEQUE * PendingRequestQ() {return &iv_PendingRequestQ;}

       CTmTxMessage * currRequest() {return ip_currRequest;}
       void currRequest(CTmTxMessage * pp_msg);

       void delete_currRequest() 
       {
          delete ip_currRequest;
          ip_currRequest = 0;
       }

       CTmTxStats *stats() {return &iv_stats;}

       Ctimeval *beginTime() {return &iv_beginTime;}
};

#define TXSTATS(exp) \
   {if (stats()->collectStats()) \
   stats()->exp;}

#endif //CTMTXBASE_H_




