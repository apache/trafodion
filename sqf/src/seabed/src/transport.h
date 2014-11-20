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

//
// Base Transport
//
#ifndef __SB_TRANSPORT_H_
#define __SB_TRANSPORT_H_

#include "common/evl_sqlog_eventnum.h"

#include "seabed/log.h"

#include "buf.h"
#include "ecid.h"
#ifndef SQ_PHANDLE_VERIFIER
#include "llmap.h"
#endif
#include "logaggr.h"
#include "mapmd.h"
#include "msi.h"
#include "mstrace.h"
#ifdef SQ_PHANDLE_VERIFIER
#include "npvmap.h"
#endif
#include "recvq.h"
#include "sbconst.h"
#include "tablemgr.h"
#include "timermap.h"
#include "utilatomic.h"

namespace SB_Trans {
    enum { MS_TAG_MON_MIN   = 22 };
    enum {
        MS_OPTS_FSDONE   = 0x0001,
        MS_OPTS_LDONE    = 0x0002,
        MS_OPTS_FSREQ    = 0x0020,
        MS_OPTS_MSIC     = 0x0040, // message-system interceptor
        MS_OPTS_PROCDEAD = 0x0080,
        MS_OPTS_CALLOC   = 0x0100,
        MS_OPTS_DALLOC   = 0x0200,
        MS_OPTS_FSDONETS = 0x0400
    };

    class Md_Table_Entry_Mgr : public SB_Table_Entry_Mgr<MS_Md_Type> {
    public:
        Md_Table_Entry_Mgr() {
        }

        MS_Md_Type *entry_alloc(size_t pv_inx) {
            pv_inx = pv_inx; // touch
            SB_util_abort("NOT implemented"); // sw fault
            return NULL;
        }

        void entry_alloc_block(MS_Md_Type **ppp_table,
                               size_t       pv_inx,
                               size_t       pv_count) {
            const char   *WHERE = "Md_Table_Entry_Mgr::entry_alloc_block";
            MS_Md_Type   *lp_md;
            SB_Buf_Lline  lv_buf;
            size_t        lv_inx;
            size_t        lv_max;

            lv_max = pv_inx + pv_count;
            if (pv_inx) {
                sb_log_aggr_cap_log(WHERE,
                                    SB_LOG_AGGR_CAP_MD_TABLE_CAP_INC,
                                    static_cast<int>(lv_max));
                if (gv_ms_trace || gv_ms_trace_md)
                    trace_where_printf(WHERE,
                                       "MD-table capacity increased new-cap=%d\n",
                                       static_cast<int>(lv_max));
            }
            for (lv_inx = pv_inx; lv_inx < lv_max; lv_inx++) {
                lp_md = new MS_Md_Type;
                lp_md->iv_inuse = false;
                lp_md->iv_msgid = static_cast<int>(lv_inx);
                lp_md->iv_link.iv_id.i = static_cast<int>(lv_inx);
                ppp_table[lv_inx] = lp_md;
            }
        }

        void entry_cap_change(size_t pv_inx, size_t pv_cap) {
            pv_inx = pv_inx; // touch
            pv_cap = pv_cap; // touch
            SB_util_abort("NOT implemented"); // sw fault
        }

        bool entry_free(MS_Md_Type *pp_md, size_t pv_inx, bool pv_urgent) {
            pp_md = pp_md; // touch
            pv_inx = pv_inx; // touch
            pv_urgent = pv_urgent; // touch
            SB_util_abort("NOT implemented"); // sw fault
            return false;
        }

        void entry_free_block(MS_Md_Type **ppp_table,
                              size_t       pv_inx,
                              size_t       pv_count) {
            MS_Md_Type *lp_md;
            size_t      lv_inx;

            for (lv_inx = 0; lv_inx < pv_count; lv_inx++, pv_inx++) {
                lp_md = ppp_table[pv_inx];
                delete lp_md;
            }
        }

        void entry_inuse(MS_Md_Type *pp_md, size_t pv_inx, bool pv_inuse) {
            pv_inx = pv_inx; // touch
            pp_md->iv_inuse = pv_inuse;
        }

        void entry_print_str(MS_Md_Type *pp_mds,
                             size_t      pv_inx,
                             char       *pp_info) {
            pp_mds = pp_mds; // touch
            pv_inx = pv_inx; // touch
            pp_info = pp_info; // touch
            SB_util_abort("NOT implemented"); // sw fault
        }
    };
    typedef SB_Ts_Table_Mgr<MS_Md_Type> Md_Table_Mgr;

