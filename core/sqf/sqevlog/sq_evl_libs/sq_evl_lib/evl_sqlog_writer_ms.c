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



FILE *ftmp = NULL;

/* Seabed library includes */

tmpl_sq_type_info_t _evlSqTmplTypeInfo[] = {
        /* size, align, isScalar, isInteger, name, default format */
        {0, 0,                  0, 0,   "none",         "%#x"},
        {SZALGN(char),          1, 1,   "char",         "%d"},
        {SZALGN(unsigned char), 1, 1,   "uchar",        "%u"},
        {SZALGN(short),         1, 1,   "short",        "%d"},
        {SZALGN(unsigned short),1, 1,   "ushort",       "%u"},
        {SZALGN(int),           1, 1,   "int",          "%d"},
        {SZALGN(unsigned int),  1, 1,   "uint",         "%u"},
        {SZALGN(long),          1, 1,   "long",         "%ld"},
        {SZALGN(unsigned long), 1, 1,   "ulong",        "%lu"},
        {sizeof(long long), LLALIGN,
                                1, 1,   "longlong",     "%Ld"},
        {sizeof(unsigned long long), ULLALIGN,
                                1, 1,   "ulonglong",    "%Lu"},
        {SZALGN(float),         1, 0,   "float",        "%f"},
        {sizeof(double), DALIGN,
                                1, 0,   "double",       "%f"},
        {sizeof(long double), LDALIGN,
                                1, 0,   "ldouble",      "%Lf"},
        {sizeof(char*), 0,      0, 0,   "string",       "%s"},
        {SZALGN(wchar_t),       1, 1,   "wchar",        "%lc"},
        {sizeof(wchar_t*), 0,   0, 0,   "wstring",      "%ls"},
        {SZALGN(void*),         1, 1,   "address",      "%p"},
        {0, 0,                  0, 0,   "struct",       "%#x"},
        {0, 0,                  0, 0,   "prefix3",      "%#x"},
        {0, 0,                  0, 0,   "list",         "%#x"},
        {0, 0,                  0, 0,   "struct",       "%#x"},/*TY_STRUCTNAME*/
        {0, 0,                  0, 0,   "typedef",      "%#x"}, /*TY_TYPEDEF*/
        {0, 0,                  0, 0,   NULL,           NULL}   /* the end */
};


struct sq_buffer_header
{
        unsigned short  version;
        size_t                                  size;
        size_t                                  len;
};


int
check_env(FILE **file)
{
    char cmdline[BUFSIZ];
    char fline[BUFSIZ];
    char tmpfile[BUFSIZ];
    char *b;
    FILE *fcmd;
    long pos;
    char *env_va = getenv("SQ_EVLOG_STDERR");
    if (env_va)
    {
        if (*env_va != '0')
        {
            *file = stderr;
            return 1;
        }
    }
    env_va = getenv("SQ_EVLOG_TMP");
    if (env_va)
    {
        if (*env_va != '0')
        {
            if (ftmp == NULL)
            {
                fcmd = fopen("/proc/self/cmdline", "r");
                fgets(cmdline, sizeof(cmdline), fcmd);
                fclose(fcmd);
                b = basename(cmdline);
                if (b == NULL)
                    sprintf(fline, "%d", getpid());
                else
                    sprintf(fline, "%s.%d", b, getpid());
                sprintf(tmpfile, "/tmp/zevlog.%s", fline);
                ftmp = fopen(tmpfile, "a");
                if (ftmp == NULL)
                {
                    printf("SQ_EVLOG_TMP=1 set, could not open %s\n", tmpfile);
                    abort();
                } else
                {
                    pos = ftell(ftmp);
                    if ((pos != -1) && (pos != 0))
                    {
                        fprintf(ftmp, "\n--- ZEVLOG APPEND ---\n\n");
                        fflush(ftmp);
                    }
                }
            }
            *file = ftmp;
            return 1;
        }
    }
    env_va = getenv("SQ_EVLOG_NONE");
    if (env_va)
    {
        if (*env_va != '0')
            return 2;
    }
    return 0;
}

static size_t
evl_memcpy(void *dest, const void *src, size_t n)
{
    size_t nb = n;
    memcpy(dest, src, nb);
    return nb;
}


