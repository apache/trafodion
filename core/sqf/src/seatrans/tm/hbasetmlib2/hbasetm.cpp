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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

#include "seabed/trace.h"
#include "dtm/tm_util.h"
#include "tmtxkey.h"
#include "tmlogging.h"
#include "hbasetm.h"
#include "javaobjectinterfacetm.h"

// Externals
using namespace std;

// Globals
CHbaseTM gv_HbaseTM;
HBASETM_TraceMask gv_HBASETM_traceMask;  // HBase TM tracing Mask.  0 = no tracing (default)

#define SQ_LIO_SIGNAL_REQUEST_REPLY (SIGRTMAX - 4)
static int tm_rtsigblock_proc() {
    sigset_t lv_sig_set;
    // Setup signal handling
    sigemptyset(&lv_sig_set);
    sigaddset(&lv_sig_set, SQ_LIO_SIGNAL_REQUEST_REPLY);
    int err = pthread_sigmask(SIG_BLOCK, &lv_sig_set, NULL);
    if (err)
        abort();
    return 0;
}

short HBasetoTxnError(short pv_HBerr) 
{
   switch (pv_HBerr)
   {
   case RET_OK: return FEOK;
   case RET_NOTX: return FENOTRANSID;
   case RET_READONLY: return FEOK; //Read-only reply is ok
   case RET_ADD_PARAM: return FEBOUNDSERR;
   case RET_EXCEPTION: return FETRANSEXCEPTION;
   case RET_HASCONFLICT: return FELOCKED; //Change to FEHASCONFLICT?
   case RET_IOEXCEPTION: return FETRANSIOEXCEPTION;
   case RET_NOCOMMITEX: return FEABORTEDTRANSID;
   default: return FETRANSERRUNKNOWN;
   }
}

// ---------------------------------------------------------------
// HbaseTM_initialize
// Purpose - Initialize HBase-trx in a client.
// ---------------------------------------------------------------
int HbaseTM_initialize (short pv_nid)
{
   int lv_error = gv_HbaseTM.initialize(pv_nid);
   return lv_error;
}


// ---------------------------------------------------------------
// HbaseTM_initialize (2)
// Purpose - Initialize the HBase-trx TM Library.
// ---------------------------------------------------------------
int HbaseTM_initialize (bool pp_tracing, bool pv_tm_stats, CTmTimer *pp_tmTimer, short pv_nid)
{
   int lv_error = 0;
   union {
      HBASETM_TraceMask lv_traceMask;
      int lv_traceMaskInt;
   } u;

   //initialize trace file
   char * lp_traceString = (char *) ms_getenv_str("HBASETM_TRACE");
   char * lp_traceStringEnd;
   if (pp_tracing != 0 && lp_traceString)
   {
      lp_traceStringEnd = lp_traceString + strlen(lp_traceString);

      if (lp_traceStringEnd == NULL)
      {
        // tm_log_event(DTM_XMAPI_INVALID_STRING_SIZE, SQ_LOG_CRIT, "DTM_XMAPI_INVALID_STRING_SIZE");
        // Make sure the lp_traceStringEnd pointer points to the null terminator.
        abort();
      }

      //Convert hexadecimal string to int
      unsigned long lv_traceMaskul = strtoul(lp_traceString, &lp_traceStringEnd, 16); 
      u.lv_traceMaskInt = (int) lv_traceMaskul;
   }
   else
      u.lv_traceMask = HBASETM_TraceOff;

   lv_error = gv_HbaseTM.initialize(u.lv_traceMask, pv_tm_stats, pp_tmTimer, pv_nid);


   HBASETrace((lv_error?HBASETM_TraceAPIExitError:HBASETM_TraceAPIExit),
           (HDR ": HbaseTM_initialize EXIT returning %d\n", lv_error));
   return lv_error;
} // HbaseTM_initialize

//----------------------------------------------------------------
// HbaseTM_initiate_cp
// Purpose - Initiate control point
//----------------------------------------------------------------
void HbaseTM_initiate_cp()
{
   gv_HbaseTM.addControlPoint();
}

//----------------------------------------------------------------
// HbaseTM_initiate_stall
// Purpose - Initiate stall in phase 2
//----------------------------------------------------------------
void HbaseTM_initiate_stall(int where)
{
   cout << "HbaseTM_initiate_stall called with parameter " << where << "\n";
   gv_HbaseTM.stall(where);
}

//----------------------------------------------------------------
// HbaseTM_process_request_regions_info
// Purpose: Retrieve region info for dtmci
//---------------------------------------------------------------
HashMapArray* HbaseTM_process_request_regions_info()
{
    cout << "Start HBaseTM_process_request_regions\n";
    HashMapArray* mapArrayRegions = gv_HbaseTM.requestRegionInfo();
    return mapArrayRegions;
}

// =============================================================
// ===== Class CHbaseTM
// =============================================================
jclass CHbaseTM::javaClass_ = 0;

// CHbaseTM Methods
// TM Default Constructor
// CHbaseTM Methods
// TM Default Constructor
CHbaseTM::CHbaseTM() : JavaObjectInterfaceTM()
{
   tm_rtsigblock_proc();  
   // cout << "CHbaseTM::CHbaseTM() called\n";
   // Mutex attributes: Recursive = true, ErrorCheck=false
   // ip_mutex = new TM_Mutex(true, false);
   iv_tm_stats = false;

   lock();
   iv_initialized = false;
   my_nid(-1);     // Indicates that the node hasn't been set yet.
   iv_traceMask = HBASETM_TraceOff;
   ip_tmTimer = NULL;
   iv_next_msgNum = 1;

   // Need to check return and handle error
   int lv_result = initJVM();
   if(lv_result != JOI_OK) {
      tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::CHbaseTM()-1", (char *) _tlp_error_msg->c_str(), -1LL);
      abort();
   }
   unlock();
}

// TM Destructor
CHbaseTM::~CHbaseTM() 
{
   //delete ip_mutex;
   HBASETrace(HBASETM_TraceExit, (HDR ": CHbaseTM::~CHbaseTM Exit.\n"));
}

