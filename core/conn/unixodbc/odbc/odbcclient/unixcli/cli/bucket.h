/**********************************************************************
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
********************************************************************/
/**************************************************************************
**************************************************************************/

#ifndef BUCKET_H
#define BUCKET_H

#ifdef MXOSS
#include <spthread.h>
#else
#include <pthread.h>
#endif

#include "transportbase.h"
#include "TCPIPUnixDrvr.h"
#include "secpwd.h"

#define NOTSIGNALED 0
#define SIGNALED    1

#define SQL_ATTR_CONNECT_STREAMS 1
#define MAX_STREAMS	512

typedef struct master_def {
    pthread_mutex_t master_mutex;
    pthread_cond_t      done;   
    long                status; //status of the total set; this is either SIGNALED or NOTSIGNALED
	CEE_status			retcode;
    long                work_tbd;
} master_t;

typedef struct bucket_def {
	pthread_mutex_t mutex;
	pthread_cond_t      run;    //signal thread to run
	pthread_attr_t      attr;   
	pthread_t           thread; 
	char                *rbuffer;
	char				*wbuffer;
	IDL_long			wlength, rlength;
	IDL_long			VarBufferLength;
	IDL_OBJECT_def		SQLSvc_ObjRef;
	IDL_long			cursorLength;
	CTCPIPUnixDrvr		*m_srvrTCPIPSystem;
	int					status; 	// we need some way to return the status to the master
	master_t            *master;

	IDL_long             prevSent;    // Was something sent in the previous Execute2 ? 
									 // If nothing was sent in this stream we don't want to
									 // parse the read buffers to map the errors
	IDL_long             currentSent; // Is something being sent in the current Execute2 ?
	                                 // after Execute2, prevSent becomes currentSent
	SecPwd*				 m_SecPwd;
	ProcInfo			 m_SecInfo;
	SQLUINTEGER		     m_SecurityMode;
} bucket_t;

extern void *send_IObuckets(void *arg);

typedef struct hashinfo_def {
	short 			hashType;
	short			hashPart;
	short			*segNum;
	short			*partNum;
	short			*hashCol;
	short 			colCount;
	long			*HashColKeys;
	unsigned int	*HashColLens;
	BYTE			**HashedVarBuffer;
	BYTE			**HashedmemPtr;
	IDL_unsigned_long	*oHashedVarLocation;
	IDL_unsigned_long	*oHashedIndLocation;
	IDL_long			*HashedinputRowCnt;
	IDL_long			*HashedrowNumber;
	IDL_long			*HashedVarBufferLength;
	IDL_long			*HashedCurRowCnt;
	// the following variables are used to recover the error
	// status as it comes back from the server
	IDL_long			ErrorWarningLength;
	BYTE				*ErrorWarning;
	IDL_long			**PreviousHashMapTable;
	IDL_long 			**CurrentHashMapTable;
	int					PreviousHashMapTableSize;
	int					CurrentHashMapTableSize;
	int					**statusArray;	// this does NOT have anything to do with CDesc::m_DescArrayStatusPtr; it is an array of pointers to warningOrError message tokens
	IDL_long			statusArraySize;
	//the following variables help to delay driver-side info for rowsets, in delayed error mode
	SQLUINTEGER			PreviousDescArraySize;
	SQLUSMALLINT		*PreviousDescArrayStatusPtr;
	SQLRETURN			PreviousRowsetRetCode;
	SQLRETURN			CurrentRowsetRetCode;
	SQLUINTEGER			CurrentDescArraySize;
	SQLUSMALLINT		*CurrentDescArrayStatusPtr;
} hashinfo_t;

#endif