#define COPYTOKEN(lt) \
{ \
    lt* val = (lt*) tk_value; \
    int status = evl_checkBufSpace(buf_maxlen, total_used_len, sizeof(lt)); \
    if ( status == 0) { \
        size_t len = evl_memcpy(d,&val,sizeof(lt)); \
            va_len = len; \
    } \
    ret_code = status; \
}

#define COPYPOINTERTOKEN(lt) \
{ \
    lt* val = (lt*) tk_value; \
    int c_status = evl_checkBufSpace(buf_maxlen, total_used_len, sizeof(lt)); \
    if ( c_status == 0) { \
            size_t len = evl_memcpy(d,val,sizeof(lt)); \
            va_len = len; \
    } \
    ret_code = c_status; \
}


int
evl_checkBufSpace(size_t buf_maxlen, size_t occupied_len, size_t add_len)
{
    int bytesLeft = (int)buf_maxlen - (int)occupied_len - (int)add_len;
    if (bytesLeft < 0) {
        return EVLSQ_ERR_BUF_OVERFLOW;
    }
    else
        return 0;
}

int
evl_packString(char *d, const char *s, size_t buf_maxlen, size_t total_used_len, size_t *tk_len_added)
{
    size_t slen = strlen(s) + 1;
    size_t va_len = 0;   /* valid return length */

    int status = evl_checkBufSpace(buf_maxlen, total_used_len, slen);

    if ( status == 0) {
        memcpy(d, s, slen);
        va_len = slen;
    } else {
        return status;
    }
    *tk_len_added = va_len;
    return 0;
}


/*
 * Like packString, except we pack a wide string.
 */
int
evl_packWstring(char *d, wchar_t *s, size_t buf_maxlen, size_t total_used_len, size_t *tk_len_added)
{
    size_t slen = (wcslen(s) + 1) * sizeof(wchar_t);
    size_t len = 0;

    int status = evl_checkBufSpace(buf_maxlen, total_used_len, slen);

    if ( status == 0) {
        len = evl_memcpy(d, (wchar_t *)s, slen);
    } else {
        return status;
    }
    *tk_len_added = len;
    return 0;
}


/* This function packs only one token to the logging buffer
 * valid_len is the length of the string has been packed into the buffer
 * ret_size is the total buffer length has been occupied
 */
