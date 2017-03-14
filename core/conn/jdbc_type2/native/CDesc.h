/*************************************************************************
*
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
**************************************************************************/
//
// MODULE: CDesc.h
//
//
// PURPOSE: Defines CSrvrStmt class
//
// 
#ifndef _CDESC_DEFINED
#define _CDESC_DEFINED

#include <platform_ndcs.h>

class SRVR_DESC_HDL {
public:
	long	dataType;
	BYTE	*varPtr;
	BYTE	*indPtr;
	long	charSet;
	long	length;
	long	precision;
	long	scale;
	long	sqlDatetimeCode;
	long	FSDataType;
	long	paramMode;
	long    vc_ind_length;
};

typedef struct tagDESC_HDL_LISTSTMT
{
    long DataType;
    long Length;
    long Nullable;
    long VarBuf;
    long IndBuf;
} DESC_HDL_LISTSTMT;

#endif
