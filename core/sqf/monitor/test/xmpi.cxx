///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "xmpi.h"

int           gv_xmpi_count_read = 0;
int           gv_xmpi_listen_sock = 0;
int           gv_xmpi_read_count = 0;
XMPI_Rd_Type  gv_xmpi_rd;
int           gv_xmpi_state = 0;
bool          gv_xmpi_trace = false;

static const char *ga_xmpi_recv_state[] =
{
    "RSTATE_INIT",
    "RSTATE_INIT2",
    "RSTATE_HDR",
    "RSTATE_DATA",
    "RSTATE_DONE",
    "RSTATE_LAST"
};



static void xmpi_chk_valid_comm(MPI_Comm pv_comm)
{
    //assert((pv_comm & XMPI_COMM_MASK) == XMPI_COMM_MASK);
    assert((pv_comm & ~XMPI_COMM_MASK) > 0);
}

static int xmpi_recv(const char   *pp_where,
                     const char   *pp_where_detail,
                     int           pv_sock,
                     void         *pp_buf,
                     int           /* pv_cnt */,
                     int          *pp_cnt)
{
    const char *FUNCTION = "xmpi_recv";

    *pp_cnt = 0;
    if (gv_xmpi_trace)
    {
        printf("%s: %s (%s) ENTER, sock=%d\n",
               pp_where,
               FUNCTION,
               pp_where_detail,
               pv_sock);
    }

    const char *lp_state;
    char       *lp_hdr;
    int         lv_ret;
    bool        lv_cont;

    do
    {
        lv_cont = false;
        if (gv_xmpi_trace)
        {
            if ((gv_xmpi_state >= XMPI_RSTATE_INIT) &&
                (gv_xmpi_state < XMPI_RSTATE_LAST))
                lp_state = ga_xmpi_recv_state[gv_xmpi_state];
            else
                lp_state = "<unknown>";
            printf("%s: %s (%s) sock=%d, state=%d(%s)\n",
                   pp_where, FUNCTION, pp_where_detail,
                   pv_sock, gv_xmpi_state, lp_state);
        }
        switch (gv_xmpi_state)
        {
        case XMPI_RSTATE_INIT:
            gv_xmpi_read_count = sizeof(XMPI_PMH_Type);
            gv_xmpi_count_read = 0;
            gv_xmpi_state = XMPI_RSTATE_INIT2;
            lv_cont = true;
            break;

        case XMPI_RSTATE_INIT2:
            if (gv_xmpi_trace)
                printf("%s: %s (%s) read hdr\n",
                       pp_where, FUNCTION,
                       pp_where_detail);
            lp_hdr = (char *) &gv_xmpi_rd.iv_hdr;
            lv_ret = (int) read(pv_sock,
                                &lp_hdr[gv_xmpi_count_read],
                                gv_xmpi_read_count - gv_xmpi_count_read);
            if (gv_xmpi_trace)
            {
                if (lv_ret == -1)
                    printf("%s: %s (%s) errno=%d\n",
                           pp_where, FUNCTION,
                           pp_where_detail,
                           errno);
                else
                    printf("%s: %s (%s) ret=%d\n",
                           pp_where, FUNCTION,
                           pp_where_detail,
                           lv_ret);
            }
            assert(lv_ret != -1);
            if (lv_ret == 0)
            {
                if (gv_xmpi_trace)
                    printf("%s: %s (%s) EOF received\n",
                           pp_where, FUNCTION,
                           pp_where_detail);
                return -1; // EOF
            }
            if (lv_ret > 0)
                gv_xmpi_count_read += lv_ret;
            else
            {
                if (gv_xmpi_trace)
                    printf("%s: %s (%s) cr=%d, rc=%d\n",
                           pp_where, FUNCTION,
                           pp_where_detail,
                           gv_xmpi_count_read,
                           gv_xmpi_read_count);
            }
            if (gv_xmpi_count_read >= gv_xmpi_read_count)
            {
                gv_xmpi_state = XMPI_RSTATE_HDR;
                lv_cont = true;
            }
            break;

        case XMPI_RSTATE_HDR:
            assert(gv_xmpi_rd.iv_hdr.ia_sig[0] == XMPI_PMH_SIG0); // TODO
            assert(gv_xmpi_rd.iv_hdr.ia_sig[1] == XMPI_PMH_SIG1); // TODO
            assert(gv_xmpi_rd.iv_hdr.ia_sig[2] == XMPI_PMH_SIG2); // TODO
            assert(gv_xmpi_rd.iv_hdr.ia_sig[3] == XMPI_PMH_SIG3); // TODO
            gv_xmpi_count_read = 0;
            gv_xmpi_read_count = 0;
            if (gv_xmpi_trace)
                printf("%s: %s (%s) hdr, dsize=%d\n",
                       pp_where, FUNCTION,
                       pp_where_detail,
                       gv_xmpi_rd.iv_hdr.iv_dsize);
            gv_xmpi_rd.iv_data_len = gv_xmpi_rd.iv_hdr.iv_dsize;
            if (gv_xmpi_rd.iv_hdr.iv_dsize > 0)
            {
                gv_xmpi_rd.ip_data = (char *) pp_buf;
                gv_xmpi_read_count += gv_xmpi_rd.iv_data_len;
            }
            gv_xmpi_state = XMPI_RSTATE_DATA;
            if (gv_xmpi_read_count == 0)
                gv_xmpi_state = XMPI_RSTATE_DONE;
            lv_cont = true;
            break;

        case XMPI_RSTATE_DATA:
            if (gv_xmpi_trace)
                printf("%s: %s (%s) read data, cnt=%d\n",
                       pp_where, FUNCTION,
                       pp_where_detail,
                       gv_xmpi_rd.iv_data_len - gv_xmpi_count_read);
            lv_ret = (int) read(pv_sock,
                                &gv_xmpi_rd.ip_data[gv_xmpi_count_read],
                                gv_xmpi_rd.iv_data_len - gv_xmpi_count_read);
            if (gv_xmpi_trace)
            {
                if (lv_ret == -1)
                    printf("%s: %s (%s) errno=%d\n",
                           pp_where, FUNCTION,
                           pp_where_detail,
                           errno);
                else
                    printf("%s: %s (%s) ret=%d\n",
                           pp_where, FUNCTION,
                           pp_where_detail,
                           lv_ret);
            }
            assert(lv_ret != -1);
            if (lv_ret == 0)
            {
                if (gv_xmpi_trace)
                    printf("%s: %s (%s) EOF received\n",
                           pp_where, FUNCTION,
                           pp_where_detail);
                return -1; // EOF
            }
            if (lv_ret > 0)
                gv_xmpi_count_read += lv_ret;
            if (gv_xmpi_count_read >= gv_xmpi_read_count)
                gv_xmpi_state = XMPI_RSTATE_DONE;
            lv_cont = true;
            break;

        case XMPI_RSTATE_DONE:
            if (gv_xmpi_trace)
            {
                printf("%s: %s (%s) hdr=%p, len=%d\n",
                       pp_where, FUNCTION,
                       pp_where_detail,
                       (void *) &gv_xmpi_rd.iv_hdr,
                       (int) sizeof(gv_xmpi_rd.iv_hdr));
                if (gv_xmpi_rd.iv_data_len > 0)
                {
                    printf("%s: %s (%s) data=%p, len=%d\n",
                           pp_where, FUNCTION,
                           pp_where_detail,
                           gv_xmpi_rd.ip_data,
                           gv_xmpi_rd.iv_data_len);
                }
            }
            *pp_cnt = gv_xmpi_rd.iv_hdr.iv_dsize;
            gv_xmpi_state = XMPI_RSTATE_INIT;
            break;

        default:
            assert(0); // WOOPS
        }
    } while (lv_cont);
    return 0; // TODO
}

