//------------------------------------------------------------------
//
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

#ifndef __SB_MPIP_H_
#define __SB_MPIP_H_

#include <pthread.h>

// us per sec
enum { MPIP_USPS = 1000000 };

//
// counter ids
//
enum {
    MPIP_C_MPI_MIN,
    MPIP_C_MPI_MARK,
    MPIP_C_MPI_ABORT,
    MPIP_C_MPI_ALLGATHER,
    MPIP_C_MPI_BARRIER,
    MPIP_C_MPI_BSEND,
    MPIP_C_MPI_CANCEL,
    MPIP_C_MPI_CLOSE_PORT,
    MPIP_C_MPI_COMM_ACCEPT,
    MPIP_C_MPI_COMM_CONNECT,
    MPIP_C_MPI_COMM_CREATE,
    MPIP_C_MPI_COMM_DISCONNECT,
    MPIP_C_MPI_COMM_DUP,
    MPIP_C_MPI_COMM_FREE,
    MPIP_C_MPI_COMM_GROUP,
    MPIP_C_MPI_COMM_RANK,
    MPIP_C_MPI_COMM_SET_ERRHANDLER,
    MPIP_C_MPI_COMM_SIZE,
    MPIP_C_MPI_ERROR_CLASS,
    MPIP_C_MPI_ERROR_STRING,
    MPIP_C_MPI_FINALIZE,
    MPIP_C_MPI_GET_COUNT,
    MPIP_C_MPI_GROUP_INCL,
    MPIP_C_MPI_IBSEND,
    MPIP_C_MPI_INFO_CREATE,
    MPIP_C_MPI_INFO_FREE,
    MPIP_C_MPI_INFO_SET,
    MPIP_C_MPI_INIT,
    MPIP_C_MPI_INIT_THREAD,
    MPIP_C_MPI_IPROBE,
    MPIP_C_MPI_IRECV,
    MPIP_C_MPI_IRSEND,
    MPIP_C_MPI_ISEND,
    MPIP_C_MPI_ISSEND,
    MPIP_C_MPI_LOOKUP_NAME,
    MPIP_C_MPI_OPEN_PORT,
    MPIP_C_MPI_PUBLISH_NAME,
    MPIP_C_MPI_RECV,
    MPIP_C_MPI_RSEND,
    MPIP_C_MPI_SEND,
    MPIP_C_MPI_SENDRECV,
    MPIP_C_MPI_SSEND,
    MPIP_C_MPI_TEST,
    MPIP_C_MPI_TEST_CANCELLED,
    MPIP_C_MPI_TESTALL,
    MPIP_C_MPI_TESTANY,
    MPIP_C_MPI_TESTSOME,
    MPIP_C_MPI_WAIT,
    MPIP_C_MPI_WAITALL,
    MPIP_C_MPI_WAITANY,
    MPIP_C_MPI_WAITSOME,
    MPIP_C_MPI_MAX
};

//   12345678901234567890
#define MPIP_OP_NAMES \
    "MPI_MIN", \
    "MPI_MARK", \
    "MPI_Abort", \
    "MPI_Allgather", \
    "MPI_Barrier", \
    "MPI_Bsend", \
    "MPI_Cancel", \
    "MPI_Close_port", \
    "MPI_Comm_accept", \
    "MPI_Comm_connect", \
    "MPI_Comm_create", \
    "MPI_Comm_disconnect", \
    "MPI_Comm_dup", \
    "MPI_Comm_free", \
    "MPI_Comm_group", \
    "MPI_Comm_rank", \
    "MPI_Comm_set_errh", \
    "MPI_Comm_size", \
    "MPI_Error_class", \
    "MPI_Error_string", \
    "MPI_Finalize", \
    "MPI_Get_count", \
    "MPI_Group_incl", \
    "MPI_Ibsend", \
    "MPI_Info_create", \
    "MPI_Info_free", \
    "MPI_Info_set", \
    "MPI_Init", \
    "MPI_Init_thread", \
    "MPI_Iprobe", \
    "MPI_Irecv", \
    "MPI_Irsend", \
    "MPI_Isend", \
    "MPI_Issend", \
    "MPI_Lookup_name", \
    "MPI_Open_port", \
    "MPI_Publish_name", \
    "MPI_Recv", \
    "MPI_Rsend", \
    "MPI_Send", \
    "MPI_Sendrecv", \
    "MPI_Ssend", \
    "MPI_Test", \
    "MPI_Test_cancelled", \
    "MPI_Testall", \
    "MPI_Testany", \
    "MPI_Testsome", \
    "MPI_Wait", \
    "MPI_Waitall", \
    "MPI_Waitany", \
    "MPI_Waitsome", \
    "MPI_MAX"

