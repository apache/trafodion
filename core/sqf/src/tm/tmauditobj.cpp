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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "seabed/ms.h"
#include "seabed/thread.h"

#include "tminfo.h"
#include "seabed/trace.h"
#include "tmlogging.h"
#include "tminfo.h"
#include "tmauditobj.h"


//----------------------------------------------------------------------------
// CTmAuditObj methods
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CTmAuditObj Constructor
// Constructs a CTmAuditObj object.
//----------------------------------------------------------------------------
CTmAuditObj::CTmAuditObj(SB_Thread::Sthr::Function pv_fun, const char *pp_name)
:SB_Thread::Thread(pv_fun, pp_name), iv_exit(false), iv_highest_fill_vsn(0), 
iv_write_buf_size(0), iv_prepared_to_write (0), iv_written_since_timer (0),
iv_double_write(0), iv_space_used (0)
{
   TMTrace(2, ("CTmAuditObj::CTmAuditObj : ENTRY.\n"));

   ia_write_buffer = new char[TM_MAX_AUDIT_BUF_SIZE];
   ia_fill_buffer  = new char[TM_MAX_AUDIT_BUF_SIZE];

   start();  // start up the thread

   TMTrace(2, ("CTmAuditObj::CTmAuditObj : EXIT.\n"));
} //CTmAuditObj::CTmAuditObj


//----------------------------------------------------------------------------
// CTmAuditObj Destructor
//----------------------------------------------------------------------------
CTmAuditObj::~CTmAuditObj()
{
   TMTrace(2, ("CTmAuditObj::~CTmAuditObj : ENTRY\n"));

   delete [] ia_write_buffer;
   delete [] ia_fill_buffer;

   TMTrace(2, ("CTmAuditObj::~CTmAuditObj : EXIT\n"));
}

// --------------------------------------------------------------
// CTmAuditObj::lock
// Lock the mutex to serialize access to the object
// --------------------------------------------------------------
void CTmAuditObj::lock()
{
   iv_mutex.lock();
   TMTrace (5, ("CTmAuditObj::lock(%d)\n", iv_mutex.lock_count()));
}


// --------------------------------------------------------------
// CTmAuditObj::unlock
// Unlock the mutex to serialize access to the object
// --------------------------------------------------------------
void CTmAuditObj::unlock()
{
   iv_mutex.unlock();
   TMTrace (5, ("CTmAuditObj::unlock(%d)\n", iv_mutex.lock_count()));
}

// -----------------------------------------------------------------
// CTmAuditObj::swap_buffers
// swap the fill buffer with the write buffer.
// TO BE CALLED UNDER MUTEX
// -----------------------------------------------------------------
void CTmAuditObj::swap_buffers()
{
    TMTrace(2, ("CTmAuditObj::swap_buffers : ENTRY.\n"));

    char *lp_temp_buffer = ia_fill_buffer;
    iv_write_buf_size = iv_space_used; // save off how large the buffer is
    ia_fill_buffer = ia_write_buffer;
    ia_write_buffer = lp_temp_buffer;
    iv_space_used = 0; // new fill buffer is empty

    TMTrace(2, ("CTmAuditObj::swap_buffers : EXIT, iv_write_buf_size %d.\n", iv_write_buf_size));
}

void CTmAuditObj::prepare_to_write(bool pv_already_locked)
{
    static TM_Mutex lv_mutex;

    lv_mutex.lock();
    if (!iv_prepared_to_write)
    {
     if (!pv_already_locked)
        iv_buf_mutex.lock(); // unlock will happen after the write.
     swap_buffers();
     iv_prepared_to_write = true;
    }
    lv_mutex.unlock();
}
// -------------------------------------------------------------------
// CTmAuditObj::push_audit_rec
// This will push an audit record onto the buffer.  If pv_hurry is try,
// it will force an immediate write.
// --------------------------------------------------------------------
void CTmAuditObj::push_audit_rec (int32 pv_size, char *pp_buffer,int64 pv_vsn, bool pv_hurry)
{
     TMTrace(2, ("CTmAuditObj::push_audit_rec : ENTRY, size %d, vsn " PFLL ", hurry flag (%d).\n", 
                  pv_size, pv_vsn, pv_hurry));

     int32 lv_buf_location = 0;
     bool lv_buf_filled = false;

     lock();
     lv_buf_location = iv_space_used;
     iv_highest_fill_vsn = pv_vsn;
     // if we still have space in the buffer for this record.
     if ((iv_space_used + pv_size) < TM_MAX_AUDIT_BUF_SIZE)
     {
          iv_space_used += pv_size;
          memcpy (&(ia_fill_buffer[lv_buf_location]), pp_buffer, pv_size);
          lv_buf_filled = true;

          // if we aren't yet full, but still have enough room for another record
          // (and this isn't a hurry write), then it is ok to return.  Otherwise, 
          // we force a write
          if (((TM_MAX_AUDIT_BUF_SIZE-iv_space_used) >= REC_SIZE) && (!pv_hurry))
          {
              unlock(); 
              TMTrace(2, ("CTmAuditObj::push_audit_rec, no hurry : EXIT.\n"));
              return;
          }
     }

     // if there was not enough space to write in the fill buffer
     // or this was a hurry request - swap out the buffers. 
     // if this lock is being held, it means that the write buffer is
     // being written and we need to wait until it is free to swap.
     prepare_to_write();

     // this is the case that there was no room in the buffer
     // we need to account for pv_hurry here - hence the double write
     if (!lv_buf_filled)
     { 
        iv_space_used = pv_size;
        memcpy (&(ia_fill_buffer[0]), pp_buffer, pv_size);
        if (pv_hurry)
           iv_double_write = true;
     }

      iv_CV.signal(true /*lock*/); 
      unlock();  // allow new fill buffer to fill up while we are writing 

      TMTrace(2, ("CTmAuditObj::push_audit_rec : EXIT.\n"));
      return;
}