    class SB_IC_Msg_Mgr;

    //
    // message
    //
    class Msg_Mgr {
    public:
        friend class SB_IC_Msg_Mgr;

        static int          get_md(MS_Md_Type      **ppp_md,
                                   void             *pp_stream,
                                   SB_Ms_Event_Mgr  *pp_mgr,
                                   bool              pv_dir_send,
                                   short            *pp_fserr,
                                   const char       *pp_where,
                                   int               pv_md_state);
        static int          get_md_count_recv();
        static int          get_md_count_send();
        static int          get_md_count_total();
        static int          get_md_hi_recv();
        static int          get_md_hi_send();
        static int          get_md_hi_total();
        static int          get_md_max_recv();
        static int          get_md_max_send();
        static int          get_md_max_total();
        static void         log_high(const char *pp_where,
                                     int         pv_high,
                                     const char *pp_str,
                                     int         pv_cap_type);
        static MS_Md_Type  *map_to_md(int pv_msgid, const char *pp_where);
        static void         print_md(MS_Md_Type *pp_md);
        static void         put_md(int pv_msgid, const char *pp_why);
        static void         put_md_link(MS_Md_Type *pp_md, const char *pp_why);
        static void         reset_md(MS_Md_Type      *pp_md,
                                     void            *pp_stream,
                                     SB_Ms_Event_Mgr *pp_mgr,
                                     const char      *pp_where);
        static void         set_md_max_recv(int pv_count);
        static void         set_md_max_send(int pv_count);
        static void         set_stream(MS_Md_Type *pp_md,
                                       void       *pp_stream,
                                       const char *pp_where);
        static void         test_check_empty(); // for testing
        static void         test_set_md_count(int pv_count); // for testing
        static int          trace_inuse_md_count();
        static void         trace_inuse_mds();

    private:
        enum { LOG_HIGH = 100 }; // log every time high moves by this much
        Msg_Mgr();
        ~Msg_Mgr();
        static int init();

        static Md_Table_Mgr       cv_md_table;
        static SB_Atomic_Int      cv_md_table_count_recv;
        static SB_Atomic_Int      cv_md_table_count_send;
        static SB_Atomic_Int      cv_md_table_count_total;
        static Md_Table_Entry_Mgr cv_md_table_entry_mgr;
        static int                cv_md_table_hi_recv;
        static int                cv_md_table_hi_send;
        static int                cv_md_table_hi_total;
        static int                cv_md_table_inx;
        static int                cv_md_table_max_recv;
        static int                cv_md_table_max_send;
        static int                cv_md_table_max_total;
    };

    // callbacks
    typedef void (*SB_Mon_Cb_Type)(MS_Md_Type *);
    typedef void (*SB_Comp_Cb_Type)(MS_Md_Type *);
    // abandon
    typedef void (*SB_Ab_Comp_Cb_Type)(MS_Md_Type *, bool);
    // inline open/close
    typedef void (*SB_ILOC_Comp_Cb_Type)(MS_Md_Type *, void *);
    // lim-queue
    typedef int  (*SB_Lim_Cb_Type)(int, short *, int, char *, int);