// counter data
typedef struct MPIP_Cnt_Type {
    const char         *ip_name;
    long long           iv_count;
    pthread_spinlock_t  iv_sl;
    long long           iv_us_min;
    long long           iv_us_max;
    long long           iv_us_total;
} MPIP_Cnt_Type;

//
// operations
//
enum {
    MPIP_P_O_SEND,
    MPIP_P_O_RECV
};

//
// flags
//
enum {
    MPIP_P_F_CNT_B,  // begin
    MPIP_P_F_CNT_E,  // end
    MPIP_P_F_CNT_X,  // --
    MPIP_P_F_CONN_B,
    MPIP_P_F_CONN_E,
    MPIP_P_F_CONN_X,
    MPIP_P_F_NONE_B,
    MPIP_P_F_NONE_E,
    MPIP_P_F_NONE_X,
    MPIP_P_F_RECV_B,
    MPIP_P_F_RECV_E,
    MPIP_P_F_RECV_X,
    MPIP_P_F_SEND_B,
    MPIP_P_F_SEND_E,
    MPIP_P_F_SEND_X,
    MPIP_P_F_TEST_B,
    MPIP_P_F_TEST_E,
    MPIP_P_F_TEST_X,
    MPIP_P_F_WAIT_B,
    MPIP_P_F_WAIT_E,
    MPIP_P_F_WAIT_X
};

//
// hdr versions
//
enum {
    MPIP_H_OP_VERS  = 1,
    MPIP_H_OPD_VERS = 1
};

// op hdr
typedef struct MPIP_Op_Hdr_Type {
    union {
        struct {
            int   iv_vers;
            int   iv_hdr_size;
            int   iv_rec_size;
            int   iv_pid;
            int   iv_cputime;
            int   iv_proglen;
            char  ia_prog[1024];
            char  ia_host[256];
        } h;
        char d[8192];
    } u;
} MPIP_Op_Hdr_Type;

// opd hdr
typedef struct MPIP_Opd_Hdr_Type {
    union {
        struct {
            int   iv_vers;
            int   iv_hdr_size;
            int   iv_rec_size;
            int   iv_pid;
            int   iv_cputime;
            int   iv_proglen;
            char  ia_prog[1024];
            char  ia_host[256];
        } h;
        char d[8192];
    } u;
} MPIP_Opd_Hdr_Type;

// allow for 32/64 bit
typedef struct MPIP_Op_Time_Type {
    int iv_sec;
    int iv_usec;
} MPIP_Op_Time_Type;

// op data
typedef struct MPIP_Op_Type {
    MPIP_Op_Time_Type iv_tv;      // time
    int               iv_tid;     // tid
    char              iv_op;      // op
    char              iv_pad[3];  // pad
    int               iv_size;    // bytes-transferred
} MPIP_Op_Type;

// opd data
typedef struct MPIP_Opd_Type {
    MPIP_Op_Time_Type iv_tv;      // time           off-0
    int               iv_tid;     // tid            off-8
    char              iv_op;      // op             off-12
    char              iv_flag;    // flag           off-13
    char              iv_pad[2];  // pad            off-14
    long long         iv_comm;    // communicator   off-16
    long long         iv_buf;     // buf            off-24
    int               iv_tag;     // tag            off-32
    int               iv_rank;    // rank           off-36
    int               iv_count;   // count          off-40
    int               iv_error;   // error          off-44
} MPIP_Opd_Type;

#endif // !__SB_MPIP_H_
