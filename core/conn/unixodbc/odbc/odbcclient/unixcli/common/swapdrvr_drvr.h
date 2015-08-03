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

#ifndef SWAPDRVR_DRVR_H
#define SWAPDRVR_DRVR_H

#include "swap.h"

void PROCESS_swap(char* buffer, int odbcAPI);

void GETOBJREF_swap(char* buffer);
void UPDATESRVRSTATE_swap(char* buffer);
void STOPSRVR_swap(char* buffer);

void SQLCONNECT_swap(char* buffer);
void SQLDISCONNECT_swap(char* buffer);
void SQLSETCONNECTATTR_swap(char* buffer);
void SQLENDTRAN_swap(char* buffer);
void SQLPREPARE_swap(char* buffer);
void SQLPREPAREROWSET_swap(char* buffer);
void SQLEXECDIRECTROWSET_swap(char* buffer);
void SQLEXECUTEROWSET_swap(char* buffer);
void SQLFETCHROWSET_swap(char* buffer);
void SQLEXECDIRECT_swap(char* buffer);
void SQLEXECUTE_swap(char* buffer);
void SQLFETCHPERF_swap(char* buffer);
void SQLFREESTMT_swap(char* buffer);
void SQLGETCATALOGS_swap(char* buffer);
void SQLEXECUTECALL_swap(char* buffer);


#endif

