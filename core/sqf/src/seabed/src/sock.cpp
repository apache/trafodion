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

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/epoll.h>

#include "seabed/labels.h"
#include "seabed/labelmaps.h"
#include "seabed/trace.h"

#include "buf.h"
#include "mstrace.h"
#include "msx.h"
#include "socktrans.h"

#ifndef AF_INET_SDP
#define AF_INET_SDP 27
#endif

static const char *ga_sock_epoll_ctl_type_labels[] = {
    "ADD",
    "DEL",
    "MOD",
    SB_LABEL_END
};

enum {
    SB_LABEL_LIMIT_SOCK_EPOLL_CTL_TYPE_LO = EPOLL_CTL_ADD,
    SB_LABEL_LIMIT_SOCK_EPOLL_CTL_TYPE_HI = EPOLL_CTL_MOD
};
SB_Label_Map gv_sock_epoll_ctl_type_label_map = {
    SB_LABEL_LIMIT_SOCK_EPOLL_CTL_TYPE_LO,
    SB_LABEL_LIMIT_SOCK_EPOLL_CTL_TYPE_HI,
    "<unknown>", ga_sock_epoll_ctl_type_labels
};

static SB_Trans::Sock_Controller *gv_sock_ctlr = NULL;

static const char *sock_get_label_epoll_ctl(int pv_value) {
    return SB_get_label(&gv_sock_epoll_ctl_type_label_map, pv_value);
}

static void *sock_comp_thread_fun(void *pp_arg) {
    SB_Trans::Sock_Comp_Thread *lp_thread =
      static_cast<SB_Trans::Sock_Comp_Thread *>(pp_arg);
    lp_thread->run();
    return NULL;
}

SB_Trans::Sock_Controller *getGlobalSockCtrl() {
  if (gv_sock_ctlr != NULL)
     return gv_sock_ctlr;
  SB_util_short_lock();
  if (gv_sock_ctlr != NULL) {
     SB_util_short_unlock();
     return gv_sock_ctlr;
  }
  gv_sock_ctlr = new SB_Trans::Sock_Controller();
  SB_util_short_unlock();
  return gv_sock_ctlr;
}


SB_Trans::Sock_Comp_Thread::Sock_Comp_Thread(const char *pp_name)
: Thread(sock_comp_thread_fun, pp_name),
  iv_fin(false),
  iv_running(false) {
    const char *WHERE = "Sock_Comp_Thread::Sock_Comp_Thread";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "creating this=%p\n", pfp(this));
}

SB_Trans::Sock_Comp_Thread::~Sock_Comp_Thread() {
    const char *WHERE = "Sock_Comp_Thread::~Sock_Comp_Thread";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "destroying this=%p\n", pfp(this));
}

void SB_Trans::Sock_Comp_Thread::fin() {
    iv_fin = true;
}

void SB_Trans::Sock_Comp_Thread::run() {
    const char *WHERE = "Sock_Comp_Thread::run";

    iv_running = true;
    while (!iv_fin) {
        getGlobalSockCtrl()->epoll_wait(WHERE, -1);
    }
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "EXITING comp thread\n");
    iv_running = false;
}

bool SB_Trans::Sock_Comp_Thread::running() {
    return iv_running;
}

SB_Trans::Sock_Client::Sock_Client()
: Sock_User_Common("Sock_Client", -1) {
}

SB_Trans::Sock_Client::Sock_Client(int pv_sock)
: Sock_User_Common("Sock_Client", pv_sock),
  iv_sdp(false) {

    if (gv_ms_trace_alloc)
        trace_printf("%s::%s creating this=%p\n",
                     ip_where, ip_where, pfp(this));
}

SB_Trans::Sock_Client::~Sock_Client() {
}

int SB_Trans::Sock_Client::connect(char *pp_host_port) {
    char *lp_host;
    char *lp_port;
    int   lv_err;
    int   lv_port;

    lp_host = pp_host_port;
    lp_port = strstr(lp_host, ":");
    SB_util_assert_pne(lp_port, NULL);
    *lp_port = 0;
    lp_port++;
    lv_port = atoi(lp_port);
    lv_err = connect(lp_host, lv_port);
    return lv_err;
}

