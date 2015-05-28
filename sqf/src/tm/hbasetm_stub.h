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

#ifndef HBASETM_STUB_H_
#define HBASETM_STUB_H_


enum {
   RET_OK = 0,
   RET_EXCEPTION = 1,
   RET_READONLY = 2,
   RET_NOTX = 3
} HBase_trx_ReturnCode;

class CHbaseTM
{
public:
   // Public member variables

private:
   // Private member variables
   bool iv_initialized;          // true after first call to HbaseTM_initialize
   int iv_my_nid;                // nid and pid of the process the library is running in.
   int iv_my_pid;                //

public:
   CHbaseTM() {};
   ~CHbaseTM() {};
      
   // Set/Get methods
   int my_nid() {return iv_my_nid;}
   void my_nid(int pv_nid) {iv_my_nid = pv_nid;}
   int my_pid() {return iv_my_pid;}
   void my_pid(int pv_pid) {iv_my_pid = pv_pid;}


   // TM interface methods
   short initConnection();
   short beginTransaction(int64 *pp_transid) { return FEOK; }
   short prepareCommit(int64 pv_transid) {return FEOK; }
   short doCommit(int64 pv_transid) { return FEOK; }
   short abortTransaction(int64 pv_transid) {return FEOK; }
   int registerRegion(int64 pv_transid, int64 pv_startid, const char pa_region[], int64 pv_hashcode, const char pa_regionInfo[], int pv_regionInfo_Length) {return FEOK; }
   int recoverRegion(int64 *pp_count, int64 *pp_transidList[], int64 *pp_flags);
   short nodeDown(int32 pv_nid) { return FEOK; }
   short nodeUp(int32 pv_nid) { return FEOK; }

   int failedRegions(int64 pv_transid) { return FEOK; }
   int participatingRegions(int64 pv_transid) { return FEOK; }
   int unresolvedRegions(int64 pv_transid) {return FEOK; }
   void shutdown() {}

private:
   // Private methods:
   void lock();
   void unlock();
   int setAndGetNid();

}; // class CHbaseTM



extern CHbaseTM gv_HbaseTM;      // One global HbaseTM object

#endif //HBASETM_H_