int
evl_sqlog_add_token(char *buf, int tk_type, void *tk_value)
{
    struct sq_buffer_header *ph = (struct sq_buffer_header*)buf;
/*  struct sq_buffer_header *ph = buf; */
    if (ph->version != 10) return EVLSQ_ERR_INVALID_BUF_VERSION;
    if (ph->size <= ph->len) return EVLSQ_ERR_INVALID_BUF_NOSPACE;
    char *d = buf + sizeof(struct sq_buffer_header) + ph->len;

    size_t va_len = 0;   /* return added token length */
    size_t buf_maxlen = ph->size;
    size_t total_used_len = ph->len + sizeof(struct sq_buffer_header);
    int ret_code = 0;
    int wstd_err;
    FILE *wstd;

    wstd_err = check_env(&wstd);

    switch (tk_type) {
    case TY_CHAR:
    case TY_UCHAR:
    {
        if (wstd_err == 1)
        {
            int st = (int)(long)tk_value;
            fprintf (wstd, "<%c> ", st);
        }
        COPYTOKEN(char);
        break;
        }
    case TY_SHORT:
    case TY_USHORT:
    {
        if (wstd_err == 1)
        {
            int ust = (int)(long)tk_value;
            fprintf (wstd, "<%d> ", ust);
        }
        COPYTOKEN(short);
        break;
    }
    case TY_INT:
    case TY_UINT:
        if (wstd_err == 1)
        {
            fprintf (wstd, "<%d> ", (int)(long)tk_value);
        }
        COPYTOKEN(int);
        break;
    case TY_LONG:
    case TY_ULONG:
        if (wstd_err == 1)
        {
            fprintf (wstd, "<%ld> ", (long)tk_value);
        }
        COPYTOKEN(long);
        break;
    case TY_LONGLONG:
    case TY_ULONGLONG:
        if (wstd_err == 1)
        {
            fprintf (wstd, "<%lld> ", (long long)tk_value);
        }
        COPYTOKEN(long long);
        break;
    case TY_ADDRESS:
        if (wstd_err == 1)
        {
            fprintf (wstd, "<%p> ", (void*)tk_value);
        }
        COPYTOKEN(void*);
        break;
    case TY_FLOAT:
        if (wstd_err == 1)
        {
            fprintf (wstd, "<%f> ", *((float *)tk_value));
           /* fprintf (wstd, "<%p> ", (void*)&tk_value); */
        }
        COPYPOINTERTOKEN(float);
        break;
    case TY_DOUBLE:
        if (wstd_err == 1)
        {
            fprintf (wstd, "<%f> ", *((double *)tk_value));
        }
        COPYPOINTERTOKEN(double);
        break;
    case TY_LDOUBLE:
        if (wstd_err == 1)
        {
            fprintf (wstd, "<%Lf> ", *((long double *)tk_value));
        }
        COPYPOINTERTOKEN(long double);
        break;
    case TY_STRING:
        {
            char *s = (char *)tk_value;
            if (wstd_err == 1)
            {
                fprintf (wstd, "<%s> ", s);
            }
            ret_code = evl_packString(d, s, buf_maxlen, total_used_len, &va_len);
            break;
    }
    case TY_WCHAR:
    {
        wchar_t ws = (wchar_t )(long)tk_value;
        if (wstd_err == 1)
        {
            fprintf (wstd, "<%c> ", ws);
        }
        COPYTOKEN(wchar_t);
        break;
    }
    case TY_WSTRING:
    {
        wchar_t *s = (wchar_t *)tk_value;
        if (wstd_err == 1)
        {
            fprintf (wstd, "<%ls> ", s);
        }
        ret_code = evl_packWstring(d, s, buf_maxlen, total_used_len, &va_len);
    }
    default: ;
    }

    if (wstd_err == 1)
        fflush(wstd);
    if (ret_code ==0){

        ph->len += va_len;
        d += va_len;
        return 0;
    }
    else
        return ret_code;
}

int
evl_sqlog_add_array_token(char *buf, int tk_type, void *tk_value, size_t count)
{
    struct sq_buffer_header *ph = (struct sq_buffer_header*) buf;
  /*      struct sq_buffer_header *ph = buf;  */
    if (ph->version != 10) return EVLSQ_ERR_INVALID_BUF_VERSION;
    if (ph->size <= ph->len) return EVLSQ_ERR_INVALID_BUF_NOSPACE;
    char *d = buf + sizeof(struct sq_buffer_header) + ph->len;

    size_t buf_maxlen = ph->size;
    size_t total_used_len = ph->len + sizeof(struct sq_buffer_header);
    int ret_code = 0;

    char *array, *a;
    size_t size = _evlSqTmplTypeInfo[tk_type].ti_size;
    int n;
    size_t len = 0;
    int wstd_err;
    FILE *wstd;
    wstd_err = check_env(&wstd);

    /* Next arg is the array size. */
    n = (int)count;
    /* Next arg is the array address. */
    array = (char*)tk_value;

    switch (tk_type) {
    case TY_STRING:
    {
        /* array points to an array of char* */
        char **sarray = (char**)array;
        int i;
        for (i = 0; i < n; i++) {
        if ( wstd_err == 1)
        {
            fprintf (wstd, "<%s> ", sarray[i]);
        }
        ret_code = evl_packString(d, sarray[i], buf_maxlen, total_used_len, &len);
        if (ret_code == 0) {
            total_used_len += len;
            d += len;
        }
        else
            return ret_code;
        }
        break;
    }
    case TY_WSTRING:
    {
         /* array points to an array of wchar_t* */
        wchar_t **warray = (wchar_t**)array;
        int i;
        for (i = 0; i < n; i++) {
            if ( wstd_err == 1)
            {
                fprintf (wstd, "<%ls> ", warray[i]);
            }
            ret_code = evl_packWstring(d, warray[i], buf_maxlen, total_used_len, &len);
            if (ret_code == 0){
                total_used_len += len;
                d += len;
            }
            else
                return ret_code;

        }
        break;
    }
    default:
        for (a = array; n > 0; a += size, n--) {
            ret_code = evl_checkBufSpace(buf_maxlen, total_used_len, size);
            if ( ret_code == 0) {
                len = evl_memcpy(d, a, size);
                total_used_len += len;
                d += len;
            }
            else
                return ret_code;

        }
        break;
    }

    ph->len = total_used_len - sizeof(struct sq_buffer_header);
    if (wstd_err == 1)
        fflush(wstd);
    return 0;

}