//
// without this pragma, gcc has problems with the following
//   lv_addr.sin_port = htons(static_cast<uint16_t>(pv_port));
// And gcc doesn't like this pragma inside the function
//
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif
int SB_Trans::Sock_Client::connect(char *pp_host, int pv_port) {
    const char         *WHERE = "Sock_Client::connect";
    enum              { SIZE = 1024 * 128 };
    char                la_errno[100];
    char                la_hostbuf[500];
    unsigned char      *lp_addrp;
    char               *lp_hostbuf;
    struct hostent     *lp_hostent;
    struct sockaddr_in  lv_addr;
    int                 lv_domain;
    int                 lv_err;
    int                 lv_errno;
    struct hostent      lv_hostent;
    int                 lv_hostenterrno;
    int                 lv_size;
    int                 lv_sock;

    if (iv_sdp)
        lv_domain = AF_INET_SDP;
    else
        lv_domain = AF_INET;
    lv_sock = ::socket(lv_domain, SOCK_STREAM, 0);
    lv_errno = errno;
    if (gv_ms_trace_sock) {
        if (lv_sock == -1)
            trace_where_printf(WHERE,
                               "creating socket, domain=%d, errno=%d(%s)\n",
                               lv_domain,
                               lv_errno,
                               strerror_r(lv_errno,
                                          la_errno,
                                          sizeof(la_errno)));
        else
            trace_where_printf(WHERE, "creating socket, domain=%d, sock=%d\n",
                               lv_domain, lv_sock);
    }
    if (lv_sock == -1)
        return lv_errno;
    lv_err = getGlobalSockCtrl()->set_keepalive(WHERE, lv_sock);
    SB_util_assert_ieq(lv_err, 0);
    lv_err = getGlobalSockCtrl()->set_nodelay(WHERE, lv_sock);
    SB_util_assert_ieq(lv_err, 0);
    lv_err = getGlobalSockCtrl()->set_size_recv(WHERE, lv_sock, SIZE);
    SB_util_assert_ieq(lv_err, 0);
    lv_err = getGlobalSockCtrl()->set_size_send(WHERE, lv_sock, SIZE);
    SB_util_assert_ieq(lv_err, 0);
    memset(&lv_addr, 0, sizeof(lv_addr));
    lv_addr.sin_family = AF_INET;
    lv_addr.sin_port = htons(static_cast<uint16_t>(pv_port));
    lp_hostbuf = NULL;
    lv_err = gethostbyname_r(pp_host,
                             &lv_hostent,
                             la_hostbuf,
                             sizeof(la_hostbuf),
                             &lp_hostent,
                             &lv_hostenterrno);
    if (lv_err && (errno == ERANGE)) {
        for (lv_size = 1000; lv_size < 5000; lv_size += 1000) {
            lp_hostbuf = new char[lv_size];
            lv_err = gethostbyname_r(pp_host,
                                     &lv_hostent,
                                     lp_hostbuf,
                                     lv_size,
                                     &lp_hostent,
                                     &lv_hostenterrno);
            if (!lv_err)
                break;
            delete [] lp_hostbuf;
        }
    }
    SB_util_assert_ieq(lv_err, 0);
    SB_util_assert_pne(lp_hostent, NULL);
    SB_util_assert(*reinterpret_cast<long *>(lp_hostent->h_addr) != -1);
    lv_addr.sin_addr.s_addr = static_cast<uint32_t>(*reinterpret_cast<uint32_t *>(lp_hostent->h_addr));
    if (gv_ms_trace_sock) {
        lp_addrp = reinterpret_cast<unsigned char *>(&lv_addr.sin_addr.s_addr);
        trace_where_printf(WHERE, "connect attempt to host=%s (%d.%d.%d.%d:%d) (0x%08X:%X), sock=%d\n",
                           pp_host,
                           lp_addrp[0], lp_addrp[1], lp_addrp[2], lp_addrp[3],
                           pv_port, lv_addr.sin_addr.s_addr,
                           pv_port, lv_sock);
    }
    lv_err = ::connect(lv_sock,
                       reinterpret_cast<struct sockaddr *>(&lv_addr),
                       sizeof(struct sockaddr));
    lv_errno = errno;
    if (gv_ms_trace_sock) {
        if (lv_err == -1)
            trace_where_printf(WHERE, "connect failure, errno=%d(%s)\n",
                               lv_errno,
                               strerror_r(lv_errno,
                                          la_errno,
                                          sizeof(la_errno)));
        else
            trace_where_printf(WHERE, "connect completed\n");
    }
    if (lp_hostbuf != NULL)
        delete [] lp_hostbuf;
    if (lv_err == -1)
        return lv_errno;
    iv_sock = lv_sock;
    return 0;
}
#ifdef __GNUC__
#pragma GCC diagnostic error "-Wconversion"
#endif

void SB_Trans::Sock_Client::set_sdp(bool pv_sdp) {
    iv_sdp = pv_sdp;
}

SB_Trans::Sock_Controller::Sock_Controller()
: ip_comp_thread(NULL) {
    int lv_err;

    lv_err = epoll_create(100);
    SB_util_assert_ine(lv_err, -1);
    iv_efd = lv_err;
    const char *WHERE = "Sock_Controller::Sock_Controller";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "creating this=%p\n", pfp(this));
}

SB_Trans::Sock_Controller::~Sock_Controller() {
    const char *WHERE = "Sock_Controller::~Sock_Controller";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "destroying this=%p\n", pfp(this));
}