int CHbaseTM::initJVM() 
{
  JavaMethods_[JM_CTOR       ].jm_name      = "<init>";
  JavaMethods_[JM_CTOR       ].jm_signature = "()V";
  JavaMethods_[JM_INIT1      ].jm_name      = "init";
  JavaMethods_[JM_INIT1      ].jm_signature = "(S)Z";
  JavaMethods_[JM_BEGIN      ].jm_name      = "beginTransaction";
  JavaMethods_[JM_BEGIN      ].jm_signature = "(J)J";
  JavaMethods_[JM_ABORT      ].jm_name      = "abortTransaction";
  JavaMethods_[JM_ABORT      ].jm_signature = "(J)S";
  JavaMethods_[JM_PRECOMMIT  ].jm_name      = "prepareCommit";
  JavaMethods_[JM_PRECOMMIT  ].jm_signature = "(J)S";
  JavaMethods_[JM_DOCOMMIT   ].jm_name      = "doCommit";
  JavaMethods_[JM_DOCOMMIT   ].jm_signature = "(J)S";
  JavaMethods_[JM_TRYCOMMIT  ].jm_name      = "tryCommit";
  JavaMethods_[JM_TRYCOMMIT  ].jm_signature = "(J)S";
  JavaMethods_[JM_COMPLETEREQUEST].jm_name      = "completeRequest";
  JavaMethods_[JM_COMPLETEREQUEST].jm_signature = "(J)S";
  JavaMethods_[JM_REGREGION  ].jm_name      = "callRegisterRegion";
  JavaMethods_[JM_REGREGION  ].jm_signature = "(JJI[BJ[B)S";
  JavaMethods_[JM_PARREGION  ].jm_name      = "participatingRegions";
  JavaMethods_[JM_PARREGION  ].jm_signature = "(J)I";
  JavaMethods_[JM_CNTPOINT   ].jm_name      = "addControlPoint";
  JavaMethods_[JM_CNTPOINT   ].jm_signature = "()J";
  JavaMethods_[JM_STALL      ].jm_name      = "stall";
  JavaMethods_[JM_STALL      ].jm_signature = "(I)S";
  JavaMethods_[JM_NODEDOWN   ].jm_name      = "nodeDown";
  JavaMethods_[JM_NODEDOWN   ].jm_signature = "(I)V";
  JavaMethods_[JM_NODEUP     ].jm_name      = "nodeUp";
  JavaMethods_[JM_NODEUP     ].jm_signature = "(I)V";
  JavaMethods_[JM_CREATETABLE ].jm_name      = "callCreateTable";
  JavaMethods_[JM_CREATETABLE ].jm_signature = "(J[B[Ljava/lang/Object;)S";
  JavaMethods_[JM_ALTERTABLE ].jm_name      = "callAlterTable";
  JavaMethods_[JM_ALTERTABLE ].jm_signature = "(J[B[Ljava/lang/Object;)S";
  JavaMethods_[JM_REGTRUNCABORT].jm_name      = "callRegisterTruncateOnAbort";
  JavaMethods_[JM_REGTRUNCABORT].jm_signature = "(J[B)S";
  JavaMethods_[JM_DROPTABLE ].jm_name      = "callDropTable";
  JavaMethods_[JM_DROPTABLE ].jm_signature = "(J[B)S";
  JavaMethods_[JM_RQREGINFO  ].jm_name      = "callRequestRegionInfo";
  JavaMethods_[JM_RQREGINFO  ].jm_signature = "()Lorg/trafodion/dtm/HashMapArray;";

  char className[]="org/trafodion/dtm/HBaseTxClient";
  return (HBTM_RetCode)JavaObjectInterfaceTM::init(className, javaClass_, (JavaMethodInit*)&JavaMethods_, (int)JM_LAST, false);
}

//////////////////////////////////////////////
//                 JNI Methods              //
//////////////////////////////////////////////
short CHbaseTM::initConnection(short pv_nid)
{
  jshort   jdtmid = pv_nid;
  jboolean jresult = _tlp_jenv->CallBooleanMethod(javaObj_, JavaMethods_[JM_INIT1].methodID, jdtmid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::initConnection()", (char *) _tlp_error_msg->c_str(), -1LL);
     return RET_EXCEPTION;
  }
  if (jresult == false)
  {
    return RET_LAST;
  }

  return RET_OK;
}

short CHbaseTM::addControlPoint() {
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::addControlPoint()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  jlong jresult = _tlp_jenv->CallLongMethod(javaObj_, JavaMethods_[JM_CNTPOINT].methodID);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::addControlPoint()", (char *)_tlp_error_msg->c_str(), -1LL);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }
  _tlp_jenv->PopLocalFrame(NULL);
  if (jresult == 0L) {
    return RET_EXCEPTION;
  }
  return RET_OK;
}