static int xmpi_send(const char *pp_where,
                     const char *pp_where_detail,
                     int         pv_sock,
                     void       *pp_buf,
                     int         pv_cnt)
{
    const char *FUNCTION = "xmpi_send";

    if (gv_xmpi_trace)
    {
        printf("%s: %s (%s) ENTER , sock=%d, buf=%p, cnt=%d\n",
               pp_where,
               FUNCTION,
               pp_where_detail,
               pv_sock,
               pp_buf,
               pv_cnt);
    }
    struct iovec la_iovec[2];
    XMPI_PMH_Type lv_hdr;
    lv_hdr.ia_sig[0] = XMPI_PMH_SIG0;
    lv_hdr.ia_sig[1] = XMPI_PMH_SIG1;
    lv_hdr.ia_sig[2] = XMPI_PMH_SIG2;
    lv_hdr.ia_sig[3] = XMPI_PMH_SIG3;
    lv_hdr.iv_dsize = pv_cnt;
    la_iovec[0].iov_base = &lv_hdr;
    la_iovec[0].iov_len = sizeof(lv_hdr);
    la_iovec[1].iov_base = pp_buf;
    la_iovec[1].iov_len = pv_cnt;
    struct msghdr lv_msg;
    lv_msg.msg_namelen = 0;
    lv_msg.msg_controllen = 0;
    lv_msg.msg_flags = 0;
    lv_msg.msg_iov = la_iovec;
    lv_msg.msg_iovlen = 2;
    if (gv_xmpi_trace)
        printf("%s: %s (%s) sendmsg iovlen=%d, iov[0].len=%d, iov[1].len=%d\n",
               pp_where,
               FUNCTION,
               pp_where_detail,
               (int) lv_msg.msg_iovlen,
               (int) la_iovec[0].iov_len,
               (int) la_iovec[1].iov_len);
    int lv_err = (int) sendmsg(pv_sock, &lv_msg, 0);
    if (gv_xmpi_trace)
        printf("%s: %s (%s) sendmsg ret=%d\n",
               pp_where,
               FUNCTION,
               pp_where_detail,
               lv_err);
    if (gv_xmpi_trace && (lv_err == -1))
        printf("%s: %s (%s) errno=%d\n",
               pp_where,
               FUNCTION,
               pp_where_detail,
               errno);
    assert(lv_err != -1);

    //
    // If it wasn't all sent, then send the rest
    //
    assert(lv_err >= (int) sizeof(lv_hdr)); // the following algorithm depends on this
    int lv_cnt = (int) (la_iovec[0].iov_len +
                        la_iovec[1].iov_len -
                        lv_err);
    int lv_off = lv_err - (int) sizeof(lv_hdr);
    char *lp_buf = (char *) pp_buf;
    while (lv_cnt > 0)
    {
        lv_err = (int) write(pv_sock, &lp_buf[lv_off], lv_cnt);
        assert(lv_err != -1);
        lv_cnt -= lv_err;
        lv_off += lv_err;
    }
    lv_err = pv_cnt;
    return lv_err;
}