int evl_sqlog_buf_used(char *buf)
{
    struct sq_buffer_header *ph = (struct sq_buffer_header*)buf;
    if (ph->version != 10) return 0;
    return ph->len;
}


/* This function allows external calling function to pass common token values to their log message
 * Returns the current total buff length
*/

int evl_sqlog_init_header(char *buf, size_t buf_maxlen, sq_common_header_t *common_tokens)
{
    /* first, check the total buffer size */
    size_t EVLSQ_LOG_ENTRY_MAXLEN = POSIX_SQLOG_ENTRY_MAXLEN - sizeof(struct sq_buffer_header);
    size_t EVLSQ_LOG_ENTRY_MINLEN = sizeof(struct sq_buffer_header) + EVLSQ_COMMON_HEADERS;
    if (!buf) return -1;
    if (buf_maxlen > EVLSQ_LOG_ENTRY_MAXLEN) return EVLSQ_ERR_BUFF_MAX;
    if (buf_maxlen < EVLSQ_LOG_ENTRY_MINLEN) return EVLSQ_ERR_BUFF_MIN;

    memset(buf, 0, buf_maxlen); /* reset buffer content */
    struct sq_buffer_header *ph = (struct sq_buffer_header*) buf;
    ph->version = 10;
    ph->size = buf_maxlen;
    ph->len = 0;

    int wstd_err;
    FILE *wstd;
    wstd_err = check_env(&wstd);
    if (wstd_err == 1)
    {
        time_t tim=time(NULL);
        char *s=ctime(&tim);
        s[strlen(s)-1]=0;        // remove \n
        fprintf (wstd, "==================================\nLog time: %s || ", s);

        char lhost[256];   /* local host name */
        if (gethostname(lhost,sizeof(lhost)) >= 0)
        {
            fprintf (wstd, "Log host: %s || ", lhost);
        }
        fprintf (wstd, "UserID: %d\n", (int)getuid());
        fprintf (wstd, "<ComponentID> <Pid> <ZonIDd> <ThreadID> <Message...>: \n");
        fflush(wstd);
    }
    /* size_t total_used_len = ph->len + sizeof(struct sq_buffer_header); */
    char *di = buf;
    int ret_code = 0;

    int* componentID = (int*)(long)common_tokens->comp_id;
    int* processID = (int*)(long)common_tokens->process_id;
    int* zoneID = (int*)(long)common_tokens->zone_id;
    int* threadID = (int*)(long)common_tokens->thread_id;

    /* Adding component ID token */
    ret_code = evl_sqlog_add_token(di, TY_INT, componentID);
    if ( ret_code != 0) return ret_code;

    /* Adding process ID token, maps to process name */
    ret_code = evl_sqlog_add_token(di, TY_INT, processID);
    if ( ret_code != 0) return ret_code;


    /* Adding zoneID token */
    ret_code = evl_sqlog_add_token(di, TY_INT, zoneID);
    if ( ret_code != 0) return ret_code;


    /* Adding threadID token */
    ret_code = evl_sqlog_add_token(di, TY_INT, threadID);
    if ( ret_code != 0) return ret_code;

    return 0;

}

// local function to scrub UTF-8 strings that aren't really UTF-8
// (cloned from seapilot/source/mal/src/MalUTF8.cpp to avoid sqevlog
// build dependency on mal)

