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

#ifndef SWAPS_SRVR_H
#define SWAPS_SRVR_H

#include "swap.h"

void PROCESS_res_swap(char* buffer, int odbcAPI);

void SQLCONNECT_res_swap(char* buffer);
void SQLDISCONNECT_res_swap(char* buffer);
void SQLSETCONNECTATTR_res_swap(char* buffer);
void SQLENDTRAN_res_swap(char* buffer);
void SQLPREPARE_res_swap(char* buffer);
void SQLPREPAREROWSET_res_swap(char* buffer);
void SQLEXECDIRECTROWSET_res_swap(char* buffer);
void SQLEXECUTEROWSET_res_swap(char* buffer);
void SQLFETCHROWSET_res_swap(char* buffer);
void SQLEXECDIRECT_res_swap(char* buffer);
void SQLEXECUTE_res_swap(char* buffer);
void SQLFETCHPERF_res_swap(char* buffer);
void SQLFREESTMT_res_swap(char* buffer);
void SQLGETCATALOGS_res_swap(char* buffer);
void SQLEXECUTECALL_res_swap(char* buffer);
void STOPSRVR_res_swap(char* buffer);
void ENABLETRACE_res_swap(char* buffer);
void DISABLETRACE_res_swap(char* buffer);
void ENABLESTATISTICS_res_swap(char* buffer);
void DISABLESTATISTICS_res_swap(char* buffer);
void UPDATECONTEXT_res_swap(char* buffer);

#endif
