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

#ifndef __SB_MSI_H_
#define __SB_MSI_H_

#include <sys/time.h>

#include "seabed/int/opts.h"

#include "imap.h"
#include "mseventmgr.h"
#include "util.h"

enum {
    MS_PMH_SIG0             = 'M',
    MS_PMH_SIG1             = 'A',
    MS_PMH_SIG2             = 'R',
    MS_PMH_SIG3             = 'K',
    MS_PMH_MAJOR            =  1,
    MS_PMH_MINOR            =  0,
    MS_PMH_OPT_FSREQ        =  8,
    MS_PMH_OPT_MSIC         = 16, // message-system interceptor
    MS_PMH_OPT2_C_ZERO      =  1,
    MS_PMH_OPT2_D_ZERO      =  2,
    MS_PMH_OPT2_C_IN_HDR    =  4,
    MS_PMH_OPT2_C_SEP       =  8,
    MS_PMH_OPT2_D_IN_HDR    = 16,
    MS_PMH_OPT2_D_SEP       = 32,
    MS_PMH_ORDER_BE         =  0,
    MS_PMH_ORDER_LE         =  1,
    MS_PMH_TYPE_INV         =  0,
    MS_PMH_TYPE_WR          =  1,
    MS_PMH_TYPE_REPLY       =  2,
    MS_PMH_TYPE_ABANDON     =  3,
    MS_PMH_TYPE_ABANDON_ACK =  4,
    MS_PMH_TYPE_CLOSE       =  5,
    MS_PMH_TYPE_CLOSE_ACK   =  6,
    MS_PMH_TYPE_OPEN        =  7,
    MS_PMH_TYPE_OPEN_ACK    =  8,
    MS_PMH_TYPE_REPLY_NACK  =  9,
    MS_PMH_TYPE_CONN        =  10,
    MS_PMH_TYPE_CONN_ACK    =  11
};

//
// header holds ctrl and/or data
//
enum {
#define HDR_DATA
#ifdef HDR_DATA
    MS_PMH_DATA_LEN = 256, // should be a multiple of 4
#else
    MS_PMH_DATA_LEN = 0,
  #define HDR_CTRL
#endif

// csize=96, dsize=1024 performance
// w/o HDR_CTRL 6.32 ms/loop, w/ 4.26 ms/loop
#ifdef HDR_CTRL
    MS_PMH_CTRL_LEN = 96, // open=148, wr=96
#else
    MS_PMH_CTRL_LEN = 0,
#endif

    MS_PMH_CTRL_DATA_LEN = (MS_PMH_CTRL_LEN > MS_PMH_DATA_LEN) ?
                            MS_PMH_CTRL_LEN : MS_PMH_DATA_LEN
};

// timestamp
typedef struct MS_PMH_TS_Type {
    int iv_sec;
    int iv_usec;
} MS_PMH_TS_Type;

// PMH = Proto Msg Hdr
typedef struct MS_PMH_Type {
    char  ia_sig[4];       // 0x00: see MS_PROTO_HDR_SIG1-4
    char  iv_maj;          // 0x04: major version
    char  iv_min;          // 0x05: minor version
    char  iv_order;        // 0x06: byte order
    char  iv_type;         // 0x07: type
    char  iv_ptype;        // 0x08: ptype
    char  iv_opts;         // 0x09: options
    short iv_opts2;        // 0x0a: opts2
    short iv_rsvd;         // 0x0c: rsvd
    short iv_fserr;        // 0x0e: fs err
    int   iv_reqid;        // 0x10: reqid
    int   iv_pri;          // 0x14: pri
    int   iv_tag;          // 0x18: tag
    int   iv_seq1;         // 0x1c: sequence #
    int   iv_seq2;         // 0x20: sequence #
    int   iv_csize;        // 0x24: control size
    int   iv_dsize;        // 0x28: data size
    int   iv_cmaxsize;     // 0x2c: control max size
    int   iv_dmaxsize;     // 0x30: data max size
    int   iv_chk_hdr;      // 0x34: chk (TODO: remove)
    int   iv_chk_ctrl;     // 0x38: chk (TODO: remove)
    int   iv_chk_data;     // 0x3c: chk (TODO: remove)
#ifdef USE_SEND_LAT
    long  ia_ts[2];        // 0x40: timestamp
#endif
    MS_PMH_TS_Type iv_ts1; // 0x40: link/reply
    MS_PMH_TS_Type iv_ts2; // 0x48: rcvd
    MS_PMH_TS_Type iv_ts3; // 0x50: listen
#ifdef HDR_CTRL
    char  ia_ctrl[MS_PMH_CTRL_LEN];
#endif
#ifdef HDR_DATA
    char  ia_data[MS_PMH_DATA_LEN]; // 0x58: data
#endif
} MS_PMH_Type;