    //
    // Abstract stream
    //
    class Stream_Base {
    public:
        //                         error of generation?
        virtual bool               error_of_generation(int pv_generation) = 0;
        //                         error sync
        virtual void               error_sync() = 0;
        //                         execute functions
        virtual short              exec_abandon(MS_Md_Type *pp_md,
                                                int         pv_reqid,
                                                int         pv_can_reqid) = 0;
        virtual short              exec_abandon_ack(MS_Md_Type *pp_md,
                                                    int         pv_reqid,
                                                    int         pv_can_ack_reqid) = 0;
        virtual short              exec_close(MS_Md_Type *pp_md,
                                              int         pv_reqid) = 0;
        virtual short              exec_close_ack(MS_Md_Type *pp_md,
                                                  int         pv_reqid) = 0;
        virtual short              exec_conn_ack(MS_Md_Type *pp_md,
                                                 int         pv_reqid) = 0;
        virtual short              exec_open(MS_Md_Type *pp_md,
                                             int         pv_reqid) = 0;
        virtual short              exec_open_ack(MS_Md_Type *pp_md,
                                                 int         pv_reqid) = 0;
        virtual short              exec_reply(MS_Md_Type *pp_md,
                                              int         pv_src,
                                              int         pv_dest,
                                              int         pv_reqid,
                                              short      *pp_req_ctrl,
                                              int         pv_req_ctrl_size,
                                              char       *pp_req_data,
                                              int         pv_req_data_size,
                                              short       pv_fserr) = 0;
        virtual short              exec_reply_nack(MS_Md_Type *pp_md,
                                                   int         pv_reqid) = 0;
        virtual short              exec_reply_nw(MS_Md_Type *pp_md,
                                                 int         pv_src,
                                                 int         pv_dest,
                                                 int         pv_reqid,
                                                 short      *pp_req_ctrl,
                                                 int         pv_req_ctrl_size,
                                                 char       *pp_req_data,
                                                 int         pv_req_data_size) = 0;
        virtual short              exec_wr(MS_Md_Type *pp_md,
                                           int         pv_src,
                                           int         pv_dest,
                                           int         pv_reqid,
                                           int         pv_pri,
                                           short      *pp_req_ctrl,
                                           int         pv_req_ctrl_size,
                                           char       *pp_req_data,
                                           int         pv_req_data_size,
                                           short      *pp_rep_ctrl,
                                           int         pv_rep_max_ctrl_size,
                                           char       *pp_rep_data,
                                           int         pv_rep_max_data_size,
                                           int         pv_opts) = 0;
        //                         free reply-md
        virtual void               free_reply_md(MS_Md_Type *pp_md) = 0;
        //                         idle-timer-link
        virtual SB_TML_Type       *get_idle_timer_link() = 0;
        //                         stream-name
        virtual char              *get_name() = 0;
        //                         open-id
        virtual int                get_oid() = 0;
        //                         node-id
        virtual int                get_open_nid() = 0;
        //                         process-id
        virtual int                get_open_pid() = 0;
        //                         node-id
        virtual int                get_remote_nid() = 0;
        //                         process-id
        virtual int                get_remote_pid() = 0;
#ifdef SQ_PHANDLE_VERIFIER
        //                         verif
        virtual SB_Verif_Type      get_remote_verif() = 0;
#endif
        //                         is self?
        virtual bool               is_self() = 0;
        //                         remove comp-q
        virtual bool               remove_comp_q(MS_Md_Type *pp_md) = 0;
        //                         start
        virtual int                start_stream() = 0;
        //                         stop-completions
        virtual void               stop_completions() = 0;
        //                         request-done
        virtual int                wait_req_done(MS_Md_Type *pp_md) = 0;
        //                         reply-done
        virtual void               wait_rep_done(MS_Md_Type *pp_md) = 0;

    protected:
        Stream_Base();
        virtual ~Stream_Base();
    };

    // read descriptor
    typedef struct Rd_Type {
        char          *ip_ctrl;            // ctrl buf
        char          *ip_data;            // data buf
        const char    *ip_where;           // where
        int            iv_ctrl_len;        // ctrl buf len
        int            iv_data_len;        // data buf len
        MS_PMH_Type    iv_hdr;             // header
        int            iv_mpi_source_rank; // MPI transport source rank
        int            iv_mpi_tag;         // MPI transport tag
        int            iv_sm_tag;          // sm transport tag
    } Rd_Type;

    //
    // Transport stream
    //
    class Trans_Stream : public Stream_Base {
    private:
        SB_Ecid_Type iv_aecid_trans_stream; // should be first instance

    public:
        enum { MAX_STREAMS_DEFAULT = 16 };

