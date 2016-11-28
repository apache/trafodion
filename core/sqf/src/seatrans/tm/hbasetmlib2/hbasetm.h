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

#ifndef HBASETM_H_
#define HBASETM_H_

#include "tmtimer.h"
//#include "tmmutex.h"
#include "hbasetmglob.h"
#include "javaobjectinterfacetm.h"
#include "jni.h"

// ===============================================================
// ===== HashMapArray class: provides access to the Java 
// ===== HashMapArray class to obtain Hbase Region Info
// ===============================================================
typedef enum {
   HMN_OK       = 0,
   HMN_LAST
} HMN_RetCode;

class HashMapArray : public JavaObjectInterfaceTM
{
public:
   // Default constructor. Creates a new JVM
   HashMapArray()
   : JavaObjectInterfaceTM()
   {}

   HashMapArray(JavaVM *jvm, JNIEnv *jenv, jobject jObj = NULL)
   : JavaObjectInterfaceTM(jvm, jenv, jObj)
   {}

   // Destructor
   virtual ~HashMapArray();

   HMN_RetCode  init();

   char* get(int index);
   char* getTableName(int index);
   char* getEncodedRegionName(int index);
   char* getRegionName(int index);
   char* getRegionOfflineStatus(int index);
   char* getRegionId(int index);
   char* getHostName(int index);
   char* getPort(int index);

private:
   enum JAVA_METHODS {
       JM_CTOR = 0,
       JM_GET,
       JM_GET_TNAME,
       JM_GET_ENCREGNAME,
       JM_GET_REGNAME,
       JM_GET_OFFLINE,
       JM_GET_REGID,
       JM_GET_HOSTNAME,
       JM_GET_PORT,
       JM_LAST
   };
   static JavaMethodInit* JavaMethods_;
   static jclass          javaClass_;
};

// ==============================================================
// === HBaseTM class
// ==============================================================
enum HBTM_RetCode {
   RET_OK = 0,
   RET_NOTX,
   RET_READONLY,
   RET_ADD_PARAM,
   RET_EXCEPTION,
   RET_HASCONFLICT,
   RET_IOEXCEPTION,
   RET_NOCOMMITEX,
   RET_LAST
};

// Forward declarations

// HBASE TM Class. This represents the Transaction 
// Manager.  There is one per HBASE TM Library.
class CHbaseTM : public JavaObjectInterfaceTM
{
public:
   // Public member variables

private:
   // Private member variables
   bool iv_initialized;          // true after first call to HbaseTM_initialize
   int iv_my_nid;                // nid and pid of the process the library is running in.
   int iv_my_pid;                //
   int64 iv_next_msgNum;
   int32 iv_lastHBaseError;        // Last error returned by an attempt to open a subordinate RM.
   HBASETM_TraceMask iv_traceMask;  // HBASE TM tracing Mask.  0 = no tracing (default)
   bool iv_tm_stats;             // Collect statistics.

   //TM_Mutex *ip_mutex;           // Semaphore to serialize updates to the object.
   CTmTimer *ip_tmTimer;         // Timer thread object pointer

   enum JAVA_METHODS {
      JM_CTOR= 0,
      JM_INIT1,  
      JM_BEGIN,
      JM_ABORT,
      JM_PRECOMMIT, 
      JM_DOCOMMIT, 
      JM_TRYCOMMIT,
      JM_COMPLETEREQUEST,
      JM_REGREGION,
      JM_PARREGION,
      JM_CNTPOINT,
      JM_STALL,
	  JM_NODEDOWN,
      JM_NODEUP,
      JM_CREATETABLE,
      JM_ALTERTABLE,
      JM_REGTRUNCABORT,
      JM_DROPTABLE,
      JM_RQREGINFO,
      JM_LAST
   };
   JavaMethodInit JavaMethods_[JM_LAST];
   static jclass       javaClass_;

public:
   CHbaseTM();
#if 0
   CHbaseTM(JavaVM *jvm, JNIEnv *jenv);
#endif
   ~CHbaseTM();
   int initialize(short pv_nid);
   int initialize(HBASETM_TraceMask pv_traceMask, bool pv_tm_stats, CTmTimer *pp_tmTimer, short pv_nid);
   bool Trace(HBASETM_TraceMask pv_traceMask);
   void setTrace(HBASETM_TraceMask pv_traceMask);      // Set HBASETM tracing.
   bool tm_stats() {return iv_tm_stats;}
   int initJVM();