void SB_Trans::Sock_Controller::epoll_ctl(const char *pp_where,
                                          int         pv_op,
                                          int         pv_fd,
                                          int         pv_event,
                                          void       *pp_data) {

    static bool         sv_ignore_enoent = true;
    static bool         sv_envvar_ignore_enoent_read = false;

    char                la_errno[100];
    const char         *lp_op;
    int                 lv_err;
    int                 lv_errno;
    struct epoll_event  lv_event;

    if (! sv_envvar_ignore_enoent_read) {
      sv_envvar_ignore_enoent_read = true;
      ms_getenv_bool("SQ_SB_IGNORE_ENOENT", &sv_ignore_enoent);
    }

    lv_event.events = pv_event;
    lv_event.data.ptr = pp_data;
    lv_err = ::epoll_ctl(iv_efd, pv_op, pv_fd, &lv_event);
    lv_errno = errno;
    if (gv_ms_trace_sock) {
        lp_op = sock_get_label_epoll_ctl(pv_op);
        if (lv_err == -1)
            trace_where_printf(pp_where,
                               "epoll-ctl op=%d(%s), fd=%d, event=%d, data=%p, errno=%d(%s)\n",
                               pv_op,
                               lp_op,
                               pv_fd,
                               pv_event,
                               pp_data,
                               lv_errno,
                               strerror_r(lv_errno,
                                          la_errno,
                                          sizeof(la_errno)));
        else
            trace_where_printf(pp_where,
                               "epoll-ctl op=%d(%s), fd=%d, event=%d, data=%p, err=%d\n",
                               pv_op, lp_op, pv_fd, pv_event, pp_data, lv_err);
    }

    if ((sv_ignore_enoent) &&
        (lv_err == -1) &&
        (lv_errno == ENOENT) &&
        ((pv_op == EPOLL_CTL_MOD) ||
         (pv_op == EPOLL_CTL_DEL))) {
        lp_op = sock_get_label_epoll_ctl(pv_op);
        SB_Buf_Line la_buf;
        sprintf(la_buf, 
            "epoll_ctl ignoring ENOENT op=%d(%s), fd=%d, event=%d, data=%p, err=%d\n",
            pv_op, lp_op, pv_fd, pv_event, pp_data, lv_err);
        sb_util_write_log(la_buf);
        return;
    }

    SB_util_assert_ine(lv_err, -1);
}

void SB_Trans::Sock_Controller::epoll_wait(const char *pp_where,
                                           int         pv_timeout) {
    enum { MAX_EVENTS = 10 };
    char               la_errno[100];
    struct epoll_event la_event[MAX_EVENTS];
    Sock_EH           *lp_eh;
    int                lv_err;
    int                lv_errno;
    int                lv_events;
    int                lv_inx;

    if (gv_ms_trace_sock)
        trace_where_printf(pp_where,
                           "epoll-wait ENTER timeout=%d\n",
                           pv_timeout);
    lv_err = ::epoll_wait(iv_efd, la_event, MAX_EVENTS, pv_timeout);
    lv_errno = errno;
    if (gv_ms_trace_sock) {
        if (lv_err == -1)
            trace_where_printf(pp_where,
                               "epoll-wait EXIT errno=%d(%s)\n",
                               lv_errno,
                               strerror_r(lv_errno,
                                          la_errno,
                                          sizeof(la_errno)));
        else
            trace_where_printf(pp_where,
                               "epoll-wait EXIT err=%d\n", lv_err);
    }
    if (lv_err == -1) {
        if (lv_errno != EINTR)
            SB_util_assert_ine(lv_err, -1);
    }
    for (lv_inx = 0; lv_inx < lv_err; lv_inx++) {
        lv_events = static_cast<int>(la_event[lv_inx].events);
        lp_eh = static_cast<Sock_EH *>(la_event[lv_inx].data.ptr);
        if (gv_ms_trace_sock)
            trace_where_printf(pp_where,
                               "epoll-wait event[%d].events=0x%x, data=%p\n",
                               lv_inx,
                               lv_events,
                               pfp(lp_eh));
        lp_eh->process_events(lv_events);
    }
}

