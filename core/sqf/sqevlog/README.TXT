Event Logging Library:



# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@

Summary:
This logging library is developed based on Open Source Linux Event Logging package, evlog.
It is designed to provide flexibility for programs to add customized event token.

Header file:
evl_sqlog_writer.h

Library name:
	//The following two libraries are for all modules except Monitor and seabed
	Static: libevlsq.a
	Shared: libevlsq.so

	//The following two libraries are only for Monitor and seabed
	static: libevlsqms.a
	static: libevlsqms.so


Library location:
After running # make,make install(if you use top-level makefile, you don't need make install), sqevlog library
(libevlsq.so/libevlsq.a and libevlsqms.so/libevlsq.a) will be linked to export/lib32 or export/lib64,
header file evl_sqlog_writer.h is located under export/include/sqevlog/evl_sqlog_writer.h


Logging function provided:
// Initializing logging buffer and common header.
// This function is called by all modules except Monitor
int evl_sqlog_init(char *buf, size_t buf_maxlen)
{
	return 0;
}

// Initializing logging buffer and allow caller to pass values for common header.
// Only monitor and seabed need call this function initialize logging buffer.

int evl_sqlog_init_header(char *buf, size_t buf_maxlen, sq_common_header_t *common_tokens);
{
return 0;
}

//Add user defined event token with values, please make sure the order to call this function to add token should be consistent with token order you defined in event template.

int evl_sqlog_add_token(char *buf, int tk_type, void *tk_value)
{
	return 0;
}

// Similarly this is for adding token with array value
int evl_sqlog_add_array_token(char *buf, int tk_type, void *tk_value, size_t count)
{
	return 0;
}

//This function writes event message
int evl_sqlog_write(posix_sqlog_facility_t facility, int event_type, posix_sqlog_severity_t severity, char *evl_buf)
{
	return 0;
}
*/

Arguments:
Buf:
This argument specifies logging buffer.
Example:
char sqleventBuf[SQLEVENT_BUF_SIZE];
		char* pbuf = sqleventBuf;

buf_maxlen:
This argument specifies logging buffer size.


tk_type:
This argument specifies event token type. Such as int or char.

Tk_value:
This argument specifies event token value.

Facility:
Currently we only have two facilities assigned: SQ_LOG_SEAQUEST and SQ_LOG_AUTHPRIV.
If your logging message belongs to private message, please use SQ_LOG_AUTHPRIV, otherwise use SQ_LOG_SEAQUEST as the logging facility.

Severity:
This argument specifies event severity.
We have defined the following severity types:
#define SQ_LOG_EMERG   0   /* system is unusable */
#define SQ_LOG_ALERT   1   /* action must be taken immediately */
#define SQ_LOG_CRIT    2   /* critical conditions */
#define SQ_LOG_ERR     3   /* error conditions */
#define SQ_LOG_WARNING 4   /* warning conditions */
#define SQ_LOG_NOTICE  5   /* normal but significant condition */
#define SQ_LOG_INFO    6   /* informational */
#define SQ_LOG_DEBUG   7   /* debug-level messages */

event_type:
This argument specifies event number. Note, do not put severity level to this argument.

sq_common_header_t: (Only for Monitor use)
This struct defines common header tokens. User can pass value to this data type.
Currently only int type of value is allowed.
It contains:
typedef struct sq_common_header {
        int  comp_id;        /* component ID */
        int  process_id;     /* process ID */
        int  zone_id;        /* zone ID */
        int  thread_id;      /* thread ID */
} sq_common_header_t;


Using sqevlog library:
Building Environment:

1. If you plan to test event logging on your local machine, please install evlog rpm first.

2. After confirming evlog exists on your system, either use top-level makefile to build the package along with the logging API. Or enter sqevlog directory to run make and make install to build all needed library under../export directory.

3. If SQ_MTYPE and USE_STATIC are not defined, only 32bit and shared library will be generated. If you want to change, modify SQ_MTYPE and USE_STATIC.

Calling library:

1. Define event numbers in the shared event type header file under [home directory]/export/include/common/evl_sqlog_eventnum.h

2. Some useful variables are defined in header file: evl_sqlog_writer.h.
(export/include/sqevlog/evl_sqlog_writer.h)

3. A simple logging example is under: sq_evl_libs/testprogram.

Compiling:
Depending on whether you are using shared or static library.
Monitor and seabed link with -levlsqms
Others link with -levlsq

(-lpthread maybe required)

View events:
5. After logging, command "/sbin/evlview" can be used to display the logged events.  By adding filter option, you can extract some particular events. For example: /sbin/evlview -f 'facility==seaquest && event_type==1021'. You can use man page to learn more about how to use evlview command.

