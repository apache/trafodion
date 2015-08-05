/*
 *  Logging library for seaquest based on evlog
 */
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

#ifndef _EVL_SQLOG_WRITER_H_
#define _EVL_SQLOG_WRITER_H_


#include <sys/types.h>
#include <stdarg.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POSIX_SQLOG_ENTRY_MAXLEN      (8 * 1024)
#define POSIX_SQLOG_BINARY   		1
#define EVLSQ_COMMON_HEADERS            200
#define EVLSQ_ERR_BUFF_MAX              201
#define EVLSQ_ERR_BUFF_MIN              202
#define EVLSQ_ERR_INVALID_BUF_VERSION   203
#define EVLSQ_ERR_INVALID_BUF_NOSPACE   204
#define EVLSQ_ERR_BUF_OVERFLOW          205


/*
 * priorities (these are ordered)
 */
#define SQ_LOG_EMERG   0   /* system is unusable */
#define SQ_LOG_ALERT   1   /* action must be taken immediately */
#define SQ_LOG_CRIT    2   /* critical conditions */
#define SQ_LOG_ERR     3   /* error conditions */
#define SQ_LOG_WARNING 4   /* warning conditions */
#define SQ_LOG_NOTICE  5   /* normal but significant condition */
#define SQ_LOG_INFO    6   /* informational */
#define SQ_LOG_DEBUG   7   /* debug-level messages */


/* facility codes */
#define SQ_LOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */

#define SQ_LOG_SEAQUEST    (40<<3) /* LOG facility for seaquest */

/*
 * components (these are ordered)
 */
#define SQEVL_MONITOR   1   /* monitor */
#define SQEVL_SEABED    2
#define SQEVL_DTM       3
#define SQEVL_TSE       4
#define SQEVL_ASE       5
#define SQEVL_AMP       6
#define SQEVL_RECOVERY  7
#define SQEVL_NDCS      8
#define SQEVL_SQL       9
#define SQEVL_LUNMGR   10
#define SQEVL_SECURITY 11
#define SQEVL_STFS     12
#define SQEVL_TRANSDUCER 13  /* also known as SeaPilot -- lower layer */
#define SQEVL_SEABRIDGE 14   /* also known as SeaPilot -- upper layer */
#define SQEVL_NVT       15
#define SQEVL_WMS       16
#define SQEVL_IODRV     17
#define SQEVL_DBSECURITY_AUDIT 18
#define SQEVL_DBSECURITY_EVENT 19

typedef unsigned int posix_sqlog_facility_t;
typedef int posix_sqlog_severity_t;

/*** ATTRIBUTE TYPE ***/

typedef enum tmpl_sq_base_type {
        TY_NONE,        /* absence of value */
        TY_CHAR,        /* signed char, actually */
        TY_UCHAR,
        TY_SHORT,
        TY_USHORT,
        TY_INT,
        TY_UINT,
        TY_LONG,
        TY_ULONG,
        TY_LONGLONG,
        TY_ULONGLONG,
        TY_FLOAT,
        TY_DOUBLE,
        TY_LDOUBLE,
        TY_STRING,
        TY_WCHAR,
        TY_WSTRING,
        TY_ADDRESS,
        TY_STRUCT,
        TY_PREFIX3,
        TY_LIST,        /*for arrays of structs, and when parsing initializers*/
        TY_STRUCTNAME,          /* converted to TY_STRUCT after lookup */
        TY_TYPEDEF,             /* type name better be a typedef name */
        TY_SPECIAL              /* catch-all for special cases */
} tmpl_sq_base_type_t;

/* Information about a data type.  See the table early in template.c. */
typedef struct tmpl_sq_type_info {
        char    ti_size;        /* in bytes */
        char    ti_align;       /* from gcc's __alignof__ */
        char    ti_isScalar;
        char    ti_isInteger;
        const char      *ti_name;
        const char      *ti_format;     /* default format */
} tmpl_sq_type_info_t;

/* Define a data type for common sq evlog header */
typedef struct sq_common_header {
        int	comp_id;        /* component ID */
        int	process_id;     /* process ID */
        int	zone_id;        /* zone ID */
        int	thread_id;      /* thread ID */
} sq_common_header_t;

#define SZALGN(type) sizeof(type), __alignof__(type)
#ifdef __i386__
#define LLALIGN 4
#define ULLALIGN 4
#define DALIGN 4
#define LDALIGN 4
#else
#define LLALIGN __alignof__(long long)
#define ULLALIGN __alignof__(unsigned long long)
#define DALIGN __alignof__(double)
#define LDALIGN __alignof__(long double)
#endif



extern int evl_sqlog_add_token(char *buf, int tk_type, void *tk_value);
extern int evl_sqlog_add_array_token(char *buf, int tk_type, void *tk_value, size_t count);
extern int evl_sqlog_buf_used(char *buf);
extern int evl_sqlog_init(char *buf, size_t buf_maxlen);
extern int evl_sqlog_init_header(char *buf, size_t buf_maxlen, sq_common_header_t *common_tokens);
extern int evl_sqlog_write(posix_sqlog_facility_t facility, int event_type, posix_sqlog_severity_t severity, char *evl_buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _EVL_SQLOG_WRITER_H_ */