void SB_Trans::Sock_Controller::lock() {
    int lv_status;

    lv_status = iv_ctlr_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

int SB_Trans::Sock_Controller::set_nodelay(const char *pp_where,
                                           int         pv_sock) {
    int lv_err;
    int lv_tcpopt;

    lv_tcpopt = 1;
    lv_err = setsockopt(pv_sock,
                        IPPROTO_TCP,
                        TCP_NODELAY,
                        reinterpret_cast<char *>(&lv_tcpopt),
                        sizeof(lv_tcpopt));
    if (gv_ms_trace_sock)
        trace_where_printf(pp_where, "setsockopt NODELAY sock=%d, err=%d\n",
                           pv_sock, lv_err);
    return lv_err;
}

int SB_Trans::Sock_Controller::set_keepalive(const char *pp_where,
                                             int         pv_sock) {
    int lv_err;
    
    static bool sv_envvar_read = false;
    static int sv_sockkeepalive = 1;
    static int sv_tcpkeepidle = 240;
    static int sv_tcpkeepintvl = 6;
    static int sv_tcpkeepcnt = 10;

    if (! sv_envvar_read) {
      sv_envvar_read = true;
      ms_getenv_int("SQ_SB_KEEPALIVE", &sv_sockkeepalive);
      ms_getenv_int("SQ_SB_KEEPIDLE", &sv_tcpkeepidle);
      ms_getenv_int("SQ_SB_KEEPINTVL", &sv_tcpkeepintvl);
      ms_getenv_int("SQ_SB_KEEPCNT", &sv_tcpkeepcnt);
    }

    lv_err = setsockopt(pv_sock,
                        SOL_SOCKET,
                        SO_KEEPALIVE,
                        reinterpret_cast<char *>(&sv_sockkeepalive),
                        sizeof(sv_sockkeepalive));

    lv_err = setsockopt(pv_sock,
                        IPPROTO_TCP,
                        TCP_KEEPIDLE,
                        reinterpret_cast<char *>(&sv_tcpkeepidle),
                        sizeof(sv_tcpkeepidle));

    lv_err = setsockopt(pv_sock,
                        IPPROTO_TCP,
                        TCP_KEEPINTVL,
                        reinterpret_cast<char *>(&sv_tcpkeepintvl),
                        sizeof(sv_tcpkeepintvl));

    lv_err = setsockopt(pv_sock,
                        IPPROTO_TCP,
                        TCP_KEEPCNT,
                        reinterpret_cast<char *>(&sv_tcpkeepcnt),
                        sizeof(sv_tcpkeepcnt));

    if (gv_ms_trace_sock)
        trace_where_printf(pp_where, "setsockopt KEEPALIVE sock=%d, err=%d\n",
                           pv_sock, lv_err);
    return lv_err;
}

int SB_Trans::Sock_Controller::set_nonblock(const char *pp_where,
                                            int         pv_sock) {
    int          lv_err;
    int          lv_value;

    lv_value = 0;
    lv_err = fcntl(pv_sock, F_GETFL, &lv_value);
    SB_util_assert_ine(lv_err, -1);
    lv_value |= O_NONBLOCK;
    lv_err = fcntl(pv_sock, F_SETFL, lv_value);
    SB_util_assert_ine(lv_err, -1);
    if (gv_ms_trace_sock)
        trace_where_printf(pp_where, "fcntl+O_NONBLOCK sock=%d, err=%d\n",
                           pv_sock, lv_err);
    return lv_err;
}

int SB_Trans::Sock_Controller::set_reuseaddr(const char *pp_where,
                                             int         pv_sock) {
    int lv_err;
    int lv_value;

    lv_value = 1;
    lv_err = setsockopt(pv_sock,
                        SOL_SOCKET,
                        SO_REUSEADDR,
                        reinterpret_cast<char *>(&lv_value),
                        sizeof(lv_value));
    if (gv_ms_trace_sock)
        trace_where_printf(pp_where, "setsockopt REUSEADDR sock=%d, err=%d\n",
                           pv_sock, lv_err);
    return lv_err;
}

int SB_Trans::Sock_Controller::set_size_recv(const char *pp_where,
                                             int         pv_sock,
                                             int         pv_size) {
    int lv_err;
    int lv_value;

    lv_value = pv_size;
    lv_err = setsockopt(pv_sock,
                        SOL_SOCKET,
                        SO_RCVBUF,
                        reinterpret_cast<char *>(&lv_value),
                        sizeof(lv_value));
    if (gv_ms_trace_sock)
        trace_where_printf(pp_where, "setsockopt SO_RCVBUF sock=%d, size=%d, err=%d\n",
                           pv_sock, pv_size, lv_err);
    return lv_err;
}

int SB_Trans::Sock_Controller::set_size_send(const char *pp_where,
                                             int         pv_sock,
                                             int         pv_size) {
    int lv_err;
    int lv_value;

    lv_value = pv_size;
    lv_err = setsockopt(pv_sock,
                        SOL_SOCKET,
                        SO_SNDBUF,
                        reinterpret_cast<char *>(&lv_value),
                        sizeof(lv_value));
    if (gv_ms_trace_sock)
        trace_where_printf(pp_where, "setsockopt SO_SNDBUF sock=%d, size=%d, err=%d\n",
                           pv_sock, pv_size, lv_err);
    return lv_err;
}

void SB_Trans::Sock_Controller::shutdown(const char *pp_where) {
    getGlobalSockCtrl()->shutdown_this(pp_where);
}

void SB_Trans::Sock_Controller::shutdown_this(const char *pp_where) {
    char  la_errno[100];
    void *lp_result;
    int   lv_err;
    int   lv_errno;
    int   lv_status;

    if (ip_comp_thread != NULL) {
        if (gv_ms_trace_sock)
            trace_where_printf(pp_where, "shutdown comp thread\n");
        ip_comp_thread->fin();
        ip_shutdown_eh->shutdown();
        lv_status = ip_comp_thread->join(&lp_result);
        if (gv_ms_trace_sock)
            trace_where_printf(pp_where, "comp thread death, status=%d\n",
                               lv_status);
        SB_util_assert_ieq(lv_status, 0);
        ip_comp_thread = NULL;
    }
    if (iv_efd > 0) {
        lv_err = ::close(iv_efd);
        lv_errno = errno;
        if (gv_ms_trace_sock) {
            if (lv_err == -1)
                trace_where_printf(pp_where, "closed efd=%d, errno=%d(%s)\n",
                                   iv_efd,
                                   lv_errno,
                                   strerror_r(lv_errno,
                                              la_errno,
                                              sizeof(la_errno)));
            else
                trace_where_printf(pp_where, "closed efd=%d\n", iv_efd);
        }
        iv_efd = -1;
    }
}

void SB_Trans::Sock_Controller::sock_add(const char *pp_where,
                                         int         pv_sock,
                                         Sock_EH    *pp_eh) {
    const char *WHERE = "Sock_Controller::sock_add";
    char        la_name[10];

    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "%s-add fd=%d, eh=%p\n",
                           pp_where, pv_sock, pfp(pp_eh));
    getGlobalSockCtrl()->epoll_ctl(pp_where,
                           EPOLL_CTL_ADD,
                           pv_sock,
                           EPOLLIN,
                           pp_eh);
    // need lock - can only have one comp thread
    getGlobalSockCtrl()->lock();
    if (ip_comp_thread == NULL) {
        ip_shutdown_eh = new Sock_Shutdown_EH();
        getGlobalSockCtrl()->epoll_ctl(pp_where,
                               EPOLL_CTL_ADD,
                               ip_shutdown_eh->get_read_fd(),
                               EPOLLIN,
                               ip_shutdown_eh);
        strcpy(la_name, "comp");
        ip_comp_thread = new Sock_Comp_Thread(la_name);
        if (gv_ms_trace_sock)
            trace_where_printf(WHERE, "starting sock comp thread %s\n", la_name);
        ip_comp_thread->start();
    }
    getGlobalSockCtrl()->unlock();
}