Note: Without template, you won't see correct message since they are in binary format. Simple template has been created, if you only have one add-on string token (such as error message), you don.t need to create any templates. However, if you add more specific tokens, new templates for those events have to be defined.

6. Redirect events to stderr

Setting by environment variable: export SQ_EVLOG_STDERR=true

7. Log events above specified severity
(Severity level from 0 to 7, and 0 is the most emergent events)
For example: Setting by environment variable: export SQ_EVLOG_SEVERITY_NUM=4
In this case, only events with severity 0, 1, 2, 3, 4 will be logged.
//This doesn't affect stderr message

Procedure to build template:

You only need to provide what I described below. I will integrate them into proper template file and compile them into evlog.

Here is the part I need (only content in blue, don.t change other parts):

event_type DTM_INVALID_SYNC_DATA;   /*Put event name or number  you defined in evl_sqlog_template.h file here */
attributes {
        struct commHeader ched;
        /* Add your tokens starting from here. */
        /* For example */
        int queryid;     /*Use tab key before each attribute */
        string errMessage;    /*Use tab key before each attribute */
        /* add more per your requirement */
}
Format   /* display format */
Event Number (event_type) = DTM_INVALID_SYNC_DATA
%ched%
/* Corresponding to tokens you defined in attributes section, please add your tokens starting from here. */
/* For example */
Query ID = %queryid%
Error Message = %errMessage%
/* more .	*/

Uncompleted:

1.	Some common tokens are not getting real values. Stay tuned. I will call seabed function to pass value to the common tokens. However this won.t affect your way to call logging API.



Attachment:

/*
 * Simple logging program for sqevlog.
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "evl_sqlog_writer.h"

main(int argc, char **argv)
{
	int status;
	int event_num;
	long larray[5]={30L,31L,32L,33L,34L};
	int iarray[5] = {1,2,3,4,5};

	long long llarray[5] = {10LL,11LL,12LL,13LL,14LL};
	unsigned long long ullarray[5]={20LL,21LL,22LL,23LL,24L};
	short sarray[5]={1,2,3,4,5};
	char carray[4]={'l','o','v','e'};
	unsigned char ucarray[4]={0xff,0x10,0xfe,0x20};
	long double ldarray[2]={2.33333, 3.333333};
	long double ld1 = 1.7676764;
	long double ld2 = 5.57575757;

	void *addr1 = (void*) 0xfeedf00d;
	void *addr2 = (void*) 0xfacef00d;
	void *addrarray[5] = { NULL, (void*)1, (void*)2, addr1, addr2 };

	char *plal[4] = { "peace", "love", "and", "linux" };

	int facility = SQ_LOG_SEAQUEST;
	event_num = 1;
	int severity = SQ_LOG_EMERG;


      char evl_buf[8 * 1024];

      size_t rec_len = 0;
      size_t va_len = 0;
      char *p = evl_buf;
	float ff = 0.123;
	double dd = 9.999;
	char *cc = 'c';
	int in = 10;


	/* strut is defined for Monitor to pass values to common headers
	* Other modules won't use this struct
*/
	sq_common_header_t* sq_header;
	sq_header = malloc(sizeof(sq_common_header_t));
	sq_header->comp_id = 8888;
	sq_header->process_id = 1122;
	sq_header->zone_id = 57322;
	sq_header->thread_id = 99;


/* Log init function called by all modules except Monitor which will call the next init function */

	  /* status = evl_sqlog_init(p, 7 * 1024);  */

/* Instead of calling the previous function, Monitor and seabed will call the below function to initiate logging buffer */
	  status = evl_sqlog_init_header(p, 7 * 1024, sq_header);

/* Adding personal tokens */
        status = evl_sqlog_add_array_token(p, TY_CHAR, plal, 4);
        status = evl_sqlog_add_token(p, TY_STRING, "LOVE");

        status = evl_sqlog_add_token(p, TY_FLOAT, &ff);

        status = evl_sqlog_add_token(p, TY_DOUBLE, &dd);

        status = evl_sqlog_add_token(p, TY_INT, 10);
        status = evl_sqlog_add_token(p, TY_SHORT, 10);
        status = evl_sqlog_add_token(p, TY_STRING, "LOVE");
        status = evl_sqlog_add_token(p, TY_LONG, 0xf0f0f0f0L);
        status = evl_sqlog_add_token(p, TY_CHAR, cc);
        status = evl_sqlog_add_array_token(p, TY_INT, &iarray, 5);
        status = evl_sqlog_add_array_token(p, TY_LONGLONG, &llarray, 5);

       status = evl_sqlog_write(facility, event_num, severity, p);

	exit(0);
}