short CHbaseTM::beginTransaction(int64 *pp_transid) {
  jlong  jlv_transid = *pp_transid;
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::beginTransaction()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  jlong jresult = _tlp_jenv->CallLongMethod(javaObj_, JavaMethods_[JM_BEGIN].methodID, jlv_transid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::beginTransaction()", (char *)_tlp_error_msg->c_str(), *pp_transid);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }
  *pp_transid = (long)jresult;

  _tlp_jenv->PopLocalFrame(NULL);
  return RET_OK;
}

short CHbaseTM::abortTransaction(int64 pv_transid) {
  jlong  jlv_transid = pv_transid;
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::abortTransaction()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  jshort jresult = _tlp_jenv->CallShortMethod(javaObj_, JavaMethods_[JM_ABORT].methodID, jlv_transid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::abortTransaction()", (char *)_tlp_error_msg->c_str(), pv_transid);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }
  _tlp_jenv->PopLocalFrame(NULL);
  //  RET_NOTX means the transaction wasn't found by the HBase client code (trx).  This is ok here, it
  //  simply means the transaction hasn't been seen by the HBase client code, so no work was done on it.
  if (jresult == RET_NOTX)
  {
    return RET_OK;
  } 
  return jresult;
}


short CHbaseTM::prepareCommit(int64 pv_transid , char *errstr, int &errstrlen) {
  jlong   jlv_transid = pv_transid;
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::prepareCommit()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  jshort jresult = _tlp_jenv->CallShortMethod(javaObj_, JavaMethods_[JM_PRECOMMIT].methodID, jlv_transid);
  if (getExceptionDetails(NULL, (short*)&lv_joi_retcode)) {
      int errMsgLen = (int)_tlp_error_msg->length();
      errstrlen = ((errMsgLen < errstrlen) && (errMsgLen > 0)) ? errMsgLen  : errstrlen -1;
      strncpy(errstr, _tlp_error_msg->c_str(), errstrlen);
      errstr[errstrlen] = '\0';
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR,(char*)"CHbaseTM::prepareCommit()", (char *)_tlp_error_msg->c_str(), pv_transid);
     
     //If an exception is received, it is only due to some issue.
     //Exception can come back with error code set or not set.
     //If there is an error code passed as part of exception, pass this
     //back to my caller. Else return RET_EXCEPTION; 
     if(lv_joi_retcode != JOI_OK)
       jresult = lv_joi_retcode;
     else
     {
       _tlp_jenv->PopLocalFrame(NULL);
       return RET_EXCEPTION;
     }
  }

  _tlp_jenv->PopLocalFrame(NULL);
  if (jresult <= RET_LAST) {
     return jresult;
  }

  // For building
  if(pv_transid) 
    return RET_OK;

  return RET_OK;
}


short CHbaseTM::doCommit(int64 pv_transid) {
  jlong   jlv_transid = pv_transid;
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::doCommit()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  jshort jresult = _tlp_jenv->CallShortMethod(javaObj_, JavaMethods_[JM_DOCOMMIT].methodID, jlv_transid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::doCommit()", (char *)_tlp_error_msg->c_str(), pv_transid);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }

  _tlp_jenv->PopLocalFrame(NULL);
  if (jresult == 1)
  {
    // jresult from abort java method - 1 is error
    return RET_ADD_PARAM;
  }


  // For building
  if(pv_transid) {
    return RET_OK;
  }
  return RET_OK;
}


short CHbaseTM::tryCommit(int64 pv_transid) {
  jlong   jlv_transid = pv_transid;
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::tryCommit()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  jshort jresult = _tlp_jenv->CallShortMethod(javaObj_, JavaMethods_[JM_TRYCOMMIT].methodID, jlv_transid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::tryCommit()", (char *)_tlp_error_msg->c_str(), pv_transid);
     return RET_EXCEPTION;
  }

  _tlp_jenv->PopLocalFrame(NULL);
  //  RET_NOTX means the transaction wasn't found by the HBase client code (trx).  This is ok here, it
  //  simply means the transaction hasn't been seen by the HBase client code, so no work was done on it.
  if (jresult == RET_NOTX)
  {
    return RET_OK;
  } 

  return jresult;
}


short CHbaseTM::completeRequest(int64 pv_transid) {
  jlong   jlv_transid = pv_transid;
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::completeRequest()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  jshort jresult = _tlp_jenv->CallShortMethod(javaObj_, JavaMethods_[JM_COMPLETEREQUEST].methodID, jlv_transid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::completeRequest()", (char *)_tlp_error_msg->c_str(), pv_transid);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }

  _tlp_jenv->PopLocalFrame(NULL);
  if (jresult == 1)
  {
    // jresult from abort java method - 1 is error
    return RET_ADD_PARAM;
  }
  // For building
  if(pv_transid) {
    return RET_OK;
  }
  return RET_OK;
}


// TM lock semaphore
void CHbaseTM::lock()
{
   /*
   HBASETrace(HBASETM_TraceLock, (HDR ": CHbaseTM::lock, count %d, owner %ld\n", 
           ip_mutex->lock_count(), ip_mutex->lock_owner()));

   int lv_error = ip_mutex->lock();
   if (lv_error)
   {
      //HBASETrace(HBASETM_TraceError, (HDR ": CHbaseTM::lock returned error %d.\n", lv_error));
      //::abort();
   }
   */
}


// CHbaseTM::setAndGetNid
// Get the node number
// The first time this is called it will retrieve the value from
// the Monitor.
inline int CHbaseTM::setAndGetNid()
{
   lock();
   if (my_nid() == -1)
   {
      msg_mon_get_process_info(NULL, &iv_my_nid, &iv_my_pid);
   }
   unlock();
   return my_nid();
} //setAndGetNid


// CHbaseTM::initialize
// Initialize the CHbaseTM object
// 
int CHbaseTM::initialize(short pv_nid)
{
    return initialize(HBASETM_TraceOff, false, NULL, pv_nid);
}


// CHbaseTM::initialize
// Initialize the CHbaseTM object
// 
int CHbaseTM::initialize(HBASETM_TraceMask pv_traceMask, bool pv_tm_stats, CTmTimer *pp_tmTimer, short pv_nid)

{
   int lv_error = 0;

   setTrace(pv_traceMask);
   lock();
   //   gv_HbaseTM.setTrace(pv_traceMask);
   iv_tm_stats = pv_tm_stats;

   if (pp_tmTimer != NULL) 
   {
      ip_tmTimer = pp_tmTimer;
      //gv_startTime = ip_tmTimer->startTime();
   }
   else 
   {
      ip_tmTimer = NULL;
      //gv_startTime = Ctimeval::now();
   }
   lv_error = initConnection(pv_nid);
   if (lv_error)
   {
      printf("CHbaseTM::initialize: CHbaseTM::initConnection failed with error %d.\n", lv_error);
      HBASETrace(HBASETM_TraceError,
                 (HDR "CHbaseTM::initialize: CHbaseTM::initConnection failed with error %d.\n", lv_error));
      //tm_log_event(DTM_HBASE_INIT_FAILED, SQ_LOG_CRIT, "DTM_HBASE_INIT_FAILED", lv_error);
      abort();
   }
   else
      HBASETrace(HBASETM_TraceDetail,
                 (HDR "CHbaseTM::initialize: CHbaseTM::initConnection succeeded.\n"));

   if (lv_error)
      abort();
   else
      iv_initialized = true;
   unlock();

   HBASETrace(HBASETM_TraceExitError,
              (HDR "CHbaseTM::initialize EXIT returning %d.\n", lv_error));
   return lv_error;
} //CHbase::initialize


// TM unlock semaphore
void CHbaseTM::unlock()
{
   /*
   HBASETrace(HBASETM_TraceLock, (HDR ": CHbaseTM::unlock, count %d, owner %ld\n", 
           ip_mutex->lock_count(), ip_mutex->lock_owner()));

    int lv_error = ip_mutex->unlock();
   if (lv_error)
   {
      HBASETrace(HBASETM_TraceError, (HDR ": CHbaseTM::unlock returned error %d.\n", lv_error));
      abort();
   }
   */
}


// TM Trace 
// Returns the value of iv_trace.
bool CHbaseTM::Trace(HBASETM_TraceMask pv_traceMask)
{
   return ((pv_traceMask & iv_traceMask)? true: false);
}

// TM setTrace
// Sets the value of iv_traceMask.
// Note that because this is a mask it is concatenated to the mask unless
// set to 0. Ie:
// If pv_traceMask == 0, set iv_traceMask = 0
// If pv_traceMask > 0, iv_traceMask |= pv_traceMask; (bit-wise OR)
void CHbaseTM::setTrace(HBASETM_TraceMask pv_traceMask)
{
   HBASETM_TraceMask iv_OldMask = iv_traceMask;

   lock();
   if (pv_traceMask == HBASETM_TraceOff)
      iv_traceMask = HBASETM_TraceOff;
   else
      iv_traceMask = (HBASETM_TraceMask) (iv_traceMask | pv_traceMask);

   gv_HBASETM_traceMask = iv_traceMask;
   unlock();

   // Don't use HBASETrace here as we always want to write the trace record.
   if (iv_OldMask != HBASETM_TraceOff && pv_traceMask == HBASETM_TraceOff)
      trace_printf(HDR ":Tracing off.\n");
   else
      if (pv_traceMask != HBASETM_TraceOff)
         trace_printf(HDR ": Tracing on, Mask=0x%x.\n", iv_traceMask);
} //Trace


//----------------------------------------------------------------------------
// CHbaseTM::registerRegion
// Purpose  : Register a region for the specified transaction.
//----------------------------------------------------------------------------
int CHbaseTM::registerRegion(int64 pv_transid,
                             int64 pv_startid,
 			     int pv_port,
 			     const char pa_hostname[],
 			     int pv_hostname_Length,
			     long pv_startcode,
 			     const char *pa_regionInfo,
 			     int pv_regionInfo_Length)

{
   int lv_error = FEOK;
   jlong  jlv_transid = pv_transid;
   jlong  jlv_startid = pv_startid;
   jint jiv_port = pv_port;
   CTmTxKey lv_tid(pv_transid);
   HBASETrace(HBASETM_TraceAPI, (HDR "CHbaseTM::registerRegion : Txn ID (%d,%d), startid %ld, hostname: %s.\n",
				 lv_tid.node(), lv_tid.seqnum(), jlv_startid, pa_hostname));
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::registerRegion()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  jbyteArray jba_hostname = _tlp_jenv->NewByteArray(pv_hostname_Length+1);
  if (jba_hostname == NULL) {
    _tlp_jenv->PopLocalFrame(NULL);
    return RET_ADD_PARAM;
  }
  _tlp_jenv->SetByteArrayRegion(jba_hostname, 0, pv_hostname_Length, (const jbyte*) pa_hostname);

  jbyteArray jba_regionInfo = _tlp_jenv->NewByteArray(pv_regionInfo_Length);
  if (jba_regionInfo == NULL) {
    _tlp_jenv->PopLocalFrame(NULL);
    return RET_ADD_PARAM;
  }

  _tlp_jenv->SetByteArrayRegion(jba_regionInfo, 0, pv_regionInfo_Length, (const jbyte*)pa_regionInfo);

//  printf("JavaObjectInterfaceTM::registerRegion calling client callRegisterRegion with trans: %ld and startid: %ld\n", jlv_transid, jlv_startid);
  lv_error = _tlp_jenv->CallShortMethod(javaObj_,
					JavaMethods_[JM_REGREGION].methodID,
					jlv_transid,
					jlv_startid,
					jiv_port,
					jba_hostname,
					pv_startcode,
					jba_regionInfo);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::registerRegion()", (char *)_tlp_error_msg->c_str(), pv_transid);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }

  HBASETrace(HBASETM_TraceExit, (HDR "CHbaseTM::registerRegion : Error %d, Txn ID (%d,%d), hostname %s.\n", 
                lv_error, lv_tid.node(), lv_tid.seqnum(), pa_hostname));

  _tlp_jenv->PopLocalFrame(NULL);
  return lv_error;
} //CHbaseTM::registerRegion


//----------------------------------------------------------------------------
// CHbaseTM::recoverRegion
// Purpose  : Recover a region for the specified transaction.
// pp_count :input Maximum number of transids to be returned in a single reply
//               :output Number of transids returned
// Replies will contain transaction lists for a single region only.  It is assumed that all transactions for a region will be contained
// within the array pointed to by pp_transidList.
// pp_flags as per xa_recover flags:
//    TMSTARTRSCAN 0x01000000LL /* start a recovery scan */
//    TMENDRSCAN 0x00800000LL /* end a recovery scan */
//    TMRESENDRSCAN 0x00010000LL 
//    TMRMFAILRSCAN 0x00001000LL 
//    When TMRESENDRSCAN is specified pp_region must contain a valid region name.  This is used to retrieve indoubt transactions
//    for a specific region following a region failure.
//----------------------------------------------------------------------------
int CHbaseTM::recoverRegion(int64 *pp_count, int64 *pp_transidList[], int64 *pp_flags)
{
   int lv_error = FEOK;
   HBASETrace(HBASETM_TraceAPI, (HDR "CHbaseTM::recoverRegion : Count " PFLL ", flags " PFLLX ".\n", 
                 *pp_count, *pp_flags));
   //!@# TODO
   HBASETrace(HBASETM_TraceExit, (HDR "CHbaseTM::recoverRegion : Error %d, Count " PFLL ", flags " PFLLX ".\n", 
                 lv_error, *pp_count, *pp_flags));
   return lv_error;
} //CHbaseTM::recoverRegion


//----------------------------------------------------------------------------
// CHbaseTM::failedRegions
// Purpose : Returns the number of failed regions for
// the specified transaction.
// Currently not supported.
//----------------------------------------------------------------------------
int CHbaseTM::failedRegions(int64 pv_transid)
{
   int lv_failedCount = 0;
   CTmTxKey lv_transid(pv_transid);
   HBASETrace(HBASETM_TraceAPI, (HDR "CHbaseTM::failedRegions : Txn ID (%d,%d).\n", lv_transid.node(), lv_transid.seqnum()));

   HBASETrace(HBASETM_TraceExit, (HDR "CHbaseTM::failedRegions : Count %d.\n", lv_failedCount));
   return lv_failedCount;
} //CHbaseTM::failedRegions


//----------------------------------------------------------------------------
// CHbaseTM::participatingRegions
// Purpose : Returns the number of failed regions for
// the specified transaction.
// Currently yet supported.
//----------------------------------------------------------------------------
int CHbaseTM::participatingRegions(int64 pv_transid)
{
   jlong lv_transid_j = pv_transid;
   jint  lv_participating = 0; 
   int lv_participatingCount = 0;
   CTmTxKey lv_transid(pv_transid);
   HBASETrace(HBASETM_TraceAPI, (HDR "CHbaseTM::participatingRegions : Txn ID (%d,%d).\n", lv_transid.node(), lv_transid.seqnum()));

   JOI_RetCode lv_joi_retcode = JOI_OK;
   lv_joi_retcode = initJNIEnv();
   if (lv_joi_retcode != JOI_OK) {
      tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::participatingRegions()", (char *)_tlp_error_msg->c_str(), -1LL);
      abort();
   }

   lv_participating = _tlp_jenv->CallIntMethod(javaObj_, JavaMethods_[JM_PARREGION].methodID, lv_transid_j);
   if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::participatingRegions()", (char *)_tlp_error_msg->c_str(), pv_transid);
      lv_participatingCount = -1;
   }
   _tlp_jenv->PopLocalFrame(NULL);

   lv_participatingCount = lv_participating;

   HBASETrace(HBASETM_TraceExit, (HDR "CHbaseTM::participatingRegions : Count %d.\n", lv_participatingCount));
   return lv_participatingCount;
} //CHbaseTM::participatingRegions


//----------------------------------------------------------------------------
// CHbaseTM::unresolvedRegions
// Purpose : Returns the number of failed regions for
// the specified transaction.
// Currently yet supported.
//----------------------------------------------------------------------------
int CHbaseTM::unresolvedRegions(int64 pv_transid)
{
   int lv_unresolvedCount = 0;
   CTmTxKey lv_transid(pv_transid);
   HBASETrace(HBASETM_TraceAPI, (HDR "CHbaseTM::unresolvedRegions : Txn ID (%d,%d).\n", lv_transid.node(), lv_transid.seqnum()));

   HBASETrace(HBASETM_TraceExit, (HDR "CHbaseTM::unresolvedRegions : Count %d.\n", lv_unresolvedCount));
   return lv_unresolvedCount;
} //CHbaseTM::unresolvedRegions

short CHbaseTM::stall(int where){
  jint jiv_where = where;
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::stall()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  cout << "CHbaseTM::stall called with: " << jiv_where << "\n";
  _tlp_jenv->CallShortMethod(javaObj_, JavaMethods_[JM_STALL].methodID, jiv_where);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::stall()", (char *)_tlp_error_msg->c_str(), -1LL);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  } 
  _tlp_jenv->PopLocalFrame(NULL);
  return RET_OK;
}

short CHbaseTM::nodeDown(int32 nid){
  jint jiv_nid = nid;
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::nodeDown()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  cout << "CHbaseTM::nodeDown called with nodeId " << jiv_nid << "\n";
  _tlp_jenv->CallShortMethod(javaObj_, JavaMethods_[JM_NODEDOWN].methodID, jiv_nid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::nodeDown()", (char *)_tlp_error_msg->c_str(), -1LL);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }
  _tlp_jenv->PopLocalFrame(NULL);
  return RET_OK;
}


short CHbaseTM::nodeUp(int32 nid){
  jint jiv_nid = nid;
  JOI_RetCode lv_joi_retcode = JOI_OK;
  lv_joi_retcode = initJNIEnv();
  if (lv_joi_retcode != JOI_OK) {
    tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::nodeUp()", (char *)_tlp_error_msg->c_str(), -1LL);
    abort();
  }

  cout << "CHbaseTM::nodeUp called with: " << jiv_nid << "\n";
  _tlp_jenv->CallShortMethod(javaObj_, JavaMethods_[JM_NODEUP].methodID, jiv_nid);
  if (getExceptionDetails(NULL)) {
     tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::nodeUp()", (char *)_tlp_error_msg->c_str(), -1LL);
    _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }
  _tlp_jenv->PopLocalFrame(NULL);
  return RET_OK;
}

//----------------------------------------------------------------------------
// CHbaseTM: createTable
// Purpose: To handle create table transactional requests
//----------------------------------------------------------------------------
int CHbaseTM::createTable(int64 pv_transid,
                           const char* pa_tbldesc,
                           int pv_tbldesc_len,
                           char** pv_keys,
                           int pv_numsplits,
                           int pv_keylen,
                           char *errstr,
                           int &errstrlen)
{
   int lv_error = FEOK;
   jlong jlv_transid = pv_transid;
   CTmTxKey lv_tid(pv_transid);

   HBASETrace(HBASETM_TraceExitError,
              (HDR "CHbaseTM::createTable returning %d.\n", lv_error));

   JOI_RetCode lv_joi_retcode = JOI_OK;
   lv_joi_retcode = initJNIEnv();
   if (lv_joi_retcode != JOI_OK) {
      tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::createTable()", (char *)_tlp_error_msg->c_str(), -1LL);
      abort();
   }

   jbyteArray jba_tbldesc = _tlp_jenv->NewByteArray(pv_tbldesc_len);
   if (jba_tbldesc == NULL) {
      _tlp_jenv->PopLocalFrame(NULL);
      return RET_ADD_PARAM;
   }
   _tlp_jenv->SetByteArrayRegion(jba_tbldesc, 0, pv_tbldesc_len, (const jbyte*) pa_tbldesc);

   jobjectArray j_keys = NULL;
   if (pv_numsplits > 0)
   {
      j_keys = convertToByteArrayObjectArray(pv_keys, pv_numsplits, pv_keylen);
   }

   lv_error = _tlp_jenv->CallShortMethod(javaObj_,
                    JavaMethods_[JM_CREATETABLE].methodID,
                    jlv_transid,
                    jba_tbldesc,
                    j_keys);
  if (getExceptionDetails(NULL)) {
     int errMsgLen = (int)_tlp_error_msg->length();
	 errstrlen = ((errMsgLen < errstrlen) && (errMsgLen > 0)) ? errMsgLen  : errstrlen -1;
	 strncpy(errstr, _tlp_error_msg->c_str(), errstrlen);
	 errstr[errstrlen] = '\0';
	 tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::createTable()", (char *)_tlp_error_msg->c_str(), pv_transid);
     _tlp_jenv->PopLocalFrame(NULL);
     return RET_EXCEPTION;
  }

   HBASETrace(HBASETM_TraceExit, (HDR "CHbaseTM::createTable : Error %d, Txn ID (%d,%d), hostname %s.\n",
                lv_error, lv_tid.node(), lv_tid.seqnum(), pa_tbldesc));
   _tlp_jenv->PopLocalFrame(NULL);
   return lv_error;
}

//----------------------------------------------------------------------------
// CHbaseTM: alterTable
// Purpose: To handle alter table requests for hbase
//---------------------------------------------------------------------------
int CHbaseTM::alterTable(int64 pv_transid,
                        const char* pa_tblname,
                        int pv_tblname_len,
                        char ** buffer_tblopts,
                        int pv_numtblopts,
                        int pv_tbloptslen,
                        char *errstr,
                        int &errstrlen)
{
   int lv_error = FEOK;
   jlong jlv_transid = pv_transid;
   CTmTxKey lv_tid(pv_transid);

   HBASETrace(HBASETM_TraceExitError,
              (HDR "CHbaseTM::alterTable returning %d.\n", lv_error));

   JOI_RetCode lv_joi_retcode = JOI_OK;
   lv_joi_retcode = initJNIEnv();
   if (lv_joi_retcode != JOI_OK) {
      tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::alterTable()", (char *)_tlp_error_msg->c_str(), -1LL);
      abort();
   }

   jbyteArray jba_tblname = _tlp_jenv->NewByteArray(pv_tblname_len);
   if (jba_tblname == NULL) {
      _tlp_jenv->PopLocalFrame(NULL);
      return RET_ADD_PARAM;
   }
   _tlp_jenv->SetByteArrayRegion(jba_tblname, 0, pv_tblname_len, (const jbyte*) pa_tblname);

   jobjectArray j_tblopts = NULL;
   j_tblopts = convertToStringObjectArray((const char **) buffer_tblopts, pv_numtblopts);

   lv_error = _tlp_jenv->CallShortMethod(javaObj_,
                    JavaMethods_[JM_ALTERTABLE].methodID,
                    jlv_transid,
                    jba_tblname,
                    j_tblopts);
   if (getExceptionDetails(NULL)) {
      int errMsgLen = (int)_tlp_error_msg->length();
      errstrlen = ((errMsgLen < errstrlen) && (errMsgLen > 0)) ? errMsgLen : errstrlen -1;
      strncpy(errstr, _tlp_error_msg->c_str(), errstrlen);
      errstr[errstrlen] = '\0';
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::alterTable()", (char *)_tlp_error_msg->c_str(), pv_transid);
      _tlp_jenv->PopLocalFrame(NULL);
      return RET_EXCEPTION;
   }
   _tlp_jenv->PopLocalFrame(NULL);

   HBASETrace(HBASETM_TraceExit, (HDR "CHbaseTM::createTable : Error %d, Txn ID (%d,%d), hostname %s.\n",
                lv_error, lv_tid.node(), lv_tid.seqnum(), pa_tblname));

   return lv_error;
}


//----------------------------------------------------------------------------
// CHbaseTM: regTruncateOnAbort
// Purpose: To handle registration of truncate table during upsert using load
// ---------------------------------------------------------------------------
int CHbaseTM::regTruncateOnAbort(int64 pv_transid,
                           const char* pa_tblname,
                           int pv_tblname_len,
                           char *errstr,
                           int &errstrlen)
{
   int lv_error = FEOK;
   jlong jlv_transid = pv_transid;
   CTmTxKey lv_tid(pv_transid);

   HBASETrace(HBASETM_TraceExitError,
              (HDR "CHbaseTM::regTruncateOnAbort returning %d.\n", lv_error));

   JOI_RetCode lv_joi_retcode = JOI_OK;
   lv_joi_retcode = initJNIEnv();
   if (lv_joi_retcode != JOI_OK) {
      tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::regTruncateAbort()", (char *)_tlp_error_msg->c_str(), -1LL);
      abort();
   }

   jbyteArray jba_tblname = _tlp_jenv->NewByteArray(pv_tblname_len);
   if (jba_tblname == NULL) {
      _tlp_jenv->PopLocalFrame(NULL);
      return RET_ADD_PARAM;
   }
   _tlp_jenv->SetByteArrayRegion(jba_tblname, 0, pv_tblname_len, (const jbyte*) pa_tblname);

   lv_error = _tlp_jenv->CallShortMethod(javaObj_,
                    JavaMethods_[JM_REGTRUNCABORT].methodID,
                    jlv_transid,
                    jba_tblname);
   if (getExceptionDetails(NULL)) {
      int errMsgLen = (int)_tlp_error_msg->length();
      errstrlen = ((errMsgLen < errstrlen) && (errMsgLen > 0)) ? errMsgLen : errstrlen -1;
      strncpy(errstr, _tlp_error_msg->c_str(), errstrlen);
      errstr[errstrlen] = '\0';
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::regTruncateOnAbort()", (char *)_tlp_error_msg->c_str(), pv_transid);
      _tlp_jenv->PopLocalFrame(NULL);
      return RET_EXCEPTION;
   }
   _tlp_jenv->PopLocalFrame(NULL);

   HBASETrace(HBASETM_TraceExit, (HDR "CHbaseTM::regTruncateOnAbort : Error %d, Txn ID (%d,%d), hostname %s.\n",
                lv_error, lv_tid.node(), lv_tid.seqnum(), pa_tblname));

   return lv_error;
}


//----------------------------------------------------------------------------
// CHbaseTM: dropTable
// Purpose: To handle drop table transactional requests
// ---------------------------------------------------------------------------
int CHbaseTM::dropTable(int64 pv_transid,
                           const char* pa_tblname,
                           int pv_tblname_len,
                           char *errstr,
                           int &errstrlen)
{
   int lv_error = FEOK;
   jlong jlv_transid = pv_transid;
   CTmTxKey lv_tid(pv_transid);

   HBASETrace(HBASETM_TraceExitError,
              (HDR "CHbaseTM::dropTable returning %d.\n", lv_error));

   JOI_RetCode lv_joi_retcode = JOI_OK;
   lv_joi_retcode = initJNIEnv();
   if (lv_joi_retcode != JOI_OK) {
      tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::dropTable()", (char *)_tlp_error_msg->c_str(), -1LL);
      abort();
   }

   jbyteArray jba_tblname = _tlp_jenv->NewByteArray(pv_tblname_len);
   if (jba_tblname == NULL) {
      _tlp_jenv->PopLocalFrame(NULL);
      return RET_ADD_PARAM;
   }
   _tlp_jenv->SetByteArrayRegion(jba_tblname, 0, pv_tblname_len, (const jbyte*) pa_tblname);

   lv_error = _tlp_jenv->CallShortMethod(javaObj_,
                    JavaMethods_[JM_DROPTABLE].methodID,
                    jlv_transid,
                    jba_tblname);
   if (getExceptionDetails(NULL)) {
      int errMsgLen = (int)_tlp_error_msg->length();
      errstrlen = ((errMsgLen < errstrlen) && (errMsgLen > 0)) ? errMsgLen : errstrlen -1;
      strncpy(errstr, _tlp_error_msg->c_str(), errstrlen);
      errstr[errstrlen] = '\0';
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::dropTable()", (char *)_tlp_error_msg->c_str(), pv_transid);
      _tlp_jenv->PopLocalFrame(NULL);
      return RET_EXCEPTION;
   }

   _tlp_jenv->PopLocalFrame(NULL);

   HBASETrace(HBASETM_TraceExit, (HDR "CHbaseTM::dropTable : Error %d, Txn ID (%d,%d), hostname %s.\n",
                lv_error, lv_tid.node(), lv_tid.seqnum(), pa_tblname));

   return lv_error;
}

jobjectArray CHbaseTM::convertToStringObjectArray(const char **textArray, int arrayLen)
{
   int i = 0;
   jobjectArray j_objArray = NULL;
   for ( i = 0; i < arrayLen ; i++)
   {
       jstring j_obj = _tlp_jenv->NewStringUTF(textArray[i]);
       if (_tlp_jenv->ExceptionCheck())
       {
          if (j_objArray != NULL)
             _tlp_jenv->DeleteLocalRef(j_objArray);
          return NULL;
       }
       if (j_objArray == NULL)
       {
          j_objArray = _tlp_jenv->NewObjectArray(arrayLen,
                 _tlp_jenv->GetObjectClass(j_obj), NULL);
          if (_tlp_jenv->ExceptionCheck())
          {
             _tlp_jenv->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       _tlp_jenv->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
       _tlp_jenv->DeleteLocalRef(j_obj);
   }
   return j_objArray;
}

jobjectArray CHbaseTM::convertToByteArrayObjectArray(char **array,
                   int numElements, int elementLen)
{
   int i = 0;
   jobjectArray j_objArray = NULL;
   for (i = 0; i < numElements; i++)
   {
       jbyteArray j_obj = _tlp_jenv->NewByteArray(elementLen);
       if (_tlp_jenv->ExceptionCheck())
       {
          if (j_objArray != NULL)
             _tlp_jenv->DeleteLocalRef(j_objArray);
          return NULL;
       }

       _tlp_jenv->SetByteArrayRegion(j_obj, 0, elementLen,
             (const jbyte *)(array[i]));
       if (j_objArray == NULL)
       {
          j_objArray = _tlp_jenv->NewObjectArray(numElements,
                 _tlp_jenv->GetObjectClass(j_obj), NULL);
          if (_tlp_jenv->ExceptionCheck())
          {
             _tlp_jenv->DeleteLocalRef(j_obj);
             return NULL;
          }
       }

       _tlp_jenv->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
       _tlp_jenv->DeleteLocalRef(j_obj);
   }
   return j_objArray;
}

//----------------------------------------------------------------------------
// CHbaseTM::shutdown
// Purpose : Informs the HBase TM Library to shutdown
// Currently yet supported.
//----------------------------------------------------------------------------
void CHbaseTM::shutdown()
{
   HBASETrace(HBASETM_TraceAPI, (HDR "CHbaseTM::shutdown ENTRY.\n"));

   HBASETrace(HBASETM_TraceExit, (HDR "CHbaseTM::shutdown EXIT.\n"));
} //CHbaseTM::shutdown


//-------------------------------------------------------------------------------------
// requestRegionInfo
// Purpose: request region information to the hbase regions api's via hbase-trx client
//------------------------------------------------------------------------------------
HashMapArray* CHbaseTM::requestRegionInfo(){

   JOI_RetCode lv_joi_retcode = JOI_OK;
   lv_joi_retcode = initJNIEnv();
   if (lv_joi_retcode != JOI_OK) {
      tm_log_write(DTM_SEA_SOFT_FAULT, SQ_LOG_CRIT, (char *)"CHbaseTM::requestRegionInfo()", (char *)_tlp_error_msg->c_str(), -1LL);
      abort();
   }
   jobject jHashMapArray = _tlp_jenv->CallObjectMethod(javaObj_, JavaMethods_[JM_RQREGINFO].methodID);
   if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"CHbaseTM::requestRegionInfo()", (char *)_tlp_error_msg->c_str(), -1LL);
      _tlp_jenv->PopLocalFrame(NULL);
      return NULL;
   }

   if (jHashMapArray == NULL) {
      _tlp_jenv->PopLocalFrame(NULL);
      return NULL;
   }

   HashMapArray* regionMap = new HashMapArray(jvm_, _tlp_jenv, jHashMapArray);
   regionMap->init();

   _tlp_jenv->PopLocalFrame(NULL);
   return regionMap;
}


// ===========================================================================
// ===== Class HashMapArray
// ===========================================================================
JavaMethodInit* HashMapArray::JavaMethods_ = NULL;
jclass HashMapArray::javaClass_ = 0;

HashMapArray::~HashMapArray()
{

}

HMN_RetCode HashMapArray::init()
{
   static char className[]="org/trafodion/dtm/HashMapArray";

   if (isInitialized())
      return HMN_OK;

   if (JavaMethods_)
      return (HMN_RetCode)JavaObjectInterfaceTM::init(className, javaClass_, JavaMethods_, (int32)JM_LAST, true);
   else
   {
      JavaMethods_ = new JavaMethodInit[JM_LAST];

      JavaMethods_[JM_CTOR           ].jm_name       = "<init>";
      JavaMethods_[JM_CTOR           ].jm_signature  = "()V";
      JavaMethods_[JM_GET            ].jm_name       = "getElement";
      JavaMethods_[JM_GET            ].jm_signature  = "(I)Ljava/lang/String;";
      JavaMethods_[JM_GET_TNAME      ].jm_name       = "getTableName";
      JavaMethods_[JM_GET_TNAME      ].jm_signature  = "(I)Ljava/lang/String;";
      JavaMethods_[JM_GET_ENCREGNAME ].jm_name       = "getEncodedRegionName";
      JavaMethods_[JM_GET_ENCREGNAME ].jm_signature  = "(I)Ljava/lang/String;";
      JavaMethods_[JM_GET_REGNAME    ].jm_name       = "getRegionName";
      JavaMethods_[JM_GET_REGNAME    ].jm_signature  = "(I)Ljava/lang/String;";
      JavaMethods_[JM_GET_OFFLINE    ].jm_name       = "getRegionOfflineStatus";
      JavaMethods_[JM_GET_OFFLINE    ].jm_signature  = "(I)Ljava/lang/String;";
      JavaMethods_[JM_GET_REGID      ].jm_name       = "getRegionId";
      JavaMethods_[JM_GET_REGID      ].jm_signature  = "(I)Ljava/lang/String;";
      JavaMethods_[JM_GET_HOSTNAME   ].jm_name       = "getHostName";
      JavaMethods_[JM_GET_HOSTNAME   ].jm_signature  = "(I)Ljava/lang/String;";
      JavaMethods_[JM_GET_PORT       ].jm_name       = "getPort";
      JavaMethods_[JM_GET_PORT       ].jm_signature  = "(I)Ljava/lang/String;";

      return (HMN_RetCode)JavaObjectInterfaceTM::init(className, javaClass_, JavaMethods_, (int32)JM_LAST, false);
    }
}

char* HashMapArray::get(int tid)
{
   jstring js_val = (jstring)(_tlp_jenv->CallObjectMethod(javaObj_, JavaMethods_[JM_GET].methodID, tid));
   if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"HashMapArray::get()", (char *)_tlp_error_msg->c_str(), -1LL);
      return NULL;
   }
   if(js_val == NULL){
       printf("hbasetm::HashMapArray::get - js_val is NULL");
       return NULL;
   }

   const jbyte* jb_val = (jbyte*)_tlp_jenv->GetStringUTFChars(js_val, NULL);
   char* cp_val = (char *)jb_val;
   return cp_val;
}

char* HashMapArray::getTableName(int tid)
{
   jstring js_val = (jstring)(_tlp_jenv->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_TNAME].methodID, tid));
   if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"HashMapArray::getTableName()", (char *)_tlp_error_msg->c_str(), -1LL);
      return NULL;
   }
   if(js_val == NULL){
        printf("hbasetm::HashMapArray::getTableName - js_val is NULL");
       return NULL;
   }

   const jbyte* jb_val = (jbyte*)_tlp_jenv->GetStringUTFChars(js_val, NULL);
   char* cp_val = (char *)jb_val;
   return cp_val;
}

