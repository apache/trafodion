//------------------------------------------------------------------
//
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

//
// special defines:
//   see socktrans.cpp
//

//
// Encapsulate transport helpers
//
#ifndef __SB_SOCKTRANS_H_
#define __SB_SOCKTRANS_H_

#include "seabed/thread.h"

#include "msi.h"
#include "mseventmgr.h"
#include "slotmgr.h"
#include "transport.h"

namespace SB_Trans {
    //
    // Abstract Socket Event Handler (meant to be overriden)
    //
    class Sock_EH {
    public:
        Sock_EH();
        virtual ~Sock_EH();

        virtual void process_events(int pv_events) = 0;
    };

    class Sock_Comp_Thread;
    class Sock_Shutdown_EH;

    //
    // Socket controller
    //
    class Sock_Controller {
    public:
        Sock_Controller();
        virtual ~Sock_Controller();

        void        epoll_ctl(const char *pp_where,
                              int         pv_op,
                              int         pv_fd,
                              int         pv_event,
                              void       *pp_data);
        void        epoll_wait(const char *pp_where,
                               int         pv_timeout);
        void        lock();
        int         set_keepalive(const char *pp_where,
                                  int         pv_sock);
        int         set_nodelay(const char *pp_where,
                                int         pv_sock);
        int         set_nonblock(const char *pp_where,
                                 int         pv_sock);
        int         set_reuseaddr(const char *pp_where,
                                  int         pv_sock);
        int         set_size_recv(const char *pp_where,
                                  int         pv_sock,
                                  int         pv_size);
        int         set_size_send(const char *pp_where,
                                  int         pv_sock,
                                  int         pv_size);
        static void shutdown(const char *pp_where);
        void        shutdown_this(const char *pp_where);
        void        sock_add(const char *pp_where,
                             int         pv_sock,
                             Sock_EH    *pp_eh);
        void        sock_del(const char *pp_where,
                             int         pv_sock);
        void        sock_mod(const char *pp_where,
                             int         pv_sock,
                             int         pv_events,
                             Sock_EH    *pp_eh);
        void        unlock();

    private:
        friend class Sock_Comp_Thread;

        Sock_Comp_Thread  *ip_comp_thread;
        Sock_Shutdown_EH  *ip_shutdown_eh;
        SB_Thread::ECM     iv_ctlr_mutex;
        int                iv_efd;
    };

    class Sock_Shutdown_EH : public Sock_EH {
    public:
        Sock_Shutdown_EH();
        virtual ~Sock_Shutdown_EH();

        int          get_read_fd();
        int          get_write_fd();
        virtual void process_events(int pv_events);
        void         shutdown();

    private:
        int ia_fds[2];
    };

    //
    // Abstract socket user
    //
    class Sock_User {
    public:
        virtual void    destroy() = 0;
        virtual void    event_change(int      pv_events,
                                     Sock_EH *pp_eh) = 0;
        virtual void    event_init(Sock_EH *pp_eh) = 0;
        virtual int     get_sock() = 0;
        virtual ssize_t read(void   *pp_buf,
                             size_t  pv_count,
                             int    *pp_errno) = 0;
        virtual void    set_nonblock() = 0;
        virtual void    stop() = 0;
        virtual ssize_t write(const void *pp_buf,
                              size_t      pv_count,
                              int        *pp_errno) = 0;

    protected:
        Sock_User();
        virtual ~Sock_User();
    };

    //
    // Simple socket user
    //
    class Sock_User_Common : public Sock_User {
    public:
        virtual void    destroy();
        virtual void    event_change(int      pv_events,
                                     Sock_EH *pp_eh);
        virtual int     get_sock();
        virtual void    event_init(Sock_EH *pp_eh);
        virtual ssize_t read(void   *pp_buf,
                             size_t  pv_count,
                             int    *pp_errno);
        virtual void    set_nonblock();
        virtual void    stop();
        virtual ssize_t write(const void *pp_buf,
                              size_t      pv_count,
                              int        *pp_errno);

    protected:
        Sock_User_Common(const char *pp_where,
                         int         pv_sock);
        virtual ~Sock_User_Common();

        const char     *ip_where;
        int             iv_events;
        int             iv_sock;
        bool            iv_sock_added;
        SB_Thread::ECM  iv_sock_mutex;
    };

    //
    // Simple socket client
    //
    class Sock_Client : public Sock_User_Common {
    public:
        Sock_Client();
        virtual ~Sock_Client();

        int             connect(char *pp_host_port);
        int             connect(char *pp_host, int pv_port);
        void            set_sdp(bool pv_sdp);

    private:
        Sock_Client(int pv_sock);
        bool            iv_sdp;
    };

    class Sock_Server;

    //
    // Simple socket listener
    //
    class Sock_Listener {
    public:
        Sock_Listener();
        virtual ~Sock_Listener();

        Sock_Server *accept();
        void         destroy();
        void         get_addr(char *pp_host, int  *pp_port);
        void         listen(char *pp_host, int  *pp_port);
        void         set_sdp(bool pv_sdp);

