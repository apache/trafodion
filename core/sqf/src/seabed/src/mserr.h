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

#ifndef __SB_MSERR_H_
#define __SB_MSERR_H_

#include <mpi.h>

extern int   ms_err_errno_to_mpierr(const char *pp_where);
extern short ms_err_mpi_to_fserr(const char *pp_where, int pv_mpierr);
extern short ms_err_sm_to_fserr(const char *pp_where, int pv_smerr);
extern short ms_err_sock_to_fserr(const char *, int);
extern short ms_err_status_mpi_to_fserr(const char *pp_where,
                                        int         pv_mpierr);

#endif // !__SB_MSERR_H_