char* HashMapArray::getEncodedRegionName(int tid)
{
   jstring js_val = (jstring)(_tlp_jenv->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_ENCREGNAME].methodID, tid));
   if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"HashMapArray::getEncodedRegionName()", (char *)_tlp_error_msg->c_str(), -1LL);
      return NULL;
   }
   if(js_val == NULL){
       printf("hbasetm.cpp::HashMapArray::getEncodedRegionName - js_val is NULL");
       return NULL;
   }

   const jbyte* jb_val = (jbyte*)_tlp_jenv->GetStringUTFChars(js_val, NULL);
   char* cp_val = (char *)jb_val;
   return cp_val;
}

char* HashMapArray::getRegionName(int tid)
{
   jstring js_val = (jstring)(_tlp_jenv->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_REGNAME].methodID, tid));
   if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"HashMapArray::getRegionName()", (char *)_tlp_error_msg->c_str(), -1LL);
      return NULL;
   }

   if(js_val == NULL){
       printf("hbasetm.cpp::HashMapArray::getRegionName - js_val is NULL");
       return NULL;
   }

   const jbyte* jb_val = (jbyte*)_tlp_jenv->GetStringUTFChars(js_val, NULL);
   char* cp_val = (char *)jb_val;
   return cp_val;
}

char* HashMapArray::getRegionOfflineStatus(int tid)
{
   jstring js_val = (jstring)(_tlp_jenv->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_OFFLINE].methodID, tid));
   if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"HashMapArray::getRegionOfflineStatus()", (char *)_tlp_error_msg->c_str(), -1LL);
      return NULL;
   }
   if(js_val == NULL){
       printf("hbasetm.cpp::HashMapArray::getRegionOfflineStatus - js_val is NULL");
       return NULL;
   }

   const jbyte* jb_val = (jbyte*)_tlp_jenv->GetStringUTFChars(js_val, NULL);
   char* cp_val = (char *)jb_val;
   return cp_val;
}

