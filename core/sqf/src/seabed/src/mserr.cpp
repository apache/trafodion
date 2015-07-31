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
#include <string.h>

#include <mpi.h>

#include "seabed/fserr.h"
#include "seabed/trace.h"

#include "mserr.h"
#include "mstrace.h"
#include "util.h"


//
// Purpose: map errno to MPI error
//
int ms_err_errno_to_mpierr(const char *pp_where) {
    int lv_mpierr;

    switch (errno) {
    case 0:
        lv_mpierr = MPI_ERR_OTHER; // errno wasn't set - probably a bug
        break;

    case EIDRM:
        // errno: the message queue was removed
        // mpi:   path to monitor is down
        lv_mpierr = MPI_ERR_EXITED;
        break;

    case ENOMEM:
        // errno: the system does not have enough memory
        // mpi:   no space
        lv_mpierr = MPI_ERR_NO_MEM;
        break;

    case EAGAIN:
        // errno: the limit of signals which may be queued has been reached
        // errno: the  system  lacked the necessary resources...
        // mpi:   no space
        lv_mpierr = MPI_ERR_NO_MEM;
        break;

    case ESRCH:
        // errno: No process has a PID matching pid
        // mpi:   path to monitor is down
        lv_mpierr = MPI_ERR_EXITED;
        break;

    case EPERM:
        // errno: the process does not have permission to send the signal...
        // mpi:   path to monitor is down
        lv_mpierr = MPI_ERR_EXITED;
        break;

    case EACCES:
        // errno: the user does not have permission to access the shared memory  segment...
        // mpi:   path to monitor is down
        lv_mpierr = MPI_ERR_EXITED;
        break;

    case ENFILE:
        // errno: the system limit on the total number of open files has been reached.
        // mpi:   path to monitor is down
        lv_mpierr = MPI_ERR_EXITED;
        break;

    case ENOENT:
        // errno: no segment exists for the given key, and IPC_CREAT was not specified
        // mpi:   path to monitor is down
        lv_mpierr = MPI_ERR_EXITED;
        break;

    case ENOSPC:
        // errno: all possible shared memory IDs have been taken (SHMMNI)...
        // mpi:   path to monitor is down
        lv_mpierr = MPI_ERR_EXITED;
        break;

    default:
        lv_mpierr = MPI_ERR_OTHER;
        break;
    }

    if ((lv_mpierr != MPI_SUCCESS) && gv_ms_trace_errors)
        trace_where_printf(pp_where, "EXIT FAILURE errno=%d, mpierr=%d\n",
                           errno, lv_mpierr);
    return lv_mpierr;
}

short ms_err_mpi_to_fserr(const char *pp_where, int pv_mpierr) {
    return ms_err_status_mpi_to_fserr(pp_where, pv_mpierr);
}


//
// Purpose: map sock error to FS error
//
short ms_err_sock_to_fserr(const char *pp_where, int pv_sockerr) {
    char  la_errno[100];
    short lv_fserr;

    if (pv_sockerr)
        lv_fserr = XZFIL_ERR_PATHDOWN;
    else
        lv_fserr = XZFIL_ERR_OK;
    if ((lv_fserr != XZFIL_ERR_OK) && gv_ms_trace_errors) {
        char  la_sockerr[100];
        char *lp_p;

        lp_p = strerror_r(pv_sockerr, la_sockerr, sizeof(la_sockerr));
        if (lp_p == NULL)
            sprintf(la_sockerr, "errno=%d", pv_sockerr);
        trace_where_printf(pp_where, "EXIT FAILURE sockerr=%d(%s), fserr=%d\n",
                           pv_sockerr,
                           strerror_r(pv_sockerr, la_errno, sizeof(la_errno)),
                           lv_fserr);
       }
    return lv_fserr;
}

