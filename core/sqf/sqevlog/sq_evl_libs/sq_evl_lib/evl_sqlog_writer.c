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
/*
 * Logging API based on evl_log_write.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <wchar.h>

#include <ctype.h>
#include <sys/klog.h>
#include "sqevlog/evl_sqlog_writer.h"

/* Seabed library includes */
#include "seabed/ms.h"


/* This function adds common tokens and values to all evl log message
 * Returns the current total buff length
*/

int evl_sqlog_init(char *buf, size_t buf_maxlen)
{

	int ret_code = 0;
	int ev_compid;
	int ev_processid;
	int ev_zoneid;
	long ev_threadid;
	/* initialize common token values by calling seabed library */

	msg_mon_get_my_info2(NULL,
                             &ev_processid,
                             NULL,
                             0,
                             NULL,
                             &ev_zoneid,
                             NULL,
                             &ev_threadid,
                             &ev_compid);

	/* passing values to common header struct */

	sq_common_header_t common_tokens;
	/* common_tokens = malloc(sizeof(sq_common_header_t));	*/
	/* For g++4 */
//	common_tokens = (sq_common_header_t *)malloc(sizeof(sq_common_header_t));
	common_tokens.comp_id = ev_compid;
	common_tokens.process_id = ev_processid;
	common_tokens.zone_id = ev_zoneid;
	common_tokens.thread_id = (int)ev_threadid;

	/* Adding common tokens to log message */
        ret_code = evl_sqlog_init_header(buf, buf_maxlen, &common_tokens);
	if ( ret_code != 0) return ret_code;

	return 0;

}