static int xmpi_set_nodelay(const char *pp_where, int pv_sock)
{
    const char *FUNCTION = "xmpi_set_nodelay";

    int lv_tcpopt = 1;
    int lv_err = setsockopt(pv_sock,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char *) &lv_tcpopt,
                            sizeof(lv_tcpopt));
    if (gv_xmpi_trace)
        printf("%s: %s sock=%d, err=%d\n",
               pp_where,
               FUNCTION,
               pv_sock,
               lv_err);
    int lv_gtcpopt = 1;
    socklen_t lv_gtcpoptlen = sizeof(lv_gtcpopt);
    int lv_gerr = getsockopt(pv_sock,
                             IPPROTO_TCP,
                             TCP_NODELAY,
                             (char *) &lv_gtcpopt,
                             &lv_gtcpoptlen);
    assert(lv_gerr == 0);
    assert(lv_gtcpopt != 0);
    assert(lv_gtcpoptlen == sizeof(lv_gtcpopt));
    return lv_err;
}

static int xmpi_set_reuse(const char *pp_where, int pv_sock)
{
    const char *FUNCTION = "xmpi_set_reuse";

    int lv_value = 1;
    int lv_err = setsockopt(pv_sock,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            (char *) &lv_value,
                            sizeof(lv_value));
    if (gv_xmpi_trace)
        printf("%s: %s sock=%d, err=%d\n",
               pp_where,
               FUNCTION,
               pv_sock,
               lv_err);
    return lv_err;
}

char *XMPIErrMsg(int err)
{
    static char strBuf[XMPI_STRING_BUF_SIZE] = { 0 };
    snprintf( strBuf, sizeof(strBuf), "MPI Error=%d ", err );
    return(strBuf);
}