   // Initialized
   bool initialized() {return iv_initialized;}
   CTmTimer *tmTimer() {return ip_tmTimer;}
   void tmTimer(CTmTimer *pp_timer) {ip_tmTimer = pp_timer;}
      
   // Set/Get methods
   int my_nid() {return iv_my_nid;}
   void my_nid(int pv_nid) {iv_my_nid = pv_nid;}
   int my_pid() {return iv_my_pid;}
   void my_pid(int pv_pid) {iv_my_pid = pv_pid;}

   void traceMask(HBASETM_TraceMask pv_traceMask) {iv_traceMask = pv_traceMask;}
   HBASETM_TraceMask traceMask() {return iv_traceMask;}
   void lastHBaseError(int32 pv_lastHBaseError) {iv_lastHBaseError=pv_lastHBaseError;}
   int32 lastHBaseError() {return iv_lastHBaseError;}

   // TM interface methods
   short initConnection(short pv_nid);
   short beginTransaction(int64 *pp_transid);
   short prepareCommit(int64 pv_transid);
   short doCommit(int64 pv_transid);
   short tryCommit(int64 pv_transid);
   short completeRequest(int64 pv_transid);
   short abortTransaction(int64 pv_transid);
   int dropTable(int64 pv_transid, const char* pa_tblname, int pv_tblname_len, char *errstr, int &errstrlen);
   int regTruncateOnAbort(int64 pv_transid,
                           const char* pa_tblname,
                           int pv_tblname_len,
                           char *errstr,
                           int &errstrlen);
   int createTable(int64 pv_transid,
                           const char* pa_tbldesc,
                           int pv_tbldesc_len,
                           char** pv_keys,
                           int pv_numsplits,
                           int pv_keylen,
                           char *errstr,
                           int &errstrlen);
   int alterTable(int64 pv_transid,
                        const char* pa_tblname,
                        int pv_tblname_len,
                        char ** buffer_tblopts,
                        int pv_numtblopts,
                        int pv_tbloptslen,
                        char *errstr,
                        int &errstrlen);
   jobjectArray convertToByteArrayObjectArray(char **array, int numElements, int elementLen);
   jobjectArray convertToStringObjectArray(const char **textArray, int arrayLen);
   int registerRegion(int64 pv_transid, const char pa_region[], const char pa_regionInfo[], int pv_regionInfo_Length);
   int registerRegion(int64 pv_transid,
            int64 pv_startid,
            int pv_port,
            const char pa_hostname[],
            int pv_hostname_Length,
            long pv_startcode,
            const char pa_regionInfo[],
            int pv_regionInfo_Length);
   short addControlPoint();
   int recoverRegion(int64 *pp_count, int64 *pp_transidList[], int64 *pp_flags);
      // pp_count :input Maximum number of transids to be returned in a single reply
      //               :output Number of transids returned
      // Replies will contain transaction lists for a single region only.  It is assumed that all transactions for a region will be contained
      // within the array pointed to by pp_transidList.
      // pp_flags as per xa_recover flags:
      //    TMSTARTRSCAN 0x01000000LL /* start a recovery scan */
      //    TMENDRSCAN 0x00800000LL /* end a recovery scan */
      //    TMRESENDRSCAN 0x00010000LL 
      //    TMRMFAILRSCAN 0x00001000LL 
      //  *** TODO JDR We need to add code in the TransactionManager to detect restarting regions when a node restarts so that
      //  *** recovery can be driven to these regions.  I'm not sure what this mechanism is yet, but it will need to be in the HBase-trx TM Library
      //  *** because the TM has not HBase specific knowledge.  This will replace TMRESENDRSCAN from the TM.
      //    When TMRESENDRSCAN is specified pp_region must contain a valid region name.  This is used to retrieve indoubt transactions
      //    for a specific region following a region failure.
   int failedRegions(int64 pv_transid);
   int participatingRegions(int64 pv_transid);
   int unresolvedRegions(int64 pv_transid);
   short stall(int32 when);
   void shutdown();
   short nodeDown(int32 nid);
   short nodeUp(int32 nid);
   HashMapArray* requestRegionInfo();

private:
   // Private methods:
   void lock();
   void unlock();
   int setAndGetNid();

}; // class CHbaseTM



extern CHbaseTM gv_HbaseTM;      // One global HbaseTM object
extern const char *ms_getenv_str(const char *pp_key);
extern const char *ms_getenv_int(const char *pp_key, int *pp_val);

short HBasetoTxnError(short pv_HBerr);

#endif //HBASETM_H_