short ms_err_status_mpi_to_fserr(const char *pp_where,
                                 int         pv_mpierr) {
    short lv_fserr;
    int   lv_errorclass;

    // catch double conversions
    SB_util_assert((pv_mpierr >= MPI_SUCCESS) || // sw fault
                   (pv_mpierr <= MPI_ERR_LASTCODE));

    if (pv_mpierr == MPI_SUCCESS)
        lv_fserr = XZFIL_ERR_OK;
    else {
            lv_errorclass = pv_mpierr;

        switch (lv_errorclass) {
        case MPI_ERR_BUFFER:        // invalid buffer
        case MPI_ERR_COUNT:         // invalid count
        case MPI_ERR_TYPE:          // invalid datatype
        case MPI_ERR_TAG:           // invalid tag
        case MPI_ERR_COMM:          // invalid communicator
        case MPI_ERR_ROOT:          // invalid root
        case MPI_ERR_GROUP:         // invalid group
        case MPI_ERR_OP:            // invalid operation
        case MPI_ERR_TOPOLOGY:      // invalid topology
        case MPI_ERR_DIMS:          // invalid dimension
        case MPI_ERR_ARG:           // invalid argument
            lv_fserr = XZFIL_ERR_BOUNDSERR;
            break;

        case MPI_ERR_UNKNOWN:       // unknown error
            lv_fserr = XZFIL_ERR_FSERR;
            break;

        case MPI_ERR_TRUNCATE:      // message truncated
            lv_fserr = XZFIL_ERR_OVERRUN;
            break;

        case MPI_ERR_OTHER:         // other known error
        case MPI_ERR_INTERN:        // internal MPI error
            lv_fserr = XZFIL_ERR_FSERR;
            break;

        case MPI_ERR_IN_STATUS:     // error code in status
                lv_fserr = XZFIL_ERR_FSERR;
            break;

        case MPI_ERR_PENDING:       // pending request
            lv_fserr = XZFIL_ERR_FSERR;
            break;

        case MPI_ERR_REQUEST:       // invalid request
        case MPI_ERR_INFO:          // invalid info arg
        case MPI_ERR_INFO_NOKEY:    // info key not defined
        case MPI_ERR_INFO_KEY:      // invalid info key
        case MPI_ERR_INFO_VALUE:    // invalid info value
        case MPI_ERR_WIN:           // invalid window arg
        case MPI_ERR_BASE:          // invalid window base
        case MPI_ERR_SIZE:          // invalid window size
        case MPI_ERR_DISP:          // invalid window unit disp.
        case MPI_ERR_LOCKTYPE:      // invalid locktype arg
        case MPI_ERR_ASSERT:        // invalid assert arg
            lv_fserr = XZFIL_ERR_BOUNDSERR;
            break;

        case MPI_ERR_RMA_CONFLICT:  // conflicting access to win
            lv_fserr = XZFIL_ERR_DEVERR;
            break;

        case MPI_ERR_RMA_SYNC:      // invalid window sync
            lv_fserr = XZFIL_ERR_BOUNDSERR;
            break;

        case MPI_ERR_NO_MEM:        // out of "special" memory
            lv_fserr = XZFIL_ERR_NOBUFSPACE;
            break;

        case MPI_ERR_KEYVAL:        // invalid key value
            lv_fserr = XZFIL_ERR_BOUNDSERR;
            break;

        case MPI_ERR_SPAWN:         // spawn error
            lv_fserr = XZFIL_ERR_FSERR;
            break;

        case MPI_ERR_PORT:          // invalid port name
            // can happen during nonstop-testing
            lv_fserr = XZFIL_ERR_NOSUCHDEV;
            break;

        case MPI_ERR_SERVICE:       // invalid service name
            lv_fserr = XZFIL_ERR_PATHDOWN;
            break;

        case MPI_ERR_NAME:          // nonexist service name
            lv_fserr = XZFIL_ERR_NOSUCHDEV;
            break;

        case MPI_ERR_EXITED:        // peer rank is exited
            lv_fserr = XZFIL_ERR_PATHDOWN;
            break;

        default:
            lv_fserr = XZFIL_ERR_FSERR;
        }
    }
    if ((lv_fserr != XZFIL_ERR_OK) && gv_ms_trace_errors)
        trace_where_printf(pp_where, "EXIT FAILURE mpierr=%d, fserr=%d\n",
                           pv_mpierr, lv_fserr);
    return lv_fserr;
}