int XMPI_Close_port(const char *port_name)
{
    const char *WHERE = "XMPI_Close_port";

    if (gv_xmpi_trace)
        printf("%s: ENTER port-name=%s\n", WHERE, port_name);
    return MPI_SUCCESS;
}

int XMPI_Comm_accept(const char *port_name,
                     MPI_Info    info,
                     int         root,
                     MPI_Comm    comm,
                     MPI_Comm   *newcomm)
{
    const char *WHERE = "XMPI_Comm_accept";
    info = info;
    root = root;

    if (gv_xmpi_trace)
        printf("%s: ENTER port-name=%s, comm=0x%x, newcomm=%p\n", WHERE, port_name, comm, newcomm);

    struct sockaddr lv_c_addr;
    socklen_t lv_c_addr_len = sizeof(lv_c_addr);
    if (gv_xmpi_trace)
        printf("%s: accept attempt to sock=%d\n", WHERE, gv_xmpi_listen_sock);
    int lv_sock = accept(gv_xmpi_listen_sock, &lv_c_addr, &lv_c_addr_len);
    assert(lv_sock != -1);
    int lv_err = xmpi_set_reuse(WHERE, lv_sock);
    assert(lv_err == 0);
    lv_err = xmpi_set_nodelay(WHERE, lv_sock);
    assert(lv_err == 0);
    *newcomm = lv_sock | XMPI_COMM_MASK;
    if (gv_xmpi_trace)
        printf("%s: EXIT accept completed on sock=%d, new sock=%d\n",
               WHERE, gv_xmpi_listen_sock, lv_sock);
    return MPI_SUCCESS;
}

int XMPI_Comm_connect(const char *port_name,
                      MPI_Info    info,
                      int         root,
                      MPI_Comm    comm,
                      MPI_Comm   *newcomm)
{
    const char *WHERE = "XMPI_Comm_connect";
    char        la_host[1000];
    char        la_hostbuf[1000];
    const char *lp_colon;
    int         lv_hostenterrno;
    uint16_t    lv_port;
    info = info;
    root = root;

    if (gv_xmpi_trace)
        printf("%s: ENTER port-name=%s, comm=0x%x, newcomm=%p\n", WHERE, port_name, comm, newcomm);
    lp_colon = strstr(port_name, ":");
    assert(lp_colon != NULL);
    strcpy(la_host, port_name);
    int lv_len = lp_colon - port_name;
    la_host[lv_len] = '\0';
    lv_port = atoi(&lp_colon[1]);
    int lv_sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(lv_sock != -1);
    int lv_err = xmpi_set_reuse(WHERE, lv_sock);
    assert(lv_err == 0);
    lv_err = xmpi_set_nodelay(WHERE, lv_sock);
    assert(lv_err == 0);
    struct sockaddr_in lv_addr;
    memset(&lv_addr, 0, sizeof(lv_addr));
    lv_addr.sin_family = AF_INET;
    lv_addr.sin_port = htons((uint16_t) lv_port);
    struct hostent lv_hostent;
    struct hostent *lp_hostent;
    lv_err = gethostbyname_r(la_host,
                             &lv_hostent,
                             la_hostbuf,
                             sizeof(la_hostbuf),
                             &lp_hostent,
                             &lv_hostenterrno);
    assert(lv_err == 0);
    assert(lp_hostent != NULL);
    assert(*(long *) lp_hostent->h_addr != -1);
    lv_addr.sin_addr.s_addr = (uint32_t) *(uint32_t *) lp_hostent->h_addr;
    if (gv_xmpi_trace)
        printf("%s: connect attempt to host=%s, port=%d\n",
               WHERE, la_host, lv_port);
    lv_err = connect(lv_sock,
                     (struct sockaddr *) &lv_addr,
                     sizeof(struct sockaddr));
    if (gv_xmpi_trace)
    {
        if (lv_err)
            printf("%s: connect failure, errno=%d\n", WHERE, errno);
        else
            printf("%s: connect completed\n", WHERE);
    }
    int lv_ret;
    if (lv_err == -1)
    {
        lv_ret = MPI_ERR_NAME;
    }
    else
    {
        assert(lv_err != -1);
        *newcomm = lv_sock | XMPI_COMM_MASK;
        lv_ret = MPI_SUCCESS;
    }

    return lv_ret;
}

