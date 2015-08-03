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

#ifndef TMAUDIT_H_
#define TMAUDIT_H_

#include <stdio.h>
#include "dtm/tm_util.h"
#include "dtm/tmtransid.h"
#include "../nq_recovery/auditrw.h"

#include "tmmutex.h"
#include "seabed/trace.h"

// ----------------------------------------------------------------
// tmaudit.h
// Includes :
//      defines for transaction states
//      defines for audit record types
//  defines for abort reaons
//  header for audit records
//  body for control point and state records
//  audit class TM_Aduit
// ----------------------------------------------------------------

#define REC_SIZE 84
#define CP_THRESHOLD 1433600
//#define CP_THRESHOLD 2048

// transaction states
enum {Active_Trans_State         = 100};
enum {Prepared_Trans_State       = 101};
enum {Committed_Trans_State      = 102};
enum {Aborting_Trans_State       = 103};
enum {Forgotten_Trans_State      = 104};
enum {Aborted_Trans_State        = 105}; 
enum {HungCommitted_Trans_State  = 106}; 
enum {HungAborted_Trans_State    = 107}; 

// types of audit records
enum {TM_Control_Point              = 1021};
enum {TM_Shutdown                   = 1022};
enum {TM_Transaction_State          = 1023};
enum {TM_Control_Transaction_State  = 1030};
enum {TM_RM_State                   = 1038};

// abort flags
#define AbortDueToBTCpuFailure      0x00000000;
#define AbortDueToAuditLoss         0x00000001;
#define AbortByAppln                0x00000002;
#define AbortDueToDpCrash           0x00000004;
#define AbortDueToDpinternalError   0x00000005;
#define AbortDueDoublecpuFailure    0x00000006;
#define AbortByNetwork              0x00000007;
#define AbortBySQL                  0x00000008;
#define AbortbyFileSys              0x00000009;
#define AbortByTimeOut              0x0000000a;
#define AbortDueToSevereFailure     0x0000000b;
#define AbortDueAuditOverflow       0x0000000c;
#define AbortMiscellaneou           0x0000000d;
#define AbortByRm                   0x0000000e;
#define AbortDueToExcessBranchInfo  0x0000000f;
#define AbortDueToExcessBranchData  0x00000010;
#define AbortDueExportingCpuFailure 0x00000011;
#define AbortDueToResumerCpuFailure 0x00000012;
#define AbortdueToOutcomeUnknown    0x00000013;   
  
#pragma pack(push, one, 4)
// Audit Header Struct, 60 bytes (aligned, needed 2 extra)
struct Audit_Header
{
    int32              iv_length;
    TM_Transid_Type    iv_transid;
    union {
        int16  iv_nameI[4];
        unsigned char iv_name;
    };

    int64              iv_vsn;
    int16              iv_type;
    int16              iv_filler[2];
};

// Audit Control Point Struct, 24 bytes + 58
struct Audit_Control_Point
{
    Audit_Header iv_hdr;
    int64    iv_time_stamp;
    int32    iv_byte_count;
    int16    iv_filler[4]; 
    int32    iv_length;
};

// Audit Transaction State Struct, 24 bytes + 58
struct Audit_Transaction_State
{
    Audit_Header    iv_hdr;
    int64           iv_time_stamp;
    int32           iv_abort_flags;
    int16           iv_filler[3];
    int16           iv_state;
    int32           iv_length;
};

// Shutdown record
struct Audit_TM_Shutdown
{
    Audit_Header    iv_hdr;
    int64           iv_time_stamp;
    int16           iv_filler[5];
    int16           iv_state;
    int32           iv_length;
};
   
#pragma pack(pop, one)

// Audit writing/reading wrapper class 
class TM_Audit
{
    public:
        // constructor/destructor 
        TM_Audit();
        ~TM_Audit();

        int32       initialize_adp();

        // read routines
        void        end_backwards_scan();
        Addr        read_audit_rec ();
        void        release_audit_rec();
        void        start_backwards_scan();

        // write routines
        int32        write_trans_state(TM_Transid_Type *pv_transid, int32 pv_nid, 
                                       int32 pv_state, int32 pv_abort_flags);
        int32        write_control_point(int32 pv_node);
        void         write_shutdown(int32 pv_nid, int32 pv_state);
        
        int32        write_buffer(int32 pv_length, char *pp_buffer, int64 pv_highest_vsn);

        bool         prepare_trans_state(Audit_Transaction_State *pp_state_rec, 
                                         char *pp_write_buffer, 
                                         TM_Transid_Type *pv_transid, 
                                         int32 pv_nid, int32 pv_state,
                                         int32 pv_abort_flags, int64 *pp_vsn /*optional*/);
        int64        audit_position() { return iv_position; }

private:
        void        initialize_hdr (Audit_Header *pp_hdr, int16 pv_length,
                                       int32 pv_type, TM_Transid_Type *pv_transid);
        int32       send_audit(char * pp_data, int32 pv_length);
        int64       get_position() {return iv_position;}

        // adp wrappers 
        void  adp_activate_cursor();
        void  adp_deactivate_cursor();
        void  adp_module_init();
        void  adp_module_terminate();
        void  adp_release_record();
        int32 adp_send_audit(char *pp_buffer, int32 pv_length, int64 pv_vsn, bool pv_force);
        void audit_send_position();

        int16 ia_vol_name[4];
        char  ia_vol_name2[9];
        FILE *ip_audit_file;
        Addr  ip_audit_rec;
        Addr  ip_cursor;
        SB_Phandle_Type iv_adp_phandle;
        bool  iv_initialized;
        bool  iv_notified_threshold;
        int64 iv_position;
        int64 iv_vsn; 

        TM_Mutex      iv_mutex;
};

#define audit_max(a,b) (((a) > (b)) ? (a) : (b))
     
#endif