        enum {
            INT_MPI_ERR_EXITED = -1 // internal MPI error
        };
        //                        add accept-count
        static void               add_stream_acc_count(int pv_val);
        //                        add connect-count
        static void               add_stream_con_count(int pv_val);
        //                        check-streams
        static int                check_streams();
        //                        close-stream
        static void               close_stream(const char   *pp_where,
                                               Trans_Stream *pp_stream,
                                               bool          pv_local,
                                               bool          pv_lock,
                                               bool          pv_sem);
        //                        close-nidpid-streams
        static void               close_nidpid_streams(bool pv_lock);
        //                        delete-streams
        static void               delete_streams(bool pv_ref_zero);
        //                        error of generation?
        virtual bool              error_of_generation(int pv_generation);
        //                        error sync
        virtual void              error_sync();
        //                        finish reply
        static void               finish_reply_static(MS_Md_Type      *pp_md,
                                                      short            pv_fserr,
                                                      bool             pv_harderr,
                                                      int              pv_generation,
                                                      SB_Ts_Md_Map    *pp_req_map,
                                                      bool             pv_req_map_lock,
                                                      bool             pv_self,
                                                      SB_Comp_Cb_Type  pv_ms_comp_callback);
        //                        free reply-md
        virtual void              free_reply_md(MS_Md_Type *pp_md);
        //                        idle-timer-link
        virtual SB_TML_Type      *get_idle_timer_link();
        //                        mon-stream
        static Trans_Stream      *get_mon_stream();
        //                        stream-name
        virtual char             *get_name();
        //                        get accept-count
        static int                get_stream_acc_count();
        //                        get accept-hi-count
        static int                get_stream_acc_hi_count();
        //                        get connect-count
        static int                get_stream_con_count();
        //                        get connect-hi-count
        static int                get_stream_con_hi_count();
        //                        get total-count
        static int                get_stream_total_count();
        //                        get total-hi-count
        static int                get_stream_total_hi_count();
        //                        open-id
        virtual int               get_oid();
        //                        is self?
        virtual bool              is_self();
        //                        node-id
        virtual int               get_open_nid();
        //                        process-id
        virtual int               get_open_pid();
        //                        node-id
        virtual int               get_remote_nid();
        //                        process-id
        virtual int               get_remote_pid();
#ifdef SQ_PHANDLE_VERIFIER
        //                        verif
        virtual SB_Verif_Type     get_remote_verif();
#endif
        //                        map nid/pid to stream
#ifdef SQ_PHANDLE_VERIFIER
        static Trans_Stream      *map_nidpid_key_to_stream(SB_NPV_Type pv_npv,
                                                           bool        pv_lock);
#else
        static Trans_Stream      *map_nidpid_key_to_stream(SB_Int64_Type pv_nidpid,
                                                           bool pv_lock);
#endif
        //                        get nid/pid map keys
#ifdef SQ_PHANDLE_VERIFIER
        static SB_NPVmap_Enum    *map_nidpid_keys();
#else
        static SB_LLmap_Enum     *map_nidpid_keys();
#endif
        //                        lock nid/pid map
        static void               map_nidpid_lock();
        //                        remove stream from nid/pid map
        bool                      map_nidpid_remove(bool pv_lock);
        //                        get nid/pid map size
        static int                map_nidpid_size();
        //                        map nid/pid to stream
        static Trans_Stream      *map_nidpid_to_stream(int           pv_nid,
                                                       int           pv_pid,
#ifdef SQ_PHANDLE_VERIFIER
                                                       SB_Verif_Type pv_verif,
#endif
                                                       bool          pv_lock);
        //                        trylock nid/pid map
        static int                map_nidpid_trylock();
        //                        unlock nid/pid map
        static void               map_nidpid_unlock();
        //                        dec md-ref
        int                       md_ref_dec();
        //                        get md-ref
        int                       md_ref_get();
        //                        inc md-ref
        void                      md_ref_inc();
        //                        set md-ref
        void                      md_ref_set(int pv_val);
        //                        get post-mon
        bool                      post_mon_messages_get();
        //                        set post-mon
        void                      post_mon_messages_set(bool pv_post_mon_messages);
        //                        lock post-mon
        void                      post_mon_mutex_lock();
        //                        unlock post-mon
        void                      post_mon_mutex_unlock();
        //                        copy
        static void               recv_copy(const char   *pp_where,
                                            char        **ppp_source,
                                            int           pv_source_count,
                                            char         *pp_dest,
                                            int           pv_count,
                                            int          *pp_rc,
                                            bool          pv_source_del);
        //                        dec stream-ref
        int                       ref_dec(const char *pp_where,
                                          void *pp_referee);
        //                        get stream-ref
        int                       ref_get();
        //                        inc stream-ref
        int                       ref_inc(const char *pp_where,
                                          void *pp_referee);
        //                        set stream-ref
        void                      ref_set(int pv_val);
        //                        remove comp-q
        virtual bool              remove_comp_q(MS_Md_Type *pp_md);
        //                        remove comp-q
        static  bool              remove_comp_q_static(MS_Md_Type *pp_md);
        //                        mon-stream
        static void               set_mon_stream(Trans_Stream *pp_stream);
        //                        pname
        void                      set_pname(const char *pp_pname);
        //                        prog
        void                      set_prog(const char *pp_prog);
        //                        shutdown
        static void               set_shutdown(bool pv_shutdown);
        //                        request-done
        virtual int               wait_req_done(MS_Md_Type *pp_md);
        //                        request-done
        static  int               wait_req_done_static(MS_Md_Type *pp_md);
        //                        reply-done
        virtual void              wait_rep_done(MS_Md_Type *pp_md);