int XMPI_Comm_disconnect(MPI_Comm *comm)
{
    const char *WHERE = "XMPI_Comm_disconnect";

    if (gv_xmpi_trace)
        printf("%s: ENTER comm=%d\n", WHERE, *comm);
    if ((*comm & XMPI_COMM_MASK) == XMPI_COMM_MASK)
    {
        xmpi_chk_valid_comm(*comm);
        *comm = 0;
    }
    else
    {
        // ignore
    }
    return MPI_SUCCESS;
}

int XMPI_Comm_set_errhandler(MPI_Comm       comm,
                             MPI_Errhandler errhandler)
{
    const char *WHERE = "XMPI_Comm_set_errhandler";
    errhandler = errhandler;

    if (gv_xmpi_trace)
        printf("%s: ENTER comm=%d\n", WHERE, comm);
    xmpi_chk_valid_comm(comm);
    return MPI_SUCCESS;
}

int XMPI_Open_port(MPI_Info info, char *port_name)
{
    const char *WHERE = "XMPI_Open_port";
    static char la_host[1000];
    char        la_hostbuf[1000];
    int         lv_err;
    int         lv_hostenterrno;
    uint16_t    lv_port;
    info = info;

    if (gv_xmpi_trace)
        printf("%s: ENTER port_name=%p\n", WHERE, port_name);
    lv_err = gethostname(la_host, sizeof(la_host));
    assert(lv_err == 0);
    int lv_sock = socket(AF_INET, SOCK_STREAM, 0);
    assert(lv_sock != -1);
    lv_err = xmpi_set_reuse(WHERE, lv_sock);
    assert(lv_err == 0);
    struct sockaddr_in lv_addr;
    lv_port = 0;
    memset(&lv_addr, 0, sizeof(lv_addr));
    lv_addr.sin_family = AF_INET;
    lv_addr.sin_port = htons(lv_port);
    struct hostent lv_hostent;
    struct hostent *lp_hostent;
    lv_err = gethostbyname_r(la_host,
                             &lv_hostent,
                             la_hostbuf,
                             sizeof(la_hostbuf),
                             &lp_hostent,
                             &lv_hostenterrno);
    assert(lv_err == 0);
    assert(lp_hostent != NULL);
    bool lv_done = false;
    for (int lv_inx = 0;; lv_inx++)
    {
        long *lp_addr = (long *) lp_hostent->h_addr_list[lv_inx];
        if (lp_addr == NULL)
            break;
        if (*lp_addr == 0)
            break;
        lv_addr.sin_addr.s_addr = (int) *lp_addr;
        if (gv_xmpi_trace)
        {
            unsigned char *lp_addrp = (unsigned char *) lp_addr;
            printf("%s: trying to bind to host=%d.%d.%d.%d:%d, sock=%d\n",
                   WHERE, lp_addrp[0], lp_addrp[1],
                   lp_addrp[2], lp_addrp[3], lv_port,
                   lv_sock);
        }
        lv_err = bind(lv_sock, (struct sockaddr *) &lv_addr,
                      sizeof(struct sockaddr));
        if (lv_err == 0)
        {
            lv_done = true;
            break;
        } else
        {
            if (gv_xmpi_trace)
                printf("%s: bind failed, err=%d, sock=%d, host=%s\n",
                       WHERE, errno, lv_sock, la_host);
        }
    }
    assert(lv_done);
    lv_err = listen(lv_sock, 1);
    assert(lv_err != -1);
    gv_xmpi_listen_sock = lv_sock;
    socklen_t lv_len = sizeof(lv_addr);
    lv_err = getsockname(lv_sock, (struct sockaddr *) &lv_addr, &lv_len);
    assert(lv_err == 0);
    lv_port = ntohs(lv_addr.sin_port);
    unsigned char *lp_addrp = (unsigned char *) &lv_addr.sin_addr.s_addr;
    if (gv_xmpi_trace)
    {
        printf("%s: bound to host=%d.%d.%d.%d:%d, sock=%d\n",
               WHERE, lp_addrp[0], lp_addrp[1],
               lp_addrp[2], lp_addrp[3], lv_port,
               lv_sock);
    }
    sprintf(port_name, "%d.%d.%d.%d:%d",
            lp_addrp[0],
            lp_addrp[1],
            lp_addrp[2],
            lp_addrp[3],
            lv_port);
    if (gv_xmpi_trace)
        printf("%s: EXIT OK port_name=%s\n", WHERE, port_name);
    return MPI_SUCCESS;
}