// -------------------------------------------------------------------------
// CTmAuditObj::prepare_trans_state
// This will set up the trans state record in a simple buffer format
// -------------------------------------------------------------------------
int64 CTmAuditObj::prepare_trans_state_rec(char * pp_buffer, int32 pv_length, TM_Transid_Type *pp_transid, 
                                          int32 pv_nid, int32 pv_state, int32 pv_abort_flags)
{
    TMTrace(2, ("CTmAuditObj::prepare_trans_state : ENTRY.\n"));

    Audit_Transaction_State lv_state_rec;
    int64 lv_vsn = 0;

    // turn state record into buffer format
    iv_mat.prepare_trans_state(&lv_state_rec, pp_buffer, pp_transid, pv_nid, 
                               pv_state, pv_abort_flags, &lv_vsn);

    TMTrace(2, ("CTmAuditObj::prepare_trans_state : EXIT.\n"));
    return lv_vsn;
}

// -----------------------------------------------------------------------
// CTmAuditObj::write_buffer
// Write the buffer to the ASE
// -----------------------------------------------------------------------
void CTmAuditObj::write_buffer()
{
    TMTrace(2, ("CTmAuditObj::write_buffer : ENTRY.\n"));

    int32 lv_notify = -1;

    if (iv_prepared_to_write) // no lock means it was not called appropriately, error TODO
    {
        if (iv_write_buf_size > 0)
        {
            TMTrace(2, ("CTmAuditObj::write_buffer with size of %d: ENTRY.\n", iv_write_buf_size));
            lv_notify = iv_mat.write_buffer(iv_write_buf_size, ia_write_buffer, iv_highest_fill_vsn);
            iv_write_buf_size = 0;
        }
        else
            TMTrace(2, ("CTmAuditObj::write_buffer called with 0 size.\n"));
    }

    if (iv_double_write)
    {
        prepare_to_write(true);
        if (iv_write_buf_size > 0)
        {  
            lv_notify = iv_mat.write_buffer(iv_write_buf_size, ia_write_buffer, iv_highest_fill_vsn);
            iv_write_buf_size = 0;
        }
        iv_double_write = false;
    }

    iv_prepared_to_write = false;
    iv_buf_mutex.unlock();
    TMTrace(2, ("CTmAuditObj::write_buffer : EXIT.\n"));
}

// --------------------------------------------------------------------------
// CTmAuditObj::flush_buffer
// This method will allow a 3rd party (TM_Info for example) to be able to 
// force a write.  This is done in the case of control point processing
// --------------------------------------------------------------------------
void CTmAuditObj::flush_buffer()
{
     TMTrace(2, ("CTmAuditObj::flush_buffer : ENTRY.\n"));

     iv_buf_mutex.lock();
     swap_buffers();
     iv_prepared_to_write = true;

     iv_CV.signal(true /*lock*/); 

     TMTrace(2, ("CTmAuditObj::flush_buffer : EXIT.\n"));
}

//----------------------------------------------------------------------------
// auditThread_main
// Purpose : Main for audit thread
//----------------------------------------------------------------------------
void * auditThread_main(void *arg)
{
   CTmAuditObj     *lp_auditTh;
   timeval          lv_waitTime;

   arg = arg;

   TMTrace(2, ("auditThread_main : ENTRY.\n"));

   while (gv_tm_info.tmAuditObj() == NULL)
   {
      SB_Thread::Sthr::usleep(10);
   }
   // Now we can set a pointer to the CTmAuditObj object because it exits
   lp_auditTh = gv_tm_info.tmAuditObj();

   TMTrace(2, ("auditThread_main : Thread %s(%p).\n",
                lp_auditTh->get_name(), (void *) lp_auditTh));

   while (!lp_auditTh->exit())
   {
        lv_waitTime.tv_sec = 0;
        lv_waitTime.tv_usec = 10000;  // 10000 microseconds is 1/100 a second
        lp_auditTh->wait(true /*lock*/, lv_waitTime.tv_sec, lv_waitTime.tv_usec);

        if (lp_auditTh->exit())
            break;
        // write out pending buffer
        if (lp_auditTh->write_oustanding())
            lp_auditTh->write_buffer();
        // timer popped with no outstanding write, which means it is time to 
        // write the fill buffer if anything is in it.
        else if (lp_auditTh->space_used() > 0)
        {
       //     TMTrace(3, ("auditThread_main : timer pop with buffer to write.\n"));
            lp_auditTh->prepare_to_write();
            lp_auditTh->write_buffer();
        }
       // else
      //      TMTrace(3, ("auditThread_main : timer pop with NO buffer to write.\n"));

   }

   TMTrace(2, ("auditThread_main : EXIT.\n"));

   //lp_auditTh->stop();
   return NULL;
} //auditThread_main