char* HashMapArray::getRegionId(int tid)
{
   jstring js_val = (jstring)(_tlp_jenv->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_REGID].methodID, tid));
   if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"HashMaparray::getRegionId()", (char *)_tlp_error_msg->c_str(), -1LL);
      return NULL;
   }
   if(js_val == NULL){
       printf("hbasetm.cpp::HashMapArray::getRegionId - js_val is NULL");
       return NULL;
   }

   const jbyte* jb_val = (jbyte*)_tlp_jenv->GetStringUTFChars(js_val, NULL);
   char* cp_val = (char *)jb_val;
   return cp_val;
}

char* HashMapArray::getHostName(int tid)
{
   jstring js_val = (jstring)(_tlp_jenv->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_HOSTNAME].methodID, tid));
   if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"HashMapArray::getHostName()", (char *)_tlp_error_msg->c_str(), -1LL);
      return NULL;
   }
   if(js_val == NULL){
       printf("hbasetm.cpp::HashMapArray::getHostName - js_val is NULL");
       return NULL;
   }

   const jbyte* jb_val = (jbyte*)_tlp_jenv->GetStringUTFChars(js_val, NULL);
   char* cp_val = (char *)jb_val;
   return cp_val;
}

char* HashMapArray::getPort(int tid)
{
   jstring js_val = (jstring)(_tlp_jenv->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_PORT].methodID, tid));
   if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"HashMapArrray::getPort()", (char *)_tlp_error_msg->c_str(), -1LL);
      return NULL;
   }

   if(js_val == NULL){
       printf("hbasetm.cpp::HashMapArray::getPort - js_val is NULL");
       return NULL;
   }

   const jbyte* jb_val = (jbyte*)_tlp_jenv->GetStringUTFChars(js_val, NULL);
   char* cp_val = (char *)jb_val;
   return cp_val;
}

