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

#ifndef AUDITOBJ_H_
#define AUDITOBJ_H_

#include <sys/types.h>
#include <sys/time.h>
#include "tmmmap.h"
#include "tmmutex.h"
#include "tmlibmsg.h"
#include "tmtxmsg.h"
#include "tmlogging.h"

// As good a size as any for the moment
#define TM_MAX_AUDIT_BUF_SIZE 4096

// Forward declarations
class TM_TX_Info;

// CTmAuditObj class definition
class CTmAuditObj :public SB_Thread::Thread
{
private: 

   // Semaphore to serialize updates to the object.
   TM_Mutex    iv_mutex; 
   bool iv_exit;


   // Semaphore to hold up fills when the fill buffer is full and the write buffer 
   // is not done writing.
   TM_Mutex    iv_buf_mutex; 

   SB_Thread::CV iv_CV; // Condition variable for controling the thread

   TM_Audit    iv_mat;  //Actual audit object interface to the ASE
   char *      ia_write_buffer;
   char *      ia_fill_buffer;
   int64       iv_highest_fill_vsn;
   int32       iv_write_buf_size;
   bool        iv_prepared_to_write;
   bool        iv_written_since_timer;
   bool        iv_double_write;
   int32       iv_space_used;

   void        swap_buffers();

public:
   CTmAuditObj(SB_Thread::Sthr::Function pv_fun, const char *pp_name);
   ~CTmAuditObj();

   void lock();
   void unlock();
   bool exit() {return iv_exit;}
   void exitNow() 
   {iv_exit=true;
    iv_CV.signal(true /*lock*/); }
   void wait () {iv_CV.wait(true /*lock*/);}
   void wait (bool pv_lock, int64 pv_sec, int64 pv_usec) { iv_CV.wait(pv_lock, pv_sec, pv_usec);}
   bool write_oustanding () {return iv_prepared_to_write;}

   int64      prepare_trans_state_rec(char * pp_buffer, int32 pv_length, TM_Transid_Type *pp_transid, 
                                          int32 pv_nid, int32 pv_state, int32 pv_abort_flags);
   void       prepare_to_write(bool pv_already_locked = false);
   void       push_audit_rec (int32 pv_size, char *pp_buffer, int64 pv_vsn, bool pv_hurry);
   int32      space_used() {return iv_space_used;}

   // Write out the write buffer.
   // Needs to be public, but cannot be called other than in Audit module.
   // The method will make sure this is the case.
   void write_buffer(); 

   // TM_Info interface
   int32       initialize_adp() {return iv_mat.initialize_adp();}
   void        end_backwards_scan() {iv_mat.end_backwards_scan();}
   void        flush_buffer();
   Addr        read_audit_rec () {return iv_mat.read_audit_rec();}
   void        release_audit_rec() {iv_mat.release_audit_rec();}
   void        start_backwards_scan() {iv_mat.start_backwards_scan();}
   int32       write_control_point(int32 pv_node) {return iv_mat.write_control_point(pv_node);}
   void        write_shutdown(int32 pv_nid, int32 pv_state) {iv_mat.write_shutdown(pv_nid, pv_state);}
   int32       write_trans_state(TM_Transid_Type *pp_transid, int32 pv_nid, int32 pv_state, int32 pv_abort_flags) 
         {return iv_mat.write_trans_state(pp_transid, pv_nid, pv_state, pv_abort_flags);}

   int64       audit_position() { return iv_mat.audit_position(); }
   
}; //CTmAuditObj


// Audit thread main line is not a method against the object.
extern void * auditThread_main(void *arg);

#endif //AUDITOBJ_H_