enum {
    MS_PMH_HDR_SIZE = sizeof(MS_PMH_Type) -
                      MS_PMH_CTRL_LEN -
                      MS_PMH_DATA_LEN
};

enum MS_Md_Op_Type {
    MS_OP_WR          = 1,
    MS_OP_REPLY       = 2,
    MS_OP_FLUSH       = 3,
    MS_OP_ABANDON     = 4,
    MS_OP_ABANDON_ACK = 5,
    MS_OP_CLOSE       = 6,
    MS_OP_CLOSE_ACK   = 7,
    MS_OP_OPEN        = 8,
    MS_OP_OPEN_ACK    = 9,
    MS_OP_REPLY_NACK  = 10,
    MS_OP_REPLY_NW    = 11,
    MS_OP_CONN        = 12,
    MS_OP_CONN_ACK    = 13,
    MS_OP_XX          = 14
};

// place in ctrl
typedef struct Ms_Sm_Conn_Type {
    int           iv_ic;
    int           iv_nid;
    int           iv_pid;
#ifdef SQ_PHANDLE_VERIFIER
    SB_Verif_Type iv_verif;
#endif
    char          ia_pname[34];
    char          ia_prog[32];
    int           iv_fserr;
} Ms_Sm_Conn_Type;

typedef struct MS_SS_Type {
    char           *ip_buf;             // sock
    short          *ip_req_ctrl;
    char           *ip_req_data;
    int             iv_dest;
    int             iv_hdr_size;        // sock
    int             iv_pri;
    int             iv_rep_max_ctrl_size;
    int             iv_rep_max_data_size;
    int             iv_req_ctrl_size;
    int             iv_req_count;
    int             iv_req_data_size;
    int             iv_req_id;
    int             iv_section;         // sock
    int             iv_send_count;      // sock
    int             iv_send_done;       // send done?
    int             iv_send_size;       // sock
    int             iv_slot_ctrl;       // slot ctrl
    int             iv_slot_data;       // slot data
    int             iv_slot_hdr;        // slot hdr
    int             iv_src;
    int             iv_state;           // sock
    int             iv_tag_ctrl;        // tag ctrl
    int             iv_tag_data;        // tag data
    int             iv_tag_hdr;         // tag hdr
    MS_PMH_Type     iv_hdr;             // hdr
} MS_SS_Type; // send state

class SB_Comp_Queue;