    protected:
        enum {
            STREAM_TYPE_MPI = 1,
            STREAM_TYPE_SELF,
            STREAM_TYPE_SM,
            STREAM_TYPE_SOCK
        };
        Trans_Stream(int pv_stream_type);
        Trans_Stream(int                   pv_stream_type,
                     const char           *pp_name,
                     const char           *pp_pname,
                     const char           *pp_prog,
                     bool                  pv_ic,
                     SB_Comp_Cb_Type       pv_ms_comp_callback,
                     SB_Ab_Comp_Cb_Type    pv_ms_abandon_callback,
                     SB_ILOC_Comp_Cb_Type  pv_ms_oc_callback,
                     SB_Lim_Cb_Type        pv_ms_lim_callback,
                     SB_Recv_Queue        *pp_ms_recv_q,
                     SB_Recv_Queue        *pp_ms_lim_q,
                     SB_Ms_Tl_Event_Mgr   *pp_event_mgr,
                     int                   pv_open_nid,
                     int                   pv_open_pid,
#ifdef SQ_PHANDLE_VERIFIER
                     SB_Verif_Type         pv_open_verif,
#endif
                     int                   pv_opened_nid,
                     int                   pv_opened_pid,
#ifdef SQ_PHANDLE_VERIFIER
                     SB_Verif_Type         pv_opened_verif,
#endif
                     int                   pv_opened_type);
        virtual ~Trans_Stream();
        void                      add_msgid_to_reqid_map(int pv_reqid,
                                                         int pv_msgid);
        void                      add_reply_piggyback(MS_Md_Type *pp_md);
        void                      add_stream_for_close();
        static void               checksum_recv_check(Rd_Type     *pp_rd,
                                                      MS_PMH_Type *pp_hdr,
                                                      bool         pv_client);
        static void               checksum_send_check(MS_Md_Type *pp_md,
                                                      bool        pv_reply);
        virtual void              close_this(bool pv_local,
                                             bool pv_lock,
                                             bool pv_sem) = 0;
        virtual void              comp_sends() = 0;
        void                      finish_abandon(MS_Md_Type *pp_md);
        void                      finish_close(MS_Md_Type *pp_md);
        void                      finish_conn(MS_Md_Type *pp_md);
        void                      finish_open(MS_Md_Type *pp_md);
        void                      finish_recv(Rd_Type     *pp_rd,
                                              MS_PMH_Type *pp_hdr,
                                              bool         pv_client);
        void                      finish_recv_server_fsreq(Rd_Type     *pp_rd,
                                                           MS_PMH_Type *pp_hdr);
        void                      finish_recv_server_msinterceptor(Rd_Type     *pp_rd,
                                                                   MS_PMH_Type *pp_hdr);
        void                      finish_recv_server_normal(Rd_Type     *pp_rd,
                                                            MS_PMH_Type *pp_hdr);
        void                      finish_reply(MS_Md_Type *pp_md,
                                               short       pv_fserr,
                                               bool        pv_harderr,
                                               bool        pv_lock);
        void                      finish_send(const char *pp_where,
                                              const char *pp_id,
                                              MS_Md_Type *pp_md,
                                              short       pv_fserr,
                                              bool        pv_harderr);
        void                      finish_writereads(short pv_fserr);
        void                      get_hdr_type(int   pv_hdr_type,
                                               char *pp_hdr_type);
        static bool               get_shutdown();
        bool                      get_thread_marker();
        void                      init(const char *pp_name,
                                       const char *pp_pname,
                                       const char *pp_prog);
        void                      map_nidpid_add_stream(int pv_nid,
                                                        int pv_pid
#ifdef SQ_PHANDLE_VERIFIER
                                                       ,SB_Verif_Type pv_verif
#endif
                                                       );
        void                      map_nidpid_remove_stream(const char *pp_where,
                                                           int         pv_nid,
                                                           int         pv_pid
#ifdef SQ_PHANDLE_VERIFIER
                                                          ,SB_Verif_Type pv_verif
#endif
                                                          );
        void                      process_abandon(const char *pp_where,
                                                  int         pv_abandon_reqid,
                                                  int         pv_req_reqid);
        void                      remove_msgid_from_reqid_map(int pv_reqid,
                                                              bool pv_lock);
        bool                      remove_recv_q(MS_Md_Type *pp_md);
        MS_Md_Type               *remove_reply_piggyback();
        virtual void              send_abandon_ack(MS_Md_Type *pp_md,
                                                   int         pv_reqid,
                                                   int         pv_can_ack_reqid);
        virtual void              send_close_ack(MS_Md_Type *pp_md,
                                                int          pv_reqid);
        virtual void              send_close_ind();
        virtual void              send_conn_ack(MS_Md_Type *pp_md,
                                                int         pv_reqid);
        virtual void              send_open_ack(MS_Md_Type *pp_md,
                                                int         pv_reqid);
        virtual void              send_reply_nack(MS_Md_Type *pp_md,
                                                  int          pv_reqid);
        void                      set_basic_md(Rd_Type     *pp_rd,
                                               MS_PMH_Type *pp_hdr,
                                               MS_Md_Type  *pp_md,
                                               bool        pv_mon_msg);
        void                      set_stream_name();
        void                      set_thread_marker(bool pv_marker);
        void                      stop_stream();
        static void               trace_close_del_q(const char *pp_where);

protected:
        enum { MAX_NAME = 200 };
        static Trans_Stream      *cp_mon_stream;
        static SB_Thread::ECM     cv_cap_mutex;
        static SB_Thread::ECM     cv_close_mutex;
        static SB_D_Queue         cv_close_q;
        static SB_Thread::ECM     cv_del_mutex;
        static SB_D_Queue         cv_del_q;
        static bool               cv_shutdown;
        static SB_Atomic_Int      cv_stream_acc_count;
        static SB_Atomic_Int      cv_stream_acc_hi_count;
        static SB_Atomic_Int      cv_stream_con_count;
        static SB_Atomic_Int      cv_stream_con_hi_count;
        static SB_Atomic_Int      cv_stream_total_count;
        static SB_Atomic_Int      cv_stream_total_hi_count;
        char                      ia_stream_name[MAX_NAME];
        char                      ia_pname[MAX_NAME];
        char                      ia_prog[SB_MAX_PROG];
        SB_Ms_Tl_Event_Mgr       *ip_event_mgr;
        SB_Recv_Queue            *ip_ms_lim_q;
        SB_Recv_Queue            *ip_ms_recv_q;
        SB_DQL_Type               iv_close_link;
        SB_DQL_Type               iv_del_link;
        SB_Thread::ECM            iv_error_mutex;
        int                       iv_generation;
        SB_Thread::ECM            iv_generation_mutex;
        bool                      iv_ic; // interceptor stream?
        SB_TML_Type               iv_idle_timer_link;
        SB_Atomic_Int             iv_md_ref_count;
        SB_Ab_Comp_Cb_Type        iv_ms_abandon_callback;
        SB_Comp_Cb_Type           iv_ms_comp_callback;
        SB_Lim_Cb_Type            iv_ms_lim_callback;
        SB_ILOC_Comp_Cb_Type      iv_ms_oc_callback;
        SB_Ts_Imap                iv_msgid_reqid_map;
        int                       iv_open_nid;
        int                       iv_open_pid;
#ifdef SQ_PHANDLE_VERIFIER
        SB_Verif_Type             iv_open_verif;
#endif
        int                       iv_opened_nid;
        int                       iv_opened_pid;
#ifdef SQ_PHANDLE_VERIFIER
        SB_Verif_Type             iv_opened_verif;
#endif
        int                       iv_opened_type;
        int                       iv_remote_nid;
        int                       iv_remote_pid;
#ifdef SQ_PHANDLE_VERIFIER
        SB_Verif_Type             iv_remote_verif;
#endif
        bool                      iv_post_mon_messages;
        SB_Thread::ECM            iv_post_mutex;
        SB_Atomic_Int             iv_ref_count;
        SB_Ts_Md_Map              iv_rep_map;
        SB_D_Queue                iv_reply_piggyback_q;
        SB_Thread::ECM            iv_reply_piggyback_mutex;
        SB_Ts_Md_Map              iv_req_map;

    private:
        int                       iv_oid;
        int                       iv_stream_type;
    };

}

#endif // !__SB_TRANSPORT_H_
