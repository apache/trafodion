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

// vim: noai:ts=4
#ifndef _SM_H
#define _SM_H
#if !defined(__KERNEL__)
#include <stdint.h>
#include <sys/types.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

#define SM_ERR_NOSERVICE		-1
#define SM_ERR_INTERNAL			-2
#define SM_ERR_SIZE			-3
#define SM_ERR_TARGET			-4
#define SM_ERR_CALL_ORDER		-5
#define SM_ERR_PREPOST			-6
#define SM_ERR_EXIST			-7
#define SM_ERR_NOMATCH			-8
#define SM_ERR_NOPEER			-9
#define SM_ERR_TRUNC			-10
#define SM_ERR_INCHUNK			-11
#define SM_ERR_EAGAIN			-12
#define SM_ERR_QUEUED			-13
#define SM_ERR_INVAL			-14
#define SM_ERR_LOCAL_TARGET		-15
#define SM_ERR_REMOTE_TARGET		-16
#define SM_ERR_NODE_DOWN		-17

typedef int64_t sm_id_t;
typedef char * sm_handle_t;

typedef struct {
	sm_id_t	id;
	int		node;
	pid_t	pid;
	int		verifier;
	int		tag;
} sm_target_t;

#define SM_FLAG_PREPOST				1	// SM_put()
										//	Input:	This chunk is a buffer to
										//			post
										// SM_get()
										//	Output: This chunk is a preposted
										//			buffer that has been filled
#define SM_FLAG_RECVANY				2	// SM_put()
										//	Input:
										//		With SM_PREPOST:
										//			post command, this chunk
										//			can be filled by any source
										//		Without SM_PREPOST
										//			send comand, this chunk
										//			can be only be filled by
										//			<node,pid>
#define SM_FLAG_SENDNOTIFICATION	4	// SM_get()
										//	Output:	This chunk is notification
										//			that a send has been
										//			delivered
#define SM_FLAG_SEND_TO_PREPOST		8	// SM_put()
										//	Cannot be used with SM_FLAG_PREPOST
										//	Input:	send command, this chunk
										//			will fill a preposted recv
										//			buffer.
										//	If not set, then this is a short
										//	message that will not fill a
										//	preposted buffer
#define SM_MAX_FLAGS 15
typedef struct {
	sm_target_t	tgt;
	char		*buff;
	char		*handle;
	uint64_t	hdr_payload;
	uint32_t	size;
	int16_t		errcode;
	uint16_t	flags;
} sm_chunk_t;

// SM_ctl commands
#define SM_GET_BUFFSIZE	1
#define SM_GET_MAXNONPP	2
#define SM_GET_MAXINTRABUFFSIZE 3

extern int SM_init(int channel, int node, int verifier);
extern int SM_finalize(int channel);

extern int SM_register(int channel, sm_id_t id);
extern int SM_cancel(int channel, sm_id_t id);

extern int SM_put(int channel, int nchunks, sm_chunk_t *chunks);
extern int SM_get(int channel, sm_chunk_t *chunks[], int *nchunks, sm_handle_t *handle);
extern int SM_get_done(int channel, sm_handle_t handle);
extern int SM_ctl(int channel, int cmd, void *ptr);

extern int SM_strerror_r(int errnum, char *buf, size_t buflen);
extern int SM_get_stats(pid_t spid, int fl_clear);

#ifdef __cplusplus
}
#endif
#endif