void SB_Trans::Sock_Controller::sock_del(const char *pp_where,
                                         int         pv_sock) {
    const char *WHERE = "Sock_Controller::sock_del";
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "%s-delete fd=%d\n", pp_where, pv_sock);
    getGlobalSockCtrl()->epoll_ctl(pp_where, EPOLL_CTL_DEL, pv_sock, 0, NULL);
}

void SB_Trans::Sock_Controller::sock_mod(const char *pp_where,
                                         int         pv_sock,
                                         int         pv_events,
                                         Sock_EH    *pp_eh) {
    getGlobalSockCtrl()->epoll_ctl(pp_where, EPOLL_CTL_MOD, pv_sock, pv_events, pp_eh);
}

void SB_Trans::Sock_Controller::unlock() {
    int lv_status;

    lv_status = iv_ctlr_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

SB_Trans::Sock_EH::Sock_EH() {
    const char *WHERE = "Sock_EH::Sock_EH";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "creating this=%p\n", pfp(this));
}

SB_Trans::Sock_EH::~Sock_EH() {
    const char *WHERE = "Sock_EH::~Sock_EH";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "destroying this=%p\n", pfp(this));
}

SB_Trans::Sock_Listener::Sock_Listener()
: iv_sdp(false),
  iv_sock(-1) {
    const char  *WHERE = "Sock_Listener::Sock_Listener";

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "creating this=%p\n", pfp(this));
}

SB_Trans::Sock_Listener::~Sock_Listener() {
    const char  *WHERE = "Sock_Listener::~Sock_Listener";
    char         la_errno[100];
    int          lv_err;
    int          lv_errno;

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "destroying this=%p, sock=%d\n",
                           pfp(this), iv_sock);
    if (iv_sock >= 0) {
        lv_err = ::close(iv_sock);
        lv_errno = errno;
        if (gv_ms_trace_sock) {
            if (lv_err == -1)
                trace_where_printf(WHERE, "closed sock=%d, errno=%d(%s)\n",
                                   iv_sock,
                                   lv_errno,
                                   strerror_r(lv_errno,
                                              la_errno,
                                              sizeof(la_errno)));
            else
                trace_where_printf(WHERE, "closed sock=%d\n", iv_sock);
        }
        iv_sock = -1;
    }
}

SB_Trans::Sock_Server *SB_Trans::Sock_Listener::accept() {
    const char       *WHERE = "Sock_Listener::accept";
    enum            { SIZE = 1024 * 128 };
    char              la_errno[100];
    struct sockaddr   lv_c_addr;
    int               lv_err;
    int               lv_errno;
    int               lv_sock;

    socklen_t lv_c_addr_len = sizeof(lv_c_addr);
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "accept attempt to sock=%d\n", iv_sock);
    lv_sock = ::accept(iv_sock, &lv_c_addr, &lv_c_addr_len);
    lv_errno = errno;
    if (gv_ms_trace_sock) {
        if (lv_sock == -1)
            trace_where_printf(WHERE, "accept completed errno=%d(%s)\n",
                               lv_errno,
                               strerror_r(lv_errno,
                                          la_errno,
                                          sizeof(la_errno)));
        else
            trace_where_printf(WHERE, "accept completed sock=%d\n", lv_sock);
    }
    if (lv_sock == -1) {
        if (lv_errno == EBADF) {
            // socket closed
            if (gv_ms_trace_sock)
                trace_where_printf(WHERE, "accept completed errno=%d(%s)\n",
                                   lv_errno,
                                   strerror_r(lv_errno,
                                              la_errno,
                                              sizeof(la_errno)));
            return NULL; //
        }
        SB_util_assert_ine(lv_sock, -1);
    }
    lv_err = getGlobalSockCtrl()->set_reuseaddr(WHERE, lv_sock);
    SB_util_assert_ieq(lv_err, 0);
    lv_err = getGlobalSockCtrl()->set_nodelay(WHERE, lv_sock);
    SB_util_assert_ieq(lv_err, 0);
    lv_err = getGlobalSockCtrl()->set_size_recv(WHERE, lv_sock, SIZE);
    SB_util_assert_ieq(lv_err, 0);
    lv_err = getGlobalSockCtrl()->set_size_send(WHERE, lv_sock, SIZE);
    SB_util_assert_ieq(lv_err, 0);
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "accept completed on sock=%d, new sock=%d\n",
                           iv_sock, lv_sock);
    Sock_Server *listener = new Sock_Server(lv_sock);
    return listener;
}

void SB_Trans::Sock_Listener::destroy() {
    delete this;
}

void SB_Trans::Sock_Listener::get_addr(char *pp_host, int *pp_port) {
    strcpy(pp_host, ia_host);
    *pp_port = iv_port;
}

