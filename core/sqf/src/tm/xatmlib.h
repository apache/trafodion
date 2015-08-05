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

#ifndef XATMLIB_H_
#define XATMLIB_H_
#include "rm.h"
#include "tmmmap.h"
#include "xatmglob.h"
#include "tmtime.h"
#include "tmpool.h"
#include "xaglob.h"


// Forward declarations
class CxaTM_RM;
class CxaTM_RMMessage;
class CTmTimer;
template<class T> class CTmPool;


// XA TM Class. This represents the Transaction 
// Manager.  There is one per XA TM Library.
class CxaTM_TM
{
public:
   // Public member variables

private:
   // Private member variables
   bool iv_initialized;          // true after first call to xaTM_initialize
   int iv_my_nid;                // nid and pid of the process the library is running in.
   int iv_my_pid;                //
   int64 iv_next_msgNum;
   int32 iv_lastMonError;        // Last error returned by an attempt to open a subordinate RM.
   XATM_TraceMask iv_traceMask;  // XA TM tracing Mask.  0 = no tracing (default)
   bool iv_tm_stats;             // Collect statistics.

   TM_MAP ia_rmList;             // Each element is a CxaTM_RM
   TM_MMAP ia_txnMsgList;        // Each element is a CxaTM_RMMessage indexed 
                                 // by txn sequence number (non-unique).

   TM_Mutex       *ip_mutex;  // Semaphore to serialize updates to the object.

#ifndef XARM_BUILD_
   CTmTimer *ip_tmTimer;         // Timer thread object pointer
#endif


   // RM Message object management:
   // RM Messages are used by the XATM Library module for all communications with RMs.
   // Each RM object maintains a list of active messages in addition to the RMMessagePools
   // inUseList.
   CTmPool<class CxaTM_RMMessage> *ip_RMMessagePool;

   int32            iv_RMmsgTotal;          // Total RM message objects allocated
   int32            iv_RMmsgMax;            // Maximum possible RM message objects allowed.
   int32            iv_RMmsgSteadyLow;      // Lower steady-state limit for RM message objects.
   int32            iv_RMmsgSteadyHigh;     // Upper steady-state limit for RM message objects.
   int32            iv_RMmsgPoolThresholdEventCounter;
                                            // Counts the number of times we
                                            // exceed the Steadystate high.


public:
   // Public methods:
   CxaTM_TM();
   ~CxaTM_TM();
   int newRM(int pv_rmid, CxaTM_RM **ppp_RM);
   void deleteRM(CxaTM_RM *pp_RM);
   int setAndGetNid();
   int initialize(XATM_TraceMask pv_traceMask, bool pv_tm_stats, CTmTimer *pp_tmTimer);
   static void initalizePhandle(SB_Phandle_Type *pp_phandle);
   static void setPhandle(SB_Phandle_Type *pp_destPhandle, SB_Phandle_Type *pp_sourcePhandle);
   static int mapMonErr_To_xaErr(int32 pv_error);
   bool xaTrace(XATM_TraceMask pv_traceMask);
   void setxaTrace(XATM_TraceMask pv_traceMask);      // Set XATM tracing.
   bool tm_stats() {return iv_tm_stats;}

   // Initialized
   bool initialized() {return iv_initialized;}
#ifndef XARM_BUILD_
   CTmTimer *tmTimer() {return ip_tmTimer;}
   void tmTimer(CTmTimer *pp_timer) {ip_tmTimer = pp_timer;}
#endif

   // Helper methods for managing RM Message object pool
   int64 next_msgNum() {return iv_next_msgNum;}
   int64 inc_next_msgNum() {return iv_next_msgNum++;}
   CTmPool<CxaTM_RMMessage> * RMMessagePool() {return ip_RMMessagePool;}
   CxaTM_RMMessage * new_RMmsg();
   void release_RMmsg(CxaTM_RMMessage * pp_msg);

   // TM getRM
   // Retrieves an RM object based on the supplied rmid.
   // If the rmid is not found, then 0 is returned.
   CxaTM_RM * getRM(int pv_rmid)
   {
      return (CxaTM_RM *) ia_rmList.get(pv_rmid);
   }

   // inuse RMMessage object list by transaction sequence number
   TM_MMAP *txnMsgList() {return &ia_txnMsgList;}
   void insert_txnMsgList(XID *pp_xid, CxaTM_RMMessage * pp_msg);
   void cancelAll(int32 pv_count, int32 pv_numMsgs, int64 pv_transid);
   bool check_msgIntegrity(int64 pv_transid, CxaTM_RMMessage * pp_msg);
       
