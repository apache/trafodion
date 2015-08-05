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
#include <mpi.h>
#include <netdb.h>

#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#define XMPI_STRING_BUF_SIZE 256

enum { XMPI_PMH_SIG0     = 'M' };
enum { XMPI_PMH_SIG1     = 'A' };
enum { XMPI_PMH_SIG2     = 'R' };
enum { XMPI_PMH_SIG3     = 'K' };
enum { XMPI_PMH_MAJOR    = 1 };
enum { XMPI_PMH_MINOR    = 0 };
enum { XMPI_PMH_ORDER_BE = 0 };
enum { XMPI_PMH_ORDER_LE = 1 };

enum { XMPI_COMM_MASK    = 0xf0000000 };

typedef enum
{
    XMPI_RSTATE_INIT,
    XMPI_RSTATE_INIT2,
    XMPI_RSTATE_HDR,
    XMPI_RSTATE_DATA,
    XMPI_RSTATE_DONE,
    XMPI_RSTATE_LAST
} XMPI_Recv_State;

/* PMH = Proto Msg Hdr */
typedef struct
{
    char  ia_sig[4];   /* 0x00: see XMPI_PROTO_HDR_SIG1-4 */
    int   iv_dsize;    /* 0x04: data size */
} XMPI_PMH_Type;

typedef struct XMPI_Rd_Type
{
    char          *ip_data;            /* data buf */
    const char    *ip_where;           /* where */
    int            iv_data_len;        /* data buf len */
    XMPI_PMH_Type  iv_hdr;             /* header */
    int            iv_mpi_source_rank; /* source rank */
    int            iv_mpi_tag;         /* tag */
} XMPI_Rd_Type;

extern int           gv_xmpi_count_read;
extern int           gv_xmpi_listen_sock;
extern int           gv_xmpi_read_count;
extern XMPI_Rd_Type  gv_xmpi_rd;
extern int           gv_xmpi_state;
extern bool          gv_xmpi_trace;

char *XMPIErrMsg(int err);

int XMPI_Close_port(const char *port_name);

int XMPI_Comm_accept(const char *port_name,
                     MPI_Info    info,
                     int         root,
                     MPI_Comm    comm,
                     MPI_Comm   *newcomm);

int XMPI_Comm_connect(const char *port_name,
                      MPI_Info    info,
                      int         root,
                      MPI_Comm    comm,
                      MPI_Comm   *newcomm);

int XMPI_Comm_disconnect(MPI_Comm *comm);

int XMPI_Comm_set_errhandler(MPI_Comm       comm,
                             MPI_Errhandler errhandler);

int XMPI_Close_port(const char *port_name);

int XMPI_Open_port(MPI_Info info, char *port_name);

int XMPI_Recv(void         *buf,
              int           count,
              MPI_Datatype  datatype,
              int           source,
              int           tag,
              MPI_Comm      comm,
              MPI_Status   *status);

int XMPI_Send(const void   *buf,
              int           count,
              MPI_Datatype  datatype,
              int           dest,
              int           tag,
              MPI_Comm      comm);

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
                  MPI_Status   *status);

