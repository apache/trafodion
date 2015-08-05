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

#ifndef SWAP_H
#define SWAP_H

void 
PROCESS_res_swap(char* buffer, int odbcAPI);

void 
swapPointers(char* buffer, short number_of_param);

void 
SHORT_swap(short* pshort);
void 
USHORT_swap(unsigned short* pshort);
void 
LONG_swap(long* plong);
void 
ULONG_swap(unsigned long* pulong);
void 
POINTER_swap(void* ppointer);

#ifndef NSK_PLATFORM
void 
LONGLONG_swap(__int64* plonglong);
#else
void 
LONGLONG_swap(long long* plonglong);
#endif

void 
VERSION_LIST_swap(char* buffer, VERSION_LIST_def* pverlist);
void 
HEADER_swap(HEADER* header);
void 
ERROR_DESC_LIST_swap(char* buffer, ERROR_DESC_LIST_def *errorList);
void 
OUT_CONNECTION_CONTEXT_swap(char* buffer, OUT_CONNECTION_CONTEXT_def *outContext);
void 
SQL_VALUE_LIST_swap(char* buffer, SQLValueList_def *valueList);
void
SQL_ITEM_DESC_LIST_swap(char* buffer, SQLItemDescList_def *itemList);


#endif