//
// without this pragma, gcc has problems with the following
//   lv_addr.sin_port = htons(static_cast<uint16_t>(*pp_port));
//   iv_port = ntohs(lv_addr.sin_port);
// And gcc doesn't like this pragma inside the function
//
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#endif
void SB_Trans::Sock_Listener::listen(char *pp_host, int *pp_port) {
    const char         *WHERE = "Sock_Listener::listen";
    char                la_errno[100];
    char                la_hostbuf[500];
    long               *lp_addr;
    unsigned char      *lp_addrp;
    char               *lp_hostbuf;
    struct hostent     *lp_hostent;
    struct sockaddr_in  lv_addr;
    int                 lv_domain;
    bool                lv_done;
    int                 lv_err;
    int                 lv_errno;
    struct hostent      lv_hostent;
    int                 lv_hostenterrno;
    int                 lv_inx;
    socklen_t           lv_len;
    int                 lv_size;
    int                 lv_sock;

    if (iv_sdp)
        lv_domain = AF_INET_SDP;
    else
        lv_domain = AF_INET;
    lv_sock = ::socket(lv_domain, SOCK_STREAM, 0);
    lv_errno = errno;
    if (gv_ms_trace_sock) {
        if (lv_sock == -1)
            trace_where_printf(WHERE, "creating socket, domain=%d, errno=%d(%s)\n",
                               lv_domain,
                               lv_errno,
                               strerror_r(lv_errno,
                                          la_errno,
                                          sizeof(la_errno)));
        else
            trace_where_printf(WHERE, "creating socket, domain=%d, sock=%d\n",
                               lv_domain, lv_sock);
    }
    SB_util_assert_ine(lv_sock, -1);
    lv_err = getGlobalSockCtrl()->set_reuseaddr(WHERE, lv_sock);
    SB_util_assert_ieq(lv_err, 0);
    memset(&lv_addr, 0, sizeof(lv_addr));
    lv_addr.sin_family = static_cast<uint16_t>(lv_domain);
    lv_addr.sin_port = htons(static_cast<uint16_t>(*pp_port));
    lp_hostbuf = NULL;
    lv_err = gethostbyname_r(pp_host,
                             &lv_hostent,
                             la_hostbuf,
                             sizeof(la_hostbuf),
                             &lp_hostent,
                             &lv_hostenterrno);
    if (lv_err && (errno == ERANGE)) {
        for (lv_size = 1000; lv_size < 5000; lv_size += 1000) {
            lp_hostbuf = new char[lv_size];
            lv_err = gethostbyname_r(pp_host,
                                     &lv_hostent,
                                     lp_hostbuf,
                                     lv_size,
                                     &lp_hostent,
                                     &lv_hostenterrno);
            if (!lv_err)
                break;
            delete [] lp_hostbuf;
        }
    }
    SB_util_assert_ieq(lv_err, 0);
    SB_util_assert_pne(lp_hostent, NULL);
    lv_done = false;
    for (lv_inx = 0;; lv_inx++) {
        lp_addr = reinterpret_cast<long *>(lp_hostent->h_addr_list[lv_inx]);
        if (lp_addr == NULL)
            break;
        if (*lp_addr == 0)
            break;
        lv_addr.sin_addr.s_addr = static_cast<int>(*lp_addr);
        if (gv_ms_trace_sock) {
            lp_addrp = reinterpret_cast<unsigned char *>(lp_addr);
            trace_where_printf(WHERE, "trying to bind to host=%s (%d.%d.%d.%d:%d) (0x%08X:%X), sock=%d\n",
                               pp_host, lp_addrp[0], lp_addrp[1],
                               lp_addrp[2], lp_addrp[3], *pp_port,
                               lv_addr.sin_addr.s_addr, *pp_port, lv_sock);
        }
        lv_err = bind(lv_sock, reinterpret_cast<struct sockaddr *>(&lv_addr),
                      sizeof(struct sockaddr));
        if (lv_err == 0) {
            lv_done = true;
            break;
        } else {
            if (gv_ms_trace_sock) {
                trace_where_printf(WHERE, "bind failed, err=%d, sock=%d, host=%s\n",
                                   lv_errno, lv_sock, pp_host);
            }
        }
    }
    SB_util_assert_it(lv_done);
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "bind complete, sock=%d\n",
                           lv_sock);

    static bool sv_envvar_read = false;
    static int sv_listen_backlog = 1024;
    if (! sv_envvar_read) {
      sv_envvar_read = true;
      ms_getenv_int("SQ_SB_LISTEN_BACKLOG", &sv_listen_backlog);
    }
    lv_err = ::listen(lv_sock, sv_listen_backlog);
    SB_util_assert_ine(lv_err, -1);

    iv_sock = lv_sock;
    lv_len = sizeof(lv_addr);
    lv_err = getsockname(lv_sock, reinterpret_cast<struct sockaddr *>(&lv_addr), &lv_len);
    SB_util_assert_ieq(lv_err, 0);
    strcpy(ia_host, pp_host);
    lp_addrp = reinterpret_cast<unsigned char *>(&lv_addr.sin_addr.s_addr);
    sprintf(ia_host, "%d.%d.%d.%d",
            lp_addrp[0], lp_addrp[1], lp_addrp[2], lp_addrp[3]);
    iv_port = ntohs(lv_addr.sin_port);
    *pp_port = iv_port;
    if (lp_hostbuf != NULL)
        delete [] lp_hostbuf;
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "listening to host=%s:%d (0x%08X:%X), sock=%d\n",
                           ia_host, iv_port, lv_addr.sin_addr.s_addr,
                           iv_port, lv_sock);
}
#ifdef __GNUC__
#pragma GCC diagnostic error "-Wconversion"
#endif