int XMPI_Recv(void         *buf,
              int           count,
              MPI_Datatype  datatype,
              int           source,
              int           tag,
              MPI_Comm      comm,
              MPI_Status   *status)
{
    const char *WHERE = "XMPI_Recv";
    int recvCount = 0;

    if (gv_xmpi_trace)
        printf("%s: ENTER buf=%p, count=%d, comm=0x%x\n", WHERE, buf, count, comm);
    xmpi_chk_valid_comm(comm);
    int lv_count;
    int lv_err = xmpi_recv(WHERE, WHERE, comm & ~XMPI_COMM_MASK, buf, count, &lv_count);
    int lv_ret;
    if (lv_err == -1)
    {
        lv_ret = MPI_ERR_EXITED;
        if (gv_xmpi_trace)
            printf("%s: EXIT EXITED\n", WHERE);
    }
    else
    {
        assert(lv_err == 0);
        status->MPI_SOURCE = source;
        status->MPI_TAG = tag;
        status->MPI_ERROR = MPI_SUCCESS;
        switch ( datatype )
        {
            case MPI_CHAR:
                count = recvCount;
                break;
            case MPI_INT:
                count = recvCount / sizeof(int);
                break;
        }
        status->count = lv_count;
        lv_ret = MPI_SUCCESS;
        if (gv_xmpi_trace)
            printf("%s: EXIT OK\n", WHERE);
    }
    return lv_ret;
}

int XMPI_Send(const void   *buf,
              int           count,
              MPI_Datatype  datatype,
              int           dest,
              int           tag,
              MPI_Comm      comm)
{
    const char *WHERE = "XMPI_Send";
    int sendCount;
    dest = dest;
    tag = tag;

    switch ( datatype )
    {
        case MPI_CHAR:
            sendCount = count;
            break;
        case MPI_INT:
            sendCount = count * sizeof(int);
            break;
    }
    
    if (gv_xmpi_trace)
        printf("%s: ENTER buf=%p, count=%d, sendCount=%d, comm=0x%x\n", 
               WHERE, buf, count, sendCount, comm);
    xmpi_chk_valid_comm(comm);
    int lv_err = xmpi_send(WHERE, WHERE, comm & ~XMPI_COMM_MASK, (void *) buf, sendCount);
    assert(lv_err == sendCount);
    if (gv_xmpi_trace)
        printf("%s: EXIT OK\n", WHERE);
    return MPI_SUCCESS;
}

int XMPI_Sendrecv(const void   *sendbuf,
                  int           sendcount,
                  MPI_Datatype  sendtype,
                  int           dest,
                  int           sendtag,
                  void         *recvbuf,
                  int           recvcount,
                  MPI_Datatype  recvtype,
                  int           source,
                  int           recvtag,
                  MPI_Comm      comm,
                  MPI_Status   *status)
{
    const char *WHERE = "XMPI_Sendrecv";
    source = source;

    if (gv_xmpi_trace)
        printf("%s: ENTER sendbuf=%p, sendcount=%d, recvbuf=%p, recvcount=%d, comm=0x%x\n", WHERE, sendbuf, sendcount, recvbuf, recvcount, comm);
    xmpi_chk_valid_comm(comm);
    int lv_err = XMPI_Send(sendbuf,
                           sendcount,
                           sendtype,
                           dest,
                           sendtag,
                           comm);
    assert(lv_err == 0);
    lv_err = XMPI_Recv(recvbuf,
                       recvcount,
                       recvtype,
                       dest,
                       recvtag,
                       comm,
                       status);
    int lv_ret;
    if (lv_err)
    {
        lv_ret = MPI_ERR_EXITED;
        if (gv_xmpi_trace)
            printf("%s: EXIT EXITED, %s\n", WHERE, XMPIErrMsg(lv_err));
    }
    else
    {
        if (gv_xmpi_trace)
            printf("%s: EXIT OK\n", WHERE);
        lv_ret = MPI_SUCCESS;
    }
    return lv_ret;
}