    private:
        enum { MAX_HOST = 256 };
        char        ia_host[MAX_HOST];
        int         iv_port;
        bool        iv_sdp;
        int         iv_sock;
    };

    //
    // Simple socket server
    //
    class Sock_Server : public Sock_User_Common {
    public:
        Sock_Server(int pv_sock);
        virtual ~Sock_Server();
    };


    class Sock_Stream;
    class Sock_Stream_Accept_Thread;
    class Sock_Stream_Helper_Thread;

    class Sock_Stream_EH : public Sock_EH {
    public:
        Sock_Stream_EH(Sock_Stream *pp_stream);
        virtual ~Sock_Stream_EH();

        void         eh_lock();
        void         eh_unlock();
        virtual void process_events(int pv_events);
        virtual void set_stream(Sock_Stream *pp_stream);

    private:
        Sock_Stream         *ip_stream;
        SB_Thread::ECM       iv_eh_mutex;
    };

    //
    // Socket Stream engine
    //
    class Sock_Stream : public Trans_Stream {
    private:
        SB_Ecid_Type iv_aecid_sock_stream; // should be first instance

    public:
        enum { MAX_STREAMS            = 1024+100 }; // TODO
        enum { MAX_HOST = 256 };
        static Sock_Stream *create();
        static Sock_Stream *create(const char           *pp_name,
                                   const char           *pp_pname,
                                   const char           *pp_prog,
                                   bool                  pv_ic,
                                   Sock_User            *pp_sock,
                                   SB_Mon_Cb_Type        pv_mon_callback,
                                   SB_Mon_Cb_Type        pv_mon_unsol_callback,
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

        friend class Sock_Stream_Accept_Thread;
        friend class Sock_Stream_EH;
        friend class Sock_Stream_Helper_Thread;

        static int          check_streams();
        static void         close_stream(Sock_Stream *pp_stream, bool pv_local);
        static void         close_streams(bool pv_join);
        // execute functions
        virtual short       exec_abandon(MS_Md_Type *pp_md,
                                         int         pv_reqid,
                                         int         pv_can_reqid);
        virtual short       exec_abandon_ack(MS_Md_Type *pp_md,
                                             int         pv_reqid,
                                             int         pv_can_ack_reqid);
        virtual short       exec_close(MS_Md_Type *pp_md,
                                       int         pv_reqid);
        virtual short       exec_close_ack(MS_Md_Type *pp_md,
                                           int         pv_reqid);
        virtual short       exec_conn_ack(MS_Md_Type *pp_md,
                                          int         pv_reqid);
        virtual short       exec_open(MS_Md_Type *pp_md,
                                      int         pv_reqid);
        virtual short       exec_open_ack(MS_Md_Type *pp_md,
                                          int         pv_reqid);
        virtual short       exec_reply(MS_Md_Type *pp_md,
                                       int         pv_src,
                                       int         pv_dest,
                                       int         pv_reqid,
                                       short      *pp_req_ctrl,
                                       int         pv_req_ctrl_size,
                                       char       *pp_req_data,
                                       int         pv_req_data_size,
                                       short       pv_fserr);
        virtual short       exec_reply_nack(MS_Md_Type *pp_md,
                                            int         pv_reqid);
        virtual short       exec_reply_nw(MS_Md_Type *pp_md,
                                          int         pv_src,
                                          int         pv_dest,
                                          int         pv_reqid,
                                          short      *pp_req_ctrl,
                                          int         pv_req_ctrl_size,
                                          char       *pp_req_data,
                                          int         pv_req_data_size);
        virtual short       exec_wr(MS_Md_Type *pp_md,
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
                                    int         pv_opts);
        static int          open_port(char *pp_port);
        static void         shutdown();
        virtual int         start_stream();
        virtual void        stop_recv();
        virtual void        stop_completions();

    protected:
        virtual void       close_this(bool pv_local, bool pv_lock, bool pv_sem);
        virtual void       comp_sends();
        short              exec_com_chk(const char *pp_where);
        void               exec_com_error(MS_Md_Op_Type  pv_op,
                                          MS_Md_Type    *pp_md,
                                          short          pv_fserr);
        void               exec_com_init_simple(MS_Md_Type *pp_md,
                                                int         pv_reqid,
                                                int         pv_pri);
        void               exec_com_wait_send_done(const char *pp_where,
                                                   MS_Md_Type *pp_md);
        void               process_events(int pv_events);

    private:
        // read sections
        typedef enum { RSECTION_HDR            = 0,
                       RSECTION_CTRL           = 1,
                       RSECTION_DATA           = 2
        } RSection;
        // read states
        typedef enum { RSTATE_INIT             = 0,
                       RSTATE_RCV              = 1,
                       RSTATE_RCVING           = 2,
                       RSTATE_RCVD             = 3,
                       RSTATE_ERROR            = 4,
                       RSTATE_EOF              = 5,
                       RSTATE_LAST             = 6
        } RState;
        // read info
        typedef struct RInfo_Type {
            RSection     iv_section;
            RState       iv_state;
            Rd_Type      iv_rd;
            ssize_t      iv_recv_size;
            ssize_t      iv_recv_count;
            char        *ip_buf;
            MS_Md_Type  *ip_md;
        } RInfo_Type;
        enum { SEND_DONE_HDR  = 1,
               SEND_DONE_CTRL = 2,
               SEND_DONE_DATA = 4,
               SEND_DONE      = SEND_DONE_HDR | SEND_DONE_CTRL | SEND_DONE_DATA
        };
        // write sections
        typedef enum { SSECTION_HDR            = 0,
                       SSECTION_CTRL           = 1,
                       SSECTION_DATA           = 2
        } SSection;
        // write states
        typedef enum { SSTATE_INIT             = 0,
                       SSTATE_SEND             = 1,
                       SSTATE_SENDING          = 2,
                       SSTATE_SENT             = 3,
                       SSTATE_DONE             = 4,
                       SSTATE_ERROR            = 5,
                       SSTATE_LAST             = 6
        } WState;

        Sock_Stream();
        Sock_Stream(const char           *pp_name,
                    const char           *pp_pname,
                    const char           *pp_prog,
                    bool                  pv_ic,
                    Sock_User            *pp_sock,
                    SB_Mon_Cb_Type        pv_mon_callback,
                    SB_Mon_Cb_Type        pv_mon_unsol_callback,
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
        ~Sock_Stream();

        void          close_sock();
        static bool   get_ib_addr(void *pp_addr);
        void          process_ctrl(MS_Md_Type *pp_md, bool *pp_cont);
        void          process_hdr(MS_Md_Type **ppp_md, bool *pp_cont);
        void          recv_buf_cont(RInfo_Type *pp_r,
                                    bool       *pp_cont);
        void          recv_buf_init(RInfo_Type *pp_r,
                                    void       *pp_buf,
                                    size_t      pv_count,
                                    bool       *pp_cont);
        virtual void  send_abandon_ack(MS_Md_Type *pp_md,
                                       int         pv_reqid,
                                       int         pv_can_ack_reqid);
        void          send_buf_cont(MS_SS_Type *pp_s,
                                    bool       *pp_cont);
        void          send_buf_init(MS_SS_Type *pp_s,
                                    void       *pp_buf,
                                    size_t      pv_count,
                                    bool       *pp_cont);
        int           send_md(MS_Md_Op_Type  pv_op,
                              int            pv_hdr_type,
                              int            pv_state,
                              int            pv_opts,
                              MS_Md_Type    *pp_md,
                              short          pv_fserr);
        void          send_sm();
        void          sock_free();
        void          sock_lock();
        void          sock_unlock();
        void          stop_sock();

        static Sock_Stream_Accept_Thread *cp_accept_thread;
        static Sock_Stream_Helper_Thread *cp_helper_thread;
        static Sock_Listener             *cp_listener;
        static bool                       cv_mon_stream_set;
        static SB_Ts_Imap                 cv_stream_map;
        MS_Md_Type                       *ip_send_md;
        Sock_User                        *ip_sock;
        Sock_Stream_EH                   *ip_sock_eh;
        RInfo_Type                        iv_r;
        bool                              iv_close_ind_sent;
        SB_Thread::ECM                    iv_send_mutex;
        int                               iv_seq;
        int                               iv_sock;
        int                               iv_sock_errored;
        SB_Thread::ECM                    iv_sock_mutex;
        bool                              iv_stopped;
    };

    typedef bool (*SB_Mon_Sock_Accept_Cb_Type)(void *pp_sock);

    //
    // Accept thread.
    //
    class Sock_Stream_Accept_Thread : public SB_Thread::Thread {
    public:
        Sock_Stream_Accept_Thread(const char                 *pp_name,
                                  const char                 *pp_port,
                                  Sock_Stream                *pp_sock_stream,
                                  SB_Mon_Sock_Accept_Cb_Type  pv_cb);
        virtual ~Sock_Stream_Accept_Thread();

        void fin();
        void run();
        bool running();

    private:
        const char                 *ip_port;
        Sock_Stream                *ip_sock_stream;
        SB_Mon_Sock_Accept_Cb_Type  iv_cb;
        bool                        iv_fin;
        bool                        iv_running;
    };

    //
    // Completion thread.
    //
    class Sock_Comp_Thread : public SB_Thread::Thread {
    public:
        Sock_Comp_Thread(const char *pp_name);
        virtual ~Sock_Comp_Thread();

        void fin();
        void run();
        bool running();

    private:
        bool iv_fin;
        bool iv_running;
    };

    //
    // Helper thread.
    //
    class Sock_Stream_Helper_Thread : public SB_Thread::Thread {
    public:
        Sock_Stream_Helper_Thread(const char  *pp_name);
        virtual ~Sock_Stream_Helper_Thread();

        void add(MS_Md_Type *pp_md);
        void fin();
        void run();
        bool running();

    private:
        bool          iv_fin;
        MS_Md_Type    iv_md;
        SB_Sig_Queue  iv_q;
        bool          iv_running;
    };
}

#endif // !__SB_SOCKTRANS_H_

