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
 * Simple logging program for sqevlog.
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "sqevlog/evl_sqlog_writer.h"

int
main()
{
	int status;
	int type;
	long larray[5]={30L,31L,32L,33L,34L};
	int iarray[5] = {1,2,3,4,5};

	long long llarray[5] = {10LL,11LL,12LL,13LL,14LL};
	unsigned long long ullarray[5]={20LL,21LL,22LL,23LL,24L};
	short sarray[5]={1,2,3,4,5};
	char carray[4]={'l','o','v','e'};
	unsigned char ucarray[4]={0xff,0x10,0xfe,0x20};
	long double ldarray[2]={2.33333, 3.333333};
	long double ld1 = 1.7676764;

	void *addr1 = (void*) 0xfeedf00d;

	char *plal[4] = { (char *) "peace", (char *) "love", (char *) "and", (char *) "linux" };

	int facility = SQ_LOG_SEAQUEST;
	type = 1;
	int severity = SQ_LOG_CRIT;


        char evl_buf[7 * 1024];
        char *p = evl_buf;
	float ff = 0.123;
	double dd = 9.999;
	char *cc = (char*)'c';

	/* strut is defined for Monitor to pass values to common headers */
	/* Other seaquest modules won't use this struct */
	sq_common_header_t* sq_header;
	sq_header = (sq_common_header_t*)malloc(sizeof(sq_common_header_t));
	sq_header->comp_id = 8888;
	sq_header->process_id = 1122;
	sq_header->zone_id = 57322;
	sq_header->thread_id = 99;


	/* Log init function called by all seaquest modules except Monitor */
	/* status = evl_sqlog_init(p, 7 * 1024);  */

        /* The next init function will be called by Monitor only */
	status = evl_sqlog_init_header(p, 7 * 1024, sq_header);

	/* Adding personal tokens */
        status = evl_sqlog_add_array_token(p, TY_CHAR, plal, 4);
        status = evl_sqlog_add_token(p, TY_LDOUBLE, &ld1);

        status = evl_sqlog_add_token(p, TY_FLOAT, &ff);

        status = evl_sqlog_add_token(p, TY_DOUBLE, &dd);

        status = evl_sqlog_add_token(p, TY_INT, (int*)10);
        status = evl_sqlog_add_token(p, TY_SHORT, (short*)10);
        status = evl_sqlog_add_token(p, TY_STRING, (void*)"TEST PROGRAM IS DONE");
        status = evl_sqlog_add_token(p, TY_LONG, (long*)0xf0f0f0f0L);
        status = evl_sqlog_add_token(p, TY_CHAR, cc);
        status = evl_sqlog_add_token(p, TY_ADDRESS, addr1);
        status = evl_sqlog_add_array_token(p, TY_UCHAR, &ucarray, 4);
        status = evl_sqlog_add_array_token(p, TY_INT, &iarray, 5);
        status = evl_sqlog_add_array_token(p, TY_SHORT, &sarray, 5);
        status = evl_sqlog_add_array_token(p, TY_LONG, &larray, 5);
        status = evl_sqlog_add_array_token(p, TY_CHAR, &carray, 4);
        status = evl_sqlog_add_array_token(p, TY_LONGLONG, &llarray, 5);
        status = evl_sqlog_add_array_token(p, TY_ULONGLONG, &ullarray, 5);
        status = evl_sqlog_add_array_token(p, TY_LDOUBLE, &ldarray, 2);

       status = evl_sqlog_write(facility, type, severity, p);

	exit(status);
}