// Md = Message Descriptor
typedef struct MS_Md_Type {
    // used by stream engine - preamble
    SB_DQL_Type      iv_link;             // MUST be first
    bool             iv_inuse;            // cb in use?
    // used by stream engine - simple vars
    bool             iv_break_done;       // break done?
    bool             iv_reply_done;       // reply done?
    int              iv_reply_done_temp;  // reply done? TODO-REMOVE!
    bool             iv_send_done;        // send done?
    bool             iv_abandoned;        // abandoned?
    bool             iv_self;             // self?
    bool             iv_inline;           // inline?
    bool             iv_free_md;          // free md
    int              iv_aa_reqid;         // abandon-ack reqid
    int              iv_aa_can_ack_reqid; // abandon-ack can-ack-reqid
    bool             iv_dir_send;         // direction?
    int              iv_md_state;         // state
    int              iv_msgid;            // msgid
    MS_Md_Op_Type    iv_op;               // operation
    int              iv_sm_queued_cnt;    // sm-queued-cnt
    int              iv_sm_recv_state;    // sm-recv-state
    int              iv_sm_rep_tag;       // sm-reply-tag
    long             iv_tag;              // tag
    long             iv_tid;              // thread id
    struct timeval   iv_ts_msg_cli_break; // break time
    struct timeval   iv_ts_msg_cli_link;  // link time
    struct timeval   iv_ts_msg_cli_rcvd;  // reply rcvd time
    struct timeval   iv_ts_msg_srv_listen;// listen time
    struct timeval   iv_ts_msg_srv_rcvd;  // link rcvd time
    struct timeval   iv_ts_msg_srv_reply; // reply time
    MS_SS_Type       iv_ss;               // send state
    // used by stream engine - pointers
    SB_Comp_Queue   *ip_comp_q;           // comp q
    SB_Comp_Queue   *ip_fs_comp_q;        // fs comp q
    SB_Ms_Event_Mgr *ip_mgr;              // event mgr
    MS_PMH_Type     *ip_sm_hdr;           // hdr // TODO: remove?
    void            *ip_stream;           // stream
    const char      *ip_where;            // where

    // stream engine fills this out
    struct out {
        short           iv_fserr;                // error?
        int             iv_fserr_generation;     // error generation?
        bool            iv_fserr_hard;           // hard error?
        bool            iv_ldone;                // ldone compl?
        bool            iv_mon_msg;              // mon msg?
        int             iv_msg_type;             // msg type
        int             iv_nid;                  // nid
        int             iv_opts;                 // opts
        int             iv_pid;                  // pid
        int             iv_pri;                  // pri
        int             iv_ptype;                // ptype
        bool            iv_reply;                // reply?
        bool            iv_requeue;              // re-queue msg?
        uint64_t        iv_sm_smid;              // smid
#ifdef SQ_PHANDLE_VERIFIER
        SB_Verif_Type   iv_verif;                // verif
#endif

        struct timeval  iv_comp_q_off_tod;       // comp queue off time
        struct timeval  iv_comp_q_on_tod;        // comp queue on time
        struct timeval  iv_recv_q_off_tod;       // receive queue of time
        struct timeval  iv_recv_q_on_tod;        // receive queue on time
        struct timeval  iv_reply_tod;            // reply time
        char           *ip_recv_ctrl;            // recv ctrl buf
        int             iv_recv_ctrl_max;        // recv control max size
        int             iv_recv_ctrl_size;       // recv control size
        char           *ip_recv_data;            // recv data buf
        int             iv_recv_data_max;        // recv data max size
        int             iv_recv_data_size;       // recv data size
        int             iv_recv_req_id;          // recv req id
        int             iv_recv_mpi_source_rank; // recv source rank
        int             iv_recv_mpi_tag;         // recv source tag
        int             iv_recv_seq1;            // recv seq #
        int             iv_recv_seq2;            // recv seq #
        int32_t         iv_recv_sm_node;         // node
        pid_t           iv_recv_sm_pid;          // pid
        int32_t         iv_recv_sm_tag;          // tag
        short          *ip_reply_ctrl;           // reply ctrl buf
        int             iv_reply_ctrl_max;       // reply ctrl max
        int             iv_reply_ctrl_count;     // reply ctrl count
        char           *ip_reply_data;           // reply data buf
        int             iv_reply_data_max;       // reply data max
        int             iv_reply_data_count;     // reply data count
    } out;
    // used by stream engine - non-simple vars (put at end for debug clarity)
    SB_Thread::ECM   iv_abandon_mutex;    // used for abandon
    SB_Thread::CV    iv_cv;               // used for signaling
    SB_Thread::CV    iv_cv2;              // used for signaling
    SB_Thread::MSL   iv_sl;               // used for wr
} MS_Md_Type;