void SB_Trans::Sock_Listener::set_sdp(bool pv_sdp) {
    iv_sdp = pv_sdp;
}

SB_Trans::Sock_Server::Sock_Server(int pv_sock)
: Sock_User_Common("Sock_Server", -1) {

    iv_sock = pv_sock;
    iv_sock_added = false;

    if (gv_ms_trace_alloc)
        trace_printf("%s::%s creating this=%p\n",
                     ip_where, ip_where, pfp(this));
}

SB_Trans::Sock_Server::~Sock_Server() {
}

SB_Trans::Sock_Shutdown_EH::Sock_Shutdown_EH() {
    const char *WHERE = "Sock_Shutdown_EH::Sock_Shutdown_EH";
    int         lv_err;

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "creating this=%p\n", pfp(this));
    lv_err = pipe(ia_fds);
    SB_util_assert_ieq(lv_err, 0);
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "read fd=%d, write fd=%d\n",
                           ia_fds[0], ia_fds[1]);
}

SB_Trans::Sock_Shutdown_EH::~Sock_Shutdown_EH() {
    const char *WHERE = "Sock_Shutdown_EH::~Sock_Shutdown_EH";
    char        la_errno[100];
    int         lv_err;
    int         lv_errno;
    int         lv_fd;

    if (gv_ms_trace_alloc)
        trace_where_printf(WHERE, "destroying this=%p\n", pfp(this));
    lv_fd = get_read_fd();
    lv_err = ::close(lv_fd);
    lv_errno = errno;
    if (gv_ms_trace_sock) {
        if (lv_err == -1)
            trace_where_printf(WHERE, "closed read fd=%d, errno=%d(%s)\n",
                               lv_fd,
                               lv_errno,
                               strerror_r(lv_errno,
                                          la_errno,
                                          sizeof(la_errno)));
        else
            trace_where_printf(WHERE, "closed read fd=%d\n", lv_fd);
    }
    lv_fd = get_write_fd();
    lv_err = ::close(lv_fd);
    lv_errno = errno;
    if (gv_ms_trace_sock) {
        if (lv_err == -1)
            trace_where_printf(WHERE, "closed write fd=%d, errno=%d(%s)\n",
                               lv_fd,
                               lv_errno,
                               strerror_r(lv_errno,
                                          la_errno,
                                          sizeof(la_errno)));
        else
            trace_where_printf(WHERE, "closed write fd=%d\n", lv_fd);
    }
}

int SB_Trans::Sock_Shutdown_EH::get_read_fd() {
    return ia_fds[0];
}

int SB_Trans::Sock_Shutdown_EH::get_write_fd() {
    return ia_fds[1];
}

void SB_Trans::Sock_Shutdown_EH::process_events(int pv_events) {
    const char *WHERE = "Sock_Shutdown_EH::process_events";

    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "events=0x%x\n", pv_events);
}

void SB_Trans::Sock_Shutdown_EH::shutdown() {
    const char *WHERE = "Sock_Shutdown_EH::shutdown";
    char        la_buf[1];
    int         lv_fd;
    ssize_t     lv_wcnt;

    lv_fd = get_write_fd();
    la_buf[0] = '\0';
    lv_wcnt = ::write(lv_fd, la_buf, sizeof(la_buf));
    if (gv_ms_trace_sock)
        trace_where_printf(WHERE, "write fd=%d, wcnt=%d\n",
                           lv_fd, static_cast<int>(lv_wcnt));
    SB_util_assert_lne(lv_wcnt, -1);
}

SB_Trans::Sock_User::Sock_User() {
}

SB_Trans::Sock_User::~Sock_User() {
}

SB_Trans::Sock_User_Common::Sock_User_Common(const char *pp_where,
                                             int         pv_sock)
: ip_where(pp_where),
  iv_events(0),
  iv_sock(pv_sock),
  iv_sock_added(false) {
    SB_Buf_Line la_where;

    sprintf(la_where, "%s::%s", ip_where, ip_where);
    if (gv_ms_trace_alloc)
        trace_where_printf(la_where, "creating this=%p, sock=%d\n",
                          pfp(this), iv_sock);
}

SB_Trans::Sock_User_Common::~Sock_User_Common() {
    char        la_errno[100];
    SB_Buf_Line la_where;
    int         lv_err;
    int         lv_errno;

    sprintf(la_where, "%s::~%s", ip_where, ip_where);
    if (gv_ms_trace_alloc)
        trace_where_printf(la_where, "destroying this=%p, sock=%d\n",
                          pfp(this), iv_sock);
    if (iv_sock >= 0) {
        if (iv_sock_added)
            getGlobalSockCtrl()->sock_del(la_where, iv_sock);
        lv_err = ::close(iv_sock);
        lv_errno = errno;
        if (gv_ms_trace_sock) {
            if (lv_err == -1)
                trace_where_printf(la_where, "closed this=%p, sock=%d, errno=%d(%s)\n",
                                   pfp(this),
                                   iv_sock,
                                   lv_errno,
                                   strerror_r(lv_errno,
                                              la_errno,
                                              sizeof(la_errno)));
            else
                trace_where_printf(la_where, "closed this=%p, sock=%d\n",
                                    pfp(this), iv_sock);
        }
//      iv_sock = -1;
    }
}