void scrubUTF8String(char * buffer)
{
    // Check to see if the string is a valid UTF-8 string. If it is not,
    // replace any characters that are not valid ASCII with ?'s. (We do this
    // because it is likely that the string has SJIS or UCS2 characters in
    // it instead.)

    // see Wikipedia for a description of valid UTF-8 characters

    bool good = true;  // assume everything is good
    unsigned char * next = (unsigned char *)buffer;

    while ((*next) && (good))
    {
        int32_t expectedUTFCharLength = 0;

        if (*next <= 0x7f)
        {
            // single byte ASCII character, which is fine
            next++;
        }
        else if ((*next & 0xC0) == 0x80)
        {
            // We have a byte that can only occur in the middle or end of a UTF-8
            // character, but we are not in a UTF-8 character; this is invalid.
            // (What is probably true is we are seeing a UCS2 or SJIS character.)
            good = false;
        }
        else if ((*next & 0xE0) == 0xC0)
        {
            expectedUTFCharLength = 2;  // binary 110xxxxx, so a 2-byte character
        }
        else if ((*next & 0xF0) == 0xE0)
        {
            expectedUTFCharLength = 3;  // binary 1110xxxx, so a 3-byte character
        }
        else if ((*next & 0xF8) == 0xF0)
        {
            expectedUTFCharLength = 4;  // binary 11110xxx, so a 4-byte character
        }
        else if ((*next & 0xFC) == 0xF8)
        {
            expectedUTFCharLength = 5;  // binary 111110xx, so a 5-byte character
        }
        else if ((*next & 0xFE) == 0xFC)
        {
            expectedUTFCharLength = 6;  // binary 1111110x, so a 6-byte character
        }
        else  // it is an invalid UTF-8 character
        {
            good = false;
        }

        // if we saw the first byte of what should be a multi-byte UTF-8 character,
        // check its middle and ending bytes (note in that case we have not yet
        // incremented next; next still points to first byte of the character)

        if (expectedUTFCharLength > 0)  // if we found first byte of multi-byte UTF-8 char
        {
            for (int32_t i = 1; good && (i < expectedUTFCharLength); i++)
            {
                if ((next[i] & 0xC0) != 0x80)
                {
                    good = false;  // expected a character of binary form 10xxxxxx
                }
            }

            if (good)
            {
                next += expectedUTFCharLength;  // step to next UTF-8 character
            }
        }
    }

    if (!good)
    {
        // we found a bad character; assume the string is not
        // UTF-8 and replace all bytes that are x80 or above with ?'s
        next = (unsigned char *)buffer;  // start from beginning again
        while (*next)
        {
            if (*next >= 0x80)
            {
                *next = '?';
            }
            next++;
        }
    }
}




int
evl_sqlog_write(posix_sqlog_facility_t facility, int event_type,
        posix_sqlog_severity_t severity, char *evl_buf)
{
    struct sq_buffer_header *ph = (struct sq_buffer_header*) evl_buf;
    char *write_buf = evl_buf + sizeof(struct sq_buffer_header);
    size_t log_data_len = ph->len;
    int flags = 0;
    int wstd_err;
    FILE *wstd;
    char *env_severity;
    wstd_err = check_env(&wstd);
    env_severity=getenv("SQ_EVLOG_SEVERITY_NUM");

    if (wstd_err == 2)
    {
        return 0; // no logging
    }
    if (wstd_err == 1)
    {
        fprintf(wstd, "\nEvent Number:%d, Severity:%d. \n==================================\n\n",
        event_type, severity);
        fflush(wstd);
        return 0; // don't both doing posix_log_write
    }

    //*** QPID Event Logging ***//
    // If this variable is defined (value does not matter!),
    // we log using the original posix evlog call instead of
    // using the new qpid logging function. This is a "safety net".
    char * disableQpidLogging = getenv("SP_DISABLE_EVLOG_QPID_LOGGING");

    if(env_severity)
    {
        if (severity <= atoi(env_severity))
        {
            {
#ifdef EVLOG_OK
                return(posix_log_write(facility, event_type, severity, write_buf,
                       log_data_len, POSIX_SQLOG_BINARY, flags));
#endif
            }
        }
    }
    else
    {
        {
#ifdef EVLOG_OK
            return(posix_log_write(facility, event_type, severity, write_buf,
                   log_data_len, POSIX_SQLOG_BINARY, flags));
#endif
        }
    }
    return 0;
}