typedef struct MS_Result_Raw_Type {
    unsigned int   rr_ctrlsize;
    unsigned int   rr_datasize;
    unsigned int   rrerr;
} MS_Result_Raw_Type;

// MESSAGE_HEADER (request)
typedef struct MS_MH_Req_Type {
    short          dialect_type;
    short          request_type;                    // reply_type
    short          request_version;                 // reply_version
    short          minimum_interpretation_version;  // error
    // request body goes here
} MS_MH_Req_Type;

// MESSAGE_HEADER (reply)
typedef struct MS_MH_Reply_Type {
    short          dialect_type;
    short          reply_type;
    short          reply_version;
    short          error;
    // reply body goes here
} MS_MH_Reply_Type;

enum {
    MS_DIALECT_MSG_ERROR      = 1,  // put in MS_MH_Reply_Type.dialect_type
    MS_MSG_ERROR_SIMPLE_REPLY = 3,  // put in MS_MH_Reply_Type.reply_type
    MS_MSG_ERROR_VERSION_D00  = 1   // put in MS_MH_Reply_Type.reply_version
};

typedef void * (*MS_Buf_Alloc_Cb_Type)(size_t len);
typedef void   (*MS_Buf_Free_Cb_Type)(void *buf);
typedef struct MS_Buf_Mgr_Type {
    union {
        MS_Buf_Alloc_Cb_Type  ialloc;
        void                 *ipalloc;
    } ua; // public alloc
    union {
        MS_Buf_Free_Cb_Type   ifree;
        void                 *ipfree;
    } uf; // public free
    union {
        MS_Buf_Alloc_Cb_Type  ialloc;
        void                 *ipalloc;
    } uai; // internal alloc
    union {
        MS_Buf_Free_Cb_Type   ifree;
        void                 *ipfree;
    } ufi; // internal free
} MS_Buf_Mgr_Type;

extern char           ga_ms_su_a_port[];
extern char           ga_ms_su_pname[];
extern char           ga_ms_su_pname_seq[];
extern char           ga_ms_su_prog[];
extern char          *gp_ms_trace_file;
extern char          *gp_ms_trace_file_dir;
extern char          *gp_ms_trace_prefix;
extern bool           gv_ms_su_called;
extern int            gv_ms_su_nid;
extern pthread_t      gv_ms_su_pthread_self;
extern int            gv_ms_su_pid;
extern int            gv_ms_su_pnid;
extern int            gv_ms_su_ptype;
extern char           ga_ms_su_sock_a_port[];
extern bool           gv_ms_su_sysmsgs;
#ifdef SQ_PHANDLE_VERIFIER
extern SB_Verif_Type  gv_ms_su_verif;
#endif

extern void  ms_buf_trace_change();
extern void  ms_od_get_my_phandle(SB_Phandle_Type *pp_phandle);
extern void *ms_od_lock(int pv_oid);
extern int   ms_od_map_phandle_to_oid(SB_Phandle_Type *pp_phandle);
extern void  ms_od_unlock(void *pp_od);
extern bool  ms_od_valid_oid(int pv_oid);
extern void  msg_send_self(SB_Phandle_Type *pp_phandle,
                           short           *pp_reqctrl,
                           int              pv_reqctrlsize,
                           char            *pp_reqdata,
                           int              pv_reqdatasize);
extern int   msg_set_recovery();
extern void  msg_trace_args(char  *pp_line,
                            char **ppp_argv,
                            int    pv_argc,
                            int    pv_len);

// uncomment to inline
//#define SB_MSG_INLINE_ENABLE

#ifdef SB_MSG_INLINE_ENABLE
  #define SB_MSG_INLINE inline
#else
  #define SB_MSG_INLINE
#endif

#ifdef SB_MSG_INLINE_ENABLE
#include "msi.inl"
#endif

#endif // !__SB_MSI_H_