void SB_Trans::Sock_User_Common::destroy() {
    delete this;
}

int SB_Trans::Sock_User_Common::get_sock() {
    return iv_sock;
}

void SB_Trans::Sock_User_Common::event_change(int      pv_events,
                                              Sock_EH *pp_eh) {
    const char  *WHERE = "::init_change";
    SB_Buf_Line  la_where;

    if (pv_events != iv_events) {
        sprintf(la_where, "%s%s", ip_where, WHERE);
        getGlobalSockCtrl()->sock_mod(la_where, iv_sock, pv_events, pp_eh);
        iv_events = pv_events;
    }
}

void SB_Trans::Sock_User_Common::event_init(Sock_EH *pp_eh) {
    const char  *WHERE = "::event_init";
    SB_Buf_Line  la_where;

    sprintf(la_where, "%s%s", ip_where, WHERE);
    getGlobalSockCtrl()->sock_add(la_where, iv_sock, pp_eh);
    iv_sock_added = true;
}

ssize_t SB_Trans::Sock_User_Common::read(void   *pp_buf,
                                         size_t  pv_count,
                                         int    *pp_errno) {
    const char *WHERE = "::read";
    char        la_errno[100];
    int         lv_errno;
    ssize_t     lv_rcnt;

    if (gv_ms_trace_sock)
        trace_printf("%s%s read started on sock=%d, count=%d\n",
                     ip_where, WHERE, iv_sock, static_cast<int>(pv_count));
if ((int) pv_count < 0) abort();
    lv_rcnt = ::read(iv_sock, pp_buf, pv_count);
    lv_errno = errno;
    if (pp_errno != NULL)
        *pp_errno = lv_errno;
    if (gv_ms_trace_sock) {
        if (lv_rcnt == -1)
            trace_printf("%s%s read error on sock=%d, errno=%d(%s)\n",
                         ip_where,
                         WHERE,
                         iv_sock,
                         lv_errno,
                         strerror_r(lv_errno, la_errno, sizeof(la_errno)));
        else
            trace_printf("%s%s read completed on sock=%d, rcnt=%d\n",
                         ip_where, WHERE, iv_sock, static_cast<int>(lv_rcnt));
    }
    return lv_rcnt;
}

void SB_Trans::Sock_User_Common::set_nonblock() {
    const char  *WHERE = "::set_nonblock";
    SB_Buf_Line  la_where;
    int          lv_err;

    sprintf(la_where, "%s%s", ip_where, WHERE);
    lv_err = getGlobalSockCtrl()->set_nonblock(la_where, iv_sock);
    SB_util_assert_ieq(lv_err, 0);
}

void SB_Trans::Sock_User_Common::stop() {
    const char  *WHERE = "::stop";
    SB_Buf_Line  la_where;
    int          lv_status;

    sprintf(la_where, "%s%s", ip_where, WHERE);

    lv_status = iv_sock_mutex.lock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
    if (iv_sock_added) {
        getGlobalSockCtrl()->sock_del(la_where, iv_sock);
        iv_sock_added = false;
    }
    lv_status = iv_sock_mutex.unlock();
    SB_util_assert_ieq(lv_status, 0); // sw fault
}

ssize_t SB_Trans::Sock_User_Common::write(const void *pp_buf,
                                          size_t      pv_count,
                                          int        *pp_errno) {
    const char *WHERE = "::write";
    char        la_errno[100];
    int         lv_errno;
    ssize_t     lv_wcnt;

    if (gv_ms_trace_sock)
        trace_printf("%s%s write started on sock=%d, buf=%p, count=%d\n",
                     ip_where, WHERE, iv_sock, pp_buf, static_cast<int>(pv_count));
    lv_wcnt = ::write(iv_sock, pp_buf, pv_count);
    lv_errno = errno;
    if (pp_errno != NULL)
        *pp_errno = lv_errno;
    if (gv_ms_trace_sock) {
        if (lv_wcnt == -1)
            trace_printf("%s%s write completed on sock=%d, errno=%d(%s)\n",
                         ip_where,
                         WHERE,
                         iv_sock,
                         lv_errno,
                         strerror_r(lv_errno, la_errno, sizeof(la_errno)));
        else
            trace_printf("%s%s write completed on sock=%d, wcnt=%d\n",
                         ip_where, WHERE, iv_sock, static_cast<int>(lv_wcnt));
    }
    return lv_wcnt;
}

#define SB_LABEL_CHK(name, low, high) \
SB_util_static_assert((sizeof(name)/sizeof(const char *)) == (high - low + 2));

//
// statically check that labels length matches map lengths
//
void sb_sock_label_map_init() {
    SB_LABEL_CHK(ga_sock_epoll_ctl_type_labels,
                 SB_LABEL_LIMIT_SOCK_EPOLL_CTL_TYPE_LO,
                 SB_LABEL_LIMIT_SOCK_EPOLL_CTL_TYPE_HI)
}