   // Set/Get methods
   int my_nid() {return iv_my_nid;}
   void my_nid(int pv_nid) {iv_my_nid = pv_nid;}
   int my_pid() {return iv_my_pid;}
   void my_pid(int pv_pid) {iv_my_pid = pv_pid;}

   void traceMask(XATM_TraceMask pv_traceMask) {iv_traceMask = pv_traceMask;}
   XATM_TraceMask traceMask() {return iv_traceMask;}
   void lastMonError(int32 pv_lastMonError) {iv_lastMonError=pv_lastMonError;}
   int32 lastMonError() {return iv_lastMonError;}

private:
   // Private methods:
   void lock();
   void unlock();
};

// XA RM Class.  This represents the RM.  
// There is one per xa_open() call.
class CxaTM_RM
{
public:
   // Public member variables
private:
   // Private member variables
   char iv_tmName[MAXPROCESSNAME]; // Name of TM process associated with this RM (XARM only).
   int32 iv_rmid;                // RM identifier
   TM_MAP iv_msgList;            // List of outstanding messages for this RM

   bool iv_recoverEnd;           // true indicates we received the final buffer of xids.

   // xarm open values
   char iv_xarmName[RMNAMESZ];     // XA RM name from the xa_open xainfo struct.
   int64 iv_xarmFlags;       
   char ia_xarmInfo[MAXINFOSIZE]; // content of xa_open xa_info parameter.

   int iv_maxSendRetries;        // Maximum times a message can be resent to 
                                 // this RM before transaction is declared HUNG.
   uint32 iv_totalRetries;       // The total number of times the TM has attempted to send
                                 // unsuccessfully to this RM. This is used to stop flooding the 
                                 // event log with warnings.
   SB_Phandle_Type iv_phandle;
   TM_Mutex *ip_mutex;  // Semaphore to serialize updates to the object.


public:
   // Public utility methods
   CxaTM_RM();
    CxaTM_RM(int pv_rmid);
   ~CxaTM_RM();
    bool getRmReplyOpenAx_Reg();
    int32 getRmReplyOpenOpener();
    bool getRmReplyPreparePartic();
    int32 getRmReplyType();
    int getRmid();
    SB_Phandle_Type *getRmPhandle();
    char * getRMname() {return (char *) &iv_xarmName;}
    short validateRMname(const char * pp_info, char * pp_xarmName);
    char *getRMnameDetail() 
    {
        RM_Open_struct *lp_info = (RM_Open_struct *) &ia_xarmInfo;
        return (char *) &lp_info->process_name;
    }
    void xarmName(char *pp_rm) {memset(iv_xarmName, 0, RMNAMESZ);
                                strncpy((char *) &iv_xarmName, pp_rm, RMNAMESZ-1);}
    char * xarmName() {return (char *) &iv_xarmName;}
    void xarmFlags(int64 pv_flags) {iv_xarmFlags = pv_flags;}
    int64 xarmFlags() {return iv_xarmFlags;}
    void xarmInfo(char *pp_rm) {memset(ia_xarmInfo, 0, MAXINFOSIZE);
                                strncpy(ia_xarmInfo, pp_rm, MAXINFOSIZE-1);}
    char * xarmInfo() {return (char *) ia_xarmInfo;}

   // RM outstanding message list methods:
   TM_MAP * msgList() {return &iv_msgList;}
   void cleanup_msgList(); //release all messages on the list

   int maxSendRetries() {return iv_maxSendRetries;}
   int totalRetries() {return iv_totalRetries;}
   bool inc_totalRetries();

   // xa implementation methods
   int close(char *info, int64 pv_flags);
   int commit(XID *pp_xid, int64 pv_flags);
   int end(XID *pp_xid, int64 pv_flags);
   int forget(XID *pp_xid, int64 pv_flags);
   int open(char * pp_name, int64 pv_flags);
   int start(XID *pp_xid, int64 pv_flags);
   int prepare(XID *pp_xid, int64 pv_flags);
   int rollback(XID *pp_xid, int64 pv_flags);
   int recoverSend(int64 pv_count, int64 pv_flags, int pv_node, bool pv_dead_node, int pv_index);

   int send_rm(CxaTM_RMMessage * pp_msg);
   void lock();
   void unlock();

private:
   // Private methods
   int setRmPhandle(char *pp_name);
};

   extern CxaTM_TM gv_xaTM;      // One global xaTM object
   extern const char *ms_getenv_str(const char *pp_key);
   extern const char *ms_getenv_int(const char *pp_key, int *pp_val);
   extern char * XIDtoa(XID *pp_xid);

#endif //XATMLIB_H_

